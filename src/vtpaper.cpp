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
Deinitialize the page and frees memory
=======================================================
*/
void VTVirtualPaper::Deinit()
{
	DeletePageMemory();
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
	VTVirtualPage*	pVPage;

	// Print to the paper
	count = m_pPages.GetSize();
	if (pageNum >= count)
		return;

	// Calculate page offset based on TopOfForm setting
	pVPage = (VTVirtualPage*) m_pPages[pageNum];
	pageOffset = (int) ((pVPage->m_topOffset + pVPage->m_tof) * 144.0);
	pPage = pVPage->m_pPage;

	// Validate the page memory is not NULL
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
			ypos = pageOffset + ((r*2+1)/3) - m_topPixel;

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
	int				pages, page;
	double			offset;
	VTVirtualPage*	pVPage;
	int				pageBottom;

	// Draw child objects, etc
	Fl_Double_Window::draw();

	// Check if page separator needs to be drawn at top
	if (m_topPixel < 9)
	{
		fl_color(FL_GRAY);
		fl_rectf(0, -m_topPixel, w()-15, 9);
	}

	// Determine in which page m_topPixel falls
	offset = (double) m_topPixel / 144.0;
	pages = m_pPages.GetSize();
	for (page = 0; page < pages; page++)
	{
		pVPage = ((VTVirtualPage*) m_pPages[page]);
		if (offset < pVPage->m_topOffset + pVPage->m_pageHeight)
			break;
	}

	// Draw the top page
	DrawPage(page);

	// Draw pages until window is full
	while (TRUE)
	{
		pageBottom = (int) ((pVPage->m_topOffset + pVPage->m_pageHeight) * 144.0);

		// Check if page separator needs to be drawn
		if (pageBottom < m_topPixel + h())
		{
			fl_color(FL_GRAY);
			fl_rectf(0,  pageBottom- m_topPixel, w()-15, 18);
			fl_color(FL_DARK3);
			fl_rectf(4,  pageBottom- m_topPixel+1, w()-19, 5);
		}

		// Check if next page needs to be drawn
		if (page + 1 < m_pPages.GetSize())
		{
			pVPage = (VTVirtualPage*) m_pPages[page+1];
			int pageTop = (int) (pVPage->m_topOffset * 144);
			if (pageTop < m_topPixel + h())
			{
				DrawPage(page+1);
				page++;
			}
			else
				break;
		}
		else
			break;
	}

	return;
}

/*
=======================================================
Get Page Preferences from dialog and save.
=======================================================
*/
void VTVirtualPaper::GetPrefs(void)
{
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
	int					everyThird;

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
	m_height = (h << 1)/3;
	m_width = w;
	m_bytes = (w+7)/8 * h;
	m_bytesPerLine = (w+7)/8;

	// Allocate line counts if it doesn't exist
	if (m_pLineCnts == NULL)
		m_pLineCnts = new char[h];
	if (m_pLineCnts == NULL)
		return PRINT_ERROR_NOMEM;

	// Calculate the number of bytes needed for each line and the total
	totalBytes = 0;
	everyThird = 0;
	for (r = 0; r < h; r++)
	{
		lineOff = r * m_bytesPerLine;

		// Set byte length for this line to zero
		*(m_pLineCnts+r) = 0;

		// Loop through all data for this line looking for non-zero
		for (c = m_bytesPerLine-1; c >= 0; c--)
		{
			data = *(pData + lineOff + c);
			// Check the 2 1/3 pixel rows together to keep aspect ratio correct
			if (everyThird > 0)
				data |= *(pData + lineOff + c + m_bytesPerLine);

			// Check data
			if (data != 0)
			{
				// Calculate number of bytes for this line
				// We only save half the data because of screen resolution
				*(m_pLineCnts + r) = (c+2)/2;
				totalBytes += 3 + *(m_pLineCnts + r);
				
				// Break out and process next line
				break;
			}
		}
		if (everyThird++ > 0)
		{
			everyThird = 0;
			r++;
		}
	}
	// Add bytes for the terminating -1 line number
	totalBytes += 2;

	// Allocate page memory
	pPage = new unsigned char[totalBytes];
	pPtr = pPage;

	everyThird = 0;
	// Loop through lines with data and copy to page memory
	for (r = 0; r < h; r++)
	{
		// Skip blank lines
		if (*(m_pLineCnts + r) == 0)
		{
			if (everyThird++ > 0)
			{
				everyThird = 0;
				r++;
			}
			continue;
		}

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
			if (everyThird > 0)
			{
				src |= (unsigned short) *(pData + lineOff + c + m_bytesPerLine) |
					((unsigned short) *(pData + lineOff + c + 1 + m_bytesPerLine) << 8);
			}
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
		if (everyThird++ > 0)
		{
			everyThird = 0;
			r++;
		}
	}

	// Add terminating -1 line number
	*pPtr++ = 0xFF;
	*pPtr++ = 0xFF;

	VTVirtualPage* pVPage = new VTVirtualPage;
	pVPage->m_pPage = pPage;
	pVPage->m_pageHeight = m_formHeight;
	pVPage->m_tof = m_tof;

	// Calculate page top 
	if (m_pPages.GetSize() == 0)
		pVPage->m_topOffset = 1.0 / 16.0;	// 1/16" at top
	else
	{
		VTVirtualPage* pTmp = ((VTVirtualPage*) m_pPages[m_pPages.GetSize()-1]);
		pVPage->m_topOffset = pTmp->m_topOffset + pTmp->m_pageHeight + 0.125;
	}

	// Save "Page" to page object array
	m_pPages.Add(pVPage);

	// Calculate total size of scroll based on individual page sizes
	int count = m_pPages.GetSize();
	double totalHeight = 0.0;
	for (c = 0; c < count; c++)
		totalHeight += ((VTVirtualPage*) m_pPages[c])->m_pageHeight;

	m_pScroll->value(0, this->h(), 0, (int) (totalHeight * 144.0) + 18 * count);
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
		delete (VTVirtualPage*) m_pPages[c];
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
	m_autoFile.ChooserTitle("Specify Output Postscript File");
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
	char		temp[512];

	// Load preferences
	if (m_pPref == NULL)
		return;

	m_pPref->get("PSPaper_PromptFilename", m_prompt, 1);
	m_pPref->get("PSPaper_GenAutoFilename", m_autoFilename, 0);
	m_pPref->get("PSPaper_FileFormat", temp, "fx_print_%s.ps", 512);
	m_pPref->get("PSPaper_InkDarkness", m_darkness, 45);
	m_fileFormat = temp;

	m_autoFile.Format(m_fileFormat);
}

/*
=======================================================
Deinitialize the page and frees memory
=======================================================
*/
void VTPSPaper::Deinit()
{
}

void cb_PSAutoFilenameCheck(Fl_Widget* w, void *ptr)
{
	if (ptr != NULL)
	{
		if (((Fl_Check_Button*) w)->value())
			((Fl_Widget*) ptr)->activate();
		else
			((Fl_Widget*) ptr)->deactivate();
	}
}

/*
=======================================================
Build the page properties dialog controls.
=======================================================
*/
void VTPSPaper::BuildControls()
{
	// Control to enable filename prompting
	m_pPrompt = new Fl_Check_Button(45, 210, 155, 20, "Prompt for Filename");
	m_pPrompt->hide();
	m_pPrompt->value(m_prompt);

	// Control to enable auto file generation
	m_pAutoFilename = new Fl_Check_Button(45, 235, 210, 20, "Generate Automatic filename");
	m_pAutoFilename->hide();
	m_pAutoFilename->value(m_autoFilename);

	// Control to edit the output file generator format
	m_pFileFormat = new Fl_Input(65, 260, 240, 20, "");
	m_pFileFormat->hide();
	m_pFileFormat->value((const char *) m_fileFormat);
	if (!m_autoFilename)
		m_pFileFormat->deactivate();

	// Control to show the format specifiers
	m_pFormatText = new Fl_Box(75, 280, 230, 20, "%d = Date   %s = Seq");
	m_pFormatText->align(FL_ALIGN_LEFT | FL_ALIGN_INSIDE);
	m_pFormatText->hide();

	// Create control for ink darkness
	m_pDarkness = new Fl_Slider(90, 305, 140, 20, "Ink Density");
	m_pDarkness->type(5);
	m_pDarkness->box(FL_FLAT_BOX);
	m_pDarkness->precision(0);
	m_pDarkness->range(20, 60);
	m_pDarkness->value(m_darkness);
	m_pDarkness->hide();
	
	m_pLight = new Fl_Box(45, 305, 40, 20, "Light");
	m_pLight->align(FL_ALIGN_LEFT | FL_ALIGN_INSIDE);
	m_pLight->hide();
	m_pDark = new Fl_Box(240, 305, 40, 20, "Dark");
	m_pDark->align(FL_ALIGN_LEFT | FL_ALIGN_INSIDE);
	m_pDark->hide();

	// Set callback for checkbox
	m_pAutoFilename->callback(cb_PSAutoFilenameCheck, m_pFileFormat);
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
	m_pPrompt->hide();
	m_pAutoFilename->hide();
	m_pFileFormat->hide();
	m_pFormatText->hide();
	m_pDarkness->hide();
	m_pDark->hide();
	m_pLight->hide();
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
	m_pPrompt->show();
	m_pAutoFilename->show();
	m_pFileFormat->show();
	m_pFormatText->show();
	m_pDarkness->show();
	m_pDark->show();
	m_pLight->show();
}

/*
=======================================================
Get Page Preferences from dialog and save.
=======================================================
*/
void VTPSPaper::GetPrefs(void)
{
	char		temp[512];

	// Read preferences from controls

	m_prompt = m_pPrompt->value();
	m_autoFilename = m_pAutoFilename->value();
	m_fileFormat = m_pFileFormat->value();
	m_darkness = (int) (m_pDarkness->value());

	// Now save the properties to the preferences
	m_pPref->set("PSPaper_PromptFilename", m_prompt);
	m_pPref->set("PSPaper_GenAutoFilename", m_autoFilename);
	strcpy(temp, (const char *) m_fileFormat);
	m_pPref->set("PSPaper_FileFormat", temp);
	m_pPref->set("PSPaper_InkDarkness", m_darkness);

	// Update the filename autoformatter
	m_autoFile.Format(m_fileFormat);

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
	else
		return PRINT_ERROR_IO_ERROR;

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

	m_autoFile.NextPage();

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
	int 		c;
	char		str[512];

	// Check if a file is open and close if it is
	if (m_pFd != NULL)
	{
		fclose(m_pFd);
		m_pFd = NULL;
	}

	// Clear page number
	m_pageNum = 0;
	m_autoFile.FirstPage();

	// Get a new filename
	if (!GetFilename())
		return PRINT_ERROR_ABORTED;

	// Open the file
	m_pFd = fopen((const char *) m_filename, "w+");
	if (m_pFd == NULL)
	{
		sprintf(str, "Error creating file %s", (const char *) m_filename);
		AddError(str);
		return PRINT_ERROR_IO_ERROR;
	}

	// Write the Postscript header
	WriteHeader();

	return PRINT_ERROR_NONE;
}

/*
==============================================================
Get a new filename based on prompt and auto geneate settings.
==============================================================
*/
int VTPSPaper::GetFilename()
{
	Fl_File_Chooser*	fc;

	// Check if an automatic filename needs to be generated
	if (m_autoFilename)
		m_filename = m_autoFile.GenFilename();

	// Check if we need to prompt for the filename
	if (m_prompt)
	{
		if (m_autoFile.PromptFilename())
			return 0;
	}

	m_filename = m_autoFile.m_Filename;

	return 1;
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
	int					lastX, xp;
	char				dotChar, lastDotChar;
	float				val;
	int					intVal;
	int					diff;
	double				tof;

	// Validate the file is open
	if (m_pFd == NULL)
		return PRINT_ERROR_IO_ERROR;

	// Some calculations
	bytesPerLine = (w+7)/8;
	m_dCnt = 0;
	m_fCnt = 0;
	tof = 72.0 * m_tof;			// Calculate TOF at 72 DPI

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
		WriteDChars();
		WriteFChars();
		WriteDots();

		// Write the new row number to the Postscript file
		fprintf(m_pFd, "%.2f R ", m_formHeight * 72.0 - tof - (float) r/3.0);
		m_len += 8;
		lastX = -99;
		m_dCnt = 0;
		m_fCnt = 0;

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
					diff = xp - lastX;
					if ((diff > 24) && (diff <= 36) && !(diff & 0x01))
					{
						WriteDChars();
						WriteFChars();
						MakeDot('A' + ((diff - 26) >> 1));
					}
					else if (lastX + 24 >= xp)
					{
						// Calculate the dotChar to be written
						dotChar = 'b' + diff;

						// Look for repeated 'd' and 'f' operators
						if (dotChar == 'd')
						{
							WriteFChars();
							++m_dCnt;
						}
						else if (dotChar == 'f')
						{
							WriteDChars();
							++m_fCnt;
						}
						else
						{
							// Check if we had reserved 'f' chars not printed
							WriteDChars();
							WriteFChars();
							MakeDot(dotChar);
						}
					}
					else
					{
						WriteDChars();
						WriteFChars();
						WriteDots();
						if (lastX != -99)
						{
							fprintf(m_pFd, "%d ", xp - lastX - 26);
							MakeDot('b');
						}
						else
						{
							val = (float)xp * .3;
							intVal = (int) val;
							if (val == (float) intVal)
								fprintf(m_pFd, "%d ", 18 + intVal);
							else
								fprintf(m_pFd, "%.1f ", 18.0+val);
							MakeDot('a');
						}
					}
					lastX = xp;
				}
				mask <<= 1;
			}

			// Control Postscript line length
			if (m_len >= 75)
			{
				fprintf(m_pFd, "\n");
				m_len =0;
			}
		}
	}
	// Print any reserved 'f' chars
	WriteDChars();
	WriteFChars();
	WriteDots();

	fprintf(m_pFd, "\n\nshowpage\n");

	return PRINT_ERROR_NONE;
}


/*
=======================================================
Marks a dot on the page.  This routine takes care of printing
reserved characters, calculating line lengths, compressing
symbols and writing strings to the file, etc.
=======================================================
*/
void VTPSPaper::MakeDot(char dotChar)
{
	// Print the dot to the Postscript File
	m_dots = m_dots + dotChar;
	if (m_len + m_dots.GetLength() > 75)
	{
		WriteDots();
		fprintf(m_pFd, "\n");
		m_len = 0;
	}
}

/*
=======================================================
Writes a Postscript header to the file
=======================================================
*/
void VTPSPaper::WriteHeader(void)
{
	double		gray, size;

	if (m_pFd == NULL)
		return;
	

	gray = 1.0;
	size = (double) m_darkness / 100.0;
	if (m_darkness < 30)
	{
		size = .45;
		gray = 0.5 + (double) (m_darkness - 20) * .05;
	}
	fprintf(m_pFd, "%%!PS-Adobe-3.0\n");
	fprintf(m_pFd, "%%%%Pages: (atend)\n");
	fprintf(m_pFd, "%%%%Creator:  VirtualT %s\n", VERSION);
	fprintf(m_pFd, "%%%%Title:  FX-80 Emulation Print\n", VERSION);
	fprintf(m_pFd, "%%%%DocumentData: Clean7Bit\n");
	fprintf(m_pFd, "%%%%LanguageLevel: 2\n");
	fprintf(m_pFd, "%%%%EndComments\n");
	fprintf(m_pFd, "/a { newpath dup /xp exch def yp %.2f 0 360 arc closepath %.02f setgray fill} def\n",
		size, gray);
	fprintf(m_pFd, "/b { 26 add .3 mul dot } def\n");
	fprintf(m_pFd, "/c { .3 dot } def ");
	fprintf(m_pFd, "/d { .6 dot } def ");
	fprintf(m_pFd, "/e { .9 dot } def ");
	fprintf(m_pFd, "/f { 1.2 dot } def\n");
	fprintf(m_pFd, "/g { 1.5 dot } def ");
	fprintf(m_pFd, "/h { 1.8 dot } def ");
	fprintf(m_pFd, "/i { 2.1 dot } def ");
	fprintf(m_pFd, "/j { 2.4 dot } def\n");
	fprintf(m_pFd, "/k { 2.7 dot } def ");
	fprintf(m_pFd, "/l { 3 dot } def ");
	fprintf(m_pFd, "/m { 3.3 dot } def ");
	fprintf(m_pFd, "/n { 3.6 dot } def\n");
	fprintf(m_pFd, "/o { 3.9 dot } def ");
	fprintf(m_pFd, "/p { 4.2 dot } def ");
	fprintf(m_pFd, "/q { 4.5 dot } def ");
	fprintf(m_pFd, "/r { 4.8 dot } def\n");
	fprintf(m_pFd, "/s { 5.1 dot } def ");
	fprintf(m_pFd, "/t { 5.4 dot } def ");
	fprintf(m_pFd, "/u { 5.7 dot } def ");
	fprintf(m_pFd, "/v { 6 dot } def\n");
	fprintf(m_pFd, "/w { 6.3 dot } def ");
	fprintf(m_pFd, "/x { 6.6 dot } def ");
	fprintf(m_pFd, "/y { 6.9 dot } def ");
	fprintf(m_pFd, "/z { 7.2 dot } def\n");
	fprintf(m_pFd, "/dot { newpath xp add dup /xp exch def yp %.2f 0 360 arc closepath %.2f setgray fill} def\n",
		size, gray);
	fprintf(m_pFd, "/A { 7.8 dot } def\n");
	fprintf(m_pFd, "/B { 8.4 dot } def ");
	fprintf(m_pFd, "/C { 9 dot } def ");
	fprintf(m_pFd, "/D { 9.6 dot } def ");
	fprintf(m_pFd, "/E { 10.2 dot } def\n");
	fprintf(m_pFd, "/F { 10.8 dot } def ");

	fprintf(m_pFd, "/N { 0 1 2 { d } for} def\n");
	fprintf(m_pFd, "/O { 0 1 4 { d } for} def ");
	fprintf(m_pFd, "/P { 0 1 6 { d } for} def\n");
	fprintf(m_pFd, "/Q { 0 1 3 -1 roll { d } for} def ");
	fprintf(m_pFd, "/R { /yp exch def } def\n");
	fprintf(m_pFd, "/S { 0 1 1 { f } for} def\n");
	fprintf(m_pFd, "/T { 0 1 2 { f } for} def ");
	fprintf(m_pFd, "/U { 0 1 3 { f } for} def\n");
	fprintf(m_pFd, "/V { 0 1 3 -1 roll { f } for} def ");
	fprintf(m_pFd, "/chr 1 string def\n");
	fprintf(m_pFd, "/chrStr { chr 0 3 -1 roll put } def\n");
	fprintf(m_pFd, "/X { 80 string cvs { chrStr chr cvx exec } forall } def\n");
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
	fprintf(m_pFd, "%%%%PageBoundingBox: 0 0 612 %d\n", (int) (m_formHeight * 72.0) );
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
Prints reserved 'd' characters to the Output file
=======================================================
*/
int VTPSPaper::WriteDChars()
{
	int		symbols = 0;

	if ((m_pFd == NULL) || (m_dCnt == 0))
		return 0;

	// We have f3, f5 and f7 symbols, so write the highest
	// f9 is handled during PrintPage
	if (m_dCnt > 7)
	{
		WriteDots();
		fprintf(m_pFd, "%d ", m_dCnt-1);
		MakeDot('Q');
		m_dCnt = 0;
		symbols++;
	}
	else if (m_dCnt == 7)
	{
		MakeDot('P');
		m_dCnt -= 7;
		symbols++;
	}
	else if (m_dCnt >= 5)
	{
		MakeDot('O');
		m_dCnt -= 5;
		symbols++;
	}
	else if (m_dCnt >= 3)
	{
		MakeDot('N');
		m_dCnt -= 3;
		symbols++;
	}

	while (m_dCnt != 0)
	{
		MakeDot('d');
		m_dCnt--;
		symbols++;
	}

	return symbols;
}

/*
=======================================================
Prints reserved 'f' characters to the Output file
=======================================================
*/
int VTPSPaper::WriteFChars()
{
	int		symbols = 0;

	if ((m_pFd == NULL) || (m_fCnt == 0))
		return 0;

	// We have f3, f5 and f7 symbols, so write the highest
	// f9 is handled during PrintPage
	if (m_fCnt > 4)
	{
		WriteDots();
		fprintf(m_pFd, "%d ", m_fCnt-1);
		MakeDot('V');
		m_fCnt = 0;
		symbols++;
	}
	else if (m_fCnt == 4)
	{
		MakeDot('U');
		m_fCnt -= 4;
		symbols++;
	}
	else if (m_fCnt == 3)
	{
		MakeDot('T');
		m_fCnt -= 3;
		symbols++;
	}
	else if (m_fCnt == 2)
	{
		MakeDot('S');
		m_fCnt -= 2;
		symbols++;
	}

	while (m_fCnt != 0)
	{
		MakeDot('f');
		m_fCnt--;
		symbols++;
	}

	return symbols;
}

/*
================================================================
Writes dots to the file, taking any reserved dots into account.
================================================================
*/
void VTPSPaper::WriteDots()
{
	int		c, count;

	count = m_dots.GetLength();

	// Return if no dots to print
	if (count == 0)
		return;

	// If More than 5 dots, write it as a string
	if (count > 5)
	{
		fprintf(m_pFd, "/%s X ", (const char *) m_dots);
		m_len += 5 + m_dots.GetLength();
	}
	else
	{
		for (c = 0; c < count; c++)
			fprintf(m_pFd, "%c ", m_dots[c]);
		m_len += (c << 2);
	}

	m_dots = "";
}

/*
==============================================================
Define lpr via Postscript paper Class below

The Postscript paper provides automatic filename generation
for postscript files and multiple page support.  It creates
the necessary Postscript comments for a Print Manager to 
print individual pages, etc.
==============================================================
*/

/*
================================================================================
VTlprPaper:	This is the class construcor for the lpr via Postscript Paper class
================================================================================
*/
VTlprPaper::VTlprPaper(Fl_Preferences* pPref) :
	VTPSPaper(pPref)
{
}

/*
=======================================================
GetName:	Returns the name of the printer.
=======================================================
*/
MString VTlprPaper::GetName()
{
	return "lpr via Postscript file";
}

/*
=======================================================
Initialize the page with preferences.
=======================================================
*/
void VTlprPaper::Init()
{
	char		temp[512];

	//VTPSPaper::Init();
	// Load preferences
	if (m_pPref == NULL)
		return;

	m_pPref->get("LprPaper_FileFormat", temp, "fx_print_%s.ps", 512);
	m_fileFormat = temp;
	m_pPref->get("LprPaper_CmdLine", temp, "lpr -r %f", 512);
	m_cmdLine = temp;
	m_pPref->get("LprPaper_InkDarkness", m_darkness, 45);

	// Force other values
	m_prompt = FALSE;			// No file prompting
	m_autoFilename = TRUE;		// Generate auto filename
	m_autoFile.Format(m_fileFormat);
}

/*
=======================================================
Build the page properties dialog controls.
=======================================================
*/
void VTlprPaper::BuildControls()
{
	// Control to edit the output file generator format
	m_pFileFormat = new Fl_Input(65, 215, 240, 20, "Format");
	m_pFileFormat->hide();
	m_pFileFormat->value((const char *) m_fileFormat);

	// Control to edit the output file generator format
	m_pCmdLine = new Fl_Input(65, 245, 240, 20, "Cmd");
	m_pCmdLine->hide();
	m_pCmdLine->value((const char *) m_cmdLine);

	// Control to show the format specifiers
	m_pFormatText = new Fl_Box(75, 270, 230, 20, "%d = Date  %s = Seq  %f = File");
	m_pFormatText->align(FL_ALIGN_LEFT | FL_ALIGN_INSIDE);
	m_pFormatText->hide();

	// Create control for ink darkness
	m_pDarkness = new Fl_Slider(90, 305, 140, 20, "Ink Density");
	m_pDarkness->type(5);
	m_pDarkness->box(FL_FLAT_BOX);
	m_pDarkness->precision(0);
	m_pDarkness->range(20, 60);
	m_pDarkness->value(m_darkness);
	m_pDarkness->hide();
	
	m_pLight = new Fl_Box(45, 305, 40, 20, "Light");
	m_pLight->align(FL_ALIGN_LEFT | FL_ALIGN_INSIDE);
	m_pLight->hide();
	m_pDark = new Fl_Box(240, 305, 40, 20, "Dark");
	m_pDark->align(FL_ALIGN_LEFT | FL_ALIGN_INSIDE);
	m_pDark->hide();

}

/*
=======================================================
Get Page Preferences from dialog and save.
=======================================================
*/
void VTlprPaper::GetPrefs()
{
	char		temp[512];

	// Read preferences from controls

	m_fileFormat = m_pFileFormat->value();
	m_cmdLine = m_pCmdLine->value();
	m_darkness = (int) (m_pDarkness->value());

	// Now save the properties to the preferences
	strcpy(temp, (const char *) m_fileFormat);
	m_pPref->set("LprPaper_FileFormat", temp);
	strcpy(temp, (const char *) m_cmdLine);
	m_pPref->set("LprPaper_CmdLine", temp);
	m_pPref->set("LprPaper_InkDarkness", m_darkness);

	// Update the filename autoformatter
	m_autoFile.Format(m_fileFormat);

}

/*
=======================================================
Hide the controls on the property dialog.  This is 
called when the paper is not seleted to make room for
other "papers".
=======================================================
*/
void VTlprPaper::HideControls()
{
	m_pFileFormat->hide();
	m_pCmdLine->hide();
	m_pFormatText->hide();
	m_pDarkness->hide();
	m_pDark->hide();
	m_pLight->hide();
}

/*
=======================================================
Show the controls on the property dialog.  This is 
called when the paper is seleted to show the specific
controls for this paper.
=======================================================
*/
void VTlprPaper::ShowControls()
{
	m_pFileFormat->show();
	m_pCmdLine->show();
	m_pFormatText->show();
	m_pDarkness->show();
	m_pDark->show();
	m_pLight->show();
}

/*
=======================================================
Prints the current print job.
=======================================================
*/
int VTlprPaper::Print(void)
{
	MString		cmd;
	int			len, c;
	FILE*		pFD;
	char		buf[512];

	// First let the PSPaper finish it's job
	VTPSPaper::Print();

	// Build command base on format line - search for %f
	len = m_cmdLine.GetLength();
	for (c = 0; c < len; c++)
	{
		if (m_cmdLine[c] == '%')
		{
			if (c + 1 != len)
			{
				if (m_cmdLine[c+1] == 'f')
					cmd += m_filename;
				c++;
			}
		}
		else
			cmd += m_cmdLine[c];
	}

	// Test if stderr redirection is part of cmd line
	if (strstr((const char *) cmd, "2>&1") == NULL)
		cmd += " 2>&1";

	// Now spool the job
	//if (fork() == 0)
	{
		// Child process - perform system call to print
		pFD = popen((const char *) cmd, "r");
		if (pFD == NULL)
		{
			// Report error spawning
			AddError("Error spawning print job");
		}

		// Read data from the process' stdout looking for errors
		while (fgets(buf, 512, pFD) != NULL)
		{
			if ((strstr(buf, "Error") != 0) || (strstr(buf, "error") != 0))
				AddError(buf);
		}

		// Close the stream and exit the child process
		pclose(pFD);
		//exit(0);
	}
}

