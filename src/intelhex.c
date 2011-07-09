/* intelhex.c */

/* $Id: intelhex.c,v 1.7 2008/11/04 07:31:22 kpettit1 Exp $ */

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



/* Intel HEX read/write functions, Paul Stoffregen, paul@ece.orst.edu */
/* This code is in the public domain.  Please retain my name and */
/* email address in distributed copies, and let me know about any bugs */

/* I, Paul Stoffregen, give no warranty, expressed or implied for */
/* this software and/or documentation provided, including, without */
/* limitation, warranty of merchantability and fitness for a */
/* particular purpose. */


#include <stdio.h>
#include <string.h>
#include "gen_defs.h"
#include "intelhex.h"
#include "memory.h"

/* some ansi prototypes.. maybe ought to make a .h file */

/* this is used by load_file to get each line of intex hex */
int parse_hex_line(char *theline, int bytes[], int *addr, int *num, int *code);

/* this does the dirty work of writing an intel hex file */
/* caution, static buffering is used, so it is necessary */
/* to call it with end=1 when finsihed to flush the buffer */
/* and close the file */
void hexout(FILE *fhex, int byte, int memory_location, int end);


//extern char	memory[65536];		/* the memory is global */

/* parses a line of intel hex code, stores the data in bytes[] */
/* and the beginning address in addr, and returns a 1 if the */
/* line was valid, or a 0 if an error occured.  The variable */
/* num gets the number of bytes that were stored into bytes[] */

int parse_hex_line(theline, bytes, addr, num, code)
char *theline;
int *addr, *num, *code, bytes[];
{
	int sum, len, cksum;
	char *ptr;
	
	*num = 0;
	if (theline[0] != ':') return 0;
	if (strlen(theline) < 11) return 0;
	ptr = theline+1;
	if (!sscanf(ptr, "%02x", &len)) return 0;
	ptr += 2;
	if ( (int)strlen(theline) < (11 + (len * 2)) ) return 0;
	if (!sscanf(ptr, "%04x", addr)) return 0;
	ptr += 4;
	  /* printf("Line: length=%d Addr=%d\n", len, *addr); */
	if (!sscanf(ptr, "%02x", code)) return 0;
	ptr += 2;
	sum = (len & 255) + ((*addr >> 8) & 255) + (*addr & 255) + (*code & 255);
	while(*num != len) {
		if (!sscanf(ptr, "%02x", &bytes[*num])) return 0;
		ptr += 2;
		sum += bytes[*num] & 255;
		(*num)++;
		if (*num >= 256) return 0;
	}
	if (!sscanf(ptr, "%02x", &cksum)) return 0;
	if ( ((sum & 255) + (cksum & 255)) & 255 ) return 0; /* checksum error */
	return 1;
}

/* loads an intel hex file into the global memory[] array */
/* filename is a string of the file to be opened */

int load_hex_file(char *filename, char *buffer, unsigned short *start_addr)
{
	char line[1000];
	FILE *fin;
	int addr, n, status, bytes[256];
	int i, total=0, lineno=1;
	int minaddr=65536, maxaddr=0;
	char	mem[65536];

	fin = fopen(filename, "rb");
	if (fin == NULL) {
		return 0;
	}
	while (!feof(fin) && !ferror(fin)) {
		line[0] = '\0';
		i = 0;
		while (!feof(fin))
		{
			line[i] = fgetc(fin);
			// Check for CR/LF
			if ((line[i] == 0x0d) || line[i] == 0x0a)
			{
				if (i != 0)
					break;;
			}
			// Check for end of file marker
			else if (line[i] == 0x1a)
				break;
			else if (line[i] != -1)
				i++;
		}
		if (i == 0)
			break;
		line[i] = 0;
		if (line[strlen(line)-1] == 0x0a) line[strlen(line)-1] = '\0';
		if (line[strlen(line)-1] == 0x0d) line[strlen(line)-1] = '\0';
		if (parse_hex_line(line, bytes, &addr, &n, &status)) {
			if (status == 0) {  /* data */
				for(i=0; i<=(n-1); i++) {
					mem[addr] = bytes[i] & 255;
					total++;
					if (addr < minaddr) minaddr = addr;
					if (addr > maxaddr) maxaddr = addr;
					addr++;
				}
			}
			if (status == 1) {  /* end of file */
				fclose(fin);
				n = 0;
				for (i = minaddr; i <maxaddr; i++)
				{
					buffer[n++] = mem[i];
				}
				*start_addr = minaddr;
				return n;
			}
			if (status == 2) ;  /* begin of file */
		} else {
			fclose(fin);
			return 0;
		}
		lineno++;
	}

	fclose(fin);
	n = 0;
	for (i = minaddr; i <= maxaddr; i++)
		buffer[n++] = mem[i];
	*start_addr = minaddr;
	return n;
}

/* the command string format is "S begin end filename" where */
/* "begin" and "end" are the locations to dump to the intel */
/* hex file, specified in hexidecimal. */

void save_hex_file_ext(int begin, int end, int region, FILE* fd)
{
	int		addr;
	char	buffer[16384];
	int		count, len, x;

	if (begin > end) {
		return;
	}

	// Calculate length of memory region
	count = end - begin + 1;
	addr = begin;
	len = count > sizeof(buffer) ? sizeof(buffer) : count;

	// Copy data out 1 buffer at a time
	while (count > 0)
	{
		// Read next buffer from memory
		get_memory8_ext(region, addr, len, (unsigned char*) buffer);

		// Output all bytes in the buffer
		for (x = 0; x < len; x++)
			hexout(fd, buffer[x], addr + x - begin, 0);

		// Update count
		count -= len;
		addr += len;

		// Update len for next loop
		len = count > sizeof(buffer) ? sizeof(buffer) : count;
	}
	hexout(fd, 0, 0, 1);
}


/* the command string format is "S begin end filename" where */
/* "begin" and "end" are the locations to dump to the intel */
/* hex file, specified in hexidecimal. */

void save_hex_file(int begin, int end, FILE* fd)
{
	int  addr;

	if (begin > end) {
		return;
	}
	for (addr=begin; addr <= end; addr++)
		hexout(fd, get_memory8((unsigned short) addr), addr, 0);
	hexout(fd, 0, 0, 1);
}

/* the command string format is "S begin end filename" where */
/* "begin" and "end" are the locations to dump to the intel */
/* hex file, specified in hexidecimal. */

void save_hex_file_buf(char *buf, int begin, int end, FILE* fd)
{
	int  addr;

	if (begin > end) {
		return;
	}
	for (addr=begin; addr <= end; addr++)
		hexout(fd, buf[(unsigned short) addr], addr, 0);
	hexout(fd, 0, 0, 1);
}



/* produce intel hex file output... call this routine with */
/* each byte to output and it's memory location.  The file */
/* pointer fhex must have been opened for writing.  After */
/* all data is written, call with end=1 (normally set to 0) */
/* so it will flush the data from its static buffer */

#define MAXHEXLINE 32	/* the maximum number of bytes to put in one line */

void hexout(FILE *fhex, int byte, int memory_location, int end)
{
	static int byte_buffer[MAXHEXLINE];
	static int last_mem, buffer_pos, buffer_addr;
	static int writing_in_progress=0;
	register int i, sum;

	if (!writing_in_progress) {
		/* initial condition setup */
		last_mem = memory_location-1;
		buffer_pos = 0;
		buffer_addr = memory_location;
		writing_in_progress = 1;
		}

	if ( (memory_location != (last_mem+1)) || (buffer_pos >= MAXHEXLINE)
	 || ((end) && (buffer_pos > 0)) ) {
		/* it's time to dump the buffer to a line in the file */
		fprintf(fhex, ":%02X%04X00", buffer_pos, buffer_addr);
		sum = buffer_pos + ((buffer_addr>>8)&255) + (buffer_addr&255);
		for (i=0; i < buffer_pos; i++) {
			fprintf(fhex, "%02X", byte_buffer[i]&255);
			sum += byte_buffer[i]&255;
		}
		fprintf(fhex, "%02X\n", (-sum)&255);
		buffer_addr = memory_location;
		buffer_pos = 0;
	}

	if (end) {
		fprintf(fhex, ":00000001FF\n");  /* end of file marker */
		fclose(fhex);
		writing_in_progress = 0;
	}
		
	last_mem = memory_location;
	byte_buffer[buffer_pos] = byte & 255;
	buffer_pos++;
}



