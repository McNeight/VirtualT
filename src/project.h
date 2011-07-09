/* project.h */

/* $Id: project.h,v 1.2 2008/03/09 16:33:56 kpettit1 Exp $ */

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


#ifndef PROJECT_H
#define PROJECT_H

#include <FL/Fl.H>
#include <FL/Fl_Double_Window.H>
#include <FL/Fl_Button.H>
#include <FL/Fl_Round_Button.H>
#include <FL/Fl_Check_Button.H>
#include <FL/Fl_Input.H>
#include <FL/Fl_File_Input.H>
#include <FL/Fl_Select_Browser.H>
#include <FL/Fl_Tabs.H>

#include "MString.h"
#include "vtobj.h"

void cb_NewProject (Fl_Widget* w, void*);

#define		VT_PROJ_TYPE_CO			0
#define		VT_PROJ_TYPE_OBJ		1
#define		VT_PROJ_TYPE_ROM		2
#define		VT_PROJ_TYPE_BA			3

class VT_Project
{
public:
    VT_Project()     { m_Dirty = 0; m_AutoLoad = 0; m_ProjectType = 0; 
					m_TargetModel = 0; m_UpdateHIMEM = 0; m_LinkScript = ""; }
    ~VT_Project();

	void			AsmDebugInfo(int enable);
	int				AsmDebugInfo(void) const;
	void			AsmListing(int enable);
	int				AsmListing(void) const;
	void			BrowseInfo(int enable);
	int				BrowseInfo(void) const;
	void			AutoExtern(int enable);
	int				AutoExtern(void) const;

	void			AddAsmOption(char *pOpt);
	void			RemoveAsmOption(char *pOpt);

	void			LinkDebugInfo(int enable);
	int				LinkDebugInfo(void) const;
	void			MapFile(int enable);
	int				MapFile(void) const;
	void			IgnoreStdLibs(int enable);
	int				IgnoreStdLibs(void) const;

	void			AddLinkOption(char *pOpt);
	void			RemoveLinkOption(char *pOpt);

	void			SaveProject(void);
	void			LoadProject(void);
	void			WriteGroupToFile(class VT_IdeGroup* pGroup, FILE* fd);
	MString			MakePathRelative(const MString& path, const MString& relTo);
	int				TestIfFileInProject(MString& filename);
	int				TestIfFileInGroup(VT_IdeGroup* pGroup, MString& filename);

    MString         m_Name;				// Project name
    MString         m_RootPath;
    MString         m_IncludePath;
	MString			m_Defines;
    MString         m_LinkPath;
	MString			m_LinkLibs;
    MString         m_AsmOptions;
    MString         m_LinkOptions;
	MString			m_LinkScript;
    VTObArray       m_Groups;
    int             m_Dirty;			// Set true when project settings change
    int             m_ProjectType;		// Type of project
    int             m_TargetModel;		// Target model to assemble for
	int				m_AutoLoad;			// Load to emulator after assemble?
	int				m_UpdateHIMEM;		// Auto update HIMEM variable?
};

class VT_NewProject 
{
public:
	VT_NewProject();
	~VT_NewProject();

	MString				m_Dir;
	void				show();
	void				hide();
	int					visible();
	const char * 		getLocation();
	void		 		setLocation(char *pPath);
	const char *		getProjName();
	int					getProjType();
	int					getTargetModel();
	int					m_makeProj;

protected:
	Fl_Double_Window*	m_pWnd;
	Fl_Select_Browser*	m_pProjType;
	Fl_Button*			m_pOk;
	Fl_Button*			m_pCancel;
	Fl_Button*			m_pBrowse;
	Fl_Input*			m_pProjName;
	Fl_File_Input*		m_pLocation;
	Fl_Round_Button*	m_pM100;
	Fl_Round_Button*	m_pM102;
	Fl_Round_Button*	m_pT200;
	Fl_Round_Button*	m_pPC8201;
	Fl_Round_Button*	m_pKC85;
};

class VT_ProjectSettings
{
public:
	VT_ProjectSettings(VT_Project* pProj);
	~VT_ProjectSettings();

	MString				m_Dir;
	int					m_makeProj;
	class	VT_Project*	m_pProject;

	void				show();
	void				hide();
	int					visible();

	int					getProjType();
	int					getTargetModel();
	int					getAutoLoad();
	int					getUpdateHIMEM();

	MString				getDefines(void);
	MString				getIncludeDirs(void);
	int					getAsmListing(void);
	int					getAsmDebugInfo(void);
	int					getBrowseInfo(void);
	int					getAutoExtern(void);

	int					getLinkDebugInfo(void);
	int					getMapFile(void);
	int					getIgnoreStdLibs(void);
	MString				getLinkPath(void);
	MString				getOutputName(void);
	MString				getLinkObjs(void);
	MString				getLinkScript(void);

	void				EnableUpdateHIMEM(int enable);

protected:
	Fl_Double_Window*	m_pWnd;

	// Define a tabs object
	Fl_Tabs*			m_pTabs;

	// Define General tab items
	Fl_Select_Browser*	m_pProjType;
	Fl_Round_Button*	m_pM100;
	Fl_Round_Button*	m_pM102;
	Fl_Round_Button*	m_pT200;
	Fl_Round_Button*	m_pPC8201;
	Fl_Round_Button*	m_pKC85;
	Fl_Check_Button*	m_pAutoLoad;
	Fl_Check_Button*	m_pUpdateHIMEM;

	// Define Assembler tab items
	Fl_Button*			m_pAsmDebugInfo;
	Fl_Button*			m_pList;
	Fl_Button*			m_pBrowseInfo;
	Fl_Button*			m_pAutoExtern;
	Fl_Input*			m_pIncludeDirs;
	Fl_Input*			m_pDefines;

	// Define Link tab items
	Fl_Button*			m_pIgnoreStdLibs;
	Fl_Button*			m_pMapFile;
	Fl_Button*			m_pLinkDebugInfo;
	Fl_Input*			m_pObjPath;
	Fl_Input*			m_pOutputName;
	Fl_Input*			m_pLinkObjs;
	Fl_Input*			m_pLinkScript;

	Fl_Button*			m_pOk;
	Fl_Button*			m_pCancel;
};


#endif

