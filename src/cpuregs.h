/* cpuregs.h */

/* $Id: cpuregs.h,v 1.0 2004/08/05 06:46:12 kpettit1 Exp $ */

/*
* Copyright 2004 Ken Pettit
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


#ifndef CPUREGS_H
#define CPUREGS_H

#include <FL/Fl_Button.H>
#include <FL/Fl_Round_Button.H>
#include <FL/Fl_Check_Button.H>
#include <FL/Fl_Box.H>
#include <FL/Fl_Input.H>

void cb_CpuRegs(Fl_Widget* w, void*);

typedef struct cpuregs_ctrl_struct
{
	Fl_Menu_Bar*			pMenu;

	Fl_Check_Button*	pEnable;

	Fl_Input*			pRegA;
	Fl_Input*			pRegF;
	Fl_Input*			pRegB;
	Fl_Input*			pRegC;
	Fl_Input*			pRegD;
	Fl_Input*			pRegE;
	Fl_Input*			pRegH;
	Fl_Input*			pRegL;
	Fl_Input*			pRegBC;
	Fl_Input*			pRegDE;
	Fl_Input*			pRegHL;
	Fl_Input*			pRegPC;
	Fl_Input*			pRegSP;
	Fl_Input*			pRegM;

	Fl_Input*			pBreak1;
	Fl_Input*			pBreak2;
	Fl_Input*			pBreak3;
	Fl_Input*			pBreak4;
	Fl_Check_Button*	pBreakDisable1;
	Fl_Check_Button*	pBreakDisable2;
	Fl_Check_Button*	pBreakDisable3;
	Fl_Check_Button*	pBreakDisable4;

	Fl_Check_Button*	pSFlag;
	Fl_Check_Button*	pZFlag;
	Fl_Check_Button*	pTSFlag;
	Fl_Check_Button*	pACFlag;
	Fl_Check_Button*	pPFlag;
	Fl_Check_Button*	pOVFlag;
	Fl_Check_Button*	pXFlag;
	Fl_Check_Button*	pCFlag;

	Fl_Round_Button*	pAllHex;
	Fl_Round_Button*	pAllDec;
	Fl_Round_Button*	pAHex;
	Fl_Round_Button*	pADec;
	Fl_Round_Button*	pBHex;
	Fl_Round_Button*	pBDec;
	Fl_Round_Button*	pCHex;
	Fl_Round_Button*	pCDec;
	Fl_Round_Button*	pDHex;
	Fl_Round_Button*	pDDec;
	Fl_Round_Button*	pEHex;
	Fl_Round_Button*	pEDec;
	Fl_Round_Button*	pHHex;
	Fl_Round_Button*	pHDec;
	Fl_Round_Button*	pLHex;
	Fl_Round_Button*	pLDec;
	Fl_Round_Button*	pPCHex;
	Fl_Round_Button*	pPCDec;
	Fl_Round_Button*	pSPHex;
	Fl_Round_Button*	pSPDec;
	Fl_Round_Button*	pBCHex;
	Fl_Round_Button*	pBCDec;
	Fl_Round_Button*	pDEHex;
	Fl_Round_Button*	pDEDec;
	Fl_Round_Button*	pHLHex;
	Fl_Round_Button*	pHLDec;
	Fl_Round_Button*	pMHex;
	Fl_Round_Button*	pMDec;
	Fl_Round_Button*	pBreakHex;
	Fl_Round_Button*	pBreakDec;

	Fl_Box*				pInstTrace[8];
	char				sInstTrace[8][120];
	int					iInstTraceHead;
	Fl_Check_Button*	pDisableTrace;

	int					breakAddr[4];
	int					breakEnable[4];
	int					breakMonitorFreq;

	Fl_Button*			pStop;
	Fl_Button*			pStep;
	Fl_Button*			pRun;
	Fl_Button*			pRedraw;

	char				sPCfmt[8];
	char				sSPfmt[8];
	char				sAfmt[8];
	char				sBfmt[8];
	char				sCfmt[8];
	char				sDfmt[8];
	char				sEfmt[8];
	char				sHfmt[8];
	char				sLfmt[8];
	char				sBCfmt[8];
	char				sDEfmt[8];
	char				sHLfmt[8];
	char				sMfmt[8];
	char				sBreakfmt[8];

	Fl_Group*			g;

} cpuregs_ctrl_t;


#endif

