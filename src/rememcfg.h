/* rememcfg.h */

/* $Id: rememcfg.h,v 1.0 2008/01/02 06:46:12 kpettit1 Exp $ */

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


#ifndef REMEMCFG_H
#define REMEMCFG_H

#include <FL/Fl_Button.H>
#include <FL/Fl_Round_Button.H>
#include <FL/Fl_Check_Button.H>
#include <FL/Fl_Box.H>
#include <FL/Fl_Input.H>
#include <FL/Fl_Value_Slider.H>
#include <FL/Fl_Hold_Browser.H>
#include "fl_usage_box.h"

#define	REMCFG_MAPPING_COLOR	1
#define	REMCFG_SECTOR_COLOR		2
#define	REMCFG_COMBO_COLOR		3
#define	REMCFG_RAMPAC_COLOR		4
#define	REMCFG_MAPLOC_COLOR		5
#define	REMCFG_MAPSEL_COLOR		6

void cb_RememCfg(Fl_Widget* w, void*);

typedef struct rememcfg_ctrl_struct
{
	Fl_Menu_Bar*			pMenu;

	Fl_Box*				pFWVersion;
	Fl_Box*				pStatus;
	Fl_Value_Slider*	pStatusMap;
	Fl_Check_Button*	pIOEnable;
	Fl_Check_Button*	pRampacEnable;
	Fl_Check_Button*	pRememEnable;
	Fl_Button*			pReadHW;
	Fl_Button*			pWriteHW;

	Fl_Hold_Browser*	pMapSelect;
	Fl_Hold_Browser*	pRegionSelect;
	Fl_Hold_Browser*	pVectorSelect;
	Fl_Box*				pVectorAddress;

	Fl_Input*			pMapData;
	Fl_Check_Button*	pRamCS;
	Fl_Check_Button*	pFlash1CS;
	Fl_Check_Button*	pFlash2CS;
	Fl_Check_Button*	pSectorLock;

	Fl_Usage_Box*		pRAM;
	Fl_Usage_Box*		pFlash1;
	Fl_Usage_Box*		pFlash2;

	char				sFW[10];
	char				sStatus[10];
	char				sVectorAddress[10];
	char				sMap[8][120];

	Fl_Button*			pSaveMap;
	Fl_Button*			pLoadMap;
	Fl_Button*			pDefaultMap;
	Fl_Button*			pCopySequential;
	Fl_Check_Button*	pShowUsage;

	Fl_Group*			g;

} rememcfg_ctrl_t;


#endif

