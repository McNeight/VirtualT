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
#include		<FL/filename.H>

extern "C"
{
#include		"intelhex.h"
char			path[512];
}

static const char *gLoaderCode[] = {
	"89GOTO96\r\n",
	"90P=1\x0D\x0A",
	"91C$=MID$(A$,P,5):IFC$=\"\"THENRETURN\x0D\x0A",
	"92V=0:FORX=1TO5:T=ASC(MID$(C$,X,1)):V=V*85+T-35:K=K+T:NEXT:P=P+5\x0D\x0A",
	"93H=INT(V/M/M):L=V-H*M*M:POKES,H/M:POKES+1,H-INT(H/M)*M:POKES+2,L/M:POKES+3,L-INT(L/M)*M:S=S+4\x0D\x0A",
	"94L=D:D=D+I:IFD>184THEND=184\x0D\x0A",
	"95FORX=L+1TOD:PSET(X,27):NEXT:PSET(D,27):PSET(D-1,26):PSET(D-2,25):PSET(D-1,28):PSET(D-2,29):PRESET(L-1,26):PRESET(L-2,25):PRESET(L-1,28):PRESET(L-2,29):GOTO 91\x0D\x0A",
	"96CLS:DEFDBLV:PRINT@50,\"Creating %s\":PRINT@129,CHR$(245);:PRINT@151,CHR$(245);:D=59:I=%.3f:S=%d:M=256:FORJ%%=1TO%d:READA$:GOSUB90:NEXT\x0D\x0A",
	"97IFK<>%dTHEN99ELSE PRINT@200,\"Success! Issue command: clear 256,%d\"\x0D\x0A",
	"98SAVEM\"%s\",%d,%d,%d:END\x0D\x0A",
	"99PRINT@200,\"Chksum error! Need %d, got\";K:END\x0D\x0A",

	// These are used for PC-8201 and PC-8300 links
	"96CLS:DEFDBLV:LOCATE10,1:PRINT\"Creating %s\":LOCATE9,3:PRINT\"|\"SPACE$(21)\"|\";:D=59:I=%.3f:S=%d:M=256:FORJ%%=1TO%d:READA$:GOSUB90:NEXT\x0D\x0A",
	"97IFK<>%dTHEN99ELSE LOCATE0,5:PRINT\"Success! Issue command: clear 256,%d\"\x0D\x0A",
	"98BSAVE\"%s\",%d,%d,%d:END\x0D\x0A",
	"99LOCATE0,5:PRINT\"Chksum error! Need %d, got\";K:END\x0D\x0A"
};

static const char *gsEdl = "Error during linking:  ";

/*
============================================================================
Construtor / destructor for the linker
============================================================================
*/
VTLinker::VTLinker()
{
	int		x;

	m_Hex = 0;
	m_FileIndex = -1;
	m_DebugInfo = 0;
	m_TotalCodeSpace = 0;
	m_TotalDataSpace = 0;
	m_EntryAddress = 0;
	m_StartAddress = 0;
	
	// Clear out the Segment assignement map
	for (x = 0; x < sizeof(m_SegMap) / sizeof(CObjSegment *); x++)
		m_SegMap[x] = NULL;
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
Sets the standard output "printf" function to write error messages to
============================================================================
*/
void VTLinker::SetOutputFile(const MString& outFile)
{
	m_OutputName = outFile;
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
	CObjSymFile*	pObjSymFile;
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
	
	// Loop through all Symbols and delete them
	pos = m_Symbols.GetStartPosition();
	while (pos != NULL)
	{
		m_Symbols.GetNextAssoc(pos, key, (VTObject *&) pObjSymFile);
		delete pObjSymFile;
	}
	m_Symbols.RemoveAll();
	
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
	m_EntryAddress = 0;
	m_StartAddress = 0;
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

	// Test for DATA specification
	else if (strcmp(pStr, "MIXED") == 0)
		command = LKR_CMD_MIXED;

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

	// Test for ENTRY specification
	else if (strcmp(pStr, "ENTRY") == 0)
		command = LKR_CMD_ENTRY;
	
	// Test for ENTRY specification
	else if (strcmp(pStr, "DEFINE") == 0)
		command = LKR_CMD_DEFINE;
	
	else
	{
		// Syntax error
		err.Format("Error in line %d(%s):  Parser syntax error", lineNo,
			(const char *) m_LinkerScript);
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
	if ((m_Command == LKR_CMD_CODE) || (m_Command == LKR_CMD_DATA) ||
		(m_Command == LKR_CMD_MIXED))
	{
		if (strncmp(pStr, "NAME=", 5) != 0)
		{
			err.Format("Error in line %d(%s):  Expected segment name", lineNo,
				(const char *) m_LinkerScript);
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

	// Test if command is OBJPATH and add path to array
	else if (m_Command == LKR_CMD_ENTRY)
	{
		m_EntryLabel = pStr;
		m_Command = LKR_CMD_COMPLETE;
	}

	// Test if command is DEFINE and add the symbol
	else if (m_Command == LKR_CMD_DEFINE)
	{
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
void VTLinker::ProcScriptField3(char *pStr, int lineNo, int& startAddr,
								char* &pSectName)
{
	MString		err;

	// Initialize startAddr in case error or ORDER command
	startAddr = -1;
	pSectName = NULL;

	// Test if CODE or DATA command
	if ((m_Command == LKR_CMD_CODE) || (m_Command == LKR_CMD_DATA) ||
		(m_Command == LKR_CMD_MIXED))
	{
		if (strncmp(pStr, "START=", 6) == 0)
		{
			// Evaluate the starting address
			if (strncmp(pStr + 6, "atend(", 6) == 0)
			{
				// Set the START address to indicate atend of a region
				startAddr = START_ADDR_CODEEND;

				// Parse the region name from the string
				if (pStr[strlen(pStr)-1] == ')')
				{
					pStr[strlen(pStr)-1] = '\0';
					pSectName = pStr + 12;
				}
				else
				{
					// Parse error!  Expected ')'
					err.Format("Error in line %d(%s):  Expected ')'", lineNo,
						(const char *) m_LinkerScript);
					m_Errors.Add(err);
					m_Command = LKR_CMD_ERROR;
				}
			}
			else
				startAddr = EvaluateScriptAddress(pStr + 6, lineNo);
		}
		else
		{
			err.Format("Error in line %d(%s):  Expected START address", lineNo,
				(const char *) m_LinkerScript);
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
			err.Format("Error in line %d(%s):  Expected open brace", lineNo,
				(const char *) m_LinkerScript);
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
		err.Format("Error in line %d(%s):  To many arguments", lineNo, 
			(const char *) m_LinkerScript);
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
	if ((m_Command == LKR_CMD_CODE) || (m_Command == LKR_CMD_DATA) ||
		(m_Command == LKR_CMD_MIXED))
	{
		if (strncmp(pStr, "END=", 4) == 0)
		{
			endAddr = EvaluateScriptAddress(pStr + 4, lineNo);
			m_Command |= LKR_CMD_CD_DONE;
		}
		else
		{
			err.Format("Error in line %d(%s):  Expected END address in linker script",
				lineNo, (const char *) m_LinkerScript);
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
			err.Format("Error in line %d(%s):  Order already specified for %s",
				lineNo, (const char *) m_LinkerScript, (const char *) m_SegName);
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
			err.Format("Error in line %d(%s):  Containment already specified for %s", lineNo, 
				(const char *) m_LinkerScript, (const char *) m_SegName);
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
			err.Format("Error in line %d(%s):  ENDSWITH already specified for %s", lineNo,
				(const char *) m_LinkerScript, (const char *) m_SegName);
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
void VTLinker::ProcScriptField5(const char *pStr, int lineNo, int& prot, int& atend)
{
	MString		err;
	const char	*pToken, *ptr;
	int			tokLen;

	// Initialize startAddr in case error or ORDER command
	prot = FALSE;
	atend = FALSE;

	// Test if CODE, DATA or MIXED command with PROTECTED or ATEND option
	if (m_Command & LKR_CMD_CD_DONE)
	{
		pToken = pStr;
		while (pToken != NULL && *pToken != '\0')
		{
			// Find token length
			tokLen = 0;
			ptr = pToken;
			while (*ptr != '+' && *ptr != '\0')
			{
				tokLen++;
				ptr++;
			}

			if (strncmp(pToken, "PROTECTED", tokLen) == 0)
			{
				prot = TRUE;
			}
			else if (strncmp(pToken, "ATEND", tokLen) == 0)
			{
				atend = TRUE;
			}
			else
			{
				char temp[128];
				if (tokLen >= sizeof(temp) - 1)
					tokLen = sizeof(temp) - 1;
				strncpy(temp, pToken, tokLen);
				temp[tokLen] = '\0';
				err.Format("Error in line %d(%s):  Unknown region attribute %s", lineNo,
					(const char *) m_LinkerScript, temp);
				m_Errors.Add(err);
				m_Command = LKR_CMD_ERROR;
			}

			// Advance to next tokan
			pToken += tokLen;
			if (*pToken == '+')
				pToken++;
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
	int endAddr, int prot, const char *pAtEndName, int atend)
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
			if (((startAddr >= pThisRange->startAddr) && (
				(startAddr <= pThisRange->endAddr)) ||
				((endAddr >= pThisRange->startAddr) && 
				(endAddr <= pThisRange->endAddr)) ||
				((startAddr <= pThisRange->startAddr) &&
				(endAddr >= pThisRange->endAddr))) &&
				(startAddr != START_ADDR_CODEEND) &&
				(startAddr != START_ADDR_DATAEND))
			{
				err.Format("Error in line %d(%s):  Linker script address ranges cannot overlap",
					lineNo, (const char *) m_LinkerScript);
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
		pLinkRgn = new CLinkRgn(m_Command, m_SegName, startAddr, endAddr, prot, pAtEndName, atend);
		pLinkRgn->m_NextLocateAddr = startAddr;
		pLinkRgn->m_EndLocateAddr = endAddr;
		m_LinkRegions[(const char *) m_SegName] = pLinkRgn;
	}
	else
	{
		// Add a new region to the existing object
		pAddrRange = new LinkAddrRange;
		pAddrRange->startAddr = startAddr;
		pAddrRange->endAddr = endAddr;
		pAddrRange->nextLocateAddr = startAddr;
		pAddrRange->endLocateAddr = endAddr;
		pAddrRange->pNext = NULL;
		pAddrRange->pPrev = NULL;

		// Insert this address range into the list
		pPrevRange = NULL;
		pThisRange = pLinkRgn->m_pFirstAddrRange;
		while ((pThisRange != NULL) && (pThisRange->startAddr < 
			pAddrRange->startAddr))
		{
			pPrevRange = pThisRange;
			pThisRange = pThisRange->pNext;
		}

		// Update the LinkRgn's NextLocate and EndLocate addresses
		if (pAddrRange->startAddr < pLinkRgn->m_NextLocateAddr)
			pLinkRgn->m_NextLocateAddr = pAddrRange->startAddr;
		if (pAddrRange->endAddr > pLinkRgn->m_EndLocateAddr)
			pLinkRgn->m_EndLocateAddr = pAddrRange->endAddr;

		// Test if new region should be inserted at beginning or in middle
		if (pThisRange == pLinkRgn->m_pFirstAddrRange)
		{
			// Insert as first item
			pAddrRange->pNext = pThisRange;
			pLinkRgn->m_pFirstAddrRange = pAddrRange;
		}
		else
		{
			// Insert into linked list
			pAddrRange->pNext = pThisRange;
			pPrevRange->pNext = pAddrRange;
		}

		// Update LinkRgn's LastAddrRange if this is the last item
		pAddrRange->pPrev = pPrevRange;
		if (pAddrRange->pNext == NULL)
			pLinkRgn->m_pLastAddrRange = pAddrRange;
		else
			pAddrRange->pNext->pPrev = pAddrRange;
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
	int				field, startAddr, endAddr, prot, atend;
	char			*pSectName;

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
	atend = 0;
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
					err.Format("Error in line %d(%s):  Incomplete linker script command", lineNo,
						(const char *) m_LinkerScript);
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
				ProcScriptField3(pTok, lineNo, startAddr, pSectName);
				break;

			case 3:
				ProcScriptField4(pTok, lineNo, endAddr);
				break;

			case 4:
				ProcScriptField5(pTok, lineNo, prot, atend);
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
			NewLinkRegion(m_Command, lineNo, startAddr, endAddr, prot, pSectName, atend);
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

	// Read ELF header from file
	bytes = fread(&ehdr, 1, sizeof(ehdr), fd);
	if ((bytes != sizeof(ehdr)) || (ehdr.e_ident[EI_MAG0] != ELFMAG0) ||
		(ehdr.e_ident[EI_MAG1] != ELFMAG1) || (ehdr.e_ident[EI_MAG2] != ELFMAG2) ||
		(ehdr.e_ident[EI_MAG3] != ELFMAG3))
	{
		fclose(fd);
		err.Format("%sObject file %s is not ELF format", gsEdl,
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
		pFileSection->m_Index = c;
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

	m_ObjFiles[filename] = pObjFile;

	return TRUE;
}

/*
===============================================================================
This routine returns a string which is the relative form of the given path
realtive to the relTo path.  The routine detects both relativeness in bot
directions and uses '..' as necessary in the returned string.
===============================================================================
*/
MString VTLinker::MakeTitle(const MString& path)
{
	MString		temp;
	int			index;

	// Search for the last directory separator token
	index = path.ReverseFind('/');
	if (index == -1)
		return path;

	return path.Right(path.GetLength() - index - 1);
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
	int				bytes = 0, count, c;
	MString			err;
	Elf32_Shdr*		pHdr = &pFileSection->m_ElfHeader;
	Elf32_Sym*		pSym;
	Elf32_Rel*		pRel;
	CObjSymFile*	pSymFile;

	if (pHdr->sh_offset != 0)
		fseek(fd, pHdr->sh_offset, SEEK_SET);

	// Set the Section type, size, etc.
	pFileSection->m_Type = pHdr->sh_type;
	pFileSection->m_Size = pHdr->sh_size;

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

		// Test if the section link points to itself
		if (pFileSection->m_Index == pHdr->sh_link)
		{
			if (strcmp(&pFileSection->m_pStrTab[pHdr->sh_name], ".shstrtab") == 0)
				if (pObjFile->m_pShStrTab == NULL)
					pObjFile->m_pShStrTab = pFileSection->m_pStrTab;
		}

		// Search for the symbol string table
		if (pObjFile->m_pStrTab == NULL)
		{
			if (pObjFile->m_pShStrTab != NULL)
				if (strcmp(&pObjFile->m_pShStrTab[pHdr->sh_name], ".strtab") == 0)
				{
					pObjFile->m_pStrTab = pFileSection->m_pStrTab;
				}
		}
		break;

	case SHT_SYMTAB:
		if (pObjFile->m_pSymSect == NULL)
			pObjFile->m_pSymSect = pFileSection;

		count = pHdr->sh_size / sizeof(Elf32_Sym);
		for (c = 0; c < count; c++)
		{
			// Allocate new symbol and read data from the file
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

			// Add PUBLIC symbols to the global symbol table
			if ((pObjFile->m_pStrTab != NULL) && (ELF32_ST_BIND(pSym->st_info) == STB_GLOBAL))
			{
				// Test if it already exists
				if (m_Symbols.Lookup(&pObjFile->m_pStrTab[pSym->st_name], (VTObject *&) pSymFile))
				{
					// Error!  Duplicate global symbol
					MString title1 = MakeTitle(pObjFile->m_Name);
					MString title2 = MakeTitle(pSymFile->m_pObjFile->m_Name);
					err.Format("%sPUBLIC symbol %s duplicated in file %s.  First encountered in %s",
						gsEdl, &pObjFile->m_pStrTab[pSym->st_name], (const char *) title1,
						(const char *) title2);
					m_Errors.Add(err);
					continue;
				}

				// Add the symbol to our table
				pSymFile = new CObjSymFile();
				pSymFile->m_pObjFile = pObjFile;
				pSymFile->m_pObjSection = pFileSection;
				pSymFile->m_pSym = pSym;
				pSymFile->m_pName = &pObjFile->m_pStrTab[pSym->st_name];
				m_Symbols[&pObjFile->m_pStrTab[pSym->st_name]] = pSymFile;
			}
		}
		break;

	case SHT_PROGBITS:
		// Allocate space for bytes and read from file
		pFileSection->m_pProgBytes = new char[pHdr->sh_size];
		pFileSection->m_Size = pHdr->sh_size;
		if (pFileSection->m_pProgBytes != NULL)
			bytes = fread(pFileSection->m_pProgBytes, 1, pHdr->sh_size, fd);

		// Assign the section type
		if ((pHdr->sh_flags & SHF_8085_ABSOLUTE) &&
			(pHdr->sh_type == SHT_PROGBITS))
		{
			// Type is ASEG
			pFileSection->m_Type = ASEG;
		}

		// Test if segment is CSEG
		else if ((pHdr->sh_flags & (SHF_ALLOC | SHF_EXECINSTR |
			SHF_WRITE)) == (SHF_ALLOC | SHF_EXECINSTR))
		{
			// Type is CSEG
			pFileSection->m_Type = CSEG;
		}

		// Test if segment is DSEG
		else if ((pHdr->sh_flags & (SHF_ALLOC | SHF_EXECINSTR |
			SHF_WRITE)) == SHF_WRITE)
		{
			// Type is DSEG
			pFileSection->m_Type = DSEG;
		}

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
		// Now loop through all all relocation elements
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
This routine loops through all sections of all object files and assignes 
the m_Name variable based on the sections name index into the string
table read from the file.
============================================================================
*/
int VTLinker::AssignSectionNames()
{
	CObjFile*			pObjFile;
	int					count, c;
	CObjFileSection*	pFileSection;
	bool				segmentFound = false;
	char *				pName;
	POSITION			pos;
	MString				fileName;

	// Find the segment to be located.  Loop through each objFile...
	pos = m_ObjFiles.GetStartPosition();
	while (pos != NULL)
	{
		// Get next ObjectFile pointer
		m_ObjFiles.GetNextAssoc(pos, fileName, (VTObject *&) pObjFile);

		if (pObjFile->m_pShStrTab == NULL)
			continue;

		// Loop through this file's obj sections and search for each segment
		count = pObjFile->m_FileSections.GetSize();
		for (c = 0; c < count; c++)
		{
			// Get a pointer to the segment data so we know which index it is
			pFileSection = (CObjFileSection *) pObjFile->m_FileSections[c];

			// Point to start of string table and loop to find the correct index
			pName = &pObjFile->m_pShStrTab[pFileSection->m_ElfHeader.sh_name];
			
			// Finally, assign the name to the segment
			if (*pName != '\0')
				pFileSection->m_Name = pName;
		}
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
				temp = m_ObjDirs[x] + pFile;
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

	// Assign Section Names
	AssignSectionNames();

	if (m_Errors.GetSize() != 0)
		return FALSE;

	return TRUE;
}

/*
============================================================================
This function locates a specific named segment into a named region.
============================================================================
*/
int VTLinker::LocateSegmentIntoRegion(MString& region, MString& segment, bool atEnd)
{
	MString				err;
	CObjFile*			pObjFile;
	int					count, c;
	CObjFileSection*	pFileSection;
	bool				segmentFound = false;
	POSITION			pos;
	MString				fileName;

	// Find the segment to be located.  Loop through each objFile...
	pos = m_ObjFiles.GetStartPosition();
	while (pos != NULL)
	{
		// Get next ObjectFile pointer
		m_ObjFiles.GetNextAssoc(pos, fileName, (VTObject *&) pObjFile);
		segmentFound = false;

		// Loop through this file's obj sections and search for the segment
		count = pObjFile->m_FileSections.GetSize();
		for (c = 0; c < count; c++)
		{
			pFileSection = (CObjFileSection *) pObjFile->m_FileSections[c];
			if (pFileSection->m_Name == segment && pFileSection->m_ElfHeader.sh_type == SHT_PROGBITS)
			{
				// Segment found
				segmentFound = true;
				break;
			}
		}
		if (!segmentFound)
			continue;

		if (!LocateSegmentIntoRegion(region, pFileSection, atEnd))
			return FALSE;
	}

	return TRUE;
}

/*
============================================================================
This function locates a specific named segment into a named region.
============================================================================
*/
int VTLinker::LocateSegmentIntoRegion(MString& region, CObjFileSection* pFileSection, 
									  bool atEnd)
{
	CLinkRgn*			pLinkRgn;
	MString				err;
	int					c, locateAddr, segSize;
	bool				segmentFound = false;
	MString				fileName;
	LinkAddrRange*		pAddrRange;
	static const char*	sSegType[] = { "ASEG", "CSEG", "DSEG" };
	MString				segment = pFileSection->m_Name;

	// Find the region where we will locate the segment
	if (!m_LinkRegions.Lookup((const char *) region, (VTObject *&) pLinkRgn))
	{
		// Report error of unknown segment type
		err.Format("%sLink region %s not defined trying to locate segment %s", gsEdl, (const char *) region,
			(const char *) pFileSection->m_Name);
		m_Errors.Add(err);
		return FALSE;
	}

	// Check if location into this region is complete and simply exit if it is
	if (pLinkRgn->m_LocateComplete)
		return TRUE;

	// Check if region is valid yet (it may have a START with an "atend" specification
	// that hasn't been met yet).
	if (pLinkRgn->m_pFirstAddrRange->startAddr < 0)
		return TRUE;

	// Validate the segment isn't already located
	if (pFileSection->m_Located)
	{
		err.Format("%sSegment %s specified in multiple locations in linker script", gsEdl, 
			(const char *) segment);
		m_Errors.Add(err);
		return FALSE;
	}

	// Validate the segment is the proper type for the region
	if (((((pFileSection->m_Type == ASEG) || (pFileSection->m_Type == CSEG)) && (pLinkRgn->m_Type != LKR_CMD_CODE)) ||
		((pFileSection->m_Type == DSEG) && (pLinkRgn->m_Type != LKR_CMD_DATA))) && (pLinkRgn->m_Type != LKR_CMD_MIXED))
	{
		err.Format("%sSegment %s type (%s) does not match region %s", gsEdl, (const char *) segment,
			sSegType[pFileSection->m_Type], (const char *) region);
		m_Errors.Add(err);
		return FALSE;
	}

	// If the Link Region is protected, ensure the Segment name and Link Region name match
	if (pLinkRgn->m_Protected && (pFileSection->m_Name != pLinkRgn->m_Name))
	{
		err.Format("%sCannot place segment %s into protected region %s", gsEdl, (const char *) segment,
			(const char *) region);
		m_Errors.Add(err);
		return FALSE;
	}

	// Get the size of the segment to be located
	segSize = pFileSection->m_Size;

	// If segment has the ATEND attribte, then force the atend parameter
	if (pLinkRgn->m_AtEnd)
		atEnd = TRUE;

	if (pFileSection->m_Type == ASEG)
	{
		// Place at an absolute location
		// Get pointer to Link Rgn's first AddrRange
		pAddrRange = pLinkRgn->m_pFirstAddrRange;
		locateAddr = pFileSection->m_ElfHeader.sh_addr;

		// Find the first region where this segment is supposed to live
		while (pAddrRange && (pAddrRange->endAddr < locateAddr))
		{
			// Get pointer to next AddrRange
			pAddrRange = pAddrRange->pNext;
		}

		// Validate the range was found
		if ((pAddrRange == NULL) || (pAddrRange->startAddr > locateAddr) ||
			(locateAddr + segSize > pAddrRange->endAddr))
		{
			err.Format("%sNo Link Range found for ASEG %s (addr=%d, size=%d)", gsEdl, 
				(const char *) segment,	locateAddr, segSize);
			m_Errors.Add(err);
			return FALSE;
		}

		// Validate we don't have overlapping ASEGs
		for (c = locateAddr; c < locateAddr + segSize; c++)
		{
			if (m_SegMap[c] != NULL)
			{
				err.Format("%sSegment %s overlaps segment %s at address %d", gsEdl, 
					(const char *) segment,	(const char *) m_SegMap[c]->m_Name, c);
				m_Errors.Add(err);
				return FALSE;
			}
		}
	}
	else
	{
		// Locate the segment into the region anywhere.  Find the address where it will live
		if (atEnd)
		{
			// Get pointer to Link Rgn's first AddrRange
			pAddrRange = pLinkRgn->m_pLastAddrRange;

			// Find the first region with enough space to hold the segment
			while (pAddrRange && (pAddrRange->endLocateAddr - pAddrRange->nextLocateAddr < segSize))
			{
				// Get pointer to next AddrRange
				pAddrRange = pAddrRange->pPrev;
			}

			// Test if a segment large enough was found
			if (pAddrRange == NULL)
			{
				err.Format("%sLink Range %s too small for segment %s", gsEdl, (const char *) region,
					(const char *) segment);
				m_Errors.Add(err);
				return FALSE;
			}

			// Update region's nextLocateAddr
			locateAddr = pAddrRange->endLocateAddr - segSize;
			pAddrRange->endLocateAddr -= segSize;
		}
		else
		{
			// Get pointer to Link Rgn's first AddrRange
			pAddrRange = pLinkRgn->m_pFirstAddrRange;

			// Find the first region with enough space to hold the segment
			while (pAddrRange && (pAddrRange->endLocateAddr - pAddrRange->nextLocateAddr < segSize))
			{
				// Get pointer to next AddrRange
				pAddrRange = pAddrRange->pNext;
			}

			// Test if a segment large enough was found
			if (pAddrRange == NULL)
			{
				err.Format("%sLink Range %s too small for segment %s", gsEdl, (const char *) region,
					(const char *) segment);
				m_Errors.Add(err);
				return FALSE;
			}

			// Update region's nextLocateAddr
			locateAddr = pAddrRange->nextLocateAddr;
			pAddrRange->nextLocateAddr += segSize;
		}
	}

	// Finally, locate the segment
	for (c = locateAddr; c < locateAddr + segSize; c++)
	{
		// Mark the Segment map as being allocated to this file section
		m_SegMap[c] = pFileSection;
	}

	// Mark the segment as located
	pFileSection->m_LocateAddr = locateAddr;
	pFileSection->m_Located = true;

	if (pFileSection->m_Type == DSEG)
		m_TotalDataSpace += segSize;
	else 
		m_TotalCodeSpace += segSize;

	return TRUE;
}

/*
============================================================================
This function locates segments based on ORDER.
============================================================================
*/
int VTLinker::LocateOrderedSegments()
{
	POSITION			pos;
	MString				name;
	MStringArray*		pSegmentNames;
	int					count, c;
	int					success = TRUE;

	pos = m_LinkOrderList.GetStartPosition();
	while (pos != NULL)
	{
		// Get pointer to this entry's data
		m_LinkOrderList.GetNextAssoc(pos, name, (VTObject *&) pSegmentNames);

		// Loop through each entry and locate that segment 
		count = pSegmentNames->GetSize();
		for (c = 0; c < count; c++)
		{
			// Now locate this segment into the named region
			MString segmentName = pSegmentNames->GetAt(c);
			if (!LocateSegmentIntoRegion(name, segmentName))
				success = FALSE;
		}
	}

	return success;
}

/*
============================================================================
This function locates segments based on ENDSWITH.
============================================================================
*/
int VTLinker::LocateEndsWithSegments()
{
	POSITION			pos;
	MString				name;
	MStringArray*		pSegmentNames;
	int					count, c;
	int					success = TRUE;

	pos = m_LinkEndsWithList.GetStartPosition();
	while (pos != NULL)
	{
		// Get pointer to this entry's data
		m_LinkEndsWithList.GetNextAssoc(pos, name, (VTObject *&) pSegmentNames);

		// Loop through each entry and locate that segment 
		count = pSegmentNames->GetSize();
		for (c = count-1; c >= 0; c--)
		{
			// Now locate this segment into the named region
			MString segName = pSegmentNames->GetAt(c);
			if (!LocateSegmentIntoRegion(name, segName, true))
				success = FALSE;
		}
	}

	return success;
}

/*
============================================================================
This function locates segments based on CONTAINS.
============================================================================
*/
int VTLinker::LocateContainsSegments()
{
	POSITION			pos;
	MString				name;
	MStringArray*		pSegmentNames;
	int					count, c;
	int					success = TRUE;

	pos = m_LinkContainsList.GetStartPosition();
	while (pos != NULL)
	{
		// Get pointer to this entry's data
		m_LinkContainsList.GetNextAssoc(pos, name, (VTObject *&) pSegmentNames);

		// Loop through each entry and locate that segment 
		count = pSegmentNames->GetSize();
		for (c = 0; c < count; c++)
		{
			// Now locate this segment into the named region
			MString segName = pSegmentNames->GetAt(c);
			if (!LocateSegmentIntoRegion(name, segName))
				success = FALSE;
		}
	}

	return success;
}

/*
============================================================================
This function locates segments from the input files as per the linker script
regions that have been defined.
============================================================================
*/
int VTLinker::LocateNondependantSegments()
{
	POSITION			pos;
	CObjFile*			pObjFile;
	CObjFileSection*	pFileSect;
	int					sect, sectCount;
	MString				err, filename;
	int					success = TRUE;
	MString				aseg = ".aseg";
	MString				data = ".data";
	MString				text = ".text";

	// Exit if error has occurred
	if (m_Errors.GetSize() != 0)
		return FALSE;

	// Locate segments based on ORDER from the Linker Script
	success = LocateOrderedSegments();

	// Locate segments based on ENDSWITH from the Linker Script
	if (!LocateEndsWithSegments())
		success = FALSE;

	// Locate segments based on CONTAINS from the Linker Script
	if (!LocateContainsSegments())
		success = FALSE;

	// Loop for all object files loaded and locate any that have not been located yet
	pos = m_ObjFiles.GetStartPosition();
	while (pos != NULL)
	{
		// Get pointer to this object file's data
		m_ObjFiles.GetNextAssoc(pos, filename, (VTObject *&) pObjFile);

		// Loop through all segments and find a place to locate them
		sectCount = pObjFile->m_FileSections.GetSize();
		for (sect = 0; sect < sectCount; sect++)
		{
			CLinkRgn* pLinkRgn;

			// Get pointer to next file section for this file
			pFileSect = (CObjFileSection *) pObjFile->m_FileSections[sect];

			// Test if file section is a segment
			if ((pFileSect->m_ElfHeader.sh_type != SHT_PROGBITS) && 
				(pFileSect->m_ElfHeader.sh_type != SHT_NOBITS))
			{
				continue;
			}

			// Test if this segment has already been located
			if (pFileSect->m_Located)
			{
				continue;
			}

			// Test if the segment has a named link region
			if (m_LinkRegions.Lookup((const char *) pFileSect->m_Name, (VTObject *&) pLinkRgn))
			{
				// Locate the segment into the region with the same name
				LocateSegmentIntoRegion(pLinkRgn->m_Name, pFileSect);
			}

			// Test if segment is ASEG
			else if ((pFileSect->m_ElfHeader.sh_flags & SHF_8085_ABSOLUTE) &&
				(pFileSect->m_ElfHeader.sh_type == SHT_PROGBITS))
			{
				// Locate ASEGs into the .aseg linker region
				LocateSegmentIntoRegion(aseg, pFileSect);
			}

			// Test if segment is CSEG
			else if ((pFileSect->m_ElfHeader.sh_flags & (SHF_ALLOC | SHF_EXECINSTR |
				SHF_WRITE)) == (SHF_ALLOC | SHF_EXECINSTR))
			{
				// Locate CSEGs into the .text linker region
				LocateSegmentIntoRegion(text, pFileSect);
			}

			// Test if segment is DSEG
			else if ((pFileSect->m_ElfHeader.sh_flags & (SHF_ALLOC | SHF_EXECINSTR |
				SHF_WRITE)) == SHF_WRITE)
			{
				// Locate DSEGs into the .data linker region
				LocateSegmentIntoRegion(data, pFileSect);
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
This function locates segments from the input files as per the linker script
regions that have been defined.  It calls the LocateDependantSegments function
repeated, progressively resolving segment dependencies until all dependencies
have been resolved.  This takes care of regions whose START address is relative
to the end ("atend") of another segment.
============================================================================
*/
int VTLinker::LocateSegments()
{
	int				allDependenciesMet = FALSE;
	int				newResolve;
	unsigned short	lastAddr;
	int				c;
	CLinkRgn*		pLinkRgn;
	POSITION		pos;
	MString			name;

	// Loop until all segment dependencies resolved
	while (!allDependenciesMet)
	{
		// Locate segments into non-dependant regions
		LocateNondependantSegments();

		// Mark all non-dependant regions as located
		pos = m_LinkRegions.GetStartPosition();
		while (pos)
		{
			// Get next link region
			m_LinkRegions.GetNextAssoc(pos, name, (VTObject *&) pLinkRgn);
			if (pLinkRgn->m_pFirstAddrRange->startAddr >= 0)
				pLinkRgn->m_LocateComplete = TRUE;
		}

		// Check if any more dependencies to resolve.  We will stop at the
		// first resolved dependency and locate segments again.  This takes
		// care of the case when multiple regions are dependent on the same
		// "atend(.regionname)".
		newResolve = FALSE;
		pos = m_LinkRegions.GetStartPosition();
		while (pos)
		{
			// Get next link region
			m_LinkRegions.GetNextAssoc(pos, name, (VTObject *&) pLinkRgn);

			// Test if this region has been located completely
			if (pLinkRgn->m_pFirstAddrRange->startAddr < 0)
			{
				// This region has a dependency still.  Test if it's dependant
				// region has been located (and has a valid start / end address)
				CLinkRgn*	pDepLinkRgn;
				if (m_LinkRegions.Lookup((const char *) pLinkRgn->m_AtEndRgn, (VTObject *&) pDepLinkRgn))
				{
					// Test if the dependant link region has been located
					if (pDepLinkRgn->m_LocateComplete)
					{
						// Resolve the dependency by locating the link region at the
						// end of the dependant region
						lastAddr = pDepLinkRgn->m_pLastAddrRange->endAddr+1;
						for (c = pDepLinkRgn->m_pLastAddrRange->endAddr; c >= 
							pDepLinkRgn->m_pLastAddrRange->startAddr; c--)
						{
							// Find the last address used by the last address range
							if (m_SegMap[c] != NULL)
								break;
							lastAddr--;
						}

						pLinkRgn->m_pFirstAddrRange->startAddr = lastAddr;
						pLinkRgn->m_pFirstAddrRange->nextLocateAddr = lastAddr;

						// Test if the link region linked-list needs to be re-ordered

						// Indiacate a new dependency has been resolved and exit the while loop
						newResolve = TRUE;
						break;
					}
				}
			}
		}

		// Check if any new dependencies resolved
		if (newResolve == FALSE)
			allDependenciesMet = TRUE;
	}

	return TRUE;
}

/*
============================================================================
Now that all segments have been located, this function assigns physical
addresses to local values, such as data, jump and call addresses, etc.
============================================================================
*/
CObjFileSection* VTLinker::FindRelSection(CObjFile* pObjFile, Elf32_Rel* pRel)
{
	int					sectCount, sect;
	CObjFileSection*	pFileSect;
	CObjFileSection*	pRelSect;

	pRelSect = NULL;
	sectCount = pObjFile->m_FileSections.GetSize();
	for (sect = 0; sect < sectCount; sect++)
	{
		// Loop through relocation segments
		pFileSect = (CObjFileSection *) pObjFile->m_FileSections[sect];
		if (pFileSect->m_ElfHeader.sh_offset < pRel->r_offset)
		{
			// Validate the section type
			if ((ELF32_R_TYPE(pRel->r_info) == SR_ADDR_XLATE) &&
				(pFileSect->m_ElfHeader.sh_type != SHT_PROGBITS))
					continue;

			// If no rel section assigned yet, just assign it
			if (pRelSect == NULL)
				pRelSect = pFileSect;

			// Else find the nearest section to the offset
			else if (pRel->r_offset - pFileSect->m_ElfHeader.sh_offset <
				pRel->r_offset - pRelSect->m_ElfHeader.sh_offset)
					pRelSect = pFileSect;
		}
	}

	return pRelSect;
}

/*
============================================================================
Now that all segments have been located, this function assigns physical
addresses to local values, such as data, jump and call addresses, etc.
============================================================================
*/
int VTLinker::ResolveLocals()
{
	POSITION			pos;
	CObjFile*			pObjFile;
	CObjFileSection*	pFileSect;
	CObjFileSection*	pRelSect;
	int					sect, sectCount, rel, relCount;
	MString				err, filename;

	// Exit if error has occurred
	if (m_Errors.GetSize() != 0)
		return FALSE;

	// Loop for all object files loaded
	pos = m_ObjFiles.GetStartPosition();
	while (pos != NULL)
	{
		// Get pointer to this object file's data
		m_ObjFiles.GetNextAssoc(pos, filename, (VTObject *&) pObjFile);

		// Loop through all segments and look for relocation info
		sectCount = pObjFile->m_FileSections.GetSize();
		for (sect = 0; sect < sectCount; sect++)
		{
			// Loop through relocation segments
			pFileSect = (CObjFileSection *) pObjFile->m_FileSections[sect];
			relCount = pFileSect->m_Reloc.GetSize();
			for (rel = 0; rel < relCount; rel++)
			{
				// Process this relocation item
				Elf32_Rel* pRel = (Elf32_Rel *) pFileSect->m_Reloc[rel];
				if (ELF32_R_TYPE(pRel->r_info) == SR_ADDR_XLATE)
				{
					CObjFileSection* pRefSect = (CObjFileSection *) pObjFile->m_FileSections[pFileSect->m_ElfHeader.sh_info];
					pRelSect = FindRelSection(pObjFile, pRel);
					if (pRelSect != NULL)
					{
						// calculate the offset into the section
						int offset = pRel->r_offset - pRelSect->m_ElfHeader.sh_offset;

						// Get the relative jump address from the program bits
						unsigned short addr = (((unsigned short) pRelSect->m_pProgBytes[offset]) & 0xFF) | (pRelSect->m_pProgBytes[offset+1] << 8);

						// Update the address based on the segments locate address
//						addr += pRelSect->m_LocateAddr;
						addr += pRefSect->m_LocateAddr;

						// Update the translated address in the ProgBits
						pRelSect->m_pProgBytes[offset] = addr & 0xFF;
						pRelSect->m_pProgBytes[offset + 1] = addr >> 8;

						// Change the relocation type so we know we have translated this item
						pRel->r_info = ELF32_R_INFO(0, SR_ADDR_PROCESSED);
					}
					else
					{
						err.Format("%sInvalid relocation at offset %d", gsEdl, pRel->r_offset);
						m_Errors.Add(err);
					}
				}
			}
		}
	}

	return TRUE;
}

/*
============================================================================
Now resolved all extern symbols.
============================================================================
*/
int VTLinker::ResolveExterns()
{
	POSITION			pos;
	CObjFile*			pObjFile;
	CObjFileSection*	pFileSect;
	int					sect, sectCount, rel, relCount;
	MString				err, filename;

	// Exit if error has occurred
	if (m_Errors.GetSize() != 0)
		return FALSE;

	// Loop for all object files loaded
	pos = m_ObjFiles.GetStartPosition();
	while (pos != NULL)
	{
		// Get pointer to this object file's data
		m_ObjFiles.GetNextAssoc(pos, filename, (VTObject *&) pObjFile);

		// Loop through all segments and look for relocation info
		sectCount = pObjFile->m_FileSections.GetSize();
		for (sect = 0; sect < sectCount; sect++)
		{
			// Loop through relocation segments
			pFileSect = (CObjFileSection *) pObjFile->m_FileSections[sect];

			if (pFileSect->m_Type != SHT_REL)
				continue;
			
			CObjFileSection* pObjSect = (CObjFileSection *) pObjFile->m_FileSections[pFileSect->m_ElfHeader.sh_info];
			relCount = pFileSect->m_Reloc.GetSize();
			for (rel = 0; rel < relCount; rel++)
			{
				// Process this relocation item
				Elf32_Rel* pRel = (Elf32_Rel *) pFileSect->m_Reloc[rel];

				// Test if this is an EXTERN relocation type
				if (ELF32_R_TYPE(pRel->r_info) == SR_EXTERN)
				{
					// Get the EXTERN symbol name
					if (pObjFile->m_pStrTab != NULL)
					{
						// First get symbol index from relocation item
						int index = ELF32_R_SYM(pRel->r_info);

						// Now get pointer to symbol data
						Elf32_Sym* pSym	= (Elf32_Sym *) pObjFile->m_pSymSect->m_Symbols[index];

						// Next lookup symbol name in string table
						const char *pSymName = &pObjFile->m_pStrTab[pSym->st_name];

						// Search for the symbol name in the global map
						CObjSymFile* pSymFile;
						if (m_Symbols.Lookup(pSymName, (VTObject *&) pSymFile))
						{
							// PUBLIC symbol found!  Now perform the link
							// First get a pointer to the section referenced by the symbol
							CObjFileSection* pSymSect = (CObjFileSection *) pSymFile->m_pObjFile->m_FileSections[pSymFile->m_pSym->st_shndx];

							// Calculate the value of the EXTERN symbol
							unsigned short value = (unsigned short) pSymFile->m_pSym->st_value;
							if (pSymSect->m_Type != ASEG)
								value +=(unsigned short) pSymSect->m_LocateAddr;

							// Calculate the address where we update with the symbol's value
							if (pRel->r_offset + 1 >= (unsigned short) pObjSect->m_Size)
							{
								err.Format("Invalid relocation offset for section %s, symbol = %s, offset = %d, size = %d\n",
									(const char *) pObjSect->m_Name, (const char *) pSymName, pRel->r_offset, pObjSect->m_Size);
								m_Errors.Add(err);
							}
							else
							{
								char * pAddr = pObjSect->m_pProgBytes + pRel->r_offset;

								// Update the code
								*pAddr++ = value & 0xFF;
								*pAddr = value >> 8;
							}
						}
						else
						{
							MString title = MakeTitle(pObjFile->m_Name);
							err.Format("%sUnresolved EXTERN symbol %s referenced in file %s", gsEdl, 
								pSymName, (const char *) title);
							m_Errors.Add(err);
						}
					}
					else
					{
						MString title = MakeTitle(pObjFile->m_Name);
						err.Format("%sFile %s has no string table", gsEdl, (const char *) title);
						m_Errors.Add(err);
					}
				}
			}
		}
	}

	return TRUE;
}

/*
============================================================================
Generate the output file(s).  For .CO projects, this will be the .CO file,
for library files, a .lib, and for ROM projects, it will be a HEX file.
============================================================================
*/
int VTLinker::GenerateOutputFile()
{
	int					startAddr, endAddr, entryAddr;
	int					c;
	unsigned char		temp;
	const int			addrSize = sizeof(m_SegMap) / sizeof(void *);
	MString				err;
	CObjSymFile*		pSymFile;
	MString				filename = m_RootPath + (char *) "/" + m_OutputName;

	// Exit if error has occurred
	if (m_Errors.GetSize() != 0)
		return FALSE;

	// Find first address of output
	for (startAddr = 0; startAddr < addrSize; startAddr++)
		if (m_SegMap[startAddr] != NULL)
			break;

	// Find last address of output
	for (endAddr = addrSize - 1; endAddr >= 0; endAddr--)
	{
		if (m_SegMap[endAddr] != NULL)
		{
			// Test if this is the ".bss" segemnt - it doesn't get written
			// to the output file
			if (m_SegMap[endAddr]->m_Name == ".bss")
				continue;
			break;
		}
	}

	// Save the start address and entry address
	m_StartAddress = startAddr;
	m_EntryAddress = startAddr;
	
	// If build type is .CO, then create the .CO file
	if (m_ProjectType == VT_PROJ_TYPE_CO)
	{
		// Determine the entry address.  If an ENTRY specification was given
		// in the linker script, then find that symbol, otherwise use the
		// first address of the program
		entryAddr = startAddr;
		if (!m_EntryLabel.IsEmpty())
		{
			// Search the global symbol table for the PUBLIC label
			if (m_Symbols.Lookup((const char *) m_EntryLabel, (VTObject *&) pSymFile))
			{
				// PUBLIC symbol found!  Now perform the link
				// First get a pointer to the section referenced by the symbol
				CObjFileSection* pSymSect = (CObjFileSection *) pSymFile->m_pObjFile->m_FileSections[pSymFile->m_pSym->st_shndx];

				// Calculate the value of the EXTERN symbol
				entryAddr = (unsigned short) (pSymFile->m_pSym->st_value + pSymSect->m_LocateAddr);
			}
			else
			{
				err.Format("%sUnresolved entry address %s - not defined by any object file", gsEdl, (const char *) m_EntryLabel);
				m_Errors.Add(err);
				return FALSE;
			}
		}

		// Update the entry address
		m_EntryAddress = entryAddr;

		// Report that we are generating the output file
		if (m_pStdoutFunc != NULL)
		{
			MString msg;
			msg.Format("Generating file %s, start=0x%0X, entry=0x%0X, length=%d\n", (const char *) m_OutputName, startAddr, entryAddr, endAddr - startAddr + 1);
			m_pStdoutFunc(m_pStdoutContext, (const char *) msg);
		}

		// Open the output file for output
		FILE* fd;
		if ((fd = fopen((const char *) filename, "wb")) == NULL)
		{
			err.Format("%sUnable to open output file %s", gsEdl, (const char *) m_OutputName);
			m_Errors.Add(err);
			return FALSE;
		}

		// Write the .CO header
		temp = (unsigned char) (startAddr & 0xFF);
		fwrite(&temp, 1, sizeof(temp), fd);
		temp = (unsigned char) (startAddr >> 8);
		fwrite(&temp, 1, sizeof(temp), fd);

		// Write the length
		temp = (unsigned char) ((endAddr - startAddr + 1) & 0xFF);
		fwrite(&temp, 1, sizeof(temp), fd);
		temp = (unsigned char) ((endAddr - startAddr + 1) >> 8);
		fwrite(&temp, 1, sizeof(temp), fd);

		// Write the entry address
		temp = (unsigned char) (entryAddr & 0xFF);
		fwrite(&temp, 1, sizeof(temp), fd);
		temp = (unsigned char) (entryAddr >> 8);
		fwrite(&temp, 1, sizeof(temp), fd);

		// Finally write the output data to the file
		unsigned char	zero = 0;
		for (c = startAddr; c <= endAddr; )
		{
			// Write the next block of bytes
			CObjFileSection *pSect = m_SegMap[c];
			fwrite(pSect->m_pProgBytes, 1, pSect->m_Size, fd);
		
			// Update to the next byte to write
			c += pSect->m_Size;

			while ((m_SegMap[c] == NULL || m_SegMap[endAddr]->m_Name == ".bss") &&
				c <= endAddr)
			{
				fwrite(&zero, 1, 1, fd);
				c++;
			}
		}

		// Close the output file
		fclose(fd);

		// Now check if we need to generate a loader for the .CO
		if (m_LoaderFilename != "")
		{
			MString msg;
			msg.Format("Generating BASIC loader file %s\n", (const char *) m_LoaderFilename);
			m_pStdoutFunc(m_pStdoutContext, (const char *) msg);
			GenerateLoaderFile(startAddr, endAddr, entryAddr);
		}
	}

	// Test if project type is Assembly ROM and create HEX output if it is
	else if (m_ProjectType == VT_PROJ_TYPE_ROM)
	{
		// Report that we are generating the output file
		if (m_pStdoutFunc != NULL)
		{
			MString msg;
			msg.Format("Generating HEX file %s\n", (const char *) m_OutputName);
			m_pStdoutFunc(m_pStdoutContext, (const char *) msg);
		}

		// Open the output file for output
		FILE* fd;
		if ((fd = fopen((const char *) filename, "wb")) == NULL)
		{
			err.Format("%sUnable to open output file %s", gsEdl, (const char *) filename);
			m_Errors.Add(err);
			return FALSE;
		}

		// Allocate a 32K memory region to hold the ROM contents
		char * pRom = new char[32768];

		// Fill contents with zero
		for (c = 0; c < 32768; c++)
			pRom[c] = 0;

		// Copy the output data to the ROM storage
		for (c = 0; c < 32768; )
		{
			// Skip empty space in the link map
			while (m_SegMap[c] == NULL && c < 32768)
				c++;

			// Write the next block of bytes
			if (c < 32768)
			{
				CObjFileSection *pSect = m_SegMap[c];
				if (pSect != NULL)
				{
					memcpy(&pRom[c], pSect->m_pProgBytes, pSect->m_Size);
			
					// Update to the next byte to write
					c += pSect->m_Size;
				}
			}
		}

		// Write the ROM data to the HEX file.  File is closed by the routine
		save_hex_file_buf(pRom, 0, 32767, 0, fd);

		// Write the data out as a REX .BX file also...
		int dot = m_OutputName.ReverseFind('.');
		MString bxFile = m_OutputName.Left(dot) + (char *) ".bx";

		// Report that we are generating the output file
		if (m_pStdoutFunc != NULL)
		{
			MString msg;
			msg.Format("Generating REX file %s\n", (const char *) bxFile);
			m_pStdoutFunc(m_pStdoutContext, (const char *) msg);
		}
		filename = m_RootPath + (char *) "/" + bxFile;

		// Open the output file for output
		if ((fd = fopen((const char *) filename, "wb")) == NULL)
		{
			err.Format("%sUnable to open output file %s", gsEdl, (const char *) filename);
			m_Errors.Add(err);
			return FALSE;
		}

		// The REX BX file is a simple binary dump of the ROM contents
		fwrite(pRom, 1, 32768, fd);
		fclose(fd);

		// Delete the ROM memory
		delete pRom;
	}

	// We don't support Library output yet, but soon...
	else if (m_ProjectType == VT_PROJ_TYPE_LIB)
	{
	}
	return TRUE;
}

/*
============================================================================
Generates a quintuple, base 85 number from the given ascii85 value and 
writes it to the file while updating control variables.
============================================================================
*/
int VTLinker::CreateQuintuple(FILE* fd, unsigned long& ascii85, int& lineNo, 
					int& dataCount, int& lastDataFilePos, 
					int& lineCount, int& checksum)
{
	int		q;
	char	quintuple[5];

	// Loop for 5 digits of base 85 number
	for (q = 4; q >= 0; q--)
	{
		// Create base 85 number
		quintuple[q] = (char) (ascii85 % 85) + '#';
		checksum += quintuple[q];
		ascii85 /= 85;
	}

	// Write results to file
	if (fwrite(quintuple, 1, 5, fd) != 5)
	{
		// Error writing to file
		return FALSE;
	}

	// Add 5 more bytes to line length and check if time for a new line
	lineCount += 5;
	if (lineCount >= MAX_LOADER_LINE_LEN)
	{
		// Terminate the current data statement
		fprintf(fd, "\"\x0D\x0A");
		lineCount = 0;

		// Start a new data line.  First grab the file position
		lastDataFilePos = ftell(fd);
		fprintf(fd, "%d DATA\"", ++lineNo);
		dataCount++;
	}

	return TRUE;
}

/*
============================================================================
Generates a BASIC loader text file for the generated .CO file.
============================================================================
*/
int VTLinker::GenerateLoaderFile(int startAddr, int endAddr, int entryAddr)
{
	MString		err;
	FILE*		fd;
	int			checksum, lineNo, dataCount, quadtuple, quintupleCount;
	int			lastDataFilePos, lineCount;
	int			c, x;
	unsigned	long ascii85;
	MString		filePath;

	// Generate file path
	filePath = m_RootPath + (char *) "/" + m_LoaderFilename;

	// Open the output file for output
	if ((fd = fopen((const char *) filePath, "wb")) == NULL)
	{
		err.Format("%sUnable to open output file %s", gsEdl, (const char *) m_LoaderFilename);
		m_Errors.Add(err);
		return FALSE;
	}

	// Write the output data to the file
	lineNo = checksum = 0;
	lineCount = quadtuple = quintupleCount = 0;
	ascii85 = 0;

	// Issue the 1st Data statement
	lastDataFilePos = ftell(fd);
	fprintf(fd, "%d DATA\"", ++lineNo);
	dataCount = 1;

	// Loop for all data
	for (c = startAddr; c <= endAddr; )
	{
		// Write the next block of bytes
		CObjFileSection *pSect = m_SegMap[c];
		for (x = 0; x < pSect->m_Size; x++)
		{
			// Munge next byte into ascii85 calculation
			ascii85 = ascii85 * 256 + (unsigned char) pSect->m_pProgBytes[x];
			
			// Check if time to output a quintuple from our quadtuple
			if (++quadtuple == 4)
			{
				CreateQuintuple(fd, ascii85, lineNo, dataCount, lastDataFilePos, 
					lineCount, checksum);

				// Reset the quadtuple counter
				quadtuple = 0;
				ascii85 = 0;
				quintupleCount++;
			}
		}

		// Update to the next byte to write
		c += pSect->m_Size;

		while ((m_SegMap[c] == NULL || m_SegMap[endAddr]->m_Name == ".bss") &&
			c <= endAddr)
		{
			// We added a zero.  Check if time to output a quintuple from our quadtuple
			if (++quadtuple == 4)
			{
				CreateQuintuple(fd, ascii85, lineNo, dataCount, lastDataFilePos, 
					lineCount, checksum);

				// Reset the quadtuple counter
				quadtuple = 0;
				ascii85 = 0;
				quintupleCount++;
			}
			c++;
		}
	}

	// Check if anything left over that didn't get written yet
	if (ascii85 != 0)
	{
		CreateQuintuple(fd, ascii85, lineNo, dataCount, lastDataFilePos, 
			lineCount, checksum);
		quintupleCount++;
	}

	// Check if last data statement was empty
	if (lineCount == 0)
	{
		// Rewind file to last data statement to remove the statement
		fseek(fd, lastDataFilePos, SEEK_SET);
		dataCount--;
	}
	else
	{
		// Terminate the last DATA statement
		fputs("\"\x0D\x0A", fd);
	}

	// Now write the rest of the program to the file
	for (x = 0; x < 7; x++)
		fwrite(gLoaderCode[x], 1, strlen(gLoaderCode[x]), fd);

	// Calculate increment value for progress indicator
	double inc = 128.0;
	if (quintupleCount != 0)
		inc = (double) 128 / (double) quintupleCount;

	// Print next line with increment, start address, and data count
	char	coname[32];
	m_LoaderFilename.MakeUpper();
	strcpy(coname, (const char *) m_LoaderFilename);
	fl_filename_setext(coname, sizeof(coname), ".CO");

	// The PC-8201 and PC-8300 use LOCATE and BSAVE instead of PRINT@ and SAVEM
	if (m_TargetModel == MODEL_PC8201 || m_TargetModel == MODEL_PC8300)
	{
		fprintf(fd, gLoaderCode[7+4], (const char *) coname, (float) inc, ((int) startAddr)-65536, dataCount);

		// Print line with checksum validation and CLEAR statement
		fprintf(fd, gLoaderCode[8+4], checksum, startAddr);

		// Print the line with the SAVEM filename command
		fprintf(fd, gLoaderCode[9+4], (const char *) coname, startAddr, endAddr-startAddr+1, entryAddr);

		// Finally print the line reporting the checksum error
		fprintf(fd, gLoaderCode[10+4], checksum);
	}
	else
	{
		fprintf(fd, gLoaderCode[7], (const char *) coname, (float) inc, ((int) startAddr)-65536, dataCount);

		// Print line with checksum validation and CLEAR statement
		fprintf(fd, gLoaderCode[8], checksum, startAddr);

		// Print the line with the SAVEM filename command
		fprintf(fd, gLoaderCode[9], (const char *) coname, startAddr, endAddr, entryAddr);

		// Finally print the line reporting the checksum error
		fprintf(fd, gLoaderCode[10], checksum);
	}

	// Now close the file and we're all done
	fclose(fd);

	return 1;
}

/*
============================================================================
Generates a map file if one was requested and no errors exist during the
assemble / link.
============================================================================
*/
int VTLinker::GenerateMapFile(void)
{
	MString		err, filename;
	FILE*		fd;
	int			c, count;
	int			codeSize = 0, dataSize = 0;
	VTObArray	m_Sorted;

	// Exit if error has occurred
	if (m_Errors.GetSize() != 0)
		return FALSE;

	// Test if MAP file generation requested
	if (m_LinkOptions.Find((char *) "-m") == -1)
		return FALSE;

	// Okay, generate the map file, replacing any old one found
	int dot = m_OutputName.ReverseFind('.');
	MString mapFile = m_OutputName.Left(dot) + (char *) ".map";

	// Report that we are generating the output file
	if (m_pStdoutFunc != NULL)
	{
		MString msg;
		msg.Format("Generating MAP file %s\n", (const char *) mapFile);
		m_pStdoutFunc(m_pStdoutContext, (const char *) msg);
	}
	filename = m_RootPath + (char *) "/" + mapFile;

	// Open the output file for output
	if ((fd = fopen((const char *) filename, "wb")) == NULL)
	{
		err.Format("%sUnable to open output file %s", gsEdl, (const char *) filename);
		m_Errors.Add(err);
		return FALSE;
	}

	fprintf(fd, " VirtualT %s, Linker\n", VERSION);
	fprintf(fd, " Linker Map File - %s\n\n", (const char *) mapFile); 
	fprintf(fd, "                                 Section Info\n");
	fprintf(fd, "                  Section    Type  Address  End Addr  Size(Bytes)\n");
	fprintf(fd, "                ---------  ------  -------  --------  ----------\n");

	// Loop through the segment map and report the location of all segments
	for (c = 0; c < 65536; )
	{
		// Skip empty space in the link map
		while (m_SegMap[c] == NULL && c < 65536)
			c++;

		if (c == 65536)
			break;

		// Write the next block of bytes
		CObjFileSection *pSect = m_SegMap[c];
		if (pSect != NULL)
		{
			const char *pType;
			if (pSect->m_Type == DSEG)
				pType = "data";
			else
				pType = "code";
			int size = pSect->m_Size;

			// Check segments after this one for the same name and concatenate them
			// together in the map listing
			CObjFileSection *pNextSect = m_SegMap[c + pSect->m_Size];
			int next = c + pSect->m_Size;
			while (pNextSect != NULL)
			{
				if (pNextSect->m_Name == pSect->m_Name)
				{
					size += pNextSect->m_Size;
					next += pNextSect->m_Size;
					pNextSect = m_SegMap[next];
				}
				else
					break;
			}

			fprintf(fd, "%25s  %6s   0x%04x    0x%04x    0x%04x\n", (const char *) pSect->m_Name,
				pType, pSect->m_LocateAddr, pSect->m_LocateAddr + size - 1, size);

			// Keep track of total code and data size
			if (pSect->m_Type == DSEG)
				dataSize += size;
			else
				codeSize += size;

			c += size;
		}
	}

	// Report the code and data usage
	fprintf(fd, "\n\n               Code size = %d,  Data size = %d,  Total = %d\n\n\n\n", 
		m_TotalCodeSpace, m_TotalDataSpace, m_TotalCodeSpace + m_TotalDataSpace);

	// Now report the symbols and their values
	POSITION		pos;
	CObjSymFile*	pObjSymFile;
	MString			key;

	fprintf(fd, "                              Symbols - Sorted by Name\n");
	fprintf(fd, "                     Name  Address       Type  File\n");
	fprintf(fd, "                ---------  -------  ---------  ---------\n");               

	// Loop through all Symbols and delete them
	pos = m_Symbols.GetStartPosition();
	while (pos != NULL)
	{
		m_Symbols.GetNextAssoc(pos, key, (VTObject *&) pObjSymFile);
		CObjSymFile* pComp;

		count = m_Sorted.GetSize();
		for (c = 0; c < count; c++)
		{
			pComp = (CObjSymFile *) m_Sorted[c];
			if (strcmp(pComp->m_pName, pObjSymFile->m_pName) > 0)
				break;
		}
		m_Sorted.InsertAt(c, pObjSymFile);
	}

	// Now print the sorted array
	count = m_Sorted.GetSize();
	for (c = 0; c < count; c++)
	{
		pObjSymFile = (CObjSymFile *) m_Sorted[c];

		CObjFileSection* pSymSect = (CObjFileSection *) pObjSymFile->m_pObjFile->m_FileSections[
			pObjSymFile->m_pSym->st_shndx];

		fprintf(fd, "%25s   0x%04x  %9s  %s\n", pObjSymFile->m_pName, 
			(int) (pObjSymFile->m_pSym->st_value + pSymSect->m_LocateAddr), 
			ELF32_ST_TYPE(pObjSymFile->m_pSym->st_info) == STT_OBJECT ? (char *) "data" : (char *) "function", 
			(const char *) pObjSymFile->m_pObjFile->m_Name);
	}

	// Remove all items from the sort array and re-sort by address
	m_Sorted.RemoveAll();

	fprintf(fd, "\n\n\n                              Symbols - Sorted by Address\n");
	fprintf(fd, "                     Name  Address       Type  File\n");
	fprintf(fd, "                ---------  -------  ---------  ---------\n");               

	// Loop through all Symbols and delete them
	pos = m_Symbols.GetStartPosition();
	while (pos != NULL)
	{
		m_Symbols.GetNextAssoc(pos, key, (VTObject *&) pObjSymFile);
		CObjFileSection* pSymSect = (CObjFileSection *) pObjSymFile->m_pObjFile->m_FileSections[
			pObjSymFile->m_pSym->st_shndx];
		CObjSymFile* pComp;

		count = m_Sorted.GetSize();
		for (c = 0; c < count; c++)
		{
			pComp = (CObjSymFile *) m_Sorted[c];
			CObjFileSection* pCompSect = (CObjFileSection *) pComp->m_pObjFile->m_FileSections[
				pComp->m_pSym->st_shndx];
			if (pComp->m_pSym->st_value + pCompSect->m_LocateAddr > 
				pObjSymFile->m_pSym->st_value + pSymSect->m_LocateAddr)
					break;
		}
		m_Sorted.InsertAt(c, pObjSymFile);
	}

	// Now print the sorted array
	count = m_Sorted.GetSize();
	for (c = 0; c < count; c++)
	{
		pObjSymFile = (CObjSymFile *) m_Sorted[c];
		CObjFileSection* pSymSect = (CObjFileSection *) pObjSymFile->m_pObjFile->m_FileSections[
			pObjSymFile->m_pSym->st_shndx];

		fprintf(fd, "%25s   0x%04x  %9s  %s\n", pObjSymFile->m_pName, 
			(int) (pObjSymFile->m_pSym->st_value + pSymSect->m_LocateAddr), 
			ELF32_ST_TYPE(pObjSymFile->m_pSym->st_info) == STT_OBJECT ? (char *) "data" : (char *) "function", 
			(const char *) pObjSymFile->m_pObjFile->m_Name);
	}

	// Remove all entries from the Sorted array
	m_Sorted.RemoveAll();

	fprintf(fd, "\n\n");

	// Close the map file
	fclose(fd);

	return TRUE;
}

/*
============================================================================
Back annotate listing files with actual addresses from the link.
============================================================================
*/
int VTLinker::BackAnnotateListingFiles(void)
{
	POSITION			pos;
	CObjFile*			pObjFile;
//	CObjFileSection*	pFileSect;
	MString				err, filename;

	// Exit if error has occurred
	if (m_Errors.GetSize() != 0)
		return FALSE;

	// Test if list file generation requested
	if (m_LinkOptions.Find((char *) "-t") == -1)
		return FALSE;

	// Loop for all object files loaded and locate any that have not been located yet
	pos = m_ObjFiles.GetStartPosition();
	while (pos != NULL)
	{
		// Get pointer to this object file's data
		m_ObjFiles.GetNextAssoc(pos, filename, (VTObject *&) pObjFile);

		// Get the path of the source file for this object file
	}

	return TRUE;
}

/*
============================================================================
The parser calls this function to perform the link operation after all
files have been assembled.
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
	
	// Load all files to be linked
	ReadObjFiles();

	// Locate segments based on commands in the Linker Script
	LocateSegments();

	// Resolve static (local) symbols for each segment
	ResolveLocals();

	// Resolve segment extern symbols
	ResolveExterns();

	// Generate output file
	GenerateOutputFile();

	// Generate debug information

	// Generate map file
	GenerateMapFile();

	// Back annotate Listing files with actual addresses
	BackAnnotateListingFiles();

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
Sets the name of the BASIC loader file to be generated
============================================================================
*/
void VTLinker::SetLoaderFilename(const MString& loaderFilename)
{
	// Save the string in case we need it later
	m_LoaderFilename = loaderFilename;
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
	m_ObjDirs.Add(m_RootPath+(char *) "/");

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
Returns the Start address (lowest address) of the generated code.
============================================================================
*/
unsigned short VTLinker::GetStartAddress(void)
{
	return m_StartAddress;
}

/*
============================================================================
Returns the Entry address (lowest address) of the generated code.
============================================================================
*/
unsigned short VTLinker::GetEntryAddress(void)
{
	return m_EntryAddress;
}

/*
============================================================================
Constructor for the CLinkRgn object.  This object is used to keep track of
regions defined in the linker script.
============================================================================
*/
CLinkRgn::CLinkRgn(int type, MString& name, int startAddr, int endAddr, int prot, 
				   const char* pAtEndName, int atend)
{
	m_Type = type; 
	m_Name = name; 
	if (pAtEndName != NULL)
		m_AtEndRgn = pAtEndName;
	m_Protected = prot;
	m_AtEnd = atend;
	m_LocateComplete = FALSE;
	m_pFirstAddrRange = new LinkAddrRange;
	m_pLastAddrRange = m_pFirstAddrRange;
	m_pFirstAddrRange->startAddr = startAddr;
	m_pFirstAddrRange->endAddr = endAddr;
	m_pFirstAddrRange->pNext = NULL;
	m_pFirstAddrRange->pPrev = NULL;
	m_pFirstAddrRange->nextLocateAddr = startAddr;
	m_pFirstAddrRange->endLocateAddr = endAddr;
	m_NextLocateAddr = startAddr;
	m_EndLocateAddr = endAddr;
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
