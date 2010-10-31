/* lpt.cpp */

/* $Id: lpt.cpp,v 1.14 2008/11/04 07:31:22 kpettit1 Exp $ */

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
#include <FL/fl_draw.H>
#include <FL/Fl_Window.H>
#include <FL/Fl_Round_Button.H>
#include <FL/Fl_Check_Button.H>
#include <FL/Fl_Return_Button.H>
#include <FL/Fl_Menu_Button.H>
#include <FL/Fl_Text_Display.H>
#include <FL/Fl_Text_Buffer.H>
#include <FL/Fl_Input.H>
#include <FL/Fl_Choice.H>
#include <FL/Fl_Box.H>
#include <FL/Fl_Preferences.H>
#include <FL/Fl_Pixmap.H>
#include <FL/fl_ask.H>

#include <string.h>
#include <stdio.h>
#include <stdlib.h>
#include <time.h>

#include "VirtualT.h"
#include "lpt.h"
#include "printer.h"
#include "fileprint.h"
#include "hostprint.h"
#include "fx80print.h"
#include "fl_action_icon.h"

extern Fl_Preferences	virtualt_prefs;
extern Fl_Action_Icon*	gpPrint;
typedef struct 
{
	Fl_Round_Button*	pNone;
	Fl_Round_Button*	pEmul;
	Fl_Choice*			pEmulPrint;
	Fl_Check_Button*	pCRtoLF;
	Fl_Check_Button*	pAutoFF;
	Fl_Input*			pAFFtimeout;
	Fl_Check_Button*	pAutoClose;
	Fl_Input*			pCloseTimeout;
} lptCtrl_t;

// Define a Printer bitmap
const char* print_xpm[] = {
"18 18 14 1",
"   c None",
".  c #555555",
"w  c #ffffff",
"g  c #A0A0A0",
"h  c #646a63",
"l  c #B0B0B0",
"d  c #404040",
"b  c #6080eF",
"v  c #202020",
"p	c #353034",
"e	c #d5d9d6",
"#	c #838383",
"n	c #C0e0F0",
"m	c #c4c4c4",
"                  ",
"      pppppp      ",
"      pwwwwp      ",
"      pwwwwp      ",
"    ghpwwwwphg    ",
"    g#pwwwwp#g    ",
" ................ ",
" .wwgeeeeeeeegww. ",
" .wegeeeeeeeegew. ",
" .gggllllllllggg. ",
" .mmgllllllllgmm. ",
" .mmvv######vvmm. ",
" .ggvvpnnnnpvvgg. ",
" .....pwbbnp..... ",
"      pwbbnp      ",
"      pwwwwp      ",
"      pppppp      ",
"                  ",
};


// Define a Cancel Printer bitmap
const char* cancel_print_xpm[] = {
"18 18 15 1",
"   c None",
".  c #555555",
"w  c #ffffff",
"g  c #A0A0A0",
"h  c #646a63",
"l  c #B0B0B0",
"d  c #404040",
"b  c #6080eF",
"v  c #202020",
"p	c #353034",
"e	c #d5d9d6",
"#	c #838383",
"n	c #C0e0F0",
"m	c #c4c4c4",
"r	c #d00000",
"                  ",
" r    pppppp    r ",
"  r   pwwwwp   r  ",
"   r  pwwwwp  r   ",
"    rhpwwwwphr    ",
"    grpwwwwprg    ",
" .....r....r..... ",
" .wwgeereereegww. ",
" .wegeeerreeegew. ",
" .ggglllrrlllggg. ",
" .mmgllrllrllgmm. ",
" .mmvvr####rvvmm. ",
" .ggvrpnnnnprvgg. ",
" ...r.pwbbnp.r... ",
"   r  pwbbnp  r   ",
"  r   pwwwwp   r  ",
" r    pppppp    r ",
"                  ",
};

// Define a Printer Error bitmap
const char* error_print_xpm[] = {
"18 18 15 1",
"   c None",
".  c #555555",
"w  c #ffffff",
"g  c #A0A0A0",
"h  c #646a63",
"l  c #B0B0B0",
"d  c #404040",
"b  c #6080eF",
"v  c #202020",
"p	c #353034",
"e	c #d5d9d6",
"#	c #838383",
"n	c #C0e0F0",
"m	c #c4c4c4",
"r	c #FF0000",
"                  ",
"      pprrpp      ",
"      rrrrrr      ",
"    rrrrrrrrrr    ",
"   rrrrrrrrrrrr   ",
"   rrrrrrrrrrrr   ",
" ..rrrrrrrrrrrr.. ",
" .wwrrrrrrrrrrww. ",
" .wegrrrrrrrrgew. ",
" .ggglrrrrrrlggg. ",
" .mmgllrrrrllgmm. ",
" .mmvv#rrrr#vvmm. ",
" .ggvvpnrrnpvvgg. ",
" .....pwbbnp..... ",
"      pwrrnp      ",
"      rrrrrr      ",
"      rrrrrr      ",
"        rr        ",
};

// Define a Printer bitmap
const char* print_xpm2[] = {
"18 18 14 1",
"   c None",
".  c #555555",
"w  c #ffffff",
"g  c #A0A0A0",
"h  c #646a63",
"l  c #B0B0B0",
"d  c #404040",
"b  c #6080eF",
"v  c #202020",
"p	c #353034",
"e	c #d5d9d6",
"#	c #838383",
"n	c #C0e0F0",
"m	c #c4c4c4",
"                  ",
"      pppppp      ",
"      pppppp      ",
"      pwwwwp      ",
"    ghpwwwwphg    ",
"    g#pwwwwp#g    ",
" ................ ",
" .wwgeeeeeeeegww. ",
" .wegeeeeeeeegew. ",
" .gggllllllllggg. ",
" .mmgllllllllgmm. ",
" .mmvv######vvmm. ",
" .ggvvpnnnnpvvgg. ",
" .....pwbbnp..... ",
"      pwbbnp      ",
"      pwwwwp      ",
"      pppppp      ",
"                  ",
};

// Define a Printer bitmap
const char* print_xpm3[] = {
"18 18 15 1",
"   c None",
".  c #555555",
"w  c #ffffff",
"g  c #A0A0A0",
"h  c #646a63",
"l  c #B0B0B0",
"d  c #404040",
"b  c #6080eF",
"v  c #202020",
"p	c #353034",
"e	c #d5d9d6",
"#	c #838383",
"n	c #C0e0F0",
"m	c #c4c4c4",
"y	c #FFFF00",
"                  ",
"      pppppp      ",
"      pwwwwp      ",
"      pppppp      ",
"    ghpwwwwphg    ",
"    g#pwwwwp#g    ",
" ................ ",
" .wwgeeeeeeeegww. ",
" .wegeeeeeeeeyyy. ",
" .gggllllllllyyy. ",
" .mmgllllllllgmm. ",
" .mmvv######vvmm. ",
" .ggvvpnnnnpvvgg. ",
" .....pwbbnp..... ",
"      pwbbnp      ",
"      pwwwwp      ",
"      pppppp      ",
"                  ",
};

// Define a Printer bitmap
const char* print_xpm4[] = {
"18 18 15 1",
"   c None",
".  c #555555",
"w  c #ffffff",
"g  c #A0A0A0",
"h  c #646a63",
"l  c #B0B0B0",
"d  c #404040",
"b  c #6080eF",
"v  c #202020",
"p	c #353034",
"e	c #d5d9d6",
"#	c #838383",
"n	c #C0e0F0",
"m	c #c4c4c4",
"y	c #FFFF00",
"                  ",
"      pppppp      ",
"      pwwwwp      ",
"      pwwwwp      ",
"    ghpppppphg    ",
"    g#pwwwwp#g    ",
" ................ ",
" .wwgeeeeeeeegww. ",
" .wegeeeeeeeeyyy. ",
" .gggllllllllyyy. ",
" .mmgllllllllgmm. ",
" .mmvv######vvmm. ",
" .ggvvpnnnnpvvgg. ",
" .....pppppp..... ",
"      pwbbnp      ",
"      pwwwwp      ",
"      pppppp      ",
"                  ",
};

// Define a Printer bitmap
const char* print_xpm5[] = {
"18 18 14 1",
"   c None",
".  c #555555",
"w  c #ffffff",
"g  c #A0A0A0",
"h  c #646a63",
"l  c #B0B0B0",
"d  c #404040",
"b  c #6080eF",
"v  c #202020",
"p	c #353034",
"e	c #d5d9d6",
"#	c #838383",
"n	c #C0e0F0",
"m	c #c4c4c4",
"                  ",
"      pppppp      ",
"      pwwwwp      ",
"      pwwwwp      ",
"    ghpwwwwphg    ",
"    g#pppppp#g    ",
" ................ ",
" .wwgeeeeeeeegww. ",
" .wegeeeeeeeegew. ",
" .gggllllllllggg. ",
" .mmgllllllllgmm. ",
" .mmvv######vvmm. ",
" .ggvvpnnnnpvvgg. ",
" .....pwwwwp..... ",
"      pppppp      ",
"      pwwwwp      ",
"      pppppp      ",
"                  ",
};


// Define a Printer bitmap
const char* print_xpm6[] = {
"18 18 14 1",
"   c None",
".  c #555555",
"w  c #ffffff",
"g  c #A0A0A0",
"h  c #646a63",
"l  c #B0B0B0",
"d  c #404040",
"b  c #6080eF",
"v  c #202020",
"p	c #353034",
"e	c #d5d9d6",
"#	c #838383",
"n	c #C0e0F0",
"m	c #c4c4c4",
"                  ",
"      pppppp      ",
"      pwwwwp      ",
"      pwwwwp      ",
"    ghpwwwwphg    ",
"    g#pwwwwp#g    ",
" ................ ",
" .wwgeeeeeeeegww. ",
" .wegeeeeeeeegew. ",
" .gggllllllllggg. ",
" .mmgllllllllgmm. ",
" .mmvv######vvmm. ",
" .ggvvpnnnnpvvgg. ",
" .....pwbbnp..... ",
"      pwwwwp      ",
"      pppppp      ",
"      pppppp      ",
"                  ",
};

Fl_Pixmap	gPrinterIcon(print_xpm);
Fl_Pixmap	gPrinterIcon2(print_xpm2);
Fl_Pixmap	gPrinterIcon3(print_xpm3);
Fl_Pixmap	gPrinterIcon4(print_xpm4);
Fl_Pixmap	gPrinterIcon5(print_xpm5);
Fl_Pixmap	gPrinterIcon6(print_xpm6);
Fl_Pixmap	gCancelPrinterIcon(cancel_print_xpm);
Fl_Pixmap	gErrorPrinterIcon(error_print_xpm);
Fl_Pixmap*	gPrinterAnimIcons[8] = {
	&gPrinterIcon, &gPrinterIcon2, &gPrinterIcon3,
	&gPrinterIcon4, &gPrinterIcon5, &gPrinterIcon6
};

extern "C" time_t one_sec_time;

/*
=======================================================
Define global variables
=======================================================
*/
VTLpt			*gLpt = NULL;
lptCtrl_t		gLptCtrl;
lpt_prefs_t		gLptPrefs;
int				gAnimationNeeded = FALSE;

/*
=======================================================
Initialize the printer subsystem
=======================================================
*/
void init_lpt(void)
{
	// If the LPT object doesn't exist, create it
	if (gLpt == NULL)
	{
		gLpt = new VTLpt();
		gLpt->UpdatePreferences(&virtualt_prefs);
	}

	gpPrint->label("Idle");

}

/*
=======================================================
Deinitialize the printer subsystem
=======================================================
*/
void deinit_lpt(void)
{
	// Deinitialize only if the object exists
	if (gLpt != NULL)
	{
		gLpt->DeinitLpt();
		delete gLpt;
		gLpt = NULL;
	}
}

/*
=======================================================
Send a byte to the printer subsystem.
=======================================================
*/
void send_to_lpt(unsigned char byte)
{
	if (gLpt != NULL)
	{
		gLpt->SendToLpt(byte);
	}
}

/*
=======================================================
Check for errors in the LPT system
=======================================================
*/
void lpt_check_errors()
{
	if (gLpt != NULL)
	{
		gLpt->CheckErrors();
	}
}

/*
=======================================================
Do lpt animation
=======================================================
*/
void lpt_perodic_update()
{
	// Check if printer animation needed
	if (gLpt != NULL)
	{
		// Check if animation neded
		if (gAnimationNeeded)
			gLpt->DoAnimation();

		// Perform perodic LPT update
		gLpt->PerodicUpdate();
	}
}

/*
=======================================================
Setup a monitor callback routine
=======================================================
*/
extern "C"
{
void lpt_set_monitor_callback(lpt_monitor_cb pCallback)
{
	if (gLpt != NULL)
	{
		gLpt->SetMonitorCallback(pCallback);
	}
}
}

/*
=======================================================
Handle LPT device timeouts
=======================================================
*/
void handle_lpt_timeout(unsigned long time)
{
	if (gLpt != NULL)
		gLpt->HandleTimeouts(time);
}

/*
=======================================================
Callback routines for radio butons
=======================================================
*/
void cb_lpt_radio_none(Fl_Widget* w, void*)
{
	gLptCtrl.pEmulPrint->deactivate();
	gLptCtrl.pCRtoLF->deactivate();
	gLptCtrl.pAutoFF->deactivate();
	gLptCtrl.pAFFtimeout->deactivate();
	gLptCtrl.pAutoClose->deactivate();
	gLptCtrl.pCloseTimeout->deactivate();
}

void cb_lpt_radio_emul(Fl_Widget* w, void*)
{
	gLptCtrl.pEmulPrint->activate();
	gLptCtrl.pCRtoLF->activate();
	gLptCtrl.pAutoFF->activate();
	gLptCtrl.pAFFtimeout->activate();
	gLptCtrl.pAutoClose->activate();
	gLptCtrl.pCloseTimeout->activate();
}

/*
=======================================================
Callback routine for Printer Properties menu item
=======================================================
*/
void cb_printer_properties(Fl_Widget* w, void*)
{
	// Test if printer emulation is enabled

	// Pass the Printer Properties task to the C++ obj
	if (gLpt != NULL)
		gLpt->PrinterProperties(FALSE);
}

void cb_LptPrintSetup(Fl_Widget* w, void*)
{
	if (gLpt != NULL)
		gLpt->PrinterProperties(TRUE);
}

void cb_LptCancelPrint(Fl_Widget* w, void*)
{
	if (gLpt != NULL)
		gLpt->CancelPrintJob();
}

void cb_LptPrintNow(Fl_Widget* w, void*)
{
	if (gLpt != NULL)
		gLpt->EndPrintSession();
}

void cb_LptResetPrint(Fl_Widget* w, void*)
{
	if (gLpt != NULL)
		gLpt->ResetPrinter();
}

void cb_LptShowErrors(Fl_Widget* w, void*)
{
	if (gLpt != NULL)
		gLpt->ShowErrors();
}

/*
====================================================================
Define a menu for printer icon popup
====================================================================
*/
Fl_Menu_Item	gPrintMenu[] = {
	{ "Invisible Item",  0,     0,                   0,   FL_MENU_INVISIBLE},
	{ "Print Now",       0,     cb_LptPrintNow,      0,   FL_MENU_INVISIBLE},
	{ "Cancel Print",    0,     cb_LptCancelPrint,   0,   FL_MENU_INVISIBLE},
	{ "Show Errors",     0,     cb_LptShowErrors,    0,   FL_MENU_INVISIBLE},
	{ "Reset Printer",   0,     cb_LptResetPrint,    0,   0},
	{ "Printer Setup",   0,     cb_LptPrintSetup,    0,   0},
	{ 0 }
};

/*
=======================================================
Build the LPT tab on the Peripheral Setup dialog
=======================================================
*/
void build_lpt_setup_tab(void)
{
	int 		c, count;
	MString		name;
	char		value[10];

	// Create Radio button for "no emulation"
	gLptCtrl.pNone = new Fl_Round_Button(20, 40, 110, 20, "No emulation");
	gLptCtrl.pNone->type(FL_RADIO_BUTTON);
	gLptCtrl.pNone->callback(cb_lpt_radio_none);

	// Create Radio button for LPT emulation
	gLptCtrl.pEmul = new Fl_Round_Button(20, 65, 160, 20, "Connect LPT to:");
	gLptCtrl.pEmul->type(FL_RADIO_BUTTON);
	gLptCtrl.pEmul->callback(cb_lpt_radio_emul);

	// Create control to choose the printer emulation mode
	gLptCtrl.pEmulPrint = new Fl_Choice(50, 90, 220, 20, "");
	if (gLptPrefs.lpt_mode != LPT_MODE_EMUL)
		gLptCtrl.pEmulPrint->deactivate();

	gLptCtrl.pCRtoLF = new Fl_Check_Button(20, 120, 200, 20, "Convert lonely CR to LF");
	gLptCtrl.pCRtoLF->value(gLptPrefs.lpt_cr2lf);
	if (gLptPrefs.lpt_mode != LPT_MODE_EMUL)
		gLptCtrl.pCRtoLF->deactivate();

	gLptCtrl.pAutoFF = new Fl_Check_Button(20, 145, 240, 20, "Auto FormFeed after timeout (sec):");
	gLptCtrl.pAutoFF->value(gLptPrefs.lpt_auto_ff);
	if (gLptPrefs.lpt_mode != LPT_MODE_EMUL)
		gLptCtrl.pAutoFF->deactivate();

	gLptCtrl.pAFFtimeout = new Fl_Input(280, 145, 40, 20, "");
	sprintf(value, "%d", gLptPrefs.lpt_aff_timeout);
	gLptCtrl.pAFFtimeout->value(value);
	if (gLptPrefs.lpt_mode != LPT_MODE_EMUL)
		gLptCtrl.pAFFtimeout->deactivate();

	gLptCtrl.pAutoClose = new Fl_Check_Button(20, 170, 240, 20, "Close Session after timeout (sec):");
	gLptCtrl.pAutoClose->value(gLptPrefs.lpt_auto_close);
	if (gLptPrefs.lpt_mode != LPT_MODE_EMUL)
		gLptCtrl.pAutoClose->deactivate();

	gLptCtrl.pCloseTimeout = new Fl_Input(280, 170, 40, 20, "");
	sprintf(value, "%d", gLptPrefs.lpt_close_timeout);
	gLptCtrl.pCloseTimeout->value(value);
	if (gLptPrefs.lpt_mode != LPT_MODE_EMUL)
		gLptCtrl.pCloseTimeout->deactivate();


	// Add help text for setting up printer preferences
	Fl_Button* b = new Fl_Button(20, 210, 120, 30, "Printer Setup");
	b->callback(cb_printer_properties);
	//Fl_Box* o = new Fl_Box(20, 220, 300, 20, "Setup Printer Preferences from File menu");
	//o->align(FL_ALIGN_LEFT | FL_ALIGN_INSIDE);

	// Add items to the EmulPrint choice box
	count = gLpt->GetPrinterCount();
	for (c = 0; c < count; c++)
	{
		// Add name to the Choice list
		name = gLpt->GetPrinter(c)->GetName();
		gLptCtrl.pEmulPrint->add((const char *) name);

		// Select the active printer in list
		if (name == gLptPrefs.lpt_emul_name) 
			gLptCtrl.pEmulPrint->value(c);
	}

	// Select the correct radio button base on preferences
	if (gLptPrefs.lpt_mode == LPT_MODE_NONE)
		gLptCtrl.pNone->value(1);
	else if (gLptPrefs.lpt_mode == LPT_MODE_EMUL)
		gLptCtrl.pEmul->value(1);
}

/*
=============================================================================
load_lpt_preferences:	Loads the emulated printer preferences from the Host.
=============================================================================
*/
void load_lpt_preferences(Fl_Preferences* pPref)
{
	// Load the preferences
	pPref->get("LptMode", gLptPrefs.lpt_mode, 0);
	pPref->get("LptEmulName", gLptPrefs.lpt_emul_name, "", 64);
	pPref->get("LptCRtoLF", gLptPrefs.lpt_cr2lf, 1);
	pPref->get("LptAutoFF", gLptPrefs.lpt_auto_ff, 0);
	pPref->get("LptAFFtimeout", gLptPrefs.lpt_aff_timeout, 10);
	pPref->get("LptAutoClose", gLptPrefs.lpt_auto_close, 0);
	pPref->get("LptCloseTimeout", gLptPrefs.lpt_close_timeout, 15);

	if (gLpt != NULL)
		gLpt->UpdatePreferences(pPref);
}

/*
=============================================================================
save_lpt_preferences:	Saves the emulated printer preferences to the Host.
=============================================================================
*/
void save_lpt_preferences(Fl_Preferences* pPref)
{
	pPref->set("LptMode", gLptPrefs.lpt_mode);
	pPref->set("LptEmulName", gLptPrefs.lpt_emul_name);
	pPref->set("LptCRtoLF", gLptPrefs.lpt_cr2lf);
	pPref->set("LptAutoFF", gLptPrefs.lpt_auto_ff);
	pPref->set("LptAFFtimeout", gLptPrefs.lpt_aff_timeout);
	pPref->set("LptAutoClose", gLptPrefs.lpt_auto_close);
	pPref->set("LptCloseTimeout", gLptPrefs.lpt_close_timeout);

	// Update the emulation properties in the Printer Class
	if (gLpt != NULL)
		gLpt->UpdatePreferences(pPref);
}


/*
=============================================================================
get_lpt_options:	Gets the LPT options from the Dialog Tab and saves them
					to the preferences.  This routien is called when the
					user selects "Ok".
=============================================================================
*/
void get_lpt_options()
{
	// End any active print sessions
	if (gLpt != NULL)
		gLpt->EndPrintSession();

	// Check if "No Emulation" is selected
	if (gLptCtrl.pNone->value())
		gLptPrefs.lpt_mode = LPT_MODE_NONE;

	// Check if "Emulated Printer" Emulation is selected
	if (gLptCtrl.pEmul->value())
		gLptPrefs.lpt_mode = LPT_MODE_EMUL;

	if (gLptPrefs.lpt_mode == LPT_MODE_NONE)
		gpPrint->label("None");
	else
		gpPrint->label("Idle");

	// Get selection of the emulated printer
	gLptPrefs.lpt_emul_name[0] = 0;
	if (gLptCtrl.pEmulPrint->value() != -1)
		strcpy(gLptPrefs.lpt_emul_name, gLptCtrl.pEmulPrint->text());

	// Get the CRtoLF parameter
	gLptPrefs.lpt_cr2lf = gLptCtrl.pCRtoLF->value();

	// Get AutoFF parameter
	gLptPrefs.lpt_auto_ff = gLptCtrl.pAutoFF->value();

	// Get the AutoFFtimeout value
	gLptPrefs.lpt_aff_timeout = atoi(gLptCtrl.pAFFtimeout->value());

	// Get AutoClose parameter
	gLptPrefs.lpt_auto_close = gLptCtrl.pAutoClose->value();

	// Get the CloseTimeout value
	gLptPrefs.lpt_close_timeout = atoi(gLptCtrl.pCloseTimeout->value());
}

/*
================================================================================
VTLpt:	This is the class construcor for the LPT Device emulation.
================================================================================
*/
VTLpt::VTLpt(void)
{
	m_EmulationMode = 0;
	m_ConvertCRtoLF = 0;
	m_PortStatus = LPT_STATUS_IDLE;
	m_PortTimeout = 0;
	m_AFFSent = FALSE;
	m_pMonCallback = NULL;
	m_animIconIndex = 1;

	// Now create printers
	VTPrinter *pPrint = new VTFilePrint();
	m_Printers.Add(pPrint);

	pPrint = new VTHostPrint();
	m_Printers.Add(pPrint);

	pPrint = new VTFX80Print();
	m_Printers.Add(pPrint);

	// Default to no active printer.  It will be selected later during
	// Preferences update.
	m_pActivePrinter = NULL;

	m_pPref = NULL;
}

/*
=======================================================
SendToPrinter:	Sends a byte to the printer for further
				emulation processing.
=======================================================
*/
void VTLpt::SendToLpt(unsigned char byte)
{
	int		ret = PRINT_ERROR_NONE;

	// Check if we are emulating a printer or not
	if (!m_EmulationMode || (m_PortStatus == LPT_STATUS_ABORTED) ||
		(m_PortStatus == LPT_STATUS_ERROR))
			return;

	if (m_pActivePrinter == NULL)
		return;

	// Test if byte is CR and to CRtoLF if needed
	if (m_ConvertCRtoLF)
	{
		if (m_PrevChar == 0x0D)
		{
#ifndef WIN32
			// Print the "reserved" 0x0D
			ret = m_pActivePrinter->Print(0x0D);

			// Log data sent to port
			if (m_pMonCallback != NULL)
				m_pMonCallback(LPT_MON_PORT_WRITE, 0x0D);

#endif

			// If the current byte isn't 0x0A, send a 0x0A
			if ((byte != 0x0A) && (ret == PRINT_ERROR_NONE))
				ret = m_pActivePrinter->Print(0x0A);

			// Log data sent to port
			if (m_pMonCallback != NULL)
				m_pMonCallback(LPT_MON_PORT_WRITE, 0x0A);
		}

		// Test if this byte is 0x0D in case there are multiple in a row
		if (byte == 0x0D)
		{
			m_PrevChar = 0x0D;
			return;
		}
	}
	
	if (ret == PRINT_ERROR_NONE)
	{
		// Send byte to the active printer
		ret = m_pActivePrinter->Print(byte);

		// Log data sent to port
		if (m_pMonCallback != NULL)
			m_pMonCallback(LPT_MON_PORT_WRITE, byte);
	}

	// Check if printer operation cancelled
	if (ret == PRINT_ERROR_ABORTED)
	{
		m_PortStatus = LPT_STATUS_ABORTED;
		gpPrint->label("Abrt");
		gpPrint->set_image(&gCancelPrinterIcon);
		gAnimationNeeded = FALSE;
		m_animIconIndex = 1;
		time(&m_PortTimeout);

		// Report port status change
		if (m_pMonCallback != NULL)
			m_pMonCallback(LPT_MON_PORT_STATUS_CHANGE, 0);
		return;
	}

	// Check if printer reported an error
	if (ret == PRINT_ERROR_IO_ERROR)
	{
		m_PortStatus = LPT_STATUS_ERROR;
		gpPrint->label("Err");
		gpPrint->set_image(&gErrorPrinterIcon);
		// Enable the errors menu
		gPrintMenu[3].flags = 0;
		gAnimationNeeded = FALSE;
		m_animIconIndex = 1;

		// Report port status change
		if (m_pMonCallback != NULL)
			m_pMonCallback(LPT_MON_PORT_STATUS_CHANGE, 0);
		return;
	}

	time(&m_PortActivity);
	int oldStatus = m_PortStatus;
	if (m_PortStatus != LPT_STATUS_ACTIVITY)
	{
		gpPrint->label("Actv");
		gPrintMenu[2].flags = 0;
		gPrintMenu[1].flags = 0;
	}

	m_PortStatus = LPT_STATUS_ACTIVITY;
	m_AFFSent = FALSE;
	m_PrevChar = byte;
	gAnimationNeeded = TRUE;

		// Report port status change
	if (oldStatus != m_PortStatus)
		if (m_pMonCallback != NULL)
			m_pMonCallback(LPT_MON_PORT_STATUS_CHANGE, 0);
}

/*
=======================================================
Handle LPT device timeouts.
=======================================================
*/
void VTLpt::HandleTimeouts(unsigned long time)
{
	unsigned long		timeout;

	// Check if port is IDLE
	if ((m_PortStatus == LPT_STATUS_IDLE) || 
		(m_PortStatus == LPT_STATUS_ERROR))
			return;

	// Check if we are emulating a printer
	if (m_EmulationMode == LPT_MODE_NONE)
		return;

	// Check for aborted print timeout
	if (m_PortStatus == LPT_STATUS_ABORTED)
	{
		if ((gLptPrefs.lpt_close_timeout > 0) && (gLptPrefs.lpt_close_timeout < 60))
			timeout = gLptPrefs.lpt_close_timeout;
		else
			timeout = 15;

		// Check if we have timed out or not
		if (m_PortTimeout + timeout <= time)
		{
			m_PortStatus = LPT_STATUS_IDLE;
			gpPrint->set_image(&gPrinterIcon);
			gpPrint->label("Idle");
			gAnimationNeeded = FALSE;
			m_animIconIndex = 1;

			// Report port status change
			if (m_pMonCallback != NULL)
				m_pMonCallback(LPT_MON_PORT_STATUS_CHANGE, 0);
		}
		else
		{
			sprintf(m_TimeStr, "%d", m_PortTimeout + timeout - time);
			gpPrint->label(m_TimeStr);
		}


		// Nothing else to do...port is in timeout mode
		return;
	}

	// Check for Auto FormFeed timeout
	if ((gLptPrefs.lpt_auto_ff) && (!m_AFFSent))
	{
		// Get AFF Timeout value and check for bounds
		if ((gLptPrefs.lpt_aff_timeout > 0) && (gLptPrefs.lpt_aff_timeout < 60))
			timeout = gLptPrefs.lpt_aff_timeout;
		else
			timeout = 15;

		// Check if timeout has expired
		if (m_PortActivity + timeout <= time)
		{
			// Mark AFF as sent so we only send one
			m_AFFSent = TRUE;
			if (m_pActivePrinter != NULL)
				m_pActivePrinter->SendAutoFF();
		}
	}

	// Check for session close timeout
	if (gLptPrefs.lpt_auto_close)
	{
		// Get Auto Close Timeout value and check for bounds
		if ((gLptPrefs.lpt_close_timeout > 0) && (gLptPrefs.lpt_close_timeout < 60))
			timeout = gLptPrefs.lpt_close_timeout;
		else
			timeout = 15;

		// Test if the auto session close timeout has expired
		if (m_PortActivity + timeout <= time)
		{
			// Ensure we have an valid printer pointer
			if (m_pActivePrinter != NULL)
				m_pActivePrinter->EndPrintSession();

			// Change the port status
			m_PortStatus = LPT_STATUS_IDLE;
			m_PrevChar = 0;
			gpPrint->label("Idle");
			gpPrint->set_image(&gPrinterIcon);
			gAnimationNeeded = FALSE;
			m_animIconIndex = 1;
			gPrintMenu[2].flags = FL_MENU_INVISIBLE;
			gPrintMenu[1].flags = FL_MENU_INVISIBLE;

			// Report port status change
			if (m_pMonCallback != NULL)
				m_pMonCallback(LPT_MON_PORT_STATUS_CHANGE, 0);
		}
		else
		{
			sprintf(m_TimeStr, "%d", m_PortActivity + timeout - time);
			gpPrint->label(m_TimeStr);
		}
	}
	// Check if we need to cancel activity animation
	if ((m_PortStatus == LPT_STATUS_ACTIVITY) && (m_PortActivity + 1 <= time))
	{
		// Change the port status to READY.  Animation will stop automatically
		m_PortStatus = LPT_STATUS_READY;
		gpPrint->label("Pend");
		gpPrint->set_image(&gPrinterIcon);
		gAnimationNeeded = FALSE;
		m_animIconIndex = 1;
	}
}

/*
=======================================================
DeinitPrinter:	Deinitialized the printer subsystem
				and cleans up all memory, etc.
=======================================================
*/
void VTLpt::DeinitLpt(void)
{
	int 		count, c;
	VTPrinter*	pPrint;

	// Close any active print session
	if (m_PortStatus == LPT_STATUS_READY)
		if (m_pActivePrinter != NULL)
			m_pActivePrinter->EndPrintSession();

	// Delete all registered printer objects
	count = m_Printers.GetSize();
	for (c = 0; c < count; c++)
	{
		pPrint = (VTPrinter*) m_Printers.GetAt(c);
		pPrint->Deinit();
		delete pPrint;
	}

	m_Printers.RemoveAll();
}


/*
=========================================================================
Callback routines for the PrinterProperties dialog
=========================================================================
*/
void cb_PrintProp_Ok(Fl_Widget*w, void* printer)
{
	gLpt->PrinterPropOk((VTPrinter*) printer);
}

void cb_PrintProp_Cancel(Fl_Widget*w, void*)
{
	gLpt->PrinterPropCancel();
}

/*
=========================================================================
Generates a Printer Properties dialog box based on the current emulation
mode with parameters appropriate for the "printer" needs.
=========================================================================
*/
void VTLpt::PrinterProperties(int useActivePrinter)
{
	int			count, c;
	int			found;
	VTPrinter*	pPrint = NULL;

	// Test if printer emulation is enabled
	if (useActivePrinter)
	{
		if (m_EmulationMode == LPT_MODE_NONE)
		{
			fl_message("Printer emulation not enabled");
			return;
		}
		if (m_pActivePrinter == NULL)
		{
			fl_message("Printer not selected");
			return;
		}
	}
	else
	{
		if (!gLptCtrl.pEmul->value())
		{
			fl_message("Printer emulation not enabled");
			return;
		}
		if (gLptCtrl.pEmulPrint->value() == -1)
		{
			fl_message("Printer not selected");
			return;
		}
	}

	// Create a dialog box first
	m_pProp = new Fl_Window(400, 350, "Printer Properties");
	
	// Create property page from selected printer
	count = GetPrinterCount();
	for (c = 0; c < count; c++)
	{
		// Get pointer to next printer
		pPrint = GetPrinter(c);

		found = 0;
		if (useActivePrinter)
		{
			if (pPrint == m_pActivePrinter)
				found = 1;
		}
		else
		{
			if (strcmp(gLptCtrl.pEmulPrint->text(), (const char *) 
				pPrint->GetName()) == 0)
					found = 1;
		}
		if (found)
		{
			if (pPrint != m_pActivePrinter)
				pPrint->Init(m_pPref);
			pPrint->BuildPropertyDialog(m_pProp);
			if (pPrint != m_pActivePrinter)
				pPrint->Deinit();
			break;
		}
	}

	// Create Ok and Cancel button
	m_pCancel = new Fl_Button(m_pProp->w() - 165, m_pProp->h()-40, 60, 30, "Cancel");
	m_pCancel->callback(cb_PrintProp_Cancel);

	m_pOk = new Fl_Return_Button(m_pProp->w()-90, m_pProp->h()-40, 60, 30, "Ok");
	m_pOk->callback(cb_PrintProp_Ok, pPrint);

	// Show the dialog box
	m_pProp->end();
	m_pProp->show();
	m_pProp->redraw();
}

/*
=========================================================================
Updates the Preferences from the global gLptPrefs object and sets up any
necessary variables for emulation if the modes change.
=========================================================================
*/
void VTLpt::UpdatePreferences(Fl_Preferences* pPref)
{
	int			count, c;
	VTPrinter*	pPrint;

	// Save the preferences pointer
	if (pPref != NULL)
		m_pPref = pPref;

	// Test if the emulation mode is being changed
	if (m_EmulationMode != gLptPrefs.lpt_mode)
	{
		// Test if an active print job is still pending completion
		if (m_pActivePrinter != NULL)
		{
			// Test if the printer is changing
			if (m_pActivePrinter->GetName() != gLptPrefs.lpt_emul_name)
				m_pActivePrinter->EndPrintSession();
			m_pActivePrinter->Deinit();
		}
	}

	// Set the new emulation mode
	m_EmulationMode = gLptPrefs.lpt_mode;
	m_ConvertCRtoLF = gLptPrefs.lpt_cr2lf;

	// If emulation is enabled, get pointer to the Priner Class 
	m_pActivePrinter = NULL;
	if (m_EmulationMode == LPT_MODE_EMUL)
	{
		count = GetPrinterCount();
		for (c = 0; c < count; c++)
		{
			// Get pointer to next printer
			pPrint = GetPrinter(c);
			if (strcmp(gLptPrefs.lpt_emul_name, (const char *) 
				pPrint->GetName()) == 0)
			{
				m_pActivePrinter = pPrint;
				break;
			}
		}
	}

	// Initialize the active printer
	if (m_pActivePrinter != NULL)
		m_pActivePrinter->Init(m_pPref);

	if (m_pMonCallback != NULL)
		m_pMonCallback(LPT_MON_EMULATION_CHANGE, 0);
}

/*
=========================================================================
Returns the number of printers registered with the Lpt object
=========================================================================
*/
int VTLpt::GetPrinterCount(void)
{
	return m_Printers.GetSize();
}

/*
=========================================================================
Returns a pointer to a specific printer
=========================================================================
*/
VTPrinter* VTLpt::GetPrinter(int index)
{
	if (index >= m_Printers.GetSize())
		return NULL;

	return (VTPrinter*) m_Printers.GetAt(index);
}

/*
=========================================================================
Handles the "Ok" button of the Printer Properties Dialog.
=========================================================================
*/
void VTLpt::PrinterPropOk(VTPrinter* pPrint)
{
	int		err;

	// Ensure the dialog exists
	if (m_pProp == NULL)
		return;

	// Tell the active printer to update and validate preferences
	if (pPrint != NULL)
	{
		if ((err = pPrint->GetProperties()))
		{
			// Handle error

			return;
		}
		if (pPrint != m_pActivePrinter)
			pPrint->Deinit();
	}

	// Hide and destroy the dialog
	m_pProp->hide();
	delete m_pProp;
}

/*
=========================================================================
Handles the "Cancel" button of the Printer Properties Dialog.
=========================================================================
*/
void VTLpt::PrinterPropCancel(void)
{
	// Kill the dialog box if it exists
	if (m_pProp != NULL)
	{
		m_pProp->hide();
		delete m_pProp;
		m_pProp = NULL;
	}
}

/*
=========================================================================
End the current print session and send to the "printer".
=========================================================================
*/
void VTLpt::EndPrintSession(void)
{
	if ((m_pActivePrinter != NULL) && (m_PortStatus != LPT_STATUS_IDLE))
	{
		m_pActivePrinter->EndPrintSession();
		gpPrint->label("Idle");
		gpPrint->set_image(&gPrinterIcon);
		gPrintMenu[3].flags = FL_MENU_INVISIBLE;
		gPrintMenu[2].flags = FL_MENU_INVISIBLE;
		gPrintMenu[1].flags = FL_MENU_INVISIBLE;
		m_PortStatus = LPT_STATUS_IDLE;
		m_PrevChar = 0;
		gAnimationNeeded = FALSE;
		m_animIconIndex = 1;

		// Report port status change
		if (m_pMonCallback != NULL)
			m_pMonCallback(LPT_MON_PORT_STATUS_CHANGE, 0);
	}
}

/*
=========================================================================
Reset the ABORT status of the printer
=========================================================================
*/
void VTLpt::ResetPrinter(void)
{
	// Reset the printer
	if (m_pActivePrinter != NULL)
		m_pActivePrinter->ResetPrinter();

	// Reset the LPT emulation too
	gpPrint->set_image(&gPrinterIcon);
	gpPrint->label("Idle");
	gPrintMenu[3].flags = FL_MENU_INVISIBLE;
	m_PortStatus = LPT_STATUS_IDLE;
	m_PrevChar = 0;
	gAnimationNeeded = FALSE;
	m_animIconIndex = 1;

	// Report port status change
	if (m_pMonCallback != NULL)
		m_pMonCallback(LPT_MON_PORT_STATUS_CHANGE, 0);
}

/*
=========================================================================
Cancel the current rint job.
=========================================================================
*/
void VTLpt::CancelPrintJob(void)
{
	if (m_pActivePrinter != NULL)
		m_pActivePrinter->CancelPrintJob();

	gpPrint->label("Idle");
	m_PortStatus = LPT_STATUS_IDLE;
	m_PrevChar = 0;
	gAnimationNeeded = FALSE;
	m_animIconIndex = 1;
	gPrintMenu[2].flags = FL_MENU_INVISIBLE;
	gPrintMenu[1].flags = FL_MENU_INVISIBLE;

	// Report port status change
	if (m_pMonCallback != NULL)
		m_pMonCallback(LPT_MON_PORT_STATUS_CHANGE, 0);
}

/*
=========================================================================
Checks for errors and updates icon.
=========================================================================
*/
int VTLpt::CheckErrors(void)
{
	int		errs = 0;

	// If we have an active printer, get error count from printer
	if (m_pActivePrinter != NULL)
		errs = m_pActivePrinter->GetErrorCount();

	// If there are errors, change the icon
	if (errs > 0)
	{
		if (gpPrint->get_image() != &gErrorPrinterIcon)
		{
			gpPrint->set_image(&gErrorPrinterIcon);
			gpPrint->label("Err");

			// Enable the errors menu
			gPrintMenu[3].flags = 0;
		}
	
		m_PortStatus = LPT_STATUS_ERROR;
		gAnimationNeeded = FALSE;
		m_animIconIndex = 1;

		// Report port status change
		if (m_pMonCallback != NULL)
			m_pMonCallback(LPT_MON_PORT_STATUS_CHANGE, 0);
	}

	return errs;
}

/*
=========================================================================
Checks for errors and updates icon.
=========================================================================
*/
void VTLpt::ShowErrors(void)
{
	int		c, count;

	if (m_pActivePrinter == NULL)
		return;

	count = m_pActivePrinter->GetErrorCount();

	if (count == 1)
	{
		fl_message((const char *) m_pActivePrinter->GetError(0));
	}
	else
	{
		Fl_Text_Buffer* tb = new Fl_Text_Buffer();
		for (c = 0; c < count; c++)
		{
			tb->append(m_pActivePrinter->GetError(c));
			tb->append("\n");
		}
		Fl_Window*	o = new Fl_Window(500, 300, "Printer Errors");
		Fl_Text_Display* td = new Fl_Text_Display(0, 0, 500, 300, "");
		td->buffer(tb);
		o->show();
	}

	// Clear the errors
	m_pActivePrinter->ClearErrors();

	// Set the icon back to normal
	gpPrint->set_image(&gPrinterIcon);
	gpPrint->label("Idle");
	m_PortStatus = LPT_STATUS_IDLE;
	gPrintMenu[3].flags = FL_MENU_INVISIBLE;
	gAnimationNeeded = FALSE;
	m_animIconIndex = 1;

	// Report port status change
	if (m_pMonCallback != NULL)
		m_pMonCallback(LPT_MON_PORT_STATUS_CHANGE, 0);
}

/*
=========================================================================
Returns a string indicating the port emulation mode
=========================================================================
*/
MString VTLpt::GetEmulationMode(void)
{
	MString		str;

	if (m_EmulationMode == LPT_MODE_NONE)
		str = "Disabled";
	else
	{
		if (m_pActivePrinter != NULL)
			str = m_pActivePrinter->GetName();
		else
			str = "Error - Unknown print emulation";
	}

	return str;
}

/*
=========================================================================
Returns a string indicating the current port status
=========================================================================
*/
MString VTLpt::GetPortStatus(void)
{
	MString		str;

	switch (m_PortStatus)
	{
	case LPT_STATUS_IDLE:
		str = "Idle - No active session";
		break;

	case LPT_STATUS_READY:
		str = "Pending - Session active";
		break;
	
	case LPT_STATUS_ABORTED:
		str = "Print aborted";
		break;

	case LPT_STATUS_ERROR:
		str = "Printer Error";
		break;

	case LPT_STATUS_ACTIVITY:
		str = "Active - Receiving data";
		break;

	default:
		str = "Unknown port status";
		break;
	}

	return str;
}

/*
=========================================================================
Sets a monitor callback routine for port change reporting
=========================================================================
*/
void VTLpt::SetMonitorCallback(lpt_monitor_cb pCallback)
{
	m_pMonCallback = pCallback;
}

/*
=========================================================================
Build the Monitor Tab controls for the specified printer
=========================================================================
*/
void VTLpt::BuildPrinterMonTab(int printer)
{
	if (m_EmulationMode != LPT_MODE_NONE)
		if (printer < m_Printers.GetSize())
			((VTPrinter*) m_Printers[printer])->BuildMonTab();
}

/*
=========================================================================
Get the name of the specified printer
=========================================================================
*/
MString VTLpt::GetPrinterName(int printer)
{
	MString		name;

	if (printer < m_Printers.GetSize())
		name = ((VTPrinter*) m_Printers[printer])->GetName();

	return name;
}

/*
=========================================================================
Get the index of the active printer or -1 if none
=========================================================================
*/
int VTLpt::GetActivePrinterIndex(void)
{
	int		c, count;

	if (m_pActivePrinter == NULL)
		return -1;

	// Loop through the printer devices and find the index of active printer
	count = m_Printers.GetSize();
	for (c = 0; c < count; c++)
		if ((VTPrinter*) m_Printers[c] == m_pActivePrinter)
			return c;

	return -1;
}

/*
=========================================================================
Do Printer icon animation
=========================================================================
*/
void VTLpt::DoAnimation(void)
{
	gpPrint->set_image(gPrinterAnimIcons[m_animIconIndex++]);
	if (m_animIconIndex >= 6)
		m_animIconIndex = 0;
	else
		if ((m_animIconIndex == 1) && (m_PortStatus != LPT_STATUS_ACTIVITY))
			gAnimationNeeded = FALSE;
}

/*
=========================================================================
Perform perodic update operations on LPT
=========================================================================
*/
void VTLpt::PerodicUpdate(void)
{
	if (m_pMonCallback != NULL)
		if (m_pActivePrinter != NULL)
			m_pActivePrinter->UpdateMonTab();
}

