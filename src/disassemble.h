/* disassemble.h */

/* $Id: $ */

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


#ifndef _DISASSEMBLE_H_
#define _DISASSEMBLE_H_

#include "display.h"

typedef struct Std_ROM_Addresses {
	int	addr;
	char*		desc;
} Std_ROM_Addresses_t;

typedef struct Std_ROM_Table {
	int addr;
	int size;
	char type;
} Std_ROM_Table_t;

typedef struct RomDescription {
	Std_ROM_Table_t		*pTables;
	Std_ROM_Addresses_t *pVars;
	Std_ROM_Addresses_t	*pFuns;
} RomDescription_t;


class VTDis 
{
public:
	int m_EndAddress;
	void CopyIntoMem(unsigned char * ptr, int len);
	unsigned char m_memory[65536];
	int m_StartAddress;
	class Fl_Text_Editor* m_pTextViewer;
	RomDescription_t	*m_pRom;

	VTDis();

	void Disassemble();
	void SetTextViewer(class Fl_Text_Editor* pTextViewer);

protected:
	static char* m_StrTable[256];
	static unsigned char m_LenTable[256];

};

enum {
	 TABLE_TYPE_STRING
	,TABLE_TYPE_JUMP
	,TABLE_TYPE_CODE
	,TABLE_TYPE_MODIFIED_STRING
	,TABLE_TYPE_2BYTE
	,TABLE_TYPE_3BYTE
	,TABLE_TYPE_CTRL_DELIM
	,TABLE_TYPE_BYTE_LOOKUP
	,TABLE_TYPE_4BYTE_CMD
	,TABLE_TYPE_CATALOG
};

#endif