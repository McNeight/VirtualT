/* fileview.h */

/* $Id: fileview.h,v 1.4 2008/09/23 00:06:13 kpettit1 Exp $ */

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


#ifndef FILEVIEW_H
#define FILEVIEW_H

void fileview_model_changed(void);

#ifdef	__cplusplus
void cb_FileView(Fl_Widget* w, void*);
#endif

#ifdef	FILE_VIEW_MAIN

typedef struct {
	char	name[12];
	unsigned short	address;
	unsigned short	dir_address;
	unsigned short	size;
	unsigned char	type;
} file_view_files_t;

typedef struct fileview_ctrl_struct
{
	Fl_Menu_Bar*			pMenu;				// Pointer to Menu bar

	Fl_Box*					pDirAddress;		// Pointer to Dir star taddress
	char					sDirAddr[20];
	Fl_Box*					pDirEntry;			// DirEntry information box
	char					sDirEntry[100];		// Text for directory entry
	Fl_Box*					pAddress;			// Address information box
	char					sAddress[100];		// Text for Address entry
	Fl_Hold_Browser*		pFileSelect;		// Pointer to Directory Table files
	Fl_Hold_Browser*		pView;				// Pointer to Browser for file contents
//	Fl_Text_Display*		pView;				// Pointer to the text display
//	Fl_Text_Buffer*			pTb;				// Pointer to a text buffer for display
	Fl_Usage_Box*			pRAM;				// Pointer to Usage bar for RAM
	file_view_files_t		tFiles[24];			// Storage for up to 24 files
	int						tCount;

	int						selectIndex;		// Index of active selection
	Fl_Group*			g;

} fileview_ctrl_t;

#define	FV_SELECT_FILE_COLOR	1
#define	FV_BASIC_FILE_COLOR		2
#define	FV_TEXT_FILE_COLOR		3
#define	FV_CO_FILE_COLOR		4
#define	FV_BASIC_VARS_COLOR		5
#define	FV_DIR_COLOR			6
#define	FV_SYS_VARS_COLOR		7
#define	FV_LCD_BUF_COLOR		8

#endif

#endif

