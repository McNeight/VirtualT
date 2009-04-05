/* cpuregs.cpp */

/* $Id: cpuregs.cpp,v 1.6 2008/11/04 07:31:22 kpettit1 Exp $ */

/*
* Copyright 2006 Ken Pettit
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
#include <FL/fl_draw.H>
#include <FL/Fl_Window.H>
#include <FL/Fl_Menu_Item.H>
#include <FL/Fl_Menu_Bar.H>

#if defined(WIN32)
#include <windows.h>
#endif
#include <stdio.h>
#include <string.h>
#include <time.h>

#include "cpuregs.h"
#include "m100emu.h"
#include "disassemble.h"
#include "periph.h"
#include "memedit.h"
#include "cpu.h"
#include "memory.h"
#include "fileview.h"

#define		MENU_HEIGHT	32

void cb_Ide(Fl_Widget* w, void*) ;

VTDis	cpu_dis;
/*
============================================================================
Global variables
============================================================================
*/
cpuregs_ctrl_t	cpuregs_ctrl;
Fl_Window		*gcpuw = NULL;
int				gStopCountdown = 0;
int				gSaveFreq = 0;
int				gDisableRealtimeTrace = 0;
int				gDebugCount = 0;;
int				gDebugMonitorFreq = 8192;

// Menu items for the disassembler
Fl_Menu_Item gCpuRegs_menuitems[] = {
  { "&Tools", 0, 0, 0, FL_SUBMENU },
	{ "Assembler / IDE",       0, cb_Ide },
	{ "Disassembler",          0, disassembler_cb },
	{ "Memory Editor",         0, cb_MemoryEditor },
	{ "Peripheral Devices",    0, cb_PeripheralDevices },
//	{ "Simulation Log Viewer", 0, 0 },
	{ "Model T File Viewer",   0, cb_FileView },
	{ 0 },
  { 0 }
};

void debug_cpuregs_cb (int);

/*
============================================================================
Callback routine for the CPU Regs window
============================================================================
*/
void cb_cpuregswin (Fl_Widget* w, void*)
{
	// Hide the window
	gcpuw->hide();

	// Disable debug processes
	gDebugActive--;

	if (gDebugActive == 0)
	{
		gStopped = 0;
		gSingleStep = 0;
	}

	// Remove ourselves as a debug monitor
	debug_clear_monitor_callback(debug_cpuregs_cb);

	// Delete the window and set to NULL
	delete gcpuw;
	gcpuw = NULL;
}


/*
============================================================================
str_to_i:	This routine converts a hex or decimal string to an integer.
			Hex values are indentified by a leading "0x".
============================================================================
*/
int str_to_i(const char *pStr)
{
	int		x, len, hex = FALSE;
	int		val, digit;

	// Test for Hex number
	if (strncmp("0x", pStr, 2) == 0)
	{
		hex = TRUE;
		pStr += 2;
	}
	else
	{
		// Scan scring for any hex digits
		len = strlen(pStr);
		if ((pStr[len-1] == 'h') || (pStr[len-1] == 'H'))
			hex = TRUE;
		else
			for (x = 0; x < len; x++)
			{
				if (((pStr[x] >= 'A') && (pStr[x] <= 'F')) ||
					((pStr[x] >= 'a') && (pStr[x] <= 'f')))
				{
					hex = TRUE;
					break;
				}
			}
	}

	// If it's a hex string, use hex conversion
	if (hex)
	{
		x = 0; val = 0;
		while ((pStr[x] != '\0') && (pStr[x] != 'h') && (pStr[x] != 'H'))
		{
			digit = pStr[x] > '9' ? (pStr[x] | 0x60) - 'a' + 10 : pStr[x] - '0';
			val = val * 16 + digit;
			x++;
		}
	}
	else
	{
		val = atoi(pStr);
	}

	// Return value to caller
	return val;
}

/*
============================================================================
get_reg_edits:	This routine reads all register input fields and updates the
				CPU registers accordingly.
============================================================================
*/
void get_reg_edits(void)
{
	const char*		pStr;
	int				val, flags;
	int				x;

	// Get value of PC
	pStr = cpuregs_ctrl.pRegPC->value();
	if (pStr[0] != 0)
	{
		val = str_to_i(pStr);
		PCL = val & 0xFF; PCH = val >> 8;
	}

	// Get value of SP
	pStr = cpuregs_ctrl.pRegSP->value();
	if (pStr[0] != 0)
	{
		val = str_to_i(pStr);
		SPL = val & 0xFF; SPH = val >> 8;
	}

	// Get value of A
	pStr = cpuregs_ctrl.pRegA->value();
	if (pStr[0] != 0)
	{
		val = str_to_i(pStr);
		A = val & 0xFF;
	}

	// Get value of M
	pStr = cpuregs_ctrl.pRegM->value();
	if (pStr[0] != 0)
	{
		val = str_to_i(pStr);
		set_memory8(HL, val & 0xFF);
	}

	// Get value of BC
	pStr = cpuregs_ctrl.pRegBC->value();
	if (pStr[0] != 0)
	{
		val = str_to_i(pStr);
		if (val != BC)
		{
			// Update the values of B and C from the BC edit field
			B = val >> 8;
			C = val & 0xFF;
		}
		else
		{
			// BC didn't change - test B and C individually
			pStr = cpuregs_ctrl.pRegB->value();
			if (pStr[0] != 0)
			{
				val = str_to_i(pStr);
				B = val;
			}
			// Now get new value for C
			pStr = cpuregs_ctrl.pRegC->value();
			if (pStr[0] != 0)
			{
				val = str_to_i(pStr);
				C = val;
			}
		}
	}

	// Get value of DE
	pStr = cpuregs_ctrl.pRegDE->value();
	if (pStr[0] != 0)
	{
		val = str_to_i(pStr);
		if (val != DE)
		{
			// Update the values of D and E from the DE edit field
			D = val >> 8;
			E = val & 0xFF;
		}
		else
		{
			// DE didn't change - test D and E individually
			pStr = cpuregs_ctrl.pRegD->value();
			if (pStr[0] != 0)
			{
				val = str_to_i(pStr);
				D = val;
			}
			// Now get new value for E
			pStr = cpuregs_ctrl.pRegE->value();
			if (pStr[0] != 0)
			{
				val = str_to_i(pStr);
				E = val;
			}
		}
	}

	// Get value of HL
	pStr = cpuregs_ctrl.pRegHL->value();
	if (pStr[0] != 0)
	{
		val = str_to_i(pStr);
		if (val != HL)
		{
			// Update the values of H and L from the HL edit field
			H = val >> 8;
			L = val & 0xFF;
		}
		else
		{
			// HL didn't change - test H and L individually
			pStr = cpuregs_ctrl.pRegH->value();
			if (pStr[0] != 0)
			{
				val = str_to_i(pStr);
				H = val;
			}
			// Now get new value for L
			pStr = cpuregs_ctrl.pRegL->value();
			if (pStr[0] != 0)
			{
				val = str_to_i(pStr);
				L = val;
			}
		}
	}

	// Finally get updates to the flags
	flags = 0;
	if (cpuregs_ctrl.pSFlag->value())
		flags |= 0x80;
	if (cpuregs_ctrl.pZFlag->value())
		flags |= 0x40;
	if (cpuregs_ctrl.pTSFlag->value())
		flags |= 0x20;
	if (cpuregs_ctrl.pACFlag->value())
		flags |= 0x10;
	if (cpuregs_ctrl.pPFlag->value())
		flags |= 0x04;
	if (cpuregs_ctrl.pOVFlag->value())
		flags |= 0x02;
	if (cpuregs_ctrl.pCFlag->value())
		flags |= 0x01;

	F = flags;

	// Get value of Breakpoint 1
	pStr = cpuregs_ctrl.pBreak1->value();
	if (pStr[0] != 0)
	{
		val = str_to_i(pStr);
		cpuregs_ctrl.breakAddr[0] = val;
	}
	else
		cpuregs_ctrl.breakAddr[0] = -1;

	// Get value of Breakpoint 2
	pStr = cpuregs_ctrl.pBreak2->value();
	if (pStr[0] != 0)
	{
		val = str_to_i(pStr);
		cpuregs_ctrl.breakAddr[1] = val;
	}
	else
		cpuregs_ctrl.breakAddr[1] = -1;

	// Get value of Breakpoint 3
	pStr = cpuregs_ctrl.pBreak3->value();
	if (pStr[0] != 0)
	{
		val = str_to_i(pStr);
		cpuregs_ctrl.breakAddr[2] = val;
	}
	else
		cpuregs_ctrl.breakAddr[2] = -1;

	// Get value of Breakpoint 4
	pStr = cpuregs_ctrl.pBreak4->value();
	if (pStr[0] != 0)
	{
		val = str_to_i(pStr);
		cpuregs_ctrl.breakAddr[3] = val;
	}
	else
		cpuregs_ctrl.breakAddr[3] = -1;

	// Check if any breakpoints set
	for (x = 0; x < 4; x++)
		if (cpuregs_ctrl.breakAddr[x] != -1)
			break;

	// If no breakpoints set, restore old monitor frequency
	if (x == 4)
	{
		if (cpuregs_ctrl.breakMonitorFreq != 0)
			gSaveFreq = cpuregs_ctrl.breakMonitorFreq;
	}
	else
	{
		if (cpuregs_ctrl.breakMonitorFreq == 0)
			cpuregs_ctrl.breakMonitorFreq = gSaveFreq;
		gSaveFreq = 1;
		gDebugMonitorFreq = 1;
	}

	cpuregs_ctrl.breakEnable[0] = !cpuregs_ctrl.pBreakDisable1->value();
	cpuregs_ctrl.breakEnable[1] = !cpuregs_ctrl.pBreakDisable2->value();
	cpuregs_ctrl.breakEnable[2] = !cpuregs_ctrl.pBreakDisable3->value();
	cpuregs_ctrl.breakEnable[3] = !cpuregs_ctrl.pBreakDisable4->value();
}

/*
============================================================================
Callback routine to redraw the trace and stack areas
============================================================================
*/
void cb_redraw_trace(Fl_Widget* w, void*)
{
	int		x, draw;
	char	str[10];

	gcpuw->make_current();

	// Clear rectangle
	fl_color(FL_GRAY);
	fl_rectf(25, 205+MENU_HEIGHT, 510, 115);
	fl_rectf(555, 205+MENU_HEIGHT, 54, 115);

	// Select 11 point Courier font
	fl_font(FL_COURIER,11);
	fl_color(FL_BLACK);

	// Determine first trace to draw
	draw = cpuregs_ctrl.iInstTraceHead;
	for (x = 0; x < 8; x++)
	{
		// Draw the Stack
		sprintf(str, "0x%02X", get_memory8(SP+x));
		fl_draw(str, 560, 212+MENU_HEIGHT+x*15);

		// Now draw the Instruction Trace
		if (x == 7)
			fl_font(FL_COURIER_BOLD, 11);
		fl_draw(cpuregs_ctrl.sInstTrace[draw], 25, 212+MENU_HEIGHT+x*15);
		if (++draw >= 8)
			draw = 0;

	}
}

/*
============================================================================
Callback routine for receiving serial port status updates
============================================================================
*/
extern "C"
{
void debug_monitor_cb(int fMonType, unsigned char data)
{
}
}

unsigned char get_m()
{
	if (gReMem)
		return (gMemory[gIndex[HL]][HL & 0x3FF]);
	else
		return gBaseMemory[HL];
}

/*
============================================================================
activate_controls:	This routine activates all the CPU controls except the
STOP button, which it deactivates.
============================================================================
*/
void activate_controls()
{
	// Change activation state of processor controls
	cpuregs_ctrl.pStop->deactivate();
	cpuregs_ctrl.pStep->activate();
	cpuregs_ctrl.pStepOver->activate();
	cpuregs_ctrl.pRun->activate();
	cpuregs_ctrl.pRedraw->activate();

	// Activate the edit fields to allow register updates
	cpuregs_ctrl.pRegA->activate();
	cpuregs_ctrl.pRegB->activate();
	cpuregs_ctrl.pRegC->activate();
	cpuregs_ctrl.pRegD->activate();
	cpuregs_ctrl.pRegE->activate();
	cpuregs_ctrl.pRegH->activate();
	cpuregs_ctrl.pRegL->activate();
	cpuregs_ctrl.pRegPC->activate();
	cpuregs_ctrl.pRegSP->activate();
	cpuregs_ctrl.pRegBC->activate();
	cpuregs_ctrl.pRegDE->activate();
	cpuregs_ctrl.pRegHL->activate();
	cpuregs_ctrl.pRegM->activate();

	// Activate Flags
	cpuregs_ctrl.pSFlag->activate();
	cpuregs_ctrl.pZFlag->activate();
	cpuregs_ctrl.pCFlag->activate();
	cpuregs_ctrl.pTSFlag->activate();
	cpuregs_ctrl.pACFlag->activate();
	cpuregs_ctrl.pPFlag->activate();
	cpuregs_ctrl.pOVFlag->activate();
	cpuregs_ctrl.pXFlag->activate();

	// Activate Breakpoints
	cpuregs_ctrl.pBreak1->activate();
	cpuregs_ctrl.pBreak2->activate();
	cpuregs_ctrl.pBreak3->activate();
	cpuregs_ctrl.pBreak4->activate();
	cpuregs_ctrl.pBreakDisable1->activate();
	cpuregs_ctrl.pBreakDisable2->activate();
	cpuregs_ctrl.pBreakDisable3->activate();
	cpuregs_ctrl.pBreakDisable4->activate();
}

void clear_trace(void)
{
	int 	x;

	for (x = 0; x < 8; x++)
		cpuregs_ctrl.sInstTrace[x][0] = 0;
}

/*
============================================================================
debug_cpuregs_cb:	This routine is the callback for the CPURegs Monitor
					window.  This routine handles updating of Register
					values while the CPU is running.
============================================================================
*/
void debug_cpuregs_cb (int reason)
{
	char	str[100];
	char	flags[10];
	int		x, len;

	// Check for breakpoint
	if (!gStopped)
	{
		for (x = 0; x < 5; x++)
		{
			if ((PC == cpuregs_ctrl.breakAddr[x]) && (cpuregs_ctrl.breakEnable[x]))
			{
				activate_controls();
				gStopped = 1;

				// Clear 'step over' breakpoint
				if (x == 4)
					cpuregs_ctrl.breakAddr[x] = -1;
				else
					clear_trace();
			}
		}
	}

	if (!gStopped)
	{
		if (cpuregs_ctrl.breakAddr[4] != -1)
			return;
		if (++gDebugCount < gDebugMonitorFreq)
			return;
	}

	gDebugCount = 0;

	// Update PC edit box
	sprintf(str, cpuregs_ctrl.sPCfmt, PC);
	cpuregs_ctrl.pRegPC->value(str);

	// Update SP edit box
	sprintf(str, cpuregs_ctrl.sSPfmt, SP);
	cpuregs_ctrl.pRegSP->value(str);

	// Update A edit box
	sprintf(str, cpuregs_ctrl.sAfmt, A);
	cpuregs_ctrl.pRegA->value(str);

	// Update B edit box
	sprintf(str, cpuregs_ctrl.sBfmt, B);
	cpuregs_ctrl.pRegB->value(str);

	// Update C edit box
	sprintf(str, cpuregs_ctrl.sCfmt, C);
	cpuregs_ctrl.pRegC->value(str);

	// Update D edit box
	sprintf(str, cpuregs_ctrl.sDfmt, D);
	cpuregs_ctrl.pRegD->value(str);

	// Update E edit box
	sprintf(str, cpuregs_ctrl.sEfmt, E);
	cpuregs_ctrl.pRegE->value(str);

	// Update H edit box
	sprintf(str, cpuregs_ctrl.sHfmt, H);
	cpuregs_ctrl.pRegH->value(str);

	// Update L edit box
	sprintf(str, cpuregs_ctrl.sLfmt, L);
	cpuregs_ctrl.pRegL->value(str);

	// Update BC edit box
	sprintf(str, cpuregs_ctrl.sBCfmt, BC);
	cpuregs_ctrl.pRegBC->value(str);

	// Update DE edit box
	sprintf(str, cpuregs_ctrl.sDEfmt, DE);
	cpuregs_ctrl.pRegDE->value(str);

	// Update HL edit box
	sprintf(str, cpuregs_ctrl.sHLfmt, HL);
	cpuregs_ctrl.pRegHL->value(str);

	// Update M edit box
	sprintf(str, cpuregs_ctrl.sMfmt, get_m());
	cpuregs_ctrl.pRegM->value(str);

	// Update flags
	cpuregs_ctrl.pSFlag->value(SF);
	cpuregs_ctrl.pZFlag->value(ZF);
	cpuregs_ctrl.pTSFlag->value(TS);
	cpuregs_ctrl.pACFlag->value(AC);
	cpuregs_ctrl.pPFlag->value(PF);
	cpuregs_ctrl.pOVFlag->value(OV);
	cpuregs_ctrl.pXFlag->value(XF);
	cpuregs_ctrl.pCFlag->value(CF);

	if (!gDisableRealtimeTrace || (gStopCountdown != 0) || gStopped)
	{
		// Disassemble 1 instruction
		cpu_dis.DisassembleLine(PC, cpuregs_ctrl.sInstTrace[cpuregs_ctrl.iInstTraceHead]);

		// Append spaces after opcode
		len = 20 - strlen(cpuregs_ctrl.sInstTrace[cpuregs_ctrl.iInstTraceHead]);
		for (x = 0; x < len; x++)
			strcat(cpuregs_ctrl.sInstTrace[cpuregs_ctrl.iInstTraceHead], " ");

		// Format flags string
		sprintf(flags, "%c%c%c%c%c%c%c%c", SF?'S':' ', ZF?'Z':' ', TS?'T':' ',AC?'A':' ',
			PF?'P':' ',OV?'O':' ',XF?'X':' ', CF?'C':' ');

		// Append regs after opcode
		sprintf(str, "A:%02X %s B:%02X C:%02X D:%02X E:%02X H:%02X L:%02X SP:%02X",
			A, flags, B, C, D, E, H, L, SP);
		strcat(cpuregs_ctrl.sInstTrace[cpuregs_ctrl.iInstTraceHead++], str);

		if (cpuregs_ctrl.iInstTraceHead >= 8)
			cpuregs_ctrl.iInstTraceHead= 0;

		// Update Trace data on window
		cb_redraw_trace(NULL, NULL);
	}

	// Check if processor stop requested
	if (gStopCountdown > 0)
	{
		// We countdown to a stop so the last 8 trace steps make sense
		if (gStopCountdown == 1)
		{
			if (!gIntActive)
			{
				gStopped = 1;
				gDebugMonitorFreq = gSaveFreq;
				gStopCountdown = 0;
			}
		}
		else
			--gStopCountdown;
	}

}

/*
============================================================================
Routine to deactivate the debugging controls
============================================================================
*/
void deactivate_controls(void)
{
	// Change activation state of processor controls
	cpuregs_ctrl.pStop->activate();
	cpuregs_ctrl.pStep->deactivate();
	cpuregs_ctrl.pStepOver->deactivate();
	cpuregs_ctrl.pRun->deactivate();
	cpuregs_ctrl.pRedraw->deactivate();

	// Deactivate the edit fields to allow register updates
	cpuregs_ctrl.pRegA->deactivate();
	cpuregs_ctrl.pRegB->deactivate();
	cpuregs_ctrl.pRegC->deactivate();
	cpuregs_ctrl.pRegD->deactivate();
	cpuregs_ctrl.pRegE->deactivate();
	cpuregs_ctrl.pRegH->deactivate();
	cpuregs_ctrl.pRegL->deactivate();
	cpuregs_ctrl.pRegPC->deactivate();
	cpuregs_ctrl.pRegSP->deactivate();
	cpuregs_ctrl.pRegBC->deactivate();
	cpuregs_ctrl.pRegDE->deactivate();
	cpuregs_ctrl.pRegHL->deactivate();
	cpuregs_ctrl.pRegM->deactivate();

	// Deactivate Flags
	cpuregs_ctrl.pSFlag->deactivate();
	cpuregs_ctrl.pZFlag->deactivate();
	cpuregs_ctrl.pCFlag->deactivate();
	cpuregs_ctrl.pTSFlag->deactivate();
	cpuregs_ctrl.pACFlag->deactivate();
	cpuregs_ctrl.pPFlag->deactivate();
	cpuregs_ctrl.pOVFlag->deactivate();
	cpuregs_ctrl.pXFlag->deactivate();

	// Deactivate breakpoints
	cpuregs_ctrl.pBreak1->deactivate();
	cpuregs_ctrl.pBreak2->deactivate();
	cpuregs_ctrl.pBreak3->deactivate();
	cpuregs_ctrl.pBreak4->deactivate();
	cpuregs_ctrl.pBreakDisable1->deactivate();
	cpuregs_ctrl.pBreakDisable2->deactivate();
	cpuregs_ctrl.pBreakDisable3->deactivate();
	cpuregs_ctrl.pBreakDisable4->deactivate();
}

/*
============================================================================
Routine to handle 'Step Over' button
============================================================================
*/
void cb_debug_step_over(Fl_Widget* w, void*)
{
	int		inst;

	// Get Register updates
	get_reg_edits();

	// Determine the 'step over' breakpoint location
	inst = get_memory8(PC);
	if (((inst & 0xC7) == 0xC4) || (inst == 0xCD)) 
	{
		cpuregs_ctrl.breakAddr[4] = PC+3;
	}
	else if ((inst & 0xC7) == 0xC7)
	{
		if ((inst == 0xCF) || (inst == 0xFF))
			cpuregs_ctrl.breakAddr[4] = PC+2;
		else
			cpuregs_ctrl.breakAddr[4] = PC+1;
	}
	else
	{
		// Single step the processor
		gSingleStep = 1;
		return;
	}

	// Deactivate the controls
//	deactivate_controls();

	// Start the processor
	gStopped = 0;
}

/*
============================================================================
Routine to handle Stop button
============================================================================
*/
void cb_debug_stop(Fl_Widget* w, void*)
{
	activate_controls();

	// Stop the processor
	gStopCountdown = 8;
	gSaveFreq = gDebugMonitorFreq;
	gDebugMonitorFreq = 1;
}

/*
============================================================================
Routine to handle Step button
============================================================================
*/
void cb_debug_step(Fl_Widget* w, void*)
{
	// Get Register updates
	get_reg_edits();

	// Single step the processor
	gSingleStep = 1;
}

/*
============================================================================
Routine to handle Run button
============================================================================
*/
void cb_debug_run(Fl_Widget* w, void*)
{
	// Deactivate the debug controls
	deactivate_controls();

	// Get Register updates
	get_reg_edits();

	// Start the processor
	gStopped = 0;
}

/*
============================================================================
Routines to handle remote debugging commands 
============================================================================
*/
int remote_cpureg_stop(void)
{
	if (gcpuw != NULL)
	{
		cb_debug_stop(NULL, NULL);
		return 1;
	}
	return 0;
}
int remote_cpureg_run(void)
{
	if (gcpuw != NULL)
	{
		cb_debug_run(NULL, NULL);
		return 1;
	}
	return 0;
}
int remote_cpureg_step(void)
{
	if (gcpuw != NULL)
	{
		cb_debug_step(NULL, NULL);
		return 1;
	}
	return 0;
}

/*
============================================================================
Routine to handle Change to PC edit field
============================================================================
*/
void cb_reg_pc_changed(Fl_Widget* w, void*)
{
	int				new_pc;
	const char		*pStr;
	int				trace_tail;
	char			str[120], flags[20];
	int				len, x;

	pStr = cpuregs_ctrl.pRegPC->value();
	new_pc = str_to_i(pStr);
	trace_tail = cpuregs_ctrl.iInstTraceHead - 1;
	if (trace_tail < 0)
		trace_tail = 7;

	// Disassemble 1 instruction
	cpu_dis.DisassembleLine(new_pc, cpuregs_ctrl.sInstTrace[trace_tail]);

	// Append spaces after opcode
	len = 20 - strlen(cpuregs_ctrl.sInstTrace[trace_tail]);
	for (x = 0; x < len; x++)
		strcat(cpuregs_ctrl.sInstTrace[trace_tail], " ");

	// Format flags string
	sprintf(flags, "%c%c%c%c%c%c%c%c", SF?'S':' ', ZF?'Z':' ', TS?'T':' ',AC?'A':' ',
		PF?'P':' ',OV?'O':' ',XF?'X':' ', CF?'C':' ');

	// Append regs after opcode
	sprintf(str, "A:%02X %s B:%02X C:%02X D:%02X E:%02X H:%02X L:%02X SP:%02X",
		A, flags, B, C, D, E, H, L, SP);
	strcat(cpuregs_ctrl.sInstTrace[trace_tail], str);

	// Update Trace data on window
	cb_redraw_trace(NULL, NULL);
}

/*
============================================================================
Routine to handle Change to BC edit field
============================================================================
*/
void cb_reg_bc_changed(Fl_Widget* w, void*)
{
	int				new_bc;
	const char		*pStr;
	char			str[20];

	pStr = cpuregs_ctrl.pRegBC->value();
	new_bc = str_to_i(pStr);

	B = (new_bc >> 8) & 0xFF;
	C = new_bc & 0xFF;

	// Update B edit box
	sprintf(str, cpuregs_ctrl.sBfmt, B);
	cpuregs_ctrl.pRegB->value(str);

	// Update C edit box
	sprintf(str, cpuregs_ctrl.sCfmt, C);
	cpuregs_ctrl.pRegC->value(str);
}

/*
============================================================================
Routine to handle Change to B edit field
============================================================================
*/
void cb_reg_b_changed(Fl_Widget* w, void*)
{
	const char		*pStr;
	char			str[20];

	pStr = cpuregs_ctrl.pRegB->value();
	B = str_to_i(pStr);

	// Update B edit box
	sprintf(str, cpuregs_ctrl.sBfmt, B);
	cpuregs_ctrl.pRegB->value(str);

	// Update BC edit box
	sprintf(str, cpuregs_ctrl.sBCfmt, BC);
	cpuregs_ctrl.pRegBC->value(str);
}

/*
============================================================================
Routine to handle Change to C edit field
============================================================================
*/
void cb_reg_c_changed(Fl_Widget* w, void*)
{
	const char		*pStr;
	char			str[20];

	pStr = cpuregs_ctrl.pRegC->value();
	C = str_to_i(pStr);

	// Update C edit box
	sprintf(str, cpuregs_ctrl.sCfmt, C);
	cpuregs_ctrl.pRegC->value(str);

	// Update BC edit box
	sprintf(str, cpuregs_ctrl.sBCfmt, BC);
	cpuregs_ctrl.pRegBC->value(str);
}

/*
============================================================================
Routine to handle Change to DE edit field
============================================================================
*/
void cb_reg_de_changed(Fl_Widget* w, void*)
{
	int				new_de;
	const char		*pStr;
	char			str[20];

	pStr = cpuregs_ctrl.pRegDE->value();
	new_de = str_to_i(pStr);

	D = (new_de >> 8) & 0xFF;
	E = new_de & 0xFF;

	// Update D edit box
	sprintf(str, cpuregs_ctrl.sDfmt, D);
	cpuregs_ctrl.pRegD->value(str);

	// Update E edit box
	sprintf(str, cpuregs_ctrl.sEfmt, E);
	cpuregs_ctrl.pRegE->value(str);
}

/*
============================================================================
Routine to handle Change to D edit field
============================================================================
*/
void cb_reg_d_changed(Fl_Widget* w, void*)
{
	const char		*pStr;
	char			str[20];

	pStr = cpuregs_ctrl.pRegD->value();
	D = str_to_i(pStr);

	// Update B edit box
	sprintf(str, cpuregs_ctrl.sDfmt, D);
	cpuregs_ctrl.pRegD->value(str);

	// Update BC edit box
	sprintf(str, cpuregs_ctrl.sDEfmt, DE);
	cpuregs_ctrl.pRegDE->value(str);
}

/*
============================================================================
Routine to handle Change to E edit field
============================================================================
*/
void cb_reg_e_changed(Fl_Widget* w, void*)
{
	const char		*pStr;
	char			str[20];

	pStr = cpuregs_ctrl.pRegE->value();
	E = str_to_i(pStr);

	// Update C edit box
	sprintf(str, cpuregs_ctrl.sEfmt, E);
	cpuregs_ctrl.pRegE->value(str);

	// Update DE edit box
	sprintf(str, cpuregs_ctrl.sDEfmt, DE);
	cpuregs_ctrl.pRegDE->value(str);
}

/*
============================================================================
Routine to handle Change to HL edit field
============================================================================
*/
void cb_reg_hl_changed(Fl_Widget* w, void*)
{
	int				new_hl;
	const char		*pStr;
	char			str[20];

	pStr = cpuregs_ctrl.pRegHL->value();
	new_hl = str_to_i(pStr);

	H = (new_hl >> 8) & 0xFF;
	L = new_hl & 0xFF;

	// Update H edit box
	sprintf(str, cpuregs_ctrl.sHfmt, H);
	cpuregs_ctrl.pRegH->value(str);

	// Update L edit box
	sprintf(str, cpuregs_ctrl.sLfmt, L);
	cpuregs_ctrl.pRegL->value(str);

	// Update M edit box
	sprintf(str, cpuregs_ctrl.sMfmt, get_m());
	cpuregs_ctrl.pRegM->value(str);

}

/*
============================================================================
Routine to handle Change to H edit field
============================================================================
*/
void cb_reg_h_changed(Fl_Widget* w, void*)
{
	const char		*pStr;
	char			str[20];

	pStr = cpuregs_ctrl.pRegH->value();
	H = str_to_i(pStr);

	// Update H edit box
	sprintf(str, cpuregs_ctrl.sHfmt, H);
	cpuregs_ctrl.pRegH->value(str);

	// Update HL edit box
	sprintf(str, cpuregs_ctrl.sHLfmt, HL);
	cpuregs_ctrl.pRegHL->value(str);

	// Update M edit box
	sprintf(str, cpuregs_ctrl.sMfmt, get_m());
	cpuregs_ctrl.pRegM->value(str);

}

/*
============================================================================
Routine to handle Change to L edit field
============================================================================
*/
void cb_reg_l_changed(Fl_Widget* w, void*)
{
	const char		*pStr;
	char			str[20];

	pStr = cpuregs_ctrl.pRegL->value();
	L = str_to_i(pStr);

	// Update H edit box
	sprintf(str, cpuregs_ctrl.sLfmt, L);
	cpuregs_ctrl.pRegL->value(str);

	// Update HL edit box
	sprintf(str, cpuregs_ctrl.sHLfmt, HL);
	cpuregs_ctrl.pRegHL->value(str);

	// Update M edit box
	sprintf(str, cpuregs_ctrl.sMfmt, get_m());
	cpuregs_ctrl.pRegM->value(str);

}

/*
============================================================================
Routine to handle Change to SP edit field
============================================================================
*/
void cb_reg_sp_changed(Fl_Widget* w, void*)
{
	const char		*pStr;
	int				val;

	// Get value of SP
	pStr = cpuregs_ctrl.pRegSP->value();
	if (pStr[0] != 0)
	{
		val = str_to_i(pStr);
		SPL = val & 0xFF; SPH = val >> 8;
	}

	// Update Trace data on window
	cb_redraw_trace(NULL, NULL);
}

/*
============================================================================
Routine to handle disabling realtime trace
============================================================================
*/
void cb_disable_realtime_trace(Fl_Widget* w, void*)
{
	gDisableRealtimeTrace = cpuregs_ctrl.pDisableTrace->value();

	if (gDisableRealtimeTrace)
	{
		gcpuw->make_current();

		// Clear rectangle
		fl_color(FL_GRAY);
		fl_rectf(25, 205+MENU_HEIGHT, 510, 115);
		fl_rectf(555, 205+MENU_HEIGHT, 54, 115);
	}
}

/*
============================================================================
Routine to handle Reg A Hex display mode
============================================================================
*/
void cb_reg_a_hex(Fl_Widget* w, void*)
{
	char str[8];

	strcpy(cpuregs_ctrl.sAfmt,  "0x%02X");
	sprintf(str, cpuregs_ctrl.sAfmt, A);
	cpuregs_ctrl.pRegA->value(str);

	cpuregs_ctrl.pAllHex->value(0);
	cpuregs_ctrl.pAllDec->value(0);
}

/*
============================================================================
Routine to handle Reg A Dec display mode
============================================================================
*/
void cb_reg_a_dec(Fl_Widget* w, void*)
{
	char str[8];

	strcpy(cpuregs_ctrl.sAfmt,  "%d");
	sprintf(str, cpuregs_ctrl.sAfmt, A);
	cpuregs_ctrl.pRegA->value(str);

	cpuregs_ctrl.pAllHex->value(0);
	cpuregs_ctrl.pAllDec->value(0);
}

/*
============================================================================
Routine to handle Reg B Hex display mode
============================================================================
*/
void cb_reg_b_hex(Fl_Widget* w, void*)
{
	char str[8];

	strcpy(cpuregs_ctrl.sBfmt,  "0x%02X");
	sprintf(str, cpuregs_ctrl.sBfmt, B);
	cpuregs_ctrl.pRegB->value(str);

	cpuregs_ctrl.pAllHex->value(0);
	cpuregs_ctrl.pAllDec->value(0);
}

/*
==========================================================================
Routine to handle Reg B Dec display mode
============================================================================
*/
void cb_reg_b_dec(Fl_Widget* w, void*)
{
	char str[8];

	strcpy(cpuregs_ctrl.sBfmt,  "%d");
	sprintf(str, cpuregs_ctrl.sBfmt, B);
	cpuregs_ctrl.pRegB->value(str);

	cpuregs_ctrl.pAllHex->value(0);
	cpuregs_ctrl.pAllDec->value(0);
}

/*
============================================================================
Routine to handle Reg C Hex display mode
============================================================================
*/
void cb_reg_c_hex(Fl_Widget* w, void*)
{
	char str[8];

	strcpy(cpuregs_ctrl.sCfmt,  "0x%02X");
	sprintf(str, cpuregs_ctrl.sCfmt, C);
	cpuregs_ctrl.pRegC->value(str);

	cpuregs_ctrl.pAllHex->value(0);
	cpuregs_ctrl.pAllDec->value(0);
}

/*
============================================================================
Routine to handle Reg C Dec display mode
============================================================================
*/
void cb_reg_c_dec(Fl_Widget* w, void*)
{
	char str[8];

	strcpy(cpuregs_ctrl.sCfmt,  "%d");
	sprintf(str, cpuregs_ctrl.sCfmt, C);
	cpuregs_ctrl.pRegC->value(str);

	cpuregs_ctrl.pAllHex->value(0);
	cpuregs_ctrl.pAllDec->value(0);
}

/*
============================================================================
Routine to handle Reg D Hex display mode
============================================================================
*/
void cb_reg_d_hex(Fl_Widget* w, void*)
{
	char str[8];

	strcpy(cpuregs_ctrl.sDfmt,  "0x%02X");
	sprintf(str, cpuregs_ctrl.sDfmt, D);
	cpuregs_ctrl.pRegD->value(str);

	cpuregs_ctrl.pAllHex->value(0);
	cpuregs_ctrl.pAllDec->value(0);
}

/*
============================================================================
Routine to handle Reg D Dec display mode
============================================================================
*/
void cb_reg_d_dec(Fl_Widget* w, void*)
{
	char str[8];

	strcpy(cpuregs_ctrl.sDfmt,  "%d");
	sprintf(str, cpuregs_ctrl.sDfmt, D);
	cpuregs_ctrl.pRegD->value(str);

	cpuregs_ctrl.pAllHex->value(0);
	cpuregs_ctrl.pAllDec->value(0);
}

/*
============================================================================
Routine to handle Reg E Hex display mode
============================================================================
*/
void cb_reg_e_hex(Fl_Widget* w, void*)
{
	char str[8];

	strcpy(cpuregs_ctrl.sEfmt,  "0x%02X");
	sprintf(str, cpuregs_ctrl.sEfmt, E);
	cpuregs_ctrl.pRegE->value(str);

	cpuregs_ctrl.pAllHex->value(0);
	cpuregs_ctrl.pAllDec->value(0);
}

/*
============================================================================
Routine to handle Reg E Dec display mode
============================================================================
*/
void cb_reg_e_dec(Fl_Widget* w, void*)
{
	char str[8];

	strcpy(cpuregs_ctrl.sEfmt,  "%d");
	sprintf(str, cpuregs_ctrl.sEfmt, E);
	cpuregs_ctrl.pRegE->value(str);

	cpuregs_ctrl.pAllHex->value(0);
	cpuregs_ctrl.pAllDec->value(0);
}

/*
============================================================================
Routine to handle Reg H Hex display mode
============================================================================
*/
void cb_reg_h_hex(Fl_Widget* w, void*)
{
	char str[8];

	strcpy(cpuregs_ctrl.sHfmt,  "0x%02X");
	sprintf(str, cpuregs_ctrl.sHfmt, H);
	cpuregs_ctrl.pRegH->value(str);

	cpuregs_ctrl.pAllHex->value(0);
	cpuregs_ctrl.pAllDec->value(0);
}

/*
============================================================================
Routine to handle Reg H Dec display mode
============================================================================
*/
void cb_reg_h_dec(Fl_Widget* w, void*)
{
	char str[8];

	strcpy(cpuregs_ctrl.sHfmt,  "%d");
	sprintf(str, cpuregs_ctrl.sHfmt, H);
	cpuregs_ctrl.pRegH->value(str);

	cpuregs_ctrl.pAllHex->value(0);
	cpuregs_ctrl.pAllDec->value(0);
}

/*
============================================================================
Routine to handle Reg L Hex display mode
============================================================================
*/
void cb_reg_l_hex(Fl_Widget* w, void*)
{
	char str[8];

	strcpy(cpuregs_ctrl.sLfmt,  "0x%02X");
	sprintf(str, cpuregs_ctrl.sLfmt, L);
	cpuregs_ctrl.pRegL->value(str);

	cpuregs_ctrl.pAllHex->value(0);
	cpuregs_ctrl.pAllDec->value(0);
}

/*
============================================================================
Routine to handle Reg L Dec display mode
============================================================================
*/
void cb_reg_l_dec(Fl_Widget* w, void*)
{
	char str[8];

	strcpy(cpuregs_ctrl.sLfmt,  "%d");
	sprintf(str, cpuregs_ctrl.sLfmt, L);
	cpuregs_ctrl.pRegL->value(str);

	cpuregs_ctrl.pAllHex->value(0);
	cpuregs_ctrl.pAllDec->value(0);
}

/*
============================================================================
Routine to handle Reg PC Hex display mode
============================================================================
*/
void cb_reg_pc_hex(Fl_Widget* w, void*)
{
	char str[8];

	strcpy(cpuregs_ctrl.sPCfmt,  "0x%04X");
	sprintf(str, cpuregs_ctrl.sPCfmt, PC);
	cpuregs_ctrl.pRegPC->value(str);

	cpuregs_ctrl.pAllHex->value(0);
	cpuregs_ctrl.pAllDec->value(0);
}

/*
============================================================================
Routine to handle Reg PC Dec display mode
============================================================================
*/
void cb_reg_pc_dec(Fl_Widget* w, void*)
{
	char str[8];

	strcpy(cpuregs_ctrl.sPCfmt,  "%d");
	sprintf(str, cpuregs_ctrl.sPCfmt, PC);
	cpuregs_ctrl.pRegPC->value(str);

	cpuregs_ctrl.pAllHex->value(0);
	cpuregs_ctrl.pAllDec->value(0);
}

/*
============================================================================
Routine to handle Reg SP Hex display mode
============================================================================
*/
void cb_reg_sp_hex(Fl_Widget* w, void*)
{
	char str[8];

	strcpy(cpuregs_ctrl.sSPfmt,  "0x%04X");
	sprintf(str, cpuregs_ctrl.sSPfmt, SP);
	cpuregs_ctrl.pRegSP->value(str);

	cpuregs_ctrl.pAllHex->value(0);
	cpuregs_ctrl.pAllDec->value(0);
}

/*
============================================================================
Routine to handle Reg SP Dec display mode
============================================================================
*/
void cb_reg_sp_dec(Fl_Widget* w, void*)
{
	char str[8];

	strcpy(cpuregs_ctrl.sSPfmt,  "%d");
	sprintf(str, cpuregs_ctrl.sSPfmt, SP);
	cpuregs_ctrl.pRegSP->value(str);

	cpuregs_ctrl.pAllHex->value(0);
	cpuregs_ctrl.pAllDec->value(0);
}

/*
============================================================================
Routine to handle Reg BC Hex display mode
============================================================================
*/
void cb_reg_bc_hex(Fl_Widget* w, void*)
{
	char str[8];

	strcpy(cpuregs_ctrl.sBCfmt,  "0x%04X");
	sprintf(str, cpuregs_ctrl.sBCfmt, BC);
	cpuregs_ctrl.pRegBC->value(str);

	cpuregs_ctrl.pAllHex->value(0);
	cpuregs_ctrl.pAllDec->value(0);
}

/*
============================================================================
Routine to handle Reg BC Dec display mode
============================================================================
*/
void cb_reg_bc_dec(Fl_Widget* w, void*)
{
	char str[8];

	strcpy(cpuregs_ctrl.sBCfmt,  "%d");
	sprintf(str, cpuregs_ctrl.sBCfmt, BC);
	cpuregs_ctrl.pRegBC->value(str);

	cpuregs_ctrl.pAllHex->value(0);
	cpuregs_ctrl.pAllDec->value(0);
}

/*
============================================================================
Routine to handle Reg DE Hex display mode
============================================================================
*/
void cb_reg_de_hex(Fl_Widget* w, void*)
{
	char str[8];

	strcpy(cpuregs_ctrl.sDEfmt,  "0x%04X");
	sprintf(str, cpuregs_ctrl.sDEfmt, DE);
	cpuregs_ctrl.pRegDE->value(str);

	cpuregs_ctrl.pAllHex->value(0);
	cpuregs_ctrl.pAllDec->value(0);
}

/*
============================================================================
Routine to handle Reg DE Dec display mode
============================================================================
*/
void cb_reg_de_dec(Fl_Widget* w, void*)
{
	char str[8];

	strcpy(cpuregs_ctrl.sDEfmt,  "%d");
	sprintf(str, cpuregs_ctrl.sDEfmt, DE);
	cpuregs_ctrl.pRegDE->value(str);

	cpuregs_ctrl.pAllHex->value(0);
	cpuregs_ctrl.pAllDec->value(0);
}

/*
============================================================================
Routine to handle Reg HL Hex display mode
============================================================================
*/
void cb_reg_hl_hex(Fl_Widget* w, void*)
{
	char str[8];

	strcpy(cpuregs_ctrl.sHLfmt,  "0x%04X");
	sprintf(str, cpuregs_ctrl.sHLfmt, HL);
	cpuregs_ctrl.pRegHL->value(str);

	cpuregs_ctrl.pAllHex->value(0);
	cpuregs_ctrl.pAllDec->value(0);
}

/*
============================================================================
Routine to handle Reg HL Dec display mode
============================================================================
*/
void cb_reg_hl_dec(Fl_Widget* w, void*)
{
	char str[8];

	strcpy(cpuregs_ctrl.sHLfmt,  "%d");
	sprintf(str, cpuregs_ctrl.sHLfmt, HL);
	cpuregs_ctrl.pRegHL->value(str);

	cpuregs_ctrl.pAllHex->value(0);
	cpuregs_ctrl.pAllDec->value(0);
}

/*
============================================================================
Routine to handle Reg M Hex display mode
============================================================================
*/
void cb_reg_m_hex(Fl_Widget* w, void*)
{
	char str[8];

	strcpy(cpuregs_ctrl.sMfmt,  "0x%02X");
	sprintf(str, cpuregs_ctrl.sMfmt, get_m());
	cpuregs_ctrl.pRegM->value(str);

	cpuregs_ctrl.pAllHex->value(0);
	cpuregs_ctrl.pAllDec->value(0);
}

/*
============================================================================
Routine to handle Reg M Dec display mode
============================================================================
*/
void cb_reg_m_dec(Fl_Widget* w, void*)
{
	char str[8];

	strcpy(cpuregs_ctrl.sMfmt,  "%d");
	sprintf(str, cpuregs_ctrl.sMfmt, get_m());
	cpuregs_ctrl.pRegM->value(str);

	cpuregs_ctrl.pAllHex->value(0);
	cpuregs_ctrl.pAllDec->value(0);
}

/*
============================================================================
Routine to handle Reg All Hex display mode
============================================================================
*/
void cb_reg_all_hex(Fl_Widget* w, void*)
{
	// Change format to "Hex" for all
	cb_reg_a_hex(w, NULL);
	cb_reg_b_hex(w, NULL);
	cb_reg_c_hex(w, NULL);
	cb_reg_d_hex(w, NULL);
	cb_reg_e_hex(w, NULL);
	cb_reg_h_hex(w, NULL);
	cb_reg_l_hex(w, NULL);
	cb_reg_pc_hex(w, NULL);
	cb_reg_sp_hex(w, NULL);
	cb_reg_bc_hex(w, NULL);
	cb_reg_de_hex(w, NULL);
	cb_reg_hl_hex(w, NULL);
	cb_reg_m_hex(w, NULL);

	// Deselect all "Dec" buttons
	cpuregs_ctrl.pADec->value(0);
	cpuregs_ctrl.pBDec->value(0);
	cpuregs_ctrl.pCDec->value(0);
	cpuregs_ctrl.pDDec->value(0);
	cpuregs_ctrl.pEDec->value(0);
	cpuregs_ctrl.pHDec->value(0);
	cpuregs_ctrl.pLDec->value(0);
	cpuregs_ctrl.pPCDec->value(0);
	cpuregs_ctrl.pSPDec->value(0);
	cpuregs_ctrl.pBCDec->value(0);
	cpuregs_ctrl.pDEDec->value(0);
	cpuregs_ctrl.pHLDec->value(0);
	cpuregs_ctrl.pMDec->value(0);

	// Select all "Hex" buttons
	cpuregs_ctrl.pAHex->value(1);
	cpuregs_ctrl.pBHex->value(1);
	cpuregs_ctrl.pCHex->value(1);
	cpuregs_ctrl.pDHex->value(1);
	cpuregs_ctrl.pEHex->value(1);
	cpuregs_ctrl.pHHex->value(1);
	cpuregs_ctrl.pLHex->value(1);
	cpuregs_ctrl.pPCHex->value(1);
	cpuregs_ctrl.pSPHex->value(1);
	cpuregs_ctrl.pBCHex->value(1);
	cpuregs_ctrl.pDEHex->value(1);
	cpuregs_ctrl.pHLHex->value(1);
	cpuregs_ctrl.pMHex->value(1);

	// reselect the "All Hex" button
	cpuregs_ctrl.pAllHex->value(1);
}

/*
============================================================================
Routine to handle Reg All Dec display mode
============================================================================
*/
void cb_reg_all_dec(Fl_Widget* w, void*)
{
	// Change format of all items to Dec
	cb_reg_a_dec(w, NULL);
	cb_reg_b_dec(w, NULL);
	cb_reg_c_dec(w, NULL);
	cb_reg_d_dec(w, NULL);
	cb_reg_e_dec(w, NULL);
	cb_reg_h_dec(w, NULL);
	cb_reg_l_dec(w, NULL);
	cb_reg_pc_dec(w, NULL);
	cb_reg_sp_dec(w, NULL);
	cb_reg_bc_dec(w, NULL);
	cb_reg_de_dec(w, NULL);
	cb_reg_hl_dec(w, NULL);
	cb_reg_m_dec(w, NULL);

	// Deselect all "Hex" buttons
	cpuregs_ctrl.pAHex->value(0);
	cpuregs_ctrl.pBHex->value(0);
	cpuregs_ctrl.pCHex->value(0);
	cpuregs_ctrl.pDHex->value(0);
	cpuregs_ctrl.pEHex->value(0);
	cpuregs_ctrl.pHHex->value(0);
	cpuregs_ctrl.pLHex->value(0);
	cpuregs_ctrl.pPCHex->value(0);
	cpuregs_ctrl.pSPHex->value(0);
	cpuregs_ctrl.pBCHex->value(0);
	cpuregs_ctrl.pDEHex->value(0);
	cpuregs_ctrl.pHLHex->value(0);
	cpuregs_ctrl.pMHex->value(0);

	// Select all "Dec" buttons
	cpuregs_ctrl.pADec->value(1);
	cpuregs_ctrl.pBDec->value(1);
	cpuregs_ctrl.pCDec->value(1);
	cpuregs_ctrl.pDDec->value(1);
	cpuregs_ctrl.pEDec->value(1);
	cpuregs_ctrl.pHDec->value(1);
	cpuregs_ctrl.pLDec->value(1);
	cpuregs_ctrl.pPCDec->value(1);
	cpuregs_ctrl.pSPDec->value(1);
	cpuregs_ctrl.pBCDec->value(1);
	cpuregs_ctrl.pDEDec->value(1);
	cpuregs_ctrl.pHLDec->value(1);
	cpuregs_ctrl.pMDec->value(1);

	// Reselect "All" Dec button
	cpuregs_ctrl.pAllDec->value(1);
}

/*
============================================================================
Routine to create the PeripheralSetup Window and tabs
============================================================================
*/
void cb_CpuRegs (Fl_Widget* w, void*)
{
	Fl_Box*			o;

	if (gcpuw != NULL)
		return;

	// Create Peripheral Setup window
	gcpuw = new Fl_Window(630, 480, "CPU Registers");
	gcpuw->callback(cb_cpuregswin);

	// Create a menu for the new window.
	cpuregs_ctrl.pMenu = new Fl_Menu_Bar(0, 0, 630, MENU_HEIGHT-2);
	cpuregs_ctrl.pMenu->menu(gCpuRegs_menuitems);

	// Create static text boxes
	o = new Fl_Box(FL_NO_BOX, 20, 20+MENU_HEIGHT, 50, 15, "PC");
	o->align(FL_ALIGN_LEFT|FL_ALIGN_INSIDE);
	o = new Fl_Box(FL_NO_BOX, 20, 45+MENU_HEIGHT, 50, 15, "SP");
	o->align(FL_ALIGN_LEFT|FL_ALIGN_INSIDE);
	o = new Fl_Box(FL_NO_BOX, 20, 70+MENU_HEIGHT, 50, 15, "B");
	o->align(FL_ALIGN_LEFT|FL_ALIGN_INSIDE);
	o = new Fl_Box(FL_NO_BOX, 20, 95+MENU_HEIGHT, 50, 15, "D");
	o->align(FL_ALIGN_LEFT|FL_ALIGN_INSIDE);
	o = new Fl_Box(FL_NO_BOX, 20, 120+MENU_HEIGHT, 50, 15, "H");
	o->align(FL_ALIGN_LEFT|FL_ALIGN_INSIDE);

	o = new Fl_Box(FL_NO_BOX, 120, 20+MENU_HEIGHT, 50, 15, "A");
	o->align(FL_ALIGN_LEFT|FL_ALIGN_INSIDE);
	o = new Fl_Box(FL_NO_BOX, 120, 70+MENU_HEIGHT, 50, 15, "C");
	o->align(FL_ALIGN_LEFT|FL_ALIGN_INSIDE);
	o = new Fl_Box(FL_NO_BOX, 120, 95+MENU_HEIGHT, 50, 15, "E");
	o->align(FL_ALIGN_LEFT|FL_ALIGN_INSIDE);
	o = new Fl_Box(FL_NO_BOX, 120, 120+MENU_HEIGHT, 50, 15, "L");
	o->align(FL_ALIGN_LEFT|FL_ALIGN_INSIDE);

	o = new Fl_Box(FL_NO_BOX, 220, 70+MENU_HEIGHT, 50, 15, "BC");
	o->align(FL_ALIGN_LEFT|FL_ALIGN_INSIDE);
	o = new Fl_Box(FL_NO_BOX, 220, 95+MENU_HEIGHT, 50, 15, "DE");
	o->align(FL_ALIGN_LEFT|FL_ALIGN_INSIDE);
	o = new Fl_Box(FL_NO_BOX, 220, 120+MENU_HEIGHT, 50, 15, "HL");
	o->align(FL_ALIGN_LEFT|FL_ALIGN_INSIDE);
	o = new Fl_Box(FL_NO_BOX, 220, 145+MENU_HEIGHT, 50, 15, "M");
	o->align(FL_ALIGN_LEFT|FL_ALIGN_INSIDE);


	// Program Counter edit box
	cpuregs_ctrl.pRegPC = new Fl_Input(50, 19+MENU_HEIGHT, 60, 20, "");
	cpuregs_ctrl.pRegPC->deactivate();
	cpuregs_ctrl.pRegPC->callback(cb_reg_pc_changed);
	cpuregs_ctrl.pRegPC->value("0x0000");

	// Accumulator Edit Box
	cpuregs_ctrl.pRegA = new Fl_Input(150, 19+MENU_HEIGHT, 60, 20, "");
	cpuregs_ctrl.pRegA->deactivate();
	cpuregs_ctrl.pRegA->value("0x00");

	// Stack Pointer edit box
	cpuregs_ctrl.pRegSP = new Fl_Input(50, 44+MENU_HEIGHT, 60, 20, "");
	cpuregs_ctrl.pRegSP->deactivate();
	cpuregs_ctrl.pRegSP->callback(cb_reg_sp_changed);
	cpuregs_ctrl.pRegSP->value("0x0000");

	// Create Flag checkboxes
	// Sign flag
	cpuregs_ctrl.pSFlag = new Fl_Check_Button(223, 19+MENU_HEIGHT, 30, 20, "S");
	cpuregs_ctrl.pSFlag->deactivate();

	// Zero flag
	cpuregs_ctrl.pZFlag = new Fl_Check_Button(253, 19+MENU_HEIGHT, 30, 20, "Z");
	cpuregs_ctrl.pZFlag->deactivate();

	// Carry flag
	cpuregs_ctrl.pCFlag = new Fl_Check_Button(290, 19+MENU_HEIGHT, 30, 20, "C");
	cpuregs_ctrl.pCFlag->deactivate();

	// TS flag
	cpuregs_ctrl.pTSFlag = new Fl_Check_Button(148, 44+MENU_HEIGHT, 40, 20, "TS");
	cpuregs_ctrl.pTSFlag->deactivate();

	// AC flag
	cpuregs_ctrl.pACFlag = new Fl_Check_Button(185, 44+MENU_HEIGHT, 40, 20, "AC");
	cpuregs_ctrl.pACFlag->deactivate();

	// Parity flag
	cpuregs_ctrl.pPFlag = new Fl_Check_Button(223, 44+MENU_HEIGHT, 30, 20, "P");
	cpuregs_ctrl.pPFlag->deactivate();

	// Overflow flag
	cpuregs_ctrl.pOVFlag = new Fl_Check_Button(253, 44+MENU_HEIGHT, 40, 20, "OV");
	cpuregs_ctrl.pOVFlag->deactivate();

	// X flag
	cpuregs_ctrl.pXFlag = new Fl_Check_Button(290, 44+MENU_HEIGHT, 30, 20, "X");
	cpuregs_ctrl.pXFlag->deactivate();

	// Register B  Edit Box
	cpuregs_ctrl.pRegB = new Fl_Input(50, 69+MENU_HEIGHT, 60, 20, "");
	cpuregs_ctrl.pRegB->deactivate();
	cpuregs_ctrl.pRegB->callback(cb_reg_b_changed);
	cpuregs_ctrl.pRegB->value("0x00");

	// Register C Edit Box
	cpuregs_ctrl.pRegC = new Fl_Input(150, 69+MENU_HEIGHT, 60, 20, "");
	cpuregs_ctrl.pRegC->deactivate();
	cpuregs_ctrl.pRegC->callback(cb_reg_c_changed);
	cpuregs_ctrl.pRegC->value("0x00");

	// Register BC Edit Box
	cpuregs_ctrl.pRegBC = new Fl_Input(250, 69+MENU_HEIGHT, 60, 20, "");
	cpuregs_ctrl.pRegBC->deactivate();
	cpuregs_ctrl.pRegBC->callback(cb_reg_bc_changed);
	cpuregs_ctrl.pRegBC->value("0x0000");

	// Register D  Edit Box
	cpuregs_ctrl.pRegD = new Fl_Input(50, 94+MENU_HEIGHT, 60, 20, "");
	cpuregs_ctrl.pRegD->deactivate();
	cpuregs_ctrl.pRegD->callback(cb_reg_d_changed);
	cpuregs_ctrl.pRegD->value("0x00");

	// Register E Edit Box
	cpuregs_ctrl.pRegE = new Fl_Input(150, 94+MENU_HEIGHT, 60, 20, "");
	cpuregs_ctrl.pRegE->deactivate();
	cpuregs_ctrl.pRegE->callback(cb_reg_e_changed);
	cpuregs_ctrl.pRegE->value("0x00");

	// Register DE Edit Box
	cpuregs_ctrl.pRegDE = new Fl_Input(250, 94+MENU_HEIGHT, 60, 20, "");
	cpuregs_ctrl.pRegDE->deactivate();
	cpuregs_ctrl.pRegDE->callback(cb_reg_de_changed);
	cpuregs_ctrl.pRegDE->value("0x0000");

	// Register H  Edit Box
	cpuregs_ctrl.pRegH = new Fl_Input(50, 119+MENU_HEIGHT, 60, 20, "");
	cpuregs_ctrl.pRegH->deactivate();
	cpuregs_ctrl.pRegH->callback(cb_reg_h_changed);
	cpuregs_ctrl.pRegH->value("0x00");

	// Register L Edit Box
	cpuregs_ctrl.pRegL = new Fl_Input(150, 119+MENU_HEIGHT, 60, 20, "");
	cpuregs_ctrl.pRegL->deactivate();
	cpuregs_ctrl.pRegL->callback(cb_reg_l_changed);
	cpuregs_ctrl.pRegL->value("0x00");

	// Register HL Edit Box
	cpuregs_ctrl.pRegHL = new Fl_Input(250, 119+MENU_HEIGHT, 60, 20, "");
	cpuregs_ctrl.pRegHL->deactivate();
	cpuregs_ctrl.pRegHL->callback(cb_reg_hl_changed);
	cpuregs_ctrl.pRegHL->value("0x0000");

	// Register M Edit Box
	cpuregs_ctrl.pRegM = new Fl_Input(250, 144+MENU_HEIGHT, 60, 20, "");
	cpuregs_ctrl.pRegM->deactivate();
	cpuregs_ctrl.pRegM->value("0x00");


	strcpy(cpuregs_ctrl.sPCfmt, "0x%04X");
	strcpy(cpuregs_ctrl.sSPfmt, "0x%04X");
	strcpy(cpuregs_ctrl.sBCfmt, "0x%04X");
	strcpy(cpuregs_ctrl.sDEfmt, "0x%04X");
	strcpy(cpuregs_ctrl.sHLfmt, "0x%04X");
	strcpy(cpuregs_ctrl.sAfmt,  "0x%02X");
	strcpy(cpuregs_ctrl.sBfmt,  "0x%02X");
	strcpy(cpuregs_ctrl.sCfmt,  "0x%02X");
	strcpy(cpuregs_ctrl.sDfmt,  "0x%02X");
	strcpy(cpuregs_ctrl.sEfmt,  "0x%02X");
	strcpy(cpuregs_ctrl.sHfmt,  "0x%02X");
	strcpy(cpuregs_ctrl.sLfmt,  "0x%02X");
	strcpy(cpuregs_ctrl.sMfmt,  "0x%02X");

	// Group the Reg "All" radio buttons together
	{
		cpuregs_ctrl.g = new Fl_Group(440, 10 + MENU_HEIGHT, 100, 20, "");

		// Display format for Reg A
		o = new Fl_Box(FL_NO_BOX, 410, 11+MENU_HEIGHT, 50, 15, "All");
		o->align(FL_ALIGN_LEFT|FL_ALIGN_INSIDE);

		cpuregs_ctrl.pAllHex = new Fl_Round_Button(440, 10 + MENU_HEIGHT, 50, 20, "Hex");
		cpuregs_ctrl.pAllHex->type(FL_RADIO_BUTTON);
		cpuregs_ctrl.pAllHex->callback(cb_reg_all_hex);
		cpuregs_ctrl.pAllHex->value(1);

		cpuregs_ctrl.pAllDec  = new Fl_Round_Button(490, 10 + MENU_HEIGHT, 50, 20, "Dec");
		cpuregs_ctrl.pAllDec->type(FL_RADIO_BUTTON);
		cpuregs_ctrl.pAllDec->callback(cb_reg_all_dec);
		cpuregs_ctrl.g->end();
	}

	// Group the Reg A radio buttons together
	{
		cpuregs_ctrl.g = new Fl_Group(370, 30 + MENU_HEIGHT, 100, 20, "");

		// Display format for Reg A
		o = new Fl_Box(FL_NO_BOX, 340, 31+MENU_HEIGHT, 50, 15, "A");
		o->align(FL_ALIGN_LEFT|FL_ALIGN_INSIDE);

		cpuregs_ctrl.pAHex = new Fl_Round_Button(370, 30 + MENU_HEIGHT, 50, 20, "Hex");
		cpuregs_ctrl.pAHex->type(FL_RADIO_BUTTON);
		cpuregs_ctrl.pAHex->callback(cb_reg_a_hex);
		cpuregs_ctrl.pAHex->value(1);

		cpuregs_ctrl.pADec  = new Fl_Round_Button(420, 30 + MENU_HEIGHT, 50, 20, "Dec");
		cpuregs_ctrl.pADec->type(FL_RADIO_BUTTON);
		cpuregs_ctrl.pADec->callback(cb_reg_a_dec);
		cpuregs_ctrl.g->end();
	}

	// Group the Reg B radio buttons togther
	{
		cpuregs_ctrl.g = new Fl_Group(370, 50 + MENU_HEIGHT, 100, 20, "");

		// Display format for Reg B
		o = new Fl_Box(FL_NO_BOX, 340, 51+MENU_HEIGHT, 50, 15, "B");
		o->align(FL_ALIGN_LEFT|FL_ALIGN_INSIDE);

		cpuregs_ctrl.pBHex = new Fl_Round_Button(370, 50 + MENU_HEIGHT, 50, 20, "Hex");
		cpuregs_ctrl.pBHex->type(FL_RADIO_BUTTON);
		cpuregs_ctrl.pBHex->callback(cb_reg_b_hex);
		cpuregs_ctrl.pBHex->value(1);

		cpuregs_ctrl.pBDec  = new Fl_Round_Button(420, 50 + MENU_HEIGHT, 50, 20, "Dec");
		cpuregs_ctrl.pBDec->type(FL_RADIO_BUTTON);
		cpuregs_ctrl.pBDec->callback(cb_reg_b_dec);
		cpuregs_ctrl.g->end();
	}

	// Group the Reg C radio buttons togther
	{
		cpuregs_ctrl.g = new Fl_Group(370, 70 + MENU_HEIGHT, 100, 20, "");

		// Display format for Reg C
		o = new Fl_Box(FL_NO_BOX, 340, 71+MENU_HEIGHT, 50, 15, "C");
		o->align(FL_ALIGN_LEFT|FL_ALIGN_INSIDE);

		cpuregs_ctrl.pCHex = new Fl_Round_Button(370, 70 + MENU_HEIGHT, 50, 20, "Hex");
		cpuregs_ctrl.pCHex->type(FL_RADIO_BUTTON);
		cpuregs_ctrl.pCHex->callback(cb_reg_c_hex);
		cpuregs_ctrl.pCHex->value(1);

		cpuregs_ctrl.pCDec  = new Fl_Round_Button(420, 70 + MENU_HEIGHT, 50, 20, "Dec");
		cpuregs_ctrl.pCDec->type(FL_RADIO_BUTTON);
		cpuregs_ctrl.pCDec->callback(cb_reg_c_dec);
		cpuregs_ctrl.g->end();
	}

	// Group the Reg D radio buttons togther
	{
		cpuregs_ctrl.g = new Fl_Group(370, 90 + MENU_HEIGHT, 100, 20, "");

		// Display format for Reg D
		o = new Fl_Box(FL_NO_BOX, 340, 91+MENU_HEIGHT, 50, 15, "D");
		o->align(FL_ALIGN_LEFT|FL_ALIGN_INSIDE);

		cpuregs_ctrl.pDHex = new Fl_Round_Button(370, 90 + MENU_HEIGHT, 50, 20, "Hex");
		cpuregs_ctrl.pDHex->type(FL_RADIO_BUTTON);
		cpuregs_ctrl.pDHex->callback(cb_reg_d_hex);
		cpuregs_ctrl.pDHex->value(1);

		cpuregs_ctrl.pDDec  = new Fl_Round_Button(420, 90 + MENU_HEIGHT, 50, 20, "Dec");
		cpuregs_ctrl.pDDec->type(FL_RADIO_BUTTON);
		cpuregs_ctrl.pDDec->callback(cb_reg_d_dec);
		cpuregs_ctrl.g->end();
	}

	// Group the Reg E radio buttons togther
	{
		cpuregs_ctrl.g = new Fl_Group(370, 110 + MENU_HEIGHT, 100, 20, "");

		// Display format for Reg E
		o = new Fl_Box(FL_NO_BOX, 340, 111+MENU_HEIGHT, 50, 15, "E");
		o->align(FL_ALIGN_LEFT|FL_ALIGN_INSIDE);

		cpuregs_ctrl.pEHex = new Fl_Round_Button(370, 110 + MENU_HEIGHT, 50, 20, "Hex");
		cpuregs_ctrl.pEHex->type(FL_RADIO_BUTTON);
		cpuregs_ctrl.pEHex->callback(cb_reg_e_hex);
		cpuregs_ctrl.pEHex->value(1);

		cpuregs_ctrl.pEDec  = new Fl_Round_Button(420, 110 + MENU_HEIGHT, 50, 20, "Dec");
		cpuregs_ctrl.pEDec->type(FL_RADIO_BUTTON);
		cpuregs_ctrl.pEDec->callback(cb_reg_e_dec);
		cpuregs_ctrl.g->end();
	}

	// Group the Reg H radio buttons togther
	{
		cpuregs_ctrl.g = new Fl_Group(370, 130 + MENU_HEIGHT, 100, 20, "");

		// Display format for Reg H
		o = new Fl_Box(FL_NO_BOX, 340, 131+MENU_HEIGHT, 50, 15, "H");
		o->align(FL_ALIGN_LEFT|FL_ALIGN_INSIDE);

		cpuregs_ctrl.pHHex = new Fl_Round_Button(370, 130 + MENU_HEIGHT, 50, 20, "Hex");
		cpuregs_ctrl.pHHex->type(FL_RADIO_BUTTON);
		cpuregs_ctrl.pHHex->callback(cb_reg_h_hex);
		cpuregs_ctrl.pHHex->value(1);

		cpuregs_ctrl.pHDec  = new Fl_Round_Button(420, 130 + MENU_HEIGHT, 50, 20, "Dec");
		cpuregs_ctrl.pHDec->type(FL_RADIO_BUTTON);
		cpuregs_ctrl.pHDec->callback(cb_reg_h_dec);
		cpuregs_ctrl.g->end();
	}

	// Group the Reg L radio buttons togther
	{
		cpuregs_ctrl.g = new Fl_Group(370, 150 + MENU_HEIGHT, 100, 20, "");

		// Display format for Reg L
		o = new Fl_Box(FL_NO_BOX, 340, 151+MENU_HEIGHT, 50, 15, "L");
		o->align(FL_ALIGN_LEFT|FL_ALIGN_INSIDE);

		cpuregs_ctrl.pLHex = new Fl_Round_Button(370, 150 + MENU_HEIGHT, 50, 20, "Hex");
		cpuregs_ctrl.pLHex->type(FL_RADIO_BUTTON);
		cpuregs_ctrl.pLHex->callback(cb_reg_l_hex);
		cpuregs_ctrl.pLHex->value(1);

		cpuregs_ctrl.pLDec  = new Fl_Round_Button(420, 150 + MENU_HEIGHT, 50, 20, "Dec");
		cpuregs_ctrl.pLDec->type(FL_RADIO_BUTTON);
		cpuregs_ctrl.pLDec->callback(cb_reg_l_dec);
		cpuregs_ctrl.g->end();
	}


	// Group the Reg PC radio buttons togther
	{
		cpuregs_ctrl.g = new Fl_Group(520, 30 + MENU_HEIGHT, 100, 20, "");

		// Display format for Reg L
		o = new Fl_Box(FL_NO_BOX, 490, 31+MENU_HEIGHT, 50, 15, "PC");
		o->align(FL_ALIGN_LEFT|FL_ALIGN_INSIDE);

		cpuregs_ctrl.pPCHex = new Fl_Round_Button(520, 30 + MENU_HEIGHT, 50, 20, "Hex");
		cpuregs_ctrl.pPCHex->type(FL_RADIO_BUTTON);
		cpuregs_ctrl.pPCHex->callback(cb_reg_pc_hex);
		cpuregs_ctrl.pPCHex->value(1);

		cpuregs_ctrl.pPCDec  = new Fl_Round_Button(570, 30 + MENU_HEIGHT, 50, 20, "Dec");
		cpuregs_ctrl.pPCDec->type(FL_RADIO_BUTTON);
		cpuregs_ctrl.pPCDec->callback(cb_reg_pc_dec);
		cpuregs_ctrl.g->end();
	}

	// Group the Reg SP radio buttons togther
	{
		cpuregs_ctrl.g = new Fl_Group(520, 50 + MENU_HEIGHT, 100, 20, "");

		// Display format for Reg L
		o = new Fl_Box(FL_NO_BOX, 490, 51+MENU_HEIGHT, 50, 15, "SP");
		o->align(FL_ALIGN_LEFT|FL_ALIGN_INSIDE);

		cpuregs_ctrl.pSPHex = new Fl_Round_Button(520, 50 + MENU_HEIGHT, 50, 20, "Hex");
		cpuregs_ctrl.pSPHex->type(FL_RADIO_BUTTON);
		cpuregs_ctrl.pSPHex->callback(cb_reg_sp_hex);
		cpuregs_ctrl.pSPHex->value(1);

		cpuregs_ctrl.pSPDec  = new Fl_Round_Button(570, 50 + MENU_HEIGHT, 50, 20, "Dec");
		cpuregs_ctrl.pSPDec->type(FL_RADIO_BUTTON);
		cpuregs_ctrl.pSPDec->callback(cb_reg_sp_dec);
		cpuregs_ctrl.g->end();
	}

	// Group the Reg BC radio buttons togther
	{
		cpuregs_ctrl.g = new Fl_Group(520, 70 + MENU_HEIGHT, 100, 20, "");

		// Display format for Reg L
		o = new Fl_Box(FL_NO_BOX, 490, 71+MENU_HEIGHT, 50, 15, "BC");
		o->align(FL_ALIGN_LEFT|FL_ALIGN_INSIDE);

		cpuregs_ctrl.pBCHex = new Fl_Round_Button(520, 70 + MENU_HEIGHT, 50, 20, "Hex");
		cpuregs_ctrl.pBCHex->type(FL_RADIO_BUTTON);
		cpuregs_ctrl.pBCHex->callback(cb_reg_bc_hex);
		cpuregs_ctrl.pBCHex->value(1);

		cpuregs_ctrl.pBCDec  = new Fl_Round_Button(570, 70 + MENU_HEIGHT, 50, 20, "Dec");
		cpuregs_ctrl.pBCDec->type(FL_RADIO_BUTTON);
		cpuregs_ctrl.pBCDec->callback(cb_reg_bc_dec);
		cpuregs_ctrl.g->end();
	}

	// Group the Reg DE radio buttons togther
	{
		cpuregs_ctrl.g = new Fl_Group(520, 90 + MENU_HEIGHT, 100, 20, "");

		// Display format for Reg L
		o = new Fl_Box(FL_NO_BOX, 490, 91+MENU_HEIGHT, 50, 15, "DE");
		o->align(FL_ALIGN_LEFT|FL_ALIGN_INSIDE);

		cpuregs_ctrl.pDEHex = new Fl_Round_Button(520, 90 + MENU_HEIGHT, 50, 20, "Hex");
		cpuregs_ctrl.pDEHex->type(FL_RADIO_BUTTON);
		cpuregs_ctrl.pDEHex->callback(cb_reg_de_hex);
		cpuregs_ctrl.pDEHex->value(1);

		cpuregs_ctrl.pDEDec  = new Fl_Round_Button(570, 90 + MENU_HEIGHT, 50, 20, "Dec");
		cpuregs_ctrl.pDEDec->type(FL_RADIO_BUTTON);
		cpuregs_ctrl.pDEDec->callback(cb_reg_de_dec);
		cpuregs_ctrl.g->end();
	}

	// Group the Reg HL radio buttons togther
	{
		cpuregs_ctrl.g = new Fl_Group(520, 110 + MENU_HEIGHT, 100, 20, "");

		// Display format for Reg HL
		o = new Fl_Box(FL_NO_BOX, 490, 111+MENU_HEIGHT, 50, 15, "HL");
		o->align(FL_ALIGN_LEFT|FL_ALIGN_INSIDE);

		cpuregs_ctrl.pHLHex = new Fl_Round_Button(520, 110 + MENU_HEIGHT, 50, 20, "Hex");
		cpuregs_ctrl.pHLHex->type(FL_RADIO_BUTTON);
		cpuregs_ctrl.pHLHex->callback(cb_reg_hl_hex);
		cpuregs_ctrl.pHLHex->value(1);

		cpuregs_ctrl.pHLDec  = new Fl_Round_Button(570, 110 + MENU_HEIGHT, 50, 20, "Dec");
		cpuregs_ctrl.pHLDec->type(FL_RADIO_BUTTON);
		cpuregs_ctrl.pHLDec->callback(cb_reg_hl_dec);
		cpuregs_ctrl.g->end();
	}

	// Group the Reg M radio buttons togther
	{
		cpuregs_ctrl.g = new Fl_Group(520, 130 + MENU_HEIGHT, 100, 20, "");

		// Display format for Reg HL
		o = new Fl_Box(FL_NO_BOX, 490, 131+MENU_HEIGHT, 50, 15, "M");
		o->align(FL_ALIGN_LEFT|FL_ALIGN_INSIDE);

		cpuregs_ctrl.pMHex = new Fl_Round_Button(520, 130 + MENU_HEIGHT, 50, 20, "Hex");
		cpuregs_ctrl.pMHex->type(FL_RADIO_BUTTON);
		cpuregs_ctrl.pMHex->callback(cb_reg_m_hex);
		cpuregs_ctrl.pMHex->value(1);

		cpuregs_ctrl.pMDec  = new Fl_Round_Button(570, 130 + MENU_HEIGHT, 50, 20, "Dec");
		cpuregs_ctrl.pMDec->type(FL_RADIO_BUTTON);
		cpuregs_ctrl.pMDec->callback(cb_reg_m_dec);
		cpuregs_ctrl.g->end();
	}

	// Create Instruction Trace Text Boxes
	o = new Fl_Box(FL_NO_BOX, 20, 180+MENU_HEIGHT, 50, 15, "Instruction Trace");
	o->align(FL_ALIGN_LEFT|FL_ALIGN_INSIDE);
	o = new Fl_Box(FL_DOWN_BOX, 20, 200+MENU_HEIGHT, 520, 124, "");
	cpuregs_ctrl.iInstTraceHead = 0;

	// Create checkbox to disable realtime trace
	cpuregs_ctrl.pDisableTrace = new Fl_Check_Button(200, 180+MENU_HEIGHT, 180, 20, "Disable Real-time Trace");
	cpuregs_ctrl.pDisableTrace->callback(cb_disable_realtime_trace);


	// Create Stack Box
	o = new Fl_Box(FL_NO_BOX, 550, 180+MENU_HEIGHT, 50, 15, "Stack");
	o->align(FL_ALIGN_LEFT|FL_ALIGN_INSIDE);
	o = new Fl_Box(FL_DOWN_BOX, 550, 200+MENU_HEIGHT, 60, 124, "");

	// Create Breakpoint edit boxes
	o = new Fl_Box(FL_NO_BOX, 20, 375, 100, 15, "Breakpoints:");
	o->align(FL_ALIGN_LEFT|FL_ALIGN_INSIDE);
	cpuregs_ctrl.pBreak1 = new Fl_Input(120, 375, 60, 20, "");
	cpuregs_ctrl.pBreak1->deactivate();
	cpuregs_ctrl.pBreak1->value("");
	cpuregs_ctrl.pBreakDisable1 = new Fl_Check_Button(125, 395, 40, 20, "Off");
	cpuregs_ctrl.pBreakDisable1->deactivate();
	cpuregs_ctrl.pBreak2 = new Fl_Input(200, 375, 60, 20, "");
	cpuregs_ctrl.pBreak2->deactivate();
	cpuregs_ctrl.pBreak2->value("");
	cpuregs_ctrl.pBreakDisable2 = new Fl_Check_Button(205, 395, 40, 20, "Off");
	cpuregs_ctrl.pBreakDisable2->deactivate();
	cpuregs_ctrl.pBreak3 = new Fl_Input(280, 375, 60, 20, "");
	cpuregs_ctrl.pBreak3->deactivate();
	cpuregs_ctrl.pBreak3->value("");
	cpuregs_ctrl.pBreakDisable3 = new Fl_Check_Button(285, 395, 40, 20, "Off");
	cpuregs_ctrl.pBreakDisable3->deactivate();
	cpuregs_ctrl.pBreak4 = new Fl_Input(360, 375, 60, 20, "");
	cpuregs_ctrl.pBreak4->deactivate();
	cpuregs_ctrl.pBreak4->value("");
	cpuregs_ctrl.pBreakDisable4 = new Fl_Check_Button(365, 395, 40, 20, "Off");
	cpuregs_ctrl.pBreakDisable4->deactivate();

	cpuregs_ctrl.pDebugInts = new Fl_Check_Button(465, 375, 40, 20, "Debug ISRs");
	cpuregs_ctrl.pDebugInts->value(gDebugInts);

	// Create Stop button
	cpuregs_ctrl.pStop = new Fl_Button(30, 445, 80, 25, "Stop");
	cpuregs_ctrl.pStop->callback(cb_debug_stop);

	// Create Step button
	cpuregs_ctrl.pStep = new Fl_Button(130, 445, 80, 25, "Step");
	cpuregs_ctrl.pStep->deactivate();
	cpuregs_ctrl.pStep->callback(cb_debug_step);

	// Create Step button
	cpuregs_ctrl.pStepOver = new Fl_Button(230, 445, 100, 25, "Step Over");
	cpuregs_ctrl.pStepOver->deactivate();
	cpuregs_ctrl.pStepOver->callback(cb_debug_step_over);

	// Create Run button
	cpuregs_ctrl.pRun = new Fl_Button(350, 445, 80, 25, "Run");
	cpuregs_ctrl.pRun->deactivate();
	cpuregs_ctrl.pRun->callback(cb_debug_run);

	// Create Redraw button
	cpuregs_ctrl.pRedraw = new Fl_Button(450, 445, 80, 25, "Redraw");
	cpuregs_ctrl.pRedraw->deactivate();
	cpuregs_ctrl.pRedraw->callback(cb_redraw_trace);

	// Set Debug callback
	debug_set_monitor_callback(debug_cpuregs_cb);

	// Show the new window
	gcpuw->show();

	// Indicate an active debug window
	gDebugActive++;
	cpuregs_ctrl.breakMonitorFreq = 0;
	for (int x = 0; x < 5; x++)
	{
		cpuregs_ctrl.breakAddr[x] = -1;
		cpuregs_ctrl.breakEnable[x] = 1;
	}
}



