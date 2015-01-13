/* doins.h */

/* $Id: doins.h,v 1.6 2014/04/24 23:30:14 kpettit1 Exp $ */

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


#ifndef _DOINS_H_
#define _DOINS_H_

extern char paritybits[256];

static inline void setflags(unsigned char regval, char sign, char zero, char auxcarry, char parity, char carry, char ov)
{
	if (sign!=-2)
	{
		if(sign>=0)
			F=(F&~SF_BIT)|(sign?SF_BIT:0);
		else 
			F=(F&~SF_BIT)|(regval&SF_BIT);
	}

	if (zero != -2)
	{
		if(zero>=0)
			F=(F&~ZF_BIT)|(zero?ZF_BIT:0);
		else
			F=(F&~ZF_BIT)|(regval?0:ZF_BIT);
	}

	if(auxcarry>=0)
		F=(F&~AC_BIT)|(auxcarry?AC_BIT:0);

	if (parity != -2)
	{
		if(parity>=0)
			F=(F&~PF_BIT)|paritybits[parity & 0xFF];
		else
			/* Table Lookup */
			F=(F&~PF_BIT)|paritybits[regval & 0xFF];
	}

	if(carry>=0)
		F=(F&~CF_BIT)|(carry?CF_BIT:0);

	if (ov >= 0)
		F=(F&~OV_BIT)|(ov?OV_BIT:0);
}

#endif
