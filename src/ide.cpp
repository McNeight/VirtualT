/* ide.cpp */

/* $Id: ide.cpp,v 1.14 2013/02/06 17:06:51 kpettit1 Exp $ */

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
#include <FL/Fl_Check_Button.H>
#include "FLU/Flu_Tree_Browser.h"
#include "FLU/flu_pixmaps.h"
#include "FLU/Flu_File_Chooser.h"
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
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
#include "linker.h"
#include "multiwin.h"
#include "idetabs.h"
#include "pref_form.h"
#include "file.h"
#include "memory.h"

/*
=======================================================
Define global variables
=======================================================
*/
#define	VT_NUM_RECENT_FILES		6
#define	VT_NUM_RECENT_PROJECTS	4

VT_Ide *			gpIde = 0;
Fl_Menu_Button*		gPopup = 0;
Fl_Multi_Window*	gpLcd;
MString				gRootpath;
char				usrdocdir[256];
int					gIdeX, gIdeY, gIdeW, gIdeH;
char				gRecentFile[VT_NUM_RECENT_FILES][256];
char				gRecentProject[VT_NUM_RECENT_PROJECTS][256];
int					gDisableHl, gSmartIndent, gReloadProject;
static Fl_Menu_Bar *gMenuBar = 0;
char				gProjectTypes[][6] = {
						".co",
						".obj",
						".rom",
						".ba",
						"" };

#ifdef _WIN32
extern "C"
#endif
extern char path[];

extern My_Text_Display::Style_Table_Entry gStyleTable[9];
extern Fl_Preferences	virtualt_prefs;

/*
=======================================================
Define function prototypes for callbacks
=======================================================
*/
void setMonitorWindow(Fl_Window* pWin);
void close_ide_cb(Fl_Widget* w, void*);
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
void cb_undo(Fl_Widget* w, void*);
void cb_copy(Fl_Widget* w, void*);
void cb_cut(Fl_Widget* w, void*);
void cb_paste(Fl_Widget* w, void*);
void cb_find(Fl_Widget* w, void*);
void cb_find_next(Fl_Widget* w, void*);
void cb_replace(Fl_Widget* w, void*);
void cb_prefs(Fl_Widget* w, void*);
// Define project management routines
void cb_build_project(Fl_Widget* w, void*);
void cb_rebuild_project(Fl_Widget* w, void*);
void cb_clean_project(Fl_Widget* w, void*);
void cb_project_settings(Fl_Widget* w, void*);
void add_recent_file_to_menu(const char *filename);
void add_recent_project_to_menu(const char *filename);

IMPLEMENT_DYNCREATE(VT_IdeGroup, VTObject);
IMPLEMENT_DYNCREATE(VT_IdeSource, VTObject);

/*
=======================================================
Menu Items for the IDE window
=======================================================
*/
const int	gProjOffset = 16;
const int	gFileOffset = 4;

Fl_Menu_Item gIde_menuitems[] = {
  { "&File", 0, 0, 0, FL_SUBMENU },
	{ "New File",			FL_CTRL + 'n',		cb_new_file, 0, 0 },
	{ "Open File...",		FL_CTRL + 'o',		cb_open_file, 0, 0 },
    { "Recent Files",    0, 0, 0, FL_SUBMENU },
		{ "", 0, 0, 0, FL_MENU_INVISIBLE },
		{ "", 0, 0, 0, FL_MENU_INVISIBLE },
		{ "", 0, 0, 0, FL_MENU_INVISIBLE },
		{ "", 0, 0, 0, FL_MENU_INVISIBLE },
		{ "", 0, 0, 0, FL_MENU_INVISIBLE },
		{ "", 0, 0, 0, FL_MENU_INVISIBLE },
	{ 0 },
	{ "Save File",			FL_CTRL + 's',		cb_save_file, 0, 0},
	{ "Save As...",			0,		cb_save_as, 0, FL_MENU_DIVIDER },
	{ "New Project...",		0,		cb_new_project, 0, 0 },
	{ "Open Project...",	0,		cb_open_project, 0, 0 },
    { "Recent Projects",    0, 0, 0, FL_SUBMENU },
		{ "", 0, 0, 0, FL_MENU_INVISIBLE },
		{ "", 0, 0, 0, FL_MENU_INVISIBLE },
		{ "", 0, 0, 0, FL_MENU_INVISIBLE },
		{ "", 0, 0, 0, FL_MENU_INVISIBLE },
	{ 0 },
	{ "Save Project",		0,		cb_save_project, 0, FL_MENU_DIVIDER },
	{ "Exit",				FL_CTRL + 'q', close_ide_cb, 0 },
	{ 0 },
  { "&Edit", 0, 0, 0, FL_SUBMENU },
  { "&Undo",				'u',				cb_undo, 0, 0 },
	{ "Copy",				'c',				cb_copy, 0, 0 },
	{ "Cut",				't',				cb_cut, 0, 0 },
	{ "&Paste",				'p',				cb_paste, 0, FL_MENU_DIVIDER},
	{ "Find...",			FL_CTRL + 'f',		cb_find, 0, 0 },
	{ "Find Next",			FL_F + 3,			cb_find_next, 0, 0 },
	{ "Replace",			FL_CTRL + 'r',		cb_replace, 0, FL_MENU_DIVIDER },
	{ "Preferences",		0,					cb_prefs, 0, 0 },
	{ 0 },
  { "&Project", 0, 0, 0, FL_SUBMENU },
	{ "Build",				FL_SHIFT + FL_F+8,		cb_build_project, 0, 0 },
	{ "Rebuild",			0,		cb_rebuild_project, 0, 0 },
	{ "Clean",				0,		cb_clean_project, 0, FL_MENU_DIVIDER },
	{ "Settings",			0,		cb_project_settings, 0, 0 },
	{ 0 },
//  { "&Debug", 0, 0, 0, FL_SUBMENU },
//	{ "Laptop Display",			0,		cb_lcd_display, 0, 0 },
//	{ 0 },
  { "&Tools", 0, 0, 0, FL_SUBMENU },
	{ "CPU Registers",         0, cb_CpuRegs },
	{ "Disassembler",          0, disassembler_cb },
	{ "Memory Editor",         0, cb_MemoryEditor },
	{ "Peripheral Devices",    0, cb_PeripheralDevices },
	{ "Model T File Viewer",   0, 0 },
	{ 0 },
  { "&Help", 0, 0, 0, FL_SUBMENU },
	{ "Help", 0, cb_help },
	{ "About VirtualT", 0, cb_about },
	{ 0 },

  { 0 }
};

void cb_new_folder(Fl_Widget* w, void*);
void cb_delete_folder(Fl_Widget* w, void*);
void cb_add_files_to_folder(Fl_Widget* w, void*);
void cb_folder_properties(Fl_Widget* w, void*);
void cb_open_tree_file(Fl_Widget* w, void*);
void cb_remove_tree_file(Fl_Widget* w, void*);
void cb_assemble_tree_file(Fl_Widget* w, void*);
void cb_tree_file_properties(Fl_Widget* w, void*);

Fl_Menu_Item gGroupMenu[] = {
	{ "  New Folder  ",			0,		cb_new_folder, 0, 0 },
	{ "  Add Files to Folder  ",0,		cb_add_files_to_folder, 0, 0 },
	{ "  Remove  ",				0,		cb_remove_tree_file, 0, FL_MENU_DIVIDER },
	{ "  Properties  ",			0,		cb_folder_properties, 0, 0},
	{ 0 }
};

Fl_Menu_Item gSourceMenu[] = {
	{ "  Open  ",				0,		cb_open_tree_file, 0, 0 },
	{ "  Remove  ",				0,		cb_remove_tree_file, 0, FL_MENU_DIVIDER },
	{ "  Assemble File  ",		0,		cb_assemble_tree_file, 0, 0 },
	{ "  Properties  ",			0,		cb_tree_file_properties, 0, 0},
	{ 0 }
};

Fl_Menu_Item gRootMenu[] = {
	{ "  Build  ",					0,		cb_build_project, 0, 0 },
	{ "  Build All  ",				0,		cb_rebuild_project, 0, FL_MENU_DIVIDER },
	{ "  New Folder...  ",			0,		cb_new_folder, 0, FL_MENU_DIVIDER},
//	{ "  Add Files to Project...  ",	0,		cb_add_files_to_folder, 0, FL_MENU_DIVIDER },
	{ "  Project Settings...  ",	0,		cb_project_settings, 0, 0},
	{ 0 }
};

Fl_Pixmap gTextDoc( textdoc_xpm ), gComputer( computer_xpm );

/*
=======================================================
Save the user's preferences for IDE window size.
=======================================================
*/
void Ide_SavePrefs(void)
{
	// Save window parameters to preferences
	if (save_window_size)
	{
		virtualt_prefs.set("IdeX", gpIde->x());
		virtualt_prefs.set("IdeY", gpIde->y());
		virtualt_prefs.set("IdeW", gpIde->w());
		virtualt_prefs.set("IdeH", gpIde->h());
		int ideTabHeight = gpIde->h() - gpIde->SpliterHeight();
		int ideTreeWidth = gpIde->SpliterWidth();
		virtualt_prefs.set("IdeTabheight", ideTabHeight);
		virtualt_prefs.set("IdeTreeWidth", ideTreeWidth);
	}
	gpIde->SavePrefs();
}

/*
=======================================================
Callback routine for the close box of the IDE window
=======================================================
*/
void close_ide_cb(Fl_Widget* w, void*)
{
	int				ans;

	if (gpIde != NULL)
	{
		// Check if project is dirty
		if (gpIde->ProjectDirty())
		{
			// Ask if project should be saved
			ans = fl_choice("Save changes to project %s?", "Cancel", "Yes", "No", (const char *) gpIde->ProjectName());
			if (ans == 0)
				return;
			if (ans == 1)
				gpIde->SaveProject();
		}

		Ide_SavePrefs();

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
Callback for opening recent files
=======================================================
*/void recent_file_cb(Fl_Widget* w, void*)
{
//  if (!check_save()) return;
	Fl_Menu_* mw = (Fl_Menu_*)w;
  	const Fl_Menu_Item* m = mw->mvalue();
  	if (m) {
		char *newfile = (char*)m->label();
		char *path = new char[strlen(newfile)+1];
		strcpy(path, newfile);

	 	if (newfile != NULL) 
		{
			char *slash;
	    	slash = strrchr(path, '/');
			*slash = '\0';
			gpIde->OpenFile(newfile);
			add_recent_file_to_menu(newfile);
		}
	}
}

/*
=======================================================
Routine to update the recent files menu items
=======================================================
*/void add_recent_file_to_menu(const char *filename)
{
	int		c;

	// Check if filename is already in the recent file list
	for (c = 0; c < VT_NUM_RECENT_FILES; c++)
	{
		if(strcmp(filename, gIde_menuitems[c + gFileOffset].label()) == 0) 
			break;
	}
	// Return if filename is already the 1st file in the list
	if (c == 0)
		return;

	// If the filename already exists, then just reorder the list
	if (c < VT_NUM_RECENT_FILES) 
	{
		Fl_Menu_Item mi = gIde_menuitems[c + gFileOffset];

		for (int x = c; x > 0; x--)
			gIde_menuitems[x + gFileOffset] = gIde_menuitems[x + gFileOffset-1];
		gIde_menuitems[gFileOffset] = mi;
	}
	else 
	{
		char *path = new char[strlen(filename)+1];
		strcpy(path, filename);

		Fl_Menu_Item mi = { path, 0, recent_file_cb };
		
		char *t = (char *)gIde_menuitems[VT_NUM_RECENT_FILES + gFileOffset-1].text;
		//cout << t << endl;
		if(t) 
			if (strlen(t) != 0)
				free(t);
		
		for (int x = VT_NUM_RECENT_FILES - 1; x > 0; x--)
			gIde_menuitems[x + gFileOffset] = gIde_menuitems[x + gFileOffset-1];
    	gIde_menuitems[gFileOffset] = mi;
	}
	
	gMenuBar->copy(gIde_menuitems, gpIde);

	for (c = 0; c < VT_NUM_RECENT_FILES; c++)
		strcpy(gRecentFile[c], gIde_menuitems[c + gFileOffset].text);
}

/*
=======================================================
Callback for opening recent projects
=======================================================
*/void recent_project_cb(Fl_Widget* w, void*)
{
//  if (!check_save()) return;
	Fl_Menu_* mw = (Fl_Menu_*)w;
  	const Fl_Menu_Item* m = mw->mvalue();
  	if (m) {
		char *newfile = (char*)m->label();
		char *path = new char[strlen(newfile)+1];
		strcpy(path, newfile);

	 	if (newfile != NULL) 
		{
			char *slash;
	    	slash = strrchr(path, '/');
			*slash = '\0';
			if (gpIde->CloseAllFiles())
			{
				gpIde->OpenProject(newfile);
				add_recent_project_to_menu(newfile);
			}
		}
	}
}

/*
=======================================================
Routine to update the recent projects menu items
=======================================================
*/
void add_recent_project_to_menu(const char *filename)
{
	int		c;

	// Check if filename is already in the recent file list
	for (c = 0; c < VT_NUM_RECENT_PROJECTS; c++)
	{
		if(strcmp(filename, gIde_menuitems[c + gProjOffset].label()) == 0) 
			break;
	}
	// Return if filename is already the 1st file in the list
	if (c == 0)
		return;

	// If the filename already exists, then just reorder the list
	if (c < VT_NUM_RECENT_PROJECTS) 
	{
		Fl_Menu_Item mi = gIde_menuitems[c + gProjOffset];

		for (int x = c; x > 0; x--)
			gIde_menuitems[x + gProjOffset] = gIde_menuitems[x + gProjOffset-1];
		gIde_menuitems[gProjOffset] = mi;
	}
	else 
	{
		char *path = new char[strlen(filename)+1];
		strcpy(path, filename);

		Fl_Menu_Item mi = { path, 0, recent_project_cb };
		
		char *t = (char *)gIde_menuitems[VT_NUM_RECENT_PROJECTS + 
			gProjOffset-1].text;
		//cout << t << endl;
		if(t) 
			if (strlen(t) != 0)
				free(t);
		
		for (int x = VT_NUM_RECENT_PROJECTS - 1; x > 0; x--)
			gIde_menuitems[x + gProjOffset] = gIde_menuitems[x + gProjOffset-1];
    	gIde_menuitems[gProjOffset] = mi;
	}
	
	gMenuBar->copy(gIde_menuitems, gpIde);

	for (c = 0; c < VT_NUM_RECENT_PROJECTS; c++)
		strcpy(gRecentProject[c], gIde_menuitems[c + gProjOffset].text);
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
Callback routine for handling preferences
=======================================================
*/
void set_text_size(int t)
{
	gStyleTable[0].size = t;
	gStyleTable[0].color = hl_plain;
	
	gStyleTable[1].size = t;			
	gStyleTable[1].color = hl_linecomment;
	
	gStyleTable[2].size = t;
	gStyleTable[2].color = hl_blockcomment;
				
	gStyleTable[3].size = t;
	gStyleTable[3].color = hl_string;
	
	gStyleTable[4].size = t;
	gStyleTable[4].color = hl_directive;
	
	gStyleTable[5].size = t;
	gStyleTable[5].color = hl_type;
	
	gStyleTable[6].size = t;
	gStyleTable[6].color = hl_keyword;
	
	gStyleTable[7].size = t;
	gStyleTable[7].color = hl_character;

	gStyleTable[8].size = t;
	gStyleTable[8].color = hl_label;

}

/*
=======================================================
Routine to set the window colors after a change in the
perferences dialog.
=======================================================
*/
void VT_Ide::SetColors(int fg, int bg)
{
	// Set all editor background colors
	int children = m_EditTabs->children();
	for (int c = 0; c < children; c++)
	{
		Fl_Multi_Edit_Window* te = (Fl_Multi_Edit_Window *) m_EditTabs->child(c);
		te->color(background_color);
		te->textcolor(hl_plain);
		te->selection_color(FL_DARK_BLUE);
		te->mCursor_color = hl_plain;
		te->smart_indent = gSmartIndent;
		te->textsize(text_size);
		if (gDisableHl)
			te->DisableHl();
		else
			te->EnableHl();
	}

	// Set Browser Tree colors
	m_ProjTree->connector_style(hl_plain, FL_SOLID);
	m_ProjTree->leaf_text(hl_plain, m_ProjTree->leaf_font(), m_ProjTree->leaf_size());
	m_ProjTree->branch_text(hl_plain, m_ProjTree->branch_font(), m_ProjTree->branch_size());
	m_ProjTree->color(background_color);
	m_ProjTree->shaded_entry_colors(background_color, background_color);
	m_ProjTree->root_color(hl_plain);
	Flu_Tree_Browser::Node * n = m_ProjTree->get_root();
	while (n != NULL)
	{
		n->label_color(hl_plain);
		n->label_size(text_size);
		n = n->next();
	}

	// Set the colors of the Build, Output and Watch windows
	m_BuildTextDisp->color(background_color);
	m_BuildTextDisp->textcolor(hl_plain);
	m_BuildTextDisp->textsize(text_size);
	m_DebugTab->color(background_color);
//	m_DebugTab->labelcolor(hl_plain);

	m_BuildTextDisp->redraw();
	m_ProjTree->redraw();
	m_DebugTab->redraw();
	if (m_EditTabs->value() != NULL)
		m_EditTabs->value()->redraw();
}

/*
=======================================================
Callback routine for displaying the editor preferences
dialog box.
=======================================================
*/
void cb_prefs(Fl_Widget* w, void*)
{
	text_size_choice->value((text_size - 6)/2);
	save_wsoe_check->value(save_window_size);
	hide_output_check->value(auto_hide);
	
	plain_btn->color(hl_plain);
	line_btn->color(hl_linecomment);
	block_btn->color(hl_blockcomment);
	string_btn->color(hl_string);
	directive_btn->color(hl_directive);
	type_btn->color(hl_type);
	keyword_btn->color(hl_keyword);
	character_btn->color(hl_character);
	label_btn->color(hl_label);
	bg_btn->color(background_color);

	pref_window->show();
	while(pref_window->visible()) 
	{
		Fl::wait(5);
	}

	set_text_size(text_size);
	gpIde->SetColors(hl_plain, background_color);
	gpIde->m_EditTabs->redraw();
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
Callback routine for undo operation
=======================================================
*/
void cb_undo(Fl_Widget* w, void*)
{
	if (gpIde != NULL)
		gpIde->Undo();
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
#if 0
	if (gpLcd != 0)
	{
		gpIde->m_EditTabs->insert(*gpLcd, gpIde->m_EditTabs->children());
		gpLcd->show();
		gpLcd->show();
		gpLcd->redraw();
		return;
	}
	if (gModel == MODEL_T200)
		gpLcd = new Fl_Multi_Window(0, TAB_HEIGHT, 240*2 + MW_BORDER_WIDTH*2, 128*2 +
			MW_BORDER_WIDTH + MW_TITLE_HEIGHT + TAB_HEIGHT, "Laptop Display");
	else
		gpLcd = new Fl_Multi_Window(0, TAB_HEIGHT, 240*2 + MW_BORDER_WIDTH*2, 64*2 +
			MW_BORDER_WIDTH + MW_TITLE_HEIGHT + TAB_HEIGHT, "Laptop Display");

	gpLcd->m_NoResize = 1;
	gpLcd->callback(cb_close_lcd);
	gpLcd->end();
	gpLcd->resize(0, TAB_HEIGHT, gpIde->m_EditTabs->w(), gpIde->m_EditTabs->h() -
		TAB_HEIGHT);
	gpIde->m_EditTabs->insert(*gpLcd, gpIde->m_EditTabs->children());
	if (gpIde->m_EditTabs->children() == 1)
		gpIde->m_EditTabs->show();
	gpLcd->show();
	gpLcd->show();
	gpLcd->take_focus();
	setMonitorWindow(gpLcd->ClientArea());
#endif
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
Callback routine to open selected tree file
=======================================================
*/
void cb_remove_tree_file(Fl_Widget* w, void* data)
{
	Flu_Tree_Browser::Node* n = (Flu_Tree_Browser::Node*) data;

	// Ensure window exists
	if (gpIde != 0)
		gpIde->DeleteItem(n);
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
Routine to load the IDE preferences
=======================================================
*/
void VT_Ide::LoadPrefs(void)
{
	int		pc, c;
	char	sRecentFile[32];

	// Get the initial window size from the user preferences
	virtualt_prefs.get("Ide_SaveWindowSize", save_window_size, 1);
	virtualt_prefs.get("Ide_AutoHide", auto_hide, 0);
	virtualt_prefs.get("Ide_TextSize", text_size, 12);
	for (c = 0; c < VT_NUM_RECENT_FILES; c++)
	{
		sprintf(sRecentFile, "Ide_RecentFile%d", c + 1);
		virtualt_prefs.get(sRecentFile, gRecentFile[c], "", 
			sizeof(gRecentFile[0]));
		if (strlen(gRecentFile[c]) != 0)
		{
			char *path = new char[strlen(gRecentFile[c])+1];
			strcpy(path, gRecentFile[c]);
			Fl_Menu_Item mi = { path, 0, recent_file_cb };
			gIde_menuitems[c + gFileOffset] = mi;
		}
	}

	for (c = 0; c < VT_NUM_RECENT_PROJECTS; c++)
	{
		sprintf(sRecentFile, "Ide_RecentProject%d", c + 1);
		virtualt_prefs.get(sRecentFile, gRecentProject[c], "", 
			sizeof(gRecentProject[0]));
		if (strlen(gRecentProject[c]) != 0)
		{
			char *path = new char[strlen(gRecentProject[c])+1];
			strcpy(path, gRecentProject[c]);
			Fl_Menu_Item mi = { path, 0, recent_project_cb };
			gIde_menuitems[c + gProjOffset] = mi;
		}
	}

	virtualt_prefs.get("Ide_ColorText", pc, FL_BLACK);
	hl_plain = (Fl_Color) pc;
	virtualt_prefs.get("Ide_ColorLineComment", pc, 95);
	hl_linecomment = (Fl_Color) pc;
	virtualt_prefs.get("Ide_ColorBlockComment", pc, 93);
	hl_blockcomment = (Fl_Color) pc;
	virtualt_prefs.get("Ide_ColorString", pc, 219);
	hl_string = (Fl_Color) pc;
	virtualt_prefs.get("Ide_ColorDirective", pc, 219);
	hl_directive = (Fl_Color) pc;
	virtualt_prefs.get("Ide_ColorKeyword", pc, 74);
	hl_type = (Fl_Color) pc;
	virtualt_prefs.get("Ide_ColorInstruction", pc, 220);
	hl_keyword = (Fl_Color) pc;
	virtualt_prefs.get("Ide_ColorCharacter", pc, 75);
	hl_character = (Fl_Color) pc;
	virtualt_prefs.get("Ide_ColorLabel", pc, 116);
	hl_label = (Fl_Color) pc;
	virtualt_prefs.get("Ide_ColorBackground", pc, FL_BLACK);
	background_color = (Fl_Color) pc;
	virtualt_prefs.get("Ide_SmartIndent", gSmartIndent, 1);
	virtualt_prefs.get("Ide_ReloadProject", gReloadProject, 1);
	virtualt_prefs.get("Ide_AutoBrace", auto_brace_mode, 1);
	virtualt_prefs.get("Ide_CreateBackups", backup_file, 0);
	virtualt_prefs.get("Ide_DeleteBackups", delbak, 0);
	virtualt_prefs.get("Ide_DisableSyntaxHilight", gDisableHl, 0);

	smart_indent_check->value(gSmartIndent!=0);
	auto_brace_check->value(auto_brace_mode!=0);			
	rec_pr_check->value(gReloadProject!=0);
	bak_check->value(backup_file!=0);
	delbak_check->value(delbak!=0);

	set_text_size(text_size);
}
/*
=======================================================
Routine to save the IDE preferences
=======================================================
*/
void VT_Ide::SavePrefs(void)
{
	char sRecentFile[32];

	// Get the initial window size from the user preferences
	virtualt_prefs.set("Ide_SaveWindowSize", save_window_size);
	virtualt_prefs.set("Ide_AutoHide", auto_hide);
	virtualt_prefs.set("Ide_TextSize", text_size);
	for (int c = 0; c < VT_NUM_RECENT_FILES; c++)
	{
		sprintf(sRecentFile, "Ide_RecentFile%d", c + 1);
		virtualt_prefs.set(sRecentFile, gRecentFile[c]);
	}
	for (int c = 0; c < VT_NUM_RECENT_PROJECTS; c++)
	{
		sprintf(sRecentFile, "Ide_RecentProject%d", c + 1);
		virtualt_prefs.set(sRecentFile, gRecentProject[c]);
	}
	virtualt_prefs.set("Ide_ColorText", (int) hl_plain);
	virtualt_prefs.set("Ide_ColorLineComment", (int) hl_linecomment);
	virtualt_prefs.set("Ide_ColorBlockComment", (int) hl_blockcomment);
	virtualt_prefs.set("Ide_ColorString", (int) hl_string);
	virtualt_prefs.set("Ide_ColorDirective", (int) hl_directive);
	virtualt_prefs.set("Ide_ColorKeyword", (int) hl_type);
	virtualt_prefs.set("Ide_ColorInstruction", (int) hl_keyword);
	virtualt_prefs.set("Ide_ColorCharacter", (int) hl_character);
	virtualt_prefs.set("Ide_ColorLabel", (int) hl_label);
	virtualt_prefs.set("Ide_ColorBackground", (int) background_color);
	virtualt_prefs.set("Ide_SmartIndent", gSmartIndent);
	virtualt_prefs.set("Ide_ReloadProject", gReloadProject);
	virtualt_prefs.set("Ide_AutoBrace", (int) auto_brace_mode);
	virtualt_prefs.set("Ide_CreateBackups", backup_file);
	virtualt_prefs.set("Ide_DeleteBackups", delbak);
	virtualt_prefs.set("Ide_DisableSyntaxHilight", gDisableHl);

	smart_indent_check->value(gSmartIndent!=0);
	auto_brace_check->value(auto_brace_mode!=0);			
	rec_pr_check->value(gReloadProject!=0);
	bak_check->value(backup_file!=0);
	delbak_check->value(delbak!=0);
	disable_hl_check->value(gDisableHl != 0);

	if (m_ActivePrj != NULL)
	{
		SaveProjectIdeSettings();
	}
}


/*
=======================================================
Menu Item Callbacks
=======================================================
*/
void cb_Ide(Fl_Widget* widget, void*) 
{
	int		maxH, maxW;

	if (gpIde == NULL)
	{
		// Get X/Y coords for IDE
		virtualt_prefs.get("IdeX", gIdeX, 40);
		virtualt_prefs.get("IdeY", gIdeY, 40);
		virtualt_prefs.get("IdeW", gIdeW, 800);
		virtualt_prefs.get("IdeH", gIdeH, 600);
		if (gIdeX < 0)
			gIdeX = 0;
		if (gIdeY < 0)
			gIdeY = 0;
		if (gIdeW+6 >= Fl::w())
			gIdeW = Fl::w()-6;
		if (gIdeH+35 >= Fl::h())
			gIdeH = Fl::h()-35;

		make_pref_form();

//#if defined(__APPLE__) || defined(WIN32)
//		int sx, sy, sw, sh;
//		Fl::screen_xywh(sx, sy, sw, sh);
//		maxH = sh;
//		maxW = sw;
//#else
		maxH = Fl::h();
		maxW = Fl::w();
//#endif

		if ((gIdeW >= maxW) && (gIdeX < 0))
			gIdeW = maxW - 50;
		if ((gIdeH >= maxH) && (gIdeY < 0))
			gIdeH = maxH - 60;
		if (gIdeY < 0)
			gIdeY = 0;
		if (gIdeX < 0)
			gIdeX = 0;
		// Create a new window for the IDE workspace
		gpIde = new VT_Ide(gIdeX, gIdeY, gIdeW, gIdeH , "Integrated Development Environment");
		gpIde->show();

		// Test if last project should be opened
		if (gReloadProject && (strlen(gRecentProject[0]) != 0))
			gpIde->OpenProject(gRecentProject[0]);
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

void ide_error_report(int lineNo, char* pFilename, void* pOpaque)
{
	VT_Ide*	pIde = (VT_Ide *) pOpaque;

	/* Report the error callback to the IDE so it can display the file / line */
	pIde->ErrorReport(lineNo, pFilename);
}

/*
==============================================================================
Display the errored file, position to the errored line, and highlight it.
==============================================================================
*/
void VT_Ide::ErrorReport(int lineNo, char* pFilename)
{
	MString path = pFilename;
	//Fl_Multi_Edit_Window* mw;
	My_Text_Editor* mte;

	/* Convert filename to an absolute path */
	MString absPath = MakePathAbsolute(path, m_ActivePrj->m_RootPath);

	/* Open the file if it isn't already */
	OpenFile(absPath);

	/* Position the file to the proper line */
	mte = (My_Text_Editor*) m_EditTabs->value();
	mte->insert_position(mte->get_line_pos(lineNo));

	/* Ensure the line is on the display */
	int topLine = mte->top_line();
	int numLines = mte->num_visible_lines();
	if ((lineNo - topLine < 5) || (topLine + numLines - lineNo < 5))
	{
		topLine = lineNo - (numLines >> 1);
		if (topLine < 1)
			topLine = 1;
		if (topLine + numLines > mte->count_lines(0, -1, true))
			topLine = mte->count_lines(0, -1, true) - numLines+2;

		mte->scroll(topLine, 0);
	}

	/* Update the status bar */
	mte->UpdateStatusBar();
}

/*
==============================================================================
VT_Ide constructor.  This routine creates a new VT_Ide window
==============================================================================
*/
VT_Ide::VT_Ide(int x, int y, int w, int h, const char *title)
: Fl_Window(x, y, w, h, title)
{
	int		ideTreeWidth;

	// Parent window has no box, only child regions
	box(FL_NO_BOX);
	callback(close_ide_cb);

	// Create the preferences tab and load the preferences
	LoadPrefs();

	// Create a menu for the new window.
	Fl_Menu_Bar *m = gMenuBar = new Fl_Menu_Bar(0, 0, w, MENU_HEIGHT-2);
	m->menu(gIde_menuitems);
	m->color(fl_rgb_color(240,239,228));

	// Create a tiled window to support Project, Edit, and debug regions
	Fl_Tile* tile = new Fl_Tile(0,MENU_HEIGHT-2,w,h-MENU_HEIGHT-2);

	virtualt_prefs.get("IdeTreeWidth", ideTreeWidth, 198);

	/*
	============================================
	Create region for Project tree
	============================================
	*/
	m_ProjWindow = new Fl_Window(2,MENU_HEIGHT-2,ideTreeWidth,h-75,"");
	m_ProjWindow->box(FL_DOWN_BOX);
	m_ProjWindow->color(background_color);

	// Create Tree control
	m_ProjTree = new Flu_Tree_Browser( 0, 0, ideTreeWidth, h-75 );
	m_ProjTree->box( FL_DOWN_FRAME );
	m_ProjTree->callback( projtree_callback );
	m_ProjTree->selection_mode( FLU_SINGLE_SELECT );
	m_ProjTree->animate(TRUE);
	m_ProjTree->leaf_text(hl_plain, m_ProjTree->leaf_font(), text_size);
	m_ProjTree->branch_text(hl_plain, m_ProjTree->branch_font(), text_size);
	Fl_Window* b = new Fl_Window(40, 20, 10, 10, "");
	b->hide();
	gPopup = new Fl_Menu_Button(0,0,100,400,"Popup");
	gPopup->type(Fl_Menu_Button::POPUP3);
	gPopup->menu(gGroupMenu);
	gPopup->selection_color(fl_rgb_color(80,80,255));
	gPopup->color(fl_rgb_color(240,239,228));
	gPopup->hide();
	b->end();
	m_ProjTree->hide();
	
	m_ProjWindow->resizable(m_ProjTree);
	m_ProjWindow->end();

	/*
	=================================================
	Create region and Child Window for editing files
	=================================================
	*/
	m_EditWindow = new Fl_Window(ideTreeWidth+2,MENU_HEIGHT-2,w-(ideTreeWidth+2),h - 75,"Edit");
	m_EditWindow->box(FL_DOWN_BOX);
	m_EditTabs = new Fl_Ide_Tabs(0, 0, m_EditWindow->w(), m_EditWindow->h()-1);
	m_EditTabs->hide();
	m_EditTabs->selection_color(fl_rgb_color(253, 252, 251));
	m_EditTabs->has_close_button(TRUE);
	m_EditTabs->end();
	m_EditWindow->resizable(m_EditTabs);
	m_EditWindow->end();

	/*
	=================================================
	Create region for Debug and output tabs
	=================================================
	*/
	m_TabWindow = new Fl_Window(0,MENU_HEIGHT-2+h-75,w,75-MENU_HEIGHT+2,"Tab");
	m_TabWindow->box(FL_DOWN_BOX);

	// Create a tab control
//	m_Tabs = new Fl_Tabs(0, 1, w, 75-MENU_HEIGHT);
	m_Tabs = new Fl_Ide_Tabs(0, 1, w, 75-MENU_HEIGHT+3);
	m_Tabs->selection_color(fl_rgb_color(253, 252, 251));

	/*
	====================
	Create build tab
	====================
	*/
	m_BuildTab = new Fl_Group(2, 0, w-3, 75-MENU_HEIGHT-22, " Build ");
	m_BuildTab->box(FL_DOWN_BOX);
	m_BuildTab->selection_color(FL_LIGHT2);
	m_BuildTab->color(background_color);

	// Create a Text Editor to show the disassembled text
	m_BuildTextDisp = new My_Text_Display(0, 0, w-3, 75-MENU_HEIGHT-22);
	m_BuildTextDisp->box(FL_DOWN_BOX);
	m_BuildTextDisp->textcolor(hl_plain);
	m_BuildTextDisp->color(background_color);
	m_BuildTextDisp->selection_color(FL_DARK_BLUE);
	m_BuildTextDisp->blink_enable(0);

	// Create a Text Buffer for the Text Editor to work with
	m_BuildTextBuf = new Fl_Text_Buffer();
	m_BuildTextDisp->buffer(m_BuildTextBuf);

	// Setup a callback for the text display to report errors
	m_BuildTextDisp->register_error_report(ide_error_report, this);

	// Show the Disassembling text to indicate activity
	if (text_size == 0)
		text_size = 12;
	m_BuildTextDisp->textfont(FL_COURIER);
	m_BuildTextDisp->textsize(text_size);
	m_BuildTextDisp->end();
	m_BuildTab->end();
	m_Tabs->insert(*m_BuildTab, 0);
	m_Tabs->resizable(m_BuildTab);
	
	/*
	====================
	Create Debug tab
	====================
	*/
	m_DebugTab = new Fl_Group(2, 0, w-3, 75-MENU_HEIGHT-22, " Debug ");
	m_DebugTab->box(FL_DOWN_BOX);
	m_DebugTab->selection_color(FL_LIGHT2);
	m_DebugTab->color(background_color);

	My_Text_Display* md = new My_Text_Display(0, 0, w-3, 75-MENU_HEIGHT-22);
	md->box(FL_DOWN_BOX);
	md->textcolor(hl_plain);
	md->color(background_color);
	md->selection_color(FL_DARK_BLUE);
	md->blink_enable(0);

	// Create a Text Buffer for the Text Editor to work with
	Fl_Text_Buffer *tb = new Fl_Text_Buffer();
	md->buffer(tb);

	// Show the Disassembling text to indicate activity
	if (text_size == 0)
		text_size = 12;
	md->textfont(FL_COURIER);
	md->textsize(text_size);
	md->end();
	m_DebugTab->end();

	m_Tabs->insert(*m_DebugTab,1);
	m_Tabs->resizable(m_DebugTab);
	
	/*
	====================
	Create watch tab
	====================
	*/
	m_WatchTab = new Fl_Group(2, 0, w-3, 75-MENU_HEIGHT-22, " Watch ");
	m_WatchTab->box(FL_NO_BOX);
	m_WatchTab->selection_color(FL_LIGHT2);
	m_WatchTab->color(background_color);

	// Create tiled window for auto and watch variables
	Fl_Tile* tile2 = new Fl_Tile(2, 0,w,75-MENU_HEIGHT-22);

	Fl_Box* box0 = new Fl_Box(2, 0,w/2-2,75-MENU_HEIGHT-22,"1");

	// Create a text display for this pane
	md = new My_Text_Display(0, 0, w-3, 75-MENU_HEIGHT-22);
	md->box(FL_DOWN_BOX);
	md->textcolor(hl_plain);
	md->color(background_color);
	md->selection_color(FL_DARK_BLUE);
	md->blink_enable(0);

	// Create a Text Buffer for the Text Editor to work with
	tb = new Fl_Text_Buffer();
	md->buffer(tb);

	// Show the Disassembling text to indicate activity
	if (text_size == 0)
		text_size = 12;
	md->textfont(FL_COURIER);
	md->textsize(text_size);
	md->end();

	Fl_Box* box1 = new Fl_Box(w/2, 0,w/2,75-MENU_HEIGHT-22,"2");
	box1->box(FL_DOWN_BOX);
	box1->color(background_color);
	box1->labelcolor(hl_plain);
	box1->labelsize(36);
	box1->align(FL_ALIGN_CLIP);
	// Create a text display for this pane
	md = new My_Text_Display(0, 0, w-3, 75-MENU_HEIGHT-22);
	md->box(FL_DOWN_BOX);
	md->textcolor(hl_plain);
	md->color(background_color);
	md->selection_color(FL_DARK_BLUE);
	md->blink_enable(0);

	// Create a Text Buffer for the Text Editor to work with
	tb = new Fl_Text_Buffer();
	md->buffer(tb);

	// Show the Disassembling text to indicate activity
	if (text_size == 0)
		text_size = 12;
	md->textfont(FL_COURIER);
	md->textsize(text_size);
	md->end();

	box0->show();
	box1->show();
	Fl_Box* r2 = new Fl_Box(0,0,w,75-MENU_HEIGHT-22);
	tile2->resizable(r2);
	tile2->end();
	m_WatchTab->resizable(tile2);
	m_WatchTab->end();
	m_Tabs->insert(*m_WatchTab,2);
	m_Tabs->resizable(m_WatchTab);

	m_Tabs->end();
	m_TabWindow->resizable(m_Tabs);

	// Create a line display box
	Fl_Group*	g = new Fl_Group(0, 0, m_TabWindow->w(), m_TabWindow->h());
	Fl_Box* b1 = new Fl_Box(0, 0, m_TabWindow->w(), m_TabWindow->h() - 25);
	m_StatusBar.m_pLineBox = new Fl_Box(m_TabWindow->w() - 250, m_TabWindow->h() - 21, 100, 20, "");
	m_StatusBar.m_pColBox = new Fl_Box(m_TabWindow->w() - 150, m_TabWindow->h() - 21, 100, 20, "");
	m_StatusBar.m_pInsBox = new Fl_Box(m_TabWindow->w() - 50, m_TabWindow->h() - 21, 50, 20, "");
	m_StatusBar.line = m_StatusBar.col = m_StatusBar.ins_mode = -1;
	g->resizable(b1);
	g->end();

	m_TabWindow->end();

	// Set resize region
	Fl_Box* r = new Fl_Box(50,MENU_HEIGHT-2,w-50,h-75);
	r->hide();
	tile->resizable(r);
	tile->end();
	resizable(m);
	resizable(tile);

	int ideTabHeight;
	virtualt_prefs.get("IdeTabheight", ideTabHeight, 75);

	// Reposition the tile separators to be a little bigger than the minimum
	tile->position(ideTreeWidth,MENU_HEIGHT-2+h-75, ideTreeWidth,MENU_HEIGHT-2+h-(ideTabHeight));

	SetColors(hl_plain, background_color);

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

VT_Ide::~VT_Ide()
{
	m_EditTabs->clear();

	delete m_pReplaceDlg;
	delete m_pFindDlg;
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
	children = m_EditTabs->children();

	for (seq = 1; ; seq++)
	{
		// Try next sequence number to see if window already exists
		title.Format("Text%d", seq);

		for (child = 0; child < children; child++)
		{
			pWidget = (Fl_Widget*) m_EditTabs->child(child);
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
	Fl_Multi_Edit_Window*	mw = NULL;
	MString					rootpath;
	MString					title;

	// First get a pointer to the active (topmost) window
	mw = (Fl_Multi_Edit_Window*) m_EditTabs->value();
	if ((mw == NULL) || !mw->active())
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
	title = MakeTitle(mw->Filename());
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
	MString					rootpath;
	MString					title;

	// First get a pointer to the active (topmost) window
	mw = (Fl_Multi_Edit_Window*) m_EditTabs->value();
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
	title = MakeTitle(mw->Filename());
	mw->Title(title);
	add_recent_file_to_menu((const char *) mw->Filename());
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
	Flu_File_Chooser*		fc;
	int						count;
	MString					filename;


	fl_cursor(FL_CURSOR_WAIT);
	fc = new Flu_File_Chooser((const char *) m_LastDir, 
		"Assembly Files (*.asm,*.a85)|Header Files (*.inc,*.h)|Listing Files (*.lst)|Map Files (*.map)", 
		1, "Open File");
	fl_cursor(FL_CURSOR_DEFAULT);
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
	filename = fc->value();
	filename.Replace('\\', '/');

	// Get the current directory and save for future use
	m_LastDir = fc->get_current_directory();
	if (m_LastDir[m_LastDir.GetLength() - 1] == '/')
		m_LastDir = m_LastDir.Left(m_LastDir.GetLength() - 1);
	delete fc;
	OpenFile(filename);
}

void VT_Ide::OpenFile(const char *file)
{
	MString					title;
	MString					rootpath;

	if (m_ActivePrj == NULL)
		rootpath = path;
	else
		rootpath = m_ActivePrj->m_RootPath;

	title = MakeTitle(file);

	// Determine if file is already open 
	int children = m_EditTabs->children();
	for (int c = 0; c < children; c++)
	{
		Fl_Widget* pWidget = (Fl_Widget*) m_EditTabs->child(c);
		if (strcmp((const char *) title, pWidget->label()) == 0)
		{
			// File already open...bring file to foreground
			m_EditTabs->value((Fl_Group *) pWidget);
			pWidget->take_focus();
			return;
		}
	}

	// Create a new edit window for the file
	NewEditWindow(title, file);
	add_recent_file_to_menu(file);
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

	// First get a pointer to the active (topmost) window
	mw = (Fl_Multi_Edit_Window*) m_EditTabs->value();
	if (mw == NULL)
		return;
	
	My_Text_Editor::kf_copy(0, mw);
}

/*
=============================================================
Undo routine handles the Edit->Undo menu item.
=============================================================
*/
void VT_Ide::Undo(void)
{
	Fl_Multi_Edit_Window*	mw;

	// First get a pointer to the active (topmost) window
	mw = (Fl_Multi_Edit_Window*) m_EditTabs->value();
	if (mw == NULL)
		return;
	
	My_Text_Editor::kf_undo(0, mw);
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

	// First get a pointer to the active (topmost) window
	mw = (Fl_Multi_Edit_Window*) m_EditTabs->value();
	if (mw == NULL)
		return;
	
	My_Text_Editor::kf_cut(0, mw);
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

	// First get a pointer to the active (topmost) window
	mw = (Fl_Multi_Edit_Window*) m_EditTabs->value();
	if (mw == NULL)
		return;
	
	My_Text_Editor::kf_paste(0, mw);
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

	// First get a pointer to the active (topmost) window
	mw = (Fl_Multi_Edit_Window*) m_EditTabs->value();
	if (mw == NULL)
		return;

	m_pFindDlg->m_pFindDlg->show();
	m_pFindDlg->m_pFind->take_focus();
	m_pFindDlg->m_pFind->selectall();
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
	const char *			pFind;

	// First get a pointer to the active (topmost) window
	mw = (Fl_Multi_Edit_Window*) m_EditTabs->value();
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
	m_Search = pFind;
	if (!mw->ForwardSearch(pFind, m_pFindDlg->m_pMatchCase->value()))
	{
		// Save the current position and search from beginning of file
		int pos = mw->insert_position();
		mw->insert_position(0);
		if (!mw->ForwardSearch(pFind, m_pFindDlg->m_pMatchCase->value()))
		{
			// If still not found, report not found
			mw->insert_position(pos);
			fl_alert("Search string %s not found", pFind);
			m_pFindDlg->m_pFind->take_focus();
		}
	}

	// Hide the dialog box
	mw->take_focus();
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
	const char *			pFind;
	const char *			pReplace;

	// First get a pointer to the active (topmost) window
	mw = (Fl_Multi_Edit_Window*) m_EditTabs->value();
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
	const char *			pFind;
	const char *			pReplace;

	// First get a pointer to the active (topmost) window
	mw = (Fl_Multi_Edit_Window*) m_EditTabs->value();
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
	}

	// Close all opened files
	if (!CloseAllFiles())
		return;

	// Delete the active project
	if (m_ActivePrj != NULL)
	{
		delete m_ActivePrj;
		m_ActivePrj = NULL;
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
	pGroup->m_Name = "Linker Script";
	pGroup->m_Filespec = "*.lkr";
	m_ActivePrj->m_Groups.Add(pGroup);

	pGroup = new VT_IdeGroup;
	pGroup->m_Name = "Object Files";
	pGroup->m_Filespec = "*.obj";
	m_ActivePrj->m_Groups.Add(pGroup);

	// Get root directory
	pRootDir = pProj->getLocation();
	if (pRootDir[0] == '.')
	{
		char ch;

		strcpy(fullPath, path);
		ch = path[strlen(path)-1];
		if ((ch != '\\') && (ch != '/'))
			strcat(fullPath, "/");
		strcat(fullPath, &pRootDir[2]);
		m_ActivePrj->m_RootPath = fullPath;
	}
	else
	{
		m_ActivePrj->m_RootPath = pRootDir;
	}

	m_ActivePrj->m_RootPath.Replace('\\', '/');

	// Check if path ends with '/'
	if (m_ActivePrj->m_RootPath[m_ActivePrj->m_RootPath.GetLength()-1] != '/')
		m_ActivePrj->m_RootPath = m_ActivePrj->m_RootPath + (char *) "/";

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
	if (m_ActivePrj->m_ProjectType == VT_PROJ_TYPE_CO ||
		m_ActivePrj->m_ProjectType == VT_PROJ_TYPE_ROM)
			m_ActivePrj->m_AutoLoad = 1;
	else
		m_ActivePrj->m_AutoLoad = 0;

	// Set the output anme based on project type
	MString		newExt;
	switch (m_ActivePrj->m_ProjectType)
	{
		case VT_PROJ_TYPE_CO:	newExt = ".co";  break;
		case VT_PROJ_TYPE_OBJ:	newExt = ".obj"; break;
		case VT_PROJ_TYPE_ROM:	newExt = ".hex"; break;
		case VT_PROJ_TYPE_LIB:	newExt = ".lib"; break;
		case VT_PROJ_TYPE_BA:	newExt = ".bas"; break;
		default:				newExt = "";     break;
	}
	m_ActivePrj->m_OutputName = m_ActivePrj->m_Name + newExt;

	// Save the target model
	m_ActivePrj->m_TargetModel = pProj->getTargetModel();
	gRootpath = m_ActivePrj->m_RootPath;
	m_LastDir = gRootpath;

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
Closes all files, asking to save if any are dirty.  Returns
TRUE if all files closed, FALSE if CANCEL selected.
=============================================================
*/
int VT_Ide::CloseAllFiles(void)
{
	int						c, count;
	int						saveAll = FALSE;
	Fl_Multi_Edit_Window*	mw;

	// Loop through all opened tabs
	count = m_EditTabs->children();
	for (c = count-1; c >= 0; c--)
	{
		mw = (Fl_Multi_Edit_Window *) m_EditTabs->child(c);
		if (mw->IsModified())
		{
			// Check if closeAll already selected
			if (saveAll)
			{
				// Save teh file
				mw->SaveFile(gRootpath);
			}
			else
			{
				// Ask if we should save the file
				int ans = fl_choice("Save file %s?", "Cancel", "Yes", "No", (const char *) mw->Title());

				// Test for CANCEL
				if (ans == 0)
					return FALSE;

				if (ans == 1)
					mw->SaveFile(gRootpath);
			}
		}

		// Hide the tab
		mw->hide();
		mw->parent()->do_callback(mw, FL_IDE_TABS_CLOSE);
	}
	return TRUE;
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
	char			fullPath[512];

	// Check if m_ActiveProj is valid
	if (m_ActivePrj == NULL)
		return;

	m_ActivePrj->SaveProject();
	SaveProjectIdeSettings();

	// Create the project file path and add to recent
	sprintf(fullPath, "%s/%s.prj", (const char *) m_ActivePrj->m_RootPath,
		(const char *) m_ActivePrj->m_Name);
	add_recent_project_to_menu(fullPath);

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
	Flu_File_Chooser	*FileWin;
	int					count;

	fl_cursor(FL_CURSOR_WAIT);
	FileWin = new Flu_File_Chooser("./Projects","*.prj",1,"Open Project file");
	fl_cursor(FL_CURSOR_DEFAULT);
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
	if (FileWin->value(0) == 0)
	{
		delete FileWin;
		return;
	}
	filename = FileWin->value(0);
	filename.Replace('\\', '/');
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
	}

	// Close any opened files
	if (!CloseAllFiles())
		return;

	if (m_ActivePrj != NULL)
	{
		// Delete existing project
		delete m_ActivePrj;
		m_ActivePrj = 0;
	}

	OpenProject((const char *) filename);
}

/*
=============================================================
The BuildTreeControl routine builds / rebuilds the Project
Tree control from the active Project structure.
=============================================================
*/
void VT_Ide::OpenProject(const char *filename)
{
	// Try to parse the file
	if (ParsePrjFile(filename))
	{
		// Now build the TreeControl
		BuildTreeControl();
		add_recent_project_to_menu(filename);

		// Now open the project files
		ReadProjectIdeSettings();
		gRootpath = m_ActivePrj->m_RootPath;
		m_LastDir = gRootpath;
	}
	else
	{
		// Report parse error
	}
}

/*
=============================================================
This routine saves the project's current file tabs and the
status of open Tree Browser items.
=============================================================
*/
void VT_Ide::SaveProjectIdeSettings()
{
	FILE*					fd;
	char					fullPath[512];
	Flu_Tree_Browser::Node* pNode;
	Fl_Multi_Edit_Window*	mw;
	Fl_Multi_Edit_Window*	mw_selected = NULL;
	int						count, c;

	// Check if m_ActiveProj is valid
	if (m_ActivePrj == NULL)
		return;

	// Create the path
	sprintf(fullPath, "%s/%s.ide", (const char *) m_ActivePrj->m_RootPath,
		(const char *) m_ActivePrj->m_Name);

	// Try to open the file for write mode
	if ((fd = fopen(fullPath, "w+")) == NULL)
	{
		// Error opening file!!
		fl_alert("Error opening project IDE file for write mode!");
		return;
	}

	// Write open tree broweser information
	pNode = this->m_ProjTree->get_root();
	if (pNode != NULL)
	{
		pNode = pNode->next();
		while (pNode)
		{
			if (pNode->is_branch() && pNode->open())
			{
				fprintf(fd, "OPEN_TREE=%s\n", pNode->find_path());
			}
			pNode = pNode->next();
		}
	}

	// Write open file tabs information
	count = m_EditTabs->children();
	for (c = 0; c < count; c++)
	{
		mw = (Fl_Multi_Edit_Window *) m_EditTabs->child(c);
		MString filename = mw->Filename();
		if (filename.GetLength() == 0)
			continue;
		fprintf(fd, "OPEN_FILE=%s\t%d\n", (const char *) filename,
			mw->insert_position());
		if (m_EditTabs->value() == mw)
			mw_selected = mw;
	}

	// Save the selected tab
	if (mw_selected != NULL)
	{
		MString filename = mw_selected->Filename();
		fprintf(fd, "SELECTED=%s\n", (const char *) filename);
	}

	// Close the IDE file
	fclose(fd);
}

/*
=============================================================
This routine restores the project's current file tabs and the
status of open Tree Browser items.
=============================================================
*/
void VT_Ide::ReadProjectIdeSettings()
{
	char			line[300];
	char			*param, *value;
	FILE*			fd;
	int				c, tabloc, count;
	MString			sParam, sSelected;
	const char *	sPtr;
	int				pos;

	// Create the path
	sprintf(line, "%s/%s.ide", (const char *) m_ActivePrj->m_RootPath,
		(const char *) m_ActivePrj->m_Name);

	// Try to open the file for write mode
	if ((fd = fopen(line, "r")) == NULL)
	{
		// Error opening file!!
		return;
	}

	// Loop through all lines in file
	while (fgets(line, sizeof(line), fd) != 0)
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
			value = (char *) "";
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
		if (strcmp(sPtr, "OPEN_TREE") == 0)
		{
			Flu_Tree_Browser::Node* n = m_ProjTree->find(value);
			if (n)
			{
				n->open(true);
			}
		}
		else if (strcmp(sPtr, "OPEN_FILE") == 0)
		{
			// Search for tab separator for insertion line number
			tabloc = 0;
			for (c = strlen(value) - 1; c > 0; c--)
			{
				if (value[c] == '\t')
				{
					tabloc = c;
					break;
				}
			}

			// If a tab was found, get the insertion point and terminate file
			if (tabloc != 0)
			{
				pos = atoi(&value[tabloc + 1]);
				value[tabloc] = '\0';
			}

			// Try to open the file
			MString	file = value;
			MString title = MakeTitle(file);
			Fl_Multi_Edit_Window* mw = NewEditWindow(title, file, FALSE);

			// Position the file to the last location
			if (tabloc)
			{
				mw->insert_position(pos);
				mw->show_insert_position();
				mw->take_focus();
				mw->show_cursor();
			}
		}
		else if (strcmp(sPtr, "SELECTED") == 0)
		{
			// Save the name of the selected file until all files loaded
			sSelected = value;
		}
	}

	// Re-select the selcted file
	if (sSelected.GetLength() != 0)
	{
		count = m_EditTabs->children();
		for (c = 0; c < count; c++)
		{
//			Fl_Multi_Edit_Window* mw = (Fl_Multi_Edit_Window *) 
//				((Fl_Group *) m_EditTabs->child(c))->child(0);
			Fl_Multi_Edit_Window* mw = (Fl_Multi_Edit_Window *) 
				m_EditTabs->child(c);
			if (mw->Filename() == sSelected)
			{
//				m_EditTabs->value(mw->parent());
				m_EditTabs->value(mw);
				mw->take_focus();
				break;
			}
		}
	}

	fclose(fd);
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

	m_ProjTree->clear();

	// set the default leaf icon to be a blue dot
	m_ProjTree->leaf_icon( &gTextDoc );
	m_ProjTree->insertion_mode( FLU_INSERT_SORTED );

	// use the default branch icons (yellow folder)
	m_ProjTree->branch_icons( NULL, NULL );

	// Clear old contents of the tree
	m_ProjTree->clear();

	// Set the Root name
	temp = m_ActivePrj->m_Name;
	temp += (char *) " files";
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
			addStr += (char *) "/";
			n = m_ProjTree->add(addStr);
			if (n)
			{
				n->user_data(pGroup);
				n->label_color(hl_plain);
				n->label_size(text_size);
				pGroup->m_Node = n;
			}

			addStr += (char *) "%s";
			
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
			int index = pSource->m_Name.ReverseFind((char *) "/");
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
				n->label_color(hl_plain);
				n->label_size(text_size);
				pSource->m_Node = n;
			}
		}
	}

	m_ProjTree->open(TRUE);
	m_ProjTree->show();
	m_ProjTree->redraw();
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
		addStr += (char *) "/";
		n = m_ProjTree->add(addStr);
		if (n)
		{
			n->user_data(pGroup);
			pGroup->m_Node = n;
			n->label_size(text_size);
		}

		addStr += (char *) "%s";
		
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
		int index = pSource->m_Name.ReverseFind((char *) "/");
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
			n->label_size(text_size);
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
	if (m_ActivePrj != 0)
		delete m_ActivePrj;

	m_ActivePrj = new VT_Project;

	// Set the RootPath
	temp = name;
	// Get just the path
	index = temp.ReverseFind((char *) "/");
	if (index == 0)
		m_ActivePrj->m_RootPath = path;
	else
		m_ActivePrj->m_RootPath = temp.Left(index);

	index = 0;

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
			value = (char *) "";
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
				newgrp->m_pParent = pGroup;

				// Insert groups alphabetically before sources
				len = pGroup->m_Objects.GetSize();
				for (c = 0; c < len; c++)
				{
					pIns = pGroup->m_Objects[c];
					// Stop at sign of first source
					if (strcmp(pIns->GetClass()->m_ClassName, "VT_IdeSource") == 0)
						break;

					if (newgrp->m_Name < ((VT_IdeGroup *) pIns)->m_Name)
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
		else if (strcmp(sPtr, "OUTPUTNAME") == 0)
		{
			if (value != 0)
				m_ActivePrj->m_OutputName = value;
		}
		else if (strcmp(sPtr, "LINKSCRIPT") == 0)
		{
			if (value != 0)
				m_ActivePrj->m_LinkScript = value;
		}
	}

	// Set the RootPath
	temp = name;
	// Get just the path
	index = temp.ReverseFind((char *) "/");
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
			int count = pGroup->m_Objects.GetSize();
			
			// If group is empty, then don't ask, just delete it
			if (count == 0)
				ans = 1;
			else
				ans = fl_choice("Delete group %s and all its members?", "No", "Yes", NULL, (const char *) pGroup->m_Name);
			if (ans == 1)
			{
				// First delete all subitems from the group
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
				if (pGroup->m_pParent != NULL)
				{
					VT_IdeGroup* pParentGroup = pGroup->m_pParent;
					count = pParentGroup->m_Objects.GetSize();
					for (int c = 0; c < count; c++)
					{
						if (pGroup == (VT_IdeGroup*) pParentGroup->m_Objects[c])
						{
							pParentGroup->m_Objects.RemoveAt(c, 1);
							break;
						}
					}
				}
				else
				{
					count = m_ActivePrj->m_Groups.GetSize();
					for (int c = 0; c < count; c++)
					{
						if (pGroup == (VT_IdeGroup*) m_ActivePrj->m_Groups[c])
						{
							m_ActivePrj->m_Groups.RemoveAt(c, 1);
							break;
						}
					}
				}
				delete pGroup;

				// Mark the project dirty
				m_ActivePrj->m_Dirty = 1;
			}
		}
		else
		{
			if (strcmp(pObj->GetClass()->m_ClassName, "VT_IdeSource") != 0)
			{
				MString err;
				err.Format("Internal error on line %d of file %s\n", __LINE__, __FILE__);
				show_error((const char *) err);
				return;
			}

			// We are deleting a source entry
			VT_IdeSource* pSrc = (VT_IdeSource*) pObj;
			Flu_Tree_Browser::Node* pNext = pSrc->m_Node->next();

			// Delete the source from the tree
			m_ProjTree->remove(pSrc->m_Node);
			if (pNext != NULL)
				m_ProjTree->set_hilighted(pNext);

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
	int		c, count;

	if (n->is_root())
	{
		count = sizeof(gRootMenu) / sizeof(Fl_Menu_Item);
		for (c = 0; c < count; c++)
			gRootMenu[c].user_data_ = n;
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
			count = sizeof(gGroupMenu) / sizeof(Fl_Menu_Item);
			for (c = 0; c < count; c++)
				gGroupMenu[c].user_data_ = n;
			gPopup->menu(gGroupMenu);
		}
		else
		{
			// Select source popup menu
			count = sizeof(gSourceMenu) / sizeof(Fl_Menu_Item);
			for (c = 0; c < count; c++)
				gSourceMenu[c].user_data_ = n;
			gPopup->menu(gSourceMenu);
		}
	}
	// Add label to popup menu & display
	make_current();
//	gPopup->label(n->label());
	gPopup->label(NULL);
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
	name += (char *) "/";

	// Test if inserting at root
	if (n == 0)
	{
		root = 1;
		n = m_ProjTree->get_root();
	}
	else if (n->parent() == NULL)
		root = 1;
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
	newNode->label_size(text_size);
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
		pNewGrp->m_pParent = pGroup;
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
	Flu_File_Chooser		*fc;		/* File Chooser */
	int					count, c;
	int					len, index, x;
	MString 			filename, relPath;
	MString				temp, path;
	MStringArray		errFiles;
	VT_IdeGroup*		pGroup = 0;
	VT_IdeSource*		pSource = 0;
	VTObject*			pIns;

	/* Create a new File Chooser object */
	fl_cursor(FL_CURSOR_WAIT);
	fc = new Flu_File_Chooser((const char *) m_LastDir, "Source Files (*.{asm,inc,h})", 
		Flu_File_Chooser::MULTI, "Add Files to Project");
	fl_cursor(FL_CURSOR_DEFAULT);
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
	m_LastDir = fc->get_current_directory();
	if (m_LastDir[m_LastDir.GetLength() - 1] == '/')
		m_LastDir = m_LastDir.Left(m_LastDir.GetLength() - 1);

	// Calculate relative path for files

	/* Loop through all files selected and try to add to project */
	for (c = 0; c < count; c++)
	{
		// Get next filename from the FileChooser
		filename = fc->value(c);
		filename.Replace('\\', '/');

		// Get just the filename
		index = filename.ReverseFind('/');
		if (index == 0)
			temp = filename;
		else
			temp = filename.Right(filename.GetLength() - index - 1);

		// Check if entry aleady exists
		if (m_ActivePrj->TestIfFileInProject(filename))
//		if (n->find((const char *) temp) != NULL)
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
		{
			pSource->m_Node->user_data(pSource);
			n->open(true);
		}
		
		m_ActivePrj->m_Dirty = 1;
	}

	m_ProjTree->redraw();
	/* Delete the file chooser */
	delete fc;

	// Test for errors during insertion
	if (errFiles.GetSize() > 0)
	{
		MString err;
		MString files;
		for (x = 0; x < errFiles.GetSize(); x++)
		{
			if (x == errFiles.GetSize() - 1)
				files += errFiles[x];
			else
				files += errFiles[x] + (char *) ", ";
		}
		if (errFiles.GetSize() > 1)
			err.Format("Files %s already in project!", (const char *) files);
		else
			err.Format("File %s already in project!", (const char *) files);
		fl_alert("%s", (const char *) err);
	}
	return;
}

void VT_Ide::FolderProperties(Flu_Tree_Browser::Node* n)
{
}

/*
=============================================================
Open the tree item in the edit window
=============================================================
*/
void VT_Ide::OpenTreeFile(Flu_Tree_Browser::Node* n)
{
	VT_IdeSource*	pSource;
	MString			file;
	MString			title;
	int				children, c;
	Fl_Widget*		pWidget;

	pSource = (VT_IdeSource *) n->user_data();
	if (pSource == NULL)
		return;

	// Process the open request
	if (pSource->m_Name.Left(2) == "./")
		file = pSource->m_Name.Right(pSource->m_Name.GetLength()-2);
	else
		file = pSource->m_Name;
	title = MakeTitle(file);

	// Check if the file is already open
	children = m_EditTabs->children();
	for (c = 0; c < children; c++)
	{
		pWidget = (Fl_Widget*) m_EditTabs->child(c);
		if (strcmp((const char *) title, pWidget->label()) == 0)
		{
			// File already open...bring file to foreground
			m_EditTabs->value(pWidget);
			pWidget->take_focus();
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
Fl_Multi_Edit_Window* VT_Ide::NewEditWindow(const MString& title, const MString& file,
	int addToRecentFiles)
{
	int 					x, y, w, h;
	int						c;
	Fl_Multi_Edit_Window*	mw;

	// Test if this file already opened in a tab
	for (c = m_EditTabs->children()-1; c >= 0; c--)
	{
		mw = (Fl_Multi_Edit_Window *) m_EditTabs->child(c);
		if (mw->Title() == title)
		{
			mw->take_focus();
			return mw;
		}
	}

	/* Calculate location of next window */
	x = 1;
	y = TAB_HEIGHT;
	h = m_EditTabs->h() - TAB_HEIGHT;
	w = m_EditTabs->w()-1;

	/* Create a group tab */
//	Fl_Group* g = new Fl_Group(x, y, w, h, (const char *) title);

	/* Now Create window */
	mw = new Fl_Multi_Edit_Window(x, y, w, h, 
		(const char *) title);
	mw->buffer()->tab_distance(4);
	if (file != "")
	{
		mw->OpenFile((const char *) file);
		if (addToRecentFiles)
			add_recent_file_to_menu((const char *) file);
	}
	mw->end();

	// Insert new window in EditWindow
	m_EditTabs->insert(*mw, m_EditTabs->children());
	m_EditTabs->resizable(m_EditTabs->child(m_EditWindow->children()-1));
	mw->add_status_bar(&m_StatusBar);
	mw->color(background_color);
	mw->mCursor_color = hl_plain;
	mw->textsize(text_size);
	mw->textcolor(hl_plain);
	mw->selection_color(fl_color_average(FL_DARK_BLUE, FL_WHITE, .85f));
	mw->utility_margin_color(20, fl_rgb_color(253, 252, 251));
	mw->show();
	m_EditTabs->value(m_EditTabs->child(m_EditTabs->children()-1));
	if (m_EditTabs->children() == 1)
		m_EditTabs->show();

	// Call show again to bring window to front
	mw->take_focus();

	return mw;
}

void VT_Ide::AssembleTreeFile(Flu_Tree_Browser::Node* n)
{
}

void VT_Ide::TreeFileProperties(Flu_Tree_Browser::Node* n)
{
}

/*
=============================================================================
Standard output support for echo messages from assembler / linker
=============================================================================
*/
void ideStdoutProc(void *pContext, const char *msg)
{
	VT_Ide*		pIde = (VT_Ide*) pContext;

	if ((pIde != NULL) && (msg != NULL))
		pIde->Stdout(msg);
}

void VT_Ide::Stdout(const char *msg)
{
	m_BuildTextBuf->append(msg);
	Fl::check();
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
	MString			linkerFiles;
	VTLinker		linker;
	MString			linkerScript;
	int				linkerScriptFound = false;

	// Be sure we have an active project
	if (m_ActivePrj == NULL)
		return;

	// Clear the Build tab at the bottom
	m_BuildTextBuf ->remove(0, m_BuildTextBuf->length());
	Fl::check();

	// Configure the assembler
	assembler.SetRootPath(m_ActivePrj->m_RootPath);
	assembler.SetAsmOptions(m_ActivePrj->m_AsmOptions);
	assembler.SetIncludeDirs(m_ActivePrj->m_IncludePath);
	assembler.SetDefines(m_ActivePrj->m_Defines);
	assembler.SetProjectType(m_ActivePrj->m_ProjectType);
	assembler.SetStdoutFunction(this, ideStdoutProc);

	m_BuildTextBuf->append("Assembling...\n");
	Fl::check();

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

				// Add this file to the list of files to be linked
				temp = pSource->m_Name.Left(pSource->m_Name.GetLength() - 4) + (char *) ".obj";
				linkerFiles += temp + (char *) ",";
			}

			// Try to assemble the file
			if (assemblyNeeded)
			{
				// Display build indication
				index = pSource->m_Name.ReverseFind('/');
				temp = pSource->m_Name.Right(pSource->m_Name.GetLength()-index-1);
				text.Format("%s\n", (const char *) temp);
				m_BuildTextBuf->append((const char *) text);
				Fl::check();

				// Try to assemble this file
				filename = MakePathAbsolute(pSource->m_Name, m_ActivePrj->m_RootPath);
				assembler.ResetContent();
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

			// Look for linker scripts while we are looping through the tree
			if (temp == ".lkr")
			{
				if (pSource->m_Name[0] == '/' || pSource->m_Name[1] == ':')
					linkerScript = pSource->m_Name;
				else
				{
					linkerScript = m_ActivePrj->m_RootPath + (char *) "/"+ pSource->m_Name;
				}
				linkerScriptFound = true;
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
		Fl::check();

		// Setup the linker
		linker.SetRootPath(m_ActivePrj->m_RootPath);
		linker.SetLinkOptions(m_ActivePrj->m_LinkOptions);
		linker.SetObjDirs(m_ActivePrj->m_LinkPath);
		linker.SetProjectType(m_ActivePrj->m_ProjectType);
		linker.SetOutputFile(m_ActivePrj->m_OutputName);
		if (linkerScriptFound)
			linker.SetLinkerScript(linkerScript);
		else
			linker.SetLinkerScript(m_ActivePrj->m_LinkScript);
		linkerFiles = linkerFiles + (char *) "," + m_ActivePrj->m_LinkLibs;
		linker.SetObjFiles(linkerFiles);
		linker.SetStdoutFunction(this, ideStdoutProc);
		
		// Now finally perform the link operation
		linker.Link();

		errors = linker.GetErrors();
		errorCount = errors.GetSize();
		totalErrors += errorCount;
		for (err = 0; err < errorCount; err++)
		{
			m_BuildTextBuf->append((const char *) errors[err]);
			m_BuildTextBuf->append("\n");
		}

		// If success, print a success message
		if (totalErrors == 0)
		{
			// Print the message
			temp.Format("Success, code size=%d, data size=%d\n",
				linker.TotalCodeSpace(), linker.TotalDataSpace());
			m_BuildTextBuf->append((const char *) temp);

			// Check if we generated a MAP file and the MAP file is opened

			// Now check if auto-reload of the generated file is enabled
			if (m_ActivePrj->m_AutoLoad)
			{
				switch (m_ActivePrj->m_ProjectType)
				{
					case VT_PROJ_TYPE_CO:	
						{
							// Test if we should update himem
							if (m_ActivePrj->m_UpdateHIMEM)
							{
								// Set Himem
								set_memory16(gStdRomDesc->sHimem, linker.GetStartAddress());

								// May need to change some other parameters??
							}

							// Load file from host to emulation
							MString coPath = m_ActivePrj->m_RootPath + (char *) "/" + 
								m_ActivePrj->m_OutputName;
							cb_LoadFromHost(NULL, (void *) (const char *) coPath);
						}
						break;

					case VT_PROJ_TYPE_OBJ:
						break;

					case VT_PROJ_TYPE_ROM:
						break;

					case VT_PROJ_TYPE_LIB:
						break;

					case VT_PROJ_TYPE_BA:
						break;

					default:
						break;
				}
			}
		}
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
MString VT_Ide::MakeTitle(const MString& path)
{
	MString		temp;
	int			index;

	// Search for the last directory separator token
	index = path.ReverseFind('/');
	if (index == -1)
		return path;

	return path.Right(path.GetLength() - index - 1);
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
	if (path.Find((char *) "/", 0) == -1)
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
#ifdef WIN32
		if (tolower(path[c]) != tolower(relTo[c]))
			break;
#else
		if (path[c] != relTo[c])
			break;
#endif
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
			temp += (char *) "../";				// Add another 'prev dir' indicator
			break;						// At last branch...exit
		}
		// Check for a directory specifier
		if (relTo[c] == '/')
		{
			temp += (char *) "../";
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
		else 
		{
			index = newRel.ReverseFind('\\');
			if (index != -1)
				newRel = newRel.Left(index - 1);
			else
				break;
		}
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
	Fl_Multi_Edit_Window*	mw;

	// First get a pointer to the active (topmost) window
	mw = (Fl_Multi_Edit_Window*) gpIde->m_EditTabs->value();
	if (mw != NULL)
		mw->take_focus();
	gpIde->show();
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

void cb_find_edit(Fl_Widget* w, void *opaque)
{
	printf("Edit callback\n");
}

/*
================================================================================
VT_FindDlg routines below.
================================================================================
*/
VT_FindDlg::VT_FindDlg(class VT_Ide* pParent)
{
	// Create Find What combo list
	m_pFindDlg = new Fl_Window(400, 300, "Find");
	Fl_Box *o = new Fl_Box(20, 10, 100, 25, "Find what:");
	o->align(FL_ALIGN_LEFT | FL_ALIGN_INSIDE);
	m_pFind = new Flu_Combo_List(20, 35, 360, 25, "");
	m_pFind->align(FL_ALIGN_LEFT);
	m_pFind->input_callback(cb_find_next, this);
	m_pFindDlg->resizable(m_pFind);

	// Create Find In choice box
	o = new Fl_Box(20, 70, 100, 20, "Find in:");
	o->align(FL_ALIGN_LEFT | FL_ALIGN_INSIDE);
	m_pFindIn = new Fl_Choice(20, 95, 360, 25, "");
	m_pFindIn->add("Current Window");
	m_pFindIn->add("Current Selection");
	m_pFindIn->value(0);
	m_pFindDlg->resizable(m_pFindIn);

	o = new Fl_Box(20, 135, 360, 110, "Find options");
	o->box(FL_ENGRAVED_BOX);
	o->labeltype(FL_ENGRAVED_LABEL);
	o->align(FL_ALIGN_TOP_LEFT | FL_ALIGN_INSIDE);
	m_pFindDlg->resizable(o);

	m_pBackward = new Fl_Check_Button(40, 160, 120, 25, "Search backward");
	m_pMatchCase = new Fl_Check_Button(40, 185, 100, 25, "Match case");
	m_pWholeWord = new Fl_Check_Button(40, 210, 140, 25, "Match whole word");

	o = new Fl_Box(20, 250, 50, 45, "");
	m_pFindDlg->resizable(o);

	m_pNext = new Flu_Return_Button(160, 255, 100, 25, "Find Next");
	m_pNext->callback(cb_find_next, this);

	m_pCancel = new Flu_Button(280, 255, 100, 25, "Close");
	m_pCancel->callback(cb_replace_cancel, this);
	o = new Fl_Box(20, 295, 360, 2, "");
	m_pFindDlg->resizable(o);

	m_pFindDlg->callback(cb_replace_cancel, this);
	m_pFindDlg->end();
	m_pFindDlg->set_non_modal();

	m_pParent = pParent;
}
