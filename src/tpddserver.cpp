/* tpddserver.cpp */

/* $Id: tpddserver.cpp,v 1.1 2013/02/20 20:47:47 kpettit1 Exp $ */

/*
 * Copyright 2013 Ken Pettit
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
//#include <ctype.h>
#include <stdio.h>
#include <string.h>
#ifdef _WIN32
#include <windows.h>
#include <direct.h>
#else
#include <sys/stat.h>
#endif

#include <FL/Fl.H>
#include <FL/Fl_Preferences.H>
#include <FL/Fl_Button.H>
#include <FL/Fl_Return_Button.H>
#include <FL/Fl_Box.H>
#include <FL/Fl_Input.H>

#include "FLU/Flu_File_Chooser.h"

#include "VirtualT.h"
#include "m100emu.h"
#include "tpddserver.h"
#include "serial.h"
#include "display.h"
#include "cpuregs.h"
#include "periph.h"
#include "memedit.h"
#include "fileview.h"

enum {
	TPDD_STATE_IDLE = 0,
	TPDD_STATE_Z,
	TPDD_STATE_OPCODE,
	TPDD_STATE_LEN,
	TPDD_STATE_READ_BYTES,
	TPDD_STATE_READ_CHECKSUM,
	TPDD_STATE_M,
	TPDD_STATE_M1,
	TPDD_STATE_R,
	TPDD_STATE_CMDLINE,
	TPDD_STATE_EXEC
};

// ===============================================
// Extern "C" linkage items
// ===============================================
extern "C"
{
extern volatile UINT64			cycles;
extern volatile DWORD			rst7cycles;

#ifdef WIN32
int strcasecmp(const char* s1, const char* s2);
#endif
}

/*
======================================================
Structure for the setup GUI controls.
======================================================
*/
typedef struct tpddserver_ctrl_struct
{
	Fl_Input*				pRootDir;			// Pointer to root dir input
	char					sRootDir[512];
	Fl_Button*				pTpddDir;			// Create TPDD directory listing
} tpddserver_ctrl_t;

// ===============================================
// Extern and global variables
// ===============================================
extern	Fl_Preferences	virtualt_prefs;
static	Fl_Window*		gtcw = NULL;
tpddserver_ctrl_t		gTsCtrl;

// Define the array of NADSBox commands
VT_NADSCmd_t VTTpddServer::m_Cmds[] = {
	{ "cd",		&VTTpddServer::CmdlineCd },
//	{ "copy",	&VTTpddServer::CmdlineCopy },
//	{ "date",	&VTTpddServer::CmdlineDate },
//	{ "del",	&VTTpddServer::CmdlineDel },
	{ "dir",	&VTTpddServer::CmdlineDir },
	{ "help",	&VTTpddServer::CmdlineHelp },
//	{ "info",	&VTTpddServer::CmdlineInfo },
//	{ "mkdir",	&VTTpddServer::CmdlineMkdir },
	{ "pwd",	&VTTpddServer::CmdlinePwd },
//	{ "ren",	&VTTpddServer::CmdlineRen },
//	{ "rmdir",	&VTTpddServer::CmdlineRmdir },
//	{ "time",	&VTTpddServer::CmdlineTime },
//	{ "trace",	&VTTpddServer::CmdlineTrace },
//	{ "type",	&VTTpddServer::CmdlineType },
	{ "ver",	&VTTpddServer::CmdlineVer },
};
int VTTpddServer::m_cmdCount = sizeof(VTTpddServer::m_Cmds) / sizeof(VT_NADSCmd_t);

// Define the array of Linux alias commands
VT_NADSCmd_t VTTpddServer::m_LinuxCmds[] = {
	{ "cat",	&VTTpddServer::CmdlineType },
	{ "cp",		&VTTpddServer::CmdlineCopy },
	{ "ls",		&VTTpddServer::CmdlineDir },
	{ "ll",		&VTTpddServer::CmdlineDir },
	{ "more",	&VTTpddServer::CmdlineType },
	{ "mv",		&VTTpddServer::CmdlineRen },
	{ "rm",		&VTTpddServer::CmdlineDel },
};
int VTTpddServer::m_linuxCmdCount = sizeof(VTTpddServer::m_LinuxCmds) / sizeof(VT_NADSCmd_t);

// Define the array of Linux alias commands
VTTpddOpcodeFunc VTTpddServer::m_tpddOpcodeHandlers[] = {
	&VTTpddServer::OpcodeDir,
	&VTTpddServer::OpcodeOpen,
	&VTTpddServer::OpcodeClose,
	&VTTpddServer::OpcodeRead,
	&VTTpddServer::OpcodeWrite,
	&VTTpddServer::OpcodeDelete,
	&VTTpddServer::OpcodeFormat,
	&VTTpddServer::OpcodeDriveStatus,
	&VTTpddServer::OpcodeId,
};

int VTTpddServer::m_tpddOpcodeCount = sizeof(VTTpddServer::m_tpddOpcodeHandlers) / sizeof(VTNADSCmdlineFunc);

/*
============================================================================
Callback routine for the File Viewer window
============================================================================
*/
void cb_tcwin (Fl_Widget* w, void*)
{
	// Hide the window
	gtcw->hide();

	// Delete the window and set to NULL
	delete gtcw;
	gtcw = NULL;
}

/*
============================================================================
Callback routine to read TPDD / NADSBox directory
============================================================================
*/
static void cb_TpddDir(Fl_Widget* w, void*)
{
	Flu_File_Chooser*	fc;
	char				fc_path[256];
	int					count;
	const char			*filename;

	// Create chooser window to pick file
	strncpy(fc_path, gTsCtrl.pRootDir->value(), sizeof(fc_path));
	fl_cursor(FL_CURSOR_WAIT);
	fc = new Flu_File_Chooser(fc_path, "", Flu_File_Chooser::DIRECTORY, "Choose Root Directory");
	fl_cursor(FL_CURSOR_DEFAULT);
	fc->preview(0);
	fc->show();

	// Show Chooser window
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

	// Copy the new root
	strncpy(gTsCtrl.sRootDir, filename, sizeof(gTsCtrl.sRootDir));
	gTsCtrl.pRootDir->value(gTsCtrl.sRootDir);

	delete fc;
}

/*
============================================================================
Callback routine for the cancel button
============================================================================
*/
static void cb_tpdd_cancel(Fl_Widget* w, void*)
{
	// Hide and delete the window
	gtcw->hide();
	delete gtcw;
	gtcw = NULL;
}

/*
============================================================================
Callback routine for the Ok button
============================================================================
*/
static void cb_tpdd_ok(Fl_Widget* w, void* pOpaque)
{
	// hide the window
	gtcw->hide();

	// Get the updated root directory
	const char *pDir = gTsCtrl.pRootDir->value();

	VTTpddServer* pServer = (VTTpddServer *) ser_get_tpdd_context();
	strcpy(gTsCtrl.sRootDir, pDir);
	pServer->RootDir(pDir);

	// Delete the window
	delete gtcw;
	gtcw = NULL;
}

/*
============================================================================
Routine to create the TPDD Server window
============================================================================
*/
void tpdd_server_config(void)
{
	Fl_Box*			o;
	Fl_Button*		b;

	if (gtcw != NULL)
		return;

	// Create TPDD Server configuration window
	gtcw = new Fl_Double_Window(480, 150, "TPDD / NADSBox Server");
	gtcw->callback(cb_tcwin);

	o = new Fl_Box(20, 20, 80, 20, "Root Directory");
	o->align(FL_ALIGN_LEFT|FL_ALIGN_INSIDE);

	// Create an input field to edit / show the root directory
	gTsCtrl.pRootDir = new Fl_Input(35, 40, 325, 20, "");
	VTTpddServer* pServer = (VTTpddServer *) ser_get_tpdd_context();
	strcpy(gTsCtrl.sRootDir, pServer->RootDir());
	gTsCtrl.pRootDir->value(gTsCtrl.sRootDir);

	// Create a browse button for the root directory
	gTsCtrl.pTpddDir = new Fl_Button(380, 40, 80, 20, "Browse");
	gTsCtrl.pTpddDir->callback(cb_TpddDir);

	// Create a Cancel button
	b = new Fl_Button(200, 120, 80, 20, "Cancel");
	b->align(FL_ALIGN_CENTER | FL_ALIGN_INSIDE);
	b->callback(cb_tpdd_cancel);

	// Create an OK button
	b = new Fl_Return_Button(300, 120, 80, 20, "OK");
	b->align(FL_ALIGN_CENTER | FL_ALIGN_INSIDE);
	b->callback(cb_tpdd_ok);

	gtcw->show();
}

/*
===========================================================================
Define Serial port interface "C" routines below
===========================================================================
*/
extern "C"
{

/*
===========================================================================
tpdd_alloc_context:	Allocates a context to define and control the TPDD
					server routines.
===========================================================================
*/
void* tpdd_alloc_context(void)
{
	return (void *) new VTTpddServer;
}

/*
===========================================================================
tpdd_free_context:	Frees the given context.
===========================================================================
*/
void tpdd_free_context(void* pContext)
{
	// Validate the context is not null
	if (pContext != NULL)
	{
		VTTpddServer* pTpdd = (VTTpddServer *) pContext;
		delete pTpdd;
	}
}

/*
===========================================================================
tpdd_load_prefs:	Loads the tpdd server preferences from the user file.
===========================================================================
*/
void tpdd_load_prefs(void* pContext)
{
	char			rootDir[512];
	VTTpddServer*	pServer;

	virtualt_prefs.get("TpddServer_RootDir",  rootDir, path, sizeof(rootDir));
	pServer = (VTTpddServer *) pContext;
	pServer->RootDir(rootDir);
}

/*
===========================================================================
tpdd_save_prefs:	Saves the tpdd server preferences to the user file.
===========================================================================
*/
void tpdd_save_prefs(void* pContext)
{
	VTTpddServer* pServer = (VTTpddServer *) pContext;

	virtualt_prefs.set("TpddServer_RootDir", pServer->RootDir());
}

/*
===========================================================================
tpdd_close_serial:	Closes the simulated TPDD serial session.
===========================================================================
*/
int tpdd_close_serial(void* pContext)
{
	VTTpddServer* pTpdd = (VTTpddServer *) pContext;

	// Let the TPDD Server class handle it
	return pTpdd->SerClosePort();
}

/*
===========================================================================
tpdd_open_serial:	Opens the simulated TPDD serial session.
===========================================================================
*/
int tpdd_open_serial(void* pContext)
{
	VTTpddServer* pTpdd = (VTTpddServer *) pContext;

	// Let the TPDD Server class handle it
	return pTpdd->SerOpenPort();
}

/*
===========================================================================
tpdd_ser_set_baud:	Sets the simulated baudrate for the TPDD server.
===========================================================================
*/
int tpdd_ser_set_baud(void* pContext, int baud_rate)
{
	VTTpddServer* pTpdd = (VTTpddServer *) pContext;

	// Let the TPDD Server class handle it
	return pTpdd->SerSetBaud(baud_rate);
}

/*
===========================================================================
tpdd_ser_get_flags:	Returns the port flags for the TPDD server
===========================================================================
*/
int tpdd_ser_get_flags(void* pContext, unsigned char *flags)
{
	VTTpddServer* pTpdd = (VTTpddServer *) pContext;

	// Let the TPDD Server class handle it
	return pTpdd->SerGetFlags(flags);
}

/*
===========================================================================
tpdd_ser_set_signals:	Sets the simulated port signals for the TPDD server
===========================================================================
*/
int tpdd_ser_set_signals(void* pContext, unsigned char signals)
{
	VTTpddServer* pTpdd = (VTTpddServer *) pContext;

	// Let the TPDD Server class handle it
	return pTpdd->SerSetSignals(signals);
}

/*
===========================================================================
tpdd_ser_get_signals:	Sets the simulated port signals for the TPDD server
===========================================================================
*/
int tpdd_ser_get_signals(void* pContext, unsigned char *signals)
{
	VTTpddServer* pTpdd = (VTTpddServer *) pContext;

	// Let the TPDD Server class handle it
	return pTpdd->SerGetSignals(signals);
}

/*
===========================================================================
tpdd_ser_read_byte:	Reads a byte from the TPDD server
===========================================================================
*/
int tpdd_ser_read_byte(void* pContext, char* data)
{
	VTTpddServer* pTpdd = (VTTpddServer *) pContext;

	// Let the TPDD Server class handle it
	return pTpdd->SerReadByte(data);
}

/*
===========================================================================
tpdd_ser_write_byte:	Writes a byte to the TPDD server
===========================================================================
*/
int tpdd_ser_write_byte(void* pContext, char data)
{
	VTTpddServer* pTpdd = (VTTpddServer *) pContext;

	// Let the TPDD Server class handle it
	return pTpdd->SerWriteByte(data);
}

/*
===========================================================================
tpdd_get_port_name:	Returns a printer to the name of this tpdd port
===========================================================================
*/
const char* tpdd_get_port_name(void* pContext)
{
	VTTpddServer* pTpdd = (VTTpddServer *) pContext;

	// Let the TPDD Server class handle it
	return pTpdd->SerGetPortName();
}

/*
===========================================================================
tpdd_ser_poll:	Poll the TPDD Server to test if it has data available
===========================================================================
*/
int tpdd_ser_poll(void* pContext)
{
	VTTpddServer* pTpdd = (VTTpddServer *) pContext;

	// Let the TPDD Server class handle it
	return pTpdd->SerPoll();
}

}  /* End of extern "C" block */

/*
===========================================================================
VTTpddServer class function implementaion follows.  This is the constructor
===========================================================================
*/
VTTpddServer::VTTpddServer(void)
{
	int		c;

	// Clear out the rx and tx buffers
	m_rxCount = m_txCount = 0;
	m_rxIn = m_rxOut = 0;
	m_txIn = m_txOut = 0;
	for (c = 0; c < TPDD_MAX_FDS; c++)
	{
		m_refFd[c] = NULL;
		m_mode[c] = 0;
	}
	m_lastWasRx = FALSE;
	m_lastOpenWasDir = FALSE;
	m_refIsDirectory = FALSE;
	m_activeFd = 0;
	m_backgroundCmd = NULL;
	m_activeDir = NULL;
	m_dirDir = NULL;
	m_dirCount = m_dirNext = 0;
	m_tsdosDmeReq = m_tsdosDmeStart = FALSE;

	// Reset the TPDD to idle state
	m_state = TPDD_STATE_IDLE;

	// Initialze the working dir and rootpath
	m_curDir = "/";
	m_sRootDir = path;
}

/*
===========================================================================
Class destructor
===========================================================================
*/
VTTpddServer::~VTTpddServer(void)
{
	int			c;

	// Close any open file
	for (c = 0; c < TPDD_MAX_FDS; c++)
	{
		if (m_refFd[c] != NULL)
			fclose(m_refFd[c]);
		m_refFd[c] = NULL;
	}

	// Test if any active dirent list
	ResetDirent();
}

/*
===========================================================================
Set the root emulation directory
===========================================================================
*/
void VTTpddServer::RootDir(const char *pDir)
{
	int		len;

	// Update the root directory
	len = strlen(pDir);
	m_sRootDir = pDir;

	// Remove trailing '/' or '\'
	if (pDir[len-1] == '/' || pDir[len-1] == '\\')
		m_sRootDir = m_sRootDir.Left(len - 1);

	// If there are any open files, then they have to be marked to send
	// a TPDD_ERR_DISK_CHANGE_ERR error
	// TODO:  Do this!!
}

/*
===========================================================================
Opens the TpddServer port
===========================================================================
*/
int VTTpddServer::SerOpenPort(void)
{
	// Clear out the rx and tx buffers
	m_txOverflow = m_rxOverflow = FALSE;
	m_cycleDelay = 100000;
	m_lastReadCycles = cycles;

	return SER_NO_ERROR;
}

/*
===========================================================================
Closes the TpddServer port
===========================================================================
*/
int VTTpddServer::SerClosePort(void)
{
	return SER_NO_ERROR;
}

/*
===========================================================================
Returns the name of the simulated port
===========================================================================
*/
const char* VTTpddServer::SerGetPortName(void)
{
	return (const char *) "TPDD";
}

/*
===========================================================================
Sets the baud rate of the simulated TPDD server
===========================================================================
*/
int VTTpddServer::SerSetBaud(int baud_rate)
{
	// Save the simulated baud rate
	m_baudRate = baud_rate;

	return SER_NO_ERROR;
}

/*
===========================================================================
Sets the simulated port flags
===========================================================================
*/
int VTTpddServer::SerGetFlags(unsigned char *flags)
{
	// Test if we can receive more data
	if (m_rxCount + 1 < sizeof(m_rxBuffer))
		*flags = SER_FLAG_TX_EMPTY;
	else
		*flags &= ~SER_FLAG_TX_EMPTY;

	return SER_NO_ERROR;
}

/*
===========================================================================
Sets the simulated port signals
===========================================================================
*/
int VTTpddServer::SerSetSignals(unsigned char signals)
{
	return SER_NO_ERROR;
}

/*
===========================================================================
Gets the simulated port signals
===========================================================================
*/
int VTTpddServer::SerGetSignals(unsigned char *signals)
{
	*signals = SER_SIGNAL_CTS | SER_SIGNAL_DSR | SER_SIGNAL_DTR | SER_SIGNAL_RTS;

	return SER_NO_ERROR;
}

/*
===========================================================================
Reads a byte from the TPDD Server
===========================================================================
*/
int VTTpddServer::SerReadByte(char *data)
{
	// Test if any data available
	if (m_txCount == 0)
		return SER_NO_DATA;

	// "Read" the next byte from the TX buffer
	*data = m_txBuffer[m_txOut++];
	if (m_txOut >= sizeof(m_txBuffer))
		m_txOut = 0;

#if 0
	if (m_lastWasRx)
		printf("\nTX:  ");
	m_lastWasRx = FALSE;
	printf("%02X ", (unsigned char) *data);
#endif

	// Decrement the count
	m_txCount--;

	// "Kick" transmission of the next block of data from background command
	if (m_backgroundCmd != NULL)
	{
		// Continue execution of the command in the background
		if ((this->*m_backgroundCmd)(TRUE) == 0)
		{
			// Background command terminated
			m_backgroundCmd = NULL;
			SendToHost("> ");
	
			// Clear out the RX path
			m_rxIn = m_rxCount = 0;
			m_state = TPDD_STATE_IDLE;
		}
	}

	// If the count reached zero, then we need to indicate that to the
	// serial routines so it will clear the interrupt
	if (m_txCount == 0)
		return SER_LAST_BYTE;

	// Update the time (cycles) of the last read
	m_lastReadCycles = cycles;

	return SER_NO_ERROR;
}

/*
===========================================================================
Test if any bytes available to read and return appropriate status code
===========================================================================
*/
int VTTpddServer::SerPoll(void)
{
	// Test if any data in the TX buffer
	if (m_txCount == 0)
		return SER_NO_DATA;

	// Test if enough "time" (i.e. cycles) have elapsed
	if (cycles > (UINT64) (m_lastReadCycles + m_cycleDelay))
		return SER_NO_ERROR;

	return SER_NO_DATA;
}

/*
===========================================================================
Sends a single character to the host
===========================================================================
*/
void VTTpddServer::SendToHost(char data)
{
	// Validate the TX buffer isn't full
	if (m_txCount + 1 >= sizeof(m_txBuffer))
	{
		m_txOverflow = TRUE;
		return;
	}

	// Add the byte
	m_txBuffer[m_txIn++] = data;
	if (m_txIn >= sizeof(m_txBuffer))
		m_txIn = 0;
	m_txCount++;
}

/*
===========================================================================
Sends a string of characters to the host
===========================================================================
*/
void VTTpddServer::SendToHost(const char* data)
{
	while (*data != '\0')
		SendToHost(*data++);
}

/*
===========================================================================
Processes received bytes while in the command line state
===========================================================================
*/
void VTTpddServer::StateCmdline(char data)
{
	// Test for CR
	if (data == '\r')
	{
		// Echo CRLF to host
		if (data != 0x1B)
			SendToHost("\r\n");

		// If there's no background command active...
		if (m_backgroundCmd == NULL)
		{
			// Terminate the command and execute it
			m_rxBuffer[m_rxIn] = '\0';
			m_cycleDelay = 400000;
			ExecuteCmdline();
		}

		// Now reset the rxBuffer and go to idle
		m_rxIn = m_rxOut = m_rxCount = 0;
		m_state = TPDD_STATE_IDLE;
		return;
	}
	// Test for BKSP
	else if (data == 0x08)
	{
		// Process the backspace
		if (m_rxCount > 0)
		{
			// Erase the line from the host
			SendToHost("\x08 \x08");
			m_rxCount--;
			m_rxIn--;
		}

		return;
	}

	// We have received part of a command line and we are looking for a CR,
	// but must look for a 'Z' or 'M' after a timeout also
	if (m_rxCount + 1 >= sizeof(m_rxBuffer))
	{
		// Buffer overflow!!!  Go to idle state
		m_rxIn = m_rxOut = m_rxCount = 0;
		m_state = TPDD_STATE_IDLE;
		m_rxOverflow = TRUE;
		return;
	}

	else
	{
		// Add the byte to our rx buffer
		m_rxBuffer[m_rxIn++] = data;
		m_rxCount++;

		// Echo the byte back to the host
		if (data != 0x1B)
			SendToHost(data);
	}
}

/*
===========================================================================
Handles the "cd" command
===========================================================================
*/
int VTTpddServer::CmdlineCd(int background)
{
	MString		dirSave;
	MString		temp;
	MString		subDir;
	int			len, i;

	if (!background)
	{
		// Test if an arguemnt given
		if (m_args.GetSize() == 0)
		{
			SendToHost("no argument given\r\n");
			return FALSE;
		}

		// We could have multiple directories in the arg, such as "../M100"
		// Process them one at a time.  First save the current directory
		// in case of error.
		temp = m_args[0];
		len = temp.GetLength();
		dirSave = m_curDir;

		// Test if directory starts with '/'
		i = 0;
		if (temp[0] == '/')
		{
			i++;
			m_curDir = '/';
		}

		// Get the first subdir specification
		while (i < len && temp[i] != '/')
			subDir += (char) temp[i++];

		// Skip the '/'
		if (i < len)
			i++;

		while (subDir != "")
		{
			// Try to change to this directory
			if (!ChangeDirectory(subDir))
			{
				MString fmt;
				fmt.Format("Dir '%s' does not exist\r\n", (const char *) subDir);
				SendToHost((const char *) fmt);
				m_curDir = dirSave;
				return FALSE;
			}

			// Get next level of subdir
			subDir = "";

			// Get the next subdir spec
			while (i < len && temp[i] != '/')
				subDir += (char) temp[i++];

			// Skip the '/'
			if (i < len)
				i++;
		}
	}

	// Don't need to execute in the background
	return FALSE;
}

/*
===========================================================================
Handles the "dir" command
===========================================================================
*/
int VTTpddServer::CmdlineDir(int background)
{
	int			i, printed;
	MString		file_path;
	int			wide = FALSE;
	MString		fmt;
	int			len, isDir;

	m_cycleDelay = rst7cycles * 70 ;
	if (m_cycleDelay > 1000000)
		m_cycleDelay = 1000000;

	// Test if running in background or not
	if (background)
	{
		// Wait for a key from the user before continuing
		if (m_rxCount == 0)
			return TRUE;

		// Key pressed.  Test for ESC or CTRL-C
		for (i = 0; i < m_rxCount; i++)
		{
			// Test if this character is CTRL-C or ESC
			if (m_rxBuffer[i] == 0x03 || m_rxBuffer[i] == 0x1B || m_rxBuffer[i] == 'q')
			{
				// Yep, the user terminated the listing
				for( i = 0; i < m_dirDirCount; i++ )
					free((void*) (m_dirDir[i]));
				free((void*) m_dirDir);
				m_dirDir = NULL;
				SendToHost("\r\n");
				return FALSE;
			}
		}

		// Terminate the message line
		m_rxIn = m_rxCount = 0;
		SendToHost("\r                                      \r");
	}
	else
	{
		// Get a listing of the current directory
		if (m_dirDir != NULL)
		{
			// Free the directory listing
			for( i = 0; i < m_dirDirCount; i++ )
				free((void*) (m_dirDir[i]));
			free((void*) m_dirDir);
		}

		// Test if any args given
		m_backgroundFlags = 0;
		if (m_args.GetSize() == 0)
		{
			Fl_File_Sort_F *sort = fl_casealphasort;

			// Get a listing of files in the current directory
			file_path = m_sRootDir + m_curDir;
			m_dirDirCount = fl_filename_list((const char *) file_path, &m_dirDir, sort);
		}
		else
		{
			SendToHost("Arguments not supported yet\r\n");
			m_rxIn = m_rxCount = 0;
			return FALSE;
		}

		// Start at beginning of list
		m_dirDirNext = 0;
	}

	// Display a page of entries
	printed = 0;

	for (i = m_dirDirNext; i < m_dirDirCount; i++)
	{
		const char* name = m_dirDir[i]->d_name;
		MString		printName;

		// ignore the "." and ".." names
		if ( strcmp( name, "." ) == 0 || strcmp( name, ".." ) == 0 ||
			strcmp( name, "./" ) == 0 || strcmp( name, "../" ) == 0 ||
			strcmp( name, ".\\" ) == 0 || strcmp( name, "..\\" ) == 0 )
		{
			continue;
		}

		// Test if this is a directory
		isDir = FALSE;
		if (name[strlen(name)-1] == '/')
			isDir = TRUE;

		// Get length of file
		if (!isDir)
		{
			// Open the file and seek to the end
			MString temp = m_sRootDir + m_curDir + name;
			FILE*   fd = fopen((const char *) temp, "r");
			len = 0;

			// Seek and tell only if file opened
			if (fd != NULL)
			{
				fseek(fd, 0, SEEK_END);
				len = ftell(fd);
				fclose(fd);
			}
		}

		// Print this entry
		if ((m_backgroundFlags & CMD_DIR_FLAG_WIDE) == 0)
		{
			// Not in wide display format
			if (!isDir)
			{
				fmt.Format("%-20s%8d\r\n", name, len);
				fmt.MakeUpper();
			}
			else
			{
				MString temp = name;
				temp = temp.Left(temp.GetLength()-1);
				temp.MakeUpper();
				fmt.Format("%-17s<dir>\r\n", (const char *) temp);
			}
			SendToHost((const char *) fmt);
			printed++;
		}

		// Test if a page printed
		if (gModel == MODEL_T200)
		{
			if (printed == 15)
			{
				i++;
				break;
			}
		}
		else if (printed == 7)
		{
			i++;
			break;
		}
	}

	// Save index for next listing
	m_dirDirNext = i;

	// Test if all entries printed
	if (i == m_dirDirCount)
	{
		// Free the directory listing
		for( i = 0; i < m_dirDirCount; i++ )
			free((void*) (m_dirDir[i]));
		free((void*) m_dirDir);
		m_dirDir = NULL;

		// Don't need to execute in the background
		return FALSE;
	}

	// We have more to display.  Present a message
	SendToHost("Any key to continue, 'q' to stop...");

	// Need to execute in the background
	return TRUE;
}

/*
===========================================================================
Handles the "rmdir" command
===========================================================================
*/
int VTTpddServer::CmdlineRmdir(int background)
{
	// Don't need to execute in the background
	return FALSE;
}

/*
===========================================================================
Handles the "Mkdir" command
===========================================================================
*/
int VTTpddServer::CmdlineMkdir(int background)
{
	// Don't need to execute in the background
	return FALSE;
}

/*
===========================================================================
Handles the "del" command
===========================================================================
*/
int VTTpddServer::CmdlineDel(int background)
{
	// Don't need to execute in the background
	return FALSE;
}

/*
===========================================================================
Handles the "copy" command
===========================================================================
*/
int VTTpddServer::CmdlineCopy(int background)
{
	// Don't need to execute in the background
	return FALSE;
}

/*
===========================================================================
Handles the "ren" command
===========================================================================
*/
int VTTpddServer::CmdlineRen(int background)
{
	// Don't need to execute in the background
	return FALSE;
}

/*
===========================================================================
Handles the "trace" command
===========================================================================
*/
int VTTpddServer::CmdlineTrace(int background)
{
	// Don't need to execute in the background
	return FALSE;
}

/*
===========================================================================
Handles the "type" command
===========================================================================
*/
int VTTpddServer::CmdlineType(int background)
{
	// Don't need to execute in the background
	return FALSE;
}

/*
===========================================================================
Handles the "date" command
===========================================================================
*/
int VTTpddServer::CmdlineDate(int background)
{
	// Don't need to execute in the background
	return FALSE;
}

/*
===========================================================================
Handles the "time" command
===========================================================================
*/
int VTTpddServer::CmdlineTime(int background)
{
	// Don't need to execute in the background
	return FALSE;
}

/*
===========================================================================
Handles the "info" command
===========================================================================
*/
int VTTpddServer::CmdlineInfo(int background)
{
	// Don't need to execute in the background
	return FALSE;
}

/*
===========================================================================
Handles the "help" command
===========================================================================
*/
int VTTpddServer::CmdlineHelp(int background)
{
	int			c, cmdsThisLine;

	// Counts the number of commands printed on this line
	cmdsThisLine = 0;

	if (!background)
	{
		// Loop for all commands
		for (c = 0; c < m_cmdCount; c++)
		{
			SendToHost(m_Cmds[c].pCmd);
			if (++cmdsThisLine == 4)
			{
				// Terminate this line
				cmdsThisLine = 0;
				SendToHost("\r\n");
			}
			else
			{
				// Send a tab between commands
				SendToHost("\t");
			}
		}

		// Test if we need a final CRLF
		if (cmdsThisLine != 0)
			SendToHost("\r\n");
	}

	// Don't need to execute in the background
	return FALSE;
}

/*
===========================================================================
Handles the "pwd" command
===========================================================================
*/
int VTTpddServer::CmdlinePwd(int background)
{
	MString temp = m_curDir;

	if (!background)
	{
		temp.MakeUpper();
		if (temp.GetLength() > 1)
			if (temp[temp.GetLength()-1] == '/')
				temp = temp.Left(temp.GetLength()-1);
		SendToHost((const char *) temp);
		SendToHost("\r\n");
	}

	// Don't need to execute in the background
	return FALSE;
}

/*
===========================================================================
Handles the "ver" command
===========================================================================
*/
int VTTpddServer::CmdlineVer(int background)
{
	// Send our Version information to the host
	if (!background)
	{
		SendToHost("VirtualT's emulated NADSBox\r\n");
		SendToHost("Firmware v1.14 equivalent\r\n");
		SendToHost("Copyright 2013, Kenneth D. Pettit\r\n");
	}

	// Don't need to execute in the background
	return FALSE;
}

/*
===========================================================================
Executes the command line command in the m_rxBuffer
===========================================================================
*/
void VTTpddServer::ExecuteCmdline(void)
{
	int		c;
	char*	ptr;
	MString	cmd, arg;

	// Split the m_rxBuffer into command and args
	ptr = m_rxBuffer;

	// Remove previous arguments
	m_args.RemoveAll();

	// Get the command
	while (*ptr != '\0' && *ptr != ' ')
		cmd += *ptr++;

	// Now get the args
	while (*ptr == ' ')
		ptr++;
	while (*ptr != '\0')
	{
		// Clear out the argument
		arg = "";
		while (*ptr != ' ' && *ptr != '\0')
			arg += *ptr++;

		// Add to the args array
		m_args.Add(arg);

		// Skip whitespace
		while (*ptr == ' ')
			ptr++;
	}

	// Loop through all registered commands and see if we have a match
	for (c = 0; c < m_cmdCount; c++)
	{
		// Test if this command matches
		if (strcmp(m_Cmds[c].pCmd, (const char *) cmd) == 0)
		{
			// This command matches the rxBuffer.  Call the command handler
			if ((this->*m_Cmds[c].pFunc)(FALSE))
				m_backgroundCmd = m_Cmds[c].pFunc;
			break;
		}
	}

	// Test if command not foune
	if (c == m_cmdCount)
	{
		// Search the Linux equivalent commands
		for (c = 0; c < m_linuxCmdCount; c++)
		{
			// Test if this command matches
			if (strcmp(m_LinuxCmds[c].pCmd, (const char *) cmd) == 0)
			{
				// This command matches the rxBuffer.  Call the command handler
				if ((this->*m_LinuxCmds[c].pFunc)(FALSE))
					m_backgroundCmd = m_LinuxCmds[c].pFunc;
				break;
			}
		}

		// Test if command not found in Linux alias list either
		if (c == m_linuxCmdCount)
		{
			// Test for "flash" command
			if (strcmp("flash", (const char *) cmd) == 0)
				SendToHost("'flash' not supported in emulation\r\n");
			else if (strcmp("stats", (const char *) cmd) == 0)
				SendToHost("'stats' not supported in emulation\r\n");
			else
				SendToHost("Unknown command\r\n");
		}
	}

	// Send an new prompt
	if (m_backgroundCmd == NULL)
		SendToHost("> ");
}

/*
===========================================================================
Executes the TPDD opcode in the m_rxBuffer
===========================================================================
*/
void VTTpddServer::ExecuteTpddOpcode(void)
{
	// Test opcode for mystery31 or mystery23
	if (m_opcode == TPDD_REQ_TSDOS_MYSTERY23)
		OpcodeMystery23();
	else if (m_opcode == TPDD_REQ_TSDOS_MYSTERY31)
		OpcodeMystery31();

	// Normal TPDD protocol opcodes
	else if (m_opcode <= TPDD_REQ_ID)
	{
		// Call the handler routine for this opcode
//		printf("Executing opcode %d\n", m_opcode);
		(this->*m_tpddOpcodeHandlers[m_opcode])();
	}

	// Unknown opcode
	else
	{
		// Send an error condition
		SendNormalReturn(TPDD_ERR_PARAM_ERR);
	}

	// Reset RX path and go to IDLE state
	m_rxIn = m_rxCount = 0;
	m_state = TPDD_STATE_IDLE;
}

/*
===========================================================================
Writes a byte to the TPDD Server
===========================================================================
*/
int VTTpddServer::SerWriteByte(char data)
{
	int		c;

	// Record the time of this byte so we can perform timeout 
	// operations in the state machine

#if 0
	if (!m_lastWasRx)
		printf("\nRX:  ");
	m_lastWasRx = TRUE;
	printf("%02X ", (unsigned char) data);
#endif

	// Switch based on state. This is the main state machine
	switch (m_state)
	{
	case TPDD_STATE_IDLE:
		// We are in idle state.  Search for 'Z', 'M', or 'R'.  Any lower case
		// chars or CR will take us to command line state
		if (m_tsdosDmeReq && (data == 0x0D))
		{
			// Reset the rx path
			m_rxIn = m_rxCount = 0;
			m_tsdosDmeStart = FALSE;
			if (m_tsdosDmeReq++ == 2)
			{
				// Clear the DME request flag after the 2nd 0x0D
				m_tsdosDmeReq = 0;
			}
			break;
		}
		if (m_tsdosDmeReq)
		{
			// Byte received that wasn't 0x0D.  Clear out the DME
			m_tsdosDmeReq = m_tsdosDmeStart = 0;
			m_rxIn = m_rxCount = 0;
		}

		// Test for command line characters
		if (data >= 'a' && data <= 'z' || m_backgroundCmd)
		{
			// Add the byte to our RX buffer
			m_rxBuffer[m_rxIn++] = data;
			m_rxCount = 1;

			// Add the byte to our TX buffer so we echo it
			if (data != 0x1B)
				SendToHost(data);

			// Clear out any acive TS-DOS DME activity
			m_tsdosDmeStart = FALSE;

			// Goto cmdline state
			m_state = TPDD_STATE_CMDLINE;
		}

		// Test for empty-line CR
		else if (data == '\n' || data == '\r')
		{
			// Send a prompt to the host
			SendToHost("\r\n> ");
		}

		// Search for 'Z'
		else if (data == 'Z')
		{
			// Add the byte to our RX buffer
			m_rxBuffer[m_rxIn++] = data;
			m_rxCount = 1;
			m_state = TPDD_STATE_Z;		// Goto Z state
		}
		else if (data == 'M')
		{
			// Add the byte to our RX buffer
			m_rxBuffer[m_rxIn++] = data;
			m_rxCount = 1;
			m_state = TPDD_STATE_M;		// Goto M state
		}
		else if (data == 'R')
		{
			// Add the byte to our RX buffer
			m_rxBuffer[m_rxIn++] = data;
			m_rxCount = 1;
			m_state = TPDD_STATE_R;		// Goto R state
		}
		break;

	case TPDD_STATE_Z:
		// We expect to see another 'Z', but look for timeout too
		if (data == 'Z')
		{
			// Add the byte to our RX buffer
			m_rxBuffer[m_rxIn++] = data;
			m_rxCount++;

			// Go to the opcode state, we got the 2nd 'Z'
			m_state = TPDD_STATE_OPCODE;
		}
		else
		{
			// Goto the IDLE state and reprocess the byte
			m_state = TPDD_STATE_IDLE;
			m_rxCount = m_rxIn = 0;		// Reset the Rx buffer
			SerWriteByte(data);
		}
		break;

	case TPDD_STATE_OPCODE:
		// We have received 'ZZ', now we are waiting for the opcode byte
		m_opcode = data;

		// Add the byte to our RX buffer
		m_rxBuffer[m_rxIn++] = data;
		m_rxCount++;

		// Go wait for the LEN byte
		m_state = TPDD_STATE_LEN;
		break;

	case TPDD_STATE_LEN:
		// We have received the opcode, now waiting for the length byte
		m_length = (unsigned char) data;
		m_bytesRead = 0;

		// Add the byte to our RX buffer
		m_rxBuffer[m_rxIn++] = data;
		m_rxCount++;

		// Go wait for bytes
		if (m_length == 0)
			m_state = TPDD_STATE_READ_CHECKSUM;
		else
			m_state = TPDD_STATE_READ_BYTES;
		break;

	case TPDD_STATE_READ_BYTES:
		// Add the byte to our RX buffer
		m_rxBuffer[m_rxIn++] = data;

		// Test if all bytes read
		m_rxCount++;
		if (++m_bytesRead == m_length)
		{
			// Goto state to read the checksum
			m_state = TPDD_STATE_READ_CHECKSUM;
		}
		break;

	case TPDD_STATE_READ_CHECKSUM:
		// All bytes read, now read the checksum
		m_rxChecksum = data;

		// Now compute the checksum of received data
		m_calcChecksum = 0;
		for (c = TPDD_PKT_OPCODE_INDEX; c < m_length+4; c++)
		{
			// Calculate checksum across opcode, length and all data
			m_calcChecksum += (unsigned char) m_rxBuffer[c];
		}

		// Compare the checksum
		if ((m_calcChecksum ^ 0xFF) != m_rxChecksum)
		{
			// Error in checksum.  Send error packet
			SendNormalReturn(TPDD_ERR_ID_CRC_CHK_ERR);
		}
		else
		{
			// Execute the TPDD opcode
			m_cycleDelay = 100000;
			ExecuteTpddOpcode();
		}

		// Now reset RX and go to idle
		m_rxIn = m_rxCount = 0;
		m_state = TPDD_STATE_IDLE;
		break;

	case TPDD_STATE_M:
		// We have received 'M' and are waiting for '1' (the 'M1' sequence)
		if (data == '1')
		{
			// Now wait for the 0Dh
			m_rxBuffer[m_rxIn++] = data;
			m_rxCount = 1;
			m_state = TPDD_STATE_M1;
		}
		else
		{
			// Reset RX path and goto idle
			m_rxIn = m_rxCount = 0;
			m_state = TPDD_STATE_IDLE;
		}
		break;

	case TPDD_STATE_M1:
		// We have received 'M1'. This is used for Directory Management Extension
		if (data == 0x0D)
		{
			// TS-DOS DME request initiated.  Save the event
			m_tsdosDmeStart = TRUE;
		}

		// Clear the RX path and goto idle
		m_rxIn = m_rxCount = 0;
		m_state = TPDD_STATE_IDLE;
		break;

	case TPDD_STATE_CMDLINE:
		// Process in the cmdline routine
		StateCmdline(data);
		break;

	default:
		// Unknown state.  Go to idle.
		m_state = TPDD_STATE_IDLE;

		// Reset the RX buffer
		m_rxIn = m_rxOut = m_rxCount = 0;
		break;
	}
	return SER_NO_ERROR;
}

/*
===========================================================================
Sends the specified file entry to the server
===========================================================================
*/
void VTTpddServer::SendDirEntReturn(dirent* e, int dir_entry)
{
	int		c, idx;
	int		send_spaces;

	// Clear the TX checksum
	m_txChecksum = 0;
	TpddSendByte(TPDD_RET_DIR_REF);		// Return code
	TpddSendByte(28);					// Dir Entry pkt length

	// Now send 24 bytes of filename, space padded
	idx = 0;
	send_spaces = FALSE;
	for (c = 0; c < 24; c++)
	{
		// Test if we are sending spaces
		if (send_spaces)
			TpddSendByte(' ');
		else if (e == NULL)
		{
			TpddSendByte(0);
			continue;
		}
		else if ((e->d_name[idx] == '.' || e->d_name[idx] == '/') && c < 6)
		{
			TpddSendByte(' ');
		}
		else if ((e->d_name[idx] == '.' && dir_entry) || e->d_name[idx] == '/')
		{
			// Send the '.' followed by "<>", then send spaces
			TpddSendByte('.');
			TpddSendByte('<');
			TpddSendByte('>');
			c += 2;
			send_spaces = true;

			// Skip idx ahead to the NULL
			while (e->d_name[idx] != '\0')
				idx++;
		}
		else if (e->d_name[idx] == '\0')
		{
			TpddSendByte(' ');
			send_spaces = TRUE;
		}
		else if (e->d_name[idx] == '.' && c < 6)
			TpddSendByte(' ');
		else
			TpddSendByte(toupper(e->d_name[idx++]));
	}

	// Send the Attribute byte 'F'
	if (e == NULL)
		TpddSendByte(0);
	else
		TpddSendByte('F');

	// Send 2-byte length
	int file_len = 0;
	if (!dir_entry && e)
	{
		// Open the file to calculate the length
		MString file_path = m_sRootDir + m_curDir + e->d_name;
		FILE* fd = fopen((const char *) file_path, "r");
		if (fd != NULL)
		{
			// Seek to end of file
			fseek(fd, 0, SEEK_END);
			file_len = ftell(fd);
			fclose(fd);

			// Truncate file length at 64K
			if (file_len > 65535)
				file_len = 65535;
		}
	}

	// Send 2 bytes of length
	TpddSendByte((file_len >> 8) & 0xFF);
	TpddSendByte(file_len & 0xFF);

	// Send the number of free sectors (255)
	TpddSendByte((char) 0x9D);

	// Now send the checksum
	TpddSendChecksum();
}

/*
===========================================================================
Finds the specified file and sends the DIR_REF retrun packet to the server.
===========================================================================
*/
void VTTpddServer::DirFindFile(const char* pFilename)
{
	// read the directory
	dirent**	e;
	char*		name;
	int			i, num, len, dir_search = FALSE, dir_entry;
	MString		file_path;
	char		find_name[16];

	// Test if accessing the "PARENT.<>" file
	if (strcmp(pFilename, "PARENT.<>") == 0)
	{
		// Just save the reference and exit
		m_dirRef = pFilename;
		dirent* pParent = (dirent *) "PARENT.<>";
		SendDirEntReturn(pParent, TRUE);
		return;
	}

	// Construct the path using the current working directory and the root
	file_path = m_sRootDir + m_curDir;
	num = fl_filename_list((const char *) file_path, &e );
	m_refIsDirectory = FALSE;

	// Test if the given filename is a diretory search.  If it is, then
	// the filename will look like "DIRNAM.<>"
	strcpy(find_name, pFilename);
	len = strlen(pFilename);
	if (len > 3 && pFilename[len-1] == '>' && pFilename[len-2] == '<')
	{
		// Indicate we are doing a directory search
		dir_search = TRUE;
		m_refIsDirectory = TRUE;

		// Terminate the filename at the '.'
		find_name[len-3] = '\0';
	}

	// If any files found
	if (num > 0)
	{
		// Loop through all files in the list and perform a 
		// case insensitive compare
		for ( i = 0; i < num; i++ )
		{
			name = e[i]->d_name;

			// ignore the "." and ".." names
			if ( strcmp( name, "." ) == 0 || strcmp( name, ".." ) == 0 ||
				strcmp( name, "./" ) == 0 || strcmp( name, "../" ) == 0 ||
				strcmp( name, ".\\" ) == 0 || strcmp( name, "..\\" ) == 0 )
			{
				continue;
			}

			// if 'name' ends in '/', remove it
			dir_entry = FALSE;
			if ( name[strlen(name)-1] == '/' )
			{
				dir_entry = TRUE;
				name[strlen(name)-1] = '\0';
			}

			// Compare this file with the requested filename
			if (strcasecmp(name, find_name) == 0)
			{				
				// Entry found!  Send the DIR_ENT return
				SendDirEntReturn(e[i], dir_entry);

				// Test if another file was already opened
				if (m_refFd[m_activeFd] != NULL)
				{
					// Close the previously referenced file
					fclose(m_refFd[m_activeFd]);
					m_refFd[m_activeFd] = NULL;
				}

				// Save this reference for future open, delete, etc.
				m_dirRef = name;
				m_refIsDirectory = dir_entry;

				// Now free the list memory
				for( i = 0; i < num; i++ )
					free((void*)(e[i]));
				free((void*)e);

				return;
			}
		}

		// File not found.  Create a new dirent and send the reference
		SendDirEntReturn(NULL, FALSE);
		m_dirRef = pFilename;

		// Now free the list memory
		for( i = 0; i < num; i++ )
			free((void*)(e[i]));
		free((void*)e);
	}
	else
	{
		// Return error
		SendNormalReturn(TPDD_ERR_NONEXISTANT_FILE);
	}
}

/*
===========================================================================
Finds the first entry in the current directory and sends a DirEnt packet
to the server.
===========================================================================
*/
void VTTpddServer::DirFindFirst(void)
{
	// read the directory
	MString		file_path;

	// Construct the path using the current working directory and the root
	file_path = m_sRootDir + m_curDir;
	if (m_activeDir != NULL)
		ResetDirent();
	m_dirCount = fl_filename_list((const char *) file_path, &m_activeDir );
	m_dirNext = 0;

	// Test if we are at the ROOT directory or not.  If not at root, then
	// the first entry is always "PARENT.<>"
	if (m_curDir != "/")
	{
		// Send the "PARENT.<>" directory entry
		dirent*	pParent;
		pParent = (dirent *) "PARENT/";
		SendDirEntReturn(pParent, TRUE);
	}
	else
	{
		// Call the routine to send the next directory entry
		DirFindNext();
	}
}

/*
===========================================================================
Finds the next entry in the current directory and sends a DirEnt packet
to the server.
===========================================================================
*/
void VTTpddServer::DirFindNext(void)
{
	// Don't count the '.' and '..' entries in the count
	while (m_dirNext < m_dirCount)
	{
		const char* name = m_activeDir[m_dirNext]->d_name;
		if ( strcmp( name, "." ) == 0 || strcmp( name, ".." ) == 0 ||
			strcmp( name, "./" ) == 0 || strcmp( name, "../" ) == 0 ||
			strcmp( name, ".\\" ) == 0 || strcmp( name, "..\\" ) == 0 )			
		{
			// Subtract this entry f
			m_dirNext++;
			continue;
		}

		// Send first directory entry
		SendDirEntReturn(m_activeDir[m_dirNext], FALSE);
		m_dirNext++;
		break;
	}

	// Send the next directory entry or all blank
	if (m_dirNext >= m_dirCount)
	{
		// Send a blank entry
		SendDirEntReturn(NULL, FALSE);
	}
}

/*
===========================================================================
Handles the Dir opcode
===========================================================================
*/
void VTTpddServer::OpcodeDir(void)
{
	char*		pFilename;
	char		fat_filename[16];
	int			idx, c;

	// Validate the length byte
	if (m_length != 26)
	{
		SendNormalReturn(TPDD_ERR_PARAM_ERR);
		return;
	}

	// Convert filename from Model T 6.2 format to FAT 8.3 format
	pFilename = &m_rxBuffer[TPDD_PKT_DATA_INDEX];
	idx = 0;
	for (c = 0; c < 6; c++)
		if ((fat_filename[idx] = *pFilename++) != ' ')
			idx++;
	fat_filename[idx++] = '.';
	if (*pFilename == '.')
		pFilename++;
	if ((fat_filename[idx] = *pFilename++) != ' ')
		idx++;
	if ((fat_filename[idx] = *pFilename++) != ' ')
		idx++;
	fat_filename[idx] = '\0';

	// Now process the request based on the search type
	switch (m_rxBuffer[TPDD_PKT_DATA_INDEX + 25])
	{
	case 0:	// Dir find file
		DirFindFile(fat_filename);
		break;

	case 1:	// Find first entry
		DirFindFirst();
		break;

	case 2:	// Find next entry
		DirFindNext();
		break;

	default:
		// Unknown 
		SendNormalReturn(TPDD_ERR_PARAM_ERR);
		break;
	}
}

/*
===========================================================================
Handles the Open opcode
===========================================================================
*/
void VTTpddServer::OpcodeOpen(void)
{
	MString			open_path;
	unsigned char	open_mode;

	// Try to open the referenced file
	open_path = m_sRootDir + m_curDir + m_dirRef;
	open_mode = m_rxBuffer[TPDD_PKT_DATA_INDEX];

	// Check if the referenced file is a directory
	if (m_refIsDirectory)
	{
		// Mark last open as a directory change
		m_lastOpenWasDir = TRUE;

		// Test the open_mode.  If read mod, then CD
		if (open_mode == TPDD_OPEN_MODE_READ)
		{
			// Perform a "Change directory" operation
			if (m_dirRef == "PARENT.<>")
			{
				// Change to parent directory
				ChangeDirectory("..");
				SendNormalReturn(TPDD_ERR_NONE);
			}
			else
			{
				// Change to m_dirRef
				ChangeDirectory(m_dirRef);
				SendNormalReturn(TPDD_ERR_NONE);
			}
		}
		else if (open_mode == TPDD_OPEN_MODE_WRITE)
		{
			MString	temp;
			// Test if we need to remove the ".<>" (likely)
			if (m_dirRef.Right(3) == ".<>")
				temp = m_dirRef.Left(m_dirRef.GetLength()-3);
			else
				temp = m_dirRef;

			// MKDIR operation
			temp.MakeUpper();
			MString dirToMake = m_sRootDir + m_curDir + temp;
#ifdef WIN32
			_mkdir((const char *) dirToMake);
#else
			mkdir((const char *) dirToMake, 0755);
#endif

			// Send a normal return code
			SendNormalReturn(TPDD_ERR_NONE);
		}
	}
	else
	{
		// Open the file based on mode
		if (open_mode == TPDD_OPEN_MODE_WRITE)
			m_refFd[m_activeFd] = fopen((const char *) open_path, "wb");
		else if (open_mode == TPDD_OPEN_MODE_APPEND)
			m_refFd[m_activeFd] = fopen((const char *) open_path, "ab");
		else if (open_mode == TPDD_OPEN_MODE_READ)
			m_refFd[m_activeFd] = fopen((const char *) open_path, "rb");
		else if (open_mode == TPDD_OPEN_MODE_READ_WRITE)
			m_refFd[m_activeFd] = fopen((const char *) open_path, "rb+");

		// Test if the file was opened successfully
		if (m_refFd[m_activeFd] == NULL)
			SendNormalReturn(TPDD_ERR_NONEXISTANT_FILE);
		else
		{
			// Save the open mode
			m_mode[m_activeFd] = open_mode;
			SendNormalReturn(TPDD_ERR_NONE);
		}
	}
}

/*
===========================================================================
Handles the Close opcode
===========================================================================
*/
void VTTpddServer::OpcodeClose(void)
{
	// Test if last "open" was a directory open
	if (m_lastOpenWasDir)
		SendNormalReturn(TPDD_ERR_NONE);

	// Test if the active FD is open
	else if (m_refFd[m_activeFd] == NULL)
		SendNormalReturn(TPDD_ERR_NONEXISTANT_FILE);
	else
	{
		// Close the file and mark it closed
		fclose(m_refFd[m_activeFd]);
		m_refFd[m_activeFd] = NULL;

		// Send normal return
		SendNormalReturn(TPDD_ERR_NONE);
	}
}

/*
===========================================================================
Handles the Read opcode
===========================================================================
*/
void VTTpddServer::OpcodeRead(void)
{
	char	fileData[128];
	int		read_len, c;

	// Validate the file is opened and in the right mode
	if (m_refFd[m_activeFd] == NULL)
		SendNormalReturn(TPDD_ERR_NONEXISTANT_FILE);
	else if (m_mode[m_activeFd] != TPDD_OPEN_MODE_READ &&
		m_mode[m_activeFd] != TPDD_OPEN_MODE_READ_WRITE)
			SendNormalReturn(TPDD_ERR_OPEN_FMT_MISMATCH);
	else
	{
		// File is open and in the correct mode.  Read up to 128 bytes
		read_len = fread(fileData, 1, sizeof(fileData), m_refFd[m_activeFd]);

		// Test for end-of-file
		if (read_len == 0)
			SendNormalReturn(TPDD_ERR_END_OF_FILE);
		else
		{
			// Send the read file packet
			m_txChecksum = 0;
			TpddSendByte(TPDD_RET_READ_FILE);
			TpddSendByte((unsigned char) read_len);

			// Loop for all data to be sent
			for (c = 0; c < read_len; c++)
				TpddSendByte(fileData[c]);

			// Send the checksum
			TpddSendChecksum();
		}
	}
}

/*
===========================================================================
Handles the Write opcode
===========================================================================
*/
void VTTpddServer::OpcodeWrite(void)
{
	int		write_len;

	write_len = m_length;

	// Validate the file is opened and in the right mode
	if (m_refFd[m_activeFd] == NULL)
		SendNormalReturn(TPDD_ERR_NONEXISTANT_FILE);
	else if (m_mode[m_activeFd] == TPDD_OPEN_MODE_READ)
		SendNormalReturn(TPDD_ERR_OPEN_FMT_MISMATCH);
	else
	{
		// File is open and in the correct mode.  Read up to 128 bytes
		write_len = fwrite(&m_rxBuffer[TPDD_PKT_DATA_INDEX], 1, m_length, m_refFd[m_activeFd]);

		// Test for end-of-file
		if (write_len == 0)
			SendNormalReturn(TPDD_ERR_DISK_FULL);
		else
			SendNormalReturn(TPDD_ERR_NONE);
	}
}

/*
===========================================================================
Handles the Delete opcode
===========================================================================
*/
void VTTpddServer::OpcodeDelete(void)
{
}

/*
===========================================================================
Handles the Format opcode
===========================================================================
*/
void VTTpddServer::OpcodeFormat(void)
{
	// Just send a normal return with no error
	SendNormalReturn(TPDD_ERR_NONE);
}

/*
===========================================================================
Handles the Drive Status opcode
===========================================================================
*/
void VTTpddServer::OpcodeDriveStatus(void)
{
	// Just send a normal return with no error
	SendNormalReturn(TPDD_ERR_NONE);
}

/*
===========================================================================
Handles the Id opcode
===========================================================================
*/
void VTTpddServer::OpcodeId(void)
{
	// Test if a TS-DOS DME request was received previously
	if (m_tsdosDmeStart)
	{
		// Send a DME Response and flag to expect a coule of 0x0D
		m_tsdosDmeReq = 1;
		SendDmeResponse();
	}
	else
	{
		// Send the drive ID
		SendToHost("NADSBox");
	}
}

/*
===========================================================================
Handles the Mystery 0x23 opcode
===========================================================================
*/
void VTTpddServer::OpcodeMystery23(void)
{
	const char	op23Table[] = "\x14\x0F\x41\x10\x01\x00\x50\x05\x00\x02\x00\x28\x00\xE1\x00\x00\x00";
	const char	op23Len = sizeof(op23Table)-1;
	int			c;

	// Send bytes from the table
	m_txChecksum = 0;
	for (c = 0; c < op23Len; c++)
	{
		// Send to host and calc checksum
		TpddSendByte(op23Table[c]);
	}

	// Now send the checksum
	TpddSendChecksum();
}

/*
===========================================================================
Handles the Mystery 0x31 opcode
===========================================================================
*/
void VTTpddServer::OpcodeMystery31(void)
{
	// Send mystery 31 response
	m_txChecksum = 0;
	TpddSendByte(0x38);
	TpddSendByte(1);
	TpddSendByte(0);

	// Now send the checksum
	TpddSendChecksum();
}

/*
===========================================================================
Sends a Normal Return packet.  This is a packet with 4 bytes and an ID
of 12, length 1 with an error code and checksum.
===========================================================================
*/
void VTTpddServer::SendNormalReturn(unsigned char errCode)
{
	// Send a Normal Return
	m_txChecksum = 0;
	TpddSendByte(TPDD_RET_NORMAL);
	TpddSendByte(1);
	TpddSendByte(errCode);

	// Now send the checksum
	TpddSendChecksum();
}

/*
===========================================================================
Sends the DME response packet to the server
===========================================================================
*/
void VTTpddServer::SendDmeResponse(void)
{
	MString		dir;
	int			c, len;

	// Send the start of the DME Resp packet
	m_txChecksum = 0;
	TpddSendByte(0x12);			// Opcode
	TpddSendByte(0x0B);			// Length
	TpddSendByte(0x00);			// RESVD 

	// Now send the 6-byte directory name
	if (m_curDir == "/")
		dir = "ROOT  ";
	else
	{
		// Find last '/' in the current directory
		len = m_curDir.GetLength();
		if (m_curDir[len-1] == '/')
			len--;
		for (c = len-1; c >= 0; c--)
			if (m_curDir[c] == '/')
				break;
		c++;

		// Now copy up to 6 bytes of the dir name
		while (dir.GetLength() < 6 && c < len)
			dir += m_curDir[c++];
		// Space pad to 6 characters
		while (dir.GetLength() < 6)
			dir += ' ';
	}

	// Add the ".<>" extension
	dir += (char *) ".<> ";

	// Now send the filename
	len = dir.GetLength();
	for (c = 0; c < len; c++)
		TpddSendByte(dir[c]);

	// Now send the checksum
	TpddSendChecksum();
}

/*
===========================================================================
Change directory to the directory specified
===========================================================================
*/
int VTTpddServer::ChangeDirectory(MString& sDir)
{
	MString		dirName;
	int			idx = 0;
	int			len = sDir.GetLength();
	
	// Strip any trailing ".<>"
	if (sDir.Right(3) == ".<>")
		dirName = sDir.Left(len - 3);
	else
		dirName = sDir;
	dirName.Trim();

	// Now change directory
	return ChangeDirectory((const char *) dirName);
}

/*
===========================================================================
Change directory to the directory specified
===========================================================================
*/
int VTTpddServer::ChangeDirectory(const char *pDir)
{
	int ret = 0;
	MString	find_name;

	// Test if changing to parent directory
	if (strcmp(pDir, "..") == 0)
	{
		// Ensure we aren't at root already
		if (m_curDir != "/")
		{
			// Search backward for the last '/'
			int idx = m_curDir.GetLength() - 2;
			while (idx >= 0 && m_curDir[idx] != '/')
				idx--;
			idx++;
			m_curDir = m_curDir.Left(idx);
			ret = 1;
		}
		else
			ret = 0;
	}
	else if (strcmp(pDir, ".") == 0)
		return 1;
	else
	{
		MString file_path = m_sRootDir + m_curDir;
		dirent** e;
		int num = fl_filename_list((const char *) file_path, &e );
		int i;

		// For directories, append the '/' for the search
		find_name = pDir;
		find_name += '/';

		// If any files found
		if (num > 0)
		{
			// Loop through all files in the list and perform a 
			// case insensitive compare
			for ( i = 0; i < num; i++ )
			{
				const char* name = e[i]->d_name;

				// Compare this file with the requested filename
				if (strcasecmp(name, find_name) == 0)
				{				
					// Directory found!  Append directory to current directory
					m_curDir += name;
					ret = 1;
					break;
				}
			}
		}

		// Free the directory entries
		for (i = 0; i < num; i++)
			free((void*) (e[i]));
		free((void*) e);

#if 0
		// Test if the directory actually exists
		MString testDir = m_sRootDir + m_curDir + pDir;
		if (fl_filename_isdir((const char *) testDir))
		{
			// Append directory to current directory
			m_curDir += pDir;
			m_curDir += (char *) "/";
			ret = 1;
		}
		else
			ret = 0;
#endif
	}

	// Delete existing dirent structure
	ResetDirent();

	return ret;
}

/*
===========================================================================
Delete all entries in m_activeDir
===========================================================================
*/
void VTTpddServer::ResetDirent(void)
{
	int		i;

	// Now free the list memory
	for( i = 0; i < m_dirCount; i++ )
		free((void*) (m_activeDir[i]));
	free((void*) m_activeDir);

	// Clear out the dirent
	m_activeDir = NULL;
	m_dirCount = 0;
}
