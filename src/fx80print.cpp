/* fx80print.cpp */

/* $Id: fx80print.cpp,v 1.1 2008/02/17 13:25:27 kpettit1 Exp $ */

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
#include <FL/Fl_Double_Window.H>
#include <FL/Fl_Round_Button.H>
#include <FL/Fl_Check_Button.H>
#include <FL/Fl_Return_Button.H>
#include <FL/Fl_Input.H>
#include <FL/Fl_Choice.H>
#include <FL/Fl_Box.H>
#include <FL/Fl_Preferences.H>
#include <FL/fl_ask.H>

#include <string.h>
#include <stdio.h>
#include <stdlib.h>

#include "VirtualT.h"
#include "fx80print.h"
#include "chargen.h"

/*
================================================================================
VTFX80Print:	This is the class construcor for the FX80Print Device emulation.
================================================================================
*/
VTFX80Print::VTFX80Print(void)
{
}

/*
=======================================================
GetName:	Returns the name of the printer.
=======================================================
*/
MString VTFX80Print::GetName()
{
	return "Emulated FX-80";
}

/*
=======================================================
Print a byte to the FX80 Emulation
=======================================================
*/
void VTFX80Print::PrintByte(unsigned char byte)
{
}

/*
=======================================================
Build the Property Dialog for the FX-80
=======================================================
*/
void VTFX80Print::BuildPropertyDialog(void)
{
	Fl_Box*		o;

	o = new Fl_Box(20, 20, 360, 20, "Emulated Epson FX-80 Printer");

	Fl_Button *b = new Fl_Button(20, 300, 120, 30, "Char Generator");
	b->callback(cb_CreateNewCharGen);
}

/*
=======================================================
Get the Busy status 
=======================================================
*/
int VTFX80Print::GetBusyStatus(void)
{
	return FALSE;
}

/*
=======================================================
Init routine for reading prefs, setting up, etc.
=======================================================
*/
void VTFX80Print::Init(void)
{
	return;
}

/*
=======================================================
Opens a new Print Session
=======================================================
*/
int VTFX80Print::OpenSession(void)
{
	return PRINT_ERROR_NONE;
}

/*
=======================================================
Closes the active Print Session
=======================================================
*/
int VTFX80Print::CloseSession(void)
{
	return PRINT_ERROR_NONE;
}

/*
=======================================================
Get Printer Properties from dialog and save.
=======================================================
*/
int VTFX80Print::GetProperties(void)
{
	return PRINT_ERROR_NONE;
}

/*
=======================================================
Send FormFeed if last byte sent not 0x0C.
=======================================================
*/
void VTFX80Print::SendAutoFF(void)
{
	return;
}

/*
=======================================================
Deinit the printer
=======================================================
*/
void VTFX80Print::Deinit(void)
{
	return;
}

/*
=======================================================
Cancels the current print job.
=======================================================
*/
int VTFX80Print::CancelPrintJob(void)
{
	return PRINT_ERROR_NONE;
}

