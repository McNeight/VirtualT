/* m100emu.h */

/* $Id: m100emu.h,v 1.9 2008/11/04 07:31:22 kpettit1 Exp $ */

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


#ifndef _M100EMU_H_
#define _M100EMU_H_

#include "gen_defs.h"
#include "roms.h"


#ifdef __cplusplus
extern "C" {
#endif

extern char  op[26];
extern int trace;
extern int fullspeed;
extern volatile int gExitApp;
extern volatile int gExitLoop;
extern float cpu_speed;
extern uchar *gMemory[64];
extern RomDescription_t	 *gStdRomDesc;
extern int   gModel;
extern char gsOptRomFile[256];
double hirestimer(void);
typedef void (*mem_monitor_cb)(void);
typedef void (*debug_monitor_callback)(int reason);
void	mem_set_monitor_callback(mem_monitor_cb cb);
void	mem_clear_monitor_callback(mem_monitor_cb cb);
int		debug_set_monitor_callback(debug_monitor_callback pCallback);
void	debug_clear_monitor_callback(debug_monitor_callback pCallback);
void	debug_step(void);
void	debug_halt(void);
void	debug_run(void);
void	remote_switch_model(int model);
extern	char	gDebugActive;
extern	char	gIntActive;
extern	char	gDebugInts;
extern	char	gStopped;
extern	char	gSingleStep;
extern	int		gDebugMonitorFreq;
extern	int		gSocketPort;
extern	int		gTelnet;
extern	int		gNoGUI;

#define	DEBUG_PC_CHANGED	1
#define	DEBUG_CPU_HALTED	2
#define	DEBUG_CPU_RESUME	3
#define	DEBUG_CPU_STEP		4 
#define	DEBUG_INTERRUPT		5

#define	SPEED_REAL			0
#define	SPEED_FRIENDLY1		1
#define	SPEED_FRIENDLY2		2
#define	SPEED_FULL			3
	
int		check_model_support(int model);
void	get_emulation_path(char* emu, int model);
void	get_model_string(char* str, int model);
int		get_model_from_string(char* str);
void	get_rom_path(char* file, int model);
void	init_cpu(void);
//void	cpu_delay(int cy);
void	resetcpu(void);
void	cb_int65(void);

#ifdef __cplusplus
}
#endif


#endif
