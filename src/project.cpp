/* project.cpp */

/* $Id: project.cpp,v 1.1 2008/01/26 14:42:51 kpettit1 Exp $ */

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
#include <FL/Fl_Double_Window.H>
#include <FL/Fl_Box.H>
#include <FL/Fl_File_Chooser.H>
#include <stdio.h>
#include <string.h>
#include <ctype.h>

#include "VirtualT.h"
#include "project.h"

#ifdef _WIN32
extern "C"
#endif
extern char path[];

void cb_SelLocation(Fl_Widget* w, void*)
{
	Fl_File_Chooser		*fileWin;
	int					count;
	Fl_Round_Button*	pButton;
	VT_NewProject*		pNewProj;
	const char *		pNewLoc;
	char				newPath[512];

	// Get current location from the window
	pButton = (Fl_Round_Button*) w;
	pNewProj = (VT_NewProject*) pButton->user_data();

	// Create a File chooser
	fileWin = new Fl_File_Chooser(pNewProj->getLocation(), "", 
		Fl_File_Chooser::DIRECTORY, "Choose Project Location");
	fileWin->preview(0);
	fileWin->show();

	// Wait until selection is made or cancel
	while (fileWin->visible())
		Fl::wait();

	// Determine if selection made
	count = fileWin->count();
	if (count == 0)
	{
		delete fileWin;
		return;
	}

	// Get the path of the new location
	pNewLoc = fileWin->value(1);

	printf("Path = %s\n", path);
	printf("New  = %s\n", pNewLoc);
	// Check if path is in the current directory and replace with "./"
	if (strncmp(path, pNewLoc, strlen(path)) == 0)
	{
		strcpy(newPath, ".");
		strcat(newPath, &pNewLoc[strlen(path)]);
		if (newPath[1] == '\0')
			strcat(newPath, "/");
	}
	else
	{
		strcpy(newPath, pNewLoc);
	}
	pNewProj->setLocation(newPath);

	delete fileWin;
}

void cb_cancelNewProject(Fl_Widget* w, void*)
{
	VT_NewProject* pNewProject;

	pNewProject = (VT_NewProject*) w->user_data();
	pNewProject->hide();
}

void cb_okNewProject(Fl_Widget* w, void*)
{
	const char *	projName;
	VT_NewProject*	pNewProject;
	char			projPath[512];
	FILE*			fd;

	pNewProject = (VT_NewProject*) w->user_data();
	if (pNewProject == NULL)
		return;

	// Check if project name provided
	projName = pNewProject->getProjName();
	if (projName == NULL)
	{
		fl_alert("Must provide project name!");
		return;
	}
	if (strlen(projName) == 0)
	{
		fl_alert("Project name must be provided!");
		return;
	}
	
	// Check if project type provided
	if (pNewProject->getProjType() == 0)
	{
		fl_alert("Project type must be specified!");
		return;
	}

	// Check if project already exists
	sprintf(projPath, "%s/%s/%s.prj", pNewProject->getLocation(), projName, projName);
	fd = fopen(projPath, "r+");
	if (fd != NULL)
	{
		fclose(fd);
		sprintf(projPath, "Project %s already exists!", projName);
		fl_alert(projPath);
		return;
	}

	// Mark project "okay" to create
	pNewProject->hide();
	pNewProject->m_makeProj = TRUE;
}

void browse_cb(Fl_Widget* w, void*)
{
	int				sel;
	int				c;
	const char 		*item;
	char			newtxt[30];

	Fl_Browser*	pB = (Fl_Browser*) w;
	sel = pB->value();

	// Clear bold from all enries
	for (c = 1; ;c++)
	{
		item = pB->text(c);
		if (item == NULL)
			break;
		if (item[0] == '@')
			pB->text(c, &item[2]);
	}
	
	// Check if selection is zero (no selection)
	if (sel == 0)
		return;

	// Make selection bold
	strcpy(&newtxt[2], pB->text(sel));
	newtxt[0] = '@';
	newtxt[1] = 'b';
	pB->text(sel, newtxt);
}

VT_NewProject::VT_NewProject()
{
	Fl_Box*				o;
	Fl_Button*			b;

	// Set flag indicating project parameters not valid yet.
	m_makeProj = FALSE;

	// Create new window
	m_pWnd = new Fl_Double_Window(100, 150, 380,320, "Create New Project");

	// Text for project name
	o = new Fl_Box(10, 10, 100, 20, "Project Name");
	o->align(FL_ALIGN_LEFT | FL_ALIGN_INSIDE);

	// Create Project Name edit field
	m_pProjName = new Fl_Input(14,30,150,25,"");

	// Text for the project type
	o = new Fl_Box(10, 60, 100, 20, "Project Type");
	o->align(FL_ALIGN_LEFT | FL_ALIGN_INSIDE);

	// Create a browser
	m_pProjType = new Fl_Select_Browser(14, 80, 150, 100);
	m_pProjType->add("@bAssembly .CO");
	m_pProjType->add("Assembly .obj");
	m_pProjType->add("Assembly ROM");
	m_pProjType->add("BASIC");
	m_pProjType->callback(browse_cb);
	m_pProjType->value(1);

	// Creae location edit field & browse button
	o = new Fl_Box(10, 190, 150, 20, "Location");
	o->align(FL_ALIGN_LEFT | FL_ALIGN_INSIDE);
	m_pLocation = new Fl_File_Input(14,210, 300,35,"");
	m_pLocation->value("./Projects/");
	b = new Fl_Button(324, 210, 35, 35, "...");
	b->callback(cb_SelLocation);
	b->user_data(this);

	// Text for target device
	o = new Fl_Box(180, 10, 100, 20, "Target Model");
	o->align(FL_ALIGN_LEFT | FL_ALIGN_INSIDE);

	// Create radio buttons for each model
	m_pM100 = new Fl_Round_Button(200, 35, 100, 20, "Model 100");
	m_pM100->type(FL_RADIO_BUTTON);
	m_pM100->value(1);
	m_pM102 = new Fl_Round_Button(200, 60, 100, 20, "Model 102");
	m_pM102->type(FL_RADIO_BUTTON);
	m_pT200 = new Fl_Round_Button(200, 85, 100, 20, "Model 200");
	m_pT200->type(FL_RADIO_BUTTON);
	m_pPC8201 = new Fl_Round_Button(200, 110, 100, 20, "Model PC-8201");
	m_pPC8201->type(FL_RADIO_BUTTON);
    //JV
	m_pKC85 = new Fl_Round_Button(200, 125, 100, 20, "Model KC85");
	m_pKC85->type(FL_RADIO_BUTTON);
	// Create an Ok button
	m_pOk = new Fl_Button(60, 265, 90, 30, "OK");
	m_pOk->callback(cb_okNewProject);
	m_pOk->user_data(this);

	// Create a Cancel button
	m_pCancel = new Fl_Button(200, 265, 90, 30, "Cancel");
	m_pCancel->callback(cb_cancelNewProject);
	m_pCancel->user_data(this);

	m_pWnd->end();
}

VT_NewProject::~VT_NewProject()
{
	if (m_pWnd != NULL)
		delete m_pWnd;
}

void VT_NewProject::show(void)
{
	if (m_pWnd != NULL)
		m_pWnd->show();
}

void VT_NewProject::hide(void)
{
	if (m_pWnd != NULL)
		m_pWnd->hide();
}

const char * VT_NewProject::getProjName(void)
{
	if (m_pProjName == NULL)
		return NULL;

	return m_pProjName->value();
}

int VT_NewProject::getProjType(void)
{
	if (m_pProjType == NULL)
		return 0;

	return m_pProjType->value();
}

int VT_NewProject::visible(void)
{
	return m_pWnd->visible();
}

const char * VT_NewProject::getLocation(void)
{
	if (m_pLocation == NULL)
		return NULL;

	return m_pLocation->value();
}

void VT_NewProject::setLocation(char *pPath)
{
	if (m_pLocation == NULL)
		return;

	m_pLocation->value(pPath);
}

int VT_NewProject::getTargetModel()
{
	if (m_pM100->value() == 1)
		return MODEL_M100;
	else if (m_pM102->value() == 1)
		return MODEL_M102;
	else if (m_pT200->value() == 1)
		return MODEL_T200;
	else if (m_pPC8201->value() == 1)
		return MODEL_PC8201;
	else if (m_pKC85->value() == 1)
		return MODEL_KC85;

	return -1;
}

/*
==============================================================================
ProjectSettings dialog box functions defined below.  This dialog is uses to
set the assembler, linker, and other settings for the active project.
==============================================================================
*/
void cb_settings_cancel(Fl_Widget* w, void*)
{
	VT_ProjectSettings* pProj;

	pProj = (VT_ProjectSettings*) w->user_data();
	pProj->hide();
}

void cb_settings_ok(Fl_Widget* w, void*)
{
	VT_ProjectSettings*	pProj;
	VT_Project*			pProject;

	// Get a pointer to the projectSettings object
	pProj = (VT_ProjectSettings*) w->user_data();
	if (pProj == NULL)
		return;
	pProject = pProj->m_pProject;

	// Test the validity of the IncludePath

	// Test the validity of the Defines

	// Test the validity of the LinkPath

	// Test the validity of the LinkLibs

	// Update the General Options
	pProject->m_ProjectType = pProj->getProjType();
	pProject->m_TargetModel = pProj->getTargetModel();
	pProject->m_AutoLoad = pProj->getAutoLoad();
	pProject->m_UpdateHIMEM = pProj->getUpdateHIMEM();

	// Update the Assembler Options
	pProject->AsmDebugInfo(pProj->getAsmDebugInfo());
	pProject->AsmListing(pProj->getAsmListing());
	pProject->BrowseInfo(pProj->getBrowseInfo());
	pProject->AutoExtern(pProj->getAutoExtern());
	pProject->m_IncludePath = pProj->getIncludeDirs();
	pProject->m_Defines = pProj->getDefines();

	// Update the Linker Options
	pProject->LinkDebugInfo(pProj->getLinkDebugInfo());
	pProject->MapFile(pProj->getMapFile());
	pProject->IgnoreStdLibs(pProj->getIgnoreStdLibs());
	pProject->m_LinkPath = pProj->getLinkPath();
	pProject->m_LinkLibs = pProj->getLinkObjs();
	pProject->m_CodeAddr = pProj->getCodeAddr();
	pProject->m_DataAddr = pProj->getDataAddr();

	// Hide the window to end the session
	pProj->m_pProject->m_Dirty = 1;
	pProj->hide();
}

void cb_AutoLoad(Fl_Widget* w, void*)
{
	VT_ProjectSettings*		pProj;

	pProj = (VT_ProjectSettings*) w->user_data();
	if (pProj == NULL)
		return;

	pProj->EnableUpdateHIMEM(pProj->getAutoLoad());
}

VT_ProjectSettings::VT_ProjectSettings(VT_Project *pProj)
{
	Fl_Box*				o;
	Fl_Group*			g;

	// Set flag indicating project parameters not valid yet.
	m_makeProj = FALSE;
	m_pProject = pProj;

	// Create new window
	m_pWnd = new Fl_Double_Window(100, 150, 380,320, "Project Settings");

	m_pTabs = new Fl_Tabs(10, 10, 360, 260);
	
	// Create the General tab
	g = new Fl_Group(10, 30, 380, 280, "General");

	// Text for the project type
	o = new Fl_Box(20, 50, 100, 20, "Project Type");
	o->align(FL_ALIGN_LEFT | FL_ALIGN_INSIDE);

	// Create a browser
	m_pProjType = new Fl_Select_Browser(24, 75, 150, 100);
	m_pProjType->add("Assembly .CO");
	m_pProjType->add("Assembly .obj");
	m_pProjType->add("Assembly ROM");
	m_pProjType->add("BASIC");
	m_pProjType->callback(browse_cb);
	m_pProjType->value(m_pProject->m_ProjectType);

	// Text for target device
	o = new Fl_Box(180, 50, 100, 20, "Target Model");
	o->align(FL_ALIGN_LEFT | FL_ALIGN_INSIDE);

	// Create radio buttons for each model
	m_pM100 = new Fl_Round_Button(210, 75, 100, 20, "Model 100");
	m_pM100->type(FL_RADIO_BUTTON);
	if (m_pProject->m_TargetModel == MODEL_M100)
		m_pM100->value(1);
	m_pM102 = new Fl_Round_Button(210, 100, 100, 20, "Model 102");
	m_pM102->type(FL_RADIO_BUTTON);
	if (m_pProject->m_TargetModel == MODEL_M102)
		m_pM102->value(1);
	m_pT200 = new Fl_Round_Button(210, 125, 100, 20, "Model 200");
	m_pT200->type(FL_RADIO_BUTTON);
	if (m_pProject->m_TargetModel == MODEL_T200)
		m_pT200->value(1);
	m_pPC8201 = new Fl_Round_Button(210, 150, 100, 20, "Model PC-8201");
	m_pPC8201->type(FL_RADIO_BUTTON);
	if (m_pProject->m_TargetModel == MODEL_PC8201)
		m_pPC8201->value(1);
	m_pKC85 = new Fl_Round_Button(210, 175, 100, 20, "Model KC85");
	m_pKC85->type(FL_RADIO_BUTTON);
	if (m_pProject->m_TargetModel == MODEL_KC85)
		m_pKC85->value(1);


	// Create checkbox for AutoLoad
	m_pAutoLoad  = new Fl_Check_Button(24, 200, 250, 20, "Load to Emulation after assembly");
	m_pAutoLoad->value(m_pProject->m_AutoLoad);
	m_pAutoLoad->callback(cb_AutoLoad);
	m_pAutoLoad->user_data(this);

	m_pUpdateHIMEM = new Fl_Check_Button(44, 230, 250, 20, "Update HIMEM automatically");
	m_pUpdateHIMEM->value(m_pProject->m_UpdateHIMEM);
	if (m_pProject->m_AutoLoad == 0)
		EnableUpdateHIMEM(FALSE);

	g->end();

	// Create an Assembler tab
	g = new Fl_Group(10, 30, 380, 280, "Assembler");

	// Create checkboxes
	m_pAsmDebugInfo = new Fl_Check_Button(20, 50, 165, 20, "Generate Debug Info");
	m_pAsmDebugInfo->value(m_pProject->AsmDebugInfo());
	m_pList = new Fl_Check_Button(20, 75, 170, 20, "Generate List Files");
	m_pList->value(m_pProject->AsmListing());
	m_pBrowseInfo = new Fl_Check_Button(190, 50, 165, 20, "Generate Browse Info");
	m_pBrowseInfo->value(m_pProject->BrowseInfo());
	m_pAutoExtern = new Fl_Check_Button(190, 75, 165, 20, "Automatic Externs");
	m_pAutoExtern->value(m_pProject->AutoExtern());

	// Create edit field for Include Directories
	o = new Fl_Box(20, 110, 100, 20, "Additional Include Directories");
	o->align(FL_ALIGN_LEFT | FL_ALIGN_INSIDE);
	m_pIncludeDirs = new Fl_Input(20, 130, 320, 20, "");
	m_pIncludeDirs->value(m_pProject->m_IncludePath);

	// Create edit field for Additional Defines
	o = new Fl_Box(20, 170, 100, 20, "Additional Assembler Defines");
	o->align(FL_ALIGN_LEFT | FL_ALIGN_INSIDE);
	m_pDefines = new Fl_Input(20, 190, 320, 20, "");
	m_pDefines->value(m_pProject->m_Defines);

	// End the Assembler tab
	g->end();

	// Create a Linker tab
	g = new Fl_Group(10, 30, 380, 280, "Linker");

	// Create checkboxes
	m_pLinkDebugInfo = new Fl_Check_Button(20, 50, 165, 20, "Generate Debug Info");
	m_pLinkDebugInfo->value(m_pProject->LinkDebugInfo());
	m_pMapFile = new Fl_Check_Button(20, 75, 170, 20, "Generate Map File");
	m_pMapFile->value(m_pProject->MapFile());
	m_pIgnoreStdLibs = new Fl_Check_Button(20, 100, 165, 20, "Ingore Std Libraries");
	m_pIgnoreStdLibs->value(m_pProject->IgnoreStdLibs());

	// Create Edit field for Output Name
	o = new Fl_Box(20, 125, 100, 20, "Output Name");
	o->align(FL_ALIGN_LEFT | FL_ALIGN_INSIDE);
	m_pOutputName = new Fl_Input(20, 145, 320, 20, "");
	m_pOutputName->value(m_pProject->m_Name);

	// Create Edit field for Link libs
	o = new Fl_Box(20, 170, 100, 20, "Additional Link Objects");
	o->align(FL_ALIGN_LEFT | FL_ALIGN_INSIDE);
	m_pLinkObjs = new Fl_Input(20, 190, 320, 20, "");
	m_pLinkObjs->value(m_pProject->m_LinkLibs);

	// Create Edit field for Link Dirs
	o = new Fl_Box(20, 215, 100, 20, "Additional Object Directories");
	o->align(FL_ALIGN_LEFT | FL_ALIGN_INSIDE);
	m_pObjPath = new Fl_Input(20, 235, 320, 20, "");
	m_pObjPath->value(m_pProject->m_LinkPath);

	// Add edit fields for Code and Data segment addresses
	o = new Fl_Box(235, 50, 100, 20, "Segment Address");
	o->align(FL_ALIGN_LEFT | FL_ALIGN_INSIDE);
	o = new Fl_Box(200, 75, 40, 20, "Code");
	o->align(FL_ALIGN_LEFT | FL_ALIGN_INSIDE);
	o = new Fl_Box(200, 100, 40, 20, "Data");
	o->align(FL_ALIGN_LEFT | FL_ALIGN_INSIDE);
	
	m_pCodeAddr = new Fl_Input(250, 75, 90, 20, "");
	m_pCodeAddr->value("test");
	m_pCodeAddr->value((const char *) m_pProject->m_CodeAddr);
	m_pDataAddr = new Fl_Input(250, 100, 90, 20, "");
	m_pDataAddr->value((const char *) m_pProject->m_DataAddr);

	// End the Linker tab
	g->end();

	m_pTabs->end();	

    // OK button
	{ 
		Fl_Button* o = new Fl_Button(200, 282, 60, 30, "Cancel");
		o->callback((Fl_Callback*)cb_settings_cancel);
		o->user_data(this);
	}
    { 
		Fl_Return_Button* o = new Fl_Return_Button(280, 282, 60, 30, "OK");
		o->callback((Fl_Callback*)cb_settings_ok);
		o->user_data(this);
	}
}

void VT_ProjectSettings::show(void)
{
	if (m_pWnd != NULL)
		m_pWnd->show();
}

int VT_ProjectSettings::visible(void)
{
	return m_pWnd->visible();
}

void VT_ProjectSettings::hide(void)
{
	if (m_pWnd != NULL)
		m_pWnd->hide();
}

int VT_ProjectSettings::getProjType(void)
{
	if (m_pProjType == NULL)
		return 0;

	return m_pProjType->value();
}

int VT_ProjectSettings::getTargetModel()
{
	if (m_pM100->value() == 1)
		return MODEL_M100;
	else if (m_pM102->value() == 1)
		return MODEL_M102;
	else if (m_pT200->value() == 1)
		return MODEL_T200;
	else if (m_pPC8201->value() == 1)
		return MODEL_PC8201;
	else if (m_pKC85->value() == 1)
		return MODEL_KC85;

	return -1;
}

int VT_ProjectSettings::getAutoLoad()
{
	if (m_pAutoLoad == NULL)
		return 0;

	return m_pAutoLoad->value();
}

int VT_ProjectSettings::getUpdateHIMEM()
{
	if (m_pUpdateHIMEM == NULL)
		return 0;

	return m_pUpdateHIMEM->value();
}

MString VT_ProjectSettings::getDefines(void)
{
	if (m_pDefines == NULL)
		return "";

	return m_pDefines->value();
}

MString VT_ProjectSettings::getIncludeDirs(void)
{
	if (m_pIncludeDirs == NULL)
		return "";

	return m_pIncludeDirs->value();
}

int VT_ProjectSettings::getAsmDebugInfo(void)
{
	if (m_pAsmDebugInfo == NULL)
		return 0;

	return m_pAsmDebugInfo->value();
}

int VT_ProjectSettings::getAsmListing(void)
{
	if (m_pList == NULL)
		return 0;

	return m_pList->value();
}

int VT_ProjectSettings::getBrowseInfo(void)
{
	if (m_pBrowseInfo == NULL)
		return 0;

	return m_pBrowseInfo->value();
}

int VT_ProjectSettings::getAutoExtern(void)
{
	if (m_pAutoExtern == NULL)
		return 0;

	return m_pAutoExtern->value();
}

int VT_ProjectSettings::getLinkDebugInfo(void)
{
	if (m_pLinkDebugInfo == NULL)
		return 0;

	return m_pLinkDebugInfo->value();
}

int VT_ProjectSettings::getMapFile(void)
{
	if (m_pMapFile == NULL)
		return 0;

	return m_pMapFile->value();
}

int VT_ProjectSettings::getIgnoreStdLibs(void)
{
	if (m_pIgnoreStdLibs == NULL)
		return 0;

	return m_pIgnoreStdLibs->value();
}

MString VT_ProjectSettings::getLinkPath(void)
{
	if (m_pObjPath == NULL)
		return "";

	return m_pObjPath->value();
}

MString VT_ProjectSettings::getLinkObjs(void)
{
	if (m_pLinkObjs == NULL)
		return "";

	return m_pLinkObjs->value();
}

MString VT_ProjectSettings::getCodeAddr(void)
{
	if (m_pCodeAddr == NULL)
		return "";

	return m_pCodeAddr->value();
}

MString VT_ProjectSettings::getDataAddr(void)
{
	if (m_pDataAddr == NULL)
		return "";

	return m_pDataAddr->value();
}

void VT_ProjectSettings::EnableUpdateHIMEM(int enable)
{
	// Insure the pointer is not NULL
	if (m_pUpdateHIMEM == NULL)
		return;

	if (enable)
		m_pUpdateHIMEM->activate();
	else
		m_pUpdateHIMEM->deactivate();
}

/*
========================================================================
VT_Project implementation routines.
========================================================================
*/
void VT_Project::AsmDebugInfo(int enable)
{
	if (enable)
		AddAsmOption("-g");
	else
		RemoveAsmOption("-g");
}

int VT_Project::AsmDebugInfo(void) const
{
	return m_AsmOptions.Find("-g", 0) != -1;
}

void VT_Project::AsmListing(int enable)
{
	if (enable)
		AddAsmOption("-l");
	else
		RemoveAsmOption("-l");
}

int VT_Project::AsmListing(void) const
{
	return m_AsmOptions.Find("-l", 0) != -1;
}

void VT_Project::BrowseInfo(int enable)
{
	if (enable)
		AddAsmOption("-b");
	else
		RemoveAsmOption("-b");
}

int VT_Project::BrowseInfo(void) const
{
	return m_AsmOptions.Find("-b", 0) != -1;
}

void VT_Project::AutoExtern(int enable)
{
	if (enable)
		AddAsmOption("-e");
	else
		RemoveAsmOption("-e");
}

int VT_Project::AutoExtern(void) const
{
	return m_AsmOptions.Find("-e", 0) != -1;
}

void VT_Project::AddAsmOption(char* pOpt)
{
	int		index;

	// Determine if option already exists in m_AsmOptions
	index = m_AsmOptions.Find(pOpt, 0);
	if (index != -1)
		return;

	// Append option to end of the m_AsmOptions string
	m_AsmOptions = m_AsmOptions + " " + pOpt;
}

void VT_Project::RemoveAsmOption(char* pOpt)
{
	int		index;
	MString	temp;

	// Determin if option already exists in m_AsmOptions
	index = m_AsmOptions.Find(pOpt, 0);
	if (index == -1)
		return;

	// Need to remove option from the m_AsmOptons string
	temp = m_AsmOptions.Left(index);
	temp.Trim();
	if (m_AsmOptions.GetLength() > index + 2)
		temp += m_AsmOptions.Right(m_AsmOptions.GetLength() - (index + 2));

	// Assign new m_AsmOptions
	m_AsmOptions = temp;
}

void VT_Project::AddLinkOption(char* pOpt)
{
	int		index;

	// Determine if option already exists in m_AsmOptions
	index = m_LinkOptions.Find(pOpt, 0);
	if (index != -1)
		return;

	// Append option to end of the m_AsmOptions string
	m_LinkOptions = m_LinkOptions + " " + pOpt;
}

void VT_Project::RemoveLinkOption(char* pOpt)
{
	int		index;
	MString	temp;

	// Determin if option already exists in m_AsmOptions
	index = m_LinkOptions.Find(pOpt, 0);
	if (index == -1)
		return;

	// Need to remove option from the m_AsmOptons string
	temp = m_LinkOptions.Left(index);
	temp.Trim();
	if (m_LinkOptions.GetLength() > index + 2)
		temp += m_LinkOptions.Right(m_LinkOptions.GetLength() - (index + 2));

	// Assign new m_AsmOptions
	m_LinkOptions = temp;
}

void VT_Project::LinkDebugInfo(int enable)
{
	if (enable)
		AddLinkOption("-g");
	else
		RemoveLinkOption("-g");
}

int VT_Project::LinkDebugInfo(void) const
{
	return m_LinkOptions.Find("-g", 0) != -1;
}

void VT_Project::MapFile(int enable)
{
	if (enable)
		AddLinkOption("-m");
	else
		RemoveLinkOption("-m");
}

int VT_Project::MapFile(void) const
{
	return m_LinkOptions.Find("-m", 0) != -1;
}

void VT_Project::IgnoreStdLibs(int enable)
{
	if (enable)
		AddLinkOption("-i");
	else
		RemoveLinkOption("-i");
}

int VT_Project::IgnoreStdLibs(void) const
{
	return m_LinkOptions.Find("-i", 0) != -1;
}

