/*
============================================================================
memory.h

  This file is part of the Virtual T Model 100/102/200 Emulator.

  This file is copywrited under the terms of the BSD license.

  Memory management / emulation support
============================================================================
*/

#ifndef MEMORY_H
#define MEMORY_H

#include "gen_defs.h"

#ifdef __cplusplus
extern "C" {
#endif

#define	REMEM_REV_MAJOR				4

#define	REMEM_REV_M100				(REMEM_REV_MAJOR << 4)
#define	REMEM_REV_M102				((REMEM_REV_MAJOR << 4) | 0x01)
#define	REMEM_REV_T200				((REMEM_REV_MAJOR << 4) | 0x02)

#define	REMEM_REV					((gModel == MODEL_M100) || (gModel == MODEL_KC85) ? REMEM_REV_M100 : gModel == MODEL_M102 ? REMEM_REV_M102 : REMEM_REV_T200)

#define	REMEM_MODE_NORMAL			0x01
#define	REMEM_MODE_RAMPAC			0x02
#define	REMEM_MODE_IOENABLE			0x04
#define	REMEM_MODE_MAP_BITS			0x38
#define	REMEM_MODE_MAP_SHIFT		3
#define	REMEM_MODE_FLASH1_RDY		0x40
#define	REMEM_MODE_FLASH2_RDY		0x80

#define	REMEM_SECTOR_PORT			0x61
#define	REMEM_DATA_PORT				0x63
#define	REMEM_MODE_PORT				0x70
#define	REMEM_REVID_PORT			0x71
#define	REMEM_FLASH1_AAA_PORT		0x74
#define	REMEM_FLASH1_555_PORT		0x75
#define	REMEM_FLASH2_AAA_PORT		0x76
#define	REMEM_FLASH2_555_PORT		0x77
#define QUAD_BANK_PORT              0x80
#define	RAMPAC_SECTOR_PORT			0x81
#define	RAMPAC_DATA_PORT			0x83

#define	REMEM_VCTR_RAM_CS			0x2000
#define	REMEM_VCTR_FLASH2_CS		0x1000
#define	REMEM_VCTR_FLASH1_CS		0x0800
#define	REMEM_VCTR_READ_ONLY		0x4000
#define	REMEM_VCTR_MMU_ACTIVE		0x8000
#define	REMEM_VCTR_ADDRESS			0x07FF

#define	REMEM_MAP_OFFSET			0x1BF000
#define	REMEM_MAP_SIZE				512
#define	REMEM_SECTOR_SIZE			128
#define	REMEM_SECTORS_PER_MAP		4
#define	REMEM_TOTAL_SECTORS			32
#define	REMEM_BLOCKS_PER_BANK		(gModel == MODEL_T200 ? 40 : 32)
#define	REMEM_RAM_MAP_OFFSET		(gModel == MODEL_T200 ? 0x50 : 0)
#define REMEM_RAMPAC_OFFSET			0x1C0000
#define	RAMPAC_SECTOR_SIZE			1024
#define	RAMPAC_SECTOR_COUNT			256

#define	FLASH_STATE_RO				0
#define FLASH_STATE_UNLK1			1
#define FLASH_STATE_CMD				2
#define FLASH_STATE_UNLK2			3
#define FLASH_STATE_PROG			4
#define FLASH_STATE_SECT_ERASE		5
#define FLASH_STATE_CHIP_ERASE		6
#define FLASH_STATE_UNLK_BYPASS		7
#define FLASH_STATE_UB_PROG			8
#define FLASH_STATE_UNLK3			9
#define FLASH_STATE_CMD2			10
#define FLASH_STATE_CFI_QUERY		11
#define FLASH_STATE_AUTOSELECT		12
#define	FLASH_STATE_UB_EXIT			13

#define	FLASH_CMD_PROG				0xA0
#define	FLASH_CMD_UNLK_BYPASS		0x20
#define FLASH_CMD_UNLK2				0x80
#define FLASH_CMD_SECT_ERASE		0x30
#define FLASH_CMD_CHIP_ERASE		0x10
#define FLASH_CMD_AUTOSELECT		0x90
#define FLASH_CMD_RESET				0xF0
#define FLASH_CMD_CFI_QUERY			0x98

#define FLASH_CYCLES_PROG			40
#define FLASH_CYCLES_SECT_ERASE		1680000
#define	FLASH_CYCLES_CHIP_ERASE		19000000
#define	FLASH_MANUF_ID				1
#define	FLASH_PRODUCT_ID			0x49

#define	REGION_RAM					0
#define	REGION_ROM					1
#define	REGION_ROM2					2
#define	REGION_OPTROM				3
#define	REGION_RAM1					4
#define	REGION_RAM2					5
#define	REGION_RAM3					6
#define	REGION_REMEM_RAM			7
#define	REGION_FLASH1				8
#define	REGION_FLASH2				9
#define	REGION_RAMPAC				10
#define	REGION_REX_FLASH			11
#define	REGION_REX2_RAM				12
#define	REGION_ROM3					13
#define	REGION_ROM4					14
#define	REGION_MAX					15
#define REGION_RAM4                 16

#define	REX							1
#define	REX2						2

#define	REX_ROM_REPLACEMENT			0x01
#define	REX2_RAM_MODE				0x02


typedef	struct {
	int				iFlashState;
	UINT64			iFlashTime;
	int				iFlashBusy;
	char*			pFlash;
	int				iFlashType;
	int				iFlashSize;
} amd_flash_t;	

#define		AMD_ACTION_RESET		0
#define		AMD_ACTION_ALLOW_WRITE	1
#define		AMD_ACTION_CMD			2
#define		AMD_ACTION_ERASING		3
#define		AMD_ACTION_DEVICE_READY	4
#define		AMD_ACTION_BUSY			5

#define		AMD_FLASH_TYPE_REMEM	1
#define		AMD_FLASH_TYPE_REX		2

extern unsigned char	*gMemory[64];
extern unsigned char	gSysROM[4*32768];
extern unsigned char	gOptROM[32768];
extern unsigned char	gBaseMemory[65536];
extern unsigned char	gReMem;
extern int				gRex;

void			init_mem(void);
void			reinit_mem(void);
void			cold_boot_mem(void);
void			free_mem(void);
void			free_remem_mem(void);
void			free_rampac_mem(void);
void			load_rampac_ram(void);
void			load_remem_ram(void);
void			save_rampac_ram(void);
void			save_remem_ram(void);
void			reload_sys_rom(void);
void			save_rex2_ram(void);
void			save_rex_flash(void);
void			load_rex2_ram(void);
void			load_rex_flash(void);

unsigned char	get_memory8(unsigned short address);
unsigned short	get_memory16(unsigned short address);
void			set_memory8(unsigned short address, unsigned char data);
void			set_memory16(unsigned short address, unsigned short data);

void			get_memory8_ext(int region, long address, int count, unsigned char *data);
void			set_memory8_ext(int region, long address, int count, unsigned char *data); 

void			remem_set8(unsigned int address, unsigned char data);
int				remem_out(unsigned char port, unsigned char data);
int				remem_in(unsigned char port, unsigned char *data);
void			remem_update_vectors(unsigned char targetMode);
void			remem_copy_normal_to_system(void);
void			remem_copy_mmu_to_system(void);
void			remem_copy_system_to_normal(void);
void			remem_copy_system_to_mmu(void);
void			remem_copy_mmu_to_block(int block);
void			remem_copy_block_to_mmu(int block);
unsigned char	remem_flash_sm_read(unsigned short address);
void			remem_flash_proc_timer(void);
void			patch_vt_version(char* pMem, int size);
unsigned char	rex_read(unsigned short address);
void			rex_set8(unsigned short address, unsigned char val);

void			save_ram(void);
void			load_ram(void);
void			load_sys_rom(void);
void			load_opt_rom(void);
void			set_ram_bank(unsigned char page);
void			set_rom_bank(unsigned char bank);
void			set_rom0_bank(unsigned char bank);
unsigned char	get_ram_bank(void);
unsigned char	get_rom_bank(void);


#ifdef __cplusplus
}
#endif

#endif
