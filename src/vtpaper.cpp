/* vtpaper.cpp */

/* $Id: vtpaper.cpp,v 1.1 2008/03/05 13:25:27 kpettit1 Exp $ */

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
#include <FL/Fl_Scrollbar.H>
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
#include "vtpaper.h"

void cb_VTPaperScroll(Fl_Widget *w, void * ptr)
{
	if (ptr != NULL)
		((VTVirtualPaper*) ptr)->Scroll();
}

void cb_VirtualPaperWin(Fl_Widget *w, void * ptr)
{
	if (ptr != NULL)
	{
		((VTVirtualPaper*) ptr)->hide();
	}
}

/*
================================================================================
VTVirtualPaper:	This is the class construcor for the VTVirtualPaper class.
================================================================================
*/
VTVirtualPaper::VTVirtualPaper(Fl_Preferences* pPref) :
	Fl_Double_Window(985, 500, "Virtual Paper Output"), VTPaper(pPref)
{
	// Initialize pointers
	m_pLineCnts = NULL;				// Line count storage not allocated
	m_bytes = 0;
	m_height = 0;
	m_width = 0;
	m_bytesPerLine = 0;
	m_pageNum = 0;					// Clear active page number
	color(FL_WHITE);
	callback(cb_VirtualPaperWin, this);

	// Create a scrollbar
	m_pScroll = new Fl_Scrollbar(970, 0, 15, 500, "");
	m_pScroll->type(FL_VERTICAL);
	//m_pScroll->maximum(200);
	//m_pScroll->slider_size(10);
	m_pScroll->linesize(12);
	m_pScroll->callback(cb_VTPaperScroll, this);
	m_topPixel = 0;
}

/*
================================================================================
~VTFX80Print:	This is the class construcor for the FX80Print Device emulation.
================================================================================
*/
VTVirtualPaper::~VTVirtualPaper(void)
{
	// Delete the page memory
	DeletePageMemory();

	// Delete line counts
	if (m_pLineCnts != NULL)
		delete m_pLineCnts;

	m_pLineCnts = NULL;
}

/*
=======================================================
GetName:	Returns the name of the printer.
=======================================================
*/
MString VTVirtualPaper::GetName()
{
	return "Virtual Paper";
}

/*
=======================================================
Initialize the page with preferences.
=======================================================
*/
void VTVirtualPaper::Init()
{
}

/*
=======================================================
Build the page properties dialog controls.
=======================================================
*/
void VTVirtualPaper::BuildControls()
{
}

/*
=======================================================
Hide the controls on the property dialog.  This is 
called when the paper is not seleted to make room for
other "papers".
=======================================================
*/
void VTVirtualPaper::HideControls()
{
}

/*
=======================================================
Show the controls on the property dialog.  This is 
called when the paper is seleted to show the specific
controls for this paper.
=======================================================
*/
void VTVirtualPaper::ShowControls()
{
}

/*
=======================================================
Draws the the speciied page number
=======================================================
*/
void VTVirtualPaper::DrawPage(int pageNum)
{
	int				r, c, b;
	int				data, mask;
	int				bytes;
	unsigned char	*pPage, *pPtr;
	int				page, count;
	int				pageOffset;
	int				xpos, ypos;

	// Print to the paper
	count = m_pPages.GetSize();
	if (pageNum >= count)
		return;

	pageOffset = pageNum * (m_height + 80)+30;

	pPage = (unsigned char *) m_pPages[pageNum];
	if (pPage != NULL)
	{
		// Use temporary pointer
		pPtr = pPage;

		// Select pen color
		fl_color(FL_BLACK);

		r = *pPtr++;
		r |= (short) (*pPtr++) << 8;
		// Loop for all lines
		while (r != 65535)
		{
			bytes = *pPtr++;			// Get number of bytes
			ypos = pageOffset + r - m_topPixel;

			for (c = 0; c < bytes; c++)
			{
				// Get next byte to draw
				data = *pPtr++;
				mask = 0x01;			// Start with LSB
				for (b = 0; b < 8; b++)
				{
					if (data & mask)
					{
						xpos = (c<<3) + b;
						fl_point(xpos, ypos);
						fl_point(xpos+1, ypos);
						fl_point(xpos, ypos+1);
						fl_point(xpos+1, ypos+1);
					}
					mask <<= 1;
				}
			}

			// Get next row number
			r = *pPtr++;
			r |= ((short) *pPtr++) << 8;
		}
	}
}

/*
=======================================================
Draws the window using paper output data.
=======================================================
*/
void VTVirtualPaper::draw(void)
{
	int		pages, page;

	// Draw child objects, etc
	Fl_Double_Window::draw();

	// Check which page we need to draw
	page = m_topPixel / (m_height + 80);
	DrawPage(page);

	// Check if page separator needs to be drawn
	if ((page+1) * (m_height+80)-20 < m_topPixel + h())
	{
		fl_color(FL_GRAY);
		fl_rectf(0, (page+1) * (m_height+80)-20 - m_topPixel,
			w()-15, 20);
	}

	// Check if next page needs to be drawn
	if ((page+1) * (m_height+80) < m_topPixel + h())
		DrawPage(page+1);

	return;
}

/*
=======================================================
Get Page Preferences from dialog and save.
=======================================================
*/
void VTVirtualPaper::GetPrefs(void)
{
/*	if (m_pUseRomFile == NULL)
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
	m_pPref->set("VirtualPaper_UseRomFile", m_useRomFile);
*/
}

/*
=======================================================
Cancels the current print job.
=======================================================
*/
int VTVirtualPaper::CancelJob(void)
{
	// Delete all the page memory
	DeletePageMemory();

	return PRINT_ERROR_NONE;
}

/*
=======================================================
Prints the current print job.
=======================================================
*/
int VTVirtualPaper::Print(void)
{
	this->show();
	redraw();
	return PRINT_ERROR_NONE;
}

/*
=======================================================
Creates a new page for printing.
=======================================================
*/
int VTVirtualPaper::NewPage(void)
{
	m_pageNum++;				// Increment page num
	return PRINT_ERROR_NONE;
}

/*
=======================================================
Loads a new sheet of paper.  Basicaly starts a new
print job from scratch and dumps all old pages.
=======================================================
*/
int VTVirtualPaper::LoadPaper(void)
{
	int c;

	// Delete all the page memory
	DeletePageMemory();

	// Clear page number
	m_pageNum = 0;

	// Force a redraw
	redraw();

	return PRINT_ERROR_NONE;
}

/*
=======================================================
Prints data to the current page.
=======================================================
*/
int VTVirtualPaper::PrintPage(unsigned char *pData, int w, int h)
{
	int					totalBytes;
	int					lineOff;
	int					r, c, cnt, b;
	unsigned short		src, srcMask,psMask;
	unsigned char		mask, data;
	unsigned char		*pPage, *pPtr;

	// Validate height and width didn't change
	if ((m_height != 0) && (h != m_height))
	{
		// Height changed - reallocate line counts
		if (m_pLineCnts != NULL)
		{
			delete m_pLineCnts;
			m_pLineCnts = NULL;
		}
	}
	m_height = h;
	m_width = w;
	m_bytes = (w+7)/8 * h;
	m_bytesPerLine = (w+7)/8;

	// Allocate line counts if it doesn't exist
	if (m_pLineCnts == NULL)
		m_pLineCnts = new char[m_height];
	if (m_pLineCnts == NULL)
		return PRINT_ERROR_NOMEM;

	// Calculate the number of bytes needed for each line and the total
	totalBytes = 0;
	for (r = 0; r < h; r++)
	{
		lineOff = r * m_bytesPerLine;

		// Set byte length for this line to zero
		*(m_pLineCnts+r) = 0;

		// Loop through all data for this line looking for non-zero
		for (c = m_bytesPerLine-1; c >= 0; c--)
		{
			if (*(pData + lineOff + c) != 0)
			{
				// Calculate number of bytes for this line
				// We only save half the data because of screen resolution
				*(m_pLineCnts + r) = (c+2)/2;
				totalBytes += 3 + *(m_pLineCnts + r);
				
				// Break out and process next line
				break;
			}
		}
	}
	// Add bytes for the terminating -1 line number
	totalBytes += 2;

	// Allocate page memory
	pPage = new unsigned char[totalBytes];
	pPtr = pPage;

	// Loop through lines with data and copy to page memory
	for (r = 0; r < h; r++)
	{
		// Skip blank lines
		if (*(m_pLineCnts + r) == 0)
			continue;

		// Calculate line offset
		lineOff = r * m_bytesPerLine;
		cnt = *(m_pLineCnts + r) << 1;

		// Save line number and byte count
		*pPtr++ = r & 0xFF;
		*pPtr++ = (unsigned char ) (r >> 8);
		*pPtr++ = *(m_pLineCnts + r);

		// Loop for all bytes on this line
		for (c = 0; c < cnt; c += 2)
		{
			// Get source data
			src = (unsigned short) *(pData + lineOff + c) | 
				((unsigned short) *(pData + lineOff + c + 1) << 8);
			srcMask = 0x0003;			// Test 2 bits at a time
			mask = 0x01;				// Set 1 bit at a time on page
			data = 0;

			// Loop for 8 bits
			for (b = 0; b < 8; b++)
			{
				if (src & srcMask)
					data |= mask;
				mask <<= 1;
				srcMask <<= 2;
			}

			// Save byte in page memory
			*pPtr++ = data;
		}
	}

	// Add terminating -1 line number
	*pPtr++ = 0xFF;
	*pPtr++ = 0xFF;

	// Save "Page" to page object array
	m_pPages.Add((VTObject*)pPage);

	m_pScroll->value(0, this->h(), 0, (m_height + 80) * m_pPages.GetSize());
	m_topPixel = 0;

	return PRINT_ERROR_NONE;
}

/*
=======================================================
Deletes all memory for all pages
=======================================================
*/
void VTVirtualPaper::DeletePageMemory(void)
{
	int		count, c;

	// Get number of pages 
	count = m_pPages.GetSize();
	for (c = 0; c < count; c++)
	{
		delete (unsigned char *) m_pPages[c];
	}

	m_pPages.RemoveAll();
}

/*
=======================================================
Handles the scroll bar action and redraws the pages
=======================================================
*/
void VTVirtualPaper::Scroll(void)
{
	m_topPixel = m_pScroll->value();

	redraw();
}

/*
==============================================================
Define Postscript paper Class below

The Postscript paper provides automatic filename generation
for postscript files and multiple page support.  It creates
the necessary Postscript comments for a Print Manager to 
print individual pages, etc.
==============================================================
*/

/*
================================================================================
VTPSPaper:	This is the class construcor for the Postscript Paper class
================================================================================
*/
VTPSPaper::VTPSPaper(Fl_Preferences* pPref) :
	VTPaper(pPref)
{
	// Initialize pointers
	m_pageNum = 0;					// Clear active page number
	m_pFd = NULL;					// Clear file pointer
}

/*
================================================================================
This is the class destrucor for the Postscript paper class
================================================================================
*/
VTPSPaper::~VTPSPaper(void)
{
}

/*
=======================================================
GetName:	Returns the name of the printer.
=======================================================
*/
MString VTPSPaper::GetName()
{
	return "Postscript File";
}

/*
=======================================================
Initialize the page with preferences.
=======================================================
*/
void VTPSPaper::Init()
{
}

/*
=======================================================
Build the page properties dialog controls.
=======================================================
*/
void VTPSPaper::BuildControls()
{
}

/*
=======================================================
Hide the controls on the property dialog.  This is 
called when the paper is not seleted to make room for
other "papers".
=======================================================
*/
void VTPSPaper::HideControls()
{
}

/*
=======================================================
Show the controls on the property dialog.  This is 
called when the paper is seleted to show the specific
controls for this paper.
=======================================================
*/
void VTPSPaper::ShowControls()
{
}

/*
=======================================================
Get Page Preferences from dialog and save.
=======================================================
*/
void VTPSPaper::GetPrefs(void)
{
}

/*
=======================================================
Cancels the current print job.
=======================================================
*/
int VTPSPaper::CancelJob(void)
{
	// Test if file is open
	if (m_pFd != NULL)
	{
		// Close the file
		fclose(m_pFd);
		m_pFd = NULL;
	}

	// Delete the file

	return PRINT_ERROR_NONE;
}

/*
=======================================================
Prints the current print job.
=======================================================
*/
int VTPSPaper::Print(void)
{
	// Add trailer to Postscript file and close
	if (m_pFd != NULL)
	{
		// Write a trailer to the file
		WriteTrailer();

		// Now close the file
		fclose(m_pFd);
		m_pFd = NULL;
	}
	return PRINT_ERROR_NONE;
}

/*
=======================================================
Creates a new page for printing.
=======================================================
*/
int VTPSPaper::NewPage(void)
{
	m_pageNum++;				// Increment page num

	// Write header for new page to Postscript file

	return PRINT_ERROR_NONE;
}

/*
=======================================================
Loads a new sheet of paper.  Basicaly starts a new
print job from scratch and dumps all old pages.
=======================================================
*/
int VTPSPaper::LoadPaper(void)
{
	int c;

	// Check if a file is open and close if it is
	if (m_pFd != NULL)
	{
		fclose(m_pFd);
		m_pFd = NULL;
	}

	// Clear page number
	m_pageNum = 0;

	// Get next filename for Postscript file
	m_filename = "test.ps";
	
	// Open the file
	m_pFd = fopen((const char *) m_filename, "w+");

	// Write the Postscript header
	WriteHeader();

	return PRINT_ERROR_NONE;
}

/*
=======================================================
Prints data to the current page.
=======================================================
*/
int VTPSPaper::PrintPage(unsigned char *pData, int w, int h)
{
	int					lineOff, numBytes;
	int					bytesPerLine;
	int					r, c, b;
	unsigned char		src;
	unsigned char		mask;
	int					lastX, xp, e;
	int					fCnt, hCnt;
	char				dotChar, lastDotChar;
	float				val;
	int					intVal;

	// Validate the file is open
	if (m_pFd == NULL)
		return PRINT_ERROR_IO_ERROR;

	// Some calculations
	bytesPerLine = (w+7)/8;
	fCnt = 0;
	hCnt = 0;

	// Write a Page header
	WritePageHeader();

	// Calculate the number of bytes needed for each line and the total
	for (r = 0; r < h; r++)
	{
		// Calculate byte offset for this line
		lineOff = r * bytesPerLine;
		numBytes = 0;

		// Loop through all data for this line looking for non-zero
		for (c = bytesPerLine-1; c >= 0; c--)
		{
			if (*(pData + lineOff + c) != 0)
			{
				// Calculate number of bytes for this line
				numBytes = c+1;
				break;
			}
		}

		// If the line is empty, then nothing to do
		if (numBytes == 0)
			continue;

		// If not already on a new line, terminate the previous line
		WriteFChars(fCnt);
		WriteHChars(hCnt);
		if (e != 0)
			fprintf(m_pFd, "\n");
		e = 0;

		// Write the new row number to the Postscript file
		fprintf(m_pFd, "%.1f r\n", 774.0 - (float) r/2.0);
		lastX = -99;
		fCnt = 0;
		hCnt = 0;

		// Loop for all bytes on this line
		for (c = 0; c < numBytes; c++)
		{
			// Get source data
			src = *(pData + lineOff + c);
			mask = 0x01;				// Set 1 bit at a time on page

			// Loop for 8 bits
			for (b = 0; b < 8; b++)
			{
				// output to PS file
				xp = c*8+b;

				// If this bit is on, mark on the page
				if (src & mask)
				{
					if (lastX + 9 >= xp)
					{
						// Calculate the dotChar to be written
						dotChar = 'd'+xp-lastX;

						// Look for repeated 'f' operators
						if (dotChar == 'f')
							++fCnt;
						else if (dotChar == 'h')
							++hCnt;
						else
						{
							// Check if we had reserved 'f' chars not printed
							if (fCnt > 0)
								e += WriteFChars(fCnt);
							if (hCnt > 0)
								e += WriteHChars(hCnt);

							// Print the dot to the Postscript File
							fprintf(m_pFd, "%c ", dotChar);
							e++;
						}
					}
					else
					{
						e += WriteFChars(fCnt);
						e += WriteHChars(hCnt);
						if (xp - lastX == 11)
							fprintf(m_pFd, "x ");
						else if (xp - lastX == 12)
							fprintf(m_pFd, "y ");
						else if (xp - lastX == 13)
							fprintf(m_pFd, "z ");
						else if (xp - lastX == 14)
							fprintf(m_pFd, "c ");
						else if (xp - lastX == 16)
							fprintf(m_pFd, "d ");
						else if (lastX != -99)
						{
							val = (float)(xp - lastX) * .3;
							intVal = (int) val;
							if (val == (float) intVal)
								fprintf(m_pFd, "%d b ", intVal);
							else
								fprintf(m_pFd, "%.1f b ", val);
						}
						else
						{
							val = (float)xp * .3;
							intVal = (int) val;
							if (val == (float) intVal)
								fprintf(m_pFd, "%d a ", 18 + intVal);
							else
								fprintf(m_pFd, "%.1f a ", 18.0+val);
						}
						e++;
					}
					lastX = xp;
				}
				mask <<= 1;
			}

			// Output 16 symbols per Postscript line
			if (e >= 16)
			{
				fprintf(m_pFd, "\n");
				e =0;
			}
		}
	}
	// Print any reserved 'f' chars
	WriteFChars(fCnt);
	WriteHChars(hCnt);

	fprintf(m_pFd, "\n\nshowpage\n");

	return PRINT_ERROR_NONE;
}

/*
=======================================================
Writes a Postscript header to the file
=======================================================
*/
void VTPSPaper::WriteHeader(void)
{
	if (m_pFd == NULL)
		return;

	fprintf(m_pFd, "%%!PS-Adobe-3.0\n");
	fprintf(m_pFd, "%%%%Pages: (atend)\n");
	fprintf(m_pFd, "%%%%BoundingBox: 0 0 612 792\n");
	fprintf(m_pFd, "%%%%Creator:  VirtualT %s\n", VERSION);
	fprintf(m_pFd, "%%%%Title:  FX-80 Emulation Print\n", VERSION);
	fprintf(m_pFd, "%%%%DocumentData: Clean7Bit\n");
	fprintf(m_pFd, "%%%%LanguageLevel: 2\n");
	fprintf(m_pFd, "%%%%EndComments\n");
	fprintf(m_pFd, "/a { newpath dup /xp exch def yp .45 0 360 arc closepath fill} def\n");
	fprintf(m_pFd, "/b { newpath xp add dup /xp exch def yp .45 0 360 arc closepath fill} def\n");
	fprintf(m_pFd, "/c { newpath xp 4.2 add dup /xp exch def yp .45 0 360 arc closepath fill} def\n");
	fprintf(m_pFd, "/d { newpath xp 4.8 add dup /xp exch def yp .45 0 360 arc closepath fill} def\n");
	fprintf(m_pFd, "/e { newpath xp .3 add dup /xp exch def yp .45 0 360 arc closepath fill} def\n");
	fprintf(m_pFd, "/f { newpath xp .6 add dup /xp exch def yp .45 0 360 arc closepath fill} def\n");
	fprintf(m_pFd, "/g { newpath xp .9 add dup /xp exch def yp .45 0 360 arc closepath fill} def\n");
	fprintf(m_pFd, "/h { newpath xp 1.2 add dup /xp exch def yp .45 0 360 arc closepath fill} def\n");
	fprintf(m_pFd, "/i { newpath xp 1.5 add dup /xp exch def yp .45 0 360 arc closepath fill} def\n");
	fprintf(m_pFd, "/j { newpath xp 1.8 add dup /xp exch def yp .45 0 360 arc closepath fill} def\n");
	fprintf(m_pFd, "/k { newpath xp 2.1 add dup /xp exch def yp .45 0 360 arc closepath fill} def\n");
	fprintf(m_pFd, "/l { newpath xp 2.4 add dup /xp exch def yp .45 0 360 arc closepath fill} def\n");
	fprintf(m_pFd, "/m { newpath xp 2.7 add dup /xp exch def yp .45 0 360 arc closepath fill} def\n");
	fprintf(m_pFd, "/n { 0 1 2 { f } for} def\n");
	fprintf(m_pFd, "/o { 0 1 4 { f } for} def\n");
	fprintf(m_pFd, "/p { 0 1 6 { f } for} def\n");
	fprintf(m_pFd, "/q { 0 1 3 -1 roll { f } for} def\n");
	fprintf(m_pFd, "/r { /yp exch def } def\n");
	fprintf(m_pFd, "/s { 0 1 1 { h } for} def\n");
	fprintf(m_pFd, "/t { 0 1 2 { h } for} def\n");
	fprintf(m_pFd, "/u { 0 1 3 { h } for} def\n");
	fprintf(m_pFd, "/v { 0 1 3 -1 roll { h } for} def\n");
	fprintf(m_pFd, "/x { newpath xp 3.3 add dup /xp exch def yp .45 0 360 arc closepath fill} def\n");
	fprintf(m_pFd, "/y { newpath xp 3.6 add dup /xp exch def yp .45 0 360 arc closepath fill} def\n");
	fprintf(m_pFd, "/z { newpath xp 3.9 add dup /xp exch def yp .45 0 360 arc closepath fill} def\n");
}

/*
=======================================================
Writes a Postscript Page Header to the file
=======================================================
*/
void VTPSPaper::WritePageHeader(void)
{
	if (m_pFd == NULL)
		return;

	fprintf(m_pFd, "%%%%Page: %d %d\n", m_pageNum, m_pageNum );
}

/*
=======================================================
Writes a Postscript trailer to the file
=======================================================
*/
void VTPSPaper::WriteTrailer(void)
{
	if (m_pFd == NULL)
		return;

	fprintf(m_pFd, "%%%%Pages: %d\n", m_pageNum + 1);
	fprintf(m_pFd, "%%%%EOF\n");
}

/*
=======================================================
Prints reserved 'f' characters to the Output file
=======================================================
*/
int VTPSPaper::WriteFChars(int &cnt)
{
	int		symbols = 0;

	if ((m_pFd == NULL) || (cnt == 0))
		return 0;

	// We have f3, f5 and f7 symbols, so write the highest
	// f9 is handled during PrintPage
	if (cnt > 7)
	{
		fprintf(m_pFd, "%d q ", cnt-1);
		cnt = 0;
		symbols++;
	}
	else if (cnt == 7)
	{
		fprintf(m_pFd, "p ");
		cnt -= 7;
		symbols++;
	}
	else if (cnt >= 5)
	{
		fprintf(m_pFd, "o ");
		cnt -= 5;
		symbols++;
	}
	else if (cnt >= 3)
	{
		fprintf(m_pFd, "n ");
		cnt -= 3;
		symbols++;
	}

	while (cnt != 0)
	{
		fprintf(m_pFd, "f ");
		cnt--;
		symbols++;
	}

	return symbols;
}

/*
=======================================================
Prints reserved 'h' characters to the Output file
=======================================================
*/
int VTPSPaper::WriteHChars(int &cnt)
{
	int		symbols = 0;

	if ((m_pFd == NULL) || (cnt == 0))
		return 0;

	// We have f3, f5 and f7 symbols, so write the highest
	// f9 is handled during PrintPage
	if (cnt > 4)
	{
		fprintf(m_pFd, "%d v ", cnt-1);
		cnt = 0;
		symbols++;
	}
	else if (cnt == 4)
	{
		fprintf(m_pFd, "u ");
		cnt -= 4;
		symbols++;
	}
	else if (cnt == 3)
	{
		fprintf(m_pFd, "t ");
		cnt -= 3;
		symbols++;
	}
	else if (cnt == 2)
	{
		fprintf(m_pFd, "s ");
		cnt -= 2;
		symbols++;
	}

	while (cnt != 0)
	{
		fprintf(m_pFd, "h ");
		cnt--;
		symbols++;
	}

	return symbols;
}

