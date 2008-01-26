/* m100emu.c */

/* $Id: m100emu.c,v 1.2 2004/08/31 15:14:40 kpettit1 Exp $ */

/*
 * Copyright 2004 Stephen Hurd and Ken Pettit
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the distribution.
 *
 * THIS SOFTWARE IS PROVIDED BY THE AUTHOR AND CONTRIBUTORS ``AS IS'' AND
 * ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
 * ARE DISCLAIMED.  IN NO EVENT SHALL THE AUTHOR OR CONTRIBUTORS BE LIABLE
 * FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
 * DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS
 * OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION)
 * HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT
 * LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY
 * OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF
 * SUCH DAMAGE.
 */

#include <sys/types.h>
#include <fcntl.h>
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#ifdef _WIN32
#include <windows.h>
#include <direct.h>
#endif

#ifdef __unix__
#include <signal.h>
#include <unistd.h>
#endif

#include "VirtualT.h"
#include "io.h"
#include "cpu.h"
#include "doins.h"
#include "display.h"
#include "genwrap.h"
#include "filewrap.h"
#include "roms.h"
#include "intelhex.h"
#include "setup.h"
#include "memory.h"
#include "m100emu.h"
#include "sound.h"
#include "remote.h"

int		fullspeed=0;
int		gModel = MODEL_M100;

uchar	cpu[14];
extern uchar	*gMemory[64];
extern uchar	gMsplanROM[32768];
extern uchar	gOptROM[32768];

mem_monitor_cb	gMemMonitor = NULL;

char	op[26];
UINT64	cycles=0;
int		cycle_delta;
UINT64	instructs=0;
static UINT64	last_isr_cycle=0;
int		trace=0;
int		starttime;
FILE	*tracefile;
DWORD	rst7cycles = 9830;
DWORD	one_sec_cycle_count;
DWORD	one_sec_time;
UINT64	one_sec_cycles;
DWORD   update_secs = 0;
float	cpu_speed;
volatile int		gExitApp = 0;
volatile int		gExitLoop = 0;
char	gsOptRomFile[256];
int		gShowVersion = 0;
DWORD	last_one_sec_time;
int		gMaintCount = 262888;
int		gOsDelay = 0;
int		gNoGUI = 0;
int		gSocketPort = 0;
int		gRemoteSwitchModel = -1;

//Added J. VERNET

char path[255];
char file[255];

extern RomDescription_t		gM100_Desc;
extern RomDescription_t		gM200_Desc;
extern RomDescription_t		gN8201_Desc;
extern RomDescription_t		gM10_Desc;
extern uchar				gReMem;
extern int					cROM;
extern unsigned char		ioD0;
RomDescription_t			*gStdRomDesc = NULL;


/* Define Debug global variables */
char					gDebugActive = 0;
char					gStopped = 0;
char					gSingleStep = 0;
debug_monitor_callback	gpDebugMonitors[3] = { NULL, NULL, NULL };


#ifdef _WIN32
__inline double hirestimer(void)
{
    static LARGE_INTEGER pcount, pcfreq;
    static int initflag = 0;

    if (!initflag)
	{
	    QueryPerformanceFrequency(&pcfreq);
        initflag++;
    }

    QueryPerformanceCounter(&pcount);
    return (double)pcount.QuadPart / (double)pcfreq.QuadPart;
}
#else
__inline double hirestimer(void)
{
    struct timeval tv;
    gettimeofday(&tv, NULL);
    return (double)tv.tv_sec + (double)tv.tv_usec / 1000000;
}
#endif

double last_instruct=0;
void throttle(int cy)
{
	double	hires;

	if(!fullspeed) 
	{
		if(last_instruct==0)
			last_instruct=hirestimer();

		last_instruct+=cy*0.000000416666666667;

		while((hires = hirestimer())<last_instruct) 
		{
	#ifdef _WIN32
			Sleep(1);
	#else
			struct timespec ts;

			ts.tv_sec=0;
			ts.tv_nsec=10000;
			nanosleep(&ts,NULL);
	#endif
		}
	}
	else
		last_instruct = 0;
	cycles+=cy;
	cycle_delta=0;
//	instructs++;
//	if (hires-last_instruct > 1)
//		last_instruct = hires;
}
/* #define cpu_delay(x)	cycles+=x; instructs++ */

void bail(char *msg)
{
	int endtime;

	endtime=msclock();
	puts(msg);
	printf("%-22s A:%02X F:%02X B:%02X C:%02X D:%02X E:%02X H:%02X L:%02X PC:%04X SP:%04X IM:%02X *SP:%04X\n",op,A,F,B,C,D,E,H,L,PC,SP,IM,MEM16(SP));
	printf("Start: %d  End: %d\n",starttime,endtime);
	printf("Time: %f\n",((double)(endtime-starttime))/1000);
	printf("MHz: %f\n",((UINT64)cycles/(((double)(endtime-starttime))/1000))/1000000);
	exit(1);
}

void jump_to_zero(void)
{
	int		i;

	/* Clear all CPU registers */
	for (i = 0; i < sizeof(cpu); i++)
		cpu[i] = 0;
}


/*
=============================================================================
debug_set_monitor_callback:	This function adds a new debug monitor process.
=============================================================================
*/
int debug_set_monitor_callback(debug_monitor_callback pCallback)
{
	int		x;

	for (x = 0; x < 3; x++)
	{
		if (gpDebugMonitors[x] == NULL)
		{
			gpDebugMonitors[x] = pCallback;
			return x;
		}
	}

	return -1;
}

/*
=============================================================================
debug_clear_monitor_callback:	This function deletes the specified callback 
								from the debug monitor list.
=============================================================================
*/
void debug_clear_monitor_callback(debug_monitor_callback pCallback)
{
	int		x;

	for (x = 0; x < 3; x++)
	{
		if (gpDebugMonitors[x] == pCallback)
		{
			gpDebugMonitors[x] = NULL;
			return;
		}
	}
}

/*
=============================================================================
debug_step:	This function performs a CPU step operation and invokes the 
			callback for each debug monitor to inform it of the CPU step 
			operation.
=============================================================================
*/
void debug_step(void)
{
	int		x;

	for (x = 0; x < 3; x++)
	{
		if (gpDebugMonitors[x] != NULL)
			gpDebugMonitors[x](DEBUG_CPU_STEP);
	}
}

/*
=============================================================================
get_model_string:	This function returns the sring name of the specified
					emulation model.
=============================================================================
*/
void get_model_string(char* str, int model)
{
	switch (model)
	{
	case MODEL_M100:
		strcpy(str, "m100");
		break;
	case MODEL_M102:
		strcpy(str, "m102");
		break;
	case MODEL_T200:
		strcpy(str, "t200");
		break;
	case MODEL_M10:
		strcpy(str, "m10");
		break;
	case MODEL_PC8201:
		strcpy(str, "pc8201");
		break;
	case MODEL_PC8300:
		strcpy(str, "pc8300");
		break;
	}
}

/*
=============================================================================
get_model_from_string:	This function returns the model number given the 
						sring name specified.
=============================================================================
*/
int get_model_from_string(char* str)
{
	if (strcmp("m100", str) == 0)
		return MODEL_M100;
	else if (strcmp("m102", str) == 0)
		return MODEL_M102;
	else if (strcmp("t200", str) == 0)
		return MODEL_T200;
	else if (strcmp("pc8201", str) == 0)
		return MODEL_PC8201;
	else if (strcmp("pc8300", str) == 0)
		return MODEL_PC8300;
	else if (strcmp("m10", str) == 0)
		return MODEL_M10;

	return -1;
}

/*
=============================================================================
get_emulation_path:	This function returns the path of the emulation directory
					based on the supplied model.
=============================================================================
*/
void get_emulation_path(char* emu, int model)
{
	strcpy(emu, path);			/* Copy VirtualT path */
	switch (model)
	{
	case MODEL_M100:
		strcat(emu, "M100/");
		break;
	case MODEL_M102:
		strcat(emu, "M102/");
		break;
	case MODEL_T200:
		strcat(emu, "T200/");
		break;
	case MODEL_M10:
		strcat(emu, "M10/");
		break;
	case MODEL_PC8201:
		strcat(emu, "PC8201/");
		break;
	case MODEL_PC8300:
		strcat(emu, "PC8300/");
		break;
	}
}

/*
=============================================================================
get_rom_path:	This function returns the path of the ROM file for the 
				model specified.  The path is strcpy(ed) into the string 
				supplied.
=============================================================================
*/
void get_rom_path(char* file, int model)
{
	strcpy(file, path);			/* Copy VirtualT path */
	switch (model)
	{
	case MODEL_M100:
		strcpy(file, "M100/M100rom.bin");
		break;
	case MODEL_M102:
		strcpy(file, "M102/M102rom.bin");
		break;
	case MODEL_T200:
		strcpy(file, "T200/T200rom.bin");
		break;
	case MODEL_M10:
		strcpy(file, "M10/m10rom.bin");
		break;
	case MODEL_PC8201:
		strcpy(file, "PC8201/PC8201rom.bin");
		break;
	case MODEL_PC8300:
		strcpy(file, "PC8300/PC8300rom.bin");
		break;
	}
}

/*
=============================================================================
check_model_support:	This function checks for support for the specified 
						Model and returns TRUE if that model is supported.

						Model Support is determined by checking for the 
						appropriate directory structure with the correct ROM
						file.
=============================================================================
*/
int check_model_support(int model)
{
	char	file[256];
	int		fd;


	/* Get the path for the model supplied */
	get_rom_path(file, model);

	/* Attempt to open the ROM file */
	if ((fd = open(file, O_RDONLY)) == -1)
		return FALSE;

	/* Open successful, close the file and return */
	close(fd);
	return TRUE;
}
void model_8201_bug_workaround(int reason)
{
	// Remove debugging after 
	if (PC == 0x6d91)
	{
		lock_remote();
		debug_clear_monitor_callback(model_8201_bug_workaround);
		gDebugActive--;
		unlock_remote();
	}
}
/*
======================================================================
initcpu:	This function initializes the CPU and the system memories
			including loading the ROM and RAM files.
======================================================================
*/
void init_cpu(void)
{
	int		i;

	/* Initialize CPU registers */
	A = F = B = C = D = E = H = L = 0;
	SPH = SPL = 0;
	PCH = PCL = 0;
	IM=0x08;
	cpuMISC=0;
	
	load_sys_rom();

	/* Clear the system memory RAM area */
	for (i = 0; i < RAMSIZE; i++)
		gBaseMemory[RAMSTART + i] = 0;

	/* Read RAM from file in emulation directory */
	load_ram();

	/* Load option ROM if any */
	load_opt_rom();

	/* 
		Work around a stupid PC-8201 bug that couldn't be found.  For some reason,
		in the Windows Release build, the ROM gets stuck looking for RS232 data and
		waiting for SHIFT-BREAK to be pressed in the boot routine.  If I try to trace
		the ROM, the problem goes away, so it has to be timing related somewhere.  I
		found that simply adding delay by "debug tracing" each PC location fixes the
		issue.  The code below adds a 1-time debug hook during boot do add delay that
		is later removed.  This allows delay to be added at boot without adding any 
		additional delay (if statements) during the main portion of the code. This is a 
		horrible kludge, but it fixes the problem.
	*/
#ifdef WIN32
	if (gModel == MODEL_PC8201)
	{
		lock_remote();
		debug_set_monitor_callback(model_8201_bug_workaround);
		gDebugActive++;
		unlock_remote();
	}
#endif

	gExitLoop = 1;
}


void resetcpu(void)
{
	int		i;

	/* Clear all CPU registers */
	for (i = 0; i < sizeof(cpu); i++)
		cpu[i] = 0;

	/* Set interrupt mask */
	IM=0x08;
	cpuMISC=0;

	/* Re-initialize memory */
	reinit_mem();

	/* Check if debug monitor active and updae if there are */
	if (gDebugActive)
	{
		/* Call all debug monitor callback routines */
		for (i = 0; i < 3; i++)
		{
			if (gpDebugMonitors[i] != NULL)
				gpDebugMonitors[i](DEBUG_PC_CHANGED);
		}
	}
	/* 
		Work around a stupid PC-8201 bug that couldn't be found.  For some reason,
		in the Windows Release build, the ROM gets stuck looking for RS232 data and
		waiting for SHIFT-BREAK to be pressed in the boot routine.  If I try to trace
		the ROM, the problem goes away, so it has to be timing related somewhere.  I
		found that simply adding delay by "debug tracing" each PC location fixes the
		issue.  The code below adds a 1-time debug hook during boot do add delay that
		is later removed.  This allows delay to be added at boot without adding any 
		additional delay (if statements) during the main portion of the code. This is a 
		horrible kludge, but it fixes the problem.
	*/
#ifdef WIN32
	if (gModel == MODEL_PC8201)
	{
		lock_remote();
		debug_set_monitor_callback(model_8201_bug_workaround);
		gDebugActive++;
		unlock_remote();
	}
#endif
}
void cb_int65(void)
{
	IM|=0x20;
	if(trace && tracefile != NULL)
		fprintf(tracefile,"RST 6.5 Issued\n");
}


__inline void check_interrupts(void)
{
	static UINT64	last_rst75=0;
	static DWORD    last_rst_ms = 0;

	if (((last_rst75 + rst7cycles) < cycles) && !INTDIS)
	{
		IM|=0x40;
		if(trace && tracefile != NULL)
			fprintf(tracefile,"RST 7.5 Issued diff = %d\n", (DWORD) (cycles - last_rst75));
		last_rst75=cycles;
	}

	/* TRAP should be first */

	if(RST65PEND && !INTDIS && !RST65MASK) {
		if(trace && tracefile != NULL)
			fprintf(tracefile,"RST 6.5 CALLed\n");
		DECSP2;
		if (gReMem)
		{
			MEMSET(SP, PCL);
			MEMSET(SP+1, PCH);
		}
		else
		{
			gBaseMemory[SP] = PCL;
			gBaseMemory[SP+1] = PCH;
		}

		/* MEM16(SP)=PC; */
		PCL=52;
		PCH=0;
		/* PC=52; */
		cycle_delta += 10;	/* This may not be correct */
		IM=IM&0xDF;
		last_isr_cycle = cycles;
	}
	else if(RST75PEND && !INTDIS && !RST75MASK) {
		if(trace && tracefile != NULL)
			fprintf(tracefile,"RST 7.5 CALLed\n");
		DECSP2;
		if (gReMem)
		{
			MEMSET(SP, PCL);
			MEMSET(SP+1, PCH);
		}
		else
		{
			gBaseMemory[SP] = PCL;
			gBaseMemory[SP+1] = PCH;
		}
		/* MEM16(SP)=PC; */
		PCL=60;
		PCH=0;
		/* PC=60; */
		cycle_delta += 10;	/* This may not be correct */
		IM=IM&0xBF;
		last_isr_cycle = cycles;

		if (gDelayUpdateKeys == 1)
			update_keys();
	}
	return;
}

void	mem_set_monitor_callback(mem_monitor_cb cb)
{
	gMemMonitor = cb;
}

void remote_switch_model(int model)
{
	gRemoteSwitchModel = model;
}

void maint(void)
{
	static time_t systime;
	static float  last_cpu_speed=-1;
	static int    twice_flag = 0;
	DWORD         new_rst7cycles;
	static double last_hires = 0;
	double		  hires;

	hires = hirestimer();
	if (hires > last_hires + .05)
//	if (systime != (time_t) one_sec_time)
	{
		last_hires = hires;
		one_sec_cycle_count = (DWORD) (cycles - one_sec_cycles) / 100;
		one_sec_cycles = cycles;
//		if (systime != (time_t) one_sec_time)
		{
			/* Update cycle counts only if 2 consecutive seconds - deals with menus */
//			if (((time_t)one_sec_time+1 == systime) && ((time_t)last_one_sec_time+2 == systime))
			{
				new_rst7cycles = (DWORD) (0.39998 * 20 * one_sec_cycle_count);	  /* 0.39998 = 9830 / 24576 */
				if ((new_rst7cycles > (rst7cycles * 0.8)) || (twice_flag == 2))
				{
					rst7cycles = new_rst7cycles;
					if (rst7cycles < 9830)
						rst7cycles = 9830;
					time(&systime);
					if (systime != (time_t) one_sec_time)
					{
						cpu_speed = (float) (.000097656 * 20 * one_sec_cycle_count);  /* 2.4 Mhz / 24576 */
						display_cpu_speed();
					}
					twice_flag = 0;
					if (gMemMonitor != NULL)
						gMemMonitor();
				}
				else
					twice_flag++;
			}
			last_one_sec_time = one_sec_time;
			one_sec_time = (unsigned long) systime;

		}
	}
	throttle(cycle_delta);
	if (gRemoteSwitchModel != -1)
	{
		switch_model(gRemoteSwitchModel);
		gRemoteSwitchModel = -1;
	}
	handle_simkey();
	process_windows_event();
}


void do_debug_stuff(void)
{
	int		i;

	/* Call all debug monitor callback routines */
	unlock_remote();
	for (i = 0; i < 3; i++)
	{
		if (gpDebugMonitors[i] != NULL)
			gpDebugMonitors[i](DEBUG_PC_CHANGED);
	}

	if (gStopped)
	{
		gOsDelay = 1;
		/* Loop until not stopped or single step */
		while (gStopped && !gSingleStep && !gExitApp)
		{
			process_windows_event();
		}
		gOsDelay = 0;
		gSingleStep = 0;
	}
	lock_remote();
}

/*
========================================================================
emulate:	This routine contains the main loop that handles active 
			emulation and maintenance tasks.
========================================================================
*/
void emulate(void)
{
	unsigned int	i,j;
	unsigned int	v;
	int				top=0;
//har			*p;
	int				nxtmaint=1;
	int				ins;

	starttime=msclock();
	lock_remote();
	while(!gExitApp) 
	{
		/* Run appropriate emulation loop base on ReMem support */
		if (gReMem)
			while (!gExitLoop)
			{
				/* Check for active debug monitor windows */
				if (gDebugActive)
				{
					do_debug_stuff();
					if (gExitLoop) 
						break;
				}

				/* Instruction emulation contained in header file */
				#include "do_instruct.h"

				// Check if next inst is SIM
				if (get_memory8(PC) == 0xF3)
				{
					check_interrupts();
				}

				/* Do maintenance tasks (Windows events, interrupts, etc.) */
				if(!(--nxtmaint & 0x3FF)) 
				{
					unlock_remote();
					gOsDelay = nxtmaint == 0;
					maint();
					check_interrupts();
					if (gOsDelay)
						nxtmaint=gMaintCount;
					lock_remote();
				}
			}

		if (!gReMem)
		{
			while (!gExitLoop)
			{
				/* Check for active debug monitor windows */
				if (gDebugActive)
				{
					do_debug_stuff();
					if (gExitLoop)
						break;
				}

				/* Instruction emulation contained in header file */
				#define NO_REMEM
				#include "cpu.h"
				#include "do_instruct.h"

				// Check if next inst is SIM
				if (get_memory8(PC) == 0xF3)
				{
					check_interrupts();
				}

				/* Do maintenance tasks (Windows events, interrupts, etc.) */
				if(!(--nxtmaint & 0x3FF)) 
				{
					unlock_remote();
					gOsDelay = nxtmaint == 0;
					maint();
					check_interrupts();
					if (gOsDelay)
						nxtmaint=gMaintCount;
					lock_remote();
				}
			}
		}

		gExitLoop = 0;
	}
	unlock_remote();
}

#ifdef __unix__
void handle_sig(int sig)
{
	bail("Kill by signal");
}
#endif

int process_args(int argc, char **argv)
{
	int i;

	for (i = 0; i < argc; i++)
	{
		// Seach for trace flag
		if (!strcmp(argv[i], "-t"))
		{
			trace = 1;				/* Turn on trace mode */
			tracefile = fopen("trace.txt", "w+");
		}

		// Search for No-GUI flag
		if (!strcmp(argv[i], "-nogui"))
			gNoGUI = 1;

		// Search for Socket Port flag
		if (!strncmp(argv[i], "-p", 2))
		{
			// Check if port number is part of flag
			if (strlen(argv[i]) != 2)
			{
				gSocketPort = atoi(&argv[i][2]);
			}
			else
			{
				// Socket number is next argument
				if (i + 1 >= argc)
				{
					printf("%s: -p port not specified\n", argv[0]);
					return 1;
				}
				else
				{
					gSocketPort = atoi(argv[i+1]);
					i++;
				}
			}
		}
	}

	return 0;
}

void setup_unix_signals(void)
{
#ifdef __unix__
	signal(SIGTERM,handle_sig);
	signal(SIGHUP,handle_sig);
	signal(SIGQUIT,handle_sig);
	signal(SIGINT,handle_sig);
#endif
}

void setup_working_path(char **argv)
{
#ifdef __unix__
	int		i;

	getcwd(path, sizeof(path));

	//J. VERNET: Get Absolute Path, as getcwd doesn't return anything when launch from finder
	if (strlen(path) == 0)
	{
		i = strlen(argv[0])-1;
		// Find last '/' in path to remove app name from the path
		while ((argv[0][i] != '/') && (i > 0))
			i--;
		strncpy(path,argv[0], i+1);
	}
	
	// Check if the last character is '/'
	i = strlen(path);
	if (path[i-1] != '/')
		strcat(path, "/");
# else
	_getcwd(path, sizeof(path));
	strcat(path,"\\");
#endif
}

/*
========================================================================
main:	This is the main entry point for the VirtualT application.
========================================================================
*/
int main(int argc, char **argv)
{
	if (process_args(argc, argv))	/* Parse command line args */
		return;

	setup_unix_signals();		/* Setup Unix signal handling */
	setup_working_path(argv);	/* Create a working dir path */

	// Added by JV for prefs
	init_pref();				/* load user Menu preferences */
	load_setup_preferences();	/* Load user Peripheral setup preferences */
	load_memory_preferences();	/* Load user Memory setup preferences */
	
	/* Perform initialization */
	init_mem();					/* Initialize Memory */
	init_io();					/* Initialize I/O structures */
	init_sound();				/* Initialize Sound system */
	init_display();				/* Initialize the Display */
	init_cpu();					/* Initialize the CPU */
	init_remote();				/* Initialize the remote control */

	/* Perform Emulation */
	emulate();					/* Main emulation loop */

	/* Save RAM contents after emulation */
	save_ram();

	/* Cleanup */
	deinit_io();				/* Deinitialize I/O */
	deinit_sound();				/* Deinitialize sound */
	free_mem();					/* Free memory used by ReMem and/or Rampac */
}
