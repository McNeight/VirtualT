/* sound.c */

/* $Id: sound.c,v 1.9 2011/07/11 06:17:23 kpettit1 Exp $ */

/*
 * Copyright 2005 Ken Pettit
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

//#define _MT
#include <sys/types.h>
#include <fcntl.h>
#include <stdio.h>
#include <string.h>
#include <math.h>
#include <stdlib.h>

#ifdef _WIN32
#include <windows.h>
#include <mmsystem.h>
#include <tchar.h>
#include <process.h>
#endif

#if defined(__unix__) || defined(__APPLE__)
#include <signal.h>
#include <unistd.h>
#endif

#include "VirtualT.h"
#include "sound.h"
#include "m100emu.h"

#pragma comment(lib, "winmm.lib")

#define				BLOCK_SIZE				1024
#define				BLOCK_COUNT				10
#define				SAMPLING_RATE			22050
#define				NUM_CHANNELS			2
#define				BITS_PER_SAMPLE			16
#define				MAX_TONE_CYCLES			5000000
#define				TONE_TRANSITION_SAMPLES	16
#define				DECAY_MAX_LEVEL			4.5

#define				TONE_STOPPED	0
#define				TONE_START		1
#define				TONE_PLAYING	2
#define				TONE_STOP		3

/* Extern data */
extern	UINT64			cycles;
extern	int				cycle_delta;

/*
 * module static data
 */
static int				gReqFreq[16];
static int				gReqIn = 0;
static int				gReqLastFreq = 0;

#ifdef _WIN32
static int				gReqOut = 0;
#endif

unsigned short			gToneBuf[BLOCK_SIZE >> 1] ;          /* input buffer */
unsigned short			gpOneHertz[SAMPLING_RATE];
static int				gPlayTone = TONE_STOPPED;
static int				gToneFreq = 0;
static int				gExit = 0;
static int				gBeepOn = 0;
static int				gOneHzPtr = 0;
static double			gToneDivisor = 1.0;
static double			gDecayLevel = DECAY_MAX_LEVEL;
static double			gDecayStep = 0.008;
static int				gLastToneFreq = 0;
static	UINT64			spkr_cycle = 0;
static	UINT64			gPlayCycle = 0;
int						sound_enable = 1;

#ifdef _WIN32

static HANDLE			g_hEquThread;
static HANDLE			gSoundEvent;
static DWORD			dwThreadID;
static WAVEFORMATEX		wf ;
static HWAVEOUT			hOutput;

/*
* function prototypes
*/ 
static void CALLBACK	sound_waveout_proc(HWAVEOUT, UINT, DWORD, DWORD, DWORD);
static WAVEHDR*			sound_allocate_blocks(int size, int count);
static void				sound_free_blocks(WAVEHDR* blockArray);
static void				sound_write_audio(HWAVEOUT hWaveOut, LPSTR data, int size);
static void				sound_flush_buffers(void);

/*
* module level variables
*/
static CRITICAL_SECTION reqCriticalSection;
static CRITICAL_SECTION waveCriticalSection;
static WAVEHDR*			waveBlocks;
static volatile int		waveFreeBlockCount;
static int				waveCurrentBlock = 0;
static int				waveLastQueueBlock = 0;
static int				waveLastProcBlock = 0;
#endif

#ifdef _WIN32

/*
========================================================================
sound_waveout_proc:	Callback routine called when a buffer is done being
					played.
========================================================================
*/
static void CALLBACK sound_waveout_proc(HWAVEOUT hWaveOut, UINT uMsg, 
	DWORD dwInstance, DWORD dwParam1, DWORD dwParam2)
{
    /* pointer to free block counter */
    int* freeBlockCounter = (int*)dwInstance;

    /*
    * ignore calls that occur due to openining and closing the
    * device.
    */
    if((uMsg != WOM_DONE) || (dwInstance == 0))
		return;

	/* Keep track of the last block processed by the audio system */
    EnterCriticalSection(&waveCriticalSection);
    (*freeBlockCounter)++;
    LeaveCriticalSection(&waveCriticalSection);
}

/*
========================================================================
sound_allocate_blocks:	Allocate from heap for all sound buffers
========================================================================
*/
WAVEHDR* sound_allocate_blocks(int size, int count)
{
    unsigned char* buffer;
    int i;
    WAVEHDR* blocks;
    DWORD totalBufferSize = (size + sizeof(WAVEHDR)) * count;

    /* allocate memory for the entire set in one go */
    if((buffer = HeapAlloc(GetProcessHeap(), HEAP_ZERO_MEMORY, 
	    totalBufferSize)) == NULL) 
	{
        fprintf(stderr, "Memory allocation error\n");
        return NULL;
    }

    /* Setup the pointers to each block */
    blocks = (WAVEHDR*) buffer;
    buffer += sizeof(WAVEHDR) * count;
    for (i = 0; i < count; i++) 
	{
        blocks[i].dwBufferLength = size;
        blocks[i].lpData = buffer;
        buffer += size;
    }

    return blocks;
}

/*
========================================================================
sound_free_blocks:	Free the heap for all sound buffers / headers
========================================================================
*/
void sound_free_blocks(WAVEHDR* blockArray)
{
    /* Free the memory for all blocks */ 
    HeapFree(GetProcessHeap(), 0, blockArray);
}

/*
========================================================================
sound_write_audio:	Writes a block of sound data to the audio device
========================================================================
*/
void sound_write_audio(HWAVEOUT hWaveOut, LPSTR data, int size)
{
    WAVEHDR* current;
    int remain;
    current = &waveBlocks[waveCurrentBlock];
	current->dwBufferLength = BLOCK_SIZE;

    while (size > 0) 
	{
        /* 
        * first make sure the header we're going to use is unprepared
        */
        if (current->dwFlags & WHDR_PREPARED)
			waveOutUnprepareHeader(hWaveOut, current, sizeof(WAVEHDR));

		remain = size < BLOCK_SIZE ? size : BLOCK_SIZE;
		memcpy(current->lpData, data, BLOCK_SIZE);
        current->dwBufferLength = BLOCK_SIZE;
        size -= remain;
        data += remain;
		waveOutPrepareHeader(hWaveOut, current, sizeof(WAVEHDR));	
        waveOutWrite(hWaveOut, current, sizeof(WAVEHDR));
        EnterCriticalSection(&waveCriticalSection);
        waveFreeBlockCount--;
        LeaveCriticalSection(&waveCriticalSection);

		/* wait for a block to become free */
        while(!waveFreeBlockCount)
			Sleep(3);

        EnterCriticalSection(&waveCriticalSection);
        LeaveCriticalSection(&waveCriticalSection);

        /* point to the next block */
		waveLastQueueBlock = waveCurrentBlock;
        waveCurrentBlock++;
        waveCurrentBlock %= BLOCK_COUNT;
        current = &waveBlocks[waveCurrentBlock];
    }
}

/*
========================================================================
sound_open_output:	Open the output sound device
========================================================================
*/
BOOL sound_open_output(void)
{
	/* Calculate buffer size */
	waveBlocks = sound_allocate_blocks(BLOCK_SIZE, BLOCK_COUNT);
    waveFreeBlockCount = BLOCK_COUNT;
    waveCurrentBlock = 0;
	waveLastProcBlock = 0;
    InitializeCriticalSection(&waveCriticalSection);
    InitializeCriticalSection(&reqCriticalSection);

	/* Prepare to open the Wave Device */
	wf.nSamplesPerSec = SAMPLING_RATE;
	wf.wFormatTag = WAVE_FORMAT_PCM ;
	wf.wBitsPerSample = BITS_PER_SAMPLE; 
	wf.nChannels = NUM_CHANNELS;
	wf.nBlockAlign = (wf.wBitsPerSample * wf.nChannels) >> 3;
	wf.nAvgBytesPerSec = wf.nSamplesPerSec * wf.nBlockAlign ;

	/* Try to open the Wave output device */
	waveOutOpen (&hOutput, WAVE_MAPPER, &wf, (DWORD) sound_waveout_proc, 
		(DWORD_PTR) &waveFreeBlockCount, CALLBACK_FUNCTION) ;
	if (hOutput == NULL) 
		return FALSE ;

	return TRUE ;
}

/*
========================================================================
sound_close_output:	Close the output device and free buffer memory.
========================================================================
*/
BOOL sound_close_output (void)
{
	/* Ensure sound device was opened */
	if (hOutput == NULL) 
		return FALSE ;

	/* Wait for all buffers to finish playing */
	sound_flush_buffers();

	/* Now reset and close the output device and free buffers */
	waveOutReset (hOutput);
	sound_free_blocks(waveBlocks);
	waveOutClose (hOutput) ;
	hOutput = NULL ;

	return TRUE ;
}

void sound_reset_output(void)
{
	DWORD		volume;

	/* Before resetting the output device cold turkey, we need to
	   reduce the volume in an "Attack" fassion so we don't get
	   clicks on the output
    */
	waveOutGetVolume(hOutput,&volume);
	waveOutSetVolume(hOutput, 0);
	if (hOutput != NULL)
		waveOutReset (hOutput);
	gOneHzPtr = 0;

	/* Restore the original volume */
	waveOutSetVolume(hOutput, volume);
}

/*
========================================================================
sound_flush_buffers:	Waits for all buffers to be played then returnss
========================================================================
*/
void sound_flush_buffers (void)
{ 
	/* Wait for all buffers to be done */
	while(waveFreeBlockCount < BLOCK_COUNT)
		Sleep(10);
}

/*
========================================================================
sound_play_tome:	Creates and plays a tone of the frequency specified
					in the global variable gToneFreq.
========================================================================
*/
void sound_play_tone(int tone, int toneFreq)
{
    int			i;
	const int	samples = BLOCK_SIZE >> 1;
	const int	decay_time = samples;
	double		divisor = (double) decay_time / (gToneDivisor);
	double		toneStep = (double) (gToneFreq - gLastToneFreq) / (double) TONE_TRANSITION_SAMPLES;
	int			stepCount = 0;
	
	/* Test for runaway tones */
	if (cycles - gPlayCycle > MAX_TONE_CYCLES)
	{
		/* Shut it down */
		tone = TONE_STOP;
	}

	/* If we are playing, then play a buffer of size 1024 (512 samples) */
	if (tone == TONE_PLAYING)
	{
		// Loop for all samples and create the buffer
		for (i = 0; i < samples; i += 2)
		{
			// Create next sample in buffer
			gToneBuf[i] = gpOneHertz[gOneHzPtr];
			gToneBuf[i + 1] = gpOneHertz[gOneHzPtr];

			/* Update pointer in the 1 Hz waveform based on frequency */
			if (gToneFreq != gLastToneFreq && gLastToneFreq != 0)
			{
				if (++stepCount >= TONE_TRANSITION_SAMPLES)
				{
					// Set the last tone frequency now that transition is complete
					if (gToneFreq != 0)
						gLastToneFreq = gToneFreq;
					gOneHzPtr += gToneFreq;
				}
				else
				{
					gOneHzPtr += gLastToneFreq + (int) (toneStep * (double) stepCount);
				}
			}
			else
			{
				gOneHzPtr += gToneFreq;
				gLastToneFreq = gToneFreq;
			}
			if (gOneHzPtr >= SAMPLING_RATE)
			{
				// We wrapped the end of the sample buffer
				gOneHzPtr -= SAMPLING_RATE;
			}
		}
	}

	/* Create a buffer with the specified frequency */
	else
	{
		// Loop for all entries in the buffer
		for (i = 0; i < samples; i += 2)
		{
			// Set the tone value
			unsigned short val = (unsigned short) (((double) (signed short) gpOneHertz[gOneHzPtr]) *
				exp(-gDecayLevel));
			gToneBuf[i] = val;
			gToneBuf[i + 1] = val;

			// Test if we are starting a tone.  We ramp-up the volume to prevent ticking
			if (tone == TONE_START)
			{
				// Ramping up means decreasing the DecayLevel to zero
				gDecayLevel -= gDecayStep;
				if (gDecayLevel <= 0.0)
				{
					// Once we reach level zero, we are done "Decaying" and switch to playing
					gDecayLevel = 0.0;
				}
			}

			// Test if we are stopping a tone and ramp it down in volume to prevent ticking
			else if (tone == TONE_STOP)
			{
				// Ramp the tone down.  This mean increasing the decay level until
				// it reaches our defined max value
				gDecayLevel += gDecayStep * 1.75;
			}

			/* Update pointer in the 1 Hz waveform based on frequency */
			if (gToneFreq != gLastToneFreq && gLastToneFreq != 0)
			{
				/* There was a frequency change.  Filter to new frequency */
				if (++stepCount >= TONE_TRANSITION_SAMPLES)
				{
					// Set the last tone frequency now that transition is complete
					if (gToneFreq != 0)
						gLastToneFreq = gToneFreq;
					gOneHzPtr += gToneFreq;
				}
				else
				{
					/* Change to new frequency in steps */
					gOneHzPtr += gLastToneFreq + (int) (toneStep * (double) stepCount);
				}
			}
			else
			{
				// No frequency smoothing needed
				gOneHzPtr += gToneFreq;
				gLastToneFreq = gToneFreq;
			}
			if (gOneHzPtr >= SAMPLING_RATE)
			{
				// We wrapped the end of the sample buffer
				gOneHzPtr -= SAMPLING_RATE;
			}
		}
	}

	// If we are stopping the tone, then mark it as stopped
	if (tone == TONE_STOP)
	{
		// We only transition to STOPPED if we have decayed enough
		if (gDecayLevel >= DECAY_MAX_LEVEL)
		{
			// Change to STOPPED state
			gPlayTone = TONE_STOPPED;
			gDecayLevel = DECAY_MAX_LEVEL;
		}
	}

	// Else if we are starting the tone, mark it as PLAYING
	else if (tone == TONE_START)
	{
		// We only transition to PLAYING once we have ramped up to full volume
		if (gDecayLevel <= 0.0)
		{
			gDecayLevel = 0.0;
			gPlayTone = TONE_PLAYING;
		}
	}

	/* Write the buffer to the output device */
	sound_write_audio(hOutput, (LPSTR) gToneBuf, BLOCK_SIZE);
    return;

}

DWORD WINAPI EquProc(LPVOID lpParam)
{
	// Loop forever, or til time to quit.  This is the audio driver thread.
	while (1)
	{
		// If no active tone being played, wait for event so we don't
		// consume processor time
		if (!gPlayTone && !gBeepOn && (gReqIn == gReqOut))
		{
			WaitForSingleObject(gSoundEvent, INFINITE);
		}

		/* Test if exiting the application */
		if (gExit)
			break;

		/* Test for new request */
		if (gReqIn != gReqOut)
		{
			int newFreq = gReqFreq[gReqOut++];
			if (gReqOut >= 16)
				gReqOut = 0;

			if ((newFreq == 0))// && (gPlayTone == TONE_PLAYING))
			{
				/* Stop the currently playing tone */
				if (gReqIn == gReqOut)
				{
					gPlayTone = TONE_STOP;
				}
			}
			else if ((newFreq > 0) && (gPlayTone != TONE_PLAYING))
			{
				/* Start new tone */
				gToneFreq = newFreq;
				gPlayTone = TONE_START;

				/* Keep track of how long a tone is playing
				   and shut it down if it's too long */
				gPlayCycle = cycles;
			}
			else
			{
				/* Change the tone frequency.  The play_tone routine will 
				   filter the change from the old frequency to the new. */
				gToneFreq = newFreq;
			}
		}

		/* Check if tone being generated */
		if (gPlayTone)
			sound_play_tone(gPlayTone, gToneFreq);

		/* Test if the "beep" command is active */
		if (gBeepOn)
		{
			UINT64			dc = cycles - spkr_cycle;

			/* Wait for the "beep" time to expire and turn it off */
			if (dc > 15000)
			{
				gPlayTone = TONE_STOP;
				gBeepOn = 0;
				gReqLastFreq = 0;
			}
		}
	}

	_endthreadex(0);

    return 0;
}

#endif


/*
==================================================================
init_sound:	This routine initializes the sound output device(s)
			for tone generations.
==================================================================
*/
void init_sound(void)
{
	int		x;
	double	w;

	// Create sin table for 1Hz
	w = 2.0 * 3.1415926536 / (double) SAMPLING_RATE;
	for (x = 0; x < SAMPLING_RATE; x++)
	{
		gpOneHertz[x] = (unsigned short) (sin(w * (double) x) * 32767.0);
	}

	/* Initialize the emulation cycles count for speaker frequency calcs */
	spkr_cycle = cycles;
	gExit = 0;

	/* Open the sound card / output device */
#ifdef WIN32
	sound_open_output();

	/* Start thread to handle Sound I/O and create an event for triggering */
	gSoundEvent = CreateEvent(NULL, TRUE, FALSE, NULL);
	g_hEquThread = (HANDLE)_beginthreadex(0,0,EquProc,0,0,&dwThreadID);
#else

    /* TODO:  Add Linux support */
#endif

}

/*
==================================================================
deinit_sound:	This routine deinitializes the sound output device(s)
				for tone generations.
==================================================================
*/
void deinit_sound(void)
{
	/* Set the exit variable */
	gExit = 1;

	/* Close the output device */
#ifdef _WIN32
	sound_close_output();

	CloseHandle(gSoundEvent);
#else

    /* TODO:  Add Linux support for closing the sound system */
#endif
}


/*
==================================================================
start_tone:	This routine starts a tone of specified frequency
==================================================================
*/
void sound_start_tone(int freq)
{
	/* Validate sound is enabled */
	if (!sound_enable)
		return;

	if ((freq < SAMPLING_RATE / 2.0) && (freq != gReqLastFreq))
	{
		gReqLastFreq = freq;
		/* Issue request for new frequency generation */
#ifdef WIN32
		EnterCriticalSection(&reqCriticalSection);
#endif
		gReqFreq[gReqIn++] = freq;
		if (gReqIn >= 16)
			gReqIn = 0;
#ifdef WIN32
		LeaveCriticalSection(&reqCriticalSection);
		PulseEvent(gSoundEvent);
#endif
	}
}

/*
==================================================================
stop_tone:	This routine stops playing of tones
==================================================================
*/
void sound_stop_tone(void)
{
	/* Validate sound is enabled */
	if (!sound_enable)
		return;

	/* Issue a request for a frequency of zero */
	sound_start_tone(0);
}

/*
==================================================================
toggle_speaker:	This routine handles toggling of the I/O bit that
				is connected directly to the speaker.  The routine
				calculates the frequency of toggle and generates
				a beep during the toggle period.
==================================================================
*/
void sound_toggle_speaker(int bitVal)
{
	UINT64			delta;

	/* Validate sound is enabled */
	if (!sound_enable)
		return;

	/* Calculate delta between current cycle and last cycle */
	delta = cycles + cycle_delta - spkr_cycle;
	spkr_cycle = cycles + cycle_delta;

	/* Test if delta is within a valid range */
	if ((delta < 5000) && (delta != 0))
	{
		/* Indicate the "Beep" is on and set the frequency */
		gBeepOn = 1;
		sound_start_tone((int) (2400000.0 / delta / 2));
	}
}

/*
==================================================================
This routine sets the tone control divisor
==================================================================
*/
void sound_set_tone_control(double tone)
{
	// Don't allow tone divisor of zero
	if (tone == 0.0)
		tone = 1.0;

	// Set the tone divisor
	gToneDivisor = tone;
	gDecayStep = tone / (double) (BLOCK_SIZE>>3);
}

/*
==================================================================
This routine gets the tone control divisor
==================================================================
*/
double sound_get_tone_control(void)
{
	return gToneDivisor;
}
