/* ide.h */

/* $Id: display.h,v 1.1.1.1 2004/08/05 06:46:12 kpettit1 Exp $ */

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

#include "MString.h"
#include "vtobj.h"
#include "project.h"

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
	VT_IdeGroup()	{};
	~VT_IdeGroup();

	MString						m_Name;
	MString						m_Filespec;
	VTObArray					m_Objects;
	Flu_Tree_Browser::Node*		m_Node;
};

class VT_ReplaceDlg
{
public:
	VT_ReplaceDlg(class VT_Ide* pParent);
	~VT_ReplaceDlg();

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
	~VT_FindDlg();

	Fl_Window*			m_pFindDlg;
	Fl_Input*			m_pFind;
	Fl_Round_Button*	m_pForward;
	Fl_Round_Button*	m_pBackward;
	Fl_Button*			m_pNext;
	Fl_Button*			m_pCancel;

	class VT_Ide*		m_pParent;

	char				search[256];
};

class VT_Ide : public Fl_Window
{
public:
	VT_Ide(int x, int y, int w, int h, const char *title = 0);

// Methods
	virtual void	show();
	virtual int		handle(int event);

	void			NewProject(void);
	void			OpenProject(void);
	void			SaveProject(void);
	void			BuildTreeControl(void);
	int				ParsePrjFile(const char *name);
	void			RightClick(Flu_Tree_Browser::Node* n);
	void			NewFolder(Flu_Tree_Browser::Node* n);
	void			NewFile(void);
	void			SaveFile(void);
	void			SaveAs(void);
	void			OpenFile(void);
	void			Copy(void);
	void			Cut(void);
	void			Paste(void);
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
	void			ShowProjectSettings(void);
	MString			MakePathRelative(const MString& path, const MString& relTo);
	MString			MakePathAbsolute(const MString& path, const MString& relTo);
	MString			ProjectName(void);
	int				ProjectDirty(void);

	Fl_Window*		m_EditWindow;
	VT_ReplaceDlg*	m_pReplaceDlg;
	VT_FindDlg*		m_pFindDlg;

protected:
	virtual void	draw();
	void			AddGroupToTree(VTObject *pObj, const char *fmt);
	void			NewEditWindow(const MString& title, const MString& file);

	Fl_Window*					m_ProjWindow;
	Flu_Tree_Browser*			m_ProjTree;
	Fl_Window*					m_TabWindow;
	Fl_Tabs*					m_Tabs;
	Fl_Group*					m_BuildTab;
	Fl_Text_Display*			m_BuildTextDisp;
	Fl_Text_Buffer*				m_BuildTextBuf;
	Fl_Group*					m_DebugTab;
	Fl_Group*					m_WatchTab;
	Flu_Tree_Browser::Node*		m_Node;
	VT_Project*					m_ActivePrj;
	MString						m_LastDir;
	int							m_OpenLocation;
};

#endif

