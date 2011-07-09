/* socketdlg.cpp */

/* $Id: socketdlg.cpp,v 1.14 2009/12/31 05:11:05 kpettit1 Exp $ */

/*
 * Copyright 2009 Ken Pettit
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
#include <FL/Fl_Input.H>
#include <FL/Fl_Button.H>
#include <FL/Fl_Check_Button.H>
#include <FL/Fl_Box.H>
#include <FL/Fl_Return_Button.H>
#include <FL/Fl_Preferences.H>
#include <FL/Enumerations.H>
#include <stdio.h>
#include <string.h>
#include <ctype.h>
#include <string>

#include "genwrap.h"
#include "m100emu.h"
#include "setup.h"
#include "memedit.h"
#include "remote.h"

extern	Fl_Preferences virtualt_prefs;

typedef struct sockdlg_ctrl_struct	
{
	Fl_Input*			pPortNumber;
	Fl_Check_Button*	pTelnet;
	Fl_Check_Button*	pEnable;
	Fl_Box*				pStatus;
	Fl_Button*			pApply;
	char				sPortNumber[20];
	char				sStatus[256];
} sockdlg_ctrl_t;

static Fl_Window		*gsdw;		// SocketDlg window
static sockdlg_ctrl_t	sockdlg_ctrl;	// SocketDlg window controls

/*
============================================================================
Callback routines for the Setup Window
============================================================================
*/
static void cb_sockdlg_cancel (Fl_Widget* w, void*)
{
	gsdw->hide();
	delete gsdw;
	gsdw = NULL;
}

/*
============================================================================
This routine updates the status line of the dialog box based on the current
remote socket connection status.
============================================================================
*/
void update_port_status(void)
{
	if (get_remote_error_status())
		strcpy(sockdlg_ctrl.sStatus, get_remote_error().c_str());
	else if (get_remote_connect_status())
		strcpy(sockdlg_ctrl.sStatus, "Connected with client");
	else if (get_remote_listen_port() != 0)
		sprintf(sockdlg_ctrl.sStatus, "Listening on port %d", get_remote_listen_port());
	else
		strcpy(sockdlg_ctrl.sStatus, "Sockets not active");
	sockdlg_ctrl.pStatus->label(sockdlg_ctrl.sStatus);
}

/*
============================================================================
This routine applies the current settings in the dialog box and calls the
remote API functions to setup and teardown the socket interface.  It is
called by the OK button also.
============================================================================
*/
static void cb_sockdlg_apply(Fl_Widget* w, void*)
{
	int		port_num = 0, listen_port;
	int		telnet;
	int		enabled;

	// ==============================================
	// Get Port Number & telnet flag
	// ==============================================
	if (sockdlg_ctrl.pPortNumber->value() != NULL)
		port_num = atoi(sockdlg_ctrl.pPortNumber->value());
	telnet = sockdlg_ctrl.pTelnet->value();
	enabled = sockdlg_ctrl.pEnable->value();

	// Test if the socket is enabled but the port is invalid
	if (enabled && ((port_num <= 0) || (port_num >= 65536)))
	{
		fl_alert("Please provide a port number between 1 and 65535");
		return;
	}

	// Get the remote socket status
	listen_port = get_remote_listen_port();

	// Test if the socket is enabled and the user want's it disabled
	if (!enabled)
	{
		// Check if the socket is currently active and shutdown if it is
		if (listen_port != 0)
			deinit_remote();
	}

	// Test if we are enabling the socket for a different port number
	if (enabled && listen_port && (listen_port != port_num))
		deinit_remote();

	// Set the new port parameters
	set_remote_telnet(telnet);
	if (enabled)
		set_remote_port(port_num);
	else
		set_remote_port(0);
	enable_remote(enabled);

	// Update preferences
	virtualt_prefs.set("SocketPort", port_num);
	virtualt_prefs.set("SocketTelnetMode", telnet);
	virtualt_prefs.set("SocketEnabled", enabled);

	// Now enable the socket if the socket number is different
	if (enabled && (listen_port != port_num))
	{
		init_remote();
	}
	
	// If the apply button was pressed, then wait a bit for the port
	// status to change in the remote thread, then update the 
	// port status
	if (w == sockdlg_ctrl.pApply)
	{
		SLEEP(500);
		update_port_status();
	}
}

/*
============================================================================
Okay button handler.  This calls the apply handler and closes the window.
============================================================================
*/
static void cb_sockdlg_OK(Fl_Widget* w, void*)
{
	// Call the apply functionality
	cb_sockdlg_apply(w, NULL);

	// Destroy the window
	gsdw->hide();
	delete gsdw;
}

/*
============================================================================
Routine to create the PeripheralSetup Window and tabs
============================================================================
*/
static void cb_sockdlg_enable (Fl_Widget* w, void*)
{
	if (sockdlg_ctrl.pEnable->value())
	{
		sockdlg_ctrl.pPortNumber->activate();
		sockdlg_ctrl.pTelnet->activate();
	}
	else
	{
		sockdlg_ctrl.pPortNumber->deactivate();
		sockdlg_ctrl.pTelnet->deactivate();
	}
}

/*
============================================================================
Callback to close the Socket Dialog window
============================================================================
*/
static void cb_sockdlg_win (Fl_Widget* w, void*)
{
	gsdw->hide();
	delete gsdw;
	gsdw = NULL;
}

/*
============================================================================
Routine to create the PeripheralSetup Window and tabs
============================================================================
*/
void cb_SocketSetup (Fl_Widget* w, void*)
{
	Fl_Box* b;
	int		port_num, enabled, telnet;

	// Get socket interface preferences
	virtualt_prefs.get("SocketPort", port_num, get_remote_port());
	telnet = get_remote_telnet();
	enabled = get_remote_enabled();

	// Create Peripheral Setup window
	gsdw = new Fl_Window(300, 260, "Socket Configuration");
	gsdw->callback(cb_sockdlg_win);

	/* Create enable checkbox */
	sockdlg_ctrl.pEnable = new Fl_Check_Button(20, 20, 190, 20, "Enable Socket Interface");
	sockdlg_ctrl.pEnable->callback(cb_sockdlg_enable);
	sockdlg_ctrl.pEnable->value(enabled);

	/* Create input field for for port number */
	b = new Fl_Box(50, 50, 70, 20, "Port Number");
	b->align(FL_ALIGN_LEFT | FL_ALIGN_INSIDE);
	sockdlg_ctrl.pPortNumber = new Fl_Input(160, 50, 60, 20, "");
	sockdlg_ctrl.pPortNumber->align(FL_ALIGN_LEFT);
	sprintf(sockdlg_ctrl.sPortNumber, "%d", port_num);
	sockdlg_ctrl.pPortNumber->value(sockdlg_ctrl.sPortNumber);

	// Create checkbox for telnet mode
	sockdlg_ctrl.pTelnet = new Fl_Check_Button(50, 80, 200, 20, "Telnet Mode (std port is 23)");
	sockdlg_ctrl.pTelnet->align(FL_ALIGN_LEFT|FL_ALIGN_INSIDE);
	sockdlg_ctrl.pTelnet->value(telnet);

	// Deactivate port number and telnet mode if socket interface inactive
	if (!enabled)
	{
		sockdlg_ctrl.pPortNumber->deactivate();
		sockdlg_ctrl.pTelnet->deactivate();
	}

	b = new Fl_Box(20, 130, 120, 20, "Socket Port Status");
	b->align(FL_ALIGN_LEFT | FL_ALIGN_INSIDE);
	sockdlg_ctrl.pStatus = new Fl_Box(50, 160, 200, 20, "");
	sockdlg_ctrl.pStatus->align(FL_ALIGN_LEFT | FL_ALIGN_INSIDE);
	update_port_status();


	// Cancel button
    { Fl_Button* o = new Fl_Button(40, 200, 60, 30, "Cancel");
      o->callback((Fl_Callback*) cb_sockdlg_cancel);
    }

	// Apply button
	sockdlg_ctrl.pApply = new Fl_Button(120, 200, 60, 30, "Apply");
    sockdlg_ctrl.pApply->callback((Fl_Callback*) cb_sockdlg_apply);
 
	// OK button
    { Fl_Return_Button* o = new Fl_Return_Button(200, 200, 60, 30, "OK");
      o->callback((Fl_Callback*)cb_sockdlg_OK);
    }

	gsdw->show();
}
