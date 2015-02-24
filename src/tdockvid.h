/* tdockvid.h */

/* $Id: tdockvid.h,v 1.10 2013/03/05 20:43:46 kpettit1 Exp $ */

/*
 * Copyright 2015 Ken Pettit
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


#ifndef _TDOCKVID_H_
#define _TDOCKVID_H_

#ifndef MENU_HEIGHT
#define MENU_HEIGHT	32
#endif

#ifndef TAB_HEIGHT
#define	TAB_HEIGHT	24
#endif

#ifdef __cplusplus

#define ESC_ERASE_SCREEN        0x6A    /* j */
#define ESC_ERASE_EOL           0x4B    /* K */
#define ESC_CLEAR_EOL           0x4A    /* J */
#define ESC_ERASE_LINE          0x6C    /* l */
#define ESC_INS_LINE            0x4C    /* L */
#define ESC_DEL_LINE            0x4D    /* M */
#define ESC_CURS_POS            0x59    /* Y */
#define ESC_CURSOR_UP           0x41    /* A */
#define ESC_CURSOR_DOWN         0x42    /* B */
#define ESC_CURSOR_RIGHT        0x43    /* C */
#define ESC_CURSOR_LEFT         0x44    /* D */
#define ESC_CURSOR_HOME         0x48    /* H */
#define ESC_START_REVERSE       0x70    /* p */
#define ESC_END_REVERSE         0x71    /* q */
#define ESC_CURSOR_ON           0x50    /* P */
#define ESC_CURSOR_OFF          0x51    /* Q */
#define ESC_SET_SYS_LINE        0x54    /* T */
#define ESC_RESET_SYS_LINE      0x55    /* U */
#define ESC_LOCK_DISPLAY        0x56    /* V */
#define ESC_UNLOCK_DISPLAY      0x57    /* W */
#define ESC_WIDTH_40            0x63    /* c */
#define ESC_WIDTH_80            0x64    /* d */
#define ESC_SET_COLOR           0x65    /* e */
#define ESC_SET_COLOR_NAME      0x66    /* f */
#define ESC_SET_BG_NAME         0x67    /* g */
#define ESC_HELP                0x68    /* h */

//#include <FL/Fl.H>
//#include <FL/Fl_Menu_Button.H>

class VTTDockVid : public Fl_Widget
{
public:
	VTTDockVid(int x, int y, int w, int h);
	~VTTDockVid();

	virtual void	WriteData(unsigned char data);

	virtual void	Clear(void);

	int				MultFact;
	int				DisplayMode;
	int				SolidChars;
	int				DispHeight;

	int				gRectsize;
	int				gXoffset;
	int				gYoffset;
	int				m_BezelTop;
	int				m_BezelLeft;
	int				m_BezelBottom;
	int				m_BezelRight;
	int				m_BezelTopH;
	int				m_BezelBottomH;
	int				m_BezelLeftW;
	int				m_BezelRightW;
	int				m_HasTopChassis;
	int				m_HasBottomChassis;
	int				m_HasLeftChassis;
	int				m_HasRightChassis;

	int				m_FrameColor;
	int				m_DetailColor;
	int				m_BackgroundColor;
	int				m_PixelColor;
	int				m_LabelColor;

	char			m_HaveMouse;

protected:
	virtual void	draw();
    void            draw_pixels();
    void            SetByte(int line, int col, uchar value);
    void            CalcScreenCoords(void);
    int             FindColorByName(char *pName);
    void            XorCursor(void);
    void            Help(void);
    void            Scroll(void);
	__inline void	drawpixel(int x, int y, int color);
	virtual void	draw_static();
    virtual int     handle(int event);

	int				m_MyFocus;
	uchar			pixdata[200][480];

    int             m_CurX, m_CurY;
    int             m_Cursor;
    int             m_Reverse;
    int             m_Locked;
    int             m_Width;
    int             m_Colors[256];
    unsigned int    m_Color;

    int             m_EscSeq;           /* Non zero for ESC sequence.  # Bytes RX otherwise */
    char            m_EscData[256];     /* Chars RX after the ESC */

};

#endif

#endif
