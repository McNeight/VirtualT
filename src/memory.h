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

unsigned char get_memory8(unsigned short address);
unsigned short get_memory16(unsigned short address);
void set_memory8(unsigned short address, unsigned char data);
void set_memory16(unsigned short address, unsigned short data);

#endif
