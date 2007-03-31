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

#include "VirtualT.h"
#include "m100emu.h"
#include "ide.h"
#include "multiwin.h"
#include "disassemble.h"
#include "cpuregs.h"
#include "memedit.h"
#include "periph.h"

VT_Ide *		gpIde = 0;
Fl_Menu_Button*	gPopup = 0;
void setMonitorWindow(Fl_Window* pWin);
Fl_Multi_Window*	gpLcd;

void close_cb(Fl_Widget* w, void*);
void cb_new_file(Fl_Widget* w, void*);
void cb_open_file(Fl_Widget* w, void*);
void cb_save_file(Fl_Widget* w, void*);
void cb_save_as(Fl_Widget* w, void*);
void cb_new_project(Fl_Widget* w, void*);
void cb_open_project(Fl_Widget* w, void*);
void cb_save_project(Fl_Widget* w, void*);
void cb_help(Fl_Widget* w, void*);
void cb_about(Fl_Widget* w, void*);
void cb_lcd_display(Fl_Widget* w, void*);

IMPLEMENT_DYNCREATE(VT_IdeGroup, VTObject);
IMPLEMENT_DYNCREATE(VT_IdeSource, VTObject);

/*
=======================================================
Menu Items for the IDE window
=======================================================
*/
Fl_Menu_Item gIde_menuitems[] = {
  { "&File", 0, 0, 0, FL_SUBMENU },
	{ "New File",			0,		cb_new_file, 0, 0 },
	{ "Open File...",		0,		cb_open_file, 0, 0 },
	{ "Save File...",		0,		cb_save_file, 0, 0},
	{ "Save As...",			0,		cb_save_as, 0, FL_MENU_DIVIDER },
	{ "New Project...",		0,		cb_new_project, 0, 0 },
	{ "Open Project...",	0,		cb_open_project, 0, 0 },
	{ "Save Project...",	0,		cb_save_project, 0, FL_MENU_DIVIDER },
	{ "E&xit",				FL_CTRL + 'q', close_cb, 0 },
	{ 0 },
  { "&Debug", 0, 0, 0, FL_SUBMENU },
	{ "Laptop Display",			0,		cb_lcd_display, 0, 0 },
	{ 0 },
/*
  { "&Emulation",         0, 0,        0, FL_SUBMENU },
	{ "Reset",            0, cb_reset, 0, FL_MENU_DIVIDER },
	{ "Model",            0, 0,        0, FL_SUBMENU 	 } ,
		{ "M100",   0, cb_M100,   (void *) 1, FL_MENU_RADIO | FL_MENU_VALUE },
		{ "M102",   0, cb_M102,   (void *) 2, FL_MENU_RADIO },
		{ "T200",   0, cb_M200,   (void *) 3, FL_MENU_RADIO },
		{ "PC-8201",  0, cb_PC8201, (void *) 4, FL_MENU_RADIO },
//		{ "8300",   0, cb_PC8300, (void *) 5, FL_MENU_RADIO },
		{ 0 },
	{ "Speed", 0, 0, 0, FL_SUBMENU },
		{ "2.4 MHz",     0, rspeed, (void *) 1, FL_MENU_RADIO | FL_MENU_VALUE },
		{ "Max Speed",   0, fspeed, (void *) 1, FL_MENU_RADIO },
		{ 0 },
	{ "Display", 0, 0, 0, FL_SUBMENU | FL_MENU_DIVIDER},
		{ "1x",  0, cb_1x, (void *) 1, FL_MENU_RADIO },
		{ "2x",  0, cb_2x, (void *) 2, FL_MENU_RADIO },
		{ "3x",  0, cb_3x, (void *) 3, FL_MENU_RADIO | FL_MENU_VALUE},
		{ "4x",  0, cb_4x, (void *) 4, FL_MENU_RADIO | FL_MENU_DIVIDER},
		{ "Framed",  0, cb_framed, (void *) 1, FL_MENU_TOGGLE|FL_MENU_VALUE },
		{ "Solid Chars",  0, cb_solidchars, (void *) 1, FL_MENU_TOGGLE},
		{ 0 },
*/
	{ "&Tools", 0, 0, 0, FL_SUBMENU },
		{ "CPU Registers",         0, cb_CpuRegs },
		{ "Disassembler",          0, disassembler_cb },
		{ "Memory Editor",         0, cb_MemoryEditor },
		{ "Peripheral Devices",    0, cb_PeripheralDevices },
		{ "Simulation Log Viewer", 0, 0 },
		{ "Model T File Viewer",   0, 0 },
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
void cb_build(Fl_Widget* w, void*);
void cb_build_all(Fl_Widget* w, void*);
void cb_project_settings(Fl_Widget* w, void*);

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
	{ "Build",					0,		cb_build, 0, 0 },
	{ "Build All",				0,		cb_build_all, 0, 0 },
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
	if (gpIde != NULL)
	{
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
}

/*
=======================================================
Callback routine for opening files
=======================================================
*/
void cb_open_file(Fl_Widget* w, void*)
{
}

/*
=======================================================
Callback routine for saving files
=======================================================
*/
void cb_save_file(Fl_Widget* w, void*)
{
}

/*
=======================================================
Callback routine for save as
=======================================================
*/
void cb_save_as(Fl_Widget* w, void*)
{
}

/*
=======================================================
Callback routine for creating new project
=======================================================
*/
void cb_new_project(Fl_Widget* w, void*)
{
	gpIde->NewProject();
}

/*
=======================================================
Callback routine for opening a project
=======================================================
*/
void cb_open_project(Fl_Widget* w, void*)
{
	gpIde->OpenProject();
}

/*
=======================================================
Callback routine for saving current project
=======================================================
*/
void cb_save_project(Fl_Widget* w, void*)
{
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

	// Insure window exists
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

	// Insure window exists
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

	// Insure window exists
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

	// Insure window exists
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

	// Insure window exists
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

	// Insure window exists
	if (gpIde != 0)
		gpIde->TreeFileProperties(n);
}

/*
=======================================================
Callback routine to build the project
=======================================================
*/
void cb_build(Fl_Widget* w, void*)
{
}

/*
=======================================================
Callback routine to build all project files
=======================================================
*/
void cb_build_all(Fl_Widget* w, void*)
{
}

/*
=======================================================
Callback routine to display project settings
=======================================================
*/
void cb_project_settings(Fl_Widget* w, void*)
{
}

/*
=======================================================
Menu Item Callbacks
=======================================================
*/
void cb_Ide(Fl_Widget* w, void*) 
{

	if (gpIde == NULL)
	{
		// Create a new window for the disassembler
		gpIde = new VT_Ide(800, 600 , "Integrated Development Environment - Work in progress!!!");
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
      printf( "%s hilighted\n", n->label() );
      break;

    case FLU_UNHILIGHTED:
      printf( "%s unhilighted\n", n->label() );
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
      printf( "%s selected\n", n->label() );*/
      break;

    case FLU_UNSELECTED:
      printf( "%s unselected\n", n->label() );
      break;

    case FLU_OPENED:
      printf( "%s opened\n", n->label() );
      break;

    case FLU_CLOSED:
      printf( "%s closed\n", n->label() );
      break;

    case FLU_DOUBLE_CLICK:
      printf( "%s double-clicked\n", n->label() );
	  if (gpIde != 0)
		  gpIde->OpenTreeFile(n);
      break;

    case FLU_WIDGET_CALLBACK:
      printf( "%s widget callback\n", n->label() );
      break;

	case FLU_RIGHT_CLICK:
		printf( "%s right click\n", n->label());
		gpIde->RightClick(n);
		break;

    case FLU_MOVED_NODE:
      printf( "%s moved\n", n->label() );
      break;

    case FLU_NEW_NODE:
      printf( "node '%s' added to the tree\n", n->label() );
      break;
    }
}


VT_Ide::VT_Ide(int w, int h, const char *title)
: Fl_Window(w, h, title)
{
	// Parent window has no box, only child regions
	box(FL_NO_BOX);
	callback(close_ide_cb);

	// Create a menu for the new window.
	Fl_Menu_Bar *m = new Fl_Menu_Bar(0, 0, 800, MENU_HEIGHT-2);
	m->menu(gIde_menuitems);

	// Create a tiled window to support Project, Edit, and debug regions
	Fl_Tile* tile = new Fl_Tile(0,MENU_HEIGHT-2,800,600-MENU_HEIGHT);

	/*
	============================================
	Create region for Project tree
	============================================
	*/
	m_ProjWindow = new Fl_Window(2,MENU_HEIGHT-2,198,430,"");
	m_ProjWindow->box(FL_DOWN_BOX);

	// Create Tree control
	m_ProjTree = new Flu_Tree_Browser( 0, 0, 198, 430 );
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
	m_EditWindow = new Fl_Window(200,MENU_HEIGHT-2,600,430,"Edit");
	m_EditWindow->box(FL_DOWN_BOX);
	m_EditWindow->color(FL_DARK2);
	m_EditWindow->end();

	/*
	=================================================
	Create region for Debug and output tabs
	=================================================
	*/
	m_TabWindow = new Fl_Window(0,MENU_HEIGHT-2+430,800,170-MENU_HEIGHT+2,"Tab");
	m_TabWindow->box(FL_DOWN_BOX);
	m_TabWindow->color(FL_LIGHT1);

	// Create a tab control
	m_Tabs = new Fl_Tabs(0, 1, 800, 170-MENU_HEIGHT);

	/*
	====================
	Create build tab
	====================
	*/
	m_BuildTab = new Fl_Group(2, 0, 797, 170-MENU_HEIGHT-20, " Build ");
	m_BuildTab->box(FL_DOWN_BOX);
	m_BuildTab->selection_color(FL_WHITE);
	m_BuildTab->color(FL_WHITE);

	// Create a Text Editor to show the disassembled text
	m_BuildTextDisp = new Fl_Text_Display(2, 0, 797, 170-MENU_HEIGHT-20);
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
	m_DebugTab = new Fl_Group(2, 0, 797, 170-MENU_HEIGHT-20, " Debug ");
	m_DebugTab->box(FL_DOWN_BOX);
	m_DebugTab->selection_color(FL_WHITE);
	m_DebugTab->color(FL_WHITE);
	m_DebugTab->end();
	
	/*
	====================
	Create watch tab
	====================
	*/
	m_WatchTab = new Fl_Group(2, 0, 797, 170-MENU_HEIGHT-20, " Watch ");
	m_WatchTab->box(FL_NO_BOX);
	m_WatchTab->selection_color(FL_WHITE);
	m_WatchTab->color(FL_WHITE);

	// Create tiled window for auto and watch variables
	Fl_Tile* tile2 = new Fl_Tile(2, 0,800,170-MENU_HEIGHT-20);

	Fl_Box* box0 = new Fl_Box(2, 0,398,170-MENU_HEIGHT-20,"1");
	box0->box(FL_DOWN_BOX);
	box0->color(FL_BACKGROUND_COLOR);
	box0->labelsize(36);
	box0->align(FL_ALIGN_CLIP);
	Fl_Box* box1 = new Fl_Box(400, 0,400,170-MENU_HEIGHT-20,"2");
	box1->box(FL_DOWN_BOX);
	box1->color(FL_BACKGROUND_COLOR);
	box1->labelsize(36);
	box1->align(FL_ALIGN_CLIP);
	Fl_Box* r2 = new Fl_Box(0,0,800,170-MENU_HEIGHT-20);
	tile2->resizable(r2);
	tile2->end();
	m_WatchTab->resizable(tile2);
	m_WatchTab->end();
	
	m_Tabs->end();

	m_TabWindow->resizable(m_Tabs);
	m_TabWindow->end();

	// Set resize region
	Fl_Box* r = new Fl_Box(150,MENU_HEIGHT-2,800-150,430);
	tile->resizable(r);
	tile->end();
	resizable(m);
	resizable(tile);

	// End the Window
	end();

	// Initialize other variables
	m_ActivePrj = 0;
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
NewProject routine handles the File->New Project menu item.
This routine creates an empty project and updates the Tree
control.
=============================================================
*/
void VT_Ide::NewProject(void)
{
	VT_IdeGroup*	pGroup;

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

	// Create new project
	m_ActivePrj = new VT_IdeProject;
	m_ActivePrj->m_Name = "Unknown";

	pGroup = new VT_IdeGroup;
	pGroup->m_Name = "Source Files";
	m_ActivePrj->m_Groups.Add(pGroup);

	pGroup = new VT_IdeGroup;
	pGroup->m_Name = "Header Files";
	m_ActivePrj->m_Groups.Add(pGroup);

	pGroup = new VT_IdeGroup;
	pGroup->m_Name = "Object Files";
	m_ActivePrj->m_Groups.Add(pGroup);

	BuildTreeControl();

	m_ProjTree->redraw();
	m_ProjTree->show();
}

/*
=============================================================
The SaveProject routine saves the active project structure to
a .prj file.
=============================================================
*/
void VT_Ide::SaveProject(void)
{
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

	// Insure we have a project structure
	if (m_ActivePrj == 0)
		return;

	// set the default leaf icon to be a blue dot
	m_ProjTree->leaf_icon( &gTextDoc );
	m_ProjTree->insertion_mode( FLU_INSERT_BACK );

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
			
			// Loop through all objects an add to tree
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
		
		// Loop through all objects an add to tree
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
	MString			sParam;
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

	// Insure active project not null
	if (m_ActivePrj == 0)
		m_ActivePrj = new VT_IdeProject;

	// Loop through all lines in file
	while (fgets(line, 512, fd) != 0)
	{
		// Skip comments and blank lines
		if ((line[0] == '#') || (line[0] == '\n'))
			continue;

		// Parse the line
		param = strtok(line, "=\n");
		value = strtok(NULL, "=\n");
		
		// Check for error in line
		if (param == 0)
		{
			// Report error
			continue;
		}

		sParam = param;
		sParam.MakeUpper();
		sPtr = (const char *) sParam;
		if (strcmp(sPtr, "SOURCE") == 0)
		{
			// Insure there is an active group
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
		else if (strcmp(sPtr, "INCLPATH") == 0)
		{
			if (value != 0)
				m_ActivePrj->m_IncludePath = value;
		}
		else if (strcmp(sPtr, "LINKPATH") == 0)
		{
			if (value != 0)
				m_ActivePrj->m_LinkPath = value;
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
	}

	// Close the file
	fclose(fd);
	return 1;
}

VT_IdeProject::~VT_IdeProject()
{ 
	VTObject*	pObj;

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
		VTObject* pObj = (VTObject *) n->user_data();
		if (strcmp(pObj->GetClass()->m_ClassName, "VT_IdeGroup") == 0)
		{
			gGroupMenu[0].user_data_ = n;
			gGroupMenu[1].user_data_ = n;
			gGroupMenu[2].user_data_ = n;
			gPopup->menu(gGroupMenu);
		}
		else
		{
			gSourceMenu[0].user_data_ = n;
			gSourceMenu[1].user_data_ = n;
			gSourceMenu[2].user_data_ = n;
			gPopup->menu(gSourceMenu);
		}
	}
	gPopup->label(n->label());
	gPopup->popup();
}

void VT_Ide::NewFolder(Flu_Tree_Browser::Node* n)
{
	MString			name, filespec;
	VT_IdeGroup*	pGroup;
	VT_IdeGroup*	pNewGrp;
	int				len, c, root;
	Flu_Tree_Browser::Node*	i;
	Flu_Tree_Browser::Node* newNode;

	// Get Folder name
	name = "ide";
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

void VT_Ide::AddFilesToFolder(Flu_Tree_Browser::Node* n)
{
}

void VT_Ide::FolderProperties(Flu_Tree_Browser::Node* n)
{
}

void VT_Ide::OpenTreeFile(Flu_Tree_Browser::Node* n)
{
	VT_IdeSource*	pSource;
	MString			file;

	pSource = (VT_IdeSource *) n->user_data();
	if (pSource->m_Name.Left(2) == "./")
		file = pSource->m_Name.Right(pSource->m_Name.GetLength()-2);
	else
		file = pSource->m_Name;

	int x = 10;
	int y = 10;
	const char *pStr = file;
	if (file == "main.asm")
		x = 200;
	Fl_Multi_Window* mw = new Fl_Multi_Window(x, 10, 400, 300, (const char *)file);
	Fl_Text_Editor* te = new Fl_Text_Editor(0, 0, mw->ClientArea()->w(), 
		mw->ClientArea()->h(), file);
	Fl_Text_Buffer* tb = new Fl_Text_Buffer();
	te->buffer(tb);

	// Show the Disassembling text to indicate activity
	tb->loadfile(file);
	te->textfont(FL_COURIER);
	te->end();
	mw->ClientArea()->resizable(te);
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
}

void VT_Ide::AssembleTreeFile(Flu_Tree_Browser::Node* n)
{
}

void VT_Ide::TreeFileProperties(Flu_Tree_Browser::Node* n)
{
}

