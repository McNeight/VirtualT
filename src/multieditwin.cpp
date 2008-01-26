/* multieditwin.cpp */

/* $Id: multieditwin.cpp,v 1.1.1.1 2007/04/05 06:46:12 kpettit1 Exp $ */

/*
 * Copyright 2007 Ken Pettit
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
#include <FL/Fl_Double_Window.H>
#include <FL/Fl_Menu_Item.H>
#include <FL/Fl_Menu_Bar.H>
#include <FL/Fl_Menu_Window.H>
#include <FL/Fl_File_Chooser.H>
#include <FL/fl_ask.H>
#include <stdio.h>
#include <string.h>
#include <ctype.h>

#include "VirtualT.h"
#include "m100emu.h"
#include "multieditwin.h"

extern MString	gRootpath;

IMPLEMENT_DYNCREATE(Fl_Multi_Edit_Window, Fl_Multi_Window)

void multiEditModCB(int pos, int nInserted, int nDeleted, int nRestyled, 
	const char* deletedText, void* cbArg)
{
	Fl_Multi_Edit_Window* mw = (Fl_Multi_Edit_Window *) cbArg;

	mw->ModifedCB(pos, nInserted, nDeleted, nRestyled, deletedText);
}

void cb_multieditwin(Fl_Widget* w, void *)
{
	Fl_Multi_Edit_Window* mw = (Fl_Multi_Edit_Window*) w;
	int					ans;

	// Check if buffer is modified & ask to close if it is
	if (mw->IsModified())
	{
		ans = fl_choice("Save file before closing?", "Cancel", "Yes", "No");
		if (ans == 0)
			return;
		if (ans == 1)
			mw->SaveFile(gRootpath);
	}
	
	mw->hide();
	delete mw;
}

Fl_Multi_Edit_Window::Fl_Multi_Edit_Window(int x, int y, int w, int h, const char* title)
: Fl_Multi_Window(x, y, w, h, title)
{
    /* Create window */
    m_te = new Fl_Text_Editor(15, 0, ClientArea()->w()-15,
        ClientArea()->h(), (const char *) title);
    m_tb = new Fl_Text_Buffer();
	m_tb->add_modify_callback(multiEditModCB, this);
    m_te->buffer(m_tb);
    m_te->textfont(FL_COURIER);
    m_te->end();
    ClientArea()->resizable(m_te);
	m_Modified = 0;
	m_Title = title;
}

Fl_Multi_Edit_Window::~Fl_Multi_Edit_Window()
{
}

/*
===============================================================
Routine to draw the border and title bar of the window
===============================================================
*/
void Fl_Multi_Edit_Window::OpenFile(const MString& filename)
{

    // Show the Disassembling text to indicate activity
    if (filename != "")
    {
        m_tb->loadfile((const char *) filename);
        m_FileName = filename;
    }

	if (m_Title.Right(1) == '*')
		m_Title = m_Title.Left(m_Title.GetLength()-1);

	label((const char *) m_Title);
	redraw();
	m_Modified = 0;
}

/*
===============================================================
Routine to draw the border and title bar of the window
===============================================================
*/
void Fl_Multi_Edit_Window::SaveFile(const MString& rootpath)
{
	// Validate the Text Buffer is valid
	if (m_tb == NULL)
		return;

    // Check if we have a valid filename. If not, to a save as
    if (m_FileName == "")
    {
		SaveAs(rootpath);
		return;
	}

	m_tb->savefile((const char *) m_FileName);

	// Check if buffer had been modified
	if (m_Modified)
	{
		m_Title = m_Title.Left(m_Title.GetLength()-1);
		label((const char *) m_Title);
		redraw();
	}

	m_Modified = 0;
}

/*
===============================================================
Routine to draw the border and title bar of the window
===============================================================
*/
void Fl_Multi_Edit_Window::SaveAs(const MString& rootpath)
{
	Fl_File_Chooser*		fc;
	int						count;

	// Validate the Text Buffer is valid
	if (m_tb == NULL)
		return;

	fc = new Fl_File_Chooser((const char *) rootpath, "*.asm,*.a85", Fl_File_Chooser::CREATE, "Save File As");
	fc->preview(0);
	fc->show();

	while (fc->visible())
		Fl::wait();

	// Determine if a filename was selected
	count = fc->count();
	if (count == 0)
	{
		delete fc;
		return;
	}

	m_FileName = fc->value(1);
	m_tb->savefile((const char *) m_FileName);

	delete fc;
	m_Modified = 0;
}

/*
===============================================================
Routine to handle modifications to the text buffer
===============================================================
*/
void Fl_Multi_Edit_Window::ModifedCB(int pos, int nInserted, int nDeleted, 
	int nRestyled, const char* deletedText)
{
	// Check if buffer had already been modified before
	if (m_Modified)
		return;

	// Update the window title to append a '*'
	if (m_Title.Right(1) == '*')
		return;

	m_Title += '*';
	label((const char *) m_Title);
	redraw();
	
	m_Modified = 1;
}

/*
================================================================================
OkToClose:	Overridden function to test if buffer is modified
================================================================================
*/
int Fl_Multi_Edit_Window::OkToClose(void)
{
	int		ans;

	// Check if buffer is modified & ask to close if it is
	if (IsModified())
	{
		ans = fl_choice("Save file before closing?", "Cancel", "Yes", "No");
		if (ans == 0)
			return FALSE;
		if (ans == 1)
			SaveFile(gRootpath);
	}
	
	return TRUE;
}

/*
===============================================================
This routine updates the title of the Edit Window
===============================================================
*/
void Fl_Multi_Edit_Window::Title(const MString& title)
{
	m_Title = title;

	label((const char *) m_Title);
	redraw();
}

/*
===============================================================
This routine performs a find and replace all on the buffer
===============================================================
*/
int Fl_Multi_Edit_Window::ReplaceAll(const char* pFind, 
	const char * pReplace)
{
	int		replacement = 0;

	// Start at the beginning of the buffer
	m_te->insert_position(0);

	// Loop while text found
	for (int found = 1; found; )
	{
		int pos = m_te->insert_position();
		found = m_tb->search_forward(pos, pFind, &pos);

		if (found)
		{
			replacement = 1;
			// Found a match...update with new text
			m_tb->select(pos, pos+strlen(pFind));
			m_tb->remove_selection();
			m_tb->insert(pos, pReplace);
			m_tb->select(pos, pos+strlen(pReplace));
			m_te->insert_position(pos+strlen(pReplace));
			m_te->show_insert_position();
		}
	}

	return replacement;
}

/*
===============================================================
This routine performs a find and replace the first occurance
of pFind with pReplace
===============================================================
*/
int Fl_Multi_Edit_Window::ReplaceNext(const char* pFind, 
	const char * pReplace)
{
	int		replacement = 0;

	// Start at the beginning of the buffer
	m_te->insert_position(0);

	// Search for the pFind text
	int pos = m_te->insert_position();
	int found = m_tb->search_forward(pos, pFind, &pos);

	if (found)
	{
		replacement = 1;
		// Found a match...update with new text
		m_tb->select(pos, pos+strlen(pFind));
		m_tb->remove_selection();
		m_tb->insert(pos, pReplace);
		m_tb->select(pos, pos+strlen(pReplace));
		m_te->insert_position(pos+strlen(pReplace));
		m_te->show_insert_position();
	}

	return replacement;
}

