/* hostprint.h */

/* $Id: hostprint.h,v 1.1 2008/02/17 13:25:27 kpettit1 Exp $ */

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


#ifndef _HOSTPRINT_H_
#define _HOSTPRINT_H_

#include "printer.h"

/*
==========================================================================
Define the VTHostPrint class.  This is an implementation of a printer 
that sends all print data to a specified Host port or Host device.
==========================================================================
*/
class VTHostPrint : public VTPrinter
{
public:
	VTHostPrint();

	virtual MString		GetName();					// Get name of the printer
	virtual void		BuildPropertyDialog();		// Build the host dialog
	virtual int			GetProperties(void);		// Get dialog properties and save
	virtual int			GetBusyStatus();			// Read BUSY from host
	virtual void		SendAutoFF(void);			// Send a FF if needed
	virtual void		Deinit(void);				// Deini routine
	virtual int			CancelPrintJob(void);		// Cancels a print job

protected:
	virtual void 		PrintByte(unsigned char byte);	// Print to host
	virtual void		Init(void);					// Init routine
	virtual int			OpenSession(void);			// Opens a new print session
	virtual int			CloseSession(void);			// Closes active print session

	Fl_Input*			m_pHostPort;				// Control for Host Port name
	Fl_Check_Button*	m_pClosePort;				// Check box for closing port
	Fl_Check_Button*	m_pReadBusy;				// Check box for reading BUSY status

	char				m_HostPort[256];			// Host port name
	int					m_ClosePort;				// True to close port between sessions
	int					m_ReadBusy;					// True to read BUSY status from port

	int					m_PrevChar;					// Previous character written

#ifdef WIN32

#else

	int					m_OutFd;					// Output File description

#endif
};

#endif
