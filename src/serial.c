/* serial.c */

/* $Id: serial.c,v 1.0 2004/08/05 06:46:12 kpettit1 Exp $ */

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

#if defined(WIN32)
#include <windows.h>
#endif
#include <stdio.h>
#include <string.h>
#include <time.h>

#include "serial.h"
#include "setup.h"
#include "display.h"
#include "m100emu.h"

ser_params_t	sp;

/*
===========================================================================
ser_init:	Initialize serial structures
===========================================================================
*/
int ser_init(void)
{
	// Initialize port name
	if (setup.com_mode == SETUP_COM_HOST)
		strcpy(sp.port_name, setup.com_port);
	else if (setup.com_mode == SETUP_COM_OTHER)
		strcpy(sp.port_name, setup.com_other);
	else if (setup.com_mode == SETUP_COM_SIMULATED)
		strcpy(sp.port_name, setup.com_cmd);

	sp.baud_rate = 9600;		// Default to 9600 baud rate
	sp.open_flag = 0;			// Port not open
	sp.bit_size = 8;			// Default to 8 bits
	sp.parity = 'N';			// Initialize to no parity
	sp.pCallback = NULL;		// No callback funciton
	sp.pMonCallback = NULL;		// No monitor callback function
	sp.stop_bits = 1;			// Initialize to 1 stop bit
	sp.pCmdFile = NULL;			// No open command file
	sp.tx_empty = TRUE;			// Indicate no active TX
	sp.rxIn = 0;				// Initialize Read buffer
	sp.rxOut = 0;
	sp.txIn = 0;				// Initialize TX buffer
	sp.txOut = 0;

#ifdef WIN32
	// Create overlapped read event. Must be closed before exiting
	// to avoid a handle leak.
	sp.osRead.hEvent = CreateEvent(NULL, TRUE, FALSE, NULL);
	if (sp.osRead.hEvent == NULL)
		return SER_IO_ERROR;

	sp.osWrite.hEvent = CreateEvent(NULL, TRUE, FALSE, NULL);
	if (sp.osWrite.hEvent == NULL)
	{
		CloseHandle(sp.osRead.hEvent);
		return SER_IO_ERROR;
	}

	// Create events for exiting threads and writing bytes
	sp.hThreadExitEvent = CreateEvent(NULL, TRUE, FALSE, NULL);
	if (sp.hThreadExitEvent == NULL)
		return SER_IO_ERROR;
	sp.hWriteEvent = CreateEvent(NULL, FALSE, FALSE, NULL);
	if (sp.hWriteEvent == NULL)
		return SER_IO_ERROR;
	sp.hWriteMutex = CreateMutex(NULL, FALSE, NULL);
	if (sp.hWriteMutex == NULL)
		return SER_IO_ERROR;
	sp.hReadMutex = CreateMutex(NULL, FALSE, NULL);
	if (sp.hReadMutex == NULL)
		return SER_IO_ERROR;

	sp.fReadThread = FALSE;
	sp.fWriteThread = FALSE;
	sp.fIntPending = FALSE;
	sp.dtrState = DTR_CONTROL_DISABLE;
	sp.rtsState = RTS_CONTROL_DISABLE;
	
	// If Serial port emulation, open the port
	if (setup.com_mode != SETUP_COM_NONE)
		ser_open_port();

#else

	// ============================================
	// Add code here for POSIX initialization
	// ============================================


#endif

	return SER_NO_ERROR;
}

/*
===========================================================================
ser_deinit:	De-Initialize serial structures
===========================================================================
*/
int ser_deinit(void)
{

#ifdef WIN32

	// Trigger event to kill worker therads
	SetEvent(sp.hThreadExitEvent);

	// Wait for threads to exit
	while ((sp.fReadThread == TRUE) && (sp.fWriteThread == TRUE))
		;

	// Close event handles
	if (sp.osRead.hEvent != NULL)
		CloseHandle(sp.osRead.hEvent);
	if (sp.osWrite.hEvent != NULL)
		CloseHandle(sp.osWrite.hEvent);
	if (sp.hThreadExitEvent != NULL)
		CloseHandle(sp.hThreadExitEvent);
	if (sp.hWriteEvent != NULL)
		CloseHandle(sp.hWriteEvent);

	// Close port
	if (sp.hComm != NULL)
		CloseHandle(sp.hComm);
#else

	// ===============================================
	// Add code here to deinitialize for POSIX systems
	// ===============================================

#endif

	return SER_NO_ERROR;
}

/*
===========================================================================
ser_get_port_list:	Get a list of valid COM port names and count.

					This function returns a comma seperated list of serial
					port names valid for the host.  This list will be 
					displayed in the Peripheral Setup dialog to allow the
					user to select where emulation should be directed.
===========================================================================
*/
int ser_get_port_list(char* port_list, int max, int *count)
{

#ifdef WIN32
	int		c;
	HANDLE	hComm;
	char	str[6];
	int		list_len = 0;

	*count = 0;
	port_list[0] = 0;

	/* Loop through up to 8 serial ports */
	for (c = 1; c <= 8; c++)
	{
		/* Create string name for each port */
		sprintf(str, "COM%d", c);

		// Test if port is already opened (it needs to be in the list too)
		if ((strcmp(str, sp.port_name) == 0) && (sp.open_flag))
			hComm = (void*)1;

		else
		{
			/* Try to open each port */
			hComm = CreateFile(str, 
						GENERIC_READ | GENERIC_WRITE, 
						0, 
						0, 
						OPEN_EXISTING,
						FILE_FLAG_OVERLAPPED,
						0);

			/* Check if COM port exists & not in use */
			if (hComm == INVALID_HANDLE_VALUE)
				continue;

			/* Close the port and add to the list */
			CloseHandle(hComm);
		}

		/* Check if port name fits in buffer */
		if (strlen(port_list) + strlen(str) + 1 <= (unsigned int) max)
		{
			// If this isn't the first item in list, append a ','
			if (*count > 0)
				strcat(port_list, ",");

			// Append port name to list
			strcat(port_list, str);

			*count++;
		}
	}
#else
	*count = 0;
	port_list[0] = 0;

	// ============================================
	// Add code here to get port list for POSIX
	// ============================================

#endif

	return SER_NO_ERROR;
}


#ifdef WIN32
/*
===========================================================================
initialize_dcb:	Configure the WIN32 dcb structure with settings in sp

  This is a WIN32 specific routine
===========================================================================
*/
void initialize_dcb(void)
{
	GetCommState(sp.hComm, &sp.dcb);

	// Set Baud Rate
	sp.dcb.BaudRate = sp.baud_rate;

	// Set number of bits and stop bits
	sp.dcb.ByteSize = sp.bit_size;
	if (sp.stop_bits == 1)
		sp.dcb.StopBits = ONESTOPBIT;
	else 
		sp.dcb.StopBits = TWOSTOPBITS;

	// Set parity check flag
	if (sp.parity == 'I')
		sp.dcb.fParity = FALSE;
	else
		sp.dcb.fParity = TRUE;
	if (sp.parity == 'E')
		sp.dcb.Parity = EVENPARITY;
	else if (sp.parity == 'O')
		sp.dcb.Parity = ODDPARITY;
	else if (sp.parity == 'N')
		sp.dcb.Parity = NOPARITY;

	// Set flow control members
	sp.dcb.fOutxCtsFlow = FALSE;
	sp.dcb.fOutxDsrFlow = FALSE;
	sp.dcb.fInX = FALSE;
	sp.dcb.fOutX = FALSE;
	sp.dcb.fDsrSensitivity = FALSE;

	// DTR is controlled by the ROM so disable it
	sp.dcb.fDtrControl = FALSE;//sp.dtrState;

	// RTS is controlled by the ROM so disable it
	sp.dcb.fRtsControl = FALSE;//sp.rtsState;;
}


void ErrorReporter(char* text)
{
	show_error(text);
}

/*
===========================================================================
process_read_byte: Called when a byte is received by the COM port.  This
		routine adds the byte to the buffer and calls he callback function.
===========================================================================
*/
void process_read_byte(ser_params_t *sp, char byte)
{
	int new_rxIn;

#ifdef WIN32
	WaitForSingleObject(sp->hReadMutex, 2000);
#endif

	// Add byte to read buffer
	sp->rx_buf[sp->rxIn] = byte;

	new_rxIn = sp->rxIn + 1;
	if (new_rxIn >= sizeof(sp->rx_buf))
		new_rxIn = 0;

	sp->rxIn = new_rxIn;

#ifdef WIN32
	ReleaseMutex(sp->hReadMutex);
#endif

	// Check if there is a monitor window open and report the write
//	if (sp->pMonCallback != NULL)
//		sp->pMonCallback(SER_MON_COM_READ, (unsigned char) byte);

	// Call callback to indicate receipt of data
	if (sp->pCallback != NULL)
	{
		sp->pCallback();
		sp->fIntPending = TRUE;
	}
}

/*
===========================================================================
stop_threads:: Terminates the Read and Write threads
===========================================================================
*/
void stop_threads(void)
{
	time_t		start_time, cur_time;

	// Trigger event to exit threads
	SetEvent(sp.hThreadExitEvent);

	// Get start time for Wait timeout
	time(&start_time);

	// Wait for threads to exit (up to 2 seconds)
	while (sp.fReadThread && sp.fWriteThread)
	{
		time(&cur_time);
		if (start_time + 2 <= cur_time)
			break;
	}

	ResetEvent(sp.hThreadExitEvent);
}

    BOOL			fWaitingOnRead = FALSE;
/*
===========================================================================
ReadProc:  Thread to read data from COM port
===========================================================================
*/
DWORD ReadProc(LPVOID lpv)
{
    HANDLE			hArray[2];

    DWORD			dwStoredFlags = 0xFFFFFFFF;     // local copy of event flags
	DWORD			dwFlags = EV_BREAK | EV_CTS | EV_DSR | EV_ERR | EV_RING | EV_RLSD;
    DWORD 			dwRead;							// bytes actually read
    DWORD			dwRes;							// result from WaitForSingleObject
	COMSTAT			comStat;
	DWORD			dwError;
    BOOL			fWaitingOnStat = FALSE;
    BOOL			fThreadDone = FALSE;
//	char			read_ch;
	char			buf[128];
	unsigned int	c;
	ser_params_t	*sp;


    sp = (ser_params_t *) lpv;

    // Detect the following events:
    //   Read events (from ReadFile)
    //   Thread exit evetns (from our shutdown functions)
    hArray[0] = sp->osRead.hEvent;
    hArray[1] = sp->hThreadExitEvent;

    while ( !fThreadDone ) 
	{
        // if no read is outstanding, then issue another one
        if (!fWaitingOnRead) 
		{
            if (!ReadFile(sp->hComm, buf, 1, &dwRead, &sp->osRead))
			{
                if (GetLastError() != ERROR_IO_PENDING)	  // read not delayed?
                    ErrorReporter("ReadFile in ReaderAndStatusProc");

                fWaitingOnRead = TRUE;
            }
            else 
			{
				// read completed immediately
                if (dwRead)
				{
					for (c = 0; c < dwRead; c++)
						process_read_byte(sp, buf[c]);
				}
            }
        }

        if (dwStoredFlags != dwFlags) {
            dwStoredFlags = dwFlags;
            if (!SetCommMask(sp->hComm, dwStoredFlags))
                ErrorReporter("SetCommMask");
        }

        // wait for pending operations to complete
        if ( fWaitingOnRead ) 
		{
            dwRes = WaitForMultipleObjects(2, hArray, FALSE, INFINITE);
            switch(dwRes)
            {
                // read completed
                case WAIT_OBJECT_0:
                    if (!GetOverlappedResult(sp->hComm, &sp->osRead, &dwRead, FALSE)) 
					{
						ClearCommError(sp->hComm, &dwError, &comStat);
                        ErrorReporter("GetOverlappedResult (in Reader)");
					}
                    else 
					{      
						// read completed successfully
                        if (dwRead)
						{
							for (c = 0; c < dwRead; c++)
								process_read_byte(sp, buf[c]);
						}
						fWaitingOnRead = FALSE;
                    }

                    break;

				case WAIT_TIMEOUT:
					break;

                // thread exit event
                case WAIT_OBJECT_0 + 1:
                    fThreadDone = TRUE;
                    break;

                default:
                    ErrorReporter("WaitForMultipleObjects(Reader & Status handles)");
                    break;
            }
        }
    }

	sp->fReadThread = FALSE;
    return 1;
}

/*
===========================================================================
WriteProc:  Thread to write data to COM port
===========================================================================
*/
DWORD WINAPI WriteProc(LPVOID lpv)
{
    HANDLE			hArray[2];
	HANDLE			hwArray[2];
    DWORD			dwRes;
	DWORD			dwWritten;
    BOOL			fDone = FALSE;
	ser_params_t	*sp;

	sp = (ser_params_t*) lpv;

    hArray[0] = sp->hWriteEvent;
    hArray[1] = sp->hThreadExitEvent;
   
    while ( !fDone ) {
        dwRes = WaitForMultipleObjects(2, hArray, FALSE, INFINITE);
        switch(dwRes)
        {
            case WAIT_FAILED:
                ErrorReporter("WaitForMultipleObjects( writer proc )");
                break;

            // write request event
            case WAIT_OBJECT_0:
                hwArray[0] = sp->osWrite.hEvent;
				hwArray[1] = sp->hThreadExitEvent;
    
				// Read bytes from sp structure
//				c = 0;
#ifdef WIN32
				WaitForSingleObject(sp->hWriteMutex, 2000);
#endif
//				while (sp->txIn != sp->txOut)
//				{
//					buf[c++] = sp->tx_buf[sp->txOut++];
//					if (sp->txOut >= sizeof(sp->tx_buf))
//						sp->txOut = 0;
//				}

#ifdef WIN32
				ReleaseMutex(sp->hWriteMutex);
#endif
				// issue write
				if (!WriteFile(sp->hComm, sp->tx_buf, 1, &dwWritten, &sp->osWrite)) 
				{
					if (GetLastError() == ERROR_IO_PENDING) 
					{ 
						// write is delayed
						dwRes = WaitForMultipleObjects(2, hwArray, FALSE, INFINITE);
						switch(dwRes)
						{
							// write event set
							case WAIT_OBJECT_0:
								SetLastError(ERROR_SUCCESS);
								if (!GetOverlappedResult(sp->hComm, &sp->osWrite, &dwWritten, FALSE)) {
									ErrorReporter("GetOverlappedResult(in Writer)");
								}
                    
								break;

							// thread exit event set
							case WAIT_OBJECT_0 + 1:
								break;

							// wait timed out
							case WAIT_TIMEOUT:
								break;
						}

						sp->tx_empty = TRUE;
					}
				}
    
                break;

            // thread exit event
            case WAIT_OBJECT_0 + 1:
                fDone = TRUE;
                break;
        }
    }

	sp->fWriteThread = FALSE;
    return 1;
}


/*
===========================================================================
start_threads:: Creates the Reader/Status and Writer threads
===========================================================================
*/
int start_threads(void)
{
    DWORD dwReadStatId;
    DWORD dwWriterId;

	// Create read thread
    sp.hReadThread = CreateThread( NULL, 0,
                      (LPTHREAD_START_ROUTINE) ReadProc,
                      (LPVOID) &sp, 0, &dwReadStatId);

    if (sp.hReadThread == NULL)
        return SER_IO_ERROR;

	sp.fReadThread = TRUE;

	// Create write thread
    sp.hWriteThread = CreateThread( NULL, 0, 
                      (LPTHREAD_START_ROUTINE) WriteProc, 
                      (LPVOID) &sp, 0, &dwWriterId );
                   
    if (sp.hWriteThread == NULL)
        return SER_IO_ERROR;

	sp.fWriteThread = TRUE;

    return SER_NO_ERROR;
}


#endif

/*
===========================================================================
ser_set_baud:	Set the baud rate to the numeric value passed.

				The value will be in the range of 75 to 19200
===========================================================================
*/
int ser_set_baud(int baud)
{
	// Update baud rate
	sp.baud_rate = baud;

	// If port not open, return -- noting else to do
	if (sp.open_flag == 0)
		return SER_NO_ERROR;

	if (setup.com_mode == SETUP_COM_HOST)
	{
		#ifdef WIN32
			// Update DCB settings
			initialize_dcb();

			// Update Port settings with new DCB
			if (!SetCommState(sp.hComm, &sp.dcb))
			{
				// Error updating port settings
				return SER_IO_ERROR;
			}
		#else

			// ==========================================
			// Add code here for POSIX to set baud rate
			// ==========================================

		#endif
	}

	// Check for a monitor window and report change
	if (sp.pMonCallback != NULL)
		sp.pMonCallback(SER_MON_COM_PORT_CHANGE, SER_MON_COM_CHANGE_BAUD);

	return SER_NO_ERROR;
}

/*
===========================================================================
ser_close_port:	Close the opened serial port
===========================================================================
*/
int ser_close_port(void)
{
	// Test if port is open
	if (sp.open_flag == 0)
		return SER_PORT_NOT_OPEN;

	if (setup.com_mode == SETUP_COM_HOST)
	{
		// ===================
		// Close the Host port
		// ===================
		#ifdef WIN32
			// Insure handle is not NULL
			if (sp.hComm != NULL)
			{
				// Stop all threads
				stop_threads();

				// Close the port
				CloseHandle(sp.hComm);
			}
			sp.hComm = NULL;
		#else

			// ============================================
			// Add code here for POSIX to close port
			// ============================================

		#endif
	}
	else if (setup.com_mode == SETUP_COM_SIMULATED)
	{
		// ======================
		// Close the command file
		// ======================
		if (sp.pCmdFile != NULL)
			fclose(sp.pCmdFile);
		sp.pCmdFile = NULL;
	}

	// Set flag indicating port not open
	sp.open_flag = 0;			

	// Check for a monitor window and report change
	if (sp.pMonCallback != NULL)
		sp.pMonCallback(SER_MON_COM_PORT_CHANGE, SER_MON_COM_CHANGE_STATE);

	// Return with no error
	return SER_NO_ERROR;
}


/*
===========================================================================
ser_open_port:	Open the serial port specified by ser_set_port
===========================================================================
*/
int ser_open_port(void)
{
	// Test if port is already opened
	if (sp.open_flag == 1)
		ser_close_port();

	// Check if using HOST port emulation
	if (setup.com_mode == SETUP_COM_HOST)
	{
		// =============
		// Open the port
		// =============
		#ifdef WIN32
			char msg[50];

			// Attempt to open the port specified by sp.port_name
			sp.hComm = CreateFile( sp.port_name,  
						GENERIC_READ | GENERIC_WRITE, 
						0, 
						0, 
						OPEN_EXISTING,
						FILE_FLAG_OVERLAPPED,
						//FILE_ATTRIBUTE_NORMAL | FILE_FLAG_OVERLAPPED,
						0);
			// Check for error opening port
			if (sp.hComm == INVALID_HANDLE_VALUE)
			{
				sprintf(msg, "Error opening port %s", sp.port_name);
				show_error(msg);
				return SER_IO_ERROR;
			}
		#else
	
			// ============================================
			// Add code here for POSIX to open the port
			// ============================================

		#endif

		// ==========================================
		// Configure port's baud rate, bit_size, etc.
		// ==========================================
		#ifdef WIN32
			FillMemory(&sp.dcb, sizeof(sp.dcb), 0);
			sp.dcb.DCBlength = sizeof(DCB);
			if (!GetCommState(sp.hComm, &sp.dcb))
			{
				DWORD err = GetLastError();

				sprintf(msg,"I/O Error on %s", sp.port_name); 
				show_error(msg);
				// Error reading port state
				return SER_IO_ERROR;
			}

			// Initialize the DCB to set baud rate, etc.
			initialize_dcb();

			// Update DCB settings
			if (!SetCommState(sp.hComm, &sp.dcb))
			{
				// Error updating port settings
				CloseHandle(sp.hComm);
				sp.hComm = 0;
				return SER_IO_ERROR;
			}

			// Start Read an Write Threads
			start_threads();

			// Update flag indicating port open
			sp.open_flag = TRUE;
		#else

			// ============================================
			// Add code here for POSIX to configure port
			// ============================================

		#endif
	}
	else if (setup.com_mode == SETUP_COM_SIMULATED)
	{
		// Open Simulation Command file
		if ((sp.pCmdFile = fopen(setup.com_cmd, "r")) == NULL)
			return SER_IO_ERROR;

		// Update flag indicating port open
		sp.open_flag = TRUE;
	}
	else if (setup.com_mode == SETUP_COM_OTHER)
	{
		// ============================================
		// Add code here to support other Host ports
		// ============================================
	}

	// Check for a monitor window and report change
	if (sp.pMonCallback != NULL)
		sp.pMonCallback(SER_MON_COM_PORT_CHANGE, SER_MON_COM_CHANGE_STATE);

	// Return with no error
	return SER_NO_ERROR;
}

/*
===========================================================================
ser_set_port:	Set the active serial port for subsequent calls

				Port is an OS specific string indicating the name of the
				host serial port where all traffic will be directed.
===========================================================================
*/
int ser_set_port(char* port)
{
	// Save name of port for later use
	strcpy(sp.port_name, port);

		// Check for a monitor window and report change
	if (sp.pMonCallback != NULL)
		sp.pMonCallback(SER_MON_COM_PORT_CHANGE, SER_MON_COM_CHANGE_NAME);

return SER_NO_ERROR;
}

/*
===========================================================================
ser_get_port_settings:	Provides current port emulation settings for 
						reporting in the PeripheralDevices window.
===========================================================================
*/
int ser_get_port_settings(char* port, int* open_state, int* baud, int* size, 
						  int* stop_bits, char* parity)
{
	// Save name of port for later use
	strcpy(port, sp.port_name);
	*open_state = sp.open_flag;
	*baud = sp.baud_rate;
	*size = sp.bit_size;
	*stop_bits = sp.stop_bits;
	*parity = sp.parity;

	return SER_NO_ERROR;
}

/*
===========================================================================
ser_set_parity:	Set parity for the emulated port 

				Valid values for parity are 'O', 'E', 'N', or 'I'
===========================================================================
*/
int ser_set_parity(char parity)
{
	// Check if parity changed
	if (parity == sp.parity)
		return SER_NO_ERROR;

	// Save new parity
	sp.parity = parity;

	// If port not open, return -- noting else to do
	if (sp.open_flag == 0)
		return SER_NO_ERROR;

	if (setup.com_mode == SETUP_COM_HOST)
	{
		#ifdef WIN32
			// Update DCB settings
			initialize_dcb();

			// Update Port settings with new DCB
			if (!SetCommState(sp.hComm, &sp.dcb))
			{
				// Error updating port settings
				return SER_IO_ERROR;
			}
		#else

			// ============================================
			// Add code here for POSIX to set parity
			// ============================================

		#endif
	}

	// Check for a monitor window and report change
	if (sp.pMonCallback != NULL)
		sp.pMonCallback(SER_MON_COM_PORT_CHANGE, SER_MON_COM_CHANGE_PARITY);

	return SER_NO_ERROR;
}

/*
===========================================================================
ser_set_bit_size:	Set the serial port's bit size

					Valid values for bit_size are 6, 7, or 8
===========================================================================
*/
int ser_set_bit_size(int bit_size)
{
	// Check if bit size changed
	if (bit_size == sp.bit_size)
		return SER_NO_ERROR;

	// Save bit size
	sp.bit_size = bit_size;

	// If port not open, return -- noting else to do
	if (sp.open_flag == 0)
		return SER_NO_ERROR;

	if (setup.com_mode == SETUP_COM_HOST)
	{
		#ifdef WIN32
			// Update DCB settings
			initialize_dcb();

			// Update Port settings with new DCB
			if (!SetCommState(sp.hComm, &sp.dcb))
			{
				// Error updating port settings
				return SER_IO_ERROR;
			}
		#else

			// ============================================
			// Add code here for POSIX to set #bits
			// ============================================

		#endif
	}

		// Check for a monitor window and report change
	if (sp.pMonCallback != NULL)
		sp.pMonCallback(SER_MON_COM_PORT_CHANGE, SER_MON_COM_CHANGE_BITS);

return SER_NO_ERROR;
}

/*
===========================================================================
ser_set_stop_bits:	Set the serial port's stop bits

					Valid values for stop_bits are 1 or 2
===========================================================================
*/
int ser_set_stop_bits(int stop_bits)
{
	// Check if stop bits changed
	if (stop_bits == sp.stop_bits)
		return SER_NO_ERROR;

	// Save stop bits
	sp.stop_bits = stop_bits;

	// If port not open, return -- noting else to do
	if (sp.open_flag == 0)
		return SER_NO_ERROR;

	if (setup.com_mode == SETUP_COM_HOST)
	{
		#ifdef WIN32
			// Update DCB settings
			initialize_dcb();

			// Update Port settings with new DCB
			if (!SetCommState(sp.hComm, &sp.dcb))
			{
				// Error updating port settings
				return SER_IO_ERROR;
			}
		#else

			// ============================================
			// Add code here for POSIX to set #stop bits
			// ============================================

		#endif
	}

	// Check for a monitor window and report change
	if (sp.pMonCallback != NULL)
		sp.pMonCallback(SER_MON_COM_PORT_CHANGE, SER_MON_COM_CHANGE_STOP);

	return SER_NO_ERROR;
}

/*
===========================================================================
ser_set_callback:	Set the callback routine for processing incomming data.
===========================================================================
*/
int ser_set_callback(ser_callback pCallback)
{
	// Save callback address
	sp.pCallback = pCallback;

	return SER_NO_ERROR;
}

/*
===========================================================================
ser_set_monitor_callback:	Set the callback routine for reporting traffic
							to a monitor window.
===========================================================================
*/
int ser_set_monitor_callback(ser_monitor_cb pCallback)
{
	// Save callback address
	sp.pMonCallback = pCallback;

	return SER_NO_ERROR;
}

/*
===========================================================================
ser_set_flags:	Set the serial port's RTS and DTR flags

===========================================================================
*/
int ser_set_signals(unsigned char flags)
{
	if (setup.com_mode == SETUP_COM_HOST)
	{
		#ifdef WIN32
			// Update DTR flag
			if (flags & SER_SIGNAL_DTR)
			{
				sp.dtrState = DTR_CONTROL_ENABLE;
				EscapeCommFunction(sp.hComm, SETDTR);
			}
			else
			{
				sp.dtrState = DTR_CONTROL_DISABLE;
				EscapeCommFunction(sp.hComm, CLRDTR);
			}

			// Update RTS flag
			if (flags & SER_SIGNAL_RTS)
			{
				sp.rtsState = RTS_CONTROL_ENABLE;
				EscapeCommFunction(sp.hComm, SETRTS);
			}
			else
			{
				sp.rtsState = RTS_CONTROL_DISABLE;
				EscapeCommFunction(sp.hComm, CLRRTS);
			}

		#else

			// ============================================
			// Add code here for POSIX to set DTR/RTS
			// ============================================

		#endif
	}

	// Check for a monitor window and report change
	if (sp.pMonCallback != NULL)
		sp.pMonCallback(SER_MON_COM_SIGNAL, (char)((flags>>8) | 
		(flags & (SER_FLAG_DSR | SER_FLAG_CTS))));

	return SER_NO_ERROR;
}

/*
===========================================================================
ser_get_flags:	Get serial port's flags

===========================================================================
*/
int ser_get_flags(unsigned char *flags)
{
	long modem_status;

	if (setup.com_mode == SETUP_COM_HOST)
	{
		if (sp.open_flag == 0)
		{
			*flags = 0;
			return SER_NO_ERROR;
		}

		#ifdef WIN32
			if (!GetCommModemStatus(sp.hComm, &modem_status))
			{
				*flags = 0;
				if (sp.tx_empty)
					*flags |= SER_FLAG_TX_EMPTY;
				return SER_IO_ERROR;
			}
			
			*flags = 0;

			// Set CTS flag
			if ((modem_status & MS_CTS_ON) == 0)
				*flags |= SER_FLAG_CTS;

			// Set DSR flag
			if ((modem_status & MS_DSR_ON) == 0)
				*flags |= SER_FLAG_DSR;

			// RING flag
			if (modem_status & MS_RING_ON)
				*flags |= SER_FLAG_RING
				;
			// TX_EMPTY flag
			if (sp.tx_empty)
				*flags |= SER_FLAG_TX_EMPTY;

			// OVERRUN flag

			// FRAMING Error flag

			if ((sp.rxIn != sp.rxOut) && (sp.fIntPending == 0))
			{
				if (sp.pCallback != NULL)
				{
					sp.pCallback();
					sp.fIntPending = TRUE;
				}
			}
		#else

			// ============================================
			// Add code here for POSIX to read flags
			// ============================================

		#endif
	}

	return SER_NO_ERROR;
}

/*
===========================================================================
ser_get_signals:	Get serial port's CTS, RTS, DTR, and DSR signals

===========================================================================
*/
int ser_get_signals(unsigned char *flags)
{
	long modem_status;

	if (setup.com_mode == SETUP_COM_HOST)
	{
		if (sp.open_flag == 0)
		{
			*flags = 0;
			return SER_NO_ERROR;
		}

		#ifdef WIN32
			if (!GetCommModemStatus(sp.hComm, &modem_status))
			{
				*flags = 0;
				return SER_IO_ERROR;
			}
			
			*flags = 0;

			// Set CTS flag
			if ((modem_status & MS_CTS_ON) == 1)
				*flags |= SER_SIGNAL_CTS;

			// Set DSR flag
			if ((modem_status & MS_DSR_ON) == 1)
				*flags |= SER_SIGNAL_DSR;

			// Set RTS flag
			if (sp.rtsState == RTS_CONTROL_ENABLE)
				*flags |= SER_SIGNAL_RTS;

			// Set DTR flag
			if (sp.dtrState == DTR_CONTROL_ENABLE)
				*flags |= SER_SIGNAL_DTR;

		#else

			// ============================================
			// Add code here for POSIX to read flags
			// ============================================

		#endif
	}

	return SER_NO_ERROR;
}

/*
===========================================================================
ser_read_byte:	Read byte from the serial port

===========================================================================
*/
int ser_read_byte(char* data)
{
	// Check if a port is open, return if not
	if (sp.open_flag == FALSE)
	{
		*data = 0;
		return SER_NO_ERROR;
	}

	// Check if COM emulation type is Host port
	if (setup.com_mode == SETUP_COM_HOST)
	{
		#ifdef WIN32
			int new_rxOut;

#ifdef WIN32
			WaitForSingleObject(sp.hReadMutex, 2000);
#endif
			// Check if any bytes in RX buffer
			if (sp.rxIn == sp.rxOut)
			{
#ifdef WIN32
				ReleaseMutex(sp.hReadMutex);
#endif
				// No data in buffer!
				*data = 0;
				// Check if there is a monitor window open and report the write
				if (sp.pMonCallback != NULL)
					sp.pMonCallback(SER_MON_COM_READ, *data);
				return SER_NO_DATA;
			}

			*data = sp.rx_buf[sp.rxOut];

			new_rxOut = sp.rxOut + 1;
			if (new_rxOut >= sizeof(sp.rx_buf))
				new_rxOut = 0;
			sp.rxOut = new_rxOut;

#ifdef WIN32
			ReleaseMutex(sp.hReadMutex);
#endif
			sp.fIntPending = FALSE;

		#else

			// ============================================
			// Add code here for POSIX to write data
			// ============================================

		#endif
	}

	// Check if there is a monitor window open and report the write
	if (sp.pMonCallback != NULL)
		sp.pMonCallback(SER_MON_COM_READ, *data);

	return SER_NO_ERROR;
}

/*
===========================================================================
ser_write_byte:	Write a byte to the serial port

===========================================================================
*/
int ser_write_byte(char data)
{
	// Check if a port is open, return if not
	if (sp.open_flag == FALSE)
		return SER_NO_ERROR;

	// Check if COM port emulation configured for HOST port
	if (setup.com_mode == SETUP_COM_HOST)
	{
		#ifdef WIN32
			// Store data in structure
#ifdef WIN32
			WaitForSingleObject(sp.hWriteMutex, 2000);
#endif
			sp.tx_buf[0] = data;
//			sp.tx_buf[sp.txIn++] = data;
//			if (sp.txIn >= sizeof(sp.tx_buf))
//				sp.txIn = 0;

			// Update tx_empy flag indicating byte to write
			sp.tx_empty = FALSE;

#ifdef WIN32
			ReleaseMutex(sp.hWriteMutex);
#endif

			// Trigger the thread to write the byte
			SetEvent(sp.hWriteEvent);

		#else

			// =================================================
			// Add code here for POSIX to write data (Host Mode)
			// =================================================

		#endif
	}

	// Check if there is a monitor window open and report the write
	if (sp.pMonCallback != NULL)
		sp.pMonCallback(SER_MON_COM_WRITE, data);

	return SER_NO_ERROR;
}

