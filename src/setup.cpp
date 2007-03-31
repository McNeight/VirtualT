/* setup.cpp */

/* $Id: setup.cpp,v 1.1 2004/08/05 06:46:12 kpettit1 Exp $ */

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
#include <FL/Fl_Tabs.H>
#include <FL/Fl_Input.H>
#include <FL/Fl_Button.H>
#include <FL/Fl_Round_Button.H>
#include <FL/Fl_Check_Button.H>
#include <FL/Fl_Choice.H>
#include <FL/Fl_Box.H>
#include <FL/Fl_Return_Button.H>
#include <FL/Fl_Preferences.H>
#include <FL/Fl_File_Chooser.H>
#include <FL/Fl_Hold_Browser.H>
#include <FL/Enumerations.H>
#include <stdio.h>
#include <string.h>
#include <ctype.h>

#include "m100emu.h"
#include "serial.h"
#include "setup.h"
#include "memory.h"
#include "memedit.h"
#include "file.h"

extern	Fl_Preferences virtualt_prefs;

typedef struct setup_ctrl_struct	
{
	Fl_Tabs*				pTabs;

	struct
	{
		Fl_Group*			g;
		Fl_Round_Button*	pNone;
		Fl_Round_Button*	pSim;
		Fl_Input*			pCmd;
		Fl_Round_Button*	pHost;
		Fl_Choice*			pPort;
		Fl_Round_Button*	pOther;
		Fl_Input*			pOtherName;
	} com;
	struct 
	{
		Fl_Group*			g;
	} lpt;
	struct 
	{
		Fl_Group*			g;
		Fl_Box*				pText;
	} mdm;
	struct 
	{
		Fl_Group*			g;
		Fl_Box*				pText;
	} cas;
	struct 
	{
		Fl_Group*			g;
		Fl_Box*				pText;
	} bcr;
	struct 
	{
		Fl_Group*			g;
		Fl_Box*				pText;
	} sound;
} setup_ctrl_t;


typedef struct memory_ctrl_struct	
{
	Fl_Round_Button*	pNone;
	Fl_Round_Button*	pReMem;
	Fl_Round_Button*	pRampac;
	Fl_Round_Button*	pReMem_Rampac;
	Fl_Input*			pReMemFile;
	Fl_Input*			pRampacFile;
	Fl_Button*			pReMemBrowse;
	Fl_Button*			pRampacBrowse;
	Fl_Box*				pReMemText;
	Fl_Check_Button*	pOptRomRW;
} memory_ctrl_t;

extern "C" {
extern int			gOptRomRW;
}


Fl_Window	*gpsw;				// Peripheral Setup Window
Fl_Window	*gmsw;				// Memory Setup Window

setup_ctrl_t		setup_ctrl;	// Setup window controls
memory_ctrl_t		mem_ctrl;	// Memory setup window
peripheral_setup_t	setup;		// Setup options
memory_setup_t		mem_setup;	// Memory setup options

/*
============================================================================
Routines to load and save setup structure to the user preferences
============================================================================
*/
void save_setup_preferences(void)
{
	// Save COM emulation settings
	virtualt_prefs.set("ComMode", setup.com_mode);
	virtualt_prefs.set("ComCmd", setup.com_cmd);
	virtualt_prefs.set("ComPort", setup.com_port);
	virtualt_prefs.set("ComOther", setup.com_other);

	// Save LPT emulation settings

	// Save MDM emulation settings

	// Save CAS emulation settings

	// Save BCR emulation settings

	// Save Sound emulation settings
}

void load_setup_preferences(void)
{
	#ifdef	__APPLE__
		//JV 10/08/05
		int ex;
		char *ret;
		//---------JV
	#endif

	// Load COM emulation settings
	virtualt_prefs.get("ComMode", setup.com_mode,0);
	virtualt_prefs.get("ComCmd",  setup.com_cmd,"", 128);
	virtualt_prefs.get("ComPort", setup.com_port,"", 128);
	virtualt_prefs.get("ComOther", setup.com_other,"", 128);

	// Load LPT emulation settings

	// Load MDM emulation settings

	// Load CAS emulation settings

	// Load BCR emulation settings

	// Load Sound emulation settings

#ifdef	__APPLE__
	//JV 08/10/05
	// Load Path Preferences
	ex=virtualt_prefs.get("Path",path,".",256);
	if(ex==0) // no path in the pref file or pref file nonexistent 
	{
		ret=ChooseWorkDir(); // return the directory
		if(ret==NULL) 
			exit(-1); //nothing choose: do nothing....
		else 
		{
			strcpy(path,ret);
			#ifdef __unix__
			strcat(path,"/");
			#else
			strcat(path,"\\");
			#endif 
			virtualt_prefs.set("Path",path); // set pref path
		}
	}
	//JV
#endif
}

/*
============================================================================
Callback routines for the Setup Window
============================================================================
*/
void cb_setup_cancel (Fl_Widget* w, void*)
{
	gpsw->hide();
	delete gpsw;
}

void cb_setup_OK(Fl_Widget* w, void*)
{
	// First check if Host port needs to be closed
	if (setup.com_mode == SETUP_COM_HOST)
	{
		// Check if we are turning Host port emulation off
		if (setup_ctrl.com.pHost->value() != 1)
			ser_close_port();

		// Check if we are changing ports
		if (strcmp(setup_ctrl.com.pPort->text(), setup.com_port) != 0)
			ser_close_port();
	}

	if (setup.com_mode == SETUP_COM_OTHER)
	{
		// Check if we are turning Host port emulation off
		if (setup_ctrl.com.pOther->value() != 1)
			ser_close_port();

		// Check if we are changing ports
		if (strcmp(setup_ctrl.com.pOtherName->value(), setup.com_other) != 0)
			ser_close_port();
	}

	// ===========================
	// Get COM options
	// ===========================
	strcpy(setup.com_cmd, setup_ctrl.com.pCmd->value());
	if (setup_ctrl.com.pPort->text() != NULL)
		strcpy(setup.com_port, setup_ctrl.com.pPort->text());
	else
		strcpy(setup.com_port, "");
	strcpy(setup.com_other, setup_ctrl.com.pOtherName->value());

	if (setup_ctrl.com.pNone->value() == 1)
	{
		ser_set_port("No Emulation");
		setup.com_mode = SETUP_COM_NONE;
	}
	else if (setup_ctrl.com.pSim->value() == 1)
	{
		// Open the Script file
		ser_set_port(setup.com_cmd);
		setup.com_mode = SETUP_COM_SIMULATED;
		ser_open_port();
	}
	else if (setup_ctrl.com.pHost->value() == 1)
	{
		// Set preference in structure
		setup.com_mode = SETUP_COM_HOST;

		// Open the Host port
		ser_set_port(setup.com_port);
		ser_open_port();
	}
	else if (setup_ctrl.com.pOther->value() == 1)
	{
		// Set preference in structure
		setup.com_mode = SETUP_COM_OTHER;

		// Open the Host port
		ser_set_port(setup.com_other);
		ser_open_port();
	}

	// ===========================
	// Get LPT options
	// ===========================

	// ===========================
	// Get MDM options
	// ===========================

	// ===========================
	// Get CAS options
	// ===========================

	// ===========================
	// Get BCR options
	// ===========================

	// ===========================
	// Get Sound options
	// ===========================

	// Save setup preferences to file
	save_setup_preferences();

	// Destroy the window
	gpsw->hide();
	delete gpsw;
}

void cb_setupwin (Fl_Widget* w, void*)
{
	gpsw->hide();
	delete gpsw;
}

void cb_memorywin (Fl_Widget* w, void*)
{
	gmsw->hide();
	delete gmsw;
}

/*
============================================================================
Callback routines for the COM Tab
============================================================================
*/
void cb_com_radio_none (Fl_Widget* w, void*)
{
	setup_ctrl.com.pCmd->deactivate();
	setup_ctrl.com.pPort->deactivate();
	setup_ctrl.com.pOtherName->deactivate();
}

void cb_com_radio_sim (Fl_Widget* w, void*)
{
	setup_ctrl.com.pCmd->activate();
	setup_ctrl.com.pPort->deactivate();
	setup_ctrl.com.pOtherName->deactivate();
}

void cb_com_radio_host (Fl_Widget* w, void*)
{
	setup_ctrl.com.pCmd->deactivate();
	setup_ctrl.com.pPort->activate();
	setup_ctrl.com.pOtherName->deactivate();
}

void cb_com_radio_other (Fl_Widget* w, void*)
{
	setup_ctrl.com.pCmd->deactivate();
	setup_ctrl.com.pPort->deactivate();
	setup_ctrl.com.pOtherName->activate();
}

/*
============================================================================
Routine to create the PeripheralSetup Window and tabs
============================================================================
*/
void cb_PeripheralSetup (Fl_Widget* w, void*)
{
	char	com_port_list[256];
	int		count;
	char	*token;
	int		index;

	// Create Peripheral Setup window
	gpsw = new Fl_Window(350, 300, "Peripheral Setup");
	gpsw->callback(cb_setupwin);

	// Create Peripheral Tabs
    {  
		setup_ctrl.pTabs = new Fl_Tabs(10, 10, 330, 240);

		// COM port Tab
		{ 
			setup_ctrl.com.g = new Fl_Group(10, 30, 350, 260, " COM ");

			// Create items on the Tab
			setup_ctrl.com.pNone = new Fl_Round_Button(20, 40, 180, 20, "No emulation");
			setup_ctrl.com.pNone->type(FL_RADIO_BUTTON);
			setup_ctrl.com.pNone->callback(cb_com_radio_none);

			setup_ctrl.com.pSim = new Fl_Round_Button(20, 65, 180, 20, "Use Simulated Port (not supported yet)");
			setup_ctrl.com.pSim->type(FL_RADIO_BUTTON);
			setup_ctrl.com.pSim->callback(cb_com_radio_sim);

			setup_ctrl.com.pCmd = new Fl_Input(105, 90, 200, 20, "Cmd File:");
			if (setup.com_mode != SETUP_COM_SIMULATED)
				setup_ctrl.com.pCmd->deactivate();
			setup_ctrl.com.pCmd->value(setup.com_cmd);

			setup_ctrl.com.pHost = new Fl_Round_Button(20, 115, 180, 20, "Use Host Port");
			setup_ctrl.com.pHost->type(FL_RADIO_BUTTON);
			setup_ctrl.com.pHost->callback(cb_com_radio_host);

			setup_ctrl.com.pPort = new Fl_Choice(50, 140, 240, 20, "");
			if (setup.com_mode != SETUP_COM_HOST)
				setup_ctrl.com.pPort->deactivate();

			setup_ctrl.com.pOther = new Fl_Round_Button(20, 170, 180, 20, "Other Host Port");
			setup_ctrl.com.pOther->type(FL_RADIO_BUTTON);
			setup_ctrl.com.pOther->callback(cb_com_radio_other);

			setup_ctrl.com.pOtherName = new Fl_Input(50, 195, 240, 20, "");
			if (setup.com_mode != SETUP_COM_OTHER)
				setup_ctrl.com.pOtherName->deactivate();
			setup_ctrl.com.pOtherName->value(setup.com_other);


			// Get list of COM ports on the host
			ser_get_port_list(com_port_list, 256, &count);
			token = strtok(com_port_list, ",");
			while (token != 0)
			{
				index = setup_ctrl.com.pPort->add(token);

				if (strcmp(token, setup.com_port) == 0)
					setup_ctrl.com.pPort->value(index);


				// Get next item from list
				token = strtok(NULL, ",");
			}

			if (setup.com_mode == SETUP_COM_NONE)
				setup_ctrl.com.pNone->value(1);
			else if (setup.com_mode == SETUP_COM_HOST)
				setup_ctrl.com.pHost->value(1);
			else if (setup.com_mode == SETUP_COM_SIMULATED)
				setup_ctrl.com.pSim->value(1);
			else if (setup.com_mode == SETUP_COM_OTHER)
				setup_ctrl.com.pOther->value(1);

			// End of Controls for this tab
			setup_ctrl.com.g->end();
		}

		// LPT Port Tab
		{ 
			// Create the Group item (the "Tab")
			setup_ctrl.lpt.g = new Fl_Group(10, 30, 350, 260, " LPT ");

			// Create controls
			setup_ctrl.mdm.pText = new Fl_Box(120, 60, 60, 80, "Parallel Port not supported yet");

			// End of control for this tab
			setup_ctrl.lpt.g->end();
		}

		// Modem Port Tab
		{
			// Create the Group item (the "Tab")
			setup_ctrl.mdm.g = new Fl_Group(10, 30, 350, 260, " MDM ");

			// Create controls
			setup_ctrl.mdm.pText = new Fl_Box(120, 60, 60, 80, "Modem Port not supported yet");

			// End of control for this tab
			setup_ctrl.mdm.g->end();
		}

		// Cassette Port Tab
		{ 
			// Create the Group item (the "Tab")
			setup_ctrl.cas.g = new Fl_Group(10, 30, 300, 260, " CAS ");

			// Create controls
			setup_ctrl.cas.pText = new Fl_Box(120, 60, 60, 80, "Cassette Port not supported yet");

			// End of control for this tab
			setup_ctrl.cas.g->end();
		}

		// BCR Port Tab
		{ 
			// Create the Group item (the "Tab")
			setup_ctrl.bcr.g = new Fl_Group(10, 30, 300, 260, " BCR ");

			// Create controls
			setup_ctrl.bcr.pText = new Fl_Box(120, 60, 60, 80, "BCR Port not supported yet");

			// End of control for this tab
			setup_ctrl.bcr.g->end();
		}

		// Speaker Port Tab
		{ 
			// Create the Group item (the "Tab")
			setup_ctrl.sound.g = new Fl_Group(10, 30, 300, 260, " Sound ");

			// Create controls
			setup_ctrl.sound.pText = new Fl_Box(120, 60, 60, 80, "Sound not supported yet");

			// End of control for this tab
			setup_ctrl.sound.g->end();
		}

		setup_ctrl.pTabs->value(setup_ctrl.com.g);
		setup_ctrl.pTabs->end();
																															 
	}

	// OK button
    { Fl_Button* o = new Fl_Button(180, 260, 60, 30, "Cancel");
      o->callback((Fl_Callback*)cb_setup_cancel);
    }
    { Fl_Return_Button* o = new Fl_Return_Button(250, 260, 60, 30, "OK");
      o->callback((Fl_Callback*)cb_setup_OK);
    }

	gpsw->show();
}

/*
============================================================================
Routines to load and save setup structure to the user preferences
============================================================================
*/
void save_memory_preferences(void)
{
	char	str[16];
	char	pref[64];

	get_model_string(str, gModel);

	// Save COM emulation settings
	strcpy(pref, str);
	strcat(pref, "_MemMode");
	virtualt_prefs.set(pref, mem_setup.mem_mode);

	strcpy(pref, str);
	strcat(pref, "_ReMemFile");
	virtualt_prefs.set(pref, mem_setup.remem_file);


	strcpy(pref, str);
	strcat(pref, "_RampacFile");
	virtualt_prefs.set(pref, mem_setup.rampac_file);

	strcpy(pref, str);
	strcat(pref, "_OptRomRW");
	virtualt_prefs.set(pref, gOptRomRW);
}

void load_memory_preferences(void)
{
	char	str[16];
	char	pref[64];
	char	path[256];

	get_model_string(str, gModel);

	// Load mem emulation mode base on Model
	strcpy(pref, str);
	strcat(pref, "_MemMode");
	virtualt_prefs.get(pref, mem_setup.mem_mode,0);

	// Load ReMem filename base on Model
	strcpy(pref, str);
	strcat(pref, "_ReMemFile");
	get_emulation_path(path, gModel);
	strcat(path, "remem.bin");
	virtualt_prefs.get(pref,  mem_setup.remem_file, path, 256);
	if (strlen(mem_setup.remem_file) == 0)
		strcpy(mem_setup.remem_file, path);

	// Load Rampac filename base on Model
	strcpy(pref, str);
	strcat(pref, "_RampacFile");
	get_emulation_path(path, gModel);
	strcat(path, "rampac.bin");
	virtualt_prefs.get(pref, mem_setup.rampac_file, path, 256);
	if (strlen(mem_setup.rampac_file) == 0)
		strcpy(mem_setup.rampac_file, path);

	// Load OptRom R/W or R/O option
	strcpy(pref, str);
	strcat(pref, "_OptRomRW");
	virtualt_prefs.get(pref, gOptRomRW, 0);
}

/*
============================================================================
Callback routines for the Memory options window
============================================================================
*/
void cb_memory_OK(Fl_Widget* w, void*)
{
	int		old_mode;

	/* 
	===================================================
	First check if ReMem memory needs to be deallocated
	===================================================
	*/
	if ((mem_setup.mem_mode == SETUP_MEM_REMEM) || (mem_setup.mem_mode == SETUP_MEM_REMEM_RAMPAC))
	{
		// Check if we are turning ReMem emulation off
		if ((mem_ctrl.pReMem->value() != 1) && (mem_ctrl.pReMem_Rampac->value() != 1))
		{
			save_remem_ram();		// Write ReMem memory to file
			free_remem_mem();		// Deallocate ReMem memory
		}
	}

	/* 
	===================================================
	Next check if Rampac memory needs to be deallocated
	===================================================
	*/
	if ((mem_setup.mem_mode == SETUP_MEM_RAMPAC) || (mem_setup.mem_mode == SETUP_MEM_REMEM_RAMPAC))
	{
		// Check if we are turning Host port emulation off
		if (mem_ctrl.pRampac->value() != 1)
		{
			save_rampac_ram();		// Write Rampac memory to file
			free_rampac_mem();		// Deallocate Rampac memory
		}
	}

	// Save old mem_mode so we know when to load data from file
	old_mode = mem_setup.mem_mode;

	// ===========================
	// Get memory options
	// ===========================
	if (mem_ctrl.pNone->value() == 1)
		mem_setup.mem_mode = SETUP_MEM_BASE;
	else if (mem_ctrl.pRampac->value() == 1)
		mem_setup.mem_mode = SETUP_MEM_RAMPAC;
	else if (mem_ctrl.pReMem->value() == 1)
		mem_setup.mem_mode = SETUP_MEM_REMEM;
	else if (mem_ctrl.pReMem_Rampac->value() == 1)
		mem_setup.mem_mode = SETUP_MEM_REMEM_RAMPAC;

	// Get OptRom R/W Enable setting
	gOptRomRW = mem_ctrl.pOptRomRW->value();

	// Allocate ReMem and / or Rampac memory if not already
	init_mem();

	// If we are in ReMem or ReMem_Rampac mode, check if ReMem filename changed
	if ((mem_setup.mem_mode == SETUP_MEM_REMEM) || (mem_setup.mem_mode == SETUP_MEM_REMEM_RAMPAC))
	{
		// Check if we are changing ReMem filename
		if (strcmp(mem_ctrl.pReMemFile->value(), mem_setup.remem_file) != 0)
		{
			// Save memory to old file
			save_remem_ram();

			// Copy new filename to preferences
			strcpy(mem_setup.remem_file, mem_ctrl.pReMemFile->value());

			// Load ReMem data from new file
			load_remem_ram();
		}
		else if ((old_mode != SETUP_MEM_REMEM) && (old_mode != SETUP_MEM_REMEM_RAMPAC))
		{
			// Load ReMem data from file
			load_remem_ram();
		}
	}

	// If we are in Rampac or ReMem_Rampac mode, check if Rampac filename changed
	if ((mem_setup.mem_mode == SETUP_MEM_RAMPAC) || (mem_setup.mem_mode == SETUP_MEM_REMEM_RAMPAC))
	{
		// Check if we are changing Rampac filename
		if (strcmp(mem_ctrl.pRampacFile->value(), mem_setup.rampac_file) != 0)
		{
			// Save memory to old file
			save_rampac_ram();

			// Copy new filename to preferences
			strcpy(mem_setup.rampac_file, mem_ctrl.pRampacFile->value());

			// Load Rampac data from new file
			load_rampac_ram();
		}
		else if ((old_mode != SETUP_MEM_RAMPAC) && (old_mode != SETUP_MEM_REMEM_RAMPAC))
		{
			// Load Rampac data from file
			load_rampac_ram();
		}
	}

	// Copy new ReMem filename and Rampac filename to preferences
	strcpy(mem_setup.remem_file, mem_ctrl.pReMemFile->value());
	strcpy(mem_setup.rampac_file, mem_ctrl.pRampacFile->value());

	// Save memory preferences to file
	save_memory_preferences();

	// Update Memory Editor
	cb_MemoryEditorUpdate();

	/* Reset the CPU */
	resetcpu();
	gExitLoop = 1;

	// Destroy the window
	gmsw->hide();
	delete gmsw;
}

void cb_remem_browse(Fl_Widget* w, void*)
{
	int					count;
	Fl_File_Chooser		*fc;
	const char			*filename;
	const char			*filename_name;
	int					len;
	char				mstr[16];
	char				mstr_upper[16];
	char				path[256];
	int					c;
	
	// Create chooser window to pick file
	strcpy(path, mem_ctrl.pReMemFile->value());
	fc = new Fl_File_Chooser(path,"Binary Files (*.bin)",2,"Choose ReMem File");
	fc->preview(0);
	fc->show();

	// Show Chooser window
	while (fc->visible())
		Fl::wait();

	count = fc->count();
	if (count == 0)
	{
		delete fc;
		return;
	}

	// Get Filename
	filename = fc->value(1);
	if (filename == 0)
	{
		delete fc;
		return;
	}
	len = strlen(filename);

	// Copy filename to edit field
	filename_name = fl_filename_name(filename);

	get_model_string(mstr, gModel);
	strcpy(mstr_upper, mstr);
	for (c = strlen(mstr_upper)-1; c >= 0; c--)
		mstr_upper[c] = toupper(mstr_upper[c]);
	if (strstr(filename, mstr) || strstr(filename, mstr_upper))
	{
		get_emulation_path(path, gModel);
		strcat(path, filename_name);
		mem_ctrl.pReMemFile->value(path);
	}
	else
		mem_ctrl.pReMemFile->value(filename);

	delete fc;
}

void cb_radio_base_memory (Fl_Widget* w, void*)
{
	mem_ctrl.pRampacFile->deactivate();
	mem_ctrl.pRampacBrowse->deactivate();
	mem_ctrl.pReMemFile->deactivate();
	mem_ctrl.pReMemBrowse->deactivate();
	mem_ctrl.pReMemText->hide();
}

void cb_radio_remem (Fl_Widget* w, void*)
{
	mem_ctrl.pReMemFile->activate();
	mem_ctrl.pReMemBrowse->activate();
	mem_ctrl.pRampacFile->deactivate();
	mem_ctrl.pRampacBrowse->deactivate();
	mem_ctrl.pReMemText->show();
}

void cb_radio_rampac (Fl_Widget* w, void*)
{
	mem_ctrl.pRampacFile->activate();
	mem_ctrl.pRampacBrowse->activate();
	mem_ctrl.pReMemFile->deactivate();
	mem_ctrl.pReMemBrowse->deactivate();
	mem_ctrl.pReMemText->hide();
}

void cb_radio_remem_and_rampac (Fl_Widget* w, void*)
{
	mem_ctrl.pReMemFile->activate();
	mem_ctrl.pReMemBrowse->activate();
	mem_ctrl.pRampacFile->activate();
	mem_ctrl.pRampacBrowse->activate();
	mem_ctrl.pReMemText->show();
}

void cb_memory_cancel (Fl_Widget* w, void*)
{
	gmsw->hide();
	delete gpsw;
}

void cb_rampac_browse (Fl_Widget* w, void*)
{
	int					count;
	Fl_File_Chooser		*fc;
	const char			*filename;
	const char			*filename_name;
	int					len;
	char				mstr[16];
	char				mstr_upper[16];
	char				path[256];
	int					c;
	
	// Create chooser window to pick file
	strcpy(path, mem_ctrl.pRampacFile->value());
	fc = new Fl_File_Chooser(path,"Binary Files (*.bin)",2,"Choose Rampac File");
	fc->preview(0);
	fc->show();

	// Show Chooser window
	while (fc->visible())
		Fl::wait();

	count = fc->count();
	if (count == 0)
	{
		delete fc;
		return;
	}

	// Get Filename
	filename = fc->value(1);
	if (filename == 0)
	{
		delete fc;
		return;
	}
	len = strlen(filename);

	// Copy filename to edit field
	filename_name = fl_filename_name(filename);

	get_model_string(mstr, gModel);
	strcpy(mstr_upper, mstr);
	for (c = strlen(mstr_upper)-1; c >= 0; c--)
		mstr_upper[c] = toupper(mstr_upper[c]);
	if (strstr(filename, mstr) || strstr(filename, mstr_upper))
	{
		get_emulation_path(path, gModel);
		strcat(path, filename_name);
		mem_ctrl.pRampacFile->value(path);
	}
	else
		mem_ctrl.pRampacFile->value(filename);

	delete fc;
}

/*
============================================================================
Routine to create the PeripheralSetup Window and tabs
============================================================================
*/
void cb_MemorySetup (Fl_Widget* w, void*)
{
	// Create Peripheral Setup window
	gmsw = new Fl_Window(420, 280, "Memory Emulation Options");
	gmsw->callback(cb_memorywin);

	// Create items on the Tab
	mem_ctrl.pNone = new Fl_Round_Button(20, 20, 180, 20, "Base Memory");
	mem_ctrl.pNone->type(FL_RADIO_BUTTON);
	mem_ctrl.pNone->callback(cb_radio_base_memory);
	if (mem_setup.mem_mode == SETUP_MEM_BASE)
		mem_ctrl.pNone->value(1);

	mem_ctrl.pRampac = new Fl_Round_Button(20, 45, 180, 20, "RamPac  (256K RAM)");
	mem_ctrl.pRampac->type(FL_RADIO_BUTTON);
	mem_ctrl.pRampac->callback(cb_radio_rampac);
	if (mem_setup.mem_mode == SETUP_MEM_RAMPAC)
		mem_ctrl.pRampac->value(1);

	mem_ctrl.pReMem = new Fl_Round_Button(20, 70, 220, 20, "ReMem   (2M RAM, 4M FLASH)");
	mem_ctrl.pReMem->type(FL_RADIO_BUTTON);
	mem_ctrl.pReMem->callback(cb_radio_remem);
	if (mem_setup.mem_mode == SETUP_MEM_REMEM)
		mem_ctrl.pReMem->value(1);

	mem_ctrl.pReMem_Rampac = new Fl_Round_Button(20, 95, 180, 20, "ReMem + RamPac");
	mem_ctrl.pReMem_Rampac->type(FL_RADIO_BUTTON);
	mem_ctrl.pReMem_Rampac->callback(cb_radio_remem_and_rampac);
	if (mem_setup.mem_mode == SETUP_MEM_REMEM_RAMPAC)
		mem_ctrl.pReMem_Rampac->value(1);

	// ===============================================
	// Setup Rampac File Edit field and Browser button
	// ===============================================
	mem_ctrl.pRampacFile = new Fl_Input(105, 120, 210, 20, "RamPac File");
	mem_ctrl.pRampacFile->value(mem_setup.rampac_file);

	mem_ctrl.pRampacBrowse =	new Fl_Button(330, 115, 60, 30, "Browse");
    mem_ctrl.pRampacBrowse->callback((Fl_Callback*)cb_rampac_browse);

	if ((mem_setup.mem_mode != SETUP_MEM_RAMPAC) && (mem_setup.mem_mode != SETUP_MEM_REMEM_RAMPAC))
	{
		mem_ctrl.pRampacFile->deactivate();
		mem_ctrl.pRampacBrowse->deactivate();
	}

	// ===============================================
	// Setup ReMem File edit field and Browser button
	// ===============================================
	mem_ctrl.pReMemFile = new Fl_Input(105, 160, 210, 20, "ReMem  File");
	mem_ctrl.pReMemFile->value(mem_setup.remem_file);
    mem_ctrl.pReMemText = new Fl_Box(45, 180, 325, 20, "(Use Memory Editor to load FLASH)");
    mem_ctrl.pReMemText->labelsize(12);

	mem_ctrl.pReMemBrowse = new Fl_Button(330, 155, 60, 30, "Browse");
    mem_ctrl.pReMemBrowse->callback((Fl_Callback*)cb_remem_browse);

	if ((mem_setup.mem_mode != SETUP_MEM_REMEM) && (mem_setup.mem_mode != SETUP_MEM_REMEM_RAMPAC))
	{
		mem_ctrl.pReMemFile->deactivate();
		mem_ctrl.pReMemBrowse->deactivate();
		mem_ctrl.pReMemText->hide();
	}

	// Option ROM RW Enable
	mem_ctrl.pOptRomRW = new Fl_Check_Button(20, 200, 210, 20, "Make Option ROM R/W");
	mem_ctrl.pOptRomRW->value(gOptRomRW);

	// OK button
    { Fl_Button* o = new Fl_Button(160, 240, 60, 30, "Cancel");
      o->callback((Fl_Callback*)cb_memory_cancel);
    }
    { Fl_Return_Button* o = new Fl_Return_Button(230, 240, 60, 30, "OK");
      o->callback((Fl_Callback*)cb_memory_OK);
    }

	gmsw->show();
}
