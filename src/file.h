/* file.h */

/* $Id: file.h,v 1.9 2013/02/08 00:07:52 kpettit1 Exp $ */

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


#ifndef _FILE_H_
#define _FILE_H_

#include "gen_defs.h"

#define TYPE_BA	0x80
#define	TYPE_CO	0xA0
#define	TYPE_DO	0xC0
#define TYPE_HEX 0x40

void	cb_LoadRam (Fl_Widget* w, void*);
void	cb_SaveRam (Fl_Widget* w, void*);
void	cb_LoadOptRom (Fl_Widget* w, void*);
void	cb_UnloadOptRom (Fl_Widget* w, void*);
void	cb_LoadFromHost(Fl_Widget* w, void*);
void	cb_SaveToHost(Fl_Widget* w, void*);
int  	load_optrom_file(const char* filename);
int		delete_file(const char* filename, unsigned short addr = 0);

typedef struct model_t_files {
	char	name[10];
	unsigned short	address;
	unsigned short	dir_address;
	unsigned short	size;
} model_t_files_t;

#define	FILE_ERROR_INVALID_HEX			1
#define FILE_ERROR_FILE_NOT_FOUND		2

extern int		BasicSaveMode;
extern int		COSaveMode;

#ifdef	__APPLE__
//JV 10/08/05
const char* ChooseWorkDir();

#ifdef __cplusplus
extern "C" {
#endif
extern char path[255];
#ifdef __cplusplus
}
#endif

//JV
#endif

#endif

