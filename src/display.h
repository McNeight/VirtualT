/* display.h */

/* $Id: display.h,v 1.12 2015/02/24 20:19:17 kpettit1 Exp $ */

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


#ifndef _DISPLAY_H_
#define _DISPLAY_H_

#define MENU_HEIGHT	32
#define	TAB_HEIGHT	24

#ifdef __cplusplus
extern "C" {
#endif
extern int		gDelayUpdateKeys;
void			init_pref(void);
void			init_display(void);
void			deinit_display(void);
void			drawbyte(int driver, int column, int value);
void			lcdcommand(int driver, int value);
void			power_down();
void			process_windows_event();
void			display_cpu_speed(void);
void			display_map_mode(char *str);
void			show_error(const char*);
void			t200_command(unsigned char ir, unsigned char data);
unsigned char	t200_readport(unsigned char port);
void			handle_simkey(void);
void			switch_model(int);
void			init_other_windows(void);
void			enable_tpdd_log_menu(int bEnabled);
#ifdef __cplusplus
void			collapse_window(Fl_Window* pWin);
void			expand_window(Fl_Window* pWin, int newx, int newy, int neww, int newh);
#endif

typedef int 	(*get_key_t)(int);
typedef int 	(*event_key_t)(void);

#ifdef __cplusplus

//#include <FL/Fl.H>
//#include <FL/Fl_Menu_Button.H>

#define	VT_SIM_KEYUP	3120
#define	VT_SIM_KEYDOWN	3121

class T100_Disp : public Fl_Widget
{
public:
	T100_Disp(int x, int y, int w, int h);
	~T100_Disp();

	virtual void	PowerDown();
	virtual void	SetByte(int driver, int col, uchar value);
	virtual void	Command(int instruction, uchar data);
	virtual void	Clear(void);
	virtual void	Reset(void);
	virtual void	CalcScreenCoords(void);
	void			SimulateKeydown(int key);
	void			SimulateKeyup(int key);
	void			HandleSimkey(void);
	void			WheelKey(int key);
	int				IsInMenu(void);
	int				IsInText(void);
	int				ButtonClickInMenu(int mx, int my);
	int				ButtonClickInMsPlan(int mx, int my);
	int				ButtonClickInText(int mx, int my);
	int				MouseMoveInText(int mx, int my);

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

	int				m_DebugMonitor;
	int				m_WheelKeys[32];
	int				m_WheelKeyIn;
	int				m_WheelKeyOut;
	int				m_WheelKeyDown;
	class Fl_Menu_Button*	m_CopyCut;
	class Fl_Menu_Button*	m_LeftClick;
	class Fl_Menu_Button*	m_LeftClickQuad;
	char			m_SimulatedCtrl;
	char			m_SelectComplete;
	char			m_HaveMouse;
	char			m_Select;
	int				m_LastPos;
#ifdef ZIPIT_Z2
	int				m_xCoord[241];
	int				m_yCoord[241];
	int				m_rectSize[241];
#endif

	const virtual	T100_Disp& operator=(const T100_Disp& srcDisp);

	virtual int		handle(int event);
protected:
	virtual void	draw();
	static int		sim_get_key(int key);
	static int		sim_event_key(void);
	inline void	drawpixel(int x, int y, int color);
	virtual void	draw_static();
	static int		m_simKeys[32];
	static int		m_simEventKey;

	int				m_MyFocus;
	uchar			lcd[10][256];
	uchar			top_row[10];

};


class T200_Disp : public T100_Disp
{
public:
	T200_Disp(int x, int y, int w, int h);
	virtual void	Command(int instruction , uchar data);
	virtual void	SetByte(int driver, int col, uchar value);
	unsigned char	ReadPort(unsigned char port);
	const virtual			T200_Disp& operator=(const T200_Disp& srcDisp);

protected:
	virtual void	draw();
	virtual void	redraw_active();

	uchar			m_ram[8192];			/* T200 Display RAM storage */
	uchar			m_mcr;					/* Mode Control Register */
	uchar			m_hpitch;				/* Horizontal Character pitch */
	uchar			m_hcnt;					/* Horizontal character count */
	uchar			m_tdiv;					/* Time divisions */
	uchar			m_curspos;				/* Cursor position */
	uchar			m_dstartl;				/* Display start Low */
	uchar			m_dstarth;				/* Display start High */
	int				m_last_dstart;			/* Last dstart for clear operations */
	uchar			m_cursaddrl;			/* Cursor address Low */
	uchar			m_cursaddrh;			/* Cursor address High */
	uchar			m_ramwr;				/* Data to be written to the RAM */
	uchar			m_ramrd;				/* Data read from the RAM */
	uchar			m_ramrd_dummy;			/* Dummy read register */
	int				m_ramrd_addr;			/* Address for RAM reads */
	int				m_redraw_count;

	enum {
		SET_MCR,
		SET_HPITCH,
		SET_HCNT,
		SET_TDIV,
		SET_CURSPOS,
		SET_DSTARTL = 8,
		SET_DSTARTH,
		SET_CURSADDRL,
		SET_CURSADDRH,
		RAM_WR,
		RAM_RD,
		RAM_BIT_CLR,
		RAM_BIT_SET 
	};
};

}
#endif

#endif
