/*
============================================================================
memory.c

  This file is part of the Virtual T Model 100/102/200 Emulator.

  This file is copywrited under the terms of the BSD license.

  Memory management / emulation support
============================================================================
*/

#include "memory.h"
#include "genwrap.h"
#include "filewrap.h"

extern uchar	memory[65536];
extern uchar	sysROM[32768];
extern uchar	optROM[32768];


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
