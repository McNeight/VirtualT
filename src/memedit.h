/* memedit.h */

/* $Id: memedit.h,v 1.7 2013/02/15 13:03:26 kpettit1 Exp $ */

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

#include <FL/Fl_Scrollbar.H>

#include "vtobj.h"
#include "MString.h"
#include "watchtable.h"

void cb_MemoryEditor (Fl_Widget* w, void*);
void cb_MemoryEditorUpdate(void);

void MemoryEditor_LoadPrefs(void);
void MemoryEditor_SavePrefs(void);

/*
============================================================================
Class definition for Memory Editor
============================================================================
*/
typedef struct memedit_colors
{
	Fl_Color			addr;
	Fl_Color			number;
	Fl_Color			ascii;
	Fl_Color			changed;
	Fl_Color			hilight;
	Fl_Color			hilight_background;
	Fl_Color			select;
	Fl_Color			select_background;
	Fl_Color			foreground;
	Fl_Color			background;
	Fl_Color			cursor;
} memedit_colors_t;

// Structure to keep track of region markers
typedef struct memedit_marker
{
	int						startAddr;
	int						endAddr;
	struct memedit_marker*	pNext;
} memedit_marker_t;

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
	void			ClearSelection(void) { m_SelStartAddr = -1; m_SelEndAddr = -1; }
	void			SetFontSize(int fontsize, int bold = 0);
	void			SetBlackBackground(int blackBackground);
	void			SetSyntaxHilight(int colorSyntaxHilight);
	void			UpdateColors(void);
	void			MoveTo(int value);

	int				GetFontSize(void) { return m_FontSize; }
	int				GetBold(void) { return m_Bold; }
	int				GetBlackBackground(void) { return m_BlackBackground; }
	int				GetSyntaxHilight(void) { return m_ColorSyntaxHilight; }
	Fl_Color		GetMarkerForegroundColor(void) { return m_colors.hilight; }
	Fl_Color		GetMarkerBackgroundColor(void) { return m_colors.hilight_background; }
	Fl_Color		GetSelectedForegroundColor(void) { return m_colors.select; }
	Fl_Color		GetSelectedBackgroundColor(void) { return m_colors.select_background; }
	void			SetMarkerForegroundColor(Fl_Color c) { m_colors.hilight = c; redraw(); }
	void			SetMarkerBackgroundColor(Fl_Color c) { m_colors.hilight_background = c; redraw(); }
	void			SetSelectedForegroundColor(Fl_Color c) { m_colors.select = c; redraw(); }
	void			SetSelectedBackgroundColor(Fl_Color c) { m_colors.select_background = c; redraw(); }

	int				IsHilighted(int address);	// Returns TRUE if given address should e hilighted
	int				IsSelected(int address);	// Returns TRUE if address is a marker region where the cursor is
	int				HasMarker(int address);		// Returns TRUE if given address has a marker
	void			AddMarker(int region=-1);	// Adds marker at current m_SelStart, m_SelEnd address
	void			DeleteMarker(void);			// Delete the marker at the cursor
	void			DeleteAllMarkers(void);		// Delete all markers for this region
	void			UndoDeleteAllMarkers(void);	// Undo Delete all markers for this region
	void			SaveRegionMarkers(void);	// Save region markers to the preferences
	void			LoadRegionMarkers(void);	// Load region markers from preferences
	void			AddMarkerToRegion(int start, int end, int region);
	void			FindNextMarker(void);
	void			FindPrevMarker(void);
	memedit_marker_t*	GetMarker(int address);	// Returns a pointer to the marker struct for address

	int				m_FirstLine;
	int				m_x, m_y, m_w, m_h;		// user preferences for window position
				
protected:
//	virtual int handle(int event);
	void			draw();
	virtual void resize(int x, int y, int w, int h);
	virtual int		handle(int event);
	void			DrawCursor(void);
	void			EraseCursor(void);
	void			ScrollUp(int lines);
	void			ScrollDown(int lines);
	void			ResetMarkers(void);

	int				m_MyFocus;
	int				m_HaveMouse;
	int				m_Region;
	int				m_Cols;
	int				m_Lines;
	double			m_Max;
	double			m_Height;
	double			m_Width;
	int				m_FirstAddress;
	int				m_FontSize;
	int				m_Bold;

	int				m_CursorAddress;
	int				m_CursorCol;
	int				m_CursorRow;
	int				m_CursorField;
	int				m_CursorAddr;
	int				m_DragX, m_DragY;

	// Current selection control -- active & start / end addresses
	int				m_SelActive;
	long			m_SelEndRow;
	long			m_SelEndCol;
	long			m_SelStartAddr;
	long			m_SelEndAddr;
	int				m_DblclkX, m_DblclkY;
	int				m_PopupActive;
	Fl_Font			m_Font;

	int				m_BlackBackground;
	int				m_ColorSyntaxHilight;

	memedit_colors_t	m_colors;
	memedit_marker_t*	m_pRegionMarkers[REGION_MAX];
	memedit_marker_t*	m_pUndoDeleteMarkers[REGION_MAX];

	Fl_Scrollbar*	m_pScroll;
	Fl_Menu_Button*	m_pPopupMenu;
};

#endif
