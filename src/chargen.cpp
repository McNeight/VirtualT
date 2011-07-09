/* chargen.cpp */

/* $Id: chargen.cpp,v 1.7 2011/07/09 08:16:21 kpettit1 Exp $ */

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
#include <FL/Fl_Multiline_Input.H>
#include <FL/Fl_Choice.H>
#include <FL/Fl_Box.H>
#include <FL/Fl_Preferences.H>
#include <FL/Fl_File_Chooser.H>
#include <FL/Fl_Bitmap.H>
#include <FL/fl_ask.H>

#include "FLU/Flu_File_Chooser.h"

#include <string.h>
#include <stdio.h>
#include <stdlib.h>

#include "VirtualT.h"
#include "chargen.h"
#include "MString.h"

#define GRID_SIZE 		24
#define	BORDER_SIZE		6
#define	EDIT_FIELD_X	200
#define EDIT_FIELD_Y	10

VTCharacterGen*		gpCharGen;

/*
================================================================================
Callback routine for character generator window.
================================================================================
*/
void cb_CloseCharGen(Fl_Widget* w, void *)
{
	// Ensure pointer isn't null
	if (gpCharGen == NULL)
		return;
	
	gpCharGen->SaveActiveChar();

	// Test if modified
	if (gpCharGen->CheckForSave())
		return;

	// Okay to close
	gpCharGen->hide();
	delete gpCharGen;
	gpCharGen = NULL;
}

void cb_CreateNewCharGen(Fl_Widget* w, void *)
{
	if (gpCharGen == NULL)
	{
		gpCharGen = new VTCharacterGen(540, 500, "FX-80 Character Generator");
		gpCharGen->end();
		gpCharGen->callback(cb_CloseCharGen);
		gpCharGen->show();
	}
}
void cb_Clear(Fl_Widget* w, void *)
{	
	if (gpCharGen != NULL)
		gpCharGen->ClearPixmap();
}

void cb_Copy(Fl_Widget* w, void *)
{	
	if (gpCharGen != NULL)
		gpCharGen->CopyPixmap();
}

void cb_Paste(Fl_Widget* w, void *)
{	
	if (gpCharGen != NULL)
		gpCharGen->PastePixmap();
}

void cb_CharGenLoad(Fl_Widget* w, void *)
{	
	if (gpCharGen != NULL)
		gpCharGen->Load();
}

void cb_CharGenSave(Fl_Widget* w, void * format)
{	
	if (gpCharGen != NULL)
		gpCharGen->Save((intptr_t) format);
}

/*
================================================================================
VTCharacterGen:	This is the class construcor for the FX80Print Device emulation.
================================================================================
*/
VTCharacterGen::VTCharacterGen(int w, int h, const char* title) :
	Fl_Double_Window(w, h, title)
{
	int			c, r;

	Fl_Box*	o;
	Fl_Button*	b;

	// Create Text field with char value
	o = new Fl_Box(FL_NO_BOX, 20, 10, 100, 20, "Character:");
	o->align(FL_ALIGN_LEFT | FL_ALIGN_INSIDE);
	m_pCharText = new Fl_Box(FL_NO_BOX, 120, 10, 60, 20, "");
	m_pCharText->align(FL_ALIGN_LEFT | FL_ALIGN_INSIDE);

	// Create Clear, Copy, Paste buttons
	b = new Fl_Button(20, 45, 60, 25, "Copy");
	b->callback(cb_Copy);
	b = new Fl_Button(100, 45, 60, 25, "Paste");
	b->callback(cb_Paste);
	b = new Fl_Button(20, 80, 60, 25, "Clear");
	b->callback(cb_Clear);

	for (c = 0; c < 6; c++)
		for (r = 0; r < 9; r++)
		{
			m_pBoxes[c][r] = new Fl_Box(FL_BORDER_BOX, EDIT_FIELD_X + c*GRID_SIZE, 
				EDIT_FIELD_Y + r*GRID_SIZE, GRID_SIZE+1, GRID_SIZE+1, "");
		}
	// Generate the dots
	for (c = 0; c < 11; c++)
		for (r = 0; r < 9; r++)
		{
			m_pDots[c][r] = new Fl_Box(FL_OVAL_BOX, EDIT_FIELD_X + c*GRID_SIZE/2+5, 
				EDIT_FIELD_Y +r*GRID_SIZE+5, GRID_SIZE-10, GRID_SIZE-10, "");
			m_pDots[c][r]->color(FL_BLACK);
			m_pDots[c][r]->hide();
			m_Dots[c][r] = 0;
		}

	// Create text for the sample output displays
	o = new Fl_Box(EDIT_FIELD_X + GRID_SIZE * 7, 10, 80, 20, "Pica:");
	o->align(FL_ALIGN_RIGHT | FL_ALIGN_INSIDE);
	m_pExPica = new Fl_Box(FL_NO_BOX, EDIT_FIELD_X + GRID_SIZE * 7 + 100, 10, 35, 35, "");
	o = new Fl_Box(EDIT_FIELD_X + GRID_SIZE * 7, 50, 80, 20, "Expanded:");
	o->align(FL_ALIGN_RIGHT | FL_ALIGN_INSIDE);
	m_pExPica = new Fl_Box(FL_NO_BOX, EDIT_FIELD_X + GRID_SIZE * 7 + 100, 10, 35, 35, "");
	m_pExExpand = new Fl_Box(FL_NO_BOX, EDIT_FIELD_X + GRID_SIZE * 7 + 100, 50, 35, 35, "");
	o = new Fl_Box(EDIT_FIELD_X + GRID_SIZE * 7, 90, 80, 20, "Enhanced:");
	o->align(FL_ALIGN_RIGHT | FL_ALIGN_INSIDE);
	m_pExPica = new Fl_Box(FL_NO_BOX, EDIT_FIELD_X + GRID_SIZE * 7 + 100, 10, 35, 35, "");
	m_pExEnhance = new Fl_Box(FL_NO_BOX, EDIT_FIELD_X + GRID_SIZE * 7 + 100, 90, 35, 35, "");
	o = new Fl_Box(EDIT_FIELD_X + GRID_SIZE * 7, 130, 80, 20, "Dbl-Strike:");
	o->align(FL_ALIGN_RIGHT | FL_ALIGN_INSIDE);
	m_pExPica = new Fl_Box(FL_NO_BOX, EDIT_FIELD_X + GRID_SIZE * 7 + 100, 10, 35, 35, "");
	m_pExDblStrike = new Fl_Box(FL_NO_BOX, EDIT_FIELD_X + GRID_SIZE * 7 + 100, 130, 35, 35, "");
	o = new Fl_Box(EDIT_FIELD_X + GRID_SIZE * 7, 170, 80, 20, "Dbl-Enhance:");
	o->align(FL_ALIGN_RIGHT | FL_ALIGN_INSIDE);
	m_pExPica = new Fl_Box(FL_NO_BOX, EDIT_FIELD_X + GRID_SIZE * 7 + 100, 10, 35, 35, "");
	m_pExDblEnhance = new Fl_Box(FL_NO_BOX, EDIT_FIELD_X + GRID_SIZE * 7 + 100, 170, 35, 35, "");

	// Create a character table
	m_pCharTable = new VTCharTable(40, EDIT_FIELD_Y + GRID_SIZE * 11, 15*32, 22*8,"");
	m_pCharTable->callback(cb_CloseCharGen);

	// Text for the Char Table
	o = new Fl_Box(FL_NO_BOX, 10, EDIT_FIELD_Y+GRID_SIZE*11+5, 25, 15,"00h");
	o->align(FL_ALIGN_RIGHT | FL_ALIGN_INSIDE);
	o->labelsize(10);
	o = new Fl_Box(FL_NO_BOX, 10, EDIT_FIELD_Y+GRID_SIZE*11+5+1*22, 25, 15,"20h");
	o->align(FL_ALIGN_RIGHT | FL_ALIGN_INSIDE);
	o->labelsize(10);
	o = new Fl_Box(FL_NO_BOX, 10, EDIT_FIELD_Y+GRID_SIZE*11+5+2*22, 25, 15,"40h");
	o->align(FL_ALIGN_RIGHT | FL_ALIGN_INSIDE);
	o->labelsize(10);
	o = new Fl_Box(FL_NO_BOX, 10, EDIT_FIELD_Y+GRID_SIZE*11+5+3*22, 25, 15,"60h");
	o->align(FL_ALIGN_RIGHT | FL_ALIGN_INSIDE);
	o->labelsize(10);
	o = new Fl_Box(FL_NO_BOX, 10, EDIT_FIELD_Y+GRID_SIZE*11+5+4*22, 25, 15,"80h");
	o->align(FL_ALIGN_RIGHT | FL_ALIGN_INSIDE);
	o->labelsize(10);
	o = new Fl_Box(FL_NO_BOX, 10, EDIT_FIELD_Y+GRID_SIZE*11+5+5*22, 25, 15,"A0h");
	o->align(FL_ALIGN_RIGHT | FL_ALIGN_INSIDE);
	o->labelsize(10);
	o = new Fl_Box(FL_NO_BOX, 10, EDIT_FIELD_Y+GRID_SIZE*11+5+6*22, 25, 15,"C0h");
	o->align(FL_ALIGN_RIGHT | FL_ALIGN_INSIDE);
	o->labelsize(10);
	o = new Fl_Box(FL_NO_BOX, 10, EDIT_FIELD_Y+GRID_SIZE*11+5+7*22, 25, 15,"E0h");
	o->align(FL_ALIGN_RIGHT | FL_ALIGN_INSIDE);
	o->labelsize(10);
	o = new Fl_Box(FL_NO_BOX, 40, EDIT_FIELD_Y+GRID_SIZE*11-15, 25, 15,"00h");
	o->align(FL_ALIGN_LEFT | FL_ALIGN_INSIDE);
	o->labelsize(10);
	o = new Fl_Box(FL_NO_BOX, 40+15*32-22, EDIT_FIELD_Y+GRID_SIZE*11-15, 25, 15,"1Fh");
	o->align(FL_ALIGN_RIGHT | FL_ALIGN_INSIDE);
	o->labelsize(10);

	// Create load button
	b = new Fl_Button(40, 460, 70, 30, "Load");
	b->callback(cb_CharGenLoad);
	
	// Create Save button
	b = new Fl_Button(130, 460, 70, 30, "Save");
	b->callback(cb_CharGenSave, (void *) CHARGEN_FORMAT_BINARY);

	// Create Save C button
	b = new Fl_Button(220, 460, 80, 30, "Save C");
	b->callback(cb_CharGenSave, (void *) CHARGEN_FORMAT_C);

	// Create Save BASIC button
	b = new Fl_Button(320, 460, 110, 30, "Save BASIC");
	b->callback(cb_CharGenSave, (void *) CHARGEN_FORMAT_BASIC);

	// Create Close button
	m_pClose = new Fl_Button(450, 460, 70, 30, "Close");
	m_pClose->callback(cb_CloseCharGen);

	m_PasteValid = FALSE;
	m_ActiveChar = -1;
	m_Modified = 0;
}

/*
================================================================================
Save active character pixel data to CharTable and set modified flag if changed.
================================================================================
*/
void VTCharacterGen::SaveActiveChar(void)
{
	unsigned char	data[12], old_data[12];
	int				r, c;
	int				shift, start;

	// Check if we need to save map data
	if (m_ActiveChar != -1)
	{
		data[0] = 0;
		// Save pixmap to CharTable
		for (c = 0; c < 11; c++)
			if (m_Dots[c][0])
			{
				data[0] = 0x80;
				break;
			}

		// Create bytes to copy to data
		for (c = 0; c < 11; c++)
		{
			data[c+1] = 0;
			shift = 0x80;
			start = data[0] ? 0 : 1;
			for (r = start; r < 8+start; r++)
			{
				if (m_Dots[c][r])
					data[c+1] |= shift;
				shift >>= 1;
			}
		}

		// Write data to CharTable
		m_pCharTable->GetCharData(m_ActiveChar, old_data);
		m_pCharTable->PutCharData(m_ActiveChar, data);
		for (c = 0; c < 12; c++)
			if (old_data[c] != data[c])
				m_Modified = 1;
	}
}

/*
================================================================================
Handle the CharTable Edit region of the window.
================================================================================
*/
void VTCharacterGen::HandleCharTableEdit(int xp, int yp)
{
	int				index;
	int				r;

	index = (xp - m_pCharTable->x()) / 15;
	r = (yp - m_pCharTable->y()) / 22;
	index += 0x20 * r;

	if ((index > 255) || (index < 0))
		return;

	ChangeActiveChar(index);
}

void VTCharacterGen::ChangeActiveChar(int index)
{
	unsigned char	data[12];
	int				c, r, shift;
	unsigned char	ored;

	m_pCharTable->SetActiveChar(index);
	sprintf(m_CharText, "0x%02X", index);
	m_pCharText->label(m_CharText);

	SaveActiveChar();

	m_ActiveChar = index;

	// Get Table Data for the selected char
	m_pCharTable->GetCharData(index, data);

	for (c = 0; c < 6; c++)
	{
		m_pBoxes[c][0]->label("");
		m_pBoxes[c][8]->label("");
	}

	for (c = 0; c < 11; c++)
		for (r = 0; r < 9; r++)
		{
			m_pDots[c][r]->hide();
			m_Dots[c][r] = 0;
		}

	ored = 0;
	// Update pixel edit field with character data
	for (c = 0; c < 11; c++)
	{
		shift = 0;
		ored |= data[c+1];

		// Loop through each row of the pixel edit field
		for (r = 0; r < 9; r++)
		{
			// Check if the bottom pins are being used
			if ((!data[0] && (r == 0)) || (data[0] && (r == 8)))
				continue;

			// Hide or show the pixel base on table data
			if (data[c+1] & (0x80 >> shift++))
			{
				m_pDots[c][r]->show();
				m_Dots[c][r] = 1;
			}
		}
	}

	// Update 'x' in top and bottom row
	if (data[0] && (ored & 0x80))
	{
		for (c = 0; c < 6; c++)
			m_pBoxes[c][8]->label("x");
	}
	else if (!data[0] && (ored & 0x01))
	{
		for (c = 0; c < 6; c++)
			m_pBoxes[c][0]->label("x");
	}

	UpdatePreviews();
}

/*
================================================================================
Handle the Pixel Edit region of the window.
================================================================================
*/
void VTCharacterGen::HandlePixelEdit(int xp, int yp)
{
	int		c, i;
	int		row, col;

	// Calculate which pixel is being seleted
	row = (yp - EDIT_FIELD_Y) / GRID_SIZE;
	col = (xp - EDIT_FIELD_X) / GRID_SIZE;
	col <<= 1;

	// Test if the coords are on an odd pixel
	if (col != 0)
	{
		if (xp - (EDIT_FIELD_X + col*GRID_SIZE/2) < BORDER_SIZE)
			col--;
	}
	if (col != 10)
	{
		if ((EDIT_FIELD_X + (col+2)*GRID_SIZE/2) - xp < BORDER_SIZE)
			col++;
	}

	if ((col >= 11) || (row >= 9))
		return;

	// Test if the pixel is being turned on or off
	if (m_Dots[col][row])
	{
		// Turning it off.  Hide the pixel
		m_Dots[col][row] = 0;
		m_pDots[col][row]->hide();
		UpdatePreviews();

		// Test if we need to enable top or bottom row
		if ((row == 0) || (row == 8))
		{
			// Test if any dot on in this row
			for (c = 0; c < 11; c++)
				if (m_Dots[c][row])
					return;
		}
		// Enable all boxes
		if (row == 0)
			for (i = 0; i < 6; i++)
				m_pBoxes[i][8]->label("");
		else if (row == 8)
			for (i = 0; i < 6; i++)
				m_pBoxes[i][0]->label("");
	}
	else
	{
		// Turning it on.  Validate no 2 dots beside each other
		if (col > 0)
			if (m_Dots[col-1][row])
				return;
		if (col < 10)
			if (m_Dots[col+1][row])
				return;

		// Validate not using both top row and bottom row
		if ((row == 0) || (row == 8))
		{
			for (i = 0; i < 11; i++)
			{
				if (row == 0)
				{
					if (m_Dots[i][8])
						return;
				}
				else
				{
					if (m_Dots[i][0])
						return;
				}
			}
		}

		m_Dots[col][row] = 1;
		m_pDots[col][row]->show();
		if (row == 0)
			for (i = 0; i < 6; i++)
				m_pBoxes[i][8]->label("X");
		else if (row == 8)
			for (i = 0; i < 6; i++)
				m_pBoxes[i][0]->label("X");
		
	}

	UpdatePreviews();
	return;
}

/*
================================================================================
Copy pixels from the pixmap
================================================================================
*/
void VTCharacterGen::CopyPixmap(void)
{
	int 	c, r;

	// Copy data to the buffer
	for (c = 0; c < 11; c++)
		for (r = 0; r < 9; r++)
			m_Buffer[c][r] = m_Dots[c][r];

	m_PasteValid = 1;
}

/*
================================================================================
Paste pixels from the buffer
================================================================================
*/
void VTCharacterGen::PastePixmap(void)
{
	int 	c, r;
	bool	r0, r8;

	if (!m_PasteValid)
		return;

	// Copy data to the buffer
	for (c = 0; c < 11; c++)
		for (r = 0; r < 9; r++)
			m_Dots[c][r] = m_Buffer[c][r];

	// Update boxes
	r0 = r8 = 0;
	for (c = 0; c < 11; c++)
	{
		r0 |= m_Dots[c][0];
		r8 |= m_Dots[c][8];
	}
	for (c = 0; c < 6; c++)
	{
		m_pBoxes[c][0]->label("");
		m_pBoxes[c][8]->label("");
		if (r0) m_pBoxes[c][8]->label("x");
		if (r8) m_pBoxes[c][0]->label("x");
	}

	// Copy data to the buffer
	for (c = 0; c < 11; c++)
		for (r = 0; r < 9; r++)
		{
			m_pDots[c][r]->hide();
			if (m_Dots[c][r])
				m_pDots[c][r]->show();
			else
				m_pDots[c][r]->hide();
		}

	UpdatePreviews();
	m_Modified = 1;
}

/*
================================================================================
Clear the current pixmap and updates the GUI.
================================================================================
*/
void VTCharacterGen::ClearPixmap(void)
{
	int		r, c;

	// Remove 'x' from the boxes
	for (c = 0; c < 6; c++)
	{
		m_pBoxes[c][0]->label("");
		m_pBoxes[c][8]->label("");
	}

	// Turn off all pixels
	for (c = 0; c < 11; c++)
		for (r = 0; r < 9; r++)
		{
			m_pDots[c][r]->hide();
			m_Dots[c][r] = 0;
		}

	UpdatePreviews();
	m_Modified = 1;
}

/*
================================================================================
Handle Mouse events to turn pixels on / off.
================================================================================
*/
int VTCharacterGen::handle(int event)
{
	int		c, xp, yp;
	int		key;

	switch (event)
	{
	case FL_PUSH:
		// Get id of mouse button
		c = Fl::event_button();

		// Check if it was the Left Mouse button
		if (c == FL_LEFT_MOUSE)
		{
			xp = Fl::event_x();
			yp = Fl::event_y();

			// Test if x,y is in the Pixel Edit region
			if ((xp >= EDIT_FIELD_X) && (xp <= EDIT_FIELD_X + 6 * GRID_SIZE) &&
				(yp >= EDIT_FIELD_Y) && (yp <= EDIT_FIELD_Y + 9 * GRID_SIZE))
					{
					HandlePixelEdit(xp, yp);
					return TRUE;
					}

			// Test if x,y is in the Character Table
			if ((xp >= m_pCharTable->x()) && (xp <= m_pCharTable->x() + m_pCharTable->w()) && 
				(yp >= m_pCharTable->y()) && (yp <= m_pCharTable->y() + m_pCharTable->h()))
					{
					HandleCharTableEdit(xp, yp);	
					return FALSE;
					}
		}
		break;
	case FL_KEYDOWN:
		key = Fl::event_key();
		switch (key)
		{
			case FL_Left:
				if (m_ActiveChar > 0)
					ChangeActiveChar(m_ActiveChar - 1);
				return TRUE;

			case FL_Right:
				if (m_ActiveChar < 255)
					ChangeActiveChar(m_ActiveChar + 1);
				return TRUE;

			case FL_Up:
				if (m_ActiveChar > 31)
					ChangeActiveChar(m_ActiveChar - 32);
				return TRUE;

			case FL_Down:
				if (m_ActiveChar < 256-32)
					ChangeActiveChar(m_ActiveChar + 32);
				return TRUE;
		}
		break;
	}
	return Fl_Double_Window::handle(event);
}

/*
================================================================================
Check the m_Modified bit to see if changes were made
================================================================================
*/
int VTCharacterGen::CheckForSave(void)
{
	int		ret;

	// If not modified, just return
	if (!m_Modified)
		return 0;

	// Ask if the file should be saved
	ret = fl_choice("Save changes?", "Cancel", "Yes", "No");

	// Test for cancel
	if (ret == 0)
		return 1;

	// Test for Yes
	if (ret == 1)
		Save(CHARGEN_FORMAT_BINARY);

	return 0;
}

/*
================================================================================
Load data from a file
================================================================================
*/
void VTCharacterGen::Load(void)
{
	Flu_File_Chooser*	fc;
	unsigned char		data[12];
	int					index;

	// Check for edits to the pixmap
	if (CheckForSave())
		return;

	// Create a file chooser
	fl_cursor(FL_CURSOR_WAIT);
	fc = new Flu_File_Chooser(".", "*.fcr", 1, "Load FX-80 Character ROM");
	fl_cursor(FL_CURSOR_DEFAULT);

	// Show the file dialog
	fc->show();
	while (fc->visible())
		Fl::wait();

	// Test if a file was selected
	if (fc->value() == NULL)
	{
		delete fc;
		return;
	}

	// Check if filename was selected
	if (strlen(fc->value()) == 0)
	{
		delete fc;
		return;
	}

	// Okay, file was seleted.  Create the file as binary
	FILE*	fd = fopen(fc->value(), "rb");
	if (fd == NULL)
	{
		fl_message("Unable to open file");
		delete fc;
		return;
	}
	
	m_pCharTable->SetActiveChar(-1);
	m_ActiveChar = -1;

	// Loop through all character in the chartable
	for (index = 0; index < 256; index++)
	{
		fread(data, 1, sizeof(data), fd);
/*		if (data[0] == 0)
			data[0] = 0x80;
		else
			data[0] = 0;  */
		m_pCharTable->PutCharData(index, data);
	}
	
	// Close the file
	delete fc;
	fclose(fd);
	m_Modified = 0;
}

/*
================================================================================
Save data to a file
================================================================================
*/
void VTCharacterGen::Save(int format)
{
	Flu_File_Chooser*	fc;
	unsigned char		data[12];
	int					index, len, c;
	char				ch;
	char				ext[5];
	MString				filename;
	const char*			pLineStr;
	int					lineNo;

	// Save any edits to the Pixmap
	SaveActiveChar();

	// Create a file chooser
	if (format == CHARGEN_FORMAT_BINARY)
	{
		fl_cursor(FL_CURSOR_WAIT);
		fc = new Flu_File_Chooser(".", "*.fcr", Fl_File_Chooser::CREATE, 
			"Save FX-80 Character ROM");
		fl_cursor(FL_CURSOR_DEFAULT);
		strcpy(ext, ".fcr");
	}
	else if (format == CHARGEN_FORMAT_C)
	{
		fl_cursor(FL_CURSOR_WAIT);
		fc = new Flu_File_Chooser(".", "*.c", Fl_File_Chooser::CREATE, 
			"Save FX-80 Character ROM");
		fl_cursor(FL_CURSOR_DEFAULT);
		strcpy(ext, ".c");
	}
	else if (format == CHARGEN_FORMAT_BASIC)
	{
		fl_cursor(FL_CURSOR_WAIT);
		fc = new Flu_File_Chooser(".", "*.BA", Fl_File_Chooser::CREATE, 
			"Save FX-80 Character ROM");
		fl_cursor(FL_CURSOR_DEFAULT);
		strcpy(ext, ".ba");
	}

	// Loop until a file selected or cancel
	while (TRUE)
	{
		// Show the file dialog
		fc->show();
		while (fc->visible())
			Fl::wait();

		// Test if a file was selected
		if (fc->value() == NULL)
		{
			delete fc;
			return;
		}

		// Check if filename was selected
		if (strlen(fc->value()) == 0)
		{
			delete fc;
			return;
		}

		// Check if the file already exists and ask to overwrite
        // Check if file already exists and ask to overwrite or not
        // Try to open the ouput file
        FILE* fd = fopen(fc->value(), "rb+");
        if (fd != NULL)
        {
            // File already exists! Check for overwrite!
            fclose(fd);

            int c = fl_choice("Overwrite existing file?", "Cancel", "Yes", "No");
            if (c == 2)
                continue;
            if (c == 0)
            {
                delete fc;
                return;
            }
        }
		break;
	}

	// Test if filename has an extension
	filename = fc->value();
	len = filename.GetLength();
	for (c = len - 1; c >= 0; c--)
	{	
		ch = filename[c];
		if ((ch == '.') || (ch == '/') || (ch == '\\'))
			break;
	}
	if (ch != '.')
		filename += ext;

	// Okay, file was seleted.  Create the file as binary
	FILE*	fd = fopen((const char *) filename, "wb");
	if (fd == NULL)
	{
		delete fc;
		return;
	}
	
	// If output is C, write C variable name to file
	if (format == CHARGEN_FORMAT_C)
	{
		fprintf(fd, "unsigned char fx80_rom[256][12] = {\n");
	}
	else if (format == CHARGEN_FORMAT_BASIC)
	{
		pLineStr = fl_input("Please specify starting BASIC line", "");
		if (pLineStr != NULL)
			lineNo = atoi(pLineStr);
		else
			lineNo = 1000;
	}

	// Loop through all character in the chartable
	for (index = 0; index < 256; index++)
	{
		// Get the data from the CharTable
		m_pCharTable->GetCharData(index, data);
	
		// Now write the data to the file
		if (format == CHARGEN_FORMAT_BINARY)
		{
			fwrite(data, 1, sizeof(data), fd);
		}
		
		// Output C formatted data
		else if (format == CHARGEN_FORMAT_C)
		{
			fprintf(fd, "\t{ ");
			for (c = 0; c < 12; c++)
			{
				fprintf(fd, "0x%02X", data[c]);
				if (c != 11)
					fprintf(fd, ", ");
			}
			fprintf(fd, " }");

			// Print line ending
			if (index != 255)
				fprintf(fd, ",\t// Char 0x%02X\n", index);
			else
				fprintf(fd, "\t// Char 0x%02X\n", index);
		}

		// Output BASIC formatted data
		else if (format == CHARGEN_FORMAT_BASIC)
		{
			fprintf(fd, "%d DATA ", lineNo++);
			for (c = 0; c < 12; c++)
			{
				fprintf(fd, "%d", data[c]);
				if (c != 11)
					fprintf(fd, ", ");
			}
			fprintf(fd, "\n");
		}
	}

	if (format == CHARGEN_FORMAT_C)
	{
		fprintf(fd, "};\n");
	}

	// Close the file
	delete fc;
	fclose(fd);
	m_Modified = 0;
}

char	pixelData[24*18/8];
/*
================================================================================
Updates the Pica view
================================================================================
*/
void VTCharacterGen::UpdatePicaView(void)
{
	int		c, i;
	char	byte;
	
	// Clear the pixelData map
	for (c = 0; c < sizeof(m_PicaPixelData); c++)
		m_PicaPixelData[c] = 0;

	// Fill in pixel
	for (i = 0; i < 9; i++)
		for (c = 0; c < 11; c++)
		{
			byte = 0;
			if (m_Dots[c][i])
				byte = 1 << (c % 8);
			m_PicaPixelData[i * 6 + (c / 8)] |= byte;
		}
	if (m_pExPica->image() != NULL)
		delete m_pExPica->image();
	m_pExPica->image(new Fl_Bitmap(m_PicaPixelData, 24, 18));
	m_pExPica->redraw();
}

/*
================================================================================
Updates the Expanded view
================================================================================
*/
void VTCharacterGen::UpdateExpandView(void)
{
	int		c, i;
	char	byte;
	
	// Clear the pixelData map
	for (c = 0; c < sizeof(m_ExpandPixelData); c++)
		m_ExpandPixelData[c] = 0;

	// Fill in pixel
	for (i = 0; i < 9; i++)
		for (c = 0; c < 11; c++)
		{
			byte = 0;
			if (m_Dots[c][i])
				byte = 3 << ((c % 4) * 2);
			m_ExpandPixelData[i * 6 + (c / 4)] |= byte;
		}
	if (m_pExExpand->image() != NULL)
		delete m_pExExpand->image();
	m_pExExpand->image(new Fl_Bitmap(m_ExpandPixelData, 24, 18));
	m_pExExpand->redraw();
}

/*
================================================================================
Updates the Enahanced view
================================================================================
*/
void VTCharacterGen::UpdateEnhanceView(void)
{
	int		c, i;
	char	byte;
	
	// Clear the pixelData map
	for (c = 0; c < sizeof(m_EnhancePixelData); c++)
		m_EnhancePixelData[c] = 0;

	// Fill in pixel
	for (i = 0; i < 9; i++)
		for (c = 0; c < 11; c++)
		{
			byte = 0;
			if (m_Dots[c][i])
				byte = 3 << (c % 8);
			m_EnhancePixelData[i * 6 + (c / 8)] |= byte;

			// Check if pixel crossed a byte boundry
			if (byte && (c % 8) == 7)
			{
				m_EnhancePixelData[i * 6 + (c / 8) + 1] |= 1;
			}
		}
	if (m_pExEnhance->image() != NULL)
		delete m_pExEnhance->image();
	m_pExEnhance->image(new Fl_Bitmap(m_EnhancePixelData, 24, 18));
	m_pExEnhance->redraw();
}

/*
================================================================================
Updates the DblStrike view
================================================================================
*/
void VTCharacterGen::UpdateDblStrikeView(void)
{
	int		c, i;
	char	byte;
	
	// Clear the pixelData map
	for (c = 0; c < sizeof(m_DblStrikePixelData); c++)
		m_DblStrikePixelData[c] = 0;

	// Fill in pixel
	for (i = 0; i < 9; i++)
		for (c = 0; c < 11; c++)
		{
			byte = 0;
			if (m_Dots[c][i])
				byte = 1 << (c % 8);
			m_DblStrikePixelData[i * 6 + (c / 8)] |= byte;
			m_DblStrikePixelData[i * 6 + (c / 8) + 3] |= byte;
		}
	if (m_pExDblStrike->image() != NULL)
		delete m_pExDblStrike->image();
	m_pExDblStrike->image(new Fl_Bitmap(m_DblStrikePixelData, 24, 18));
	m_pExDblStrike->redraw();
}

/*
================================================================================
Updates the Double Strike Enahanced view
================================================================================
*/
void VTCharacterGen::UpdateDblEnhanceView(void)
{
	int		c, i;
	char	byte;
	
	// Clear the pixelData map
	for (c = 0; c < sizeof(m_DblEnhancePixelData); c++)
		m_DblEnhancePixelData[c] = 0;

	// Fill in pixel
	for (i = 0; i < 9; i++)
		for (c = 0; c < 11; c++)
		{
			byte = 0;
			if (m_Dots[c][i])
				byte = 3 << (c % 8);
			m_DblEnhancePixelData[i * 6 + (c / 8)] |= byte;
			m_DblEnhancePixelData[i * 6 + (c / 8)+3] |= byte;

			// Check if pixel crossed a byte boundry
			if (byte && (c % 8) == 7)
			{
				m_DblEnhancePixelData[i * 6 + (c / 8) + 1] |= 1;
				m_DblEnhancePixelData[i * 6 + (c / 8) + 4] |= 1;
			}
		}
	if (m_pExDblEnhance->image() != NULL)
		delete m_pExDblEnhance->image();
	m_pExDblEnhance->image(new Fl_Bitmap(m_DblEnhancePixelData, 24, 18));
	m_pExDblEnhance->redraw();
}

/*
================================================================================
Updates all previews with current data in m_Dots.
================================================================================
*/
void VTCharacterGen::UpdatePreviews(void)
{
	UpdatePicaView();
	UpdateExpandView();
	UpdateEnhanceView();
	UpdateDblStrikeView();
	UpdateDblEnhanceView();
}

/*
================================================================================
Class to draw the full 256 character table.
================================================================================
*/
VTCharTable::VTCharTable(int x, int y, int w, int h, const char *title)
	: Fl_Box(FL_FRAME_BOX, x, y, w, h, title) 
{
	int		c, i;
	int		j;

	j = 0;
	for (c = 0; c < 256; c++)
		for (i = 0; i < 12; i++)
			m_Data[c][i] = 0;

	m_ActiveChar = -1;
}

/*
================================================================================
Draw a single character in the table.
================================================================================
*/
void VTCharTable::DrawChar(int index)
{
	int		i, j, xp, yp;
	int		bk_color;

	if (index == m_ActiveChar)
		bk_color = fl_rgb_color(255, 255, 0);
	else
		bk_color = FL_GRAY;

	for (j = 1; j < 12; j++)
	{
		xp = x() + index % 32 * 15 + j + 2;
		yp = y() + (index / 32) * 22 + 3;
		
		// If lower 8 "pins" are used, then increment y
		if (!m_Data[index][0])
		{
			fl_color(bk_color);
			fl_point(xp, yp++);
			fl_point(xp, yp++);
		}

		// Print each dot as 2 verticle pixels
		for (i = 7; i >= 0; i--)
		{
			if (m_Data[index][j] & (0x01 << i))
				fl_color(FL_BLACK);
			else
				fl_color(bk_color);

			fl_point(xp, yp++);
			fl_point(xp, yp++);
		}

		if (m_Data[index][0])
		{
			fl_color(bk_color);
			fl_point(xp, yp++);
			fl_point(xp, yp++);
		}
	}
}

/*
================================================================================
Get data for the specified character
================================================================================
*/
void VTCharTable::GetCharData(int index, unsigned char *data)
{
	int	c;

	for (c = 0; c < 12; c++)
		*(data+c) = m_Data[index][c];
}

/*
================================================================================
Put data for the specified character
================================================================================
*/
void VTCharTable::PutCharData(int index, unsigned char *data)
{
	int	c;

	for (c = 0; c < 12; c++)
		m_Data[index][c] = *(data+c) ;

	redraw();
}

/*
================================================================================
Set the active character.
================================================================================
*/
void VTCharTable::SetActiveChar(int index)
{
	if ((index < -1) || (index > 255))
		return;

	m_ActiveChar = index;

	redraw();
}

/*
================================================================================
Draw the entire table, including grid.
================================================================================
*/
void VTCharTable::draw(void)
{
	int		i;

	Fl_Box::draw();

	// Draw gird
	fl_color(fl_rgb_color(128,128,128));
	for (i = 1; i < 32; i++)
		fl_line(x()+i*15, y(), x()+i*15, y()+h());
	for (i = 1; i < 8; i++)
		fl_line(x(), y()+i*22, x()+w(), y()+i*22);
	
	// Draw the characters
	for (i = 0; i < 256; i++)
	{
		DrawChar(i);
	}
}

