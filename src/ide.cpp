/* ide.cpp */

/* $Id: ide.cpp,v 1.1.1.1 2004/08/05 06:46:12 kpettit1 Exp $ */

/*
 * Copyright 2006 Ken Pettit
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
#include <FL/Fl_Menu_Item.H>
#include <FL/Fl_Menu_Bar.H>
#include <FL/Fl_Menu_Window.H>
#include <FL/Fl_Tile.H>
#include <FL/Fl_Box.H>
#include <FL/Fl_Tabs.H>
#include <FL/Fl_Text_Editor.H>
#include <FL/Fl_Text_Buffer.H>
#include "FLU/Flu_Tree_Browser.h"
#include "FLU/flu_pixmaps.h"
#include <stdio.h>
#include <string.h>
#include <ctype.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <errno.h>
#ifdef _WIN32
#include <direct.h>
#endif

#include "VirtualT.h"
#include "m100emu.h"
#include "ide.h"
#include "multieditwin.h"
#include "disassemble.h"
#include "cpuregs.h"
#include "memedit.h"
#include "periph.h"
#include "MStringArray.h"
#include "assemble.h"

VT_Ide *		gpIde = 0;
Fl_Menu_Button*	gPopup = 0;
void setMonitorWindow(Fl_Window* pWin);
Fl_Multi_Window*	gpLcd;

#ifdef _WIN32
extern "C"
#endif
extern char path[];

MString		gRootpath;

extern Fl_Preferences	virtualt_prefs;

void close_cb(Fl_Widget* w, void*);
void cb_help(Fl_Widget* w, void*);
void cb_about(Fl_Widget* w, void*);
void cb_lcd_display(Fl_Widget* w, void*);
// Define File Manu callbacks
void cb_new_file(Fl_Widget* w, void*);
void cb_open_file(Fl_Widget* w, void*);
void cb_save_file(Fl_Widget* w, void*);
void cb_save_as(Fl_Widget* w, void*);
void cb_new_project(Fl_Widget* w, void*);
void cb_open_project(Fl_Widget* w, void*);
void cb_save_project(Fl_Widget* w, void*);
// Define Edit menu callbacks
void cb_copy(Fl_Widget* w, void*);
void cb_cut(Fl_Widget* w, void*);
void cb_paste(Fl_Widget* w, void*);
void cb_find(Fl_Widget* w, void*);
void cb_find_next(Fl_Widget* w, void*);
void cb_replace(Fl_Widget* w, void*);
// Define Window management routines
void cb_window_cascade(Fl_Widget* w, void*);
void cb_window_1(Fl_Widget* w, void*);
void cb_window_2(Fl_Widget* w, void*);
void cb_window_3(Fl_Widget* w, void*);
void cb_window_4(Fl_Widget* w, void*);
void cb_window_5(Fl_Widget* w, void*);
void cb_window_6(Fl_Widget* w, void*);
void cb_window_more(Fl_Widget* w, void*);
// Define project management routines
void cb_build_project(Fl_Widget* w, void*);
void cb_rebuild_project(Fl_Widget* w, void*);
void cb_clean_project(Fl_Widget* w, void*);
void cb_project_settings(Fl_Widget* w, void*);

IMPLEMENT_DYNCREATE(VT_IdeGroup, VTObject);
IMPLEMENT_DYNCREATE(VT_IdeSource, VTObject);

char gProjectTypes[][6] = {
	".co",
	".obj",
	".rom",
	".ba",
	""
};

/*
=======================================================
Menu Items for the IDE window
=======================================================
*/
Fl_Menu_Item gIde_menuitems[] = {
  { "&File", 0, 0, 0, FL_SUBMENU },
	{ "New File",			FL_CTRL + 'n',		cb_new_file, 0, 0 },
	{ "Open File...",		FL_CTRL + 'o',		cb_open_file, 0, 0 },
	{ "Save File",			FL_CTRL + 's',		cb_save_file, 0, 0},
	{ "Save As...",			0,		cb_save_as, 0, FL_MENU_DIVIDER },
	{ "New Project...",		0,		cb_new_project, 0, 0 },
	{ "Open Project...",	0,		cb_open_project, 0, 0 },
	{ "Save Project",		0,		cb_save_project, 0, FL_MENU_DIVIDER },
	{ "E&xit",				FL_CTRL + 'q', close_cb, 0 },
	{ 0 },
  { "&Edit", 0, 0, 0, FL_SUBMENU },
	{ "Copy",				FL_CTRL + 'c',		cb_copy, 0, 0 },
	{ "Cut",				FL_CTRL + 'x',		cb_cut, 0, 0 },
	{ "Paste",				FL_CTRL + 'v',		cb_paste, 0, FL_MENU_DIVIDER},
	{ "Find...",			FL_CTRL + 'f',		cb_find, 0, 0 },
	{ "Find Next",			FL_F + 3,			cb_find_next, 0, 0 },
	{ "Replace",			FL_CTRL + 'r',		cb_replace, 0, 0 },
	{ 0 },
  { "&Project", 0, 0, 0, FL_SUBMENU },
	{ "Build",				FL_SHIFT + FL_F+8,		cb_build_project, 0, 0 },
	{ "Rebuild",			0,		cb_rebuild_project, 0, 0 },
	{ "Clean",				0,		cb_clean_project, 0, FL_MENU_DIVIDER },
	{ "Settings",			0,		cb_project_settings, 0, 0 },
	{ 0 },
  { "&Debug", 0, 0, 0, FL_SUBMENU },
	{ "Laptop Display",			0,		cb_lcd_display, 0, 0 },
	{ 0 },
  { "&Tools", 0, 0, 0, FL_SUBMENU },
	{ "CPU Registers",         0, cb_CpuRegs },
	{ "Disassembler",          0, disassembler_cb },
	{ "Memory Editor",         0, cb_MemoryEditor },
	{ "Peripheral Devices",    0, cb_PeripheralDevices },
	{ "Simulation Log Viewer", 0, 0 },
	{ "Model T File Viewer",   0, 0 },
	{ 0 },
  { "&Window", 0, 0, 0, FL_SUBMENU },
	{ "Cascade",			0,		cb_window_cascade, 0, 0 },
	{ "1",	 				0,		cb_window_1, 0, 0 },
	{ "2",	 				0,		cb_window_2, 0, 0 },
	{ "3",	 				0,		cb_window_3, 0, 0 },
	{ "4",	 				0,		cb_window_4, 0, 0 },
	{ "5",	 				0,		cb_window_5, 0, 0 },
	{ "6",	 				0,		cb_window_6, 0, 0 },
	{ "More...",			0,		cb_window_more, 0, 0 },
	{ 0 },
  { "&Help", 0, 0, 0, FL_SUBMENU },
	{ "Help", 0, cb_help },
	{ "About VirtualT", 0, cb_about },
	{ 0 },

  { 0 }
};

void cb_new_folder(Fl_Widget* w, void*);
void cb_add_files_to_folder(Fl_Widget* w, void*);
void cb_folder_properties(Fl_Widget* w, void*);
void cb_open_tree_file(Fl_Widget* w, void*);
void cb_assemble_tree_file(Fl_Widget* w, void*);
void cb_tree_file_properties(Fl_Widget* w, void*);

Fl_Menu_Item gGroupMenu[] = {
	{ "New Folder",			0,		cb_new_folder, 0, 0 },
	{ "Add Files to Folder",0,		cb_add_files_to_folder, 0, 0 },
	{ "Properties",			0,		cb_folder_properties, 0, 0},
	{ 0 }
};

Fl_Menu_Item gSourceMenu[] = {
	{ "Open",				0,		cb_open_tree_file, 0, 0 },
	{ "Assemble File",		0,		cb_assemble_tree_file, 0, 0 },
	{ "Properties",			0,		cb_tree_file_properties, 0, 0},
	{ 0 }
};

Fl_Menu_Item gRootMenu[] = {
	{ "Build",					0,		cb_build_project, 0, 0 },
	{ "Build All",				0,		cb_rebuild_project, 0, 0 },
	{ "New Folder...",			0,		cb_new_folder, 0, 0},
	{ "Add Files to Project...",	0,		cb_add_files_to_folder, 0, FL_MENU_DIVIDER },
	{ "Project Settings...",	0,		cb_project_settings, 0, 0},
	{ 0 }
};

Fl_Pixmap gTextDoc( textdoc_xpm ), gComputer( computer_xpm );

/*
=======================================================
Callback routine for the close box of the IDE window
=======================================================
*/
void close_ide_cb(Fl_Widget* w, void*)
{
	int				ans;
	MString			question;

	if (gpIde != NULL)
	{
		// Check if project is dirty
		if (gpIde->ProjectDirty())
		{
			// Ask if project should be saved
			question.Format("Save changes to project %s?", (const char *) gpIde->ProjectName());
			ans = fl_choice((const char *) question, "Cancel", "Yes", "No");
			if (ans == 0)
				return;
			if (ans == 1)
				gpIde->SaveProject();
		}

		// Save window parameters to preferences
		virtualt_prefs.set("IdeX", gpIde->x());
		virtualt_prefs.set("IdeY", gpIde->y());
		virtualt_prefs.set("IdeW", gpIde->w());
		virtualt_prefs.set("IdeH", gpIde->h());

		// Okay, close the window
		setMonitorWindow(0);
		gpIde->hide();
		delete gpIde;
		gpIde = 0;
		gPopup = 0;
		gpLcd = 0;
	}
}

/*
=======================================================
Callback routine for creating new files
=======================================================
*/
void cb_new_file(Fl_Widget* w, void*)
{
	if (gpIde != NULL)
	{
		gpIde->NewFile();
	}
}

/*
=======================================================
Callback routine for opening files
=======================================================
*/
void cb_open_file(Fl_Widget* w, void*)
{
	if (gpIde != NULL)
		gpIde->OpenFile();
}

/*
=======================================================
Callback routine for saving files
=======================================================
*/
void cb_save_file(Fl_Widget* w, void*)
{
	if (gpIde != NULL)
		gpIde->SaveFile();
}

/*
=======================================================
Callback routine for save as
=======================================================
*/
void cb_save_as(Fl_Widget* w, void*)
{
	if (gpIde != NULL)
		gpIde->SaveAs();
}

/*
=======================================================
Callback routine for creating new project
=======================================================
*/
void cb_new_project(Fl_Widget* w, void*)
{
	if (gpIde != NULL)
		gpIde->NewProject();
}

/*
=======================================================
Callback routine for opening a project
=======================================================
*/
void cb_open_project(Fl_Widget* w, void*)
{
	if (gpIde != NULL)
		gpIde->OpenProject();
}

/*
=======================================================
Callback routine for saving current project
=======================================================
*/
void cb_save_project(Fl_Widget* w, void*)
{
	if (gpIde != NULL)
		gpIde->SaveProject();
}

/*
=======================================================
Callback routine for copying text
=======================================================
*/
void cb_copy(Fl_Widget* w, void*)
{
	if (gpIde != NULL)
		gpIde->Copy();
}

/*
=======================================================
Callback routine for cutting text
=======================================================
*/
void cb_cut(Fl_Widget* w, void*)
{
	if (gpIde != NULL)
		gpIde->Cut();
}

/*
=======================================================
Callback routine for pasting text
=======================================================
*/
void cb_paste(Fl_Widget* w, void*)
{
	if (gpIde != NULL)
		gpIde->Paste();
}

/*
=======================================================
Callback routine for finding text
=======================================================
*/
void cb_find(Fl_Widget* w, void*)
{
	if (gpIde != NULL)
		gpIde->Find();
}

/*
=======================================================
Callback routine for finding next text
=======================================================
*/
void cb_find_next(Fl_Widget* w, void*)
{
	if (gpIde != NULL)
		gpIde->FindNext();
}

/*
=======================================================
Callback routine for replacing text
=======================================================
*/
void cb_replace(Fl_Widget* w, void*)
{
	if (gpIde != NULL)
		gpIde->Replace();
}

void cb_close_lcd(Fl_Widget* w, void*)
{
	setMonitorWindow(0);
	delete gpLcd;
	gpLcd = 0;
}

/*
=======================================================
Callback routine for displaying the laptop screen
=======================================================
*/
void cb_lcd_display(Fl_Widget* w, void*)
{
	if (gpLcd != 0)
	{
		gpIde->m_EditWindow->insert(*gpLcd, gpIde->m_EditWindow->children());
		gpLcd->show();
		gpLcd->show();
		gpLcd->redraw();
		return;
	}
	if (gModel == MODEL_T200)
		gpLcd = new Fl_Multi_Window(0, 0, 240*2 + MW_BORDER_WIDTH*2, 128*2 +
			MW_BORDER_WIDTH + MW_TITLE_HEIGHT, "Laptop Display");
	else
		gpLcd = new Fl_Multi_Window(0, 0, 240*2 + MW_BORDER_WIDTH*2, 64*2 +
			MW_BORDER_WIDTH + MW_TITLE_HEIGHT, "Laptop Display");

	gpLcd->m_NoResize = 1;
	gpLcd->callback(cb_close_lcd);
	gpLcd->end();
	gpIde->m_EditWindow->insert(*gpLcd, gpIde->m_EditWindow->children());
	gpLcd->show();
	gpLcd->show();
	gpLcd->take_focus();
	setMonitorWindow(gpLcd->ClientArea());
}

/*
=======================================================
Callback routine to create new folder in Project Tree
=======================================================
*/
void cb_new_folder(Fl_Widget* w, void* data)
{
	Flu_Tree_Browser::Node* n = (Flu_Tree_Browser::Node*) data;

	// Ensure window exists
	if (gpIde != 0)
		gpIde->NewFolder(n);
}

/*
=======================================================
Callback routine to add files to Project Tree Group
=======================================================
*/
void cb_add_files_to_folder(Fl_Widget* w, void* data)
{
	Flu_Tree_Browser::Node* n = (Flu_Tree_Browser::Node*) data;

	// Ensure window exists
	if (gpIde != 0)
		gpIde->AddFilesToFolder(n);
}

/*
=======================================================
Callback routine to display Tree folder properties
=======================================================
*/
void cb_folder_properties(Fl_Widget* w, void* data)
{
	Flu_Tree_Browser::Node* n = (Flu_Tree_Browser::Node*) data;

	// Ensure window exists
	if (gpIde != 0)
		gpIde->FolderProperties(n);
}

/*
=======================================================
Callback routine to open selected tree file
=======================================================
*/
void cb_open_tree_file(Fl_Widget* w, void* data)
{
	Flu_Tree_Browser::Node* n = (Flu_Tree_Browser::Node*) data;

	// Ensure window exists
	if (gpIde != 0)
		gpIde->OpenTreeFile(n);
}

/*
=======================================================
Callback routine to assemble selected tree file
=======================================================
*/
void cb_assemble_tree_file(Fl_Widget* w, void* data)
{
	Flu_Tree_Browser::Node* n = (Flu_Tree_Browser::Node*) data;

	// Ensure window exists
	if (gpIde != 0)
		gpIde->AssembleTreeFile(n);
}

/*
=======================================================
Callback routine to display selected tree file properties
=======================================================
*/
void cb_tree_file_properties(Fl_Widget* w, void* data)
{
	Flu_Tree_Browser::Node* n = (Flu_Tree_Browser::Node*) data;

	// knsure window exists
	if (gpIde != 0)
		gpIde->TreeFileProperties(n);
}

/*
=======================================================
Callback routine to build the project
=======================================================
*/
void cb_build_project(Fl_Widget* w, void*)
{
	// Call the routine to build the project
	if (gpIde != NULL)
		gpIde->BuildProject();
}

/*
=======================================================
Callback routine to build all project files
=======================================================
*/
void cb_rebuild_project(Fl_Widget* w, void*)
{
	// Rebuild is a clean followed by a build
	cb_clean_project(w, NULL);
	cb_build_project(w, NULL);
}

/*
=======================================================
Callback routine to display project settings
=======================================================
*/
void cb_project_settings(Fl_Widget* w, void*)
{
	// Display the project settings
	if (gpIde != NULL)
		gpIde->ShowProjectSettings();
}

/*
=======================================================
Callback routine to clean the active project
=======================================================
*/
void cb_clean_project(Fl_Widget* w, void*)
{
	// Delete all output files for this project
	if (gpIde != NULL)
		gpIde->CleanProject();
}

/*
=======================================================
Callback routine to cascade all windows
=======================================================
*/
void cb_window_cascade(Fl_Widget* w, void*)
{
}

/*
=======================================================
Callback routine to select window 1
=======================================================
*/
void cb_window_1(Fl_Widget* w, void*)
{
}

/*
=======================================================
Callback routine to select window 2
=======================================================
*/
void cb_window_2(Fl_Widget* w, void*)
{
}

/*
=======================================================
Callback routine to select window 3
=======================================================
*/
void cb_window_3(Fl_Widget* w, void*)
{
}

/*
=======================================================
Callback routine to select window 4
=======================================================
*/
void cb_window_4(Fl_Widget* w, void*)
{
}

/*
=======================================================
Callback routine to select window 5
=======================================================
*/
void cb_window_5(Fl_Widget* w, void*)
{
}

/*
=======================================================
Callback routine to select window 6
=======================================================
*/
void cb_window_6(Fl_Widget* w, void*)
{
}

/*
=======================================================
Callback routine to display more windows
=======================================================
*/
void cb_window_more(Fl_Widget* w, void*)
{
}

/*
=======================================================
Menu Item Callbacks
=======================================================
*/
void cb_Ide(Fl_Widget* widget, void*) 
{
	int		x, y, w, h;

	if (gpIde == NULL)
	{
		// Get the initial window size from the user preferences
		virtualt_prefs.get("IdeX", x, 40);
		virtualt_prefs.get("IdeY", y, 40);
		virtualt_prefs.get("IdeW", w, 800);
		virtualt_prefs.get("IdeH", h, 600);

		// Create a new window for the IDE workspace
		gpIde = new VT_Ide(x, y, w, h , "Integrated Development Environment - Work in progress!!!");
		gpIde->show();
	}

}

void projtree_callback( Fl_Widget* w, void* )
{
  Flu_Tree_Browser *t = (Flu_Tree_Browser*)w;

  int reason = t->callback_reason();
  Flu_Tree_Browser::Node *n = t->callback_node();

  switch( reason )
    {
    case FLU_HILIGHTED:
      break;

    case FLU_UNHILIGHTED:
      break;

    case FLU_SELECTED:
/*	  if ((t1 != 0) && (t2 != 0))
	  {
		  MString str = n->label();
		  if (strcmp((const char *) str, "Header Files") == 0)
		  {
			  gpIde->m_EditWindow->insert(*t1, 0);
			  t1->hide();
			  t1->show();
			  t1->take_focus();
		  }
		  else
		  {
			  gpIde->m_EditWindow->insert(*t2, 0);
			  t2->hide();
			  t2->take_focus();
			  t2->show();
		  }
	  }
*/
      break;

    case FLU_UNSELECTED:
      break;

    case FLU_OPENED:
      break;

    case FLU_CLOSED:
      break;

    case FLU_DOUBLE_CLICK:
	  if (gpIde != 0)
		  gpIde->OpenTreeFile(n);
      break;

    case FLU_WIDGET_CALLBACK:
		printf("Flu WIDGET callback\n");
		break;
      break;

	case FLU_RIGHT_CLICK:
		gpIde->RightClick(n);
		break;

    case FLU_MOVED_NODE:
      break;

    case FLU_NEW_NODE:
      break;

	case FLU_DELETE:
		gpIde->DeleteItem(n);
		break;
    }
}


/*
==============================================================================
VT_Ide constructor.  This routine creates a new VT_Ide window
==============================================================================
*/
VT_Ide::VT_Ide(int x, int y, int w, int h, const char *title)
: Fl_Window(x, y, w, h, title)
{
	// Parent window has no box, only child regions
	box(FL_NO_BOX);
	callback(close_ide_cb);

	// Get the window size from the preferences

	// Create a menu for the new window.
	Fl_Menu_Bar *m = new Fl_Menu_Bar(0, 0, w, MENU_HEIGHT-2);
	m->menu(gIde_menuitems);

	// Create a tiled window to support Project, Edit, and debug regions
	Fl_Tile* tile = new Fl_Tile(0,MENU_HEIGHT-2,w,h-MENU_HEIGHT);

	/*
	============================================
	Create region for Project tree
	============================================
	*/
	m_ProjWindow = new Fl_Window(2,MENU_HEIGHT-2,198,h-170,"");
	m_ProjWindow->box(FL_DOWN_BOX);

	// Create Tree control
	m_ProjTree = new Flu_Tree_Browser( 0, 0, 198, h-170 );
	m_ProjTree->box( FL_DOWN_FRAME );
	m_ProjTree->callback( projtree_callback );
	m_ProjTree->selection_mode( FLU_SINGLE_SELECT );
	gPopup = new Fl_Menu_Button(0,0,100,400,"Popup");
	gPopup->type(Fl_Menu_Button::POPUP3);
	gPopup->menu(gGroupMenu);
	gPopup->selection_color(FL_LIGHT3);
	gPopup->hide();
	m_ProjTree->hide();
	
	m_ProjWindow->resizable(m_ProjTree);
	m_ProjWindow->end();

	/*
	=================================================
	Create region and Child Window for editing files
	=================================================
	*/
	m_EditWindow = new Fl_Window(200,MENU_HEIGHT-2,w-200,h-170,"Edit");
	m_EditWindow->box(FL_DOWN_BOX);
	m_EditWindow->color(FL_DARK2);
	m_EditWindow->end();

	/*
	=================================================
	Create region for Debug and output tabs
	=================================================
	*/
	m_TabWindow = new Fl_Window(0,MENU_HEIGHT-2+h-170,w,170-MENU_HEIGHT+2,"Tab");
	m_TabWindow->box(FL_DOWN_BOX);
	m_TabWindow->color(FL_LIGHT1);

	// Create a tab control
	m_Tabs = new Fl_Tabs(0, 1, w, 170-MENU_HEIGHT);

	/*
	====================
	Create build tab
	====================
	*/
	m_BuildTab = new Fl_Group(2, 0, w-3, 170-MENU_HEIGHT-20, " Build ");
	m_BuildTab->box(FL_DOWN_BOX);
	m_BuildTab->selection_color(FL_WHITE);
	m_BuildTab->color(FL_WHITE);

	// Create a Text Editor to show the disassembled text
	m_BuildTextDisp = new Fl_Text_Display(2, 0, w-3, 170-MENU_HEIGHT-20);
	m_BuildTextDisp->box(FL_DOWN_BOX);
	m_BuildTextDisp->selection_color(FL_WHITE);
	m_BuildTextDisp->color(FL_WHITE);

	// Create a Text Buffer for the Text Editor to work with
	m_BuildTextBuf = new Fl_Text_Buffer();
	m_BuildTextDisp->buffer(m_BuildTextBuf);

	// Show the Disassembling text to indicate activity
	m_BuildTextDisp->textfont(FL_COURIER);
	m_BuildTextDisp->end();
	m_BuildTab->end();
	
	/*
	====================
	Create Debug tab
	====================
	*/
	m_DebugTab = new Fl_Group(2, 0, w-3, 170-MENU_HEIGHT-20, " Debug ");
	m_DebugTab->box(FL_DOWN_BOX);
	m_DebugTab->selection_color(FL_WHITE);
	m_DebugTab->color(FL_WHITE);
	m_DebugTab->end();
	
	/*
	====================
	Create watch tab
	====================
	*/
	m_WatchTab = new Fl_Group(2, 0, w-3, 170-MENU_HEIGHT-20, " Watch ");
	m_WatchTab->box(FL_NO_BOX);
	m_WatchTab->selection_color(FL_WHITE);
	m_WatchTab->color(FL_WHITE);

	// Create tiled window for auto and watch variables
	Fl_Tile* tile2 = new Fl_Tile(2, 0,w,170-MENU_HEIGHT-20);

	Fl_Box* box0 = new Fl_Box(2, 0,w/2-2,170-MENU_HEIGHT-20,"1");
	box0->box(FL_DOWN_BOX);
	box0->color(FL_BACKGROUND_COLOR);
	box0->labelsize(36);
	box0->align(FL_ALIGN_CLIP);
	Fl_Box* box1 = new Fl_Box(w/2, 0,w/2,170-MENU_HEIGHT-20,"2");
	box1->box(FL_DOWN_BOX);
	box1->color(FL_BACKGROUND_COLOR);
	box1->labelsize(36);
	box1->align(FL_ALIGN_CLIP);
	Fl_Box* r2 = new Fl_Box(0,0,w,170-MENU_HEIGHT-20);
	tile2->resizable(r2);
	tile2->end();
	m_WatchTab->resizable(tile2);
	m_WatchTab->end();
	
	m_Tabs->end();

	m_TabWindow->resizable(m_Tabs);
	m_TabWindow->end();

	// Set resize region
	Fl_Box* r = new Fl_Box(150,MENU_HEIGHT-2,w-150,h-170);
	tile->resizable(r);
	tile->end();
	resizable(m);
	resizable(tile);

	// End the Window
	end();

	// Create a replace dialog
	m_pReplaceDlg = new VT_ReplaceDlg(this);
	m_pFindDlg = new VT_FindDlg(this);

	// Initialize other variables
	m_ActivePrj = 0;
	m_LastDir = ".";
	m_OpenLocation = 0;
	gRootpath = path;
}

/*
=============================================================
Override the default show function to display the children
windows
=============================================================
*/
void VT_Ide::show()
{
	Fl_Window::show();
	m_EditWindow->show();
	m_TabWindow->show();
	m_BuildTextDisp->show_cursor();
}

/*
=============================================================
Override the default handle function to handle key sequences.
=============================================================
*/
int VT_Ide::handle(int event)
{
	int			key;

	switch (event)
	{
	case FL_KEYDOWN:
		key = Fl::event_key();
		switch (key)
		{
		case FL_Escape:
			return 1;

		default:
			// Process all other keys using default processing
			return Fl_Window::handle(event);
		}
		break;
	
	default:
		// Handle all other events using default processing
		return Fl_Window::handle(event);
		break;
	}

	return 0;
}

/*
=============================================================
Override the default draw routine to display white regions
here and there
=============================================================
*/
void VT_Ide::draw(void)
{
	// Color for outer border
	fl_color(FL_WHITE);

	// Draw border along the top
	fl_rectf(0,0,2,h());

	Fl_Window::draw();
}

/*
=============================================================
NewFile routine handles the File->New File menu item.
This routine creates an empty file with no filename.  The
title of the window will be Text1, Text2, etc.
=============================================================
*/
void VT_Ide::NewFile(void)
{
	int					seq;			// Next sequence number
	MString				title;			// Tile of new file
	int					children, child;
	Fl_Widget*			pWidget;

	// Get number of child windows in the Ide
	children = m_EditWindow->children();

	for (seq = 1; ; seq++)
	{
		// Try next sequence number ot see if window already exists
		title.Format("Text%d", seq);

		for (child = 0; child < children; child++)
		{
			pWidget = (Fl_Widget*) m_EditWindow->child(child);
			if (strcmp((const char *) title, pWidget->label()) == 0)
				break;
		}

		// Check if title already exists
		if (child == children)
		{
			// Okay, now create a new window
			NewEditWindow(title, "");
			break;
		}
	}
}

/*
=============================================================
SaveFile routine handles the File->Save File menu item.
This routine performs a save operation if the active edit
window has a filename associated with it, or a SaveAs
if it doesn't.
=============================================================
*/
void VT_Ide::SaveFile(void)
{
	Fl_Multi_Edit_Window*	mw;
	int						children;
	MString					rootpath;
	MString					title;

	// First get a pointer to the active (topmost) window
	children = m_EditWindow->children();
	mw = (Fl_Multi_Edit_Window*) m_EditWindow->child(children-1);
	if (mw == NULL)
		return;

	// Validate this is truly a Multi_Edit_Window
	if (strcmp(mw->GetClass()->m_ClassName, "Fl_Multi_Edit_Window") != 0)
		return;

	// Determine root path
	if (m_ActivePrj == NULL)
		rootpath = path;
	else
		rootpath = m_ActivePrj->m_RootPath;

	mw->SaveFile(rootpath);
	title = MakePathRelative(mw->Filename(), rootpath);
	mw->Title(title);
}

/*
=============================================================
SaveAs routine handles the File->Save As File menu item.
This routine determines the active edit window and calls
its SaveAs function.
=============================================================
*/
void VT_Ide::SaveAs(void)
{
	Fl_Multi_Edit_Window*	mw;
	int						children;
	MString					rootpath;
	MString					title;

	// First get a pointer to the active (topmost) window
	children = m_EditWindow->children();
	mw = (Fl_Multi_Edit_Window*) m_EditWindow->child(children-1);
	if (mw == NULL)
		return;

	// Validate this is truly a Multi_Edit_Window
	if (strcmp(mw->GetClass()->m_ClassName, "Fl_Multi_Edit_Window") != 0)
		return;

	// Determine root path
	if (m_ActivePrj == NULL)
		rootpath = path;
	else
		rootpath = m_ActivePrj->m_RootPath;

	mw->SaveAs(rootpath);
	title = MakePathRelative(mw->Filename(), rootpath);
	mw->Title(title);
}

/*
=============================================================
OpenFile routine handles the File->Save File menu item.
This routine opens an existing file and creates an edit
window for the newly opened file.
=============================================================
*/
void VT_Ide::OpenFile(void)
{
	Fl_File_Chooser*		fc;
	int						count;
	MString					filename;
	MString					title;
	MString					rootpath;

	if (m_ActivePrj == NULL)
		rootpath = path;
	else
		rootpath = m_ActivePrj->m_RootPath;

	fc = new Fl_File_Chooser((const char *) path, "*.asm,*.a85", 1, "Open File");
	fc->preview(0);
	fc->show();

	// Wait for user to select a file or escape
	while (fc->visible())
		Fl::wait();

	// Determine if a file was selected or not
	count = fc->count();
	if (count == 0)
	{
		delete fc;
		return;
	}

	// Get the filename from the file chooser
	filename = fc->value(1);
	delete fc;
	title = MakePathRelative(filename, rootpath);

	// Determine if file is already open 
	int children = m_EditWindow->children();
	for (int c = 0; c < children; c++)
	{
		Fl_Widget* pWidget = (Fl_Widget*) m_EditWindow->child(c);
		if (strcmp((const char *) title, pWidget->label()) == 0)
		{
			// File already open...bring file to foreground
			return;
		}
	}

	// Create a new edit window for the file
	NewEditWindow(title, filename);
}

/*
=============================================================
Copy routine handles the Edit->Copy menu item.
The routine identifies the active window and calls the Fl 
copy routine.
=============================================================
*/
void VT_Ide::Copy(void)
{
	Fl_Multi_Edit_Window*	mw;
	int						children;

	// First get a pointer to the active (topmost) window
	children = m_EditWindow->children();
	mw = (Fl_Multi_Edit_Window*) m_EditWindow->child(children-1);
	if (mw == NULL)
		return;
	
	Fl_Text_Editor::kf_copy(0, mw->m_te);
}

/*
=============================================================
Cut routine handles the Edit->Cut menu item.
The routine identifies the active window and calls the Fl 
cut routine.
=============================================================
*/
void VT_Ide::Cut(void)
{
	Fl_Multi_Edit_Window*	mw;
	int						children;

	// First get a pointer to the active (topmost) window
	children = m_EditWindow->children();
	mw = (Fl_Multi_Edit_Window*) m_EditWindow->child(children-1);
	if (mw == NULL)
		return;
	
	Fl_Text_Editor::kf_cut(0, mw->m_te);
}

/*
=============================================================
Paste routine handles the Edit->Paste menu item.
The routine identifies the active window and calls the Fl 
cut routine.
=============================================================
*/
void VT_Ide::Paste(void)
{
	Fl_Multi_Edit_Window*	mw;
	int						children;

	// First get a pointer to the active (topmost) window
	children = m_EditWindow->children();
	mw = (Fl_Multi_Edit_Window*) m_EditWindow->child(children-1);
	if (mw == NULL)
		return;
	
	Fl_Text_Editor::kf_paste(0, mw->m_te);
}

/*
=============================================================
Find routine handles the Edit->Find menu item.
The routine identifies the active window and calls the Fl 
cut routine.
=============================================================
*/
void VT_Ide::Find(void)
{
	Fl_Multi_Edit_Window*	mw;
	int						children;

	// First get a pointer to the active (topmost) window
	children = m_EditWindow->children();
	mw = (Fl_Multi_Edit_Window*) m_EditWindow->child(children-1);
	if (mw == NULL)
		return;

	m_pFindDlg->m_pFindDlg->show();
	m_pFindDlg->m_pFind->take_focus();
}

/*
=============================================================
FindAgain routine handles the Edit->Find Next menu item.
The routine identifies the active window and calls the Fl 
cut routine.
=============================================================
*/
void VT_Ide::FindNext(void)
{
	Fl_Multi_Edit_Window*	mw;
	int						children;
	const char *			pFind;

	// First get a pointer to the active (topmost) window
	children = m_EditWindow->children();
	mw = (Fl_Multi_Edit_Window*) m_EditWindow->child(children-1);
	if (mw == NULL)
		return;

	// Ensure there is a search string 
	pFind = m_pFindDlg->m_pFind->value();
	if (pFind[0] == '\0')
	{
		m_pFindDlg->m_pFindDlg->show();
		m_pFindDlg->m_pFind->take_focus();
		return;
	}

	// Find the text

	// Hide the dialog box
	m_pFindDlg->m_pFindDlg->hide();
}

/*
=============================================================
Replace routine handles the Edit->Replace Next menu item.
The routine identifies the active window and calls the Fl 
cut routine.
=============================================================
*/
void VT_Ide::Replace(void)
{
	m_pReplaceDlg->m_pReplaceDlg->show();
	m_pReplaceDlg->m_pFind->take_focus();
}

/*
=============================================================
ReplaceAll routine is called when the RelaceAll button is 
pressed in the ReplaceDlg.
=============================================================
*/
void VT_Ide::ReplaceAll(void)
{
	Fl_Multi_Edit_Window*	mw;
	int						children;
	const char *			pFind;
	const char *			pReplace;

	// First get a pointer to the active (topmost) window
	children = m_EditWindow->children();
	mw = (Fl_Multi_Edit_Window*) m_EditWindow->child(children-1);
	if (mw == NULL)
		return;
	
	// Ensure there is a search string
	pFind = (const char *) m_pReplaceDlg->m_pFind->value();
	pReplace = (const char *) m_pReplaceDlg->m_pWith->value();
	if (pFind[0] == '\0')
	{
		m_pReplaceDlg->m_pReplaceDlg->show();
		m_pReplaceDlg->m_pFind->take_focus();
		return;
	}

	m_pReplaceDlg->m_pReplaceDlg->hide();

	// Do the replacement
	if (!mw->ReplaceAll(pFind, pReplace))
		fl_alert("No occurrences of \'%s\' found!", pFind);
}

/*
=============================================================
ReplaceNext routine is called when the RelaceNext button is 
pressed in the ReplaceDlg.
=============================================================
*/
void VT_Ide::ReplaceNext(void)
{
	Fl_Multi_Edit_Window*	mw;
	int						children;
	const char *			pFind;
	const char *			pReplace;

	// First get a pointer to the active (topmost) window
	children = m_EditWindow->children();
	mw = (Fl_Multi_Edit_Window*) m_EditWindow->child(children-1);
	if (mw == NULL)
		return;

	// Ensure there is a search string
	pFind = (const char *) m_pReplaceDlg->m_pFind->value();
	pReplace = (const char *) m_pReplaceDlg->m_pWith->value();
	if (pFind[0] == '\0')
	{
		m_pReplaceDlg->m_pReplaceDlg->show();
		m_pReplaceDlg->m_pFind->take_focus();
		return;
	}

	// Do the replacement
	if (!mw->ReplaceNext(pFind, pReplace))
		fl_alert("No occurrences of \'%s\' found!", pFind);
}

/*
=============================================================
NewProject routine handles the File->New Project menu item.
This routine creates an empty project and updates the Tree
control.
=============================================================
*/
void VT_Ide::NewProject(void)
{
	VT_IdeGroup*	pGroup;
	VT_NewProject*	pProj;
	const char *	pRootDir;
	char			fullPath[512];

	// Get project parameters
	pProj = new VT_NewProject;
	pProj->m_Dir = m_LastDir;
	pProj->show();
	
	while (pProj->visible())
		Fl::wait();

	if (pProj->m_makeProj == FALSE)
	{
		delete pProj;
		return;
	}

	// Check if there is an active project
	if (m_ActivePrj != 0)
	{
		// Check if project is dirty
		if (m_ActivePrj->m_Dirty)
		{
			int c = fl_choice("Save project changes?", "Cancel", "Yes", "No");

			if (c == 0)
			{
				delete pProj;
				return;
			}
			if (c == 1)
				SaveProject();
		}

		// Delete existing project
		delete m_ActivePrj;
		m_ActivePrj = 0;
	}

	// Create new project
	m_ActivePrj = new VT_Project;
	m_ActivePrj->m_Name = pProj->getProjName();

	pGroup = new VT_IdeGroup;
	pGroup->m_Name = "Source Files";
	pGroup->m_Filespec = "*.asm;*.a85";
	m_ActivePrj->m_Groups.Add(pGroup);

	pGroup = new VT_IdeGroup;
	pGroup->m_Name = "Header Files";
	pGroup->m_Filespec = "*.h;*.inc";
	m_ActivePrj->m_Groups.Add(pGroup);

	pGroup = new VT_IdeGroup;
	pGroup->m_Name = "Object Files";
	pGroup->m_Filespec = "*.obj";
	m_ActivePrj->m_Groups.Add(pGroup);

	// Get root directory
	pRootDir = pProj->getLocation();
	if (pRootDir[0] == '.')
	{
		strcpy(fullPath, path);
		strcat(fullPath, "/");
		strcat(fullPath, &pRootDir[2]);
		m_ActivePrj->m_RootPath = fullPath;
	}
	else
	{
		m_ActivePrj->m_RootPath = pRootDir;
	}


	// Check if path ends with '/'
	if (m_ActivePrj->m_RootPath[m_ActivePrj->m_RootPath.GetLength()-1] != '/')
		m_ActivePrj->m_RootPath = m_ActivePrj->m_RootPath + "/";

	m_ActivePrj->m_RootPath = m_ActivePrj->m_RootPath + m_ActivePrj->m_Name;
	
	// Try to create the project directory
#ifdef _WIN32
	if (_mkdir((const char *) m_ActivePrj->m_RootPath) != 0)
#else
	if (mkdir((const char *) m_ActivePrj->m_RootPath, 0755) != 0)
#endif
	{
		fl_alert("Unable to create directory %s", (const char *) m_ActivePrj->m_RootPath);
		delete pProj;
		return;
	}

	// Save the project type
	m_ActivePrj->m_ProjectType = pProj->getProjType();
	if (m_ActivePrj->m_ProjectType == 0)
		m_ActivePrj->m_AutoLoad = 1;
	else
		m_ActivePrj->m_AutoLoad = 0;

	// Save the target model
	m_ActivePrj->m_TargetModel = pProj->getTargetModel();
	gRootpath = m_ActivePrj->m_RootPath;

	// Save the new project
	SaveProject();

	BuildTreeControl();

	m_ProjTree->open_on_select(TRUE);
	m_ProjTree->redraw();
	m_ProjTree->show();

	delete pProj;
}

/*
=============================================================
The ProjectDirty routine returns TRUE if there is an active
project and the project is dirty.
=============================================================
*/
int VT_Ide::ProjectDirty(void)
{
	if (m_ActivePrj == NULL)
		return 0;

	return m_ActivePrj->m_Dirty;
}

/*
=============================================================
The ProjectName routine returns the name of he active project
or "" if no active project.
=============================================================
*/
MString VT_Ide::ProjectName(void)
{
	if (m_ActivePrj == NULL)
		return "";

	return m_ActivePrj->m_Name;
}

/*
=============================================================
The SaveProject routine saves the active project structure to
a .prj file.
=============================================================
*/
void VT_Ide::SaveProject(void)
{
	FILE*			fd;
	char			fullPath[512];
	char			model[10];
	int				count, c, objs, x;
	VT_IdeGroup*	pGroup;
	VT_IdeSource*	pObj;

	// Check if m_ActiveProj is valid
	if (m_ActivePrj == NULL)
		return;

	// Create the path
	sprintf(fullPath, "%s/%s.prj", (const char *) m_ActivePrj->m_RootPath,
		(const char *) m_ActivePrj->m_Name);

	// Try to open the file for write mode
	if ((fd = fopen(fullPath, "w+")) == NULL)
	{
		// Error opening file!!
		fl_alert("Error opening projec file for write mode!");
		return;
	}

	// Write header information
	fprintf(fd, "NAME=%s\n", (const char *) m_ActivePrj->m_Name);
	fprintf(fd, "INCLPATH=%s\n", (const char *) m_ActivePrj->m_IncludePath);
	fprintf(fd, "DEFINES=%s\n", (const char *) m_ActivePrj->m_Defines);
	fprintf(fd, "LINKPATH=%s\n", (const char *) m_ActivePrj->m_LinkPath);
	fprintf(fd, "LINKLIBS=%s\n", (const char *) m_ActivePrj->m_LinkLibs);
	fprintf(fd, "CSEG=%s\n", (const char *) m_ActivePrj->m_CodeAddr);
	fprintf(fd, "DSEG=%s\n", (const char *) m_ActivePrj->m_DataAddr);
	fprintf(fd, "ASMOPT=%s\n", (const char *) m_ActivePrj->m_AsmOptions);
	fprintf(fd, "LINKOPT=%s\n", (const char *) m_ActivePrj->m_LinkOptions);
	fprintf(fd, "TYPE=%s\n", gProjectTypes[m_ActivePrj->m_ProjectType]);
	get_model_string(model, m_ActivePrj->m_TargetModel);
	fprintf(fd, "TARGET=%s\n", model);
	fprintf(fd, "AUTOLOAD=%d\n", m_ActivePrj->m_AutoLoad);
	fprintf(fd, "UPDATEHIMEM=%d\n", m_ActivePrj->m_UpdateHIMEM);
	fprintf(fd, "\n");

	// Write group information to the file
	count = m_ActivePrj->m_Groups.GetSize();
	for (c = 0; c < count; c++)
	{
		pGroup = (VT_IdeGroup*) m_ActivePrj->m_Groups[c];
		fprintf(fd, "GROUP=%s\n", (const char *) pGroup->m_Name);
		fprintf(fd, "FILESPEC=%s\n", (const char *) pGroup->m_Filespec);

		// Write objects
		objs = pGroup->m_Objects.GetSize();
		for (x = 0; x < objs; x++)
		{
			pObj = (VT_IdeSource*) pGroup->m_Objects[x];
			fprintf(fd, "SOURCE=%s\n", (const char *) pObj->m_Name);
		}

		// End the group
		fprintf(fd, "ENDGROUP\n\n");
	}

	m_ActivePrj->m_Dirty = 0;
	
	// Close the file
	fclose(fd);
}

/*
=============================================================
The OpenProject routine handles the File->Open Project menu
item.  It presents a File Browser dialog to allow the user
to select a prj file then calls the ParsePrjFile method to
load the project.
=============================================================
*/
void VT_Ide::OpenProject(void)
{
	MString				filename;
	Fl_File_Chooser		*FileWin;
	int					count;

	FileWin = new Fl_File_Chooser("./Projects","*.prj",1,"Open Project file");
	FileWin->preview(0);
	FileWin->show();

	while (FileWin->visible())
		Fl::wait();

	count = FileWin->count();
	if (count == 0)
	{
		delete FileWin;
		return;
	}

	// Get Filename
	if (FileWin->value(1) == 0)
	{
		delete FileWin;
		return;
	}
	filename = FileWin->value(1);
	delete FileWin;

	// Check if there is an active project
	if (m_ActivePrj != 0)
	{
		// Check if project is dirty
		if (m_ActivePrj->m_Dirty)
		{
			int c = fl_choice("Save project changes?", "Cancel", "Yes", "No");

			if (c == 0)
				return;
			if (c == 1)
				SaveProject();
		}

		// Delete existing project
		delete m_ActivePrj;
		m_ActivePrj = 0;
	}

	// Try to parse the file
	if (ParsePrjFile(filename))
		BuildTreeControl();
	else
	{
		// Report parse error
	}
	gRootpath = m_ActivePrj->m_RootPath;
}

/*
=============================================================
The BuildTreeControl routine builds / rebuilds the Project
Tree control from the active Project structure.
=============================================================
*/
void VT_Ide::BuildTreeControl(void)
{
	MString						temp;
	Flu_Tree_Browser::Node*		n;
	int							len, c;
	VT_IdeGroup*				pGroup;
	VT_IdeSource*				pSource;   
	VTObject*					pObj;
	MString						fmt, addStr;

	// Ensure we have a project structure
	if (m_ActivePrj == 0)
		return;

	// set the default leaf icon to be a blue dot
	m_ProjTree->leaf_icon( &gTextDoc );
	m_ProjTree->insertion_mode( FLU_INSERT_SORTED );

	// use the default branch icons (yellow folder)
	m_ProjTree->branch_icons( NULL, NULL );

	// Clear old contents of the tree
	m_ProjTree->clear();

	// Set the Root name
	temp = m_ActivePrj->m_Name;
	temp += " files";
	m_ProjTree->label( temp );

	n = m_ProjTree->get_root();
	if( n ) 
		n->branch_icons( &gComputer, &gComputer );

	// Initalize the format for adding items
	fmt = "/%s";

	// Loop through all objects in the project
	len = m_ActivePrj->m_Groups.GetSize();
	for (c = 0; c < len; c++)
	{
		pObj = m_ActivePrj->m_Groups[c];
		const char *pStr = pObj->GetClass()->m_ClassName;
		if (strcmp(pStr, "VT_IdeGroup") == 0)
		{
			// Object is a group add node to tree
			pGroup = (VT_IdeGroup *) pObj;
			addStr.Format(fmt, (const char *) pGroup->m_Name);
			addStr += "/";
			n = m_ProjTree->add(addStr);
			if (n)
			{
				n->user_data(pGroup);
				pGroup->m_Node = n;
			}

			addStr += "%s";
			
			// Loop through all objects and add to tree
			int sublen = pGroup->m_Objects.GetSize();
			for (int x = 0; x < sublen; x++)
			{
				AddGroupToTree(pGroup->m_Objects[x], addStr);
			}

		}
		else
		{
			// Object is a source
			pSource = (VT_IdeSource *) pObj;
			
			// Get just the filename
			int index = pSource->m_Name.ReverseFind("/");
			if (index == 0)
				temp = pSource->m_Name;
			else
				temp = pSource->m_Name.Right(pSource->m_Name.GetLength() - index - 1);
			addStr.Format(fmt, (const char *) temp);

			pStr = addStr;
			n = m_ProjTree->add(addStr);
			if( n )
			{
				n->leaf_icon( &gTextDoc );
				n->user_data(pSource);
				pSource->m_Node = n;
			}
		}
	}

	m_ProjTree->open(TRUE);
	m_ProjTree->show();
}

/*
=============================================================
The AddGroupToTree routine adds a complete subgroup to the
Project Tree control.  This is a nested routine.
=============================================================
*/
void VT_Ide::AddGroupToTree(VTObject *pObj, const char *fmt)
{
	MString						temp;
	Flu_Tree_Browser::Node*		n;
	int							len, c;
	VT_IdeGroup*				pGroup;
	VT_IdeSource*				pSource;   
	MString						addStr;

	const char *pStr = pObj->GetClass()->m_ClassName;
	if (strcmp(pStr, "VT_IdeGroup") == 0)
	{
		// Object is a group add node to tree
		pGroup = (VT_IdeGroup *) pObj;
		addStr.Format(fmt, (const char *) pGroup->m_Name);
		addStr += "/";
		n = m_ProjTree->add(addStr);
		if (n)
		{
			n->user_data(pGroup);
			pGroup->m_Node = n;
		}

		addStr += "%s";
		
		// Loop through all objects and add to tree
		len = pGroup->m_Objects.GetSize();
		for (c = 0; c < len; c++)
		{
			AddGroupToTree(pGroup->m_Objects[c], addStr);
		}

	}
	else
	{
		// Object is a source
		pSource = (VT_IdeSource *) pObj;

		// Get just the filename
		int index = pSource->m_Name.ReverseFind("/");
		if (index == 0)
			temp = pSource->m_Name;
		else
			temp = pSource->m_Name.Right(pSource->m_Name.GetLength() - index - 1);
		addStr.Format(fmt, (const char *) temp);

		pStr = addStr;
		n = m_ProjTree->add(addStr);
		if( n ) 
		{
			n->leaf_icon( &gTextDoc );
			n->user_data(pSource);
			pSource->m_Node = n;
		}
	}
}

/*
=============================================================
The ParsePrjFile routine parses the specified input file and
creates a Project structure which the IDE uses for control.
=============================================================
*/
int VT_Ide::ParsePrjFile(const char *name)
{
	FILE*			fd;
	char			line[512];
	char			*param, *value;
	MString			sParam, temp;
	const char *	sPtr;
	VT_IdeGroup*	pGroup = 0;
	VT_IdeSource*	pSource = 0;
	VT_IdeGroup*	stack[100];
	VTObject*		pIns;
	int				index = 0;
	int				len, c;

	// Try to open input file
	fd = fopen(name, "r+");
	if (fd == 0)
		return 0;

	// Ensure active project not null
	if (m_ActivePrj == 0)
		m_ActivePrj = new VT_Project;

	// Loop through all lines in file
	while (fgets(line, 512, fd) != 0)
	{
		// Skip comments and blank lines
		if ((line[0] == '#') || (line[0] == '\n') || (line[0] == '\x0d'))
			continue;

		// Parse the line - find the first '=' 
		for (c = 0; (line[c] != 0) && (line[c] != '=') && (line[c] != '\n') && (line[c] != 13) ; c++)
			;
		param = line;
		if (line[c] == '=')
		{
			line[c] = 0;
			value = &line[c+1];
			for (c++; (line[c] != 0) && (line[c] != '\n') && (line[c] != 13); c++)
				;
			line[c] = 0;
		}
		else
		{
			line[c] = 0;
			value = "";
		}
		
		// Check for error in line
		if (value == 0)
		{
			// Report error
			continue;
		}

		sParam = param;
		sParam.MakeUpper();
		sPtr = (const char *) sParam;
		if (strcmp(sPtr, "SOURCE") == 0)
		{
			// Ensure there is an active group
			if (pGroup != 0)
			{
				pSource = new VT_IdeSource;
				pSource->m_Name = value;

				// Insert source alphabetically after groups
				len = pGroup->m_Objects.GetSize();
				for (c = 0; c < len; c++)
				{
					pIns = pGroup->m_Objects[c];
					// Skip groups so they appear at top
					if (strcmp(pIns->GetClass()->m_ClassName, "VT_IdeGroup") == 0)
						continue;

					if (pSource->m_Name < ((VT_IdeSource *) pIns)->m_Name)
						break;
				}
				pGroup->m_Objects.InsertAt(c, pSource);
				pSource->m_ParentGroup = pGroup;
				pSource = 0;
			}
		}
		else if (strcmp(sPtr, "GROUP") == 0)
		{
			// Check if there is an active group.  If there is, we are adding a
			// subgroup and need to "push" the pointer
			if (pGroup != 0)
			{
				// Push pointer on stack
				stack[index++] = pGroup;

				// Create new sub group item
				VT_IdeGroup* newgrp = new VT_IdeGroup;

				// Assign name to new group
				newgrp->m_Name = value;

				// Insert groups alphabetically before sources
				len = pGroup->m_Objects.GetSize();
				for (c = 0; c < len; c++)
				{
					pIns = pGroup->m_Objects[c];
					// Stop at sign of first source
					if (strcmp(pIns->GetClass()->m_ClassName, "VT_IdeSource") == 0)
						break;

					if (pSource->m_Name < ((VT_IdeGroup *) pIns)->m_Name)
						break;
				}
				pGroup->m_Objects.InsertAt(c, newgrp);
				pGroup = newgrp;
			}
			else
			{
				pGroup = new VT_IdeGroup;
				pGroup->m_Name = value;
			}
		}
		else if (strcmp(sPtr, "ENDGROUP") == 0)
		{
			// Check if ending a sub group
			if (index == 0)
			{
				// Not a subgroup, add group to project
				m_ActivePrj->m_Groups.Add(pGroup);

				// Null out the group pointer
				pGroup = 0;
			}
			else
			{
				// Pop the group from the stack
				pGroup = stack[--index];
			}
		}
		else if (strcmp(sPtr, "FILESPEC") == 0)
		{
			if ((pGroup != 0) && (value != 0))
				pGroup->m_Filespec = value;
		}
		else if (strcmp(sPtr, "NAME") == 0)
		{
			if (value != 0)
				m_ActivePrj->m_Name = value;
		}
		else if (strcmp(sPtr, "TARGET") == 0)
		{
			if (value != 0)
				m_ActivePrj->m_TargetModel = get_model_from_string(value);
		}
		else if (strcmp(sPtr, "AUTOLOAD") == 0)
		{
			if (value != 0)
				m_ActivePrj->m_AutoLoad = atoi(value);
		}
		else if (strcmp(sPtr, "UPDATEHIMEM") == 0)
		{
			if (value != 0)
				m_ActivePrj->m_UpdateHIMEM = atoi(value);
		}
		else if (strcmp(sPtr, "TYPE") == 0)
		{
			if (value != 0)
			{
				// Search for type in global array
				for (c = 0; strlen(gProjectTypes[c]) != 0; c++)
				{
					if (strcmp(gProjectTypes[c], value) == 0)
					{
						m_ActivePrj->m_ProjectType = c;
						break;
					}
				}
			}
		}
		else if (strcmp(sPtr, "INCLPATH") == 0)
		{
			if (value != 0)
				m_ActivePrj->m_IncludePath = value;
		}
		else if (strcmp(sPtr, "DEFINES") == 0)
		{
			if (value != 0)
				m_ActivePrj->m_Defines = value;
		}
		else if (strcmp(sPtr, "LINKPATH") == 0)
		{
			if (value != 0)
				m_ActivePrj->m_LinkPath = value;
		}
		else if (strcmp(sPtr, "LINKLIBS") == 0)
		{
			if (value != 0)
				m_ActivePrj->m_LinkLibs = value;
		}
		else if (strcmp(sPtr, "ASMOPT") == 0)
		{
			if (value != 0)
				m_ActivePrj->m_AsmOptions = value;
		}
		else if (strcmp(sPtr, "LINKOPT") == 0)
		{
			if (value != 0)
				m_ActivePrj->m_LinkOptions = value;
		}
		else if (strcmp(sPtr, "DSEG") == 0)
		{
			if (value != 0)
				m_ActivePrj->m_DataAddr = value;
		}
		else if (strcmp(sPtr, "CSEG") == 0)
		{
			if (value != 0)
				m_ActivePrj->m_CodeAddr = value;
		}
	}

	// Set the RootPath
	temp = name;
	// Get just the path
	index = temp.ReverseFind("/");
	if (index == 0)
		m_ActivePrj->m_RootPath = path;
	else
		m_ActivePrj->m_RootPath = temp.Left(index);

	// Close the file
	fclose(fd);
	return 1;
}

VT_Project::~VT_Project()
{
    VTObject*   pObj;

    for (int c =0; c < m_Groups.GetSize(); c++)
    {
        pObj = m_Groups[c];
        if (strcmp(pObj->GetClass()->m_ClassName, "VT_IdeGroup") == 0)
            delete (VT_IdeGroup *) pObj;
        else
            delete (VT_IdeSource *) pObj;
       }
    m_Groups.RemoveAll();
}

VT_IdeGroup::~VT_IdeGroup()
{ 
	VTObject*	pObj;

	for (int c =0; c < m_Objects.GetSize(); c++) 
	{
		pObj = m_Objects[c];
		if (strcmp(pObj->GetClass()->m_ClassName, "VT_IdeGroup") == 0)
			delete (VT_IdeGroup *) pObj;
		else
			delete (VT_IdeSource *) pObj;
	}

	m_Objects.RemoveAll(); 
}

/*
=============================================================
The delete routine is called when the user presses delete on
a specific item in the tree.
=============================================================
*/
void VT_Ide::DeleteItem(Flu_Tree_Browser::Node* n)
{
	int			ans;
	MString		question;

	if (!n->is_root())
	{
		// Get user data from the node
		VTObject* pObj = (VTObject *) n->user_data();
		if (pObj == NULL)
			return;

		// Determine if node is a group or a leaf (source)
		if (strcmp(pObj->GetClass()->m_ClassName, "VT_IdeGroup") == 0)
		{
			// For groups, ask before deleting the entire group
			VT_IdeGroup* pGroup = (VT_IdeGroup*) pObj;
			question.Format("Delete group %s and all its members?", (const char *) pGroup->m_Name);
			ans = fl_choice((const char *) question, "No", "Yes", NULL);
			if (ans == 1)
			{
				// First delete all subitems from the group
				int count = pGroup->m_Objects.GetSize();
				while (count != 0)
				{
					VT_IdeSource* pSrc = (VT_IdeSource*) pGroup->m_Objects[0];
					count--;

					// Delete the tree item
					pGroup->m_Objects.RemoveAt(0, 1);
					m_ProjTree->remove(pSrc->m_Node);
		
					// Delete the object
					delete pSrc;
				}

				// Now delete the group - tree node first
				m_ProjTree->remove(pGroup->m_Node);

				// Now find the group entry in the active project
				count = m_ActivePrj->m_Groups.GetSize();
				for (int c = 0; c < count; c++)
				{
					if (pGroup == (VT_IdeGroup*) m_ActivePrj->m_Groups[c])
					{
						m_ActivePrj->m_Groups.RemoveAt(c, 1);
						break;
					}
				}
				delete pGroup;

				// Mark the project dirty
				m_ActivePrj->m_Dirty = 1;
			}
		}
		else
		{
			VT_IdeSource* pSrc = (VT_IdeSource*) pObj;

			// Delete the source from the tree
			m_ProjTree->remove(pSrc->m_Node);

			// Delete the source from the parent Group
			int count = pSrc->m_ParentGroup->m_Objects.GetSize();
			for (int c = 0; c < count; c++)
			{
				if (pSrc == (VT_IdeSource*) pSrc->m_ParentGroup->m_Objects[c])
				{
					pSrc->m_ParentGroup->m_Objects.RemoveAt(c, 1);
					break;
				}
			}
			delete pSrc;

			// Mark the project dirty
			m_ActivePrj->m_Dirty = 1;
		}
	}
}

/*
=============================================================
The RightClick routine is called when the user performs a 
RightClick operation on one of the items in the projec tree.
The routine displays a pop-up menu
=============================================================
*/
void VT_Ide::RightClick(Flu_Tree_Browser::Node* n)
{
	if (n->is_root())
	{
		gPopup->menu(gRootMenu);
	}
	else
	{
		// Get user data from the node
		VTObject* pObj = (VTObject *) n->user_data();
		if (pObj == NULL)
			return;

		// Determine if node is a group or a leaf (source)
		if (strcmp(pObj->GetClass()->m_ClassName, "VT_IdeGroup") == 0)
		{
			// Select group popup menu
			gGroupMenu[0].user_data_ = n;
			gGroupMenu[1].user_data_ = n;
			gGroupMenu[2].user_data_ = n;
			gPopup->menu(gGroupMenu);
		}
		else
		{
			// Select source popup menu
			gSourceMenu[0].user_data_ = n;
			gSourceMenu[1].user_data_ = n;
			gSourceMenu[2].user_data_ = n;
			gPopup->menu(gSourceMenu);
		}
	}
	// Add label to popup menu & display
	gPopup->label(n->label());
	gPopup->popup();
}

/*
=============================================================
The NewFolder routine is called when the user selects Add New
Folder from the popup menu.  This routine asks the user for
the name of the new folder and adds it to both the active
project and to the tree control.
=============================================================
*/
void VT_Ide::NewFolder(Flu_Tree_Browser::Node* n)
{
	MString			name, filespec;
	VT_IdeGroup*	pGroup;
	VT_IdeGroup*	pNewGrp;
	int				len, c, root;
	Flu_Tree_Browser::Node*	i;
	Flu_Tree_Browser::Node* newNode;
	const char*			pName;

	// Get Folder name
	pName = fl_input("Please enter Folder Name", "");
	if (pName == NULL)
		return;
	if (strlen(pName) == 0)
		return;
	name = pName;
	filespec = "";

	// Create new VT_IdeGroup object for this folder
	pNewGrp = new VT_IdeGroup;
	pNewGrp->m_Name = name;
	pNewGrp->m_Filespec = filespec;

	// Convert name to a branch
	name += "/";

	// Test if inserting at root
	if (n == 0)
	{
		root = 1;
		n = m_ProjTree->get_root();
	}
	else
		root = 0;

	if (root)
	{
		len = m_ActivePrj->m_Groups.GetSize();
		for (c = 0; c < len; c++)
		{
			// Stop when we encounter a source
			if (strcmp(m_ActivePrj->m_Groups[c]->GetClass()->m_ClassName, "VT_IdeSource") == 0)
			{
				i = ((VT_IdeSource *) m_ActivePrj->m_Groups[c])->m_Node;
				break;
			}
			
			if (name < ((VT_IdeGroup*) m_ActivePrj->m_Groups[c])->m_Name)
			{
				i = ((VT_IdeGroup *) m_ActivePrj->m_Groups[c])->m_Node;
				break;
			}
		}
	}
	else
	{
		// Seach group object for insertion point for folder 
		pGroup = (VT_IdeGroup *) n->user_data();
		if (pGroup == NULL)
			return;
		len = pGroup->m_Objects.GetSize();
		for (c = 0; c < len; c++)
		{
			// Stop when we encounter a source
			if (strcmp(pGroup->m_Objects[c]->GetClass()->m_ClassName, "VT_IdeSource") == 0)
			{
				i = ((VT_IdeSource *) pGroup->m_Objects[c])->m_Node;
				break;
			}
			
			if (name < ((VT_IdeGroup*)pGroup->m_Objects[c])->m_Name)
			{
				i = ((VT_IdeGroup *) pGroup->m_Objects[c])->m_Node;
				break;
			}
		}
	}

	// Test if adding at end of list
	if (c == len)
		newNode = m_ProjTree->insert_at(n, 0, name);
	else
		newNode = m_ProjTree->insert_at(n, i, name);

	newNode->user_data(pNewGrp);
	pNewGrp->m_Node = newNode;

	// Add new node to active project
	if (root)
	{
		if (c == len)
			m_ActivePrj->m_Groups.Add(pNewGrp);
		else
			m_ActivePrj->m_Groups.InsertAt(c, pNewGrp);
	}
	else
	{
		if (c == len)
			pGroup->m_Objects.Add(pNewGrp);
		else
			pGroup->m_Objects.InsertAt(c, pNewGrp);
	}

	// Set Dirty flag
	m_ActivePrj->m_Dirty = 1;
}

/*
=============================================================
The AddFilesToFolder routine is called when the user selects
Add Files to Project from the popup menu.  The routine asks
the user which files should be added then adds them to the
requested folder.
=============================================================
*/
void VT_Ide::AddFilesToFolder(Flu_Tree_Browser::Node* n)
{
	Fl_File_Chooser		*fc;		/* File Chooser */
	int					count, c;
	int					len, index, x;
	MString 			filename, relPath;
	MString				temp, path;
	MStringArray		errFiles;
	VT_IdeGroup*		pGroup = 0;
	VT_IdeSource*		pSource = 0;
	VTObject*			pIns;

	/* Create a new File Chooser object */
	fc = new Fl_File_Chooser((const char *) m_LastDir, "Source Files (*.{asm,inc,h})", 
		Fl_File_Chooser::MULTI, "Add Files to Project");
	fc->preview(0);
	fc->show();

	/* Wait until the files have been choosen or they hit cancel */
	while (fc->visible())
		Fl::wait();

	/* Get count of files selected */
	count = fc->count();

	/* Check if cancel was pressed & return if it was */
	if (count == 0)
	{
		delete fc;
		return;
	}

	// Get the Group item associated with this tree node
	pGroup = (VT_IdeGroup*) n->user_data();

	// Get the current directory and save for future use
	m_LastDir = fc->directory();

	// Calculate relative path for files

	/* Loop through all files selected and try to add to project */
	for (c = 0; c < count; c++)
	{
		// Get next filename from the FileChooser
		filename = fc->value(c+1);

		// Get just the filename
		index = filename.ReverseFind('/');
		if (index == 0)
			temp = filename;
		else
			temp = filename.Right(filename.GetLength() - index - 1);

		// Check if entry aleady exists
		if (n->find((const char *) temp) != NULL)
		{
			// Add file to list of "error" files
			errFiles.Add(temp);
			continue;
		}

		// Create an IdeSource object for this entry
		pSource = new VT_IdeSource;
		pSource->m_Name = m_LastDir + '/' + temp;

		// Insert source alphabetically after groups
		len = pGroup->m_Objects.GetSize();
		for (x = 0; x < len; x++)
		{
			pIns = pGroup->m_Objects[x];
			// Skip groups so they appear at top
			if (strcmp(pIns->GetClass()->m_ClassName, "VT_IdeGroup") == 0)
				continue;

			if (pSource->m_Name < ((VT_IdeSource *) pIns)->m_Name)
				break;
		}
		pGroup->m_Objects.InsertAt(x, pSource);
		pSource->m_ParentGroup = pGroup;

		// Insert item in tree
		pSource->m_Node = n->add((const char *) temp);
		if (pSource->m_Node == NULL)
			printf("Error inserting node!\n");
		else
			pSource->m_Node->user_data(pSource);
		
		m_ActivePrj->m_Dirty = 1;
	}

	/* Delete the file chooser */
	delete fc;
	return;
}

void VT_Ide::FolderProperties(Flu_Tree_Browser::Node* n)
{
}

void VT_Ide::OpenTreeFile(Flu_Tree_Browser::Node* n)
{
	VT_IdeSource*	pSource;
	MString			file;
	MString			title;
	int				children, c;
	int				count = 6;
	Fl_Widget*		pWidget;

	pSource = (VT_IdeSource *) n->user_data();
	if (pSource == NULL)
		return;

	// Process the open request
	if (pSource->m_Name.Left(2) == "./")
		file = pSource->m_Name.Right(pSource->m_Name.GetLength()-2);
	else
		file = pSource->m_Name;
	title = MakePathRelative(file, m_ActivePrj->m_RootPath);

	// Check if the file is already open
	children = m_EditWindow->children();
	for (c = 0; c < children; c++)
	{
		pWidget = (Fl_Widget*) m_EditWindow->child(c);
		if (strcmp((const char *) title, pWidget->label()) == 0)
		{
			// File already open...bring file to foreground
			return;
		}
	}

	file = MakePathAbsolute(file, m_ActivePrj->m_RootPath);
	NewEditWindow(title, file);
}

/*
===============================================================================
NewEditWindow:	This routine creates a new edit window with the specified 
				title.  If the File parameter is not empty, then the edit
				window is loaded from the file.
===============================================================================
*/
void VT_Ide::NewEditWindow(const MString& title, const MString& file)
{
	int 			x = 1;
	int 			y = 1;
	int 			w = 400;
	int 			h = 300;
	int				count = 6;

	/* Calculate location of next window */
	if (m_EditWindow->h() < 550)
		count = 5;
	h = m_EditWindow->h() - 28 * count;
	w = m_EditWindow->w() - 28 * count;
	x = y = m_OpenLocation++ * 28;
	if (m_OpenLocation >= count)
		m_OpenLocation = 0;

	/* Create window */
	Fl_Multi_Edit_Window* mw = new Fl_Multi_Edit_Window(x, y, w, h, (const char *) title);
	if (file != "")
		mw->OpenFile((const char *) file);
	mw->end();

	// Insert new window in EditWindow
	Fl_Widget* prev = m_EditWindow->child(m_EditWindow->children()-1);
	m_EditWindow->insert(*mw, m_EditWindow->children());
	mw->show();

	// Redraw current window to show it as inactive
	if (prev != 0)
		prev->redraw();
	// Call show again to bring window to front
	mw->show();
	mw->take_focus();
}

void VT_Ide::AssembleTreeFile(Flu_Tree_Browser::Node* n)
{
}

void VT_Ide::TreeFileProperties(Flu_Tree_Browser::Node* n)
{
}

/*
=============================================================================
BuildProjet:  This routine is the reason for all this mess!  Try to assemble
				each file in the project and then link if no errors.
=============================================================================
*/
void VT_Ide::BuildProject(void)
{
	int				groups, sources;
	int				c, x, err;
	int				index;
	VT_IdeGroup*	pGroup;
	VT_IdeSource*	pSource;
	int				errorCount, totalErrors=0;
	MString			text, temp;
	VTAssembler		assembler;
	int				assemblyNeeded;
	MStringArray	errors;
	MString			filename;

	// Be sure we have an active project
	if (m_ActivePrj == NULL)
		return;

	// Clear the Build tab at the bottom
	m_BuildTextBuf ->remove(0, m_BuildTextBuf->length());

	// Configure the assembler
	assembler.SetRootPath(m_ActivePrj->m_RootPath);
	assembler.SetAsmOptions(m_ActivePrj->m_AsmOptions);
	assembler.SetIncludeDirs(m_ActivePrj->m_IncludePath);
	assembler.SetDefines(m_ActivePrj->m_Defines);

	m_BuildTextBuf->append("Assembling...\n");

	// Loop through each group and look for files to assemble
	errorCount = 0;
	groups = m_ActivePrj->m_Groups.GetSize();
	for (c = 0; c < groups; c++)
	{
		// Get group
		pGroup = (VT_IdeGroup*) m_ActivePrj->m_Groups[c];

		// Loop through each source in group
		sources = pGroup->m_Objects.GetSize();
		for (x = 0; x < sources; x++)
		{
			// Get next source from group
			pSource = (VT_IdeSource*) pGroup->m_Objects[x];

			// Get extension of this source 

			// Test if source is .asm or .a85
			assemblyNeeded = FALSE;
			temp = pSource->m_Name.Right(4);
			temp.MakeLower();
			if ((temp == ".asm") || (temp == ".a85"))
			{
				// Check source dependencies

				assemblyNeeded = TRUE;

				// Delete old .obj file
			}

			// Try to assemble the file
			if (assemblyNeeded)
			{
				// Display build indication
				index = pSource->m_Name.ReverseFind('/');
				temp = pSource->m_Name.Right(pSource->m_Name.GetLength()-index-1);
				text.Format("%s\n", (const char *) temp);
				m_BuildTextBuf->append((const char *) text);

				// Try to assemble this file
				filename = MakePathAbsolute(pSource->m_Name, m_ActivePrj->m_RootPath);
				assembler.Parse(filename);

				// Check if any errors occurred & report them
				errors = assembler.GetErrors();
				errorCount = errors.GetSize();
				totalErrors += errorCount;
				for (err = 0; err < errorCount; err++)
				{
					m_BuildTextBuf->append((const char *) errors[err]);
					m_BuildTextBuf->append("\n");
				}
			}
		}
	}

	// Check if there were any erros during assembly and if not, 
	// invoke the linker
	if (totalErrors != 0)
	{
		temp.Format("\n%d Errors during assembly - skipping link step\n", totalErrors);
		m_BuildTextBuf->append((const char *) temp);
	}
	else
	{
		m_BuildTextBuf->append("Linking...\n");
	}

}

void VT_Ide::CleanProject(void)
{
}

void VT_Ide::ShowProjectSettings(void)
{
	VT_ProjectSettings*		pProj;

	// Ensure there is an active project
	if (m_ActivePrj == NULL)
		return;

	// Get project parameters
	pProj = new VT_ProjectSettings(m_ActivePrj);
	pProj->show();
	
	while (pProj->visible())
		Fl::wait();

}

/*
===============================================================================
This routine returns a string which is the relative form of the given path
realtive to the relTo path.  The routine detects both relativeness in bot
directions and uses '..' as necessary in the returned string.
===============================================================================
*/
MString VT_Ide::MakePathRelative(const MString& path, const MString& relTo)
{
	int				c;
	int				lenPath, lenRel;	// Length of each string
	int				lastRelBranch;		// Index of last '/' in relTo
	int				lastMatch;
	MString			temp;
	int				slashIndex = 0;

	// Determine if path is already relative
	if ((path.Left(2) == "./") || (path.Left(3) == "../"))
		return path;
	if (path.Find("/", 0) == -1)
		return path;
	if (path[1] == ':')
		slashIndex = 2;
	if (path[slashIndex] != '/')
		return path;

	// Find the location of the last '/' in each string
	lastRelBranch = relTo.ReverseFind('/');
	lenPath = path.GetLength();
	lenRel = relTo.GetLength();

	// Start at beginning of string and compare each path segment
	// to find the first location where they don't match
	c = lastMatch = 0;
	while ((c != lenPath) && (c != lenRel))
	{
		// Test 
		if (path[c] != relTo[c])
			break;
		if (path[c] == '/')
			lastMatch = c;
		c++;
	}
	// Determine if the last branch is identical.  This must be done because
	// the loop could have exited simply becuase we came to the end of one of
	// the 2 strings:
	if (((c == lenPath) && (relTo[c] == '/')) || ((path[c] == '/') && (c == lenRel)))
		lastMatch = c;

	// Okay, now we know where the path's are different, scan through the
	// relTo path to determine how many '../' entries we need, if any
	c = lastMatch;
	while (c != lenRel)
	{
		if (c == lastRelBranch)
		{
			temp += "../";				// Add another 'prev dir' indicator
			break;						// At last branch...exit
		}
		// Check for a directory specifier
		if (relTo[c] == '/')
		{
			temp += "../";
		}
		c++;
	}

	// Now append the remainder of the path from the lastMatch point
	c = lastMatch;
	if (path[c] == '/')
		c++;
	while (c < lenPath)
	{
		temp += path[c++];
	}

	return temp;
}

/*
===============================================================================
This routine returns a string which is the absolute form of the given path
realtive to the relTo path.  If the path is already an absolute path, it is 
simply returned unaltered.  The routine resolves ./ and ../ references.
===============================================================================
*/
MString VT_Ide::MakePathAbsolute(const MString& path, const MString& relTo)
{
	int				index;
	MString			temp, newRel;
	int				slashIndex = 0;

	// Determine if path is already absolute
	if (path[1] == ':')
		slashIndex = 2;
	if (path[slashIndex] == '/')
		return path;

	// If path starts with ./ then simply remove it
	if (path.Left(2) == "./")
		temp = path.Right(path.GetLength()-2);
	else
		temp = path;

	newRel = relTo;
	while (temp.Left(3) == "../")
	{
		index = newRel.ReverseFind('/');
		if (index != -1)
			newRel = newRel.Left(index - 1);
	}

	temp = newRel + '/' + temp;
	return temp;
}

void cb_replace_all(Fl_Widget* w, void* v)
{
	VT_ReplaceDlg*	pDlg = (VT_ReplaceDlg*) v;

	pDlg->m_pParent->ReplaceAll();
}

void cb_replace_next(Fl_Widget* w, void* v)
{
	VT_ReplaceDlg*	pDlg = (VT_ReplaceDlg*) v;

	pDlg->m_pParent->ReplaceNext();
}

void cb_replace_cancel(Fl_Widget* w, void* v)
{
	VT_ReplaceDlg*	pDlg = (VT_ReplaceDlg*) v;

	pDlg->m_pReplaceDlg->hide();
}

/*
================================================================================
VT_ReplaceDlg routines below.
================================================================================
*/
VT_ReplaceDlg::VT_ReplaceDlg(class VT_Ide* pParent)
{
	m_pReplaceDlg = new Fl_Window(300, 105, "Replace");
	m_pFind = new Fl_Input(80, 10, 210, 25, "Find:");
	m_pFind->align(FL_ALIGN_LEFT);

	m_pWith = new Fl_Input(80, 40, 210, 25, "Replace:");
	m_pWith->align(FL_ALIGN_LEFT);

	m_pAll = new Fl_Button(10, 70, 90, 25, "Replace All");
	m_pAll->callback(cb_replace_all, this);

	m_pNext = new Fl_Button(105, 70, 120, 25, "Replace Next");
	m_pNext->callback(cb_replace_next, this);

	m_pCancel = new Fl_Button(230, 70, 60, 25, "Cancel");
	m_pCancel->callback(cb_replace_cancel, this);

	m_pReplaceDlg->end();
	m_pReplaceDlg->set_non_modal();

	m_pParent = pParent;
}

/*
================================================================================
VT_FindDlg routines below.
================================================================================
*/
VT_FindDlg::VT_FindDlg(class VT_Ide* pParent)
{
	m_pFindDlg = new Fl_Window(300, 105, "Find");
	m_pFind = new Fl_Input(40, 10, 250, 25, "Find:");
	m_pFind->align(FL_ALIGN_LEFT);

	m_pForward = new Fl_Round_Button(80, 40, 90, 25, "Forward");
	m_pForward->value(1);
	m_pBackward = new Fl_Round_Button(200, 40, 90, 25, "Backward");

	m_pNext = new Fl_Return_Button(105, 70, 100, 25, "Find Next");
	m_pNext->callback(cb_replace_next, this);

	m_pCancel = new Fl_Button(230, 70, 60, 25, "Cancel");
	m_pCancel->callback(cb_replace_cancel, this);

	m_pFindDlg->end();
	m_pFindDlg->set_non_modal();

	m_pParent = pParent;
}

