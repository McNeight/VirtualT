/* tpddserver.h */

/* $Id: tpddserver.h,v 1.1 2013/02/20 20:47:47 kpettit1 Exp $ */

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

#ifndef TPDDSERVER_H
#define TPDDSERVER_H

/*
============================================================================
Define call routines to hook to serial port functionality.
============================================================================
*/
// If compiled in cpp file, make declarations extern "C"
#ifdef __cplusplus

#include <FL/filename.H>
#include "MString.h"
#include "MStringArray.h"

extern "C"
{
#endif /* __cplusplus */

// Now define the function prototypes
void*			tpdd_alloc_context(void);
void			tpdd_free_context(void* pContext);
void			tpdd_save_prefs(void* pContext);
void			tpdd_load_prefs(void* pContext);
int				tpdd_close_serial(void* pContext);
int				tpdd_open_serial(void* pContext);
int				tpdd_ser_set_baud(void* pContext, int baud_rate);
int				tpdd_ser_get_flags(void* pContext, unsigned char *flags);
int				tpdd_ser_set_signals(void* pContext, unsigned char flags);
int				tpdd_ser_get_signals(void* pContext, unsigned char *signals);
int				tpdd_ser_read_byte(void* pContext, char* data);
int				tpdd_ser_write_byte(void* pContext, char data);
int				tpdd_ser_poll(void* pContext);

const char*		tpdd_get_port_name(void* pContext);

// Close the extern "C" block if compiling a cpp file
#ifdef __cplusplus
}

// Define CPP only prototypes 
void			tpdd_server_config(void);

class VTTpddServer;
class VTTpddServerLog;

// Typedef for command line and opcode handler
typedef int (VTTpddServer::*VTNADSCmdlineFunc)(int background);
typedef void (VTTpddServer::*VTTpddOpcodeFunc)(void);

typedef struct VT_NADSCmd
{
	const char*			pCmd;
	VTNADSCmdlineFunc	pFunc;
	const char*			pHelp;
} VT_NADSCmd_t;

// Define offsets within m_rxBuffer of various TPDD fields
#define		TPDD_PKT_OPCODE_INDEX		2
#define		TPDD_PKT_LEN_INDEX			3
#define		TPDD_PKT_DATA_INDEX			4		// Location in rxBuffer of TPDD packet data

#define		TPDD_MAX_FDS				4

// Base TPDD requests
#define		TPDD_REQ_DIR_REF			0x00
#define		TPDD_REQ_OPEN_FILE			0x01
#define		TPDD_REQ_CLOSE_FILE			0x02
#define		TPDD_REQ_READ_FILE			0x03
#define		TPDD_REQ_WRITE_FILE			0x04
#define		TPDD_REQ_DELETE_FILE		0x05
#define		TPDD_REQ_FORMAT				0x06
#define		TPDD_REQ_DRIVE_STATUS		0x07
#define		TPDD_REQ_ID					0x08
#define		TPDD_REQ_DRIVE_CONDITION	0x0C
#define		TPDD_REQ_RENAME_FILE		0x0D

// Extended requests for potential future Model "T" DOS usage
#define		TPDD_REQ_SEEK				0x09
#define		TPDD_REQ_TELL				0x0A
#define		TPDD_REQ_SET_EXTENDED		0x0B
#define		TPDD_REQ_QUERY_EXTENDED		0x0E
#define		TPDD_REQ_CONDENSED_LIST		0x0F
#define		TPDD_REQ_LAST_OPCODE		0x0F

#define		TPDD_REQ_TSDOS_MYSTERY23	0x23
#define		TPDD_REQ_TSDOS_MYSTERY31	0x31

// Expanded requests for potential future Model "T" DOS usage
#define		TPDD_REQ_SEEK				0x09
#define		TPDD_REQ_TELL				0x0A
#define		TPDD_REQ_SET_EXTENDED		0x0B
#define		TPDD_REQ_QUERY_EXTENDED		0x0E
#define		TPDD_REQ_CONDENSED_LIST		0x0F
#define		TPDD_REQ_LAST_OPCODE		0x0F

// Define OPEN mode constants here
#define		TPDD_OPEN_MODE_WRITE		0x01
#define		TPDD_OPEN_MODE_APPEND		0x02
#define		TPDD_OPEN_MODE_READ			0x03
#define		TPDD_OPEN_MODE_READ_WRITE	0x04

// Return commands
#define		TPDD_RET_READ_FILE			0x10
#define		TPDD_RET_DIR_REF			0x11
#define		TPDD_RET_NORMAL				0x12
#define		TPDD_RET_TELL				0x13
#define		TPDD_RET_QUERY_EXTENDED		0x14
#define		TPDD_RET_CONDITION			0x15
#define		TPDD_RET_CONDENSED_LIST		0x16

// Normal Return Error Codes
#define		TPDD_ERR_NONE				0x00
#define		TPDD_ERR_NONEXISTANT_FILE	0x10
#define		TPDD_ERR_FILE_EXISTS		0x11
#define		TPDD_ERR_NO_FILENAME		0x30
#define		TPDD_ERR_DIR_SEARCH_ERR		0x31
#define		TPDD_ERR_BANK_ERR			0x35
#define		TPDD_ERR_PARAM_ERR			0x36
#define		TPDD_ERR_OPEN_FMT_MISMATCH	0x37
#define		TPDD_ERR_END_OF_FILE		0x3f
#define		TPDD_ERR_NO_START_MARK		0x40
#define		TPDD_ERR_ID_CRC_CHK_ERR		0x41
#define		TPDD_ERR_SECTOR_LEN_ERR		0x42
#define		TPDD_ERR_FORMAT_VERIF_ERR	0x44
#define		TPDD_ERR_FORMAT_INTERRUPT	0x46
#define		TPDD_ERR_ERASE_OFFSET_ERR	0x47
#define		TPDD_ERR_DATA_CRC_CHK_ERR	0x49
#define		TPDD_ERR_READ_DATA_TIMEOUT	0x4b
#define		TPDD_ERR_SECTOR_NUM_ERR		0x4d
#define		TPDD_ERR_DISK_WRITE_PROT	0x50
#define		TPDD_ERR_UNINITALIZED_DISK	0x5e
#define		TPDD_ERR_DIRECTORY_FULL		0x60
#define		TPDD_ERR_DISK_FULL			0x61
#define		TPDD_ERR_FILE_TOO_LONG		0x6e
#define		TPDD_ERR_NO_DISK			0x70
#define		TPDD_ERR_DISK_CHANGE_ERR	0x71

// Define Seek codes
#define		TPDD_SEEK_SET				1
#define		TPDD_SEEK_CUR				2
#define		TPDD_SEEK_END				3

// Define CMdline flags
#define		CMD_DIR_FLAG_WIDE			0x0001
#define		CMD_DIR_FLAG_DIR			0x0002

// Logging defines
#define		TPDD_LOG_RX					0
#define		TPDD_LOG_TX					1

/*
=====================================================================
Define the TPDD Server class.  This will be passed around in C land
as a void* context.
=====================================================================
*/
class VTTpddServer 
{
public: 
	VTTpddServer();
	~VTTpddServer();

	// Methods
	int				SerOpenPort(void);
	int				SerClosePort(void);
	const char*		SerGetPortName(void);
	int				SerSetBaud(int baud_rate);
	int				SerGetFlags(unsigned char *flags);
	int				SerSetSignals(unsigned char signals);
	int				SerGetSignals(unsigned char *signals);
	int				SerReadByte(char *data);
	int				SerWriteByte(char data);
	int				SerPoll(void);

	void			RootDir(const char *pDir);	// Set the root emulation directory
	const char*		RootDir(void) { return (const char *) m_sRootDir; }

	int				ChangeDirectory(const char *pDir);
	int				ChangeDirectory(MString& sDir);

	void			RegisterServerLog(VTTpddServerLog* pServerLog) { m_pServerLog = pServerLog;
																	 m_logEnabled = TRUE; }
	void			UnregisterServerLog(VTTpddServerLog* pServerLog) { m_logEnabled = FALSE;
																	   m_pServerLog = NULL; }
	int				IsCmdlineState(void);

private:
	void			StateCmdline(char data);// Process data while in Cmdline state
	void			ExecuteCmdline(void);	// Executes the command in m_rxBuffer
	void			ExecuteTpddOpcode(void);// Executes the opcode in m_rxBuffer 
											//   (buffer has full packet 'ZZ'+opcode+len+data

	// General functions
	void			ResetDirent(void);		// Delete all entries in m_activeDir
	int				ParseOptions(MString& args, const char* pOptions, int& flags);
	void			LogData(char data, int rxTx);

	// Routines to return data to the server
	void			SendToHost(char data);	// Send a single byte to the host TX buffer
	void			SendToHost(const char *str);	// Send a string to the host
	inline void		TpddSendByte(char data) { m_txChecksum += (unsigned char) data; SendToHost(data); }
	inline void		TpddSendChecksum(void) { SendToHost(m_txChecksum ^ 0xFF); }
	void			SendNormalReturn(unsigned char errCode);
	void			SendDirEntReturn(dirent* e, int dir_search = FALSE);
	void			SendDmeResponse(void);

	// TPDD Execution routines
	void			DirFindFile(const char* pFilename);
	void			DirFindFirst(void);
	void			DirFindNext(void);

	MString			m_sRootDir;				// The path of the root directory
	MString			m_curDir;				// Current subdirectory relative to root
	int				m_baudRate;				// The simulated baud rate
	MString			m_dirRef;				// Directory Reference file within curent directory
	int				m_refIsDirectory;		// Indicates if referenced file is a directory
	FILE*			m_refFd[TPDD_MAX_FDS];	// Referenced FD
	int				m_mode[TPDD_MAX_FDS];	// Open mode for each of the FDs
	int				m_activeFd;				// Index of active FD
	MString			m_cmd;					// The command that is executing
	MStringArray	m_args;					// Cmdline arguments
	VTNADSCmdlineFunc	m_backgroundCmd;	// Cmdline command executing in the "background"
	int				m_backgroundFlags;		// Argument flags for background command
	dirent**		m_activeDir;			// Active directory listing
	dirent**		m_dirDir;				// Directory listing for 'dir' command
	int				m_dirCount;				// Count of entries in m_activeDir
	int				m_dirDirCount;			// Count of entries in m_dirDir
	int				m_dirNext;				// Next entry in m_activeDir to send to client
	int				m_dirDirNext;			// Next entry in m_dirDir to send to client
	int				m_tsdosDmeStart;		// Indicates start of TS-DOS DME request
	int				m_tsdosDmeReq;			// Indicates 2nd packet of DME request received
	int				m_lastWasRx;			// Indicates if last logged byte was RX
	int				m_logEnabled;			// Set TRUE when logging enabled
	int				m_lastOpenWasDir;		// Indicates if last open was a directory change
	VTTpddServerLog*	m_pServerLog;		// Server log object for logging data

	int				m_rxCount;				// Number of bytes in RX buffer (received from Model T)
	int				m_txCount;				// Number of bytes in TX buffer (to be sent to Model T)
	char			m_rxBuffer[1024];		// Rx Buffer (received from Model T)
	char			m_txBuffer[4096];		// Tx BUffer (to be sent to Model T)
	int				m_rxIn;
	int				m_txIn;
	int				m_txOut;
	int				m_txOverflow;			// Set true if TX buffer overflows
	int				m_rxOverflow;			// Set true if RX buffer overflows

	int				m_state;				// The state of the TPDD protocol engine
	int				m_opcode;				// Opcode receivd from client
	int				m_length;				// Length byte received from client
	int				m_bytesRead;			// Number of bytes read from client
	unsigned char	m_rxChecksum;			// Received checksum from client
	unsigned char	m_calcChecksum;			// Calculated checksum
	unsigned char	m_txChecksum;			// Checksum of TX packet

	UINT64			m_lastReadCycles;		// Cycle count the last time a byte was sent
	UINT64			m_cycleDelay;			// Number of cycles to delay between bytes

	// Array of emulated NADSBox cmdline commands
	static	VT_NADSCmd_t	m_Cmds[5];		// Array of our command line commands
	static	VT_NADSCmd_t	m_LinuxCmds[7];	// Array of alias commands for cat, cp, ls, mv, rm
	static	int		m_cmdCount;				// Count of commands
	static	int		m_linuxCmdCount;		// Count of linux aliases

	// Array of TPDD opcode handler functions (prototype looks like Cmdline func)
	static VTTpddOpcodeFunc	m_tpddOpcodeHandlers[16];
	static int		m_tpddOpcodeCount;		// Countof opcode handlers

	// Our command line handler functions
	int				CmdlineHelp(int);		// Handler for "help" command
	int				CmdlineVer(int);		// Handler for "ver" command
	int				CmdlineCd(int);			// Handler for "cd" command
	int				CmdlineDir(int);		// Handler for "dir" command
	int				CmdlinePwd(int);		// Handler for "pwd" command
	int				CmdlineRmdir(int);		// Handler for "rmdir" command
	int				CmdlineMkdir(int);		// Handler for "mkdir" command
	int				CmdlineDel(int);		// Handler for "del" command
	int				CmdlineCopy(int);		// Handler for "copy" command
	int				CmdlineRen(int);		// Handler for "ren" command
	int				CmdlineTrace(int);		// Handler for "trace" command
	int				CmdlineType(int);		// Handler for "type" command
	int				CmdlineDate(int);		// Handler for "date" command
	int				CmdlineTime(int);		// Handler for "time" command
	int				CmdlineInfo(int);		// Handler for "info" command

	// Define our TPDD opcode handler functions
	virtual void	OpcodeDir(void);		// Handler for DIR REFERENCE opcode
	virtual void	OpcodeOpen(void);		// Handler for Open opcode
	virtual void	OpcodeClose(void);		// Handler for Close opcode
	virtual void	OpcodeRead(void);		// Handler for Read opcode
	virtual void	OpcodeWrite(void);		// Handler for Write opcode
	virtual void	OpcodeDelete(void);		// Handler for Delete opcode
	virtual void	OpcodeFormat(void);		// Handler for Format opcode
	virtual void	OpcodeId(void);			// Handler for ID opcode
	virtual void	OpcodeDriveStatus(void);// Handler for Drive Status opcode
	virtual void	OpcodeDriveCond(void);	// Handler for Drive Condition opcode
	virtual void	OpcodeRename(void);		// Handler for Rename opcode

	// Extended opcodes
	virtual void	OpcodeSeek(void);		// Handler for Seek opcode
	virtual void	OpcodeTell(void);		// Handler for Tell opcode
	virtual void	OpcodeSetExtended(void);// Handler for SetExtended opcode
	virtual void	OpcodeQueryExtended(void);// Handler for QueryExtended opcode
	virtual void	OpcodeCondensedList(void);// Handler for CondensedList opcode

	// Mystery opcodes
	virtual void	OpcodeMystery23(void);	// Handler for the mystery 0x23 opcode
	virtual void	OpcodeMystery31(void);	// Handler for the mystery 0x31 opcode

};

#endif  /* __cplusplus */

#endif	/* TPDDSERVER_H */

