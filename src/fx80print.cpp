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
#include <FL/Fl_File_Chooser.H>
#include <FL/fl_ask.H>

#include <string.h>
#include <stdio.h>
#include <stdlib.h>

#include "VirtualT.h"
#include "fx80print.h"
#include "vtpaper.h"
#include "chargen.h"

extern unsigned char gFX80CharRom[256][12];

unsigned char	gIntlTable[9][12] = {
	{ 35, 36, 64, 91, 92, 93, 94, 96, 123, 124, 125, 126 },			// USA
	{ 35, 36, 0,  5,  15, 16, 94, 96, 30,  2,   1,   22  },			// FRANCE
	{ 35, 36, 16, 23, 24, 25, 94, 96, 26,  27,  28,  17  },			// GERMANY
	{ 6,  36, 64, 91, 92, 93, 94, 96, 123, 124, 125, 126 },			// U.K.   
	{ 35, 36, 64, 18, 20, 13, 94, 96, 19,  21,  14,  126 },			// DENMARK
	{ 35, 11, 29, 23, 24, 13, 25, 30, 26,  27,  14,  28  },			// SWEDEN
	{ 35, 36, 64, 5,  92, 30, 94, 2,  0,   3,   1,   4   },			// ITALY
	{ 12, 36, 64, 7,  9,  8,  94, 96, 22,  10,  125, 126 },			// SPAIN
	{ 35, 36, 64, 91, 31, 93, 94, 96, 123, 124, 125, 126 }			// JAPAN
};   

double gGraphicsDpis[7] = {
	60.0,				// Mode 0 - Single density
	120.0,				// Mode 1 - Low speed Double density
	120.0,				// Mode 2 - High speed Double density, no consecutive dots
	240.0,				// Mode 3 - Quad density
	80.0,				// Mode 4 - 640 Dots per line
	72.0,				// Mode 5 - Aspect ratio match
	90.0				// Mode 6 - DEC Screen
};

/*
================================================================================
VTFX80Print:	This is the class construcor for the FX80Print Device emulation.
================================================================================
*/
VTFX80Print::VTFX80Print(void)
{
	// Initialize pointers
	m_pPage = NULL;				// No page memory
	m_pPaper = NULL;			// No paper
	m_pRomFile = NULL;			// No ROM file control
	m_pRamFile = NULL;			// No RAM file control
	m_pUseRomFile = NULL;
	m_pUseRamFile = NULL;
	m_pPaperChoice = NULL;

	m_useRamFile = FALSE;
	m_marksMade = FALSE;

	ResetPrinter();
}

/*
================================================================================
~VTFX80Print:	This is the class construcor for the FX80Print Device emulation.
================================================================================
*/
VTFX80Print::~VTFX80Print(void)
{
	int			c, count;
	VTPaper*	pPaper;

	// Delete the page memory
	if (m_pPage != NULL)
		delete m_pPage;

	// Delete all papers
	count = m_papers.GetSize();
	for (c = 0; c < count; c++)
	{
		pPaper = (VTPaper*) m_papers[c];
		delete pPaper;
	}	
	m_papers.RemoveAll();

	// Zero out the pointers, just in case	
	m_pPage = NULL;
	m_pPaper = NULL;
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
	// Validate we have page memory
	if (m_pPage == NULL)
		return;
	
	if ((byte == 0x1C) && (m_lastChar == 0x1D))
	{
		printf("Printer Status\n");
		printf("  ESC Start = %d\n", m_escSeen);
		printf("  ESC Cmd = %d\n", m_escCmd);
		printf("  ESC Params = %d\n", m_escParamsRcvd);
		printf("  Char Set = %d\n", m_fontSource);
		printf("  User First = %d\n", m_userFirstChar);
		printf("  User Last = %d\n", m_userLastChar);
		printf("  User Update = %d\n", m_userUpdateChar);
		printf("  Print High Codes = %d\n", m_highCodesPrinted);
		printf("  Print Low Codes = %d\n", m_lowCodesPrinted);
		printf("  Graphics Mode = %d\n", m_graphicsMode);
		printf("  Graphics Length = %d\n", m_graphicsLength);
		printf("  Graphics Received = %d\n", m_graphicsRcvd);
		printf("  Graphics DPI = %f\n", (float) m_graphicsDpi);
		printf("  CurX = %d\n", m_curX);
		printf("  CurY = %d\n", m_curY);
		printf("  Line Spacing = %f\n",(float) m_lineSpacing);
		printf("  CPI = %f\n",(float) m_cpi);
	}
	m_lastChar = byte;

	// Check if in Graphics Mode
	if (m_graphicsMode)
	{
		// Process byte as graphics data
		RenderGraphic(byte);
		return;
	}

	// Check if it is a control code sequence
	if (ProcessAsCmd(byte))
		return;

	// Okay, it is a byte that needs to be printed
	// Render the specified character
	RenderChar(byte);
}

/*
=======================================================
Property Dialog callbacks
=======================================================
*/
void cb_CharRomBrowse(Fl_Widget* w, void* ptr)
{
	if (ptr != NULL)
		((VTFX80Print*) ptr)->CharRomBrowse(w);
}

void cb_UseRomCheck(Fl_Widget* w, void* ptr)
{
	if (ptr != NULL)
		((VTFX80Print*) ptr)->UseRomCheck(w);
}

/*
=======================================================
Use ROM File checkbox callback
=======================================================
*/
void VTFX80Print::UseRomCheck(Fl_Widget* w)
{
	// Enable or disable buttons base on which box checked
	if (w == m_pUseRomFile)
	{
		// Activate / deactivate ROM controls
		if (m_pUseRomFile->value())
		{
			m_pRomFile->activate();
			m_pRomBrowse->activate();
		}
		else
		{
			m_pRomFile->deactivate();
			m_pRomBrowse->deactivate();
		}
	}
	else
	{
		// Activate / deactivate RAM controls
		if (m_pUseRamFile->value())
		{
			m_pRamFile->activate();
			m_pRamBrowse->activate();
		}
		else
		{
			m_pRamFile->deactivate();
			m_pRamBrowse->deactivate();
		}
	}
}

/*
=======================================================
Property Dialog callbacks
=======================================================
*/
void VTFX80Print::CharRomBrowse(Fl_Widget* w)
{
	Fl_File_Chooser* 	fc;
	char				defaultfile[256];

	// Get default filename for browse
	strcpy(defaultfile, ".");
	if (w == m_pRomBrowse)
	{
		if (m_pRomFile->value() != 0)
			if (strlen(m_pRomFile->value()) != 0)
				strcpy(defaultfile, m_pRomFile->value());
	}
	else
	{
		if (m_pRamFile->value() != 0)
			if (strlen(m_pRamFile->value()) != 0)
				strcpy(defaultfile, m_pRamFile->value());
	}

	// Create a file browser for .fcr files
	fc = new Fl_File_Chooser(defaultfile, "*.fcr", 1, "Choose FX-80 Char ROM File");

	// Show the dialog
	fc->show();
	while (fc->visible())
		Fl::wait();

	// Test if a file was selected
	if (fc->value() == NULL)
	{
		delete fc;
		return;
	}

	// Validate lenth of file selected
	if (strlen(fc->value()) == 0)
	{
		delete fc;
		return;
	}

	// Update the appropriate edit field with the new filename
	if (w == m_pRomBrowse)
	{
		strcpy(m_romFileStr, fc->value());
		m_pRomFile->value(m_romFileStr);
	}
	else
	{
		strcpy(m_ramFileStr, fc->value());
		m_pRamFile->value(m_ramFileStr);
	}

	delete fc;
}

/*
=======================================================
Build the Property Dialog for the FX-80
=======================================================
*/
void VTFX80Print::BuildPropertyDialog(void)
{
	Fl_Box*		o;
	Fl_Button*	b;
	int			c, count;
	MString		name;

	o = new Fl_Box(20, 20, 360, 20, "Emulated Epson FX-80 Printer");

	// Create checkbox for use of external ROM file
	m_pUseRomFile = new Fl_Check_Button(20, 50, 230, 20, "Use External Character ROM");
	m_pUseRomFile->align(FL_ALIGN_LEFT | FL_ALIGN_INSIDE);
	m_pUseRomFile->value(m_useRomFile);
	m_pUseRomFile->callback(cb_UseRomCheck, this);

	// Create edit field for External ROM file
	m_pRomFile = new Fl_Input(40, 78, 260, 20, "");
	strcpy(m_romFileStr, (const char *) m_sRomFile);
	m_pRomFile->value(m_romFileStr);

	// Create Browse button
	m_pRomBrowse = new Fl_Button(315, 73, 60, 30, "Browse");
	m_pRomBrowse->callback(cb_CharRomBrowse, this);

	// Deactivate controls if check not selected
	if (!m_useRomFile)
	{
		m_pRomFile->deactivate();
		m_pRomBrowse->deactivate();
	}

	// Create checkbox for use of external RAM file
	m_pUseRamFile = new Fl_Check_Button(20, 105, 230, 20, "Initialize Character RAM");
	m_pUseRamFile->align(FL_ALIGN_LEFT | FL_ALIGN_INSIDE);
	m_pUseRamFile->value(m_useRamFile);
	m_pUseRamFile->callback(cb_UseRomCheck, this);

	// Create edit field for External RAM file
	m_pRamFile = new Fl_Input(40, 128, 260, 20, "");
	strcpy(m_ramFileStr, (const char *) m_sRamFile);
	m_pRamFile->value(m_ramFileStr);

	// Create Browse button
	m_pRamBrowse = new Fl_Button(315, 123, 60, 30, "Browse");
	m_pRamBrowse->callback(cb_CharRomBrowse, this);

	// Deactivate controls if check not selected
	if (!m_useRamFile)
	{
		m_pRamFile->deactivate();
		m_pRamBrowse->deactivate();
	}

	// Create control to selet the paper
	o = new Fl_Box(20, 155, 260, 20, "Output Paper");
	o->align(FL_ALIGN_LEFT | FL_ALIGN_INSIDE);
	m_pPaperChoice = new Fl_Choice(40, 180, 260, 20, "");

	// Populate the Paper Choice with options
	count = m_papers.GetSize();
	for (c = 0; c < count; c++)
	{
		name = ((VTPaper*) m_papers[c])->GetName();
		m_pPaperChoice->add((const char *) name);
		if (name == (const char *) m_paperName)
			m_pPaperChoice->value(c);
	}

	// Ensure a paper is selected
	if (m_pPaperChoice->value() == -1)
		m_pPaperChoice->value(0);

	// Create controls for setting DIP switch default settings
	
	b = new Fl_Button(20, 310, 120, 30, "Char Generator");
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
	char		temp[512];

	// Load preferences
	m_pPref->get("FX80Print_UseRomFile", m_useRomFile, 0);
	m_pPref->get("FX80Print_RomFile", temp, "", 512);
	m_sRomFile = temp;
	m_pPref->get("FX80Print_UseRamFile", m_useRamFile, 0);
	m_pPref->get("FX80Print_RamFile", temp, "", 512);
	m_sRamFile = temp;
	m_pPref->get("FX80Print_PaperName", temp, "", 512);
	m_paperName = temp;

	// Add all papers to the m_paper object
	m_papers.Add(new VTPSPaper(m_pPref));
	m_papers.Add(new VTVirtualPaper(m_pPref));

	// Reset printer
	ResetPrinter();

	return;
}

/*
=======================================================
Opens a new Print Session
=======================================================
*/
int VTFX80Print::OpenSession(void)
{
	// If we have paper loaded, force a reload
	if (m_pPaper != NULL)
		m_pPaper->LoadPaper();

	// Indicate no marks made on paper
	m_marksMade = FALSE;

	return PRINT_ERROR_NONE;
}

/*
=======================================================
Creates a new Paper object for printing
=======================================================
*/
void VTFX80Print::CreatePaper(void)
{
	int			c, count;
	VTPaper*	pPaper = NULL;

	// Find the selected paper
	count = m_papers.GetSize();
	for (c = 0; c < count; c++)
	{
		pPaper = (VTPaper*) m_papers[c];
		if (pPaper->GetName() == (const char *) m_paperName)
			break;
	}
	
	// Create paper if not already created
	if (pPaper != NULL)
	{
		m_pPaper = pPaper;
		m_pPaper->Init();
		m_pPaper->LoadPaper();
	}
}

/*
=======================================================
Closes the active Print Session
=======================================================
*/
int VTFX80Print::CloseSession(void)
{
	int		r, c, b;
	int		data, mask;

	// Print to the paper
	if (m_pPage != NULL)
	{
		// Create paper if not already created
		if (m_pPaper == NULL)
			CreatePaper();

		// Print the current page and close the session
		if ((m_pPaper != NULL) && m_marksMade)
		{
			m_pPaper->PrintPage(m_pPage, m_horizDots, m_vertDots);
			m_pPaper->Print();
		}

		// Zero the page memory
		for (c = 0; c < m_vertDots * m_bytesPerLine; c++)
			*(m_pPage + c) = 0;

		// Reset to top of page for next print
		m_curX = m_leftMarginX;
		m_curY = 0;
	}
	return PRINT_ERROR_NONE;
}

/*
=======================================================
Get Printer Properties from dialog and save.
=======================================================
*/
int VTFX80Print::GetProperties(void)
{
	int		index;

	if (m_pUseRomFile == NULL)
		return PRINT_ERROR_NONE;

	// Get values from the dialog controls
	m_useRomFile = m_pUseRomFile->value();
	m_useRamFile = m_pUseRamFile->value();
	if (m_pRomFile->value() != 0)
		m_sRomFile = m_pRomFile->value();

	if (m_pRamFile->value() != 0)
		m_sRamFile = m_pRamFile->value();

	m_paperName = ((VTPaper*) m_papers[m_pPaperChoice->value()])->GetName();

	// Now save values to the preferences
	m_pPref->set("FX80Print_UseRomFile", m_useRomFile);
	m_pPref->set("FX80Print_RomFile", (const char *) m_sRomFile);
	m_pPref->set("FX80Print_UseRamFile", m_useRamFile);
	m_pPref->set("FX80Print_RamFile", (const char *) m_sRamFile);
	m_pPref->set("FX80Print_PaperName", (const char *) m_paperName);

	// Close active session if one is open
	if (m_SessionActive)
		CloseSession();

	ResetPrinter();

	// Clear paper type because it may have changed
	m_pPaper = NULL;

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
	// Delete page memory
	if (m_pPage != NULL)
		delete m_pPage;
	m_pPage = NULL;

	m_pPaper = NULL; 

	return;
}

/*
=======================================================
Cancels the current print job.
=======================================================
*/
int VTFX80Print::CancelPrintJob(void)
{
	if (m_pPaper != NULL)
		m_pPaper->CancelJob();
	return PRINT_ERROR_NONE;
}

/*
=======================================================
Reset the printer mode to default, power on settings.
=======================================================
*/
void VTFX80Print::ResetMode(void)
{
	// Reset protocol settings
	m_escSeen = m_escCmd = 0;					// No active ESC commands
	m_escParamsRcvd = 0;						// No ESC parameters needed
	m_topMargin = m_leftMargin = 0.0;			// Default to .5" margin
	m_leftMarginX = 0;							// Left margin in dots
	m_rightMarginX = 0;							// Right margin in dots
	m_cpi = 10.0;								// Default to Pica (10 cpi) font
	m_elite = FALSE;							// Turn off elite print mode
	m_compressed = FALSE;						// Turn off compressed print mode
	m_lineSpacing = 1.0 / 6.0;					// Default to 1/6" line spacing
	m_fontSource = 0;							// Default to ROM font
	m_proportional = FALSE;						// Default to non-proportional
	m_enhanced = FALSE;							// Default to non-enhanced
	m_dblStrike = FALSE;						// Default to non-dbl strike
	m_expanded = FALSE;							// Default to non-expanded
	m_italic = FALSE;
	m_underline = FALSE;
	m_superscript = m_subscript = FALSE;
	m_intlSelect = 0;							// Select USA char set
	m_userUpdateChar = 0;						// Reset user define char vars
	m_userFirstChar = 0;
	m_userLastChar = 0;
	m_userCharSet = 0;
	m_highCodesPrinted = 0;						// Turn off printing of high codes
	m_lowCodesPrinted = 0;						// Turn off printing of low codes
	m_graphicsMode = FALSE;						// Turn off graphics mode
	m_escKmode = 0;								// Default ESC K to mode 0
	m_escLmode = 1;								// Default ESC L to mode 1
	m_escYmode = 2;								// Default ESC Y to mode 2
	m_escZmode = 3;								// Default ESC Z to mode 3
	m_graphicsDpi = 60.0;						// Default graphics mode to 60
	m_graphicsRcvd = 0;

	ResetTabStops();							// Reset to factory tabs
	ResetVertTabs();							// Reset to factory vert tabs
}

/*
=======================================================
Reset the tab stops to factory default.
=======================================================
*/
void VTFX80Print::ResetTabStops(void)
{
	int 		tab, c;

	// Set default tab stops
	tab = 0;
	for (c = 8; c < 80; c+= 8)
		m_tabs[tab++] = (int) ((double) c * m_horizDpi / 10.0);

	// Fill remaining tabs with zero
	while (tab < 32)
		m_tabs[tab++] = 0;
}

/*
=======================================================
Reset the vertical tab stops to factory default.
=======================================================
*/
void VTFX80Print::ResetVertTabs(void)
{
	int		c, channel;
	int		stop;

	// Restore to Tab Channel zero
	m_tabChannel = 0;
	m_escTabChannel = 0;

	// Loop for all 16 vertical tab stops
	for (c = 0; c < 16; c++)
	{
		stop = (int) (m_vertDpi * (double) ((c * 2) + 1) / 12.0);
		for (channel = 0; channel < 8; channel++)
			m_vertTabs[channel][c] = stop;
	}
}

/*
=======================================================
Reset the printer to default, power on settings.
=======================================================
*/
void VTFX80Print::ResetPrinter(void)
{
	int		c, r;
	int		fileLoaded;
	FILE*	fd;

	// Reset margins, cpi and pin posiitons
	m_width = FX80_PAGE_WIDTH;					// Default to 8" printable
	m_height = FX80_PAGE_HEIGHT;				// Default to 10.5" printable
	m_vertDpi = FX80_VERT_DPI;					// The number of "Dots" on the page
	m_horizDpi = FX80_HORIZ_DPI;				// The number of "Dots" on the page
	m_vertDots = (int) (m_vertDpi * m_height);	// Calculate number of vertical dots
	m_horizDots = (int) (m_horizDpi * m_width);	// Calculate number of horizontal dots
	m_bytesPerLine = (m_horizDots + 7 ) / 8;	

	// Reset the print and protocol mode
	ResetMode();

	m_curX = m_leftMarginX;
	m_curY = 0;

	// Load ROM from the specified ROM file
	fileLoaded = FALSE;
	if (m_useRomFile)
	{
		if (m_sRomFile.GetLength() != 0)
		{
			fd = fopen((const char *) m_sRomFile, "r");
			if (fd != NULL)
			{
				// Read data from File into charRam buffer
				fread(m_charRom, 1, sizeof(m_charRom), fd);
				fclose(fd);
				
				// Indicate ROM loaded
				fileLoaded = TRUE;
			}
		}
	}

	// Initialize ROM from internal storage if not loaded externally
	if (!fileLoaded)
	{
		for (c = 0; c < 256; c++)
			for (r = 0; r < 12; r++)
				m_charRom[c][r] = gFX80CharRom[c][r];
	}

	// Load RAM from the specified RAM file if required
	fileLoaded = FALSE;
	if (m_useRamFile)
	{
		if (m_sRamFile.GetLength() != 0)
		{
			fd = fopen((const char *) m_sRamFile, "r");
			if (fd != NULL)
			{
				// Read data from File into charRam buffer
				fread(m_charRam, 1, sizeof(m_charRam), fd);
				fclose(fd);
				
				// Indicate RAM loaded
				fileLoaded = TRUE;
			}
		}
	}

	// Clear the RAM (zero) if not loaded from a file above
	if (!fileLoaded)
	{
		for (c = 0; c < 256; c++)
		{
			// Loop for 12 columns per character
			for (r = 0; r < 12; r++)
				m_charRam[c][r] = 0;				// Set all pin data to zero
		}
	}

	// Ensure memory is allocated for the "page"
	if (m_pPage == NULL)
	{
		m_pPage = new unsigned char[m_vertDots * m_bytesPerLine];
	}

	// Zero the page memory
	for (c = 0; c < m_vertDots * m_bytesPerLine; c++)
		*(m_pPage + c) = 0;

}

/*
====================================================================
Identifies the first and last columns of the specified character 
data to print for proportional mode, if enabled
====================================================================
*/
void VTFX80Print::TrimForProportional(unsigned char *pData, int&
	first, int& last)
{
	int		c, b;

	// Check if proportional spacing is on.  If it is, trim all blank cols
	if (m_proportional)
	{
		// Check if proportional data embedded in attribute byte
		if (*pData & 0x7F)
		{
			last = *pData & 0x0F;			// Lower 4 bits are end col
			first = (*pData >> 4) & 0x07;	// Bits 4,5,6 are start col
			if (last > 10)
				last = 10;
			return;
		}

		// Skip attribute and move onto check data
		pData++;

		// Find first non-blank column
		for (c = 0; c < 11; c++)
			if (*(pData+c) != 0)
				break;
	
		// Check for a blank character (i.e. space) and print the whole thing
		if (c == 11)
		{
			// Print the whole character if it is blank
			first = 0; 
			last = 10;
			return;
		}
	
		// Now find last blank col
		for (b = 10; b >= 0; b--)
			if (*(pData + b) != 0)
				break;
		if (b != 10) 
			b++;

		// Now calculate number of bytes to render
		last = b;
		first = c;
	}
	else
	{
		// If not in proporitonal mode, print the whole character
		first = 0;
		last = 10;
	}
}

/*
====================================================================
Maps international characters to ROM entries
====================================================================
*/
unsigned char VTFX80Print::MapIntlChar(unsigned char byte)
{
	int		c;

	// Validate the map is valid
	if (m_intlSelect > 8)
		return byte;

	// Check if byte is an international code
	for (c = 0; c < 12; c++)
		if (byte == gIntlTable[0][c])
			break;

	// Check if found
	if (c == 12)
		return byte;

	// Lookup the code in the intl table
	return gIntlTable[m_intlSelect][c];
}

/*
====================================================================
Renders the specified character to the page.  This uses the current
X and Y location, CPI, font enhancements, RAM/ROM selection, etc.
====================================================================
*/
void VTFX80Print::RenderChar(unsigned char byte)
{
	unsigned char*	pData;
	int				topRow;
	int				c, first, last;
	int				mask, bits;
	int				xpos, ypos, i, dotsPerPin;
	int				xpos2;			// Used for locating enhanced dots
	double			scalar, expandScale;
	int				proportionAdjust;

	// Indicate marks made on the page
	m_marksMade = TRUE;

	// Perform interntional character mapping
	if (m_intlSelect != 0)
		byte = MapIntlChar(byte);

	// Check if printing in italic
	if (m_italic)
		byte |= 0x80;

	// Choose from ROM or RAM character data
	if (m_fontSource)
		pData = m_charRam[byte];
	else
		pData = m_charRom[byte];

	// Check if font uses upper 8 pins or lower 8 pins
	topRow = (*pData & 0x80) ? 0 : 2;

	// Trim the character for proportional mode printing
	TrimForProportional(pData, first, last);
	proportionAdjust = m_proportional ? 1 : 0;

	// Skip attribute byte
	pData++;

	// Calculate scalar for dot positioning base ond on CPI / DPI
	scalar = (double) m_horizDpi / ((double) m_cpi * 12.0);	// 11 "dots" per character + extra half dot

	// For expanded mode, we must expand the pins out twice as far
	dotsPerPin = (m_expanded || m_expandedOneLine) ? 2 : 1;
	
	// Loop through all bytes to be rendered 
	for (c = first; c <= last; c++)
	{
		// On the standard FX-80 Character ROM, over half the cols are blank, so
		// Test for a blank row to improve speed
		if (*(pData + c) == 0)
			continue;

		// Walk through each bit for the current byte
		mask = 0x80;
		for (bits = 0; bits < 8; bits++)
		{
			// If this pin does not "strike", move on to next pin
			if (!(*(pData +c) & mask))
			{
				mask >>= 1;
				continue;
			}

			// Okay, we have to render some "dots" on the page
			// Calculate the y location based on superscript, subscript, which pins to use
			ypos = m_curY + (m_superscript ? bits : m_subscript ? 9 + bits : topRow + bits * 2);

			// For expanded mode, we loop on each "pin" twice and create double dots
			for (i = 0; i < dotsPerPin; i++)
			{
				// Calculate the X location based on the current CPI, m_curX, m_dpi
				xpos = m_curX + (int) ((double) ((c - first)*dotsPerPin + i) * scalar);
				xpos2 = m_curX + (int) ((double) ((c - first+1)*dotsPerPin + i) * scalar);

				// Set this pixel on the page
				PlotPixel(xpos, ypos);

				// Test if enhance mode is on and set next pixel too
				if (m_enhanced)
//					PlotPixel(xpos+1, ypos);
					PlotPixel(xpos2, ypos);

				// Test if double-strike is on
				if (m_dblStrike && !(m_superscript || m_subscript))
				{
					PlotPixel(xpos, ypos+1);
					if (m_enhanced)
						PlotPixel(xpos2, ypos+1);
				}
			}
	
			// Shift mask
			mask >>= 1;
		}
	}

	// Draw underline, if needed
	if (m_underline)
	{
		// Plot a pixel in row 10 (actually 20 because of "half dots" for each col
		for (c = first; c <= last + proportionAdjust; c += 2)
		{
			for (i = 0; i < dotsPerPin; i++)
			{
				xpos = m_curX + (int) ((double) ((c - first)*dotsPerPin + i) * scalar);
				PlotPixel(xpos, m_curY + 18);
				if (m_enhanced)
					PlotPixel(xpos+1, m_curY + 18);
			}
		} 
	}

	// Update m_curX
	m_curX += (int) ((double) ((last - first + 1 + proportionAdjust) * dotsPerPin) * scalar);
	if (m_curX & 0x01)
		m_curX++;
}

/*
====================================================================
Plots the specified x,y pixel location and updated the page memory
as necessary.
====================================================================
*/
inline void VTFX80Print::PlotPixel(int xpos, int ypos)
{
	unsigned char 	byte;
	int				offset;

	// Test if printing "off the page"
	if ((xpos >= m_horizDots) || (ypos >= m_vertDots))
		return;

	offset = xpos / 8 + ypos * m_bytesPerLine;
	
	byte = 0x01 << (xpos & 0x07);
	*(m_pPage + offset) |= byte;
}

/*
====================================================================
Processes a byte as a command or ESC sequence.  Returns TRUE if
the command was processed as a command.
====================================================================
*/
int VTFX80Print::ProcessAsCmd(unsigned char byte)
{
	int		c;

	// Check if the last byte we "saw" was ESC
	if (m_escSeen)
	{
		// Clear the ESC seen flag
		m_escSeen = FALSE;

		// Last character was ESC.  Get the ESC command
		switch (byte)
		{
		case 0x02:
			return TRUE;

		case '@':			// Reset printer
			ResetMode();	// Reset mode settings
			return TRUE;

		case '<':			// Carriage return
			m_curX = m_leftMarginX;
			return TRUE;

		case 0x0e:			// ESC 0x0e same as just 0x0e
		case 0x0f:			// ESC 0x0f same as just 0x0f
			ProcessDirectControl(byte);
			return TRUE;

		case '0':			// Switch to 1/8" line spacing
			m_lineSpacing = 1.0 / 8.0;
			return TRUE;

		case '1':			// Switch to 7/72" line spacing
			m_lineSpacing = 7.0 / 72.0;
			return TRUE;

		case '2':			// Switch to 1/6" line spacing
			m_lineSpacing = 1.0 / 6.0;
			return TRUE;

		case '4':			// Turn Italic Mode on
			m_italic = TRUE;
			return TRUE;

		case '5':			// Turn Italic Mode off
			m_italic = FALSE;
			return TRUE;

		case '6':			// Enable printing of high ASCII codes
			m_highCodesPrinted = TRUE;
			return TRUE;

		case '7':			// Disable printing of high ASCII codes
			m_highCodesPrinted = FALSE;
			return TRUE;

		case 'E':			// Turn Emphasized on
			m_enhanced = TRUE;
			return TRUE;

		case 'F':			// Turn Emphasized off
			m_enhanced = FALSE;
			return TRUE;

		case 'G':			// Turn Double Strike on
			m_dblStrike = TRUE;
			return TRUE;

		case 'H':			// Turn Double Strike off
			m_dblStrike = FALSE;
			return TRUE;

		case 'M':			// Turn on Elite printing
			m_elite = TRUE;
			m_cpi = 12.0;	// Elite has highest priority
			return TRUE;

		case 'P':			// Turn off Elite printing
			m_elite = FALSE;
			if (m_compressed)		// Compressed take priority over Pica
				m_cpi = 17.125;
			else
				m_cpi = 10.0;
			return TRUE;

		case 'T':			// Superscript or Subscript mode off
			m_superscript = FALSE;
			m_subscript = FALSE;
			return TRUE;

		case '-':			// Turn Underline on or off
		case '/':			// Select tab channel
		case '!':			// Master select
		case ':':			// Download ROM to RAM
		case '%':			// Select ROM or RAM char set
		case '*':			// Graphics Variable density mode
		case '^':			// 9-Pin Graphics variable density mode
		case '?':			// Reassign graphics command
		case '&':			// Define User Character
		case '3':			// Set line spacing to n/216"
		case 'A':			// Set line spacing to n/72"
		case 'B':			// Set Vertical Tab Stops
		case 'C':			// Set Form Length
		case 'D':			// Set Tab Stops
		case 'I':			// Enable /Disable printing of low codes
		case 'J':			// Line Feed n/216", no CR
		case 'j':			// Reverse Line Feed n/216", no CR
		case 'K':			// Single density graphics mode
		case 'L':			// Double density graphics mode
		case 'Y':			// High speed Double density graphics mode
		case 'Z':			// Quad density graphics mode
		case 'N':			// Set skip perforation length
		case 'Q':			// Set right margin
		case 'R':			// International character set select
		case 'S':			// Superscript or Subscript mode on
		case 'U':			// Unidirectional mode on
		case 'W':			// Expanded mode on/off
		case 'b':			// Set Tab Channel vertical tab stops
		case 'l':			// Set left margin
		case 'p':			// Proportional mode on/off
		case 's':			// Print speed
		case 'i':			// Immediate print
			m_escCmd = byte;			// Indicate we are in an ESC sequence
			m_escParamsRcvd = 0;		// Need to wait for 1 parameter
			return TRUE;

		// ESC Codes that don't mean anything for the emulation, but need to be processed
		case '>':			// High-order-bit.  Old support for 7-bit systems
		case '8':			// Turn off paper sensor
		case '9':			// Turn on paper sensor
		case 'O':			// Turn off skip perforations
			return TRUE;

		default:			// Unknown ESC code - terminate ESC mode
			break;
		}
	}

	// Check if we are in the middle of an ESC sequence
	else if (m_escCmd)
	{
		switch (m_escCmd)
		{
		case '-':			// Turn underline on or off
			m_escCmd = 0;
			if (byte == '0')
				// Turning off underline mode
				m_underline = FALSE;
			else if (byte == '1')
				// Turning on underline mode
				m_underline = TRUE;
			return TRUE;

		case '/':			// Select tab channel
			// Validate tab channel is in range
			if (byte < 8)
				m_tabChannel = byte;
			m_escCmd = 0;
			return TRUE;
			
		case '&':			// Define User Character
			return DefineUserChar(byte);

		case '!':			// Master select
			return MasterSelect(byte);

		case '?':			// Graphics Mode command reassignment
			return ReassignGraphicsCmd(byte);

		case ':':			// Download ROM to RAM
			// On first byte, copy ROM to RAM
			if (m_escParamsRcvd  == 0)
			{
				for (c = 0; c < sizeof(m_charRam); c++)
					((char*)m_charRam)[c] = ((char*)m_charRom)[c];
				m_escParamsRcvd++;
			}
			else if (++m_escParamsRcvd == 3)
				m_escCmd = m_escParamsRcvd = 0;

			return TRUE;

		case '%':			// Select ROM or RAM char set
			// Check if first byte and save font source
			if (m_escParamsRcvd == 0)
			{
				m_fontSource = byte;
				m_escParamsRcvd++;
			}
			else
				m_escCmd = m_escParamsRcvd = 0;
			return TRUE;

		case '*':			// Graphics variable density mode
		case '^':			// 9-Pin Graphics variable density mode
			return VarGraphicsCommand(byte);

		case '3':			// Set line spacing to n/216"
			m_escCmd = 0;
			m_lineSpacing = (double) byte / 216.0;
			return TRUE;

		case 'A':			// Set line spacing to n/72"
			m_escCmd = 0;
			byte &= 0x7F;			// Mask upper bit
			if (byte > 85)			// 85/72" is max
				byte = 85;		
			m_lineSpacing = (double) byte / 72.0;
			return TRUE;

		case 'B':			// Set Vertical Tab Stops
			// Check if done setting tab stops
			if ((byte == 0) || ((m_escParamsRcvd > 0) && 
				(byte < m_vertTabs[0][m_escParamsRcvd-1])))
			{
				m_escCmd = m_escParamsRcvd = 0;
				return TRUE;
			}

			// Set next tab stop base on current cpi
			if (m_escParamsRcvd < 16)
				m_vertTabs[0][m_escParamsRcvd++] = (int) (m_vertDpi  * (double) byte * m_lineSpacing);
			if (m_escParamsRcvd == 16)
				m_escCmd = m_escParamsRcvd = 0;
			return TRUE;

		case 'C':			// Set form length
			// Check if using inches mode.  If inches, need to wait for next byte
			if (byte == 0)
				return TRUE;
			// Setting form length doesn't mean anything for emulation - do nothing
			m_escCmd = 0;
			return TRUE;

		case 'D':			// Set Tab Stops
			// Check if done setting tab stops
			c = (int) (m_horizDpi  * (double) byte / m_cpi) + m_leftMarginX;
			if ((byte == 0) || (m_escParamsRcvd && (c < m_tabs[m_escParamsRcvd-1])))
			{
				m_escCmd = 0;
				return TRUE;
			}
			// Set next tab stop base on current cpi
			if (m_escParamsRcvd < 32)
				m_tabs[m_escParamsRcvd++] = c;
			return TRUE;
			
		case 'I':			// Enable / Disable printing of low codes
			// Update low code printing flag
			if (byte == '0')
				m_lowCodesPrinted = FALSE;
			else if (byte == '1')
				m_lowCodesPrinted = TRUE;
			m_escCmd = 0;
			return TRUE;

		case 'J':			// Line feed n/216", no CR
			m_escCmd = 0;
			m_curY += (int) (((double) byte / 216.0) * m_vertDpi + 0.25);

			// Check for new page
			if (m_curY+9 > m_vertDots)
				NewPage();
			return TRUE;

		case 'j':			// Reverse line feed n/216", no CR
			m_escCmd = 0;
			m_curY -= (int) (((double) byte / 216.0) * m_vertDpi + 0.25);
			if (m_curY < 0)
				m_curY = 0;
			return TRUE;

		case 'K':			// Single density graphics mode
		case 'L':			// Low speed Double density graphics mode
		case 'Y':			// Double density graphics mode
		case 'Z':			// Quad density graphics mode
			return GraphicsCommand(byte);

		case 'N':			// Turn on Skip Perforation - No operation
			m_escCmd = 0;
			return TRUE;

		case 'Q':			// Set right margin
			m_escCmd = 0;
			SetRightMargin(byte);
			return TRUE;

		case 'R':			// Superscript or Subscript mode on
			m_escCmd = 0;

			// Save the interntional character mode
			m_intlSelect = byte;
			if (m_intlSelect > 8)
				m_intlSelect = 0;
			return TRUE;

		case 'S':			// Superscript or Subscript mode on
			m_escCmd = 0;
			if (byte == '0')
			{
				// Turning on superscript mode
				m_superscript = TRUE;
				m_subscript = FALSE;
			}
			else if (byte == '1')
			{
				// Turning on subscript mode
				m_superscript = FALSE;
				m_subscript = TRUE;
			}
			return TRUE;
		case 'U':			// Unidiretional mode
		case 's':			// Print speed
		case 'i':			// Immediate print
			m_escCmd = 0;
			return TRUE;

		case 'W':			// Expanded mode on/off
			m_escCmd = 0;

			// Check if turning expanded mode on
			if (byte == '1')
				m_expanded = TRUE;
			else if (byte == '0')
				m_expanded = FALSE;
			return TRUE;

		case 'b':			// Set Vertical Channel Tab Stops
			return SetVertChannelTabs(byte);

		case 'l':			// Set left margin
			m_escCmd = 0;
			SetLeftMargin(byte);
			return TRUE;

		case 'p':			// Proportional mode on/off
			m_escCmd = 0;

			// Check if turning proportional mode on
			if (byte == '1')
				m_proportional = TRUE;
			else if (byte == '0')
				m_proportional = FALSE;
			return TRUE;

		default:			// Unknown ESCCmd mode
			m_escCmd = 0;
			break;			// Terminate ESCCmd mode
		}
	}

	// Test if it is a direct control byte
	else if ((byte <= ' ') || ((byte > '~') && (byte <= 159)))
		return ProcessDirectControl(byte);

	return FALSE;
}

/*
====================================================================
Process a direct control (non ESC sequence) byte
====================================================================
*/
int VTFX80Print::ProcessDirectControl(unsigned char byte)
{
	int		c;

	// Test if it is a high code and high codes are printed
	if ((byte > 'z') && m_highCodesPrinted)
		return FALSE;

	// Test for printing of low codes
	if (m_lowCodesPrinted && (byte < ' '))
	{
		// For codes that are printable, return FALSE
		if ((byte < 7) || (byte == 16) || 
			((byte > 20) && (byte != 24) && (byte != 27)))
				return FALSE;
	}

	switch (byte & 0x7F)
	{
	case 0x07:					// Beep
		return TRUE;

	case 0x08:					// Backspace character
		m_curX -= (int) (m_horizDpi / m_cpi * ((m_expanded || m_expandedOneLine) ? 24.0 : 12.0));
		if (m_curX < m_leftMarginX)
			m_curX = m_leftMarginX;
		return TRUE;

	case 0x09:					// TAB character
		// Loop through tabs and find nex tab stop
		for (c = 0; c < 32; c++)
		{
			// Find next tab location greater than the current position
			if (m_curX < m_tabs[c])
			{
				m_curX = m_tabs[c];
				break;
			}
		}
		return TRUE;

	case ASCII_CR:				// CR character
		// Turn off Expanded One Line
		m_expandedOneLine = FALSE;
		m_curX = m_leftMarginX;
		return TRUE;

	case 0x0b:					// Vertical tab
		m_expandedOneLine = FALSE;

		// Loop through tabs and find nex tab stop
		for (c = 0; c < 16; c++)
		{
			// Find next tab location greater than the current position
			if (m_curY < m_vertTabs[m_tabChannel][c])
			{
				m_curY = m_vertTabs[m_tabChannel][c];
				break;
			}
		}

		return TRUE;

	case ASCII_LF:				// LF Character
		// Turn off Expanded One Line
		m_expandedOneLine = FALSE;

		// Update X and Y locations
		m_curX = m_leftMarginX;
		m_curY += (int) (m_lineSpacing * m_vertDpi + 0.25);

		// Check for new page
		if (m_curY+9 > m_vertDots)
			NewPage();
		return TRUE;

	case ASCII_FF:				// FF character
		// Turn off Expanded One Line
		NewPage();
		m_curX = m_leftMarginX;
		m_curY = 0;
		m_expandedOneLine = FALSE;
		return TRUE;

	case 0x0e:					// Expanded One Line mode
		m_expandedOneLine = TRUE;
		return TRUE;

	case 0x0f:					// Enable COMPRESSED print mode
		m_compressed = TRUE;
		if (!m_elite)			// Elite has priority
			m_cpi = 17.125;		// Update print density
		return TRUE;

	case 0x12:					// Disable COMPRESSED print mode
		m_compressed = FALSE;
		if (m_elite)			// Update print density
			m_cpi = 12.0;
		else
			m_cpi = 10.0;
		return TRUE;

	case 0x14:					// Cancel double-width one line
		m_expandedOneLine = FALSE;
		return TRUE;

	case 0x1B:					// ESC character
		m_escSeen = TRUE;		// Indicate we started ESC sequence
		return TRUE;

	// Unused codes, but still valid codes
	case 0x11:					// Select printer
	case 0x13:					// Deselect printer
	case 0x18:					// Cancel line
		return TRUE;
	}

	return FALSE;
}

/*
====================================================================
Create a new page, processing the existing page as needed.
====================================================================
*/
void VTFX80Print::NewPage(void)
{
	int c;

	// Check if already on a new page
	if ((m_curX == m_leftMarginX) && (m_curY == 0))
	{
		// Zero out the page memory
		for (c = 0; c < m_vertDots * m_bytesPerLine; c++)
			*(m_pPage + c) = 0;
		return;
	}

	// Check if page is empty and we are ignoring empty pages
	for (c = 0; c < m_vertDots * m_bytesPerLine; c++)
		if (*(m_pPage + c) != 0)
		{

			// Create paper if we don't already have it
			if (m_pPaper == NULL)
				CreatePaper();

			// Print page to the paper and create a new page
			if (m_pPaper != NULL)
			{
				m_pPaper->PrintPage(m_pPage, m_horizDots, m_vertDots);
				m_pPaper->NewPage();
			}

			// Only need to print once
			break;
		}

	// Zero out the page memory
	for (c = 0; c < m_vertDots * m_bytesPerLine; c++)
		*(m_pPage + c) = 0;

	m_curY = 0;
}

/*
====================================================================
Sets the left margin based on current print style.
====================================================================
*/
void VTFX80Print::SetLeftMargin(unsigned char byte)
{
	int		newLeftMargin;

	// Calculate new left margin based on current cpi
	newLeftMargin = (int) ((double) byte * m_cpi + 0.25);

	// Validate the left margin value
	if (m_elite)
	{
		if (byte <= 93)
			m_leftMarginX = newLeftMargin;
	}
	else if (m_compressed)
	{
		if (byte <= 133)
			m_leftMarginX = newLeftMargin;
	}
	else
	{
		if (byte <= 78)
			m_leftMarginX = newLeftMargin;
	}

	if (m_curX == 0)
		m_curX = m_leftMarginX;

	ResetTabStops();							// Reset to factory tabs
}

/*
====================================================================
Sets the right margin based on current print style.
====================================================================
*/
void VTFX80Print::SetRightMargin(unsigned char byte)
{
	int		newRightMargin;

	// Calculate new left margin based on current cpi
	newRightMargin = (int) ((double) byte * m_cpi + 0.25);

	// Validate the left margin value
	if (m_elite)
	{
		if ((byte <= 96) && (byte >= 3))
			m_rightMarginX = newRightMargin;
	}
	else if (m_compressed)
	{
		if ((byte <= 137) && (byte >= 4))
			m_rightMarginX = newRightMargin;
	}
	else
	{
		if ((byte <= 80) && (byte >= 2))
			m_rightMarginX = newRightMargin;
	}

	ResetTabStops();							// Reset to factory tabs
}

/*
====================================================================
Set vertical channel tab stops.
====================================================================
*/
int VTFX80Print::SetVertChannelTabs(unsigned char byte)
{
	// Check if done setting tab stops
	if (m_escParamsRcvd == 0)
	{
		// Save the tab channel number
		m_escTabChannel = byte;
		m_escParamsRcvd++;
		return TRUE;
	}
	else if ((byte == 0) || ((m_escParamsRcvd > 1) && 
		(byte < m_vertTabs[m_escTabChannel][m_escParamsRcvd-2])))
	{
		m_escCmd = m_escParamsRcvd = 0;
		return TRUE;
	}

	// Set next tab stop base on current cpi
	if (m_escParamsRcvd < 17)
	{
		if (m_escTabChannel < 8)
		{
			m_vertTabs[m_escTabChannel][m_escParamsRcvd++] = 
				(int) (m_vertDpi  * (double) byte * m_lineSpacing);
		}
		else
			m_escParamsRcvd++;
	}
	return TRUE;
}

/*
====================================================================
Sets up character font / style based on master code
====================================================================
*/
int VTFX80Print::MasterSelect(unsigned char byte)
{
	m_escCmd = 0;
	m_dblStrike = FALSE;
	m_compressed = FALSE;
	m_enhanced = FALSE;
	m_elite = FALSE;
	m_expanded = FALSE;

	if (byte & 0x20)		// Check if expanded selected
		m_expanded = TRUE;
	if (byte & 0x10) 		// Check if Double Strike selected
		m_dblStrike = TRUE;
	if (byte & 0x08)		// Check if Emphasized selected
		m_enhanced = TRUE;
	if (byte & 0x04)		// Check if compressed selected
		m_compressed = TRUE;
	if (byte & 0x01)		// Check if Elite selected
		m_elite = TRUE;

	if (m_elite)
		m_cpi = 12.0;		// Elite takex presidence
	else if (m_compressed)
		m_cpi = 17.125;		// Compressed takes next precidence
	else
		m_cpi = 10.0;		// Must be Pic

	return TRUE;
}

/*
====================================================================
Processes the escape sequence for defining user characters
====================================================================
*/
int VTFX80Print::DefineUserChar(unsigned char byte)
{
	int		index;

	// Get the character set. Zero is all that is allowed
	if (m_escParamsRcvd == 0)
	{
		m_userCharSet = byte;
		m_escParamsRcvd++;
	}

	// Check if first character selection
	else if (m_escParamsRcvd == 1)
	{
		m_userFirstChar = byte;
		m_userUpdateChar = byte;
		m_escParamsRcvd++;
	}

	// Check if last character selection
	else if (m_escParamsRcvd == 2)
	{
		m_userLastChar = byte;
		m_escParamsRcvd++;
	}

	// Must be character data
	else
	{
		index = (m_escParamsRcvd - 3) % 12;
		m_charRam[m_userUpdateChar][index] = byte;

		// Check if we need to move to the next character
		if (index == 11)
		{
			// Move to next character to be defined
			m_userUpdateChar++;
			if (m_userUpdateChar > m_userLastChar)
			{
				// All data received, stop esc mode
				m_escCmd = 0;
				m_escParamsRcvd = 0;
				return TRUE;
			}
		}
		
		// Increment parameter received count
		m_escParamsRcvd++;
	}

	return TRUE;
}

/*
====================================================================
Processes the escape sequence for setting a graphics mode (K,L,Y,Z)
====================================================================
*/
int VTFX80Print::GraphicsCommand(unsigned char byte)
{
	// Save the graphics width
	if (m_escParamsRcvd == 0)
	{
		// Save LSB of length
		m_graphicsLength = byte;
		m_escParamsRcvd++;
	}
	else if (m_escParamsRcvd == 1)
	{
		// Update length byte
		m_graphicsLength  |= ((int) byte % 8) << 8;
		m_escParamsRcvd = 0;

		// Setup graphics mode for 8PIN
		m_graphicsMode = FX80_8PIN_MODE;
		m_graphicsRcvd = 0;

		// Set resolution based on current mode assignment
		if (m_escCmd == 'K')
			m_graphicsDpi = gGraphicsDpis[m_escKmode];
		else if (m_escCmd == 'L')
			m_graphicsDpi = gGraphicsDpis[m_escLmode];
		else if (m_escCmd == 'Y')
			m_graphicsDpi = gGraphicsDpis[m_escYmode];
		else if (m_escCmd == 'Z')
			m_graphicsDpi = gGraphicsDpis[m_escZmode];
		m_graphicsStartX = m_curX;			// Save starrting X
		m_escCmd = 0;
	}
	return TRUE;
}

/*
====================================================================
Processes the escape sequence for variable density graphics modes.
====================================================================
*/
int VTFX80Print::VarGraphicsCommand(unsigned char byte)
{
	// Save density and graphics length
	if (m_escParamsRcvd == 0)
	{
		// Validate mode byte
		if (byte < 7)
			m_graphicsDpi = gGraphicsDpis[byte];
		m_escParamsRcvd++;
	}
	// Save low byte of length
	else if (m_escParamsRcvd == 1)
	{
		m_graphicsLength = byte;
		m_escParamsRcvd++;
	}
	else
	{
		// Save high byte of length & enable graphics mode
		m_graphicsLength |= ((int) byte % 8) << 8;
		if (m_escCmd == '*')
			m_graphicsMode = FX80_8PIN_MODE;
		else
			m_graphicsMode = FX80_9PIN_MODE;
		m_graphicsStartX = m_curX;			// Save starrting X
		m_graphicsRcvd = 0;
		m_escCmd = 0; 
		m_escParamsRcvd = 0;
	}
	return TRUE;
}

/*
====================================================================
Reassigns a graphics mode command (K, L, Y, Z).
====================================================================
*/
int VTFX80Print::ReassignGraphicsCmd(unsigned char byte)
{
	// Get code to be reassigned and the new mode
	if (m_escParamsRcvd == 0)
	{
		m_graphicsReassign = byte;
		m_escParamsRcvd++;
	}
	else
	{
		// Validate mode range
		if (byte < 7)
		{
			if (m_graphicsReassign == 'K')
				m_escKmode = byte;
			else if (m_graphicsReassign == 'L')
				m_escLmode = byte;
			else if (m_graphicsReassign == 'Y')
				m_escYmode = byte;
			else if (m_graphicsReassign == 'Z')
				m_escZmode = byte;
		}
		m_escParamsRcvd = 0;
		m_escCmd = 0;
	}
	return TRUE;
}

/*
=======================================================
Renders byte as a graphics mode bit pattern.
=======================================================
*/
void VTFX80Print::RenderGraphic(unsigned char byte)
{
	int		advance;
	int		colsRcvd;
	int		c, mask;

	// Indicated marks made on the page
	m_marksMade = TRUE;

	// Plot dots at current x,y location.  Test if full byte plotted
	if ((m_graphicsMode == FX80_8PIN_MODE) || !(m_graphicsRcvd & 1))
	{
		// Plot all pins as indicated by the data
		mask = 0x80;
		for (c = 0; c < 8; c++)
		{
			if (byte & mask)
				PlotPixel(m_curX, m_curY + c * 2);		// Times 2 for 144 DPI

			// Update mask
			mask >>= 1;
		}

		// If 8PIN mode, advance the x position
		advance = m_graphicsMode == FX80_8PIN_MODE ? 1 : 0;
	}
	else
	{
		// Must be 2nd byte of 9PIN mode.  Plot only upper bit
		if (byte & 0x80)
		{
			// Bit is set...fire the "bottom pin"
			// NOTE: this really should use m_vertDpi to calc the location
			PlotPixel(m_curX, m_curY + 16);		// 16 is pin 9 (value 8) times 2
		}
		
		// Advance the x position
		advance = 1;
	}

	// Increment x position based on 8PIN or 9PIN mode
	colsRcvd = ++m_graphicsRcvd >> (m_graphicsMode-1); 
	if (advance)
		m_curX = m_graphicsStartX + (int) ((double) colsRcvd * 
			(m_horizDpi / m_graphicsDpi) + 0.5);

	// Check if we received all bytes
	if (colsRcvd >= m_graphicsLength)
		m_graphicsMode = FALSE;
}

