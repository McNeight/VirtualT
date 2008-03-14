/* fileprint.h */

/* $Id: fileprint.h,v 1.1 2008/02/17 13:25:27 kpettit1 Exp $ */

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


#ifndef _FILEPRINT_H_
#define _FILEPRINT_H_

#include "printer.h"

/*
==========================================================================
Define the VTFilePrint class.  This is an implementation of a printer 
that sends all print data to output files.
==========================================================================
*/
class VTFilePrint : public VTPrinter
{
public:
	VTFilePrint();

	virtual MString		GetName();					// Get name of the printer
	virtual void		BuildPropertyDialog(Fl_Window *pParent);		// Specific dialog for File I/O
	virtual int			GetProperties(void);		// Get dialog properties and save
	virtual int 		GetBusyStatus();			// Does nothing for file I/O
	virtual void		SendAutoFF(void);			// Send a FF to printer
	void				UpdateFormatForPage(void);
	virtual void		Deinit(void);				// Deinit routine
	virtual int			CancelPrintJob(void);

protected:
	virtual void		Init(void);					// Initialization routine
	virtual void		PrintByte(unsigned char byte);	// Print a byte to the file
	virtual int			OpenSession(void);			// Opens a file output session
	virtual int			CloseSession(void);			// Closes a file output session

	int					PromptFilename(MString& filename);
	MString				GenFilename(void);			// Generates autofilename
	void				OpenNextPage(void);			// Opens the next page of the output

	Fl_Input*			m_pDirName;					// Edit field for Dir Name
	Fl_Check_Button*	m_pPrompt;
	Fl_Check_Button*	m_pSeparateFiles;			// Control for splitting pages into files
	Fl_Check_Button*	m_pAutoFormat;				// Control for automatically format filenames
	Fl_Input*			m_pFormatCode;				// Control for spcifying format string
	Fl_Check_Button*	m_pFilterCodes;				// Control for filtering control codes

	char				m_DirName[256];				// Name of file output directory
	int					m_Prompt;					// True if prompt for filename
	int					m_SeparateFiles;			// True if printing pages to separate files
	int					m_AutoFormat;				// True if generating auto filenames
	char				m_FormatCode[256];			// String for autoformat filenames
	int					m_FilterCodes;				// True if filtering control codes

	FILE*				m_OutFd;					// Output File Decriptor
	MString				m_Filename;					// Filename of active print file
	unsigned char		m_LastBytePrinted;
	int					m_PageNum;					// Current page number of opened file
	int					m_ActiveSeqNum;				// Sequence number of active session, if any
};

#endif
