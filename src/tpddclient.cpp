/* tpddclient.cpp */

/* $Id: tpddclient.cpp,v 1.1 2008/12/23 07:31:22 kpettit1 Exp $ */

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

#define	TPDD_CLIENT_MAIN

#include <sys/types.h>
#include <ctype.h>
#include <stdio.h>
#ifdef _WIN32
#include <windows.h>
#endif

#include <FL/Fl.H>
#include <FL/Fl_Menu_Item.H>
#include <FL/Fl_Menu_Bar.H>
#include <FL/Fl_Help_Dialog.H>
#include <FL/fl_ask.H> 
#include <FL/Fl_Preferences.H>
#include <FL/Fl_Hold_Browser.H>
#include <FL/Fl_Button.H>
#include <FL/Fl_Round_Button.H>
#include <FL/Fl_Check_Button.H>
#include <FL/Fl_Box.H>
#include <FL/Fl_Input.H>
#include <FL/Fl_Hold_Browser.H>
#include <FL/Fl_Text_Display.H>
#include <FL/Fl_Text_Buffer.H>

#include "roms.h"
#include "fl_usage_box.h"
#include "display.h"
#include "cpuregs.h"
#include "disassemble.h"
#include "periph.h"
#include "memedit.h"
#include "file.h"
#include "tpddclient.h"
#include "fileview.h"

// ===============================================
// Extern "C" linkage items
// ===============================================
extern "C"
{
#include "memory.h"
#include "m100emu.h"

extern RomDescription_t *gStdRomDesc;
}

// ===============================================
// Extern and global variables
// ===============================================
extern	Fl_Preferences virtualt_prefs;
extern	Fl_Menu_Item gCpuRegs_menuitems[];
extern	char *gKeywordTable[];
static Fl_Window*		gtcw = NULL;
tpddclient_ctrl_t	gTcCtrl;

void cb_Ide(Fl_Widget* w, void*) ;
void cb_view_refreseh(Fl_Widget*w, void*);

// Menu items for the disassembler
Fl_Menu_Item gTpddClient_menuitems[] = {
 { "&View",         0, 0,        0, FL_SUBMENU },
	{ "Refresh",	0, cb_view_refreseh, 0, 0 },
	{ 0 },
 { "&Tools", 0, 0, 0, FL_SUBMENU },
	{ "CPU Registers",         0, cb_CpuRegs },
	{ "Assembler / IDE",       0, cb_Ide },
	{ "Disassembler",          0, disassembler_cb },
	{ "Memory Editor",         0, cb_MemoryEditor },
	{ "Peripheral Devices",    0, cb_PeripheralDevices },
	{ "Model T File Viewer",   0, cb_FileView },
	{ 0 },
  { 0 }
};

/*
============================================================================
Callback routine for the File Viewer window
============================================================================
*/
void cb_tcwin (Fl_Widget* w, void*)
{
	// Hide the window
	gtcw->hide();

	// Delete the window and set to NULL
	delete gtcw;
	gtcw = NULL;
}

/*
============================================================================
Callback routine when a file is selected
============================================================================
*/
static void display_file(int index)
{
	int					addr1, addr2, c, x, y, lineno, maxline;
	char				line[1000], substr[10], linefmt[6], fill[7];
	unsigned char		ch;
	tpdd_client_files_t	*pfile;

	// First clear the text buffer
//	gTcCtrl.pTb->remove(0, gTcCtrl.pTb->length());
	gTcCtrl.pView->clear();

	// Populate with data based on file type
	pfile = &gTcCtrl.tFiles[index];
	switch (pfile->type)
	{
	case TYPE_BA:
		// Get first BASIC line pointer
		addr1 = pfile->address;

		// Calculate the maximum linenumber length
		while (get_memory16(addr1) != 0)
		{
			addr2 = addr1;
			addr1 = get_memory16(addr1);
		}
		maxline = get_memory16(addr2+2);
		if (maxline > 9999)
		{
			strcpy(linefmt, "%5u ");
			strcpy(fill, "      ");
		}
		else if (maxline > 999)
		{
			strcpy(linefmt, "%4u ");
			strcpy(fill, "     ");
		}
		else if (maxline > 99)
		{
			strcpy(linefmt, "%3u ");
			strcpy(fill, "    ");
		}
		else if (maxline > 9)
		{
			strcpy(linefmt, "%2u ");
			strcpy(fill, "   ");
		}
		else
		{
			strcpy(linefmt, "%1u ");
			strcpy(fill, "  ");
		}

		// Restore pointer to beginning of file
		addr1 = pfile->address;

		// Get BASIC lines until NULL
		while ((addr2 = get_memory16(addr1)) != 0)
		{
			// Detokenize the line and save as ASCII
			addr1 += 2;
			line[0] = 0;

			// Print line number
			lineno = get_memory16(addr1);
			addr1 += 2;
			sprintf(substr, linefmt, lineno);
			strcat(line, substr);

			// Read remaining data & detokenize
			while (addr1 < addr2)
			{
				// Get next byte
				ch = get_memory8(addr1++);
				
				// Check if byte is ':'
				if (ch == ':')
				{
					// Get next character
					ch = get_memory8(addr1);

					// Check for REM tick
					if (ch == 0x8E)
					{
						if (get_memory8(addr1+1) == 0xFF)
						{
							ch = '\'';
							line[strlen(line)+1] = 0;
							line[strlen(line)] = ch;
							if (strlen(line) > 54)
							{
								strcat(line, "\n");
								gTcCtrl.pView->add(line);
//								gTcCtrl.pTb->append(line);
								strcpy(line, fill);
							}
							addr1 += 2;
							continue;
						}
					}

					// Check for ELSE
					if (ch == 0x91)
						continue;

					ch = ':';
					line[strlen(line)+1] = 0;
					line[strlen(line)] = ch;
					if (strlen(line) > 54)
					{
						strcat(line, "\n");
						gTcCtrl.pView->add(line);
//						gTcCtrl.pTb->append(line);
						strcpy(line, fill);
					}
				}
				else if (ch > 0x7F)
				{
					if (strlen(line) + strlen(gKeywordTable[ch & 0x7F]) > 54)
					{
						strcat(line, "\n");
						gTcCtrl.pView->add(line);
//						gTcCtrl.pTb->append(line);
						strcpy(line, fill);
					}
					sprintf(substr, "%s", gKeywordTable[ch & 0x7F]);
					strcat(line, substr);
				}
				else if (ch == 0)
				{
					if (strlen(line) != strlen(fill))
						strcat(line, "\n");
				}
				else
				{
					line[strlen(line)+1] = 0;
					line[strlen(line)] = ch;
					if (strlen(line) > 54)
					{
						strcat(line, "\n");
						gTcCtrl.pView->add(line);
//						gTcCtrl.pTb->append(line);
						strcpy(line, fill);
					}
				}
			}
//			gTcCtrl.pTb->append(line);
			if (strlen(line) != strlen(fill))
				gTcCtrl.pView->add(line);
		}
		break;

	case TYPE_DO:
		// Start at beginning of file
		c = 0;
		while (c < pfile->size)
		{
			// Y is the line input location (we skip CR)
			y = 0;
			// Copy up to 999 bytes into temp line storage
			for (x = 0; x < 999; x++)
			{
				// Test if past end of file
				if (x + c > pfile->size)
					break;

				// Get next byte from file
				line[y] = get_memory8(pfile->address + x + c);
				line[y+1] = 0;

				// Write line if newline received
				if (line[y] == 0x0A)
				{
					line[y+1] = 0;
					gTcCtrl.pView->add(line);
//					gTcCtrl.pTb->append(line);
					y = 0;
					line[0] = 0;
					continue;
				}

				if (strlen(line) > 55)
				{
					line[y+1] = '\n';
					line[y+2] = 0;
//					gTcCtrl.pTb->append(line);
					gTcCtrl.pView->add(line);
					y = 0;
					line[0] = 0;
					continue;
				}

				// Test for end of file
				if (line[y] == 0x1A)
					break;

				// If byte isn't CR, increment input location
				if (line[y] != 0x0D)
					y++;
			}

			// Terminate the string add add to buffer
			line[y] = 0;
//			gTcCtrl.pTb->append(line);
			gTcCtrl.pView->add(line);

			// Add this block to length processed
			c += x;
		}
		break;

	case TYPE_CO:
		x = 0;
		while (x < pfile->size)
		{
			// Start new line
			sprintf(line, "%04X: ", pfile->address + x);

			// Add up to 16 bytes to line
			for (c = 0; c < 16; c++)
			{
				// Test if at end of line
				if (x + c > pfile->size)
					break;

				// Add next byte to line
				sprintf(substr, "%02X ", get_memory8(pfile->address + c + x));
				strcat(line, substr);
			}

			// Add line to text buffer
			strcat(line, "\n");
//			gTcCtrl.pTb->append(line);
			gTcCtrl.pView->add(line);
			x += c;
		}
		break;
	}
}

/*
============================================================================
Callback routine when a file is selected
============================================================================
*/
static void cb_FileSelect(Fl_Widget* w, void*)
{
	int					index;
	tpdd_client_files_t	*pfile;
	
	// Get selection from Hold Browser
	index = ((Fl_Browser*)w)->value()-1;
	if ((index < 0) || (index > gTcCtrl.tCount))
	{
		// Clear the Directry Entry text
		gTcCtrl.sDirEntry[0] = 0;
		gTcCtrl.sAddress[0] = 0;
		gTcCtrl.pAddress->label(gTcCtrl.sAddress);
		gTcCtrl.pDirEntry->label(gTcCtrl.sDirEntry);
//		gTcCtrl.pTb->remove(0, gTcCtrl.pTb->length());
		gTcCtrl.pView->clear();
		return;
	}
}

/*
============================================================================
Get list of files in tFiles structure and update SelectBrowser
============================================================================
*/
static void get_file_list(void)
{
	int				dir_index, addr1, addr2;
	unsigned char	file_type;
	int				c, x;
	char			mt_file[40], fdata;

	// Initialize variables
	dir_index = 0;	 
	gTcCtrl.tCount = 0;
	gTcCtrl.pFileSelect->clear();
	gTcCtrl.selectIndex = -1;

	// Check for Unsved BASIC program
	addr2 = get_memory16(gStdRomDesc->sFilePtrBA);
//	addr2 = get_memory16(addr2);
	if ((get_memory16(addr2) != 0) && (addr2 != 0))
	{
		// Add entry for unsaved BASIC program
		strcpy(gTcCtrl.tFiles[0].name, "Unsaved BA");
		gTcCtrl.tFiles[0].dir_address = gStdRomDesc->sFilePtrBA;
		gTcCtrl.tFiles[0].address = addr2;
		gTcCtrl.tFiles[0].type = TYPE_BA;
		addr1 = get_memory16(addr2);
		if (addr1 == 0)
		{
			gTcCtrl.tFiles[0].size = addr2 - get_memory16(gStdRomDesc->sFilePtrBA);
		}
		else
		{
			while ((get_memory16(addr1) != 0) && (addr1 != 0))
			{
				// Point to next BASIC line
				addr1 = get_memory16(addr1);
			}
			gTcCtrl.tFiles[0].size = addr1 - addr2;
		}

		// Add file to Hold Browser
		sprintf(mt_file, "%-11s %d", gTcCtrl.tFiles[gTcCtrl.tCount].name, 
			gTcCtrl.tFiles[gTcCtrl.tCount].size);
		gTcCtrl.pFileSelect->add(mt_file);

		// Increment file count
		gTcCtrl.tCount++;
	}

	// Now point to the file directory
	addr1 = gStdRomDesc->sDirectory;
	while (dir_index < gStdRomDesc->sDirCount)
	{
		file_type = get_memory8(addr1);
		if ((file_type != TYPE_BA) && (file_type != TYPE_DO) &&
			(file_type != TYPE_CO))
		{
			dir_index++;
			addr1 += 11;
			continue;
		}

		// Get the 1st part of the filename
		c = 0;
		for (x = 0; x < 6; x++)
		{
			if (get_memory8(addr1+3+x) != ' ')
				mt_file[c++] = get_memory8(addr1+3+x);
		}

		// Check for extension and add to mt_file
		if (get_memory8(addr1+3+x) != ' ')
			mt_file[c++] = '.';
		mt_file[c++] = get_memory8(addr1+3+x++);
		mt_file[c++] = get_memory8(addr1+3+x);
		mt_file[c] = '\0';

		// Add filename to tFiles structure
		strcpy(gTcCtrl.tFiles[gTcCtrl.tCount].name, mt_file);
		gTcCtrl.tFiles[gTcCtrl.tCount].type = file_type;

		// Get address of file
		gTcCtrl.tFiles[gTcCtrl.tCount].address = get_memory16(addr1+1);
		gTcCtrl.tFiles[gTcCtrl.tCount].dir_address = addr1;

		// Calculate file size
		if (file_type == TYPE_CO)
		{
			// Get file size from the .CO file
			gTcCtrl.tFiles[gTcCtrl.tCount].size = 
				get_memory16(gTcCtrl.tFiles[gTcCtrl.tCount].address + 2) + 6;
		}
		else if (file_type == TYPE_DO)
		{
			// Start at beginning of file
			x = 0;
			fdata = get_memory8(gTcCtrl.tFiles[gTcCtrl.tCount].address);

			// Search through the file until 0x1A found
			while (fdata != 0x1A)
			{
				x++;
				fdata = get_memory8(gTcCtrl.tFiles[gTcCtrl.tCount].address + x);
			}

			// Set size
			gTcCtrl.tFiles[gTcCtrl.tCount].size = x;
		}
		else if (file_type == TYPE_BA)
		{
			// Scan through BASIC file until NULL pointer reached
			addr2 = get_memory16(gTcCtrl.tFiles[gTcCtrl.tCount].address);
			while (get_memory16(addr2) != 0)
			{
				// Get line pointer of the next line
				addr2 = get_memory16(addr2);
			}
			gTcCtrl.tFiles[gTcCtrl.tCount].size = addr2 - gTcCtrl.tFiles[gTcCtrl.tCount].address;
		}
		
		// Space pad the filename
		sprintf(mt_file, "%-11s %d", gTcCtrl.tFiles[gTcCtrl.tCount].name, 
			gTcCtrl.tFiles[gTcCtrl.tCount].size);

		// Add file to Hold Browser and increment for next file
		gTcCtrl.pFileSelect->add(mt_file);
		gTcCtrl.tCount++;
		dir_index++;
		addr1 += 11;
	}
}

/*
============================================================================
Callback routine to read TPDD / NADSBox directory
============================================================================
*/
static void cb_TpddDir(Fl_Widget* w, void*)
{
}

/*
============================================================================
Routine to create the TPDD Client window
============================================================================
*/
void cb_TpddClient(Fl_Widget* w, void*)
{
	Fl_Box*			o;

	if (gtcw != NULL)
		return;

	// Create File Viewer window
	gtcw = new Fl_Double_Window(650, 480, "TPDD / NADSBox Client");
	gtcw->callback(cb_tcwin);

	// Create a menu for the new window.
	gTcCtrl.pMenu = new Fl_Menu_Bar(0, 0, 700, MENU_HEIGHT-2);
	gTcCtrl.pMenu->menu(gTpddClient_menuitems);

	o = new Fl_Box(20, 40, 80, 20, "File Directory");
	o->align(FL_ALIGN_LEFT|FL_ALIGN_INSIDE);

	// Create a browser for the files
	gTcCtrl.pFileSelect = new Fl_Hold_Browser(20, 65, 170, 270, 0);
	gTcCtrl.pFileSelect->textfont(FL_COURIER);
	gTcCtrl.pFileSelect->has_scrollbar(Fl_Browser_::VERTICAL);
	gTcCtrl.pFileSelect->callback(cb_FileSelect); 

	// Create a Text Editor / Text Buffer for the file display
	gTcCtrl.pView = new Fl_Hold_Browser(200, 65, 420, 270, "");
	gTcCtrl.pView->has_scrollbar(Fl_Browser_::VERTICAL );
    gTcCtrl.pView->textfont(FL_COURIER);
	gTcCtrl.pView->textsize(12);

	// Create a button to Read the TPDD / NADSBox Directory
	gTcCtrl.pTpddDir = new Fl_Button(20, 350, 120, 20, "TPDD Directory");
	gTcCtrl.pTpddDir->callback(cb_TpddDir);

	// Create a button to copy files to the TPDD / NADSBox

	// Create a button to copy files from the TPDD / NADSBox

	// Create a static box showing the COM port being used (from the Setup page)

	gtcw->show();
}
