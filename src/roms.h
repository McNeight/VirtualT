/* roms.h */

/* $Id: roms.h,v 1.5 2008/03/01 15:41:11 kpettit1 Exp $ */

/*
 * Copyright 2004 Stephen Hurd and Ken Pettit
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


#ifndef ROMS_H
#define ROMS_H

/* Define structure for storing addresses and descriptions for */
/* known subroutine calls and variable locations within the ROM	*/
typedef struct Std_ROM_Addresses {
	int	        addr;
	int			strnum;
} Std_ROM_Addresses_t;

/* Define structure for storing known table data stored withing the ROM */
typedef struct Std_ROM_Table {
	int addr;
	int size;
	char type;
} Std_ROM_Table_t;

/* Define structure for storing addresses and descriptions for */
/* known subroutine calls and variable locations within the ROM	*/
typedef struct Std_ROM_Strings {
	int	        addr;
	char*		desc;
} Std_ROM_Strings_t;

/* Define a structure to contain all information regarding a ROM */
typedef struct RomDescription {
	unsigned long       dwSignature;       /* Unique 32bit value in ROM */
	unsigned short      sSignatureAddr;    /* Address of signature */
	unsigned short      sStdRom;           /* Indicates it this is a standard ROM */

	Std_ROM_Table_t		*pTables;          /* Pointer to known tables */
	Std_ROM_Addresses_t *pVars;            /* Pointer to known variables */
	Std_ROM_Addresses_t	*pFuns;            /* Pointer ot known functions */

	unsigned short      sFilePtrBA;        /* Address of unsaved BASIC prgm */
	unsigned short      sFilePtrDO;        /* Address of next DO file */
	unsigned short      sBeginDO;          /* Start of DO file area */
	unsigned short      sBeginCO;          /* Start of CO file area */
	unsigned short      sBeginVar;         /* Start of Variable space */
	unsigned short      sBeginArray;       /* Start of Array data */
	unsigned short      sUnusedMem;        /* Pointer to unused memory */
	unsigned short      sHimem;            /* Address where HIMEM is stored	*/
	unsigned short      sRamEnd;           /* End of RAM for file storage */
	unsigned short      sLowRam;           /* Lowest RAM used by system */
	unsigned short      sDirectory;        /* Start of RAM directory */
	unsigned short      sBasicStrings;     /* BASIC string buffer pointer */
	unsigned short		sBasicSize;	       /* Size of all BASIC programs */
	unsigned short		sKeyscan;	       /* Location of Keyscan array */
	unsigned short		sCharTable;        /* Location of Charater generator table */
	unsigned short		sYear;             /* Location of Year storage */
	unsigned short		sLcdBuf;           /* Lccation of LCD buffer */
	unsigned short		sLabelEn;		   /* Label line enable flag */

	unsigned short		sDirCount;         /* Number of entries in Directory */
	unsigned short		sFirstDirEntry;    /* Index of first available enry */
	unsigned short		sMSCopyright;	   /* Address of MS Copyright string on MENU */
} RomDescription_t;

enum {
	 TABLE_TYPE_STRING,
	 TABLE_TYPE_JUMP,
	 TABLE_TYPE_CODE,
	 TABLE_TYPE_MODIFIED_STRING,
	 TABLE_TYPE_MODIFIED_STRING2,
	 TABLE_TYPE_MODIFIED_STRING3,
	 TABLE_TYPE_2BYTE,
	 TABLE_TYPE_3BYTE,
	 TABLE_TYPE_CTRL_DELIM,
	 TABLE_TYPE_BYTE_LOOKUP,
	 TABLE_TYPE_4BYTE_CMD,
	 TABLE_TYPE_CATALOG
};

#endif
