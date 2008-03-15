/* fileprint.cpp */

/* $Id: fileprint.cpp,v 1.1 2008/02/17 13:25:27 kpettit1 Exp $ */

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
#include <FL/Fl_Check_Button.H>
#include <FL/Fl_Input.H>
#include <FL/Fl_Choice.H>
#include <FL/Fl_Box.H>
#include <FL/Fl_Preferences.H>
#include <FL/Fl_File_Chooser.H>
#include <FL/fl_ask.H>

#include <string.h>
#include <stdio.h>
#include <stdlib.h>

#include "VirtualT.h"
#include "fileprint.h"
#include "m100emu.h"

extern "C" struct tm* mytime;
extern "C" char path[255];
/*
================================================================================
VTFilePrint:	This is the class constructor for the FilePrint Printer.
================================================================================
*/
VTFilePrint::VTFilePrint(void)
{
	m_Prompt = 0;
	m_SeparateFiles = 0;
	m_DirName[0] = 0;
	m_FormatCode[0] = 0;
	m_FilterCodes = 0;
	m_AutoFormat = 0;
	m_OutFd = NULL;
	m_PageNum = 0;
	m_ActiveSeqNum = -1;
}

/*
=======================================================
GetName:	Returns the name of the printer.
=======================================================
*/
MString VTFilePrint::GetName()
{
	return "File Output";
}

/*
=======================================================
Closes the current file and opens a new page.
=======================================================
*/
void VTFilePrint::OpenNextPage()
{
	MString filename;
	char		str[512];

	// Check if file is open
	if (m_OutFd != NULL)
	{
		fclose(m_OutFd);
		m_OutFd = NULL;
	}

	// Check if we are generating filename automatically
	if (m_AutoFormat)
	{
		while (1)
		{
			// Increment page number
			m_PageNum++;

			// Generate auto filename
			filename = GenFilename();

			// Try to open the filename for read to ensure it doens't exist
			FILE* fd = fopen((const char *) filename, "r");
			if (fd == NULL)
				break;

			// Close the file
			fclose(fd);
		}

		// Open the file
		m_Filename = filename;
		m_OutFd = fopen((const char *) filename, "wb+");
		if (m_OutFd == NULL)
		{
			sprintf(str, "Unable to open file %s", (const char *) filename);
			AddError(str);
		}

		return;
	}

	if (m_Prompt)
	{
		// Increment page number
		m_PageNum++;

		// Prompt for a new filename
		if (PromptFilename(m_Filename) == PRINT_ERROR_NONE)
		{
			m_OutFd = fopen((const char *) m_Filename, "wb+");
		}
	}
}

/*
=======================================================
Print a byte to the file.
=======================================================
*/
void VTFilePrint::PrintByte(unsigned char byte)
{
	// Save the last byte printed to test for autoFF
	m_LastBytePrinted = byte;

	// Test if pages are being separated into separate files
	if (m_SeparateFiles && (byte == 0x0C))
	{
		// Close current file and open next page
		OpenNextPage();

		// Return (we don't actually write the 0x0c
		return;
	}

	// Test if Control Codes are being filtered
	if (m_FilterCodes)
	{
		if (((byte < ' ') || (byte > '~')) && (byte != 0x0D) && 
			(byte != 0x0A))
		{
			return;
		}
	}

	// Print byte to the file
	if (m_OutFd != NULL)
		fwrite(&byte, 1, 1, m_OutFd);
}

/*
=======================================================
Callback for SeparateFiles checkbox.  This changes the
text of the auto-format string to add / remove the %p.
=======================================================
*/
void cb_SeparateFiles(Fl_Widget*w, void* ptr)
{
	if (ptr != NULL)
		((VTFilePrint*) ptr)->UpdateFormatForPage();
}

/*
=======================================================
Does the actual work of modifying the format string
with %p
=======================================================
*/
void VTFilePrint::UpdateFormatForPage(void)
{
	int		separate;
	char	format[256];
	char	store[256];
	int		len, c;
	int		found;

	// Get checkbox satus
	separate = m_pSeparateFiles->value();

	// Get format string and check length
	strcpy(format, m_pFormatCode->value());
	if ((len = strlen(format)) == 0)
		return;

	// Find the location of "%s"
	found = FALSE;
	for (c = 0; c < len; c++)
	{
		if (format[c] == '%')
			if (format[c+1] == 'p')
			{
				found = TRUE;
				break;
			}
	}

	// If the specifier is already in the string & it needs to be
	// Or it isn't and doesn't, then return
	if (!(found ^ separate))
		return;

	// Add or remove the %p
	if (separate)
	{
		// Need to add specifier.  Find a '.' if any
		for (c = len-1; c >0; c--)
		{
			if ((format[c] == '.') || (format[c] == '/') || (format[c] == '\\'))
				break;
		}
		// Check if a '.' was found
		store[0] = 0;
		if (c > 0)
		{
			if (format[c] == '.')
			{
				strcpy(store, &format[c]);
				format[c] = 0;
			}
		}

		// Append the page specifier
		strcat(format, "_pg%p");
		strcat(format, store);

		// Update the Input field
		m_pFormatCode->value((const char *) format);
	}
	else
	{
		// Need to remove specifier at c
		strcpy(store, &format[c+2]);
		if (strncmp("pg", &format[c-2], 2) == 0)
			format[c-2] = 0;
		else if (strncmp("page", &format[c-4], 4) == 0)
			format[c-4] = 0;
		else if (strncmp("_p", &format[c-2], 2) == 0)
			format[c-2] = 0;
		else
			format[c] = 0;

		// Check for a trailing '_'
		if (format[strlen(format)-1] == '_')
			format[strlen(format)-1] = 0;

		// Append anything we removed
		strcat(format, store);

		// Update the Input field
		m_pFormatCode->value((const char *) format);
	}
}

/*
=======================================================
Build the Property Dialog for the FilePrint Printer
=======================================================
*/
void VTFilePrint::BuildPropertyDialog(Fl_Window* pParent)
{
	// Create controls for File Output emulaiton mode
	Fl_Box* o = new Fl_Box(20, 20, 360, 20, "File Output Printer");

	// File Directory Name
	o = new Fl_Box(20, 50, 300, 20, "File Output Directory");
	o->align(FL_ALIGN_LEFT | FL_ALIGN_INSIDE);
	m_pDirName = new Fl_Input(50, 75, 300, 20, "");
	m_pDirName->value(m_DirName);

	// Prompt for filename
	m_pPrompt = new Fl_Check_Button(20, 110, 200, 20, "Prompt for filename");
	m_pPrompt->value(m_Prompt);

	// Pages in separate files
	m_pSeparateFiles = new Fl_Check_Button(20, 135, 200, 20, "Print pages to separate files");
	m_pSeparateFiles->value(m_SeparateFiles);
	m_pSeparateFiles->callback(cb_SeparateFiles, this);

	// Auto formatted filenames
	m_pAutoFormat = new Fl_Check_Button(20, 160, 200, 20, "Automatically generate filenames");
	m_pAutoFormat->value(m_AutoFormat);

	// Filename format string
	o = new Fl_Box(50, 185, 50, 20, "Format");
	o->align(FL_ALIGN_LEFT | FL_ALIGN_INSIDE);
	m_pFormatCode = new Fl_Input(110, 185, 180, 20, "");
	m_pFormatCode->value(m_FormatCode);
	o = new Fl_Box(110, 210, 50, 20, "%d=Date  %s=Seq#  %p=Page#");
	o->align(FL_ALIGN_LEFT | FL_ALIGN_INSIDE);

	// Control code filtering
	m_pFilterCodes = new Fl_Check_Button(20, 235, 200, 20, "Filter control codes");
	m_pFilterCodes->value(m_FilterCodes);

}

/*
=======================================================
Get the Busy status
=======================================================
*/
int VTFilePrint::GetBusyStatus(void)
{
	return FALSE;
}

/*
=======================================================
Init routine to read preferences, setup files, etc.
=======================================================
*/
void VTFilePrint::Init(void)
{
	char	modelDir[256];

	strcpy(modelDir, path);
	strcat(modelDir, "print");

	// Load specific printer preferences
	m_pPref->get("FilePrint_DirName", m_DirName, modelDir, 256);
	m_pPref->get("FilePrint_Prompt", m_Prompt, 1);
	m_pPref->get("FilePrint_SeparateFiles", m_SeparateFiles, 0);
	m_pPref->get("FilePrint_FilterCodes", m_FilterCodes, 0);
	m_pPref->get("FilePrint_AutoFormat", m_AutoFormat, 0);
	m_pPref->get("FilePrint_FormatString", m_FormatCode, "lpt_out_%d_%s.txt", 256);
	return;
}

/*
=======================================================
Generates an autoformat filename based on preferences.
=======================================================
*/
MString VTFilePrint::GenFilename()
{
	MString		filename;
	MString		beforeSeq;
	MString		afterSeq;
	int			len, c;
	int			seq = 0;
	int			seqFound = FALSE;
	char		page[10];
	FILE*		fd;

	// Build the filename by parsing the format & looking for % modifiers
	len = strlen(m_FormatCode);
	for (c = 0; c < len; c++)
	{
		if (m_FormatCode[c] == '%')
		{
			if (m_FormatCode[c+1] == 'd')
			{
				// Add date code
				page[0] = (mytime->tm_year % 100) / 10 + '0';
				page[1] = (mytime->tm_year % 10) + '0';
				page[2] = (mytime->tm_mon+1) / 10 + '0';
				page[3] = (mytime->tm_mon+1) % 10 + '0';
				page[4] = (mytime->tm_mday / 10) + '0';
				page[5] = (mytime->tm_mday % 10) + '0';
				page[6] = 0;
	
				// Add date to filename
				if (seqFound)
					afterSeq += page;
				else
					beforeSeq += page;
			}
			else if (m_FormatCode[c+1] == 's')
			{
				seqFound = TRUE;
			}
			else if (m_FormatCode[c+1] == 'p')
			{
				// Add page number
				sprintf(page, "%d", m_PageNum);
				if (seqFound)
					afterSeq += page;
				else
					filename += page;
			}

			// Skip the format code
			c++;
		}
		else
		{
			if (seqFound)
				afterSeq += m_FormatCode[c];
			else
				beforeSeq += m_FormatCode[c];
		}
	}

	// Okay, the format string is parsed.  If it contains a seq number,
	// then find a seq that doesn't exist yet, if not already found.
	if (seqFound)
	{
		// Test if we already have a seq number for this session
		if (m_ActiveSeqNum == -1)
		{
			// Find a seq number that represents a file that doens't exist
			for (seq = 0; ;seq++)
			{
				// Build filename for testing
				sprintf(page, "%d", seq);
				filename = m_DirName;
				filename += beforeSeq + page + afterSeq;

				fd = fopen((const char *) filename, "r");
				if (fd == NULL)
				{
					break;
				}
				// Close the file
				fclose(fd);
			}

			// Okay, now save the sequence number
			m_ActiveSeqNum = seq;
		}

		// Build the filename
		sprintf(page, "%d", m_ActiveSeqNum);
		filename = m_DirName + beforeSeq + page + afterSeq;
	}
	else
	{
		filename = beforeSeq;
	}

	return filename;
}

/*
=======================================================
Prompts the user for a filename using the a default 
name or autogenerated name.
=======================================================
*/
int VTFilePrint::PromptFilename(MString& filename)
{
	Fl_File_Chooser*	fc;
	char				text[80];

	while (1)
	{
		if (m_PageNum == 1)
			strcpy(text, "Selet Output File for Print");
		else
			sprintf(text, "Select File for Page %d", m_PageNum);

		// Prompt the user with the filename & give a chance to change
		fc = new Fl_File_Chooser((const char *) filename, "*.*", 
			Fl_File_Chooser::CREATE, text);

		fc->show();
		fc->value((const char *) filename);
		while (fc->visible())
			Fl::wait();

		if (fc->value() == NULL)
			return PRINT_ERROR_ABORTED;
			
		// Check if filename was selected
		if (strlen(fc->value()) == 0)
			return PRINT_ERROR_ABORTED;

		// Check if file already exists and ask to overwrite or not
	    // Try to open the ouput file
	    FILE* fd = fopen(fc->value(), "rb+");
	    if (fd != NULL)
	    {
	        // File already exists! Check for overwrite!
	        fclose(fd);

			int c = fl_choice("Overwrite existing file?", "Cancel", "Yes", "No");
			if (c == 2)
				continue;
			if (c == 0)
			{
				delete fc;
				return PRINT_ERROR_ABORTED;
			}
		}
		break;
	}

	filename = fc->value();
	delete fc;
	
	return PRINT_ERROR_NONE;
}

/*
=======================================================
Opens a new Print Session
=======================================================
*/
int VTFilePrint::OpenSession(void)
{
	MString		filename;
	int			ret;
	char		str[512];

	// Reset page number
	m_PageNum = 1;

	// Okay, we need to open a Print Session.  First see if we have
	// to auto generate a filename
	if (m_AutoFormat)
	{
		// Generate a filename based on format code & dir
		filename = GenFilename();
	}
	else
	{
		filename = m_DirName;
		filename += "lpt_out.txt";
	}

	// Check if Prompt is enabled
	if (m_Prompt)
	{
		ret = PromptFilename(filename);
		if (ret == PRINT_ERROR_ABORTED)
			return ret;
	}

	// Try to open the file
	if ((m_OutFd = fopen((const char *) filename, "wb+")) == NULL)
	{
		sprintf(str, "Unable to open file %s", (const char *) filename);
		m_errors.Add(str);
		return PRINT_ERROR_IO_ERROR;
	}

	// Save the filename
	m_Filename = filename;

	return PRINT_ERROR_NONE;
}

/*
=======================================================
Closes the active Print Session
=======================================================
*/
int VTFilePrint::CloseSession(void)
{
	// Test if there is an open file
	if (m_OutFd != NULL)
	{
		fclose(m_OutFd);
		m_OutFd = NULL;
		m_ActiveSeqNum = -1;
	}

	return PRINT_ERROR_NONE;
}

/*
=======================================================
Get the Printer Perferences from Dialog and save
=======================================================
*/
int VTFilePrint::GetProperties(void)
{
	int		len;
	char	format[256];

	// First validate the separate page function
	if (m_pSeparateFiles->value())
	{
		// Get the format string to test for %p
		strcpy(format, m_pFormatCode->value());
		if (strstr(format, "%p") == NULL)
		{
			fl_message("To print pages to separate files, you must use the page format specifier (%%p)");
			return 1;
		}
	}

	// Read the DirName setting
	strcpy(m_DirName, m_pDirName->value());
	
	// Ensure dirname ends with slash
	len = strlen(m_DirName);
	if (len != 0)
	{
		if ((m_DirName[len-1] != '\\') && (m_DirName[len-1] != '/'))
#ifdef WIN32
			strcat(m_DirName, "\\");
#else
			strcat(m_DirName, "/");
#endif
	}

	// Get Prompt setting
	m_Prompt = m_pPrompt->value();
	m_SeparateFiles = m_pSeparateFiles->value();
	m_FilterCodes = m_pFilterCodes->value();
	m_AutoFormat = m_pAutoFormat->value();
	strcpy(m_FormatCode, m_pFormatCode->value());

	// Now save to preferences
	m_pPref->set("FilePrint_DirName", m_DirName);
	m_pPref->set("FilePrint_Prompt", m_Prompt);
	m_pPref->set("FilePrint_SeparateFiles", m_SeparateFiles);
	m_pPref->set("FilePrint_FilterCodes", m_FilterCodes);
	m_pPref->set("FilePrint_AutoFormat", m_AutoFormat);
	m_pPref->set("FilePrint_FormatString", m_FormatCode);

	return PRINT_ERROR_NONE;
}

/*
=======================================================
Sends a FormFeed to the file if the last character
wasn't a FormFeed.
=======================================================
*/
void VTFilePrint::SendAutoFF(void)
{
	if (m_LastBytePrinted != 0x0C)
		PrintByte(0x0C);

	return;
}

/*
=======================================================
Deinitialized the printer.
=======================================================
*/
void VTFilePrint::Deinit(void)
{
	// Close any open files
	if (m_OutFd != NULL)
	{
		fclose(m_OutFd);
		m_OutFd = NULL;
	}
	return;
}

/*
=======================================================
Cancel the print job & delete the file
=======================================================
*/
int VTFilePrint::CancelPrintJob(void)
{
	// Ensure there is an active session first
	if (!m_SessionActive)
		return PRINT_ERROR_NONE;

	// Close the file
	if (m_OutFd != NULL)
		fclose(m_OutFd);

	m_OutFd = NULL;

	// Remove the file
	std::remove((const char *) m_Filename);
	
	return PRINT_ERROR_NONE;
}

/*
=======================================================
Build the monitor tab
=======================================================
*/
void VTFilePrint::BuildMonTab(void)
{
}

