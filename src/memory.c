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
int				gRamBottom = 0x8000;/* Defines the amount of RAM installed */
uchar			gBaseMemory[65536];	/* System Memory */
uchar			gSysROM[4*32768];	/* System ROM */
uchar			gOptROM[32768];		/* Option ROM */
uchar			gMsplanROM[32768];	/* MSPLAN ROM T200 Only */
//extern char		path[255];
extern char		file[255];

int				gOptRomRW = 0;		/* Flag to make OptROM R/W */
unsigned char	rambanks[4*32768];	/* Model T200 & NEC RAM banks */
uchar			gRamBank = 0;		/* Currently enabled bank */
int				gRomBank = 0;		/* Current ROM Bank selection */
static int		gRom0Bank = 0;		/* Current ROM #0 Bank for PC-8300 */
int				gRomSize = 32768;   /* Current ROM Size for R/O calculation */
unsigned char   gQuad = 0;          /* QUAD (128K RAM bank on M100) enabled */

uchar			gReMem = 0;			/* Flag indicating if ReMem emulation enabled */
uchar			gReMemBoot = 0;		/* ALT boot flag */
uchar			gReMemMode = 0;		/* Current ReMem emulation mode */
uchar			gReMemSector = 0;	/* Current Vector access sector */
uchar			gReMemCounter = 0;	/* Current Vector access counter */
unsigned char	*gReMemSectPtr = 0;	/* Pointer to current Sector memory */
unsigned short	gReMemMap[64];		/* Map of 1K blocks - 64 total */
unsigned int	gReMemMapLower = 0; /* Lower address of actively mapped MMU Map */
unsigned int	gReMemMapUpper = 0; /* Upper address of actively mapped MMU Map */
int				gReMemFlashReady = TRUE;
amd_flash_t		gReMemFlash1 = { FLASH_STATE_RO, 0, FALSE, 0 };
amd_flash_t		gReMemFlash2 = { FLASH_STATE_RO, 0, FALSE, 0 };

uchar			*gReMemRam = 0;		/* Pointer to ReMem RAM space */

int				gRampac = 0;		/* Rampac Enable flag */
unsigned char	*gRampacRam = 0;	/* Rampac RAM pointer */
int				gRampacSector = 0;	/* Rampac emulation Sector number */
int				gRampacCounter = 0;	/* Rampac emulation Counter */
unsigned char	*gRampacSectPtr = 0;/* Pointer to current Sector memory */
int				gRampacSaveSector = 0;	/* Rampac emulation Sector number */
int				gRampacSaveCounter = 0;	/* Rampac emulation Counter */
unsigned char	*gRampacSaveSectPtr = 0;/* Pointer to current Sector memory */
int				gRampacEmulation = 0;/* ReMem's Rampac emulation active? */

int				gRex = 0;			/* Rex Emulation Enable flag */
unsigned char	*gRex2Ram = NULL;	/* Rex2 RAM pointer */
int				gRexSector = 0;		/* Active sector */
int				gRexState = 7;		/* Rex Command State */
unsigned char	gRexReturn = 0;		/* Return value for Status & HW reads */
unsigned char	gRexModel = 0;		/* Rex model */
unsigned char	gRexFlashSel = 1;	/* Rex Flash enable signal */
int				gRexKeyState = 0;	/* Rex Key State */
int				gRexFlashPA = 0;	/* REX Flash Address Line during programing */
unsigned char	gRex3Data = 0;		/* REX3 data to write for commands 2,5 and 6 */
unsigned char	gRex3Cmd = 0;		/* REX3 command saved for state 6 */
amd_flash_t		gRexFlash = { FLASH_STATE_RO, 0, FALSE, 0 };
unsigned char	gRexKeyTable[6] = { 184, 242, 52, 176, 49, 191 };
int				gIndex[65536];

extern RomDescription_t		gM100_Desc;
extern RomDescription_t		gM200_Desc;
extern RomDescription_t		gN8201_Desc;
extern RomDescription_t		gN8300_Desc;
extern RomDescription_t		gM10_Desc;
//JV
extern RomDescription_t		gKC85_Desc;

extern int					gShowVersion;
extern UINT64				cycles;
extern int					cycle_delta;

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
		if  (gRex)
			return rex_read(address);
		else
		{
			if (gReMemFlashReady)
				return gMemory[address >> 10][address & 0x3FF];
			/* Process Flash state machine and return the value */
			return remem_flash_sm_read(address);
		}
	}
	else
		return (address>ROMSIZE&&address<RAMBOTTOM)? 0xFF:gBaseMemory[address];
}

unsigned short get_memory16(unsigned short address)
{
	if (gReMem)
	{
		if (gRex)
			return rex_read(address) | (rex_read(address+1) << 8);
		return gMemory[address>>10][address&0x3FF] + (gMemory[(address+1)>>10][(address+1)&0x3FF] << 8);
	}
	else
		return (address>ROMSIZE&&address<RAMBOTTOM)? 0xFFFF: gBaseMemory[address] + (gBaseMemory[address+1] << 8);
}

void set_memory8(unsigned short address, unsigned char data)
{
	if (gReMem)
	{
		if (gRex)
			rex_set8(address, data);
		else
			remem_set8(address, data);
	}
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
			if (gReMem && !gRex)
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
			if (gReMem && !gRex)
				data[c] = gMemory[addr>>10][addr&0x3FF];
			else
				data[c] = gBaseMemory[addr];
			addr++;
		}
		break;

	case REGION_ROM2:
		addr = address;
		if (gModel == MODEL_T200)
		{
			// Copy from the MsPlan ROM
			for (c = 0; c < count; c++)
				data[c] = gMsplanROM[addr++];
		}
		else if (gModel == MODEL_PC8300)
		{
			// Copy from ROM Bank B
			for (c = 0; c < count; c++)
				data[c] = gSysROM[32768 + addr++];
		}
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
				if (gReMem && !gRex)
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
				if (gReMem && !gRex)
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
		// Check if RAM Bank 3 is active & copy from system memory if it is
		if (gRamBank == 2)
		{
			addr = address + RAMSTART;
			for (c = 0; c < count; c++)
			{
				if (gReMem && !gRex)
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

	case REGION_RAM4:
		// Check if RAM Bank 4 is active & copy from system memory if it is
		if (gRamBank == 3)
		{
			addr = address + RAMSTART;
			for (c = 0; c < count; c++)
			{
				if (gReMem && !gRex)
					data[c] = gMemory[addr>>10][addr&0x3FF];
				else
					data[c] = gBaseMemory[addr];
				addr++;
			}
		}
		else
		{
			if (gModel == MODEL_T200)
				addr = 24576*3 + address;
			else
				addr = 32768*3 + address;
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
		if ((gReMemRam == NULL) || (gReMemFlash1.pFlash == NULL) || (gReMemFlash2.pFlash == NULL))
			break;

		// Indicate zero bytes copied so far
		cp_ptr = 0;

		// Determine which location to copy memory from
		if (region == REGION_REMEM_RAM)
			ptr = gReMemRam;
		else if (region == REGION_FLASH1)
			ptr = (uchar *) gReMemFlash1.pFlash;
		else 
			ptr = (uchar *) gReMemFlash2.pFlash;

		// Block is not mapped -- copy from 2Meg Region (RAM or FLASH)
		addr = address;
		for (;(cp_ptr < count); cp_ptr++)
			data[cp_ptr] = ptr[addr++];
		break;

		/*
		================================================================
		The following deals with REX emulation of FLASH.
		================================================================
		*/
	case REGION_REX_FLASH:
		// First check that the ReMem RAM has been allocated
		if (gRexFlash.pFlash == NULL)
			break;

		// Indicate zero bytes copied so far
		cp_ptr = 0;

		// Determine which location to copy memory from
		ptr = (uchar *) gRexFlash.pFlash;

		// Block is not mapped -- copy from 2Meg Region (RAM or FLASH)
		addr = address;
		for (;(cp_ptr < count); cp_ptr++)
			data[cp_ptr] = ptr[addr++];
		break;

	case REGION_ROM3:
		addr = address;
		if (gModel == MODEL_PC8300)
		{
			// Copy from ROM Bank B
			for (c = 0; c < count; c++)
				data[c] = gSysROM[2*32768 + addr++];
		}
		break;

	case REGION_ROM4:
		addr = address;
		if (gModel == MODEL_PC8300)
		{
			// Copy from ROM Bank B
			for (c = 0; c < count; c++)
				data[c] = gSysROM[3*32768 + addr++];
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
			if (gReMem && !gRex)
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
			if (gReMem && !gRex)
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
				if (gReMem && !gRex)
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
				if (gReMem && !gRex)
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
		// Check if RAM Bank 3 is active & copy from system memory if it is
		if (gRamBank == 2)
		{
			addr = address + RAMSTART;
			for (c = 0; c < count; c++)
			{
				if (gReMem && !gRex)
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

	case REGION_RAM4:
		// Check if RAM Bank 4 is active & copy from system memory if it is
		if (gRamBank == 3)
		{
			addr = address + RAMSTART;
			for (c = 0; c < count; c++)
			{
				if (gReMem && !gRex)
					gMemory[addr>>10][addr&0x3FF] = data[c];
				else
					gBaseMemory[addr] = data[c];
				addr++;
			}
		}
		else
		{
			if (gModel == MODEL_T200)
				addr = 24576*3 + address;
			else
				addr = 32768*3 + address;
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
		if ((gReMemRam == NULL) || (gReMemFlash1.pFlash == NULL) || 
			(gReMemFlash2.pFlash == NULL))
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
				ptr = (uchar *) gReMemFlash1.pFlash;
			else if (region == REGION_FLASH2)
				ptr = (uchar *) gReMemFlash2.pFlash;

			addr = address;

			// Copy bytes up to the limit to allocation
			for (cp_ptr = 0; cp_ptr < count; cp_ptr++)
				ptr[addr++] = data[cp_ptr];
		}
		break;

		/*
		================================================================
		The following deals with REX emulation of FLASH.
		================================================================
		*/
	case REGION_REX_FLASH:
		/* Do sanity check on the pointer */
		if (gRexFlash.pFlash == NULL)
			return;

		/* Get pointer to the REX flash */
		ptr = (uchar *) gRexFlash.pFlash;

		/* Setup to copy */
		addr = address;

		// Copy bytes up to the limit to allocation
		for (cp_ptr = 0; cp_ptr < count; cp_ptr++)
			ptr[addr++] = data[cp_ptr];

		break;
	}
}

/*
========================================================================
amd_flash_init:		This routine initializes an amd_flash_t structure
					and allocates memory of the specified size.
========================================================================
*/
void amd_flash_init(amd_flash_t *pFlash, int size, int type)
{
	int		c;

	/* Allocate RAM for the flash and initiaize its state */
	if (pFlash->pFlash == NULL)
	{
		pFlash->pFlash = malloc(size);
	}
	pFlash->iFlashState = FLASH_STATE_RO;
	pFlash->iFlashBusy = FALSE;
	pFlash->iFlashTime = 0;
	pFlash->iFlashType = type;
	pFlash->iFlashSize = size;

	/* Initialize memory to zero */
	if (pFlash->pFlash != NULL)
	{
		for (c = 0; c < size; c++)
		{
			pFlash->pFlash[c] = 0xff;
		}
	}
}

/*
=============================================================================
amd_flash_read_sm:	This routine processes FLASH read events while a timer
					is active, such as during a write or erase operation.
=============================================================================
*/
int amd_flash_read_sm(amd_flash_t *pFlash, unsigned short address)
{
	int		index;

	/* The flash is busy during a read - process as State Machine */
	if (pFlash->iFlashBusy && (pFlash->iFlashState == FLASH_STATE_RO))
	{
		/* Waiting for timeout of erase / program */
		if (pFlash->iFlashTime > 0)
		{
			if (cycles + cycle_delta >= pFlash->iFlashTime)
				pFlash->iFlashTime = 0;
		}

		/* Test if timeout expired */
		if (pFlash->iFlashTime == 0)
		{
			pFlash->iFlashBusy = FALSE;
			return -1;
		}
	}
	else if (pFlash->iFlashState == FLASH_STATE_AUTOSELECT)
	{
		if ((address & 0xFF) == 0)
			return FLASH_MANUF_ID;
		else if ((address & 0xFF) == 1)
			return FLASH_PRODUCT_ID;
		else
			return 0;
	}
	else if (pFlash->iFlashState == FLASH_STATE_CFI_QUERY)
	{
		/* Test Query address */
		if ((address < 32) || (address > 0x98))
			return 0xFF;

		/* Calculate index for CFI address */
		index = (address - 0x20) >> 1;
		return gFlashCFIData[index];
	}

	/* Test for failed command sequence */
	if (pFlash->iFlashState != FLASH_STATE_RO)
	{
		/* Reset back to read state */
		pFlash->iFlashState = FLASH_STATE_RO;
		pFlash->iFlashBusy = FALSE;
		pFlash->iFlashTime = 0;
		return -1;
	}

	return 0xFF;
}

/*
========================================================================
amd_flash_proc_timer:	This routine processes timed events, such as
						write and erase to determine when they are
						complete.
========================================================================
*/
int amd_flash_proc_timer(amd_flash_t *pFlash)
{
	/* Check if FLASH1 is busy */
	if (pFlash->iFlashTime > 0)
	{
		if (cycles + cycle_delta >= pFlash->iFlashTime)
		{
			/* Clear busy bit for Flash1 */
			pFlash->iFlashTime = 0;
			pFlash->iFlashBusy = FALSE;
			return AMD_ACTION_DEVICE_READY;
		}

		return AMD_ACTION_BUSY;
	}

	return AMD_ACTION_DEVICE_READY;
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
    gRex = (mem_setup.mem_mode == SETUP_MEM_REX || mem_setup.mem_mode == SETUP_MEM_REX_QUAD) ? REX : 
           (mem_setup.mem_mode == SETUP_MEM_REX2) ? REX2 : 0;
    gQuad = (mem_setup.mem_mode == SETUP_MEM_QUAD || mem_setup.mem_mode == SETUP_MEM_REX_QUAD) ? 1 : 0;

	if (gRex)
	{
		gRexModel = REX | gRex;		
		gReMem = 1;
		gRexState = 7;
		gRexFlash.iFlashState = FLASH_STATE_RO;
		gRexSector = 0;
		gRexKeyState = 0;
	}
	gRampacEmulation = 0;
	gRampacSectPtr = NULL;
	gRamBank = 0;
	gRomBank = 0;

    if (gQuad)
        set_ram_bank(0);

	// Initialize ROM size base on current model
	gRomSize = gModel == MODEL_T200 ? 40960 : 32768;

	for (c = 0; c < 65536; c++)
		gIndex[c] = c >> 10;

    /* Zero out QUAD memory */
    if (gQuad)
    {
        for (c = 0; c < 65536*2; c++)
            rambanks[c] = 0;
    }

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

			/* Initialize memory to zero */
			for (c = 0; c < size; c++)
			{
				gReMemRam[c] = 0;
			}

			amd_flash_init(&gReMemFlash1, size, AMD_FLASH_TYPE_REMEM);
			amd_flash_init(&gReMemFlash2, size, AMD_FLASH_TYPE_REMEM);
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

	/* Test if Rex emulation enabled */
	if (gRex)
	{
		/* Allocate memory only if not already allocated */
		if  (gRexFlash.pFlash == 0)
		{
			/* Size of allocation for RAM and FLASH */
			size = 1024 * 1024;

			/* Allocate space for Rex FLASH */
			amd_flash_init(&gRexFlash, size, AMD_FLASH_TYPE_REX);
		}

		/* Test if Rex2 enabled and allocate RAM */
		if  ((gRex == REX2) && (gRex2Ram == 0))
		{
			/* Size of allocation for RAM and FLASH */
			size = 1024 * 128;

			/* Allocate space for Rex2 RAM */
			gRex2Ram = malloc(size);

			/* Initialize memory to zero */
			for (c = 0; c < size; c++)
				gRex2Ram[c] = 0;
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

	/* Test if Rex needs to be reinitialized */
	if (gRex)
	{
		/* Initialize Rex state varialbles */
		gRexState = 7;
		gRexFlash.iFlashState = FLASH_STATE_RO;
		gRexSector = 0;
		gRexKeyState = 0;
	}

	// Clear the RAM
//	for (x = ROMSIZE; x < 65536; x++)
//		set_memory8(x, 0);

//	gRamBank = 0;
	gRomBank = 0;

}

/*
============================================================================
Clear RAM to emulate a cold-boot based on Memory mode selected.
============================================================================
*/
void cold_boot_mem(void)
{
	int		x;

	// Reload the system ROM just in case it got over-written
	load_sys_rom();

	// Now zero out the RAM
	if (gReMem && (gRex == 0))
	{
	}
	else
	{
		for (x = ROMSIZE; x < 65536; x++)
			gBaseMemory[x] = 0;
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
	free(gReMemFlash1.pFlash);
	free(gReMemFlash2.pFlash);

	/* Set memory pointers to NULL */
	gReMemRam = 0;
	gReMemFlash1.pFlash = 0;
	gReMemFlash2.pFlash = 0;

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
free_rex_mem:	This routine frees the memory used by the Rex 
				emulation and resets the pointers to zero.
========================================================================
*/
void free_rex_mem(void)
{
	/* Delete memory allocated for RAM and FLASH */
	if (gRexFlash.pFlash != NULL)
		free(gRexFlash.pFlash);

	/* Set memory pointers to NULL */
	gRexFlash.pFlash = 0;
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

	if (gRex != 0)
		free_rex_mem();
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

	/* Print error if unable to open the file */
	if (fd == NULL)
	{
		char  msg[100];
		sprintf(msg, "Could not save Remem file %s", mem_setup.remem_file);
		show_error(msg);
		return;
	}

	/* Check if file opened successfully */
	if (fd != 0)
	{
		size = 1024 * 2048;		/* Copy 2 meg of RAM & FLASH */

		/* Write ReMem RAM first */
		fwrite(gReMemRam, 1, size, fd);

		/* Now write Flash */
		fwrite(gReMemFlash1.pFlash, 1, size, fd);
		fwrite(gReMemFlash2.pFlash, 1, size, fd);

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

	/* Print error if unable to open the file */
	if (fd == NULL)
	{
		char  msg[100];
		sprintf(msg, "Could not save Rampac file %s", mem_setup.rampac_file);
		show_error(msg);
		return;
	}

	/* Check if file opened successfully */
	if (fd != 0)
	{
		size = 1024 * 256;		/* Copy 256 K of RAM */

		/* Write ReMem RAM first */
		fwrite(gRampacRam, 1, size, fd);

		/* Close the file */
		fclose(fd);
	}
}

/*
========================================================================
save_rex2_ram:	This routine saves the Rex2 emulation RAM to the Rex2
				AM file.
========================================================================
*/
void save_rex2_ram(void)
{
	FILE	*fd;
	int		size;

	if (gRex != REX2)
		return;

	/* Open ReMem file */
	fd = fopen(mem_setup.rex2_ram_file, "wb+");

	/* Print error if unable to open the file */
	if (fd == NULL)
	{
		char  msg[100];
		sprintf(msg, "Could not save REX2 RAM file %s", mem_setup.rex2_ram_file);
		show_error(msg);
		return;
	}

	/* Check if file opened successfully */
	if (fd != 0)
	{
		size = 1024 * 128;		/* Copy 256K of RAM */

		/* Write ReMem RAM first */
		fwrite(gRex2Ram, 1, size, fd);

		/* Close the file */
		fclose(fd);
	}
}

/*
========================================================================
save_rex_flash:	This routine saves the Rex emulation flash to the Rex
				flash file.
========================================================================
*/					
void save_rex_flash(void)
{
	FILE	*fd;
	int		size;

	/* Open ReMem file */
	fd = fopen(mem_setup.rex_flash_file, "wb+");

	/* Print error if unable to open the file */
	if (fd == NULL)
	{
		char  msg[100];
		sprintf(msg, "Could not save REX file %s", mem_setup.rex_flash_file);
		show_error(msg);
		return;
	}

	/* Check if file opened successfully */
	if (fd != 0)
	{
		size = 1024 * 1024;		/* Copy 1 Meg of flash */

		/* Write ReMem RAM first */
		fwrite(gRexFlash.pFlash, 1, size, fd);

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
	if (gReMem & !gRex)
	{
		save_remem_ram();		/* Save ReMem data to file */
	}
	else
	{
		/* Base memory -- First get the emulation path */
		get_emulation_path(file, gModel);

		/* Append the RAM filename for Base Memory emulation */
        if (gModel == MODEL_M100 && gQuad)
            strcat(file, "QUAD.bin");
        else
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
                if (gQuad)
                {
                    set_ram_bank(gRamBank);
                    fwrite(rambanks, 1, 4*RAMSIZE, fd);
                }
                else
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

	/*
	===========================================
	Save Rex Flash & RAM if enabled
	===========================================
	*/
	if (gRex)
	{
		save_rex_flash();		/* Save the flash */
		save_rex2_ram();		/* Save Rex2 RAM to file */
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
	int		readlen;

	/* Open ReMem file */
	fd = fopen(mem_setup.remem_file, "rb+");

	/* Print error if unable to open the file */
	if (fd == NULL)
	{
		char  msg[100];
		sprintf(msg, "Could not open Remem file %s", mem_setup.remem_file);
		show_error(msg);
		return;
	}

	/* Check if file opened successfully */
	if ((fd != 0) && (gReMemFlash1.pFlash != NULL) && (gReMemFlash2.pFlash != NULL))
	{
		size = 1024 * 2048;		/* Copy 2 meg of RAM & FLASH */

		/* Read ReMem RAM first */
		readlen = fread(gReMemRam, 1, size, fd);

		/* Now read Flash */
		readlen = fread(gReMemFlash1.pFlash, 1, size, fd);
		readlen = fread(gReMemFlash2.pFlash, 1, size, fd);

		/* Close the file */
		fclose(fd);

		/* Now update the VirtualT version if needed */
		patch_vt_version(gReMemFlash1.pFlash, ROMSIZE);
	}

	/* Test if ReMem FLASH "Normal" region is empty */
	if (gReMemFlash1.pFlash != NULL)
	{
		for (x = 0; x < 10; x++)
		{
			/* Check for any non-zero bytes in normal ROM area */
			if (gReMemFlash1.pFlash[x] != 0)
				empty = 0;
		}
	}

	if (empty)
	{
		/* Rom area is empty - initialize with SYSROM */
		for (x = 0; x < ROMSIZE; x++)
			gReMemFlash1.pFlash[x] = gSysROM[x];
	}

	/* Initalize MsplanROM area */
	if (gModel == MODEL_T200)
		for (x = 0; x < sizeof(gMsplanROM); x++)
			gReMemFlash1.pFlash[0x10000 + x] = gMsplanROM[x];
}

void reload_sys_rom(void)
{
	int		x;
	
	for (x = 0; x < ROMSIZE; x++)
		gReMemFlash1.pFlash[x] = gSysROM[x];
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
	int		readlen;

	/* Open Rampac file */
	fd = fopen(mem_setup.rampac_file, "rb+");

	/* Print error if unable to open the file */
	if (fd == NULL)
	{
		char  msg[100];
		sprintf(msg, "Could not open Rampac file %s", mem_setup.rampac_file);
		show_error(msg);
		return;
	}

	/* Check if file opened successfully */
	if (fd != 0)
	{
		size = 1024 * 256;		/* Copy 256 K of RAM */

		/* Write ReMem RAM first */
		readlen = fread(gRampacRam, 1, size, fd);

		/* Close the file */
		fclose(fd);
	}
}

/*
========================================================================
load_rex_flash:	This routine loads the Rex emulation flash from the 
				Rex flash file.
========================================================================
*/					
void load_rex_flash(void)
{
	FILE	*fd;
	int		size;
	int		readlen;

	/* Open ReMem file */
	fd = fopen(mem_setup.rex_flash_file, "rb+");

	/* Print error if unable to open the file */
	if (fd == NULL)
	{
		char  msg[100];
		sprintf(msg, "Could not open REX file %s", mem_setup.rex_flash_file);
		show_error(msg);
		return;
	}

	/* Check if file opened successfully */
	if ((fd != 0) && (gRexFlash.pFlash != NULL))
	{
		size = 1024 * 1024;		/* Copy 1 meg of FLASH */

		/* Read Rex Flash first */
		readlen = fread(gRexFlash.pFlash, 1, size, fd);

		/* Close the file */
		fclose(fd);

		/* Now update the VirtualT version if needed */
		patch_vt_version(gRexFlash.pFlash, ROMSIZE);
	}
}

/*
========================================================================
load_rex2_ram:	This routine loads the Rex2 emulation RAM from the 
				Rex2 RAM file.
========================================================================
*/					
void load_rex2_ram(void)
{
	FILE	*fd;
	int		size;
	int		readlen;

	/* Open ReMem file */
	fd = fopen(mem_setup.rex2_ram_file, "rb+");

	/* Print error if unable to open the file */
	if (fd == NULL)
	{
		char  msg[100];
		sprintf(msg, "Could not open REX2 RAM file %s", mem_setup.rex2_ram_file);
		show_error(msg);
		return;
	}

	/* Check if file opened successfully */
	if (fd != 0)
	{
		size = 1024 * 128;		/* Copy 128K of RAM */

		/* Read Rex2 RAM */
		readlen = fread(gRex2Ram, 1, size, fd);

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
    char            sbank[10];
	FILE			*fd;
	int				x;
	int				readlen;

	/* Check if ReMem emulation enabled or Base Memory emulation */
	if (gReMem & !gRex)
	{
		/* In ReMem mode - load RAM */
		load_remem_ram();		/* Call routine to load ReMem */
		remem_copy_normal_to_system();
	}
	else
	{
        /* Zero the base memory */
		for (x = 0; x < 64; x++)
			gMemory[x] = 0;

		/* First get the emulation path */
		get_emulation_path(file, gModel);

		/* Append the RAM filename */
        if (gModel == MODEL_M100 && gQuad)
            strcat(file, "QUAD.bin");
        else
        {
            strcat(file, "RAM.bin");
            display_map_mode("");
        }

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
            if (gQuad)
            {
				gRamBank = 0;
			    readlen = fread(rambanks, 1, RAMSIZE*4, fd);
                memcpy(&gBaseMemory[RAMSTART], &rambanks[gRamBank*32768], RAMSIZE);
                sprintf(sbank, "Bank %d", gRamBank+1);
                display_map_mode(sbank);
            }
            else
                readlen = fread(gBaseMemory+RAMSTART, 1, RAMSIZE, fd);
			break;

		case MODEL_T200:
			/* Read all ram into rambanks array */
			readlen = fread(rambanks, 1, 3*RAMSIZE, fd);

			/* Read bank number to file */
			readlen = fread(&gRamBank, 1, 1, fd);

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
			readlen = fread(rambanks, 1, 3*RAMSIZE, fd);

			/* Read bank number to file */
			readlen = fread(&gRamBank, 1, 1, fd);
			readlen = fread(&gRomBank, 1, 1, fd);

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

	/*
	===========================================
	Load Rex Flash and RAM if enabled
	===========================================
	*/
	if (gRex)
	{
		/* Load Rex Flash and RAM if needed */
		load_rex_flash();
		if (gRex == REX2)
			load_rex2_ram();
	}
}

/*
=============================================================================
patch_vt_version:  This function patches the VirtualT verison in the ROM
					memory space specified.  It searches for M**soft and
					old VirtulT Version strings in case an old VT version is
					saved in a ReMem Flash memory.
=============================================================================
*/
void patch_vt_version(char* pMem, int size)
{
	unsigned short		address;
	char				oldString[15];
	char				newString[40];
	int					c, srchLen, strLen;
	int					found = FALSE;

	/* Validate the ROM was found and we have a good pointer */
	if (gStdRomDesc == NULL)
		return;

	/* Test if VirtulalT version should replace (C) Micro***t text */
	if (gShowVersion)
	{
		if (gModel == MODEL_PC8201 || gModel == MODEL_PC8300)
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

		/* Check if the address contains a (possibly previous version) VT string */
		if (strncmp((char*) &pMem[address], "VirtualT ", strlen("VirtualT ")) == 0)
			found = TRUE;

		/* Check if the copyright string is where we expect it to be */
		if (strncmp((char*) &pMem[address], oldString, strlen(oldString)) == 0)
			found = TRUE;

		if (!found)
		{
			/* Okay, the copyright string has been moved (by the user?)  Find it */
			address = 0;
			srchLen = size-strlen(oldString)-1;
			strLen = strlen(oldString);

			/* Loop through entire memory range */
			for (c = 0; c < srchLen; c++)
			{
				/* Search for MSoft string */
				if (strncmp((char*) &pMem[c], oldString, strLen) == 0)
				{
					address = (unsigned short) c;
					break;
				}
				/* Search for VirtualT string */
				if (strncmp((char*) &pMem[c], "VirtualT ", strlen("VirtualT ")) == 0)
				{
					address = (unsigned short) c;
					break;
				}
			}
		}

		/* If the address is good, update the ROM */
		if (address != 0)
			strcpy((char*) &pMem[address], newString);
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
	int				readlen;

	/* Set pointer to ROM Description */
	if (gModel == MODEL_T200)
		gStdRomDesc = &gM200_Desc;
	else if (gModel == MODEL_PC8201)
		gStdRomDesc = &gN8201_Desc;
	else if (gModel == MODEL_M10)
		gStdRomDesc = &gM10_Desc;
	else if (gModel == MODEL_KC85)
		gStdRomDesc = &gKC85_Desc;
	else if (gModel == MODEL_PC8300)
		gStdRomDesc = &gN8300_Desc;
	else 
		gStdRomDesc = &gM100_Desc;

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
	if ((int) fread(gSysROM, 1, ROMSIZE, fd) < ROMSIZE)
	{
		show_error("Error reading ROM file: Bad Rom file");
	}

	/* If Model = T200 then read the 2nd ROM (MSPLAN) */
	if (gModel == MODEL_T200)
		readlen = fread(gMsplanROM, 1, 32768, fd);

	/* If Model is PC-8300, then read the rest of the 128K ROM */
	if (gModel == MODEL_PC8300)
		readlen = fread(&gSysROM[32768], 1, 3 * 32768, fd);
	
	/* Close the ROM file */
	fclose(fd);

	/* Patch the ROM with VirtualT version if requested */
	patch_vt_version((char *) gSysROM, ROMSIZE);
	if (gModel == MODEL_PC8300)
		patch_vt_version((char *) &gSysROM[2*32768], ROMSIZE);

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
	int				readlen;

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
					gReMemFlash1.pFlash[0x18000 + c] = buf[c];
			else
				for (c = 0; c < len; c++)
					gReMemFlash1.pFlash[0x8000 + c] = buf[c];
		}

	}
	else
	{
		// Open BIN file
		fd=fopen(gsOptRomFile,"rb");
		if(fd!=0) 
		{
			readlen = fread(gOptROM,1, OPTROMSIZE, fd);
			fclose(fd);
		}	

		if (gReMem)
		{
			if (gModel == MODEL_T200)
				for (c = 0; c < OPTROMSIZE; c++)
					gReMemFlash1.pFlash[0x18000 + c] = gOptROM[c];
			else
				for (c = 0; c < OPTROMSIZE; c++)
					gReMemFlash1.pFlash[0x8000 + c] = gOptROM[c];
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
    char    sbank[10];
	int		block;

	if (!(gReMem && !gRex))
	{
		/* Deal with Non-Remem Banks */
		switch (gModel)
		{
        case MODEL_M100:    /* Moel 100 QUAD support */
            if (gQuad)
            {
                memcpy(&rambanks[gRamBank*RAMSIZE], &gBaseMemory[RAMSTART], RAMSIZE);
                gRamBank = bank;
                memcpy(&gBaseMemory[RAMSTART], &rambanks[gRamBank*RAMSIZE], RAMSIZE);
                sprintf(sbank, "Bank %d", bank+1);
                display_map_mode(sbank);
            }
            break;

		case MODEL_T200:	/* Model T200 RAM banks */
		case MODEL_PC8201:	/* NEC Laptop banks */
		case MODEL_PC8300:
			memcpy(&rambanks[gRamBank*RAMSIZE], &gBaseMemory[RAMSTART], RAMSIZE);
			gRamBank = bank;
			memcpy(&gBaseMemory[RAMSTART], &rambanks[gRamBank*RAMSIZE], RAMSIZE);
            break;
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
set_rom0_bank:	This function sets the current ROM #0 bank for the PC-8300s. 
				The 8300 has a 128K rom with 4 selectable banks.
=============================================================================
*/
void set_rom0_bank(unsigned char bank)
{
	// Save the bank
	gRom0Bank = bank;

	// Now update the bank if needed
	if (gRomBank == 0)
		set_rom_bank(gRomBank);
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

	if (!(gReMem && !gRex))
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
				memcpy(gOptROM, gBaseMemory, OPTROMSIZE);

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
				if (gModel == MODEL_PC8201)
					memcpy(gBaseMemory, gSysROM, ROMSIZE);
				else
					memcpy(gBaseMemory, &gSysROM[gRom0Bank*32768], ROMSIZE);
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
amd_flash_write:	This function processes 8-bit write operatsions to the
					AMD flash and manages portions of the FLASH state machine
					for controlling Erase Read Only modes, etc.
=============================================================================
*/
int amd_flash_write(amd_flash_t *pFlash, unsigned int address, unsigned char data)
{
	int		sectOffset;
	int		sectSize, c;
	int		sector;
	int		retval = AMD_ACTION_RESET;
	char	*ptr;

	/* Look for Reset command.  If it's reset, set the FLASH to Read Only mode and clear BUSY */
	if ((data == FLASH_CMD_RESET) && (pFlash->iFlashState != FLASH_STATE_PROG))
	{
		pFlash->iFlashState = FLASH_STATE_RO;
		pFlash->iFlashTime = 0;
		pFlash->iFlashBusy = FALSE;
		return AMD_ACTION_RESET;
	}

	/* Switch on the current flash state and operate on data appropriatey */
	switch (pFlash->iFlashState)
	{
	case FLASH_STATE_RO:
		/* Proocess write as command */
		if ((data == FLASH_CMD_CFI_QUERY) && (address == 0xAA))
		{
			pFlash->iFlashState = FLASH_STATE_CFI_QUERY;
			pFlash->iFlashBusy = TRUE;
		}

		/* Test for transition to UNLK1 State */
		else if ((data == 0xAA) && ((address & 0xFF) == 0xAA))
			pFlash->iFlashState = FLASH_STATE_UNLK1;

		retval = AMD_ACTION_CMD;
		break;

	case FLASH_STATE_UNLK1:
		// Check for 2nd byte of unlock sequence
		if ((data == 0x55) && ((address & 0xFF) == 0x55))
			pFlash->iFlashState = FLASH_STATE_CMD;
		else
			pFlash->iFlashState = FLASH_STATE_RO;
		retval = AMD_ACTION_CMD;
		break;

	case FLASH_STATE_UNLK2:
		// Get 1st byte of 2nd Unlock
		if ((data == 0xAA) && ((address & 0xFF) == 0xAA))
			pFlash->iFlashState = FLASH_STATE_UNLK3;
		else
			pFlash->iFlashState = FLASH_STATE_RO;
		retval = AMD_ACTION_CMD;
		break;

	case FLASH_STATE_UNLK3:
		// Check for double unlock 2nd byte
		if ((data == 0x55) && ((address & 0xFF) == 0x55))
			pFlash->iFlashState = FLASH_STATE_CMD2;
		else
			pFlash->iFlashState = FLASH_STATE_RO;
		retval = AMD_ACTION_CMD;
		break;

	case FLASH_STATE_UNLK_BYPASS:
		/* We are in Unlock Bypass mode.  We only accept 0xA0 and 0x90 in this mode */
		/* Test for transition to program state */
		if (data == FLASH_CMD_PROG)
			pFlash->iFlashState = FLASH_STATE_UB_PROG;
		else if (data == FLASH_CMD_AUTOSELECT)
			pFlash->iFlashState = FLASH_STATE_UB_EXIT;
		retval = AMD_ACTION_CMD;
		break;

	case FLASH_STATE_UB_EXIT:
		/* Look for 2nd byte of exit sequence from Unlock Bypass */
		if (data == 0)
		{
			pFlash->iFlashState = FLASH_STATE_RO;
		}
		else
		{
			pFlash->iFlashState = FLASH_STATE_UNLK_BYPASS;
		}
		retval = AMD_ACTION_CMD;
		break;

	case FLASH_STATE_UB_PROG:
		/* Program in Unlock Bypass mode. Allow write and go back to UNLK_BYPASS state */
		pFlash->iFlashState = FLASH_STATE_UNLK_BYPASS;
		pFlash->iFlashBusy = TRUE;
		pFlash->iFlashTime = cycles + cycle_delta + FLASH_CYCLES_PROG;

		/* May need to add code here to write to the FLASH */
		if (pFlash->iFlashType == AMD_FLASH_TYPE_REX)
		{
			/* Write data to the flash */
			pFlash->pFlash[address] &= data;

			/* If trying to change a 1 to a 0, then go to BUSY until reset */
			if (pFlash->pFlash[address] != data)
				pFlash->iFlashBusy = TRUE;
		}

		retval = AMD_ACTION_ALLOW_WRITE;
		break;


	case FLASH_STATE_PROG:
		/* Change state back to Read Only and return code indicating write allowed */
		pFlash->iFlashState = FLASH_STATE_RO;
		pFlash->iFlashBusy = TRUE;
		pFlash->iFlashTime = cycles + cycle_delta + FLASH_CYCLES_PROG;

		/* May need to add code here to write to the FLASH */
		if (pFlash->iFlashType == AMD_FLASH_TYPE_REX)
		{
			/* Write data to the flash */
			pFlash->pFlash[address] &= data;

			/* If trying to change a 1 to a 0, then go to BUSY until reset */
			if (pFlash->pFlash[address] != data)
				pFlash->iFlashBusy = TRUE;
		}

		retval = AMD_ACTION_ALLOW_WRITE;
		break;

	case FLASH_STATE_CMD:
		if ((address & 0xFF) == 0xAA)
		{
			// Get the command
			if (data == FLASH_CMD_PROG)
				pFlash->iFlashState = FLASH_STATE_PROG;
			else if (data == FLASH_CMD_UNLK_BYPASS)
				pFlash->iFlashState = FLASH_STATE_UNLK_BYPASS;
			else if (data == FLASH_CMD_UNLK2)
				pFlash->iFlashState = FLASH_STATE_UNLK2;
			else if (data == FLASH_CMD_AUTOSELECT)
				pFlash->iFlashState = FLASH_STATE_AUTOSELECT;
			else
				pFlash->iFlashState = FLASH_STATE_RO;
		}
		else
		{
			pFlash->iFlashState = FLASH_STATE_RO;
			retval = AMD_ACTION_RESET;
			break;
		}
		retval = AMD_ACTION_CMD;
		break;

	case FLASH_STATE_CMD2:
		// Test for Erase Chip command
		if ((data == FLASH_CMD_CHIP_ERASE) && ((address & 0xFF) == 0xAA))
		{
			pFlash->iFlashState = FLASH_STATE_RO;
			pFlash->iFlashTime = cycles + cycle_delta + FLASH_CYCLES_CHIP_ERASE;
			pFlash->iFlashBusy = TRUE;

			/* Erase the Flash */
			ptr = pFlash->pFlash;
			for (c = 0; c < 2048 * 1024; c++)
				*ptr++ = 0xFF;

			retval = AMD_ACTION_ERASING;
			break;
		}

		/* Test if data is a Sect Erase command */
		else if (data != FLASH_CMD_SECT_ERASE)
		{
			pFlash->iFlashState = FLASH_STATE_RO;
			retval = AMD_ACTION_RESET;
			break;
		}

		/* Process Sector erase */
		sector = address >> 16;

		/* Calculate sector offset and size */
		if (sector == 0)
		{
			/* Test for sector zero */
			if ((address & 0xFFC000) == 0)
			{
				/* Sector zero is 16K */
				sectOffset = 0;
				sectSize = 16384;
			}
			else if ((address & 0xFF8000) != 0)
			{
				/* Sector 3 is a 32K sector */
				sectOffset = 0x8000;
				sectSize = 32768;
			}
			else
			{
				sectOffset = address & 0xFFF000;
				sectSize = 8192;
			}
		}
		else
		{
			/* Setup for 64K sector */
			sectOffset = address & 0xFF0000;
			sectSize = 0x10000;
		}

		/* Point to the correct memory space */
		ptr = pFlash->pFlash + sectOffset;

		/* Erase the sector */
		for (c = 0; c < sectSize; c++)
			*ptr++ = 0xFF;

		/* Update state and set busy flag */
		pFlash->iFlashState = FLASH_STATE_RO;
		pFlash->iFlashBusy = TRUE;
		pFlash->iFlashTime = cycles + cycle_delta + FLASH_CYCLES_SECT_ERASE;
		retval = AMD_ACTION_ERASING;
		break;

	case FLASH_STATE_CHIP_ERASE:
	case FLASH_STATE_SECT_ERASE:
		retval = AMD_ACTION_BUSY;

	default:
		pFlash->iFlashState = FLASH_STATE_RO;
		pFlash->iFlashTime = 0;
		pFlash->iFlashBusy = FALSE;
		retval = AMD_ACTION_RESET;
		break;
	}

	return retval;
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
	if ((gReMemMode & REMEM_MODE_NORMAL) && 
		((gReMemMap[block] & (REMEM_VCTR_FLASH1_CS | REMEM_VCTR_FLASH2_CS)) != 0x1800))
	{
		amd_flash_t		*pFlash;
		uchar			mask;
		int				iFlashAction;
		unsigned int	flashAddress;

		/* Writing to FLASH.  Do FLASH State machine processing */
		if (!(gReMemMap[block] & REMEM_VCTR_FLASH1_CS))
		{
			pFlash = &gReMemFlash1;
			mask = REMEM_MODE_FLASH1_RDY;
		}
		else
		{
			pFlash = &gReMemFlash2;
			mask = REMEM_MODE_FLASH2_RDY;
		}

		/* Call into the amd_flash_t object and issue the write.  Let the amd_flash_t
		   state manager take care of state transitions within the FALSH
		 */
		flashAddress = ((gReMemMap[block] & REMEM_VCTR_ADDRESS) << 10) + (address & 0x3FF);
		iFlashAction = amd_flash_write(pFlash, flashAddress, data);

		/* If the amd_flash indicates a write is allowd, or that an erase is in progress */
		/* then update our global ReMemMode variable to indicate FLASH not ready. */
		if ((iFlashAction == AMD_ACTION_ALLOW_WRITE) || (iFlashAction == AMD_ACTION_ERASING))
		{
			gReMemMode &= ~mask;

			/* Set our local FlashReady variable to FALSE so we know flash is busy */
			gReMemFlashReady = FALSE;
		}

		/* If writing to FLASH is allowed, then break to allow write to global memory */
		if (iFlashAction != AMD_ACTION_ALLOW_WRITE)
		{
			/* Writes to memory not allowed ... return */
			return;
		}
		gMemory[block][address & 0x3FF] = data;
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
	amd_flash_t		*pFlash;
	int				action;

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
		if (!gReMem)
			break;

		/* Test if old mode = new mode and do nothing if equal */
		if (data == (gReMemMode & 0x3F))
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
		if (!gReMem)
			break;

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
		if (!gReMem)
			break;

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
			pFlash = &gReMemFlash1;
		else
			pFlash = &gReMemFlash2;

		/* Simulate a write to address 0xAA */
		action = amd_flash_write(pFlash, 0xAA, data);

		if ((action == AMD_ACTION_ERASING) || (action == AMD_ACTION_ALLOW_WRITE))
		{
			if (port == REMEM_FLASH1_AAA_PORT)
			{
				gReMemMode &= ~REMEM_MODE_FLASH1_RDY;
			}
			else
			{
				gReMemMode &= ~REMEM_MODE_FLASH2_RDY;
			}

			/* Set our local FlashReady variable to FALSE so we know flash is busy */
			gReMemFlashReady = FALSE;
		}
		break;

	case REMEM_FLASH1_555_PORT:
	case REMEM_FLASH2_555_PORT:
		if (port == REMEM_FLASH1_555_PORT)
			pFlash = &gReMemFlash1;
		else
			pFlash = &gReMemFlash2;

		/* Write to the FLASH and let it manage the state machine */
		action = amd_flash_write(pFlash, 0x55, data);

		if ((action == AMD_ACTION_ERASING) || (action == AMD_ACTION_ALLOW_WRITE))
		{
			if (port == REMEM_FLASH1_AAA_PORT)
			{
				gReMemMode &= ~REMEM_MODE_FLASH1_RDY;
			}
			else
			{
				gReMemMode &= ~REMEM_MODE_FLASH2_RDY;
			}

			/* Set our local FlashReady variable to FALSE so we know flash is busy */
			gReMemFlashReady = FALSE;
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
		pSrc = (uchar *) (gReMemBoot ? gReMemFlash2.pFlash : gReMemFlash1.pFlash) + 0x8000 * gRomBank;
	else
		pSrc = (uchar *) (gReMemBoot ? gReMemFlash2.pFlash : gReMemFlash1.pFlash) + (gRomBank == 0 ? 0 : 
			0x10000 + 0x8000 * (gRomBank - 1));
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
		pSrc = (uchar *) gReMemFlash1.pFlash;
	else if ((gReMemMap[block] & REMEM_VCTR_FLASH2_CS) == 0)
		pSrc = (uchar *) gReMemFlash2.pFlash;
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
	amd_flash_t		*pFlash;
	unsigned char	mask;
	int				action;

	/* Calculate block number */
	block = address >> 10;

	/* Test if this is the flash that is not ready */
	if ((gReMemMap[block] & REMEM_VCTR_FLASH1_CS) == 0)
	{
		if (!gReMemFlash1.iFlashBusy && (gReMemFlash1.iFlashState == FLASH_STATE_RO))
			return gMemory[block][address & 0x3FF];

		/* Set variables for state machine processing */
		pFlash = &gReMemFlash1;
		mask = REMEM_MODE_FLASH1_RDY;
	}
	else
	{
		if (!gReMemFlash2.iFlashBusy && (gReMemFlash2.iFlashState == FLASH_STATE_RO))
			return gMemory[block][address & 0x3FF];

		/* Set variables for state machine processing */
		pFlash = &gReMemFlash2;
		mask = REMEM_MODE_FLASH2_RDY;
	}

	action = amd_flash_read_sm(pFlash, address);

	if (action == -1)
	{
		gReMemFlashReady = !gReMemFlash1.iFlashBusy | !gReMemFlash2.iFlashBusy;
		gReMemMode |= mask;
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
	if (amd_flash_proc_timer(&gReMemFlash1) == AMD_ACTION_DEVICE_READY)
	{
		/* Clear busy bit for Flash1 */
		gReMemMode |= REMEM_MODE_FLASH1_RDY;
	}

	/* Check if FLASH2 is busy */
	if (amd_flash_proc_timer(&gReMemFlash2) == AMD_ACTION_DEVICE_READY)
	{
		/* Clear busy bit for Flash1 */
		gReMemMode |= REMEM_MODE_FLASH2_RDY;
	}

	/* Check if either flash is busy */
	if (gReMemFlash1.iFlashBusy || gReMemFlash2.iFlashBusy)
		gReMemFlashReady = FALSE;
	else
		gReMemFlashReady = TRUE;
}

/*
=============================================================================
rex_read:	This routine processes reads during REX emulation mode.
=============================================================================
*/
unsigned char rex_read(unsigned short address)
{
	/* Reads from primary ROM are processed as usual */
	if ((gRomBank == 0) || ((gRomBank == 1) && (gModel == MODEL_T200)))
	{
		return gBaseMemory[address];
	}
	else
	{
		if (address & 0x8000)
			return (address>ROMSIZE&&address<RAMBOTTOM)? 0xFF : gBaseMemory[address];
	}

	// If the RexFlash is busy, we need to process the timer
	if (gRexFlash.iFlashTime > 0)
		amd_flash_proc_timer(&gRexFlash);

	// Process read as a REX State machine read
	switch (gRexState)
	{
	case 7:				/* Read State */
		if ((address & 0xFF) == gRexKeyTable[gRexKeyState])
		{
			if (++gRexKeyState == 6)
			{
				/* Key sequence detected - switch to state zero */
				gRexState = 0;
			}
		}
		else
			/* Key state not detected - clear keystate */
			gRexKeyState =  0;
		return gRexFlash.pFlash[gRexSector | address];

	case 0:				/* Command mode */
		switch (address & 0x07)
		{
		case 3:		/* Read Status CMD */
			gRexReturn =(gRexFlash.iFlashBusy ? 0 : 0x80) |
						(gRexFlashSel ? 0x40 : 0) |
						(gRexSector >> 15);
			gRexState = address & 0x07;
			return gRexReturn;

		case 2:		/* Send AAA,AA CMD */
		case 5:		/* Send 555,55 CMD */
		case 6:		/* Send PA, PD */
			/* For REX2 (PC8201), the state machine had to change because of no ALE.
			   In this case, we transition to state 2 to await the data, then to state
			   6 to await the address. */
			if (gModel == MODEL_PC8201)
			{
				// Go to state 2
				gRexState = 2;
				gRex3Cmd = address & 0x07;
				break;
			}

			// Fall through

		case 1:		/* Set Sector CMD */
		case 4:		/* Read from flash */
			gRexState = address & 0x07;
			gRexFlashPA = address & 0xFF;
			break;

		case 7:		/* Read FW/HW ID */
			gRexReturn = (gRex == REX2 ? 0x40 : 0) |
						 (0x10) | gRexModel;
			gRexState = 3;
			return gRexReturn;

		default:
			gRexState = 7;
			break;
		}
		break;

	case 1:		/* Set Sector state */
		/* Test if RAM mode is enabled and set sector based on result */
//		if (gModel && REX2_RAM_MODE)
//			gRexSector = (address & 0x3F) << 15;
//		else
			gRexSector = (address & 0x1F) << 15;

		/* Save Flash Select bit */
		gRexFlashSel = address & 0x40 ? 0 : 1;

		/* Back to state 0 for next command */
		gRexState = 0;
		break;

	case 2:		/* Send AAA, AD to Flash */
		if (gModel == MODEL_PC8201)
		{
			/* For REX3 (PC8201), we save the write data during state 2, then go to 
			   state 6 to await the address */
			gRex3Data = address & 0xFF;
			gRexState = 6;
		}
		else
		{
			/* Write to the REX Flash object.  LSB of address is actually data */
			amd_flash_write(&gRexFlash, gRexSector | gRexFlashPA, address & 0xFF);

			/* Back to state 0 for next command */
			gRexState = 0;
		}
		break;

	case 3:
		/* Switch back to state 0 */
		gRexState = 0;
		return gRexReturn;

	case 4:		/* Read from flash */
		/* Test if reading from RAM sector */
		gRexState = 8;		/* Holding state */

		/* Read from either RAM for Flash */
		if  (gRexSector & 0x100000)
		{
			/* Read from RAM if REX2 */
			if (gRex == REX2)
				gRexReturn = gRex2Ram[(gRexSector & 0x18000) | (address & 0x7FFF)];
			else
				gRexReturn = 0xFF;
		}
		else
		{
			/* Read from Flash if enabled */
			if (gRexFlashSel)
				gRexReturn = gRexFlash.pFlash[gRexSector | (address & 0x7FFF)];
			else
				gRexReturn = 0xFF;
		}
		return gRexReturn;

	case 8:		/* Temp holding state for state 4 */
		gRexState = 0;		/* Back to state 0 */
		return gRexReturn;

	case 5:		/* Send 555,data to flash */
		gRexState = 0;		/* Back to state 0 */

		/* Send 555,data to flash */
		amd_flash_write(&gRexFlash, 0x55, 0x55);

		break;

	case 6:		/* Send PA,PD to flash, first we receive PA */
		/* Test for REX3 (PC8201) */
		if (gModel == MODEL_PC8201)
		{
			/* For REX3, we perform the write during state 6 */
			switch (gRex3Cmd)
			{
				case 2:
					gRexFlashPA = gRexSector | (address & 0xFF);
					break;
				case 5:
					gRexFlashPA = address & 0xFF;
					break;
				case 6:
					gRexFlashPA = gRexSector | (address & 0x7FFF);
					break;
				default:
					gRexFlashPA = 0;
					break;
			}

			/* Write to the REX Flash object.  LSB of address is actually data */
			amd_flash_write(&gRexFlash, gRexFlashPA, gRex3Data);

			/* Back to state 0 for next command */
			gRexState = 0;
		}
		else
		{
			/* Save the address from this read as our Flash PA */
			gRexFlashPA = address & 0x7FFF;

			/* Go to State 2 to await the data */
			gRexState = 2;
		}

		break;
	}
	return (unsigned char) (address & 0xFF);
}

/*
=============================================================================
rex_set8:	This routine processes writes during REX emulation mode.
=============================================================================
*/
void rex_set8(unsigned short address, unsigned char val)
{
	if (address >= ROMSIZE)
	{
		if (address>=RAMBOTTOM)
			gBaseMemory[address] = val;
	}
	else
	{
		if (gRex == REX2)
		{
			/* Add logic here to write to the selected RAM sector */
			gBaseMemory[address] = val;
		}
	}
}

