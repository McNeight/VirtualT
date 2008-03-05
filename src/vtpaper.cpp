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
	unsigned short		src, srcMask;
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
	fflush(stdout);

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

