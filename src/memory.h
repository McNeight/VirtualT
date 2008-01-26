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

#ifdef __cplusplus
extern "C" {
#endif

#define	REMEM_REV_MAJOR				4

#define	REMEM_REV_M100				(REMEM_REV_MAJOR << 4)
#define	REMEM_REV_M102				((REMEM_REV_MAJOR << 4) | 0x01)
#define	REMEM_REV_T200				((REMEM_REV_MAJOR << 4) | 0x02)

#define	REMEM_REV					(gModel == MODEL_M100 ? REMEM_REV_M100 : gModel == MODEL_M102 ? REMEM_REV_M102 : REMEM_REV_T200)

#define	REMEM_MODE_NORMAL			0x01
#define	REMEM_MODE_RAMPAC			0x02
#define	REMEM_MODE_IOENABLE			0x04
#define	REMEM_MODE_MAP_BITS			0x38
#define	REMEM_MODE_MAP_SHIFT		3
#define	REMEM_MODE_FLASH1_RO		0x40
#define	REMEM_MODE_FLASH2_RO		0x80

#define	REMEM_SECTOR_PORT			0x61
#define	REMEM_DATA_PORT				0x63
#define	REMEM_MODE_PORT				0x70
#define	REMEM_REVID_PORT			0x71
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


extern unsigned char	*gMemory[64];
extern unsigned char	gSysROM[65536];
extern unsigned char	gBaseMemory[65536];
extern unsigned char	gReMem;

void			init_mem(void);
void			reinit_mem(void);
void			free_mem(void);
void			free_remem_mem(void);
void			free_rampac_mem(void);
void			load_rampac_ram(void);
void			load_remem_ram(void);
void			save_rampac_ram(void);
void			save_remem_ram(void);
void			reload_sys_rom(void);

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

void			save_ram(void);
void			load_ram(void);
void			load_sys_rom(void);
void			load_opt_rom(void);
void			set_ram_bank(unsigned char page);
void			set_rom_bank(unsigned char bank);
unsigned char	get_ram_bank(void);
unsigned char	get_rom_bank(void);


#ifdef __cplusplus
}
#endif

#endif
