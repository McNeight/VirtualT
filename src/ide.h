/* ide.h */

/* $Id: ide.h,v 1.5 2013/02/05 01:20:59 kpettit1 Exp $ */

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


#ifndef _IDE_H_
#define _IDE_H_

#include <FL/Fl.H>
#include <FL/Fl_Choice.H>
#include <FL/Fl_Double_Window.H>
#include <FL/Fl_Tabs.H>
#include "FLU/Flu_Combo_List.h"
#include "FLU/Flu_Return_Button.h"
#include "FLU/Flu_Button.h"
#include "FLU/Flu_Tree_Browser.h"
#include "FLU/flu_pixmaps.h"
#include "FLU/Flu_File_Chooser.h"

#include "MString.h"
#include "vtobj.h"
#include "project.h"
#include "My_Text_Editor.h"
#include "idetabs.h"

void cb_Ide(Fl_Widget* w, void*) ;


#ifndef MENU_HEIGHT
#define MENU_HEIGHT	32
#endif

class VT_IdeSource : public VTObject
{
	DECLARE_DYNCREATE(VT_IdeSource);
public:
	VT_IdeSource()  {};

	MString						m_Name;
	Flu_Tree_Browser::Node*		m_Node;
	class VT_IdeGroup*			m_ParentGroup;
};

class VT_IdeGroup : public VTObject
{
	DECLARE_DYNCREATE(VT_IdeGroup);

public:
	VT_IdeGroup()	{m_Node = NULL; m_pParent = NULL; };
	~VT_IdeGroup();

	MString						m_Name;
	MString						m_Filespec;
	VTObArray					m_Objects;
	Flu_Tree_Browser::Node*		m_Node;
	VT_IdeGroup*				m_pParent;
};

class VT_ReplaceDlg
{
public:
	VT_ReplaceDlg(class VT_Ide* pParent);

	Fl_Window*		m_pReplaceDlg;
	Fl_Input*		m_pFind;
	Fl_Input*		m_pWith;
	Fl_Button*		m_pAll;
	Fl_Button*		m_pNext;
	Fl_Button*		m_pCancel;

	class VT_Ide*	m_pParent;

	char			search[256];
};

class VT_FindDlg
{
public:
	VT_FindDlg(class VT_Ide* pParent);

	Fl_Window*			m_pFindDlg;
	Flu_Combo_List*		m_pFind;
	Fl_Choice*			m_pFindIn;
	Fl_Check_Button*	m_pBackward;
	Fl_Check_Button*	m_pMatchCase;
	Fl_Check_Button*	m_pWholeWord;
	Flu_Button*			m_pNext;
	Flu_Button*			m_pCancel;

	class VT_Ide*		m_pParent;

	char				search[256];
};

class VT_Ide : public Fl_Window
{
public:
	VT_Ide(int x, int y, int w, int h, const char *title = 0);
	~VT_Ide();

// Methods
	virtual void	show();
	virtual int		handle(int event);

	void			NewProject(void);
	void			OpenProject(void);
	void			OpenProject(const char *filename);
	void			SaveProject(void);
	int				CloseAllFiles(void);
	void			SaveProjectIdeSettings(void);
	void			ReadProjectIdeSettings(void);
	void			BuildTreeControl(void);
	int				ParsePrjFile(const char *name);
	void			RightClick(Flu_Tree_Browser::Node* n);
	void			NewFolder(Flu_Tree_Browser::Node* n);
	void			NewFile(void);
	void			SaveFile(void);
	void			SaveAs(void);
	void			OpenFile(void);
	void			OpenFile(const char *file);
	void			Copy(void);
	void			Cut(void);
	void			Paste(void);
	void			Undo(void);
	void			Find(void);
	void			FindNext(void);
	void			Replace(void);
	void			ReplaceAll(void);
	void			ReplaceNext(void);
	void			AddFilesToFolder(Flu_Tree_Browser::Node* n);
	void			DeleteItem(Flu_Tree_Browser::Node* n);
	void			FolderProperties(Flu_Tree_Browser::Node* n);
	void			OpenTreeFile(Flu_Tree_Browser::Node* n);
	void			AssembleTreeFile(Flu_Tree_Browser::Node* n);
	void			TreeFileProperties(Flu_Tree_Browser::Node* n);
	void			BuildProject(void);
	void			CleanProject(void);
	void			SetColors(int fg, int bg);
	void			ShowProjectSettings(void);
	void			LoadPrefs(void);
	void			SavePrefs(void);
	MString			MakePathRelative(const MString& path, const MString& relTo);
	MString			MakePathAbsolute(const MString& path, const MString& relTo);
	MString			MakeTitle(const MString& path);
	MString			ProjectName(void);
	void			Stdout(const char *msg);
	int				ProjectDirty(void);
	void			ErrorReport(int lineNo, char* pFilename);
	int				SpliterHeight(void) { return m_ProjWindow->h(); }
	int				SpliterWidth(void) { return m_ProjWindow->w(); }

	Fl_Double_Window*		m_EditWindow;
	Fl_Ide_Tabs*	m_EditTabs;
    Fl_Box*         m_TabNoBlinkBox;
	VT_ReplaceDlg*	m_pReplaceDlg;
	VT_FindDlg*		m_pFindDlg;

protected:
	virtual void	draw();
	void			AddGroupToTree(VTObject *pObj, const char *fmt);
	class Fl_Multi_Edit_Window*	NewEditWindow(const MString& title, const MString& file,
						int addToRecentFiles = TRUE);

	Fl_Double_Window*					m_ProjWindow;
	Flu_Tree_Browser*			m_ProjTree;
	Fl_Window*					m_TabWindow;
	Fl_Ide_Tabs*					m_Tabs;
//	Fl_Tabs*					m_Tabs;
	Fl_Group*					m_BuildTab;
	My_Text_Display*			m_BuildTextDisp;
	My_Text_Buffer*				m_BuildTextBuf;
	Fl_Group*					m_DebugTab;
	Fl_Group*					m_WatchTab;
	Flu_Tree_Browser::Node*		m_Node;
	VT_Project*					m_ActivePrj;
	MString						m_LastDir;
	int							m_OpenLocation;
	StatusBar_t					m_StatusBar;
	MString						m_Search;
};

#endif

