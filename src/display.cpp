/* display.cpp */

/* $Id: display.cpp,v 1.18 2008/11/04 02:56:27 kpettit1 Exp $ */

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
#include <FL/Fl_Box.H>
#include <FL/Fl_Help_Dialog.H>
#include <FL/Fl_File_Chooser.H>
#include <FL/Fl_Pixmap.H>
//Added by J. VERNET for pref files
// see also cb_xxxxxx
#include <FL/Fl_Preferences.H>



#include <string.h>
#include <stdio.h>

#include "VirtualT.h"
#include "display.h"
#include "m100emu.h"
#include "io.h"
#include "file.h"
#include "setup.h"
#include "periph.h"
#include "memory.h"
#include "memedit.h"
#include "cpuregs.h"
#include "rememcfg.h"
#include "lpt.h"
#include "fl_action_icon.h"
#include "clock.h"
#include "fileview.h"

extern "C" {
extern RomDescription_t		gM100_Desc;
extern RomDescription_t		gM200_Desc;
extern RomDescription_t		gN8201_Desc;
extern RomDescription_t		gM10_Desc;
//JV
//extern RomDescription_t		gN8300_Desc;
extern RomDescription_t		gKC85_Desc;

extern RomDescription_t		*gStdRomDesc;
extern int					gRomSize;
extern int					gMaintCount;
extern int					gOsDelay;
}

void cb_Ide(Fl_Widget* w, void*) ;

Fl_Window		*MainWin = NULL;
T100_Disp		*gpDisp;
T100_Disp		*gpDebugMonitor;
Fl_Box			*gpCode, *gpGraph, *gpKey, *gpSpeed, *gpCaps, *gpKeyInfo;
Fl_Action_Icon	*gpPrint;
Fl_Box			*gpMap = NULL;
Fl_Menu_Bar		*Menu;
Fl_Preferences	virtualt_prefs(Fl_Preferences::USER, "virtualt.", "virtualt" ); 
char			gsMenuROM[40];
char			gDelayedError[128] = {0};
int				MultFact = 3;
int				DisplayMode = 1;
int				Fullscreen = 0;
int				SolidChars = 0;
int				DispHeight = 64;
int				gRectsize = 2;
int				gXoffset;
int				gYoffset;
int				gSimKey = 0;
extern char*	print_xpm[];
extern Fl_Menu_Item	gPrintMenu[];

extern Fl_Pixmap	gPrinterIcon;

void switch_model(int model);

void setMonitorWindow(Fl_Window* pWin)
{
	if (pWin == 0)
	{
		gpDebugMonitor = 0;
		return;
	}

	if (gpDebugMonitor == 0)   
	{
		if (gModel == MODEL_T200)
		{
			gpDebugMonitor = new T200_Disp(0, 0, 240*2, 128*2);
			(T200_Disp &) *gpDebugMonitor = (T200_Disp &) *gpDisp;
		}
		else
		{
			gpDebugMonitor = new T100_Disp(0, 0, 240*2,64*2);
			// Copy current monitor to debug monitor
			*gpDebugMonitor = *gpDisp;

		}

		gpDebugMonitor->DisplayMode = 0;
		gpDebugMonitor->SolidChars = 1;
		gpDebugMonitor->MultFact = 2;
		gpDebugMonitor->gRectsize = 2;
		gpDebugMonitor->gXoffset = 0;
		gpDebugMonitor->gYoffset = 0;

		pWin->insert(*gpDebugMonitor, 3);
		gpDebugMonitor->show();
	}
}

int T100_Disp::m_simKeys[32];
int	T100_Disp::m_simEventKey;

char *gSpKeyText[] = {
	"SHIFT",
	"CTRL",
	"GRAPH",
	"CODE",
	"",
	"CAPS",
	"",
	"PAUSE",

	"F1",
	"F2",
	"F3",
	"F4",
	"F5",
	"F6",
	"F7",
	"F8",

	"SPACE",
	"BKSP",
	"TAB",
	"ESC",
	"PASTE",
	"LABEL",
	"PRINT",
	"ENTER",

	"",
	"",
	"",
	"",
	"LEFT",
	"RIGHT",
	"UP",
	"DOWN"
};


/*
=======================================================
External Menu Item Callbacks
=======================================================
*/
void disassembler_cb(Fl_Widget* w, void*);

void rspeed(Fl_Widget* w, void*) 
{
	fullspeed=0;
	gMaintCount = 4096;
    virtualt_prefs.set("fullspeed",0);

}
void fspeed(Fl_Widget* w, void*) 
{
	fullspeed=3;
#ifdef WIN32
	gMaintCount = 300000;
#else
	gMaintCount = 131272;
#endif
	virtualt_prefs.set("fullspeed",3);

}

void pf1speed(Fl_Widget* w, void*) 
{
	fullspeed=1;
#ifdef WIN32
	gMaintCount = 16384;
#else
	gMaintCount = 4096;
#endif
	virtualt_prefs.set("fullspeed",1);

}

void pf2speed(Fl_Widget* w, void*) 
{
	fullspeed=2;
#ifdef WIN32
	gMaintCount = 100000;
#else
	gMaintCount = 16384;
#endif
	virtualt_prefs.set("fullspeed",2);

}

void close_disp_cb(Fl_Widget* w, void*)
{
	if (gpDisp != NULL)
	{
		gExitApp = 1;
		gExitLoop = 1;
	}
}

/*
=====================================================================
resize_window:	This function resizes the main window and repositions
				all of the status windows.  The new size of the 
				window is determined by the following values:

				gModel, MultFact, DisplayMode
=====================================================================
*/
void resize_window()
{
	if (gpDisp == NULL)
		return;

	if (gModel == MODEL_T200)
	{
		gpDisp->DispHeight = 128;
		DispHeight = 128;
	}
	else
	{
		gpDisp->DispHeight = 64;
		DispHeight = 64;
	}
	if (Fullscreen)
	{
		MainWin->fullscreen();
		MultFact = min(MainWin->w()/240, MainWin->h()/128);
		gpDisp->MultFact = MultFact;
	}
	else
	{
		if (MainWin->y() <= 0)
			MainWin->fullscreen_off(32, 32, 240*gpDisp->MultFact +
				90*gpDisp->DisplayMode+2, gpDisp->DispHeight*gpDisp->MultFact + 
				50*gpDisp->DisplayMode + MENU_HEIGHT + 22);
		else
			MainWin->fullscreen_off(MainWin->x(), MainWin->y(), 240*gpDisp->MultFact +
				90*gpDisp->DisplayMode+2, gpDisp->DispHeight*gpDisp->MultFact + 
				50*gpDisp->DisplayMode + MENU_HEIGHT + 22);
	}

	if (MainWin->y() <= 0)
	{
		if (!Fullscreen)
			MainWin->resize(32, 32, 240*gpDisp->MultFact +
				90*gpDisp->DisplayMode+2,gpDisp->DispHeight*gpDisp->MultFact + 
				50*gpDisp->DisplayMode + MENU_HEIGHT + 22);
	}
	else
	{
		MainWin->resize(MainWin->x(), MainWin->y(), 240*gpDisp->MultFact +
			90*gpDisp->DisplayMode+2,gpDisp->DispHeight*gpDisp->MultFact + 
			50*gpDisp->DisplayMode + MENU_HEIGHT + 22);
	}

	Menu->resize(0, 0, MainWin->w(), MENU_HEIGHT-2);
	gpDisp->resize(0, MENU_HEIGHT, MainWin->w(), MainWin->h() - MENU_HEIGHT - 20);
	gpGraph->resize(0, MENU_HEIGHT+gpDisp->DispHeight*gpDisp->MultFact + 
		50*gpDisp->DisplayMode+2, 60, 20);
	gpCode->resize(60, MENU_HEIGHT+gpDisp->DispHeight*gpDisp->MultFact + 
		50*gpDisp->DisplayMode+2, 60, 20);
	gpCaps->resize(120, MENU_HEIGHT+gpDisp->DispHeight*gpDisp->MultFact + 
		50*gpDisp->DisplayMode+2, 60, 20);
	gpKey->resize(180, MENU_HEIGHT+gpDisp->DispHeight*gpDisp->MultFact + 
		50*gpDisp->DisplayMode+2, 120, 20);
	gpSpeed->resize(300, MENU_HEIGHT+gpDisp->DispHeight*gpDisp->MultFact + 
		50*gpDisp->DisplayMode+2, 60, 20);
	gpMap->resize(360, MENU_HEIGHT+gpDisp->DispHeight*gpDisp->MultFact + 
		50*gpDisp->DisplayMode+2, 60, 20);
	gpPrint->resize(420, MENU_HEIGHT+gpDisp->DispHeight*gpDisp->MultFact + 
		50*gpDisp->DisplayMode+2, 60, 20);
	gpKeyInfo->resize(480, MENU_HEIGHT+gpDisp->DispHeight*gpDisp->MultFact + 
		50*gpDisp->DisplayMode+2, MainWin->w()-480, 20);
	if (gpDisp->MultFact < 3)
	{
		gpKeyInfo->label("F Keys");
		gpKeyInfo->tooltip("F9:Label  F10:Print  F11:Paste  F12:Pause");
	}
	else
	{
		gpKeyInfo->tooltip("");
		gpKeyInfo->label("F9:Label  F10:Print  F11:Paste  F12:Pause");
	}

	gRectsize = MultFact - (1 - SolidChars);
	if (gRectsize == 0)
		gRectsize = 1;
	gXoffset = 45*DisplayMode+1;
	gYoffset = 25*DisplayMode + MENU_HEIGHT+1;

	gpDisp->gRectsize = gRectsize;
	gpDisp->gXoffset = gXoffset;
	gpDisp->gYoffset = gYoffset;

	Fl::check();
	MainWin->redraw();
}

/* use this Function to display a pop up box */

void show_error (const char *st)
{
	// Check if MainWin created yet.  If not, delay the error
	if (MainWin == NULL)
	{
		strcpy(gDelayedError, st);
		return;
	}

	fl_alert(st);
	
}

void show_message (const char *st)
{
	fl_message(st);
	
}

/*
=======================================================
Menu Item Callbacks
=======================================================
*/
void cb_1x(Fl_Widget* w, void*)
{
	gpDisp->MultFact = 1;
	MultFact = 1;
	Fullscreen = 0;

	virtualt_prefs.set("MultFact",1);
	resize_window();
}
void cb_2x(Fl_Widget* w, void*)
{
	gpDisp->MultFact = 2;
	MultFact = 2;
	Fullscreen = 0;

	virtualt_prefs.set("MultFact",2);
	resize_window();
}
void cb_3x(Fl_Widget* w, void*)
{
	gpDisp->MultFact = 3;
	MultFact = 3;
	Fullscreen = 0;

	virtualt_prefs.set("MultFact",3);
	resize_window();
}
void cb_4x(Fl_Widget* w, void*)
{
	gpDisp->MultFact = 4;
	MultFact = 4;
	Fullscreen = 0;

	virtualt_prefs.set("MultFact",4);
	resize_window();
}
void cb_fullscreen(Fl_Widget* w, void*)
{
	gpDisp->MultFact = 5;
	MultFact = 5;
	Fullscreen = 1;

	virtualt_prefs.set("MultFact",5);
	resize_window();
}
void cb_framed(Fl_Widget* w, void*)
{
	gpDisp->DisplayMode  ^= 1;
	DisplayMode ^= 1;

	virtualt_prefs.set("DisplayMode",gpDisp->DisplayMode);
	resize_window();
}
void cb_solidchars (Fl_Widget* w, void*)
{
	gpDisp->SolidChars  ^= 1;
	SolidChars ^= 1;
	
	virtualt_prefs.set("SolidChars",gpDisp->SolidChars);
	resize_window();
}
void cb_save_basic(Fl_Widget* w, void*)
{
	BasicSaveMode  ^= 1;
    virtualt_prefs.set("BasicSaveMode",BasicSaveMode);
}
void cb_save_co(Fl_Widget* w, void*)
{
	COSaveMode  ^= 1;
    virtualt_prefs.set("COSaveMode",COSaveMode);
}

void cb_reset (Fl_Widget* w, void*)
{
	int 	a;
	
	if (w != NULL)
		a=fl_choice("Really Reset ?", "No", "Yes", NULL);
	else
		a = 1;

	if(a==1) 
	{
		resetcpu();
		show_remem_mode();

		if (gpDisp != NULL)
			gpDisp->Reset();
		if (gpDebugMonitor != 0)
			gpDebugMonitor->Reset();
	}
}

void remote_reset(void)
{
	cb_reset(NULL, NULL);
}

void cb_coldBoot (Fl_Widget* w, void*)
{
	int a;

	if (gReMem)
	{
		if (w != NULL)
			a = fl_choice("Reload OpSys ROM?", "Cancel", "Yes", "No", NULL);
		else
			a = 2;
		if (a == 1)
			reload_sys_rom();
	}
	else
	{
		if (w != NULL)
			a = fl_choice("Really Cold Boot ?", "No", "Yes", NULL);
		else
			a = 2;
	}
	
	if(a != 0) 
	{
		resetcpu();
		reinit_mem();

		show_remem_mode();

		if (gpDisp != NULL)
			gpDisp->Reset();
		if (gpDebugMonitor != 0)
			gpDebugMonitor->Reset();
		fileview_model_changed();
	}
}

void remote_cold_boot(void)
{
	cb_coldBoot(NULL, NULL);
}

void cb_UnloadOptRom (Fl_Widget* w, void*)
{
	char	option_name[32];

	gsOptRomFile[0] = 0;

	// Update user preferences
	get_model_string(option_name, gModel);
	strcat(option_name, "_OptRomFile");
    virtualt_prefs.set(option_name, gsOptRomFile);

	// Clear menu
	strcpy(gsMenuROM, "No ROM loaded");

	// Clear optROM memory
	load_opt_rom();

	// Reset the CPU
	if (w != NULL)
		resetcpu();

}

void cb_M100(Fl_Widget* w, void*)
{
	/* Switch to new model */
	switch_model(MODEL_M100);
}
void cb_M102(Fl_Widget* w, void*)
{
	/* Switch to new model */
	switch_model(MODEL_M102);
}
void cb_M200(Fl_Widget* w, void*)
{
	/* Switch to new model */
	switch_model(MODEL_T200);
}
void cb_M10(Fl_Widget* w, void*)
{
	/* Switch to new model */
	switch_model(MODEL_M10);
}
void cb_PC8201(Fl_Widget* w, void*)
{
	/* Switch to new model */
	switch_model(MODEL_PC8201);
}
void cb_PC8300(Fl_Widget* w, void*)
{
	/* Switch to new model */
	switch_model(MODEL_PC8300);
}

void cb_KC85(Fl_Widget* w, void*)
{
	/* Switch to new model */
	switch_model(MODEL_KC85);
}


#ifdef	__APPLE__
//JV 08/10/05: add an action to choose the working directory
void cb_choosewkdir(Fl_Widget* w, void*)
{
	/* Choose the working directory (ROM, RAM Files... */
	char *ret;
	
	
          ret=ChooseWorkDir();
		  if(ret==NULL) exit(-1); 
			else 
			{
				strcpy(path,ret);
				//#ifdef __unix__
				//strcat(path,"/");
				//#else
				//strcat(path,"\\");
				//#endif
				virtualt_prefs.set("Path",path);
			}
        	
}
//--JV
#endif

/*
=======================================================
cb_help:	This routine displays the Help Subsystem
=======================================================
*/
void cb_help (Fl_Widget* w, void*)
{

    Fl_Help_Dialog *HelpWin;
	//JV 02/02/2008
	
	
	HelpWin = new Fl_Help_Dialog();
	HelpWin->resize(HelpWin->x(), HelpWin->y(), 760, 550);
	
	#ifdef	__APPLE__
	char helppath[255];
	strcpy(helppath,path); strcat(helppath,"/doc/help.html");
	HelpWin->load(helppath);
	#else
	HelpWin->load("doc/help.html");
	#endif
	
	HelpWin->show();

}

/*
==========================================================
cb_about:	This callback routine displays the about box
==========================================================
*/
void cb_about (Fl_Widget* w, void*)
{
  
   Fl_Window* o = new Fl_Window(420, 340);
  
    { Fl_Box* o = new Fl_Box(20, 0, 345, 95, "Virtual T");
      o->labelfont(11);
      o->labelsize(80);
    }
    { Fl_Box* o = new Fl_Box(50, 105, 305, 40, "A Tandy Model 100/102/200 Emulator");
      o->labelfont(8);
      o->labelsize(24);
    }
    { Fl_Box* o = new Fl_Box(30, 150, 335, 35, "written by: "); 
      o->labelfont(8);
      o->labelsize(18);
    }
    { Fl_Box* o = new Fl_Box(30, 170, 335, 35, "Ken Pettit");
      o->labelfont(8);
      o->labelsize(18);
    }
    { Fl_Box* o = new Fl_Box(25, 190, 340, 35, "Stephen Hurd");
      o->labelfont(8);
      o->labelsize(18);
    }
    { Fl_Box* o = new Fl_Box(25, 210, 340, 35, "Jerome Vernet");
      o->labelfont(8);
      o->labelsize(18);
    }
    { Fl_Box* o = new Fl_Box(25, 230, 340, 35, "John Hogerhuis");
      o->labelfont(8);
      o->labelsize(18);
    }
    { Fl_Box* o = new Fl_Box(95, 265, 195, 25, "V "VERSION);
      o->labelfont(8);
      o->labelsize(18);
    }
    { Fl_Box* o = new Fl_Box(35, 295, 320, 25, "This program may be distributed freely ");
      o->labelfont(8);
      o->labelsize(16);
    }
    { Fl_Box* o = new Fl_Box(35, 315, 320, 25, "under the terms of the BSD license");
      o->labelfont(8);
      o->labelsize(16);
    }
    { Fl_Box* o = new Fl_Box(20, 75, 320, 20, "in FLTK");
      o->labelfont(8);
      o->labelsize(18);
    }
    o->end();
    o->show();   
}



Fl_Menu_Item menuitems[] = {
  { "&File", 0, 0, 0, FL_SUBMENU },
	{ "Load file from HD",     0, cb_LoadFromHost, 0 },
	{ "Save file to HD",       0, cb_SaveToHost, 0, 0 },
	{ "Save Options",	       0, 0,  0, FL_SUBMENU | FL_MENU_DIVIDER },
		{ "Save Basic as ASCII",  0, cb_save_basic, (void *) 1, FL_MENU_TOGGLE },
		{ "Save CO as HEX",       0, cb_save_co,    (void *) 2, FL_MENU_TOGGLE },
		{ 0 },
//	{ "Load RAM...",      0,      cb_LoadRam, 0 },
//	{ "Save RAM...",      0,      cb_SaveRam, 0, 0 },
//	{ "RAM Snapshots...", 0,      0, 0, FL_MENU_DIVIDER },
	{ "E&xit",            0, 	  close_disp_cb, 0 },
	{ 0 },

  { "&Emulation",         0, 0,        0, FL_SUBMENU },
	{ "Reset",            0, cb_reset, 0, 0 },
	{ "Cold Boot",        0, cb_coldBoot, 0, FL_MENU_DIVIDER },
	{ "Model",            0, 0,        0, FL_SUBMENU 	 } ,
		{ "M100",   0, cb_M100,   (void *) 1, FL_MENU_RADIO | FL_MENU_VALUE },
		{ "M102",   0, cb_M102,   (void *) 2, FL_MENU_RADIO },
		{ "T200",   0, cb_M200,   (void *) 3, FL_MENU_RADIO },
		{ "PC-8201",  0, cb_PC8201, (void *) 4, FL_MENU_RADIO },
//		{ "PC-8300",  0, cb_PC8300, (void *) 5, FL_MENU_RADIO },
		{ "M10",    0, cb_M10, (void *) 5, FL_MENU_RADIO },
		{ "KC85",    0, cb_KC85, (void *) 6, FL_MENU_RADIO },
		{ 0 },
	{ "Speed", 0, 0, 0, FL_SUBMENU },
		{ "2.4 MHz",     0, rspeed, (void *) 1, FL_MENU_RADIO | FL_MENU_VALUE },
		{ "Very CPU Friendly",0, pf1speed, (void *) 2, FL_MENU_RADIO },
		{ "CPU Friendly",0, pf2speed, (void *) 3, FL_MENU_RADIO },
		{ "Max Speed",   0, fspeed, (void *) 4, FL_MENU_RADIO },
		{ 0 },
	{ "Display", 0, 0, 0, FL_SUBMENU | FL_MENU_DIVIDER},
		{ "1x",  0, cb_1x, (void *) 1, FL_MENU_RADIO },
		{ "2x",  0, cb_2x, (void *) 2, FL_MENU_RADIO },
		{ "3x",  0, cb_3x, (void *) 3, FL_MENU_RADIO | FL_MENU_VALUE},
		{ "4x",  0, cb_4x, (void *) 4, FL_MENU_RADIO },
		{ "Fullscreen",  0, cb_fullscreen, (void *) 5, FL_MENU_RADIO | FL_MENU_DIVIDER},
		{ "Framed",  0, cb_framed, (void *) 1, FL_MENU_TOGGLE|FL_MENU_VALUE },
		{ "Solid Chars",  0, cb_solidchars, (void *) 1, FL_MENU_TOGGLE},
		{ 0 },
	{ "Peripheral Setup...",     0, cb_PeripheralSetup, 0, 0 },
#ifdef	__APPLE__
    { "Directory...",			 0, cb_choosewkdir, 0, 0 }, // JV 08/10/05 menu Directory added
#endif
	{ "Memory Options...",       0, cb_MemorySetup, 0, FL_MENU_DIVIDER },
	{ "Option ROM",              0, 0, 0, FL_SUBMENU },
		{ gsMenuROM,             0, 0, 0, FL_MENU_DIVIDER },
		{ "Load ROM...",         0, cb_LoadOptRom, 0, 0 },
		{ "Unload ROM",          0, cb_UnloadOptRom, 0, 0 },
		{ 0 },
	{ 0 },

  { "&Tools", 0, 0, 0, FL_SUBMENU },
	{ "CPU Registers",         0, cb_CpuRegs },
	{ "Assembler / IDE",       0, cb_Ide},
	{ "Disassembler",          0, disassembler_cb },
	{ "Memory Editor",         0, cb_MemoryEditor },
	{ "ReMem Configuration",   0, cb_RememCfg, 0, 0 },
	{ "Peripheral Devices",    0, cb_PeripheralDevices },
//	{ "Simulation Log Viewer", 0, 0 },
	{ "Model T File Viewer",   0, cb_FileView },
	{ 0 },
  { "&Help", 0, 0, 0, FL_SUBMENU },
	{ "Help", 0, cb_help },
	{ "About VirtualT", 0, cb_about },
	{ 0 },

  { 0 }
};

/*
============================================================================
remote_set_speed:	This function sets the speed of the emulation
============================================================================
*/
void remote_set_speed(int speed)
{
	int		mIndex;
	int		i;
	
	/* Set CPU Speed */
	fullspeed = speed;
	switch (fullspeed)
	{
#ifdef WIN32
		case 0: gMaintCount = 4096; break;
		case 1: gMaintCount = 16384; break;
		case 2: gMaintCount = 100000; break;
		case 3: gMaintCount = 300000; break;
		default: gMaintCount = 4096; break;
#else
		case 0: gMaintCount = 4096; break;
		case 1: gMaintCount = 4096; break;
		case 2: gMaintCount = 16384; break;
		case 3: gMaintCount = 131272; break;
		default: gMaintCount = 4096; break;
#endif
	}

	//==================================================
	// Update Speed menu item if not default value
	//==================================================
	mIndex = 0;
	while (menuitems[mIndex].callback_ != rspeed)
		mIndex++;
	for (i = 0; i < 4; i++)
	{
		if (i == fullspeed)
			menuitems[mIndex+i].flags= FL_MENU_RADIO |FL_MENU_VALUE;
		else
			menuitems[mIndex+i].flags= FL_MENU_RADIO;
	}
}

void init_menus(void)
{
	int 	remem_menu_flag = FL_MENU_INVISIBLE;
	int		mIndex;

	if (gReMem)
	{
		remem_menu_flag = 0;
	}

	// Locate the ReMem Configuration menu item
	mIndex = 0;
	while (menuitems[mIndex].callback_ != cb_RememCfg)
		mIndex++;
	menuitems[mIndex].flags= remem_menu_flag;
}

/*
============================================================================
switch_model:	This function is called by the menu callback routines that 
				manage selection of the current model being emulated. It 
				saves the RAM for the current model, changes the to new 
				model, resizes the display, then reinitializes the system.
============================================================================
*/
void switch_model(int model)
{
	/* Save RAM for current emulation */
	save_ram();
	free_mem();

	// Save time for current model
	save_model_time();

	/* Switch to new model */
	gModel = model;
    virtualt_prefs.set("Model",gModel);

	init_pref();

	/* Load Memory preferences */
	load_memory_preferences();
	init_mem();
	get_model_time();

	gRomSize = 32768;
	/* Set pointer to ROM Description */
	if (gModel == MODEL_T200)
	{
		gStdRomDesc = &gM200_Desc;
		gRomSize = 40960;
	}
	else if (gModel == MODEL_PC8201)
		gStdRomDesc = &gN8201_Desc;
	else if (gModel == MODEL_M10)
	{
		gStdRomDesc = &gM10_Desc;
	}
	else if (gModel == MODEL_KC85)
	{
		gStdRomDesc = &gKC85_Desc;
	} 
//	else if (gModel == MODEL_PC8300)
//	{
//		gStdRomDesc = &gN8300_Desc;
//	} 
	else 
	      
		gStdRomDesc = &gM100_Desc;

	/* Clear the LCD */
	if (gpDisp != NULL)
		gpDisp->Clear();
	if (gpDebugMonitor != 0)
		gpDebugMonitor->Clear();

	delete MainWin;
	init_display();
	show_remem_mode();

	/* Resize the window in case of size difference */
	resize_window();

	/* Re-initialize the CPU */
	init_cpu();

	/* Update Memory Editor window if any */
	cb_MemoryEditorUpdate();

	// Update the File View window if it is open
	fileview_model_changed();
}



/*
=======================================================
T100:Disp:	This is the class construcor
=======================================================
*/
T100_Disp::T100_Disp(int x, int y, int w, int h) :
  Fl_Widget(x, y, w, h)
{
	  int driver, c;

	  for (driver = 0; driver < 10; driver++)
	  {
		  for (int c = 0; c < 256; c++)
			  lcd[driver][c] = 0;
		  top_row[driver] = 0;
	  }
	  for (c = 0; c < 32; c++)
		m_simKeys[c] = 0;

	  m_MyFocus = 0;
	  m_DebugMonitor = 0;

	  MultFact = 3;
	  DisplayMode = 1;
	  SolidChars = 0;
	  DispHeight = 64;
	  gRectsize = 2;
}

const T100_Disp& T100_Disp::operator=(const T100_Disp& srcDisp)
{
	int		driver, c;

	// Copy the LCD data
	if (this != &srcDisp)
	{
		for (driver = 0; driver < 10; driver++)
		{
			for (c = 0; c < 256; c++)
				lcd[driver][c] = srcDisp.lcd[driver][c];
			top_row[driver] = srcDisp.top_row[driver];		 
		}
	}

	return *this;
}

/*
==========================================================
Command:	This function processes commands sent to 
			Model 100 LCD subsystem.
==========================================================
*/
void T100_Disp::Command(int instruction, uchar data)
{
	int driver = instruction;

	if ((data & 0x3F) == 0x3E)
	{
		if (top_row[driver] != data >> 6)
			damage(1, gXoffset, gYoffset, 240*MultFact, 64*MultFact);
		top_row[driver] = data >> 6;
	}
}

/*
=================================================================
Clear:	This routine clears the "LCD"
=================================================================
*/
void T100_Disp::Clear(void)
{
  int driver;

  for (driver = 0; driver < 10; driver++)
  {
	  for (int c = 0; c < 256; c++)
		  lcd[driver][c] = 0;
  }
}

/*
=================================================================
Reset:	This routine Resets the "LCD"
=================================================================
*/
void T100_Disp::Reset(void)
{
  int driver;

  for (driver = 0; driver < 10; driver++)
  {
	  for (int c = 0; c < 256; c++)
		  lcd[driver][c] = 0;
	  top_row[driver] = 0;
  }
  redraw();
}

/*
=================================================================
drawpixel:	This routine is called by the system to draw a single
			black pixel on the "LCD".
=================================================================
*/
// Draw the black pixels on the LCD
__inline void T100_Disp::drawpixel(int x, int y, int color)
{
	// Check if the pixel color is black and draw if it is
	if (color)
		fl_rectf(x*MultFact + gXoffset,y*MultFact + gYoffset,
		gRectsize, gRectsize);
}

/*
=================================================================
draw_static:	This routine draws the static portions of the LCD,
				such as erasing the background, drawing function
				key labls, etc.
=================================================================
*/
void T100_Disp::draw_static()
{
	int c;
	int width;
	int x_pos, inc, start, y_pos;
	int xl_start, xl_end, xr_start, xr_end;
	int	num_labels;

	// Draw gray "screen"
    fl_color(FL_GRAY);
    fl_rectf(x(),y(),w(),h());

	/* Check if the user wants the display "framed" */
	if (DisplayMode == 1)
	{
		// Color for outer border
		fl_color(FL_WHITE);

		// Draw border along the top
		fl_rectf(x(),y(),240*MultFact+92,5);

		// Draw border along the bottom
		fl_rectf(x(),y()+DispHeight*MultFact+45,240*MultFact+92,5);

		// Draw border along the left
		fl_rectf(x(),y()+5,5,DispHeight*MultFact+42);

		// Draw border along the right
		fl_rectf(x()+240*MultFact+87,y()+5,5,DispHeight*MultFact+42);


		// Color for inner border
		fl_color(FL_BLACK);
												    
		// Draw border along the top
		fl_rectf(x()+5,y()+5,240*MultFact+42,20);

		// Draw border along the bottom
		fl_rectf(x()+5,y()+DispHeight*MultFact+27,240*MultFact+82,20);

		// Draw border along the left
		fl_rectf(x()+5,y()+5,40,DispHeight*MultFact+32);

		// Draw border along the right
		fl_rectf(x()+240*MultFact+47,y()+5,40,DispHeight*MultFact+32);


		width = w() - 90;
		num_labels = gModel == MODEL_PC8201 ? 5 : 8;
		inc = width / num_labels;
		start = 28 + width/16 + (4-MultFact)*2;
		fl_color(FL_WHITE);
		fl_font(FL_COURIER,12);
		char  text[3] = "F1";
//		y_pos = h()+20;
		y_pos = y()+DispHeight*MultFact+40;
		xl_start = 2*MultFact;
		xl_end = 7*MultFact;

		xr_start = 12 + 2*MultFact;
		xr_end = 12 + 7*MultFact;

		// Draw function key labels
		for (c = 0; c < num_labels; c++)
		{
			// Draw text
			x_pos = start + inc*c;
			text[1] = c + '1';
			fl_draw(text, x_pos, y_pos);

			if (MultFact != 1)
			{
				// Draw lines to left
				fl_line(x_pos - xl_start, y_pos-2, x_pos - xl_end, y_pos-2);
				fl_line(x_pos - xl_start, y_pos-7, x_pos - xl_end, y_pos-7);
				fl_line(x_pos - xl_end, y_pos-2, x_pos - xl_end, y_pos-7);

				// Draw lines to right
				fl_line(x_pos + xr_start, y_pos-2, x_pos + xr_end, y_pos-2);
				fl_line(x_pos + xr_start, y_pos-7, x_pos + xr_end, y_pos-7);
				fl_line(x_pos + xr_end, y_pos-2, x_pos + xr_end, y_pos-7);
			}
		}
	}

	// Draw printer icon
	//printIcon.draw(420, gpGraph->y());

}

/*
=================================================================
draw:	This routine draws the entire LCD.  This is a member 
		function of Fl_Window.
=================================================================
*/
void T100_Disp::draw()
{
	int x=0;
	int y=0;
	int driver, col, row;
	uchar value;

	/* Draw static background stuff */
	draw_static();

	for (driver = 0; driver < 10; driver++)
	{
		for (row = 0; row < 4; row++)
		{
			for (col = 0; col < 50; col++)
			{
				if (((driver == 4) || (driver == 9)) && ((col & 0x3F)  >= 40))
					continue;

				x=(driver % 5) * 50;
				x+=col;

				y = ((row-top_row[driver]+4) % 4) << 3;
				if (driver > 4)
					y += 32;

				if (x > 240)
					x = x + 1;
				value = lcd[driver][row*64+col];

				// Erase line so it is grey, then fill in with black where needed
				fl_color(FL_GRAY);
				fl_rectf(x*MultFact + gXoffset,y*MultFact + 
					gYoffset,gRectsize,8*MultFact);
				fl_color(FL_BLACK);

				// Draw the black pixels
				drawpixel(x,y++, value&0x01);
				drawpixel(x,y++, value&0x02);
				drawpixel(x,y++, value&0x04);
				drawpixel(x,y++, value&0x08);
				drawpixel(x,y++, value&0x10);
				drawpixel(x,y++, value&0x20);
				drawpixel(x,y++, value&0x40);
				drawpixel(x,y++, value&0x80);
			}
		}
	}
}

void T100_Disp::SetByte(int driver, int col, uchar value)
{
	int x;
	int y;

//	if (driver < 10)
//	{
		// Check if LCD already has the value being requested
		if (lcd[driver][col] == value)
			return;

		//if (driver == 4)
			//printf("%d=%d ", col, value);

		// Load new value into lcd "RAM"
		lcd[driver][col] = value;

		// Check if value is an LCD command
		if ((col&0x3f) > 49)
			return;

		// Calculate X position of byte
		x=(driver % 5) * 50 + (col&0x3F);

		// Calcluate y position of byte
		y = ((((col & 0xC0) >> 6) - top_row[driver] + 4) % 4) * 8;
		if (driver > 4)
			y += 32;

		// Set the display
		window()->make_current();

		fl_color(FL_GRAY);
		fl_rectf(x*MultFact + gXoffset, y*MultFact + 
			gYoffset, gRectsize,MultFact<<3);
		fl_color(FL_BLACK);

		// Draw each pixel of byte
		drawpixel(x,y++,value&0x01);
		drawpixel(x,y++,(value&0x02)>>1);
		drawpixel(x,y++,(value&0x04)>>2);
		drawpixel(x,y++,(value&0x08)>>3);
		drawpixel(x,y++,(value&0x10)>>4);
		drawpixel(x,y++,(value&0x20)>>5);
		drawpixel(x,y++,(value&0x40)>>6);
		drawpixel(x,y++,(value&0x80)>>7);
}
// read Pref File
// J. VERNET

void init_pref(void)
{
	char	option_name[32];

	virtualt_prefs.get("fullspeed",fullspeed,0);
	virtualt_prefs.get("MultFact",MultFact,3);
	virtualt_prefs.get("DisplayMode",DisplayMode,1);
	virtualt_prefs.get("SolidChars",SolidChars,0);
	virtualt_prefs.get("BasicSaveMode",BasicSaveMode,0);
	virtualt_prefs.get("COSaveMode",COSaveMode,0);
	virtualt_prefs.get("Model",gModel, MODEL_M100);
	
	Fullscreen = 0;
	if (MultFact == 5)
		Fullscreen = 1;

	get_model_string(option_name, gModel);
	strcat(option_name, "_OptRomFile");
	virtualt_prefs.get(option_name,gsOptRomFile,"", 256);

	/* Set CPU Speed */
	switch (fullspeed)
	{
#ifdef WIN32
		case 0: gMaintCount = 4096; break;
		case 1: gMaintCount = 16384; break;
		case 2: gMaintCount = 100000; break;
		case 3: gMaintCount = 300000; break;
		default: gMaintCount = 4096; break;
#else
		case 0: gMaintCount = 4096; break;
		case 1: gMaintCount = 4096; break;
		case 2: gMaintCount = 16384; break;
		case 3: gMaintCount = 131272; break;
		default: gMaintCount = 4096; break;
#endif
	}

	if (strlen(gsOptRomFile) == 0)
	{
		strcpy(gsMenuROM, "No ROM loaded");
	}
	else
	{
		strcpy(gsMenuROM, fl_filename_name(gsOptRomFile));
	}
}
// Create the main display window and all children (status, etc.)
void init_display(void)
{
	int	mIndex;
	int	i;
	char		temp[20];

	if (gModel == MODEL_T200)
		DispHeight = 128;
	else
		DispHeight = 64;
	MainWin = new Fl_Window(240*MultFact + 90*DisplayMode+2,DispHeight*MultFact +
		50*DisplayMode + MENU_HEIGHT + 22, "Virtual T");

	// Check if we are running in full screen mode
	if (MultFact == 5)
	{
		MainWin->fullscreen();
		MultFact = min(MainWin->w()/240, MainWin->h()/128);
	}
	else
	{
		if (MainWin->y() <= 0)
			MainWin->fullscreen_off(32, 32, 
			240*MultFact + 90*DisplayMode+2,DispHeight*MultFact +
			50*DisplayMode + MENU_HEIGHT + 22);
	}

	Menu = new Fl_Menu_Bar(0, 0, MainWin->w(), MENU_HEIGHT-2);
	if (gModel == MODEL_T200)
		gpDisp = new T200_Disp(0, MENU_HEIGHT, MainWin->w(), MainWin->h() - MENU_HEIGHT - 20);
	else
		gpDisp = new T100_Disp(0, MENU_HEIGHT, MainWin->w(), MainWin->h() - MENU_HEIGHT - 20);

	MainWin->callback(close_disp_cb);
	Menu->menu(menuitems);
        
	init_menus();

// Treat Values read in pref files
// J. VERNET 
	//==================================================
	// Update Speed menu item if not default value
	//==================================================
	mIndex = 0;
	while (menuitems[mIndex].callback_ != rspeed)
		mIndex++;
	for (i = 0; i < 4; i++)
	{
		if (i == fullspeed)
			menuitems[mIndex+i].flags= FL_MENU_RADIO |FL_MENU_VALUE;
		else
			menuitems[mIndex+i].flags= FL_MENU_RADIO;
	}

	//==================================================
	// Update Display Size from preference
	//==================================================
	mIndex = 0;
	// Find first display size menu item
	while (menuitems[mIndex].callback_ != cb_1x)
		mIndex++;
	mIndex--;

    for(i=1;i<6;i++)
    {
        if(i==MultFact) 
		{
            if(i==5) 
                menuitems[i+mIndex].flags=FL_MENU_RADIO | FL_MENU_VALUE | FL_MENU_DIVIDER;
            else menuitems[i+mIndex].flags=FL_MENU_RADIO | FL_MENU_VALUE;  
		}
		else
		{
            if(i==5) 
                menuitems[i+mIndex].flags=FL_MENU_RADIO | FL_MENU_DIVIDER;
            else menuitems[i+mIndex].flags=FL_MENU_RADIO;  
		}
    }

	//==================================================
	// Update Model selection
	//==================================================
	mIndex = 0;
	// Find first model menu item
	while (menuitems[mIndex].callback_ != cb_M100)
		mIndex++;

    for(i=MODEL_M100;i<=MODEL_PC8300;i++)
    {
        if(i==gModel) 
            if(i==MODEL_PC8300) 
                menuitems[i+mIndex].flags=FL_MENU_RADIO | FL_MENU_VALUE | FL_MENU_DIVIDER;
            else menuitems[i+mIndex].flags=FL_MENU_RADIO | FL_MENU_VALUE;  
        else
            if(i==MODEL_PC8300) 
                menuitems[i+mIndex].flags=FL_MENU_RADIO | FL_MENU_DIVIDER;
            else menuitems[i+mIndex].flags=FL_MENU_RADIO;  
    }

	//==================================================
	// Update DisplayMode parameter 
	//==================================================
    if(DisplayMode==0)
	{
		mIndex = 0;
		while (menuitems[mIndex].callback_ != cb_framed)
			mIndex++;
        menuitems[mIndex].flags=FL_MENU_TOGGLE;
	}

	/*
	==================================================
	Update SolidChars menu item
	==================================================
	*/
    if(SolidChars==1)
	{
		mIndex = 0;
		while (menuitems[mIndex].callback_ != cb_solidchars)
			mIndex++;
        menuitems[mIndex].flags=FL_MENU_TOGGLE|FL_MENU_VALUE;
	}
        
	/*
	==================================================
	Update BasicSaveMode parameter 
	==================================================
	*/
    if(BasicSaveMode==1)
	{
		mIndex = 0;
		while (menuitems[mIndex].callback_ != cb_save_basic)
			mIndex++;
        menuitems[mIndex].flags=FL_MENU_TOGGLE|FL_MENU_VALUE;
	}
    
	/*
	==================================================
	 Update COSaveMode parameter 
	==================================================
	*/
    if(COSaveMode==1)
	{
		mIndex = 0;
		while (menuitems[mIndex].callback_ != cb_save_co)
			mIndex++;
        menuitems[mIndex].flags=FL_MENU_TOGGLE|FL_MENU_VALUE;
	}

	/* 
	========================================
	Create Status boxes for various things
	========================================
	*/
	gpGraph = new Fl_Box(FL_DOWN_BOX,0, MENU_HEIGHT+DispHeight*MultFact +
		50*DisplayMode+2, 60, 20,"GRAPH");
	gpGraph->labelsize(10);
	gpGraph->deactivate();
	gpCode = new Fl_Box(FL_DOWN_BOX,60, MENU_HEIGHT+DispHeight*MultFact +
		50*DisplayMode+2, 60, 20,"CODE");
	gpCode->labelsize(10);
	gpCode->deactivate();
	gpCaps = new Fl_Box(FL_DOWN_BOX,120, MENU_HEIGHT+DispHeight*MultFact +
		50*DisplayMode+2, 60, 20,"CAPS");
	gpCaps->labelsize(10);
	gpCaps->deactivate();
	gpKey = new Fl_Box(FL_DOWN_BOX,180, MENU_HEIGHT+DispHeight*MultFact +
		50*DisplayMode+2, 120, 20,"");
	gpKey->labelsize(10);
	gpSpeed = new Fl_Box(FL_DOWN_BOX,300, MENU_HEIGHT+DispHeight*MultFact +
		50*DisplayMode+2, 60, 20,"");
	gpSpeed->labelsize(10);
	gpMap = new Fl_Box(FL_DOWN_BOX,360, MENU_HEIGHT+DispHeight*MultFact +
		50*DisplayMode+2, 60, 20,"");
	gpMap->labelsize(10);
	gpPrint = new Fl_Action_Icon(420, MENU_HEIGHT+DispHeight*MultFact + 
		50*DisplayMode+2, 60, 20, "Print Menu");
	gpPrint->align(FL_ALIGN_TOP | FL_ALIGN_INSIDE | FL_ALIGN_LEFT);
	gpPrint->set_image(&gPrinterIcon);
	gpPrint->menu(gPrintMenu);
	if (MultFact < 3)
	{
		gpKeyInfo = new Fl_Box(FL_DOWN_BOX,480, MENU_HEIGHT+DispHeight*MultFact +
			50*DisplayMode+2, MainWin->w()-480, 20,
			"F Keys");
		gpKeyInfo->tooltip("F9:Label  F10:Print  F11:Paste  F12:Pause");
	}
	else
		gpKeyInfo = new Fl_Box(FL_DOWN_BOX,480, MENU_HEIGHT+DispHeight*MultFact +
			50*DisplayMode+2, MainWin->w()-480, 20,
			"F9:Label  F10:Print  F11:Paste  F12:Pause");
	gpKeyInfo->labelsize(10);
	gSimKey = 0;

	gXoffset = 45*DisplayMode+1;
	gYoffset = 25*DisplayMode + MENU_HEIGHT+1;
	gRectsize = MultFact - (1 - SolidChars);
	if (gRectsize == 0)
		gRectsize = 1;

	if (gpDisp != 0)
	{
		gpDisp->DispHeight = DispHeight;
		gpDisp->DisplayMode = DisplayMode;
		gpDisp->MultFact = MultFact;
		gpDisp->SolidChars = SolidChars;
		gpDisp->gRectsize = gRectsize;
		gpDisp->gXoffset = gXoffset;
		gpDisp->gYoffset = gYoffset;
	}

	/* End the Window and show it */
	MainWin->end();
	MainWin->show();

	// Set the initial string for ReMem
	show_remem_mode();
	if (gReMem)
	{
		// Check if ReMem is in "Normal" mode or MMU mode
		if (inport(REMEM_MODE_PORT) & 0x01)
		{
			// Read the MMU Map
			sprintf(temp, "Map:%d", (inport(REMEM_MODE_PORT) >> 3) & 0x07);
			display_map_mode(temp);
		}
		else
			display_map_mode("Normal");
	}
        

	/* Check for Fl_Window event */
    Fl::check();

	/* Check if an error occured prior to Window generation ... */
	/* ...and display it now if any */
	if (strlen(gDelayedError) != 0)
	{
		fl_alert(gDelayedError);
		gDelayedError[0] = '\0';
	}

	gpPrint->label("Idle");
}

static char	label[40];
static char mapStr[40];

void display_cpu_speed(void)
{
	sprintf(label, "%4.1f Mhz", cpu_speed + .05);
	Fl::check();
	gpSpeed->label(label);
}

void display_map_mode(char *str)
{
	if (gpMap == NULL)
		return;

	strcpy(mapStr, str);
	Fl::check();
	gpMap->label(mapStr);
}

void drawbyte(int driver, int column, int value)
{
	if (gpDisp != NULL)
		gpDisp->SetByte(driver, column, value);
	if (gpDebugMonitor != 0)
		gpDebugMonitor->SetByte(driver, column, value);


	return;
}

void lcdcommand(int driver, int value)
{
	if (gpDisp != NULL)
		gpDisp->Command(driver, value);
	if (gpDebugMonitor != 0)
		gpDebugMonitor->Command(driver, value);

	return;
}

void power_down()
{
	if (gpDisp != NULL)
		gpDisp->PowerDown();
	if (gpDebugMonitor != 0)
		gpDebugMonitor->PowerDown();

	return;
}

void process_windows_event()
{
	if (gOsDelay)
#ifdef WIN32
		Fl::wait(0.001);
#elif defined(__APPLE__)
        Fl::check();
#else
		Fl::wait(0.00001);
#endif
	else
        Fl::check();

	return;
}

// Called when the Model T shuts its power off
void T100_Disp::PowerDown()
{
	window()->make_current();
	// Clear display
    fl_color(FL_GRAY);
	if (DisplayMode == 1)
	    fl_rectf(45,25+30,w()-90,h()-49);
	else
	    fl_rectf(x(),y(),w(),h());

	//Fl::wait(0);
        Fl::check();
        
	// Print power down message
	char *msg = "System Powered Down";
	char *msg2 = "Press any key to reset";
	int x;
	int col;
	int driver, column;
	int addr;

	addr = gStdRomDesc->sCharTable;

	for (driver = 0; driver < 10; driver++)
		for (col = 0; col < 256; col++)
			lcd[driver][col] = 0;

	// Display first line of powerdown message
    fl_color(FL_BLACK);
	col = 20 - strlen(msg) / 2;
	for (x = 0; x < (int) strlen(msg); x++)
	{
		int mem_index = addr + (msg[x] - ' ')*5;
		column = col++ * 6;
		for (int c = 0; c < 6; c++)
		{
			driver = column / 50;		// 50 pixels per driver
			if (c == 5)
				SetByte(driver, (column % 50) | 0xC0, 0);
			else
				SetByte(driver, (column % 50) | 0xC0, gSysROM[mem_index++]);
			column++;
		}
	}

	// Display 2nd line of powerdown message
	col = 20 - strlen(msg2) / 2;
	for (x = 0; x < (int) strlen(msg2); x++)
	{
		int mem_index = addr + (msg2[x] - ' ')*5;
		column = col++ * 6;
		for (int c = 0; c < 6; c++)
		{
			driver = column / 50 + 5;		// 50 pixels per driver
			if (c == 5)
				SetByte(driver, (column % 50), 0);
			else
				SetByte(driver, (column % 50), gSysROM[mem_index++]);
			column++;
		}
	}
}

void simulate_keydown(int key)
{
	if (gpDisp == NULL)
		return;

	gpDisp->SimulateKeydown(key);
}

void simulate_keyup(int key)
{
	if (gpDisp == NULL)
		return;

	gpDisp->SimulateKeyup(key);
}

void handle_simkey(void)
{
	if (gpDisp == NULL)
		return;

	if (gSimKey)
		gpDisp->HandleSimkey();
}

void T100_Disp::HandleSimkey(void)
{
	int	simkey = gSimKey;
	gSimKey = 0;
	handle(simkey);
}

/*
==========================================================================
siulate_keydown:	Simulates an FL_KEYDOWN event
==========================================================================
*/
void T100_Disp::SimulateKeydown(int key)
{
	int 	c;

	// Loop through array and check if key is already down
	for (c = 0; c < 32; c++)
	{
		if (m_simKeys[c] == key)
			break;
	}

	// Test if key needs to be added to simKeys
	if (c != 32)
	{
		// Find first non-zero entry
		for (c = 0; c < 32; c++)
			if (m_simKeys[c] == 0)
				break;
		if (c != 32)
			m_simKeys[c] = key;
	}
	m_simEventKey = key;
	gSimKey = VT_SIM_KEYDOWN;
}

/*
==========================================================================
siulate_keyup:	Simulates an FL_KEYUP event
==========================================================================
*/
void T100_Disp::SimulateKeyup(int key)
{
	int  c;

	// Loop through array and check if key is already down
	for (c = 0; c < 32; c++)
	{
		if (m_simKeys[c] == key)
		{
			m_simKeys[c] = 0;
			break;
		}
	}

	m_simEventKey = key;
	gSimKey = VT_SIM_KEYUP;
}

/*
==========================================================================
sim_get_key:	Returns the key that caused the event
==========================================================================
*/
int T100_Disp::sim_get_key(int key)
{
	int  c;

	// Loop through array and check if key is already down
	for (c = 0; c < 32; c++)
	{
		if (m_simKeys[c] == key)
			return TRUE;
	}

	return FALSE;
}

/*
==========================================================================
sim_event_key:	Returns the state of a simulated key
==========================================================================
*/
int T100_Disp::sim_event_key(void)
{
	return m_simEventKey;
}

// Handle mouse events, key events, focus events, etc.
char	keylabel[128];
int T100_Disp::handle(int event)
{
	char	keystr[10];
	char	isSpecialKey = 1;
	int		c;
	int		simulated;
	get_key_t	get_key;
	event_key_t	event_key;

	unsigned int key;

	get_key = Fl::get_key;
	event_key = Fl::event_key;
	simulated = FALSE;

	if ((event == VT_SIM_KEYUP) || (event == VT_SIM_KEYDOWN))
	{
		get_key = sim_get_key;
		event_key = sim_event_key;
		simulated = TRUE;

		if (event == VT_SIM_KEYUP)
			event = FL_KEYUP;
		else
			event = FL_KEYDOWN;
	}

	switch (event)
	{
	case FL_FOCUS:
		m_MyFocus = 1;
		break;
	case FL_PUSH:
		m_MyFocus = 1;
		break;

	case FL_UNFOCUS:
		m_MyFocus = 0;
		gSpecialKeys = 0xFFFFFFFF;
		gpCode->deactivate();
		gpGraph->deactivate();
		gpKey->label("");
		for (c = 0; c < 128; c++)
			gKeyStates[c] = 0;
		update_keys();
		break;

	case FL_KEYUP:
		if (!m_MyFocus && !simulated)
			return 1;

		// Get the Key that was pressed
		//key = Fl::event_key();
		key = event_key();
		switch (key)
		{
		case FL_Escape:
			gSpecialKeys |= MT_ESC;
			break;
		case FL_Delete:
			if ((Fl::get_key(FL_Shift_L) | Fl::get_key(FL_Shift_R)) == 0)
				gSpecialKeys |= MT_SHIFT;
		case FL_BackSpace:
			gSpecialKeys |= MT_BKSP;
			break;
		case FL_Tab:
			gSpecialKeys |= MT_TAB;
			break;
		case FL_Enter:
			gSpecialKeys |= MT_ENTER;
			break;
		case FL_Print:
			gSpecialKeys |= MT_PRINT;
			break;
		case FL_F+12:
		case FL_Pause:
			gSpecialKeys |= MT_PAUSE;
			break;
		case FL_Left:
			gSpecialKeys |= MT_LEFT;
			break;
		case FL_Up:
			gSpecialKeys |= MT_UP;
			break;
		case FL_Right:
			gSpecialKeys |= MT_RIGHT;
			break;
		case FL_Down:
			gSpecialKeys |= MT_DOWN;
			break;
		case FL_Shift_L:
		case FL_Shift_R:
			gSpecialKeys |= MT_SHIFT;
			break;
		case FL_Control_L:
		case FL_Control_R:
			gSpecialKeys |= MT_CTRL;
			break;
//		case FL_Caps_Lock:
//			gSpecialKeys |= MT_CAP_LOCK;
//			break;
		case FL_Alt_L:
			gSpecialKeys |= MT_GRAPH;
			gpGraph->deactivate();
			break;
		case FL_Alt_R:
			gSpecialKeys |= MT_CODE;
			gpCode->deactivate();
			break;
		case FL_KP_Enter:
			gSpecialKeys |= MT_ENTER;
			break;
		case FL_Insert:
		case FL_Home:
			if ((Fl::get_key(FL_Control_L) | Fl::get_key(FL_Control_R)) == 0)
				gSpecialKeys |= MT_CTRL;
			if (Fl::get_key(FL_Left) == 0)
				gSpecialKeys |= MT_LEFT;
			break;
		case FL_End:
			if ((Fl::get_key(FL_Control_L) | Fl::get_key(FL_Control_R)) == 0)
				gSpecialKeys |= MT_CTRL;
			if (Fl::get_key(FL_Right) == 0)
				gSpecialKeys |= MT_RIGHT;
			break;
		case FL_Page_Up:
			if ((Fl::get_key(FL_Shift_L) | Fl::get_key(FL_Shift_R)) == 0)
				gSpecialKeys |= MT_SHIFT;
			if (Fl::get_key(FL_Up) == 0)
				gSpecialKeys |= MT_UP;
			break;
		case FL_Page_Down:
			if ((Fl::get_key(FL_Shift_L) | Fl::get_key(FL_Shift_R)) == 0)
				gSpecialKeys |= MT_SHIFT;
			if (Fl::get_key(FL_Down) == 0)
				gSpecialKeys |= MT_DOWN;
			break;
		case FL_F+1:
			gSpecialKeys |= MT_F1;
			break;
		case FL_F+2:
			gSpecialKeys |= MT_F2;
			break;
		case FL_F+3:
			gSpecialKeys |= MT_F3;
			break;
		case FL_F+4:
			gSpecialKeys |= MT_F4;
			break;
		case FL_F+5:
			gSpecialKeys |= MT_F5;
			break;
		case FL_F+6:
			gSpecialKeys |= MT_F6;
			break;
		case FL_F+7:
			gSpecialKeys |= MT_F7;
			break;
		case FL_F+8:
			gSpecialKeys |= MT_F8;
			break;
		case FL_F+9:
			gSpecialKeys |= MT_LABEL;
			break;
		case FL_F+10:
			gSpecialKeys |= MT_PRINT;
			break;
		case FL_F+11:
			gSpecialKeys |= MT_PASTE;
			break;
		default:
			key &= 0x7F;
                       // key=(unsigned int)Fl::event_text();
			// fprintf(stderr,"Et aussi %d\n",key);
			gKeyStates[key] = 0;
			if (key == 0x20)
				gSpecialKeys |= MT_SPACE;

			/*
			=========================================
			=========================================
			*/
			if (gModel == MODEL_PC8201)
			{
				// Handle the '^' key (Shift 6)
				if (key == '6')
				{
					if (Fl::get_key(FL_Shift_L) || Fl::get_key(FL_Shift_R))
					{
						gSpecialKeys &= ~MT_SHIFT;
						gKeyStates['6'] = 0;
						gKeyStates['@'] = 0;
					}
					else
					{
						gSpecialKeys |= MT_SHIFT;
						gKeyStates['@'] = 0;
					}
				}
				// Handle the '"' key (shift ')
				if (key == '\'')
				{
					if (Fl::get_key(FL_Shift_L) || Fl::get_key(FL_Shift_R))
					{
						gSpecialKeys &= ~MT_SHIFT;
						gKeyStates['\''] = 0;
						gKeyStates['2'] = 0;
						gKeyStates['7'] = 0;
					}
					else
					{
						gSpecialKeys |= MT_SHIFT;
						gKeyStates['2'] = 0;
						gKeyStates['7'] = 0;
					}
				}
				// Handle the '+' and '=' keys
				if (key == '=')
				{
					if (Fl::get_key(FL_Shift_L) || Fl::get_key(FL_Shift_R))
					{
						gSpecialKeys &= ~MT_SHIFT;
						gKeyStates[';'] = 0;
						gKeyStates['-'] = 0;
					}
					else
					{
						gSpecialKeys |= MT_SHIFT;
						gKeyStates['-'] = 0;
						gKeyStates[';'] = 0;
					}
				}
				// Handle the '(' key
				if (key == '9')
				{
					gKeyStates['8'] = 0;
					gKeyStates['9'] = 0;
					if (Fl::get_key(FL_Shift_L) || Fl::get_key(FL_Shift_R))
						gSpecialKeys &= ~MT_SHIFT;
					else
						gSpecialKeys |= MT_SHIFT;
				}
				// Handle the ')' key
				if (key == '0')
				{
					gKeyStates['9'] = 0;
					gKeyStates['0'] = 0;
					if (Fl::get_key(FL_Shift_L) || Fl::get_key(FL_Shift_R))
						gSpecialKeys &= ~MT_SHIFT;
					else
						gSpecialKeys |= MT_SHIFT;
				}
				// Handle the '_' key
				if (key == '-')
				{
					gKeyStates['-'] = 0;
					gKeyStates['0'] = 0;
					if (Fl::get_key(FL_Shift_L) || Fl::get_key(FL_Shift_R))
						gSpecialKeys &= ~MT_SHIFT;
					else
						gSpecialKeys |= MT_SHIFT;
				}
				// Handle the ':' key
				if (key == ';')
				{
					gKeyStates[';'] = 0;
					gKeyStates[':'] = 0;
					if (Fl::get_key(FL_Shift_L) || Fl::get_key(FL_Shift_R))
						gSpecialKeys &= ~MT_SHIFT;
					else
						gSpecialKeys |= MT_SHIFT;
				}
				// Handle the '*' key
				if (key == '8')
				{
					gKeyStates['8'] = 0;
					gKeyStates[':'] = 0;
					if (Fl::get_key(FL_Shift_L) || Fl::get_key(FL_Shift_R))
						gSpecialKeys &= ~MT_SHIFT;
					else
						gSpecialKeys |= MT_SHIFT;
				}
				// Handle the '&' key
				if (key == '7')
				{
					gKeyStates['7'] = 0;
					gKeyStates['6'] = 0;
					if (Fl::get_key(FL_Shift_L) || Fl::get_key(FL_Shift_R))
						gSpecialKeys &= ~MT_SHIFT;
					else
						gSpecialKeys |= MT_SHIFT;
				}
				// Handle the '@' key
				if (key == '2')
				{
					gKeyStates['2'] = 0;
					gKeyStates['@'] = 0;
					if (Fl::get_key(FL_Shift_L) || Fl::get_key(FL_Shift_R))
						gSpecialKeys &= ~MT_SHIFT;
					else
						gSpecialKeys |= MT_SHIFT;
				}
			}
			else if (gModel == MODEL_M10)
			{
				// Handle the '@' key
				if (key == '2')
				{
					gKeyStates['2'] = 0;
					gKeyStates['@'] = 0;
					gSpecialKeys |= MT_SHIFT;
				}
				// Handle the '#' key
				if (key == '3')
				{
					gSpecialKeys |= MT_GRAPH;
					gKeyStates['h'] = 0;
				}
				// Handle the '^' key (Shift 6)
				if (key == '6')
				{
					if (Fl::get_key(FL_Shift_L) || Fl::get_key(FL_Shift_R))
					{
						gSpecialKeys &= ~MT_SHIFT;
						gKeyStates['6'] = 0;
						gKeyStates['^'] = 0;
					}
					else
					{
						gSpecialKeys |= MT_SHIFT;
						gKeyStates['^'] = 0;
					}
				}
				if (key == '`')
				{
					gKeyStates['@'] = 0;
					gKeyStates['^'] = 0;
				}
				// Handle the '&' key
				if (key == '7')
				{
					gKeyStates['7'] = 0;
					gKeyStates['6'] = 0;
					if (Fl::get_key(FL_Shift_L) || Fl::get_key(FL_Shift_R))
						gSpecialKeys &= ~MT_SHIFT;
					else
						gSpecialKeys |= MT_SHIFT;
				}
				// Handle the '(' key
				if (key == '9')
				{
					gKeyStates['8'] = 0;
					gKeyStates['9'] = 0;
					if (Fl::get_key(FL_Shift_L) || Fl::get_key(FL_Shift_R))
						gSpecialKeys &= ~MT_SHIFT;
					else
						gSpecialKeys |= MT_SHIFT;
				}
				// Handle the ')' key
				if (key == '0')
				{
					gKeyStates['9'] = 0;
					gKeyStates['0'] = 0;
					if (Fl::get_key(FL_Shift_L) || Fl::get_key(FL_Shift_R))
						gSpecialKeys &= ~MT_SHIFT;
					else
						gSpecialKeys |= MT_SHIFT;
				}
				// Handle the '*' key
				if (key == '8')
				{
					gKeyStates['8'] = 0;
					gKeyStates[':'] = 0;
					if (Fl::get_key(FL_Shift_L) || Fl::get_key(FL_Shift_R))
						gSpecialKeys &= ~MT_SHIFT;
					else
						gSpecialKeys |= MT_SHIFT;
				}
				// Handle the '_' key
				if (key == '-')
				{
					if (Fl::get_key(FL_Shift_L) || Fl::get_key(FL_Shift_R))
					{
						gKeyStates['-'] = 0;
						gKeyStates['0'] = 0;
						gSpecialKeys |= MT_SHIFT;
					}
				}
				if (key == '=')
				{
					gKeyStates[';'] = 0;
					gKeyStates['-'] = 0;
					gSpecialKeys |= MT_SHIFT;
				}
				// Handle the ':' key
				if (key == ';')
				{
					gKeyStates[';'] = 0;
					gKeyStates[':'] = 0;
					if (Fl::get_key(FL_Shift_L) || Fl::get_key(FL_Shift_R))
						gSpecialKeys &= ~MT_SHIFT;
					else
						gSpecialKeys |= MT_SHIFT;
				}
				// Handle the '"' key (shift ')
				if (key == '\'')
				{
					if (Fl::get_key(FL_Shift_L) || Fl::get_key(FL_Shift_R))
					{
						gSpecialKeys &= ~MT_SHIFT;
						gKeyStates['\''] = 0;
						gKeyStates['2'] = 0;
						gKeyStates['7'] = 0;
					}
					else
					{
						gSpecialKeys |= MT_SHIFT;
						gKeyStates['2'] = 0;
						gKeyStates['7'] = 0;
					}
				}
			}
			else
			{
				if (key == ']')
				{
					if (Fl::get_key(FL_Shift_L) || Fl::get_key(FL_Shift_R))
					{
						gSpecialKeys |= MT_GRAPH;
						gSpecialKeys &= ~MT_SHIFT;
						gKeyStates[']'] = 0;
						gKeyStates['0'] = 0;
					}
					else
					{
						gSpecialKeys |= MT_SHIFT;
						gKeyStates['['] = 0;
					}
				}
				if (key == '[')
				{
					if (Fl::get_key(FL_Shift_L) || Fl::get_key(FL_Shift_R))
					{
						gSpecialKeys |= MT_GRAPH;
						gSpecialKeys &= ~MT_SHIFT;
						gKeyStates['['] = 0;
						gKeyStates['9'] = 0;
					}
				}
				if (key == '`')
				{
					gSpecialKeys |= MT_GRAPH;
					gKeyStates['['] = 0;
				}
				if (key == '\\')
				{
					if (Fl::get_key(FL_Shift_L) || Fl::get_key(FL_Shift_R))
					{
						gSpecialKeys |= MT_GRAPH;
						gKeyStates[';'] = 0;
					}
					else
					{
						gSpecialKeys |= MT_GRAPH;
						gKeyStates['-'] = 0;
					}
				}
			}

			isSpecialKey = 0;
			break;
		}
		update_keys();
		break;

	case FL_KEYDOWN:
		if (!m_MyFocus && !simulated)
			return 1;

		if (ioBA & 0x10)
		{
			resetcpu();
			break;
		}

		// Get the Key that was pressed
		key = event_key();
		switch (key)
		{
		case FL_Escape:
			gSpecialKeys &= ~MT_ESC;
			break;
		case FL_Delete:
			gSpecialKeys &= ~MT_SHIFT;
		case FL_BackSpace:
			gSpecialKeys &= ~MT_BKSP;
			break;
		case FL_Tab:
			gSpecialKeys &= ~MT_TAB;
			break;
		case FL_Enter:
			gSpecialKeys &= ~MT_ENTER;
			break;
		case FL_Print:
			gSpecialKeys &= ~MT_PRINT;
			break;
		case FL_F+12:
		case FL_Pause:
			gSpecialKeys &= ~MT_PAUSE;
			break;
		case FL_Left:
			gSpecialKeys &= ~MT_LEFT;
			break;
		case FL_Up:
			gSpecialKeys &= ~MT_UP;
			break;
		case FL_Right:

			gSpecialKeys &= ~MT_RIGHT;
			break;
		case FL_Down:
			gSpecialKeys &= ~MT_DOWN;
			break;
		case FL_Shift_L:
		case FL_Shift_R:
			gSpecialKeys &= ~MT_SHIFT;
			break;
		case FL_Control_L:
		case FL_Control_R:
			gSpecialKeys &= ~MT_CTRL;
			break;
		case FL_Caps_Lock:
			gSpecialKeys ^= MT_CAP_LOCK;
			if (gSpecialKeys & MT_CAP_LOCK)
				gpCaps->deactivate();
			else
				gpCaps->activate();
			break;
		case FL_Alt_L:
			gSpecialKeys &= ~MT_GRAPH;
			gpGraph->activate();
			break;
		case FL_Alt_R:
			gSpecialKeys &= ~MT_CODE;
			gpCode->activate();
			break;
		case FL_KP_Enter:
			gSpecialKeys &= ~MT_ENTER;
			break;
		case FL_Insert:
		case FL_Home:
			gSpecialKeys &= ~(MT_CTRL | MT_LEFT);
			break;
		case FL_End:
			gSpecialKeys &= ~(MT_CTRL | MT_RIGHT);
			break;
		case FL_Page_Up:
			gSpecialKeys &= ~(MT_SHIFT | MT_UP);
			break;
		case FL_Page_Down:
			gSpecialKeys &= ~(MT_SHIFT | MT_DOWN);
			break;
		case FL_F+1:
			gSpecialKeys &= ~MT_F1;
			break;
		case FL_F+2:
			gSpecialKeys &= ~MT_F2;
			break;
		case FL_F+3:
			gSpecialKeys &= ~MT_F3;
			break;
		case FL_F+4:
			gSpecialKeys &= ~MT_F4;
			break;
		case FL_F+5:
			gSpecialKeys &= ~MT_F5;
			break;
		case FL_F+6:
			gSpecialKeys &= ~MT_F6;
			break;
		case FL_F+7:
			gSpecialKeys &= ~MT_F7;
			break;
		case FL_F+8:
			gSpecialKeys &= ~MT_F8;
			break;
		case FL_F+9:
			gSpecialKeys &= ~MT_LABEL;
			break;
		case FL_F+10:
			gSpecialKeys &= ~MT_PRINT;
			break;
		case FL_F+11:
			gSpecialKeys &= ~MT_PASTE;
			break;
		default:
			key &= 0x7F;
			gKeyStates[key] = 1;
			if (key == 0x20)
			{
				gSpecialKeys &= ~MT_SPACE;
			}
			if (gModel == MODEL_PC8201)
			{
				// Deal with special keys for the PC-8201

				// Handle the '^' key  (Shift 6)
				if (key == '6')
				{
					if (Fl::get_key(FL_Shift_L) || Fl::get_key(FL_Shift_R))
					{
						gKeyStates['6'] = 0;
						gKeyStates['@'] = 1;
					}
					else
					{
						gSpecialKeys |= MT_SHIFT;
						gKeyStates['@'] = 0;
					}
				}
				// Handle the '"' key  (Shift ')
				if (key == '\'')
				{
					if (Fl::get_key(FL_Shift_L) || Fl::get_key(FL_Shift_R))
					{
						gKeyStates['\''] = 0;
						gKeyStates['2'] = 1;
					}
					else
					{
						gSpecialKeys &= ~MT_SHIFT;
						gKeyStates['2'] = 0;
						gKeyStates['7'] = 1;
					}
				}
				// Handle the '+' and '=' keys
				if (key == '=')
				{
					if (Fl::get_key(FL_Shift_L) || Fl::get_key(FL_Shift_R))
					{
						gKeyStates['='] = 0;
						gKeyStates[';'] = 1;
					}
					else
					{
						gSpecialKeys &= ~MT_SHIFT;
						gKeyStates['-'] = 1;
						gKeyStates['='] = 0;
					}
				}
				// Handle the '(' key
				if (key == '9')
				{
					if (Fl::get_key(FL_Shift_L) || Fl::get_key(FL_Shift_R))
					{
						gKeyStates['9'] = 0;
						gKeyStates['8'] = 1;
					}
				}
				// Handle the '(' key
				if (key == '0')
				{
					if (Fl::get_key(FL_Shift_L) || Fl::get_key(FL_Shift_R))
					{
						gKeyStates['0'] = 0;
						gKeyStates['9'] = 1;
					}
				}
				// Handle the '_' key
				if (key == '-')
				{
					if (Fl::get_key(FL_Shift_L) || Fl::get_key(FL_Shift_R))
					{
						gKeyStates['-'] = 0;
						gKeyStates['0'] = 1;
					}
				}
				// Handle the ':' key
				if (key == ';')
				{
					if (Fl::get_key(FL_Shift_L) || Fl::get_key(FL_Shift_R))
					{				 
						gSpecialKeys |= MT_SHIFT;
						gKeyStates[';'] = 0;
						gKeyStates[':'] = 1;
					}
				}
				// Handle the '*' key
				if (key == '8')
				{
					if (Fl::get_key(FL_Shift_L) || Fl::get_key(FL_Shift_R))
					{				 
						gKeyStates['8'] = 0;
						gKeyStates[':'] = 1;
					}
				}
				// Handle the '&' key
				if (key == '7')
				{
					if (Fl::get_key(FL_Shift_L) || Fl::get_key(FL_Shift_R))
					{				 
						gKeyStates['7'] = 0;
						gKeyStates['6'] = 1;
					}
				}
				// Handle the '@' key
				if (key == '2')
				{
					if (Fl::get_key(FL_Shift_L) || Fl::get_key(FL_Shift_R))
					{				 
						gSpecialKeys |= MT_SHIFT;
						gKeyStates['2'] = 0;
						gKeyStates['@'] = 1;
					}
				}
			}
			else if (gModel == MODEL_M10)
			{
				// Handle the '@' key
				if (key == '2')
				{
					if (Fl::get_key(FL_Shift_L) || Fl::get_key(FL_Shift_R))
					{				 
						gSpecialKeys |= MT_SHIFT;
						gKeyStates['2'] = 0;
						gKeyStates['@'] = 1;
					}
				}
				// Handle the '#' key
				if (key == '3')
				{
					if (Fl::get_key(FL_Shift_L) || Fl::get_key(FL_Shift_R))
					{				 
						gSpecialKeys &= ~MT_GRAPH;
						gKeyStates['3'] = 0;
						gKeyStates['h'] = 1;
					}
				}
				// Handle the '^' key  (Shift 6)
				if (key == '6')
				{
					if (Fl::get_key(FL_Shift_L) || Fl::get_key(FL_Shift_R))
					{
						gSpecialKeys |= MT_SHIFT;
						gKeyStates['6'] = 0;
						gKeyStates['^'] = 1;
					}
					else
					{
						gKeyStates['^'] = 0;
					}
				}
				if (key == '`')
				{
					if (Fl::get_key(FL_Shift_L) || Fl::get_key(FL_Shift_R))
					{
						gSpecialKeys &= ~MT_SHIFT;
						gKeyStates['`'] = 0;
						gKeyStates['^'] = 1;
					}
					else
					{
						gSpecialKeys &= ~MT_SHIFT;
						gKeyStates['`'] = 0;
						gKeyStates['@'] = 1;
					}
				}
				// Handle the '&' key
				if (key == '7')
				{
					if (Fl::get_key(FL_Shift_L) || Fl::get_key(FL_Shift_R))
					{				 
						gKeyStates['7'] = 0;
						gKeyStates['6'] = 1;
					}
				}
				// Handle the '(' key
				if (key == '9')
				{
					if (Fl::get_key(FL_Shift_L) || Fl::get_key(FL_Shift_R))
					{
						gKeyStates['9'] = 0;
						gKeyStates['8'] = 1;
					}
				}
				// Handle the '(' key
				if (key == '0')
				{
					if (Fl::get_key(FL_Shift_L) || Fl::get_key(FL_Shift_R))
					{
						gKeyStates['0'] = 0;
						gKeyStates['9'] = 1;
					}
				}
				// Handle the '*' key
				if (key == '8')
				{
					if (Fl::get_key(FL_Shift_L) || Fl::get_key(FL_Shift_R))
					{				 
						gKeyStates['8'] = 0;
						gKeyStates[':'] = 1;
					}
				}
				// Handle the '_' key
				if (key == '-')
				{
					if (Fl::get_key(FL_Shift_L) || Fl::get_key(FL_Shift_R))
					{
						gKeyStates['-'] = 0;
						gKeyStates['0'] = 1;
					}
				}
				if (key == '=')
				{
					if (Fl::get_key(FL_Shift_L) || Fl::get_key(FL_Shift_R))
					{
						gSpecialKeys &= ~MT_SHIFT;
						gKeyStates['='] = 0;
						gKeyStates[';'] = 1;
					}
					else
					{
						gSpecialKeys &= ~MT_SHIFT;
						gKeyStates['='] = 0;
						gKeyStates['-'] = 1;
					}
				}
				// Handle the ':' key
				if (key == ';')
				{
					if (Fl::get_key(FL_Shift_L) || Fl::get_key(FL_Shift_R))
					{				 
						gSpecialKeys |= MT_SHIFT;
						gKeyStates[';'] = 0;
						gKeyStates[':'] = 1;
					}
				}
				// Handle the '"' key  (Shift ')
				if (key == '\'')
				{
					if (Fl::get_key(FL_Shift_L) || Fl::get_key(FL_Shift_R))
					{
						gKeyStates['\''] = 0;
						gKeyStates['2'] = 1;
					}
					else
					{
						gSpecialKeys &= ~MT_SHIFT;
						gKeyStates['2'] = 0;
						gKeyStates['7'] = 1;
					}
				}
			}
			else
			{
				if (key == ']')
				{
					if (Fl::get_key(FL_Shift_L) || Fl::get_key(FL_Shift_R))
					{
						gSpecialKeys &= ~MT_GRAPH;
						gSpecialKeys |= MT_SHIFT;
						gKeyStates[']'] = 0;
						gKeyStates['0'] = 1;
					}
					else
					{
						gSpecialKeys &= ~MT_SHIFT;
						gKeyStates['['] = 1;
						gKeyStates[key] = 0;
					}
				}
				if (key == '[')
				{
					if (Fl::get_key(FL_Shift_L) || Fl::get_key(FL_Shift_R))
					{
						gSpecialKeys &= ~MT_GRAPH;
						gSpecialKeys |= MT_SHIFT;
						gKeyStates['['] = 0;
						gKeyStates['9'] = 1;
					}
				}
				if (key == '`')
				{
					if (Fl::get_key(FL_Shift_L) || Fl::get_key(FL_Shift_R))
					{
						gSpecialKeys &= ~MT_GRAPH;
						gSpecialKeys &= ~MT_SHIFT;
						gKeyStates['`'] = 0;
						gKeyStates['['] = 1;
					}
					else
					{
						gSpecialKeys &= ~MT_GRAPH;
						gKeyStates['`'] = 0;
						gKeyStates['['] = 1;
					}
				}
				if (key == '\\')
				{
					if (Fl::get_key(FL_Shift_L) || Fl::get_key(FL_Shift_R))
					{
						gSpecialKeys &= ~MT_GRAPH;
						gSpecialKeys &= ~MT_SHIFT;
						gKeyStates['\\'] = 0;
						gKeyStates[';'] = 1;
					}
					else
					{
						gSpecialKeys &= ~MT_GRAPH;
						gKeyStates['-'] = 1;
						gKeyStates['\\'] = 0;
					}
				}
			}
			isSpecialKey = 0;
			break;
		}
		update_keys();
		break;

	}

	// Display keystroke info on status line of display
	if (event == FL_KEYUP || event == FL_KEYDOWN)
	{
		// Start with empty string
		keylabel[0] = 0;

		// Add text for special keys
		for (c = 0; c < 32; c++)
		{
			if ((gSpecialKeys & (0x01 << c)) == 0)
			{
				if ((c == 2) || (c == 3) || (c==5))
					continue;
				if (keylabel[0] != 0)
					strcat(keylabel, "+");
				strcat(keylabel, gSpKeyText[c]);
			}
		}

		// Add text for regular keys
		for (c = ' '+1; c < '~'; c++)
		{
			if (gKeyStates[c])
			{
				if (keylabel[0] != 0)
					strcat(keylabel, "+");
				sprintf(keystr, "'%c'", c);

				// Space shows up better written instead of ' '
				if (c == 0x20)
					strcat(keylabel, "SPACE");
				else
					strcat(keylabel, keystr);
			}
		}

		// For some reason the F1 key causes garbage to print in
		// the status box.  This check fixed it for some reason -- ????
		Fl::check();

		// Update the text
		gpKey->label(keylabel);
	}

	return 1;
}

/*
==========================================================
t200_command:	This function processes commands sent to 
				Model 200 LCD subsystem.
==========================================================
*/
void t200_command(unsigned char ir, unsigned char data)
{
	if (gpDisp != NULL)
		gpDisp->Command(ir, data);
	if (gpDebugMonitor != 0)
		gpDebugMonitor->Command(ir, data);
}

/*
==========================================================
t200_readport:	This function returns the I/O port data
				for the port specified.
==========================================================
*/
unsigned char t200_readport(unsigned char port)
{
	if (gpDisp == NULL)
		return 0;
	return ((T200_Disp*)gpDisp)->ReadPort(port);
}

const T200_Disp& T200_Disp::operator=(const T200_Disp& srcDisp)
{
	int		c;

	// Copy the LCD data
	if (this != &srcDisp)
	{
		for (c = 0; c < 8192; c++)
			m_ram[c] = srcDisp.m_ram[c];

		// Copy control variables
		m_dstarth = srcDisp.m_dstarth;
		m_dstartl = srcDisp.m_dstartl;
		m_mcr = srcDisp.m_mcr;
		m_hpitch = srcDisp.m_hpitch;
		m_hcnt = srcDisp.m_hcnt;
		m_curspos = srcDisp.m_curspos;
		m_cursaddrh = srcDisp.m_cursaddrh;
		m_cursaddrl = srcDisp.m_cursaddrl;
		m_ramrd = srcDisp.m_ramrd;
		m_ramwr = srcDisp.m_ramwr;
		m_ramrd_addr = srcDisp.m_ramrd_addr;
		m_ramrd_dummy = srcDisp.m_ramrd_dummy;
		m_redraw_count = srcDisp.m_redraw_count;
		m_tdiv = srcDisp.m_tdiv;
		m_last_dstart = srcDisp.m_last_dstart;
	}

	return *this;
}

/*
=======================================================
T200:Disp:	This is the class construcor
=======================================================
*/
T200_Disp::T200_Disp(int x, int y, int w, int h) :
  T100_Disp(x, y, w, h)
{
	  int c;

	  /* Clear the display RAM */
	  for (c = 0; c < 8192; c++)
		  m_ram[c] = 0;

	  m_MyFocus = 0;
	  m_dstarth = 0;
	  m_dstartl = 0;
	  m_mcr = 0;
	  m_hpitch = 0;
	  m_hcnt = 0;
	  m_curspos = 0;
	  m_cursaddrh = 0;
	  m_cursaddrl = 0;
	  m_ramrd = 0;
	  m_ramrd_dummy = 0;
	  m_redraw_count = 0;
}

/*
==========================================================
Command:	This function processes commands sent to 
			Model 200 LCD subsystem.
==========================================================
*/
void T200_Disp::Command(int instruction, uchar data)
{
	int		cursaddr, dstart;
	char	mask;
	int		c;

	/* Calculate cursaddr just in case */
	cursaddr = (m_cursaddrh << 8) | m_cursaddrl;

	switch (instruction)
	{
	case SET_MCR:
		m_mcr = data;
		break;

	case SET_HPITCH:
		m_hpitch = data;
		break;

	case SET_HCNT:
		m_hcnt = data;
		break;

	case SET_TDIV:
		m_tdiv = data;
		break;

	case SET_CURSPOS:
		m_curspos = data;
		break;

	case SET_DSTARTL:
		/* Save new value for m_dstartl */
		m_dstartl = data;

		break;

	case SET_DSTARTH:
		/* Save new value of m_dstarth */
		m_dstarth = data;


		/* Calculate location of new page start */
		dstart = (m_dstarth << 8) | m_dstartl;

		/* Test if changing to a totally new page */
		c = abs(dstart - m_last_dstart);
		if (c != 0x140)
			c = dstart - m_last_dstart + 0x2000;
		if (c != 0x140)
			c = m_last_dstart - dstart + 0x2000;

		m_last_dstart = dstart;

		if (c != 0x140)
		{
			/* Clear all of memory to prevent screen "jump" */
			for (c = 0; c < 0x2000; c++)
				m_ram[c] = 0;
		}
		else
		{
			/* Clear RAM "above" display area */
			dstart -= 640;
			if (dstart < 0)
				dstart += 8192;
			for (c = 0; c < 640; c++)
			{
				m_ram[dstart++] = 0;
				if (dstart >= 8192)
					dstart = 0;
			}
		}

		m_redraw_count = 40 * 8;
//		printf("T200:IR=0x%04X  DATA=0x%02X\n", instruction, dstart);

		break;

	case SET_CURSADDRL:
		/* Test for increment of m_dstarth */
		if ((m_cursaddrl & 0x80) && !(data & 0x80))
		{
			m_cursaddrh++;

			/* Test if address >= 8192 */
			if (m_cursaddrh >= 0x20)
				m_cursaddrh = 0;
		}

		/* Save new value of cursor address */
		m_cursaddrl = data;
		break;

	case SET_CURSADDRH:
		/* Save new value of m_cursaddrh */
		m_cursaddrh = data;
		break;

	case RAM_WR:
		/* Draw new data on the LCD */
		SetByte(cursaddr, 0, data);

		/* Increment cursor address */
		if (++m_cursaddrl == 0)
			m_cursaddrh++;

		/* Test if address >= 8192 */
		if (m_cursaddrh >= 0x20)
			m_cursaddrh = 0;

		if (m_redraw_count > 0)
		{
			if (--m_redraw_count == 0)
				redraw_active();
		}
		break;

	case RAM_RD:
		/* Load m_ramrd register from the dummy register */
		m_ramrd_addr = cursaddr;

		break;

	case RAM_BIT_CLR:		/* Clear the bit specified by data at cursaddr */
		/* First calculate the AND mask */
		mask = 0xFE << data;

		/* Draw new data on the LCD */
		SetByte(cursaddr, 0, m_ram[cursaddr] & mask);

		/* Now increment the address */
		if (++m_cursaddrl == 0)
			m_cursaddrh++;
		/* Test if address >= 8192 */

		if (m_cursaddrh >= 0x20)
			m_cursaddrh = 0;

		break;

	case RAM_BIT_SET:		/* Set the bit specified by data at cursaddr */
		/* First calculate the OR mask */
		mask = 0x01 << data;

		/* Draw new data on the LCD */
		SetByte(cursaddr, 0, m_ram[cursaddr] | mask);

		/* Now increment the address */
		if (++m_cursaddrl == 0)
			m_cursaddrh++;

		/* Test if address >= 8192 */
		if (m_cursaddrh >= 0x20)
			m_cursaddrh = 0;

		break;

	}
}

/*
=================================================================
draw:	This routine draws the entire LCD.  This is a member 
		function of Fl_Window.
=================================================================
*/
void T200_Disp::draw()
{

	int		x=0;
	int		y=0;
	int		addr, col, row;
	uchar	value;

	/* Draw the static portions of the LCD / screen */
	draw_static();

	/* Get RAM address where display should start */
	addr = ((m_dstarth << 8) | m_dstartl) & (8192-1);

	fl_color(FL_BLACK);

	/* Check if the driver is in "graphics" mode */
	if (m_mcr & 0x02)
	{
		/* Loop through all 128 display pixel rows */
		for (row = 0; row < 128; row++)
		{
			y = row;

			/* Loop through all 40 LCD columns */
			for (col = 0; col < 40; col++)
			{
				/* Calculate x coordinate (Characters are 6 pixels wide) */
				x= col * 6;


				/* Get data to be displayed on "LCD" */
				value = m_ram[addr++];
				if (addr >= 8192)
					addr = 0;

				// Erase line so it is grey, then fill in with black where needed
//				fl_color(FL_GRAY);
//				fl_rectf(x*MultFact + gXoffset,y*MultFact + 
//					gYoffset,8*MultFact,gRectsize);
//				fl_color(FL_BLACK);

				// Draw the black pixels
				drawpixel(x++,y, value&0x01);
				drawpixel(x++,y, value&0x02);
				drawpixel(x++,y, value&0x04);
				drawpixel(x++,y, value&0x08);
				drawpixel(x++,y, value&0x10);
				drawpixel(x++,y, value&0x20);
			}
		}
	}
}


void T200_Disp::SetByte(int driver, int col, uchar value)
{
	int x;
	int y;
	int addr = driver;
	int diff = addr - ((m_dstarth << 8) | m_dstartl);
	if (diff < 0)
		diff += 8192;

	// Check if LCD already has the value being requested
	if (m_ram[addr] == value)
		return;

	// Load new value into lcd "RAM"
	m_ram[addr] = value;

	// Calculate X position of byte */
	x = (diff % 40) * 6;

	// Calcluate y position of byte
	y = (diff / 40);
	if (y >= 128)
		return;

	// Set the display
	window()->make_current();

	fl_color(FL_GRAY);
	fl_rectf(x*MultFact + gXoffset, y*MultFact + 
		gYoffset, MultFact*6, gRectsize);
	fl_color(FL_BLACK);

	// Draw each pixel of byte
	drawpixel(x++,y,value&0x01);
	drawpixel(x++,y,(value&0x02)>>1);
	drawpixel(x++,y,(value&0x04)>>2);
	drawpixel(x++,y,(value&0x08)>>3);
	drawpixel(x++,y,(value&0x10)>>4);
	drawpixel(x++,y,(value&0x20)>>5);
}

unsigned char T200_Disp::ReadPort(unsigned char port)
{
	/* Read RAM data through dummy register */
	m_ramrd = m_ramrd_dummy;
	
	/* Do a dummy read */
	m_ramrd_dummy = m_ram[m_ramrd_addr++];
	if (m_ramrd_addr >= 8192)
		m_ramrd_addr = 0;

	return m_ramrd;
}

/*
=================================================================
redraw_active:	This routine redraws only the active LCD pixel
				portion of the screen.  This prevents screen
				"flicker" that would be caused by erasing the
				entire background and redrawing the static data
				every time the screen scrolls.
=================================================================
*/
void T200_Disp::redraw_active()
{

	int		x=0;
	int		y=0;
	int		addr, col, row;
	uchar	value;

	// Set the display
	window()->make_current();

	/* Get RAM address where display should start */
	addr =( (m_dstarth << 8) | m_dstartl) &(8192-1);

	/* Check if the driver is in "graphics" mode */
	if (m_mcr & 0x02)
	{
		/* Loop through all 128 display pixel rows */
		for (row = 0; row < 128; row++)
		{
			// Erase line so it is grey, then fill in with black where needed
			y = row;
			fl_color(FL_GRAY);
			fl_rectf(gXoffset,y*MultFact + 
				gYoffset,240*MultFact,gRectsize);
			fl_color(FL_BLACK);

			/* Loop through all 40 LCD columns */
			for (col = 0; col < 40; col++)
			{
				/* Calculate x coordinate (Characters are 6 pixels wide) */
				x= col * 6;

				/* Get data to be displayed on "LCD" */
				value = m_ram[addr++];
				if (addr >= 8192)
					addr = 0;

				// Draw the black pixels
				drawpixel(x++,y, value&0x01);
				drawpixel(x++,y, value&0x02);
				drawpixel(x++,y, value&0x04);
				drawpixel(x++,y, value&0x08);
				drawpixel(x++,y, value&0x10);
				drawpixel(x++,y, value&0x20);
			}
		}
	}
}

