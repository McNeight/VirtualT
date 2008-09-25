/* clock.h */

/* $Id: clock.h,v 1.2 2008/03/31 22:09:17 kpettit1 Exp $ */

/*
 * Copyright 2008 Ken Pettit
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

#ifndef CLOCK_H
#define CLOCK_H

#ifdef __cplusplus
extern "C" {
#endif

void	init_clock				(void);
void	rp5c01_write			(uchar port, uchar val);
uchar	rp5c01_read				(uchar val);
void	pd1990ac_chip_cmd		(uchar val);
void	pd1990ac_clk_pulse		(uchar val);
void	save_model_time(void);
void	get_model_time(void);

#ifdef __cplusplus
}
#endif

#ifdef __cplusplus

void load_clock_preferences(Fl_Preferences* pPref);
void save_clock_preferences(Fl_Preferences* pPref);
void get_clock_options(void);
void build_clock_setup_tab(void);

typedef struct
{
	Fl_Round_Button*	pSysTime;
	Fl_Round_Button*	pEmulTime;
	Fl_Check_Button*	pReload;
	Fl_Check_Button*	pTimeElapse;
} clock_ctrl_t;

#define	CLOCK_MODE_SYS		1
#define	CLOCK_MODE_EMUL		2

#endif


#endif
