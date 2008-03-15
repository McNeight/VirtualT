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
#define	FX80_PAGE_HEIGHT	11
#define	FX80_VERT_DPI		216
#define FX80_HORIZ_DPI		240

#define	ASCII_CR			0x0D
#define	ASCII_LF			0x0A
#define	ASCII_FF			0x0C

#define	FX80_8PIN_MODE		1
#define	FX80_9PIN_MODE		2

class	VTPaper;			// The printer prints on the "Paper" class

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
	virtual void		BuildPropertyDialog(Fl_Window* pParent);		// Build dialog for FX80
	virtual int			GetProperties(void);		// Get dialog properties and save
	virtual int			GetBusyStatus(void);
	virtual void		SendAutoFF(void);			// Send FormFeed if needed
	virtual void		Deinit(void);				// Deinit routine
	virtual int			CancelPrintJob(void);		// Cancels the current print job
	virtual int			GetErrorCount(void);		// Get count of print errors
	virtual void		BuildMonTab(void);			// Builds the monitor tab

	int					IntlDipChanged(void);		// Called when a change made to DIP setting
	void				CharRomBrowse(Fl_Widget* w);// Call back for ROM file browsing
	void				UseRomCheck(Fl_Widget* w);	// Call back for use ROM checkbox

	virtual void		ResetPrinter(void);			// Resets the FX-80 "printer" settings
	void				PaperSelect(void);
	void				SkipPerfSwitch(void);

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
	Fl_Choice*			m_pPaperChoice;				// Control to select the paper source
	Fl_Round_Button*	m_pIntlChar4off;			// Bit 4 of Intl Char Set DIP
	Fl_Round_Button*	m_pIntlChar4on;				// Bit 4 of Intl Char Set DIP
	Fl_Round_Button*	m_pIntlChar2off;			// Bit 2 of Intl Char Set DIP
	Fl_Round_Button*	m_pIntlChar2on;				// Bit 2 of Intl Char Set DIP
	Fl_Round_Button*	m_pIntlChar1off;			// Bit 1 of Intl Char Set DIP
	Fl_Round_Button*	m_pIntlChar1on;				// Bit 1 of Intl Char Set DIP
	Fl_Box*				m_pIntlCharText;			// Text for International Char
	Fl_Round_Button*	m_pPrintWeightOn;			// Print Weight switch
	Fl_Round_Button*	m_pPrintWeightOff;			// Print Weight switch
	Fl_Round_Button*	m_pZeroSlashOn;				// Zero slashed switch
	Fl_Round_Button*	m_pZeroSlashOff;			// Zero slashed switch
	Fl_Round_Button*	m_pPitchOn;					// Pitch switch
	Fl_Round_Button*	m_pPitchOff;				// Pitch switch
	Fl_Round_Button*	m_pAutoCROn;				// AutoCR switch
	Fl_Round_Button*	m_pAutoCROff;				// AutoCR switch
	Fl_Round_Button*	m_pSkipPerfOn;				// Skip Perforation switch
	Fl_Round_Button*	m_pSkipPerfOff;				// Skip Perforation switch
	Fl_Input*			m_pTopOfForm;				// Top of Form location in inches

	void				SetIntlDipText(char set);	// Set Dialog box text based on code

	char				m_romFileStr[256];
	char				m_ramFileStr[256];
	int					m_useRomFile;				// TRUE if ROM should be initialized
	int					m_useRamFile;				// TRUE if RAM should be initialized

	MString				m_sRomFile;					// File to load Char ROM
	MString				m_sRamFile;					// File to load Char RAM
	MString				m_paperName;				// Name of seleted paper
	int					m_selectedPaper;

	// Define functions used for handling the FX-80 protocol
	virtual void		ResetMode(void);			// Resets the FX-80 "printer" settings
	virtual void		RenderChar(unsigned char ch);// Render character to page
	virtual void		RenderGraphic(unsigned char ch);// Render graphics to page
	void				TrimForProportional(unsigned char *pData, int& first, int& last);
	void				PlotPixel(int xpos, int ypos);	// Plot specified pixel
	int					ProcessAsCmd(unsigned char byte);
	int					ProcessDirectControl(unsigned char byte);
	void				NewPage(void);				// Start a new page
	int					CreatePaper(void);			// Create a paper object
	unsigned char		MapIntlChar(unsigned char byte);
	void				SetLeftMargin(unsigned char byte);
	void				SetRightMargin(unsigned char byte);
	int					SetVertChannelTabs(unsigned char byte);
	int					MasterSelect(unsigned char byte);
	int					DefineUserChar(unsigned char byte);
	int					GraphicsCommand(unsigned char byte);
	int					VarGraphicsCommand(unsigned char byte);
	int					ReassignGraphicsCmd(unsigned char byte);
	void				ResetTabStops(void);		// Reset to factory default tabs
	void				ResetVertTabs(void);		// Reset to factory default Vertical tabs
	int					FormHeightCmd(unsigned char byte);
	void				SetNewFormHeight(double newHeight);
	void				ProcessLineFeed(void);

	// Define variables used for controling the ESCP/2 printing
protected:
	double				m_width;					// Width of printable region
	int					m_curX;						// Current pin X location (dots)
	int					m_curY;						// Current pin Y loacation (top dot)
	char				m_escSeen;					// True if processing ESC sequence
	int					m_escCmd;					// Current ESC command being processed
	int					m_escParamsRcvd;			// Number of ESC parameters received
	char				m_escTabChannel;			// Actively updated Tab Channel
	double				m_leftMargin;				// Left margin, in inches
	int					m_leftMarginX;				// Left margin, in dots
	double				m_rightMargin;				// Right margin, in inches
	int					m_rightMarginX;				// Right margin, in dots
	double				m_formHeight;				// Height of Form (11 inch default)
	double				m_allocHeight;				// Height of Form allocation
	double				m_bottomMargin;				// Bottom margin, in inches
	int					m_bottomMarginY;			// Bottom margin in dots
	double				m_cpi;						// Characters per inch setting
	double				m_lineSpacing;				// Current line spacing setting
	char				m_fontSource;				// Selects "ROM" or "RAM" character font
	char				m_marksMade;				// Indicates if any marks made during session
	char				m_compressed;				// Indicates if compressed mode is on
	char				m_elite;					// Indicates if elite mode is on
	char				m_proportional;				// Indicates if proportional mode is on
	char				m_enhanced;					// indicates if enhanced mode is on
	char				m_dblStrike;				// Indicates if double strike mode is on
	char				m_expanded;					// Indicates if expanded mode is on
	char				m_expandedOneLine;			// Indicates if expanded mode is on
	char				m_highCodesPrinted;			// Indicates if high codes are printed
	char				m_lowCodesPrinted;			// Indicates if low codes are printed
	char				m_italic;					// Indicates if italic mode is on
	char				m_subscript;				// Indicates if subscript mode is on
	char				m_superscript;				// Indicates if superscript mode is on
	char				m_underline;				// Indicates if underline mode is on
	char				m_skipPerf;					// Indicates if skip Perforation is on
	char				m_formsInchMode;			// Determines the mode form height being set
	char				m_highOrderBitMode;			// Determines if High-Order bit processing on
	char				m_zeroRedefined;			// Indicates if zero redefined 
	int					m_vertDots;					// Number of vertical "dots"
	int					m_horizDots;				// Number of horizontal "dots"
	int					m_vertDpi;					// Vertical DPI
	int					m_horizDpi;					// Horizontal DPI
	int					m_bytesPerLine;				// Number of bytes per line on page
	unsigned char		m_intlSelect;				// Internatinal character select
	int					m_tabs[32];					// Up to 32 tabs supported on FX80
	int					m_vertTabs[8][16];			// Eight channels of 16 tabs
	unsigned char		m_tabChannel;				// Currently selected tab channel
	unsigned char		m_userCharSet;
	unsigned char		m_userFirstChar;
	unsigned char		m_userLastChar;
	int					m_userUpdateChar;
	unsigned char		m_lastChar;

	// Dip switch preferences
	char				m_autoCR;					// Generate CR when LF received?
	char				m_zeroSlashed;				// Indicates if the zero is slashed
	char				m_defCharSet;				// Default international char set
	char				m_defCompressed;			// Default compressed pitch
	char				m_defEnhance;				// Default print weight
	char				m_beep;						// Turns the beeper on / off
	char				m_defSkipPerf;				// Default skip perforation
	double				m_topOfForm;				// Distance to TOF from Page top
	char				m_topOfFormStr[10];			// String version of TOF

	// Graphics mode variables
	char				m_graphicsMode;				// Indicates if graphics mode is on
	int					m_graphicsLength;			// Length of graphics command
	int					m_graphicsRcvd;				// Number of bytes received
	int					m_graphicsStartX;			// Start X location - preserves precision
	double				m_graphicsDpi;				// DPI of graphics mode
	char				m_escKmode;					// Graphics mode for ESC K
	char				m_escLmode;					// Graphics mode for ESC L
	char				m_escYmode;					// Graphics mode for ESC Y
	char				m_escZmode;					// Graphics mode for ESC Z
	char				m_graphicsReassign;			// Graphics command being reassigned
	
	unsigned char		m_charRom[256][12];			// Character ROM
	unsigned char		m_charRam[256][12];			// Character RAM

	unsigned char*		m_pPage;					// Pointer to "Page" memory
	VTPaper*			m_pPaper;					// Pointer to the Paper
	VTObArray			m_papers;					// Array of Papers 

};

#endif

