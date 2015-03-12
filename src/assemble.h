#ifndef		_VTAssembler_
#define		_VTAssembler_

#include "vtobj.h"
#include "rpn_eqn.h"
#include "a85parse.h"
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
#define		OPCODE_RSTV		90

// Extended OPCODES
#define		OPCODE_LRET		91
#define		OPCODE_LCALL	92
#define		OPCODE_LJMP		93
#define		OPCODE_LPUSH	94
#define		OPCODE_LPOP		95
#define		OPCODE_BR		96
#define		OPCODE_BRA		97
#define		OPCODE_BZ		98
#define		OPCODE_BNZ		99
#define		OPCODE_BC		100
#define		OPCODE_BNC		101
#define		OPCODE_BM		102
#define		OPCODE_BP		103
#define		OPCODE_BPE		104
#define		OPCODE_SBZ		105
#define		OPCODE_SBNZ		106
#define		OPCODE_SBC		107
#define		OPCODE_SBNC		108
#define		OPCODE_RCALL	109
#define		OPCODE_SPI		110
#define		OPCODE_SPG		111
#define		OPCODE_RPG		112

#define		INST_ORG		113
#define		INST_DS			114
#define		INST_DB			115
#define		INST_DW			116
#define		INST_STKLN		117
#define		INST_END		118
#define		INST_PUBLIC		119
#define		INST_EXTERN		120
#define		INST_IF			121
#define		INST_ELSE		122
#define		INST_ENDIF		123
#define		INST_LINK		124
#define		INST_MACLIB		125
#define		INST_PAGE		126
#define		INST_SYM		127
#define		INST_LABEL		128
#define		INST_FILL		129
#define		INST_ENDIAN		130
#define		INST_ELIF		131
#define		INST_MODULE		132
#define		INST_DEFINE		133
#define		INST_UNDEFINE	134
#define		INST_MACRO		135

#define		SYM_LABEL		1
#define		SYM_EQUATE		2
#define		SYM_SET			3
#define		SYM_EXTERN		4
#define		SYM_DEFINE		5
#define		SYM_EXTERN8		6
#define		SYM_CSEG		0x0100
#define		SYM_DSEG		0x0200
#define		SYM_8BIT		0x0400
#define		SYM_16BIT		0x0800
#define		SYM_PUBLIC		0x1000
#define		SYM_ISREG		0x2000
#define		SYM_ISEQ		0x4000
#define		SYM_HASVALUE	0x8000

#define		OPCODE_NOARG		0
#define		OPCODE_1REG			1
#define		OPCODE_2REG			2
#define		OPCODE_IMM			3
#define		OPCODE_REG_IMM		4
#define		OPCODE_EQU8			5
#define		OPCODE_EQU16		6
#define		OPCODE_REG_EQU16	7
#define		OPCODE_PG			8
#define		OPCODE_PGI			9
#define		OPCODE_SBCC			10
#define		OPCODE_BRX			11
#define		OPCODE_EQU24		12
#define		OPCODE_LPP			13

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

#define		PREPROC_STAT_NO_SUBST		0
#define		PREPROC_STAT_SUBST			1
#define		PREPROC_STAT_ERROR			2

#define		MAX_SEG_SIZE	(4*1024*1024)

#define	VT_ISDIGIT(x)  (((x) >= '0') && ((x) <= '9'))

#ifdef WIN32
#define	LINE_ENDING		"\r\n"
#else
#define	LINE_ENDING		"\n"
#endif
// Support classes for VTAssembler objects...

typedef struct sAddrRange {
	unsigned int		address;
	unsigned short		length;
	unsigned short		shidx;
	struct sAddrRange*	pNext;
	unsigned int		line;				// Line No in file where this range starts
} AddrRange;

class CInstruction : public VTObject
{
public:
	CInstruction() { m_ID = 0; m_Line = -1; m_FileIndex = 0; m_Address = 0; 
						m_Operand1 = 0; m_Group = NULL; m_Bytes = 0; }
	~CInstruction();

	DECLARE_DYNCREATE(CInstruction);

// Attributes
	unsigned char		m_ID;
	unsigned char		m_FileIndex;
	unsigned int		m_Address;
	long				m_Line;
	int					m_Bytes;
	MString*			m_Operand1;
	VTObject*			m_Group;
};

class CModule : public VTObject
{
public:
	CModule(const char *name);
	~CModule();

	MString				m_Name;				// Module name
	MString				m_Title;			// Module title

	VTMapStringToOb*	m_Symbols;			// Array of Symbols for this module
	VTObArray			m_Publics;			// Array of public symbols from this module
};

class CSegment : public VTObject
{
public:
	CSegment(const char *name, int type, CModule* pInitialMod);
	~CSegment();

	MString				m_Name;				// Name of segment
	CModule*			m_InitialMod;		// Initial module upon creation
	CModule*			m_LastMod;			// Last active module
	int					m_Type;				// ASEG, CSEG or DSEG type
	int					m_InstIndex;		// Used for listing generation
	int					m_Page;
	int					m_Index;			// Current index for listing
	int					m_Count;			// Instruction count for listing
	int					m_sh_offset;		// Offset in .obj file of segment name
	int					m_Line;				// Line numbrer in source of the seg directive
	int					m_LastLine;			// Line numbrer in source of last line
	VTObArray*			m_Instructions;		// Array of Instructions for each segment
	unsigned int		m_Address;			// Address counter for each segment
	VTObArray			m_Reloc;
	unsigned char		m_AsmBytes[MAX_SEG_SIZE];
	AddrRange*			m_UsedAddr;			// List of used address ranges
	AddrRange*			m_ActiveAddr;		// Pointer to active address range
};

class CExpression : public VTObject
{
public:
	CExpression()		{ m_Equation = 0; }
	~CExpression()		{ if (m_Equation != 0) delete m_Equation; };
	MString				m_Literal;
	CRpnEquation*		m_Equation;
};

class CMacro : public VTObject
{
public:
	CMacro()			{ m_ParamList = 0; m_DefList = 0; }
	~CMacro();

	DECLARE_DYNCREATE(CMacro);

	MString				m_Name;
	VTObArray*			m_ParamList;
	VTObArray*			m_DefList;
	MString				m_DefString;
};

class CExtern : public VTObject
{
public:
	CExtern()			{ m_Address = 0; m_pRange = 0; };

	MString				m_Name;
	unsigned int		m_Address;
	unsigned short		m_Segment;
	unsigned short		m_SymIdx;
	AddrRange*			m_pRange;
	unsigned char		m_Size;
};

class CLinkerEquation : public VTObject
{
public:
	CLinkerEquation()	{ m_Address = 0; m_pRange = 0; m_pRpnEq = 0; };
    ~CLinkerEquation()  { if (m_pRpnEq) delete m_pRpnEq; }

	unsigned int		m_Address;
	unsigned int		m_Line;
	unsigned short		m_Segment;
	unsigned short		m_SegIdx;
	AddrRange*			m_pRange;
	CRpnEquation*		m_pRpnEq;
    MString             m_Sourcefile;
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

	unsigned int		m_Address;
    int                 m_Size;
	CSegment*			m_Segment;
	AddrRange*			m_pSourceRange;
	AddrRange*			m_pTargetRange;
};

class CSymbol : public VTObject
{
public:
	CSymbol() { m_Line = -1; m_Value = -1; m_SymType = 0; m_Equation = NULL; 
			m_StrtabOffset = 0; m_Segment = NULL; m_FileIndex = -1;
			m_pRange = NULL; m_Off8 = 0; m_Off16 = 0; }
	~CSymbol()			{ if (m_Equation != 0) delete m_Equation; };

// Attributes

	MString				m_Name;
	CRpnEquation*		m_Equation;
	long				m_Line;
	long				m_Value;
	CSegment*			m_Segment;
	unsigned short		m_SymType;
	unsigned short		m_FileIndex;
	long				m_StrtabOffset;
	unsigned long		m_Off8;
	unsigned long		m_Off16;
	AddrRange*			m_pRange;
};

typedef void (*stdOutFunc_t)(void *pContext, const char *msg);

class CSegLines : public VTObject
{
public:
	CSegLines(CSegment* pSeg, int start)  
		{ pSegment = pSeg, startLine = start, lastLine = -1;  }

	CSegment*	pSegment;					// Pointer to the segment
	int			startLine;					// First line for this entry
	int			lastLine;					// Last line for this entry
};

class VTAssembler : public VTObject
{
public:
	VTAssembler();
	~VTAssembler();

	int					PerformSubstitution(CMacro* pMacro, const char* pLoc);
	int					MacroSubstitution(void);
	int					preprocessor(void);

	// Define Preprocessor functions
	void				preproc_endif(void);
	void				preproc_ifndef(const char *name);
	void				preproc_if(void);
	void				preproc_elif(void);
	void				preproc_ifdef(const char *name, int negate = 0);
	void				preproc_else(void);
	int					preproc_error(const char *msg);
	void				preproc_define();
	void				preproc_undef(const char *name);
	int					preproc_macro(void);

	// Define Pragma functions
	void				pragma_list();
	void				pragma_hex();
	void				pragma_verilog();
	void				pragma_extended();
	void				pragma_entry(const char *pName);

	// Define directive functions
	void				directive_set(const char *name);
	void				directive_aseg(void);
	void				directive_cdseg(int seg, int page);
	void				directive_ds(void);
	void				directive_db(void);
	void				directive_dw(void);
	void				directive_echo(void);
	void				directive_echo(const char *msg);
	void				directive_fill(void);
	void				directive_printf(const char *fmt, int hasEquation = 1);
	void				directive_extern(void);
	void				directive_endian(int msbFirst);
	void				directive_org();
	void				directive_public(void);
	void				directive_name(const char *name);
	void				directive_stkln(void);
	void				directive_end(const char *name);
	void				directive_if(int inst = INST_IF);
	void				directive_else(void);
	void				directive_module(const char *name);
	void				directive_endif(void);
	void				directive_title(const char *name);
	void				directive_link(const char *name);
	void				directive_maclib(const char *name);
	void				directive_sym(void);
	void				directive_page(int page);

	void				include(const char *filename);
	void				equate(const char *name);
	void				label(const char *label, int address = -1);
	
	void				opcode_arg_0(int opcode);
	void				opcode_arg_1reg(int opcode);
	void				opcode_arg_imm(int opcode, char c);
	void				opcode_arg_2reg(int opcode);
	void				opcode_arg_1reg_equ8(int opcode);
	void				opcode_arg_1reg_2byte(int opcode);
	void				opcode_arg_1reg_equ16(int opcode);
	void				opcode_arg_equ8(int opcode);
	void				opcode_arg_equ16(int opcode);
	void				opcode_arg_equ24(int opcode);

// Attributes
//	MString				m_Filename;			// Filename that design was parsed from
	MString				m_FileDir;
	FILE*				m_fd;				// File descriptor of open file
	int					m_Line;
	char*				m_pInLine;			// Pointer to expanded unput line
	char*				m_pInPtr;			// Pointer to next char to process
	int					m_InLineCount;		// Size of m_pInLine buffer
	int					m_LastLabelLine;	// Line number of last label
	int					m_FileIndex;
	int					m_ProjectType;
	int					m_LastIfElseLine;	// Line number of last #if, #ifdef, IF, or else
	int					m_LastIfElseIsIf;	// True if last was #if or #ifdef

	VTMapStringToOb		m_Modules;			// Map of CModules
	VTMapStringToOb		m_Segments;			// Map of CSegments
	CModule*			m_ActiveMod;		// Pointer to active CModule
	CSegment*			m_ActiveSeg;		// Active segment for assembly
	AddrRange*			m_ActiveAddr;		// Pointer to active address range from segment
	CSegLines*			m_ActiveSegLines;	// Active segment line tracker
	VTMapStringToOb*	m_Symbols;			// Array of Symbols
	VTObArray*			m_Instructions;		// Array of Instructions
	VTObArray			m_Defines;			// Array of preprocessor defines

	VTObArray			m_SegLines;			// Array of segment line objects
	unsigned int		m_Address;
	MStringArray		m_Filenames;		// Array of filenames parsed during assembly
	MString				m_LastLabel;		// Save value of last label parsed
	MString				m_EntryLabel;		// Label of program entry location
	CSymbol*			m_LastLabelSym;		// Pointer to CSymbol object for last label
	int					m_LastLabelAdded;
	char				m_LocalModuleChar;	// Module local label starting character

	stdOutFunc_t		m_pStdoutFunc;		// Standard out message routine
	void*				m_pStdoutContext;   // Opaque context for stdout
	MStringArray		m_Errors;			// Array of error messages during parsing
	VTObArray			m_Externs;          // Array of externs to add to ELF file
	VTObArray			m_Equations;        // Array of equations to add to ELF file
	VTMapStringToOb		m_UndefSymbols;
	int					m_List;				// Create a list file?
	int					m_Hex;				// Create a HEX file?
	int					m_Verilog;			// Create a Verilog $readmem file
	int					m_Extended;			// Indicated extended instructions valid
	int					m_DebugInfo;		// Include debug info in .obj?
	int					m_MsbFirst;			// Output WORDS MSB first instead of LSB
	MString				m_IncludeName[32];
	FILE*				m_IncludeStack[32];
	int					m_IncludeIndex[32];
	char*				m_IncludeInLine[32];
	char*				m_IncludeInPtr[32];
	int					m_IncludeInLineCount[32];
	a85parse_pcb_struct m_ParserPCBs[32];
	int					m_IncludeDepth;
	MString				m_AsmOptions;		// Assembler options
	MString				m_IncludePath;
	MString				m_ExtDefines;		// External additional defines
	MString				m_RootPath;			// Root path of project.
	MStringArray		m_IncludeDirs;		// Array of '/' terminated include dirs

	char				m_IfStat[100];
	int					m_IfDepth;


// Operations
	int					Evaluate(class CRpnEquation* eq, double* value,  
							int reportError, MString& errVariable);
	inline int			Evaluate(class CRpnEquation* eq, double* value,  
							int reportError)
							{
								MString str;
								return Evaluate(eq, value, reportError, str);
							}
	int					Assemble();
	int					GetValue(MString & string, int & value);
	int					LookupSymbol(MString& name, CSymbol *& symbol);
	int					LookupMacro(MString& name, CMacro *& macro);
	CSymbol*			LookupSymOtherModules(MString& name, CSegment** pSeg = NULL);
	void				ResetContent(void);
	int					CreateObjFile(const char *filename, const char *sourcefile);
	int					InvalidRelocation(CRpnEquation* pEq, char &rel_mask, CSegment *&pSeg,
							char &pcRel);
	int					EquationIsExtern(CRpnEquation* pEq, int size);
	void				MakeBinary(int val, int length, MString& binary);
	void				CreateHex(MString& filename);
	void				CreateCO(MString& filename);
	void				CreateVerilog(MString& filename);
	void				CreateList(MString& filename, MString& asmFilename);
	void				CalcIncludeDirs();
	void				ParseExternalDefines(void);
	void				ActivateSegment(CSegment* pSeg);
	CInstruction*		AddInstruction(int opcode);

// Public Access functions
	void				Parse(MString filename);
	void				SetAsmOptions(const MString& options);
	void				SetIncludeDirs(const MString& dirs);
	void				SetDefines(const MString& defines);
	void				SetRootPath(const MString& rootPath);
	void				SetProjectType(int type);
	void				SetStdoutFunction(void *pContext, stdOutFunc_t pFunc);
	const MStringArray&		GetErrors() { return m_Errors; };
};

#endif
