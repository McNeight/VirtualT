/* autofile.h */

/* $Id: autofile.h,v 1.1 2008/03/08 13:25:27 kpettit1 Exp $ */

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


#ifndef _AUTOFILE_H_
#define _AUTOFILE_H_

#include	"MString.h"

/*
==========================================================================
Define the VTFilePrint class.  This is an implementation of a printer 
that sends all print data to output files.
==========================================================================
*/
class VTAutoFile 
{
public:
	VTAutoFile();

	MString				GenFilename(void);			// Generates autofilename
	int					PromptFilename(void);		// Prompts for filename.  True if selected
	void				FirstPage(void) { m_PageNum = 1; m_ActiveSeqNum = -1; }
	void				NextPage(void) { m_PageNum++; }

	// Sets the format string
	void				Format(MString format) { m_Format = format; }

	// Sets the format string
	void				DirName(MString dir) { m_DirName = dir; }

	// Sets the Chooser Title Text
	void				ChooserTitle(MString text) { m_ChooserTitle = text; }

	MString				m_Filename;					// Filename of active print file

protected:
	MString				m_Format;					// Format code for generting names
	MString				m_DirName;					// Directory for files
	MString				m_ChooserTitle;				// Test for file chooser
	int					m_SeparateFiles;			// True if printing pages to separate files
	int					m_PageNum;					// Current page number of opened file
	int					m_ActiveSeqNum;				// Sequence number of active session, if any
};

#endif

