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
#include "autofile.h"
#include "MStringArray.h"

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
	virtual void		Deinit(void) = 0;				// Deinitializes & frees memory
	virtual void		GetPrefs(void) = 0;				// Get prefs from controls & save
	virtual void		BuildControls(void) = 0;		// Build paper specific controls
	virtual void		HideControls(void) = 0;			// Hide paper specific controls
	virtual void		ShowControls(void) = 0;			// Show paper specific controls

	// Sets the top of form in inches
	virtual void		SetTopOfForm(double tof) {m_tof=tof;}
	virtual void		SetFormHeight(double height) {m_formHeight = height;}

	// PrintPage sends data to the active page, it does not send data to a printer
	virtual int			PrintPage(unsigned char* pData, int x, int y) = 0;
	virtual int			LoadPaper(void) = 0;			// Called when session is opened
	virtual	int			NewPage(void) = 0;				// Creates a new page
	virtual int			CancelJob(void) = 0;			// Cancels printing
	virtual int			Print(void) = 0;				// Send pages to the printer

	virtual int			GetErrorCount(void) { return m_errors.GetSize(); }
	const char *		GetError(int c) { if (c < m_errors.GetSize()) return
												(const char *) m_errors[c]; else return NULL; }
	virtual void		ClearErrors(void) { m_errors.RemoveAll(); }

protected:
	// Adds an error to the list of errors associated with the print
	void 				AddError(const char *pStr) { m_errors.Add(pStr); }
	Fl_Preferences*		m_pPref;						// Pointer to preferences
	double				m_tof;							// Top of form (inches)
	double				m_formHeight;					// Form height (inches)
	MStringArray		m_errors;						// Array of Errors

};

class VTVirtualPage : public VTObject
{
public:
	VTVirtualPage() { m_pPage = NULL; m_pageHeight = 0.0; }
	~VTVirtualPage() { if (m_pPage != NULL) delete m_pPage; }
	unsigned char *	m_pPage;
	double			m_pageHeight;
	double			m_topOffset;
	double			m_tof;
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
	virtual void		Deinit(void);				// Deinitializes & frees memory
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
	virtual void		Deinit(void);				// Deinitializes & frees memory
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
	void				MakeDot(char dotChar);		// Puts a dot on the page
	int					WriteDChars();				// Writes reserved 'd' chars to the file
	int					WriteFChars();				// Writes reserved 'f' chars to the file
	void				WriteDots();				// Writes reserved dots to the file
	int					GetFilename(void);			// Get new filename for print
	int					m_pageNum;					// Active page number
	FILE*				m_pFd;						// The output file handle
	MString				m_filename;					// The output filename
	MString				m_dir;						// The directory for storing PS files
	int					m_fCnt, m_dCnt;
	MString				m_dots;
	int					m_len;

	// Define Perferences below
	Fl_Check_Button*	m_pPrompt;					// Prompt for Filename checkbox
	Fl_Check_Button*	m_pAutoFilename;			// Checkbox to create auto filenames
	Fl_Input*			m_pFileFormat;				// Edit field for filename format
	Fl_Box*				m_pFormatText;	
	Fl_Slider*			m_pDarkness;				// Controls size of dots
	Fl_Box*				m_pLight;	
	Fl_Box*				m_pDark;	

	int					m_prompt;					// Flag to prompt for filename
	int					m_autoFilename;				// Flag to generate auto filename
	int					m_darkness;					// Setting for the ink darkness
	MString				m_fileFormat;				// String indicating file format
	VTAutoFile			m_autoFile;					// Generates auto filenames
};

/*
===========================================================
Define Linux lpr via Postscript paper.  This uses the 
Postscript paper to generate a PS file, then spawns a print
job via an lpr (user defineable) process.
===========================================================
*/
class VTlprPaper : public VTPSPaper
{
public:
	VTlprPaper(Fl_Preferences* pPref); 		// Class constructor

	virtual MString		GetName(void);				// Name of the paper
	virtual void		Init(void);					// Initializes with perferences
	virtual void		GetPrefs(void);				// Get prefs from controls & save
	virtual void		BuildControls(void);		// Build paper specific controls
	virtual void		HideControls(void);			// Hide paper specific controls
	virtual void		ShowControls(void);			// Show paper specific controls

	// Override the Print funtion so we can spawn the job
	virtual int			Print(void);				// Send pages to the printer
	MString				m_cmdLine;
	Fl_Input*			m_pCmdLine;					// Control or editing command line
};

#endif

