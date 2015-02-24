/* tdock.cpp */

/* $Id: tdock.cpp,v 1.9 2015/02/12 01:15:57 kpettit1 Exp $ */

/*
 * Copyright 2015 Ken Pettit
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

#include <sys/types.h>
#include <stdio.h>
#include <string.h>
#ifdef _WIN32
#include <windows.h>
#include <direct.h>
#else
#include <sys/stat.h>
#include <unistd.h>
#endif

#include <FL/Fl.H>
#include <FL/Fl_Preferences.H>
#include <FL/Fl_Button.H>
#include <FL/Fl_Return_Button.H>
#include <FL/Fl_Menu_Bar.H>
#include <FL/Fl_Box.H>
#include <FL/Fl_Input.H>

#include "FLU/Flu_File_Chooser.h"

#include "VirtualT.h"
#include "m100emu.h"
#include "tdock.h"
#include "tdockvid.h"

// ===============================================
// Extern "C" linkage items
// ===============================================
extern "C"
{
extern volatile UINT64			cycles;
extern volatile DWORD			rst7cycles;

#ifdef WIN32
int strcasecmp(const char* s1, const char* s2);
#endif
}

extern int MultFact;
static VTTDockVid*      gpDisp = NULL;
static Fl_Window*       gpExtWin = NULL;
static Fl_Menu_Bar*     gpExtMenu = NULL;

static Fl_Menu_Item menuitems[] = {
  { "&File", 0, 0, 0, FL_SUBMENU },
     { "&Open", 0, 0, 0 },
     { 0 },
  { "&Emulation",         0, 0,  0, FL_SUBMENU },
     { "&Some Option",    0, 0,  0 },
     { 0 },
  { "&Tools", 0, 0, 0, FL_SUBMENU },
     { "&Assembler", 0, 0, 0, 0 },
     { 0 },
  { "&Help", 0, 0, 0, FL_SUBMENU}, // | FL_MENU_DIVIDER},
     { "&About", 0, 0, 0, 0 },
     { 0 },
  { 0 }
};


/*
============================================================================
TDock C init routine
============================================================================
*/
void tdock_init (void)
{
	gpExtWin = new Fl_Window((480)*MultFact+10, 200*MultFact+MENU_HEIGHT+10, "VirtualT's TDock VGA Emulation");

    /* Create a Menu bar in the window */
	gpExtMenu = new Fl_Menu_Bar(0, 0, gpExtWin->w(), MENU_HEIGHT-2);
	gpExtMenu->menu(menuitems);

    /* Crate a TDock Video window */
    gpDisp = new VTTDockVid(0, MENU_HEIGHT, gpExtWin->w(), gpExtWin->h() - MENU_HEIGHT);

    gpExtWin->end();
    gpExtWin->show();
}

/*
============================================================================
TDock C write hook
============================================================================
*/
void tdock_write(unsigned char device, unsigned char data)
{
    if (gpDisp == NULL)
        tdock_init();

    if (device == 1 && gpDisp)
        gpDisp->WriteData(data);
    else
        printf("TDock write: Device=%d, val=%02X\n", device, data);
}

/*
============================================================================
TDock C read hook
============================================================================
*/
unsigned char tdock_read(void)
{
    return 0xFF;
}

/*
===========================================================================
VTTpddServer class function implementaion follows.  This is the constructor
===========================================================================
*/
VTTDock::VTTDock(void)
{
}

/*
===========================================================================
Class destructor
===========================================================================
*/
VTTDock::~VTTDock(void)
{
}

/*
===========================================================================
Writes data to the TDock
===========================================================================
*/
void VTTDock::Write(unsigned char data)
{
}

/*
===========================================================================
Reads data from the TDock
===========================================================================
*/
unsigned char VTTDock::Read(void)
{
    return 0xFF;
}

