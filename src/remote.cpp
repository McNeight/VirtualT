/* remote.cpp */

/* $Id: remote.cpp,v 1.13 2009/04/05 08:18:35 kpettit1 Exp $ */

/*
 * Copyright 2008 Ken Pettit
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

#include <FL/Fl.H>
#include <FL/Fl_Window.H>
#include <FL/Fl_Preferences.H>
#include <string.h>
#include <stdio.h>
#include <string>

using namespace std;

#include <algorithm>

#ifdef WIN32
#include <cctype>
#else
#include <pthread.h>
#endif

#include "VirtualT.h"
#include "m100emu.h"
#include "remote.h"
#include "socket.h"
#include "serversocket.h"
#include "socketexception.h"
#include "cpu.h"
#include "memory.h"
#include "io.h"
#include "roms.h"
#include "romstrings.h"
#include "disassemble.h"
#include "file.h"

#ifdef WIN32
HANDLE				gRemoteThread;		// The remote control thread
HANDLE				gRemoteLock;		// Lock to access emulation
#else
pthread_t			gRemoteThread;		// The remote control thread
pthread_mutex_t		gRemoteLock;		// Lock to access emulation
#endif

int					gRadix = 10;		// Radix used for reporting values
int					gRemoteBreak[65536];// Storage of breakpoint types
int					gBreakActive = 0;	// Indicates if an active debug was reported
int					gHaltActive = 0;	// Indicates if an active halt was issued
int					gStepOverBreak = 0;	// Indicates if a break was for a "Step Over"
unsigned char		gLastIns = 0;		// Saves last instruction for stepover break
int					gLcdTrapAddr = -1;	// Address of LCD level 6 output routine
int					gLcdRowAddr = -1;	// Address of LCD output row storage
int					gMonitorLcd = 0;	// Flag indicating if LCD is monitored
std::string			gLcdString;			// String being sent to LCD
UINT64				gLcdUpdateCycle;	// Cycle time when LCD last updated
int					gLcdTimeoutCycles;	// Number of cycles for the string timeout
int					gLcdRow = -1;		// Current LCD update row
int					gLcdCol = -1;		// Current LCD update col
int					gLcdColStart = -1;	// Column where current string started
lcd_rect_t			gIgnoreRects[10];
int					gIgnoreCount = 0;
int					gLastDisAddress = 0;
int					gLastDisCount = 10;
int					gLastCmdDis = FALSE;
int					gTraceOpen = 0;
int					gTraceActive = 0;
std::string			gTraceFile;
FILE*				gTraceFileFd;
VTDis				gTraceDis;
char				gTelnetCR;
std::string			gLineTerm = "\n";
std::string			gOk = "Ok";
std::string			gParamError = "Parameter Error\nOk";
std::string			gTelnetCmd;

// Socket interface management structure
typedef struct  
{
	ServerSocket		*serverSock;
	unsigned char		socketShutdown;
	unsigned char		socketOpened;
	unsigned char		socketError;
	unsigned char		socketInitDone;
	unsigned char		telnet;
	unsigned char		cmdLineTelnet;
	unsigned char		enabled;
	unsigned char		termMode;
	unsigned char		iacState;
	unsigned char		sbState;
	unsigned char		sbOption;
    unsigned char       closeSocket;
	int					listenPort;
	int					socketPort;
	int					cmdLinePort;
	ServerSocket 		openSock;
	std::string			socketErrorMsg;
} socket_ctrl_t;

static socket_ctrl_t	gSocket = { NULL, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0 };
char gTelnetTerminal[256];

int remote_cpureg_stop(void);
int remote_cpureg_run(void);
int remote_cpureg_step(void);
void remote_reset(void);
void remote_cold_boot(void);
int remote_load_from_host(const char *filename);
void remote_set_speed(int speed);
int	str_to_i(const char *pStr);
void remote_switch_model(int model);
extern int fullspeed;
extern int fl_wait(double);
extern void simulate_keydown(int key);
extern void simulate_keyup(int key);
extern	Fl_Preferences virtualt_prefs;

extern "C"
{
extern int gDelayUpdateKeys;
extern UINT64 cycles;
extern RomDescription_t	*gStdRomDesc;
void process_windows_event(void);
}

#define LCD_TIMEOUT_NORMAL 100000

/*
=======================================================
Help command:  Send list of supported commands
=======================================================
*/
std::string cmd_help(ServerSocket& sock)
{
	sock << "Help" << gLineTerm << "====" << gLineTerm;
	sock << "  bye" << gLineTerm;
	sock << "  clear_break(cb) address" << gLineTerm;
	sock << "  cold_boot" << gLineTerm;
	sock << "  debug_isr(isr) [on off]" << gLineTerm;
	sock << "  dis address [lines]" << gLineTerm;
	sock << "  exit" << gLineTerm;
	sock << "  flags [all S Z ac P=1 s=0 ...]" << gLineTerm;
	sock << "  halt" << gLineTerm;
	sock << "  in port" << gLineTerm;
	sock << "  key(k) [enter cr f1 esc ctrl+c shift+code+a \"Text\" ...]" << gLineTerm;
	sock << "  lcd_ignore(li) [none (row,col)-(row,col)]" << gLineTerm;
	sock << "  lcd_mon(lm) [on off]" << gLineTerm;
	sock << "  list_break(lb)" << gLineTerm;
	sock << "  load filename" << gLineTerm;
	sock << "  model [m10 m102 t200 pc8201 m10 kc85]" << gLineTerm;
	sock << "  optrom [unload, filename]" << gLineTerm;
	sock << "  out port, value" << gLineTerm;
	sock << "  radix [10 or 16]" << gLineTerm;
	sock << "  read_mem(rm) address [count]" << gLineTerm;
	sock << "  read_reg(rr) [all A B h m DE ...]" << gLineTerm;
	sock << "  reset" << gLineTerm;
	sock << "  run" << gLineTerm;
	sock << "  screen_dump(sd)" << gLineTerm;
	sock << "  set_break(sb) address [main opt mplan ram ram2 ram3 read write]" << gLineTerm;
	sock << "  speed [2.4 friendly max]" << gLineTerm;
	sock << "  status" << gLineTerm;
	sock << "  step(s) [count]" << gLineTerm;
	sock << "  step_over(so) [count]" << gLineTerm;
	sock << "  string address" << gLineTerm;
	sock << "  terminate" << gLineTerm;
	sock << "  write_mem(wm) address [data data data ...]" << gLineTerm;
	sock << "  write_reg(wr) [A=xx B=xx hl=xx ...]" << gLineTerm;
	sock << "  x [lines] (short for \"dis pc\")" << gLineTerm;

	return gOk;
}

/*
=======================================================
LCD String timeout routine
=======================================================
*/
void handle_lcd_timeout()
{
	char	str[120];

	if ((int) (cycles - gLcdUpdateCycle) > gLcdTimeoutCycles)
	{
		// New string being written.  Send the old one
		sprintf(str, "event, lcdwrite, (%d,%d),%s%s", gLcdRow, gLcdColStart,
			gLcdString.c_str(), gLineTerm.c_str());

		if ((gLcdRow != -1) && (gLcdColStart != -1))
			if (gSocket.socketOpened)
				gSocket.openSock << str;
		gLcdUpdateCycle = (UINT64) -1;	
		gLcdRow = -1;
		gLcdColStart = -1;
		gLcdString = "";
	}
}

/*
=======================================================
Remote debug monitor routine.  This is called from the
main execution thread and loop!
=======================================================
*/
void handle_lcd_trap()
{
	int		row, col;
	int		c;
	char	str[120];

	// Get row and column
	row = get_memory8(gLcdRowAddr);
	col = get_memory8(gLcdRowAddr+1);

	// Test if there are any ignore regions
	if (gIgnoreCount)
	{
		// Loop through all ignore rects
		for (c = 0; c < gIgnoreCount; c++)
		{
			// Test if row,col is in this region
			if ((row >= gIgnoreRects[c].top_row) &&
				(row <= gIgnoreRects[c].bottom_row) &&
				(col >= gIgnoreRects[c].top_col) &&
				(col <= gIgnoreRects[c].bottom_col))
			{
				return;
			}
		}
	}

	// Test if this is a new string
	if ((row != gLcdRow) || (col != gLcdCol +1))
	{
		if (gLcdRow != -1)
		{
			// New string being written.  Send the old one
			sprintf(str, "event, lcdwrite, (%d,%d),%s%s", gLcdRow, gLcdColStart,
				gLcdString.c_str(), gLineTerm.c_str());
			if (gSocket.socketOpened)
				gSocket.openSock << str;
		}

		// Start a new string
		gLcdString = "";

		// Now save the Row & Col as the start of the new string
		gLcdColStart = col;
	}

	// Save the byte being written to the display
	gLcdString += C;

	// Now update the current row/col
	gLcdRow = row;
	gLcdCol = col;

	// Save the cycle count for timeout purposes
	gLcdUpdateCycle = cycles;
}

/*
=======================================================
Test for memory read / write operations on monitored
addresses and returns true if a breakpoint address 
access is about to be executed.
=======================================================
*/
int check_mem_access_break(void)
{
	unsigned char	ins;
	int				address, len;
	int				write, read;
	int				mask;

	// Get next instruction
	ins = get_memory8(PC);
	address = -1;		// Default to opcode that doesn't access memory
	len = 1;
	write = FALSE;
	read = FALSE;
	switch (ins)
	{
		case 0x02:	/* STAX B */
			write = TRUE;
		case 0x0A:	/* LDAX B */
			read = !write;
			address = BC;
			break;

		case 0x12:	/* STAX D */
			write = TRUE;
		case 0x1A:	/* LDAX D */
			address = DE;
			read = !write;
			break;

		case 0x32:	/* STA */
			write = TRUE;
		case 0x3A:	/* LDA */
			address = (((int)get_memory8((unsigned short) (PC+1)))|(((int)get_memory8((unsigned short) (PC+2)))<<8));
			read = !write;
			break;

		case 0x11:	/* LXI D */
		case 0x21:	/* LXI H */
		case 0x31:	/* LXI SP */
			address = (((int)get_memory8((unsigned short) (PC+1)))|(((int)get_memory8((unsigned short) (PC+2)))<<8));
			read = TRUE;
			len = 2;
			break;

		case 0x22:	/* SHLD */
			write = TRUE;
		case 0x2A:	/* LHLD */
			address = (((int)get_memory8((unsigned short) (PC+1)))|(((int)get_memory8((unsigned short) (PC+2)))<<8));
			len = 2;
			read = !write;
			break;

		case 0xD9:	/* SHLX */
			address = DE;
			write = TRUE;
			read = FALSE;
			len = 2;
			break;

		case 0xE3:	/* XTHL */
			address = SP;
			len = 2;
			read = TRUE;
			write = TRUE;
			break;

		case 0x34:	/* INR M */
		case 0x35:	/* DCR M */
			read = TRUE;
			write = TRUE;
			address = HL;
			break;

		case 0x36:	/* MVI M */
		case 0x70:	/* MOV M,B */
		case 0x71:	/* MOV M,C */
		case 0x72:	/* MOV M,D */
		case 0x73:	/* MOV M,E */
		case 0x74:	/* MOV M,H */
		case 0x75:	/* MOV M,L */
		case 0x77:	/* MOV M,A */
			write = TRUE;
			address = HL;
			break;

		case 0x46:	/* MOV B,M */
		case 0x4E:	/* MOV C,M */
		case 0x56:	/* MOV D,M */
		case 0x5E:	/* MOV E,M */
		case 0x66:	/* MOV H,M */
		case 0x6E:	/* MOV L,M */
		case 0x7E:	/* MOV A,M */
		case 0x86:	/* ADD M */
		case 0x8E:	/* ADC M */
		case 0x96:	/* SUB M */
		case 0x9E:	/* SBB M */
		case 0xA6:	/* ANA M */
		case 0xAE:	/* XRA M */
		case 0xB6:	/* ORA M */
		case 0xBE:	/* CMP M */
			read = true;
			address = HL;
			break;

		case 0x38:	/* LDES */
			read = TRUE;
			address = PC + 1;
			break;

		case 0xC1:	/* POP B */
		case 0xD1:	/* POP D */
		case 0xE1:	/* POP H */
		case 0xF1:	/* POP PSW */
			address = SP;
			len = 2;
			read = TRUE;
			break;

		case 0xC5:	/* PUSH B */
		case 0xD5:	/* PUSH D */
		case 0xE5:	/* PUSH H */
		case 0xF5:	/* PUSH PSW */
			address = SP-2;
			len = 2;
			write = TRUE;
			break;
	}

	// Test if opcode accesses memory.  If it does, test if the address accessed
	// is monitored and if it is the proper read/write type
	if (address != -1)
	{
		mask = 0;
		if (read)
			mask |= BPTYPE_READ;
		if (write)
			mask |= BPTYPE_WRITE;

		// Test if the address is monitored for read / write
		if (gRemoteBreak[address] & mask)
		{
			return TRUE;
		}

		if ((len == 2) && (address != 63353))
		{
			if (gRemoteBreak[address+1] & mask)
				return TRUE;
		}
	}

	return FALSE;
}

/*
=======================================================
Remote debug monitor routine.  This is called from the
main execution thread and loop!
=======================================================
*/
void cb_remote_debug(int reason)
{
	unsigned char	bank;
	char			str[40];

	// Test if callback is for an interrupt
	if (reason == DEBUG_INTERRUPT)
	{
		return;
	}

	// Check if trace is on
	if (gTraceActive && gTraceOpen)
	{
/*		gTraceDis.DisassembleLine(PC, line);
		len = strlen(line);
		for (c = len; c < 28; c++)
			line[c] = ' ';
		line[c] = 0;

		// Add flags
*/

		// Print to file
		fprintf(gTraceFileFd, "%04X\n", PC);
	}

	// Check if the processor is stopped.  
	if (!gStopped)
	{
		// Check if there is a breakpoint for this address
		if (gRemoteBreak[PC] != 0)
		{
			// Check if the correct bank is selected
			if (PC < ROMSIZE)
			{
				bank = get_rom_bank();
				if ((gModel == MODEL_T200) && (bank != 0))
					bank ^= 0x03;
				if ((bank == 0) && (gRemoteBreak[PC] & BPTYPE_MAIN))
					gStopped = 1;
				if ((bank == 1) && (gRemoteBreak[PC] & BPTYPE_OPT))
					gStopped = 1;
				if ((bank == 2) && (gRemoteBreak[PC] & BPTYPE_MPLAN))
					gStopped = 1;
			}
			else
			{
				bank = get_ram_bank();
				if ((bank == 0) && (gRemoteBreak[PC] & BPTYPE_RAM))
					gStopped = 1;
				if ((bank == 1) && (gRemoteBreak[PC] & BPTYPE_RAM2))
					gStopped = 1;
				if ((bank == 2) && (gRemoteBreak[PC] & BPTYPE_RAM3))
					gStopped = 1;
			}

			// Report break to client socket
			if (gStopped)
			{
				if (gSocket.socketOpened)
				{
					if (gRadix == 10)
						sprintf(str, "event, break, PC=%d%s", PC, gLineTerm.c_str());
					else
						sprintf(str, "event, break, PC=%04X%s", PC, gLineTerm.c_str());
					gSocket.openSock << str;
				}
			}
		}
		// Test for address read / write breakpoints
		if (!gStopped)
		{
			if (check_mem_access_break())
			{
				// Stop the CPU
				gStopped = 1;

				// Report breakpoint to socket interface
				if (gSocket.socketOpened)
				{
					if (gRadix == 10)
						sprintf(str, "event, break, PC=%d%s", PC, gLineTerm.c_str());
					else
						sprintf(str, "event, break, PC=%04X%s", PC, gLineTerm.c_str());
					gSocket.openSock << str;
				}
			}
		}
		// Check for step-over break condition
		if (!gStopped && gStepOverBreak)
		{
			// Check if SP equals gStepOverBreak upon return
			if  (gStepOverBreak == SP)
			{
				// Check if we are about to execute a retrun
				if ((gLastIns == 0xC9) || ((gLastIns & 0xC7) == 0xC0))
				{
					// Stop the processor
					gStopped = 1;
				}
			}
			// Save this instruction for next test
			gLastIns = INS;
		}
	}

	// Trap output to the display
	if ((PC == gLcdTrapAddr) && (get_rom_bank() == 0))
		handle_lcd_trap();
	// Perform LCD string timeout processing
	if (gLcdUpdateCycle != (UINT64) -1)
		handle_lcd_timeout();
}

/*
=======================================================
Parse next argument from argumen list.
=======================================================
*/
std::string get_next_arg(std::string& more_args)
{
	std::string 	next_arg;
	int				pos;

	// Extract string of next byte to be written
	pos = more_args.find(' ');
	if (pos != -1)
	{
		next_arg = more_args.substr(0, pos);
		more_args = more_args.substr(pos+1, more_args.length()-pos-1);
	}
	else
	{
		next_arg = more_args;
		more_args = "";
	}

	return next_arg;
}

/*
=======================================================
Terminate command:  Send list of supported commands
=======================================================
*/
std::string cmd_terminate(ServerSocket& sock)
{
	gExitApp = TRUE;
	gExitLoop = TRUE;
	return gOk;
}

/*
=======================================================
Status command:  Report status of CPU and any async
				 events.
=======================================================
*/
std::string cmd_status(ServerSocket& sock)
{
	char	modelStr[15];
	char	retStr[80];

	get_model_string(modelStr, gModel);

	if (gStopped)
		sprintf(retStr, "Model=%s, CPU halted%s%s", modelStr, 
			gLineTerm.c_str(), gOk.c_str());
	else
		sprintf(retStr, "Model=%s, CPU running%s%s", modelStr,
			gLineTerm.c_str(), gOk.c_str());

	return retStr;
	
}

/*
=======================================================
Halt command:  Halt the CPU
=======================================================
*/
std::string cmd_halt(ServerSocket& sock)
{
	if (!gStopped)
		if (!remote_cpureg_stop())
		{
			lock_remote();
			gDebugActive++;
			gHaltActive = 1;
			gStopped = 1;
			unlock_remote();
		}

	return gOk;
}

/*
=======================================================
Reset command:  Reset the CPU
=======================================================
*/
std::string cmd_reset(ServerSocket& sock)
{
	remote_reset();
	return gOk;
}

/*
=======================================================
Cold Boot command:  Cold boot the machine
=======================================================
*/
std::string cmd_cold_boot(ServerSocket& sock)
{
	remote_cold_boot();
	return gOk;
}

/*
=======================================================
Run command:  Resume the CPU
=======================================================
*/
std::string cmd_run(ServerSocket& sock)
{
	// Check if CPU running.  Continue only if halted
	if (gStopped)
		if (!remote_cpureg_run())
		{
			lock_remote();
			if (gHaltActive)
			{
				gDebugActive--;
				gHaltActive = 0;
			}
			gStopped = 0;
			unlock_remote();
		}

	return gOk;
}

/*
=======================================================
Step command:  Single steps the CPU the specified
				number of steps.
=======================================================
*/
std::string cmd_step(ServerSocket& sock, std::string& args)
{
	int				value;
	std::string 	next_arg;
	std::string 	more_args;
	int				c;

	// If argument list is empty, assume 1
	if (args == "")
	{
		next_arg = "1";
	}
	else
	{
		// Get first parameter if more than 1
		next_arg = get_next_arg(args);
	}

	// Get step count
	value = str_to_i(next_arg.c_str());
	if (value < 0)
		return gParamError;

	// Loop for each step
	for (c = 0; c < value; c++)
	{
		lock_remote();
		gSingleStep = 1;
		unlock_remote();

		while (gSingleStep)
			fl_wait(0.001);
	}
	
	return gOk;
}

/*
=======================================================
Step command:  Single steps the CPU the specified
				number of steps.
=======================================================
*/
std::string cmd_step_over(ServerSocket& sock, std::string& args)
{
	int				value, inst;
	std::string 	next_arg;
	std::string 	more_args;
	int				c, addr;

	// If argument list is empty, assume 1
	if (args == "")
	{
		next_arg = "1";
	}
	else
	{
		// Get first parameter if more than 1
		next_arg = get_next_arg(args);
	}

	// Get step over count
	value = str_to_i(next_arg.c_str());
	if (value < 0)
		return gParamError;

	// Loop for each step
	for (c = 0; c < value; c++)
	{
		// Get the next instruction
		addr = PC;
		inst = get_memory8(addr);

		// Check if instruction is a CALL type inst.
		if (((inst & 0xC7) == 0xC4) || (inst == 0xCD) || ((inst & 0xC7) == 0xC7)) 
		{
/*			if ((inst & 0xC7) == 0xC7)
			{
				if ((inst == 0xCF) || (inst == 0xFF))
				{
					// Its a RST 1 or RST 7 instruction
					addr += 2;
					saveBrk = gRemoteBreak[addr];
					gRemoteBreak[addr] = 0x1F;
					gStepOverBreak = addr;
				}
				else
				{
					// Its an RST instruction
					addr++;
					saveBrk = gRemoteBreak[addr];
					gRemoteBreak[addr] = 0x2F;
					gStepOverBreak = addr;
				}
			}
			else
			{
				// It's a CAlL, CZ, etc. instruction
				addr += 3;
				saveBrk = gRemoteBreak[addr];
				gRemoteBreak[addr] = 0x3F;
				gStepOverBreak = addr;
			}
*/

			// Set gStepOverBreak to current SP */
			gStepOverBreak = SP;

			// Take the processor out of STOP mode
			lock_remote();
			gStopped = 0;
			unlock_remote();

			// Wait for the processor to stop again
			while (!gStopped)
				fl_wait(0.001);

//			gRemoteBreak[addr] = saveBrk;
			gStepOverBreak = 0;
		}
		else
		{
			// Not a CALL or RST - process as single step
			lock_remote();
			gSingleStep = 1;
			unlock_remote();

			// Wait for single step to complete
			while (gSingleStep)
				fl_wait(0.001);
		}
	}
	
	return gOk;
}

int get_address(std::string arg)
{
	int address;

	// Convert address to lower
	std::transform(arg.begin(), arg.end(), arg.begin(), (int(*)(int)) std::tolower);

	// Get address 
	if (arg == "pc")
		address = PC;
	else if (arg == "hl")
		address = HL;
	else if (arg == "de")
		address = DE;
	else if (arg == "bc")
		address = BC;
	else if (arg == "sp")
		address = SP;
	else
		address = str_to_i(arg.c_str());
	
	return address;
}

/*
=======================================================
Write Mem command:  Writes data to memory space
=======================================================
*/
std::string cmd_write_mem(ServerSocket& sock, std::string& args)
{
	int				address;
	int				value;
	std::string 	next_arg;
	std::string 	more_args;

	// Separate the address from the other args
	more_args = args;
	next_arg = get_next_arg(more_args);
	
	// Get address of read
	address = get_address(next_arg);

	// Loop through all data to be written
	lock_remote();
	while (more_args != "")
	{
		// Extract string of next byte to be written
		next_arg = get_next_arg(more_args);

		// Convert string to integer
		value = str_to_i(next_arg.c_str());

		// Write byte to memory
		set_memory8(address++, value);
	}
	unlock_remote();
			
	return gOk;
}

/*
=======================================================
Read Mem command:  Reads data to memory space
=======================================================
*/
std::string cmd_read_mem(ServerSocket& sock, std::string& args)
{
	int				address;
	int				len, c;
	std::string 	next_arg;
	std::string 	more_args;
	std::string 	ret;
	char			str[20];

	// Separate arguments
	more_args = args;
	next_arg = get_next_arg(more_args);

	address = get_address(next_arg);

	// Get length of 
	if (more_args != "")
		len = str_to_i(more_args.c_str());
	else
		len = 1;

	// Build string of return values
	ret = "";
	lock_remote();
	for (c = 0; c < len; c++)
	{
		if (gRadix == 10)
			sprintf(str, "%d ", get_memory8(address++));
		else
			sprintf(str, "%02X ", get_memory8(address++));
	
		ret = ret + str;
	}
	unlock_remote();
	ret = ret + gLineTerm;
	
	return ret + gOk;
}

/*
=======================================================
Read reg command:  Reads registers
=======================================================
*/
std::string cmd_read_reg(ServerSocket& sock, std::string& args)
{
	std::string next_arg;
	std::string more_args;
	std::string format_str;
	std::string format_str16;
	std::string ret="";
	char		reg_str[300];

	// Lock the global access object so the registers are stable
	lock_remote();

	// Parse the registers and build a reply string
	if (args == "all")
	{
		if (gRadix == 10)
			sprintf(reg_str, "A=%d F=%d B=%d C=%d D=%d E=%d H=%d L=%d M=%d PC=%d SP=%d%s%s",
				A, F, B, C, D, E, H, L, get_memory8(HL), PC, SP, gLineTerm.c_str(), gOk.c_str());
		else
			sprintf(reg_str, "A=%02X F=%02X B=%02X C=%02X D=%02X E=%02X H=%02X L=%02X M=%02X PC=%04X SP=%04X%s%s",
				A, F, B, C, D, E, H, L, get_memory8(HL), PC, SP, gLineTerm.c_str(), gOk.c_str());
		unlock_remote();
		return reg_str;
	}
	else
	{
		if (gRadix == 10)
		{
			format_str = "%d ";
			format_str16 = "%d ";
		}
		else
		{
			format_str = "%02X ";
			format_str16 = "%04X ";
		}
		more_args = args;
		// Convert to lower
		std::transform(more_args.begin(), more_args.end(), more_args.begin(), (int(*)(int)) std::tolower);

		while (more_args != "")
		{
			next_arg = get_next_arg(more_args);
			
			// Now figure out which reg it is
			if (next_arg == "a")
			{
				sprintf(reg_str, format_str.c_str(), A);
				ret = ret + reg_str;
			}
			else if (next_arg == "b")
			{
				sprintf(reg_str, format_str.c_str(), B);
				ret = ret + reg_str;
			}
			else if (next_arg == "c")
			{
				sprintf(reg_str, format_str.c_str(), C);
				ret = ret + reg_str;
			}
			else if (next_arg == "d")
			{
				sprintf(reg_str, format_str.c_str(), D);
				ret = ret + reg_str;
			}
			else if (next_arg == "e")
			{
				sprintf(reg_str, format_str.c_str(), E);
				ret = ret + reg_str;
			}
			else if (next_arg == "h")
			{
				sprintf(reg_str, format_str.c_str(), H);
				ret = ret + reg_str;
			}
			else if (next_arg == "l")
			{
				sprintf(reg_str, format_str.c_str(), L);
				ret = ret + reg_str;
			}
			else if (next_arg == "f")
			{
				sprintf(reg_str, format_str.c_str(), F);
				ret = ret + reg_str;
			}
			else if (next_arg == "m")
			{
				sprintf(reg_str, format_str.c_str(), get_memory8(HL));
				ret = ret + reg_str;
			}
			else if (next_arg == "bc")
			{
				sprintf(reg_str, format_str16.c_str(), BC);
				ret = ret + reg_str;
			}
			else if (next_arg == "de")
			{
				sprintf(reg_str, format_str16.c_str(), DE);
				ret = ret + reg_str;
			}
			else if (next_arg == "hl")
			{
				sprintf(reg_str, format_str16.c_str(), HL);
				ret = ret + reg_str;
			}
			else if (next_arg == "pc")
			{
				sprintf(reg_str, format_str16.c_str(), PC);
				ret = ret + reg_str;
			}
			else if (next_arg == "sp")
			{
				sprintf(reg_str, format_str16.c_str(), SP);
				ret = ret + reg_str;
			}

			else
			{
				ret = ret + "xx ";
			}
		}
		unlock_remote();
		return ret + gLineTerm + gOk;
	}
	return gOk;
}

/*
=======================================================
Write reg command:  Writes registers
=======================================================
*/
std::string cmd_write_reg(ServerSocket& sock, std::string& args)
{
	std::string next_arg;
	std::string more_args;
	std::string regStr;
	std::string valueStr;
	int			pos;
	int			value;

	// Convert to lower
	more_args = args;
	std::transform(more_args.begin(), more_args.end(), more_args.begin(), (int(*)(int)) std::tolower);

	// Lock the global access object so the registers are stable
	lock_remote();

	// Loop through all arguments
	while (more_args != "")
	{
		// Get next argument from arg list
		next_arg = get_next_arg(more_args);

		// Split argument into reg and value
		pos = next_arg.find('=');
		if (pos == -1)
		{
			unlock_remote();
			return gParamError;
		}
		regStr = next_arg.substr(0, pos);
		valueStr = next_arg.substr(pos+1, next_arg.length()-pos-1);
		value = str_to_i(valueStr.c_str());

		// Now update the register
		if (regStr == "a")
			A = value;
		else if (regStr == "b")
			B = value;
		else if (regStr == "c")
			C = value;
		else if (regStr == "d")
			D = value;
		else if (regStr == "e")
			E = value;
		else if (regStr == "h")
			H = value;
		else if (regStr == "l")
			L = value;
		else if (regStr == "f")
			F = value;
		else if (regStr == "m")
			set_memory8(HL, value);
		else if (regStr == "bc")
		{
			B = value >> 8;
			C = value & 0xFF;
		}
		else if (regStr == "de")
		{
			D = value >> 8;
			E = value & 0xFF;
		}
		else if (regStr == "hl")
		{
			H = value >> 8;
			L = value & 0xFF;
		}
		else if (regStr == "pc")
		{
			PCH = value >> 8;
			PCL = value & 0xFF;
		}
		else if (regStr == "sp")
		{
			SPH = value >> 8;
			SPL = value & 0xFF;
		}
	}

	// Unlock access to remote control
	unlock_remote();

	return gOk;
}

/*
=======================================================
Load command:  Loads a target file from the host
=======================================================
*/
std::string cmd_load(ServerSocket& sock, std::string& args)
{
	if (remote_load_from_host(args.c_str()))
	{
		std::string ret = "Load Error" + gLineTerm + gOk;
		return ret;
	}

	return gOk;
}

/*
=======================================================
Load command:  Loads a target file from the host
=======================================================
*/
void cb_UnloadOptRom (Fl_Widget* w, void*);

std::string cmd_optrom(ServerSocket& sock, std::string& args)
{
	int err;

	// 
	if (args == "")
	{
		if (strlen(gsOptRomFile) == 0)
		{
			std::string ret = "none" + gLineTerm + gOk;
			return ret;
		}
		sock << gsOptRomFile;
		return gLineTerm + gOk;
	}

	if (args == "unload")
	{
		cb_UnloadOptRom(NULL, NULL);
		return gOk;
	}

	// Load the optrom specified
	std::string ret = gOk;
	err = load_optrom_file(args.c_str());
	if (err == FILE_ERROR_INVALID_HEX)
		ret = "Invalid hex file" + gLineTerm + gOk;
	if (err == FILE_ERROR_FILE_NOT_FOUND)
		ret = "File not found" + gLineTerm + gOk;

	return ret;
}

/*
=======================================================
Radix command:  Changes the radix
=======================================================
*/
std::string cmd_radix(ServerSocket& sock, std::string& args)
{
	char		str[15];

	if (args == "")
	{
		sprintf(str, "%d%s%s", gRadix, gLineTerm.c_str(), gOk.c_str());
		return str;
	}
		
	if ((args != "10") && (args != "16"))
		return gParamError;

	gRadix = str_to_i(args.c_str());
	return gOk;
}

/*
=======================================================
In command:  Prints data from the specified I/O port
=======================================================
*/
std::string cmd_in(ServerSocket& sock, std::string& args)
{
	int		port;
	char	str[20];

	// Ensure there are params
	if (args == "")
		return gParamError;

	// Convert the port # to integer
	port = str_to_i(args.c_str());

	// Report the value based on radix
	if (gRadix == 10)
		sprintf(str, "%d%s%s", inport(port), gLineTerm.c_str(), gOk.c_str());
	else
		sprintf(str, "%02X%s%s", inport(port), gLineTerm.c_str(), gOk.c_str());
	return str;
}

/*
=======================================================
Flags command:  Print or set the CPU flags
=======================================================
*/
std::string cmd_flags(ServerSocket& sock, std::string& args)
{
	std::string 	next_arg;
	std::string 	more_args;
	std::string 	regStr;
	std::string 	valueStr;
	std::string		str;
	int				pos;
	int				value;
	char			cstr[20];

	// Test if flags should be set or printed
	lock_remote();
	if (args == "")
	{
		// Print all flags that are set
		if (ZF) str = str + "Z ";
		if (SF) str = str + "S ";
		if (CF) str = str + "C ";
		if (PF) str = str + "P ";
		if (OV) str = str + "OV ";
		if (AC) str = str + "AC ";
		if (TS) str = str + "TS ";
		if (str == "")  str = "(none)";
		str = str + gLineTerm;
	}
	else
	{
		// Okay we have parameters.  Loop through each and test if setting
		// or requesting status
		more_args = args;

		// Convert to lower
		std::transform(more_args.begin(), more_args.end(), more_args.begin(), (int(*)(int)) std::tolower);

		// Loop through all arguments
		while (more_args != "")
		{
			// Get next argument from arg list
			next_arg = get_next_arg(more_args);

			// Now check if this arg is being set or requested
			pos = next_arg.find('=');
			if (pos == -1)
			{
				// Report the status of this flag
				cstr[0] = 0;
				if ((next_arg == "z") || (next_arg == "all")) {sprintf(cstr, "Z=%d ", ZF);str= str +cstr;}
				if ((next_arg == "s") || (next_arg == "all")) {sprintf(cstr, "S=%d ", SF);str= str +cstr;}
				if ((next_arg == "c") || (next_arg == "all")) {sprintf(cstr, "C=%d ", CF);str= str +cstr;}
				if ((next_arg == "p") || (next_arg == "all")) {sprintf(cstr, "P=%d ", PF);str= str +cstr;}
				if ((next_arg == "ov") || (next_arg == "all")) {sprintf(cstr, "OV=%d ", OV);str= str +cstr;}
				if ((next_arg == "ac") || (next_arg == "all")) {sprintf(cstr, "AC=%d ", AC);str= str +cstr;}
				if ((next_arg == "ts") || (next_arg == "all")) {sprintf(cstr, "TS=%d ", TS);str= str +cstr;}
			}
			else
			{
				// Okay, tring to set a flag, parse the args
				regStr = next_arg.substr(0, pos);
				valueStr = next_arg.substr(pos+1, next_arg.length()-pos-1);
				value = str_to_i(valueStr.c_str());

				// Set the flag
				if (regStr == "z")
				{
					if (value) F |= ZF_BIT; else F &= ~ZF_BIT;
				}
				else if (regStr == "c")
				{
					if (value) F |= CF_BIT; else F &= ~CF_BIT;
				}
				else if (regStr == "s")
				{
					if (value) F |= SF_BIT; else F &= ~SF_BIT;
				}
				else if (regStr == "p")
				{
					if (value) F |= PF_BIT; else F &= ~PF_BIT;
				}
				else if (regStr == "ov")
				{
					if (value) F |= OV_BIT; else F &= ~OV_BIT;
				}
				else if (regStr == "ac")
				{
					if (value) F |= AC_BIT; else F &= ~AC_BIT;
				}
				else if (regStr == "ts")
				{
					if (value) F |= TS_BIT; else F &= ~TS_BIT;
				}
			}
		}
		str = str + gLineTerm;
	}
	str = str + gOk;

	unlock_remote();
	return str;
}

/*
=======================================================
Out command:  Sends data to the specified I/O port
=======================================================
*/
std::string cmd_out(ServerSocket& sock, std::string& args)
{
	int		port;
	int		pos, value;
	std::string		portStr, valStr;

	// Validate there are parameters
	if (args == "")
		return gParamError;

	// Find the comma separating the port and value
	pos = args.find(',');
	if (pos != -1)
	{
		// Comma found, separate port and value strings
		portStr = args.substr(0, pos);
		valStr = args.substr(pos + 1, args.length()-pos-1);
	
		// Check for a space after the comma
		if (valStr[0] == ' ')
			valStr = valStr.substr(1, valStr.length()-1);
	}
	else
	{
		// No comma found, check for space separated params
		pos = args.find(' ');
		if (pos != -1)
		{
			// Space found.  Separate port and value strings
			portStr = args.substr(0, pos);
			valStr = args.substr(pos + 1, args.length()-pos-1);
		}
		else
		{
			// Invalid format
			return gParamError;
		}
	}

	// Convert to integer
	port = str_to_i(portStr.c_str());
	value = str_to_i(valStr.c_str());

	// Perform the out function
	out(port, value);
	return gOk;
}

/*
=======================================================
Set Break command:  Sets a breakpoint and configures
	the remote debugger for debug monitoring if needed.
=======================================================
*/
std::string cmd_set_break(ServerSocket& sock, std::string& args)
{
	std::string next_arg;
	std::string more_args;
	std::string regStr;
	std::string valueStr;
	int			value, address;

	// Get breakpoint arguments
	more_args = args;
	next_arg = get_next_arg(more_args);

	// Convert address
	address = get_address(next_arg);
	if ((address > 65535) || (address < 0))
		return gParamError;

	// Determine the types of breaks
	if (more_args == "")
	{
		// Not specified - break for all
		if (gModel == MODEL_T200)
		{
			if (address < ROMSIZE)
				value = 0x07;			// All T200 Banks
			else
				value = 0x38;			// All T200 Banks
		}
		else
		{
			if (address < ROMSIZE)
				value = 0x03;		// Main & Opt
			else
				value = 0x08;		// RAM
		}
	}
	else
	{
		// Convert to lower
		std::transform(more_args.begin(), more_args.end(), more_args.begin(), (int(*)(int)) std::tolower);

		value = 0;

		// Loop through all arguments
		while (more_args != "")
		{
			// Get next argument from arg list
			next_arg = get_next_arg(more_args);

			// Determine type of this break
			if (next_arg == "ram2") value |= BPTYPE_RAM2;
			if (next_arg == "ram3") value |= BPTYPE_RAM3;
			if (next_arg == "main") value |= BPTYPE_MAIN;
			if (next_arg == "opt") value |= BPTYPE_OPT;
			if (next_arg == "mplan") value |= BPTYPE_MPLAN;
			if (next_arg == "ram") value |= BPTYPE_RAM;
			if (next_arg == "read") value |= BPTYPE_READ;
			if (next_arg == "write") value |= BPTYPE_WRITE;
		}
		
		if (value == 0)
			return gParamError;
	}

	// Okay, now set the breakpoint
	lock_remote();
	gRemoteBreak[address] = value;

	// Check if we need to increment the gDebugActive variable
	if (!gBreakActive)
		gDebugActive++;
	gBreakActive = 1;

	// Unlock access to global vars
	unlock_remote();

	return gOk;
}

/*
=======================================================
Clear Break command:  Clears one or more breakpoints.
=======================================================
*/
std::string cmd_clear_break(ServerSocket& sock, std::string& args)
{
	std::string next_arg;
	std::string more_args;
	int			address;
	int			c;

	// Get breakpoint arguments
	more_args = args;
	next_arg = get_next_arg(more_args);

	// Check if all breakpoints being cleared
	if (next_arg == "all")
	{
		for (c = 0; c < 65536; c++)
			gRemoteBreak[c] = 0;
	}
	else
	{
		// Convert address
		address = get_address(next_arg);
		if ((address > 65535) || (address < 0))
			return gParamError;

		// Clear the breakpoint
		gRemoteBreak[address] = 0;
	}

	// Check if any active breakpoints
	if (gBreakActive)
	{
		for (c = 0; c < 65536; c++)
		{
			if (gRemoteBreak[c] != 0)
				break;
		}

		// If no active breakpoints, decrement gDebugActive
		if (c == 65536)
		{
			lock_remote();
			gDebugActive--;
			gBreakActive = 0;
			unlock_remote();
		}
	}

	return gOk;
}

/*
=======================================================
List Break command:  Lists all breakpoints.
=======================================================
*/
std::string cmd_list_break(ServerSocket& sock, std::string& args)
{
	int		c;
	char	str[80];
	
	for (c = 0; c < 65536; c++)
	{
		if (gRemoteBreak[c] != 0)
		{
			if (gRadix == 10)
				sprintf(str, "%5d: ", c);
			else
				sprintf(str, "%04X: ", c);

			if (gRemoteBreak[c] & BPTYPE_MAIN)
				strcat(str, "main ");
			if (gRemoteBreak[c] & BPTYPE_OPT)
				strcat(str, "opt ");
			if (gRemoteBreak[c] & BPTYPE_MPLAN)
				strcat(str, "mplan ");
			if (gRemoteBreak[c] & BPTYPE_RAM)
				strcat(str, "ram ");
			if (gRemoteBreak[c] & BPTYPE_RAM2)
				strcat(str, "ram2 ");
			if (gRemoteBreak[c] & BPTYPE_RAM3)
				strcat(str, "ram3 ");
			if (gRemoteBreak[c] & BPTYPE_READ)
				strcat(str, "read ");
			if (gRemoteBreak[c] & BPTYPE_WRITE)
				strcat(str, "write ");
			strcat(str, gLineTerm.c_str());
			sock << str;
		}
	}
	return gOk;
}

void key_delay(void)
{
	fl_wait(0.01);
	while (gDelayUpdateKeys)
		fl_wait(0.001);
	fl_wait(0.01);
}
/*
=======================================================
key_press:  Causes a keydown & keyup event for the
			specified key.
=======================================================
*/
void key_press(int key)
{
	static char specialShift[] =  "~!@#$%^&*()_+{}|:\"<>?";
	static char specialUnshft[] = "`1234567890-=[]\\;',./";
	int		index;
	char*	ptr;

	// Test if it is an alpha key and is cap
	if (isalpha(key))
	{
		if (key != tolower(key))
		{
			simulate_keydown(FL_Shift_L);
			key_delay();
			simulate_keydown(tolower(key));
			key_delay();
			simulate_keyup(tolower(key));
			key_delay();
			simulate_keyup(FL_Shift_L);
			key_delay();
			return;
		}
	}
	else if ((ptr = strchr(specialShift, key)) != NULL)
	{
		index = ptr-specialShift;
		simulate_keydown(FL_Shift_L);
		key_delay();
		simulate_keydown(specialUnshft[index]);
		key_delay();
		simulate_keyup(specialUnshft[index]);
		key_delay();
		simulate_keyup(FL_Shift_L);
		key_delay();
		return;
	}
	

	simulate_keydown(key);
	key_delay();
	simulate_keyup(key);
	key_delay();
}

/*
=======================================================
key_press:  Causes a keydown & keyup event for the
			specified key.
=======================================================
*/
int keyword_lookup(std::string& keyword)
{
	// Make the keyword lowercase
	std::transform(keyword.begin(), keyword.end(), keyword.begin(), (int(*)(int)) std::tolower);

	if (keyword == "enter")
		return FL_Enter;
	else if (keyword == "cr")
		return FL_Enter;
	else if (keyword == "esc")
		return FL_Escape;
	else if (keyword == "tab")
		return FL_Tab;
	else if (keyword == "ctrl")
		return FL_Control_L;
	else if ((keyword == "graph") || (keyword == "grph"))
		return FL_Alt_L;
	else if (keyword == "code")
		return FL_Alt_R;
	else if ((keyword == "delete") || (keyword == "del"))
		return FL_Delete;
	else if ((keyword == "insert") || (keyword == "ins"))
		return FL_Insert;
	else if (keyword == "shift")
		return FL_Shift_L;
	else if (keyword == "f1")
		return FL_F + 1;
	else if (keyword == "f2")
		return FL_F + 2;
	else if (keyword == "f3")
		return FL_F + 3;
	else if (keyword == "f4")
		return FL_F + 4;
	else if (keyword == "f5")
		return FL_F + 5;
	else if (keyword == "f6")
		return FL_F + 6;
	else if (keyword == "f7")
		return FL_F + 7;
	else if (keyword == "f8")
		return FL_F + 8;
	else if (keyword == "space")
		return ' ';
	else if (keyword == "left")
		return FL_Left;
	else if (keyword == "right")
		return FL_Right;
	else if (keyword == "up")
		return FL_Up;
	else if (keyword == "down")
		return FL_Down;
	else if (keyword == "label")
		return FL_F + 9;
	else if (keyword == "print")
		return FL_F + 10;
	else if (keyword == "paste")
		return FL_F + 11;
	else if (keyword == "pause")
		return FL_F + 12;
	else if (keyword == "bksp")
		return FL_BackSpace;
	else if (keyword == "backspace")
		return FL_BackSpace;
	else if (keyword == "back")
		return FL_BackSpace;
	else if (keyword == "home")
		return FL_Home;
	else if (keyword == "end")
		return FL_End;
	else if ((keyword == "pgup") || (keyword == "pageup"))
		return FL_Page_Up;
	else if ((keyword == "pgdn") || (keyword == "pagedown"))
		return FL_Page_Down;

	return 0;
}

/*
=======================================================
send_key_list: Sends a list of keywords as a single 
				sequence.
=======================================================
*/
void send_key_list(int* key_list, int count)
{
	int		c;

	for (c = 0; c < count; c++)
	{
		simulate_keydown(key_list[c]);
		key_delay();
	}
	for (c = count-1; c >= 0; c--)
	{
		simulate_keyup(key_list[c]);
		key_delay();
	}
}

/*
=======================================================
Key command:  Simulates a keystroke sequence from the
				keyboard.
=======================================================
*/
std::string cmd_key(ServerSocket& sock, std::string& args)
{
	int			quote;
	int			escape;
	int			len, c;
	int			key;
	int			key_list[50], key_count;
	std::string	keyword;
	
	quote = 0;
	escape = 0;
	key_count = 0;
	len = args.length();

	// Check if it is a simple 1-character key
	if (len == 1)
	{
		key_press(args[0]);
		return gOk;
	}

	// Loop through all charaters
	for (c = 0; c < len; c++)
	{
		// First check if we are in a quote
		if (quote)
		{
			// In a quote string, copy bytes directly except '"' and '\'
			if (args[c] == '"')
			{
				if (escape)
				{
					key_press('"');
					escape = 0;
				}
				else
				{
					quote = 0;
					gLcdTimeoutCycles = LCD_TIMEOUT_NORMAL;
				}
			}
			else if (args[c] == '\\')
			{
				// Test if last char was escape
				if (escape)
				{
					key_press('\\');
					escape = 0;
				}
				else
					escape = 1;
			}
			else
			{
				// Perform a keypress operation for the character
				key_press(args[c]);
			}
		}
		// Search for quote
		else if (args[c] == '"')
		{
			quote = 1;
			gLcdTimeoutCycles = 3000000;
		}
		else if (args[c] == '+')
		{
			// Check if there is an acive keyword
			if (keyword != "")
			{
				// Append the key to the key_sequence array
				if (keyword.length() == 1)
					key = keyword[0];
				else
					key = keyword_lookup(keyword);
				keyword = "";
				key_list[key_count++] = key;
			}
		}
		else if (args[c] == ' ')
		{
			if (keyword != "")
			{
				if (keyword.length() == 1)
					key = keyword[0];
				else
					key = keyword_lookup(keyword);
				keyword = "";
				key_list[key_count++] = key;
			}
			if (key_count != 0)
			{
				send_key_list(key_list, key_count);
				key_count = 0;
			}
		}
		else
		{
			// Build keyword
			keyword += args[c];
		}
	}

	if (keyword != "")
	{
		if (keyword.length() == 1)
			key = keyword[0];
		else
			key = keyword_lookup(keyword);
		key_list[key_count++] = key;
	}
	if (key_count != 0)
	{
		send_key_list(key_list, key_count);
	}

	return gOk;
}

/*
=======================================================
local_switch_model:  Switches to a new model and checks
	if LCD monitoring is turned on.  LCD monitoring 
	must be turned off to switch models because each
	uses a different trap address.
=======================================================
*/
static void local_switch_model(int model)
{
	if (gMonitorLcd)
	{
		lock_remote();
		gMonitorLcd = 0;
		gDebugActive--;
		unlock_remote();
	}

	remote_switch_model(model);
}
/*
=======================================================
Model command:  Sets the emulation model
=======================================================
*/
std::string cmd_model(ServerSocket& sock, std::string& args)
{
	char		str[30];
	char		model[20];
	std::string	more_args = args;

	if (args == "")
	{
		get_model_string(model, gModel);
		sprintf(str, "%s%s%s", model, gLineTerm.c_str(), gOk.c_str());
		return str;
	}
		
	// Convert args to lowercase
	std::transform(more_args.begin(), more_args.end(), more_args.begin(), (int(*)(int)) std::tolower);

	// Check wich model it is
	if ((more_args == "m100") || (more_args == "t102"))
	{
		if (gModel != MODEL_M100)
			local_switch_model(MODEL_M100);
	}
	else if ((more_args == "m102") || (more_args == "t102"))
	{
		if (gModel != MODEL_M102)
			local_switch_model(MODEL_M102);
	}
	else if ((more_args == "m200") || (more_args == "t200"))
	{
		if (gModel != MODEL_T200)
			local_switch_model(MODEL_T200);
	}
	else if (more_args == "m10")
	{
		if (gModel != MODEL_M10)
			local_switch_model(MODEL_M10);
	}
	else if (more_args == "pc8201")
	{
		if (gModel != MODEL_PC8201)
			local_switch_model(MODEL_PC8201);
	}
	else if (more_args == "kc85")
	{
		if (gModel != MODEL_KC85)
			local_switch_model(MODEL_KC85);
	}
	else
		return gParamError;

	fl_wait(0.25);
	return gOk;
}

/*
=======================================================
Speed command:  Sets the emulation speed
=======================================================
*/
std::string cmd_speed(ServerSocket& sock, std::string& args)
{
	std::string	more_args = args;
	std::string ret;

	if (args == "")
	{
		switch (fullspeed)
		{
		case SPEED_REAL: 
			ret = "2.4" + gLineTerm + gOk;
			return ret;
		case SPEED_FRIENDLY1:
		case SPEED_FRIENDLY2: 
			ret = "friendly" + gLineTerm + gOk;
			return ret;
		case SPEED_FULL: 
			ret = "max" + gLineTerm + gOk;
			return ret;
		}
	}

	// Convert args to lowercase
	std::transform(more_args.begin(), more_args.end(), more_args.begin(), (int(*)(int)) std::tolower);

	// Check wich model it is
	if (more_args == "2.4")
	{
		remote_set_speed(SPEED_REAL);
	}
	else if (more_args == "friendly")
	{
		remote_set_speed(SPEED_FRIENDLY1);
	}
	else if (more_args == "max")
	{
		remote_set_speed(SPEED_FULL);
	}
	else
		return gParamError;

	return gOk;
}

/*
=======================================================
get_lcd_coords: Extracts row and column values from
				a string with the format (c,r)
=======================================================
*/
int get_lcd_coords(std::string& str, int& row, int& col)
{
	int		pos, c;

	// Ensure there is a '('
	if (str[0] != '(')
		return 1;

	// Find the ','
	pos = str.find(",");
	if (pos == -1)
		return 1;

	// Find the position of the col value
	for (c = pos+1; c < (int) str.length(); c++)
	{
		if (str[c] != ' ')
			break;
	}

	// Check for valid format
	if (c == str.length())
		return 1;
	if (str[c] == ')')
		return 1;

	// Get the values
	row = atoi(&str.c_str()[1]);
	col = atoi(&str.c_str()[c]);

	return 0;
}

/*
=======================================================
Lcd_ignore command:  Configures regions of the LCD to
					 ignore during monitoring.
=======================================================
*/
std::string cmd_lcd_ignore(ServerSocket& sock, std::string& args)
{
	std::string next_arg;
	std::string more_args;
	int			pos;
	int			top_row, top_col, bottom_row, bottom_col;
	char		str[30];

	// If no args, then list the ignores
	if (args == "")
	{
		if (gIgnoreCount == 0)
		{
			std::string ret = "none" + gLineTerm + gOk;
			return ret;
		}

		for (pos = 0; pos < gIgnoreCount; pos++)
		{
			sprintf(str, "(%d,%d)-(%d,%d)%s", gIgnoreRects[pos].top_row,
				gIgnoreRects[pos].top_col, gIgnoreRects[pos].bottom_row,
				gIgnoreRects[pos].bottom_col, gLineTerm.c_str());
			sock << str;
		}
		return gOk;
	}

	// Ckeck if clearing all ignores
	if (args == "none")
	{
		// Clear the ignore count so all regions are monitored
		gIgnoreCount = 0;
		return gOk;
	}

	// Separate coordinates
	pos = args.find('-');
	if (pos != -1)
	{
		next_arg = args.substr(0, pos);
		more_args = args.substr(pos+1, args.length()-pos-1);
	}
	else
		return gParamError;

	// Get row from first argument
	if (get_lcd_coords(next_arg, top_row, top_col))
		return gParamError;
	if (get_lcd_coords(more_args, bottom_row, bottom_col))
		return gParamError;

	// Update the ignore region
	gIgnoreRects[gIgnoreCount].top_row = top_row;
	gIgnoreRects[gIgnoreCount].top_col = top_col;
	gIgnoreRects[gIgnoreCount].bottom_row = bottom_row;
	gIgnoreRects[gIgnoreCount].bottom_col = bottom_col;
	gIgnoreCount++;

	return gOk;
}

/*
=======================================================
Show reg command:  Outputs the specified value
=======================================================
*/
std::string cmd_show_reg(ServerSocket& sock, int value, int size)
{
	char		str[20];

	if (gRadix == 10)
		sprintf(str, "%d%s%s", value, gLineTerm.c_str(), gOk.c_str());
	else
	{
		if (size == 2)
			sprintf(str, "%04X%s%s", value, gLineTerm.c_str(), gOk.c_str());
		else
			sprintf(str, "%02X%s%s", value, gLineTerm.c_str(), gOk.c_str());
	}

	return str;
}


/*
=======================================================
Read IM command:  Outputs the current interrupt mask
=======================================================
*/
std::string cmd_rim(ServerSocket& sock, std::string& args)
{
	char		str[20];

	if (gRadix == 10)
		sprintf(str, "%d%s%s", IM, gLineTerm.c_str(), gOk.c_str());
	else
	{
		sprintf(str, "%02X%s%s", IM, gLineTerm.c_str(), gOk.c_str());
	}

	return str;
}

/*
=======================================================
Lcd_mon command:  Enables or disables LCD monitoring.
=======================================================
*/
std::string cmd_lcd_mon(ServerSocket& sock, std::string& args)
{
	int		c;

	// Get breakpoint arguments
	if (args == "")
	{
		std::string ret;
		if (gMonitorLcd)
			ret = "on";
		else
			ret = "off";
		return ret + gLineTerm + gOk;
	}

	std::transform(args.begin(), args.end(), args.begin(), (int(*)(int)) std::tolower);
	if (args == "on")
	{
		if (!gMonitorLcd)
		{
			// Configure LCD monitoring
			gLcdRow = -1;
			gLcdCol = -1;
			gLcdColStart = -1;
			gLcdUpdateCycle = (UINT64) -1;
			gLcdTimeoutCycles = LCD_TIMEOUT_NORMAL;

			// Find the LCD Trap address
			for (c = 0; ;c++)
			{
				// Test for end of table
				if (gStdRomDesc->pFuns[c].strnum == -1)
				{
					std::string ret = "Error - no trap vector" + gLineTerm + gOk;
					return ret;
				}

				// Search for the Level 6 plotting function
				if (gStdRomDesc->pFuns[c].strnum == R_CHAR_PLOT_6)
				{
					gLcdTrapAddr = gStdRomDesc->pFuns[c].addr;
					break;
				}
			}

			// Now find the address of the LCD Row storage
			for (c = 0; ;c++)
			{
				// Test for end of table
				if (gStdRomDesc->pVars[c].strnum == -1)
				{
					std::string ret = "Error - no row vector" + gLineTerm + gOk;
					return ret;
				}

				// Search for the Level 6 plotting function
				if (gStdRomDesc->pVars[c].strnum == R_CURSOR_ROW)
				{
					gLcdRowAddr = gStdRomDesc->pVars[c].addr;
					break;
				}
			}

			// Enable LCD monitoring
			gMonitorLcd = 1;
			lock_remote();

			// Increment debug counter
			gDebugActive++;
			unlock_remote();
		}
	}
	else if (args == "off")
	{
		if (gMonitorLcd)
		{
			// Turn off LCD monitoring
			gMonitorLcd = 0;
			lock_remote();

			// Decrement debug counter
			gDebugActive--;
			unlock_remote();
		}
	}
	else
		return gParamError;

	return gOk;
}

/*
=======================================================
Lcd_mon command:  Enables or disables LCD monitoring.
=======================================================
*/
std::string cmd_debug_isr(ServerSocket& sock, std::string& args)
{
	// Get breakpoint arguments
	if (args == "")
	{
		std::string ret;
		if (gDebugInts)
			ret = "on";
		else
			ret = "off";
		return ret + gLineTerm + gOk;
	}

	std::transform(args.begin(), args.end(), args.begin(), (int(*)(int)) std::tolower);
	if (args == "on")
	{
		lock_remote();
		gDebugInts = TRUE;
		unlock_remote();
	}
	else if (args == "off")
	{
		lock_remote();
		gDebugInts = FALSE;
		unlock_remote();
	}
	else
		return gParamError;

	return gOk;
}

/*
=======================================================
string command:  Outputs data at address as a string.
=======================================================
*/
std::string cmd_string(ServerSocket& sock, std::string& args)
{
	int			address;
	int			ch;
	char		build[20];
	std::string	str;

	address = get_address(args);
	while ((ch = get_memory8(address)) != 0)
	{
		if ((ch < ' ') || (ch > '~'))
		{
			if (gRadix == 10)
				sprintf(build, "<%d>", ch);
			else
				sprintf(build,"<%02X>", ch);
			str = str + build;
		}
		else
			str += ch;
		address++;
	}

	str = str + gLineTerm + gOk;
	return str;
}

/*
=======================================================
Disassemble command:  Dissassembles code at the 
					  address specified.
=======================================================
*/
std::string cmd_disassemble(ServerSocket& sock, std::string& args)
{
	std::string next_arg;
	std::string more_args;
	int			address;
	int			lineCount;
	int			dis_addr;
	int			c;
	VTDis		dis;
	char		line[300];

	// Get breakpoint arguments
	more_args = args;
	next_arg = get_next_arg(more_args);

	if (next_arg == "")
		address = gLastDisAddress;
	else
		address = get_address(next_arg);
		
	// Get line count
	if (more_args == "")
		lineCount = gLastDisCount;
	else
		lineCount = str_to_i(more_args.c_str());

	// Create a disassembler
	dis.m_pRom = gStdRomDesc;
	dis.m_WantComments = 1;

	// Disassemble lineCount lines
	dis_addr = address;
	for (c = 0; c < lineCount; c++)
	{
		dis_addr += dis.DisassembleLine(dis_addr, line);
		strcat(line, gLineTerm.c_str());
		sock << line;
	}
	gLastDisAddress = dis_addr;
	gLastDisCount = lineCount;

	return gOk;
}

/*
=======================================================
Trace command:  Enables instruction tracing for all or
				specific address ranges.
=======================================================
*/
std::string cmd_screen_dump(ServerSocket& sock, std::string& args)
{
	int				lines, col, line, first_line = 0;
	unsigned short	lcdAddr, curAddr;

	lines = gModel == MODEL_T200 ? 16 : 8;
	
	// For Model 200, the 1st byte of the LCD character buffer isn't
	// always the first row on the LCD.  There is a 1st row offset
	// variable at FEAEh that tells which row is the 1st row
	if (gModel == MODEL_T200)
		first_line = get_memory8(0xFEAE);

	// Now find the address of the LCD Row storage
#if 0
	for (c = 0; ;c++)
	{
		// Test for end of table
		if (gStdRomDesc->pVars[c].strnum == -1)
		{
			std::string ret = "Couldn't locate LCD buffer" + gLineTerm + gOk;
			return ret;
		}

		// Search for the Level 6 plotting function
		if (gStdRomDesc->pVars[c].strnum == R_LCD_CHAR_BUF)
		{
			lcdAddr = gStdRomDesc->pVars[c].addr;
			break;
		}
	}
#endif

	lcdAddr = gStdRomDesc->sLcdBuf;

	// Now loop through each row and build a string to send
	curAddr = lcdAddr + first_line * 40;
	unsigned short maxAddr = lcdAddr + lines * 40;
	for (line = 0; line < lines; line++)
	{
		std::string line_buf = "";
		unsigned char ch;
		for (col = 0; col < 40; col++)
		{
			ch = get_memory8(curAddr++);
			if ((ch < ' ') || (ch > '~'))
				ch = ' ';
			line_buf += ch;
		}

		// Terminate this line and send to the socket
		line_buf += gLineTerm;
		sock << line_buf;

		if (curAddr >= maxAddr)
			curAddr = lcdAddr;
	}

	return gOk;
}

/*
=======================================================
Trace command:  Enables instruction tracing for all or
				specific address ranges.
=======================================================
*/
std::string cmd_trace(ServerSocket& sock, std::string& args)
{
	std::string		next_arg, more_args;
	std::string		trace_file;
	int				on_off_flag;
	int				create_new_flag;
	char			line[300];


	// Get next argument for trace command
	more_args = args;
	next_arg = get_next_arg(more_args);

	// Set default flags
	on_off_flag = -1;			// Default to "not specified"
	create_new_flag = 0;		// Default to append mode

	// Loop through all arguments
	while (next_arg != "")
	{
		if (next_arg == "off")
			on_off_flag = 0;
		else if (next_arg == "on")
			on_off_flag = 1;
		else if ((next_arg.substr(0, 2) == "0x") || isdigit(next_arg[0]))
		{
			// Handle address argument
		}
		else if (next_arg == "-c")
		{
			// Handle "create new file" flag
			create_new_flag = 1;
		}
		else
		{
			// Must be a filename.  Only 1 filename allowed
			if (trace_file != "")
			{
				sprintf(line, "Error filename \"%s\" already specified%s%s", trace_file.c_str(),
					gLineTerm.c_str(), gOk.c_str());
				return line;
			}
			// Save filename
			trace_file = next_arg;
		}

		// Get next argument
		next_arg = get_next_arg(more_args);
	}

	// Check if a trace file was specified
	if (trace_file != "")
	{
		// If there is an open trace file, then start tracing to a new file
		if (gTraceOpen)
			fclose(gTraceFileFd);

		gTraceOpen = 0;
		gTraceFile = trace_file;

		// If tracing active, then open the new file
		if ((on_off_flag == 1) || (gTraceActive))
		{
			if (create_new_flag)
				gTraceFileFd = fopen(gTraceFile.c_str(), "w");
			else
				gTraceFileFd = fopen(gTraceFile.c_str(), "a");

			// Set flag indicating if trace open
			if (gTraceFileFd != NULL)
				gTraceOpen = 1;
			else
			{
				sock << "Error opening file" << gLineTerm;
			}
		}
	}

	if (on_off_flag)
	{
		// Ensure tracing is turned on, the file is open, and we are in Active Debug mode
		if (!gTraceActive)
		{
			// Update the active debug
			lock_remote();
			gDebugActive++;
			gTraceActive = 1;
			unlock_remote();

			// Open trece file
			if (!gTraceOpen)
			{
				gTraceFileFd = fopen(gTraceFile.c_str(), "wa");
				gTraceOpen = (gTraceFileFd != NULL);
			}
		}
		gTraceDis.m_pRom = gStdRomDesc;
		gTraceDis.m_WantComments = 0;
	}
	else
	{
		// Close any open trace file and end the trace mode
		if (gTraceActive)
		{
			// Update the active debug
			lock_remote();
			gDebugActive--;
			gTraceActive = 0;
			unlock_remote();

			// Close open trece file
			if (gTraceOpen)
			{
				gTraceOpen = 0;
				fclose(gTraceFileFd);
			}
		}
	}

	return gOk;
}

/*
=======================================================
Routine to process TELNET bytes during subnegotiation.
=======================================================
*/
void handle_telnet_sb(unsigned char ch)
{
	char	temp[10];

	switch (gSocket.sbState)
	{
	// Handle first entry into SB state.  The first byte is
	// The option code being negotiatied
	case 0:
		// Just save the TELNET option byte and exit
		gSocket.sbOption = ch;
		gSocket.sbState = 1;

		// If TERMINAL-TYPE option, clear the terminal type
		if (ch == TELNET_TERMTYPE)
			gTelnetTerminal[0] = 0;

		break;

	// Now we have the option byte, check for IS, etc
	case 1:
		switch (ch)
		{
		// We receive the TELNET_IS byte.  Go get the parameter
		case TELNET_IS:
			gSocket.sbState = 2;
			break;
		
		// Unknown...go back to non-telnet IAC mode
		default:
			gSocket.sbState = 0;
			gSocket.iacState = 0;
			break;
		}

	// Receiving the option parameter
	case 2:
		switch (gSocket.sbOption)
		{
		// For TERMTYPE option, save terminal type to global string
		case TELNET_TERMTYPE:
			// Test if TELNET_IAC code during reception of terminal type
			if (ch == TELNET_IAC)
			{
				printf("Term-type: %s\n", gTelnetTerminal);
				gSocket.sbState = 0;
				gSocket.iacState = ch;
				break;
			}

			// Not TELNET_IAC, save to terminal-type
			strcat(gTelnetTerminal, temp);
			break;

		// Other options not supported
		default:
			gSocket.sbState = 0;
			gSocket.iacState = 0;
			break;
		}

	// Unknown case...abort back to non IAC mode
	default:
		gSocket.sbState = 0;
		gSocket.iacState = 0;
		break;
	}
}

/*
=======================================================
Routine to process remote commands
=======================================================
*/
int telnet_command_ready(ServerSocket& sock, std::string &cmd, char* sockData, int len)
{
	int		x, cmd_term;
	char	temp[10];

	cmd_term = FALSE;
	for (x = 0; x < len; x++)
	{
		unsigned char ch = sockData[x];

		/* Test if we are in an IAC mode */
		if (gSocket.iacState != 0)
		{
			switch (gSocket.iacState)
			{
			case TELNET_IAC:
				switch (ch)
				{
				case TELNET_DO:
				case TELNET_DONT:
				case TELNET_WILL:
				case TELNET_WONT:
					gSocket.iacState = ch;
					continue;
				case TELNET_SB:
					gSocket.iacState = ch;
					gSocket.sbState = 0;
					continue;
				case TELNET_AYT:
					sprintf(temp, "%c%c", TELNET_IAC, TELNET_GA);
					sock << temp;
					gSocket.iacState = ch;
					continue;

				default:
					gSocket.iacState = 0;
				}
				continue;

			case TELNET_WILL:
				switch (ch)
				{
					// Process response from our IAC DO TERMINAL_TYPE query
				case TELNET_TERMTYPE:
					sprintf(temp, "%c%c%c%c%c%c", TELNET_IAC, TELNET_SB, TELNET_TERMTYPE,
						TELNET_SEND, TELNET_IAC, TELNET_SE);
					sock << temp;
					gSocket.iacState = 0;
					continue;
				
				default:
					gSocket.iacState = 0;
					continue;
				}
			case TELNET_SB:
				handle_telnet_sb(ch);
				continue;

			default:
				gSocket.iacState = 0;
				continue;
			}
		}
		else if (ch == TELNET_IAC)
		{
			gSocket.iacState = TELNET_IAC;
			continue;
		}

		/* Test for 0x0a command line terminaor */
		if (ch == gTelnetCR)
		{
			char temp[2];
			cmd_term = TRUE;
			cmd = gTelnetCmd;
			gTelnetCmd = "";
			temp[0] = ch;
			temp[1] = 0;
#ifdef WIN32
			sock << temp;
			temp[0] = 0x0A;
			sock << temp;
#endif
			break;
		}
		/* Test for 0x0D and ignore it */
		if (ch == 0x0D)
		{
			char temp[2];
			temp[0] = ch;
			temp[1] = 0;
			temp[1] = 0;
#ifdef WIN32
			sock << temp;
#endif
			continue;
		}
		// Test for backspace
		if (ch == 0x08)
		{
			if (gTelnetCmd.length() == 0)
			{
#ifdef WIN32
				sock << " ";
#endif
				continue;
			}
			gTelnetCmd = gTelnetCmd.substr(0, gTelnetCmd.length()-1);
#ifdef WIN32
			sock << "\x08 \x08";
#endif
			continue;
			
		}
		/* If this character isn't a control char, add it to the cmd */
		if ((ch >= ' ') && (ch <= '~'))
		{
			char temp[2];
			temp[0] = ch;
			temp[1] = 0;
			gTelnetCmd = gTelnetCmd + temp;
#ifdef WIN32
			sock << temp;
#endif
		}
		else
		{
			// we don't need these chars in our command line
		}
	}

	return cmd_term;
}

/*
=======================================================
Routine to process remote commands
=======================================================
*/
void send_telnet_greeting(ServerSocket& sock)
{
	sock << gLineTerm << "Welcome to the VirtualT v" << VERSION << " TELNET interface." << gLineTerm;
	sock << "Use decimal, C hex (0x2a) or Asm hex (14h) for input." << gLineTerm;
	sock << "Return data reported according to specified radix." << gLineTerm;
	sock << "Type 'help' for a list of supported comands." << gLineTerm;
	sock << gOk;
}

/*
=======================================================
Routine to process remote commands
=======================================================
*/
std::string process_command(ServerSocket& sock, char *sockdata, int len)
{
	std::string ret = "Syntax error";
	std::string cmd_word;
	std::string args;
	std::string cmd = sockdata;

	// Test for telnet protocol bytes
	if (gSocket.telnet)
	{
		ret = ret + gLineTerm + gOk;
		if (!telnet_command_ready(sock, cmd, sockdata, len))
			return "";
		if (cmd == "")
		{
			if (gLastCmdDis)
			{
				ret = cmd_disassemble(sock, args);
				return ret;
			}
			return gOk;
		}
	}

	// Separate out the command word from any arguments
	args = cmd;
	cmd_word = get_next_arg(args);

	if ((cmd == "") && gLastCmdDis)
	{
		ret = cmd_disassemble(sock, args);
		return ret;
	}

	if (cmd == "help")				// Check for help
		ret = cmd_help(sock);

	else if ((cmd == "run") || (cmd == "go"))	// Check for run
		ret = cmd_run(sock);

	else if (cmd == "reset")		// Check for reset
		ret = cmd_reset(sock);

	else if (cmd == "terminate")	// Check for terminate
		ret = cmd_terminate(sock);

	else if ((cmd == "exit") || (cmd == "bye"))	// Check for terminate
    {
	    sock << "Thank you for contacting VirtualT v" << VERSION << ".  Bye." << gLineTerm;
        ret = "";
        gSocket.closeSocket = TRUE;
    }

	else if (cmd == "cold_boot")	// Check for terminate
		ret = cmd_cold_boot(sock);

	else if ((cmd == "halt") || (cmd == "stop"))	// Check for halt
		ret = cmd_halt(sock);

	else if (cmd == "status")		// Report status and any events
		ret = cmd_status(sock);

	else if (cmd_word == "load")
		ret = cmd_load(sock, args);

	else if ((cmd_word == "read_mem") || (cmd_word == "rm"))
		ret = cmd_read_mem(sock, args);

	else if ((cmd_word == "write_mem") || (cmd_word == "wm"))
		ret = cmd_write_mem(sock, args);

	else if ((cmd_word == "read_reg") || (cmd_word == "rr"))
		ret = cmd_read_reg(sock, args);

	else if (cmd_word == "regs")
	{
		args = "all";
		ret = cmd_read_reg(sock, args);
	}

	else if ((cmd_word == "write_reg") || (cmd_word == "wr"))
		ret = cmd_write_reg(sock, args);

	else if (cmd_word == "radix")
		ret = cmd_radix(sock, args);

	else if (cmd_word == "in")
		ret = cmd_in(sock, args);

	else if (cmd_word == "out")
		ret = cmd_out(sock, args);

	else if (cmd_word == "flags")
		ret = cmd_flags(sock, args);

	else if ((cmd_word == "step") || (cmd_word == "s"))
		ret = cmd_step(sock, args);

	else if ((cmd_word == "lcd_mon") || (cmd_word == "lm"))
		ret = cmd_lcd_mon(sock, args);

	else if ((cmd_word == "lcd_ignore") || (cmd_word == "li"))
		ret = cmd_lcd_ignore(sock, args);

	else if ((cmd_word == "set_break") || (cmd_word == "sb"))
		ret = cmd_set_break(sock, args);

	else if ((cmd_word == "clear_break") || (cmd_word == "cb"))
		ret = cmd_clear_break(sock, args);

	else if ((cmd_word == "list_break") || (cmd_word == "lb"))
		ret = cmd_list_break(sock, args);

	else if ((cmd_word == "key") || (cmd_word == "k"))
		ret = cmd_key(sock, args);

	else if (cmd_word == "model")
		ret = cmd_model(sock, args);

	else if (cmd_word == "speed")
		ret = cmd_speed(sock, args);

	else if (cmd_word == "dis")
	{
		ret = cmd_disassemble(sock, args);
		gLastCmdDis = TRUE;
		return ret;
	}

	else if (cmd_word == "pc")
		ret = cmd_show_reg(sock, PC, 2);

	else if (cmd_word == "hl")
		ret = cmd_show_reg(sock, HL, 2);

	else if (cmd_word == "sp")
		ret = cmd_show_reg(sock, SP, 2);

	else if (cmd_word == "de")
		ret = cmd_show_reg(sock, DE, 2);

	else if (cmd_word == "bc")
		ret = cmd_show_reg(sock, BC, 2);

	else if (cmd_word == "im")
		ret = cmd_show_reg(sock, IM, 1);

	else if (cmd_word == "a")
		ret = cmd_show_reg(sock, A, 1);

	else if (cmd_word == "b")
		ret = cmd_show_reg(sock, B, 1);

	else if (cmd_word == "c")
		ret = cmd_show_reg(sock, C, 1);

	else if (cmd_word == "d")
		ret = cmd_show_reg(sock, D, 1);

	else if (cmd_word == "e")
		ret = cmd_show_reg(sock, E, 1);

	else if (cmd_word == "h")
		ret = cmd_show_reg(sock, H, 1);

	else if (cmd_word == "l")
		ret = cmd_show_reg(sock, L, 1);

	else if (cmd_word == "m")
		ret = cmd_show_reg(sock, get_memory8(HL), 1);

	else if (cmd_word == "string")
		ret = cmd_string(sock, args);

	else if (cmd_word == "trace")
		ret = cmd_trace(sock, args);

	else if (cmd_word == "optrom")
		ret = cmd_optrom(sock, args);

	else if ((cmd_word == "step_over") || (cmd_word == "so"))
		ret = cmd_step_over(sock, args);

	else if ((cmd_word == "debug_isr") || (cmd_word == "isr"))
		ret = cmd_debug_isr(sock, args);

	else if ((cmd_word == "screen_dump") || (cmd_word == "sd"))
		ret = cmd_screen_dump(sock, args);

	else if (cmd_word == "rim")
		ret = cmd_rim(sock, args);

	else if (cmd_word == "x")
	{
		args = "pc " + args;
		ret = cmd_disassemble(sock, args);
		gLastCmdDis = TRUE;
		return ret;
	}
	else if (cmd_word.find('=') != -1)
		ret = cmd_write_reg(sock, cmd);

	gLastCmdDis = FALSE;

	return ret;
}

void initiate_termtype_negotiation(ServerSocket& sock)
{
	char		buf[4];

	sprintf(buf, "%c%c%c", TELNET_IAC ,TELNET_DO, TELNET_TERMTYPE);
	sock << buf;
	gSocket.termMode = REMOTE_TERM_INIT;
}

/*
=======================================================
Routine that will become the remote control thread.
=======================================================
*/
void* remote_control(void* arg)
{
	std::string  ret;
	char		 errmsg[80];
	char		 sockData[128];
	int			 len;

	// Create a listener socket on the desired port number
	gSocket.listenPort = 0;
	gSocket.socketOpened = FALSE;
	gSocket.closeSocket = FALSE;
	gSocket.serverSock = new ServerSocket(gSocket.socketPort);
	gSocket.socketErrorMsg = "";
	if (gSocket.serverSock == NULL)
	{
		gSocket.socketError = TRUE;
		sprintf(errmsg, "Unable to open port %d", gSocket.socketPort);
		gSocket.socketErrorMsg = errmsg;
		return NULL;
	}
	gSocket.listenPort = gSocket.socketPort;

	// Now listen on the listener port for connections
	try
	{
		while (!gExitApp && !gSocket.socketShutdown)
		{
			gSocket.serverSock->accept(gSocket.openSock);
			try
			{
				gSocket.socketError = FALSE;
				gSocket.socketErrorMsg = "";
				gSocket.socketOpened = TRUE;
                gSocket.closeSocket = FALSE;

				// Negotiate for the terminal type
//				initiate_termtype_negotiation(gSocket.openSock);

				// Send greeting if in telnet mode
				if (gSocket.telnet)
					send_telnet_greeting(gSocket.openSock);

				while (!gExitApp && !gSocket.socketShutdown && !gSocket.closeSocket)
				{
					std::string data;
//					gSocket.openSock >> data;

					len = gSocket.openSock.recv(sockData, sizeof(sockData));

					if (gSocket.socketShutdown)
						break;
					ret = process_command(gSocket.openSock, sockData, len);

                    /* Test for exit / bye command to close the socket */
                    if (gSocket.closeSocket)
                    {
                        gSocket.openSock << ret;
                        gSocket.openSock.shutdown();
                    }
                    else if (gSocket.telnet)
					{
						if (ret != "")
							gSocket.openSock << ret;
					}
					else
						gSocket.openSock << ret;
				}
			}
			catch (SocketException&)
			{
				gSocket.socketOpened = FALSE;
			}
		}
	}
	catch (SocketException& e)
	{
		if (!gSocket.socketShutdown)
		{
			gSocket.socketError  = TRUE;
			gSocket.socketErrorMsg = e.description();
		}
	}

	// Unhook from the debug callback processor
	debug_clear_monitor_callback(cb_remote_debug);

	// Shutdown and close the server listener socket
	gSocket.socketOpened = FALSE;
	delete gSocket.serverSock;
	gSocket.serverSock = NULL;
	gSocket.listenPort = 0;
	gSocket.enabled = FALSE;
	
	return NULL;
}

/*
=======================================================
Tears down the remote control socket inerface.
=======================================================
*/
void deinit_remote(void)
{
	// First teardown the listener socket so we don't accept anything new
	gSocket.socketShutdown = TRUE;
	if (gSocket.serverSock != NULL)
	{
		// Just shutdown the socket.  The remote thread will delete it
		try {
			gSocket.serverSock->shutdown();
			gSocket.listenPort = 0;
		}
		catch (SocketException &e)
		{
			gSocket.socketError = TRUE;
			gSocket.socketErrorMsg = e.description();
		}
	}

	// Now check for an opened socket.  If none opened, then we are done
	if (gSocket.socketOpened == FALSE)
		return;

	// Try to shutdown the open socket
	try {
		gSocket.openSock.shutdown();
	}
	catch (SocketException &e)
	{
		gSocket.socketError = TRUE;
		gSocket.socketErrorMsg = e.description();
		gSocket.socketOpened = FALSE;
	}

	// Wait for the thread to report shutdown
	int retry = 0;
	while (retry < 200)
	{
		if (gSocket.socketOpened == FALSE)
			break;
		Fl::wait(.01);
	}
}

/*
=======================================================
Configure remote control using sockets inerface.
=======================================================
*/
void init_remote(void)
{
	int 	c;

	/*
	Test if we need to override the preferences settings
    with the command line settings during the 1st 
	invocation of init_remote
	*/
	if (gSocket.cmdLinePort)
	{
		gSocket.socketPort = gSocket.cmdLinePort;
		gSocket.telnet = gSocket.cmdLineTelnet;
		gSocket.cmdLinePort = 0;
		gSocket.cmdLineTelnet = 0;
		gSocket.enabled = TRUE;
	}
	
	// Check if Remote Socket interface enabled
	if ((gSocket.socketPort == 0) || (gSocket.enabled == FALSE))
		return;

	// Initialize breakpoints
	for (c = 0; c < 65536; c++)
	{
		// Zero all breakpoints
		gRemoteBreak[c] = 0;
	}

	// Hook our routine to the debug monitor
	debug_set_monitor_callback(cb_remote_debug);

	// Setup the line termination for \r\n if in telnet mode
	if (gSocket.telnet)
	{
		gLineTerm = "\r\n";
		gOk = "Ok> ";
		gParamError = "Parameter Error" + gLineTerm + "Ok> ";
#ifdef WIN32
		gTelnetCR = '\r';
#else
		gTelnetCR = '\n';
#endif
	}

	// Clear the shutdown flag
	gSocket.socketShutdown = FALSE;

	// Create the thread
#ifdef WIN32
	DWORD id;
	WORD wVersionRequested; 
	WSADATA wsaData; 
	int err; 

	if (!gSocket.socketInitDone)
	{
		gRemoteLock = CreateMutex(NULL, FALSE, NULL);

		wVersionRequested = MAKEWORD( 1, 1 ); 

		err = WSAStartup( wVersionRequested, &wsaData ); 
		if ( err != 0 ) 
		{ 
			/* Tell the user that we couldn't find a useable */ 
			/* winsock.dll.                                  */ 
			return; 
		} 

		/* Confirm that the Windows Sockets DLL supports 1.1.*/ 
		/* Note that if the DLL supports versions greater    */ 
		/* than 1.1 in addition to 1.1, it will still return */ 
		/* 1.1 in wVersion since that is the version we      */ 
		/* requested.                                        */ 

		if ( LOBYTE( wsaData.wVersion ) != 1 || 
				HIBYTE( wsaData.wVersion ) != 1 ) 
		{ 
			/* Tell the user that we couldn't find a useable */ 
			/* winsock.dll.                                  */ 
			WSACleanup( ); 
			return;    
		} 
	}

	gRemoteThread = CreateThread( NULL, 0,
			(LPTHREAD_START_ROUTINE) remote_control,
			NULL, 0, &id);

#else
	if (!gSocket.socketInitDone)
	{
		// Initialize thread processing
		pthread_mutex_init(&gRemoteLock, NULL);
	}

	pthread_create(&gRemoteThread, NULL, remote_control, NULL);
#endif

	// Set Init flag so we only init once
	gSocket.socketInitDone = TRUE;
}

volatile long gRemoteThreadId = 0L;

void lock_remote(void)
{
	if (gSocket.socketPort != 0)
#ifdef WIN32
//		WaitForSingleObject(gRemoteLock, INFINITE);
		InterlockedCompareExchange(&gRemoteThreadId, GetCurrentThreadId(), 0);
#else
		pthread_mutex_lock(&gRemoteLock);
#endif
}

void unlock_remote(void)
{
	if (gSocket.socketPort != 0)
#ifdef WIN32
//		ReleaseMutex(gRemoteLock);
		InterlockedCompareExchange(&gRemoteThreadId, 0, GetCurrentThreadId());
#else
		pthread_mutex_unlock(&gRemoteLock);
#endif
}


/*
=======================================================
Set socket port from command line.  Overrides initial
setting from preferences.
=======================================================
*/
void set_remote_cmdline_port(int port)
{
	gSocket.cmdLinePort = port;
	gSocket.enabled = TRUE;
}

/*
=======================================================
Set socket telnet from command line.  Overrides initial
setting from preferences.
=======================================================
*/
void set_remote_cmdline_telnet(int telnet)
{
	gSocket.cmdLineTelnet = telnet;
}

/*
=======================================================
Set socket port from preferences.
=======================================================
*/
void set_remote_port(int port)
{
	gSocket.socketPort = port;
}

/*
=======================================================
Set socket telnet from preferences.
=======================================================
*/
void set_remote_telnet(int telnet)
{
	gSocket.telnet = telnet;
}

/*
=======================================================
Get the socket listening port.  This will return zero
if the listener port is not open.
=======================================================
*/
int get_remote_listen_port(void)
{
	return gSocket.listenPort;
}

/*
=======================================================
Get the socket telnet enable status
=======================================================
*/
int get_remote_telnet(void)
{
	return gSocket.telnet;
}

/*
=======================================================
Get the socket error status.  If this returns TRUE, then
the error string will contain the error message.
=======================================================
*/
int get_remote_error_status(void)
{
	return gSocket.socketError;
}

/*
=======================================================
Get the socket error message.
=======================================================
*/
std::string get_remote_error(void)
{
	return gSocket.socketErrorMsg;
}

/*
=======================================================
Get the socket connection status.  This will return
TRUE if a client is connected.
=======================================================
*/
int get_remote_connect_status(void)
{
	return gSocket.socketOpened;
}

/*
=======================================================
Get the telnet enable status.
=======================================================
*/
int get_remote_connect_telnet(void)
{
	return gSocket.telnet;
}

/*
=======================================================
Get the target listener port.  This may be different
than the listen port if the port cannot be opened.
=======================================================
*/
int get_remote_port(void)
{
	return gSocket.socketPort;
}

/*
=======================================================
Gets the user preference for the socket enabled setting
=======================================================
*/
int get_remote_enabled(void)
{
	return gSocket.enabled;
}

/*
=======================================================
Gets the user preference for the socket enabled setting
=======================================================
*/
void enable_remote(int enabled)
{
	gSocket.enabled = enabled;
}

/*
=======================================================
Loads the user preferences for the socket interface.
=======================================================
*/
void load_remote_preferences()
{
	int		enabled, telnet;

	virtualt_prefs.get("SocketPort", gSocket.socketPort, get_remote_port());
	virtualt_prefs.get("SocketTelnetMode", telnet, get_remote_telnet());
	int default_enabled = gSocket.cmdLinePort != 0;
	virtualt_prefs.get("SocketEnabled", enabled, default_enabled);

	gSocket.enabled = enabled;
	gSocket.telnet = telnet;
}
