/*
============================================================================
memory.c

  This file is part of the Virtual T Model 100/102/200 Emulator.

  This file is copywrited under the terms of the BSD license.

  Memory management / emulation support
============================================================================
*/

#include <string.h>
#include <stdio.h>
#include <string.h>
#include <stdlib.h>

#include "VirtualT.h"
#include "m100emu.h"
#include "memory.h"
#include "display.h"
#include "cpu.h"
#include "io.h"
#include "intelhex.h"
#include "setup.h"
#include "filewrap.h"

uchar			*gMemory[64];		/* CPU Memory space */
uchar			gBaseMemory[65536];	/* System Memory */
uchar			gSysROM[65536];		/* System ROM */
uchar			gOptROM[32768];		/* Option ROM */
uchar			gMsplanROM[32768];	/* MSPLAN ROM T200 Only */
extern char		path[255];
extern char		file[255];

int				gOptRomRW = 0;		/* Flag to make OptROM R/W */
unsigned char	rambanks[3*32768];	/* Model T200 & NEC RAM banks */
uchar			gRamBank = 0;		/* Currently enabled bank */
int				gRomBank = 0;		/* Current ROM Bank selection */
int				gRomSize = 32768;   /* Current ROM Size for R/O calculation */

uchar			gReMem = 0;			/* Flag indicating if ReMem emulation enabled */
uchar			gReMemBoot = 0;		/* ALT boot flag */
uchar			gReMemMode = 0;		/* Current ReMem emulation mode */
uchar			gReMemSector = 0;	/* Current Vector access sector */
uchar			gReMemCounter = 0;	/* Current Vector access counter */
unsigned char	*gReMemSectPtr = 0;	/* Pointer to current Sector memory */
unsigned short	gReMemMap[64];		/* Map of 1K blocks - 64 total */
unsigned int	gReMemMapLower = 0; /* Lower address of actively mapped MMU Map */
unsigned int	gReMemMapUpper = 0; /* Upper address of actively mapped MMU Map */
int				gReMemFlash1State = FLASH_STATE_RO;
int				gReMemFlash2State = FLASH_STATE_RO;
UINT64			gReMemFlash1Time = 0;
UINT64			gReMemFlash2Time = 0;
int				gReMemFlash1Busy = FALSE;
int				gReMemFlash2Busy = FALSE;
int				gReMemFlashReady = TRUE;

uchar			*gReMemRam = 0;		/* Pointer to ReMem RAM space */
uchar			*gReMemFlash1 = 0;	/* Pointer to ReMem Flash1 space */
uchar			*gReMemFlash2 = 0;	/* Pointer to ReMem Flash2 space */

int				gRampac = 0;		/* Rampac Enable flag */
unsigned char	*gRampacRam = 0;	/* Rampac RAM pointer */
int				gRampacSector = 0;	/* Rampac emulation Sector number */
int				gRampacCounter = 0;	/* Rampac emulation Counter */
unsigned char	*gRampacSectPtr = 0;/* Pointer to current Sector memory */
int				gRampacSaveSector = 0;	/* Rampac emulation Sector number */
int				gRampacSaveCounter = 0;	/* Rampac emulation Counter */
unsigned char	*gRampacSaveSectPtr = 0;/* Pointer to current Sector memory */
int				gRampacEmulation = 0;/* ReMem's Rampac emulation active? */

int				gIndex[65536];

extern RomDescription_t		gM100_Desc;
extern RomDescription_t		gM200_Desc;
extern RomDescription_t		gN8201_Desc;
extern RomDescription_t		gM10_Desc;
//JV
extern RomDescription_t		gKC85_Desc;

extern int					gShowVersion;
extern UINT64				cycles;

uchar	gFlashCFIData[] = {
	0x51, 0x52, 0x59, 0x02, 0x00, 0x40, 0x00, 0x00, 0x00, 0x00, 0x00, 0x27, 0x36, 0x00, 0x00, 0x04,
	0x00, 0x0A, 0x00, 0x05, 0x00, 0x04, 0x00, 0x15, 0x02, 0x00, 0x00, 0x00, 0x04, 0x00, 0x00, 0x40, 
	0x00, 0x01, 0x00, 0x20, 0x00, 0x00, 0x00, 0x80, 0x00, 0x1E, 0x00, 0x00, 0x01, 0x00, 0x00, 0x00,
	0x50, 0x52, 0x49, 0x31, 0x30, 0x00, 0x00, 0x00, 0x00, 0x01, 0x00, 0x00, 0x00
};

unsigned char get_memory8(unsigned short address)
{
	if (gReMem)
	{
		if (gReMemFlashReady)
			return gMemory[address >> 10][address & 0x3FF];
		/* Process Flash state machine and return the value */
		return remem_flash_sm_read(address);
	}
	else
		return gBaseMemory[address];
}

unsigned short get_memory16(unsigned short address)
{
	if (gReMem)
		return gMemory[address>>10][address&0x3FF] + (gMemory[(address+1)>>10][(address+1)&0x3FF] << 8);
	else
		return gBaseMemory[address] + (gBaseMemory[address+1] << 8);
}

void set_memory8(unsigned short address, unsigned char data)
{
	if (gReMem)
		remem_set8(address, data);
	else
		gBaseMemory[address] = data;
}

void set_memory16(unsigned short address, unsigned short data)
{
	set_memory8(address, (unsigned char) (data & 0xFF));
	set_memory8((unsigned short)(address+1), (unsigned char) (data >> 8));
}

/*
========================================================================
get_memory8_ext:	This routine reads the specified number of bytes 
					from the address given for the region identified.
========================================================================
*/
void get_memory8_ext(int region, long address, int count, unsigned char *data)
{
	int				addr;
	int				c;
	int				cp_ptr;
	unsigned char	*ptr;

	switch (region)
	{
	case REGION_RAM:
		addr = RAMSTART + address;
		for (c = 0; c < count; c++)
		{
			if (gReMem)
				data[c] = gMemory[addr>>10][addr&0x3FF];
			else
				data[c] = gBaseMemory[addr];
			addr++;
		}
		break;

	case REGION_ROM:
		addr = address;
		for (c = 0; c < count; c++)
		{
			if (gReMem)
				data[c] = gMemory[addr>>10][addr&0x3FF];
			else
				data[c] = gBaseMemory[addr];
			addr++;
		}
		break;

	case REGION_ROM2:
		addr = address;
		for (c = 0; c < count; c++)
			data[c] = gMsplanROM[addr++];
		break;

	case REGION_OPTROM:
		addr = address;
		for (c = 0; c < count; c++)
			data[c] = gOptROM[addr++];
		break;

	case REGION_RAM1:
		// Check if RAM Bank 1 is active & copy from system memory if it is
		if (gRamBank == 0)
		{
			addr = address + RAMSTART;
			for (c = 0; c < count; c++)
			{
				if (gReMem)
					data[c] = gMemory[addr>>10][addr&0x3FF];
				else
					data[c] = gBaseMemory[addr];
				addr++;
			}
		}
		else
		{
			addr = address;
			for (c = 0; c < count; c++)
				data[c] = rambanks[addr++];
		}
		break;

	case REGION_RAM2:
		// Check if RAM Bank 2 is active & copy from system memory if it is
		if (gRamBank == 1)
		{
			addr = address + RAMSTART;
			for (c = 0; c < count; c++)
			{
				if (gReMem)
					data[c] = gMemory[addr>>10][addr&0x3FF];
				else
					data[c] = gBaseMemory[addr];
				addr++;
			}
		}
		else
		{
			if (gModel == MODEL_T200)
				addr = 24576 + address;
			else
				addr = 32768 + address;
			for (c = 0; c < count; c++)
				data[c] = rambanks[addr++];
		}
		break;

	case REGION_RAM3:
		// Check if RAM Bank 2 is active & copy from system memory if it is
		if (gRamBank == 2)
		{
			addr = address + RAMSTART;
			for (c = 0; c < count; c++)
			{
				if (gReMem)
					data[c] = gMemory[addr>>10][addr&0x3FF];
				else
					data[c] = gBaseMemory[addr];
				addr++;
			}
		}
		else
		{
			if (gModel == MODEL_T200)
				addr = 24576*2 + address;
			else
				addr = 32768*2 + address;
			for (c = 0; c < count; c++)
				data[c] = rambanks[addr++];
		}
		break;

	case REGION_RAMPAC:
		// Check if RamPack memory is valid
		if (gRampacRam == NULL)
			break;

		// Load data from RamPac RAM
		addr = address;
		for (c = 0; c < count; c++)
			data[c] = gRampacRam[addr++];
		break;

		/*
		================================================================
		The following cases deal with ReMem emulation of RAM and FLASH.
		For these cases, the MMU settings must be queried to determine
		if memory should be copied from the 2Meg region or from the
		system memory where 1K blocks are copied to/from.
		================================================================
		*/
	case REGION_REMEM_RAM:
	case REGION_FLASH1:
	case REGION_FLASH2:
		// First check that the ReMem RAM has been allocated
		if ((gReMemRam == NULL) || (gReMemFlash1 == NULL) || (gReMemFlash2 == NULL))
			break;

		// Indicate zero bytes copied so far
		cp_ptr = 0;

		// Determine which location to copy memory from
		if (region == REGION_REMEM_RAM)
			ptr = gReMemRam;
		else if (region == REGION_FLASH1)
			ptr = gReMemFlash1;
		else 
			ptr = gReMemFlash2;

		// Block is not mapped -- copy from 2Meg Region (RAM or FLASH)
		addr = address;
		for (;(cp_ptr < count); cp_ptr++)
			data[cp_ptr] = ptr[addr++];
		break;
	}
}

/*
========================================================================
set_memory8_ext:	This routine writes the specified number of bytes 
					from the address given to the region identified.
========================================================================
*/
void set_memory8_ext(int region, long address, int count, unsigned char *data)
{
	int				addr;
	int				c;
	unsigned short	map;
	int				cp_ptr;
	unsigned char	*ptr;
	int				map_changed = 0;

	switch (region)
	{
	case REGION_RAM:
		addr = RAMSTART + address;
		for (c = 0; c < count; c++)
		{
			if (gReMem)
				gMemory[addr>>10][addr&0x3FF] = data[c];
			else
				gBaseMemory[addr] = data[c];
			addr++;
		}
		break;

	case REGION_ROM:
		addr = address;
		for (c = 0; c < count; c++)
		{
			if (gReMem)
				gMemory[addr>>10][addr&0x3FF] = data[c];
			else
				gBaseMemory[addr] = data[c];
			addr++;
		}
		break;

	case REGION_ROM2:
		addr = address;
		for (c = 0; c < count; c++)
			gMsplanROM[addr++] = data[c];
		break;

	case REGION_OPTROM:
		addr = address;
		for (c = 0; c < count; c++)
			gOptROM[addr++] = data[c];
		break;

	case REGION_RAM1:
		// Check if RAM Bank 1 is active & copy from system memory if it is
		if (gRamBank == 0)
		{
			addr = address + RAMSTART;
			for (c = 0; c < count; c++)
			{
				if (gReMem)
					gMemory[addr>>10][addr&0x3FF] = data[c];
				else
					gBaseMemory[addr] = data[c];
				addr++;
			}
		}
		else
		{
			addr = address;
			for (c = 0; c < count; c++)
				rambanks[addr++] = data[c];
		}
		break;

	case REGION_RAM2:
		// Check if RAM Bank 2 is active & copy from system memory if it is
		if (gRamBank == 1)
		{
			addr = address + RAMSTART;
			for (c = 0; c < count; c++)
			{
				if (gReMem)
					gMemory[addr>>10][addr&0x3FF] = data[c];
				else
					gBaseMemory[addr] = data[c];
				addr++;
			}
		}
		else
		{
			if (gModel == MODEL_T200)
				addr = 24576 + address;
			else
				addr = 32768 + address;
			for (c = 0; c < count; c++)
				rambanks[addr++] = data[c];
		}
		break;

	case REGION_RAM3:
		// Check if RAM Bank 2 is active & copy from system memory if it is
		if (gRamBank == 2)
		{
			addr = address + RAMSTART;
			for (c = 0; c < count; c++)
			{
				if (gReMem)
					gMemory[addr>>10][addr&0x3FF] = data[c];
				else
					gBaseMemory[addr] = data[c];
				addr++;
			}
		}
		else
		{
			if (gModel == MODEL_T200)
				addr = 24576*2 + address;
			else
				addr = 32768*2 + address;
			for (c = 0; c < count; c++)
				rambanks[addr++] = data[c];
		}
		break;

	case REGION_RAMPAC:
		// Check if RamPack memory is valid
		if (gRampacRam == NULL)
			break;

		// Load data from RamPac RAM
		addr = address;
		for (c = 0; c < count; c++)
			gRampacRam[addr++] = data[c];
		break;

		/*
		================================================================
		The following cases deal with ReMem emulation of RAM and FLASH.
		For these cases, the MMU settings must be queried to determine
		if memory should be copied from the 2Meg region or from the
		system memory where 1K blocks are copied to/from.
		================================================================
		*/
	case REGION_REMEM_RAM:
	case REGION_FLASH1:
	case REGION_FLASH2:
		// First check that the ReMem RAM has been allocated
		if ((gReMemRam == NULL) || (gReMemFlash1 == NULL) || (gReMemFlash2 == NULL))
			break;

		// Check ReMem emulation mode
		// If REMEM_MODE_NORMAL bit is set then ReMem is mapping 1K 
		// blocks of memory to the 64K address space
		if ((gReMemMode & REMEM_MODE_NORMAL) && (region == REGION_REMEM_RAM))
		{
			// Indicate zero bytes copied so far
			cp_ptr = 0;
			addr = address;

			// Calculate ReMemMap value to search for to test if
			map = REMEM_VCTR_FLASH1_CS | REMEM_VCTR_FLASH2_CS;
			ptr = gReMemRam;

			/* Loop for each byte and copy to RAM.  If byte is in MMU space */
			/* we must update the mapping tables */
			for (cp_ptr = 0; cp_ptr < count; cp_ptr++)
			{
				/* Determine if address is part of MMU */
				if ((addr >= REMEM_MAP_OFFSET) && (addr < (REMEM_MAP_OFFSET + REMEM_MAP_SIZE * 8)))
					/* Indiacte possible map chage */
					map_changed = 1;

				/* Copy the data to RAM */
				ptr[addr++] = data[cp_ptr];
			}

			/* Check if map changed above and update vectors if it did */
			if (map_changed)
			{
				remem_update_vectors(gReMemMode);
				remem_copy_mmu_to_system();
			}
		}
		else
		{
			// Get pointer to correct memory space
			if (region == REGION_REMEM_RAM)
				ptr = gReMemRam;
			else if (region == REGION_FLASH1)
				ptr = gReMemFlash1;
			else if (region == REGION_FLASH2)
				ptr = gReMemFlash2;

			addr = address;

			// Copy bytes up to the limit to allocation
			for (cp_ptr = 0; cp_ptr < count; cp_ptr++)
				ptr[addr++] = data[cp_ptr];
		}
		break;
	}
}

/*
========================================================================
init_mem:	This routine initialized the Memory subsystem.  It takes
			care of allocation of ReMem memory, Rampac memory, etc.
========================================================================
*/
void init_mem(void)
{
	int		c, size;
	int		next = 32;

	/* Set gReMem and gRampac based on preferences */
	gReMem = (mem_setup.mem_mode == SETUP_MEM_REMEM) || (mem_setup.mem_mode == SETUP_MEM_REMEM_RAMPAC);
	gRampac = (mem_setup.mem_mode == SETUP_MEM_RAMPAC) || (mem_setup.mem_mode == SETUP_MEM_REMEM_RAMPAC);
	gRampacEmulation = 0;
	gRampacSectPtr = NULL;

	// Initialize ROM size base on current model
	gRomSize = gModel == MODEL_T200 ? 40960 : 32768;

	for (c = 0; c < 65536; c++)
		gIndex[c] = c >> 10;

	/* Test if ReMem emulation enabled */
	if (gReMem)
	{
		/* Allocate memory only if not already allocated */
		if  (gReMemRam == 0)
		{
			/* Size of allocation for RAM and FLASH */
			size = 1024 * 2048;

			/* Allocate space for RAM and FLASH */
			gReMemRam = malloc(size);
			gReMemFlash1 = malloc(size);
			gReMemFlash2 = malloc(size);

			/* Initialize memory to zero */
			for (c = 0; c < size; c++)
			{
				gReMemRam[c] = 0;
				gReMemFlash1[c] = 0;
				gReMemFlash2[c] = 0;
			}
		}

		remem_copy_normal_to_system();

		/* Initialize Rampac I/O mode access variables */
		gReMemMode = REMEM_MODE_FLASH1_RDY | REMEM_MODE_FLASH2_RDY;
		gReMemFlashReady = TRUE;
		gReMemSector = 0;
		gReMemCounter = 0;
		gReMemSectPtr = (unsigned char *) (gReMemRam + REMEM_MAP_OFFSET);
	}
	else
	{
		/* Reset Rom Size back to original */
		if (gModel == MODEL_T200)
			gRomSize = 40960;
		else
			gRomSize = 32768;

		/* Copy Memory */
		for (c = 0; c < ROMSIZE/1024; c++)
			gMemory[c] = &gSysROM[c*1024];

		for (; c < 64; c++)
			gMemory[c] = &gBaseMemory[next++ * 1024];
	}

	/* Test if Rampac emulation enabled */
	if (gRampac)
	{
		/* Allocate memory only if not already allocated */
		if (gRampacRam == 0)
		{
			/* Size of allocation for RAM and FLASH */
			size = 1024 * 256;

			/* Allocate space for Rampac RAM */
			gRampacRam = malloc(size);

			/* Initialize memory to zero */
			for (c = 0; c < size; c++)
				gRampacRam[c] = 0;
		}

		/* Initialize Rampac mode access variables */
		gRampacSector = 0;
		gRampacCounter = 0;
		gRampacSectPtr = gRampacRam;
	}
}

void reinit_mem(void)
{
	// Initialize ROM size base on current model
	gRomSize = gModel == MODEL_T200 ? 40960 : 32768;

	/* Check if ReMem emulation on */
	if (gReMem)
	{
		gReMemMode = REMEM_MODE_FLASH1_RDY | REMEM_MODE_FLASH2_RDY;
		gReMemFlashReady = TRUE;
		gReMemSector = 0;
		gReMemCounter = 0;
		gReMemSectPtr = (unsigned char *) (gReMemRam + REMEM_MAP_OFFSET);

		remem_copy_normal_to_system();
	}
	else
	{
		memcpy(gBaseMemory,gSysROM,ROMSIZE);
	}

	gRamBank = 0;
	gRomBank = 0;

}


/*
========================================================================
free_remem_mem:	This routine frees the memory used by the ReMem 
				emulation and resets the pointers to zero.
========================================================================
*/
void free_remem_mem(void)
{
	/* Delete memory allocated for RAM and FLASH */
	free(gReMemRam);
	free(gReMemFlash1);
	free(gReMemFlash2);

	/* Set memory pointers to NULL */
	gReMemRam = 0;
	gReMemFlash1 = 0;
	gReMemFlash2 = 0;

	gReMemMapLower = 0;
	gReMemMapLower = 0;
}

/*
========================================================================
free_rampac_mem:	This routine frees the memory used by the Rampac 
					emulation and resets the pointers to zero.
========================================================================
*/
void free_rampac_mem(void)
{
	/* Delete memory allocated for RAM and FLASH */
	free(gRampacRam);

	/* Set memory pointers to NULL */
	gRampacRam = 0;
}

/*
========================================================================
free_mem:	This routine frees ReMem and/or Rampac memory allocated 
			during the emulation session.
========================================================================
*/
void free_mem(void)
{
	/* Test if ReMem RAM currently allocated */
	if (gReMemRam != 0)
		free_remem_mem();

	/* Test if Rampac RAM currently allocated */
	if (gRampacRam != 0)
		free_rampac_mem();
}

/*
========================================================================
save_remem_ram:	This routine saves the ReMem emulation RAM to the ReMem
				file.
========================================================================
*/					
void save_remem_ram(void)
{
	FILE	*fd;
	int		size;

	/* Open ReMem file */
	fd = fopen(mem_setup.remem_file, "wb+");

	/* Check if file opened successfully */
	if (fd != 0)
	{
		size = 1024 * 2048;		/* Copy 2 meg of RAM & FLASH 

		/* Write ReMem RAM first */
		fwrite(gReMemRam, 1, size, fd);

		/* Now write Flash */
		fwrite(gReMemFlash1, 1, size, fd);
		fwrite(gReMemFlash2, 1, size, fd);

		/* Close the file */
		fclose(fd);
	}
}

/*
========================================================================
save_rampac_ram:	This routine saves the Rampac emulation RAM to the 
					Rampac file.
========================================================================
*/					
void save_rampac_ram(void)
{
	FILE	*fd;
	int		size;

	/* Open Rampac file */
	fd = fopen(mem_setup.rampac_file, "wb+");

	/* Check if file opened successfully */
	if (fd != 0)
	{
		size = 1024 * 256;		/* Copy 256 K of RAM 

		/* Write ReMem RAM first */
		fwrite(gRampacRam, 1, size, fd);

		/* Close the file */
		fclose(fd);
	}
}

/*
========================================================================
save_ram:	This routine saves the contens of the RAM after
			emulation has completed (or anytime it is called).

			This routine currently does not support any external 
			hardware memory additions.  Current support is as follows:

			M100, M102:			32K RAM
			T200:				72K RAM
			PC8201, PC8300:		64K RAM
========================================================================
*/
void save_ram(void)
{
	char	file[256]; 
	FILE	*fd;

	/* Check if ReMem emulation enabled or Base Memory emulation */
	if (gReMem)
	{
		save_remem_ram();		/* Save ReMem data to file */
	}
	else
	{
		/* Base memory -- First get the emulation path */
		get_emulation_path(file, gModel);

		/* Append the RAM filename for Base Memory emulation */
		strcat(file, "RAM.bin");

		/* Open the RAM file */
		fd=fopen(file, "wb+");
		if(fd != 0)
		{
			/* Save based on which model is being emulated */
			switch (gModel)
			{
			case MODEL_M100:				/* M100 & M102 have single bank */
			case MODEL_M10:					/* M100 & M102 have single bank */
			case MODEL_KC85:				// JV
			case MODEL_M102:
				fwrite(gBaseMemory+RAMSTART, 1, RAMSIZE, fd);
				break;

			case MODEL_T200:
				/* Copy base memory to rambanks */
				set_ram_bank(gRamBank);

				/* Save RAM */
				fwrite(rambanks, 1, 3*RAMSIZE, fd);

				/* Write bank number to file */
				fwrite(&gRamBank, 1, 1, fd);

				break;

			case MODEL_PC8201:
			case MODEL_PC8300:
				/* Copy base memory to rambanks */
				set_ram_bank(gRamBank);

				/* Save RAM */
				fwrite(rambanks, 1, 3*RAMSIZE, fd);

				/* Write bank number to file */
				fwrite(&gRamBank, 1, 1, fd);
				fwrite(&gRomBank, 1, 1, fd);

				break;
			}

			/* Close the file */
			fclose(fd);
		}
	}

	/*
	===========================================
	Save Rampac RAM if enabled
	===========================================
	*/
	if (gRampac)
	{
		save_rampac_ram();		/* Save Rampac RAM to file */
	}
}

/*
========================================================================
load_remem_ram:	This routine loads the ReMem emulation RAM from the 
				ReMem file.
========================================================================
*/					
void load_remem_ram(void)
{
	FILE	*fd;
	int		size, x;
	int		empty = 1;

	/* Open ReMem file */
	fd = fopen(mem_setup.remem_file, "rb+");

	/* Check if file opened successfully */
	if (fd != 0)
	{
		size = 1024 * 2048;		/* Copy 2 meg of RAM & FLASH 

		/* Write ReMem RAM first */
		fread(gReMemRam, 1, size, fd);

		/* Now write Flash */
		fread(gReMemFlash1, 1, size, fd);
		fread(gReMemFlash2, 1, size, fd);

		/* Close the file */
		fclose(fd);
	}

	/* Test if ReMem FLASH "Normal" region is empty */
	for (x = 0; x < 10; x++)
	{
		/* Check for any non-zero bytes in normal ROM area */
		if (gReMemFlash1[x] != 0)
			empty = 0;
	}

	if (empty)
	{
		/* Rom area is empty - initialize with SYSROM */
		for (x = 0; x < ROMSIZE; x++)
			gReMemFlash1[x] = gSysROM[x];
	}

	/* Initalize MsplanROM area */
	if (gModel == MODEL_T200)
		for (x = 0; x < sizeof(gMsplanROM); x++)
			gReMemFlash1[0x10000 + x] = gMsplanROM[x];
}

void reload_sys_rom(void)
{
	int		x;
	
	for (x = 0; x < ROMSIZE; x++)
		gReMemFlash1[x] = gSysROM[x];
}

/*
========================================================================
load_rampac_ram:	This routine loads the Rampac emulation RAM from the 
					Rampac file.
========================================================================
*/					
void load_rampac_ram(void)
{
	FILE	*fd;
	int		size;

	/* Open Rampac file */
	fd = fopen(mem_setup.rampac_file, "wb+");

	/* Check if file opened successfully */
	if (fd != 0)
	{
		size = 1024 * 256;		/* Copy 256 K of RAM 

		/* Write ReMem RAM first */
		fread(gRampacRam, 1, size, fd);

		/* Close the file */
		fclose(fd);
	}
}

/*
========================================================================
load_ram:	This routine loads the contens of the RAM in preparation
			for emulation (or anytime it is called).

			This routine currently does not support any external 
			hardware memory additions.  Current support is as follows:

			M100, M102:			32K RAM
			T200:				72K RAM
			PC8201, PC8300:		64K RAM
========================================================================
*/
void load_ram(void)
{
	char			file[256]; 
	FILE			*fd;
	int				x;

	/* Check if ReMem emulation enabled or Base Memory emulation */
	if (gReMem)
	{
		load_remem_ram();		/* Call routine to load ReMem */
		remem_copy_normal_to_system();
	}
	else
	{
		for (x = 0; x < 64; x++)
			gMemory[x] = 0;

		/* First get the emulation path */
		get_emulation_path(file, gModel);

		/* Append the RAM filename */
		strcat(file, "RAM.bin");

		/* Open the RAM file */
		fd = fopen(file, "rb+");
		if(fd == 0)
			return;

		/* Save based on which model is being emulated */
		switch (gModel)
		{
		case MODEL_M100:				/* M100 & M102 have single bank */
		case MODEL_M10:					/* M100 & M102 have single bank */
		case MODEL_KC85:
		case MODEL_M102:
			fread(gBaseMemory+RAMSTART, 1, RAMSIZE, fd);
			break;

		case MODEL_T200:
			/* Read all ram into rambanks array */
			fread(rambanks, 1, 3*RAMSIZE, fd);

			/* Read bank number to file */
			fread(&gRamBank, 1, 1, fd);

			/* Test bank number for bounds */
			if (gRamBank > 2)
				gRamBank = 0;

			/* Seek to the correct location for this bank */
			memcpy(&gBaseMemory[RAMSTART], &rambanks[gRamBank*24576], RAMSIZE);

			/* Update Hardware with Bank */
			io_set_ram_bank(gRamBank);

			break;

		case MODEL_PC8201:
		case MODEL_PC8300:
			/* Read all ram into rambanks array */
			fread(rambanks, 1, 3*RAMSIZE, fd);

			/* Read bank number to file */
			fread(&gRamBank, 1, 1, fd);
			fread(&gRomBank, 1, 1, fd);

			/* Test bank number for bounds */
			if (gRamBank > 2)
				gRamBank = 0;

			/* Seek to the correct location for this bank */
			memcpy(&gBaseMemory[RAMSTART], &rambanks[gRamBank*RAMSIZE], RAMSIZE);

			/* Update Hardware with RAM Bank */
			io_set_ram_bank(gRamBank);

			/* Update Hardware with ROM Bank */

			break;
		}

		/* Close the file */
		fclose(fd);
	}

	/*
	===========================================
	Load Rampac RAM if enabled
	===========================================
	*/
	if (gRampac)
	{
		load_rampac_ram();		/* Call routine to load Rampac */
	}
}

/*
=============================================================================
load_opt_rom:  This function loads option ROMS as specified by user settings.
=============================================================================
*/
void load_sys_rom(void)
{
	FILE*			fd;
	//int					fd;
	unsigned short		address;
	char				oldString[15];
	char				newString[40];
	int					c, srchLen, strLen;

	/* Get Path to ROM based on current Model selection */
	get_rom_path(file, gModel);

	/* Open the ROM file */
	fd = fopen(file, "rb");
	//fd = open(file,O_RDONLY | O_BINARY);
	if (fd == NULL)
	{
		show_error("Could not open ROM file");
		return;
	}

	gRomSize = gModel == MODEL_T200 ? 40960 : 32768;

	/* Read data from the ROM file */
	if (fread(gSysROM, 1, ROMSIZE, fd)<ROMSIZE)
	{
		show_error("Error reading ROM file: Bad Rom file");
	}

	/* If Model = T200 then read the 2nd ROM (MSPLAN) */
	if (gModel == MODEL_T200)
		fread(gMsplanROM, 1, 32768, fd);
	
	/* Close the ROM file */
	fclose(fd);

	/* Set pointer to ROM Description */
	if (gModel == MODEL_T200)
		gStdRomDesc = &gM200_Desc;
	else if (gModel == MODEL_PC8201)
		gStdRomDesc = &gN8201_Desc;
	else if (gModel == MODEL_M10)
		gStdRomDesc = &gM10_Desc;
	else if (gModel == MODEL_KC85)
		gStdRomDesc = &gKC85_Desc;
//	else if (gModel == MODEL_PC8300)
//		gStdRomDesc = &gN8300_Desc;
	else 
		gStdRomDesc = &gM100_Desc;

	/* Test if VirtulalT version should replace (C) Micro***t text */
	if (gShowVersion)
	{
		if (gModel == MODEL_PC8201)
		{
			sprintf(newString, " VirtualT %s ", VERSION);
			newString[14] = 0;
			strcpy(oldString, "(C) Micro");
		}
		else
		{
			sprintf(newString, "VirtualT %s", VERSION);
			newString[12] = 0;
			strcpy(oldString, "(C)Micro");
		}
		strcat(oldString, "soft");
		
		/* Start by checking the address identified in the StdRomDescription */
		address = gStdRomDesc->sMSCopyright;

		/* Check if the copyright string is where we expect it to be */
		if (strncmp((char*) &gSysROM[address], oldString, strlen(oldString)) != 0)
		{
			/* Okay, the copyright string has been moved (by the user?)  Find it */
			address = 0;
			srchLen = ROMSIZE-strlen(oldString)-1;
			strLen = strlen(oldString);

			for (c = 0; c < srchLen; c++)
			{
				if (strncmp((char*) &gSysROM[c], oldString, strLen) == 0)
				{
					address = (unsigned short) c;
					break;
				}
			}
		}

		/* If the address is good, update the ROM */
		if (address != 0)
			strcpy((char*) &gSysROM[address], newString);
	}

	/* Copy ROM into system memory */
	memcpy(gBaseMemory, gSysROM, ROMSIZE);
}
/*
=============================================================================
load_opt_rom:  This function loads option ROMS as specified by user settings.
=============================================================================
*/
void load_opt_rom(void)
{
	int				len, c;
	FILE			*fd;
	unsigned short	start_addr;
	char			buf[65536];

	// Clear the option ROM memory
	memset(gOptROM,0,OPTROMSIZE);

	// Check if an option ROM is loaded
	if ((len = strlen(gsOptRomFile)) == 0)
		return;

	// Check type of option ROM
	if (((gsOptRomFile[len-1] | 0x20) == 'x') &&
		((gsOptRomFile[len-2] | 0x20) == 'e') &&
		((gsOptRomFile[len-3] | 0x20) == 'h'))
	{
		// Load Intel HEX file
		len = load_hex_file(gsOptRomFile, buf, &start_addr);

		// Check for invalid HEX file
		if ((len > 32768) || (start_addr != 0))
			return;

		// Copy data to optROM
		for (c = 0; c < len; c++)
			gOptROM[c] = buf[c];

		if (gReMem)
		{
			if (gModel == MODEL_T200)
				for (c = 0; c < len; c++)
					gReMemFlash1[0x18000 + c] = buf[c];
			else
				for (c = 0; c < len; c++)
					gReMemFlash1[0x8000 + c] = buf[c];
		}

	}
	else
	{
		// Open BIN file
		fd=fopen(gsOptRomFile,"rb");
		if(fd!=0) 
		{
			fread(gOptROM,1, OPTROMSIZE, fd);
			fclose(fd);
		}	

		if (gReMem)
		{
			if (gModel == MODEL_T200)
				for (c = 0; c < OPTROMSIZE; c++)
					gReMemFlash1[0x18000 + c] = gOptROM[c];
			else
				for (c = 0; c < OPTROMSIZE; c++)
					gReMemFlash1[0x8000 + c] = gOptROM[c];
		}
	}
}

/*
=============================================================================
set_ram_bank:	This function sets the current RAM bank for Model T200 and
				NEC PCs.  It checks for ReMem emulation and properly swaps
				out the memory space in the 2Meg Memory space.
=============================================================================
*/
void set_ram_bank(unsigned char bank)
{
	int		block;

	if (!gReMem)
	{
		/* Deal with Non-Remem Banks */
		switch (gModel)
		{
		case MODEL_T200:	/* Model T200 RAM banks */
		case MODEL_PC8201:	/* NEC Laptop banks */
		case MODEL_PC8300:
			memcpy(&rambanks[gRamBank*RAMSIZE], &gBaseMemory[RAMSTART], RAMSIZE);
			gRamBank = bank;
			memcpy(&gBaseMemory[RAMSTART], &rambanks[gRamBank*RAMSIZE], RAMSIZE);
		}
	}
	else
	{
		/* Deal with ReMem emulation mode */
		if (gModel == MODEL_T200)
		{
			/* Write all RAM bank blocks back to ReMem memory */
			if (gReMemMode & REMEM_MODE_NORMAL)
			{
				/* Set the new RAM bank */
				gRamBank = bank;
				remem_update_vectors(gReMemMode);

				/* Loop through 24 1K RAM Blocks */
				for (block = 40; block < 64; block++)
					remem_copy_mmu_to_block(block);
			}
			else
			{
				// ReMem Normal mode not enabled - use system model
				gRamBank = bank;
				remem_copy_normal_to_system();
			}
		}
	}
}

/*
=============================================================================
get_ram_bank:	This function gets the current RAM bank. 
=============================================================================
*/
unsigned char get_ram_bank(void)
{
	return gRamBank;
}

/*
=============================================================================
get_rom_bank:	This function gets the current ROM bank. 
=============================================================================
*/
unsigned char get_rom_bank(void)
{
	return gRomBank;
}

/*
=============================================================================
set_rom_bank:	This function sets the current ROM bank for all models.  It 
				checks for ReMem emulation and properly swaps out the memory 
				space to one of the 2Meg FLASH Memory spaces.
=============================================================================
*/
void set_rom_bank(unsigned char bank)
{
	int		block, blocks;

	if (!gReMem)
	{
		/* Deal with non-ReMem emulation */
		switch (gModel)
		{
		case MODEL_M100:	/* Model 100 / 102 emulation */
		case MODEL_M102:
		case MODEL_KC85:
			// Default ROM size
			gRomSize = 32768;

			// Save any writes to OptROM space
			if ((gOptRomRW) && (gRomBank == 1))
				memcpy(gOptROM, gBaseMemory, ROMSIZE);

			// Update ROM bank
			gRomBank = bank;
			if (bank & 0x01) 
			{
				memcpy(gBaseMemory,gOptROM,ROMSIZE);
				gRomSize = gOptRomRW ? 0 : 32768;				
			}	
			else 
				memcpy(gBaseMemory,gSysROM,ROMSIZE);
			break;

		case MODEL_T200:	/* Model 200 emulation */
			// Default ROM size
			gRomSize = 40960;

			// Save any writes to OptROM space
			if ((gOptRomRW) && (gRomBank == 2))
				memcpy(gOptROM, gBaseMemory, ROMSIZE);

			// Save ROM bank
			gRomBank = bank;

			switch (bank) {
			case 0:
				memcpy(gBaseMemory,gSysROM,ROMSIZE);
				break;
			case 1:
				memcpy(gBaseMemory, gMsplanROM, sizeof(gMsplanROM));
				break;
			case 2:
				memcpy(gBaseMemory,gOptROM,sizeof(gOptROM));
				gRomSize = gOptRomRW ? 0 : 40960;
				break;
			}
			break;

		case MODEL_PC8201:	/* NEC laptops */
		case MODEL_PC8300:
			// Default ROM size
			gRomSize = 32768;

			// Save any writes to OptROM space
			if ((gOptRomRW) && (gRomBank == 2))
				memcpy(gOptROM, gBaseMemory, ROMSIZE);

			// Save ROM bank
			gRomBank = bank;	/* Update global ROM bank var */

			switch (bank)
			{
			case 0:			/* System ROM bank */
				memcpy(gBaseMemory,gSysROM,ROMSIZE);
				break;

			case 1:			/* Option ROM bank */
				memcpy(gBaseMemory,gOptROM,ROMSIZE);
				gRomSize = gOptRomRW ? 0 : 32768;
				break;

			case 2:			/* RAM banks */
			case 3:
				memcpy(gBaseMemory,&rambanks[(bank-1)*RAMSIZE],RAMSIZE);
				break;
			}
			break;
		}
	}
	else
	{
		gRomBank = bank;

		/* Write all ROM bank blocks back to ReMem memory */
		if (gReMemMode & REMEM_MODE_NORMAL)
		{
			/* Calculate number of ROM blocks base on model */
			blocks = gModel == MODEL_T200 ? 40 : 32;

			/* Now set the new RAM bank */
			if (gModel == MODEL_T200)
				gRomBank = bank;
			else
				gRomBank = bank & 0x01;
			remem_update_vectors(gReMemMode);

			/* Loop through all 1K ROM Blocks & copy to system memory */
			for (block = 0; block < blocks; block++)
				remem_copy_mmu_to_block(block);
		}
		else
		{
			remem_copy_normal_to_system();
		}
	}
}

/*
=============================================================================
remem_set8:		This function services memory writes while in ReMem emulation
				mode.  It validates if the 1K block	of memory being accessed 
				is set to Read-Only and traps writes to the MMU section of 
				memory to copy ReMem memory to/from system memory when writes
				to the active map are made.
=============================================================================
*/
void remem_set8(unsigned int address, unsigned char data)
{
	int		block;			/* Which 1K block is being accessed */
	int		mmuBlock;		/* Block with MMU that is being modified */
	int		bank;			/* Bank with map that is being modified */
	int		romBank;		/* The currently mapped romBank within the Map */
	int		ramBank;		/* The currently mapped ramBank within the Map */
	int		sectOffset;
	int		sectSize, c;
	int		sector;
	uchar	*ptr;

 	/* Calculate which block is being accessed */
	block = gIndex[address];	

	/* Test if ReMem memory block is read-only */
	if ((gReMemMode & REMEM_MODE_NORMAL) && (gReMemMap[block] & REMEM_VCTR_READ_ONLY))
		return;

	/* Test if ReMem is in Normal mode and writing to ROM */
	if (!(gReMemMode & REMEM_MODE_NORMAL) && (address < (unsigned int) ROMSIZE))
		return;

	/* Test if accessing MMU vector for current map */
	if (gReMemMap[block] & REMEM_VCTR_MMU_ACTIVE)
	{
		/* Need to validate that address is accssing active portion of MMU page */
		if (((address & 0x3FF) >= gReMemMapLower) && ((address & 0x3FF) < gReMemMapUpper))
		{
			/* First validate that the data is actually changing */
			if (gMemory[block][address&0x3FF] == data)
				return;

			/* Okay, data is really changing first update system memory */
			gMemory[block][address&0x3FF] = data;										 

			/* System is making a change to the active MAP!!  Now we need to determine if */
			/* The portion of the map being modified is selected by the current ROM/RAM */
			/* bank settings */
			bank = ((address & 0x3FF) - gReMemMapLower) / REMEM_SECTOR_SIZE;
			romBank = gModel == MODEL_T200 ? gRomBank : gRomBank << 1;
			ramBank = gModel == MODEL_T200 ? gRamBank : 1;

			/* Determine if the change was made in a mapped bank ROM or RAM */
			if ((bank != romBank) && (bank != ramBank))
				return;

			/* Determine which map entry was modified */
			mmuBlock = ((address & 0x3FF) - gReMemMapLower - (bank * REMEM_SECTOR_SIZE)) >> 1;
			if ((gModel != MODEL_T200) && (bank == 1))
				mmuBlock += 32;

			/* Update the gReMemMap vector table for this block */
			if (address & 0x01)
				gReMemMap[mmuBlock] = (gReMemMap[mmuBlock] & 0x00FF) | (data << 8);
			else
				gReMemMap[mmuBlock] = (gReMemMap[mmuBlock] & 0xFF00) | data;

			/* Now copy new block to system memory */
			remem_copy_mmu_to_block(mmuBlock);

			return;
		}
	}

	/* Do FLASH state machine management */
	if (gReMemMap[block] & (REMEM_VCTR_FLASH1_CS | REMEM_VCTR_FLASH2_CS) != 0x1800)
	{
		int		*pFlashState;
		UINT64	*pFlashTime;
		int		*pFlashBusy;
		uchar	mask;

		/* Writing to FLASH.  Do FLASH State machine processing */
		if (!(gReMemMap[block] & REMEM_VCTR_FLASH1_CS))
		{
			pFlashState = &gReMemFlash1State;
			pFlashTime = &gReMemFlash1Time;
			pFlashBusy = &gReMemFlash1Busy;
			mask = REMEM_MODE_FLASH1_RDY;
		}
		else
		{
			pFlashState = &gReMemFlash2State;
			pFlashTime = &gReMemFlash2Time;
			pFlashBusy = &gReMemFlash2Busy;
			mask = REMEM_MODE_FLASH2_RDY;
		}

		/* Look for Reset command */
		if ((data == FLASH_CMD_RESET) && (*pFlashState != FLASH_STATE_PROG))
		{
			*pFlashState = FLASH_STATE_RO;
			*pFlashTime = 0;
			*pFlashBusy = FALSE;
			gReMemFlashReady = !gReMemFlash1Busy | !gReMemFlash2Busy;
			return;
		}

		switch (*pFlashState)
		{
		case FLASH_STATE_RO:
			/* Proocess write as command */
			if ((data == FLASH_CMD_CFI_QUERY) && (address == 0xAA))
			{
				*pFlashState = FLASH_STATE_CFI_QUERY;
				*pFlashBusy = TRUE;
				gReMemFlashReady = FALSE;
			}
			return;

		case FLASH_STATE_PROG:
			/* Change state back to Read Only and break to perform the write */
			*pFlashState = FLASH_STATE_RO;
			*pFlashBusy = TRUE;
			*pFlashTime = cycles + FLASH_CYCLES_PROG;
			gReMemMode &= ~mask;
			gReMemFlashReady = FALSE;
			break;

		case FLASH_STATE_CMD2:
			/* Test if data is a Sect Erase command */
			if (data != FLASH_CMD_SECT_ERASE)
			{
				*pFlashState = FLASH_STATE_RO;
				return;
			}

			/* Process Sector erase */
			sector = address >> 15;

			/* Calculate sector offset and size */
			if (sector == 0)
			{
				/* Test for sector zero */
				if ((address & 0xFE000) == 0)
				{
					/* Sector zero is 16K */
					sectOffset = 0;
					sectSize = 16384;
				}
				else if ((address & 0xFC00) != 0)
				{
					/* Sector 3 is a 32K sector */
					sectOffset = 0x4000;
					sectSize = 32768;
				}
				else
				{
					sectOffset = address & 0xFF000;
					sectSize = 8192;
				}
			}
			else
			{
				/* Setup for 64K sector */
				sectOffset = address & 0xF8000;
				sectSize = 0x10000;
			}

			/* Point to the correct memory space */
			if (pFlashState == &gReMemFlash1State)
				ptr = gReMemFlash1 + sectOffset;
			else
				ptr = gReMemFlash2 + sectOffset;		

			/* Erase the sector */
			for (c = 0; c < sectSize; c++)
				*ptr++ = 0xFF;

			/* Update state and set busy flag */
			*pFlashState = FLASH_STATE_RO;
			*pFlashBusy = TRUE;
			*pFlashTime = cycles + FLASH_CYCLES_SECT_ERASE;
			gReMemMode &= ~mask;
			gReMemFlashReady = FALSE;

			return;
		}
	}	

	/* Update memory with data */
	gMemory[block][address & 0x3FF] = data;
}


/*
=============================================================================
remem_in:		This function services in operations to the ReMem emulation.
				ReMem uses port I/O for configuration of memory maps, etc.
=============================================================================
*/
int remem_in(unsigned char port, unsigned char *data)
{
	int				ret = 0;	/* Indicate if port serviced or not */

	/* Test if ReMem emulation is enabled */
	if (!(gReMem | gRampac))
		return 0;

	// Test if pointer for mode was allocated
	if ((gReMem && !gReMemRam) || (gRampac && !gRampacRam))
		return 0;

	/* Process port number being accessed */
	switch (port)
	{

	case REMEM_MODE_PORT:		/* ReMem mode port */
		if (!gReMemFlashReady)
			remem_flash_proc_timer();

		*data = gReMemMode;			/* Return the current ReMem mode */
		ret = 1;					/* Indicate port processed */
		break;

	case REMEM_REVID_PORT:		/* ReMem Firmware Rev being emulated */
		*data = (unsigned char) REMEM_REV;			/* Provide emulation level */
		ret = 1;					/* Indicate port processed */
		break;

	case REMEM_SECTOR_PORT:		/* ReMem I/O Sector number */
		*data = gReMemSector;		/* Provide current sector number */
		ret = 1;					/* Indicate port processed */
		break;

	case REMEM_DATA_PORT:		/* ReMem Data Port */
		if (gReMemSectPtr != NULL)
		{
			*data = *gReMemSectPtr++;		/* Get data at current Sector Pointer */
			/* Update the ReMem Counter and Sector */
			gReMemCounter++;				/* Increment Sector Counter */
			if (gReMemCounter >= REMEM_SECTOR_SIZE)
			{
				gReMemSector++;				/* Increment to next sector */
				if (gReMemSector >= REMEM_TOTAL_SECTORS)
					gReMemSector = 0;
				gReMemCounter = 0;			/* Clear sector counter */
				gReMemSectPtr = (unsigned char *) (gReMemRam + REMEM_MAP_OFFSET + 
						gReMemSector * REMEM_SECTOR_SIZE);
			}
		}
		else
			*data = 0xCC;			/* Code to indicate unallocated memory */
		ret = 1;					/* Indicate port processed */
		break;

	case RAMPAC_SECTOR_PORT:	/* RamPac I/O Sector number */
		*data = gRampacSector;		/* Provide current Rampac Sector */
		ret = 1;					/* Indicate port processed */
		break;

	case RAMPAC_DATA_PORT:		/* Rampac I/O Data port */
		if (gRampacSectPtr != NULL)
		{
			*data = *gRampacSectPtr++;/* Proivde data at current sector/counter */

			/* Update Counter */
			if (++gRampacCounter >= RAMPAC_SECTOR_SIZE)
			{
				/* Clear counter and move to next sector */
				gRampacCounter = 0;
				if (++gRampacSector >= RAMPAC_SECTOR_COUNT)
				{
					/* Past end of RAM, reset to start */
					gRampacSector = 0;
					if (gRampacEmulation)
						gRampacSectPtr = gReMemRam + REMEM_RAMPAC_OFFSET;
					else if (gRampac)
						gRampacSectPtr = gRampacRam;
					else
						gRampacSectPtr = NULL;
				}
			}
		}
		else
			*data = 0xCC;			/* Code to indicate unallocated memory */
		ret = 1;					/* Indicate port processed */
		break;

	}

	return ret;
}

/*
=============================================================================
remem_out:		This function services out operations to the ReMem emulation.
				ReMem uses port I/O for configuration of memory maps, etc.
=============================================================================
*/
int remem_out(unsigned char port, unsigned char data)
{
	int				ret = 0;	/* Indicate if port serviced or not */
	int				romSector, ramSector;
	int				bank, block, mmuBlock;
	int				map;
	int				*pFlashState;
	uchar			*ptr;
	int				c;

	/* Test if ReMem emulation is enabled */
	if (!(gReMem | gRampac))
		return 0;

	// Test if pointer for mode was allocated
	if ((gReMem && !gReMemRam) || (gRampac && !gRampacRam))
		return 0;

	/* Process port number being accessed */
	switch (port)
	{

	case REMEM_MODE_PORT:		/* ReMem mode port */
		ret = 1;			/* Indicate port processed */

		/* Test if old mode = new mode and do nothing if equal */
		if (data == gReMemMode & 0x3F)
			break;

		/*
		======================================================================
		First check to see if we are enabling MMU mode
		======================================================================
		*/												 
		if (data & REMEM_MODE_NORMAL)
		{

			/* Calculate ReMem Map vector pointer */
			remem_update_vectors(data);
		}

		/*
		======================================================================
		Now copy ReMem memory to system memory
		======================================================================
		*/
		/* Test if new mode is "Normal" mode */
		if (!(data & REMEM_MODE_NORMAL) && (gReMemMode & REMEM_MODE_NORMAL))
		{
			/* Copy "Normal" ReMem memory space to sysem memory */
			remem_copy_normal_to_system();
		}
		/* Test if changing to MMU mode or MAP changed */
		else if (((data & REMEM_MODE_NORMAL) && !(gReMemMode & REMEM_MODE_NORMAL)) ||
			((data & REMEM_MODE_MAP_BITS) != (gReMemMode & REMEM_MODE_MAP_BITS) &&
			(data & REMEM_MODE_NORMAL)))
		{
			/* Copy MMU data to system RAM */
			remem_copy_mmu_to_system();
		}

		/*
		======================================================================
		Check if we are enabling RamPac emulation
		======================================================================
		*/
		if ((data & REMEM_MODE_RAMPAC) && ((gReMemMode & REMEM_MODE_RAMPAC) == 0))
		{
			if (!gRampac || mem_setup.remem_override)
			{
				gRampacEmulation = TRUE;
				/* If Rampac is present, save global variables to restore later */
				if (gRampac)
				{
					gRampacSaveSector = gRampacSector;
					gRampacSaveCounter = gRampacCounter;
					gRampacSaveSectPtr = gRampacSectPtr;
				}
	
				gRampacSector = 0;			/* Initialize sector number */
				gRampacCounter = 0;			/* Initialize counter number */
				gRampacSectPtr = gReMemRam + REMEM_RAMPAC_OFFSET;
			}
		}
		/* Check if we are disabling Rampac emulation */
		else if (((data & REMEM_MODE_RAMPAC) == 0) && (gReMemMode & REMEM_MODE_RAMPAC))
		{
			gRampacEmulation = FALSE;
			if (!gRampac || mem_setup.remem_override)
			{
				/* If Rampac is present, restore registers from save variables */
				if (gRampac)
				{
					gRampacSector = gRampacSaveSector;
					gRampacCounter = gRampacSaveCounter;
					gRampacSectPtr = gRampacSaveSectPtr;
				}
				else
				{
					/* Clear all rampac variables */
					gRampacSector = 0;			/* Initialize sector number */
					gRampacCounter = 0;			/* Initialize counter number */
					gRampacSectPtr = 0;
				}
			}
		}

		/* Update ReMem mode */
		gReMemMode = (gReMemMode & 0xC0) | (data & 0x3F);
		break;


		/* ReMem port for setting the vector access sector */
	case REMEM_REVID_PORT:
		ret = 1;
		break;

	case REMEM_SECTOR_PORT:
		/* Test if MMU I/O Enable mode is set */
		if (gReMemMode & REMEM_MODE_IOENABLE)
		{
			/* Store new value for ReMemSector & Clear counter */
			gReMemSector = data;			/* Update Sector number */
			gReMemCounter = 0;				/* Clear the ReMem counter */

			/* Calculate pointer to actual RAM storage for this sector */
			gReMemSectPtr = (unsigned char *) (gReMemRam + REMEM_MAP_OFFSET + 
				gReMemSector * REMEM_SECTOR_SIZE);
		}
		ret = 1;
		break;

		/* ReMem port for writing vector data */
	case REMEM_DATA_PORT:
		/* First check if RamPac Mode enabled */
		ret = 1;
		if (!(gReMemMode & REMEM_MODE_IOENABLE))
			break;


		/* Check if MMU mode is enabled.  If it is, we may need to dynamically */
		/* load and unload blocks from system memory to ReMem Memory as a */
		/* Result of the I/O update */
		if (gReMemMode & REMEM_MODE_NORMAL)
		{
			/* MMU mode is enabled - Test if update changes mapped block */

			/* Calculate current MMU ROM bank */
			bank = gModel == MODEL_T200 ? gRomBank : gRomBank << 1;

			/* Convert current MMU map / ROM bank to sector format */
			romSector = ((gReMemMode & REMEM_MODE_MAP_BITS) >> REMEM_MODE_MAP_SHIFT) *
				REMEM_SECTORS_PER_MAP + bank;

			/* Calculate current MMU RAM bank */
			bank = gModel == MODEL_T200 ? gRamBank : 1;

			/* Convert current MMU map / ROM bank to sector format */
			ramSector = ((gReMemMode & REMEM_MODE_MAP_BITS) >> REMEM_MODE_MAP_SHIFT) *
				REMEM_SECTORS_PER_MAP + bank;

			/* Test if Current ReMem Sector same as current ReMem Map/Bank selection */
			/* If it is, this means we are writing to an actively mapped bank and */
			/* Need to swap the memory to/from system RAM */
			if ((gReMemSector == romSector) || (gReMemSector == ramSector))
			{
				/* Swap memory in/out only if data actually changes */
				if (*gReMemSectPtr != data)
				{
					/* Calculate which ReMem Vector block is being changed */
					block = (gReMemCounter >> 1);
					if ((gModel != MODEL_T200) && (gReMemSector == ramSector))
						block += 32;
					if (block >= 64)
					{
						printf("Panic!  Error calculating ReMem block update, block = %d\n", block);
						break;
					}

					/* Update ReMem Vector table */
					if (gReMemCounter & 0x01)
						gReMemMap[block] = (gReMemMap[block] & 0x00FF) | (data << 8);
					else
						gReMemMap[block] = (gReMemMap[block] & 0xFF00) | data;

					/* Check if new assignment maps the MMU bank to system memory */
					map = (gReMemMode & REMEM_MODE_MAP_BITS) >> REMEM_MODE_MAP_SHIFT;
					mmuBlock = (unsigned short) ((REMEM_MAP_OFFSET + map * REMEM_MAP_SIZE) >> 10);
					
					if ((gReMemMap[block] & REMEM_VCTR_ADDRESS) == mmuBlock)
					{
						gReMemMap[block] |= REMEM_VCTR_MMU_ACTIVE;
						gReMemMapLower = (map & 0x02) * REMEM_MAP_SIZE;
						gReMemMapUpper = ((map & 0x02) + 1) * REMEM_MAP_SIZE;
					}

					/* Copy new MMU data to system based on new vector */
					remem_copy_mmu_to_block(block);
				}
			}
		}

		/* Update ReMem MAP RAM with new value */
		*gReMemSectPtr++ = data;

		/* Update the ReMem Counter and Sector */
		gReMemCounter++;				/* Increment Sector Counter */
		if (gReMemCounter >= REMEM_SECTOR_SIZE)
		{
			/* Get the map number */
			map = (gReMemMode & REMEM_MODE_MAP_BITS) >> REMEM_MODE_MAP_SHIFT;

			gReMemSector++;				/* Increment to next sector */
			if (gReMemSector >= REMEM_TOTAL_SECTORS)
				gReMemSector = 0;
			gReMemCounter = 0;			/* Clear sector counter */
			gReMemSectPtr = (unsigned char *) (gReMemRam + REMEM_MAP_OFFSET + 
					gReMemSector * REMEM_SECTOR_SIZE);
		}
		break;

	case 0x85:
	case RAMPAC_SECTOR_PORT:
		/* Store new value for RampacSector & Clear counter */
		gRampacSector = data;		/* Update Sector for Rampac */
		gRampacCounter = 0;			/* Clear Rampac counter */

		/* Initialize pointer to memory based on emulation mode */
		if (gRampacEmulation)
			gRampacSectPtr = gReMemRam + REMEM_RAMPAC_OFFSET + 
				gRampacSector * RAMPAC_SECTOR_SIZE;
		else if (gRampac)
			gRampacSectPtr = gRampacRam + gRampacSector * RAMPAC_SECTOR_SIZE;
		else
			gRampacSectPtr = NULL;

		ret = 1;						/* Indicate Port I/O processed */
		break;

		/* RamPac Write Byte port */
	case RAMPAC_DATA_PORT:
		if (gRampacSectPtr != NULL)
		{
			/* Save data in Ram */
			*gRampacSectPtr++ = data;

			/* Update Counter */
			if (++gRampacCounter >= RAMPAC_SECTOR_SIZE)
			{
				/* Clear counter and move to next sector */
				gRampacCounter = 0;
				if (++gRampacSector >= RAMPAC_SECTOR_COUNT)
				{
					/* Past end of RAM, reset to start */
					gRampacSector = 0;
			
					if (gRampacEmulation)
						gRampacSectPtr = gReMemRam + REMEM_RAMPAC_OFFSET;
					else if (gRampac)
						gRampacSectPtr = gRampacRam;
					else
						gRampacSectPtr = NULL;
				}
			}
		}

		ret = 1;
		break;

	case REMEM_FLASH1_AAA_PORT:
	case REMEM_FLASH2_AAA_PORT:
		if (port == REMEM_FLASH1_AAA_PORT)
			pFlashState = &gReMemFlash1State;
		else
			pFlashState = &gReMemFlash2State;

		switch (*pFlashState)
		{
		case FLASH_STATE_RO:
			if (data == 0xAA)
				*pFlashState = FLASH_STATE_UNLK1;

			// Check for other states, such as Erase Pause, etc.

			break;

		case FLASH_STATE_CMD:
			// Get the command
			if (data == FLASH_CMD_PROG)
				*pFlashState = FLASH_STATE_PROG;
			else if (data == FLASH_CMD_UNLK_BYPASS)
				*pFlashState = FLASH_STATE_UNLK_BYPASS;
			else if (data == FLASH_CMD_UNLK2)
				*pFlashState = FLASH_STATE_UNLK2;
			else if (data == FLASH_CMD_AUTOSELECT)
				*pFlashState = FLASH_STATE_AUTOSELECT;
			else
				*pFlashState = FLASH_STATE_RO;
			break;

		case FLASH_STATE_UNLK2:
			// Get 1st byte of 2nd Unlock
			if (data == 0xAA)
				*pFlashState = FLASH_STATE_UNLK3;
			else
				*pFlashState = FLASH_STATE_RO;
			break;

		case FLASH_STATE_CMD2:
			// Test for Erase Chip command
			if (data == FLASH_CMD_CHIP_ERASE)
			{
				*pFlashState = FLASH_STATE_RO;
				if (port == REMEM_FLASH1_AAA_PORT)
				{
					gReMemFlash1Time = cycles + FLASH_CYCLES_CHIP_ERASE;
					gReMemMode &= ~REMEM_MODE_FLASH1_RDY;
					gReMemFlashReady = FALSE;
					gReMemFlash1Busy = TRUE;
					ptr = gReMemFlash1;
				}
				else
				{
					gReMemFlash2Time = cycles + FLASH_CYCLES_CHIP_ERASE;
					gReMemMode &= ~REMEM_MODE_FLASH2_RDY;
					gReMemFlashReady = FALSE;
					gReMemFlash2Busy = TRUE;
					ptr = gReMemFlash2;
				}
				/* Erase the Flash */
				for (c = 0; c < 2048 * 1024; c++)
					*ptr++ = 0xFF;
			}
			break;

		default:
			*pFlashState = FLASH_STATE_RO;
			break;
		}
		break;

	case REMEM_FLASH1_555_PORT:
	case REMEM_FLASH2_555_PORT:
		if (port == REMEM_FLASH1_555_PORT)
			pFlashState = &gReMemFlash1State;
		else
			pFlashState = &gReMemFlash2State;

		switch (*pFlashState)
		{
		case FLASH_STATE_UNLK1:
			// Check for 2nd byte of unlock sequence
			if (data == 0x55)
				*pFlashState = FLASH_STATE_CMD;
			else
				*pFlashState = FLASH_STATE_RO;
			break;

		case FLASH_STATE_UNLK3:
			// Check for double unlock 2nd byte
			if (data == 0x55)
				*pFlashState = FLASH_STATE_CMD2;
			else
				*pFlashState = FLASH_STATE_RO;
			break;

		default:
			*pFlashState = FLASH_STATE_RO;
			break;
		}

		break;

		/* Other Ports not supported */
	default:
		ret = 0;
		break;
	}

	return ret;
}


/*
=============================================================================
remem_update_vectors:	This function updates the gReMemMap with memory 
						vectors for the targetMode mode.
=============================================================================
*/
void remem_update_vectors(unsigned char targetMode)
{
	int				map;		/* ReMem map selection */
	int				c;			/* Map vector counter */
	int				x;
	int				block;		/* Block being updated */
	int				bank;		/* ReMem vector bank being used based on optRom, etc. */
	unsigned short	mmuBlock;	/* Start address of active MMU Map region */
	unsigned short	*pMapPtr;	/* Pointer to ReMem map vectors */
	int				romBlocks;	/* Number of ROM blocks that need to be updated */
	int				ramBlocks;	/* Number of RAM blocks that need to be updated */

	/* Calculate number of ROM blocks vs. RAM blocks */
	romBlocks = (gModel == MODEL_T200 ? 40 : 32);
	ramBlocks = 64 - romBlocks;
	block = 0;
	map = (targetMode & REMEM_MODE_MAP_BITS) >> REMEM_MODE_MAP_SHIFT;

	/* Update block vectors for both ROM and RAM banks */
	for (x = 0; x < 2; x++)
	{
		/* Calculate location of vectors in ROM map based on Bank selection */
		if (x == 0)
			bank = gModel == MODEL_T200 ? gRomBank : gRomBank << 1;
		else
			bank = gModel == MODEL_T200 ? gRamBank : 1;
		mmuBlock = (unsigned short) ((REMEM_MAP_OFFSET + map * REMEM_MAP_SIZE + 
			bank * REMEM_SECTOR_SIZE) >> 10);
		pMapPtr = (unsigned short *) (gReMemRam + REMEM_MAP_OFFSET + map * REMEM_MAP_SIZE + 
			bank * REMEM_SECTOR_SIZE + (x ? REMEM_RAM_MAP_OFFSET : 0));

		/* Update ReMem ROM/RAM blocks */
		for (c = 0; c < (!x ? romBlocks : ramBlocks); c++)
		{
			gReMemMap[block] = pMapPtr[c];		/* Copy map vector from ReMem RAM */

			/* Test for Global R/O bit in ReMemMode for FLASH1 */
/*			if ((gReMemMap[block] & REMEM_VCTR_FLASH1_CS) == 0)
			{
				if (gReMemMode & REMEM_MODE_FLASH1_RO)
					gReMemMap[block] |= REMEM_VCTR_READ_ONLY;
			}
*/
			/* Test for Global R/O bit in ReMemMode for FLASH2 */
/*			if ((gReMemMap[block] & REMEM_VCTR_FLASH2_CS) == 0)
			{
				if (gReMemMode & REMEM_MODE_FLASH2_RO)
					gReMemMap[block] |= REMEM_VCTR_READ_ONLY;
			}
*/
			/* Test if Block being mapped is active MMU Map space */
			if (x && ((gReMemMap[block] & REMEM_VCTR_ADDRESS) == mmuBlock))
			{
				gReMemMap[block] |= REMEM_VCTR_MMU_ACTIVE;
				gReMemMapLower = (map & 0x01) * REMEM_MAP_SIZE;
				gReMemMapUpper = ((map & 0x01) + 1) * REMEM_MAP_SIZE;
			}

			/* Increment to next block */
			block++;
		}
	}
}

/*
=============================================================================
remem_copy_normal_to_system:	This function copies memory from "Normal" 
								mode space to system memory space.
=============================================================================
*/
void remem_copy_normal_to_system(void)
{
	int		blocks;				/* Number of blocks to copy */
	int		c;					/* Byte counter */
	int		dest;				/* Destination location */
	uchar	*pSrc;				/* Source address */

	/* First copy ROM to system memory */
	blocks = gModel == MODEL_T200 ? 40 : 32;
	if (gModel != MODEL_T200)
		pSrc = (gReMemBoot ? gReMemFlash2 : gReMemFlash1) + 0x8000 * gRomBank;
	else
		pSrc = (gReMemBoot ? gReMemFlash2 : gReMemFlash1) + (gRomBank == 0 ? 0 : 0x10000 + 0x8000 * (gRomBank - 1));
	dest = 0;

	/* Copy the ROM bytes */
	for (c = 0; c < blocks; c++)
		gMemory[dest++] = &pSrc[c*1024];

	/* Now prepare to copy the RAM bytes */
	blocks = gModel == MODEL_T200 ? 24 : 32;
	if (gModel != MODEL_T200)
		pSrc = gReMemRam;
	else
		pSrc = gReMemRam + 0x8000 * gRamBank;

	/* Copy the RAM bytes */
	for (c = 0; c < blocks; c++)
		gMemory[dest++] = &pSrc[c*1024];
}

/*
=============================================================================
remem_copy_mmu_to_system:	This function copies memory from MMU mode space
							to system memory space.
=============================================================================
*/
void remem_copy_mmu_to_system(void)
{
	int			block;			/* Block being copied */

	/* Loop through all blocks */
	for (block = 0; block < 64; block++)
	{
		remem_copy_mmu_to_block(block);
	}
}

/*
=============================================================================
remem_copy_mmu_to_block:	This function copies a 1K block from MMU mode 
							space to system memory space.
=============================================================================
*/
void remem_copy_mmu_to_block(int block)
{
	uchar		*pSrc;			/* Source pointer */

	/* Calculate source pointer based on block vector */
	if ((gReMemMap[block] & REMEM_VCTR_RAM_CS) == 0)
		pSrc = gReMemRam;
	else if ((gReMemMap[block] & REMEM_VCTR_FLASH1_CS) == 0)
		pSrc = gReMemFlash1;
	else if ((gReMemMap[block] & REMEM_VCTR_FLASH2_CS) == 0)
		pSrc = gReMemFlash2;
	else
		return;

	/* Calculate offset of 1K block */
	pSrc += (gReMemMap[block] & REMEM_VCTR_ADDRESS) << 10;

	/* Copy bytes */
	gMemory[block] = pSrc;
}

/*
=============================================================================
remem_flash_sm_read:	This routine performs a read operation when there is
						a pending FLASH command still being processed.  It 
						updates the state machine and returns the correct 
						value from the FLASH.
=============================================================================
*/
unsigned char remem_flash_sm_read(unsigned short address)
{
	int				block;
	int				*pFlashState;
	UINT64			*pFlashTime;
	int				*pFlashBusy;
	unsigned char	mask;
	int				index;

	/* Calculate block number */
	block = address >> 10;

	/* Test if this is the flash that is not ready */
	if ((gReMemMap[block] & REMEM_VCTR_FLASH1_CS) == 0)
	{
		if (!gReMemFlash1Busy && (gReMemFlash1State == FLASH_STATE_RO))
			return gMemory[block][address & 0x3FF];

		/* Set variables for state machine processing */
		pFlashState = &gReMemFlash1State;
		pFlashTime = &gReMemFlash1Time;
		pFlashBusy = &gReMemFlash1Busy;
		mask = REMEM_MODE_FLASH1_RDY;
	}
	else
	{
		if (!gReMemFlash2Busy && (gReMemFlash2State == FLASH_STATE_RO))
			return gMemory[block][address & 0x3FF];

		/* Set variables for state machine processing */
		pFlashState = &gReMemFlash2State;
		pFlashTime = &gReMemFlash2Time;
		pFlashBusy = &gReMemFlash2Busy;
		mask = REMEM_MODE_FLASH2_RDY;
	}

	/* The flash is busy during a read - process as State Machine */
	if (*pFlashBusy && (*pFlashState == FLASH_STATE_RO))
	{
		/* Waiting for timeout of erase / program */
		if (*pFlashTime > 0)
		{
			if (cycles >= *pFlashTime)
				*pFlashTime = 0;
		}

		/* Test if timeout expired */
		if (*pFlashTime == 0)
		{
			*pFlashBusy = FALSE;
			gReMemFlashReady = !gReMemFlash1Busy | !gReMemFlash2Busy;
			gReMemMode |= mask;
			return gMemory[block][address & 0x3FF];
		}
	}
	else if (*pFlashState == FLASH_STATE_AUTOSELECT)
	{
		if ((address & 0xFF) == 0)
			return FLASH_MANUF_ID;
		else if ((address & 0xFF) == 1)
			return FLASH_PRODUCT_ID;
		else
			return 0;
	}
	else if (*pFlashState == FLASH_STATE_CFI_QUERY)
	{
		/* Test Query address */
		if ((address < 32) || (address > 0x98))
			return 0xFF;

		/* Calculate index for CFI address */
		index = (address - 0x20) >> 1;
		return gFlashCFIData[index];
	}

	/* Test for failed command sequence */
	if (*pFlashState != FLASH_STATE_RO)
	{
		/* Reset back to read state */
		*pFlashState = FLASH_STATE_RO;
		gReMemMode |= mask;
		*pFlashBusy = FALSE;
		*pFlashTime = 0;
		gReMemFlashReady = !gReMemFlash1Busy | !gReMemFlash2Busy;
		return gMemory[block][address & 0x3FF];
	}

	return 0xFF;
}

/*
=============================================================================
remem_flash_proc_timer:	This routine processes FLASH timers.
=============================================================================
*/
void remem_flash_proc_timer(void)
{
	/* Check if FLASH1 is busy */
	if (gReMemFlash1Time > 0)
	{
		if (cycles >= gReMemFlash1Time)
		{
			/* Clear busy bit for Flash1 */
			gReMemMode |= REMEM_MODE_FLASH1_RDY;
			gReMemFlash1Time = 0;
			gReMemFlash1Busy = FALSE;
		}
	}

	/* Check if FLASH2 is busy */
	if (gReMemFlash2Time > 0)
	{
		if (cycles >= gReMemFlash2Time)
		{
			/* Clear busy bit for Flash1 */
			gReMemMode |= REMEM_MODE_FLASH2_RDY;
			gReMemFlash2Time = 0;
			gReMemFlash2Busy = FALSE;
		}
	}

	/* Check if either flash is busy */
	if (gReMemFlash1Busy || gReMemFlash2Busy)
		gReMemFlashReady = FALSE;
	else
		gReMemFlashReady = TRUE;
}


