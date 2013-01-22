/* idetabs.h */

/* $Id: idetabs.h,v 1.1 2011/07/09 08:16:21 kpettit1 Exp $ */

/*
 * Copyright 2009 Ken Pettit
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

#ifndef _IDETABS_H_
#define _IDETABS_H_

#include <FL/Fl_Tabs.H>
#include "vtobj.h"

#define	FL_IDE_TABS_CLOSE	(0xDEADD00D)

class Fl_Ide_Tabs : public Fl_Group, public VTObject
{
public:
	Fl_Ide_Tabs(int x=0, int y=0, int w=600, int h=500, const char *label = 0);
	~Fl_Ide_Tabs();

	DECLARE_DYNCREATE(Fl_Ide_Tabs)

	Fl_Widget *value();
	int value(Fl_Widget *);
	Fl_Widget *push() const {return push_;}
	int push(Fl_Widget *);
	Fl_Widget *which(int event_x, int event_y);
	void has_close_button(int bHasCloseButton) { m_hasCloseButton = bHasCloseButton; }

protected:
	VT_Rect	m_closeRect;
	int		m_prevInRect;
	int		m_pushInRect;
	int		m_hasCloseButton;
	Fl_Widget *value_;
	Fl_Widget *push_;

    virtual int handle(int e);
    virtual int orig_handle(int e);
	void	redraw_tabs();
	void	draw(void);
	int		tab_positions(int* p, int* wp);
	int		tab_height();
	void	draw_tab(int x1, int x2, int W, int H, Fl_Widget* o, int what);
};

#endif

