/* tpddserverlog.h */

/* $Id: tpddserverlog.h,v 1.1 2013/02/20 20:47:47 kpettit1 Exp $ */

/*
* Copyright 2013 Ken Pettit
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

#ifndef TPDDSERVERLOG_H
#define TPDDSERVERLOG_H

/*
============================================================================
Define call routines to hook to serial port functionality.
============================================================================
*/
// If compiled in cpp file, make declarations extern "C"
#ifdef __cplusplus

#include <FL/filename.H>
#include <FL/Fl_Window.H>
#include <FL/Fl_Scrollbar.H>
#include <FL/Fl_Menu_Bar.H>
#include <FL/Fl_Check_Button.H>
#include "MString.h"
#include "MStringArray.h"
#include "tpddserver.h"
#include "vtobj.h"

/*
=====================================================================
Define the class for a single log entry
=====================================================================
*/
class VTTpddLogEntry : public VTObject
{
public:
	VTTpddLogEntry() { m_count = 0; m_pData = NULL; }

	int			m_ref;				// Reference number
	int			m_rxTx;				// Indicates if this is an RX or TX entry
	int			m_count;			// Number of bytes 
	char*		m_pData;			// Pointer to the data
};

/*
=====================================================================
Define the TPDD Server Log class.
=====================================================================
*/
class VTTpddServerLog : public Fl_Double_Window
{
public: 
	VTTpddServerLog(int w, int h, const char* pTitle);
	~VTTpddServerLog();

	// Methods
	void				draw(void);
	int					handle(int event);
	void				resize(int x, int y, int w, int h);

	void				Server(VTTpddServer* pServer) { m_pServer = pServer; }
	void				LogData(char data, int rxTx);
	void				ResetContent(void);
	void				CheckboxCallback(void);

	void				LoadPreferences(void);
	void				SavePreferences(void);
	void				ResizeToPref(void);

	void				SaveFile(MString filename);
	int					LoadFile(MString filename);

	int					m_callbackActive;		// Indicates if redraw timeout call back active

private:
	// Methods
	void				AddNewEntry(int rxTx, int count, char* buffer);
	void				SetScrollSizes(void);

	// TPDD Server interface
	VTTpddServer*		m_pServer;				// Pointer to the TpddServer we are logging
	int					m_enabled;				// Indicates if the log is enabled

	// RX and TX buffer control
	int					m_lastWasRx;			// Indicates if last logged data was RX
	int					m_rxCount;				// Count of data in rx buffer
	int					m_txCount;				// Count of data in tx buffer
	char				m_rxBuffer[256];		// RX accumulation buffer
	char				m_txBuffer[256];		// TX accumulation buffer

	// The actual log
	VTObArray			m_log;					// Array of log entries
	int					m_maxLogEntries;		// Maximum number of log entries we keep
	int					m_nextRef;				// Reference entry count

	// Drawing parameters
	int					m_height;				// Height of each character
	int					m_width;				// Width of each character
	int					m_fontSize;				// Font size selection
	int					m_bytesPerLine;			// Number of bytes drawn per line

	// Define a structure for our colors
	typedef struct log_colors
	{
		Fl_Color		background;				// Window background color
		Fl_Color		ref;					// Color for reference number
		Fl_Color		rxLabel;				// Color of RX: label
		Fl_Color		txLabel;				// Color of TX: label
		Fl_Color		rxHex;					// Color of RX HEX values
		Fl_Color		txHex;					// Color of TX HEX values
		Fl_Color		rxAscii;				// Color of RX ASCII data
		Fl_Color		txAscii;				// Color of TX ASCII data
	} log_colors_t;

	log_colors_t		m_colors;				// Color coding for the window
	int					m_x, m_y, m_w, m_h;		// User preference window location

	// FLTK widget stuff
	Fl_Menu_Bar*		m_pMenu;				// Menu bar
	Fl_Scrollbar*		m_pScroll;				// Pointer to our scrollbar
	Fl_Check_Button*	m_pDisable;				// Pointer to disable checkbox
	Fl_Double_Window*	m_pLog;					// Window for the log items
};

#endif  /* __cplusplus */

#endif	/* TPDDSERVERLOG_H */

