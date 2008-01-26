/* rememcfg.cpp */

/* $Id: rememcfg.cpp,v 1.0 2008/01/02 06:46:12 kpettit1 Exp $ */

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
#include <FL/Fl_Menu_Item.H>
#include <FL/Fl_Menu_Bar.H>

#if defined(WIN32)
#include <windows.h>
#endif
#include <stdio.h>
#include <string.h>
#include <time.h>

#include "cpuregs.h"
#include "rememcfg.h"
#include "m100emu.h"
#include "disassemble.h"
#include "periph.h"
#include "memedit.h"
#include "cpu.h"
#include "memory.h"
#include "io.h"
#include "VirtualT.h"

#define		MENU_HEIGHT	32

void	cb_Ide(Fl_Widget* w, void*);
int		str_to_i(const char *pStr);


/*
============================================================================
Global variables
============================================================================
*/
rememcfg_ctrl_t	rememcfg_ctrl;
Fl_Window		*grmcw = NULL;

static int		gMap;				// Map currently being edited
static int		gRegion;			// Region currently being edited
static int		gVector;			// Vector currently being edited
static int		gMapModified;		// Flag indicating if map has been modified
static char		gMapData[REMEM_MAP_SIZE];	// 512 bytes of Map Data being edited

// Menu items 
Fl_Menu_Item gRememCfg_menuitems[] = {
  { "&Tools", 0, 0, 0, FL_SUBMENU },
	{ "CPU Registers",         0, cb_CpuRegs },
	{ "Assembler / IDE",       0, cb_Ide },
	{ "Disassembler",          0, disassembler_cb },
	{ "Memory Editor",         0, cb_MemoryEditor },
	{ "Peripheral Devices",    0, cb_PeripheralDevices },
//	{ "Simulation Log Viewer", 0, 0 },
	{ "Model T File Viewer",   0, 0 },
	{ 0 },
  { 0 }
};

void cb_readStatus (Fl_Widget* w, void*)
{
	int		mode = inport(REMEM_MODE_PORT);

	rememcfg_ctrl.pRememEnable->value(mode & REMEM_MODE_NORMAL);
	rememcfg_ctrl.pIOEnable->value(mode & REMEM_MODE_IOENABLE);
	rememcfg_ctrl.pRampacEnable->value(mode & REMEM_MODE_RAMPAC);
	rememcfg_ctrl.pStatusMap->value((mode & REMEM_MODE_MAP_BITS) >> REMEM_MODE_MAP_SHIFT);
	sprintf(rememcfg_ctrl.sStatus, "0x%02X", mode);
	rememcfg_ctrl.pStatus->label(rememcfg_ctrl.sStatus);
}

void cb_writeStatus (Fl_Widget* w, void*)
{
	int 	mode;

	mode = (int) rememcfg_ctrl.pStatusMap->value() << REMEM_MODE_MAP_SHIFT;
	mode |= rememcfg_ctrl.pIOEnable->value() ? REMEM_MODE_IOENABLE : 0; 
	mode |= rememcfg_ctrl.pRampacEnable->value() ? REMEM_MODE_RAMPAC: 0; 
	mode |= rememcfg_ctrl.pRememEnable->value() ? REMEM_MODE_NORMAL: 0; 

	out(REMEM_MODE_PORT, mode);

	sprintf(rememcfg_ctrl.sStatus, "0x%02X", mode);
	rememcfg_ctrl.pStatus->label(rememcfg_ctrl.sStatus);
}

int	get_region_offset(int region)
{
	if (gModel != MODEL_T200)
		return region * 64;

	return (region >> 1) * 128 + (region & 1) * 0x50;
}

/*
============================================================================
Routine to read Map data from RAM chip to local storage for editing.
============================================================================
*/
void read_map_data(void)
{
	get_memory8_ext(REGION_REMEM_RAM, REMEM_MAP_OFFSET + gMap * REMEM_MAP_SIZE, 
		REMEM_MAP_SIZE, (unsigned char *) gMapData);
}

/*
============================================================================
Routine to read Map data from RAM chip to local storage for editing.
============================================================================
*/
void write_map_data(void)
{
	set_memory8_ext(REGION_REMEM_RAM, REMEM_MAP_OFFSET + gMap * REMEM_MAP_SIZE, 
		REMEM_MAP_SIZE, (unsigned char *) gMapData);
}

void add_memory_usage(void)
{
	int				block, c;
	unsigned char	mem[512];		// Read memory from ReMem 512 bytes at a time

	// First add RAM usage.  Usage is in 512 byte blocks
	for (block = 0; block < 4096-520; block++)
	{
		// Calulate base address
		get_memory8_ext(REGION_REMEM_RAM, block*512, 512, mem);

		// Loop through data searching for non-zero
		for (c = 0; c < 512; c++)
		{
			if (mem[c] != 0)
			{
				rememcfg_ctrl.pRAM->AddUsageEvent(block, block, REMCFG_SECTOR_COLOR);
				break;			// No need to search further
			}
		}
	}
	// Now add Flash1 usage.  Usage is in 512 byte blocks
	for (block = 0; block < 4096; block++)
	{
		// Calulate base address
		get_memory8_ext(REGION_FLASH1, block*512, 512, mem);

		// Loop through data searching for non-zero
		for (c = 0; c < 512; c++)
		{
			if (mem[c] != 0)
			{
				rememcfg_ctrl.pFlash1->AddUsageEvent(block, block, REMCFG_SECTOR_COLOR);
				break;			// No need to search further
			}
		}
	}
	// Now add Flash1 usage.  Usage is in 512 byte blocks
	for (block = 0; block < 4096; block++)
	{
		// Calulate base address
		get_memory8_ext(REGION_FLASH2, block*512, 512, mem);

		// Loop through data searching for non-zero
		for (c = 0; c < 512; c++)
		{
			if (mem[c] != 0)
			{
				rememcfg_ctrl.pFlash2->AddUsageEvent(block, block, REMCFG_SECTOR_COLOR);
				break;			// No need to search further
			}
		}
	}
}

/*
============================================================================
Routine to update the RAM and FLASH bars based on the current MAP and vector
selection.
============================================================================
*/
void update_mem_bars(void)
{
	int		c, mapdata;

	// Test if map and region are valid
	if ((gMap == -1) || (gRegion == -1))
		return;

	rememcfg_ctrl.pRAM->ClearUsageMap();
	rememcfg_ctrl.pFlash1->ClearUsageMap();
	rememcfg_ctrl.pFlash2->ClearUsageMap();

	// Add Memory usage information
	if (rememcfg_ctrl.pShowUsage->value())
		add_memory_usage();

	// Loop through all vectors and add entries as appropriate
	for (c = 0; c < 256; c++)
	{
		// Skip unused areas
		if (((gModel != MODEL_T200) && (c & 0x20)) || (c >= 192))
			continue;

		mapdata = (unsigned char) gMapData[c*2] + ((unsigned char) gMapData[c*2+1] << 8);
		if (mapdata == 0)
			continue;
		
		if (!(mapdata & REMEM_VCTR_RAM_CS))
			rememcfg_ctrl.pRAM->AddUsageEvent((mapdata & REMEM_VCTR_ADDRESS)<<1, 
				((mapdata & REMEM_VCTR_ADDRESS)<<1)+1, REMCFG_MAPPING_COLOR);
		if (!(mapdata & REMEM_VCTR_FLASH1_CS))
			rememcfg_ctrl.pFlash1->AddUsageEvent((mapdata & REMEM_VCTR_ADDRESS)<<1, 
				((mapdata & REMEM_VCTR_ADDRESS)<<1)+1, REMCFG_MAPPING_COLOR);
		if (!(mapdata & REMEM_VCTR_FLASH2_CS))
			rememcfg_ctrl.pFlash2->AddUsageEvent((mapdata & REMEM_VCTR_ADDRESS)<<1, 
				((mapdata & REMEM_VCTR_ADDRESS)<<1)+1, REMCFG_MAPPING_COLOR);
	}

	// Add Rampac memory region
	rememcfg_ctrl.pRAM->AddUsageEvent(4096-512, 4095, REMCFG_RAMPAC_COLOR);

	// Add location of the map
	rememcfg_ctrl.pRAM->AddUsageEvent((REMEM_MAP_OFFSET + gMap*REMEM_MAP_SIZE) >> 9, 
		((REMEM_MAP_OFFSET + gMap*REMEM_MAP_SIZE) >> 9), REMCFG_MAPLOC_COLOR);
}

/*
============================================================================
Routine to disable the vector browser and the edit controls.
============================================================================
*/
void disable_vector_editing(void)
{
	rememcfg_ctrl.pVectorSelect->deactivate();
	rememcfg_ctrl.pSectorLock->deactivate();
	rememcfg_ctrl.pRamCS->deactivate();
	rememcfg_ctrl.pFlash1CS->deactivate();
	rememcfg_ctrl.pFlash2CS->deactivate();
	rememcfg_ctrl.pMapData->deactivate();
}

/*
============================================================================
Routine to disable the vector browser and the edit controls.
============================================================================
*/
void enable_vector_editing(void)
{
	rememcfg_ctrl.pVectorSelect->activate();
	rememcfg_ctrl.pSectorLock->activate();
	rememcfg_ctrl.pRamCS->activate();
	rememcfg_ctrl.pFlash1CS->activate();
	rememcfg_ctrl.pFlash2CS->activate();
	rememcfg_ctrl.pMapData->activate();
}

/*
============================================================================
Routine to update the active vector edit information with based on current
selection in the vector list.
============================================================================
*/
void save_vector_edits(void)
{
	int				mapdata, address;
	const char*		pStr;
	char			temp[20];

	if ((gRegion == -1) || (gVector == -1))
		return;

	pStr = rememcfg_ctrl.pMapData->value();	
	mapdata = 0;
	if (pStr[0] != 0)
	{
		mapdata = str_to_i(pStr) & REMEM_VCTR_ADDRESS;
	}
	mapdata |= rememcfg_ctrl.pSectorLock->value() ? REMEM_VCTR_READ_ONLY : 0;
	mapdata |= rememcfg_ctrl.pRamCS->value() ? REMEM_VCTR_RAM_CS : 0;
	mapdata |= rememcfg_ctrl.pFlash1CS->value() ? REMEM_VCTR_FLASH1_CS : 0;
	mapdata |= rememcfg_ctrl.pFlash2CS->value() ? REMEM_VCTR_FLASH2_CS : 0;

	address = get_region_offset(gRegion) + (gVector * 2);
	if (((unsigned char) gMapData[address] != (mapdata & 0xFF)) || 
		((unsigned char) gMapData[address+1] != (mapdata >> 8)))
	{
		gMapData[address] = mapdata & 0xFF;
		gMapData[address+1] = mapdata >> 8;
		gMapModified = TRUE;
	}

	// Update the vector browser text
	sprintf(temp, "%02d  0x%04X", gVector, mapdata);
	rememcfg_ctrl.pVectorSelect->text(gVector+1, temp);
}

/*
============================================================================
Routine to update the active vector edit information with based on current
selection in the vector list.
============================================================================
*/
void update_edit_info(void)
{
	int		region, vector;
	int		mapdata;
	char	temp[20];
	int		regionAddr;

	region = rememcfg_ctrl.pRegionSelect->value();
	vector = rememcfg_ctrl.pVectorSelect->value();

	if ((region == 0) || (vector == 0))
	{
		return;
	}

	region--;
	vector--;

	regionAddr = get_region_offset(region);

	// Get map data from local storage
	mapdata = (unsigned char) gMapData[regionAddr + (vector<<1)] + 
		((unsigned char) gMapData[regionAddr + (vector<<1) + 1] << 8); 

	// Update controls base on map data
	rememcfg_ctrl.pSectorLock->value(mapdata & REMEM_VCTR_READ_ONLY);
	rememcfg_ctrl.pRamCS->value(mapdata & REMEM_VCTR_RAM_CS);
	rememcfg_ctrl.pFlash1CS->value(mapdata & REMEM_VCTR_FLASH1_CS);
	rememcfg_ctrl.pFlash2CS->value(mapdata & REMEM_VCTR_FLASH2_CS);
	sprintf(rememcfg_ctrl.sVectorAddress, "0x1BF%03X", gMap * REMEM_MAP_SIZE + 
		regionAddr + vector * 2);
	rememcfg_ctrl.pVectorAddress->label(rememcfg_ctrl.sVectorAddress);
	sprintf(temp, "0x%03X", mapdata & REMEM_VCTR_ADDRESS);
	rememcfg_ctrl.pMapData->value(temp);
}

/*
============================================================================
Routine to update the vector select browser with accurate data base on the
current map and region selections.
============================================================================
*/
void update_vector_browser(void)
{
	int		region, start;
	int		c;
	char	temp[20];
	int		vectors;

	region = rememcfg_ctrl.pRegionSelect->value() - 1;

	// If no region is selected, disable the vector browser
	if (region == -1)
	{
		disable_vector_editing();
		return;
	}

	// Enable editing
	enable_vector_editing();

	vectors = gModel == MODEL_T200 ? 40 - (region & 1) * 16 : 32;

	// Add or delete vectors in T200 emulation mode as needed
	if (gModel == MODEL_T200)
	{
		if ((vectors == 40) && (rememcfg_ctrl.pVectorSelect->size() == 24))
		{
			for (c = 0; c < 16; c++)
				rememcfg_ctrl.pVectorSelect->add("XX  0000");
		}
		else if ((vectors == 24) && (rememcfg_ctrl.pVectorSelect->size() == 40))
		{
			for (c = 0; c < 16; c++)
				rememcfg_ctrl.pVectorSelect->remove(25);
			if (rememcfg_ctrl.pVectorSelect->value() == 0)
			{
				rememcfg_ctrl.pVectorSelect->select(1);
				rememcfg_ctrl.pVectorSelect->show(1);
			}
		}
	}

	// Now populate the entries with the proper values
	start = get_region_offset(region);
	for (c = 0; c < vectors; c++)
	{
		sprintf(temp, "%02d  0x%02X%02X", c, (unsigned char)gMapData[start+(c<<1)+1], 
			(unsigned char) gMapData[start+(c<<1)]);
		rememcfg_ctrl.pVectorSelect->text(c+1, temp);
	}

	// Now update the vector edit controls based on new vector browser data
	update_edit_info();
}

/*
============================================================================
Callback routine for the RememCfg window
============================================================================
*/
void cb_rememcfgwin (Fl_Widget* w, void*)
{
	int		ret;
	char	temp[40];

	// Test if current map was modified
	save_vector_edits();
	if (gMapModified)
	{
		sprintf(temp, "Save Map %d changes to ReMem RAM?", gMap);
		// Ask user if map should be saved
		ret = fl_choice(temp, "Cancel", "Yes", "No");

		// Test for Cancel
		if (ret == 0)
			return;

		// Test for "Yes"
		if (ret == 1)
			write_map_data();
	}

	// Hide the window
	grmcw->hide();

	// Delete the window and set to NULL
	delete grmcw;
	grmcw = NULL;
}

/*
============================================================================
Callback function for the Map Select browser.
============================================================================
*/
void cb_mapselect (Fl_Widget* w, void*)
{
	int		ret, map;
	char	temp[40];

	save_vector_edits();			// Save any vector modifications

	map = ((Fl_Browser*)w)->value()-1;
	if (map == gMap)
		return;

	// Test if current map was modified
	if (gMapModified)
	{
		sprintf(temp, "Save Map %d changes to ReMem RAM?", gMap);

		// Ask user if map should be saved
		ret = fl_choice(temp, "Cancel", "Yes", "No");

		// Test for Cancel
		if (ret == 0)
		{
			rememcfg_ctrl.pMapSelect->select(gMap+1);
			return;
		}

		// Test for "Yes"
		if (ret == 1)
			write_map_data();

		gMapModified = FALSE;
	}


	// Set new gMap value
	gMap = map;

	// Read the map data from ReMem RAM
	read_map_data();

	update_vector_browser();

	// Draw new memory bar information
	update_mem_bars();

	// Update the edit info based on current selection
	update_edit_info();

}

void cb_regionselect (Fl_Widget* w, void*)
{
	save_vector_edits();
	update_vector_browser();
	gRegion = ((Fl_Browser*)w)->value()-1;
}

void cb_vectorselect (Fl_Widget* w, void*)
{
	save_vector_edits();
	update_edit_info();
	gVector = ((Fl_Browser*)w)->value()-1;
}

/*
============================================================================
Callback routine for Save Map button
============================================================================
*/
void cb_save_map (Fl_Widget* w, void*)
{
	write_map_data();
	gMapModified = FALSE;
}

/*
============================================================================
Callback routine for Load Map button
============================================================================
*/
void cb_revert_map (Fl_Widget* w, void*)
{
	read_map_data();
	update_vector_browser();
	update_mem_bars();
}

void create_default_map(int map)
{
	int 		c, mapdata;

	// First zero all map memory
	for (c = 0; c < sizeof(gMapData); c++)
		gMapData[c] = 0;

	// Now create defaults for each region
	if (gModel == MODEL_T200)
	{
		// Rom Region
		mapdata = 0x7000;					// Default for all maps
		for (c = 0; c < 40; c++)
		{
			gMapData[c*2] = mapdata & 0xFF;
			gMapData[c*2 + 1] = mapdata >> 8;
			mapdata++;
		}
	
		// RAM 1 Region
		if (map == 0)
		{
			mapdata = 0x1800;
			for (c = 40; c < 64; c++)
			{
				gMapData[c*2] = mapdata & 0xFF;
				gMapData[c*2 + 1] = mapdata >> 8;
				mapdata++;
			}
		}	
		else if (map == 1)
		{
			mapdata = 0x1818;
			for (c = 40; c < 48; c++)
			{
				gMapData[c*2] = mapdata & 0xFF;
				gMapData[c*2 + 1] = mapdata >> 8;
				mapdata++;
			}
			mapdata = 0x1838;
			for (c = 48; c < 56; c++)
			{
				gMapData[c*2] = mapdata & 0xFF;
				gMapData[c*2 + 1] = mapdata >> 8;
				mapdata++;
			}
			mapdata = 0x1858;
			for (c = 56; c < 64; c++)
			{
				gMapData[c*2] = mapdata & 0xFF;
				gMapData[c*2 + 1] = mapdata >> 8;
				mapdata++;
			}
		}
		else
		{
			mapdata = 0x1890 + (map-2) * 72 ;
			for (c = 40; c < 64; c++)
			{
				gMapData[c*2] = mapdata & 0xFF;
				gMapData[c*2 + 1] = mapdata >> 8;
				mapdata++;
			}
		}
		// MSPLAN ROM
		mapdata = 0x7040;
		for (c = 64; c < 96; c++)
		{
			gMapData[c*2] = mapdata & 0xFF;
			gMapData[c*2 + 1] = mapdata >> 8;
			mapdata++;
		}
		// RAM 2
		if (map == 0)
		{
			mapdata = 0x1820;
			for (c = 104; c < 104+24; c++)
			{
				gMapData[c*2] = mapdata & 0xFF;
				gMapData[c*2 + 1] = mapdata >> 8;
				mapdata++;
			}
		}
		else
		{
			mapdata = 0x1860 + (map-1) * 72;
			for (c = 104; c < 104+24; c++)
			{
				gMapData[c*2] = mapdata & 0xFF;
				gMapData[c*2 + 1] = mapdata >> 8;
				mapdata++;
			}
		}
		// Opt ROM
		mapdata = 0x7060 + map*0x20;
		for (c = 128; c < 128+32; c++)
		{
			gMapData[c*2] = mapdata & 0xFF;
			gMapData[c*2 + 1] = mapdata >> 8;
			mapdata++;
		}
		// RAM 3
		if (map == 0)
		{
			mapdata = 0x1840;
			for (c = 168; c < 168+24; c++)
			{
				gMapData[c*2] = mapdata & 0xFF;
				gMapData[c*2 + 1] = mapdata >> 8;
				mapdata++;
			}
		}
		else
		{
			mapdata = 0x1878 + (map-1) * 72;
			for (c = 168; c < 168+24; c++)
			{
				gMapData[c*2] = mapdata & 0xFF;
				gMapData[c*2 + 1] = mapdata >> 8;
				mapdata++;
			}
		}
	}
	else
	{
		// Rom Region
		mapdata = 0x7000;					// Default for all maps
		for (c = 0; c < 32; c++)
		{
			gMapData[c*2] = mapdata & 0xFF;
			gMapData[c*2 + 1] = mapdata >> 8;
			mapdata++;
		}

		// RAM Region
		mapdata = 0x1800 + 0x20 * map;
		for (c = 0x40; c < 0x60; c++)
		{
			gMapData[c*2] = mapdata & 0xFF;
			gMapData[c*2 + 1] = mapdata >> 8;
			mapdata++;
		}

		// Opt ROM Region
		mapdata = 0x7000 + 0x20 * (map+1);
		for (c = 0x80; c < 0xA0; c++)
		{
			gMapData[c*2] = mapdata & 0xFF;
			gMapData[c*2 + 1] = mapdata >> 8;
			mapdata++;
		}
	}
}
/*
============================================================================
Callback routine for Default Map button
============================================================================
*/
void cb_default_map (Fl_Widget* w, void*)
{
	int ret, c, oldmap;

	ret = fl_choice("Create default map for what?", "Cancel", "Current Map", "All Maps");
	if (ret == 0)
		return;

	if (ret == 1)
	{
		create_default_map(gMap);
		gMapModified = TRUE;
	}

	if (ret == 2)
	{
		oldmap = gMap;
		for (c = 0; c < 8; c++)
		{
			gMap = c;
			create_default_map(c);
			write_map_data();
		}
		gMap = oldmap;
		read_map_data();
		gMapModified = FALSE;
	}

	update_vector_browser();
	update_mem_bars();
}

/*
============================================================================
Callback routine for Copy Sequential button
============================================================================
*/
void cb_copy_sequential (Fl_Widget* w, void*)
{
	int		mapdata, c, address, vectors;
	int		regionAddr;

	// Test if selection valid
	if ((gMap == -1) || (gRegion == -1) || (gVector == -1))
		return;

	// Get the map data for the current vector
	save_vector_edits();
	regionAddr = get_region_offset(gRegion);
	address = regionAddr + gVector * 2;
	mapdata = (unsigned char) gMapData[address] + ((unsigned char) gMapData[address+1] << 8);
	vectors = gModel == MODEL_T200 ? 40-16*(gRegion&1) : 32;
	
	// Copy current vector to remainder of region
	for (c = gVector+1; c < vectors; c++)
	{
		if (mapdata != 0)
			mapdata++;
		gMapData[regionAddr + c*2] = mapdata & 0xFF;
		gMapData[regionAddr + c*2 + 1] = mapdata >> 8;
	}

	gMapModified = 1;
	update_vector_browser();
	update_mem_bars();
}

/*
============================================================================
Callback routine for Show Memory Usage checkbox
============================================================================
*/
void cb_show_usage (Fl_Widget* w, void*)
{
	update_mem_bars();
}
/*
============================================================================
Routine to create the ReMemCfg Window and tabs
============================================================================
*/
void cb_RememCfg (Fl_Widget* w, void*)
{
	Fl_Box*			o;
	int				mode = inport(REMEM_MODE_PORT);
	char			temp[20];
	int				x, vectors;

	if (grmcw != NULL)
		return;

	// Create Peripheral Setup window
	grmcw = new Fl_Window(610, 520, "ReMem Configuration");
	grmcw->callback(cb_rememcfgwin);

	// Create a menu for the new window.
	rememcfg_ctrl.pMenu = new Fl_Menu_Bar(0, 0, 630, MENU_HEIGHT-2);
	rememcfg_ctrl.pMenu->menu(gRememCfg_menuitems);

	// Create a "Group Box" for the status / control items
	rememcfg_ctrl.g = new Fl_Group(15, 15 + MENU_HEIGHT, 580, 120, "");
	rememcfg_ctrl.g->box(FL_EMBOSSED_FRAME);

	o = new Fl_Box(FL_NO_BOX, 250, 30+MENU_HEIGHT, 100, 15, "FW Version:");
	o->align(FL_ALIGN_LEFT|FL_ALIGN_INSIDE);
	o = new Fl_Box(FL_NO_BOX, 30, 30+MENU_HEIGHT, 100, 15, "ReMem Status:");
	o->align(FL_ALIGN_LEFT|FL_ALIGN_INSIDE);
	o = new Fl_Box(FL_NO_BOX, 90, 65+MENU_HEIGHT, 100, 15, "Active Map");
	o->align(FL_ALIGN_LEFT|FL_ALIGN_INSIDE);
	o = new Fl_Box(FL_NO_BOX, 250, 65+MENU_HEIGHT, 100, 15, "I/O Enable");
	o->align(FL_ALIGN_LEFT|FL_ALIGN_INSIDE);
	o = new Fl_Box(FL_NO_BOX, 250, 85+MENU_HEIGHT, 100, 15, "Rampac Enable");
	o->align(FL_ALIGN_LEFT|FL_ALIGN_INSIDE);
	o = new Fl_Box(FL_NO_BOX, 250, 105+MENU_HEIGHT, 100, 15, "ReMem Enable");
	o->align(FL_ALIGN_LEFT|FL_ALIGN_INSIDE);

	rememcfg_ctrl.pFWVersion = new Fl_Box(360, 30+MENU_HEIGHT, 50, 15, "");
	sprintf(rememcfg_ctrl.sFW, "%d", inport(REMEM_REVID_PORT));
	rememcfg_ctrl.pFWVersion->label(rememcfg_ctrl.sFW);

	rememcfg_ctrl.pStatus = new Fl_Box(130, 30+MENU_HEIGHT, 50, 15, "");
	sprintf(rememcfg_ctrl.sStatus, "0x%02X", mode);
	rememcfg_ctrl.pStatus->label(rememcfg_ctrl.sStatus);

	// Create a slider for selecting the active map
	rememcfg_ctrl.pStatusMap = new Fl_Value_Slider(20, 85+MENU_HEIGHT, 200, 20, "");
	rememcfg_ctrl.pStatusMap->type(5);
	rememcfg_ctrl.pStatusMap->box(FL_FLAT_BOX);
	rememcfg_ctrl.pStatusMap->labelsize(8);
	rememcfg_ctrl.pStatusMap->precision(0);
	rememcfg_ctrl.pStatusMap->range(0, 7);
	rememcfg_ctrl.pStatusMap->value((mode & REMEM_MODE_MAP_BITS) >> REMEM_MODE_MAP_SHIFT);

	// Create a check box for the I/O Enable
	rememcfg_ctrl.pIOEnable = new Fl_Check_Button(375, 64+MENU_HEIGHT, 20,20,"");
	rememcfg_ctrl.pIOEnable->value(mode & REMEM_MODE_IOENABLE);
	rememcfg_ctrl.pRampacEnable = new Fl_Check_Button(375, 84+MENU_HEIGHT, 20,20,"");
	rememcfg_ctrl.pRampacEnable->value(mode & REMEM_MODE_RAMPAC);
	rememcfg_ctrl.pRememEnable = new Fl_Check_Button(375, 104+MENU_HEIGHT, 20,20,"");
	rememcfg_ctrl.pRememEnable->value(mode & REMEM_MODE_NORMAL);

	// Create HW Read and Write buttons 
	rememcfg_ctrl.pReadHW = new Fl_Button(430, 70+MENU_HEIGHT, 140, 20, "Read from HW");
	rememcfg_ctrl.pReadHW->callback(cb_readStatus);
	rememcfg_ctrl.pWriteHW = new Fl_Button(430, 100+MENU_HEIGHT, 140, 20, "Write to HW");
	rememcfg_ctrl.pWriteHW->callback(cb_writeStatus);

	rememcfg_ctrl.g->end();

	// Create a select browser to selet the map
	rememcfg_ctrl.pMapSelect = new Fl_Hold_Browser(20, 145+MENU_HEIGHT, 60, 140, 0);
	rememcfg_ctrl.pMapSelect->add("Map 0");
	rememcfg_ctrl.pMapSelect->add("Map 1");
	rememcfg_ctrl.pMapSelect->add("Map 2");
	rememcfg_ctrl.pMapSelect->add("Map 3");
	rememcfg_ctrl.pMapSelect->add("Map 4");
	rememcfg_ctrl.pMapSelect->add("Map 5");
	rememcfg_ctrl.pMapSelect->add("Map 6");
	rememcfg_ctrl.pMapSelect->add("Map 7");
	rememcfg_ctrl.pMapSelect->has_scrollbar(0);
	rememcfg_ctrl.pMapSelect->select(1);
	rememcfg_ctrl.pMapSelect->callback(cb_mapselect); 

	// Create a select browser to selet the vector
	rememcfg_ctrl.pRegionSelect = new Fl_Hold_Browser(110, 145+MENU_HEIGHT, 130, 140, 0);
	if (gModel != MODEL_T200)
	{
		rememcfg_ctrl.pRegionSelect->add("000-03F ROM"); 
		rememcfg_ctrl.pRegionSelect->add("040-07F unused"); 
		rememcfg_ctrl.pRegionSelect->add("080-0BF RAM"); 
		rememcfg_ctrl.pRegionSelect->add("0C0-0FF unused"); 
		rememcfg_ctrl.pRegionSelect->add("100-13F OPT"); 
		rememcfg_ctrl.pRegionSelect->add("140-17F unused"); 
		rememcfg_ctrl.pRegionSelect->add("180-1BF unused"); 
		rememcfg_ctrl.pRegionSelect->add("1C0-1FF unused"); 
	}
	else
	{
		rememcfg_ctrl.pRegionSelect->add("000-04F ROM"); 
		rememcfg_ctrl.pRegionSelect->add("050-07F RAM 1"); 
		rememcfg_ctrl.pRegionSelect->add("080-0CF MPLAN"); 
		rememcfg_ctrl.pRegionSelect->add("0D0-0FF RAM 2"); 
		rememcfg_ctrl.pRegionSelect->add("100-14F OPT"); 
		rememcfg_ctrl.pRegionSelect->add("150-17F RAM 3"); 
		rememcfg_ctrl.pRegionSelect->add("180-1CF unused"); 
		rememcfg_ctrl.pRegionSelect->add("1D0-1FF unused"); 
	}
	rememcfg_ctrl.pRegionSelect->has_scrollbar(0);
	rememcfg_ctrl.pRegionSelect->select(1);
	rememcfg_ctrl.pRegionSelect->callback(cb_regionselect); 

	rememcfg_ctrl.pVectorSelect = new Fl_Hold_Browser(260, 145+MENU_HEIGHT, 110, 140, 0);
	vectors = 40;
	if (gModel != MODEL_T200)
		vectors = 32;
	for (x = 0; x < vectors; x++)
	{
		sprintf(temp, "%02d  0x0000", x);
		rememcfg_ctrl.pVectorSelect->add(temp);
	}
	rememcfg_ctrl.pVectorSelect->callback(cb_vectorselect); 
	rememcfg_ctrl.pVectorSelect->select(1); 

	// Create controls for editing the vector
	o = new Fl_Box(FL_NO_BOX, 390, 145+MENU_HEIGHT, 100, 15, "RAM Address:");
	o->align(FL_ALIGN_LEFT|FL_ALIGN_INSIDE);
	rememcfg_ctrl.pVectorAddress = new Fl_Box(510, 145+MENU_HEIGHT, 100, 15, "");
	sprintf(rememcfg_ctrl.sVectorAddress, "0x1BF000");
	rememcfg_ctrl.pVectorAddress->label(rememcfg_ctrl.sVectorAddress);

	o = new Fl_Box(FL_NO_BOX, 390, 170+MENU_HEIGHT, 100, 15, "Sector Lock");
	o->align(FL_ALIGN_LEFT|FL_ALIGN_INSIDE);
	rememcfg_ctrl.pSectorLock = new Fl_Check_Button(500, 170+MENU_HEIGHT, 20,20,0);

	o = new Fl_Box(FL_NO_BOX, 390, 190+MENU_HEIGHT, 100, 15, "RAM CS");
	o->align(FL_ALIGN_LEFT|FL_ALIGN_INSIDE);
	rememcfg_ctrl.pRamCS = new Fl_Check_Button(500, 190+MENU_HEIGHT, 20,20,0);

	o = new Fl_Box(FL_NO_BOX, 390, 210+MENU_HEIGHT, 100, 15, "Flash 1 CS");
	o->align(FL_ALIGN_LEFT|FL_ALIGN_INSIDE);
	rememcfg_ctrl.pFlash1CS = new Fl_Check_Button(500, 210+MENU_HEIGHT, 20,20,0);

	o = new Fl_Box(FL_NO_BOX, 390, 230+MENU_HEIGHT, 100, 15, "Flash 2 CS");
	o->align(FL_ALIGN_LEFT|FL_ALIGN_INSIDE);
	rememcfg_ctrl.pFlash2CS = new Fl_Check_Button(500, 230+MENU_HEIGHT, 20,20,0);

	o = new Fl_Box(FL_NO_BOX, 390, 255+MENU_HEIGHT, 100, 15, "Map Data");
	o->align(FL_ALIGN_LEFT|FL_ALIGN_INSIDE);
	rememcfg_ctrl.pMapData = new Fl_Input(500, 255+MENU_HEIGHT, 90,20,0);

	// Create buttons for managing the map
	rememcfg_ctrl.pSaveMap = new Fl_Button(40, 295+MENU_HEIGHT, 80, 20, "Save Map");
	rememcfg_ctrl.pSaveMap->callback(cb_save_map);
	rememcfg_ctrl.pLoadMap = new Fl_Button(140, 295+MENU_HEIGHT, 90, 20, "Revert Map");
	rememcfg_ctrl.pLoadMap->callback(cb_revert_map);
	rememcfg_ctrl.pDefaultMap = new Fl_Button(260, 295+MENU_HEIGHT, 90, 20, "Default Map");
	rememcfg_ctrl.pDefaultMap->callback(cb_default_map);
	rememcfg_ctrl.pCopySequential = new Fl_Button(420, 295+MENU_HEIGHT, 120, 20, "Copy Sequential");
	rememcfg_ctrl.pCopySequential->callback(cb_copy_sequential);

	// Create a box for RAM
	o = new Fl_Box(FL_NO_BOX, 20, 365, 100, 15, "RAM");
	o->align(FL_ALIGN_LEFT|FL_ALIGN_INSIDE);
	rememcfg_ctrl.pRAM = new Fl_Usage_Box(80, 355, 516, 36);
	rememcfg_ctrl.pRAM->box(FL_BORDER_BOX);
	rememcfg_ctrl.pRAM->SetUsageColor(REMCFG_MAPPING_COLOR, fl_rgb_color(225, 225, 0));
	rememcfg_ctrl.pRAM->SetUsageColor(REMCFG_SECTOR_COLOR, fl_rgb_color(128, 128, 0));
	rememcfg_ctrl.pRAM->SetUsageColor(REMCFG_COMBO_COLOR, fl_rgb_color(225, 0, 0));
	rememcfg_ctrl.pRAM->SetUsageColor(REMCFG_RAMPAC_COLOR, fl_rgb_color(225, 225, 225));
	rememcfg_ctrl.pRAM->SetUsageColor(REMCFG_MAPLOC_COLOR, fl_rgb_color(0, 0, 225));
	rememcfg_ctrl.pRAM->SetUsageColor(REMCFG_MAPSEL_COLOR, fl_rgb_color(0, 0, 225));
	rememcfg_ctrl.pRAM->SetUsageRange(0, 4095);
	rememcfg_ctrl.pRAM->AddUsageEvent(4096-512, 4095, REMCFG_RAMPAC_COLOR);

	// Create a box for Flash1
	o = new Fl_Box(FL_NO_BOX, 20, 405, 100, 15, "Flash1");
	o->align(FL_ALIGN_LEFT|FL_ALIGN_INSIDE);
	rememcfg_ctrl.pFlash1 = new Fl_Usage_Box(80, 395, 516, 36);
	rememcfg_ctrl.pFlash1->box(FL_BORDER_BOX);
	rememcfg_ctrl.pFlash1->SetUsageColor(REMCFG_MAPPING_COLOR, fl_rgb_color(225, 225, 0));
	rememcfg_ctrl.pFlash1->SetUsageColor(REMCFG_SECTOR_COLOR, fl_rgb_color(128, 128, 0));
	rememcfg_ctrl.pFlash1->SetUsageColor(REMCFG_COMBO_COLOR, fl_rgb_color(225, 0, 0));
	rememcfg_ctrl.pFlash1->SetUsageRange(0, 4095);

	// Create a box for Flash2
	o = new Fl_Box(FL_NO_BOX, 20, 445, 100, 15, "Flash2");
	o->align(FL_ALIGN_LEFT|FL_ALIGN_INSIDE);
	rememcfg_ctrl.pFlash2 = new Fl_Usage_Box(80, 435, 516, 36);
	rememcfg_ctrl.pFlash2->box(FL_BORDER_BOX);
	rememcfg_ctrl.pFlash2->SetUsageColor(REMCFG_MAPPING_COLOR, fl_rgb_color(225, 225, 0));
	rememcfg_ctrl.pFlash2->SetUsageColor(REMCFG_SECTOR_COLOR, fl_rgb_color(128, 128, 0));
	rememcfg_ctrl.pFlash2->SetUsageColor(REMCFG_COMBO_COLOR, fl_rgb_color(225, 0, 0));
	rememcfg_ctrl.pFlash2->SetUsageRange(0, 4095);

	o = new Fl_Box(FL_NO_BOX, 20, 485, 100, 15, "Show Memory Usage");
	o->align(FL_ALIGN_LEFT|FL_ALIGN_INSIDE);
	rememcfg_ctrl.pShowUsage = new Fl_Check_Button(170, 484, 20,20,"");
	rememcfg_ctrl.pShowUsage->value(1);
	rememcfg_ctrl.pShowUsage->callback(cb_show_usage);

	// Create color code boxes
	o = new Fl_Box(FL_NO_BOX, 230, 480, 60, 15, "Free");
	o->align(FL_ALIGN_LEFT|FL_ALIGN_INSIDE);
	o = new Fl_Box(FL_DOWN_BOX, 290, 480, 15, 15, "");
	o->color(fl_rgb_color(255, 255, 255));

	o = new Fl_Box(FL_NO_BOX, 230, 500, 60, 15, "Used");
	o->align(FL_ALIGN_LEFT|FL_ALIGN_INSIDE);
	o = new Fl_Box(FL_DOWN_BOX, 290, 500, 15, 15, "");
	o->color(fl_rgb_color(128, 128, 0));

	o = new Fl_Box(FL_NO_BOX, 340, 480, 60, 15, "Mapped");
	o->align(FL_ALIGN_LEFT|FL_ALIGN_INSIDE);
	o = new Fl_Box(FL_DOWN_BOX, 410, 480, 15, 15, "");
	o->color(fl_rgb_color(255, 255, 0));

	o = new Fl_Box(FL_NO_BOX, 340, 500, 60, 15, "  (used)");
	o->align(FL_ALIGN_LEFT|FL_ALIGN_INSIDE);
	o = new Fl_Box(FL_DOWN_BOX, 410, 500, 15, 15, "");
	o->color(fl_rgb_color(255, 0, 0));

	o = new Fl_Box(FL_NO_BOX, 470, 480, 60, 15, "Map ");
	o->align(FL_ALIGN_LEFT|FL_ALIGN_INSIDE);
	o = new Fl_Box(FL_DOWN_BOX, 550, 480, 15, 15, "");
	o->color(fl_rgb_color(0, 0, 225));

	o = new Fl_Box(FL_NO_BOX, 470, 500, 60, 15, "Rampac");
	o->align(FL_ALIGN_LEFT|FL_ALIGN_INSIDE);
	o = new Fl_Box(FL_DOWN_BOX, 550, 500, 15, 15, "");
	o->color(fl_rgb_color(225, 225, 225));

	gMapModified = FALSE;
	gMap = 0;
	gRegion = 0;
	gVector = 0;

	read_map_data();			/* Get map data for map 0 */
	update_vector_browser();
	update_mem_bars();			/* Draw the memory data */

	// Show the new window
	grmcw->show();
}
