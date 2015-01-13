/* io.c */

/* $Id: io.c,v 1.24 2013/03/05 20:43:46 kpettit1 Exp $ */

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


#include <time.h>
#ifdef __APPLE__
#include <sys/time.h>
#endif
#include <stdio.h>

#include "VirtualT.h"
#include "cpu.h"
#include "gen_defs.h"
#include "roms.h"
#include "io.h"
#include "serial.h"
#include "display.h"
#include "setup.h"
#include "m100emu.h"
#include "memory.h"
#include "sound.h"
#include "lpt.h"
#include "clock.h"

uchar lcd[10][256];
uchar lcdpointers[10]={0,0,0,0,0,0,0,0,0,0};
uchar lcddriver=0;
uchar lcd_fresh_ptr[10] = {1,1,1,1,1,1,1,1,1,1 };
uchar io21;			/* Real-time milisecond countdown */
double gPort21Time;	/* Starting time for io21 from previous out */
uchar io90;
uchar ioA1;
static uchar ioA3;
uchar ioBA;
uchar ioB9;
uchar ioE8;
uchar ioBC;		/* Low byte of 14-bit timer */
uchar ioBD;		/* High byte of 14-bit timer */
uchar ioD0;		/* D0-DF io for T200 */
uchar t200_ir;  /* Instruction register */
uchar t200_mcr; /* Mode Control Register */
uchar t200_uart_state = 0;
int		gInMsPlanROM = FALSE;
uchar keyscan[9] = { 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF};
char  keyin[9];
extern int		sound_enable;

uint			lcdbits=0;
unsigned long	gSpecialKeys = 0;
unsigned char	gKeyStates[128];
int				gDelayUpdateKeys = 0;
int				gDelayCount = 0;
extern uchar	clock_serial_out;
extern int		gRomBank;
extern RomDescription_t	*gStdRomDesc;
void handle_wheel_keys(void);

int gCapture = FALSE;
/*
=============================================================================
This routine supplies an OS independant high-resolution timer for use with
emulation speed calculations and throttling.
=============================================================================
*/
#ifdef _WIN32
double hirestimer(void)
{
    static LARGE_INTEGER pcount, pcfreq;
    static int initflag = 0;

    if (!initflag)
	{
	    QueryPerformanceFrequency(&pcfreq);
        initflag++;
    }

    QueryPerformanceCounter(&pcount);
    return (double)pcount.QuadPart / (double)pcfreq.QuadPart;
}
#else
inline double hirestimer(void)
{
    struct timeval tv;
    gettimeofday(&tv, NULL);
    return (double)tv.tv_sec + (double)tv.tv_usec / 1000000;
}
#endif

void update_keys(void)
{
	int equal = 1;
	int buf_index;
	int c;
	unsigned char key;

	if (gStdRomDesc == NULL)
		return;

	buf_index = gStdRomDesc->sKeyscan;

	equal = 1;
	/* Insure keystroke was recgonized by the system */
	for (c = 0; c < 9; c++)
	{
		key = (uchar) ((~keyscan[c]) & 0xFF);
		/* Old keyscan matrix must match buffer in memory */
		if (get_memory8((unsigned short) buf_index) != key)
		{
			equal = 0;
			break;
		}
		buf_index++;
	}

	if (!equal)
	{
		if (++gDelayCount < 10)
		{
			gDelayUpdateKeys = 1;
			return;
		}
	}

	if (gModel == MODEL_M10)
	{
		keyscan[0] = ~(gKeyStates['q']       | (gKeyStates['w'] << 1) |
				  	(gKeyStates['['] << 2) | (gKeyStates['1'] << 3) |
				  	(gKeyStates['2'] << 4) | (gKeyStates['3'] << 5) |
				  	(gKeyStates['4'] << 6) | (gKeyStates['5'] << 7));

		keyscan[1] = ~(gKeyStates['y']       | (gKeyStates['6'] << 1) |
				  	(gKeyStates['7'] << 2) | (gKeyStates['8'] << 3) |
				  	(gKeyStates['9'] << 4) | (gKeyStates['0'] << 5) |
				  	(gKeyStates['-'] << 6) | (gKeyStates['^'] << 7));
	
		keyscan[2] = ~(gKeyStates[']']       | (gKeyStates['@'] << 1) |
				  	(gKeyStates[';'] << 2) | (gKeyStates[':'] << 3) |
				  	(gKeyStates['/'] << 4) | (gKeyStates['.'] << 5) |
				  	(gKeyStates[','] << 6) | (gKeyStates['m'] << 7));

		keyscan[3] = ~(gKeyStates['a']       | (gKeyStates['\\'] << 1) |
				  	(gKeyStates['z'] << 2) | (gKeyStates['x'] << 3) |
				  	(gKeyStates['c'] << 4) | (gKeyStates['v'] << 5) |
				  	(gKeyStates['b'] << 6) | (gKeyStates['n'] << 7));

		keyscan[4] = ~(gKeyStates['s']       | (gKeyStates['d'] << 1) |
				  	(gKeyStates['f'] << 2) | (gKeyStates['g'] << 3) |
				  	(gKeyStates['h'] << 4) | (gKeyStates['j'] << 5) |
				  	(gKeyStates['k'] << 6) | (gKeyStates['l'] << 7));

		keyscan[5] = ~(gKeyStates['e']       | (gKeyStates['r'] << 1) |
				  	(gKeyStates['t'] << 2) | (gKeyStates['u'] << 3) |
				  	(gKeyStates['i'] << 4) | (gKeyStates['o'] << 5) |
				  	(gKeyStates['p'] << 6)) & 
					(unsigned char) (((gSpecialKeys & MT_SPACE) | ~MT_SPACE) >> 9);

		keyscan[6] = (unsigned char) (((gSpecialKeys & MT_LEFT) >> 28) | ~(MT_LEFT >> 28)) &
				 	(unsigned char) (((gSpecialKeys & MT_RIGHT) >> 28) | ~(MT_RIGHT >> 28)) &
				 	(unsigned char) (((gSpecialKeys & MT_UP) >> 28) | ~(MT_UP >> 28)) &
				 	(unsigned char) (((gSpecialKeys & MT_DOWN) >> 28) | ~(MT_DOWN >> 28)) &
				 	(unsigned char) (((gSpecialKeys & MT_BKSP) >> 13) | ~(MT_BKSP >> 13)) &
				 	(unsigned char) (((gSpecialKeys & MT_TAB) >> 13) | ~(MT_TAB >> 13)) &
				 	(unsigned char) (((gSpecialKeys & MT_ENTER) >> 17) | ~(MT_ENTER >> 17)) &
				 	(unsigned char) (((gSpecialKeys & MT_PASTE) >> 13) | ~(MT_PASTE >> 13));

		keyscan[7] = (unsigned char) ((gSpecialKeys >> 8) & 0xFF);

		keyscan[8] = (unsigned char) ((gSpecialKeys & 0x80) | 0x7F) &
					(unsigned char) (((gSpecialKeys & 0x30) << 1) | 0x9F) &
				 	(unsigned char) (((gSpecialKeys & 0x07) << 2) | 0xE3) &
					(unsigned char) (((gSpecialKeys & MT_PRINT) >> 21) | ~(MT_PRINT >> 21)) &
					(unsigned char) (((gSpecialKeys & MT_LABEL) >> 21) | ~(MT_LABEL >> 21));
	}
	else
	{
		keyscan[0] = ~(gKeyStates['z']       | (gKeyStates['x'] << 1) |
				  	(gKeyStates['c'] << 2) | (gKeyStates['v'] << 3) |
				  	(gKeyStates['b'] << 4) | (gKeyStates['n'] << 5) |
				  	(gKeyStates['m'] << 6) | (gKeyStates['l'] << 7));

		keyscan[1] = ~(gKeyStates['a']       | (gKeyStates['s'] << 1) |
				  	(gKeyStates['d'] << 2) | (gKeyStates['f'] << 3) |
				  	(gKeyStates['g'] << 4) | (gKeyStates['h'] << 5) |
				  	(gKeyStates['j'] << 6) | (gKeyStates['k'] << 7));
	
		keyscan[2] = ~(gKeyStates['q']       | (gKeyStates['w'] << 1) |
				  	(gKeyStates['e'] << 2) | (gKeyStates['r'] << 3) |
				  	(gKeyStates['t'] << 4) | (gKeyStates['y'] << 5) |
				  	(gKeyStates['u'] << 6) | (gKeyStates['i'] << 7));

		keyscan[4] = ~(gKeyStates['1']       | (gKeyStates['2'] << 1) |
				  	(gKeyStates['3'] << 2) | (gKeyStates['4'] << 3) |
				  	(gKeyStates['5'] << 4) | (gKeyStates['6'] << 5) |
				  	(gKeyStates['7'] << 6) | (gKeyStates['8'] << 7));

		if (gModel != MODEL_PC8201 && gModel != MODEL_PC8300)
		{
			keyscan[3] = ~(gKeyStates['o']       | (gKeyStates['p'] << 1) |
					  	(gKeyStates['['] << 2) | (gKeyStates[';'] << 3) |
					  	(gKeyStates['\''] << 4) | (gKeyStates[','] << 5) |
					  	(gKeyStates['.'] << 6) | (gKeyStates['/'] << 7));

			keyscan[5] = ~(gKeyStates['9']       | (gKeyStates['0'] << 1) |
					  	(gKeyStates['-'] << 2) | (gKeyStates['='] << 3)) &
					  	(unsigned char) (gSpecialKeys >> 24);

			keyscan[6] = (unsigned char) ((gSpecialKeys >> 16) & 0xFF);
	
			keyscan[7] = (unsigned char) ((gSpecialKeys >> 8) & 0xFF);

			keyscan[8] = (unsigned char) (gSpecialKeys & 0xFF);
		}
		else
		{
			keyscan[3] = ~(gKeyStates['o']       | (gKeyStates['p'] << 1) |
					  	(gKeyStates['@'] << 2) | (gKeyStates['\\'] << 3) |
					  	(gKeyStates[','] << 4) | (gKeyStates['.'] << 5) |
					  	(gKeyStates['/'] << 6) | (gKeyStates[']'] << 7));

			keyscan[5] = ~(gKeyStates['9']       | (gKeyStates['0'] << 1) |
					  	(gKeyStates[';'] << 2) | (gKeyStates[':'] << 3) |
					  	(gKeyStates['-'] << 4) | (gKeyStates['['] << 5)) &
					  	(unsigned char) (((gSpecialKeys & MT_SPACE) | ~MT_SPACE) >> 10) &
					  	(unsigned char) (((gSpecialKeys & MT_INS) | ~MT_INS) >> 17);

			keyscan[6] = (unsigned char) (((gSpecialKeys & MT_BKSP) >> 17) | ~(MT_BKSP >> 17)) &
					 	(unsigned char) (((gSpecialKeys & MT_UP) >> 29) | ~(MT_UP >> 29)) &
					 	(unsigned char) (((gSpecialKeys & MT_DOWN) >> 29) | ~(MT_DOWN >> 29)) &
					 	(unsigned char) (((gSpecialKeys & MT_LEFT) >> 25) | ~(MT_LEFT >> 25)) &
					 	(unsigned char) (((gSpecialKeys & MT_RIGHT) >> 25) | ~(MT_RIGHT >> 25)) &
					 	(unsigned char) (((gSpecialKeys & MT_TAB) >> 13) | ~(MT_TAB >> 13)) &
					 	(unsigned char) (((gSpecialKeys & MT_ESC) >> 13) | ~(MT_ESC >> 13)) &
					 	(unsigned char) (((gSpecialKeys & MT_ENTER) >> 16) | ~(MT_ENTER >> 16));
 	
			keyscan[7] = (unsigned char) ((gSpecialKeys >> 8) & 0x1F);
	
			keyscan[8] = (unsigned char) ((gSpecialKeys & 0x07) | 0xF8) &
					 	(unsigned char) (((gSpecialKeys & MT_CAP_LOCK) | ~MT_CAP_LOCK) >> 1);
		}
	}

	if (((gSpecialKeys & (MT_GRAPH | MT_CODE | MT_SHIFT)) == 0) && !gCapture)
	{
		FILE* fd;
		int		d, col, row;
		gCapture = TRUE;
		
		fd = fopen("lcd_cap.txt", "w");
		if (fd != NULL)
		{
			// Set start page for all drivers to 0
			fprintf(fd, "f 0 3e\n");	
			fprintf(fd, "f 0 3b\n");	

			for (d = 0; d < 10; d++)
			{
				for (row = 0; row < 4; row++)
				{
					// Set the X/Y address
					fprintf(fd, "%x 0 %02x\n", d, row<<6);
					for(col = 0; col < 50; col++)
					{
						fprintf(fd, "%x 1 %02x\n", d, lcd[d][(row<<2) + col]);
					}
				}
			}
			fclose(fd);
		}
	}
	else
		gCapture = FALSE;
	gDelayUpdateKeys = 0;
	gDelayCount = 0;
	handle_wheel_keys();
}

void init_io(void)
{
	int c;

	/* Initialize special keys variable */
	gSpecialKeys = 0xFFFFFFFF;

	for (c = 0; c < 128; c++)
		gKeyStates[c] = 0;

	/* Initialize keyscan variables */
	update_keys();

	/* Initialize serial I/O structures */
	t200_uart_state = 0;			/* Confiture 8152 to Mode state */
	ser_init();
	init_clock();

	/* Setup callback for serial I/O */
	ser_set_callback(cb_int65);

	io21 = 0;
	gPort21Time = 0.0;
	ioA1 = 0;
	io90 = 0;
	ioA1 = 0;
	ioBA = 0;
	ioB9 = 0;
	ioE8 = 0;
	ioBC = 0;		/* Low byte of 14-bit timer */
	ioBD = 0;		/* High byte of 14-bit timer */
	ioD0 = 0;		/* D0-DF io for T200 */

}

void deinit_io(void)
{
	/* Deinitialize the serial port */
	ser_deinit();
}

void show_remem_mode(void)
{
	char	temp[20];

	/* Update Display map if output to Mode Port */
	if (gReMem && !gRex)
	{
		if (inport(REMEM_MODE_PORT) & 0x01)
		{
			sprintf(temp, "Map:%d", (inport(REMEM_MODE_PORT) >> 3) & 0x07);
			display_map_mode(temp);
		}
		else
			display_map_mode("Normal");

		return;
	}

	/* Not in ReMem emulation mode */
	display_map_mode("");
}

void out(uchar port, uchar val)
{
	int		c;
	unsigned char flags;

	switch(port) {
		/* 
		==========================================================
		Extended I/O - Real-time Delay port set value
		==========================================================
		*/
		case 0x21:					/* Delay port */
			io21 = val;
			gPort21Time = hirestimer();
			break;
		case 0x22:
			sound_enable = val;
			break;

		/* 
		==========================================================
		Extended I/O - Debug log
		==========================================================
		*/
		case 0x23:
			break;

		case REMEM_MODE_PORT:		/* ReMem Mode port */
		case REMEM_SECTOR_PORT:		/* ReMem Sector access port */
		case REMEM_DATA_PORT:		/* ReMem Data Port */
		case REMEM_FLASH1_555_PORT:	/* ReMem Flash Command Port */
		case REMEM_FLASH1_AAA_PORT:	/* ReMem Flash Command Port */
		case REMEM_FLASH2_555_PORT:	/* ReMem Flash Command Port */
		case REMEM_FLASH2_AAA_PORT:	/* ReMem Flash Command Port */
		case RAMPAC_SECTOR_PORT:	/* ReMem/RAMPAC emulation port */
		case RAMPAC_DATA_PORT:		/* ReMem RAMPAC emulation port */
		case 0x85:
			remem_out(port, val);
			if (port == REMEM_MODE_PORT)
				show_remem_mode();	/* Update ReMem mode */
			break;

			/*
			I/O Port 0x90 PC-8201a Output Port
			Bit:
			    0 - 
			    1 - 
				2 -
				3 -
			    4 - Clock Chip Data strobe
			    5 - Printer Data Strobe
			6 & 7 - Active Serial Port
			*/
		case 0x90:	/* T200 Clock/Timer chip */
			if (gModel == MODEL_PC8201 || gModel == MODEL_PC8300)
			{
				if ((val & 0x10) && !(io90 & 0x10))
				{
					pd1990ac_chip_cmd(ioB9);
					break;
				}

				/* Check for data to the printer */
				if ((val & 0x20) && !(io90 & 0x20))
					send_to_lpt(ioB9);

				/* Save value of this port */
				io90 = val;
				break;
			}
			/* Fallthrough */
		case 0x91:  
		case 0x92:
		case 0x93:
		case 0x94:
		case 0x95:
		case 0x96:
		case 0x97:
		case 0x98:
		case 0x99:
		case 0x9A:
		case 0x9B:
		case 0x9C:
		case 0x9D:	/* Clock chip Mode Register */
			if (gModel == MODEL_T200)
				rp5c01_write(port, val);
			break;

		case 0x9E:
		case 0x9F:
			break;

		case 0xA0:	/* Modem control port */
			/*
			Bit:
			    0 - Modem telephone relay (1-Modem connected to phone 
				line)
			    1 - Modem enable (1-Modem chip enabled) */
			return;

		case 0xA1:	/* RAM bank port on PC-8201A */
			/* Test for a change to the ROM bank (0x0000 - 0x7FFF) */
			if ((val & 0x0F) == ioA1)
				break;

			if ((val & 0x03) != (ioA1 & 0x03))
			{
				set_rom_bank((unsigned char) (val & 0x03));
			}

			/* Test for a change to the RAM bank (0x8000 - 0xFFFF) */
			if ((val & 0x0C) != (ioA1 & 0x0C))
			{
				/* Convert HW select to bank number */
				c = (val & 0x0C) >> 2;
				if (c > 0)
					c--;

				/* Set bank number */
				set_ram_bank((unsigned char) c);
			}

			ioA1 = val & 0x0F;
			break;

		case 0xA3:	/* PC-8300 ROM Bank #0 Select */
			ioA3 = val & 0x03;
			set_rom0_bank(ioA3);
			break;

		case 0xB0:	/* PIO Command/Status Register */
		case 0xB8:
			/*
			Bit:
			    0 - Direction of Port A (0-input, 1-output)
			    1 - Direction of Port B (0-input, 1-output)
			2 & 3 - Port C definition (00 - All input, 11 - All output, 
				01 - Alt 3, 10 - Alt 4 (see Intel technical sheets 
				for more information))
			    4 - Enable Port A interrupt (1 - enable)
			    5 - Enable Port B interrupt (1 - enable)
			6 & 7 - Timer mode (00 - No effect on counter, 01 - Stop 
				counter immediately, 10 - Stop counter after TC, 11 
				- Start counter) */
			return;

		case 0xB1:	/* 8155 PIO Port A */
		case 0xB9:
			/*
			8 bit data port for printer output, keyboard column strobe,
			and LCD
			In addition, the first 5 bits of this port is used to 
			control the 1990 real time clock chip.  The configuration of 
			these five bits are:
			Bit:
			    0 -  C0
			    1 -  C1
			    2 -  C2
			    3 -  Clock
			    4 -  Serial data into clock chip */
			lcdbits = (lcdbits & 0x0300) | val;

			/* Check for clock chip serial shift clock */
			if ((val & 0x08) && !(ioB9 & 0x08))
			{
				/* Call routine to process the CLK pulse */
				pd1990ac_clk_pulse(val);
			}
			ioB9 = val;
			return;

		case 0xB2:	/* 8155 PIO Port B. */
		case 0xBA:
			/*
			Bit:
			    0 - Column 9 select line for keyboard.  This line is
				also used for the CS-28 line of the LCD.
			    1 - CS 29 line of LCD
			    2 - Beep toggle (1-Data from bit 5, 0-Data from 8155 
				timer)
			    3 - Serial toggle (1-Modem, 0-RS232)
			    4 - Software on/off switch for computer
			    5 - Data to beeper if bit 2 set.  Set if bit 2 low.
			    6 - DTR (not) line for RS232
			    7 - RTS (not) line for RS232 */
			lcdbits = (lcdbits & 0x00FF) | ((A & 0x03) << 8);

			/* Check if software "turned the simulator off" */
			if (A & 0x10)
			{
				power_down();
			}

			/* Update COM settings */
			if (gModel != MODEL_T200)
			{
				if ((val & 0xC8) != (ioBA & 0xC8))
				{
					if ((val & 0x08) == 1)
						c = 0;
					else
					{
						flags = 0;
						if ((val & 0x40) == 0)
							flags |= SER_SIGNAL_DTR;
						if ((val & 0x80) == 0)
							flags |= SER_SIGNAL_RTS;
					}
					ser_set_signals(flags);
				}
			}

			if ((ioBA & 0x04) != (A & 0x04))
			{
				if (A & 0x04)
					sound_stop_tone();
				else
				{
					unsigned short div, freq;
					div = ((ioBD & 0x3F) << 8) | ioBC;
					if (div != 0)
						freq = (unsigned short) (2457600L / div);
					else
						freq = 500;
					sound_start_tone(freq);
				}
			}
			if ((ioBA & 0x20) != (A & 0x20))
			{
				sound_toggle_speaker((A & 0x20) >> 5);
			}

			ioBA = A;
			return;

		case 0xB3:	/* 8155 PIO Port C */
		case 0xBB:
			return;

		case 0xB4:	/* 8155 Timer register.  LSB of timer counter */
		case 0xBC:
			ioBC = val;
			return;

		case 0xB5:	/* 8155 Timer register.  MSB of timer counter */
		case 0xBD:
			ioBD = val;
			c = (((int) (ioBD & 0x3F) << 8) | (ioBC));
			if (c != 0)
				c = 153600 / c;
			ser_set_baud(c);
			return;

		case 0xC0:	/* Bidirectional data bus for UART (6402) (C0H-CFH same) */
		case 0xC1:  /* Status/Control bus for T200 */
		case 0xC2:
		case 0xC3:
		case 0xC4:
		case 0xC5:
		case 0xC6:
		case 0xC7:
		case 0xC8:
		case 0xC9:
		case 0xCA:
		case 0xCB:
		case 0xCC:
		case 0xCD:
		case 0xCE:
			if (gModel == MODEL_T200)
			{
				ser_write_byte(val);
				return;
			}
			/* Fallthrough */
		case 0xCF:
			if (gModel == MODEL_T200)
			{
				if (t200_uart_state == 0)
				{
					/* Set Mode state - first check data bits*/
					c = ((val & 0x0C) >> 2) + 5;
					ser_set_bit_size(c);

					/* Set stop bits */
					c = val & 0x80 ? 2 : 1;
					ser_set_stop_bits(c);

					/* Set Parity */
					if (val & 0x10) 
						c = val & 0x20 ? 'E' : 'O';
					else
						c = 'N';
					ser_set_parity((char) c);

					t200_uart_state = 1;
				}
				else
				{
					/* Command state - first checkf for reset operation */
					if (val & 0x40)
					{
						/* Reset the UART */
						t200_uart_state = 0;
						return;
					}

					/* Set UART state based on command */
					flags = 0;
					/* Test for DTR */
					if (val & 0x02)
						flags |= SER_SIGNAL_DTR;

					/* Test for RTS */
					if (val & 0x20)
						flags |= SER_SIGNAL_RTS;

					ser_set_signals(flags);
				}													    
			}
			else
				ser_write_byte(val);
			return;

		case 0xD0:	/* Status control register for UART, modem, and phone (6402)  */
		case 0xD1:
		case 0xD2:
		case 0xD3:
		case 0xD4:
		case 0xD5:
		case 0xD6:
		case 0xD7:
		case 0xD8:
		case 0xD9:
		case 0xDA:
		case 0xDB:
		case 0xDC:
		case 0xDD:
		case 0xDE:
		case 0xDF:
			if (gModel != MODEL_T200)
			{
				/*
				Bits:
					0 - Stop Bits (1-1.5, 0-2)
					1 - Parity (1-even, 0-odd)
					2 - Parity Enable (1-no parity, 0-parity enabled)
					3 - Data length (00-5 bits, 10-6 bits, 01-7 bits, 11-8 
					bits)
					4 - Data length (see bit 3) */

				if ((ioBA & 0x08) == 0)
				{
					/* Set stop bits */
					c = val & 0x01 ? 2 : 1;
					ser_set_stop_bits(c);
  
					/* Set Parity */
					if (val & 0x04) 
						c = 'N';
					else
						c = val & 0x02 ? 'E' : 'O';
					ser_set_parity((char) c);

					/* Set bit size */
					c = ((val & 0x18) >> 3) + 5;
					ser_set_bit_size(c);
				}
			}
			else
			{
				/* ROM and RAM bank selection bits */
				/* Check for a change in the current RAM bank */
				if ((ioD0 & 0x0C) != (val & 0x0C))
				{
					set_ram_bank((unsigned char)((val >> 2) & 0x03));
				}

				/* Check for a change in the current ROM bank */
				if ((ioD0 & 0x03) != (val & 0x03))
				{
					set_rom_bank((uchar) (val & 0x03));
					// Test if in MSPLAN rom for mouse events
					if ((val & 0x03) == 1)
						gInMsPlanROM = 10;
				}

				ioD0 = val;
			}
			return;

		case 0xE0:	/* Keyboard input and misc. device select (E0H-EFH same) */
		case 0xE1:
		case 0xE2:
		case 0xE3:
		case 0xE4:
		case 0xE5:
		case 0xE6:
		case 0xE7:
		case 0xE8:
		case 0xE9:
		case 0xEA:
		case 0xEB:
		case 0xEC:
		case 0xED:
		case 0xEE:
		case 
			0xEF:
			/*
			Bits:
			    0 - ROM select (0-Standard ROM, 1-Option ROM)
			    1 - STROBE (not) signal to printer
			    2 - STROBE for Clock chip (1990)
			    3 - Remote plug control signal */
			if (gModel != MODEL_T200)
			{
				/* Check for change in ROM selection */
				if ((val & 0x01) != gRomBank)
					set_rom_bank((uchar) (val & 0x01));

				/* Check for Clock Chip strobe */
				if ((val & 0x04) && !(ioE8 & 0x04))
				{
					pd1990ac_chip_cmd(ioB9);
				}

				/* Check for data to the printer */
				if ((val & 0x02) && !(ioE8 & 0x02))
					send_to_lpt(ioB9);
			}
			else
			{
				// T200 Printer port STROBE is on Bit 0, not Bit 1
				if ((val & 0x01) && !(ioE8 & 0x01))
					send_to_lpt(ioB9);
			}

			ioE8 = val;
			return;

		case 0xF0:	/* LCD display data bus (F0H-FFH same) */
		case 0xF2:
		case 0xF4:
		case 0xF6:
		case 0xF8:
		case 0xFA:
		case 0xFC:
		case 0xFE:	/* Row select */
			if (gModel != MODEL_T200)
			{
				for (c = 0; c < 10; c++)
				{
					if (lcdbits & (1 << c))
					{
						/* Save page and column info for later use */
						lcdpointers[c]=A;

						/* New pointer loaded - mark it as "fresh" */
						if ((A & 0x3F) < 50)
							lcd_fresh_ptr[c] = 1;
						else
							lcdcommand(c, A);
					}
				}
			} 
			else
			{
				/* Execute the command using the value supplied */
				t200_command(t200_ir, val);
			}
			return;
		case 0xF1:
		case 0xF3:
		case 0xF5:
		case 0xF7:
		case 0xF9:
		case 0xFB:
		case 0xFD:
		case 0xFF:	/* Data output */
			if (gModel != MODEL_T200)
			{
				for (c = 0; c < 10; c++)
				{
					if (lcdbits & (1 << c))
					{
						int maxPixels = 50;
						if ((c == 4) || (c == 9))
							maxPixels = 40;

						if ((lcdpointers[c]&0x3f) < 50)
						{
							/* Save Byte in LCD memory */
							lcd[c][lcdpointers[c]]=A;

							/* Draw the byte on the display */
							if ((lcdpointers[c]&0x3f) < maxPixels)
								drawbyte(c,lcdpointers[c],A);

							/* Update the pointer if */
							lcdpointers[c]++;
							if ((lcdpointers[c] & 0x3F) > 50)
								lcdpointers[c] &= 0xC0;	

							/* We just changed the pointer so it isn't fresh any more */
							lcd_fresh_ptr[c] = 0;
						}
					}
				}
			}
			else
			{
				/* Save the T200 LCD controller Instruction Register */
				if (val == 0x0D)
					t200_command(val, 0);
				else
					t200_ir = val;
			}
			return;
	}
}

int inport(uchar port)
{
	int c;
	unsigned char	 ret;
	unsigned char	flags;
	double			timeNow, timeLeft;

	switch(port) {

		/* Special VirtualT emulation ports */
		case 0x20:
			return 'V';				/* Tell host app it's running on VirtualT */

		case 0x21:					/* Real-time timing support */
			/* If the io21 value is zero, then timeing is done */
			if (io21 == 0)
				return 0;

			/* Get the timeNow so we can test if time has expired */
			timeNow = hirestimer();
			timeLeft = timeNow - gPort21Time;

			/* Convert timeLeft (in ms) to int */
			c = (char) (timeLeft * 1000.0);

			/* Test if time has expired and clear io21 if it has */
			if (c >= io21)
			{
				io21 = 0;
				return 0;
			}

			/* Return number of ms remaining */
			return io21 - c;

		case REMEM_SECTOR_PORT:		/* ReMem Sector access port */
		case REMEM_DATA_PORT:		/* ReMem Data Port */
		case REMEM_MODE_PORT:		/* ReMem Mode port */
		case REMEM_REVID_PORT:		/* ReMem Rev ID */
		case RAMPAC_SECTOR_PORT:	/* ReMem/RAMPAC emulation port */
		case RAMPAC_DATA_PORT:		/* ReMem RAMPAC emulation port */
			if (remem_in(port, &ret))
				return ret;
			else
				return 0;

		case 0x82:  /* Optional IO thinger? */
			return(0xA2);
	
		case 0x90:	/* T200 Clock/Timer chip */
		case 0x91:  
		case 0x92:
		case 0x93:
		case 0x94:
		case 0x95:
		case 0x96:
		case 0x97:
		case 0x98:
		case 0x99:
		case 0x9A:
		case 0x9B:
		case 0x9C:
		case 0x9D:	/* Clock chip Mode Register */
			return rp5c01_read(port);

		case 0x9E:
		case 0x9F:
			return 0;;

		case 0xA0:	/* Modem control port */
			if ((gModel == MODEL_PC8201) || (gModel == MODEL_PC8300))
				return (ioA1 & 0x3F) | (io90 & 0xC0);
			else
				return 0xA0;

		case 0xA1:	/* Bank control port on NEC laptops */
			return ioA1;

		case 0xA3:	/* ROM #0 Bank select for PC-8300 */
			return ioA3;

		case 0xB0:	/* PIO Command/Status Register */
		case 0xB8:
			/*
			Bit:
			    0 - Port A interrupt request
			    1 - Port A buffer full/empty (input/output)
			    2 - Port A interrupt enabled
			    3 - Port B interrupt request
			    4 - Port B buffer full/empty (input/output)
			    5 - Port B interrupt enabled
			    6 - Timer interrupt (status of TC pin)
			    7 - Not used */

			/* Force TC high */
			return(0x89);

		case 0xB1:	/* 8155 PIO Port A */
		case 0xB9:
			return(lcdbits & 0xFF);

		case 0xB2:	/* 8155 PIO Port B. */
		case 0xBA:
			return(ioBA);

		case 0xB3:	/* 8155 PIO Port C */
		case 0xBB:
			/*
			Bits:
			    0 - Serial data input from clock chip
			    1 - Busy (not) signal from printer
			    2 - Busy signal from printer
			    3 - Data from BCR
			    4 - CTS (not) line from RS232
			    5 - DSR (not) line from RS232
			    6-7 - Not avaiable on 8155 */
			if (setup.com_mode != SETUP_COM_NONE)
				ser_get_flags(&flags);
			flags &= SER_FLAG_CTS | SER_FLAG_DSR;
			if (gModel == MODEL_T200)
				flags |= 0x01;				/* Low Power Sense (not) */
			else
				flags |= clock_serial_out;

			// OR the Printer Not Busy bit
			return flags | 0x02;

		case 0xB4:	/* 8155 Timer register.  LSB of timer counter */
		case 0xBC:
			return(0);

		case 0xB5:	/* 8155 Timer register.  MSB of timer counter */
		case 0xBD:
			return(0);

		case 0xC0:	/* Bidirectional data bus for UART (6402) (C0H-CFH same) */
		case 0xC1:
		case 0xC2:
		case 0xC3:
		case 0xC4:
		case 0xC5:
		case 0xC6:
		case 0xC7:
		case 0xC8:
		case 0xC9:
		case 0xCA:
		case 0xCB:
		case 0xCC:
		case 0xCD:
		case 0xCE:
			if (gModel == MODEL_T200)
			{
				ser_read_byte((char *) &ret);
				return ret;
			}
			/* Fallthrough */
		case 0xCF:
			if (gModel == MODEL_T200)
			{
				ret = 0;
				if (ser_get_flags(&flags) == SER_PORT_NOT_OPEN)
				{
					ret = (SER_FLAG_TX_EMPTY << 2) | 1;
				}
				else
				{
					ret |= (flags & (SER_FLAG_OVERRUN | SER_FLAG_FRAME_ERR)) << 3;
					ret |= flags & (SER_FLAG_PARITY_ERR | SER_FLAG_TX_EMPTY);
					ret |= (flags & (SER_FLAG_TX_EMPTY | SER_FLAG_DSR)) << 2;
				}
			}
			else
				ser_read_byte((char *) &ret);
			return ret;

		case 0xD0:	/* Status control register for UART, modem, and phone (6402)  */
		case 0xD1:
		case 0xD2:
		case 0xD3:
		case 0xD4:
		case 0xD5:
		case 0xD6:
		case 0xD7:
		case 0xD8:
		case 0xD9:
		case 0xDA:
		case 0xDB:
		case 0xDC:
		case 0xDD:
		case 0xDE:
		case 0xDF:
			if (gModel != MODEL_T200)
			{
				/*
				Bits:
					0 - Data on telephone line (used to detect carrier)
					1 - Overrun error from UART
					2 - Framing error from UART
					3 - Parity error from UART
					4 - Transmit buffer empty from UART
					5 - Ring line on modem connector
					6 - Not used
					7 - Low Power signal from power supply (LPS not) */

				/* Check if RS-232 is "Muxed in" */
				if ((ioBA & 0x08) == 0)
				{
					flags = 0;
					if (setup.com_mode != SETUP_COM_NONE)
					{
						/* Get flags from serial routines */
						ser_get_flags(&flags);
						flags = (flags & (SER_FLAG_OVERRUN | SER_FLAG_FRAME_ERR | 
							SER_FLAG_PARITY_ERR)) | ((flags & SER_FLAG_TX_EMPTY) 
							<<	4) | ((flags & SER_FLAG_RING) >> 1);
					}
					return flags | 0x80;
				}
				return(0x90);
			}
			else
			{
				return ioD0;
			}

		case 0xE0:	/* Keyboard input and misc. device select (E0H-EFH same) */
		case 0xE1:
		case 0xE2:
		case 0xE3:
		case 0xE4:
		case 0xE5:
		case 0xE6:
		case 0xE7:
		case 0xE8:
			ret = 0xFF;

			/* Read keyboard status -- First check bit 0 of output port 0xBA */
			if ((ioBA & 0x01) == 0)
				//return (unsigned char) (gSpecialKeys & 0xFF);
				return keyscan[8];

			if (ioB9 == 0)
			{
				ret = keyscan[7] & keyscan[6] & keyscan[5] & keyscan[4] &
					   keyscan[3] & keyscan[2] & keyscan[1] & keyscan[0];
				return ret;
			}

			/* Check Bit 7 of port B9 */
			if ((ioB9 & 0x80) == 0)
				return keyscan[7];

			/* Check Bit 6 of port B9 */
			if ((ioB9 & 0x40) == 0)
				return keyscan[6];

			/* Check Bit 5 of port B9 */
			if ((ioB9 & 0x20) == 0)
				return keyscan[5];

			/* Check Bit 4 of port B9 */
			if ((ioB9 & 0x10) == 0)
				return keyscan[4];

			/* Check Bit 3 of port B9 */
			if ((ioB9 & 0x08) == 0)
				return keyscan[3];

			/* Check Bit 2 of port B9 */
			if ((ioB9 & 0x04) == 0)
				return keyscan[2];

			/* Check Bit 1 of port B9 */
			if ((ioB9 & 0x02) == 0)
				return keyscan[1];

			/* Check Bit 0 of port B9 */
			if ((ioB9 & 0x01) == 0)
				return keyscan[0];

			return ret;

		case 0xE9:
		case 0xEA:
		case 0xEB:
		case 0xEC:
		case 0xED:
		case 0xEE:
		case 0xEF:
			/*
			    8 bit data row from keyboard strobe */
			return(0x00);

		case 0xF0:	/* LCD display data bus (F0H-FFH same) */
		case 0xF1:
		case 0xF2:
		case 0xF3:
		case 0xF4:
		case 0xF5:
		case 0xF6:
		case 0xF7:
		case 0xF8:
		case 0xF9:
		case 0xFA:
		case 0xFB:
		case 0xFC:
		case 0xFD:
		case 0xFE:
			if (gModel == MODEL_T200)
				return t200_readport(0xFE);
			else
				return(64);
		case 0xFF:
			/* Loop through all LCD driver modules */
			for (c = 0; c < 10; c++)
			{
				/* Check if this driver is enabled */
				if (lcdbits & (1 << c))
				{
					/* Get the return data from the LCD memory */
					int ret = lcd[c][lcdpointers[c]];

					/* Update the pointer only if it isn't fresh */
					if (!lcd_fresh_ptr[c])
						lcdpointers[c]++;

					/* We just did a read, so pointer no longer fresh */
					lcd_fresh_ptr[c] = 0;
					return ret;
				}
			}
			return(0);

		default:
			return(0);
	}
}

void io_set_ram_bank(unsigned char bank)
{
	if (gModel == MODEL_T200)
	{
		/* Update ioD0 port with bank bits */
		ioD0 = (ioD0 & 3) | ((bank & 3) << 2);
	}
	else if ((gModel == MODEL_PC8201) || (gModel == MODEL_PC8300))
	{
		/* Convert bank number to HW select.  Bank 1 not used in HW */
		if (bank > 0)
			bank++;

		/* Update I/O port with RAM bank info (High address bits) */
		ioA1 = (ioA1 & 3) | ((bank & 3) << 2);
	}
}
