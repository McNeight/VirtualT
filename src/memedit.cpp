/* memedit.cpp */

/* $Id: memedit.cpp,v 1.15 2013/02/11 08:37:17 kpettit1 Exp $ */

/*
 * Copyright 2004 Ken Pettit and Stephen Hurd 
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
#include <FL/Fl_Menu_Bar.H>
#include <FL/fl_show_colormap.H>

#include "FLU/Flu_File_Chooser.h"

#if defined(WIN32)
#include <windows.h>
#endif
#include <stdio.h>
#include <string.h>

#include "VirtualT.h"
#include "m100emu.h"
#include "disassemble.h"
#include "memedit.h"
#include "memory.h"
#include "cpu.h"
#include "cpuregs.h"
#include "fileview.h"
#include "watchtable.h"

extern "C"
{
#include "intelhex.h"

extern int		gRamBottom;
}

void cb_Ide(Fl_Widget* w, void*); 
void cb_menu_run(Fl_Widget* w, void*);
void cb_menu_stop(Fl_Widget* w, void*);
void cb_menu_step(Fl_Widget* w, void*);
void cb_menu_step_over(Fl_Widget* w, void*);

/*
================================================================
Define the structure to hold all memory editor controls
================================================================
*/
typedef struct memedit_ctrl_struct	
{
	Fl_Menu_Bar*		pMenu;

	Fl_Double_Window*	pTopPane;
	Fl_Double_Window*	pBottomPane;
	T100_MemEditor*		pMemEdit;
	VT_Watch_Table*		pWatchTable;
	Fl_Scrollbar*		pScroll;
	Fl_Input*			pMemRange;
	Fl_Box*				pRegionText;
	Fl_Box*				pAddressText;
	Fl_Choice*			pRegion;

	char				sFilename[256];
	int					sAddrStart;
	int					sAddrEnd;
	int					sAddrTop;
	int					iSplitH;
	int					iCol0W;
	int					iCol1W;
	int					iCol2W;
	int					iCol3W;
	int					iCol4W;
} memedit_ctrl_t;

typedef struct memedit_dialog
{
	Fl_Window*			pWin;			// Dialog box window
	Fl_Input*			pFilename;
	Fl_Input*			pStartAddr;
	Fl_Input*			pEndSaveAddr;
	Fl_Input*			pSaveLen;
	Fl_Box*				pEndAddr;
	Fl_Button*			pBrowse;
	Fl_Box*				pBytes;
	Fl_Button*			pHex;
	Fl_Button*			pBin;
	char				sBytes[20];
	int					iOk;
} memedit_dialog_t;

void cb_PeripheralDevices (Fl_Widget* w, void*);
void cb_load(Fl_Widget* w, void*);
void cb_save_bin(Fl_Widget* w, void*);
void cb_save_hex(Fl_Widget* w, void*);
void cb_save_memory(Fl_Widget* w, void*);
void cb_setup_memedit(Fl_Widget* w, void*);
void cb_add_marker(Fl_Widget* w, void*);
void cb_goto_next_marker(Fl_Widget* w, void*);
void cb_goto_prev_marker(Fl_Widget* w, void*);
void cb_delete_marker(Fl_Widget* w, void*);
void cb_delete_all_markers(Fl_Widget* w, void*);
void cb_undo_delete_all_markers(Fl_Widget* w, void*);

int str_to_i(const char *pStr);

extern "C"
{
extern uchar			gReMem;			/* Flag indicating if ReMem emulation enabled */
extern uchar			gRampac;
void			memory_monitor_cb(void);
}

/*
================================================================
Some bad bad bad global variables.
================================================================
*/
memedit_ctrl_t		memedit_ctrl;
memedit_dialog_t	gDialog;
Fl_Window			*gmew;				// Global Memory Edit Window
unsigned char		gDispMemory[8192];
extern	Fl_Preferences virtualt_prefs;

/*
================================================================
Menu items for the Memory Editor
================================================================
*/
Fl_Menu_Item gMemEdit_menuitems[] = {
  { "&File",              0, 0, 0, FL_SUBMENU },
	{ "Load from File...",          0, cb_load, 0 },
	{ "Save to File...",      0, cb_save_memory, 0, FL_MENU_DIVIDER },
	{ "Setup...",      0, cb_setup_memedit, 0},
	{ 0 },

  { "&Tools", 0, 0, 0, FL_SUBMENU },
	{ "CPU Registers",         0, cb_CpuRegs },
	{ "Assembler / IDE",       0, cb_Ide },
	{ "Disassembler",          0, disassembler_cb },
	{ "Peripheral Devices",    0, cb_PeripheralDevices },
	{ "Model T File Viewer",   0, cb_FileView },
	{ 0 },

  { 0 }
};

Fl_Menu_Item gAddMarkerMenu[] = {
	{ "  Create marker  ",			0,		cb_add_marker, 0 },
	{ "  Find next marker  ",		FL_F+7,	cb_goto_next_marker, 0 },
	{ "  Find prev marker  ",		FL_SHIFT+FL_F+7,	cb_goto_prev_marker, 0, FL_MENU_DIVIDER },
	{ "  Delete all markers  ",		0,		cb_delete_all_markers, 0, 0 },
	{ 0,							0,		cb_undo_delete_all_markers, 0, 0 },
	{ 0 }
};

Fl_Menu_Item gDeleteMarkerMenu[] = {
	{ "  Delete Marker  ",			0,		cb_delete_marker, 0},
	{ "  Find next marker  ",		FL_F+7,	cb_goto_next_marker, 0 },
	{ "  Find prev marker  ",		FL_SHIFT+FL_F+7,	cb_goto_prev_marker, 0, FL_MENU_DIVIDER },
	{ "  Delete All Markers  ",		0,		cb_delete_all_markers, 0, 0 },
	{ 0,							0,		cb_undo_delete_all_markers, 0, 0 },
	{ 0 }
};

/*
============================================================================
Callback routine for the memory editor window
============================================================================
*/
void cb_memeditwin (Fl_Widget* w, void*)
{
	gmew->hide();
	mem_clear_monitor_callback(memory_monitor_cb);
	MemoryEditor_SavePrefs();

	delete gmew;
	gmew = NULL;
}

/*
============================================================================
Callback routine for the Memory Dialog window
============================================================================
*/
void cb_dialogwin (Fl_Widget* w, void*)
{
	gDialog.pWin->hide();
}

/*
============================================================================
Callback routine for the Region choice box
============================================================================
*/
void cb_region(Fl_Widget* w, void*)
{
	// Set new region here
	memedit_ctrl.pMemEdit->SetScrollSize();
	memedit_ctrl.pMemEdit->ClearSelection();
	memedit_ctrl.pMemEdit->redraw();
	memedit_ctrl.pMemEdit->UpdateAddressText();
}

/*
============================================================================
Callback routine for the Memory Range edit box
============================================================================
*/
void cb_memory_range(Fl_Widget* w, void*)
{
	const char		*pStr;
	int				address;

	pStr = memedit_ctrl.pMemRange->value();
	if (strlen(pStr) != 0)
	{
		address = str_to_i(pStr);

		// Set new region here
		memedit_ctrl.pMemEdit->MoveTo(address);
	}
	else
	{
		memedit_ctrl.pMemEdit->ClearSelection();
		memedit_ctrl.pMemEdit->redraw();
	}
}

/*
============================================================================
Callback routine the OK button on the load memory dialog
============================================================================
*/
void load_okButton_cb(Fl_Widget* w, void*)
{

	// Test for valid filename
	if (strlen(gDialog.pFilename->value()) == 0)
	{
		fl_alert("Please supply filename");
		gDialog.pFilename->take_focus();
		return;
	}

	// Test if Start address supplied
	if (strlen(gDialog.pStartAddr->value()) == 0)
	{
		fl_alert("Please supply start address");
		gDialog.pStartAddr->take_focus();
		return;
	}

	// Test if writing - need end address or length if so
	if (gDialog.pHex != NULL)
	{
		if ((strlen(gDialog.pEndSaveAddr->value()) == 0) &&
			(strlen(gDialog.pSaveLen->value()) == 0))
		{
			fl_alert("Please supply end address or length");
			gDialog.pEndSaveAddr->take_focus();
			return;
		}
	}

	// Indicate OK button pressed
	gDialog.iOk = 1;

	gDialog.pWin->hide();
}

/*
============================================================================
Callback routine the Cancel button on the load memory dialog
============================================================================
*/
void load_cancelButton_cb(Fl_Widget* w, void*)
{
	gDialog.iOk = 0;
	gDialog.pWin->hide();
}

/*
============================================================================
Callback routine the length field updates
============================================================================
*/
void update_length_field(const char *filename)
{
	int					len;
	char				buffer[65536];
	unsigned short		start_addr;

	// Check for hex file
	len = strlen(filename);
	if (((filename[len-1] | 0x20) == 'x') &&
		((filename[len-2] | 0x20) == 'e') &&
		((filename[len-3] | 0x20) == 'h'))
	{
		// Check if hex file format is valid
		len = load_hex_file((char *) filename, buffer, &start_addr);

		if (len == 0)
		{
			strcpy(gDialog.sBytes, "Error in HEX");
			gDialog.pBytes->redraw_label();
			return;
		}
		else
		{
			// Update Length field and End address fields
			sprintf(gDialog.sBytes, "%d", len);
			gDialog.pBytes->redraw_label();
		}
	}
	else
	{
		// Open file as binary
		FILE* fd;
		if ((fd = fopen(filename, "r")) == NULL)
		{
			strcpy(gDialog.sBytes, "Error opening file");
			gDialog.pBytes->redraw_label();
		}
		else
		{
			// Determine file length
			fseek(fd, 0, SEEK_END);
			len = ftell(fd);
			fclose(fd);
			sprintf(gDialog.sBytes, "%d", len);
			gDialog.pBytes->redraw_label();
		}
		
	}
}

void load_file_to_mem(const char *filename, int address)
{
	int					len, region;
	unsigned char		*buffer;
	unsigned short		start_addr;

	// Adjust address if writing to RAM region
	region = memedit_ctrl.pMemEdit->GetRegionEnum();
	if ((region == REGION_RAM) || (region == REGION_RAM1) || (region == REGION_RAM2) ||
		(region == REGION_RAM3))
			address -= RAMSTART;

	// Check for hex file
	len = strlen(filename);
	if (((filename[len-1] | 0x20) == 'x') &&
		((filename[len-2] | 0x20) == 'e') &&
		((filename[len-3] | 0x20) == 'h'))
	{
		buffer = new unsigned char[65536];

		// Check if hex file format is valid
		len = load_hex_file((char *) filename, (char *) buffer, &start_addr);

		if (len == 0)
			return;
		else
		{
			// Write data to the selected region
			set_memory8_ext(region, address, len, buffer);
		}

		// Free the memory
		delete buffer;
	}
	else
	{
		// Open file as binary
		FILE* fd;
		if ((fd = fopen(filename, "rb")) != NULL)
		{
			int read_len;

			// Determine file length
			fseek(fd, 0, SEEK_END);
			len = ftell(fd);
			buffer = new unsigned char[len];
			fseek(fd, 0, SEEK_SET);
			while (len != 0)
			{
				read_len = fread(buffer, 1, len, fd);
				if (read_len == 0)
				{
					fl_alert("Error reading file!");
					break;
				}
				set_memory8_ext(region, address, read_len, buffer);
				len -= read_len;
				address += read_len;
			}
			fclose(fd);

			delete buffer;
		}
	}
}

void save_mem_to_file(const char *filename, int address, int count, int save_as_hex)
{
	unsigned char		*buffer;
	FILE				*fd;
	int					region;


	// Adjust address if writing from RAM
	region = memedit_ctrl.pMemEdit->GetRegionEnum();
	if ((region == REGION_RAM) || (region == REGION_RAM1) || (region == REGION_RAM2) ||
		(region == REGION_RAM3))
			address -= RAMSTART;

	// Check for hex file
	if (save_as_hex)
	{
		// Try to open the file for write
		fd = fopen(filename, "w+");
		if (fd == NULL)
			return;

		save_hex_file_ext(address, address+count-1, region, 0, fd);

		fclose(fd);
	}
	else
	{
		// Open file as binary
		FILE* fd;
		if ((fd = fopen(filename, "wb+")) != NULL)
		{
			buffer = new unsigned char[count];

			get_memory8_ext(region, address, count, buffer);
			fwrite(buffer, count, 1, fd);
			fclose(fd);

			delete buffer;
		}
	}
}

void load_browseButton_cb(Fl_Widget* w, void*)
{
	int					count;
	const char *		filename;

	fl_cursor(FL_CURSOR_WAIT);
	Flu_File_Chooser file(".","*.{bin,hex}",1,"Select File to Open");
	gDialog.pWin->set_non_modal();
	file.set_modal();

	fl_cursor(FL_CURSOR_DEFAULT);
	file.show();
	while (file.visible())
		Fl::wait();

	// Get filename
	count = file.count();
	if (count != 1)
	{
		gDialog.pWin->set_modal();
		return;
	}
	filename = file.value();
	if (filename == 0)
		return;

	// Update filename field
	gDialog.pFilename->value(filename);

	update_length_field(filename);
}

void save_browseButton_cb(Fl_Widget* w, void*)
{
	int					count, len;
	const char *		filename;
	char				filespec[10];
	char				filepath[256];
	FILE				*fd;
	int					c;

	if (gDialog.pHex->value())
		strcpy(filespec, "*.hex");
	else
		strcpy(filespec, "*.bin");
	fl_cursor(FL_CURSOR_WAIT);
	Flu_File_Chooser file(".", filespec, 1, "Select File to Open");
	fl_cursor(FL_CURSOR_DEFAULT);

	gDialog.pWin->set_non_modal();
	file.set_modal();
	while (1)
	{
		file.show();
		file.take_focus();
		while (file.visible())
			Fl::wait();

		// Get filename
		count = file.count();
		if (count != 1)
		{
			gDialog.pWin->set_modal();
			return;
		}
		filename = file.value();
		if (filename == 0)
		{
			gDialog.pWin->set_modal();
			return;
		}

		// Try to open the ouput file
		fd = fopen(filename, "rb+");
		if (fd != NULL)
		{
			// File already exists! Check for overwrite!
			fclose(fd);

			c = fl_choice("Overwrite existing file?", "Cancel", "Yes", "No");

			// Test if Cancel selected
			if (c == 0)
			{
				gDialog.pWin->set_modal();
				return;
			}

			if (c == 1)
				break;
		}
	}
	gDialog.pWin->set_modal();

	// Check if filename ends with ".hex"
	len = strlen(filename);
	if (((filename[len-1] | 0x20) == filespec[4]) &&
		((filename[len-2] | 0x20) == filespec[3]) &&
		((filename[len-3] | 0x20) == filespec[2]) &&
		((filename[len-4] == '.')))
	{
			// Update filename field
			gDialog.pFilename->value(filename);
	}
	else
	{
		strcpy(filepath, filename);
		strcat(filepath, &filespec[1]);

		// Update filename field
		gDialog.pFilename->value(filepath);
	}
}

void load_filename_cb(Fl_Widget* w, void*)
{
	const char *filename = gDialog.pFilename->value();

	update_length_field(filename);
}

/*
============================================================================
Callback routine for the Load Menu Item
============================================================================
*/
void cb_load(Fl_Widget* w, void*)
{
	Fl_Box				*o;
	Fl_Return_Button	*rb;
	Fl_Button			*b;
	const char *		pStr;
	int					address;

	// Build dialog box for selecting file and Memory Range
	gDialog.pWin = new Fl_Window(400, 200, "Load Memory Dialog");
	gDialog.pWin->callback(cb_dialogwin);

	// Create edit field for filename
	o = new Fl_Box(FL_NO_BOX, 10, 10, 50, 15, "Filename");
	o->align(FL_ALIGN_LEFT|FL_ALIGN_INSIDE);
	gDialog.pFilename = new Fl_Input(10, 30, 300, 20, "");
	gDialog.pFilename->callback(load_filename_cb);

	// Create browse button
	gDialog.pBrowse = new Fl_Button(320, 30, 70, 20, "Browse");
	gDialog.pBrowse->callback(load_browseButton_cb);


	// Create report field for # bytes being loaded
	o = new Fl_Box(FL_NO_BOX, 25, 55, 100, 15, "Filesize (bytes):");
	o->align(FL_ALIGN_LEFT|FL_ALIGN_INSIDE);
	gDialog.sBytes[0] = 0;
	gDialog.pBytes = new Fl_Box(FL_NO_BOX, 135, 55, 150, 15, gDialog.sBytes);
	gDialog.pBytes->align(FL_ALIGN_LEFT|FL_ALIGN_INSIDE);

	// Create Edit fields for memory range
	o = new Fl_Box(FL_NO_BOX, 10, 90, 110, 15, "Start Address");
	o->align(FL_ALIGN_LEFT|FL_ALIGN_INSIDE);
	gDialog.pStartAddr = new Fl_Input(120, 90, 90, 20, "");
	gDialog.pStartAddr->value(memedit_ctrl.pMemRange->value());

	o = new Fl_Box(FL_NO_BOX, 10, 120, 110, 15, "End Address");
	o->align(FL_ALIGN_LEFT|FL_ALIGN_INSIDE);
	gDialog.pEndAddr = new Fl_Box(FL_NO_BOX, 120, 120, 90, 15, "");
	gDialog.pEndAddr->align(FL_ALIGN_LEFT|FL_ALIGN_INSIDE);

	// NULL out controls for Hex and Bin
	gDialog.pHex = NULL;
	gDialog.pBin = NULL;

	// Create OK and Cancel buttons
	rb = new Fl_Return_Button(90, 160, 75, 25, "OK");
	rb->callback(load_okButton_cb);

	b = new Fl_Button(210, 160, 75, 25, "Cancel");
	b->callback(load_cancelButton_cb);

	gDialog.pWin->set_modal();

	// Show the dialog box
	gDialog.pWin->show();

	while (gDialog.pWin->visible())
		Fl::wait();

	gmew->take_focus();
	gmew->show();

	// Check if OK button was pressed
	if (gDialog.iOk)
	{
		// Get Address where data should be written
		pStr = gDialog.pStartAddr->value();
		address = str_to_i(pStr);
		
		pStr = gDialog.pFilename->value();
		load_file_to_mem(pStr, address);

	}

	delete gDialog.pWin;
}

/*
============================================================================
Callback routine for the save BIN Menu Item
============================================================================
*/
void cb_save_bin(Fl_Widget* w, void*)
{
	char		filepath[256];
	const char	*pStr;
	int			len;

	// Get current filepath
	pStr = gDialog.pFilename->value();

	len = strlen(pStr);
	if (len != 0)
	{
		strcpy(filepath, pStr);
		if (filepath[len-4] == '.')
			filepath[len-4] = '\0';

		strcat(filepath, ".bin");

		gDialog.pFilename->value(filepath);
	}

}

/*
============================================================================
Callback routine for the save HEX Menu Item
============================================================================
*/
void cb_save_hex(Fl_Widget* w, void*)
{
	char		filepath[256];
	const char	*pStr;
	int			len;

	// Get current filepath
	pStr = gDialog.pFilename->value();

	len = strlen(pStr);
	if (len != 0)
	{
		strcpy(filepath, pStr);
		if (filepath[len-4] == '.')
			filepath[len-4] = '\0';

		strcat(filepath, ".hex");

		gDialog.pFilename->value(filepath);
	}
}

/*
============================================================================
Callback routine for the Save Menu Item
============================================================================
*/
void cb_save_memory(Fl_Widget* w, void*)
{
	Fl_Box				*o;
	Fl_Return_Button	*rb;
	Fl_Button			*b;
	const char *		pStr;
	int					address, end_addr, count;
	int					save_as_hex;

	// Build dialog box for selecting file and Memory Range
	gDialog.pWin = new Fl_Window(400, 230, "Save Memory Dialog");
	gDialog.pWin->callback(cb_dialogwin);

	// Create radio buttons to select save type
	gDialog.pHex = new Fl_Round_Button(10, 10, 110, 20, "Save as HEX");
	gDialog.pHex->type(FL_RADIO_BUTTON);
	gDialog.pHex->callback(cb_save_hex);
	gDialog.pHex->value(1);

	gDialog.pBin = new Fl_Round_Button(140, 10, 110, 20, "Save as BIN");
	gDialog.pBin->type(FL_RADIO_BUTTON);
	gDialog.pBin->callback(cb_save_bin);

	// Create edit field for filename
	o = new Fl_Box(FL_NO_BOX, 10, 40, 50, 15, "Filename");
	o->align(FL_ALIGN_LEFT|FL_ALIGN_INSIDE);
	gDialog.pFilename = new Fl_Input(10, 60, 300, 20, "");
	gDialog.pFilename->callback(load_filename_cb);

	// Create browse button
	gDialog.pBrowse = new Fl_Button(320, 60, 70, 20, "Browse");
	gDialog.pBrowse->callback(save_browseButton_cb);

	// Create Edit fields for memory range
	o = new Fl_Box(FL_NO_BOX, 10, 90, 110, 15, "Start Address");
	o->align(FL_ALIGN_LEFT|FL_ALIGN_INSIDE);
	gDialog.pStartAddr = new Fl_Input(120, 90, 90, 20, "");
	gDialog.pStartAddr->value(memedit_ctrl.pMemRange->value());

	o = new Fl_Box(FL_NO_BOX, 10, 120, 110, 15, "End Address");
	o->align(FL_ALIGN_LEFT|FL_ALIGN_INSIDE);
	gDialog.pEndSaveAddr = new Fl_Input(120, 120, 90, 20, "");

	// Create edit field for length
	o = new Fl_Box(FL_NO_BOX, 10, 150, 110, 15, "Length");
	o->align(FL_ALIGN_LEFT|FL_ALIGN_INSIDE);
	gDialog.pSaveLen = new Fl_Input(120, 150, 90, 20, "");

	// Create OK and Cancel buttons
	rb = new Fl_Return_Button(90, 190, 75, 25, "OK");
	rb->callback(load_okButton_cb);

	b = new Fl_Button(210, 190, 75, 25, "Cancel");
	b->callback(load_cancelButton_cb);

	gDialog.pWin->set_modal();

	// Show the dialog box
	gDialog.pWin->show();

	while (gDialog.pWin->visible())
		Fl::wait();

	// Check if OK button was pressed
	if (gDialog.iOk)
	{
		// Get Address where data should be written
		pStr = gDialog.pStartAddr->value();
		address = str_to_i(pStr);

		// Get End address
		pStr = gDialog.pEndSaveAddr->value();
		if (strlen(pStr) != 0)
		{
			end_addr = str_to_i(pStr);
			count = end_addr - address + 1;
		}
		else
		{
			pStr = gDialog.pSaveLen->value();
			count = str_to_i(pStr);
		}
		
		// Get save mode selection
		save_as_hex = gDialog.pHex->value();

		// Get filename selection
		pStr = gDialog.pFilename->value();
		save_mem_to_file(pStr, address, count, save_as_hex);

	}

	delete gDialog.pWin;
	gmew->take_focus();
}

/*
============================================================================
Callback routine for processing memory changes during debug
============================================================================
*/
extern "C"
{

void memory_monitor_cb(void)
{
	// Test if any currently displayed memory changed from last time
	memedit_ctrl.pMemEdit->UpdateDispMem();

	// Update any visible watch variables too
	memedit_ctrl.pWatchTable->UpdateTable();
}
}

/*
============================================================================
Routine to manages changes to emulation mode -- different Model type, 
  or different memory emulation.
============================================================================
*/
void update_memory_emulation()
{
}

/*
============================================================================
Callback routine for the Com Scrollbar
============================================================================
*/
void cb_MemEditScroll (Fl_Widget* w, void*)
{
	memedit_ctrl.pMemEdit->m_FirstLine = ((Fl_Scrollbar*) w)->value();
	memedit_ctrl.pMemEdit->redraw();
	memedit_ctrl.pMemEdit->UpdateAddressText();
}

/*
============================================================================
Callback routine to handle events from the WatchTable
============================================================================
*/
void cb_watch_events (int event, void* pOpaque)
{
	switch (event)
	{
	case WATCH_EVENT_VAL_CHANGED:
		memedit_ctrl.pMemEdit->UpdateDispMem();
		break;

	case FL_F + 2:
	case FL_F + 7:
		// Helper key to find next / prev marker
		if (Fl::get_key(FL_Shift_R) || Fl::get_key(FL_Shift_L))
			memedit_ctrl.pMemEdit->FindPrevMarker();
		else
			memedit_ctrl.pMemEdit->FindNextMarker();
		break;

	case FL_F + 10:
		cb_menu_step_over(NULL, NULL);
		break;
	case FL_F + 8:
		cb_menu_step(NULL, NULL);
		break;
	case FL_F + 5:
		cb_menu_run(NULL, NULL);
		break;
	case FL_F + 6:
		cb_menu_stop(NULL, NULL);
		break;
	}
}

/*
============================================================================
Routine to create the MemoryEditor Window
============================================================================
*/
void cb_MemoryEditor (Fl_Widget* pW, void*)
{
	Fl_Box*		o;
	int			h = 400;
	int			w = 585;
	const int	wh = 50;

	if (gmew != NULL)
		return;

	// Create Peripheral Setup window
#ifdef WIN32
	gmew = new Fl_Double_Window(w, h+wh, "Memory Editor");
	// Create a menu for the new window.
	memedit_ctrl.pMenu = new Fl_Menu_Bar(0, 0, w, MENU_HEIGHT-2);
#else
	gmew = new Fl_Window(735, h+wh, "Memory Editor");
	// Create a menu for the new window.
	memedit_ctrl.pMenu = new Fl_Menu_Bar(0, 0, 735, MENU_HEIGHT-2);
#endif

	gmew->callback(cb_memeditwin);
	memedit_ctrl.pMenu->menu(gMemEdit_menuitems);

		// Create a tiled window to support Project, Edit, and debug regions
	Fl_Tile* tile = new Fl_Tile(0,MENU_HEIGHT,w,h+wh-MENU_HEIGHT-3);

	memedit_ctrl.pTopPane = new Fl_Double_Window(0, MENU_HEIGHT, w, h-MENU_HEIGHT, "");

	Fl_Group* g = new Fl_Group(0, 10, w, 25, "");

	// Create static text boxes
	o = new Fl_Box(FL_NO_BOX, 10, 10/*+MENU_HEIGHT*/, 50, 15, "Region");
	o->align(FL_ALIGN_LEFT|FL_ALIGN_INSIDE);
	o = new Fl_Box(FL_NO_BOX, 175, 10/*+MENU_HEIGHT*/, 50, 15, "Address");
	o->align(FL_ALIGN_LEFT|FL_ALIGN_INSIDE);
	
	// Create Region choice box
	memedit_ctrl.pRegion = new Fl_Choice(70, 8/*+MENU_HEIGHT*/, 100, 20, "");
	memedit_ctrl.pRegion->callback(cb_region);

	// Create edit field for memory search / display field
	memedit_ctrl.pMemRange = new Fl_Input(235, 8/*+MENU_HEIGHT*/, 80, 20, "");
	memedit_ctrl.pMemRange->callback(cb_memory_range);
	memedit_ctrl.pMemRange->when(FL_WHEN_ENTER_KEY|FL_WHEN_NOT_CHANGED);

	// Create a resize box for the group and end the group
	o = new Fl_Box(w-15, 10, 5, 5, "");
	o->hide();
	g->resizable(o);
	g->end();

#ifdef WIN32
	memedit_ctrl.pMemEdit = new T100_MemEditor(10, 40/*+MENU_HEIGHT*/, 545, 350-MENU_HEIGHT+10);
	memedit_ctrl.pScroll = new Fl_Scrollbar(555, 40/*+MENU_HEIGHT*/, 15, 350-MENU_HEIGHT+10, "");
#else
	memedit_ctrl.pMemEdit = new T100_MemEditor(10, 40/*+MENU_HEIGHT*/, 695, 350-MENU_HEIGHT);
	memedit_ctrl.pScroll = new Fl_Scrollbar(705, 40/*+MENU_HEIGHT*/, 15, 350-MENU_HEIGHT, "");
#endif

	memedit_ctrl.pScroll->type(FL_VERTICAL);
	memedit_ctrl.pScroll->linesize(2);
	memedit_ctrl.pScroll->maximum(2);
	memedit_ctrl.pScroll->callback(cb_MemEditScroll);
	memedit_ctrl.pScroll->slider_size(1);

	// Make the menu and edit widget resizeable with the top tile window
	memedit_ctrl.pTopPane->resizable(memedit_ctrl.pMemEdit);
	memedit_ctrl.pTopPane->end();

	// Create a window for the watches
	memedit_ctrl.pBottomPane = new Fl_Double_Window(0, h, w, wh, "");

	g = new Fl_Group(0, 0, w, wh, "");
	Fl_Box* b = new Fl_Box(FL_DOWN_FRAME, 9, 2, w-22, wh-5, "");

	memedit_ctrl.pWatchTable = new VT_Watch_Table(12, 4, w-27, wh-9);
	memedit_ctrl.pWatchTable->header_color(fl_rgb_color(253, 252, 251));
	memedit_ctrl.pWatchTable->EventHandler(cb_watch_events);

#if 0
	MString name = "test_data0";
	MString addr = "0x8003";
	memedit_ctrl.pWatchTable->AddWatch(name, addr, 0, REGION_RAM, FALSE);
	name = "test_data1";
	memedit_ctrl.pWatchTable->AddWatch(name, addr, 0, REGION_RAM, FALSE);
	name = "test_data2";
	memedit_ctrl.pWatchTable->AddWatch(name, addr, 0, REGION_RAM, FALSE);
	name = "test_data3";
	memedit_ctrl.pWatchTable->AddWatch(name, addr, 0, REGION_RAM, FALSE);
	name = "test_data4";
	memedit_ctrl.pWatchTable->AddWatch(name, addr, 0, REGION_RAM, FALSE);
	name = "test_data5";
	memedit_ctrl.pWatchTable->AddWatch(name, addr, 0, REGION_RAM, FALSE);
	name = "test_data6";
	memedit_ctrl.pWatchTable->AddWatch(name, addr, 0, REGION_RAM, FALSE);
	name = "test_data7";
	memedit_ctrl.pWatchTable->AddWatch(name, addr, 0, REGION_RAM, FALSE);
	name = "test_data8";
	memedit_ctrl.pWatchTable->AddWatch(name, addr, 0, REGION_RAM, FALSE);
#endif
	// make the window resizable
	b = new Fl_Box(FL_NO_BOX, 12, wh-9, 5, 3, "");
	b->hide();
	g->resizable(b);
	g->end();


	memedit_ctrl.pBottomPane->resizable(g);
	memedit_ctrl.pBottomPane->end();

	// Make the tile resizable
	b = new Fl_Box(10, 0, w-40, h,"");
	b->hide();
	tile->resizable(b);
	tile->end();

	// Make the window resizable
	gmew->resizable(memedit_ctrl.pMenu);
	gmew->resizable(tile);
	gmew->end();

	memedit_ctrl.pMemEdit->SetScrollSize();

	// Set Memory Monitor callback
	mem_set_monitor_callback(memory_monitor_cb);

	// Get the user preferences for the memory editor
	MemoryEditor_LoadPrefs();
	memedit_ctrl.pMemEdit->UpdateColors();

	// Resize if user preferences have been set
	if (memedit_ctrl.pMemEdit->m_x != -1 && memedit_ctrl.pMemEdit->m_y != -1 && 
		memedit_ctrl.pMemEdit->m_w != -1 && memedit_ctrl.pMemEdit->m_h != -1)
	{
		int newx, newy, neww, newh;

		newx = memedit_ctrl.pMemEdit->m_x;
		newy = memedit_ctrl.pMemEdit->m_y;
		neww = memedit_ctrl.pMemEdit->m_w;
		newh = memedit_ctrl.pMemEdit->m_h;

		gmew->resize(newx, newy, neww, newh);
	}

	gmew->show();

	// Resize top pane if user preference has been set
	if (memedit_ctrl.iSplitH != -1)
	{
		if (memedit_ctrl.iSplitH < 64)
			memedit_ctrl.iSplitH = 64;
		memedit_ctrl.pTopPane->resize(memedit_ctrl.pTopPane->x(), memedit_ctrl.pTopPane->y(),
			memedit_ctrl.pTopPane->w(), memedit_ctrl.iSplitH);
		memedit_ctrl.pBottomPane->resize(memedit_ctrl.pBottomPane->x(), 
			memedit_ctrl.pTopPane->y()+memedit_ctrl.iSplitH, memedit_ctrl.pBottomPane->w(), 
			tile->h()-memedit_ctrl.iSplitH );
	}
	else
	{
		memedit_ctrl.pTopPane->resize(memedit_ctrl.pTopPane->x(), 
			memedit_ctrl.pTopPane->y(), memedit_ctrl.pTopPane->w(), 
			memedit_ctrl.pTopPane->h()-20);
		memedit_ctrl.pBottomPane->resize(memedit_ctrl.pBottomPane->x(), 
			memedit_ctrl.pBottomPane->y()-20, memedit_ctrl.pBottomPane->w(), 
			memedit_ctrl.pBottomPane->h()+20);
	}

	// Resize the column headers if user preferences have been set
	if (memedit_ctrl.iCol0W != -1)
	{
		memedit_ctrl.pWatchTable->HeaderBoxWidth(0, memedit_ctrl.iCol0W);
		memedit_ctrl.pWatchTable->HeaderBoxWidth(1, memedit_ctrl.iCol1W);
		memedit_ctrl.pWatchTable->HeaderBoxWidth(2, memedit_ctrl.iCol2W);
		memedit_ctrl.pWatchTable->HeaderBoxWidth(3, memedit_ctrl.iCol3W);
		memedit_ctrl.pWatchTable->HeaderBoxWidth(4, memedit_ctrl.iCol4W);
	}
}

/*
================================================================
cb_MemoryEditorUpdate:	This function updates the Memory Editor
						window controls when the model or memory
						emulation is changed.
================================================================
*/
void cb_MemoryEditorUpdate(void)
{
	// First validate that the memory editor window is open
	if (gmew == NULL)
		return;

	// Update the Region choice box
	memedit_ctrl.pMemEdit->SetRegionOptions();

	// Update the Scroll Sizes
	memedit_ctrl.pMemEdit->SetScrollSize();

	// Update Region ENUM
	memedit_ctrl.pMemEdit->GetRegionEnum();

	memedit_ctrl.pMemEdit->LoadRegionMarkers();

	// Redraw the memory editor
	memedit_ctrl.pMemEdit->redraw();
}

void cb_popup(Fl_Widget* w, void*)
{
	// Test if we need to cancel the selection
	memedit_ctrl.pMemEdit->ClearSelection();
	memedit_ctrl.pMemEdit->redraw();
}

void T100_MemEditor::resize(int x, int y, int w, int h)
{
	int		size;

	// Do the default resize stuff
	Fl_Widget::resize(x, y, w, h);

	// Now check if the cursor is out of bounds
	size = h / (int) m_Height;
	if (m_CursorRow >= size)
	{
		window()->make_current();
		EraseCursor();
		m_CursorRow = size-1;
		DrawCursor();
	}
	if (memedit_ctrl.pScroll != NULL)
	{
		if (memedit_ctrl.pScroll->value() + size > m_Max)
		{
			memedit_ctrl.pScroll->value((int) (m_Max - size), 1, 0, (int) m_Max);
			SetScrollSize();
			redraw();
		}
	}
}

/*
================================================================
Class constructor for the Memory Editor widget
================================================================
*/
T100_MemEditor::T100_MemEditor(int x, int y, int w, int h) :
  Fl_Widget(x, y, w, h)
{
	int			c;

	m_MyFocus = 0;
	m_FirstLine = 0;
	m_HaveMouse = FALSE;
	m_DragX = m_DragY = 0;
	m_Cols = 0;
	m_Lines = 0;
	m_FirstAddress = 0;
	m_Height = 0;
	m_Width = 0;
	m_CursorAddress = 0;
	m_CursorCol = -1;
	m_CursorRow = -1;
	m_CursorField = 0;
	m_pScroll = NULL;
	m_SelActive = 0;
	m_SelEndRow = 0;
	m_SelEndCol = 0;
	m_SelStartAddr = -1;
	m_SelEndAddr = -1;
	m_BlackBackground = 0;
	m_ColorSyntaxHilight = 0;
	m_Font = FL_COURIER;
	m_PopupActive = FALSE;
	m_Region = 0;
#ifdef WIN32
	m_FontSize = 12;
#else
	m_FontSize = 14;
#endif
	SetRegionOptions();

	// NULL out the region marker pointers
	for (c = 0; c < REGION_MAX; c++)
	{
		m_pRegionMarkers[c] = NULL;
		m_pUndoDeleteMarkers[c] = NULL;
	}

	// Create a popup-menu for right-clicks
	m_pPopupMenu = new Fl_Menu_Button(0,0,100,400,"Popup");
	m_pPopupMenu->type(Fl_Menu_Button::POPUP123);
	m_pPopupMenu->menu(gAddMarkerMenu);
	m_pPopupMenu->selection_color(fl_rgb_color(80,80,255));
	m_pPopupMenu->color(fl_rgb_color(240,239,228));
	m_pPopupMenu->hide();
	m_pPopupMenu->callback(cb_popup);
}

T100_MemEditor::~T100_MemEditor()
{
	// Delete the popup menu
	delete m_pPopupMenu;

	// Delete all markers and undo markers
	ResetMarkers();
}

/*
================================================================
Updates the Region list items based on the current model.
================================================================
*/
void T100_MemEditor::SetRegionOptions(void)
{
	memedit_ctrl.pRegion->clear();

	// Update the Regions control with appropriate options
	if (gReMem)
	{
		if (gRex)
		{
			if (gModel == MODEL_T200 || gModel == MODEL_PC8201)
			{
				memedit_ctrl.pRegion->add("RAM 1");
				memedit_ctrl.pRegion->add("RAM 2");
				memedit_ctrl.pRegion->add("RAM 3");
			}
			else
			{
				memedit_ctrl.pRegion->add("RAM");
			}
			memedit_ctrl.pRegion->add("ROM");
			if (gModel == MODEL_T200)
				memedit_ctrl.pRegion->add("ROM 2");
			memedit_ctrl.pRegion->add("Flash");
			if (gRex == REX2)
				memedit_ctrl.pRegion->add("128K SRAM");
			memedit_ctrl.pRegion->value(0);
		}
		else 
		{
			memedit_ctrl.pRegion->add("RAM");
			memedit_ctrl.pRegion->add("Flash 1");
			memedit_ctrl.pRegion->add("Flash 2");
			memedit_ctrl.pRegion->value(0);
		}
	}
	else
	{
		switch (gModel)
		{
		case MODEL_M100:
		case MODEL_KC85:
		case MODEL_M102:
		case MODEL_M10:
			memedit_ctrl.pRegion->add("RAM");
			memedit_ctrl.pRegion->add("ROM");
			memedit_ctrl.pRegion->add("Opt ROM");
			memedit_ctrl.pRegion->value(0);
			break;

		case MODEL_T200:
			memedit_ctrl.pRegion->add("RAM 1");
			memedit_ctrl.pRegion->add("RAM 2");
			memedit_ctrl.pRegion->add("RAM 3");
			memedit_ctrl.pRegion->add("ROM");
			memedit_ctrl.pRegion->add("ROM 2");
			memedit_ctrl.pRegion->add("Opt ROM");
			memedit_ctrl.pRegion->value(0);
			break;

		case MODEL_PC8201:
			memedit_ctrl.pRegion->add("RAM 1");
			memedit_ctrl.pRegion->add("RAM 2");
			memedit_ctrl.pRegion->add("RAM 3");
			memedit_ctrl.pRegion->add("ROM");
			memedit_ctrl.pRegion->add("Opt ROM");
			memedit_ctrl.pRegion->value(0);
			break;
		}
	}
	if (gRampac)
		memedit_ctrl.pRegion->add("RamPac");
}

/*
================================================================
Sets the scroll bar sizes based on current region selection
================================================================
*/
void T100_MemEditor::SetScrollSize(void)
{
	double	size;
	int		height;
	int		region;

	// Test for ReMem emulation mode
	if (gReMem)
	{
		/* Test if the ReMem emulation is really REX */
		if (gRex)
		{
			region = memedit_ctrl.pRegion->value();
			if (gModel == MODEL_T200 || gModel == MODEL_PC8201)
			{
				switch (region)
				{
				case 0:
				case 1:
				case 2:
					m_Max = RAMSIZE / 16;
					break;
				case 3:
					m_Max = ROMSIZE / 16;
					break;
				case 4:
					if (gModel == MODEL_T200)
						m_Max = ROMSIZE / 16;
					else
						m_Max = 1024 * 1024 / 16;
					break;
				case 5:
					m_Max = 1024 * 1024 / 16;
					break;
				case 6:
					m_Max = 128 * 1024 / 16;			
					break;
				}
			}
			else
			{
				if (region == 0)
					m_Max = RAMSIZE / 16;
				else if (region == 1)
					m_Max = ROMSIZE / 16;
				else if (region == 2)
					m_Max = 1024 * 1024 / 16;			
				else if (region == 3)
					m_Max = 128 * 1024 / 16;			
			}
		}
		else
		{
			// All ReMem blocks are 2Meg = 128K lines * 16 bytes
			m_Max = 1024 * 2048 / 16;
		}
	}
	else
	{
		// Check if RAM or ROM region selected
		region = memedit_ctrl.pRegion->value();
		switch (gModel)
		{
		case MODEL_M100:
		case MODEL_KC85:
		case MODEL_M10:
		case MODEL_M102:
		case MODEL_PC8201:
			if (region == 0)
				m_Max = RAMSIZE / 16;
			else
				m_Max = ROMSIZE / 16;
			break;
			
		case MODEL_T200:
			if (region < 3)
				m_Max = RAMSIZE / 16;
			else
				m_Max = ROMSIZE / 16;
			break;
		}
	}
	// Check if RamPac RAM is selected
	if (strcmp(memedit_ctrl.pRegion->text(), "RamPac") == 0)
		m_Max = 1024 * 256 / 16;

	// Select 12 point Courier font
	fl_font(m_Font, m_FontSize);

	// Get character width & height
	m_Height = fl_height();
	height = memedit_ctrl.pScroll->h();
	size = (int) (height / m_Height);

	// Set maximum value of scroll 
	if (memedit_ctrl.pScroll->value() >= m_Max)
		memedit_ctrl.pScroll->value((int) (m_Max-size), 1, 0, (int) m_Max);
	memedit_ctrl.pScroll->maximum(m_Max-size);
	memedit_ctrl.pScroll->step(size / m_Max);
	memedit_ctrl.pScroll->slider_size(size/m_Max);
	memedit_ctrl.pScroll->linesize(1);
}

/*
================================================================
Returns the ENUM of the currently selected region list
================================================================
*/
int T100_MemEditor::GetRegionEnum(void)
{
	char	reg_text[16];

	// Get region text from control
	strcpy(reg_text, memedit_ctrl.pRegion->text());

	// Test if RAM region is selected
	if (strcmp(reg_text, "RAM") == 0)
	{
		if (gReMem && !gRex)
			m_Region = REGION_REMEM_RAM;
		else
			m_Region = REGION_RAM;
	}

	// Test if RAM 1 region is selecte
	if (strcmp(reg_text, "RAM 1") == 0)
		m_Region = REGION_RAM1;

	// Test if RAM 2 region is selecte
	if (strcmp(reg_text, "RAM 2") == 0)
		m_Region = REGION_RAM2;

	// Test if RAM 3 region is selecte
	if (strcmp(reg_text, "RAM 3") == 0)
		m_Region = REGION_RAM3;

	// Test if ROM region is selected
	if (strcmp(reg_text, "ROM") == 0)
		m_Region = REGION_ROM;

	// Test if OptROM region is selected
	if (strcmp(reg_text, "Opt ROM") == 0)
		m_Region = REGION_OPTROM;

	// Test if ROM2 region is selected
	if (strcmp(reg_text, "ROM 2") == 0)
		m_Region = REGION_ROM2;

	// Test if RAMPAC region is selected
	if (strcmp(reg_text, "RamPac") == 0)
		m_Region = REGION_RAMPAC;

	// Test if Flash 1 region is selected
	if (strcmp(reg_text, "Flash 1") == 0)
		m_Region = REGION_FLASH1;

	// Test if Flash 2 region is selected
	if (strcmp(reg_text, "Flash 2") == 0)
		m_Region = REGION_FLASH2;

	// Test if REX Flash region is selected
	if (strcmp(reg_text, "Flash") == 0)
		m_Region = REGION_REX_FLASH;

	// Test if REX Flash region is selected
	if (strcmp(reg_text, "128K SRAM") == 0)
		m_Region = REGION_REX2_RAM;

	return m_Region;

}

/*
================================================================
Called when memory changes to redraw any regions which have
changed.
================================================================
*/
void T100_MemEditor::UpdateDispMem(void)
{
	int				count, c;
	unsigned char	mem[8192];
	int				xpos, ypos, selWidth, addrBase;
	char			string[6];
	int				modified = 0;
	int				linesVisible;

	// Calculate # bytes to read
	count = m_Lines * 16;

	// Read memory 
	get_memory8_ext(m_Region, m_FirstAddress, count, mem);
	addrBase = 0;
	if ((m_Region == REGION_RAM) || (m_Region == REGION_RAM2) || 
		(m_Region == REGION_RAM3) || m_Region == REGION_RAM1)
			addrBase = ROMSIZE;

	// Set the display
	window()->make_current();

	// Select 12 point Courier font
	fl_font(m_Font, m_FontSize);
	linesVisible = (int) (memedit_ctrl.pScroll->h() / m_Height);

	// Clip any drawing
	fl_push_clip(x(), y(), w(), h());

	// Check for changes
	for (c = 0; c < count; c++)
	{
		if (mem[c] != gDispMemory[c])
		{
			int addr = m_FirstAddress + c;
			int actualAddr = addr + addrBase;
			gDispMemory[c] = mem[c];

			// Display new text
			sprintf(string, "%02X", mem[c]);

			// Calculate xpos and ypos of the text
			xpos = (int) (x() + ((c&0x0F)*3+9+((c&0x0F)>7)) * m_Width);
			ypos = (int) (y() + ((c>>4)+1) * m_Height);
			
			// Erase the old text
			fl_color(m_colors.background);
			fl_rectf(xpos, ypos+2 - (int) m_Height, (int) m_Width*2, (int) m_Height);
			
			// Draw as disabled RAM region?
			if (actualAddr >= ROMSIZE && actualAddr < gRamBottom)
			{
				fl_color(fl_darker(FL_GRAY));
				fl_draw(string, xpos, ypos-2);
			}

			// Draw as marker / selection?
			else if (IsHilighted(addr))
			{
				selWidth = 3;
				if ((c & 0x0F) == 7 || (c & 0x0F) == 15)
					selWidth = 2;
				if (!IsHilighted(addr+c+1))
					selWidth = 2;
				if (IsSelected(addr))
					fl_color(m_colors.select_background);
				else
				    fl_color(m_colors.hilight_background);
				fl_rectf(xpos, ypos+2-(int) m_Height, (int) m_Width * selWidth, (int) m_Height);
				if (IsSelected(addr))
					fl_color(m_colors.select);
				else
					fl_color(m_colors.hilight);
				fl_draw(string, xpos, ypos-2);
				fl_color(m_colors.number);
			}
			else
			{
				// Draw HEX value for changed item
				fl_color(m_colors.number);
				fl_draw(string, xpos, ypos-2);
			}

			// Draw the new ASCII value
			if ((mem[c] >= ' ') && (mem[c] <= '~'))
				sprintf(string, "%c", mem[c]);
			else
				strcpy(string, ".");
			xpos = (int) (x() + (59 + (c&0x0F)) * m_Width);

			// Erase the old text
			fl_color(m_colors.background);
			fl_rectf(xpos, ypos+2 - (int) m_Height, (int) m_Width, (int) m_Height+3);

			// Erase spill-over text below this field
			if (!(actualAddr+16 >= ROMSIZE && actualAddr+16 < gRamBottom))
			{
				if (IsHilighted(addr+16))
				{
					if (IsSelected(addr+16))
						fl_color(m_colors.select_background);
					else
						fl_color(m_colors.hilight_background);
				}
			}

			// Draw spill over region only if not on bottom line
			if ((c+16) / 16 < linesVisible)
				fl_rectf(xpos, ypos+2, (int) m_Width, 3);	

			// Draw as disabled RAM region?
			if (actualAddr >= ROMSIZE && actualAddr < gRamBottom)
			{
				fl_color(fl_darker(FL_GRAY));
				fl_draw(string, xpos, ypos-2);
			}

			// Draw as marker / selection?
			else if (IsHilighted(addr))
			{
				if (IsSelected(addr))
					fl_color(m_colors.select_background);
				else
				    fl_color(m_colors.hilight_background);
				fl_rectf(xpos, ypos+2-(int) m_Height, (int) m_Width, (int) m_Height);
				if (IsSelected(addr))
					fl_color(m_colors.select);
				else
					fl_color(m_colors.hilight);
				fl_draw(string, xpos, ypos-2);
				fl_color(m_colors.number);
			}
			else
			{
				fl_color(m_colors.ascii);
				fl_draw(string, xpos, ypos-2);
			}

			// Set modified flag
			modified = 1;
		}
	}

	if (modified)
		DrawCursor();

	// Pop the clippping region
	fl_pop_clip();
}

/*
================================================================
Redraw the whole memory editor widget
================================================================
*/
void T100_MemEditor::draw()
{
	int				lines, cols;
	int				line;
	unsigned char	line_bytes[16];		// Bytes for line being displayed
	int				addr, actualAddr;
	char			string[10];
	int				xpos, ypos;
	int				c, max, selWidth;
	int				region;				// Region where memory is being drawn from

	// Select 12 point Courier font
	fl_font(m_Font, m_FontSize);

	// Calculate max cols and lines
	lines = (int) (h() / m_Height);
	cols = (int) (w() / m_Width);

	if ((lines != m_Lines) || (cols != m_Cols))
	{
		m_Lines = lines;
		m_Cols = cols;
		SetScrollSize();
	}

	// Draw white background
    fl_color(m_colors.background);
    fl_rectf(x(),y(),w(),h());
	fl_push_clip(x(), y(), w(), h());

	line = 0;
	max = (int) m_Max * 16;		// Calc maximum address

	m_FirstAddress = memedit_ctrl.pScroll->value() * 16;

	// Calculate region where memory is read from
	region = GetRegionEnum();

	// Read memory 
	get_memory8_ext(region, m_FirstAddress, m_Lines*16, gDispMemory);

	// Loop through all lines on display
	while (line < m_Lines)
	{
		// Calculate address to be displayed
		addr = m_FirstAddress + line * 16;
		actualAddr = addr;
		if (addr >= max)
			break;
	    fl_color(m_colors.addr);
		if ((region == REGION_RAM) || (region == REGION_RAM2) || 
			(region == REGION_RAM3) || region == REGION_RAM1)
		{
			sprintf(string, "%06X  ", addr + RAMSTART);
			actualAddr += RAMSTART;
		}
		else
			sprintf(string, "%06X  ", addr);

		// Calculate xpos and ypos of the text
		xpos = (int) (x() + m_Width);
		ypos = (int) (y() + (line+1) * m_Height);
		
		if (actualAddr >= ROMSIZE && actualAddr < gRamBottom)
			fl_color(fl_darker(FL_GRAY));

		// Draw Address text for line
		fl_draw(string, xpos, ypos-2);

		// Read memory for this line
		get_memory8_ext(region, addr, 16, line_bytes);

		// Set the color for numbers
	    fl_color(m_colors.number);

		// Loop through 16 bytes and display in HEX
		for (c = 0; c < 16; c++)
		{
			// Calculate xpos for this byte
			if (c < 8)
				xpos = (int) (x() + (c*3 + 9) * m_Width);
			else
				xpos = (int) (x() + (c*3 + 10) * m_Width);

			sprintf(string, "%02X", line_bytes[c]);

			if (xpos + m_Width < w())
			{
				if (actualAddr >= ROMSIZE && actualAddr < gRamBottom)
				{
					fl_color(fl_darker(FL_GRAY));
					fl_draw(string, xpos, ypos-2);
				}

				// Draw selection box
				else if (IsHilighted(addr+c))
				{
					selWidth = 3;
					if (c == 7 || c == 15)
						selWidth = 2;
					if (!IsHilighted(addr+c+1))
						selWidth = 2;
					if (IsSelected(addr+c))
					{
						fl_color(m_colors.select_background);
						fl_rectf(xpos, ypos+2-(int) m_Height, (int) m_Width * selWidth, (int) m_Height);
						fl_color(m_colors.select);
					}
					else
					{
					    fl_color(m_colors.hilight_background);
						fl_rectf(xpos, ypos+2-(int) m_Height, (int) m_Width * selWidth, (int) m_Height);
						fl_color(m_colors.hilight);
					}
					fl_draw(string, xpos, ypos-2);
					fl_color(m_colors.number);
				}
				else
					fl_draw(string, xpos, ypos-2);
			}
		}

		// Calculate xpos for first byte
		xpos = (int) (x() + (16*3 + 11) * m_Width);

		string[1] = 0;

		// Set the color for ascii
	    fl_color(m_colors.ascii);

		// Loop through 16 bytes and display as text
		for (c = 0; c < 16; c++)
		{
			// Determine character to display
			if ((line_bytes[c] < 0x20) || (line_bytes[c] > '~'))
				string[0] = '.';
			else
				string[0] = line_bytes[c];

			// Print the character
			if (xpos < w())
			{
				if (actualAddr >= ROMSIZE && actualAddr < gRamBottom)
				{
					fl_color(fl_darker(FL_GRAY));
					fl_draw(string, xpos, ypos-2);
				}

				// Draw selection box
				else if (IsHilighted(addr + c))
				{
					if (IsSelected(addr+c))
					{
						fl_color(m_colors.select_background);
						fl_rectf(xpos, ypos+2-(int) m_Height, (int) m_Width, (int) m_Height);
						fl_color(m_colors.select);
					}
					else
					{
					    fl_color(m_colors.hilight_background);
						fl_rectf(xpos, ypos+2-(int) m_Height, (int) m_Width, (int) m_Height);
						fl_color(m_colors.hilight);
					}
					fl_draw(string, xpos, ypos-2);
					fl_color(m_colors.ascii);
				}
				else
					fl_draw(string, xpos, ypos-2);
			}

			// Calculate xpos of next byte 
			xpos += (int) m_Width;
		}

		// Increment Line number
		line++;
	}

	if (m_MyFocus)
		DrawCursor();

	// Pop the clipping rect
	fl_pop_clip();
}

/*
================================================================
Handle mouse events, key events, focus events, etc.
================================================================
*/
int T100_MemEditor::handle(int event)
{
	int				c, xp, yp, shift;
	int				line_click, col_click, cnt;
	int				col, line;
	int				lineIndex;
	int				height, size;
	int				value;
	unsigned int	key;
	int				first, address, prevAddress;
	unsigned char	data;
	int				xpos, ypos, actualAddr;
	char			string[6];

	// Do some common processing before the switch
	if (event == FL_PUSH || event == FL_MOVE || event == FL_DRAG)
	{
		// Get X,Y position of button press
		xp = Fl::event_x();
		yp = Fl::event_y();

		// Check if Shift was depressed during the Mouse Button event
		if (Fl::get_key(FL_Shift_L) || Fl::get_key(FL_Shift_R))
			shift = 1;
		else
			shift = 0;

		// Determine line & col of mouse click
		col_click = (int) ((xp - x()) / m_Width);
		line_click = (int) ((yp - y()) / m_Height);
		height = memedit_ctrl.pScroll->h();
		size = (int) (height / m_Height);
		if (line_click >= size)
			line_click--;

		// Constrain Column base on geometry
		if (col_click < 9)
			col_click = 9;

		// Constrain click to 1st column in of value in first field
		if (col_click < 24+9)
			col_click = (int) ((col_click-9) / 3) * 3 + 9;
		else if (col_click < 58)
			col_click = (int) ((col_click-25-9) / 3) * 3 + 25+9;
		else if (col_click == 58)
			col_click = 59;
		else if (col_click > 74)
			col_click = 74;
	}

	// Save current cursor address in case the selection changes
	prevAddress = m_CursorAddress;

	switch (event)
	{
	case FL_FOCUS:
		m_MyFocus = 1;
		if (m_PopupActive)
		{
			m_PopupActive = FALSE;
			break;
		}
		else if (m_SelStartAddr != -1)
		{
			m_SelStartAddr = m_SelEndAddr = -1;
			redraw();
		}
		else if (m_CursorRow != -1 && m_CursorCol != -1)
			DrawCursor();
		break;

	case FL_UNFOCUS:
		m_MyFocus = 0;

		// Make our window the current window for drawing
		window()->make_current();

		// "Unselect" previous start or stop char
		EraseCursor();
		break;

	case FL_RELEASE:
		// Release the pointer
		if (m_HaveMouse)
		{
			Fl::release();
			m_HaveMouse = FALSE;
		}

		// Ensure the SelStart less than SelEnd
		if (m_SelStartAddr > m_SelEndAddr)
		{
			// Swap them
			address = m_SelStartAddr;
			m_SelStartAddr = m_SelEndAddr;
			m_SelEndAddr = address;
		}
		break;

	case FL_MOVE:
	case FL_DRAG:
		// Cancel any double click operation possibilities
		if (xp != m_DblclkX || yp != m_DblclkY)
		{
			m_DblclkX = -1;
			m_DblclkY = -1;
		}

		// Done processing if we don't have the mouse
		if (!m_HaveMouse)
			break;

		// Test if the mouse actually moved from the last position
		if ((abs(xp - m_DragX) > 4) || (abs(yp - m_DragY) > 4))
		{
			// Test for an active selection.  If no active selection, then 
			// create one.
			if (m_SelStartAddr == -1)
				m_SelStartAddr = m_CursorAddress;

			// Calculate the address of the new x,y location and set it as
			// the selection end address.
			if (col_click >= 59)
				col = (int) (col_click-59);
			else if (col_click > 8+8*3)
				col = (int) ((col_click-10) / 3);
			else 
				col = (int) ((col_click-9) / 3);

			// Calculate the address associated with the click
			address = memedit_ctrl.pScroll->value() * 16;
			address += line_click * 16;
			address += col;

			// Set address as the selection end
			m_SelEndAddr = address;

			// Perform a redraw to show new selection
			redraw();
		}
		break;

	case FL_PUSH:
		// Get id of mouse button pressed
		c = Fl::event_button();

		// We must take the focus so keystrokes work properly
		if (Fl::focus() != this)
			take_focus();

		// Make our window the current window for drawing
		window()->make_current();

		// Check if it was the Left Mouse button
		if (c == FL_LEFT_MOUSE)
		{
			// Take control of the cursor
			Fl::grab();
			m_HaveMouse = TRUE;
			m_DragX = xp;
			m_DragY = yp;

			// Get LineStart index
			lineIndex = (m_FirstLine + line_click) >> 1;

			cnt = 0;
			col = 0;
			line = lineIndex << 1;

			// "Unselect" previous start or stop char
			window()->make_current();

			// Select 12 point Courier font
			fl_font(m_Font, m_FontSize);

			// Erase current cursor
			EraseCursor();

			// Save new cursor position
			int oldCursorCol = m_CursorCol;
			int oldCursorRow = m_CursorRow;

			m_CursorCol = col_click;
			m_CursorRow = line_click;
			if (col_click >= 59)
				m_CursorField = 2;
			else
				m_CursorField = 1;

			if (m_CursorField == 1)
			{
				// Calculate the byte number within the field
				if (m_CursorCol > 8+8*3)
				{
					// Calculate index of byte 
					col = (int) ((m_CursorCol-10) / 3);
				}
				else 
				{
					// Calculate index of byte
					col = (int) ((m_CursorCol-9) / 3);
				}
			}
			else
			{
				col = (int) (m_CursorCol-59);
			}

			// Calculate the address associated with the click
			address = memedit_ctrl.pScroll->value() * 16;
			address += m_CursorRow * 16;
			address += col;

			// Save this address for future use
			m_CursorAddress = address;

			// Process as a double click if the coords are the same
			if ((xp == m_DblclkX && yp == m_DblclkY) || shift)
			{
				// Set the selection to the click upon address
				m_SelEndAddr = address;

				// Test for shift click and selStart not set yet
				if (shift && m_SelStartAddr == -1)
				{
					// Set the start address based on old cursor position
					if (oldCursorCol >= 59)
						col = oldCursorCol - 59;
					else if (oldCursorCol > 8+8*3) 
						col = (int) ((oldCursorCol-10) / 3);
					else
						col = (int) ((oldCursorCol-9) / 3);

					// Calculate address of last cursor loction
					address = memedit_ctrl.pScroll->value() * 16;
					address += oldCursorRow * 16;
					address += col;

					// Use this address as the SelStart address
					m_SelStartAddr = address;
				}
				else
					m_SelStartAddr = address;

				// Ensure start is less than end
				if (m_SelStartAddr > m_SelEndAddr)
				{
					address = m_SelStartAddr;
					m_SelStartAddr = m_SelEndAddr;
					m_SelEndAddr = address;
				}

				// Cancel double click for next click operation
				m_DblclkX = -1;
				m_DblclkY = -1;

				redraw();
				break;	
			}
			else if (!(address >= m_SelStartAddr && address <= m_SelEndAddr))
			{
				// We clicked in a region that wasn't selected.  Clear
				// the old selection.
				m_SelStartAddr = m_SelEndAddr = -1;
				redraw();
			}

			// Draw new cursor
			DrawCursor();

			// Save x and y as the last click location
			m_DblclkX = xp;
			m_DblclkY = yp;
		}
		else if (c == FL_RIGHT_MOUSE)
		{
			// Calculate the address of the new x,y location and set it as
			// the selection end address.
			if (col_click >= 59)
				col = (int) (col_click-59);
			else if (col_click > 8+8*3)
				col = (int) ((col_click-10) / 3);
			else 
				col = (int) ((col_click-9) / 3);

			// Calculate the address associated with the click
			address = memedit_ctrl.pScroll->value() * 16;
			address += line_click * 16;
			address += col;

			// Test if we right-clicked outside the selection area
			if (!(address >= m_SelStartAddr && address <= m_SelEndAddr))
			{
				// Create a new selection area at the current address and redraw
				m_SelStartAddr = address;
				m_SelEndAddr = address;
			}
			m_CursorCol = col_click;
			m_CursorRow = line_click;
			m_CursorAddress = address;
			redraw();

			// If we selected inside an existing marker, then show the delete marker menu
			if (HasMarker(address))
				// Show the "Remove Marker" menu
				m_pPopupMenu->menu(gDeleteMarkerMenu);
			else
				// Show the "Create Marker" menu
				m_pPopupMenu->menu(gAddMarkerMenu);

			// Set or clear the Undo based on availablilty
			int count = sizeof(gDeleteMarkerMenu) / sizeof(Fl_Menu_Item);
			int region = m_Region;
			if (region == REGION_RAM1 || region == REGION_RAM2 || region == REGION_RAM3)
				region = REGION_RAM;
			if (m_pUndoDeleteMarkers[region] == NULL)
			{
				gDeleteMarkerMenu[count-2].text = NULL;
				gAddMarkerMenu[count-2].text = NULL;
			}
			else
			{
				gDeleteMarkerMenu[count-2].text = "  Undo Delete all  ";
				gAddMarkerMenu[count-2].text = "  Undo Delete all  ";
			}

			// Now Popup the menu
			m_PopupActive = TRUE;
			Fl_Widget *mb = m_pPopupMenu;
			const Fl_Menu_Item* m;
			Fl::watch_widget_pointer(mb);
			m = m_pPopupMenu->menu()->popup(Fl::event_x(), y()+(line_click+1)*(int)m_Height+3, NULL, 0, m_pPopupMenu);
			m_pPopupMenu->picked(m);
			if (m == NULL)
				cb_popup(NULL, NULL);
			Fl::release_widget_pointer(mb);
		}
		break;

	case FL_KEYDOWN:
		if (!m_MyFocus)
			return 1;

		// Get the Key that was pressed
		key = Fl::event_key();

		// Check if Shift was depressed during the Mouse Button event
		if (Fl::get_key(FL_Shift_L) || Fl::get_key(FL_Shift_R))
			shift = 1;
		else
			shift = 0;

		// Make our window the current window for drawing
		window()->make_current();

		switch (key)
		{
		case FL_F + 10:
			cb_menu_step_over(NULL, NULL);
			break;
		case FL_F + 8:
			cb_menu_step(NULL, NULL);
			break;
		case FL_F + 5:
			cb_menu_run(NULL, NULL);
			break;
		case FL_F + 6:
			cb_menu_stop(NULL, NULL);
			break;
		case FL_F+7:
		case FL_F+2:
			// Calculate the address associated with the click
			if (m_CursorCol >= 59)
				col = m_CursorCol - 59;
			else if (m_CursorCol > 8+8*3) 
				col = (int) ((m_CursorCol-10) / 3);
			else
				col = (int) ((m_CursorCol-9) / 3);

			m_CursorAddress = memedit_ctrl.pScroll->value() * 16;
			m_CursorAddress += m_CursorRow * 16;
			m_CursorAddress += col;
			if (shift)
				FindPrevMarker();
			else
				FindNextMarker();
			break;

		case FL_Left:
			// First test if selection active. If active, dactivate the selection
			// and place cursor at m_CursorRow,m_CursorCol
			if (m_SelActive)
			{
				// Call routine to "unhilight" the selection

				// Deactivate the selection
				m_SelActive = 0;

				// Draw the cursor at the m_Cursor position
				DrawCursor();
			}
			else
			{
				// Call routine to hide current cursor
				EraseCursor();

				// Move cursor left 1 position in current field
				if (m_CursorField == 1)
				{
					col = m_CursorCol-1;

					// Test if cursor on leftmost entry
					if (col < 9)
					{
						if (m_CursorRow != 0)
						{
							m_CursorRow--;
							col = 10 + 15 * 3;
						}
						else
						{
							// Test if we can scroll up one line
							if (memedit_ctrl.pScroll->value() != 0)
							{
								col = 10 + 15 * 3;
								ScrollUp(1);
							}
							else
								col = m_CursorCol;
						}
					} else if (col < 25+9)
						col = (int) ((col-10) / 3) * 3 + 9;
					else 
						col = (int) ((col-25-9) / 3) * 3 + 25+9;

					m_CursorCol = col;
				}
				else
				{
					// Subtract 1 from cursor position & test if out of field
					if (--m_CursorCol < 59)
					{
						if (m_CursorRow != 0)
						{
							m_CursorRow--;
							m_CursorCol = 74;
						}
						else
						{
							// Test if we can scroll up one line
							if (memedit_ctrl.pScroll->value() != 0)
							{
								m_CursorCol = 74;
								ScrollUp(1);
							}
							else
								m_CursorCol++;
						}

					}
				}

				DrawCursor();
			}
			break;
		case FL_Right:
			// First test if selection active. If active, dactivate the selection
			// and place cursor at m_CursorRow,m_CursorCol
			if (m_SelActive)
			{
				// Call routine to "unhilight" the selection

				// Deactivate the selection
				m_SelActive = 0;

				// Make SelEnd the Cursor position
				m_CursorRow = m_SelEndRow;
				m_CursorCol = m_SelEndCol;

				// Draw the cursor at the m_Cursor position
				DrawCursor();
			}
			else
			{
				// Call routine to hide current cursor
				EraseCursor();

				// Move cursor left 1 position in current field
				if (m_CursorField == 1)
				{
					col = m_CursorCol+3;

					// Test if cursor on leftmost entry
					if (col > 9+16*3)
					{
						if (m_CursorRow != m_Lines-1)
						{
							m_CursorRow++;
							col = 9;
						}
						else
						{
							// Select 12 point Courier font
							fl_font(m_Font, m_FontSize);

							// Get character width & height
							height = memedit_ctrl.pScroll->h();
							size = (int) (height / m_Height);
							if (memedit_ctrl.pScroll->value() != m_Max - size)
							{
								col = 9;
								ScrollUp(-1);
							}
							else
								col = m_CursorCol;
						}
					} else if (col > 8+8*3)
						col = (int) ((col-25-9) / 3) * 3 + 25+9;
					else 
						col = (int) ((col-9) / 3) * 3 + 9;

					m_CursorCol = col;
				}
				else
				{
					// Subtract 1 from cursor position & test if out of field
					if (++m_CursorCol > 74)
					{
						if (m_CursorRow != m_Lines-1)
						{
							m_CursorRow++;
							m_CursorCol = 59;
						}
						else
						{
							// Select 12 point Courier font
							fl_font(m_Font, m_FontSize);

							// Get character width & height
							height = memedit_ctrl.pScroll->h();
							size = (int) (height / m_Height);
							if (memedit_ctrl.pScroll->value() != m_Max - size)
							{
								m_CursorCol = 59;
								ScrollUp(-1);
							}
							else
								m_CursorCol--;
						}

					}
				}

				// Draw the cursor at the m_Cursor position
				DrawCursor();
			}
			break;
		case FL_Up:
			// First test if selection active. If active, dactivate the selection
			// and place cursor at m_CursorRow,m_CursorCol
			if (m_SelActive)
			{
				// Call routine to "unhilight" the selection

				// Deactivate the selection
				m_SelActive = 0;

				// Make SelEnd the Cursor position
				m_CursorRow = m_SelEndRow;
				m_CursorCol = m_SelEndCol;

				// Draw the cursor at the m_Cursor position
				DrawCursor();
			}
			else
			{
				// Call routine to hide current cursor
				EraseCursor();

				// Update row
				if (m_CursorRow != 0)
					m_CursorRow--;
				else
				{
					ScrollUp(1);
					DrawCursor();
					break;
				}

				// Update Col so it is on 1st byte of field
				if (m_CursorField == 1)
				{
					col = m_CursorCol;

					if (col > 8+8*3)
						col = (int) ((col-25-9) / 3) * 3 + 25+9;
					else 
						col = (int) ((col-9) / 3) * 3 + 9;

					m_CursorCol = col;
				}

				// Draw the cursor at the m_Cursor position
				DrawCursor();
			}

			break;
		case FL_Down:
			// First test if selection active. If active, dactivate the selection
			// and place cursor at m_CursorRow,m_CursorCol
			if (m_SelActive)
			{
				// Call routine to "unhilight" the selection

				// Deactivate the selection
				m_SelActive = 0;

				// Draw the cursor at the m_Cursor position
				DrawCursor();
			}
			else
			{
				// Call routine to hide current cursor
				EraseCursor();

				// Update row
				if (m_CursorRow != m_Lines - 1)
					m_CursorRow++;
				else
				{
					ScrollUp(-1);
					DrawCursor();
					break;
				}

				// Update Col so it is on 1st byte of field
				if (m_CursorField == 1)
				{
					col = m_CursorCol;

					if (col > 8+8*3)
						col = (int) ((col-25-9) / 3) * 3 + 25+9;
					else 
						col = (int) ((col-9) / 3) * 3 + 9;

					m_CursorCol = col;
				}

				// Draw the cursor at the m_Cursor position
				DrawCursor();
			}

			break;

		case FL_Home:
			// Erase current cursor
			EraseCursor();

			// Test if CTRL key is pressed
			if ((Fl::get_key(FL_Control_L) | Fl::get_key(FL_Control_R)) != 0)
			{
				// Ctrl key pressed go to very top
				ScrollUp((int) m_Max);
				m_CursorRow = 0;
			}

			// Move cursor to beginning of field
			m_CursorCol = m_CursorField == 1 ? 9 : 59;

			// Draw the cursor at the m_Cursor position
			DrawCursor();
			break;

		case FL_End:
			// Erase current cursor
			EraseCursor();

			if ((Fl::get_key(FL_Control_L) | Fl::get_key(FL_Control_R)) != 0)
			{
				// Calculate scroll distance to end
				value = memedit_ctrl.pScroll->value();
				height = memedit_ctrl.pScroll->h();
				size = (int) (height / m_Height);
				m_CursorRow = size-1;
				size = (int) m_Max - size - value;

				ScrollUp(-size);
			}

			// Move cursor to end of row
			m_CursorCol = m_CursorField == 1 ? 55 : 74;

			// Draw the cursor at the m_Cursor position
			DrawCursor();
			break;

		case FL_Page_Up:
			// Erase current cursor
			EraseCursor();

			// Test if already at top of memory
			if ((value = memedit_ctrl.pScroll->value()) == 0)
			{
				m_CursorRow = 0;

				// Move cursor to beginning of field
				m_CursorCol = m_CursorField == 1 ? 9 : 59;
			}

			height = memedit_ctrl.pScroll->h();
			size = (int) (height / m_Height);

			if ((value < size) && (value != 0))
				m_CursorRow -= size - value;

			ScrollUp(size);

			// Draw the cursor
			DrawCursor();
			break;

		case FL_Page_Down:
			// Erase current cursor
			EraseCursor();

			// Calculate distance of scroll			
			height = memedit_ctrl.pScroll->h();
			size = (int) (height / m_Height);

			// Test if on last page already
			value = memedit_ctrl.pScroll->value();
			if (value == (int) m_Max - size)
			{
				// Move cursor to beginning of field
				m_CursorCol = m_CursorField == 1 ? 9 : 59;
				m_CursorRow = size-1;
			}

			// Test scroll size for bounds
			if (value + size > m_Max - size)
				size = (int) m_Max - size - value;

			ScrollUp(-size);

			// Draw the cursor
			DrawCursor();

			break;

		case FL_Tab:
			// Erase current cursor
			EraseCursor();

			// If shift-tab selected, then change focus to Address edit field
			if (shift)
			{
				memedit_ctrl.pMemRange->take_focus();
				memedit_ctrl.pMemRange->position(0);
				memedit_ctrl.pMemRange->mark(9999);
				return 1;
			}

			// Change to other field
			if (m_CursorField == 1)
			{
				// Change to ASCII field
				m_CursorField = 2;

				// Calculate absolute position (0 to 15)
				if (m_CursorCol > 8+8*3)
					col = (int) ((m_CursorCol-10) / 3);
				else 
					col = (int) ((m_CursorCol-9) / 3);

				// Position cursor in new field
				m_CursorCol = 59 + col;
			}
			else
			{
				// Change to HEX field
				m_CursorField = 1;

				// Calculate absolution position (0 to 15)
				col = m_CursorCol - 59;

				// Position cursor in new field
				if (col > 7)
					m_CursorCol = col * 3 + 10;
				else 
					m_CursorCol = col * 3 + 9;
			}
			// Draw the cursor
			DrawCursor();

			break;

		default:
			if (Fl::get_key(FL_Control_L) || Fl::get_key(FL_Control_R))
			{
				if (key == 'g' || key=='G')
				{
					memedit_ctrl.pMemRange->take_focus();
					memedit_ctrl.pMemRange->position(0);
					memedit_ctrl.pMemRange->mark(9999);
				}
				return 1;
			}

			// Handle changes to the memory 
			if (m_CursorField == 1)
			{
				// Check for ASCII keystrokes
				if (((key >= '0') && (key <= '9')) || ((key >= 'a') && (key <= 'f')) ||
					((key >= 'A') && (key <= 'F')))
				{
					// Erase current cursor
					EraseCursor();

					// Process keystroke -- first determine address being modified
					if (m_CursorCol > 8+8*3)
					{
						// Calculate index of byte 
						col = (int) ((m_CursorCol-10) / 3);

						// Test if cursor at beginning of byte or in middle
						if ((int)((m_CursorCol - 25 - 9) / 3) * 3 == m_CursorCol-25-9)
							first = 1;
						else
							first = 0;
					}
					else 
					{
						// Calculate index of byte
						col = (int) ((m_CursorCol-9) / 3);

						// Test if cursor at beginning of byte or in middle
						if ((int)((m_CursorCol - 9) / 3) * 3 == m_CursorCol-9)
							first = 1;
						else
							first = 0;
					}

					// Get address of first item on screen
					address = memedit_ctrl.pScroll->value() * 16;
					address += m_CursorRow * 16;
					address += col;

					actualAddr = address;
					if ((m_Region == REGION_RAM) || (m_Region == REGION_RAM2) || 
						(m_Region == REGION_RAM3) || m_Region == REGION_RAM1)
							actualAddr += ROMSIZE;

					// Determine if this is the first keystroke at this address
					if (first)
					{
						data = 0;
						set_memory8_ext(m_Region, address, 1, &data);
						gDispMemory[actualAddr] = data;
					}

					// Determine value of keystroke (convert from HEX)
					if (key <= '9')
						value = key - '0';
					else if (key <= 'F')
						value = key - 'A' + 10;
					else 
						value = key - 'a' + 10;

					// Get current memory value
					get_memory8_ext(m_Region, address, 1, &data);
					data = data * 16 + value;

					// Write new value to memory
					set_memory8_ext(m_Region, address, 1, &data);
					gDispMemory[actualAddr] = data;

					// Display new text
					// Select 12 point Courier font
					fl_font(m_Font, m_FontSize);

					sprintf(string, "%02X", data);

					// Calculate xpos and ypos of the text
					xpos = (int) (x() + (col*3+9+(col>7)) * m_Width);
					ypos = (int) (y() + (m_CursorRow+1) * m_Height);
					
					// Set the display
					window()->make_current();

					// Erase old HEX value
					if (actualAddr >= ROMSIZE && actualAddr < gRamBottom)
						fl_color(m_colors.background);
					else if (IsHilighted(address))
					{
						if (IsSelected(address))
							fl_color(m_colors.select_background);
						else
							fl_color(m_colors.hilight_background);
					}
					else
						fl_color(m_colors.background);
					fl_rectf(xpos, ypos+2 - (int) m_Height, (int) m_Width*2, (int) m_Height-2);

					// Draw Draw the HEX value
					if (actualAddr >= ROMSIZE && actualAddr < gRamBottom)
						fl_color(fl_darker(FL_GRAY));
					else if (IsHilighted(address))
					{
						if (IsSelected(address))
							fl_color(m_colors.select);
						else
							fl_color(m_colors.hilight);
					}
					else
						fl_color(m_colors.number);
					fl_draw(string, xpos, ypos-2);

					// Prepare the new ASCII value to be drawn
					if ((data >= ' ') && (data <= '~'))
						sprintf(string, "%c", data);
					else
						strcpy(string, ".");
					xpos = (int) (x() + (59 + col) * m_Width);

					// Erase the old ASCII value
					if (actualAddr >= ROMSIZE && actualAddr < gRamBottom)
						fl_color(m_colors.background);
					else if (IsHilighted(address))
					{
						if (IsSelected(address))
							fl_color(m_colors.select_background);
						else
							fl_color(m_colors.hilight_background);
					}
					else
						fl_color(m_colors.background);
					fl_rectf(xpos, ypos+2 - (int) m_Height, (int) m_Width, (int) m_Height);

					// Draw the new ASCII value
					if (actualAddr >= ROMSIZE && actualAddr < gRamBottom)
						fl_color(fl_darker(FL_GRAY));
					else if (IsHilighted(address))
					{
						if (IsSelected(address))
							fl_color(m_colors.select);
						else
							fl_color(m_colors.hilight);
					}
					else
						fl_color(m_colors.ascii);
					fl_draw(string, xpos, ypos-2);

					// Update cursor position
					m_CursorCol++;
					if (!first)
					{
						// Skip the white space between bytes
						m_CursorCol++;

						if (m_CursorCol == 9+8*3)
							m_CursorCol = 10+8*3;

						// Test if cursor on leftmost entry
						else if (m_CursorCol > 9+16*3)
						{
							// Test if cursor at bottom of screen
							if (m_CursorRow != m_Lines-1)
							{
								// Not at bottom, simply increment to next line
								m_CursorRow++;
								m_CursorCol = 9;
							}
							else
							{
								// Cursor at bottom of screen.  Need to scroll 

								// Select 12 point Courier font
								fl_font(m_Font, m_FontSize);

								// Get character width & height
								height = memedit_ctrl.pScroll->h();
								size = (int) (height / m_Height);

								// Test if scroll at bottom
								if (memedit_ctrl.pScroll->value() != m_Max - size)
								{
									// Not at bottom.  Scroll 1 line
									m_CursorCol = 9;
									ScrollUp(-1);
								}
								else
									m_CursorCol = 55;
							}
						}
					}

					// Draw new cursor
					DrawCursor();

				}
			}
			else
			{
				key = *(Fl::event_text());

				// In ASCII field.  Test for valid keystorke
				if ((key >= ' ') && (key <= '~'))
				{
					// Erase current cursor
					EraseCursor();

					// Calculate index of byte 
					col = (int) m_CursorCol-59;

					// Get address of first item on screen
					address = memedit_ctrl.pScroll->value() * 16;
					address += m_CursorRow * 16;
					actualAddr = address;
					if ((m_Region == REGION_RAM) || (m_Region == REGION_RAM2) || 
						(m_Region == REGION_RAM3) || m_Region == REGION_RAM1)
							actualAddr += ROMSIZE;
					address += col;

					// Write new value to memory
					data = key;
					set_memory8_ext(m_Region, address, 1, &data);

					// Display new text
					// Select 12 point Courier font
					fl_font(m_Font, m_FontSize);

					// Calculate xpos and ypos of the text
					xpos = (int) (x() + (col*3+9+(col>7)) * m_Width);
					ypos = (int) (y() + (m_CursorRow+1) * m_Height);
					
					// Set the display active for drawing
					window()->make_current();

					// Erase old HEX value
					if (actualAddr >= ROMSIZE && actualAddr < gRamBottom)
						fl_color(m_colors.background);
					else if (IsHilighted(address))
					{
						if (IsSelected(address))
							fl_color(m_colors.select_background);
						else
							fl_color(m_colors.hilight_background);
					}
					else
						fl_color(m_colors.background);
					fl_rectf(xpos, ypos+2 - (int) m_Height, (int) m_Width*2, (int) m_Height-2);

					// Draw Draw the HEX value
					sprintf(string, "%02X", data);
					if (actualAddr >= ROMSIZE && actualAddr < gRamBottom)
						fl_color(fl_darker(FL_GRAY));
					else if (IsHilighted(address))
					{
						if (IsSelected(address))
							fl_color(m_colors.select);
						else
							fl_color(m_colors.hilight);
					}
					else
						fl_color(m_colors.number);
					fl_draw(string, xpos, ypos-2);

					// Prepare the new ASCII value to be drawn
					if ((data >= ' ') && (data <= '~'))
						sprintf(string, "%c", data);
					else
						strcpy(string, ".");
					xpos = (int) (x() + (59 + col) * m_Width);

					// Erase the old ASCII value
					if (actualAddr >= ROMSIZE && actualAddr < gRamBottom)
						fl_color(m_colors.background);
					else if (IsHilighted(address))
					{
						if (IsSelected(address))
							fl_color(m_colors.select_background);
						else
							fl_color(m_colors.hilight_background);
					}
					else
						fl_color(m_colors.background);
					fl_rectf(xpos, ypos+2 - (int) m_Height, (int) m_Width, (int) m_Height);

					// Draw the new ASCII value
					if (actualAddr >= ROMSIZE && actualAddr < gRamBottom)
						fl_color(fl_darker(FL_GRAY));
					else if (IsHilighted(address))
					{
						if (IsSelected(address))
							fl_color(m_colors.select);
						else
							fl_color(m_colors.hilight);
					}
					else
						fl_color(m_colors.ascii);
					fl_draw(string, xpos, ypos-2);

					// Update cursor position
					m_CursorCol++;

					// Test if cursor on leftmost entry
					if (m_CursorCol > 74)
					{
						// Test if cursor at bottom of screen
						if (m_CursorRow != m_Lines-1)
						{
							// Not at bottom, simply increment to next line
							m_CursorRow++;
							m_CursorCol = 59;
						}
						else
						{
							// Cursor at bottom of screen.  Need to scroll 

							// Select 12 point Courier font
							fl_font(m_Font, m_FontSize);

							// Get character width & height
							height = memedit_ctrl.pScroll->h();
							size = (int) (height / m_Height);

							// Test if scroll at bottom
							if (memedit_ctrl.pScroll->value() != m_Max - size)
							{
								// Not at bottom.  Scroll 1 line
								m_CursorCol = 59;
								ScrollUp(-1);
							}
							else
								m_CursorCol = 73;
						}
					}

					// Draw new cursor
					DrawCursor();

				}
			}

			// Update the Watch window
			memedit_ctrl.pWatchTable->UpdateTable();

			break;
			
		}
		break;

	case FL_MOUSEWHEEL:
		return memedit_ctrl.pScroll->handle(event);

	default:
		Fl_Widget::handle(event);
		break;

	}

	// Calculate the address associated with the click
	if (m_CursorCol >= 59)
		col = m_CursorCol - 59;
	else if (m_CursorCol > 8+8*3) 
		col = (int) ((m_CursorCol-10) / 3);
	else
		col = (int) ((m_CursorCol-9) / 3);

	m_CursorAddress = memedit_ctrl.pScroll->value() * 16;
	m_CursorAddress += m_CursorRow * 16;
	m_CursorAddress += col;

	// Test if our a marker selection changed
	memedit_marker_t*	pOldSel = GetMarker(prevAddress);
	memedit_marker_t*	pNewSel = GetMarker(m_CursorAddress);
	if (pOldSel != pNewSel)
		redraw();

	return 1;
}


/*
================================================================
ScrollUp:	This function scrolls the window up the specified
			number of lines
================================================================
*/
void T100_MemEditor::ScrollUp(int lines)
{
	int		height, size, value;

	// Get current scroll value
	value = memedit_ctrl.pScroll->value();

	// Test if at top already
	if ((value == 0) && (lines > 0))
		return;


	// Select 12 point Courier font
	fl_font(m_Font, m_FontSize);

	// Get height of screen in lines
	height = memedit_ctrl.pScroll->h();
	size = (int) (height / m_Height);

	// Test if at bottom and scrolling down
	if ((value == m_Max - size) && (lines < 0))
		return;

	// Update new value base on scroll distance requested
	value -= lines;
	if (value < 0)
		value = 0;

	// Update scroll bar
	memedit_ctrl.pScroll->value(value, 1, 0, (int) m_Max);
	memedit_ctrl.pScroll->maximum(m_Max-size);
	memedit_ctrl.pScroll->step(size / m_Max);
	memedit_ctrl.pScroll->slider_size(size/m_Max);
	memedit_ctrl.pScroll->linesize(1);
	redraw();
}

/*
================================================================
MoveTo:	This function moves to the specified line
================================================================
*/
void T100_MemEditor::MoveTo(int address)
{
	int		height, size, value, col;
	int		curVal, moveToVal;

	/* If the range selected is RAM, then we need to subtract 
	   the ROM size from the address so we jump to the right
	   location in the scroll bar */
	if ((m_Region == REGION_RAM) || (m_Region == REGION_RAM2) || 
		(m_Region == REGION_RAM3) || m_Region == REGION_RAM1)
			address -= ROMSIZE;

	/* Calculate line value */
	value = address / 16 - 4;
	col = address % 16;

	// Erase the cursor, if any */
	EraseCursor();

	// Select 12 point Courier font
	fl_font(m_Font, m_FontSize);

	// Get height of screen in lines
	height = memedit_ctrl.pScroll->h();
	size = (int) (height / m_Height);

	// Test if at bottom and scrolling down
	moveToVal = value;
	if (value > m_Max - size)
		moveToVal = (int) (m_Max - size);

	// Update new value base on scroll distance requested
	if (moveToVal < 0)
		moveToVal = 0;

	// Selet the selection region to hilight this location
	m_SelStartAddr = address;
	m_SelEndAddr = address;

	// Test if selection already displayed
	curVal = memedit_ctrl.pScroll->value();
	if (!(value >= curVal && value < curVal+size))
	{
		// Update scroll bar
		memedit_ctrl.pScroll->value(moveToVal, 1, 0, (int) m_Max);
		memedit_ctrl.pScroll->maximum(m_Max-size);
		memedit_ctrl.pScroll->step(size / m_Max);
		memedit_ctrl.pScroll->slider_size(size/m_Max);
		memedit_ctrl.pScroll->linesize(1);
	}

	/* Set cursor row and column */
	m_CursorRow = address / 16 - memedit_ctrl.pScroll->value();
	if (m_CursorField == 0)
		m_CursorField = 1;
	if (m_CursorField == 2)
		m_CursorCol = 59 + col;
	else
		m_CursorCol = 9 + col * 3 + (col > 7 ? 1 : 0);
	m_CursorAddress = address;

	redraw();
}

/*
================================================================
Determine if the given address should be hilighted
================================================================
*/
int T100_MemEditor::IsHilighted(int address)
{
	int		start, end;

	if (m_SelStartAddr > m_SelEndAddr)
	{
		start = m_SelEndAddr;
		end = m_SelStartAddr;
	}
	else
	{
		start = m_SelStartAddr;
		end = m_SelEndAddr;
	}

	// Test if address falls within selection region
	if (address >= start && address <= end)
		return TRUE;

	return HasMarker(address);
}

/*
================================================================
Determine if the given address has an associated marker
================================================================
*/
int T100_MemEditor::HasMarker(int address)
{
	int					region;
	memedit_marker_t*	pMarkers;

	// Determine which region marker to use.  For RAM banks 1,2,3,
	// we use the REGION_RAM marker so they all have the same
	// set of markers
	region = m_Region;
	if (region == REGION_RAM1 || region == REGION_RAM2 || region == REGION_RAM3)
		region = REGION_RAM;

	// Get pointer to start of marker list
	pMarkers = m_pRegionMarkers[region];

	// Scan through the markers
	while (pMarkers != NULL)
	{
		if (address >= pMarkers->startAddr && address <= pMarkers->endAddr)
			return TRUE;
		pMarkers = pMarkers->pNext;
	}

	return FALSE;
}

/*
================================================================
Erase the cursor at its current position
================================================================
*/
void T100_MemEditor::EraseCursor()
{
	int		x_pos, y_pos, address, col;

	// Clip the drawing
	window()->make_current();
	fl_push_clip(x(), y(), w(), h());

	// Erase current cursor
	fl_color(m_colors.background);
	if (m_CursorRow != -1)
	{
		x_pos = (int) (m_CursorCol * m_Width + x());
		y_pos = (int) (m_CursorRow * m_Height + y());
		fl_line(x_pos-1, y_pos+2, x_pos-1, y_pos + (int) m_Height);
		fl_line(x_pos, y_pos+2, x_pos, y_pos + (int) m_Height);

		// Now determine if we need to redraw any hilight regions for 
		// either the current or previous address
		if (m_CursorField == 1)
		{
			// Process keystroke -- first determine address being modified
			if (m_CursorCol > 8+8*3)
				col = (int) ((m_CursorCol-10) / 3);
			else 
				col = (int) ((m_CursorCol-9) / 3);
		}
		else
			col = (int) m_CursorCol-59;

		// Get address of first item on screen
		address = memedit_ctrl.pScroll->value() * 16;
		address += m_CursorRow * 16;
		address += col;

		// If this item is hilighted, then redraw the hilight region
		if (IsHilighted(address))
		{
			// Select the hilight background color
			if (IsSelected(address))
				fl_color(m_colors.select_background);
			else
			    fl_color(m_colors.hilight_background);
			fl_line(x_pos, y_pos+2, x_pos, y_pos + (int) m_Height);

			// If we aren't on col zero, then test the next lower address too
			if (m_CursorCol != 9 && m_CursorCol != 59 && m_CursorCol != 34)
			{
				if (IsHilighted(address-1))
				{
					// Select the hilight background color
					if (IsSelected(address-1))
						fl_color(m_colors.select_background);
					else
						fl_color(m_colors.hilight_background);
					fl_line(x_pos-1, y_pos+2, x_pos-1, y_pos+(int)m_Height);
				}
			}
		}
	}

	// Pop the clipping rect
	fl_pop_clip();
}

/*
================================================================
Draws the cursor at its current position
================================================================
*/
void T100_MemEditor::DrawCursor()
{
	int		x_pos, y_pos;

	if (!m_MyFocus)
		return;

	// Clip the drawing
	window()->make_current();
	fl_push_clip(x(), y(), w(), h());

	// Draw new cursor
	fl_color(m_colors.cursor);
	if (m_CursorRow != -1)
	{
		x_pos = (int) (m_CursorCol * m_Width + x());
		y_pos = (int) (m_CursorRow * m_Height + y());
		fl_line(x_pos-1, y_pos+2, x_pos-1, y_pos + (int) m_Height-2);
		fl_line(x_pos, y_pos+2, x_pos, y_pos + (int) m_Height-2);
	}

	UpdateAddressText();

	fl_pop_clip();
}

/*
================================================================
Update the Address edit box base on the position of the cursor
================================================================
*/
void T100_MemEditor::UpdateAddressText()
{
	char	string[10];
	long	address;
	int		col, region;

	if (!m_MyFocus)
		return;

	// Determine column of cursor
	if (m_CursorField == 1)
	{
		// Process keystroke -- first determine address being modified
		if (m_CursorCol > 8+8*3)
			col = (int) ((m_CursorCol-10) / 3);
		else 
			col = (int) ((m_CursorCol-9) / 3);
	}
	else
		col = (int) m_CursorCol-59;


	// Get address of first item on screen
	address = memedit_ctrl.pScroll->value() * 16;
	address += m_CursorRow * 16;
	address += col;

	// Now create text
	// Calculate region where memory is read from
	region = GetRegionEnum();

	if ((region == REGION_RAM) || (region == REGION_RAM2) || 
		(region == REGION_RAM3) || region == REGION_RAM1)
	{
		if (gReMem)
			sprintf(string, "0x%06X", (unsigned int) (address + RAMSTART));
		else
			sprintf(string, "0x%04X", (unsigned int) (address + RAMSTART));
	}
	else
	{
		if (gReMem)
			sprintf(string, "0x%06X", (unsigned int) address);
		else
			sprintf(string, "0x%04X", (unsigned int) address);
	}
	memedit_ctrl.pMemRange->value(string);
}

/*
============================================================================
Load user preferences
============================================================================
*/
void MemoryEditor_LoadPrefs(void)
{
	int		temp, bold, size;
	char	model[16];
	char*	pVarText;
	Fl_Preferences g(virtualt_prefs, "MemEdit_Group");

	// Get main window geometry
	g.get("x", memedit_ctrl.pMemEdit->m_x, -1);
	g.get("y", memedit_ctrl.pMemEdit->m_y, -1);
	g.get("w", memedit_ctrl.pMemEdit->m_w, -1);
	g.get("h", memedit_ctrl.pMemEdit->m_h, -1);

	// Test for old style preferences
	if (memedit_ctrl.pMemEdit->m_x == -1)
	{
		virtualt_prefs.get("MemEdit_x", memedit_ctrl.pMemEdit->m_x, -1);
		virtualt_prefs.get("MemEdit_y", memedit_ctrl.pMemEdit->m_y, -1);
		virtualt_prefs.get("MemEdit_w", memedit_ctrl.pMemEdit->m_w, -1);
		virtualt_prefs.get("MemEdit_h", memedit_ctrl.pMemEdit->m_h, -1);
	}

	// Get watch window geometry
	g.get("SplitH", memedit_ctrl.iSplitH, -1);
	g.get("Col0_Width", memedit_ctrl.iCol0W, -1);
	g.get("Col1_Width", memedit_ctrl.iCol1W, -1);
	g.get("Col2_Width", memedit_ctrl.iCol2W, -1);
	g.get("Col3_Width", memedit_ctrl.iCol3W, -1);
	g.get("Col4_Width", memedit_ctrl.iCol4W, -1);

	// Test if we have saved to the group yet
	g.get("FontSize", temp, -1);
	if (temp == -1)
	{	
		// We haven't saved to the group yet... use non-group entries

		// Get font settings
		virtualt_prefs.get("MemEdit_FontSize", temp, 12);
		virtualt_prefs.get("MemEdit_Bold", bold, 0);
		memedit_ctrl.pMemEdit->SetFontSize(temp, bold);

		// Get color settings
		virtualt_prefs.get("MemEdit_BlackBackground", temp, 1);
		memedit_ctrl.pMemEdit->SetBlackBackground(temp);
		virtualt_prefs.get("MemEdit_SyntaxHilight", temp, 1);
		memedit_ctrl.pMemEdit->SetSyntaxHilight(temp);
		virtualt_prefs.get("MemEdit_MarkerForegroundColor", temp, (int) FL_WHITE);
		memedit_ctrl.pMemEdit->SetMarkerForegroundColor((Fl_Color) temp);
		virtualt_prefs.get("MemEdit_MarkerBackgroundColor", temp, 226);
		memedit_ctrl.pMemEdit->SetMarkerBackgroundColor((Fl_Color) temp);
		virtualt_prefs.get("MemEdit_SelectedForegroundColor", temp, (int) FL_WHITE);
		memedit_ctrl.pMemEdit->SetSelectedForegroundColor((Fl_Color) temp);
		virtualt_prefs.get("MemEdit_SelectedBackgroundColor", temp, 81);
		memedit_ctrl.pMemEdit->SetSelectedBackgroundColor((Fl_Color) temp);
	}
	else
	{
		// Get font settings
		g.get("FontSize", temp, 12);
		g.get("Bold", bold, 0);
		memedit_ctrl.pMemEdit->SetFontSize(temp, bold);

		// Get color settings
		g.get("BlackBackground", temp, 1);
		memedit_ctrl.pMemEdit->SetBlackBackground(temp);
		g.get("SyntaxHilight", temp, 1);
		memedit_ctrl.pMemEdit->SetSyntaxHilight(temp);
		g.get("MarkerFgColor", temp, (int) FL_WHITE);
		memedit_ctrl.pMemEdit->SetMarkerForegroundColor((Fl_Color) temp);
		g.get("MarkerBgColor", temp, 226);
		memedit_ctrl.pMemEdit->SetMarkerBackgroundColor((Fl_Color) temp);
		g.get("SelectedFgColor", temp, (int) FL_WHITE);
		memedit_ctrl.pMemEdit->SetSelectedForegroundColor((Fl_Color) temp);
		g.get("SelectedBgColor", temp, 81);
		memedit_ctrl.pMemEdit->SetSelectedBackgroundColor((Fl_Color) temp);
	}

	// Load the region markers
	memedit_ctrl.pMemEdit->LoadRegionMarkers();

	// Load the watch variables
	memedit_ctrl.pWatchTable->ResetContent();
	get_model_string(model, gModel);
	size = g.size(Fl_Preferences::Name("%s_WatchVars", model));
	if (size > 0)
	{
		// Allocate the buffer
		pVarText = new char[size+1];
		if (pVarText != NULL)
		{
			// Get the watch variables string
			g.get(Fl_Preferences::Name("%s_WatchVars", model), pVarText, "", size+1);
		
			// And give it to the watch table
			memedit_ctrl.pWatchTable->WatchVarsString(pVarText);

			// Now delete the string
			delete pVarText;
		}
	}

	// Redraw the watch table
	memedit_ctrl.pWatchTable->redraw();
}

/*
============================================================================
Save user preferences
============================================================================
*/
void MemoryEditor_SavePrefs(void)
{
	MString		watch_vars, key;
	char		model[16];
	Fl_Preferences g(virtualt_prefs, "MemEdit_Group");

	g.set("x", gmew->x());
	g.set("y", gmew->y());
	g.set("w", gmew->w());
	g.set("h", gmew->h());

	// Save watch variable geometry
	g.set("SplitH", memedit_ctrl.pTopPane->h());
	g.set("Col0_Width", memedit_ctrl.pWatchTable->HeaderBoxWidth(0));
	g.set("Col1_Width", memedit_ctrl.pWatchTable->HeaderBoxWidth(1));
	g.set("Col2_Width", memedit_ctrl.pWatchTable->HeaderBoxWidth(2));
	g.set("Col3_Width", memedit_ctrl.pWatchTable->HeaderBoxWidth(3));
	g.set("Col4_Width", memedit_ctrl.pWatchTable->HeaderBoxWidth(4));

	g.set("FontSize", memedit_ctrl.pMemEdit->GetFontSize());
	g.set("Bold", memedit_ctrl.pMemEdit->GetBold());
	g.set("BlackBackground", memedit_ctrl.pMemEdit->GetBlackBackground());
	g.set("SyntaxHilight", memedit_ctrl.pMemEdit->GetSyntaxHilight());
	g.set("MarkerFgColor", (int) memedit_ctrl.pMemEdit->GetMarkerForegroundColor());
	g.set("MarkerBgColor", (int) memedit_ctrl.pMemEdit->GetMarkerBackgroundColor());
	g.set("SelectedFgColor", (int) memedit_ctrl.pMemEdit->GetSelectedForegroundColor());
	g.set("SelectedBgColor", (int) memedit_ctrl.pMemEdit->GetSelectedBackgroundColor());

	// Save the region markers
	memedit_ctrl.pMemEdit->SaveRegionMarkers();

	// Save watch variables
	get_model_string(model, gModel);
	watch_vars = memedit_ctrl.pWatchTable->WatchVarsString();
	g.set(Fl_Preferences::Name("%s_WatchVars", model), (const char *) watch_vars);

	// Delete old keys from the file
	virtualt_prefs.deleteEntry("MemEdit_x");
	virtualt_prefs.deleteEntry("MemEdit_y");
	virtualt_prefs.deleteEntry("MemEdit_w");
	virtualt_prefs.deleteEntry("MemEdit_h");
	virtualt_prefs.deleteEntry("MemEdit_FontSize");
	virtualt_prefs.deleteEntry("MemEdit_Bold");
	virtualt_prefs.deleteEntry("MemEdit_BlackBackground");
	virtualt_prefs.deleteEntry("MemEdit_SyntaxHilight");
	virtualt_prefs.deleteEntry("MemEdit_MarkerForegroundColor");
	virtualt_prefs.deleteEntry("MemEdit_MarkerBackgroundColor");
	virtualt_prefs.deleteEntry("MemEdit_SelectedForegroundColor");
	virtualt_prefs.deleteEntry("MemEdit_SelectedBackgroundColor");

	// Flush the preferences to disk in case we only closed the window
	// or changed model, that way we are protected from a crash
	virtualt_prefs.flush();
}

/*
============================================================================
Set's the memory editor's font size
============================================================================
*/
void T100_MemEditor::SaveRegionMarkers()
{
	char				str[16];
	char				markerText[32];
	MString				markers;
	int					region;
	memedit_marker_t*	pMarkers;
	Fl_Preferences		g(virtualt_prefs, "MemEdit_Group");

	// Get the model string to save stuff per model
	get_model_string(str, gModel);

	// Save region markers
	for (region = 0; region < REGION_MAX; region++)
	{
		// Now build the region marker text
		markers = "";

		// Get pointer to start of marker list and iterate
		pMarkers = m_pRegionMarkers[region];
		while (pMarkers != NULL)
		{
			// Create next marker text
			if (pMarkers->startAddr == pMarkers->endAddr)
				sprintf(markerText, "%d", pMarkers->startAddr);
			else
				sprintf(markerText, "%d-%d", pMarkers->startAddr, pMarkers->endAddr);

			// If there's another marker after this one, append a ','
			if (pMarkers->pNext != NULL)
				strcat(markerText, ",");

			// Add to the text
			markers += markerText;

			// Point to next marker
			pMarkers = pMarkers->pNext;
		}

		// Test if we even need the setting in the preferences file
		if (m_pRegionMarkers[region] == NULL)
			g.deleteEntry(Fl_Preferences::Name("%s_Region%d_Markers", str, region));
		else
			g.set(Fl_Preferences::Name("%s_Region%d_Markers", str, region),
				(const char *) markers);

		// Clean up old style entries
		virtualt_prefs.deleteEntry(Fl_Preferences::Name("%s_Region%d_Markers", str, region));
	}
}

/*
============================================================================
Set's the memory editor's font size
============================================================================
*/
void T100_MemEditor::LoadRegionMarkers()
{
	char				str[16];
	char*				pMarkerText;
	int					region, startAddr, endAddr, c, bufSize;
	int					size, which;
	Fl_Preferences		g(virtualt_prefs, "MemEdit_Group");

	// First reset any old markers
	ResetMarkers();

	// Get the model string to save stuff per model
	get_model_string(str, gModel);

	// Allocate a big buffer to read in marker text
	pMarkerText = new char[65536];
	bufSize = 65536;

	// Only if we could allocate the buffer...
	if (pMarkerText != NULL)
	{
		// Loop for each region
		for (region = 0; region < REGION_MAX; region++)
		{
			// Determine the size buffer needed
			which = 0;
			size = g.size(Fl_Preferences::Name("%s_Region%d_Markers", str, region));
			if (size == 0)
			{
				size = virtualt_prefs.size(Fl_Preferences::Name("%s_Region%d_Markers", 
					str, region));
				which = 1;
			}

			// If no markers defined, skip to next region
			if (size == 0)
				continue;

			// Test if our buffer is big enough
			if (size > bufSize)
			{
				// Allocate a bigger buffer
				delete pMarkerText;
				pMarkerText = new char[size];
				bufSize = size;
				if (pMarkerText == NULL)
					return;
			}

			// Now get the text
			if (which == 0)
			{
				// Get it from the group
				g.get(Fl_Preferences::Name("%s_Region%d_Markers", 
					str, region), pMarkerText, "", bufSize);
			}
			else
			{
				// Get the text from base preferences
				virtualt_prefs.get(Fl_Preferences::Name("%s_Region%d_Markers", 
				str, region), pMarkerText, "", bufSize);
			}

			// Check if any markers defined
			if (pMarkerText[0] != '\0')
			{
				// Parse the marker list
				c = startAddr = endAddr = 0;
				
				while (pMarkerText[c] != '\0')
				{
					// Parse the startAddr
					while (pMarkerText[c] != '-' && pMarkerText[c] != '\0' &&
						pMarkerText[c] != ',')
							startAddr = startAddr * 10 + pMarkerText[c++] - '0';
					
					// Skip the '-'
					if (pMarkerText[c] == '-')
						c++;
					else if(pMarkerText[c] == ',' || pMarkerText[c] == '\0')
						endAddr = startAddr;

					// Parse the endAddr
					while (pMarkerText[c] != ',' && pMarkerText[c] != '\0')
						endAddr = endAddr * 10 + pMarkerText[c++] - '0';

					// Add this marker to the region
					AddMarkerToRegion(startAddr, endAddr, region);

					// Skip the ',' if any
					if (pMarkerText[c] == ',')
						c++;

					// Clear start and end addr for the next marker
					startAddr = endAddr = 0;
				}
			}
		}

		// Delete the marker text
		delete pMarkerText;
	}
}

/*
============================================================================
Set's the memory editor's font size
============================================================================
*/
void T100_MemEditor::SetFontSize(int fontsize, int bold)
{
	// Save the font size
	m_FontSize = fontsize;
	m_Bold = bold;

	if (bold)
		m_Font = FL_COURIER_BOLD;
	else
		m_Font = FL_COURIER;

	// Recalculate width and height
	fl_font(m_Font, m_FontSize);
	m_Width = fl_width("W", 1);
	m_Height = fl_height();
}

/*
============================================================================
Updates the editor's color scheme.
============================================================================
*/
void T100_MemEditor::UpdateColors(void)
{
	// Update the colors
	if (m_ColorSyntaxHilight)
	{
		if (m_BlackBackground)
		{
			m_colors.background = FL_BLACK;
			m_colors.foreground = FL_WHITE;
			m_colors.addr = fl_lighter((Fl_Color) 166);
			m_colors.number = FL_WHITE;
			m_colors.ascii = FL_YELLOW;
			m_colors.cursor = FL_WHITE;
			m_colors.changed = FL_WHITE;
		}
		else
		{
			m_colors.background = FL_WHITE;
			m_colors.foreground = FL_BLACK;
			m_colors.addr = fl_darker(FL_RED);
			m_colors.number = FL_BLACK;
			m_colors.ascii = fl_darker(FL_BLUE);
			m_colors.cursor = FL_BLACK;
			m_colors.changed = FL_BLACK;
		}
	}
	else
	{
		if (m_BlackBackground)
		{
			m_colors.background = FL_BLACK;
			m_colors.foreground = FL_WHITE;
			m_colors.addr = FL_WHITE;
			m_colors.number = FL_WHITE;
			m_colors.ascii = FL_WHITE;
			m_colors.cursor = FL_WHITE;
			m_colors.changed = FL_WHITE;
		}
		else
		{
			m_colors.background = FL_WHITE;
			m_colors.foreground = FL_BLACK;
			m_colors.addr = FL_BLACK;
			m_colors.number = FL_BLACK;
			m_colors.ascii = FL_BLACK;
			m_colors.cursor = FL_BLACK;
			m_colors.changed = FL_BLACK;
		}
	}

	// Now redraw
	redraw();
}

/*
============================================================================
Set's the memory editor's black background selection
============================================================================
*/
void T100_MemEditor::SetBlackBackground(int blackBackground)
{
	// Set the selection and update colors
	m_BlackBackground = blackBackground;
	UpdateColors();
}

/*
============================================================================
Set's the memory editor's color syntax hilight selection
============================================================================
*/
void T100_MemEditor::SetSyntaxHilight(int colorSyntaxHilight)
{
	// Set the selection and update colors
	m_ColorSyntaxHilight = colorSyntaxHilight;
	UpdateColors();
}

/*
============================================================================
Define a local struct for the setup parameters.
============================================================================
*/
typedef struct memedit_setup_params
{
	Fl_Input*			pFontSize;
	Fl_Check_Button*	pBold;
	Fl_Check_Button*	pBlackBackground;
	Fl_Check_Button*	pColorHilight;
	Fl_Button*			pMarkerForeground;
	Fl_Button*			pMarkerBackground;
	Fl_Button*			pSelectedForeground;
	Fl_Button*			pSelectedBackground;
	Fl_Button*			pDefaults;
	char				sFontSize[20];
	Fl_Color			origMarkerForeground;
	Fl_Color			origMarkerBackground;
	Fl_Color			origSelectedForeground;
	Fl_Color			origSelectedBackground;
} memedit_setup_params_t;

/*
============================================================================
Callback for Trace Setup Ok button
============================================================================
*/
static void cb_setupdlg_OK(Fl_Widget* w, void* pOpaque)
{
	memedit_setup_params_t*	p = (memedit_setup_params_t *) pOpaque;
	int						fontsize;

	// Save values
	if (strlen(p->pFontSize->value()) > 0)
	{
		// update the font size
		fontsize = atoi(p->pFontSize->value());
		if (fontsize < 6)
			fontsize = 6;

		// Set the new size
		if (fontsize > 0)
			memedit_ctrl.pMemEdit->SetFontSize(fontsize, p->pBold->value());
	}

	// Get Inverse Highlight selection
	memedit_ctrl.pMemEdit->SetBlackBackground(p->pBlackBackground->value());

	// Get hilighting option
	memedit_ctrl.pMemEdit->SetSyntaxHilight(p->pColorHilight->value());

	// Get Marker colors
	memedit_ctrl.pMemEdit->SetMarkerForegroundColor((Fl_Color) p->pMarkerForeground->color());
	memedit_ctrl.pMemEdit->SetMarkerBackgroundColor((Fl_Color) p->pMarkerBackground->color());
	memedit_ctrl.pMemEdit->SetSelectedForegroundColor((Fl_Color) p->pSelectedForeground->color());
	memedit_ctrl.pMemEdit->SetSelectedBackgroundColor((Fl_Color) p->pSelectedBackground->color());

	// Redraw the window
	gmew->redraw();
	
	// Now hide the parent dialog
	w->parent()->hide();
}

/*
============================================================================
Callback for Trace Setup Cancel button
============================================================================
*/
static void cb_setupdlg_cancel(Fl_Widget* w, void* pOpaque)
{
	memedit_setup_params_t*	p = (memedit_setup_params_t *) pOpaque;

	// Hide the parent dialog so we cancel out
	w->parent()->hide();

	// Restore original marker colors and redraw
	memedit_ctrl.pMemEdit->SetMarkerForegroundColor(p->origMarkerForeground);
	memedit_ctrl.pMemEdit->SetMarkerBackgroundColor(p->origMarkerBackground);
	memedit_ctrl.pMemEdit->SetSelectedForegroundColor(p->origSelectedForeground);
	memedit_ctrl.pMemEdit->SetSelectedBackgroundColor(p->origSelectedBackground);
	memedit_ctrl.pMemEdit->redraw();
}

/*
============================================================================
Callback for default marker colors.
============================================================================
*/
static void cb_default_colors(Fl_Button*, void* pOpaque) 
{
	memedit_setup_params_t*	p = (memedit_setup_params_t *) pOpaque;

	p->pMarkerBackground->color((Fl_Color) 226);
	p->pMarkerForeground->color(FL_WHITE);
	p->pSelectedBackground->color((Fl_Color) 81);
	p->pSelectedForeground->color(FL_WHITE);
	p->pMarkerBackground->redraw();
	p->pMarkerForeground->redraw();
	p->pSelectedBackground->redraw();
	p->pSelectedForeground->redraw();
	memedit_ctrl.pMemEdit->SetMarkerForegroundColor(p->pMarkerForeground->color());
	memedit_ctrl.pMemEdit->SetMarkerBackgroundColor(p->pMarkerBackground->color());
	memedit_ctrl.pMemEdit->SetSelectedForegroundColor(p->pSelectedForeground->color());
	memedit_ctrl.pMemEdit->SetSelectedBackgroundColor(p->pSelectedBackground->color());
	memedit_ctrl.pMemEdit->redraw();
}

/*
============================================================================
Callback for Marker Foreground color button
============================================================================
*/
static void cb_marker_foreground(Fl_Button*, void* pOpaque) 
{
	memedit_setup_params_t*	p = (memedit_setup_params_t *) pOpaque;

	p->pMarkerForeground->color(fl_show_colormap(p->pMarkerForeground->color()));
	memedit_ctrl.pMemEdit->SetMarkerForegroundColor(p->pMarkerForeground->color());
	memedit_ctrl.pMemEdit->redraw();
}

/*
============================================================================
Callback for Marker Background color button
============================================================================
*/
static void cb_marker_background(Fl_Button*, void* pOpaque) 
{
	memedit_setup_params_t*	p = (memedit_setup_params_t *) pOpaque;

	p->pMarkerBackground->color(fl_show_colormap(p->pMarkerBackground->color()));
	memedit_ctrl.pMemEdit->SetMarkerBackgroundColor(p->pMarkerBackground->color());
	memedit_ctrl.pMemEdit->redraw();
}

/*
============================================================================
Callback for Selected Marker Foreground color button
============================================================================
*/
static void cb_selected_foreground(Fl_Button*, void* pOpaque) 
{
	memedit_setup_params_t*	p = (memedit_setup_params_t *) pOpaque;

	p->pSelectedForeground->color(fl_show_colormap(p->pSelectedForeground->color()));
	memedit_ctrl.pMemEdit->SetSelectedForegroundColor(p->pSelectedForeground->color());
	memedit_ctrl.pMemEdit->redraw();
}

/*
============================================================================
Callback for Selected Marker Background color button
============================================================================
*/
static void cb_selected_background(Fl_Button*, void* pOpaque) 
{
	memedit_setup_params_t*	p = (memedit_setup_params_t *) pOpaque;

	p->pSelectedBackground->color(fl_show_colormap(p->pSelectedBackground->color()));
	memedit_ctrl.pMemEdit->SetSelectedBackgroundColor(p->pSelectedBackground->color());
	memedit_ctrl.pMemEdit->redraw();
}

/*
============================================================================
Menu callback for setup memory editor
============================================================================
*/
void cb_setup_memedit(Fl_Widget* w, void* pOpaque)
{
	Fl_Box*					b;
	Fl_Window*				pWin;
	memedit_setup_params_t	p;

	// Save current Marker colors
	p.origMarkerForeground = memedit_ctrl.pMemEdit->GetMarkerForegroundColor();
	p.origMarkerBackground = memedit_ctrl.pMemEdit->GetMarkerBackgroundColor();
	p.origSelectedForeground = memedit_ctrl.pMemEdit->GetSelectedForegroundColor();
	p.origSelectedBackground = memedit_ctrl.pMemEdit->GetSelectedBackgroundColor();

	// Create new window for memory editor configuration
	pWin = new Fl_Window(330, 240, "Memory Editor Configuration");

	/* Create input field for font size */
	b = new Fl_Box(20, 20, 100, 20, "Font Size");
	b->align(FL_ALIGN_LEFT | FL_ALIGN_INSIDE);
	p.pFontSize = new Fl_Input(120, 20, 60, 20, "");
	p.pFontSize->align(FL_ALIGN_LEFT);
	sprintf(p.sFontSize, "%d", memedit_ctrl.pMemEdit->GetFontSize());
	p.pFontSize->value(p.sFontSize);

	/* Create checkbox for bold font selection */
	p.pBold = new Fl_Check_Button(200, 20, 60, 20, "Bold");
	p.pBold->value(memedit_ctrl.pMemEdit->GetBold());

	/* Create checkbox for hilight style */
	p.pBlackBackground = new Fl_Check_Button(20, 50, 190, 20, "Black background");
	p.pBlackBackground->value(memedit_ctrl.pMemEdit->GetBlackBackground());

	/* Create checkbox for color hilighting */
	p.pColorHilight = new Fl_Check_Button(20, 80, 200, 20, "Color hilighting");
	p.pColorHilight->value(memedit_ctrl.pMemEdit->GetSyntaxHilight());

	/* Create a button for the marker foreground color */
	p.pMarkerForeground = new Fl_Button(22, 110, 15, 15, "Marker Forground Color");
    p.pMarkerForeground->callback((Fl_Callback*) cb_marker_foreground, &p);
	p.pMarkerForeground->align(FL_ALIGN_RIGHT);
	p.pMarkerForeground->color(memedit_ctrl.pMemEdit->GetMarkerForegroundColor());

	/* Create a button for the marker background color */
	p.pMarkerBackground = new Fl_Button(22, 130, 15, 15, "Marker Background Color");
    p.pMarkerBackground->callback((Fl_Callback*) cb_marker_background, &p);
	p.pMarkerBackground->align(FL_ALIGN_RIGHT);
	p.pMarkerBackground->color(memedit_ctrl.pMemEdit->GetMarkerBackgroundColor());

	/* Create a button for the marker foreground color */
	p.pSelectedForeground = new Fl_Button(22, 150, 15, 15, "Selected Marker Forground");
    p.pSelectedForeground->callback((Fl_Callback*) cb_selected_foreground, &p);
	p.pSelectedForeground->align(FL_ALIGN_RIGHT);
	p.pSelectedForeground->color(memedit_ctrl.pMemEdit->GetSelectedForegroundColor());

	/* Create a button for the marker background color */
	p.pSelectedBackground = new Fl_Button(22, 170, 15, 15, "Selected Marker Background");
    p.pSelectedBackground->callback((Fl_Callback*) cb_selected_background, &p);
	p.pSelectedBackground->align(FL_ALIGN_RIGHT);
	p.pSelectedBackground->color(memedit_ctrl.pMemEdit->GetSelectedBackgroundColor());

	p.pDefaults = new Fl_Button(240, 135, 70, 30, "Defaults");
	p.pDefaults->callback((Fl_Callback *) cb_default_colors, &p);

	// Cancel button
    { Fl_Button* o = new Fl_Button(80, 200, 60, 30, "Cancel");
      o->callback((Fl_Callback*) cb_setupdlg_cancel, &p);
    }

	// OK button
    { Fl_Return_Button* o = new Fl_Return_Button(160, 200, 60, 30, "OK");
      o->callback((Fl_Callback*) cb_setupdlg_OK, &p);
    }

	// Loop until user presses OK or Cancel
	pWin->show();
	while (pWin->visible())
		Fl::wait();

	// Delete the window
	delete pWin;
}

/*
============================================================================
Menu callback for adding a marker
============================================================================
*/
void cb_add_marker(Fl_Widget* w, void* pOpaque)
{
	memedit_ctrl.pMemEdit->AddMarker();
}

/*
============================================================================
Adds the current SelStart / SelEnd selection as a new marker
============================================================================
*/
void T100_MemEditor::AddMarker(int region)
{
	memedit_marker_t*	pMarkers;
	memedit_marker_t*	pNewMarker;

	// Determine which region marker to use.  For RAM banks 1,2,3,
	// we use the REGION_RAM marker so they all have the same
	// set of markers
	if (region == -1)
	{
		region = m_Region;
		if (region == REGION_RAM1 || region == REGION_RAM2 || region == REGION_RAM3)
			region = REGION_RAM;
	}

	// Get pointer to start of marker list
	pMarkers = m_pRegionMarkers[region];

	// Create a new marker for this selection
	pNewMarker = new memedit_marker_t;
	pNewMarker->startAddr = m_SelStartAddr;
	pNewMarker->endAddr = m_SelEndAddr;
	pNewMarker->pNext = NULL;

	// Insert this marker into the list
	if (pMarkers == NULL)
	{
		// This is the head item.  Just insert it
		m_pRegionMarkers[region] = pNewMarker;
	}
	else
	{
		// Find end of marker list
		while (pMarkers->pNext != NULL)
			pMarkers = pMarkers->pNext;

		// Insert the marker here
		pMarkers->pNext = pNewMarker;
	}

	redraw();
}

/*
============================================================================
Adds the specified marker to the specified region
============================================================================
*/
void T100_MemEditor::AddMarkerToRegion(int startAddr, int endAddr, int region)
{
	memedit_marker_t*	pMarkers;
	memedit_marker_t*	pNewMarker;

	// Get pointer to start of marker list
	pMarkers = m_pRegionMarkers[region];

	// Create a new marker for this selection
	pNewMarker = new memedit_marker_t;
	pNewMarker->startAddr = startAddr;
	pNewMarker->endAddr = endAddr;
	pNewMarker->pNext = NULL;

	// Insert this marker into the list
	if (pMarkers == NULL)
	{
		// This is the head item.  Just insert it
		m_pRegionMarkers[region] = pNewMarker;
	}
	else
	{
		// Find end of marker list
		while (pMarkers->pNext != NULL)
			pMarkers = pMarkers->pNext;

		// Insert the marker here
		pMarkers->pNext = pNewMarker;
	}
}

/*
============================================================================
Menu callback for deleting a marker
============================================================================
*/
void cb_delete_marker(Fl_Widget* w, void*)
{
	memedit_ctrl.pMemEdit->DeleteMarker();
}

/*
============================================================================
Menu callback for deleting all markers
============================================================================
*/
void cb_delete_all_markers(Fl_Widget* w, void*)
{
	memedit_ctrl.pMemEdit->DeleteAllMarkers();
}

/*
============================================================================
Menu callback for deleting all markers
============================================================================
*/
void cb_undo_delete_all_markers(Fl_Widget* w, void*)
{
	memedit_ctrl.pMemEdit->UndoDeleteAllMarkers();
}

/*
============================================================================
Menu callback for finding the next marker
============================================================================
*/
void cb_goto_next_marker(Fl_Widget* w, void*)
{
	memedit_ctrl.pMemEdit->FindNextMarker();
}

/*
============================================================================
Menu callback for finding the prev marker
============================================================================
*/
void cb_goto_prev_marker(Fl_Widget* w, void*)
{
	memedit_ctrl.pMemEdit->FindPrevMarker();
}

/*
============================================================================
Find's the next marker after the current cursor address
============================================================================
*/
void T100_MemEditor::FindNextMarker(void)
{
	int					region, nextAddr, firstAddr;
	memedit_marker_t*	pMarkers;

	// Determine which region marker to use.  For RAM banks 1,2,3,
	// we use the REGION_RAM marker so they all have the same
	// set of markers
	region = m_Region;
	if (region == REGION_RAM1 || region == REGION_RAM2 || region == REGION_RAM3)
		region = REGION_RAM;

	// Get pointer to start of marker list
	pMarkers = m_pRegionMarkers[region];
	nextAddr = 99999999;
	firstAddr = 99999999;

	// Loop through all markers
	while (pMarkers != NULL)
	{
		// Test if this address is greater than the current address
		if (pMarkers->startAddr > m_CursorAddress)
		{
			// Find the lowest address that is greater than current address
			if (pMarkers->startAddr < nextAddr)
				nextAddr = pMarkers->startAddr;
		}

		// Find the address of the lowest marker in case we wrap
		if (pMarkers->startAddr < firstAddr)
			firstAddr = pMarkers->startAddr;

		// Point to next marker
		pMarkers = pMarkers->pNext;
	}

	// If no more markers greater than current address, then wrap to 1st
	if (nextAddr == 99999999)
		nextAddr = firstAddr;

	// If we found a marker, then jump there
	if (nextAddr != 99999999)
	{
		if (region == REGION_RAM)
			nextAddr += ROMSIZE;
		MoveTo(nextAddr);
		UpdateAddressText();
	}
}

/*
============================================================================
Find's the next marker after the current cursor address
============================================================================
*/
void T100_MemEditor::FindPrevMarker(void)
{
	int					region, prevAddr, lastAddr;
	memedit_marker_t*	pMarkers;

	// Determine which region marker to use.  For RAM banks 1,2,3,
	// we use the REGION_RAM marker so they all have the same
	// set of markers
	region = m_Region;
	if (region == REGION_RAM1 || region == REGION_RAM2 || region == REGION_RAM3)
		region = REGION_RAM;

	// Get pointer to start of marker list
	pMarkers = m_pRegionMarkers[region];
	prevAddr = -1;
	lastAddr = -1;

	// Loop through all markers
	while (pMarkers != NULL)
	{
		// Test if this address is greater than the current address
		if (pMarkers->endAddr < m_CursorAddress)
		{
			// Find the lowest address that is greater than current address
			if (pMarkers->startAddr > prevAddr)
				prevAddr = pMarkers->startAddr;
		}

		// Find the address of the lowest marker in case we wrap
		if (pMarkers->startAddr > lastAddr)
			lastAddr = pMarkers->startAddr;

		// Point to next marker
		pMarkers = pMarkers->pNext;
	}

	// If no more markers greater than current address, then wrap to 1st
	if (prevAddr == -1)
		prevAddr = lastAddr;

	// If we found a marker, then jump there
	if (prevAddr != -1)
	{
		if (region == REGION_RAM)
			prevAddr += ROMSIZE;
		MoveTo(prevAddr);
		UpdateAddressText();
	}
}

/*
============================================================================
Determines if address is a marker field in which the cursor is located
============================================================================
*/
memedit_marker_t* T100_MemEditor::GetMarker(int address)
{
	int					region;
	memedit_marker_t*	pMarker;

	// Determine which region marker to use.  For RAM banks 1,2,3,
	// we use the REGION_RAM marker so they all have the same
	// set of markers
	region = m_Region;
	if (region == REGION_RAM1 || region == REGION_RAM2 || region == REGION_RAM3)
		region = REGION_RAM;

	// Get pointer to start of marker list
	pMarker = m_pRegionMarkers[region];

	// Loop through all markers until we find one containing address
	while (pMarker != NULL)
	{
		// Test if address is within this marker
		if (address >= pMarker->startAddr && address <= pMarker->endAddr)
		{
			return pMarker;
		}

		// Get pointer to next marker
		pMarker = pMarker->pNext;;
	}

	return NULL;
}

/*
============================================================================
Determines if address is a marker field in which the cursor is located
============================================================================
*/
int T100_MemEditor::IsSelected(int address)
{
	int					region;
	memedit_marker_t*	pMarker;

	// Determine which region marker to use.  For RAM banks 1,2,3,
	// we use the REGION_RAM marker so they all have the same
	// set of markers
	region = m_Region;
	if (region == REGION_RAM1 || region == REGION_RAM2 || region == REGION_RAM3)
		region = REGION_RAM;

	// Get pointer to start of marker list
	pMarker = m_pRegionMarkers[region];

	// Loop through all markers until we find one containing address
	while (pMarker != NULL)
	{
		// Test if address is within this marker
		if (address >= pMarker->startAddr && address <= pMarker->endAddr)
		{
			// Now test if the current cursor address is also in this marker
			if (m_CursorAddress >= pMarker->startAddr && m_CursorAddress <= pMarker->endAddr)
				return TRUE;
			
			// Nope, cursor not in this marker.  We're done
			return FALSE;
		}

		// Point to next marker
		pMarker = pMarker->pNext;
	}

	return FALSE;
}

/*
============================================================================
Deletes the marker identified by the Cursor.
============================================================================
*/
void T100_MemEditor::DeleteMarker(void)
{
	int					region;
	memedit_marker_t*	pMarkers;
	memedit_marker_t*	pLastMarker;

	// Determine which region marker to use.  For RAM banks 1,2,3,
	// we use the REGION_RAM marker so they all have the same
	// set of markers
	region = m_Region;
	if (region == REGION_RAM1 || region == REGION_RAM2 || region == REGION_RAM3)
		region = REGION_RAM;

	// Get pointer to start of marker list
	pMarkers = m_pRegionMarkers[region];
	pLastMarker = NULL;

	// Loop for all markers and search for the one containing
	// the current cursor address
	while (pMarkers != NULL)
	{
		if (m_CursorAddress >= pMarkers->startAddr && m_CursorAddress <=
			pMarkers->endAddr)
		{
			// Delete this marker
			if (pLastMarker == NULL)
			{
				// We are deleting the 1st marker
				m_pRegionMarkers[region] = pMarkers->pNext;
			}
			else
				// We are deleting in the middle or end
				pLastMarker->pNext = pMarkers->pNext;

			// Delete this marker
			delete pMarkers;

			// Clear any selection that may exist
			m_SelStartAddr = m_SelEndAddr = -1;
			redraw();

			break;
		}

		// Get pointer to next marker
		pLastMarker = pMarkers;
		pMarkers = pMarkers->pNext;
	}
}

/*
============================================================================
Deletes the marker identified by the Cursor.
============================================================================
*/
void T100_MemEditor::DeleteAllMarkers(void)
{
	int					region;
	memedit_marker_t*	pMarkers;
	memedit_marker_t*	pNextMarker;

	// Determine which region marker to use.  For RAM banks 1,2,3,
	// we use the REGION_RAM marker so they all have the same
	// set of markers
	region = m_Region;
	if (region == REGION_RAM1 || region == REGION_RAM2 || region == REGION_RAM3)
		region = REGION_RAM;

	// First test if we have an existing undo delete all markers list
	pMarkers = m_pUndoDeleteMarkers[region];
	while (pMarkers != NULL)
	{
		// Get pointer to next marker
		pNextMarker = pMarkers->pNext;

		// Delete this marker and move on to the next one
		delete pMarkers;
		pMarkers = pNextMarker;
	}

	// Get pointer to start of marker list
	pMarkers = m_pRegionMarkers[region];

	// Save it in the undo delete all markers buffer
	if (pMarkers != NULL)
	{
		m_pUndoDeleteMarkers[region] = pMarkers;
		m_pRegionMarkers[region] = NULL;
	}

	m_SelStartAddr = m_SelEndAddr = -1;
	redraw();
}

/*
============================================================================
Undoes the delete all markers for this region
============================================================================
*/
void T100_MemEditor::UndoDeleteAllMarkers(void)
{
	int					region;

	// Determine which region marker to use.  For RAM banks 1,2,3,
	// we use the REGION_RAM marker so they all have the same
	// set of markers
	region = m_Region;
	if (region == REGION_RAM1 || region == REGION_RAM2 || region == REGION_RAM3)
		region = REGION_RAM;

	// Check if we have an undo buffer for this region
	if (m_pUndoDeleteMarkers[region] != NULL)
	{
		// Check if we have new markers that have been created
		if (m_pRegionMarkers[region] != NULL)
		{
			// Do something.  Should we delete, merge or ask?
		}

		// Restore the old marker list
		m_pRegionMarkers[region] = m_pUndoDeleteMarkers[region];
		m_pUndoDeleteMarkers[region] = NULL;

		m_SelStartAddr = m_SelEndAddr = -1;
		redraw();
	}
}

/*
============================================================================
Deletes the marker identified by the Cursor.
============================================================================
*/
void T100_MemEditor::ResetMarkers(void)
{
	int					region;
	memedit_marker_t*	pMarkers;
	memedit_marker_t*	pNextMarker;

	// Determine which region marker to use.  For RAM banks 1,2,3,
	// we use the REGION_RAM marker so they all have the same
	// set of markers
	for (region = 0; region < REGION_MAX; region++)
	{
		// Get pointer to start of marker list
		pMarkers = m_pRegionMarkers[region];

		// Loop for all markers in this region
		while (pMarkers != NULL)
		{
			// Get pointer to next marker
			pNextMarker = pMarkers->pNext;

			// Delete this marker
			delete pMarkers;
			pMarkers = pNextMarker;
		}

		// Now delete the undo all markers (if any)
		// Get pointer to start of marker list
		pMarkers = m_pUndoDeleteMarkers[region];

		// Loop for all markers in this region
		while (pMarkers != NULL)
		{
			// Get pointer to next marker
			pNextMarker = pMarkers->pNext;

			// Delete this marker
			delete pMarkers;
			pMarkers = pNextMarker;
		}

		// NULL out the head pointers
		m_pUndoDeleteMarkers[region] = NULL;
		m_pRegionMarkers[region] = NULL;
	}
}
