/* watchtable.h */

/* $Id: watchtable.h,v 1.1 2013/02/15 13:03:27 kpettit1 Exp $ */

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

#ifndef WATCHTABLE_H
#define WATCHTABLE_H

#include <FL/Fl_File_Chooser.H>
#include <FL/Fl_Menu_Button.H>
#include <FL/Fl_Scrollbar.H>

#include "memory.h"
#include "vtobj.h"
#include "MString.h"

/*
============================================================================
Class definition for Watch Variables
============================================================================
*/
#define		WATCH_TYPE_UCHAR		0
#define		WATCH_TYPE_SCHAR		1
#define		WATCH_TYPE_HEXCHAR		2
#define		WATCH_TYPE_USHORT		3
#define		WATCH_TYPE_SSHORT		4
#define		WATCH_TYPE_HEXSHORT		5
#define		WATCH_TYPE_STRING		6

#define		WATCH_EVENT_VAL_CHANGED	50

typedef void (*watch_event_handler_t)(int reason, void *pOpaque);

class CWatchDef : public VTObject
{
public:
	CWatchDef() { m_iAddr = -1; m_Value = -1; m_Type = 0; m_Region = REGION_RAM;
					m_ValEdit = 0; }
	MString			m_Name;
	MString			m_Addr;			// String form of address as given
	MString			m_StrData;		// String data for string format display

	int				m_iAddr;		// integer form of address
	int				m_Value;		// Current value
	int				m_Type;			// Type of watch
	int				m_Region;		// Region the variable is in
	int				m_ValEdit;		// Set true when value being edited
};

/*
============================================================================
Class definition for Watch Table Column Header boxes
============================================================================
*/
class VT_TableHeaderBox : public Fl_Box
{
public:
	VT_TableHeaderBox(int boxType, int x, int y, int w, int h, const char *title);

	virtual void		resize(int x, int y, int w, int h);
	virtual int			handle(int event);
};

/*
============================================================================
Class definition for Table Input
============================================================================
*/
class VT_TableInput : public Fl_Input
{
public:
	VT_TableInput(int x, int y, int w, int h, const char *title);

	virtual int			handle(int event);
	int					m_reason;
};

/*
============================================================================
Class definition and definesfor Watch Table Widget
============================================================================
*/
typedef struct watchtable_colors
{
	Fl_Color			watch_background;
	Fl_Color			watch_lines;
	Fl_Color			name;
	Fl_Color			address;
	Fl_Color			value;
	Fl_Color			type;
	Fl_Color			region;
	Fl_Color			select;
	Fl_Color			select_background;
} watchtable_color_t;

#define		VT_WATCH_TABLE_HDR_HEIGHT  20
#define		VT_WATCH_TABLE_COL_COUNT	5

class VT_Watch_Table : public Fl_Widget
{
public:
	VT_Watch_Table(int x, int y, int w, int h);
	~VT_Watch_Table();

	virtual void			draw(void);
	virtual void			resize(int x, int y, int w, int h);
	virtual int				handle(int event);

	void					header_color(Fl_Color c);
	void					HeaderResize(VT_TableHeaderBox *pBox, int x, int y, int w, int h);

	int						AddWatch(MString& name, MString& addr, int type, int region, int redraw = TRUE);
	void					DeleteWatch(int index);
	void					ResetContent(void);
	void					UpdateTable(void);
	void					SetSelectedType(int type);
	void					SetSelectedRegion(int region);
	void					PopupInputCallback(int reason);	// The popup input has lost focus or had a keypress
	void					SetScrollSizes(void);
	void					ScrollAction(void);
	void					HeaderBoxWidth(int col, int width);
	int						HeaderBoxWidth(int col) { return col < VT_WATCH_TABLE_COL_COUNT ? m_ColWidth[col] : 0; }
	MString					WatchVarsString(void);
	void					WatchVarsString(const char *pVarsString);
	void					EventHandler(watch_event_handler_t pHandler);

	int						m_DoingWidgetResize;

protected:
	virtual int				handle_variable_events(int event);
	void					SetVarValue(CWatchDef* pVar, const char *psVal);
	void					UpdateVarValue(CWatchDef* pVar);
	void					DrawWatch(CWatchDef* pVar, int line);
	void					HandleDoubleClick(int field, int line);
	void					FormatVarValue(CWatchDef* pVar, char* str, int len);
	int						WatchStringParseError(const char*& ptr);

	// FLTK conrols
	Fl_Tile*				m_pColHeaderTile;
	Fl_Box*					m_pColHeaders[VT_WATCH_TABLE_COL_COUNT];
	Fl_Box*					m_pVarWindow;
	Fl_Group*				m_pGroup;
	Fl_Scrollbar*			m_pScroll;
	Fl_Menu_Button*			m_pPopupMenu;
	Fl_Box*					m_pPopupBox;
	Fl_Input*				m_pPopupInput;

	VTObArray				m_WatchVars;
	watchtable_color_t		m_colors;

	int						m_FontSize;
	int						m_ColStart[VT_WATCH_TABLE_COL_COUNT];
	int						m_ColWidth[VT_WATCH_TABLE_COL_COUNT];
	double					m_WidthPercentage[VT_WATCH_TABLE_COL_COUNT];
	int						m_SelLine;
	int						m_DblclkX, m_DblclkY;
	int						m_MoveStartX, m_MoveStartY;
	int						m_Height;				// Font height
	int						m_PopupSelSave;
	int						m_PopupInputField;
	watch_event_handler_t	m_pEventHandler;
	MString					m_NewWatchName;
};

#endif
