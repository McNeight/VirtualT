/* tdock.h */

/* $Id: tdock.h,v 1.2 2015/02/11 17:31:49 kpettit1 Exp $ */

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

#ifndef VDOCK_H
#define VDOCK_H

/*
============================================================================
Define call routines to hook to serial port functionality.
============================================================================
*/
#ifdef __cplusplus

extern "C"
{
#endif /* __cplusplus */

// Now define the function prototypes
void            tdock_init(void);
unsigned char	tdock_read(void);
void			tdock_write(unsigned char device, unsigned char data);
#ifdef __cplusplus
}

class VTTDock;

/*
=====================================================================
Define the TDock class.  This will be passed around in C land
as a void* context.
=====================================================================
*/
class VTTDock 
{
public: 
	VTTDock();
	~VTTDock();

    void            Init(void);
	void			Write(unsigned char data);
	unsigned char	Read(void);
};

#endif  /* __cplusplus */

#endif	/* VDOCK_H */

