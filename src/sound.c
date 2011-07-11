/* sound.c */

/* $Id: sound.c,v 1.8 2011/07/09 08:16:21 kpettit1 Exp $ */

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

//#define				BLOCK_SIZE		2048
//#define				BLOCK_COUNT		4
#define				BLOCK_SIZE		1024
#define				BLOCK_COUNT		8
#define				SAMPLING_RATE	22050
#define				NUM_CHANNELS	2
#define				BITS_PER_SAMPLE	16

#define				TONE_STOPPED	0
#define				TONE_START		1
#define				TONE_PLAYING	2
#define				TONE_STOP		3

/*
 * module static data
 */
static int			gReqFreq[16];
static int			gReqIn = 0;
static int			gReqLastFreq = 0;

#ifdef _WIN32
static int			gReqOut = 0;
static int			gDecaySample = 0;
#endif

unsigned short			readbuf [BLOCK_SIZE >> 1] ;          /* input buffer */
unsigned short			gpOneHertz[SAMPLING_RATE];
//unsigned short			*gpOneHertz = NULL;

int						gPlayTone = TONE_STOPPED;
int						gToneFreq = 0;
volatile int			gNewToneFreq = 0;
int						gFlushBuffers = 0;
int						gExit = 0;
int						gBeepOn = 0;
int						gOneHzPtr = 0;
extern	UINT64			cycles;
extern	int				cycle_delta;
static	UINT64			spkr_cycle = 0;
int						sound_enable = 1;

FILE*					gFd;
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

	/* Get pointer to the block processed */
//    current = &waveBlocks[waveLastProcBlock++];
//	if (waveLastProcBlock > BLOCK_COUNT)
//		waveLastProcBlock = 0;

    EnterCriticalSection(&waveCriticalSection);
    (*freeBlockCounter)++;
//	current->dwUser = -1;
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
        if(current->dwFlags & WHDR_PREPARED) 
			waveOutUnprepareHeader(hWaveOut, current, sizeof(WAVEHDR));

		remain = size < BLOCK_SIZE ? size : BLOCK_SIZE;
		current->dwUser = size;
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
//	gFd = fopen("beep.txt", "w+");
	/* Calculate buffer size */
	waveBlocks = sound_allocate_blocks(BLOCK_SIZE, BLOCK_COUNT);
    waveFreeBlockCount = BLOCK_COUNT;
    waveCurrentBlock = 0;
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

//	fclose(gFd);
	/* Wait for all buffers to finish playing */
	sound_flush_buffers();

	/* Pulse the sound thread thread so it will release */
//	PulseEvent(gSoundEvent);

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
/*	newVol = ((volume >> 1) & 0xFFFF0000) | ((volume & 0xFFFF) >> 1);
	while (newVol > 0)
	{
		waveOutSetVolume(hOutput, newVol);
//		Sleep(3);
		newVol = ((newVol >> 1) & 0xFFFF0000) | ((newVol & 0xFFFF) >> 1);
	}
*/
	if (hOutput != NULL)
		waveOutReset (hOutput);
	gOneHzPtr = 0;

	/* Wait for all buffers to be complete */
//	sound_flush_buffers();

	/* Restore the original volume */
	waveOutSetVolume(hOutput, volume);
//	waveFreeBlockCount = BLOCK_COUNT;
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
void sound_play_tone()
{
    int		i;
//	int		zero_cross, zero_cross_point;
	int		samples = BLOCK_SIZE >> 1;
	int		tone = gPlayTone;
	int		toneFreq = gToneFreq;
//	int		decay_samples = samples;
	int		decay_time = samples;
	double	divisor = (double) decay_time / 7;
	
	/* Create a buffer with the specified frequency */
//	zero_cross = 0;
	for (i = 0; i < samples; i += 2)
	{
		if (tone == TONE_START)
		{
			unsigned short val = (unsigned short) (((double) (signed short) gpOneHertz[gOneHzPtr]) *
				exp(- (double) (decay_time*2 - gDecaySample++) / divisor));
			readbuf[i] = val;
			readbuf[i + 1] = val;
		}
		else if (tone == TONE_PLAYING)
		{
			readbuf[i] = gpOneHertz[gOneHzPtr];
			readbuf[i + 1] = gpOneHertz[gOneHzPtr];
		}
		else if (tone == TONE_STOP)
		{
			unsigned short val = (unsigned short) (((double) (signed short) gpOneHertz[gOneHzPtr]) *
				exp(- (double) i / (divisor) ));
			readbuf[i] = val;
			readbuf[i + 1] = val;
		}

		/* Update pointer in the 1 Hz waveform based on frequency */
		gOneHzPtr += gToneFreq;
		if (gOneHzPtr >= SAMPLING_RATE)
		{
			// Zero crossing occurred
//			zero_cross = i + 1;
			gOneHzPtr -= SAMPLING_RATE;
//			zero_cross_point = gOneHzPtr;
		}

//				readbuf[i] = 200;
//				readbuf[i + 1] = 200;
//		fprintf(gFd, "%d\n", (int) (signed short) readbuf[i]);
	}

	if (tone == TONE_STOP)
	{
		gPlayTone = TONE_STOPPED;
		gOneHzPtr = 0;
	}
	else if ((tone == TONE_START) && (gDecaySample >= BLOCK_SIZE))
	{
		gDecaySample = 0;
		gPlayTone = TONE_PLAYING;
	}

	/* Write the buffer to the output device */
	sound_write_audio(hOutput, (LPSTR) readbuf, BLOCK_SIZE);

    return;

}

DWORD WINAPI EquProc(LPVOID lpParam)
{
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

		/* Test for new tone frequency changes only if stopped or playing */
		if ((gPlayTone == TONE_STOPPED) || (gPlayTone == TONE_PLAYING))
		{
			/* Test for new request */
			if (gReqIn != gReqOut)
			{
				int newFreq = gReqFreq[gReqOut++];
				if (gReqOut >= 16)
					gReqOut = 0;

				if ((newFreq == 0) && (gPlayTone == TONE_PLAYING))
				{
					/* Stop the currently playing tone */
					if (gReqIn == gReqOut)
						gPlayTone = TONE_STOP;
				}
				else if ((newFreq > 0) && (gPlayTone == TONE_STOPPED))
				{
					/* Start new tone */
					gToneFreq = newFreq;
					gPlayTone = TONE_START;
				}
				else
				{
					/* Change the tone frequency */
					gToneFreq = newFreq;

					/* Probably need to change the unplayed buffers here */
				}
			}
		}

		/* Check if tone being generated */
		if (gPlayTone)// && (gToneFreq = gNewToneFreq))
			sound_play_tone();	

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

	// Create sin table
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

	/*
    WAVEHDR*		current;
	unsigned short	*sData;
	int				c;

	EnterCriticalSection(&reqCriticalSection);
	gReqFreq[gReqFreqIn++] = 0;
	if (gReqFreqIn > 16)
		gReqFreqIn = 0;
	LeaveCriticalSection(&reqCriticalSection);
*/
	/* Go to the last queued block and clear the data past the zero-crossing */
/*    current = &waveBlocks[waveLastQueueBlock];
	if (current->dwUser != -1)
	{
		sData = (unsigned short *) current->lpData;
		c = ((int) current->lpData) >> 1;
		for (; c < BLOCK_SIZE >> 1; c++)
			sData[c] = 0;
	} */\
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
