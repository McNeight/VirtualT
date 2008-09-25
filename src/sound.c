/* sound.c */

/* $Id: sound.c,v 1.6 2008/02/01 06:18:04 kpettit1 Exp $ */

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

#define				snd_Buffers		2 
#define				cwSizel			128

/*
 * module static data
 */
unsigned int			g_FrameSize ;
unsigned int			g_Blocks ;
unsigned int			g_BufSize ;
const unsigned int		g_Buffers = 2 ;

unsigned int			NextBuf, fDone [snd_Buffers] ;
unsigned int			Index ;
unsigned short			readbuf [2 * cwSizel] ;          /* input buffer */
short int				tmpbuf [2 *  cwSizel] ;
unsigned short			*gpOneHertz = NULL;
int						m_Channels = 2 , 
						m_SamplingRate = 22050, 
						SamplesPerFrame = cwSizel;
short					*pcmbuf;
short					*tmppcmbuf;
int						frameCount=0;
int						cwSize = 2*cwSizel;

int						gPlayTone = 0;
int						gToneFreq = 0;
int						gFlushBuffers = 0;
int						gExit = 0;
int						gBeepOn = 0;
int						gOneHzPtr = 0;
extern	UINT64			cycles;
extern	int				cycle_delta;
static	UINT64			spkr_cycle = 0;

#ifdef _WIN32
HANDLE					g_hEquThread;
DWORD					dwThreadID;
WAVEFORMATEX			wf ;
HWAVEOUT				hOutput ;
WAVEHDR					wh [snd_Buffers] ;
#endif


#ifdef _WIN32

void APIENTRY sndCallback (HWAVEOUT hWaveOut, UINT wMsg, DWORD dwInstance, DWORD dwParam1, DWORD dwParam2)
{
	if (wMsg == WOM_DONE) 
		fDone [((LPWAVEHDR) dwParam1)->dwUser] = TRUE ;
}

BOOL sndOpenOutput (unsigned int chan, unsigned int freq, unsigned int framesize)
{
	UINT		i;
	UINT		n;
	UINT		id = WAVE_MAPPER;
	UINT		devs = waveOutGetNumDevs ();
	WAVEOUTCAPS	caps ;
	MMRESULT	err;

    // we're not using wavemapper, since soundblaster probably is
    // preferred device --> look for the multichannel card
	if (chan > 2)
	{
		/* Loop through all devices looking for multi-channel support */
		for (id = 0 ; id < devs ; id++)
		{
			err = waveOutGetDevCaps (id, &caps, sizeof (WAVEOUTCAPS)) ;
			if ((err == MMSYSERR_NOERROR) && (caps.wChannels >= chan)) 
				break ;
		}

		/* Check if no output devices found */
		if (id == devs) 
			return FALSE ;

		g_FrameSize = 8 * framesize ;
		g_Blocks = 10 ;
	}
	else
	{
		/* Configure for up to 2 channel support */
		g_FrameSize = chan * framesize ;
		g_Blocks = 22 ;
	}

	/* Calculate buffer size */
	g_BufSize = 2 * g_Blocks * g_FrameSize ;

	/* Prepare to open the Wave Device */
	wf.nSamplesPerSec = freq ;
	wf.wFormatTag = WAVE_FORMAT_PCM ;
	wf.nChannels = chan ;
	wf.nBlockAlign = 2 * wf.nChannels ;   
	wf.nAvgBytesPerSec = wf.nSamplesPerSec * wf.nBlockAlign ;
	wf.wBitsPerSample = 16 ; 

	/* Try to open the Wave output device */
	waveOutOpen (&hOutput, id, &wf, (DWORD) sndCallback, 0, /* WAVE_FORMAT_DIRECT | */ CALLBACK_FUNCTION) ;
	if (hOutput == NULL) 
		return FALSE ;

	/* Initialize buffers for the device */
	for ( i = 0 ; i < g_Buffers ; i++)
	{
		ZeroMemory (&wh [i], sizeof (WAVEHDR)) ;

		wh [i].dwBufferLength = g_BufSize ;
		wh [i].lpData = (LPSTR) VirtualAlloc (NULL, g_BufSize, MEM_COMMIT, PAGE_READWRITE) ;
		if (wh [i].lpData == NULL)
		{
			for (n = 0 ; n < i ; n++) 
				VirtualFree (wh [n].lpData, 0, MEM_RELEASE) ;
			waveOutClose (hOutput) ;
			return FALSE ;
		}

		ZeroMemory (wh [i].lpData, g_BufSize) ;

		wh [i].dwUser = i ;
		fDone [i] = TRUE ;
		NextBuf = 0 ;
	}

	Index = 0 ;

	return TRUE ;
}



BOOL sndCloseOutput (void)
{
	UINT i;

	if (hOutput == NULL) 
		return FALSE ;

	waveOutReset (hOutput) ;

	for (i = 0 ; i < g_Buffers ; i++)
	{
		if (wh [i].dwFlags & WHDR_PREPARED) 
			waveOutUnprepareHeader (hOutput, &wh [i], sizeof (WAVEHDR)) ;
		VirtualFree (wh [i].lpData, 0, MEM_RELEASE) ;
	}

	waveOutClose (hOutput) ;
	hOutput = NULL ;

	return TRUE ;
}

short *sndGetBuffer (void)
{  
	short *wbuf;
  
	if (hOutput == NULL) 
		return NULL ;

	wbuf = (short *) wh [NextBuf].lpData ;
	wbuf = &wbuf [g_FrameSize * Index++] ;

	if (Index <= g_Blocks) 
		return wbuf ;

	Index = 1 ; /* i.e. Index = 0++ */

	fDone [NextBuf] = FALSE ;
	waveOutPrepareHeader (hOutput, &wh [NextBuf], sizeof (WAVEHDR)) ;
	waveOutWrite (hOutput, &wh [NextBuf], sizeof (WAVEHDR)) ;     

	NextBuf = ++NextBuf % g_Buffers ;

	while (fDone [NextBuf] == FALSE) 
		Sleep (60) ;
	/* if (WaitForSingleObject (hStopNow, 40) == WAIT_OBJECT_0) return FALSE ; */

	if (wh [NextBuf].dwFlags & WHDR_PREPARED)
		waveOutUnprepareHeader (hOutput, &wh [NextBuf], sizeof (WAVEHDR)) ;

	return (short *) wh [NextBuf].lpData ;
}

void sndFlushBuffers (void)
{ 
	UINT i;

	/* Zero the memory in all buffers */
	for (i = Index ; i <= g_Blocks ; i++)
	{  
		PBYTE buf;
		buf = (PBYTE) sndGetBuffer () ;
		if (!buf) 
			return ;

		ZeroMemory (buf, g_FrameSize * sizeof (short)) ;
	}

	/* Wait for all buffers to be done */
	for (i = 0 ; i < g_Buffers ; i++)
	{
		while (fDone [i] == FALSE)
			Sleep (20) ;
	}
}

//;=============================================================
void playback()
{
    int		i;
    BOOL	fWantPlayback;
	

	for (i = 0; i < cwSize; i++)
	{
		readbuf[i] = gpOneHertz[gOneHzPtr];
		gOneHzPtr += gToneFreq;
		if (gOneHzPtr >= m_SamplingRate)
			gOneHzPtr -= m_SamplingRate;
	}
	fWantPlayback = TRUE;

	/*   get an output buffer and do decoding work   */
	pcmbuf = (fWantPlayback && frameCount) ? sndGetBuffer () : tmpbuf ;
	tmppcmbuf = pcmbuf;

	for (i=0;i<cwSize;i++)
	{
		*tmppcmbuf = (short)readbuf[i];
		tmppcmbuf++;
	}

	if (!frameCount++)
	{
		if (!sndOpenOutput (m_Channels , m_SamplingRate, SamplesPerFrame))
			exit(0);
	}

    return;

}

DWORD WINAPI EquProc(LPVOID lpParam)
{
	static  creatflag;

	while (1)
	{
		// If no active tone being played, wait for event so we don't
		// consume processor time
		if (!gPlayTone && !gBeepOn)
		{
		}

		// Check if tone being generated
		if (gPlayTone)
			playback();	

		if (gFlushBuffers)
		{
			sndFlushBuffers();
			gFlushBuffers = 0;
		}

		if (gBeepOn)
		{
			if (cycles + cycle_delta - spkr_cycle > 0.001)
			{
				gPlayTone = 0;
				gBeepOn = 0;
				sndFlushBuffers();
			}
		}
		if (gExit)
			break;
	}

	sndCloseOutput ();
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

	gExit = 0;

	// Create sin table
	gpOneHertz = (unsigned short *)malloc(m_SamplingRate * sizeof(unsigned short));
	w = 2.0 * 3.14159 / (double) m_SamplingRate;
	for (x = 0; x < m_SamplingRate; x++)
	{
		gpOneHertz[x] = (unsigned short) (sin(w * (double) x) * 32767.0);
	}
	spkr_cycle = cycles;

	return;

#ifdef _WIN32
	// Start thread to handle Sound I/O
	g_hEquThread = (HANDLE)_beginthreadex(0,0,EquProc,0,0,&dwThreadID);
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
	gExit = 1;
	free(gpOneHertz);

#ifdef _WIN32
	sndCloseOutput();
#endif
}


/*
==================================================================
start_tone:	This routine starts a tone of specified frequency
==================================================================
*/
void sound_start_tone(int freq)
{
	if (freq < m_SamplingRate / 2.0)
	{
//		printf("Playing tone of frequency %d\n", freq);
		gToneFreq = freq;
		gPlayTone = 1;
	}
}

/*
==================================================================
stop_tone:	This routine stops playing of tones
==================================================================
*/
void sound_stop_tone(void)
{

//	printf("Stop playing tone\n");
#ifdef _WIN32
//	waveOutReset (hOutput) ;
#endif
	gPlayTone = 0;
//	gFlushBuffers = 1;
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

	/* Calculate delta between current cycle and last cycle */
	delta = cycles + cycle_delta - spkr_cycle;
	spkr_cycle = cycles + cycle_delta;

	/* Test if delta is within a valid range */
	if ((delta < 5000) && (delta != 0))
	{
		gBeepOn = 1;
		sound_start_tone((int) (60000.0 / delta));
	}

}


