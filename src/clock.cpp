/* clock.cpp */

/* $Id: io.c,v 1.14 2008/09/23 00:03:46 kpettit1 Exp $ */

/*
 * Copyright 2008 Ken Pettit
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
#include <stdlib.h>
#include <string.h>
#include <FL/Fl.H>
#include <FL/fl_draw.H>
#include <FL/Fl_Window.H>
#include <FL/Fl_Preferences.H>
#include <FL/Fl_Round_Button.H>
#include <FL/Fl_Check_Button.H>
#include <FL/Fl_Box.H>

#include "VirtualT.h"
#include "gen_defs.h"
#include "roms.h"
#include "io.h"
#include "setup.h"
#include "m100emu.h"
#include "memory.h"
#include "clock.h"

uchar			gClockMode = 255;
uchar			gRp5c01_mode;
uchar			gRp5c01_data[4][13];
time_t			clock_time = 0;
time_t			last_clock_time = 1;
time_t			gStartTime = 0;
time_t			gRefTime = 0;
uchar			clock_sr[5];			/* 40 Bit shift register */
uchar			clock_sr_save[5];		/* 40 Bit shift register backup storage */
uchar			clock_sr_index = 0;
int				gClockTimingMode;
int				gTimeElapse;
int				gReload;
clock_ctrl_t	gClockCtrl;
extern RomDescription_t	*gStdRomDesc;
extern	Fl_Preferences virtualt_prefs;

extern "C"
{
struct	tm		*mytime	;
uchar			clock_serial_out = 0;

void init_clock(void)
{
	/* Do clock initialization stuff */
}

/*
===================================================================
Handle commands written to the uPD1990AC clock chip for models
other than T200.
===================================================================
*/
void pd1990ac_chip_cmd(uchar val)
{
	static int	clk_cnt = 1;
	time_t		time_delta;
	struct	tm	when;
	int			x;

	gClockMode = val & 0x07;

	/* Clock chip command strobe */
	switch (gClockMode)
	{
	case 0:		/* NOP */
		break;
	case 1:		/* Serial Shift */
		clock_serial_out = clock_sr[0] & 0x01;
//		clock_sr_index = 0;
		break;
	case 2:		/* Write Clock chip */
		// Reset the start time
		gStartTime = time(&gStartTime);
		mytime = localtime(&gStartTime);

		/* Update Clock Chip Shift register Seconds */
		when.tm_sec = (clock_sr[0]& 0x0F) + (clock_sr[0] >> 4) * 10;

		/* Minutes */
		when.tm_min = (clock_sr[1] & 0x0F) + (clock_sr[1] >> 4) * 10;

		/* Hours */
		when.tm_hour = (clock_sr[2] & 0x0F) + (clock_sr[2] >> 4) * 10;

		/* Day of month */
		when.tm_mday = (clock_sr[3] & 0x0F) + (clock_sr[3] >> 4) * 10;

		/* Day of week */
		when.tm_wday = clock_sr[4] & 0x0F;

		/* Month */
		when.tm_mon = (clock_sr[4] >> 4) - 1;

		when.tm_isdst = -1;
		when.tm_year = mytime->tm_year;
		
		gRefTime = mktime(&when);

		for (x = 0; x < 5; x++)
			clock_sr_save[x] = clock_sr[x];

		break;
	case 3:		/* Read clock chip */
		for (x = 0; x < 5; x++)
			clock_sr[x] = clock_sr_save[x];

		// Get current time and test if it changed from the last check
		clock_time = time(&clock_time);
		if (clock_time == last_clock_time)
			break;

		// Calculate the time delta from start of program
		if (gStartTime == 0)
		{
			gStartTime = clock_time;
			gRefTime = clock_time;
		}
		time_delta = clock_time - gStartTime;
		last_clock_time = clock_time;
		clock_time = gRefTime + time_delta;
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

		for (x = 0; x < 5; x++)
			clock_sr_save[x] = clock_sr[x];
//		clock_sr_index = 0;

		if (gModel == MODEL_PC8201)
		{
			if ((get_memory8(gStdRomDesc->sYear+1) == '8') && (get_memory8(gStdRomDesc->sYear) == '3'))
			{
				set_memory8(gStdRomDesc->sYear, (unsigned char) (mytime->tm_year % 10) + '0');
				set_memory8(gStdRomDesc->sYear+1, (unsigned char) ((mytime->tm_year % 100) / 10) + '0');
			}
		}
		else
		{
			if ((get_memory8(gStdRomDesc->sYear+1) == 0) && (get_memory8(gStdRomDesc->sYear) == 0))
			{
				set_memory8(gStdRomDesc->sYear, (unsigned char) (mytime->tm_year % 10));
				set_memory8(gStdRomDesc->sYear+1, (unsigned char) ((mytime->tm_year % 100) / 10));
			}
		}
	}
}

/*
===================================================================
Handle writes to the T200 clock chip, RP5C01.
===================================================================
*/
void rp5c01_write(uchar port, uchar val)
{
	struct tm	when;

	/* Test if writing to Mode Register */
	if (port == 0x9D)	
	{
		if (gModel == MODEL_T200)
			gRp5c01_mode = val;
	}
	else
	{
		/* Save write data to array */
		gRp5c01_data[gRp5c01_mode & 0x03][port & 0x0F] = val;

		/* Check if the time is being set */
		if ((gRp5c01_mode & 3) == 0)
		{
			// Check if the last byte is being written and update
			// the gStartTime if it is using the new time data
			if (port == 0x9C)
			{
				/* Update Clock Chip Shift register Seconds */
				when.tm_sec = gRp5c01_data[0][0]+ gRp5c01_data[0][1] * 10;;

				/* Minutes */
				when.tm_min = gRp5c01_data[0][2] + gRp5c01_data[0][3] * 10;

				/* Hours */
				/* Test if we are in 12 or 24 hour mode */
				if (gRp5c01_data[1][10] == 1)
				{
					when.tm_hour = gRp5c01_data[0][4] + gRp5c01_data[0][5] * 10;
				}
				else
				{
					when.tm_hour = gRp5c01_data[0][4] + (gRp5c01_data[0][5] & 0x01) * 10 +
						(gRp5c01_data[0][5] & 0x02) ? 12 : 0;
				}

				/* Day of week */
				when.tm_wday = gRp5c01_data[0][6];

				/* Day of month */
				when.tm_mday = gRp5c01_data[0][7] + gRp5c01_data[0][8] * 10;

				/* Month */
				when.tm_mon = (gRp5c01_data[0][9] + gRp5c01_data[0][10] * 10)-1;

				/* Year */
				when.tm_year = gRp5c01_data[0][11] + gRp5c01_data[0][12] * 10 + 100;

				when.tm_isdst = -1;
				when.tm_yday = 1;

				gRefTime = mktime(&when);
			}
			/* Add code here to update the system time */
		}
		/* Check if the alarm is being set */
		else if ((gRp5c01_mode& 3) == 1)
		{
		}
	}
}

/*
===================================================================
Handle reads from the T200 clock chip, RP5C01.
===================================================================
*/
uchar rp5c01_read(uchar port)
{
	time_t		time_delta;
	uchar		ret = 0;

	if (gModel == MODEL_T200)
	{
		/* Test for clock Mode Register */
		if (port == 0x9D)
			return gRp5c01_mode;

		/* Check if the time is being set */
		if ((gRp5c01_mode & 3) == 0)
		{
			/* Read system clock and update "chip" */
			if (port == 0x90)
			{
				clock_time = time(&clock_time);
				if (clock_time != last_clock_time)
				{
					// Check if we just started up
					if (gStartTime == 0)
					{
						gStartTime = clock_time;
						gRefTime = clock_time;
					}

					// Calculated emulated time
					last_clock_time = clock_time;
					time_delta = clock_time - gStartTime;
					clock_time = gRefTime + time_delta;
					mytime = localtime(&clock_time);
					
					/* Update Clock Chip Shift register Seconds */
					gRp5c01_data[0][0] = mytime->tm_sec % 10;
					gRp5c01_data[0][1] = mytime->tm_sec / 10;

					/* Minutes */
					gRp5c01_data[0][2] = mytime->tm_min % 10;
					gRp5c01_data[0][3] = mytime->tm_min / 10;

					/* Hours */
					/* Test if we are in 12 or 24 hour mode */
					if (gRp5c01_data[1][10] == 1)
					{
						gRp5c01_data[0][4] = mytime->tm_hour % 10;
						gRp5c01_data[0][5] = mytime->tm_hour / 10;
					}
					else
					{
						gRp5c01_data[0][4] = (mytime->tm_hour % 12) % 10;
						gRp5c01_data[0][5] = (mytime->tm_hour % 12) / 10;
						if (mytime->tm_hour >=12)
							gRp5c01_data[0][5] |= 2;
					}

					/* Day of week */
					gRp5c01_data[0][6] = mytime->tm_wday;

					/* Day of month */
					gRp5c01_data[0][7] = mytime->tm_mday % 10;
					gRp5c01_data[0][8] = mytime->tm_mday / 10;

					/* Month */
					gRp5c01_data[0][9] = (mytime->tm_mon+1) % 10;
					gRp5c01_data[0][10] = (mytime->tm_mon+1) / 10;

					/* Year */
					gRp5c01_data[0][11] = mytime->tm_year % 10;
					gRp5c01_data[0][12] = (mytime->tm_year % 100) / 10;
				}
			}
		}
		/* Check if the alarm is being set */
		else if ((gRp5c01_mode & 3) == 1)
		{
		}

		/* Return data stored in the RP5C01 "chip" */
		ret = gRp5c01_data[gRp5c01_mode & 0x03][port & 0x0F];
	}

	return ret;
}

/*
===================================================================
Handle a pulse on the chip's CLK pin.  This causes the shift
register to shift 1 bit if in the Shift Mode.
===================================================================
*/
void pd1990ac_clk_pulse(uchar val)
{
	if (gClockMode == 1)
	{
		/* Perform a shift operation */
		clock_sr[0] >>= 1;
		clock_sr[0] |= clock_sr[1] & 1 ? 0x80 : 0;
		clock_sr[1] >>= 1;
		clock_sr[1] |= clock_sr[2] & 1 ? 0x80 : 0;
		clock_sr[2] >>= 1;
		clock_sr[2] |= clock_sr[3] & 1 ? 0x80 : 0;
		clock_sr[3] >>= 1;
		clock_sr[3] |= clock_sr[4] & 1 ? 0x80 : 0;
		clock_sr[4] >>= 1;
		clock_sr[4] |= val & 0x10 ? 0x80 : 0;
		clock_serial_out = clock_sr[0] & 1;
	}
}

/*
=============================================================================
save_model_time:	Saves the emulated time for the current model.
=============================================================================
*/
void save_model_time(void)
{
	char	str[16];
	char	pref[64];
	time_t	now;

	get_model_string(str, gModel);

	// Save Reference Time
	strcpy(pref, str);
	strcat(pref, "_RefTime");
	virtualt_prefs.set(pref, ctime(&gRefTime));
	printf(ctime(&gRefTime));

	// Save Start Time
	strcpy(pref, str);
	strcat(pref, "_StartTime");
	virtualt_prefs.set(pref, ctime(&gStartTime));

	// Save Suspend Time
	now = time(&now);
	strcpy(pref, str);
	strcat(pref, "_SuspendTime");
	virtualt_prefs.set(pref, ctime(&now));
}

/*
=============================================================================
convert_ctime:	Convert from string time to time_t.
=============================================================================
*/
char *gMonths[12] = { "Jan", "Feb", "Mar", "Apr", "May", "Jun", 
							"Jul", "Aug", "Sep", "Oct", "Nov", "Dec" };
time_t convert_ctime(char* stime)
{
	char	month[5];			// Storage for month
	struct	tm	tm_time;		// Used to build time_t
	int		x;
	time_t	convTime;			// Converted time

	// If the string is blank, return current time
	if ((strlen(stime) == 0) || (strlen(stime) < 24))
	{
		convTime = time(&convTime);
		return convTime;
	}

	// Convert month from string to integer
	tm_time.tm_mon = 0;
	strncpy(month, &stime[4], 3);		// Get month
	month[3] = 0;
	for (x = 0; x < 12; x++)
	{
		if (strcmp(month, gMonths[x]) == 0)
		{
			// Save month in tm struct
			tm_time.tm_mon = x;
			break;
		}
	}
	
	tm_time.tm_mday = atoi(&stime[8]);		// Convert day
	tm_time.tm_hour = atoi(&stime[11]);		// Convert hours
	tm_time.tm_min = atoi(&stime[14]);		// Convert minutes
	tm_time.tm_sec = atoi(&stime[17]);		// Convert seconds
	tm_time.tm_year = atoi(&stime[20]);		// Convert year
	tm_time.tm_year -= 1900;

	// Convert to time_t format
	convTime = mktime(&tm_time);
	return convTime;
}

/*
=============================================================================
get_model_time:		Gets the emulated time for the current model.
=============================================================================
*/
void get_model_time(void)
{
	char	str[16];
	char	pref[64];
	char	stringtime[30];
	time_t	now, suspendTime;

	get_model_string(str, gModel);

	// Save Reference Time
	strcpy(pref, str);
	strcat(pref, "_RefTime");
	virtualt_prefs.get(pref, stringtime, "", 30);
	gRefTime = convert_ctime(stringtime);

	// Save Start Time
	strcpy(pref, str);
	strcat(pref, "_StartTime");
	virtualt_prefs.get(pref, stringtime, "", 30);
	gStartTime = convert_ctime(stringtime);

	// Save Suspend Time
	strcpy(pref, str);
	strcat(pref, "_SuspendTime");
	virtualt_prefs.get(pref, stringtime, "", 30);
	suspendTime = convert_ctime(stringtime);

	// Set the Emulation time base on preferences
	now = time(&now);
	if (gClockTimingMode == CLOCK_MODE_SYS)
	{
		// Reset the clock to current time
		if (gReload)
		{
			gRefTime = now;
			gStartTime = now;
			clock_time = now;
		}
	}
	else
	{
		// Use emulated time.  Test if time elapses during suspend
		if (!gTimeElapse)
		{
			// Time doesn't elapse during suspend.  Therefore we must 
			// update the gStartTime variable to add in the elapsed time
			// so the calculation now - gStartTime won't reflect the
			// actual elapsed time since the last suspension
			time_t delta = suspendTime - gStartTime;
			gStartTime = now - delta;
		}
	}
}

}		// End of Extern "C" items

/*
=======================================================
Callback routines for radio butons
=======================================================
*/
void cb_clock_radio_sys(Fl_Widget* w, void*)
{
	gClockCtrl.pReload->activate();
	gClockCtrl.pTimeElapse->deactivate();
}

/*
=======================================================
Callback routines for radio butons
=======================================================
*/
void cb_clock_radio_emul(Fl_Widget* w, void*)
{
	gClockCtrl.pReload->deactivate();
	gClockCtrl.pTimeElapse->activate();
}

/*
=======================================================
Build the CLOCK tab on the Peripheral Setup dialog
=======================================================
*/
void build_clock_setup_tab(void)
{
	Fl_Box* pText = new Fl_Box(20, 10, 60, 80, "Clock operation upon power-up");
	pText->align(FL_ALIGN_LEFT|FL_ALIGN_INSIDE);

	// Create Radio button for "Host Time"
	gClockCtrl.pSysTime = new Fl_Round_Button(20, 70, 200, 20, "Reset to Host Time");
	gClockCtrl.pSysTime->type(FL_RADIO_BUTTON);
	gClockCtrl.pSysTime->callback(cb_clock_radio_sys);

	gClockCtrl.pReload = new Fl_Check_Button(45, 95, 250, 20, "Reset time when changing models");
	gClockCtrl.pReload->value(gReload);
	if (gClockTimingMode != CLOCK_MODE_SYS)
		gClockCtrl.pReload->deactivate();

	// Create Radio button for Model Clock Time
	gClockCtrl.pEmulTime = new Fl_Round_Button(20, 130, 200, 20, "Restore Emulated Time");
	gClockCtrl.pEmulTime->type(FL_RADIO_BUTTON);
	gClockCtrl.pEmulTime->callback(cb_clock_radio_emul);

	gClockCtrl.pTimeElapse = new Fl_Check_Button(45, 155, 270, 20, "Time elapses while emulation stopped");
	gClockCtrl.pTimeElapse->value(gTimeElapse);
	if (gClockTimingMode != CLOCK_MODE_EMUL)
		gClockCtrl.pTimeElapse->deactivate();

	// Select the correct radio button base on preferences
	if (gClockTimingMode == CLOCK_MODE_SYS)
		gClockCtrl.pSysTime->value(1);
	else if (gClockTimingMode == CLOCK_MODE_EMUL)
		gClockCtrl.pEmulTime->value(1);
}

/*
=============================================================================
load_clock_preferences:	Loads the emulated printer preferences from the Host.
=============================================================================
*/
void load_clock_preferences(Fl_Preferences* pPref)
{
	// Load the preferences
	pPref->get("ClockTimingMode", gClockTimingMode, CLOCK_MODE_SYS);
	pPref->get("ClockReload", gReload, 1);
	pPref->get("ClockTimeElapse", gTimeElapse, 0);
}

/*
=============================================================================
save_clock_preferences:	Saves the emulated printer preferences to the Host.
=============================================================================
*/
void save_clock_preferences(Fl_Preferences* pPref)
{
	pPref->set("ClockTimingMode", gClockTimingMode);
	pPref->set("ClockReload", gReload);
	pPref->set("ClockTimeElapse", gTimeElapse);
}


/*
=============================================================================
get_clock_options:	Gets the LPT options from the Dialog Tab and saves them
					to the preferences.  This routien is called when the
					user selects "Ok".
=============================================================================
*/
void get_clock_options()
{
	// End any active print sessions
	// Check if Timing Mode is "System Timing"
	if (gClockCtrl.pSysTime->value())
		gClockTimingMode = CLOCK_MODE_SYS;
	else
		gClockTimingMode = CLOCK_MODE_EMUL;

	// Check if Reload option is selectd
	gReload = gClockCtrl.pReload->value();

	// Check if Time Elapse if selected
	gTimeElapse = gClockCtrl.pTimeElapse->value();
}

