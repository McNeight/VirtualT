/* periph.cpp */

/* $Id: periph.cpp,v 1.0 2004/08/05 06:46:12 kpettit1 Exp $ */

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
#include <FL/Fl_Tabs.H>
#include <FL/Fl_Scrollbar.H>
#include <FL/Fl_Check_Button.H>
#include <FL/Fl_Choice.H>


#if defined(WIN32)
#include <windows.h>
#endif
#include <stdio.h>
#include <string.h>
#include <time.h>

#include "m100emu.h"
#include "serial.h"
#include "display.h"
#include "disassemble.h"
#include "periph.h"
#include "memedit.h"
#include "cpuregs.h"

void cb_Ide(Fl_Widget* w, void*) ;

typedef struct periph_ctrl_struct	
{
	Fl_Menu_Bar*			pMenu;
	Fl_Tabs*				pTabs;

	struct
	{
		Fl_Group*			g;
		Fl_Check_Button*	pEnable;
		Fl_Check_Button*	pHex;
		T100_ComMon*		pLog;
		Fl_Scrollbar*		pScroll;
		Fl_Button*			pClear;
		Fl_Choice*			pFont;
		Fl_Box*				pPortName;
		Fl_Box*				pPortStatus;
		Fl_Box*				pBaud;
		Fl_Box*				pWordSize;
		Fl_Box*				pStopBits;
		Fl_Box*				pParity;
		Fl_Box*				pStartChar;
		Fl_Box*				pStopChar;
		Fl_Box*				pDeltaTime;
		Fl_Box*				pDTR;
		Fl_Box*				pRTS;
		Fl_Box*				pDSR;
		Fl_Box*				pCTS;
		Fl_Box*				pTXD;
		Fl_Box*				pRXD;

		char				sPortName[128];
		char				sBaud[10];
		char				sPortStatus[40];
		char				sWordSize[2];
		char				sStopBits[2];
		char				sParity[2];
		char				sComMdm[10];
		char				sStartChar[40];
		char				sStopChar[40];
		char				sDeltaTime[40];
		unsigned char		cSignal;
	} com;
	struct 
	{
		Fl_Group*			g;
		Fl_Box*				pText;
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
} periph_ctrl_t;

// Menu items for the disassembler
Fl_Menu_Item gPeriph_menuitems[] = {
  { "&Tools", 0, 0, 0, FL_SUBMENU },
	{ "CPU Registers",         0, cb_CpuRegs },
	{ "Assembler / IDE",       0, cb_Ide },
	{ "Disassembler",          0, disassembler_cb },
	{ "Memory Editor",         0, cb_MemoryEditor },
//	{ "Simulation Log Viewer", 0, 0 },
	{ "Model T File Viewer",   0, 0 },
	{ 0 },

  { 0 }
};


periph_ctrl_t	periph_ctrl;
int				gComEnableOn = 0;
int				gHexOn = 0;
int				gFontSize = 12;
Fl_Window		*gpdw;

/*
============================================================================
Callback routine for the Peripherial Devices window
============================================================================
*/
void cb_peripheralwin (Fl_Widget* w, void*)
{
	gpdw->hide();
	ser_set_monitor_callback(NULL);
	gComEnableOn = 0;
	gHexOn = 0;
	delete gpdw;
	gpdw = NULL;
}

/*
============================================================================
Callback routine for the Com Clear Log button
============================================================================
*/
void cb_com_clear (Fl_Widget* w, void*)
{
	periph_ctrl.com.pScroll->maximum(2);
	periph_ctrl.com.pScroll->slider_size(1);
	periph_ctrl.com.pScroll->value(0, 2, 0, 2);
	periph_ctrl.com.pLog->Clear();
}

/*
============================================================================
Callback routine for the Com Font Size choice
============================================================================
*/
void cb_com_font_size (Fl_Widget* w, void*)
{
	int size = periph_ctrl.com.pFont->value();
	if (size == 0)
		gFontSize = 12;
	else if (size == 1)
		gFontSize = 14;
	else if (size == 2)
		gFontSize = 16;
	periph_ctrl.com.pLog->CalcLineStarts();
	periph_ctrl.com.pLog->redraw();
}

/*
============================================================================
Callback routine for the Com Enable checkbox
============================================================================
*/
void cb_com_enable_box (Fl_Widget* w, void*)
{
	gComEnableOn = periph_ctrl.com.pEnable->value();
	periph_ctrl.com.pLog->redraw();
}


/*
============================================================================
Callback routine for the Com Enable checkbox
============================================================================
*/
void cb_com_hex_box (Fl_Widget* w, void*)
{
	gHexOn = periph_ctrl.com.pHex->value();
	periph_ctrl.com.pLog->CalcLineStarts();
	periph_ctrl.com.pLog->redraw();
}

/*
============================================================================
Callback routine for the Com Scrollbar
============================================================================
*/
void cb_ComMonScroll (Fl_Widget* w, void*)
{
	periph_ctrl.com.pLog->m_FirstLine = ((Fl_Scrollbar*) w)->value();
	periph_ctrl.com.pLog->redraw();
}

/*
============================================================================
Callback routine for receiving serial port status updates
============================================================================
*/
extern "C"
{
void ser_com_monitor_cb(int fMonType, unsigned char data)
{
	int		baud, size, stop, openState;
	char	parity;

	switch (fMonType)
	{
	case SER_MON_COM_PORT_CHANGE:
		// Get new settings
		ser_get_port_settings(periph_ctrl.com.sPortName, &openState, &baud, &size, 
			&stop, &parity);

		strcpy(periph_ctrl.com.sPortStatus, openState ? "Open" : "Closed");
		sprintf(periph_ctrl.com.sBaud, "%d", baud);
		sprintf(periph_ctrl.com.sWordSize, "%d", size);
		sprintf(periph_ctrl.com.sStopBits, "%d", stop);
		periph_ctrl.com.sParity[0] = parity;
		periph_ctrl.com.sParity[1] = 0;
		periph_ctrl.com.sComMdm[0] = 0;
		periph_ctrl.com.pBaud->hide();
		periph_ctrl.com.pBaud->show();
		break;

	case SER_MON_COM_SIGNAL:
		ser_get_signals(&periph_ctrl.com.cSignal);

		// Update "LEDs"
		if (periph_ctrl.com.cSignal & SER_SIGNAL_RTS)
			periph_ctrl.com.pRTS->color(FL_YELLOW);
		else
			periph_ctrl.com.pRTS->color(FL_BLACK);
		
		if (periph_ctrl.com.cSignal & SER_SIGNAL_DTR)
			periph_ctrl.com.pDTR->color(FL_YELLOW);
		else
			periph_ctrl.com.pDTR->color(FL_BLACK);
		
		if (periph_ctrl.com.cSignal & SER_SIGNAL_CTS)
			periph_ctrl.com.pCTS->color(FL_YELLOW);
		else
			periph_ctrl.com.pCTS->color(FL_BLACK);

		if (periph_ctrl.com.cSignal & SER_SIGNAL_DSR)
			periph_ctrl.com.pDSR->color(FL_YELLOW);
		else
			periph_ctrl.com.pDSR->color(FL_BLACK);

		periph_ctrl.com.pRTS->redraw();
		periph_ctrl.com.pDTR->redraw();
		periph_ctrl.com.pCTS->redraw();
		periph_ctrl.com.pDSR->redraw();
		break;

	case SER_MON_COM_READ:
		if (gComEnableOn)
			periph_ctrl.com.pLog->AddByte(0, data, periph_ctrl.com.cSignal);
		break;

	case SER_MON_COM_WRITE:
		if (gComEnableOn)
			periph_ctrl.com.pLog->AddByte(1, data, periph_ctrl.com.cSignal);
		break;

	}
}
}

/*
============================================================================
Routine to create the PeripheralSetup Window and tabs
============================================================================
*/
void update_com_port_settings()
{
	int		baud, size, stop, openState;
	char	parity;

	ser_get_port_settings(periph_ctrl.com.sPortName, &openState, &baud, &size, 
		&stop, &parity);

	strcpy(periph_ctrl.com.sPortStatus, openState ? "Open" : "Closed");
	sprintf(periph_ctrl.com.sBaud, "%d", baud);
	sprintf(periph_ctrl.com.sWordSize, "%d", size);
	sprintf(periph_ctrl.com.sStopBits, "%d", stop);
	periph_ctrl.com.sParity[0] = parity;
	periph_ctrl.com.sParity[1] = 0;
	periph_ctrl.com.sComMdm[0] = 0;
}

/*
============================================================================
Routine to create the PeripheralSetup Window and tabs
============================================================================
*/
void cb_PeripheralDevices (Fl_Widget* w, void*)
{
	Fl_Box*		o;

	if (gpdw != NULL)
		return;

	// Create Peripheral Setup window
	gpdw = new Fl_Window(550, 400, "Peripheral Devices");
	gpdw->callback(cb_peripheralwin);

	// Create Peripheral Tabs
    {  
		// Create a menu for the new window.
		periph_ctrl.pMenu = new Fl_Menu_Bar(0, 0, 550, MENU_HEIGHT-2);
		periph_ctrl.pMenu->menu(gPeriph_menuitems);

		periph_ctrl.pTabs = new Fl_Tabs(10, MENU_HEIGHT+10, 530, 380-MENU_HEIGHT);

		// COM port Tab
		{ 
			periph_ctrl.com.g = new Fl_Group(10, 30+MENU_HEIGHT, 540, 380-MENU_HEIGHT, " COM ");

			// Create static text boxes
			o = new Fl_Box(FL_NO_BOX, 20, 45+MENU_HEIGHT, 50, 15, "Port Name:");
			o->align(FL_ALIGN_LEFT|FL_ALIGN_INSIDE);
			o = new Fl_Box(FL_NO_BOX, 20, 70+MENU_HEIGHT, 50, 15, "Port Status:");
			o->align(FL_ALIGN_LEFT|FL_ALIGN_INSIDE);
			o = new Fl_Box(FL_NO_BOX, 20, 95+MENU_HEIGHT, 50, 15, "Baud Rate:");
			o->align(FL_ALIGN_LEFT|FL_ALIGN_INSIDE);
			o = new Fl_Box(FL_NO_BOX, 20, 120+MENU_HEIGHT, 50, 15, "Start Char:");
			o->align(FL_ALIGN_LEFT|FL_ALIGN_INSIDE);
	
			o = new Fl_Box(FL_NO_BOX, 180, 70+MENU_HEIGHT, 50, 15, "Word Size:");
			o->align(FL_ALIGN_LEFT|FL_ALIGN_INSIDE);
			o = new Fl_Box(FL_NO_BOX, 180, 95+MENU_HEIGHT, 50, 15, "Parity:");
			o->align(FL_ALIGN_LEFT|FL_ALIGN_INSIDE);
			o = new Fl_Box(FL_NO_BOX, 180, 120+MENU_HEIGHT, 50, 15, "End Char:");
			o->align(FL_ALIGN_LEFT|FL_ALIGN_INSIDE);

			o = new Fl_Box(FL_NO_BOX, 320, 70+MENU_HEIGHT, 50, 15, "Stop Bits:");
			o->align(FL_ALIGN_LEFT|FL_ALIGN_INSIDE);

			o = new Fl_Box(FL_NO_BOX, 450, 45+MENU_HEIGHT, 50, 15, "RTS:");
			o->align(FL_ALIGN_LEFT|FL_ALIGN_INSIDE);
			o = new Fl_Box(FL_NO_BOX, 450, 70+MENU_HEIGHT, 50, 15, "DTR:");
			o->align(FL_ALIGN_LEFT|FL_ALIGN_INSIDE);
			o = new Fl_Box(FL_NO_BOX, 450, 95+MENU_HEIGHT, 50, 15, "CTS:");
			o->align(FL_ALIGN_LEFT|FL_ALIGN_INSIDE);
			o = new Fl_Box(FL_NO_BOX, 450, 120+MENU_HEIGHT, 50, 15, "DSR:");
			o->align(FL_ALIGN_LEFT|FL_ALIGN_INSIDE);

			periph_ctrl.com.pPortName = new Fl_Box(FL_NO_BOX, 100, 
				45+MENU_HEIGHT, 400, 15, periph_ctrl.com.sPortName);
			periph_ctrl.com.pPortName->align(FL_ALIGN_LEFT|FL_ALIGN_INSIDE);

			periph_ctrl.com.pPortStatus = new Fl_Box(FL_NO_BOX, 100, 
				70+MENU_HEIGHT, 50, 15, periph_ctrl.com.sPortStatus);
			periph_ctrl.com.pPortStatus->align(FL_ALIGN_LEFT|FL_ALIGN_INSIDE);

			periph_ctrl.com.pBaud = new Fl_Box(FL_NO_BOX, 100, 
				95+MENU_HEIGHT, 50, 15, periph_ctrl.com.sBaud);
			periph_ctrl.com.pBaud->align(FL_ALIGN_LEFT|FL_ALIGN_INSIDE);

			periph_ctrl.com.pStartChar = new Fl_Box(FL_NO_BOX, 100, 
				120+MENU_HEIGHT, 60, 15, periph_ctrl.com.sStartChar);
			periph_ctrl.com.pStartChar->align(FL_ALIGN_LEFT|FL_ALIGN_INSIDE);
			strcpy(periph_ctrl.com.sStartChar, "(Click)");

			periph_ctrl.com.pWordSize = new Fl_Box(FL_NO_BOX, 260, 
				70+MENU_HEIGHT, 50, 15, periph_ctrl.com.sWordSize);
			periph_ctrl.com.pWordSize->align(FL_ALIGN_LEFT|FL_ALIGN_INSIDE);

			periph_ctrl.com.pParity = new Fl_Box(FL_NO_BOX, 260, 
				95+MENU_HEIGHT, 50, 15, periph_ctrl.com.sParity);
			periph_ctrl.com.pParity->align(FL_ALIGN_LEFT|FL_ALIGN_INSIDE);

			periph_ctrl.com.pStopChar = new Fl_Box(FL_NO_BOX, 260, 
				120+MENU_HEIGHT, 120, 15, periph_ctrl.com.sStopChar);
			periph_ctrl.com.pStopChar->align(FL_ALIGN_LEFT|FL_ALIGN_INSIDE);
			strcpy(periph_ctrl.com.sStopChar, "(Shift/Right Click)");

			periph_ctrl.com.pStopBits = new Fl_Box(FL_NO_BOX, 380, 
				70+MENU_HEIGHT, 50, 15, periph_ctrl.com.sStopBits);
			periph_ctrl.com.pStopBits->align(FL_ALIGN_LEFT|FL_ALIGN_INSIDE);

			periph_ctrl.com.pDeltaTime = new Fl_Box(FL_NO_BOX, 20, 
				145+MENU_HEIGHT, 190, 15, periph_ctrl.com.sDeltaTime);
			periph_ctrl.com.pDeltaTime->align(FL_ALIGN_LEFT|FL_ALIGN_INSIDE);
			strcpy(periph_ctrl.com.sDeltaTime, "");

			ser_get_signals(&periph_ctrl.com.cSignal);

			periph_ctrl.com.pRTS = new Fl_Box(FL_OVAL_BOX, 490, 47+MENU_HEIGHT, 12, 12, "");
			periph_ctrl.com.pRTS->align(FL_ALIGN_LEFT|FL_ALIGN_INSIDE);
			if (periph_ctrl.com.cSignal & SER_SIGNAL_RTS)
				periph_ctrl.com.pRTS->color(FL_YELLOW);
			else
				periph_ctrl.com.pRTS->color(FL_BLACK);

			periph_ctrl.com.pDTR = new Fl_Box(FL_OVAL_BOX, 490, 72+MENU_HEIGHT, 12, 12, "");
			periph_ctrl.com.pDTR->align(FL_ALIGN_LEFT|FL_ALIGN_INSIDE);
			if (periph_ctrl.com.cSignal & SER_SIGNAL_DTR)
				periph_ctrl.com.pDTR->color(FL_YELLOW);
			else
				periph_ctrl.com.pDTR->color(FL_BLACK);

			periph_ctrl.com.pCTS = new Fl_Box(FL_OVAL_BOX, 490, 97+MENU_HEIGHT, 12, 12, "");
			periph_ctrl.com.pCTS->align(FL_ALIGN_LEFT|FL_ALIGN_INSIDE);
			if (periph_ctrl.com.cSignal & SER_SIGNAL_CTS)
				periph_ctrl.com.pCTS->color(FL_YELLOW);
			else
				periph_ctrl.com.pCTS->color(FL_BLACK);

			periph_ctrl.com.pDSR = new Fl_Box(FL_OVAL_BOX, 490, 122+MENU_HEIGHT, 12, 12, "");
			periph_ctrl.com.pDSR->align(FL_ALIGN_LEFT|FL_ALIGN_INSIDE);
			if (periph_ctrl.com.cSignal & SER_SIGNAL_DSR)
				periph_ctrl.com.pDSR->color(FL_YELLOW);
			else
				periph_ctrl.com.pDSR->color(FL_BLACK);

			periph_ctrl.com.pLog = new T100_ComMon(20, 200, 495, 190-MENU_HEIGHT);
//			periph_ctrl.com.pLog->color(FL_WHITE);

			periph_ctrl.com.pScroll = new Fl_Scrollbar(515, 200, 20, 190-MENU_HEIGHT, "");
			periph_ctrl.com.pScroll->type(FL_VERTICAL);
			periph_ctrl.com.pScroll->linesize(2);
			periph_ctrl.com.pScroll->maximum(2);
			periph_ctrl.com.pScroll->callback(cb_ComMonScroll);
			periph_ctrl.com.pScroll->slider_size(1);

			// Create items on the Tab
			periph_ctrl.com.pEnable = new Fl_Check_Button(20, 362, 130, 20, "Enable Capture");
			periph_ctrl.com.pEnable->callback(cb_com_enable_box);

			// Create items on the Tab
			periph_ctrl.com.pHex = new Fl_Check_Button(160, 362, 50, 20, "HEX");
			periph_ctrl.com.pHex->callback(cb_com_hex_box);

			// Create items on the Tab
			periph_ctrl.com.pClear = new Fl_Button(330, 360, 80, 25, "Clear Log");
			periph_ctrl.com.pClear->callback(cb_com_clear);

			periph_ctrl.com.pFont = new Fl_Choice(250, 362, 60, 20, "Size");
			periph_ctrl.com.pFont->callback(cb_com_font_size);
			periph_ctrl.com.pFont->add("12");
			periph_ctrl.com.pFont->add("14");
			periph_ctrl.com.pFont->add("16");
			periph_ctrl.com.pFont->value(0);

			periph_ctrl.com.g->end();

			update_com_port_settings();

		}

		// LPT Port Tab
		{ 
			// Create the Group item (the "Tab")
			periph_ctrl.lpt.g = new Fl_Group(10, MENU_HEIGHT+30, 500, 380-MENU_HEIGHT, " LPT ");

			// Create controls
			periph_ctrl.lpt.pText = new Fl_Box(120, MENU_HEIGHT+60, 60, 80, "Parallel Port not supported yet");

			// End of control for this tab
			periph_ctrl.lpt.g->end();
		}

		// Modem Port Tab
		{
			// Create the Group item (the "Tab")
			periph_ctrl.mdm.g = new Fl_Group(10, MENU_HEIGHT+30, 500, 380-MENU_HEIGHT, " MDM ");

			// Create controls
			periph_ctrl.mdm.pText = new Fl_Box(120, MENU_HEIGHT+60, 60, 80, "Modem Port not supported yet");

			// End of control for this tab
			periph_ctrl.mdm.g->end();
		}

		// Cassette Port Tab
		{ 
			// Create the Group item (the "Tab")
			periph_ctrl.cas.g = new Fl_Group(10, MENU_HEIGHT+30, 500, 380-MENU_HEIGHT, " CAS ");

			// Create controls
			periph_ctrl.cas.pText = new Fl_Box(120, MENU_HEIGHT+60, 60, 80, "Cassette Port not supported yet");

			// End of control for this tab
			periph_ctrl.cas.g->end();
		}

		// BCR Port Tab
		{ 
			// Create the Group item (the "Tab")
			periph_ctrl.bcr.g = new Fl_Group(10, MENU_HEIGHT+30, 500, 380-MENU_HEIGHT, " BCR ");

			// Create controls
			periph_ctrl.bcr.pText = new Fl_Box(120, MENU_HEIGHT+60, 60, 80, "BCR Port not supported yet");

			// End of control for this tab
			periph_ctrl.bcr.g->end();
		}


		periph_ctrl.pTabs->value(periph_ctrl.com.g);
		periph_ctrl.pTabs->end();
	
	}

	// Make things resizeable
	gpdw->resizable(periph_ctrl.pMenu);
	gpdw->resizable(periph_ctrl.pTabs);

	// Set COM callback
	ser_set_monitor_callback(ser_com_monitor_cb);

	gpdw->show();
}

T100_ComMon::T100_ComMon(int x, int y, int w, int h) :
  Fl_Widget(x, y, w, h)
{
	// Create first TcomLogBlock
	m_log = new TcomLogBlock();
	m_MyFocus = 0;
	m_FirstLine = 0;
	m_LastLine = 0;
	m_LastCol = 0;
	m_LastCnt = 0;
	m_LineStartCount = 1;
	m_LineStarts[0].clb = m_log;
	m_LineStarts[0].index = 0;
	m_Height = 0;
	m_Width = 0;
	m_Lines = 0;
	m_Cols = 0;
	m_pLastEntry = NULL;
	m_pStartTime = NULL;
	m_pStopTime = NULL;
}

T100_ComMon::~T100_ComMon()
{
	delete m_log;
}

void T100_ComMon::Clear(void)
{
	delete m_log;
	m_log = new TcomLogBlock();
	m_FirstLine = 0;
	m_LastLine = 0;
	m_LastCol = 0;
	m_LastCnt = 0;
	m_LineStartCount = 1;
	m_LineStarts[0].clb = m_log;
	m_LineStarts[0].index = 0;
	m_Height = 0;
	m_Width = 0;
	m_Lines = 0;
	m_Cols = 0;
	m_pLastEntry = NULL;
	m_pStartTime = NULL;
	m_pStopTime = NULL;
	periph_ctrl.com.sDeltaTime[0] = 0;
	periph_ctrl.com.pDeltaTime->label(periph_ctrl.com.sDeltaTime);
	strcpy(periph_ctrl.com.sStartChar, "(Click)");
	periph_ctrl.com.pStartChar->label(periph_ctrl.com.sStartChar);
	strcpy(periph_ctrl.com.sStopChar, "(Shift/Right Click)");
	periph_ctrl.com.pStopChar->label(periph_ctrl.com.sStopChar);
	redraw();
}

void T100_ComMon::AddByte(int rx_tx, char byte, char flags)
{
	TcomLogBlock	*b = m_log;
	int				xpos, ypos, adder;
	char			string[4];
	int				line;

	line = m_LastLine;

	// Find last block with room in it
	while (b->used == b->max)
	{
		if (b->next == NULL)
			b->next = new TcomLogBlock();

		b = b->next;
	}

	// Add byte to the log
	b->entries[b->used].byte = byte;
	if (rx_tx)
		flags |= 0x80;
	b->entries[b->used].flags = flags;
	b->entries[b->used].time = hirestimer();

	// Check if last char was save type as this & advance col if it was
	if (m_pLastEntry != NULL)
	{
		if ((m_pLastEntry->flags & 0x80) == (b->entries[b->used].flags & 0x80) ||
			(m_LastCnt == 2))
		{
			adder = gHexOn ? 3 : 1;
			// Add 1 or 3 bytes (2 hex plus space)
			m_LastCol += adder;
			m_LastCnt = 0;
			if (m_LastCol + adder >= m_Cols)
			{
				m_LastCol = 0;
				m_LastLine += 2;
				m_LineStarts[m_LineStartCount].clb = b;
				m_LineStarts[m_LineStartCount++].index = b->used;
			}
		}
	}
	// Increment number of bytes displayed at this col
	m_LastCnt++;

	// Determine if byte is on the window
	if ((m_LastLine >= m_FirstLine) && (m_LastLine <= m_FirstLine + m_Lines-2))
	{
		// Draw byte in window -- first set the display active
		window()->make_current();

		// Select 12 point Courier font
		fl_font(FL_COURIER_BOLD,gFontSize);

		// Determine xoffset of text
		xpos = (int) (x() + m_LastCol * m_Width);

		// Determine color & yoffset of text
		if (b->entries[b->used].flags & 0x80)
		{
			// Byte is a TX byte
			fl_color(FL_RED);
			ypos = (int) (y() + (m_LastLine-m_FirstLine+1) * m_Height);

		}
		else
		{
			// Byte is an RX byte
			fl_color(FL_BLUE);
			ypos = (int) (y() + (m_LastLine-m_FirstLine+2) * m_Height);
		}


		// Draw the text
		if (gHexOn)
			sprintf(string, "%02X", b->entries[b->used].byte);
		else
			sprintf(string, "%c", b->entries[b->used].byte);
		fl_draw(string, xpos, ypos-2);
	}

	if (line != m_LastLine)
	{
		periph_ctrl.com.pScroll->maximum((m_LineStartCount<<1)-m_Lines);
		periph_ctrl.com.pScroll->slider_size((double) m_Lines / (double) (m_LastLine+2));
	}

	// Save pointer to this entry as last byte added
	m_pLastEntry = &b->entries[b->used];

	b->used++;

//	redraw();
}

void T100_ComMon::CalcLineStarts(void)
{
	comLogEntry_t	*cle;
	TcomLogBlock	*clb;
	int				index;
	int				col, line, cnt;
	int				adder;

	// Get pointer/index of first comLogEntry of first line/col
	clb = m_log;
	index = 0;
	adder = gHexOn ? 3 : 1;
	line = 0;
	col = 0;
	cle = NULL;
	cnt = 0;
	m_LineStartCount = 0;
	m_LineStarts[0].clb = clb;
	m_LineStarts[0].index = index;

	while (index < clb->used)
	{
		col = 0;
		cle = NULL;
		cnt = 0;
		m_LineStarts[m_LineStartCount].clb = clb;
		m_LineStarts[m_LineStartCount++].index = index;
		while (col + adder < m_Cols)
		{
			if (index >= clb->used)
				break;

			// Calculate col/line where data should be drawn
			if (cle != NULL)
			{
				// Check if last char was save type as this & advance if it was
				if ((cle->flags & 0x80) == (clb->entries[index].flags & 0x80) ||
					(cnt == 2))
				{
					// Add 1 or 3 bytes (2 hex plus space)
					col += adder;
					cnt = 0;
					if (col + adder >= m_Cols)
					{
						col = 0;
						line += 2;
						break;
					}
				}
			}

			// Save pointer to this cle for next comparison
			cle = &clb->entries[index];

			cnt++;

			// Get location of next byte
			if (++index == clb->max)
			{
				// Get pointer to next clb
				if (clb->next == NULL)
					// No more data -- break loop
					break;

				clb = clb->next;
				index = 0;
			}

		}
	}

	if (m_LineStartCount == 0)
		m_LineStartCount++;
	m_LastLine = line;
	m_LastCol = col;
	m_LastCnt = cnt;
	m_pLastEntry = cle;
	periph_ctrl.com.pScroll->maximum((m_LineStartCount<<1)-m_Lines);
	periph_ctrl.com.pScroll->slider_size((double) m_Lines / (double) (m_LastLine+2));
}

// Redraw the whole LCD
void T100_ComMon::draw()
{
	int				line, col, cnt;
	comLogEntry_t	*cle;
	TcomLogBlock	*clb;
	int				index;
	int				xpos, ypos;
	char			string[4];
	int				adder;
	int				lines, cols;

	// Select 12 point Courier font
	fl_font(FL_COURIER_BOLD,gFontSize);

	// Get character width & height
	m_Width = fl_width("W", 1);
	m_Height = fl_height();
	
	// Calculate max cols and lines
	lines = (int) (h() / m_Height);
	cols = (int) (w() / m_Width);

	if ((lines != m_Lines) || (cols != m_Cols))
	{
		m_Lines = lines;
		m_Cols = cols;
		CalcLineStarts();
	}

	// Draw white background
    fl_color(FL_WHITE);
    fl_rectf(x(),y(),w(),h());

	// Draw GRAY lines between every other line
	fl_color(FL_GRAY);
	for (line = 2; line < m_Lines; line += 2)
		fl_line(x(), y() + (int) (line * m_Height)+2, x()+w(), 
			y()+(int)(line*m_Height)+2);

	// Get pointer/index of first comLogEntry of first line/col
	clb = m_LineStarts[m_FirstLine>>1].clb;
	index = m_LineStarts[m_FirstLine>>1].index;

	adder = gHexOn ? 3 : 1;

	line = 0;
	while (line + 1 < m_Lines)
	{
		col = 0;
		cle = NULL;
		cnt = 0;
		while (col + adder < m_Cols)
		{
			if (index >= clb->used)
				break;

			// Calculate col/line where data should be drawn
			if (cle != NULL)
			{
				// Check if last char was also a TX & advance
				if ((cle->flags & 0x80) == (clb->entries[index].flags & 0x80) ||
					(cnt == 2))
				{
					// Add 3 bytes (2 hex plus space)
					col += adder;
					cnt = 0;
					if (col + adder >= m_Cols)
					{
						col = 0;
						line += 2;
						break;
					}
				}
			}

			// Determine xoffset of text
			xpos = (int) (x() + col * m_Width);

			// Determine color & yoffset of text
			if (clb->entries[index].flags & 0x80)
			{
				// Byte is a TX byte
				fl_color(FL_RED);
				ypos = (int) (y() + (line+1) * m_Height);

			}
			else
			{
				// Byte is an RX byte
				fl_color(FL_BLUE);
				ypos = (int) (y() + (line+2) * m_Height);
			}

			if (&clb->entries[index] == m_pStartTime)
			{
				fl_color(FL_GREEN);
				fl_rectf(xpos, ypos-(int) m_Height+3, (int) (adder * m_Width), (int) m_Height-1);
				fl_color(FL_BLACK);
				m_StartTimeLine = line;
				if ((clb->entries[index].flags & 0x80) == 0)
					m_StartTimeLine++;
				m_StartTimeCol = col;
			}
			else if (&clb->entries[index] == m_pStopTime)
			{
				fl_color(FL_RED);
				fl_rectf(xpos, ypos-(int)m_Height+3, (int) (adder * m_Width), (int) m_Height-1);
				fl_color(FL_WHITE);
				m_StopTimeLine = line;
				if ((clb->entries[index].flags & 0x80) == 0)
					m_StopTimeLine++;
				m_StopTimeCol = col;
			}
			// Draw the text
			if (gHexOn)
				sprintf(string, "%02X", clb->entries[index].byte);
			else
				sprintf(string, "%c", clb->entries[index].byte);
			fl_draw(string, xpos, ypos-2);

			// Save pointer to this cle for next comparison
			cle = &clb->entries[index];

			cnt++;

			// Get location of next byte
			if (++index == clb->max)
			{
				// Get pointer to next clb
				if (clb->next == NULL)
					// No more data -- break loop
					break;

				clb = clb->next;
				index = 0;
			}

		}

		if (index >= clb->used)
			break;
	}
}

// Handle mouse events, key events, focus events, etc.
int T100_ComMon::handle(int event)
{
	int				c, xp, yp, shift;
	int				line_click, col_click, cnt;
	int				col, line, adder;
	int				index, lineIndex;
	int				xpos, ypos;
	char			string[4];
	TcomLogBlock	*clb;
	comLogEntry_t	*cle, *prev_cle;
	comLogEntry_t	*cle_sel;
	double			delta;

	switch (event)
	{
	case FL_FOCUS:
		m_MyFocus = 1;
		break;

	case FL_UNFOCUS:
		m_MyFocus = 0;
		break;

	case FL_PUSH:
		// Get id of mouse button pressed
		c = Fl::event_button();

		// Check if it was the Left Mouse button
		if ((c == FL_LEFT_MOUSE) || (c == FL_RIGHT_MOUSE))
		{
			// Get X,Y position of button press
			xp = Fl::event_x();
			yp = Fl::event_y();

			// Check if Shift was depressed during the Mouse Button event
			if (Fl::get_key(FL_Shift_L) || Fl::get_key(FL_Shift_R))
				shift = 1;
			else
				shift = 0;


			if (c == FL_RIGHT_MOUSE)
				shift = 1;

			// Determine line & col of mouse click
			col_click = (int) ((xp - x()) / m_Width);
			line_click = (int) ((yp - y()) / m_Height);

			// Get LineStart index
			lineIndex = (m_FirstLine + line_click) >> 1;

			// Get comLogEntry of first item on this line
			if (lineIndex < m_LineStartCount)
			{
				clb = m_LineStarts[lineIndex].clb;
				index = m_LineStarts[lineIndex].index;
			}
			else
				clb = NULL;

			// Check if mouse was clicked below existing data
			if (clb == NULL)
				cle_sel = NULL;

			cnt = 0;
			col = 0;
			cle = NULL;
			cle_sel = NULL;
			line = lineIndex << 1;
			adder = gHexOn ? 3 : 1;

			// Scan through comLogEntries to find this entry
			while (clb != NULL)
			{
				if (index >= clb->used)
					break;

				if (col > col_click)
					break;

				// Calculate col/line where data should be drawn
				if (cle != NULL)
				{
					// Check if last char was save type as this & advance if it was
					if ((cle->flags & 0x80) == (clb->entries[index].flags & 0x80) ||
						(cnt == 2))
					{
						// Add 1 or 3 bytes (2 hex plus space)
						col += adder;
						cnt = 0;
						if (col + adder >= m_Cols)
						{
							col = 0;
							line += 2;
							break;
						}
					}
				}

				// Save pointer to this cle for next comparison
				cle = &clb->entries[index];

				// Check if we are on the correct column
				if ((col_click >= col) && (col_click < col + adder))
				{
					// Check for correct line number
					if (cle->flags & 0x80)
					{
						if (line == line_click)
						{
							cle_sel = &clb->entries[index];	
							break;
						}
					}
					else
					{
						if (line + 1 == line_click)
						{
							cle_sel = &clb->entries[index];
							break;
						}
					}
				}

				cnt++;

				// Get location of next byte
				if (++index == clb->max)
				{
					// Get pointer to next clb
					if (clb->next == NULL)
						// No more data -- break loop
						break;

					clb = clb->next;
					index = 0;
				}
			}

			// Check for  existing start or stop character
			prev_cle = NULL;
			if (shift)
			{
				// Check if for pre-existing stop selection
				if (m_pStopTime != NULL)
				{
					// Check if char is currently displayed
					if ((m_StopTimeLine >= m_FirstLine) && 
						(m_StopTimeLine < m_FirstLine + m_Lines))
					{
						xpos = (int) (x() + m_StopTimeCol * m_Width);
						ypos = (int) (y() + m_StopTimeLine * m_Height);
						prev_cle = m_pStopTime;
						line = m_StopTimeLine;
					}
				}
			}
			else
			{
				// Check if for pre-existing stop selection
				if (m_pStartTime != NULL)
				{
					// Check if char is currently displayed
					if ((m_StartTimeLine >= m_FirstLine) && 
						(m_StartTimeLine < m_FirstLine + m_Lines))
					{
						xpos = (int) (x() + m_StartTimeCol * m_Width);
						ypos = (int) (y() + m_StartTimeLine * m_Height);
						prev_cle = m_pStartTime;
						line = m_StartTimeLine;
					}
				}
			}

			// "Unselect" previous start or stop char
			window()->make_current();
			// Select 12 point Courier font
			fl_font(FL_COURIER_BOLD,gFontSize);
			if (prev_cle != NULL)
			{
				// Draw background
				fl_color(FL_WHITE);
				fl_rectf(xpos, ypos+3, (int) (adder * m_Width), (int) m_Height-1);

				if (line & 0x01)
					fl_color(FL_BLUE);
				else
					fl_color(FL_RED);

				// Draw the text
				if (gHexOn)
					sprintf(string, "%02X", prev_cle->byte);
				else
					sprintf(string, "%c", prev_cle->byte);
				fl_draw(string, xpos, ypos+(int) m_Height-2);

				// Restore line number of current selection
				line = line_click;
			}

			// Check if cursor was clicked on valid data
			if (cle_sel != NULL)
			{
				// Check if shift key was pressed during mouse click
				if (shift)
				{
					m_pStopTime = cle_sel;
					sprintf(periph_ctrl.com.sStopChar, "%c (%02xh)", cle_sel->byte, cle_sel->byte);
					periph_ctrl.com.pStopChar->label(periph_ctrl.com.sStopChar);
					m_pStopTime = cle_sel;
					m_StopTimeLine = line_click;
					m_StopTimeCol = col;
					fl_color(FL_RED);
				}
				else
				{
					m_pStartTime = cle_sel;
					sprintf(periph_ctrl.com.sStartChar, "%c (%02xh)", cle_sel->byte, cle_sel->byte);
					periph_ctrl.com.pStartChar->label(periph_ctrl.com.sStartChar);
					m_pStartTime = cle_sel;
					m_StartTimeLine = line_click;
					m_StartTimeCol = col;
				}

				if (m_pStartTime != NULL)
				{
					// Draw the Start selecton box
					fl_color(FL_GREEN);
					xpos = (int) (x() + m_StartTimeCol * m_Width);
					ypos = (int) (y() + m_StartTimeLine * m_Height);
					fl_rectf(xpos, ypos+3, (int) (adder * m_Width), (int) m_Height-1);
					
					// Draw the StartTime text
					fl_color(FL_BLACK);
					if (gHexOn)
						sprintf(string, "%02X", m_pStartTime->byte);
					else
						sprintf(string, "%c", m_pStartTime->byte);
					fl_draw(string, xpos, ypos + (int)m_Height-2);
				}
				
				if (m_pStopTime != NULL)
				{
					// Draw the Start selecton box
					if (m_pStartTime == m_pStopTime)
						fl_color(FL_YELLOW);
					else
						fl_color(FL_RED);
					xpos = (int) (x() + m_StopTimeCol * m_Width);
					ypos = (int) (y() + m_StopTimeLine * m_Height);
					fl_rectf(xpos, ypos+3, (int) (adder * m_Width), (int) m_Height-1);
					
					// Draw the StartTime text
					if (m_pStartTime == m_pStopTime)
						fl_color(FL_BLACK);
					else
						fl_color(FL_WHITE);
					if (gHexOn)
						sprintf(string, "%02X", m_pStopTime->byte);
					else
						sprintf(string, "%c", m_pStopTime->byte);
					fl_draw(string, xpos, ypos + (int)m_Height-2);
				}
			}	
			else
			{
				if (shift)
				{
					m_pStopTime = NULL;
					strcpy(periph_ctrl.com.sStopChar, "No Data");
					periph_ctrl.com.pStopChar->label(periph_ctrl.com.sStopChar);
				}
				else
				{
					m_pStartTime = NULL;
					strcpy(periph_ctrl.com.sStartChar, "No Data");
					periph_ctrl.com.pStartChar->label(periph_ctrl.com.sStartChar);
				}
			}

			// Check if user has selected both the start & stop bytes
			if ((m_pStopTime != NULL) && (m_pStartTime != NULL))
			{
				delta = (m_pStopTime->time - m_pStartTime->time) * 1000.0;
				if ((delta > 1000.0) || (delta < -1000.0))
					sprintf(periph_ctrl.com.sDeltaTime, "Time = %.3f s", delta/1000.0);
				else
					sprintf(periph_ctrl.com.sDeltaTime, "Time = %.1f ms", delta);
			}
			else
				strcpy(periph_ctrl.com.sDeltaTime, "");

			periph_ctrl.com.pDeltaTime->label(periph_ctrl.com.sDeltaTime);
		}
		break;

	default:
		Fl_Widget::handle(event);
		break;

	}

	return 1;
}
