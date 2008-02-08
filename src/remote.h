/* remote.h */

/* $Id: remote.h,v 1.1 2008/01/26 14:42:51 kpettit1 Exp $ */

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


#ifndef _REMOTE_H_
#define _REMOTE_H_

#ifdef __cplusplus
extern "C" {
#endif

void	init_remote(void);
void	lock_remote(void);
void	unlock_remote(void);

#ifdef __cplusplus
}
#endif

#define	BPTYPE_MAIN		0x01
#define	BPTYPE_OPT		0x02
#define	BPTYPE_MPLAN	0x04
#define	BPTYPE_RAM		0x08
#define	BPTYPE_RAM2		0x10
#define	BPTYPE_RAM3		0x20
#define	BPTYPE_READ		0x40
#define	BPTYPE_WRITE	0x80

typedef struct
{
	int		top_row;
	int		top_col;
	int		bottom_row;
	int		bottom_col;
} lcd_rect_t;

#endif
