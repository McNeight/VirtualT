/* m100emu.c */

/* $Id: m100emu.c,v 1.1.1.1 2004/08/05 06:46:11 deuce Exp $ */

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
#ifdef _WIN32
#include <windows.h>
#endif

#ifdef __unix__
#include <signal.h>
#endif

#include "io.h"
#include "cpu.h"
#include "doins.h"
#include "display.h"
#include "genwrap.h"
#include "filewrap.h"
#include "roms.h"
#include "intelhex.h"

int fullspeed=0;

uchar	cpu[14];
uchar	memory[65536];
uchar	sysROM[ROMSIZE];
uchar	optROM[ROMSIZE];
char	op[26];
UINT64	cycles=0;
UINT64	instructs=0;
UINT64	last_isr_cycle=0;
int		trace=0;
int		starttime;
FILE	*tracefile;
DWORD	rst7cycles = 9830;
DWORD	one_sec_cycle_count;
DWORD	one_sec_time;
UINT64	one_sec_cycles;
DWORD   update_secs = 0;
float	cpu_speed;
int		gExitApp = 0;
char	gsOptRomFile[256];
DWORD	last_one_sec_time;


extern RomDescription_t		gM100_Desc;
extern int					cROM;
RomDescription_t	*gStdRomDesc;


#ifdef _WIN32
__inline double hirestimer(void)
{
    static LARGE_INTEGER pcount, pcfreq;
    static int initflag;

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

void cpu_delay(int cy)
{
	static double last_instruct=0;

	if(last_instruct==0)
		last_instruct=hirestimer();
	if(!fullspeed) {
		last_instruct+=cy*0.000000416666666667;
		while(hirestimer()<last_instruct) {
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
	cycles+=cy;
	instructs++;
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

void resetcpu(void)
{
	A=0;
	F=0;
	B=0;
	C=0;
	E=0;
	H=0;
	L=0;
	SPH=0;
	SPL=0;
	PCH=0;
	PCL=0;
	IM=0x08;
	cpuMISC=0;
	memcpy(memory,sysROM,ROMSIZE);
	cROM = 0;

}

void load_opt_rom(void)
{
	int				len, c;
	int				fd;
	unsigned short	start_addr;
	char			buf[65536];

	// Clear the option ROM memory
	memset(optROM,0,ROMSIZE);

	// Check if an option ROM is loaded
	if ((len = strlen(gsOptRomFile)) == 0)
		return;

	// Check type of option ROM
	if (((gsOptRomFile[len-1] | 0x20) == 'x') &&
		((gsOptRomFile[len-2] | 0x20) == 'e') &&
		((gsOptRomFile[len-3] | 0x20) == 'h'))
	{
		// Load Intel HEX file
		len = load_hex_file(gsOptRomFile, buf, &start_addr);

		// Check for invalid HEX file
		if ((len > 32768) || (start_addr != 0))
			return;

		// Copy data to optROM
		for (c = 0; c < len; c++)
			optROM[c] = buf[c];
	}
	else
	{
		// Open BIN file
		fd=open(gsOptRomFile,O_RDONLY);
		if(fd!=-1) 
		{
			read(fd, optROM, ROMSIZE);
			close(fd);
		}
	}
}

void initcpu(void)
{
	int	fd;
	int	i;
	FILE	*ram;

	A=0;
	F=0;
	B=0;
	C=0;
	E=0;
	H=0;
	L=0;
	SPH=0;
	SPL=0;
	PCH=0;
	PCL=0;
	IM=0x08;
	cpuMISC=0;

	fd=open("ROM.bin",O_RDONLY | O_BINARY);
	if(fd<0)
		bail("Could not open ROM.bin");
	if(read(fd, sysROM, ROMSIZE)<ROMSIZE)
		bail("Error reading ROM.bin");
	close(fd);

	memcpy(memory, sysROM, ROMSIZE);

	gStdRomDesc = &gM100_Desc;

	for (i = 0; i < RAMSIZE; i++)
		memory[ROMSIZE + i] = 0;

	ram=fopen("RAM.bin", "rb");
	i=1;
	if(ram!=0)
	{
		if(fread(memory+ROMSIZE, 1, RAMSIZE, ram)==RAMSIZE)
			i=0;
		fclose(ram);
	}


	// Load option ROM if any
	load_opt_rom();
}

__inline void check_interrupts(void)
{
	static UINT64	last_rst75=0;
	static DWORD    last_rst_ms = 0;

	if((last_rst75+rst7cycles)<cycles) {
		IM|=0x40;
		if(trace && tracefile != NULL)
			fprintf(tracefile,"RST 7.5 Issued diff = %d\n", (DWORD) (cycles - last_rst75));
		last_rst75=cycles;
	}

	/* TRAP should be first */

	if(RST75PEND && !INTDIS && !RST75MASK) {
		if(trace && tracefile != NULL)
			fprintf(tracefile,"RST 7.5 CALLed\n");
		DECSP2;
		MEM(SP)=PCL;
		MEM(SP+1)=PCH;
		/* MEM16(SP)=PC; */
		PCL=60;
		PCH=0;
		/* PC=60; */
		cpu_delay(10);	/* This may not be correct */
		IM=IM&0xBF;
		last_isr_cycle = cycles;

		if (gDelayUpdateKeys == 1)
			update_keys();
	}

	return;
}

void maint(void)
{
	static time_t systime;
	static float  last_cpu_speed=-1;
	static int    twice_flag = 0;
	DWORD         new_rst7cycles;

	time(&systime);
	if (systime != (time_t) one_sec_time)
	{
		one_sec_cycle_count = (DWORD) (cycles - one_sec_cycles) / 100;
		one_sec_cycles = cycles;
		if (systime != (time_t) one_sec_time)
		{
			/* Update cycle counts only if 2 consecutive seconds - deals with menus */
			if (((time_t)one_sec_time+1 == systime) && ((time_t)last_one_sec_time+2 == systime))
			{
				new_rst7cycles = (DWORD) (0.39998 * one_sec_cycle_count);	  /* 0.39998 = 9830 / 24576 */
				if ((new_rst7cycles > (rst7cycles * 0.8)) || (twice_flag == 2))
				{
					rst7cycles = new_rst7cycles;
					if (rst7cycles < 9830)
						rst7cycles = 9830;
					cpu_speed = (float) (.000097656 * one_sec_cycle_count);  /* 2.4 Mhz / 24576 */
					display_cpu_speed();
					twice_flag = 0;
				}
				else
					twice_flag++;
			}
			last_one_sec_time = one_sec_time;
			one_sec_time = systime;

		}
	}
	process_windows_event();
}

void emulate(void)
{
	unsigned int i;
	unsigned int j;
	int	top=0;
	char *p;
	FILE *fd;
	int	nxtmaint=1;

	starttime=msclock();
	while(!gExitApp) {
#include "do_instruct.h"
		if(trace && tracefile != NULL)
			fprintf(tracefile, "%-22s A:%02X F:%02X B:%02X C:%02X D:%02X E:%02X H:%02X L:%02X PC:%04X SP:%04X IM:%02X *SP:%04X\n"
			,op,A,F,B,C,D,E,H,L,PC,SP,IM,MEM16(SP));
		nxtmaint--;
		if(!nxtmaint) {
			check_interrupts();
			maint();
			nxtmaint=128;
		}
	}

	fd=fopen("RAM.bin", "wb+");
	if(fd!=0)
	{
		fwrite(memory+ROMSIZE, 1, RAMSIZE, fd);
		fclose(fd);
	}
}

#ifdef __unix__
void handle_sig(int sig)
{
	bail("Kill by signal");
}
#endif

int main(int argc, char **argv)
{
	int i;

	for(i=0;i<argc;i++) {
		if(!strcmp(argv[i],"-t"))
			trace=1;
	}

	if (trace)
		tracefile = fopen("trace.txt", "w+");
#ifdef __unix__
	signal(SIGTERM,handle_sig);
	signal(SIGHUP,handle_sig);
	signal(SIGQUIT,handle_sig);
	signal(SIGINT,handle_sig);
#endif
	// Added by JV for prefs
	initpref();
	initcpu();
	init_io();
	initdisplay();
	emulate();
}

