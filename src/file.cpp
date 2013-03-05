/* file.cpp */

/* $Id: file.cpp,v 1.18 2013/02/08 00:07:52 kpettit1 Exp $ */

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


#include <sys/types.h>
#include <ctype.h>
#include <stdio.h>
#ifdef _WIN32
#include <windows.h>
#endif

#include <FL/Fl.H>
#include <FL/Fl_Help_Dialog.H>
#include <FL/fl_ask.H> 
#include <FL/Fl_File_Chooser.H>
#include <FL/Fl_Hold_Browser.H>

#include "FLU/Flu_File_Chooser.h"

extern "C"
{
#include "memory.h"
#include "roms.h"
#include "intelhex.h"
#include "m100emu.h"

extern RomDescription_t *gStdRomDesc;
void jump_to_zero(void);
}

#include "file.h"
#include "fileview.h"

int		BasicSaveMode = 0;
int		COSaveMode = 0;
int 	gLoadError;				// Indicates load error (for remote interface)
extern	Fl_Preferences virtualt_prefs;
extern	char gsMenuROM[40];

void cb_LoadRam (Fl_Widget* w, void*)
{
	Flu_File_Chooser *FileWin;

	fl_cursor(FL_CURSOR_WAIT);
#ifdef __APPLE__	
	FileWin = new Flu_File_Chooser(path,"*.bin",1,"Load RAM file");
#else
	FileWin = new Flu_File_Chooser(".","*.bin",1,"Load RAM file");
#endif
	fl_cursor(FL_CURSOR_DEFAULT);
	FileWin->preview(0);
	FileWin->show();
	
}

void cb_SaveRam (Fl_Widget* w, void*)
{
	Flu_File_Chooser *FileWin;
	
	fl_cursor(FL_CURSOR_WAIT);
#ifdef __APPLE__
	FileWin = new Flu_File_Chooser(path,"*.bin",1,"Save RAM file");
#else
	FileWin = new Flu_File_Chooser(".","*.bin",1,"Save RAM file");
#endif
	fl_cursor(FL_CURSOR_DEFAULT);
	FileWin->preview(0);
	// FileWin.value(0)="RAM.bin"
	FileWin->show();
	
}
/*
================================================================
Load Option ROM using the specified filename.
================================================================
*/
int load_optrom_file(const char* filename)
{
	int					len;
	char				buffer[65536];
	char				option_name[32];
	unsigned short		start_addr;
	FILE*				fd;

	// Try to open file
	if ((fd = fopen(filename, "r")) == NULL)
	{
		return FILE_ERROR_FILE_NOT_FOUND;
	}
	fclose(fd);

	// Check for hex file
	len = strlen(filename);
	if (((filename[len-1] | 0x20) == 'x') &&
		((filename[len-2] | 0x20) == 'e') &&
		((filename[len=3] | 0x20) == 'h'))
	{
		// Check if hex file format is valid
		len = load_hex_file((char *) filename, buffer, &start_addr);

		if (len == 0)
		{
			return FILE_ERROR_INVALID_HEX;
		}
	}

	// Save the selected file in the preferences
	get_model_string(option_name, gModel);
	strcat(option_name, "_OptRomFile");
    virtualt_prefs.set(option_name, filename);

	strcpy(gsOptRomFile, filename);

	// Update menu
	strcpy(gsMenuROM, fl_filename_name(gsOptRomFile));

	load_opt_rom();

	return 0;
}

void cb_LoadOptRom (Fl_Widget* w, void*)
{
	Flu_File_Chooser		*FileWin;
	int					count;
	int					ret;
	const char			*filename;
	
	fl_cursor(FL_CURSOR_WAIT);
#ifdef __APPLE__
	FileWin = new Flu_File_Chooser(".","*.{bin,hex}",1,"Load Optional ROM file");
#else
	FileWin = new Flu_File_Chooser(".","*.{bin,hex}",1,"Load Optional ROM file");
#endif
	fl_cursor(FL_CURSOR_DEFAULT);
	FileWin->preview(0);
	FileWin->show();

	while (FileWin->visible())
		Fl::wait();

	count = FileWin->count();
	if (count == 0)
	{
		delete FileWin;
		return;
	}

	// Get Filename
	filename = FileWin->value();
	if (filename == 0)
	{
		delete FileWin;
		return;
	}

	ret = load_optrom_file((char*) filename);

	if (ret == FILE_ERROR_INVALID_HEX)
	{
		fl_message("HEX file format invalid!");
		return;
	}

	jump_to_zero();
}


const char			*gIllformedBasic = "Ill formed BASIC file";
static const char		*gTooLargeMsg = "File too large for available memory";

const char *gKeywordTable[] = {
	"END",    "FOR",   "NEXT",  "DATA",    "INPUT", "DIM",    "READ",   "LET",
	"GOTO",	  "RUN",   "IF",    "RESTORE", "GOSUB", "RETURN", "REM",    "STOP",
	"WIDTH",  "ELSE",  "LINE",  "EDIT",    "ERROR", "RESUME", "OUT",    "ON",
	"DSKO$",  "OPEN",  "CLOSE", "LOAD",    "MERGE", "FILES",  "SAVE",   "LFILES",
	"LPRINT", "DEF",   "POKE",	"PRINT",   "CONT",  "LIST",   "LLIST",  "CLEAR",
	"CLOAD",  "CSAVE", "TIME$", "DATE$",   "DAY$",  "COM",    "MDM",    "KEY",
	"CLS",    "BEEP",  "SOUND", "LCOPY",   "PSET",  "PRESET", "MOTOR",  "MAX",
	"POWER",  "CALL",  "MENU",  "IPL",     "NAME",  "KILL",   "SCREEN", "NEW",

// ==========================================
// Function keyword table TAB to <
// ==========================================
	"TAB(",   "TO",    "USING", "VARPTR",  "ERL",   "ERR",    "STRING$","INSTR",
	"DSKI$",  "INKEY$","CSRLIN","OFF",     "HIMEM", "THEN",   "NOT",    "STEP",
	"+",      "-",     "*",     "/",       "^",     "AND",    "OR",     "XOR",
	"EQV",    "IMP",   "MOD",   "\\",      ">",     "=",      "<",

// ==========================================
// Function keyword table SGN to MID$
// ==========================================
	"SGN",    "INT",   "ABS",   "FRE",     "INP",   "LPOS",   "POS",    "SQR",
	"RND",    "LOG",   "EXP",   "COS",     "SIN",   "TAN",    "ATN",    "PEEK",
	"EOF",    "LOC",   "LOF",   "CINT",    "CSNG",  "CDBL",   "FIX",    "LEN",
	"STR$",   "VAL",   "ASC",   "CHR$",    "SPACE$","LEFT$",  "RIGHT$", "MID$",

// End of table marker
	""
};

/*
=======================================================
This routine relocates BASIC files in the emulation by 
updating all the start of line pointers.
=======================================================
*/
int relocate(unsigned char* in, unsigned char* out, unsigned short addr)
{
	int				c;
	unsigned short	next_line;
	unsigned short	this_line;
	int				line_number;
	int				line_len;
	unsigned short	addr1;
	unsigned short	base;

	next_line = 1;;
	this_line = 0;
	c = 0;
	line_number = -1;

	// Get pointer to next line
	next_line = ((unsigned short) in[c+1] << 8) + (unsigned short) in[c];
	line_len =0;
	base = in[0] + (in[1] << 8);
	while (next_line != 0)
	{
		c += 2;				// Skip next line pointer
		line_len += 2;		// Update BASIC line length

		if (in[c] + (in[c+1] << 8) <= line_number)
		{
			fl_message("%s", gIllformedBasic);
			return 0;
		}

		line_number = in[c] + (in[c+1] << 8);

		out[c] = in[c];		// Copy low byte of line number
		c++;
		line_len++;
		out[c] = in[c];		// Copy high byte of line number
		c++;
		line_len++;

		// Copy tokenized line
		while (in[c] != 0)
		{
			out[c] = in[c];
			c++;
			line_len++;
		}

		out[c] = in[c];		// Copy terminating zero
		c++;
		line_len++;

		// Update pointer to next line (of line just copied)
		out[this_line] = (addr + c) & 0xFF;
		out[this_line+1] = (addr + c) >> 8;

		// Validate the line pointers are valid
		if (this_line != 0)
		{
			addr1 = in[this_line] + (in[this_line+1] << 8);
			if (addr1 != base + line_len)
			{
				fl_message("%s", gIllformedBasic);
				return 0;
			}
		}

		// Update location where next pointer will be stored
		base = in[this_line] + (in[this_line+1] << 8);
		this_line = c;

		// Get pointer to next line
		next_line = ((unsigned short) in[c+1] << 8) + (unsigned short) in[c];
		line_len = 0;
	}

	// Copy terminating 0x0000 to out
	out[c++] = 0;
	out[c++] = 0;

	return c;
}

/*
=======================================================
This is the tokenizer that converts ASCII mode BASIC files
to actual .BA files in the Model 100/102/200 format.
=======================================================
*/
#define	STATE_LINENUM	1
#define	STATE_WHITE		2
#define	STATE_TOKENIZE	3
#define	STATE_CR		4

int tokenize(unsigned char* in, unsigned char* out, unsigned short addr)
{
	int				state;
	int				c, x;
	unsigned short	line_num;
	int				line_num_len;
	unsigned char	tok_line[256];
	int				line_len;
	unsigned char	token;
	int				tok_len;
	int				out_len;
	int				last_line_num;
	int				next_line_offset;

	c = 0;
	line_num = 0;
	line_num_len = 0;
	line_len = 0;
	out_len = 0;
	last_line_num = -1;
	next_line_offset = 0;
	state = STATE_LINENUM;

	// Initialize out array with a blank line
	out[0] = 0;
	out[1] = 0;

	// Parse until end of string
	while ((in[c] != 0) && (in[c] != 0x1A))
	{
		switch (state)
		{
		// Read linenumber until non numeric
		case STATE_LINENUM:
			if ((in[c] < '0') || (in[c] > '9'))
			{
				// Check for blank lines
				if ((in[c] == 0x0d) || (in[c] == 0x0a))
				{
					// Skip this character
					c++;
				}
				// Check for characters with no line number
				else if (line_num_len == 0)
				{
					fl_message("%s", gIllformedBasic);
					return 0;
				}

				// First space not added to output - skip it
				if (in[c] == ' ')
					c++;

				// Line number found, go to tokenize state
				state = STATE_TOKENIZE;
			}
			else
			{
				// Add byte to line number
				line_num = line_num * 10 + in[c] - '0';
				line_num_len++;
				c++;
			}
			break;

		case STATE_TOKENIZE:
			// Check for '?' and assign to PRINT
			if (in[c] == '?')
			{
				tok_line[line_len++] = 0xA3;
				c++;
			}
			// Check for REM tick
			else if (in[c] == '\'')
			{
				tok_line[line_len++] = ':';
				tok_line[line_len++] = 14 | 0x80;
				c++;
				tok_line[line_len++] = 0xFF;

				// Copy bytes to token line until 0x0D or 0x0A
				while ((in[c] != 0x0d) && (in[c] != 0x0a)
					&& (in[c] != 0))
				{
					tok_line[line_len++] = in[c++];
				}
			}
			// Check for characters greater than 127 and skip
			else if ((in[c] > 127) || (in[c] == 0x0d))
			{
				c++;
			}
			// Check for quote
			else if (in[c] == '"')
			{
				tok_line[line_len++] = in[c++];
				// Copy bytes to token line until quote ended
				while ((in[c] != '"') && (in[c] != 0x0d) && (in[c] != 0x0a)
					&& (in[c] != 0))
				{
					tok_line[line_len++] = in[c++];
				}

				// Add trailing quote to tok_line
				if (in[c] == '"')
					tok_line[line_len++] = in[c++];
			}
			// Check for end of line
			else if (in[c] == 0x0a)
			{
				// Skip this byte - don't add it to the line
				c++;

				if (line_len != 0)
				{
					// Add line to output
					if (line_num > last_line_num)
					{
						// Add line to end of output
						out[next_line_offset] = (addr + next_line_offset + 5 + line_len) & 0xFF;
						out[next_line_offset] = (addr + next_line_offset + 5 + line_len) >> 8;
						next_line_offset += 2;

						// Add line number
						out[next_line_offset++] = line_num & 0xFF;
						out[next_line_offset++] = line_num >> 8;

						// Add tokenized line
						for (x = 0; x < line_len; x++)
							out[next_line_offset++] = tok_line[x];

						// Add terminating zero
						out[next_line_offset++] = 0;

						// Add 0x0000 as next line number offset
						out[next_line_offset] = 0;
						out[next_line_offset+1] = 0;

						// Update out_len to reflect current length
						out_len += line_len + 5;

						// Update last_line_num
						last_line_num = line_num;

					}
					else
					{
						// Add line somewhere in the middle

						// NEED TO ADD CODE HERE!!!!!
					}
				}
				state = STATE_LINENUM;
				line_len = 0;
				line_num = 0;
			}
			// Add '0' through ';' to tokenized line "as-is"
			else if ((in[c] >= '0') && (in[c] <= ';'))
			{
				tok_line[line_len++] = in[c++];
			}
			else
			{
				// Search token table for match
				for (token = 0; strlen(gKeywordTable[token]) != 0; token++)
				{
					tok_len = strlen(gKeywordTable[token]);
					// Compare next Keyword with input
					if (strncmp(gKeywordTable[token], (char *) &in[c], 
						tok_len) == 0)
					{
						// Add ':' prior to ELSE if not already there
						if ((token == 17) && (tok_line[line_len-1] != ':'))
						{
							tok_line[line_len++] = ':';
						}
						tok_line[line_len++] = token + 0x80;
						c += tok_len;

						// If REM token, add bytes to end of line 
						if (token == 14)
						{
							// Copy bytes to token line until 0x0D or 0x0A
							while ((in[c] != 0x0d) && (in[c] != 0x0a)
								&& (in[c] != 0))
							{
								tok_line[line_len++] = in[c++];
							}
						}

						// If DATA token, add bytes to EOL or ':'
						if (token == 3)
						{
							// Copy bytes to token line until 0x0D or 0x0A
							while ((in[c] != ':') && (in[c] != 0x0d) && (in[c] != 0x0a)
								&& (in[c] != 0))
							{
								tok_line[line_len++] = in[c++];
							}
						}
						break;
					}
				}

				// Check if token not found - add byte directly
				if (token == 0x7F)
				{
					tok_line[line_len++] = in[c++];
				}
			}
			break;
		}
	}

	return out_len + 2;
}

/*
=======================================================
Find the address of directory entry for the specified file.
=======================================================
*/
unsigned short find_file_in_directory(const char* filename)
{
	unsigned short 	addr;
	int				dir_index, x;

	// Determine if file alaready exists in directory
	addr = gStdRomDesc->sDirectory;
	dir_index = 0;
	while (dir_index < gStdRomDesc->sDirCount)
	{
		// Test if this slot is empty
		if ((get_memory8(addr) & 0x80) == 0)
		{
			addr += 11;
			dir_index++;
			continue;
		}

		// Compare this entry to the file being added
		for (x = 0; x < 8; x++)
		{
			if (get_memory8(addr + 3 + x) != filename[x])
				break;
		}

		// Test if we matched the file (i.e. it exists already)
		if (x == 8)
		{
			// File return the address
			return addr;
		}

		// Advance to next file in directory
		addr += 11;
		dir_index++;
	}

	// File not found, return address of zero
	return 0;
}

/*
=======================================================
Add an offset to all entries in the directory whose
file content is higher in memory than the specified
address.
=======================================================
*/
void update_file_offsets(unsigned short start_addr, int len)
{
	unsigned short	dir_addr;
	unsigned short	file_addr;
	int				dir_index;

	// Update existing Directory entry addresses
	dir_addr = gStdRomDesc->sDirectory;
	dir_index = 0;
	while (dir_index < gStdRomDesc->sDirCount)
	{
		// Check if this entry is active
		if (get_memory8(dir_addr) == 0)
		{
			dir_addr += 11;
			dir_index++;
			continue;
		}

		// Get address of this file
		file_addr = get_memory16(dir_addr + 1);
		
		// Check if file moved
		if (file_addr >= start_addr)
			set_memory16(dir_addr + 1, file_addr + len);

		dir_addr += 11;   
		dir_index++;
	}
}

/*
=======================================================
Routine to load files from host into the emulation.
=======================================================
*/
int delete_file(const char* filename, unsigned short dir_addr)
{
	unsigned short	file_addr;	// Address of file data
	unsigned short	len;
	unsigned short	tmp_addr;
	unsigned short	reloc_addr;
	int				file_type, dir_index;
	int				x, i;
	char			mt_file[10];

	// First determine if an address was given.  If not, find it
	if (dir_addr == 0)
	{
		// Convert filename to MT format
		i = 0;
		if (filename[i] == '"')
			i++;
		for (x = 0; x < 6; x++)
		{
			if  (filename[i] == '.' || filename[i] == '"')
				mt_file[x] = ' ';
			else
				mt_file[x] = toupper(filename[i++]);
		}
		if (filename[i] == '.')
			i++;
		for (; x < 8; x++)
		{
			if (filename[i] == '\0' || filename[i] == '"')
				mt_file[x] = ' ';
			else
				mt_file[x] = toupper(filename[i++]);
		}

		// Find the filename in the directory
		dir_addr = find_file_in_directory(mt_file);
		if (dir_addr == 0)
			return 0;
	}

	// Get the file address
	file_addr = get_memory16(dir_addr + 1);

	// Determine length of file being deleted
	len = 0;
	tmp_addr = file_addr;
	if (get_memory8(dir_addr + 3 + 6) == 'D')
	{
		// Set the file type to .DO
		file_type = TYPE_DO;

		// Find end of .DO file
		while (get_memory8(tmp_addr) != 0x1A)
		{
			len++;
			tmp_addr++;
		}
		// Include terminating 0x1A in length
		len++;
	}
	else if (get_memory8(dir_addr + 3 + 6) == 'C')
	{
		// Set the file type to .CO
		file_type = TYPE_CO;

		// Deleting a binary file
		len = get_memory16(file_addr + 2);
	}
	else
	{
		// Set the file type to .BA
		file_type = TYPE_BA;

		// Must be a BASIC file, but validate that
		if (get_memory8(dir_addr + 3 + 6) != 'B')
		{
			// ERROR!  TODO: Report it somehow!!
			return 0;
		}

		// Yep, it's a BASIC file.  Find end of BASIC program
		while (get_memory16(tmp_addr) != 0)
			tmp_addr = get_memory16(tmp_addr);

		// Be sure to include the terminating 0x0000 in the length
		len = tmp_addr - file_addr + 2;
	}

	// Delete the file from the directory (mark it invalid)
	set_memory8(dir_addr, 0);

	// Move files and variable data after this file (i.e. higher in 
	// memory space) to delete it from the "file system"
	int move_len = get_memory16(gStdRomDesc->sUnusedMem) - (file_addr + len);
	for (x = 0; x < move_len; x++)
		set_memory8(file_addr + x, get_memory8(file_addr + len + x)); 

	// Update existing Directory entry addresses
	update_file_offsets(file_addr, -len);

	// Update pointers of all BASIC programs
	dir_addr = gStdRomDesc->sDirectory;
	dir_index = 0;
	while (dir_index < gStdRomDesc->sDirCount)
	{
		// Check if this entry is active
		if (get_memory8(dir_addr) != TYPE_BA)
		{
			dir_addr += 11;
			dir_index++;
			continue;
		}

		// Get address of this file
		reloc_addr = get_memory16(dir_addr + 1);
		if (reloc_addr >= file_addr)
		{
			// Update BASIC pointers for this file
			while (get_memory16(reloc_addr) != 0)
			{
				// Subtract len from current pointer
				set_memory16(reloc_addr, get_memory16(reloc_addr) - len);

				// Get pointer to next line
				reloc_addr = get_memory16(reloc_addr);
			}
		}

		// Advance to next entry in directory
		dir_addr += 11;   
		dir_index++;
	}

	// If a .BA file was added, renumber the address
	// pointers in any "unsaved" BASIC program that
	// may exist.
	if (file_type == TYPE_BA)
	{
		// Update BASIC pointers for the Unsaved BASIC program
		reloc_addr = get_memory16(gStdRomDesc->sFilePtrBA);

		while (get_memory16(reloc_addr) != 0)
		{
			set_memory16(reloc_addr, get_memory16(reloc_addr)-len);
			reloc_addr = get_memory16(reloc_addr);
		}
	}

	// Update system pointers
	reloc_addr = get_memory16(gStdRomDesc->sBeginArray);
	if (reloc_addr >= file_addr)
		set_memory16(gStdRomDesc->sBeginArray, reloc_addr - len);

	if ((file_type != TYPE_CO) || (file_type == TYPE_HEX))
	{
		reloc_addr = get_memory16(gStdRomDesc->sBeginCO);
		if (reloc_addr > file_addr)
			set_memory16(gStdRomDesc->sBeginCO, reloc_addr - len);
	}

	// If we deleted a BASIC file, then update the beginning of .DO file pointer
	if (file_type == TYPE_BA)
	{
		/* Update beginning of .DO file pointer */
		reloc_addr = get_memory16(gStdRomDesc->sBeginDO);
		if (reloc_addr > file_addr)
			set_memory16(gStdRomDesc->sBeginDO, reloc_addr - len);

		/* Update BASIC size variable */
		reloc_addr = get_memory16(gStdRomDesc->sBasicSize);
		set_memory16(gStdRomDesc->sBasicSize, reloc_addr - (len-2));
		//set_memory16(gStdRomDesc->sBasicSize, reloc_addr - len);
	}

	// Update beginning of variable space pointer
	reloc_addr = get_memory16(gStdRomDesc->sBeginVar);
	if (reloc_addr >= file_addr)
		set_memory16(gStdRomDesc->sBeginVar, reloc_addr - len);

	// Update unused memory variable
	reloc_addr = get_memory16(gStdRomDesc->sUnusedMem);
	if (reloc_addr >= file_addr)
		set_memory16(gStdRomDesc->sUnusedMem, reloc_addr - len);

	// Update file view if open
	fileview_model_changed();

	// Reset the system so file will show up
	jump_to_zero();

	// Indicate file deleted successfully
	return 1;
}

/*
=======================================================
Routine to load files from host into the emulation.
=======================================================
*/
void cb_LoadFromHost(Fl_Widget* w, void* host_filename)
{
	int					count, i, x;
	Flu_File_Chooser	*fc = NULL;
	const char			*filename;
	const char			*filename_name;
	char                mt_file[8];
	int					len;
	FILE				*fd;
	int					file_type;
	unsigned char		data[262144];
	unsigned char		conv[32768];
	unsigned short		start_addr;
	char				ch;
	unsigned short		addr1, addr2, addr3, addr4;
	int					dir_index;
	int                 move_len;
	int					need_tokenize;
	
	if (w != NULL)
	{
		fl_cursor(FL_CURSOR_WAIT);
#ifdef __APPLE__
		fc = new Flu_File_Chooser(path,"Model T Files (*.{do,txt,ba,co,hex})",1,"Load file from Host");
#else
		fc = new Flu_File_Chooser(".","Model T Files (*.{do,txt,ba,co,hex})",1,"Load file from Host");
#endif
		fl_cursor(FL_CURSOR_DEFAULT);
		fc->preview(0);
		fc->show();

		while (fc->visible())
			Fl::wait();

		count = fc->count();
		if (count == 0)
		{
			delete fc;
			return;
		}

		// Get Filename
		filename = fc->value();
		if (filename == 0)
		{
			delete fc;
			return;
		}

		len = strlen(filename);
	}
	else
	{
		if (host_filename == NULL)
			return;

		filename = (const char*) host_filename;
		len = strlen(filename);
	}

	// Open file
	fd = fopen(filename, "rb");
	if (fd == 0)
	{
		if (w != NULL)
			fl_message("Unable to open file %s", filename);
		else
			gLoadError = 1;
		if (fc != NULL)
			delete fc;
		return;
	}

	// Determine type of file
	file_type = TYPE_DO;
	if (((filename[len-1] | 0x20) == 'o') &&
		((filename[len-2] | 0x20) == 'c'))
			file_type = TYPE_CO;
	// Deal with other binary types
	if (((filename[len-2] | 0x20) == 'c') &&
		(filename[len-3] == '.'))
			file_type = TYPE_CO;
	if (((filename[len-1] | 0x20) == 'a') &&
		((filename[len-2] | 0x20) == 'b'))
			file_type = TYPE_BA;
	if (((filename[len-1] | 0x20) == 'x') &&
		((filename[len-2] | 0x20) == 'e') &&
		((filename[len-3] | 0x20) == 'h'))
			file_type = TYPE_HEX;

	// Convert filename to Model T name
	filename_name = fl_filename_name(filename);
	i = 0;
	for (x = 0; x < 6; x++)
	{
		if (filename_name[i] == '.')
			mt_file[x] = ' ';
		else
			mt_file[x] = toupper(filename_name[i++]);
	}
	if (file_type == TYPE_BA)
	{
		mt_file[6] = 'B';
		mt_file[7] = 'A';
	}
	else if (file_type == TYPE_DO)
	{
		mt_file[6] = 'D';
		mt_file[7] = 'O';
	}
	else if (file_type == TYPE_CO)
	{
		mt_file[6] = 'C';
		mt_file[7] = toupper(filename[len-1]);
	}	
	else if (file_type == TYPE_HEX)
	{
		mt_file[6] = 'C';
		mt_file[7] = 'O';
	}	
	
	// Determine "RAW" length of file (w/o CRLF expansion)
	fseek(fd, 0, SEEK_END);
	len = ftell(fd);
	if (len > 262144)
	{
		fclose(fd);
		if (w != NULL)
			fl_message("%s", gTooLargeMsg);
		else
			gLoadError = 1;
		if (fc != NULL)
			delete fc;
		return;
	}

	// Read contents to buffer
	fseek(fd, 0, SEEK_SET);
	int readlen = fread(data, 1, len, fd);

	// Close the file
	fclose(fd);

	// Determine file location
	if (file_type == TYPE_BA)
	{
		addr1 = get_memory16(gStdRomDesc->sFilePtrBA);
	}
	else if ((file_type == TYPE_CO) || (file_type == TYPE_HEX))
	{
		addr1 = get_memory16(gStdRomDesc->sBeginVar);
	} 
	else if (file_type == TYPE_DO)
	{
		addr1 = get_memory16(gStdRomDesc->sFilePtrDO);
	}

	// Determine length of data and expand LF to CRLF
	if (file_type == TYPE_DO)
	{
		i=x=ch=0;
		while (i < len)
		{
			if ((data[i] == 0x0A) && (ch != 0x0D))
				conv[x++] = 0x0D;
			ch = data[i++];
			conv[x++] = ch;
			if (x >= 32768)
			{
				if (w != NULL)
					fl_message("%s", gTooLargeMsg);
				else
					gLoadError = 1;
				if (fc != NULL)
					delete fc;
				return;
			}
		}

		if (ch != 0x1A)
		{
			conv[x++] = 0x1A;
		}

		len = x;
	}
	// For BASIC files, determine if tokenization required
	else if (file_type == TYPE_BA)
	{
		need_tokenize = 1;

		// Check first 10 bytes searching for control characters
		for (x = 0; x < 10; x++)
		{
			if (iscntrl(data[x]) && (data[x] != 0x0d) && (data[x] != 0x0a))
			{
				need_tokenize = 0;
				break;
			}
		}

		if (need_tokenize)
		{
			// Insure file ends with CR
			data[len] = 0x0a;
			data[len+1] = 0;

			// Tokenize the file & locate at addr1
			len = tokenize(data, conv, addr1);
		}
		else
		{
			// Insure tokenized file ends with 0x0000
			data[len] = 0;
			data[len+1] = 0;

			// Relocate to new address
			len = relocate(data, conv, addr1);
		}
	}
	// For HEX files, load the hex file into conv to get length
	else if (file_type == TYPE_HEX)
	{
		len = load_hex_file((char *)  filename, (char *) data, &start_addr);
		if (len == 0)
		{
			if (w != NULL)
				fl_message("Invalid HEX file format");
			else
				gLoadError = 1;
		}
	}

	// Dont add zero length files
	if (len == 0)
	{
		if (fc != NULL)
			delete fc;
		return;
	}

	// Determine if file alaready exists in directory
	addr2 = find_file_in_directory(mt_file);
	if (addr2 != 0)
	{
		// File exists.  If we are calling from the menu, validate they want to replace
		if (w != NULL)
		{
			int ans = fl_choice("Replace existing file?", "Cancel", "Yes", NULL);

			// Test if "Cancel" was selected
			if (ans == 0)
			{
				if (fc != NULL)
					delete fc;
				return;
			}
		}

		// Delete the file
		if (!delete_file(mt_file, addr2))
		{
			// Could not delete the file for some reason??
			if (fc != NULL)
				delete fc;
			gLoadError = 1;
			return;
		}
	}

	// Determine file location
	if (file_type == TYPE_BA)
	{
		addr1 = get_memory16(gStdRomDesc->sFilePtrBA);
	}
	else if ((file_type == TYPE_CO) || (file_type == TYPE_HEX))
	{
		addr1 = get_memory16(gStdRomDesc->sBeginVar);
	} 
	else if (file_type == TYPE_DO)
	{
		addr1 = get_memory16(gStdRomDesc->sFilePtrDO);
	}

	// Determine if file will fit in memory
	addr3 = get_memory16(gStdRomDesc->sBasicStrings);
	addr2 = get_memory16(gStdRomDesc->sBeginVar);
	if (addr3 - addr2 < len)
	{
		if (w != NULL)
			fl_message("%s", gTooLargeMsg);
		else
			gLoadError = 1;
		if (fc != NULL)
			delete fc;
		return;
	}

	// Determine Directory entry location for new file
	addr4 = gStdRomDesc->sDirectory + 11 * gStdRomDesc->sFirstDirEntry;
	dir_index = 0;
	while (get_memory8(addr4) != 0)
	{
		// Point to next directory entry
		addr4 += 11;
		if (++dir_index >= gStdRomDesc->sDirCount)
		{
			if (w != NULL)
				fl_message("Too many files in directory");
			else
				gLoadError = 1;
			if (fc != NULL)
				delete fc;
			return;
		}
	}

	// Move memory to make space for file
	move_len = get_memory16(gStdRomDesc->sUnusedMem) - addr1;
	for (x = move_len-1; x >= 0; x--)
		set_memory8(addr1+len+x, get_memory8(addr1+x)); 

	// Update existing Directory entry addresses
	update_file_offsets(addr1, len);

	// Add new Directory entry
	if (file_type == TYPE_HEX)
		set_memory8(addr4, TYPE_CO);
	else
		set_memory8(addr4, file_type);
	set_memory16(addr4+1, addr1);
	for (x = 0; x < 8; x++)
		set_memory8(addr4+3+x, mt_file[x]);

	// If a .BA file was added, renumber the address
	// pointers in any "unsaved" BASIC program that
	// may exist.
	if (file_type == TYPE_BA)
	{
		addr3 = get_memory16(gStdRomDesc->sFilePtrBA);

		while (get_memory16(addr3) != 0)
		{
			set_memory16(addr3, get_memory16(addr3)+len);
			addr3 = get_memory16(addr3);
		}
	}

	// Update Other pointers stored in memory
	addr3 = get_memory16(gStdRomDesc->sBeginArray);
	if (addr3 >= addr1)
		set_memory16(gStdRomDesc->sBeginArray, addr3 + len);

	if ((file_type != TYPE_CO) || (file_type == TYPE_HEX))
	{
		addr3 = get_memory16(gStdRomDesc->sBeginCO);
		if (addr3 > addr1)
			set_memory16(gStdRomDesc->sBeginCO, addr3 + len);
	}

	if (file_type == TYPE_BA)
	{
		/* Update beginning of .DO file pointer */
		addr3 = get_memory16(gStdRomDesc->sBeginDO);
		if (addr3 > addr1)
			set_memory16(gStdRomDesc->sBeginDO, addr3 + len);

		/* Update BASIC size variable */
		addr3 = get_memory16(gStdRomDesc->sBasicSize);
		set_memory16(gStdRomDesc->sBasicSize, addr3 + len-2);
	}

	addr3 = get_memory16(gStdRomDesc->sBeginVar);
	if (addr3 >= addr1)
		set_memory16(gStdRomDesc->sBeginVar, addr3 + len);

	addr3 = get_memory16(gStdRomDesc->sUnusedMem);
	if (addr3 >= addr1)
		set_memory16(gStdRomDesc->sUnusedMem, addr3 + len);

	// Copy the data to the Model T
	if (file_type == TYPE_DO)
	{
		for (x = 0; x < len; x++)
			set_memory8(addr1+x, conv[x]);
	}
	else if ((file_type == TYPE_CO) || (file_type == TYPE_HEX))
	{
		for (x = 0; x < len; x++)
			set_memory8(addr1+x, data[x]);
	}
	else if (file_type == TYPE_BA)
	{
		for (x = 0; x < len; x++)
			set_memory8(addr1+x, conv[x]);
	}

	// Update file view if open
	fileview_model_changed();

	// Reset the system so file will show up
	jump_to_zero();

	if (fc != NULL)
		delete fc;
}

int remote_load_from_host(const char *filename)
{
	gLoadError = 0;
	cb_LoadFromHost(NULL, (void *) filename);
	return gLoadError;
}

/*
=======================================================================
Routines to save Model T files to the host
=======================================================================
*/
Fl_Window	*gSaveToHost;
int			gSave;

void cb_save (Fl_Widget* w, void*)
{
	gSave = 1;
	gSaveToHost->hide();
}

void cb_cancel (Fl_Widget* w, void*)
{
	gSave = 0;
	gSaveToHost->hide();
}

void save_file(model_t_files_t *pFile)
{
	unsigned short	addr1, addr2;
	unsigned char	ch;
	unsigned short	len;
	int				c;
	unsigned char	type;
	unsigned short	line;
	FILE			*fd;
	Flu_File_Chooser *fc;
	char            filename[12];

	if (pFile == 0)
		return;

	// Get output filename
	strcpy(filename, pFile->name);
	len = strlen(filename);
	if (filename[len-1] == 'O' && filename[len-2] == 'C')
	{
		if (COSaveMode == 1)
		{
			filename[len-3] = 0;
			strcat(filename, ".HEX");
		}
	}
	
	fl_cursor(FL_CURSOR_WAIT);
#ifdef __APPLE__
	fc = new Flu_File_Chooser(filename,"*.*",Fl_File_Chooser::CREATE,"Save file as");
#else
	fc = new Flu_File_Chooser(filename,"*.*",Fl_File_Chooser::CREATE,"Save file as");
#endif
	fl_cursor(FL_CURSOR_DEFAULT);
	fc->preview(0);
	// FileWin.value(0)="RAM.bin"
	fc->show();

	while (fc->visible())
		Fl::wait();


	// Check if a file was selected
	if (fc->value() == 0)
	{
		delete fc;
		return;
	}
	if (strlen(fc->value()) == 0)
	{
		delete fc;
		return;
	}


	// Try to open the ouput file
	fd = fopen(fc->value(), "rb+");
	if (fd != NULL)
	{
		// File already exists! Check for overwrite!
		fclose(fd);

		c = fl_choice("Overwrite existing file?", "Cancel", "Yes", "No");

		if ((c == 0) || (c == 2))
		{
			delete fc;
			return;
		}
	}

	fd = fopen(fc->value(), "wb+");
	if (fd == NULL)
	{
		fl_message("Error opening %s for write", fc->value());
		delete fc;
		return;
	}

	delete fc;

	// Get file type
	type = get_memory8(pFile->dir_address);

	// Save base on file type
	switch (type)
	{
	case TYPE_BA:
		addr1 = pFile->address;
		while ((addr2 = get_memory16(addr1)) != 0)
		{
			if (BasicSaveMode == 0)
			{
				// Save line as tokenized data
				for (c = addr2-addr1; c > 0; c--)
				{
					ch = get_memory8(addr1++);
					fwrite(&ch, 1, 1, fd);
				}
			}
			else
			// Detokenize the line and save as ASCII
			{
				addr1 += 2;
				// Print line number
				line = get_memory16(addr1);
				addr1 += 2;
				fprintf(fd, "%u ", line);

				// Read remaining data & detokenize
				while (addr1 < addr2)
				{
					// Get next byte
					ch = get_memory8(addr1++);
					
					// Check if byte is ':'
					if (ch == ':')
					{
						// Get next character
						ch = get_memory8(addr1);

						// Check for REM tick
						if (ch == 0x8E)
						{
							if (get_memory8(addr1+1) == 0xFF)
							{
								ch = '\'';
								fwrite(&ch, 1, 1, fd);
								addr1 += 2;
								continue;
							}
						}

						// Check for ELSE
						if (ch == 0x91)
							continue;

						ch = ':';
						fwrite(&ch, 1, 1, fd);
					}
					else if (ch > 0x7F)
					{
						fprintf(fd, "%s", gKeywordTable[ch & 0x7F]);
					}
					else if (ch == 0)
					{
						fprintf(fd, "\n");
					}
					else
					{
						fwrite(&ch, 1, 1, fd);
					}

				}
			}

		}
		break;

	case TYPE_DO:
		addr1 = pFile->address;
		while ((ch = get_memory8(addr1++)) != 0x1a )
		{
			fwrite(&ch, 1, 1, fd);
		}

		// Write end of file marker (0x1a) to file
		fwrite(&ch, 1, 1, fd);
		break;

	case TYPE_CO:
		// Get length of file to be saved
		addr1 = pFile->address;
		len = get_memory16(addr1+2) + 6;
		if (COSaveMode == 1)
		{
			save_hex_file(addr1, addr1+len, 0, fd);
		}
		else
		{
			for (c = 0; c < len; c++)
			{
				ch = get_memory8(addr1+c);
				fwrite(&ch, 1, 1, fd);
			}
		}
		break;
	}

	// Close the output file
	fclose(fd);
}

void cb_SaveToHost(Fl_Widget* w, void*)
{
	Fl_Hold_Browser		*fsb;
	Fl_Button			*fsave;
	Fl_Button			*fcancel;
	unsigned short		addr1;
	char				mt_file[10];
	int					dir_index;
	unsigned char		file_type;
	int					x, c;
	model_t_files_t		tFiles[100];
	int					tCount;

	// Create the windows
	gSaveToHost = new Fl_Window(220,320,"Select file to save to Host");
	fsb = new Fl_Hold_Browser(0, 0, 220,280);
	fsave = new Fl_Button(20,288,80,25,"Save");
	fsave->callback(cb_save);
	fcancel = new Fl_Button(130, 288, 80, 25, "Cancel");
	fcancel->callback(cb_cancel);

	// Add Model T files to the browser
	addr1 = gStdRomDesc->sDirectory;
	dir_index = 0;	 
	tCount = 0;
	while (dir_index < gStdRomDesc->sDirCount)
	{
		file_type = get_memory8(addr1);
		if ((file_type != TYPE_BA) && (file_type != TYPE_DO) &&
			(file_type != TYPE_CO))
		{
			dir_index++;
			addr1 += 11;
			continue;
		}

		c = 0;
		for (x = 0; x < 6; x++)
		{
			if (get_memory8(addr1+3+x) != ' ')
				mt_file[c++] = get_memory8(addr1+3+x);
		}

		mt_file[c++] = '.';
		mt_file[c++] = get_memory8(addr1+3+x++);
		mt_file[c++] = get_memory8(addr1+3+x);
		mt_file[c] = '\0';

		// Add filename to tFiles structure
		strcpy(tFiles[tCount].name, mt_file);

		// Get address of file
		tFiles[tCount].address = get_memory16(addr1+1);
		tFiles[tCount++].dir_address = addr1;

		fsb->add(mt_file);

		dir_index++;
		addr1 += 11;
	}

	// Show the window
	gSaveToHost->resizable(fsb);
	gSaveToHost->show();

	// Default to No-save and wait for a button
	gSave = 0;
	while (gSaveToHost->visible())
		Fl::wait();

	// Check if save button was selected
	if (gSave)
	{
		// Find entry for the file selected
		c = fsb->value();
		if (c)
		{
			for (x = 0; x < tCount; x++)
			{
				// Search array for entry
				if (strcmp(fsb->text(c), tFiles[x].name) == 0)
				{
					save_file(&tFiles[x]);
					break;
				}
			}
		}
	}

	// Cleanup the windows stuff
	delete fsave;
	delete fcancel;
	delete fsb;
	delete gSaveToHost;
}

#ifdef	__APPLE__
//JV 08/10/05: add a function to choose the working directory (where are the ROMs files)
const char* ChooseWorkDir()
{
	Flu_File_Chooser	*fileWin;
	int					count;
	const char 			*ret = NULL;
	
	// Create a File chooser
	fl_cursor(FL_CURSOR_WAIT);
	fileWin = new Flu_File_Chooser(path, "", 
		Flu_File_Chooser::DIRECTORY, "Choose Working Directory");
	fl_cursor(FL_CURSOR_DEFAULT);
	fileWin->preview(0);
	fileWin->show();

	// Wait until selection is made or cancel
	while (fileWin->visible())
		Fl::wait();

	// Determine if selection made
	count = fileWin->count();
	if (count == 0)
	{
		delete fileWin;
		return NULL;
	}

	// Get the path of the new location
	ret = fileWin->value();

	return ret;
}
//JV
#endif
