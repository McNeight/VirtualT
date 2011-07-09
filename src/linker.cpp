/*
========================================================================
VTLinker:	This class implements an 8085 Linker for the
			VirtualT project.

Written:	11/13/09  Kenneth D. Pettit

========================================================================
*/

#include		"VirtualT.h"
#include		"linker.h"
#include		"elf.h"
#include		"project.h"
#include		<FL/Fl.H>
#include		<math.h>
#include		<string.h>
#include		<stdio.h>
#include		<stdlib.h>

extern "C"
{
#include		"intelhex.h"
char			path[512];
}

static const char *gsEdl = "Error during linking:  ";

/*
============================================================================
Construtor / destructor for the linker
============================================================================
*/
VTLinker::VTLinker()
{
	m_Hex = 0;
	m_FileIndex = -1;
	m_DebugInfo = 0;
}

VTLinker::~VTLinker()
{
	ResetContent();
}

/*
============================================================================
Sets the standard output "printf" function to write error messages to
============================================================================
*/
void VTLinker::SetStdoutFunction(void *pContext, stdOutFunc_t pFunc)
{
	m_pStdoutFunc = pFunc;
	m_pStdoutContext = pContext;
}

/*
============================================================================
This routine free's all memory and initializes all variables for a new
link operation.
============================================================================
*/
void VTLinker::ResetContent(void)
{
	MString			key;
	CObjSegment*	pSeg;
	CLinkRgn*		pLinkRgn;
	MStringArray*	pStrArray;
	POSITION		pos;

	// Loop through each segment
	pos = m_ObjFiles.GetStartPosition();
	while (pos != NULL)
	{
		// Get the next module
		m_ObjFiles.GetNextAssoc(pos, key, (VTObject *&) pSeg);
		delete pSeg;
	}
	m_ObjFiles.RemoveAll();

	// Loop through all LinkRgn objects and delete them
	pos = m_LinkRegions.GetStartPosition();
	while (pos != NULL)
	{
		m_LinkRegions.GetNextAssoc(pos, key, (VTObject *&) pLinkRgn);
		delete pLinkRgn;
	}
	m_LinkRegions.RemoveAll();
	
	// Loop through all Order string arrays and delete them
	pos = m_LinkOrderList.GetStartPosition();
	while (pos != NULL)
	{
		m_LinkOrderList.GetNextAssoc(pos, key, (VTObject *&) pStrArray);
		pStrArray->RemoveAll();
		delete pStrArray;
	}
	m_LinkOrderList.RemoveAll();
	
	// Loop through all Order string arrays and delete them
	pos = m_LinkContainsList.GetStartPosition();
	while (pos != NULL)
	{
		m_LinkContainsList.GetNextAssoc(pos, key, (VTObject *&) pStrArray);
		pStrArray->RemoveAll();
		delete pStrArray;
	}
	m_LinkContainsList.RemoveAll();
	
	// Loop through all Order string arrays and delete them
	pos = m_LinkEndsWithList.GetStartPosition();
	while (pos != NULL)
	{
		m_LinkEndsWithList.GetNextAssoc(pos, key, (VTObject *&) pStrArray);
		pStrArray->RemoveAll();
		delete pStrArray;
	}
	m_LinkEndsWithList.RemoveAll();
	
	// Delete all undefined symbols
	m_UndefSymbols.RemoveAll();

	// Delete all errors
	m_Errors.RemoveAll();

	// Delete filenames parsed
	m_LinkFiles.RemoveAll();
	m_FileIndex = -1;
	m_ObjDirs.RemoveAll();

	// Assign active assembly pointers
	m_Hex = FALSE;
	m_DebugInfo = FALSE;
	m_Map = FALSE;
}

/*
============================================================================
This function processes the argument list that was passed in
============================================================================
*/
void VTLinker::ProcessArgs(MString &str, const char *pDelim)
{
	char*		pStr;
	char*		pTok;
	MString		optStr;

	pStr = new char[str.GetLength() + 1];
	strcpy(pStr, (const char *) str);

	pTok = strtok(pStr, pDelim);

	while (pTok != NULL)
	{
		// Ensure this is an option
		if (*pTok == '-')
		{
			// Skip the hyphen
			pTok++;

			// Test if map file requested
			if (*pTok == 'm')
				m_Map = TRUE;

			// Test if map file requested
			else if (*pTok == 'g')
				m_DebugInfo = TRUE;

			// Test for hex file generation
			else if (*pTok == 'h')
				m_Hex = TRUE;

			// Test for include library
			else if (*pTok == 'l')
			{
				// Extract the library to includein linking
				optStr = pTok + 1;
				m_LinkFiles.Add(optStr);
			}

			// Test for include lib path
			else if (*pTok == 'L')
			{
				// Extract the library name
				optStr = pTok + 1;
				m_ObjDirs.Add(optStr);
			}

			// Test for output name definition
			else if (*pTok == 'o')
			{
				// Set the output filename
				m_OutputName = pTok + 1;
			}

			// Test for linker script definition
			else if (*pTok == 's')
			{
				// Set the linker script filename
				m_LinkerScript = pTok + 1;
			}
		}
		else
		{
			// Anything that isn't an option must be an input file
			m_LinkFiles.Add(pTok);
		}

		// Get pointer to next token
		pTok = strtok(NULL, pDelim);
	}

	// Free the memory
	delete pStr;
}

/*
============================================================================
This routine maps the input string which is a command field from the 
linker script into a script command number.
============================================================================
*/
int VTLinker::MapScriptCommand(const char *pStr, int lineNo)
{
	int			command;
	MString		err;

	command = LKR_CMD_NONE;

	// Test for CODE specification
	if (strcmp(pStr, "CODE") == 0)
		command = LKR_CMD_CODE;

	// Test for DATA specification
	else if (strcmp(pStr, "DATA") == 0)
		command = LKR_CMD_DATA;

	// Test for OBJPATH specification
	else if (strcmp(pStr, "OBJPATH") == 0)
		command = LKR_CMD_OBJPATH;

	// Test for FILES specification
	else if (strcmp(pStr, "OBJFILE") == 0)
		command = LKR_CMD_OBJFILE;

	// Test for FILES specification
	else if (strcmp(pStr, "ORDER") == 0)
		command = LKR_CMD_ORDER;
	
	// Test for FILES specification
	else if (strcmp(pStr, "CONTAINS") == 0)
		command = LKR_CMD_CONTAINS;
	
	// Test for FILES specification
	else if (strcmp(pStr, "ENDSWITH") == 0)
		command = LKR_CMD_ENDSWITH;
	
	else
	{
		// Syntax error
		err.Format("Error during linking:  Parser syntax error on line %d",
			lineNo);
		m_Errors.Add(err);
	}

	return command;
}

/*
============================================================================
This routine processes the 2nd field from the current line o the linker
script.  It performs operations based on the current m_Command.
============================================================================
*/
void VTLinker::ProcScriptField2(const char *pStr, int lineNo, MString &segname)
{
	MString		err, temp;

	// Process 1st field of CODE or DATA command
	if ((m_Command == LKR_CMD_CODE) || (m_Command == LKR_CMD_DATA))
	{
		if (strncmp(pStr, "NAME=", 5) != 0)
		{
			err.Format("%sExpected segment name on line %d", gsEdl,	lineNo);
			m_Errors.Add(err);
			m_Command = LKR_CMD_ERROR;
		}
		else
		{
			// Skip the NAME= keyword
			segname = pStr + 5;
		}
	}

	// Test if command is FILES and process filename
	else if (m_Command == LKR_CMD_OBJFILE)
	{
		m_LinkFiles.Add(pStr);
		m_Command = LKR_CMD_COMPLETE;
	}

	// Test if command is OBJPATH and add path to array
	else if (m_Command == LKR_CMD_OBJPATH)
	{
		temp = PreprocessDirectory(pStr);
		m_ObjDirs.Add(temp);
		m_Command = LKR_CMD_COMPLETE;
	}

	// Test if command is LKR_CMD_ORDER and save segment name
	else if ((m_Command == LKR_CMD_ORDER) || (m_Command == LKR_CMD_CONTAINS) ||
		(m_Command == LKR_CMD_ENDSWITH))
	{
		segname = pStr;
	}
}

/*
============================================================================
This routine evaluates the string provided and returns it's value.  The
string can be decimal, hex, etc. and can contain simple math.
============================================================================
*/
int VTLinker::EvaluateScriptAddress(const char *pStr, int lineNo)
{
	int		len = strlen(pStr);
	int		hex = 0;
	int		address;

	// Test for Hex conversion
	if (*pStr == '$')
	{
		hex = 1;	// Indicate HEX conversion
		pStr++;		// Skip the '$' hex delimiter
	}
	else if ((*pStr == '0') && (*(pStr + 1) == 'x'))
	{
		hex = 1;
		pStr += 2;	// Skip the "0x" hex delimiter
	}
	else if ((pStr[len-1] == 'h') || (pStr[len-1] == 'H'))
	{
		hex = 1;
	}

	// Perform the conversion
	if (hex)
		sscanf(pStr, "%x", &address);
	else
		sscanf(pStr, "%d", &address);

	return address;
}

/*
============================================================================
This routine processes the 2nd field from the current line o the linker
script.  It performs operations based on the current m_Command.
============================================================================
*/
void VTLinker::ProcScriptField3(const char *pStr, int lineNo, int& startAddr)
{
	MString		err;

	// Initialize startAddr in case error or ORDER command
	startAddr = -1;

	// Test if CODE or DATA command
	if ((m_Command == LKR_CMD_CODE) || (m_Command == LKR_CMD_DATA))
	{
		if (strncmp(pStr, "START=", 6) == 0)
		{
			// Evaluate the starting address
			if (strcmp(pStr + 6, "codeend") == 0)
				startAddr = START_ADDR_CODEEND;
			else
				startAddr = EvaluateScriptAddress(pStr + 6, lineNo);
		}
		else
		{
			err.Format("%sExpected START address in linker script on line %d",
				gsEdl, lineNo);
			m_Errors.Add(err);
			m_Command = LKR_CMD_ERROR;
		}
	}

	// Test if ORDER, CONTAINS or ENDSWITH command
	else if ((m_Command == LKR_CMD_ORDER) || (m_Command == LKR_CMD_CONTAINS) ||
		(m_Command == LKR_CMD_ENDSWITH))
	{
		if (strcmp(pStr, "{") != 0)
		{
			err.Format("%sExpected open brace in linker script on line %d",
				gsEdl, lineNo);
			m_Errors.Add(err);
			m_Command = LKR_CMD_ERROR;
		}
		else
		{
			m_ActiveLinkOrder = new MStringArray;
		}
	}

	// Other commands don't have more than 2 fields
	else
	{
		err.Format("%sTo many linker script arguments in line %d", gsEdl, lineNo);
		m_Errors.Add(err);
		m_Command = LKR_CMD_ERROR;
	}
}

/*
============================================================================
This routine processes the 2nd field from the current line o the linker
script.  It performs operations based on the current m_Command.
============================================================================
*/
void VTLinker::ProcScriptField4(const char *pStr, int lineNo, int& endAddr)
{
	MString		err;

	// Initialize startAddr in case error or ORDER command
	endAddr = -1;

	// Test if CODE or DATA command
	if ((m_Command == LKR_CMD_CODE) || (m_Command == LKR_CMD_DATA))
	{
		if (strncmp(pStr, "END=", 4) == 0)
		{
			endAddr = EvaluateScriptAddress(pStr + 4, lineNo);
			m_Command |= LKR_CMD_CD_DONE;
		}
		else
		{
			err.Format("%sExpected END address in linker script on line %d",
				gsEdl, lineNo);
			m_Errors.Add(err);
			m_Command = LKR_CMD_ERROR;
		}
	}

	// Test if ORDER command
	else if (m_Command == LKR_CMD_ORDER)
		AddOrderedSegment(pStr, lineNo);
	else if (m_Command == LKR_CMD_CONTAINS)
		AddContainsSegment(pStr, lineNo);
	else if (m_Command == LKR_CMD_ENDSWITH)
		AddEndsWithSegment(pStr, lineNo);
}

/*
============================================================================
This routine adds the specified label as an ordered label for the acive
LinkOrder.  It checks to see if the label is the terminating brace and
completes the LinkOrder processing.
============================================================================
*/
void VTLinker::AddOrderedSegment(const char *pStr, int lineNo)
{
	MStringArray	*pOrder;
	MString			err;

	// Test for terminating brace
	if (strcmp(pStr, "}") == 0)
	{
		// Test if this segment has already been defined as ordered
		if (m_LinkOrderList.Lookup(m_SegName, (VTObject *&) pOrder))
		{
			err.Format("%sOrder already specified for %s on line %d",
				gsEdl, (const char *) m_SegName, lineNo);
			m_Errors.Add(err);
			delete m_ActiveLinkOrder;
			m_ActiveLinkOrder = NULL;
			return;
		}
		m_LinkOrderList[m_SegName] = (VTObject *) m_ActiveLinkOrder;
		m_ActiveLinkOrder = NULL;
		m_Command = LKR_CMD_COMPLETE;
	}
	else
	{
		if (m_ActiveLinkOrder != NULL)
			m_ActiveLinkOrder->Add(pStr);
	}
}

/*
============================================================================
This routine adds the specified label as an ordered label for the acive
LinkOrder.  It checks to see if the label is the terminating brace and
completes the LinkOrder processing.
============================================================================
*/
void VTLinker::AddContainsSegment(const char *pStr, int lineNo)
{
	MStringArray	*pOrder;
	MString			err;

	// Test for terminating brace
	if (strcmp(pStr, "}") == 0)
	{
		// Test if this segment has already been defined as ordered
		if (m_LinkContainsList.Lookup(m_SegName, (VTObject *&) pOrder))
		{
			err.Format("%sContainment already specified for %s on line %d",
				gsEdl, (const char *) m_SegName, lineNo);
			m_Errors.Add(err);
			delete m_ActiveLinkOrder;
			m_ActiveLinkOrder = NULL;
			return;
		}
		m_LinkContainsList[m_SegName] = (VTObject *) m_ActiveLinkOrder;
		m_ActiveLinkOrder = NULL;
		m_Command = LKR_CMD_COMPLETE;
	}
	else
	{
		if (m_ActiveLinkOrder != NULL)
			m_ActiveLinkOrder->Add(pStr);
	}
}

/*
============================================================================
This routine adds the specified label as an ordered label for the acive
LinkOrder.  It checks to see if the label is the terminating brace and
completes the LinkOrder processing.
============================================================================
*/
void VTLinker::AddEndsWithSegment(const char *pStr, int lineNo)
{
	MStringArray	*pOrder;
	MString			err;

	// Test for terminating brace
	if (strcmp(pStr, "}") == 0)
	{
		// Test if this segment has already been defined as ordered
		if (m_LinkEndsWithList.Lookup(m_SegName, (VTObject *&) pOrder))
		{
			err.Format("%sENDSWITH already specified for %s on line %d",
				gsEdl, (const char *) m_SegName, lineNo);
			m_Errors.Add(err);
			delete m_ActiveLinkOrder;
			m_ActiveLinkOrder = NULL;
			return;
		}
		m_LinkEndsWithList[m_SegName] = (VTObject *) m_ActiveLinkOrder;
		m_ActiveLinkOrder = NULL;
		m_Command = LKR_CMD_COMPLETE;
	}
	else
	{
		if (m_ActiveLinkOrder != NULL)
			m_ActiveLinkOrder->Add(pStr);
	}
}

/*
============================================================================
This routine processes the 2nd field from the current line o the linker
script.  It performs operations based on the current m_Command.
============================================================================
*/
void VTLinker::ProcScriptField5(const char *pStr, int lineNo, int& prot)
{
	MString		err;

	// Initialize startAddr in case error or ORDER command
	prot = FALSE;

	// Test if CODE or DATA command with PROTECTED option
	if (m_Command & LKR_CMD_CD_DONE)
	{
		if (strcmp(pStr, "PROTECTED") == 0)
		{
			prot = TRUE;
		}
		else
		{
			err.Format("%sExpected argument in linker script on line %d",
				gsEdl, lineNo);
			m_Errors.Add(err);
			m_Command = LKR_CMD_ERROR;
		}
	}

	// Test if ORDER command
	else if (m_Command == LKR_CMD_ORDER)
		AddOrderedSegment(pStr, lineNo);
	else if (m_Command == LKR_CMD_CONTAINS)
		AddContainsSegment(pStr, lineNo);
	else if (m_Command == LKR_CMD_ENDSWITH)
		AddEndsWithSegment(pStr, lineNo);
}

/*
============================================================================
This routine adds a new link region processed from the linker script.  It
adds the region to the m_SegName segment and checks for overlapping
address definitions.
============================================================================
*/
void VTLinker::NewLinkRegion(int type, int lineNo, int startAddr,
	int endAddr, int prot)
{
	MString			err, key;
	CLinkRgn		*pLinkRgn;
	LinkAddrRange*	pAddrRange;
	LinkAddrRange*	pPrevRange;
	LinkAddrRange*	pThisRange;
	POSITION		pos;

	// Validate this region hasn't already been defined (no overlapping regions)
	pos = m_LinkRegions.GetStartPosition();
	while (pos != NULL)
	{
		m_LinkRegions.GetNextAssoc(pos, key, (VTObject *&) pLinkRgn);
		pThisRange = pLinkRgn->m_pFirstAddrRange;
		while (pThisRange != NULL)
		{
			// Test if this region overlaps with new region
			if ((startAddr >= pThisRange->startAddr) && (
				(startAddr <= pThisRange->endAddr)) ||
				((endAddr >= pThisRange->startAddr) && 
				(endAddr <= pThisRange->endAddr)) ||
				((startAddr <= pThisRange->startAddr) &&
				(endAddr >= pThisRange->endAddr)) &&
				(startAddr != START_ADDR_CODEEND))
			{
				err.Format("%sLinker script address ranges cannot overlap at line %d",
					gsEdl, lineNo);
				m_Errors.Add(err);
				m_Command = LKR_CMD_ERROR;
				return;
			}
			
			pThisRange = pThisRange->pNext;
		}
	}

	// Test if this link region already defined
	if (m_LinkRegions.Lookup((const char *) m_SegName, (VTObject *&) pLinkRgn))
	{
		// Ensure the type and protected bits are the same
		if (prot != pLinkRgn->m_Protected)
		{
			err.Format("%sLinker script region protection doesn't match previous definition on line %d",
				gsEdl, lineNo);
			m_Errors.Add(err);
			m_Command = LKR_CMD_ERROR;
			return;
		}

		if (m_Command != pLinkRgn->m_Type)
		{
			err.Format("%sLinker script region type doesn't match previous definition on line %d",
				gsEdl, lineNo);
			m_Errors.Add(err);
			m_Command = LKR_CMD_ERROR;
			return;
		}
	}
	else
		pLinkRgn = NULL;

	// Create a new link region
	if (pLinkRgn == NULL)
	{
		pLinkRgn = new CLinkRgn(m_Command, m_SegName, startAddr, endAddr, prot);
		m_LinkRegions[(const char *) m_SegName] = pLinkRgn;
	}
	else
	{
		// Add a new region to the existing object
		pAddrRange = new LinkAddrRange;
		pAddrRange->startAddr = startAddr;
		pAddrRange->endAddr = endAddr;
		pAddrRange->pNext = NULL;

		// Insert this address range into the list
		pPrevRange = NULL;
		pThisRange = pLinkRgn->m_pFirstAddrRange;
		while ((pThisRange != NULL) && (pAddrRange->startAddr < 
			pThisRange->startAddr))
		{
			pPrevRange = pThisRange;
			pThisRange = pThisRange->pNext;
		}

		// Test if new region should be inserted at beginning or in middle
		if (pThisRange == pLinkRgn->m_pFirstAddrRange)
		{
			pAddrRange->pNext = pThisRange;
			pLinkRgn->m_pFirstAddrRange = pAddrRange;
		}
		else
		{
			pAddrRange->pNext = pThisRange;
			pPrevRange->pNext = pAddrRange;
		}
	}
	m_Command = LKR_CMD_COMPLETE;
}

/*
============================================================================
This routine attempts to open and parse the linker script to be used during
the link operation.  If the file cannot be opened or contains an erro, the
routine returns FALSE, otherwise it configures internal structures and
returns TRUE.
============================================================================
*/
int VTLinker::ReadLinkerScript()
{
	MString			filePath, err, orderSeg, segname;
	int				lineNo;
	FILE			*fd;
	char			lineBuf[256];
	char			*pRead, *pTok;
	int				field, startAddr, endAddr, prot;

	// Test if linker script was supplied
	if (m_LinkerScript.GetLength() == 0)
	{
		err.Format("%sNo linker script specified", gsEdl);
		m_Errors.Add(err);
		return FALSE;
	}

	// Attpempt to open the file with no special path extension
	fd = fopen((const char *) m_LinkerScript, "rb");

	// If linker script file not found in current directory, search in the
	// project root path
	if (fd == NULL)
	{
		filePath = m_RootPath + (char *) "/" + m_LinkerScript;
		fd = fopen((const char *) filePath, "rb");
	}

	// Test for error opening linker script file
	if (fd == NULL)
	{
		err.Format("%sUnable to locate linker script file %s", gsEdl,
			(const char *) m_LinkerScript);
		m_Errors.Add(err);
		return FALSE;
	}

	// The linker script file is open.  Read lines and process each
    // Read lines from the input file and locate them 
	m_Command = LKR_CMD_NONE;
	lineNo = 0;
	field = 0;
	prot = 0;
	while (TRUE)
	{
		// Read the line from the file
		pRead = fgets(lineBuf, sizeof(lineBuf), fd);
		if (pRead == NULL)
		{
			break;
		}
		lineNo++;

		// Test for comment line
		if (lineBuf[0] == ';')
			continue;

		// Separate the line into tokens
		pTok = strtok(lineBuf, " ,\t\n\r");
		if ((m_Command == LKR_CMD_ERROR) || (m_Command == LKR_CMD_COMPLETE))
		{
			field = 0;
			m_Command = LKR_CMD_NONE;
			startAddr = endAddr = prot = 0;
		}

		// Loop for all fields on the line
		while (pTok != NULL)
		{
			// Test for comment in the line
			if (*pTok == ';')
			{
				// Comment only allowd on line if all args processed
				if ((m_Command != LKR_CMD_NONE) && (m_Command != LKR_CMD_COMPLETE) &&
					!(m_Command & LKR_CMD_CD_DONE))
				{
					err.Format("%sIncomplete linker script command on line %d",
						gsEdl, lineNo);
					m_Errors.Add(err);
				}
				break;
			}

			// Parse based on field number
			switch (field)
			{
			case 0:
				m_Command = MapScriptCommand(pTok, lineNo);
				break;

			case 1:
				ProcScriptField2(pTok, lineNo, segname);
				m_SegName = segname;
				break;

			case 2:
				ProcScriptField3(pTok, lineNo, startAddr);
				break;

			case 3:
				ProcScriptField4(pTok, lineNo, endAddr);
				break;

			case 4:
				ProcScriptField5(pTok, lineNo, prot);
				break;

			default:
				// Keep processing arguments to the ORDER command
				if (m_Command == LKR_CMD_ORDER)
					AddOrderedSegment(pTok, lineNo);
				else if (m_Command == LKR_CMD_CONTAINS)
					AddContainsSegment(pTok, lineNo);
				else if (m_Command == LKR_CMD_ENDSWITH)
					AddEndsWithSegment(pTok, lineNo);
				break;
			}

			// Increment the field number
			field++;
			if ((m_Command == LKR_CMD_NONE) || (m_Command == LKR_CMD_ERROR))
				break;
			pTok = strtok(NULL, " ,\t\n\r");
		}

		// If this was a DATA or CODE command that completed, process the
		// information
		if (m_Command & LKR_CMD_CD_DONE)
		{
			// Clear the CMD_CD_DONE bit
			m_Command &= ~LKR_CMD_CD_DONE;
			NewLinkRegion(m_Command, lineNo, startAddr, endAddr, prot);
		}
	}

	// Close the linker script file
	fclose(fd);

	// Check for errors during processing
	if (m_Errors.GetSize() != 0)
		return FALSE;

	return TRUE;
}

/*
============================================================================
This routine reads the ELF header data from the opened file given.
============================================================================
*/
int VTLinker::ReadElfHeaders(FILE* fd, MString& filename)
{
	Elf32_Ehdr			ehdr;
	CObjFileSection*	pFileSection;
	CObjFile*			pObjFile;
	int					bytes, c;
	MString				err;

	// Write ELF header to file
	bytes = fread(&ehdr, 1, sizeof(ehdr), fd);
	if ((bytes != sizeof(ehdr)) || (ehdr.e_ident[EI_MAG0] != ELFMAG0) ||
		(ehdr.e_ident[EI_MAG1] != ELFMAG1) || (ehdr.e_ident[EI_MAG2] != ELFMAG2) ||
		(ehdr.e_ident[EI_MAG3] != ELFMAG3))
	{
		fclose(fd);
		err.Format("Error during linking:  Object file %s is not ELF format",
			(const char *) filename);
		m_Errors.Add(err);
		return FALSE;
	}

	// Ensure the ELF file is the right type
	if ((ehdr.e_ident[EI_CLASS] != ELFCLASS32) || (ehdr.e_ident[EI_DATA] != ELFDATA2LSB) ||
		(ehdr.e_ident[EI_VERSION] < EV_CURRENT) || (ehdr.e_type != ET_REL) ||
		(ehdr.e_machine != EM_8085) || (ehdr.e_version < EV_CURRENT) ||
		(ehdr.e_shoff == 0))
	{
		fclose(fd);
		err.Format("%sObject file %s has invalid ELF format", gsEdl,
			(const char *) filename);
		m_Errors.Add(err);
		return FALSE;
	}

	// Read all the section headers from the header table
	if (fseek(fd, ehdr.e_shoff, SEEK_SET))
	{
		fclose(fd);
		err.Format("%sObject file %s has invalid ELF format", gsEdl,
			(const char *) filename);
		m_Errors.Add(err);
		return FALSE;
	}

	// Okay, now create a new CObjFile object for this file
	pObjFile = new CObjFile(filename);
	if (pObjFile == NULL)
	{
		fclose(fd);
		err.Format("%sunable to allocate memory for section header", gsEdl);
		m_Errors.Add(err);
		return FALSE;
	}
	memcpy(&pObjFile->m_Ehdr, &ehdr, sizeof(ehdr));

	// Loop for all reported headers
	for (c = 0; c < ehdr.e_shnum; c++)
	{
		// Allocate a new section header structure
		if ((pFileSection = new CObjFileSection) == NULL)
		{
			fclose(fd);
			delete pObjFile;
			err.Format("%sUnable to allocate memory for section header", gsEdl);
			m_Errors.Add(err);
			return FALSE;
		}
		if (fread(&pFileSection->m_ElfHeader, 1, sizeof(Elf32_Shdr), fd) != 
			sizeof(Elf32_Shdr))
		{
			fclose(fd);
			delete pObjFile;
			err.Format("%sEnd of file reading section header %d from %s",
				gsEdl, c, (const char *) filename);
			m_Errors.Add(err);
			return FALSE;
		}

		// Add this section header to the array
		pObjFile->m_FileSections.Add(pFileSection);
	}

	// Loop for all reported headers
	for (c = 0; c < ehdr.e_shnum; c++)
	{
		// Now read this section's data
		if (!ReadSectionData(fd, pObjFile, (CObjFileSection *) pObjFile->m_FileSections[c]))
		{
			fclose(fd);
			delete pObjFile;
			return FALSE;
		}
	}

	return TRUE;
}

/*
============================================================================
This routine reads the ELF section specified by the Elf32_Shdr into the
CObjFile object provided.
============================================================================
*/
int VTLinker::ReadSectionData(FILE* fd, CObjFile* pObjFile, 
	CObjFileSection* pFileSection)
{
	int			bytes = 0, count, c;
	MString		err;
	Elf32_Shdr*	pHdr = &pFileSection->m_ElfHeader;
	Elf32_Sym*	pSym;
	Elf32_Rel*	pRel;


	if (pHdr->sh_offset != 0)
		fseek(fd, pHdr->sh_offset, SEEK_SET);

	// Test for null header
	switch (pHdr->sh_type)
	{
	case SHT_NULL:
		// Nothing to do for NULL header
		return TRUE;

		// String table section
	case SHT_STRTAB:
		// Allocate space for the string table
		pFileSection->m_pStrTab = new char[pHdr->sh_size];

		// Read the string table from the file
		if (pFileSection->m_pStrTab != NULL)
			bytes = fread(pFileSection->m_pStrTab, 1, pHdr->sh_size, fd);

		// Test for error
		if (bytes != pHdr->sh_size)
		{
			err.Format("%sUnable to allocate memory for string table for %s",
				gsEdl, (const char *) pObjFile->m_Name);
			m_Errors.Add(err);
			return FALSE;
		}
		break;

	case SHT_SYMTAB:
		count = pHdr->sh_size / sizeof(Elf32_Sym);
		for (c = 0; c < count; c++)
		{
			// Allocate new symbol and rezd data from the file
			pSym = new Elf32_Sym;
			if (pSym != NULL)
				bytes = fread(pSym, 1, sizeof(Elf32_Sym), fd);

			// Test for error during read or allocate
			if (bytes != sizeof(Elf32_Sym))
			{
				err.Format("%sUnable to read symbol table for %s",
					gsEdl, (const char *) pObjFile->m_Name);
				m_Errors.Add(err);
				if (pSym != NULL)
					delete pSym;
				return FALSE;
			}

			// Add the symbol to the array
			pFileSection->m_Symbols.Add((VTObject *) pSym);
		}
		break;

	case SHT_PROGBITS:
		// Allocate space for bytes and read from file
		pFileSection->m_pProgBytes = new char[pHdr->sh_size];
		if (pFileSection->m_pProgBytes != NULL)
			bytes = fread(pFileSection->m_pProgBytes, 1, pHdr->sh_size, fd);

		// Test for error during read
		if (bytes != pHdr->sh_size)
		{
			err.Format("%sUnable to segment data for %s",
				gsEdl, (const char *) pObjFile->m_Name);
			m_Errors.Add(err);
			return FALSE;
		}
		break;

	case SHT_REL:
		count = pHdr->sh_size / sizeof(Elf32_Rel);
		for (c = 0; c < count; c++)
		{
			// Allocate new symbol and read data from the file
			pRel = new Elf32_Rel;
			if (pRel != NULL)
				bytes = fread(pRel, 1, sizeof(Elf32_Rel), fd);

			// Test for error during read or allocate
			if (bytes != sizeof(Elf32_Rel))
			{
				err.Format("%sUnable to read relocation table for %s",
					gsEdl, (const char *) pObjFile->m_Name);
				m_Errors.Add(err);
				if (pRel != NULL)
					delete pRel;
				return FALSE;
			}

			// Add the symbol to the array
			pFileSection->m_Reloc.Add((VTObject *) pRel);
		}
		break;
	}

	return TRUE;
}

/*
============================================================================
This routine attempts to open and read each of the files to be linked. 
During reading, the routine builds control structures based on the contents
of the OBJ file to be used during the link operation, such as symbol and
segment names, relocation objects, etc.
============================================================================
*/
int VTLinker::ReadObjFiles()
{
	int			count, c;
	int			dirs, x;
	FILE*		fd;
	const char *pFile;
	MString		temp, err;

	// Exit if eror has occurred
	if (m_Errors.GetSize() != 0)
		return FALSE;

	count = m_LinkFiles.GetSize();
	for (c = 0; c < count; c++)
	{
		// Read each file and create a lists of segments, publics, externs
		// and reloc objects
		pFile = (const char *) m_LinkFiles[c];

		// First try to open the file with none of the known directory prefixes
		temp = pFile;
		fd = fopen(pFile, "rb");
		if (fd == NULL)
		{
			// Loop through all known library directory prefixes
			dirs = m_ObjDirs.GetSize();
			for (x = 0; x < dirs; x++)
			{
				// Prepend the next object directory
				temp = m_ObjDirs[c] + pFile;
				fd = fopen((const char *) temp, "rb");
				if (fd != NULL)
					break;
			}
		}

		// Test if file could be opened
		if (fd == NULL)
		{
			err.Format("%sUnable to locate object file %s", gsEdl, pFile);
			m_Errors.Add(err);
			continue;
		}

		// Read the Elf data 
		ReadElfHeaders(fd, temp);
	}

	if (m_Errors.GetSize() != 0)
		return FALSE;

	return TRUE;
}

/*
============================================================================
This function locates segments from the input files as per the linker script
regions that have been defined.
============================================================================
*/
int VTLinker::LocateSegments()
{
	POSITION			pos;
//	CObjSegement*		pSeg;
	CObjFile*			pObjFile;
	CObjFileSection*	pFileSect;
	int					sect, sectCount;
	MString				err, filename;

	// Loop for all object files loaded
	pos = m_ObjFiles.GetStartPosition();
	while (pos != NULL)
	{
		// Get pointer to this object file's data
		m_ObjFiles.GetNextAssoc(pos, filename, (VTObject *&) pObjFile);

		// Loop through all segments and find a place to locate them
		sectCount = pObjFile->m_FileSections.GetSize();
		for (sect = 0; sect < sectCount; sect++)
		{
			// Get pointer to next file section for this file
			pFileSect = (CObjFileSection *) pObjFile->m_FileSections[sect];

			// Test if file section is a segment
			if ((pFileSect->m_ElfHeader.sh_type != SHT_PROGBITS) && 
				(pFileSect->m_ElfHeader.sh_type != SHT_NOBITS))
			{
				continue;
			}

			// Test if segment is ASEG
			if ((pFileSect->m_ElfHeader.sh_flags & SHF_8085_ABSOLUTE) &&
				(pFileSect->m_ElfHeader.sh_type == SHT_PROGBITS))
			{
				LocateAsegSegment(pFileSect);
			}

			// Test if segment is CSEG
			else if ((pFileSect->m_ElfHeader.sh_flags & (SHF_ALLOC | SHF_EXECINSTR |
				SHF_WRITE)) == (SHF_ALLOC | SHF_EXECINSTR))
			{
				LocateCsegSegment(pFileSect);
			}

			// Test if segment is DSEG
			else if ((pFileSect->m_ElfHeader.sh_flags & (SHF_ALLOC | SHF_EXECINSTR |
				SHF_WRITE)) == SHF_WRITE)
			{
				LocateDsegSegment(pFileSect);
			}

			// Unknown segment type
			else
			{
				// Report error of unknown segment type
				err.Format("%sUnknown segment type in %s", gsEdl, (const char *) filename);
				m_Errors.Add(err);
			}
		}
	}

	return TRUE;
}

/*
============================================================================
Tries to locate the given ASEG segment
============================================================================
*/
int VTLinker::LocateAsegSegment(CObjFileSection* pFileSect)
{
	return TRUE;
}

/*
============================================================================
Tries to locate the given CSEG segment
============================================================================
*/
int VTLinker::LocateCsegSegment(CObjFileSection* pFileSect)
{
	return TRUE;
}

/*
============================================================================
Tries to locate the given DSEG segment
============================================================================
*/
int VTLinker::LocateDsegSegment(CObjFileSection* pFileSect)
{
	return TRUE;
}

/*
============================================================================
The parser calls this function when it detects a list output pragma.
============================================================================
*/
int VTLinker::Link()
{
	// Process any input arguments
	ProcessArgs(m_LinkOptions, " ");

	// Calculate any additional directories from the IDE
	CalcObjDirs();

	// Process the input filename list from the IDE
	ProcessArgs(m_ObjFileList, ",;");

	// Read the linker script
	ReadLinkerScript();
	
	// Loop through all files to be linked
	ReadObjFiles();

	// Locate segments based on commands in the Linker Script
	LocateSegments();

	// Resolve static (local) symbols for each segment

	// Resolve segment extern symbols

	// Test for unresolved symbols

	// Generate output file

	// Generate debug information

	// Generate hex file

	// Generate map file

	// Back annotate Listing files with actual addresses

	return m_Errors.GetSize() == 0;
}

/*
============================================================================
Set's the linker options.  Called from either the IDE or command line mode.
============================================================================
*/
void VTLinker::SetLinkOptions(const MString& options)
{
	// Set the options string in case we need it later
	m_LinkOptions = options;

	// Parse the options later during assembly
}

/*
============================================================================
Set's the linker script when running from the IDE.
============================================================================
*/
void VTLinker::SetLinkerScript(const MString& script)
{
	// Set the linker script filename for processing later
	m_LinkerScript = script;

	// Parse the options later during assembly
}

/*
============================================================================
Sets a comma delimeted list of object directories when running from the IDE
============================================================================
*/
void VTLinker::SetObjDirs(const MString& dirs)
{
	// Save the string in case we need it later
	m_ObjPath = dirs;

	// Recalculate the include directories
	CalcObjDirs();
}

/*
============================================================================
Sets the root project path
============================================================================
*/
void VTLinker::SetRootPath(const MString& path)
{
	// Save the string in case we need it later
	m_RootPath = path;

	// Recalculate the include directories
	CalcObjDirs();
}

/*
============================================================================
Sets the root project path
============================================================================
*/
void VTLinker::SetObjFiles(const MString& objFiles)
{
	// Save the string in case we need it later
	m_ObjFileList = objFiles;
}

/*
============================================================================
Sets the root project path
============================================================================
*/
void VTLinker::SetProjectType(int type)
{
	// Save the project type
	m_ProjectType = type;
}

/*
============================================================================
Parses the comma delimeted object filename list and splits it into an array
of filenames to be linked.
============================================================================
*/
void VTLinker::CalcObjDirs(void)
{
	char*		pStr;
	char*		pToken;
	MString		temp;

	m_ObjDirs.RemoveAll();

	// Add the root directory to the ObjDirs
	m_ObjDirs.Add(m_RootPath);

	// Check if there is an include path
	if (m_ObjPath.GetLength() == 0)
		return;

	// Copy path to a char array so we can tokenize
	pStr = new char[m_ObjPath.GetLength() + 1];
	strcpy(pStr, (const char *) m_ObjPath);

	pToken = strtok(pStr, ",;");
	while (pToken != NULL)
	{
		// Preprocess the directory by searching for $VT_ROOT and $VT_PROJ
		temp = PreprocessDirectory(pToken);

		// Now add it to the array
		m_ObjDirs.Add(temp);

		// Get next token
		pToken = strtok(NULL, ",;");
	}

	delete pStr;
}

/*
============================================================================
Parses the provided directory name and substitutes {$VT_ROOT} and {$VT_PROJ}
with actual path's.  Returns the resultant string.
============================================================================
*/
MString VTLinker::PreprocessDirectory(const char *pDir)
{
	MString temp;

	if (strncmp(pDir, "{$VT_ROOT}", 10) == 0)
	{
		// Substitute the VirtualT path
		temp = path;
		temp += pDir[10];
	}
	else if (strncmp(pDir, "{$VT_PROJ}", 10) == 0)
	{
		// Generate a projet relative path
		temp = pDir[10];
		temp.Trim();
		temp += (char *) "/";
		temp = m_RootPath + (char *) "/" + temp;
	}
	else
		temp = pDir;

	return temp;
}

/*
============================================================================
Constructor for the CLinkRgn object.  This object is used to keep track of
regions defined in the linker script.
============================================================================
*/
CLinkRgn::CLinkRgn(int type, MString& name, int startAddr, int endAddr, int prot)
{
	m_Type = type; 
	m_Name = name; 
	m_Protected = prot;
	m_pFirstAddrRange = new LinkAddrRange;
	m_pFirstAddrRange->startAddr = startAddr;
	m_pFirstAddrRange->endAddr = endAddr;
	m_pFirstAddrRange->pNext = NULL;
}

CLinkRgn::~CLinkRgn()
{
	LinkAddrRange	*pRange;
	LinkAddrRange	*pNextRange;

	pRange = m_pFirstAddrRange;
	while (pRange != NULL)
	{
		pNextRange = pRange->pNext;
		delete pRange;
		pRange = pNextRange;
	}
}

CObjFile::~CObjFile()
{
	int			count, c;

	count = m_FileSections.GetSize();
	for (c = 0; c < count; c++)
	{
		delete (CObjFileSection *) m_FileSections[c];
	}
	m_FileSections.RemoveAll();
}

CObjFileSection::~CObjFileSection()
{
	int		count, c;

	if (m_pStrTab != NULL)
		delete m_pStrTab;

	if (m_pObjSegment != NULL)
		delete m_pObjSegment;

	count = m_Symbols.GetSize();
	for (c = 0; c < count; c++)
	{
		delete (Elf32_Sym *) m_Symbols[c];
	}
	m_Symbols.RemoveAll();

	count = m_Reloc.GetSize();
	for (c = 0; c < count; c++)
	{
		delete (Elf32_Rel *) m_Reloc[c];
	}
	m_Symbols.RemoveAll();

	// Delete program bytes pointer if not NULL
	if (m_pProgBytes != NULL)
		delete m_pProgBytes;
}
