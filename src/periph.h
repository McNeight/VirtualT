/* periph.h */

/* $Id: periph.h,v 1.0 2004/08/05 06:46:12 kpettit1 Exp $ */

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


#ifndef PERIPH_H
#define PERIPH_H

void cb_PeripheralDevices (Fl_Widget* w, void*);


#define		MENU_HEIGHT	32

typedef struct comLogEntry
{
	double			time;
	unsigned char	byte;
	unsigned char	flags;
} comLogEntry_t;
		

class TcomLogBlock
{
public:
	TcomLogBlock()	{ next = NULL; entries = new comLogEntry_t[16384];
						max = 16384; used = 0; };
	~TcomLogBlock() { if (next != NULL) delete next; delete entries; };
	
	TcomLogBlock			*next;
	struct	comLogEntry		*entries;
	int						max;
	int						used;
};

typedef struct lineStart
{
	TcomLogBlock	*clb;
	int				index;
} lineStart_t;

class T100_ComMon : public Fl_Widget
{
public:
	T100_ComMon(int x, int y, int w, int h);
	~T100_ComMon();

	void			AddByte(int rx_tx, char byte, char flags);
	void			CalcLineStarts(void);
	void			Clear(void);
	int				m_FirstLine;
				
protected:
//	virtual int handle(int event);
	void			draw();
	virtual int		handle(int event);

	int				m_MyFocus;
	int				m_Cols;
	int				m_Lines;
	double			m_Height;
	double			m_Width;
	int				m_LastLine;
	int				m_LastCol;
	int				m_LastCnt;
	comLogEntry_t*	m_pLastEntry;
	comLogEntry_t*	m_pStartTime;
	comLogEntry_t*	m_pStopTime;
	int				m_StartTimeCol;
	int				m_StartTimeLine;
	int				m_StopTimeCol;
	int				m_StopTimeLine;
	TcomLogBlock*	m_log;
	int				m_LineStartCount;
	lineStart_t		m_LineStarts[20000];
	Fl_Scrollbar*	m_pScroll;

};



#endif
