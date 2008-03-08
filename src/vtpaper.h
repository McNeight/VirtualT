/* fx80paper.h */

/* $Id: fx80paper.h,v 1.1 2008/03/05 13:25:27 kpettit1 Exp $ */

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


#ifndef _FX80PAPER_H_
#define _FX80PAPER_H_

#include "printer.h"
#include "vtobj.h"

/*
=====================================================================
Define the class to represent the "Paper" used for printer emulation.
=====================================================================
*/
class VTPaper : public VTObject
{
public:
	VTPaper(Fl_Preferences* pPref) { m_pPref = pPref; }

	virtual MString		GetName(void) = 0;				// Name of the paper
	virtual void		Init(void) = 0;					// Initializes with perferences
	virtual void		GetPrefs(void) = 0;				// Get prefs from controls & save
	virtual void		BuildControls(void) = 0;		// Build paper specific controls
	virtual void		HideControls(void) = 0;			// Hide paper specific controls
	virtual void		ShowControls(void) = 0;			// Show paper specific controls

	// PrintPage sends data to the active page, it does not send data to a printer
	virtual int			PrintPage(unsigned char* pData, int x, int y) = 0;
	virtual int			LoadPaper(void) = 0;			// Called when session is opened
	virtual	int			NewPage(void) = 0;				// Creates a new page
	virtual int			CancelJob(void) = 0;			// Cancels printing
	virtual int			Print(void) = 0;				// Send pages to the printer

protected:
	Fl_Preferences*		m_pPref;						// Pointer to preferences

};

/*
========================================================
Okay, now define VirtualPaper.  This prints to a window.
========================================================
*/
class VTVirtualPaper : public VTPaper, public Fl_Double_Window
{
public:
	VTVirtualPaper(Fl_Preferences* pPref);
	~VTVirtualPaper();

	virtual MString		GetName(void);				// Name of the paper
	virtual void		Init(void);					// Initializes with perferences
	virtual void		GetPrefs(void);				// Get prefs from controls & save
	virtual void		BuildControls(void);		// Build paper specific controls
	virtual void		HideControls(void);			// Hide paper specific controls
	virtual void		ShowControls(void);			// Show paper specific controls

	// PrintPage sends data to the active page, it does not send data to a printer
	virtual int			PrintPage(unsigned char* pData, int x, int y);
	virtual int			LoadPaper(void);			// Called when session is opened
	virtual	int			NewPage(void);				// Creates a new page
	virtual int			CancelJob(void);			// Cancels printing
	virtual int			Print(void);				// Send pages to the printer
	void				Scroll(void);				// Called to scroll the window

protected:
	void				DeletePageMemory(void);		// Deletes all allocated memory
	void				draw(void);
	void				DrawPage(int pageNum);

	unsigned char*		m_pPage;

	int					m_bytes;
	int					m_bytesPerLine;
	int					m_height;
	int					m_width;
	VTObArray			m_pPages;					// Pointers to page memory
	char*				m_pLineCnts;				// Pointer to m_height byte counts
	int					m_pageNum;					// Active page number
	int					m_topPixel;					// Top pixel on window

	Fl_Scrollbar*		m_pScroll;
};

/*
===========================================================
Define Postscript paper.  This prints to a Postscript file.
===========================================================
*/
class VTPSPaper : public VTPaper
{
public:
	VTPSPaper(Fl_Preferences* pPref);
	~VTPSPaper();

	virtual MString		GetName(void);				// Name of the paper
	virtual void		Init(void);					// Initializes with perferences
	virtual void		GetPrefs(void);				// Get prefs from controls & save
	virtual void		BuildControls(void);		// Build paper specific controls
	virtual void		HideControls(void);			// Hide paper specific controls
	virtual void		ShowControls(void);			// Show paper specific controls

	// PrintPage sends data to the active page, it does not send data to a printer
	virtual int			PrintPage(unsigned char* pData, int x, int y);
	virtual int			LoadPaper(void);			// Called when session is opened
	virtual	int			NewPage(void);				// Creates a new page
	virtual int			CancelJob(void);			// Cancels printing
	virtual int			Print(void);				// Send pages to the printer

protected:
	void				WriteHeader();				// Writes a Postscript header to the file
	void				WritePageHeader();			// Writes a Postscript page header
	void				WriteTrailer();				// Writes a Postscript trailer
	int					WriteFChars(int& cnt);		// Writes reserved 'f' chars to the file
	int					WriteHChars(int& cnt);		// Writes reserved 'h' chars to the file
	int					m_pageNum;					// Active page number
	FILE*				m_pFd;						// The output file handle
	MString				m_filename;					// The output filename
	MString				m_dir;						// The directory for storing PS files
};

#endif

