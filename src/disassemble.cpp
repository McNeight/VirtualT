/* disassemble.cpp */

/* $Id: disassemble.cpp,v 1.12 2011/07/11 06:17:23 kpettit1 Exp $ */

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


#include <FL/Fl.H>
#include <FL/fl_draw.H>
#include <FL/Fl_Window.H>
#include <FL/Fl_Menu_Item.H>
#include <FL/Fl_Menu_Bar.H>
#include <FL/Fl_Text_Editor.H>
#include <FL/Fl_Text_Buffer.H>
#include <stdio.h>
#include <string.h>
#include <ctype.h>

#include "VirtualT.h"
#include "m100emu.h"
#include "disassemble.h"
#include "io.h"
#include "cpu.h"
#include "periph.h"
#include "memedit.h"
#include "romstrings.h"
#include "cpuregs.h"
#include "memory.h"
#include "fileview.h"

void cb_Ide(Fl_Widget* w, void*) ;

Fl_Window *gpDis;

// Callback routine for the close box of the Disassembler window
static void close_cb(Fl_Widget* w, void*)
{
	if (gpDis != NULL)
	{
		gpDis->hide();
		delete gpDis;
		gpDis = 0;
	}
}

// Table of OPCODE
const char * gStrTable[256] = {
	"NOP",     "LXI B,",  "STAX B",  "INX B",   "INR B",   "DCR B",    "MVI B,",  "RLC",
	"DSUB",    "DAD B",   "LDAX B",  "DCX B",   "INR C",   "DCR C",    "MVI C,",  "RRC", 

	"ASHR",    "LXI D,",  "STAX D",  "INX D",   "INR D",   "DCR D",    "MVI D,",  "RAL", 
	"RLDE",    "DAD D",   "LDAX D",  "DCX D",   "INR E",   "DCR E",    "MVI E,",  "RAR",

	"RIM",     "LXI H,",  "SHLD ",   "INX H",   "INR H",   "DCR H",    "MVI H,",  "DAA",
	"LDEH ",   "DAD H",   "LHLD ",   "DCX H",   "INR L",   "DCR L",    "MVI L,",  "CMA", 

	"SIM",     "LXI SP,", "STA ",    "INX SP",  "INR M",   "DCR M",    "MVI M,",  "STC",
	"LDES ",   "DAD SP",  "LDA ",    "DCX SP",  "INR A",   "DCR A",    "MVI A,",  "CMC",

	"MOV B,B", "MOV B,C", "MOV B,D", "MOV B,E", "MOV B,H", "MOV B,L",  "MOV B,M", "MOV B,A",
	"MOV C,B", "MOV C,C", "MOV C,D", "MOV C,E", "MOV C,H", "MOV C,L",  "MOV C,M", "MOV C,A",

	"MOV D,B", "MOV D,C", "MOV D,D", "MOV D,E", "MOV D,H", "MOV D,L",  "MOV D,M", "MOV D,A",
	"MOV E,B", "MOV E,C", "MOV E,D", "MOV E,E", "MOV E,H", "MOV E,L",  "MOV E,M", "MOV E,A",

	"MOV H,B", "MOV H,C", "MOV H,D", "MOV H,E", "MOV H,H", "MOV H,L",  "MOV H,M", "MOV H,A",
	"MOV L,B", "MOV L,C", "MOV L,D", "MOV L,E", "MOV L,H", "MOV L,L",  "MOV L,M", "MOV L,A",

	"MOV M,B", "MOV M,C", "MOV M,D", "MOV M,E", "MOV M,H", "MOV M,L",  "HLT",     "MOV M,A",
	"MOV A,B", "MOV A,C", "MOV A,D", "MOV A,E", "MOV A,H", "MOV A,L",  "MOV A,M", "MOV A,A",

	"ADD B",   "ADD C",   "ADD D",   "ADD E",   "ADD H",   "ADD L",    "ADD M",   "ADD A",
	"ADC B",   "ADC C",   "ADC D",   "ADC E",   "ADC H",   "ADC L",    "ADC M",   "ADC A",

	"SUB B",   "SUB C",   "SUB D",   "SUB E",   "SUB H",   "SUB L",    "SUB M",   "SUB A",
	"SBB B",   "SBB C",   "SBB D",   "SBB E",   "SUB H",   "SBB L",    "SBB M",   "SBB A",

	"ANA B",   "ANA C",   "ANA D",   "ANA E",   "ANA H",   "ANA L",    "ANA M",   "ANA A",
	"XRA B",   "XRA C",   "XRA D",   "XRA E",   "XRA H",   "XRA L",    "XRA M",   "XRA A",

	"ORA B",   "ORA C",   "ORA D",   "ORA E",   "ORA H",   "ORA L",    "ORA M",   "ORA A",
	"CMP B",   "CMP C",   "CMP D",   "CMP E",   "CMP H",   "CMP L",    "CMP M",   "CMP A",

	"RNZ",     "POP B",   "JNZ ",    "JMP ",    "CNZ ",    "PUSH B",   "ADI ",    "RST 0",
	"RZ",      "RET",     "JZ ",     "RSTV",    "CZ ",     "CALL ",    "ACI ",    "RST 1",

	"RNC",     "POP D",   "JNC ",    "OUT ",    "CNC ",    "PUSH D",   "SUI ",    "RST 2",
	"RC",      "SHLX",    "JC ",     "IN ",     "CC ",     "JNX ",     "SBI ",    "RST 3",

	"RPO",     "POP H",   "JPO ",    "XTHL",    "CPO ",    "PUSH H",   "ANI ",    "RST 4",
	"RPE",     "PCHL",    "JPE ",    "XCHG",    "CPE ",    "LHLX",     "XRI ",    "RST 5",

	"RP",      "POP PSW", "JP ",     "DI",      "CP ",     "PUSH PSW", "ORI ",    "RST 6",
	"RM",      "SPHL",    "JM ",     "EI",      "CM ",     "JX ",      "CPI ",    "RST 7" 
};

// Table indicating length of each opcode
unsigned char gLenTable[256] = {	
	0,2,0,0,0,0,1,0,
	0,0,0,0,0,0,1,0,
	0,2,0,0,0,0,1,0,
	0,0,0,0,0,0,1,0,
	0,2,2,0,0,0,1,0,
	1,0,2,0,0,0,1,0,
	0,2,2,0,0,0,1,0,
	1,0,2,0,0,0,1,0,
	0,0,0,0,0,0,0,0,
	0,0,0,0,0,0,0,0,
	0,0,0,0,0,0,0,0,
	0,0,0,0,0,0,0,0,
	0,0,0,0,0,0,0,0,
	0,0,0,0,0,0,0,0,
	0,0,0,0,0,0,0,0,
	0,0,0,0,0,0,0,0,
	0,0,0,0,0,0,0,0,
	0,0,0,0,0,0,0,0,
	0,0,0,0,0,0,0,0,
	0,0,0,0,0,0,0,0,
	0,0,0,0,0,0,0,0,
	0,0,0,0,0,0,0,0,
	0,0,0,0,0,0,0,0,
	0,0,0,0,0,0,0,0,
	0,0,2,2,2,0,1,0,
	0,0,2,0,2,2,1,0,
	0,0,2,1,2,0,1,0,
	0,0,2,1,2,2,1,0,
	0,0,2,0,2,0,1,0,
	0,0,2,0,2,0,1,0,
	0,0,2,0,2,0,1,0,
	0,0,2,0,2,2,1,0
};

// Menu items for the disassembler
Fl_Menu_Item gDis_menuitems[] = {
  { "&File",              0, 0, 0, FL_SUBMENU },
	{ "Open...",          0, 0, 0 },
	{ "Save...",          0, 0, 0, FL_MENU_DIVIDER },
	{ "Close",            FL_CTRL + 'q', close_cb, 0 },
	{ 0 },

  { "&Tools", 0, 0, 0, FL_SUBMENU },
	{ "CPU Registers",         0, cb_CpuRegs },
	{ "Assembler / IDE",       0, cb_Ide },
	{ "Memory Editor",         0, cb_MemoryEditor },
	{ "Peripheral Devices",    0, cb_PeripheralDevices },
//	{ "Simulation Log Viewer", 0, 0 },
	{ "Model T File Viewer",   0, cb_FileView },
	{ 0 },

  { 0 }
};

/*
=======================================================
Menu Item Callbacks
=======================================================
*/
void disassembler_cb(Fl_Widget* w, void*) {

	if (gpDis == NULL)
	{
		// Create a new window for the disassembler
		gpDis = new Fl_Window(600, 400 , "Disassembler");
		gpDis->callback(close_cb);

		// Create a menu for the new window.
		Fl_Menu_Bar *m = new Fl_Menu_Bar(0, 0, 600, MENU_HEIGHT-2);
//		gpDisp = new T100_Disp(0, MENU_HEIGHT, 240*MultFact + 90*DisplayMode, 64*MultFact + 50*DisplayMode);
		m->menu(gDis_menuitems);

		// Create a Text Editor to show/edit the disassembled text
		Fl_Text_Editor* td = new Fl_Text_Editor(0, MENU_HEIGHT, 600, 400-MENU_HEIGHT);

		// Create a Text Buffer for the Text Editor to work with
		Fl_Text_Buffer* tb = new Fl_Text_Buffer();
		td->buffer(tb);

		// Show the Disassembling text to indicate activity
		tb->append("\n\n\n   Disassembling...");
		td->textfont(FL_COURIER);

		// Create a Disassembler
		VTDis *pDisassembler = new VTDis();

		// Assign the RomDescription table for the model being emulated
		pDisassembler->m_pRom = gStdRomDesc;

		// Give the disassembler the text editor to dump it's data into
		pDisassembler->SetTextViewer(td);

		// Give the disassembler something to disassemble
		pDisassembler->CopyIntoMem(gSysROM, ROMSIZE);
//		pDisassembler->CopyIntoMem((unsigned char*) gBaseMemory, 65536);

		gpDis->resizable(m);
		gpDis->resizable(td);

		gpDis->end();
		gpDis->show();

		// Show the new window
		Fl::wait();

		// Disassemble the code (this should be moved later when dissasembly
		// of .CO programs is supported)
		pDisassembler->Disassemble();
	}
	else
	{
	}
}

/*
=======================================================
Disassembler Class Definition
=======================================================
*/
VTDis::VTDis()
{
	m_StartAddress = 0;
	m_EndAddress = ROMSIZE-1;
	m_BaseAddress = 0;
	m_WantComments = 0;
}

void VTDis::SetTextViewer(Fl_Text_Editor *pTextViewer)
{
	m_pTextViewer = pTextViewer;
}

void VTDis::AppendComments(char* line, int opcode, int address)
{
	int		i, x, len, op_len;

	// Calculate # spaces to add for fixed line len (make comments line up)
	len = 28 - strlen(line);

	// Determine length of this opcode
	op_len = gLenTable[opcode] & 0x03;

	// Add comments for known addresses.  Opcodes less that 128 are Variable
	// spaces (these opcodes are LXI, LHLD, etc.)
	if ((op_len == 2) && (opcode < 128))
	{
		x = 0;
		while (m_pRom->pVars[x].addr != -1)
		{
			if (m_pRom->pVars[x].addr == address)
			{
				for (i = 0; i < len; i++)
					strcat(line, " ");
				strcat(line, "; ");
				strcat(line, gDisStrings[m_pRom->pVars[x].strnum].desc);
				break;
			}

			x++;
		}
	}
	// Add comments for known function call addresses. Opcodes 128 and greater
	// are CALLs and JMPs
	if ((op_len == 2) && (opcode > 128))
	{
		x = 0;
		while (m_pRom->pFuns[x].addr != -1)
		{
			if (m_pRom->pFuns[x].addr == address)
			{
				for (i = 0; i < len; i++)
					strcat(line, " ");
				strcat(line, "; ");
				strcat(line, gDisStrings[m_pRom->pFuns[x].strnum].desc);
				break;
			}

			x++;
		}
	}

	// Handle RST 1 Commenting
	if (opcode == 0xCF)
	{
		for (i = 0; i < len; i++)
			strcat(line, " ");
		strcat(line, "; Compare next byte with M");
	}
	// Handle RST 2 Commenting
	if (opcode == 0xD7)
	{
		for (i = 0; i < len; i++)
			strcat(line, " ");
		strcat(line, "; Get next non-white char from M");
	}
	// Handle RST 3 Commenting
	if (opcode == 0xDF)
	{
		for (i = 0; i < len; i++)
			strcat(line, " ");
		strcat(line, "; Compare DE and HL");
	}
	// Handle RST 4 Commenting
	if (opcode == 0xE7)
	{
		for (i = 0; i < len; i++)
			strcat(line, " ");
		strcat(line, "; Send character in A to screen/printer");
	}
	// Handle RST 5 Commenting
	if (opcode == 0xEF)
	{
		for (i = 0; i < len; i++)
			strcat(line, " ");
		strcat(line, "; Determine type of last var used");
	}
	// Handle RST 6 Commenting
	if (opcode == 0xF7)
	{
		for (i = 0; i < len; i++)
			strcat(line, " ");
		strcat(line, "; Get sign of FAC1");
	}
	// Handle RST 7 Commenting
	if (opcode == 0xFF)
	{
		for (i = 0; i < len; i++)
			strcat(line, " ");
		strcat(line, "; Jump to RST 38H Vector entry of following byte");
	}
}

void VTDis::Disassemble()
{
	int		c;
	int		x;
	int		table;
	char	line[200];
	char	arg[60];
	int		addr;
	int		rst7 = 0;
	int		rst1 = 0;
	unsigned char opcode;
	unsigned char op_len;
	Fl_Text_Buffer*	tb;


	tb = new Fl_Text_Buffer;
	for (c = m_StartAddress; c <= m_EndAddress; c++)
	{
		// Search for C in the function table
		x = 0;
		while (m_pRom->pFuns[x].addr != -1)
		{
			if (m_pRom->pFuns[x].addr == c)
			{
				strcpy(line, "\n; ======================================================\n; ");
				strcat(line, gDisStrings[m_pRom->pFuns[x].strnum].desc);
				strcat(line, "\n; ======================================================\n");
				// Add line to Edit Buffer
				tb->append(line);
				break;
			}

			x++;
		}

		// Check if address is a table instead of code
		x = 0;
		table = 0;

		// Check if last instruction was RST7 (that means this byte is a DB)
		if (rst7 == 1)
		{
			sprintf(line, "%04XH  DB   %02XH\n", c, m_memory[c]);
			tb->append(line);
			rst7 = 0;
			continue;
		}

		// Check if last instruction was RST1 (that means this byte is a DB)
		if (rst1 == 1)
		{
			sprintf(line, "%04XH  DB   %02XH\n", c, m_memory[c]);
			tb->append(line);
			rst1 = 0;
			continue;
		}

		// Loop though the array of table definitions for the current address
		while (m_pRom->pTables[x].addr != -1)
		{
			// Current address found?
			if (m_pRom->pTables[x].addr == c)
			{
				table = 1 ;
				// Modified strings are those that start with a 0x80 and ends
				// when the next string is found (the next 0x80).  All the
				// BASIC keywords are stored this way
				if (m_pRom->pTables[x].type == TABLE_TYPE_MODIFIED_STRING)
				{
					int next;
					int last = c + m_pRom->pTables[x].size;
					int str_active = 0;
					for (next = c; next < last; next++)
					{
						if ((m_memory[next] & 0x80) && (str_active))
						{
							if ((m_memory[next-1] & 0x80) == 0)
								strcat(line, "\"");
							strcat(line, "\n");
							tb->append(line);
							line[0]=0;
							str_active = 0;
						}
						if (!str_active)
						{
							sprintf(line, "%04XH  DB   80H or '%c'", next, m_memory[next] & 0x7F);
							str_active = 1;
							if ((m_memory[next+1] & 0x80) == 0)
								strcat(line, ",\"");
						}
						else
						{
							sprintf(arg, "%c", m_memory[next]);
							strcat(line, arg);
						}

						if (next + 1 == last)
						{
							if ((m_memory[next-1] & 0x80) == 0)
								strcat(line, "\"");
							strcat(line, "\n\n");
							tb->append(line);
							line[0] = 0;
						}
					}
				}
				// Modified strings are those that start with a 0x80 and ends
				// when the next string is found (the next 0x80).  All the
				// BASIC keywords are stored this way
				else if ((m_pRom->pTables[x].type == TABLE_TYPE_MODIFIED_STRING2)
					|| (m_pRom->pTables[x].type == TABLE_TYPE_MODIFIED_STRING3))
				{
					int next;
					int last = c + m_pRom->pTables[x].size;
					int str_active = 0;
					for (next = c; next < last; next++)
					{
						if (m_memory[next] == 0)
						{
							sprintf(line, "%04XH  DB   00H\n", next);
							tb->append(line);
							line[0] = 0;
							continue;
						}
						if ((m_memory[next] & 0x80) && (str_active))
						{
							sprintf(arg, "\",'%c' or 80H", m_memory[next] & 0x7F);
							strcat(line, arg);
							if (m_pRom->pTables[x].type == TABLE_TYPE_MODIFIED_STRING2)
							{ 
								sprintf(arg, ", %02XH\n", m_memory[next+1]);
								next++;
							}
							else
								strcpy(arg, "\n");
							strcat(line, arg);
//							if ((m_memory[next-1] & 0x80) == 0)
//								strcat(line, "\"");
//							strcat(line, "\n");
							tb->append(line);
							line[0]=0;
							str_active = 0;
							continue;
						}
						if (!str_active)
						{
							if (m_memory[next] & 0x80)
							{
								sprintf(line, "%04XH  DB   '%c' OR 80H", next, m_memory[next] & 0x7F);
								if (m_pRom->pTables[x].type == TABLE_TYPE_MODIFIED_STRING2)
								{
									sprintf(arg, ", %02XH\n", m_memory[next+1]);
									next++;
								}
								else
									strcpy(arg, "\n");
								strcat(line, arg);
								tb->append(line);
								line[0]=0;
								str_active = 0;
								continue;
							}
							else
							{
								sprintf(line, "%04XH  DB   \"%c", next, m_memory[next]);
								str_active = 1;
							}
						}
						else
						{
							sprintf(arg, "%c", m_memory[next]);
							strcat(line, arg);
						}

						if (next + 1 == last)
						{
							if ((m_memory[next-1] & 0x80) == 0)
								strcat(line, "\"");
							strcat(line, "\n\n");
							tb->append(line);
							line[0] = 0;
						}
					}
				}
				// A string is one that ends with NULL (0x00)
				else if (m_pRom->pTables[x].type == TABLE_TYPE_STRING)
				{
					int next;
					int last = c + m_pRom->pTables[x].size;
					int str_active = 0;
					int quote_active = 0;
					for (next = c; next < last; next++)
					{
						if ((m_memory[next] == 0) && (str_active))
						{
							if (quote_active)
								strcat(line, "\"");
							strcat(line, ",00H\n");
							tb->append(line);
							line[0]=0;
							str_active = 0;
							quote_active = 0;
						}
						else if (!str_active)
						{
							if (m_memory[next] == 0)
							{
								sprintf(line, "%04XH  DB   00H\n", next);
								tb->append(line);
								line[0]=0;
							}
							else
							{
								sprintf(line, "%04XH  DB   \"%c", next, m_memory[next]);
								quote_active = 1;
								str_active = 1;
							}
						}
						else
						{
							if (iscntrl(m_memory[next]))
							{
								if (quote_active)
								{
									strcat(line, "\"");
									quote_active = 0;
								}
								sprintf(arg, ",%02XH", m_memory[next]);
							}
							else
							{
								if (!quote_active)
									sprintf(arg, ",\"%c", m_memory[next]);
								else if (m_memory[next] == '"')
									sprintf(arg, "\\%c", m_memory[next]);
								else
									sprintf(arg, "%c", m_memory[next]);
								quote_active = 1;
							}
							strcat(line, arg);
						}
						if ((next+1 == last) && (m_memory[next] != 0))
						{
							if (quote_active)
								strcat(line, "\"");
							strcat(line, "\n");
							tb->append(line);
							line[0]=0;
							str_active = 0;
							quote_active = 0;
						}
					}
				}
				// CTRL_DELIM tables are string that have a non-ASCII byte
				// between them
				else if (m_pRom->pTables[x].type == TABLE_TYPE_CTRL_DELIM)
				{
					int next;
					int last = c + m_pRom->pTables[x].size;
					int str_active = 0;
					for (next = c; next < last; next++)
					{
						if ((m_memory[next] > 0x7E) && (str_active))
						{
							sprintf(arg, "\",%02XH\n", m_memory[next]);
							strcat(line, arg);
							tb->append(line);
							line[0]=0;
							str_active = 0;
							continue;
						}
						if (!str_active)
						{
							if (m_memory[next] > 0x7E)
							{
								sprintf(line, "%04XH  DB   %02XH\n", next, m_memory[next]);
								tb->append(line);
							}
							else					   
							{
								sprintf(line, "%04XH  DB   \"%c", next, m_memory[next]);
								str_active = 1;
							}
						}
						else
						{
							sprintf(arg, "%c", m_memory[next]);
							strcat(line, arg);
						}
					}
				}
				// A JUMP table is one that contains 16 bit address in a table
				else if (m_pRom->pTables[x].type == TABLE_TYPE_JUMP)
				{
					int next;
					int last = c + m_pRom->pTables[x].size;
					int count = 0;
					for (next = c; next < last; next += 2)
					{
						if (count == 0)
							sprintf(line, "%04XH  DW   ", next);

						sprintf(arg, "%04XH", m_memory[next] | (m_memory[next+1] << 8));
						strcat(line, arg);
						if ((next+2 != last) && (count != 3))
							strcat(line, ",");
						else
						{
							strcat(line, "\n");
							if (next+2 == last)
								strcat(line, "\n");
						}

						if ((count == 3) || (next+2 == last))
						{
							tb->append(line);
							count = 0;
						}
						else
						    count++;
					}
				}
				// A 2BYTE table is one with entries that are exactly 2 bytes
				// This is used for the BASIC error message text
				else if (m_pRom->pTables[x].type == TABLE_TYPE_2BYTE)
				{
					int next;
					int last = c + m_pRom->pTables[x].size;
					for (next = c; next < last; next += 2)
					{
						sprintf(line, "%04XH  DB   \"%c%c\"\n", next, m_memory[next], m_memory[next+1]);
						tb->append(line);
					}
				}
				// A 3BYTE table is one with entries that are exactly 3 bytes
				// This is used for the Day of week and Month text (Mon, Tue, Jan, Feb)
				else if (m_pRom->pTables[x].type == TABLE_TYPE_3BYTE)
				{
					int next;
					int last = c + m_pRom->pTables[x].size;
					for (next = c; next < last; next += 3)
					{
						sprintf(line, "%04XH  DB   \"%c%c%c\"\n", next, m_memory[next], m_memory[next+1],
							m_memory[next+2]);
						tb->append(line);
					}
				}
				// A BYTE_LOOKUP is a table that has a single "search" byte with a 
				// 2 byte address following it
				else if (m_pRom->pTables[x].type == TABLE_TYPE_BYTE_LOOKUP)
				{
					int next;
					int last = c + m_pRom->pTables[x].size;
					for (next = c; next < last; next += 3)
					{
						if (iscntrl(m_memory[next]))
							sprintf(line, "%04XH  DB   %02XH\n%04XH  DW   %04XH\n", next, m_memory[next],
								next+1, m_memory[next+1] | (m_memory[next+2] << 8));
						else
							sprintf(line, "%04XH  DB   '%c'\n%04XH  DW   %04XH\n", next, m_memory[next],
								next+1, m_memory[next+1] | (m_memory[next+2] << 8));
						tb->append(line);
					}
				}
				// A 4BYTE_CMD table has 4 ASCII bytes (the command) followed by a 
				// 2 byte address
				else if (m_pRom->pTables[x].type == TABLE_TYPE_4BYTE_CMD)
				{
					int next;
					int last = c + m_pRom->pTables[x].size;
					for (next = c; next < last; next += 6)
					{
						sprintf(line, "%04XH  DB   \"%c%c%c%c\"\n", next, m_memory[next],
							m_memory[next+1], m_memory[next+2], m_memory[next+3]);
						tb->append(line);
						sprintf(line, "%04XH  DW   %04XH\n", next+4, m_memory[next+4] | 
							(m_memory[next+5] << 8));
						tb->append(line);
					}
				}
				// A catalog table is just that.  One that contains ROM catalog entries
				else if (m_pRom->pTables[x].type == TABLE_TYPE_CATALOG)
				{
					int next;
					int last = c + m_pRom->pTables[x].size;
					for (next = c; next < last; next += 11)
					{
						sprintf(line, "%04XH  DB   %02XH\n", next, m_memory[next]);
						tb->append(line);
						sprintf(line, "%04XH  DW   %04XH\n", next+1, m_memory[next+1] | 
							(m_memory[next+2] << 8));
						tb->append(line);

						if (m_memory[next+3] == 0)
							sprintf(line, "%04XH  DB   %02XH,\"%c%c%c%c%c%c\",%02XH\n", next+3, m_memory[next+3], 
								m_memory[next+4], m_memory[next+5], m_memory[next+6], m_memory[next+7],
								m_memory[next+8], m_memory[next+9], m_memory[next+10]);
						else
							sprintf(line, "%04XH  DB   \"%c%c%c%c%c%c%c\",%02XH\n", next+3, m_memory[next+3], 
								m_memory[next+4], m_memory[next+5], m_memory[next+6], m_memory[next+7],
								m_memory[next+8], m_memory[next+9], m_memory[next+10]);
						tb->append(line);
					}
				}
				// A CODE table is one where bytes are simply represented with
				// a DB directive.  Bytes are split into lines with 8 bytes per line
				else if (m_pRom->pTables[x].type == TABLE_TYPE_CODE)
				{
					int next;
					int last = c + m_pRom->pTables[x].size;
					int count = 0;
					for (next = c; next < last; next++)
					{
						if (count == 0)
							sprintf(line, "%04XH  DB   ", next);

						sprintf(arg, "%02XH", m_memory[next]);
						strcat(line, arg);
						if ((next+1 != last) && (count != 7))
							strcat(line, ",");
						else
						{
							strcat(line, "\n");
							if (next+1 == last)
							{
								strcat(line, "\n");
								tb->append(line);
								line[0] = 0;
							}
						}

						if (count == 7)
						{
							tb->append(line);
							line[0] = 0;
							count = 0;
						}
						else
						    count++;
					}
				}
				// Add line to Edit Buffer
//				tb->append(line);
				c += m_pRom->pTables[x].size-1;
				break;
			}

			x++;
		}

		// Was this address identified as a table?  If so, don't disassemble it!
		if (table)
			continue;

		// Get opcode from memory
		opcode = m_memory[c];

		// Determine length of this opcode
		op_len = gLenTable[opcode] & 0x03;

		// Print the address and opcode value to the temporary line buffer
		sprintf(line, "%04XH  (%02XH) ", c, opcode);

		// Print the opcode text to the temp line buffer
		strcat(line, gStrTable[opcode]);

		// Check if this opcode has a single byte argument
		if (op_len == 1)
		{
			// Single byte argument
			sprintf(arg, "%02XH", m_memory[c+1]);
			strcat(line, arg);
		}

		// Check if this opcode as a 2 byte argument
		else if (op_len == 2)
		{
			// Double byte argument
			addr = m_memory[c+1] | (m_memory[c+2] << 8);
			sprintf(arg, "%04XH", addr);
			strcat(line, arg);
		}

		AppendComments(line, opcode, addr);

		// Finish off the line
		strcat(line, "\n");

		// Increment memory counter by opcode length
		c += op_len;

		// Add a blank line for JMP, CALL, and RET to provide routine separation
		if (opcode == 195 || opcode == 201 || opcode == 0xE9)
		{
			strcat(line, "\n");
		}

		// Check if opcode is a rst7
		if (opcode == 0xFF)
			rst7 = 1;

		// Check if opcode is a rst1
		if (opcode == 0xCF)
			rst1 = 1;


		// Add line to Edit Buffer
		tb->append(line);
	}

	// Display the text buffer in the text editor
	Fl_Text_Buffer* old_tb = m_pTextViewer->buffer();
	m_pTextViewer->buffer(tb);
	delete old_tb;
}


void VTDis::CopyIntoMem(unsigned char *ptr, int len)
{
	int c;

	for (c = 0; c < len; c++)
	{
		m_memory[c] = ptr[c];
	}
}

void VTDis::SetBaseAddress(int address)
{
	m_BaseAddress = address;
}

int VTDis::DisassembleLine(int address, char* line)
{
	char			arg[60];
	int				addr;
	unsigned char	opcode;
	unsigned char	op_len;

	// Get opcode from memory
	opcode = get_memory8(address);

	// Determine length of this opcode
	op_len = gLenTable[opcode] & 0x03;

	// Print the address and opcode value to the temporary line buffer
	sprintf(line, "%04XH  ", address + m_BaseAddress);

	// Print the opcode text to the temp line buffer
	strcat(line, gStrTable[opcode]);

	// Check if this opcode has a single byte argument
	if (op_len == 1)
	{
		// Single byte argument
		sprintf(arg, "%02XH", get_memory8(address+1));
		strcat(line, arg);
	}

	// Check if this opcode as a 2 byte argument
	else if (op_len == 2)
	{
		// Double byte argument
		addr = get_memory8(address+1) | (get_memory8(address+2) << 8);
		sprintf(arg, "%04XH", addr);
		strcat(line, arg);

		if (m_WantComments)
		{
			AppendComments(line, opcode, addr);
		}
	}

	return 1+op_len;
}

int VTDis::DisassembleLine(int address, unsigned char opcode, unsigned short operand, char* line)
{
	char			arg[60];
	unsigned char	op_len;

	// Determine length of this opcode
	op_len = gLenTable[opcode] & 0x03;

	// Print the address and opcode value to the temporary line buffer
	sprintf(line, "%04XH  ", address);

	// Print the opcode text to the temp line buffer
	strcat(line, gStrTable[opcode]);

	// Check if this opcode has a single byte argument
	if (op_len == 1)
	{
		// Single byte argument
		sprintf(arg, "%02XH", operand & 0xFF);
		strcat(line, arg);
	}

	// Check if this opcode as a 2 byte argument
	else if (op_len == 2)
	{
		// Double byte argument
		sprintf(arg, "%04XH", operand);
		strcat(line, arg);

		if (m_WantComments)
		{
			AppendComments(line, opcode, operand);
		}
	}

	return 1+op_len;
}
