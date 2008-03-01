/* chargen.h */

/* $Id: chargen.h,v 1.1 2008/02/17 13:25:27 kpettit1 Exp $ */

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


#ifndef _CHARGEN_H_
#define _CHARGEN_H_

void cb_CreateNewCharGen(Fl_Widget* w, void *);

#define	CHARGEN_FORMAT_BINARY	0
#define CHARGEN_FORMAT_C		1
#define CHARGEN_FORMAT_BASIC	2

/*
==========================================================================
Define the CharTable class
==========================================================================
*/
class VTCharTable : public Fl_Box
{
public:
	VTCharTable(int x, int y, int w, int h, const char* title);
	void	GetCharData(int index, unsigned char *data);
	void	PutCharData(int index, unsigned char *data);
	void	SetActiveChar(int index);

private:
	void 	draw(void);								// Draw routine
	void	DrawChar(int index);

	unsigned char	m_Data[256][12];	// Character data
	int				m_ActiveChar;
};

/*
==========================================================================
Define the VTFX80Print class.  This is an implementation of a printer 
that emulates an Epson FX-80 impact printer and send the output to PNG
or Postscript.  It can also write directly to a host printer.
==========================================================================
*/
class VTCharacterGen : public Fl_Double_Window
{
public:
	VTCharacterGen(int w, int h, const char* title);

	Fl_Window*		m_pWin;

	virtual int		handle(int event);
	void			HandlePixelEdit(int xp, int yp);		// Handle Pixel region edits
	void			HandleCharTableEdit(int xp, int yp);	// Handle CharTable edits
	void			ClearPixmap(void);						// Clear the current pixmap
	void			CopyPixmap(void);						// Copy the current pixmap
	void			PastePixmap(void);						// Paste the current pixmap
	void			Load(void);								// Loads data from file
	void			Save(int format);						// Saves data using specified format
	int				CheckForSave(void);
	void			SaveActiveChar(void);					// Save active pixmap to chartable

private:
	Fl_Box*			m_pDots[11][9];
	Fl_Box*			m_pBoxes[11][9];
	VTCharTable*	m_pCharTable;
	bool			m_Dots[11][9];
	bool			m_Buffer[11][9];
	int				m_PasteValid;
	int				m_ActiveChar;
	int				m_Modified;
	void			ChangeActiveChar(int index);
	void			UpdatePreviews(void);
	void			UpdatePicaView(void);
	void			UpdateExpandView(void);
	void			UpdateEnhanceView(void);
	void			UpdateDblStrikeView(void);
	void			UpdateDblEnhanceView(void);

	// Buttons
	Fl_Button*		m_pClose;						// Close the window
	Fl_Box*			m_pCharText;
	Fl_Box*			m_pExPica;						// Example of Pica
	Fl_Box*			m_pExExpand;					// Example of Expanded
	Fl_Box*			m_pExEnhance;					// Example of Enhanced
	Fl_Box*			m_pExDblStrike;					// Example of Double Strike
	Fl_Box*			m_pExDblEnhance;				// Example of Double Strike Enhanced
	char			m_CharText[6];

	char			m_PicaPixelData[3*18];			// Data for Pica preview
	char			m_ExpandPixelData[3*18];			// Data for Pica preview
	char			m_EnhancePixelData[3*18];			// Data for Pica preview
	char			m_DblStrikePixelData[3*18];			// Data for Pica preview
	char			m_DblEnhancePixelData[3*18];			// Data for Pica preview
};

#endif

