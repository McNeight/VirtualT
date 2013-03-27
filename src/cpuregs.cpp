/* cpuregs.cpp */

/* $Id: cpuregs.cpp,v 1.21 2013/03/15 00:35:54 kpettit1 Exp $ */

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
#include <FL/Fl_Menu_Item.H>
#include <FL/Fl_Menu_Bar.H>

#include "FLU/Flu_File_Chooser.h"

#if defined(WIN32)
#include <windows.h>
#endif
#include <stdio.h>
#include <string.h>
#include <time.h>

#include "VirtualT.h"
#include "cpuregs.h"
#include "m100emu.h"
#include "disassemble.h"
#include "periph.h"
#include "memedit.h"
#include "cpu.h"
#include "memory.h"
#include "fileview.h"

#define		MENU_HEIGHT					32

/*
============================================================================
Global variables
============================================================================
*/
VTCpuRegs		*gcpuw = NULL;
int				gStopCountdown = 0;
int				gSaveFreq = 0;
int				gDisableRealtimeTrace = 0;
int				gDebugCount = 0;
int				gDebugMonitorFreq = 32768;
VTDis			cpu_dis;

extern			Fl_Preferences	virtualt_prefs;
extern			Fl_Window*		gmew;				// Global Memory Edit Window
extern			Fl_Window		*MainWin;

extern "C"
{
extern volatile UINT64			cycles;
void memory_monitor_cb(void);
}

// Menu item callback definitions
static void cb_save_trace(Fl_Widget* w, void*);
static void cb_clear_trace(Fl_Widget* w, void*);
void cb_menu_run(Fl_Widget* w, void*);
void cb_menu_stop(Fl_Widget* w, void*);
void cb_menu_step(Fl_Widget* w, void*);
void cb_menu_step_over(Fl_Widget* w, void*);
static void cb_setup_trace(Fl_Widget* w, void*);

void cb_goto_next_marker(Fl_Widget* w, void*);
void cb_goto_prev_marker(Fl_Widget* w, void*);

// Other prototypes
void debug_cpuregs_cb (int);
void cb_Ide(Fl_Widget* w, void*) ;

// Menu items for the disassembler
Fl_Menu_Item gCpuRegs_menuitems[] = {
  { "T&race", 0, 0, 0, FL_SUBMENU },
	{ "Save",					FL_CTRL + 's', cb_save_trace },
	{ "Clear",					0, cb_clear_trace, 0, FL_MENU_DIVIDER },
	{ "Run",					FL_F + 5, cb_menu_run },
	{ "Stop",					FL_F + 6, cb_menu_stop },
	{ "Step",					FL_F + 8, cb_menu_step },
	{ "Step Over  ",			FL_F + 10, cb_menu_step_over, 0, FL_MENU_DIVIDER },
	{ "Setup...",				0, cb_setup_trace },
	{ 0 },
  { "&Tools", 0, 0, 0, FL_SUBMENU },
	{ "Assembler / IDE",       0, cb_Ide },
	{ "Disassembler",          0, disassembler_cb },
	{ "Memory Editor",         0, cb_MemoryEditor },
	{ "Peripheral Devices",    0, cb_PeripheralDevices },
	{ "Model T File Viewer",   0, cb_FileView },
	{ 0 },
  { 0 }
};

/*
============================================================================
Callback routine for the CPU Regs window
============================================================================
*/
void cb_cpuregswin (Fl_Widget* w, void* pOpaque)
{
	// Save the user preferences
	gcpuw->SavePrefs();

	// Collapse the window
	collapse_window(gcpuw);

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
				if ((pStr[x] == 'h') || (pStr[x] == 'H'))
				{
					hex = TRUE;
					break;
				}

				if (pStr[x] == ' ')
					break;
			}
	}

	// If it's a hex string, use hex conversion
	if (hex)
	{
		x = 0; val = 0;
		while ((pStr[x] != '\0') && (pStr[x] != 'h') && (pStr[x] != 'H') && (pStr[x] != ' '))
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
Check the breakpoint entries
============================================================================
*/
void VTCpuRegs::check_breakpoint_entries(void)
{
	const char*		pStr;
	int				val;
	int				x;

	// Get value of Breakpoint 1
	pStr = m_pBreak1->value();
	if (pStr[0] != 0)
	{
		val = str_to_i(pStr);
		m_breakAddr[0] = val;
	}
	else
		m_breakAddr[0] = -1;

	// Get value of Breakpoint 2
	pStr = m_pBreak2->value();
	if (pStr[0] != 0)
	{
		val = str_to_i(pStr);
		m_breakAddr[1] = val;
	}
	else
		m_breakAddr[1] = -1;

	// Get value of Breakpoint 3
	pStr = m_pBreak3->value();
	if (pStr[0] != 0)
	{
		val = str_to_i(pStr);
		m_breakAddr[2] = val;
	}
	else
		m_breakAddr[2] = -1;

	// Get value of Breakpoint 4
	pStr = m_pBreak4->value();
	if (pStr[0] != 0)
	{
		val = str_to_i(pStr);
		m_breakAddr[3] = val;
	}
	else
		m_breakAddr[3] = -1;

	// Check if any breakpoints set
	for (x = 0; x < 4; x++)
		if (m_breakAddr[x] != -1)
			break;

	// If no breakpoints set, restore old monitor frequency
	if (x == 4)
	{
		if (m_breakMonitorFreq != 0)
			gSaveFreq = m_breakMonitorFreq;
	}
	else
	{
		if (m_breakMonitorFreq == 0)
			m_breakMonitorFreq = gSaveFreq;
		gSaveFreq = 1;
		gDebugMonitorFreq = 1;
	}

	m_breakEnable[0] = !m_pBreakDisable1->value();
	m_breakEnable[1] = !m_pBreakDisable2->value();
	m_breakEnable[2] = !m_pBreakDisable3->value();
	m_breakEnable[3] = !m_pBreakDisable4->value();
}

/*
============================================================================
get_reg_edits:	This routine reads all register input fields and updates the
				CPU registers accordingly.
============================================================================
*/
void VTCpuRegs::get_reg_edits(void)
{
	const char*		pStr;
	int				val, flags;

	// Get value of PC
	pStr = m_pRegPC->value();
	if (pStr[0] != 0)
	{
		val = str_to_i(pStr);
		PCL = val & 0xFF; PCH = val >> 8;
	}

	// Get value of SP
	pStr = m_pRegSP->value();
	if (pStr[0] != 0)
	{
		val = str_to_i(pStr);
		SPL = val & 0xFF; SPH = val >> 8;
	}

	// Get value of A
	pStr = m_pRegA->value();
	if (pStr[0] != 0)
	{
		val = str_to_i(pStr);
		A = val & 0xFF;
	}

	// Get value of M
	pStr = m_pRegM->value();
	if (pStr[0] != 0)
	{
		val = str_to_i(pStr);
		set_memory8(HL, val & 0xFF);
	}

	// Get value of BC
	pStr = m_pRegBC->value();
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
			pStr = m_pRegB->value();
			if (pStr[0] != 0)
			{
				val = str_to_i(pStr);
				B = val;
			}
			// Now get new value for C
			pStr = m_pRegC->value();
			if (pStr[0] != 0)
			{
				val = str_to_i(pStr);
				C = val;
			}
		}
	}

	// Get value of DE
	pStr = m_pRegDE->value();
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
			pStr = m_pRegD->value();
			if (pStr[0] != 0)
			{
				val = str_to_i(pStr);
				D = val;
			}
			// Now get new value for E
			pStr = m_pRegE->value();
			if (pStr[0] != 0)
			{
				val = str_to_i(pStr);
				E = val;
			}
		}
	}

	// Get value of HL
	pStr = m_pRegHL->value();
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
			pStr = m_pRegH->value();
			if (pStr[0] != 0)
			{
				val = str_to_i(pStr);
				H = val;
			}
			// Now get new value for L
			pStr = m_pRegL->value();
			if (pStr[0] != 0)
			{
				val = str_to_i(pStr);
				L = val;
			}
		}
	}

	// Finally get updates to the flags
	flags = 0;
	if (m_pSFlag->value())
		flags |= SF_BIT;
	if (m_pZFlag->value())
		flags |= ZF_BIT;
	if (m_pXFlag->value())
		flags |= XF_BIT;
	if (m_pACFlag->value())
		flags |= AC_BIT;
	if (m_pTSFlag->value())
		flags |= TS_BIT;
	if (m_pPFlag->value())
		flags |= PF_BIT;
	if (m_pOVFlag->value())
		flags |= OV_BIT;
	if (m_pCFlag->value())
		flags |= CF_BIT;

	F = flags;

	check_breakpoint_entries();
}

/*
============================================================================
Draws the specified string at the given x,y location using color syntax
hilighting rules.
============================================================================
*/
void draw_color_syntax(const char* lineStr, int x, int y, int bold = FALSE)
{
	int					curx, idx, c, char_width;
	char				str[20];
	cpu_trace_colors_t*	pColors = &gcpuw->m_traceColors;

	// Start at given x location
	curx = x;
	char_width = (int) fl_width(" ");
	
	// Copy and draw the address
	strncpy(str, lineStr, 4);
	str[4] = '\0';
	idx = 4;
	fl_color(pColors->addr);
	if (bold && !gcpuw->m_inverseHilight)
		fl_font(FL_COURIER_BOLD_ITALIC, gcpuw->m_fontSize);
	else if (gcpuw->m_inverseHilight)
		fl_font(FL_COURIER_BOLD, gcpuw->m_fontSize);
	else
		fl_font(FL_COURIER, gcpuw->m_fontSize);
	fl_draw(str, curx, y);
	curx += (int) fl_width(str);

	// Copy and draw the execution location
	strncpy(str, &lineStr[4], 3);
	str[3] = '\0';
	idx += 3;
	fl_color(pColors->number);
	fl_draw(str, curx, y);
	curx += (int) fl_width(str);

	// Copy and draw the instruction
	c = 0;
	while (lineStr[idx] != ' ' && lineStr[idx] != '\0')
		str[c++] = lineStr[idx++];
	// Get the space too
	if (lineStr[idx] != '\0')
		str[c++] = lineStr[idx++];
	str[c] = '\0';
	fl_color(pColors->inst);
	if (!bold)
		fl_font(FL_COURIER_BOLD, gcpuw->m_fontSize);
	fl_draw(str, curx, y);
	curx += (int) fl_width(str);

	// Back to the selected font size
	if (bold && !gcpuw->m_inverseHilight)
		fl_font(FL_COURIER_BOLD_ITALIC, gcpuw->m_fontSize);
	else if (gcpuw->m_inverseHilight)
		fl_font(FL_COURIER_BOLD, gcpuw->m_fontSize);
	else
		fl_font(FL_COURIER, gcpuw->m_fontSize);

	// Check for end of string in case this is a look-ahead instruction
	if (lineStr[idx] == '\0')
	{
		// Back to foreground color and return
		fl_color(pColors->foreground);
		return;
	}

	// Determine the operand type.  First check for A,blah format
	if (lineStr[idx+1] == ',')
	{
		fl_color(pColors->reg);
		str[0] = lineStr[idx];
		str[1] = '\0';
		fl_draw(str, curx, y);
		curx += char_width;
		fl_color(pColors->foreground);
		fl_draw(",", curx, y);
		curx += char_width;
		idx += 2;
	}

	// Now determine any remaining operand type.  First look for register A, B, M, PSW, etc.
	if (lineStr[idx+1] == ' ' || lineStr[idx+1] == 'S' || lineStr[idx+1] == '\0')
		// It's a register operand
		fl_color(pColors->reg);
	else if (lineStr[idx+2] == 'H')
		// It's a numeric value
		fl_color(pColors->number);
	else
		// It's an address
		fl_color(pColors->addr);

	// Copy the operand up to the 1st space
	c = 0;
	while (lineStr[idx] != ' ' && lineStr[idx] != '\0')
		str[c++] = lineStr[idx++];

	// Now copy the spaces
	while (lineStr[idx] == ' ')
		str[c++] = lineStr[idx++];
	str[c] = '\0';

	// Draw the operand
	fl_draw(str, curx, y);
	curx += (int) fl_width(str);

	// Check if we have trace data or if this is a look-ahead instruction
	if (lineStr[idx] == '\0')
	{
		// Back to the default color
		fl_color(pColors->foreground);
		return;
	}

	// Draw the A register
	fl_color(pColors->reg);
	str[0] = lineStr[idx++];
	str[1] = '\0';
	fl_draw(str, curx, y);
	curx += char_width;
	fl_color(pColors->foreground);
	str[0] = lineStr[idx++];
	fl_draw(str, curx, y);
	curx += char_width;

	// Draw the A register value
	fl_color(pColors->number);
	strncpy(str, &lineStr[idx], 2);
	str[2] = '\0';
	fl_draw(str, curx, y);
	curx += char_width * 2;
	idx += 2;

	// Draw the flags
	fl_color(pColors->flags);
	strncpy(str, &lineStr[idx], 11);
	str[11] = '\0';
	fl_draw(str, curx, y);
	curx += (int) fl_width(str);
	idx += 11;

	// Now print the registers
	while (lineStr[idx] != '\0')
	{
		// Copy next register to our temp string
		c = 0;
		while (lineStr[idx] != ':')
			str[c++] = lineStr[idx++];
		str[c] = '\0';
		fl_color(pColors->reg);
		fl_draw(str, curx, y);
		curx += (int) fl_width(str);

		// Copy and draw the ':' and spaces
		fl_color(pColors->foreground);
		fl_draw(":", curx, y);
		curx += char_width;
		idx++;

		// Copy the register value
		c = 0;
		str[c++] = lineStr[idx++];
		while (lineStr[idx] != ' ' && lineStr[idx] != '\0')
			str[c++] = lineStr[idx++];
		// Copy the spaces too
		while (lineStr[idx] == ' ')
			str[c++] = lineStr[idx++];
		str[c] = '\0';
		fl_color(pColors->foreground);
		fl_draw(str, curx, y);
		curx += (int) fl_width(str);
	}

	// Back to the default color
	fl_color(pColors->foreground);
}

/*
============================================================================
Callback routine to redraw the trace and stack areas
============================================================================
*/
int build_trace_line(int line, char* lineStr)
{
	char				str[100], flags[20];
	int					y, len, last_pc;
	cpu_trace_data_t*	pTrace = &gcpuw->m_pTraceData[line];

	// Disassemble the opcode
	last_pc = pTrace->pc + cpu_dis.DisassembleLine(pTrace->pc, (unsigned char) 
		pTrace->opcode & 0xFF, pTrace->operand, lineStr);

	// Replace 'H' in address with execution location ('s'ystem, 'o'pt rom, 'r'am)
	if (pTrace->pc > ROMSIZE)
		lineStr[4] = 'r';
	else if ((pTrace->opcode >> 8) == 0)
		lineStr[4] = 's';
	else
		lineStr[4] = 'o';

	// Append spaces after opcode
	len = 20 - strlen(lineStr);
	for (y = 0; y < len; y++)
		strcat(lineStr, " ");

	// Format flags string
	unsigned char fl = pTrace->af & 0xFF;
	sprintf(flags, "%c%c%c%c%c%c%c%c", fl & SF_BIT ?'S':' ', fl & ZF_BIT ?'Z':' ', fl & TS_BIT ?'T':' ',
		fl & AC_BIT ? 'A':' ', fl & PF_BIT ? 'P':' ', fl & OV_BIT ? 'O':' ', fl & XF_BIT ? 'X':' ', 
		fl & CF_BIT ? 'C':' ');

	// Append regs after opcode
	if (gcpuw->m_showAs16Bit)
		sprintf(str, "A:%02X %s  BC:%04X  DE:%04X  HL:%04X  SP:%02X",
			pTrace->af >> 8, flags, pTrace->bc, pTrace->de, pTrace->hl, pTrace->sp);
	else
		sprintf(str, "A:%02X %s  B:%02X C:%02X D:%02X E:%02X H:%02X L:%02X SP:%02X",
			pTrace->af >> 8, flags, pTrace->bc >> 8, pTrace->bc & 0xFF, pTrace->de >> 8, 
			pTrace->de & 0xFF, pTrace->hl >> 8, pTrace->de & 0xFF, pTrace->sp);
	strcat(lineStr, str);

	return last_pc;
}

/*
============================================================================
Callback routine to redraw the trace and stack areas
============================================================================
*/
void redraw_trace(Fl_Widget* w, void* pOpaque)
{
	int		x, draw, lines, last_pc, last_sp;
	char	lineStr[120], str[100];
	int		trace_top = gcpuw->m_pTraceBox->y() + gcpuw->m_fontHeight - 4;
	int		lookahead = 0, bold;
	int		selStart, selEnd;

	// Clear rectangle
	fl_color(gcpuw->m_traceColors.background);
	fl_rectf(gcpuw->m_pTraceBox->x()+3, gcpuw->m_pTraceBox->y()+3,
		gcpuw->m_pTraceBox->w()-6, gcpuw->m_pTraceBox->h()-6);
	fl_rectf(gcpuw->m_pStackBox->x()+3, gcpuw->m_pStackBox->y()+3,
		gcpuw->m_pStackBox->w()-6, gcpuw->m_pStackBox->h()-6);

	lines = gcpuw->m_pTraceBox->h() / gcpuw->m_fontHeight;

	// Select 11 point Courier font
	if (gcpuw->m_inverseHilight)
		fl_font(FL_COURIER_BOLD, gcpuw->m_fontSize);
	else
		fl_font(FL_COURIER, gcpuw->m_fontSize);
	fl_color(gcpuw->m_traceColors.foreground);

	// Initialize last_sp and last_pc in case trace is clear
	last_sp = SP;
	last_pc = PC;

	// Draw either real-time (sampled every so often) data if stopped
	// or actual address-by-address data if we are stopped
	if (!gStopped)
	{
		// Determine first trace to draw
		draw = gcpuw->m_iInstTraceHead - lines;
		if (draw < 0)
			draw += 64;
		for (x = 0; x < lines; x++)
		{
			// Draw the Stack
			sprintf(str, "%04X", get_memory16(SP+x*2));
			fl_draw(str, gcpuw->m_pStackBox->x()+6, trace_top+x*gcpuw->m_fontHeight);
			if (gcpuw->m_colorSyntaxHilight)
				fl_color(gcpuw->m_traceColors.number);
			fl_draw("H", gcpuw->m_pStackBox->x()+6+4*(int) fl_width(" "), trace_top+x*gcpuw->m_fontHeight);
			fl_color(gcpuw->m_traceColors.foreground);

			fl_push_clip(gcpuw->m_pTraceBox->x(), gcpuw->m_pTraceBox->y(),
				gcpuw->m_pTraceBox->w()-3, gcpuw->m_pTraceBox->h());
			if (gcpuw->m_colorSyntaxHilight)
			{
				if (gcpuw->m_sInstTrace[draw][0] != '\0')
					draw_color_syntax(gcpuw->m_sInstTrace[draw], 25, trace_top+x*gcpuw->m_fontHeight);
			}
			else
				fl_draw(gcpuw->m_sInstTrace[draw], 25, trace_top+x*gcpuw->m_fontHeight);
			fl_pop_clip();
			if (++draw >= 64)
				draw = 0;
		}
	}
	else
	{
		// Draw actual trace information
		int firstLine = gcpuw->m_pScroll->value();
		if (firstLine < 0)
			firstLine = 0;
		int draw = gcpuw->m_iTraceHead + firstLine;
		if (draw >= gcpuw->m_traceAvail)
			draw -= gcpuw->m_traceAvail;
		last_sp = SP;
		selStart = gcpuw->m_selStart;
		selEnd = gcpuw->m_selEnd;
		if (selStart > selEnd)
		{
			selStart = gcpuw->m_selEnd;
			selEnd = gcpuw->m_selStart;
		}

		for (x = 0; x < lines; x++)
		{
			// Draw the Stack
			sprintf(str, "%04X", get_memory16(SP+x*2));
			fl_draw(str, gcpuw->m_pStackBox->x()+6, trace_top+x*gcpuw->m_fontHeight);

			if (gcpuw->m_colorSyntaxHilight)
				fl_color(gcpuw->m_traceColors.number);
			fl_draw("H", gcpuw->m_pStackBox->x()+6+4*(int) fl_width(" "), trace_top+x*gcpuw->m_fontHeight);
			fl_color(gcpuw->m_traceColors.foreground);

			// Now draw the Instruction Trace
			if (x + firstLine < gcpuw->m_traceAvail)
			{
				cpu_trace_data_t*	pTrace = &gcpuw->m_pTraceData[draw];
				unsigned char fl = pTrace->af & 0xFF;

				/* Build the trace text */
				last_pc = build_trace_line(draw, &lineStr[0]);

				// Clip the drawing
				fl_push_clip(gcpuw->m_pTraceBox->x()+2, gcpuw->m_pTraceBox->y()+3,
					gcpuw->m_pTraceBox->w()-4, gcpuw->m_pTraceBox->h()-6);

				// Test if drawing a selected item
				if (x+firstLine >= selStart && x+firstLine <= selEnd)
				{
					// Draw the selection box
					fl_color(gcpuw->m_traceColors.select);
					fl_rectf(gcpuw->m_pTraceBox->x()+3, trace_top+(x-1)*gcpuw->m_fontHeight+5,
						gcpuw->m_pTraceBox->w()-6, gcpuw->m_fontHeight);
					fl_color(FL_WHITE);
				}

				/* If on the execution line, make the font bold */
				else if (x+firstLine == gcpuw->m_traceAvail-1)
				{
					bold = TRUE;
					if (gcpuw->m_inverseHilight)
					{
						fl_color(gcpuw->m_traceColors.hilight);
						fl_rectf(gcpuw->m_pTraceBox->x()+3, trace_top+(x-1)*gcpuw->m_fontHeight+5,
							gcpuw->m_pTraceBox->w()-6, gcpuw->m_fontHeight);
						fl_color(FL_WHITE);
					}
					else
					{
						fl_font(FL_COURIER_BOLD, gcpuw->m_fontSize);
					}
				}
				else
					bold = FALSE;

				// Finally, draw the trace on the window
				if (gcpuw->m_colorSyntaxHilight)
					draw_color_syntax(lineStr, 25, trace_top+x*gcpuw->m_fontHeight, bold);
				else
					fl_draw(lineStr, 25, trace_top+x*gcpuw->m_fontHeight);
				fl_pop_clip();
				if (++draw >= gcpuw->m_traceAvail)
					draw = 0;

				// If we just drew a selected line, then change back to regular font color
				if (x+firstLine >= selStart && x+firstLine <= selEnd)
					fl_color(gcpuw->m_traceColors.foreground);

				// If we just drew the execution line, do special processing
				else if (x+firstLine == gcpuw->m_traceAvail-1)
				{
					// Change back to normal font
					if (gcpuw->m_inverseHilight)
						fl_color(gcpuw->m_traceColors.foreground);
					else
						fl_font(FL_COURIER, gcpuw->m_fontSize);

					// Search opcode for conditional branching
					if (((strncmp(&lineStr[7], "JZ", 2) == 0) ||
						(strncmp(&lineStr[7], "CZ", 2) == 0)) &&
						(fl & ZF_BIT))
							last_pc = str_to_i(&lineStr[10]);
					else if (((strncmp(&lineStr[7], "JNZ", 3) == 0) ||
						(strncmp(&lineStr[7], "CNZ", 3) == 0)) &&
						!(fl & ZF_BIT))
							last_pc = str_to_i(&lineStr[11]);
					else if (((strncmp(&lineStr[7], "JC", 2) == 0) ||
						(strncmp(&lineStr[7], "CC", 2) == 0)) &&
						(fl & CF_BIT))
							last_pc = str_to_i(&lineStr[10]);
					else if (((strncmp(&lineStr[7], "JNC", 3) == 0) ||
						(strncmp(&lineStr[7], "CNC", 3) == 0)) &&
						!(fl & CF_BIT))
							last_pc = str_to_i(&lineStr[11]);
					else if (((strncmp(&lineStr[7], "JP", 2) == 0) ||
						(strncmp(&lineStr[7], "CP", 2) == 0)) &&
						(fl & SF_BIT))
							last_pc = str_to_i(&lineStr[10]);
					else if (((strncmp(&lineStr[7], "JM ", 3) == 0) ||
						(strncmp(&lineStr[7], "CM ", 3) == 0)) &&
						!(fl & SF_BIT))
							last_pc = str_to_i(&lineStr[10]);
					else if (((strncmp(&lineStr[7], "JPE", 3) == 0) ||
						(strncmp(&lineStr[7], "CPE", 3) == 0)) &&
						(fl & PF_BIT))
							last_pc = str_to_i(&lineStr[11]);
					else if (((strncmp(&lineStr[7], "JPO", 3) == 0) ||
						(strncmp(&lineStr[7], "CPO", 3) == 0)) &&
						!(fl & PF_BIT))
							last_pc = str_to_i(&lineStr[11]);

					// Conditional RETurns
					else if ((strncmp(&lineStr[7], "RC", 2) == 0) && (fl & CF_BIT))
						last_pc = get_memory16(pTrace->sp);
					else if ((strncmp(&lineStr[7], "RNC", 3) == 0) && !(fl & CF_BIT))
						last_pc = get_memory16(pTrace->sp);
					else if ((strncmp(&lineStr[7], "RZ", 2) == 0) && (fl & ZF_BIT))
						last_pc = get_memory16(pTrace->sp);
					else if ((strncmp(&lineStr[7], "RNZ", 3) == 0) && !(fl & ZF_BIT))
						last_pc = get_memory16(pTrace->sp);
					else if ((strncmp(&lineStr[7], "RP", 2) == 0) && (fl & SF_BIT))
						last_pc = get_memory16(pTrace->sp);
					else if ((strncmp(&lineStr[7], "RM", 2) == 0) && !(fl & SF_BIT))
						last_pc = get_memory16(pTrace->sp);
					else if ((strncmp(&lineStr[7], "RPE", 3) == 0) && (fl & PF_BIT))
						last_pc = get_memory16(pTrace->sp);
					else if ((strncmp(&lineStr[7], "RPO", 3) == 0) && !(fl & PF_BIT))
						last_pc = get_memory16(pTrace->sp);

					else if (strncmp(&lineStr[7], "PCHL", 4) == 0)
						last_pc = HL;
				}
			}
			else
			{
				// Check opcode for various changes to the PC
				if (strncmp(&lineStr[7], "CALL", 4) == 0)
					last_pc = str_to_i(&lineStr[12]);
				else if (strncmp(&lineStr[7], "JMP", 3) == 0)
					last_pc = str_to_i(&lineStr[11]);
				else if (strncmp(&lineStr[7], "POP", 3) == 0)
					last_sp += 2;
				else if (strncmp(&lineStr[7], "RET", 3) == 0)
				{
					last_pc = get_memory16(last_sp);
					last_sp += 2;
				}
				else if (strncmp(&lineStr[7], "RST", 3) == 0)
				{
					last_sp -= 2;
					last_pc = str_to_i(&lineStr[11]) * 8;
				}

				// If trace is empty, then make 1st (current) inst bold
				if (x == 0)
				{
					if (gcpuw->m_inverseHilight)
						fl_color(FL_DARK_BLUE);
					else
						fl_font(FL_COURIER_BOLD, gcpuw->m_fontSize);
				}

				// Disassemble the next line that hasn't executed yet
				last_pc += cpu_dis.DisassembleLine(last_pc, lineStr);
				if (lookahead++ < 4)
				{
					// Replace 'H' in address with execution location ('s'ystem, 'o'pt rom, 'r'am)
					if (last_pc > ROMSIZE)
						lineStr[4] = 'r';
					else if (get_rom_bank() == 0)
						lineStr[4] = 's';
					else
						lineStr[4] = 'o';

					if (gcpuw->m_colorSyntaxHilight)
						draw_color_syntax(lineStr, 25, trace_top+x*gcpuw->m_fontHeight);
					else
						fl_draw(lineStr, 25, trace_top+x*gcpuw->m_fontHeight);
				}

				if (x == 0)
				{
					if (gcpuw->m_inverseHilight)
						fl_color(gcpuw->m_traceColors.foreground);
					else
						fl_font(FL_COURIER, gcpuw->m_fontSize);
				}
			}
		}
	}
}

/*
============================================================================
Callback for the trace scroll bar
============================================================================
*/
void cb_scrollbar (Fl_Widget* w, void*)
{
	int size = (int) (gcpuw->m_pScroll->h() / gcpuw->m_fontHeight);
	int max_size = gcpuw->m_traceAvail-size+4;

	// Update the max size of the scrollbar
	if (gcpuw->m_traceAvail < gcpuw->m_traceCount)
		max_size++;
	if (max_size < 0)
		max_size = 4;
	gcpuw->m_pScroll->maximum(max_size);
	gcpuw->m_pScroll->linesize(1);
	if (gcpuw->m_traceAvail > 0)
	{
		gcpuw->m_pScroll->step(size / gcpuw->m_traceAvail);
		gcpuw->m_pScroll->slider_size((double) size/ (double) gcpuw->m_traceAvail);
	}
	else
		gcpuw->m_pScroll->slider_size(1.0);

	gcpuw->redraw();
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
	if (gReMem & !gRex)
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
void VTCpuRegs::activate_controls(void)
{
	// Change activation state of processor controls
	m_pStop->deactivate();
	m_pStep->activate();
	m_pStepOver->activate();
	m_pRun->activate();
	m_pScroll->activate();

	// Activate the edit fields to allow register updates
	m_pRegA->activate();
	m_pRegB->activate();
	m_pRegC->activate();
	m_pRegD->activate();
	m_pRegE->activate();
	m_pRegH->activate();
	m_pRegL->activate();
	m_pRegPC->activate();
	m_pRegSP->activate();
	m_pRegBC->activate();
	m_pRegDE->activate();
	m_pRegHL->activate();
	m_pRegM->activate();

	// Activate Flags
	m_pSFlag->activate();
	m_pZFlag->activate();
	m_pCFlag->activate();
	m_pTSFlag->activate();
	m_pACFlag->activate();
	m_pPFlag->activate();
	m_pOVFlag->activate();
	m_pXFlag->activate();

	// Activate Breakpoints
	m_pBreak1->activate();
	m_pBreak2->activate();
	m_pBreak3->activate();
	m_pBreak4->activate();
	m_pBreakDisable1->activate();
	m_pBreakDisable2->activate();
	m_pBreakDisable3->activate();
	m_pBreakDisable4->activate();
}

void clear_trace(void)
{
	int 	x;

	for (x = 0; x < 8; x++)
		gcpuw->m_sInstTrace[x][0] = 0;
}

void inline trace_instruction(void)
{
	if (gcpuw->m_pTraceData != NULL)
	{
		int idx = gcpuw->m_iTraceHead++;
		if (gcpuw->m_iTraceHead >= gcpuw->m_traceCount)
			gcpuw->m_iTraceHead = 0;
		gcpuw->m_pTraceData[idx].pc = PC;
		gcpuw->m_pTraceData[idx].sp = SP;
		gcpuw->m_pTraceData[idx].hl = HL;
		gcpuw->m_pTraceData[idx].de = DE;
		gcpuw->m_pTraceData[idx].bc = BC;
		gcpuw->m_pTraceData[idx].af = AF;
		gcpuw->m_pTraceData[idx].opcode = get_memory8(PC) | (get_rom_bank() << 8);
		gcpuw->m_pTraceData[idx].operand = get_memory16(PC+1);

		// Update trace selections
		if (gcpuw->m_selStart >= 0)
			gcpuw->m_selStart--;
		if (gcpuw->m_selEnd >= 0)
		{
			// Decrement selEnd and test if the range needs to update
			gcpuw->m_selEnd--;
			if (gcpuw->m_selEnd >= 0 && gcpuw->m_selStart == -1)
				gcpuw->m_selStart = 0;
		}
	}

	/* Increment the available trace count until it reaches the max */
	if (gcpuw->m_traceAvail < gcpuw->m_traceCount)
		gcpuw->m_traceAvail++;
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

	if (reason == DEBUG_CPU_STEP)
	{
		if (gmew != NULL)
			memory_monitor_cb();
		return;
	}

	// Check for breakpoint
	if (!gStopped)
	{
		for (x = 0; x < 5; x++)
		{
			if ((PC == gcpuw->m_breakAddr[x]) && (gcpuw->m_breakEnable[x]))
			{
				gcpuw->activate_controls();
				gStopped = 1;

				// Clear 'step over' breakpoint
				if (x == 4)
					gcpuw->m_breakAddr[x] = -1;
				else
					clear_trace();

				// Scroll to bottom of window if needed
				if (gcpuw->m_autoScroll)
					gcpuw->ScrollToBottom();

				// Ensure the memory window is refreshed if it's open
				if (gmew != NULL)
					memory_monitor_cb();
			}
		}
	}

	/* Trace this instruction */
	trace_instruction();

	if (!gStopped)
	{
		if (gcpuw->m_breakAddr[4] != -1)
			return;
		if (++gDebugCount < gDebugMonitorFreq)
			return;
	}

	if (gDebugCount >= gDebugMonitorFreq + 64)
		gDebugCount = 0;

	// Update PC edit box
	sprintf(str, gcpuw->m_sPCfmt, PC);
	gcpuw->m_pRegPC->value(str);

	// Update SP edit box
	sprintf(str, gcpuw->m_sSPfmt, SP);
	gcpuw->m_pRegSP->value(str);

	// Update A edit box
	sprintf(str, gcpuw->m_sAfmt, A);
	gcpuw->m_pRegA->value(str);

	// Update B edit box
	sprintf(str, gcpuw->m_sBfmt, B);
	gcpuw->m_pRegB->value(str);

	// Update C edit box
	sprintf(str, gcpuw->m_sCfmt, C);
	gcpuw->m_pRegC->value(str);

	// Update D edit box
	sprintf(str, gcpuw->m_sDfmt, D);
	gcpuw->m_pRegD->value(str);

	// Update E edit box
	sprintf(str, gcpuw->m_sEfmt, E);
	gcpuw->m_pRegE->value(str);

	// Update H edit box
	sprintf(str, gcpuw->m_sHfmt, H);
	gcpuw->m_pRegH->value(str);

	// Update L edit box
	sprintf(str, gcpuw->m_sLfmt, L);
	gcpuw->m_pRegL->value(str);

	// Update BC edit box
	sprintf(str, gcpuw->m_sBCfmt, BC);
	gcpuw->m_pRegBC->value(str);

	// Update DE edit box
	sprintf(str, gcpuw->m_sDEfmt, DE);
	gcpuw->m_pRegDE->value(str);

	// Update HL edit box
	sprintf(str, gcpuw->m_sHLfmt, HL);
	gcpuw->m_pRegHL->value(str);

	// Update M edit box
	sprintf(str, gcpuw->m_sMfmt, get_m());
	gcpuw->m_pRegM->value(str);

	// Update flags
	gcpuw->m_pSFlag->value(SF);
	gcpuw->m_pZFlag->value(ZF);
	gcpuw->m_pTSFlag->value(TS);
	gcpuw->m_pACFlag->value(AC);
	gcpuw->m_pPFlag->value(PF);
	gcpuw->m_pOVFlag->value(OV);
	gcpuw->m_pXFlag->value(XF);
	gcpuw->m_pCFlag->value(CF);

	// Update RAM bank
	if (gModel == MODEL_T200 || gModel == MODEL_PC8201)
	{
		int bank = get_ram_bank();
		switch (bank)
		{
		case 0:
			gcpuw->m_pRamBank->label("RAM Bank 1");
			break;
		case 1:
			gcpuw->m_pRamBank->label("RAM Bank 2");
			break;
		case 2:
			gcpuw->m_pRamBank->label("RAM Bank 3");
			break;
		}
	}
	else
		gcpuw->m_pRamBank->label("");

	if (!gDisableRealtimeTrace || (gStopCountdown != 0) || gStopped)
	{
		// Disassemble 1 instruction
		cpu_dis.DisassembleLine(PC, gcpuw->m_sInstTrace[gcpuw->m_iInstTraceHead]);
		if (PC > ROMSIZE)
			gcpuw->m_sInstTrace[gcpuw->m_iInstTraceHead][4] = 'r';
		else if (get_rom_bank() == 0)
			gcpuw->m_sInstTrace[gcpuw->m_iInstTraceHead][4] = 's';
		else
			gcpuw->m_sInstTrace[gcpuw->m_iInstTraceHead][4] = 'o';

		// Append spaces after opcode
		len = 20 - strlen(gcpuw->m_sInstTrace[gcpuw->m_iInstTraceHead]);
		for (x = 0; x < len; x++)
			strcat(gcpuw->m_sInstTrace[gcpuw->m_iInstTraceHead], " ");

		// Format flags string
		sprintf(flags, "%c%c%c%c%c%c%c%c", SF?'S':' ', ZF?'Z':' ', TS?'T':' ',AC?'A':' ',
			PF?'P':' ',OV?'O':' ',XF?'X':' ', CF?'C':' ');

		// Append regs after opcode
		if (gcpuw->m_showAs16Bit)
			sprintf(str, "A:%02X %s  BC:%04X  DE:%04X  HL:%04X  SP:%02X",
				A, flags, BC, DE, HL, SP);
		else		
			sprintf(str, "A:%02X %s  B:%02X C:%02X D:%02X E:%02X H:%02X L:%02X SP:%02X",
				A, flags, B, C, D, E, H, L, SP);
		strcat(gcpuw->m_sInstTrace[gcpuw->m_iInstTraceHead++], str);

		if (gcpuw->m_iInstTraceHead >= 64)
			gcpuw->m_iInstTraceHead= 0;

		// Update Trace data on window
		gcpuw->redraw();

		// Update monitor frequency based on cpu speed
		if (fullspeed == 0)
			gDebugMonitorFreq = 8192;
		else if (fullspeed == 3)
			gDebugMonitorFreq = 65536;
		else
			gDebugMonitorFreq = 32768;
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

				// Scroll to bottom of window if needed
				if (gcpuw->m_autoScroll)
					gcpuw->ScrollToBottom();

				// Redraw the trace window
				gcpuw->redraw();
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
void VTCpuRegs::deactivate_controls(void)
{
	// Change activation state of processor controls
	m_pStop->activate();
	m_pStep->deactivate();
	m_pStepOver->deactivate();
	m_pRun->deactivate();
	m_pScroll->deactivate();

	// Deactivate the edit fields to allow register updates
	m_pRegA->deactivate();
	m_pRegB->deactivate();
	m_pRegC->deactivate();
	m_pRegD->deactivate();
	m_pRegE->deactivate();
	m_pRegH->deactivate();
	m_pRegL->deactivate();
	m_pRegPC->deactivate();
	m_pRegSP->deactivate();
	m_pRegBC->deactivate();
	m_pRegDE->deactivate();
	m_pRegHL->deactivate();
	m_pRegM->deactivate();

	// Deactivate Flags
	m_pSFlag->deactivate();
	m_pZFlag->deactivate();
	m_pCFlag->deactivate();
	m_pTSFlag->deactivate();
	m_pACFlag->deactivate();
	m_pPFlag->deactivate();
	m_pOVFlag->deactivate();
	m_pXFlag->deactivate();

	// Deactivate breakpoints
	m_pBreak1->deactivate();
	m_pBreak2->deactivate();
	m_pBreak3->deactivate();
	m_pBreak4->deactivate();
	m_pBreakDisable1->deactivate();
	m_pBreakDisable2->deactivate();
	m_pBreakDisable3->deactivate();
	m_pBreakDisable4->deactivate();
}

/*
============================================================================
Routine to handle 'Step Over' button
============================================================================
*/
void cb_debug_step_over(Fl_Widget* w, void* pOpaque)
{
	VTCpuRegs*	pCpuRegs = (VTCpuRegs *) pOpaque;
	int			inst;

	// Get Register updates
	pCpuRegs->get_reg_edits();

	// Determine the 'step over' breakpoint location
	inst = get_memory8(PC);
	if (((inst & 0xC7) == 0xC4) || (inst == 0xCD)) 
	{
		pCpuRegs->m_breakAddr[4] = PC+3;
	}
	else if ((inst & 0xC7) == 0xC7)
	{
		if ((inst == 0xCF) || (inst == 0xFF))
			pCpuRegs->m_breakAddr[4] = PC+2;
		else
			pCpuRegs->m_breakAddr[4] = PC+1;
	}
	else
	{
		// Single step the processor
		gSingleStep = 1;

		// If auto scroll is enabled, then scroll to th bottom
		if (pCpuRegs->m_autoScroll)
			pCpuRegs->ScrollToBottom();

		return;
	}

	// If auto scroll is enabled, then scroll to th bottom
	if (pCpuRegs->m_autoScroll)
		pCpuRegs->ScrollToBottom();

	// Start the processor
	gStopped = 0;
}

/*
============================================================================
Routine to handle Stop button
============================================================================
*/
void cb_debug_stop(Fl_Widget* w, void* pOpaque)
{
	VTCpuRegs*	pCpuRegs = (VTCpuRegs *) pOpaque;

	// Activate the controls
	pCpuRegs->activate_controls();

	// Update the scrollbar
	cb_scrollbar(pCpuRegs->m_pScroll, pOpaque);

	// If auto scroll is enabled, then scroll to the bottom
	if (pCpuRegs->m_autoScroll)
		pCpuRegs->ScrollToBottom();

	// Stop the processor
	gStopCountdown = 1;
	gSaveFreq = gDebugMonitorFreq;
	gDebugMonitorFreq = 1;
}

/*
============================================================================
Routine to handle Step button
============================================================================
*/
void cb_debug_step(Fl_Widget* w, void* pOpaque)
{
	VTCpuRegs*	pCpuRegs = (VTCpuRegs *) pOpaque;

	// Get Register updates
	pCpuRegs->get_reg_edits();

	// If auto scroll is enabled, then scroll to th bottom
	if (pCpuRegs->m_autoScroll)
		pCpuRegs->ScrollToBottom();

	// Single step the processor
	gSingleStep = 1;
}

/*
============================================================================
Routine to handle Run button
============================================================================
*/
void cb_debug_run(Fl_Widget* w, void* pOpaque)
{
	VTCpuRegs*	pCpuRegs = (VTCpuRegs *) pOpaque;

	// Deactivate the debug controls
	pCpuRegs->deactivate_controls();

	// Get Register updates
	pCpuRegs->get_reg_edits();

	// Start the processor
	gStopped = 0;

	// Redraw the trace in case real-time trace is off
	gcpuw->redraw();
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
		cb_debug_stop(NULL, gcpuw);
		return 1;
	}
	return 0;
}
int remote_cpureg_run(void)
{
	if (gcpuw != NULL)
	{
		cb_debug_run(NULL, gcpuw);
		return 1;
	}
	return 0;
}
int remote_cpureg_step(void)
{
	if (gcpuw != NULL)
	{
		cb_debug_step(NULL, gcpuw);
		return 1;
	}
	return 0;
}

/*
============================================================================
Routine to handle Change to PC edit field
============================================================================
*/
void cb_reg_pc_changed(Fl_Widget* w, void* pOpaque)
{
	int				new_pc;
	const char		*pStr;
	int				trace_tail;
	char			str[120], flags[20];
	int				len, x;
	VTCpuRegs*		pCpuRegs = (VTCpuRegs *) pOpaque;

	pStr = pCpuRegs->m_pRegPC->value();
	new_pc = str_to_i(pStr);
	trace_tail = pCpuRegs->m_iInstTraceHead - 1;
	if (trace_tail < 0)
		trace_tail = 7;

	// Disassemble 1 instruction
	cpu_dis.DisassembleLine(new_pc, pCpuRegs->m_sInstTrace[trace_tail]);

	// Append spaces after opcode
	len = 20 - strlen(pCpuRegs->m_sInstTrace[trace_tail]);
	for (x = 0; x < len; x++)
		strcat(pCpuRegs->m_sInstTrace[trace_tail], " ");

	// Format flags string
	sprintf(flags, "%c%c%c%c%c%c%c%c", SF?'S':' ', ZF?'Z':' ', TS?'T':' ',AC?'A':' ',
		PF?'P':' ',OV?'O':' ',XF?'X':' ', CF?'C':' ');

	// Append regs after opcode
	sprintf(str, "A:%02X %s B:%02X C:%02X D:%02X E:%02X H:%02X L:%02X SP:%02X",
		A, flags, B, C, D, E, H, L, SP);
	strcat(pCpuRegs->m_sInstTrace[trace_tail], str);

	// Update Trace data on window
	gcpuw->redraw();
}

/*
============================================================================
Routine to handle Change to BC edit field
============================================================================
*/
void cb_reg_bc_changed(Fl_Widget* w, void* pOpaque)
{
	int				new_bc;
	const char		*pStr;
	char			str[20];
	VTCpuRegs*		pCpuRegs = (VTCpuRegs *) pOpaque;

	pStr = pCpuRegs->m_pRegBC->value();
	new_bc = str_to_i(pStr);

	B = (new_bc >> 8) & 0xFF;
	C = new_bc & 0xFF;

	// Update B edit box
	sprintf(str, pCpuRegs->m_sBfmt, B);
	pCpuRegs->m_pRegB->value(str);

	// Update C edit box
	sprintf(str, pCpuRegs->m_sCfmt, C);
	pCpuRegs->m_pRegC->value(str);
}

/*
============================================================================
Routine to handle Change to B edit field
============================================================================
*/
void cb_reg_b_changed(Fl_Widget* w, void* pOpaque)
{
	const char		*pStr;
	char			str[20];
	VTCpuRegs*		pCpuRegs = (VTCpuRegs *) pOpaque;

	pStr = pCpuRegs->m_pRegB->value();
	B = str_to_i(pStr);

	// Update B edit box
	sprintf(str, pCpuRegs->m_sBfmt, B);
	pCpuRegs->m_pRegB->value(str);

	// Update BC edit box
	sprintf(str, pCpuRegs->m_sBCfmt, BC);
	pCpuRegs->m_pRegBC->value(str);
}

/*
============================================================================
Routine to handle Change to C edit field
============================================================================
*/
void cb_reg_c_changed(Fl_Widget* w, void* pOpaque)
{
	const char		*pStr;
	char			str[20];
	VTCpuRegs*		pCpuRegs = (VTCpuRegs *) pOpaque;

	pStr = pCpuRegs->m_pRegC->value();
	C = str_to_i(pStr);

	// Update C edit box
	sprintf(str, pCpuRegs->m_sCfmt, C);
	pCpuRegs->m_pRegC->value(str);

	// Update BC edit box
	sprintf(str, pCpuRegs->m_sBCfmt, BC);
	pCpuRegs->m_pRegBC->value(str);
}

/*
============================================================================
Routine to handle Change to DE edit field
============================================================================
*/
void cb_reg_de_changed(Fl_Widget* w, void* pOpaque)
{
	int				new_de;
	const char		*pStr;
	char			str[20];
	VTCpuRegs*		pCpuRegs = (VTCpuRegs *) pOpaque;

	pStr = pCpuRegs->m_pRegDE->value();
	new_de = str_to_i(pStr);

	D = (new_de >> 8) & 0xFF;
	E = new_de & 0xFF;

	// Update D edit box
	sprintf(str, pCpuRegs->m_sDfmt, D);
	pCpuRegs->m_pRegD->value(str);

	// Update E edit box
	sprintf(str, pCpuRegs->m_sEfmt, E);
	pCpuRegs->m_pRegE->value(str);
}

/*
============================================================================
Routine to handle Change to D edit field
============================================================================
*/
void cb_reg_d_changed(Fl_Widget* w, void* pOpaque)
{
	const char		*pStr;
	char			str[20];
	VTCpuRegs*		pCpuRegs = (VTCpuRegs *) pOpaque;

	pStr = pCpuRegs->m_pRegD->value();
	D = str_to_i(pStr);

	// Update B edit box
	sprintf(str, pCpuRegs->m_sDfmt, D);
	pCpuRegs->m_pRegD->value(str);

	// Update BC edit box
	sprintf(str, pCpuRegs->m_sDEfmt, DE);
	pCpuRegs->m_pRegDE->value(str);
}

/*
============================================================================
Routine to handle Change to E edit field
============================================================================
*/
void cb_reg_e_changed(Fl_Widget* w, void* pOpaque)
{
	const char		*pStr;
	char			str[20];
	VTCpuRegs*		pCpuRegs = (VTCpuRegs *) pOpaque;

	pStr = pCpuRegs->m_pRegE->value();
	E = str_to_i(pStr);

	// Update C edit box
	sprintf(str, pCpuRegs->m_sEfmt, E);
	pCpuRegs->m_pRegE->value(str);

	// Update DE edit box
	sprintf(str, pCpuRegs->m_sDEfmt, DE);
	pCpuRegs->m_pRegDE->value(str);
}

/*
============================================================================
Routine to handle Change to HL edit field
============================================================================
*/
void cb_reg_hl_changed(Fl_Widget* w, void* pOpaque)
{
	int				new_hl;
	const char		*pStr;
	char			str[20];
	VTCpuRegs*		pCpuRegs = (VTCpuRegs *) pOpaque;

	pStr = pCpuRegs->m_pRegHL->value();
	new_hl = str_to_i(pStr);

	H = (new_hl >> 8) & 0xFF;
	L = new_hl & 0xFF;

	// Update H edit box
	sprintf(str, pCpuRegs->m_sHfmt, H);
	pCpuRegs->m_pRegH->value(str);

	// Update L edit box
	sprintf(str, pCpuRegs->m_sLfmt, L);
	pCpuRegs->m_pRegL->value(str);

	// Update M edit box
	sprintf(str, pCpuRegs->m_sMfmt, get_m());
	pCpuRegs->m_pRegM->value(str);

}

/*
============================================================================
Routine to handle Change to H edit field
============================================================================
*/
void cb_reg_h_changed(Fl_Widget* w, void* pOpaque)
{
	const char		*pStr;
	char			str[20];
	VTCpuRegs*		pCpuRegs = (VTCpuRegs *) pOpaque;

	pStr = pCpuRegs->m_pRegH->value();
	H = str_to_i(pStr);

	// Update H edit box
	sprintf(str, pCpuRegs->m_sHfmt, H);
	pCpuRegs->m_pRegH->value(str);

	// Update HL edit box
	sprintf(str, pCpuRegs->m_sHLfmt, HL);
	pCpuRegs->m_pRegHL->value(str);

	// Update M edit box
	sprintf(str, pCpuRegs->m_sMfmt, get_m());
	pCpuRegs->m_pRegM->value(str);

}

/*
============================================================================
Routine to handle Change to L edit field
============================================================================
*/
void cb_reg_l_changed(Fl_Widget* w, void* pOpaque)
{
	const char		*pStr;
	char			str[20];
	VTCpuRegs*		pCpuRegs = (VTCpuRegs *) pOpaque;

	pStr = pCpuRegs->m_pRegL->value();
	L = str_to_i(pStr);

	// Update H edit box
	sprintf(str, pCpuRegs->m_sLfmt, L);
	pCpuRegs->m_pRegL->value(str);

	// Update HL edit box
	sprintf(str, pCpuRegs->m_sHLfmt, HL);
	pCpuRegs->m_pRegHL->value(str);

	// Update M edit box
	sprintf(str, pCpuRegs->m_sMfmt, get_m());
	pCpuRegs->m_pRegM->value(str);

}

/*
============================================================================
Routine to handle Change to SP edit field
============================================================================
*/
void cb_reg_sp_changed(Fl_Widget* w, void* pOpaque)
{
	const char		*pStr;
	int				val;
	VTCpuRegs*		pCpuRegs = (VTCpuRegs *) pOpaque;

	// Get value of SP
	pStr = pCpuRegs->m_pRegSP->value();
	if (pStr[0] != 0)
	{
		val = str_to_i(pStr);
		SPL = val & 0xFF; SPH = val >> 8;
	}

	// Update Trace data on window
	gcpuw->redraw();
}

/*
============================================================================
Routine to handle disabling realtime trace
============================================================================
*/
void cb_disable_realtime_trace(Fl_Widget* w, void* pOpaque)
{
	VTCpuRegs*		pCpuRegs = (VTCpuRegs *) pOpaque;

	gDisableRealtimeTrace = pCpuRegs->m_pDisableTrace->value();

	if (gDisableRealtimeTrace)
	{
		pCpuRegs->redraw();
	}
}

/*
============================================================================
Routine to handle Reg A Hex display mode
============================================================================
*/
void cb_reg_a_hex(Fl_Widget* w, void* pOpaque)
{
	char		str[8];
	VTCpuRegs*	pCpuRegs = (VTCpuRegs *) pOpaque;

	strcpy(pCpuRegs->m_sAfmt,  "0x%02X");
	sprintf(str, pCpuRegs->m_sAfmt, A);
	pCpuRegs->m_pRegA->value(str);

	pCpuRegs->m_pAllHex->value(0);
	pCpuRegs->m_pAllDec->value(0);
}

/*
============================================================================
Routine to handle Reg A Dec display mode
============================================================================
*/
void cb_reg_a_dec(Fl_Widget* w, void* pOpaque)
{
	char		str[8];
	VTCpuRegs*	pCpuRegs = (VTCpuRegs *) pOpaque;

	strcpy(pCpuRegs->m_sAfmt,  "%d");
	sprintf(str, pCpuRegs->m_sAfmt, A);
	pCpuRegs->m_pRegA->value(str);

	pCpuRegs->m_pAllHex->value(0);
	pCpuRegs->m_pAllDec->value(0);
}

/*
============================================================================
Routine to handle Reg B Hex display mode
============================================================================
*/
void cb_reg_b_hex(Fl_Widget* w, void* pOpaque)
{
	char		str[8];
	VTCpuRegs*	pCpuRegs = (VTCpuRegs *) pOpaque;

	strcpy(pCpuRegs->m_sBfmt,  "0x%02X");
	sprintf(str, pCpuRegs->m_sBfmt, B);
	pCpuRegs->m_pRegB->value(str);

	pCpuRegs->m_pAllHex->value(0);
	pCpuRegs->m_pAllDec->value(0);
}

/*
==========================================================================
Routine to handle Reg B Dec display mode
============================================================================
*/
void cb_reg_b_dec(Fl_Widget* w, void* pOpaque)
{
	char		str[8];
	VTCpuRegs*	pCpuRegs = (VTCpuRegs *) pOpaque;

	strcpy(pCpuRegs->m_sBfmt,  "%d");
	sprintf(str, pCpuRegs->m_sBfmt, B);
	pCpuRegs->m_pRegB->value(str);

	pCpuRegs->m_pAllHex->value(0);
	pCpuRegs->m_pAllDec->value(0);
}

/*
============================================================================
Routine to handle Reg C Hex display mode
============================================================================
*/
void cb_reg_c_hex(Fl_Widget* w, void* pOpaque)
{
	char		str[8];
	VTCpuRegs*	pCpuRegs = (VTCpuRegs *) pOpaque;

	strcpy(pCpuRegs->m_sCfmt,  "0x%02X");
	sprintf(str, pCpuRegs->m_sCfmt, C);
	pCpuRegs->m_pRegC->value(str);

	pCpuRegs->m_pAllHex->value(0);
	pCpuRegs->m_pAllDec->value(0);
}

/*
============================================================================
Routine to handle Reg C Dec display mode
============================================================================
*/
void cb_reg_c_dec(Fl_Widget* w, void* pOpaque)
{
	char		str[8];
	VTCpuRegs*	pCpuRegs = (VTCpuRegs *) pOpaque;

	strcpy(pCpuRegs->m_sCfmt,  "%d");
	sprintf(str, pCpuRegs->m_sCfmt, C);
	pCpuRegs->m_pRegC->value(str);

	pCpuRegs->m_pAllHex->value(0);
	pCpuRegs->m_pAllDec->value(0);
}

/*
============================================================================
Routine to handle Reg D Hex display mode
============================================================================
*/
void cb_reg_d_hex(Fl_Widget* w, void* pOpaque)
{
	char		str[8];
	VTCpuRegs*	pCpuRegs = (VTCpuRegs *) pOpaque;

	strcpy(pCpuRegs->m_sDfmt,  "0x%02X");
	sprintf(str, pCpuRegs->m_sDfmt, D);
	pCpuRegs->m_pRegD->value(str);

	pCpuRegs->m_pAllHex->value(0);
	pCpuRegs->m_pAllDec->value(0);
}

/*
============================================================================
Routine to handle Reg D Dec display mode
============================================================================
*/
void cb_reg_d_dec(Fl_Widget* w, void* pOpaque)
{
	char		str[8];
	VTCpuRegs*	pCpuRegs = (VTCpuRegs *) pOpaque;

	strcpy(pCpuRegs->m_sDfmt,  "%d");
	sprintf(str, pCpuRegs->m_sDfmt, D);
	pCpuRegs->m_pRegD->value(str);

	pCpuRegs->m_pAllHex->value(0);
	pCpuRegs->m_pAllDec->value(0);
}

/*
============================================================================
Routine to handle Reg E Hex display mode
============================================================================
*/
void cb_reg_e_hex(Fl_Widget* w, void* pOpaque)
{
	char		str[8];
	VTCpuRegs*	pCpuRegs = (VTCpuRegs *) pOpaque;

	strcpy(pCpuRegs->m_sEfmt,  "0x%02X");
	sprintf(str, pCpuRegs->m_sEfmt, E);
	pCpuRegs->m_pRegE->value(str);

	pCpuRegs->m_pAllHex->value(0);
	pCpuRegs->m_pAllDec->value(0);
}

/*
============================================================================
Routine to handle Reg E Dec display mode
============================================================================
*/
void cb_reg_e_dec(Fl_Widget* w, void* pOpaque)
{
	char		str[8];
	VTCpuRegs*	pCpuRegs = (VTCpuRegs *) pOpaque;

	strcpy(pCpuRegs->m_sEfmt,  "%d");
	sprintf(str, pCpuRegs->m_sEfmt, E);
	pCpuRegs->m_pRegE->value(str);

	pCpuRegs->m_pAllHex->value(0);
	pCpuRegs->m_pAllDec->value(0);
}

/*
============================================================================
Routine to handle Reg H Hex display mode
============================================================================
*/
void cb_reg_h_hex(Fl_Widget* w, void* pOpaque)
{
	char		str[8];
	VTCpuRegs*	pCpuRegs = (VTCpuRegs *) pOpaque;

	strcpy(pCpuRegs->m_sHfmt,  "0x%02X");
	sprintf(str, pCpuRegs->m_sHfmt, H);
	pCpuRegs->m_pRegH->value(str);

	pCpuRegs->m_pAllHex->value(0);
	pCpuRegs->m_pAllDec->value(0);
}

/*
============================================================================
Routine to handle Reg H Dec display mode
============================================================================
*/
void cb_reg_h_dec(Fl_Widget* w, void* pOpaque)
{
	char		str[8];
	VTCpuRegs*	pCpuRegs = (VTCpuRegs *) pOpaque;

	strcpy(pCpuRegs->m_sHfmt,  "%d");
	sprintf(str, pCpuRegs->m_sHfmt, H);
	pCpuRegs->m_pRegH->value(str);

	pCpuRegs->m_pAllHex->value(0);
	pCpuRegs->m_pAllDec->value(0);
}

/*
============================================================================
Routine to handle Reg L Hex display mode
============================================================================
*/
void cb_reg_l_hex(Fl_Widget* w, void* pOpaque)
{
	char		str[8];
	VTCpuRegs*	pCpuRegs = (VTCpuRegs *) pOpaque;

	strcpy(pCpuRegs->m_sLfmt,  "0x%02X");
	sprintf(str, pCpuRegs->m_sLfmt, L);
	pCpuRegs->m_pRegL->value(str);

	pCpuRegs->m_pAllHex->value(0);
	pCpuRegs->m_pAllDec->value(0);
}

/*
============================================================================
Routine to handle Reg L Dec display mode
============================================================================
*/
void cb_reg_l_dec(Fl_Widget* w, void* pOpaque)
{
	char		str[8];
	VTCpuRegs*	pCpuRegs = (VTCpuRegs *) pOpaque;

	strcpy(pCpuRegs->m_sLfmt,  "%d");
	sprintf(str, pCpuRegs->m_sLfmt, L);
	pCpuRegs->m_pRegL->value(str);

	pCpuRegs->m_pAllHex->value(0);
	pCpuRegs->m_pAllDec->value(0);
}

/*
============================================================================
Routine to handle Reg PC Hex display mode
============================================================================
*/
void cb_reg_pc_hex(Fl_Widget* w, void* pOpaque)
{
	char		str[8];
	VTCpuRegs*	pCpuRegs = (VTCpuRegs *) pOpaque;

	strcpy(pCpuRegs->m_sPCfmt,  "0x%04X");
	sprintf(str, pCpuRegs->m_sPCfmt, PC);
	pCpuRegs->m_pRegPC->value(str);

	pCpuRegs->m_pAllHex->value(0);
	pCpuRegs->m_pAllDec->value(0);
}

/*
============================================================================
Routine to handle Reg PC Dec display mode
============================================================================
*/
void cb_reg_pc_dec(Fl_Widget* w, void* pOpaque)
{
	char		str[8];
	VTCpuRegs*	pCpuRegs = (VTCpuRegs *) pOpaque;

	strcpy(pCpuRegs->m_sPCfmt,  "%d");
	sprintf(str, pCpuRegs->m_sPCfmt, PC);
	pCpuRegs->m_pRegPC->value(str);

	pCpuRegs->m_pAllHex->value(0);
	pCpuRegs->m_pAllDec->value(0);
}

/*
============================================================================
Routine to handle Reg SP Hex display mode
============================================================================
*/
void cb_reg_sp_hex(Fl_Widget* w, void* pOpaque)
{
	char		str[8];
	VTCpuRegs*	pCpuRegs = (VTCpuRegs *) pOpaque;

	strcpy(pCpuRegs->m_sSPfmt,  "0x%04X");
	sprintf(str, pCpuRegs->m_sSPfmt, SP);
	pCpuRegs->m_pRegSP->value(str);

	pCpuRegs->m_pAllHex->value(0);
	pCpuRegs->m_pAllDec->value(0);
}

/*
============================================================================
Routine to handle Reg SP Dec display mode
============================================================================
*/
void cb_reg_sp_dec(Fl_Widget* w, void* pOpaque)
{
	char		str[8];
	VTCpuRegs*	pCpuRegs = (VTCpuRegs *) pOpaque;

	strcpy(pCpuRegs->m_sSPfmt,  "%d");
	sprintf(str, pCpuRegs->m_sSPfmt, SP);
	pCpuRegs->m_pRegSP->value(str);

	pCpuRegs->m_pAllHex->value(0);
	pCpuRegs->m_pAllDec->value(0);
}

/*
============================================================================
Routine to handle Reg BC Hex display mode
============================================================================
*/
void cb_reg_bc_hex(Fl_Widget* w, void* pOpaque)
{
	char		str[8];
	VTCpuRegs*	pCpuRegs = (VTCpuRegs *) pOpaque;

	strcpy(pCpuRegs->m_sBCfmt,  "0x%04X");
	sprintf(str, pCpuRegs->m_sBCfmt, BC);
	pCpuRegs->m_pRegBC->value(str);

	pCpuRegs->m_pAllHex->value(0);
	pCpuRegs->m_pAllDec->value(0);
}

/*
============================================================================
Routine to handle Reg BC Dec display mode
============================================================================
*/
void cb_reg_bc_dec(Fl_Widget* w, void* pOpaque)
{
	char		str[8];
	VTCpuRegs*	pCpuRegs = (VTCpuRegs *) pOpaque;

	strcpy(pCpuRegs->m_sBCfmt,  "%d");
	sprintf(str, pCpuRegs->m_sBCfmt, BC);
	pCpuRegs->m_pRegBC->value(str);

	pCpuRegs->m_pAllHex->value(0);
	pCpuRegs->m_pAllDec->value(0);
}

/*
============================================================================
Routine to handle Reg DE Hex display mode
============================================================================
*/
void cb_reg_de_hex(Fl_Widget* w, void* pOpaque)
{
	char		str[8];
	VTCpuRegs*	pCpuRegs = (VTCpuRegs *) pOpaque;

	strcpy(pCpuRegs->m_sDEfmt,  "0x%04X");
	sprintf(str, pCpuRegs->m_sDEfmt, DE);
	pCpuRegs->m_pRegDE->value(str);

	pCpuRegs->m_pAllHex->value(0);
	pCpuRegs->m_pAllDec->value(0);
}

/*
============================================================================
Routine to handle Reg DE Dec display mode
============================================================================
*/
void cb_reg_de_dec(Fl_Widget* w, void* pOpaque)
{
	char		str[8];
	VTCpuRegs*	pCpuRegs = (VTCpuRegs *) pOpaque;

	strcpy(pCpuRegs->m_sDEfmt,  "%d");
	sprintf(str, pCpuRegs->m_sDEfmt, DE);
	pCpuRegs->m_pRegDE->value(str);

	pCpuRegs->m_pAllHex->value(0);
	pCpuRegs->m_pAllDec->value(0);
}

/*
============================================================================
Routine to handle Reg HL Hex display mode
============================================================================
*/
void cb_reg_hl_hex(Fl_Widget* w, void* pOpaque)
{
	char		str[8];
	VTCpuRegs*	pCpuRegs = (VTCpuRegs *) pOpaque;

	strcpy(pCpuRegs->m_sHLfmt,  "0x%04X");
	sprintf(str, pCpuRegs->m_sHLfmt, HL);
	pCpuRegs->m_pRegHL->value(str);

	pCpuRegs->m_pAllHex->value(0);
	pCpuRegs->m_pAllDec->value(0);
}

/*
============================================================================
Routine to handle Reg HL Dec display mode
============================================================================
*/
void cb_reg_hl_dec(Fl_Widget* w, void* pOpaque)
{
	char		str[8];
	VTCpuRegs*	pCpuRegs = (VTCpuRegs *) pOpaque;

	strcpy(pCpuRegs->m_sHLfmt,  "%d");
	sprintf(str, pCpuRegs->m_sHLfmt, HL);
	pCpuRegs->m_pRegHL->value(str);

	pCpuRegs->m_pAllHex->value(0);
	pCpuRegs->m_pAllDec->value(0);
}

/*
============================================================================
Routine to handle Reg M Hex display mode
============================================================================
*/
void cb_reg_m_hex(Fl_Widget* w, void* pOpaque)
{
	char		str[8];
	VTCpuRegs*	pCpuRegs = (VTCpuRegs *) pOpaque;

	strcpy(pCpuRegs->m_sMfmt,  "0x%02X");
	sprintf(str, pCpuRegs->m_sMfmt, get_m());
	pCpuRegs->m_pRegM->value(str);

	pCpuRegs->m_pAllHex->value(0);
	pCpuRegs->m_pAllDec->value(0);
}

/*
============================================================================
Routine to handle Reg M Dec display mode
============================================================================
*/
void cb_reg_m_dec(Fl_Widget* w, void* pOpaque)
{
	char		str[8];
	VTCpuRegs*	pCpuRegs = (VTCpuRegs *) pOpaque;

	strcpy(pCpuRegs->m_sMfmt,  "%d");
	sprintf(str, pCpuRegs->m_sMfmt, get_m());
	pCpuRegs->m_pRegM->value(str);

	pCpuRegs->m_pAllHex->value(0);
	pCpuRegs->m_pAllDec->value(0);
}

/*
============================================================================
Routine to handle Reg All Hex display mode
============================================================================
*/
void cb_reg_all_hex(Fl_Widget* w, void* pOpaque)
{
	VTCpuRegs* pCpuRegs = (VTCpuRegs *) pOpaque;
	pCpuRegs->RegAllHex();
}

void VTCpuRegs::RegAllHex(void)
{
	// Change format to "Hex" for all
	cb_reg_a_hex(NULL, this);
	cb_reg_b_hex(NULL, this);
	cb_reg_c_hex(NULL, this);
	cb_reg_d_hex(NULL, this);
	cb_reg_e_hex(NULL, this);
	cb_reg_h_hex(NULL, this);
	cb_reg_l_hex(NULL, this);
	cb_reg_pc_hex(NULL, this);
	cb_reg_sp_hex(NULL, this);
	cb_reg_bc_hex(NULL, this);
	cb_reg_de_hex(NULL, this);
	cb_reg_hl_hex(NULL, this);
	cb_reg_m_hex(NULL, this);

	// Deselect all "Dec" buttons
	m_pADec->value(0);
	m_pBDec->value(0);
	m_pCDec->value(0);
	m_pDDec->value(0);
	m_pEDec->value(0);
	m_pHDec->value(0);
	m_pLDec->value(0);
	m_pPCDec->value(0);
	m_pSPDec->value(0);
	m_pBCDec->value(0);
	m_pDEDec->value(0);
	m_pHLDec->value(0);
	m_pMDec->value(0);

	// Select all "Hex" buttons
	m_pAHex->value(1);
	m_pBHex->value(1);
	m_pCHex->value(1);
	m_pDHex->value(1);
	m_pEHex->value(1);
	m_pHHex->value(1);
	m_pLHex->value(1);
	m_pPCHex->value(1);
	m_pSPHex->value(1);
	m_pBCHex->value(1);
	m_pDEHex->value(1);
	m_pHLHex->value(1);
	m_pMHex->value(1);

	// reselect the "All Hex" button
	m_pAllHex->value(1);
}

/*
============================================================================
Routine to handle Reg All Dec display mode
============================================================================
*/
void cb_reg_all_dec(Fl_Widget* w, void* pOpaque)
{
	VTCpuRegs* pCpuRegs = (VTCpuRegs *) pOpaque;
	pCpuRegs->RegAllDec();
}

void VTCpuRegs::RegAllDec(void)
{
	// Change format of all items to Dec
	cb_reg_a_dec(NULL, this);
	cb_reg_b_dec(NULL, this);
	cb_reg_c_dec(NULL, this);
	cb_reg_d_dec(NULL, this);
	cb_reg_e_dec(NULL, this);
	cb_reg_h_dec(NULL, this);
	cb_reg_l_dec(NULL, this);
	cb_reg_pc_dec(NULL, this);
	cb_reg_sp_dec(NULL, this);
	cb_reg_bc_dec(NULL, this);
	cb_reg_de_dec(NULL, this);
	cb_reg_hl_dec(NULL, this);
	cb_reg_m_dec(NULL, this);

	// Deselect all "Hex" buttons
	m_pAHex->value(0);
	m_pBHex->value(0);
	m_pCHex->value(0);
	m_pDHex->value(0);
	m_pEHex->value(0);
	m_pHHex->value(0);
	m_pLHex->value(0);
	m_pPCHex->value(0);
	m_pSPHex->value(0);
	m_pBCHex->value(0);
	m_pDEHex->value(0);
	m_pHLHex->value(0);
	m_pMHex->value(0);

	// Select all "Dec" buttons
	m_pADec->value(1);
	m_pBDec->value(1);
	m_pCDec->value(1);
	m_pDDec->value(1);
	m_pEDec->value(1);
	m_pHDec->value(1);
	m_pLDec->value(1);
	m_pPCDec->value(1);
	m_pSPDec->value(1);
	m_pBCDec->value(1);
	m_pDEDec->value(1);
	m_pHLDec->value(1);
	m_pMDec->value(1);

	// Reselect "All" Dec button
	m_pAllDec->value(1);
}

/*
============================================================================
Routine to handle debugInts checkbox clicks
============================================================================
*/
void cb_debugInts(Fl_Widget* w, void* pOpaque)
{
	VTCpuRegs*	pCpuRegs = (VTCpuRegs *) pOpaque;

	gDebugInts = pCpuRegs->m_pDebugInts->value();
}

/*
============================================================================
Routine to handle autoScroll checkbox clicks
============================================================================
*/
void cb_autoscroll(Fl_Widget* w, void* pOpaque)
{
	VTCpuRegs*	pCpuRegs = (VTCpuRegs *) pOpaque;

	pCpuRegs->m_autoScroll = pCpuRegs->m_pAutoScroll->value();
}

/*
============================================================================
Routine to handle saveBreak checkbox clicks
============================================================================
*/
void cb_saveBreakpoints(Fl_Widget* w, void* pOpaque)
{
	VTCpuRegs*	pCpuRegs = (VTCpuRegs *) pOpaque;

	pCpuRegs->m_saveBreakpoints = pCpuRegs->m_pSaveBreak->value();
}

/*
============================================================================
This routine is the timout callback for enabling debug monitoring.
============================================================================
*/
void cb_enable_monitor(void *pOpaque)
{
	debug_set_monitor_callback(debug_cpuregs_cb);
}

/*
============================================================================
Routine to create the PeripheralSetup Window and tabs
============================================================================
*/
void cb_CpuRegs (Fl_Widget* w, void*)
{
	if (gcpuw != NULL)
		return;

	// Create a CPU Registers window
	gcpuw = new VTCpuRegs(640, 480, "CPU Registers");
	gcpuw->callback(cb_cpuregswin, gcpuw);
	gcpuw->end();

	// Show the new window
	gcpuw->show();

	// Resize if user preferences have been set
	if (gcpuw->m_x != -1 && gcpuw->m_y != -1 && gcpuw->m_w != -1 && gcpuw->m_h != -1)
	{
		// Expand the window
		expand_window(gcpuw, gcpuw->m_x, gcpuw->m_y, gcpuw->m_w, gcpuw->m_h);
	}

	// Check if VirtualT has had time to initialize yet.  If not, we set a
	// timeout callback to enable monitoring, otherwise we just enable it directly
	if (cycles < 10000000)
		Fl::add_timeout(1.0, cb_enable_monitor, gcpuw);
	else
		debug_set_monitor_callback(debug_cpuregs_cb);

	// Indicate an active debug window
	gDebugActive++;
}

/*
============================================================================
CpuRegisters class constructor.  Creates all controls within the window.
============================================================================
*/
void VTCpuRegs::SetTraceColors(void) 
{
	/* Setup some trace colors */
	if (m_colorSyntaxHilight)
	{
		m_traceColors.addr = FL_WHITE;
		m_traceColors.inst = (Fl_Color) 221;
		m_traceColors.number = (Fl_Color) 166;
		m_traceColors.flags = FL_DARK_GREEN;
		m_traceColors.reg = (Fl_Color) 95;
		m_traceColors.reg_val = (Fl_Color) 166;
		m_traceColors.foreground = FL_WHITE;
		m_traceColors.background = FL_BLACK;
		m_traceColors.hilight = FL_DARK_BLUE;
		m_traceColors.select = fl_darker(FL_RED);
	}
	else
	{
		m_traceColors.foreground = FL_BLACK;
		m_traceColors.background = FL_GRAY;
	}
}

/*
============================================================================
CpuRegisters class constructor.  Creates all controls within the window.
============================================================================
*/
VTCpuRegs::VTCpuRegs(int x, int y, const char *title) :
#ifdef WIN32
	Fl_Double_Window(x, y, title)
#else
	Fl_Window(x, y, title)
#endif
{
	Fl_Box*			o;
	int				c;

	/* Load the user preferences for window size, etc. */
	LoadPrefs();
	SetTraceColors();
	for (c = 0; c < sizeof(m_pad); c++)
		m_pad[c] = '1';

	// Allocate the trace buffer
	m_pTraceData = new cpu_trace_data_t[m_traceCount];

	// Clear debugging data
	m_traceAvail = 0;
	m_breakMonitorFreq = 0;
	m_selStart = -1;
	m_selEnd = -1;
	m_haveMouse = FALSE;
	for (c = 0; c < 5; c++)
	{
		m_breakAddr[c] = -1;
		m_breakEnable[c] = 1;
	}

	for (c = 0; c < 64; c++)
		m_sInstTrace[c][0] = '\0';
	m_lastH = 0;

	fl_font(FL_COURIER, m_fontSize);
	m_fontHeight = fl_height() + 1;

	// Create a menu for the new window.
	m_pMenu = new Fl_Menu_Bar(0, 0, x, MENU_HEIGHT-2);
	m_pMenu->menu(gCpuRegs_menuitems);

	// Create static text boxes
	Fl_Group* g1 = new Fl_Group(0, 2+MENU_HEIGHT, x, 170+MENU_HEIGHT-2);

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
	m_pRegPC = new Fl_Input(50, 19+MENU_HEIGHT, 60, 20, "");
	m_pRegPC->deactivate();
	m_pRegPC->callback(cb_reg_pc_changed, this);
	m_pRegPC->value("0x0000");

	// Accumulator Edit Box
	m_pRegA = new Fl_Input(150, 19+MENU_HEIGHT, 60, 20, "");
	m_pRegA->deactivate();
	m_pRegA->value("0x00");

	// Stack Pointer edit box
	m_pRegSP = new Fl_Input(50, 44+MENU_HEIGHT, 60, 20, "");
	m_pRegSP->deactivate();
	m_pRegSP->callback(cb_reg_sp_changed, this);
	m_pRegSP->value("0x0000");

	// Create Flag checkboxes
	// Sign flag
	m_pSFlag = new Fl_Check_Button(223, 19+MENU_HEIGHT, 30, 20, "S");
	m_pSFlag->deactivate();

	// Zero flag
	m_pZFlag = new Fl_Check_Button(253, 19+MENU_HEIGHT, 30, 20, "Z");
	m_pZFlag->deactivate();

	// Carry flag
	m_pCFlag = new Fl_Check_Button(290, 19+MENU_HEIGHT, 30, 20, "C");
	m_pCFlag->deactivate();

	// TS flag
	m_pTSFlag = new Fl_Check_Button(148, 44+MENU_HEIGHT, 40, 20, "TS");
	m_pTSFlag->deactivate();

	// AC flag
	m_pACFlag = new Fl_Check_Button(185, 44+MENU_HEIGHT, 40, 20, "AC");
	m_pACFlag->deactivate();

	// Parity flag
	m_pPFlag = new Fl_Check_Button(223, 44+MENU_HEIGHT, 30, 20, "P");
	m_pPFlag->deactivate();

	// Overflow flag
	m_pOVFlag = new Fl_Check_Button(253, 44+MENU_HEIGHT, 40, 20, "OV");
	m_pOVFlag->deactivate();

	// X flag
	m_pXFlag = new Fl_Check_Button(290, 44+MENU_HEIGHT, 30, 20, "X");
	m_pXFlag->deactivate();

	// Register B  Edit Box
	m_pRegB = new Fl_Input(50, 69+MENU_HEIGHT, 60, 20, "");
	m_pRegB->deactivate();
	m_pRegB->callback(cb_reg_b_changed, this);
	m_pRegB->value("0x00");

	// Register C Edit Box
	m_pRegC = new Fl_Input(150, 69+MENU_HEIGHT, 60, 20, "");
	m_pRegC->deactivate();
	m_pRegC->callback(cb_reg_c_changed, this);
	m_pRegC->value("0x00");

	// Register BC Edit Box
	m_pRegBC = new Fl_Input(250, 69+MENU_HEIGHT, 60, 20, "");
	m_pRegBC->deactivate();
	m_pRegBC->callback(cb_reg_bc_changed, this);
	m_pRegBC->value("0x0000");

	// Register D  Edit Box
	m_pRegD = new Fl_Input(50, 94+MENU_HEIGHT, 60, 20, "");
	m_pRegD->deactivate();
	m_pRegD->callback(cb_reg_d_changed, this);
	m_pRegD->value("0x00");

	// Register E Edit Box
	m_pRegE = new Fl_Input(150, 94+MENU_HEIGHT, 60, 20, "");
	m_pRegE->deactivate();
	m_pRegE->callback(cb_reg_e_changed, this);
	m_pRegE->value("0x00");

	// Register DE Edit Box
	m_pRegDE = new Fl_Input(250, 94+MENU_HEIGHT, 60, 20, "");
	m_pRegDE->deactivate();
	m_pRegDE->callback(cb_reg_de_changed, this);
	m_pRegDE->value("0x0000");

	// Register H  Edit Box
	m_pRegH = new Fl_Input(50, 119+MENU_HEIGHT, 60, 20, "");
	m_pRegH->deactivate();
	m_pRegH->callback(cb_reg_h_changed, this);
	m_pRegH->value("0x00");

	// Register L Edit Box
	m_pRegL = new Fl_Input(150, 119+MENU_HEIGHT, 60, 20, "");
	m_pRegL->deactivate();
	m_pRegL->callback(cb_reg_l_changed, this);
	m_pRegL->value("0x00");

	// Register HL Edit Box
	m_pRegHL = new Fl_Input(250, 119+MENU_HEIGHT, 60, 20, "");
	m_pRegHL->deactivate();
	m_pRegHL->callback(cb_reg_hl_changed, this);
	m_pRegHL->value("0x0000");

	// Register M Edit Box
	m_pRegM = new Fl_Input(250, 144+MENU_HEIGHT, 60, 20, "");
	m_pRegM->deactivate();
	m_pRegM->value("0x00");


	strcpy(m_sPCfmt, "0x%04X");
	strcpy(m_sSPfmt, "0x%04X");
	strcpy(m_sBCfmt, "0x%04X");
	strcpy(m_sDEfmt, "0x%04X");
	strcpy(m_sHLfmt, "0x%04X");
	strcpy(m_sAfmt,  "0x%02X");
	strcpy(m_sBfmt,  "0x%02X");
	strcpy(m_sCfmt,  "0x%02X");
	strcpy(m_sDfmt,  "0x%02X");
	strcpy(m_sEfmt,  "0x%02X");
	strcpy(m_sHfmt,  "0x%02X");
	strcpy(m_sLfmt,  "0x%02X");
	strcpy(m_sMfmt,  "0x%02X");

	// Group the Reg "All" radio buttons together
	{
		m_g = new Fl_Group(440, 10 + MENU_HEIGHT, 100, 20, "");

		// Display format for Reg A
		o = new Fl_Box(FL_NO_BOX, 410, 11+MENU_HEIGHT, 50, 15, "All");
		o->align(FL_ALIGN_LEFT|FL_ALIGN_INSIDE);

		m_pAllHex = new Fl_Round_Button(440, 10 + MENU_HEIGHT, 50, 20, "Hex");
		m_pAllHex->type(FL_RADIO_BUTTON);
		m_pAllHex->callback(cb_reg_all_hex, this);
		m_pAllHex->value(1);

		m_pAllDec  = new Fl_Round_Button(490, 10 + MENU_HEIGHT, 50, 20, "Dec");
		m_pAllDec->type(FL_RADIO_BUTTON);
		m_pAllDec->callback(cb_reg_all_dec, this);
		m_g->end();
	}

	// Group the Reg A radio buttons together
	{
		m_g = new Fl_Group(370, 30 + MENU_HEIGHT, 100, 20, "");

		// Display format for Reg A
		o = new Fl_Box(FL_NO_BOX, 340, 31+MENU_HEIGHT, 50, 15, "A");
		o->align(FL_ALIGN_LEFT|FL_ALIGN_INSIDE);

		m_pAHex = new Fl_Round_Button(370, 30 + MENU_HEIGHT, 50, 20, "Hex");
		m_pAHex->type(FL_RADIO_BUTTON);
		m_pAHex->callback(cb_reg_a_hex, this);
		m_pAHex->value(1);

		m_pADec  = new Fl_Round_Button(420, 30 + MENU_HEIGHT, 50, 20, "Dec");
		m_pADec->type(FL_RADIO_BUTTON);
		m_pADec->callback(cb_reg_a_dec, this);
		m_g->end();
	}

	// Group the Reg B radio buttons togther
	{
		m_g = new Fl_Group(370, 50 + MENU_HEIGHT, 100, 20, "");

		// Display format for Reg B
		o = new Fl_Box(FL_NO_BOX, 340, 51+MENU_HEIGHT, 50, 15, "B");
		o->align(FL_ALIGN_LEFT|FL_ALIGN_INSIDE);

		m_pBHex = new Fl_Round_Button(370, 50 + MENU_HEIGHT, 50, 20, "Hex");
		m_pBHex->type(FL_RADIO_BUTTON);
		m_pBHex->callback(cb_reg_b_hex, this);
		m_pBHex->value(1);

		m_pBDec  = new Fl_Round_Button(420, 50 + MENU_HEIGHT, 50, 20, "Dec");
		m_pBDec->type(FL_RADIO_BUTTON);
		m_pBDec->callback(cb_reg_b_dec, this);
		m_g->end();
	}

	// Group the Reg C radio buttons togther
	{
		m_g = new Fl_Group(370, 70 + MENU_HEIGHT, 100, 20, "");

		// Display format for Reg C
		o = new Fl_Box(FL_NO_BOX, 340, 71+MENU_HEIGHT, 50, 15, "C");
		o->align(FL_ALIGN_LEFT|FL_ALIGN_INSIDE);

		m_pCHex = new Fl_Round_Button(370, 70 + MENU_HEIGHT, 50, 20, "Hex");
		m_pCHex->type(FL_RADIO_BUTTON);
		m_pCHex->callback(cb_reg_c_hex, this);
		m_pCHex->value(1);

		m_pCDec  = new Fl_Round_Button(420, 70 + MENU_HEIGHT, 50, 20, "Dec");
		m_pCDec->type(FL_RADIO_BUTTON);
		m_pCDec->callback(cb_reg_c_dec, this);
		m_g->end();
	}

	// Group the Reg D radio buttons togther
	{
		m_g = new Fl_Group(370, 90 + MENU_HEIGHT, 100, 20, "");

		// Display format for Reg D
		o = new Fl_Box(FL_NO_BOX, 340, 91+MENU_HEIGHT, 50, 15, "D");
		o->align(FL_ALIGN_LEFT|FL_ALIGN_INSIDE);

		m_pDHex = new Fl_Round_Button(370, 90 + MENU_HEIGHT, 50, 20, "Hex");
		m_pDHex->type(FL_RADIO_BUTTON);
		m_pDHex->callback(cb_reg_d_hex, this);
		m_pDHex->value(1);

		m_pDDec  = new Fl_Round_Button(420, 90 + MENU_HEIGHT, 50, 20, "Dec");
		m_pDDec->type(FL_RADIO_BUTTON);
		m_pDDec->callback(cb_reg_d_dec, this);
		m_g->end();
	}

	// Group the Reg E radio buttons togther
	{
		m_g = new Fl_Group(370, 110 + MENU_HEIGHT, 100, 20, "");

		// Display format for Reg E
		o = new Fl_Box(FL_NO_BOX, 340, 111+MENU_HEIGHT, 50, 15, "E");
		o->align(FL_ALIGN_LEFT|FL_ALIGN_INSIDE);

		m_pEHex = new Fl_Round_Button(370, 110 + MENU_HEIGHT, 50, 20, "Hex");
		m_pEHex->type(FL_RADIO_BUTTON);
		m_pEHex->callback(cb_reg_e_hex, this);
		m_pEHex->value(1);

		m_pEDec  = new Fl_Round_Button(420, 110 + MENU_HEIGHT, 50, 20, "Dec");
		m_pEDec->type(FL_RADIO_BUTTON);
		m_pEDec->callback(cb_reg_e_dec, this);
		m_g->end();
	}

	// Group the Reg H radio buttons togther
	{
		m_g = new Fl_Group(370, 130 + MENU_HEIGHT, 100, 20, "");

		// Display format for Reg H
		o = new Fl_Box(FL_NO_BOX, 340, 131+MENU_HEIGHT, 50, 15, "H");
		o->align(FL_ALIGN_LEFT|FL_ALIGN_INSIDE);

		m_pHHex = new Fl_Round_Button(370, 130 + MENU_HEIGHT, 50, 20, "Hex");
		m_pHHex->type(FL_RADIO_BUTTON);
		m_pHHex->callback(cb_reg_h_hex, this);
		m_pHHex->value(1);

		m_pHDec  = new Fl_Round_Button(420, 130 + MENU_HEIGHT, 50, 20, "Dec");
		m_pHDec->type(FL_RADIO_BUTTON);
		m_pHDec->callback(cb_reg_h_dec, this);
		m_g->end();
	}

	// Group the Reg L radio buttons togther
	{
		m_g = new Fl_Group(370, 150 + MENU_HEIGHT, 100, 20, "");

		// Display format for Reg L
		o = new Fl_Box(FL_NO_BOX, 340, 151+MENU_HEIGHT, 50, 15, "L");
		o->align(FL_ALIGN_LEFT|FL_ALIGN_INSIDE);

		m_pLHex = new Fl_Round_Button(370, 150 + MENU_HEIGHT, 50, 20, "Hex");
		m_pLHex->type(FL_RADIO_BUTTON);
		m_pLHex->callback(cb_reg_l_hex, this);
		m_pLHex->value(1);

		m_pLDec  = new Fl_Round_Button(420, 150 + MENU_HEIGHT, 50, 20, "Dec");
		m_pLDec->type(FL_RADIO_BUTTON);
		m_pLDec->callback(cb_reg_l_dec, this);
		m_g->end();
	}


	// Group the Reg PC radio buttons togther
	{
		m_g = new Fl_Group(520, 30 + MENU_HEIGHT, 100, 20, "");

		// Display format for Reg L
		o = new Fl_Box(FL_NO_BOX, 490, 31+MENU_HEIGHT, 50, 15, "PC");
		o->align(FL_ALIGN_LEFT|FL_ALIGN_INSIDE);

		m_pPCHex = new Fl_Round_Button(520, 30 + MENU_HEIGHT, 50, 20, "Hex");
		m_pPCHex->type(FL_RADIO_BUTTON);
		m_pPCHex->callback(cb_reg_pc_hex, this);
		m_pPCHex->value(1);

		m_pPCDec  = new Fl_Round_Button(570, 30 + MENU_HEIGHT, 50, 20, "Dec");
		m_pPCDec->type(FL_RADIO_BUTTON);
		m_pPCDec->callback(cb_reg_pc_dec, this);
		m_g->end();
	}

	// Group the Reg SP radio buttons togther
	{
		m_g = new Fl_Group(520, 50 + MENU_HEIGHT, 100, 20, "");

		// Display format for Reg L
		o = new Fl_Box(FL_NO_BOX, 490, 51+MENU_HEIGHT, 50, 15, "SP");
		o->align(FL_ALIGN_LEFT|FL_ALIGN_INSIDE);

		m_pSPHex = new Fl_Round_Button(520, 50 + MENU_HEIGHT, 50, 20, "Hex");
		m_pSPHex->type(FL_RADIO_BUTTON);
		m_pSPHex->callback(cb_reg_sp_hex, this);
		m_pSPHex->value(1);

		m_pSPDec  = new Fl_Round_Button(570, 50 + MENU_HEIGHT, 50, 20, "Dec");
		m_pSPDec->type(FL_RADIO_BUTTON);
		m_pSPDec->callback(cb_reg_sp_dec, this);
		m_g->end();
	}

	// Group the Reg BC radio buttons togther
	{
		m_g = new Fl_Group(520, 70 + MENU_HEIGHT, 100, 20, "");

		// Display format for Reg L
		o = new Fl_Box(FL_NO_BOX, 490, 71+MENU_HEIGHT, 50, 15, "BC");
		o->align(FL_ALIGN_LEFT|FL_ALIGN_INSIDE);

		m_pBCHex = new Fl_Round_Button(520, 70 + MENU_HEIGHT, 50, 20, "Hex");
		m_pBCHex->type(FL_RADIO_BUTTON);
		m_pBCHex->callback(cb_reg_bc_hex, this);
		m_pBCHex->value(1);

		m_pBCDec  = new Fl_Round_Button(570, 70 + MENU_HEIGHT, 50, 20, "Dec");
		m_pBCDec->type(FL_RADIO_BUTTON);
		m_pBCDec->callback(cb_reg_bc_dec, this);
		m_g->end();
	}

	// Group the Reg DE radio buttons togther
	{
		m_g = new Fl_Group(520, 90 + MENU_HEIGHT, 100, 20, "");

		// Display format for Reg L
		o = new Fl_Box(FL_NO_BOX, 490, 91+MENU_HEIGHT, 50, 15, "DE");
		o->align(FL_ALIGN_LEFT|FL_ALIGN_INSIDE);

		m_pDEHex = new Fl_Round_Button(520, 90 + MENU_HEIGHT, 50, 20, "Hex");
		m_pDEHex->type(FL_RADIO_BUTTON);
		m_pDEHex->callback(cb_reg_de_hex, this);
		m_pDEHex->value(1);

		m_pDEDec  = new Fl_Round_Button(570, 90 + MENU_HEIGHT, 50, 20, "Dec");
		m_pDEDec->type(FL_RADIO_BUTTON);
		m_pDEDec->callback(cb_reg_de_dec, this);
		m_g->end();
	}

	// Group the Reg HL radio buttons togther
	{
		m_g = new Fl_Group(520, 110 + MENU_HEIGHT, 100, 20, "");

		// Display format for Reg HL
		o = new Fl_Box(FL_NO_BOX, 490, 111+MENU_HEIGHT, 50, 15, "HL");
		o->align(FL_ALIGN_LEFT|FL_ALIGN_INSIDE);

		m_pHLHex = new Fl_Round_Button(520, 110 + MENU_HEIGHT, 50, 20, "Hex");
		m_pHLHex->type(FL_RADIO_BUTTON);
		m_pHLHex->callback(cb_reg_hl_hex, this);
		m_pHLHex->value(1);

		m_pHLDec  = new Fl_Round_Button(570, 110 + MENU_HEIGHT, 50, 20, "Dec");
		m_pHLDec->type(FL_RADIO_BUTTON);
		m_pHLDec->callback(cb_reg_hl_dec, this);
		m_g->end();
	}

	// Group the Reg M radio buttons togther
	{
		m_g = new Fl_Group(520, 130 + MENU_HEIGHT, 100, 20, "");

		// Display format for Reg HL
		o = new Fl_Box(FL_NO_BOX, 490, 131+MENU_HEIGHT, 50, 15, "M");
		o->align(FL_ALIGN_LEFT|FL_ALIGN_INSIDE);

		m_pMHex = new Fl_Round_Button(520, 130 + MENU_HEIGHT, 50, 20, "Hex");
		m_pMHex->type(FL_RADIO_BUTTON);
		m_pMHex->callback(cb_reg_m_hex, this);
		m_pMHex->value(1);

		m_pMDec  = new Fl_Round_Button(570, 130 + MENU_HEIGHT, 50, 20, "Dec");
		m_pMDec->type(FL_RADIO_BUTTON);
		m_pMDec->callback(cb_reg_m_dec, this);
		m_g->end();
	}

	o = new Fl_Box(FL_NO_BOX, x-5, 170+MENU_HEIGHT, 5, 5, "");
	o->hide();
	g1->resizable(o);
	g1->end();

	// Create Instruction Trace Text Boxes
	o = new Fl_Box(FL_NO_BOX, 20, 180+MENU_HEIGHT, 50, 15, "Instruction Trace");
	o->align(FL_ALIGN_LEFT|FL_ALIGN_INSIDE);
	m_pTraceBox = new Fl_Box(FL_DOWN_BOX, 20, 200+MENU_HEIGHT, 530-CPUREGS_TRACE_SCROLL_W, 124, "");

	// Create scroll bar for trace data
	m_pScroll = new Fl_Scrollbar(550-CPUREGS_TRACE_SCROLL_W, 200+MENU_HEIGHT, CPUREGS_TRACE_SCROLL_W, 124);
	m_pScroll->deactivate();

	// Set maximum value of scroll 
	int size = (int) (m_pScroll->h() / m_fontHeight);
	m_pScroll->value(m_traceAvail - size+4, m_traceAvail/size, 0, 4);
	m_pScroll->linesize(1);
	m_pScroll->slider_size(1.0);
	m_pScroll->callback(cb_scrollbar);

	Fl_Box* resize_box = new Fl_Box(FL_DOWN_BOX, 500, 200+MENU_HEIGHT, 5, 5, "");
	resize_box->hide();
	m_iInstTraceHead = 0;
	m_iTraceHead = 0;

	// Create checkbox to disable realtime trace
	m_pDisableTrace = new Fl_Check_Button(200, 180+MENU_HEIGHT, 180, 20, "Disable Real-time Trace");
	m_pDisableTrace->value(gDisableRealtimeTrace);
	m_pDisableTrace->callback(cb_disable_realtime_trace, this);

	// Create Stack Box
	o = new Fl_Box(FL_NO_BOX, 560, 180+MENU_HEIGHT, 50, 20, "Stack");
	o->align(FL_ALIGN_LEFT|FL_ALIGN_INSIDE);
	m_pStackBox = new Fl_Box(FL_DOWN_BOX, 560, 200+MENU_HEIGHT, 65, 124, "");

	// Create a text box to show the selected RAM bank
	m_pRamBank = new Fl_Box(FL_NO_BOX, 20, 360, 100, 18, "");
	m_pRamBank->align(FL_ALIGN_LEFT|FL_ALIGN_INSIDE);
	m_pRamBank->labelfont(FL_HELVETICA_BOLD);

	// Create Breakpoint edit boxes
	o = new Fl_Box(FL_NO_BOX, 20, 390, 100, 15, "Breakpoints:");
	o->align(FL_ALIGN_LEFT|FL_ALIGN_INSIDE);

	// Breakpoint 1
	o = new Fl_Box(FL_NO_BOX, 120, 375, 60, 15, "F1");
	o->align(FL_ALIGN_INSIDE);
	m_pBreak1 = new Fl_Input(120, 390, 60, 20, "");
	m_pBreak1->deactivate();
	m_pBreak1->value("");
	m_pBreakDisable1 = new Fl_Check_Button(125, 410, 40, 20, "Off");
	m_pBreakDisable1->deactivate();

	// Breakpoint 2
	o = new Fl_Box(FL_NO_BOX, 200, 375, 60, 15, "F2");
	o->align(FL_ALIGN_INSIDE);
	m_pBreak2 = new Fl_Input(200, 390, 60, 20, "");
	m_pBreak2->deactivate();
	m_pBreak2->value("");
	m_pBreakDisable2 = new Fl_Check_Button(205, 410, 40, 20, "Off");
	m_pBreakDisable2->deactivate();

	// Breakpoint 3
	o = new Fl_Box(FL_NO_BOX, 280, 375, 60, 15, "F3");
	o->align(FL_ALIGN_INSIDE);
	m_pBreak3 = new Fl_Input(280, 390, 60, 20, "");
	m_pBreak3->deactivate();
	m_pBreak3->value("");
	m_pBreakDisable3 = new Fl_Check_Button(285, 410, 40, 20, "Off");
	m_pBreakDisable3->deactivate();

	// Breakpoint 4
	o = new Fl_Box(FL_NO_BOX, 360, 375, 60, 15, "F4");
	o->align(FL_ALIGN_INSIDE);
	m_pBreak4 = new Fl_Input(360, 390, 60, 20, "");
	m_pBreak4->deactivate();
	m_pBreak4->value("");
	m_pBreakDisable4 = new Fl_Check_Button(365, 410, 40, 20, "Off");
	m_pBreakDisable4->deactivate();

	if (m_saveBreakpoints)
	{
		// Set breakpoint values from user preferences
		m_pBreak1->value((const char *) m_sBreak1);
		m_pBreakDisable1->value(m_iBreakDis1);
		m_pBreak2->value((const char *) m_sBreak2);
		m_pBreakDisable2->value(m_iBreakDis2);
		m_pBreak3->value((const char *) m_sBreak3);
		m_pBreakDisable3->value(m_iBreakDis3);
		m_pBreak4->value((const char *) m_sBreak4);
		m_pBreakDisable4->value(m_iBreakDis4);
	}

	m_pDebugInts = new Fl_Check_Button(465, 375, 90, 20, "Debug ISRs");
	m_pDebugInts->value(gDebugInts);
	m_pDebugInts->callback(cb_debugInts, this);

	m_pAutoScroll = new Fl_Check_Button(465, 395, 80, 20, "Auto Scroll");
	m_pAutoScroll->value(m_autoScroll);
	m_pAutoScroll->callback(cb_autoscroll, this);

	m_pSaveBreak = new Fl_Check_Button(465, 415, 120, 20, "Save Breakpoints");
	m_pSaveBreak->value(m_saveBreakpoints);
	m_pSaveBreak->callback(cb_saveBreakpoints, this);

	// Create Stop button
	m_pStop = new Fl_Button(30, 445, 80, 25, "Stop");
	m_pStop->callback(cb_debug_stop, this);

	// Create Step button
	m_pStep = new Fl_Button(130, 445, 80, 25, "Step");
	m_pStep->deactivate();
	m_pStep->callback(cb_debug_step, this);

	// Create Step button
	m_pStepOver = new Fl_Button(230, 445, 100, 25, "Step Over");
	m_pStepOver->deactivate();
	m_pStepOver->callback(cb_debug_step_over, this);

	// Create Run button
	m_pRun = new Fl_Button(350, 445, 80, 25, "Run");
	m_pRun->deactivate();
	m_pRun->callback(cb_debug_run, this);

	resizable(resize_box);

	check_breakpoint_entries();
}

/*
============================================================================
CpuRegisters class destructor.
============================================================================
*/
VTCpuRegs::~VTCpuRegs()
{
	// Delete the trace data buffer
	if (m_pTraceData != NULL)
	{
		delete[] m_pTraceData;
		m_pTraceData = NULL;
	}
}

void VTCpuRegs::draw(void)
{
	make_current();

#ifdef WIN32
	Fl_Window::draw();
#else
	Fl_Window::draw();
#endif

	// Select 12 point Courier font
	fl_font(FL_COURIER, m_fontSize);
	m_fontHeight = fl_height() + 1;

	// Test for a change in height
	if (m_lastH != m_pTraceBox->h())
	{
		// Save the current trace box height
		m_lastH = m_pTraceBox->h();

		int current_val = m_pScroll->value();
		int size = (int) (m_pScroll->h() / m_fontHeight);
		if (size == 0)
			size = 1;
		if (current_val + size + 4 > m_traceAvail)
			current_val = m_traceAvail - size + 4;
		int max_size =  m_traceAvail-size+4;
		if (max_size < 0)
			max_size = 4;
		m_pScroll->value(current_val, m_traceAvail/size, 0, max_size);
	}

	if (gDisableRealtimeTrace && !gStopped)
	{
		// Make our window current for drawing
		make_current();

		// Clear rectangle
		fl_color(m_traceColors.background);
		fl_rectf(m_pTraceBox->x()+3, m_pTraceBox->y()+3,
			m_pTraceBox->w()-6, m_pTraceBox->h()-6);
		fl_rectf(m_pStackBox->x()+3, m_pStackBox->y()+3,
			m_pStackBox->w()-6, m_pStackBox->h()-6);

		// Select 11 point Courier font
		fl_font(FL_COURIER,18);
		fl_color(m_traceColors.foreground);

		fl_draw("Realtime trace disabled", 55, 212+MENU_HEIGHT+3*m_fontHeight);
	}
	else
		redraw_trace(NULL, this);

}

/*
============================================================================
Load user preferences
============================================================================
*/
void VTCpuRegs::LoadPrefs(void)
{
	int		temp;
	char	str[100];

	virtualt_prefs.get("CpuRegs_autoScroll", m_autoScroll, 1);
	virtualt_prefs.get("CpuRegs_x", m_x, -1);
	virtualt_prefs.get("CpuRegs_y", m_y, -1);
	virtualt_prefs.get("CpuRegs_w", m_w, -1);
	virtualt_prefs.get("CpuRegs_h", m_h, -1);
	virtualt_prefs.get("CpuRegs_traceCount", m_traceCount, CPUREGS_TRACE_COUNT);
	virtualt_prefs.get("CpuRegs_fontSize", m_fontSize, 12);
	virtualt_prefs.get("CpuRegs_disableTrace", gDisableRealtimeTrace, 0);
	virtualt_prefs.get("CpuRegs_debugInts", temp, 1);
	gDebugInts = temp;
	virtualt_prefs.get("CpuRegs_rememberBreakpoints", m_saveBreakpoints, 1);
	virtualt_prefs.get("CpuRegs_inverseHilight", m_inverseHilight, 1);
	virtualt_prefs.get("CpuRegs_showAs16Bit", m_showAs16Bit, 0);
	virtualt_prefs.get("CpuRegs_colorSyntaxHilight", m_colorSyntaxHilight, 1);

	// Validate the x,y coords
	int screenX, screenY, screenW, screenH;
	Fl::screen_xywh(screenX, screenY, screenW, screenH);
	if (m_x < 5)
		m_x = 30;
	if (m_y < 5)
		m_y = 30;
	if (m_x > screenW)
		m_x = screenW - 200;
	if (m_y > screenH)
		m_y = screenH - 200;
	if (m_w > screenW)
		m_w = screenW - 100;
	if (m_h > screenH)
		m_h = screenH - 100;

	if (m_saveBreakpoints)
	{
		virtualt_prefs.get("CpuRegs_break1", str, "", sizeof(str));
		m_sBreak1 = str;
		virtualt_prefs.get("CpuRegs_break2", str, "", sizeof(str));
		m_sBreak2 = str;
		virtualt_prefs.get("CpuRegs_break3", str, "", sizeof(str));
		m_sBreak3 = str;
		virtualt_prefs.get("CpuRegs_break4", str, "", sizeof(str));
		m_sBreak4 = str;
		virtualt_prefs.get("CpuRegs_break1_disable", m_iBreakDis1, 0);
		virtualt_prefs.get("CpuRegs_break2_disable", m_iBreakDis2, 0);
		virtualt_prefs.get("CpuRegs_break3_disable", m_iBreakDis3, 0);
		virtualt_prefs.get("CpuRegs_break4_disable", m_iBreakDis4, 0);
	}
}

/*
============================================================================
Load user preferences
============================================================================
*/
void VTCpuRegs::SavePrefs(void)
{
	virtualt_prefs.set("CpuRegs_autoScroll", m_autoScroll);
	virtualt_prefs.set("CpuRegs_x", x());
	virtualt_prefs.set("CpuRegs_y", y());
	virtualt_prefs.set("CpuRegs_w", w());
	virtualt_prefs.set("CpuRegs_h", h());
	virtualt_prefs.set("CpuRegs_traceCount", m_traceCount);
	virtualt_prefs.set("CpuRegs_fontSize", m_fontSize);
	virtualt_prefs.set("CpuRegs_disableTrace", gDisableRealtimeTrace);
	virtualt_prefs.set("CpuRegs_debugInts", gDebugInts);
	virtualt_prefs.set("CpuRegs_rememberBreakpoints", m_saveBreakpoints);
	virtualt_prefs.set("CpuRegs_inverseHilight", m_inverseHilight);
	virtualt_prefs.set("CpuRegs_showAs16Bit", m_showAs16Bit);

	if (m_saveBreakpoints)
	{
		virtualt_prefs.set("CpuRegs_break1", m_pBreak1->value());
		virtualt_prefs.set("CpuRegs_break2", m_pBreak2->value());
		virtualt_prefs.set("CpuRegs_break3", m_pBreak3->value());
		virtualt_prefs.set("CpuRegs_break4", m_pBreak4->value());
		virtualt_prefs.set("CpuRegs_break1_disable", m_pBreakDisable1->value());
		virtualt_prefs.set("CpuRegs_break2_disable", m_pBreakDisable2->value());
		virtualt_prefs.set("CpuRegs_break3_disable", m_pBreakDisable3->value());
		virtualt_prefs.set("CpuRegs_break4_disable", m_pBreakDisable4->value());
	}
}

/*
============================================================================
Scroll to bottom of trace window
============================================================================
*/
void VTCpuRegs::ScrollToBottom(void)
{
	// Update the value to the bottom of the scroll window
	int size = (int) (m_pScroll->h() / m_fontHeight);
	int max_size = m_traceAvail-size+4;
	int new_value = m_traceAvail - size+4;

	// Test if we have less than a screen of data to display
	if (m_traceAvail < m_traceCount)
		new_value++;
	if (max_size < 0)
		max_size = 4;
	if (new_value < 0)
		new_value = 0;
	m_pScroll->value(new_value, m_traceAvail/size, 0, max_size);
	m_pScroll->linesize(1);
	if (m_traceAvail > 0)
	{
		m_pScroll->step(size / m_traceAvail);
		m_pScroll->slider_size((double) size/ (double) m_traceAvail);
	}
	else
		m_pScroll->slider_size(1.0);
	damage(FL_DAMAGE_ALL);
}

/*
============================================================================
Our FLTK event handler.  Capture key-strokes here and don't allow ESC to
close our window, which is the default.
============================================================================
*/
int VTCpuRegs::handle(int eventId)
{
	unsigned int 	key;
	int				mx, my;
	int				tx, ty, th, tw;

	switch (eventId)
	{
	case FL_KEYDOWN:
		key = Fl::event_key();

		if (key == FL_Escape)
			return 1;
		else if (key == FL_F + 1)
		{
			m_pBreak1->take_focus();
			m_pBreak1->position(0, m_pBreak1->size());
		}
		else if (key == FL_F + 2)
		{
			m_pBreak2->take_focus();
			m_pBreak2->position(0, m_pBreak2->size());
		}
		else if (key == FL_F + 3)
		{
			m_pBreak3->take_focus();
			m_pBreak3->position(0, m_pBreak3->size());
		}
		else if (key == FL_F + 4)
		{
			m_pBreak4->take_focus();
			m_pBreak4->position(0, m_pBreak4->size());
		}
		else if (key == FL_F + 7)
		{
			// Find next / prev marker in memory editor
			if (gmew != NULL)
			{
				if (Fl::get_key(FL_Shift_L) || Fl::get_key(FL_Shift_R))
					cb_goto_prev_marker(NULL, NULL);
				else
					cb_goto_next_marker(NULL, NULL);
			}
		}

		break;
	
	case FL_DRAG:
		if (m_haveMouse)
		{
			// Get the coords
			mx = Fl::event_x();
			my = Fl::event_y();

			// Get coords of the trace box
			tx = m_pTraceBox->x();
			ty = m_pTraceBox->y();
			tw = m_pTraceBox->w();
			th = m_pTraceBox->h();

			// Test if mouse pressed within trace window
			if ((mx >= tx) && (mx <= tx+tw) && (my >= ty) && (my <= ty+th))
			{
				int firstLine = m_pScroll->value();
				int	line = (my-ty-2) / m_fontHeight;
				int	selection = firstLine + line;

				// Bound selection on bottom
				if (selection > m_traceAvail)
					selection = m_traceAvail - 1;
				m_selEnd = selection;
				//damage(FL_DAMAGE_EXPOSE, tx, ty, tw, th);
				redraw();
			}

			// Not in window.  Test if we are above or below the window and 
			// do a scroll
			else if (mx >= tx && mx <= tx+tw)
			{
				// We are either above or below the window.  Perform scroll
			}
		}
		break;

	case FL_RELEASE:
		m_haveMouse = FALSE;
		Fl::release();

		if (m_selStart > m_selEnd)
		{
			mx = m_selStart;
			m_selStart = m_selEnd;
			m_selEnd = mx;
		}
		break;

	case FL_PUSH:
		// Get the push coords
		mx = Fl::event_x();
		my = Fl::event_y();

		// Grab the mouse
		Fl::grab();
		m_haveMouse = TRUE;

		// Get coords of the trace box
		tx = m_pTraceBox->x();
		ty = m_pTraceBox->y();
		tw = m_pTraceBox->w();
		th = m_pTraceBox->h();

		// Test if mouse pressed within trace window
		if ((mx >= tx) && (mx <= tx+tw) && (my >= ty) && (my <= ty+th))
		{
			int firstLine = m_pScroll->value();
			int	line = (my-ty-2) / m_fontHeight;
			int	selection = firstLine + line;

			// Check if we clicked within current selection
			if (selection >= m_selStart && selection <= m_selEnd)
			{
				// Click within current selection
			}
			else if (selection >= m_traceAvail)
			{
				// Selection below the trace data.  De-select
				m_selStart = -1;
				m_selEnd = -1;
				m_haveMouse = FALSE;
				Fl::release();
				redraw();
				break;
			}
			else
			{
				// Start of new selection
				m_selStart = firstLine + line;
				m_selEnd = m_selStart;

				// Issue a redraw to show new selection
				redraw();
				return 1;
			}

		}
		break;

	default:
		break;
	}

#ifdef WIN32
	return Fl_Double_Window::handle(eventId);
#else
	return Fl_Window::handle(eventId);
#endif
}

/*
============================================================================
Menu callback for saving trace data
============================================================================
*/
void cb_save_trace(Fl_Widget* w, void* pOpaque)
{
	FILE			*fd;
	Flu_File_Chooser *fc;
	int				c, idx;
	char			lineStr[120];

	fl_cursor(FL_CURSOR_WAIT);
#ifdef __APPLE__
	fc = new Flu_File_Chooser(path,"*.txt",Fl_File_Chooser::CREATE,"Save trace as");
#else
	fc = new Flu_File_Chooser(".","*.txt",Fl_File_Chooser::CREATE,"Save trace as");
#endif
	fl_cursor(FL_CURSOR_DEFAULT);
	fc->preview(0);
	// FileWin.value(0)="RAM.bin"
	fc->show();

	while (fc->visible())
		Fl::wait();

	// Check if a file was selected
	if (fc->value() == 0)
	{
		delete fc;
		return;
	}
	if (strlen(fc->value()) == 0)
	{
		delete fc;
		return;
	}

	// Try to open the ouput file
	fd = fopen(fc->value(), "rb+");
	if (fd != NULL)
	{
		// File already exists! Check for overwrite!
		fclose(fd);

		c = fl_choice("Overwrite existing file?", "Cancel", "Yes", "No");

		if ((c == 0) || (c == 2))
		{
			delete fc;
			return;
		}
	}

	fd = fopen(fc->value(), "wb+");
	if (fd == NULL)
	{
		fl_message("Error opening %s for write", fc->value());
		delete fc;
		return;
	}

	delete fc;

	// Now write trace data to the file
	idx = gcpuw->m_iTraceHead;
	for (c = 0; c < gcpuw->m_traceAvail; c++)
	{
		build_trace_line(idx, lineStr);
		fprintf(fd, "%s\n", lineStr);
		if (++idx >= gcpuw->m_traceCount)
			idx = 0;
	}

	// Finally,close the file
	fclose(fd);
}

/*
============================================================================
Menu callback for clearing trace data
============================================================================
*/
void cb_clear_trace(Fl_Widget* w, void* pOpaque)
{
	// Set availalbe trace items to zero
	gcpuw->m_traceAvail = 0;
	gcpuw->m_iTraceHead = 0;

	// Trace the current instruction
	trace_instruction();

	// Update the scrollbar and redraw
	cb_scrollbar(gcpuw->m_pScroll, gcpuw);
	int size = (int) (gcpuw->m_pScroll->h() / gcpuw->m_fontHeight);
	gcpuw->m_pScroll->value(0, gcpuw->m_traceAvail/size, 0, 4);
	gcpuw->redraw();
}

/*
============================================================================
Menu callback for run
============================================================================
*/
void cb_menu_run(Fl_Widget* w, void* pOpaque)
{
	if (gcpuw && gStopped)
		cb_debug_run(w, gcpuw);
}

/*
============================================================================
Menu callback for stop
============================================================================
*/
void cb_menu_stop(Fl_Widget* w, void* pOpaque)
{
	if (gcpuw && !gStopped)
		cb_debug_stop(w, gcpuw);
}

/*
============================================================================
Menu callback for step
============================================================================
*/
void cb_menu_step(Fl_Widget* w, void* pOpaque)
{
	if (gcpuw && gStopped)
		cb_debug_step(w, gcpuw);
}

/*
============================================================================
Menu callback for step over
============================================================================
*/
void cb_menu_step_over(Fl_Widget* w, void* pOpaque)
{
	if (gcpuw && gStopped)
		cb_debug_step_over(w, gcpuw);
}

/*
============================================================================
Define a local struct for the setup parameters.
============================================================================
*/
typedef struct trace_setup_params
{
	Fl_Input*			pTraceDepth;
	Fl_Input*			pFontSize;
	Fl_Check_Button*	pInverseHilight;
	Fl_Check_Button*	pShowAs16Bit;
	Fl_Check_Button*	pColorHilight;
	char				sDepth[20];
	char				sFontSize[20];
} trace_setup_params_t;

/*
============================================================================
Callback for Trace Setup Ok button
============================================================================
*/
static void cb_setupdlg_OK(Fl_Widget* w, void* pOpaque)
{
	trace_setup_params_t*	p = (trace_setup_params_t *) pOpaque;
	cpu_trace_data_t*		pData;
	int						newDepth, copyLen;
	int						srcIdx, dstIdx;

	// Save values
	if (strlen(p->pFontSize->value()) > 0)
	{
		// update the font size
		gcpuw->m_fontSize = atoi(p->pFontSize->value());
		if (gcpuw->m_fontSize < 6)
			gcpuw->m_fontSize = 6;

		// Upate the font height base on the font size
		fl_font(FL_COURIER, gcpuw->m_fontSize);
		gcpuw->m_fontHeight = fl_height() + 1;
	}

	// Get Inverse Highlight selection
	gcpuw->m_inverseHilight = p->pInverseHilight->value();

	// Get show as 16-bit selection
	gcpuw->m_showAs16Bit = p->pShowAs16Bit->value();

	// Get hilighting option
	gcpuw->m_colorSyntaxHilight = p->pColorHilight->value();
	gcpuw->SetTraceColors();

	// Get the updated trace depth
	newDepth = atoi(p->pTraceDepth->value());
	if (newDepth > 0 && newDepth != gcpuw->m_traceCount)
	{
		if (newDepth > 64000000)
		{
			show_error("Come on, seriously?  That's over a Gig of RAM!  The max I'm willing to do is 64,000,000.");
			return;
		}
		// Allocate a new buffer for trace data
		pData = new cpu_trace_data_t[newDepth];

		if (pData == NULL)
		{
			MString	err;

			err.Format("Unable to allocate %d bytes!\n", newDepth * sizeof(cpu_trace_data_t));
			show_error((const char *) err);
			return;
		}

		// Calculate length of new copy
		copyLen = newDepth;
		if (copyLen > gcpuw->m_traceAvail)
			copyLen = gcpuw->m_traceAvail;

		// Copy data from old buffer to new buffer
		srcIdx = gcpuw->m_iTraceHead;
		for (dstIdx = 0; dstIdx < copyLen; dstIdx++)
		{
			// Check if srcIdx has rolled over
			if (srcIdx >= gcpuw->m_traceAvail)
				srcIdx =0;

			// Copy the data
			pData[dstIdx] = gcpuw->m_pTraceData[srcIdx++];
		}

		// Test for overflow during copy
		if (srcIdx >= newDepth)
			srcIdx = 0;

		// Now replace the old buffer with the new one and update the count
		delete gcpuw->m_pTraceData;
		gcpuw->m_pTraceData = pData;
		gcpuw->m_iTraceHead = srcIdx;
		gcpuw->m_traceCount = newDepth;
		gcpuw->m_traceAvail = copyLen;

		// Redraw the window
		gcpuw->ScrollToBottom();
	}

	// Redraw the window
	gcpuw->redraw();
	
	// Now hide the parent dialog
	w->parent()->hide();
}

/*
============================================================================
Callback for Trace Setup Cancel button
============================================================================
*/
static void cb_setupdlg_cancel(Fl_Widget* w, void* pOpaque)
{
	// Hide the parent dialog so we cancel out
	w->parent()->hide();
}

/*
============================================================================
Menu callback for setup trace
============================================================================
*/
void cb_setup_trace(Fl_Widget* w, void* pOpaque)
{
	Fl_Box*					b;
	Fl_Window*				pWin;
	trace_setup_params_t	p;

	/* Create a new window for the trace configuration */
	pWin = new Fl_Window(300, 200, "Trace Configuration");

	/* Create input field for trace depth */
	b = new Fl_Box(20, 20, 100, 20, "Trace Depth");
	b->align(FL_ALIGN_LEFT | FL_ALIGN_INSIDE);
	p.pTraceDepth = new Fl_Input(120, 20, 80, 20, "");
	p.pTraceDepth->align(FL_ALIGN_LEFT);
	sprintf(p.sDepth, "%d", gcpuw->m_traceCount);
	p.pTraceDepth->value(p.sDepth);
	b = new Fl_Box(210, 20, 100, 20, "64M max");
	b->align(FL_ALIGN_LEFT | FL_ALIGN_INSIDE);

	/* Create input field for font size */
	b = new Fl_Box(20, 50, 100, 20, "Font Size");
	b->align(FL_ALIGN_LEFT | FL_ALIGN_INSIDE);
	p.pFontSize = new Fl_Input(120, 50, 60, 20, "");
	p.pFontSize->align(FL_ALIGN_LEFT);
	sprintf(p.sFontSize, "%d", gcpuw->m_fontSize);
	p.pFontSize->value(p.sFontSize);

	/* Create checkbox for hilight style */
	p.pInverseHilight = new Fl_Check_Button(20, 80, 190, 20, "Inverse Video Hilight");
	p.pInverseHilight->value(gcpuw->m_inverseHilight);

	/* Create checkbox for 16-bit register style */
	p.pShowAs16Bit = new Fl_Check_Button(20, 100, 200, 20, "Show registers as 16 bit");
	p.pShowAs16Bit->value(gcpuw->m_showAs16Bit);

	/* Create checkbox for 16-bit register style */
	p.pColorHilight = new Fl_Check_Button(20, 120, 200, 20, "Color syntax hilighting");
	p.pColorHilight->value(gcpuw->m_colorSyntaxHilight);

	// Cancel button
    { Fl_Button* o = new Fl_Button(80, 160, 60, 30, "Cancel");
      o->callback((Fl_Callback*) cb_setupdlg_cancel);
    }

	// OK button
    { Fl_Return_Button* o = new Fl_Return_Button(160, 160, 60, 30, "OK");
      o->callback((Fl_Callback*) cb_setupdlg_OK, &p);
    }

	// Loop until user presses OK or Cancel
	pWin->show();
	while (pWin->visible())
		Fl::wait();

	// Delete the window
	delete pWin;
}
