#ifndef		_VTAssembler_
#define		_VTAssembler_

#include "vtobj.h"
#include "rpn_eqn.h"
#include <stdio.h>

// Define values for opcodes
#define		OPCODE_LDAX		1
#define		OPCODE_LXI		2
#define		OPCODE_STAX		3
#define		OPCODE_INX		4
#define		OPCODE_DCX		5
#define		OPCODE_INR		6
#define		OPCODE_DCR		7
#define		OPCODE_MVI		8
#define		OPCODE_ADD		9
#define		OPCODE_ADC		10
#define		OPCODE_SUB		11
#define		OPCODE_SBB		12
#define		OPCODE_ANA		13
#define		OPCODE_XRA		14
#define		OPCODE_ORA		15
#define		OPCODE_CMP		16
#define		OPCODE_DAD		17
#define		OPCODE_POP		18
#define		OPCODE_PUSH		19
#define		OPCODE_DAA		20
#define		OPCODE_LDEH		21
#define		OPCODE_LHLD		22
#define		OPCODE_CMA		23
#define		OPCODE_SIM		24
#define		OPCODE_STA		25
#define		OPCODE_STC		26
#define		OPCODE_LDES		27
#define		OPCODE_LDA		28
#define		OPCODE_CMC		29
#define		OPCODE_MOV		30
#define		OPCODE_RLC		31
#define		OPCODE_DSUB		32
#define		OPCODE_RAR		33
#define		OPCODE_NOP		34
#define		OPCODE_RRC		35
#define		OPCODE_ASHR		36
#define		OPCODE_RAL		37
#define		OPCODE_RLDE		38
#define		OPCODE_RNZ		39
#define		OPCODE_RZ		40
#define		OPCODE_RNC		41
#define		OPCODE_RC		42
#define		OPCODE_RPO		43
#define		OPCODE_RPE		44
#define		OPCODE_RP		45
#define		OPCODE_RM		46
#define		OPCODE_RIM		47
#define		OPCODE_SHLX		48
#define		OPCODE_PCHL		49
#define		OPCODE_SPHL		50
#define		OPCODE_JNZ		51
#define		OPCODE_JZ		52
#define		OPCODE_JNC		53
#define		OPCODE_JC		54
#define		OPCODE_JPO		55
#define		OPCODE_JPE		56
#define		OPCODE_JP		57
#define		OPCODE_JM		58
#define		OPCODE_OUT		59
#define		OPCODE_IN		60
#define		OPCODE_XTHL		61
#define		OPCODE_XCHG		62
#define		OPCODE_DI		63
#define		OPCODE_EI		64
#define		OPCODE_CNZ		65
#define		OPCODE_CZ		66
#define		OPCODE_CNC		67
#define		OPCODE_CC		68
#define		OPCODE_CPO		69
#define		OPCODE_CPE		70
#define		OPCODE_CP		71
#define		OPCODE_CM		72
#define		OPCODE_SHLD		73
#define		OPCODE_CALL		74
#define		OPCODE_JND		75
#define		OPCODE_LHLX		76
#define		OPCODE_JD		77
#define		OPCODE_ADI		78
#define		OPCODE_ACI		79
#define		OPCODE_SUI		80
#define		OPCODE_SBI		81
#define		OPCODE_ANI		82
#define		OPCODE_XRI		83
#define		OPCODE_ORI		84
#define		OPCODE_CPI		85
#define		OPCODE_RST		86
#define		OPCODE_JMP		87
#define		OPCODE_RET		88
#define		OPCODE_HLT		89

#define		INST_ORG		90
#define		INST_DS			91
#define		INST_DB			92
#define		INST_DW			93
#define		INST_STKLN		94
#define		INST_END		95
#define		INST_PUBLIC		96
#define		INST_EXTERN		97
#define		INST_IF			98
#define		INST_ELSE		99
#define		INST_ENDIF		100
#define		INST_LINK		101
#define		INST_MACLIB		102
#define		INST_PAGE		103
#define		INST_SYM		104
#define		INST_LABEL		105

#define		SYM_LABEL		1
#define		SYM_EQUATE		2
#define		SYM_SET			3
#define		SYM_EXTERN		4
#define		SYM_8BIT		0x0400
#define		SYM_16BIT		0x0800
#define		SYM_PUBLIC		0x1000
#define		SYM_HASVALUE	0x8000
#define		SYM_ISEQ		0x4000
#define		SYM_ISREG		0x2000

#define		OPCODE_NOARG		0
#define		OPCODE_1REG			1
#define		OPCODE_2REG			2
#define		OPCODE_IMM			3
#define		OPCODE_REG_IMM		4
#define		OPCODE_EQU8			5
#define		OPCODE_EQU16		6
#define		OPCODE_REG_EQU16	7

#define		PAGE			1
#define		INPAGE			2

#define		COND_EQ			1
#define		COND_NE			2
#define		COND_GE			3
#define		COND_LE			4
#define		COND_GT			5
#define		COND_LT			6
#define		COND_NOCMP		7

#define		IF_STAT_ASSEMBLE				1
#define		IF_STAT_DONT_ASSEMBLE			2
#define		IF_STAT_NESTED_DONT_ASSEMBLE	3
#define		IF_STAT_EVAL_ERROR				4

#define		ASEG			0
#define		CSEG			1
#define		DSEG			2

// Support classes for VTAssembler objects...

typedef struct sAddrRange {
	unsigned short		address;
	unsigned short		length;
	unsigned short		shidx;
	struct sAddrRange*	pNext;
} AddrRange;

class CInstruction : public VTObject
{
public:
	CInstruction() { m_ID = 0; m_Line = -1; m_FileIndex = 0; m_Address = 0; 
						m_Operand1 = 0; m_Group = NULL; }
	~CInstruction();

	DECLARE_DYNCREATE(CInstruction);

// Attributes
	unsigned char		m_ID;
	unsigned char		m_FileIndex;
	unsigned short		m_Address;
	long				m_Line;
	MString*			m_Operand1;
	VTObject*			m_Group;
};

class CExpression : public VTObject
{
public:
	CExpression()		{ m_Equation = 0; }
	~CExpression()		{ if (m_Equation != 0) delete m_Equation; };
	MString				m_Literal;
	CRpnEquation*		m_Equation;
};

class CExtern : public VTObject
{
public:
	CExtern()			{ m_Address = 0; m_pRange = 0; };

	MString				m_Name;
	unsigned short		m_Address;
	unsigned short		m_Segment;
	unsigned short		m_SymIdx;
	AddrRange*			m_pRange;
	unsigned char		m_Size;
};

class CCondition : public VTObject
{
public:
	CCondition()		{ m_EqRight = 0, m_EqLeft = 0; m_Condition = 0;};
	~CCondition()		{ if (m_EqLeft != 0) delete m_EqLeft;  if (m_EqRight != 0) delete m_EqRight; };

	DECLARE_DYNCREATE(CCondition);

	CRpnEquation*		m_EqRight;
	CRpnEquation*		m_EqLeft;
	int					m_Condition;
};

class CRelocation : public VTObject
{
public:
	CRelocation()		{ m_Address = 0; m_Segment = 0; m_pSourceRange = 0; m_pTargetRange = 0; };

	unsigned short		m_Address;
	unsigned char		m_Segment;
	AddrRange*			m_pSourceRange;
	AddrRange*			m_pTargetRange;
};

class CSymbol : public VTObject
{
public:
	CSymbol() { m_Line = -1; m_Value = -1; m_SymType = 0; m_Equation = 0; m_StrtabOffset = 0; }
	~CSymbol()			{ if (m_Equation != 0) delete m_Equation; };

// Attributes

	MString				m_Name;
	CRpnEquation*		m_Equation;
	long				m_Line;
	long				m_Value;
	unsigned short		m_SymType;
	unsigned short		m_FileIndex;
	long				m_StrtabOffset;
	unsigned long		m_Off8;
	unsigned long		m_Off16;
	AddrRange*			m_pRange;
};

class CModule : public VTObject
{
public:
	CModule();
	~CModule();

	MString				m_Name;				// Module name
	MString				m_Title;			// Module title

	int					m_ActiveSeg;		// Segment we are compiling into

	VTMapStringToOb*	m_Symbols[3];		// Array of Symbols for each segment
	VTObArray*			m_Instructions[3];	// Array of Instructions for each segment
	long				m_Address[3];		// Address counter for each segment
	int					m_Page[3];

	AddrRange*			m_UsedAddr[3];		// List of used address ranges
	AddrRange*			m_ActiveAddr[3];	// Pointer to active address range
};

class VTAssembler : public VTObject
{
public:
	VTAssembler();
	~VTAssembler();

	// Define Preprocessor functions
	void				preproc_endif(void);
	void				preproc_ifndef(const char *name);
	void				preproc_ifdef(const char *name);
	void				preproc_else(void);
	void				preproc_define(const char *name);
	void				preproc_undef(const char *name);

	// Define Pragma functions
	void				pragma_list();
	void				pragma_hex();

	// Define directive functions
	void				directive_set(const char *name);
	void				directive_aseg(void);
	void				directive_cseg(int page);
	void				directive_dseg(int page);
	void				directive_ds(void);
	void				directive_db(void);
	void				directive_dw(void);
	void				directive_extern(void);
	void				directive_org();
	void				directive_public(void);
	void				directive_name(const char *name);
	void				directive_stkln(void);
	void				directive_end(const char *name);
	void				directive_if(void);
	void				directive_else(void);
	void				directive_endif(void);
	void				directive_title(const char *name);
	void				directive_link(const char *name);
	void				directive_maclib(const char *name);
	void				directive_sym(void);
	void				directive_page(int page);

	void				include(const char *filename);
	void				equate(const char *name);
	void				label(const char *label);
	
	void				opcode_arg_0(int opcode);
	void				opcode_arg_1reg(int opcode);
	void				opcode_arg_imm(int opcode, char c);
	void				opcode_arg_2reg(int opcode);
	void				opcode_arg_1reg_equ8(int opcode);
	void				opcode_arg_equ8(int opcode);
	void				opcode_arg_equ16(int opcode);

// Attributes
//	MString				m_Filename;			// Filename that design was parsed from
	FILE*				m_fd;				// File descriptor of open file
	int					m_Line;
	int					m_FileIndex;

	VTMapStringToOb*	m_Modules;			// Map of CModules
	CModule*			m_ActiveMod;		// Pointer to active CModule
	AddrRange*			m_ActiveAddr;		// Pointer to active address range

	VTMapStringToOb*	m_Symbols;			// Array of Symbols
	VTObArray*			m_Instructions;		// Array of Instructions
	long				m_Address;
	MStringArray		m_Filenames;		// Array of filenames parsed during assembly
	MString				m_LastLabel;		// Save value of last label parsed
	CSymbol*			m_LastLabelSym;		// Pointer to CSymbol object for last label
	int					m_LastLabelAdded;

	MStringArray		m_Errors;			// Array of error messages during parsing
	unsigned char		m_AsmBytes[3][65536];
	VTObArray			m_Reloc[3];
	VTObArray			m_Externs;
	VTObArray			m_Publics[3];		// Array of public symbols for each segment
	VTMapStringToOb		m_UndefSymbols;
	int					m_List;				// Create a list file?
	int					m_Hex;				// Create a HEX file?
	int					m_DebugInfo;		// Include debug info in .obj?
	MString				m_IncludeName[32];
	FILE*				m_IncludeStack[32];
	int					m_IncludeIndex[32];
	int					m_IncludeDepth;

	char				m_IfStat[100];
	int					m_IfDepth;


// Operations
	int					Evaluate(class CRpnEquation* eq, double* value,  
							int reportError);
	int					GetValue(MString & string, int & value);
	void				ResetContent(void);
	int					Assemble();
	int					CreateObjFile(const char *filename);
	int					InvalidRelocation(CRpnEquation* pEq, char &rel_mask);
	int					EquationIsExtern(CRpnEquation* pEq, int size);
	void				Parse(MString filename);
	void				MakeBinary(int val, int length, MString& binary);
	void				CreateHex();
	void				CreateList();
};

#endif
