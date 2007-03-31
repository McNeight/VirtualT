// 8085 Assembler Parser Syntax

// Written by Kenneth D. Pettit
//	      This is an input syntax grammer for the Anagram LARL Parser Generator


/*

 AnaGram Parsing Engine
 Copyright (c) 1993-2000, Parsifal Software.
 All Rights Reserved.

 Serial number 2P18859U
 Registered to:
   Kenneth D. Pettit
   Conexant Systems, Inc.

*/

#ifndef A85PARSE_H_1147178743
#include "a85parse.h"
#endif

#ifndef A85PARSE_H_1147178743
#error Mismatched header file
#endif

#include <ctype.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>

#define RULE_CONTEXT (&((PCB).cs[(PCB).ssx]))
#define ERROR_CONTEXT ((PCB).cs[(PCB).error_frame_ssx])
#define CONTEXT ((PCB).cs[(PCB).ssx])



a85parse_pcb_type a85parse_pcb;
#define PCB a85parse_pcb

/*  Line 483, C:/Projects/VirtualT/src/asm/a85parse.syn */
                       //  Embedded C

#include "MString.h"							// Include MString header
#include "MStringArray.h"						// Include MStringArray header
#include "assemble.h"							// Include VTAssembler header

#ifndef BOOL
typedef int     BOOL;
#endif

#ifndef TRUE
#define TRUE    1
#define FALSE   0
#endif

VTAssembler*            gAsm = 0;				// Pointer to the assembler
CRpnEquation*			gEq = 0;				// Pointer to active equation
VTObArray*				gExpList = 0;			// Pointer to active expression list
MStringArray*			gNameList = 0;			// Pointer to active name list
CCondition*				gCond = 0;				// Pointer to active condition
char					ss[32][256];			// String Stack;
char					integer[64];			// Integer storage space
char					int_len = 0;			// Integer string length
int						ss_idx = 0;				// String Stack Index
int						ss_len = 0;				// SS string length 
double                  gDivisor = 1.0;			// Current divisor for float converwions
int						gTabSize = 4;
char					reg[10];				// Register arguments
int						reg_cnt = 0;			// Register Arg count
int						label = 0;				// Number of labels on string stack
int						name_list_cnt = 0;		// Number of strings in name list
int						ex_cnt = 0;				// Number of expressions in expression list

void a85parse(void);

// This function checks the string accumulator for errors
// parsing the file.  If there are no errors, check the string
// accumulator to insure it is empty.  If anything is left in the
// accumulator, then there is an error with our parser.
void check_string_stack(void)
{
	// If we have no errors...
	if (gAsm->m_Errors.GetSize() == 0)
	{
		// Check string accumulator size
		int stringCount;
		if ((stringCount = ss_idx) != 0)
		{
			MString string;
			string.Format("Internal Design Parse Error - %d string(s) left on stack!",
				stringCount);
			gAsm->m_Errors.Add(string);
		}
	}
}

// ParseDesignFile is the entry point for parsing design files.  The full path of
// the design file should be passed as the only parameter.  If the design file is
// parsed successfully, the routine will return TRUE, otherwise it will return
// FALSE.
BOOL ParseASMFile(const char* filename, VTAssembler* pAsm)
{
	if (pAsm == NULL)
		return FALSE;

	gAsm = pAsm;

	// Clear old design from the CDesign object
	gAsm->ResetContent();

	// Insure no active equation left over from a previous run
	if (gEq != 0)
		delete gEq;
	if (gExpList != 0)
		delete gExpList;
	if (gNameList != 0)
		delete gNameList;
	if (gCond != 0)
		delete gCond;

	// Allocate an active equation to be used during parsing
	gEq = new CRpnEquation;
	gExpList = new VTObArray;
	gNameList = new MStringArray;
	gCond = new CCondition;
	gAsm->m_FileIndex = gAsm->m_Filenames.Add(filename);

	// Try to open the file
	if ((gAsm->m_fd = fopen(gAsm->m_Filenames[gAsm->m_FileIndex], "r")) == 0)
		return FALSE;

	// Reset the string stack
	ss_idx = 0;
	ss_len = 0;
	int_len = 0;

	// Reset the Register, name, expression, and label counters
	reg_cnt = label = ex_cnt = name_list_cnt = 0;

	// Parse the file.
	a85parse();

	fclose(gAsm->m_fd);

	// Check for parser errors
	check_string_stack();

	// Check if errors occured
	int count = gAsm->m_Errors.GetSize();
	if (count != 0)
		return FALSE;

	// No errors!
	return TRUE;
}

// Define macros to handle input and errors

#define TAB_SPACING gTabSize

#define GET_INPUT ((PCB).input_code = gAsm->m_fd != 0 ? \
	fgetc(gAsm->m_fd) : 0)

#define SYNTAX_ERROR {MString string;  string.Format("%s, line %d, column %d", \
  (PCB).error_message, (PCB).line, (PCB).column); gAsm->m_Errors.Add(string); }

#define PARSER_STACK_OVERFLOW {MString string;  string.Format(\
   "\nParser stack overflow, line %d, column %d",\
   (PCB).line, (PCB).column); gAsm->m_Errors.Add(string);}

#define REDUCTION_TOKEN_ERROR {MString string;  string.Format(\
    "\nReduction token error, line %d, column %d", \
    (PCB).line, (PCB).column); gAsm->m_Errors.Add(string);}

int do_hex(int h, int n)
{
	int temp1, temp2;

	temp2 = h * 16;
	temp1 = (n <='9' ? n-'0' : (n | 0x60) - 'a' + 10);
	temp1 += temp2;
	return temp1;
}

void expression_list_literal()
{
	CExpression *pExp = new CExpression;

	pExp->m_Literal = ss[ss_idx--];
	gExpList->Add(pExp);
}

void expression_list_equation()
{
	CExpression *pExp = new CExpression;

	pExp->m_Equation = gEq;
	gEq = new CRpnEquation;

	gExpList->Add(pExp);
}

void condition(int condition)
{
	if (condition == -1)
	{
		// Get Right hand side equation
		gCond->m_EqRight = gEq;
	}
	else
	{
		// Save conditional
		gCond->m_Condition = condition;
		gCond->m_EqLeft = gEq;
	}

		// Assign new equation for parser
	gEq = new CRpnEquation;
}

int conv_to_hex()
{
	int temp1;

	sscanf(integer, "%x", &temp1);
	
	return temp1;
}

int conv_to_dec()
{
	int temp1;

	sscanf(integer, "%d", &temp1);
	
	return temp1;
}

int conv_to_oct()
{
	int temp1;

	sscanf(integer, "%o", &temp1);
	
	return temp1;
}

int conv_to_bin()
{
	int temp1, x;

	temp1 = 0;
	x = 0;

	while (integer[x] != 0)
		temp1 = (temp1 << 1) + integer[x++] - '0';
		
	return temp1;
}


#ifndef CONVERT_CASE

static const char agCaseTable[31] = {
  0x20, 0x20, 0x20, 0x20, 0x20, 0x20, 0x20, 0x20,
  0x20, 0x20, 0x20, 0x20, 0x20, 0x20, 0x20, 0x20,
  0x20, 0x20, 0x20, 0x20, 0x20, 0x20, 0x20,    0,
  0x20, 0x20, 0x20, 0x20, 0x20, 0x20, 0x20
};

static int agConvertCase(int c) {
  if (c >= 'a' && c <= 'z') return c ^= 0x20;
  if (c >= 0xe0 && c < 0xff) c ^= agCaseTable[c-0xe0];
  return c;
}

#define CONVERT_CASE(c) agConvertCase(c)

#endif


#ifndef TAB_SPACING
#define TAB_SPACING 8
#endif

#define ag_rp_1() (gAsm->include(ss[ss_idx--]))

#define ag_rp_6() (gAsm->pragma_list())

#define ag_rp_7() (gAsm->pragma_hex())

#define ag_rp_8() (gAsm->preproc_ifdef(ss[ss_idx--]))

#define ag_rp_9() (gAsm->preproc_ifndef(ss[ss_idx--]))

#define ag_rp_10() (gAsm->preproc_else())

#define ag_rp_11() (gAsm->preproc_endif())

#define ag_rp_12() (gAsm->preproc_define(ss[ss_idx--]))

#define ag_rp_13() (gAsm->preproc_undef(ss[ss_idx--]))

#define ag_rp_14() (gAsm->equate(ss[ss_idx--]))

#define ag_rp_15() (gAsm->directive_set(ss[ss_idx--]))

#define ag_rp_16() (gAsm->equate((const char *) -1))

#define ag_rp_17() (gAsm->directive_set((const char *) -1))

#define ag_rp_18() (gAsm->directive_org())

#define ag_rp_19() (gAsm->directive_aseg())

#define ag_rp_20() (gAsm->directive_cseg(0))

#define ag_rp_21() (gAsm->directive_dseg(0))

static void ag_rp_22(void) {
/* Line 100, C:/Projects/VirtualT/src/asm/a85parse.syn */
 gAsm->label(ss[ss_idx--]); gAsm->directive_ds(); 
}

#define ag_rp_23() (gAsm->directive_ds())

static void ag_rp_24(void) {
/* Line 102, C:/Projects/VirtualT/src/asm/a85parse.syn */
 gAsm->label(ss[ss_idx--]); gAsm->directive_db(); 
}

#define ag_rp_25() (gAsm->directive_db())

static void ag_rp_26(void) {
/* Line 104, C:/Projects/VirtualT/src/asm/a85parse.syn */
 gAsm->label(ss[ss_idx--]); gAsm->directive_dw(); 
}

#define ag_rp_27() (gAsm->directive_dw())

#define ag_rp_28() (gAsm->directive_extern())

#define ag_rp_29() (gAsm->directive_extern())

#define ag_rp_30() (gAsm->directive_public())

#define ag_rp_31() (gAsm->directive_name(ss[ss_idx--]))

#define ag_rp_32() (gAsm->directive_stkln())

#define ag_rp_33() (gAsm->directive_end(""))

#define ag_rp_34() (gAsm->directive_end(ss[ss_idx--]))

#define ag_rp_35() (gAsm->directive_if())

#define ag_rp_36() (gAsm->directive_else())

#define ag_rp_37() (gAsm->directive_endif())

#define ag_rp_38() (gAsm->directive_title(ss[ss_idx--]))

#define ag_rp_39() (gAsm->directive_title(ss[ss_idx--]))

#define ag_rp_40(n) (gAsm->directive_page(n))

#define ag_rp_41() (gAsm->directive_page(-1))

#define ag_rp_42() (gAsm->directive_sym())

#define ag_rp_43() (gAsm->directive_link(ss[ss_idx--]))

#define ag_rp_44() (gAsm->directive_maclib(ss[ss_idx--]))

#define ag_rp_45() (expression_list_literal())

#define ag_rp_46() (expression_list_equation())

#define ag_rp_47() (expression_list_equation())

#define ag_rp_48() (expression_list_literal())

#define ag_rp_49() (expression_list_literal())

#define ag_rp_50() (gNameList->Add(ss[ss_idx--]))

#define ag_rp_51() (gNameList->Add(ss[ss_idx--]))

static void ag_rp_52(void) {
/* Line 147, C:/Projects/VirtualT/src/asm/a85parse.syn */
 strcpy(ss[++ss_idx], "$"); ss_len = 1; 
}

static void ag_rp_53(int c) {
/* Line 150, C:/Projects/VirtualT/src/asm/a85parse.syn */
 ss[++ss_idx][0] = c; ss[ss_idx][1] = 0; ss_len = 1; 
}

static void ag_rp_54(int c) {
/* Line 151, C:/Projects/VirtualT/src/asm/a85parse.syn */
 ss[ss_idx][ss_len++] = c; ss[ss_idx][ss_len] = 0; 
}

static void ag_rp_55(int c) {
/* Line 152, C:/Projects/VirtualT/src/asm/a85parse.syn */
 ss[ss_idx][ss_len++] = c; ss[ss_idx][ss_len] = 0; 
}

static void ag_rp_56(int c) {
/* Line 155, C:/Projects/VirtualT/src/asm/a85parse.syn */
 ss[++ss_idx][0] = c; ss[ss_idx][1] = 0; ss_len = 1; 
}

static void ag_rp_57(int c) {
/* Line 156, C:/Projects/VirtualT/src/asm/a85parse.syn */
 ss[ss_idx][ss_len++] = c; ss[ss_idx][ss_len] = 0; 
}

static void ag_rp_58(int ch1, int ch2) {
/* Line 162, C:/Projects/VirtualT/src/asm/a85parse.syn */
 ss_idx++; ss_len = 2; sprintf(ss[ss_idx], "%c%c", ch1, ch2); 
}

static void ag_rp_59(int c) {
/* Line 163, C:/Projects/VirtualT/src/asm/a85parse.syn */
 ss[ss_idx][ss_len++] = c; ss[ss_idx][ss_len] = 0; 
}

static void ag_rp_60(void) {
/* Line 169, C:/Projects/VirtualT/src/asm/a85parse.syn */
 ss_idx++; ss_len = 0; 
}

static void ag_rp_61(int c) {
/* Line 170, C:/Projects/VirtualT/src/asm/a85parse.syn */
 ss[ss_idx][ss_len++] = c; ss[ss_idx][ss_len] = 0; 
}

static void ag_rp_62(int c) {
/* Line 171, C:/Projects/VirtualT/src/asm/a85parse.syn */
 ss[ss_idx][ss_len++] = '\\'; ss[ss_idx][ss_len++] = c; ss[ss_idx][ss_len] = 0; 
}

static void ag_rp_63(void) {
/* Line 175, C:/Projects/VirtualT/src/asm/a85parse.syn */
 ss[ss_idx][ss_len++] = '>'; ss[ss_idx][ss_len] = 0; 
}

static void ag_rp_64(void) {
/* Line 178, C:/Projects/VirtualT/src/asm/a85parse.syn */
 ss[++ss_idx][0] = '<'; ss[ss_idx][1] = 0; ss_len = 1; 
}

static void ag_rp_65(int c) {
/* Line 179, C:/Projects/VirtualT/src/asm/a85parse.syn */
 ss[ss_idx][ss_len++] = c; ss[ss_idx][ss_len] = 0; 
}

static void ag_rp_66(int c) {
/* Line 180, C:/Projects/VirtualT/src/asm/a85parse.syn */
 ss[ss_idx][ss_len++] = '\\'; ss[ss_idx][ss_len++] = c; ss[ss_idx][ss_len] = 0; 
}

#define ag_rp_67() (gAsm->label(ss[ss_idx--]))

#define ag_rp_68() (condition(-1))

#define ag_rp_69() (condition(COND_NOCMP))

#define ag_rp_70() (condition(COND_EQ))

#define ag_rp_71() (condition(COND_NE))

#define ag_rp_72() (condition(COND_GE))

#define ag_rp_73() (condition(COND_LE))

#define ag_rp_74() (condition(COND_GT))

#define ag_rp_75() (condition(COND_LT))

#define ag_rp_76() (gEq->Add(RPN_BITOR))

#define ag_rp_77() (gEq->Add(RPN_BITOR))

#define ag_rp_78() (gEq->Add(RPN_BITXOR))

#define ag_rp_79() (gEq->Add(RPN_BITXOR))

#define ag_rp_80() (gEq->Add(RPN_BITAND))

#define ag_rp_81() (gEq->Add(RPN_BITAND))

#define ag_rp_82() (gEq->Add(RPN_LEFTSHIFT))

#define ag_rp_83() (gEq->Add(RPN_LEFTSHIFT))

#define ag_rp_84() (gEq->Add(RPN_RIGHTSHIFT))

#define ag_rp_85() (gEq->Add(RPN_RIGHTSHIFT))

#define ag_rp_86() (gEq->Add(RPN_ADD))

#define ag_rp_87() (gEq->Add(RPN_SUBTRACT))

#define ag_rp_88() (gEq->Add(RPN_MULTIPLY))

#define ag_rp_89() (gEq->Add(RPN_DIVIDE))

#define ag_rp_90() (gEq->Add(RPN_MODULUS))

#define ag_rp_91() (gEq->Add(RPN_MODULUS))

#define ag_rp_92() (gEq->Add(RPN_EXPONENT))

#define ag_rp_93() (gEq->Add(RPN_NOT))

#define ag_rp_94() (gEq->Add(RPN_NOT))

#define ag_rp_95() (gEq->Add(RPN_BITNOT))

#define ag_rp_96(n) (gEq->Add((double) n))

#define ag_rp_97() (gEq->Add(ss[ss_idx--]))

#define ag_rp_98() (gEq->Add(RPN_FLOOR))

#define ag_rp_99() (gEq->Add(RPN_CEIL))

#define ag_rp_100() (gEq->Add(RPN_LN))

#define ag_rp_101() (gEq->Add(RPN_LOG))

#define ag_rp_102() (gEq->Add(RPN_SQRT))

#define ag_rp_103() (gEq->Add(RPN_IP))

#define ag_rp_104() (gEq->Add(RPN_FP))

#define ag_rp_105(n) (n)

#define ag_rp_106(r) (r)

#define ag_rp_107(n) (n)

#define ag_rp_108() (conv_to_dec())

#define ag_rp_109() (conv_to_hex())

#define ag_rp_110() (conv_to_hex())

#define ag_rp_111() (conv_to_hex())

#define ag_rp_112() (conv_to_bin())

#define ag_rp_113() (conv_to_oct())

#define ag_rp_114() (conv_to_dec())

static void ag_rp_115(int n) {
/* Line 304, C:/Projects/VirtualT/src/asm/a85parse.syn */
 int_len = 1; integer[0] = n; integer[1] = 0; 
}

static void ag_rp_116(int n) {
/* Line 305, C:/Projects/VirtualT/src/asm/a85parse.syn */
 int_len = 2; integer[0] = '-', integer[1] = n; integer[2] = 0; 
}

static void ag_rp_117(int n) {
/* Line 306, C:/Projects/VirtualT/src/asm/a85parse.syn */
 int_len = 1; integer[0] = n; integer[1] = 0; 
}

static void ag_rp_118(int n) {
/* Line 307, C:/Projects/VirtualT/src/asm/a85parse.syn */
 integer[int_len++] = n; integer[int_len] = 0; 
}

static void ag_rp_119(int n) {
/* Line 312, C:/Projects/VirtualT/src/asm/a85parse.syn */
 int_len = 1; integer[0] = n; integer[1] = 0; 
}

static void ag_rp_120(int n) {
/* Line 313, C:/Projects/VirtualT/src/asm/a85parse.syn */
 integer[int_len++] = n; integer[int_len] = 0; 
}

static void ag_rp_121(int n) {
/* Line 316, C:/Projects/VirtualT/src/asm/a85parse.syn */
 integer[int_len++] = n; integer[int_len] = 0; 
}

static void ag_rp_122(int n) {
/* Line 317, C:/Projects/VirtualT/src/asm/a85parse.syn */
 integer[int_len++] = n; integer[int_len] = 0; 
}

#define ag_rp_123(n) (n)

#define ag_rp_124() ('\\')

#define ag_rp_125(n) (n)

#define ag_rp_126() ('\\')

#define ag_rp_127() ('\n')

#define ag_rp_128() ('\t')

#define ag_rp_129() ('\r')

#define ag_rp_130() ('\0')

#define ag_rp_131() ('\'')

#define ag_rp_132() ('\'')

static double ag_rp_133(void) {
/* Line 337, C:/Projects/VirtualT/src/asm/a85parse.syn */
 gDivisor = 1.0; return (double) conv_to_dec(); 
}

static double ag_rp_134(int d) {
/* Line 338, C:/Projects/VirtualT/src/asm/a85parse.syn */
 gDivisor = 10.0; return ((double) (d - '0') / gDivisor); 
}

static double ag_rp_135(double r, int d) {
/* Line 339, C:/Projects/VirtualT/src/asm/a85parse.syn */
 gDivisor *= 10.0; return (r + (double) (d - '0') / gDivisor); 
}

#define ag_rp_136() (reg[reg_cnt++] = '0')

#define ag_rp_137() (reg[reg_cnt++] = '1')

#define ag_rp_138() (reg[reg_cnt++] = '2')

#define ag_rp_139() (reg[reg_cnt++] = '3')

#define ag_rp_140() (reg[reg_cnt++] = '4')

#define ag_rp_141() (reg[reg_cnt++] = '5')

#define ag_rp_142() (reg[reg_cnt++] = '6')

#define ag_rp_143() (reg[reg_cnt++] = '7')

#define ag_rp_144() (reg[reg_cnt++] = '0')

#define ag_rp_145() (reg[reg_cnt++] = '1')

#define ag_rp_146() (reg[reg_cnt++] = '2')

#define ag_rp_147() (reg[reg_cnt++] = '3')

#define ag_rp_148() (reg[reg_cnt++] = '0')

#define ag_rp_149() (reg[reg_cnt++] = '1')

#define ag_rp_150() (reg[reg_cnt++] = '2')

#define ag_rp_151() (reg[reg_cnt++] = '3')

#define ag_rp_152() (reg[reg_cnt++] = '3')

#define ag_rp_153() (reg[reg_cnt++] = '0')

#define ag_rp_154() (reg[reg_cnt++] = '1')

#define ag_rp_155() (gAsm->opcode_arg_equ8		(OPCODE_ACI))

#define ag_rp_156() (gAsm->opcode_arg_1reg		(OPCODE_ADC))

#define ag_rp_157() (gAsm->opcode_arg_1reg		(OPCODE_ADD))

#define ag_rp_158() (gAsm->opcode_arg_equ8		(OPCODE_ADI))

#define ag_rp_159() (gAsm->opcode_arg_1reg		(OPCODE_ANA))

#define ag_rp_160() (gAsm->opcode_arg_equ8		(OPCODE_ANI))

#define ag_rp_161() (gAsm->opcode_arg_0		(OPCODE_ASHR))

#define ag_rp_162() (gAsm->opcode_arg_equ16	(OPCODE_CALL))

#define ag_rp_163() (gAsm->opcode_arg_equ16	(OPCODE_CC))

#define ag_rp_164() (gAsm->opcode_arg_equ16	(OPCODE_CM))

#define ag_rp_165() (gAsm->opcode_arg_0		(OPCODE_CMA))

#define ag_rp_166() (gAsm->opcode_arg_0		(OPCODE_CMC))

#define ag_rp_167() (gAsm->opcode_arg_1reg		(OPCODE_CMP))

#define ag_rp_168() (gAsm->opcode_arg_equ16	(OPCODE_CNC))

#define ag_rp_169() (gAsm->opcode_arg_equ16	(OPCODE_CNZ))

#define ag_rp_170() (gAsm->opcode_arg_equ16	(OPCODE_CP))

#define ag_rp_171() (gAsm->opcode_arg_equ16	(OPCODE_CPE))

#define ag_rp_172() (gAsm->opcode_arg_equ8		(OPCODE_CPI))

#define ag_rp_173() (gAsm->opcode_arg_equ16	(OPCODE_CPO))

#define ag_rp_174() (gAsm->opcode_arg_equ16	(OPCODE_CZ))

#define ag_rp_175() (gAsm->opcode_arg_0		(OPCODE_DAA))

#define ag_rp_176() (gAsm->opcode_arg_1reg		(OPCODE_DAD))

#define ag_rp_177() (gAsm->opcode_arg_1reg		(OPCODE_DCR))

#define ag_rp_178() (gAsm->opcode_arg_1reg		(OPCODE_DCX))

#define ag_rp_179() (gAsm->opcode_arg_0		(OPCODE_DI))

#define ag_rp_180() (gAsm->opcode_arg_0		(OPCODE_DSUB))

#define ag_rp_181() (gAsm->opcode_arg_0		(OPCODE_EI))

#define ag_rp_182() (gAsm->opcode_arg_0		(OPCODE_HLT))

#define ag_rp_183() (gAsm->opcode_arg_equ8		(OPCODE_IN))

#define ag_rp_184() (gAsm->opcode_arg_1reg		(OPCODE_INR))

#define ag_rp_185() (gAsm->opcode_arg_1reg		(OPCODE_INX))

#define ag_rp_186() (gAsm->opcode_arg_equ16	(OPCODE_JC))

#define ag_rp_187() (gAsm->opcode_arg_equ16	(OPCODE_JD))

#define ag_rp_188() (gAsm->opcode_arg_equ16	(OPCODE_JD))

#define ag_rp_189() (gAsm->opcode_arg_equ16	(OPCODE_JM))

#define ag_rp_190() (gAsm->opcode_arg_equ16	(OPCODE_JMP))

#define ag_rp_191() (gAsm->opcode_arg_equ16	(OPCODE_JNC))

#define ag_rp_192() (gAsm->opcode_arg_equ16	(OPCODE_JND))

#define ag_rp_193() (gAsm->opcode_arg_equ16	(OPCODE_JND))

#define ag_rp_194() (gAsm->opcode_arg_equ16	(OPCODE_JNZ))

#define ag_rp_195() (gAsm->opcode_arg_equ16	(OPCODE_JP))

#define ag_rp_196() (gAsm->opcode_arg_equ16	(OPCODE_JPE))

#define ag_rp_197() (gAsm->opcode_arg_equ16	(OPCODE_JPO))

#define ag_rp_198() (gAsm->opcode_arg_equ16	(OPCODE_JZ))

#define ag_rp_199() (gAsm->opcode_arg_equ16	(OPCODE_LDA))

#define ag_rp_200() (gAsm->opcode_arg_1reg		(OPCODE_LDAX))

#define ag_rp_201() (gAsm->opcode_arg_equ8		(OPCODE_LDEH))

#define ag_rp_202() (gAsm->opcode_arg_equ8		(OPCODE_LDES))

#define ag_rp_203() (gAsm->opcode_arg_equ16	(OPCODE_LHLD))

#define ag_rp_204() (gAsm->opcode_arg_0		(OPCODE_LHLX))

#define ag_rp_205() (gAsm->opcode_arg_1reg_equ8(OPCODE_LXI))

#define ag_rp_206() (gAsm->opcode_arg_2reg		(OPCODE_MOV))

#define ag_rp_207() (gAsm->opcode_arg_1reg_equ8(OPCODE_MVI))

#define ag_rp_208() (gAsm->opcode_arg_0		(OPCODE_NOP))

#define ag_rp_209() (gAsm->opcode_arg_1reg		(OPCODE_ORA))

#define ag_rp_210() (gAsm->opcode_arg_equ8		(OPCODE_ORI))

#define ag_rp_211() (gAsm->opcode_arg_equ8		(OPCODE_OUT))

#define ag_rp_212() (gAsm->opcode_arg_0		(OPCODE_PCHL))

#define ag_rp_213() (gAsm->opcode_arg_1reg		(OPCODE_POP))

#define ag_rp_214() (gAsm->opcode_arg_1reg		(OPCODE_PUSH))

#define ag_rp_215() (gAsm->opcode_arg_0		(OPCODE_RAL))

#define ag_rp_216() (gAsm->opcode_arg_0		(OPCODE_RAR))

#define ag_rp_217() (gAsm->opcode_arg_0		(OPCODE_RC))

#define ag_rp_218() (gAsm->opcode_arg_0		(OPCODE_RET))

#define ag_rp_219() (gAsm->opcode_arg_0		(OPCODE_RIM))

#define ag_rp_220() (gAsm->opcode_arg_0		(OPCODE_RLC))

#define ag_rp_221() (gAsm->opcode_arg_0		(OPCODE_RLDE))

#define ag_rp_222() (gAsm->opcode_arg_0		(OPCODE_RM))

#define ag_rp_223() (gAsm->opcode_arg_0		(OPCODE_RNC))

#define ag_rp_224() (gAsm->opcode_arg_0		(OPCODE_RNZ))

#define ag_rp_225() (gAsm->opcode_arg_0		(OPCODE_RP))

#define ag_rp_226() (gAsm->opcode_arg_0		(OPCODE_RPE))

#define ag_rp_227() (gAsm->opcode_arg_0		(OPCODE_RPO))

#define ag_rp_228() (gAsm->opcode_arg_0		(OPCODE_RRC))

#define ag_rp_229(c) (gAsm->opcode_arg_imm		(OPCODE_RST, c))

#define ag_rp_230() (gAsm->opcode_arg_0		(OPCODE_RZ))

#define ag_rp_231() (gAsm->opcode_arg_1reg		(OPCODE_SBB))

#define ag_rp_232() (gAsm->opcode_arg_equ8		(OPCODE_SBI))

#define ag_rp_233() (gAsm->opcode_arg_equ16	(OPCODE_SHLD))

#define ag_rp_234() (gAsm->opcode_arg_0		(OPCODE_SHLX))

#define ag_rp_235() (gAsm->opcode_arg_0		(OPCODE_SIM))

#define ag_rp_236() (gAsm->opcode_arg_0		(OPCODE_SPHL))

#define ag_rp_237() (gAsm->opcode_arg_equ16	(OPCODE_STA))

#define ag_rp_238() (gAsm->opcode_arg_1reg		(OPCODE_STAX))

#define ag_rp_239() (gAsm->opcode_arg_0		(OPCODE_STC))

#define ag_rp_240() (gAsm->opcode_arg_1reg		(OPCODE_SUB))

#define ag_rp_241() (gAsm->opcode_arg_equ8		(OPCODE_SUI))

#define ag_rp_242() (gAsm->opcode_arg_0		(OPCODE_XCHG))

#define ag_rp_243() (gAsm->opcode_arg_1reg		(OPCODE_XRA))

#define ag_rp_244() (gAsm->opcode_arg_equ8		(OPCODE_XRI))

#define ag_rp_245() (gAsm->opcode_arg_0		(OPCODE_XTHL))


#define READ_COUNTS 
#define WRITE_COUNTS 
#undef V
#define V(i,t) (*(t *) (&(PCB).vs[(PCB).ssx + i]))
#undef VS
#define VS(i) (PCB).vs[(PCB).ssx + i]

#ifndef GET_CONTEXT
#define GET_CONTEXT CONTEXT = (PCB).input_context
#endif

typedef enum {
  ag_action_1,
  ag_action_2,
  ag_action_3,
  ag_action_4,
  ag_action_5,
  ag_action_6,
  ag_action_7,
  ag_action_8,
  ag_action_9,
  ag_action_10,
  ag_action_11,
  ag_action_12
} ag_parser_action;

#ifndef NULL_VALUE_INITIALIZER
#define NULL_VALUE_INITIALIZER = { 0 }
#endif


static a85parse_vs_type const ag_null_value NULL_VALUE_INITIALIZER;

static const unsigned char ag_rpx[] = {
    0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,
    0,  0,  0,  0,  0,  0,  0,  1,  1,  1,  1,  1,  2,  3,  4,  5,  6,  7,
    8,  9, 10, 11, 12, 13,  0,  0, 14, 15, 16, 17, 18, 19, 20, 21, 22, 23,
   24, 25, 26, 27, 28, 29, 30, 31, 32, 33, 34, 35, 36, 37, 38, 39, 40,  0,
   41,  0, 42, 43, 44, 45, 46, 47,  0, 48, 49, 50, 51, 52, 53,  0, 54, 55,
    0, 56, 57, 58,  0, 59, 60, 61, 62,  0, 63, 64, 65,  0,  0,  0, 66,  0,
    0, 67,  0,  0, 68,  0,  0, 69,  0,  0, 70,  0,  0, 71,  0,  0, 72, 73,
    0, 74, 75,  0, 76, 77,  0, 78, 79, 80, 81,  0, 82, 83,  0, 84, 85, 86,
   87, 88,  0, 89, 90, 91, 92, 93,  0,  0, 94, 95, 96, 97, 98, 99,100,101,
  102,103,104,105,106,107,108,109,110,111,112,113,114,  0,  0,115,116,117,
  118,119,120,121,122,123,124,125,126,127,128,  0,129,130,131,132,133,134,
  135,136,137,138,139,140,141,142,143,144,145,146,147,148,149,150,  0,  0,
    0,151,152,153,154,155,156,157,158,159,160,161,162,163,164,165,166,167,
  168,169,170,171,172,173,174,175,176,177,178,179,180,181,182,183,184,185,
  186,187,188,189,190,191,192,193,194,195,196,197,198,199,200,201,202,203,
  204,205,206,207,208,209,210,211,212,213,214,215,216,217,218,219,220,221,
  222,223,224,225,226,227,228,229,230,231,232,233,234,235,236,237,238,239,
  240,241
};

static const unsigned char ag_key_itt[] = {
 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 1, 0, 0, 0, 0, 0, 0, 1, 0, 1, 0,
 0, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 0, 0, 0, 0, 0, 0, 0, 1, 1, 1, 1, 1, 1,
 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 0, 0, 0, 0,
 1, 0, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1,
 1, 1, 1, 1, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
 0
};

static const unsigned short ag_key_pt[] = {
  1,291,  1,295,  1,297,  1,305,  1,306,  1,307,  1,308,  1,309,
  1,310,  1,311,  1,312,  1,313,  1,314,  1,315,  1,316,  1,317,
  1,318,  1,319,  1,320,  1,321,  1,322,  1,324,  1,326,  1,327,
  1,328,  1,329,  1, 85,  1,331,  1,334,  1,336,  1,338,  1,340,
  1,342,  1,345,  1,347,  1,349,  1,351,  1,353,  1,359,  1,362,
  1,366,  1,367,  1,368,  1,369,  1,370,  1,371,  1,372,  1,376,
  1,374,  1,159,  1,375,  1,175,  1,176,  1,177,  1,178,  1,378,
  1,180,  1,377,  1,379,  1,381,  1,382,  1,383,  1,384,  1,385,
  1,386,  1,387,  1,388,  1,389,  1,390,  1,391,  1,392,  1,393,
  1,394,  1,395,  1,396,  1,397,  1,398,  1,399,  1,400,  1,402,
  1,403,  1,404,  1,405,  1,406,  1,407,  1,408,  1,409,  1,410,
  1,411,  1,412,  1,413,  1,414,  1,415,  1,416,  1,417,  1,418,
  1,419,  1,420,  1,421,  1,422,  1,423,  1,424,  1,425,  1,426,
  1,427,  1,428,  1,429,  1,430,  1,431,  1,432,  1,433,  1,434,
  1,435,  1,436,  1,437,  1,438,  1,439,  1,440,  1,441,  1,442,
  1,443,  1,444,  1,445,  1,446,  1,447,  1,448,  1,449,  1,450,
  1,451,  1,452,  1,453,  1,454,  1,455,  1,456,  1,457,  1,458,
  1,459,  1,460,  1,461,  1,462,  1,463,  1,464,  1,465,  1,466,
  1,467,  1,468,  1,469,  1,470,  1,471,0
};

static const unsigned char ag_key_ch[] = {
    0, 76, 78,255, 68, 78,255, 70, 78,255, 68, 69, 73, 80, 85,255, 42, 47,
  255, 67, 68, 73,255, 65, 73,255, 69, 72,255, 67, 68, 78, 83,255, 65, 67,
   80,255, 67, 90,255, 69, 73, 79,255, 65, 67, 77, 78, 80, 83, 90,255, 65,
   68,255, 82, 88,255, 69, 85,255, 65, 66, 67, 73, 83, 87,255, 73,255, 68,
  255, 69, 82,255, 84,255, 73, 76, 78, 88,255, 67, 82, 88,255, 70, 78,255,
   80,255, 67, 68, 88, 90,255, 69, 79,255, 67, 68, 77, 78, 80, 88, 90,255,
   88,255, 72, 83,255, 65, 69,255, 68, 88,255, 76,255, 68, 72, 73, 88,255,
   65, 79, 86,255, 65, 79,255, 65, 71, 73,255, 82, 85,255, 66, 83,255, 65,
   67, 79, 85,255, 76, 82,255, 67, 68,255, 67, 90,255, 69, 79,255, 65, 67,
   69, 73, 76, 77, 78, 80, 82, 83, 90,255, 66, 73,255, 68, 88,255, 76,255,
   88,255, 65, 67, 75,255, 66, 73,255, 66, 72, 73, 80, 84, 85, 89,255, 65,
   73,255, 67, 82, 84,255, 35, 36, 47, 65, 67, 68, 69, 72, 73, 74, 76, 77,
   78, 79, 80, 82, 83, 84, 88,255, 76, 78,255, 68, 78,255, 70, 78,255, 68,
   69, 73, 80, 85,255, 42, 47,255, 60, 61,255, 61,255, 61, 62,255, 67, 68,
   73,255, 65, 68, 73,255, 69, 72,255, 67, 68, 78, 83,255, 65, 67, 80,255,
   67, 90,255, 69, 73, 79,255, 65, 67, 69, 77, 78, 80, 83, 90,255, 65, 68,
  255, 82, 88,255, 69, 85,255, 65, 66, 67, 73, 83, 87,255, 73,255, 68,255,
   85,255, 69, 82,255, 84,255, 73, 76, 78, 81, 88,255, 76, 80,255, 69, 84,
  255, 69, 76,255, 67, 82, 88,255, 70, 78, 80,255, 80,255, 67, 68, 88, 90,
  255, 69, 79,255, 67, 68, 77, 78, 80, 88, 90,255, 88,255, 72, 83,255, 65,
   69,255, 68, 88,255, 76,255, 78, 83,255, 68, 69, 72, 73, 78, 79, 84, 88,
  255, 68, 86,255, 65, 79, 86,255, 80, 84,255, 65, 69, 79,255, 65, 71, 73,
  255, 82, 85,255, 66, 83,255, 65, 67, 79, 83, 85,255, 76, 82,255, 67, 68,
  255, 67, 90,255, 69, 79,255, 65, 67, 69, 73, 76, 77, 78, 80, 82, 83, 90,
  255, 66, 73,255, 68, 88,255, 76, 82,255, 72,255, 88,255, 65, 67, 75,255,
   66, 73,255, 66, 69, 72, 73, 80, 81, 84, 85, 89,255, 65, 73,255, 67, 79,
   82, 84,255, 33, 35, 36, 39, 42, 44, 47, 60, 61, 62, 65, 66, 67, 68, 69,
   70, 71, 72, 73, 74, 76, 77, 78, 79, 80, 82, 83, 84, 88,255, 42,255, 76,
   78,255, 68, 78,255, 70, 78,255, 68, 69, 73, 80, 85,255, 42, 47,255, 60,
   61,255, 61,255, 61, 62,255, 67, 68, 73,255, 65, 68, 73,255, 69, 72,255,
   67, 68, 78, 83,255, 65, 67, 80,255, 67, 90,255, 69, 73, 79,255, 65, 67,
   69, 77, 78, 80, 83, 90,255, 65, 68,255, 82, 88,255, 69, 85,255, 65, 66,
   67, 73, 83, 87,255, 73,255, 68,255, 69, 82,255, 84,255, 73, 76, 78, 81,
   88,255, 76, 80,255, 69, 84,255, 76,255, 67, 82, 88,255, 70, 78, 80,255,
   80,255, 67, 68, 88, 90,255, 69, 79,255, 67, 68, 77, 78, 80, 88, 90,255,
   88,255, 72, 83,255, 65, 69,255, 68, 88,255, 76,255, 68, 69, 72, 73, 78,
   79, 84, 88,255, 68, 86,255, 65, 79, 86,255, 80, 84,255, 65, 69, 79,255,
   65, 71, 73,255, 82, 85,255, 66, 83,255, 65, 67, 79, 83, 85,255, 76, 82,
  255, 67, 68,255, 67, 90,255, 69, 79,255, 65, 67, 69, 73, 76, 77, 78, 80,
   82, 83, 90,255, 66, 73,255, 68, 88,255, 76, 82,255, 72,255, 88,255, 65,
   67, 75,255, 66, 73,255, 66, 72, 73, 80, 81, 84, 85, 89,255, 65, 73,255,
   67, 79, 82, 84,255, 33, 35, 36, 39, 42, 44, 47, 60, 61, 62, 65, 66, 67,
   68, 69, 70, 71, 72, 73, 74, 76, 77, 78, 79, 80, 82, 83, 84, 88,255, 76,
   80,255, 78, 79,255, 36, 39, 47, 67, 70, 73, 76, 78, 83,255, 47,255, 47,
   58,255, 36, 47,255, 42, 47,255, 47,255, 47, 72, 76,255, 42, 47,255, 67,
   68, 73,255, 65, 73,255, 69, 72,255, 67, 68, 78, 83,255, 65, 67, 80,255,
   67, 90,255, 69, 73, 79,255, 65, 67, 77, 78, 80, 83, 90,255, 65, 68,255,
   82, 88,255, 69, 85,255, 65, 66, 67, 73, 83, 87,255, 73,255, 68,255, 69,
   82,255, 84,255, 73, 76, 78, 81, 88,255, 82, 88,255, 70, 78,255, 80,255,
   67, 68, 88, 90,255, 69, 79,255, 67, 68, 77, 78, 80, 88, 90,255, 88,255,
   72, 83,255, 65, 69,255, 68, 88,255, 76,255, 68, 72, 73, 88,255, 65, 79,
   86,255, 65, 79,255, 65, 71, 73,255, 82, 85,255, 66, 83,255, 65, 67, 79,
   85,255, 76, 82,255, 67, 68,255, 67, 90,255, 69, 79,255, 65, 67, 69, 73,
   76, 77, 78, 80, 82, 83, 90,255, 66, 73,255, 68, 88,255, 76,255, 88,255,
   65, 67, 75,255, 66, 73,255, 66, 69, 72, 73, 80, 84, 85, 89,255, 65, 73,
  255, 67, 82, 84,255, 47, 65, 67, 68, 69, 72, 73, 74, 76, 77, 78, 79, 80,
   82, 83, 84, 88,255, 67, 68, 73,255, 65, 73,255, 69, 72,255, 67, 68, 78,
   83,255, 65, 67, 80,255, 67, 90,255, 69, 73, 79,255, 65, 67, 77, 78, 80,
   83, 90,255, 65, 68,255, 82, 88,255, 69, 85,255, 65, 66, 67, 73, 83, 87,
  255, 73,255, 68,255, 69, 82,255, 84,255, 73, 76, 78, 88,255, 82, 88,255,
   70, 78,255, 80,255, 67, 68, 88, 90,255, 69, 79,255, 67, 68, 77, 78, 80,
   88, 90,255, 88,255, 72, 83,255, 65, 69,255, 68, 88,255, 76,255, 68, 72,
   73, 88,255, 65, 79, 86,255, 65, 79,255, 65, 71, 73,255, 82, 85,255, 66,
   83,255, 65, 67, 79, 85,255, 76, 82,255, 67, 68,255, 67, 90,255, 69, 79,
  255, 65, 67, 69, 73, 76, 77, 78, 80, 82, 83, 90,255, 66, 73,255, 68, 88,
  255, 76,255, 88,255, 65, 67, 75,255, 66, 73,255, 66, 72, 73, 80, 84, 85,
   89,255, 65, 73,255, 67, 82, 84,255, 65, 67, 68, 69, 72, 73, 74, 76, 77,
   78, 79, 80, 82, 83, 84, 88,255, 66, 83, 87,255, 68, 69, 83,255, 36,255,
   72, 76,255, 67, 68, 73,255, 65, 73,255, 69, 72,255, 67, 68, 78, 83,255,
   65, 67, 80,255, 67, 90,255, 69, 73, 79,255, 65, 67, 77, 78, 80, 83, 90,
  255, 65, 68,255, 82, 88,255, 69, 85,255, 65, 66, 67, 73, 83, 87,255, 73,
  255, 68,255, 69, 82,255, 84,255, 73, 76, 78, 81, 88,255, 82, 88,255, 70,
   78,255, 80,255, 67, 68, 88, 90,255, 69, 79,255, 67, 68, 77, 78, 80, 88,
   90,255, 88,255, 72, 83,255, 65, 69,255, 68, 88,255, 76,255, 68, 72, 73,
   88,255, 65, 79, 86,255, 65, 79,255, 65, 71, 73,255, 82, 85,255, 66, 83,
  255, 65, 67, 79, 85,255, 76, 82,255, 67, 68,255, 67, 90,255, 69, 79,255,
   65, 67, 69, 73, 76, 77, 78, 80, 82, 83, 90,255, 66, 73,255, 68, 88,255,
   76,255, 88,255, 65, 67, 75,255, 66, 73,255, 66, 69, 72, 73, 80, 84, 85,
   89,255, 65, 73,255, 67, 82, 84,255, 47, 65, 67, 68, 69, 72, 73, 74, 76,
   77, 78, 79, 80, 82, 83, 84, 88,255, 47,255, 76, 80,255, 78, 79,255, 36,
   39, 47, 67, 70, 73, 76, 83,255, 42, 47,255, 67, 68, 73,255, 65, 73,255,
   67, 68, 78, 83,255, 65, 67, 80,255, 67, 90,255, 69, 73, 79,255, 65, 67,
   77, 78, 80, 90,255, 65, 68,255, 82, 88,255, 65, 67, 73, 83,255, 82, 88,
  255, 78,255, 80,255, 67, 68, 88, 90,255, 69, 79,255, 67, 68, 77, 78, 80,
   88, 90,255, 88,255, 72, 83,255, 65, 69,255, 68, 88,255, 76,255, 68, 72,
   88,255, 79, 86,255, 65, 73,255, 82, 85,255, 67, 79, 85,255, 76, 82,255,
   67, 68,255, 67, 90,255, 69, 79,255, 65, 67, 69, 73, 76, 77, 78, 80, 82,
   83, 90,255, 66, 73,255, 68, 88,255, 76,255, 88,255, 65, 67,255, 66, 73,
  255, 66, 72, 73, 80, 84, 85,255, 65, 73,255, 67, 82, 84,255, 47, 65, 67,
   68, 69, 72, 73, 74, 76, 77, 78, 79, 80, 82, 83, 88,255, 42, 47,255, 60,
   61,255, 61,255, 61, 62,255, 67, 68, 73,255, 65, 68, 73,255, 67, 68, 78,
   83,255, 65, 67, 80,255, 67, 90,255, 69, 73, 79,255, 65, 67, 77, 78, 80,
   90,255, 65, 68,255, 82, 88,255, 65, 67, 73, 83,255, 73, 81,255, 69, 84,
  255, 82, 88,255, 78,255, 80,255, 67, 68, 88, 90,255, 69, 79,255, 67, 68,
   77, 78, 80, 88, 90,255, 88,255, 72, 83,255, 65, 69,255, 68, 88,255, 76,
  255, 68, 69, 72, 84, 88,255, 68, 86,255, 79, 86,255, 69, 79,255, 65, 73,
  255, 82, 85,255, 67, 79, 85,255, 76, 82,255, 67, 68,255, 67, 90,255, 69,
   79,255, 65, 67, 69, 73, 76, 77, 78, 80, 82, 83, 90,255, 66, 73,255, 68,
   88,255, 76, 82,255, 88,255, 65, 67,255, 66, 73,255, 66, 72, 73, 80, 84,
   85,255, 65, 73,255, 67, 79, 82, 84,255, 33, 42, 44, 47, 60, 61, 62, 65,
   67, 68, 69, 71, 72, 73, 74, 76, 77, 78, 79, 80, 82, 83, 88,255, 42, 47,
  255, 67, 68, 73,255, 65, 73,255, 67, 68, 78, 83,255, 65, 67, 80,255, 67,
   90,255, 69, 73, 79,255, 65, 67, 69, 77, 78, 80, 90,255, 65, 68,255, 82,
   88,255, 65, 67, 73, 83,255, 76, 80,255, 82, 88,255, 78, 80,255, 80,255,
   67, 68, 88, 90,255, 69, 79,255, 67, 68, 77, 78, 80, 88, 90,255, 88,255,
   72, 83,255, 65, 69,255, 68, 88,255, 76,255, 68, 72, 78, 79, 88,255, 79,
   86,255, 65, 73,255, 82, 85,255, 67, 79, 85,255, 76, 82,255, 67, 68,255,
   67, 90,255, 69, 79,255, 65, 67, 69, 73, 76, 77, 78, 80, 82, 83, 90,255,
   66, 73,255, 68, 88,255, 76,255, 88,255, 65, 67,255, 66, 73,255, 66, 72,
   73, 80, 81, 84, 85,255, 65, 73,255, 67, 82, 84,255, 36, 39, 47, 65, 67,
   68, 69, 70, 72, 73, 74, 76, 77, 78, 79, 80, 82, 83, 88,255, 47, 65, 66,
   67, 68, 69, 72, 76, 77,255, 47, 66, 68,255, 47, 65, 66, 68, 72, 80,255,
   47, 66, 68, 72, 83,255, 76, 80,255, 78, 79,255, 36, 39, 67, 70, 73, 76,
   78, 83,255, 65, 66, 67, 68, 69, 72, 76, 77,255, 66, 68,255, 65, 66, 68,
   72, 80,255, 66, 68, 72, 83,255, 67, 68, 73,255, 65, 73,255, 67, 68, 78,
   83,255, 65, 67, 80,255, 67, 90,255, 69, 73, 79,255, 65, 67, 77, 78, 80,
   90,255, 65, 68,255, 82, 88,255, 65, 67, 73, 83,255, 82, 88,255, 78,255,
   80,255, 67, 68, 88, 90,255, 69, 79,255, 67, 68, 77, 78, 80, 88, 90,255,
   88,255, 72, 83,255, 65, 69,255, 68, 88,255, 76,255, 68, 72, 88,255, 79,
   86,255, 65, 73,255, 82, 85,255, 67, 79, 85,255, 76, 82,255, 67, 68,255,
   67, 90,255, 69, 79,255, 65, 67, 69, 73, 76, 77, 78, 80, 82, 83, 90,255,
   66, 73,255, 68, 88,255, 76,255, 88,255, 65, 67,255, 66, 73,255, 66, 72,
   73, 80, 84, 85,255, 65, 73,255, 67, 82, 84,255, 47, 65, 67, 68, 69, 72,
   73, 74, 76, 77, 78, 79, 80, 82, 83, 88,255, 42, 47,255, 36, 47,255, 36,
   47,255, 42, 47,255, 60, 61,255, 61,255, 61, 62,255, 33, 42, 44, 47, 60,
   61, 62,255, 42, 47,255, 44, 47,255, 39,255, 39,255, 42, 47,255, 60, 61,
  255, 61,255, 61, 62,255, 67, 68, 73,255, 65, 68, 73,255, 67, 68, 78, 83,
  255, 65, 67, 80,255, 67, 90,255, 69, 73, 79,255, 65, 67, 77, 78, 80, 90,
  255, 65, 68,255, 82, 88,255, 65, 67, 73, 83,255, 73, 81,255, 69, 84,255,
   76,255, 82, 88,255, 78,255, 80,255, 67, 68, 88, 90,255, 69, 79,255, 67,
   68, 77, 78, 80, 88, 90,255, 88,255, 72, 83,255, 65, 69,255, 68, 88,255,
   76,255, 68, 69, 72, 84, 88,255, 68, 86,255, 79, 86,255, 69, 79,255, 65,
   73,255, 82, 85,255, 67, 79, 85,255, 76, 82,255, 67, 68,255, 67, 90,255,
   69, 79,255, 65, 67, 69, 73, 76, 77, 78, 80, 82, 83, 90,255, 66, 73,255,
   68, 88,255, 76, 82,255, 88,255, 65, 67,255, 66, 73,255, 66, 72, 73, 80,
   84, 85,255, 65, 73,255, 67, 79, 82, 84,255, 33, 42, 44, 47, 60, 61, 62,
   65, 66, 67, 68, 69, 71, 72, 73, 74, 76, 77, 78, 79, 80, 81, 82, 83, 88,
  255, 72,255, 42, 47,255, 60, 61,255, 61,255, 61, 62,255, 69, 84,255, 76,
  255, 82, 88,255, 78,255, 80,255, 67, 68, 88, 90,255, 69, 79,255, 67, 68,
   77, 78, 80, 88, 90,255, 88,255, 72, 83,255, 65, 69,255, 68, 88,255, 76,
  255, 68, 69, 72, 84, 88,255, 68, 86,255, 79, 86,255, 69, 79,255, 65, 73,
  255, 82, 85,255, 67, 79, 85,255, 76, 82,255, 67, 68,255, 67, 90,255, 69,
   79,255, 65, 67, 69, 73, 76, 77, 78, 80, 82, 83, 90,255, 66, 73,255, 68,
   88,255, 76, 82,255, 88,255, 65, 67,255, 66, 73,255, 66, 72, 73, 80, 84,
   85,255, 65, 73,255, 67, 79, 82, 84,255, 33, 42, 44, 47, 60, 61, 62, 66,
   68, 71, 72, 73, 74, 76, 77, 78, 79, 80, 81, 82, 83, 88,255, 76, 80,255,
   78, 79,255, 36, 39, 67, 70, 73, 76, 83,255, 60, 61,255, 61,255, 61, 62,
  255, 67, 68, 73,255, 65, 68, 73,255, 67, 68, 78, 83,255, 65, 67, 80,255,
   67, 90,255, 69, 73, 79,255, 65, 67, 77, 78, 80, 90,255, 65, 68,255, 82,
   88,255, 65, 67, 73, 83,255, 73, 81,255, 69, 84,255, 82, 88,255, 78,255,
   80,255, 67, 68, 88, 90,255, 69, 79,255, 67, 68, 77, 78, 80, 88, 90,255,
   88,255, 72, 83,255, 65, 69,255, 68, 88,255, 76,255, 68, 69, 72, 84, 88,
  255, 68, 86,255, 79, 86,255, 69, 79,255, 65, 73,255, 82, 85,255, 67, 79,
   85,255, 76, 82,255, 67, 68,255, 67, 90,255, 69, 79,255, 65, 67, 69, 73,
   76, 77, 78, 80, 82, 83, 90,255, 66, 73,255, 68, 88,255, 76, 82,255, 88,
  255, 65, 67,255, 66, 73,255, 66, 72, 73, 80, 84, 85,255, 65, 73,255, 67,
   79, 82, 84,255, 33, 42, 44, 47, 60, 61, 62, 65, 67, 68, 69, 71, 72, 73,
   74, 76, 77, 78, 79, 80, 82, 83, 88,255, 60, 61,255, 61,255, 61, 62,255,
   67, 68, 73,255, 65, 68, 73,255, 67, 68, 78, 83,255, 65, 67, 80,255, 67,
   90,255, 69, 73, 79,255, 65, 67, 77, 78, 80, 90,255, 65, 68,255, 82, 88,
  255, 65, 67, 73, 83,255, 73, 81,255, 69, 84,255, 82, 88,255, 78,255, 80,
  255, 67, 68, 88, 90,255, 69, 79,255, 67, 68, 77, 78, 80, 88, 90,255, 88,
  255, 72, 83,255, 65, 69,255, 68, 88,255, 76,255, 68, 69, 72, 84, 88,255,
   79, 86,255, 69, 79,255, 65, 73,255, 82, 85,255, 67, 79, 85,255, 76, 82,
  255, 67, 68,255, 67, 90,255, 69, 79,255, 65, 67, 69, 73, 76, 77, 78, 80,
   82, 83, 90,255, 66, 73,255, 68, 88,255, 76, 82,255, 88,255, 65, 67,255,
   66, 73,255, 66, 72, 73, 80, 84, 85,255, 65, 73,255, 67, 79, 82, 84,255,
   33, 44, 47, 60, 61, 62, 65, 67, 68, 69, 71, 72, 73, 74, 76, 77, 78, 79,
   80, 82, 83, 88,255, 61,255, 61,255, 61,255, 67, 68, 73,255, 65, 68, 73,
  255, 67, 68, 78, 83,255, 65, 67, 80,255, 67, 90,255, 69, 73, 79,255, 65,
   67, 77, 78, 80, 90,255, 65, 68,255, 82, 88,255, 65, 67, 73, 83,255, 73,
   81,255, 69, 84,255, 82, 88,255, 78,255, 80,255, 67, 68, 88, 90,255, 69,
   79,255, 67, 68, 77, 78, 80, 88, 90,255, 88,255, 72, 83,255, 65, 69,255,
   68, 88,255, 76,255, 68, 69, 72, 84, 88,255, 79, 86,255, 69, 79,255, 65,
   73,255, 82, 85,255, 67, 79, 85,255, 76, 82,255, 67, 68,255, 67, 90,255,
   69, 79,255, 65, 67, 69, 73, 76, 77, 78, 80, 82, 83, 90,255, 66, 73,255,
   68, 88,255, 76,255, 88,255, 65, 67,255, 66, 73,255, 66, 72, 73, 80, 84,
   85,255, 65, 73,255, 67, 79, 82, 84,255, 33, 44, 47, 60, 61, 62, 65, 67,
   68, 69, 71, 72, 73, 74, 76, 77, 78, 79, 80, 82, 83, 88,255, 61,255, 61,
  255, 61,255, 67, 68, 73,255, 65, 73,255, 67, 68, 78, 83,255, 65, 67, 80,
  255, 67, 90,255, 69, 73, 79,255, 65, 67, 77, 78, 80, 90,255, 65, 68,255,
   82, 88,255, 65, 67, 73, 83,255, 73, 81,255, 69, 84,255, 82, 88,255, 78,
  255, 80,255, 67, 68, 88, 90,255, 69, 79,255, 67, 68, 77, 78, 80, 88, 90,
  255, 88,255, 72, 83,255, 65, 69,255, 68, 88,255, 76,255, 68, 69, 72, 84,
   88,255, 79, 86,255, 69, 79,255, 65, 73,255, 82, 85,255, 67, 79, 85,255,
   76, 82,255, 67, 68,255, 67, 90,255, 69, 79,255, 65, 67, 69, 73, 76, 77,
   78, 80, 82, 83, 90,255, 66, 73,255, 68, 88,255, 76,255, 88,255, 65, 67,
  255, 66, 73,255, 66, 72, 73, 80, 84, 85,255, 65, 73,255, 67, 79, 82, 84,
  255, 33, 44, 47, 60, 61, 62, 65, 67, 68, 69, 71, 72, 73, 74, 76, 77, 78,
   79, 80, 82, 83, 88,255, 61,255, 61,255, 61,255, 67, 68, 73,255, 65, 73,
  255, 67, 68, 78, 83,255, 65, 67, 80,255, 67, 90,255, 69, 73, 79,255, 65,
   67, 77, 78, 80, 90,255, 65, 68,255, 82, 88,255, 65, 67, 73, 83,255, 73,
   81,255, 69, 84,255, 82, 88,255, 78,255, 80,255, 67, 68, 88, 90,255, 69,
   79,255, 67, 68, 77, 78, 80, 88, 90,255, 88,255, 72, 83,255, 65, 69,255,
   68, 88,255, 76,255, 68, 69, 72, 84, 88,255, 79, 86,255, 69, 79,255, 65,
   73,255, 82, 85,255, 67, 79, 85,255, 76, 82,255, 67, 68,255, 67, 90,255,
   69, 79,255, 65, 67, 69, 73, 76, 77, 78, 80, 82, 83, 90,255, 66, 73,255,
   68, 88,255, 76,255, 88,255, 65, 67,255, 66, 73,255, 66, 72, 73, 80, 84,
   85,255, 65, 73,255, 67, 82, 84,255, 33, 44, 47, 60, 61, 62, 65, 67, 68,
   69, 71, 72, 73, 74, 76, 77, 78, 79, 80, 82, 83, 88,255, 61,255, 61,255,
   61,255, 69, 84,255, 69, 84,255, 33, 47, 60, 61, 62, 69, 71, 76, 78,255,
   44, 47,255, 42, 47,255, 67, 68, 73,255, 65, 73,255, 67, 68, 78, 83,255,
   65, 67, 80,255, 67, 90,255, 69, 73, 79,255, 65, 67, 77, 78, 80, 90,255,
   65, 68,255, 82, 88,255, 65, 67, 73, 83,255, 82, 88,255, 78,255, 80,255,
   67, 68, 88, 90,255, 69, 79,255, 67, 68, 77, 78, 80, 88, 90,255, 88,255,
   72, 83,255, 65, 69,255, 68, 88,255, 76,255, 68, 72, 88,255, 79, 86,255,
   65, 73,255, 82, 85,255, 67, 79, 85,255, 76, 82,255, 67, 68,255, 67, 90,
  255, 69, 79,255, 65, 67, 69, 73, 76, 77, 78, 80, 82, 83, 90,255, 66, 73,
  255, 68, 88,255, 76,255, 88,255, 65, 67,255, 66, 73,255, 66, 72, 73, 80,
   84, 85,255, 65, 73,255, 67, 82, 84,255, 44, 47, 65, 67, 68, 69, 72, 73,
   74, 76, 77, 78, 79, 80, 82, 83, 88,255, 44,255, 42, 47,255, 47, 66, 68,
   72, 81,255, 69,255, 76, 80,255, 78, 79,255, 36, 39, 47, 65, 66, 67, 68,
   69, 70, 72, 73, 76, 77, 78, 83,255
};

static const unsigned char ag_key_act[] = {
  0,3,3,4,3,3,4,2,3,4,3,2,2,3,3,4,0,0,4,5,5,5,4,5,5,4,7,7,4,7,2,2,2,4,5,
  5,5,4,5,5,4,5,5,5,4,7,5,6,2,6,7,5,4,5,5,4,5,5,4,7,7,4,2,5,2,5,6,5,4,7,
  4,6,4,7,7,4,2,4,5,7,2,2,4,7,5,5,4,5,6,4,5,4,5,5,5,5,4,5,5,4,5,5,6,2,6,
  5,5,4,5,4,5,5,4,6,2,4,5,5,4,2,4,2,2,7,7,4,7,7,7,4,7,7,4,5,5,5,4,2,7,4,
  7,7,4,7,7,7,2,4,5,5,4,5,7,4,5,5,4,5,5,4,2,5,7,7,2,5,2,6,7,7,5,4,5,5,4,
  5,5,4,2,4,5,4,6,5,7,4,5,5,4,2,2,7,7,2,2,7,4,5,5,4,7,2,7,4,2,5,2,2,2,2,
  2,7,2,2,2,2,2,2,2,2,2,7,2,4,3,3,4,3,3,4,2,3,4,3,2,2,3,3,4,0,0,4,0,0,4,
  0,4,0,0,4,5,5,5,4,5,5,5,4,7,7,4,7,2,2,2,4,5,5,5,4,5,5,4,5,5,5,4,7,5,7,
  6,2,6,7,5,4,5,5,4,5,5,4,7,7,4,2,5,2,5,6,5,4,7,4,6,4,5,4,7,7,4,2,4,5,7,
  2,6,2,4,7,5,4,5,5,4,7,7,4,7,5,5,4,5,6,5,4,5,4,5,5,5,5,4,5,5,4,5,5,6,2,
  6,5,5,4,5,4,5,5,4,6,2,4,5,5,4,2,4,7,7,4,2,5,2,2,5,7,5,7,4,5,5,4,7,2,7,
  4,5,5,4,7,5,2,4,5,5,5,4,6,7,4,7,7,4,7,7,7,7,2,4,5,5,4,5,7,4,5,5,4,5,5,
  4,2,5,7,7,2,5,2,6,7,7,5,4,5,5,4,5,5,4,6,5,4,7,4,5,4,6,5,7,4,5,5,4,2,7,
  2,7,6,7,2,2,7,4,5,5,4,7,7,2,7,4,3,2,5,3,3,0,2,1,1,1,6,5,6,6,6,2,2,6,2,
  2,6,6,2,2,2,2,2,7,2,4,3,4,3,3,4,3,3,4,2,3,4,3,2,2,3,3,4,0,0,4,0,0,4,0,
  4,0,0,4,5,5,5,4,5,5,5,4,7,7,4,7,2,2,2,4,5,5,5,4,5,5,4,5,5,5,4,7,5,7,6,
  2,6,7,5,4,5,5,4,5,5,4,7,7,4,2,5,2,5,6,5,4,7,4,6,4,7,7,4,2,4,5,7,2,5,2,
  4,7,5,4,5,5,4,7,4,7,5,5,4,5,6,5,4,5,4,5,5,5,5,4,5,5,4,5,5,6,2,6,5,5,4,
  5,4,5,5,4,6,2,4,5,5,4,2,4,2,5,2,7,5,7,5,7,4,5,5,4,7,2,7,4,5,5,4,7,5,2,
  4,5,5,5,4,6,7,4,7,7,4,7,7,7,7,2,4,5,5,4,5,7,4,5,5,4,5,5,4,2,5,7,7,2,5,
  2,6,7,7,5,4,5,5,4,5,5,4,6,5,4,7,4,5,4,6,5,7,4,5,5,4,2,2,7,6,7,2,2,7,4,
  5,5,4,7,7,2,7,4,3,2,5,3,3,0,2,1,1,1,6,5,6,6,6,2,2,6,2,2,6,6,2,2,2,2,2,
  7,2,4,7,5,4,5,7,4,5,3,3,7,2,7,2,7,7,4,3,4,3,0,4,5,3,4,0,0,4,2,4,3,7,7,
  4,0,0,4,5,5,5,4,5,5,4,7,7,4,7,2,2,2,4,5,5,5,4,5,5,4,5,5,5,4,7,5,6,2,6,
  7,5,4,5,5,4,5,5,4,7,7,4,2,5,2,5,6,5,4,7,4,6,4,7,7,4,2,4,5,7,2,7,2,4,5,
  5,4,5,6,4,5,4,5,5,5,5,4,5,5,4,5,5,6,2,6,5,5,4,5,4,5,5,4,6,2,4,5,5,4,2,
  4,2,2,7,7,4,7,7,7,4,7,7,4,5,5,5,4,2,7,4,7,7,4,7,7,7,2,4,5,5,4,5,7,4,5,
  5,4,5,5,4,2,5,7,7,2,5,2,6,7,7,5,4,5,5,4,5,5,4,2,4,5,4,6,5,7,4,5,5,4,2,
  7,2,7,7,2,2,7,4,5,5,4,7,2,7,4,2,2,2,2,2,7,2,2,2,2,2,2,2,2,2,7,2,4,5,5,
  5,4,5,5,4,7,7,4,7,2,2,2,4,5,5,5,4,5,5,4,5,5,5,4,7,5,6,2,6,7,5,4,5,5,4,
  5,5,4,7,7,4,2,5,2,5,6,5,4,7,4,6,4,7,7,4,2,4,5,7,2,2,4,5,5,4,5,6,4,5,4,
  5,5,5,5,4,5,5,4,5,5,6,2,6,5,5,4,5,4,5,5,4,6,2,4,5,5,4,2,4,2,2,7,7,4,7,
  7,7,4,7,7,4,5,5,5,4,2,7,4,7,7,4,7,7,7,2,4,5,5,4,5,7,4,5,5,4,5,5,4,2,5,
  7,7,2,5,2,6,7,7,5,4,5,5,4,5,5,4,2,4,5,4,6,5,7,4,5,5,4,2,2,7,7,2,2,7,4,
  5,5,4,7,2,7,4,2,2,2,2,7,2,2,2,2,2,2,2,2,2,7,2,4,5,5,5,4,2,7,7,4,5,4,7,
  7,4,5,5,5,4,5,5,4,7,7,4,7,2,2,2,4,5,5,5,4,5,5,4,5,5,5,4,7,5,6,2,6,7,5,
  4,5,5,4,5,5,4,7,7,4,2,5,2,5,6,5,4,7,4,6,4,7,7,4,2,4,5,7,2,7,2,4,5,5,4,
  5,6,4,5,4,5,5,5,5,4,5,5,4,5,5,6,2,6,5,5,4,5,4,5,5,4,6,2,4,5,5,4,2,4,2,
  2,7,7,4,7,7,7,4,7,7,4,5,5,5,4,2,7,4,7,7,4,7,7,7,2,4,5,5,4,5,7,4,5,5,4,
  5,5,4,2,5,7,7,2,5,2,6,7,7,5,4,5,5,4,5,5,4,2,4,5,4,6,5,7,4,5,5,4,2,7,2,
  7,7,2,2,7,4,5,5,4,7,2,7,4,3,2,2,2,2,7,2,2,2,2,2,2,2,2,2,7,2,4,3,4,7,5,
  4,5,7,4,5,3,3,7,2,7,2,7,4,0,0,4,5,5,5,4,5,5,4,7,2,2,7,4,5,5,5,4,5,5,4,
  5,5,5,4,7,5,6,2,6,5,4,5,5,4,5,5,4,2,2,5,7,4,5,5,4,6,4,5,4,5,5,5,5,4,5,
  5,4,5,5,6,2,6,5,5,4,5,4,5,5,4,6,2,4,5,5,4,2,4,2,2,7,4,7,7,4,5,5,4,2,7,
  4,7,7,7,4,5,5,4,5,7,4,5,5,4,5,5,4,2,5,7,7,2,5,2,6,7,7,5,4,5,5,4,5,5,4,
  2,4,5,4,6,5,4,5,5,4,2,2,7,7,2,2,4,5,5,4,7,2,7,4,2,2,2,2,7,7,2,2,2,2,7,
  2,2,2,2,2,4,0,0,4,0,0,4,0,4,0,0,4,5,5,5,4,5,5,5,4,7,2,2,7,4,5,5,5,4,5,
  5,4,5,5,5,4,7,5,6,2,6,5,4,5,5,4,5,5,4,2,2,5,7,4,5,5,4,5,5,4,5,5,4,6,4,
  5,4,5,5,5,5,4,5,5,4,5,5,6,2,6,5,5,4,5,4,5,5,4,6,2,4,5,5,4,2,4,2,5,2,5,
  7,4,5,5,4,2,7,4,5,7,4,5,5,4,6,7,4,7,7,7,4,5,5,4,5,7,4,5,5,4,5,5,4,2,5,
  7,7,2,5,2,6,7,7,5,4,5,5,4,5,5,4,6,5,4,5,4,6,5,4,5,5,4,2,2,7,7,2,2,4,5,
  5,4,7,7,2,7,4,3,3,0,2,1,1,1,2,2,2,2,2,7,2,2,2,2,2,2,2,2,2,2,4,0,0,4,5,
  5,5,4,5,5,4,7,2,2,7,4,5,5,5,4,5,5,4,5,5,5,4,7,5,7,6,2,6,5,4,5,5,4,5,5,
  4,2,2,5,7,4,7,5,4,5,5,4,6,5,4,5,4,5,5,5,5,4,5,5,4,5,5,6,2,6,5,5,4,5,4,
  5,5,4,6,2,4,5,5,4,2,4,2,2,5,7,7,4,7,7,4,5,5,4,2,7,4,7,7,7,4,5,5,4,5,7,
  4,5,5,4,5,5,4,2,5,7,7,2,5,2,6,7,7,5,4,5,5,4,5,5,4,2,4,5,4,6,5,4,5,5,4,
  2,2,7,7,7,2,2,4,5,5,4,7,2,7,4,5,3,2,2,2,2,7,2,7,2,2,2,2,7,2,2,2,2,2,4,
  3,5,5,5,5,5,5,5,5,4,3,5,5,4,3,5,5,5,5,7,4,3,5,5,5,7,4,7,5,4,5,7,4,5,3,
  7,2,7,2,7,7,4,5,5,5,5,5,5,5,5,4,5,5,4,5,5,5,5,7,4,5,5,5,7,4,5,5,5,4,5,
  5,4,7,2,2,7,4,5,5,5,4,5,5,4,5,5,5,4,7,5,6,2,6,5,4,5,5,4,5,5,4,2,2,5,7,
  4,5,5,4,6,4,5,4,5,5,5,5,4,5,5,4,5,5,6,2,6,5,5,4,5,4,5,5,4,6,2,4,5,5,4,
  2,4,2,2,7,4,7,7,4,5,5,4,2,7,4,7,7,7,4,5,5,4,5,7,4,5,5,4,5,5,4,2,5,7,7,
  2,5,2,6,7,7,5,4,5,5,4,5,5,4,2,4,5,4,6,5,4,5,5,4,2,2,7,7,2,2,4,5,5,4,7,
  2,7,4,3,2,2,2,7,7,2,2,2,2,7,2,2,2,2,2,4,0,0,4,5,2,4,5,3,4,0,0,4,0,0,4,
  0,4,0,0,4,3,3,0,2,1,1,1,4,0,0,4,0,2,4,3,4,3,4,0,0,4,0,0,4,0,4,0,0,4,5,
  5,5,4,5,5,5,4,7,2,2,7,4,5,5,5,4,5,5,4,5,5,5,4,7,5,6,2,6,5,4,5,5,4,5,5,
  4,2,2,5,7,4,5,5,4,5,5,4,7,4,5,5,4,6,4,5,4,5,5,5,5,4,5,5,4,5,5,6,2,6,5,
  5,4,5,4,5,5,4,6,2,4,5,5,4,2,4,2,5,2,5,7,4,5,5,4,2,7,4,5,7,4,5,5,4,6,7,
  4,7,7,7,4,5,5,4,5,7,4,5,5,4,5,5,4,2,5,7,7,2,5,2,6,7,7,5,4,5,5,4,5,5,4,
  6,5,4,5,4,6,5,4,5,5,4,2,2,7,7,2,2,4,5,5,4,7,7,2,7,4,3,3,0,2,1,1,1,2,5,
  2,6,2,2,6,2,2,2,2,2,2,2,5,2,2,2,4,5,4,0,0,4,0,0,4,0,4,0,0,4,5,5,4,7,4,
  5,5,4,6,4,5,4,5,5,5,5,4,5,5,4,5,5,6,2,6,5,5,4,5,4,5,5,4,6,2,4,5,5,4,2,
  4,2,5,2,5,7,4,5,5,4,2,7,4,5,7,4,5,5,4,6,7,4,7,7,7,4,5,5,4,5,7,4,5,5,4,
  5,5,4,2,5,7,7,2,5,2,6,7,7,5,4,5,5,4,5,5,4,6,5,4,5,4,6,5,4,5,5,4,2,2,7,
  7,2,2,4,5,5,4,7,7,2,7,4,3,3,0,2,1,1,1,5,5,2,6,2,2,2,2,2,2,2,5,2,2,2,4,
  7,5,4,5,7,4,5,3,7,2,7,2,7,4,0,0,4,0,4,0,0,4,5,5,5,4,5,5,5,4,7,2,2,7,4,
  5,5,5,4,5,5,4,5,5,5,4,7,5,6,2,6,5,4,5,5,4,5,5,4,2,2,5,7,4,5,5,4,5,5,4,
  5,5,4,6,4,5,4,5,5,5,5,4,5,5,4,5,5,6,2,6,5,5,4,5,4,5,5,4,6,2,4,5,5,4,2,
  4,2,5,2,5,7,4,5,5,4,2,7,4,5,7,4,5,5,4,6,7,4,7,7,7,4,5,5,4,5,7,4,5,5,4,
  5,5,4,2,5,7,7,2,5,2,6,7,7,5,4,5,5,4,5,5,4,6,5,4,5,4,6,5,4,5,5,4,2,2,7,
  7,2,2,4,5,5,4,7,7,2,7,4,3,3,0,3,1,1,1,2,2,2,2,2,7,2,2,2,2,2,2,2,2,2,2,
  4,0,0,4,0,4,0,0,4,5,5,5,4,5,5,5,4,7,2,2,7,4,5,5,5,4,5,5,4,5,5,5,4,7,5,
  6,2,6,5,4,5,5,4,5,5,4,2,2,5,7,4,5,5,4,5,5,4,5,5,4,6,4,5,4,5,5,5,5,4,5,
  5,4,5,5,6,2,6,5,5,4,5,4,5,5,4,6,2,4,5,5,4,2,4,2,5,2,5,7,4,7,7,4,5,7,4,
  5,5,4,6,7,4,7,7,7,4,5,5,4,5,7,4,5,5,4,5,5,4,2,5,7,7,2,5,2,6,7,7,5,4,5,
  5,4,5,5,4,6,5,4,5,4,6,5,4,5,5,4,2,2,7,7,2,2,4,5,5,4,7,7,2,7,4,3,0,3,1,
  1,1,2,2,2,2,2,7,2,2,2,2,2,2,2,2,2,2,4,0,4,0,4,0,4,5,5,5,4,5,5,5,4,7,2,
  2,7,4,5,5,5,4,5,5,4,5,5,5,4,7,5,6,2,6,5,4,5,5,4,5,5,4,2,2,5,7,4,5,5,4,
  5,5,4,5,5,4,6,4,5,4,5,5,5,5,4,5,5,4,5,5,6,2,6,5,5,4,5,4,5,5,4,6,2,4,5,
  5,4,2,4,2,5,2,5,7,4,7,7,4,5,7,4,5,5,4,6,7,4,7,7,7,4,5,5,4,5,7,4,5,5,4,
  5,5,4,2,5,7,7,2,5,2,6,7,7,5,4,5,5,4,5,5,4,2,4,5,4,6,5,4,5,5,4,2,2,7,7,
  2,2,4,5,5,4,7,7,2,7,4,3,0,3,1,1,1,2,2,2,2,2,7,2,2,2,2,2,2,2,2,2,2,4,0,
  4,0,4,0,4,5,5,5,4,5,5,4,7,2,2,7,4,5,5,5,4,5,5,4,5,5,5,4,7,5,6,2,6,5,4,
  5,5,4,5,5,4,2,2,5,7,4,5,5,4,5,5,4,5,5,4,6,4,5,4,5,5,5,5,4,5,5,4,5,5,6,
  2,6,5,5,4,5,4,5,5,4,6,2,4,5,5,4,2,4,2,5,2,5,7,4,7,7,4,5,7,4,5,5,4,6,7,
  4,7,7,7,4,5,5,4,5,7,4,5,5,4,5,5,4,2,5,7,7,2,5,2,6,7,7,5,4,5,5,4,5,5,4,
  2,4,5,4,6,5,4,5,5,4,2,2,7,7,2,2,4,5,5,4,7,7,2,7,4,3,0,3,1,1,1,2,2,2,2,
  2,7,2,2,2,2,2,2,2,2,2,2,4,0,4,0,4,0,4,5,5,5,4,5,5,4,7,2,2,7,4,5,5,5,4,
  5,5,4,5,5,5,4,7,5,6,2,6,5,4,5,5,4,5,5,4,2,2,5,7,4,5,5,4,5,5,4,5,5,4,6,
  4,5,4,5,5,5,5,4,5,5,4,5,5,6,2,6,5,5,4,5,4,5,5,4,6,2,4,5,5,4,2,4,2,5,2,
  5,7,4,7,7,4,5,7,4,5,5,4,6,7,4,7,7,7,4,5,5,4,5,7,4,5,5,4,5,5,4,2,5,7,7,
  2,5,2,6,7,7,5,4,5,5,4,5,5,4,2,4,5,4,6,5,4,5,5,4,2,2,7,7,2,2,4,5,5,4,7,
  2,7,4,3,0,3,1,1,1,2,2,2,2,2,7,2,2,2,2,2,2,2,2,2,2,4,0,4,0,4,0,4,5,5,4,
  5,5,4,3,3,1,1,1,7,2,2,7,4,0,3,4,0,0,4,5,5,5,4,5,5,4,7,2,2,7,4,5,5,5,4,
  5,5,4,5,5,5,4,7,5,6,2,6,5,4,5,5,4,5,5,4,2,2,5,7,4,5,5,4,6,4,5,4,5,5,5,
  5,4,5,5,4,5,5,6,2,6,5,5,4,5,4,5,5,4,6,2,4,5,5,4,2,4,2,2,7,4,7,7,4,5,5,
  4,2,7,4,7,7,7,4,5,5,4,5,7,4,5,5,4,5,5,4,2,5,7,7,2,5,2,6,7,7,5,4,5,5,4,
  5,5,4,2,4,5,4,6,5,4,5,5,4,2,2,7,7,2,2,4,5,5,4,7,2,7,4,0,2,2,2,2,7,7,2,
  2,2,2,7,2,2,2,2,2,4,0,4,0,0,4,2,5,5,5,5,4,7,4,7,5,4,5,7,4,5,3,3,5,5,6,
  5,5,2,5,7,6,5,7,7,4
};

static const unsigned short ag_key_parm[] = {
    0,301,302,  0,299,300,  0,  0,294,  0,303,  0,  0,296,304,  0, 35,285,
    0,118,120,122,  0,124,126,  0, 12,128,  0,116,  0,  0,  0,  0,136,138,
  140,  0,142,144,  0,148,150,152,  0,130,132,134,  0,146, 14,154,  0,156,
  158,  0,160,162,  0, 16,166,  0,  0, 20,  0,164, 18, 22,  0, 40,  0, 34,
    0, 26, 24,  0,  0,  0,168, 38,  0,  0,  0,  0,174,176,  0, 36,172,  0,
  186,  0,188,190,192,194,  0,198,200,  0,178,180,184,  0,196,182,202,  0,
  206,  0,208,210,  0,204,  0,  0,212,214,  0,  0,  0,  0,  0, 48,216,  0,
   50,218,220,  0, 30,222,  0,224, 10,226,  0,  0,228,  0, 28,234,  0, 44,
  230,232,  0,  0,236,238,  0,246,248,  0,252,254,  0,258,260,  0,  0,240,
  242,244,  0,250,  0,256,262,264,266,  0,268,270,  0,272,274,  0,  0,  0,
  282,  0,280,284, 32,  0,286,288,  0,  0,  0,276,278,  0,  0, 46,  0,292,
  294,  0,290,  0,296,  0,  0, 52,  0,  0,  0,  0,  0,170,  0,  0,  0,  0,
    0,  0,  0,  0,  0, 42,  0,  0,301,302,  0,299,300,  0,  0,294,  0,303,
    0,  0,296,304,  0, 35,285,  0,350,339,  0,332,  0,337,352,  0,118,120,
  122,  0,124, 70,126,  0, 12,128,  0,116,  0,  0,  0,  0,136,138,140,  0,
  142,144,  0,148,150,152,  0,130,132, 82,134,  0,146, 14,154,  0,156,158,
    0,160,162,  0, 16,166,  0,  0, 20,  0,164, 18, 22,  0, 40,  0, 34,  0,
    6,  0, 26, 24,  0,  0,  0,168, 38,  0, 54,  0,  0, 80, 92,  0, 58, 62,
    0,  4,170,  0,  0,174,176,  0, 36,172, 90,  0,186,  0,188,190,192,194,
    0,198,200,  0,178,180,184,  0,196,182,202,  0,206,  0,208,210,  0,204,
    0,  0,212,214,  0,  0,  0, 48,  2,  0,  0, 60,  0,  0, 84, 86, 64,216,
    0, 76,218,  0, 50,  0,220,  0,222, 78,  0, 30, 56,  0,  0,224, 10,226,
    0, 66,228,  0, 28,234,  0, 44,230,232,114,  0,  0,236,238,  0,246,248,
    0,252,254,  0,258,260,  0,  0,240,242,244,  0,250,  0,256,262,264,266,
    0,268,270,  0,272,274,  0, 72, 74,  0,278,  0,282,  0,280,284, 32,  0,
  286,288,  0,  0,  8,  0,276,112, 88,  0,  0, 46,  0,292,294,  0,290, 68,
    0,296,  0,335,  0, 52,168,360,330,  0,343,333,341,110, 96,102,100,104,
    0,  0, 94,  0,  0,106,108,  0,  0,  0,  0,  0, 42,  0,  0, 38,  0,301,
  302,  0,299,300,  0,  0,294,  0,303,  0,  0,296,304,  0, 35,285,  0,350,
  339,  0,332,  0,337,352,  0,118,120,122,  0,124, 70,126,  0, 12,128,  0,
  116,  0,  0,  0,  0,136,138,140,  0,142,144,  0,148,150,152,  0,130,132,
   82,134,  0,146, 14,154,  0,156,158,  0,160,162,  0, 16,166,  0,  0, 20,
    0,164, 18, 22,  0, 40,  0, 34,  0, 26, 24,  0,  0,  0,168, 38,  0, 54,
    0,  0, 80, 92,  0, 58, 62,  0,170,  0,  0,174,176,  0, 36,172, 90,  0,
  186,  0,188,190,192,194,  0,198,200,  0,178,180,184,  0,196,182,202,  0,
  206,  0,208,210,  0,204,  0,  0,212,214,  0,  0,  0,  0, 60,  0, 48, 84,
   86, 64,216,  0, 76,218,  0, 50,  0,220,  0,222, 78,  0, 30, 56,  0,  0,
  224, 10,226,  0, 66,228,  0, 28,234,  0, 44,230,232,114,  0,  0,236,238,
    0,246,248,  0,252,254,  0,258,260,  0,  0,240,242,244,  0,250,  0,256,
  262,264,266,  0,268,270,  0,272,274,  0, 72, 74,  0,278,  0,282,  0,280,
  284, 32,  0,286,288,  0,  0,  0,276,112, 88,  0,  0, 46,  0,292,294,  0,
  290, 68,  0,296,  0,335,  0, 52,168,360,330,  0,343,333,341,110, 96,102,
  100,104,  0,  0, 94,  0,  0,106,108,  0,  0,  0,  0,  0, 42,  0,  0, 80,
   92,  0, 84, 86,  0, 52,168, 35, 82,  0, 90,  0, 78, 88,  0, 35,  0, 35,
   97,  0, 52, 35,  0, 35,285,  0,  0,  0, 35,  4,  2,  0, 35,285,  0,118,
  120,122,  0,124,126,  0, 12,128,  0,116,  0,  0,  0,  0,136,138,140,  0,
  142,144,  0,148,150,152,  0,130,132,134,  0,146, 14,154,  0,156,158,  0,
  160,162,  0, 16,166,  0,  0, 20,  0,164, 18, 22,  0, 40,  0, 34,  0, 26,
   24,  0,  0,  0,168, 38,  0,  6,  0,  0,174,176,  0, 36,172,  0,186,  0,
  188,190,192,194,  0,198,200,  0,178,180,184,  0,196,182,202,  0,206,  0,
  208,210,  0,204,  0,  0,212,214,  0,  0,  0,  0,  0, 48,216,  0, 50,218,
  220,  0, 30,222,  0,224, 10,226,  0,  0,228,  0, 28,234,  0, 44,230,232,
    0,  0,236,238,  0,246,248,  0,252,254,  0,258,260,  0,  0,240,242,244,
    0,250,  0,256,262,264,266,  0,268,270,  0,272,274,  0,  0,  0,282,  0,
  280,284, 32,  0,286,288,  0,  0,  8,  0,276,278,  0,  0, 46,  0,292,294,
    0,290,  0,296,  0,  0,  0,  0,  0,  0,170,  0,  0,  0,  0,  0,  0,  0,
    0,  0, 42,  0,  0,118,120,122,  0,124,126,  0, 12,128,  0,116,  0,  0,
    0,  0,136,138,140,  0,142,144,  0,148,150,152,  0,130,132,134,  0,146,
   14,154,  0,156,158,  0,160,162,  0, 16,166,  0,  0, 20,  0,164, 18, 22,
    0, 40,  0, 34,  0, 26, 24,  0,  0,  0,168, 38,  0,  0,  0,174,176,  0,
   36,172,  0,186,  0,188,190,192,194,  0,198,200,  0,178,180,184,  0,196,
  182,202,  0,206,  0,208,210,  0,204,  0,  0,212,214,  0,  0,  0,  0,  0,
   48,216,  0, 50,218,220,  0, 30,222,  0,224, 10,226,  0,  0,228,  0, 28,
  234,  0, 44,230,232,  0,  0,236,238,  0,246,248,  0,252,254,  0,258,260,
    0,  0,240,242,244,  0,250,  0,256,262,264,266,  0,268,270,  0,272,274,
    0,  0,  0,282,  0,280,284, 32,  0,286,288,  0,  0,  0,276,278,  0,  0,
   46,  0,292,294,  0,290,  0,296,  0,  0,  0,  0,  0,170,  0,  0,  0,  0,
    0,  0,  0,  0,  0, 42,  0,  0, 20, 18, 22,  0,  0,  6,  8,  0, 52,  0,
    4,  2,  0,118,120,122,  0,124,126,  0, 12,128,  0,116,  0,  0,  0,  0,
  136,138,140,  0,142,144,  0,148,150,152,  0,130,132,134,  0,146, 14,154,
    0,156,158,  0,160,162,  0, 16,166,  0,  0, 20,  0,164, 18, 22,  0, 40,
    0, 34,  0, 26, 24,  0,  0,  0,168, 38,  0,  6,  0,  0,174,176,  0, 36,
  172,  0,186,  0,188,190,192,194,  0,198,200,  0,178,180,184,  0,196,182,
  202,  0,206,  0,208,210,  0,204,  0,  0,212,214,  0,  0,  0,  0,  0, 48,
  216,  0, 50,218,220,  0, 30,222,  0,224, 10,226,  0,  0,228,  0, 28,234,
    0, 44,230,232,  0,  0,236,238,  0,246,248,  0,252,254,  0,258,260,  0,
    0,240,242,244,  0,250,  0,256,262,264,266,  0,268,270,  0,272,274,  0,
    0,  0,282,  0,280,284, 32,  0,286,288,  0,  0,  8,  0,276,278,  0,  0,
   46,  0,292,294,  0,290,  0,296,  0,285,  0,  0,  0,  0,170,  0,  0,  0,
    0,  0,  0,  0,  0,  0, 42,  0,  0,285,  0, 80, 92,  0, 84, 86,  0, 52,
  168, 35, 82,  0, 90,  0, 88,  0, 35,285,  0,118,120,122,  0,124,126,  0,
  116,  0,  0,128,  0,136,138,140,  0,142,144,  0,148,150,152,  0,130,132,
  134,  0,146,154,  0,156,158,  0,160,162,  0,  0,  0,164,166,  0,174,176,
    0,172,  0,186,  0,188,190,192,194,  0,198,200,  0,178,180,184,  0,196,
  182,202,  0,206,  0,208,210,  0,204,  0,  0,212,214,  0,  0,  0,  0,  0,
  216,  0,218,220,  0,224,226,  0,  0,228,  0,230,232,234,  0,236,238,  0,
  246,248,  0,252,254,  0,258,260,  0,  0,240,242,244,  0,250,  0,256,262,
  264,266,  0,268,270,  0,272,274,  0,  0,  0,282,  0,280,284,  0,286,288,
    0,  0,  0,276,278,  0,  0,  0,292,294,  0,290,  0,296,  0,  0,  0,  0,
    0,168,170,  0,  0,  0,  0,222,  0,  0,  0,  0,  0,  0, 35,285,  0,350,
  339,  0,332,  0,337,352,  0,118,120,122,  0,124, 70,126,  0,116,  0,  0,
  128,  0,136,138,140,  0,142,144,  0,148,150,152,  0,130,132,134,  0,146,
  154,  0,156,158,  0,160,162,  0,  0,  0,164,166,  0,168, 54,  0, 58, 62,
    0,174,176,  0,172,  0,186,  0,188,190,192,194,  0,198,200,  0,178,180,
  184,  0,196,182,202,  0,206,  0,208,210,  0,204,  0,  0,212,214,  0,  0,
    0,  0, 60,  0, 64,216,  0, 76,218,  0,  0,220,  0, 56,222,  0,224,226,
    0, 66,228,  0,230,232,234,  0,236,238,  0,246,248,  0,252,254,  0,258,
  260,  0,  0,240,242,244,  0,250,  0,256,262,264,266,  0,268,270,  0,272,
  274,  0, 72, 74,  0,282,  0,280,284,  0,286,288,  0,  0,  0,276,278,  0,
    0,  0,292,294,  0,290, 68,  0,296,  0,335,360,330,  0,343,333,341,  0,
    0,  0,  0,  0,170,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0, 35,285,
    0,118,120,122,  0,124,126,  0,116,  0,  0,128,  0,136,138,140,  0,142,
  144,  0,148,150,152,  0,130,132, 82,134,  0,146,154,  0,156,158,  0,160,
  162,  0,  0,  0,164,166,  0, 80, 92,  0,174,176,  0,172, 90,  0,186,  0,
  188,190,192,194,  0,198,200,  0,178,180,184,  0,196,182,202,  0,206,  0,
  208,210,  0,204,  0,  0,212,214,  0,  0,  0,  0,  0, 84, 86,216,  0,218,
  220,  0,224,226,  0,  0,228,  0,230,232,234,  0,236,238,  0,246,248,  0,
  252,254,  0,258,260,  0,  0,240,242,244,  0,250,  0,256,262,264,266,  0,
  268,270,  0,272,274,  0,  0,  0,282,  0,280,284,  0,286,288,  0,  0,  0,
  276,278, 88,  0,  0,  0,292,294,  0,290,  0,296,  0, 52,168,  0,  0,  0,
    0,168,  0,170,  0,  0,  0,  0,222,  0,  0,  0,  0,  0,  0, 35,110, 96,
  102,100,104, 94,106,108,  0, 35, 96,100,  0, 35,110, 96,100, 94,114,  0,
   35, 96,100, 94,112,  0, 80, 92,  0, 84, 86,  0, 52,168, 82,  0, 90,  0,
   78, 88,  0,110, 96,102,100,104, 94,106,108,  0, 96,100,  0,110, 96,100,
   94,114,  0, 96,100, 94,112,  0,118,120,122,  0,124,126,  0,116,  0,  0,
  128,  0,136,138,140,  0,142,144,  0,148,150,152,  0,130,132,134,  0,146,
  154,  0,156,158,  0,160,162,  0,  0,  0,164,166,  0,174,176,  0,172,  0,
  186,  0,188,190,192,194,  0,198,200,  0,178,180,184,  0,196,182,202,  0,
  206,  0,208,210,  0,204,  0,  0,212,214,  0,  0,  0,  0,  0,216,  0,218,
  220,  0,224,226,  0,  0,228,  0,230,232,234,  0,236,238,  0,246,248,  0,
  252,254,  0,258,260,  0,  0,240,242,244,  0,250,  0,256,262,264,266,  0,
  268,270,  0,272,274,  0,  0,  0,282,  0,280,284,  0,286,288,  0,  0,  0,
  276,278,  0,  0,  0,292,294,  0,290,  0,296,  0,285,  0,  0,  0,168,170,
    0,  0,  0,  0,222,  0,  0,  0,  0,  0,  0, 35,285,  0, 52,  0,  0, 52,
  285,  0, 35,285,  0,350,339,  0,332,  0,337,352,  0,335,360,330,  0,343,
  333,341,  0, 35,285,  0,330,  0,  0,168,  0,173,  0, 35,285,  0,350,339,
    0,332,  0,337,352,  0,118,120,122,  0,124, 70,126,  0,116,  0,  0,128,
    0,136,138,140,  0,142,144,  0,148,150,152,  0,130,132,134,  0,146,154,
    0,156,158,  0,160,162,  0,  0,  0,164,166,  0,168, 54,  0, 58, 62,  0,
  170,  0,174,176,  0,172,  0,186,  0,188,190,192,194,  0,198,200,  0,178,
  180,184,  0,196,182,202,  0,206,  0,208,210,  0,204,  0,  0,212,214,  0,
    0,  0,  0, 60,  0, 64,216,  0, 76,218,  0,  0,220,  0, 56,222,  0,224,
  226,  0, 66,228,  0,230,232,234,  0,236,238,  0,246,248,  0,252,254,  0,
  258,260,  0,  0,240,242,244,  0,250,  0,256,262,264,266,  0,268,270,  0,
  272,274,  0, 72, 74,  0,282,  0,280,284,  0,286,288,  0,  0,  0,276,278,
    0,  0,  0,292,294,  0,290, 68,  0,296,  0,335,360,330,  0,343,333,341,
    0, 96,  0,100,  0,  0, 94,  0,  0,  0,  0,  0,  0,  0, 98,  0,  0,  0,
    0, 94,  0, 35,285,  0,350,339,  0,332,  0,337,352,  0, 58, 62,  0,170,
    0,174,176,  0,172,  0,186,  0,188,190,192,194,  0,198,200,  0,178,180,
  184,  0,196,182,202,  0,206,  0,208,210,  0,204,  0,  0,212,214,  0,  0,
    0,  0, 60,  0, 64,216,  0, 76,218,  0,  0,220,  0, 56,222,  0,224,226,
    0, 66,228,  0,230,232,234,  0,236,238,  0,246,248,  0,252,254,  0,258,
  260,  0,  0,240,242,244,  0,250,  0,256,262,264,266,  0,268,270,  0,272,
  274,  0, 72, 74,  0,282,  0,280,284,  0,286,288,  0,  0,  0,276,278,  0,
    0,  0,292,294,  0,290, 68,  0,296,  0,335,360,330,  0,343,333,341, 96,
  100,  0, 94,  0,  0,  0,  0,  0,  0,  0, 98,  0,  0,  0,  0, 80, 92,  0,
   84, 86,  0, 52,168, 82,  0, 90,  0, 88,  0,350,339,  0,332,  0,337,352,
    0,118,120,122,  0,124, 70,126,  0,116,  0,  0,128,  0,136,138,140,  0,
  142,144,  0,148,150,152,  0,130,132,134,  0,146,154,  0,156,158,  0,160,
  162,  0,  0,  0,164,166,  0,168, 54,  0, 58, 62,  0,174,176,  0,172,  0,
  186,  0,188,190,192,194,  0,198,200,  0,178,180,184,  0,196,182,202,  0,
  206,  0,208,210,  0,204,  0,  0,212,214,  0,  0,  0,  0, 60,  0, 64,216,
    0, 76,218,  0,  0,220,  0, 56,222,  0,224,226,  0, 66,228,  0,230,232,
  234,  0,236,238,  0,246,248,  0,252,254,  0,258,260,  0,  0,240,242,244,
    0,250,  0,256,262,264,266,  0,268,270,  0,272,274,  0, 72, 74,  0,282,
    0,280,284,  0,286,288,  0,  0,  0,276,278,  0,  0,  0,292,294,  0,290,
   68,  0,296,  0,335,360,330,285,343,333,341,  0,  0,  0,  0,  0,170,  0,
    0,  0,  0,  0,  0,  0,  0,  0,  0,  0,350,339,  0,332,  0,337,352,  0,
  118,120,122,  0,124, 70,126,  0,116,  0,  0,128,  0,136,138,140,  0,142,
  144,  0,148,150,152,  0,130,132,134,  0,146,154,  0,156,158,  0,160,162,
    0,  0,  0,164,166,  0,168, 54,  0, 58, 62,  0,174,176,  0,172,  0,186,
    0,188,190,192,194,  0,198,200,  0,178,180,184,  0,196,182,202,  0,206,
    0,208,210,  0,204,  0,  0,212,214,  0,  0,  0,  0, 60,  0, 64,216,  0,
  218,220,  0, 56,222,  0,224,226,  0, 66,228,  0,230,232,234,  0,236,238,
    0,246,248,  0,252,254,  0,258,260,  0,  0,240,242,244,  0,250,  0,256,
  262,264,266,  0,268,270,  0,272,274,  0, 72, 74,  0,282,  0,280,284,  0,
  286,288,  0,  0,  0,276,278,  0,  0,  0,292,294,  0,290, 68,  0,296,  0,
  335,330,285,343,333,341,  0,  0,  0,  0,  0,170,  0,  0,  0,  0,  0,  0,
    0,  0,  0,  0,  0,339,  0,332,  0,337,  0,118,120,122,  0,124, 70,126,
    0,116,  0,  0,128,  0,136,138,140,  0,142,144,  0,148,150,152,  0,130,
  132,134,  0,146,154,  0,156,158,  0,160,162,  0,  0,  0,164,166,  0,168,
   54,  0, 58, 62,  0,174,176,  0,172,  0,186,  0,188,190,192,194,  0,198,
  200,  0,178,180,184,  0,196,182,202,  0,206,  0,208,210,  0,204,  0,  0,
  212,214,  0,  0,  0,  0, 60,  0, 64,216,  0,218,220,  0, 56,222,  0,224,
  226,  0, 66,228,  0,230,232,234,  0,236,238,  0,246,248,  0,252,254,  0,
  258,260,  0,  0,240,242,244,  0,250,  0,256,262,264,266,  0,268,270,  0,
  272,274,  0,  0,  0,282,  0,280,284,  0,286,288,  0,  0,  0,276,278,  0,
    0,  0,292,294,  0,290, 68,  0,296,  0,335,330,285,343,333,341,  0,  0,
    0,  0,  0,170,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,339,  0,332,
    0,337,  0,118,120,122,  0,124,126,  0,116,  0,  0,128,  0,136,138,140,
    0,142,144,  0,148,150,152,  0,130,132,134,  0,146,154,  0,156,158,  0,
  160,162,  0,  0,  0,164,166,  0,168, 54,  0, 58, 62,  0,174,176,  0,172,
    0,186,  0,188,190,192,194,  0,198,200,  0,178,180,184,  0,196,182,202,
    0,206,  0,208,210,  0,204,  0,  0,212,214,  0,  0,  0,  0, 60,  0, 64,
  216,  0,218,220,  0, 56,222,  0,224,226,  0, 66,228,  0,230,232,234,  0,
  236,238,  0,246,248,  0,252,254,  0,258,260,  0,  0,240,242,244,  0,250,
    0,256,262,264,266,  0,268,270,  0,272,274,  0,  0,  0,282,  0,280,284,
    0,286,288,  0,  0,  0,276,278,  0,  0,  0,292,294,  0,290, 68,  0,296,
    0,335,330,285,343,333,341,  0,  0,  0,  0,  0,170,  0,  0,  0,  0,  0,
    0,  0,  0,  0,  0,  0,339,  0,332,  0,337,  0,118,120,122,  0,124,126,
    0,116,  0,  0,128,  0,136,138,140,  0,142,144,  0,148,150,152,  0,130,
  132,134,  0,146,154,  0,156,158,  0,160,162,  0,  0,  0,164,166,  0,168,
   54,  0, 58, 62,  0,174,176,  0,172,  0,186,  0,188,190,192,194,  0,198,
  200,  0,178,180,184,  0,196,182,202,  0,206,  0,208,210,  0,204,  0,  0,
  212,214,  0,  0,  0,  0, 60,  0, 64,216,  0,218,220,  0, 56,222,  0,224,
  226,  0, 66,228,  0,230,232,234,  0,236,238,  0,246,248,  0,252,254,  0,
  258,260,  0,  0,240,242,244,  0,250,  0,256,262,264,266,  0,268,270,  0,
  272,274,  0,  0,  0,282,  0,280,284,  0,286,288,  0,  0,  0,276,278,  0,
    0,  0,292,294,  0,290,  0,296,  0,335,330,285,343,333,341,  0,  0,  0,
    0,  0,170,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,339,  0,332,  0,
  337,  0, 58, 62,  0, 60, 64,  0,335,285,343,333,341, 54,  0,  0, 56,  0,
  330,285,  0, 35,285,  0,118,120,122,  0,124,126,  0,116,  0,  0,128,  0,
  136,138,140,  0,142,144,  0,148,150,152,  0,130,132,134,  0,146,154,  0,
  156,158,  0,160,162,  0,  0,  0,164,166,  0,174,176,  0,172,  0,186,  0,
  188,190,192,194,  0,198,200,  0,178,180,184,  0,196,182,202,  0,206,  0,
  208,210,  0,204,  0,  0,212,214,  0,  0,  0,  0,  0,216,  0,218,220,  0,
  224,226,  0,  0,228,  0,230,232,234,  0,236,238,  0,246,248,  0,252,254,
    0,258,260,  0,  0,240,242,244,  0,250,  0,256,262,264,266,  0,268,270,
    0,272,274,  0,  0,  0,282,  0,280,284,  0,286,288,  0,  0,  0,276,278,
    0,  0,  0,292,294,  0,290,  0,296,  0,330,  0,  0,  0,  0,168,170,  0,
    0,  0,  0,222,  0,  0,  0,  0,  0,  0,330,  0, 35,285,  0,  0, 96,100,
   94, 98,  0, 82,  0, 80, 92,  0, 84, 86,  0, 52,168, 35,110, 96,102,100,
  104,  0, 94, 90,106,108, 78, 88,  0
};

static const unsigned short ag_key_jmp[] = {
    0,  6,  9,  0, 13, 16,  0,  4, 20,  0,  0,  1,  7, 26, 32,  0,  0,  0,
    0,  0,  0,  0,  0,  0,  0,  0, 39, 41,  0, 37, 19, 23, 26,  0,  0,  0,
    0,  0,  0,  0,  0,  0,  0,  0,  0, 43,  0, 34, 38, 41, 46,  0,  0,  0,
    0,  0,  0,  0,  0, 49, 51,  0, 53,  0, 56,  0, 59,  0,  0, 56,  0, 69,
    0, 58, 61,  0, 73,  0,  0, 53, 71, 76,  0, 66,  0,  0,  0,  0, 83,  0,
    0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0, 90, 92, 97,  0,  0,  0,
    0,  0,  0,  0,  0,108,110,  0,  0,  0,  0,116,  0,113,119, 71, 74,  0,
   76, 81, 83,  0, 85, 88,  0,  0,  0,  0,  0,133, 90,  0,100,104,  0, 92,
   95, 98,140,  0,  0,  0,  0,  0,110,  0,  0,  0,  0,  0,  0,  0,148,  0,
  106,108,151,  0,154,157,112,114,  0,  0,  0,  0,  0,  0,  0,  0,175,  0,
    0,  0,180,  0,121,  0,  0,  0,  0,172,178,116,118,182,186,124,  0,  0,
    0,  0,131,197,134,  0, 10,  0, 16, 29, 45, 62, 78, 63, 87,100,121,126,
  130,137,143,160,189,126,200,  0,145,148,  0,152,155,  0,227,159,  0,139,
  224,230,165,171,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,
    0,  0,  0,  0,  0,  0,183,185,  0,181,250,254,258,  0,  0,  0,  0,  0,
    0,  0,  0,  0,  0,  0,  0,187,  0,190,266,270,273,193,  0,  0,  0,  0,
    0,  0,  0,  0,196,198,  0,286,  0,289,  0,292,  0,  0,203,  0,302,  0,
    0,  0,205,208,  0,308,  0,  0,200,304,306,311,  0,210,  0,  0,  0,  0,
    0,214,216,  0,218,  0,  0,  0,  0,328,  0,  0,  0,  0,  0,  0,  0,  0,
    0,  0,  0,  0,  0,  0,336,338,343,  0,  0,  0,  0,  0,  0,  0,  0,354,
  356,  0,  0,  0,  0,362,  0,223,225,  0,359,  0,365,367,  0,227,  0,229,
    0,  0,  0,  0,231,379,236,  0,  0,  0,  0,238,  0,386,  0,  0,  0,  0,
    0,393,241,  0,253,257,  0,243,246,249,251,400,  0,  0,  0,  0,  0,263,
    0,  0,  0,  0,  0,  0,  0,409,  0,259,261,412,  0,415,418,265,267,  0,
    0,  0,  0,  0,  0,  0,  0,436,  0,  0,273,  0,  0,  0,444,  0,278,  0,
    0,  0,  0,433,269,439,271,442,275,446,450,281,  0,  0,  0,  0,288,291,
  463,293,  0,137,233,  0,176,179,  0,239,242,245,247,261,  0,277,295,313,
  319,322,325,332,346,370,382,389,397,403,421,453,283,466,  0,296,  0,306,
  309,  0,313,316,  0,506,320,  0,300,503,509,326,332,  0,  0,  0,  0,  0,
    0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,344,346,  0,
  342,529,533,537,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,348,  0,
  351,545,549,552,354,  0,  0,  0,  0,  0,  0,  0,  0,357,359,  0,565,  0,
  568,  0,571,  0,  0,364,  0,581,  0,366,369,  0,585,  0,  0,361,583,  0,
  588,  0,371,  0,  0,  0,  0,  0,375,  0,377,  0,  0,  0,  0,604,  0,  0,
    0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,612,614,619,  0,  0,  0,
    0,  0,  0,  0,  0,630,632,  0,  0,  0,  0,638,  0,635,  0,641,382,  0,
  385,  0,387,  0,  0,  0,  0,389,652,394,  0,  0,  0,  0,396,  0,659,  0,
    0,  0,  0,  0,666,399,  0,411,415,  0,401,404,407,409,673,  0,  0,  0,
    0,  0,421,  0,  0,  0,  0,  0,  0,  0,682,  0,417,419,685,  0,688,691,
  423,425,  0,  0,  0,  0,  0,  0,  0,  0,709,  0,  0,429,  0,  0,  0,717,
    0,434,  0,  0,  0,  0,706,712,427,715,431,719,723,437,  0,  0,  0,  0,
  444,447,735,449,  0,298,512,  0,337,340,  0,518,521,524,526,540,  0,556,
  574,590,596,599,602,608,622,643,655,662,670,676,694,726,439,738,  0,461,
    0,  0,  0,467,  0,  0,452,455,457,773,465,776,469,472,  0,476,  0,478,
    0,  0,  0,480,  0,  0,  0,  0,797,  0,482,484,487,  0,  0,  0,  0,  0,
    0,  0,  0,  0,  0,  0,493,495,  0,491,809,813,816,  0,  0,  0,  0,  0,
    0,  0,  0,  0,  0,  0,  0,497,  0,824,828,831,500,  0,  0,  0,  0,  0,
    0,  0,  0,503,505,  0,843,  0,846,  0,849,  0,  0,510,  0,859,  0,514,
  517,  0,863,  0,  0,507,861,512,866,  0,  0,  0,  0,  0,874,  0,  0,  0,
    0,  0,  0,  0,  0,  0,  0,  0,  0,  0,880,882,887,  0,  0,  0,  0,  0,
    0,  0,  0,898,900,  0,  0,  0,  0,906,  0,903,909,522,525,  0,527,532,
  534,  0,536,539,  0,  0,  0,  0,  0,923,541,  0,551,555,  0,543,546,549,
  930,  0,  0,  0,  0,  0,561,  0,  0,  0,  0,  0,  0,  0,938,  0,557,559,
  941,  0,944,947,563,565,  0,  0,  0,  0,  0,  0,  0,  0,965,  0,  0,  0,
  970,  0,574,  0,  0,  0,  0,962,567,968,569,571,972,976,577,  0,  0,  0,
    0,584,988,587,  0,806,819,835,852,868,519,877,890,911,916,920,927,933,
  950,979,579,991,  0,  0,  0,  0,  0,  0,  0,  0,592,594,  0,590,1013,1017,
  1020,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,596,  0,1028,1032,
  1035,599,  0,  0,  0,  0,  0,  0,  0,  0,602,604,  0,1047,  0,1050,  0,
  1053,  0,  0,609,  0,1063,  0,611,614,  0,1067,  0,  0,606,1065,1070,  0,
    0,  0,  0,  0,1077,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,
  1083,1085,1090,  0,  0,  0,  0,  0,  0,  0,  0,1101,1103,  0,  0,  0,  0,
  1109,  0,1106,1112,619,622,  0,624,629,631,  0,633,636,  0,  0,  0,  0,
    0,1126,638,  0,648,652,  0,640,643,646,1133,  0,  0,  0,  0,  0,658,
    0,  0,  0,  0,  0,  0,  0,1141,  0,654,656,1144,  0,1147,1150,660,662,
    0,  0,  0,  0,  0,  0,  0,  0,1168,  0,  0,  0,1173,  0,669,  0,  0,
    0,  0,1165,1171,664,666,1175,1179,672,  0,  0,  0,  0,679,1190,682,  0,
  1023,1039,1056,1072,616,1080,1093,1114,1119,1123,1130,1136,1153,1182,674,
  1193,  0,  0,  0,  0,  0,1214,685,688,  0,  0,  0,691,694,  0,  0,  0,
    0,  0,  0,  0,  0,702,704,  0,700,1227,1231,1234,  0,  0,  0,  0,  0,
    0,  0,  0,  0,  0,  0,  0,706,  0,1242,1246,1249,709,  0,  0,  0,  0,
    0,  0,  0,  0,712,714,  0,1261,  0,1264,  0,1267,  0,  0,719,  0,1277,
    0,723,726,  0,1281,  0,  0,716,1279,721,1284,  0,  0,  0,  0,  0,1292,
    0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,1298,1300,1305,  0,
    0,  0,  0,  0,  0,  0,  0,1316,1318,  0,  0,  0,  0,1324,  0,1321,1327,
  731,734,  0,736,741,743,  0,745,748,  0,  0,  0,  0,  0,1341,750,  0,760,
  764,  0,752,755,758,1348,  0,  0,  0,  0,  0,770,  0,  0,  0,  0,  0,  0,
    0,1356,  0,766,768,1359,  0,1362,1365,772,774,  0,  0,  0,  0,  0,  0,
    0,  0,1383,  0,  0,  0,1388,  0,783,  0,  0,  0,  0,1380,776,1386,778,
  780,1390,1394,786,  0,  0,  0,  0,793,1406,796,  0,698,1237,1253,1270,
  1286,728,1295,1308,1329,1334,1338,1345,1351,1368,1397,788,1409,  0,799,
    0,810,  0,  0,  0,816,  0,  0,801,804,806,1433,814,1436,818,  0,  0,
    0,  0,  0,  0,  0,  0,  0,  0,  0,822,1451,1455,824,  0,  0,  0,  0,
    0,  0,  0,  0,  0,  0,  0,  0,827,  0,1463,1467,1470,  0,  0,  0,  0,
    0,  0,  0,  0,1481,1484,  0,830,  0,  0,  0,  0,1492,  0,  0,  0,  0,
    0,  0,  0,  0,  0,  0,  0,  0,  0,1497,1499,1504,  0,  0,  0,  0,  0,
    0,  0,  0,1515,1517,  0,  0,  0,  0,1523,  0,1520,1526,838,  0,840,842,
    0,  0,  0,  0,1535,847,  0,849,852,854,  0,  0,  0,  0,  0,861,  0,  0,
    0,  0,  0,  0,  0,1545,  0,857,859,1548,  0,1551,1554,863,865,  0,  0,
    0,  0,  0,  0,  0,  0,1572,  0,  0,  0,1577,  0,  0,  0,  0,  0,1569,
  1575,867,869,1579,1582,  0,  0,  0,  0,872,1592,875,  0,1448,1458,1474,
  1487,833,835,1495,1507,1528,1532,844,1538,1541,1557,1585,1595,  0,  0,
    0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,
  882,1627,1631,884,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,887,
    0,1640,1644,1647,  0,  0,  0,  0,  0,  0,  0,  0,1658,1661,  0,890,  0,
    0,  0,  0,  0,  0,  0,  0,  0,  0,1675,  0,  0,  0,  0,  0,  0,  0,  0,
    0,  0,  0,  0,  0,1680,1682,1687,  0,  0,  0,  0,  0,  0,  0,  0,1698,
  1700,  0,  0,  0,  0,1706,  0,1703,  0,1709,  0,896,  0,  0,  0,  0,1717,
  898,  0,  0,900,  0,  0,  0,  0,1726,902,  0,904,907,909,  0,  0,  0,  0,
    0,916,  0,  0,  0,  0,  0,  0,  0,1736,  0,912,914,1739,  0,1742,1745,
  918,920,  0,  0,  0,  0,  0,  0,  0,  0,1763,  0,  0,  0,  0,1769,  0,
    0,  0,  0,  0,1760,1766,922,924,1771,1774,  0,  0,  0,  0,927,930,1784,
  932,  0,878,880,  0,1616,1619,1622,1624,1635,1651,1664,1669,1672,893,1678,
  1690,1711,1720,1723,1729,1732,1748,1777,1787,  0,  0,  0,  0,  0,  0,  0,
    0,  0,  0,  0,938,1819,1823,940,  0,  0,  0,  0,  0,  0,  0,  0,  0,
    0,  0,  0,943,  0,946,1831,1835,1838,  0,  0,  0,  0,  0,  0,  0,  0,
  1850,1853,  0,949,  0,954,  0,  0,  0,  0,  0,1864,  0,  0,  0,  0,  0,
    0,  0,  0,  0,  0,  0,  0,  0,  0,1870,1872,1877,  0,  0,  0,  0,  0,
    0,  0,  0,1888,1890,  0,  0,  0,  0,1896,  0,1893,1899,  0,961,963,  0,
  965,967,  0,  0,  0,  0,1910,972,  0,974,977,979,  0,  0,  0,  0,  0,986,
    0,  0,  0,  0,  0,  0,  0,1920,  0,982,984,1923,  0,1926,1929,988,990,
    0,  0,  0,  0,  0,  0,  0,  0,1947,  0,  0,  0,1952,  0,  0,  0,  0,
    0,1944,1950,992,994,997,1954,1957,  0,  0,  0,  0,1000,1968,1003,  0,
    0,935,1816,1826,1842,1856,952,1861,958,1867,1880,1901,1907,969,1913,
  1916,1932,1960,1971,  0,1006,  0,  0,  0,  0,  0,  0,  0,  0,  0,1008,
    0,  0,  0,1010,  0,  0,  0,  0,1012,  0,1015,  0,  0,  0,1017,  0,1026,
    0,  0,  0,1032,  0,  0,1019,1022,2022,1030,2025,1034,1037,  0,  0,  0,
    0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,1041,  0,  0,  0,
    0,1044,  0,  0,  0,  0,  0,  0,  0,  0,1048,2060,2064,1050,  0,  0,  0,
    0,  0,  0,  0,  0,  0,  0,  0,  0,1053,  0,2072,2076,2079,  0,  0,  0,
    0,  0,  0,  0,  0,2090,2093,  0,1056,  0,  0,  0,  0,2101,  0,  0,  0,
    0,  0,  0,  0,  0,  0,  0,  0,  0,  0,2106,2108,2113,  0,  0,  0,  0,
    0,  0,  0,  0,2124,2126,  0,  0,  0,  0,2132,  0,2129,2135,1064,  0,
  1066,1068,  0,  0,  0,  0,2144,1073,  0,1075,1078,1080,  0,  0,  0,  0,
    0,1087,  0,  0,  0,  0,  0,  0,  0,2154,  0,1083,1085,2157,  0,2160,
  2163,1089,1091,  0,  0,  0,  0,  0,  0,  0,  0,2181,  0,  0,  0,2186,  0,
    0,  0,  0,  0,2178,2184,1093,1095,2188,2191,  0,  0,  0,  0,1098,2201,
  1101,  0,1046,2067,2083,2096,1059,1061,2104,2116,2137,2141,1070,2147,2150,
  2166,2194,2204,  0,  0,  0,  0,  0,2225,  0,  0,1104,  0,  0,  0,  0,  0,
    0,  0,  0,  0,  0,  0,  0,1106,1108,  0,2234,2237,2240,2242,  0,  0,
    0,  0,  0,2253,  0,1110,  0,1113,  0,  0,  0,  0,  0,  0,  0,  0,  0,
    0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,1119,2274,2278,1121,  0,  0,
    0,  0,  0,  0,  0,  0,  0,  0,  0,  0,1124,  0,2287,2291,2294,  0,  0,
    0,  0,  0,  0,  0,  0,2305,2308,  0,1127,  0,  0,  0,  0,  0,  0,  0,
  1130,  0,  0,  0,  0,2324,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,
    0,  0,2329,2331,2336,  0,  0,  0,  0,  0,  0,  0,  0,2347,2349,  0,  0,
    0,  0,2355,  0,2352,  0,2358,  0,1132,  0,  0,  0,  0,2366,1134,  0,
    0,1136,  0,  0,  0,  0,2375,1138,  0,1140,1143,1145,  0,  0,  0,  0,
    0,1152,  0,  0,  0,  0,  0,  0,  0,2385,  0,1148,1150,2388,  0,2391,
  2394,1154,1156,  0,  0,  0,  0,  0,  0,  0,  0,2412,  0,  0,  0,  0,2418,
    0,  0,  0,  0,  0,2409,2415,1158,1160,2420,2423,  0,  0,  0,  0,1163,
  1166,2433,1168,  0,1115,1117,  0,2263,2266,2269,2271,2282,  0,2298,2311,
  2316,2319,2322,2327,2339,2360,2369,2372,2378,2381,  0,2397,2426,2436,  0,
    0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,1175,  0,
    0,  0,  0,2485,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,2490,
  2492,2497,  0,  0,  0,  0,  0,  0,  0,  0,2508,2510,  0,  0,  0,  0,2516,
    0,2513,  0,2519,  0,1177,  0,  0,  0,  0,2527,1179,  0,  0,1181,  0,
    0,  0,  0,2536,1183,  0,1185,1188,1190,  0,  0,  0,  0,  0,1197,  0,
    0,  0,  0,  0,  0,  0,2546,  0,1193,1195,2549,  0,2552,2555,1199,1201,
    0,  0,  0,  0,  0,  0,  0,  0,2573,  0,  0,  0,  0,2579,  0,  0,  0,
    0,  0,2570,2576,1203,1205,2581,2584,  0,  0,  0,  0,1208,1211,2594,1213,
    0,1171,1173,  0,2469,2472,2475,2477,  0,  0,2480,2483,2488,2500,2521,
  2530,2533,2539,2542,  0,2558,2587,2597,  0,1223,  0,  0,  0,1229,  0,  0,
  1216,1219,2625,1227,2628,1231,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,
    0,  0,  0,  0,  0,  0,  0,1241,2647,2651,1243,  0,  0,  0,  0,  0,  0,
    0,  0,  0,  0,  0,  0,1246,  0,2660,2664,2667,  0,  0,  0,  0,  0,  0,
    0,  0,2678,2681,  0,1249,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,2695,
    0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,2700,2702,2707,  0,
    0,  0,  0,  0,  0,  0,  0,2718,2720,  0,  0,  0,  0,2726,  0,2723,  0,
  2729,  0,1255,  0,  0,  0,  0,2737,1257,  0,  0,1259,  0,  0,  0,  0,2746,
  1261,  0,1263,1266,1268,  0,  0,  0,  0,  0,1275,  0,  0,  0,  0,  0,  0,
    0,2756,  0,1271,1273,2759,  0,2762,2765,1277,1279,  0,  0,  0,  0,  0,
    0,  0,  0,2783,  0,  0,  0,  0,2789,  0,  0,  0,  0,  0,2780,2786,1281,
  1283,2791,2794,  0,  0,  0,  0,1286,1289,2804,1291,  0,1235,1237,  0,1239,
  2639,2642,2644,2655,2671,2684,2689,2692,1252,2698,2710,2731,2740,2743,
  2749,2752,2768,2797,2807,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,
    0,  0,  0,  0,  0,1298,2844,2848,1300,  0,  0,  0,  0,  0,  0,  0,  0,
    0,  0,  0,  0,1303,  0,2857,2861,2864,  0,  0,  0,  0,  0,  0,  0,  0,
  2875,2878,  0,1306,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,2892,  0,  0,
    0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,2897,2899,2904,  0,  0,  0,
    0,  0,  0,  0,  0,2915,2917,  0,  0,  0,  0,2923,  0,2920,  0,2926,  0,
  1312,  0,1314,1316,  0,  0,1318,  0,  0,  0,  0,2940,1320,  0,1322,1325,
  1327,  0,  0,  0,  0,  0,1334,  0,  0,  0,  0,  0,  0,  0,2950,  0,1330,
  1332,2953,  0,2956,2959,1336,1338,  0,  0,  0,  0,  0,  0,  0,  0,2977,
    0,  0,  0,  0,2983,  0,  0,  0,  0,  0,2974,2980,1340,1342,2985,2988,
    0,  0,  0,  0,1345,1348,2998,1350,  0,1294,  0,1296,2836,2839,2841,2852,
  2868,2881,2886,2889,1309,2895,2907,2928,2934,2937,2943,2946,2962,2991,
  3001,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,1357,
  3035,3039,1359,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,1362,  0,
  3048,3052,3055,  0,  0,  0,  0,  0,  0,  0,  0,3066,3069,  0,1365,  0,
    0,  0,  0,  0,  0,  0,  0,  0,  0,3083,  0,  0,  0,  0,  0,  0,  0,  0,
    0,  0,  0,  0,  0,3088,3090,3095,  0,  0,  0,  0,  0,  0,  0,  0,3106,
  3108,  0,  0,  0,  0,3114,  0,3111,  0,3117,  0,1371,  0,1373,1375,  0,
    0,1377,  0,  0,  0,  0,3131,1379,  0,1381,1384,1386,  0,  0,  0,  0,
    0,1393,  0,  0,  0,  0,  0,  0,  0,3141,  0,1389,1391,3144,  0,3147,
  3150,1395,1397,  0,  0,  0,  0,  0,  0,  0,  0,3168,  0,  0,  0,3173,  0,
    0,  0,  0,  0,3165,3171,1399,1401,3175,3178,  0,  0,  0,  0,1404,1407,
  3188,1409,  0,1353,  0,1355,3029,3031,3033,3043,3059,3072,3077,3080,1368,
  3086,3098,3119,3125,3128,3134,3137,3153,3181,3191,  0,  0,  0,  0,  0,
    0,  0,  0,  0,  0,  0,  0,  0,  0,1416,3225,3229,1418,  0,  0,  0,  0,
    0,  0,  0,  0,  0,  0,  0,  0,1421,  0,3237,3241,3244,  0,  0,  0,  0,
    0,  0,  0,  0,3255,3258,  0,1424,  0,  0,  0,  0,  0,  0,  0,  0,  0,
    0,3272,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,3277,3279,
  3284,  0,  0,  0,  0,  0,  0,  0,  0,3295,3297,  0,  0,  0,  0,3303,  0,
  3300,  0,3306,  0,1430,  0,1432,1434,  0,  0,1436,  0,  0,  0,  0,3320,
  1438,  0,1440,1443,1445,  0,  0,  0,  0,  0,1452,  0,  0,  0,  0,  0,  0,
    0,3330,  0,1448,1450,3333,  0,3336,3339,1454,1456,  0,  0,  0,  0,  0,
    0,  0,  0,3357,  0,  0,  0,3362,  0,  0,  0,  0,  0,3354,3360,1458,1460,
  3364,3367,  0,  0,  0,  0,1463,1466,3377,1468,  0,1412,  0,1414,3219,3221,
  3223,3232,3248,3261,3266,3269,1427,3275,3287,3308,3314,3317,3323,3326,
  3342,3370,3380,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,
  1475,3414,3418,1477,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,1480,
    0,3426,3430,3433,  0,  0,  0,  0,  0,  0,  0,  0,3444,3447,  0,1483,
    0,  0,  0,  0,  0,  0,  0,  0,  0,  0,3461,  0,  0,  0,  0,  0,  0,  0,
    0,  0,  0,  0,  0,  0,3466,3468,3473,  0,  0,  0,  0,  0,  0,  0,  0,
  3484,3486,  0,  0,  0,  0,3492,  0,3489,  0,3495,  0,1489,  0,1491,1493,
    0,  0,1495,  0,  0,  0,  0,3509,1497,  0,1499,1502,1504,  0,  0,  0,
    0,  0,1511,  0,  0,  0,  0,  0,  0,  0,3519,  0,1507,1509,3522,  0,3525,
  3528,1513,1515,  0,  0,  0,  0,  0,  0,  0,  0,3546,  0,  0,  0,3551,  0,
    0,  0,  0,  0,3543,3549,1517,1519,3553,3556,  0,  0,  0,  0,1522,3566,
  1525,  0,1471,  0,1473,3408,3410,3412,3421,3437,3450,3455,3458,1486,3464,
  3476,3497,3503,3506,3512,3515,3531,3559,3569,  0,  0,  0,  0,  0,  0,  0,
    0,  0,  0,  0,  0,  0,1528,1530,3596,3598,3600,1532,3602,3605,1534,  0,
    0,1536,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,1538,3624,3628,1540,
    0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,1543,  0,3636,3640,3643,
    0,  0,  0,  0,  0,  0,  0,  0,3654,3657,  0,1546,  0,  0,  0,  0,3665,
    0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,3670,3672,3677,  0,
    0,  0,  0,  0,  0,  0,  0,3688,3690,  0,  0,  0,  0,3696,  0,3693,3699,
  1554,  0,1556,1558,  0,  0,  0,  0,3708,1563,  0,1565,1568,1570,  0,  0,
    0,  0,  0,1577,  0,  0,  0,  0,  0,  0,  0,3718,  0,1573,1575,3721,  0,
  3724,3727,1579,1581,  0,  0,  0,  0,  0,  0,  0,  0,3745,  0,  0,  0,3750,
    0,  0,  0,  0,  0,3742,3748,1583,1585,3752,3755,  0,  0,  0,  0,1588,
  3765,1591,  0,  0,3621,3631,3647,3660,1549,1551,3668,3680,3701,3705,1560,
  3711,3714,3730,3758,3768,  0,  0,  0,  0,  0,  0,3792,  0,  0,  0,  0,
    0,1599,  0,1602,  0,  0,  0,1608,  0,  0,1594,1597,  0,  0,3801,  0,
    0,3803,  0,1606,3806,  0,1610,1613,  0
};

static const unsigned short ag_key_index[] = {
  204,  0,471,204,501,743,204,204,779,789,789,  0,  0,  0,791,794,794,800,
  800,794,794,802,789,789,995,1197,1218,1222,1222,1222,1222,1224,  0,  0,
  1413,1431,789,1439,779,789,779,789,789,789,789,789,789,789,789,789,789,
  1599,1599,1439,789,779,779,1439,1792,779,789,779,1439,789,789,1975,789,
    0,  0,  0,  0,1599,779,1995,1599,779,1995,1599,2005,779,1599,1599,1599,
  779,779,1995,1599,789,1599,1599,1599,1599,1599,1599,1599,1599,1599,1599,
  1599,1599,1599,1599,2009,2009,1599,779,779,1995,1599,1995,1995,2016,1599,
  779,779,779,2005,779,779,779,779,779,779,779,779,779,779,779,779,779,779,
  2016,1995,779,1599,1599,1599,1599,2016,1995,2016,1599,779,779,779,779,
  779,779,779,1995,1599,1599,779,779,779,1599,779,1995,779,1995,1995,779,
  2028,2037,2028,2037,2046,2028,2028,2028,2037,  0,2049,2049,2028,2028,2037,
  2037,2037,2055,2028,2028,2028,2046,2028,2028,2028,2028,2028,2028,2028,
  2028,2028,2028,2028,2028,2028,2028,2055,2037,2028,2055,2037,2055,2028,
  2028,2028,2028,2028,2028,2028,2037,2028,2028,2028,2028,2037,2028,2037,
  2037,2028,2208,794,1222,789,  0,800,800,1431,789,  0,800,800,779,2028,
  2228,2231,779,2028,794,1222,794,1222,794,1222,794,1222,779,2028,779,2028,
  779,2028,800,800,800,779,2028,2028,2028,2028,779,2028,779,2028,2245,800,
  800,  0,800,  0,2256,789,2028,2028,204,2028,2037,2028,2037,2046,2028,2028,
  2028,2037,  0,2049,2049,2028,2028,2037,2037,2037,2055,2028,2028,2028,2046,
  2028,2028,2028,2028,2028,2028,2028,2028,2028,2028,2028,2028,2028,2028,
  2055,2037,2028,2055,2037,2055,2028,2028,2028,2028,2028,2028,2028,2037,
  2028,2028,2028,2028,2037,2028,2037,2037,2028,2208,2259,2261,2261,2256,
    0,2441,  0,  0,2261,1792,2467,1792,2602,789,789,789,789,789,789,789,
  1792,1792,  0,  0,  0,  0,  0,  0,  0,1439,2028,2631,2631,2631,2812,3006,
  3006,3196,3385,3573,3608,2028,1431,3618,3618,3618,2261,3618,3618,3618,
  3618,  0,  0,3772,1599,1599,1599,1599,1599,3790,3790,1599,  0,3795,  0,
  2261,  0,  0,2028,2028,2028,2028,2028,2028,2028,  0,1439,2631,1439,2631,
  2631,2631,2631,2028,2028,779,2028,779,2028,779,2028,779,2028,779,2028,
  2028,779,2028,2028,779,2028,2028,779,779,779,779,779,779,779,779,779,779,
  779,779,779,3790,2261,3618,3618,3618,3618,3790,3790,  0,  0,  0,  0,  0,
    0,  0,  0,2812,2631,2631,2631,2631,2631,2028,2028,2028,2028,2028,2028,
  2028,2028,2028,2028,2028,2028,3809,1222,2028,2028,2037,2028,2812,2812,
  3006,3006,3006,3006,3006,3006,3196,3196,3385,3385,1222,2028,2028,2037,
  2028
};

static const unsigned char ag_key_ends[] = {
69,70,73,78,69,0, 83,69,0, 68,73,70,0, 69,70,0, 68,69,70,0, 
67,76,85,68,69,0, 82,65,71,77,65,0, 78,68,69,70,0, 73,0, 71,0, 
82,0, 76,76,0, 69,71,0, 71,0, 66,0, 83,69,0, 70,0, 82,78,0, 
78,0, 76,84,0, 76,85,68,69,0, 78,75,0, 73,0, 67,76,73,66,0, 
86,0, 73,0, 77,69,0, 80,0, 84,0, 71,69,0, 72,76,0, 80,0, 
76,73,67,0, 72,0, 84,0, 77,0, 69,0, 67,0, 84,0, 77,0, 
72,76,0, 76,78,0, 77,0, 73,84,76,69,0, 72,71,0, 72,76,0, 61,0, 
69,70,73,78,69,0, 83,69,0, 68,73,70,0, 69,70,0, 68,69,70,0, 
67,76,85,68,69,0, 82,65,71,77,65,0, 78,68,69,70,0, 92,39,0, 42,0, 
73,0, 71,0, 82,0, 76,76,0, 73,76,0, 69,71,0, 71,0, 66,0, 
83,69,0, 70,0, 82,78,0, 78,0, 79,79,82,0, 88,0, 84,0, 
76,85,68,69,0, 75,0, 84,0, 71,0, 73,0, 67,76,73,66,0, 73,0, 
77,69,0, 84,0, 71,69,0, 72,76,0, 80,0, 87,0, 76,73,67,0, 72,0, 
84,0, 77,0, 69,0, 67,0, 84,0, 84,0, 77,0, 76,0, 82,84,0, 
76,78,0, 77,0, 73,84,76,69,0, 72,71,0, 82,0, 72,76,0, 47,0, 
61,0, 69,70,73,78,69,0, 83,69,0, 68,73,70,0, 69,70,0, 
68,69,70,0, 67,76,85,68,69,0, 82,65,71,77,65,0, 78,68,69,70,0, 
92,39,0, 42,0, 73,0, 71,0, 82,0, 76,76,0, 73,76,0, 69,71,0, 
71,0, 66,0, 83,69,0, 70,0, 82,78,0, 78,0, 79,79,82,0, 84,0, 
76,85,68,69,0, 78,75,0, 71,0, 73,0, 67,76,73,66,0, 73,0, 
77,69,0, 84,0, 71,69,0, 72,76,0, 80,0, 87,0, 76,73,67,0, 72,0, 
84,0, 77,0, 69,0, 67,0, 84,0, 77,0, 76,0, 82,84,0, 76,78,0, 
77,0, 73,84,76,69,0, 72,71,0, 82,0, 72,76,0, 92,39,0, 42,0, 
69,73,76,0, 79,79,82,0, 80,0, 71,0, 79,84,0, 81,82,84,0, 42,0, 
42,0, 42,0, 42,0, 69,88,0, 73,83,84,0, 73,0, 71,0, 82,0, 
76,76,0, 69,71,0, 71,0, 66,0, 83,69,0, 70,0, 85,0, 82,78,0, 
78,0, 76,84,0, 78,75,0, 73,0, 67,76,73,66,0, 86,0, 73,0, 
77,69,0, 80,0, 84,0, 71,69,0, 72,76,0, 80,0, 76,73,67,0, 72,0, 
84,0, 77,0, 69,0, 67,0, 84,0, 84,0, 77,0, 72,76,0, 76,78,0, 
77,0, 73,84,76,69,0, 72,71,0, 72,76,0, 73,0, 71,0, 82,0, 
76,76,0, 69,71,0, 71,0, 66,0, 83,69,0, 70,0, 82,78,0, 78,0, 
76,84,0, 78,75,0, 73,0, 67,76,73,66,0, 86,0, 73,0, 77,69,0, 
80,0, 84,0, 71,69,0, 72,76,0, 80,0, 76,73,67,0, 72,0, 84,0, 
77,0, 69,0, 67,0, 84,0, 77,0, 72,76,0, 76,78,0, 77,0, 
73,84,76,69,0, 72,71,0, 72,76,0, 81,85,0, 69,84,0, 69,88,0, 
73,83,84,0, 47,0, 73,0, 71,0, 82,0, 76,76,0, 69,71,0, 71,0, 
66,0, 83,69,0, 70,0, 85,0, 82,78,0, 78,0, 76,84,0, 78,75,0, 
73,0, 67,76,73,66,0, 86,0, 73,0, 77,69,0, 80,0, 84,0, 71,69,0, 
72,76,0, 80,0, 76,73,67,0, 72,0, 84,0, 77,0, 69,0, 67,0, 
84,0, 84,0, 77,0, 72,76,0, 76,78,0, 77,0, 73,84,76,69,0, 
72,71,0, 72,76,0, 47,0, 92,39,0, 42,0, 69,73,76,0, 79,79,82,0, 
80,0, 71,0, 81,82,84,0, 73,0, 72,82,0, 76,76,0, 85,66,0, 73,0, 
76,84,0, 73,0, 86,0, 73,0, 79,80,0, 84,0, 72,76,0, 80,0, 
83,72,0, 84,0, 77,0, 69,0, 67,0, 84,0, 77,0, 72,76,0, 
72,71,0, 72,76,0, 61,0, 42,0, 73,0, 72,82,0, 76,76,0, 85,66,0, 
76,84,0, 73,0, 73,0, 80,0, 84,0, 72,76,0, 80,0, 83,72,0, 
84,0, 77,0, 69,0, 67,0, 84,0, 77,0, 72,76,0, 72,71,0, 82,0, 
72,76,0, 92,39,0, 73,0, 72,82,0, 76,76,0, 73,76,0, 85,66,0, 
73,0, 79,79,82,0, 76,84,0, 71,0, 73,0, 86,0, 73,0, 79,80,0, 
84,0, 72,76,0, 80,0, 83,72,0, 84,0, 77,0, 69,0, 67,0, 84,0, 
77,0, 72,76,0, 82,84,0, 72,71,0, 72,76,0, 42,0, 42,0, 42,0, 
83,87,0, 42,0, 80,0, 92,39,0, 69,73,76,0, 79,79,82,0, 80,0, 
71,0, 79,84,0, 81,82,84,0, 83,87,0, 80,0, 47,0, 73,0, 72,82,0, 
76,76,0, 85,66,0, 73,0, 76,84,0, 73,0, 86,0, 73,0, 79,80,0, 
84,0, 72,76,0, 80,0, 83,72,0, 84,0, 77,0, 69,0, 67,0, 84,0, 
77,0, 72,76,0, 72,71,0, 72,76,0, 47,0, 61,0, 42,0, 92,39,0, 
39,0, 61,0, 42,0, 73,0, 72,82,0, 76,76,0, 85,66,0, 84,0, 
73,0, 73,0, 80,0, 84,0, 72,76,0, 80,0, 83,72,0, 84,0, 77,0, 
69,0, 67,0, 84,0, 77,0, 72,76,0, 72,71,0, 82,0, 72,76,0, 
61,0, 42,0, 84,0, 73,0, 73,0, 80,0, 84,0, 72,76,0, 80,0, 
83,72,0, 84,0, 77,0, 69,0, 67,0, 84,0, 77,0, 72,76,0, 
72,71,0, 82,0, 72,76,0, 92,39,0, 69,73,76,0, 79,79,82,0, 80,0, 
71,0, 81,82,84,0, 61,0, 42,0, 47,0, 73,0, 72,82,0, 76,76,0, 
85,66,0, 76,84,0, 73,0, 73,0, 80,0, 84,0, 72,76,0, 80,0, 
83,72,0, 84,0, 77,0, 69,0, 67,0, 84,0, 77,0, 72,76,0, 
72,71,0, 82,0, 72,76,0, 61,0, 47,0, 73,0, 72,82,0, 76,76,0, 
85,66,0, 76,84,0, 73,0, 86,0, 73,0, 80,0, 84,0, 72,76,0, 
80,0, 83,72,0, 84,0, 77,0, 69,0, 67,0, 84,0, 77,0, 72,76,0, 
72,71,0, 82,0, 72,76,0, 61,0, 47,0, 73,0, 72,82,0, 76,76,0, 
85,66,0, 76,84,0, 73,0, 86,0, 73,0, 80,0, 84,0, 72,76,0, 
80,0, 83,72,0, 84,0, 77,0, 69,0, 67,0, 84,0, 77,0, 72,76,0, 
72,71,0, 82,0, 72,76,0, 61,0, 47,0, 73,0, 72,82,0, 76,76,0, 
85,66,0, 76,84,0, 73,0, 86,0, 73,0, 80,0, 84,0, 72,76,0, 
80,0, 83,72,0, 84,0, 77,0, 69,0, 67,0, 84,0, 77,0, 72,76,0, 
72,71,0, 82,0, 72,76,0, 61,0, 47,0, 73,0, 72,82,0, 76,76,0, 
85,66,0, 76,84,0, 73,0, 86,0, 73,0, 80,0, 84,0, 72,76,0, 
80,0, 83,72,0, 84,0, 77,0, 69,0, 67,0, 84,0, 77,0, 72,76,0, 
72,71,0, 72,76,0, 61,0, 47,0, 81,0, 69,0, 47,0, 73,0, 
72,82,0, 76,76,0, 85,66,0, 73,0, 76,84,0, 73,0, 86,0, 73,0, 
79,80,0, 84,0, 72,76,0, 80,0, 83,72,0, 84,0, 77,0, 69,0, 
67,0, 84,0, 77,0, 72,76,0, 72,71,0, 72,76,0, 92,39,0, 42,0, 
73,76,0, 79,79,82,0, 80,0, 71,0, 79,84,0, 81,82,84,0, 
};
#define AG_TCV(x) (((int)(x) >= -1 && (int)(x) <= 255) ? ag_tcv[(x) + 1] : 0)

static const unsigned short ag_tcv[] = {
   26, 26,472,472,472,472,472,472,472,472,  1,288,472,472,472,472,472,472,
  472,472,472,472,472,472,472,472,472,472,472,472,472,472,472,  1,361,473,
  472,474,358,348,475,365,364,356,354,287,355,476,357,477,478,478,478,478,
  478,478,478,479,479,472,286,480,472,481,472,472,482,482,482,482,482,482,
  483,483,483,483,483,483,483,484,483,483,483,485,483,486,483,483,483,487,
  483,483,472,488,472,346,489,472,482,482,482,482,482,482,483,483,483,483,
  483,483,483,484,483,483,483,485,483,486,483,483,483,487,483,483,472,344,
  472,363,490,490,490,490,490,490,490,490,490,490,490,490,490,490,490,490,
  490,490,490,490,490,490,490,490,490,490,490,490,490,490,490,490,490,490,
  490,490,490,490,490,490,490,490,490,490,490,490,490,490,490,490,490,490,
  490,490,490,490,490,490,490,490,490,490,490,490,490,490,490,490,490,490,
  490,490,490,490,490,490,490,490,490,490,490,490,490,490,490,490,490,490,
  490,490,490,490,490,490,490,490,490,490,490,490,490,490,490,490,490,490,
  490,490,490,490,490,490,490,490,490,490,490,490,490,490,490,490,490,490,
  490,490,490,490,490
};

#ifndef SYNTAX_ERROR
#define SYNTAX_ERROR fprintf(stderr,"%s, line %d, column %d\n", \
  (PCB).error_message, (PCB).line, (PCB).column)
#endif

#ifndef FIRST_LINE
#define FIRST_LINE 1
#endif

#ifndef FIRST_COLUMN
#define FIRST_COLUMN 1
#endif

#ifndef PARSER_STACK_OVERFLOW
#define PARSER_STACK_OVERFLOW {fprintf(stderr, \
   "\nParser stack overflow, line %d, column %d\n",\
   (PCB).line, (PCB).column);}
#endif

#ifndef REDUCTION_TOKEN_ERROR
#define REDUCTION_TOKEN_ERROR {fprintf(stderr, \
    "\nReduction token error, line %d, column %d\n", \
    (PCB).line, (PCB).column);}
#endif


typedef enum
  {ag_accept_key, ag_set_key, ag_jmp_key, ag_end_key, ag_no_match_key,
   ag_cf_accept_key, ag_cf_set_key, ag_cf_end_key} key_words;

#ifndef GET_INPUT
#define GET_INPUT ((PCB).input_code = getchar())
#endif


static int ag_look_ahead(void) {
  if ((PCB).rx < (PCB).fx) {
    return CONVERT_CASE((PCB).lab[(PCB).rx++]);
  }
  GET_INPUT;
  (PCB).fx++;
  return CONVERT_CASE((PCB).lab[(PCB).rx++] = (PCB).input_code);
}

static void ag_get_key_word(int ag_k) {
  int save_index = (PCB).rx;
  const  unsigned char *sp;
  int ag_ch;
  while (1) {
    switch (ag_key_act[ag_k]) {
    case ag_cf_end_key:
      sp = ag_key_ends + ag_key_jmp[ag_k];
      do {
        if ((ag_ch = *sp++) == 0) {
          int ag_k1 = ag_key_parm[ag_k];
          int ag_k2 = ag_key_pt[ag_k1];
          if (ag_key_itt[ag_k2 + ag_look_ahead()]) goto ag_fail;
          (PCB).rx--;
          (PCB).token_number = (a85parse_token_type) ag_key_pt[ag_k1 + 1];
          return;
        }
      } while (ag_look_ahead() == ag_ch);
      goto ag_fail;
    case ag_end_key:
      sp = ag_key_ends + ag_key_jmp[ag_k];
      do {
        if ((ag_ch = *sp++) == 0) {
          (PCB).token_number = (a85parse_token_type) ag_key_parm[ag_k];
          return;
        }
      } while (ag_look_ahead() == ag_ch);
    case ag_no_match_key:
ag_fail:
      (PCB).rx = save_index;
      return;
    case ag_cf_set_key: {
      int ag_k1 = ag_key_parm[ag_k];
      int ag_k2 = ag_key_pt[ag_k1];
      ag_k = ag_key_jmp[ag_k];
      if (ag_key_itt[ag_k2 + (ag_ch = ag_look_ahead())]) break;
      save_index = --(PCB).rx;
      (PCB).token_number = (a85parse_token_type) ag_key_pt[ag_k1+1];
      break;
    }
    case ag_set_key:
      save_index = (PCB).rx;
      (PCB).token_number = (a85parse_token_type) ag_key_parm[ag_k];
    case ag_jmp_key:
      ag_k = ag_key_jmp[ag_k];
      ag_ch = ag_look_ahead();
      break;
    case ag_accept_key:
      (PCB).token_number =  (a85parse_token_type) ag_key_parm[ag_k];
      return;
    case ag_cf_accept_key: {
      int ag_k1 = ag_key_parm[ag_k];
      int ag_k2 = ag_key_pt[ag_k1];
      if (ag_key_itt[ag_k2 + ag_look_ahead()]) (PCB).rx = save_index;
      else {
        (PCB).rx--;
        (PCB).token_number =  (a85parse_token_type) ag_key_pt[ag_k1+1];
      }
      return;
    }
    }
    while (ag_key_ch[ag_k] < ag_ch) ag_k++;
    if (ag_key_ch[ag_k] != ag_ch) {
      (PCB).rx = save_index;
      return;
    }
  }
}


#ifndef AG_NEWLINE
#define AG_NEWLINE 10
#endif

#ifndef AG_RETURN
#define AG_RETURN 13
#endif

#ifndef AG_FORMFEED
#define AG_FORMFEED 12
#endif

#ifndef AG_TABCHAR
#define AG_TABCHAR 9
#endif

static void ag_track(void) {
  int ag_k = 0;
  while (ag_k < (PCB).rx) {
    int ag_ch = (PCB).lab[ag_k++];
    switch (ag_ch) {
    case AG_NEWLINE:
      (PCB).column = 1, (PCB).line++;
    case AG_RETURN:
    case AG_FORMFEED:
      break;
    case AG_TABCHAR:
      (PCB).column += (TAB_SPACING) - ((PCB).column - 1) % (TAB_SPACING);
      break;
    default:
      (PCB).column++;
    }
  }
  ag_k = 0;
  while ((PCB).rx < (PCB).fx) (PCB).lab[ag_k++] = (PCB).lab[(PCB).rx++];
  (PCB).fx = ag_k;
  (PCB).rx = 0;
}


static void ag_prot(void) {
  int ag_k = 128 - ++(PCB).btsx;
  if (ag_k <= (PCB).ssx) {
    (PCB).exit_flag = AG_STACK_ERROR_CODE;
    PARSER_STACK_OVERFLOW;
    return;
  }
  (PCB).bts[(PCB).btsx] = (PCB).sn;
  (PCB).bts[ag_k] = (PCB).ssx;
  (PCB).vs[ag_k] = (PCB).vs[(PCB).ssx];
  (PCB).ss[ag_k] = (PCB).ss[(PCB).ssx];
}

static void ag_undo(void) {
  if ((PCB).drt == -1) return;
  while ((PCB).btsx) {
    int ag_k = 128 - (PCB).btsx;
    (PCB).sn = (PCB).bts[(PCB).btsx--];
    (PCB).ssx = (PCB).bts[ag_k];
    (PCB).vs[(PCB).ssx] = (PCB).vs[ag_k];
    (PCB).ss[(PCB).ssx] = (PCB).ss[ag_k];
  }
  (PCB).token_number = (a85parse_token_type) (PCB).drt;
  (PCB).ssx = (PCB).dssx;
  (PCB).sn = (PCB).dsn;
  (PCB).drt = -1;
}


static const unsigned short ag_tstt[] = {
489,487,486,485,484,483,482,474,471,470,469,468,467,466,465,464,463,462,461,
  460,459,458,457,456,455,454,453,452,451,450,449,448,447,446,445,444,443,
  442,441,440,439,438,437,436,435,434,433,432,431,430,429,428,427,426,425,
  424,423,422,421,420,419,418,417,416,415,414,413,412,411,410,409,408,407,
  406,405,404,403,402,400,399,398,397,396,395,394,393,392,391,390,389,388,
  387,386,385,384,383,382,381,379,329,328,327,326,324,322,321,320,319,318,
  317,316,315,314,313,312,311,310,309,308,307,304,303,302,301,300,299,296,
  294,291,288,287,286,285,85,35,1,0,2,282,283,284,
490,489,488,487,486,485,484,483,482,481,480,479,478,477,476,475,474,473,472,
  365,364,363,361,358,357,356,355,354,348,346,344,288,287,286,1,0,37,
35,1,0,2,282,
489,487,486,485,484,483,482,474,471,470,469,468,467,466,465,464,463,462,461,
  460,459,458,457,456,455,454,453,452,451,450,449,448,447,446,445,444,443,
  442,441,440,439,438,437,436,435,434,433,432,431,430,429,428,427,426,425,
  424,423,422,421,420,419,418,417,416,415,414,413,412,411,410,409,408,407,
  406,405,404,403,402,400,399,398,397,396,395,394,393,392,391,390,389,388,
  387,386,385,384,383,382,381,379,329,328,327,326,324,322,321,320,319,318,
  317,316,315,314,313,312,311,310,309,308,307,304,303,302,301,300,299,296,
  294,291,288,287,286,285,85,35,1,0,22,23,24,25,39,40,
490,489,488,487,486,485,484,483,482,481,480,479,478,477,476,475,474,473,472,
  365,364,363,361,358,357,356,355,354,348,346,344,288,287,286,38,1,0,
1,0,
489,487,486,485,484,483,482,474,471,470,469,468,467,466,465,464,463,462,461,
  460,459,458,457,456,455,454,453,452,451,450,449,448,447,446,445,444,443,
  442,441,440,439,438,437,436,435,434,433,432,431,430,429,428,427,426,425,
  424,423,422,421,420,419,418,417,416,415,414,413,412,411,410,409,408,407,
  406,405,404,403,402,400,399,398,397,396,395,394,393,392,391,390,389,388,
  387,386,385,384,383,382,381,379,329,328,327,326,324,322,321,320,319,318,
  317,316,315,314,313,312,311,310,309,308,307,304,303,302,301,300,299,296,
  294,291,288,287,286,285,85,35,0,2,4,12,27,28,32,33,34,41,43,44,45,48,49,
  50,51,52,53,57,84,289,298,
489,487,486,485,484,483,482,474,471,470,469,468,467,466,465,464,463,462,461,
  460,459,458,457,456,455,454,453,452,451,450,449,448,447,446,445,444,443,
  442,441,440,439,438,437,436,435,434,433,432,431,430,429,428,427,426,425,
  424,423,422,421,420,419,418,417,416,415,414,413,412,411,410,409,408,407,
  406,405,404,403,402,400,399,398,397,396,395,394,393,392,391,390,389,388,
  387,386,385,384,383,382,381,379,329,328,327,326,324,322,321,320,319,318,
  317,316,315,314,313,312,311,310,309,308,307,304,303,302,301,300,299,296,
  294,291,288,287,286,285,85,35,26,1,0,23,24,39,40,
490,489,488,487,486,485,484,483,482,481,480,479,478,477,476,475,474,473,472,
  372,371,370,369,368,367,366,365,364,363,362,361,358,357,356,355,354,348,
  346,344,288,287,286,168,85,35,1,0,2,282,283,284,
490,489,488,487,486,485,484,483,482,481,480,479,478,477,476,475,474,473,472,
  365,364,363,361,358,357,356,355,354,348,346,344,288,287,286,35,1,0,2,
  282,283,284,
490,489,488,487,486,485,484,483,482,481,480,479,478,477,476,475,474,473,472,
  365,364,363,361,358,357,356,355,354,348,346,344,288,287,286,35,1,0,2,
  282,283,284,
490,489,488,487,486,485,484,483,482,481,480,479,478,477,476,475,474,473,472,
  365,364,363,361,358,357,356,355,354,348,346,344,288,287,286,1,0,30,31,
  33,34,89,90,92,94,95,120,123,126,134,135,137,139,140,143,145,148,149,
  162,163,164,165,167,170,171,172,174,277,278,279,280,281,
490,489,488,487,486,485,484,483,482,481,480,479,478,477,476,475,474,473,472,
  365,364,363,361,358,357,356,355,354,348,346,344,288,287,286,1,0,30,31,
  33,34,89,90,92,94,95,120,123,126,134,135,137,139,140,143,145,148,149,
  162,163,164,165,167,170,171,172,174,277,278,279,280,281,
490,489,488,487,486,485,484,483,482,481,480,479,478,477,476,475,474,473,472,
  365,364,363,361,358,357,356,355,354,348,346,344,288,287,286,1,0,30,31,
  33,34,89,90,92,94,95,120,123,126,134,135,137,139,140,143,145,148,149,
  162,163,164,165,167,170,171,172,174,277,278,279,280,281,
489,487,486,485,484,483,482,479,478,477,474,97,35,1,0,2,87,282,283,284,
489,487,486,485,484,483,482,474,85,35,1,0,2,282,283,284,
489,487,486,485,484,483,482,474,85,35,1,0,2,282,283,284,
288,287,286,285,35,1,0,2,282,283,284,
288,287,286,285,35,1,0,2,282,283,284,
489,487,486,485,484,483,482,474,85,35,1,0,2,282,283,284,
489,487,486,485,484,483,482,474,85,35,1,0,2,282,283,284,
297,295,35,1,0,2,282,283,284,
480,473,35,1,0,2,282,283,284,
489,488,487,486,485,484,483,482,480,479,478,477,476,473,357,35,1,0,2,282,
  283,284,
471,470,469,468,467,466,465,464,463,462,461,460,459,458,457,456,455,454,453,
  452,451,450,449,448,447,446,445,444,443,442,441,440,439,438,437,436,435,
  434,433,432,431,430,429,428,427,426,425,424,423,422,421,420,419,418,417,
  416,415,414,413,412,411,410,409,408,407,406,405,404,403,402,400,399,398,
  397,396,395,394,393,392,391,390,389,388,387,386,385,384,383,382,381,379,
  329,328,327,326,324,322,321,320,319,318,317,316,315,314,313,312,311,310,
  309,308,307,306,305,288,287,286,285,35,1,0,2,282,283,284,
471,470,469,468,467,466,465,464,463,462,461,460,459,458,457,456,455,454,453,
  452,451,450,449,448,447,446,445,444,443,442,441,440,439,438,437,436,435,
  434,433,432,431,430,429,428,427,426,425,424,423,422,421,420,419,418,417,
  416,415,414,413,412,411,410,409,408,407,406,405,404,403,402,400,399,398,
  397,396,395,394,393,392,391,390,389,388,387,386,385,384,383,382,381,379,
  329,328,327,326,324,322,321,320,319,318,317,316,315,314,313,312,311,310,
  309,308,307,0,58,59,60,61,62,63,65,66,68,69,70,71,72,73,75,76,77,78,79,
  80,81,82,184,185,186,187,188,189,190,191,192,193,194,195,196,197,198,
  199,200,201,202,203,204,205,206,207,208,209,210,211,212,213,214,215,216,
  217,218,219,220,221,222,223,224,225,226,227,228,229,230,231,232,233,234,
  235,236,237,238,239,240,241,242,243,244,245,246,247,248,249,250,251,252,
  253,254,255,256,257,258,259,261,262,263,264,265,266,267,268,269,270,271,
  272,273,274,275,276,
313,312,311,306,305,0,54,56,62,63,65,
489,487,486,485,484,483,482,474,85,0,4,84,298,
489,487,486,485,484,483,482,474,85,0,4,84,298,
489,487,486,485,484,483,482,474,85,0,4,84,298,
489,487,486,485,484,483,482,474,85,0,4,84,298,
297,295,0,46,47,
480,473,0,11,13,14,15,290,292,
489,488,487,486,485,484,483,482,480,479,478,477,476,473,357,0,11,13,14,15,
  20,290,292,293,
471,470,469,468,467,466,465,464,463,462,461,460,459,458,457,456,455,454,453,
  452,451,450,449,448,447,446,445,444,443,442,441,440,439,438,437,436,435,
  434,433,432,431,430,429,428,427,426,425,424,423,422,421,420,419,418,417,
  416,415,414,413,412,411,410,409,408,407,406,405,404,403,402,400,399,398,
  397,396,395,394,393,392,391,390,389,388,387,386,385,384,383,382,381,379,
  329,328,327,326,324,322,321,320,319,318,317,316,315,314,313,312,311,310,
  309,308,307,306,305,288,287,286,285,0,54,56,
288,287,286,285,0,27,28,33,34,42,
490,489,488,487,486,485,484,483,482,481,480,479,478,477,476,475,474,473,472,
  365,364,363,361,358,357,356,355,354,348,346,344,288,287,286,35,1,0,2,
  282,283,284,
490,489,488,487,486,485,484,483,482,481,480,479,478,477,476,475,474,473,472,
  372,371,370,369,368,367,366,365,364,363,361,358,357,356,355,354,348,346,
  344,288,287,286,168,85,35,1,0,2,282,283,284,
490,489,488,487,486,485,484,483,482,481,480,479,478,477,476,475,474,473,472,
  372,371,370,369,368,367,366,365,364,363,362,361,358,357,356,355,354,348,
  346,344,288,287,286,168,85,35,1,0,2,282,283,284,
490,489,488,487,486,485,484,483,482,481,480,479,478,477,476,475,474,473,472,
  365,364,363,361,358,357,356,355,354,348,346,344,288,287,286,35,1,0,2,
  282,283,284,
490,489,488,487,486,485,484,483,482,481,480,479,478,477,476,475,474,473,472,
  372,371,370,369,368,367,366,365,364,363,362,361,358,357,356,355,354,348,
  346,344,288,287,286,168,85,35,1,0,2,282,283,284,
490,489,488,487,486,485,484,483,482,481,480,479,478,477,476,475,474,473,472,
  365,364,363,361,358,357,356,355,354,348,346,344,288,287,286,35,1,0,2,
  282,283,284,
490,489,488,487,486,485,484,483,482,481,480,479,478,477,476,475,474,473,472,
  365,364,363,361,358,357,356,355,354,348,346,344,288,287,286,35,1,0,2,
  282,283,284,
490,489,488,487,486,485,484,483,482,481,480,479,478,477,476,475,474,473,472,
  365,364,363,361,358,357,356,355,354,348,346,344,288,287,286,35,1,0,2,
  282,283,284,
490,489,488,487,486,485,484,483,482,481,480,479,478,477,476,475,474,473,472,
  365,364,363,361,358,357,356,355,354,348,346,344,288,287,286,35,1,0,2,
  282,283,284,
490,489,488,487,486,485,484,483,482,481,480,479,478,477,476,475,474,473,472,
  365,364,363,361,358,357,356,355,354,348,346,344,288,287,286,35,1,0,2,
  282,283,284,
490,489,488,487,486,485,484,483,482,481,480,479,478,477,476,475,474,473,472,
  365,364,363,361,358,357,356,355,354,348,346,344,288,287,286,35,1,0,2,
  282,283,284,
490,489,488,487,486,485,484,483,482,481,480,479,478,477,476,475,474,473,472,
  365,364,363,361,358,357,356,355,354,348,346,344,288,287,286,35,1,0,2,
  282,283,284,
490,489,488,487,486,485,484,483,482,481,480,479,478,477,476,475,474,473,472,
  365,364,363,361,358,357,356,355,354,348,346,344,288,287,286,35,1,0,2,
  282,283,284,
490,489,488,487,486,485,484,483,482,481,480,479,478,477,476,475,474,473,472,
  365,364,363,361,358,357,356,355,354,348,346,344,288,287,286,35,1,0,2,
  282,283,284,
490,489,488,487,486,485,484,483,482,481,480,479,478,477,476,475,474,473,472,
  365,364,363,361,358,357,356,355,354,348,346,344,288,287,286,35,1,0,2,
  282,283,284,
490,489,488,487,486,485,484,483,482,481,480,479,478,477,476,475,474,473,472,
  471,470,469,468,467,466,465,464,463,462,461,460,459,458,457,456,455,454,
  453,452,451,450,449,448,447,446,445,444,443,442,441,440,439,438,437,436,
  435,434,433,432,431,430,429,428,427,426,425,424,423,422,421,420,419,418,
  417,416,415,414,413,412,411,410,409,408,407,406,405,404,403,402,400,399,
  398,397,396,395,394,393,392,391,390,389,388,387,386,385,384,383,382,381,
  379,365,364,363,361,358,357,356,355,354,348,346,344,288,287,286,285,35,
  1,0,2,282,283,284,
490,489,488,487,486,485,484,483,482,481,480,479,478,477,476,475,474,473,472,
  471,470,469,468,467,466,465,464,463,462,461,460,459,458,457,456,455,454,
  453,452,451,450,449,448,447,446,445,444,443,442,441,440,439,438,437,436,
  435,434,433,432,431,430,429,428,427,426,425,424,423,422,421,420,419,418,
  417,416,415,414,413,412,411,410,409,408,407,406,405,404,403,402,400,399,
  398,397,396,395,394,393,392,391,390,389,388,387,386,385,384,383,382,381,
  379,365,364,363,361,358,357,356,355,354,348,346,344,288,287,286,285,35,
  1,0,2,282,283,284,
490,489,488,487,486,485,484,483,482,481,480,479,478,477,476,475,474,473,472,
  372,371,370,369,368,367,366,365,364,363,361,358,357,356,355,354,348,346,
  344,288,287,286,168,85,35,1,0,2,282,283,284,
490,489,488,487,486,485,484,483,482,481,480,479,478,477,476,475,474,473,472,
  365,364,363,361,358,357,356,355,354,348,346,344,288,287,286,35,1,0,2,
  282,283,284,
490,489,488,487,486,485,484,483,482,481,480,479,478,477,476,475,474,473,472,
  372,371,370,369,368,367,366,365,364,363,362,361,358,357,356,355,354,348,
  346,344,288,287,286,168,85,35,1,0,2,282,283,284,
490,489,488,487,486,485,484,483,482,481,480,479,478,477,476,475,474,473,472,
  372,371,370,369,368,367,366,365,364,363,362,361,358,357,356,355,354,348,
  346,344,288,287,286,168,85,35,1,0,2,282,283,284,
490,489,488,487,486,485,484,483,482,481,480,479,478,477,476,475,474,473,472,
  372,371,370,369,368,367,366,365,364,363,361,358,357,356,355,354,348,346,
  344,288,287,286,168,85,35,1,0,2,282,283,284,
490,489,488,487,486,485,484,483,482,481,480,479,478,477,476,475,474,473,472,
  471,470,469,468,467,466,465,464,463,462,461,460,459,458,457,456,455,454,
  453,452,451,450,449,448,447,446,445,444,443,442,441,440,439,438,437,436,
  435,434,433,432,431,430,429,428,427,426,425,424,423,422,421,420,419,418,
  417,416,415,414,413,412,411,410,409,408,407,406,405,404,403,402,400,399,
  398,397,396,395,394,393,392,391,390,389,388,387,386,385,384,383,382,381,
  379,365,364,363,361,360,359,358,357,356,355,354,353,352,351,350,349,348,
  347,346,345,344,343,342,341,340,339,338,337,336,335,334,333,332,331,330,
  288,287,286,285,35,1,0,2,282,283,284,
490,489,488,487,486,485,484,483,482,481,480,479,478,477,476,475,474,473,472,
  372,371,370,369,368,367,366,365,364,363,362,361,358,357,356,355,354,348,
  346,344,288,287,286,168,85,35,1,0,2,282,283,284,
490,489,488,487,486,485,484,483,482,481,480,479,478,477,476,475,474,473,472,
  365,364,363,361,358,357,356,355,354,348,346,344,288,287,286,35,1,0,2,
  282,283,284,
490,489,488,487,486,485,484,483,482,481,480,479,478,477,476,475,474,473,472,
  372,371,370,369,368,367,366,365,364,363,362,361,358,357,356,355,354,348,
  346,344,288,287,286,168,85,35,1,0,2,282,283,284,
490,489,488,487,486,485,484,483,482,481,480,479,478,477,476,475,474,473,472,
  372,371,370,369,368,367,366,365,364,363,361,358,357,356,355,354,348,346,
  344,288,287,286,168,85,35,1,0,2,282,283,284,
490,489,488,487,486,485,484,483,482,481,480,479,478,477,476,475,474,473,472,
  365,364,363,361,358,357,356,355,354,348,346,344,288,287,286,35,1,0,2,
  282,283,284,
490,489,488,487,486,485,484,483,482,481,480,479,478,477,476,475,474,473,472,
  365,364,363,361,358,357,356,355,354,348,346,344,288,287,286,35,1,0,2,
  282,283,284,
490,489,488,487,486,485,484,483,482,481,480,479,478,477,476,475,474,473,472,
  471,470,469,468,467,466,465,464,463,462,461,460,459,458,457,456,455,454,
  453,452,451,450,449,448,447,446,445,444,443,442,441,440,439,438,437,436,
  435,434,433,432,431,430,429,428,427,426,425,424,423,422,421,420,419,418,
  417,416,415,414,413,412,411,410,409,408,407,406,405,404,403,402,400,399,
  398,397,396,395,394,393,392,391,390,389,388,387,386,385,384,383,382,381,
  379,372,371,370,369,368,367,366,365,364,363,361,358,357,356,355,354,348,
  346,344,288,287,286,285,168,85,35,1,0,2,282,283,284,
490,489,488,487,486,485,484,483,482,481,480,479,478,477,476,475,474,473,472,
  365,364,363,361,358,357,356,355,354,348,346,344,288,287,286,35,1,0,2,
  282,283,284,
490,489,488,487,486,485,484,483,482,481,480,479,478,477,476,475,474,473,472,
  365,364,363,361,358,357,356,355,354,348,346,344,287,286,1,0,33,34,89,90,
  92,94,95,120,123,126,134,135,137,139,140,143,145,148,149,162,163,164,
  165,167,170,171,172,174,277,278,279,280,281,
288,0,32,
288,0,32,
288,0,32,
471,470,469,468,467,466,465,464,463,462,461,460,459,458,457,456,455,454,453,
  452,451,450,449,448,447,446,445,444,443,442,441,440,439,438,437,436,435,
  434,433,432,431,430,429,428,427,426,425,424,423,422,421,420,419,418,417,
  416,415,414,413,412,411,410,409,408,407,406,405,404,403,402,400,399,398,
  397,396,395,394,393,392,391,390,389,388,387,386,385,384,383,382,381,379,
  361,288,287,286,285,35,1,0,2,282,283,284,
489,487,486,485,484,483,482,479,478,477,476,475,474,372,371,370,369,368,367,
  366,365,363,362,361,355,354,168,85,35,1,0,2,282,283,284,
378,376,375,374,178,177,176,175,35,1,0,2,282,283,284,
471,470,469,468,467,466,465,464,463,462,461,460,459,458,457,456,455,454,453,
  452,451,450,449,448,447,446,445,444,443,442,441,440,439,438,437,436,435,
  434,433,432,431,430,429,428,427,426,425,424,423,422,421,420,419,418,417,
  416,415,414,413,412,411,410,409,408,407,406,405,404,403,402,400,399,398,
  397,396,395,394,393,392,391,390,389,388,387,386,385,384,383,382,381,379,
  361,288,287,286,285,35,1,0,2,282,283,284,
489,487,486,485,484,483,482,479,478,477,476,475,474,372,371,370,369,368,367,
  366,365,363,362,361,355,354,168,85,35,1,0,2,282,283,284,
378,376,375,374,178,177,176,175,35,1,0,2,282,283,284,
471,470,469,468,467,466,465,464,463,462,461,460,459,458,457,456,455,454,453,
  452,451,450,449,448,447,446,445,444,443,442,441,440,439,438,437,436,435,
  434,433,432,431,430,429,428,427,426,425,424,423,422,421,420,419,418,417,
  416,415,414,413,412,411,410,409,408,407,406,405,404,403,402,400,399,398,
  397,396,395,394,393,392,391,390,389,388,387,386,385,384,383,382,381,379,
  361,288,287,286,285,35,1,0,2,282,283,284,
375,374,35,1,0,2,282,283,284,
489,487,486,485,484,483,482,479,478,477,476,475,474,372,371,370,369,368,367,
  366,365,363,362,361,355,354,168,85,35,1,0,2,282,283,284,
471,470,469,468,467,466,465,464,463,462,461,460,459,458,457,456,455,454,453,
  452,451,450,449,448,447,446,445,444,443,442,441,440,439,438,437,436,435,
  434,433,432,431,430,429,428,427,426,425,424,423,422,421,420,419,418,417,
  416,415,414,413,412,411,410,409,408,407,406,405,404,403,402,400,399,398,
  397,396,395,394,393,392,391,390,389,388,387,386,385,384,383,382,381,379,
  361,288,287,286,285,35,1,0,2,282,283,284,
471,470,469,468,467,466,465,464,463,462,461,460,459,458,457,456,455,454,453,
  452,451,450,449,448,447,446,445,444,443,442,441,440,439,438,437,436,435,
  434,433,432,431,430,429,428,427,426,425,424,423,422,421,420,419,418,417,
  416,415,414,413,412,411,410,409,408,407,406,405,404,403,402,400,399,398,
  397,396,395,394,393,392,391,390,389,388,387,386,385,384,383,382,381,379,
  361,288,287,286,285,35,1,0,2,282,283,284,
471,470,469,468,467,466,465,464,463,462,461,460,459,458,457,456,455,454,453,
  452,451,450,449,448,447,446,445,444,443,442,441,440,439,438,437,436,435,
  434,433,432,431,430,429,428,427,426,425,424,423,422,421,420,419,418,417,
  416,415,414,413,412,411,410,409,408,407,406,405,404,403,402,400,399,398,
  397,396,395,394,393,392,391,390,389,388,387,386,385,384,383,382,381,379,
  361,288,287,286,285,35,1,0,2,282,283,284,
489,487,486,485,484,483,482,479,478,477,476,475,474,372,371,370,369,368,367,
  366,365,363,362,361,355,354,168,85,35,1,0,2,282,283,284,
489,487,486,485,484,483,482,479,478,477,476,475,474,372,371,370,369,368,367,
  366,365,363,362,361,355,354,168,85,35,1,0,2,282,283,284,
378,376,375,374,178,177,176,175,35,1,0,2,282,283,284,
471,470,469,468,467,466,465,464,463,462,461,460,459,458,457,456,455,454,453,
  452,451,450,449,448,447,446,445,444,443,442,441,440,439,438,437,436,435,
  434,433,432,431,430,429,428,427,426,425,424,423,422,421,420,419,418,417,
  416,415,414,413,412,411,410,409,408,407,406,405,404,403,402,400,399,398,
  397,396,395,394,393,392,391,390,389,388,387,386,385,384,383,382,381,379,
  361,288,287,286,285,35,1,0,2,282,283,284,
478,477,35,1,0,2,282,283,284,
471,470,469,468,467,466,465,464,463,462,461,460,459,458,457,456,455,454,453,
  452,451,450,449,448,447,446,445,444,443,442,441,440,439,438,437,436,435,
  434,433,432,431,430,429,428,427,426,425,424,423,422,421,420,419,418,417,
  416,415,414,413,412,411,410,409,408,407,406,405,404,403,402,400,399,398,
  397,396,395,394,393,392,391,390,389,388,387,386,385,384,383,382,381,379,
  361,288,287,286,285,35,1,0,2,282,283,284,
471,470,469,468,467,466,465,464,463,462,461,460,459,458,457,456,455,454,453,
  452,451,450,449,448,447,446,445,444,443,442,441,440,439,438,437,436,435,
  434,433,432,431,430,429,428,427,426,425,424,423,422,421,420,419,418,417,
  416,415,414,413,412,411,410,409,408,407,406,405,404,403,402,400,399,398,
  397,396,395,394,393,392,391,390,389,388,387,386,385,384,383,382,381,379,
  361,288,287,286,285,35,1,0,2,282,283,284,
471,470,469,468,467,466,465,464,463,462,461,460,459,458,457,456,455,454,453,
  452,451,450,449,448,447,446,445,444,443,442,441,440,439,438,437,436,435,
  434,433,432,431,430,429,428,427,426,425,424,423,422,421,420,419,418,417,
  416,415,414,413,412,411,410,409,408,407,406,405,404,403,402,400,399,398,
  397,396,395,394,393,392,391,390,389,388,387,386,385,384,383,382,381,379,
  361,288,287,286,285,35,1,0,2,282,283,284,
471,470,469,468,467,466,465,464,463,462,461,460,459,458,457,456,455,454,453,
  452,451,450,449,448,447,446,445,444,443,442,441,440,439,438,437,436,435,
  434,433,432,431,430,429,428,427,426,425,424,423,422,421,420,419,418,417,
  416,415,414,413,412,411,410,409,408,407,406,405,404,403,402,400,399,398,
  397,396,395,394,393,392,391,390,389,388,387,386,385,384,383,382,381,379,
  361,288,287,286,285,35,1,0,2,282,283,284,
471,470,469,468,467,466,465,464,463,462,461,460,459,458,457,456,455,454,453,
  452,451,450,449,448,447,446,445,444,443,442,441,440,439,438,437,436,435,
  434,433,432,431,430,429,428,427,426,425,424,423,422,421,420,419,418,417,
  416,415,414,413,412,411,410,409,408,407,406,405,404,403,402,400,399,398,
  397,396,395,394,393,392,391,390,389,388,387,386,385,384,383,382,381,379,
  361,288,287,286,285,35,1,0,2,282,283,284,
471,470,469,468,467,466,465,464,463,462,461,460,459,458,457,456,455,454,453,
  452,451,450,449,448,447,446,445,444,443,442,441,440,439,438,437,436,435,
  434,433,432,431,430,429,428,427,426,425,424,423,422,421,420,419,418,417,
  416,415,414,413,412,411,410,409,408,407,406,405,404,403,402,400,399,398,
  397,396,395,394,393,392,391,390,389,388,387,386,385,384,383,382,381,379,
  361,288,287,286,285,35,1,0,2,282,283,284,
471,470,469,468,467,466,465,464,463,462,461,460,459,458,457,456,455,454,453,
  452,451,450,449,448,447,446,445,444,443,442,441,440,439,438,437,436,435,
  434,433,432,431,430,429,428,427,426,425,424,423,422,421,420,419,418,417,
  416,415,414,413,412,411,410,409,408,407,406,405,404,403,402,400,399,398,
  397,396,395,394,393,392,391,390,389,388,387,386,385,384,383,382,381,379,
  361,288,287,286,285,35,1,0,2,282,283,284,
471,470,469,468,467,466,465,464,463,462,461,460,459,458,457,456,455,454,453,
  452,451,450,449,448,447,446,445,444,443,442,441,440,439,438,437,436,435,
  434,433,432,431,430,429,428,427,426,425,424,423,422,421,420,419,418,417,
  416,415,414,413,412,411,410,409,408,407,406,405,404,403,402,400,399,398,
  397,396,395,394,393,392,391,390,389,388,387,386,385,384,383,382,381,379,
  361,288,287,286,285,35,1,0,2,282,283,284,
471,470,469,468,467,466,465,464,463,462,461,460,459,458,457,456,455,454,453,
  452,451,450,449,448,447,446,445,444,443,442,441,440,439,438,437,436,435,
  434,433,432,431,430,429,428,427,426,425,424,423,422,421,420,419,418,417,
  416,415,414,413,412,411,410,409,408,407,406,405,404,403,402,400,399,398,
  397,396,395,394,393,392,391,390,389,388,387,386,385,384,383,382,381,379,
  361,288,287,286,285,35,1,0,2,282,283,284,
471,470,469,468,467,466,465,464,463,462,461,460,459,458,457,456,455,454,453,
  452,451,450,449,448,447,446,445,444,443,442,441,440,439,438,437,436,435,
  434,433,432,431,430,429,428,427,426,425,424,423,422,421,420,419,418,417,
  416,415,414,413,412,411,410,409,408,407,406,405,404,403,402,400,399,398,
  397,396,395,394,393,392,391,390,389,388,387,386,385,384,383,382,381,379,
  361,288,287,286,285,35,1,0,2,282,283,284,
471,470,469,468,467,466,465,464,463,462,461,460,459,458,457,456,455,454,453,
  452,451,450,449,448,447,446,445,444,443,442,441,440,439,438,437,436,435,
  434,433,432,431,430,429,428,427,426,425,424,423,422,421,420,419,418,417,
  416,415,414,413,412,411,410,409,408,407,406,405,404,403,402,400,399,398,
  397,396,395,394,393,392,391,390,389,388,387,386,385,384,383,382,381,379,
  361,288,287,286,285,35,1,0,2,282,283,284,
471,470,469,468,467,466,465,464,463,462,461,460,459,458,457,456,455,454,453,
  452,451,450,449,448,447,446,445,444,443,442,441,440,439,438,437,436,435,
  434,433,432,431,430,429,428,427,426,425,424,423,422,421,420,419,418,417,
  416,415,414,413,412,411,410,409,408,407,406,405,404,403,402,400,399,398,
  397,396,395,394,393,392,391,390,389,388,387,386,385,384,383,382,381,379,
  361,288,287,286,285,35,1,0,2,282,283,284,
471,470,469,468,467,466,465,464,463,462,461,460,459,458,457,456,455,454,453,
  452,451,450,449,448,447,446,445,444,443,442,441,440,439,438,437,436,435,
  434,433,432,431,430,429,428,427,426,425,424,423,422,421,420,419,418,417,
  416,415,414,413,412,411,410,409,408,407,406,405,404,403,402,400,399,398,
  397,396,395,394,393,392,391,390,389,388,387,386,385,384,383,382,381,379,
  361,288,287,286,285,35,1,0,2,282,283,284,
471,470,469,468,467,466,465,464,463,462,461,460,459,458,457,456,455,454,453,
  452,451,450,449,448,447,446,445,444,443,442,441,440,439,438,437,436,435,
  434,433,432,431,430,429,428,427,426,425,424,423,422,421,420,419,418,417,
  416,415,414,413,412,411,410,409,408,407,406,405,404,403,402,400,399,398,
  397,396,395,394,393,392,391,390,389,388,387,386,385,384,383,382,381,379,
  361,288,287,286,285,35,1,0,2,282,283,284,
378,377,376,375,374,35,1,0,2,282,283,284,
378,377,376,375,374,35,1,0,2,282,283,284,
471,470,469,468,467,466,465,464,463,462,461,460,459,458,457,456,455,454,453,
  452,451,450,449,448,447,446,445,444,443,442,441,440,439,438,437,436,435,
  434,433,432,431,430,429,428,427,426,425,424,423,422,421,420,419,418,417,
  416,415,414,413,412,411,410,409,408,407,406,405,404,403,402,400,399,398,
  397,396,395,394,393,392,391,390,389,388,387,386,385,384,383,382,381,379,
  361,288,287,286,285,35,1,0,2,282,283,284,
489,487,486,485,484,483,482,479,478,477,476,475,474,372,371,370,369,368,367,
  366,365,363,362,361,355,354,168,85,35,1,0,2,282,283,284,
489,487,486,485,484,483,482,479,478,477,476,475,474,372,371,370,369,368,367,
  366,365,363,362,361,355,354,168,85,35,1,0,2,282,283,284,
378,376,375,374,178,177,176,175,35,1,0,2,282,283,284,
471,470,469,468,467,466,465,464,463,462,461,460,459,458,457,456,455,454,453,
  452,451,450,449,448,447,446,445,444,443,442,441,440,439,438,437,436,435,
  434,433,432,431,430,429,428,427,426,425,424,423,422,421,420,419,418,417,
  416,415,414,413,412,411,410,409,408,407,406,405,404,403,402,400,399,398,
  397,396,395,394,393,392,391,390,389,388,387,386,385,384,383,382,381,379,
  361,288,287,286,285,35,1,0,2,282,283,284,
378,376,375,374,178,177,176,175,35,1,0,2,282,283,284,
378,376,375,374,178,177,176,175,35,1,0,2,282,283,284,
376,375,374,180,35,1,0,2,282,283,284,
471,470,469,468,467,466,465,464,463,462,461,460,459,458,457,456,455,454,453,
  452,451,450,449,448,447,446,445,444,443,442,441,440,439,438,437,436,435,
  434,433,432,431,430,429,428,427,426,425,424,423,422,421,420,419,418,417,
  416,415,414,413,412,411,410,409,408,407,406,405,404,403,402,400,399,398,
  397,396,395,394,393,392,391,390,389,388,387,386,385,384,383,382,381,379,
  361,288,287,286,285,35,1,0,2,282,283,284,
489,487,486,485,484,483,482,479,478,477,476,475,474,372,371,370,369,368,367,
  366,365,363,362,361,355,354,168,85,35,1,0,2,282,283,284,
489,487,486,485,484,483,482,479,478,477,476,475,474,372,371,370,369,368,367,
  366,365,363,362,361,355,354,168,85,35,1,0,2,282,283,284,
489,487,486,485,484,483,482,479,478,477,476,475,474,372,371,370,369,368,367,
  366,365,363,362,361,355,354,168,85,35,1,0,2,282,283,284,
375,374,35,1,0,2,282,283,284,
489,487,486,485,484,483,482,479,478,477,476,475,474,372,371,370,369,368,367,
  366,365,363,362,361,355,354,168,85,35,1,0,2,282,283,284,
489,487,486,485,484,483,482,479,478,477,476,475,474,372,371,370,369,368,367,
  366,365,363,362,361,355,354,168,85,35,1,0,2,282,283,284,
489,487,486,485,484,483,482,479,478,477,476,475,474,372,371,370,369,368,367,
  366,365,363,362,361,355,354,168,85,35,1,0,2,282,283,284,
489,487,486,485,484,483,482,479,478,477,476,475,474,372,371,370,369,368,367,
  366,365,363,362,361,355,354,168,85,35,1,0,2,282,283,284,
489,487,486,485,484,483,482,479,478,477,476,475,474,372,371,370,369,368,367,
  366,365,363,362,361,355,354,168,85,35,1,0,2,282,283,284,
489,487,486,485,484,483,482,479,478,477,476,475,474,372,371,370,369,368,367,
  366,365,363,362,361,355,354,168,85,35,1,0,2,282,283,284,
489,487,486,485,484,483,482,479,478,477,476,475,474,372,371,370,369,368,367,
  366,365,363,362,361,355,354,168,85,35,1,0,2,282,283,284,
489,487,486,485,484,483,482,479,478,477,476,475,474,372,371,370,369,368,367,
  366,365,363,362,361,355,354,168,85,35,1,0,2,282,283,284,
489,487,486,485,484,483,482,479,478,477,476,475,474,372,371,370,369,368,367,
  366,365,363,362,361,355,354,168,85,35,1,0,2,282,283,284,
489,487,486,485,484,483,482,479,478,477,476,475,474,372,371,370,369,368,367,
  366,365,363,362,361,355,354,168,85,35,1,0,2,282,283,284,
489,487,486,485,484,483,482,479,478,477,476,475,474,372,371,370,369,368,367,
  366,365,363,362,361,355,354,168,85,35,1,0,2,282,283,284,
489,487,486,485,484,483,482,479,478,477,476,475,474,372,371,370,369,368,367,
  366,365,363,362,361,355,354,168,85,35,1,0,2,282,283,284,
489,487,486,485,484,483,482,479,478,477,476,475,474,372,371,370,369,368,367,
  366,365,363,362,361,355,354,168,85,35,1,0,2,282,283,284,
489,487,486,485,484,483,482,479,478,477,476,475,474,372,371,370,369,368,367,
  366,365,363,362,361,355,354,168,85,35,1,0,2,282,283,284,
376,375,374,180,35,1,0,2,282,283,284,
378,376,375,374,178,177,176,175,35,1,0,2,282,283,284,
489,487,486,485,484,483,482,479,478,477,476,475,474,372,371,370,369,368,367,
  366,365,363,362,361,355,354,168,85,35,1,0,2,282,283,284,
471,470,469,468,467,466,465,464,463,462,461,460,459,458,457,456,455,454,453,
  452,451,450,449,448,447,446,445,444,443,442,441,440,439,438,437,436,435,
  434,433,432,431,430,429,428,427,426,425,424,423,422,421,420,419,418,417,
  416,415,414,413,412,411,410,409,408,407,406,405,404,403,402,400,399,398,
  397,396,395,394,393,392,391,390,389,388,387,386,385,384,383,382,381,379,
  361,288,287,286,285,35,1,0,2,282,283,284,
471,470,469,468,467,466,465,464,463,462,461,460,459,458,457,456,455,454,453,
  452,451,450,449,448,447,446,445,444,443,442,441,440,439,438,437,436,435,
  434,433,432,431,430,429,428,427,426,425,424,423,422,421,420,419,418,417,
  416,415,414,413,412,411,410,409,408,407,406,405,404,403,402,400,399,398,
  397,396,395,394,393,392,391,390,389,388,387,386,385,384,383,382,381,379,
  361,288,287,286,285,35,1,0,2,282,283,284,
471,470,469,468,467,466,465,464,463,462,461,460,459,458,457,456,455,454,453,
  452,451,450,449,448,447,446,445,444,443,442,441,440,439,438,437,436,435,
  434,433,432,431,430,429,428,427,426,425,424,423,422,421,420,419,418,417,
  416,415,414,413,412,411,410,409,408,407,406,405,404,403,402,400,399,398,
  397,396,395,394,393,392,391,390,389,388,387,386,385,384,383,382,381,379,
  361,288,287,286,285,35,1,0,2,282,283,284,
471,470,469,468,467,466,465,464,463,462,461,460,459,458,457,456,455,454,453,
  452,451,450,449,448,447,446,445,444,443,442,441,440,439,438,437,436,435,
  434,433,432,431,430,429,428,427,426,425,424,423,422,421,420,419,418,417,
  416,415,414,413,412,411,410,409,408,407,406,405,404,403,402,400,399,398,
  397,396,395,394,393,392,391,390,389,388,387,386,385,384,383,382,381,379,
  361,288,287,286,285,35,1,0,2,282,283,284,
376,375,374,180,35,1,0,2,282,283,284,
378,376,375,374,178,177,176,175,35,1,0,2,282,283,284,
376,375,374,180,35,1,0,2,282,283,284,
471,470,469,468,467,466,465,464,463,462,461,460,459,458,457,456,455,454,453,
  452,451,450,449,448,447,446,445,444,443,442,441,440,439,438,437,436,435,
  434,433,432,431,430,429,428,427,426,425,424,423,422,421,420,419,418,417,
  416,415,414,413,412,411,410,409,408,407,406,405,404,403,402,400,399,398,
  397,396,395,394,393,392,391,390,389,388,387,386,385,384,383,382,381,379,
  361,288,287,286,285,35,1,0,2,282,283,284,
489,487,486,485,484,483,482,479,478,477,476,475,474,372,371,370,369,368,367,
  366,365,363,362,361,355,354,168,85,35,1,0,2,282,283,284,
489,487,486,485,484,483,482,479,478,477,476,475,474,372,371,370,369,368,367,
  366,365,363,362,361,355,354,168,85,35,1,0,2,282,283,284,
489,487,486,485,484,483,482,479,478,477,476,475,474,372,371,370,369,368,367,
  366,365,363,362,361,355,354,168,85,35,1,0,2,282,283,284,
489,487,486,485,484,483,482,479,478,477,476,475,474,372,371,370,369,368,367,
  366,365,363,362,361,355,354,168,85,35,1,0,2,282,283,284,
489,487,486,485,484,483,482,479,478,477,476,475,474,372,371,370,369,368,367,
  366,365,363,362,361,355,354,168,85,35,1,0,2,282,283,284,
489,487,486,485,484,483,482,479,478,477,476,475,474,372,371,370,369,368,367,
  366,365,363,362,361,355,354,168,85,35,1,0,2,282,283,284,
489,487,486,485,484,483,482,479,478,477,476,475,474,372,371,370,369,368,367,
  366,365,363,362,361,355,354,168,85,35,1,0,2,282,283,284,
378,376,375,374,178,177,176,175,35,1,0,2,282,283,284,
471,470,469,468,467,466,465,464,463,462,461,460,459,458,457,456,455,454,453,
  452,451,450,449,448,447,446,445,444,443,442,441,440,439,438,437,436,435,
  434,433,432,431,430,429,428,427,426,425,424,423,422,421,420,419,418,417,
  416,415,414,413,412,411,410,409,408,407,406,405,404,403,402,400,399,398,
  397,396,395,394,393,392,391,390,389,388,387,386,385,384,383,382,381,379,
  361,288,287,286,285,35,1,0,2,282,283,284,
471,470,469,468,467,466,465,464,463,462,461,460,459,458,457,456,455,454,453,
  452,451,450,449,448,447,446,445,444,443,442,441,440,439,438,437,436,435,
  434,433,432,431,430,429,428,427,426,425,424,423,422,421,420,419,418,417,
  416,415,414,413,412,411,410,409,408,407,406,405,404,403,402,400,399,398,
  397,396,395,394,393,392,391,390,389,388,387,386,385,384,383,382,381,379,
  361,288,287,286,285,35,1,0,2,282,283,284,
489,487,486,485,484,483,482,479,478,477,476,475,474,372,371,370,369,368,367,
  366,365,363,362,361,355,354,168,85,35,1,0,2,282,283,284,
489,487,486,485,484,483,482,479,478,477,476,475,474,372,371,370,369,368,367,
  366,365,363,362,361,355,354,168,85,35,1,0,2,282,283,284,
489,487,486,485,484,483,482,479,478,477,476,475,474,372,371,370,369,368,367,
  366,365,363,362,361,355,354,168,85,35,1,0,2,282,283,284,
471,470,469,468,467,466,465,464,463,462,461,460,459,458,457,456,455,454,453,
  452,451,450,449,448,447,446,445,444,443,442,441,440,439,438,437,436,435,
  434,433,432,431,430,429,428,427,426,425,424,423,422,421,420,419,418,417,
  416,415,414,413,412,411,410,409,408,407,406,405,404,403,402,400,399,398,
  397,396,395,394,393,392,391,390,389,388,387,386,385,384,383,382,381,379,
  361,288,287,286,285,35,1,0,2,282,283,284,
489,487,486,485,484,483,482,479,478,477,476,475,474,372,371,370,369,368,367,
  366,365,363,362,361,355,354,168,85,35,1,0,2,282,283,284,
378,376,375,374,178,177,176,175,35,1,0,2,282,283,284,
489,487,486,485,484,483,482,479,478,477,476,475,474,372,371,370,369,368,367,
  366,365,363,362,361,355,354,168,85,35,1,0,2,282,283,284,
378,376,375,374,178,177,176,175,35,1,0,2,282,283,284,
378,376,375,374,178,177,176,175,35,1,0,2,282,283,284,
489,487,486,485,484,483,482,479,478,477,476,475,474,372,371,370,369,368,367,
  366,365,363,362,361,355,354,168,85,35,1,0,2,282,283,284,
489,487,486,485,484,483,482,479,478,477,476,475,474,372,371,370,369,368,367,
  366,365,363,362,361,355,354,168,85,1,0,39,40,
378,376,375,374,178,177,176,175,1,0,39,40,
489,487,486,485,484,483,482,479,478,477,476,475,474,372,371,370,369,368,367,
  366,365,363,362,361,355,354,168,85,1,0,39,40,
378,376,375,374,178,177,176,175,1,0,39,40,
375,374,1,0,39,40,
489,487,486,485,484,483,482,479,478,477,476,475,474,372,371,370,369,368,367,
  366,365,363,362,361,355,354,168,85,1,0,39,40,
489,487,486,485,484,483,482,479,478,477,476,475,474,372,371,370,369,368,367,
  366,365,363,362,361,355,354,168,85,1,0,39,40,
489,487,486,485,484,483,482,479,478,477,476,475,474,372,371,370,369,368,367,
  366,365,363,362,361,355,354,168,85,1,0,39,40,
378,376,375,374,178,177,176,175,1,0,39,40,
478,477,1,0,39,40,
378,377,376,375,374,1,0,39,40,
378,377,376,375,374,1,0,39,40,
489,487,486,485,484,483,482,479,478,477,476,475,474,372,371,370,369,368,367,
  366,365,363,362,361,355,354,168,85,1,0,39,40,
489,487,486,485,484,483,482,479,478,477,476,475,474,372,371,370,369,368,367,
  366,365,363,362,361,355,354,168,85,1,0,39,40,
378,376,375,374,178,177,176,175,1,0,39,40,
378,376,375,374,178,177,176,175,1,0,39,40,
378,376,375,374,178,177,176,175,1,0,39,40,
376,375,374,180,1,0,39,40,
489,487,486,485,484,483,482,479,478,477,476,475,474,372,371,370,369,368,367,
  366,365,363,362,361,355,354,168,85,1,0,39,40,
489,487,486,485,484,483,482,479,478,477,476,475,474,372,371,370,369,368,367,
  366,365,363,362,361,355,354,168,85,1,0,39,40,
489,487,486,485,484,483,482,479,478,477,476,475,474,372,371,370,369,368,367,
  366,365,363,362,361,355,354,168,85,1,0,39,40,
375,374,1,0,39,40,
489,487,486,485,484,483,482,479,478,477,476,475,474,372,371,370,369,368,367,
  366,365,363,362,361,355,354,168,85,1,0,39,40,
489,487,486,485,484,483,482,479,478,477,476,475,474,372,371,370,369,368,367,
  366,365,363,362,361,355,354,168,85,1,0,39,40,
489,487,486,485,484,483,482,479,478,477,476,475,474,372,371,370,369,368,367,
  366,365,363,362,361,355,354,168,85,1,0,39,40,
489,487,486,485,484,483,482,479,478,477,476,475,474,372,371,370,369,368,367,
  366,365,363,362,361,355,354,168,85,1,0,39,40,
489,487,486,485,484,483,482,479,478,477,476,475,474,372,371,370,369,368,367,
  366,365,363,362,361,355,354,168,85,1,0,39,40,
489,487,486,485,484,483,482,479,478,477,476,475,474,372,371,370,369,368,367,
  366,365,363,362,361,355,354,168,85,1,0,39,40,
489,487,486,485,484,483,482,479,478,477,476,475,474,372,371,370,369,368,367,
  366,365,363,362,361,355,354,168,85,1,0,39,40,
489,487,486,485,484,483,482,479,478,477,476,475,474,372,371,370,369,368,367,
  366,365,363,362,361,355,354,168,85,1,0,39,40,
489,487,486,485,484,483,482,479,478,477,476,475,474,372,371,370,369,368,367,
  366,365,363,362,361,355,354,168,85,1,0,39,40,
489,487,486,485,484,483,482,479,478,477,476,475,474,372,371,370,369,368,367,
  366,365,363,362,361,355,354,168,85,1,0,39,40,
489,487,486,485,484,483,482,479,478,477,476,475,474,372,371,370,369,368,367,
  366,365,363,362,361,355,354,168,85,1,0,39,40,
489,487,486,485,484,483,482,479,478,477,476,475,474,372,371,370,369,368,367,
  366,365,363,362,361,355,354,168,85,1,0,39,40,
489,487,486,485,484,483,482,479,478,477,476,475,474,372,371,370,369,368,367,
  366,365,363,362,361,355,354,168,85,1,0,39,40,
489,487,486,485,484,483,482,479,478,477,476,475,474,372,371,370,369,368,367,
  366,365,363,362,361,355,354,168,85,1,0,39,40,
376,375,374,180,1,0,39,40,
378,376,375,374,178,177,176,175,1,0,39,40,
489,487,486,485,484,483,482,479,478,477,476,475,474,372,371,370,369,368,367,
  366,365,363,362,361,355,354,168,85,1,0,39,40,
376,375,374,180,1,0,39,40,
378,376,375,374,178,177,176,175,1,0,39,40,
376,375,374,180,1,0,39,40,
489,487,486,485,484,483,482,479,478,477,476,475,474,372,371,370,369,368,367,
  366,365,363,362,361,355,354,168,85,1,0,39,40,
489,487,486,485,484,483,482,479,478,477,476,475,474,372,371,370,369,368,367,
  366,365,363,362,361,355,354,168,85,1,0,39,40,
489,487,486,485,484,483,482,479,478,477,476,475,474,372,371,370,369,368,367,
  366,365,363,362,361,355,354,168,85,1,0,39,40,
489,487,486,485,484,483,482,479,478,477,476,475,474,372,371,370,369,368,367,
  366,365,363,362,361,355,354,168,85,1,0,39,40,
489,487,486,485,484,483,482,479,478,477,476,475,474,372,371,370,369,368,367,
  366,365,363,362,361,355,354,168,85,1,0,39,40,
489,487,486,485,484,483,482,479,478,477,476,475,474,372,371,370,369,368,367,
  366,365,363,362,361,355,354,168,85,1,0,39,40,
489,487,486,485,484,483,482,479,478,477,476,475,474,372,371,370,369,368,367,
  366,365,363,362,361,355,354,168,85,1,0,39,40,
378,376,375,374,178,177,176,175,1,0,39,40,
489,487,486,485,484,483,482,479,478,477,476,475,474,372,371,370,369,368,367,
  366,365,363,362,361,355,354,168,85,1,0,39,40,
489,487,486,485,484,483,482,479,478,477,476,475,474,372,371,370,369,368,367,
  366,365,363,362,361,355,354,168,85,1,0,39,40,
489,487,486,485,484,483,482,479,478,477,476,475,474,372,371,370,369,368,367,
  366,365,363,362,361,355,354,168,85,1,0,39,40,
489,487,486,485,484,483,482,479,478,477,476,475,474,372,371,370,369,368,367,
  366,365,363,362,361,355,354,168,85,1,0,39,40,
378,376,375,374,178,177,176,175,1,0,39,40,
489,487,486,485,484,483,482,479,478,477,476,475,474,372,371,370,369,368,367,
  366,365,363,362,361,355,354,168,85,1,0,39,40,
378,376,375,374,178,177,176,175,1,0,39,40,
378,376,375,374,178,177,176,175,1,0,39,40,
489,487,486,485,484,483,482,479,478,477,476,475,474,372,371,370,369,368,367,
  366,365,363,362,361,355,354,168,85,1,0,39,40,
471,470,469,468,467,466,465,464,463,462,461,460,459,458,457,456,455,454,453,
  452,451,450,449,448,447,446,445,444,443,442,441,440,439,438,437,436,435,
  434,433,432,431,430,429,428,427,426,425,424,423,422,421,420,419,418,417,
  416,415,414,413,412,411,410,409,408,407,406,405,404,403,402,400,399,398,
  397,396,395,394,393,392,391,390,389,388,387,386,385,384,383,382,381,379,
  361,288,287,286,285,1,0,39,40,
489,487,486,485,484,483,482,474,85,35,1,0,2,282,283,284,
489,487,486,485,484,483,482,474,85,0,4,84,298,
489,488,487,486,485,484,483,482,479,478,477,476,357,35,1,0,2,282,283,284,
489,488,487,486,485,484,483,482,479,478,477,476,357,0,20,293,
288,287,286,285,35,1,0,2,282,283,284,
288,287,286,285,35,1,0,2,282,283,284,
1,0,39,
475,473,35,1,0,2,282,283,284,
475,473,0,11,13,19,21,290,323,
288,287,286,285,35,1,0,2,282,283,284,
288,287,286,285,35,1,0,2,282,283,284,
489,487,486,485,484,483,482,479,478,477,476,475,474,372,371,370,369,368,367,
  366,365,363,362,361,355,354,168,85,35,1,0,2,282,283,284,
489,487,486,485,484,483,482,479,478,477,476,475,474,372,371,370,369,368,367,
  366,365,363,362,361,355,354,168,85,0,3,4,7,8,9,10,16,18,55,74,84,98,118,
  119,122,125,128,133,136,138,143,144,145,146,147,148,150,151,152,153,154,
  155,156,298,325,373,
489,487,486,485,484,483,482,474,288,287,286,285,85,35,1,0,2,282,283,284,
489,487,486,485,484,483,482,474,85,0,4,84,298,
489,487,486,485,484,483,482,479,478,477,476,475,474,372,371,370,369,368,367,
  366,365,363,362,361,355,354,168,85,35,1,0,2,282,283,284,
489,487,486,485,484,483,482,479,478,477,476,475,474,372,371,370,369,368,367,
  366,365,363,362,361,355,354,168,85,0,3,4,7,8,9,10,16,18,55,84,118,119,
  122,125,128,133,136,138,143,144,145,146,147,148,150,151,152,153,154,155,
  156,298,325,373,
489,487,486,485,484,483,482,474,85,35,1,0,2,282,283,284,
489,487,486,485,484,483,482,474,85,0,4,84,298,
489,487,486,485,484,483,482,474,85,35,1,0,2,282,283,284,
489,487,486,485,484,483,482,474,85,0,4,67,84,298,
489,487,486,485,484,483,482,474,85,35,1,0,2,282,283,284,
489,487,486,485,484,483,482,474,85,0,4,67,84,298,
489,487,486,485,484,483,482,474,85,35,1,0,2,282,283,284,
489,487,486,485,484,483,482,474,85,0,4,67,84,298,
489,487,486,485,484,483,482,479,478,477,476,475,474,473,372,371,370,369,368,
  367,366,365,363,362,361,355,354,168,85,35,1,0,2,282,283,284,
489,487,486,485,484,483,482,479,478,477,476,475,474,473,372,371,370,369,368,
  367,366,365,363,362,361,355,354,168,85,0,3,4,7,8,9,10,11,13,16,18,19,21,
  55,64,84,118,119,122,125,128,133,136,138,143,144,145,146,147,148,150,
  151,152,153,154,155,156,290,298,323,325,373,
489,487,486,485,484,483,482,479,478,477,476,475,474,473,372,371,370,369,368,
  367,366,365,363,362,361,355,354,168,85,35,1,0,2,282,283,284,
489,487,486,485,484,483,482,479,478,477,476,475,474,473,372,371,370,369,368,
  367,366,365,363,362,361,355,354,168,85,0,3,4,7,8,9,10,11,13,16,18,19,21,
  55,64,84,118,119,122,125,128,133,136,138,143,144,145,146,147,148,150,
  151,152,153,154,155,156,290,298,323,325,373,
489,487,486,485,484,483,482,479,478,477,476,475,474,372,371,370,369,368,367,
  366,365,363,362,361,355,354,168,85,35,1,0,2,282,283,284,
489,487,486,485,484,483,482,479,478,477,476,475,474,372,371,370,369,368,367,
  366,365,363,362,361,355,354,168,85,0,3,4,7,8,9,10,16,18,55,84,118,119,
  122,125,128,133,136,138,143,144,145,146,147,148,150,151,152,153,154,155,
  156,298,325,373,
288,287,286,285,35,1,0,2,282,283,284,
288,287,286,285,35,1,0,2,282,283,284,
288,287,286,285,35,1,0,2,282,283,284,
489,487,486,485,484,483,482,479,478,477,476,475,474,372,371,370,369,368,367,
  366,365,363,362,361,355,354,168,85,35,1,0,2,282,283,284,
489,487,486,485,484,483,482,479,478,477,476,475,474,372,371,370,369,368,367,
  366,365,363,362,361,355,354,168,85,0,3,4,7,8,9,10,16,18,55,84,118,119,
  122,125,128,133,136,138,143,144,145,146,147,148,150,151,152,153,154,155,
  156,298,325,373,
489,487,486,485,484,483,482,479,478,477,476,475,474,473,372,371,370,369,368,
  367,366,365,363,362,361,355,354,168,85,0,3,4,7,8,9,10,11,13,16,18,19,21,
  55,64,84,118,119,122,125,128,133,136,138,143,144,145,146,147,148,150,
  151,152,153,154,155,156,290,298,323,325,373,
489,487,486,485,484,483,482,479,478,477,476,475,474,473,372,371,370,369,368,
  367,366,365,363,362,361,355,354,168,85,0,3,4,7,8,9,10,11,13,16,18,19,21,
  55,64,84,118,119,122,125,128,133,136,138,143,144,145,146,147,148,150,
  151,152,153,154,155,156,290,298,323,325,373,
489,487,486,485,484,483,482,479,478,477,476,475,474,372,371,370,369,368,367,
  366,365,363,362,361,355,354,168,85,0,3,4,7,8,9,10,16,18,55,84,118,119,
  122,125,128,133,136,138,143,144,145,146,147,148,150,151,152,153,154,155,
  156,298,325,373,
489,487,486,485,484,483,482,479,478,477,476,475,474,372,371,370,369,368,367,
  366,365,363,362,361,355,354,168,85,35,1,0,2,282,283,284,
489,487,486,485,484,483,482,479,478,477,476,475,474,372,371,370,369,368,367,
  366,365,363,362,361,355,354,168,85,0,3,4,7,8,9,10,16,18,55,84,118,119,
  122,125,128,133,136,138,143,144,145,146,147,148,150,151,152,153,154,155,
  156,298,325,373,
489,487,486,485,484,483,482,479,478,477,476,475,474,372,371,370,369,368,367,
  366,365,363,362,361,355,354,168,85,35,1,0,2,282,283,284,
489,487,486,485,484,483,482,479,478,477,476,475,474,372,371,370,369,368,367,
  366,365,363,362,361,355,354,168,85,0,3,4,7,8,9,10,16,18,55,84,118,119,
  122,125,128,133,136,138,143,144,145,146,147,148,150,151,152,153,154,155,
  156,298,325,373,
489,487,486,485,484,483,482,479,478,477,474,364,361,360,358,357,356,355,354,
  352,350,348,346,344,343,341,339,337,335,333,332,330,288,287,286,285,35,
  1,0,2,87,282,283,284,
288,287,286,285,35,1,0,2,282,283,284,
288,287,286,285,35,1,0,2,282,283,284,
490,489,488,487,486,485,484,483,482,481,480,479,478,477,476,475,474,472,365,
  364,363,361,358,357,356,355,354,348,346,344,287,286,1,0,
288,287,286,285,35,1,0,2,282,283,284,
490,489,488,487,486,485,484,483,482,481,480,479,478,477,476,475,474,473,472,
  365,364,363,361,358,357,356,355,354,348,346,344,287,286,1,0,
330,288,287,286,285,35,1,0,2,282,283,284,
489,488,487,486,485,484,483,482,479,478,477,476,357,288,287,286,35,1,0,2,
  282,283,284,
489,487,486,485,484,483,482,479,478,477,476,475,474,372,371,370,369,368,367,
  366,365,363,362,361,355,354,168,85,0,3,4,7,8,9,10,16,18,55,84,118,119,
  122,125,128,133,136,138,143,144,145,146,147,148,150,151,152,153,154,155,
  156,298,325,373,
489,487,486,485,484,483,482,479,478,477,476,475,474,372,371,370,369,368,367,
  366,365,363,362,361,355,354,168,85,0,3,4,7,8,9,10,16,18,55,84,118,119,
  122,125,128,133,136,138,143,144,145,146,147,148,150,151,152,153,154,155,
  156,298,325,373,
489,487,486,485,484,483,482,474,471,470,469,468,467,466,465,464,463,462,461,
  460,459,458,457,456,455,454,453,452,451,450,449,448,447,446,445,444,443,
  442,441,440,439,438,437,436,435,434,433,432,431,430,429,428,427,426,425,
  424,423,422,421,420,419,418,417,416,415,414,413,412,411,410,409,408,407,
  406,405,404,403,402,400,399,398,397,396,395,394,393,392,391,390,389,388,
  387,386,385,384,383,382,381,379,329,328,327,326,324,322,321,320,319,318,
  317,316,315,314,313,312,311,310,309,308,307,304,303,302,301,300,299,296,
  294,291,288,287,286,285,85,35,26,1,0,2,282,283,284,
489,487,486,485,484,483,482,479,478,477,476,475,474,372,371,370,369,368,367,
  366,365,363,362,361,355,354,168,85,0,3,4,7,8,9,10,16,18,55,84,118,119,
  122,125,128,133,136,138,143,144,145,146,147,148,150,151,152,153,154,155,
  156,298,325,373,
378,376,375,374,178,177,176,175,0,5,380,
489,487,486,485,484,483,482,479,478,477,476,475,474,372,371,370,369,368,367,
  366,365,363,362,361,355,354,168,85,0,3,4,7,8,9,10,16,18,55,84,118,119,
  122,125,128,133,136,138,143,144,145,146,147,148,150,151,152,153,154,155,
  156,298,325,373,
378,376,375,374,178,177,176,175,0,5,380,
375,374,0,158,160,183,
489,487,486,485,484,483,482,479,478,477,476,475,474,372,371,370,369,368,367,
  366,365,363,362,361,355,354,168,85,0,3,4,7,8,9,10,16,18,55,84,118,119,
  122,125,128,133,136,138,143,144,145,146,147,148,150,151,152,153,154,155,
  156,298,325,373,
489,487,486,485,484,483,482,479,478,477,476,475,474,372,371,370,369,368,367,
  366,365,363,362,361,355,354,168,85,0,3,4,7,8,9,10,16,18,55,84,118,119,
  122,125,128,133,136,138,143,144,145,146,147,148,150,151,152,153,154,155,
  156,298,325,373,
489,487,486,485,484,483,482,479,478,477,476,475,474,372,371,370,369,368,367,
  366,365,363,362,361,355,354,168,85,0,3,4,7,8,9,10,16,18,55,84,118,119,
  122,125,128,133,136,138,143,144,145,146,147,148,150,151,152,153,154,155,
  156,298,325,373,
378,376,375,374,178,177,176,175,0,5,380,
478,477,0,164,278,
378,377,376,375,374,0,157,158,160,179,181,182,
378,377,376,375,374,0,157,158,160,179,181,182,
489,487,486,485,484,483,482,479,478,477,476,475,474,372,371,370,369,368,367,
  366,365,363,362,361,355,354,168,85,0,3,4,7,8,9,10,16,18,55,84,118,119,
  122,125,128,133,136,138,143,144,145,146,147,148,150,151,152,153,154,155,
  156,298,325,373,
489,487,486,485,484,483,482,479,478,477,476,475,474,372,371,370,369,368,367,
  366,365,363,362,361,355,354,168,85,0,3,4,7,8,9,10,16,18,55,84,118,119,
  122,125,128,133,136,138,143,144,145,146,147,148,150,151,152,153,154,155,
  156,298,325,373,
378,376,375,374,178,177,176,175,0,5,380,
378,376,375,374,178,177,176,175,0,5,380,
378,376,375,374,178,177,176,175,0,5,380,
376,375,374,180,0,6,401,
489,487,486,485,484,483,482,479,478,477,476,475,474,372,371,370,369,368,367,
  366,365,363,362,361,355,354,168,85,0,3,4,7,8,9,10,16,18,55,84,118,119,
  122,125,128,133,136,138,143,144,145,146,147,148,150,151,152,153,154,155,
  156,298,325,373,
489,487,486,485,484,483,482,479,478,477,476,475,474,372,371,370,369,368,367,
  366,365,363,362,361,355,354,168,85,0,3,4,7,8,9,10,16,18,55,84,118,119,
  122,125,128,133,136,138,143,144,145,146,147,148,150,151,152,153,154,155,
  156,298,325,373,
489,487,486,485,484,483,482,479,478,477,476,475,474,372,371,370,369,368,367,
  366,365,363,362,361,355,354,168,85,0,3,4,7,8,9,10,16,18,55,84,118,119,
  122,125,128,133,136,138,143,144,145,146,147,148,150,151,152,153,154,155,
  156,298,325,373,
375,374,0,158,160,183,
489,487,486,485,484,483,482,479,478,477,476,475,474,372,371,370,369,368,367,
  366,365,363,362,361,355,354,168,85,0,3,4,7,8,9,10,16,18,55,84,118,119,
  122,125,128,133,136,138,143,144,145,146,147,148,150,151,152,153,154,155,
  156,298,325,373,
489,487,486,485,484,483,482,479,478,477,476,475,474,372,371,370,369,368,367,
  366,365,363,362,361,355,354,168,85,0,3,4,7,8,9,10,16,18,55,84,118,119,
  122,125,128,133,136,138,143,144,145,146,147,148,150,151,152,153,154,155,
  156,298,325,373,
489,487,486,485,484,483,482,479,478,477,476,475,474,372,371,370,369,368,367,
  366,365,363,362,361,355,354,168,85,0,3,4,7,8,9,10,16,18,55,84,118,119,
  122,125,128,133,136,138,143,144,145,146,147,148,150,151,152,153,154,155,
  156,298,325,373,
489,487,486,485,484,483,482,479,478,477,476,475,474,372,371,370,369,368,367,
  366,365,363,362,361,355,354,168,85,0,3,4,7,8,9,10,16,18,55,84,118,119,
  122,125,128,133,136,138,143,144,145,146,147,148,150,151,152,153,154,155,
  156,298,325,373,
489,487,486,485,484,483,482,479,478,477,476,475,474,372,371,370,369,368,367,
  366,365,363,362,361,355,354,168,85,0,3,4,7,8,9,10,16,18,55,84,118,119,
  122,125,128,133,136,138,143,144,145,146,147,148,150,151,152,153,154,155,
  156,298,325,373,
489,487,486,485,484,483,482,479,478,477,476,475,474,372,371,370,369,368,367,
  366,365,363,362,361,355,354,168,85,0,3,4,7,8,9,10,16,18,55,84,118,119,
  122,125,128,133,136,138,143,144,145,146,147,148,150,151,152,153,154,155,
  156,298,325,373,
489,487,486,485,484,483,482,479,478,477,476,475,474,372,371,370,369,368,367,
  366,365,363,362,361,355,354,168,85,0,3,4,7,8,9,10,16,18,55,84,118,119,
  122,125,128,133,136,138,143,144,145,146,147,148,150,151,152,153,154,155,
  156,298,325,373,
489,487,486,485,484,483,482,479,478,477,476,475,474,372,371,370,369,368,367,
  366,365,363,362,361,355,354,168,85,0,3,4,7,8,9,10,16,18,55,84,118,119,
  122,125,128,133,136,138,143,144,145,146,147,148,150,151,152,153,154,155,
  156,298,325,373,
489,487,486,485,484,483,482,479,478,477,476,475,474,372,371,370,369,368,367,
  366,365,363,362,361,355,354,168,85,0,3,4,7,8,9,10,16,18,55,84,118,119,
  122,125,128,133,136,138,143,144,145,146,147,148,150,151,152,153,154,155,
  156,298,325,373,
489,487,486,485,484,483,482,479,478,477,476,475,474,372,371,370,369,368,367,
  366,365,363,362,361,355,354,168,85,0,3,4,7,8,9,10,16,18,55,84,118,119,
  122,125,128,133,136,138,143,144,145,146,147,148,150,151,152,153,154,155,
  156,298,325,373,
489,487,486,485,484,483,482,479,478,477,476,475,474,372,371,370,369,368,367,
  366,365,363,362,361,355,354,168,85,0,3,4,7,8,9,10,16,18,55,84,118,119,
  122,125,128,133,136,138,143,144,145,146,147,148,150,151,152,153,154,155,
  156,298,325,373,
489,487,486,485,484,483,482,479,478,477,476,475,474,372,371,370,369,368,367,
  366,365,363,362,361,355,354,168,85,0,3,4,7,8,9,10,16,18,55,84,118,119,
  122,125,128,133,136,138,143,144,145,146,147,148,150,151,152,153,154,155,
  156,298,325,373,
489,487,486,485,484,483,482,479,478,477,476,475,474,372,371,370,369,368,367,
  366,365,363,362,361,355,354,168,85,0,3,4,7,8,9,10,16,18,55,84,118,119,
  122,125,128,133,136,138,143,144,145,146,147,148,150,151,152,153,154,155,
  156,298,325,373,
489,487,486,485,484,483,482,479,478,477,476,475,474,372,371,370,369,368,367,
  366,365,363,362,361,355,354,168,85,0,3,4,7,8,9,10,16,18,55,84,118,119,
  122,125,128,133,136,138,143,144,145,146,147,148,150,151,152,153,154,155,
  156,298,325,373,
376,375,374,180,0,6,401,
378,376,375,374,178,177,176,175,0,5,380,
489,487,486,485,484,483,482,479,478,477,476,475,474,372,371,370,369,368,367,
  366,365,363,362,361,355,354,168,85,0,3,4,7,8,9,10,16,18,55,84,118,119,
  122,125,128,133,136,138,143,144,145,146,147,148,150,151,152,153,154,155,
  156,298,325,373,
376,375,374,180,0,6,401,
378,376,375,374,178,177,176,175,0,5,380,
376,375,374,180,0,6,401,
489,487,486,485,484,483,482,479,478,477,476,475,474,372,371,370,369,368,367,
  366,365,363,362,361,355,354,168,85,0,3,4,7,8,9,10,16,18,55,84,118,119,
  122,125,128,133,136,138,143,144,145,146,147,148,150,151,152,153,154,155,
  156,298,325,373,
489,487,486,485,484,483,482,479,478,477,476,475,474,372,371,370,369,368,367,
  366,365,363,362,361,355,354,168,85,0,3,4,7,8,9,10,16,18,55,84,118,119,
  122,125,128,133,136,138,143,144,145,146,147,148,150,151,152,153,154,155,
  156,298,325,373,
489,487,486,485,484,483,482,479,478,477,476,475,474,372,371,370,369,368,367,
  366,365,363,362,361,355,354,168,85,0,3,4,7,8,9,10,16,18,55,84,118,119,
  122,125,128,133,136,138,143,144,145,146,147,148,150,151,152,153,154,155,
  156,298,325,373,
489,487,486,485,484,483,482,479,478,477,476,475,474,372,371,370,369,368,367,
  366,365,363,362,361,355,354,168,85,0,3,4,7,8,9,10,16,18,55,84,118,119,
  122,125,128,133,136,138,143,144,145,146,147,148,150,151,152,153,154,155,
  156,298,325,373,
489,487,486,485,484,483,482,479,478,477,476,475,474,372,371,370,369,368,367,
  366,365,363,362,361,355,354,168,85,0,3,4,7,8,9,10,16,18,55,84,118,119,
  122,125,128,133,136,138,143,144,145,146,147,148,150,151,152,153,154,155,
  156,298,325,373,
489,487,486,485,484,483,482,479,478,477,476,475,474,372,371,370,369,368,367,
  366,365,363,362,361,355,354,168,85,0,3,4,7,8,9,10,16,18,55,84,118,119,
  122,125,128,133,136,138,143,144,145,146,147,148,150,151,152,153,154,155,
  156,298,325,373,
489,487,486,485,484,483,482,479,478,477,476,475,474,372,371,370,369,368,367,
  366,365,363,362,361,355,354,168,85,0,3,4,7,8,9,10,16,18,55,84,118,119,
  122,125,128,133,136,138,143,144,145,146,147,148,150,151,152,153,154,155,
  156,298,325,373,
378,376,375,374,178,177,176,175,0,5,380,
489,487,486,485,484,483,482,479,478,477,476,475,474,372,371,370,369,368,367,
  366,365,363,362,361,355,354,168,85,0,3,4,7,8,9,10,16,18,55,84,118,119,
  122,125,128,133,136,138,143,144,145,146,147,148,150,151,152,153,154,155,
  156,298,325,373,
489,487,486,485,484,483,482,479,478,477,476,475,474,372,371,370,369,368,367,
  366,365,363,362,361,355,354,168,85,0,3,4,7,8,9,10,16,18,55,84,118,119,
  122,125,128,133,136,138,143,144,145,146,147,148,150,151,152,153,154,155,
  156,298,325,373,
489,487,486,485,484,483,482,479,478,477,476,475,474,372,371,370,369,368,367,
  366,365,363,362,361,355,354,168,85,0,3,4,7,8,9,10,16,18,55,84,118,119,
  122,125,128,133,136,138,143,144,145,146,147,148,150,151,152,153,154,155,
  156,298,325,373,
489,487,486,485,484,483,482,479,478,477,476,475,474,372,371,370,369,368,367,
  366,365,363,362,361,355,354,168,85,0,3,4,7,8,9,10,16,18,55,84,118,119,
  122,125,128,133,136,138,143,144,145,146,147,148,150,151,152,153,154,155,
  156,298,325,373,
378,376,375,374,178,177,176,175,0,5,380,
489,487,486,485,484,483,482,479,478,477,476,475,474,372,371,370,369,368,367,
  366,365,363,362,361,355,354,168,85,0,3,4,7,8,9,10,16,18,55,84,118,119,
  122,125,128,133,136,138,143,144,145,146,147,148,150,151,152,153,154,155,
  156,298,325,373,
378,376,375,374,178,177,176,175,0,5,380,
378,376,375,374,178,177,176,175,0,5,380,
489,487,486,485,484,483,482,479,478,477,476,475,474,372,371,370,369,368,367,
  366,365,363,362,361,355,354,168,85,0,3,4,7,8,9,10,16,18,55,84,118,119,
  122,125,128,133,136,138,143,144,145,146,147,148,150,151,152,153,154,155,
  156,298,325,373,
471,470,469,468,467,466,465,464,463,462,461,460,459,458,457,456,455,454,453,
  452,451,450,449,448,447,446,445,444,443,442,441,440,439,438,437,436,435,
  434,433,432,431,430,429,428,427,426,425,424,423,422,421,420,419,418,417,
  416,415,414,413,412,411,410,409,408,407,406,405,404,403,402,400,399,398,
  397,396,395,394,393,392,391,390,389,388,387,386,385,384,383,382,381,379,
  361,0,143,184,185,186,187,188,189,190,191,192,193,194,195,196,197,198,
  199,200,201,202,203,204,205,206,207,208,209,210,211,212,213,214,215,216,
  217,218,219,220,221,222,223,224,225,226,227,228,229,230,231,232,233,234,
  235,236,237,238,239,240,241,242,243,244,245,246,247,248,249,250,251,252,
  253,254,255,256,257,258,259,261,262,263,264,265,266,267,268,269,270,271,
  272,273,274,275,276,
479,478,477,475,355,354,168,1,0,3,7,8,16,18,325,
489,488,487,486,485,484,483,482,481,480,479,478,477,476,474,473,472,365,364,
  363,361,358,357,356,355,354,348,346,344,288,287,286,173,1,0,17,
489,488,487,486,485,484,483,482,481,480,479,478,477,476,475,474,473,472,365,
  364,363,361,358,357,356,355,354,348,346,344,288,287,286,173,1,0,17,
330,288,287,286,285,35,1,0,2,282,283,284,
479,478,477,0,
487,0,
479,478,477,0,
479,478,477,0,
489,488,487,486,485,484,483,482,481,480,479,478,477,476,474,473,472,365,364,
  363,361,358,357,356,355,354,348,346,344,288,287,286,173,1,0,17,
479,478,477,0,
482,479,478,477,376,0,
482,479,478,477,0,
489,482,479,478,477,476,474,376,375,374,159,0,
365,35,1,0,2,282,283,284,
365,35,1,0,2,282,283,284,
365,35,1,0,2,282,283,284,
365,35,1,0,2,282,283,284,
365,35,1,0,2,282,283,284,
365,35,1,0,2,282,283,284,
365,35,1,0,2,282,283,284,
471,470,469,468,467,466,465,464,463,462,461,460,459,458,457,456,455,454,453,
  452,451,450,449,448,447,446,445,444,443,442,441,440,439,438,437,436,435,
  434,433,432,431,430,429,428,427,426,425,424,423,422,421,420,419,418,417,
  416,415,414,413,412,411,410,409,408,407,406,405,404,403,402,400,399,398,
  397,396,395,394,393,392,391,390,389,388,387,386,385,384,383,382,381,379,
  364,361,360,359,358,357,356,355,354,353,352,351,350,349,348,347,346,345,
  344,343,342,341,340,339,338,337,336,335,334,333,332,331,330,288,287,286,
  285,35,1,0,2,282,283,284,
471,470,469,468,467,466,465,464,463,462,461,460,459,458,457,456,455,454,453,
  452,451,450,449,448,447,446,445,444,443,442,441,440,439,438,437,436,435,
  434,433,432,431,430,429,428,427,426,425,424,423,422,421,420,419,418,417,
  416,415,414,413,412,411,410,409,408,407,406,405,404,403,402,400,399,398,
  397,396,395,394,393,392,391,390,389,388,387,386,385,384,383,382,381,379,
  364,361,360,359,358,357,356,355,354,353,352,351,350,349,348,347,346,345,
  344,343,342,341,340,339,338,337,336,335,334,333,332,331,330,288,287,286,
  285,35,1,0,2,282,283,284,
365,0,148,
365,0,148,
365,0,148,
365,0,148,
365,0,148,
365,0,148,
365,0,148,
489,487,486,485,484,483,482,479,478,477,476,475,474,372,371,370,369,368,367,
  366,365,355,354,168,85,35,1,0,2,282,283,284,
489,487,486,485,484,483,482,479,478,477,476,475,474,372,371,370,369,368,367,
  366,365,363,362,361,355,354,168,85,0,3,4,7,8,9,10,16,18,55,84,118,119,
  122,125,128,133,136,138,143,144,145,146,147,148,150,151,152,153,154,155,
  156,298,325,373,
489,487,486,485,484,483,482,479,478,477,476,475,474,372,371,370,369,368,367,
  366,365,355,354,168,85,0,3,4,7,8,9,10,16,18,84,138,146,147,148,150,151,
  152,153,154,155,156,298,325,373,
489,487,486,485,484,483,482,479,478,477,476,475,474,372,371,370,369,368,367,
  366,365,355,354,168,85,0,3,4,7,8,9,10,16,18,84,138,146,147,148,150,151,
  152,153,154,155,156,298,325,373,
489,487,486,485,484,483,482,479,478,477,476,475,474,372,371,370,369,368,367,
  366,365,355,354,168,85,0,3,4,7,8,9,10,16,18,84,138,146,147,148,150,151,
  152,153,154,155,156,298,325,373,
360,359,358,357,356,0,137,139,140,141,142,
355,354,0,134,135,
353,352,351,350,0,129,130,131,132,
349,348,0,126,127,
347,346,0,123,124,
345,344,0,120,121,
343,342,341,340,339,338,337,336,335,334,333,332,331,0,99,100,101,102,103,
  104,105,106,107,108,109,110,111,112,113,114,115,116,117,
489,487,486,485,484,483,482,479,478,477,476,475,474,372,371,370,369,368,367,
  366,365,363,362,361,355,354,168,85,0,3,4,7,8,9,10,16,18,55,84,118,119,
  122,125,128,133,136,138,143,144,145,146,147,148,150,151,152,153,154,155,
  156,298,325,373,
288,287,286,285,1,0,39,40,
330,288,287,286,285,1,0,39,40,
330,288,287,286,285,1,0,39,40,
330,288,287,286,285,1,0,39,40,
489,488,487,486,485,484,483,482,481,480,479,478,477,476,474,473,472,365,364,
  363,361,358,357,356,355,354,348,346,344,288,287,286,173,1,0,17,
330,288,287,286,285,1,0,39,40,
330,288,287,286,285,1,0,39,40,
330,288,287,286,285,1,0,39,40,
330,288,287,286,285,1,0,39,40,
490,489,488,487,486,485,484,483,482,481,480,479,478,477,476,475,474,473,472,
  365,364,363,361,358,357,356,355,354,348,346,344,288,287,286,1,0,
490,489,488,487,486,485,484,483,482,481,480,479,478,477,476,475,474,473,472,
  365,364,363,361,358,357,356,355,354,348,346,344,288,287,286,1,0,
471,470,469,468,467,466,465,464,463,462,461,460,459,458,457,456,455,454,453,
  452,451,450,449,448,447,446,445,444,443,442,441,440,439,438,437,436,435,
  434,433,432,431,430,429,428,427,426,425,424,423,422,421,420,419,418,417,
  416,415,414,413,412,411,410,409,408,407,406,405,404,403,402,400,399,398,
  397,396,395,394,393,392,391,390,389,388,387,386,385,384,383,382,381,379,
  361,330,288,287,286,285,35,1,0,2,282,283,284,
471,470,469,468,467,466,465,464,463,462,461,460,459,458,457,456,455,454,453,
  452,451,450,449,448,447,446,445,444,443,442,441,440,439,438,437,436,435,
  434,433,432,431,430,429,428,427,426,425,424,423,422,421,420,419,418,417,
  416,415,414,413,412,411,410,409,408,407,406,405,404,403,402,400,399,398,
  397,396,395,394,393,392,391,390,389,388,387,386,385,384,383,382,381,379,
  361,288,287,286,285,35,1,0,2,282,283,284,
471,470,469,468,467,466,465,464,463,462,461,460,459,458,457,456,455,454,453,
  452,451,450,449,448,447,446,445,444,443,442,441,440,439,438,437,436,435,
  434,433,432,431,430,429,428,427,426,425,424,423,422,421,420,419,418,417,
  416,415,414,413,412,411,410,409,408,407,406,405,404,403,402,400,399,398,
  397,396,395,394,393,392,391,390,389,388,387,386,385,384,383,382,381,379,
  361,288,287,286,285,35,1,0,2,282,283,284,
471,470,469,468,467,466,465,464,463,462,461,460,459,458,457,456,455,454,453,
  452,451,450,449,448,447,446,445,444,443,442,441,440,439,438,437,436,435,
  434,433,432,431,430,429,428,427,426,425,424,423,422,421,420,419,418,417,
  416,415,414,413,412,411,410,409,408,407,406,405,404,403,402,400,399,398,
  397,396,395,394,393,392,391,390,389,388,387,386,385,384,383,382,381,379,
  361,288,287,286,285,35,1,0,2,282,283,284,
471,470,469,468,467,466,465,464,463,462,461,460,459,458,457,456,455,454,453,
  452,451,450,449,448,447,446,445,444,443,442,441,440,439,438,437,436,435,
  434,433,432,431,430,429,428,427,426,425,424,423,422,421,420,419,418,417,
  416,415,414,413,412,411,410,409,408,407,406,405,404,403,402,400,399,398,
  397,396,395,394,393,392,391,390,389,388,387,386,385,384,383,382,381,379,
  361,288,287,286,285,35,1,0,2,282,283,284,
471,470,469,468,467,466,465,464,463,462,461,460,459,458,457,456,455,454,453,
  452,451,450,449,448,447,446,445,444,443,442,441,440,439,438,437,436,435,
  434,433,432,431,430,429,428,427,426,425,424,423,422,421,420,419,418,417,
  416,415,414,413,412,411,410,409,408,407,406,405,404,403,402,400,399,398,
  397,396,395,394,393,392,391,390,389,388,387,386,385,384,383,382,381,379,
  361,288,287,286,285,35,1,0,2,282,283,284,
330,1,0,39,40,
330,1,0,39,40,
471,470,469,468,467,466,465,464,463,462,461,460,459,458,457,456,455,454,453,
  452,451,450,449,448,447,446,445,444,443,442,441,440,439,438,437,436,435,
  434,433,432,431,430,429,428,427,426,425,424,423,422,421,420,419,418,417,
  416,415,414,413,412,411,410,409,408,407,406,405,404,403,402,400,399,398,
  397,396,395,394,393,392,391,390,389,388,387,386,385,384,383,382,381,379,
  361,288,287,286,285,35,1,0,2,282,283,284,
287,1,0,39,40,
489,482,479,478,477,474,376,375,374,159,0,
488,486,485,484,477,475,0,
489,488,487,486,485,484,483,482,481,480,479,478,477,476,474,473,472,365,364,
  363,361,358,357,356,355,354,348,346,344,288,287,286,173,1,0,17,
482,479,478,477,0,
475,0,
489,487,486,485,484,483,482,479,478,477,476,475,474,372,371,370,369,368,367,
  366,365,363,362,361,355,354,168,85,0,3,4,7,8,9,10,16,18,55,84,118,119,
  122,125,128,133,136,138,143,144,145,146,147,148,150,151,152,153,154,155,
  156,298,325,373,
489,487,486,485,484,483,482,479,478,477,476,475,474,372,371,370,369,368,367,
  366,365,363,362,361,355,354,168,85,0,3,4,7,8,9,10,16,18,55,84,118,119,
  122,125,128,133,136,138,143,144,145,146,147,148,150,151,152,153,154,155,
  156,298,325,373,
489,487,486,485,484,483,482,479,478,477,476,475,474,372,371,370,369,368,367,
  366,365,363,362,361,355,354,168,85,0,3,4,7,8,9,10,16,18,55,84,118,119,
  122,125,128,133,136,138,143,144,145,146,147,148,150,151,152,153,154,155,
  156,298,325,373,
489,487,486,485,484,483,482,479,478,477,476,475,474,372,371,370,369,368,367,
  366,365,363,362,361,355,354,168,85,0,3,4,7,8,9,10,16,18,55,84,118,119,
  122,125,128,133,136,138,143,144,145,146,147,148,150,151,152,153,154,155,
  156,298,325,373,
489,487,486,485,484,483,482,479,478,477,476,475,474,372,371,370,369,368,367,
  366,365,363,362,361,355,354,168,85,0,3,4,7,8,9,10,16,18,55,84,118,119,
  122,125,128,133,136,138,143,144,145,146,147,148,150,151,152,153,154,155,
  156,298,325,373,
489,487,486,485,484,483,482,479,478,477,476,475,474,372,371,370,369,368,367,
  366,365,363,362,361,355,354,168,85,0,3,4,7,8,9,10,16,18,55,84,118,119,
  122,125,128,133,136,138,143,144,145,146,147,148,150,151,152,153,154,155,
  156,298,325,373,
489,487,486,485,484,483,482,479,478,477,476,475,474,372,371,370,369,368,367,
  366,365,363,362,361,355,354,168,85,0,3,4,7,8,9,10,16,18,55,84,118,119,
  122,125,128,133,136,138,143,144,145,146,147,148,150,151,152,153,154,155,
  156,298,325,373,
364,0,149,
489,487,486,485,484,483,482,479,478,477,476,475,474,372,371,370,369,368,367,
  366,365,355,354,168,85,35,1,0,2,282,283,284,
489,487,486,485,484,483,482,479,478,477,476,475,474,372,371,370,369,368,367,
  366,365,355,354,168,85,1,0,39,40,
489,487,486,485,484,483,482,479,478,477,476,475,474,372,371,370,369,368,367,
  366,365,355,354,168,85,35,1,0,2,282,283,284,
489,487,486,485,484,483,482,479,478,477,476,475,474,372,371,370,369,368,367,
  366,365,355,354,168,85,1,0,39,40,
489,487,486,485,484,483,482,479,478,477,476,475,474,372,371,370,369,368,367,
  366,365,355,354,168,85,1,0,39,40,
489,487,486,485,484,483,482,479,478,477,476,475,474,372,371,370,369,368,367,
  366,365,355,354,168,85,1,0,39,40,
489,487,486,485,484,483,482,479,478,477,476,475,474,372,371,370,369,368,367,
  366,365,355,354,168,85,1,0,39,40,
489,487,486,485,484,483,482,479,478,477,476,475,474,372,371,370,369,368,367,
  366,365,363,362,361,355,354,168,85,1,0,39,40,
489,487,486,485,484,483,482,479,478,477,476,475,474,372,371,370,369,368,367,
  366,365,363,362,361,355,354,168,85,1,0,39,40,
489,487,486,485,484,483,482,479,478,477,476,475,474,372,371,370,369,368,367,
  366,365,363,362,361,355,354,168,85,35,1,0,2,282,283,284,
489,487,486,485,484,483,482,479,478,477,476,475,474,372,371,370,369,368,367,
  366,365,363,362,361,355,354,168,85,1,0,39,40,
489,487,486,485,484,483,482,479,478,477,476,475,474,372,371,370,369,368,367,
  366,365,363,362,361,355,354,168,85,35,1,0,2,282,283,284,
489,487,486,485,484,483,482,479,478,477,476,475,474,372,371,370,369,368,367,
  366,365,363,362,361,355,354,168,85,1,0,39,40,
489,487,486,485,484,483,482,479,478,477,476,475,474,372,371,370,369,368,367,
  366,365,363,362,361,355,354,168,85,35,1,0,2,282,283,284,
489,487,486,485,484,483,482,479,478,477,476,475,474,372,371,370,369,368,367,
  366,365,363,362,361,355,354,168,85,1,0,39,40,
489,487,486,485,484,483,482,479,478,477,476,475,474,372,371,370,369,368,367,
  366,365,363,362,361,355,354,168,85,35,1,0,2,282,283,284,
489,487,486,485,484,483,482,479,478,477,476,475,474,372,371,370,369,368,367,
  366,365,363,362,361,355,354,168,85,1,0,39,40,
489,487,486,485,484,483,482,479,478,477,476,475,474,372,371,370,369,368,367,
  366,365,363,362,361,355,354,168,85,35,1,0,2,282,283,284,
489,487,486,485,484,483,482,479,478,477,476,475,474,372,371,370,369,368,367,
  366,365,363,362,361,355,354,168,85,1,0,39,40,
489,487,486,485,484,483,482,479,478,477,476,475,474,372,371,370,369,368,367,
  366,365,363,362,361,355,354,168,85,1,0,39,40,
489,487,486,485,484,483,482,479,478,477,476,475,474,372,371,370,369,368,367,
  366,365,363,362,361,355,354,168,85,35,1,0,2,282,283,284,
489,487,486,485,484,483,482,479,478,477,476,475,474,372,371,370,369,368,367,
  366,365,363,362,361,355,354,168,85,1,0,39,40,
489,487,486,485,484,483,482,479,478,477,476,475,474,372,371,370,369,368,367,
  366,365,363,362,361,355,354,168,85,1,0,39,40,
489,487,486,485,484,483,482,479,478,477,476,475,474,372,371,370,369,368,367,
  366,365,363,362,361,355,354,168,85,35,1,0,2,282,283,284,
489,487,486,485,484,483,482,479,478,477,476,475,474,372,371,370,369,368,367,
  366,365,363,362,361,355,354,168,85,1,0,39,40,
489,487,486,485,484,483,482,479,478,477,476,475,474,372,371,370,369,368,367,
  366,365,363,362,361,355,354,168,85,1,0,39,40,
489,487,486,485,484,483,482,479,478,477,476,475,474,372,371,370,369,368,367,
  366,365,363,362,361,355,354,168,85,35,1,0,2,282,283,284,
489,487,486,485,484,483,482,479,478,477,476,475,474,372,371,370,369,368,367,
  366,365,363,362,361,355,354,168,85,35,1,0,2,282,283,284,
489,487,486,485,484,483,482,479,478,477,476,475,474,372,371,370,369,368,367,
  366,365,363,362,361,355,354,168,85,35,1,0,2,282,283,284,
489,487,486,485,484,483,482,479,478,477,476,475,474,372,371,370,369,368,367,
  366,365,363,362,361,355,354,168,85,35,1,0,2,282,283,284,
489,487,486,485,484,483,482,479,478,477,476,475,474,372,371,370,369,368,367,
  366,365,363,362,361,355,354,168,85,35,1,0,2,282,283,284,
489,487,486,485,484,483,482,479,478,477,476,475,474,372,371,370,369,368,367,
  366,365,363,362,361,355,354,168,85,35,1,0,2,282,283,284,
489,487,486,485,484,483,482,479,478,477,476,475,474,372,371,370,369,368,367,
  366,365,363,362,361,355,354,168,85,35,1,0,2,282,283,284,
489,487,486,485,484,483,482,479,478,477,476,475,474,372,371,370,369,368,367,
  366,365,363,362,361,355,354,168,85,35,1,0,2,282,283,284,
489,487,486,485,484,483,482,479,478,477,476,475,474,372,371,370,369,368,367,
  366,365,363,362,361,355,354,168,85,35,1,0,2,282,283,284,
489,487,486,485,484,483,482,479,478,477,476,475,474,372,371,370,369,368,367,
  366,365,363,362,361,355,354,168,85,35,1,0,2,282,283,284,
489,487,486,485,484,483,482,479,478,477,476,475,474,372,371,370,369,368,367,
  366,365,363,362,361,355,354,168,85,35,1,0,2,282,283,284,
489,487,486,485,484,483,482,479,478,477,476,475,474,372,371,370,369,368,367,
  366,365,363,362,361,355,354,168,85,35,1,0,2,282,283,284,
489,487,486,485,484,483,482,479,478,477,476,475,474,372,371,370,369,368,367,
  366,365,363,362,361,355,354,168,85,35,1,0,2,282,283,284,
330,0,83,
489,488,487,486,485,484,483,482,481,480,479,478,477,476,475,474,473,472,365,
  364,363,361,358,357,356,355,354,348,346,344,288,287,286,173,1,0,17,
330,0,83,
330,0,83,
330,0,83,
330,0,83,
330,0,83,
330,0,83,
287,0,34,
364,0,149,
364,0,149,
364,0,149,
364,0,149,
364,0,149,
364,0,149,
364,0,149,
471,470,469,468,467,466,465,464,463,462,461,460,459,458,457,456,455,454,453,
  452,451,450,449,448,447,446,445,444,443,442,441,440,439,438,437,436,435,
  434,433,432,431,430,429,428,427,426,425,424,423,422,421,420,419,418,417,
  416,415,414,413,412,411,410,409,408,407,406,405,404,403,402,400,399,398,
  397,396,395,394,393,392,391,390,389,388,387,386,385,384,383,382,381,379,
  364,361,360,359,358,357,356,355,354,353,352,351,350,349,348,347,346,345,
  344,343,342,341,340,339,338,337,336,335,334,333,332,331,330,288,287,286,
  285,1,0,39,40,
489,487,486,485,484,483,482,479,478,477,476,475,474,372,371,370,369,368,367,
  366,365,355,354,168,85,0,3,4,7,8,9,10,16,18,84,138,146,147,148,150,151,
  152,153,154,155,156,298,325,373,
489,487,486,485,484,483,482,479,478,477,476,475,474,372,371,370,369,368,367,
  366,365,355,354,168,85,0,3,4,7,8,9,10,16,18,84,138,146,147,148,150,151,
  152,153,154,155,156,298,325,373,
489,487,486,485,484,483,482,479,478,477,476,475,474,372,371,370,369,368,367,
  366,365,355,354,168,85,0,3,4,7,8,9,10,16,18,84,138,146,147,148,150,151,
  152,153,154,155,156,298,325,373,
489,487,486,485,484,483,482,479,478,477,476,475,474,372,371,370,369,368,367,
  366,365,355,354,168,85,0,3,4,7,8,9,10,16,18,84,138,146,147,148,150,151,
  152,153,154,155,156,298,325,373,
489,487,486,485,484,483,482,479,478,477,476,475,474,372,371,370,369,368,367,
  366,365,355,354,168,85,0,3,4,7,8,9,10,16,18,84,138,146,147,148,150,151,
  152,153,154,155,156,298,325,373,
489,487,486,485,484,483,482,479,478,477,476,475,474,372,371,370,369,368,367,
  366,365,363,362,361,355,354,168,85,0,3,4,7,8,9,10,16,18,84,133,136,138,
  143,144,145,146,147,148,150,151,152,153,154,155,156,298,325,373,
489,487,486,485,484,483,482,479,478,477,476,475,474,372,371,370,369,368,367,
  366,365,363,362,361,355,354,168,85,0,3,4,7,8,9,10,16,18,84,133,136,138,
  143,144,145,146,147,148,150,151,152,153,154,155,156,298,325,373,
489,487,486,485,484,483,482,479,478,477,476,475,474,372,371,370,369,368,367,
  366,365,363,362,361,355,354,168,85,0,3,4,7,8,9,10,16,18,84,128,133,136,
  138,143,144,145,146,147,148,150,151,152,153,154,155,156,298,325,373,
489,487,486,485,484,483,482,479,478,477,476,475,474,372,371,370,369,368,367,
  366,365,363,362,361,355,354,168,85,0,3,4,7,8,9,10,16,18,84,128,133,136,
  138,143,144,145,146,147,148,150,151,152,153,154,155,156,298,325,373,
489,487,486,485,484,483,482,479,478,477,476,475,474,372,371,370,369,368,367,
  366,365,363,362,361,355,354,168,85,0,3,4,7,8,9,10,16,18,84,128,133,136,
  138,143,144,145,146,147,148,150,151,152,153,154,155,156,298,325,373,
489,487,486,485,484,483,482,479,478,477,476,475,474,372,371,370,369,368,367,
  366,365,363,362,361,355,354,168,85,0,3,4,7,8,9,10,16,18,84,128,133,136,
  138,143,144,145,146,147,148,150,151,152,153,154,155,156,298,325,373,
489,487,486,485,484,483,482,479,478,477,476,475,474,372,371,370,369,368,367,
  366,365,363,362,361,355,354,168,85,0,3,4,7,8,9,10,16,18,84,125,128,133,
  136,138,143,144,145,146,147,148,150,151,152,153,154,155,156,298,325,373,
489,487,486,485,484,483,482,479,478,477,476,475,474,372,371,370,369,368,367,
  366,365,363,362,361,355,354,168,85,0,3,4,7,8,9,10,16,18,84,125,128,133,
  136,138,143,144,145,146,147,148,150,151,152,153,154,155,156,298,325,373,
489,487,486,485,484,483,482,479,478,477,476,475,474,372,371,370,369,368,367,
  366,365,363,362,361,355,354,168,85,0,3,4,7,8,9,10,16,18,84,122,125,128,
  133,136,138,143,144,145,146,147,148,150,151,152,153,154,155,156,298,325,
  373,
489,487,486,485,484,483,482,479,478,477,476,475,474,372,371,370,369,368,367,
  366,365,363,362,361,355,354,168,85,0,3,4,7,8,9,10,16,18,84,122,125,128,
  133,136,138,143,144,145,146,147,148,150,151,152,153,154,155,156,298,325,
  373,
489,487,486,485,484,483,482,479,478,477,476,475,474,372,371,370,369,368,367,
  366,365,363,362,361,355,354,168,85,0,3,4,7,8,9,10,16,18,84,119,122,125,
  128,133,136,138,143,144,145,146,147,148,150,151,152,153,154,155,156,298,
  325,373,
489,487,486,485,484,483,482,479,478,477,476,475,474,372,371,370,369,368,367,
  366,365,363,362,361,355,354,168,85,0,3,4,7,8,9,10,16,18,84,119,122,125,
  128,133,136,138,143,144,145,146,147,148,150,151,152,153,154,155,156,298,
  325,373,
489,487,486,485,484,483,482,479,478,477,476,475,474,473,378,376,375,374,372,
  371,370,369,368,367,366,365,363,362,361,355,354,178,177,176,175,168,85,
  35,1,0,2,282,283,284,
489,487,486,485,484,483,482,474,85,1,0,39,40,
489,487,486,485,484,483,482,479,478,477,476,475,474,473,372,371,370,369,368,
  367,366,365,363,362,361,355,354,168,85,1,0,39,40,
489,487,486,485,484,483,482,479,478,477,476,475,474,372,371,370,369,368,367,
  366,365,363,362,361,355,354,168,85,1,0,39,40,
378,376,375,374,178,177,176,175,1,0,39,40,
489,487,486,485,484,483,482,479,478,477,476,475,474,372,371,370,369,368,367,
  366,365,363,362,361,355,354,168,85,1,0,39,40,
360,359,358,357,356,0,137,139,140,141,142,
360,359,358,357,356,0,137,139,140,141,142,
355,354,0,134,135,
355,354,0,134,135,
355,354,0,134,135,
355,354,0,134,135,
353,352,351,350,0,129,130,131,132,
353,352,351,350,0,129,130,131,132,
349,348,0,126,127,
349,348,0,126,127,
347,346,0,123,124,
347,346,0,123,124,
489,487,486,485,484,483,482,474,85,0,4,84,298,
489,487,486,485,484,483,482,479,478,477,476,475,474,473,372,371,370,369,368,
  367,366,365,363,362,361,355,354,168,85,0,3,4,7,8,9,10,11,13,16,18,19,21,
  55,84,118,119,122,125,128,133,136,138,143,144,145,146,147,148,150,151,
  152,153,154,155,156,290,298,323,325,373,
489,487,486,485,484,483,482,479,478,477,476,475,474,372,371,370,369,368,367,
  366,365,363,362,361,355,354,168,85,0,3,4,7,8,9,10,16,18,55,84,118,119,
  122,125,128,133,136,138,143,144,145,146,147,148,150,151,152,153,154,155,
  156,298,325,373,
378,376,375,374,178,177,176,175,0,5,380,
489,487,486,485,484,483,482,479,478,477,476,475,474,372,371,370,369,368,367,
  366,365,363,362,361,355,354,168,85,0,3,4,7,8,9,10,16,18,55,84,118,119,
  122,125,128,133,136,138,143,144,145,146,147,148,150,151,152,153,154,155,
  156,298,325,373,

};


static unsigned const char ag_astt[19815] = {
  8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,
  8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,
  8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,
  8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,1,8,7,1,1,1,1,1,1,1,1,1,1,1,
  1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,7,1,1,9,5,3,3,8,8,
  8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,
  8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,
  8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,
  8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,1,7,0,1,1,1,1,1,9,9,9,9,9,9,9,
  9,9,9,9,9,9,9,9,9,9,9,9,9,9,9,9,9,9,9,9,9,9,9,9,9,9,9,3,9,7,9,5,2,2,2,2,2,
  2,2,2,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,
  8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,
  8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,
  8,8,8,8,1,1,1,1,1,1,1,1,1,3,1,1,1,2,1,7,3,1,1,3,1,3,1,1,1,1,1,1,1,1,2,2,1,
  1,1,1,1,1,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,
  8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,
  8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,
  8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,3,1,7,3,3,1,1,5,
  5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,
  5,5,5,5,5,5,1,5,7,1,1,1,3,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,
  5,5,5,5,5,5,5,5,5,5,1,5,7,1,1,1,3,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,
  5,5,5,5,5,5,5,5,5,5,5,5,5,5,1,5,7,1,1,1,3,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,
  1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,8,1,1,1,7,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,
  1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,
  1,1,1,1,1,1,1,1,1,1,1,1,8,1,1,1,7,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,
  1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,
  1,1,1,1,1,1,1,1,1,8,1,1,1,7,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,
  1,1,1,1,1,1,1,1,1,1,1,1,2,2,2,2,2,2,2,2,2,2,2,2,1,1,7,1,2,1,1,3,5,5,5,5,5,
  5,5,5,5,1,1,7,1,1,1,3,5,5,5,5,5,5,5,5,5,1,1,7,1,1,1,3,5,5,5,5,1,1,7,1,1,1,
  3,5,5,5,5,1,1,7,1,1,1,3,5,5,5,5,5,5,5,5,5,1,1,7,1,1,1,3,5,5,5,5,5,5,5,5,5,
  1,1,7,1,1,1,3,5,5,1,1,7,1,1,1,3,5,5,1,1,7,1,1,1,3,5,5,5,5,5,5,5,5,5,5,5,5,
  5,5,5,1,1,7,1,1,1,3,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,
  5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,
  5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,
  5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,1,1,7,1,1,1,3,1,1,1,1,1,1,1,1,1,1,1,1,1,
  1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,
  1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,
  1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,7,1,2,2,2,1,1,1,1,1,1,1,
  1,1,1,2,2,1,1,2,1,1,1,1,1,1,1,1,1,1,2,1,1,1,2,2,1,1,1,1,1,1,1,1,2,1,1,1,2,
  2,2,2,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,2,1,1,1,2,1,1,1,2,1,1,2,2,
  2,2,2,2,2,2,2,2,2,2,2,2,1,2,1,1,1,2,2,2,1,1,2,1,1,2,1,1,2,1,1,1,1,1,7,1,1,
  1,1,1,2,2,2,2,2,2,2,2,2,7,2,1,1,2,2,2,2,2,2,2,2,2,7,2,1,1,2,2,2,2,2,2,2,2,
  2,7,2,1,1,2,2,2,2,2,2,2,2,2,7,2,1,1,1,1,7,2,2,2,2,7,2,1,2,1,1,1,2,2,2,2,2,
  2,2,2,2,2,2,2,2,2,2,7,2,1,2,1,2,1,1,1,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,
  5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,
  5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,
  5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,1,1,5,5,5,5,7,1,1,1,1,1,1,7,3,1,1,
  1,3,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,1,
  5,7,1,1,1,3,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,
  5,5,5,5,5,5,5,5,5,5,5,5,1,5,7,1,1,1,3,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,
  5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,1,5,7,1,1,1,3,5,5,5,5,
  5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,1,5,7,1,1,1,3,
  5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,
  5,5,5,5,5,5,5,1,5,7,1,1,1,3,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,
  5,5,5,5,5,5,5,5,5,5,5,1,5,7,1,1,1,3,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,
  5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,1,5,7,1,1,1,3,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,
  5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,1,5,7,1,1,1,3,5,5,5,5,5,5,5,5,5,5,5,
  5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,1,5,7,1,1,1,3,5,5,5,5,5,5,5,
  5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,1,5,7,1,1,1,3,5,5,5,
  5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,1,5,7,1,1,1,
  3,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,1,5,
  7,1,1,1,3,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,
  5,5,1,5,7,1,1,1,3,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,
  5,5,5,5,5,5,1,5,7,1,1,1,3,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,
  5,5,5,5,5,5,5,5,5,5,1,5,7,1,1,1,3,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,
  5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,
  5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,
  5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,1,5,7,1,1,
  1,3,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,
  5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,
  5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,
  5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,1,5,7,1,1,1,3,5,5,5,5,5,5,5,5,5,5,5,5,5,
  5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,1,5,7,1,1,1,3,
  5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,1,5,7,
  1,1,1,3,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,
  5,5,5,5,5,5,5,5,5,5,5,1,5,7,1,1,1,3,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,
  5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,1,5,7,1,1,1,3,5,5,5,5,5,
  5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,
  5,1,5,7,1,1,1,3,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,
  5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,
  5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,
  5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,
  5,5,5,5,5,5,5,5,5,1,5,7,1,1,1,3,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,
  5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,1,5,7,1,1,1,3,5,5,5,5,5,5,5,
  5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,1,5,7,1,1,1,3,5,5,5,
  5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,
  5,5,5,5,1,5,7,1,1,1,3,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,
  5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,1,5,7,1,1,1,3,5,5,5,5,5,5,5,5,5,5,5,5,5,
  5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,1,5,7,1,1,1,3,5,5,5,5,5,5,5,5,5,
  5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,1,5,7,1,1,1,3,5,5,5,5,5,
  5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,
  5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,
  5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,
  5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,1,5,7,1,1,1,3,5,5,5,5,5,5,5,5,5,5,5,
  5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,1,5,7,1,1,1,3,1,1,1,1,1,1,1,
  1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,9,5,3,3,3,3,3,3,3,3,3,
  3,3,3,3,3,3,3,3,3,3,3,3,3,3,3,3,3,3,3,3,3,3,3,3,3,7,3,3,7,3,3,7,3,5,5,5,5,
  5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,
  5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,
  5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,1,5,7,1,1,1,3,5,5,5,5,5,5,5,5,5,5,5,5,
  5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,1,5,7,1,1,1,3,5,5,5,5,5,5,5,5,1,5,7,1,1,1,
  3,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,
  5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,
  5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,1,5,7,1,1,1,3,5,5,5,5,5,5,5,
  5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,1,5,7,1,1,1,3,5,5,5,5,5,5,5,5,1,
  5,7,1,1,1,3,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,
  5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,
  5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,1,5,7,1,1,1,3,5,5,
  1,5,7,1,1,1,3,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,1,5,
  7,1,1,1,3,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,
  5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,
  5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,1,5,7,1,1,1,3,5,5,5,
  5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,
  5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,
  5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,1,5,7,1,1,1,3,5,5,5,5,5,5,5,5,5,5,5,
  5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,
  5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,
  5,5,5,5,5,5,5,5,5,5,5,1,5,7,1,1,1,3,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,
  5,5,5,5,5,5,5,5,5,1,5,7,1,1,1,3,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,
  5,5,5,5,5,5,5,1,5,7,1,1,1,3,5,5,5,5,5,5,5,5,1,5,7,1,1,1,3,5,5,5,5,5,5,5,5,
  5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,
  5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,
  5,5,5,5,5,5,5,5,5,5,5,5,5,5,1,5,7,1,1,1,3,5,5,1,5,7,1,1,1,3,5,5,5,5,5,5,5,
  5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,
  5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,
  5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,1,5,7,1,1,1,3,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,
  5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,
  5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,
  5,5,5,5,5,5,5,1,5,7,1,1,1,3,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,
  5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,
  5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,1,
  5,7,1,1,1,3,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,
  5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,
  5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,1,5,7,1,1,1,3,5,5,
  5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,
  5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,
  5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,1,5,7,1,1,1,3,5,5,5,5,5,5,5,5,5,5,
  5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,
  5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,
  5,5,5,5,5,5,5,5,5,5,5,5,1,5,7,1,1,1,3,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,
  5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,
  5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,
  5,5,5,5,1,5,7,1,1,1,3,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,
  5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,
  5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,1,5,7,1,
  1,1,3,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,
  5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,
  5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,1,5,7,1,1,1,3,5,5,5,5,5,
  5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,
  5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,
  5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,1,5,7,1,1,1,3,5,5,5,5,5,5,5,5,5,5,5,5,5,
  5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,
  5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,
  5,5,5,5,5,5,5,5,5,1,5,7,1,1,1,3,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,
  5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,
  5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,
  5,1,5,7,1,1,1,3,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,
  5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,
  5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,1,5,7,1,1,1,3,
  5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,
  5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,
  5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,1,5,7,1,1,1,3,5,5,5,5,5,1,5,7,
  1,1,1,3,5,5,5,5,5,1,5,7,1,1,1,3,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,
  5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,
  5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,
  5,1,5,7,1,1,1,3,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,1,
  5,7,1,1,1,3,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,1,5,7,
  1,1,1,3,5,5,5,5,5,5,5,5,1,5,7,1,1,1,3,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,
  5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,
  5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,
  5,5,5,5,1,5,7,1,1,1,3,5,5,5,5,5,5,5,5,1,5,7,1,1,1,3,5,5,5,5,5,5,5,5,1,5,7,
  1,1,1,3,5,5,5,5,1,5,7,1,1,1,3,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,
  5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,
  5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,
  1,5,7,1,1,1,3,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,1,5,
  7,1,1,1,3,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,1,5,7,1,
  1,1,3,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,1,5,7,1,1,1,
  3,5,5,1,5,7,1,1,1,3,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,
  5,1,5,7,1,1,1,3,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,1,
  5,7,1,1,1,3,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,1,5,7,
  1,1,1,3,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,1,5,7,1,1,
  1,3,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,1,5,7,1,1,1,3,
  5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,1,5,7,1,1,1,3,5,5,
  5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,1,5,7,1,1,1,3,5,5,5,5,
  5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,1,5,7,1,1,1,3,5,5,5,5,5,5,
  5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,1,5,7,1,1,1,3,5,5,5,5,5,5,5,5,
  5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,1,5,7,1,1,1,3,5,5,5,5,5,5,5,5,5,5,
  5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,1,5,7,1,1,1,3,5,5,5,5,5,5,5,5,5,5,5,5,
  5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,1,5,7,1,1,1,3,5,5,5,5,5,5,5,5,5,5,5,5,5,5,
  5,5,5,5,5,5,5,5,5,5,5,5,5,5,1,5,7,1,1,1,3,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,
  5,5,5,5,5,5,5,5,5,5,5,5,1,5,7,1,1,1,3,5,5,5,5,1,5,7,1,1,1,3,5,5,5,5,5,5,5,
  5,1,5,7,1,1,1,3,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,1,
  5,7,1,1,1,3,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,
  5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,
  5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,1,5,7,1,1,1,3,5,5,
  5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,
  5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,
  5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,1,5,7,1,1,1,3,5,5,5,5,5,5,5,5,5,5,
  5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,
  5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,
  5,5,5,5,5,5,5,5,5,5,5,5,1,5,7,1,1,1,3,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,
  5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,
  5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,
  5,5,5,5,1,5,7,1,1,1,3,5,5,5,5,1,5,7,1,1,1,3,5,5,5,5,5,5,5,5,1,5,7,1,1,1,3,
  5,5,5,5,1,5,7,1,1,1,3,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,
  5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,
  5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,1,5,7,1,
  1,1,3,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,1,5,7,1,1,1,
  3,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,1,5,7,1,1,1,3,5,
  5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,1,5,7,1,1,1,3,5,5,5,
  5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,1,5,7,1,1,1,3,5,5,5,5,5,
  5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,1,5,7,1,1,1,3,5,5,5,5,5,5,5,
  5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,1,5,7,1,1,1,3,5,5,5,5,5,5,5,5,5,
  5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,1,5,7,1,1,1,3,5,5,5,5,5,5,5,5,1,5,7,
  1,1,1,3,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,
  5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,
  5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,1,5,7,1,1,1,3,5,5,5,5,
  5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,
  5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,
  5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,1,5,7,1,1,1,3,5,5,5,5,5,5,5,5,5,5,5,5,
  5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,1,5,7,1,1,1,3,5,5,5,5,5,5,5,5,5,5,5,5,5,5,
  5,5,5,5,5,5,5,5,5,5,5,5,5,5,1,5,7,1,1,1,3,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,
  5,5,5,5,5,5,5,5,5,5,5,5,1,5,7,1,1,1,3,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,
  5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,
  5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,
  5,5,5,5,1,5,7,1,1,1,3,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,
  5,5,1,5,7,1,1,1,3,5,5,5,5,5,5,5,5,1,5,7,1,1,1,3,5,5,5,5,5,5,5,5,5,5,5,5,5,
  5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,1,5,7,1,1,1,3,5,5,5,5,5,5,5,5,1,5,7,1,1,1,3,
  5,5,5,5,5,5,5,5,1,5,7,1,1,1,3,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,
  5,5,5,5,5,5,1,5,7,1,1,1,3,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,
  8,8,8,8,1,7,1,1,8,8,8,8,8,8,8,8,1,7,1,1,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,
  8,8,8,8,8,8,8,8,8,8,8,1,7,1,1,8,8,8,8,8,8,8,8,1,7,1,1,8,8,1,7,1,1,8,8,8,8,
  8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,1,7,1,1,8,8,8,8,8,8,8,8,8,
  8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,1,7,1,1,8,8,8,8,8,8,8,8,8,8,8,8,8,8,
  8,8,8,8,8,8,8,8,8,8,8,8,8,8,1,7,1,1,8,8,8,8,8,8,8,8,1,7,1,1,8,8,1,7,1,1,8,
  8,8,8,8,1,7,1,1,8,8,8,8,8,1,7,1,1,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,
  8,8,8,8,8,8,8,8,1,7,1,1,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,
  8,8,8,1,7,1,1,8,8,8,8,8,8,8,8,1,7,1,1,8,8,8,8,8,8,8,8,1,7,1,1,8,8,8,8,8,8,
  8,8,1,7,1,1,8,8,8,8,1,7,1,1,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,
  8,8,8,8,8,1,7,1,1,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,
  1,7,1,1,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,1,7,1,1,8,
  8,1,7,1,1,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,1,7,1,1,
  8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,1,7,1,1,8,8,8,8,8,
  8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,1,7,1,1,8,8,8,8,8,8,8,8,8,8,
  8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,1,7,1,1,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,
  8,8,8,8,8,8,8,8,8,8,8,8,8,1,7,1,1,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,
  8,8,8,8,8,8,8,8,1,7,1,1,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,
  8,8,8,1,7,1,1,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,1,7,
  1,1,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,1,7,1,1,8,8,8,
  8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,1,7,1,1,8,8,8,8,8,8,8,8,
  8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,1,7,1,1,8,8,8,8,8,8,8,8,8,8,8,8,8,
  8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,1,7,1,1,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,
  8,8,8,8,8,8,8,8,8,8,1,7,1,1,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,
  8,8,8,8,8,1,7,1,1,8,8,8,8,1,7,1,1,8,8,8,8,8,8,8,8,1,7,1,1,8,8,8,8,8,8,8,8,
  8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,1,7,1,1,8,8,8,8,1,7,1,1,8,8,8,8,8,
  8,8,8,1,7,1,1,8,8,8,8,1,7,1,1,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,
  8,8,8,8,8,8,1,7,1,1,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,
  8,1,7,1,1,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,1,7,1,1,
  8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,1,7,1,1,8,8,8,8,8,
  8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,1,7,1,1,8,8,8,8,8,8,8,8,8,8,
  8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,1,7,1,1,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,
  8,8,8,8,8,8,8,8,8,8,8,8,8,1,7,1,1,8,8,8,8,8,8,8,8,1,7,1,1,8,8,8,8,8,8,8,8,
  8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,1,7,1,1,8,8,8,8,8,8,8,8,8,8,8,8,8,
  8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,1,7,1,1,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,
  8,8,8,8,8,8,8,8,8,8,1,7,1,1,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,
  8,8,8,8,8,1,7,1,1,8,8,8,8,8,8,8,8,1,7,1,1,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,
  8,8,8,8,8,8,8,8,8,8,8,8,1,7,1,1,8,8,8,8,8,8,8,8,1,7,1,1,8,8,8,8,8,8,8,8,1,
  7,1,1,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,1,7,1,1,8,8,
  8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,
  8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,
  8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,5,5,5,5,1,7,1,1,5,5,5,5,5,5,5,5,5,1,1,7,1,
  1,1,3,2,2,2,2,2,2,2,2,2,7,2,1,1,5,5,5,5,5,5,5,5,5,5,5,5,5,1,1,7,1,1,1,3,2,
  2,2,2,2,2,2,2,2,2,2,2,2,7,2,1,5,5,5,5,1,1,7,1,1,1,3,5,5,5,5,1,5,7,1,1,1,3,
  1,4,1,5,5,1,1,7,1,1,1,3,1,2,7,2,1,1,2,1,1,5,5,5,5,1,1,7,1,1,1,3,5,5,5,5,1,
  1,7,1,1,1,3,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,1,1,7,
  1,1,1,3,2,2,2,2,2,2,2,2,2,1,1,1,2,1,1,1,1,1,1,1,1,1,1,1,1,1,2,2,7,2,2,1,1,
  2,1,2,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,2,1,1,1,1,1,1,1,1,1,1,1,1,5,5,5,5,5,
  5,5,5,5,5,5,5,5,1,1,7,1,1,1,3,2,2,2,2,2,2,2,2,2,4,2,1,1,5,5,5,5,5,5,5,5,5,
  5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,1,1,7,1,1,1,3,2,2,2,2,2,2,2,2,2,1,1,
  1,2,1,1,1,1,1,1,1,1,1,1,1,1,1,2,2,7,2,2,1,1,2,1,2,1,2,1,1,1,1,1,1,1,1,1,1,
  1,1,2,1,1,1,1,1,1,1,1,1,1,1,1,5,5,5,5,5,5,5,5,5,1,1,7,1,1,1,3,2,2,2,2,2,2,
  2,2,2,7,2,1,1,5,5,5,5,5,5,5,5,5,1,1,7,1,1,1,3,2,2,2,2,2,2,2,2,2,7,2,1,1,1,
  5,5,5,5,5,5,5,5,5,1,1,7,1,1,1,3,2,2,2,2,2,2,2,2,2,7,2,1,1,1,5,5,5,5,5,5,5,
  5,5,1,1,7,1,1,1,3,2,2,2,2,2,2,2,2,2,7,2,1,1,1,5,5,5,5,5,5,5,5,5,5,5,5,5,5,
  5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,1,1,7,1,1,1,3,2,2,2,2,2,2,2,2,2,1,1,1,2,2,1,
  1,1,1,1,1,1,1,1,1,1,1,1,2,2,7,2,2,1,1,2,1,2,1,2,1,1,1,2,1,1,1,1,1,1,1,1,1,
  1,1,1,1,2,1,1,1,1,1,1,1,1,1,1,1,1,1,1,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,
  5,5,5,5,5,5,5,5,5,5,5,1,1,7,1,1,1,3,2,2,2,2,2,2,2,2,2,1,1,1,2,2,1,1,1,1,1,
  1,1,1,1,1,1,1,1,2,2,7,2,2,1,1,2,1,2,1,2,1,1,1,2,1,1,1,1,1,1,1,1,1,1,1,1,1,
  2,1,1,1,1,1,1,1,1,1,1,1,1,1,1,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,
  5,5,5,5,5,5,1,1,7,1,1,1,3,2,2,2,2,2,2,2,2,2,1,1,1,2,1,1,1,1,1,1,1,1,1,1,1,
  1,1,2,2,7,2,2,1,1,2,1,2,1,2,1,1,1,1,1,1,1,1,1,1,1,1,2,1,1,1,1,1,1,1,1,1,1,
  1,1,5,5,5,5,1,1,7,1,1,1,3,5,5,5,5,1,1,7,1,1,1,3,5,5,5,5,1,1,7,1,1,1,3,5,5,
  5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,1,1,7,1,1,1,3,2,2,2,2,
  2,2,2,2,2,1,1,1,2,1,1,1,1,1,1,1,1,1,1,1,1,1,2,2,7,2,2,1,1,2,1,2,1,2,1,1,1,
  1,1,1,1,1,1,1,1,1,2,1,1,1,1,1,1,1,1,1,1,1,1,2,2,2,2,2,2,2,2,2,1,1,1,2,2,1,
  1,1,1,1,1,1,1,1,1,1,1,1,2,2,7,2,2,1,1,2,1,2,1,2,1,1,1,2,1,1,1,1,1,1,1,1,1,
  1,1,1,1,2,1,1,1,1,1,1,1,1,1,1,1,1,1,1,2,2,2,2,2,2,2,2,2,1,1,1,2,2,1,1,1,1,
  1,1,1,1,1,1,1,1,1,2,2,7,2,2,1,1,2,1,2,1,2,1,1,1,2,1,1,1,1,1,1,1,1,1,1,1,1,
  1,2,1,1,1,1,1,1,1,1,1,1,1,1,1,1,2,2,2,2,2,2,2,2,2,1,1,1,2,1,1,1,1,1,1,1,1,
  1,1,1,1,1,2,2,7,2,2,1,1,2,1,2,1,2,1,1,1,1,1,1,1,1,1,1,1,1,2,1,1,1,1,1,1,1,
  1,1,1,1,1,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,1,1,7,1,
  1,1,3,2,2,2,2,2,2,2,2,2,1,1,1,2,1,1,1,1,1,1,1,1,1,1,1,1,1,2,2,7,2,2,1,1,2,
  1,2,1,2,1,1,1,1,1,1,1,1,1,1,1,1,2,1,1,1,1,1,1,1,1,1,1,1,1,5,5,5,5,5,5,5,5,
  5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,1,1,7,1,1,1,3,2,2,2,2,2,2,2,2,2,1,
  1,1,2,1,1,1,1,1,1,1,1,1,1,1,1,1,2,2,7,2,2,1,1,2,1,2,1,2,1,1,1,1,1,1,1,1,1,
  1,1,1,2,1,1,1,1,1,1,1,1,1,1,1,1,2,2,2,2,2,2,2,2,2,2,2,5,5,5,5,5,5,5,5,5,5,
  5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,1,1,7,1,2,1,1,3,5,5,5,5,1,1,7,1,1,1,3,5,5,5,
  5,1,1,7,1,1,1,3,10,10,1,10,10,10,10,10,10,2,10,10,10,10,10,10,10,10,10,10,
  10,10,10,10,10,10,10,10,10,10,10,10,10,7,5,5,5,5,1,1,7,1,1,1,3,10,10,1,10,
  10,10,10,10,10,10,10,10,10,10,10,10,10,3,10,10,10,10,10,10,10,10,10,10,10,
  10,10,10,10,10,7,5,5,5,5,5,1,5,7,1,1,1,3,10,10,10,10,10,10,10,10,10,10,10,
  10,10,5,5,5,1,1,7,1,1,1,3,2,2,2,2,2,2,2,2,2,1,1,1,2,1,1,1,1,1,1,1,1,1,1,1,
  1,1,2,2,7,2,2,1,1,2,1,2,1,2,1,1,1,1,1,1,1,1,1,1,1,1,2,1,1,1,1,1,1,1,1,1,1,
  1,1,2,2,2,2,2,2,2,2,2,1,1,1,2,1,1,1,1,1,1,1,1,1,1,1,1,1,2,2,7,2,2,1,1,2,1,
  2,1,2,1,1,1,1,1,1,1,1,1,1,1,1,2,1,1,1,1,1,1,1,1,1,1,1,1,5,5,5,5,5,5,5,5,5,
  5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,
  5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,
  5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,
  5,5,5,5,5,5,5,5,5,5,5,5,5,5,1,5,5,7,1,1,1,3,2,2,2,2,2,2,2,2,2,1,1,1,2,1,1,
  1,1,1,1,1,1,1,1,1,1,1,2,2,7,2,2,1,1,2,1,2,1,2,1,1,1,1,1,1,1,1,1,1,1,1,2,1,
  1,1,1,1,1,1,1,1,1,1,1,2,2,2,2,2,2,2,2,7,2,1,2,2,2,2,2,2,2,2,2,1,1,1,2,1,1,
  1,1,1,1,1,1,1,1,1,1,1,2,2,7,2,2,1,1,2,1,2,1,2,1,1,1,1,1,1,1,1,1,1,1,1,2,1,
  1,1,1,1,1,1,1,1,1,1,1,2,2,2,2,2,2,2,2,7,2,1,1,1,7,2,2,2,2,2,2,2,2,2,2,2,2,
  1,1,1,2,1,1,1,1,1,1,1,1,1,1,1,1,1,2,2,7,2,2,1,1,2,1,2,1,2,1,1,1,1,1,1,1,1,
  1,1,1,1,2,1,1,1,1,1,1,1,1,1,1,1,1,2,2,2,2,2,2,2,2,2,1,1,1,2,1,1,1,1,1,1,1,
  1,1,1,1,1,1,2,2,7,2,2,1,1,2,1,2,1,2,1,1,1,1,1,1,1,1,1,1,1,1,2,1,1,1,1,1,1,
  1,1,1,1,1,1,2,2,2,2,2,2,2,2,2,1,1,1,2,1,1,1,1,1,1,1,1,1,1,1,1,1,2,2,7,2,2,
  1,1,2,1,2,1,2,1,1,1,1,1,1,1,1,1,1,1,1,2,1,1,1,1,1,1,1,1,1,1,1,1,2,2,2,2,2,
  2,2,2,7,2,1,1,1,7,2,2,1,1,1,1,1,7,2,2,2,2,2,2,1,1,1,1,1,7,2,2,2,2,2,2,2,2,
  2,2,2,2,2,2,2,1,1,1,2,1,1,1,1,1,1,1,1,1,1,1,1,1,2,2,7,2,2,1,1,2,1,2,1,2,1,
  1,1,1,1,1,1,1,1,1,1,1,2,1,1,1,1,1,1,1,1,1,1,1,1,2,2,2,2,2,2,2,2,2,1,1,1,2,
  1,1,1,1,1,1,1,1,1,1,1,1,1,2,2,7,2,2,1,1,2,1,2,1,2,1,1,1,1,1,1,1,1,1,1,1,1,
  2,1,1,1,1,1,1,1,1,1,1,1,1,2,2,2,2,2,2,2,2,7,2,1,2,2,2,2,2,2,2,2,7,1,1,2,2,
  2,2,2,2,2,2,7,1,1,2,2,2,2,7,1,1,2,2,2,2,2,2,2,2,2,1,1,1,2,1,1,1,1,1,1,1,1,
  1,1,1,1,1,2,2,7,2,2,1,1,2,1,2,1,2,1,1,1,1,1,1,1,1,1,1,1,1,2,1,1,1,1,1,1,1,
  1,1,1,1,1,2,2,2,2,2,2,2,2,2,1,1,1,2,1,1,1,1,1,1,1,1,1,1,1,1,1,2,2,7,2,2,1,
  1,2,1,2,1,2,1,1,1,1,1,1,1,1,1,1,1,1,2,1,1,1,1,1,1,1,1,1,1,1,1,2,2,2,2,2,2,
  2,2,2,1,1,1,2,1,1,1,1,1,1,1,1,1,1,1,1,1,2,2,7,2,2,1,1,2,1,2,1,2,1,1,1,1,1,
  1,1,1,1,1,1,1,2,1,1,1,1,1,1,1,1,1,1,1,1,1,1,7,2,2,2,2,2,2,2,2,2,2,2,2,1,1,
  1,2,1,1,1,1,1,1,1,1,1,1,1,1,1,2,2,7,2,2,1,1,2,1,2,1,2,1,1,1,1,1,1,1,1,1,1,
  1,1,2,1,1,1,1,1,1,1,1,1,1,1,1,2,2,2,2,2,2,2,2,2,1,1,1,2,1,1,1,1,1,1,1,1,1,
  1,1,1,1,2,2,7,2,2,1,1,2,1,2,1,2,1,1,1,1,1,1,1,1,1,1,1,1,2,1,1,1,1,1,1,1,1,
  1,1,1,1,2,2,2,2,2,2,2,2,2,1,1,1,2,1,1,1,1,1,1,1,1,1,1,1,1,1,2,2,7,2,2,1,1,
  2,1,2,1,2,1,1,1,1,1,1,1,1,1,1,1,1,2,1,1,1,1,1,1,1,1,1,1,1,1,2,2,2,2,2,2,2,
  2,2,1,1,1,2,1,1,1,1,1,1,1,1,1,1,1,1,1,2,2,7,2,2,1,1,2,1,2,1,2,1,1,1,1,1,1,
  1,1,1,1,1,1,2,1,1,1,1,1,1,1,1,1,1,1,1,2,2,2,2,2,2,2,2,2,1,1,1,2,1,1,1,1,1,
  1,1,1,1,1,1,1,1,2,2,7,2,2,1,1,2,1,2,1,2,1,1,1,1,1,1,1,1,1,1,1,1,2,1,1,1,1,
  1,1,1,1,1,1,1,1,2,2,2,2,2,2,2,2,2,1,1,1,2,1,1,1,1,1,1,1,1,1,1,1,1,1,2,2,7,
  2,2,1,1,2,1,2,1,2,1,1,1,1,1,1,1,1,1,1,1,1,2,1,1,1,1,1,1,1,1,1,1,1,1,2,2,2,
  2,2,2,2,2,2,1,1,1,2,1,1,1,1,1,1,1,1,1,1,1,1,1,2,2,7,2,2,1,1,2,1,2,1,2,1,1,
  1,1,1,1,1,1,1,1,1,1,2,1,1,1,1,1,1,1,1,1,1,1,1,2,2,2,2,2,2,2,2,2,1,1,1,2,1,
  1,1,1,1,1,1,1,1,1,1,1,1,2,2,7,2,2,1,1,2,1,2,1,2,1,1,1,1,1,1,1,1,1,1,1,1,2,
  1,1,1,1,1,1,1,1,1,1,1,1,2,2,2,2,2,2,2,2,2,1,1,1,2,1,1,1,1,1,1,1,1,1,1,1,1,
  1,2,2,7,2,2,1,1,2,1,2,1,2,1,1,1,1,1,1,1,1,1,1,1,1,2,1,1,1,1,1,1,1,1,1,1,1,
  1,2,2,2,2,2,2,2,2,2,1,1,1,2,1,1,1,1,1,1,1,1,1,1,1,1,1,2,2,7,2,2,1,1,2,1,2,
  1,2,1,1,1,1,1,1,1,1,1,1,1,1,2,1,1,1,1,1,1,1,1,1,1,1,1,2,2,2,2,2,2,2,2,2,1,
  1,1,2,1,1,1,1,1,1,1,1,1,1,1,1,1,2,2,7,2,2,1,1,2,1,2,1,2,1,1,1,1,1,1,1,1,1,
  1,1,1,2,1,1,1,1,1,1,1,1,1,1,1,1,2,2,2,2,2,2,2,2,2,1,1,1,2,1,1,1,1,1,1,1,1,
  1,1,1,1,1,2,2,7,2,2,1,1,2,1,2,1,2,1,1,1,1,1,1,1,1,1,1,1,1,2,1,1,1,1,1,1,1,
  1,1,1,1,1,2,2,2,2,2,2,2,2,2,1,1,1,2,1,1,1,1,1,1,1,1,1,1,1,1,1,2,2,7,2,2,1,
  1,2,1,2,1,2,1,1,1,1,1,1,1,1,1,1,1,1,2,1,1,1,1,1,1,1,1,1,1,1,1,2,2,2,2,2,2,
  2,2,2,1,1,1,2,1,1,1,1,1,1,1,1,1,1,1,1,1,2,2,7,2,2,1,1,2,1,2,1,2,1,1,1,1,1,
  1,1,1,1,1,1,1,2,1,1,1,1,1,1,1,1,1,1,1,1,2,2,2,2,7,2,1,2,2,2,2,2,2,2,2,7,2,
  1,2,2,2,2,2,2,2,2,2,1,1,1,2,1,1,1,1,1,1,1,1,1,1,1,1,1,2,2,7,2,2,1,1,2,1,2,
  1,2,1,1,1,1,1,1,1,1,1,1,1,1,2,1,1,1,1,1,1,1,1,1,1,1,1,2,2,2,2,7,2,1,2,2,2,
  2,2,2,2,2,7,2,1,2,2,2,2,7,2,1,2,2,2,2,2,2,2,2,2,1,1,1,2,1,1,1,1,1,1,1,1,1,
  1,1,1,1,2,2,7,2,2,1,1,2,1,2,1,2,1,1,1,1,1,1,1,1,1,1,1,1,2,1,1,1,1,1,1,1,1,
  1,1,1,1,2,2,2,2,2,2,2,2,2,1,1,1,2,1,1,1,1,1,1,1,1,1,1,1,1,1,2,2,7,2,2,1,1,
  2,1,2,1,2,1,1,1,1,1,1,1,1,1,1,1,1,2,1,1,1,1,1,1,1,1,1,1,1,1,2,2,2,2,2,2,2,
  2,2,1,1,1,2,1,1,1,1,1,1,1,1,1,1,1,1,1,2,2,7,2,2,1,1,2,1,2,1,2,1,1,1,1,1,1,
  1,1,1,1,1,1,2,1,1,1,1,1,1,1,1,1,1,1,1,2,2,2,2,2,2,2,2,2,1,1,1,2,1,1,1,1,1,
  1,1,1,1,1,1,1,1,2,2,7,2,2,1,1,2,1,2,1,2,1,1,1,1,1,1,1,1,1,1,1,1,2,1,1,1,1,
  1,1,1,1,1,1,1,1,2,2,2,2,2,2,2,2,2,1,1,1,2,1,1,1,1,1,1,1,1,1,1,1,1,1,2,2,7,
  2,2,1,1,2,1,2,1,2,1,1,1,1,1,1,1,1,1,1,1,1,2,1,1,1,1,1,1,1,1,1,1,1,1,2,2,2,
  2,2,2,2,2,2,1,1,1,2,1,1,1,1,1,1,1,1,1,1,1,1,1,2,2,7,2,2,1,1,2,1,2,1,2,1,1,
  1,1,1,1,1,1,1,1,1,1,2,1,1,1,1,1,1,1,1,1,1,1,1,2,2,2,2,2,2,2,2,2,1,1,1,2,1,
  1,1,1,1,1,1,1,1,1,1,1,1,2,2,7,2,2,1,1,2,1,2,1,2,1,1,1,1,1,1,1,1,1,1,1,1,2,
  1,1,1,1,1,1,1,1,1,1,1,1,2,2,2,2,2,2,2,2,7,2,1,2,2,2,2,2,2,2,2,2,1,1,1,2,1,
  1,1,1,1,1,1,1,1,1,1,1,1,2,2,7,2,2,1,1,2,1,2,1,2,1,1,1,1,1,1,1,1,1,1,1,1,2,
  1,1,1,1,1,1,1,1,1,1,1,1,2,2,2,2,2,2,2,2,2,1,1,1,2,1,1,1,1,1,1,1,1,1,1,1,1,
  1,2,2,7,2,2,1,1,2,1,2,1,2,1,1,1,1,1,1,1,1,1,1,1,1,2,1,1,1,1,1,1,1,1,1,1,1,
  1,2,2,2,2,2,2,2,2,2,1,1,1,2,1,1,1,1,1,1,1,1,1,1,1,1,1,2,2,7,2,2,1,1,2,1,2,
  1,2,1,1,1,1,1,1,1,1,1,1,1,1,2,1,1,1,1,1,1,1,1,1,1,1,1,2,2,2,2,2,2,2,2,2,1,
  1,1,2,1,1,1,1,1,1,1,1,1,1,1,1,1,2,2,7,2,2,1,1,2,1,2,1,2,1,1,1,1,1,1,1,1,1,
  1,1,1,2,1,1,1,1,1,1,1,1,1,1,1,1,2,2,2,2,2,2,2,2,7,2,1,2,2,2,2,2,2,2,2,2,1,
  1,1,2,1,1,1,1,1,1,1,1,1,1,1,1,1,2,2,7,2,2,1,1,2,1,2,1,2,1,1,1,1,1,1,1,1,1,
  1,1,1,2,1,1,1,1,1,1,1,1,1,1,1,1,2,2,2,2,2,2,2,2,7,2,1,2,2,2,2,2,2,2,2,7,2,
  1,2,2,2,2,2,2,2,2,2,1,1,1,2,1,1,1,1,1,1,1,1,1,1,1,1,1,2,2,7,2,2,1,1,2,1,2,
  1,2,1,1,1,1,1,1,1,1,1,1,1,1,2,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,
  1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,
  1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,
  1,1,1,1,1,1,1,1,5,3,3,1,1,1,1,1,1,2,1,1,1,2,2,1,1,1,1,1,1,1,1,2,1,1,1,2,2,
  2,2,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,2,1,1,1,2,1,1,1,2,1,1,2,2,2,
  2,2,2,2,2,2,2,2,2,2,2,1,2,1,1,1,2,2,2,1,1,2,1,1,2,1,1,2,2,2,1,1,1,1,2,9,7,
  2,1,1,2,1,1,2,1,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,
  2,2,2,7,1,2,1,2,2,2,2,2,2,2,2,2,2,2,2,3,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,
  2,2,2,7,2,5,5,5,5,5,1,5,7,1,1,1,3,2,2,2,7,1,4,2,2,2,7,2,2,2,7,2,1,2,2,2,2,
  2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,7,1,10,10,10,5,10,
  10,10,10,2,7,10,10,10,10,4,9,2,10,10,10,2,9,2,2,2,2,4,5,1,1,7,1,1,1,3,5,1,
  1,7,1,1,1,3,5,1,1,7,1,1,1,3,5,1,1,7,1,1,1,3,5,1,1,7,1,1,1,3,5,1,1,7,1,1,1,
  3,5,1,1,7,1,1,1,3,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,
  5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,
  5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,
  5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,1,5,7,1,1,1,3,5,5,5,5,
  5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,
  5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,
  5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,
  5,5,5,5,5,5,5,5,5,5,5,5,5,1,1,7,1,1,1,3,1,7,1,1,7,1,1,7,1,1,7,1,1,7,1,1,7,
  1,1,7,1,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,1,1,7,1,1,1,3,2,
  2,2,2,2,2,2,2,2,1,1,1,2,1,1,1,1,1,1,1,1,1,1,1,1,1,2,2,7,2,2,1,1,2,1,2,1,1,
  1,1,1,1,1,1,1,1,1,1,1,1,2,1,1,1,1,1,1,1,1,1,1,1,1,2,2,2,2,2,2,2,2,2,1,1,1,
  2,1,1,1,1,1,1,1,1,1,1,2,2,7,2,2,1,1,2,1,2,1,1,2,2,2,1,1,1,1,1,1,1,1,1,1,1,
  2,2,2,2,2,2,2,2,2,1,1,1,2,1,1,1,1,1,1,1,1,1,1,2,2,7,2,2,1,1,2,1,2,1,1,2,2,
  2,1,1,1,1,1,1,1,1,1,1,1,2,2,2,2,2,2,2,2,2,1,1,1,2,1,1,1,1,1,1,1,1,1,1,2,2,
  7,2,2,1,1,2,1,2,1,1,2,2,2,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,5,1,1,1,1,1,1,1,
  5,1,1,1,1,1,1,5,1,1,1,1,1,1,5,1,1,1,1,5,1,1,1,1,5,1,1,1,1,1,1,1,1,1,1,1,1,
  1,1,1,4,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,1,1,1,2,1,
  1,1,1,1,1,1,1,1,1,1,1,1,2,2,7,2,2,1,1,2,1,2,1,2,1,1,1,1,1,1,1,1,1,1,1,1,2,
  1,1,1,1,1,1,1,1,1,1,1,1,5,5,5,5,1,7,1,2,8,4,4,4,4,1,7,1,1,8,4,4,4,4,1,7,1,
  1,8,4,4,4,4,1,7,1,1,2,1,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,
  2,2,2,2,2,2,2,7,1,8,5,5,5,5,1,7,1,1,8,5,5,5,5,1,7,1,1,8,5,5,5,5,1,7,1,1,8,
  5,5,5,5,1,7,1,1,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,
  2,2,3,2,2,2,7,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,
  2,3,2,2,2,7,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,
  5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,
  5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,1,5,7,1,1,1,3,5,
  5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,
  5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,
  5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,1,5,7,1,1,1,3,5,5,5,5,5,5,5,5,5,
  5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,
  5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,
  5,5,5,5,5,5,5,5,5,5,5,5,5,1,5,7,1,1,1,3,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,
  5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,
  5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,
  5,5,5,5,5,1,5,7,1,1,1,3,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,
  5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,
  5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,1,5,7,
  1,1,1,3,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,
  5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,
  5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,1,5,7,1,1,1,3,8,1,7,1,
  1,8,1,7,1,1,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,
  5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,
  5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,1,5,7,1,1,1,3,8,1,
  7,1,1,9,2,10,10,10,9,2,2,2,2,4,2,2,2,2,2,2,7,2,1,2,2,2,2,2,2,2,2,2,2,2,2,2,
  2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,7,2,2,2,2,2,7,2,7,2,2,2,2,2,2,2,2,2,
  1,1,1,2,1,1,1,1,1,1,1,1,1,1,1,1,1,2,2,7,2,2,1,1,2,1,2,1,1,1,1,1,1,1,1,1,1,
  1,1,1,1,2,1,1,1,1,1,1,1,1,1,1,1,1,2,2,2,2,2,2,2,2,2,1,1,1,2,1,1,1,1,1,1,1,
  1,1,1,1,1,1,2,2,7,2,2,1,1,2,1,2,1,1,1,1,1,1,1,1,1,1,1,1,1,1,2,1,1,1,1,1,1,
  1,1,1,1,1,1,2,2,2,2,2,2,2,2,2,1,1,1,2,1,1,1,1,1,1,1,1,1,1,1,1,1,2,2,7,2,2,
  1,1,2,1,2,1,1,1,1,1,1,1,1,1,1,1,1,1,1,2,1,1,1,1,1,1,1,1,1,1,1,1,2,2,2,2,2,
  2,2,2,2,1,1,1,2,1,1,1,1,1,1,1,1,1,1,1,1,1,2,2,7,2,2,1,1,2,1,2,1,1,1,1,1,1,
  1,1,1,1,1,1,1,1,2,1,1,1,1,1,1,1,1,1,1,1,1,2,2,2,2,2,2,2,2,2,1,1,1,2,1,1,1,
  1,1,1,1,1,1,1,1,1,1,2,2,7,2,2,1,1,2,1,2,1,1,1,1,1,1,1,1,1,1,1,1,1,1,2,1,1,
  1,1,1,1,1,1,1,1,1,1,2,2,2,2,2,2,2,2,2,1,1,1,2,1,1,1,1,1,1,1,1,1,1,1,1,1,2,
  2,7,2,2,1,1,2,1,2,1,1,1,1,1,1,1,1,1,1,1,1,1,1,2,1,1,1,1,1,1,1,1,1,1,1,1,2,
  2,2,2,2,2,2,2,2,1,1,1,2,1,1,1,1,1,1,1,1,1,1,1,1,1,2,2,7,2,2,1,1,2,1,2,1,1,
  1,1,1,1,1,1,1,1,1,1,1,1,2,1,1,1,1,1,1,1,1,1,1,1,1,1,7,1,5,5,5,5,5,5,5,5,5,
  5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,1,5,7,1,1,1,3,8,8,8,8,8,8,8,8,8,8,8,8,8,8,
  8,8,8,8,8,8,8,8,8,8,8,1,7,1,1,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,
  5,5,5,1,5,7,1,1,1,3,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,1,7,
  1,1,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,1,7,1,1,8,8,8,8,8,8,
  8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,1,7,1,1,8,8,8,8,8,8,8,8,8,8,8,8,8,8,
  8,8,8,8,8,8,8,8,8,8,8,1,7,1,1,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,
  8,8,8,8,8,8,1,7,1,1,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,
  8,1,7,1,1,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,1,5,7,1,
  1,1,3,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,1,7,1,1,5,5,
  5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,1,5,7,1,1,1,3,8,8,8,8,
  8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,1,7,1,1,5,5,5,5,5,5,5,5,5,
  5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,1,5,7,1,1,1,3,8,8,8,8,8,8,8,8,8,8,8,
  8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,1,7,1,1,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,
  5,5,5,5,5,5,5,5,5,5,5,5,1,5,7,1,1,1,3,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,
  8,8,8,8,8,8,8,8,8,8,1,7,1,1,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,
  5,5,5,5,5,1,5,7,1,1,1,3,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,
  8,8,8,1,7,1,1,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,1,7,
  1,1,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,1,5,7,1,1,1,3,
  8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,1,7,1,1,8,8,8,8,8,
  8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,1,7,1,1,5,5,5,5,5,5,5,5,5,5,
  5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,1,5,7,1,1,1,3,8,8,8,8,8,8,8,8,8,8,8,8,
  8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,1,7,1,1,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,
  8,8,8,8,8,8,8,8,8,8,8,1,7,1,1,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,
  5,5,5,5,5,5,1,1,7,1,1,1,3,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,
  5,5,5,5,1,1,7,1,1,1,3,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,
  5,5,1,1,7,1,1,1,3,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,
  1,1,7,1,1,1,3,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,1,1,
  7,1,1,1,3,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,1,1,7,1,
  1,1,3,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,1,1,7,1,1,1,
  3,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,1,1,7,1,1,1,3,5,
  5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,1,1,7,1,1,1,3,5,5,5,
  5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,1,1,7,1,1,1,3,5,5,5,5,5,
  5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,1,1,7,1,1,1,3,5,5,5,5,5,5,5,
  5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,1,1,7,1,1,1,3,5,5,5,5,5,5,5,5,5,
  5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,1,1,7,1,1,1,3,1,7,1,2,1,2,2,2,2,2,2,
  2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,7,2,1,4,1,1,4,1,1,4,
  1,1,4,1,1,7,1,1,7,1,1,7,1,1,7,2,1,7,2,1,7,2,1,7,2,1,7,2,1,7,2,1,7,2,5,5,5,
  5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,
  5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,
  5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,
  5,5,5,5,5,5,5,5,5,5,5,5,5,5,1,7,1,3,2,2,2,2,2,2,2,2,2,1,1,1,2,1,1,1,1,1,1,
  1,1,1,1,2,2,7,2,2,1,1,2,1,2,1,1,2,2,2,1,1,1,1,1,1,1,1,1,1,1,2,2,2,2,2,2,2,
  2,2,1,1,1,2,1,1,1,1,1,1,1,1,1,1,2,2,7,2,2,1,1,2,1,2,1,1,2,2,2,1,1,1,1,1,1,
  1,1,1,1,1,2,2,2,2,2,2,2,2,2,1,1,1,2,1,1,1,1,1,1,1,1,1,1,2,2,7,2,2,1,1,2,1,
  2,1,1,2,2,2,1,1,1,1,1,1,1,1,1,1,1,2,2,2,2,2,2,2,2,2,1,1,1,2,1,1,1,1,1,1,1,
  1,1,1,2,2,7,2,2,1,1,2,1,2,1,1,2,2,2,1,1,1,1,1,1,1,1,1,1,1,2,2,2,2,2,2,2,2,
  2,1,1,1,2,1,1,1,1,1,1,1,1,1,1,2,2,7,2,2,1,1,2,1,2,1,1,2,2,2,1,1,1,1,1,1,1,
  1,1,1,1,2,2,2,2,2,2,2,2,2,1,1,1,2,1,1,1,1,1,1,1,1,1,1,1,1,1,2,2,7,2,2,1,1,
  2,1,2,1,1,1,1,1,1,1,1,2,1,1,1,1,1,1,1,1,1,1,1,1,2,2,2,2,2,2,2,2,2,1,1,1,2,
  1,1,1,1,1,1,1,1,1,1,1,1,1,2,2,7,2,2,1,1,2,1,2,1,1,1,1,1,1,1,1,2,1,1,1,1,1,
  1,1,1,1,1,1,1,2,2,2,2,2,2,2,2,2,1,1,1,2,1,1,1,1,1,1,1,1,1,1,1,1,1,2,2,7,2,
  2,1,1,2,1,2,1,1,1,1,1,1,1,1,1,2,1,1,1,1,1,1,1,1,1,1,1,1,2,2,2,2,2,2,2,2,2,
  1,1,1,2,1,1,1,1,1,1,1,1,1,1,1,1,1,2,2,7,2,2,1,1,2,1,2,1,1,1,1,1,1,1,1,1,2,
  1,1,1,1,1,1,1,1,1,1,1,1,2,2,2,2,2,2,2,2,2,1,1,1,2,1,1,1,1,1,1,1,1,1,1,1,1,
  1,2,2,7,2,2,1,1,2,1,2,1,1,1,1,1,1,1,1,1,2,1,1,1,1,1,1,1,1,1,1,1,1,2,2,2,2,
  2,2,2,2,2,1,1,1,2,1,1,1,1,1,1,1,1,1,1,1,1,1,2,2,7,2,2,1,1,2,1,2,1,1,1,1,1,
  1,1,1,1,2,1,1,1,1,1,1,1,1,1,1,1,1,2,2,2,2,2,2,2,2,2,1,1,1,2,1,1,1,1,1,1,1,
  1,1,1,1,1,1,2,2,7,2,2,1,1,2,1,2,1,1,1,1,1,1,1,1,1,1,2,1,1,1,1,1,1,1,1,1,1,
  1,1,2,2,2,2,2,2,2,2,2,1,1,1,2,1,1,1,1,1,1,1,1,1,1,1,1,1,2,2,7,2,2,1,1,2,1,
  2,1,1,1,1,1,1,1,1,1,1,2,1,1,1,1,1,1,1,1,1,1,1,1,2,2,2,2,2,2,2,2,2,1,1,1,2,
  1,1,1,1,1,1,1,1,1,1,1,1,1,2,2,7,2,2,1,1,2,1,2,1,1,1,1,1,1,1,1,1,1,1,2,1,1,
  1,1,1,1,1,1,1,1,1,1,2,2,2,2,2,2,2,2,2,1,1,1,2,1,1,1,1,1,1,1,1,1,1,1,1,1,2,
  2,7,2,2,1,1,2,1,2,1,1,1,1,1,1,1,1,1,1,1,2,1,1,1,1,1,1,1,1,1,1,1,1,2,2,2,2,
  2,2,2,2,2,1,1,1,2,1,1,1,1,1,1,1,1,1,1,1,1,1,2,2,7,2,2,1,1,2,1,2,1,1,1,1,1,
  1,1,1,1,1,1,1,2,1,1,1,1,1,1,1,1,1,1,1,1,2,2,2,2,2,2,2,2,2,1,1,1,2,1,1,1,1,
  1,1,1,1,1,1,1,1,1,2,2,7,2,2,1,1,2,1,2,1,1,1,1,1,1,1,1,1,1,1,1,2,1,1,1,1,1,
  1,1,1,1,1,1,1,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,
  5,5,5,5,5,5,5,1,5,7,1,1,1,3,8,8,8,8,8,8,8,8,8,1,7,1,1,8,8,8,8,8,8,8,8,8,8,
  8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,1,7,1,1,8,8,8,8,8,8,8,8,8,8,8,8,8,8,
  8,8,8,8,8,8,8,8,8,8,8,8,8,8,1,7,1,1,8,8,8,8,8,8,8,8,1,7,1,1,8,8,8,8,8,8,8,
  8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,1,7,1,1,1,1,1,1,1,4,1,1,1,1,1,1,
  1,1,1,1,4,1,1,1,1,1,1,1,4,1,1,1,1,4,1,1,1,1,4,1,1,1,1,4,1,1,1,1,1,1,4,1,1,
  1,1,1,1,1,1,4,1,1,1,1,1,1,4,1,1,1,1,4,1,1,1,1,4,1,1,1,1,4,1,1,2,2,2,2,2,2,
  2,2,2,7,2,1,1,2,2,2,2,2,2,2,2,2,1,1,1,2,2,1,1,1,1,1,1,1,1,1,1,1,1,1,2,2,7,
  2,2,1,1,2,1,2,1,2,1,1,2,2,1,1,1,1,1,1,1,1,1,1,1,1,2,1,1,1,1,1,1,1,1,1,1,1,
  1,1,1,2,2,2,2,2,2,2,2,2,1,1,1,2,1,1,1,1,1,1,1,1,1,1,1,1,1,2,2,7,2,2,1,1,2,
  1,2,1,2,1,1,1,1,1,1,1,1,1,1,1,1,2,1,1,1,1,1,1,1,1,1,1,1,1,2,2,2,2,2,2,2,2,
  7,2,1,2,2,2,2,2,2,2,2,2,1,1,1,2,1,1,1,1,1,1,1,1,1,1,1,1,1,2,2,7,2,2,1,1,2,
  1,2,1,2,1,1,1,1,1,1,1,1,1,1,1,1,2,1,1,1,1,1,1,1,1,1,1,1,1
};


static const unsigned short ag_pstt[] = {
3,3,3,3,3,3,3,3,3,3,3,3,3,3,3,3,3,3,3,3,3,3,3,3,3,3,3,3,3,3,3,3,3,3,3,3,3,3,
  3,3,3,3,3,3,3,3,3,3,3,3,3,3,3,3,3,3,3,3,3,3,3,3,3,3,3,3,3,3,3,3,3,3,3,3,
  3,3,3,3,3,3,3,3,3,3,3,3,3,3,3,3,3,3,3,3,3,3,3,3,3,3,3,3,3,3,3,3,3,3,3,3,
  3,3,3,3,3,3,3,3,3,3,3,3,3,3,3,3,3,3,3,3,3,3,3,3,1,3,0,2,2,2,3,
4,4,4,4,4,4,4,4,4,4,4,4,4,4,4,4,4,4,4,4,4,4,4,4,4,4,4,4,4,4,4,4,4,4,4,1,4,
1,541,543,541,541,
6,6,6,6,6,6,6,6,6,6,6,6,6,6,6,6,6,6,6,6,6,6,6,6,6,6,6,6,6,6,6,6,6,6,6,6,6,6,
  6,6,6,6,6,6,6,6,6,6,6,6,6,6,6,6,6,6,6,6,6,6,6,6,6,6,6,6,6,6,6,6,6,6,6,6,
  6,6,6,6,6,6,6,6,6,6,6,6,6,6,6,6,6,6,6,6,6,6,6,6,6,6,6,6,6,6,6,6,6,6,6,6,
  6,6,6,6,6,6,6,6,6,6,6,6,6,6,6,6,6,6,6,6,6,6,6,6,6,5,3,0,7,7,7,5,6,
13,13,13,13,13,13,13,13,13,13,13,13,13,13,13,13,13,13,13,13,13,13,13,13,13,
  13,13,13,13,13,13,13,13,13,14,13,4,
16,18,
82,82,82,82,82,82,82,82,25,25,25,25,25,25,25,25,25,25,25,25,25,25,25,25,25,
  25,25,25,25,25,25,25,25,25,25,25,25,25,25,25,25,25,25,25,25,25,25,25,25,
  25,25,25,25,25,25,25,25,25,25,25,25,25,25,25,25,25,25,25,25,25,25,25,25,
  25,25,25,25,25,25,25,25,25,25,25,25,25,25,25,25,25,25,25,25,25,25,25,25,
  25,25,25,25,25,25,25,25,25,25,25,25,25,25,25,25,25,25,25,25,25,25,25,15,
  16,17,18,19,20,21,22,23,22,8,9,10,81,1,6,23,26,34,21,13,22,12,11,35,33,
  32,31,30,29,34,35,28,27,25,14,24,14,
6,6,6,6,6,6,6,6,6,6,6,6,6,6,6,6,6,6,6,6,6,6,6,6,6,6,6,6,6,6,6,6,6,6,6,6,6,6,
  6,6,6,6,6,6,6,6,6,6,6,6,6,6,6,6,6,6,6,6,6,6,6,6,6,6,6,6,6,6,6,6,6,6,6,6,
  6,6,6,6,6,6,6,6,6,6,6,6,6,6,6,6,6,6,6,6,6,6,6,6,6,6,6,6,6,6,6,6,6,6,6,6,
  6,6,6,6,6,6,6,6,6,6,6,6,6,6,6,6,6,6,6,6,6,6,6,6,6,4,5,7,3,3,5,6,
542,542,542,542,542,542,542,542,542,542,542,542,542,542,542,542,542,542,542,
  542,542,542,542,542,542,542,542,542,542,542,542,542,542,542,542,542,542,
  542,542,542,542,542,542,542,1,542,8,2,2,2,546,
542,542,542,542,542,542,542,542,542,542,542,542,542,542,542,542,542,542,542,
  542,542,542,542,542,542,542,542,542,542,542,542,542,542,542,1,542,9,2,2,
  2,545,
542,542,542,542,542,542,542,542,542,542,542,542,542,542,542,542,542,542,542,
  542,542,542,542,542,542,542,542,542,542,542,542,542,542,542,1,542,10,2,
  2,2,544,
36,39,41,42,43,44,45,46,47,48,49,50,51,52,54,60,63,64,66,59,58,37,65,62,53,
  57,55,56,61,40,38,68,8,9,67,11,67,68,67,67,67,67,67,67,67,67,67,67,67,
  67,67,67,67,67,67,67,67,67,67,67,67,67,67,67,67,67,67,67,67,67,67,
36,39,41,42,43,44,45,46,47,48,49,50,51,52,54,60,63,64,66,59,58,37,65,62,53,
  57,55,56,61,40,38,69,8,9,67,12,67,69,67,67,67,67,67,67,67,67,67,67,67,
  67,67,67,67,67,67,67,67,67,67,67,67,67,67,67,67,67,67,67,67,67,67,
36,39,41,42,43,44,45,46,47,48,49,50,51,52,54,60,63,64,66,59,58,37,65,62,53,
  57,55,56,61,40,38,70,8,9,67,13,67,70,67,67,67,67,67,67,67,67,67,67,67,
  67,67,67,67,67,67,67,67,67,67,67,67,67,67,67,67,67,67,67,67,67,67,
83,83,83,83,83,83,83,84,84,84,83,100,1,2,14,2,84,2,2,557,
542,542,542,542,542,542,542,542,542,1,2,15,2,2,2,563,
542,542,542,542,542,542,542,542,542,1,2,16,2,2,2,562,
542,542,542,542,1,2,17,2,2,2,561,
542,542,542,542,1,2,18,2,2,2,560,
542,542,542,542,542,542,542,542,542,1,2,19,2,2,2,559,
542,542,542,542,542,542,542,542,542,1,2,20,2,2,2,558,
542,542,1,2,21,2,2,2,555,
542,542,1,2,22,2,2,2,553,
542,542,542,542,542,542,542,542,542,542,542,542,542,542,542,1,2,23,2,2,2,
  550,
542,542,542,542,542,542,542,542,542,542,542,542,542,542,542,542,542,542,542,
  542,542,542,542,542,542,542,542,542,542,542,542,542,542,542,542,542,542,
  542,542,542,542,542,542,542,542,542,542,542,542,542,542,542,542,542,542,
  542,542,542,542,542,542,542,542,542,542,542,542,542,542,542,542,542,542,
  542,542,542,542,542,542,542,542,542,542,542,542,542,542,542,542,542,542,
  542,542,542,542,542,542,542,542,542,542,542,542,542,542,542,542,542,542,
  542,542,542,542,542,542,542,542,542,1,2,24,2,2,2,548,
71,72,73,74,75,76,77,78,79,80,81,82,83,84,85,86,87,88,89,90,91,92,93,94,95,
  96,97,98,99,100,101,102,103,104,105,106,107,108,109,110,111,112,113,114,
  115,116,117,118,119,120,121,122,123,124,125,126,127,128,129,130,131,132,
  133,134,135,136,137,138,139,140,141,142,143,144,145,146,147,148,149,150,
  151,152,153,154,155,156,157,158,159,160,161,222,224,226,227,229,231,232,
  233,235,237,239,241,243,245,247,249,251,253,254,255,256,25,257,45,46,47,
  252,250,248,246,244,242,240,238,236,234,62,63,230,228,68,225,223,221,
  221,220,219,218,217,216,215,223,214,213,212,227,228,211,210,209,208,207,
  206,205,204,237,203,202,201,241,242,243,244,200,199,198,197,196,195,194,
  193,192,191,190,189,188,187,186,185,184,183,182,181,180,266,179,178,177,
  270,176,175,174,274,173,172,277,278,279,280,281,282,283,284,285,286,287,
  288,289,290,171,292,170,169,168,296,297,298,167,166,301,165,164,304,163,
  162,307,
247,249,251,261,263,26,264,262,260,259,258,
82,82,82,82,82,82,82,82,81,27,37,265,265,
82,82,82,82,82,82,82,82,81,28,36,265,265,
82,82,82,82,82,82,82,82,81,29,33,265,265,
82,82,82,82,82,82,82,82,81,30,32,265,265,
266,267,31,30,31,
96,91,32,28,270,29,268,271,269,
85,85,85,85,85,85,85,85,96,85,85,85,85,91,85,33,25,270,26,268,27,271,269,
  272,
43,43,43,43,43,43,43,43,43,43,43,43,43,43,43,43,43,43,43,43,43,43,43,43,43,
  43,43,43,43,43,43,43,43,43,43,43,43,43,43,43,43,43,43,43,43,43,43,43,43,
  43,43,43,43,43,43,43,43,43,43,43,43,43,43,43,43,43,43,43,43,43,43,43,43,
  43,43,43,43,43,43,43,43,43,43,43,43,43,43,43,43,43,43,43,43,43,43,43,43,
  43,43,43,43,43,43,43,43,43,43,43,43,43,43,43,261,263,24,24,24,24,34,274,
  273,
275,8,9,10,35,20,13,12,11,19,
542,542,542,542,542,542,542,542,542,542,542,542,542,542,542,542,542,542,542,
  542,542,542,542,542,542,542,542,542,542,542,542,542,542,542,1,542,36,2,
  2,2,749,
542,542,542,542,542,542,542,542,542,542,542,542,542,542,542,542,542,542,542,
  542,542,542,542,542,542,542,542,542,542,542,542,542,542,542,542,542,542,
  542,542,542,542,542,542,1,542,37,2,2,2,622,
542,542,542,542,542,542,542,542,542,542,542,542,542,542,542,542,542,542,542,
  542,542,542,542,542,542,542,542,542,542,542,542,542,542,542,542,542,542,
  542,542,542,542,542,542,542,1,542,38,2,2,2,603,
542,542,542,542,542,542,542,542,542,542,542,542,542,542,542,542,542,542,542,
  542,542,542,542,542,542,542,542,542,542,542,542,542,542,542,1,542,39,2,
  2,2,748,
542,542,542,542,542,542,542,542,542,542,542,542,542,542,542,542,542,542,542,
  542,542,542,542,542,542,542,542,542,542,542,542,542,542,542,542,542,542,
  542,542,542,542,542,542,542,1,542,40,2,2,2,605,
542,542,542,542,542,542,542,542,542,542,542,542,542,542,542,542,542,542,542,
  542,542,542,542,542,542,542,542,542,542,542,542,542,542,542,1,542,41,2,
  2,2,747,
542,542,542,542,542,542,542,542,542,542,542,542,542,542,542,542,542,542,542,
  542,542,542,542,542,542,542,542,542,542,542,542,542,542,542,1,542,42,2,
  2,2,746,
542,542,542,542,542,542,542,542,542,542,542,542,542,542,542,542,542,542,542,
  542,542,542,542,542,542,542,542,542,542,542,542,542,542,542,1,542,43,2,
  2,2,745,
542,542,542,542,542,542,542,542,542,542,542,542,542,542,542,542,542,542,542,
  542,542,542,542,542,542,542,542,542,542,542,542,542,542,542,1,542,44,2,
  2,2,744,
542,542,542,542,542,542,542,542,542,542,542,542,542,542,542,542,542,542,542,
  542,542,542,542,542,542,542,542,542,542,542,542,542,542,542,1,542,45,2,
  2,2,743,
542,542,542,542,542,542,542,542,542,542,542,542,542,542,542,542,542,542,542,
  542,542,542,542,542,542,542,542,542,542,542,542,542,542,542,1,542,46,2,
  2,2,742,
542,542,542,542,542,542,542,542,542,542,542,542,542,542,542,542,542,542,542,
  542,542,542,542,542,542,542,542,542,542,542,542,542,542,542,1,542,47,2,
  2,2,741,
542,542,542,542,542,542,542,542,542,542,542,542,542,542,542,542,542,542,542,
  542,542,542,542,542,542,542,542,542,542,542,542,542,542,542,1,542,48,2,
  2,2,740,
542,542,542,542,542,542,542,542,542,542,542,542,542,542,542,542,542,542,542,
  542,542,542,542,542,542,542,542,542,542,542,542,542,542,542,1,542,49,2,
  2,2,739,
542,542,542,542,542,542,542,542,542,542,542,542,542,542,542,542,542,542,542,
  542,542,542,542,542,542,542,542,542,542,542,542,542,542,542,1,542,50,2,
  2,2,738,
542,542,542,542,542,542,542,542,542,542,542,542,542,542,542,542,542,542,542,
  542,542,542,542,542,542,542,542,542,542,542,542,542,542,542,542,542,542,
  542,542,542,542,542,542,542,542,542,542,542,542,542,542,542,542,542,542,
  542,542,542,542,542,542,542,542,542,542,542,542,542,542,542,542,542,542,
  542,542,542,542,542,542,542,542,542,542,542,542,542,542,542,542,542,542,
  542,542,542,542,542,542,542,542,542,542,542,542,542,542,542,542,542,542,
  542,542,542,542,542,542,542,542,542,542,542,542,542,542,542,542,542,1,
  542,51,2,2,2,737,
542,542,542,542,542,542,542,542,542,542,542,542,542,542,542,542,542,542,542,
  542,542,542,542,542,542,542,542,542,542,542,542,542,542,542,542,542,542,
  542,542,542,542,542,542,542,542,542,542,542,542,542,542,542,542,542,542,
  542,542,542,542,542,542,542,542,542,542,542,542,542,542,542,542,542,542,
  542,542,542,542,542,542,542,542,542,542,542,542,542,542,542,542,542,542,
  542,542,542,542,542,542,542,542,542,542,542,542,542,542,542,542,542,542,
  542,542,542,542,542,542,542,542,542,542,542,542,542,542,542,542,542,1,
  542,52,2,2,2,736,
542,542,542,542,542,542,542,542,542,542,542,542,542,542,542,542,542,542,542,
  542,542,542,542,542,542,542,542,542,542,542,542,542,542,542,542,542,542,
  542,542,542,542,542,542,1,542,53,2,2,2,616,
542,542,542,542,542,542,542,542,542,542,542,542,542,542,542,542,542,542,542,
  542,542,542,542,542,542,542,542,542,542,542,542,542,542,542,1,542,54,2,
  2,2,735,
542,542,542,542,542,542,542,542,542,542,542,542,542,542,542,542,542,542,542,
  542,542,542,542,542,542,542,542,542,542,542,542,542,542,542,542,542,542,
  542,542,542,542,542,542,542,1,542,55,2,2,2,614,
542,542,542,542,542,542,542,542,542,542,542,542,542,542,542,542,542,542,542,
  542,542,542,542,542,542,542,542,542,542,542,542,542,542,542,542,542,542,
  542,542,542,542,542,542,542,1,542,56,2,2,2,613,
542,542,542,542,542,542,542,542,542,542,542,542,542,542,542,542,542,542,542,
  542,542,542,542,542,542,542,542,542,542,542,542,542,542,542,542,542,542,
  542,542,542,542,542,542,1,542,57,2,2,2,615,
542,542,542,542,542,542,542,542,542,542,542,542,542,542,542,542,542,542,542,
  542,542,542,542,542,542,542,542,542,542,542,542,542,542,542,542,542,542,
  542,542,542,542,542,542,542,542,542,542,542,542,542,542,542,542,542,542,
  542,542,542,542,542,542,542,542,542,542,542,542,542,542,542,542,542,542,
  542,542,542,542,542,542,542,542,542,542,542,542,542,542,542,542,542,542,
  542,542,542,542,542,542,542,542,542,542,542,542,542,542,542,542,542,542,
  542,542,542,542,542,542,542,542,542,542,542,542,542,542,542,542,542,542,
  542,542,542,542,542,542,542,542,542,542,542,542,542,542,542,542,542,542,
  542,542,542,542,1,542,58,2,2,2,623,
542,542,542,542,542,542,542,542,542,542,542,542,542,542,542,542,542,542,542,
  542,542,542,542,542,542,542,542,542,542,542,542,542,542,542,542,542,542,
  542,542,542,542,542,542,542,1,542,59,2,2,2,624,
542,542,542,542,542,542,542,542,542,542,542,542,542,542,542,542,542,542,542,
  542,542,542,542,542,542,542,542,542,542,542,542,542,542,542,1,542,60,2,
  2,2,734,
542,542,542,542,542,542,542,542,542,542,542,542,542,542,542,542,542,542,542,
  542,542,542,542,542,542,542,542,542,542,542,542,542,542,542,542,542,542,
  542,542,542,542,542,542,542,1,542,61,2,2,2,607,
542,542,542,542,542,542,542,542,542,542,542,542,542,542,542,542,542,542,542,
  542,542,542,542,542,542,542,542,542,542,542,542,542,542,542,542,542,542,
  542,542,542,542,542,542,1,542,62,2,2,2,617,
542,542,542,542,542,542,542,542,542,542,542,542,542,542,542,542,542,542,542,
  542,542,542,542,542,542,542,542,542,542,542,542,542,542,542,1,542,63,2,
  2,2,733,
542,542,542,542,542,542,542,542,542,542,542,542,542,542,542,542,542,542,542,
  542,542,542,542,542,542,542,542,542,542,542,542,542,542,542,1,542,64,2,
  2,2,732,
542,542,542,542,542,542,542,542,542,542,542,542,542,542,542,542,542,542,542,
  542,542,542,542,542,542,542,542,542,542,542,542,542,542,542,542,542,542,
  542,542,542,542,542,542,542,542,542,542,542,542,542,542,542,542,542,542,
  542,542,542,542,542,542,542,542,542,542,542,542,542,542,542,542,542,542,
  542,542,542,542,542,542,542,542,542,542,542,542,542,542,542,542,542,542,
  542,542,542,542,542,542,542,542,542,542,542,542,542,542,542,542,542,542,
  542,542,542,542,542,542,542,542,542,542,542,542,542,542,542,542,542,542,
  542,542,542,542,542,542,542,542,1,542,65,2,2,2,620,
542,542,542,542,542,542,542,542,542,542,542,542,542,542,542,542,542,542,542,
  542,542,542,542,542,542,542,542,542,542,542,542,542,542,542,1,542,66,2,
  2,2,731,
36,39,41,42,43,44,45,46,47,48,49,50,51,52,54,60,63,64,66,59,58,37,65,62,53,
  57,55,56,61,40,38,8,9,6,8,6,6,6,6,6,6,6,6,6,6,6,6,6,6,6,6,6,6,6,6,6,6,6,
  6,6,6,6,6,6,6,6,6,6,
11,68,11,
10,69,10,
9,70,9,
542,542,542,542,542,542,542,542,542,542,542,542,542,542,542,542,542,542,542,
  542,542,542,542,542,542,542,542,542,542,542,542,542,542,542,542,542,542,
  542,542,542,542,542,542,542,542,542,542,542,542,542,542,542,542,542,542,
  542,542,542,542,542,542,542,542,542,542,542,542,542,542,542,542,542,542,
  542,542,542,542,542,542,542,542,542,542,542,542,542,542,542,542,542,542,
  542,542,542,542,542,1,542,71,2,2,2,730,
542,542,542,542,542,542,542,542,542,542,542,542,542,542,542,542,542,542,542,
  542,542,542,542,542,542,542,542,542,1,542,72,2,2,2,729,
542,542,542,542,542,542,542,542,1,542,73,2,2,2,728,
542,542,542,542,542,542,542,542,542,542,542,542,542,542,542,542,542,542,542,
  542,542,542,542,542,542,542,542,542,542,542,542,542,542,542,542,542,542,
  542,542,542,542,542,542,542,542,542,542,542,542,542,542,542,542,542,542,
  542,542,542,542,542,542,542,542,542,542,542,542,542,542,542,542,542,542,
  542,542,542,542,542,542,542,542,542,542,542,542,542,542,542,542,542,542,
  542,542,542,542,542,1,542,74,2,2,2,727,
542,542,542,542,542,542,542,542,542,542,542,542,542,542,542,542,542,542,542,
  542,542,542,542,542,542,542,542,542,1,542,75,2,2,2,726,
542,542,542,542,542,542,542,542,1,542,76,2,2,2,725,
542,542,542,542,542,542,542,542,542,542,542,542,542,542,542,542,542,542,542,
  542,542,542,542,542,542,542,542,542,542,542,542,542,542,542,542,542,542,
  542,542,542,542,542,542,542,542,542,542,542,542,542,542,542,542,542,542,
  542,542,542,542,542,542,542,542,542,542,542,542,542,542,542,542,542,542,
  542,542,542,542,542,542,542,542,542,542,542,542,542,542,542,542,542,542,
  542,542,542,542,542,1,542,77,2,2,2,724,
542,542,1,542,78,2,2,2,723,
542,542,542,542,542,542,542,542,542,542,542,542,542,542,542,542,542,542,542,
  542,542,542,542,542,542,542,542,542,1,542,79,2,2,2,722,
542,542,542,542,542,542,542,542,542,542,542,542,542,542,542,542,542,542,542,
  542,542,542,542,542,542,542,542,542,542,542,542,542,542,542,542,542,542,
  542,542,542,542,542,542,542,542,542,542,542,542,542,542,542,542,542,542,
  542,542,542,542,542,542,542,542,542,542,542,542,542,542,542,542,542,542,
  542,542,542,542,542,542,542,542,542,542,542,542,542,542,542,542,542,542,
  542,542,542,542,542,1,542,80,2,2,2,721,
542,542,542,542,542,542,542,542,542,542,542,542,542,542,542,542,542,542,542,
  542,542,542,542,542,542,542,542,542,542,542,542,542,542,542,542,542,542,
  542,542,542,542,542,542,542,542,542,542,542,542,542,542,542,542,542,542,
  542,542,542,542,542,542,542,542,542,542,542,542,542,542,542,542,542,542,
  542,542,542,542,542,542,542,542,542,542,542,542,542,542,542,542,542,542,
  542,542,542,542,542,1,542,81,2,2,2,720,
542,542,542,542,542,542,542,542,542,542,542,542,542,542,542,542,542,542,542,
  542,542,542,542,542,542,542,542,542,542,542,542,542,542,542,542,542,542,
  542,542,542,542,542,542,542,542,542,542,542,542,542,542,542,542,542,542,
  542,542,542,542,542,542,542,542,542,542,542,542,542,542,542,542,542,542,
  542,542,542,542,542,542,542,542,542,542,542,542,542,542,542,542,542,542,
  542,542,542,542,542,1,542,82,2,2,2,719,
542,542,542,542,542,542,542,542,542,542,542,542,542,542,542,542,542,542,542,
  542,542,542,542,542,542,542,542,542,1,542,83,2,2,2,718,
542,542,542,542,542,542,542,542,542,542,542,542,542,542,542,542,542,542,542,
  542,542,542,542,542,542,542,542,542,1,542,84,2,2,2,717,
542,542,542,542,542,542,542,542,1,542,85,2,2,2,716,
542,542,542,542,542,542,542,542,542,542,542,542,542,542,542,542,542,542,542,
  542,542,542,542,542,542,542,542,542,542,542,542,542,542,542,542,542,542,
  542,542,542,542,542,542,542,542,542,542,542,542,542,542,542,542,542,542,
  542,542,542,542,542,542,542,542,542,542,542,542,542,542,542,542,542,542,
  542,542,542,542,542,542,542,542,542,542,542,542,542,542,542,542,542,542,
  542,542,542,542,542,1,542,86,2,2,2,715,
542,542,1,542,87,2,2,2,714,
542,542,542,542,542,542,542,542,542,542,542,542,542,542,542,542,542,542,542,
  542,542,542,542,542,542,542,542,542,542,542,542,542,542,542,542,542,542,
  542,542,542,542,542,542,542,542,542,542,542,542,542,542,542,542,542,542,
  542,542,542,542,542,542,542,542,542,542,542,542,542,542,542,542,542,542,
  542,542,542,542,542,542,542,542,542,542,542,542,542,542,542,542,542,542,
  542,542,542,542,542,1,542,88,2,2,2,713,
542,542,542,542,542,542,542,542,542,542,542,542,542,542,542,542,542,542,542,
  542,542,542,542,542,542,542,542,542,542,542,542,542,542,542,542,542,542,
  542,542,542,542,542,542,542,542,542,542,542,542,542,542,542,542,542,542,
  542,542,542,542,542,542,542,542,542,542,542,542,542,542,542,542,542,542,
  542,542,542,542,542,542,542,542,542,542,542,542,542,542,542,542,542,542,
  542,542,542,542,542,1,542,89,2,2,2,712,
542,542,542,542,542,542,542,542,542,542,542,542,542,542,542,542,542,542,542,
  542,542,542,542,542,542,542,542,542,542,542,542,542,542,542,542,542,542,
  542,542,542,542,542,542,542,542,542,542,542,542,542,542,542,542,542,542,
  542,542,542,542,542,542,542,542,542,542,542,542,542,542,542,542,542,542,
  542,542,542,542,542,542,542,542,542,542,542,542,542,542,542,542,542,542,
  542,542,542,542,542,1,542,90,2,2,2,711,
542,542,542,542,542,542,542,542,542,542,542,542,542,542,542,542,542,542,542,
  542,542,542,542,542,542,542,542,542,542,542,542,542,542,542,542,542,542,
  542,542,542,542,542,542,542,542,542,542,542,542,542,542,542,542,542,542,
  542,542,542,542,542,542,542,542,542,542,542,542,542,542,542,542,542,542,
  542,542,542,542,542,542,542,542,542,542,542,542,542,542,542,542,542,542,
  542,542,542,542,542,1,542,91,2,2,2,710,
542,542,542,542,542,542,542,542,542,542,542,542,542,542,542,542,542,542,542,
  542,542,542,542,542,542,542,542,542,542,542,542,542,542,542,542,542,542,
  542,542,542,542,542,542,542,542,542,542,542,542,542,542,542,542,542,542,
  542,542,542,542,542,542,542,542,542,542,542,542,542,542,542,542,542,542,
  542,542,542,542,542,542,542,542,542,542,542,542,542,542,542,542,542,542,
  542,542,542,542,542,1,542,92,2,2,2,709,
542,542,542,542,542,542,542,542,542,542,542,542,542,542,542,542,542,542,542,
  542,542,542,542,542,542,542,542,542,542,542,542,542,542,542,542,542,542,
  542,542,542,542,542,542,542,542,542,542,542,542,542,542,542,542,542,542,
  542,542,542,542,542,542,542,542,542,542,542,542,542,542,542,542,542,542,
  542,542,542,542,542,542,542,542,542,542,542,542,542,542,542,542,542,542,
  542,542,542,542,542,1,542,93,2,2,2,708,
542,542,542,542,542,542,542,542,542,542,542,542,542,542,542,542,542,542,542,
  542,542,542,542,542,542,542,542,542,542,542,542,542,542,542,542,542,542,
  542,542,542,542,542,542,542,542,542,542,542,542,542,542,542,542,542,542,
  542,542,542,542,542,542,542,542,542,542,542,542,542,542,542,542,542,542,
  542,542,542,542,542,542,542,542,542,542,542,542,542,542,542,542,542,542,
  542,542,542,542,542,1,542,94,2,2,2,707,
542,542,542,542,542,542,542,542,542,542,542,542,542,542,542,542,542,542,542,
  542,542,542,542,542,542,542,542,542,542,542,542,542,542,542,542,542,542,
  542,542,542,542,542,542,542,542,542,542,542,542,542,542,542,542,542,542,
  542,542,542,542,542,542,542,542,542,542,542,542,542,542,542,542,542,542,
  542,542,542,542,542,542,542,542,542,542,542,542,542,542,542,542,542,542,
  542,542,542,542,542,1,542,95,2,2,2,706,
542,542,542,542,542,542,542,542,542,542,542,542,542,542,542,542,542,542,542,
  542,542,542,542,542,542,542,542,542,542,542,542,542,542,542,542,542,542,
  542,542,542,542,542,542,542,542,542,542,542,542,542,542,542,542,542,542,
  542,542,542,542,542,542,542,542,542,542,542,542,542,542,542,542,542,542,
  542,542,542,542,542,542,542,542,542,542,542,542,542,542,542,542,542,542,
  542,542,542,542,542,1,542,96,2,2,2,705,
542,542,542,542,542,542,542,542,542,542,542,542,542,542,542,542,542,542,542,
  542,542,542,542,542,542,542,542,542,542,542,542,542,542,542,542,542,542,
  542,542,542,542,542,542,542,542,542,542,542,542,542,542,542,542,542,542,
  542,542,542,542,542,542,542,542,542,542,542,542,542,542,542,542,542,542,
  542,542,542,542,542,542,542,542,542,542,542,542,542,542,542,542,542,542,
  542,542,542,542,542,1,542,97,2,2,2,704,
542,542,542,542,542,542,542,542,542,542,542,542,542,542,542,542,542,542,542,
  542,542,542,542,542,542,542,542,542,542,542,542,542,542,542,542,542,542,
  542,542,542,542,542,542,542,542,542,542,542,542,542,542,542,542,542,542,
  542,542,542,542,542,542,542,542,542,542,542,542,542,542,542,542,542,542,
  542,542,542,542,542,542,542,542,542,542,542,542,542,542,542,542,542,542,
  542,542,542,542,542,1,542,98,2,2,2,703,
542,542,542,542,542,542,542,542,542,542,542,542,542,542,542,542,542,542,542,
  542,542,542,542,542,542,542,542,542,542,542,542,542,542,542,542,542,542,
  542,542,542,542,542,542,542,542,542,542,542,542,542,542,542,542,542,542,
  542,542,542,542,542,542,542,542,542,542,542,542,542,542,542,542,542,542,
  542,542,542,542,542,542,542,542,542,542,542,542,542,542,542,542,542,542,
  542,542,542,542,542,1,542,99,2,2,2,702,
542,542,542,542,542,542,542,542,542,542,542,542,542,542,542,542,542,542,542,
  542,542,542,542,542,542,542,542,542,542,542,542,542,542,542,542,542,542,
  542,542,542,542,542,542,542,542,542,542,542,542,542,542,542,542,542,542,
  542,542,542,542,542,542,542,542,542,542,542,542,542,542,542,542,542,542,
  542,542,542,542,542,542,542,542,542,542,542,542,542,542,542,542,542,542,
  542,542,542,542,542,1,542,100,2,2,2,701,
542,542,542,542,542,542,542,542,542,542,542,542,542,542,542,542,542,542,542,
  542,542,542,542,542,542,542,542,542,542,542,542,542,542,542,542,542,542,
  542,542,542,542,542,542,542,542,542,542,542,542,542,542,542,542,542,542,
  542,542,542,542,542,542,542,542,542,542,542,542,542,542,542,542,542,542,
  542,542,542,542,542,542,542,542,542,542,542,542,542,542,542,542,542,542,
  542,542,542,542,542,1,542,101,2,2,2,700,
542,542,542,542,542,1,542,102,2,2,2,699,
542,542,542,542,542,1,542,103,2,2,2,698,
542,542,542,542,542,542,542,542,542,542,542,542,542,542,542,542,542,542,542,
  542,542,542,542,542,542,542,542,542,542,542,542,542,542,542,542,542,542,
  542,542,542,542,542,542,542,542,542,542,542,542,542,542,542,542,542,542,
  542,542,542,542,542,542,542,542,542,542,542,542,542,542,542,542,542,542,
  542,542,542,542,542,542,542,542,542,542,542,542,542,542,542,542,542,542,
  542,542,542,542,542,1,542,104,2,2,2,697,
542,542,542,542,542,542,542,542,542,542,542,542,542,542,542,542,542,542,542,
  542,542,542,542,542,542,542,542,542,1,542,105,2,2,2,696,
542,542,542,542,542,542,542,542,542,542,542,542,542,542,542,542,542,542,542,
  542,542,542,542,542,542,542,542,542,1,542,106,2,2,2,695,
542,542,542,542,542,542,542,542,1,542,107,2,2,2,694,
542,542,542,542,542,542,542,542,542,542,542,542,542,542,542,542,542,542,542,
  542,542,542,542,542,542,542,542,542,542,542,542,542,542,542,542,542,542,
  542,542,542,542,542,542,542,542,542,542,542,542,542,542,542,542,542,542,
  542,542,542,542,542,542,542,542,542,542,542,542,542,542,542,542,542,542,
  542,542,542,542,542,542,542,542,542,542,542,542,542,542,542,542,542,542,
  542,542,542,542,542,1,542,108,2,2,2,693,
542,542,542,542,542,542,542,542,1,542,109,2,2,2,692,
542,542,542,542,542,542,542,542,1,542,110,2,2,2,691,
542,542,542,542,1,542,111,2,2,2,690,
542,542,542,542,542,542,542,542,542,542,542,542,542,542,542,542,542,542,542,
  542,542,542,542,542,542,542,542,542,542,542,542,542,542,542,542,542,542,
  542,542,542,542,542,542,542,542,542,542,542,542,542,542,542,542,542,542,
  542,542,542,542,542,542,542,542,542,542,542,542,542,542,542,542,542,542,
  542,542,542,542,542,542,542,542,542,542,542,542,542,542,542,542,542,542,
  542,542,542,542,542,1,542,112,2,2,2,689,
542,542,542,542,542,542,542,542,542,542,542,542,542,542,542,542,542,542,542,
  542,542,542,542,542,542,542,542,542,1,542,113,2,2,2,688,
542,542,542,542,542,542,542,542,542,542,542,542,542,542,542,542,542,542,542,
  542,542,542,542,542,542,542,542,542,1,542,114,2,2,2,687,
542,542,542,542,542,542,542,542,542,542,542,542,542,542,542,542,542,542,542,
  542,542,542,542,542,542,542,542,542,1,542,115,2,2,2,686,
542,542,1,542,116,2,2,2,685,
542,542,542,542,542,542,542,542,542,542,542,542,542,542,542,542,542,542,542,
  542,542,542,542,542,542,542,542,542,1,542,117,2,2,2,684,
542,542,542,542,542,542,542,542,542,542,542,542,542,542,542,542,542,542,542,
  542,542,542,542,542,542,542,542,542,1,542,118,2,2,2,683,
542,542,542,542,542,542,542,542,542,542,542,542,542,542,542,542,542,542,542,
  542,542,542,542,542,542,542,542,542,1,542,119,2,2,2,682,
542,542,542,542,542,542,542,542,542,542,542,542,542,542,542,542,542,542,542,
  542,542,542,542,542,542,542,542,542,1,542,120,2,2,2,681,
542,542,542,542,542,542,542,542,542,542,542,542,542,542,542,542,542,542,542,
  542,542,542,542,542,542,542,542,542,1,542,121,2,2,2,680,
542,542,542,542,542,542,542,542,542,542,542,542,542,542,542,542,542,542,542,
  542,542,542,542,542,542,542,542,542,1,542,122,2,2,2,679,
542,542,542,542,542,542,542,542,542,542,542,542,542,542,542,542,542,542,542,
  542,542,542,542,542,542,542,542,542,1,542,123,2,2,2,678,
542,542,542,542,542,542,542,542,542,542,542,542,542,542,542,542,542,542,542,
  542,542,542,542,542,542,542,542,542,1,542,124,2,2,2,677,
542,542,542,542,542,542,542,542,542,542,542,542,542,542,542,542,542,542,542,
  542,542,542,542,542,542,542,542,542,1,542,125,2,2,2,676,
542,542,542,542,542,542,542,542,542,542,542,542,542,542,542,542,542,542,542,
  542,542,542,542,542,542,542,542,542,1,542,126,2,2,2,675,
542,542,542,542,542,542,542,542,542,542,542,542,542,542,542,542,542,542,542,
  542,542,542,542,542,542,542,542,542,1,542,127,2,2,2,674,
542,542,542,542,542,542,542,542,542,542,542,542,542,542,542,542,542,542,542,
  542,542,542,542,542,542,542,542,542,1,542,128,2,2,2,673,
542,542,542,542,542,542,542,542,542,542,542,542,542,542,542,542,542,542,542,
  542,542,542,542,542,542,542,542,542,1,542,129,2,2,2,672,
542,542,542,542,542,542,542,542,542,542,542,542,542,542,542,542,542,542,542,
  542,542,542,542,542,542,542,542,542,1,542,130,2,2,2,671,
542,542,542,542,1,542,131,2,2,2,670,
542,542,542,542,542,542,542,542,1,542,132,2,2,2,669,
542,542,542,542,542,542,542,542,542,542,542,542,542,542,542,542,542,542,542,
  542,542,542,542,542,542,542,542,542,1,542,133,2,2,2,668,
542,542,542,542,542,542,542,542,542,542,542,542,542,542,542,542,542,542,542,
  542,542,542,542,542,542,542,542,542,542,542,542,542,542,542,542,542,542,
  542,542,542,542,542,542,542,542,542,542,542,542,542,542,542,542,542,542,
  542,542,542,542,542,542,542,542,542,542,542,542,542,542,542,542,542,542,
  542,542,542,542,542,542,542,542,542,542,542,542,542,542,542,542,542,542,
  542,542,542,542,542,1,542,134,2,2,2,667,
542,542,542,542,542,542,542,542,542,542,542,542,542,542,542,542,542,542,542,
  542,542,542,542,542,542,542,542,542,542,542,542,542,542,542,542,542,542,
  542,542,542,542,542,542,542,542,542,542,542,542,542,542,542,542,542,542,
  542,542,542,542,542,542,542,542,542,542,542,542,542,542,542,542,542,542,
  542,542,542,542,542,542,542,542,542,542,542,542,542,542,542,542,542,542,
  542,542,542,542,542,1,542,135,2,2,2,666,
542,542,542,542,542,542,542,542,542,542,542,542,542,542,542,542,542,542,542,
  542,542,542,542,542,542,542,542,542,542,542,542,542,542,542,542,542,542,
  542,542,542,542,542,542,542,542,542,542,542,542,542,542,542,542,542,542,
  542,542,542,542,542,542,542,542,542,542,542,542,542,542,542,542,542,542,
  542,542,542,542,542,542,542,542,542,542,542,542,542,542,542,542,542,542,
  542,542,542,542,542,1,542,136,2,2,2,665,
542,542,542,542,542,542,542,542,542,542,542,542,542,542,542,542,542,542,542,
  542,542,542,542,542,542,542,542,542,542,542,542,542,542,542,542,542,542,
  542,542,542,542,542,542,542,542,542,542,542,542,542,542,542,542,542,542,
  542,542,542,542,542,542,542,542,542,542,542,542,542,542,542,542,542,542,
  542,542,542,542,542,542,542,542,542,542,542,542,542,542,542,542,542,542,
  542,542,542,542,542,1,542,137,2,2,2,664,
542,542,542,542,1,542,138,2,2,2,663,
542,542,542,542,542,542,542,542,1,542,139,2,2,2,662,
542,542,542,542,1,542,140,2,2,2,661,
542,542,542,542,542,542,542,542,542,542,542,542,542,542,542,542,542,542,542,
  542,542,542,542,542,542,542,542,542,542,542,542,542,542,542,542,542,542,
  542,542,542,542,542,542,542,542,542,542,542,542,542,542,542,542,542,542,
  542,542,542,542,542,542,542,542,542,542,542,542,542,542,542,542,542,542,
  542,542,542,542,542,542,542,542,542,542,542,542,542,542,542,542,542,542,
  542,542,542,542,542,1,542,141,2,2,2,659,
542,542,542,542,542,542,542,542,542,542,542,542,542,542,542,542,542,542,542,
  542,542,542,542,542,542,542,542,542,1,542,142,2,2,2,658,
542,542,542,542,542,542,542,542,542,542,542,542,542,542,542,542,542,542,542,
  542,542,542,542,542,542,542,542,542,1,542,143,2,2,2,657,
542,542,542,542,542,542,542,542,542,542,542,542,542,542,542,542,542,542,542,
  542,542,542,542,542,542,542,542,542,1,542,144,2,2,2,656,
542,542,542,542,542,542,542,542,542,542,542,542,542,542,542,542,542,542,542,
  542,542,542,542,542,542,542,542,542,1,542,145,2,2,2,655,
542,542,542,542,542,542,542,542,542,542,542,542,542,542,542,542,542,542,542,
  542,542,542,542,542,542,542,542,542,1,542,146,2,2,2,654,
542,542,542,542,542,542,542,542,542,542,542,542,542,542,542,542,542,542,542,
  542,542,542,542,542,542,542,542,542,1,542,147,2,2,2,653,
542,542,542,542,542,542,542,542,542,542,542,542,542,542,542,542,542,542,542,
  542,542,542,542,542,542,542,542,542,1,542,148,2,2,2,652,
542,542,542,542,542,542,542,542,1,542,149,2,2,2,651,
542,542,542,542,542,542,542,542,542,542,542,542,542,542,542,542,542,542,542,
  542,542,542,542,542,542,542,542,542,542,542,542,542,542,542,542,542,542,
  542,542,542,542,542,542,542,542,542,542,542,542,542,542,542,542,542,542,
  542,542,542,542,542,542,542,542,542,542,542,542,542,542,542,542,542,542,
  542,542,542,542,542,542,542,542,542,542,542,542,542,542,542,542,542,542,
  542,542,542,542,542,1,542,150,2,2,2,650,
542,542,542,542,542,542,542,542,542,542,542,542,542,542,542,542,542,542,542,
  542,542,542,542,542,542,542,542,542,542,542,542,542,542,542,542,542,542,
  542,542,542,542,542,542,542,542,542,542,542,542,542,542,542,542,542,542,
  542,542,542,542,542,542,542,542,542,542,542,542,542,542,542,542,542,542,
  542,542,542,542,542,542,542,542,542,542,542,542,542,542,542,542,542,542,
  542,542,542,542,542,1,542,151,2,2,2,649,
542,542,542,542,542,542,542,542,542,542,542,542,542,542,542,542,542,542,542,
  542,542,542,542,542,542,542,542,542,1,542,152,2,2,2,648,
542,542,542,542,542,542,542,542,542,542,542,542,542,542,542,542,542,542,542,
  542,542,542,542,542,542,542,542,542,1,542,153,2,2,2,647,
542,542,542,542,542,542,542,542,542,542,542,542,542,542,542,542,542,542,542,
  542,542,542,542,542,542,542,542,542,1,542,154,2,2,2,646,
542,542,542,542,542,542,542,542,542,542,542,542,542,542,542,542,542,542,542,
  542,542,542,542,542,542,542,542,542,542,542,542,542,542,542,542,542,542,
  542,542,542,542,542,542,542,542,542,542,542,542,542,542,542,542,542,542,
  542,542,542,542,542,542,542,542,542,542,542,542,542,542,542,542,542,542,
  542,542,542,542,542,542,542,542,542,542,542,542,542,542,542,542,542,542,
  542,542,542,542,542,1,542,155,2,2,2,645,
542,542,542,542,542,542,542,542,542,542,542,542,542,542,542,542,542,542,542,
  542,542,542,542,542,542,542,542,542,1,542,156,2,2,2,644,
542,542,542,542,542,542,542,542,1,542,157,2,2,2,643,
542,542,542,542,542,542,542,542,542,542,542,542,542,542,542,542,542,542,542,
  542,542,542,542,542,542,542,542,542,1,542,158,2,2,2,642,
542,542,542,542,542,542,542,542,1,542,159,2,2,2,641,
542,542,542,542,542,542,542,542,1,542,160,2,2,2,640,
542,542,542,542,542,542,542,542,542,542,542,542,542,542,542,542,542,542,542,
  542,542,542,542,542,542,542,542,542,1,542,161,2,2,2,638,
276,276,276,276,276,276,276,276,276,276,276,276,276,276,276,276,276,276,276,
  276,276,276,276,276,276,276,276,276,5,162,5,276,
277,277,277,277,277,277,277,277,5,163,5,277,
278,278,278,278,278,278,278,278,278,278,278,278,278,278,278,278,278,278,278,
  278,278,278,278,278,278,278,278,278,5,164,5,278,
279,279,279,279,279,279,279,279,5,165,5,279,
280,280,5,166,5,280,
281,281,281,281,281,281,281,281,281,281,281,281,281,281,281,281,281,281,281,
  281,281,281,281,281,281,281,281,281,5,167,5,281,
282,282,282,282,282,282,282,282,282,282,282,282,282,282,282,282,282,282,282,
  282,282,282,282,282,282,282,282,282,5,168,5,282,
283,283,283,283,283,283,283,283,283,283,283,283,283,283,283,283,283,283,283,
  283,283,283,283,283,283,283,283,283,5,169,5,283,
284,284,284,284,284,284,284,284,5,170,5,284,
285,285,5,171,5,285,
286,286,286,286,286,5,172,5,286,
287,287,287,287,287,5,173,5,287,
288,288,288,288,288,288,288,288,288,288,288,288,288,288,288,288,288,288,288,
  288,288,288,288,288,288,288,288,288,5,174,5,288,
289,289,289,289,289,289,289,289,289,289,289,289,289,289,289,289,289,289,289,
  289,289,289,289,289,289,289,289,289,5,175,5,289,
290,290,290,290,290,290,290,290,5,176,5,290,
291,291,291,291,291,291,291,291,5,177,5,291,
292,292,292,292,292,292,292,292,5,178,5,292,
293,293,293,293,5,179,5,293,
294,294,294,294,294,294,294,294,294,294,294,294,294,294,294,294,294,294,294,
  294,294,294,294,294,294,294,294,294,5,180,5,294,
295,295,295,295,295,295,295,295,295,295,295,295,295,295,295,295,295,295,295,
  295,295,295,295,295,295,295,295,295,5,181,5,295,
296,296,296,296,296,296,296,296,296,296,296,296,296,296,296,296,296,296,296,
  296,296,296,296,296,296,296,296,296,5,182,5,296,
297,297,5,183,5,297,
298,298,298,298,298,298,298,298,298,298,298,298,298,298,298,298,298,298,298,
  298,298,298,298,298,298,298,298,298,5,184,5,298,
299,299,299,299,299,299,299,299,299,299,299,299,299,299,299,299,299,299,299,
  299,299,299,299,299,299,299,299,299,5,185,5,299,
300,300,300,300,300,300,300,300,300,300,300,300,300,300,300,300,300,300,300,
  300,300,300,300,300,300,300,300,300,5,186,5,300,
301,301,301,301,301,301,301,301,301,301,301,301,301,301,301,301,301,301,301,
  301,301,301,301,301,301,301,301,301,5,187,5,301,
302,302,302,302,302,302,302,302,302,302,302,302,302,302,302,302,302,302,302,
  302,302,302,302,302,302,302,302,302,5,188,5,302,
303,303,303,303,303,303,303,303,303,303,303,303,303,303,303,303,303,303,303,
  303,303,303,303,303,303,303,303,303,5,189,5,303,
304,304,304,304,304,304,304,304,304,304,304,304,304,304,304,304,304,304,304,
  304,304,304,304,304,304,304,304,304,5,190,5,304,
305,305,305,305,305,305,305,305,305,305,305,305,305,305,305,305,305,305,305,
  305,305,305,305,305,305,305,305,305,5,191,5,305,
306,306,306,306,306,306,306,306,306,306,306,306,306,306,306,306,306,306,306,
  306,306,306,306,306,306,306,306,306,5,192,5,306,
307,307,307,307,307,307,307,307,307,307,307,307,307,307,307,307,307,307,307,
  307,307,307,307,307,307,307,307,307,5,193,5,307,
308,308,308,308,308,308,308,308,308,308,308,308,308,308,308,308,308,308,308,
  308,308,308,308,308,308,308,308,308,5,194,5,308,
309,309,309,309,309,309,309,309,309,309,309,309,309,309,309,309,309,309,309,
  309,309,309,309,309,309,309,309,309,5,195,5,309,
310,310,310,310,310,310,310,310,310,310,310,310,310,310,310,310,310,310,310,
  310,310,310,310,310,310,310,310,310,5,196,5,310,
311,311,311,311,311,311,311,311,311,311,311,311,311,311,311,311,311,311,311,
  311,311,311,311,311,311,311,311,311,5,197,5,311,
312,312,312,312,5,198,5,312,
313,313,313,313,313,313,313,313,5,199,5,313,
314,314,314,314,314,314,314,314,314,314,314,314,314,314,314,314,314,314,314,
  314,314,314,314,314,314,314,314,314,5,200,5,314,
315,315,315,315,5,201,5,315,
316,316,316,316,316,316,316,316,5,202,5,316,
317,317,317,317,5,203,5,317,
318,318,318,318,318,318,318,318,318,318,318,318,318,318,318,318,318,318,318,
  318,318,318,318,318,318,318,318,318,5,204,5,318,
319,319,319,319,319,319,319,319,319,319,319,319,319,319,319,319,319,319,319,
  319,319,319,319,319,319,319,319,319,5,205,5,319,
320,320,320,320,320,320,320,320,320,320,320,320,320,320,320,320,320,320,320,
  320,320,320,320,320,320,320,320,320,5,206,5,320,
321,321,321,321,321,321,321,321,321,321,321,321,321,321,321,321,321,321,321,
  321,321,321,321,321,321,321,321,321,5,207,5,321,
322,322,322,322,322,322,322,322,322,322,322,322,322,322,322,322,322,322,322,
  322,322,322,322,322,322,322,322,322,5,208,5,322,
323,323,323,323,323,323,323,323,323,323,323,323,323,323,323,323,323,323,323,
  323,323,323,323,323,323,323,323,323,5,209,5,323,
324,324,324,324,324,324,324,324,324,324,324,324,324,324,324,324,324,324,324,
  324,324,324,324,324,324,324,324,324,5,210,5,324,
325,325,325,325,325,325,325,325,5,211,5,325,
326,326,326,326,326,326,326,326,326,326,326,326,326,326,326,326,326,326,326,
  326,326,326,326,326,326,326,326,326,5,212,5,326,
327,327,327,327,327,327,327,327,327,327,327,327,327,327,327,327,327,327,327,
  327,327,327,327,327,327,327,327,327,5,213,5,327,
328,328,328,328,328,328,328,328,328,328,328,328,328,328,328,328,328,328,328,
  328,328,328,328,328,328,328,328,328,5,214,5,328,
329,329,329,329,329,329,329,329,329,329,329,329,329,329,329,329,329,329,329,
  329,329,329,329,329,329,329,329,329,5,215,5,329,
330,330,330,330,330,330,330,330,5,216,5,330,
331,331,331,331,331,331,331,331,331,331,331,331,331,331,331,331,331,331,331,
  331,331,331,331,331,331,331,331,331,5,217,5,331,
332,332,332,332,332,332,332,332,5,218,5,332,
333,333,333,333,333,333,333,333,5,219,5,333,
334,334,334,334,334,334,334,334,334,334,334,334,334,334,334,334,334,334,334,
  334,334,334,334,334,334,334,334,334,5,220,5,334,
335,335,335,335,335,335,335,335,335,335,335,335,335,335,335,335,335,335,335,
  335,335,335,335,335,335,335,335,335,335,335,335,335,335,335,335,335,335,
  335,335,335,335,335,335,335,335,335,335,335,335,335,335,335,335,335,335,
  335,335,335,335,335,335,335,335,335,335,335,335,335,335,335,335,335,335,
  335,335,335,335,335,335,335,335,335,335,335,335,335,335,335,335,335,335,
  335,17,17,17,17,5,221,5,335,
542,542,542,542,542,542,542,542,542,1,2,222,2,2,2,588,
82,82,82,82,82,82,82,82,81,223,70,265,265,
542,542,542,542,542,542,542,542,542,542,542,542,542,1,2,224,2,2,2,587,
85,85,85,85,85,85,85,85,85,85,85,85,85,225,69,272,
542,542,542,542,1,2,226,2,2,2,586,
542,542,542,542,1,542,227,2,2,2,585,
336,67,336,
542,542,1,2,229,2,2,2,583,
337,91,230,65,270,338,64,271,339,
542,542,542,542,1,2,231,2,2,2,581,
542,542,542,542,1,2,232,2,2,2,580,
542,542,542,542,542,542,542,542,542,542,542,542,542,542,542,542,542,542,542,
  542,542,542,542,542,542,542,542,542,1,2,233,2,2,2,579,
82,82,82,82,82,82,82,173,173,341,340,344,82,349,350,351,352,353,354,355,59,
  37,365,65,342,343,182,81,234,161,151,347,348,162,345,163,346,376,378,
  265,377,375,374,373,372,371,370,370,370,369,368,367,150,370,366,364,363,
  362,361,360,359,358,265,357,356,
542,542,542,542,542,542,542,542,542,542,542,542,542,1,2,235,2,2,2,578,
82,82,82,82,82,82,82,82,81,59,60,265,265,
542,542,542,542,542,542,542,542,542,542,542,542,542,542,542,542,542,542,542,
  542,542,542,542,542,542,542,542,542,1,2,237,2,2,2,577,
82,82,82,82,82,82,82,173,173,341,340,344,82,349,350,351,352,353,354,355,59,
  37,365,65,342,343,182,81,238,161,151,347,348,162,345,163,346,58,265,375,
  374,373,372,371,370,370,370,369,368,367,150,370,366,364,363,362,361,360,
  359,358,265,357,356,
542,542,542,542,542,542,542,542,542,1,2,239,2,2,2,576,
82,82,82,82,82,82,82,82,81,240,57,265,265,
542,542,542,542,542,542,542,542,542,1,2,241,2,2,2,575,
82,82,82,82,82,82,82,82,81,242,78,379,265,265,
542,542,542,542,542,542,542,542,542,1,2,243,2,2,2,574,
82,82,82,82,82,82,82,82,81,244,78,380,265,265,
542,542,542,542,542,542,542,542,542,1,2,245,2,2,2,573,
82,82,82,82,82,82,82,82,81,246,78,381,265,265,
542,542,542,542,542,542,542,542,542,542,542,542,542,542,542,542,542,542,542,
  542,542,542,542,542,542,542,542,542,542,1,2,247,2,2,2,572,
82,82,82,82,82,82,82,173,173,341,340,382,82,91,349,350,351,352,353,354,355,
  59,37,365,65,342,343,182,81,248,161,151,347,348,162,345,72,270,163,346,
  338,383,74,383,265,375,374,373,372,371,370,370,370,369,368,367,150,370,
  366,364,363,362,361,360,359,358,271,265,339,357,356,
542,542,542,542,542,542,542,542,542,542,542,542,542,542,542,542,542,542,542,
  542,542,542,542,542,542,542,542,542,542,1,2,249,2,2,2,571,
82,82,82,82,82,82,82,173,173,341,340,382,82,91,349,350,351,352,353,354,355,
  59,37,365,65,342,343,182,81,250,161,151,347,348,162,345,72,270,163,346,
  338,384,74,384,265,375,374,373,372,371,370,370,370,369,368,367,150,370,
  366,364,363,362,361,360,359,358,271,265,339,357,356,
542,542,542,542,542,542,542,542,542,542,542,542,542,542,542,542,542,542,542,
  542,542,542,542,542,542,542,542,542,1,2,251,2,2,2,570,
82,82,82,82,82,82,82,173,173,341,340,344,82,349,350,351,352,353,354,355,59,
  37,365,65,342,343,182,81,252,161,151,347,348,162,345,163,346,49,265,375,
  374,373,372,371,370,370,370,369,368,367,150,370,366,364,363,362,361,360,
  359,358,265,357,356,
542,542,542,542,1,2,253,2,2,2,569,
542,542,542,542,1,2,254,2,2,2,568,
542,542,542,542,1,2,255,2,2,2,567,
542,542,542,542,542,542,542,542,542,542,542,542,542,542,542,542,542,542,542,
  542,542,542,542,542,542,542,542,542,1,2,256,2,2,2,566,
82,82,82,82,82,82,82,173,173,341,340,344,82,349,350,351,352,353,354,355,59,
  37,365,65,342,343,182,81,257,161,151,347,348,162,345,163,346,44,265,375,
  374,373,372,371,370,370,370,369,368,367,150,370,366,364,363,362,361,360,
  359,358,265,357,356,
82,82,82,82,82,82,82,173,173,341,340,382,82,91,349,350,351,352,353,354,355,
  59,37,365,65,342,343,182,81,258,161,151,347,348,162,345,72,270,163,346,
  338,385,74,385,265,375,374,373,372,371,370,370,370,369,368,367,150,370,
  366,364,363,362,361,360,359,358,271,265,339,357,356,
82,82,82,82,82,82,82,173,173,341,340,382,82,91,349,350,351,352,353,354,355,
  59,37,365,65,342,343,182,81,259,161,151,347,348,162,345,72,270,163,346,
  338,386,74,386,265,375,374,373,372,371,370,370,370,369,368,367,150,370,
  366,364,363,362,361,360,359,358,271,265,339,357,356,
82,82,82,82,82,82,82,173,173,341,340,344,82,349,350,351,352,353,354,355,59,
  37,365,65,342,343,182,81,260,161,151,347,348,162,345,163,346,48,265,375,
  374,373,372,371,370,370,370,369,368,367,150,370,366,364,363,362,361,360,
  359,358,265,357,356,
542,542,542,542,542,542,542,542,542,542,542,542,542,542,542,542,542,542,542,
  542,542,542,542,542,542,542,542,542,1,2,261,2,2,2,565,
82,82,82,82,82,82,82,173,173,341,340,344,82,349,350,351,352,353,354,355,59,
  37,365,65,342,343,182,81,262,161,151,347,348,162,345,163,346,39,265,375,
  374,373,372,371,370,370,370,369,368,367,150,370,366,364,363,362,361,360,
  359,358,265,357,356,
542,542,542,542,542,542,542,542,542,542,542,542,542,542,542,542,542,542,542,
  542,542,542,542,542,542,542,542,542,1,2,263,2,2,2,564,
82,82,82,82,82,82,82,173,173,341,340,344,82,349,350,351,352,353,354,355,59,
  37,365,65,342,343,182,81,264,161,151,347,348,162,345,163,346,38,265,375,
  374,373,372,371,370,370,370,369,368,367,150,370,366,364,363,362,361,360,
  359,358,265,357,356,
83,83,83,83,83,83,83,84,84,84,83,542,542,542,542,542,542,542,542,542,542,
  542,542,542,542,542,542,542,542,542,542,542,542,542,542,542,1,2,265,2,
  84,2,2,557,
542,542,542,542,1,2,266,2,2,2,556,
542,542,542,542,1,2,267,2,2,2,554,
97,97,387,97,97,97,97,97,97,95,97,97,97,97,97,97,97,97,97,97,97,97,97,97,97,
  97,97,97,97,97,97,97,97,268,
542,542,542,542,1,2,269,2,2,2,551,
92,92,388,92,92,92,92,92,92,92,92,92,92,92,92,92,92,90,92,92,92,92,92,92,92,
  92,92,92,92,92,92,92,92,92,270,
542,542,542,542,542,1,542,271,2,2,2,549,
86,86,86,86,86,86,86,86,86,86,86,86,86,542,542,542,1,2,272,2,2,2,552,
82,82,82,82,82,82,82,173,173,341,340,344,82,349,350,351,352,353,354,355,59,
  37,365,65,342,343,182,81,273,161,151,347,348,162,345,163,346,41,265,375,
  374,373,372,371,370,370,370,369,368,367,150,370,366,364,363,362,361,360,
  359,358,265,357,356,
82,82,82,82,82,82,82,173,173,341,340,344,82,349,350,351,352,353,354,355,59,
  37,365,65,342,343,182,81,274,161,151,347,348,162,345,163,346,40,265,375,
  374,373,372,371,370,370,370,369,368,367,150,370,366,364,363,362,361,360,
  359,358,265,357,356,
542,542,542,542,542,542,542,542,542,542,542,542,542,542,542,542,542,542,542,
  542,542,542,542,542,542,542,542,542,542,542,542,542,542,542,542,542,542,
  542,542,542,542,542,542,542,542,542,542,542,542,542,542,542,542,542,542,
  542,542,542,542,542,542,542,542,542,542,542,542,542,542,542,542,542,542,
  542,542,542,542,542,542,542,542,542,542,542,542,542,542,542,542,542,542,
  542,542,542,542,542,542,542,542,542,542,542,542,542,542,542,542,542,542,
  542,542,542,542,542,542,542,542,542,542,542,542,542,542,542,542,542,542,
  542,542,542,542,542,542,542,1,542,542,275,2,2,2,547,
82,82,82,82,82,82,82,173,173,341,340,344,82,349,350,351,352,353,354,355,59,
  37,365,65,342,343,182,81,276,161,151,347,348,162,345,163,346,306,265,
  375,374,373,372,371,370,370,370,369,368,367,150,370,366,364,363,362,361,
  360,359,358,265,357,356,
202,199,197,195,201,200,198,196,277,305,389,
82,82,82,82,82,82,82,173,173,341,340,344,82,349,350,351,352,353,354,355,59,
  37,365,65,342,343,182,81,278,161,151,347,348,162,345,163,346,303,265,
  375,374,373,372,371,370,370,370,369,368,367,150,370,366,364,363,362,361,
  360,359,358,265,357,356,
202,199,197,195,201,200,198,196,279,302,389,
390,391,280,212,213,300,
82,82,82,82,82,82,82,173,173,341,340,344,82,349,350,351,352,353,354,355,59,
  37,365,65,342,343,182,81,281,161,151,347,348,162,345,163,346,299,265,
  375,374,373,372,371,370,370,370,369,368,367,150,370,366,364,363,362,361,
  360,359,358,265,357,356,
82,82,82,82,82,82,82,173,173,341,340,344,82,349,350,351,352,353,354,355,59,
  37,365,65,342,343,182,81,282,161,151,347,348,162,345,163,346,295,265,
  375,374,373,372,371,370,370,370,369,368,367,150,370,366,364,363,362,361,
  360,359,358,265,357,356,
82,82,82,82,82,82,82,173,173,341,340,344,82,349,350,351,352,353,354,355,59,
  37,365,65,342,343,182,81,283,161,151,347,348,162,345,163,346,294,265,
  375,374,373,372,371,370,370,370,369,368,367,150,370,366,364,363,362,361,
  360,359,358,265,357,356,
202,199,197,195,201,200,198,196,284,293,389,
51,52,285,291,291,
392,393,394,390,391,286,209,207,208,211,276,210,
392,393,394,390,391,287,209,207,208,211,275,210,
82,82,82,82,82,82,82,173,173,341,340,344,82,349,350,351,352,353,354,355,59,
  37,365,65,342,343,182,81,288,161,151,347,348,162,345,163,346,273,265,
  375,374,373,372,371,370,370,370,369,368,367,150,370,366,364,363,362,361,
  360,359,358,265,357,356,
82,82,82,82,82,82,82,173,173,341,340,344,82,349,350,351,352,353,354,355,59,
  37,365,65,342,343,182,81,289,161,151,347,348,162,345,163,346,272,265,
  375,374,373,372,371,370,370,370,369,368,367,150,370,366,364,363,362,361,
  360,359,358,265,357,356,
202,199,197,195,201,200,198,196,290,271,389,
202,199,197,195,201,200,198,196,291,395,389,
202,199,197,195,201,200,198,196,292,396,389,
205,204,203,206,293,398,397,
82,82,82,82,82,82,82,173,173,341,340,344,82,349,350,351,352,353,354,355,59,
  37,365,65,342,343,182,81,294,161,151,347,348,162,345,163,346,265,265,
  375,374,373,372,371,370,370,370,369,368,367,150,370,366,364,363,362,361,
  360,359,358,265,357,356,
82,82,82,82,82,82,82,173,173,341,340,344,82,349,350,351,352,353,354,355,59,
  37,365,65,342,343,182,81,295,161,151,347,348,162,345,163,346,264,265,
  375,374,373,372,371,370,370,370,369,368,367,150,370,366,364,363,362,361,
  360,359,358,265,357,356,
82,82,82,82,82,82,82,173,173,341,340,344,82,349,350,351,352,353,354,355,59,
  37,365,65,342,343,182,81,296,161,151,347,348,162,345,163,346,263,265,
  375,374,373,372,371,370,370,370,369,368,367,150,370,366,364,363,362,361,
  360,359,358,265,357,356,
390,391,297,212,213,262,
82,82,82,82,82,82,82,173,173,341,340,344,82,349,350,351,352,353,354,355,59,
  37,365,65,342,343,182,81,298,161,151,347,348,162,345,163,346,261,265,
  375,374,373,372,371,370,370,370,369,368,367,150,370,366,364,363,362,361,
  360,359,358,265,357,356,
82,82,82,82,82,82,82,173,173,341,340,344,82,349,350,351,352,353,354,355,59,
  37,365,65,342,343,182,81,299,161,151,347,348,162,345,163,346,260,265,
  375,374,373,372,371,370,370,370,369,368,367,150,370,366,364,363,362,361,
  360,359,358,265,357,356,
82,82,82,82,82,82,82,173,173,341,340,344,82,349,350,351,352,353,354,355,59,
  37,365,65,342,343,182,81,300,161,151,347,348,162,345,163,346,259,265,
  375,374,373,372,371,370,370,370,369,368,367,150,370,366,364,363,362,361,
  360,359,358,265,357,356,
82,82,82,82,82,82,82,173,173,341,340,344,82,349,350,351,352,353,354,355,59,
  37,365,65,342,343,182,81,301,161,151,347,348,162,345,163,346,258,265,
  375,374,373,372,371,370,370,370,369,368,367,150,370,366,364,363,362,361,
  360,359,358,265,357,356,
82,82,82,82,82,82,82,173,173,341,340,344,82,349,350,351,352,353,354,355,59,
  37,365,65,342,343,182,81,302,161,151,347,348,162,345,163,346,257,265,
  375,374,373,372,371,370,370,370,369,368,367,150,370,366,364,363,362,361,
  360,359,358,265,357,356,
82,82,82,82,82,82,82,173,173,341,340,344,82,349,350,351,352,353,354,355,59,
  37,365,65,342,343,182,81,303,161,151,347,348,162,345,163,346,256,265,
  375,374,373,372,371,370,370,370,369,368,367,150,370,366,364,363,362,361,
  360,359,358,265,357,356,
82,82,82,82,82,82,82,173,173,341,340,344,82,349,350,351,352,353,354,355,59,
  37,365,65,342,343,182,81,304,161,151,347,348,162,345,163,346,255,265,
  375,374,373,372,371,370,370,370,369,368,367,150,370,366,364,363,362,361,
  360,359,358,265,357,356,
82,82,82,82,82,82,82,173,173,341,340,344,82,349,350,351,352,353,354,355,59,
  37,365,65,342,343,182,81,305,161,151,347,348,162,345,163,346,254,265,
  375,374,373,372,371,370,370,370,369,368,367,150,370,366,364,363,362,361,
  360,359,358,265,357,356,
82,82,82,82,82,82,82,173,173,341,340,344,82,349,350,351,352,353,354,355,59,
  37,365,65,342,343,182,81,306,161,151,347,348,162,345,163,346,253,265,
  375,374,373,372,371,370,370,370,369,368,367,150,370,366,364,363,362,361,
  360,359,358,265,357,356,
82,82,82,82,82,82,82,173,173,341,340,344,82,349,350,351,352,353,354,355,59,
  37,365,65,342,343,182,81,307,161,151,347,348,162,345,163,346,252,265,
  375,374,373,372,371,370,370,370,369,368,367,150,370,366,364,363,362,361,
  360,359,358,265,357,356,
82,82,82,82,82,82,82,173,173,341,340,344,82,349,350,351,352,353,354,355,59,
  37,365,65,342,343,182,81,308,161,151,347,348,162,345,163,346,251,265,
  375,374,373,372,371,370,370,370,369,368,367,150,370,366,364,363,362,361,
  360,359,358,265,357,356,
82,82,82,82,82,82,82,173,173,341,340,344,82,349,350,351,352,353,354,355,59,
  37,365,65,342,343,182,81,309,161,151,347,348,162,345,163,346,250,265,
  375,374,373,372,371,370,370,370,369,368,367,150,370,366,364,363,362,361,
  360,359,358,265,357,356,
82,82,82,82,82,82,82,173,173,341,340,344,82,349,350,351,352,353,354,355,59,
  37,365,65,342,343,182,81,310,161,151,347,348,162,345,163,346,249,265,
  375,374,373,372,371,370,370,370,369,368,367,150,370,366,364,363,362,361,
  360,359,358,265,357,356,
82,82,82,82,82,82,82,173,173,341,340,344,82,349,350,351,352,353,354,355,59,
  37,365,65,342,343,182,81,311,161,151,347,348,162,345,163,346,248,265,
  375,374,373,372,371,370,370,370,369,368,367,150,370,366,364,363,362,361,
  360,359,358,265,357,356,
205,204,203,206,312,247,397,
202,199,197,195,201,200,198,196,313,246,389,
82,82,82,82,82,82,82,173,173,341,340,344,82,349,350,351,352,353,354,355,59,
  37,365,65,342,343,182,81,314,161,151,347,348,162,345,163,346,245,265,
  375,374,373,372,371,370,370,370,369,368,367,150,370,366,364,363,362,361,
  360,359,358,265,357,356,
205,204,203,206,315,240,397,
202,199,197,195,201,200,198,196,316,239,389,
205,204,203,206,317,238,397,
82,82,82,82,82,82,82,173,173,341,340,344,82,349,350,351,352,353,354,355,59,
  37,365,65,342,343,182,81,318,161,151,347,348,162,345,163,346,236,265,
  375,374,373,372,371,370,370,370,369,368,367,150,370,366,364,363,362,361,
  360,359,358,265,357,356,
82,82,82,82,82,82,82,173,173,341,340,344,82,349,350,351,352,353,354,355,59,
  37,365,65,342,343,182,81,319,161,151,347,348,162,345,163,346,235,265,
  375,374,373,372,371,370,370,370,369,368,367,150,370,366,364,363,362,361,
  360,359,358,265,357,356,
82,82,82,82,82,82,82,173,173,341,340,344,82,349,350,351,352,353,354,355,59,
  37,365,65,342,343,182,81,320,161,151,347,348,162,345,163,346,234,265,
  375,374,373,372,371,370,370,370,369,368,367,150,370,366,364,363,362,361,
  360,359,358,265,357,356,
82,82,82,82,82,82,82,173,173,341,340,344,82,349,350,351,352,353,354,355,59,
  37,365,65,342,343,182,81,321,161,151,347,348,162,345,163,346,233,265,
  375,374,373,372,371,370,370,370,369,368,367,150,370,366,364,363,362,361,
  360,359,358,265,357,356,
82,82,82,82,82,82,82,173,173,341,340,344,82,349,350,351,352,353,354,355,59,
  37,365,65,342,343,182,81,322,161,151,347,348,162,345,163,346,232,265,
  375,374,373,372,371,370,370,370,369,368,367,150,370,366,364,363,362,361,
  360,359,358,265,357,356,
82,82,82,82,82,82,82,173,173,341,340,344,82,349,350,351,352,353,354,355,59,
  37,365,65,342,343,182,81,323,161,151,347,348,162,345,163,346,231,265,
  375,374,373,372,371,370,370,370,369,368,367,150,370,366,364,363,362,361,
  360,359,358,265,357,356,
82,82,82,82,82,82,82,173,173,341,340,344,82,349,350,351,352,353,354,355,59,
  37,365,65,342,343,182,81,324,161,151,347,348,162,345,163,346,230,265,
  375,374,373,372,371,370,370,370,369,368,367,150,370,366,364,363,362,361,
  360,359,358,265,357,356,
202,199,197,195,201,200,198,196,325,229,389,
82,82,82,82,82,82,82,173,173,341,340,344,82,349,350,351,352,353,354,355,59,
  37,365,65,342,343,182,81,326,161,151,347,348,162,345,163,346,226,265,
  375,374,373,372,371,370,370,370,369,368,367,150,370,366,364,363,362,361,
  360,359,358,265,357,356,
82,82,82,82,82,82,82,173,173,341,340,344,82,349,350,351,352,353,354,355,59,
  37,365,65,342,343,182,81,327,161,151,347,348,162,345,163,346,225,265,
  375,374,373,372,371,370,370,370,369,368,367,150,370,366,364,363,362,361,
  360,359,358,265,357,356,
82,82,82,82,82,82,82,173,173,341,340,344,82,349,350,351,352,353,354,355,59,
  37,365,65,342,343,182,81,328,161,151,347,348,162,345,163,346,224,265,
  375,374,373,372,371,370,370,370,369,368,367,150,370,366,364,363,362,361,
  360,359,358,265,357,356,
82,82,82,82,82,82,82,173,173,341,340,344,82,349,350,351,352,353,354,355,59,
  37,365,65,342,343,182,81,329,161,151,347,348,162,345,163,346,222,265,
  375,374,373,372,371,370,370,370,369,368,367,150,370,366,364,363,362,361,
  360,359,358,265,357,356,
202,199,197,195,201,200,198,196,330,221,389,
82,82,82,82,82,82,82,173,173,341,340,344,82,349,350,351,352,353,354,355,59,
  37,365,65,342,343,182,81,331,161,151,347,348,162,345,163,346,220,265,
  375,374,373,372,371,370,370,370,369,368,367,150,370,366,364,363,362,361,
  360,359,358,265,357,356,
202,199,197,195,201,200,198,196,332,219,389,
202,199,197,195,201,200,198,196,333,218,389,
82,82,82,82,82,82,82,173,173,341,340,344,82,349,350,351,352,353,354,355,59,
  37,365,65,342,343,182,81,334,161,151,347,348,162,345,163,346,217,265,
  375,374,373,372,371,370,370,370,369,368,367,150,370,366,364,363,362,361,
  360,359,358,265,357,356,
71,72,73,74,75,76,77,78,79,80,81,82,83,84,85,86,87,88,89,90,91,92,93,94,95,
  96,97,98,99,100,101,102,103,104,105,106,107,108,109,110,111,112,113,114,
  115,116,117,118,119,120,121,122,123,124,125,126,127,128,129,130,131,132,
  133,134,135,136,137,138,139,140,141,142,143,144,145,146,147,148,149,150,
  151,152,153,154,155,156,157,158,159,160,161,65,71,215,216,220,219,218,
  217,216,215,223,214,213,212,227,228,211,210,209,208,207,206,205,204,237,
  203,202,201,241,242,243,244,200,199,198,197,196,195,194,193,192,191,190,
  189,188,187,186,185,184,183,182,181,180,266,179,178,177,270,176,175,174,
  274,173,172,277,278,279,280,281,282,283,284,285,286,287,288,289,290,171,
  292,170,169,168,296,297,298,167,166,301,165,164,304,163,162,307,
173,173,341,344,342,343,182,16,336,66,347,399,163,346,357,
183,400,183,183,183,183,183,183,183,183,183,183,183,183,183,183,183,183,183,
  183,183,183,183,183,183,183,183,183,183,183,183,183,189,183,337,401,
183,400,183,183,183,183,183,183,183,183,183,183,183,183,87,183,183,183,183,
  183,183,183,183,183,183,183,183,183,183,183,183,183,183,189,183,338,89,
542,542,542,542,542,1,542,339,2,2,2,582,
193,193,193,340,
402,173,
172,172,172,342,
171,171,171,343,
183,400,183,183,183,183,183,183,183,183,183,183,183,183,183,183,183,183,183,
  183,183,183,183,183,183,183,183,183,183,183,183,183,189,183,344,403,
194,194,194,191,
180,180,180,180,166,346,
178,178,178,178,165,
175,179,174,174,174,192,176,167,170,168,169,164,
542,1,2,349,2,2,2,631,
542,1,2,350,2,2,2,630,
542,1,2,351,2,2,2,629,
542,1,2,352,2,2,2,628,
542,1,2,353,2,2,2,627,
542,1,2,354,2,2,2,626,
542,1,2,355,2,2,2,625,
542,542,542,542,542,542,542,542,542,542,542,542,542,542,542,542,542,542,542,
  542,542,542,542,542,542,542,542,542,542,542,542,542,542,542,542,542,542,
  542,542,542,542,542,542,542,542,542,542,542,542,542,542,542,542,542,542,
  542,542,542,542,542,542,542,542,542,542,542,542,542,542,542,542,542,542,
  542,542,542,542,542,542,542,542,542,542,542,542,542,542,542,542,542,542,
  542,542,542,542,542,542,542,542,542,542,542,542,542,542,542,542,542,542,
  542,542,542,542,542,542,542,542,542,542,542,542,542,542,542,542,542,542,
  542,1,542,356,2,2,2,632,
542,542,542,542,542,542,542,542,542,542,542,542,542,542,542,542,542,542,542,
  542,542,542,542,542,542,542,542,542,542,542,542,542,542,542,542,542,542,
  542,542,542,542,542,542,542,542,542,542,542,542,542,542,542,542,542,542,
  542,542,542,542,542,542,542,542,542,542,542,542,542,542,542,542,542,542,
  542,542,542,542,542,542,542,542,542,542,542,542,542,542,542,542,542,542,
  542,542,542,542,542,542,542,542,542,542,542,542,542,542,542,542,542,542,
  542,542,542,542,542,542,542,542,542,542,542,542,542,542,542,542,542,542,
  542,1,2,357,2,2,2,584,
59,358,404,
59,359,405,
59,360,406,
59,361,407,
59,362,408,
59,363,409,
59,364,410,
542,542,542,542,542,542,542,542,542,542,542,542,542,542,542,542,542,542,542,
  542,542,542,542,542,542,1,2,365,2,2,2,621,
82,82,82,82,82,82,82,173,173,341,340,344,82,349,350,351,352,353,354,355,59,
  37,365,65,342,343,182,81,366,161,151,347,348,162,345,163,346,411,265,
  375,374,373,372,371,370,370,370,369,368,367,150,370,366,364,363,362,361,
  360,359,358,265,357,356,
82,82,82,82,82,82,82,173,173,341,340,344,82,349,350,351,352,353,354,355,59,
  342,343,182,81,367,161,151,347,348,162,345,163,346,265,149,150,149,366,
  364,363,362,361,360,359,358,265,357,356,
82,82,82,82,82,82,82,173,173,341,340,344,82,349,350,351,352,353,354,355,59,
  342,343,182,81,368,161,151,347,348,162,345,163,346,265,148,150,148,366,
  364,363,362,361,360,359,358,265,357,356,
82,82,82,82,82,82,82,173,173,341,340,344,82,349,350,351,352,353,354,355,59,
  342,343,182,81,369,161,151,347,348,162,345,163,346,265,147,150,147,366,
  364,363,362,361,360,359,358,265,357,356,
412,414,62,53,57,137,418,417,416,415,413,
55,56,132,420,419,
421,423,425,427,129,428,426,424,422,
429,61,126,431,430,
432,40,123,434,433,
435,38,122,437,436,
438,439,440,441,442,443,444,445,446,447,448,449,450,102,106,106,106,106,109,
  109,109,112,112,112,115,115,115,118,118,118,121,121,121,
82,82,82,82,82,82,82,173,173,341,340,344,82,349,350,351,352,353,354,355,59,
  37,365,65,342,343,182,81,377,161,151,347,348,162,345,163,346,101,265,
  375,374,373,372,371,370,370,370,369,368,367,150,370,366,364,363,362,361,
  360,359,358,265,357,356,
17,17,17,17,5,378,5,61,
451,56,56,56,56,5,379,5,451,
451,55,55,55,55,5,380,5,451,
451,54,54,54,54,5,381,5,451,
183,400,183,183,183,183,183,183,183,183,183,183,183,183,183,183,183,183,183,
  183,183,183,183,183,183,183,183,183,183,183,183,183,189,183,382,452,
453,17,17,17,17,5,383,5,453,
454,17,17,17,17,5,384,5,454,
455,17,17,17,17,5,385,5,455,
456,17,17,17,17,5,386,5,456,
98,98,98,98,98,98,98,98,98,98,98,98,98,98,98,98,98,98,98,98,98,98,98,98,98,
  98,98,98,98,98,98,99,98,98,98,387,
93,93,93,93,93,93,93,93,93,93,93,93,93,93,93,93,93,93,93,93,93,93,93,93,93,
  93,93,93,93,93,93,94,93,93,93,388,
542,542,542,542,542,542,542,542,542,542,542,542,542,542,542,542,542,542,542,
  542,542,542,542,542,542,542,542,542,542,542,542,542,542,542,542,542,542,
  542,542,542,542,542,542,542,542,542,542,542,542,542,542,542,542,542,542,
  542,542,542,542,542,542,542,542,542,542,542,542,542,542,542,542,542,542,
  542,542,542,542,542,542,542,542,542,542,542,542,542,542,542,542,542,542,
  542,542,542,542,542,542,1,542,389,2,2,2,639,
542,542,542,542,542,542,542,542,542,542,542,542,542,542,542,542,542,542,542,
  542,542,542,542,542,542,542,542,542,542,542,542,542,542,542,542,542,542,
  542,542,542,542,542,542,542,542,542,542,542,542,542,542,542,542,542,542,
  542,542,542,542,542,542,542,542,542,542,542,542,542,542,542,542,542,542,
  542,542,542,542,542,542,542,542,542,542,542,542,542,542,542,542,542,542,
  542,542,542,542,542,1,542,390,2,2,2,634,
542,542,542,542,542,542,542,542,542,542,542,542,542,542,542,542,542,542,542,
  542,542,542,542,542,542,542,542,542,542,542,542,542,542,542,542,542,542,
  542,542,542,542,542,542,542,542,542,542,542,542,542,542,542,542,542,542,
  542,542,542,542,542,542,542,542,542,542,542,542,542,542,542,542,542,542,
  542,542,542,542,542,542,542,542,542,542,542,542,542,542,542,542,542,542,
  542,542,542,542,542,1,542,391,2,2,2,633,
542,542,542,542,542,542,542,542,542,542,542,542,542,542,542,542,542,542,542,
  542,542,542,542,542,542,542,542,542,542,542,542,542,542,542,542,542,542,
  542,542,542,542,542,542,542,542,542,542,542,542,542,542,542,542,542,542,
  542,542,542,542,542,542,542,542,542,542,542,542,542,542,542,542,542,542,
  542,542,542,542,542,542,542,542,542,542,542,542,542,542,542,542,542,542,
  542,542,542,542,542,1,542,392,2,2,2,637,
542,542,542,542,542,542,542,542,542,542,542,542,542,542,542,542,542,542,542,
  542,542,542,542,542,542,542,542,542,542,542,542,542,542,542,542,542,542,
  542,542,542,542,542,542,542,542,542,542,542,542,542,542,542,542,542,542,
  542,542,542,542,542,542,542,542,542,542,542,542,542,542,542,542,542,542,
  542,542,542,542,542,542,542,542,542,542,542,542,542,542,542,542,542,542,
  542,542,542,542,542,1,542,393,2,2,2,636,
542,542,542,542,542,542,542,542,542,542,542,542,542,542,542,542,542,542,542,
  542,542,542,542,542,542,542,542,542,542,542,542,542,542,542,542,542,542,
  542,542,542,542,542,542,542,542,542,542,542,542,542,542,542,542,542,542,
  542,542,542,542,542,542,542,542,542,542,542,542,542,542,542,542,542,542,
  542,542,542,542,542,542,542,542,542,542,542,542,542,542,542,542,542,542,
  542,542,542,542,542,1,542,394,2,2,2,635,
457,5,395,5,457,
458,5,396,5,458,
542,542,542,542,542,542,542,542,542,542,542,542,542,542,542,542,542,542,542,
  542,542,542,542,542,542,542,542,542,542,542,542,542,542,542,542,542,542,
  542,542,542,542,542,542,542,542,542,542,542,542,542,542,542,542,542,542,
  542,542,542,542,542,542,542,542,542,542,542,542,542,542,542,542,542,542,
  542,542,542,542,542,542,542,542,542,542,542,542,542,542,542,542,542,542,
  542,542,542,542,542,1,542,397,2,2,2,660,
459,5,398,5,459,
175,179,174,174,174,176,167,170,168,169,164,
184,186,187,185,188,190,400,
183,400,183,183,183,183,183,183,183,183,183,183,183,183,183,183,183,183,183,
  183,183,183,183,183,183,183,183,183,183,183,183,183,189,183,401,88,
177,177,177,177,402,
181,403,
82,82,82,82,82,82,82,173,173,341,340,344,82,349,350,351,352,353,354,355,59,
  37,365,65,342,343,182,81,404,161,151,347,348,162,345,163,346,460,265,
  375,374,373,372,371,370,370,370,369,368,367,150,370,366,364,363,362,361,
  360,359,358,265,357,356,
82,82,82,82,82,82,82,173,173,341,340,344,82,349,350,351,352,353,354,355,59,
  37,365,65,342,343,182,81,405,161,151,347,348,162,345,163,346,461,265,
  375,374,373,372,371,370,370,370,369,368,367,150,370,366,364,363,362,361,
  360,359,358,265,357,356,
82,82,82,82,82,82,82,173,173,341,340,344,82,349,350,351,352,353,354,355,59,
  37,365,65,342,343,182,81,406,161,151,347,348,162,345,163,346,462,265,
  375,374,373,372,371,370,370,370,369,368,367,150,370,366,364,363,362,361,
  360,359,358,265,357,356,
82,82,82,82,82,82,82,173,173,341,340,344,82,349,350,351,352,353,354,355,59,
  37,365,65,342,343,182,81,407,161,151,347,348,162,345,163,346,463,265,
  375,374,373,372,371,370,370,370,369,368,367,150,370,366,364,363,362,361,
  360,359,358,265,357,356,
82,82,82,82,82,82,82,173,173,341,340,344,82,349,350,351,352,353,354,355,59,
  37,365,65,342,343,182,81,408,161,151,347,348,162,345,163,346,464,265,
  375,374,373,372,371,370,370,370,369,368,367,150,370,366,364,363,362,361,
  360,359,358,265,357,356,
82,82,82,82,82,82,82,173,173,341,340,344,82,349,350,351,352,353,354,355,59,
  37,365,65,342,343,182,81,409,161,151,347,348,162,345,163,346,465,265,
  375,374,373,372,371,370,370,370,369,368,367,150,370,366,364,363,362,361,
  360,359,358,265,357,356,
82,82,82,82,82,82,82,173,173,341,340,344,82,349,350,351,352,353,354,355,59,
  37,365,65,342,343,182,81,410,161,151,347,348,162,345,163,346,466,265,
  375,374,373,372,371,370,370,370,369,368,367,150,370,366,364,363,362,361,
  360,359,358,265,357,356,
58,411,467,
542,542,542,542,542,542,542,542,542,542,542,542,542,542,542,542,542,542,542,
  542,542,542,542,542,542,1,542,412,2,2,2,619,
468,468,468,468,468,468,468,468,468,468,468,468,468,468,468,468,468,468,468,
  468,468,468,468,468,468,5,413,5,468,
542,542,542,542,542,542,542,542,542,542,542,542,542,542,542,542,542,542,542,
  542,542,542,542,542,542,1,542,414,2,2,2,618,
469,469,469,469,469,469,469,469,469,469,469,469,469,469,469,469,469,469,469,
  469,469,469,469,469,469,5,415,5,469,
470,470,470,470,470,470,470,470,470,470,470,470,470,470,470,470,470,470,470,
  470,470,470,470,470,470,5,416,5,470,
471,471,471,471,471,471,471,471,471,471,471,471,471,471,471,471,471,471,471,
  471,471,471,471,471,471,5,417,5,471,
472,472,472,472,472,472,472,472,472,472,472,472,472,472,472,472,472,472,472,
  472,472,472,472,472,472,5,418,5,472,
473,473,473,473,473,473,473,473,473,473,473,473,473,473,473,473,473,473,473,
  473,473,473,473,473,473,473,473,473,5,419,5,473,
474,474,474,474,474,474,474,474,474,474,474,474,474,474,474,474,474,474,474,
  474,474,474,474,474,474,474,474,474,5,420,5,474,
542,542,542,542,542,542,542,542,542,542,542,542,542,542,542,542,542,542,542,
  542,542,542,542,542,542,542,542,542,1,542,421,2,2,2,612,
475,475,475,475,475,475,475,475,475,475,475,475,475,475,475,475,475,475,475,
  475,475,475,475,475,475,475,475,475,5,422,5,475,
542,542,542,542,542,542,542,542,542,542,542,542,542,542,542,542,542,542,542,
  542,542,542,542,542,542,542,542,542,1,542,423,2,2,2,611,
476,476,476,476,476,476,476,476,476,476,476,476,476,476,476,476,476,476,476,
  476,476,476,476,476,476,476,476,476,5,424,5,476,
542,542,542,542,542,542,542,542,542,542,542,542,542,542,542,542,542,542,542,
  542,542,542,542,542,542,542,542,542,1,542,425,2,2,2,610,
477,477,477,477,477,477,477,477,477,477,477,477,477,477,477,477,477,477,477,
  477,477,477,477,477,477,477,477,477,5,426,5,477,
542,542,542,542,542,542,542,542,542,542,542,542,542,542,542,542,542,542,542,
  542,542,542,542,542,542,542,542,542,1,542,427,2,2,2,609,
478,478,478,478,478,478,478,478,478,478,478,478,478,478,478,478,478,478,478,
  478,478,478,478,478,478,478,478,478,5,428,5,478,
542,542,542,542,542,542,542,542,542,542,542,542,542,542,542,542,542,542,542,
  542,542,542,542,542,542,542,542,542,1,542,429,2,2,2,608,
479,479,479,479,479,479,479,479,479,479,479,479,479,479,479,479,479,479,479,
  479,479,479,479,479,479,479,479,479,5,430,5,479,
480,480,480,480,480,480,480,480,480,480,480,480,480,480,480,480,480,480,480,
  480,480,480,480,480,480,480,480,480,5,431,5,480,
542,542,542,542,542,542,542,542,542,542,542,542,542,542,542,542,542,542,542,
  542,542,542,542,542,542,542,542,542,1,542,432,2,2,2,606,
481,481,481,481,481,481,481,481,481,481,481,481,481,481,481,481,481,481,481,
  481,481,481,481,481,481,481,481,481,5,433,5,481,
482,482,482,482,482,482,482,482,482,482,482,482,482,482,482,482,482,482,482,
  482,482,482,482,482,482,482,482,482,5,434,5,482,
542,542,542,542,542,542,542,542,542,542,542,542,542,542,542,542,542,542,542,
  542,542,542,542,542,542,542,542,542,1,542,435,2,2,2,604,
483,483,483,483,483,483,483,483,483,483,483,483,483,483,483,483,483,483,483,
  483,483,483,483,483,483,483,483,483,5,436,5,483,
484,484,484,484,484,484,484,484,484,484,484,484,484,484,484,484,484,484,484,
  484,484,484,484,484,484,484,484,484,5,437,5,484,
542,542,542,542,542,542,542,542,542,542,542,542,542,542,542,542,542,542,542,
  542,542,542,542,542,542,542,542,542,1,2,438,2,2,2,602,
542,542,542,542,542,542,542,542,542,542,542,542,542,542,542,542,542,542,542,
  542,542,542,542,542,542,542,542,542,1,2,439,2,2,2,601,
542,542,542,542,542,542,542,542,542,542,542,542,542,542,542,542,542,542,542,
  542,542,542,542,542,542,542,542,542,1,2,440,2,2,2,600,
542,542,542,542,542,542,542,542,542,542,542,542,542,542,542,542,542,542,542,
  542,542,542,542,542,542,542,542,542,1,2,441,2,2,2,599,
542,542,542,542,542,542,542,542,542,542,542,542,542,542,542,542,542,542,542,
  542,542,542,542,542,542,542,542,542,1,2,442,2,2,2,598,
542,542,542,542,542,542,542,542,542,542,542,542,542,542,542,542,542,542,542,
  542,542,542,542,542,542,542,542,542,1,2,443,2,2,2,597,
542,542,542,542,542,542,542,542,542,542,542,542,542,542,542,542,542,542,542,
  542,542,542,542,542,542,542,542,542,1,2,444,2,2,2,596,
542,542,542,542,542,542,542,542,542,542,542,542,542,542,542,542,542,542,542,
  542,542,542,542,542,542,542,542,542,1,2,445,2,2,2,595,
542,542,542,542,542,542,542,542,542,542,542,542,542,542,542,542,542,542,542,
  542,542,542,542,542,542,542,542,542,1,2,446,2,2,2,594,
542,542,542,542,542,542,542,542,542,542,542,542,542,542,542,542,542,542,542,
  542,542,542,542,542,542,542,542,542,1,2,447,2,2,2,593,
542,542,542,542,542,542,542,542,542,542,542,542,542,542,542,542,542,542,542,
  542,542,542,542,542,542,542,542,542,1,2,448,2,2,2,592,
542,542,542,542,542,542,542,542,542,542,542,542,542,542,542,542,542,542,542,
  542,542,542,542,542,542,542,542,542,1,2,449,2,2,2,591,
542,542,542,542,542,542,542,542,542,542,542,542,542,542,542,542,542,542,542,
  542,542,542,542,542,542,542,542,542,1,2,450,2,2,2,590,
485,451,486,
183,400,183,183,183,183,183,183,183,183,183,183,183,183,181,183,183,183,183,
  183,183,183,183,183,183,183,183,183,183,183,183,183,183,189,183,452,88,
485,53,487,
485,51,487,
485,52,487,
485,50,487,
485,457,488,
485,458,489,
8,459,490,
58,460,160,
58,461,159,
58,462,158,
58,463,157,
58,464,156,
58,465,155,
58,466,154,
17,17,17,17,17,17,17,17,17,17,17,17,17,17,17,17,17,17,17,17,17,17,17,17,17,
  17,17,17,17,17,17,17,17,17,17,17,17,17,17,17,17,17,17,17,17,17,17,17,17,
  17,17,17,17,17,17,17,17,17,17,17,17,17,17,17,17,17,17,17,17,17,17,17,17,
  17,17,17,17,17,17,17,17,17,17,17,17,17,17,17,17,17,17,17,17,17,17,17,17,
  17,17,17,17,17,17,17,17,17,17,17,17,17,17,17,17,17,17,17,17,17,17,17,17,
  17,17,17,17,17,17,17,5,467,5,153,
82,82,82,82,82,82,82,173,173,341,340,344,82,349,350,351,352,353,354,355,59,
  342,343,182,81,468,161,151,347,348,162,345,163,346,265,145,150,145,366,
  364,363,362,361,360,359,358,265,357,356,
82,82,82,82,82,82,82,173,173,341,340,344,82,349,350,351,352,353,354,355,59,
  342,343,182,81,469,161,151,347,348,162,345,163,346,265,144,150,144,366,
  364,363,362,361,360,359,358,265,357,356,
82,82,82,82,82,82,82,173,173,341,340,344,82,349,350,351,352,353,354,355,59,
  342,343,182,81,470,161,151,347,348,162,345,163,346,265,143,150,143,366,
  364,363,362,361,360,359,358,265,357,356,
82,82,82,82,82,82,82,173,173,341,340,344,82,349,350,351,352,353,354,355,59,
  342,343,182,81,471,161,151,347,348,162,345,163,346,265,142,150,142,366,
  364,363,362,361,360,359,358,265,357,356,
82,82,82,82,82,82,82,173,173,341,340,344,82,349,350,351,352,353,354,355,59,
  342,343,182,81,472,161,151,347,348,162,345,163,346,265,141,150,141,366,
  364,363,362,361,360,359,358,265,357,356,
82,82,82,82,82,82,82,173,173,341,340,344,82,349,350,351,352,353,354,355,59,
  37,365,65,342,343,182,81,473,161,151,347,348,162,345,163,346,265,491,
  491,491,369,368,367,150,491,366,364,363,362,361,360,359,358,265,357,356,
82,82,82,82,82,82,82,173,173,341,340,344,82,349,350,351,352,353,354,355,59,
  37,365,65,342,343,182,81,474,161,151,347,348,162,345,163,346,265,492,
  492,492,369,368,367,150,492,366,364,363,362,361,360,359,358,265,357,356,
82,82,82,82,82,82,82,173,173,341,340,344,82,349,350,351,352,353,354,355,59,
  37,365,65,342,343,182,81,475,161,151,347,348,162,345,163,346,265,493,
  370,370,370,369,368,367,150,370,366,364,363,362,361,360,359,358,265,357,
  356,
82,82,82,82,82,82,82,173,173,341,340,344,82,349,350,351,352,353,354,355,59,
  37,365,65,342,343,182,81,476,161,151,347,348,162,345,163,346,265,494,
  370,370,370,369,368,367,150,370,366,364,363,362,361,360,359,358,265,357,
  356,
82,82,82,82,82,82,82,173,173,341,340,344,82,349,350,351,352,353,354,355,59,
  37,365,65,342,343,182,81,477,161,151,347,348,162,345,163,346,265,495,
  370,370,370,369,368,367,150,370,366,364,363,362,361,360,359,358,265,357,
  356,
82,82,82,82,82,82,82,173,173,341,340,344,82,349,350,351,352,353,354,355,59,
  37,365,65,342,343,182,81,478,161,151,347,348,162,345,163,346,265,496,
  370,370,370,369,368,367,150,370,366,364,363,362,361,360,359,358,265,357,
  356,
82,82,82,82,82,82,82,173,173,341,340,344,82,349,350,351,352,353,354,355,59,
  37,365,65,342,343,182,81,479,161,151,347,348,162,345,163,346,265,497,
  371,370,370,370,369,368,367,150,370,366,364,363,362,361,360,359,358,265,
  357,356,
82,82,82,82,82,82,82,173,173,341,340,344,82,349,350,351,352,353,354,355,59,
  37,365,65,342,343,182,81,480,161,151,347,348,162,345,163,346,265,498,
  371,370,370,370,369,368,367,150,370,366,364,363,362,361,360,359,358,265,
  357,356,
82,82,82,82,82,82,82,173,173,341,340,344,82,349,350,351,352,353,354,355,59,
  37,365,65,342,343,182,81,481,161,151,347,348,162,345,163,346,265,499,
  372,371,370,370,370,369,368,367,150,370,366,364,363,362,361,360,359,358,
  265,357,356,
82,82,82,82,82,82,82,173,173,341,340,344,82,349,350,351,352,353,354,355,59,
  37,365,65,342,343,182,81,482,161,151,347,348,162,345,163,346,265,500,
  372,371,370,370,370,369,368,367,150,370,366,364,363,362,361,360,359,358,
  265,357,356,
82,82,82,82,82,82,82,173,173,341,340,344,82,349,350,351,352,353,354,355,59,
  37,365,65,342,343,182,81,483,161,151,347,348,162,345,163,346,265,501,
  373,372,371,370,370,370,369,368,367,150,370,366,364,363,362,361,360,359,
  358,265,357,356,
82,82,82,82,82,82,82,173,173,341,340,344,82,349,350,351,352,353,354,355,59,
  37,365,65,342,343,182,81,484,161,151,347,348,162,345,163,346,265,502,
  373,372,371,370,370,370,369,368,367,150,370,366,364,363,362,361,360,359,
  358,265,357,356,
542,542,542,542,542,542,542,542,542,542,542,542,542,542,542,542,542,542,542,
  542,542,542,542,542,542,542,542,542,542,542,542,542,542,542,542,542,542,
  1,542,485,2,2,2,589,
503,503,503,503,503,503,503,503,503,5,486,5,503,
504,504,504,504,504,504,504,504,504,504,504,504,504,504,504,504,504,504,504,
  504,504,504,504,504,504,504,504,504,504,5,487,5,504,
505,505,505,505,505,505,505,505,505,505,505,505,505,505,505,505,505,505,505,
  505,505,505,505,505,505,505,505,505,5,488,5,505,
506,506,506,506,506,506,506,506,5,489,5,506,
507,507,507,507,507,507,507,507,507,507,507,507,507,507,507,507,507,507,507,
  507,507,507,507,507,507,507,507,507,5,490,5,507,
412,414,62,53,57,139,418,417,416,415,413,
412,414,62,53,57,138,418,417,416,415,413,
55,56,136,420,419,
55,56,135,420,419,
55,56,134,420,419,
55,56,133,420,419,
421,423,425,427,131,428,426,424,422,
421,423,425,427,130,428,426,424,422,
429,61,128,431,430,
429,61,127,431,430,
432,40,125,434,433,
432,40,124,434,433,
82,82,82,82,82,82,82,82,81,503,79,265,265,
82,82,82,82,82,82,82,173,173,341,340,382,82,91,349,350,351,352,353,354,355,
  59,37,365,65,342,343,182,81,504,161,151,347,348,162,345,76,270,163,346,
  338,77,75,265,375,374,373,372,371,370,370,370,369,368,367,150,370,366,
  364,363,362,361,360,359,358,271,265,339,357,356,
82,82,82,82,82,82,82,173,173,341,340,344,82,349,350,351,352,353,354,355,59,
  37,365,65,342,343,182,81,505,161,151,347,348,162,345,163,346,269,265,
  375,374,373,372,371,370,370,370,369,368,367,150,370,366,364,363,362,361,
  360,359,358,265,357,356,
202,199,197,195,201,200,198,196,506,268,389,
82,82,82,82,82,82,82,173,173,341,340,344,82,349,350,351,352,353,354,355,59,
  37,365,65,342,343,182,81,507,161,151,347,348,162,345,163,346,267,265,
  375,374,373,372,371,370,370,370,369,368,367,150,370,366,364,363,362,361,
  360,359,358,265,357,356,

};


static const unsigned short ag_sbt[] = {
     0, 141, 178, 183, 326, 363, 365, 523, 665, 716, 757, 798, 869, 940,
  1011,1031,1047,1063,1074,1085,1101,1117,1126,1135,1157,1282,1509,1520,
  1533,1546,1559,1572,1577,1586,1610,1731,1741,1782,1832,1883,1924,1975,
  2016,2057,2098,2139,2180,2221,2262,2303,2344,2385,2518,2651,2701,2742,
  2793,2844,2894,3050,3101,3142,3193,3243,3284,3325,3467,3508,3576,3579,
  3582,3585,3688,3723,3738,3841,3876,3891,3994,4003,4038,4141,4244,4347,
  4382,4417,4432,4535,4544,4647,4750,4853,4956,5059,5162,5265,5368,5471,
  5574,5677,5780,5883,5986,5998,6010,6113,6148,6183,6198,6301,6316,6331,
  6342,6445,6480,6515,6550,6559,6594,6629,6664,6699,6734,6769,6804,6839,
  6874,6909,6944,6979,7014,7049,7060,7075,7110,7213,7316,7419,7522,7533,
  7548,7559,7662,7697,7732,7767,7802,7837,7872,7907,7922,8025,8128,8163,
  8198,8233,8336,8371,8386,8421,8436,8451,8486,8518,8530,8562,8574,8580,
  8612,8644,8676,8688,8694,8703,8712,8744,8776,8788,8800,8812,8820,8852,
  8884,8916,8922,8954,8986,9018,9050,9082,9114,9146,9178,9210,9242,9274,
  9306,9338,9370,9378,9390,9422,9430,9442,9450,9482,9514,9546,9578,9610,
  9642,9674,9686,9718,9750,9782,9814,9826,9858,9870,9882,9914,10014,10030,
  10043,10063,10079,10090,10101,10104,10113,10122,10133,10144,10179,10244,
  10264,10277,10312,10375,10391,10404,10420,10434,10450,10464,10480,10494,
  10530,10601,10637,10708,10743,10806,10817,10828,10839,10874,10937,11008,
  11079,11142,11177,11240,11275,11338,11382,11393,11404,11438,11449,11484,
  11496,11519,11582,11645,11787,11850,11861,11924,11935,11941,12004,12067,
  12130,12141,12146,12158,12170,12233,12296,12307,12318,12329,12336,12399,
  12462,12525,12531,12594,12657,12720,12783,12846,12909,12972,13035,13098,
  13161,13224,13287,13350,13413,13420,13431,13494,13501,13512,13519,13582,
  13645,13708,13771,13834,13897,13960,13971,14034,14097,14160,14223,14234,
  14297,14308,14319,14382,14568,14583,14619,14656,14668,14672,14674,14678,
  14682,14718,14722,14728,14733,14745,14753,14761,14769,14777,14785,14793,
  14801,14936,15071,15074,15077,15080,15083,15086,15089,15092,15124,15187,
  15236,15285,15334,15345,15350,15359,15364,15369,15374,15407,15470,15478,
  15487,15496,15505,15541,15550,15559,15568,15577,15613,15649,15753,15856,
  15959,16062,16165,16268,16273,16278,16381,16386,16397,16404,16440,16445,
  16447,16510,16573,16636,16699,16762,16825,16888,16891,16923,16952,16984,
  17013,17042,17071,17100,17132,17164,17199,17231,17266,17298,17333,17365,
  17400,17432,17467,17499,17531,17566,17598,17630,17665,17697,17729,17764,
  17799,17834,17869,17904,17939,17974,18009,18044,18079,18114,18149,18184,
  18187,18224,18227,18230,18233,18236,18239,18242,18245,18248,18251,18254,
  18257,18260,18263,18266,18398,18447,18496,18545,18594,18643,18700,18757,
  18815,18873,18931,18989,19048,19107,19167,19227,19288,19349,19393,19406,
  19439,19471,19483,19515,19526,19537,19542,19547,19552,19557,19566,19575,
  19580,19585,19590,19595,19608,19678,19741,19752,19815
};


static const unsigned short ag_sbe[] = {
   136, 176, 180, 319, 362, 364, 500, 660, 711, 752, 793, 833, 904, 975,
  1025,1042,1058,1069,1080,1096,1112,1121,1130,1152,1277,1394,1514,1529,
  1542,1555,1568,1574,1579,1601,1728,1735,1777,1827,1878,1919,1970,2011,
  2052,2093,2134,2175,2216,2257,2298,2339,2380,2513,2646,2696,2737,2788,
  2839,2889,3045,3096,3137,3188,3238,3279,3320,3462,3503,3542,3577,3580,
  3583,3683,3718,3733,3836,3871,3886,3989,3998,4033,4136,4239,4342,4377,
  4412,4427,4530,4539,4642,4745,4848,4951,5054,5157,5260,5363,5466,5569,
  5672,5775,5878,5981,5993,6005,6108,6143,6178,6193,6296,6311,6326,6337,
  6440,6475,6510,6545,6554,6589,6624,6659,6694,6729,6764,6799,6834,6869,
  6904,6939,6974,7009,7044,7055,7070,7105,7208,7311,7414,7517,7528,7543,
  7554,7657,7692,7727,7762,7797,7832,7867,7902,7917,8020,8123,8158,8193,
  8228,8331,8366,8381,8416,8431,8446,8481,8515,8527,8559,8571,8577,8609,
  8641,8673,8685,8691,8700,8709,8741,8773,8785,8797,8809,8817,8849,8881,
  8913,8919,8951,8983,9015,9047,9079,9111,9143,9175,9207,9239,9271,9303,
  9335,9367,9375,9387,9419,9427,9439,9447,9479,9511,9543,9575,9607,9639,
  9671,9683,9715,9747,9779,9811,9823,9855,9867,9879,9911,10011,10025,10039,
  10058,10076,10085,10096,10102,10108,10115,10128,10139,10174,10207,10259,
  10273,10307,10340,10386,10400,10415,10429,10445,10459,10475,10489,10525,
  10559,10632,10666,10738,10771,10812,10823,10834,10869,10902,10966,11037,
  11107,11172,11205,11270,11303,11376,11388,11399,11437,11444,11483,11491,
  11514,11547,11610,11782,11815,11858,11889,11932,11937,11969,12032,12095,
  12138,12143,12151,12163,12198,12261,12304,12315,12326,12333,12364,12427,
  12490,12527,12559,12622,12685,12748,12811,12874,12937,13000,13063,13126,
  13189,13252,13315,13378,13417,13428,13459,13498,13509,13516,13547,13610,
  13673,13736,13799,13862,13925,13968,13999,14062,14125,14188,14231,14262,
  14305,14316,14347,14474,14576,14617,14654,14663,14671,14673,14677,14681,
  14716,14721,14727,14732,14744,14748,14756,14764,14772,14780,14788,14796,
  14931,15066,15072,15075,15078,15081,15084,15087,15090,15119,15152,15212,
  15261,15310,15339,15347,15354,15361,15366,15371,15387,15435,15475,15484,
  15493,15502,15539,15547,15556,15565,15574,15612,15648,15748,15851,15954,
  16057,16160,16263,16270,16275,16376,16383,16396,16403,16438,16444,16446,
  16475,16538,16601,16664,16727,16790,16853,16889,16918,16949,16979,17010,
  17039,17068,17097,17129,17161,17194,17228,17261,17295,17328,17362,17395,
  17429,17462,17496,17528,17561,17595,17627,17660,17694,17726,17759,17794,
  17829,17864,17899,17934,17969,18004,18039,18074,18109,18144,18179,18185,
  18222,18225,18228,18231,18234,18237,18240,18243,18246,18249,18252,18255,
  18258,18261,18264,18395,18423,18472,18521,18570,18619,18671,18728,18785,
  18843,18901,18959,19017,19076,19135,19195,19255,19316,19388,19403,19436,
  19468,19480,19512,19520,19531,19539,19544,19549,19554,19561,19570,19577,
  19582,19587,19592,19604,19637,19706,19749,19780,19815
};


static const unsigned char ag_fl[] = {
  2,1,1,2,2,1,2,0,1,3,3,3,1,2,3,1,2,0,1,3,3,2,2,2,1,2,2,2,2,2,2,2,2,2,1,
  1,2,2,3,3,3,3,0,1,3,2,2,2,3,3,4,4,4,4,3,3,3,3,3,2,3,4,2,2,3,3,4,2,2,3,
  3,3,1,1,1,5,5,5,1,5,1,1,1,2,2,1,2,2,3,2,2,1,2,3,3,2,1,2,3,3,2,2,1,1,1,
  1,2,1,1,2,1,1,2,1,1,2,1,1,2,1,1,2,1,1,4,4,1,4,4,1,4,4,1,4,4,4,4,1,4,4,
  1,4,4,4,4,4,1,2,2,2,1,1,1,4,4,4,4,4,4,4,4,1,1,1,1,1,2,2,2,2,2,2,2,1,2,
  2,2,3,2,2,2,3,1,1,2,2,2,2,2,1,2,1,2,2,2,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,
  1,1,1,1,1,3,3,3,3,3,3,3,3,1,3,3,3,1,1,3,3,3,3,3,3,3,3,1,3,3,3,1,1,1,1,
  3,3,3,3,3,3,3,3,3,3,3,3,3,3,3,3,3,3,3,3,3,1,7,7,7,1,3,3,3,1,3,3,1,1,1,
  1,1,1,1,1,1,1,1,1,1,1,3,1,3,3,3,1,1,1,3,3,1,3,3,1,3,3,1,1,1,1,1,1,1,1,
  1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,
  1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,
  1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,
  1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,
  1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,
  1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,
  1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,2,0,1,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,
  2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,
  2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,
  2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,
  2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,
  2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,
  2,2,2,2,2,2,2,2,2,2,2,2,2,2,2
};

static const unsigned short ag_ptt[] = {
    0, 24, 25, 25, 22, 30, 30, 31, 31, 27, 27, 27, 37, 37,  2, 39, 39, 40,
   40, 23, 23, 23, 23, 23, 41, 41, 41, 41, 41, 41, 41, 41, 41, 41, 41, 41,
   41, 41, 41, 41, 41, 41, 57, 57, 41, 41, 41, 41, 41, 41, 41, 41, 41, 41,
   41, 41, 41, 41, 41, 41, 41, 41, 41, 41, 41, 41, 41, 41, 41, 41, 41, 41,
   64, 64, 64, 64, 64, 64, 67, 67,298,298, 84, 84, 84,293,293,323, 19, 19,
  290, 13, 13, 13, 13,292, 15, 15, 15, 15,289, 74, 74,102,102,102, 98,105,
  105, 98,108,108, 98,111,111, 98,114,114, 98,117,117, 98, 55,118,118,118,
  119,119,119,122,122,122,125,125,125,125,125,128,128,128,133,133,133,133,
  133,133,136,136,136,136,138,138,138,138,147,147,147,147,147,147,147,146,
  146,325,325,325,325,325,325,325,325,  8,  8,  8,  8,  8,  8,  7,  7, 18,
   18, 16, 16, 17, 17, 17, 17, 17, 17, 17, 17,373, 10, 10, 10,380,380,380,
  380,380,380,380,380,401,401,401,401,181,181,181,181,181,183,183, 82, 82,
   82,184,184,184,184,184,184,184,184,184,184,184,184,184,184,184,184,184,
  184,184,184,184,184,184,184,184,184,184,184,184,184,184,184,184,184,184,
  184,184,184,184,184,184,184,184,184,184,184,184,184,184,184,184,184,184,
  184,184,184,184,184,184,184,184,184,184,184,184,184,184,184,184,184,184,
  184,184,184,184,184,184,184,184,184,184,184,184,184,184,184,184,184,184,
  184,184, 32, 87, 29, 29, 29, 29, 29, 29, 29, 29, 29, 29, 29, 29, 29, 29,
   29, 29, 29, 29, 29, 29, 29, 29, 29, 29, 29, 29, 29, 29, 29, 29, 29, 29,
   29, 29, 36, 36, 36, 36, 36, 36, 36, 36, 36, 36, 36, 36, 36, 36, 36, 36,
   36, 36, 36, 36, 36, 36, 36, 36, 36, 36, 36, 36, 36, 36, 36, 36, 36, 36,
   36, 86, 86, 86, 86, 86, 86, 86, 86, 88, 88, 88, 88, 88, 88, 88, 88, 88,
   88, 88, 88, 88, 91, 91, 91, 91, 91, 91, 91, 91, 91, 91, 91, 91, 91, 91,
   91, 91, 91, 91, 91, 91, 91, 91, 91, 91, 91, 91, 91, 91, 91, 91, 91, 91,
   93, 93, 93, 93, 93, 93, 93, 93, 93, 93, 93, 93, 93, 93, 93, 93, 93, 93,
   93, 93, 93, 93, 93, 93, 93, 93, 93, 93, 93, 93, 93, 93, 93, 93, 96, 96,
   96, 96, 96, 96, 96, 96, 96, 96, 96, 96, 96, 96, 96, 96, 96, 96, 96, 96,
   96, 96, 96, 96, 96, 96, 96, 96, 96, 96, 96,161,161,161,166,166,166,166,
  169,169,169,169,169,169,169,169,169,169,169,169,169,169,169,169,169,169,
  169,169,169,169,169,169,169,169,169,169,169,169,169,169,260,260,282,282,
  283,283,284,284, 28, 33, 34, 42, 12, 11, 43, 14, 20, 44, 46, 45, 47,  4,
   48, 49, 50, 51, 52, 53, 54, 56, 58, 59, 60, 61, 62, 63, 65, 66, 68, 69,
   70, 71, 72, 73, 75, 76, 21, 77,  3, 78, 79, 80, 81, 83, 99,100,101,103,
  104,106,107,109,110,112,113,115,116,120,121,123,124,126,127,129,130,131,
  132,134,135,137,139,140,141,142,143,144,145,149,148,150,151,152,153,154,
  155,156,  9,158,160,157,182,179,185,  5,186,187,188,189,190,191,192,193,
  194,195,196,197,198,199,200,201,202,203,204,205,  6,206,207,208,209,210,
  211,212,213,214,215,216,217,218,219,220,221,222,223,224,225,226,227,228,
  229,230,231,232,233,234,235,236,237,238,239,240,241,242,243,244,245,246,
  247,248,249,250,251,252,253,254,255,256,257,258,259,261,262,263,264,265,
  266,267,268,269,270,271,272,273,274,275,276,277, 90,163, 89,174,164,278,
  279, 95, 94,167,280,170,172,171,165, 92,162,281
};


static void ag_ra(void)
{
  switch(ag_rpx[(PCB).ag_ap]) {
    case 1: ag_rp_1(); break;
    case 2: ag_rp_6(); break;
    case 3: ag_rp_7(); break;
    case 4: ag_rp_8(); break;
    case 5: ag_rp_9(); break;
    case 6: ag_rp_10(); break;
    case 7: ag_rp_11(); break;
    case 8: ag_rp_12(); break;
    case 9: ag_rp_13(); break;
    case 10: ag_rp_14(); break;
    case 11: ag_rp_15(); break;
    case 12: ag_rp_16(); break;
    case 13: ag_rp_17(); break;
    case 14: ag_rp_18(); break;
    case 15: ag_rp_19(); break;
    case 16: ag_rp_20(); break;
    case 17: ag_rp_21(); break;
    case 18: ag_rp_22(); break;
    case 19: ag_rp_23(); break;
    case 20: ag_rp_24(); break;
    case 21: ag_rp_25(); break;
    case 22: ag_rp_26(); break;
    case 23: ag_rp_27(); break;
    case 24: ag_rp_28(); break;
    case 25: ag_rp_29(); break;
    case 26: ag_rp_30(); break;
    case 27: ag_rp_31(); break;
    case 28: ag_rp_32(); break;
    case 29: ag_rp_33(); break;
    case 30: ag_rp_34(); break;
    case 31: ag_rp_35(); break;
    case 32: ag_rp_36(); break;
    case 33: ag_rp_37(); break;
    case 34: ag_rp_38(); break;
    case 35: ag_rp_39(); break;
    case 36: ag_rp_40(V(3,int)); break;
    case 37: ag_rp_41(); break;
    case 38: ag_rp_42(); break;
    case 39: ag_rp_43(); break;
    case 40: ag_rp_44(); break;
    case 41: ag_rp_45(); break;
    case 42: ag_rp_46(); break;
    case 43: ag_rp_47(); break;
    case 44: ag_rp_48(); break;
    case 45: ag_rp_49(); break;
    case 46: ag_rp_50(); break;
    case 47: ag_rp_51(); break;
    case 48: ag_rp_52(); break;
    case 49: ag_rp_53(V(0,int)); break;
    case 50: ag_rp_54(V(1,int)); break;
    case 51: ag_rp_55(V(1,int)); break;
    case 52: ag_rp_56(V(0,int)); break;
    case 53: ag_rp_57(V(1,int)); break;
    case 54: ag_rp_58(V(1,int), V(2,int)); break;
    case 55: ag_rp_59(V(1,int)); break;
    case 56: ag_rp_60(); break;
    case 57: ag_rp_61(V(1,int)); break;
    case 58: ag_rp_62(V(2,int)); break;
    case 59: ag_rp_63(); break;
    case 60: ag_rp_64(); break;
    case 61: ag_rp_65(V(1,int)); break;
    case 62: ag_rp_66(V(2,int)); break;
    case 63: ag_rp_67(); break;
    case 64: ag_rp_68(); break;
    case 65: ag_rp_69(); break;
    case 66: ag_rp_70(); break;
    case 67: ag_rp_71(); break;
    case 68: ag_rp_72(); break;
    case 69: ag_rp_73(); break;
    case 70: ag_rp_74(); break;
    case 71: ag_rp_75(); break;
    case 72: ag_rp_76(); break;
    case 73: ag_rp_77(); break;
    case 74: ag_rp_78(); break;
    case 75: ag_rp_79(); break;
    case 76: ag_rp_80(); break;
    case 77: ag_rp_81(); break;
    case 78: ag_rp_82(); break;
    case 79: ag_rp_83(); break;
    case 80: ag_rp_84(); break;
    case 81: ag_rp_85(); break;
    case 82: ag_rp_86(); break;
    case 83: ag_rp_87(); break;
    case 84: ag_rp_88(); break;
    case 85: ag_rp_89(); break;
    case 86: ag_rp_90(); break;
    case 87: ag_rp_91(); break;
    case 88: ag_rp_92(); break;
    case 89: ag_rp_93(); break;
    case 90: ag_rp_94(); break;
    case 91: ag_rp_95(); break;
    case 92: ag_rp_96(V(0,double)); break;
    case 93: ag_rp_97(); break;
    case 94: ag_rp_98(); break;
    case 95: ag_rp_99(); break;
    case 96: ag_rp_100(); break;
    case 97: ag_rp_101(); break;
    case 98: ag_rp_102(); break;
    case 99: ag_rp_103(); break;
    case 100: ag_rp_104(); break;
    case 101: V(0,double) = ag_rp_105(V(0,int)); break;
    case 102: V(0,double) = ag_rp_106(V(0,double)); break;
    case 103: V(0,int) = ag_rp_107(V(0,int)); break;
    case 104: V(0,int) = ag_rp_108(); break;
    case 105: V(0,int) = ag_rp_109(); break;
    case 106: V(0,int) = ag_rp_110(); break;
    case 107: V(0,int) = ag_rp_111(); break;
    case 108: V(0,int) = ag_rp_112(); break;
    case 109: V(0,int) = ag_rp_113(); break;
    case 110: V(0,int) = ag_rp_114(); break;
    case 111: ag_rp_115(V(1,int)); break;
    case 112: ag_rp_116(V(1,int)); break;
    case 113: ag_rp_117(V(0,int)); break;
    case 114: ag_rp_118(V(1,int)); break;
    case 115: ag_rp_119(V(2,int)); break;
    case 116: ag_rp_120(V(1,int)); break;
    case 117: ag_rp_121(V(1,int)); break;
    case 118: ag_rp_122(V(1,int)); break;
    case 119: V(0,int) = ag_rp_123(V(1,int)); break;
    case 120: V(0,int) = ag_rp_124(); break;
    case 121: V(0,int) = ag_rp_125(V(0,int)); break;
    case 122: V(0,int) = ag_rp_126(); break;
    case 123: V(0,int) = ag_rp_127(); break;
    case 124: V(0,int) = ag_rp_128(); break;
    case 125: V(0,int) = ag_rp_129(); break;
    case 126: V(0,int) = ag_rp_130(); break;
    case 127: V(0,int) = ag_rp_131(); break;
    case 128: V(0,int) = ag_rp_132(); break;
    case 129: V(0,double) = ag_rp_133(); break;
    case 130: V(0,double) = ag_rp_134(V(1,int)); break;
    case 131: V(0,double) = ag_rp_135(V(0,double), V(1,int)); break;
    case 132: ag_rp_136(); break;
    case 133: ag_rp_137(); break;
    case 134: ag_rp_138(); break;
    case 135: ag_rp_139(); break;
    case 136: ag_rp_140(); break;
    case 137: ag_rp_141(); break;
    case 138: ag_rp_142(); break;
    case 139: ag_rp_143(); break;
    case 140: ag_rp_144(); break;
    case 141: ag_rp_145(); break;
    case 142: ag_rp_146(); break;
    case 143: ag_rp_147(); break;
    case 144: ag_rp_148(); break;
    case 145: ag_rp_149(); break;
    case 146: ag_rp_150(); break;
    case 147: ag_rp_151(); break;
    case 148: ag_rp_152(); break;
    case 149: ag_rp_153(); break;
    case 150: ag_rp_154(); break;
    case 151: ag_rp_155(); break;
    case 152: ag_rp_156(); break;
    case 153: ag_rp_157(); break;
    case 154: ag_rp_158(); break;
    case 155: ag_rp_159(); break;
    case 156: ag_rp_160(); break;
    case 157: ag_rp_161(); break;
    case 158: ag_rp_162(); break;
    case 159: ag_rp_163(); break;
    case 160: ag_rp_164(); break;
    case 161: ag_rp_165(); break;
    case 162: ag_rp_166(); break;
    case 163: ag_rp_167(); break;
    case 164: ag_rp_168(); break;
    case 165: ag_rp_169(); break;
    case 166: ag_rp_170(); break;
    case 167: ag_rp_171(); break;
    case 168: ag_rp_172(); break;
    case 169: ag_rp_173(); break;
    case 170: ag_rp_174(); break;
    case 171: ag_rp_175(); break;
    case 172: ag_rp_176(); break;
    case 173: ag_rp_177(); break;
    case 174: ag_rp_178(); break;
    case 175: ag_rp_179(); break;
    case 176: ag_rp_180(); break;
    case 177: ag_rp_181(); break;
    case 178: ag_rp_182(); break;
    case 179: ag_rp_183(); break;
    case 180: ag_rp_184(); break;
    case 181: ag_rp_185(); break;
    case 182: ag_rp_186(); break;
    case 183: ag_rp_187(); break;
    case 184: ag_rp_188(); break;
    case 185: ag_rp_189(); break;
    case 186: ag_rp_190(); break;
    case 187: ag_rp_191(); break;
    case 188: ag_rp_192(); break;
    case 189: ag_rp_193(); break;
    case 190: ag_rp_194(); break;
    case 191: ag_rp_195(); break;
    case 192: ag_rp_196(); break;
    case 193: ag_rp_197(); break;
    case 194: ag_rp_198(); break;
    case 195: ag_rp_199(); break;
    case 196: ag_rp_200(); break;
    case 197: ag_rp_201(); break;
    case 198: ag_rp_202(); break;
    case 199: ag_rp_203(); break;
    case 200: ag_rp_204(); break;
    case 201: ag_rp_205(); break;
    case 202: ag_rp_206(); break;
    case 203: ag_rp_207(); break;
    case 204: ag_rp_208(); break;
    case 205: ag_rp_209(); break;
    case 206: ag_rp_210(); break;
    case 207: ag_rp_211(); break;
    case 208: ag_rp_212(); break;
    case 209: ag_rp_213(); break;
    case 210: ag_rp_214(); break;
    case 211: ag_rp_215(); break;
    case 212: ag_rp_216(); break;
    case 213: ag_rp_217(); break;
    case 214: ag_rp_218(); break;
    case 215: ag_rp_219(); break;
    case 216: ag_rp_220(); break;
    case 217: ag_rp_221(); break;
    case 218: ag_rp_222(); break;
    case 219: ag_rp_223(); break;
    case 220: ag_rp_224(); break;
    case 221: ag_rp_225(); break;
    case 222: ag_rp_226(); break;
    case 223: ag_rp_227(); break;
    case 224: ag_rp_228(); break;
    case 225: ag_rp_229(V(2,int)); break;
    case 226: ag_rp_230(); break;
    case 227: ag_rp_231(); break;
    case 228: ag_rp_232(); break;
    case 229: ag_rp_233(); break;
    case 230: ag_rp_234(); break;
    case 231: ag_rp_235(); break;
    case 232: ag_rp_236(); break;
    case 233: ag_rp_237(); break;
    case 234: ag_rp_238(); break;
    case 235: ag_rp_239(); break;
    case 236: ag_rp_240(); break;
    case 237: ag_rp_241(); break;
    case 238: ag_rp_242(); break;
    case 239: ag_rp_243(); break;
    case 240: ag_rp_244(); break;
    case 241: ag_rp_245(); break;
  }
}

#define TOKEN_NAMES a85parse_token_names
const char *const a85parse_token_names[491] = {
  "a85parse",
  "WS",
  "cstyle comment",
  "integer",
  "literal name",
  "register 8 bit",
  "register 16 bit",
  "hex integer",
  "decimal integer",
  "real",
  "simple real",
  "literal string",
  "label",
  "string chars",
  "include string",
  "include chars",
  "ascii integer",
  "escape char",
  "asm hex value",
  "singlequote chars",
  "asm include",
  "singlequote string",
  "a85parse",
  "statement",
  "",
  "",
  "eof",
  "comment",
  "\"//\"",
  "any text char",
  "",
  "",
  "newline",
  "';'",
  "','",
  "\"/*\"",
  "",
  "",
  "\"*/\"",
  "",
  "",
  "expression",
  "'\\n'",
  "\"INCLUDE\"",
  "\"#INCLUDE\"",
  "\"#PRAGMA\"",
  "\"LIST\"",
  "\"HEX\"",
  "\"#IFDEF\"",
  "\"#IFNDEF\"",
  "\"#ELSE\"",
  "\"#ENDIF\"",
  "\"#DEFINE\"",
  "\"#UNDEF\"",
  "\"EQU\"",
  "equation",
  "\"SET\"",
  "",
  "\"ORG\"",
  "\"ASEG\"",
  "\"CSEG\"",
  "\"DSEG\"",
  "\"DS\"",
  "\"DB\"",
  "expression list",
  "\"DW\"",
  "\"EXTRN\"",
  "name list",
  "\"EXTERN\"",
  "\"PUBLIC\"",
  "\"NAME\"",
  "\"STKLN\"",
  "\"END\"",
  "\"IF\"",
  "condition",
  "\"ELSE\"",
  "\"ENDIF\"",
  "\"TITLE\"",
  "\"PAGE\"",
  "\"SYM\"",
  "\"LINK\"",
  "\"MACLIB\"",
  "instruction list",
  "\",\"",
  "literal alpha",
  "\"$\"",
  "",
  "digit",
  "asm incl char",
  "'\\''",
  "'\\\"'",
  "string char",
  "'\\\\'",
  "",
  "'>'",
  "'<'",
  "",
  "\":\"",
  "condition start",
  "\"EQ\"",
  "\"==\"",
  "\"=\"",
  "",
  "\"NE\"",
  "\"!=\"",
  "",
  "\"GE\"",
  "\">=\"",
  "",
  "\"LE\"",
  "\"<=\"",
  "",
  "\"GT\"",
  "\">\"",
  "",
  "\"LT\"",
  "\"<\"",
  "",
  "inclusive or exp",
  "exclusive or exp",
  "'|'",
  "\"OR\"",
  "and exp",
  "'^'",
  "\"XOR\"",
  "shift exp",
  "'&'",
  "\"AND\"",
  "additive exp",
  "\"<<\"",
  "\"SHL\"",
  "\">>\"",
  "\"SHR\"",
  "multiplicative exp",
  "'+'",
  "'-'",
  "urinary exp",
  "'*'",
  "primary exp",
  "'/'",
  "'%'",
  "\"MOD\"",
  "\"**\"",
  "'!'",
  "\"NOT\"",
  "'~'",
  "value",
  "function",
  "'('",
  "')'",
  "\"FLOOR\"",
  "\"CEIL\"",
  "\"LN\"",
  "\"LOG\"",
  "\"SQRT\"",
  "\"IP\"",
  "\"FP\"",
  "\"H\"",
  "\"B\"",
  "\"Q\"",
  "\"D\"",
  "",
  "'_'",
  "'$'",
  "'0'",
  "",
  "hex digit",
  "",
  "\"\\'\\\\\\'\"",
  "",
  "'n'",
  "'t'",
  "'r'",
  "\"\\'\\'\"",
  "'.'",
  "\"C\"",
  "\"E\"",
  "\"L\"",
  "\"M\"",
  "\"A\"",
  "\"SP\"",
  "stack register",
  "\"PSW\"",
  "bd register",
  "instruction",
  "\"ACI\"",
  "\"ADC\"",
  "\"ADD\"",
  "\"ADI\"",
  "\"ANA\"",
  "\"ANI\"",
  "\"ASHR\"",
  "\"CALL\"",
  "\"CC\"",
  "\"CM\"",
  "\"CMA\"",
  "\"CMC\"",
  "\"CMP\"",
  "\"CNC\"",
  "\"CNZ\"",
  "\"CP\"",
  "\"CPE\"",
  "\"CPI\"",
  "\"CPO\"",
  "\"CZ\"",
  "\"DAA\"",
  "\"DAD\"",
  "\"DCR\"",
  "\"DCX\"",
  "\"DI\"",
  "\"DSUB\"",
  "\"EI\"",
  "\"HLT\"",
  "\"IN\"",
  "\"INR\"",
  "\"INX\"",
  "\"JC\"",
  "\"JD\"",
  "\"JX\"",
  "\"JM\"",
  "\"JMP\"",
  "\"JNC\"",
  "\"JND\"",
  "\"JNX\"",
  "\"JNZ\"",
  "\"JP\"",
  "\"JPE\"",
  "\"JPO\"",
  "\"JZ\"",
  "\"LDA\"",
  "\"LDAX\"",
  "\"LDEH\"",
  "\"LDES\"",
  "\"LHLD\"",
  "\"LHLX\"",
  "\"LXI\"",
  "\"MOV\"",
  "\"MVI\"",
  "\"NOP\"",
  "\"ORA\"",
  "\"ORI\"",
  "\"OUT\"",
  "\"PCHL\"",
  "\"POP\"",
  "\"PUSH\"",
  "\"RAL\"",
  "\"RAR\"",
  "\"RC\"",
  "\"RET\"",
  "\"RIM\"",
  "\"RLC\"",
  "\"RLDE\"",
  "\"RM\"",
  "\"RNC\"",
  "\"RNZ\"",
  "\"RP\"",
  "\"RPE\"",
  "\"RPO\"",
  "\"RRC\"",
  "\"RST\"",
  "rst arg",
  "\"RZ\"",
  "\"SBB\"",
  "\"SBI\"",
  "\"SHLD\"",
  "\"SHLX\"",
  "\"SIM\"",
  "\"SPHL\"",
  "\"STA\"",
  "\"STAX\"",
  "\"STC\"",
  "\"SUB\"",
  "\"SUI\"",
  "\"XCHG\"",
  "\"XRA\"",
  "\"XRI\"",
  "\"XTHL\"",
  "",
  "",
  "",
  "",
  "",
  "",
  "",
  "",
  "\"//\"",
  "';'",
  "','",
  "'\\n'",
  "label",
  "literal string",
  "\"INCLUDE\"",
  "include string",
  "asm include",
  "\"#INCLUDE\"",
  "\"LIST\"",
  "\"#PRAGMA\"",
  "\"HEX\"",
  "literal name",
  "\"#IFDEF\"",
  "\"#IFNDEF\"",
  "\"#ELSE\"",
  "\"#ENDIF\"",
  "\"#DEFINE\"",
  "\"#UNDEF\"",
  "\"EQU\"",
  "\"SET\"",
  "\"ORG\"",
  "\"ASEG\"",
  "\"CSEG\"",
  "\"DSEG\"",
  "\"DS\"",
  "\"DB\"",
  "\"DW\"",
  "\"EXTRN\"",
  "\"EXTERN\"",
  "\"PUBLIC\"",
  "\"NAME\"",
  "\"STKLN\"",
  "\"END\"",
  "\"IF\"",
  "\"ELSE\"",
  "\"ENDIF\"",
  "singlequote string",
  "\"TITLE\"",
  "integer",
  "\"PAGE\"",
  "\"SYM\"",
  "\"LINK\"",
  "\"MACLIB\"",
  "\",\"",
  "\"EQ\"",
  "\"==\"",
  "\"=\"",
  "\"NE\"",
  "\"!=\"",
  "\"GE\"",
  "\">=\"",
  "\"LE\"",
  "\"<=\"",
  "\"GT\"",
  "\">\"",
  "\"LT\"",
  "\"<\"",
  "'|'",
  "\"OR\"",
  "'^'",
  "\"XOR\"",
  "'&'",
  "\"AND\"",
  "\"<<\"",
  "\"SHL\"",
  "\">>\"",
  "\"SHR\"",
  "'+'",
  "'-'",
  "'*'",
  "'/'",
  "'%'",
  "\"MOD\"",
  "\"**\"",
  "'!'",
  "\"NOT\"",
  "'~'",
  "')'",
  "'('",
  "\"FLOOR\"",
  "\"CEIL\"",
  "\"LN\"",
  "\"LOG\"",
  "\"SQRT\"",
  "\"IP\"",
  "\"FP\"",
  "real",
  "\"B\"",
  "\"D\"",
  "\"H\"",
  "\"PSW\"",
  "\"A\"",
  "\"ACI\"",
  "register 8 bit",
  "\"ADC\"",
  "\"ADD\"",
  "\"ADI\"",
  "\"ANA\"",
  "\"ANI\"",
  "\"ASHR\"",
  "\"CALL\"",
  "\"CC\"",
  "\"CM\"",
  "\"CMA\"",
  "\"CMC\"",
  "\"CMP\"",
  "\"CNC\"",
  "\"CNZ\"",
  "\"CP\"",
  "\"CPE\"",
  "\"CPI\"",
  "\"CPO\"",
  "\"CZ\"",
  "\"DAA\"",
  "register 16 bit",
  "\"DAD\"",
  "\"DCR\"",
  "\"DCX\"",
  "\"DI\"",
  "\"DSUB\"",
  "\"EI\"",
  "\"HLT\"",
  "\"IN\"",
  "\"INR\"",
  "\"INX\"",
  "\"JC\"",
  "\"JD\"",
  "\"JX\"",
  "\"JM\"",
  "\"JMP\"",
  "\"JNC\"",
  "\"JND\"",
  "\"JNX\"",
  "\"JNZ\"",
  "\"JP\"",
  "\"JPE\"",
  "\"JPO\"",
  "\"JZ\"",
  "\"LDA\"",
  "\"LDAX\"",
  "\"LDEH\"",
  "\"LDES\"",
  "\"LHLD\"",
  "\"LHLX\"",
  "\"LXI\"",
  "\"MOV\"",
  "\"MVI\"",
  "\"NOP\"",
  "\"ORA\"",
  "\"ORI\"",
  "\"OUT\"",
  "\"PCHL\"",
  "\"POP\"",
  "\"PUSH\"",
  "\"RAL\"",
  "\"RAR\"",
  "\"RC\"",
  "\"RET\"",
  "\"RIM\"",
  "\"RLC\"",
  "\"RLDE\"",
  "\"RM\"",
  "\"RNC\"",
  "\"RNZ\"",
  "\"RP\"",
  "\"RPE\"",
  "\"RPO\"",
  "\"RRC\"",
  "\"RST\"",
  "\"RZ\"",
  "\"SBB\"",
  "\"SBI\"",
  "\"SHLD\"",
  "\"SHLX\"",
  "\"SIM\"",
  "\"SPHL\"",
  "\"STA\"",
  "\"STAX\"",
  "\"STC\"",
  "\"SUB\"",
  "\"SUI\"",
  "\"XCHG\"",
  "\"XRA\"",
  "\"XRI\"",
  "\"XTHL\"",
  "",
  "'\\\"'",
  "'$'",
  "'\\''",
  "'.'",
  "'0'",
  "",
  "",
  "'<'",
  "'>'",
  "",
  "",
  "'n'",
  "'r'",
  "'t'",
  "",
  "'\\\\'",
  "'_'",
  "",

};


static const unsigned short ag_ctn[] = {
    0,0,  2,1,  0,0,  0,0,  0,0,  0,0, 23,1,  0,0,  0,0,  0,0, 27,1, 27,1,
   27,1, 27,1, 41,1, 41,1, 41,1, 41,1, 41,1, 41,1, 41,1, 41,1, 41,1, 41,1,
   41,1, 41,1, 41,1, 41,1, 41,1, 41,1, 41,1, 41,1, 41,1, 41,1, 41,1, 23,2,
   29,1,  0,0,  0,0, 29,1,  0,0, 29,1, 29,1, 29,1, 29,1, 29,1, 29,1, 29,1,
   29,1, 29,1, 29,1,  0,0,  0,0,  0,0, 29,1,  0,0,  0,0,  0,0,  0,0,  0,0,
   29,1,  0,0,  0,0, 29,1, 29,1,  0,0, 29,1,  0,0, 27,2, 27,2, 27,2,184,1,
  184,1,184,1,184,1,184,1,184,1,184,1,184,1,184,1,184,1,184,1,184,1,184,1,
  184,1,184,1,184,1,184,1,184,1,184,1,184,1,184,1,184,1,184,1,184,1,184,1,
  184,1,184,1,184,1,184,1,184,1,184,1,184,1,184,1,184,1,184,1,184,1,184,1,
  184,1,184,1,184,1,184,1,184,1,184,1,184,1,184,1,184,1,184,1,184,1,184,1,
  184,1,184,1,184,1,184,1,184,1,184,1,184,1,184,1,184,1,184,1,184,1,184,1,
  184,1,184,1,184,1,184,1,184,1,184,1,184,1,184,1,184,1,184,1,184,1,184,1,
  184,1,184,1,184,1,184,1,184,1,184,1,184,1,184,1,184,1,184,1,184,1,184,1,
  184,1,184,1,184,1,184,1,184,1,184,1,184,1,184,1,184,1,184,1,184,1,184,1,
  184,1,184,1,184,1,184,1,184,1,184,1,184,1,184,1,184,1,184,1,184,1,184,1,
  184,1,184,1,184,1,184,1,184,1,184,1,184,1,184,1,184,1,184,1,184,1,184,1,
  184,1,184,1,184,1,184,1,184,1,184,1,184,1,184,1,184,1,184,1,184,1,184,1,
  184,1,184,1,184,1,184,1,184,1,184,1,184,1,184,1,184,1,184,1,184,1,184,1,
  184,1,184,1,184,1,184,1,184,1, 82,1,  0,0, 41,2,  0,0, 41,2,  0,0,  0,0,
   41,2,  0,0, 41,2,  0,0,  0,0,  0,0, 41,2,  0,0, 41,2,  0,0, 41,2,  0,0,
   41,2,  0,0, 41,2,  0,0, 41,2,  0,0, 41,2,  0,0, 41,2,  0,0, 41,2,  0,0,
   41,2,  0,0,  0,0,  0,0,  0,0, 41,2, 41,2, 41,2, 41,2,  0,0, 41,2,  0,0,
   41,2,  0,0,  0,0,  0,0,292,1,  0,0,290,1,  0,0,  0,0, 41,2, 41,2,  0,0,
  184,2,184,2,184,2,184,2,184,2,184,2,184,2,184,2,184,2,184,2,184,2,184,2,
  184,2,184,2,184,2,184,2,184,2,184,2,184,2,184,2,184,2,184,2,184,2,184,2,
  184,2,184,2,184,2,184,2,184,2,184,2,184,2,184,2,184,2,184,2,184,2,184,2,
  184,2,184,2,184,2,184,2,184,2,184,2,184,2,184,2,184,2,184,2,184,2,184,2,
  184,2,184,2,184,2,184,2,184,2,184,2,184,2,184,2,184,2,184,2,184,2, 82,2,
    0,0, 19,1,323,1,  0,0, 10,1,  7,1,  8,1,  8,1, 16,1, 10,1,325,1,  7,1,
  146,1,147,1,147,1,147,1,147,1,147,1,147,1,147,1,146,1,  0,0,147,1,147,1,
  147,1,147,1,147,1,147,1,147,1,136,1,138,1,136,1,136,1,136,1,133,1,128,1,
  125,1,122,1,119,1,118,1, 98,1, 74,1, 41,3, 67,1, 67,1, 67,1,  0,0, 64,1,
   64,1, 64,1, 64,1, 15,2, 13,2,  0,0,  0,0,  0,0,181,1,181,1,181,1,184,3,
  184,3,  0,0,184,3,325,1, 17,1, 19,2,  7,2, 16,2,147,2,147,2,147,2,147,2,
  147,2,147,2,147,2,138,2,  0,0,133,2,  0,0,133,2,133,2,133,2,133,2,128,2,
  128,2,  0,0,125,2,  0,0,125,2,  0,0,125,2,  0,0,125,2,  0,0,122,2,122,2,
    0,0,119,2,119,2,  0,0,118,2,118,2,  0,0,  0,0,  0,0,  0,0,  0,0,  0,0,
    0,0,  0,0,  0,0,  0,0,  0,0,  0,0,  0,0, 67,2,  0,0, 64,2, 64,2, 64,2,
   64,2,184,4,184,4,184,4,147,3,147,3,147,3,147,3,147,3,147,3,147,3,138,3,
  133,3,133,3,133,3,133,3,133,3,128,3,128,3,125,3,125,3,125,3,125,3,122,3,
  122,3,119,3,119,3,118,3,118,3,  0,0, 67,3, 64,3,184,5,184,5,184,5,133,1,
  133,1,128,1,128,1,128,1,128,1,125,1,125,1,122,1,122,1,119,1,119,1, 67,4,
   64,4,184,6,184,6,184,6
};

#ifndef MISSING_FORMAT
#define MISSING_FORMAT "Missing %s"
#endif
#ifndef UNEXPECTED_FORMAT
#define UNEXPECTED_FORMAT "Unexpected %s"
#endif
#ifndef UNNAMED_TOKEN
#define UNNAMED_TOKEN "input"
#endif


static void ag_diagnose(void) {
  int ag_snd = (PCB).sn;
  int ag_k = ag_sbt[ag_snd];

  if (*TOKEN_NAMES[ag_tstt[ag_k]] && ag_astt[ag_k + 1] == ag_action_8) {
    sprintf((PCB).ag_msg, MISSING_FORMAT, TOKEN_NAMES[ag_tstt[ag_k]]);
  }
  else if (ag_astt[ag_sbe[(PCB).sn]] == ag_action_8
          && (ag_k = (int) ag_sbe[(PCB).sn] + 1) == (int) ag_sbt[(PCB).sn+1] - 1
          && *TOKEN_NAMES[ag_tstt[ag_k]]) {
    sprintf((PCB).ag_msg, MISSING_FORMAT, TOKEN_NAMES[ag_tstt[ag_k]]);
  }
  else if ((PCB).token_number && *TOKEN_NAMES[(PCB).token_number]) {
    sprintf((PCB).ag_msg, UNEXPECTED_FORMAT, TOKEN_NAMES[(PCB).token_number]);
  }
  else if (isprint((*(PCB).lab)) && (*(PCB).lab) != '\\') {
    char buf[20];
    sprintf(buf, "\'%c\'", (char) (*(PCB).lab));
    sprintf((PCB).ag_msg, UNEXPECTED_FORMAT, buf);
  }
  else sprintf((PCB).ag_msg, UNEXPECTED_FORMAT, UNNAMED_TOKEN);
  (PCB).error_message = (PCB).ag_msg;


}
static int ag_action_1_r_proc(void);
static int ag_action_2_r_proc(void);
static int ag_action_3_r_proc(void);
static int ag_action_4_r_proc(void);
static int ag_action_1_s_proc(void);
static int ag_action_3_s_proc(void);
static int ag_action_1_proc(void);
static int ag_action_2_proc(void);
static int ag_action_3_proc(void);
static int ag_action_4_proc(void);
static int ag_action_5_proc(void);
static int ag_action_6_proc(void);
static int ag_action_7_proc(void);
static int ag_action_8_proc(void);
static int ag_action_9_proc(void);
static int ag_action_10_proc(void);
static int ag_action_11_proc(void);
static int ag_action_8_proc(void);


static int (*const  ag_r_procs_scan[])(void) = {
  ag_action_1_r_proc,
  ag_action_2_r_proc,
  ag_action_3_r_proc,
  ag_action_4_r_proc
};

static int (*const  ag_s_procs_scan[])(void) = {
  ag_action_1_s_proc,
  ag_action_2_r_proc,
  ag_action_3_s_proc,
  ag_action_4_r_proc
};

static int (*const  ag_gt_procs_scan[])(void) = {
  ag_action_1_proc,
  ag_action_2_proc,
  ag_action_3_proc,
  ag_action_4_proc,
  ag_action_5_proc,
  ag_action_6_proc,
  ag_action_7_proc,
  ag_action_8_proc,
  ag_action_9_proc,
  ag_action_10_proc,
  ag_action_11_proc,
  ag_action_8_proc
};


static int ag_rns(int ag_t, int *ag_sx, int ag_snd) {
  while (1) {
    int ag_act, ag_k = ag_sbt[ag_snd], ag_lim = ag_sbt[ag_snd+1];
    int ag_p;

    while (ag_k < ag_lim && ag_tstt[ag_k] != ag_t) ag_k++;
    if (ag_k == ag_lim) break;
    ag_act = ag_astt[ag_k];
    ag_p = ag_pstt[ag_k];
    if (ag_act == ag_action_2) return ag_p;
    if (ag_act == ag_action_10 || ag_act == ag_action_11) {
      (*ag_sx)--;
      return ag_snd;
    }
    if (ag_act != ag_action_3 &&
      ag_act != ag_action_4) break;
    *ag_sx -= (ag_fl[ag_p] - 1);
    ag_snd = (PCB).ss[*ag_sx];
    ag_t = ag_ptt[ag_p];
  }
  return 0;
}

static int ag_jns(int ag_t) {
  int ag_k;

  ag_k = ag_sbt[(PCB).sn];
  while (ag_tstt[ag_k] != ag_t && ag_tstt[ag_k]) ag_k++;
  while (1) {
    int ag_p = ag_pstt[ag_k];
    int ag_sd;

    switch (ag_astt[ag_k]) {
    case ag_action_2:
      (PCB).ss[(PCB).ssx] = (PCB).sn;
      return ag_p;
    case ag_action_10:
    case ag_action_11:
      return (PCB).ss[(PCB).ssx--];
    case ag_action_9:
      (PCB).ss[(PCB).ssx] = (PCB).sn;
      (PCB).ssx++;
      (PCB).sn = ag_p;
      ag_k = ag_sbt[(PCB).sn];
      while (ag_tstt[ag_k] != ag_t && ag_tstt[ag_k]) ag_k++;
      continue;
    case ag_action_3:
    case ag_action_4:
      ag_sd = ag_fl[ag_p] - 1;
      if (ag_sd) (PCB).sn = (PCB).ss[(PCB).ssx -= ag_sd];
      else (PCB).ss[(PCB).ssx] = (PCB).sn;
      ag_t = ag_ptt[ag_p];
      ag_k = ag_sbt[(PCB).sn+1];
      while (ag_tstt[--ag_k] != ag_t);
      continue;
    case ag_action_5:
    case ag_action_6:
      if (ag_fl[ag_p]) break;
      (PCB).sn = ag_rns(ag_ptt[ag_p],&(PCB).ssx, (PCB).sn);
      (PCB).ss[++(PCB).ssx] = (PCB).sn;
      ag_k = ag_sbt[(PCB).sn];
      while (ag_tstt[ag_k] != ag_t && ag_tstt[ag_k]) ag_k++;
      continue;
    }
    break;
  }
  return 0;
}


static int ag_atx(int ag_t, int *ag_sx, int ag_snd) {
  int ag_k, ag_f;
  int ag_save_btsx = (PCB).btsx;
  int ag_flag = 1;

  while (1) {
    int ag_a;

    (PCB).bts[128 - ++(PCB).btsx] = *ag_sx;
    (PCB).ss[128 - (PCB).btsx] = (PCB).ss[*ag_sx];
    (PCB).ss[*ag_sx] = ag_snd;
    ag_k = ag_sbt[ag_snd];
    while (ag_tstt[ag_k] != ag_t && ag_tstt[ag_k]) ag_k++;
    ag_a = ag_astt[ag_k];
    if (ag_a == ag_action_2 ||
        ag_a == ag_action_3 ||
        ag_a == ag_action_10 ||
        ag_a == ag_action_11 ||
        ag_a == ag_action_1 ||
        ag_a == ag_action_4) break;
    if ((ag_a == ag_action_5 ||
        ag_a == ag_action_6) &&
        (ag_k = ag_fl[ag_f = ag_pstt[ag_k]]) == 0) {
        ag_snd = ag_rns(ag_ptt[ag_f],ag_sx, (PCB).ss[*ag_sx]);
        (*ag_sx)++;
        continue;
    }
    if (ag_a == ag_action_9) {
      ag_snd = ag_pstt[ag_k];
      (*ag_sx)++;
      continue;
    }
    ag_flag = 0;
    break;
  }
  while ((PCB).btsx > ag_save_btsx) {
    *ag_sx = (PCB).bts[128 - (PCB).btsx];
    (PCB).ss[*ag_sx] = (PCB).ss[128 - (PCB).btsx--];
  }
  return ag_flag;
}


static int ag_tst_tkn(void) {
  int ag_rk, ag_sx, ag_snd = (PCB).sn;

  if ((PCB).rx < (PCB).fx) {
    (PCB).input_code = (PCB).lab[(PCB).rx++];
    (PCB).token_number = (a85parse_token_type) AG_TCV((PCB).input_code);}
  else {
    GET_INPUT;
    (PCB).lab[(PCB).fx++] = (PCB).input_code;
    (PCB).token_number = (a85parse_token_type) AG_TCV((PCB).input_code);
    (PCB).rx++;
  }
  if (ag_key_index[(PCB).sn]) {
    unsigned ag_k = ag_key_index[(PCB).sn];
    int ag_ch = CONVERT_CASE((PCB).input_code);
    while (ag_key_ch[ag_k] < ag_ch) ag_k++;
    if (ag_key_ch[ag_k] == ag_ch) ag_get_key_word(ag_k);
  }
  for (ag_rk = 0; ag_rk < (PCB).ag_lrss; ag_rk += 2) {
    ag_sx = (PCB).ag_rss[ag_rk];
    if (ag_sx > (PCB).ssx || ag_sx > (PCB).ag_min_depth) continue;
    (PCB).sn = (PCB).ag_rss[ag_rk + 1];
    if (ag_atx((PCB).token_number, &ag_sx, (PCB).sn)) break;
  }
  (PCB).sn = ag_snd;
  return ag_rk;
}

static void ag_set_error_procs(void);

static void ag_auto_resynch(void) {
  int ag_sx, ag_rk;
  int ag_rk1, ag_rk2, ag_tk1;
  (PCB).ss[(PCB).ssx] = (PCB).sn;
  if ((PCB).ag_error_depth && (PCB).ag_min_depth >= (PCB).ag_error_depth) {
    (PCB).ssx = (PCB).ag_error_depth;
    (PCB).sn = (PCB).ss[(PCB).ssx];
  }
  else {
    ag_diagnose();
    SYNTAX_ERROR;
    if ((PCB).exit_flag != AG_RUNNING_CODE) return;
    (PCB).ag_error_depth = (PCB).ag_min_depth = 0;
    (PCB).ag_lrss = 0;
    (PCB).ss[ag_sx = (PCB).ssx] = (PCB).sn;
    (PCB).ag_min_depth = (PCB).ag_rss[(PCB).ag_lrss++] = ag_sx;
    (PCB).ag_rss[(PCB).ag_lrss++] = (PCB).sn;
    while (ag_sx && (PCB).ag_lrss < 2*128) {
      int ag_t = 0, ag_x, ag_s, ag_sxs = ag_sx;

      while (ag_sx && (ag_t = ag_ctn[2*(PCB).sn]) == 0) (PCB).sn = (PCB).ss[--ag_sx];
      if (ag_t) (PCB).sn = (PCB).ss[ag_sx -= ag_ctn[2*(PCB).sn +1]];
      else {
        if (ag_sx == 0) (PCB).sn = 0;
        ag_t = ag_ptt[0];
      }
      if ((ag_s = ag_rns(ag_t, &ag_sx, (PCB).sn)) == 0) break;
      for (ag_x = 0; ag_x < (PCB).ag_lrss; ag_x += 2)
        if ((PCB).ag_rss[ag_x] == ag_sx + 1 && (PCB).ag_rss[ag_x+1] == ag_s) break;
      if (ag_x == (PCB).ag_lrss) {
        (PCB).ag_rss[(PCB).ag_lrss++] = ++ag_sx;
        (PCB).ag_rss[(PCB).ag_lrss++] = (PCB).sn = ag_s;
      }
      else if (ag_sx >= ag_sxs) ag_sx--;
    }
    ag_set_error_procs();
  }
  (PCB).rx = 0;
  if ((PCB).ssx > (PCB).ag_min_depth) (PCB).ag_min_depth = (PCB).ssx;
  while (1) {
    ag_rk1 = ag_tst_tkn();
    if ((PCB).token_number == 26)
      {(PCB).exit_flag = AG_SYNTAX_ERROR_CODE; return;}
    if (ag_rk1 < (PCB).ag_lrss) break;
    {(PCB).rx = 1; ag_track();}
  }
  ag_tk1 = (PCB).token_number;
  ag_track();
  ag_rk2 = ag_tst_tkn();
  if (ag_rk2 < ag_rk1) {ag_rk = ag_rk2; ag_track();}
  else {ag_rk = ag_rk1; (PCB).token_number = (a85parse_token_type) ag_tk1; (PCB).rx = 0;}
  (PCB).ag_min_depth = (PCB).ssx = (PCB).ag_rss[ag_rk++];
  (PCB).sn = (PCB).ss[(PCB).ssx] = (PCB).ag_rss[ag_rk];
  (PCB).sn = ag_jns((PCB).token_number);
  if ((PCB).ag_error_depth == 0 || (PCB).ag_error_depth > (PCB).ssx)
    (PCB).ag_error_depth = (PCB).ssx;
  if (++(PCB).ssx >= 128) {
    (PCB).exit_flag = AG_STACK_ERROR_CODE;
    PARSER_STACK_OVERFLOW;
    return;
  }
  (PCB).ss[(PCB).ssx] = (PCB).sn;
  (PCB).ag_tmp_depth = (PCB).ag_min_depth;
  (PCB).rx = 0;
  return;
}


static int ag_action_10_proc(void) {
  int ag_t = (PCB).token_number;
  (PCB).btsx = 0, (PCB).drt = -1;
  do {
    ag_track();
    if ((PCB).rx < (PCB).fx) {
      (PCB).input_code = (PCB).lab[(PCB).rx++];
      (PCB).token_number = (a85parse_token_type) AG_TCV((PCB).input_code);}
    else {
      GET_INPUT;
      (PCB).lab[(PCB).fx++] = (PCB).input_code;
      (PCB).token_number = (a85parse_token_type) AG_TCV((PCB).input_code);
      (PCB).rx++;
    }
    if (ag_key_index[(PCB).sn]) {
      unsigned ag_k = ag_key_index[(PCB).sn];
      int ag_ch = CONVERT_CASE((PCB).input_code);
      while (ag_key_ch[ag_k] < ag_ch) ag_k++;
      if (ag_key_ch[ag_k] == ag_ch) ag_get_key_word(ag_k);
    }
  } while ((PCB).token_number == (a85parse_token_type) ag_t);
  (PCB).rx = 0;
  return 1;
}

static int ag_action_11_proc(void) {
  int ag_t = (PCB).token_number;

  (PCB).btsx = 0, (PCB).drt = -1;
  do {
    (*(int *) &(PCB).vs[(PCB).ssx]) = *(PCB).lab;
    (PCB).ssx--;
    ag_track();
    ag_ra();
    if ((PCB).exit_flag != AG_RUNNING_CODE) return 0;
    (PCB).ssx++;
    if ((PCB).rx < (PCB).fx) {
      (PCB).input_code = (PCB).lab[(PCB).rx++];
      (PCB).token_number = (a85parse_token_type) AG_TCV((PCB).input_code);}
    else {
      GET_INPUT;
      (PCB).lab[(PCB).fx++] = (PCB).input_code;
      (PCB).token_number = (a85parse_token_type) AG_TCV((PCB).input_code);
      (PCB).rx++;
    }
    if (ag_key_index[(PCB).sn]) {
      unsigned ag_k = ag_key_index[(PCB).sn];
      int ag_ch = CONVERT_CASE((PCB).input_code);
      while (ag_key_ch[ag_k] < ag_ch) ag_k++;
      if (ag_key_ch[ag_k] == ag_ch) ag_get_key_word(ag_k);
    }
  }
  while ((PCB).token_number == (a85parse_token_type) ag_t);
  (PCB).rx = 0;
  return 1;
}

static int ag_action_3_r_proc(void) {
  int ag_sd = ag_fl[(PCB).ag_ap] - 1;
  if (ag_sd) (PCB).sn = (PCB).ss[(PCB).ssx -= ag_sd];
  (PCB).btsx = 0, (PCB).drt = -1;
  (PCB).reduction_token = (a85parse_token_type) ag_ptt[(PCB).ag_ap];
  ag_ra();
  return (PCB).exit_flag == AG_RUNNING_CODE;
}

static int ag_action_3_s_proc(void) {
  int ag_sd = ag_fl[(PCB).ag_ap] - 1;
  if (ag_sd) (PCB).sn = (PCB).ss[(PCB).ssx -= ag_sd];
  (PCB).btsx = 0, (PCB).drt = -1;
  (PCB).reduction_token = (a85parse_token_type) ag_ptt[(PCB).ag_ap];
  ag_ra();
  return (PCB).exit_flag == AG_RUNNING_CODE;;
}

static int ag_action_4_r_proc(void) {
  int ag_sd = ag_fl[(PCB).ag_ap] - 1;
  if (ag_sd) (PCB).sn = (PCB).ss[(PCB).ssx -= ag_sd];
  (PCB).reduction_token = (a85parse_token_type) ag_ptt[(PCB).ag_ap];
  return 1;
}

static int ag_action_2_proc(void) {
  (PCB).btsx = 0, (PCB).drt = -1;
  if ((PCB).ssx >= 128) {
    (PCB).exit_flag = AG_STACK_ERROR_CODE;
    PARSER_STACK_OVERFLOW;
  }
  (*(int *) &(PCB).vs[(PCB).ssx]) = *(PCB).lab;
  (PCB).ss[(PCB).ssx] = (PCB).sn;
  (PCB).ssx++;
  (PCB).sn = (PCB).ag_ap;
  ag_track();
  return 0;
}

static int ag_action_9_proc(void) {
  if ((PCB).drt == -1) {
    (PCB).drt=(PCB).token_number;
    (PCB).dssx=(PCB).ssx;
    (PCB).dsn=(PCB).sn;
  }
  ag_prot();
  (PCB).vs[(PCB).ssx] = ag_null_value;
  (PCB).ss[(PCB).ssx] = (PCB).sn;
  (PCB).ssx++;
  (PCB).sn = (PCB).ag_ap;
  (PCB).rx = 0;
  return (PCB).exit_flag == AG_RUNNING_CODE;
}

static int ag_action_2_r_proc(void) {
  (PCB).ssx++;
  (PCB).sn = (PCB).ag_ap;
  return 0;
}

static int ag_action_7_proc(void) {
  --(PCB).ssx;
  (PCB).rx = 0;
  (PCB).exit_flag = AG_SUCCESS_CODE;
  return 0;
}

static int ag_action_1_proc(void) {
  ag_track();
  (PCB).exit_flag = AG_SUCCESS_CODE;
  return 0;
}

static int ag_action_1_r_proc(void) {
  (PCB).exit_flag = AG_SUCCESS_CODE;
  return 0;
}

static int ag_action_1_s_proc(void) {
  (PCB).exit_flag = AG_SUCCESS_CODE;
  return 0;
}

static int ag_action_4_proc(void) {
  int ag_sd = ag_fl[(PCB).ag_ap] - 1;
  (PCB).reduction_token = (a85parse_token_type) ag_ptt[(PCB).ag_ap];
  (PCB).btsx = 0, (PCB).drt = -1;
  (*(int *) &(PCB).vs[(PCB).ssx]) = *(PCB).lab;
  if (ag_sd) (PCB).sn = (PCB).ss[(PCB).ssx -= ag_sd];
  else (PCB).ss[(PCB).ssx] = (PCB).sn;
  ag_track();
  while ((PCB).exit_flag == AG_RUNNING_CODE) {
    unsigned ag_t1 = ag_sbe[(PCB).sn] + 1;
    unsigned ag_t2 = ag_sbt[(PCB).sn+1] - 1;
    do {
      unsigned ag_tx = (ag_t1 + ag_t2)/2;
      if (ag_tstt[ag_tx] < (const unsigned short)(PCB).reduction_token) ag_t1 = ag_tx + 1;
      else ag_t2 = ag_tx;
    } while (ag_t1 < ag_t2);
    (PCB).ag_ap = ag_pstt[ag_t1];
    if ((*(PCB).s_procs[ag_astt[ag_t1]])() == 0) break;
  }
  return 0;
}

static int ag_action_3_proc(void) {
  int ag_sd = ag_fl[(PCB).ag_ap] - 1;
  (PCB).btsx = 0, (PCB).drt = -1;
  (*(int *) &(PCB).vs[(PCB).ssx]) = *(PCB).lab;
  if (ag_sd) (PCB).sn = (PCB).ss[(PCB).ssx -= ag_sd];
  else (PCB).ss[(PCB).ssx] = (PCB).sn;
  ag_track();
  (PCB).reduction_token = (a85parse_token_type) ag_ptt[(PCB).ag_ap];
  ag_ra();
  while ((PCB).exit_flag == AG_RUNNING_CODE) {
    unsigned ag_t1 = ag_sbe[(PCB).sn] + 1;
    unsigned ag_t2 = ag_sbt[(PCB).sn+1] - 1;
    do {
      unsigned ag_tx = (ag_t1 + ag_t2)/2;
      if (ag_tstt[ag_tx] < (const unsigned short)(PCB).reduction_token) ag_t1 = ag_tx + 1;
      else ag_t2 = ag_tx;
    } while (ag_t1 < ag_t2);
    (PCB).ag_ap = ag_pstt[ag_t1];
    if ((*(PCB).s_procs[ag_astt[ag_t1]])() == 0) break;
  }
  return 0;
}

static int ag_action_8_proc(void) {
  ag_undo();
  (PCB).rx = 0;
  ag_auto_resynch();
  return (PCB).exit_flag == AG_RUNNING_CODE;
}

static int ag_action_5_proc(void) {
  int ag_sd = ag_fl[(PCB).ag_ap];
  (PCB).btsx = 0, (PCB).drt = -1;
  if (ag_sd) (PCB).sn = (PCB).ss[(PCB).ssx -= ag_sd];
  else {
    (PCB).ss[(PCB).ssx] = (PCB).sn;
  }
  (PCB).rx = 0;
  (PCB).reduction_token = (a85parse_token_type) ag_ptt[(PCB).ag_ap];
  ag_ra();
  while ((PCB).exit_flag == AG_RUNNING_CODE) {
    unsigned ag_t1 = ag_sbe[(PCB).sn] + 1;
    unsigned ag_t2 = ag_sbt[(PCB).sn+1] - 1;
    do {
      unsigned ag_tx = (ag_t1 + ag_t2)/2;
      if (ag_tstt[ag_tx] < (const unsigned short)(PCB).reduction_token) ag_t1 = ag_tx + 1;
      else ag_t2 = ag_tx;
    } while (ag_t1 < ag_t2);
    (PCB).ag_ap = ag_pstt[ag_t1];
    if ((*(PCB).r_procs[ag_astt[ag_t1]])() == 0) break;
  }
  return (PCB).exit_flag == AG_RUNNING_CODE;
}

static int ag_action_6_proc(void) {
  int ag_sd = ag_fl[(PCB).ag_ap];
  (PCB).reduction_token = (a85parse_token_type) ag_ptt[(PCB).ag_ap];
  if ((PCB).drt == -1) {
    (PCB).drt=(PCB).token_number;
    (PCB).dssx=(PCB).ssx;
    (PCB).dsn=(PCB).sn;
  }
  if (ag_sd) {
    (PCB).sn = (PCB).ss[(PCB).ssx -= ag_sd];
  }
  else {
    ag_prot();
    (PCB).vs[(PCB).ssx] = ag_null_value;
    (PCB).ss[(PCB).ssx] = (PCB).sn;
  }
  (PCB).rx = 0;
  while ((PCB).exit_flag == AG_RUNNING_CODE) {
    unsigned ag_t1 = ag_sbe[(PCB).sn] + 1;
    unsigned ag_t2 = ag_sbt[(PCB).sn+1] - 1;
    do {
      unsigned ag_tx = (ag_t1 + ag_t2)/2;
      if (ag_tstt[ag_tx] < (const unsigned short)(PCB).reduction_token) ag_t1 = ag_tx + 1;
      else ag_t2 = ag_tx;
    } while (ag_t1 < ag_t2);
    (PCB).ag_ap = ag_pstt[ag_t1];
    if ((*(PCB).r_procs[ag_astt[ag_t1]])() == 0) break;
  }
  return (PCB).exit_flag == AG_RUNNING_CODE;
}


static void ag_check_depth(int ag_fl) {
  int ag_sx = (PCB).ssx - ag_fl;
  if ((PCB).ag_error_depth && ag_sx < (PCB).ag_tmp_depth) (PCB).ag_tmp_depth = ag_sx;
}

static int ag_action_3_er_proc(void) {
  ag_check_depth(ag_fl[(PCB).ag_ap] - 1);
  return ag_action_4_r_proc();
}

static int ag_action_2_e_proc(void) {
  ag_action_2_proc();
  (PCB).ag_min_depth = (PCB).ag_tmp_depth;
  return 0;
}

static int ag_action_4_e_proc(void) {
  ag_check_depth(ag_fl[(PCB).ag_ap] - 1);
  (PCB).ag_min_depth = (PCB).ag_tmp_depth;
  return ag_action_4_proc();
}

static int ag_action_6_e_proc(void) {
  ag_check_depth(ag_fl[(PCB).ag_ap]);
  return ag_action_6_proc();
}

static int ag_action_11_e_proc(void) {
  return ag_action_10_proc();
}

static int (*ag_r_procs_error[])(void) = {
  ag_action_1_r_proc,
  ag_action_2_r_proc,
  ag_action_3_er_proc,
  ag_action_3_er_proc
};

static int (*ag_s_procs_error[])(void) = {
  ag_action_1_s_proc,
  ag_action_2_r_proc,
  ag_action_3_er_proc,
  ag_action_3_er_proc
};

static int (*ag_gt_procs_error[])(void) = {
  ag_action_1_proc,
  ag_action_2_e_proc,
  ag_action_4_e_proc,
  ag_action_4_e_proc,
  ag_action_6_e_proc,
  ag_action_6_e_proc,
  ag_action_7_proc,
  ag_action_8_proc,
  ag_action_9_proc,
  ag_action_10_proc,
  ag_action_11_e_proc,
  ag_action_8_proc
};

static void ag_set_error_procs(void) {
  (PCB).gt_procs = ag_gt_procs_error;
  (PCB).r_procs = ag_r_procs_error;
  (PCB).s_procs = ag_s_procs_error;
}


void init_a85parse(void) {
  (PCB).rx = (PCB).fx = 0;
  (PCB).gt_procs = ag_gt_procs_scan;
  (PCB).r_procs = ag_r_procs_scan;
  (PCB).s_procs = ag_s_procs_scan;
  (PCB).ag_error_depth = (PCB).ag_min_depth = (PCB).ag_tmp_depth = 0;
  (PCB).ag_resynch_active = 0;
  (PCB).ss[0] = (PCB).sn = (PCB).ssx = 0;
  (PCB).exit_flag = AG_RUNNING_CODE;
  (PCB).line = FIRST_LINE;
  (PCB).column = FIRST_COLUMN;
  (PCB).btsx = 0, (PCB).drt = -1;
}

void a85parse(void) {
  init_a85parse();
  (PCB).exit_flag = AG_RUNNING_CODE;
  while ((PCB).exit_flag == AG_RUNNING_CODE) {
    unsigned ag_t1 = ag_sbt[(PCB).sn];
    if (ag_tstt[ag_t1]) {
      unsigned ag_t2 = ag_sbe[(PCB).sn] - 1;
      if ((PCB).rx < (PCB).fx) {
        (PCB).input_code = (PCB).lab[(PCB).rx++];
        (PCB).token_number = (a85parse_token_type) AG_TCV((PCB).input_code);}
      else {
        GET_INPUT;
        (PCB).lab[(PCB).fx++] = (PCB).input_code;
        (PCB).token_number = (a85parse_token_type) AG_TCV((PCB).input_code);
        (PCB).rx++;
      }
      if (ag_key_index[(PCB).sn]) {
        unsigned ag_k = ag_key_index[(PCB).sn];
        int ag_ch = CONVERT_CASE((PCB).input_code);
        while (ag_key_ch[ag_k] < ag_ch) ag_k++;
        if (ag_key_ch[ag_k] == ag_ch) ag_get_key_word(ag_k);
      }
      do {
        unsigned ag_tx = (ag_t1 + ag_t2)/2;
        if (ag_tstt[ag_tx] > (const unsigned short)(PCB).token_number)
          ag_t1 = ag_tx + 1;
        else ag_t2 = ag_tx;
      } while (ag_t1 < ag_t2);
      if (ag_tstt[ag_t1] != (const unsigned short)(PCB).token_number)
        ag_t1 = ag_sbe[(PCB).sn];
    }
    (PCB).ag_ap = ag_pstt[ag_t1];
    (*(PCB).gt_procs[ag_astt[ag_t1]])();
  }
}


