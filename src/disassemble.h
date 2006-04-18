/* disassemble.h */

/* $Id: disassemble.h,v 1.1.1.1 2004/08/05 06:46:12 deuce Exp $ */

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

void disassembler_cb(Fl_Widget* w, void*);

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

	static int DisassembleLine(int address, char* line);
protected:
	static char* m_StrTable[256];
	static unsigned char m_LenTable[256];

};


#endif
