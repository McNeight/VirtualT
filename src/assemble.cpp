/*
========================================================================
VTAssembler:		This class defines an 8085 Macro Assembler for the
					VirtualT project.

========================================================================
*/

#include		"VirtualT.h"
#include		"assemble.h"
#include		"a85parse.h"
#include		"elf.h"
#include		<FL/Fl.H>
#include		<math.h>
#include		<string.h>
#include		<stdio.h>
#include		<stdlib.h>

extern	CRpnEquation*	gEq;
extern	VTObArray*		gExpList;
extern	MStringArray*	gNameList;
extern	CCondition*		gCond;
extern	char	reg[10];
extern	int		reg_cnt;

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
	OPCODE_NOARG, OPCODE_EQU16, OPCODE_EQU16, OPCODE_NOARG,		// 73-76
	OPCODE_EQU16, OPCODE_EQU8,  OPCODE_EQU8,  OPCODE_EQU8,		// 77-80
	OPCODE_EQU8,  OPCODE_EQU8,  OPCODE_EQU8,  OPCODE_EQU8,		// 81-84
	OPCODE_EQU8,  OPCODE_IMM,   OPCODE_EQU16, OPCODE_NOARG,		// 85-88
	OPCODE_NOARG												// 89
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
	0x76														// 89
};

unsigned char gShift[20] = { 0,
	4, 4, 4, 4, 4, 3, 3, 3,										// 1-8
	0, 0, 0, 0, 0, 0, 0, 0,										// 9-16
	4, 4, 4
};

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

	m_ActiveMod = new CModule;

	m_Symbols = m_ActiveMod->m_Symbols[ASEG];
	m_Instructions = m_ActiveMod->m_Instructions[ASEG];
	m_ActiveAddr = m_ActiveMod->m_ActiveAddr[ASEG];

	// Initialize IF stack
	m_IfDepth = 0;
	m_IfStat[0] = IF_STAT_ASSEMBLE;
}

VTAssembler::~VTAssembler()
{
	ResetContent();

	delete m_ActiveMod;
}

void VTAssembler::ResetContent(void)
{
	MString		key;
	POSITION	pos;
	int			c, count, seg;

	// Loop through each segment
	for (seg = 0; seg < 3; seg++)
	{
		// Delete all symbols
		pos = m_ActiveMod->m_Symbols[seg]->GetStartPosition();
		while (pos != NULL)
		{
			CSymbol* pSymbol;
			m_ActiveMod->m_Symbols[seg]->GetNextAssoc(pos, key, (VTObject*&) pSymbol);
			delete pSymbol;
		}
		m_ActiveMod->m_Symbols[seg]->RemoveAll();

		// Delete all Instrucitons
		count = m_ActiveMod->m_Instructions[seg]->GetSize();
		for (c = count - 1; c >= 0; c--) 
		{
			CInstruction* pInst = (CInstruction*) (*m_ActiveMod->m_Instructions[seg])[c];
			delete pInst;
		}
		m_ActiveMod->m_Instructions[seg]->RemoveAll();

		// Delete all Relocation objects
		count = m_Reloc[seg].GetSize();
		for (c = 0; c < count; c++)
		{
			CRelocation* pReloc = (CRelocation *) m_Reloc[seg][c];
			delete pReloc;
		}
		m_Reloc[seg].RemoveAll();

		// Remove all items from Publics array.  The actual items are duplicates
		// and will be deleted elsewhere
		m_Publics[seg].RemoveAll();

		// Delete the CModule and create a new one
		delete m_ActiveMod;
		m_ActiveMod = new CModule;
		m_Symbols = m_ActiveMod->m_Symbols[ASEG];
		m_Instructions = m_ActiveMod->m_Instructions[ASEG];
		m_ActiveAddr = m_ActiveMod->m_ActiveAddr[ASEG];
	}


	// Delete all Extern reference object
	count = m_Externs.GetSize();
	for (c = 0; c < count; c++)
	{
		CExtern* pExt = (CExtern *) m_Externs[c];
		delete pExt;
	}
	m_Externs.RemoveAll();

	// Delete all undefined symbols
	m_UndefSymbols.RemoveAll();

	// Delete all errors
	m_Errors.RemoveAll();

	// Delete filenames parsed
	m_Filenames.RemoveAll();
	m_FileIndex = -1;

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
	int					c, seg, local;
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
				if ((temp[0] == '$') && (temp[1] != 0))
				{
					temp = m_LastLabel + "%%" + op.m_Variable;
					local = 1;
				}
			}
			pStr = (const char *) temp;
			symbol = (CSymbol *) 0;
			for (seg = 0; seg < 3; seg++)
			{
				if (m_ActiveMod->m_Symbols[seg]->Lookup(pStr, (VTObject *&) symbol))
					break;
			}
			if (symbol != (CSymbol*) 0)
			{
				// If symbol has no vaule, try to get it
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
							errMsg.Format("Error in line %d(%s):  Local symbol %s undefined", m_Line,
								(const char *) m_Filenames[m_FileIndex], (const char *) op.m_Variable);
						else
						{
							// Check if AuotExtern is enabled
							if (m_AsmOptions.Find("-e") != -1)
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
					errMsg.Format("Error in line %d(%s):  Divide by zero!", m_Line, (const char *) m_Filenames[m_FileIndex]);
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

		case RPN_LOG:
			s1 = stack[--stk];
			stack[stk++] = log10(s1);
			break;

		case RPN_SQRT:
			s1 = stack[--stk];
			stack[stk++] = sqrt(s1);
			break;
		}
	}

	// Get the result from the equation and return to calling function
	*value = stack[--stk];

	return 1;
}

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
	if ((string[0] == '$') && (string[1] != 0))
	{
		myStr = m_LastLabel + "%%" + string;
	}
	for (c = 0; c < 3; c++)
	{
		if (m_ActiveMod->m_Symbols[c]->Lookup(myStr, (VTObject*&) pSymbol))
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
	}

	return 0;
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

	// Update line number
	m_Line = (PCB).line;
	
	// Read operand (register) from string accumulator
	CInstruction*	pInst = new CInstruction;
	if (pInst != NULL)
	{
		pInst->m_ID = opcode;						// Set opcode value
		pInst->m_Line = m_Line;						// Get Line number
		pInst->m_Address = m_Address++;				// Get current program address
		m_ActiveAddr->length++;						// Update add range length
		pInst->m_FileIndex = m_FileIndex;			// Save index of the current file
		m_Instructions->Add(pInst);					// Save instruction 
	}
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

	// Update line number
	m_Line = (PCB).line;
	
	// Read operand (register) from register stack
	CInstruction*	pInst = new CInstruction;
	if (pInst != NULL)
	{
		pInst->m_ID = opcode;						// Set opcode value
		pInst->m_Operand1 = new MString;			// Allocte operand object
		pInst->m_Operand1->Format("%c", reg[--reg_cnt]);	// Get register operand
		pInst->m_Line = m_Line;						// Get Line number
		pInst->m_Address = m_Address++;				// Get current program address
		m_ActiveAddr->length++;						// Update add range length
		pInst->m_FileIndex = m_FileIndex;			// Save index of the current file
		m_Instructions->Add(pInst);					// Save instruction 
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

	// Update line number
	m_Line = (PCB).line;
	
	// Read operand (register) from register stack
	CInstruction*	pInst = new CInstruction;
	if (pInst != NULL)
	{
		pInst->m_ID = opcode;						// Set opcode value
		pInst->m_Operand1 = new MString;			// Allocte operand object
		pInst->m_Operand1->Format("%c", c);			// Get register operand
		pInst->m_Line = m_Line;						// Get Line number
		pInst->m_Address = m_Address++;				// Get current program address
		m_ActiveAddr->length++;						// Update add range length
		pInst->m_FileIndex = m_FileIndex;			// Save index of the current file
		m_Instructions->Add(pInst);					// Save instruction 
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

	// Update line number
	m_Line = (PCB).line;
	
	// Read operands (2 registers) from Register stack
	CInstruction*	pInst = new CInstruction;
	if (pInst != NULL)
	{
		pInst->m_ID = opcode;						// Set opcode value
		pInst->m_Group = (VTObject *) (int) reg[--reg_cnt];	// Get register operand
		pInst->m_Operand1 = new MString;			// Allocte operand object
		pInst->m_Operand1->Format("%c", reg[--reg_cnt]);	// Get register operand
		pInst->m_Line = m_Line;						// Get Line number
		pInst->m_Address = m_Address++;				// Get current program address
		m_ActiveAddr->length++;						// Update add range length
		pInst->m_FileIndex = m_FileIndex;			// Save index of the current file
		m_Instructions->Add(pInst);					// Save instruction 
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

	// Update line number
	m_Line = (PCB).line;
	
	// Read operands (register & equation) from parser
	CInstruction*	pInst = new CInstruction;
	if (pInst != NULL)
	{
		pInst->m_ID = opcode;						// Set opcode value
		pInst->m_Operand1 = new MString;			// Allocte operand object
		pInst->m_Operand1->Format("%c", reg[--reg_cnt]);	// Get register operand
		pInst->m_Group = gEq;						// Get equation 
		pInst->m_Line = m_Line;						// Get Line number
		pInst->m_Address = m_Address++;				// Get current program address
		pInst->m_FileIndex = m_FileIndex;			// Save index of the current file

		//  Allocate new equation for the parser
		gEq = new CRpnEquation;

		// Increment Address again to account for 8-bit value
		m_Address++;
		m_ActiveAddr->length += 2;					// Update add range length
		m_Instructions->Add(pInst);					// Save instruction 
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

	// Update line number
	m_Line = (PCB).line;
	
	// Read operand (equation) from parser
	CInstruction*	pInst = new CInstruction;
	if (pInst != NULL)
	{
		pInst->m_ID = opcode;						// Set opcode value
		pInst->m_Group = gEq;						// Get equation 
		pInst->m_Line = m_Line;						// Get Line number
		pInst->m_Address = m_Address++;				// Get current program address
		pInst->m_FileIndex = m_FileIndex;			// Save index of the current file

		//  Allocate new equation for the parser
		gEq = new CRpnEquation;

		// Increment Address again to account for 8-bit value
		m_Address++;
		m_ActiveAddr->length += 2;					// Update add range length
		m_Instructions->Add(pInst);					// Save instruction 
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

	// Update line number
	m_Line = (PCB).line;
	
	// Read operand (register) from string accumulator
	CInstruction*	pInst = new CInstruction;
	if (pInst != NULL)
	{
		pInst->m_ID = opcode;						// Set opcode value
		pInst->m_Group = gEq;						// Get equation 
		pInst->m_Line = m_Line;						// Get Line number
		pInst->m_Address = m_Address++;				// Get current program address
		pInst->m_FileIndex = m_FileIndex;			// Save index of the current file

		//  Allocate new equation for the parser
		gEq = new CRpnEquation;

		// Increment Address to account for 16-bit value
		m_Address += 2;
		m_ActiveAddr->length += 3;					// Update add range length
		m_Instructions->Add(pInst);					// Save instruction 
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
	m_Line = (PCB).line;
	
	// Try to open the file
	fd = fopen(filename, "rb");

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
		m_IncludeIndex[m_IncludeDepth] = m_FileIndex;
		m_IncludeStack[m_IncludeDepth++] = m_fd;

		// Check if this file already in the m_Filenames list!


		m_FileIndex = m_Filenames.Add(filename);
										  
		// Now loading from a different FD!
		m_fd = fd;
		a85parse();
		fclose(m_fd);

		// Restore the previous FD
		m_fd = m_IncludeStack[--m_IncludeDepth];
		m_FileIndex = m_IncludeIndex[m_IncludeDepth];
	}
}

char*		types[5] = { "", "a label", "an equate", "a set", "an extern" };

/*
============================================================================
This function is called when the parser detects an equate operation.
============================================================================
*/
void VTAssembler::equate(const char *name)
{
	double		value;
	VTObject*	dummy;
	int			c;
	CSymbol*	pSymbol;

	// Determine if conditional assembly enabled
	if (m_IfStat[m_IfDepth] != IF_STAT_ASSEMBLE)
	{
		delete gEq;									// Delete the unused equaiton
		gEq = new CRpnEquation;						// Allocate new equation for parser
		return;
	}

	// Update line number
	m_Line = (PCB).line;

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
			for (c = 0; c < 3; c++)
			{
				if (m_ActiveMod->m_Symbols[c]->Lookup(pSymbol->m_Name, dummy))
				{
					MString string;
					string.Format("Error in line %d(%s): Symbol %s already defined as %s",
						pSymbol->m_Line, (const char *) m_Filenames[m_FileIndex], 
						(const char *) pSymbol->m_Name,
						types[((CSymbol *)dummy)->m_SymType & 0x00FF]);
					m_Errors.Add(string);

					// Cleanup
					delete pSymbol->m_Equation;
					delete pSymbol;

					return;
				}
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
void VTAssembler::label(const char *label)
{
	VTObject*	dummy;
	MString		string;
	int			c;
	int			local;

	// Determine if conditional assembly enabled
	if (m_IfStat[m_IfDepth] != IF_STAT_ASSEMBLE)
		return;

	// Update line number
	m_Line = (PCB).line;

	// Determine is label is local (has format $###)
	if (label[0] == '$')
		local = 1;
	else
		local = 0;

	CSymbol*	pSymbol = new CSymbol;
	if (pSymbol != NULL)
	{
		if (local)
			pSymbol->m_Name = m_LastLabel + "%%" + label;
		else
			pSymbol->m_Name = label;
		pSymbol->m_Line = m_Line;
		pSymbol->m_Value = m_Address;
		pSymbol->m_FileIndex = m_FileIndex;			// Save index of the current file
		pSymbol->m_SymType = SYM_LABEL | SYM_HASVALUE;
		pSymbol->m_pRange = m_ActiveAddr;

		// Check if label exists in the symbol table for this module
		for (c = 0; c < 3; c++)
		{
			if (m_ActiveMod->m_Symbols[c]->Lookup(pSymbol->m_Name, dummy))
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
		}
		
		const char *pStr = (const char *) pSymbol->m_Name;
		// Assign symbol to active segment
		(*m_Symbols)[pStr] = pSymbol;
		if (!local)
		{
			m_LastLabel = pStr;
			m_LastLabelSym= pSymbol;
			m_LastLabelAdded = 0;
		}
		else if (m_LastLabelAdded == 0)
		{
			CInstruction* pInst = new CInstruction;
			pInst->m_ID = INST_LABEL;
			pInst->m_Operand1 = new MString;
			*pInst->m_Operand1 = m_LastLabel;
			m_LastLabelAdded = 1;
			m_Instructions->Add(pInst);					// Save instruction 
		}
	}
}

void VTAssembler::directive_set(const char *name)
{
	double		value;
	VTObject*	dummy;
	int			c, symtype;
	CSymbol*	pSymbol;

	// Determine if conditional assembly enabled
	if (m_IfStat[m_IfDepth] != IF_STAT_ASSEMBLE)
	{
		delete gEq;									// Delete the unused equaiton
		gEq = new CRpnEquation;						// Allocate new equation for parser
		return;
	}

	// Update line number
	m_Line = (PCB).line;
	
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

		// Loop through all segments for the ative module and check if symbol exists
		if (name != (const char *) -1)
		{
			for (c = 0; c < 3; c++)
			{
				if (m_ActiveMod->m_Symbols[c]->Lookup(pSymbol->m_Name, dummy))
				{
					symtype = ((CSymbol*) dummy)->m_SymType & 0x00FF;

					// Now insure symbol esists as a SYM_SET object
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
			}

			const char *pStr = (const char *) pSymbol->m_Name;
			// Assign symbol to active segment
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
			m_Address = (unsigned short) value;
			if (m_ActiveAddr->length != 0)
			{
				// Allocate a new AddrRange object for this ORG
				newRange = new AddrRange;
				newRange->address = m_Address;
				newRange->length = 0;

				// Find location to insert this object
				thisRange = m_ActiveMod->m_UsedAddr[m_ActiveMod->m_ActiveSeg];
				prevRange = 0;

				while ((thisRange != 0) && (thisRange->address < m_Address))
				{
					prevRange = thisRange;
					thisRange = thisRange->pNext;
				}

				// Insert new range in list
				if (prevRange->address + prevRange->length == m_Address)
				{
					delete newRange;
					m_ActiveMod->m_ActiveAddr[m_ActiveMod->m_ActiveSeg] = prevRange;
					m_ActiveAddr = prevRange;
				}
				else
				{
					newRange->pNext = 0;
					prevRange->pNext = newRange;

					m_ActiveMod->m_ActiveAddr[m_ActiveMod->m_ActiveSeg] = newRange;
					m_ActiveAddr = newRange;
				}
			}
			else
			{
				m_ActiveAddr->address = m_Address;
			}
		}
		
		// Allocate a new equation for the parser
		gEq = new CRpnEquation;

		// Add instruction to the active segment
		m_Instructions->Add(pInst);
	}
}

void VTAssembler::preproc_ifdef(const char* name)
{
}

void VTAssembler::preproc_ifndef(const char* name)
{
}

void VTAssembler::preproc_else()
{
}

void VTAssembler::preproc_endif()
{
}

void VTAssembler::preproc_define(const char *name)
{
}

void VTAssembler::preproc_undef(const char *name)
{
}

void VTAssembler::directive_aseg()
{
	// Determine if conditional assembly enabled
	if (m_IfStat[m_IfDepth] != IF_STAT_ASSEMBLE)
		return;

	// Update line number
	m_Line = (PCB).line;
	
	// Save current address in current module
	m_ActiveMod->m_Address[m_ActiveMod->m_ActiveSeg] = m_Address;

	// Set segment of current module
	m_ActiveMod->m_ActiveSeg = ASEG;

	// Update pointers to assembly objects
	m_Instructions = m_ActiveMod->m_Instructions[ASEG];
	m_Symbols = m_ActiveMod->m_Symbols[ASEG];
	m_ActiveAddr = m_ActiveMod->m_ActiveAddr[ASEG];

	// Update address location
	m_Address = (unsigned short) m_ActiveMod->m_Address[ASEG];
}

void VTAssembler::directive_cseg(int page)
{
	// Determine if conditional assembly enabled
	if (m_IfStat[m_IfDepth] != IF_STAT_ASSEMBLE)
		return;

	// Update line number
	m_Line = (PCB).line;
	
	// Save current address in current module
	m_ActiveMod->m_Address[m_ActiveMod->m_ActiveSeg] = m_Address;

	// Set segment of current module
	m_ActiveMod->m_ActiveSeg = CSEG;

	// Update pointers to assembly objects
	m_Instructions = m_ActiveMod->m_Instructions[CSEG];
	m_Symbols = m_ActiveMod->m_Symbols[CSEG];
	m_ActiveMod->m_Page[CSEG] = page;
	m_ActiveAddr = m_ActiveMod->m_ActiveAddr[CSEG];

	// Update address location
	m_Address = (unsigned short) m_ActiveMod->m_Address[CSEG];
}

void VTAssembler::directive_dseg(int page)
{
	// Determine if conditional assembly enabled
	if (m_IfStat[m_IfDepth] != IF_STAT_ASSEMBLE)
		return;

	// Update line number
	m_Line = (PCB).line;
	
	// Save current address in current module
	m_ActiveMod->m_Address[m_ActiveMod->m_ActiveSeg] = m_Address;

	// Set segment of current module
	m_ActiveMod->m_ActiveSeg = DSEG;

	// Update pointers to assembly objects
	m_Instructions = m_ActiveMod->m_Instructions[DSEG];
	m_Symbols = m_ActiveMod->m_Symbols[DSEG];
	m_ActiveMod->m_Page[DSEG] = page;
	m_ActiveAddr = m_ActiveMod->m_ActiveAddr[DSEG];

	// Update address location
	m_Address = (unsigned short) m_ActiveMod->m_Address[DSEG];
}

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
		gEq = new CRpnEquation;

		// Try to evaluate the equation
		if (Evaluate((CRpnEquation *) pInst->m_Group, &len, 0))
		{
			m_Address += (int) len;
			m_ActiveAddr->length += (unsigned short) len;
		}

		m_Instructions->Add(pInst);
	}
}

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
		pInst->m_FileIndex = m_FileIndex;			// Save index of the current file

		// Update address based on number of items in expression list
		len = gExpList->GetSize();
		for (c = 0; c < len; c++)
		{
			if (((CExpression *) (*gExpList)[c])->m_Equation != NULL)
			{
				m_Address++;
				m_ActiveAddr->length++;				// Update add range length
			}
			else
			{
				int size = strlen(((CExpression *) (*gExpList)[c])->m_Literal);
				m_Address += size;
				m_ActiveAddr->length += size;		// Update add range length
			}
		}

		gExpList = new VTObArray;
		m_Instructions->Add(pInst);
	}
}

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
		pInst->m_FileIndex = m_FileIndex;			// Save index of the current file

		// Update address based on number of items in expression list
		len = gExpList->GetSize();
		for (c = 0; c < len; c++)
		{
			if (((CExpression *) (*gExpList)[c])->m_Equation != NULL)
			{
				m_Address += 2;
				m_ActiveAddr->length += 2;			// Update add range length
			}
			else
			{
				int size = strlen(((CExpression *) (*gExpList)[c])->m_Literal);
				if (size & 0x01)
					size++;
				m_Address += size;
				m_ActiveAddr->length += size;		// Update add range length
			}
		}

		gExpList = new VTObArray;
		m_Instructions->Add(pInst);
	}
}

void VTAssembler::directive_extern()
{
	int			c, count, x;
	int			exists;
	CSymbol*	pSymbol;
	VTObject*	dummy;

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
			exists = 0;

			// Check if symbol already exists
			for (x = 0; x < 3; x++)
			{
				if (m_ActiveMod->m_Symbols[x]->Lookup((*gNameList)[c], dummy))
				{
					MString string;
					string.Format("Error in line %d(%s): Symbol %s already defined as %s",
						pSymbol->m_Line, (const char *) m_Filenames[m_FileIndex], 
						(const char *) (*gNameList)[c],
						types[((CSymbol *) dummy)->m_SymType & 0x00FF]);
					m_Errors.Add(string);
					exists = 1;
				}
			}

			// If symbol does not exist, add it to the symbol table
			if (!exists)
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

void VTAssembler::directive_link(const char *name)
{
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

void VTAssembler::directive_name(const char *name)
{
	// Determine if conditional assembly enabled
	if (m_IfStat[m_IfDepth] != IF_STAT_ASSEMBLE)
		return;

	// Update line number
	m_Line = (PCB).line;
	
	// Check if module already has a name
	if (m_ActiveMod->m_Name != "")
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
	m_ActiveMod->m_Name = name;
}

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

void VTAssembler::directive_if()
{
	double		valuel, valuer;
	MString		err;

	// Update line number
	m_Line = (PCB).line;
	
	CInstruction*	pInst = new CInstruction;
	if (pInst != NULL)
	{
		pInst->m_ID = INST_IF;
		pInst->m_Line = m_Line;
		pInst->m_Group = gCond;
		pInst->m_FileIndex = m_FileIndex;			// Save index of the current file

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

		if (m_IfDepth >= sizeof(m_IfStat))
		{
			m_IfDepth--;
			err.Format("Error in line %d(%s):  Too many nested ifs", m_Line, 
				(const char *) m_Filenames[m_FileIndex]);
			m_Errors.Add(err);
			gCond = new CCondition;
			m_Instructions->Add(pInst);
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
		m_Instructions->Add(pInst);
	}
}

void VTAssembler::directive_else()
{
	MString		err;

	// Update line number
	m_Line = (PCB).line;
	
	CInstruction*	pInst = new CInstruction;
	if (pInst != NULL)
	{
		pInst->m_ID = INST_ELSE;
		pInst->m_Line = m_Line;
		pInst->m_FileIndex = m_FileIndex;			// Save index of the current file
		m_Instructions->Add(pInst);

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
	}
}

void VTAssembler::directive_endif()
{
	MString		err;

	// Update line number
	m_Line = (PCB).line;
	
	CInstruction*	pInst = new CInstruction;
	if (pInst != NULL)
	{
		pInst->m_ID = INST_ENDIF;
		pInst->m_Line = m_Line;
		pInst->m_FileIndex = m_FileIndex;			// Save index of the current file
		m_Instructions->Add(pInst);

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
	}
}

int VTAssembler::Assemble()
{
	MString			err;
	int				c, count, len, x;
	int				seg, seg2;
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
	unsigned char	op1, op2;
	unsigned char	type;
	double			value;
	char			rel_mask;
	int				valid, extern_label;

	/*
	========================================================================
	Check if active module has a name and assign default name if it does not
	========================================================================
	*/
	m_Address = 0;
	m_IfDepth = 0;
	m_IfStat[0] = IF_STAT_ASSEMBLE;

	/*
	========================================================================
	Loop through eash segment and assemble 
	========================================================================
	*/
	for (seg = 0; seg < 3; seg++)
	{
		m_Instructions = m_ActiveMod->m_Instructions[seg];
		m_Symbols = m_ActiveMod->m_Symbols[seg];
		m_Address = 0;
		m_ActiveAddr = m_ActiveMod->m_UsedAddr[seg];

		// Loop through all instructions and substitute equates and create instruction words
		count = m_Instructions->GetSize();
		for (c = 0; c < count; c++)
		{
			pInst = (CInstruction*) (*m_Instructions)[c];
			m_Line = pInst->m_Line;
			m_FileIndex = pInst->m_FileIndex;
			rel_mask = 0;

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
					opcode |= (int) pInst->m_Group - '0';
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
						if (InvalidRelocation((CRpnEquation *) pInst->m_Group, rel_mask))
						{
							// Error, equation cannot be evaluted
							err.Format("Error in line %d(%s):  Equation depends on multiple segments",
								pInst->m_Line, (const char *) m_Filenames[m_FileIndex]);
							m_Errors.Add(err);
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
							err.Format("Error in line %d(%s):  Equation cannot be evaluated",
								pInst->m_Line, (const char *) m_Filenames[m_FileIndex]);
//							m_Errors.Add(err);
							valid = 0;
						}
					}
				}

				// Add opcode and operands to array
				m_AsmBytes[seg][m_Address++] = opcode;

				// Check if operand is PC relative
				if ((rel_mask > 1) && valid)
				{
					// Add relocation information
					pRel = new CRelocation;
					pRel->m_Address = m_Address;
					pRel->m_pSourceRange = m_ActiveAddr;
					if (rel_mask & 0x02)
						pRel->m_Segment = CSEG;
					if (rel_mask & 0x04)
						pRel->m_Segment = DSEG;
					pRange = m_ActiveMod->m_UsedAddr[pRel->m_Segment];
					while ((pRange != 0) && (pRel->m_pTargetRange == 0))
					{
						if ((pRange->address <= (int) value) &&
							(pRange->address + pRange->length > (int) value))
						{
							pRel->m_pTargetRange = pRange;
						}
						pRange = pRange->pNext;
					}
					m_Reloc[pRel->m_Segment].Add(pRel);
				}

				// Check if operand is extern
				if (extern_label)
				{
					// Add extern linkage information
					pExt = new CExtern;
					pExt->m_Address = m_Address;
					pExt->m_Name = ((CRpnEquation *)
						pInst->m_Group)->m_OperationArray[0].m_Variable;
					pExt->m_Segment = seg;
					pExt->m_Size = size;
					pExt->m_pRange = m_ActiveAddr;
					m_Externs.Add(pExt);
				}

				// Add operands to output
				if ((type == OPCODE_REG_IMM) || (type == OPCODE_EQU8) ||
					(type == OPCODE_EQU16) || (type == OPCODE_REG_EQU16))
				{
					m_AsmBytes[seg][m_Address++] = op1;
				}

				if ((type == OPCODE_EQU16) || (type == OPCODE_REG_EQU16))
				{
					m_AsmBytes[seg][m_Address++] = op2;
				}
			}
			

			/*
			====================================================================
			An ORG instruciton has no FBI portion, so process it separately.
			====================================================================
			*/
			else if (pInst->m_ID == INST_ORG)
			{
				// The parser takes care of this instruction but we need to start a new
				// Segment in the .obj file
				if (Evaluate((CRpnEquation *) pInst->m_Group, &value, 1))
				{
					m_Address = (int) value;
					
					// Find AddrRange object for this ORG
					pRange = m_ActiveMod->m_UsedAddr[seg];
					while (pRange != 0)
					{
						if ((pRange->address == m_Address) || ((pRange->address < m_Address) &&
							(pRange->address + pRange->length > m_Address)))
						{
							m_ActiveMod->m_ActiveAddr[seg] = pRange;
							m_ActiveAddr = pRange;
							break;
						}

						// Point to next AddrRange
						pRange = pRange->pNext;
					}
				}
				else
				{
					err.Format("Error in line %d(%s):  Equation cannot be evaluated",
						pInst->m_Line, (const char *) m_Filenames[m_FileIndex]);
					m_Errors.Add(err);
				}
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
							m_AsmBytes[seg][m_Address++] = (unsigned char) value;
						else
						{
							err.Format("Error in line %d(%s):  Equation cannot be evaluated",
								pInst->m_Line, (const char *) m_Filenames[m_FileIndex]);
							m_Errors.Add(err);
						}
					}
					else
					{
						// Add bytes from the string
						int y, str_len;
						str_len = pExp->m_Literal.GetLength();
						for (y = 0; y < str_len; y++)
							m_AsmBytes[seg][m_Address++] = pExp->m_Literal[y];
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
							m_AsmBytes[seg][m_Address++] = ((unsigned short) value) & 0xFF;
							m_AsmBytes[seg][m_Address++] = ((unsigned short) value) >> 8;
						}
						else
						{
							err.Format("Error in line %d(%s):  Equation cannot be evaluated",
								pInst->m_Line, (const char *) m_Filenames[m_FileIndex]);
							m_Errors.Add(err);
						}
					}
					else
					{
						// Add bytes from the string
						int y, str_len;
						str_len = pExt->m_Name.GetLength();
						for (y = 0; y < str_len; y++)
							m_AsmBytes[seg][m_Address++] = pExt->m_Name[y];
						if (str_len & 1)
							m_AsmBytes[seg][m_Address++] = 0;
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
					err.Format("Error in line %d(%s):  Equation cannot be evaluated",
						pInst->m_Line, (const char *) m_Filenames[m_FileIndex]);
					m_Errors.Add(err);
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
					// Lookup symbol in each segment
					for (seg2 = 0; seg2 < 3; seg2++)
					{
						if (m_ActiveMod->m_Symbols[seg2]->Lookup((*pNameList)[x], 
							(VTObject *&) pSymbol))
						{
							pSymbol->m_SymType |= SYM_PUBLIC;

							// Check if symbol is a relative label
							if ((pSymbol->m_SymType & 0x00FF) == SYM_LABEL)
							{
								m_Publics[seg2].Add(pSymbol);
							}
							break;
						}
						else
							pSymbol = 0;
					}

					// Check if symbol was found in symbol table
					if (pSymbol == 0)
					{
						// Report error
						err.Format("Error in line %d(%s):  Symbol %s undefined",
							pInst->m_Line, (const char *) m_Filenames[m_FileIndex], 
							(const char *) (*pNameList)[x]);
						m_Errors.Add(err);
					}
				}
			}
		}
	}

	// Check for errors during assembly
	if (m_Errors.GetSize() != 0)
		return 0;

	return 1;
}

/*
========================================================================
This routine checks the specified equation to determine if it depends on
more than one segment.
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
	CRelocation*	pReloc;
	AddrRange*		pRange;
	CExtern*		pExt;
	MString			key;
	POSITION		pos;
	char			sectString[] = "\0.strtab\0.symtab\0.aseg\0.text\0.data\0";
	char			relString[] = ".relcseg\0.reldseg\0";
	char			debugString[] = ".debug\0";
	int				debugStrSize = 7;
	int				strtab_off = 1;
	int				symtab_off = 9;
	int				aseg_off = 17;
	int				cseg_off = 23;
	int				dseg_off = 29;
	int				relcseg_off = 35;
	int				reldseg_off = 44;
	int				extern_off = 53;
	int				seg, len;
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
	ehdr.e_ident[EI_CLASS] = ELFCLASS8;
	ehdr.e_ident[EI_DATA] = ELFDATA2LSB;
	ehdr.e_ident[EI_VERSION] = EV_CURRENT;
	for (c = EI_PAD; c < EI_NIDENT; c++)
		ehdr.e_ident[c] = 0;
	ehdr.e_type = ET_REL;			// Relocatable object file
	ehdr.e_machine = ET_8085;		// Model 100 machine
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
	strhdr.sh_flags = 0;			// No flags for this section
	strhdr.sh_addr = 0;				// No address data for string table
	strhdr.sh_offset = ftell(fd);
	strhdr.sh_size = extern_off;	// Initialize size to zero -- fill in later
	strhdr.sh_link = 0;				// Set link data to zero
	strhdr.sh_info = 0;				// Set info byte to zero
	strhdr.sh_addralign = 0;		// Set allignment data to zero
	strhdr.sh_entsize = 0;			// String table does not have equal size entries
	ehdr.e_shnum++;					// Increment Section Header count

	// Start STRTAB section for section strings
	fwrite(sectString, relcseg_off, 1, fd);
	fwrite(relString, 18, 1, fd); 
	if (m_DebugInfo)
	{
		fwrite(debugString, debugStrSize, 1, fd);
		strhdr.sh_size += debugStrSize;
	}

	/*
	=========================================================
	Now create a STRTAB section for EXTERN and PUBLIC symbols
	=========================================================
	*/
	// Create section header for string table
	str2hdr.sh_name = strtab_off;	// Put ".strtab" as first entry in table
	str2hdr.sh_type = SHT_STRTAB;	// Make section a string table
	str2hdr.sh_flags = 0;			// No flags for this section
	str2hdr.sh_addr = 0;			// No address data for string table
	str2hdr.sh_offset = ftell(fd);
	str2hdr.sh_size = 1;			// Initialize size to zero -- fill in later
	str2hdr.sh_link = 0;			// Set link data to zero
	str2hdr.sh_info = 0;			// Set info byte to zero
	str2hdr.sh_addralign = 0;		// Set allignment data to zero
	str2hdr.sh_entsize = 0;			// String table does not have equal size entries
	ehdr.e_shnum++;					// Increment Section Header count

	// Start with a null string
	fwrite(sectString, 1, 1, fd);
	idx = 1;
	for (seg = 0; seg < 3; seg++)
	{
		pos = m_ActiveMod->m_Symbols[seg]->GetStartPosition();
		while (pos != 0)
		{
			// Get next symbol
			m_ActiveMod->m_Symbols[seg]->GetNextAssoc(pos, key, (VTObject *&) pSymbol);
			if ((pSymbol->m_SymType & SYM_PUBLIC) || (pSymbol->m_SymType & SYM_EXTERN))
			{
				// Add string for this symbol
				pSymbol->m_StrtabOffset = idx;
				len = strlen(pSymbol->m_Name) + 1;
				fwrite(pSymbol->m_Name, len, 1, fd);
				str2hdr.sh_size += len;
				idx++;
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
	symhdr.sh_addr = 0;			// No address data for symbol table
	symhdr.sh_offset = ftell(fd);
	symhdr.sh_size = 0;			// Initialize size to zero -- fill in later
	symhdr.sh_link = 0;			// Set link data to zero
	symhdr.sh_info = 0;			// Set info byte to zero
	symhdr.sh_addralign = 0;		// Set allignment data to zero
	symhdr.sh_entsize = sizeof(Elf32_Sym);// Symbol table has equal size items
	ehdr.e_shnum++;					// Increment Section Header count
	
	// Now add all EXTERN and PUBLIC symbols to string table
	idx = 0;
	for (seg = 0; seg < 3; seg++)
	{
		pos = m_ActiveMod->m_Symbols[seg]->GetStartPosition();
		while (pos != 0)
		{
			// Get next symbol
			m_ActiveMod->m_Symbols[seg]->GetNextAssoc(pos, key, (VTObject *&) pSymbol);
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
					fwrite(&sym, sizeof(sym), 1, fd);
					idx++;
				}
				if (pSymbol->m_SymType & SYM_8BIT)
				{
					sym.st_size = 1;
					pSymbol->m_Off8 = idx;
					fwrite(&sym, sizeof(sym), 1, fd);
					idx++;
				}
				if (pSymbol->m_SymType & SYM_16BIT)
				{
					sym.st_size = 2;
					pSymbol->m_Off16 = idx;
					fwrite(&sym, sizeof(sym), 1, fd);
					idx++;
				}

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
	symhdr.sh_size = len * sizeof(Elf32_Sym);

	/*
	==========================================
	Create section for ASEG code
	==========================================
	*/
	// First count the number of sections needed
	aseg_sections = 1;
	pRange = m_ActiveMod->m_UsedAddr[ASEG];
	while (pRange->pNext != 0)
	{
		aseg_sections++;
		pRange =pRange->pNext;
	}
	// Allocate pointers for aseg_section headers
	aseg_hdr = (Elf32_Shdr *) malloc(aseg_sections * sizeof(Elf32_Shdr));

	pRange = m_ActiveMod->m_UsedAddr[ASEG];
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
		aseg_hdr[idx].sh_flags = SHF_ALLOC | SHF_EXECINSTR;	// Set flag bits
		aseg_hdr[idx].sh_addr = pRange->address;// Address for this section
		aseg_hdr[idx].sh_offset = ftell(fd);
		aseg_hdr[idx].sh_size = pRange->length;	// Get size from AddrRange
		aseg_hdr[idx].sh_link = 0;			// Set link data to zero
		aseg_hdr[idx].sh_info = 0;			// Set info byte to zero
		aseg_hdr[idx].sh_addralign = 0;		// Set allignment data to zero
		aseg_hdr[idx].sh_entsize = 0;		// Symbol table has equal size items

		// Save the index of this CSEG Section Header for relocation reference
		pRange->shidx = ehdr.e_shnum;
	
		ehdr.e_shnum++;						// Increment Section Header count

		// Write data to the file
		fwrite(&m_AsmBytes[ASEG][pRange->address], pRange->length, 1, fd);

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
	cseg_sections = 1;
	pRange = m_ActiveMod->m_UsedAddr[CSEG];
	while (pRange->pNext != 0)
	{
		cseg_sections++;
		pRange =pRange->pNext;
	}
	// Allocate pointers for aseg_section headers
	cseg_hdr = (Elf32_Shdr *) malloc(cseg_sections * sizeof(Elf32_Shdr));

	pRange = m_ActiveMod->m_UsedAddr[CSEG];
	idx = 0;
	first_cseg_idx = ehdr.e_shnum;

	while (pRange != NULL)
	{
		if (pRange->length == 0)
		{
			// If AddrRange is emtpy, don't write it to the .obj file
			pRange = pRange->pNext;
			cseg_hdr[idx].sh_name = 0;		// Indicate empty section
			idx++;							// Increment to next index
			continue;
		}

		cseg_hdr[idx].sh_name = cseg_off;	// Point to ".cseg" name
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
		fwrite(&m_AsmBytes[CSEG][pRange->address], pRange->length, 1, fd);

		// Point to next ASEG address range
		pRange = pRange->pNext;
		idx++;								// Increment to next index
	}


	/*
	==========================================
	Create section for DSEG code.  DSEGs are
	relocatable
	==========================================
	*/
	// First count the number of sections needed
	dseg_sections = 1;
	pRange = m_ActiveMod->m_UsedAddr[DSEG];
	while (pRange->pNext != 0)
	{
		dseg_sections++;
		pRange =pRange->pNext;
	}
	// Allocate pointers for aseg_section headers
	dseg_hdr = (Elf32_Shdr *) malloc(dseg_sections * sizeof(Elf32_Shdr));

	pRange = m_ActiveMod->m_UsedAddr[DSEG];
	idx = 0;
	first_dseg_idx = ehdr.e_shnum;
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

		dseg_hdr[idx].sh_name = dseg_off;	// Point to ".dseg" name
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
		fwrite(&m_AsmBytes[DSEG][pRange->address], pRange->length, 1, fd);

		// Point to next ASEG address range
		pRange = pRange->pNext;
		idx++;								// Increment to next index
	}

	/*
	=======================================
	Add relocation sections to .OBJ file
	=======================================
	*/
	rel_hdrs = (Elf32_Shdr *) malloc((dseg_sections + 
		cseg_sections) * sizeof(Elf32_Shdr));
	idx = 0;
	pRange = m_ActiveMod->m_UsedAddr[CSEG];
	while (idx < dseg_sections + cseg_sections)
	{
		if (idx < cseg_sections)
		{
			rel_hdrs[idx].sh_name = relcseg_off;
			seg = CSEG;
		}
		else
		{
			rel_hdrs[idx].sh_name = reldseg_off;
			seg = DSEG;
		}
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
		len = m_Reloc[seg].GetSize();

		// Loop through all relocation items in seg
		for (c = 0; c < len; c++)
		{
			// Check if this relocation relative to current section
			pReloc = (CRelocation *) m_Reloc[seg][c];
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
		len = m_Publics[seg].GetSize();
		for (c = 0; c < len; c++)
		{
			pSymbol = (CSymbol *) m_Publics[seg][c];

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
		if (pRange == 0)
			pRange = m_ActiveMod->m_UsedAddr[DSEG];

		idx++;								// Increment to next index
	}

	/*
	=======================================
	Add debug sections to .OBJ file
	=======================================
	*/
	if (m_AsmOptions.Find("-g") >= 0)
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
int VTAssembler::InvalidRelocation(CRpnEquation *pEq, char &rel_mask)
{
	int		c, count;
	char	rel[3];
	int		seg;
	CSymbol	*pSym;

	count = pEq->m_OperationArray.GetSize();
	for (c = 0; c < 3; c++)
		rel[c] = 0;

	for (c = 0; c < count; c++)
	{
		// Check if operation type is a variable
		if (pEq->m_OperationArray[c].m_Operation == RPN_VARIABLE)
		{
			// Loop through all segments and find the symbol
			for (seg = 0; seg < 3; seg++)
			{
				if (m_ActiveMod->m_Symbols[seg]->Lookup(pEq->m_OperationArray[c].m_Variable, 
					(VTObject *&) pSym))
				{
					if ((pSym->m_SymType & 0x00FF) == SYM_LABEL)
						rel[seg] = 1;

					break;
				}
			}
		}
	}

	rel_mask = rel[0] | (rel[1] << 1) | (rel[2] << 2);

	// Check if euqation is relative to more than one segment
	if (rel[0] + rel[1] + rel[2] > 1)
		return 1;

	return 0;
}

/*
========================================================================
This routine checks the specified equation to determine if it depends on
more than one segment.
========================================================================
*/
int VTAssembler::EquationIsExtern(CRpnEquation* pEq, int size)
{
	int			seg, count;
	CSymbol*	pSym;

	// Get count of operation array
	count = pEq->m_OperationArray.GetSize();

	// Check if quation has exactly 1 operation
	if (count == 1)
	{
		// Check if operation is a variable
		if (pEq->m_OperationArray[0].m_Operation == RPN_VARIABLE)
		{
			for (seg = 0; seg < 3; seg++)
			{
				if (m_ActiveMod->m_Symbols[seg]->Lookup(pEq->m_OperationArray[0].m_Variable, 
					(VTObject *&) pSym))
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
	}
	return 0;
}

void VTAssembler::Parse(MString filename)
{
	int		success = 1;
	MString	outfile;								 
	MString	temp;


	// Clean up the assembler files from previous assembler
	ResetContent();

	// Try to assemble the file
	if (success = ParseASMFile(filename, this))
	{
		// No parse errors!  Try to assemble
		if (success = Assemble())
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
			outfile += ".obj";

			// Generate the object file
			CreateObjFile(outfile);

			// Generate a listing file if requested
			CreateList(temp);
		}
	}
}

void VTAssembler::CreateList(MString& filename)
{
	MString		outfile;
	FILE*		fd;

	if ((m_AsmOptions.Find("-l") != -1) || (m_List))
	{
		if ((strcmp(filename, ".asm") == 0) || (strcmp(filename, "a85") == 0))
		{
			outfile = filename.Left(filename.GetLength()-4);
		}
		else
			outfile = filename;

		// Append .lst to filename
		outfile += ".lst";

		fd = fopen(outfile, "wb+");
		fclose(fd);
	}
}

void VTAssembler::CreateHex()
{
}

/*
=====================================================================
Implement the CModule class.  This class is used to keep track of 
assembly segments that are broken down into modules.
=====================================================================
*/
CModule::CModule()
{
	int		c;

	for (c=0; c < 3; c++)
	{
		m_Symbols[c] = new VTMapStringToOb;
		m_Instructions[c] = new VTObArray;

		m_Address[c] = 0;
		m_Page[c] = 0;
		m_UsedAddr[c] = 0;
		m_UsedAddr[c] = new AddrRange;
		m_ActiveAddr[c] = m_UsedAddr[c];

		m_ActiveAddr[c]->address = 0;
		m_ActiveAddr[c]->length = 0;
		m_ActiveAddr[c]->pNext = 0;
		m_ActiveAddr[c]->shidx = 0;
	}

	m_ActiveSeg = 0;
}

CModule::~CModule()
{
	int			c;
	AddrRange*	pRange;
	AddrRange*	pNext;

	for (c = 0; c < 3; c++)
	{
		delete m_Symbols[c];
		delete m_Instructions[c];

		pRange = m_UsedAddr[c];
		while (pRange != 0)
		{
			pNext = pRange->pNext;
			delete pRange;
			pRange = pNext;
		}
	}
}

IMPLEMENT_DYNCREATE(CInstruction, VTObject);
IMPLEMENT_DYNCREATE(CCondition, VTObject);

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
	pStr = new char[m_IncludePath.GetLength()];
	strcpy(pStr, (const char *) m_IncludePath);

	pToken = strtok(pStr, ",;");
	while (pToken != NULL)
	{
		// Generate the path
		temp = pToken;
		temp.Trim();
		temp += "/";
		temp = m_RootPath + "/" + temp;

		// Now add it to the array
		m_IncludeDirs.Add(temp);

		// Get next token
		pToken = strtok(NULL, ",;");
	}
}
