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
#include <FL/Fl_Tabs.h>
#include <FL/Fl_Input.h>
#include <FL/Fl_Button.h>
#include <FL/Fl_Round_Button.h>
#include <FL/Fl_Check_Button.h>
#include <FL/Fl_Choice.h>
#include <FL/Fl_Box.h>
#include <FL/Fl_Return_Button.h>
#include <FL/Fl_Preferences.h>
#include <stdio.h>
#include <string.h>
#include <ctype.h>

#include "m100emu.h"
#include "io.h"
#include "serial.h"
#include "setup.h"

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


Fl_Window	*gpsw;				// Peripheral Setup Window

setup_ctrl_t		setup_ctrl;	// Setup window controls
peripheral_setup_t	setup;		// Setup options

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

