/* memedit.cpp */

/* $Id: memedit.cpp,v 1.5 2008/03/09 16:33:56 kpettit1 Exp $ */

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
#include <FL/fl_draw.H>
#include <FL/Fl_Window.H>
#include <FL/Fl_Menu_Item.H>
#include <FL/Fl_Menu_Bar.H>
#include <FL/Fl_Box.H>
#include <FL/Fl_Tabs.H>
#include <FL/Fl_Scrollbar.H>
#include <FL/Fl_Check_Button.H>
#include <FL/Fl_Round_Button.H>
#include <FL/Fl_Choice.H>
#include <FL/Fl_Input.H>  

#if defined(WIN32)
#include <windows.h>
#endif
#include <stdio.h>
#include <string.h>
#include <time.h>

#include "VirtualT.h"
#include "m100emu.h"
#include "disassemble.h"
#include "memedit.h"
#include "memory.h"
#include "cpu.h"
#include "cpuregs.h"
#include "fileview.h"

extern "C"
{
#include "intelhex.h"
}

void cb_Ide(Fl_Widget* w, void*); 

typedef struct memedit_ctrl_struct	
{
	Fl_Menu_Bar*		pMenu;

	T100_MemEditor*		pMemEdit;
	Fl_Scrollbar*		pScroll;
	Fl_Input*			pMemRange;
	Fl_Box*				pRegionText;
	Fl_Box*				pAddressText;
	Fl_Choice*			pRegion;

	char				sFilename[256];
	int					sAddrStart;
	int					sAddrEnd;
	int					sAddrTop;
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
int str_to_i(const char *pStr);

extern "C"
{
extern uchar			gReMem;			/* Flag indicating if ReMem emulation enabled */
extern uchar			gRampac;
void			memory_monitor_cb(void);
}


// Menu items for the disassembler
Fl_Menu_Item gMemEdit_menuitems[] = {
  { "&File",              0, 0, 0, FL_SUBMENU },
	{ "Load from File...",          0, cb_load, 0 },
	{ "Save to File...",      0, cb_save_memory, 0 },
	{ 0 },

  { "&Tools", 0, 0, 0, FL_SUBMENU },
	{ "CPU Registers",         0, cb_CpuRegs },
	{ "Assembler / IDE",       0, cb_Ide },
	{ "Disassembler",          0, disassembler_cb },
	{ "Peripheral Devices",    0, cb_PeripheralDevices },
//	{ "Simulation Log Viewer", 0, 0 },
	{ "Model T File Viewer",   0, cb_FileView },
	{ 0 },

  { 0 }
};


memedit_ctrl_t		memedit_ctrl;
memedit_dialog_t	gDialog;
Fl_Window		*gmew;				// Global Memory Edit Window
unsigned char	gDispMemory[8192];

/*
============================================================================
Callback routine for the Peripherial Devices window
============================================================================
*/
void cb_memeditwin (Fl_Widget* w, void*)
{
	gmew->hide();
	mem_clear_monitor_callback(memory_monitor_cb);
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
	int size = memedit_ctrl.pRegion->value();

	// Set new region here
	memedit_ctrl.pMemEdit->SetScrollSize();
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
	address = str_to_i(pStr);

	// Set new region here
	memedit_ctrl.pMemEdit->MoveTo(address);
}


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

void load_cancelButton_cb(Fl_Widget* w, void*)
{
	gDialog.iOk = 0;
	gDialog.pWin->hide();
}

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
	int					len;
	unsigned char		*buffer;
	unsigned short		start_addr;

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
			set_memory8_ext(memedit_ctrl.pMemEdit->GetRegionEnum(), address, len, buffer);
		}

		// Free the memory
		delete buffer;
	}
	else
	{
		// Open file as binary
		FILE* fd;
		if ((fd = fopen(filename, "r")) != NULL)
		{
			// Determine file length
			fseek(fd, 0, SEEK_END);
			len = ftell(fd);
			buffer = new unsigned char[len];
			fseek(fd, 0, SEEK_SET);
			fread(buffer, 1, len, fd);
			set_memory8_ext(memedit_ctrl.pMemEdit->GetRegionEnum(), address, len, buffer);
			fclose(fd);

			delete buffer;
		}
		
	}
}

void save_mem_to_file(const char *filename, int address, int count, int save_as_hex)
{
	unsigned char		*buffer;
	FILE				*fd;


	// Check for hex file
	if (save_as_hex)
	{
		// Try to open the file for write
		fd = fopen(filename, "w+");
		if (fd == NULL)
			return;

		save_hex_file_ext(address, address+count-1, memedit_ctrl.pMemEdit->GetRegionEnum(), fd);

		fclose(fd);
	}
	else
	{
		// Open file as binary
		FILE* fd;
		if ((fd = fopen(filename, "wb+")) != NULL)
		{
			buffer = new unsigned char[count];
			get_memory8_ext(memedit_ctrl.pMemEdit->GetRegionEnum(), address, count, buffer);
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

	Fl_File_Chooser file(".","*.{bin,hex}",1,"Select File to Open");
	file.show();

	while (file.visible())
		Fl::wait();

	// Get filename
	count = file.count();
	if (count != 1)
		return;
	filename = file.value(1);
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
	Fl_File_Chooser file(".", filespec, Fl_File_Chooser::CREATE, "Select File to Open");

	while (1)
	{
		file.show();
		while (file.visible())
			Fl::wait();

		// Get filename
		count = file.count();
		if (count != 1)
			return;
		filename = file.value(1);
		if (filename == 0)
			return;

		// Try to open the ouput file
		fd = fopen(filename, "rb+");
		if (fd != NULL)
		{
			// File already exists! Check for overwrite!
			fclose(fd);

			c = fl_choice("Overwrite existing file?", "Cancel", "Yes", "No");

			// Test if Cancel selected
			if (c == 0)
				return;

			if (c == 1)
				break;
		}
	}

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
	gmew->take_focus();
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
Routine to create the MemoryEditor Window
============================================================================
*/
void cb_MemoryEditor (Fl_Widget* w, void*)
{
	Fl_Box*		o;

	if (gmew != NULL)
		return;

	// Create Peripheral Setup window
	gmew = new Fl_Window(585, 400, "Memory Editor");
	gmew->callback(cb_memeditwin);

	// Create a menu for the new window.
	memedit_ctrl.pMenu = new Fl_Menu_Bar(0, 0, 585, MENU_HEIGHT-2);
	memedit_ctrl.pMenu->menu(gMemEdit_menuitems);

	// Create static text boxes
	o = new Fl_Box(FL_NO_BOX, 10, 10+MENU_HEIGHT, 50, 15, "Region");
	o->align(FL_ALIGN_LEFT|FL_ALIGN_INSIDE);
	o = new Fl_Box(FL_NO_BOX, 175, 10+MENU_HEIGHT, 50, 15, "Address");
	o->align(FL_ALIGN_LEFT|FL_ALIGN_INSIDE);
	
	// Create Region choice box
	memedit_ctrl.pRegion = new Fl_Choice(70, 8+MENU_HEIGHT, 100, 20, "");
	memedit_ctrl.pRegion->callback(cb_region);

	// Create Filename edit field
	memedit_ctrl.pMemRange = new Fl_Input(235, 8+MENU_HEIGHT, 210, 20, "");
	memedit_ctrl.pMemRange->callback(cb_memory_range);
//	memedit_ctrl.pMemRange->deactivate();

	memedit_ctrl.pMemEdit = new T100_MemEditor(10, 40+MENU_HEIGHT, 545, 350-MENU_HEIGHT);

	memedit_ctrl.pScroll = new Fl_Scrollbar(555, 40+MENU_HEIGHT, 15, 350-MENU_HEIGHT, "");
	memedit_ctrl.pScroll->type(FL_VERTICAL);
	memedit_ctrl.pScroll->linesize(2);
	memedit_ctrl.pScroll->maximum(2);
	memedit_ctrl.pScroll->callback(cb_MemEditScroll);
	memedit_ctrl.pScroll->slider_size(1);

 
	// Make things resizeable
	gmew->resizable(memedit_ctrl.pMenu);
	gmew->resizable(memedit_ctrl.pMemEdit);

	memedit_ctrl.pMemEdit->SetScrollSize();

	// Set Memory Monitor callback
	mem_set_monitor_callback(memory_monitor_cb);

	gmew->show();
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

	// Redraw the memory editor
	memedit_ctrl.pMemEdit->redraw();
}

T100_MemEditor::T100_MemEditor(int x, int y, int w, int h) :
  Fl_Widget(x, y, w, h)
{
	m_MyFocus = 0;
	m_FirstLine = 0;
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
	SetRegionOptions();
}

T100_MemEditor::~T100_MemEditor()
{
}

void T100_MemEditor::SetRegionOptions(void)
{
	memedit_ctrl.pRegion->clear();

	// Update the Regions control with appropriate options
	if (gReMem)
	{
		memedit_ctrl.pRegion->add("RAM");
		memedit_ctrl.pRegion->add("Flash 1");
		memedit_ctrl.pRegion->add("Flash 2");
		memedit_ctrl.pRegion->value(0);
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

void T100_MemEditor::SetScrollSize(void)
{
	double	size;
	int		height;
	int		region;

	// Test for ReMem emulation mode
	if (gReMem)
	{
		// All ReMem blocks are 2Meg = 128K lines * 16 bytes
		m_Max = 1024 * 2048 / 16;
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
	fl_font(FL_COURIER, 12);

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

int T100_MemEditor::GetRegionEnum(void)
{
	char	reg_text[16];

	// Get region text from control
	strcpy(reg_text, memedit_ctrl.pRegion->text());

	// Test if RAM region is selected
	if (strcmp(reg_text, "RAM") == 0)
	{
		if (gReMem)
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

	return m_Region;

}

void T100_MemEditor::UpdateDispMem(void)
{
	int				count, c;
	unsigned char	mem[8192];
	int				xpos, ypos;
	char			string[6];
	int				modified = 0;

	// Calculate # bytes to read
	count = m_Lines * 16;

	// Read memory 
	get_memory8_ext(m_Region, m_FirstAddress, count, mem);

	// Set the display
	window()->make_current();

	// Select 12 point Courier font
	fl_font(FL_COURIER, 12);

	// Check for changes
	for (c = 0; c < count; c++)
		if (mem[c] != gDispMemory[c])
		{
			gDispMemory[c] = mem[c];

			// Display new text
			sprintf(string, "%02X", mem[c]);

			// Calculate xpos and ypos of the text
			xpos = (int) (x() + ((c&0x0F)*3+9+((c&0x0F)>7)) * m_Width);
			ypos = (int) (y() + ((c>>4)+1) * m_Height);
			
			// Draw Address text for line
			fl_color(FL_WHITE);
			fl_rectf(xpos, ypos - (int) m_Height, (int) m_Width*2, (int) m_Height);
			fl_color(FL_BLACK);
			fl_draw(string, xpos, ypos-2);

			// Draw the new ASCII value
			if ((mem[c] >= ' ') && (mem[c] <= '~'))
				sprintf(string, "%c", mem[c]);
			else
				strcpy(string, ".");
			xpos = (int) (x() + (59 + (c&0x0F)) * m_Width);
			fl_color(FL_WHITE);
			fl_rectf(xpos, ypos - (int) m_Height, (int) m_Width, (int) m_Height);
			fl_color(FL_BLACK);
			fl_draw(string, xpos, ypos-2);

			// Set modified flag
			modified = 1;
		}

	if (modified)
		DrawCursor();
}


// Redraw the whole LCD
void T100_MemEditor::draw()
{
	int				lines, cols;
	int				line;
	unsigned char	line_bytes[16];		// Bytes for line being displayed
	int				addr;
	char			string[10];
	int				xpos, ypos;
	int				c, max;
	int				region;				// Region where memory is being drawn from

	// Select 12 point Courier font
	fl_font(FL_COURIER, 12);

	// Get character width & height
	m_Width = fl_width("W", 1);
	m_Height = fl_height();
	
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
    fl_color(FL_WHITE);
    fl_rectf(x(),y(),w(),h());

    fl_color(FL_BLACK);
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
		if (addr >= max)
			break;
		if ((region == REGION_RAM) || (region == REGION_RAM2) || 
			(region == REGION_RAM3) || region == REGION_RAM1)
			sprintf(string, "%06X  ", addr + RAMSTART);
		else
			sprintf(string, "%06X  ", addr);

		// Calculate xpos and ypos of the text
		xpos = (int) (x() + m_Width);
		ypos = (int) (y() + (line+1) * m_Height);
		
		// Draw Address text for line
		fl_draw(string, xpos, ypos-2);

		// Read memory for this line
		get_memory8_ext(region, addr, 16, line_bytes);

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
				fl_draw(string, xpos, ypos-2);
		}

		// Calculate xpos for first byte
		xpos = (int) (x() + (16*3 + 11) * m_Width);

		string[1] = 0;

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
				fl_draw(string, xpos, ypos-2);

			// Calculate xpos of next byte 
			xpos += (int) m_Width;
		}

		// Increment Line number
		line++;
	}

	if (m_MyFocus)
		DrawCursor();
}

// Handle mouse events, key events, focus events, etc.
int T100_MemEditor::handle(int event)
{
	int				c, xp, yp, shift;
	int				line_click, col_click, cnt;
	int				col, line;
	int				lineIndex;
	int				height, size;
	int				value;
	unsigned int	key;
	int				first, address;
	unsigned char	data;
	int				xpos, ypos;
	char			string[6];

	switch (event)
	{
	case FL_FOCUS:
		m_MyFocus = 1;
		break;

	case FL_UNFOCUS:
		m_MyFocus = 0;

		// Make our window the current window for drawing
		window()->make_current();

		// "Unselect" previous start or stop char
		EraseCursor();
		break;

	case FL_PUSH:
		// Get id of mouse button pressed
		c = Fl::event_button();

		// We must take the focus so keystrokes work properly
		take_focus();

		// Make our window the current window for drawing
		window()->make_current();

		// Check if it was the Left Mouse button
		if ((c == FL_LEFT_MOUSE) || (c == FL_RIGHT_MOUSE))
		{
			// Get X,Y position of button press
			xp = Fl::event_x();
			yp = Fl::event_y();

			// Check if Shift was depressed during the Mouse Button event
			if (Fl::get_key(FL_Shift_L) || Fl::get_key(FL_Shift_R))
				shift = 1;
			else
				shift = 0;


			if (c == FL_RIGHT_MOUSE)
				shift = 1;

			// Determine line & col of mouse click
			col_click = (int) ((xp - x()) / m_Width);
			line_click = (int) ((yp - y()) / m_Height);

			// Constrain Column base on geometry
			if (col_click < 9)
				col_click = 9;

			// Constrain click to 1st column in of value in first field
			if (col_click < 24+9)
				col_click = (int) ((col_click-9) / 3) * 3 + 9;
			else if (col_click < 59)
				col_click = (int) ((col_click-25-9) / 3) * 3 + 25+9;
			else if (col_click > 74)
				col_click = 74;

			// Get LineStart index
			lineIndex = (m_FirstLine + line_click) >> 1;

			cnt = 0;
			col = 0;
			line = lineIndex << 1;

			// "Unselect" previous start or stop char
			window()->make_current();

			// Select 12 point Courier font
			fl_font(FL_COURIER, 12);

			// Erase current cursor
			EraseCursor();

			// Save new cursor position
			m_CursorCol = col_click;
			m_CursorRow = line_click;
			if (col_click >= 59)
				m_CursorField = 2;
			else
				m_CursorField = 1;

			// Draw new cursor
			DrawCursor();
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
							fl_font(FL_COURIER, 12);

							// Get character width & height
							m_Height = fl_height();
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
							fl_font(FL_COURIER, 12);

							// Get character width & height
							m_Height = fl_height();
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

					// Determine if this is the first keystroke at this address
					if (first)
					{
						data = 0;
						set_memory8_ext(m_Region, address, 1, &data);
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

					// Display new text
					// Select 12 point Courier font
					fl_font(FL_COURIER, 12);

					sprintf(string, "%02X", data);

					// Calculate xpos and ypos of the text
					xpos = (int) (x() + (col*3+9+(col>7)) * m_Width);
					ypos = (int) (y() + (m_CursorRow+1) * m_Height);
					
					// Set the display
					window()->make_current();

					// Draw Address text for line
					fl_color(FL_WHITE);
					fl_rectf(xpos, ypos - (int) m_Height, (int) m_Width*2, (int) m_Height);
					fl_color(FL_BLACK);
					fl_draw(string, xpos, ypos-2);

					// Draw the new ASCII value
					if ((data >= ' ') && (data <= '~'))
						sprintf(string, "%c", data);
					else
						strcpy(string, ".");
					xpos = (int) (x() + (59 + col) * m_Width);
					fl_color(FL_WHITE);
					fl_rectf(xpos, ypos - (int) m_Height, (int) m_Width, (int) m_Height);
					fl_color(FL_BLACK);
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
								fl_font(FL_COURIER, 12);

								// Get character width & height
								m_Height = fl_height();
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
					address += col;

					// Write new value to memory
					data = key;
					set_memory8_ext(m_Region, address, 1, &data);

					// Display new text
					// Select 12 point Courier font
					fl_font(FL_COURIER, 12);

					sprintf(string, "%02X", data);

					// Calculate xpos and ypos of the text
					xpos = (int) (x() + (col*3+9+(col>7)) * m_Width);
					ypos = (int) (y() + (m_CursorRow+1) * m_Height);
					
					// Set the display
					window()->make_current();

					// Draw Address text for line
					fl_color(FL_WHITE);
					fl_rectf(xpos, ypos - (int) m_Height, (int) m_Width*2, (int) m_Height);
					fl_color(FL_BLACK);
					fl_draw(string, xpos, ypos-2);

					// Draw the new ASCII value
					if ((data >= ' ') && (data <= '~'))
						sprintf(string, "%c", data);
					else
						strcpy(string, ".");
					xpos = (int) (x() + (59 + col) * m_Width);
					fl_color(FL_WHITE);
					fl_rectf(xpos, ypos - (int) m_Height, (int) m_Width, (int) m_Height);
					fl_color(FL_BLACK);
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
							fl_font(FL_COURIER, 12);

							// Get character width & height
							m_Height = fl_height();
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

			break;
			
		}
		break;

	default:
		Fl_Widget::handle(event);
		break;

	}

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
	fl_font(FL_COURIER, 12);

	// Get height of screen in lines
	m_Height = fl_height();
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

	/* Calculate line value */
	value = address / 16;
	col = address % 16;

	// Erase the cursor, if any */
	EraseCursor();

	// Select 12 point Courier font
	fl_font(FL_COURIER, 12);

	// Get height of screen in lines
	m_Height = fl_height();
	height = memedit_ctrl.pScroll->h();
	size = (int) (height / m_Height);

	// Test if at bottom and scrolling down
	if (value > m_Max - size)
		value = (int) (m_Max - size);

	// Update new value base on scroll distance requested
	if (value < 0)
		value = 0;

	// Update scroll bar
	memedit_ctrl.pScroll->value(value, 1, 0, (int) m_Max);
	memedit_ctrl.pScroll->maximum(m_Max-size);
	memedit_ctrl.pScroll->step(size / m_Max);
	memedit_ctrl.pScroll->slider_size(size/m_Max);
	memedit_ctrl.pScroll->linesize(1);
	redraw();

	/* Set cursor row and column */
	m_CursorRow = address / 16 - value;
	if (m_CursorField == 0)
		m_CursorField = 1;
	if (m_CursorField == 2)
		m_CursorCol = 59 + col;
	else
		m_CursorCol = 9 + col * 3 + (col > 8 ? 1 : 0);

	DrawCursor();
}

// Erase the cursor at its current position
void T100_MemEditor::EraseCursor()
{
	int		x_pos, y_pos;

	// Erase current cursor
	fl_color(FL_WHITE);
	if (m_CursorRow != -1)
	{
		x_pos = (int) (m_CursorCol * m_Width + x());
		y_pos = (int) (m_CursorRow * m_Height + y());
		fl_line(x_pos-1, y_pos, x_pos-1, y_pos + (int) m_Height);
		fl_line(x_pos, y_pos, x_pos, y_pos + (int) m_Height);
	}

}

// Erase the cursor at its current position
void T100_MemEditor::DrawCursor()
{
	int		x_pos, y_pos;

	if (!m_MyFocus)
		return;

	// Draw new cursor
	fl_color(FL_BLACK);
	if (m_CursorRow != -1)
	{
		x_pos = (int) (m_CursorCol * m_Width + x());
		y_pos = (int) (m_CursorRow * m_Height + y());
		fl_line(x_pos-1, y_pos, x_pos-1, y_pos + (int) m_Height-2);
		fl_line(x_pos, y_pos, x_pos, y_pos + (int) m_Height-2);
	}

	UpdateAddressText();
}

// Update the Address edit box base on the position of the cursor
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
			sprintf(string, "0x%06X", address + RAMSTART);
		else
			sprintf(string, "0x%04X", address + RAMSTART);
	}
	else
	{
		if (gReMem)
			sprintf(string, "0x%06X", address);
		else
			sprintf(string, "0x%04X", address);
	}
	memedit_ctrl.pMemRange->value(string);
}


