/* cpuregs.h */

/* $Id: cpuregs.h,v 1.5 2013/02/05 01:20:59 kpettit1 Exp $ */

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
#include <FL/Fl_Scrollbar.H>
#include <FL/Fl_Double_Window.H>

#include "MString.h"

void cb_CpuRegs(Fl_Widget* w, void*);

/*
================================================
Structure to keep track of CPU instruction trace
================================================
*/
typedef struct cpu_trace_data
{
	unsigned short		pc;
	unsigned short		sp;
	unsigned short		hl;
	unsigned short		de;
	unsigned short		bc;
	unsigned short		af;
	unsigned short		opcode;
	unsigned short		operand;
} cpu_trace_data_t;

#define	CPUREGS_TRACE_COUNT		1024
#define	CPUREGS_TRACE_SCROLL_W	15

/*
================================================
The CPU Registers Window
================================================
*/
//class VTCpuRegs : public Fl_Double_Window
class VTCpuRegs : public Fl_Window
{
public:
	VTCpuRegs(int x, int y, const char* title);
	~VTCpuRegs();

	void				RegAllDec(void);
	void				RegAllHex(void);

	void				activate_controls(void);
	void				deactivate_controls(void);
	void				get_reg_edits(void);
	void				check_breakpoint_entries(void);
	void				LoadPrefs(void);
	void				SavePrefs(void);
	void				ScrollToBottom(void);

	virtual void		draw(void);
	virtual int			handle(int event);

	Fl_Menu_Bar*		m_pMenu;
	Fl_Check_Button*	m_pEnable;

	Fl_Input*			m_pRegA;
	Fl_Input*			m_pRegF;
	Fl_Input*			m_pRegB;
	Fl_Input*			m_pRegC;
	Fl_Input*			m_pRegD;
	Fl_Input*			m_pRegE;
	Fl_Input*			m_pRegH;
	Fl_Input*			m_pRegL;
	Fl_Input*			m_pRegBC;
	Fl_Input*			m_pRegDE;
	Fl_Input*			m_pRegHL;
	Fl_Input*			m_pRegPC;
	Fl_Input*			m_pRegSP;
	Fl_Input*			m_pRegM;

	Fl_Input*			m_pBreak1;
	Fl_Input*			m_pBreak2;
	Fl_Input*			m_pBreak3;
	Fl_Input*			m_pBreak4;
	Fl_Check_Button*	m_pBreakDisable1;
	Fl_Check_Button*	m_pBreakDisable2;
	Fl_Check_Button*	m_pBreakDisable3;
	Fl_Check_Button*	m_pBreakDisable4;

	Fl_Check_Button*	m_pDebugInts;

	Fl_Check_Button*	m_pSFlag;
	Fl_Check_Button*	m_pZFlag;
	Fl_Check_Button*	m_pTSFlag;
	Fl_Check_Button*	m_pACFlag;
	Fl_Check_Button*	m_pPFlag;
	Fl_Check_Button*	m_pOVFlag;
	Fl_Check_Button*	m_pXFlag;
	Fl_Check_Button*	m_pCFlag;

	Fl_Round_Button*	m_pAllHex;
	Fl_Round_Button*	m_pAllDec;
	Fl_Round_Button*	m_pAHex;
	Fl_Round_Button*	m_pADec;
	Fl_Round_Button*	m_pBHex;
	Fl_Round_Button*	m_pBDec;
	Fl_Round_Button*	m_pCHex;
	Fl_Round_Button*	m_pCDec;
	Fl_Round_Button*	m_pDHex;
	Fl_Round_Button*	m_pDDec;
	Fl_Round_Button*	m_pEHex;
	Fl_Round_Button*	m_pEDec;
	Fl_Round_Button*	m_pHHex;
	Fl_Round_Button*	m_pHDec;
	Fl_Round_Button*	m_pLHex;
	Fl_Round_Button*	m_pLDec;
	Fl_Round_Button*	m_pPCHex;
	Fl_Round_Button*	m_pPCDec;
	Fl_Round_Button*	m_pSPHex;
	Fl_Round_Button*	m_pSPDec;
	Fl_Round_Button*	m_pBCHex;
	Fl_Round_Button*	m_pBCDec;
	Fl_Round_Button*	m_pDEHex;
	Fl_Round_Button*	m_pDEDec;
	Fl_Round_Button*	m_pHLHex;
	Fl_Round_Button*	m_pHLDec;
	Fl_Round_Button*	m_pMHex;
	Fl_Round_Button*	m_pMDec;
	Fl_Round_Button*	m_pBreakHex;
	Fl_Round_Button*	m_pBreakDec;

	Fl_Box*				m_pTraceBox;
	Fl_Box*				m_pStackBox;
	char				m_sInstTrace[64][120];
	int					m_iInstTraceHead;
	Fl_Check_Button*	m_pDisableTrace;

	cpu_trace_data_t*	m_pTraceData;
	int					m_iTraceHead;
	int					m_traceCount;
	int					m_traceAvail;

	int					m_breakAddr[5];
	int					m_breakEnable[5];
	int					m_breakMonitorFreq;

	Fl_Button*			m_pStop;
	Fl_Button*			m_pStep;
	Fl_Button*			m_pStepOver;
	Fl_Button*			m_pRun;

	char				m_sPCfmt[8];
	char				m_sSPfmt[8];
	char				m_sAfmt[8];
	char				m_sBfmt[8];
	char				m_sCfmt[8];
	char				m_sDfmt[8];
	char				m_sEfmt[8];
	char				m_sHfmt[8];
	char				m_sLfmt[8];
	char				m_sBCfmt[8];
	char				m_sDEfmt[8];
	char				m_sHLfmt[8];
	char				m_sMfmt[8];
	char				m_sBreakfmt[8];

	MString				m_sBreak1;
	MString				m_sBreak2;
	MString				m_sBreak3;
	MString				m_sBreak4;
	int					m_iBreakDis1;
	int					m_iBreakDis2;
	int					m_iBreakDis3;
	int					m_iBreakDis4;

	Fl_Scrollbar*		m_pScroll;					// Trace buffer scroll bar
	Fl_Group*			m_g;

	Fl_Check_Button*	m_pAutoScroll;
	Fl_Check_Button*	m_pSaveBreak;

	// Preferences
	int					m_autoScroll;
	int					m_saveBreakpoints;
	int					m_x, m_y, m_h, m_w;
	int					m_lastH;
	int					m_fontSize, m_fontHeight;
};

#endif

