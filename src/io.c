/* io.c */

/* $Id: io.c,v 1.1.1.1 2004/08/05 06:46:12 deuce Exp $ */

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
#include <stdio.h>

#include "cpu.h"
#include "gen_defs.h"
#include "io.h"
#include "serial.h"
#include "display.h"
#include "setup.h"
#include "m100emu.h"

uchar lcd[10][256];
uchar lcdpointers[10]={0,0,0,0,0,0,0,0,0,0};
uchar lcddriver=0;
uchar lcd_fresh_ptr[10] = {1,1,1,1,1,1,1,1,1,1 };
uchar ioBA;
uchar ioB9;
uchar ioE8;
uchar ioBC;		/* Low byte of 14-bit timer */
uchar ioBD;		/* High byte of 14-bit timer */


uchar keyscan[9] = { 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF};

uint  lcdbits=0;
time_t	clock_time = 0;
time_t	last_clock_time = 1;
struct	tm	*mytime	;

unsigned long	gSpecialKeys = 0;
unsigned	char	gKeyStates[128];
int				gDelayUpdateKeys = 0;
int				gDelayCount = 0;
int				gModel = MODEL_M100;

uchar clock_sr[5];			/* 40 Bit shift register */
uchar clock_sr_index = 0;
uchar clock_serial_out = 0;

char keyin[9];

void update_keys(void)
{
	int equal = 1;
	int buf_index = 0xFF9A;
	int c;
	uchar key;

	/* Insure keystroke was recgonized by the system */
	for (c = 0; c < 9; c++)
	{
		key = (uchar) ((~keyscan[c]) & 0xFF);
		/* Old keyscan matrix must match buffer in memory */
		if (memory[buf_index] != key)
		{
			equal = 0;
			break;
		}
		buf_index++;
	}

	if (!equal)
	{
		if (++gDelayCount < 20)
		{
			gDelayUpdateKeys = 1;
			return;
		}
	}

	gDelayUpdateKeys = 0;
	gDelayCount = 0;
	
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

	keyscan[3] = ~(gKeyStates['o']       | (gKeyStates['p'] << 1) |
				  (gKeyStates['['] << 2) | (gKeyStates[';'] << 3) |
				  (gKeyStates['\''] << 4) | (gKeyStates[','] << 5) |
				  (gKeyStates['.'] << 6) | (gKeyStates['/'] << 7));

	keyscan[4] = ~(gKeyStates['1']       | (gKeyStates['2'] << 1) |
				  (gKeyStates['3'] << 2) | (gKeyStates['4'] << 3) |
				  (gKeyStates['5'] << 4) | (gKeyStates['6'] << 5) |
				  (gKeyStates['7'] << 6) | (gKeyStates['8'] << 7));

	keyscan[5] = ~(gKeyStates['9']       | (gKeyStates['0'] << 1) |
				  (gKeyStates['-'] << 2) | (gKeyStates['='] << 3)) &
				  (unsigned char) (gSpecialKeys >> 24);

	keyscan[6] = (unsigned char) ((gSpecialKeys >> 16) & 0xFF);

	keyscan[7] = (unsigned char) ((gSpecialKeys >> 8) & 0xFF);

	keyscan[8] = (unsigned char) (gSpecialKeys & 0xFF);
}

void init_io(void)
{
	int c;

	// Initialize special keys variable
	gSpecialKeys = 0xFFFFFFFF;

	for (c = 0; c < 128; c++)
		gKeyStates[c] = 0;

	// Initialize keyscan variables
	update_keys();

	// Initialize serial I/O structures
	ser_init();

	// Setup callback for serial I/O
	ser_set_callback(cb_int65);
}

void deinit_io(void)
{
	// Deinitialize the serial port
	ser_deinit();
}


int cROM = 0;

void out(uchar port, uchar val)
{
	int		c;
	unsigned char flags;
	static int	clk_cnt = 20;

	switch(port) {
		case 0xA0:	/* Modem control port */
			/*
			Bit:
			    0 - Modem telephone relay (1-Modem connected to phone 
				line)
			    1 - Modem enable (1-Modem chip enabled) */
			return;

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

				/* Increment shift register index */
				clock_sr_index++;

				/* Update clock chip output bit based on index */
				clock_serial_out = clock_sr[clock_sr_index / 8] & (0x01 << clock_sr_index % 8) ? 1 : 0;

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

			// Update COM settings
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
		case 0xCF:
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
				// Set stop bits
				c = val & 0x01 ? 2 : 1;
				ser_set_stop_bits(c);
  
				// Set Parity
				if (val & 0x04) 
					c = 'N';
				else
					c = val & 0x02 ? 'E' : 'O';
				ser_set_parity((char) c);

				// Set bit size
				c = ((val & 0x18) >> 3) + 5;
				ser_set_bit_size(c);
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
		case 0xEF:
			/*
			Bits:
			    0 - ROM select (0-Standard ROM, 1-Option ROM)
			    1 - STROBE (not) signal to printer
			    2 - STROBE for Clock chip (1990)
			    3 - Remote plug control signal */
			if(val&0x01) {
				if(cROM!=1) {
					memcpy(memory,optROM,ROMSIZE);
					cROM=1;
				}
			}
			else {
				if(cROM!=0) {
					memcpy(memory,sysROM,ROMSIZE);
					cROM=0;
				}
			}
			if ((val & 0x04) && !(ioE8 & 0x04))
			{
				/* Clock chip command strobe */
				switch (ioB9 & 0x07)
				{
				case 0:		/* NOP */
					break;
				case 1:		/* Serial Shift */
					clock_serial_out = clock_sr[0] & 0x01;
					clock_sr_index = 0;
					break;
				case 2:		/* Write Clock chip */
					clock_sr_index = 0;
					break;
				case 3:		/* Read clock chip */
					if (--clk_cnt > 0)
						break;

					clk_cnt = 20;

					clock_time = time(&clock_time);
					if (clock_time == last_clock_time)
						break;

					last_clock_time = clock_time;
					mytime = localtime(&clock_time);
					
					/* Update Clock Chip Shift register Seconds */
					clock_sr[0] = mytime->tm_sec % 10;
					clock_sr[0] |= (mytime->tm_sec / 10) << 4;

					/* Minutes */
					clock_sr[1] = mytime->tm_min % 10;
					clock_sr[1] |= (mytime->tm_min / 10) << 4;

					/* Hours */
					clock_sr[2] = mytime->tm_hour % 10;
					clock_sr[2] |= (mytime->tm_hour / 10) << 4;

					/* Day of month */
					clock_sr[3] = mytime->tm_mday % 10;
					clock_sr[3] |= (mytime->tm_mday / 10) << 4;

					/* Day of week */
					clock_sr[4] = (mytime->tm_wday) % 10;

					/* Month */
					clock_sr[4] |= (mytime->tm_mon + 1) << 4;

					clock_sr_index = 0;

					if ((memory[0xF92E] == 0) && (memory[0xF92D] == 0))
					{
						memory[0xF92D] = mytime->tm_year % 10;
						memory[0xF92E] = (mytime->tm_year % 100) / 10;
					}
					break;
				}
			}

			ioE8 = val;

			return;

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
		case 0xFE:	/* Row select */
			for (c = 0; c < 10; c++)
			{
				if (lcdbits & (1 << c))
				{
					/* Save page and column info for later use */
					lcdpointers[c]=A;

					/* New pointer loaded - mark it as "fresh" */
					if ((A & 0x3F) < 50)
						lcd_fresh_ptr[c] = 1;
				}
			}
			return;
		case 0xFF:	/* Data output */
			for (c = 0; c < 10; c++)
			{
				if (lcdbits & (1 << c))
				{
					if ((lcdpointers[c]&0x3f) < 50)
					{
						/* Save Byte in LCD memory */
						lcd[c][lcdpointers[c]]=A;

						/* Draw the byte on the display */
						drawbyte(c,lcdpointers[c],A);

						/* Update the pointer if */
						lcdpointers[c]++;
						if ((lcdpointers[c] & 0x3F) >= 50)
							lcdpointers[c] &= 0xC0;	

						/* We just changed the pointer so it isn't fresh any more */
						lcd_fresh_ptr[c] = 0;
					}
				}
			}
			return;
	}
}

int inport(uchar port)
{
	int c;
	unsigned char ret;
	unsigned char flags;

	switch(port) {
		case 0x82:  /* Optional IO thinger? */
			return(0xA2);
	
		case 0xA0:	/* Modem control port */
			return(0xA0);

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
			return clock_serial_out | flags;

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
		case 0xCF:
			ser_read_byte(&ret);
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

			// Check if RS-232 is "Muxed in"
			if ((ioBA & 0x08) == 0)
			{
				flags = 0;
				if (setup.com_mode != SETUP_COM_NONE)
				{
					// Get flags from serial routines
					ser_get_flags(&flags);
					flags = (flags & (SER_FLAG_OVERRUN | SER_FLAG_FRAME_ERR | 
						SER_FLAG_PARITY_ERR)) | ((flags & SER_FLAG_TX_EMPTY) 
						<<	4) | ((flags & SER_FLAG_RING) >> 1);
				}
				return flags | 0x80;
			}
			return(0x90);

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
				return (unsigned char) (gSpecialKeys & 0xFF);

			if (ioB9 == 0)
			{
				return keyscan[7] & keyscan[6] & keyscan[5] & keyscan[4] &
					   keyscan[3] & keyscan[2] & keyscan[1] & keyscan[0];
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
