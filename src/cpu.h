/* cpu.h */

/* $Id: cpu.h,v 1.1.1.1 2004/08/05 06:46:12 deuce Exp $ */

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


#ifndef _CPU_H_
#define _CPU_H_

#include <sys/types.h>

#include "gen_defs.h"

#define RAMSIZE		32768
#define ROMSIZE		32768
#define ADDRESSSPACE	65536

extern uchar cpu[14];
extern uchar memory[ADDRESSSPACE];
extern uchar sysROM[ROMSIZE];
extern uchar optROM[ROMSIZE];

#define A cpu[0]
#define F cpu[1]
#define B cpu[2]
#define C cpu[3]
#define D cpu[4]
#define E cpu[5]
#define H cpu[6]
#define L cpu[7]
#define PCH cpu[8]
#define PCL cpu[9]
#define SPH cpu[10]
#define SPL cpu[11]

/* Thoses macros can NOT be set */
#define AF ((((ushort)A)<<8)|F)
#define BC ((((ushort)B)<<8)|C)
#define DE ((((ushort)D)<<8)|E)
#define HL ((((ushort)H)<<8)|L)
#define PC ((((ushort)PCH)<<8)|PCL)
#define SP ((((ushort)SPH)<<8)|SPL)
#define INCPC	{PCL++;  if(PCL==0) PCH++;}
#define INCPC2	{PCL+=2; if(PCL<2)  PCH++;}
#define INCSP	{SPL++;  if(SPL==0) SPH++;}
#define INCSP2	{SPL+=2; if(SPL<2)  SPH++;}
#define DECPC	{PCL--;  if(PCL==0xff) PCH--;}
#define DECPC2	{PCL-=2; if(PCL>0xfd)  PCH--;}
#define DECSP	{SPL--;  if(SPL==0xff) SPH--;}
#define DECSP2	{SPL-=2; if(SPL>0xfd)  SPH--;}
#define SETPCINS16 {int pc=PC; PCL=memory[pc++]; PCH=memory[pc];}

#define IM cpu[12]
/* bit 0 is interrupts disabled (1==disabled) */
#define cpuMISC cpu[13]
#define M memory[HL]
#define INS memory[PC]
#define NXTINS memory[PC+1]
#define INS16 (((int)memory[PC])|(((int)memory[PC+1])<<8))
#define CF (F&0x01)
#define XF ((F&0x02)>>1)
#define XF_BIT	0x02
#define PF ((F&0x04)>>2)
#define AC ((F&0x10)>>4)
#define ZF ((F&0x40)>>6)
#define SF ((F&0x80)>>7)
#define RST55MASK	(IM&0x01)
#define RST65MASK	((IM&0x02)>>1)
#define RST75MASK	((IM&0x04)>>2)
#define INTDIS		((IM&0x08)>>3)
#define RST55PEND	((IM&0x10)>>4)
#define RST65PEND	((IM&0x20)>>5)
#define RST75PEND	((IM&0x40)>>6)
#define SOD		((IM&0x80)>>7)

#define MEM(x)		memory[x]
#define MEM16(x)	(((ushort)MEM(x))|((ushort)MEM(x+1))<<8)

#endif
