/* io.h */

/* $Id: io.h,v 1.1.1.1 2004/08/05 06:46:11 deuce Exp $ */

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


#ifndef _IO_H_
#define _IO_H_

#include "gen_defs.h"


#ifdef __cplusplus
extern "C" {
#endif

extern unsigned long gSpecialKeys;
extern unsigned char gKeyStates[128];
extern uchar ioBA;
extern	int	gModel;

void init_io(void);
void deinit_io(void);
void out(uchar port, uchar val);
void update_keys(void);
void io_set_ram_bank(unsigned char bank);
int inport(uchar port);
#ifdef __cplusplus
}
#endif


#define	MT_SHIFT		0x00000001
#define	MT_CTRL			0x00000002
#define	MT_GRAPH		0x00000004
#define	MT_CODE			0x00000008
#define	MT_NUM			0x00000010     // Not used
#define	MT_CAP_LOCK		0x00000020
#define	MT_PAUSE		0x00000080

#define	MT_F1			0x00000100
#define	MT_F2			0x00000200
#define	MT_F3			0x00000400
#define	MT_F4			0x00000800
#define	MT_F5			0x00001000
#define	MT_F6			0x00002000
#define	MT_F7			0x00004000
#define	MT_F8			0x00008000

#define MT_SPACE		0x00010000
#define	MT_BKSP			0x00020000
#define	MT_TAB			0x00040000
#define	MT_ESC			0x00080000
#define	MT_PASTE		0x00100000
#define	MT_LABEL		0x00200000     // F9
#define	MT_PRINT		0x00400000
#define	MT_ENTER		0x00800000
#define	MT_INS			0x01000000
#define	MT_DEL			0x02000000


#define	MT_LEFT			0x10000000
#define	MT_RIGHT		0x20000000
#define	MT_UP			0x40000000
#define	MT_DOWN			0x80000000

#endif
