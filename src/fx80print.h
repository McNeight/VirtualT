/* fx80print.h */

/* $Id: fx80print.h,v 1.1 2008/02/17 13:25:27 kpettit1 Exp $ */

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


#ifndef _FX80PRINT_H_
#define _FX80PRINT_H_

#include "printer.h"

#define	FX80_PAGE_WIDTH		8.0
#define	FX80_PAGE_HEIGHT	10.5
#define	FX80_VERT_DPI		144
#define FX80_HORIZ_DPI		240

#define	ASCII_CR			0x0D
#define	ASCII_LF			0x0A
#define	ASCII_FF			0x0C

/*
==========================================================================
Define the VTFX80Print class.  This is an implementation of a printer 
that emulates an Epson FX-80 impact printer and send the output to PNG
or Postscript.  It can also write directly to a host printer.
==========================================================================
*/
class VTFX80Print : public VTPrinter
{
public:
	VTFX80Print();
	~VTFX80Print();

	virtual MString		GetName();					// Get name of the printer
	virtual void		BuildPropertyDialog();		// Build dialog for FX80
	virtual int			GetProperties(void);		// Get dialog properties and save
	virtual int			GetBusyStatus(void);
	virtual void		SendAutoFF(void);			// Send FormFeed if needed
	virtual void		Deinit(void);				// Deinit routine
	virtual int			CancelPrintJob(void);		// Cancels the current print job

	void				CharRomBrowse(Fl_Widget* w);// Call back for ROM file browsing
	void				UseRomCheck(Fl_Widget* w);	// Call back for use ROM checkbox

protected:
	virtual void		PrintByte(unsigned char byte);	// Send byte to FX80 emulation
	virtual void		Init(void);					// Init routine
	virtual int			OpenSession(void);			// Open a new print session
	virtual int			CloseSession(void);			// Closes the active print session

	// Define variables for saving preferences
	Fl_Input*			m_pRomFile;					// File to load CharROM from
	Fl_Input*			m_pRamFile;					// File to initialize CharRAM from
	Fl_Check_Button*	m_pUseRomFile;				// Checkbox to use ROM initialization
	Fl_Check_Button*	m_pUseRamFile;				// Checkbox to use RAM initialization
	Fl_Button*			m_pRomBrowse;				// Browse button for ROM file
	Fl_Button*			m_pRamBrowse;				// Browse button for RAM file
	Fl_Check_Button*	m_pVirtualPaper;			// Checkbox to use Virtual Paper

	char				m_romFileStr[256];
	char				m_ramFileStr[256];
	int					m_useRomFile;				// TRUE if ROM should be initialized
	int					m_useRamFile;				// TRUE if RAM should be initialized
	int					m_virtualPaper;				// TRUE if RAM should be initialized

	MString				m_sRomFile;					// File to load Char ROM
	MString				m_sRamFile;					// File to load Char RAM

	// Define functions used for handling the FX-80 protocol
	virtual void		ResetPrinter(void);			// Resets the FX-80 "printer" settings
	virtual void		ResetMode(void);			// Resets the FX-80 "printer" settings
	virtual void		RenderChar(unsigned char ch);// Render character to page
	void				TrimForProportional(unsigned char *pData, int& first, int& last);
	void				PlotPixel(int xpos, int ypos);	// Plot specified pixel
	int					ProcessAsCmd(unsigned char byte);
	int					ProcessDirectControl(unsigned char byte);
	void				NewPage(void);				// Start a new page
	unsigned char		MapIntlChar(unsigned char byte);

	// Define variables used for controling the ESCP/2 printing
protected:
	double				m_width;					// Width of printable region
	double				m_height;					// Height of printable region
	int					m_curX;						// Current pin X location (dots)
	int					m_curY;						// Current pin Y loacation (top dot)
	int					m_escSeen;					// True if processing ESC sequence
	int					m_escCmd;					// Current ESC command being processed
	int					m_escParamsNeeded;			// Number of ESC parameters needed
	double				m_topMargin;				// Top margin, in inches
	double				m_leftMargin;				// Left margin, in inches
	double				m_rightMargin;				// Right margin, in inches
	double				m_bottomMargin;				// Bottom margin, in inches
	int					m_bottomMarginY;			// Bottom margin in dots
	double				m_cpi;						// Characters per inch setting
	double				m_lineSpacing;				// Current line spacing setting
	char				m_graphicsMode;				// Indicates if graphics mode is on
	char				m_fontSource;				// Selects "ROM" or "RAM" character font
	char				m_compressed;				// Indicates if compressed mode is on
	char				m_elite;					// Indicates if elite mode is on
	char				m_proportional;				// Indicates if proportional mode is on
	char				m_enhanced;					// indicates if enhanced mode is on
	char				m_dblStrike;				// Indicates if double strike mode is on
	char				m_expanded;					// Indicates if expanded mode is on
	char				m_expandedOneLine;			// Indicates if expanded mode is on
	char				m_italic;					// Indicates if italic mode is on
	char				m_subscript;				// Indicates if subscript mode is on
	char				m_superscript;				// Indicates if superscript mode is on
	char				m_underline;				// Indicates if underline mode is on
	int					m_vertDots;					// Number of vertical "dots"
	int					m_horizDots;				// Number of horizontal "dots"
	int					m_vertDpi;					// Vertical DPI
	int					m_horizDpi;					// Horizontal DPI
	int					m_bytesPerLine;				// Number of bytes per line on page
	int					m_intlSelect;				// Internatinal character select
	
	unsigned char		m_charRom[256][12];			// Character ROM
	unsigned char		m_charRam[256][12];			// Character RAM

	unsigned char*		m_pPage;					// Pointer to "Page" memory

public:
	Fl_Window*			m_pOut;
};

#endif

