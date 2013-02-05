/* memedit.h */

/* $Id: memedit.h,v 1.4 2011/07/09 08:16:21 kpettit1 Exp $ */

/*
 * Copyright 2004 Ken Pettit and Stephen Hurd 
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


#ifndef MEMEDIT_H
#define MEMEDIT_H


#include "FL/Fl_File_Chooser.H"

void cb_MemoryEditor (Fl_Widget* w, void*);
void cb_MemoryEditorUpdate(void);

void MemoryEditor_LoadPrefs(void);
void MemoryEditor_SavePrefs(void);

#define		MENU_HEIGHT	32

class T100_MemEditor : public Fl_Widget
{
public:
	T100_MemEditor(int x, int y, int w, int h);
	~T100_MemEditor();

	void			SetMemRegion(int region);
	void			SetMemAddress(int address);
	void			SetScrollSize(void);
	void			SetRegionOptions(void);
	int				GetRegionEnum(void);
	void			UpdateAddressText(void);
	void			UpdateDispMem(void);
	void			MoveTo(int value);
	int				m_FirstLine;
	int				m_x, m_y, m_w, m_h;		// user preferences for window position
				
protected:
//	virtual int handle(int event);
	void			draw();
	virtual int		handle(int event);
	void			DrawCursor(void);
	void			EraseCursor(void);
	void			ScrollUp(int lines);
	void			ScrollDown(int lines);

	int				m_MyFocus;
	int				m_Region;
	int				m_Cols;
	int				m_Lines;
	double			m_Max;
	double			m_Height;
	double			m_Width;
	int				m_FirstAddress;
	int				m_FontSize;

	int				m_CursorAddress;
	int				m_CursorCol;
	int				m_CursorRow;
	int				m_CursorField;

	// Current selection control -- active & start / end addresses
	int				m_SelActive;
	long			m_SelEndRow;
	long			m_SelEndCol;

	Fl_Scrollbar*	m_pScroll;
};

#endif
