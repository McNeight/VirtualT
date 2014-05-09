/* disassemble.cpp */

/* $Id: disassemble.cpp,v 1.18 2013/03/15 00:30:37 kpettit1 Exp $ */

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
#include <FL/Fl_Text_Buffer.H>
#include <FL/Fl_Text_Editor.H>
#include <FL/Fl_Return_Button.H>
#include "My_Text_Editor.h"
#include "multieditwin.h"
#include <stdio.h>
#include <string.h>
#include <ctype.h>

#include "FLU/Flu_File_Chooser.h"

#include "VirtualT.h"
#include "m100emu.h"
#include "disassemble.h"
#include "file.h"
#include "io.h"
#include "periph.h"
#include "memedit.h"
#include "cpu.h"
#include "romstrings.h"
#include "cpuregs.h"
#include "memory.h"
#include "fileview.h"
#include "vtobj.h"

// Define extern variables
extern	Fl_Preferences virtualt_prefs;

extern "C"
{
extern	uchar			gMsplanROM[32768];	/* MSPLAN ROM T200 Only */
}

// Declare extern functions
void cb_Ide(Fl_Widget* w, void*) ;
void set_text_size(int t);
void get_hilight_color_prefs(void);

// Our globalton object
VTDis *gpDis;

// List of opcodes that dictate a label is needed for a given address
const unsigned char	gLabelOpcodes[] = {
	0xC3, 0xCD, 0xC2, 0xC4, 0xCA, 0xCC, 0xD2, 0xD4, 0xDA, 0xDC,
	0xE2, 0xE4, 0xEA, 0xEC, 0xF2, 0xF4, 0xFA, 0xFC
};

#define	INST_LXI_B		0x01
#define	INST_LXI_D		0x11
#define	INST_LXI_H		0x21
#define	INST_RET		0xC9
#define	INST_JMP		0xC3
#define	INST_PCHL		0xE9

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

static Std_ROM_Table_t	gEmpty_Tables[] = {
	{ 0x0003, 5,   TABLE_TYPE_STRING },
	{ 0x0004, 4,   TABLE_TYPE_STRING },
	{ -1, 0, 0}
};
static Std_ROM_Addresses_t gEmpty_Vars[] = {
	{ -1, -1 }
};
static Std_ROM_Addresses_t gEmpty_Funs[] = {
	{ 0x0000, R_RESET_VECTOR },            
	{ 0x0008, R_RST_1 },                 
	{ 0x0010, R_RST_2 },                 
	{ 0x0018, R_RST_3 },                 
	{ 0x0020, R_RST_4 },                 
	{ 0x0028, R_RST_5 },                 
	{ 0x002C, R_RST_5_5 },                 
	{ 0x0030, R_RST_6 },                 
	{ 0x0034, R_RST_6_5 },                 
	{ 0x0038, R_RST_7 },                 
	{ 0x003C, R_RST_7_5 },                 
	{ -1, -1 }
};

static RomDescription_t gEmpty_Desc = {
	0,						/* Signature */
	0,						/* Signature address */
	0,                      /* StdRom */

	gEmpty_Tables,			/* Known tables */
	gEmpty_Vars,			/* Known variables */
	gEmpty_Funs,			/* Known functions */

	0,                     /* Address of unsaved BASIC prgm */
	0,                     /* Address of next DO file */
	0,                     /* Start of DO file area */
	0,                     /* Start of CO file area */
	0,                     /* Start of Variable space */
	0,                     /* Start of Array data */
	0,                     /* Pointer to unused memory */
	0,                     /* Address where HIMEM is stored */
	0,                     /* End of RAM for file storage */
	0,                     /* Lowest RAM used by system */
	0,                     /* Start of RAM directory */
	0,                     /* BASIC string	buffer pointer */
	0,						/* BASIC Size */
	0,						/* Keyscan array */
	0,						/* Character generator array */
	0,						/* Location of Year storage */
	0,						/* LCD Buffer storage area */
	0,						/* Label line enable flag (not defined) */

	0,						/* Number of directory entries */
	0,						/* Index of first Dir entry */
	0						/* Address of MS Copyright string */
};

// Callback routine the disassembler
static void close_cb(Fl_Widget* w, void*);
static void cb_dis_what(Fl_Widget* w, void*);
static void cb_setup(Fl_Widget* w, void*);
static void cb_dis_find(Fl_Widget* w, void*);
static void cb_find_next(Fl_Widget* w, void* pOpaque);
static void cb_open_file(Fl_Widget* w, void* pOpaque);
static void cb_save_file(Fl_Widget* w, void* pOpaque);

// Menu items for the disassembler
Fl_Menu_Item gDis_menuitems[] = {
  { "&File",              0, 0, 0, FL_SUBMENU },
	{ "Open...",          FL_CTRL + 'o', cb_open_file, 0 },
	{ "Save...",          FL_CTRL + 's', cb_save_file, 0 },
	{ "Disassemble...",   FL_CTRL + 'd', cb_dis_what, 0, FL_MENU_DIVIDER },
	{ "Setup...",         0, cb_setup, 0, FL_MENU_DIVIDER },
	{ "Close",            FL_CTRL + 'q', close_cb, 0 },
	{ 0 },

  { "&Edit",              0, 0, 0, FL_SUBMENU },
	{ "Find...",          FL_CTRL + 'f',   cb_dis_find, 0 },
	{ "Find Next",        FL_F + 3,        cb_find_next, 0, 0 },
	{ 0 },

  { "&Tools",             0, 0, 0, FL_SUBMENU },
	{ "CPU Registers",         0, cb_CpuRegs },
	{ "Assembler / IDE",       0, cb_Ide },
	{ "Memory Editor",         0, cb_MemoryEditor },
	{ "Peripheral Devices",    0, cb_PeripheralDevices },
	{ "Model T File Viewer",   0, cb_FileView },
	{ 0 },

  { 0 }
};

/*
============================================================================
Closes the disassembler
============================================================================
*/
static void close_cb(Fl_Widget* w, void*)
{
	if (gpDis != NULL)
	{
		// Save the window preferences
		gpDis->SavePrefs();

		// Collapse the window
		collapse_window(gpDis);

		// Now delete the window
		delete gpDis;
		gpDis = 0;
	}
}

/*
============================================================================
Class to manage Disassemble What dialog
============================================================================
*/
class VTDisWhat : public Fl_Window
{
public:
	VTDisWhat(int x, int y, const char *title);

	static void			CbRadioROM(Fl_Widget *w, void *pOpaque);
	static void			CbRadioTargetCO(Fl_Widget *w, void *pOpaque);
	static void			CbRadioHostFile(Fl_Widget *w, void *pOpaque);
	static void			CbHostFileBrowse(Fl_Widget *w, void *pOpaque);
	static void			CbCancel(Fl_Widget *w, void *pOpaque);
	static void			CbOkay(Fl_Widget *w, void *pOpaque);
	void				CbOkay(void);
	void				PopulateTargetFiles(void);

	Fl_Round_Button*	m_pDisRom;
	Fl_Round_Button*	m_pDisTargetCo;
	Fl_Round_Button*	m_pDisHostFile;
	Fl_Choice*			m_whichRom;
	Fl_Choice*			m_whichCO;
	Fl_Input*			m_pHostFilename;
	Fl_Button*			m_pHostFileBrowse;
	model_t_files_t		m_tFiles[100];
	int					m_tCount;
	char				m_sHostFile[512];
};

/*
============================================================================
Class constructor for VTDisWhat
============================================================================
*/
VTDisWhat::VTDisWhat(int x, int y, const char *title) :
	Fl_Window(x, y, title)
{
	// Create items on the dialog
	m_pDisRom = new Fl_Round_Button(20, 20, 60, 20, "ROM");
	m_pDisRom->type(FL_RADIO_BUTTON);
	m_pDisRom->callback(CbRadioROM, this);
	m_pDisRom->value(1);

	// Create a selection box for the ROM to disassemble
	m_whichRom = new Fl_Choice(140, 20, 130, 20, "");
	m_whichRom->add("Main ROM");

	// Add other ROMs based on the current model
	if (gModel == MODEL_T200)
		m_whichRom->add("MS Plan");
	else if (gModel == MODEL_PC8300)
	{
		m_whichRom->add("ROM B");
		m_whichRom->add("ROM C");
	}

	m_whichRom->add("OptROM");
	m_whichRom->value(0);

	// Create a radio button to select in-memory Model T files
	m_pDisTargetCo = new Fl_Round_Button(20, 50, 100, 20, "Target .CO");
	m_pDisTargetCo->type(FL_RADIO_BUTTON);
	m_pDisTargetCo->callback(CbRadioTargetCO, this);

	// Create a selection box for target .CO files
	m_whichCO = new Fl_Choice(140, 50, 130, 20, "");
	m_whichCO->deactivate();
	PopulateTargetFiles();

	// Create a radio button to select external host file
	m_pDisHostFile = new Fl_Round_Button(20, 80, 100, 20, "Host file");
	m_pDisHostFile->type(FL_RADIO_BUTTON);
	m_pDisHostFile->callback(CbRadioHostFile, this);

	// Create an edit field and browse button for the host filename
	m_sHostFile[0] = '\0';
	m_pHostFilename = new Fl_Input(140, 80, 220, 20, "");
	m_pHostFilename->deactivate();
	m_pHostFileBrowse = new Fl_Button(160, 105, 80, 20, "Browse");
	m_pHostFileBrowse->callback(CbHostFileBrowse, this);
	m_pHostFileBrowse->deactivate();

	// Create a Cancel button
	Fl_Button* o = new Fl_Button(100, 160, 60, 30, "Cancel");
	o->callback(CbCancel, this);

	// OK button
	Fl_Return_Button* ro = new Fl_Return_Button(200, 160, 60, 30, "OK");
	ro->callback(CbOkay, this);
}

/*
============================================================================
Populates m_whichCO with target .CO files from the emulation
============================================================================
*/
/*
============================================================================
Populates m_whichCO with target .CO files from the emulation
============================================================================
*/
void VTDisWhat::PopulateTargetFiles(void)
{
	unsigned short		addr1;
	int					dir_index;
	unsigned char		file_type;
	int					c, x;
	char				mt_file[10];

	// Add Model T files to the browser
	addr1 = gStdRomDesc->sDirectory;
	dir_index = 0;	 
	m_tCount = 0;
	while (dir_index < gStdRomDesc->sDirCount)
	{
		// Get the filetype of this file
		file_type = get_memory8(addr1);
		if (file_type != TYPE_CO)
		{
			// Skip files that aren't .CO
			dir_index++;
			addr1 += 11;
			continue;
		}

		c = 0;
		for (x = 0; x < 6; x++)
		{
			if (get_memory8(addr1+3+x) != ' ')
				mt_file[c++] = get_memory8(addr1+3+x);
		}

		mt_file[c++] = '.';
		mt_file[c++] = get_memory8(addr1+3+x++);
		mt_file[c++] = get_memory8(addr1+3+x);
		mt_file[c] = '\0';

		// Add filename to tFiles structure
		strcpy(m_tFiles[m_tCount].name, mt_file);

		// Get address of file
		m_tFiles[m_tCount].address = get_memory16(addr1+1);
		m_tFiles[m_tCount].dir_address = addr1;

		m_whichCO->add(m_tFiles[m_tCount++].name);

		dir_index++;
		addr1 += 11;
	}
}

/*
============================================================================
Callback for the ROM radio button
============================================================================
*/
void VTDisWhat::CbRadioROM(Fl_Widget* w, void* pOpaque)
{
	VTDisWhat*	pWin = (VTDisWhat *) pOpaque;

	// Enable the ROM selection
	pWin->m_whichRom->activate();

	// Disable other controls
	pWin->m_whichCO->deactivate();
	pWin->m_pHostFilename->deactivate();
	pWin->m_pHostFileBrowse->deactivate();
}

/*
============================================================================
Callback for the Target CO radio button
============================================================================
*/
void VTDisWhat::CbRadioTargetCO(Fl_Widget* w, void* pOpaque)
{
	VTDisWhat*	pWin = (VTDisWhat *) pOpaque;

	// Enable the ROM selection
	pWin->m_whichCO->activate();

	// Disable other controls
	pWin->m_whichRom->deactivate();
	pWin->m_pHostFilename->deactivate();
	pWin->m_pHostFileBrowse->deactivate();
}

/*
============================================================================
Callback for the host File radio button
============================================================================
*/
void VTDisWhat::CbRadioHostFile(Fl_Widget* w, void* pOpaque)
{
	VTDisWhat*	pWin = (VTDisWhat *) pOpaque;

	// Enable the ROM selection
	pWin->m_pHostFilename->activate();
	pWin->m_pHostFileBrowse->activate();

	// Disable other controls
	pWin->m_whichCO->deactivate();
	pWin->m_whichRom->deactivate();
}

/*
============================================================================
Callback for the Host file browse button
============================================================================
*/
void VTDisWhat::CbHostFileBrowse(Fl_Widget* w, void* pOpaque)
{
	VTDisWhat*			pWin = (VTDisWhat *) pOpaque;
	Flu_File_Chooser *	fc;
	FILE*				fd;

	// Create a file browser
	fl_cursor(FL_CURSOR_WAIT);
#ifdef __APPLE__
	fc = new Flu_File_Chooser(pWin->m_sHostFile,"*.*",Fl_File_Chooser::CREATE,"File to disassemble");
#else
	fc = new Flu_File_Chooser(pWin->m_sHostFile,"*.*",Fl_File_Chooser::CREATE,"File to disassemble");
#endif
	fl_cursor(FL_CURSOR_DEFAULT);
	fc->preview(0);
	fc->show();

	// Show the file browser and wait for it to go away
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

	// Try to open the input file
	fd = fopen(fc->value(), "rb");
	if (fd == NULL)
	{
		// Error!  File must exist
		fl_alert("Error opening file %s", fl_filename_name(fc->value()));
		delete fc;
		return;
	}

	// Save the filename
	strcpy(pWin->m_sHostFile, fc->value());
	pWin->m_pHostFilename->value(fl_filename_name(pWin->m_sHostFile));
}

/*
============================================================================
Callback for the DisWhat Cancel button
============================================================================
*/
void VTDisWhat::CbCancel(Fl_Widget* w, void* pOpaque)
{
	VTDisWhat*	pWin = (VTDisWhat *) pOpaque;

	// Just hide the window.
	pWin->hide();
}

/*
============================================================================
Callback for the DisWhat Okay button
============================================================================
*/
void VTDisWhat::CbOkay(Fl_Widget* w, void* pOpaque)
{
	VTDisWhat*	pWin = (VTDisWhat *) pOpaque;

	// Call the Okay handler for this window
	pWin->CbOkay();

	// Now hide the window
	pWin->hide();

	// Change the text in the disasembler to show we are disassembling
	gpDis->ShowDisassemblyMessage();

	Fl::wait(0.5);

	gpDis->Disassemble();
}

/*
============================================================================
Callback for the DisWhat Okay button
============================================================================
*/
void VTDisWhat::CbOkay(void)
{
	// Determine what should be disassembled
	if (m_pDisRom->value())
	{
		// Now set the disassembly range
		gpDis->m_StartAddress = 0;
		gpDis->m_EndAddress = ROMSIZE-1;

		// Disassembling a ROM
		if (strcmp(m_whichRom->text(), "Main ROM") == 0)
		{
			// Copy the standard ROM into memory
			gpDis->CopyIntoMem(gSysROM, ROMSIZE);
		}
		else if (strcmp(m_whichRom->text(), "MS Plan") == 0)
		{
			// Copy ROM bank B into memory
			gpDis->CopyIntoMem(gMsplanROM, ROMSIZE);
			gpDis->m_EndAddress = 32767;
		}
		else if (strcmp(m_whichRom->text(), "ROM B") == 0)
		{
			// Copy ROM bank B into memory
			gpDis->CopyIntoMem(&gSysROM[32768], ROMSIZE);
		}
		else if (strcmp(m_whichRom->text(), "ROM C") == 0)
		{
			// Copy ROM bank C into memory
			gpDis->CopyIntoMem(&gSysROM[2*32768], ROMSIZE);
		}
		else if (strcmp(m_whichRom->text(), "OptROM") == 0)
		{
			gpDis->CopyIntoMem(gOptROM, 32768);
			gpDis->m_EndAddress = 32767;
		}
	}

	else if (m_pDisTargetCo->value())
	{
		// Disassembling a target CO file
		unsigned char*	pBuf = new unsigned char[32768];
		int		x, idx, addr, size;

		// Get index of file to disassemble
		idx = m_whichCO->value();
		addr = get_memory16(m_tFiles[idx].address);
		size = get_memory16(m_tFiles[idx].address+2);
		for (x = 0; x < size; x++)
			pBuf[x] = get_memory8(m_tFiles[idx].address + 6 + x);
		gpDis->CopyIntoMem(pBuf, size, addr);
		gpDis->m_StartAddress = addr;
		gpDis->m_EndAddress = addr+size-1;
		delete pBuf;

		// Now copy the standard ROM into the lower memory space
		gpDis->CopyIntoMem(gSysROM, ROMSIZE);
	}

	else if (m_pDisHostFile->value())
	{
		// Copy the standard ROM into the lower memory space

		// Disassembling a host file.  Load the host file into memory
	}
}

/*
============================================================================
Chooses what to actually disassemble
============================================================================
*/
static void cb_dis_what(Fl_Widget* w, void*)
{
	VTDisWhat*				pWin;

	/* Create a new window for the trace configuration */
	pWin = new VTDisWhat(400, 200, "Disassemble What");

	// Show the window
	pWin->show();
	pWin->set_modal();

	// Now wait until OK or Cancel pressed
	while (pWin->visible())
		Fl::wait();

	// Delete the window
	delete pWin;
}

/*
============================================================================
Menu Item Callbacks
============================================================================
*/
void disassembler_cb(Fl_Widget* w, void*) 
{
	const int	wx = 50;
	const int	wy = 50;

	if (gpDis == NULL)
	{
		// Create a Disassembler
		gpDis = new VTDis(wx, wy , "Disassembler");
		gpDis->callback(close_cb);

		// Load user preferences
		gpDis->LoadPrefs();

		// Get the hilight color preferences
		get_hilight_color_prefs();

		// Create a menu for the new window.
		Fl_Menu_Bar *m = new Fl_Menu_Bar(0, 0, wx, MENU_HEIGHT-2);
		m->menu(gDis_menuitems);

		// Create a Text Editor to show/edit the disassembled text
		My_Text_Editor* td = new Fl_Multi_Edit_Window(0, MENU_HEIGHT, wx, wy-MENU_HEIGHT);
		gpDis->SetTextEditor(td);

		// Set the background if black background enabled
		if (gpDis->m_blackBackground)
		{
			td->textcolor(FL_WHITE);
			td->color(FL_BLACK);
			td->selection_color(FL_DARK_BLUE);
			td->textfont(FL_COURIER);
			td->utility_margin_color(5, FL_BLACK);
		}
		else
		{
			td->utility_margin_color(5, FL_WHITE);
			td->textfont(FL_COURIER_BOLD);
		}

		// Set the font size preference
		set_text_size(gpDis->m_fontSize);

		// Create a Text Buffer for the Text Editor to work with
		My_Text_Buffer* tb = new My_Text_Buffer();
		td->buffer(tb);

		// Show the Disassembling text to indicate activity
		tb->append("\n\n\n   Disassembling...");

		// Give the disassembler the text editor to dump it's data into
		gpDis->SetTextViewer(td);

		// Give the disassembler something to disassemble
		gpDis->CopyIntoMem(gSysROM, ROMSIZE);

		gpDis->resizable(m);
		gpDis->resizable(td);

		// End and show the window
		gpDis->end();
		gpDis->show();
		gpDis->CreateFindDlg();

		// Now expand the window to the user prefrence size
		expand_window(gpDis, gpDis->m_x, gpDis->m_y, 
			gpDis->m_w, gpDis->m_h);

		// Show the new window
		Fl::check();

		// Disassemble the code (this should be moved later when dissasembly
		// of .CO programs is supported)
		gpDis->Disassemble();
	}
	else
	{
		// Show the window so it appears on top
		gpDis->show();
	}
}

/*
============================================================================
Disassembler Class Definition
============================================================================
*/
VTDis::VTDis(int x, int y, const char *title) :
	Fl_Double_Window(x, y, title)
{
	m_StartAddress = 0;
	m_EndAddress = ROMSIZE-1;
	m_BaseAddress = 0;
	m_WantComments = 0;
	m_pFindDlg = NULL;
}

/*
============================================================================
Disassembler Class Definition - empty constructor
============================================================================
*/
VTDis::VTDis() :
	Fl_Double_Window(0, 0, "")
{
	m_StartAddress = 0;
	m_EndAddress = ROMSIZE-1;
	m_BaseAddress = 0;
	m_WantComments = 0;
	m_pFindDlg = NULL;
}

/*
============================================================================
Creates a find dialog for the disassembler window
============================================================================
*/
void VTDis::CreateFindDlg(void)
{
	m_pFindDlg = new VTDisFindDlg(this);
}

/*
============================================================================
Disassembler Class Definition - empty constructor
============================================================================
*/
VTDis::~VTDis()
{
	// Delete the find dialog
	if (m_pFindDlg != NULL)
		delete m_pFindDlg;
}

/*
============================================================================
Sets the text viewer object for the disassembler
============================================================================
*/
void VTDis::SetTextViewer(My_Text_Editor *pTextViewer)
{
	m_pTextViewer = pTextViewer;
}

/*
============================================================================
Appends comments from the current StdRom description
to the given line.
============================================================================
*/
void VTDis::AppendComments(char* line, int opcode, int address, int oldSchool)
{
	int		i, x, len, op_len;
	int		margin;

	if (oldSchool)
		margin = 28;
	else
		margin = 39 + (m_includeOpcodes ? 6 : 0);

	// Calculate # spaces to add for fixed line len (make comments line up)
	len = margin - strlen(line);

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

/*
============================================================================
This is the actual disassembler code. It disassembles whatever has been
copied into m_memory
============================================================================
*/
void VTDis::Disassemble()
{
	int		c, x, table;
	char	line[200];
	char	arg[60];
	int		addr, generate;
	int		rst7 = 0;
	int		rst1 = 0;
	int		oldSchool = m_oldSchool;		// Indicates Old school style disassembly
	int		tricked_out;
	int		sig_addr;
	const char*	pSpaces;
	unsigned long	sig;
	unsigned char opcode;
	unsigned char op_len;
	My_Text_Buffer*	tb;

	// Assign the RomDescription table for the model being emulated
	gpDis->m_pRom = gStdRomDesc;

	// Allocate a disassembly type array
	m_pDisType = new DisType_t[65536];

	// Validate we are disassembling a std ROM.  If not, the point to an empty ROM description
	sig_addr = gpDis->m_pRom->sSignatureAddr;
	sig = (unsigned char) m_memory[sig_addr] + 
			((unsigned char) m_memory[sig_addr+1] << 8) +
			((unsigned char) m_memory[sig_addr+2] << 16) +
			((unsigned char) m_memory[sig_addr+3] << 24);
	if (sig != gpDis->m_pRom->dwSignature)
		gpDis->m_pRom = &gEmpty_Desc;

	// Mark all lines as needing no label
	for (x = 0; x < 65536; x++)
	{
		m_pDisType[x].dis_type = DIS_TYPE_UNKNOWN;
		m_pDisType[x].pLabel = NULL;
	}

	// Insert either 5 spaces or zero based on if opcodes are included
	if (m_includeOpcodes)
		pSpaces = "      ";
	else
		pSpaces = "";

	// Assign labels we know we need
	x = 0;
	while (m_pRom->pFuns[x].addr != -1)
	{
		m_pDisType[m_pRom->pFuns[x].addr].dis_type |= DIS_TYPE_LABEL | DIS_TYPE_TABLE | DIS_TYPE_CODE;
		m_pDisType[m_pRom->pFuns[x].addr].pLabel = 
			gDisStrings[m_pRom->pFuns[x].strnum].desc;
		m_pDisType[m_pRom->pFuns[x].addr].idx = m_pRom->pFuns[x].strnum;

		// Increment to next function
		x++;
	}

	// We make two passes.  The 1st pass we throw away as we are just
	// identifying the addresses that need labels
	for (generate = 0; generate < 2; generate++)
	{
		// Create a new text buffer
		tb = new My_Text_Buffer;

		for (c = m_StartAddress; c <= m_EndAddress; c++)
		{
			// Test if we need to print a label for this line
			if (m_pDisType[c].dis_type & DIS_TYPE_LABEL)
			{
				// Add a label to the disassembly
				DisassembleAddLabel(tb, c);
			}

#if 0
			if (m_pDisType[c].dis_type & DIS_TYPE_CODE)
				strcpy(line, "C ");
			else if (m_pDisType[c].dis_type & DIS_TYPE_STRING)
				strcpy(line, "S ");
			else strcpy(line, "  ");
			tb->append(line);
#endif

			// Check if address is a table instead of code
			x = 0;
			table = 0;

			// Check if last instruction was RST7 (that means this byte is a DB)
			if (rst7 == 1)
			{
				if (oldSchool)
					sprintf(line, "%04XH  DB   %02XH\n", c, m_memory[c]);
				else
					sprintf(line, "    %sDB      %02XH\n", pSpaces, m_memory[c]);
				tb->append(line);
				rst7 = 0;
				continue;
			}

			// Check if last instruction was RST1 (that means this byte is a DB)
			if (rst1 == 1)
			{
				if (oldSchool)
					sprintf(line, "%04XH  DB   %02XH\n", c, m_memory[c]);
				else
					sprintf(line, "    %sDB      %02XH\n", pSpaces, m_memory[c]);
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
					// Mark this address as a table address
					table = 1;

					// Mark all address covered by the table entry as covered
					int j, k;
					k = m_pRom->pTables[x].addr + m_pRom->pTables[x].size;
					for (j = m_pRom->pTables[x].addr; j < k; j++)
						m_pDisType[j].dis_type |= DIS_TYPE_TABLE;
					m_pDisType[m_pRom->pTables[x].addr].dis_type |= DIS_TYPE_LABEL;

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
								if (oldSchool)
									sprintf(line, "%04XH  DB   80H or '%c'", next, m_memory[next] & 0x7F);
								else
									sprintf(line, "    %sDB      80H or '%c'", pSpaces, m_memory[next] & 0x7F);
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
								if (oldSchool)
									sprintf(line, "%04XH  DB   00H\n", next);
								else
									sprintf(line, "    %sDB      00H\n", pSpaces);
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
								tb->append(line);
								line[0]=0;
								str_active = 0;
								continue;
							}
							if (!str_active)
							{
								if (m_memory[next] & 0x80)
								{
									if (oldSchool)
										sprintf(line, "%04XH  DB   '%c' OR 80H", next, m_memory[next] & 0x7F);
									else
										sprintf(line, "    %sDB      '%c' OR 80H", pSpaces, m_memory[next] & 0x7F);
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
									if (oldSchool)
										sprintf(line, "%04XH  DB   \"%c", next, m_memory[next]);
									else
										sprintf(line, "    %sDB      \"%c", pSpaces, m_memory[next]);
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
						// Disassemble this block of code as a string
						DisassembleAsString(tb, c, m_pRom->pTables[x].size);
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
									if (oldSchool)
										sprintf(line, "%04XH  DB   %02XH\n", next, m_memory[next]);
									else
										sprintf(line, "    %sDB      %02XH\n", pSpaces, m_memory[next]);
									tb->append(line);
								}
								else					   
								{
									if (oldSchool)
										sprintf(line, "%04XH  DB   \"%c", next, m_memory[next]);
									else
										sprintf(line, "    %sDB      \"%c", pSpaces, m_memory[next]);
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
						int count = 0, itemsPerLine;

						// Determine number of items per line
						itemsPerLine = 3;
						if (!oldSchool)
							itemsPerLine = 2;

						for (next = c; next < last; next += 2)
						{
							if (count == 0)
							{
								if (oldSchool)
									sprintf(line, "%04XH  DW   ", next);
								else
									sprintf(line, "    %sDW      ", pSpaces);
							}

							addr = m_memory[next] | (m_memory[next+1] << 8);
							if (oldSchool)
								sprintf(arg, "%04XH", addr);
							else
							{
								// Display the label
								if (m_pDisType[addr].pLabel)
								{
									// Append the defined label
									strcpy(arg, gDisStrings[m_pDisType[addr].idx].pLabel);
									if (arg[strlen(arg)-1] == ':')
										arg[strlen(arg)-1] = '\0';
								}
								else
								{
									// Append the generated label
									sprintf(arg, "L_%04XH", addr);
									m_pDisType[addr].dis_type |= DIS_TYPE_LABEL;
								}
							}

							// Append the argument to the line
							strcat(line, arg);

							// Append a comma if more items on this line
							if ((next+2 != last) && (count != itemsPerLine))
								strcat(line, ", ");
							else
							{
								strcat(line, "\n");
								if (next+2 == last)
									strcat(line, "\n");
							}

							// Test if we need to start a new line
							if ((count == itemsPerLine) || (next+2 == last))
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
							if (oldSchool)
								sprintf(line, "%04XH  DB   \"%c%c\"\n", next, m_memory[next], m_memory[next+1]);
							else
								sprintf(line, "    %sDB      \"%c%c\"\n", pSpaces, m_memory[next], m_memory[next+1]);
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
							if (oldSchool)
								sprintf(line, "%04XH  DB   \"%c%c%c\"\n", next, m_memory[next], m_memory[next+1],
									m_memory[next+2]);
							else
								sprintf(line, "    %sDB      \"%c%c%c\"\n", pSpaces, m_memory[next], m_memory[next+1],
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
							{
								if (oldSchool)
									sprintf(line, "%04XH  DB   %02XH\n%04XH  DW   %04XH\n", next, m_memory[next],
										next+1, m_memory[next+1] | (m_memory[next+2] << 8));
								else
									sprintf(line, "    %sDB      %02XH\n%04XH  %sDW   %04XH\n", pSpaces, m_memory[next],
										next+1, pSpaces, m_memory[next+1] | (m_memory[next+2] << 8));
							}
							else
							{
								if (oldSchool)
									sprintf(line, "%04XH  DB   '%c'\n%04XH  DW   %04XH\n", next, m_memory[next],
										next+1, m_memory[next+1] | (m_memory[next+2] << 8));
								else
									sprintf(line, "    %sDB      '%c'\n%04XH  %sDW   %04XH\n", pSpaces, m_memory[next],
										next+1, pSpaces, m_memory[next+1] | (m_memory[next+2] << 8));
							}
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
							if (oldSchool)
								sprintf(line, "%04XH  DB   \"%c%c%c%c\"\n", next, m_memory[next],
									m_memory[next+1], m_memory[next+2], m_memory[next+3]);
							else
								sprintf(line, "    %sDB      \"%c%c%c%c\"\n", pSpaces, m_memory[next],
									m_memory[next+1], m_memory[next+2], m_memory[next+3]);
							tb->append(line);
							if (oldSchool)
								sprintf(line, "%04XH  DW   %04XH\n", next+4, m_memory[next+4] | 
									(m_memory[next+5] << 8));
							else
								sprintf(line, "    %sDW      %04XH\n", pSpaces, m_memory[next+4] | 
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
							if (oldSchool)
								sprintf(line, "%04XH  DB   %02XH\n", next, m_memory[next]);
							else
								sprintf(line, "    %sDB      %02XH\n", pSpaces, m_memory[next]);
							tb->append(line);
							if (oldSchool)
								sprintf(line, "%04XH  DW   %04XH\n", next+1, m_memory[next+1] | 
									(m_memory[next+2] << 8));
							else
								sprintf(line, "    %sDW      %04XH\n", pSpaces, m_memory[next+1] | 
									(m_memory[next+2] << 8));
							tb->append(line);

							if (m_memory[next+3] == 0)
							{
								if (oldSchool)
									sprintf(line, "%04XH  DB   %02XH,\"%c%c%c%c%c%c\",%02XH\n", next+3, m_memory[next+3], 
										m_memory[next+4], m_memory[next+5], m_memory[next+6], m_memory[next+7],
										m_memory[next+8], m_memory[next+9], m_memory[next+10]);
								else
									sprintf(line, "    %sDB      %02XH,\"%c%c%c%c%c%c\",%02XH\n", pSpaces, m_memory[next+3], 
										m_memory[next+4], m_memory[next+5], m_memory[next+6], m_memory[next+7],
										m_memory[next+8], m_memory[next+9], m_memory[next+10]);
							}
							else
							{
								if (oldSchool)
									sprintf(line, "%04XH  DB   \"%c%c%c%c%c%c%c\",%02XH\n", next+3, m_memory[next+3], 
										m_memory[next+4], m_memory[next+5], m_memory[next+6], m_memory[next+7],
										m_memory[next+8], m_memory[next+9], m_memory[next+10]);
								else
									sprintf(line, "    %sDB      \"%c%c%c%c%c%c%c\",%02XH\n", pSpaces, m_memory[next+3], 
										m_memory[next+4], m_memory[next+5], m_memory[next+6], m_memory[next+7],
										m_memory[next+8], m_memory[next+9], m_memory[next+10]);
							}
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
							{
								if (oldSchool)
									sprintf(line, "%04XH  DB   ", next);
								else
									sprintf(line, "    %sDB      ", pSpaces);
							}

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
					c += m_pRom->pTables[x].size-1;
					break;
				}

				x++;
			}

			// Was this address identified as a table?  If so, don't disassemble it!
			if (table)
				continue;

			// Test if the disassembly type for this address has been calculated by inference
			if (m_pDisType[c].dis_type & DIS_TYPE_STRING)
			{
				DisassembleAsString(tb, c, m_pDisType[c].idx);
				c += m_pDisType[c].idx - 1;
				continue;
			}

			// Get opcode from memory
			opcode = m_memory[c];

			// Test if this opcode refers to an address which will need a label
			int j;
			for (j = 0; j < sizeof(gLabelOpcodes); j++)
			{
				// Test for label reference
				if (opcode == gLabelOpcodes[j])
				{
					int addr = m_memory[c+1] | (m_memory[c+2] << 8);

					if (opcode < 0x80 && addr < 0x200)
						break;

					// This address will need a label
					m_pDisType[addr].dis_type |= DIS_TYPE_LABEL | DIS_TYPE_CODE;
					break;
				}
			}

			// Determine length of this opcode
			op_len = gLenTable[opcode] & 0x03;
			tricked_out = 0;

			// Print the address and opcode value to the temporary line buffer
			if (oldSchool)
			{
				sprintf(line, "%04XH  (%02XH) ", c, opcode);
				strcat(line, gStrTable[opcode]);
			}
			else
			{
				if (m_includeOpcodes)
					sprintf(line, "    (%02XH) ", opcode);
				else
					strcpy(line, "    ");

				// Print the opcode text to the temp line buffer
				const char*	ptr = gStrTable[opcode];

				// Test if this opcode has been tricked out with the next 
				// opcode to produce a mutli-entry routine (ORI AFH / XRA A)
				if (op_len > 0)
				{
					// Opcode len is not zero. Test if next address needs a label too
					if (m_pDisType[c+1].dis_type & DIS_TYPE_LABEL)
					{
						// This opcode is tricked out.
						tricked_out = TRUE;

						// Disassemble it as a DB instead
						sprintf(line, "    %sDB      %02XH", pSpaces, opcode);
						strcat(line, "                        ; Tricked out ");
					}
				}

				// Copy opcode to line
				int		y = strlen(line);
				while (*ptr != ' ' && *ptr != '\0')
				{
					if (m_opcodesLowercase)
						line[y++] = tolower(*ptr++);
					else
						line[y++] = *ptr++;
				}
				while (y < 11 + (m_includeOpcodes ? 6 : 0))
					line[y++] = ' ';
				while (*ptr != '\0')
					line[y++] = *ptr++;
				line[y] = '\0';
			}

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
				if ((m_pDisType[addr].dis_type & DIS_TYPE_LABEL) && !oldSchool && !tricked_out)
				{
					// Test if we need to build a label or use a ROM label
					if (m_pDisType[addr].pLabel != NULL)
						strcpy(arg, gDisStrings[m_pDisType[addr].idx].pLabel);
					else
						sprintf(arg, "L_%04XH", addr);
				}
				else
					sprintf(arg, "%04XH", addr);
				strcat(line, arg);
			}

			if (!tricked_out)
				AppendComments(line, opcode, addr, oldSchool);

			// Finish off the line
			strcat(line, "\n");

			// Test for LXI opcodes and check the target memory if it is a 
			// string or other special disassembly format
			if (opcode == INST_LXI_B || opcode == INST_LXI_D || opcode == INST_LXI_H)
			{
				// Get the LXI target address
				addr = m_memory[c+1] | (m_memory[c+2] << 8);
				TestForStringArg(addr);
			}

			// Increment memory counter by opcode length
			if (!tricked_out)
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

		// If we aren't on the generate phase, then throw this buffer away
		if (!generate)
		{
			// Delete the text buffer
			delete tb;

			// Now scan through all disassembled addresses and determine which have actual code
			ScanForCodeBlocks();

			// Now scan non-code blocks for strings
			ScanForStrings();
		}
	}

	// Display the text buffer in the text editor
	My_Text_Buffer* old_tb = m_pTextViewer->buffer();
	m_pTextViewer->buffer(tb);

	ResetHilight();

	// Change the title of the window in case there was an opened file
	label("Disassembler");

	delete old_tb;

	// Delete the disassembly type array
	delete[] m_pDisType;
}

/*
============================================================================
Resets the hilight buffer
============================================================================
*/
void VTDis::ResetHilight(void)
{
	// Test if color syntax hilighting enabled
	((Fl_Multi_Edit_Window *) m_pTextViewer)->DisableHl();
	if (m_colorHilight)
	{
		((Fl_Multi_Edit_Window *) m_pTextViewer)->EnableHl();
	}
	else
	{
		// Set the background if black background enabled
		if (gpDis->m_blackBackground)
		{
			m_pTd->textcolor(FL_WHITE);
			m_pTd->color(FL_BLACK);
			m_pTd->selection_color(FL_DARK_BLUE);
			m_pTd->textfont(FL_COURIER);
			m_pTd->utility_margin_color(5, FL_BLACK);
		}
		else
		{
			m_pTd->utility_margin_color(5, FL_WHITE);
			m_pTd->textfont(FL_COURIER_BOLD);
		}

		m_pTd->textsize(m_fontSize);
	}
}

/*
============================================================================
Scans all disassembled addresses and fills in actual code blocks
============================================================================
*/
void VTDis::ScanForCodeBlocks(void)
{
	int		disType = DIS_TYPE_UNKNOWN;
	int		c, j, len;

	// Loop for all disassembled code
	for (c = m_StartAddress; c <= m_EndAddress; c++)
	{
		// Test if the type of disassembly for this address is known
		if ((m_pDisType[c].dis_type & DIS_TYPE_CODE) || (disType == DIS_TYPE_CODE))
		{
			disType = DIS_TYPE_CODE;

			// We are processing a code block.  Fill in this entry
			m_pDisType[c].dis_type |= disType;
			m_pDisType[c].dis_type &= ~DIS_TYPE_STRING;

			// Mark operand bytes as code
			len = gLenTable[m_memory[c]] & 0x03;
			for (j = 1; j <= len; j++)
			{
				m_pDisType[c+j].dis_type |= disType;
				m_pDisType[c+j].dis_type &= ~DIS_TYPE_STRING;
			}

			// Now test this opcode for end of code block
			if (m_memory[c] == INST_JMP || m_memory[c] == INST_RET ||
				m_memory[c] == INST_PCHL)
			{
				// Change to unknown disassembly type
				disType = DIS_TYPE_UNKNOWN;
			}

			// Skip over operand length
			c += len;
		}
		else if (m_pDisType[c].dis_type & (DIS_TYPE_STRING | DIS_TYPE_JUMP))
		{
			// Skip over this string region
			c += m_pDisType[c].idx - 1;
		}
	}
}

/*
============================================================================
Scans all disassembled addresses for unknown type and tests for strings
============================================================================
*/
void VTDis::ScanForStrings(void)
{
	int		c;

	// Loop for all disassembled code
	for (c = m_StartAddress; c <= m_EndAddress; c++)
	{
		if (m_pDisType[c].dis_type == DIS_TYPE_UNKNOWN)
		{
			// Test this block for strings
			TestForStringArg(c);
		}
	}
}

/*
============================================================================
Adds a label to the disassembly
============================================================================
*/
void VTDis::DisassembleAddLabel(My_Text_Buffer*	tb, int addr)
{
	char	line[200];
	char	arg[60];

	// Test if a comment block needed
	if (m_pDisType[addr].pLabel != NULL)
	{
		strcpy(line, "\n; ======================================================\n; ");
		strcat(line, gDisStrings[m_pDisType[addr].idx].desc);
		strcat(line, "\n; ======================================================\n");
		// Add line to Edit Buffer
		tb->append(line);
		if (!m_oldSchool)
		{
			sprintf(arg, "%s:", gDisStrings[m_pDisType[addr].idx].pLabel);
			sprintf(line, "%-39s%s; %04XH\n", arg, m_includeOpcodes ? "      " : "", addr);
			tb->append(line);
		}
	}
	else if (!m_oldSchool)
	{
		// Generate a label based on the address
		sprintf(line, "L_%04XH:\n", addr);
		tb->append(line);
	}
}

/*
============================================================================
Disassembles the specified memory memory region as a string
============================================================================
*/
void VTDis::DisassembleAsString(My_Text_Buffer*	tb, int startAddr, int len)
{
	int		next;
	int		last = startAddr + len;
	int		str_active = 0;
	int		quote_active = 0;
	char	line[200];
	char	arg[60];
	const char* pSpaces;

	if (m_includeOpcodes)
		pSpaces = "      ";
	else
		pSpaces = "";

	for (next = startAddr; next < last; next++)
	{
		// Test if we need to print a label
		if ((m_pDisType[next].dis_type & DIS_TYPE_LABEL) &&
			(next != startAddr))
		{
			// Add a label for this string
			DisassembleAddLabel(tb, next);
		}

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
				if (m_oldSchool)
					sprintf(line, "%04XH  DB   00H\n", next);
				else
					sprintf(line, "    %sDB      00H\n", pSpaces);
				tb->append(line);
				line[0]=0;
			}
			else
			{
				if (iscntrl(m_memory[next]) || m_memory[next] > '~')
				{
					if (m_oldSchool)
						sprintf(line, "%04XH  DB   %02XH", next, m_memory[next]);
					else
						sprintf(line, "    %sDB      %02XH", pSpaces, m_memory[next]);
				}
				else
				{
					if (m_oldSchool)
						sprintf(line, "%04XH  DB   \"%c", next, m_memory[next]);
					else
						sprintf(line, "    %sDB      \"%c", pSpaces, m_memory[next]);
					quote_active = 1;
				}
				str_active = 1;
			}
		}
		else
		{
			if (iscntrl(m_memory[next]) || m_memory[next] > '~')
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

		// Test if we need to terminate the string
		if (((next+1 == last) && (m_memory[next] != 0)) ||
			(strlen(line) > 100) || (m_memory[next] < ' ' && m_memory[next+1] >= ' ' &&
			 m_memory[next] != '\0' && strlen(line) > 16) ||
			 (m_pDisType[next+1].dis_type & DIS_TYPE_LABEL))
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

/*
============================================================================
Tests if the memory at address is a string and marks the m_pDisType as
DIS_TYPE_STRING if it is.
============================================================================
*/
void VTDis::TestForStringArg(int addr)
{
	int		len, lastLen;
	int		c;

	// Test if this address already covered by another disassembly type
	if (m_pDisType[addr].dis_type != DIS_TYPE_UNKNOWN)
		return;

	// Validate the code prior to this is either marked as non-code or is a JMP, RET, PCHL
	if (m_memory[addr-1] != INST_RET && m_memory[addr-3] != INST_JMP &&
		m_pDisType[addr-1].dis_type == DIS_TYPE_UNKNOWN)
			return;

	// Keep track of string length.  If less than 3, we don't count it as a 
	// string unless it is NULL terminated
	len = lastLen = 0;
	for (c = addr; c <= m_EndAddress; c++)
	{
		// Test if this byte is a string byte or not
		if (m_memory[c] != '\x0D' && m_memory[c] != '\x0A' &&
			m_memory[c] != '\x09' && m_memory[c] != '\0' &&
			m_memory[c] != '\x0C' &&
			(m_memory[c] < ' ' || m_memory[c] >= '~'))
		{
			// End of possible string found
			break;
		}

		// Test if this address already has a disassembly type
		if (m_pDisType[c].dis_type != DIS_TYPE_UNKNOWN)
			break;

		// Increment the length
		len++;
		lastLen++;

		// Test if we need to reset the lastLen value
		if (m_memory[c] == '\0')
			lastLen = 0;
	}

	// Test if length is greater than 3 (or 2 with a NULL)
	if (len > 3 || (len == 2 && m_memory[c] == '\0'))
	{
		// Test the length of the last string seen
		if (lastLen <= 2)
			len -= lastLen;

		// Delcare it as a string
		for (c = addr; c < addr + len; c++)
		{
			// Mark this entry as a string disassembly type
			m_pDisType[c].dis_type |= DIS_TYPE_STRING;
			m_pDisType[c].pLabel = NULL;
			m_pDisType[c].idx = len;
		}
	}

	// Test if this LXI address is a string.  If so, mark it as a label also
	if (m_pDisType[addr].dis_type & DIS_TYPE_STRING)
		m_pDisType[addr].dis_type |= DIS_TYPE_LABEL;

}

/*
============================================================================
This copies the code to be disassembled into memory from the given pointer.
============================================================================
*/
void VTDis::CopyIntoMem(unsigned char *ptr, int len, int startAddr)
{
	int c;

	for (c = 0; c < len; c++)
	{
		if (c + startAddr < sizeof(m_memory))
			m_memory[c + startAddr] = ptr[c];
	}
}

/*
============================================================================
This sets the base address of the disassembly when disassembling individual
lines of code.  It is used by the remote debugger.
============================================================================
*/
void VTDis::SetBaseAddress(int address)
{
	m_BaseAddress = address;
}

/*
============================================================================
Disassembles a single line of code from system memory at the given address
using the current memory configuration.
============================================================================
*/
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

/*
============================================================================
Disassembles the given address / opcode / operand data with comments from
the current StdRom description.  This is used by the CPU Registers trace
window to disassemble the trace history.
============================================================================
*/
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

/*
============================================================================
Sets the editor color and font
============================================================================
*/
void VTDis::SetEditorStyle(int blackBackground, int colorHilight, int fontSize)
{
	// Save the values
	m_fontSize = fontSize;
	m_blackBackground = blackBackground;
	m_colorHilight = colorHilight;

	// Now update the text editor
	if (m_pTd != NULL)
	{
		// Set the text color
		if (blackBackground)
		{
			m_pTd->textcolor(FL_WHITE);
			m_pTd->color(FL_BLACK);
			m_pTd->textfont(FL_COURIER);
			m_pTd->utility_margin_color(5, FL_BLACK);
		}
		else
		{
			m_pTd->textcolor(FL_BLACK);
			m_pTd->color(FL_WHITE);
			m_pTd->textfont(FL_COURIER_BOLD);
			m_pTd->utility_margin_color(5, FL_WHITE);
		}

		m_pTd->textsize(fontSize);
	}
}

/*
============================================================================
Load the user preferences
============================================================================
*/
void VTDis::LoadPrefs(void)
{
	Fl_Preferences	g(virtualt_prefs, "Disassembler");

	// Get the window location
	g.get("x", m_x, 50);
	g.get("y", m_y, 50);
	g.get("w", m_w, 600);
	g.get("h", m_h, 400);

	// Validate the x/y coordinates
	if (m_x < 10)
		m_x = 50;
	if (m_y < 10)
		m_y = 50;

	// Get display style preferences
	g.get("FontSize", m_fontSize, 14);
	g.get("OldSchool", m_oldSchool, 0);
	g.get("BlackBackground", m_blackBackground, 1);
	g.get("ColorHilight", m_colorHilight, 1);
	g.get("DisDepth", m_disassemblyDepth, 1000);
	g.get("OpcodesLowercase", m_opcodesLowercase, 0);
	g.get("ShowOpcodes", m_includeOpcodes, 0);
}

/*
============================================================================
Save the user preferences
============================================================================
*/
void VTDis::SavePrefs(void)
{
	Fl_Preferences	g(virtualt_prefs, "Disassembler");

	// Get the window location
	g.set("x", x());
	g.set("y", y());
	g.set("w", w());
	g.set("h", h());

	// Get display style preferences
	g.set("FontSize", m_fontSize);
	g.set("OldSchool", m_oldSchool);
	g.set("BlackBackground", m_blackBackground);
	g.set("ColorHilight", m_colorHilight);
	g.set("DisDepth", m_disassemblyDepth);
	g.set("OpcodesLowercase", m_opcodesLowercase);
	g.set("ShowOpcodes", m_includeOpcodes);
}

/*
============================================================================
Changes the text in the disassembler to indicate an active disassembly
============================================================================
*/
void VTDis::ShowDisassemblyMessage(void)
{
	((Fl_Multi_Edit_Window *) m_pTextViewer)->DisableHl();
	m_pTd->buffer()->replace(0, 99999999, "\n\nDisassembling...");
}

/*
============================================================================
FindDlg routine to show the Disassembler's Find Dialog
============================================================================
*/
void VTDis::FindDlg(void)
{
	// Validate the pointer isn't NULL
	if (m_pFindDlg != NULL)
		m_pFindDlg->m_pFindDlg->show();
}

/*
============================================================================
FindNext routine finds the next item specified by the FindDlg
============================================================================
*/
void VTDis::FindNext(void)
{
	const char *			pFind;

	// Determine what to find and how to find it
	if (m_pFindDlg != NULL)
	{
		// Ensure there is a search string 
		pFind = m_pFindDlg->m_pFind->value();
		if (pFind[0] == '\0')
		{
			m_pFindDlg->m_pFindDlg->show();
			m_pFindDlg->m_pFind->take_focus();
			return;
		}

		// Find the text
//		m_Search = pFind;
		Fl_Multi_Edit_Window* mw = (Fl_Multi_Edit_Window *) m_pTd;

		if (!mw->ForwardSearch(pFind, m_pFindDlg->m_pMatchCase->value()))
		{
			// Save the current position and search from beginning of file
			int pos = mw->insert_position();
			mw->insert_position(0);
			if (!mw->ForwardSearch(pFind, m_pFindDlg->m_pMatchCase->value()))
			{
				// If still not found, report not found
				mw->insert_position(pos);
				fl_alert("Search string %s not found", pFind);
				m_pFindDlg->m_pFind->take_focus();
			}
		}

		// Hide the dialog box
		mw->take_focus();
		m_pFindDlg->m_pFindDlg->hide();
		show();
	}
}

/*
=============================================================
OpenFile routine handles the File->Save File menu item.
This routine opens an existing file and creates an edit
window for the newly opened file.
=============================================================
*/
void VTDis::OpenFile(void)
{
	Flu_File_Chooser*		fc;
	int						count;
	MString					filename;

	fl_cursor(FL_CURSOR_WAIT);
	fc = new Flu_File_Chooser((const char *) m_LastDir, 
		"Assembly Files (*.asm,*.a85,*.txt)", 
		1, "Open File");
	fl_cursor(FL_CURSOR_DEFAULT);
	fc->preview(0);
	fc->show();

	// Wait for user to select a file or escape
	while (fc->visible())
		Fl::wait();

	// Determine if a file was selected or not
	count = fc->count();
	if (count == 0)
	{
		delete fc;
		return;
	}

	// Get the filename from the file chooser
	filename = fc->value();
	filename.Replace('\\', '/');

	// Delete the file chooser
	m_LastDir = fc->get_current_directory();
	if (m_LastDir[m_LastDir.GetLength() - 1] == '/')
		m_LastDir = m_LastDir.Left(m_LastDir.GetLength() - 1);
	delete fc;

	// Try to load the file
	if (!m_pTd->buffer()->loadfile((const char *) filename))
	{
		MString title;
		title.Format("Disassembler - %s", fl_filename_name((const char *) filename));
		label((const char *) title);

		// Reset the color hilighting
		ResetHilight();
	}
}

/*
=============================================================
SaveAs routine handles the File->Save As File menu item.
This routine determines the active edit window and calls
its SaveAs function.
=============================================================
*/
void VTDis::SaveFile(void)
{
	Fl_Multi_Edit_Window*	mw;
	MString					rootpath;
	MString					title;

	// First get a pointer to the active (topmost) window
	mw = (Fl_Multi_Edit_Window*) m_pTd;
	if (mw == NULL)
		return;

	// Validate this is truly a Multi_Edit_Window
	if (strcmp(mw->GetClass()->m_ClassName, "Fl_Multi_Edit_Window") != 0)
		return;

	// Determine root path
	rootpath = path;

	if (mw->Filename() == "")
		mw->SaveAs(rootpath);
	else
		mw->SaveFile(rootpath);
}

/*
=============================================================
draw draws the display
=============================================================
*/
void VTDis::draw(void)
{
#if 0
	if (m_blackBackground)
		fl_color(FL_BLACK);
	else
		fl_color(FL_WHITE);

	fl_rectf(0, 0, w(), h());
#endif
	Fl_Double_Window::draw();
}

/*
============================================================================
Define a local struct for the setup parameters.
============================================================================
*/
typedef struct dis_setup_params
{
	Fl_Input*			pDepth;
	Fl_Input*			pFontSize;
	Fl_Check_Button*	pInverseHilight;
	Fl_Check_Button*	pOldSchool;
	Fl_Check_Button*	pColorHilight;
	Fl_Check_Button*	pOpcodeLowercase;
	Fl_Check_Button*	pIncludeOpcodes;
	char				sDepth[20];
	char				sFontSize[20];
} dis_setup_params_t;

/*
============================================================================
Callback for Trace Setup Ok button
============================================================================
*/
static void cb_setupdlg_OK(Fl_Widget* w, void* pOpaque)
{
	dis_setup_params_t*	p = (dis_setup_params_t *) pOpaque;
	int					fontSize, blackBackground, hilight;

	// Save values
	if (strlen(p->pFontSize->value()) > 0)
	{
		// update the font size
		fontSize = atoi(p->pFontSize->value());
		if (fontSize < 6)
			fontSize = 6;
	}

	// Get Inverse Highlight selection
	blackBackground = p->pInverseHilight->value();

	// Get show as 16-bit selection
	gpDis->m_oldSchool = p->pOldSchool->value();

	// Get hilighting option
	hilight = p->pColorHilight->value();

	// Get lowercase setting
	gpDis->m_opcodesLowercase = p->pOpcodeLowercase->value();

	// Get show opcodes setting
	gpDis->m_includeOpcodes = p->pIncludeOpcodes->value();

	// Set the editor style
	gpDis->SetEditorStyle(blackBackground, hilight, fontSize);
	set_text_size(fontSize);

	// Get the updated trace depth
//	gpDis->m_disassemblyDepth = atoi(p->pDepth->value());

	// Now hide the parent dialog
	w->parent()->hide();

	Fl::check();

	// Re-disassemble
	gpDis->Disassemble();
	
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
Menu callback for disassembly setup
============================================================================
*/
void cb_setup(Fl_Widget* w, void* pOpaque)
{
	Fl_Box*					b;
	Fl_Window*				pWin;
	dis_setup_params_t		p;

	/* Create a new window for the trace configuration */
	pWin = new Fl_Window(300, 230, "Disassembler Setup");

	/* Create input field for trace depth */
//	b = new Fl_Box(20, 20, 100, 20, "Disassembly Depth");
//	b->align(FL_ALIGN_LEFT | FL_ALIGN_INSIDE);
//	p.pDepth = new Fl_Input(160, 20, 80, 20, "");
//	p.pDepth->align(FL_ALIGN_LEFT);
//	sprintf(p.sDepth, "%d", gpDis->m_disassemblyDepth);
//	p.pDepth->value(p.sDepth);

	/* Create input field for font size */
	b = new Fl_Box(20, 20, 100, 20, "Font Size");
	b->align(FL_ALIGN_LEFT | FL_ALIGN_INSIDE);
	p.pFontSize = new Fl_Input(120, 20, 60, 20, "");
	p.pFontSize->align(FL_ALIGN_LEFT);
	sprintf(p.sFontSize, "%d", gpDis->m_fontSize);
	p.pFontSize->value(p.sFontSize);

	/* Create checkbox for hilight style */
	p.pInverseHilight = new Fl_Check_Button(20, 60, 190, 20, "Black background");
	p.pInverseHilight->value(gpDis->m_blackBackground);

	/* Create checkbox for 16-bit register style */
	p.pColorHilight = new Fl_Check_Button(20, 80, 200, 20, "Color syntax hilighting");
	p.pColorHilight->value(gpDis->m_colorHilight);

	/* Create checkbox for 16-bit register style */
	p.pOldSchool = new Fl_Check_Button(20, 100, 200, 20, "Old-school disassembly");
	p.pOldSchool->value(gpDis->m_oldSchool);

	/* Create a checkbox to make opcodes lowercase */
	p.pOpcodeLowercase = new Fl_Check_Button(20, 120, 200, 20, "Make opcode lowercase");
	p.pOpcodeLowercase->value(gpDis->m_opcodesLowercase);

	/* Create a checkbox to include opcodes */
	p.pIncludeOpcodes = new Fl_Check_Button(20, 140, 200, 20, "Show opcode values");
	p.pIncludeOpcodes->value(gpDis->m_includeOpcodes);

	// Cancel button
    { Fl_Button* o = new Fl_Button(80, 190, 60, 30, "Cancel");
      o->callback((Fl_Callback*) cb_setupdlg_cancel);
    }

	// OK button
    { Fl_Return_Button* o = new Fl_Return_Button(160, 190, 60, 30, "OK");
      o->callback((Fl_Callback*) cb_setupdlg_OK, &p);
    }

	// Loop until user presses OK or Cancel
	pWin->show();
	while (pWin->visible())
		Fl::wait();

	// Delete the window
	delete pWin;
}

/*
=======================================================
Callback routine for opening files
=======================================================
*/
static void cb_open_file(Fl_Widget* w, void*)
{
	if (gpDis != NULL)
		gpDis->OpenFile();
}

/*
=======================================================
Callback routine for saving files
=======================================================
*/
static void cb_save_file(Fl_Widget* w, void*)
{
	if (gpDis != NULL)
		gpDis->SaveFile();
}

/*
================================================================================
VTDisFindDlg pops up the Find Dlg for the disassembly window
================================================================================
*/
static void cb_dis_find(Fl_Widget* w, void* pOpaque)
{
	if (gpDis != NULL)
		gpDis->FindDlg();
}

/*
================================================================================
Callback for find next button
================================================================================
*/
static void cb_find_next(Fl_Widget* w, void* pOpaque)
{
	VTDis*	pWin = (VTDis *) pOpaque;

	if (pWin == NULL)
		pWin = gpDis;

	pWin->FindNext();
}

/*
================================================================================
Callback for find close button
================================================================================
*/
static void cb_find_close(Fl_Widget* w, void* pOpaque)
{
	VTDisFindDlg*	pWin = (VTDisFindDlg *) pOpaque;

	pWin->m_pFindDlg->hide();
}

/*
================================================================================
VT_FindDlg routines below.
================================================================================
*/
VTDisFindDlg::VTDisFindDlg(class VTDis* pParent)
{
	// Create Find What combo list
	m_pFindDlg = new Fl_Window(400, 300, "Find");
	Fl_Box *o = new Fl_Box(20, 10, 100, 25, "Find what:");
	o->align(FL_ALIGN_LEFT | FL_ALIGN_INSIDE);
	m_pFind = new Flu_Combo_List(20, 35, 360, 25, "");
	m_pFind->align(FL_ALIGN_LEFT);
	m_pFind->input_callback(cb_find_next, pParent);
	m_pFindDlg->resizable(m_pFind);
	m_pFindDlg->callback(cb_find_close, this);

	// Create Find In choice box
	o = new Fl_Box(20, 70, 100, 20, "Find in:");
	o->align(FL_ALIGN_LEFT | FL_ALIGN_INSIDE);
	m_pFindIn = new Fl_Choice(20, 95, 360, 25, "");
	m_pFindIn->add("Current Window");
	m_pFindIn->add("Current Selection");
	m_pFindIn->value(0);
	m_pFindDlg->resizable(m_pFindIn);

	o = new Fl_Box(20, 135, 360, 110, "Find options");
	o->box(FL_ENGRAVED_BOX);
	o->labeltype(FL_ENGRAVED_LABEL);
	o->align(FL_ALIGN_TOP_LEFT | FL_ALIGN_INSIDE);
	m_pFindDlg->resizable(o);

	m_pBackward = new Fl_Check_Button(40, 160, 120, 25, "Search backward");
	m_pMatchCase = new Fl_Check_Button(40, 185, 100, 25, "Match case");
	m_pWholeWord = new Fl_Check_Button(40, 210, 140, 25, "Match whole word");

	o = new Fl_Box(20, 250, 50, 45, "");
	m_pFindDlg->resizable(o);

	m_pNext = new Flu_Return_Button(160, 255, 100, 25, "Find Next");
	m_pNext->callback(cb_find_next, pParent);

	m_pCancel = new Flu_Button(280, 255, 100, 25, "Close");
	m_pCancel->callback(cb_find_close, this);
	o = new Fl_Box(20, 295, 360, 2, "");
	m_pFindDlg->resizable(o);

	m_pFindDlg->end();
	m_pFindDlg->set_non_modal();
	m_pFindDlg->hide();

	m_pParent = pParent;
}
