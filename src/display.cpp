/* display.cpp */

/* $Id: display.cpp,v 1.2 2004/08/31 15:08:56 kpettit1 Exp $ */

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
#include <FL/Fl_Help_Dialog.h>
#include <FL/fl_ask.H> 
#include <FL/Fl_File_Chooser.H>
//Added by J. VERNET for pref files
// see also cb_xxxxxx
#include <FL/Fl_Preferences.h>



#include <string.h>
#include <stdio.h>

#include "VirtualT.h"
#include "display.h"
#include "m100emu.h"
#include "io.h"
#include "file.h"
#include "setup.h"
#include "periph.h"

Fl_Window *MainWin = NULL;
T100_Disp *gpDisp;
Fl_Box    *gpCode, *gpGraph, *gpKey, *gpSpeed, *gpCaps, *gpKeyInfo;
Fl_Menu_Bar	*Menu;
Fl_Preferences virtualt_prefs(Fl_Preferences::USER, "virtualt.", "virtualt" ); 
char	gsMenuROM[40];
char	gDelayedError[128] = {0};

int		MultFact = 3;
int		DisplayMode = 1;
int		SolidChars = 0;

int		gRectsize = 2;
int		gXoffset;
int		gYoffset;

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
        virtualt_prefs.set("fullspeed",0);

}
void fspeed(Fl_Widget* w, void*) 
{
	fullspeed=1;
        virtualt_prefs.set("fullspeed",1);

}

void close_disp_cb(Fl_Widget* w, void*)
{
	if (gpDisp != NULL)
	{
		gExitApp = 1;
	}
}

void resize_window()
{
	MainWin->resize(MainWin->x(), MainWin->y(), 240*MultFact +
		90*DisplayMode+2,64*MultFact + 50*DisplayMode + MENU_HEIGHT + 22);
	Menu->resize(0, 0, 240*MultFact + 90*DisplayMode+2, MENU_HEIGHT-2);
	gpDisp->resize(0, MENU_HEIGHT, 240*MultFact + 90*DisplayMode+2, 64*MultFact
		+ 50*DisplayMode+2);
	gpGraph->resize(0, MENU_HEIGHT+64*MultFact + 50*DisplayMode+2, 60, 20);
	gpCode->resize(60, MENU_HEIGHT+64*MultFact + 50*DisplayMode+2, 60, 20);
	gpCaps->resize(120, MENU_HEIGHT+64*MultFact + 50*DisplayMode+2, 60, 20);
	gpKey->resize(180, MENU_HEIGHT+64*MultFact + 50*DisplayMode+2, 120, 20);
	gpSpeed->resize(300, MENU_HEIGHT+64*MultFact + 50*DisplayMode+2, 60, 20);
	gpKeyInfo->resize(360, MENU_HEIGHT+64*MultFact + 50*DisplayMode+2,
		MainWin->w()-360, 20);

	gRectsize = MultFact - (1 - SolidChars);
	if (gRectsize == 0)
		gRectsize = 1;
	gXoffset = 45*DisplayMode+1;
	gYoffset = 25*DisplayMode + MENU_HEIGHT+1;
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
	MultFact = 1;
        virtualt_prefs.set("MultFact",1);
	resize_window();
}
void cb_2x(Fl_Widget* w, void*)
{
	MultFact = 2;
        virtualt_prefs.set("MultFact",2);
	resize_window();
}
void cb_3x(Fl_Widget* w, void*)
{
	MultFact = 3;
        virtualt_prefs.set("MultFact",3);
	resize_window();
}
void cb_4x(Fl_Widget* w, void*)
{
	MultFact = 4;
        virtualt_prefs.set("MultFact",4);
	resize_window();
}
void cb_framed(Fl_Widget* w, void*)
{
	DisplayMode  ^= 1;
        virtualt_prefs.set("DisplayMode",DisplayMode);
	resize_window();
}
void cb_solidchars (Fl_Widget* w, void*)
{
	SolidChars  ^= 1;
        virtualt_prefs.set("SolidChars",SolidChars);
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
	int a=fl_ask("Really Reset ?");
	
	if(a==1) 
		resetcpu();
}

void cb_UnloadOptRom (Fl_Widget* w, void*)
{
	gsOptRomFile[0] = 0;

	// Update user preferences
    virtualt_prefs.set("OptRomFile",gsOptRomFile);

	// Clear menu
	strcpy(gsMenuROM, "No ROM loaded");

	// Clear optROM memory
	load_opt_rom();

	// Reset the CPU
	resetcpu();

}


void cb_help (Fl_Widget* w, void*)
{
/*
#ifndef WIN32
    Fl_Help_Dialog *HelpWin;
	
	HelpWin = new Fl_Help_Dialog();
	HelpWin->load("./help.htm");
	HelpWin->show();
#endif
*/

}

void cb_about (Fl_Widget* w, void*)
{
  
   Fl_Window* o = new Fl_Window(420, 340);
  
    { Fl_Box* o = new Fl_Box(20, 0, 325, 95, "Virtual T");
      o->labelfont(11);
      o->labelsize(80);
    }
    { Fl_Box* o = new Fl_Box(40, 105, 305, 40, "A Tandy Model 100/102/200 Emulator");
      o->labelfont(8);
      o->labelsize(24);
    }
    { Fl_Box* o = new Fl_Box(20, 150, 335, 35, "written by: "); 
      o->labelfont(8);
      o->labelsize(18);
    }
    { Fl_Box* o = new Fl_Box(20, 170, 335, 35, "Stephen Hurd");
      o->labelfont(8);
      o->labelsize(18);
    }
    { Fl_Box* o = new Fl_Box(15, 190, 340, 35, "Ken Pettit");
      o->labelfont(8);
      o->labelsize(18);
    }
    { Fl_Box* o = new Fl_Box(15, 210, 340, 35, "Jerome Vernet (Mac OsX Port)");
      o->labelfont(8);
      o->labelsize(18);
    }
    { Fl_Box* o = new Fl_Box(85, 255, 195, 25, "V "VERSION);
      o->labelfont(8);
      o->labelsize(18);
    }
    { Fl_Box* o = new Fl_Box(25, 295, 320, 25, "This program may be distributed freely ");
      o->labelfont(8);
      o->labelsize(16);
    }
    { Fl_Box* o = new Fl_Box(25, 315, 320, 25, "under the terms of the BSD license");
      o->labelfont(8);
      o->labelsize(16);
    }
    { Fl_Box* o = new Fl_Box(10, 75, 320, 20, "in FLTK");
      o->labelfont(8);
      o->labelsize(18);
    }
    o->end();
    o->show();   
}

T100_Disp::T100_Disp(int x, int y, int w, int h) :
  Fl_Widget(x, y, w, h)
{
	  int driver;

	  for (driver = 0; driver < 10; driver++)
	  {
		  for (int c = 0; c < 256; c++)
			  lcd[driver][c] = 0;
	  }

	  m_MyFocus = 0;
}

// Draw the black pixels on the LCD
__inline void drawpixel(int x, int y, int color)
{
	if (color)
		fl_rectf(x*MultFact + gXoffset,y*MultFact + gYoffset,gRectsize,gRectsize);
}


// Redraw the whole LCD
void T100_Disp::draw()
{
	int c;
	int width;
	int x_pos, inc, start, y_pos;
	int xl_start, xl_end, xr_start, xr_end;

	// Draw gray "screen"
    fl_color(FL_GRAY);
    fl_rectf(x(),y(),w(),h());

	if (DisplayMode == 1)
	{
		// Color for outer border
		fl_color(FL_WHITE);

		// Draw border along the top
		fl_rectf(x(),y(),240*MultFact+92,5);

		// Draw border along the bottom
		fl_rectf(x(),y()+64*MultFact+45,240*MultFact+92,5);

		// Draw border along the left
		fl_rectf(x(),y()+5,5,64*MultFact+42);

		// Draw border along the right
		fl_rectf(x()+240*MultFact+87,y()+5,5,64*MultFact+42);


		// Color for inner border
		fl_color(FL_BLACK);

		// Draw border along the top
		fl_rectf(x()+5,y()+5,240*MultFact+42,20);

		// Draw border along the bottom
		fl_rectf(x()+5,y()+64*MultFact+27,240*MultFact+82,20);

		// Draw border along the left
		fl_rectf(x()+5,y()+5,40,64*MultFact+32);

		// Draw border along the right
		fl_rectf(x()+240*MultFact+47,y()+5,40,64*MultFact+32);


		width = w() - 90;
		inc = width/8;
		start = 28 + width/16 + (4-MultFact)*2;
		fl_color(FL_WHITE);
		fl_font(FL_COURIER,12);
		char  text[3] = "F1";
		y_pos = h()+20;
		xl_start = 2*MultFact;
		xl_end = 7*MultFact;

		xr_start = 12 + 2*MultFact;
		xr_end = 12 + 7*MultFact;

		// Draw function key labels
		for (c = 0; c < 8; c++)
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

	int x=0;
	int y=0;
	int driver, col, row;
	uchar value;

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

				y = row * 8;
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

		// Load new value into lcd "RAM"
		lcd[driver][col] = value;

		// Check if value is an LCD command
		if ((col&0x3f) > 49)
			return;

		// Calculate X position of byte
		x=(driver % 5) * 50 + (col&0x3F);

		// Calcluate y position of byte
		y = ((col & 0xC0) >> 6) * 8;
		if (driver > 4)
			y += 32;

		// Set the display
		gpDisp->window()->make_current();

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

		// Update display
//		Fl::wait(0);
//	}
}


Fl_Menu_Item menuitems[] = {
  { "&File", 0, 0, 0, FL_SUBMENU },
	{ "Load file from HD",     0, cb_LoadFromHost, 0 },
	{ "Save file to HD",       0, cb_SaveToHost, 0, 0 },
	{ "Save Options",	       0, 0,  0, FL_SUBMENU | FL_MENU_DIVIDER },
		{ "Save Basic as ASCII",  0, cb_save_basic, (void *) 1, FL_MENU_TOGGLE },
		{ "Save CO as HEX",       0, cb_save_co,    (void *) 2, FL_MENU_TOGGLE },
		{ 0 },
	{ "Load RAM...",      0, cb_LoadRam, 0 },
	{ "Save RAM...",      0, cb_SaveRam, 0, FL_MENU_DIVIDER },
	{ "E&xit",            FL_CTRL + 'q', close_disp_cb, 0 },
	{ 0 },

  { "&Emulation",         0, 0,        0, FL_SUBMENU },
	{ "Reset",            0, cb_reset, 0, FL_MENU_DIVIDER },
	{ "Model",            0, 0,        0, FL_SUBMENU 	 } ,
		{ "M100",   0, 0, (void *) 1, FL_MENU_RADIO | FL_MENU_VALUE },
		{ "M102",   0, 0, (void *) 2, FL_MENU_RADIO },
		{ "T200",   0, 0, (void *) 3, FL_MENU_RADIO },
		{ 0 },
	{ "Speed", 0, 0, 0, FL_SUBMENU },
		{ "2.4 MHz",     0, rspeed, (void *) 1, FL_MENU_RADIO | FL_MENU_VALUE },
		{ "Max Speed",   0, fspeed, (void *) 1, FL_MENU_RADIO },
		{ 0 },
	{ "Display", 0, 0, 0, FL_SUBMENU | FL_MENU_DIVIDER},
		{ "1x",  0, cb_1x, (void *) 1, FL_MENU_RADIO },
		{ "2x",  0, cb_2x, (void *) 2, FL_MENU_RADIO },
		{ "3x",  0, cb_3x, (void *) 3, FL_MENU_RADIO | FL_MENU_VALUE},
		{ "4x",  0, cb_4x, (void *) 4, FL_MENU_RADIO | FL_MENU_DIVIDER},
		{ "Framed",  0, cb_framed, (void *) 1, FL_MENU_TOGGLE|FL_MENU_VALUE },
		{ "Solid Chars",  0, cb_solidchars, (void *) 1, FL_MENU_TOGGLE},
		{ 0 },
	{ "Peripheral Setup...",     0, cb_PeripheralSetup, 0, FL_MENU_DIVIDER },
	{ "Option ROM",              0, 0, 0, FL_SUBMENU },
		{ gsMenuROM,             0, 0, 0, FL_MENU_DIVIDER },
		{ "Load ROM...",         0, cb_LoadOptRom, 0, 0 },
		{ "Unload ROM",          0, cb_UnloadOptRom, 0, 0 },
		{ 0 },
	{ 0 },

  { "&Tools", 0, 0, 0, FL_SUBMENU },
	{ "CPU Registers",         0, 0 },
	{ "Assembler",             0, 0 },
	{ "Disassembler",          0, disassembler_cb },
	{ "Debugger",              0, 0 },
	{ "Memory Editor",         0, 0 },
	{ "Peripheral Devices",    0, cb_PeripheralDevices },
	{ "Simulation Log Viewer", 0, 0 },
	{ "Model T File Viewer",   0, 0 },
	{ "BASIC Debugger",        0, 0 },
	{ 0 },
  { "&Help", 0, 0, 0, FL_SUBMENU },
	{ "Help", 0, cb_help },
	{ "About VirtualT", 0, cb_about },
	{ 0 },

  { 0 }
};

// read Pref File
// J. VERNET

void initpref(void)
{
	virtualt_prefs.get("fullspeed",fullspeed,0);
	virtualt_prefs.get("MultFact",MultFact,3);
	virtualt_prefs.get("DisplayMode",DisplayMode,1);
	virtualt_prefs.get("SolidChars",SolidChars,0);
	virtualt_prefs.get("BasicSaveMode",BasicSaveMode,0);
	virtualt_prefs.get("COSaveMode",COSaveMode,0);
	virtualt_prefs.get("OptRomFile",gsOptRomFile,"", 256);

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
void initdisplay(void)
{
	int	mIndex;

	MainWin = new Fl_Window(240*MultFact + 90*DisplayMode+2,64*MultFact +
		50*DisplayMode + MENU_HEIGHT + 22, "Virtual T");
	Menu = new Fl_Menu_Bar(0, 0, 240*MultFact + 90*DisplayMode+2,
		MENU_HEIGHT-2);
	gpDisp = new T100_Disp(0, MENU_HEIGHT, 240*MultFact + 90*DisplayMode+2,
		64*MultFact + 50*DisplayMode+2);
	MainWin->callback(close_disp_cb);
	Menu->menu(menuitems);
        
        
// Treat Values read in pref files
// J. VERNET 
// Be careful, if order or add to Emulation Menu, this code need to be changed.
	// Update Speed menu item if not default value
	if(fullspeed==1) 
	{
		mIndex = 0;
		while (menuitems[mIndex].callback_ != rspeed)
			mIndex++;
		menuitems[mIndex+1].flags= FL_MENU_RADIO |FL_MENU_VALUE;
		menuitems[mIndex].flags= FL_MENU_RADIO;
	}

	// Update Display Size from preference
	mIndex = 0;
	// Find first display size menu item
	while (menuitems[mIndex].callback_ != cb_1x)
		mIndex++;
	mIndex--;

    for(int i=1;i<5;i++)
    {
        if(i==MultFact) 
            if(i==4) 
                menuitems[i+mIndex].flags=FL_MENU_RADIO | FL_MENU_VALUE | FL_MENU_DIVIDER;
            else menuitems[i+mIndex].flags=FL_MENU_RADIO | FL_MENU_VALUE;  
        else
            if(i==4) 
                menuitems[i+mIndex].flags=FL_MENU_RADIO | FL_MENU_DIVIDER;
            else menuitems[i+mIndex].flags=FL_MENU_RADIO;  
    }

	// Update DisplayMode parameter 
    if(DisplayMode==0)
	{
		mIndex = 0;
		while (menuitems[mIndex].callback_ != cb_framed)
			mIndex++;
        menuitems[mIndex].flags=FL_MENU_TOGGLE;
	}

	// Update SolidChars menu item
    if(SolidChars==1)
	{
		mIndex = 0;
		while (menuitems[mIndex].callback_ != cb_solidchars)
			mIndex++;
        menuitems[mIndex].flags=FL_MENU_TOGGLE|FL_MENU_VALUE;
	}
        
	// Update BasicSaveMode parameter 
    if(BasicSaveMode==1)
	{
		mIndex = 0;
		while (menuitems[mIndex].callback_ != cb_save_basic)
			mIndex++;
        menuitems[mIndex].flags=FL_MENU_TOGGLE|FL_MENU_VALUE;
	}
    
	// Update COSaveMode parameter 
    if(COSaveMode==1)
	{
		mIndex = 0;
		while (menuitems[mIndex].callback_ != cb_save_co)
			mIndex++;
        menuitems[mIndex].flags=FL_MENU_TOGGLE|FL_MENU_VALUE;
	}

	gpGraph = new Fl_Box(FL_DOWN_BOX,0, MENU_HEIGHT+64*MultFact +
		50*DisplayMode+2, 60, 20,"GRAPH");
	gpGraph->labelsize(10);
	gpGraph->deactivate();
	gpCode = new Fl_Box(FL_DOWN_BOX,60, MENU_HEIGHT+64*MultFact +
		50*DisplayMode+2, 60, 20,"CODE");
	gpCode->labelsize(10);
	gpCode->deactivate();
	gpCaps = new Fl_Box(FL_DOWN_BOX,120, MENU_HEIGHT+64*MultFact +
		50*DisplayMode+2, 60, 20,"CAPS");
	gpCaps->labelsize(10);
	gpCaps->deactivate();
	gpKey = new Fl_Box(FL_DOWN_BOX,180, MENU_HEIGHT+64*MultFact +
		50*DisplayMode+2, 120, 20,"");
	gpKey->labelsize(10);
	gpSpeed = new Fl_Box(FL_DOWN_BOX,300, MENU_HEIGHT+64*MultFact +
		50*DisplayMode+2, 60, 20,"");
	gpSpeed->labelsize(10);
	gpKeyInfo = new Fl_Box(FL_DOWN_BOX,360, MENU_HEIGHT+64*MultFact +
		50*DisplayMode+2, MainWin->w()-360, 20,
		"F9:Label  F10:Print  F11:Paste  F12:Pause");
	gpKeyInfo->labelsize(10);

	gXoffset = 45*DisplayMode+1;
	gYoffset = 25*DisplayMode + MENU_HEIGHT+1;
	gRectsize = MultFact - (1 - SolidChars);
	if (gRectsize == 0)
		gRectsize = 1;

	MainWin->end();
	MainWin->show();


	//Fl::wait();
        Fl::check();

	if (strlen(gDelayedError) != 0)
		fl_alert(gDelayedError);

}

char	label[40];

void display_cpu_speed(void)
{

	sprintf(label, "%4.1f Mhz", cpu_speed + .05);
	Fl::check();
	gpSpeed->label(label);
}

void drawbyte(int driver, int column, int value)
{
	gpDisp->SetByte(driver, column, value);

	return;
}

void power_down()
{
	gpDisp->PowerDown();

	return;
}

void process_windows_event()
{
	//Fl::wait(0);
        Fl::check();
	return;
}

// Called when the Model T shuts its power off
void T100_Disp::PowerDown()
{
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
	for (driver = 0; driver < 10; driver++)
		for (col = 0; col < 256; col++)
			lcd[driver][col] = 0;

	// Display first line of powerdown message
    fl_color(FL_BLACK);
	col = 20 - strlen(msg) / 2;
	for (x = 0; x < (int) strlen(msg); x++)
	{
		int mem_index = 0x7711 + (msg[x] - ' ')*5;
		column = col++ * 6;
		for (int c = 0; c < 6; c++)
		{
			driver = column / 50;		// 50 pixels per driver
			if (c == 5)
				SetByte(driver, (column % 50) | 0xC0, 0);
			else
				SetByte(driver, (column % 50) | 0xC0, memory[mem_index++]);
			column++;
		}
	}

	// Display 2nd line of powerdown message
	col = 20 - strlen(msg2) / 2;
	for (x = 0; x < (int) strlen(msg2); x++)
	{
		int mem_index = 0x7711 + (msg2[x] - ' ')*5;
		column = col++ * 6;
		for (int c = 0; c < 6; c++)
		{
			driver = column / 50 + 5;		// 50 pixels per driver
			if (c == 5)
				SetByte(driver, (column % 50), 0);
			else
				SetByte(driver, (column % 50), memory[mem_index++]);
			column++;
		}
	}
}

// Handle mouse events, key events, focus events, etc.
int T100_Disp::handle(int event)
{
	char	label[128];
	char	keystr[10];
	char	isSpecialKey = 1;
	int		c;

	unsigned int key;

	switch (event)
	{
	case FL_FOCUS:
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
		if (!m_MyFocus)
			return 1;

		// Get the Key that was pressed
		key = Fl::event_key();
		//fprintf(stderr,"Touche: %d\n",key);
                //fprintf(stderr,"Texte %s\n",Fl::event_text());
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
			isSpecialKey = 0;
			break;
		}
		update_keys();
		break;

	case FL_KEYDOWN:
		if (!m_MyFocus)
			return 1;

		if (ioBA & 0x10)
		{
			resetcpu();
			break;
		}

		// Get the Key that was pressed
		key = Fl::event_key();
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
		label[0] = 0;

		// Add text for special keys
		for (c = 0; c < 32; c++)
		{
			if ((gSpecialKeys & (0x01 << c)) == 0)
			{
				if ((c == 2) || (c == 3) || (c==5))
					continue;
				if (label[0] != 0)
					strcat(label, "+");
				strcat(label, gSpKeyText[c]);
			}
		}

		// Add text for regular keys
		for (c = ' '+1; c < '~'; c++)
		{
			if (gKeyStates[c])
			{
				if (label[0] != 0)
					strcat(label, "+");
				sprintf(keystr, "'%c'", c);

				// Space shows up better written instead of ' '
				if (c == 0x20)
					strcat(label, "SPACE");
				else
					strcat(label, keystr);
			}
		}

		// For some reason the F1 key causes garbage to print in
		// the status box.  This check fixed it for some reason -- ????
		Fl::check();

		// Update the text
		gpKey->label(label);
	}

	return 1;
}





