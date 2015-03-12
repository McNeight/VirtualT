/*
========================================================================
VTLinker:	This class implements an 8085 Linker for the
			VirtualT project.

Written:	11/13/09  Kenneth D. Pettit

========================================================================
*/

#include		"VirtualT.h"
#include		"linker.h"
#include 		"assemble.h"
#include		"elf.h"
#include		"project.h"
#include		<FL/Fl.H>
#include		<math.h>
#include		<string.h>
#include		<stdio.h>
#include		<stdlib.h>
#include        <ctype.h>
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
    m_LinkDone = FALSE;
	m_LinkScriptRaw = FALSE;
	
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
	int				count, c;
	CObjFileSection	*pSect;

	count = m_FileSections.GetSize();
	for (c = 0; c < count; c++)
	{
		pSect = (CObjFileSection *) m_FileSections[c]; 
		delete pSect;
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
	m_pProgBytes = NULL;
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
    int             x;

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

    // Delete all entries in the Prelink script
    for (x = 0; x < m_PrelinkScript.m_Script.GetSize(); x++)
    {
        CScriptCmd *pCmd = (CScriptCmd *) m_PrelinkScript.m_Script[x];
        delete pCmd;
    }
    m_PrelinkScript.m_Script.RemoveAll();

    // Delete all entries in the Postlink script
    for (x = 0; x < m_PostlinkScript.m_Script.GetSize(); x++)
    {
        CScriptCmd *pCmd = (CScriptCmd *) m_PostlinkScript.m_Script[x];
        delete pCmd;
    }
    m_PostlinkScript.m_Script.RemoveAll();

    // Delete all entries in the m_Defines map
	pos = m_Defines.GetStartPosition();
    CDefine* pDefine;
	while (pos != NULL)
	{
		m_Defines.GetNextAssoc(pos, key, (VTObject *&) pDefine);
		delete pDefine;
	}
	m_Defines.RemoveAll();
	
	m_ProgSections.RemoveAll();

    // Reset the AddrLines array
    for (x = 0; x < 65536; x++)
        m_AddrLines.a[x].pObjFile = NULL;

	// Assign active assembly pointers
	m_Hex = FALSE;
	m_DebugInfo = FALSE;
	m_Map = FALSE;
	m_EntryAddress = 0;
	m_StartAddress = 0;
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

            // Test for OptROM Bytes
            else if (*pTok == 'b')
            {
                // Get pointer to next token
                pTok = strtok(NULL, pDelim);
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
	
	// Test for PRELINK specification
	else if (strcmp(pStr, "PRELINK") == 0)
		command = LKR_CMD_PRELINK;
	
	// Test for POSTLINK specification
	else if (strcmp(pStr, "POSTLINK") == 0)
		command = LKR_CMD_POSTLINK;
	
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

	// Test if command is PRELINK or POSTLINK
	else if ((m_Command == LKR_CMD_PRELINK) || (m_Command == LKR_CMD_POSTLINK))
	{
		if (strcmp(pStr, "{") != 0)
		{
			err.Format("Error in line %d(%s):  Expected open brace", lineNo,
				(const char *) m_LinkerScript);
			m_Errors.Add(err);
			m_Command = LKR_CMD_ERROR;
		}
        else
            m_LinkScriptRaw = TRUE;
	}
}

/*
============================================================================
This routine evaluates the string provided and returns it's value.  The
string can be decimal, hex, etc. and can contain simple math or comparisons
such as:

    Preamble_start + (1 + COUNT*4+a)*2
    modeltype=2
    COUNT > 4
    count+1 > 3 || modeltype * 2 < 4

============================================================================
*/
int VTLinker::EvaluateStringEquation(const char *pEq, int lineNo, int* asHex)
{
	int					len = strlen(pEq);
	int					hex = 0;
	int					stack[10];
	int					slevel = 1;
	int					address, x, i;
    char    			op, unwindto;
    char    			*token, *pStr;
	SimpleEqOperator_t	eq[20];
	SimpleEqOperator_t	build[20];
	CLinkRgn			*pLinkRgn;
    CDefine             *pDefine;
	MString 			err;
    char                temp[256];
	int					eqsize = 0;
	int					buildidx = 0;
    int                 value, found;

    strncpy(temp, pEq, sizeof(temp));
    pStr = temp;
    token = pStr;

    while (*pStr == ' ')
        pStr++;
    if (asHex)
    {
        if (*pStr == '$')
            *asHex = 1;
        else
            *asHex = 0;
    }

	for (x = 0; x < len; )
	{
		// Find end of next token
		while (*pStr != '+' && *pStr != '-' && *pStr != '*' && *pStr != '/' && *pStr != 0 &&
				*pStr != '(' && *pStr != ')' && *pStr != '=' && *pStr != '>' && *pStr != '<' &&
                *pStr != '|' && *pStr != '&' && *pStr != '!' && *pStr != ' ')
		{
			pStr++;
			x++;
		}
		op = *pStr;
		*pStr = 0;

		if (strlen(token) > 0)
		{
			if (strcmp(token, "sizeof") == 0 && op == '(')
			{
				// Push SIMPLE_EQ_SIZEOF to build stack
				build[buildidx++].op = SIMPLE_EQ_SIZEOF;
			}
			else
			{
				// Push the value to the equation
				eq[eqsize].op = SIMPLE_EQ_VALUE;
				eq[eqsize++].val = token;
			}
		}

		switch (op)
		{
            case ' ':
                op = SIMPLE_EQ_NOADD;
                unwindto = SIMPLE_EQ_NOADD;
                break;

			case '+': 
				op= SIMPLE_EQ_ADD;
				unwindto = SIMPLE_EQ_ADD;
				break;

			case '-': 
				op = SIMPLE_EQ_SUB; 
				unwindto = SIMPLE_EQ_ADD;
				break;

			case '*': 
				op = SIMPLE_EQ_MULT; 
				unwindto = SIMPLE_EQ_ADD;
				break;
			case '/': 
				op = SIMPLE_EQ_DIV; 
				unwindto = SIMPLE_EQ_ADD;
				break;

			case '(': 
                op = SIMPLE_EQ_PAREN;
				build[buildidx++].op = SIMPLE_EQ_PAREN; 
                unwindto = 99;
				break;

			case ')':
                op = SIMPLE_EQ_NOADD;
				unwindto = SIMPLE_EQ_PAREN;
				break;

			case '|':
                if (*(pStr+1) != '|')
                {
					err.Format("Error in line %d(%s):  Bitwise OR not supported", lineNo,
						(const char *) m_LinkerScript);
					m_Errors.Add(err);
					return 0;
                }
				pStr++;
                op = SIMPLE_EQ_LOGOR;
				unwindto = SIMPLE_EQ_LOGOR;
				break;

			case '&':
                if (*(pStr+1) != '&')
                {
					err.Format("Error in line %d(%s):  Bitwise AND not supported", lineNo,
						(const char *) m_LinkerScript);
					m_Errors.Add(err);
					return 0;
                }
				pStr++;
                op = SIMPLE_EQ_LOGAND;
				unwindto = SIMPLE_EQ_LOGOR;
				break;

            case '=':
                if (*(pStr+1) == '=')
                {
                    // Skip the 2nd = so "==" and "=" are the same
                    pStr++;
                }
                op = SIMPLE_EQ_EQUAL;
                unwindto = SIMPLE_EQ_EQUAL;
                break;

            case '<':
                if (*(pStr+1) == '=')
                {
                    // Skip the '<' 
                    pStr++;
                    op = SIMPLE_EQ_LTE;
                }
                else
                    op = SIMPLE_EQ_LT;
                break;
                unwindto = SIMPLE_EQ_LT;

            case '>':
                if (*(pStr+1) == '=')
                {
                    // Skip the '>' 
                    pStr++;
                    op = SIMPLE_EQ_GTE;
                }
                else
                    op = SIMPLE_EQ_GT;
                break;
                unwindto = SIMPLE_EQ_LT;

			case '!':
				if (*(pStr+1) == '=')
				{
                    // Skip the '=' 
                    pStr++;
					op = SIMPLE_EQ_NOTEQUAL;
					unwindto = SIMPLE_EQ_EQUAL;
				}
				else
				{
					op = SIMPLE_EQ_LOGNOT;
					unwindto = SIMPLE_EQ_NOADD;
				}
				break;

            default:
                op = SIMPLE_EQ_NOADD;
                break;
		}

		// Unwind the build stack
		while (buildidx > 0 && build[buildidx-1].op >= unwindto)
		{
			if (build[buildidx-1].op != SIMPLE_EQ_PAREN)
			{
				// Test for special processing of "sizeof(SECTION)" syntax
				if (build[buildidx-1].op == SIMPLE_EQ_SIZEOF)
				{
					// Pop the sizeof from the build stack
					buildidx--;
					if (eqsize == 0 || eq[eqsize-1].op != SIMPLE_EQ_VALUE)
					{
						// Error condition ... nothing else on the stack.  Must
						// be something like sizeof() 
						// Error condition in equation!  Something like "/3" was specified
						err.Format("Error in line %d(%s):  Malformed sizeof", lineNo,
							(const char *) m_LinkerScript);
						m_Errors.Add(err);
						return 0;
					}

					eq[eqsize-1].op = SIMPLE_EQ_SIZEOF;
					continue;
				}
				eq[eqsize++].op = build[buildidx-1].op;
			}
			buildidx--;
		}

        if (op != SIMPLE_EQ_NOADD)
        {
            build[buildidx++].op = op;
        }

		x++;
		token = ++pStr;
	}
	while (buildidx > 0)
	{
		eq[eqsize++].op = build[buildidx-1].op;
		buildidx--;
	}

	// Evaluate equation
	for (x = 0; x < eqsize; x++)
	{
		switch (eq[x].op)
		{
		case SIMPLE_EQ_VALUE:
			token = eq[x].val;

			// Lookup symbol in active module
			CObjSymFile* pSymFile;
			if (m_Symbols.Lookup(token, (VTObject *&) pSymFile))
			{
				// If symbol was found, try to evaluate it
				if (pSymFile != (CObjSymFile*) NULL)
				{
					// First get a pointer to the section referenced by the symbol
					CObjFileSection* pSymSect = (CObjFileSection *) pSymFile->m_pObjFile->m_FileSections[pSymFile->m_pSym->st_shndx];

					// Get the symbol value
					value = (unsigned short) pSymFile->m_pSym->st_value;
					if (pSymSect->m_Type != ASEG && ELF32_ST_TYPE(pSymFile->m_pSym->st_info)
								!= STT_EQUATE)
					{
						value +=(unsigned short) pSymSect->m_LocateAddr;
					}

					// Symbol is only valid if it is ASEG or has been located
					if (pSymSect->m_Type == ASEG || m_LinkDone)
					{
						stack[slevel++] = value;
					}
					else
						stack[slevel++] = 0;
				}
				else
					stack[slevel++] = 0;
			}
            else if (m_Defines.Lookup(token, (VTObject *&) pDefine))
            {
                // It is a define, evaluate it
                value = EvaluateStringEquation((const char *) pDefine->m_Value, lineNo);
				stack[slevel++] = value;
            }
			else
			{
				// Test for Hex conversion
                int tokenlen = strlen(token);
				if (*token == '$')
				{
					hex = 1;	// Indicate HEX conversion
					token++;		// Skip the '$' hex delimiter
				}
				else if ((*token == '0') && (*(token + 1) == 'x'))
				{
					hex = 1;
					token += 2;	// Skip the "0x" hex delimiter
				}
				else if ((token[tokenlen-1] == 'h') || (token[tokenlen-1] == 'H'))
				{
					hex = 1;
					token[--tokenlen] = '\0';
				}

				// Validate it is a value
                tokenlen = strlen(token);
				for (i = 0; i < tokenlen; i++)
				{
					if (token[i] < '0' || token[i] > '9')
					{
						if (!hex || !isxdigit(token[i]))
						{
							MString err;
							err.Format("Error in line %d(%s):  Unknown symbol %s", lineNo,
								(const char *) m_LinkerScript, token);
							m_Errors.Add(err);
							return 0;
						}
					}
				}

				// Perform the conversion
				if (hex)
					sscanf(token, "%x", &address);
				else
					sscanf(token, "%d", &address);

				stack[slevel++] = address;
			}
			break;

		case SIMPLE_EQ_ADD:
			// Add the 2 top stack items and save in stack
			value = stack[--slevel];
			stack[slevel-1] += value;
			break;

		case SIMPLE_EQ_SUB:
			// Subtract top stack item from next one down
			value = stack[--slevel];
			stack[slevel-1] -= value;
			break;

		case SIMPLE_EQ_MULT:
			// Subtract top stack item from next one down
			value = stack[--slevel];
			stack[slevel-1] *= value;
			break;

		case SIMPLE_EQ_DIV:
			// Subtract top stack item from next one down
			value = stack[--slevel];
			stack[slevel-1] /= value;
			break;

		case SIMPLE_EQ_LOGNOT:
			stack[slevel] = !stack[slevel];
			break;

		case SIMPLE_EQ_EQUAL:
			value = stack[--slevel];
			stack[slevel-1] = value == stack[slevel-1];
			break;

		case SIMPLE_EQ_NOTEQUAL:
			value = stack[--slevel];
			stack[slevel-1] = value != stack[slevel-1];
			break;

		case SIMPLE_EQ_LT:
			value = stack[--slevel];
			stack[slevel-1] = value < stack[slevel-1];
			break;

		case SIMPLE_EQ_LTE:
			value = stack[--slevel];
			stack[slevel-1] = value <= stack[slevel-1];
			break;

		case SIMPLE_EQ_GT:
			value = stack[--slevel];
			stack[slevel-1] = value > stack[slevel-1];
			break;

		case SIMPLE_EQ_GTE:
			value = stack[--slevel];
			stack[slevel-1] = value >= stack[slevel-1];
			break;

		case SIMPLE_EQ_LOGOR:
			value = stack[--slevel];
			stack[slevel-1] = value || stack[slevel-1];
			break;

		case SIMPLE_EQ_LOGAND:
			value = stack[--slevel];
			stack[slevel-1] = value && stack[slevel-1];
			break;

		case SIMPLE_EQ_SIZEOF:
			token = eq[x].val;
			value = 0;
			found = FALSE;

			// Search for token in the Progbits section array
			for (i = 0; i < m_ProgSections.GetSize(); i++)
			{
				// Test if this section name matches.
				if (((CObjFileSection *) m_ProgSections[i])->m_Name == token)
				{
					// Add all sizes of sections that match
					value += ((CObjFileSection *) m_ProgSections[i])->m_Size;
					found = TRUE;
				}
			}
			if (found)
			{
				// Add the value to the stack
				stack[slevel++] = value;
				break;
			}

			// Maybe it's a Linker Region?
			else if (m_LinkRegions.Lookup(token, (VTObject *&) pLinkRgn))
			{
				// Search for Progbits sections located in this region
				for (i = 0; i < m_ProgSections.GetSize(); i++)
				{
					// Test if this section name matches.
					if (((CObjFileSection *) m_ProgSections[i])->m_pLinkRgn == pLinkRgn)
					{
						// Add all sizes of sections that match
						value += ((CObjFileSection *) m_ProgSections[i])->m_Size;
					}
				}

				// Add the size to the stack
				stack[slevel++] = value;
				break;
			}
            else
            {
				MString err;
				// Error condition in equation!  Something like "/3" was specified
				err.Format("Error in line %d(%s):  Unknown symbol '%s'", lineNo,
					(const char *) m_LinkerScript, token);
				m_Errors.Add(err);
                return 0;
            }

			break;
		}

		if (slevel <= 1)
		{
            MString err;
			// Error condition in equation!  Something like "/3" was specified
			err.Format("Error in line %d(%s):  Malformed equation", lineNo,
				(const char *) m_LinkerScript);
			m_Errors.Add(err);
			return 0;
		}
	}

	return stack[slevel - 1];
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
            {
				startAddr = EvaluateStringEquation(pStr + 6, lineNo);
                if (startAddr > 65535)
                {
                    err.Format("Error in line %d(%s):  START address too large", lineNo,
                        (const char *) m_LinkerScript);
                    m_Errors.Add(err);
                    m_Command = LKR_CMD_ERROR;
                }
            }
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
			endAddr = EvaluateStringEquation(pStr + 4, lineNo);
            if (endAddr > 65535)
            {
                err.Format("Error in line %d(%s):  END address too large", lineNo,
                    (const char *) m_LinkerScript);
                m_Errors.Add(err);
                m_Command = LKR_CMD_ERROR;
            }
            else
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
                err.Format("    Range %X-%X  and    %X-%X\n", pThisRange->startAddr, pThisRange->endAddr,
                        startAddr, endAddr);
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
Process a Linker Sub Script
============================================================================
*/
void VTLinker::ProcessScript(CLinkScript *pScript, int singleStep)
{
    MString     msg;
    int         x, count, value, index;
    CScriptCmd  *pCmd;
	CDefine  	*pDefine;
    MString     name, val;

	count = pScript->m_Script.GetSize();
    for (x = singleStep ? pScript->m_executeIdx++ : 0; x < count; x++)
	{
		pCmd = (CScriptCmd *) pScript->m_Script[x];

		switch (pCmd->m_ID)
		{
		case SCR_CMD_IF:
			if (pScript->m_execute == FALSE)
			{
				// Even if we can't execute, we still have to keep
				// track of the nested if condition so we know which
				// else and endif belongs to us
				pScript->m_ifStack[pScript->m_ifdepth].m_CanExecute = FALSE;
				pScript->m_ifStack[pScript->m_ifdepth++].m_StartLine = pCmd->m_Line;
				continue;
			}

			// Evaluate the if condition
			value = EvaluateStringEquation((const char *) pCmd->m_CmdArg,
					pCmd->m_Line);
			if (m_Errors.GetSize() > 0)
				return;

			pScript->m_ifStack[pScript->m_ifdepth].m_StartLine = pCmd->m_Line;
			pScript->m_ifStack[pScript->m_ifdepth].m_CanExecute = pScript->m_execute;
			pScript->m_ifStack[pScript->m_ifdepth].m_Executed = value;
			pScript->m_execute = value;
			pScript->m_ifdepth++;
			break;

		case SCR_CMD_IFDEF:
		case SCR_CMD_IFNDEF:
			if (pScript->m_execute == FALSE)
			{
				// Even if we can't execute, we still have to keep
				// track of the nested if condition so we know which
				// else and endif belongs to us
				pScript->m_ifStack[pScript->m_ifdepth].m_CanExecute = FALSE;
				pScript->m_ifStack[pScript->m_ifdepth++].m_StartLine = pCmd->m_Line;
				continue;
			}

			// Lookup symbol in active module
			CObjSymFile* pSymFile;
			value = m_Symbols.Lookup((const char *) pCmd->m_CmdArg, (VTObject *&) pSymFile);

			pScript->m_ifStack[pScript->m_ifdepth].m_StartLine = pCmd->m_Line;
			pScript->m_ifStack[pScript->m_ifdepth].m_CanExecute = pScript->m_execute;
			if (pCmd->m_ID == SCR_CMD_IFDEF)
			{
				pScript->m_ifStack[pScript->m_ifdepth].m_Executed = value;
				pScript->m_execute = value;
			}
			else
			{
				pScript->m_ifStack[pScript->m_ifdepth].m_Executed = !value;
				pScript->m_execute = !value;
			}
			pScript->m_ifdepth++;
			break;

		case SCR_CMD_ENDIF:
			if (!pScript->m_ifdepth)
			{
				msg.Format("Error in line %d(%s):  #endif with no matching #if statement", pCmd->m_Line,
					(const char *) m_LinkerScript);
				m_Errors.Add(msg);
				return;
			}

			// Pop the ifStack and restore the execute flag
			pScript->m_execute = pScript->m_ifStack[--pScript->m_ifdepth].m_CanExecute;
			break;

		case SCR_CMD_ELSE:
			if (!pScript->m_ifdepth)
			{
				msg.Format("Error in line %d(%s):  #else with no matching #if statement", pCmd->m_Line,
					(const char *) m_LinkerScript);
				m_Errors.Add(msg);
				return;
			}

			// Check if we can execute at this level
			if (pScript->m_ifStack[pScript->m_ifdepth-1].m_CanExecute)
			{
				// If we haven't executed this if, then execute the else
				pScript->m_execute = !pScript->m_ifStack[pScript->m_ifdepth-1].m_Executed;
				pScript->m_ifStack[pScript->m_ifdepth-1].m_Executed = 1;
			}
			break;

		case SCR_CMD_ELSIF:
			if (!pScript->m_ifdepth)
			{
				msg.Format("Error in line %d(%s):  #elsif with no matching #if statement", pCmd->m_Line,
					(const char *) m_LinkerScript);
				m_Errors.Add(msg);
				return;
			}

			// Check if we can execute at this level
			if (pScript->m_ifStack[pScript->m_ifdepth-1].m_CanExecute)
			{
				if (!pScript->m_ifStack[pScript->m_ifdepth-1].m_Executed)
				{
					// We haven't executed yet.  Evaluate and test if we should execute
					// Evaluate the if condition
					value = EvaluateStringEquation((const char *) pCmd->m_CmdArg,
							pCmd->m_Line);
					if (m_Errors.GetSize() > 0)
						return;

					pScript->m_ifStack[pScript->m_ifdepth-1].m_StartLine = pCmd->m_Line;
					pScript->m_ifStack[pScript->m_ifdepth-1].m_Executed = value;
					pScript->m_execute = value;
					break;
				}
				else
				{
					pScript->m_execute = FALSE;
				}
			}
			break;

		case SCR_CMD_DEFINE:
			// Test if we are in a non-execute mode (#if)
			if (pScript->m_execute == FALSE)
				continue;

			// Split the CmdArg into name and value
			pCmd->m_CmdArg.Replace('\t', ' ');
			index = pCmd->m_CmdArg.Find(' ');
			name = pCmd->m_CmdArg.Left(index);
			val = pCmd->m_CmdArg.Right(pCmd->m_CmdArg.GetLength()-index-1);
			val.TrimLeft();
			val.Trim();
			
			// Add a symbol to the m_Defines map.
			if (m_Defines.Lookup((const char *) name, (VTObject *&) pDefine))
			{
				msg.Format("Error in line %d(%s):  symbol %s already defined", pCmd->m_Line,
					(const char *) m_LinkerScript, (const char *) name);
				m_Errors.Add(msg);
				return;
				break;
			}
			if (pDefine != NULL)
			{
				pDefine = new CDefine;
				pDefine->m_Name = name;
				pDefine->m_Value = val;
				m_Defines[name] = pDefine;
			}

			break;

		case SCR_CMD_ECHO:
			// Test if we are in a non-execute mode (#if)
			if (pScript->m_execute == FALSE)
				continue;

			// Echo data to the build window
			if (pCmd->m_CmdArg[0] == '"')
			{
				// Data is a simple string
				msg = pCmd->m_CmdArg.Right(pCmd->m_CmdArg.GetLength()-1);
				msg = msg.Left(msg.GetLength()-1);
				msg.Replace((char *) "\\n", (char *) "\n");
				m_pStdoutFunc(m_pStdoutContext, (const char *) msg);
			}
			else
			{
				// Data is an equation
				int asHex;
				value = EvaluateStringEquation((const char *) pCmd->m_CmdArg, 
						pCmd->m_Line, &asHex);
				if (asHex)
					msg.Format("%X", value);
				else
					msg.Format("%d", value);
				m_pStdoutFunc(m_pStdoutContext, (const char *) msg);
			}
			break;
	
		}

		// Test for single step and return if set
		if (singleStep)
			return;
	}
}

/*
============================================================================
This routine parses lines from the Linker Script that occur between the
open brace and close brace of PRELINK and POSTLINK commands in the script.
============================================================================
*/
void VTLinker::ParseSubScript(char *pStr, int lineNo, CLinkScript* pScript)
{
	char*		token;
	char*		arg;
	MString		cmd, sArg, err;
	CScriptCmd	*pCmd;

	// Test for close brace
	if (pStr[0] == '}')
	{
		// Turn off RAW linker script processing
		m_LinkScriptRaw = FALSE;

		// Indicate sub-script parsing complete
		m_Command = LKR_CMD_COMPLETE;
		return;
	}

    while (*pStr == ' ' || *pStr == '\t')
        pStr++;
    if (*pStr == ';')
        return;

	// Parse command from the rest of the line
	token = strtok(pStr, " \t\n\r");
	arg = strtok(NULL,"\n\r;");

	if (token == NULL)
		return;

	cmd = token;
	cmd.MakeLower();
	if (arg == NULL)
	{
		if (cmd != "#else" && cmd != "#endif")
		{
			err.Format("Error in line %d(%s):  Expecting argument to %s", lineNo,
				(const char *) m_LinkerScript, token);
			m_Errors.Add(err);
			return;
		}
	}
    else
        while (*arg == ' ' || *arg == '\t')
            arg++;
	sArg = arg;
	sArg.Trim();

	// Test for #ifdef or #ifndef
	pCmd = new CScriptCmd;
	if (cmd == "#ifdef" || cmd == "#ifndef")
	{
		if (cmd == "#ifdef")
			pCmd->m_ID = SCR_CMD_IFDEF;
		else
			pCmd->m_ID = SCR_CMD_IFNDEF;
	}

	// Test for "#if"
	else if (cmd == "#if")
	{
		pCmd->m_ID = SCR_CMD_IF;
	}

    // Test for "#else"
    else if (cmd == "#else")
    {
		pCmd->m_ID = SCR_CMD_ELSE;
    }

    // Test for "#elsif"
    else if (cmd == "#elseif" || cmd == "#elsif")
    {
		pCmd->m_ID = SCR_CMD_ELSIF;
    }

    // Test for "#endif"
    else if (cmd == "#endif")
    {
		pCmd->m_ID = SCR_CMD_ENDIF;
    }

	// Test for #define
	else if (cmd == "#define")
	{
        // Validate there is an identifier and an argument
        while (*arg != ' ' && *arg != '\t' && *arg != '\0')
            arg++;
        token = arg;
        while (*token == ' ' || *token == '\t')
            token++;
        if ((*arg != ' ' && *arg != '\t') || *token == '\0')
        {
            // Error, only 1 argument given
			err.Format("Error in line %d(%s):  Expecting argument to %s", lineNo,
				(const char *) m_LinkerScript, (const char *) sArg);
			m_Errors.Add(err);
			delete pCmd;
			return;
        }
		pCmd->m_ID = SCR_CMD_DEFINE;
	}

	// Test for echo
	else if (cmd == "echo" || cmd == ".echo")
	{
		pCmd->m_ID = SCR_CMD_ECHO;
	}

    // Unknown linker script command!
    else
    {
        err.Format("Error in line %d(%s):  Unkown linker script command %s", lineNo,
            (const char *) m_LinkerScript, token);
        m_Errors.Add(err);
		delete pCmd;
        return;
    }

	// Add the command to the script
	pCmd->m_CmdArg = sArg;
	pCmd->m_Line= lineNo;
	pScript->m_Script.Add(pCmd);
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
    CLinkScript     readScript;

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

        if (lineBuf[0] == '#')
        {
            // Process #if / #else / #endif directives at the root level
            ParseSubScript(lineBuf, lineNo, &readScript);
            ProcessScript(&readScript, TRUE);
            continue;
        }

        if (readScript.m_execute == FALSE)
            continue;

		// If we are not in RAW mode, then tokenized.  RAW mode is used
		// by the PRELINK and POSTLINK commands.
		if (!m_LinkScriptRaw)
		{
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
		else
		{
			// Must be a PRELINK or POSTLINK command.  Send RAW strings
			// to the ParseScript routine.
			if (m_Command == LKR_CMD_PRELINK)
            {
				ParseSubScript(lineBuf, lineNo, &m_PrelinkScript);
                if (m_Command  == LKR_CMD_COMPLETE)
                    ProcessScript(&m_PrelinkScript);
            }
			else
				ParseSubScript(lineBuf, lineNo, &m_PostlinkScript);
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
	int					bytes = 0, count, c, size, idx;
	MString				err, sourcefile;
	Elf32_Shdr*			pHdr = &pFileSection->m_ElfHeader;
	Elf32_Sym*			pSym;
	Elf32_Rel*			pRel;
	Elf32_LinkEq		eqent;
    Elf32_Half          namelen;
	CObjSymFile*		pSymFile;
	CLinkerEquation*	pEq;
    char            	eqdata[2048];

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
		pFileSection->m_Line = pHdr->sh_addralign & 0xFFFF;
		pFileSection->m_LastLine = pHdr->sh_addralign >> 16;
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

		/* Read in the Linker Equations */
	case SHT_LINK_EQ:
		size = pHdr->sh_size;

		// Read the source file name
		bytes = fread(&namelen, 1, sizeof(namelen), fd);
		bytes = fread(eqdata, 1, namelen, fd);
		if (bytes != namelen)
		{
			err.Format("%sUnable to read equation data for %s",
				gsEdl, (const char *) pObjFile->m_Name);
			m_Errors.Add(err);
			return FALSE;
		}
		sourcefile = eqdata;

		while (size)
		{
			pEq = new CLinkerEquation;
			pEq->m_pRpnEq = new CRpnEquation;

			// Read equation header
			bytes = fread(&eqent, 1, sizeof(eqent), fd);

			// Read equation data from the file
			bytes = fread(eqdata, 1, eqent.st_len - sizeof(eqent), fd);
			if (bytes != eqent.st_len - sizeof(eqent))
			{
				err.Format("%sUnable to read equation data for %s",
					gsEdl, (const char *) pObjFile->m_Name);
				m_Errors.Add(err);
				if (pEq != NULL)
					delete pEq;
				return FALSE;
			}

			// Save the index of the relative segment
			pEq->m_Segment = eqent.st_info;
			pEq->m_Address = eqent.st_addr;
			pEq->m_Size = (unsigned char) eqent.st_size;
			pEq->m_Line = eqent.st_line;
            pEq->m_Sourcefile = sourcefile;

			// Build the equation from the data
			count = eqent.st_num;
			idx = 0;
			for (c = 0; c < count; c++)
			{
				int operation = eqdata[idx];
				idx++;

				switch (operation)
				{
				case RPN_VALUE:
					pEq->m_pRpnEq->Add(atof(&eqdata[idx]));
					idx += strlen(&eqdata[idx])+1;
					break;

				case RPN_VARIABLE:
					pEq->m_pRpnEq->Add(RPN_VARIABLE, (const char *) &eqdata[idx]);
					idx += strlen(&eqdata[idx])+1;
					break;

				default:
					pEq->m_pRpnEq->Add(operation);
					break;
				}
				
			}

			// Subtract equation size from total section size
			size -= eqent.st_len;

			// Add the equation to the array
			pFileSection->m_Equations.Add((VTObject *) pEq);
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

            if (pFileSection->m_ElfHeader.sh_type == SHT_PROGBITS)
                m_ProgSections.Add(pFileSection);
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
			(locateAddr + segSize-1 > pAddrRange->endAddr))
		{
			err.Format("%sNo link range found for ASEG %s (addr=%d, size=%d)", gsEdl, 
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
	pFileSection->m_pLinkRgn = pLinkRgn;

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
This routine finds the file section referenced by the relocation object
pRel in th especified object file.
============================================================================
*/
CObjFileSection* VTLinker::FindRelSection(CObjFile* pObjFile, Elf32_Rel* pRel)
{
	int					sectCount, sect, type;
	CObjFileSection*	pFileSect;
	CObjFileSection*	pRelSect;

	pRelSect = NULL;
	sectCount = pObjFile->m_FileSections.GetSize();
	for (sect = 0; sect < sectCount; sect++)
	{
		// Loop through relocation segments
		pFileSect = (CObjFileSection *) pObjFile->m_FileSections[sect];
		if (pFileSect->m_ElfHeader.sh_offset <= pRel->r_offset)
		{
			// Validate the section type
            type = ELF32_R_TYPE(pRel->r_info); 
			if ((type == SR_ADDR_XLATE || type == SR_8BIT || type == SR_24BIT) &&
				(pFileSect->m_ElfHeader.sh_type != SHT_PROGBITS))
					continue;

			if (pFileSect->m_ElfHeader.sh_offset + pFileSect->m_ElfHeader.sh_size <= pRel->r_offset)
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
	int					sect, sectCount, rel, relCount, type;
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
				type = ELF32_R_TYPE(pRel->r_info);
				if (type == SR_ADDR_XLATE || type == SR_8BIT || type == SR_24BIT)
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
						addr += pRefSect->m_LocateAddr;

						// Update the translated address in the ProgBits
						pRelSect->m_pProgBytes[offset] = addr & 0xFF;
						if (type != SR_8BIT)
							pRelSect->m_pProgBytes[offset + 1] = addr >> 8;

						// Save the relative update info for listing back annotation
                        offset += pRelSect->m_LocateAddr;
                        m_AddrLines.a[offset].pObjFile = pObjFile;
                        m_AddrLines.a[offset].line = 0; 
                        m_AddrLines.a[offset].fdPos = -1;
                        m_AddrLines.a[offset].value = addr & 0xFF;
						if (type != SR_8BIT)
						{
							m_AddrLines.a[offset+1].pObjFile = pObjFile;
							m_AddrLines.a[offset+1].line = 0; 
							m_AddrLines.a[offset+1].fdPos = -1;
							m_AddrLines.a[offset+1].value = addr >> 8;
						}

						// Change the relocation type so we know we have translated this item
						pRel->r_info = ELF32_R_INFO(0, SR_ADDR_PROCESSED);
						pRel->r_offset = offset;
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
*/ int VTLinker::ResolveExterns()
{
	POSITION			pos;
	CObjFile*			pObjFile;
	CObjFileSection*	pFileSect;
	int					sect, sectCount, rel, relCount, type;
	MString				err, filename;
	VTObject*			dummy;

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
				type = ELF32_R_TYPE(pRel->r_info);

				// Test if this is an EXTERN relocation type
				if (type == SR_EXTERN || type == SR_EXTERN8)
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
							CObjFileSection* pSymSect = (CObjFileSection *) 
								pSymFile->m_pObjFile->m_FileSections[pSymFile->m_pSym->st_shndx];

							// Calculate the value of the EXTERN symbol
							unsigned short value = (unsigned short) pSymFile->m_pSym->st_value;
							if (pSymSect->m_Type != ASEG && ((pSymFile->m_pSym->st_info & 0x0F)
										!= STT_EQUATE))
								value +=(unsigned short) pSymSect->m_LocateAddr;

							// Calculate the address where we update with the symbol's value
							if (pRel->r_offset + 1 >= (unsigned short) pObjSect->m_Size)
							{
								err.Format("Invalid relocation offset for section %s, symbol = %s, "
									"offset = %d, size = %d\n",
									(const char *) pObjSect->m_Name, (const char *) pSymName, 
									pRel->r_offset, pObjSect->m_Size);
								m_Errors.Add(err);
							}
							else
							{
								char * pAddr = pObjSect->m_pProgBytes + pRel->r_offset;

								// Update the code
								*pAddr++ = value & 0xFF;
								if (type != SR_EXTERN8)
									*pAddr = value >> 8;

								int offset = pRel->r_offset + pObjSect->m_LocateAddr;

								// Save the relative update info for listing back annotation
								m_AddrLines.a[offset].pObjFile = pObjFile;
								m_AddrLines.a[offset].line = 0; 
								m_AddrLines.a[offset].fdPos = -1;
								m_AddrLines.a[offset].value = value & 0xFF;
								if (type != SR_EXTERN8)
								{
									m_AddrLines.a[offset+1].pObjFile = pObjFile;
									m_AddrLines.a[offset+1].line = 0; 
									m_AddrLines.a[offset+1].fdPos = -1;
									m_AddrLines.a[offset+1].value = value >> 8;
								}

								// Update the relocation information
								pRel->r_info = ELF32_R_INFO(0, SR_ADDR_PROCESSED);
								pRel->r_offset += pObjSect->m_LocateAddr;
							}
						}
						else
						{
							if (!m_UndefSymbols.Lookup(pObjFile->m_Name, dummy))
							{
								MString title = MakeTitle(pObjFile->m_Name);
								err.Format("%sUnresolved symbol \"%s\" referenced in %s", 
									gsEdl, pSymName, (const char *) title);
								m_Errors.Add(err);
								m_UndefSymbols[pObjFile->m_Name] = 0;
							}
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
This function evaluates equations and attempts to determine a numeric value
for the equation.  If a numeric value can be achieved, it updates the value
parameter and returns TRUE.  If it cannot achieve a numeric value, the 
function returns FALSE, and creates an error report in design if the
reportError flag is TRUE.
============================================================================
*/
int VTLinker::Evaluate(class CRpnEquation* eq, double* value,  
	int reportError, MString& errVariable, MString& filename)
{
	double				s1, s2;
	MString				errMsg, temp;
	double				stack[200];
	int					stk = 0;
	const char*			pStr;
	int					c, local;
	VTObject*			dummy;

	// Get count of number of operations in equation and initalize stack
	int count = eq->m_OperationArray.GetSize();

	for (c = 0; c < count; c++)
	{
		CRpnOperation& 		op = eq->m_OperationArray[c];
		switch (op.m_Operation)
		{
		case RPN_VALUE:
			stack[stk++] = op.m_Value;
			break;

		case RPN_VARIABLE:
			// Try to find variable in equate array
			temp = op.m_Variable;
			local = 0;

			// Lookup symbol in active module
			pStr = (const char *) temp;
			CObjSymFile* pSymFile;
			if (!m_Symbols.Lookup(pStr, (VTObject *&) pSymFile))
				pSymFile = NULL;

			// If symbol was found, try to evaluate it
			if (pSymFile != (CObjSymFile*) NULL)
			{
				// First get a pointer to the section referenced by the symbol
				CObjFileSection* pSymSect = (CObjFileSection *) pSymFile->m_pObjFile->m_FileSections[pSymFile->m_pSym->st_shndx];

				// Get the symbol value
				unsigned short value = (unsigned short) pSymFile->m_pSym->st_value;
				if (pSymSect->m_Type != ASEG && ELF32_ST_TYPE(pSymFile->m_pSym->st_info)
                            != STT_EQUATE)
					value +=(unsigned short) pSymSect->m_LocateAddr;

				// Add the value to the stack
				stack[stk++] = value;
				break;
			}
			else
			{
				if (reportError)
				{
					// Check if thie symbol has already been declared undefined
					if (!m_UndefSymbols.Lookup(op.m_Variable, dummy))
					{
						errMsg.Format("%sin line %d(%s): Unresolved symbol %s", gsEdl, eq->m_Line, 
								(const char *) filename, pStr);
						m_Errors.Add(errMsg);
						m_UndefSymbols[op.m_Variable] = 0;
					}
				}
				else 
					errVariable = op.m_Variable;
				return 0;
			}
			break;

		case RPN_MACRO:
			// Need to add code here to process macros
			break;

		case RPN_MULTIPLY:
			s2 = stack[--stk];
			s1 = stack[--stk];
			stack[stk++] = s2 * s1;
			break;

		case RPN_DIVIDE:
			s2 = stack[--stk];
			s1 = stack[--stk];
			if (s2 == 0.0)
			{
				// Divide by zero error
				if (reportError)
				{
					errMsg.Format("Error in line %d(%s):  Divide by zero!", 
						eq->m_Line, (const char *) filename);
					m_Errors.Add(errMsg);
				}
				return 0;
			}
			stack[stk++] = s1 / s2;
			break;

		case RPN_ADD:
			s2 = stack[--stk];
			s1 = stack[--stk];
			stack[stk++] = s1 + s2;
			break;

		case RPN_SUBTRACT:
			s2 = stack[--stk];
			s1 = stack[--stk];
			stack[stk++] = s1 - s2;
			break;

		case RPN_EXPONENT:
			s2 = stack[--stk];
			s1 = stack[--stk];
			stack[stk++] = pow(s1, s2);
			break;

		case RPN_MODULUS:
			s2 = stack[--stk];
			s1 = stack[--stk];
			stack[stk++] = (double) ((int) s1 % (int) s2);
			break;

		case RPN_BITOR:
			s2 = stack[--stk];
			s1 = stack[--stk];
			stack[stk++] = (double) ((int) s2 | (int) s1);
			break;

		case RPN_BITAND:
			s2 = stack[--stk];
			s1 = stack[--stk];
			stack[stk++] = (double) ((int) s2 & (int) s1);
			break;

		case RPN_NEGATE:
			s1 = stack[--stk];
			stack[stk++] = -s1;
			break;

		case RPN_BITXOR:
			s2 = stack[--stk];
			s1 = stack[--stk];
			stack[stk++] = (double) ((int) s1 ^ (int) s2);
			break;

		case RPN_LEFTSHIFT:
			s2 = stack[--stk];
			s1 = stack[--stk];
			stack[stk++] = (double) ((int) s1 << (int) s2);
			break;

		case RPN_RIGHTSHIFT:
			s2 = stack[--stk];
			s1 = stack[--stk];
			stack[stk++] = (double) ((int) s1 >> (int) s2);
			break;

		case RPN_FLOOR:
			s1 = stack[--stk];
			stack[stk++] = floor(s1);
			break;

		case RPN_CEIL:
			s1 = stack[--stk];
			stack[stk++] = ceil(s1);
			break;

		case RPN_IP:
			s1 = stack[--stk];
			stack[stk++] = floor(s1);
			break;

		case RPN_FP:
			s1 = stack[--stk];
			stack[stk++] = s1 - floor(s1);
			break;

		case RPN_LN:
			s1 = stack[--stk];
			stack[stk++] = log(s1);
			break;

		case RPN_PAGE:
			s1 = stack[--stk];
			stack[stk++] = ((unsigned int) s1 >> 16) & 0xFF;
			break;

		case RPN_HIGH:
			s1 = stack[--stk];
			stack[stk++] = ((unsigned int) s1 >> 8) & 0xFF;
			break;

		case RPN_LOW:
			s1 = stack[--stk];
			stack[stk++] = (unsigned int) s1 & 0xFF;
			break;

		case RPN_LOG:
			s1 = stack[--stk];
			stack[stk++] = log10(s1);
			break;

		case RPN_SQRT:
			s1 = stack[--stk];
			stack[stk++] = sqrt(s1);
			break;

#if 0
		case RPN_DEFINED:
			if (LookupSymbol(op.m_Variable, symbol))
				stack[stk++] = 1.0;
			else
				stack[stk++] = 0.0;
			break;
#endif

		case RPN_NOT:
			s1 = stack[--stk];
			if (s1 == 0.0)
				stack[stk++] = 1.0;
			else
				stack[stk++] = 0.0;
			break;

		case RPN_BITNOT:
			s1 = stack[--stk];
			stack[stk++] = (double) (~((int) s1));
			break;

		}
		errVariable = "";
	}

	// Get the result from the equation and return to calling function
	*value = stack[--stk];

	return 1;
}

/*
============================================================================
Now resolved all equations.
============================================================================
*/
int VTLinker::ResolveEquations()
{
	POSITION			pos;
	CObjFile*			pObjFile;
	CObjFileSection*	pFileSect;
	int					sect, sectCount, c, eqCount;
	MString				err, errVar, filename;
    double              value;

	// Exit if error has occurred
	if (m_Errors.GetSize() != 0)
		return FALSE;

	// Loop for all object files loaded
	pos = m_ObjFiles.GetStartPosition();
	while (pos != NULL)
	{
		// Get pointer to this object file's data
		m_ObjFiles.GetNextAssoc(pos, filename, (VTObject *&) pObjFile);

		// Loop through all segments and look for equation info
		sectCount = pObjFile->m_FileSections.GetSize();
		for (sect = 0; sect < sectCount; sect++)
		{
			// Loop through equation segments
			pFileSect = (CObjFileSection *) pObjFile->m_FileSections[sect];

			if (pFileSect->m_Type != SHT_LINK_EQ)
				continue;
			
			eqCount = pFileSect->m_Equations.GetSize();
			for (c = 0; c < eqCount; c++)
			{
				// Process this equation item
				CLinkerEquation* pEq = (CLinkerEquation *) pFileSect->m_Equations[c];
                CObjFileSection* pObjSect = (CObjFileSection *) pObjFile->m_FileSections[pEq->m_Segment];

				// Try to evaluate the equation
				if (Evaluate(pEq->m_pRpnEq, &value, 0, errVar, pObjSect->m_Name))
				{
					// Calculate the address where we update with the symbol's value
					if (pEq->m_Address + 1 >= (unsigned short) pObjSect->m_Size)
					{
						err.Format("Invalid relocation offset for section %s, offset = %d, size = %d\n",
							(const char *) pObjSect->m_Name, pEq->m_Address, pObjSect->m_Size);
						m_Errors.Add(err);
					}
					else
					{
						char * pAddr = pObjSect->m_pProgBytes + pEq->m_Address;

						// Update the code
						*pAddr++ = (unsigned int) value & 0xFF;
						*pAddr = (unsigned int) value >> 8;

						int offset = pObjSect->m_LocateAddr + pEq->m_Address;

						// Save the relative update info for listing back annotation
						m_AddrLines.a[offset].pObjFile = pObjFile;
						m_AddrLines.a[offset].line = 0; 
						m_AddrLines.a[offset].fdPos = -1;
						m_AddrLines.a[offset].value = (int) value & 0xFF;
						m_AddrLines.a[offset+1].pObjFile = pObjFile;
						m_AddrLines.a[offset+1].line = 0; 
						m_AddrLines.a[offset+1].fdPos = -1;
						m_AddrLines.a[offset+1].value = (int) value >> 8;
					}
				}
				else
				{
					MString title = MakeTitle(pObjFile->m_Name);
					err.Format("Error in line %d(%s): Unresolved symbol %s", pEq->m_Line, 
                            (const char *) pEq->m_Sourcefile, (const char *) errVar);
					m_Errors.Add(err);
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
	int					c, size, index;
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
        unsigned char  fillchar = 0;

        if (m_LinkOptions.Find((char *) "-f") != -1)
            fillchar = 0xFF;

		// Fill contents with zero
		for (c = 0; c < 32768; c++)
			pRom[c] = fillchar;

        size = 32768;
        if ((index = m_LinkOptions.Find((char *) "-b")) != -1)
        {
            size = m_LinkOptions.ToInt(index+2);
        }

		// Copy the output data to the ROM storage
		for (c = 0; c < size; )
		{
			// Skip empty space in the link map
			while (m_SegMap[c] == NULL && c < size)
				c++;

			// Write the next block of bytes
			if (c < size)
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
		save_hex_file_buf(pRom, 0, size-1, 0, fd);

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
		fwrite(pRom, 1, size, fd);
		fclose(fd);

		// Delete the ROM memory
		delete pRom;
	}

	// We don't support Library output yet, but soon...
	else if (m_ProjectType == VT_PROJ_TYPE_LIB)
	{
	}

    m_LinkDone = TRUE;
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
	int			c, count, addr;
	int			x;
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

		int sectionname = FALSE;
		for (x = 0; x < m_ProgSections.GetSize(); x++)
		{
			if (((CObjFileSection *) m_ProgSections[x])->m_Name == pObjSymFile->m_pName)
			{
				sectionname = TRUE;
				break;
			}
		}
		if (sectionname)
			continue;

		// Test if this symbol is a section name

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
        CFileString fstr;

		pObjSymFile = (CObjSymFile *) m_Sorted[c];

		CObjFileSection* pSymSect = (CObjFileSection *) pObjSymFile->m_pObjFile->m_FileSections[
			pObjSymFile->m_pSym->st_shndx];

		addr = (int) pObjSymFile->m_pSym->st_value;
		if (pSymSect->m_Type != ASEG && ELF32_ST_TYPE(pObjSymFile->m_pSym->st_info) != STT_EQUATE)
		   addr += (int) pSymSect->m_LocateAddr; 

        fstr = pObjSymFile->m_pObjFile->m_Name;
		fprintf(fd, "%35s   0x%04x  %9s  %s\n", pObjSymFile->m_pName, addr, 
			ELF32_ST_TYPE(pObjSymFile->m_pSym->st_info) == STT_OBJECT ? (char *) "data" : (char *) "function", 
			(const char *) fstr.Filename());
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

		int sectionname = FALSE;
		for (x = 0; x < m_ProgSections.GetSize(); x++)
		{
			if (((CObjFileSection *) m_ProgSections[x])->m_Name == pObjSymFile->m_pName)
			{
				sectionname = TRUE;
				break;
			}
		}
		if (sectionname)
			continue;

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
        CFileString fstr;

		pObjSymFile = (CObjSymFile *) m_Sorted[c];
		CObjFileSection* pSymSect = (CObjFileSection *) pObjSymFile->m_pObjFile->m_FileSections[
			pObjSymFile->m_pSym->st_shndx];

		addr = (int) pObjSymFile->m_pSym->st_value;
		if (pSymSect->m_Type != ASEG && ELF32_ST_TYPE(pObjSymFile->m_pSym->st_info) != STT_EQUATE)
		   addr += (int) pSymSect->m_LocateAddr; 

        fstr = pObjSymFile->m_pObjFile->m_Name;
		fprintf(fd, "%35s   0x%04x  %9s  %s\n", pObjSymFile->m_pName, addr,
			ELF32_ST_TYPE(pObjSymFile->m_pSym->st_info) == STT_OBJECT ? (char *) "data" : (char *) "function", 
			(const char *) fstr.Filename());
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
	CObjFile*			pOpenFile;
	CObjFileSection*	pFileSection;
	MString				err, filename;
	FILE*				fd;
	char*				pRead;
	char				lineBuf[512];
	char				str[6];
	int					fdpos, fdpos2, c, x, eq, count, line;
	int					addr, rel, relCount;

	// Exit if error has occurred
	if (m_Errors.GetSize() != 0)
		return FALSE;

	// Test if list file generation requested
	//if (m_LinkOptions.Find((char *) "-t") == -1)
	//	return FALSE;

	// Loop for all object files loaded and update the address of all CSEG and DSEG
    // segments
	pos = m_ObjFiles.GetStartPosition(); while (pos != NULL)
	{
		// Get pointer to this object file's data
		m_ObjFiles.GetNextAssoc(pos, filename, (VTObject *&) pObjFile);

		// Get the path of the source file for this object file
		CFileString  lstFile(pObjFile->m_Name);
		lstFile.NewExt(".lst");

		// Try to open the listing file.  If no file, just move on
		if ((fd = fopen((const char *) lstFile.GetString(), "rb+")) == NULL)
			continue;

		// Now loop through all ObjFileSections for this file and update the
		// address for any CSEG or DSEG segments
		count = pObjFile->m_FileSections.GetSize();
		for (c = 0; c < count; c++)
		{
			// Get a pointer to the next file section
			pFileSection = (CObjFileSection *) pObjFile->m_FileSections[c];
            if (pFileSection->m_Type != CSEG && pFileSection->m_Type != DSEG && 
                    pFileSection->m_Type != ASEG)
                continue;

			// Start at beginning of list file and search for the starting line
			fseek(fd, 0, SEEK_SET);
			line = 1;
			fdpos = 0;
			while (line != 0 && line < pFileSection->m_Line)
			{
				// Read next line from listing file
				pRead = fgets(lineBuf, sizeof(lineBuf), fd);
				if (pRead == NULL || feof(fd))
					break;

				// Keep track of the file position
				line++;
			}

			// Test if end of file encoundered	
			if (feof(fd))
				continue;

			// Now loop for all lines in this section and update addresses
			for (; line < pFileSection->m_LastLine; line++)
			{
				// Save the file position so we can rewind for saving new address
				fdpos = ftell(fd);
				
				// Read in the next line
				pRead = fgets(lineBuf, sizeof(lineBuf), fd);
				if (pRead == NULL || feof(fd))
					break;

				// Check if this line has an address at col 1
				if (lineBuf[0] == ' ')
					continue;

				// Okay, we need to add our segment start address to the address
				// (offset) reported in the listing file
				sscanf(lineBuf, "%X", &addr);
				if (pFileSection->m_Type != ASEG)
				{
					addr += pFileSection->m_LocateAddr;
					sprintf(lineBuf, "%04X", addr);

					// Seek back, write the new addr and then restore
					fdpos2 = ftell(fd);
					fseek(fd, fdpos, SEEK_SET);
					fwrite(lineBuf, 1, 4, fd);
					fseek(fd, fdpos2, SEEK_SET);
				}

                // Add this address to our AddrLines array
				m_AddrLines.a[addr].line = line;
				m_AddrLines.a[addr].fdPos = fdpos+6;

				// Check if this line has multiple bytes on it and add all AddrLines
				for (x = 1; x < 4; x++)
				{
					if (lineBuf[6+x*3] != ' ')
					{
						m_AddrLines.a[addr+x].line = line;
						m_AddrLines.a[addr+x].fdPos = fdpos+6+x*3;
					}
                    else
                        break;
				}
			}
		}

		// Close the listing file
		fclose(fd);
	}

	// Now loop through all relocate items and replace the offsets with actual addresses
	// Loop for all object files loaded
	pos = m_ObjFiles.GetStartPosition();
    pOpenFile = NULL;
	while (pos != NULL)
	{
		// Get pointer to this object file's data
		m_ObjFiles.GetNextAssoc(pos, filename, (VTObject *&) pObjFile);

		// Loop through all segments and look for relocation info
		count = pObjFile->m_FileSections.GetSize();
		for (c = 0; c < count; c++)
		{
			// Loop through relocation segments
			pFileSection = (CObjFileSection *) pObjFile->m_FileSections[c];
			relCount = pFileSection->m_Reloc.GetSize();
			for (rel = 0; rel < relCount; rel++)
			{
				// Process this relocation item
				Elf32_Rel* pRel = (Elf32_Rel *) pFileSection->m_Reloc[rel];
				if (ELF32_R_TYPE(pRel->r_info) == SR_ADDR_PROCESSED)
				{
					// Lookup this address in the AddrLines array
					if (pRel->r_offset > 0 && pRel->r_offset < 65536)
					{
						if (m_AddrLines.a[pRel->r_offset].pObjFile != NULL)
						{
							// Found!  Now go update the data
							if (pOpenFile != NULL && pOpenFile != pObjFile)
							{
								// Close the file so we can open a new one
								fclose(fd);
								pOpenFile = NULL;
							}

							// Get the path of the source file for this object file
							CFileString  lstFile(m_AddrLines.a[pRel->r_offset].pObjFile->m_Name);
							lstFile.NewExt(".lst");

							// Try to open the listing file.  If no file, just move on
							if (pOpenFile == NULL)
							{
								if ((fd = fopen((const char *) lstFile.GetString(), "rb+")) == NULL)
									continue;

								// Mark the file as opened
								pOpenFile = pObjFile;
							}

							// Seek to the location in the lst file for LSB
							fseek(fd, m_AddrLines.a[pRel->r_offset].fdPos, SEEK_SET);
							sprintf(str, "%02X", m_AddrLines.a[pRel->r_offset].value);
							fwrite(str, 1, 2, fd);

							if (m_AddrLines.a[pRel->r_offset+1].pObjFile != NULL)
							{
								// Seek to the location in the lst file for MSB
								fseek(fd, m_AddrLines.a[pRel->r_offset+1].fdPos, SEEK_SET);
								sprintf(str, "%02X", m_AddrLines.a[pRel->r_offset+1].value);
								fwrite(str, 1, 2, fd);
							}
						}
					}
				}
			}

			// Now loop for all equations in this section
			if (pFileSection->m_Type != SHT_LINK_EQ)
				continue;
			
			int eqCount = pFileSection->m_Equations.GetSize();
			for (eq = 0; eq < eqCount; eq++)
			{
				// Process this equation item
				CLinkerEquation* pEq = (CLinkerEquation *) pFileSection->m_Equations[eq];
                CObjFileSection* pObjSect = (CObjFileSection *) pObjFile->m_FileSections[pEq->m_Segment];

				int offset = pObjSect->m_LocateAddr + pEq->m_Address;
				if (m_AddrLines.a[offset].pObjFile != NULL)
				{
					// Found!  Now go update the data
					if (pOpenFile != NULL && pOpenFile != pObjFile)
					{
						// Close the file so we can open a new one
						fclose(fd);
						pOpenFile = NULL;
					}

					// Get the path of the source file for this object file
					CFileString  lstFile(m_AddrLines.a[offset].pObjFile->m_Name);
					lstFile.NewExt(".lst");

					// Try to open the listing file.  If no file, just move on
					if (pOpenFile == NULL)
					{
						if ((fd = fopen((const char *) lstFile.GetString(), "rb+")) == NULL)
							continue;

						// Mark the file as opened
						pOpenFile = pObjFile;
					}

					// Seek to the location in the lst file for LSB
					fseek(fd, m_AddrLines.a[offset].fdPos, SEEK_SET);
					sprintf(str, "%02X", m_AddrLines.a[offset].value);
					fwrite(str, 1, 2, fd);

					if (m_AddrLines.a[offset+1].pObjFile != NULL)
					{
						// Seek to the location in the lst file for MSB
						fseek(fd, m_AddrLines.a[offset+1].fdPos, SEEK_SET);
						sprintf(str, "%02X", m_AddrLines.a[offset+1].value);
						fwrite(str, 1, 2, fd);
					}
				}
			}
		}
	}

	if (pOpenFile != NULL)
		fclose(fd);

	return TRUE;
}

/*
============================================================================
This function is called to parse an input file.  If there are no errors
during parsing, the routine calls the routines to assemlbe and generate
output files for .obj, .lst, .hex, etc.
============================================================================
*/
void VTLinker::ParseExternalDefines(void)
{
	int			startIndex, endIndex;
	MString		def, sval;
	int			valIdx, len;
	int			value = -1;

	// If zero length then we're done
	if ((len = m_ExtDefines.GetLength()) == 0)
		return;

	// Convert any commas to semicolons to make searches easy
	m_ExtDefines.Replace(',', ';');

	// Loop for all items in the string and assign labels
	startIndex = 0;
	while (startIndex >= 0)
	{
		// Find end of next define
		endIndex = m_ExtDefines.Find(';', startIndex);
		if (endIndex < 0)
			endIndex = len;
		
		// Extract the define from the string
		def = m_ExtDefines.Mid(startIndex, endIndex - startIndex);

        sval = "";

		// Test if the define has an embedded '='
		if ((valIdx = def.Find('=')) != -1)
		{
			MString name = def.Left(valIdx);
			sval = def.Mid(valIdx+1, endIndex - (valIdx+1));

			// The value was valid.  Replace the def name
			def = name;
		}

        // Add this define / string value pair
        CDefine  *pDefine = new CDefine;
        if (pDefine != NULL)
        {
            pDefine->m_Name = def;
            pDefine->m_Value = sval;
            m_Defines[def] = pDefine;
        }

		// Update startIndex
		startIndex = endIndex + 1;
		if (startIndex >= len)
			startIndex = -1;
		while (startIndex > 0)
		{
			if (m_ExtDefines[startIndex] == ' ')
			{
				if (++startIndex >= len)
					startIndex = -1;
			}
			else
				break;
		}
	}
}

/*
============================================================================
Process any POSTLINK commands from the Linker Script.
============================================================================
*/
void VTLinker::ProcessPostlink(void)
{
    // Validate link done and no errors
    if (!m_LinkDone || m_Errors.GetSize() > 0)
        return;

    // Process the postlink script from the linker script file
    ProcessScript(&m_PostlinkScript);
}

/*
============================================================================
The IDE calls this function to perform the link operation after all
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

    // Add any external defines from the IDE
    ParseExternalDefines();

	// Load all files to be linked
	ReadObjFiles();

	// Read the linker script
	ReadLinkerScript();
	
	// Locate segments based on commands in the Linker Script
	LocateSegments();

	// Resolve static (local) symbols for each segment
	ResolveLocals();

	// Resolve segment extern symbols
	ResolveExterns();

	// Resolve reloatable equations
	ResolveEquations();

	// Generate output file
	GenerateOutputFile();

	// Generate debug information

	// Generate map file
	GenerateMapFile();

	// Back annotate Listing files with actual addresses
	BackAnnotateListingFiles();

    // Process Linker Script POSTLINK commands
    ProcessPostlink();

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

	// Parse the options later during linking
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

	// Parse the options later during linking
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
Sets the Linker external defines
============================================================================
*/
void VTLinker::SetDefines(const MString& defines)
{
	// Save the project type
	m_ExtDefines = defines;
}

