/* disassemble.h */

/* $Id: disassemble.h,v 1.8 2013/02/05 01:20:59 kpettit1 Exp $ */

/*
 * Copyright 2004 Stephen Hurd and Ken Pettit
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


#ifndef _DISASSEMBLE_H_
#define _DISASSEMBLE_H_

#include <FL/Fl_Text_Buffer.H>
#include <FL/Fl_Double_Window.H>
#include <FL/Fl_Check_Button.H>
#include <FL/Fl_Choice.H>
#include "FLU/Flu_Combo_List.h"
#include "FLU/Flu_Button.h"

#include "display.h"
#include "MString.h"

void disassembler_cb(Fl_Widget* w, void*);

// Structure to keep track of which addresses need labels printed
typedef struct DisType 
{
	int			dis_type;		// Disassembly type for this address
	const char	*pLabel;		// Pointer to the description
	int			idx;			// Index into Disassembly String table 
} DisType_t;

#define		DIS_TYPE_UNKNOWN	0x00
#define		DIS_TYPE_LABEL		0x01
#define		DIS_TYPE_STRING		0x02
#define		DIS_TYPE_JUMP		0x04
#define		DIS_TYPE_CODE		0x08
#define		DIS_TYPE_TABLE		0x10

class VTDis;

class VTDisFindDlg
{
public:
	VTDisFindDlg(class VTDis* pParent);

	Fl_Window*			m_pFindDlg;
	Flu_Combo_List*		m_pFind;
	Fl_Choice*			m_pFindIn;
	Fl_Check_Button*	m_pBackward;
	Fl_Check_Button*	m_pMatchCase;
	Fl_Check_Button*	m_pWholeWord;
	Flu_Button*			m_pNext;
	Flu_Button*			m_pCancel;

	class VTDis*		m_pParent;

	char				search[256];
};


class VTDis : public Fl_Double_Window
{
public:
	VTDis(int x, int y, const char* title);
	VTDis(void);
	~VTDis();

	int 					m_StartAddress;				// Starting address of the disassembly
	int 					m_EndAddress;				// Ending address of the disassembly
	unsigned char			m_memory[65536];			// Our local memory to disassemble from
	class My_Text_Editor* 	m_pTextViewer;
	RomDescription_t		*m_pRom;					// The ROM description table we are using
	int						m_WantComments;				// Indicates if comments should be appended
	int						m_x, m_y, m_w, m_h;			// User preference window location
	int						m_fontSize;					// Font size
	int						m_oldSchool;				// Set true to generate legacy style disassembly
	int						m_blackBackground;			// Set true for black background
	int						m_colorHilight;				// Set TRUE for color hilighting
	int						m_disassemblyDepth;			// Depth (in instructions) to simulate for disassembly
	int						m_opcodesLowercase;			// Creates disassembly with lower-case opcodes

	void 					Disassemble();
	void 					SetTextViewer(class My_Text_Editor* pTextViewer);
	void 					CopyIntoMem(unsigned char * ptr, int len, int startAddr = 0);
	int						DisassembleLine(int address, char* line);
	int						DisassembleLine(int address, unsigned char opcode, unsigned short operand, char* line);
	void					SetBaseAddress(int address);
	void					AppendComments(char* line, int opcode, int address, int oldSchool = 1);
	void					LoadPrefs(void);
	void					SavePrefs(void);
	void					SetTextEditor(My_Text_Editor* pTd) { m_pTd = pTd; }
	void					SetEditorStyle(int blackBackground, int colorHilight, int fontSize);
	void					ResetHilight(void);
	void					ShowDisassemblyMessage(void);
	void					CreateFindDlg(void);
	void					FindDlg(void);
	void					FindNext(void);
	void					OpenFile(void);
	void					SaveFile(void);

protected:
	void					DisassembleAddLabel(Fl_Text_Buffer* tb, int addr);
	void					DisassembleAsString(Fl_Text_Buffer* tb, int addr, int size);
	void					TestForStringArg(int address);
	void					ScanForCodeBlocks(void);
	void					ScanForStrings(void);

	char* 					m_StrTable[256];
	unsigned char			m_LenTable[256];
	int						m_BaseAddress;
	DisType_t*				m_pDisType;
	My_Text_Editor*			m_pTd;
	VTDisFindDlg*			m_pFindDlg;
	MString					m_LastDir;
};


#endif

