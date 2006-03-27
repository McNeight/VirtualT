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

#define	REMEM_MODE_NORMAL			0x01
#define	REMEM_MODE_RAMPAC			0x02
#define	REMEM_MODE_MAP_BITS			0x1C
#define	REMEM_MODE_MAP_SHIFT		2
#define	REMEM_MODE_FLASH1_RO		0x20
#define	REMEM_MODE_FLASH2_RO		0x40

#define	REMEM_MODE_PORT				0x70
#define	REMEM_SECTOR_PORT			0x71
#define	REMEM_VCTR_PORT				0x73
#define	RAMPAC_SECTOR_PORT			0x81
#define	RAMPAC_DATA_PORT			0x83

#define	REMEM_VCTR_RAM_CS			0x2000
#define	REMEM_VCTR_FLASH1_CS		0x1000
#define	REMEM_VCTR_FLASH2_CS		0x0800
#define	REMEM_VCTR_READ_ONLY		0x4000
#define	REMEM_VCTR_MMU_ACTIVE		0x8000
#define	REMEM_VCTR_ADDRESS			0x07FF

#define	REMEM_MAP_OFFSET			0x00100000000
#define	REMEM_MAP_SIZE				(gModel == MODEL_T200 ? 1024 : 512)
#define	REMEM_BANK_SIZE				(gModel == MODEL_T200 ? 256 : 128)
#define	REMEM_BANKS_PER_MAP			(gModel == MODEL_T200 ? 8 : 4)
#define	REMEM_RAM_BANK_OFFSET		0xA0

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

void			init_mem(void);
void			free_mem(void);
void			free_remem_mem(void);
void			free_rampac_mem(void);
void			load_rampac_ram(void);
void			load_remem_ram(void);
void			save_rampac_ram(void);
void			save_remem_ram(void);

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
void			load_opt_rom(void);
void			set_ram_bank(unsigned char page);
void			set_rom_bank(unsigned char bank);


#ifdef __cplusplus
}
#endif

#endif
