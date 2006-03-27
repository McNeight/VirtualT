/*
============================================================================
memory.c

  This file is part of the Virtual T Model 100/102/200 Emulator.

  This file is copywrited under the terms of the BSD license.

  Memory management / emulation support
============================================================================
*/

#include <stdio.h>
#include <malloc.h>

#include "VirtualT.h"
#include "m100emu.h"
#include "memory.h"
#include "cpu.h"
#include "io.h"
#include "intelhex.h"
#include "setup.h"

extern uchar	memory[65536];		/* System Memory */
extern uchar	sysROM[65536];		/* System ROM */
extern uchar	optROM[32768];		/* Option ROM */
extern uchar	msplanROM[32768];	/* MSPLAN ROM T200 Only */
extern char		path[255];
extern char		file[255];
extern int		cROM;				/* Model 100/102 Option ROM flag */

unsigned char	rambanks[3*32768];	/* Model T200 & NEC RAM banks */
uchar			gRamBank = 0;		/* Currently enabled bank */
uchar			gRomBank = 0;		/* Current ROM Bank selection */

uchar			gReMem = 0;			/* Flag indicating if ReMem emulation enabled */
uchar			gReMemBoot = 0;		/* ALT boot flag */
uchar			gReMemMode = 0;		/* Current ReMem emulation mode */
uchar			gReMemSector = 0;	/* Current Vector access sector */
uchar			gReMemCounter = 0;	/* Current Vector access counter */
unsigned char	*gReMemSectPtr = 0;	/* Pointer to current Sector memory */
unsigned short	gReMemMap[64];		/* Map of 1K blocks - 64 total */
uchar			*gReMemRam = 0;		/* Pointer to ReMem RAM space */
uchar			*gReMemFlash1 = 0;	/* Pointer to ReMem Flash1 space */
uchar			*gReMemFlash2 = 0;	/* Pointer to ReMem Flash2 space */

int				gRampac = 0;		/* Rampac Enable flag */
unsigned char	*gRampacRam = 0;	/* Rampac RAM pointer */
int				gRampacSector = 0;	/* Rampac emulation Sector number */
int				gRampacCounter = 0;	/* Rampac emulation Counter */


unsigned char get_memory8(unsigned short address)
{
	return memory[address];
}

unsigned short get_memory16(unsigned short address)
{
	return memory[address] + (memory[address+1] << 8);
}

void set_memory8(unsigned short address, unsigned char data)
{
	memory[address] = data;;
}

void set_memory16(unsigned short address, unsigned short data)
{
	memory[address] = data & 0xFF;
	memory[address+1] = data >> 8;
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
	int				c, x;
	int				start_1k, end_1k;	// 1K Block address for start & end of copy
	unsigned short	map;
	int				mapped;
	int				cp_ptr;
	unsigned char	*ptr;
	int				limit;

	switch (region)
	{
	case REGION_RAM:
		addr = RAMSTART + address;
		for (c = 0; c < count; c++)
			data[c] = memory[addr++];
		break;

	case REGION_ROM:
		addr = address;
		for (c = 0; c < count; c++)
			data[c] = memory[addr++];
		break;

	case REGION_ROM2:
		addr = address;
		for (c = 0; c < count; c++)
			data[c] = msplanROM[addr++];
		break;

	case REGION_OPTROM:
		addr = address;
		for (c = 0; c < count; c++)
			data[c] = optROM[addr++];
		break;

	case REGION_RAM1:
		// Check if RAM Bank 1 is active & copy from system memory if it is
		if (gRamBank == 0)
		{
			addr = address + RAMSTART;
			for (c = 0; c < count; c++)
				data[c] = memory[addr++];
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
				data[c] = memory[addr++];
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
				data[c] = memory[addr++];
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

		// Check ReMem emulation mode
		if (gReMemMode & REMEM_MODE_NORMAL)
		{
			// Check if copy crosses 1K block boundries
			start_1k = address / 1024;
			end_1k = (address + count) / 1024;

			// Indicate zero bytes copied so far
			cp_ptr = 0;

			// Loop for each 1K block & copy data
			for (x = start_1k; x <= end_1k; x++)
			{
				// Calculate ReMemMap value to search for to test if
				// this block is mapped to system memory
				if (region == REGION_REMEM_RAM)
				{
					map = REMEM_VCTR_FLASH1_CS | REMEM_VCTR_FLASH2_CS;
					ptr = gReMemRam;
				}
				else if (region == REGION_FLASH1)
				{
					map = REMEM_VCTR_RAM_CS | REMEM_VCTR_FLASH2_CS;
					ptr = gReMemFlash1;
				}
				else 
				{
					map = REMEM_VCTR_RAM_CS | REMEM_VCTR_FLASH1_CS;
					ptr = gReMemFlash2;
				}
				map |= x;

				// Loop through all maps and test if this block mapped
				mapped = FALSE;
				for (c = 0; c < 64; c++)
				{
					if ((gReMemMap[c] & ~(REMEM_VCTR_READ_ONLY | REMEM_VCTR_MMU_ACTIVE)) == map)
					{
						mapped = TRUE;
						break;
					}
				}

				// Check if copy is from System Memory or ReMem memory
				if (mapped)
				{
					// Block is mapped -- copy from system memory
					addr = (address - x*1024) + c * 1024;
					for (; (cp_ptr < count) && (addr < (c+1)*1024); cp_ptr++)
						data[cp_ptr] = memory[addr++];
				}
				else
				{
					// Block is not mapped -- copy from appropriate allocation
					addr = address + cp_ptr;
					for (;(cp_ptr < count) & (addr < (x+1)*1024); cp_ptr++)
						data[cp_ptr] = ptr[addr++];
				}
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
			cp_ptr = 0;

			// ReMem Normal mode enabled
			if (gModel == MODEL_T200)
				limit = 40960;
			else
				limit = 32768;

			// Test if reading from "ROM" space
			if (addr < limit)
			{
				// If reading from Flash1 space, read system memory instead
				if (region == REGION_FLASH1)
				{
					// Copy bytes from the system memory
					for (; (cp_ptr < count) && (addr < limit); cp_ptr++)
						data[cp_ptr] = memory[addr++];
				}
				// Test if copying to RAM below the "Normal" region
				else
				{
					// Copy bytes up to the limit from allocation
					for (; (cp_ptr < count) && (addr < limit); cp_ptr++)
						data[cp_ptr] = ptr[addr++];
				}
			}

			// Test if reading from "RAM" space
			if ((addr >= limit) && (addr < 65536))
			{
				if (region == REGION_REMEM_RAM)
				{
					// Copy bytes from the System RAM between limit and 64K
					for (; (cp_ptr < count) && (addr < 65536) ; cp_ptr++)
						data[cp_ptr] = memory[addr++];
				}
				else
				{
					// Copy remainder of bytes above 64K from allocation
					for (; cp_ptr < count; cp_ptr++)
						data[cp_ptr] = ptr[addr++];
				}
			}

			// Copy bytes that are above Normal "RAM" locations
			if (addr >= 65536)
			{
				// Copy all bytes directly if Flash2
				for (; cp_ptr < count; cp_ptr++)
					data[cp_ptr] = ptr[addr++];
			}
		}
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
	int				c, x;
	int				start_1k, end_1k;	// 1K Block address for start & end of copy
	unsigned short	map;
	int				mapped;
	int				cp_ptr;
	unsigned char	*ptr;
	int				limit;

	switch (region)
	{
	case REGION_RAM:
		addr = RAMSTART + address;
		for (c = 0; c < count; c++)
			memory[addr++] = data[c];
		break;

	case REGION_ROM:
		addr = address;
		for (c = 0; c < count; c++)
			memory[addr++] = data[c];
		break;

	case REGION_ROM2:
		addr = address;
		for (c = 0; c < count; c++)
			msplanROM[addr++] = data[c];
		break;

	case REGION_OPTROM:
		addr = address;
		for (c = 0; c < count; c++)
			optROM[addr++] = data[c];
		break;

	case REGION_RAM1:
		// Check if RAM Bank 1 is active & copy from system memory if it is
		if (gRamBank == 0)
		{
			addr = address + RAMSTART;
			for (c = 0; c < count; c++)
				memory[addr++] = data[c];
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
				memory[addr++] = data[c];
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
				memory[addr++] = data[c];
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
		if (gReMemMode & REMEM_MODE_NORMAL)
		{
			// Check if copy crosses 1K block boundries
			start_1k = address / 1024;
			end_1k = (address + count) / 1024;

			// Indicate zero bytes copied so far
			cp_ptr = 0;

			// Loop for each 1K block & copy data
			for (x = start_1k; x <= end_1k; x++)
			{
				// Calculate ReMemMap value to search for to test if
				// this block is mapped to system memory
				if (region == REGION_REMEM_RAM)
				{
					map = REMEM_VCTR_FLASH1_CS | REMEM_VCTR_FLASH2_CS;
					ptr = gReMemRam;
				}
				else if (region == REGION_FLASH1)
				{
					map = REMEM_VCTR_RAM_CS | REMEM_VCTR_FLASH2_CS;
					ptr = gReMemFlash1;
				}
				else 
				{
					map = REMEM_VCTR_RAM_CS | REMEM_VCTR_FLASH1_CS;
					ptr = gReMemFlash2;
				}
				map |= x;

				// Loop through all maps and test if this block mapped
				mapped = FALSE;
				for (c = 0; c < 64; c++)
				{
					if ((gReMemMap[c] & ~(REMEM_VCTR_READ_ONLY | REMEM_VCTR_MMU_ACTIVE)) == map)
					{
						mapped = TRUE;
						break;
					}
				}

				// Check if copy is from System Memory or ReMem memory
				if (mapped)
				{
					// Block is mapped -- copy from system memory
					addr = (address - x*1024) + c * 1024;
					for (; (cp_ptr < count) && (addr < (c+1)*1024); cp_ptr++)
						memory[addr++] = data[cp_ptr];
				}
				else
				{
					// Block is not mapped -- copy from appropriate allocation
					addr = address + cp_ptr;
					for (;(cp_ptr < count) & (addr < (x+1)*1024); cp_ptr++)
						ptr[addr++] = data[cp_ptr];
				}
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
			cp_ptr = 0;

			// ReMem Normal mode enabled
			if (gModel == MODEL_T200)
				limit = 40960;
			else
				limit = 32768;

			// Test if reading from "ROM" space
			if (addr < limit)
			{
				// If reading from Flash1 space, read system memory instead
				if (region == REGION_FLASH1)
				{
					// Copy bytes from the system memory
					for (; (cp_ptr < count) && (addr < limit); cp_ptr++)
						memory[addr++] = data[cp_ptr];
				}
				// Test if copying to RAM below the "Normal" region
				else
				{
					// Copy bytes up to the limit from allocation
					for (; (cp_ptr < count) && (addr < limit); cp_ptr++)
						ptr[addr++] = data[cp_ptr];
				}
			}

			// Test if reading from "RAM" space
			if ((addr >= limit) && (addr < 65536))
			{
				if (region == REGION_REMEM_RAM)
				{
					// Copy bytes from the System RAM between limit and 64K
					for (; (cp_ptr < count) && (addr < 65536) ; cp_ptr++)
						memory[addr++] = data[cp_ptr];
				}
				else
				{
					// Copy remainder of bytes above 64K from allocation
					for (; cp_ptr < count; cp_ptr++)
						ptr[addr++] = data[cp_ptr];
				}
			}

			// Copy bytes that are above Normal "RAM" locations
			if (addr >= 65536)
			{
				// Copy all bytes directly if Flash2
				for (; cp_ptr < count; cp_ptr++)
					ptr[addr++] = data[cp_ptr];
			}
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

	/* Set gReMem and gRampac based on preferences */
	gReMem = (mem_setup.mem_mode == SETUP_MEM_REMEM) || (mem_setup.mem_mode == SETUP_MEM_REMEM_RAMPAC);
	gRampac = (mem_setup.mem_mode == SETUP_MEM_RAMPAC) || (mem_setup.mem_mode == SETUP_MEM_REMEM_RAMPAC);

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
	}
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
			case MODEL_M102:
				fwrite(memory+RAMSTART, 1, RAMSIZE, fd);
				break;

			case MODEL_T200:
				/* Update rambanks array with current memory bank */
				memcpy(&rambanks[gRamBank*24576], &memory[RAMSTART], RAMSIZE);

				/* Save RAM */
				fwrite(rambanks, 1, 3*RAMSIZE, fd);

				/* Write bank number to file */
				fwrite(&gRamBank, 1, 1, fd);

				break;

			case MODEL_PC8201:
			case MODEL_PC8300:
				/* Update rambanks array with current memory bank */
				memcpy(&rambanks[gRamBank*RAMSIZE], &memory[RAMSTART], RAMSIZE);

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
	int		size;

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

	/* Check if ReMem emulation enabled or Base Memory emulation */
	if (gReMem)
	{
		load_remem_ram();		/* Call routine to load ReMem */
	}
	else
	{
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
		case MODEL_M102:
			fread(memory+RAMSTART, 1, RAMSIZE, fd);
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
			memcpy(&memory[RAMSTART], &rambanks[gRamBank*24576], RAMSIZE);

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
			memcpy(&memory[RAMSTART], &rambanks[gRamBank*RAMSIZE], RAMSIZE);

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
void load_opt_rom(void)
{
	int				len, c;
	FILE			*fd;
	unsigned short	start_addr;
	char			buf[65536];

	// Clear the option ROM memory
	memset(optROM,0,OPTROMSIZE);

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
			optROM[c] = buf[c];

	}
	else
	{
		// Open BIN file
		fd=fopen(gsOptRomFile,"rb");
		if(fd!=0) 
		{
			fread(optROM,1, OPTROMSIZE, fd);
			fclose(fd);
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
			memcpy(&rambanks[gRamBank*RAMSIZE], &memory[RAMSTART], RAMSIZE);
			gRamBank = bank;
			memcpy(&memory[RAMSTART], &rambanks[gRamBank*RAMSIZE], RAMSIZE);
		}
	}
	else
	{
		/* Deal with ReMem emulation mode */
		if (gModel == MODEL_T200)
		{
			/* Write all RAM bank blocks back to ReMem memory */
			if (!(gReMemMode & REMEM_MODE_NORMAL))
			{
				/* Copy system to normal mode ReMem RAM */
				remem_copy_system_to_normal();
			}
			else 
			{
				/* Loop through 24 1K RAM Blocks */
				for (block = 40; block < 64; block++)
					remem_copy_block_to_mmu(block);

				/* Now set the new RAM bank */
				gRamBank = bank;
				remem_update_vectors(gReMemMode);

				/* Loop through 24 1K RAM Blocks */
				for (block = 40; block < 64; block++)
					remem_copy_mmu_to_block(block);
			}
		}
	}
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
			if (bank & 0x01) 
			{
				memcpy(memory,optROM,ROMSIZE);
				cROM=1;
			}
			else 
			{
				memcpy(memory,sysROM,ROMSIZE);
				cROM=0;
			}
			break;

		case MODEL_T200:	/* Model 200 emulation */
			gRomBank = bank;
			switch (bank) {
			case 0:
				memcpy(memory,sysROM,ROMSIZE);
				break;
			case 1:
				memcpy(memory, msplanROM, sizeof(msplanROM));
				break;
			case 2:
				memcpy(memory,optROM,sizeof(optROM));
				break;
			}
			break;

		case MODEL_PC8201:	/* NEC laptops */
		case MODEL_PC8300:
			gRomBank = bank;	/* Update global ROM bank var */

			switch (bank)
			{
			case 0:			/* System ROM bank */
				memcpy(memory,sysROM,ROMSIZE);
				break;

			case 1:			/* Option ROM bank */
				memcpy(memory,optROM,ROMSIZE);
				break;

			case 2:			/* RAM banks */
			case 3:
				memcpy(memory,&rambanks[(bank-1)*RAMSIZE],RAMSIZE);
				break;
			}
			break;
		}
	}
	else
	{
		/* Write all ROM bank blocks back to ReMem memory */
		if (gReMemMode & REMEM_MODE_NORMAL)
		{
			/* Calculate number of ROM blocks base on model */
			blocks = gModel == MODEL_T200 ? 40 : 32;

			/* Loop through all 1K ROM Blocks & copy from system to MMU */
			for (block = 0; block < blocks; block++)
				remem_copy_block_to_mmu(block);

			/* Now set the new RAM bank */
			if (gModel == MODEL_T200)
				gRomBank = bank;
			else
				cROM = bank & 0x01;
			remem_update_vectors(gReMemMode);

			/* Loop through all 1K ROM Blocks & copy to system memory */
			for (block = 0; block < blocks; block++)
				remem_copy_mmu_to_block(block);
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

 	/* Calculate which block is being accessed */
	block = address >> 11;	

	/* Test if ReMem memory block is read-only */
	if (gReMemMap[block] & REMEM_VCTR_READ_ONLY)
		return;

	/* Test if accessing MMU vector for current map */
	if (gReMemMap[block] & REMEM_VCTR_MMU_ACTIVE)
	{
		/* Need to validate that address is accssing active portion of MMU page */
	}

	/* Update memory with data */
	memory[address] = data;
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
	int				bank;

	/* Test if ReMem emulation is enabled */
	if (!gReMem || (gReMemRam == 0))
		return 0;

	/* Process port number being accessed */
	switch (port)
	{

	case REMEM_MODE_PORT:		/* ReMem mode port */
		ret = 1;			/* Indicate port processed */

		/* Test if old mode = new mode and do nothing if equal */
		if (data == gReMemMode)
			break;

		/*
		======================================================================
		First copy existing system memory to ReMem memory
		======================================================================
		*/												 

		/* Test if changing from normal mode to ReMem mode */
		if ((data & REMEM_MODE_NORMAL) != (gReMemMode & REMEM_MODE_NORMAL))
		{
			/* Save system memory to ReMem "Normal mode" memory */
			if (!(gReMemMode & REMEM_MODE_NORMAL))
				remem_copy_system_to_normal();
		}

		/* Test if the MAP selection is being changed */
		else if ((data & REMEM_MODE_MAP_BITS) != (gReMemMode & REMEM_MODE_MAP_BITS))
		{
			/* Copy system memory blocks to ReMem blocks */
			if (gReMemMode & REMEM_MODE_NORMAL)
				remem_copy_system_to_mmu();
		}

		/* Calculate ReMem Map vector pointer */
		remem_update_vectors(data);

		/*
		======================================================================
		Now copy ReMem memory to system memory
		======================================================================
		*/
		/* Test if new mode is "Normal" mode */
		if (!(data & REMEM_MODE_NORMAL))
		{
			/* Copy "Normal" ReMem memory space to sysem memory */
			remem_copy_normal_to_system();
		}
		else if ((data & REMEM_MODE_MAP_BITS) != (gReMemMode & REMEM_MODE_MAP_BITS))
		{
			/* Copy MMU data to system RAM */
			remem_copy_mmu_to_system();
		}

		/* Update ReMem mode */
		gReMemMode = data;
		break;


		/* ReMem port for setting the vector access sector */
	case REMEM_SECTOR_PORT:
		/* Test if ReMem emulation enabled */
		if (!gReMem)
			break;

		gReMemSector = data;			/* Update Sector number */
		gReMemCounter = 0;				/* Clear the ReMem counter */
		gReMemSectPtr = (unsigned char *) (gReMemRam + gReMemSector * REMEM_BANK_SIZE);
		ret = 1;						/* Indicate Port I/O processed */
		break;

		/* ReMem port for writing vector data */
	case REMEM_VCTR_PORT:
		/* Insure ReMem enabled */
		if (!gReMem)
			break;

		/* Calculate current MMU ROM bank */
		bank = gModel == MODEL_T200 ? gRomBank : cROM;

		/* Convert current MMU map / ROM bank to sector format */
		romSector = ((gReMemMode & REMEM_MODE_MAP_BITS) >> REMEM_MODE_MAP_SHIFT) *
			REMEM_BANKS_PER_MAP + bank;

		/* Calculate current MMU RAM bank */
		bank = gModel == MODEL_T200 ? gRamBank : 1;

		/* Convert current MMU map / ROM bank to sector format */
		ramSector = ((gReMemMode & REMEM_MODE_MAP_BITS) >> REMEM_MODE_MAP_SHIFT) *
			REMEM_BANKS_PER_MAP + bank;

		/* Test if Current ReMem Sector same as current ReMem Map/Bank selection */
		if ((gReMemSector == romSector) || (gReMemSector == ramSector))
		{
			/* Copy system memory to ReMem memory */
			remem_copy_block_to_mmu(gReMemCounter >> 1);

			/* Update ReMem MMU */
			*gReMemSectPtr = data;

			/* Copy new MMU data to system */
			remem_copy_mmu_to_block(gReMemCounter >> 1);
		}

		/* Write vector data to ReMem RAM */
		*gReMemSectPtr++ = data;
		gReMemCounter++;				/* Increment Sector Counter */
		if (gReMemCounter >= REMEM_BANK_SIZE)
		{
			gReMemSector++;				/* Increment to next sector */
			gReMemCounter = 0;			/* Clear sector counter */
			gReMemSectPtr = (unsigned char *) (gReMemRam + gReMemSector * REMEM_BANK_SIZE);
		}
		ret = 1;						/* Indicate Port I/O processed */
		break;

		/* RamPac Set Vector port */
	case RAMPAC_SECTOR_PORT:
		ret = 1;
		break;

		/* RamPac Write Byte port */
	case RAMPAC_DATA_PORT:
		ret = 1;
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
	int				offset;		/* Offset of RAM vectors within page */
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
			bank = gModel == MODEL_T200 ? gRomBank : cROM << 1;
		else
			bank = gModel == MODEL_T200 ? gRamBank : 1;
		offset = x ? REMEM_RAM_BANK_OFFSET : 0;
		mmuBlock = (unsigned short) ((REMEM_MAP_OFFSET + map * REMEM_MAP_SIZE + 
			bank * REMEM_BANK_SIZE + offset) >> 11);
		pMapPtr = (unsigned short *) (gReMemRam + REMEM_MAP_OFFSET + map * REMEM_MAP_SIZE + 
			bank * REMEM_BANK_SIZE + offset);

		/* Update ReMem ROM/RAM blocks */
		for (c = 0; c < !x ? romBlocks : ramBlocks; c++)
		{
			gReMemMap[block] = pMapPtr[c];		/* Copy map vector from ReMem RAM */

			/* Test for Global R/O bit in ReMemMode for FLASH1 */
			if ((gReMemMap[block] & REMEM_VCTR_FLASH1_CS) == 0)
			{
				if (gReMemMode & REMEM_MODE_FLASH1_RO)
					gReMemMap[block] |= REMEM_VCTR_READ_ONLY;
			}

			/* Test for Global R/O bit in ReMemMode for FLASH2 */
			if ((gReMemMap[block] & REMEM_VCTR_FLASH2_CS) == 0)
			{
				if (gReMemMode & REMEM_MODE_FLASH2_RO)
					gReMemMap[block] |= REMEM_VCTR_READ_ONLY;
			}

			/* Test if Block being mapped is active MMU Map space */
			if ((gReMemMap[block] & REMEM_VCTR_ADDRESS) == mmuBlock)
				gReMemMap[block] |= REMEM_VCTR_MMU_ACTIVE;

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
	int		bytes;				/* Number of bytes to copy */
	int		c;					/* Byte counter */
	int		dest;				/* Destination location */
	uchar	*pSrc;				/* Source address */

	/* First copy ROM to system memory */
	bytes = gModel == MODEL_T200 ? gRomBank ? 32768 : 40960 : 32768;
	pSrc = (gReMemBoot ? gReMemFlash2 : gReMemFlash1) + 
		0x10000 * (gModel == MODEL_T200 ? gRomBank : cROM);
	dest = 0;

	/* Copy the ROM bytes */
	for (c = 0; c < bytes; c++)
		memory[dest++] = *pSrc++;

	/* Now prepare to copy the RAM bytes */
	bytes = gModel == MODEL_T200 ? 24576 : 32768;
	pSrc = gReMemRam + 0x10000 * (gModel == MODEL_T200 ? gRamBank : cROM) +
		(gModel == MODEL_T200 ? 40960 : 32768);

	/* Copy the RAM bytes */
	for (c = 0; c < bytes; c++)
		memory[dest++] = *pSrc++;
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
	int			c;				/* Byte counter */
	uchar		*pSrc;			/* Source pointer */
	int			dest;			/* Destination pointer */

	/* Calculate source pointer based on block vector */
	if ((gReMemMap[block] & REMEM_VCTR_RAM_CS) == 0)
		pSrc = gReMemRam;
	else if ((gReMemMap[block] & REMEM_VCTR_FLASH1_CS) == 0)
		pSrc = gReMemFlash1;
	else if ((gReMemMap[block] & REMEM_VCTR_FLASH1_CS) == 0)
		pSrc = gReMemFlash2;
	else
		return;

	/* Calculate offset of 1K block */
	pSrc += (gReMemMap[block] & REMEM_VCTR_ADDRESS) << 11;
	dest = block * 1024;

	/* Copy bytes */
	for (c = 0; c < 1024; c++)
		memory[dest++] = *pSrc++;
}

/*
=============================================================================
remem_copy_system_to_normal:	This function copies memory from System 
								memory space to "Normal" mode space.
=============================================================================
*/
void remem_copy_system_to_normal(void)
{
	int		bytes;				/* Number of bytes to copy */
	int		c;					/* Byte counter */
	int		src;				/* Destination location */
	uchar	*pDest;				/* Source address */

	/* Copy only the RAM bytes */
	if (gModel == MODEL_T200)
	{
		bytes = 24576;
		pDest = gReMemRam + 0x10000 * gRamBank + 40960;
		src = 40960;
	}
	else
	{
		bytes = 32768;
		pDest = gReMemRam + 0x10000 * cROM + 32768;
		src = 32768;
	}

	/* Copy the RAM bytes */
	for (c = 0; c < bytes; c++)
		*pDest++ = memory[src++];
}

/*
=============================================================================
remem_copy_system_to_mmu:	This function copies memory from System 
							memory space to MMU mode space.
=============================================================================
*/
void remem_copy_system_to_mmu(void)
{
	int			block;			/* Block being copied */

	/* Loop through all blocks */
	for (block = 0; block < 64; block++)
	{
		remem_copy_block_to_mmu(block);
	}
}

/*
=============================================================================
remem_copy_block_to_mmu:	This function copies a 1K block of RAM from
							system space to ReMem space base on the current
							ReMem mode settings.  The spcified 1K block of
							system RAM is copied.
=============================================================================
*/
void remem_copy_block_to_mmu(int block)
{
	int			c;				/* Byte counter */
	uchar		*pDest;			/* Destination pointer */
	int			src;			/* Source pointer */

	/* Check if block is read-only, skip copy if read-only*/
	if (gReMemMap[block] & REMEM_VCTR_READ_ONLY)
		return;

	/* Calculate source pointer base on block vector */
	if ((gReMemMap[block] & REMEM_VCTR_RAM_CS) == 0)
		pDest = gReMemRam;
	else if ((gReMemMap[block] & REMEM_VCTR_FLASH1_CS) == 0)
		pDest = gReMemFlash1;
	else if ((gReMemMap[block] & REMEM_VCTR_FLASH1_CS) == 0)
		pDest = gReMemFlash2;
	else
		return;

	/* Calculate offset of 1K block */
	pDest += (gReMemMap[block] & REMEM_VCTR_ADDRESS) << 11;
	src = block * 1024;

	/* Copy bytes */
	for (c = 0; c < 1024; c++)
		*pDest++ = memory[src++];
}

