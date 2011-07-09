/*
 * $Id: assemble.cpp,v 1.5 2011/07/09 08:16:21 kpettit1 Exp $
 *
 * Copyright 2010 Ken Pettit
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

 
 VTAssembler:		This class defines an 8085 Macro Assembler for the
					VirtualT project.

========================================================================
*/

#include		"VirtualT.h"
#include		"assemble.h"
#include		"elf.h"
#include		"project.h"
#include		<FL/Fl.H>
#include		<math.h>
#include		<string.h>
#include		<stdio.h>
#include		<stdlib.h>
#ifndef WIN32
#include 		<unistd.h>
#endif

extern "C"
{
#include		"intelhex.h"
}

extern	CRpnEquation*	gEq;
extern	VTObArray*		gExpList;
extern	MStringArray*	gNameList;
extern	CCondition*		gCond;
extern	CMacro*			gMacro;
extern	char			reg[10];
extern	int				reg_cnt;
extern	const char *	gFilename;
extern	int				gDefine;

#define	PCB	a85parse_pcb

extern	int ParseASMFile(const char *filename, class VTAssembler *pAsm);

unsigned char gOpcodeType[INST_ORG] = { 0,
	OPCODE_1REG,  OPCODE_REG_EQU16,  OPCODE_1REG,  OPCODE_1REG,		// 1-4
	OPCODE_1REG,  OPCODE_1REG,  OPCODE_1REG,  OPCODE_REG_IMM,	// 5-8
	OPCODE_1REG,  OPCODE_1REG,  OPCODE_1REG,  OPCODE_1REG,		// 9-12
	OPCODE_1REG,  OPCODE_1REG,  OPCODE_1REG,  OPCODE_1REG,		// 13-16
	OPCODE_1REG,  OPCODE_1REG,  OPCODE_1REG,  OPCODE_NOARG,		// 17-20
	OPCODE_EQU8,  OPCODE_EQU16, OPCODE_NOARG, OPCODE_NOARG,		// 21-24
	OPCODE_EQU16, OPCODE_NOARG, OPCODE_EQU8,  OPCODE_EQU16,		// 25-28
	OPCODE_NOARG, OPCODE_2REG,  OPCODE_NOARG, OPCODE_NOARG,		// 29-32
	OPCODE_NOARG, OPCODE_NOARG, OPCODE_NOARG, OPCODE_NOARG,		// 33-36
	OPCODE_NOARG, OPCODE_NOARG, OPCODE_NOARG, OPCODE_NOARG,		// 37-40
	OPCODE_NOARG, OPCODE_NOARG, OPCODE_NOARG, OPCODE_NOARG,		// 41-44
	OPCODE_NOARG, OPCODE_NOARG, OPCODE_NOARG, OPCODE_NOARG,		// 45-48
	OPCODE_NOARG, OPCODE_NOARG, OPCODE_EQU16, OPCODE_EQU16,		// 49-52
	OPCODE_EQU16, OPCODE_EQU16, OPCODE_EQU16, OPCODE_EQU16,		// 53-56
	OPCODE_EQU16, OPCODE_EQU16, OPCODE_EQU8,  OPCODE_EQU8,		// 57-60
	OPCODE_NOARG, OPCODE_NOARG, OPCODE_NOARG, OPCODE_NOARG,		// 61-64
	OPCODE_EQU16, OPCODE_EQU16, OPCODE_EQU16, OPCODE_EQU16,		// 55-68
	OPCODE_EQU16, OPCODE_EQU16, OPCODE_EQU16, OPCODE_EQU16,		// 69-72
	OPCODE_EQU16, OPCODE_EQU16, OPCODE_EQU16, OPCODE_NOARG,		// 73-76
	OPCODE_EQU16, OPCODE_EQU8,  OPCODE_EQU8,  OPCODE_EQU8,		// 77-80
	OPCODE_EQU8,  OPCODE_EQU8,  OPCODE_EQU8,  OPCODE_EQU8,		// 81-84
	OPCODE_EQU8,  OPCODE_IMM,   OPCODE_EQU16, OPCODE_NOARG,		// 85-88
	OPCODE_NOARG, OPCODE_NOARG												// 89
};

unsigned char gOpcodeBase[INST_ORG] = { 0,
	0x0A, 0x01, 0x02, 0x03, 0x0B, 0x04, 0x05, 0x06,				// 1-8
	0x80, 0x88, 0x90, 0x98, 0xA0, 0xA8, 0xB0, 0xB8,				// 2-16
	0x09, 0xC1, 0xC5, 0x27, 0x28, 0x2A, 0x2F, 0x30,				// 17-24
	0x32, 0x37, 0x38, 0x3A, 0x3F, 0x40, 0x07, 0x08,				// 25-32
	0x1F, 0x00, 0x0F, 0x10, 0x17, 0x18, 0xC0, 0xC8,				// 33-40
	0xD0, 0xD8, 0xE0, 0xE8, 0xF0, 0xF8, 0x20, 0xD9,				// 41-48
	0xE9, 0xF9, 0xC2, 0xCA, 0xD2, 0xDA, 0xE2, 0xEA,				// 49-56
	0xF2, 0xFA, 0xD3, 0xDB, 0xE3, 0xEB, 0xF3, 0xFB,				// 57-64
	0xC4, 0xCC, 0xD4, 0xDC, 0xE4, 0xEC, 0xF4, 0xFC,				// 65-72
	0x22, 0xCD, 0xDD, 0xED, 0xFD, 0xC6, 0xCE, 0xD6,				// 73-80
	0xDE, 0xE6, 0xEE, 0xF6, 0xFE, 0xC7, 0xC3, 0xC9,				// 81-88
	0x76, 0xCB													// 89
};

unsigned char gShift[20] = { 0,
	4, 4, 4, 4, 4, 3, 3, 3,										// 1-8
	0, 0, 0, 0, 0, 0, 0, 0,										// 9-16
	4, 4, 4
};

const char*		types[5] = { "", "a label", "an equate", "a set", "an extern" };


void cb_assembler(class Fl_Widget* w, void*)
{
	VTAssembler*	pAsm = new VTAssembler;

	int x = sizeof(CInstruction);

	pAsm->Parse("test.asm");

	delete pAsm;
}

VTAssembler::VTAssembler()
{						 
	m_List = 0;
	m_Hex = 0;
	m_IncludeDepth = 0;
	m_FileIndex = -1;
	m_DebugInfo = 0;
	m_LastLabelLine = -1;
	m_LastIfElseLine = -1;
	m_LastIfElseIsIf = 0;
	m_MsbFirst = FALSE;
	m_LocalModuleChar = '_';
	m_ActiveSeg = ASEG;

	// Create a new symbol module
	m_ActiveMod = new CModule("basemod");
	m_Modules["basemod"] = m_ActiveMod;

	// Create a segment to assemble into and a seglines line tracker
	m_ActiveSeg = new CSegment(".aseg", ASEG, m_ActiveMod);
	m_Segments[".aseg"] = m_ActiveSeg;
	m_ActiveSegLines = new CSegLines(m_ActiveSeg, 1);
	m_SegLines.Add(m_ActiveSegLines);

	// Assign the active pointers for module and segment
	m_Symbols = m_ActiveMod->m_Symbols;
	m_Instructions = m_ActiveSeg->m_Instructions;
	m_ActiveAddr = m_ActiveSeg->m_ActiveAddr;
	m_pStdoutFunc = NULL;

	// Initialize IF stack
	m_IfDepth = 0;
	m_IfStat[0] = IF_STAT_ASSEMBLE;
}

VTAssembler::~VTAssembler()
{
	ResetContent();

	delete m_ActiveMod;
	delete m_ActiveSeg;
	delete m_ActiveSegLines;
}

void VTAssembler::SetStdoutFunction(void *pContext, stdOutFunc_t pFunc)
{
	m_pStdoutFunc = pFunc;
	m_pStdoutContext = pContext;
}

void VTAssembler::ResetContent(void)
{
	MString		key, modName;
	POSITION	modPos;
	int			c, count;
	CModule*	pMod;
	CSegment*	pSeg;
	CSegLines*	pSegLines;
	CSymbol*	pSym;

	// Loop through each module and delete them
	modPos = m_Modules.GetStartPosition();
	while (modPos != NULL)
	{
		// Get the next module
		m_Modules.GetNextAssoc(modPos, modName, (VTObject *&) pMod);
		delete pMod;
	}
	m_Modules.RemoveAll();

	// Loop through each segment
	modPos = m_Segments.GetStartPosition();
	while (modPos != NULL)
	{
		// Get the next module
		m_Segments.GetNextAssoc(modPos, modName, (VTObject *&) pSeg);
		delete pSeg;
	}
	m_Segments.RemoveAll();
	
	// Delete all module line objects
	count = m_SegLines.GetSize();
	for (c = 0; c < count; c++)
	{
		pSegLines = (CSegLines *) m_SegLines[c];
		delete pSegLines;
	}
	m_SegLines.RemoveAll();

	// Delete all Extern reference object
	count = m_Externs.GetSize();
	for (c = 0; c < count; c++)
	{
		CExtern* pExt = (CExtern *) m_Externs[c];
		delete pExt;
	}
	m_Externs.RemoveAll();

	// Delete all #defines
	count = m_Defines.GetSize();
	for (c = 0; c < count; c++)
	{
		CMacro* pMacro = (CMacro *) m_Defines[c];
		delete pMacro;
	}
	m_Defines.RemoveAll();

	// Delete all undefined symbols
	m_UndefSymbols.RemoveAll();

	// Delete all errors
	m_Errors.RemoveAll();
	m_IncludeDirs.RemoveAll();

	// Delete filenames parsed
	m_Filenames.RemoveAll();
	m_FileIndex = -1;

	// Create a new basemod
	m_ActiveMod = new CModule("basemod");
	m_Modules["basemod"] = m_ActiveMod;

	// Create a new segment to assmeble into
	m_ActiveSeg = new CSegment(".aseg", ASEG, m_ActiveMod);
	m_Segments[".aseg"] = m_ActiveSeg;

	// Assign active assembly pointers
	m_Symbols = m_ActiveMod->m_Symbols;
	m_Instructions = m_ActiveSeg->m_Instructions;
	m_ActiveAddr = m_ActiveSeg->m_ActiveAddr;

	// Add the "VTASM" symbol
	pSym = new CSymbol();
	pSym->m_Name = "VTASM";
	pSym->m_Value = 1;
	pSym->m_SymType = SYM_EQUATE | SYM_HASVALUE;
	(*m_Symbols)[(const char *) pSym->m_Name] = pSym; 

	// Create a new SegLines object
	m_ActiveSegLines = new CSegLines(m_ActiveSeg, 1);
	m_SegLines.Add(m_ActiveSegLines);

	m_Hex = 0;
	m_List = 0;
	m_Address = 0;
	m_DebugInfo = 0;
	m_LastLabelAdded = 0;

	// Clean up include stack
	while (m_IncludeDepth != 0)
		fclose(m_IncludeStack[--m_IncludeDepth]);
	for (c = 0; c < 32; c++)
		m_IncludeName[c] = "";

	// Initialize IF stack
	m_IfDepth = 0;
	m_IfStat[0] = IF_STAT_ASSEMBLE;
}

/*
============================================================================
The parser calls this function when it detects a list output pragma.
============================================================================
*/
void VTAssembler::pragma_list()
{
	if (m_IfStat[m_IfDepth] == IF_STAT_ASSEMBLE)
		m_List = 1;
}

/*
============================================================================
The parser calls this function when it detects a hex output pragma.
============================================================================
*/
void VTAssembler::pragma_hex()
{
	if (m_IfStat[m_IfDepth] == IF_STAT_ASSEMBLE)
		m_Hex = 1;
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
int VTAssembler::Evaluate(class CRpnEquation* eq, double* value,  
	int reportError)
{
	double				s1, s2;
	CSymbol*			symbol;
	MString				errMsg, temp;
	double				stack[200];
	int					stk = 0;
	int					int_value;
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
			if (temp.GetLength() > 1)
			{
				// Check if label is local to the last label only
				if (temp[0] == '&')
				{
					// Prepend the last label to make this a local symbol
					temp = m_LastLabel + (char *) "%%" + op.m_Variable;
					local = 1;
				}
				else if (temp[0] == m_LocalModuleChar)
				{
					// Prepend the last label to make this a local symbol
					temp = m_ActiveMod->m_Name + (char *) "%%" + op.m_Variable;
					local = 1;
				}
			}

			// Lookup symbol in active module
			pStr = (const char *) temp;
			if (!m_ActiveMod->m_Symbols->Lookup(pStr, (VTObject *&) symbol))
				symbol = NULL;

			// If symbol not found, look in other modules if not module local
			if ((symbol == NULL) && !local)
			{
				symbol = LookupSymOtherModules(op.m_Variable);
			}

			// If symbol was found, try to evaluate it
			if (symbol != (CSymbol*) NULL)
			{
				// If symbol has no value, try to get it
				if ((symbol->m_SymType & SYM_HASVALUE) == 0)
				{
					if ((symbol->m_SymType & 0xFF) == SYM_EXTERN)
						return 0;

					if (GetValue(symbol->m_Name, int_value))
					{
						stack[stk++] = int_value;
						break;
					}
					else
					{
						if (reportError)
						{
							errMsg.Format("Error in line %d(%s):  Symbol %s has no numeric value", 
								m_Line, (const char *) m_Filenames[m_FileIndex], 
								(const char *) op.m_Variable);
							m_Errors.Add(errMsg);
						}
						return 0;
					}
				}

				// Check if equate has a numeric value associated with it
				if (symbol->m_SymType & SYM_HASVALUE)
				{
					stack[stk++] = symbol->m_Value;
				}
			} 
			else if (temp == "$")
				stack[stk++] = m_Address;
			else	// Genereate error indication
			{
				if (reportError)
				{
					// Check if thie symbol has already been declared undefined
					if (!m_UndefSymbols.Lookup(op.m_Variable, dummy))
					{
						if (local)
						{
							errMsg.Format("Error in line %d(%s):  Local symbol %s undefined", m_Line,
								(const char *) m_Filenames[m_FileIndex], (const char *) op.m_Variable);
						}
						else
						{
							// Check if AuotExtern is enabled
							if (m_AsmOptions.Find((char *) "-e") != -1)
							{
								// Add symbol as an EXTERN
								symbol = new CSymbol;
								symbol->m_Line = m_Line;
								symbol->m_FileIndex = m_FileIndex;			// Save index of the current file
								symbol->m_Name = op.m_Variable;
								symbol->m_SymType = SYM_EXTERN;
								(*m_Symbols)[symbol->m_Name] = symbol;
								return 0;
							}
							errMsg.Format("Error in line %d(%s):  Symbol %s undefined", m_Line,
								(const char *) m_Filenames[m_FileIndex], (const char *) op.m_Variable);
						}
						m_Errors.Add(errMsg);
						m_UndefSymbols[op.m_Variable] = 0;
					}
				}
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
						m_Line, (const char *) m_Filenames[m_FileIndex]);
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

		case RPN_DEFINED:
			if (LookupSymbol(op.m_Variable, symbol))
				stack[stk++] = 1.0;
			else
				stack[stk++] = 0.0;
			break;

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
	}

	// Get the result from the equation and return to calling function
	*value = stack[--stk];

	return 1;
}

/*
============================================================================
Returns the value of the given string.  If the value is numeric, it converts
it to an integer, otherwise it searches the symbol tables for the value.
============================================================================
*/
int VTAssembler::GetValue(MString & string, int & value)
{
	int		c, len;
	int		numericVal = 1;
	double	dValue;

	// Get length of string
	len = string.GetLength();

	// First check if string is a numeric value
	for (c = 0; c < len; c++)
		if ((string[c] > '9') || (string[c] < '0') && string[c] != '-')
		{
			numericVal = 0;
			break;
		}

	if (numericVal)
	{
		value = atoi((const char *) string);
		return 1;
	}

	// Make the string uppercase and search for it in the symbol table
	// For each segment for each module
	CSymbol* pSymbol = NULL;
	MString myStr = string;
	if ((string[0] == '%') && (string[1] != 0))
	{
		myStr = m_LastLabel + (char *) "%%" + string;
	}

	// Lookup the symbol in all modules
	if (!m_ActiveMod->m_Symbols->Lookup(myStr, (VTObject*&) pSymbol))
		pSymbol = LookupSymOtherModules(myStr);

	// If symbol was found, evaluate it
	if (pSymbol != NULL)
	{
		// Check if symbol has been evaluated yet
		if (pSymbol->m_SymType & SYM_HASVALUE)
		{
			value = pSymbol->m_Value;
			return 1;
		}

		// Evaluate the equation and save the value if any.  Report errors.
		if (pSymbol->m_Equation != NULL)
		{
			if (Evaluate(pSymbol->m_Equation, &dValue, 0))
			{
				value = (int) dValue;
				pSymbol->m_Value = (long) dValue;
				pSymbol->m_SymType |= SYM_HASVALUE;
				return 1;
			}
		}
	}

	return 0;
}

/*
===========================================================================================
Add a new instruction to the instruction array with the specified opcode and default
settings for line number, file index, etc.
===========================================================================================
*/
CInstruction* VTAssembler::AddInstruction(int opcode)
{
	CInstruction	*pInst;

	// Update line number
	m_Line = (PCB).line;

	/* Create a new instruction object */
	pInst = new CInstruction;
	if (pInst != NULL)
	{
		pInst->m_ID = opcode;						// Set opcode value
		pInst->m_Line = m_Line;						// Get Line number
		pInst->m_Address = m_ActiveSeg->m_Address++;// Get current program address
		m_ActiveAddr->length++;						// Update add range length
		pInst->m_FileIndex = m_FileIndex;			// Save index of the current file
		m_Instructions->Add(pInst);					// Save instruction 
	}

	return pInst;
}

/*
===========================================================================================
Function to process zero argument opcodes
===========================================================================================
*/
void VTAssembler::opcode_arg_0(int opcode)
{
	// Determine if conditional assembly enabled
	if (m_IfStat[m_IfDepth] != IF_STAT_ASSEMBLE)
		return;

	// Read operand (register) from string accumulator
	AddInstruction(opcode);
}

/*
===========================================================================================
Function to process opcodes that take a single register argument
===========================================================================================
*/
void VTAssembler::opcode_arg_1reg(int opcode)
{
	// Determine if conditional assembly enabled
	if (m_IfStat[m_IfDepth] != IF_STAT_ASSEMBLE)
	{
		reg_cnt--;									// Pop register arg from stack
		return;
	}

	// Read operand (register) from register stack
	CInstruction*	pInst = AddInstruction(opcode);
	if (pInst != NULL)
	{
		// Append instruction with operand
		pInst->m_Operand1 = new MString;			// Allocte operand object
		pInst->m_Operand1->Format("%c", reg[--reg_cnt]);	// Get register operand
	}
}

/*
===========================================================================================
Function to process opcodes that take a single register argument
===========================================================================================
*/
void VTAssembler::opcode_arg_imm(int opcode, char c)
{
	// Determine if conditional assembly enabled
	if (m_IfStat[m_IfDepth] != IF_STAT_ASSEMBLE)
		return;

	// Read operand (register) from register stack
	CInstruction*	pInst = AddInstruction(opcode);
	if (pInst != NULL)
	{
		// Append instruction with operand
		pInst->m_Operand1 = new MString;			// Allocte operand object
		pInst->m_Operand1->Format("%c", c);			// Get register operand
	}
}

/*
===========================================================================================
Function to process opcodes that take two register argument
===========================================================================================
*/
void VTAssembler::opcode_arg_2reg(int opcode)
{
	// Determine if conditional assembly enabled
	if (m_IfStat[m_IfDepth] != IF_STAT_ASSEMBLE)
	{
		reg_cnt -= 2;								// Pop unused register args from stack
		return;
	}

	// Read operands (2 registers) from Register stack
	CInstruction*	pInst = AddInstruction(opcode);
	if (pInst != NULL)
	{
		// Append instruction with operands
		pInst->m_Group = (VTObject *) (int) reg[--reg_cnt];	// Get register operand
		pInst->m_Operand1 = new MString;			// Allocte operand object
		pInst->m_Operand1->Format("%c", reg[--reg_cnt]);	// Get register operand
	}
}

/*
===========================================================================================
Function to process opcodes that take a single register argument and an equation that
results in an 8-bit value
===========================================================================================
*/
void VTAssembler::opcode_arg_1reg_equ8(int opcode)
{
	// Determine if conditional assembly enabled
	if (m_IfStat[m_IfDepth] != IF_STAT_ASSEMBLE)
	{
		reg_cnt--;									// Pop unused reg operand from stack
		delete gEq;									// Delete the unused equaiton
		gEq = new CRpnEquation;						// Allocate new equation for parser
		return;
	}

	// Read operands (register & equation) from parser
	CInstruction*	pInst = AddInstruction(opcode);
	if (pInst != NULL)
	{
		// Append instruction with operands
		pInst->m_Operand1 = new MString;			// Allocte operand object
		pInst->m_Operand1->Format("%c", reg[--reg_cnt]);	// Get register operand
		pInst->m_Group = gEq;						// Get equation 

		//  Allocate new equation for the parser
		gEq = new CRpnEquation;

		// Increment Address again to account for 8-bit value
		m_ActiveSeg->m_Address++;
		m_ActiveAddr->length += 2;					// Update add range length
	}
}

/*
===========================================================================================
Function to process opcodes that take a single register argument and an equation that
results in an 16-bit value
===========================================================================================
*/
void VTAssembler::opcode_arg_1reg_equ16(int opcode)
{
	// Determine if conditional assembly enabled
	if (m_IfStat[m_IfDepth] != IF_STAT_ASSEMBLE)
	{
		reg_cnt--;									// Pop unused reg operand from stack
		delete gEq;									// Delete the unused equaiton
		gEq = new CRpnEquation;						// Allocate new equation for parser
		return;
	}

	// Read operands (register & equation) from parser
	CInstruction*	pInst = AddInstruction(opcode);
	if (pInst != NULL)
	{
		// Append instruction with operands
		pInst->m_Operand1 = new MString;			// Allocte operand object
		pInst->m_Operand1->Format("%c", reg[--reg_cnt]);	// Get register operand
		pInst->m_Group = gEq;						// Get equation 

		//  Allocate new equation for the parser
		gEq = new CRpnEquation;

		// Add 2 to address to account for 16-bit argument
		m_ActiveSeg->m_Address += 2;
		m_ActiveAddr->length += 3;					// Update add range length
	}
}

/*
===========================================================================================
Function to process opcodes that take an 8 bit equation argument
===========================================================================================
*/
void VTAssembler::opcode_arg_equ8(int opcode)
{
	// Determine if conditional assembly enabled
	if (m_IfStat[m_IfDepth] != IF_STAT_ASSEMBLE)
	{
		delete gEq;									// Delete the unused equaiton
		gEq = new CRpnEquation;						// Allocate new equation for parser
		return;
	}

	// Read operand (equation) from parser
	CInstruction*	pInst = AddInstruction(opcode);
	if (pInst != NULL)
	{
		// Append instruction with operands
		pInst->m_Group = gEq;						// Get equation 
		pInst->m_Line = m_Line;						// Get Line number

		//  Allocate new equation for the parser
		gEq = new CRpnEquation;

		// Increment Address again to account for 8-bit value
		m_ActiveSeg->m_Address++;
		m_ActiveAddr->length += 2;					// Update add range length
	}
}

/*
===========================================================================================
Function to process opcodes that take a 16 bit equation argument
===========================================================================================
*/
void VTAssembler::opcode_arg_equ16(int opcode)
{
	// Determine if conditional assembly enabled
	if (m_IfStat[m_IfDepth] != IF_STAT_ASSEMBLE)
	{
		delete gEq;									// Delete the unused equaiton
		gEq = new CRpnEquation;						// Allocate new equation for parser
		return;
	}

	// Read operand (register) from string accumulator
	CInstruction*	pInst = AddInstruction(opcode);
	if (pInst != NULL)
	{
		// Append instruction with operand
		pInst->m_Group = gEq;						// Get equation 

		//  Allocate new equation for the parser
		gEq = new CRpnEquation;

		// Increment Address to account for 16-bit value
		m_ActiveSeg->m_Address += 2;
		m_ActiveAddr->length += 3;					// Update add range length
	}
}

/*
============================================================================
This function is called when the parser detects an include operation.
============================================================================
*/
void VTAssembler::include(const char *filename)
{
	int				count, c;
	MString			incPathFile;
	FILE*			fd;

	// Determine if conditional assembly enabled
	if (m_IfStat[m_IfDepth] != IF_STAT_ASSEMBLE)
		return;

	// Update line number
	m_Line = (PCB).line - 1;
	
	// Try to open the file
	fd = fopen(filename, "rb");

	// If file not open, try to append the current asm file path and open
	if (fd == 0)
	{
		incPathFile = m_FileDir + filename;
		fd = fopen((const char *) incPathFile, "rb");
	}

	// If file did not open, then look in include directories
	if (fd == 0)
	{
		count = m_IncludeDirs.GetSize();
		for (c = 0; c < count; c++)
		{
			incPathFile = m_IncludeDirs[c] + filename;
			fd = fopen((const char *) incPathFile, "rb");
			if (fd != 0)
				break;
		}
	}

	// Check if file was found in any of the directories
	if (fd == 0)
	{
		MString errMsg;
		errMsg.Format("Error in line %d(%s):  Unable to open include file %s",
			m_Line, (const char *) m_Filenames[m_FileIndex], (const char *) filename);
		m_Errors.Add(errMsg);
		return;
	} else {
		/* Save the parser control block for the current file */
		memcpy(&m_ParserPCBs[m_IncludeDepth], &a85parse_pcb,
			sizeof(a85parse_pcb_type));
		m_IncludeIndex[m_IncludeDepth] = m_FileIndex;
		m_IncludeStack[m_IncludeDepth++] = m_fd;

		// Check if this file already in the m_Filenames list!


		m_FileIndex = m_Filenames.Add(filename);
										  
		// Now loading from a different FD!
		m_fd = fd;
		gFilename = filename;
		a85parse();
		fclose(m_fd);

		// Restore the previous FD
		m_fd = m_IncludeStack[--m_IncludeDepth];
		m_FileIndex = m_IncludeIndex[m_IncludeDepth];
		gFilename = (const char *) m_Filenames[m_FileIndex];
		memcpy(&a85parse_pcb, &m_ParserPCBs[m_IncludeDepth],
			sizeof(a85parse_pcb_type));
	}
}

/*
============================================================================
This function is called when the parser detects an equate operation.
============================================================================
*/
void VTAssembler::equate(const char *name)
{
	double		value;
	VTObject*	dummy;
	CSymbol*	pSymbol;

	// Determine if conditional assembly enabled
	if (m_IfStat[m_IfDepth] != IF_STAT_ASSEMBLE)
	{
		delete gEq;									// Delete the unused equaiton
		gEq = new CRpnEquation;						// Allocate new equation for parser
		return;
	}

	// Update line number
	m_Line = (PCB).line - 1;

	// Check if converting label to equate
	if (name == (const char *) -1)
		pSymbol = m_LastLabelSym;
	else
		pSymbol = new CSymbol;

	// Check if symbol valid
	if (pSymbol != NULL)
	{
		if (name != (const char *) -1)
			pSymbol->m_Name = (char *) name;
		pSymbol->m_Line = m_Line;
		pSymbol->m_FileIndex = m_FileIndex;			// Save index of the current file
		pSymbol->m_SymType = SYM_EQUATE;
		pSymbol->m_pRange = m_ActiveAddr;

		// Try to evaluate the equation.  Note that we may not be able to
		// successfully evaluate it at this point because it may contain
		// a forward reference, so don't report any errors yet.
		if (Evaluate(gEq, &value, 0))
		{
			pSymbol->m_Value = (long) value;
			pSymbol->m_SymType |= SYM_HASVALUE;
		}
		else
		{
			// Symbol did not evaluate, save as an equation
			pSymbol->m_SymType |= SYM_ISEQ;
		}

		// Save equation
		pSymbol->m_Equation = gEq;
		gEq = new CRpnEquation;

		// Loop through all segments for the ative module and check if symbol exists
		if (name != (const char *) -1)
		{
			if (name[0] == '&')
				pSymbol->m_Name = m_LastLabel + (char *) "%%" + pSymbol->m_Name;
			else if (name[0] == m_LocalModuleChar)
				pSymbol->m_Name = m_ActiveMod->m_Name + (char *) "%%" + pSymbol->m_Name;

			// Look in active module for the symbol
			if (!m_Symbols->Lookup(pSymbol->m_Name, dummy))
			{
				// Not found.  If symbol isn't a local label, look in other modules
				if ((name[0] != '&') && (name[0] != m_LocalModuleChar))
					dummy = LookupSymOtherModules(pSymbol->m_Name);
			}

			// If symbol exists, report error
			if (dummy != NULL)
			{
				MString string;
				string.Format("Error in line %d(%s): Symbol %s already defined as %s",
					pSymbol->m_Line, (const char *) m_Filenames[m_FileIndex], 
					(const char *) pSymbol->m_Name,
					types[((CSymbol *)dummy)->m_SymType & 0x00FF]);
				m_Errors.Add(string);

				// Cleanup
				delete pSymbol;
				return;
			}

			// Assign symbol to active segment
			(*m_Symbols)[(const char *) pSymbol->m_Name] = pSymbol;
		}
	}
}

/*
============================================================================
This function is called when the parser detects a label.
============================================================================
*/
void VTAssembler::label(const char *label, int address)
{
	VTObject*	dummy = NULL;
	MString		string;
	int			local;

	// Determine if conditional assembly enabled
	if (m_IfStat[m_IfDepth] != IF_STAT_ASSEMBLE)
		return;

	// Update line number
	m_Line = (PCB).line;
	m_LastLabelLine = m_Line;

	// Determine is label is local (has format $###)
	if (label[0] == '&')
		local = 1;
	else
		local = 0;

	CSymbol*	pSymbol = new CSymbol;
	if (pSymbol != NULL)
	{
		if (local)
			pSymbol->m_Name = m_LastLabel + (char *) "%%" + label;
		else if (label[0] == m_LocalModuleChar)
			pSymbol->m_Name = m_ActiveMod->m_Name + (char *) "%%" + label;
		else
			pSymbol->m_Name = label;
		pSymbol->m_Line = m_Line;
		if (address == -1)
			pSymbol->m_Value = m_ActiveSeg->m_Address;
		else
			pSymbol->m_Value = address;
		pSymbol->m_FileIndex = m_FileIndex;			// Save index of the current file
		pSymbol->m_SymType = SYM_LABEL | SYM_HASVALUE;
		pSymbol->m_pRange = m_ActiveAddr;
		if (m_ActiveSeg->m_Type == CSEG)
			pSymbol->m_SymType |= SYM_CSEG;
		else if (m_ActiveSeg->m_Type == DSEG)
			pSymbol->m_SymType |= SYM_DSEG;

		// Check if label exists in the symbol table for this module
		if (!m_ActiveMod->m_Symbols->Lookup(pSymbol->m_Name, dummy))
		{
			if (!local && (label[0] != m_LocalModuleChar))
				dummy = LookupSymOtherModules(pSymbol->m_Name);
		}

		// If Symbol already exists, report an error
		if (dummy != NULL)
		{
			string.Format("Error in line %d(%s): Label %s already defined as %s",
				pSymbol->m_Line, (const char *) m_Filenames[m_FileIndex], 
				(const char *) pSymbol->m_Name,
				types[((CSymbol *)dummy)->m_SymType & 0x00FF]);
			m_Errors.Add(string);

			// Cleanup
			delete pSymbol;
			return;
		}

		// Assign symbol to active segment
		const char *pStr = (const char *) pSymbol->m_Name;
		(*m_Symbols)[pStr] = pSymbol;
		if (!local)
		{
			m_LastLabel = pStr;
			m_LastLabelSym= pSymbol;
			CInstruction* pInst = new CInstruction;
			pInst->m_ID = INST_LABEL;
			pInst->m_Operand1 = new MString;
			*pInst->m_Operand1 = m_LastLabel;
			m_Instructions->Add(pInst);					// Save instruction 
		}
	}
}

/*
============================================================================
This function is called when a set directive is encountered
============================================================================
*/
void VTAssembler::directive_set(const char *name)
{
	double		value;
	VTObject*	dummy;
	int			symtype;
	CSymbol*	pSymbol;

	// Determine if conditional assembly enabled
	if (m_IfStat[m_IfDepth] != IF_STAT_ASSEMBLE)
	{
		delete gEq;									// Delete the unused equaiton
		gEq = new CRpnEquation;						// Allocate new equation for parser
		return;
	}

	// Update line number
	m_Line = (PCB).line - 1;
	
	// Check if converting label to set
	if (name == (const char *) -1)
		pSymbol = m_LastLabelSym;
	else
		pSymbol = new CSymbol;

	// Check if symbol valid
	if (pSymbol != NULL)
	{
		if (name != (const char *) -1)
			pSymbol->m_Name = (char *) name;
		pSymbol->m_Line = m_Line;
		pSymbol->m_FileIndex = m_FileIndex;			// Save index of the current file
		pSymbol->m_SymType = SYM_SET;
		pSymbol->m_pRange = m_ActiveAddr;

		// Try to evaluate the equation.  Note that we may not be able to
		// successfully evaluate it at this point because it may contain
		// a forward reference, so don't report any errors yet.
		if (Evaluate(gEq, &value, 0))
		{
			pSymbol->m_Value = (long) value;
			pSymbol->m_SymType |= SYM_HASVALUE;
		}
		else
		{
			// Symbol did not evaluate, save as an equation
			pSymbol->m_SymType |= SYM_ISEQ;
		}

		// Save equation
		pSymbol->m_Equation = gEq;
		gEq = new CRpnEquation;

		// Search all modules to see if the symbol exists
		if (name != (const char *) -1)
		{
			// Convert local symbol names
			if (name[0] == '&')
				pSymbol->m_Name = m_LastLabel + (char *) "%%" + pSymbol->m_Name;
			else if (name[0] == m_LocalModuleChar)
				pSymbol->m_Name = m_ActiveMod->m_Name + (char *) "%%" + pSymbol->m_Name;

			// Look in active module for the symbol
			if (!m_Symbols->Lookup(pSymbol->m_Name, dummy))
			{
				// Not found.  If symbol isn't a local label, look in other modules
				if ((name[0] != '&') && (name[0] != m_LocalModuleChar))
					dummy = LookupSymOtherModules(pSymbol->m_Name);
			}

			if (dummy != NULL)
			{
				symtype = ((CSymbol*) dummy)->m_SymType & 0x00FF;

				// Now insure symbol exists as a SYM_SET object
				if (symtype == SYM_SET)
				{
					// Update value of existing symbol object
					if (pSymbol->m_SymType & SYM_HASVALUE)
					{
						((CSymbol *) dummy)->m_Value = pSymbol->m_Value;
						((CSymbol *) dummy)->m_SymType |= SYM_HASVALUE;
					}
					else
						((CSymbol *) dummy)->m_SymType &= ~SYM_HASVALUE;

					// Save equation from the new symbol
					((CSymbol *) dummy)->m_Equation = pSymbol->m_Equation;

					// Delete the newly created CSymbol object
					delete pSymbol->m_Equation;
					delete pSymbol;
					return;
				}
				else
				{
					MString string;
					string.Format("Error in line %d(%s): Symbol %s already defined as %s",
						pSymbol->m_Line, (const char *) m_Filenames[m_FileIndex], 
						(const char *) pSymbol->m_Name,	types[symtype]);
					m_Errors.Add(string);

					// Delete the newly created CSymbol object
					delete pSymbol->m_Equation;
					delete pSymbol;

					return;
				}
			}

			// Assign symbol to active segment
			const char *pStr = (const char *) pSymbol->m_Name;
			(*m_Symbols)[pStr] = pSymbol;
		}
	}
}

/*
============================================================================
This function is called when the parser detects an org operation.
============================================================================
*/
void VTAssembler::directive_org()
{
	double		value;
	AddrRange*	newRange;
	AddrRange*	thisRange;
	AddrRange*	prevRange;

	// Determine if conditional assembly enabled
	if (m_IfStat[m_IfDepth] != IF_STAT_ASSEMBLE)
	{
		delete gEq;									// Delete the unused equaiton
		gEq = new CRpnEquation;						// Allocate new equation for parser
		return;
	}

	// Update line number
	m_Line = (PCB).line;
	
	CInstruction*	pInst = new CInstruction;
	if (pInst != NULL)
	{
		pInst->m_ID = INST_ORG;
		pInst->m_Line = m_Line;
		pInst->m_Group = gEq;
		pInst->m_FileIndex = m_FileIndex;			// Save index of the current file

		// Try to calculate new address
		if (Evaluate(gEq, &value, 0))
		{
			m_ActiveSeg->m_Address = (unsigned short) value;
			pInst->m_Address = m_ActiveSeg->m_Address;

			if (m_ActiveAddr->length != 0)
			{
				// Allocate a new AddrRange object for this ORG
				newRange = new AddrRange;
				newRange->address = m_ActiveSeg->m_Address;
				newRange->length = 0;

				// Find location to insert this object
				thisRange = m_ActiveSeg->m_UsedAddr;
				prevRange = 0;

				while ((thisRange != 0) && (thisRange->address < m_ActiveSeg->m_Address))
				{
					prevRange = thisRange;
					thisRange = thisRange->pNext;
				}

				// Hmmm... we have a region being inserted before the first 
				// region.  This may be possible if the ORG statements are out of order
				// Make this range the first in the list
				if (prevRange == NULL)
				{
					newRange->pNext = m_ActiveSeg->m_UsedAddr;
					m_ActiveSeg->m_UsedAddr = newRange;
					m_ActiveSeg->m_ActiveAddr = newRange;
				}
				else
				{
					// Insert new range in list
					if (prevRange->address + prevRange->length == m_ActiveSeg->m_Address)
					{
						delete newRange;
						m_ActiveSeg->m_ActiveAddr = prevRange;
						m_ActiveAddr = prevRange;
					}
					else
					{
						newRange->pNext = 0;
						prevRange->pNext = newRange;

						m_ActiveSeg->m_ActiveAddr = newRange;
						m_ActiveAddr = newRange;
					}
				}
			}
			else
			{
				m_ActiveAddr->address = m_ActiveSeg->m_Address;
			}
		}
		
		// Allocate a new equation for the parser
		gEq = new CRpnEquation;

		// Add instruction to the active segment
		m_Instructions->Add(pInst);
	}
}

/*
============================================================================
This function is called when an echo string directive is encountered
============================================================================
*/
void VTAssembler::directive_echo(const char *string)
{
	// Determine if conditional assembly enabled
	if (m_IfStat[m_IfDepth] != IF_STAT_ASSEMBLE)
		return;

	if (m_pStdoutFunc != NULL)
		m_pStdoutFunc(m_pStdoutContext, string);
}

/*
============================================================================
This function is called when a printf directive is encountered
============================================================================
*/
void VTAssembler::directive_printf(const char *string)
{
	char		str[256];
	double		value = 0.0;
	long		lval;
	const char	*ptr;
	char		fmtCh = 0;
	int			fmtDigits = 0;

	// Determine if conditional assembly enabled
	if (m_IfStat[m_IfDepth] != IF_STAT_ASSEMBLE)
	{
		delete gEq;									// Delete the unused equaiton
		gEq = new CRpnEquation;						// Allocate new equation for parser
		return;
	}

	// Find the locaiton of the format specifier
	ptr = string;
	while ((*ptr != '%') && (*ptr != '\0'))
		ptr++;
	if (*ptr == '%')
	{
		while (VT_ISDIGIT(*(ptr + 1 + fmtDigits)))
			fmtDigits++;
		fmtCh = *(ptr + 1 + fmtDigits);
	}

	// Try to evaluate the equation.  Note that we may not be able to
	// successfully evaluate it at this point because it may contain
	// a forward reference, so don't report any errors yet.
	if (Evaluate(gEq, &value, 0))
	{
		if (fmtCh == 'f')
			sprintf(str, string, value);
		else
		{
			lval = (int) value;
			sprintf(str, string, lval);
		}
	}
	else
	{
		strncpy(str, string, (int) (ptr - string));
		strcat(str, "#UNDEFINED");
		if (*ptr != '\0')
			strcat(str, ptr + 2 + fmtDigits);
	}

	if (m_pStdoutFunc != NULL)
		m_pStdoutFunc(m_pStdoutContext, (const char *) str);
}

/*
============================================================================
This function is called when an echo equation directive is encountered
============================================================================
*/
void VTAssembler::directive_echo()
{
	double	value, value2;
	long	lval;
	char	str[20];

	// Determine if conditional assembly enabled
	if (m_IfStat[m_IfDepth] != IF_STAT_ASSEMBLE)
	{
		delete gEq;									// Delete the unused equaiton
		gEq = new CRpnEquation;						// Allocate new equation for parser
		return;
	}

	// Try to evaluate the equation.  Note that we may not be able to
	// successfully evaluate it at this point because it may contain
	// a forward reference, so don't report any errors yet.
	if (Evaluate(gEq, &value, 0))
	{
		// Check if value is integer or float
		lval = (long) value;
		value2 = lval;
		if (value != value2)
			sprintf(str, "%f", value2);
		else
		{
			sprintf(str, "%ld", lval);
		}
		if (m_pStdoutFunc != NULL)
			m_pStdoutFunc(m_pStdoutContext, (const char *) str);
	}
	else
	{
		if (m_pStdoutFunc != NULL)
			m_pStdoutFunc(m_pStdoutContext, "#UNDEFINED");
	}

	delete gEq;									// Delete the unused equaiton
	gEq = new CRpnEquation;						// Allocate new equation for parser
}

/*
============================================================================
This function searches for the specified #define 
============================================================================
*/
int VTAssembler::LookupMacro(MString& name, CMacro *&macro)
{
	int		count, c;
	CMacro*	pMacro;

	count = m_Defines.GetSize();
	for (c = 0; c < count; c++)
	{
		pMacro = (CMacro *) m_Defines[c];

		// Check if this 
		if (pMacro->m_Name == gMacro->m_Name)
		{
			// We found a match on the macro name
			macro = pMacro;
			return TRUE;
		}
	}

	macro = NULL;
	return FALSE;
}

/*
============================================================================
This function searches for the specified symbol in the active module
============================================================================
*/
int VTAssembler::LookupSymbol(MString& name, CSymbol *&symbol)
{

	// Test if the specified label / define exists
	if (m_ActiveMod->m_Symbols->Lookup(name, (VTObject*&) symbol))
		return TRUE;

	// Symbol not found in active module.  Check other modules
	if (name.GetLength() > 1)
	{
		if ((name[0] != '&') && (name[0] != m_LocalModuleChar))
		{
			if ((symbol = LookupSymOtherModules(name)) != NULL)
				return TRUE;
		}
	}

	symbol = NULL;
	return FALSE;
}

/*
============================================================================
This function searches for the specified symbol in the active module
============================================================================
*/
CSymbol * VTAssembler::LookupSymOtherModules(MString& name, CSegment** pSeg)
{
	CSymbol		*pSymbol;
	CModule		*pMod;
	POSITION	pos;
	MString		key;

	pos = m_Modules.GetStartPosition();

	// Scan all modules that have been created
	while (pos != NULL)
	{
		// Get pointer to next module
		m_Modules.GetNextAssoc(pos, key, (VTObject *&) pMod);
		if (pMod == m_ActiveMod)
		{
			// Don't search the active module
			continue;
		}

		// Test if the specified label / define exists
		if (pMod->m_Symbols->Lookup(name, (VTObject*&) pSymbol))
		{
			if (pSeg != NULL)
				*pSeg = pSymbol->m_Segment;
			return pSymbol;
		}
	}

	return NULL;
}

/*
============================================================================
This function is called when an ASEG directive is found by the parser
============================================================================
*/
void VTAssembler::directive_aseg()
{
	CSegment*		pSeg;

	// Determine if conditional assembly enabled
	if (m_IfStat[m_IfDepth] != IF_STAT_ASSEMBLE)
		return;

	// Update line number
	m_Line = (PCB).line;
	
	// Set segment of current module
	m_Segments.Lookup(".aseg", (VTObject *&) pSeg);
//	pSeg = (CSegment *) m_Segments[".aseg"];

	// Activate the new segment
	ActivateSegment(pSeg);
}

/*
============================================================================
This function sets the specified Segment as Active.  
============================================================================
*/
void VTAssembler::ActivateSegment(CSegment* pSeg)
{
	// Check if we were already in the ASEG
	if (pSeg == m_ActiveSeg)
		return;

	// Update the active segment to assemble into
	m_ActiveSeg = pSeg;

	// Update pointers to assembly objects
	m_Instructions = m_ActiveSeg->m_Instructions;
	m_ActiveAddr = m_ActiveSeg->m_ActiveAddr;

	// Update last line of current SegLines
	m_ActiveSegLines->lastLine = (PCB).line - 1;

	// Create a new SegLines and add to the array
	m_ActiveSegLines = new CSegLines(m_ActiveSeg, m_Line);
	m_SegLines.Add(m_ActiveSegLines);

	// Check if the module changed while we were away
	if (m_ActiveSeg->m_LastMod != m_ActiveMod)
	{
		// Add an artificial instruction to reflect change in module
		CInstruction*	pInst = new CInstruction;
		if (pInst != NULL)
		{
			pInst->m_ID = INST_MODULE;
			pInst->m_Line = m_Line;
			pInst->m_Operand1 = new MString;	// Allocte operand object
			*pInst->m_Operand1 = m_ActiveMod->m_Name;
			pInst->m_FileIndex = m_FileIndex;	// Save index of the current file
			m_Instructions->Add(pInst);
		}
	}
}

/*
============================================================================
This function is called when a cseg or dseg directive is encountered
============================================================================
*/
void VTAssembler::directive_cdseg(int segType, int page)
{
	MString		segName = ".text";
	const char *name;
	CSegment*	pSeg;
	MString		err;

	// Determine if conditional assembly enabled
	if (m_IfStat[m_IfDepth] != IF_STAT_ASSEMBLE)
		return;

	// Update line number
	m_Line = (PCB).line - 1;
	if (segType == DSEG)
		segName = ".data";
	
	// If this line has a label on it, then change the segment name
	if (m_LastLabelLine == m_Line)
	{
		segName = m_LastLabel;
	}

	// Lookup the segment in our map
	name = (const char *) segName;
	if (!m_Segments.Lookup(name, (VTObject *&) pSeg))
	{
		// Segment does not exist.  Create new segment
		pSeg = new CSegment(name, segType, m_ActiveMod);
		if (pSeg == NULL)
		{
			err.Format("Error in line %d(%s):  Error allocating new segment %s", m_Line, 
				(const char *) m_Filenames[m_FileIndex], name);
			m_Errors.Add(err);
			return;
		}

		// Add segment to our list of segments
		m_Segments[name] = pSeg;
	}

	// Activate the new segment
	ActivateSegment(pSeg);
}

/*
============================================================================
This function is called when a ds directive is encountered
============================================================================
*/
void VTAssembler::directive_ds()
{
	double		len;

	// Determine if conditional assembly enabled
	if (m_IfStat[m_IfDepth] != IF_STAT_ASSEMBLE)
	{
		delete gEq;									// Delete the unused equaiton
		gEq = new CRpnEquation;						// Allocate new equation for parser
		return;
	}

	// Update line number
	m_Line = (PCB).line;
	
	CInstruction*	pInst = new CInstruction;
	if (pInst != NULL)
	{
		pInst->m_ID = INST_DS;
		pInst->m_Line = m_Line;
		pInst->m_Group = gEq;
		pInst->m_FileIndex = m_FileIndex;			// Save index of the current file
		pInst->m_Address = m_ActiveSeg->m_Address;
		gEq = new CRpnEquation;

		// Try to evaluate the equation
		if (Evaluate((CRpnEquation *) pInst->m_Group, &len, 0))
		{
			m_ActiveSeg->m_Address += (int) len;
			m_ActiveAddr->length += (unsigned short) len;
		}

		m_Instructions->Add(pInst);
	}
}

/*
============================================================================
This function is called when a fill directive is encountered
============================================================================
*/
void VTAssembler::directive_fill()
{
	int				c, len;
	int				fillToAddr = -1;
	unsigned char	fillByte = 0xFF;
	double			value;
	MString			string;

	// Determine if conditional assembly enabled
	if (m_IfStat[m_IfDepth] != IF_STAT_ASSEMBLE)
	{
		for (c = 0; c < gExpList->GetSize(); c++)
			delete (*gExpList)[c];
		delete gExpList;							// Delete the unused expression list
		gExpList = new VTObArray;					// Allocate new expresssion list for parser
		return;
	}

	// Update line number
	m_Line = (PCB).line;

	CInstruction *pInst = new CInstruction;

	if (pInst != NULL)
	{
		pInst->m_ID = INST_FILL;
		pInst->m_Line = m_Line;
		pInst->m_Group = gExpList;
		pInst->m_Address = m_ActiveSeg->m_Address;
		pInst->m_FileIndex = m_FileIndex;			// Save index of the current file

		// Test for syntax error if too many expressions
		len = gExpList->GetSize();
		if (len > 2)
		{
			// Log an error
			string.Format("Error in line %d(%s): Too many expressions for fill",
				m_Line, (const char *) m_Filenames[m_FileIndex]);
			m_Errors.Add(string);
		}
		else
		{
			// Update address based on number of items in expression list
			if (((CExpression *) (*gExpList)[0])->m_Equation != NULL)
			{
				// Try to evaluate the equation
				CRpnEquation *pEq = ((CExpression *) (*gExpList)[0])->m_Equation;
				if (Evaluate(pEq, &value, 0))
				{
					// Update address based on fill size
					len = (int) value;
					m_ActiveSeg->m_Address += len;
					m_ActiveAddr->length += len;	// Update add range length
				}
				else
				{
					// Log an error
					string.Format("Error in line %d(%s): Must be able to evaluate fill size on first pass!",
						m_Line, (const char *) m_Filenames[m_FileIndex]);
					m_Errors.Add(string);
				}
			}
			else
			{
				// Log an error
				string.Format("Error in line %d(%s): Number of bytes for fill must be an equation",
					m_Line, (const char *) m_Filenames[m_FileIndex]);
				m_Errors.Add(string);
			}
		}

		// Add new instruction to the instruction list and create new expression list
		gExpList = new VTObArray;
		m_Instructions->Add(pInst);
	}
}

/*
============================================================================
This function is called when a db directive is encountered
============================================================================
*/
void VTAssembler::directive_db()
{
	int		c, len;

	// Determine if conditional assembly enabled
	if (m_IfStat[m_IfDepth] != IF_STAT_ASSEMBLE)
	{
		for (c = 0; c < gExpList->GetSize(); c++)
			delete (*gExpList)[c];
		delete gExpList;							// Delete the unused expression list
		gExpList = new VTObArray;					// Allocate new expresssion list for parser
		return;
	}

	// Update line number
	m_Line = (PCB).line;
	
	CInstruction*	pInst = new CInstruction;
	if (pInst != NULL)
	{
		pInst->m_ID = INST_DB;
		pInst->m_Line = m_Line;
		pInst->m_Group = gExpList;
		pInst->m_Address = m_ActiveSeg->m_Address;
		pInst->m_FileIndex = m_FileIndex;			// Save index of the current file

		// Update address based on number of items in expression list
		len = gExpList->GetSize();
		for (c = 0; c < len; c++)
		{
			if (((CExpression *) (*gExpList)[c])->m_Equation != NULL)
			{
				m_ActiveSeg->m_Address++;
				m_ActiveAddr->length++;				// Update add range length
			}
			else
			{
				int size = strlen(((CExpression *) (*gExpList)[c])->m_Literal);
				m_ActiveSeg->m_Address += size;
				m_ActiveAddr->length += size;		// Update add range length
			}
		}

		gExpList = new VTObArray;
		m_Instructions->Add(pInst);
	}
}

/*
============================================================================
This function is called when a dw directive is encountered
============================================================================
*/
void VTAssembler::directive_dw()
{
	int		len, c;

	// Determine if conditional assembly enabled
	if (m_IfStat[m_IfDepth] != IF_STAT_ASSEMBLE)
	{
		for (c = 0; c < gExpList->GetSize(); c++)
			delete (*gExpList)[c];
		delete gExpList;							// Delete the unused expression list
		gExpList = new VTObArray;					// Allocate new expresssion list for parser
		return;
	}

	// Update line number
	m_Line = (PCB).line;
	
	CInstruction*	pInst = new CInstruction;
	if (pInst != NULL)
	{
		pInst->m_ID = INST_DW;
		pInst->m_Line = m_Line;
		pInst->m_Group = gExpList;
		pInst->m_Address = m_ActiveSeg->m_Address;
		pInst->m_Operand1 = new MString;			// Allocte operand object
		pInst->m_Operand1->Format("%d", m_MsbFirst);
		pInst->m_FileIndex = m_FileIndex;			// Save index of the current file

		// Update address based on number of items in expression list
		len = gExpList->GetSize();
		for (c = 0; c < len; c++)
		{
			if (((CExpression *) (*gExpList)[c])->m_Equation != NULL)
			{
				m_ActiveSeg->m_Address += 2;
				m_ActiveAddr->length += 2;			// Update add range length
			}
			else
			{
				int size = strlen(((CExpression *) (*gExpList)[c])->m_Literal);
				if (size & 0x01)
					size++;
				m_ActiveSeg->m_Address += size;
				m_ActiveAddr->length += size;		// Update add range length
			}
		}

		gExpList = new VTObArray;
		m_Instructions->Add(pInst);
	}
}

/*
============================================================================
This function is called when an extern directive is encountered
============================================================================
*/
void VTAssembler::directive_extern()
{
	int			c, count;
	CSymbol*	pSymbol;

	// Determine if conditional assembly enabled
	if (m_IfStat[m_IfDepth] != IF_STAT_ASSEMBLE)
	{
		delete gNameList;							// Delete the unused name list
		gNameList = new MStringArray;				// Allocate new name list for parser
		return;
	}

	// Update line number
	m_Line = (PCB).line;
	
	CInstruction*	pInst = new CInstruction;
	if (pInst != NULL)
	{
		pInst->m_ID = INST_EXTERN;
		pInst->m_Line = m_Line;
		pInst->m_Group = (VTObject *) gNameList;
		pInst->m_FileIndex = m_FileIndex;			// Save index of the current file

		// Add names to symbol table as "extern"
		count = gNameList->GetSize();
		for (c = 0; c < count; c++)
		{
			// Check if symbol already exists
			if (LookupSymbol((*gNameList)[c], pSymbol))
			{
				MString string;
				string.Format("Error in line %d(%s): Symbol %s already defined as %s",
					pSymbol->m_Line, (const char *) m_Filenames[m_FileIndex], 
					(const char *) (*gNameList)[c],
					types[pSymbol->m_SymType & 0x00FF]);
				m_Errors.Add(string);
			}
			else
			{
				pSymbol = new CSymbol;
				pSymbol->m_Line = m_Line;
				pSymbol->m_FileIndex = m_FileIndex;			// Save index of the current file
				pSymbol->m_Name = (*gNameList)[c];
				pSymbol->m_SymType = SYM_EXTERN;
				(*m_Symbols)[pSymbol->m_Name] = pSymbol;
			}
		}

		gNameList = new MStringArray;
		m_Instructions->Add(pInst);
	}
}

/*
============================================================================
This function is called when a public directive is encountered
============================================================================
*/
void VTAssembler::directive_public()
{
	// Determine if conditional assembly enabled
	if (m_IfStat[m_IfDepth] != IF_STAT_ASSEMBLE)
	{
		delete gNameList;							// Delete the unused name list
		gNameList = new MStringArray;				// Allocate new name list for parser
		return;
	}

	// Update line number
	m_Line = (PCB).line;
	
	CInstruction*	pInst = new CInstruction;
	if (pInst != NULL)
	{
		pInst->m_ID = INST_PUBLIC;
		pInst->m_Line = m_Line;
		pInst->m_Group = (VTObject *) gNameList;
		pInst->m_FileIndex = m_FileIndex;			// Save index of the current file
		gNameList = new MStringArray;
		m_Instructions->Add(pInst);
	}
}

/*
============================================================================
This function is called when a sym directive is encountered
============================================================================
*/
void VTAssembler::directive_sym()
{
	// Determine if conditional assembly enabled
	if (m_IfStat[m_IfDepth] != IF_STAT_ASSEMBLE)
		return;

	// Update line number
	m_Line = (PCB).line;
	
	CInstruction*	pInst = new CInstruction;
	if (pInst != NULL)
	{
		pInst->m_ID = INST_SYM;
		pInst->m_Line = m_Line;
		pInst->m_FileIndex = m_FileIndex;			// Save index of the current file
		m_Instructions->Add(pInst);
	}
}

/*
============================================================================
This function is called when a page directive is encountered
============================================================================
*/
void VTAssembler::directive_page(int page)
{
	// Determine if conditional assembly enabled
	if (m_IfStat[m_IfDepth] != IF_STAT_ASSEMBLE)
		return;

	// Update line number
	m_Line = (PCB).line;
	
	CInstruction*	pInst = new CInstruction;
	if (pInst != NULL)
	{
		pInst->m_ID = INST_PAGE;
		pInst->m_Line = m_Line;
		pInst->m_Operand1 = new MString;			// Allocte operand object
		pInst->m_Operand1->Format("%d", page);
		pInst->m_FileIndex = m_FileIndex;			// Save index of the current file
		m_Instructions->Add(pInst);
	}
}

/*
============================================================================
This function is called when a link directive is encountered
============================================================================
*/
void VTAssembler::directive_link(const char *name)
{
	MString		linkName = name;

	linkName += (char *) ".asm";
	include ((const char *) linkName);
	return;
	// Determine if conditional assembly enabled
	if (m_IfStat[m_IfDepth] != IF_STAT_ASSEMBLE)
		return;

	// Update line number
	m_Line = (PCB).line;
	
	CInstruction*	pInst = new CInstruction;
	if (pInst != NULL)
	{
		pInst->m_ID = INST_LINK;
		pInst->m_Line = m_Line;
		pInst->m_Operand1 = new MString;			// Allocte operand object
		*pInst->m_Operand1 = name;
		pInst->m_FileIndex = m_FileIndex;			// Save index of the current file
		m_Instructions->Add(pInst);
	}
}

/*
============================================================================
This function is called when a maclib directive is encountered
============================================================================
*/
void VTAssembler::directive_maclib(const char *name)
{
	// Determine if conditional assembly enabled
	if (m_IfStat[m_IfDepth] != IF_STAT_ASSEMBLE)
		return;

	// Update line number
	m_Line = (PCB).line;
	
	CInstruction*	pInst = new CInstruction;
	if (pInst != NULL)
	{
		pInst->m_ID = INST_MACLIB;
		pInst->m_Line = m_Line;
		pInst->m_Operand1 = new MString;			// Allocte operand object
		*pInst->m_Operand1 = name;
		pInst->m_FileIndex = m_FileIndex;			// Save index of the current file
		m_Instructions->Add(pInst);
	}
}

/*
============================================================================
This function is called when a module directive is encountered.  Modules
allow definition of local labels using the _ prefix.
============================================================================
*/
void VTAssembler::directive_module(const char *name)
{
	CModule*	pMod;
	MString		err;

	CInstruction*	pInst = new CInstruction;
	if (pInst != NULL)
	{
		pInst->m_ID = INST_MODULE;
		pInst->m_Line = m_Line;
		pInst->m_Operand1 = new MString;			// Allocte operand object
		*pInst->m_Operand1 = name;
		pInst->m_FileIndex = m_FileIndex;			// Save index of the current file
		m_Instructions->Add(pInst);
	}

	// Check if module already exists
	if (m_Modules.Lookup(name, (VTObject *&) pMod))
	{
		m_ActiveMod = pMod;
		m_Symbols = m_ActiveMod->m_Symbols;
		m_ActiveSeg->m_LastMod = m_ActiveMod;
		return;
	}

	// Module doesn't exit.  Create a new module
	pMod = new CModule(name);
	if (pMod == NULL)
	{
		err.Format("Error in line %d(%s):  Error creating new module %s", m_Line, 
			(const char *) m_Filenames[m_FileIndex], name);
		m_Errors.Add(err);
	}

	// Add this module to the list and make it active
	m_Modules[name] = pMod;
	m_ActiveMod = pMod;
	m_Symbols = m_ActiveMod->m_Symbols;
	m_ActiveSeg->m_LastMod = m_ActiveMod;
}

/*
============================================================================
This function is called when a name directive is encountered
============================================================================
*/
void VTAssembler::directive_name(const char *name)
{
	// Determine if conditional assembly enabled
	if (m_IfStat[m_IfDepth] != IF_STAT_ASSEMBLE)
		return;

	// Update line number
	m_Line = (PCB).line;
	
	// Check if module already has a name
	if (m_ActiveMod->m_Name != "basemod")
	{
		// Report an error
		MString	string;

		string.Format("Error in line %d(%s): Module name already specified as %s",
			m_Line, (const char *) m_Filenames[m_FileIndex], 
			(const char *) m_ActiveMod->m_Name);
		m_Errors.Add(string);

		return;
	}

	// Save module name in active CModule object
	m_Modules.RemoveAt(m_ActiveMod->m_Name);
	m_ActiveMod->m_Name = name;

	// Reassign current module with new name
	m_Modules[name] = m_ActiveMod;
}

/*
============================================================================
This function is called when a title directive is encountered
============================================================================
*/
void VTAssembler::directive_title(const char *name)
{
	// Determine if conditional assembly enabled
	if (m_IfStat[m_IfDepth] != IF_STAT_ASSEMBLE)
		return;

	// Update line number
	m_Line = (PCB).line;
	
	// Check if module already has a name
	if (m_ActiveMod->m_Title != "")
	{
		// Report an error
		MString	string;

		string.Format("Error in line %d(%s): Module title already specified as %s",
			m_Line, (const char *) m_Filenames[m_FileIndex], 
			(const char *) m_ActiveMod->m_Title);
		m_Errors.Add(string);

		return;
	}

	// Save module name in active CModule object
	m_ActiveMod->m_Title = name;
}

/*
============================================================================
This function is called when a stkln directive is encountered
============================================================================
*/
void VTAssembler::directive_stkln()
{
	// Determine if conditional assembly enabled
	if (m_IfStat[m_IfDepth] != IF_STAT_ASSEMBLE)
	{
		delete gEq;									// Delete the unused equation
		gEq = new CRpnEquation;						// Allocate new equation for parser
		return;
	}

	// Update line number
	m_Line = (PCB).line;
	
	CInstruction*	pInst = new CInstruction;
	if (pInst != NULL)
	{
		pInst->m_ID = INST_STKLN;
		pInst->m_Line = m_Line;
		pInst->m_Group = gEq;
		pInst->m_FileIndex = m_FileIndex;			// Save index of the current file
		gEq = new CRpnEquation;
		m_Instructions->Add(pInst);
	}
}

/*
============================================================================
This function is called when a lsfirst / msfirst directive is encountered
============================================================================
*/
void VTAssembler::directive_endian(int msbFirst)
{
	// Determine if conditional assembly enabled
	if (m_IfStat[m_IfDepth] != IF_STAT_ASSEMBLE)
	{
		return;
	}
	m_MsbFirst = msbFirst;
}

/*
============================================================================
This function is called when an end directive is encountered
============================================================================
*/
void VTAssembler::directive_end(const char *name)
{
	// Determine if conditional assembly enabled
	if (m_IfStat[m_IfDepth] != IF_STAT_ASSEMBLE)
		return;

	// Update line number
	m_Line = (PCB).line;
	
	CInstruction*	pInst = new CInstruction;
	if (pInst != NULL)
	{
		pInst->m_ID = INST_END;
		pInst->m_Line = m_Line;
		pInst->m_Operand1 = new MString;			// Allocte operand object
		*pInst->m_Operand1 = name;
		pInst->m_FileIndex = m_FileIndex;			// Save index of the current file
		m_Instructions->Add(pInst);
	}
}

/*
============================================================================
This function is called when an echo string directive is encountered
============================================================================
*/
int VTAssembler::preproc_error(const char *string)
{
	// Determine if conditional assembly enabled
	if (m_IfStat[m_IfDepth] != IF_STAT_ASSEMBLE)
	{
		return FALSE;
	}

	MString err = string;
	err.Format("Error in line %d(%s):  %s", m_Line, 
		(const char *) m_Filenames[m_FileIndex], string);
	m_Errors.Add(err);

	return TRUE;
}

/*
============================================================================
This is the preprocessor function to handle conditional IFDEF statements
============================================================================
*/
void VTAssembler::preproc_ifdef(const char* name, int negate)
{
	MString		err, strName;
	CSymbol*	dummy;
	int			defined;

	// Update line number
	m_Line = (PCB).line;
	
	m_LastIfElseLine = m_Line;
	m_LastIfElseIsIf = 1;

	// First push results from previous IF/ELSE/ENDIF operation so
	// we generate "nested if" conditions.  Initialize condition to
	// EVAL_ERROR in case the condition does not evaluate
	if ((m_IfStat[m_IfDepth] == IF_STAT_DONT_ASSEMBLE) ||
		(m_IfStat[m_IfDepth] == IF_STAT_NESTED_DONT_ASSEMBLE) ||
		(m_IfStat[m_IfDepth] == IF_STAT_EVAL_ERROR))
	{
		m_IfStat[++m_IfDepth] = IF_STAT_NESTED_DONT_ASSEMBLE;
	}
	else
		m_IfStat[++m_IfDepth] = IF_STAT_EVAL_ERROR;

	// Ensure our #ifdef stack depth hasn't overflowed
	if (m_IfDepth >= sizeof(m_IfStat))
	{
		m_IfDepth--;
		err.Format("Error in line %d(%s):  Too many nested ifs", m_Line, 
			(const char *) m_Filenames[m_FileIndex]);
		m_Errors.Add(err);
	}
	else if (m_IfStat[m_IfDepth] == IF_STAT_EVAL_ERROR)
	{
		m_IfStat[m_IfDepth] = IF_STAT_DONT_ASSEMBLE;
		strName = name;
		defined = LookupSymbol(strName, dummy);
		if ((defined && !negate) || (!defined && negate))
			m_IfStat[m_IfDepth] = IF_STAT_ASSEMBLE;
	}
}

/*
============================================================================
This is the preprocessor function to handle conditional IF statements
============================================================================
*/
void VTAssembler::preproc_if(void)
{
	directive_if();
}

/*
============================================================================
This is the preprocessor function to handle conditional ELIF statements
============================================================================
*/
void VTAssembler::preproc_elif(void)
{
	directive_if(INST_ELIF);
}

/*
============================================================================
This is the preprocessor function to handle conditional IFNDEF statements
============================================================================
*/
void VTAssembler::preproc_ifndef(const char* name)
{
	preproc_ifdef(name, TRUE);
}

/*
============================================================================
This is the preprocessor function to handle conditional ELSE statements
============================================================================
*/
void VTAssembler::preproc_else()
{
	directive_else();
}

/*
============================================================================
This is the preprocessor function to handle conditional ENDIF statements
============================================================================
*/
void VTAssembler::preproc_endif()
{
	directive_endif();
}

/*
============================================================================
This is the preprocessor function to handle conditional DEFINE statements
============================================================================
*/
void VTAssembler::preproc_define()
{
	// Update line number
	m_Line = (PCB).line;

	CInstruction *pInst = new CInstruction;

	if (pInst != NULL)
	{
		pInst->m_ID = INST_DEFINE;
		pInst->m_Line = m_Line;
		pInst->m_Group = gMacro;				// Save the macro
		pInst->m_Operand1 = new MString;
		*pInst->m_Operand1 = gMacro->m_Name;	// Save name of define
		pInst->m_Address = m_ActiveSeg->m_Address;
		pInst->m_FileIndex = m_FileIndex;			// Save index of the current file

		// Add the macro to our array of defines
		m_Defines.Add(gMacro);

		// Add new instruction to the instruction list and create new expression list
		gMacro = new CMacro;					// Create a new macro object
		m_Instructions->Add(pInst);
	}
}

/*
============================================================================
This is the preprocessor function to handle conditional UNDEF statements
============================================================================
*/
void VTAssembler::preproc_undef(const char *name)
{
}

/*
============================================================================
This is the preprocessor function to handle macros found in the code that
is acrually a statement vs. a parameter / equation
============================================================================
*/
int VTAssembler::preproc_macro()
{
	CMacro*		pMacro;
	MString		err, replace;

	// Test if we are in the middle of a #define reduction and simply exit if we are
	if (gDefine)
		return 0;

	// Test if the macro is found in our m_Defines array
	if (LookupMacro(gMacro->m_Name, pMacro))
	{
		// Macro found.  We need to do any parameter substitution and configure
		// a macro string for parser input
		if (pMacro->m_ParamList != NULL)
		{
			if ((gMacro->m_ParamList == NULL) || (pMacro->m_ParamList->GetSize() != gMacro->m_ParamList->GetSize()))
			{
				// Invalid number of parameters to macro!
				err.Format("Error in line %d(%s):  Macro %s expects %d parameters", m_Line, 
					(const char *) m_Filenames[m_FileIndex], (const char *) pMacro->m_Name, 
					pMacro->m_ParamList->GetSize());
				m_Errors.Add(err);
				return FALSE;
			}
		}
		else if (gMacro->m_ParamList != 0)
		{
			// Invalid number of parameters to macro!
			err.Format("Error in line %d(%s):  Macro %s expects no parameters", m_Line, 
				(const char *) m_Filenames[m_FileIndex], (const char *) pMacro->m_Name);
			m_Errors.Add(err);
			return FALSE;
		}

		// Assign the replacement string
		replace = pMacro->m_DefString;

		// Do parameter substitution
		if (pMacro->m_ParamList != NULL)
		{
			int		c, count;

			// Loop for each parameter
			count = pMacro->m_ParamList->GetSize();
			for (c = 0; c < count; c++)
			{
				int		atEnd = FALSE;
				int		pos = 0;
				CExpression*	paramExp = (CExpression *) pMacro->m_ParamList->GetAt(c);
				MString	param = paramExp->m_Literal;
				int		len = param.GetLength();
				CExpression*	argExp = (CExpression *) gMacro->m_ParamList->GetAt(c);
				MString arg = argExp->m_Literal;

				// Find all occurances of this parameter in the Definition String
				while (pos != -1)
				{
					int wholeMatch = TRUE;
					pos = replace.Find((char *) (const char *) param, pos);
					if (pos >= 0)
					{
						// Test if the item found is a full match
						if (pos != 0)
						{
							char ch = replace[pos-1];	// Get char before the find
							if ((ch >= 'A') && (ch <= 'z') || (ch == '_') || (ch == '$') || (ch == '&'))
								wholeMatch = FALSE;
						}
						if (pos + len != replace.GetLength()-1)
						{
							char ch = replace[pos + len +1 ];
							if ((ch >= 'A') && (ch <= 'z') || (ch == '_') || (ch == '$') || (ch == '&'))
								wholeMatch = FALSE;
						}

						// If this is full match then substitue the text
						replace.Delete(pos, len);
						replace.Insert(pos, (char *) (const char *) arg);
						pos += arg.GetLength();
					}
				}
			}
		}

		// Indicate the token should be changed to WS
		return TRUE;
	}

	return 0;
}

/*
============================================================================
This function is called when an if directive is encountered
============================================================================
*/
void VTAssembler::directive_if(int inst)
{
	double		valuel, valuer;
	MString		err;

	// Update line number
	m_Line = (PCB).line;
	
//	CInstruction*	pInst = new CInstruction;
//	if (pInst != NULL)
//	{
//		pInst->m_ID = inst;							// If or elif
//		pInst->m_Line = m_Line;
//		pInst->m_Group = gCond;
//		pInst->m_FileIndex = m_FileIndex;			// Save index of the current file
		m_LastIfElseLine = m_Line;
		m_LastIfElseIsIf = 1;

		// First push results from previous IF/ELSE/ENDIF operation so
		// we generate "nested if" conditions.  Initialize condition to
		// EVAL_ERROR in case the condition does not evaluate
		if (inst == INST_IF)
		{
			// Process if instrution
			if ((m_IfStat[m_IfDepth] == IF_STAT_DONT_ASSEMBLE) ||
				(m_IfStat[m_IfDepth] == IF_STAT_NESTED_DONT_ASSEMBLE) ||
				(m_IfStat[m_IfDepth] == IF_STAT_EVAL_ERROR))
			{
				m_IfStat[++m_IfDepth] = IF_STAT_NESTED_DONT_ASSEMBLE;
			}
			else
				m_IfStat[++m_IfDepth] = IF_STAT_EVAL_ERROR;

			if (m_IfDepth >= sizeof(m_IfStat))
			{
				m_IfDepth--;
				err.Format("Error in line %d(%s):  Too many nested ifs", m_Line, 
					(const char *) m_Filenames[m_FileIndex]);
				m_Errors.Add(err);
				gCond = new CCondition;
//				m_Instructions->Add(pInst);
			}
		}
		else
		{
			// Process elif instruction
			if (m_IfDepth == 0)
			{
				err.Format("Error in line %d(%s):  ELSE without a matching IF", m_Line, 
					(const char *) m_Filenames[m_FileIndex]);
				m_Errors.Add(err);
				return;
			}

			if (m_IfStat[m_IfDepth] == IF_STAT_ASSEMBLE)
				m_IfStat[m_IfDepth] = IF_STAT_NESTED_DONT_ASSEMBLE;
			else
				if (m_IfStat[m_IfDepth] == IF_STAT_DONT_ASSEMBLE)
					m_IfStat[m_IfDepth] = IF_STAT_EVAL_ERROR;
		}

		// Determine if both equations can be evaluated
		if (m_IfStat[m_IfDepth] == IF_STAT_EVAL_ERROR)
		{
			if (Evaluate(gCond->m_EqLeft, &valuel, 0))
			{
				// Check if condition contains 2 equations or not
				if (gCond->m_EqRight != 0)
				{
					if (Evaluate(gCond->m_EqRight, &valuer, 0))
					{
						m_IfStat[m_IfDepth] = IF_STAT_DONT_ASSEMBLE;

						// Both equations evaluate, check condition
						switch (gCond->m_Condition)
						{
						case COND_EQ:
							if (valuel == valuer)
								m_IfStat[m_IfDepth] = IF_STAT_ASSEMBLE;
							break;

						case COND_NE:	
							if (valuel != valuer)
								m_IfStat[m_IfDepth] = IF_STAT_ASSEMBLE;
							break;

						case COND_GE:	
							if (valuel >= valuer)
								m_IfStat[m_IfDepth] = IF_STAT_ASSEMBLE;
							break;

						case COND_LE:	
							if (valuel <= valuer)
								m_IfStat[m_IfDepth] = IF_STAT_ASSEMBLE;
							break;

						case COND_GT:	
							if (valuel > valuer)
								m_IfStat[m_IfDepth] = IF_STAT_ASSEMBLE;
							break;

						case COND_LT:	
							if (valuel < valuer)
								m_IfStat[m_IfDepth] = IF_STAT_ASSEMBLE;
							break;
						}
					}
				}
				else
				{
					// Check bit 0 of the evaluated expression from equation 1
					if (((int) valuel) & 0x01)
						m_IfStat[m_IfDepth] = IF_STAT_ASSEMBLE;
					else
						m_IfStat[m_IfDepth] = IF_STAT_DONT_ASSEMBLE;
				}
			}
		}		
		gCond = new CCondition;
//		m_Instructions->Add(pInst);
//	}
}

/*
============================================================================
This function is called when an else directive is encountered
============================================================================
*/
void VTAssembler::directive_else()
{
	MString		err;

	// Update line number
	m_Line = (PCB).line;
	
//	CInstruction*	pInst = new CInstruction;
//	if (pInst != NULL)
//	{
//		pInst->m_ID = INST_ELSE;
//		pInst->m_Line = m_Line;
//		pInst->m_FileIndex = m_FileIndex;			// Save index of the current file
		m_LastIfElseLine = m_Line;
		m_LastIfElseIsIf = 0;
//		m_Instructions->Add(pInst);

		// Process ELSE statement during parsing.  First insure else has a matching if
		if (m_IfDepth == 0)
		{
			err.Format("Error in line %d(%s):  ELSE without a matching IF", m_Line, 
				(const char *) m_Filenames[m_FileIndex]);
			m_Errors.Add(err);
			return;
		}
		
		// Now check if the active IF statement is not a NESTED_DONT_ASSEMBLE.
		// If it isn't then change the state of the  assembly
		if (m_IfStat[m_IfDepth] == IF_STAT_ASSEMBLE)
			m_IfStat[m_IfDepth] = IF_STAT_DONT_ASSEMBLE;
		else
			if (m_IfStat[m_IfDepth] == IF_STAT_DONT_ASSEMBLE)
				m_IfStat[m_IfDepth] = IF_STAT_ASSEMBLE;
//	}
}

/*
============================================================================
This function is called when an endif directive is encountered
============================================================================
*/
void VTAssembler::directive_endif()
{
	MString		err;

	// Update line number
	m_Line = (PCB).line;
	
//	CInstruction*	pInst = new CInstruction;
//	if (pInst != NULL)
//	{
//		pInst->m_ID = INST_ENDIF;
//		pInst->m_Line = m_Line;
//		pInst->m_FileIndex = m_FileIndex;			// Save index of the current file
//		m_Instructions->Add(pInst);

		// Process ENDIF statement during parsing.  First insure else has a matching if
		if (m_IfDepth == 0)
		{
			err.Format("Error in line %d(%s):  ENDIF without a matching IF", m_Line, 
				(const char *) m_Filenames[m_FileIndex]);
			m_Errors.Add(err);
			return;
		}

		// Pop If from stack
		m_IfDepth--;
//	}
}

/*
============================================================================
This is the main assemble routine.  It is called after the parsing has
completed and generates object code for all ASEG, CSEG and DSEG segments
defined in the input file.  The object code is stored in 3 separate arrays,
one for each segment to be writen to the object file later.
============================================================================
*/
int VTAssembler::Assemble()
{
	MString			err;
	int				c, count, len, x;
	int				size;
	CInstruction*	pInst;
	CRelocation*	pRel;
	CExtern	*		pExt;
	CExpression*	pExp;
	VTObArray*		pExpList;
	MStringArray*	pNameList;
	CSymbol*		pSymbol;
	AddrRange*		pRange;
	unsigned char	opcode;
	unsigned char	op1 = 0xCC, op2 = 0xCC;
	unsigned char	type;
	unsigned short	address;
	double			value;
	char			rel_mask;
	int				valid, extern_label;
	POSITION		pos;
	MString			key;
	CSegment*		relSeg;

	/*
	========================================================================
	Set the #ifdef level to zero.  Not sure we need this
	========================================================================
	*/
	m_IfDepth = 0;
	m_IfStat[0] = IF_STAT_ASSEMBLE;

	/*
	========================================================================
	Loop through each segment and assemble 
	========================================================================
	*/
	pos = m_Segments.GetStartPosition();
	while (pos != NULL)
	{
		// Get next segment to assemble
		m_Segments.GetNextAssoc(pos, key, (VTObject *&) m_ActiveSeg);

		m_Instructions = m_ActiveSeg->m_Instructions;
		m_ActiveAddr = m_ActiveSeg->m_UsedAddr;
		m_Address = 0;

		// Get the initial module that was active during segment creation
		m_ActiveMod = m_ActiveSeg->m_InitialMod;
		m_Symbols = m_ActiveMod->m_Symbols;

		// Loop through all instructions and substitute equates and create instruction words
		count = m_Instructions->GetSize();
		for (c = 0; c < count; c++)
		{
			pInst = (CInstruction*) (*m_Instructions)[c];
			m_Line = pInst->m_Line;
			m_FileIndex = pInst->m_FileIndex;
			rel_mask = 0;
			address = m_Address;

			/*
			====================================================================
			First check if instruction is an opcode.  Handle opcodes by looking
			up the output value in the opcode table
			====================================================================
			*/
			if (pInst->m_ID < INST_ORG)
			{
				// Get base opcode value
				opcode = gOpcodeBase[pInst->m_ID];
				type = gOpcodeType[pInst->m_ID];
				valid = 1;
				extern_label = 0;

				// Modifiy base opcode based on opcode type
				switch (type)
				{
				case OPCODE_IMM:
					opcode |= atoi(*pInst->m_Operand1) << 3;
					break;

				case OPCODE_1REG:
				case OPCODE_REG_IMM:
				case OPCODE_REG_EQU16:
					opcode |= atoi(*pInst->m_Operand1) << gShift[pInst->m_ID];
					break;

				case OPCODE_2REG:
					opcode |= (intptr_t) pInst->m_Group - '0';
					opcode |= atoi(*pInst->m_Operand1) << 3;
					break;
				}

				// Now determine operand bytes
				if ((type == OPCODE_EQU8) || (type == OPCODE_EQU16) ||
					(type == OPCODE_REG_IMM) || (type == OPCODE_REG_EQU16))
				{
					// Try to get value of the equation
					if (Evaluate((CRpnEquation *) pInst->m_Group, &value, 1))
					{
						// Equation evaluated to a value.check if it is 
						// relative to a relocatable segment
						if (InvalidRelocation((CRpnEquation *) pInst->m_Group, rel_mask,
							relSeg))
						{
							// Error, equation cannot be evaluted
//							err.Format("Error in line %d(%s):  Equation depends on multiple segments",
//								pInst->m_Line, (const char *) m_Filenames[m_FileIndex]);
//							m_Errors.Add(err);
							valid = 0;
						}

						op1 = (int) value & 0xFF;
						op2 = ((int) value >> 8) & 0xFF;
					}
					else
					{
						if ((type == OPCODE_EQU8) || (type == OPCODE_REG_IMM))
							size = 1;
						else
							size = 2;
						
						// Equation does not evaluate.  Check if it is an extern
						if (EquationIsExtern((CRpnEquation *) pInst->m_Group, size))
						{
							// Add a dummy value (0) with relocation
							op1 = 0;
							op2 = 0;
							value = 0.0;
							extern_label = 1;
						}
						else
						{
							// Error, equation cannot be evaluted
//							err.Format("Error in line %d(%s):  Equation cannot be evaluated",
//								pInst->m_Line, (const char *) m_Filenames[m_FileIndex]);
//							m_Errors.Add(err);
							valid = 0;
						}
					}
				}

				// Add opcode and operands to array
				m_ActiveSeg->m_AsmBytes[m_Address++] = opcode;

				// Check if operand is PC relative
				if ((rel_mask > 1) && valid)
				{
					// Add relocation information
					pRel = new CRelocation;
					pRel->m_Address = m_Address;
					pRel->m_pSourceRange = m_ActiveAddr;
					pRel->m_Segment = relSeg;
					pRange = m_ActiveSeg->m_UsedAddr;
					while ((pRange != 0) && (pRel->m_pTargetRange == 0))
					{
						if ((pRange->address <= (int) value) &&
							(pRange->address + pRange->length > (int) value))
						{
							pRel->m_pTargetRange = pRange;
						}
						pRange = pRange->pNext;
					}
					m_ActiveSeg->m_Reloc.Add(pRel);
				}

				// Check if operand is extern
				if (extern_label)
				{
					// Add extern linkage information
					pExt = new CExtern;
					pExt->m_Address = m_Address;
					pExt->m_Name = ((CRpnEquation *)
						pInst->m_Group)->m_OperationArray[0].m_Variable;
					pExt->m_Segment = m_ActiveSeg->m_Type;
					pExt->m_Size = size;
					pExt->m_pRange = m_ActiveAddr;
					m_Externs.Add(pExt);
				}

				// Add operands to output
				if ((type == OPCODE_REG_IMM) || (type == OPCODE_EQU8) ||
					(type == OPCODE_EQU16) || (type == OPCODE_REG_EQU16))
				{
					m_ActiveSeg->m_AsmBytes[m_Address++] = op1;
				}

				if ((type == OPCODE_EQU16) || (type == OPCODE_REG_EQU16))
				{
					m_ActiveSeg->m_AsmBytes[m_Address++] = op2;
				}
			}
			

			/*
			====================================================================
			An ORG instruciton has no FBI portion, so process it separately.
			====================================================================
			*/
			else if (pInst->m_ID == INST_ORG)
			{
				// Indicate instruction address not to be reported in listing
				pInst->m_Bytes = -1;

				// The parser takes care of this instruction but we need to start a new
				// Segment in the .obj file
				if (Evaluate((CRpnEquation *) pInst->m_Group, &value, 1))
				{
					m_Address = (int) value;
					
					// Find AddrRange object for this ORG
					pRange = m_ActiveSeg->m_UsedAddr;
					while (pRange != 0)
					{
						if ((pRange->address == m_Address) || ((pRange->address < m_Address) &&
							(pRange->address + pRange->length > m_Address)))
						{
							m_ActiveSeg->m_ActiveAddr = pRange;
							m_ActiveAddr = pRange;
							break;
						}

						// Point to next AddrRange
						pRange = pRange->pNext;
					}
				}
				else
				{
//					err.Format("Error in line %d(%s):  Equation cannot be evaluated",
//						pInst->m_Line, (const char *) m_Filenames[m_FileIndex]);
//					m_Errors.Add(err);
				}
			}

			/*
			====================================================================
			Deal with FILL directive by adding bytes to the output stream
			====================================================================
			*/
			else if (pInst->m_ID == INST_FILL)
			{
				unsigned char fillVal = 0xFF;
				size = 0;

				// Get count of expression in list
				pExpList = (VTObArray *) pInst->m_Group;
				len = pExpList->GetSize();

				// Try to evaluate the fill to expression
				pExp = (CExpression *) (*pExpList)[0];
				if (Evaluate(pExp->m_Equation, &value, 1))
				{
					// Update address based on fill size
					size = (int) value;
				}

				// If 2nd expression given, try to evaluate the fill value
				if (len == 2)
				{
					// Get next expression from expression list
					pExp = (CExpression *) (*pExpList)[1];

					// If expression has an equation, try to evaluate it
					if (pExp->m_Equation != NULL)
					{
						if (Evaluate(pExp->m_Equation, &value, 1))
						{
							fillVal = (unsigned char) value;
						}
					}
				}

				// Add bytes with the fill character
				int y;
				for (y = 0; y < size; y++)
					m_ActiveSeg->m_AsmBytes[m_Address++] = fillVal;
			}

			/*
			====================================================================
			Deal with DB directive by adding bytes to the output stream
			====================================================================
			*/
			else if (pInst->m_ID == INST_DB)
			{
				// Get count of expression in list
				pExpList = (VTObArray *) pInst->m_Group;
				len = pExpList->GetSize();

				// Update address based on number of items in expression list
				for (x = 0; x < len; x++)
				{
					// Get next expression from expression list
					pExp = (CExpression *) (*pExpList)[x];

					// If expression has an equation, try to evaluate it
					if (pExp->m_Equation != NULL)
					{
						if (Evaluate(pExp->m_Equation, &value, 1))
							m_ActiveSeg->m_AsmBytes[m_Address++] = (unsigned char) value;
						else
						{
//							err.Format("Error in line %d(%s):  Equation cannot be evaluated",
//								pInst->m_Line, (const char *) m_Filenames[m_FileIndex]);
//							m_Errors.Add(err);
						}
					}
					else
					{
						// Add bytes from the string
						int y, str_len;
						str_len = pExp->m_Literal.GetLength();
						for (y = 0; y < str_len; y++)
							m_ActiveSeg->m_AsmBytes[m_Address++] = pExp->m_Literal[y];
					}
				}
			}

			/*
			====================================================================
			Deal with DW directive by adding bytes to the output stream
			====================================================================
			*/
			else if (pInst->m_ID == INST_DW)
			{
				// Get count of expression in list
				pExpList = (VTObArray *) pInst->m_Group;
				len = pExpList->GetSize();

				// Update address based on number of items in expression list
				for (x = 0; x < len; x++)
				{
					// Get next expression from expression list
					pExp = (CExpression *) (*pExpList)[x];

					// If expression has an equation, try to evaluate it
					if (pExp->m_Equation != NULL)
					{
						if (Evaluate(pExp->m_Equation, &value, 1))
						{
							// Test operand1 to check if MSFIRST is on or not
							if (*pInst->m_Operand1 == "1")
							{
								m_ActiveSeg->m_AsmBytes[m_Address++] = ((unsigned short) value) >> 8;
								m_ActiveSeg->m_AsmBytes[m_Address++] = ((unsigned short) value) & 0xFF;
							}
							else
							{
								m_ActiveSeg->m_AsmBytes[m_Address++] = ((unsigned short) value) & 0xFF;
								m_ActiveSeg->m_AsmBytes[m_Address++] = ((unsigned short) value) >> 8;
							}
						}
						else
						{
//							err.Format("Error in line %d(%s):  Equation cannot be evaluated",
//								pInst->m_Line, (const char *) m_Filenames[m_FileIndex]);
//							m_Errors.Add(err);
						}
					}
					else
					{
						// Add bytes from the string
						int y, str_len;
						str_len = pExt->m_Name.GetLength();
						for (y = 0; y < str_len; y++)
							m_ActiveSeg->m_AsmBytes[m_Address++] = pExt->m_Name[y];
						if (str_len & 1)
							m_ActiveSeg->m_AsmBytes[m_Address++] = 0;
					}
				}
			}

			/*
			====================================================================
			Deal with LABEL instruction
			====================================================================
			*/
			else if (pInst->m_ID == INST_LABEL)
			{
				m_LastLabel = *pInst->m_Operand1;
			}
			/*
			====================================================================
			Deal with DW directive by adding bytes to the output stream
			====================================================================
			*/
			else if (pInst->m_ID == INST_DS)
			{
				if (Evaluate((CRpnEquation *) pInst->m_Group, &value, 1))
				{
					// Increment address counter based on equation
					m_Address += (int) value;
				}
				else
				{
					// Report error
//					err.Format("Error in line %d(%s):  Equation cannot be evaluated",
//						pInst->m_Line, (const char *) m_Filenames[m_FileIndex]);
//					m_Errors.Add(err);
				}
			}
			/*
			====================================================================
			Deal with PUBLIC directive by insuring all PUBLIC symbols exists and
			updating the symbol table with the SYM_PUBLIC bit
			====================================================================
			*/
			else if (pInst->m_ID == INST_PUBLIC)
			{
				// Loop through each item in name list
				pNameList = (MStringArray *) pInst->m_Group;
				len = pNameList->GetSize();
				for (x = 0; x < len; x++)
				{
					// Lookup symbol in each module
					if (LookupSymbol((*pNameList)[x], pSymbol))
					{
						pSymbol->m_SymType |= SYM_PUBLIC;

						// Check if symbol is a relative label
						if ((pSymbol->m_SymType & 0x00FF) == SYM_LABEL)
						{
							m_ActiveMod->m_Publics.Add(pSymbol);
						}
					}
					else
					{
						// Report error
						err.Format("Error in line %d(%s):  Symbol %s undefined",
							pInst->m_Line, (const char *) m_Filenames[m_FileIndex], 
							(const char *) (*pNameList)[x]);
						m_Errors.Add(err);
					}
				}
				pInst->m_Bytes = -1;
			}

			/*
			====================================================================
			Deal with Module directive by changing the active module
			====================================================================
			*/
			else if (pInst->m_ID == INST_MODULE)
			{
				// Search for module name in map
				m_Modules.Lookup((const char *) pInst->m_Operand1, 
					(VTObject *&) m_ActiveMod);
				pInst->m_Bytes = -1;
			}

			// Set the instructions length in bytes based on the distance it
			// increased m_Address
			if (pInst->m_Bytes != -1)
				pInst->m_Bytes = m_Address - address;
		}

		// Here we should check if m_Address == m_ActiveSeg->m_Address as validation
	}

	// Check for errors during assembly
	if (m_Errors.GetSize() != 0)
		return 0;

	return 1;
}

/*
========================================================================
This routine creates the ELF object file.
========================================================================
*/
int VTAssembler::CreateObjFile(const char *filename)
{
	Elf32_Ehdr		ehdr;
	Elf32_Shdr		strhdr, str2hdr, symhdr, *aseg_hdr, *cseg_hdr, *dseg_hdr;
	Elf32_Shdr		*rel_hdrs, null_hdr;
	Elf32_Sym		sym;
	Elf32_Rel		rel;
	int				c, idx;
	int				aseg_sections, cseg_sections, dseg_sections;
	FILE*			fd;
	CSymbol*		pSymbol;
	CSegment*		pSegment;
	CRelocation*	pReloc;
	AddrRange*		pRange;
	CExtern*		pExt;
	MString			key, modname;
	POSITION		pos, modPos;
	char			sectString[] = "\0.strtab\0.symtab\0.aseg\0.text\0.data\0";
	char			relString[] = ".relcseg\0.reldseg\0";
	char			debugString[] = ".debug\0";
	const int		debugStrSize = 7;
	const int		strtab_off = 1;
	const int		symtab_off = 9;
	const int		aseg_off = 17;
	const int		cseg_off = 23;
	const int		dseg_off = 29;
	const int		relcseg_off = 35;
	const int		reldseg_off = 44;
	const int		extern_off = 53;
	int				len, strtab_start;
	int				first_aseg_idx, first_cseg_idx, first_dseg_idx;
	int				shidx, type, bind;
	const char *	pStr;

	// Now try to open the object file
	if ((fd = fopen(filename, "wb+")) == 0)
	{
		// Report error
		return 0;
	}

	/*
	=============================
	Add ELF header to .OBJ file
	=============================
	*/
	// Populate the ELF header
	ehdr.e_ident[EI_MAG0] = ELFMAG0;
	ehdr.e_ident[EI_MAG1] = ELFMAG1;
	ehdr.e_ident[EI_MAG2] = ELFMAG2;
	ehdr.e_ident[EI_MAG3] = ELFMAG3;
	ehdr.e_ident[EI_CLASS] = ELFCLASS32;
	ehdr.e_ident[EI_DATA] = ELFDATA2LSB;
	ehdr.e_ident[EI_VERSION] = EV_CURRENT;
	for (c = EI_PAD; c < EI_NIDENT; c++)
		ehdr.e_ident[c] = 0;
	ehdr.e_type = ET_REL;			// Relocatable object file
	ehdr.e_machine = EM_8085;		// Model 100 machine
	ehdr.e_version = EV_CURRENT;	// Version info
	ehdr.e_entry = 0;				// No entry point for .obj file
	ehdr.e_phoff = 0;				// No Program Header for .obj file
	ehdr.e_shoff = 0;				// Fill in Section Header Table offset later
	ehdr.e_flags = 0;				// No machine specific flags
	ehdr.e_ehsize = sizeof(ehdr);	// Size of this header
	ehdr.e_phentsize = 0;			// No program header table
	ehdr.e_phnum  = 0;				// No program header table
	ehdr.e_shentsize = sizeof(strhdr);// Size of each section header entry
	ehdr.e_shnum = 1;				// Initialize to 1 section - the NULL section
	ehdr.e_shstrndx = 1;			// Make the section name string table the first entry

	// Write ELF header to file
	fwrite(&ehdr, sizeof(ehdr), 1, fd);

	/*
	===================================
	Create a NULL header
	===================================
	*/
	// Create section header for string table
	null_hdr.sh_name = 0;			// Name for NULL header is zero
	null_hdr.sh_type = SHT_NULL;	// Make section NULL
	null_hdr.sh_flags = 0;			// No flags for this section
	null_hdr.sh_addr = 0;			// No address data for string table
	null_hdr.sh_offset = 0;			// No offset for NULL header
	null_hdr.sh_size = 0;			// Initialize size to zero
	null_hdr.sh_link = 0;			// Set link data to zero
	null_hdr.sh_info = 0;			// Set info byte to zero
	null_hdr.sh_addralign = 0;		// Set allignment data to zero
	null_hdr.sh_entsize = 0;		// String table does not have equal size entries

	/*
	===================================
	Create STRTAB section for .OBJ file
	===================================
	*/
	// Create section header for string table
	strhdr.sh_name = strtab_off;	// Put ".strtab" as first entry in table
	strhdr.sh_type = SHT_STRTAB;	// Make section a string table
	strhdr.sh_flags = SHF_STRINGS;	// This section contains NULL terminated strings
	strhdr.sh_addr = 0;				// No address data for string table
	strhdr.sh_offset = ftell(fd);
	strhdr.sh_link = 0;				// Set link data to zero
	strhdr.sh_info = 0;				// Set info byte to zero
	strhdr.sh_addralign = 0;		// Set allignment data to zero
	strhdr.sh_entsize = 0;			// String table does not have equal size entries
	ehdr.e_shnum++;					// Increment Section Header count

	// Start STRTAB section for section strings
	strtab_start = ftell(fd);
	fwrite(sectString, sizeof(sectString), 1, fd);
	fwrite(relString, sizeof(relString), 1, fd); 
	strhdr.sh_size = sizeof(sectString) + sizeof(relString);
	if (m_DebugInfo)
	{
		fwrite(debugString, debugStrSize, 1, fd);
		strhdr.sh_size += debugStrSize;
	}
	
	// Write section names for all other sections
	pos = m_Segments.GetStartPosition();
	while (pos != NULL)
	{
		m_Segments.GetNextAssoc(pos, key, (VTObject *&) pSegment);
		if (pSegment->m_Name == ".aseg")
			pSegment->m_sh_offset = aseg_off;
		else if (pSegment->m_Name == ".text")
			pSegment->m_sh_offset = cseg_off;
		else if (pSegment->m_Name == ".data")
			pSegment->m_sh_offset = dseg_off;
		else
		{
			// Write string to the file including NULL termination.  Also,
			// calculate the string's ofset from beginning of STRTAB section
			pStr = (const char *) pSegment->m_Name;
			pSegment->m_sh_offset = ftell(fd) - strtab_start;
			fwrite(pStr, 1, strlen(pStr) + 1, fd);
			strhdr.sh_size += strlen(pStr) + 1;
		}
	}

	/*
	=========================================================
	Now create a STRTAB section for EXTERN and PUBLIC symbols
	=========================================================
	*/
	// Create section header for string table
	str2hdr.sh_name = strtab_off;	// Put ".strtab" as first entry in table
	str2hdr.sh_type = SHT_STRTAB;	// Make section a string table
	str2hdr.sh_flags = SHF_STRINGS;	// This section contains NULL terminates strings
	str2hdr.sh_addr = 0;			// No address data for string table
	str2hdr.sh_offset = ftell(fd);
	str2hdr.sh_size = 1;			// Initialize size to zero -- fill in later
	str2hdr.sh_link = 0;			// Set link data to zero
	str2hdr.sh_info = 0;			// Set info byte to zero
	str2hdr.sh_addralign = 0;		// Set allignment data to zero
	str2hdr.sh_entsize = 0;			// String table does not have equal size entries
	ehdr.e_shnum++;					// Increment Section Header count

	// Start with a null string
	strtab_start = ftell(fd);
	fwrite(sectString, 1, 1, fd);
	modPos = m_Modules.GetStartPosition();
	while (modPos != NULL)
	{
		// Get the pointer to this module
		m_Modules.GetNextAssoc(modPos, modname, (VTObject *&) m_ActiveMod);

		// Loop through all symbols for this module
		pos = m_ActiveMod->m_Symbols->GetStartPosition();
		while (pos != 0)
		{
			// Get next symbol
			m_ActiveMod->m_Symbols->GetNextAssoc(pos, key, (VTObject *&) pSymbol);
			if ((pSymbol->m_SymType & SYM_PUBLIC) || (pSymbol->m_SymType & SYM_EXTERN))
			{
				// Add string for this symbol
				pSymbol->m_StrtabOffset = ftell(fd) - strtab_start;
				len = strlen(pSymbol->m_Name) + 1;
				fwrite(pSymbol->m_Name, len, 1, fd);
				str2hdr.sh_size += len;
			}
		}
	}

	/*
	==========================================
	Create SYMTAB header section to .OBJ file
	==========================================
	*/
	symhdr.sh_name = symtab_off;	// Put ".strtab" as first entry in table
	symhdr.sh_type = SHT_SYMTAB;	// Make section a symbol table
	symhdr.sh_flags = 0;			// No flags for this section
	symhdr.sh_addr = 0;				// No address data for symbol table
	symhdr.sh_offset = ftell(fd);
	symhdr.sh_size = 0;				// Initialize size to zero -- fill in later
	symhdr.sh_link = 0;				// Set link data to zero
	symhdr.sh_info = 0;				// Set info byte to zero
	symhdr.sh_addralign = 0;		// Set allignment data to zero
	symhdr.sh_entsize = sizeof(Elf32_Sym);// Symbol table has equal size items
	ehdr.e_shnum++;					// Increment Section Header count
	
	// Now add all EXTERN and PUBLIC symbols to string table
	idx = 0;
	modPos = m_Modules.GetStartPosition();
	while (modPos != NULL)
	{
		// Get the pointer to this module
		m_Modules.GetNextAssoc(modPos, modname, (VTObject *&) m_ActiveMod);

		pos = m_ActiveMod->m_Symbols->GetStartPosition();
		while (pos != 0)
		{
			// Get next symbol
			m_ActiveMod->m_Symbols->GetNextAssoc(pos, key, (VTObject *&) pSymbol);
			if ((pSymbol->m_SymType & SYM_PUBLIC) || (pSymbol->m_SymType & SYM_EXTERN))
			{
				// Add Elf32_Sym entry for this symbol
				sym.st_name = pSymbol->m_StrtabOffset;
				sym.st_value = pSymbol->m_Value;

				// Determine symbol Bind and type
				if (pSymbol->m_SymType & SYM_PUBLIC)
					bind = STB_GLOBAL;
				else
					bind = STB_EXTERN;
				if ((pSymbol->m_SymType & 0xFF) == SYM_LABEL)
					type = STT_FUNC;
				else
					type = STT_OBJECT;

				sym.st_info = ELF32_ST_INFO(bind, type);
				sym.st_other = 0;
				sym.st_shndx = 2;

				if ((pSymbol->m_SymType & (SYM_8BIT | SYM_16BIT)) == 0)
				{
					sym.st_size = 0;
					pSymbol->m_Off16 = idx;
				}
				if (pSymbol->m_SymType & SYM_8BIT)
				{
					sym.st_size = 1;
					pSymbol->m_Off8 = idx;
				}
				if (pSymbol->m_SymType & SYM_16BIT)
				{
					sym.st_size = 2;
					pSymbol->m_Off16 = idx;
				}

				fwrite(&sym, sizeof(sym), 1, fd);
				idx++;

				// If symbol is EXTERN, find all m_Externs and update index
				if (bind == STB_EXTERN)
				{
					len = m_Externs.GetSize();
					for (c = 0; c < len; c++)
					{
						pExt = (CExtern *) m_Externs[c];
						pStr = pExt->m_Name;
						if (pSymbol->m_Name == pExt->m_Name)
						{
							if (pExt->m_Size == 1)
								pExt->m_SymIdx = (unsigned short) pSymbol->m_Off8;
							else
								pExt->m_SymIdx = (unsigned short) pSymbol->m_Off16;
						}
					}
				}
			}
		}
	}
	// Set the symbol table size in bytes
	symhdr.sh_size = idx * sizeof(Elf32_Sym);

	/*
	==========================================
	Create section for ASEG code
	==========================================
	*/
	// First count the number of sections needed
	aseg_sections = 1;
	m_Segments.Lookup(".aseg", (VTObject *&) m_ActiveSeg);
	pRange = m_ActiveSeg->m_UsedAddr;
	while (pRange->pNext != 0)
	{
		aseg_sections++;
		pRange =pRange->pNext;
	}
	// Allocate pointers for aseg_section headers
	aseg_hdr = (Elf32_Shdr *) malloc(aseg_sections * sizeof(Elf32_Shdr));

	pRange = m_ActiveSeg->m_UsedAddr;
	idx = 0;
	first_aseg_idx = ehdr.e_shnum;
	while (pRange != NULL)
	{
		if (pRange->length == 0)
		{
			// If AddrRange is emtpy, don't write it to the .obj file
			pRange = pRange->pNext;
			aseg_hdr[idx].sh_name = 0;		// Indicate empty section
			idx++;							// Increment to next index
			continue;
		}

		aseg_hdr[idx].sh_name = aseg_off;	// Point to ".aseg" name
		aseg_hdr[idx].sh_type = SHT_PROGBITS;// Make section a Program Bits section
		// Set flag bits. Note the SHF_8085_ABSOLUTE  bit is OS specific
		aseg_hdr[idx].sh_flags = SHF_ALLOC | SHF_EXECINSTR | SHF_8085_ABSOLUTE;	
		aseg_hdr[idx].sh_addr = pRange->address;// Address for this section
		aseg_hdr[idx].sh_offset = ftell(fd);
		aseg_hdr[idx].sh_size = pRange->length;	// Get size from AddrRange
		aseg_hdr[idx].sh_link = 0;			// Set link data to zero
		aseg_hdr[idx].sh_info = 0;			// Set info byte to zero
		aseg_hdr[idx].sh_addralign = 0;		// Set allignment data to zero
		aseg_hdr[idx].sh_entsize = 0;		// Symbol table has equal size items

		// Save the index of this ASEG Section Header for relocation reference
		pRange->shidx = ehdr.e_shnum;
	
		ehdr.e_shnum++;						// Increment Section Header count

		// Write data to the file
		fwrite(&m_ActiveSeg->m_AsmBytes[pRange->address], pRange->length, 1, fd);

		// Point to next ASEG address range
		pRange = pRange->pNext;
		idx++;								// Increment to next index
	}


	/*
	==========================================
	Create section for CSEG code.  CSEGs are
	relocatable
	==========================================
	*/
	// First count the number of sections needed
	cseg_sections = 0;
	pos = m_Segments.GetStartPosition();
	while (pos != NULL)
	{
		m_Segments.GetNextAssoc(pos, key, (VTObject *&) m_ActiveSeg);
		if (m_ActiveSeg->m_Type != CSEG)
			continue;

		// Count the first address range
		cseg_sections++;
		pRange = m_ActiveSeg->m_UsedAddr;
		while (pRange->pNext != 0)
		{
			cseg_sections++;
			pRange =pRange->pNext;
		}
	}
	// Allocate pointers for cseg_section headers
	cseg_hdr = (Elf32_Shdr *) malloc(cseg_sections * sizeof(Elf32_Shdr));

	idx = 0;
	pos = m_Segments.GetStartPosition();
	first_cseg_idx = ehdr.e_shnum;
	while (pos != NULL)
	{
		// Get pointer to next segment and validate it is a CSEG type
		m_Segments.GetNextAssoc(pos, key, (VTObject *&) m_ActiveSeg);
		if (m_ActiveSeg->m_Type != CSEG)
			continue;

		// Start with first AddrRange and loop for all
		pRange = m_ActiveSeg->m_UsedAddr;
		while (pRange != NULL)
		{
			// If the address range is zero, mark this as an empty section
			if (pRange->length == 0)
			{
				// If AddrRange is emtpy, don't write it to the .obj file
				pRange = pRange->pNext;
				cseg_hdr[idx].sh_name = 0;		// Indicate empty section
				idx++;							// Increment to next index
				continue;
			}

			cseg_hdr[idx].sh_name = m_ActiveSeg->m_sh_offset;	// Point segment name
			cseg_hdr[idx].sh_type = SHT_PROGBITS;// Make section a Program Bits section
			cseg_hdr[idx].sh_flags = SHF_ALLOC | SHF_EXECINSTR;	// Set flag bits
			cseg_hdr[idx].sh_addr = pRange->address;// Address for this section
			cseg_hdr[idx].sh_offset = ftell(fd);
			cseg_hdr[idx].sh_size = pRange->length;	// Get size from AddrRange
			cseg_hdr[idx].sh_link = 0;			// Set link data to zero
			cseg_hdr[idx].sh_info = 0;			// Set info byte to zero
			cseg_hdr[idx].sh_addralign = 0;		// Set allignment data to zero
			cseg_hdr[idx].sh_entsize = 0;		// Symbol table has equal size items

			// Save the index of this CSEG Section Header for relocation reference
			pRange->shidx = ehdr.e_shnum;
			ehdr.e_shnum++;						// Increment Section Header count

			// Write data to the file
			fwrite(&m_ActiveSeg->m_AsmBytes[pRange->address], pRange->length, 1, fd);

			// Point to next ASEG address range
			pRange = pRange->pNext;
			idx++;								// Increment to next index
		}
	}

	/*
	==========================================
	Create section for DSEG code.  DSEGs are
	relocatable
	==========================================
	*/
	// First count the number of sections needed
	dseg_sections = 0;
	pos = m_Segments.GetStartPosition();
	while (pos != NULL)
	{
		m_Segments.GetNextAssoc(pos, key, (VTObject *&) m_ActiveSeg);
		if (m_ActiveSeg->m_Type != DSEG)
			continue;

		dseg_sections++;
		pRange = m_ActiveSeg->m_UsedAddr;
		while (pRange->pNext != 0)
		{
			dseg_sections++;
			pRange =pRange->pNext;
		}
	}
	// Allocate pointers for dseg_section headers
	dseg_hdr = (Elf32_Shdr *) malloc(dseg_sections * sizeof(Elf32_Shdr));

	idx = 0;
	first_dseg_idx = ehdr.e_shnum;
	pos = m_Segments.GetStartPosition();
	while (pos != NULL)
	{
		m_Segments.GetNextAssoc(pos, key, (VTObject *&) m_ActiveSeg);
		if (m_ActiveSeg->m_Type != DSEG)
			continue;

		pRange = m_ActiveSeg->m_UsedAddr;
		while (pRange != NULL)
		{
			if (pRange->length == 0)
			{
				// If AddrRange is emtpy, don't write it to the .obj file
				pRange = pRange->pNext;
				dseg_hdr[idx].sh_name = 0;		// Indicate empty section
				idx++;							// Increment to next index
				continue;
			}

			dseg_hdr[idx].sh_name = m_ActiveSeg->m_sh_offset;	// Point to segment name
			dseg_hdr[idx].sh_type = SHT_PROGBITS;// Make section a Program Bits section
			dseg_hdr[idx].sh_flags = SHF_WRITE;	// Set flag bits
			dseg_hdr[idx].sh_addr = pRange->address;// Address for this section
			dseg_hdr[idx].sh_offset = ftell(fd);
			dseg_hdr[idx].sh_size = pRange->length;	// Get size from AddrRange
			dseg_hdr[idx].sh_link = 0;			// Set link data to zero
			dseg_hdr[idx].sh_info = 0;			// Set info byte to zero
			dseg_hdr[idx].sh_addralign = 0;		// Set allignment data to zero
			dseg_hdr[idx].sh_entsize = 0;		// Symbol table has equal size items

			// Save the index of this CSEG Section Header for relocation reference
			pRange->shidx = ehdr.e_shnum;
			
			ehdr.e_shnum++;						// Increment Section Header count

			// Write data to the file
			fwrite(&m_ActiveSeg->m_AsmBytes[pRange->address], pRange->length, 1, fd);

			// Point to next ASEG address range
			pRange = pRange->pNext;
			idx++;								// Increment to next index
		}
	}

	/*
	=======================================
	Add relocation sections to .OBJ file
	=======================================
	*/
	rel_hdrs = (Elf32_Shdr *) malloc((dseg_sections + 
		cseg_sections) * sizeof(Elf32_Shdr));
	idx = 0;
	pos = m_Segments.GetStartPosition();
	while (pos != NULL)
	{
		m_Segments.GetNextAssoc(pos, key, (VTObject *&) m_ActiveSeg);
		if (m_ActiveSeg->m_Type == ASEG)
			continue;

		pRange = m_ActiveSeg->m_UsedAddr;
		while (pRange != NULL)
		{
			// Set the relative section's name offset based on type
			rel_hdrs[idx].sh_name = m_ActiveSeg->m_sh_offset;
			rel_hdrs[idx].sh_type = SHT_REL;	// Make section a relocation section
			rel_hdrs[idx].sh_flags = 0;			// No flag bits for relocation section
			rel_hdrs[idx].sh_addr = 0;			// No address specifier for relocaton section
			rel_hdrs[idx].sh_offset = ftell(fd);// Fill in offset later
			rel_hdrs[idx].sh_size = 0;			// Fill in size later
			rel_hdrs[idx].sh_link = 3;			// Point to symtab section
			rel_hdrs[idx].sh_info = pRange->shidx;
			rel_hdrs[idx].sh_addralign = 0;		// No alignment requirements
			rel_hdrs[idx].sh_entsize = sizeof(Elf32_Rel);

			// Generage Elf32_Rel object for each relocation relative to this section
			len = m_ActiveSeg->m_Reloc.GetSize();

			// Loop through all relocation items in seg
			for (c = 0; c < len; c++)
			{
				// Check if this relocation relative to current section
				pReloc = (CRelocation *) m_ActiveSeg->m_Reloc[c];
				if (pReloc->m_pTargetRange == pRange)
				{	
					if (pReloc->m_pSourceRange->shidx >= first_dseg_idx)
					{
						shidx = pReloc->m_pSourceRange->shidx - first_dseg_idx;
						rel.r_offset = pReloc->m_Address - dseg_hdr[shidx].sh_addr + 
							dseg_hdr[shidx].sh_offset;
						rel.r_info = ELF32_R_INFO(0, SR_ADDR_XLATE); 
					}
					else if (pReloc->m_pSourceRange->shidx >= first_cseg_idx)
					{
						shidx = pReloc->m_pSourceRange->shidx - first_cseg_idx;
						rel.r_offset = pReloc->m_Address - cseg_hdr[shidx].sh_addr + 
							cseg_hdr[shidx].sh_offset;
						rel.r_info = ELF32_R_INFO(0, SR_ADDR_XLATE); 
					}
					else
					{
						shidx = pReloc->m_pSourceRange->shidx - first_aseg_idx;
						rel.r_offset = pReloc->m_Address - aseg_hdr[shidx].sh_addr + 
							aseg_hdr[shidx].sh_offset;
						rel.r_info = ELF32_R_INFO(0, SR_ADDR_XLATE); 
					}

					// Update size of header
					rel_hdrs[idx].sh_size += sizeof(Elf32_Rel);

					// Write entry to file
					fwrite(&rel, sizeof(Elf32_Rel), 1, fd);
				}
			}

			// Loop through externs
			len = m_Externs.GetSize();
			for (c = 0; c < len; c++)
			{
				// Get next extern from array
				pExt = (CExtern *) m_Externs[c];

				// Check if extern is for this address range
				if (pExt->m_pRange == pRange)
				{
					// Set address for this relocation
					rel.r_offset = pExt->m_Address;
					rel.r_info = ELF32_R_INFO(pExt->m_SymIdx, SR_EXTERN);

					// Update size of header
					rel_hdrs[idx].sh_size += sizeof(Elf32_Rel);

					// Write entry to file
					fwrite(&rel, sizeof(Elf32_Rel), 1, fd);
				}
			}

			// Search for PUBLIC symbols in this section
			len = m_ActiveMod->m_Publics.GetSize();
			for (c = 0; c < len; c++)
			{
				pSymbol = (CSymbol *) m_ActiveMod->m_Publics[c];

				rel.r_offset = 0;
				if (pSymbol->m_SymType & SYM_8BIT)
					rel.r_info = ELF32_R_INFO(pSymbol->m_Off8, SR_PUBLIC);
				else
					rel.r_info = ELF32_R_INFO(pSymbol->m_Off16, SR_PUBLIC);

				// Update size of header
				rel_hdrs[idx].sh_size += sizeof(Elf32_Rel);

				// Write entry to file
				fwrite(&rel, sizeof(Elf32_Rel), 1, fd);
			}

			// Check if anything actually written for this header.  If not, we will
			// omit writing header to section header table and therefore will not
			// update the Elf header section header count
			if (rel_hdrs[idx].sh_size != 0)
				ehdr.e_shnum++;					// Increment Section Header count

			// Update pRange
			pRange = pRange->pNext;	
			idx++;								// Increment to next index
		}
	}

	/*
	=======================================
	Add debug sections to .OBJ file
	=======================================
	*/
	if (m_AsmOptions.Find((char *) "-g") >= 0)
	{
	}

	/*
	=======================================
	Write Section Header table to .OBJ file
	=======================================
	*/
	// Get offset of Section Header Table for ELF header update
	ehdr.e_shoff = ftell(fd);

	// Write the NULL Section header
	fwrite(&null_hdr, sizeof(null_hdr), 1, fd);

	// Write String Table header
	fwrite(&strhdr, sizeof(strhdr), 1, fd);

	// Write String Table 2 header
	fwrite(&str2hdr, sizeof(str2hdr), 1, fd);

	// Write Symbol Table header
	fwrite(&symhdr, sizeof(symhdr), 1, fd);

	// Write ASEG headers
	for (c = 0; c < aseg_sections; c++)
	{
		if (aseg_hdr[c].sh_name != 0)
			fwrite(&aseg_hdr[c], sizeof(Elf32_Shdr), 1, fd);
	}

	// Write CSEG headers
	for (c = 0; c < cseg_sections; c++)
	{
		if (cseg_hdr[c].sh_name != 0)
			fwrite(&cseg_hdr[c], sizeof(Elf32_Shdr), 1, fd);
	}

	// Write DSEG headers
	for (c = 0; c < dseg_sections; c++)
	{
		if (dseg_hdr[c].sh_name != 0)
			fwrite(&dseg_hdr[c], sizeof(Elf32_Shdr), 1, fd);
	}

	// Write Relocation section headers
	for (c = 0; c < cseg_sections + dseg_sections; c++)
	{
		// Skip writing empty headers
		if (rel_hdrs[c].sh_size == 0)
			continue;

		fwrite(&rel_hdrs[c], sizeof(Elf32_Shdr), 1, fd);
	}

	// Write Debug section headers

	// Seek to beginning and update Elf Header
	fseek(fd, SEEK_SET, 0);
	fwrite(&ehdr, sizeof(ehdr), 1, fd);

	// Close the .OBJ file
	fclose(fd);

	// Free memory used during build
	free(aseg_hdr);
	free(cseg_hdr);
	free(dseg_hdr);
	free(rel_hdrs);

	return 1;
}

/*
========================================================================
This routine checks the specified equation to determine if it depends on
more than one segment.
========================================================================
*/
int VTAssembler::InvalidRelocation(CRpnEquation *pEq, char &rel_mask,
	CSegment *&pSeg)
{
	int		c, count, invalid;
	char	rel[3];
	CSymbol	*pSym;

	// Initialize to not invalid. If the equation references labels
	// in more than just one segment, we will mark the equation invalid
	// for relocation.
	invalid = FALSE;
	count = pEq->m_OperationArray.GetSize();
	for (c = 0; c < 3; c++)
		rel[c] = 0;

	pSeg = NULL;
	for (c = 0; c < count; c++)
	{
		// Check if operation type is a variable
		if (pEq->m_OperationArray[c].m_Operation == RPN_VARIABLE)
		{
			// Find the symbol
			if (LookupSymbol(pEq->m_OperationArray[c].m_Variable, pSym))
			{
				if ((pSym->m_SymType & 0x00FF) == SYM_LABEL)
				{
					if (pSym->m_SymType & SYM_CSEG)
						rel[CSEG] = 1;
					else if (pSym->m_SymType & SYM_DSEG)
						rel[DSEG] = 1;
					else
						rel[ASEG] = 1;

					// Assign or test the segment to ensure all are the same
					if (pSeg == NULL)
						pSeg = pSym->m_Segment;
					else
					{
						if (pSeg != pSym->m_Segment)
							invalid = TRUE;
					}
				}
			}
		}
	}

	// Update rel_mask based on which types of segments referenced
	rel_mask = rel[0] | (rel[1] << 1) | (rel[2] << 2);
	return invalid;
}

/*
========================================================================
This routine checks the specified equation to determine if it depends on
more than one segment.
========================================================================
*/
int VTAssembler::EquationIsExtern(CRpnEquation* pEq, int size)
{
	int			count;
	CSymbol*	pSym;

	// Get count of operation array
	count = pEq->m_OperationArray.GetSize();

	// Check if quation has exactly 1 operation
	if (count == 1)
	{
		// Check if operation is a variable
		if (pEq->m_OperationArray[0].m_Operation == RPN_VARIABLE)
		{
			if (LookupSymbol(pEq->m_OperationArray[0].m_Variable, pSym))
			{
				// Check if variable type s EXTERN
				if (pSym->m_SymType == SYM_EXTERN)
				{
					if (size == 1)
						pSym->m_SymType |= SYM_8BIT;
					if (size == 2)
						pSym->m_SymType |= SYM_16BIT;
					return 1;
				}
			}
		}
	}
	return 0;
}

/*
============================================================================
This function is called to parse an input file.  If there are no errors
during parsing, the routine calls the routines to assemlbe and generate
output files for .obj, .lst, .hex, etc.
============================================================================
*/
void VTAssembler::ParseExternalDefines(void)
{
	int			startIndex, endIndex;
	MString		def;
	int			len;

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

		// Now assign this define
		CSymbol*	pSymbol = new CSymbol;
		if (pSymbol != NULL)
		{
			pSymbol->m_Name = def;
			pSymbol->m_Line = -1;
			pSymbol->m_SymType = SYM_DEFINE;
			pSymbol->m_pRange = NULL;
			pSymbol->m_FileIndex = -1;

			// Assign symbol to active segment
			const char *pStr = (const char *) pSymbol->m_Name;
			(*m_Symbols)[pStr] = pSymbol;
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
This function is called to parse an input file.  If there are no errors
during parsing, the routine calls the routines to assemlbe and generate
output files for .obj, .lst, .hex, etc.
============================================================================
*/
void VTAssembler::Parse(MString filename)
{
	int		success = 1;
	int		lastDirMark;
	MString	outfile, errMsg;
	MString	temp;


	// Clean up the assembler files from previous assembler
	ResetContent();

	// Assign the file's directory for searching include files
	lastDirMark = filename.ReverseFind('/');
	if (lastDirMark == -1)
		lastDirMark = filename.ReverseFind('\\');
	if (lastDirMark != -1)
	{
		m_FileDir = filename.Left(lastDirMark + 1);
	}

	// Assign external defines
	ParseExternalDefines();

	// Try to assemble the file
	if (success = ParseASMFile(filename, this))
	{
		// Test if #ifdef stack is zero (mis-matched if / else / end
		if (m_IfDepth != 0)
		{
			if (m_LastIfElseIsIf)
				errMsg.Format("Error in line %d(%s):  #ifdef without matching #endif", 
					m_Line, (const char *) m_Filenames[m_FileIndex]);
			else
				errMsg.Format("Error in line %d(%s):  #else without matching #endif", 
					m_Line, (const char *) m_Filenames[m_FileIndex]);
			m_Errors.Add(errMsg);
		}

		// No parse errors!  Try to assemble
		else if (success = Assemble())
		{
			// Get Output filename
			temp = filename.Right(4);
			temp.MakeLower();
			if ((strcmp(temp, ".asm") == 0) || (strcmp(temp, "a85") == 0))
			{
				outfile = filename.Left(filename.GetLength()-4);
			}
			else
				outfile = filename;

			// Append .obj to filename
			temp = outfile;
			outfile += (char *) ".obj";

			// Generate the object file
			CreateObjFile(outfile);

			// Generate a listing file if requested
			outfile = temp + (char *) ".lst";
			CreateList(outfile, filename);

			// Generate a hex file if requested
			outfile = temp + (char *) ".hex";
			CreateHex(outfile);
		}
	}
}

/*
============================================================================
This function creates a listing file if one was requested.
============================================================================
*/
void VTAssembler::CreateList(MString& filename, MString& asmFilename)
{
	MString			outfile;
	FILE*			fd;
	FILE*			inFd;
	int				lineNo = 1;
	int				c;				// Segment this line was found in
	char *			pRead;
	char		    lineBuf[256];
	CInstruction*	pInst;			// Next instruction pointer
	int				firstInst;
	CSegLines*		pSegLines;		// Pointer to the current SegLines object
	int				segLinesCnt;	// Number of segLines objects
	int				segLines;		// Index of active SegLines
	CSegment*		pSeg;
	POSITION		pos;
	MString			key;

	// Test if listing requested
	if (!((m_AsmOptions.Find((char *) "-t") != -1) || (m_List)))
	{
		// No listing requested...return
		return;
	}

	// Open the output listing file
	if ((fd = fopen(filename, "wb+")) == NULL)
		return;

	// Open the input file
	if ((inFd = fopen(asmFilename, "rb")) == NULL)
	{
		// Unable to open input file
		fclose(fd);
		return;
	}

	// Initialize the pInst m_Count and current pInst m_Index for each segment
	segLinesCnt = m_SegLines.GetSize();
	pos = m_Segments.GetStartPosition();
	while (pos != NULL)
	{
		m_Segments.GetNextAssoc(pos, key, (VTObject *&) pSeg);
		pSeg->m_Count = pSeg->m_Instructions->GetSize();
		pSeg->m_Index = 0;
	}

	// Get the first SegLines object
	segLines = 0;
	pSegLines = (CSegLines *) m_SegLines[0];
	pSeg = pSegLines->pSegment;

	if (pSeg->m_Count != 0)
	{
		pInst = (CInstruction*) (*pSeg->m_Instructions)[0];
		while ((pInst != NULL) && (pInst->m_ID == INST_LABEL))
		{
			if (++pSeg->m_Index < pSeg->m_Count)
				pInst = (CInstruction *) (*pSeg->m_Instructions)[pSeg->m_Index];
			else
				pInst = NULL;
		}
	}	
	else
		pInst = NULL;

    // Read lines from the input file and locate them 
	while (TRUE)
	{
		// Read the line from the file
		pRead = fgets(lineBuf, sizeof(lineBuf), inFd);
		if (pRead == NULL)
		{
			break;
		}

		// If the line has a segment entry, then print the address and 
		// opcode / data information for this line
		if ((pInst != NULL) && (pInst->m_Line == lineNo))
		{
			firstInst = TRUE;
			while ((pInst != NULL) && (pInst->m_Line == lineNo))
			{
				// Don't print the address if m_Bytes is -1
				if (pInst->m_Bytes != -1)
				{
					if (firstInst)
					{
						// Segment found.  Print address
						fprintf(fd, "%04X  ", pInst->m_Address);

						// Print bytes on 1st line.  Up to 4 bytes
						for (int x = 0; x < 4; x++)
						{
							if (x < pInst->m_Bytes)
								fprintf(fd, "%02X ", pSeg->m_AsmBytes
									[pInst->m_Address + x]);
							else
								fprintf(fd, "   ");
						}

						// Write line to output file
						fprintf(fd, "  ");
						fputs(lineBuf, fd);
					}
					else
					{
						// Segment found.  Print address
						fprintf(fd, "%04X  ", pInst->m_Address);

						// Not the first instruction on this line.  Just print the bytes
						// Print bytes on 1st line.  Up to 4 bytes
						for (int x = 0; x < 4; x++)
						{
							if (x < pInst->m_Bytes)
								fprintf(fd, "%02X ", pSeg->m_AsmBytes[pInst->m_Address + x]);
							else
								fprintf(fd, "   ");
						}
						fprintf(fd, LINE_ENDING);
					}

					// Print any remaining bytes (dw or db statement)
					for (c = 4; c < pInst->m_Bytes; c+= 4)
					{
						if (pInst->m_ID == INST_FILL)
						{
							if (c == 4)
							{
								if (pInst->m_Bytes > 12)
								{
									fprintf(fd, "...%s", LINE_ENDING);
									continue;
								}
							}
							else if (c + 4 < pInst->m_Bytes)
								continue;
						}

						// Segment found.  Print address
						fprintf(fd, "%04X  ", pInst->m_Address + c);

						// Not the first instruction on this line.  Just print the bytes
						// Print bytes on 1st line.  Up to 4 bytes
						for (int x = 0; x < 4; x++)
						{
							if (x + c < pInst->m_Bytes)
								fprintf(fd, "%02X ", pSeg->m_AsmBytes[pInst->m_Address + x + c]);
							else
								fprintf(fd, "   ");
						}
						fprintf(fd, LINE_ENDING);
					}

					firstInst = FALSE;
				}
				else
				{
					// Write line to output file
					fprintf(fd, "                    "); 
					fputs(lineBuf, fd);
				}

				// Increment to next instruction for this segment
				pSeg->m_Index++;
				if (pSeg->m_Index < pSeg->m_Count)
				{
					pInst = (CInstruction*) (*pSeg->m_Instructions)[pSeg->m_Index];
					while ((pInst != NULL) && (pInst->m_ID == INST_LABEL))
					{
						if (++pSeg->m_Index < pSeg->m_Count)
							pInst = (CInstruction *) (*pSeg->m_Instructions)[pSeg->m_Index];
						else
							pInst = NULL;
					}
				}	
				else
					pInst = NULL;
			}
		}
		else
		{
			// Write line to output file
			fprintf(fd, "                    "); 
			fputs(lineBuf, fd);
		}

		// Append .lst to filename
		outfile += (char *) ".lst";

		// Increment the line number
		lineNo++;

		// Test if the new lineNo causes us to change to a new segment
		if ((pSegLines->lastLine != -1) && (lineNo > pSegLines->lastLine))
		{
			// Increment to the next m_Index in m_SegLines
			if (++segLines >= segLinesCnt)
			{
				MString err;
				err.Format("Error in line %d(%s):  Internal parse error - Segment line mismatch", 
					lineNo,	(const char *) m_Filenames[m_FileIndex]);
				m_Errors.Add(err);
				break;
			}

			// Update pSegLines and pSeg plus pInst structures
			pSegLines = (CSegLines *) m_SegLines[segLines];
			pSeg = pSegLines->pSegment;

			// Get the pInst for this module
			if (pSeg->m_Index < pSeg->m_Count)
			{
				while ((pInst != NULL) && (pInst->m_ID == INST_LABEL))
				{
					if (++pSeg->m_Index < pSeg->m_Count)
						pInst = (CInstruction *) (*pSeg->m_Instructions)[pSeg->m_Index];
					else
						pInst = NULL;
				}
			}
			else
				pInst = NULL;
		}
	}

	/* Close the input and output file */
	fclose(fd);
	fclose(inFd);
}

/*
============================================================================
This function sets the type of project being built (.CO, ROM, BASIC, etc.)
============================================================================
*/
void VTAssembler::SetProjectType(int type)
{
	m_ProjectType = type;
}

/*
============================================================================
This function creates an Intel HEX output file of the assembled file
============================================================================
*/
void VTAssembler::CreateHex(MString& filename)
{
	unsigned short		start, len;
	MString				outfile;
	FILE*				fd;
	CSegment*			pSeg;

	// Test if HEX output requested
	if (!((m_AsmOptions.Find((char *) "-h") != -1) || (m_Hex)))
	{
		// No ASEG Hex output requested...return
		return;
	}
	
	if (m_ProjectType == VT_PROJ_TYPE_ROM)
	{
		start = 0;
		len = 32768;
	}
	else
	{
		start = 32768;
		len = 16384;
	}

	// Open the output listing file
	if ((fd = fopen(filename, "wb+")) == NULL)
		return;

	// Save the Intel Hex file
	m_Segments.Lookup(".aseg", (VTObject *&) pSeg);
	save_hex_file_buf((char *) pSeg->m_AsmBytes, start, start + len, fd);

	fclose(fd);
}

/*
=====================================================================
Implement the CModule class.  This class is used to keep track of 
assembly segments that are broken down into modules.
=====================================================================
*/
CModule::CModule(const char *name)
{
	// Initialze the variables
	m_Name = name;
	m_Symbols = new VTMapStringToOb;
}

CModule::~CModule()
{
	CSymbol*		pSymbol;
	POSITION		pos;
	MString			key;

	// Delete symbols
	pos = m_Symbols->GetStartPosition();
	while (pos != NULL)
	{
		m_Symbols->GetNextAssoc(pos, key, (VTObject *&) pSymbol);
		delete pSymbol;
	}
	m_Symbols->RemoveAll();
	delete m_Symbols;

	// Delete publics.  The actual sybols are already deleted, so just the array
	m_Publics.RemoveAll();
}

CInstruction::~CInstruction()
{
	if (m_Operand1 != 0)
		delete m_Operand1;

	// We used m_Group to hold reg2.  Return without trying to delete m_Group
	if (m_ID == OPCODE_MOV)
		return;

	if (m_Group != 0)
	{
		if ((m_ID == INST_PUBLIC) || (m_ID == INST_EXTERN))
		{
			MStringArray *pNameList = (MStringArray *) m_Group;
			delete pNameList;
		}
		else if (strcmp(m_Group->GetClass()->m_ClassName, "CRpnEquation") == 0)
		{
			CRpnEquation *pEq = (CRpnEquation *) m_Group;
			delete pEq;
		}
		else if (strcmp(m_Group->GetClass()->m_ClassName, "CCondition") == 0)
		{
			CCondition *pCond = (CCondition *) m_Group;
			delete pCond;
		}
		else if (strcmp(m_Group->GetClass()->m_ClassName, "VTObArray") == 0)
		{
			// Delete all objects in array
			int c, count;
			VTObArray*	pArray = (VTObArray *) m_Group;
			count = pArray->GetSize();
			for (c = 0; c < count; c++)
			{
				VTObject * pOb = pArray->GetAt(c);
				delete pOb;
			}
			delete pArray;
		}
		else
		{
			char str[30];
			strcpy(str, m_Group->GetClass()->m_ClassName);
		}
	}
}

void VTAssembler::SetAsmOptions(const MString& options)
{
	// Set the options string in case we need it later
	m_AsmOptions = options;

	// Parse the options later during assembly
}

void VTAssembler::SetIncludeDirs(const MString& dirs)
{
	// Save the string in case we need it later
	m_IncludePath = dirs;

	// Recalculate the include directories
	CalcIncludeDirs();
}

void VTAssembler::SetDefines(const MString& defines)
{
	// Save the string in case we need it later
	m_ExtDefines = defines;
}

void VTAssembler::SetRootPath(const MString& path)
{
	// Save the string in case we need it later
	m_RootPath = path;

	// Recalculate the include directories
	CalcIncludeDirs();
}

void VTAssembler::CalcIncludeDirs(void)
{
	char*		pStr;
	char*		pToken;
	MString		temp;

	m_IncludeDirs.RemoveAll();

	// Check if there is an include path
	if (m_IncludePath.GetLength() == 0)
		return;

	// Copy path to a char array so we can tokenize
	pStr = new char[m_IncludePath.GetLength() + 1];
	strcpy(pStr, (const char *) m_IncludePath);

	pToken = strtok(pStr, ",;");
	while (pToken != NULL)
	{
		// Generate the path
		temp = pToken;
		temp.Trim();
		temp += (char *) "/";
		temp = m_RootPath + (char *) "/" + temp;

		// Now add it to the array
		m_IncludeDirs.Add(temp);

		// Get next token
		pToken = strtok(NULL, ",;");
	}

	delete pStr;
}

CSegment::CSegment(const char *name, int type, CModule* initialMod)
{
	m_Name = name;
	m_Type = type;
	m_InitialMod = initialMod;
	m_LastMod = initialMod;
	m_InstIndex = 0;
	m_Instructions = new VTObArray;
	m_Address = 0;
	m_UsedAddr = new AddrRange;
	m_Index = m_Count = 0;

	// Initialize the active Addr range to the first in the list
	m_ActiveAddr = m_UsedAddr;
	m_ActiveAddr->address = 0;
	m_ActiveAddr->length = 0;
	m_ActiveAddr->pNext = 0;
	m_ActiveAddr->shidx = 0;
}

CSegment::~CSegment()
{
	int				c, size;
	CInstruction*	pInst;
	AddrRange*		pRange;
	AddrRange*		pNext;
	CRelocation*	pReloc;

	// Delete the instructions
	size = m_Instructions->GetSize();
	for (c = 0; c < size; c++)
	{
		pInst = (CInstruction *) (*m_Instructions)[c];
		delete pInst;
	}
	m_Instructions->RemoveAll();
	delete m_Instructions;

	// Delete tha used address range structs
	pRange = m_UsedAddr;
	while (pRange != 0)
	{
		pNext = pRange->pNext;
		delete pRange;
		pRange = pNext;
	}

	// Delete the relocation objects
	size = m_Reloc.GetSize();
	for (c = 0; c < size; c++)
	{
		pReloc = (CRelocation *) m_Reloc[c];
		delete pReloc;
	}
	m_Reloc.RemoveAll();
}

/*
============================================================================
Implement the CInstruction object dynamic constructs
============================================================================
*/
IMPLEMENT_DYNCREATE(CInstruction, VTObject);
IMPLEMENT_DYNCREATE(CCondition, VTObject);
IMPLEMENT_DYNCREATE(CMacro, VTObject);

CMacro::~CMacro()
{
	// Delete equation if we have one
	if (m_DefList != 0) 
	{
		int size = m_DefList->GetSize();
		for (int c = 0; c < size; c++)
		{
			CExpression* pExp = (CExpression *) m_DefList->GetAt(c);
			delete pExp;
		}
		delete m_DefList;
	}
	
	// Delete expression list if we have one
	if (m_ParamList != 0) 
	{
		int size = m_ParamList->GetSize();
		for (int c = 0; c < size; c++)
		{
			CExpression* pExp = (CExpression *) m_ParamList->GetAt(c);
			delete pExp;
		}
		delete m_ParamList;
	}
}
