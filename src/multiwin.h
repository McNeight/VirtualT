/* multiwin.cpp */

/* $Id: multiwin.cpp,v 1.1.1.1 2004/08/05 06:46:12 kpettit1 Exp $ */

/*
 * Copyright 2006 Ken Pettit
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


#ifndef _MULTIWIN_H_
#define _MULTIWIN_H_

#define	MW_TITLE_HEIGHT	28
#define	MW_BORDER_WIDTH	4
#define	MW_MINIMIZE_WIDTH	150

#include <FL/Fl_Double_Window.H>
#include <FL/Fl_Pixmap.H>
#include "vtobj.h"

class Fl_Multi_Window : public Fl_Double_Window
{
public:
	Fl_Multi_Window(int x, int y, int w, int h, const char *label = 0);
	~Fl_Multi_Window();

	Fl_Window*		ClientArea() { return m_pClientArea; };

	int				m_NoResize;

protected:
	void virtual	draw(void);
	int	 virtual	handle(int event);
	void			CloseIconSelected();
	void			MaximizeIconSelected();
	void			MinimizeIconSelected();

	Fl_Window*		m_pClientArea;
	Fl_Pixmap*		m_pIcon;
	char*			m_pLabel;
	int				m_InDragArea;
	int				m_InResizeArea;
	int				m_InIconArea;
	int				m_LastInIconArea;
	VT_Rect			m_IconArea[3];
	Fl_Pixmap*		m_NormalIcon[3];
	Fl_Pixmap*		m_PushedIcon[3];
	Fl_Pixmap*		m_InactiveIcon[3];
	int				m_Minimized;
	int				m_Maximized;
	VT_Rect			m_MinimizeRect;
	VT_Rect			m_RestoreRect;
	int				m_MinimizeLoc;
	static int		m_MinimizeRegions[200];

	int				m_CaptureMode;
	int				m_MouseLastX, m_MouseLastY;
	int				m_MouseAnchorX, m_MouseAnchorY;
	int				m_WinAnchorX, m_WinAnchorY;

	static	int		m_WindowCount;
};

#endif

