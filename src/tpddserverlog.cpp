/* tpddserverlog.cpp */

/* $Id: tpddserverlog.cpp,v 1.4 2013/02/20 22:19:45 kpettit1 Exp $ */

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

#include <stdio.h>
#include <string.h>

#include <FL/Fl.H>
#include <FL/fl_ask.H>
#include <FL/Fl_Preferences.H>

#include "FLU/Flu_File_Chooser.h"

#include "VirtualT.h"
#include "m100emu.h"
#include "tpddserver.h"
#include "tpddserverlog.h"
#include "display.h"
#include "serial.h"

// ===============================================
// Extern "C" linkage items
// ===============================================
extern "C"
{
extern volatile UINT64			cycles;
extern volatile DWORD			rst7cycles;

}

// ===============================================
// Extern and global variables
// ===============================================
extern	Fl_Preferences		virtualt_prefs;
VTTpddServerLog*			gpLog = NULL;

static void cb_load_log(Fl_Widget* w, void* pOpaque);
static void cb_save_log(Fl_Widget* w, void* pOpaque);
static void cb_setup_log(Fl_Widget* w, void* pOpaque);

void cb_CpuRegs(Fl_Widget* w, void* pOpaque);
void cb_Ide(Fl_Widget* w, void* pOpaque);
void disassembler_cb(Fl_Widget* w, void* pOpaque);
void cb_PeripheralDevices(Fl_Widget* w, void* pOpaque);
void cb_FileView(Fl_Widget* w, void* pOpaque);

static Fl_Menu_Item gServerLog_menuitems[] = {
  { "&File",              		0, 0, 0, FL_SUBMENU },
	{ "Load from File...",      0, cb_load_log, 0 },
	{ "Save to File...",      	0, cb_save_log, 0, 0 /*FL_MENU_DIVIDER*/ },
//	{ "Setup...",      			0, cb_setup_log, 0},
	{ 0 },

  { "&Tools", 0, 0, 0, FL_SUBMENU },
	{ "CPU Registers",         0, cb_CpuRegs },
	{ "Assembler / IDE",       0, cb_Ide },
	{ "Disassembler",          0, disassembler_cb },
	{ "Peripheral Devices",    0, cb_PeripheralDevices },
	{ "Model T File Viewer",   0, cb_FileView },
	{ 0 },

  { 0 }
};

/*
============================================================================
Callback routine for the File Viewer window
============================================================================
*/
void cb_server_log (Fl_Widget* w, void* pOpaque)
{
	VTTpddServerLog* pLog = (VTTpddServerLog *) pOpaque;

	// Hide the window
	pLog->hide();

	// If we are closing the root window, then save preferences
	if (pLog == gpLog)
		pLog->SavePreferences();

	// Delete the window and set to NULL
	delete pLog;

	if (pLog == gpLog)
	{
		VTTpddServer* pServer = (VTTpddServer *) ser_get_tpdd_context();
		pServer->UnregisterServerLog(pLog);
		gpLog = NULL;
	}
}

/*
============================================================================
Callback for disable checkbox
============================================================================
*/
static void cb_disable_log(Fl_Widget* w, void* pOpaque)
{
	VTTpddServerLog* pLog = (VTTpddServerLog *) pOpaque;

	pLog->CheckboxCallback();
}

/*
============================================================================
Callback for load log
============================================================================
*/
static void cb_load_log(Fl_Widget* w, void* pOpaque)
{
	Flu_File_Chooser*	fc;
	char				fc_path[256];
	int					count;
	const char			*filename;
	VTTpddServerLog*	pLog;
	CFileString			fileStr;
	MString				filePath;

	// Create chooser window to pick file
	strcpy(fc_path, path);
	strcat(fc_path,"/*.txt");
	fl_cursor(FL_CURSOR_WAIT);
	fc = new Flu_File_Chooser(fc_path, "Text Files (*.txt)", 0, "Load Log Data From...");
	fl_cursor(FL_CURSOR_DEFAULT);
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
	filename = fc->value();
	if (filename == 0)
	{
		delete fc;
		return;
	}

	// Check extension of filename
	fileStr = filename;
	filePath = filename;
	if (fileStr.Ext() == "")
		filePath += (char *) ".txt";

	// Copy the new root
	pLog = (VTTpddServerLog *) pOpaque;
	if (pLog == NULL)
		pLog = gpLog;
	if (pLog == NULL)
		pLog = gpLog;
	pLog->LoadFile((const char *) filePath);

	delete fc;
}

/*
============================================================================
Callback for setup dialog
============================================================================
*/
static void cb_setup_log(Fl_Widget* w, void* pOpaque)
{
}

/*
============================================================================
Callback for redrawing the window
============================================================================
*/
static void cb_redraw(void* pOpaque)
{
	VTTpddServerLog* pLog = (VTTpddServerLog *) pOpaque;
	pLog->m_callbackActive = FALSE;
	pLog->redraw();
}

/*
============================================================================
Callback for the scrollbar
============================================================================
*/
static void cb_scroll_log(Fl_Widget* w, void* pOpaque)
{
	VTTpddServerLog* pLog = (VTTpddServerLog *) pOpaque;
	pLog->redraw();
}

/*
============================================================================
Callback for the clear log button
============================================================================
*/
static void cb_clear_log(Fl_Widget* w, void* pOpaque)
{
	VTTpddServerLog* pLog = (VTTpddServerLog *) pOpaque;
	pLog->ResetContent();
	pLog->resize(pLog->x(), pLog->y(), pLog->w(), pLog->h());
	pLog->redraw();
}

/*
============================================================================
Callback routine to read TPDD / NADSBox directory
============================================================================
*/
static void cb_save_log(Fl_Widget* w, void* pOpaque)
{
	Flu_File_Chooser*	fc;
	char				fc_path[256];
	int					count;
	const char			*filename;
	VTTpddServerLog*	pLog;
	CFileString			fileStr;
	MString				filePath;

	// Create chooser window to pick file
	strcpy(fc_path, path);
	strcat(fc_path,"/*.txt");
	fl_cursor(FL_CURSOR_WAIT);
	fc = new Flu_File_Chooser(fc_path, "Text Files (*.txt)", 0, "Save Log Data As...");
	fl_cursor(FL_CURSOR_DEFAULT);
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
	filename = fc->value();
	if (filename == 0)
	{
		delete fc;
		return;
	}

	// Check extension of filename
	fileStr = filename;
	filePath = filename;
	if (fileStr.Ext() == "")
		filePath += (char *) ".txt";

	// Copy the new root
	pLog = (VTTpddServerLog *) pOpaque;
	pLog->SaveFile((const char *) filePath);

	delete fc;
}

/*
============================================================================
Routine to create the TPDD Server configuration window
============================================================================
*/
void cb_TpddServerLog(Fl_Widget* w, void* pOpaque)
{
	VTTpddServerLog*	pLog;

	// Test if requesting a 2nd root log window
	if (pOpaque == NULL && gpLog != NULL)
	{
		gpLog->show();
		return;
	}

	// Create TPDD Server Log window
	pLog = new VTTpddServerLog(470, 300, "TPDD Server Log");
	pLog->callback(cb_server_log, pLog);
	pLog->LoadPreferences();

	// Show the window
	pLog->show();

	// If this is the root log, then assign it
	if (pOpaque == NULL)
	{
		// Save this as the root log
		gpLog = pLog;

		VTTpddServer* pServer = (VTTpddServer *) ser_get_tpdd_context();
		pServer->RegisterServerLog(pLog);
		pLog->Server(pServer);

		// Now resize if we have saved preferences
		pLog->ResizeToPref();
	}
}

/*
============================================================================
Class constructor
============================================================================
*/
VTTpddServerLog::VTTpddServerLog(int w, int h, const char* title) :
	Fl_Double_Window(w, h, title)
{
	Fl_Box*				o;
	Fl_Button*			b;
	Fl_Group*			g;

	// Initialize everything
	m_pServer = NULL;
	m_enabled = TRUE;
	m_lastWasRx = FALSE;
	m_rxCount = m_txCount = 0;
	m_maxLogEntries = 8192;
	m_nextRef = 1;
	m_callbackActive = FALSE;

	// Define our default colors
	m_colors.background = FL_BLACK;
	m_colors.ref = FL_WHITE;
	m_colors.rxLabel = FL_YELLOW;
	m_colors.txLabel = (Fl_Color) 221;
	m_colors.rxHex = fl_color_average(FL_DARK_GREEN, FL_WHITE, 0.8);
	m_colors.txHex = fl_color_average((Fl_Color) 221, FL_WHITE, 0.5);
	m_colors.rxAscii = FL_GREEN;
	m_colors.txAscii = (Fl_Color) 221;
	m_fontSize = 14;

	fl_font(FL_COURIER, m_fontSize);
	m_height = fl_height();
	m_width = fl_width("W");

	// ===============================
	// Now create the controls we need
	// ===============================

	// Create a menu
	m_pMenu = new Fl_Menu_Bar(0, 0, w, MENU_HEIGHT-2);
	m_pMenu->menu(gServerLog_menuitems);

	// Create a window for the log
	m_pLog = new Fl_Double_Window(10, MENU_HEIGHT+10, w-20-15, h-MENU_HEIGHT-50, "");
	//m_pLog->color(FL_BLACK);
	m_pLog->end();
	m_pLog->hide();

	// Create a scrollbar
	m_pScroll = new Fl_Scrollbar(w-10-15, MENU_HEIGHT+10, 15, h-MENU_HEIGHT-50, "");
	m_pScroll->callback(cb_scroll_log, this);

	// Create a resizing group
	g = new Fl_Group(0, h-35, w, 35, "");

	// Create a disable log checkbox
	m_pDisable = new Fl_Check_Button(20, h-30, 110, 20, "Disable log");
	m_pDisable->callback(cb_disable_log, this);

	// Create a Save button
	b = new Fl_Button(150, h-30, 80, 20, "Save");
	b->align(FL_ALIGN_CENTER | FL_ALIGN_INSIDE);
	b->callback(cb_save_log, this);

	// Create a Load button
	b = new Fl_Button(250, h-30, 80, 20, "Load");
	b->align(FL_ALIGN_CENTER | FL_ALIGN_INSIDE);
	b->callback(cb_load_log, this);

	// Create a clear button
	b = new Fl_Button(350, h-30, 80, 20, "Clear");
	b->align(FL_ALIGN_CENTER | FL_ALIGN_INSIDE);
	b->callback(cb_clear_log, this);
	
	// Make the group resizable
	o = new Fl_Box(440, 350, 5, 5, "");
	g->resizable(o);
	g->end();

	// Make the window resizable
	o = new Fl_Box(20, MENU_HEIGHT + 30, 5, 5, "");
	resizable(o);

	// Set the scrollbar size
	SetScrollSizes();
}

/*
============================================================================
Class destructor
============================================================================
*/
VTTpddServerLog::~VTTpddServerLog(void)
{
	// Reset all content
	ResetContent();
}

/*
============================================================================
Our custom draw routine
============================================================================
*/
void VTTpddServerLog::draw(void)
{
	int				xMargin = 7;
	int				topEntry, lines, line, idx;
	int				dataIdx, x, count, tempRef = m_nextRef;
	int				wx, wy, dataStart, labelStart, dataX, labelX, asciiX;
	char			str[10];
	VTTpddLogEntry*	pEntry, rxEntry, txEntry;

	// Do default FLTK stuff
	Fl_Double_Window::draw();

	// Push a clipping rect
	fl_push_clip(m_pLog->x(), m_pLog->y(), m_pLog->w(), m_pLog->h());

	// Draw the log background
	fl_color(m_colors.background);
	fl_font(FL_COURIER, m_fontSize);
	fl_rectf(m_pLog->x(), m_pLog->y(), m_pLog->w(), m_pLog->h());

	// Calculate the number of lines that fit in the window
	lines = m_pLog->h() / m_height;
	wy = m_pLog->y();
	wx = m_pLog->x();

	// Get the top entry of the display
	topEntry = m_pScroll->value();

	// Create fake rx and tx entries
	rxEntry.m_ref = tempRef;
	if (m_rxCount > 0)
		tempRef++;
	rxEntry.m_pData = m_rxBuffer;
	rxEntry.m_rxTx = 0;
	rxEntry.m_count = m_rxCount;

	txEntry.m_ref = tempRef;
	txEntry.m_pData = m_txBuffer;
	txEntry.m_rxTx = 1;
	txEntry.m_count = m_txCount;

	// Precalculate x offsets
	labelStart = 6;
	dataStart = labelStart + 4;
	labelX = labelStart * m_width + wx + xMargin;
	dataX = dataStart * m_width + wx + xMargin;
	asciiX = dataX + (2 + m_bytesPerLine * 3) * m_width;

	// Loop for all lines that can be displayed
	line = 1;
	idx = 0;

	count = m_log.GetSize();
	while (line <= lines && topEntry + idx < count + 2)
	{
		// Get the next log item to be displayed
		if (topEntry + idx == count)
			pEntry = &rxEntry;
		else if (topEntry + idx == count+1)
			pEntry = &txEntry;
		else
			pEntry = (VTTpddLogEntry *) m_log[topEntry + idx];
		dataIdx = 0;

		// Draw RX or TX as per the entry
		if (pEntry->m_count > 0)
		{
			// Draw the reference number
			if (pEntry->m_ref != -1)
			{
				// Print the reference number
				fl_color(m_colors.ref);
				sprintf(str, "%4d:", pEntry->m_ref);
				fl_draw(str, wx + xMargin, wy + line * m_height);
			}

			// Draw either TX or RX
			if (pEntry->m_rxTx)
			{
				// It's a TX entry
				fl_color(m_colors.txLabel);
				fl_draw("TX:", labelX, wy + line * m_height);
				fl_color(m_colors.txHex);
			}
			else
			{
				// It's an RX entry
				fl_color(m_colors.rxLabel);
				fl_draw("RX:", labelX, wy + line * m_height);
				fl_color(m_colors.rxHex);
			}
		}

		// Loop for all data
		while (dataIdx < pEntry->m_count && line <= lines)
		{
			// Select the proper color
			if (pEntry->m_rxTx)
				fl_color(m_colors.txHex);
			else
				fl_color(m_colors.rxHex);

			// Draw bytesPerLine hex values on the current line
			int c = 0;
			for (x = dataIdx; x < pEntry->m_count && c < m_bytesPerLine; x++)
			{
				// Format the data  to draw the HEX data
				sprintf(str, "%02X", (unsigned char) pEntry->m_pData[x]);
				fl_draw(str, dataX + c++ * m_width*3, wy + line * m_height);
			}

			// Select the proper color
			if (pEntry->m_rxTx)
				fl_color(m_colors.txAscii);
			else
				fl_color(m_colors.rxAscii);

			// Draw bytesPerLine ASCII values
			c = 0;
			for (x = dataIdx; x < pEntry->m_count && c < m_bytesPerLine; x++)
			{
				// Format the data  to draw the HEX data
				if (pEntry->m_pData[x] >= ' ' && pEntry->m_pData[x] <= '~')
					sprintf(str, "%c", pEntry->m_pData[x]);
				else
					strcpy(str, ".");
				fl_draw(str, asciiX + c++ * m_width, wy + line * m_height);
			}

			// Increment to next dataIdx
			dataIdx = x;
			line++;
		}

		// Advance to next entry
		idx++;
	}

	// Pop the clipping rect
	fl_pop_clip();
}

/*
============================================================================
Our custom handle routine
============================================================================
*/
int VTTpddServerLog::handle(int event)
{
	// Do default FLTK stuff
	return Fl_Double_Window::handle(event);
}

/*
============================================================================
Our custom resize routine
============================================================================
*/
void VTTpddServerLog::resize(int nx, int ny, int nw, int nh)
{
	// Do default FLTK stuff
	Fl_Double_Window::resize(nx, ny, nw, nh);

	int				xOffset = 7;
	m_bytesPerLine = (m_pLog->w() - xOffset*2 - 12*m_width) / 4 / m_width;

	// Now set the new scrollbar size
	SetScrollSizes();
}

/*
============================================================================
Log data to the TPPD Server Log
============================================================================
*/
void VTTpddServerLog::LogData(char data, int rxTx)
{
	// Test if logging is enabled
	if (m_enabled)
	{
		int		cmdlineState = m_pServer->IsCmdlineState();

		// Test if logging RX data
		if (rxTx == TPDD_LOG_RX)
		{
			// Test if last logged data was RX or not
			if (!m_lastWasRx && !cmdlineState)
			{
				// We are starting a new RX entry.  Save the existing TX entry 
				// And start a new RX entry
				if (m_rxCount > 0)
					AddNewEntry(TPDD_LOG_RX, m_rxCount, m_rxBuffer);
				if (m_txCount > 0)
					AddNewEntry(TPDD_LOG_TX, m_txCount, m_txBuffer);
				m_txCount = 0;
				m_rxCount = 0;
				//printf("\nRX:  ");
			}
			m_lastWasRx = TRUE;
			m_rxBuffer[m_rxCount++] = data;
			
			// Test if the rxBuffer is full
			if (m_rxCount == sizeof(m_rxBuffer))
			{
				// Need to dump this packet and start a new one
				AddNewEntry(TPDD_LOG_RX, m_rxCount, m_rxBuffer);
				m_rxCount = 0;
			}

			//printf("%02X ", (unsigned char) data);
		}
		else
		{
			// Logging TX data test if starting a new TX packet
			if (m_lastWasRx && !cmdlineState)
			{
				// We are starting a new RX entry.  Save the existing TX entry 
				// And start a new RX entry
				if (m_rxCount > 0)
					AddNewEntry(TPDD_LOG_RX, m_rxCount, m_rxBuffer);
				if (m_txCount > 0)
					AddNewEntry(TPDD_LOG_TX, m_txCount, m_txBuffer);
				m_txCount = m_rxCount = 0;
				//printf("\nTX:  ");
			}
			m_lastWasRx = FALSE;

			// Add data to the tx buffer
			m_txBuffer[m_txCount++] = data;

			// Test if the TX buffer is full
			if (m_txCount == sizeof(m_txBuffer))
			{
				// Need to dump this packet and start a new one
				AddNewEntry(TPDD_LOG_TX, m_txCount, m_txBuffer);
				m_txCount = 0;
			}

			//printf("%02X ", (unsigned char) data);
		}
	}


	// TODO:  Make redrawing smarter and add auto-scroll
	if (!m_callbackActive)
	{
		Fl::add_timeout(0.1, cb_redraw, this);
		m_callbackActive = TRUE;
	}
}

/*
============================================================================
Add a new log entry to the log
============================================================================
*/
void VTTpddServerLog::AddNewEntry(int rxTx, int count, char* pBuffer)
{
	VTTpddLogEntry*	pEntry = new VTTpddLogEntry;

	// Validate memory was allocated
	if (pEntry != NULL)
	{
		pEntry->m_ref = m_nextRef++;
		pEntry->m_count = count;
		pEntry->m_rxTx = rxTx;
		pEntry->m_pData = new char[count];

		// Validate memory was allocated
		if (pEntry->m_pData == NULL)
		{
			// Delete this entry and return
			delete pEntry;
			return;
		}

		// Copy memory to the buffer
		memcpy(pEntry->m_pData, pBuffer, count);

		// Test if the max number of entries already reached
		// and delete the oldest one if full
		if (m_log.GetSize() >= m_maxLogEntries)
		{
			// Get the oldest entry and delete it
			VTTpddLogEntry* pDelEntry = (VTTpddLogEntry *) m_log[0];
			delete[] pDelEntry->m_pData;
			delete pDelEntry;

			// Now remove it from the log
			m_log.RemoveAt(0, 1);
		}

		// Add the entry to our log
		m_log.Add(pEntry);

		// TODO: Update scrollbar settings
		SetScrollSizes();
	}
}

/*
============================================================================
Sets the scrollbar settings based on current geometry and item count.
============================================================================
*/
void VTTpddServerLog::SetScrollSizes(void)
{
	int				size, height, count, max;
	int				lines, c;
	VTTpddLogEntry*	pEntry;

	// Select 12 point Courier font
	fl_font(FL_COURIER, m_fontSize);

	// Get character width & height
	height = m_pLog->h();
	size = (int) (height / m_height);
	count = m_log.GetSize();

	// Count the number of lines that will be disiplayed
	if (m_bytesPerLine == 0)
		m_bytesPerLine = 1;
	lines = 0;
	for (c = 0; c < count; c++)
	{
		// Get next entry
		pEntry = (VTTpddLogEntry *) m_log[c];
		lines += (pEntry->m_count+m_bytesPerLine-1) / m_bytesPerLine;
	}
	lines += (m_rxCount+m_bytesPerLine-1) / m_bytesPerLine;
	lines += (m_txCount+m_bytesPerLine-1) / m_bytesPerLine;

	// Set maximum value of scroll 
	if (m_pScroll->value()+size-1 > count )
	{
		int newValue = count-size+1;
		if (newValue < 0)
			newValue = 0;
		m_pScroll->value(newValue, 0, 0, count-2);
	}
	max = count + 1 - size;
	if (max < 1)
		max = 1;
	m_pScroll->maximum(count-2);
	m_pScroll->minimum(0);
	if (lines > 1)
	{
		m_pScroll->step((double) size/(double)(lines-1));
		m_pScroll->slider_size((double) size/(double)(lines-1));
	}
	m_pScroll->linesize(1);

	redraw();
}

/*
============================================================================
Resets the content of the log (i.e. clears it out)
============================================================================
*/
void VTTpddServerLog::ResetContent(void)
{
	int				count, c;
	VTTpddLogEntry*	pEntry;

	count = m_log.GetSize();
	for(c = 0; c < count; c++)
	{
		// Get next log entry
		pEntry = (VTTpddLogEntry *) m_log[c];

		// Delete the data memory
		delete[] pEntry->m_pData;
		delete pEntry;
	}

	m_log.RemoveAll();

	// Reset other vars too
	m_rxCount = m_txCount = 0;
	m_lastWasRx = FALSE;
	m_nextRef = 1;
}

/*
============================================================================
Called when the disable log checkbox is modified
============================================================================
*/
void VTTpddServerLog::CheckboxCallback(void)
{
	m_enabled = !m_pDisable->value();
}

/*
============================================================================
Called to resize the window and position to user preference settings
============================================================================
*/
void VTTpddServerLog::ResizeToPref(void)
{
	if (m_x != -1 && m_y != -1 && m_w != -1 && m_h != -1)
		resize(m_x, m_y, m_w, m_h);
}

/*
============================================================================
Saves the log data to a file using the current window width
============================================================================
*/
void VTTpddServerLog::SaveFile(MString filename)
{
	FILE*			fd;
	int				count, c, i, dataIdx;
	VTTpddLogEntry*	pEntry;
	MString			fmt, hexFmt;

	// Validate we have data
	if (m_log.GetSize() == 0)
		return;

	// Test if the file exists
	if ((fd = fopen((const char *) filename, "r")) != NULL)
	{
		// Close the file
		fclose(fd);
		int ans = fl_choice("Overwrite existing file %s?", "Ok", "Cancel", NULL, 
				fl_filename_name((const char *) filename));	
		if (ans == 1)
			return;
	}

	// Try to open the file
	if ((fd = fopen((const char *) filename, "w")) == NULL)
	{
		MString err;

		err.Format("Unable to create file %s\n", (const char *) filename);
		fl_message("%s", (const char *) err);
		return;
	}

	// Test if any leftover RX or TX data not logged to an entry yet
	if (m_rxCount > 0)
		AddNewEntry(TPDD_LOG_RX, m_rxCount, m_rxBuffer);
	if (m_txCount > 0)
		AddNewEntry(TPDD_LOG_TX, m_txCount, m_txBuffer);
	m_txCount = 0;
	m_rxCount = 0;
	
	// Now loop for all entries
	count = m_log.GetSize();
	for (c = 0; c < count; c++)
	{
		// Get the next entry
		pEntry = (VTTpddLogEntry *) m_log[c];
		fmt.Format("%4d: %s: ", pEntry->m_ref, pEntry->m_rxTx ? "TX" : "RX");
		i = 0;
		dataIdx = 0;
		while (dataIdx < pEntry->m_count)
		{
			if (dataIdx != 0)
				fmt = "          ";
			// "Print" the HEX data to the line
			for (i = 0; i < m_bytesPerLine && dataIdx + i < pEntry->m_count; i++)
			{
				hexFmt.Format("%02X ", (unsigned char) pEntry->m_pData[dataIdx + i]);
				fmt += hexFmt;
			}

			// Pad with spaces if less then m_bytesPerLine
			for ( ; i < m_bytesPerLine; i++)
				fmt += (char *) "   ";

			// "Print" the ASCII data to the line
			fmt += (char *) "  ";
			for (i = 0; i < m_bytesPerLine && dataIdx + i < pEntry->m_count; i++)
			{
				// Test if it's actual ASCII data or not
				if (pEntry->m_pData[dataIdx + i] >= ' ' && pEntry->m_pData[dataIdx + i] <= '~')
					hexFmt.Format("%c", (unsigned char) pEntry->m_pData[dataIdx + i]);
				else
					hexFmt = '.';
				fmt += hexFmt;
			}

			// Save to the file
			fprintf(fd, "%s\n", (const char *) fmt);
			dataIdx += i;
		}
	}

	// Close the file
	fclose(fd);
}

/*
============================================================================
Loads the log data from a file.
============================================================================
*/
int VTTpddServerLog::LoadFile(MString filename)
{
	FILE*			fd;
	int				c, lineNo, rxTx;
	//VTTpddLogEntry*	pEntry;
	//MString			fmt, hexFmt;
	char			line[256];
	char			*ptr;

	// Test if we are loading from the root window
	if (this == gpLog)
	{
		// Don't load files from the root window.  Create a new window
		VTTpddServerLog* pLog = new VTTpddServerLog(w(), h(), "TPDD Log Viewer");
		
		// Validate the window was created
		if (pLog == NULL)
			return FALSE;

		// Let the new window open the log
		if (!pLog->LoadFile(filename))
		{
			// Load was not successful.  Destroy the window
			delete pLog;
			return FALSE;
		}

		// Now show the new window
		pLog->show();
		pLog->resize(x()+MENU_HEIGHT, y()+MENU_HEIGHT, w(), h());
		return TRUE;
	}

	// Try to open the file
	if ((fd = fopen(filename, "r")) == NULL)
	{
		fl_message("Unable to open file %s", fl_filename_name((const char *) filename)); return FALSE;
	}

	// Loop for all data in the file
	lineNo = 0;
	while (fgets(line, sizeof(line), fd) != NULL)
	{
		// Start at beginning of line
		ptr = line;
		c = 0;
		lineNo++;
		
		// Search for a reference.  This means we log the existing entry and start a new one
		while (*ptr == ' ')
		{
			// Increment ptr and c
			ptr++;
			c++;
		}

		// Test if a reference found
		if (c < 4)
		{
			// Validate the file format
			if (*ptr < '0' || *ptr > '9')
			{
				// Not a trace file!!
				fl_message("This does not appear to be a valid file on line %d", lineNo);
				return FALSE;
			}

			// Reference found.  This entry and start a new one
			if (m_rxCount > 0)
				AddNewEntry(TPDD_LOG_RX, m_rxCount, m_rxBuffer);
			if (m_txCount > 0)
				AddNewEntry(TPDD_LOG_TX, m_txCount, m_txBuffer);
			m_txCount = 0;
			m_rxCount = 0;

			// Skip past the reference and find the ':'
			while (*ptr != ':' && c < sizeof(line))
			{
				// Increment pointer and index
				ptr++;
				c++;
			}

			// Test if ':' found
			if (c >= sizeof(line) || *ptr != ':')
			{
				// Not a trace file!!
				fl_message("This does not appear to be a valid file on line %d", lineNo);
				return FALSE;
			}

			// Skip the ':' and spaces after it
			ptr += 2;
			c += 2;

			// Now get RX/TX marker
			if (*ptr == 'T')
				rxTx = 1;
			else if (*ptr == 'R')
				rxTx = 0;
			else
			{
				// Not a trace file!!
				fl_message("This does not appear to be a valid file on line %d", lineNo);
				return FALSE;
			}

			// Skip past "RX: " or "TX: "
			ptr += 4;
			c += 4;
		}

		// Now we are pointing at the HEX data.  Read all data from this line into
		// either the m_rxBuffer or m_txBuffer
		while (*ptr != ' ' && *ptr != '\0' && c < sizeof(line))
		{
			unsigned char val;

			val = (*ptr > '9' ? *ptr = 'A' + 10 : *ptr - '0') << 4;
			ptr++;
			val += *ptr > '9' ? *ptr = 'A' + 10 : *ptr - '0';
			ptr += 2;
			c += 3;

			// Now save val in either rx or tx buffer
			if (rxTx)
			{
				m_txBuffer[m_txCount++] = val;
				if (m_txCount >= sizeof(m_txBuffer))
				{
					AddNewEntry(TPDD_LOG_TX, m_txCount, m_txBuffer);
					m_txCount = 0;
				}
			}
			else
			{
				m_rxBuffer[m_rxCount++] = val;
				if (m_rxCount >= sizeof(m_rxBuffer))
				{
					AddNewEntry(TPDD_LOG_RX, m_rxCount, m_rxBuffer);
					m_rxCount = 0;
				}
			}
		}
	}

	// Log any remaining data
	if (m_rxCount > 0)
		AddNewEntry(TPDD_LOG_RX, m_rxCount, m_rxBuffer);
	if (m_txCount > 0)
		AddNewEntry(TPDD_LOG_TX, m_txCount, m_txBuffer);
	m_txCount = 0;
	m_rxCount = 0;

	// Close the file
	fclose(fd);
	return TRUE;
}

/*
============================================================================
Loads the user preferences
============================================================================
*/
void VTTpddServerLog::LoadPreferences(void)
{
	Fl_Preferences g(virtualt_prefs, "TpddServerLog_Group");

	// Load window location
	g.get("x", m_x, -1);
	g.get("y", m_y, -1);
	g.get("w", m_w, -1);
	g.get("h", m_h, -1);

	// Load selected font size
	g.get("FontSize", m_fontSize, 14);
}

/*
============================================================================
Saves the user preferences
============================================================================
*/
void VTTpddServerLog::SavePreferences(void)
{
	Fl_Preferences g(virtualt_prefs, "TpddServerLog_Group");

	// Save window location
	g.set("x", x());
	g.set("y", y());
	g.set("w", w());
	g.set("h", h());

	// Save selected font size
	g.set("FontSize", m_fontSize);

}

