/* printer.cpp */

/* $Id: printer.cpp,v 1.1 2008/02/17 13:25:27 kpettit1 Exp $ */

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

#include <FL/Fl.H>
#include <FL/fl_draw.H>
#include <FL/Fl_Window.H>

#include <string.h>
#include <stdio.h>
#include <stdlib.h>

#include "VirtualT.h"
#include "printer.h"

/*
================================================================================
VTPrinter:	This is the virtual class for printers to derive their functinaltiy.
================================================================================
*/
VTPrinter::VTPrinter(void)
{
	m_pPref = NULL;
	m_SessionActive = FALSE;
	m_initialized = FALSE;
}

/*
================================================================================
Init routine to save the Fl_Preferences pointer and call the printer's Init.
================================================================================
*/
void VTPrinter::Init(Fl_Preferences* pPref)
{
	if (pPref != NULL)
	{
		// Save the preferences pointer
		m_pPref = pPref;

		if (!m_initialized)
		{
			// call the pure virtual Init funciton
			Init();
			m_initialized = TRUE;
		}
	}
}

/*
================================================================================
Init routine to save the Fl_Preferences pointer and call the printer's Init.
================================================================================
*/
int VTPrinter::Print(unsigned char byte)
{
	int		err;

	// Test if there is an acive session.  If not, start one
	if (!m_SessionActive)
	{
		// Try to open a new print session with the printer
		if ((err = OpenSession()))
		{
			// Deal with error here

			return err;
		}
		
		// Indicate session open
		m_SessionActive = TRUE;
	}

	// Now print the byte
	PrintByte(byte);

	return PRINT_ERROR_NONE;
}

/*
================================================================================
Ends the current print session (if any).
================================================================================
*/
void VTPrinter::EndPrintSession(void)
{
	// Close the session only if there is one open
	if (m_SessionActive)
	{
		CloseSession();
		m_SessionActive = FALSE;
	}
}

/*
================================================================================
Get the text for a specified error number
================================================================================
*/
MString VTPrinter::GetError(int index)
{
	MString		none;

	if (index < m_errors.GetSize())
		return m_errors[index];

	return	none;
}

