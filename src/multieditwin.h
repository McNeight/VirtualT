/* multieditwin.h */

/* $Id: multieditwin.h,v 1.4 2014/05/09 18:27:44 kpettit1 Exp $ */

/*
 * Copyright 2007 Ken Pettit
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


#ifndef _MULTIEDITWIN_H_
#define _MULTIEDITWIN_H_

#include "multiwin.h"
#include "My_Text_Editor.h"
#include "highlight.h"

#ifndef TRUE
#define	TRUE 1
#endif

class Fl_Multi_Edit_Window : public My_Text_Editor, public VTObject
{
public:
	Fl_Multi_Edit_Window(int x=0, int y=0, int w=600, int h=500, const char *label = 0);
	~Fl_Multi_Edit_Window();

	DECLARE_DYNCREATE(Fl_Multi_Edit_Window)

	void			OpenFile(const MString& filename);
	void			SaveFile(const MString& rootpath);
	void			SaveAs(const MString& rootpath);
	void			Copy(void);
	void			Cut(void);
	void			Paste(void);
	int				ReplaceAll(const char * pFind, const char *pReplace);
	int				ReplaceNext(const char * pFind, const char *pReplace);
	int				IsModified(void) { return m_Modified; }
	void			Modified(void);
	void			ModifedCB(int, int, int, int, const char *);
	const MString&	Filename(void) { return m_FileName; }
	void			Title(const MString& title);
	MString&		Title(void) { return m_Title; }
	void			DisableHl(void);
	void			EnableHl(void);
	int				ForwardSearch(const char *pFind, int caseSensitive = TRUE);
	virtual void	show(void) { My_Text_Editor::show(); Fl_Widget::show(); }
    virtual void	buffer(My_Text_Buffer* buf);
	void			tab_distance(int);
	My_Text_Buffer* buffer() { return My_Text_Display::buffer(); }

	My_Text_Editor*	m_te;
protected:
	My_Text_Buffer*	m_tb;
	HighlightCtrl_t	*m_pHlCtrl;
	MString			m_FileName;
	MString			m_Title;
	int				m_Modified;
	int				m_TabChange;
	int virtual		OkToClose(void);
};

#endif

