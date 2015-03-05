/* multieditwin.cpp */

/* $Id: multieditwin.cpp,v 1.7 2015/03/03 01:51:44 kpettit1 Exp $ */

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

#include "FLU/Flu_File_Chooser.h"

#include "VirtualT.h"
#include "m100emu.h"
#include "multieditwin.h"
#include "highlight.h"
#include "idetabs.h"

extern MString	gRootpath;
extern int		gDisableHl;

//Editor colors
extern Fl_Color hl_plain;
extern Fl_Color hl_linecomment;
extern Fl_Color hl_blockcomment ;
extern Fl_Color hl_string ;
extern Fl_Color hl_directive ;
extern Fl_Color hl_type ;
extern Fl_Color hl_keyword ;
extern Fl_Color hl_character ;
extern Fl_Color hl_label ;
extern Fl_Color background_color ;

My_Text_Display::Style_Table_Entry
 gStyleTable[] = {	// Style table
		     { hl_plain,      FL_COURIER,        TEXTSIZE }, // A - Plain
		     { hl_linecomment, FL_COURIER_ITALIC, TEXTSIZE }, // B - Line comments
		     { hl_blockcomment, FL_COURIER_ITALIC, TEXTSIZE }, // C - Block comments
		     { hl_string,       FL_COURIER,        TEXTSIZE }, // D - Strings
		     { hl_directive,   FL_COURIER,        TEXTSIZE }, // E - Directives
		     { hl_type,   FL_COURIER_BOLD,   TEXTSIZE }, // F - Types
		     { hl_keyword,       FL_COURIER_BOLD,   TEXTSIZE }, // G - Keywords
		     { hl_character,    FL_COURIER,        TEXTSIZE },  // H - Character
		     { hl_label,    FL_COURIER,        TEXTSIZE }  // H - Character
		   };


IMPLEMENT_DYNCREATE(Fl_Multi_Edit_Window, VTObject)

void multiEditModCB(int pos, int nInserted, int nDeleted, int nRestyled, 
	const char* deletedText, void* cbArg)
{
	Fl_Multi_Edit_Window* mw = (Fl_Multi_Edit_Window *) cbArg;

	mw->ModifedCB(pos, nInserted, nDeleted, nRestyled, deletedText);
}

void cb_multieditwin(Fl_Widget* w, void *args)
{
	Fl_Multi_Edit_Window* mw = (Fl_Multi_Edit_Window*) w;
	int					ans;
	int					event;

	event = Fl::event();

	if ((args != (void *) FL_IDE_TABS_CLOSE) && (event != FL_HIDE))
		return;

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
	mw->parent()->do_callback(mw, FL_IDE_TABS_CLOSE);
//	delete mw;
}

void Fl_Multi_Edit_Window::buffer(My_Text_Buffer* buf)
{
	DisableHl();
	if (m_pHlCtrl != NULL)
		m_pHlCtrl->textbuf = buf;
	My_Text_Editor::buffer(buf);
	m_tb = buf;
	if (!gDisableHl && m_pHlCtrl != NULL)
		EnableHl();
}

Fl_Multi_Edit_Window::Fl_Multi_Edit_Window(int x, int y, int w, int h, const char* title)
 : My_Text_Editor(x, y, w, h, title)
{
    /* Create window */
	m_pHlCtrl = NULL;

    m_tb = new My_Text_Buffer();
	buffer(m_tb);
	m_tb->add_modify_callback(multiEditModCB, this);

	m_pHlCtrl = new HighlightCtrl_t;
	if (m_pHlCtrl != NULL)
	{
		m_pHlCtrl->textbuf = m_tb;
		m_pHlCtrl->te = this;
		m_pHlCtrl->cppfile = TRUE;
		m_pHlCtrl->stylebuf = NULL;
	}

	mStyleTable = gStyleTable;
	mNStyles = sizeof(gStyleTable) / sizeof(My_Text_Display::Style_Table_Entry);
	m_tb->add_modify_callback(style_update, m_pHlCtrl);

	if (!gDisableHl)
	{
		style_init(m_pHlCtrl);
		mStyleBuffer = m_pHlCtrl->stylebuf;
	}

	callback(cb_multieditwin);
	when(FL_WHEN_RELEASE);

	textfont(FL_COURIER);
#ifndef WIN32
	textsize(14);
#endif
	m_Modified = 0;
	m_TabChange = 0;
	m_Title = title;
	this->label((const char *) m_Title);
}


Fl_Multi_Edit_Window::~Fl_Multi_Edit_Window()
{
	// Free the Highlight control struct
	if (m_pHlCtrl != NULL)
	{
		if (m_pHlCtrl->stylebuf != NULL)
			delete m_pHlCtrl->stylebuf;

		delete m_pHlCtrl;
	}

	// Delete the text buffer
/*	if (m_tb != NULL)
	{
		delete m_tb;
		m_tb = NULL;
	} */
}

void Fl_Multi_Edit_Window::DisableHl(void)
{
	if (mStyleBuffer == NULL)
		return;

	mStyleBuffer = NULL;
	if (m_pHlCtrl != NULL)
	{
		if (m_pHlCtrl->stylebuf != NULL)
			delete m_pHlCtrl->stylebuf;
		m_pHlCtrl->stylebuf = NULL;
	}
}

void Fl_Multi_Edit_Window::EnableHl(void)
{
	if (mStyleBuffer != NULL)
		return;

	style_init(m_pHlCtrl);
	mStyleBuffer = m_pHlCtrl->stylebuf;
}

/*
===============================================================
Routine to draw the border and title bar of the window
===============================================================
*/
void Fl_Multi_Edit_Window::OpenFile(const MString& filename)
{
    MString ext;

    // Show the Disassembling text to indicate activity
    if (filename != "")
    {
        m_tb->canUndo(0);
        m_tb->loadfile((const char *) filename);
        m_tb->canUndo(1);
        m_FileName = filename;
        ext = filename.Right(4);
        ext.MakeLower();
        if (ext != ".asm" && ext != ".inc" && ext != ".lkr" && ext != ".lst")
            DisableHl();
    }

	if (m_Title.Right(1) == '*')
		m_Title = m_Title.Left(m_Title.GetLength()-1);

	label((const char *) m_Title);
    if (parent() != NULL)
	    parent()->redraw();
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
        if (parent() != NULL)
		    parent()->redraw();
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
	Flu_File_Chooser*		fc;
	int						count;

	// Validate the Text Buffer is valid
	if (m_tb == NULL)
		return;

	fl_cursor(FL_CURSOR_WAIT);
	fc = new Flu_File_Chooser((const char *) rootpath, "*.asm,*.a85", Fl_File_Chooser::CREATE, "Save File As");
	fl_cursor(FL_CURSOR_DEFAULT);
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

	m_FileName = fc->value();
	m_tb->savefile((const char *) m_FileName);

	MString title = fl_filename_name((const char *) m_FileName);
	title = (char *) "Disassembler - " + title;
	label((const char *) title);

	delete fc;
	m_Modified = 0;
}

/*
===============================================================
Routine to handle modifications to the text buffer
===============================================================
*/
void Fl_Multi_Edit_Window::tab_distance(int size)
{
	m_TabChange = 1;
	buffer()->tab_distance(size);
	m_TabChange = 0;
}

/*
===============================================================
Routine to handle modifications to the text buffer
===============================================================
*/
void Fl_Multi_Edit_Window::ModifedCB(int pos, int nInserted, int nDeleted, 
	int nRestyled, const char* deletedText)
{
	if ((nInserted == 0) && (nDeleted == 0))
		return;

	// Check if buffer had already been modified before
	if (!m_TabChange)
	{
		// Update the window title to append a '*'
		if (m_Title.Right(1) != '*')
		{
			m_Title += '*';
			label((const char *) m_Title);

			m_Modified = 1;
		}
	}

    if (parent() != NULL)
        parent()->redraw();
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
	insert_position(0);

	// Loop while text found
	for (int found = 1; found; )
	{
		int pos = insert_position();
		found = m_tb->search_forward(pos, pFind, &pos);

		if (found)
		{
			replacement = 1;
			// Found a match...update with new text
			m_tb->select(pos, pos+strlen(pFind));
			m_tb->remove_selection();
			m_tb->insert(pos, pReplace);
			m_tb->select(pos, pos+strlen(pReplace));
			insert_position(pos+strlen(pReplace));
			show_insert_position();
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
	insert_position(0);

	// Search for the pFind text
	int pos = insert_position();
	int found = m_tb->search_forward(pos, pFind, &pos);

	if (found)
	{
		replacement = 1;
		// Found a match...update with new text
		m_tb->select(pos, pos+strlen(pFind));
		m_tb->remove_selection();
		m_tb->insert(pos, pReplace);
		m_tb->select(pos, pos+strlen(pReplace));
		insert_position(pos+strlen(pReplace));
		show_insert_position();
	}

	return replacement;
}

int Fl_Multi_Edit_Window::ForwardSearch(const char *pFind, int caseSensitive)
{
	int pos = insert_position();
	int found = m_tb->search_forward(pos, pFind, &pos, caseSensitive);
	if (found)
	{
		insert_position(pos+strlen(pFind));
		show_insert_position();
		take_focus();
		m_tb->select(pos, pos+strlen(pFind));
	}
	else
		return FALSE;

	return TRUE;
}

#if 0
void Fl_Multi_Edit_Window::show(void)
{
	My_Text_Display::show();
	redraw();
}
#endif
