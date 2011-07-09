/* fileview.cpp */

/* $Id: fileview.cpp,v 1.1 2008/11/04 07:31:22 kpettit1 Exp $ */

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

#define	FILE_VIEW_MAIN

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
#include "fileview.h"
#include "display.h"
#include "cpuregs.h"
#include "disassemble.h"
#include "periph.h"
#include "memedit.h"
#include "file.h"

// ===============================================
// Extern "C" linkage items
// ===============================================
extern "C"
{
#include "memory.h"
#include "m100emu.h"

extern RomDescription_t *gStdRomDesc;
void jump_to_zero(void);
}

// ===============================================
// Extern and global variables
// ===============================================
extern	Fl_Preferences virtualt_prefs;
extern	Fl_Menu_Item gCpuRegs_menuitems[];
extern	const char *gKeywordTable[];
Fl_Window*		gfvw = NULL;
int				gLowmem = 0;
int				gPrevUnused = 0;
unsigned char	gRam[32768];
fileview_ctrl_t	gFvCtrl;

void cb_Ide(Fl_Widget* w, void*) ;
void cb_view_refreseh(Fl_Widget*w, void*);

// Menu items for the disassembler
Fl_Menu_Item gFileView_menuitems[] = {
 { "&View",         0, 0,        0, FL_SUBMENU },
	{ "Refresh",	0, cb_view_refreseh, 0, 0 },
	{ 0 },
 { "&Tools", 0, 0, 0, FL_SUBMENU },
	{ "CPU Registers",         0, cb_CpuRegs },
	{ "Assembler / IDE",       0, cb_Ide },
	{ "Disassembler",          0, disassembler_cb },
	{ "Memory Editor",         0, cb_MemoryEditor },
	{ "Peripheral Devices",    0, cb_PeripheralDevices },
	{ 0 },
  { 0 }
};
void fileview_mem_monitor(void);

/*
============================================================================
Callback routine for the File Viewer window
============================================================================
*/
void cb_fvwin (Fl_Widget* w, void*)
{
	mem_clear_monitor_callback(fileview_mem_monitor);

	// Hide the window
	gfvw->hide();

	// Delete the window and set to NULL
	delete gfvw;
	gfvw = NULL;
}

/*
============================================================================
Callback routine when a file is selected
============================================================================
*/
void display_file(int index)
{
	int					addr1, addr2, c, x, y, lineno, maxline;
	char				line[1000], substr[10], linefmt[6], fill[7];
	unsigned char		ch;
	file_view_files_t	*pfile;

	// First clear the text buffer
//	gFvCtrl.pTb->remove(0, gFvCtrl.pTb->length());
	gFvCtrl.pView->clear();

	// Populate with data based on file type
	pfile = &gFvCtrl.tFiles[index];
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
								gFvCtrl.pView->add(line);
//								gFvCtrl.pTb->append(line);
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
						gFvCtrl.pView->add(line);
//						gFvCtrl.pTb->append(line);
						strcpy(line, fill);
					}
				}
				else if (ch > 0x7F)
				{
					if (strlen(line) + strlen(gKeywordTable[ch & 0x7F]) > 54)
					{
						strcat(line, "\n");
						gFvCtrl.pView->add(line);
//						gFvCtrl.pTb->append(line);
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
						gFvCtrl.pView->add(line);
//						gFvCtrl.pTb->append(line);
						strcpy(line, fill);
					}
				}
			}
//			gFvCtrl.pTb->append(line);
			if (strlen(line) != strlen(fill))
				gFvCtrl.pView->add(line);
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
					gFvCtrl.pView->add(line);
//					gFvCtrl.pTb->append(line);
					y = 0;
					line[0] = 0;
					continue;
				}

				if (strlen(line) > 55)
				{
					line[y+1] = '\n';
					line[y+2] = 0;
//					gFvCtrl.pTb->append(line);
					gFvCtrl.pView->add(line);
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
//			gFvCtrl.pTb->append(line);
			gFvCtrl.pView->add(line);

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
//			gFvCtrl.pTb->append(line);
			gFvCtrl.pView->add(line);
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
void cb_FileSelect(Fl_Widget* w, void*)
{
	int					index, x, usage, lowram;
	file_view_files_t	*pfile;
	
	lowram = get_memory16(gStdRomDesc->sLowRam);

	// Check if there is an active selection and restore usage map
	if (gFvCtrl.selectIndex != -1)
	{
		pfile = & gFvCtrl.tFiles[gFvCtrl.selectIndex];
		gFvCtrl.pRAM->AddUsageEvent(pfile->address - lowram,
			pfile->address + pfile->size - lowram, -FV_SELECT_FILE_COLOR);
		switch (pfile->type)
		{
		case TYPE_BA:
			usage = FV_BASIC_FILE_COLOR;
			break;
		case TYPE_DO:
			usage = FV_TEXT_FILE_COLOR;
			break;
		case TYPE_CO:
			usage = FV_CO_FILE_COLOR;
			break;
		}
		gFvCtrl.pRAM->AddUsageEvent(pfile->address - lowram, pfile->address +
			pfile->size - lowram, usage);
		gFvCtrl.selectIndex = -1;
	}

	// Get selection from Hold Browser
	index = ((Fl_Browser*)w)->value()-1;
	if ((index < 0) || (index > gFvCtrl.tCount))
	{
		// Clear the Directry Entry text
		gFvCtrl.sDirEntry[0] = 0;
		gFvCtrl.sAddress[0] = 0;
		gFvCtrl.pAddress->label(gFvCtrl.sAddress);
		gFvCtrl.pDirEntry->label(gFvCtrl.sDirEntry);
//		gFvCtrl.pTb->remove(0, gFvCtrl.pTb->length());
		gFvCtrl.pView->clear();
		return;
	}

	pfile = & gFvCtrl.tFiles[index];

	// Format the Directory Entry text
	if (strcmp(pfile->name, "Unsaved BA") == 0)
		sprintf(gFvCtrl.sDirEntry, "%04Xh:  %02X %02X ", pfile->dir_address,
			pfile->address & 0xFF,pfile->address >> 8);
	else
	{
		sprintf(gFvCtrl.sDirEntry, "%04Xh:  %02X %02X %02X ", pfile->dir_address,
			pfile->type, pfile->address & 0xFF,pfile->address >> 8);

		// Add the filename bytes
		for (x = 3; x < 11; x++)
		{
			char	substr[5];
			sprintf(substr, "%02x ", get_memory8(pfile->dir_address+x));
			strcat(gFvCtrl.sDirEntry, substr);
		}
	}

	//  Update file address
	sprintf(gFvCtrl.sAddress, "Addr: %d (%04Xh)", pfile->address, pfile->address);
	gFvCtrl.pAddress->label(gFvCtrl.sAddress);

	// Set the text label
	gFvCtrl.pDirEntry->label(gFvCtrl.sDirEntry);

	// Change selection on usage map
	switch (pfile->type)
	{
	case TYPE_BA:
		usage = FV_BASIC_FILE_COLOR;
		break;
	case TYPE_DO:
		usage = FV_TEXT_FILE_COLOR;
		break;
	case TYPE_CO:
		usage = FV_CO_FILE_COLOR;
		break;
	}

	// Clear the previous usage
	gFvCtrl.pRAM->AddUsageEvent(pfile->address - lowram, pfile->address +
		pfile->size - lowram, -usage);
	gFvCtrl.pRAM->AddUsageEvent(pfile->address - lowram, pfile->address +
		pfile->size - lowram, FV_SELECT_FILE_COLOR);
	gFvCtrl.selectIndex = index;
	
	// Display the file
	display_file(index);
}

/*
============================================================================
Get list of files in tFiles structure and update SelectBrowser
============================================================================
*/
void get_file_list(void)
{
	int				dir_index, addr1, addr2;
	unsigned char	file_type;
	int				c, x;
	char			mt_file[40], fdata;

	// Initialize variables
	dir_index = 0;	 
	gFvCtrl.tCount = 0;
	gFvCtrl.pFileSelect->clear();
	gFvCtrl.selectIndex = -1;

	// Check for Unsved BASIC program
	addr2 = get_memory16(gStdRomDesc->sFilePtrBA);
//	addr2 = get_memory16(addr2);
	if ((get_memory16(addr2) != 0) && (addr2 != 0))
	{
		// Add entry for unsaved BASIC program
		strcpy(gFvCtrl.tFiles[0].name, "Unsaved BA");
		gFvCtrl.tFiles[0].dir_address = gStdRomDesc->sFilePtrBA;
		gFvCtrl.tFiles[0].address = addr2;
		gFvCtrl.tFiles[0].type = TYPE_BA;
		addr1 = get_memory16(addr2);
		if (addr1 == 0)
		{
			gFvCtrl.tFiles[0].size = addr2 - get_memory16(gStdRomDesc->sFilePtrBA);
		}
		else
		{
			while ((get_memory16(addr1) != 0) && (addr1 != 0))
			{
				// Point to next BASIC line
				addr1 = get_memory16(addr1);
			}
			gFvCtrl.tFiles[0].size = addr1 - addr2;
		}

		// Add file to Hold Browser
		sprintf(mt_file, "%-11s %d", gFvCtrl.tFiles[gFvCtrl.tCount].name, 
			gFvCtrl.tFiles[gFvCtrl.tCount].size);
		gFvCtrl.pFileSelect->add(mt_file);

		// Increment file count
		gFvCtrl.tCount++;
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
		strcpy(gFvCtrl.tFiles[gFvCtrl.tCount].name, mt_file);
		gFvCtrl.tFiles[gFvCtrl.tCount].type = file_type;

		// Get address of file
		gFvCtrl.tFiles[gFvCtrl.tCount].address = get_memory16(addr1+1);
		gFvCtrl.tFiles[gFvCtrl.tCount].dir_address = addr1;

		// Calculate file size
		if (file_type == TYPE_CO)
		{
			// Get file size from the .CO file
			gFvCtrl.tFiles[gFvCtrl.tCount].size = 
				get_memory16(gFvCtrl.tFiles[gFvCtrl.tCount].address + 2) + 6;
		}
		else if (file_type == TYPE_DO)
		{
			// Start at beginning of file
			x = 0;
			fdata = get_memory8(gFvCtrl.tFiles[gFvCtrl.tCount].address);

			// Search through the file until 0x1A found
			while (fdata != 0x1A)
			{
				x++;
				fdata = get_memory8(gFvCtrl.tFiles[gFvCtrl.tCount].address + x);
			}

			// Set size
			gFvCtrl.tFiles[gFvCtrl.tCount].size = x;
		}
		else if (file_type == TYPE_BA)
		{
			// Scan through BASIC file until NULL pointer reached
			addr2 = get_memory16(gFvCtrl.tFiles[gFvCtrl.tCount].address);
			while (get_memory16(addr2) != 0)
			{
				// Get line pointer of the next line
				addr2 = get_memory16(addr2);
			}
			gFvCtrl.tFiles[gFvCtrl.tCount].size = addr2 - gFvCtrl.tFiles[gFvCtrl.tCount].address;
		}
		
		// Space pad the filename
		sprintf(mt_file, "%-11s %d", gFvCtrl.tFiles[gFvCtrl.tCount].name, 
			gFvCtrl.tFiles[gFvCtrl.tCount].size);

		// Add file to Hold Browser and increment for next file
		gFvCtrl.pFileSelect->add(mt_file);
		gFvCtrl.tCount++;
		dir_index++;
		addr1 += 11;
	}
}

/*
============================================================================
Update the RAM usage map
============================================================================
*/
void update_ram_usage_map(void)
{
	int		lowram;

	// Clear old usage information
	gFvCtrl.pRAM->ClearUsageMap();

	// Get LOWRAM address.  This will change for T200 emulation
	lowram = get_memory16(gStdRomDesc->sLowRam);
	gLowmem = lowram;
	gFvCtrl.pRAM->SetUsageRange(0, 65536 - lowram);

	// Set usage for BASIC files
	gFvCtrl.pRAM->AddUsageEvent(1, get_memory16(gStdRomDesc->sBeginDO) - 
		lowram, FV_BASIC_FILE_COLOR);

	// Set usage for TEXT files
	gFvCtrl.pRAM->AddUsageEvent(get_memory16(gStdRomDesc->sBeginDO)-lowram, 
		get_memory16(gStdRomDesc->sBeginCO) - lowram, FV_TEXT_FILE_COLOR);

	// Set usage for CO files
	gFvCtrl.pRAM->AddUsageEvent(get_memory16(gStdRomDesc->sBeginCO)-lowram, 
		get_memory16(gStdRomDesc->sBeginVar) - lowram, FV_CO_FILE_COLOR);

	// Set usage for CO files
	gFvCtrl.pRAM->AddUsageEvent(get_memory16(gStdRomDesc->sBeginVar)-lowram, 
		get_memory16(gStdRomDesc->sUnusedMem) - lowram, FV_BASIC_VARS_COLOR);

	// Set usage for Directory
	gFvCtrl.pRAM->AddUsageEvent(gStdRomDesc->sDirectory-lowram, 
		gStdRomDesc->sDirectory - lowram + 11 * gStdRomDesc->sDirCount, FV_DIR_COLOR);

	// Set usage for Directory
	gFvCtrl.pRAM->AddUsageEvent(gStdRomDesc->sDirectory-lowram + gStdRomDesc->sDirCount * 11, 
		65536-lowram, FV_SYS_VARS_COLOR);
}

/*
============================================================================
Routine to change display when the model changes
============================================================================
*/
void fileview_model_changed(void)
{
	// Test if File View window is open
	if (gfvw == NULL)
		return;

	// Update the usage map
	update_ram_usage_map();

	// Update the FileList
	get_file_list();

	cb_FileSelect(gFvCtrl.pFileSelect, NULL);
}

void cb_view_refreseh(Fl_Widget*w, void*)
{
	fileview_model_changed();
}
/*
============================================================================
Update file pointers in tFiles structure
============================================================================
*/
void adjust_file_pointers()
{
	// Calculate all BASIC file pointers
}

/*
============================================================================
Monitor routine for memory changes
============================================================================
*/
void fileview_mem_monitor(void)
{
	int		unused, x, c;
	int		index;
	char	selectfile[15];

	// Test if unused memory changed
	unused = get_memory16(gStdRomDesc->sBeginVar);
	if (unused == gPrevUnused)
		return;
	gPrevUnused = unused;

	// Test if file area changed
	for (x = gLowmem; x < gPrevUnused; x++)
	{
		// Test if memory changed
		if (get_memory8(x) != gRam[x-gLowmem])
		{
			// Memory changed - Update file menu
			strcpy(selectfile, gFvCtrl.tFiles[gFvCtrl.selectIndex].name);
			get_file_list();

			// Detect if any file pointers are incorrect because of data inserted
			adjust_file_pointers();

			// Update usage map
			update_ram_usage_map();

			// Reselect the previously selected file
			index = -1;
			for (c = 0; c < gFvCtrl.tCount; c++)
			{
				// Find the name of the previously selected file
				if (strcmp(selectfile, gFvCtrl.tFiles[c].name) == 0)
				{
					index = c;
					break;
				}
			}

			// Test if file still exists
			if (index != -1)
			{
				gFvCtrl.pFileSelect->select(index+1);
				cb_FileSelect(gFvCtrl.pFileSelect, NULL);
			}
			else
			{
				// File is gone!  Select nothing
//				gFvCtrl.pTb->remove(0, gFvCtrl.pTb->length());
				gFvCtrl.pView->clear();
				gFvCtrl.sAddress[0] = 0;
				gFvCtrl.sDirEntry[0] = 0;
				gFvCtrl.pAddress->label(gFvCtrl.sAddress);
				gFvCtrl.pDirEntry->label(gFvCtrl.sDirEntry);
			}

			// Save RAM for next comparison
			for (; x < gPrevUnused; x++)
				gRam[x-gLowmem] = get_memory8(x);
			return;
		}
	}

	return;
}

/*
============================================================================
Routine to create the Model T File viewer window
============================================================================
*/
void cb_FileView(Fl_Widget* w, void*)
{
	Fl_Box*			o;
	int				width;

	if (gfvw != NULL)
		return;

#ifdef WIN32
	width = 650;
#else
	width = 760;
#endif

	// Create File Viewer window
	gfvw = new Fl_Double_Window(width, 480, "Model T File Viewer");
	gfvw->callback(cb_fvwin);

	// Create a menu for the new window.
	gFvCtrl.pMenu = new Fl_Menu_Bar(0, 0, width, MENU_HEIGHT-2);
	gFvCtrl.pMenu->menu(gFileView_menuitems);

	o = new Fl_Box(20, 40, 80, 20, "File Directory");
	o->align(FL_ALIGN_LEFT|FL_ALIGN_INSIDE);

	// Create a browser for the files
	gFvCtrl.pFileSelect = new Fl_Hold_Browser(20, 65, 170, 270, 0);
	gFvCtrl.pFileSelect->textfont(FL_COURIER);
	gFvCtrl.pFileSelect->has_scrollbar(Fl_Browser_::VERTICAL);
	gFvCtrl.pFileSelect->callback(cb_FileSelect); 

	// Create a Text Editor / Text Buffer for the file display
	gFvCtrl.pView = new Fl_Hold_Browser(200, 65, width-230, 270, "");
	gFvCtrl.pView->has_scrollbar(Fl_Browser_::VERTICAL );
//    gFvCtrl.pView = new Fl_Text_Display(200, 65, 420, 270, "");
//    gFvCtrl.pTb = new Fl_Text_Buffer();
//    gFvCtrl.pView->buffer(gFvCtrl.pTb);
    gFvCtrl.pView->textfont(FL_COURIER);
#ifdef WIN32
	gFvCtrl.pView->textsize(12);
#else
	gFvCtrl.pView->textsize(14);
#endif
//	gFvCtrl.pView->wrap_mode(56, 56);
//    gFvCtrl.pView->end();

	// Create a box for RAM
	o = new Fl_Box(FL_NO_BOX, 40, 380, 100, 15, "RAM");
	o->align(FL_ALIGN_LEFT|FL_ALIGN_INSIDE);
	o = new Fl_Box(FL_NO_BOX, 40, 395, 100, 15, "Map");
	o->align(FL_ALIGN_LEFT|FL_ALIGN_INSIDE);
	gFvCtrl.pRAM = new Fl_Usage_Box(100, 365, width-134, 68);
	gFvCtrl.pRAM->box(FL_BORDER_BOX);
	gFvCtrl.pRAM->SetUsageColor(FV_SELECT_FILE_COLOR, fl_rgb_color(64, 64, 64));
	gFvCtrl.pRAM->SetUsageColor(FV_BASIC_FILE_COLOR	, fl_rgb_color(96, 255,96));
	gFvCtrl.pRAM->SetUsageColor(FV_TEXT_FILE_COLOR	, fl_rgb_color(96, 96, 255));
	gFvCtrl.pRAM->SetUsageColor(FV_CO_FILE_COLOR	, fl_rgb_color(255, 96, 96));
	gFvCtrl.pRAM->SetUsageColor(FV_BASIC_VARS_COLOR	, fl_rgb_color(255, 255, 96));
	gFvCtrl.pRAM->SetUsageColor(FV_DIR_COLOR		, fl_rgb_color(255, 128, 0));
	gFvCtrl.pRAM->SetUsageColor(FV_SYS_VARS_COLOR	, fl_rgb_color(0, 128, 255));
	gFvCtrl.pRAM->SetUsageColor(FV_LCD_BUF_COLOR	, fl_rgb_color(225, 225, 0));

	// Create Directory Entry display box
	o = new Fl_Box(200, 40, 80, 20, "Dir Entry:");
	o->align(FL_ALIGN_LEFT|FL_ALIGN_INSIDE);

	// Create a text box for showing Directory Entry information
	gFvCtrl.pDirEntry = new Fl_Box(280, 40, 400, 20, "");
	gFvCtrl.pDirEntry->align(FL_ALIGN_LEFT|FL_ALIGN_INSIDE);
	gFvCtrl.pDirEntry->labelfont(FL_COURIER);

	// Create a text box for showing the file address
	gFvCtrl.pAddress = new Fl_Box(20, 340, 150, 20, "");
	gFvCtrl.pAddress->align(FL_ALIGN_LEFT|FL_ALIGN_INSIDE);

	// Create color code boxes
	o = new Fl_Box(FL_NO_BOX, 100, 440, 60, 15, "Free");
	o->align(FL_ALIGN_LEFT|FL_ALIGN_INSIDE);
	o = new Fl_Box(FL_DOWN_BOX, 150, 440, 15, 15, "");
	o->color(fl_rgb_color(255, 255, 255));

	// Create color code boxes
	o = new Fl_Box(FL_NO_BOX, 180, 440, 60, 15, "BASIC");
	o->align(FL_ALIGN_LEFT|FL_ALIGN_INSIDE);
	o = new Fl_Box(FL_DOWN_BOX, 230, 440, 15, 15, "");
	o->color(fl_rgb_color(96, 255, 96));

	// Create color code boxes
	o = new Fl_Box(FL_NO_BOX, 255, 440, 60, 15, "Text");
	o->align(FL_ALIGN_LEFT|FL_ALIGN_INSIDE);
	o = new Fl_Box(FL_DOWN_BOX, 300, 440, 15, 15, "");
	o->color(fl_rgb_color(96, 96, 255));

	// Create color code boxes
	o = new Fl_Box(FL_NO_BOX, 330, 440, 60, 15, "CO");
	o->align(FL_ALIGN_LEFT|FL_ALIGN_INSIDE);
	o = new Fl_Box(FL_DOWN_BOX, 365, 440, 15, 15, "");
	o->color(fl_rgb_color(255, 96, 96));

	// Create color code boxes
	o = new Fl_Box(FL_NO_BOX, 400, 440, 60, 15, "Dir");
	o->align(FL_ALIGN_LEFT|FL_ALIGN_INSIDE);
	o = new Fl_Box(FL_DOWN_BOX, 435, 440, 15, 15, "");
	o->color(fl_rgb_color(255, 128, 0));

	// Create color code boxes
	o = new Fl_Box(FL_NO_BOX, 475, 440, 60, 15, "Sys");
	o->align(FL_ALIGN_LEFT|FL_ALIGN_INSIDE);
	o = new Fl_Box(FL_DOWN_BOX, 510, 440, 15, 15, "");
	o->color(fl_rgb_color(0, 128, 255));

	// Create color code boxes
	o = new Fl_Box(FL_NO_BOX, 545, 440, 60, 15, "Select");
	o->align(FL_ALIGN_LEFT|FL_ALIGN_INSIDE);
	o = new Fl_Box(FL_DOWN_BOX, 595, 440, 15, 15, "");
	o->color(fl_rgb_color(64, 64, 64));

	// Populate file list and usage map with data
	get_file_list();
	update_ram_usage_map();

	gfvw->show();

//	mem_set_monitor_callback(fileview_mem_monitor);
}
