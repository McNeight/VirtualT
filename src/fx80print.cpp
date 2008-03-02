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
#include "chargen.h"

extern unsigned char gFX80CharRom[256][12];

unsigned char	gIntlTable[9][12] = {
	{ 35, 36, 64, 91, 92, 93, 94, 96, 123, 124, 125, 126 },			// USA
	{ 35, 36, 0,  5,  15, 16, 94, 96, 30,  2,   1,   22  },			// FRANCE
	{ 35, 36, 16, 23, 24, 25, 94, 96, 26,  27,  28,  17  },			// GERMANY
	{ 6,  36, 64, 91, 92, 93, 94, 96, 123, 124, 125, 126 },			// U.K.   
	{ 35, 36, 64, 18, 20, 13, 94, 96, 19,  21,  14,  126 },			// DENMARK
	{ 35, 11, 29, 23, 24, 13, 25, 30, 26,  27,  14,  28  },			// SWEDEN
	{ 35, 36, 64, 5,  92, 30, 94, 30, 0,   3,   1,   4   },			// ITALY
	{ 12, 36, 64, 7,  9,  8,  94, 96, 22,  10,  125, 126 },			// SPAIN
	{ 35, 36, 64, 91, 31, 93, 94, 96, 123, 124, 125, 126 }			// JAPAN
};   

/*
================================================================================
VTFX80Print:	This is the class construcor for the FX80Print Device emulation.
================================================================================
*/
VTFX80Print::VTFX80Print(void)
{
	// Initialize pointers
	m_pPage = NULL;
	m_pRomFile = NULL;
	m_pRamFile = NULL;
	m_pUseRomFile = NULL;
	m_pUseRamFile = NULL;

	m_useRamFile = FALSE;
	m_pVirtualPaper = FALSE;
	m_virtualPaper = FALSE;
	m_pOut = NULL;

	ResetPrinter();
}

/*
================================================================================
~VTFX80Print:	This is the class construcor for the FX80Print Device emulation.
================================================================================
*/
VTFX80Print::~VTFX80Print(void)
{
	// Delete the page memory
	if (m_pPage != NULL)
		delete m_pPage;

	m_pPage = NULL;
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

void cb_VirtualPaper(Fl_Widget *w, void * ptr)
{
	if (ptr != NULL)
	{
		((VTFX80Print*) ptr)->m_pOut->hide();
		delete ((VTFX80Print*) ptr)->m_pOut;
		((VTFX80Print*) ptr)->m_pOut = NULL;
	}
}

/*
=======================================================
Print a byte to the FX80 Emulation
=======================================================
*/
void VTFX80Print::PrintByte(unsigned char byte)
{
	if (m_pPage == NULL)
		return;

	if ((m_pOut == NULL) && m_virtualPaper)
	{
		m_pOut = new Fl_Window(1024, 500, "Printer Output");
		m_pOut->callback(cb_VirtualPaper, this);
		m_pOut->show();
	}

	// Check if in Graphics Mode
	if (m_graphicsMode)
	{
		// Process byte as graphics data

		return;
	}

	// Check if it is a control code sequence
	if (ProcessAsCmd(byte))
		return;

	// Okay, it is a byte that needs to be printed
	if (byte == ASCII_LF)
		m_curY += (int) (m_lineSpacing * m_vertDpi);
	else if (byte == ASCII_CR)
		m_curX = 0;
	else if (byte == ASCII_FF)
	{
		m_curX = 0;
		m_curY = 0;
		m_pOut->redraw();
	}
	else
	{
		// Render the specified character
		RenderChar(byte);
	}
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

	m_pVirtualPaper = new Fl_Check_Button(20, 200, 230, 20, "Print to Virtual Paper");
	m_pVirtualPaper->value(m_virtualPaper);

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
	m_pPref->get("FX80Print_VirtualPaper", m_virtualPaper, 0);

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
	if (m_pUseRomFile == NULL)
		return PRINT_ERROR_NONE;

	// Get values from the dialog controls
	m_useRomFile = m_pUseRomFile->value();
	m_useRamFile = m_pUseRamFile->value();
	if (m_pRomFile->value() != 0)
		m_sRomFile = m_pRomFile->value();

	if (m_pRamFile->value() != 0)
		m_sRamFile = m_pRamFile->value();

	m_virtualPaper = m_pVirtualPaper->value();

	// Now save values to the preferences
	m_pPref->set("FX80Print_UseRomFile", m_useRomFile);
	m_pPref->set("FX80Print_RomFile", (const char *) m_sRomFile);
	m_pPref->set("FX80Print_UseRamFile", m_useRamFile);
	m_pPref->set("FX80Print_RamFile", (const char *) m_sRamFile);
	m_pPref->set("FX80Print_VirtualPaper", m_virtualPaper);

	ResetPrinter();

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

/*
=======================================================
Reset the printer mode to default, power on settings.
=======================================================
*/
void VTFX80Print::ResetMode(void)
{
	// Reset protocol settings
	m_escSeen = m_escCmd = 0;					// No active ESC commands
	m_escParamsNeeded = 0;						// No ESC parameters needed
	m_cpi = 10.0;								// Default to Pica (10 cpi) font
	m_elite = FALSE;							// Turn off elite print mode
	m_compressed = FALSE;						// Turn off compressed print mode
	m_lineSpacing = 1.0 / 6.0;					// Default to 1/6" line spacing
	m_fontSource = 0;							// Default to ROM font
	m_proportional = FALSE;
	m_enhanced = FALSE;
	m_dblStrike = FALSE;
	m_expanded = FALSE;
	m_italic = FALSE;
	m_underline = FALSE;
	m_superscript = m_subscript = FALSE;
	m_intlSelect = 0;							// Select USA char set
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
	m_curX = m_curY = 0;
	m_topMargin = m_leftMargin = 0.5;			// Default to .5" margin
	m_width = FX80_PAGE_WIDTH;					// Default to 8" printable
	m_height = FX80_PAGE_HEIGHT;				// Default to 10.5" printable
	m_vertDpi = FX80_VERT_DPI;					// The number of "Dots" on the page
	m_horizDpi = FX80_HORIZ_DPI;				// The number of "Dots" on the page
	m_vertDots = (int) (m_vertDpi * m_height);	// Calculate number of vertical dots
	m_horizDots = (int) (m_horizDpi * m_width);	// Calculate number of horizontal dots
	m_bytesPerLine = (m_horizDots + 7 ) / 8;	

	// Reset the print and protocol mode
	ResetMode();

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
	double			scalar, expandScale;
	int				proportionAdjust;

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
	topRow = *pData++ ? 2 : 0;

	// Trim the character for proportional mode printing
	TrimForProportional(pData, first, last);
	proportionAdjust = m_proportional ? 1 : 0;

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

				// Set this pixel on the page
				PlotPixel(xpos, ypos);

				// Test if enhance mode is on and set next pixel too
				if (m_enhanced)
					PlotPixel(xpos+1, ypos);

				// Test if double-strike is on
				if (m_dblStrike && !(m_superscript || m_subscript))
				{
					PlotPixel(xpos, ypos+1);
					if (m_enhanced)
						PlotPixel(xpos+1, ypos+1);
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
	
	m_pOut->make_current();
	fl_color(FL_BLACK);
	fl_point(xpos, ypos);
}

/*
====================================================================
Processes a byte as a command or ESC sequence.  Returns TRUE if
the command was processed as a command.
====================================================================
*/
int VTFX80Print::ProcessAsCmd(unsigned char byte)
{
	// Check if the last byte we "saw" was ESC
	if (m_escSeen)
	{
		m_escSeen = FALSE;
		m_escCmd = byte;

		// Last character was ESC.  Get the ESC command
		switch (byte)
		{
		case 0x02:
			m_escCmd = 0;
			return TRUE;

		case '@':			// Reset printer
			ResetMode();	// Reset mode settings
			return TRUE;

		case '<':			// Carriage return
			m_curX = 0;
			m_escCmd = 0;
			return TRUE;

		case 0x0e:			// ESC 0x0e same as just 0x0e
		case 0x0f:			// ESC 0x0f same as just 0x0f
			ProcessDirectControl(byte);
			m_escCmd = 0;
			return TRUE;

		case '4':			// Turn Italic Mode on
			m_italic = TRUE;
			m_escCmd = 0;
			return TRUE;

		case '5':			// Turn Italic Mode off
			m_italic = FALSE;
			m_escCmd = 0;
			return TRUE;

		case 'E':			// Turn Emphasized on
			m_enhanced = TRUE;
			m_escCmd = 0;
			return TRUE;

		case 'F':			// Turn Emphasized off
			m_enhanced = FALSE;
			m_escCmd = 0;
			return TRUE;

		case 'G':			// Turn Double Strike on
			m_dblStrike = TRUE;
			m_escCmd = 0;
			return TRUE;

		case 'H':			// Turn Double Strike off
			m_dblStrike = FALSE;
			m_escCmd = 0;
			return TRUE;

		case 'M':			// Turn on Elite printing
			m_elite = TRUE;
			m_cpi = 12.0;	// Elite has highest priority
			m_escCmd = 0;
			return TRUE;

		case 'P':			// Turn off Elite printing
			m_elite = FALSE;
			if (m_compressed)		// Compressed take priority over Pica
				m_cpi = 17.125;
			else
				m_cpi = 10.0;
			m_escCmd = 0;
			return TRUE;

		case 'T':			// Superscript or Subscript mode off
			m_superscript = FALSE;
			m_subscript = FALSE;
			m_escCmd = 0;
			return TRUE;

		case '-':			// Turn Underline on or off
		case '!':			// Master select
		case 'R':			// International character set select
		case 'S':			// Superscript or Subscript mode on
		case 'U':			// Unidirectional mode on
		case 'W':			// Expanded mode on/off
		case 'p':			// Proportional mode on/off
		case 's':			// Print speed
		case 'i':			// Immedate print
			m_escParamsNeeded = 1;		// Need to wait for 1 parameter
			return TRUE;
		}
	}

	// Check if we are in the middle of an ESC sequence
	else if (m_escCmd)
	{
		switch (m_escCmd)
		{
		case '-':			// Turn underline on or off
			m_escCmd = m_escParamsNeeded = 0;
			if (byte == '0')
				// Turning off underline mode
				m_underline = FALSE;
			else if (byte == '1')
				// Turning on underline mode
				m_underline = TRUE;
			return TRUE;

		case '!':			// Master select
			m_escCmd = m_escParamsNeeded = 0;
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

		case 'R':			// Superscript or Subscript mode on
			m_escCmd = m_escParamsNeeded = 0;

			// Save the interntional character mode
			m_intlSelect = byte;
			if (m_intlSelect > 8)
				m_intlSelect = 0;
			return TRUE;

		case 'S':			// Superscript or Subscript mode on
			m_escCmd = m_escParamsNeeded = 0;
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
			m_escCmd = m_escParamsNeeded = 0;
			return TRUE;

		case 'W':			// Expanded mode on/off
			m_escCmd = m_escParamsNeeded = 0;

			// Check if turning expanded mode on
			if (byte == '1')
				m_expanded = TRUE;
			else if (byte == '0')
				m_expanded = FALSE;
			return TRUE;

		case 'p':			// Proportional mode on/off
			m_escCmd = m_escParamsNeeded = 0;

			// Check if turning proportional mode on
			if (byte == '1')
				m_proportional = TRUE;
			else if (byte == '0')
				m_proportional = FALSE;
			return TRUE;
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
	switch (byte)
	{
	case 0x07:					// Beep
		return TRUE;

	case 0x08:					// Backspace character
		m_curX -= (int) (m_horizDpi / m_cpi * ((m_expanded || m_expandedOneLine) ? 24.0 : 12.0));
		if (m_curX < 0)
			m_curX = 0;
		return TRUE;

	case 0x09:					// TAB character
		// Add tab processing
		return TRUE;

	case ASCII_CR:				// CR character
		// Turn off Expanded One Line
		m_expandedOneLine = FALSE;
		m_curX = 0;
		return TRUE;

	case 0x0b:					// Vertical tab
		m_expandedOneLine = FALSE;
		// Add vertical tab processing

		return TRUE;

	case ASCII_LF:				// LF Character
		// Turn off Expanded One Line
		m_expandedOneLine = FALSE;

		// Update X and Y locations
		m_curX = 0;
		m_curY += (int) (m_lineSpacing * m_vertDpi);

		// Check for new page
		if (m_curY > m_vertDots)
			NewPage();
		return TRUE;

	case ASCII_FF:				// FF character
		// Turn off Expanded One Line
		m_curX = 0;
		m_curY = 0;
		m_expandedOneLine = FALSE;
		NewPage();
		if (m_pOut != NULL)
			m_pOut->redraw();
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
}

