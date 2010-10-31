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

#ifndef A85PARSE_H_1176345433
#include "a85parse.h"
#endif

#ifndef A85PARSE_H_1176345433
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

/*  Line 493, C:/Projects/VirtualT/src/a85parse.syn */
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
	{
		// Add error indicating failure to open file
		gAsm->m_Errors.Add((char *) "Error opening file");
		return FALSE;
	}

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

#define GET_INPUT {(PCB).input_code = gAsm->m_fd != 0 ? \
	fgetc(gAsm->m_fd) : 0; if ((PCB).input_code == 13) (PCB).input_code = fgetc(gAsm->m_fd);}

#define SYNTAX_ERROR {MString string;  if ((PCB).error_frame_token != 0) \
  string.Format("Malformed %s - %s, line %d, column %d", \
  TOKEN_NAMES[(PCB).error_frame_token], (PCB).error_message, (PCB).line, (PCB).column); else \
  string.Format("%s, line %d, column %d", \
  (PCB).error_message, (PCB).line, (PCB).column); \
  gAsm->m_Errors.Add(string); ss_idx = 0; ss_len = 0; }

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
/* Line 103, C:/Projects/VirtualT/src/a85parse.syn */
 gAsm->label(ss[ss_idx--]); gAsm->directive_ds(); 
}

#define ag_rp_23() (gAsm->directive_ds())

static void ag_rp_24(void) {
/* Line 105, C:/Projects/VirtualT/src/a85parse.syn */
 gAsm->label(ss[ss_idx--]); gAsm->directive_db(); 
}

#define ag_rp_25() (gAsm->directive_db())

static void ag_rp_26(void) {
/* Line 107, C:/Projects/VirtualT/src/a85parse.syn */
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

#define ag_rp_46() (expression_list_literal())

#define ag_rp_47() (expression_list_equation())

#define ag_rp_48() (expression_list_equation())

#define ag_rp_49() (expression_list_literal())

#define ag_rp_50() (expression_list_literal())

#define ag_rp_51() (gNameList->Add(ss[ss_idx--]))

#define ag_rp_52() (gNameList->Add(ss[ss_idx--]))

static void ag_rp_53(void) {
/* Line 151, C:/Projects/VirtualT/src/a85parse.syn */
 strcpy(ss[++ss_idx], "$"); ss_len = 1; 
}

static void ag_rp_54(int c) {
/* Line 154, C:/Projects/VirtualT/src/a85parse.syn */
 ss[++ss_idx][0] = c; ss[ss_idx][1] = 0; ss_len = 1; 
}

static void ag_rp_55(int c) {
/* Line 155, C:/Projects/VirtualT/src/a85parse.syn */
 ss[ss_idx][ss_len++] = c; ss[ss_idx][ss_len] = 0; 
}

static void ag_rp_56(int c) {
/* Line 156, C:/Projects/VirtualT/src/a85parse.syn */
 ss[ss_idx][ss_len++] = c; ss[ss_idx][ss_len] = 0; 
}

static void ag_rp_57(int c) {
/* Line 159, C:/Projects/VirtualT/src/a85parse.syn */
 ss[++ss_idx][0] = c; ss[ss_idx][1] = 0; ss_len = 1; 
}

static void ag_rp_58(int c) {
/* Line 160, C:/Projects/VirtualT/src/a85parse.syn */
 ss[ss_idx][ss_len++] = c; ss[ss_idx][ss_len] = 0; 
}

static void ag_rp_59(int ch1, int ch2) {
/* Line 166, C:/Projects/VirtualT/src/a85parse.syn */
 ss_idx++; ss_len = 2; sprintf(ss[ss_idx], "%c%c", ch1, ch2); 
}

static void ag_rp_60(int c) {
/* Line 167, C:/Projects/VirtualT/src/a85parse.syn */
 ss[ss_idx][ss_len++] = c; ss[ss_idx][ss_len] = 0; 
}

static void ag_rp_61(void) {
/* Line 174, C:/Projects/VirtualT/src/a85parse.syn */
 ss_idx++; ss_len = 0; 
}

static void ag_rp_62(int c) {
/* Line 175, C:/Projects/VirtualT/src/a85parse.syn */
 ss[ss_idx][ss_len++] = c; ss[ss_idx][ss_len] = 0; 
}

static void ag_rp_63(int c) {
/* Line 176, C:/Projects/VirtualT/src/a85parse.syn */
 ss[ss_idx][ss_len++] = '\\'; ss[ss_idx][ss_len++] = c; ss[ss_idx][ss_len] = 0; 
}

static void ag_rp_64(void) {
/* Line 181, C:/Projects/VirtualT/src/a85parse.syn */
 ss[ss_idx][ss_len++] = '>'; ss[ss_idx][ss_len] = 0; 
}

static void ag_rp_65(void) {
/* Line 184, C:/Projects/VirtualT/src/a85parse.syn */
 ss[++ss_idx][0] = '<'; ss[ss_idx][1] = 0; ss_len = 1; 
}

static void ag_rp_66(int c) {
/* Line 185, C:/Projects/VirtualT/src/a85parse.syn */
 ss[ss_idx][ss_len++] = c; ss[ss_idx][ss_len] = 0; 
}

static void ag_rp_67(int c) {
/* Line 186, C:/Projects/VirtualT/src/a85parse.syn */
 ss[ss_idx][ss_len++] = '\\'; ss[ss_idx][ss_len++] = c; ss[ss_idx][ss_len] = 0; 
}

#define ag_rp_68() (gAsm->label(ss[ss_idx--]))

#define ag_rp_69() (condition(-1))

#define ag_rp_70() (condition(COND_NOCMP))

#define ag_rp_71() (condition(COND_EQ))

#define ag_rp_72() (condition(COND_NE))

#define ag_rp_73() (condition(COND_GE))

#define ag_rp_74() (condition(COND_LE))

#define ag_rp_75() (condition(COND_GT))

#define ag_rp_76() (condition(COND_LT))

#define ag_rp_77() (gEq->Add(RPN_BITOR))

#define ag_rp_78() (gEq->Add(RPN_BITOR))

#define ag_rp_79() (gEq->Add(RPN_BITXOR))

#define ag_rp_80() (gEq->Add(RPN_BITXOR))

#define ag_rp_81() (gEq->Add(RPN_BITAND))

#define ag_rp_82() (gEq->Add(RPN_BITAND))

#define ag_rp_83() (gEq->Add(RPN_LEFTSHIFT))

#define ag_rp_84() (gEq->Add(RPN_LEFTSHIFT))

#define ag_rp_85() (gEq->Add(RPN_RIGHTSHIFT))

#define ag_rp_86() (gEq->Add(RPN_RIGHTSHIFT))

#define ag_rp_87() (gEq->Add(RPN_ADD))

#define ag_rp_88() (gEq->Add(RPN_SUBTRACT))

#define ag_rp_89() (gEq->Add(RPN_MULTIPLY))

#define ag_rp_90() (gEq->Add(RPN_DIVIDE))

#define ag_rp_91() (gEq->Add(RPN_MODULUS))

#define ag_rp_92() (gEq->Add(RPN_MODULUS))

#define ag_rp_93() (gEq->Add(RPN_EXPONENT))

#define ag_rp_94() (gEq->Add(RPN_NOT))

#define ag_rp_95() (gEq->Add(RPN_NOT))

#define ag_rp_96() (gEq->Add(RPN_BITNOT))

#define ag_rp_97(n) (gEq->Add((double) n))

#define ag_rp_98() (gEq->Add(ss[ss_idx--]))

#define ag_rp_99() (gEq->Add(RPN_FLOOR))

#define ag_rp_100() (gEq->Add(RPN_CEIL))

#define ag_rp_101() (gEq->Add(RPN_LN))

#define ag_rp_102() (gEq->Add(RPN_LOG))

#define ag_rp_103() (gEq->Add(RPN_SQRT))

#define ag_rp_104() (gEq->Add(RPN_IP))

#define ag_rp_105() (gEq->Add(RPN_FP))

#define ag_rp_106(n) (n)

#define ag_rp_107(r) (r)

#define ag_rp_108(n) (n)

#define ag_rp_109() (conv_to_dec())

#define ag_rp_110() (conv_to_hex())

#define ag_rp_111() (conv_to_hex())

#define ag_rp_112() (conv_to_hex())

#define ag_rp_113() (conv_to_bin())

#define ag_rp_114() (conv_to_oct())

#define ag_rp_115() (conv_to_dec())

static void ag_rp_116(int n) {
/* Line 312, C:/Projects/VirtualT/src/a85parse.syn */
 int_len = 1; integer[0] = n; integer[1] = 0; 
}

static void ag_rp_117(int n) {
/* Line 313, C:/Projects/VirtualT/src/a85parse.syn */
 int_len = 2; integer[0] = '-', integer[1] = n; integer[2] = 0; 
}

static void ag_rp_118(int n) {
/* Line 314, C:/Projects/VirtualT/src/a85parse.syn */
 int_len = 1; integer[0] = n; integer[1] = 0; 
}

static void ag_rp_119(int n) {
/* Line 315, C:/Projects/VirtualT/src/a85parse.syn */
 integer[int_len++] = n; integer[int_len] = 0; 
}

static void ag_rp_120(int n) {
/* Line 320, C:/Projects/VirtualT/src/a85parse.syn */
 int_len = 1; integer[0] = n; integer[1] = 0; 
}

static void ag_rp_121(int n) {
/* Line 321, C:/Projects/VirtualT/src/a85parse.syn */
 integer[int_len++] = n; integer[int_len] = 0; 
}

static void ag_rp_122(int n) {
/* Line 324, C:/Projects/VirtualT/src/a85parse.syn */
 integer[int_len++] = n; integer[int_len] = 0; 
}

static void ag_rp_123(int n) {
/* Line 325, C:/Projects/VirtualT/src/a85parse.syn */
 integer[int_len++] = n; integer[int_len] = 0; 
}

#define ag_rp_124(n) (n)

#define ag_rp_125(n1, n2) ((n1 << 8) | n2)

#define ag_rp_126() ('\\')

#define ag_rp_127(n) (n)

#define ag_rp_128() ('\\')

#define ag_rp_129() ('\n')

#define ag_rp_130() ('\t')

#define ag_rp_131() ('\r')

#define ag_rp_132() ('\0')

#define ag_rp_133() ('\'')

#define ag_rp_134() ('\'')

static double ag_rp_135(void) {
/* Line 346, C:/Projects/VirtualT/src/a85parse.syn */
 gDivisor = 1.0; return (double) conv_to_dec(); 
}

static double ag_rp_136(int d) {
/* Line 347, C:/Projects/VirtualT/src/a85parse.syn */
 gDivisor = 10.0; return ((double) (d - '0') / gDivisor); 
}

static double ag_rp_137(double r, int d) {
/* Line 348, C:/Projects/VirtualT/src/a85parse.syn */
 gDivisor *= 10.0; return (r + (double) (d - '0') / gDivisor); 
}

#define ag_rp_138() (reg[reg_cnt++] = '0')

#define ag_rp_139() (reg[reg_cnt++] = '1')

#define ag_rp_140() (reg[reg_cnt++] = '2')

#define ag_rp_141() (reg[reg_cnt++] = '3')

#define ag_rp_142() (reg[reg_cnt++] = '4')

#define ag_rp_143() (reg[reg_cnt++] = '5')

#define ag_rp_144() (reg[reg_cnt++] = '6')

#define ag_rp_145() (reg[reg_cnt++] = '7')

#define ag_rp_146() (reg[reg_cnt++] = '0')

#define ag_rp_147() (reg[reg_cnt++] = '1')

#define ag_rp_148() (reg[reg_cnt++] = '2')

#define ag_rp_149() (reg[reg_cnt++] = '3')

#define ag_rp_150() (reg[reg_cnt++] = '0')

#define ag_rp_151() (reg[reg_cnt++] = '1')

#define ag_rp_152() (reg[reg_cnt++] = '2')

#define ag_rp_153() (reg[reg_cnt++] = '3')

#define ag_rp_154() (reg[reg_cnt++] = '3')

#define ag_rp_155() (reg[reg_cnt++] = '0')

#define ag_rp_156() (reg[reg_cnt++] = '1')

#define ag_rp_157() (gAsm->opcode_arg_equ8		(OPCODE_ACI))

#define ag_rp_158() (gAsm->opcode_arg_1reg		(OPCODE_ADC))

#define ag_rp_159() (gAsm->opcode_arg_1reg		(OPCODE_ADD))

#define ag_rp_160() (gAsm->opcode_arg_equ8		(OPCODE_ADI))

#define ag_rp_161() (gAsm->opcode_arg_1reg		(OPCODE_ANA))

#define ag_rp_162() (gAsm->opcode_arg_equ8		(OPCODE_ANI))

#define ag_rp_163() (gAsm->opcode_arg_0		(OPCODE_ASHR))

#define ag_rp_164() (gAsm->opcode_arg_equ16	(OPCODE_CALL))

#define ag_rp_165() (gAsm->opcode_arg_equ16	(OPCODE_CC))

#define ag_rp_166() (gAsm->opcode_arg_equ16	(OPCODE_CM))

#define ag_rp_167() (gAsm->opcode_arg_0		(OPCODE_CMA))

#define ag_rp_168() (gAsm->opcode_arg_0		(OPCODE_CMC))

#define ag_rp_169() (gAsm->opcode_arg_1reg		(OPCODE_CMP))

#define ag_rp_170() (gAsm->opcode_arg_equ16	(OPCODE_CNC))

#define ag_rp_171() (gAsm->opcode_arg_equ16	(OPCODE_CNZ))

#define ag_rp_172() (gAsm->opcode_arg_equ16	(OPCODE_CP))

#define ag_rp_173() (gAsm->opcode_arg_equ16	(OPCODE_CPE))

#define ag_rp_174() (gAsm->opcode_arg_equ8		(OPCODE_CPI))

#define ag_rp_175() (gAsm->opcode_arg_equ16	(OPCODE_CPO))

#define ag_rp_176() (gAsm->opcode_arg_equ16	(OPCODE_CZ))

#define ag_rp_177() (gAsm->opcode_arg_0		(OPCODE_DAA))

#define ag_rp_178() (gAsm->opcode_arg_1reg		(OPCODE_DAD))

#define ag_rp_179() (gAsm->opcode_arg_1reg		(OPCODE_DCR))

#define ag_rp_180() (gAsm->opcode_arg_1reg		(OPCODE_DCX))

#define ag_rp_181() (gAsm->opcode_arg_0		(OPCODE_DI))

#define ag_rp_182() (gAsm->opcode_arg_0		(OPCODE_DSUB))

#define ag_rp_183() (gAsm->opcode_arg_0		(OPCODE_EI))

#define ag_rp_184() (gAsm->opcode_arg_0		(OPCODE_HLT))

#define ag_rp_185() (gAsm->opcode_arg_equ8		(OPCODE_IN))

#define ag_rp_186() (gAsm->opcode_arg_1reg		(OPCODE_INR))

#define ag_rp_187() (gAsm->opcode_arg_1reg		(OPCODE_INX))

#define ag_rp_188() (gAsm->opcode_arg_equ16	(OPCODE_JC))

#define ag_rp_189() (gAsm->opcode_arg_equ16	(OPCODE_JD))

#define ag_rp_190() (gAsm->opcode_arg_equ16	(OPCODE_JD))

#define ag_rp_191() (gAsm->opcode_arg_equ16	(OPCODE_JM))

#define ag_rp_192() (gAsm->opcode_arg_equ16	(OPCODE_JMP))

#define ag_rp_193() (gAsm->opcode_arg_equ16	(OPCODE_JNC))

#define ag_rp_194() (gAsm->opcode_arg_equ16	(OPCODE_JND))

#define ag_rp_195() (gAsm->opcode_arg_equ16	(OPCODE_JND))

#define ag_rp_196() (gAsm->opcode_arg_equ16	(OPCODE_JNZ))

#define ag_rp_197() (gAsm->opcode_arg_equ16	(OPCODE_JP))

#define ag_rp_198() (gAsm->opcode_arg_equ16	(OPCODE_JPE))

#define ag_rp_199() (gAsm->opcode_arg_equ16	(OPCODE_JPO))

#define ag_rp_200() (gAsm->opcode_arg_equ16	(OPCODE_JZ))

#define ag_rp_201() (gAsm->opcode_arg_equ16	(OPCODE_LDA))

#define ag_rp_202() (gAsm->opcode_arg_1reg		(OPCODE_LDAX))

#define ag_rp_203() (gAsm->opcode_arg_equ8		(OPCODE_LDEH))

#define ag_rp_204() (gAsm->opcode_arg_equ8		(OPCODE_LDES))

#define ag_rp_205() (gAsm->opcode_arg_equ16	(OPCODE_LHLD))

#define ag_rp_206() (gAsm->opcode_arg_0		(OPCODE_LHLX))

#define ag_rp_207() (gAsm->opcode_arg_1reg_equ8(OPCODE_LXI))

#define ag_rp_208() (gAsm->opcode_arg_2reg		(OPCODE_MOV))

#define ag_rp_209() (gAsm->opcode_arg_1reg_equ8(OPCODE_MVI))

#define ag_rp_210() (gAsm->opcode_arg_0		(OPCODE_NOP))

#define ag_rp_211() (gAsm->opcode_arg_1reg		(OPCODE_ORA))

#define ag_rp_212() (gAsm->opcode_arg_equ8		(OPCODE_ORI))

#define ag_rp_213() (gAsm->opcode_arg_equ8		(OPCODE_OUT))

#define ag_rp_214() (gAsm->opcode_arg_0		(OPCODE_PCHL))

#define ag_rp_215() (gAsm->opcode_arg_1reg		(OPCODE_POP))

#define ag_rp_216() (gAsm->opcode_arg_1reg		(OPCODE_PUSH))

#define ag_rp_217() (gAsm->opcode_arg_0		(OPCODE_RAL))

#define ag_rp_218() (gAsm->opcode_arg_0		(OPCODE_RAR))

#define ag_rp_219() (gAsm->opcode_arg_0		(OPCODE_RC))

#define ag_rp_220() (gAsm->opcode_arg_0		(OPCODE_RET))

#define ag_rp_221() (gAsm->opcode_arg_0		(OPCODE_RIM))

#define ag_rp_222() (gAsm->opcode_arg_0		(OPCODE_RLC))

#define ag_rp_223() (gAsm->opcode_arg_0		(OPCODE_RLDE))

#define ag_rp_224() (gAsm->opcode_arg_0		(OPCODE_RM))

#define ag_rp_225() (gAsm->opcode_arg_0		(OPCODE_RNC))

#define ag_rp_226() (gAsm->opcode_arg_0		(OPCODE_RNZ))

#define ag_rp_227() (gAsm->opcode_arg_0		(OPCODE_RP))

#define ag_rp_228() (gAsm->opcode_arg_0		(OPCODE_RPE))

#define ag_rp_229() (gAsm->opcode_arg_0		(OPCODE_RPO))

#define ag_rp_230() (gAsm->opcode_arg_0		(OPCODE_RRC))

#define ag_rp_231(c) (gAsm->opcode_arg_imm		(OPCODE_RST, c))

#define ag_rp_232() (gAsm->opcode_arg_0		(OPCODE_RZ))

#define ag_rp_233() (gAsm->opcode_arg_1reg		(OPCODE_SBB))

#define ag_rp_234() (gAsm->opcode_arg_equ8		(OPCODE_SBI))

#define ag_rp_235() (gAsm->opcode_arg_equ16	(OPCODE_SHLD))

#define ag_rp_236() (gAsm->opcode_arg_0		(OPCODE_SHLX))

#define ag_rp_237() (gAsm->opcode_arg_0		(OPCODE_SIM))

#define ag_rp_238() (gAsm->opcode_arg_0		(OPCODE_SPHL))

#define ag_rp_239() (gAsm->opcode_arg_equ16	(OPCODE_STA))

#define ag_rp_240() (gAsm->opcode_arg_1reg		(OPCODE_STAX))

#define ag_rp_241() (gAsm->opcode_arg_0		(OPCODE_STC))

#define ag_rp_242() (gAsm->opcode_arg_1reg		(OPCODE_SUB))

#define ag_rp_243() (gAsm->opcode_arg_equ8		(OPCODE_SUI))

#define ag_rp_244() (gAsm->opcode_arg_0		(OPCODE_XCHG))

#define ag_rp_245() (gAsm->opcode_arg_1reg		(OPCODE_XRA))

#define ag_rp_246() (gAsm->opcode_arg_equ8		(OPCODE_XRI))

#define ag_rp_247() (gAsm->opcode_arg_0		(OPCODE_XTHL))


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
    0,  0,  0,  0,  0,  0,  0,  0,  1,  1,  1,  1,  1,  2,  3,  4,  5,  6,
    7,  8,  9, 10, 11, 12, 13,  0,  0, 14, 15, 16, 17, 18, 19, 20, 21, 22,
   23, 24, 25, 26, 27, 28, 29, 30, 31, 32, 33, 34, 35, 36, 37, 38, 39, 40,
    0, 41, 42, 43, 44, 45, 46, 47, 48,  0, 49, 50, 51, 52, 53, 54,  0, 55,
   56,  0, 57, 58, 59,  0, 60, 61, 62, 63,  0, 64, 65, 66,  0,  0,  0, 67,
    0,  0, 68,  0,  0, 69,  0,  0, 70,  0,  0, 71,  0,  0, 72,  0,  0, 73,
   74,  0, 75, 76,  0, 77, 78,  0, 79, 80, 81, 82,  0, 83, 84,  0, 85, 86,
   87, 88, 89,  0, 90, 91, 92, 93, 94,  0,  0, 95, 96, 97, 98, 99,100,101,
  102,103,104,105,106,107,108,109,110,111,112,113,114,115,  0,  0,116,117,
  118,119,120,121,122,123,124,125,126,127,128,129,130,  0,131,132,133,134,
  135,136,137,138,139,140,141,142,143,144,145,146,147,148,149,150,151,152,
    0,  0,  0,153,154,155,156,157,158,159,160,161,162,163,164,165,166,167,
  168,169,170,171,172,173,174,175,176,177,178,179,180,181,182,183,184,185,
  186,187,188,189,190,191,192,193,194,195,196,197,198,199,200,201,202,203,
  204,205,206,207,208,209,210,211,212,213,214,215,216,217,218,219,220,221,
  222,223,224,225,226,227,228,229,230,231,232,233,234,235,236,237,238,239,
  240,241,242,243
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
  1,292,  1,296,  1,298,  1,306,  1,307,  1,308,  1,309,  1,310,
  1,311,  1,312,  1,313,  1,314,  1,315,  1,316,  1,317,  1,318,
  1,319,  1,320,  1,321,  1,322,  1,323,  1,325,  1,327,  1,328,
  1,329,  1,330,  1, 86,  1,332,  1,335,  1,337,  1,339,  1,341,
  1,343,  1,346,  1,348,  1,350,  1,352,  1,354,  1,360,  1,363,
  1,367,  1,368,  1,369,  1,370,  1,371,  1,372,  1,373,  1,377,
  1,375,  1,160,  1,376,  1,176,  1,177,  1,178,  1,179,  1,379,
  1,181,  1,378,  1,380,  1,382,  1,383,  1,384,  1,385,  1,386,
  1,387,  1,388,  1,389,  1,390,  1,391,  1,392,  1,393,  1,394,
  1,395,  1,396,  1,397,  1,398,  1,399,  1,400,  1,401,  1,403,
  1,404,  1,405,  1,406,  1,407,  1,408,  1,409,  1,410,  1,411,
  1,412,  1,413,  1,414,  1,415,  1,416,  1,417,  1,418,  1,419,
  1,420,  1,421,  1,422,  1,423,  1,424,  1,425,  1,426,  1,427,
  1,428,  1,429,  1,430,  1,431,  1,432,  1,433,  1,434,  1,435,
  1,436,  1,437,  1,438,  1,439,  1,440,  1,441,  1,442,  1,443,
  1,444,  1,445,  1,446,  1,447,  1,448,  1,449,  1,450,  1,451,
  1,452,  1,453,  1,454,  1,455,  1,456,  1,457,  1,458,  1,459,
  1,460,  1,461,  1,462,  1,463,  1,464,  1,465,  1,466,  1,467,
  1,468,  1,469,  1,470,  1,471,  1,472,0
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
    0,302,303,  0,300,301,  0,  0,295,  0,304,  0,  0,297,305,  0, 35,286,
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
    0,  0,  0,  0,  0, 42,  0,  0,302,303,  0,300,301,  0,  0,295,  0,304,
    0,  0,297,305,  0, 35,286,  0,351,340,  0,333,  0,338,353,  0,118,120,
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
    0,296,  0,336,  0, 52,169,361,331,  0,344,334,342,110, 96,102,100,104,
    0,  0, 94,  0,  0,106,108,  0,  0,  0,  0,  0, 42,  0,  0, 38,  0,302,
  303,  0,300,301,  0,  0,295,  0,304,  0,  0,297,305,  0, 35,286,  0,351,
  340,  0,333,  0,338,353,  0,118,120,122,  0,124, 70,126,  0, 12,128,  0,
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
  290, 68,  0,296,  0,336,  0, 52,169,361,331,  0,344,334,342,110, 96,102,
  100,104,  0,  0, 94,  0,  0,106,108,  0,  0,  0,  0,  0, 42,  0,  0, 80,
   92,  0, 84, 86,  0, 52,169, 35, 82,  0, 90,  0, 78, 88,  0, 35,  0, 35,
   98,  0, 52, 35,  0, 35,286,  0,  0,  0, 35,  4,  2,  0, 35,286,  0,118,
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
   46,  0,292,294,  0,290,  0,296,  0,286,  0,  0,  0,  0,170,  0,  0,  0,
    0,  0,  0,  0,  0,  0, 42,  0,  0,286,  0, 80, 92,  0, 84, 86,  0, 52,
  169, 35, 82,  0, 90,  0, 88,  0, 35,286,  0,118,120,122,  0,124,126,  0,
  116,  0,  0,128,  0,136,138,140,  0,142,144,  0,148,150,152,  0,130,132,
  134,  0,146,154,  0,156,158,  0,160,162,  0,  0,  0,164,166,  0,174,176,
    0,172,  0,186,  0,188,190,192,194,  0,198,200,  0,178,180,184,  0,196,
  182,202,  0,206,  0,208,210,  0,204,  0,  0,212,214,  0,  0,  0,  0,  0,
  216,  0,218,220,  0,224,226,  0,  0,228,  0,230,232,234,  0,236,238,  0,
  246,248,  0,252,254,  0,258,260,  0,  0,240,242,244,  0,250,  0,256,262,
  264,266,  0,268,270,  0,272,274,  0,  0,  0,282,  0,280,284,  0,286,288,
    0,  0,  0,276,278,  0,  0,  0,292,294,  0,290,  0,296,  0,  0,  0,  0,
    0,168,170,  0,  0,  0,  0,222,  0,  0,  0,  0,  0,  0, 35,286,  0,351,
  340,  0,333,  0,338,353,  0,118,120,122,  0,124, 70,126,  0,116,  0,  0,
  128,  0,136,138,140,  0,142,144,  0,148,150,152,  0,130,132,134,  0,146,
  154,  0,156,158,  0,160,162,  0,  0,  0,164,166,  0,168, 54,  0, 58, 62,
    0,174,176,  0,172,  0,186,  0,188,190,192,194,  0,198,200,  0,178,180,
  184,  0,196,182,202,  0,206,  0,208,210,  0,204,  0,  0,212,214,  0,  0,
    0,  0, 60,  0, 64,216,  0, 76,218,  0,  0,220,  0, 56,222,  0,224,226,
    0, 66,228,  0,230,232,234,  0,236,238,  0,246,248,  0,252,254,  0,258,
  260,  0,  0,240,242,244,  0,250,  0,256,262,264,266,  0,268,270,  0,272,
  274,  0, 72, 74,  0,282,  0,280,284,  0,286,288,  0,  0,  0,276,278,  0,
    0,  0,292,294,  0,290, 68,  0,296,  0,336,361,331,  0,344,334,342,  0,
    0,  0,  0,  0,170,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0, 35,286,
    0,118,120,122,  0,124,126,  0,116,  0,  0,128,  0,136,138,140,  0,142,
  144,  0,148,150,152,  0,130,132, 82,134,  0,146,154,  0,156,158,  0,160,
  162,  0,  0,  0,164,166,  0, 80, 92,  0,174,176,  0,172, 90,  0,186,  0,
  188,190,192,194,  0,198,200,  0,178,180,184,  0,196,182,202,  0,206,  0,
  208,210,  0,204,  0,  0,212,214,  0,  0,  0,  0,  0, 84, 86,216,  0,218,
  220,  0,224,226,  0,  0,228,  0,230,232,234,  0,236,238,  0,246,248,  0,
  252,254,  0,258,260,  0,  0,240,242,244,  0,250,  0,256,262,264,266,  0,
  268,270,  0,272,274,  0,  0,  0,282,  0,280,284,  0,286,288,  0,  0,  0,
  276,278, 88,  0,  0,  0,292,294,  0,290,  0,296,  0, 52,169,  0,  0,  0,
    0,168,  0,170,  0,  0,  0,  0,222,  0,  0,  0,  0,  0,  0, 35,110, 96,
  102,100,104, 94,106,108,  0, 35, 96,100,  0, 35,110, 96,100, 94,114,  0,
   35, 96,100, 94,112,  0, 80, 92,  0, 84, 86,  0, 52,169, 82,  0, 90,  0,
   78, 88,  0,110, 96,102,100,104, 94,106,108,  0, 96,100,  0,110, 96,100,
   94,114,  0, 96,100, 94,112,  0,118,120,122,  0,124,126,  0,116,  0,  0,
  128,  0,136,138,140,  0,142,144,  0,148,150,152,  0,130,132,134,  0,146,
  154,  0,156,158,  0,160,162,  0,  0,  0,164,166,  0,174,176,  0,172,  0,
  186,  0,188,190,192,194,  0,198,200,  0,178,180,184,  0,196,182,202,  0,
  206,  0,208,210,  0,204,  0,  0,212,214,  0,  0,  0,  0,  0,216,  0,218,
  220,  0,224,226,  0,  0,228,  0,230,232,234,  0,236,238,  0,246,248,  0,
  252,254,  0,258,260,  0,  0,240,242,244,  0,250,  0,256,262,264,266,  0,
  268,270,  0,272,274,  0,  0,  0,282,  0,280,284,  0,286,288,  0,  0,  0,
  276,278,  0,  0,  0,292,294,  0,290,  0,296,  0,286,  0,  0,  0,168,170,
    0,  0,  0,  0,222,  0,  0,  0,  0,  0,  0, 35,286,  0, 52,  0,  0, 52,
  286,  0, 35,286,  0,351,340,  0,333,  0,338,353,  0,336,361,331,  0,344,
  334,342,  0, 35,286,  0,331,  0,  0,169,  0,174,  0, 35,286,  0,351,340,
    0,333,  0,338,353,  0,118,120,122,  0,124, 70,126,  0,116,  0,  0,128,
    0,136,138,140,  0,142,144,  0,148,150,152,  0,130,132,134,  0,146,154,
    0,156,158,  0,160,162,  0,  0,  0,164,166,  0,168, 54,  0, 58, 62,  0,
  170,  0,174,176,  0,172,  0,186,  0,188,190,192,194,  0,198,200,  0,178,
  180,184,  0,196,182,202,  0,206,  0,208,210,  0,204,  0,  0,212,214,  0,
    0,  0,  0, 60,  0, 64,216,  0, 76,218,  0,  0,220,  0, 56,222,  0,224,
  226,  0, 66,228,  0,230,232,234,  0,236,238,  0,246,248,  0,252,254,  0,
  258,260,  0,  0,240,242,244,  0,250,  0,256,262,264,266,  0,268,270,  0,
  272,274,  0, 72, 74,  0,282,  0,280,284,  0,286,288,  0,  0,  0,276,278,
    0,  0,  0,292,294,  0,290, 68,  0,296,  0,336,361,331,  0,344,334,342,
    0, 96,  0,100,  0,  0, 94,  0,  0,  0,  0,  0,  0,  0, 98,  0,  0,  0,
    0, 94,  0, 35,286,  0,351,340,  0,333,  0,338,353,  0, 58, 62,  0,170,
    0,174,176,  0,172,  0,186,  0,188,190,192,194,  0,198,200,  0,178,180,
  184,  0,196,182,202,  0,206,  0,208,210,  0,204,  0,  0,212,214,  0,  0,
    0,  0, 60,  0, 64,216,  0, 76,218,  0,  0,220,  0, 56,222,  0,224,226,
    0, 66,228,  0,230,232,234,  0,236,238,  0,246,248,  0,252,254,  0,258,
  260,  0,  0,240,242,244,  0,250,  0,256,262,264,266,  0,268,270,  0,272,
  274,  0, 72, 74,  0,282,  0,280,284,  0,286,288,  0,  0,  0,276,278,  0,
    0,  0,292,294,  0,290, 68,  0,296,  0,336,361,331,  0,344,334,342, 96,
  100,  0, 94,  0,  0,  0,  0,  0,  0,  0, 98,  0,  0,  0,  0, 80, 92,  0,
   84, 86,  0, 52,169, 82,  0, 90,  0, 88,  0,351,340,  0,333,  0,338,353,
    0,118,120,122,  0,124, 70,126,  0,116,  0,  0,128,  0,136,138,140,  0,
  142,144,  0,148,150,152,  0,130,132,134,  0,146,154,  0,156,158,  0,160,
  162,  0,  0,  0,164,166,  0,168, 54,  0, 58, 62,  0,174,176,  0,172,  0,
  186,  0,188,190,192,194,  0,198,200,  0,178,180,184,  0,196,182,202,  0,
  206,  0,208,210,  0,204,  0,  0,212,214,  0,  0,  0,  0, 60,  0, 64,216,
    0, 76,218,  0,  0,220,  0, 56,222,  0,224,226,  0, 66,228,  0,230,232,
  234,  0,236,238,  0,246,248,  0,252,254,  0,258,260,  0,  0,240,242,244,
    0,250,  0,256,262,264,266,  0,268,270,  0,272,274,  0, 72, 74,  0,282,
    0,280,284,  0,286,288,  0,  0,  0,276,278,  0,  0,  0,292,294,  0,290,
   68,  0,296,  0,336,361,331,286,344,334,342,  0,  0,  0,  0,  0,170,  0,
    0,  0,  0,  0,  0,  0,  0,  0,  0,  0,351,340,  0,333,  0,338,353,  0,
  118,120,122,  0,124, 70,126,  0,116,  0,  0,128,  0,136,138,140,  0,142,
  144,  0,148,150,152,  0,130,132,134,  0,146,154,  0,156,158,  0,160,162,
    0,  0,  0,164,166,  0,168, 54,  0, 58, 62,  0,174,176,  0,172,  0,186,
    0,188,190,192,194,  0,198,200,  0,178,180,184,  0,196,182,202,  0,206,
    0,208,210,  0,204,  0,  0,212,214,  0,  0,  0,  0, 60,  0, 64,216,  0,
  218,220,  0, 56,222,  0,224,226,  0, 66,228,  0,230,232,234,  0,236,238,
    0,246,248,  0,252,254,  0,258,260,  0,  0,240,242,244,  0,250,  0,256,
  262,264,266,  0,268,270,  0,272,274,  0, 72, 74,  0,282,  0,280,284,  0,
  286,288,  0,  0,  0,276,278,  0,  0,  0,292,294,  0,290, 68,  0,296,  0,
  336,331,286,344,334,342,  0,  0,  0,  0,  0,170,  0,  0,  0,  0,  0,  0,
    0,  0,  0,  0,  0,340,  0,333,  0,338,  0,118,120,122,  0,124, 70,126,
    0,116,  0,  0,128,  0,136,138,140,  0,142,144,  0,148,150,152,  0,130,
  132,134,  0,146,154,  0,156,158,  0,160,162,  0,  0,  0,164,166,  0,168,
   54,  0, 58, 62,  0,174,176,  0,172,  0,186,  0,188,190,192,194,  0,198,
  200,  0,178,180,184,  0,196,182,202,  0,206,  0,208,210,  0,204,  0,  0,
  212,214,  0,  0,  0,  0, 60,  0, 64,216,  0,218,220,  0, 56,222,  0,224,
  226,  0, 66,228,  0,230,232,234,  0,236,238,  0,246,248,  0,252,254,  0,
  258,260,  0,  0,240,242,244,  0,250,  0,256,262,264,266,  0,268,270,  0,
  272,274,  0,  0,  0,282,  0,280,284,  0,286,288,  0,  0,  0,276,278,  0,
    0,  0,292,294,  0,290, 68,  0,296,  0,336,331,286,344,334,342,  0,  0,
    0,  0,  0,170,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,340,  0,333,
    0,338,  0,118,120,122,  0,124,126,  0,116,  0,  0,128,  0,136,138,140,
    0,142,144,  0,148,150,152,  0,130,132,134,  0,146,154,  0,156,158,  0,
  160,162,  0,  0,  0,164,166,  0,168, 54,  0, 58, 62,  0,174,176,  0,172,
    0,186,  0,188,190,192,194,  0,198,200,  0,178,180,184,  0,196,182,202,
    0,206,  0,208,210,  0,204,  0,  0,212,214,  0,  0,  0,  0, 60,  0, 64,
  216,  0,218,220,  0, 56,222,  0,224,226,  0, 66,228,  0,230,232,234,  0,
  236,238,  0,246,248,  0,252,254,  0,258,260,  0,  0,240,242,244,  0,250,
    0,256,262,264,266,  0,268,270,  0,272,274,  0,  0,  0,282,  0,280,284,
    0,286,288,  0,  0,  0,276,278,  0,  0,  0,292,294,  0,290, 68,  0,296,
    0,336,331,286,344,334,342,  0,  0,  0,  0,  0,170,  0,  0,  0,  0,  0,
    0,  0,  0,  0,  0,  0,340,  0,333,  0,338,  0,118,120,122,  0,124,126,
    0,116,  0,  0,128,  0,136,138,140,  0,142,144,  0,148,150,152,  0,130,
  132,134,  0,146,154,  0,156,158,  0,160,162,  0,  0,  0,164,166,  0,168,
   54,  0, 58, 62,  0,174,176,  0,172,  0,186,  0,188,190,192,194,  0,198,
  200,  0,178,180,184,  0,196,182,202,  0,206,  0,208,210,  0,204,  0,  0,
  212,214,  0,  0,  0,  0, 60,  0, 64,216,  0,218,220,  0, 56,222,  0,224,
  226,  0, 66,228,  0,230,232,234,  0,236,238,  0,246,248,  0,252,254,  0,
  258,260,  0,  0,240,242,244,  0,250,  0,256,262,264,266,  0,268,270,  0,
  272,274,  0,  0,  0,282,  0,280,284,  0,286,288,  0,  0,  0,276,278,  0,
    0,  0,292,294,  0,290,  0,296,  0,336,331,286,344,334,342,  0,  0,  0,
    0,  0,170,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,340,  0,333,  0,
  338,  0, 58, 62,  0, 60, 64,  0,336,286,344,334,342, 54,  0,  0, 56,  0,
  331,286,  0, 35,286,  0,118,120,122,  0,124,126,  0,116,  0,  0,128,  0,
  136,138,140,  0,142,144,  0,148,150,152,  0,130,132,134,  0,146,154,  0,
  156,158,  0,160,162,  0,  0,  0,164,166,  0,174,176,  0,172,  0,186,  0,
  188,190,192,194,  0,198,200,  0,178,180,184,  0,196,182,202,  0,206,  0,
  208,210,  0,204,  0,  0,212,214,  0,  0,  0,  0,  0,216,  0,218,220,  0,
  224,226,  0,  0,228,  0,230,232,234,  0,236,238,  0,246,248,  0,252,254,
    0,258,260,  0,  0,240,242,244,  0,250,  0,256,262,264,266,  0,268,270,
    0,272,274,  0,  0,  0,282,  0,280,284,  0,286,288,  0,  0,  0,276,278,
    0,  0,  0,292,294,  0,290,  0,296,  0,331,  0,  0,  0,  0,168,170,  0,
    0,  0,  0,222,  0,  0,  0,  0,  0,  0,331,  0, 35,286,  0,  0, 96,100,
   94, 98,  0, 82,  0, 80, 92,  0, 84, 86,  0, 52,169, 35,110, 96,102,100,
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
  204,  0,471,204,501,743,  0,204,204,204,779,789,789,  0,  0,  0,791,794,
  794,800,800,794,794,802,789,789,995,1197,1218,1222,1222,1222,1222,1224,
    0,  0,1413,1431,789,1439,779,789,779,789,789,789,789,789,789,789,789,
  789,789,1599,1599,1439,789,779,779,1439,1792,779,789,779,1439,789,789,
  1975,789,  0,  0,  0,  0,1599,779,1995,1599,779,1995,1599,2005,779,1599,
  1599,1599,779,779,1995,1599,789,1599,1599,1599,1599,1599,1599,1599,1599,
  1599,1599,1599,1599,1599,1599,2009,2009,1599,779,779,1995,1599,1995,1995,
  2016,1599,779,779,779,2005,779,779,779,779,779,779,779,779,779,779,779,
  779,779,779,2016,1995,779,1599,1599,1599,1599,2016,1995,2016,1599,779,
  779,779,779,779,779,779,1995,1599,1599,779,779,779,1599,779,1995,779,1995,
  1995,779,2028,2037,2028,2037,2046,2028,2028,2028,2037,  0,2049,2049,2028,
  2028,2037,2037,2037,2055,2028,2028,2028,2046,2028,2028,2028,2028,2028,
  2028,2028,2028,2028,2028,2028,2028,2028,2028,2055,2037,2028,2055,2037,
  2055,2028,2028,2028,2028,2028,2028,2028,2037,2028,2028,2028,2028,2037,
  2028,2037,2037,2028,2208,794,1222,789,  0,800,800,1431,789,  0,800,800,
  779,2028,2228,2231,779,2028,794,1222,794,1222,794,1222,794,1222,779,2028,
  779,2028,779,2028,800,800,800,779,2028,2028,2028,2028,779,2028,779,2028,
  2245,800,800,  0,800,  0,2256,789,2028,2028,2028,2037,2028,2037,2046,2028,
  2028,2028,2037,  0,2049,2049,2028,2028,2037,2037,2037,2055,2028,2028,2028,
  2046,2028,2028,2028,2028,2028,2028,2028,2028,2028,2028,2028,2028,2028,
  2028,2055,2037,2028,2055,2037,2055,2028,2028,2028,2028,2028,2028,2028,
  2037,2028,2028,2028,2028,2037,2028,2037,2037,2028,2208,2259,2261,2261,
  2256,  0,2441,  0,  0,2261,1792,2467,1792,2602,789,789,789,789,789,789,
  789,1792,1792,  0,  0,  0,  0,  0,  0,  0,1439,2028,2631,2631,2631,2812,
  3006,3006,3196,3385,3573,3608,2028,1431,3618,3618,3618,2261,3618,3618,
  3618,3618,  0,  0,3772,1599,1599,1599,1599,1599,3790,3790,1599,  0,3795,
    0,2261,  0,2261,2028,2028,2028,2028,2028,2028,2028,  0,1439,2631,1439,
  2631,2631,2631,2631,2028,2028,779,2028,779,2028,779,2028,779,2028,779,
  2028,2028,779,2028,2028,779,2028,2028,779,779,779,779,779,779,779,779,
  779,779,779,779,779,3790,2261,3618,3618,3618,3618,3790,3790,  0,  0,  0,
    0,  0,  0,  0,  0,  0,2812,2631,2631,2631,2631,2631,2028,2028,2028,2028,
  2028,2028,2028,2028,2028,2028,2028,2028,3809,1222,2261,2028,2028,2037,
  2028,2812,2812,3006,3006,3006,3006,3006,3006,3196,3196,3385,3385,1222,
  2028,2028,2037,2028
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
   26, 26,473,473,473,473,473,473,473,473,  1,289,473,473,473,473,473,473,
  473,473,473,473,473,473,473,473,473,473,473,473,473,473,473,  1,362,474,
  473,475,359,349,476,366,365,357,355,288,356,477,358,478,479,479,479,479,
  479,479,479,480,480,473,287,481,473,482,473,473,483,483,483,483,483,483,
  484,484,484,484,484,484,484,485,484,484,484,486,484,487,484,484,484,488,
  484,484,473,489,473,347,490,473,483,483,483,483,483,483,484,484,484,484,
  484,484,484,485,484,484,484,486,484,487,484,484,484,488,484,484,473,345,
  473,364,491,491,491,491,491,491,491,491,491,491,491,491,491,491,491,491,
  491,491,491,491,491,491,491,491,491,491,491,491,491,491,491,491,491,491,
  491,491,491,491,491,491,491,491,491,491,491,491,491,491,491,491,491,491,
  491,491,491,491,491,491,491,491,491,491,491,491,491,491,491,491,491,491,
  491,491,491,491,491,491,491,491,491,491,491,491,491,491,491,491,491,491,
  491,491,491,491,491,491,491,491,491,491,491,491,491,491,491,491,491,491,
  491,491,491,491,491,491,491,491,491,491,491,491,491,491,491,491,491,491,
  491,491,491,491,491
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
490,488,487,486,485,484,483,475,472,471,470,469,468,467,466,465,464,463,462,
  461,460,459,458,457,456,455,454,453,452,451,450,449,448,447,446,445,444,
  443,442,441,440,439,438,437,436,435,434,433,432,431,430,429,428,427,426,
  425,424,423,422,421,420,419,418,417,416,415,414,413,412,411,410,409,408,
  407,406,405,404,403,401,400,399,398,397,396,395,394,393,392,391,390,389,
  388,387,386,385,384,383,382,380,330,329,328,327,325,323,322,321,320,319,
  318,317,316,315,314,313,312,311,310,309,308,305,304,303,302,301,300,297,
  295,292,289,288,287,286,86,35,1,0,2,283,284,285,
491,490,489,488,487,486,485,484,483,482,481,480,479,478,477,476,475,474,473,
  366,365,364,362,359,358,357,356,355,349,347,345,289,288,287,1,0,37,
35,1,0,2,283,
490,488,487,486,485,484,483,475,472,471,470,469,468,467,466,465,464,463,462,
  461,460,459,458,457,456,455,454,453,452,451,450,449,448,447,446,445,444,
  443,442,441,440,439,438,437,436,435,434,433,432,431,430,429,428,427,426,
  425,424,423,422,421,420,419,418,417,416,415,414,413,412,411,410,409,408,
  407,406,405,404,403,401,400,399,398,397,396,395,394,393,392,391,390,389,
  388,387,386,385,384,383,382,380,330,329,328,327,325,323,322,321,320,319,
  318,317,316,315,314,313,312,311,310,309,308,305,304,303,302,301,300,297,
  295,292,289,288,287,286,86,43,35,1,0,22,23,24,25,39,40,
491,490,489,488,487,486,485,484,483,482,481,480,479,478,477,476,475,474,473,
  366,365,364,362,359,358,357,356,355,349,347,345,289,288,287,38,1,0,
1,0,
289,0,42,
490,488,487,486,485,484,483,475,472,471,470,469,468,467,466,465,464,463,462,
  461,460,459,458,457,456,455,454,453,452,451,450,449,448,447,446,445,444,
  443,442,441,440,439,438,437,436,435,434,433,432,431,430,429,428,427,426,
  425,424,423,422,421,420,419,418,417,416,415,414,413,412,411,410,409,408,
  407,406,405,404,403,401,400,399,398,397,396,395,394,393,392,391,390,389,
  388,387,386,385,384,383,382,380,330,329,328,327,325,323,322,321,320,319,
  318,317,316,315,314,313,312,311,310,309,308,305,304,303,302,301,300,297,
  295,292,289,288,287,286,86,35,0,2,4,12,27,28,32,33,34,41,44,45,46,49,50,
  51,52,53,54,58,85,290,299,
490,488,487,486,485,484,483,475,472,471,470,469,468,467,466,465,464,463,462,
  461,460,459,458,457,456,455,454,453,452,451,450,449,448,447,446,445,444,
  443,442,441,440,439,438,437,436,435,434,433,432,431,430,429,428,427,426,
  425,424,423,422,421,420,419,418,417,416,415,414,413,412,411,410,409,408,
  407,406,405,404,403,401,400,399,398,397,396,395,394,393,392,391,390,389,
  388,387,386,385,384,383,382,380,330,329,328,327,325,323,322,321,320,319,
  318,317,316,315,314,313,312,311,310,309,308,305,304,303,302,301,300,297,
  295,292,289,288,287,286,86,43,35,26,1,0,23,24,39,40,
490,488,487,486,485,484,483,475,472,471,470,469,468,467,466,465,464,463,462,
  461,460,459,458,457,456,455,454,453,452,451,450,449,448,447,446,445,444,
  443,442,441,440,439,438,437,436,435,434,433,432,431,430,429,428,427,426,
  425,424,423,422,421,420,419,418,417,416,415,414,413,412,411,410,409,408,
  407,406,405,404,403,401,400,399,398,397,396,395,394,393,392,391,390,389,
  388,387,386,385,384,383,382,380,330,329,328,327,325,323,322,321,320,319,
  318,317,316,315,314,313,312,311,310,309,308,305,304,303,302,301,300,297,
  295,292,289,288,287,286,86,35,26,1,0,2,283,284,285,
491,490,489,488,487,486,485,484,483,482,481,480,479,478,477,476,475,474,473,
  373,372,371,370,369,368,367,366,365,364,363,362,359,358,357,356,355,349,
  347,345,289,288,287,169,86,35,1,0,2,283,284,285,
491,490,489,488,487,486,485,484,483,482,481,480,479,478,477,476,475,474,473,
  366,365,364,362,359,358,357,356,355,349,347,345,289,288,287,35,1,0,2,
  283,284,285,
491,490,489,488,487,486,485,484,483,482,481,480,479,478,477,476,475,474,473,
  366,365,364,362,359,358,357,356,355,349,347,345,289,288,287,35,1,0,2,
  283,284,285,
491,490,489,488,487,486,485,484,483,482,481,480,479,478,477,476,475,474,473,
  366,365,364,362,359,358,357,356,355,349,347,345,289,288,287,1,0,30,31,
  33,34,90,91,93,95,96,121,124,127,135,136,138,140,141,144,146,149,150,
  163,164,165,166,168,171,172,173,175,278,279,280,281,282,
491,490,489,488,487,486,485,484,483,482,481,480,479,478,477,476,475,474,473,
  366,365,364,362,359,358,357,356,355,349,347,345,289,288,287,1,0,30,31,
  33,34,90,91,93,95,96,121,124,127,135,136,138,140,141,144,146,149,150,
  163,164,165,166,168,171,172,173,175,278,279,280,281,282,
491,490,489,488,487,486,485,484,483,482,481,480,479,478,477,476,475,474,473,
  366,365,364,362,359,358,357,356,355,349,347,345,289,288,287,1,0,30,31,
  33,34,90,91,93,95,96,121,124,127,135,136,138,140,141,144,146,149,150,
  163,164,165,166,168,171,172,173,175,278,279,280,281,282,
490,488,487,486,485,484,483,480,479,478,475,98,35,1,0,2,88,283,284,285,
490,488,487,486,485,484,483,475,86,35,1,0,2,283,284,285,
490,488,487,486,485,484,483,475,86,35,1,0,2,283,284,285,
289,288,287,286,35,1,0,2,283,284,285,
289,288,287,286,35,1,0,2,283,284,285,
490,488,487,486,485,484,483,475,86,35,1,0,2,283,284,285,
490,488,487,486,485,484,483,475,86,35,1,0,2,283,284,285,
298,296,35,1,0,2,283,284,285,
481,474,35,1,0,2,283,284,285,
490,489,488,487,486,485,484,483,481,480,479,478,477,474,358,35,1,0,2,283,
  284,285,
472,471,470,469,468,467,466,465,464,463,462,461,460,459,458,457,456,455,454,
  453,452,451,450,449,448,447,446,445,444,443,442,441,440,439,438,437,436,
  435,434,433,432,431,430,429,428,427,426,425,424,423,422,421,420,419,418,
  417,416,415,414,413,412,411,410,409,408,407,406,405,404,403,401,400,399,
  398,397,396,395,394,393,392,391,390,389,388,387,386,385,384,383,382,380,
  330,329,328,327,325,323,322,321,320,319,318,317,316,315,314,313,312,311,
  310,309,308,307,306,289,288,287,286,35,1,0,2,283,284,285,
472,471,470,469,468,467,466,465,464,463,462,461,460,459,458,457,456,455,454,
  453,452,451,450,449,448,447,446,445,444,443,442,441,440,439,438,437,436,
  435,434,433,432,431,430,429,428,427,426,425,424,423,422,421,420,419,418,
  417,416,415,414,413,412,411,410,409,408,407,406,405,404,403,401,400,399,
  398,397,396,395,394,393,392,391,390,389,388,387,386,385,384,383,382,380,
  330,329,328,327,325,323,322,321,320,319,318,317,316,315,314,313,312,311,
  310,309,308,0,59,60,61,62,63,64,66,67,69,70,71,72,73,74,76,77,78,79,80,
  81,82,83,185,186,187,188,189,190,191,192,193,194,195,196,197,198,199,
  200,201,202,203,204,205,206,207,208,209,210,211,212,213,214,215,216,217,
  218,219,220,221,222,223,224,225,226,227,228,229,230,231,232,233,234,235,
  236,237,238,239,240,241,242,243,244,245,246,247,248,249,250,251,252,253,
  254,255,256,257,258,259,260,262,263,264,265,266,267,268,269,270,271,272,
  273,274,275,276,277,
314,313,312,307,306,0,55,57,63,64,66,
490,488,487,486,485,484,483,475,86,0,4,85,299,
490,488,487,486,485,484,483,475,86,0,4,85,299,
490,488,487,486,485,484,483,475,86,0,4,85,299,
490,488,487,486,485,484,483,475,86,0,4,85,299,
298,296,0,47,48,
481,474,0,11,13,14,15,291,293,
490,489,488,487,486,485,484,483,481,480,479,478,477,474,358,0,11,13,14,15,
  20,291,293,294,
472,471,470,469,468,467,466,465,464,463,462,461,460,459,458,457,456,455,454,
  453,452,451,450,449,448,447,446,445,444,443,442,441,440,439,438,437,436,
  435,434,433,432,431,430,429,428,427,426,425,424,423,422,421,420,419,418,
  417,416,415,414,413,412,411,410,409,408,407,406,405,404,403,401,400,399,
  398,397,396,395,394,393,392,391,390,389,388,387,386,385,384,383,382,380,
  330,329,328,327,325,323,322,321,320,319,318,317,316,315,314,313,312,311,
  310,309,308,307,306,289,288,287,286,0,55,57,
289,288,287,286,0,27,28,33,34,42,
491,490,489,488,487,486,485,484,483,482,481,480,479,478,477,476,475,474,473,
  366,365,364,362,359,358,357,356,355,349,347,345,289,288,287,35,1,0,2,
  283,284,285,
491,490,489,488,487,486,485,484,483,482,481,480,479,478,477,476,475,474,473,
  373,372,371,370,369,368,367,366,365,364,362,359,358,357,356,355,349,347,
  345,289,288,287,169,86,35,1,0,2,283,284,285,
491,490,489,488,487,486,485,484,483,482,481,480,479,478,477,476,475,474,473,
  373,372,371,370,369,368,367,366,365,364,363,362,359,358,357,356,355,349,
  347,345,289,288,287,169,86,35,1,0,2,283,284,285,
491,490,489,488,487,486,485,484,483,482,481,480,479,478,477,476,475,474,473,
  366,365,364,362,359,358,357,356,355,349,347,345,289,288,287,35,1,0,2,
  283,284,285,
491,490,489,488,487,486,485,484,483,482,481,480,479,478,477,476,475,474,473,
  373,372,371,370,369,368,367,366,365,364,363,362,359,358,357,356,355,349,
  347,345,289,288,287,169,86,35,1,0,2,283,284,285,
491,490,489,488,487,486,485,484,483,482,481,480,479,478,477,476,475,474,473,
  366,365,364,362,359,358,357,356,355,349,347,345,289,288,287,35,1,0,2,
  283,284,285,
491,490,489,488,487,486,485,484,483,482,481,480,479,478,477,476,475,474,473,
  366,365,364,362,359,358,357,356,355,349,347,345,289,288,287,35,1,0,2,
  283,284,285,
491,490,489,488,487,486,485,484,483,482,481,480,479,478,477,476,475,474,473,
  366,365,364,362,359,358,357,356,355,349,347,345,289,288,287,35,1,0,2,
  283,284,285,
491,490,489,488,487,486,485,484,483,482,481,480,479,478,477,476,475,474,473,
  366,365,364,362,359,358,357,356,355,349,347,345,289,288,287,35,1,0,2,
  283,284,285,
491,490,489,488,487,486,485,484,483,482,481,480,479,478,477,476,475,474,473,
  366,365,364,362,359,358,357,356,355,349,347,345,289,288,287,35,1,0,2,
  283,284,285,
491,490,489,488,487,486,485,484,483,482,481,480,479,478,477,476,475,474,473,
  366,365,364,362,359,358,357,356,355,349,347,345,289,288,287,35,1,0,2,
  283,284,285,
491,490,489,488,487,486,485,484,483,482,481,480,479,478,477,476,475,474,473,
  366,365,364,362,359,358,357,356,355,349,347,345,289,288,287,35,1,0,2,
  283,284,285,
491,490,489,488,487,486,485,484,483,482,481,480,479,478,477,476,475,474,473,
  366,365,364,362,359,358,357,356,355,349,347,345,289,288,287,35,1,0,2,
  283,284,285,
491,490,489,488,487,486,485,484,483,482,481,480,479,478,477,476,475,474,473,
  366,365,364,362,359,358,357,356,355,349,347,345,289,288,287,35,1,0,2,
  283,284,285,
491,490,489,488,487,486,485,484,483,482,481,480,479,478,477,476,475,474,473,
  366,365,364,362,359,358,357,356,355,349,347,345,289,288,287,35,1,0,2,
  283,284,285,
491,490,489,488,487,486,485,484,483,482,481,480,479,478,477,476,475,474,473,
  472,471,470,469,468,467,466,465,464,463,462,461,460,459,458,457,456,455,
  454,453,452,451,450,449,448,447,446,445,444,443,442,441,440,439,438,437,
  436,435,434,433,432,431,430,429,428,427,426,425,424,423,422,421,420,419,
  418,417,416,415,414,413,412,411,410,409,408,407,406,405,404,403,401,400,
  399,398,397,396,395,394,393,392,391,390,389,388,387,386,385,384,383,382,
  380,366,365,364,362,359,358,357,356,355,349,347,345,289,288,287,286,35,
  1,0,2,283,284,285,
491,490,489,488,487,486,485,484,483,482,481,480,479,478,477,476,475,474,473,
  472,471,470,469,468,467,466,465,464,463,462,461,460,459,458,457,456,455,
  454,453,452,451,450,449,448,447,446,445,444,443,442,441,440,439,438,437,
  436,435,434,433,432,431,430,429,428,427,426,425,424,423,422,421,420,419,
  418,417,416,415,414,413,412,411,410,409,408,407,406,405,404,403,401,400,
  399,398,397,396,395,394,393,392,391,390,389,388,387,386,385,384,383,382,
  380,366,365,364,362,359,358,357,356,355,349,347,345,289,288,287,286,35,
  1,0,2,283,284,285,
491,490,489,488,487,486,485,484,483,482,481,480,479,478,477,476,475,474,473,
  373,372,371,370,369,368,367,366,365,364,362,359,358,357,356,355,349,347,
  345,289,288,287,169,86,35,1,0,2,283,284,285,
491,490,489,488,487,486,485,484,483,482,481,480,479,478,477,476,475,474,473,
  366,365,364,362,359,358,357,356,355,349,347,345,289,288,287,35,1,0,2,
  283,284,285,
491,490,489,488,487,486,485,484,483,482,481,480,479,478,477,476,475,474,473,
  373,372,371,370,369,368,367,366,365,364,363,362,359,358,357,356,355,349,
  347,345,289,288,287,169,86,35,1,0,2,283,284,285,
491,490,489,488,487,486,485,484,483,482,481,480,479,478,477,476,475,474,473,
  373,372,371,370,369,368,367,366,365,364,363,362,359,358,357,356,355,349,
  347,345,289,288,287,169,86,35,1,0,2,283,284,285,
491,490,489,488,487,486,485,484,483,482,481,480,479,478,477,476,475,474,473,
  373,372,371,370,369,368,367,366,365,364,362,359,358,357,356,355,349,347,
  345,289,288,287,169,86,35,1,0,2,283,284,285,
491,490,489,488,487,486,485,484,483,482,481,480,479,478,477,476,475,474,473,
  472,471,470,469,468,467,466,465,464,463,462,461,460,459,458,457,456,455,
  454,453,452,451,450,449,448,447,446,445,444,443,442,441,440,439,438,437,
  436,435,434,433,432,431,430,429,428,427,426,425,424,423,422,421,420,419,
  418,417,416,415,414,413,412,411,410,409,408,407,406,405,404,403,401,400,
  399,398,397,396,395,394,393,392,391,390,389,388,387,386,385,384,383,382,
  380,366,365,364,362,361,360,359,358,357,356,355,354,353,352,351,350,349,
  348,347,346,345,344,343,342,341,340,339,338,337,336,335,334,333,332,331,
  289,288,287,286,35,1,0,2,283,284,285,
491,490,489,488,487,486,485,484,483,482,481,480,479,478,477,476,475,474,473,
  373,372,371,370,369,368,367,366,365,364,363,362,359,358,357,356,355,349,
  347,345,289,288,287,169,86,35,1,0,2,283,284,285,
491,490,489,488,487,486,485,484,483,482,481,480,479,478,477,476,475,474,473,
  366,365,364,362,359,358,357,356,355,349,347,345,289,288,287,35,1,0,2,
  283,284,285,
491,490,489,488,487,486,485,484,483,482,481,480,479,478,477,476,475,474,473,
  373,372,371,370,369,368,367,366,365,364,363,362,359,358,357,356,355,349,
  347,345,289,288,287,169,86,35,1,0,2,283,284,285,
491,490,489,488,487,486,485,484,483,482,481,480,479,478,477,476,475,474,473,
  373,372,371,370,369,368,367,366,365,364,362,359,358,357,356,355,349,347,
  345,289,288,287,169,86,35,1,0,2,283,284,285,
491,490,489,488,487,486,485,484,483,482,481,480,479,478,477,476,475,474,473,
  366,365,364,362,359,358,357,356,355,349,347,345,289,288,287,35,1,0,2,
  283,284,285,
491,490,489,488,487,486,485,484,483,482,481,480,479,478,477,476,475,474,473,
  366,365,364,362,359,358,357,356,355,349,347,345,289,288,287,35,1,0,2,
  283,284,285,
491,490,489,488,487,486,485,484,483,482,481,480,479,478,477,476,475,474,473,
  472,471,470,469,468,467,466,465,464,463,462,461,460,459,458,457,456,455,
  454,453,452,451,450,449,448,447,446,445,444,443,442,441,440,439,438,437,
  436,435,434,433,432,431,430,429,428,427,426,425,424,423,422,421,420,419,
  418,417,416,415,414,413,412,411,410,409,408,407,406,405,404,403,401,400,
  399,398,397,396,395,394,393,392,391,390,389,388,387,386,385,384,383,382,
  380,373,372,371,370,369,368,367,366,365,364,362,359,358,357,356,355,349,
  347,345,289,288,287,286,169,86,35,1,0,2,283,284,285,
491,490,489,488,487,486,485,484,483,482,481,480,479,478,477,476,475,474,473,
  366,365,364,362,359,358,357,356,355,349,347,345,289,288,287,35,1,0,2,
  283,284,285,
491,490,489,488,487,486,485,484,483,482,481,480,479,478,477,476,475,474,473,
  366,365,364,362,359,358,357,356,355,349,347,345,288,287,1,0,33,34,90,91,
  93,95,96,121,124,127,135,136,138,140,141,144,146,149,150,163,164,165,
  166,168,171,172,173,175,278,279,280,281,282,
289,0,32,
289,0,32,
289,0,32,
472,471,470,469,468,467,466,465,464,463,462,461,460,459,458,457,456,455,454,
  453,452,451,450,449,448,447,446,445,444,443,442,441,440,439,438,437,436,
  435,434,433,432,431,430,429,428,427,426,425,424,423,422,421,420,419,418,
  417,416,415,414,413,412,411,410,409,408,407,406,405,404,403,401,400,399,
  398,397,396,395,394,393,392,391,390,389,388,387,386,385,384,383,382,380,
  362,289,288,287,286,35,1,0,2,283,284,285,
490,488,487,486,485,484,483,480,479,478,477,476,475,373,372,371,370,369,368,
  367,366,364,363,362,356,355,169,86,35,1,0,2,283,284,285,
379,377,376,375,179,178,177,176,35,1,0,2,283,284,285,
472,471,470,469,468,467,466,465,464,463,462,461,460,459,458,457,456,455,454,
  453,452,451,450,449,448,447,446,445,444,443,442,441,440,439,438,437,436,
  435,434,433,432,431,430,429,428,427,426,425,424,423,422,421,420,419,418,
  417,416,415,414,413,412,411,410,409,408,407,406,405,404,403,401,400,399,
  398,397,396,395,394,393,392,391,390,389,388,387,386,385,384,383,382,380,
  362,289,288,287,286,35,1,0,2,283,284,285,
490,488,487,486,485,484,483,480,479,478,477,476,475,373,372,371,370,369,368,
  367,366,364,363,362,356,355,169,86,35,1,0,2,283,284,285,
379,377,376,375,179,178,177,176,35,1,0,2,283,284,285,
472,471,470,469,468,467,466,465,464,463,462,461,460,459,458,457,456,455,454,
  453,452,451,450,449,448,447,446,445,444,443,442,441,440,439,438,437,436,
  435,434,433,432,431,430,429,428,427,426,425,424,423,422,421,420,419,418,
  417,416,415,414,413,412,411,410,409,408,407,406,405,404,403,401,400,399,
  398,397,396,395,394,393,392,391,390,389,388,387,386,385,384,383,382,380,
  362,289,288,287,286,35,1,0,2,283,284,285,
376,375,35,1,0,2,283,284,285,
490,488,487,486,485,484,483,480,479,478,477,476,475,373,372,371,370,369,368,
  367,366,364,363,362,356,355,169,86,35,1,0,2,283,284,285,
472,471,470,469,468,467,466,465,464,463,462,461,460,459,458,457,456,455,454,
  453,452,451,450,449,448,447,446,445,444,443,442,441,440,439,438,437,436,
  435,434,433,432,431,430,429,428,427,426,425,424,423,422,421,420,419,418,
  417,416,415,414,413,412,411,410,409,408,407,406,405,404,403,401,400,399,
  398,397,396,395,394,393,392,391,390,389,388,387,386,385,384,383,382,380,
  362,289,288,287,286,35,1,0,2,283,284,285,
472,471,470,469,468,467,466,465,464,463,462,461,460,459,458,457,456,455,454,
  453,452,451,450,449,448,447,446,445,444,443,442,441,440,439,438,437,436,
  435,434,433,432,431,430,429,428,427,426,425,424,423,422,421,420,419,418,
  417,416,415,414,413,412,411,410,409,408,407,406,405,404,403,401,400,399,
  398,397,396,395,394,393,392,391,390,389,388,387,386,385,384,383,382,380,
  362,289,288,287,286,35,1,0,2,283,284,285,
472,471,470,469,468,467,466,465,464,463,462,461,460,459,458,457,456,455,454,
  453,452,451,450,449,448,447,446,445,444,443,442,441,440,439,438,437,436,
  435,434,433,432,431,430,429,428,427,426,425,424,423,422,421,420,419,418,
  417,416,415,414,413,412,411,410,409,408,407,406,405,404,403,401,400,399,
  398,397,396,395,394,393,392,391,390,389,388,387,386,385,384,383,382,380,
  362,289,288,287,286,35,1,0,2,283,284,285,
490,488,487,486,485,484,483,480,479,478,477,476,475,373,372,371,370,369,368,
  367,366,364,363,362,356,355,169,86,35,1,0,2,283,284,285,
490,488,487,486,485,484,483,480,479,478,477,476,475,373,372,371,370,369,368,
  367,366,364,363,362,356,355,169,86,35,1,0,2,283,284,285,
379,377,376,375,179,178,177,176,35,1,0,2,283,284,285,
472,471,470,469,468,467,466,465,464,463,462,461,460,459,458,457,456,455,454,
  453,452,451,450,449,448,447,446,445,444,443,442,441,440,439,438,437,436,
  435,434,433,432,431,430,429,428,427,426,425,424,423,422,421,420,419,418,
  417,416,415,414,413,412,411,410,409,408,407,406,405,404,403,401,400,399,
  398,397,396,395,394,393,392,391,390,389,388,387,386,385,384,383,382,380,
  362,289,288,287,286,35,1,0,2,283,284,285,
479,478,35,1,0,2,283,284,285,
472,471,470,469,468,467,466,465,464,463,462,461,460,459,458,457,456,455,454,
  453,452,451,450,449,448,447,446,445,444,443,442,441,440,439,438,437,436,
  435,434,433,432,431,430,429,428,427,426,425,424,423,422,421,420,419,418,
  417,416,415,414,413,412,411,410,409,408,407,406,405,404,403,401,400,399,
  398,397,396,395,394,393,392,391,390,389,388,387,386,385,384,383,382,380,
  362,289,288,287,286,35,1,0,2,283,284,285,
472,471,470,469,468,467,466,465,464,463,462,461,460,459,458,457,456,455,454,
  453,452,451,450,449,448,447,446,445,444,443,442,441,440,439,438,437,436,
  435,434,433,432,431,430,429,428,427,426,425,424,423,422,421,420,419,418,
  417,416,415,414,413,412,411,410,409,408,407,406,405,404,403,401,400,399,
  398,397,396,395,394,393,392,391,390,389,388,387,386,385,384,383,382,380,
  362,289,288,287,286,35,1,0,2,283,284,285,
472,471,470,469,468,467,466,465,464,463,462,461,460,459,458,457,456,455,454,
  453,452,451,450,449,448,447,446,445,444,443,442,441,440,439,438,437,436,
  435,434,433,432,431,430,429,428,427,426,425,424,423,422,421,420,419,418,
  417,416,415,414,413,412,411,410,409,408,407,406,405,404,403,401,400,399,
  398,397,396,395,394,393,392,391,390,389,388,387,386,385,384,383,382,380,
  362,289,288,287,286,35,1,0,2,283,284,285,
472,471,470,469,468,467,466,465,464,463,462,461,460,459,458,457,456,455,454,
  453,452,451,450,449,448,447,446,445,444,443,442,441,440,439,438,437,436,
  435,434,433,432,431,430,429,428,427,426,425,424,423,422,421,420,419,418,
  417,416,415,414,413,412,411,410,409,408,407,406,405,404,403,401,400,399,
  398,397,396,395,394,393,392,391,390,389,388,387,386,385,384,383,382,380,
  362,289,288,287,286,35,1,0,2,283,284,285,
472,471,470,469,468,467,466,465,464,463,462,461,460,459,458,457,456,455,454,
  453,452,451,450,449,448,447,446,445,444,443,442,441,440,439,438,437,436,
  435,434,433,432,431,430,429,428,427,426,425,424,423,422,421,420,419,418,
  417,416,415,414,413,412,411,410,409,408,407,406,405,404,403,401,400,399,
  398,397,396,395,394,393,392,391,390,389,388,387,386,385,384,383,382,380,
  362,289,288,287,286,35,1,0,2,283,284,285,
472,471,470,469,468,467,466,465,464,463,462,461,460,459,458,457,456,455,454,
  453,452,451,450,449,448,447,446,445,444,443,442,441,440,439,438,437,436,
  435,434,433,432,431,430,429,428,427,426,425,424,423,422,421,420,419,418,
  417,416,415,414,413,412,411,410,409,408,407,406,405,404,403,401,400,399,
  398,397,396,395,394,393,392,391,390,389,388,387,386,385,384,383,382,380,
  362,289,288,287,286,35,1,0,2,283,284,285,
472,471,470,469,468,467,466,465,464,463,462,461,460,459,458,457,456,455,454,
  453,452,451,450,449,448,447,446,445,444,443,442,441,440,439,438,437,436,
  435,434,433,432,431,430,429,428,427,426,425,424,423,422,421,420,419,418,
  417,416,415,414,413,412,411,410,409,408,407,406,405,404,403,401,400,399,
  398,397,396,395,394,393,392,391,390,389,388,387,386,385,384,383,382,380,
  362,289,288,287,286,35,1,0,2,283,284,285,
472,471,470,469,468,467,466,465,464,463,462,461,460,459,458,457,456,455,454,
  453,452,451,450,449,448,447,446,445,444,443,442,441,440,439,438,437,436,
  435,434,433,432,431,430,429,428,427,426,425,424,423,422,421,420,419,418,
  417,416,415,414,413,412,411,410,409,408,407,406,405,404,403,401,400,399,
  398,397,396,395,394,393,392,391,390,389,388,387,386,385,384,383,382,380,
  362,289,288,287,286,35,1,0,2,283,284,285,
472,471,470,469,468,467,466,465,464,463,462,461,460,459,458,457,456,455,454,
  453,452,451,450,449,448,447,446,445,444,443,442,441,440,439,438,437,436,
  435,434,433,432,431,430,429,428,427,426,425,424,423,422,421,420,419,418,
  417,416,415,414,413,412,411,410,409,408,407,406,405,404,403,401,400,399,
  398,397,396,395,394,393,392,391,390,389,388,387,386,385,384,383,382,380,
  362,289,288,287,286,35,1,0,2,283,284,285,
472,471,470,469,468,467,466,465,464,463,462,461,460,459,458,457,456,455,454,
  453,452,451,450,449,448,447,446,445,444,443,442,441,440,439,438,437,436,
  435,434,433,432,431,430,429,428,427,426,425,424,423,422,421,420,419,418,
  417,416,415,414,413,412,411,410,409,408,407,406,405,404,403,401,400,399,
  398,397,396,395,394,393,392,391,390,389,388,387,386,385,384,383,382,380,
  362,289,288,287,286,35,1,0,2,283,284,285,
472,471,470,469,468,467,466,465,464,463,462,461,460,459,458,457,456,455,454,
  453,452,451,450,449,448,447,446,445,444,443,442,441,440,439,438,437,436,
  435,434,433,432,431,430,429,428,427,426,425,424,423,422,421,420,419,418,
  417,416,415,414,413,412,411,410,409,408,407,406,405,404,403,401,400,399,
  398,397,396,395,394,393,392,391,390,389,388,387,386,385,384,383,382,380,
  362,289,288,287,286,35,1,0,2,283,284,285,
472,471,470,469,468,467,466,465,464,463,462,461,460,459,458,457,456,455,454,
  453,452,451,450,449,448,447,446,445,444,443,442,441,440,439,438,437,436,
  435,434,433,432,431,430,429,428,427,426,425,424,423,422,421,420,419,418,
  417,416,415,414,413,412,411,410,409,408,407,406,405,404,403,401,400,399,
  398,397,396,395,394,393,392,391,390,389,388,387,386,385,384,383,382,380,
  362,289,288,287,286,35,1,0,2,283,284,285,
472,471,470,469,468,467,466,465,464,463,462,461,460,459,458,457,456,455,454,
  453,452,451,450,449,448,447,446,445,444,443,442,441,440,439,438,437,436,
  435,434,433,432,431,430,429,428,427,426,425,424,423,422,421,420,419,418,
  417,416,415,414,413,412,411,410,409,408,407,406,405,404,403,401,400,399,
  398,397,396,395,394,393,392,391,390,389,388,387,386,385,384,383,382,380,
  362,289,288,287,286,35,1,0,2,283,284,285,
472,471,470,469,468,467,466,465,464,463,462,461,460,459,458,457,456,455,454,
  453,452,451,450,449,448,447,446,445,444,443,442,441,440,439,438,437,436,
  435,434,433,432,431,430,429,428,427,426,425,424,423,422,421,420,419,418,
  417,416,415,414,413,412,411,410,409,408,407,406,405,404,403,401,400,399,
  398,397,396,395,394,393,392,391,390,389,388,387,386,385,384,383,382,380,
  362,289,288,287,286,35,1,0,2,283,284,285,
379,378,377,376,375,35,1,0,2,283,284,285,
379,378,377,376,375,35,1,0,2,283,284,285,
472,471,470,469,468,467,466,465,464,463,462,461,460,459,458,457,456,455,454,
  453,452,451,450,449,448,447,446,445,444,443,442,441,440,439,438,437,436,
  435,434,433,432,431,430,429,428,427,426,425,424,423,422,421,420,419,418,
  417,416,415,414,413,412,411,410,409,408,407,406,405,404,403,401,400,399,
  398,397,396,395,394,393,392,391,390,389,388,387,386,385,384,383,382,380,
  362,289,288,287,286,35,1,0,2,283,284,285,
490,488,487,486,485,484,483,480,479,478,477,476,475,373,372,371,370,369,368,
  367,366,364,363,362,356,355,169,86,35,1,0,2,283,284,285,
490,488,487,486,485,484,483,480,479,478,477,476,475,373,372,371,370,369,368,
  367,366,364,363,362,356,355,169,86,35,1,0,2,283,284,285,
379,377,376,375,179,178,177,176,35,1,0,2,283,284,285,
472,471,470,469,468,467,466,465,464,463,462,461,460,459,458,457,456,455,454,
  453,452,451,450,449,448,447,446,445,444,443,442,441,440,439,438,437,436,
  435,434,433,432,431,430,429,428,427,426,425,424,423,422,421,420,419,418,
  417,416,415,414,413,412,411,410,409,408,407,406,405,404,403,401,400,399,
  398,397,396,395,394,393,392,391,390,389,388,387,386,385,384,383,382,380,
  362,289,288,287,286,35,1,0,2,283,284,285,
379,377,376,375,179,178,177,176,35,1,0,2,283,284,285,
379,377,376,375,179,178,177,176,35,1,0,2,283,284,285,
377,376,375,181,35,1,0,2,283,284,285,
472,471,470,469,468,467,466,465,464,463,462,461,460,459,458,457,456,455,454,
  453,452,451,450,449,448,447,446,445,444,443,442,441,440,439,438,437,436,
  435,434,433,432,431,430,429,428,427,426,425,424,423,422,421,420,419,418,
  417,416,415,414,413,412,411,410,409,408,407,406,405,404,403,401,400,399,
  398,397,396,395,394,393,392,391,390,389,388,387,386,385,384,383,382,380,
  362,289,288,287,286,35,1,0,2,283,284,285,
490,488,487,486,485,484,483,480,479,478,477,476,475,373,372,371,370,369,368,
  367,366,364,363,362,356,355,169,86,35,1,0,2,283,284,285,
490,488,487,486,485,484,483,480,479,478,477,476,475,373,372,371,370,369,368,
  367,366,364,363,362,356,355,169,86,35,1,0,2,283,284,285,
490,488,487,486,485,484,483,480,479,478,477,476,475,373,372,371,370,369,368,
  367,366,364,363,362,356,355,169,86,35,1,0,2,283,284,285,
376,375,35,1,0,2,283,284,285,
490,488,487,486,485,484,483,480,479,478,477,476,475,373,372,371,370,369,368,
  367,366,364,363,362,356,355,169,86,35,1,0,2,283,284,285,
490,488,487,486,485,484,483,480,479,478,477,476,475,373,372,371,370,369,368,
  367,366,364,363,362,356,355,169,86,35,1,0,2,283,284,285,
490,488,487,486,485,484,483,480,479,478,477,476,475,373,372,371,370,369,368,
  367,366,364,363,362,356,355,169,86,35,1,0,2,283,284,285,
490,488,487,486,485,484,483,480,479,478,477,476,475,373,372,371,370,369,368,
  367,366,364,363,362,356,355,169,86,35,1,0,2,283,284,285,
490,488,487,486,485,484,483,480,479,478,477,476,475,373,372,371,370,369,368,
  367,366,364,363,362,356,355,169,86,35,1,0,2,283,284,285,
490,488,487,486,485,484,483,480,479,478,477,476,475,373,372,371,370,369,368,
  367,366,364,363,362,356,355,169,86,35,1,0,2,283,284,285,
490,488,487,486,485,484,483,480,479,478,477,476,475,373,372,371,370,369,368,
  367,366,364,363,362,356,355,169,86,35,1,0,2,283,284,285,
490,488,487,486,485,484,483,480,479,478,477,476,475,373,372,371,370,369,368,
  367,366,364,363,362,356,355,169,86,35,1,0,2,283,284,285,
490,488,487,486,485,484,483,480,479,478,477,476,475,373,372,371,370,369,368,
  367,366,364,363,362,356,355,169,86,35,1,0,2,283,284,285,
490,488,487,486,485,484,483,480,479,478,477,476,475,373,372,371,370,369,368,
  367,366,364,363,362,356,355,169,86,35,1,0,2,283,284,285,
490,488,487,486,485,484,483,480,479,478,477,476,475,373,372,371,370,369,368,
  367,366,364,363,362,356,355,169,86,35,1,0,2,283,284,285,
490,488,487,486,485,484,483,480,479,478,477,476,475,373,372,371,370,369,368,
  367,366,364,363,362,356,355,169,86,35,1,0,2,283,284,285,
490,488,487,486,485,484,483,480,479,478,477,476,475,373,372,371,370,369,368,
  367,366,364,363,362,356,355,169,86,35,1,0,2,283,284,285,
490,488,487,486,485,484,483,480,479,478,477,476,475,373,372,371,370,369,368,
  367,366,364,363,362,356,355,169,86,35,1,0,2,283,284,285,
377,376,375,181,35,1,0,2,283,284,285,
379,377,376,375,179,178,177,176,35,1,0,2,283,284,285,
490,488,487,486,485,484,483,480,479,478,477,476,475,373,372,371,370,369,368,
  367,366,364,363,362,356,355,169,86,35,1,0,2,283,284,285,
472,471,470,469,468,467,466,465,464,463,462,461,460,459,458,457,456,455,454,
  453,452,451,450,449,448,447,446,445,444,443,442,441,440,439,438,437,436,
  435,434,433,432,431,430,429,428,427,426,425,424,423,422,421,420,419,418,
  417,416,415,414,413,412,411,410,409,408,407,406,405,404,403,401,400,399,
  398,397,396,395,394,393,392,391,390,389,388,387,386,385,384,383,382,380,
  362,289,288,287,286,35,1,0,2,283,284,285,
472,471,470,469,468,467,466,465,464,463,462,461,460,459,458,457,456,455,454,
  453,452,451,450,449,448,447,446,445,444,443,442,441,440,439,438,437,436,
  435,434,433,432,431,430,429,428,427,426,425,424,423,422,421,420,419,418,
  417,416,415,414,413,412,411,410,409,408,407,406,405,404,403,401,400,399,
  398,397,396,395,394,393,392,391,390,389,388,387,386,385,384,383,382,380,
  362,289,288,287,286,35,1,0,2,283,284,285,
472,471,470,469,468,467,466,465,464,463,462,461,460,459,458,457,456,455,454,
  453,452,451,450,449,448,447,446,445,444,443,442,441,440,439,438,437,436,
  435,434,433,432,431,430,429,428,427,426,425,424,423,422,421,420,419,418,
  417,416,415,414,413,412,411,410,409,408,407,406,405,404,403,401,400,399,
  398,397,396,395,394,393,392,391,390,389,388,387,386,385,384,383,382,380,
  362,289,288,287,286,35,1,0,2,283,284,285,
472,471,470,469,468,467,466,465,464,463,462,461,460,459,458,457,456,455,454,
  453,452,451,450,449,448,447,446,445,444,443,442,441,440,439,438,437,436,
  435,434,433,432,431,430,429,428,427,426,425,424,423,422,421,420,419,418,
  417,416,415,414,413,412,411,410,409,408,407,406,405,404,403,401,400,399,
  398,397,396,395,394,393,392,391,390,389,388,387,386,385,384,383,382,380,
  362,289,288,287,286,35,1,0,2,283,284,285,
377,376,375,181,35,1,0,2,283,284,285,
379,377,376,375,179,178,177,176,35,1,0,2,283,284,285,
377,376,375,181,35,1,0,2,283,284,285,
472,471,470,469,468,467,466,465,464,463,462,461,460,459,458,457,456,455,454,
  453,452,451,450,449,448,447,446,445,444,443,442,441,440,439,438,437,436,
  435,434,433,432,431,430,429,428,427,426,425,424,423,422,421,420,419,418,
  417,416,415,414,413,412,411,410,409,408,407,406,405,404,403,401,400,399,
  398,397,396,395,394,393,392,391,390,389,388,387,386,385,384,383,382,380,
  362,289,288,287,286,35,1,0,2,283,284,285,
490,488,487,486,485,484,483,480,479,478,477,476,475,373,372,371,370,369,368,
  367,366,364,363,362,356,355,169,86,35,1,0,2,283,284,285,
490,488,487,486,485,484,483,480,479,478,477,476,475,373,372,371,370,369,368,
  367,366,364,363,362,356,355,169,86,35,1,0,2,283,284,285,
490,488,487,486,485,484,483,480,479,478,477,476,475,373,372,371,370,369,368,
  367,366,364,363,362,356,355,169,86,35,1,0,2,283,284,285,
490,488,487,486,485,484,483,480,479,478,477,476,475,373,372,371,370,369,368,
  367,366,364,363,362,356,355,169,86,35,1,0,2,283,284,285,
490,488,487,486,485,484,483,480,479,478,477,476,475,373,372,371,370,369,368,
  367,366,364,363,362,356,355,169,86,35,1,0,2,283,284,285,
490,488,487,486,485,484,483,480,479,478,477,476,475,373,372,371,370,369,368,
  367,366,364,363,362,356,355,169,86,35,1,0,2,283,284,285,
490,488,487,486,485,484,483,480,479,478,477,476,475,373,372,371,370,369,368,
  367,366,364,363,362,356,355,169,86,35,1,0,2,283,284,285,
379,377,376,375,179,178,177,176,35,1,0,2,283,284,285,
472,471,470,469,468,467,466,465,464,463,462,461,460,459,458,457,456,455,454,
  453,452,451,450,449,448,447,446,445,444,443,442,441,440,439,438,437,436,
  435,434,433,432,431,430,429,428,427,426,425,424,423,422,421,420,419,418,
  417,416,415,414,413,412,411,410,409,408,407,406,405,404,403,401,400,399,
  398,397,396,395,394,393,392,391,390,389,388,387,386,385,384,383,382,380,
  362,289,288,287,286,35,1,0,2,283,284,285,
472,471,470,469,468,467,466,465,464,463,462,461,460,459,458,457,456,455,454,
  453,452,451,450,449,448,447,446,445,444,443,442,441,440,439,438,437,436,
  435,434,433,432,431,430,429,428,427,426,425,424,423,422,421,420,419,418,
  417,416,415,414,413,412,411,410,409,408,407,406,405,404,403,401,400,399,
  398,397,396,395,394,393,392,391,390,389,388,387,386,385,384,383,382,380,
  362,289,288,287,286,35,1,0,2,283,284,285,
490,488,487,486,485,484,483,480,479,478,477,476,475,373,372,371,370,369,368,
  367,366,364,363,362,356,355,169,86,35,1,0,2,283,284,285,
490,488,487,486,485,484,483,480,479,478,477,476,475,373,372,371,370,369,368,
  367,366,364,363,362,356,355,169,86,35,1,0,2,283,284,285,
490,488,487,486,485,484,483,480,479,478,477,476,475,373,372,371,370,369,368,
  367,366,364,363,362,356,355,169,86,35,1,0,2,283,284,285,
472,471,470,469,468,467,466,465,464,463,462,461,460,459,458,457,456,455,454,
  453,452,451,450,449,448,447,446,445,444,443,442,441,440,439,438,437,436,
  435,434,433,432,431,430,429,428,427,426,425,424,423,422,421,420,419,418,
  417,416,415,414,413,412,411,410,409,408,407,406,405,404,403,401,400,399,
  398,397,396,395,394,393,392,391,390,389,388,387,386,385,384,383,382,380,
  362,289,288,287,286,35,1,0,2,283,284,285,
490,488,487,486,485,484,483,480,479,478,477,476,475,373,372,371,370,369,368,
  367,366,364,363,362,356,355,169,86,35,1,0,2,283,284,285,
379,377,376,375,179,178,177,176,35,1,0,2,283,284,285,
490,488,487,486,485,484,483,480,479,478,477,476,475,373,372,371,370,369,368,
  367,366,364,363,362,356,355,169,86,35,1,0,2,283,284,285,
379,377,376,375,179,178,177,176,35,1,0,2,283,284,285,
379,377,376,375,179,178,177,176,35,1,0,2,283,284,285,
490,488,487,486,485,484,483,480,479,478,477,476,475,373,372,371,370,369,368,
  367,366,364,363,362,356,355,169,86,35,1,0,2,283,284,285,
490,488,487,486,485,484,483,480,479,478,477,476,475,373,372,371,370,369,368,
  367,366,364,363,362,356,355,169,86,1,0,39,40,
379,377,376,375,179,178,177,176,1,0,39,40,
490,488,487,486,485,484,483,480,479,478,477,476,475,373,372,371,370,369,368,
  367,366,364,363,362,356,355,169,86,1,0,39,40,
379,377,376,375,179,178,177,176,1,0,39,40,
376,375,1,0,39,40,
490,488,487,486,485,484,483,480,479,478,477,476,475,373,372,371,370,369,368,
  367,366,364,363,362,356,355,169,86,1,0,39,40,
490,488,487,486,485,484,483,480,479,478,477,476,475,373,372,371,370,369,368,
  367,366,364,363,362,356,355,169,86,1,0,39,40,
490,488,487,486,485,484,483,480,479,478,477,476,475,373,372,371,370,369,368,
  367,366,364,363,362,356,355,169,86,1,0,39,40,
379,377,376,375,179,178,177,176,1,0,39,40,
479,478,1,0,39,40,
379,378,377,376,375,1,0,39,40,
379,378,377,376,375,1,0,39,40,
490,488,487,486,485,484,483,480,479,478,477,476,475,373,372,371,370,369,368,
  367,366,364,363,362,356,355,169,86,1,0,39,40,
490,488,487,486,485,484,483,480,479,478,477,476,475,373,372,371,370,369,368,
  367,366,364,363,362,356,355,169,86,1,0,39,40,
379,377,376,375,179,178,177,176,1,0,39,40,
379,377,376,375,179,178,177,176,1,0,39,40,
379,377,376,375,179,178,177,176,1,0,39,40,
377,376,375,181,1,0,39,40,
490,488,487,486,485,484,483,480,479,478,477,476,475,373,372,371,370,369,368,
  367,366,364,363,362,356,355,169,86,1,0,39,40,
490,488,487,486,485,484,483,480,479,478,477,476,475,373,372,371,370,369,368,
  367,366,364,363,362,356,355,169,86,1,0,39,40,
490,488,487,486,485,484,483,480,479,478,477,476,475,373,372,371,370,369,368,
  367,366,364,363,362,356,355,169,86,1,0,39,40,
376,375,1,0,39,40,
490,488,487,486,485,484,483,480,479,478,477,476,475,373,372,371,370,369,368,
  367,366,364,363,362,356,355,169,86,1,0,39,40,
490,488,487,486,485,484,483,480,479,478,477,476,475,373,372,371,370,369,368,
  367,366,364,363,362,356,355,169,86,1,0,39,40,
490,488,487,486,485,484,483,480,479,478,477,476,475,373,372,371,370,369,368,
  367,366,364,363,362,356,355,169,86,1,0,39,40,
490,488,487,486,485,484,483,480,479,478,477,476,475,373,372,371,370,369,368,
  367,366,364,363,362,356,355,169,86,1,0,39,40,
490,488,487,486,485,484,483,480,479,478,477,476,475,373,372,371,370,369,368,
  367,366,364,363,362,356,355,169,86,1,0,39,40,
490,488,487,486,485,484,483,480,479,478,477,476,475,373,372,371,370,369,368,
  367,366,364,363,362,356,355,169,86,1,0,39,40,
490,488,487,486,485,484,483,480,479,478,477,476,475,373,372,371,370,369,368,
  367,366,364,363,362,356,355,169,86,1,0,39,40,
490,488,487,486,485,484,483,480,479,478,477,476,475,373,372,371,370,369,368,
  367,366,364,363,362,356,355,169,86,1,0,39,40,
490,488,487,486,485,484,483,480,479,478,477,476,475,373,372,371,370,369,368,
  367,366,364,363,362,356,355,169,86,1,0,39,40,
490,488,487,486,485,484,483,480,479,478,477,476,475,373,372,371,370,369,368,
  367,366,364,363,362,356,355,169,86,1,0,39,40,
490,488,487,486,485,484,483,480,479,478,477,476,475,373,372,371,370,369,368,
  367,366,364,363,362,356,355,169,86,1,0,39,40,
490,488,487,486,485,484,483,480,479,478,477,476,475,373,372,371,370,369,368,
  367,366,364,363,362,356,355,169,86,1,0,39,40,
490,488,487,486,485,484,483,480,479,478,477,476,475,373,372,371,370,369,368,
  367,366,364,363,362,356,355,169,86,1,0,39,40,
490,488,487,486,485,484,483,480,479,478,477,476,475,373,372,371,370,369,368,
  367,366,364,363,362,356,355,169,86,1,0,39,40,
377,376,375,181,1,0,39,40,
379,377,376,375,179,178,177,176,1,0,39,40,
490,488,487,486,485,484,483,480,479,478,477,476,475,373,372,371,370,369,368,
  367,366,364,363,362,356,355,169,86,1,0,39,40,
377,376,375,181,1,0,39,40,
379,377,376,375,179,178,177,176,1,0,39,40,
377,376,375,181,1,0,39,40,
490,488,487,486,485,484,483,480,479,478,477,476,475,373,372,371,370,369,368,
  367,366,364,363,362,356,355,169,86,1,0,39,40,
490,488,487,486,485,484,483,480,479,478,477,476,475,373,372,371,370,369,368,
  367,366,364,363,362,356,355,169,86,1,0,39,40,
490,488,487,486,485,484,483,480,479,478,477,476,475,373,372,371,370,369,368,
  367,366,364,363,362,356,355,169,86,1,0,39,40,
490,488,487,486,485,484,483,480,479,478,477,476,475,373,372,371,370,369,368,
  367,366,364,363,362,356,355,169,86,1,0,39,40,
490,488,487,486,485,484,483,480,479,478,477,476,475,373,372,371,370,369,368,
  367,366,364,363,362,356,355,169,86,1,0,39,40,
490,488,487,486,485,484,483,480,479,478,477,476,475,373,372,371,370,369,368,
  367,366,364,363,362,356,355,169,86,1,0,39,40,
490,488,487,486,485,484,483,480,479,478,477,476,475,373,372,371,370,369,368,
  367,366,364,363,362,356,355,169,86,1,0,39,40,
379,377,376,375,179,178,177,176,1,0,39,40,
490,488,487,486,485,484,483,480,479,478,477,476,475,373,372,371,370,369,368,
  367,366,364,363,362,356,355,169,86,1,0,39,40,
490,488,487,486,485,484,483,480,479,478,477,476,475,373,372,371,370,369,368,
  367,366,364,363,362,356,355,169,86,1,0,39,40,
490,488,487,486,485,484,483,480,479,478,477,476,475,373,372,371,370,369,368,
  367,366,364,363,362,356,355,169,86,1,0,39,40,
490,488,487,486,485,484,483,480,479,478,477,476,475,373,372,371,370,369,368,
  367,366,364,363,362,356,355,169,86,1,0,39,40,
379,377,376,375,179,178,177,176,1,0,39,40,
490,488,487,486,485,484,483,480,479,478,477,476,475,373,372,371,370,369,368,
  367,366,364,363,362,356,355,169,86,1,0,39,40,
379,377,376,375,179,178,177,176,1,0,39,40,
379,377,376,375,179,178,177,176,1,0,39,40,
490,488,487,486,485,484,483,480,479,478,477,476,475,373,372,371,370,369,368,
  367,366,364,363,362,356,355,169,86,1,0,39,40,
472,471,470,469,468,467,466,465,464,463,462,461,460,459,458,457,456,455,454,
  453,452,451,450,449,448,447,446,445,444,443,442,441,440,439,438,437,436,
  435,434,433,432,431,430,429,428,427,426,425,424,423,422,421,420,419,418,
  417,416,415,414,413,412,411,410,409,408,407,406,405,404,403,401,400,399,
  398,397,396,395,394,393,392,391,390,389,388,387,386,385,384,383,382,380,
  362,289,288,287,286,1,0,39,40,
490,488,487,486,485,484,483,475,86,35,1,0,2,283,284,285,
490,488,487,486,485,484,483,475,86,0,4,85,299,
490,489,488,487,486,485,484,483,480,479,478,477,358,35,1,0,2,283,284,285,
490,489,488,487,486,485,484,483,480,479,478,477,358,0,20,294,
289,288,287,286,35,1,0,2,283,284,285,
289,288,287,286,35,1,0,2,283,284,285,
289,288,287,286,1,0,39,
476,474,35,1,0,2,283,284,285,
476,474,0,11,13,19,21,291,324,
289,288,287,286,35,1,0,2,283,284,285,
289,288,287,286,35,1,0,2,283,284,285,
490,488,487,486,485,484,483,480,479,478,477,476,475,373,372,371,370,369,368,
  367,366,364,363,362,356,355,169,86,35,1,0,2,283,284,285,
490,488,487,486,485,484,483,480,479,478,477,476,475,373,372,371,370,369,368,
  367,366,364,363,362,356,355,169,86,0,3,4,7,8,9,10,16,18,56,75,85,99,119,
  120,123,126,129,134,137,139,144,145,146,147,148,149,151,152,153,154,155,
  156,157,299,326,374,
490,488,487,486,485,484,483,475,289,288,287,286,86,35,1,0,2,283,284,285,
490,488,487,486,485,484,483,475,289,288,287,286,86,0,4,85,299,
490,488,487,486,485,484,483,480,479,478,477,476,475,373,372,371,370,369,368,
  367,366,364,363,362,356,355,169,86,35,1,0,2,283,284,285,
490,488,487,486,485,484,483,480,479,478,477,476,475,373,372,371,370,369,368,
  367,366,364,363,362,356,355,169,86,0,3,4,7,8,9,10,16,18,56,85,119,120,
  123,126,129,134,137,139,144,145,146,147,148,149,151,152,153,154,155,156,
  157,299,326,374,
490,488,487,486,485,484,483,475,86,35,1,0,2,283,284,285,
490,488,487,486,485,484,483,475,86,0,4,85,299,
490,488,487,486,485,484,483,475,86,35,1,0,2,283,284,285,
490,488,487,486,485,484,483,475,86,0,4,68,85,299,
490,488,487,486,485,484,483,475,86,35,1,0,2,283,284,285,
490,488,487,486,485,484,483,475,86,0,4,68,85,299,
490,488,487,486,485,484,483,475,86,35,1,0,2,283,284,285,
490,488,487,486,485,484,483,475,86,0,4,68,85,299,
490,488,487,486,485,484,483,480,479,478,477,476,475,474,373,372,371,370,369,
  368,367,366,364,363,362,356,355,169,86,35,1,0,2,283,284,285,
490,488,487,486,485,484,483,480,479,478,477,476,475,474,373,372,371,370,369,
  368,367,366,364,363,362,356,355,169,86,0,3,4,7,8,9,10,11,13,16,18,19,21,
  56,65,85,119,120,123,126,129,134,137,139,144,145,146,147,148,149,151,
  152,153,154,155,156,157,291,299,324,326,374,
490,488,487,486,485,484,483,480,479,478,477,476,475,474,373,372,371,370,369,
  368,367,366,364,363,362,356,355,169,86,35,1,0,2,283,284,285,
490,488,487,486,485,484,483,480,479,478,477,476,475,474,373,372,371,370,369,
  368,367,366,364,363,362,356,355,169,86,0,3,4,7,8,9,10,11,13,16,18,19,21,
  56,65,85,119,120,123,126,129,134,137,139,144,145,146,147,148,149,151,
  152,153,154,155,156,157,291,299,324,326,374,
490,488,487,486,485,484,483,480,479,478,477,476,475,373,372,371,370,369,368,
  367,366,364,363,362,356,355,169,86,35,1,0,2,283,284,285,
490,488,487,486,485,484,483,480,479,478,477,476,475,373,372,371,370,369,368,
  367,366,364,363,362,356,355,169,86,0,3,4,7,8,9,10,16,18,56,85,119,120,
  123,126,129,134,137,139,144,145,146,147,148,149,151,152,153,154,155,156,
  157,299,326,374,
289,288,287,286,35,1,0,2,283,284,285,
289,288,287,286,35,1,0,2,283,284,285,
289,288,287,286,35,1,0,2,283,284,285,
490,488,487,486,485,484,483,480,479,478,477,476,475,373,372,371,370,369,368,
  367,366,364,363,362,356,355,169,86,35,1,0,2,283,284,285,
490,488,487,486,485,484,483,480,479,478,477,476,475,373,372,371,370,369,368,
  367,366,364,363,362,356,355,169,86,0,3,4,7,8,9,10,16,18,56,85,119,120,
  123,126,129,134,137,139,144,145,146,147,148,149,151,152,153,154,155,156,
  157,299,326,374,
490,488,487,486,485,484,483,480,479,478,477,476,475,474,373,372,371,370,369,
  368,367,366,364,363,362,356,355,169,86,0,3,4,7,8,9,10,11,13,16,18,19,21,
  56,65,85,119,120,123,126,129,134,137,139,144,145,146,147,148,149,151,
  152,153,154,155,156,157,291,299,324,326,374,
490,488,487,486,485,484,483,480,479,478,477,476,475,474,373,372,371,370,369,
  368,367,366,364,363,362,356,355,169,86,0,3,4,7,8,9,10,11,13,16,18,19,21,
  56,65,85,119,120,123,126,129,134,137,139,144,145,146,147,148,149,151,
  152,153,154,155,156,157,291,299,324,326,374,
490,488,487,486,485,484,483,480,479,478,477,476,475,373,372,371,370,369,368,
  367,366,364,363,362,356,355,169,86,0,3,4,7,8,9,10,16,18,56,85,119,120,
  123,126,129,134,137,139,144,145,146,147,148,149,151,152,153,154,155,156,
  157,299,326,374,
490,488,487,486,485,484,483,480,479,478,477,476,475,373,372,371,370,369,368,
  367,366,364,363,362,356,355,169,86,35,1,0,2,283,284,285,
490,488,487,486,485,484,483,480,479,478,477,476,475,373,372,371,370,369,368,
  367,366,364,363,362,356,355,169,86,0,3,4,7,8,9,10,16,18,56,85,119,120,
  123,126,129,134,137,139,144,145,146,147,148,149,151,152,153,154,155,156,
  157,299,326,374,
490,488,487,486,485,484,483,480,479,478,477,476,475,373,372,371,370,369,368,
  367,366,364,363,362,356,355,169,86,35,1,0,2,283,284,285,
490,488,487,486,485,484,483,480,479,478,477,476,475,373,372,371,370,369,368,
  367,366,364,363,362,356,355,169,86,0,3,4,7,8,9,10,16,18,56,85,119,120,
  123,126,129,134,137,139,144,145,146,147,148,149,151,152,153,154,155,156,
  157,299,326,374,
490,488,487,486,485,484,483,480,479,478,475,365,362,361,359,358,357,356,355,
  353,351,349,347,345,344,342,340,338,336,334,333,331,289,288,287,286,35,
  1,0,2,88,283,284,285,
289,288,287,286,35,1,0,2,283,284,285,
289,288,287,286,35,1,0,2,283,284,285,
491,490,489,488,487,486,485,484,483,482,481,480,479,478,477,476,475,473,366,
  365,364,362,359,358,357,356,355,349,347,345,288,287,1,0,
289,288,287,286,35,1,0,2,283,284,285,
491,490,489,488,487,486,485,484,483,482,481,480,479,478,477,476,475,474,473,
  366,365,364,362,359,358,357,356,355,349,347,345,288,287,1,0,
331,289,288,287,286,35,1,0,2,283,284,285,
490,489,488,487,486,485,484,483,480,479,478,477,358,289,288,287,35,1,0,2,
  283,284,285,
490,488,487,486,485,484,483,480,479,478,477,476,475,373,372,371,370,369,368,
  367,366,364,363,362,356,355,169,86,0,3,4,7,8,9,10,16,18,56,85,119,120,
  123,126,129,134,137,139,144,145,146,147,148,149,151,152,153,154,155,156,
  157,299,326,374,
490,488,487,486,485,484,483,480,479,478,477,476,475,373,372,371,370,369,368,
  367,366,364,363,362,356,355,169,86,0,3,4,7,8,9,10,16,18,56,85,119,120,
  123,126,129,134,137,139,144,145,146,147,148,149,151,152,153,154,155,156,
  157,299,326,374,
490,488,487,486,485,484,483,480,479,478,477,476,475,373,372,371,370,369,368,
  367,366,364,363,362,356,355,169,86,0,3,4,7,8,9,10,16,18,56,85,119,120,
  123,126,129,134,137,139,144,145,146,147,148,149,151,152,153,154,155,156,
  157,299,326,374,
379,377,376,375,179,178,177,176,0,5,381,
490,488,487,486,485,484,483,480,479,478,477,476,475,373,372,371,370,369,368,
  367,366,364,363,362,356,355,169,86,0,3,4,7,8,9,10,16,18,56,85,119,120,
  123,126,129,134,137,139,144,145,146,147,148,149,151,152,153,154,155,156,
  157,299,326,374,
379,377,376,375,179,178,177,176,0,5,381,
376,375,0,159,161,184,
490,488,487,486,485,484,483,480,479,478,477,476,475,373,372,371,370,369,368,
  367,366,364,363,362,356,355,169,86,0,3,4,7,8,9,10,16,18,56,85,119,120,
  123,126,129,134,137,139,144,145,146,147,148,149,151,152,153,154,155,156,
  157,299,326,374,
490,488,487,486,485,484,483,480,479,478,477,476,475,373,372,371,370,369,368,
  367,366,364,363,362,356,355,169,86,0,3,4,7,8,9,10,16,18,56,85,119,120,
  123,126,129,134,137,139,144,145,146,147,148,149,151,152,153,154,155,156,
  157,299,326,374,
490,488,487,486,485,484,483,480,479,478,477,476,475,373,372,371,370,369,368,
  367,366,364,363,362,356,355,169,86,0,3,4,7,8,9,10,16,18,56,85,119,120,
  123,126,129,134,137,139,144,145,146,147,148,149,151,152,153,154,155,156,
  157,299,326,374,
379,377,376,375,179,178,177,176,0,5,381,
479,478,0,165,279,
379,378,377,376,375,0,158,159,161,180,182,183,
379,378,377,376,375,0,158,159,161,180,182,183,
490,488,487,486,485,484,483,480,479,478,477,476,475,373,372,371,370,369,368,
  367,366,364,363,362,356,355,169,86,0,3,4,7,8,9,10,16,18,56,85,119,120,
  123,126,129,134,137,139,144,145,146,147,148,149,151,152,153,154,155,156,
  157,299,326,374,
490,488,487,486,485,484,483,480,479,478,477,476,475,373,372,371,370,369,368,
  367,366,364,363,362,356,355,169,86,0,3,4,7,8,9,10,16,18,56,85,119,120,
  123,126,129,134,137,139,144,145,146,147,148,149,151,152,153,154,155,156,
  157,299,326,374,
379,377,376,375,179,178,177,176,0,5,381,
379,377,376,375,179,178,177,176,0,5,381,
379,377,376,375,179,178,177,176,0,5,381,
377,376,375,181,0,6,402,
490,488,487,486,485,484,483,480,479,478,477,476,475,373,372,371,370,369,368,
  367,366,364,363,362,356,355,169,86,0,3,4,7,8,9,10,16,18,56,85,119,120,
  123,126,129,134,137,139,144,145,146,147,148,149,151,152,153,154,155,156,
  157,299,326,374,
490,488,487,486,485,484,483,480,479,478,477,476,475,373,372,371,370,369,368,
  367,366,364,363,362,356,355,169,86,0,3,4,7,8,9,10,16,18,56,85,119,120,
  123,126,129,134,137,139,144,145,146,147,148,149,151,152,153,154,155,156,
  157,299,326,374,
490,488,487,486,485,484,483,480,479,478,477,476,475,373,372,371,370,369,368,
  367,366,364,363,362,356,355,169,86,0,3,4,7,8,9,10,16,18,56,85,119,120,
  123,126,129,134,137,139,144,145,146,147,148,149,151,152,153,154,155,156,
  157,299,326,374,
376,375,0,159,161,184,
490,488,487,486,485,484,483,480,479,478,477,476,475,373,372,371,370,369,368,
  367,366,364,363,362,356,355,169,86,0,3,4,7,8,9,10,16,18,56,85,119,120,
  123,126,129,134,137,139,144,145,146,147,148,149,151,152,153,154,155,156,
  157,299,326,374,
490,488,487,486,485,484,483,480,479,478,477,476,475,373,372,371,370,369,368,
  367,366,364,363,362,356,355,169,86,0,3,4,7,8,9,10,16,18,56,85,119,120,
  123,126,129,134,137,139,144,145,146,147,148,149,151,152,153,154,155,156,
  157,299,326,374,
490,488,487,486,485,484,483,480,479,478,477,476,475,373,372,371,370,369,368,
  367,366,364,363,362,356,355,169,86,0,3,4,7,8,9,10,16,18,56,85,119,120,
  123,126,129,134,137,139,144,145,146,147,148,149,151,152,153,154,155,156,
  157,299,326,374,
490,488,487,486,485,484,483,480,479,478,477,476,475,373,372,371,370,369,368,
  367,366,364,363,362,356,355,169,86,0,3,4,7,8,9,10,16,18,56,85,119,120,
  123,126,129,134,137,139,144,145,146,147,148,149,151,152,153,154,155,156,
  157,299,326,374,
490,488,487,486,485,484,483,480,479,478,477,476,475,373,372,371,370,369,368,
  367,366,364,363,362,356,355,169,86,0,3,4,7,8,9,10,16,18,56,85,119,120,
  123,126,129,134,137,139,144,145,146,147,148,149,151,152,153,154,155,156,
  157,299,326,374,
490,488,487,486,485,484,483,480,479,478,477,476,475,373,372,371,370,369,368,
  367,366,364,363,362,356,355,169,86,0,3,4,7,8,9,10,16,18,56,85,119,120,
  123,126,129,134,137,139,144,145,146,147,148,149,151,152,153,154,155,156,
  157,299,326,374,
490,488,487,486,485,484,483,480,479,478,477,476,475,373,372,371,370,369,368,
  367,366,364,363,362,356,355,169,86,0,3,4,7,8,9,10,16,18,56,85,119,120,
  123,126,129,134,137,139,144,145,146,147,148,149,151,152,153,154,155,156,
  157,299,326,374,
490,488,487,486,485,484,483,480,479,478,477,476,475,373,372,371,370,369,368,
  367,366,364,363,362,356,355,169,86,0,3,4,7,8,9,10,16,18,56,85,119,120,
  123,126,129,134,137,139,144,145,146,147,148,149,151,152,153,154,155,156,
  157,299,326,374,
490,488,487,486,485,484,483,480,479,478,477,476,475,373,372,371,370,369,368,
  367,366,364,363,362,356,355,169,86,0,3,4,7,8,9,10,16,18,56,85,119,120,
  123,126,129,134,137,139,144,145,146,147,148,149,151,152,153,154,155,156,
  157,299,326,374,
490,488,487,486,485,484,483,480,479,478,477,476,475,373,372,371,370,369,368,
  367,366,364,363,362,356,355,169,86,0,3,4,7,8,9,10,16,18,56,85,119,120,
  123,126,129,134,137,139,144,145,146,147,148,149,151,152,153,154,155,156,
  157,299,326,374,
490,488,487,486,485,484,483,480,479,478,477,476,475,373,372,371,370,369,368,
  367,366,364,363,362,356,355,169,86,0,3,4,7,8,9,10,16,18,56,85,119,120,
  123,126,129,134,137,139,144,145,146,147,148,149,151,152,153,154,155,156,
  157,299,326,374,
490,488,487,486,485,484,483,480,479,478,477,476,475,373,372,371,370,369,368,
  367,366,364,363,362,356,355,169,86,0,3,4,7,8,9,10,16,18,56,85,119,120,
  123,126,129,134,137,139,144,145,146,147,148,149,151,152,153,154,155,156,
  157,299,326,374,
490,488,487,486,485,484,483,480,479,478,477,476,475,373,372,371,370,369,368,
  367,366,364,363,362,356,355,169,86,0,3,4,7,8,9,10,16,18,56,85,119,120,
  123,126,129,134,137,139,144,145,146,147,148,149,151,152,153,154,155,156,
  157,299,326,374,
490,488,487,486,485,484,483,480,479,478,477,476,475,373,372,371,370,369,368,
  367,366,364,363,362,356,355,169,86,0,3,4,7,8,9,10,16,18,56,85,119,120,
  123,126,129,134,137,139,144,145,146,147,148,149,151,152,153,154,155,156,
  157,299,326,374,
377,376,375,181,0,6,402,
379,377,376,375,179,178,177,176,0,5,381,
490,488,487,486,485,484,483,480,479,478,477,476,475,373,372,371,370,369,368,
  367,366,364,363,362,356,355,169,86,0,3,4,7,8,9,10,16,18,56,85,119,120,
  123,126,129,134,137,139,144,145,146,147,148,149,151,152,153,154,155,156,
  157,299,326,374,
377,376,375,181,0,6,402,
379,377,376,375,179,178,177,176,0,5,381,
377,376,375,181,0,6,402,
490,488,487,486,485,484,483,480,479,478,477,476,475,373,372,371,370,369,368,
  367,366,364,363,362,356,355,169,86,0,3,4,7,8,9,10,16,18,56,85,119,120,
  123,126,129,134,137,139,144,145,146,147,148,149,151,152,153,154,155,156,
  157,299,326,374,
490,488,487,486,485,484,483,480,479,478,477,476,475,373,372,371,370,369,368,
  367,366,364,363,362,356,355,169,86,0,3,4,7,8,9,10,16,18,56,85,119,120,
  123,126,129,134,137,139,144,145,146,147,148,149,151,152,153,154,155,156,
  157,299,326,374,
490,488,487,486,485,484,483,480,479,478,477,476,475,373,372,371,370,369,368,
  367,366,364,363,362,356,355,169,86,0,3,4,7,8,9,10,16,18,56,85,119,120,
  123,126,129,134,137,139,144,145,146,147,148,149,151,152,153,154,155,156,
  157,299,326,374,
490,488,487,486,485,484,483,480,479,478,477,476,475,373,372,371,370,369,368,
  367,366,364,363,362,356,355,169,86,0,3,4,7,8,9,10,16,18,56,85,119,120,
  123,126,129,134,137,139,144,145,146,147,148,149,151,152,153,154,155,156,
  157,299,326,374,
490,488,487,486,485,484,483,480,479,478,477,476,475,373,372,371,370,369,368,
  367,366,364,363,362,356,355,169,86,0,3,4,7,8,9,10,16,18,56,85,119,120,
  123,126,129,134,137,139,144,145,146,147,148,149,151,152,153,154,155,156,
  157,299,326,374,
490,488,487,486,485,484,483,480,479,478,477,476,475,373,372,371,370,369,368,
  367,366,364,363,362,356,355,169,86,0,3,4,7,8,9,10,16,18,56,85,119,120,
  123,126,129,134,137,139,144,145,146,147,148,149,151,152,153,154,155,156,
  157,299,326,374,
490,488,487,486,485,484,483,480,479,478,477,476,475,373,372,371,370,369,368,
  367,366,364,363,362,356,355,169,86,0,3,4,7,8,9,10,16,18,56,85,119,120,
  123,126,129,134,137,139,144,145,146,147,148,149,151,152,153,154,155,156,
  157,299,326,374,
379,377,376,375,179,178,177,176,0,5,381,
490,488,487,486,485,484,483,480,479,478,477,476,475,373,372,371,370,369,368,
  367,366,364,363,362,356,355,169,86,0,3,4,7,8,9,10,16,18,56,85,119,120,
  123,126,129,134,137,139,144,145,146,147,148,149,151,152,153,154,155,156,
  157,299,326,374,
490,488,487,486,485,484,483,480,479,478,477,476,475,373,372,371,370,369,368,
  367,366,364,363,362,356,355,169,86,0,3,4,7,8,9,10,16,18,56,85,119,120,
  123,126,129,134,137,139,144,145,146,147,148,149,151,152,153,154,155,156,
  157,299,326,374,
490,488,487,486,485,484,483,480,479,478,477,476,475,373,372,371,370,369,368,
  367,366,364,363,362,356,355,169,86,0,3,4,7,8,9,10,16,18,56,85,119,120,
  123,126,129,134,137,139,144,145,146,147,148,149,151,152,153,154,155,156,
  157,299,326,374,
490,488,487,486,485,484,483,480,479,478,477,476,475,373,372,371,370,369,368,
  367,366,364,363,362,356,355,169,86,0,3,4,7,8,9,10,16,18,56,85,119,120,
  123,126,129,134,137,139,144,145,146,147,148,149,151,152,153,154,155,156,
  157,299,326,374,
379,377,376,375,179,178,177,176,0,5,381,
490,488,487,486,485,484,483,480,479,478,477,476,475,373,372,371,370,369,368,
  367,366,364,363,362,356,355,169,86,0,3,4,7,8,9,10,16,18,56,85,119,120,
  123,126,129,134,137,139,144,145,146,147,148,149,151,152,153,154,155,156,
  157,299,326,374,
379,377,376,375,179,178,177,176,0,5,381,
379,377,376,375,179,178,177,176,0,5,381,
490,488,487,486,485,484,483,480,479,478,477,476,475,373,372,371,370,369,368,
  367,366,364,363,362,356,355,169,86,0,3,4,7,8,9,10,16,18,56,85,119,120,
  123,126,129,134,137,139,144,145,146,147,148,149,151,152,153,154,155,156,
  157,299,326,374,
472,471,470,469,468,467,466,465,464,463,462,461,460,459,458,457,456,455,454,
  453,452,451,450,449,448,447,446,445,444,443,442,441,440,439,438,437,436,
  435,434,433,432,431,430,429,428,427,426,425,424,423,422,421,420,419,418,
  417,416,415,414,413,412,411,410,409,408,407,406,405,404,403,401,400,399,
  398,397,396,395,394,393,392,391,390,389,388,387,386,385,384,383,382,380,
  362,0,144,185,186,187,188,189,190,191,192,193,194,195,196,197,198,199,
  200,201,202,203,204,205,206,207,208,209,210,211,212,213,214,215,216,217,
  218,219,220,221,222,223,224,225,226,227,228,229,230,231,232,233,234,235,
  236,237,238,239,240,241,242,243,244,245,246,247,248,249,250,251,252,253,
  254,255,256,257,258,259,260,262,263,264,265,266,267,268,269,270,271,272,
  273,274,275,276,277,
480,479,478,476,356,355,169,1,0,3,7,8,16,18,326,
490,489,488,487,486,485,484,483,482,481,480,479,478,477,475,474,473,366,365,
  364,362,359,358,357,356,355,349,347,345,289,288,287,174,1,0,17,
490,489,488,487,486,485,484,483,482,481,480,479,478,477,476,475,474,473,366,
  365,364,362,359,358,357,356,355,349,347,345,289,288,287,174,1,0,17,
331,289,288,287,286,35,1,0,2,283,284,285,
480,479,478,0,
490,488,483,480,479,478,477,475,472,471,470,469,468,467,466,465,464,463,462,
  461,460,459,458,457,456,455,454,453,452,451,450,449,448,447,446,445,444,
  443,442,441,440,439,438,437,436,435,434,433,432,431,430,429,428,427,426,
  425,424,423,422,421,420,419,418,417,416,415,414,413,412,411,410,409,408,
  407,406,405,404,403,401,400,399,398,397,396,395,394,393,392,391,390,389,
  388,387,386,385,384,383,382,380,377,376,375,365,362,361,360,359,358,357,
  356,355,354,353,352,351,350,349,348,347,346,345,344,343,342,341,340,339,
  338,337,336,335,334,333,332,331,289,288,287,286,160,35,1,0,
480,479,478,0,
480,479,478,0,
490,489,488,487,486,485,484,483,482,481,480,479,478,477,475,474,473,366,365,
  364,362,359,358,357,356,355,349,347,345,289,288,287,174,1,0,17,
480,479,478,0,
483,480,479,478,377,0,
483,480,479,478,472,471,470,469,468,467,466,465,464,463,462,461,460,459,458,
  457,456,455,454,453,452,451,450,449,448,447,446,445,444,443,442,441,440,
  439,438,437,436,435,434,433,432,431,430,429,428,427,426,425,424,423,422,
  421,420,419,418,417,416,415,414,413,412,411,410,409,408,407,406,405,404,
  403,401,400,399,398,397,396,395,394,393,392,391,390,389,388,387,386,385,
  384,383,382,380,365,362,361,360,359,358,357,356,355,354,353,352,351,350,
  349,348,347,346,345,344,343,342,341,340,339,338,337,336,335,334,333,332,
  331,289,288,287,286,35,1,0,
490,483,480,479,478,477,475,472,471,470,469,468,467,466,465,464,463,462,461,
  460,459,458,457,456,455,454,453,452,451,450,449,448,447,446,445,444,443,
  442,441,440,439,438,437,436,435,434,433,432,431,430,429,428,427,426,425,
  424,423,422,421,420,419,418,417,416,415,414,413,412,411,410,409,377,376,
  375,365,362,361,360,359,358,357,356,355,354,353,352,351,349,348,347,346,
  345,344,343,342,341,340,339,338,337,336,335,334,333,331,289,288,287,286,
  160,35,1,0,
366,35,1,0,2,283,284,285,
366,35,1,0,2,283,284,285,
366,35,1,0,2,283,284,285,
366,35,1,0,2,283,284,285,
366,35,1,0,2,283,284,285,
366,35,1,0,2,283,284,285,
366,35,1,0,2,283,284,285,
472,471,470,469,468,467,466,465,464,463,462,461,460,459,458,457,456,455,454,
  453,452,451,450,449,448,447,446,445,444,443,442,441,440,439,438,437,436,
  435,434,433,432,431,430,429,428,427,426,425,424,423,422,421,420,419,418,
  417,416,415,414,413,412,411,410,409,408,407,406,405,404,403,401,400,399,
  398,397,396,395,394,393,392,391,390,389,388,387,386,385,384,383,382,380,
  365,362,361,360,359,358,357,356,355,354,353,352,351,350,349,348,347,346,
  345,344,343,342,341,340,339,338,337,336,335,334,333,332,331,289,288,287,
  286,35,1,0,2,283,284,285,
472,471,470,469,468,467,466,465,464,463,462,461,460,459,458,457,456,455,454,
  453,452,451,450,449,448,447,446,445,444,443,442,441,440,439,438,437,436,
  435,434,433,432,431,430,429,428,427,426,425,424,423,422,421,420,419,418,
  417,416,415,414,413,412,411,410,409,408,407,406,405,404,403,401,400,399,
  398,397,396,395,394,393,392,391,390,389,388,387,386,385,384,383,382,380,
  365,362,361,360,359,358,357,356,355,354,353,352,351,350,349,348,347,346,
  345,344,343,342,341,340,339,338,337,336,335,334,333,332,331,289,288,287,
  286,35,1,0,2,283,284,285,
366,0,149,
366,0,149,
366,0,149,
366,0,149,
366,0,149,
366,0,149,
366,0,149,
490,488,487,486,485,484,483,480,479,478,477,476,475,373,372,371,370,369,368,
  367,366,356,355,169,86,35,1,0,2,283,284,285,
490,488,487,486,485,484,483,480,479,478,477,476,475,373,372,371,370,369,368,
  367,366,364,363,362,356,355,169,86,0,3,4,7,8,9,10,16,18,56,85,119,120,
  123,126,129,134,137,139,144,145,146,147,148,149,151,152,153,154,155,156,
  157,299,326,374,
490,488,487,486,485,484,483,480,479,478,477,476,475,373,372,371,370,369,368,
  367,366,356,355,169,86,0,3,4,7,8,9,10,16,18,85,139,147,148,149,151,152,
  153,154,155,156,157,299,326,374,
490,488,487,486,485,484,483,480,479,478,477,476,475,373,372,371,370,369,368,
  367,366,356,355,169,86,0,3,4,7,8,9,10,16,18,85,139,147,148,149,151,152,
  153,154,155,156,157,299,326,374,
490,488,487,486,485,484,483,480,479,478,477,476,475,373,372,371,370,369,368,
  367,366,356,355,169,86,0,3,4,7,8,9,10,16,18,85,139,147,148,149,151,152,
  153,154,155,156,157,299,326,374,
361,360,359,358,357,0,138,140,141,142,143,
356,355,0,135,136,
354,353,352,351,0,130,131,132,133,
350,349,0,127,128,
348,347,0,124,125,
346,345,0,121,122,
344,343,342,341,340,339,338,337,336,335,334,333,332,289,288,287,286,1,0,100,
  101,102,103,104,105,106,107,108,109,110,111,112,113,114,115,116,117,118,
490,488,487,486,485,484,483,480,479,478,477,476,475,373,372,371,370,369,368,
  367,366,364,363,362,356,355,169,86,0,3,4,7,8,9,10,16,18,56,85,119,120,
  123,126,129,134,137,139,144,145,146,147,148,149,151,152,153,154,155,156,
  157,299,326,374,
289,288,287,286,1,0,39,40,
331,289,288,287,286,1,0,39,40,
331,289,288,287,286,1,0,39,40,
331,289,288,287,286,1,0,39,40,
490,489,488,487,486,485,484,483,482,481,480,479,478,477,475,474,473,366,365,
  364,362,359,358,357,356,355,349,347,345,289,288,287,174,1,0,17,
331,289,288,287,286,1,0,39,40,
331,289,288,287,286,1,0,39,40,
331,289,288,287,286,1,0,39,40,
331,289,288,287,286,1,0,39,40,
491,490,489,488,487,486,485,484,483,482,481,480,479,478,477,476,475,474,473,
  366,365,364,362,359,358,357,356,355,349,347,345,289,288,287,1,0,
491,490,489,488,487,486,485,484,483,482,481,480,479,478,477,476,475,474,473,
  366,365,364,362,359,358,357,356,355,349,347,345,289,288,287,1,0,
472,471,470,469,468,467,466,465,464,463,462,461,460,459,458,457,456,455,454,
  453,452,451,450,449,448,447,446,445,444,443,442,441,440,439,438,437,436,
  435,434,433,432,431,430,429,428,427,426,425,424,423,422,421,420,419,418,
  417,416,415,414,413,412,411,410,409,408,407,406,405,404,403,401,400,399,
  398,397,396,395,394,393,392,391,390,389,388,387,386,385,384,383,382,380,
  362,331,289,288,287,286,35,1,0,2,283,284,285,
472,471,470,469,468,467,466,465,464,463,462,461,460,459,458,457,456,455,454,
  453,452,451,450,449,448,447,446,445,444,443,442,441,440,439,438,437,436,
  435,434,433,432,431,430,429,428,427,426,425,424,423,422,421,420,419,418,
  417,416,415,414,413,412,411,410,409,408,407,406,405,404,403,401,400,399,
  398,397,396,395,394,393,392,391,390,389,388,387,386,385,384,383,382,380,
  362,289,288,287,286,35,1,0,2,283,284,285,
472,471,470,469,468,467,466,465,464,463,462,461,460,459,458,457,456,455,454,
  453,452,451,450,449,448,447,446,445,444,443,442,441,440,439,438,437,436,
  435,434,433,432,431,430,429,428,427,426,425,424,423,422,421,420,419,418,
  417,416,415,414,413,412,411,410,409,408,407,406,405,404,403,401,400,399,
  398,397,396,395,394,393,392,391,390,389,388,387,386,385,384,383,382,380,
  362,289,288,287,286,35,1,0,2,283,284,285,
472,471,470,469,468,467,466,465,464,463,462,461,460,459,458,457,456,455,454,
  453,452,451,450,449,448,447,446,445,444,443,442,441,440,439,438,437,436,
  435,434,433,432,431,430,429,428,427,426,425,424,423,422,421,420,419,418,
  417,416,415,414,413,412,411,410,409,408,407,406,405,404,403,401,400,399,
  398,397,396,395,394,393,392,391,390,389,388,387,386,385,384,383,382,380,
  362,289,288,287,286,35,1,0,2,283,284,285,
472,471,470,469,468,467,466,465,464,463,462,461,460,459,458,457,456,455,454,
  453,452,451,450,449,448,447,446,445,444,443,442,441,440,439,438,437,436,
  435,434,433,432,431,430,429,428,427,426,425,424,423,422,421,420,419,418,
  417,416,415,414,413,412,411,410,409,408,407,406,405,404,403,401,400,399,
  398,397,396,395,394,393,392,391,390,389,388,387,386,385,384,383,382,380,
  362,289,288,287,286,35,1,0,2,283,284,285,
472,471,470,469,468,467,466,465,464,463,462,461,460,459,458,457,456,455,454,
  453,452,451,450,449,448,447,446,445,444,443,442,441,440,439,438,437,436,
  435,434,433,432,431,430,429,428,427,426,425,424,423,422,421,420,419,418,
  417,416,415,414,413,412,411,410,409,408,407,406,405,404,403,401,400,399,
  398,397,396,395,394,393,392,391,390,389,388,387,386,385,384,383,382,380,
  362,289,288,287,286,35,1,0,2,283,284,285,
331,1,0,39,40,
331,1,0,39,40,
472,471,470,469,468,467,466,465,464,463,462,461,460,459,458,457,456,455,454,
  453,452,451,450,449,448,447,446,445,444,443,442,441,440,439,438,437,436,
  435,434,433,432,431,430,429,428,427,426,425,424,423,422,421,420,419,418,
  417,416,415,414,413,412,411,410,409,408,407,406,405,404,403,401,400,399,
  398,397,396,395,394,393,392,391,390,389,388,387,386,385,384,383,382,380,
  362,289,288,287,286,35,1,0,2,283,284,285,
288,1,0,39,40,
490,483,480,479,478,475,377,376,375,289,288,287,286,160,35,1,0,
489,487,486,485,478,476,0,
490,489,488,487,486,485,484,483,482,481,480,479,478,477,475,474,473,366,365,
  364,362,359,358,357,356,355,349,347,345,289,288,287,174,1,0,17,
483,480,479,478,0,
490,489,488,487,486,485,484,483,482,481,480,479,478,477,476,475,474,473,366,
  365,364,362,359,358,357,356,355,349,347,345,289,288,287,174,1,0,17,
490,488,487,486,485,484,483,480,479,478,477,476,475,373,372,371,370,369,368,
  367,366,364,363,362,356,355,169,86,0,3,4,7,8,9,10,16,18,56,85,119,120,
  123,126,129,134,137,139,144,145,146,147,148,149,151,152,153,154,155,156,
  157,299,326,374,
490,488,487,486,485,484,483,480,479,478,477,476,475,373,372,371,370,369,368,
  367,366,364,363,362,356,355,169,86,0,3,4,7,8,9,10,16,18,56,85,119,120,
  123,126,129,134,137,139,144,145,146,147,148,149,151,152,153,154,155,156,
  157,299,326,374,
490,488,487,486,485,484,483,480,479,478,477,476,475,373,372,371,370,369,368,
  367,366,364,363,362,356,355,169,86,0,3,4,7,8,9,10,16,18,56,85,119,120,
  123,126,129,134,137,139,144,145,146,147,148,149,151,152,153,154,155,156,
  157,299,326,374,
490,488,487,486,485,484,483,480,479,478,477,476,475,373,372,371,370,369,368,
  367,366,364,363,362,356,355,169,86,0,3,4,7,8,9,10,16,18,56,85,119,120,
  123,126,129,134,137,139,144,145,146,147,148,149,151,152,153,154,155,156,
  157,299,326,374,
490,488,487,486,485,484,483,480,479,478,477,476,475,373,372,371,370,369,368,
  367,366,364,363,362,356,355,169,86,0,3,4,7,8,9,10,16,18,56,85,119,120,
  123,126,129,134,137,139,144,145,146,147,148,149,151,152,153,154,155,156,
  157,299,326,374,
490,488,487,486,485,484,483,480,479,478,477,476,475,373,372,371,370,369,368,
  367,366,364,363,362,356,355,169,86,0,3,4,7,8,9,10,16,18,56,85,119,120,
  123,126,129,134,137,139,144,145,146,147,148,149,151,152,153,154,155,156,
  157,299,326,374,
490,488,487,486,485,484,483,480,479,478,477,476,475,373,372,371,370,369,368,
  367,366,364,363,362,356,355,169,86,0,3,4,7,8,9,10,16,18,56,85,119,120,
  123,126,129,134,137,139,144,145,146,147,148,149,151,152,153,154,155,156,
  157,299,326,374,
365,0,150,
490,488,487,486,485,484,483,480,479,478,477,476,475,373,372,371,370,369,368,
  367,366,356,355,169,86,35,1,0,2,283,284,285,
490,488,487,486,485,484,483,480,479,478,477,476,475,373,372,371,370,369,368,
  367,366,356,355,169,86,1,0,39,40,
490,488,487,486,485,484,483,480,479,478,477,476,475,373,372,371,370,369,368,
  367,366,356,355,169,86,35,1,0,2,283,284,285,
490,488,487,486,485,484,483,480,479,478,477,476,475,373,372,371,370,369,368,
  367,366,356,355,169,86,1,0,39,40,
490,488,487,486,485,484,483,480,479,478,477,476,475,373,372,371,370,369,368,
  367,366,356,355,169,86,1,0,39,40,
490,488,487,486,485,484,483,480,479,478,477,476,475,373,372,371,370,369,368,
  367,366,356,355,169,86,1,0,39,40,
490,488,487,486,485,484,483,480,479,478,477,476,475,373,372,371,370,369,368,
  367,366,356,355,169,86,1,0,39,40,
490,488,487,486,485,484,483,480,479,478,477,476,475,373,372,371,370,369,368,
  367,366,364,363,362,356,355,169,86,1,0,39,40,
490,488,487,486,485,484,483,480,479,478,477,476,475,373,372,371,370,369,368,
  367,366,364,363,362,356,355,169,86,1,0,39,40,
490,488,487,486,485,484,483,480,479,478,477,476,475,373,372,371,370,369,368,
  367,366,364,363,362,356,355,169,86,35,1,0,2,283,284,285,
490,488,487,486,485,484,483,480,479,478,477,476,475,373,372,371,370,369,368,
  367,366,364,363,362,356,355,169,86,1,0,39,40,
490,488,487,486,485,484,483,480,479,478,477,476,475,373,372,371,370,369,368,
  367,366,364,363,362,356,355,169,86,35,1,0,2,283,284,285,
490,488,487,486,485,484,483,480,479,478,477,476,475,373,372,371,370,369,368,
  367,366,364,363,362,356,355,169,86,1,0,39,40,
490,488,487,486,485,484,483,480,479,478,477,476,475,373,372,371,370,369,368,
  367,366,364,363,362,356,355,169,86,35,1,0,2,283,284,285,
490,488,487,486,485,484,483,480,479,478,477,476,475,373,372,371,370,369,368,
  367,366,364,363,362,356,355,169,86,1,0,39,40,
490,488,487,486,485,484,483,480,479,478,477,476,475,373,372,371,370,369,368,
  367,366,364,363,362,356,355,169,86,35,1,0,2,283,284,285,
490,488,487,486,485,484,483,480,479,478,477,476,475,373,372,371,370,369,368,
  367,366,364,363,362,356,355,169,86,1,0,39,40,
490,488,487,486,485,484,483,480,479,478,477,476,475,373,372,371,370,369,368,
  367,366,364,363,362,356,355,169,86,35,1,0,2,283,284,285,
490,488,487,486,485,484,483,480,479,478,477,476,475,373,372,371,370,369,368,
  367,366,364,363,362,356,355,169,86,1,0,39,40,
490,488,487,486,485,484,483,480,479,478,477,476,475,373,372,371,370,369,368,
  367,366,364,363,362,356,355,169,86,1,0,39,40,
490,488,487,486,485,484,483,480,479,478,477,476,475,373,372,371,370,369,368,
  367,366,364,363,362,356,355,169,86,35,1,0,2,283,284,285,
490,488,487,486,485,484,483,480,479,478,477,476,475,373,372,371,370,369,368,
  367,366,364,363,362,356,355,169,86,1,0,39,40,
490,488,487,486,485,484,483,480,479,478,477,476,475,373,372,371,370,369,368,
  367,366,364,363,362,356,355,169,86,1,0,39,40,
490,488,487,486,485,484,483,480,479,478,477,476,475,373,372,371,370,369,368,
  367,366,364,363,362,356,355,169,86,35,1,0,2,283,284,285,
490,488,487,486,485,484,483,480,479,478,477,476,475,373,372,371,370,369,368,
  367,366,364,363,362,356,355,169,86,1,0,39,40,
490,488,487,486,485,484,483,480,479,478,477,476,475,373,372,371,370,369,368,
  367,366,364,363,362,356,355,169,86,1,0,39,40,
490,488,487,486,485,484,483,480,479,478,477,476,475,373,372,371,370,369,368,
  367,366,364,363,362,356,355,169,86,35,1,0,2,283,284,285,
490,488,487,486,485,484,483,480,479,478,477,476,475,373,372,371,370,369,368,
  367,366,364,363,362,356,355,169,86,35,1,0,2,283,284,285,
490,488,487,486,485,484,483,480,479,478,477,476,475,373,372,371,370,369,368,
  367,366,364,363,362,356,355,169,86,35,1,0,2,283,284,285,
490,488,487,486,485,484,483,480,479,478,477,476,475,373,372,371,370,369,368,
  367,366,364,363,362,356,355,169,86,35,1,0,2,283,284,285,
490,488,487,486,485,484,483,480,479,478,477,476,475,373,372,371,370,369,368,
  367,366,364,363,362,356,355,169,86,35,1,0,2,283,284,285,
490,488,487,486,485,484,483,480,479,478,477,476,475,373,372,371,370,369,368,
  367,366,364,363,362,356,355,169,86,35,1,0,2,283,284,285,
490,488,487,486,485,484,483,480,479,478,477,476,475,373,372,371,370,369,368,
  367,366,364,363,362,356,355,169,86,35,1,0,2,283,284,285,
490,488,487,486,485,484,483,480,479,478,477,476,475,373,372,371,370,369,368,
  367,366,364,363,362,356,355,169,86,35,1,0,2,283,284,285,
490,488,487,486,485,484,483,480,479,478,477,476,475,373,372,371,370,369,368,
  367,366,364,363,362,356,355,169,86,35,1,0,2,283,284,285,
490,488,487,486,485,484,483,480,479,478,477,476,475,373,372,371,370,369,368,
  367,366,364,363,362,356,355,169,86,35,1,0,2,283,284,285,
490,488,487,486,485,484,483,480,479,478,477,476,475,373,372,371,370,369,368,
  367,366,364,363,362,356,355,169,86,35,1,0,2,283,284,285,
490,488,487,486,485,484,483,480,479,478,477,476,475,373,372,371,370,369,368,
  367,366,364,363,362,356,355,169,86,35,1,0,2,283,284,285,
490,488,487,486,485,484,483,480,479,478,477,476,475,373,372,371,370,369,368,
  367,366,364,363,362,356,355,169,86,35,1,0,2,283,284,285,
331,0,84,
490,489,488,487,486,485,484,483,482,481,480,479,478,477,476,475,474,473,366,
  365,364,362,359,358,357,356,355,349,347,345,289,288,287,174,1,0,17,
331,289,288,287,286,0,84,
331,289,288,287,286,0,84,
331,289,288,287,286,0,84,
331,289,288,287,286,0,84,
331,0,84,
331,0,84,
288,0,34,
476,0,
365,0,150,
365,0,150,
365,0,150,
365,0,150,
365,0,150,
365,0,150,
365,0,150,
472,471,470,469,468,467,466,465,464,463,462,461,460,459,458,457,456,455,454,
  453,452,451,450,449,448,447,446,445,444,443,442,441,440,439,438,437,436,
  435,434,433,432,431,430,429,428,427,426,425,424,423,422,421,420,419,418,
  417,416,415,414,413,412,411,410,409,408,407,406,405,404,403,401,400,399,
  398,397,396,395,394,393,392,391,390,389,388,387,386,385,384,383,382,380,
  365,362,361,360,359,358,357,356,355,354,353,352,351,350,349,348,347,346,
  345,344,343,342,341,340,339,338,337,336,335,334,333,332,331,289,288,287,
  286,1,0,39,40,
490,488,487,486,485,484,483,480,479,478,477,476,475,373,372,371,370,369,368,
  367,366,356,355,169,86,0,3,4,7,8,9,10,16,18,85,139,147,148,149,151,152,
  153,154,155,156,157,299,326,374,
490,488,487,486,485,484,483,480,479,478,477,476,475,373,372,371,370,369,368,
  367,366,356,355,169,86,0,3,4,7,8,9,10,16,18,85,139,147,148,149,151,152,
  153,154,155,156,157,299,326,374,
490,488,487,486,485,484,483,480,479,478,477,476,475,373,372,371,370,369,368,
  367,366,356,355,169,86,0,3,4,7,8,9,10,16,18,85,139,147,148,149,151,152,
  153,154,155,156,157,299,326,374,
490,488,487,486,485,484,483,480,479,478,477,476,475,373,372,371,370,369,368,
  367,366,356,355,169,86,0,3,4,7,8,9,10,16,18,85,139,147,148,149,151,152,
  153,154,155,156,157,299,326,374,
490,488,487,486,485,484,483,480,479,478,477,476,475,373,372,371,370,369,368,
  367,366,356,355,169,86,0,3,4,7,8,9,10,16,18,85,139,147,148,149,151,152,
  153,154,155,156,157,299,326,374,
490,488,487,486,485,484,483,480,479,478,477,476,475,373,372,371,370,369,368,
  367,366,364,363,362,356,355,169,86,0,3,4,7,8,9,10,16,18,85,134,137,139,
  144,145,146,147,148,149,151,152,153,154,155,156,157,299,326,374,
490,488,487,486,485,484,483,480,479,478,477,476,475,373,372,371,370,369,368,
  367,366,364,363,362,356,355,169,86,0,3,4,7,8,9,10,16,18,85,134,137,139,
  144,145,146,147,148,149,151,152,153,154,155,156,157,299,326,374,
490,488,487,486,485,484,483,480,479,478,477,476,475,373,372,371,370,369,368,
  367,366,364,363,362,356,355,169,86,0,3,4,7,8,9,10,16,18,85,129,134,137,
  139,144,145,146,147,148,149,151,152,153,154,155,156,157,299,326,374,
490,488,487,486,485,484,483,480,479,478,477,476,475,373,372,371,370,369,368,
  367,366,364,363,362,356,355,169,86,0,3,4,7,8,9,10,16,18,85,129,134,137,
  139,144,145,146,147,148,149,151,152,153,154,155,156,157,299,326,374,
490,488,487,486,485,484,483,480,479,478,477,476,475,373,372,371,370,369,368,
  367,366,364,363,362,356,355,169,86,0,3,4,7,8,9,10,16,18,85,129,134,137,
  139,144,145,146,147,148,149,151,152,153,154,155,156,157,299,326,374,
490,488,487,486,485,484,483,480,479,478,477,476,475,373,372,371,370,369,368,
  367,366,364,363,362,356,355,169,86,0,3,4,7,8,9,10,16,18,85,129,134,137,
  139,144,145,146,147,148,149,151,152,153,154,155,156,157,299,326,374,
490,488,487,486,485,484,483,480,479,478,477,476,475,373,372,371,370,369,368,
  367,366,364,363,362,356,355,169,86,0,3,4,7,8,9,10,16,18,85,126,129,134,
  137,139,144,145,146,147,148,149,151,152,153,154,155,156,157,299,326,374,
490,488,487,486,485,484,483,480,479,478,477,476,475,373,372,371,370,369,368,
  367,366,364,363,362,356,355,169,86,0,3,4,7,8,9,10,16,18,85,126,129,134,
  137,139,144,145,146,147,148,149,151,152,153,154,155,156,157,299,326,374,
490,488,487,486,485,484,483,480,479,478,477,476,475,373,372,371,370,369,368,
  367,366,364,363,362,356,355,169,86,0,3,4,7,8,9,10,16,18,85,123,126,129,
  134,137,139,144,145,146,147,148,149,151,152,153,154,155,156,157,299,326,
  374,
490,488,487,486,485,484,483,480,479,478,477,476,475,373,372,371,370,369,368,
  367,366,364,363,362,356,355,169,86,0,3,4,7,8,9,10,16,18,85,123,126,129,
  134,137,139,144,145,146,147,148,149,151,152,153,154,155,156,157,299,326,
  374,
490,488,487,486,485,484,483,480,479,478,477,476,475,373,372,371,370,369,368,
  367,366,364,363,362,356,355,169,86,0,3,4,7,8,9,10,16,18,85,120,123,126,
  129,134,137,139,144,145,146,147,148,149,151,152,153,154,155,156,157,299,
  326,374,
490,488,487,486,485,484,483,480,479,478,477,476,475,373,372,371,370,369,368,
  367,366,364,363,362,356,355,169,86,0,3,4,7,8,9,10,16,18,85,120,123,126,
  129,134,137,139,144,145,146,147,148,149,151,152,153,154,155,156,157,299,
  326,374,
490,488,487,486,485,484,483,480,479,478,477,476,475,474,379,377,376,375,373,
  372,371,370,369,368,367,366,364,363,362,356,355,179,178,177,176,169,86,
  35,1,0,2,283,284,285,
490,488,487,486,485,484,483,475,86,1,0,39,40,
490,489,488,487,486,485,484,483,482,481,480,479,478,477,476,475,474,473,366,
  365,364,362,359,358,357,356,355,349,347,345,289,288,287,174,1,0,
490,488,487,486,485,484,483,480,479,478,477,476,475,474,373,372,371,370,369,
  368,367,366,364,363,362,356,355,169,86,1,0,39,40,
490,488,487,486,485,484,483,480,479,478,477,476,475,373,372,371,370,369,368,
  367,366,364,363,362,356,355,169,86,1,0,39,40,
379,377,376,375,179,178,177,176,1,0,39,40,
490,488,487,486,485,484,483,480,479,478,477,476,475,373,372,371,370,369,368,
  367,366,364,363,362,356,355,169,86,1,0,39,40,
472,471,470,469,468,467,466,465,464,463,462,461,460,459,458,457,456,455,454,
  453,452,451,450,449,448,447,446,445,444,443,442,441,440,439,438,437,436,
  435,434,433,432,431,430,429,428,427,426,425,424,423,422,421,420,419,418,
  417,416,415,414,413,412,411,410,409,408,407,406,405,404,403,401,400,399,
  398,397,396,395,394,393,392,391,390,389,388,387,386,385,384,383,382,380,
  365,362,361,360,359,358,357,356,355,354,353,352,351,350,349,348,347,346,
  345,344,343,342,341,340,339,338,337,336,335,334,333,332,331,289,288,287,
  286,1,0,138,140,141,142,143,
472,471,470,469,468,467,466,465,464,463,462,461,460,459,458,457,456,455,454,
  453,452,451,450,449,448,447,446,445,444,443,442,441,440,439,438,437,436,
  435,434,433,432,431,430,429,428,427,426,425,424,423,422,421,420,419,418,
  417,416,415,414,413,412,411,410,409,408,407,406,405,404,403,401,400,399,
  398,397,396,395,394,393,392,391,390,389,388,387,386,385,384,383,382,380,
  365,362,361,360,359,358,357,356,355,354,353,352,351,350,349,348,347,346,
  345,344,343,342,341,340,339,338,337,336,335,334,333,332,331,289,288,287,
  286,1,0,138,140,141,142,143,
472,471,470,469,468,467,466,465,464,463,462,461,460,459,458,457,456,455,454,
  453,452,451,450,449,448,447,446,445,444,443,442,441,440,439,438,437,436,
  435,434,433,432,431,430,429,428,427,426,425,424,423,422,421,420,419,418,
  417,416,415,414,413,412,411,410,409,408,407,406,405,404,403,401,400,399,
  398,397,396,395,394,393,392,391,390,389,388,387,386,385,384,383,382,380,
  365,362,356,355,354,353,352,351,350,349,348,347,346,345,344,343,342,341,
  340,339,338,337,336,335,334,333,332,331,289,288,287,286,1,0,135,136,
472,471,470,469,468,467,466,465,464,463,462,461,460,459,458,457,456,455,454,
  453,452,451,450,449,448,447,446,445,444,443,442,441,440,439,438,437,436,
  435,434,433,432,431,430,429,428,427,426,425,424,423,422,421,420,419,418,
  417,416,415,414,413,412,411,410,409,408,407,406,405,404,403,401,400,399,
  398,397,396,395,394,393,392,391,390,389,388,387,386,385,384,383,382,380,
  365,362,356,355,354,353,352,351,350,349,348,347,346,345,344,343,342,341,
  340,339,338,337,336,335,334,333,332,331,289,288,287,286,1,0,135,136,
472,471,470,469,468,467,466,465,464,463,462,461,460,459,458,457,456,455,454,
  453,452,451,450,449,448,447,446,445,444,443,442,441,440,439,438,437,436,
  435,434,433,432,431,430,429,428,427,426,425,424,423,422,421,420,419,418,
  417,416,415,414,413,412,411,410,409,408,407,406,405,404,403,401,400,399,
  398,397,396,395,394,393,392,391,390,389,388,387,386,385,384,383,382,380,
  365,362,356,355,354,353,352,351,350,349,348,347,346,345,344,343,342,341,
  340,339,338,337,336,335,334,333,332,331,289,288,287,286,1,0,135,136,
472,471,470,469,468,467,466,465,464,463,462,461,460,459,458,457,456,455,454,
  453,452,451,450,449,448,447,446,445,444,443,442,441,440,439,438,437,436,
  435,434,433,432,431,430,429,428,427,426,425,424,423,422,421,420,419,418,
  417,416,415,414,413,412,411,410,409,408,407,406,405,404,403,401,400,399,
  398,397,396,395,394,393,392,391,390,389,388,387,386,385,384,383,382,380,
  365,362,356,355,354,353,352,351,350,349,348,347,346,345,344,343,342,341,
  340,339,338,337,336,335,334,333,332,331,289,288,287,286,1,0,135,136,
472,471,470,469,468,467,466,465,464,463,462,461,460,459,458,457,456,455,454,
  453,452,451,450,449,448,447,446,445,444,443,442,441,440,439,438,437,436,
  435,434,433,432,431,430,429,428,427,426,425,424,423,422,421,420,419,418,
  417,416,415,414,413,412,411,410,409,408,407,406,405,404,403,401,400,399,
  398,397,396,395,394,393,392,391,390,389,388,387,386,385,384,383,382,380,
  365,362,354,353,352,351,350,349,348,347,346,345,344,343,342,341,340,339,
  338,337,336,335,334,333,332,331,289,288,287,286,1,0,130,131,132,133,
472,471,470,469,468,467,466,465,464,463,462,461,460,459,458,457,456,455,454,
  453,452,451,450,449,448,447,446,445,444,443,442,441,440,439,438,437,436,
  435,434,433,432,431,430,429,428,427,426,425,424,423,422,421,420,419,418,
  417,416,415,414,413,412,411,410,409,408,407,406,405,404,403,401,400,399,
  398,397,396,395,394,393,392,391,390,389,388,387,386,385,384,383,382,380,
  365,362,354,353,352,351,350,349,348,347,346,345,344,343,342,341,340,339,
  338,337,336,335,334,333,332,331,289,288,287,286,1,0,130,131,132,133,
472,471,470,469,468,467,466,465,464,463,462,461,460,459,458,457,456,455,454,
  453,452,451,450,449,448,447,446,445,444,443,442,441,440,439,438,437,436,
  435,434,433,432,431,430,429,428,427,426,425,424,423,422,421,420,419,418,
  417,416,415,414,413,412,411,410,409,408,407,406,405,404,403,401,400,399,
  398,397,396,395,394,393,392,391,390,389,388,387,386,385,384,383,382,380,
  365,362,350,349,348,347,346,345,344,343,342,341,340,339,338,337,336,335,
  334,333,332,331,289,288,287,286,1,0,127,128,
472,471,470,469,468,467,466,465,464,463,462,461,460,459,458,457,456,455,454,
  453,452,451,450,449,448,447,446,445,444,443,442,441,440,439,438,437,436,
  435,434,433,432,431,430,429,428,427,426,425,424,423,422,421,420,419,418,
  417,416,415,414,413,412,411,410,409,408,407,406,405,404,403,401,400,399,
  398,397,396,395,394,393,392,391,390,389,388,387,386,385,384,383,382,380,
  365,362,350,349,348,347,346,345,344,343,342,341,340,339,338,337,336,335,
  334,333,332,331,289,288,287,286,1,0,127,128,
472,471,470,469,468,467,466,465,464,463,462,461,460,459,458,457,456,455,454,
  453,452,451,450,449,448,447,446,445,444,443,442,441,440,439,438,437,436,
  435,434,433,432,431,430,429,428,427,426,425,424,423,422,421,420,419,418,
  417,416,415,414,413,412,411,410,409,408,407,406,405,404,403,401,400,399,
  398,397,396,395,394,393,392,391,390,389,388,387,386,385,384,383,382,380,
  365,362,348,347,346,345,344,343,342,341,340,339,338,337,336,335,334,333,
  332,331,289,288,287,286,1,0,124,125,
472,471,470,469,468,467,466,465,464,463,462,461,460,459,458,457,456,455,454,
  453,452,451,450,449,448,447,446,445,444,443,442,441,440,439,438,437,436,
  435,434,433,432,431,430,429,428,427,426,425,424,423,422,421,420,419,418,
  417,416,415,414,413,412,411,410,409,408,407,406,405,404,403,401,400,399,
  398,397,396,395,394,393,392,391,390,389,388,387,386,385,384,383,382,380,
  365,362,348,347,346,345,344,343,342,341,340,339,338,337,336,335,334,333,
  332,331,289,288,287,286,1,0,124,125,
490,488,487,486,485,484,483,475,86,0,4,85,299,
490,488,487,486,485,484,483,480,479,478,477,476,475,474,373,372,371,370,369,
  368,367,366,364,363,362,356,355,169,86,0,3,4,7,8,9,10,11,13,16,18,19,21,
  56,85,119,120,123,126,129,134,137,139,144,145,146,147,148,149,151,152,
  153,154,155,156,157,291,299,324,326,374,
490,488,487,486,485,484,483,480,479,478,477,476,475,373,372,371,370,369,368,
  367,366,364,363,362,356,355,169,86,0,3,4,7,8,9,10,16,18,56,85,119,120,
  123,126,129,134,137,139,144,145,146,147,148,149,151,152,153,154,155,156,
  157,299,326,374,
379,377,376,375,179,178,177,176,0,5,381,
490,488,487,486,485,484,483,480,479,478,477,476,475,373,372,371,370,369,368,
  367,366,364,363,362,356,355,169,86,0,3,4,7,8,9,10,16,18,56,85,119,120,
  123,126,129,134,137,139,144,145,146,147,148,149,151,152,153,154,155,156,
  157,299,326,374,

};


static unsigned const char ag_astt[21732] = {
  8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,
  8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,
  8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,
  8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,1,8,7,1,1,1,1,1,1,1,1,1,1,1,
  1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,7,1,1,9,5,3,3,8,8,
  8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,
  8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,
  8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,
  8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,1,8,1,7,0,1,1,1,1,1,9,9,9,9,9,9,
  9,9,9,9,9,9,9,9,9,9,9,9,9,9,9,9,9,9,9,9,9,9,9,9,9,9,9,9,3,9,7,9,5,1,7,3,2,
  2,2,2,2,2,2,2,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,
  8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,
  8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,
  8,8,8,8,8,8,8,8,1,1,1,1,1,1,1,1,1,3,1,1,1,2,1,7,3,1,1,3,1,3,1,1,1,1,1,1,1,
  1,2,2,1,1,1,1,1,1,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,
  8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,
  8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,
  8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,1,8,3,1,7,
  3,3,1,1,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,
  5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,
  5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,
  5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,1,5,5,7,1,1,1,3,5,5,
  5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,
  5,5,5,5,5,1,5,7,1,1,1,3,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,
  5,5,5,5,5,5,5,5,5,1,5,7,1,1,1,3,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,
  5,5,5,5,5,5,5,5,5,5,5,5,5,1,5,7,1,1,1,3,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,
  1,1,1,1,1,1,1,1,1,1,1,1,1,1,8,1,1,1,7,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,
  1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,
  1,1,1,1,1,1,1,1,1,1,1,8,1,1,1,7,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,
  1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,
  1,1,1,1,1,1,1,1,8,1,1,1,7,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,
  1,1,1,1,1,1,1,1,1,1,1,2,2,2,2,2,2,2,2,2,2,2,2,1,1,7,1,2,1,1,3,5,5,5,5,5,5,
  5,5,5,1,1,7,1,1,1,3,5,5,5,5,5,5,5,5,5,1,1,7,1,1,1,3,5,5,5,5,1,1,7,1,1,1,3,
  5,5,5,5,1,1,7,1,1,1,3,5,5,5,5,5,5,5,5,5,1,1,7,1,1,1,3,5,5,5,5,5,5,5,5,5,1,
  1,7,1,1,1,3,5,5,1,1,7,1,1,1,3,5,5,1,1,7,1,1,1,3,5,5,5,5,5,5,5,5,5,5,5,5,5,
  5,5,1,1,7,1,1,1,3,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,
  5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,
  5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,
  5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,1,1,7,1,1,1,3,1,1,1,1,1,1,1,1,1,1,1,1,1,1,
  1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,
  1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,
  1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,7,1,2,2,2,1,1,1,1,1,1,1,1,
  1,1,2,2,1,1,2,1,1,1,1,1,1,1,1,1,1,2,1,1,1,2,2,1,1,1,1,1,1,1,1,2,1,1,1,2,2,
  2,2,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,2,1,1,1,2,1,1,1,2,1,1,2,2,2,
  2,2,2,2,2,2,2,2,2,2,2,1,2,1,1,1,2,2,2,1,1,2,1,1,2,1,1,2,1,1,1,1,1,7,1,1,1,
  1,1,2,2,2,2,2,2,2,2,2,7,2,1,1,2,2,2,2,2,2,2,2,2,7,2,1,1,2,2,2,2,2,2,2,2,2,
  7,2,1,1,2,2,2,2,2,2,2,2,2,7,2,1,1,1,1,7,2,2,2,2,7,2,1,2,1,1,1,2,2,2,2,2,2,
  2,2,2,2,2,2,2,2,2,7,2,1,2,1,2,1,1,1,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,
  5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,
  5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,
  5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,1,1,5,5,5,5,7,1,1,1,1,1,1,7,3,1,1,1,
  3,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,1,5,
  7,1,1,1,3,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,
  5,5,5,5,5,5,5,5,5,5,5,1,5,7,1,1,1,3,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,
  5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,1,5,7,1,1,1,3,5,5,5,5,5,
  5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,1,5,7,1,1,1,3,5,
  5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,
  5,5,5,5,5,5,1,5,7,1,1,1,3,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,
  5,5,5,5,5,5,5,5,5,5,1,5,7,1,1,1,3,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,
  5,5,5,5,5,5,5,5,5,5,5,5,5,5,1,5,7,1,1,1,3,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,
  5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,1,5,7,1,1,1,3,5,5,5,5,5,5,5,5,5,5,5,5,
  5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,1,5,7,1,1,1,3,5,5,5,5,5,5,5,5,
  5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,1,5,7,1,1,1,3,5,5,5,5,
  5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,1,5,7,1,1,1,3,
  5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,1,5,7,
  1,1,1,3,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,
  5,1,5,7,1,1,1,3,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,
  5,5,5,5,5,1,5,7,1,1,1,3,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,
  5,5,5,5,5,5,5,5,5,1,5,7,1,1,1,3,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,
  5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,
  5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,
  5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,1,5,7,1,1,1,
  3,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,
  5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,
  5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,
  5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,1,5,7,1,1,1,3,5,5,5,5,5,5,5,5,5,5,5,5,5,5,
  5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,1,5,7,1,1,1,3,5,
  5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,1,5,7,1,
  1,1,3,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,
  5,5,5,5,5,5,5,5,5,5,1,5,7,1,1,1,3,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,
  5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,1,5,7,1,1,1,3,5,5,5,5,5,5,
  5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,
  1,5,7,1,1,1,3,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,
  5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,
  5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,
  5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,
  5,5,5,5,5,5,5,5,1,5,7,1,1,1,3,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,
  5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,1,5,7,1,1,1,3,5,5,5,5,5,5,5,5,
  5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,1,5,7,1,1,1,3,5,5,5,5,
  5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,
  5,5,5,1,5,7,1,1,1,3,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,
  5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,1,5,7,1,1,1,3,5,5,5,5,5,5,5,5,5,5,5,5,5,5,
  5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,1,5,7,1,1,1,3,5,5,5,5,5,5,5,5,5,5,
  5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,1,5,7,1,1,1,3,5,5,5,5,5,5,
  5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,
  5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,
  5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,
  5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,1,5,7,1,1,1,3,5,5,5,5,5,5,5,5,5,5,5,5,
  5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,1,5,7,1,1,1,3,1,1,1,1,1,1,1,1,
  1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,9,5,3,3,3,3,3,3,3,3,3,3,
  3,3,3,3,3,3,3,3,3,3,3,3,3,3,3,3,3,3,3,3,3,3,3,3,7,3,3,7,3,3,7,3,5,5,5,5,5,
  5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,
  5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,
  5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,1,5,7,1,1,1,3,5,5,5,5,5,5,5,5,5,5,5,5,5,
  5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,1,5,7,1,1,1,3,5,5,5,5,5,5,5,5,1,5,7,1,1,1,3,
  5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,
  5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,
  5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,1,5,7,1,1,1,3,5,5,5,5,5,5,5,5,
  5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,1,5,7,1,1,1,3,5,5,5,5,5,5,5,5,1,5,
  7,1,1,1,3,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,
  5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,
  5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,1,5,7,1,1,1,3,5,5,1,
  5,7,1,1,1,3,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,1,5,7,
  1,1,1,3,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,
  5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,
  5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,1,5,7,1,1,1,3,5,5,5,5,
  5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,
  5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,
  5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,1,5,7,1,1,1,3,5,5,5,5,5,5,5,5,5,5,5,5,
  5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,
  5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,
  5,5,5,5,5,5,5,5,5,5,1,5,7,1,1,1,3,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,
  5,5,5,5,5,5,5,5,1,5,7,1,1,1,3,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,
  5,5,5,5,5,5,1,5,7,1,1,1,3,5,5,5,5,5,5,5,5,1,5,7,1,1,1,3,5,5,5,5,5,5,5,5,5,
  5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,
  5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,
  5,5,5,5,5,5,5,5,5,5,5,5,5,1,5,7,1,1,1,3,5,5,1,5,7,1,1,1,3,5,5,5,5,5,5,5,5,
  5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,
  5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,
  5,5,5,5,5,5,5,5,5,5,5,5,5,5,1,5,7,1,1,1,3,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,
  5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,
  5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,
  5,5,5,5,5,5,1,5,7,1,1,1,3,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,
  5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,
  5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,1,5,
  7,1,1,1,3,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,
  5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,
  5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,1,5,7,1,1,1,3,5,5,5,
  5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,
  5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,
  5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,1,5,7,1,1,1,3,5,5,5,5,5,5,5,5,5,5,5,
  5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,
  5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,
  5,5,5,5,5,5,5,5,5,5,5,1,5,7,1,1,1,3,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,
  5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,
  5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,
  5,5,5,1,5,7,1,1,1,3,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,
  5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,
  5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,1,5,7,1,1,
  1,3,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,
  5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,
  5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,1,5,7,1,1,1,3,5,5,5,5,5,5,
  5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,
  5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,
  5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,1,5,7,1,1,1,3,5,5,5,5,5,5,5,5,5,5,5,5,5,5,
  5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,
  5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,
  5,5,5,5,5,5,5,5,1,5,7,1,1,1,3,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,
  5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,
  5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,
  1,5,7,1,1,1,3,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,
  5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,
  5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,1,5,7,1,1,1,3,5,
  5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,
  5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,
  5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,1,5,7,1,1,1,3,5,5,5,5,5,1,5,7,1,
  1,1,3,5,5,5,5,5,1,5,7,1,1,1,3,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,
  5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,
  5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,
  1,5,7,1,1,1,3,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,1,5,
  7,1,1,1,3,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,1,5,7,1,
  1,1,3,5,5,5,5,5,5,5,5,1,5,7,1,1,1,3,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,
  5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,
  5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,
  5,5,5,1,5,7,1,1,1,3,5,5,5,5,5,5,5,5,1,5,7,1,1,1,3,5,5,5,5,5,5,5,5,1,5,7,1,
  1,1,3,5,5,5,5,1,5,7,1,1,1,3,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,
  5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,
  5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,1,
  5,7,1,1,1,3,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,1,5,7,
  1,1,1,3,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,1,5,7,1,1,
  1,3,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,1,5,7,1,1,1,3,
  5,5,1,5,7,1,1,1,3,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,
  1,5,7,1,1,1,3,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,1,5,
  7,1,1,1,3,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,1,5,7,1,
  1,1,3,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,1,5,7,1,1,1,
  3,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,1,5,7,1,1,1,3,5,
  5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,1,5,7,1,1,1,3,5,5,5,
  5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,1,5,7,1,1,1,3,5,5,5,5,5,
  5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,1,5,7,1,1,1,3,5,5,5,5,5,5,5,
  5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,1,5,7,1,1,1,3,5,5,5,5,5,5,5,5,5,
  5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,1,5,7,1,1,1,3,5,5,5,5,5,5,5,5,5,5,5,
  5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,1,5,7,1,1,1,3,5,5,5,5,5,5,5,5,5,5,5,5,5,
  5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,1,5,7,1,1,1,3,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,
  5,5,5,5,5,5,5,5,5,5,5,5,5,1,5,7,1,1,1,3,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,
  5,5,5,5,5,5,5,5,5,5,5,1,5,7,1,1,1,3,5,5,5,5,1,5,7,1,1,1,3,5,5,5,5,5,5,5,5,
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
  5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,
  5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,
  5,5,5,1,5,7,1,1,1,3,5,5,5,5,1,5,7,1,1,1,3,5,5,5,5,5,5,5,5,1,5,7,1,1,1,3,5,
  5,5,5,1,5,7,1,1,1,3,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,
  5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,
  5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,1,5,7,1,1,
  1,3,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,1,5,7,1,1,1,3,
  5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,1,5,7,1,1,1,3,5,5,
  5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,1,5,7,1,1,1,3,5,5,5,5,
  5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,1,5,7,1,1,1,3,5,5,5,5,5,5,
  5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,1,5,7,1,1,1,3,5,5,5,5,5,5,5,5,
  5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,1,5,7,1,1,1,3,5,5,5,5,5,5,5,5,5,5,
  5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,1,5,7,1,1,1,3,5,5,5,5,5,5,5,5,1,5,7,1,
  1,1,3,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,
  5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,
  5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,1,5,7,1,1,1,3,5,5,5,5,5,
  5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,
  5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,
  5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,1,5,7,1,1,1,3,5,5,5,5,5,5,5,5,5,5,5,5,5,
  5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,1,5,7,1,1,1,3,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,
  5,5,5,5,5,5,5,5,5,5,5,5,5,1,5,7,1,1,1,3,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,
  5,5,5,5,5,5,5,5,5,5,5,1,5,7,1,1,1,3,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,
  5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,
  5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,
  5,5,5,1,5,7,1,1,1,3,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,
  5,1,5,7,1,1,1,3,5,5,5,5,5,5,5,5,1,5,7,1,1,1,3,5,5,5,5,5,5,5,5,5,5,5,5,5,5,
  5,5,5,5,5,5,5,5,5,5,5,5,5,5,1,5,7,1,1,1,3,5,5,5,5,5,5,5,5,1,5,7,1,1,1,3,5,
  5,5,5,5,5,5,5,1,5,7,1,1,1,3,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,
  5,5,5,5,5,1,5,7,1,1,1,3,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,
  8,8,8,1,7,1,1,8,8,8,8,8,8,8,8,1,7,1,1,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,
  8,8,8,8,8,8,8,8,8,8,1,7,1,1,8,8,8,8,8,8,8,8,1,7,1,1,8,8,1,7,1,1,8,8,8,8,8,
  8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,1,7,1,1,8,8,8,8,8,8,8,8,8,8,
  8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,1,7,1,1,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,
  8,8,8,8,8,8,8,8,8,8,8,8,8,1,7,1,1,8,8,8,8,8,8,8,8,1,7,1,1,8,8,1,7,1,1,8,8,
  8,8,8,1,7,1,1,8,8,8,8,8,1,7,1,1,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,
  8,8,8,8,8,8,8,1,7,1,1,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,
  8,8,1,7,1,1,8,8,8,8,8,8,8,8,1,7,1,1,8,8,8,8,8,8,8,8,1,7,1,1,8,8,8,8,8,8,8,
  8,1,7,1,1,8,8,8,8,1,7,1,1,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,
  8,8,8,8,1,7,1,1,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,1,
  7,1,1,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,1,7,1,1,8,8,
  1,7,1,1,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,1,7,1,1,8,
  8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,1,7,1,1,8,8,8,8,8,8,
  8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,1,7,1,1,8,8,8,8,8,8,8,8,8,8,8,
  8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,1,7,1,1,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,
  8,8,8,8,8,8,8,8,8,8,8,8,1,7,1,1,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,
  8,8,8,8,8,8,8,1,7,1,1,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,
  8,8,1,7,1,1,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,1,7,1,
  1,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,1,7,1,1,8,8,8,8,
  8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,1,7,1,1,8,8,8,8,8,8,8,8,8,
  8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,1,7,1,1,8,8,8,8,8,8,8,8,8,8,8,8,8,8,
  8,8,8,8,8,8,8,8,8,8,8,8,8,8,1,7,1,1,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,
  8,8,8,8,8,8,8,8,8,1,7,1,1,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,
  8,8,8,8,1,7,1,1,8,8,8,8,1,7,1,1,8,8,8,8,8,8,8,8,1,7,1,1,8,8,8,8,8,8,8,8,8,
  8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,1,7,1,1,8,8,8,8,1,7,1,1,8,8,8,8,8,8,
  8,8,1,7,1,1,8,8,8,8,1,7,1,1,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,
  8,8,8,8,8,1,7,1,1,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,
  1,7,1,1,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,1,7,1,1,8,
  8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,1,7,1,1,8,8,8,8,8,8,
  8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,1,7,1,1,8,8,8,8,8,8,8,8,8,8,8,
  8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,1,7,1,1,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,
  8,8,8,8,8,8,8,8,8,8,8,8,1,7,1,1,8,8,8,8,8,8,8,8,1,7,1,1,8,8,8,8,8,8,8,8,8,
  8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,1,7,1,1,8,8,8,8,8,8,8,8,8,8,8,8,8,8,
  8,8,8,8,8,8,8,8,8,8,8,8,8,8,1,7,1,1,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,
  8,8,8,8,8,8,8,8,8,1,7,1,1,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,
  8,8,8,8,1,7,1,1,8,8,8,8,8,8,8,8,1,7,1,1,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,
  8,8,8,8,8,8,8,8,8,8,8,1,7,1,1,8,8,8,8,8,8,8,8,1,7,1,1,8,8,8,8,8,8,8,8,1,7,
  1,1,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,1,7,1,1,8,8,8,
  8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,
  8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,
  8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,5,5,5,5,1,7,1,1,5,5,5,5,5,5,5,5,5,1,1,7,1,1,
  1,3,2,2,2,2,2,2,2,2,2,7,2,1,1,5,5,5,5,5,5,5,5,5,5,5,5,5,1,1,7,1,1,1,3,2,2,
  2,2,2,2,2,2,2,2,2,2,2,7,2,1,5,5,5,5,1,1,7,1,1,1,3,5,5,5,5,1,5,7,1,1,1,3,4,
  4,4,4,1,7,1,5,5,1,1,7,1,1,1,3,1,2,7,2,1,1,2,1,1,5,5,5,5,1,1,7,1,1,1,3,5,5,
  5,5,1,1,7,1,1,1,3,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,
  1,1,7,1,1,1,3,2,2,2,2,2,2,2,2,2,1,1,1,2,1,1,1,1,1,1,1,1,1,1,1,1,1,2,2,7,2,
  2,1,1,2,1,2,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,2,1,1,1,1,1,1,1,1,1,1,1,1,5,5,
  5,5,5,5,5,5,5,5,5,5,5,1,1,7,1,1,1,3,2,2,2,2,2,2,2,2,4,4,4,4,2,7,2,1,1,5,5,
  5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,1,1,7,1,1,1,3,2,2,2,2,
  2,2,2,2,2,1,1,1,2,1,1,1,1,1,1,1,1,1,1,1,1,1,2,2,7,2,2,1,1,2,1,2,1,2,1,1,1,
  1,1,1,1,1,1,1,1,1,2,1,1,1,1,1,1,1,1,1,1,1,1,5,5,5,5,5,5,5,5,5,1,1,7,1,1,1,
  3,2,2,2,2,2,2,2,2,2,7,2,1,1,5,5,5,5,5,5,5,5,5,1,1,7,1,1,1,3,2,2,2,2,2,2,2,
  2,2,7,2,1,1,1,5,5,5,5,5,5,5,5,5,1,1,7,1,1,1,3,2,2,2,2,2,2,2,2,2,7,2,1,1,1,
  5,5,5,5,5,5,5,5,5,1,1,7,1,1,1,3,2,2,2,2,2,2,2,2,2,7,2,1,1,1,5,5,5,5,5,5,5,
  5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,1,1,7,1,1,1,3,2,2,2,2,2,2,2,2,
  2,1,1,1,2,2,1,1,1,1,1,1,1,1,1,1,1,1,1,2,2,7,2,2,1,1,2,1,2,1,2,1,1,2,2,1,1,
  1,1,1,1,1,1,1,1,1,1,1,2,1,1,1,1,1,1,1,1,1,1,1,1,1,1,5,5,5,5,5,5,5,5,5,5,5,
  5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,1,1,7,1,1,1,3,2,2,2,2,2,2,2,2,2,1,1,1,
  2,2,1,1,1,1,1,1,1,1,1,1,1,1,1,2,2,7,2,2,1,1,2,1,2,1,2,1,1,2,2,1,1,1,1,1,1,
  1,1,1,1,1,1,1,2,1,1,1,1,1,1,1,1,1,1,1,1,1,1,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,
  5,5,5,5,5,5,5,5,5,5,5,5,5,1,1,7,1,1,1,3,2,2,2,2,2,2,2,2,2,1,1,1,2,1,1,1,1,
  1,1,1,1,1,1,1,1,1,2,2,7,2,2,1,1,2,1,2,1,2,1,1,1,1,1,1,1,1,1,1,1,1,2,1,1,1,
  1,1,1,1,1,1,1,1,1,5,5,5,5,1,1,7,1,1,1,3,5,5,5,5,1,1,7,1,1,1,3,5,5,5,5,1,1,
  7,1,1,1,3,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,1,1,7,1,
  1,1,3,2,2,2,2,2,2,2,2,2,1,1,1,2,1,1,1,1,1,1,1,1,1,1,1,1,1,2,2,7,2,2,1,1,2,
  1,2,1,2,1,1,1,1,1,1,1,1,1,1,1,1,2,1,1,1,1,1,1,1,1,1,1,1,1,2,2,2,2,2,2,2,2,
  2,1,1,1,2,2,1,1,1,1,1,1,1,1,1,1,1,1,1,2,2,7,2,2,1,1,2,1,2,1,2,1,1,2,2,1,1,
  1,1,1,1,1,1,1,1,1,1,1,2,1,1,1,1,1,1,1,1,1,1,1,1,1,1,2,2,2,2,2,2,2,2,2,1,1,
  1,2,2,1,1,1,1,1,1,1,1,1,1,1,1,1,2,2,7,2,2,1,1,2,1,2,1,2,1,1,2,2,1,1,1,1,1,
  1,1,1,1,1,1,1,1,2,1,1,1,1,1,1,1,1,1,1,1,1,1,1,2,2,2,2,2,2,2,2,2,1,1,1,2,1,
  1,1,1,1,1,1,1,1,1,1,1,1,2,2,7,2,2,1,1,2,1,2,1,2,1,1,1,1,1,1,1,1,1,1,1,1,2,
  1,1,1,1,1,1,1,1,1,1,1,1,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,
  5,5,5,1,1,7,1,1,1,3,2,2,2,2,2,2,2,2,2,1,1,1,2,1,1,1,1,1,1,1,1,1,1,1,1,1,2,
  2,7,2,2,1,1,2,1,2,1,2,1,1,1,1,1,1,1,1,1,1,1,1,2,1,1,1,1,1,1,1,1,1,1,1,1,5,
  5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,1,1,7,1,1,1,3,2,2,2,
  2,2,2,2,2,2,1,1,1,2,1,1,1,1,1,1,1,1,1,1,1,1,1,2,2,7,2,2,1,1,2,1,2,1,2,1,1,
  1,1,1,1,1,1,1,1,1,1,2,1,1,1,1,1,1,1,1,1,1,1,1,2,2,2,2,2,2,2,2,2,2,2,5,5,5,
  5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,1,1,7,1,2,1,1,3,5,5,5,5,1,1,7,
  1,1,1,3,5,5,5,5,1,1,7,1,1,1,3,10,10,1,10,10,10,10,10,10,2,10,10,10,10,10,
  10,10,10,10,10,10,10,10,10,10,10,10,10,10,10,10,10,10,7,5,5,5,5,1,1,7,1,1,
  1,3,10,10,1,10,10,10,10,10,10,10,10,10,10,10,10,10,10,3,10,10,10,10,10,10,
  10,10,10,10,10,10,10,10,10,10,7,5,5,5,5,5,1,5,7,1,1,1,3,10,10,10,10,10,10,
  10,10,10,10,10,10,10,5,5,5,1,1,7,1,1,1,3,2,2,2,2,2,2,2,2,2,1,1,1,2,1,1,1,1,
  1,1,1,1,1,1,1,1,1,2,2,7,2,2,1,1,2,1,2,1,2,1,1,1,1,1,1,1,1,1,1,1,1,2,1,1,1,
  1,1,1,1,1,1,1,1,1,2,2,2,2,2,2,2,2,2,1,1,1,2,1,1,1,1,1,1,1,1,1,1,1,1,1,2,2,
  7,2,2,1,1,2,1,2,1,2,1,1,1,1,1,1,1,1,1,1,1,1,2,1,1,1,1,1,1,1,1,1,1,1,1,2,2,
  2,2,2,2,2,2,2,1,1,1,2,1,1,1,1,1,1,1,1,1,1,1,1,1,2,2,7,2,2,1,1,2,1,2,1,2,1,
  1,1,1,1,1,1,1,1,1,1,1,2,1,1,1,1,1,1,1,1,1,1,1,1,2,2,2,2,2,2,2,2,7,2,1,2,2,
  2,2,2,2,2,2,2,1,1,1,2,1,1,1,1,1,1,1,1,1,1,1,1,1,2,2,7,2,2,1,1,2,1,2,1,2,1,
  1,1,1,1,1,1,1,1,1,1,1,2,1,1,1,1,1,1,1,1,1,1,1,1,2,2,2,2,2,2,2,2,7,2,1,1,1,
  7,2,2,2,2,2,2,2,2,2,2,2,2,1,1,1,2,1,1,1,1,1,1,1,1,1,1,1,1,1,2,2,7,2,2,1,1,
  2,1,2,1,2,1,1,1,1,1,1,1,1,1,1,1,1,2,1,1,1,1,1,1,1,1,1,1,1,1,2,2,2,2,2,2,2,
  2,2,1,1,1,2,1,1,1,1,1,1,1,1,1,1,1,1,1,2,2,7,2,2,1,1,2,1,2,1,2,1,1,1,1,1,1,
  1,1,1,1,1,1,2,1,1,1,1,1,1,1,1,1,1,1,1,2,2,2,2,2,2,2,2,2,1,1,1,2,1,1,1,1,1,
  1,1,1,1,1,1,1,1,2,2,7,2,2,1,1,2,1,2,1,2,1,1,1,1,1,1,1,1,1,1,1,1,2,1,1,1,1,
  1,1,1,1,1,1,1,1,2,2,2,2,2,2,2,2,7,2,1,1,1,7,2,2,1,1,1,1,1,7,2,2,2,2,2,2,1,
  1,1,1,1,7,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,1,1,1,2,1,1,1,1,1,1,1,1,1,1,1,1,1,
  2,2,7,2,2,1,1,2,1,2,1,2,1,1,1,1,1,1,1,1,1,1,1,1,2,1,1,1,1,1,1,1,1,1,1,1,1,
  2,2,2,2,2,2,2,2,2,1,1,1,2,1,1,1,1,1,1,1,1,1,1,1,1,1,2,2,7,2,2,1,1,2,1,2,1,
  2,1,1,1,1,1,1,1,1,1,1,1,1,2,1,1,1,1,1,1,1,1,1,1,1,1,2,2,2,2,2,2,2,2,7,2,1,
  2,2,2,2,2,2,2,2,7,1,1,2,2,2,2,2,2,2,2,7,1,1,2,2,2,2,7,1,1,2,2,2,2,2,2,2,2,
  2,1,1,1,2,1,1,1,1,1,1,1,1,1,1,1,1,1,2,2,7,2,2,1,1,2,1,2,1,2,1,1,1,1,1,1,1,
  1,1,1,1,1,2,1,1,1,1,1,1,1,1,1,1,1,1,2,2,2,2,2,2,2,2,2,1,1,1,2,1,1,1,1,1,1,
  1,1,1,1,1,1,1,2,2,7,2,2,1,1,2,1,2,1,2,1,1,1,1,1,1,1,1,1,1,1,1,2,1,1,1,1,1,
  1,1,1,1,1,1,1,2,2,2,2,2,2,2,2,2,1,1,1,2,1,1,1,1,1,1,1,1,1,1,1,1,1,2,2,7,2,
  2,1,1,2,1,2,1,2,1,1,1,1,1,1,1,1,1,1,1,1,2,1,1,1,1,1,1,1,1,1,1,1,1,1,1,7,2,
  2,2,2,2,2,2,2,2,2,2,2,1,1,1,2,1,1,1,1,1,1,1,1,1,1,1,1,1,2,2,7,2,2,1,1,2,1,
  2,1,2,1,1,1,1,1,1,1,1,1,1,1,1,2,1,1,1,1,1,1,1,1,1,1,1,1,2,2,2,2,2,2,2,2,2,
  1,1,1,2,1,1,1,1,1,1,1,1,1,1,1,1,1,2,2,7,2,2,1,1,2,1,2,1,2,1,1,1,1,1,1,1,1,
  1,1,1,1,2,1,1,1,1,1,1,1,1,1,1,1,1,2,2,2,2,2,2,2,2,2,1,1,1,2,1,1,1,1,1,1,1,
  1,1,1,1,1,1,2,2,7,2,2,1,1,2,1,2,1,2,1,1,1,1,1,1,1,1,1,1,1,1,2,1,1,1,1,1,1,
  1,1,1,1,1,1,2,2,2,2,2,2,2,2,2,1,1,1,2,1,1,1,1,1,1,1,1,1,1,1,1,1,2,2,7,2,2,
  1,1,2,1,2,1,2,1,1,1,1,1,1,1,1,1,1,1,1,2,1,1,1,1,1,1,1,1,1,1,1,1,2,2,2,2,2,
  2,2,2,2,1,1,1,2,1,1,1,1,1,1,1,1,1,1,1,1,1,2,2,7,2,2,1,1,2,1,2,1,2,1,1,1,1,
  1,1,1,1,1,1,1,1,2,1,1,1,1,1,1,1,1,1,1,1,1,2,2,2,2,2,2,2,2,2,1,1,1,2,1,1,1,
  1,1,1,1,1,1,1,1,1,1,2,2,7,2,2,1,1,2,1,2,1,2,1,1,1,1,1,1,1,1,1,1,1,1,2,1,1,
  1,1,1,1,1,1,1,1,1,1,2,2,2,2,2,2,2,2,2,1,1,1,2,1,1,1,1,1,1,1,1,1,1,1,1,1,2,
  2,7,2,2,1,1,2,1,2,1,2,1,1,1,1,1,1,1,1,1,1,1,1,2,1,1,1,1,1,1,1,1,1,1,1,1,2,
  2,2,2,2,2,2,2,2,1,1,1,2,1,1,1,1,1,1,1,1,1,1,1,1,1,2,2,7,2,2,1,1,2,1,2,1,2,
  1,1,1,1,1,1,1,1,1,1,1,1,2,1,1,1,1,1,1,1,1,1,1,1,1,2,2,2,2,2,2,2,2,2,1,1,1,
  2,1,1,1,1,1,1,1,1,1,1,1,1,1,2,2,7,2,2,1,1,2,1,2,1,2,1,1,1,1,1,1,1,1,1,1,1,
  1,2,1,1,1,1,1,1,1,1,1,1,1,1,2,2,2,2,2,2,2,2,2,1,1,1,2,1,1,1,1,1,1,1,1,1,1,
  1,1,1,2,2,7,2,2,1,1,2,1,2,1,2,1,1,1,1,1,1,1,1,1,1,1,1,2,1,1,1,1,1,1,1,1,1,
  1,1,1,2,2,2,2,2,2,2,2,2,1,1,1,2,1,1,1,1,1,1,1,1,1,1,1,1,1,2,2,7,2,2,1,1,2,
  1,2,1,2,1,1,1,1,1,1,1,1,1,1,1,1,2,1,1,1,1,1,1,1,1,1,1,1,1,2,2,2,2,2,2,2,2,
  2,1,1,1,2,1,1,1,1,1,1,1,1,1,1,1,1,1,2,2,7,2,2,1,1,2,1,2,1,2,1,1,1,1,1,1,1,
  1,1,1,1,1,2,1,1,1,1,1,1,1,1,1,1,1,1,2,2,2,2,2,2,2,2,2,1,1,1,2,1,1,1,1,1,1,
  1,1,1,1,1,1,1,2,2,7,2,2,1,1,2,1,2,1,2,1,1,1,1,1,1,1,1,1,1,1,1,2,1,1,1,1,1,
  1,1,1,1,1,1,1,2,2,2,2,2,2,2,2,2,1,1,1,2,1,1,1,1,1,1,1,1,1,1,1,1,1,2,2,7,2,
  2,1,1,2,1,2,1,2,1,1,1,1,1,1,1,1,1,1,1,1,2,1,1,1,1,1,1,1,1,1,1,1,1,2,2,2,2,
  7,2,1,2,2,2,2,2,2,2,2,7,2,1,2,2,2,2,2,2,2,2,2,1,1,1,2,1,1,1,1,1,1,1,1,1,1,
  1,1,1,2,2,7,2,2,1,1,2,1,2,1,2,1,1,1,1,1,1,1,1,1,1,1,1,2,1,1,1,1,1,1,1,1,1,
  1,1,1,2,2,2,2,7,2,1,2,2,2,2,2,2,2,2,7,2,1,2,2,2,2,7,2,1,2,2,2,2,2,2,2,2,2,
  1,1,1,2,1,1,1,1,1,1,1,1,1,1,1,1,1,2,2,7,2,2,1,1,2,1,2,1,2,1,1,1,1,1,1,1,1,
  1,1,1,1,2,1,1,1,1,1,1,1,1,1,1,1,1,2,2,2,2,2,2,2,2,2,1,1,1,2,1,1,1,1,1,1,1,
  1,1,1,1,1,1,2,2,7,2,2,1,1,2,1,2,1,2,1,1,1,1,1,1,1,1,1,1,1,1,2,1,1,1,1,1,1,
  1,1,1,1,1,1,2,2,2,2,2,2,2,2,2,1,1,1,2,1,1,1,1,1,1,1,1,1,1,1,1,1,2,2,7,2,2,
  1,1,2,1,2,1,2,1,1,1,1,1,1,1,1,1,1,1,1,2,1,1,1,1,1,1,1,1,1,1,1,1,2,2,2,2,2,
  2,2,2,2,1,1,1,2,1,1,1,1,1,1,1,1,1,1,1,1,1,2,2,7,2,2,1,1,2,1,2,1,2,1,1,1,1,
  1,1,1,1,1,1,1,1,2,1,1,1,1,1,1,1,1,1,1,1,1,2,2,2,2,2,2,2,2,2,1,1,1,2,1,1,1,
  1,1,1,1,1,1,1,1,1,1,2,2,7,2,2,1,1,2,1,2,1,2,1,1,1,1,1,1,1,1,1,1,1,1,2,1,1,
  1,1,1,1,1,1,1,1,1,1,2,2,2,2,2,2,2,2,2,1,1,1,2,1,1,1,1,1,1,1,1,1,1,1,1,1,2,
  2,7,2,2,1,1,2,1,2,1,2,1,1,1,1,1,1,1,1,1,1,1,1,2,1,1,1,1,1,1,1,1,1,1,1,1,2,
  2,2,2,2,2,2,2,2,1,1,1,2,1,1,1,1,1,1,1,1,1,1,1,1,1,2,2,7,2,2,1,1,2,1,2,1,2,
  1,1,1,1,1,1,1,1,1,1,1,1,2,1,1,1,1,1,1,1,1,1,1,1,1,2,2,2,2,2,2,2,2,7,2,1,2,
  2,2,2,2,2,2,2,2,1,1,1,2,1,1,1,1,1,1,1,1,1,1,1,1,1,2,2,7,2,2,1,1,2,1,2,1,2,
  1,1,1,1,1,1,1,1,1,1,1,1,2,1,1,1,1,1,1,1,1,1,1,1,1,2,2,2,2,2,2,2,2,2,1,1,1,
  2,1,1,1,1,1,1,1,1,1,1,1,1,1,2,2,7,2,2,1,1,2,1,2,1,2,1,1,1,1,1,1,1,1,1,1,1,
  1,2,1,1,1,1,1,1,1,1,1,1,1,1,2,2,2,2,2,2,2,2,2,1,1,1,2,1,1,1,1,1,1,1,1,1,1,
  1,1,1,2,2,7,2,2,1,1,2,1,2,1,2,1,1,1,1,1,1,1,1,1,1,1,1,2,1,1,1,1,1,1,1,1,1,
  1,1,1,2,2,2,2,2,2,2,2,2,1,1,1,2,1,1,1,1,1,1,1,1,1,1,1,1,1,2,2,7,2,2,1,1,2,
  1,2,1,2,1,1,1,1,1,1,1,1,1,1,1,1,2,1,1,1,1,1,1,1,1,1,1,1,1,2,2,2,2,2,2,2,2,
  7,2,1,2,2,2,2,2,2,2,2,2,1,1,1,2,1,1,1,1,1,1,1,1,1,1,1,1,1,2,2,7,2,2,1,1,2,
  1,2,1,2,1,1,1,1,1,1,1,1,1,1,1,1,2,1,1,1,1,1,1,1,1,1,1,1,1,2,2,2,2,2,2,2,2,
  7,2,1,2,2,2,2,2,2,2,2,7,2,1,2,2,2,2,2,2,2,2,2,1,1,1,2,1,1,1,1,1,1,1,1,1,1,
  1,1,1,2,2,7,2,2,1,1,2,1,2,1,2,1,1,1,1,1,1,1,1,1,1,1,1,2,1,1,1,1,1,1,1,1,1,
  1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,
  1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,
  1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,5,3,3,1,1,1,1,1,1,2,1,1,1,2,2,1,
  1,1,1,1,1,1,1,2,1,1,1,2,2,2,2,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,2,
  1,1,1,2,1,1,1,2,1,1,2,2,2,2,2,2,2,2,2,2,2,2,2,2,1,2,1,1,1,2,2,2,1,1,2,1,1,
  2,1,1,2,2,2,1,1,1,1,2,9,7,2,1,1,2,1,1,2,1,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,
  2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,7,1,2,1,2,2,2,2,2,2,2,2,2,2,2,2,3,2,2,2,2,
  2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,7,2,5,5,5,5,5,1,5,7,1,1,1,3,2,2,2,7,4,1,4,
  4,4,4,4,4,4,4,4,4,4,4,4,4,4,4,4,4,4,4,4,4,4,4,4,4,4,4,4,4,4,4,4,4,4,4,4,4,
  4,4,4,4,4,4,4,4,4,4,4,4,4,4,4,4,4,4,4,4,4,4,4,4,4,4,4,4,4,4,4,4,4,4,4,4,4,
  4,4,4,4,4,4,4,4,4,4,4,4,4,4,4,4,4,4,4,4,4,4,4,4,4,4,4,4,4,4,4,4,4,4,4,4,4,
  4,4,4,4,4,4,4,4,4,4,4,4,4,4,4,4,4,4,4,4,4,4,4,4,4,4,4,4,7,2,2,2,7,2,2,2,7,
  2,1,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,7,1,10,
  10,10,5,10,10,10,10,2,7,10,10,10,10,4,4,4,4,4,4,4,4,4,4,4,4,4,4,4,4,4,4,4,
  4,4,4,4,4,4,4,4,4,4,4,4,4,4,4,4,4,4,4,4,4,4,4,4,4,4,4,4,4,4,4,4,4,4,4,4,4,
  4,4,4,4,4,4,4,4,4,4,4,4,4,4,4,4,4,4,4,4,4,4,4,4,4,4,4,4,4,4,4,4,4,4,4,4,4,
  4,4,4,4,4,4,4,4,4,4,4,4,4,4,4,4,4,4,4,4,4,4,4,4,4,4,4,4,4,4,4,4,4,4,4,4,4,
  7,9,2,10,10,10,2,9,4,4,4,4,4,4,4,4,4,4,4,4,4,4,4,4,4,4,4,4,4,4,4,4,4,4,4,4,
  4,4,4,4,4,4,4,4,4,4,4,4,4,4,4,4,4,4,4,4,4,4,4,4,4,4,4,4,4,4,4,4,4,4,4,4,2,
  2,2,4,4,4,4,4,4,4,4,4,4,4,4,4,4,4,4,4,4,4,4,4,4,4,4,4,4,4,4,4,4,4,4,4,4,4,
  2,4,4,7,5,1,1,7,1,1,1,3,5,1,1,7,1,1,1,3,5,1,1,7,1,1,1,3,5,1,1,7,1,1,1,3,5,
  1,1,7,1,1,1,3,5,1,1,7,1,1,1,3,5,1,1,7,1,1,1,3,5,5,5,5,5,5,5,5,5,5,5,5,5,5,
  5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,
  5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,
  5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,
  5,5,5,1,5,7,1,1,1,3,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,
  5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,
  5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,
  5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,1,1,7,1,1,1,3,1,7,1,
  1,7,1,1,7,1,1,7,1,1,7,1,1,7,1,1,7,1,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,
  5,5,5,5,5,5,1,1,7,1,1,1,3,2,2,2,2,2,2,2,2,2,1,1,1,2,1,1,1,1,1,1,1,1,1,1,1,
  1,1,2,2,7,2,2,1,1,2,1,2,1,1,1,1,1,1,1,1,1,1,1,1,1,1,2,1,1,1,1,1,1,1,1,1,1,
  1,1,2,2,2,2,2,2,2,2,2,1,1,1,2,1,1,1,1,1,1,1,1,1,1,2,2,7,2,2,1,1,2,1,2,1,1,
  2,2,2,1,1,1,1,1,1,1,1,1,1,1,2,2,2,2,2,2,2,2,2,1,1,1,2,1,1,1,1,1,1,1,1,1,1,
  2,2,7,2,2,1,1,2,1,2,1,1,2,2,2,1,1,1,1,1,1,1,1,1,1,1,2,2,2,2,2,2,2,2,2,1,1,
  1,2,1,1,1,1,1,1,1,1,1,1,2,2,7,2,2,1,1,2,1,2,1,1,2,2,2,1,1,1,1,1,1,1,1,1,1,
  1,1,1,1,1,1,5,1,1,1,1,1,1,1,5,1,1,1,1,1,1,5,1,1,1,1,1,1,5,1,1,1,1,5,1,1,1,
  1,5,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,4,4,4,4,4,7,2,2,2,2,2,2,2,2,2,2,2,2,2,2,
  2,2,2,2,2,2,2,2,2,2,2,2,2,2,1,1,1,2,1,1,1,1,1,1,1,1,1,1,1,1,1,2,2,7,2,2,1,
  1,2,1,2,1,2,1,1,1,1,1,1,1,1,1,1,1,1,2,1,1,1,1,1,1,1,1,1,1,1,1,5,5,5,5,1,7,
  1,2,8,4,4,4,4,1,7,1,1,8,4,4,4,4,1,7,1,1,8,4,4,4,4,1,7,1,1,2,1,2,2,2,2,2,2,
  2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,7,1,8,5,5,5,5,1,7,1,1,
  8,5,5,5,5,1,7,1,1,8,5,5,5,5,1,7,1,1,8,5,5,5,5,1,7,1,1,2,2,2,2,2,2,2,2,2,2,
  2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,3,2,2,2,7,2,2,2,2,2,2,2,2,2,2,2,
  2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,3,2,2,2,7,5,5,5,5,5,5,5,5,5,5,5,5,
  5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,
  5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,
  5,5,5,5,5,5,5,5,5,5,5,1,5,7,1,1,1,3,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,
  5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,
  5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,
  5,5,5,1,5,7,1,1,1,3,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,
  5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,
  5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,1,5,7,1,1,
  1,3,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,
  5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,
  5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,1,5,7,1,1,1,3,5,5,5,5,5,5,
  5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,
  5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,
  5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,1,5,7,1,1,1,3,5,5,5,5,5,5,5,5,5,5,5,5,5,5,
  5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,
  5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,
  5,5,5,5,5,5,5,5,1,5,7,1,1,1,3,8,1,7,1,1,8,1,7,1,1,5,5,5,5,5,5,5,5,5,5,5,5,
  5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,
  5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,
  5,5,5,5,5,5,5,5,5,5,1,5,7,1,1,1,3,8,1,7,1,1,9,2,10,10,10,9,2,2,2,4,4,4,4,2,
  4,4,7,2,2,2,2,2,2,7,2,1,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,
  2,2,2,2,2,2,2,7,2,2,2,2,2,7,2,1,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,
  2,2,2,2,2,2,2,2,2,2,2,2,7,1,2,2,2,2,2,2,2,2,2,1,1,1,2,1,1,1,1,1,1,1,1,1,1,
  1,1,1,2,2,7,2,2,1,1,2,1,2,1,1,1,1,1,1,1,1,1,1,1,1,1,1,2,1,1,1,1,1,1,1,1,1,
  1,1,1,2,2,2,2,2,2,2,2,2,1,1,1,2,1,1,1,1,1,1,1,1,1,1,1,1,1,2,2,7,2,2,1,1,2,
  1,2,1,1,1,1,1,1,1,1,1,1,1,1,1,1,2,1,1,1,1,1,1,1,1,1,1,1,1,2,2,2,2,2,2,2,2,
  2,1,1,1,2,1,1,1,1,1,1,1,1,1,1,1,1,1,2,2,7,2,2,1,1,2,1,2,1,1,1,1,1,1,1,1,1,
  1,1,1,1,1,2,1,1,1,1,1,1,1,1,1,1,1,1,2,2,2,2,2,2,2,2,2,1,1,1,2,1,1,1,1,1,1,
  1,1,1,1,1,1,1,2,2,7,2,2,1,1,2,1,2,1,1,1,1,1,1,1,1,1,1,1,1,1,1,2,1,1,1,1,1,
  1,1,1,1,1,1,1,2,2,2,2,2,2,2,2,2,1,1,1,2,1,1,1,1,1,1,1,1,1,1,1,1,1,2,2,7,2,
  2,1,1,2,1,2,1,1,1,1,1,1,1,1,1,1,1,1,1,1,2,1,1,1,1,1,1,1,1,1,1,1,1,2,2,2,2,
  2,2,2,2,2,1,1,1,2,1,1,1,1,1,1,1,1,1,1,1,1,1,2,2,7,2,2,1,1,2,1,2,1,1,1,1,1,
  1,1,1,1,1,1,1,1,1,2,1,1,1,1,1,1,1,1,1,1,1,1,2,2,2,2,2,2,2,2,2,1,1,1,2,1,1,
  1,1,1,1,1,1,1,1,1,1,1,2,2,7,2,2,1,1,2,1,2,1,1,1,1,1,1,1,1,1,1,1,1,1,1,2,1,
  1,1,1,1,1,1,1,1,1,1,1,1,7,1,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,
  5,5,1,5,7,1,1,1,3,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,1,7,1,
  1,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,1,5,7,1,1,1,3,8,8,8,8,
  8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,1,7,1,1,8,8,8,8,8,8,8,8,8,8,8,8,
  8,8,8,8,8,8,8,8,8,8,8,8,8,1,7,1,1,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,
  8,8,8,8,8,1,7,1,1,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,1,7,1,
  1,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,1,7,1,1,8,8,8,8,
  8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,1,7,1,1,5,5,5,5,5,5,5,5,5,
  5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,1,5,7,1,1,1,3,8,8,8,8,8,8,8,8,8,8,8,
  8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,1,7,1,1,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,
  5,5,5,5,5,5,5,5,5,5,5,5,1,5,7,1,1,1,3,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,
  8,8,8,8,8,8,8,8,8,8,1,7,1,1,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,
  5,5,5,5,5,1,5,7,1,1,1,3,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,
  8,8,8,1,7,1,1,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,1,5,
  7,1,1,1,3,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,1,7,1,1,
  5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,1,5,7,1,1,1,3,8,8,
  8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,1,7,1,1,8,8,8,8,8,8,8,
  8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,1,7,1,1,5,5,5,5,5,5,5,5,5,5,5,5,
  5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,1,5,7,1,1,1,3,8,8,8,8,8,8,8,8,8,8,8,8,8,8,
  8,8,8,8,8,8,8,8,8,8,8,8,8,8,1,7,1,1,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,
  8,8,8,8,8,8,8,8,8,1,7,1,1,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,
  5,5,5,5,1,5,7,1,1,1,3,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,
  8,8,1,7,1,1,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,1,7,1,
  1,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,1,1,7,1,1,1,3,5,
  5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,1,1,7,1,1,1,3,5,5,5,
  5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,1,1,7,1,1,1,3,5,5,5,5,5,
  5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,1,1,7,1,1,1,3,5,5,5,5,5,5,5,
  5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,1,1,7,1,1,1,3,5,5,5,5,5,5,5,5,5,
  5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,1,1,7,1,1,1,3,5,5,5,5,5,5,5,5,5,5,5,
  5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,1,1,7,1,1,1,3,5,5,5,5,5,5,5,5,5,5,5,5,5,
  5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,1,1,7,1,1,1,3,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,
  5,5,5,5,5,5,5,5,5,5,5,5,5,1,1,7,1,1,1,3,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,
  5,5,5,5,5,5,5,5,5,5,5,1,1,7,1,1,1,3,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,
  5,5,5,5,5,5,5,5,5,1,1,7,1,1,1,3,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,
  5,5,5,5,5,5,5,1,1,7,1,1,1,3,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,
  5,5,5,5,5,1,1,7,1,1,1,3,1,7,1,2,1,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,
  2,2,2,2,2,2,2,2,2,2,2,2,2,7,1,1,4,4,4,4,7,1,1,4,4,4,4,7,1,1,4,4,4,4,7,1,1,
  4,4,4,4,7,1,1,7,1,1,7,1,1,7,1,2,7,1,7,2,1,7,2,1,7,2,1,7,2,1,7,2,1,7,2,1,7,
  2,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,
  5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,
  5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,
  5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,1,7,1,3,2,2,2,2,2,2,2,2,2,1,1,1,2,1,1,
  1,1,1,1,1,1,1,1,2,2,7,2,2,1,1,2,1,2,1,1,2,2,2,1,1,1,1,1,1,1,1,1,1,1,2,2,2,
  2,2,2,2,2,2,1,1,1,2,1,1,1,1,1,1,1,1,1,1,2,2,7,2,2,1,1,2,1,2,1,1,2,2,2,1,1,
  1,1,1,1,1,1,1,1,1,2,2,2,2,2,2,2,2,2,1,1,1,2,1,1,1,1,1,1,1,1,1,1,2,2,7,2,2,
  1,1,2,1,2,1,1,2,2,2,1,1,1,1,1,1,1,1,1,1,1,2,2,2,2,2,2,2,2,2,1,1,1,2,1,1,1,
  1,1,1,1,1,1,1,2,2,7,2,2,1,1,2,1,2,1,1,2,2,2,1,1,1,1,1,1,1,1,1,1,1,2,2,2,2,
  2,2,2,2,2,1,1,1,2,1,1,1,1,1,1,1,1,1,1,2,2,7,2,2,1,1,2,1,2,1,1,2,2,2,1,1,1,
  1,1,1,1,1,1,1,1,2,2,2,2,2,2,2,2,2,1,1,1,2,1,1,1,1,1,1,1,1,1,1,1,1,1,2,2,7,
  2,2,1,1,2,1,2,1,1,1,1,1,1,1,1,2,1,1,1,1,1,1,1,1,1,1,1,1,2,2,2,2,2,2,2,2,2,
  1,1,1,2,1,1,1,1,1,1,1,1,1,1,1,1,1,2,2,7,2,2,1,1,2,1,2,1,1,1,1,1,1,1,1,2,1,
  1,1,1,1,1,1,1,1,1,1,1,2,2,2,2,2,2,2,2,2,1,1,1,2,1,1,1,1,1,1,1,1,1,1,1,1,1,
  2,2,7,2,2,1,1,2,1,2,1,1,1,1,1,1,1,1,1,2,1,1,1,1,1,1,1,1,1,1,1,1,2,2,2,2,2,
  2,2,2,2,1,1,1,2,1,1,1,1,1,1,1,1,1,1,1,1,1,2,2,7,2,2,1,1,2,1,2,1,1,1,1,1,1,
  1,1,1,2,1,1,1,1,1,1,1,1,1,1,1,1,2,2,2,2,2,2,2,2,2,1,1,1,2,1,1,1,1,1,1,1,1,
  1,1,1,1,1,2,2,7,2,2,1,1,2,1,2,1,1,1,1,1,1,1,1,1,2,1,1,1,1,1,1,1,1,1,1,1,1,
  2,2,2,2,2,2,2,2,2,1,1,1,2,1,1,1,1,1,1,1,1,1,1,1,1,1,2,2,7,2,2,1,1,2,1,2,1,
  1,1,1,1,1,1,1,1,2,1,1,1,1,1,1,1,1,1,1,1,1,2,2,2,2,2,2,2,2,2,1,1,1,2,1,1,1,
  1,1,1,1,1,1,1,1,1,1,2,2,7,2,2,1,1,2,1,2,1,1,1,1,1,1,1,1,1,1,2,1,1,1,1,1,1,
  1,1,1,1,1,1,2,2,2,2,2,2,2,2,2,1,1,1,2,1,1,1,1,1,1,1,1,1,1,1,1,1,2,2,7,2,2,
  1,1,2,1,2,1,1,1,1,1,1,1,1,1,1,2,1,1,1,1,1,1,1,1,1,1,1,1,2,2,2,2,2,2,2,2,2,
  1,1,1,2,1,1,1,1,1,1,1,1,1,1,1,1,1,2,2,7,2,2,1,1,2,1,2,1,1,1,1,1,1,1,1,1,1,
  1,2,1,1,1,1,1,1,1,1,1,1,1,1,2,2,2,2,2,2,2,2,2,1,1,1,2,1,1,1,1,1,1,1,1,1,1,
  1,1,1,2,2,7,2,2,1,1,2,1,2,1,1,1,1,1,1,1,1,1,1,1,2,1,1,1,1,1,1,1,1,1,1,1,1,
  2,2,2,2,2,2,2,2,2,1,1,1,2,1,1,1,1,1,1,1,1,1,1,1,1,1,2,2,7,2,2,1,1,2,1,2,1,
  1,1,1,1,1,1,1,1,1,1,1,2,1,1,1,1,1,1,1,1,1,1,1,1,2,2,2,2,2,2,2,2,2,1,1,1,2,
  1,1,1,1,1,1,1,1,1,1,1,1,1,2,2,7,2,2,1,1,2,1,2,1,1,1,1,1,1,1,1,1,1,1,1,2,1,
  1,1,1,1,1,1,1,1,1,1,1,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,
  5,5,5,5,5,5,5,5,5,5,5,1,5,7,1,1,1,3,8,8,8,8,8,8,8,8,8,1,7,1,1,4,4,4,4,4,4,
  4,4,4,4,4,4,4,4,2,4,4,4,4,4,4,4,4,4,4,4,4,4,4,4,4,4,4,4,4,7,8,8,8,8,8,8,8,
  8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,1,7,1,1,8,8,8,8,8,8,8,8,8,8,8,
  8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,1,7,1,1,8,8,8,8,8,8,8,8,1,7,1,1,8,8,8,8,
  8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,1,7,1,1,4,4,4,4,4,4,4,4,4,
  4,4,4,4,4,4,4,4,4,4,4,4,4,4,4,4,4,4,4,4,4,4,4,4,4,4,4,4,4,4,4,4,4,4,4,4,4,
  4,4,4,4,4,4,4,4,4,4,4,4,4,4,4,4,4,4,4,4,4,4,4,4,4,4,4,4,4,4,4,4,4,4,4,4,4,
  4,4,4,4,4,4,4,4,4,4,1,1,1,1,1,4,4,4,4,4,4,4,4,4,4,4,4,4,4,4,4,4,4,4,4,4,4,
  4,4,4,4,4,4,4,4,4,7,1,1,1,1,1,4,4,4,4,4,4,4,4,4,4,4,4,4,4,4,4,4,4,4,4,4,4,
  4,4,4,4,4,4,4,4,4,4,4,4,4,4,4,4,4,4,4,4,4,4,4,4,4,4,4,4,4,4,4,4,4,4,4,4,4,
  4,4,4,4,4,4,4,4,4,4,4,4,4,4,4,4,4,4,4,4,4,4,4,4,4,4,4,4,4,4,4,4,4,4,1,1,1,
  1,1,4,4,4,4,4,4,4,4,4,4,4,4,4,4,4,4,4,4,4,4,4,4,4,4,4,4,4,4,4,4,4,7,1,1,1,
  1,1,4,4,4,4,4,4,4,4,4,4,4,4,4,4,4,4,4,4,4,4,4,4,4,4,4,4,4,4,4,4,4,4,4,4,4,
  4,4,4,4,4,4,4,4,4,4,4,4,4,4,4,4,4,4,4,4,4,4,4,4,4,4,4,4,4,4,4,4,4,4,4,4,4,
  4,4,4,4,4,4,4,4,4,4,4,4,4,4,4,4,4,4,4,4,4,1,1,4,4,4,4,4,4,4,4,4,4,4,4,4,4,
  4,4,4,4,4,4,4,4,4,4,4,4,4,4,4,7,1,1,4,4,4,4,4,4,4,4,4,4,4,4,4,4,4,4,4,4,4,
  4,4,4,4,4,4,4,4,4,4,4,4,4,4,4,4,4,4,4,4,4,4,4,4,4,4,4,4,4,4,4,4,4,4,4,4,4,
  4,4,4,4,4,4,4,4,4,4,4,4,4,4,4,4,4,4,4,4,4,4,4,4,4,4,4,4,4,4,4,4,4,4,4,4,4,
  1,1,4,4,4,4,4,4,4,4,4,4,4,4,4,4,4,4,4,4,4,4,4,4,4,4,4,4,4,4,4,7,1,1,4,4,4,
  4,4,4,4,4,4,4,4,4,4,4,4,4,4,4,4,4,4,4,4,4,4,4,4,4,4,4,4,4,4,4,4,4,4,4,4,4,
  4,4,4,4,4,4,4,4,4,4,4,4,4,4,4,4,4,4,4,4,4,4,4,4,4,4,4,4,4,4,4,4,4,4,4,4,4,
  4,4,4,4,4,4,4,4,4,4,4,4,4,4,4,4,1,1,4,4,4,4,4,4,4,4,4,4,4,4,4,4,4,4,4,4,4,
  4,4,4,4,4,4,4,4,4,4,7,1,1,4,4,4,4,4,4,4,4,4,4,4,4,4,4,4,4,4,4,4,4,4,4,4,4,
  4,4,4,4,4,4,4,4,4,4,4,4,4,4,4,4,4,4,4,4,4,4,4,4,4,4,4,4,4,4,4,4,4,4,4,4,4,
  4,4,4,4,4,4,4,4,4,4,4,4,4,4,4,4,4,4,4,4,4,4,4,4,4,4,4,4,4,4,4,4,1,1,4,4,4,
  4,4,4,4,4,4,4,4,4,4,4,4,4,4,4,4,4,4,4,4,4,4,4,4,4,4,7,1,1,4,4,4,4,4,4,4,4,
  4,4,4,4,4,4,4,4,4,4,4,4,4,4,4,4,4,4,4,4,4,4,4,4,4,4,4,4,4,4,4,4,4,4,4,4,4,
  4,4,4,4,4,4,4,4,4,4,4,4,4,4,4,4,4,4,4,4,4,4,4,4,4,4,4,4,4,4,4,4,4,4,4,4,4,
  4,4,4,4,4,4,4,4,4,4,4,1,1,1,1,4,4,4,4,4,4,4,4,4,4,4,4,4,4,4,4,4,4,4,4,4,4,
  4,4,4,7,1,1,1,1,4,4,4,4,4,4,4,4,4,4,4,4,4,4,4,4,4,4,4,4,4,4,4,4,4,4,4,4,4,
  4,4,4,4,4,4,4,4,4,4,4,4,4,4,4,4,4,4,4,4,4,4,4,4,4,4,4,4,4,4,4,4,4,4,4,4,4,
  4,4,4,4,4,4,4,4,4,4,4,4,4,4,4,4,4,4,4,4,4,4,4,4,4,4,4,1,1,1,1,4,4,4,4,4,4,
  4,4,4,4,4,4,4,4,4,4,4,4,4,4,4,4,4,4,4,7,1,1,1,1,4,4,4,4,4,4,4,4,4,4,4,4,4,
  4,4,4,4,4,4,4,4,4,4,4,4,4,4,4,4,4,4,4,4,4,4,4,4,4,4,4,4,4,4,4,4,4,4,4,4,4,
  4,4,4,4,4,4,4,4,4,4,4,4,4,4,4,4,4,4,4,4,4,4,4,4,4,4,4,4,4,4,4,4,4,4,4,4,4,
  4,4,4,4,4,4,1,1,4,4,4,4,4,4,4,4,4,4,4,4,4,4,4,4,4,4,4,4,4,4,4,7,1,1,4,4,4,
  4,4,4,4,4,4,4,4,4,4,4,4,4,4,4,4,4,4,4,4,4,4,4,4,4,4,4,4,4,4,4,4,4,4,4,4,4,
  4,4,4,4,4,4,4,4,4,4,4,4,4,4,4,4,4,4,4,4,4,4,4,4,4,4,4,4,4,4,4,4,4,4,4,4,4,
  4,4,4,4,4,4,4,4,4,4,4,4,4,4,4,4,1,1,4,4,4,4,4,4,4,4,4,4,4,4,4,4,4,4,4,4,4,
  4,4,4,4,7,1,1,4,4,4,4,4,4,4,4,4,4,4,4,4,4,4,4,4,4,4,4,4,4,4,4,4,4,4,4,4,4,
  4,4,4,4,4,4,4,4,4,4,4,4,4,4,4,4,4,4,4,4,4,4,4,4,4,4,4,4,4,4,4,4,4,4,4,4,4,
  4,4,4,4,4,4,4,4,4,4,4,4,4,4,4,4,4,4,4,4,4,4,4,4,4,4,1,1,4,4,4,4,4,4,4,4,4,
  4,4,4,4,4,4,4,4,4,4,4,4,7,1,1,4,4,4,4,4,4,4,4,4,4,4,4,4,4,4,4,4,4,4,4,4,4,
  4,4,4,4,4,4,4,4,4,4,4,4,4,4,4,4,4,4,4,4,4,4,4,4,4,4,4,4,4,4,4,4,4,4,4,4,4,
  4,4,4,4,4,4,4,4,4,4,4,4,4,4,4,4,4,4,4,4,4,4,4,4,4,4,4,4,4,4,4,4,4,4,1,1,4,
  4,4,4,4,4,4,4,4,4,4,4,4,4,4,4,4,4,4,4,4,7,1,1,2,2,2,2,2,2,2,2,2,7,2,1,1,2,
  2,2,2,2,2,2,2,2,1,1,1,2,2,1,1,1,1,1,1,1,1,1,1,1,1,1,2,2,7,2,2,1,1,2,1,2,1,
  2,1,1,2,2,1,1,1,1,1,1,1,1,1,1,1,1,2,1,1,1,1,1,1,1,1,1,1,1,1,1,1,2,2,2,2,2,
  2,2,2,2,1,1,1,2,1,1,1,1,1,1,1,1,1,1,1,1,1,2,2,7,2,2,1,1,2,1,2,1,2,1,1,1,1,
  1,1,1,1,1,1,1,1,2,1,1,1,1,1,1,1,1,1,1,1,1,2,2,2,2,2,2,2,2,7,2,1,2,2,2,2,2,
  2,2,2,2,1,1,1,2,1,1,1,1,1,1,1,1,1,1,1,1,1,2,2,7,2,2,1,1,2,1,2,1,2,1,1,1,1,
  1,1,1,1,1,1,1,1,2,1,1,1,1,1,1,1,1,1,1,1,1
};


static const unsigned short ag_pstt[] = {
3,3,3,3,3,3,3,3,3,3,3,3,3,3,3,3,3,3,3,3,3,3,3,3,3,3,3,3,3,3,3,3,3,3,3,3,3,3,
  3,3,3,3,3,3,3,3,3,3,3,3,3,3,3,3,3,3,3,3,3,3,3,3,3,3,3,3,3,3,3,3,3,3,3,3,
  3,3,3,3,3,3,3,3,3,3,3,3,3,3,3,3,3,3,3,3,3,3,3,3,3,3,3,3,3,3,3,3,3,3,3,3,
  3,3,3,3,3,3,3,3,3,3,3,3,3,3,3,3,3,3,3,3,3,3,3,3,1,3,0,2,2,2,3,
4,4,4,4,4,4,4,4,4,4,4,4,4,4,4,4,4,4,4,4,4,4,4,4,4,4,4,4,4,4,4,4,4,4,4,1,4,
1,543,545,543,543,
7,7,7,7,7,7,7,7,7,7,7,7,7,7,7,7,7,7,7,7,7,7,7,7,7,7,7,7,7,7,7,7,7,7,7,7,7,7,
  7,7,7,7,7,7,7,7,7,7,7,7,7,7,7,7,7,7,7,7,7,7,7,7,7,7,7,7,7,7,7,7,7,7,7,7,
  7,7,7,7,7,7,7,7,7,7,7,7,7,7,7,7,7,7,7,7,7,7,7,7,7,7,7,7,7,7,7,7,7,7,7,7,
  7,7,7,7,7,7,7,7,7,7,7,7,7,7,7,7,7,7,7,7,7,7,7,7,6,7,5,3,0,8,8,8,5,7,
13,13,13,13,13,13,13,13,13,13,13,13,13,13,13,13,13,13,13,13,13,13,13,13,13,
  13,13,13,13,13,13,13,13,13,14,13,4,
16,18,
9,6,24,
83,83,83,83,83,83,83,83,27,27,27,27,27,27,27,27,27,27,27,27,27,27,27,27,27,
  27,27,27,27,27,27,27,27,27,27,27,27,27,27,27,27,27,27,27,27,27,27,27,27,
  27,27,27,27,27,27,27,27,27,27,27,27,27,27,27,27,27,27,27,27,27,27,27,27,
  27,27,27,27,27,27,27,27,27,27,27,27,27,27,27,27,27,27,27,27,27,27,27,27,
  27,27,27,27,27,27,27,27,27,27,27,27,27,27,27,27,27,27,27,27,27,27,27,17,
  18,19,20,21,22,23,24,25,22,10,11,12,82,1,7,23,28,36,21,15,22,14,13,37,
  35,34,33,32,31,35,36,30,29,27,16,26,16,
7,7,7,7,7,7,7,7,7,7,7,7,7,7,7,7,7,7,7,7,7,7,7,7,7,7,7,7,7,7,7,7,7,7,7,7,7,7,
  7,7,7,7,7,7,7,7,7,7,7,7,7,7,7,7,7,7,7,7,7,7,7,7,7,7,7,7,7,7,7,7,7,7,7,7,
  7,7,7,7,7,7,7,7,7,7,7,7,7,7,7,7,7,7,7,7,7,7,7,7,7,7,7,7,7,7,7,7,7,7,7,7,
  7,7,7,7,7,7,7,7,7,7,7,7,7,7,7,7,7,7,7,7,7,7,7,7,6,7,4,5,8,3,3,5,7,
544,544,544,544,544,544,544,544,544,544,544,544,544,544,544,544,544,544,544,
  544,544,544,544,544,544,544,544,544,544,544,544,544,544,544,544,544,544,
  544,544,544,544,544,544,544,544,544,544,544,544,544,544,544,544,544,544,
  544,544,544,544,544,544,544,544,544,544,544,544,544,544,544,544,544,544,
  544,544,544,544,544,544,544,544,544,544,544,544,544,544,544,544,544,544,
  544,544,544,544,544,544,544,544,544,544,544,544,544,544,544,544,544,544,
  544,544,544,544,544,544,544,544,544,544,544,544,544,544,544,544,544,544,
  544,544,544,544,544,544,544,1,544,544,9,2,2,2,549,
544,544,544,544,544,544,544,544,544,544,544,544,544,544,544,544,544,544,544,
  544,544,544,544,544,544,544,544,544,544,544,544,544,544,544,544,544,544,
  544,544,544,544,544,544,544,1,544,10,2,2,2,548,
544,544,544,544,544,544,544,544,544,544,544,544,544,544,544,544,544,544,544,
  544,544,544,544,544,544,544,544,544,544,544,544,544,544,544,1,544,11,2,
  2,2,547,
544,544,544,544,544,544,544,544,544,544,544,544,544,544,544,544,544,544,544,
  544,544,544,544,544,544,544,544,544,544,544,544,544,544,544,1,544,12,2,
  2,2,546,
38,41,43,44,45,46,47,48,49,50,51,52,53,54,56,62,65,66,68,61,60,39,67,64,55,
  59,57,58,63,42,40,70,10,11,69,13,69,70,69,69,69,69,69,69,69,69,69,69,69,
  69,69,69,69,69,69,69,69,69,69,69,69,69,69,69,69,69,69,69,69,69,69,
38,41,43,44,45,46,47,48,49,50,51,52,53,54,56,62,65,66,68,61,60,39,67,64,55,
  59,57,58,63,42,40,71,10,11,69,14,69,71,69,69,69,69,69,69,69,69,69,69,69,
  69,69,69,69,69,69,69,69,69,69,69,69,69,69,69,69,69,69,69,69,69,69,
38,41,43,44,45,46,47,48,49,50,51,52,53,54,56,62,65,66,68,61,60,39,67,64,55,
  59,57,58,63,42,40,72,10,11,69,15,69,72,69,69,69,69,69,69,69,69,69,69,69,
  69,69,69,69,69,69,69,69,69,69,69,69,69,69,69,69,69,69,69,69,69,69,
84,84,84,84,84,84,84,85,85,85,84,101,1,2,16,2,85,2,2,559,
544,544,544,544,544,544,544,544,544,1,2,17,2,2,2,565,
544,544,544,544,544,544,544,544,544,1,2,18,2,2,2,564,
544,544,544,544,1,2,19,2,2,2,563,
544,544,544,544,1,2,20,2,2,2,562,
544,544,544,544,544,544,544,544,544,1,2,21,2,2,2,561,
544,544,544,544,544,544,544,544,544,1,2,22,2,2,2,560,
544,544,1,2,23,2,2,2,557,
544,544,1,2,24,2,2,2,555,
544,544,544,544,544,544,544,544,544,544,544,544,544,544,544,1,2,25,2,2,2,
  552,
544,544,544,544,544,544,544,544,544,544,544,544,544,544,544,544,544,544,544,
  544,544,544,544,544,544,544,544,544,544,544,544,544,544,544,544,544,544,
  544,544,544,544,544,544,544,544,544,544,544,544,544,544,544,544,544,544,
  544,544,544,544,544,544,544,544,544,544,544,544,544,544,544,544,544,544,
  544,544,544,544,544,544,544,544,544,544,544,544,544,544,544,544,544,544,
  544,544,544,544,544,544,544,544,544,544,544,544,544,544,544,544,544,544,
  544,544,544,544,544,544,544,544,544,1,2,26,2,2,2,550,
73,74,75,76,77,78,79,80,81,82,83,84,85,86,87,88,89,90,91,92,93,94,95,96,97,
  98,99,100,101,102,103,104,105,106,107,108,109,110,111,112,113,114,115,
  116,117,118,119,120,121,122,123,124,125,126,127,128,129,130,131,132,133,
  134,135,136,137,138,139,140,141,142,143,144,145,146,147,148,149,150,151,
  152,153,154,155,156,157,158,159,160,161,162,163,224,226,228,229,231,233,
  234,235,237,239,241,243,245,247,249,251,253,255,256,257,258,27,259,46,
  47,48,254,252,250,248,246,244,242,240,238,236,63,64,232,230,69,227,225,
  223,223,222,221,220,219,218,217,225,216,215,214,229,230,213,212,211,210,
  209,208,207,206,239,205,204,203,243,244,245,246,202,201,200,199,198,197,
  196,195,194,193,192,191,190,189,188,187,186,185,184,183,182,268,181,180,
  179,272,178,177,176,276,175,174,279,280,281,282,283,284,285,286,287,288,
  289,290,291,292,173,294,172,171,170,298,299,300,169,168,303,167,166,306,
  165,164,309,
249,251,253,263,265,28,266,264,262,261,260,
83,83,83,83,83,83,83,83,82,29,38,267,267,
83,83,83,83,83,83,83,83,82,30,37,267,267,
83,83,83,83,83,83,83,83,82,31,34,267,267,
83,83,83,83,83,83,83,83,82,32,33,267,267,
268,269,33,31,32,
97,92,34,29,272,30,270,273,271,
86,86,86,86,86,86,86,86,97,86,86,86,86,92,86,35,26,272,27,270,28,273,271,
  274,
44,44,44,44,44,44,44,44,44,44,44,44,44,44,44,44,44,44,44,44,44,44,44,44,44,
  44,44,44,44,44,44,44,44,44,44,44,44,44,44,44,44,44,44,44,44,44,44,44,44,
  44,44,44,44,44,44,44,44,44,44,44,44,44,44,44,44,44,44,44,44,44,44,44,44,
  44,44,44,44,44,44,44,44,44,44,44,44,44,44,44,44,44,44,44,44,44,44,44,44,
  44,44,44,44,44,44,44,44,44,44,44,44,44,44,44,263,265,25,25,25,25,36,276,
  275,
9,10,11,12,37,20,15,14,13,19,
544,544,544,544,544,544,544,544,544,544,544,544,544,544,544,544,544,544,544,
  544,544,544,544,544,544,544,544,544,544,544,544,544,544,544,1,544,38,2,
  2,2,751,
544,544,544,544,544,544,544,544,544,544,544,544,544,544,544,544,544,544,544,
  544,544,544,544,544,544,544,544,544,544,544,544,544,544,544,544,544,544,
  544,544,544,544,544,544,1,544,39,2,2,2,624,
544,544,544,544,544,544,544,544,544,544,544,544,544,544,544,544,544,544,544,
  544,544,544,544,544,544,544,544,544,544,544,544,544,544,544,544,544,544,
  544,544,544,544,544,544,544,1,544,40,2,2,2,605,
544,544,544,544,544,544,544,544,544,544,544,544,544,544,544,544,544,544,544,
  544,544,544,544,544,544,544,544,544,544,544,544,544,544,544,1,544,41,2,
  2,2,750,
544,544,544,544,544,544,544,544,544,544,544,544,544,544,544,544,544,544,544,
  544,544,544,544,544,544,544,544,544,544,544,544,544,544,544,544,544,544,
  544,544,544,544,544,544,544,1,544,42,2,2,2,607,
544,544,544,544,544,544,544,544,544,544,544,544,544,544,544,544,544,544,544,
  544,544,544,544,544,544,544,544,544,544,544,544,544,544,544,1,544,43,2,
  2,2,749,
544,544,544,544,544,544,544,544,544,544,544,544,544,544,544,544,544,544,544,
  544,544,544,544,544,544,544,544,544,544,544,544,544,544,544,1,544,44,2,
  2,2,748,
544,544,544,544,544,544,544,544,544,544,544,544,544,544,544,544,544,544,544,
  544,544,544,544,544,544,544,544,544,544,544,544,544,544,544,1,544,45,2,
  2,2,747,
544,544,544,544,544,544,544,544,544,544,544,544,544,544,544,544,544,544,544,
  544,544,544,544,544,544,544,544,544,544,544,544,544,544,544,1,544,46,2,
  2,2,746,
544,544,544,544,544,544,544,544,544,544,544,544,544,544,544,544,544,544,544,
  544,544,544,544,544,544,544,544,544,544,544,544,544,544,544,1,544,47,2,
  2,2,745,
544,544,544,544,544,544,544,544,544,544,544,544,544,544,544,544,544,544,544,
  544,544,544,544,544,544,544,544,544,544,544,544,544,544,544,1,544,48,2,
  2,2,744,
544,544,544,544,544,544,544,544,544,544,544,544,544,544,544,544,544,544,544,
  544,544,544,544,544,544,544,544,544,544,544,544,544,544,544,1,544,49,2,
  2,2,743,
544,544,544,544,544,544,544,544,544,544,544,544,544,544,544,544,544,544,544,
  544,544,544,544,544,544,544,544,544,544,544,544,544,544,544,1,544,50,2,
  2,2,742,
544,544,544,544,544,544,544,544,544,544,544,544,544,544,544,544,544,544,544,
  544,544,544,544,544,544,544,544,544,544,544,544,544,544,544,1,544,51,2,
  2,2,741,
544,544,544,544,544,544,544,544,544,544,544,544,544,544,544,544,544,544,544,
  544,544,544,544,544,544,544,544,544,544,544,544,544,544,544,1,544,52,2,
  2,2,740,
544,544,544,544,544,544,544,544,544,544,544,544,544,544,544,544,544,544,544,
  544,544,544,544,544,544,544,544,544,544,544,544,544,544,544,544,544,544,
  544,544,544,544,544,544,544,544,544,544,544,544,544,544,544,544,544,544,
  544,544,544,544,544,544,544,544,544,544,544,544,544,544,544,544,544,544,
  544,544,544,544,544,544,544,544,544,544,544,544,544,544,544,544,544,544,
  544,544,544,544,544,544,544,544,544,544,544,544,544,544,544,544,544,544,
  544,544,544,544,544,544,544,544,544,544,544,544,544,544,544,544,544,1,
  544,53,2,2,2,739,
544,544,544,544,544,544,544,544,544,544,544,544,544,544,544,544,544,544,544,
  544,544,544,544,544,544,544,544,544,544,544,544,544,544,544,544,544,544,
  544,544,544,544,544,544,544,544,544,544,544,544,544,544,544,544,544,544,
  544,544,544,544,544,544,544,544,544,544,544,544,544,544,544,544,544,544,
  544,544,544,544,544,544,544,544,544,544,544,544,544,544,544,544,544,544,
  544,544,544,544,544,544,544,544,544,544,544,544,544,544,544,544,544,544,
  544,544,544,544,544,544,544,544,544,544,544,544,544,544,544,544,544,1,
  544,54,2,2,2,738,
544,544,544,544,544,544,544,544,544,544,544,544,544,544,544,544,544,544,544,
  544,544,544,544,544,544,544,544,544,544,544,544,544,544,544,544,544,544,
  544,544,544,544,544,544,1,544,55,2,2,2,618,
544,544,544,544,544,544,544,544,544,544,544,544,544,544,544,544,544,544,544,
  544,544,544,544,544,544,544,544,544,544,544,544,544,544,544,1,544,56,2,
  2,2,737,
544,544,544,544,544,544,544,544,544,544,544,544,544,544,544,544,544,544,544,
  544,544,544,544,544,544,544,544,544,544,544,544,544,544,544,544,544,544,
  544,544,544,544,544,544,544,1,544,57,2,2,2,616,
544,544,544,544,544,544,544,544,544,544,544,544,544,544,544,544,544,544,544,
  544,544,544,544,544,544,544,544,544,544,544,544,544,544,544,544,544,544,
  544,544,544,544,544,544,544,1,544,58,2,2,2,615,
544,544,544,544,544,544,544,544,544,544,544,544,544,544,544,544,544,544,544,
  544,544,544,544,544,544,544,544,544,544,544,544,544,544,544,544,544,544,
  544,544,544,544,544,544,1,544,59,2,2,2,617,
544,544,544,544,544,544,544,544,544,544,544,544,544,544,544,544,544,544,544,
  544,544,544,544,544,544,544,544,544,544,544,544,544,544,544,544,544,544,
  544,544,544,544,544,544,544,544,544,544,544,544,544,544,544,544,544,544,
  544,544,544,544,544,544,544,544,544,544,544,544,544,544,544,544,544,544,
  544,544,544,544,544,544,544,544,544,544,544,544,544,544,544,544,544,544,
  544,544,544,544,544,544,544,544,544,544,544,544,544,544,544,544,544,544,
  544,544,544,544,544,544,544,544,544,544,544,544,544,544,544,544,544,544,
  544,544,544,544,544,544,544,544,544,544,544,544,544,544,544,544,544,544,
  544,544,544,544,1,544,60,2,2,2,625,
544,544,544,544,544,544,544,544,544,544,544,544,544,544,544,544,544,544,544,
  544,544,544,544,544,544,544,544,544,544,544,544,544,544,544,544,544,544,
  544,544,544,544,544,544,544,1,544,61,2,2,2,626,
544,544,544,544,544,544,544,544,544,544,544,544,544,544,544,544,544,544,544,
  544,544,544,544,544,544,544,544,544,544,544,544,544,544,544,1,544,62,2,
  2,2,736,
544,544,544,544,544,544,544,544,544,544,544,544,544,544,544,544,544,544,544,
  544,544,544,544,544,544,544,544,544,544,544,544,544,544,544,544,544,544,
  544,544,544,544,544,544,544,1,544,63,2,2,2,609,
544,544,544,544,544,544,544,544,544,544,544,544,544,544,544,544,544,544,544,
  544,544,544,544,544,544,544,544,544,544,544,544,544,544,544,544,544,544,
  544,544,544,544,544,544,1,544,64,2,2,2,619,
544,544,544,544,544,544,544,544,544,544,544,544,544,544,544,544,544,544,544,
  544,544,544,544,544,544,544,544,544,544,544,544,544,544,544,1,544,65,2,
  2,2,735,
544,544,544,544,544,544,544,544,544,544,544,544,544,544,544,544,544,544,544,
  544,544,544,544,544,544,544,544,544,544,544,544,544,544,544,1,544,66,2,
  2,2,734,
544,544,544,544,544,544,544,544,544,544,544,544,544,544,544,544,544,544,544,
  544,544,544,544,544,544,544,544,544,544,544,544,544,544,544,544,544,544,
  544,544,544,544,544,544,544,544,544,544,544,544,544,544,544,544,544,544,
  544,544,544,544,544,544,544,544,544,544,544,544,544,544,544,544,544,544,
  544,544,544,544,544,544,544,544,544,544,544,544,544,544,544,544,544,544,
  544,544,544,544,544,544,544,544,544,544,544,544,544,544,544,544,544,544,
  544,544,544,544,544,544,544,544,544,544,544,544,544,544,544,544,544,544,
  544,544,544,544,544,544,544,544,1,544,67,2,2,2,622,
544,544,544,544,544,544,544,544,544,544,544,544,544,544,544,544,544,544,544,
  544,544,544,544,544,544,544,544,544,544,544,544,544,544,544,1,544,68,2,
  2,2,733,
38,41,43,44,45,46,47,48,49,50,51,52,53,54,56,62,65,66,68,61,60,39,67,64,55,
  59,57,58,63,42,40,10,11,6,8,6,6,6,6,6,6,6,6,6,6,6,6,6,6,6,6,6,6,6,6,6,6,
  6,6,6,6,6,6,6,6,6,6,6,
11,70,11,
10,71,10,
9,72,9,
544,544,544,544,544,544,544,544,544,544,544,544,544,544,544,544,544,544,544,
  544,544,544,544,544,544,544,544,544,544,544,544,544,544,544,544,544,544,
  544,544,544,544,544,544,544,544,544,544,544,544,544,544,544,544,544,544,
  544,544,544,544,544,544,544,544,544,544,544,544,544,544,544,544,544,544,
  544,544,544,544,544,544,544,544,544,544,544,544,544,544,544,544,544,544,
  544,544,544,544,544,1,544,73,2,2,2,732,
544,544,544,544,544,544,544,544,544,544,544,544,544,544,544,544,544,544,544,
  544,544,544,544,544,544,544,544,544,1,544,74,2,2,2,731,
544,544,544,544,544,544,544,544,1,544,75,2,2,2,730,
544,544,544,544,544,544,544,544,544,544,544,544,544,544,544,544,544,544,544,
  544,544,544,544,544,544,544,544,544,544,544,544,544,544,544,544,544,544,
  544,544,544,544,544,544,544,544,544,544,544,544,544,544,544,544,544,544,
  544,544,544,544,544,544,544,544,544,544,544,544,544,544,544,544,544,544,
  544,544,544,544,544,544,544,544,544,544,544,544,544,544,544,544,544,544,
  544,544,544,544,544,1,544,76,2,2,2,729,
544,544,544,544,544,544,544,544,544,544,544,544,544,544,544,544,544,544,544,
  544,544,544,544,544,544,544,544,544,1,544,77,2,2,2,728,
544,544,544,544,544,544,544,544,1,544,78,2,2,2,727,
544,544,544,544,544,544,544,544,544,544,544,544,544,544,544,544,544,544,544,
  544,544,544,544,544,544,544,544,544,544,544,544,544,544,544,544,544,544,
  544,544,544,544,544,544,544,544,544,544,544,544,544,544,544,544,544,544,
  544,544,544,544,544,544,544,544,544,544,544,544,544,544,544,544,544,544,
  544,544,544,544,544,544,544,544,544,544,544,544,544,544,544,544,544,544,
  544,544,544,544,544,1,544,79,2,2,2,726,
544,544,1,544,80,2,2,2,725,
544,544,544,544,544,544,544,544,544,544,544,544,544,544,544,544,544,544,544,
  544,544,544,544,544,544,544,544,544,1,544,81,2,2,2,724,
544,544,544,544,544,544,544,544,544,544,544,544,544,544,544,544,544,544,544,
  544,544,544,544,544,544,544,544,544,544,544,544,544,544,544,544,544,544,
  544,544,544,544,544,544,544,544,544,544,544,544,544,544,544,544,544,544,
  544,544,544,544,544,544,544,544,544,544,544,544,544,544,544,544,544,544,
  544,544,544,544,544,544,544,544,544,544,544,544,544,544,544,544,544,544,
  544,544,544,544,544,1,544,82,2,2,2,723,
544,544,544,544,544,544,544,544,544,544,544,544,544,544,544,544,544,544,544,
  544,544,544,544,544,544,544,544,544,544,544,544,544,544,544,544,544,544,
  544,544,544,544,544,544,544,544,544,544,544,544,544,544,544,544,544,544,
  544,544,544,544,544,544,544,544,544,544,544,544,544,544,544,544,544,544,
  544,544,544,544,544,544,544,544,544,544,544,544,544,544,544,544,544,544,
  544,544,544,544,544,1,544,83,2,2,2,722,
544,544,544,544,544,544,544,544,544,544,544,544,544,544,544,544,544,544,544,
  544,544,544,544,544,544,544,544,544,544,544,544,544,544,544,544,544,544,
  544,544,544,544,544,544,544,544,544,544,544,544,544,544,544,544,544,544,
  544,544,544,544,544,544,544,544,544,544,544,544,544,544,544,544,544,544,
  544,544,544,544,544,544,544,544,544,544,544,544,544,544,544,544,544,544,
  544,544,544,544,544,1,544,84,2,2,2,721,
544,544,544,544,544,544,544,544,544,544,544,544,544,544,544,544,544,544,544,
  544,544,544,544,544,544,544,544,544,1,544,85,2,2,2,720,
544,544,544,544,544,544,544,544,544,544,544,544,544,544,544,544,544,544,544,
  544,544,544,544,544,544,544,544,544,1,544,86,2,2,2,719,
544,544,544,544,544,544,544,544,1,544,87,2,2,2,718,
544,544,544,544,544,544,544,544,544,544,544,544,544,544,544,544,544,544,544,
  544,544,544,544,544,544,544,544,544,544,544,544,544,544,544,544,544,544,
  544,544,544,544,544,544,544,544,544,544,544,544,544,544,544,544,544,544,
  544,544,544,544,544,544,544,544,544,544,544,544,544,544,544,544,544,544,
  544,544,544,544,544,544,544,544,544,544,544,544,544,544,544,544,544,544,
  544,544,544,544,544,1,544,88,2,2,2,717,
544,544,1,544,89,2,2,2,716,
544,544,544,544,544,544,544,544,544,544,544,544,544,544,544,544,544,544,544,
  544,544,544,544,544,544,544,544,544,544,544,544,544,544,544,544,544,544,
  544,544,544,544,544,544,544,544,544,544,544,544,544,544,544,544,544,544,
  544,544,544,544,544,544,544,544,544,544,544,544,544,544,544,544,544,544,
  544,544,544,544,544,544,544,544,544,544,544,544,544,544,544,544,544,544,
  544,544,544,544,544,1,544,90,2,2,2,715,
544,544,544,544,544,544,544,544,544,544,544,544,544,544,544,544,544,544,544,
  544,544,544,544,544,544,544,544,544,544,544,544,544,544,544,544,544,544,
  544,544,544,544,544,544,544,544,544,544,544,544,544,544,544,544,544,544,
  544,544,544,544,544,544,544,544,544,544,544,544,544,544,544,544,544,544,
  544,544,544,544,544,544,544,544,544,544,544,544,544,544,544,544,544,544,
  544,544,544,544,544,1,544,91,2,2,2,714,
544,544,544,544,544,544,544,544,544,544,544,544,544,544,544,544,544,544,544,
  544,544,544,544,544,544,544,544,544,544,544,544,544,544,544,544,544,544,
  544,544,544,544,544,544,544,544,544,544,544,544,544,544,544,544,544,544,
  544,544,544,544,544,544,544,544,544,544,544,544,544,544,544,544,544,544,
  544,544,544,544,544,544,544,544,544,544,544,544,544,544,544,544,544,544,
  544,544,544,544,544,1,544,92,2,2,2,713,
544,544,544,544,544,544,544,544,544,544,544,544,544,544,544,544,544,544,544,
  544,544,544,544,544,544,544,544,544,544,544,544,544,544,544,544,544,544,
  544,544,544,544,544,544,544,544,544,544,544,544,544,544,544,544,544,544,
  544,544,544,544,544,544,544,544,544,544,544,544,544,544,544,544,544,544,
  544,544,544,544,544,544,544,544,544,544,544,544,544,544,544,544,544,544,
  544,544,544,544,544,1,544,93,2,2,2,712,
544,544,544,544,544,544,544,544,544,544,544,544,544,544,544,544,544,544,544,
  544,544,544,544,544,544,544,544,544,544,544,544,544,544,544,544,544,544,
  544,544,544,544,544,544,544,544,544,544,544,544,544,544,544,544,544,544,
  544,544,544,544,544,544,544,544,544,544,544,544,544,544,544,544,544,544,
  544,544,544,544,544,544,544,544,544,544,544,544,544,544,544,544,544,544,
  544,544,544,544,544,1,544,94,2,2,2,711,
544,544,544,544,544,544,544,544,544,544,544,544,544,544,544,544,544,544,544,
  544,544,544,544,544,544,544,544,544,544,544,544,544,544,544,544,544,544,
  544,544,544,544,544,544,544,544,544,544,544,544,544,544,544,544,544,544,
  544,544,544,544,544,544,544,544,544,544,544,544,544,544,544,544,544,544,
  544,544,544,544,544,544,544,544,544,544,544,544,544,544,544,544,544,544,
  544,544,544,544,544,1,544,95,2,2,2,710,
544,544,544,544,544,544,544,544,544,544,544,544,544,544,544,544,544,544,544,
  544,544,544,544,544,544,544,544,544,544,544,544,544,544,544,544,544,544,
  544,544,544,544,544,544,544,544,544,544,544,544,544,544,544,544,544,544,
  544,544,544,544,544,544,544,544,544,544,544,544,544,544,544,544,544,544,
  544,544,544,544,544,544,544,544,544,544,544,544,544,544,544,544,544,544,
  544,544,544,544,544,1,544,96,2,2,2,709,
544,544,544,544,544,544,544,544,544,544,544,544,544,544,544,544,544,544,544,
  544,544,544,544,544,544,544,544,544,544,544,544,544,544,544,544,544,544,
  544,544,544,544,544,544,544,544,544,544,544,544,544,544,544,544,544,544,
  544,544,544,544,544,544,544,544,544,544,544,544,544,544,544,544,544,544,
  544,544,544,544,544,544,544,544,544,544,544,544,544,544,544,544,544,544,
  544,544,544,544,544,1,544,97,2,2,2,708,
544,544,544,544,544,544,544,544,544,544,544,544,544,544,544,544,544,544,544,
  544,544,544,544,544,544,544,544,544,544,544,544,544,544,544,544,544,544,
  544,544,544,544,544,544,544,544,544,544,544,544,544,544,544,544,544,544,
  544,544,544,544,544,544,544,544,544,544,544,544,544,544,544,544,544,544,
  544,544,544,544,544,544,544,544,544,544,544,544,544,544,544,544,544,544,
  544,544,544,544,544,1,544,98,2,2,2,707,
544,544,544,544,544,544,544,544,544,544,544,544,544,544,544,544,544,544,544,
  544,544,544,544,544,544,544,544,544,544,544,544,544,544,544,544,544,544,
  544,544,544,544,544,544,544,544,544,544,544,544,544,544,544,544,544,544,
  544,544,544,544,544,544,544,544,544,544,544,544,544,544,544,544,544,544,
  544,544,544,544,544,544,544,544,544,544,544,544,544,544,544,544,544,544,
  544,544,544,544,544,1,544,99,2,2,2,706,
544,544,544,544,544,544,544,544,544,544,544,544,544,544,544,544,544,544,544,
  544,544,544,544,544,544,544,544,544,544,544,544,544,544,544,544,544,544,
  544,544,544,544,544,544,544,544,544,544,544,544,544,544,544,544,544,544,
  544,544,544,544,544,544,544,544,544,544,544,544,544,544,544,544,544,544,
  544,544,544,544,544,544,544,544,544,544,544,544,544,544,544,544,544,544,
  544,544,544,544,544,1,544,100,2,2,2,705,
544,544,544,544,544,544,544,544,544,544,544,544,544,544,544,544,544,544,544,
  544,544,544,544,544,544,544,544,544,544,544,544,544,544,544,544,544,544,
  544,544,544,544,544,544,544,544,544,544,544,544,544,544,544,544,544,544,
  544,544,544,544,544,544,544,544,544,544,544,544,544,544,544,544,544,544,
  544,544,544,544,544,544,544,544,544,544,544,544,544,544,544,544,544,544,
  544,544,544,544,544,1,544,101,2,2,2,704,
544,544,544,544,544,544,544,544,544,544,544,544,544,544,544,544,544,544,544,
  544,544,544,544,544,544,544,544,544,544,544,544,544,544,544,544,544,544,
  544,544,544,544,544,544,544,544,544,544,544,544,544,544,544,544,544,544,
  544,544,544,544,544,544,544,544,544,544,544,544,544,544,544,544,544,544,
  544,544,544,544,544,544,544,544,544,544,544,544,544,544,544,544,544,544,
  544,544,544,544,544,1,544,102,2,2,2,703,
544,544,544,544,544,544,544,544,544,544,544,544,544,544,544,544,544,544,544,
  544,544,544,544,544,544,544,544,544,544,544,544,544,544,544,544,544,544,
  544,544,544,544,544,544,544,544,544,544,544,544,544,544,544,544,544,544,
  544,544,544,544,544,544,544,544,544,544,544,544,544,544,544,544,544,544,
  544,544,544,544,544,544,544,544,544,544,544,544,544,544,544,544,544,544,
  544,544,544,544,544,1,544,103,2,2,2,702,
544,544,544,544,544,1,544,104,2,2,2,701,
544,544,544,544,544,1,544,105,2,2,2,700,
544,544,544,544,544,544,544,544,544,544,544,544,544,544,544,544,544,544,544,
  544,544,544,544,544,544,544,544,544,544,544,544,544,544,544,544,544,544,
  544,544,544,544,544,544,544,544,544,544,544,544,544,544,544,544,544,544,
  544,544,544,544,544,544,544,544,544,544,544,544,544,544,544,544,544,544,
  544,544,544,544,544,544,544,544,544,544,544,544,544,544,544,544,544,544,
  544,544,544,544,544,1,544,106,2,2,2,699,
544,544,544,544,544,544,544,544,544,544,544,544,544,544,544,544,544,544,544,
  544,544,544,544,544,544,544,544,544,1,544,107,2,2,2,698,
544,544,544,544,544,544,544,544,544,544,544,544,544,544,544,544,544,544,544,
  544,544,544,544,544,544,544,544,544,1,544,108,2,2,2,697,
544,544,544,544,544,544,544,544,1,544,109,2,2,2,696,
544,544,544,544,544,544,544,544,544,544,544,544,544,544,544,544,544,544,544,
  544,544,544,544,544,544,544,544,544,544,544,544,544,544,544,544,544,544,
  544,544,544,544,544,544,544,544,544,544,544,544,544,544,544,544,544,544,
  544,544,544,544,544,544,544,544,544,544,544,544,544,544,544,544,544,544,
  544,544,544,544,544,544,544,544,544,544,544,544,544,544,544,544,544,544,
  544,544,544,544,544,1,544,110,2,2,2,695,
544,544,544,544,544,544,544,544,1,544,111,2,2,2,694,
544,544,544,544,544,544,544,544,1,544,112,2,2,2,693,
544,544,544,544,1,544,113,2,2,2,692,
544,544,544,544,544,544,544,544,544,544,544,544,544,544,544,544,544,544,544,
  544,544,544,544,544,544,544,544,544,544,544,544,544,544,544,544,544,544,
  544,544,544,544,544,544,544,544,544,544,544,544,544,544,544,544,544,544,
  544,544,544,544,544,544,544,544,544,544,544,544,544,544,544,544,544,544,
  544,544,544,544,544,544,544,544,544,544,544,544,544,544,544,544,544,544,
  544,544,544,544,544,1,544,114,2,2,2,691,
544,544,544,544,544,544,544,544,544,544,544,544,544,544,544,544,544,544,544,
  544,544,544,544,544,544,544,544,544,1,544,115,2,2,2,690,
544,544,544,544,544,544,544,544,544,544,544,544,544,544,544,544,544,544,544,
  544,544,544,544,544,544,544,544,544,1,544,116,2,2,2,689,
544,544,544,544,544,544,544,544,544,544,544,544,544,544,544,544,544,544,544,
  544,544,544,544,544,544,544,544,544,1,544,117,2,2,2,688,
544,544,1,544,118,2,2,2,687,
544,544,544,544,544,544,544,544,544,544,544,544,544,544,544,544,544,544,544,
  544,544,544,544,544,544,544,544,544,1,544,119,2,2,2,686,
544,544,544,544,544,544,544,544,544,544,544,544,544,544,544,544,544,544,544,
  544,544,544,544,544,544,544,544,544,1,544,120,2,2,2,685,
544,544,544,544,544,544,544,544,544,544,544,544,544,544,544,544,544,544,544,
  544,544,544,544,544,544,544,544,544,1,544,121,2,2,2,684,
544,544,544,544,544,544,544,544,544,544,544,544,544,544,544,544,544,544,544,
  544,544,544,544,544,544,544,544,544,1,544,122,2,2,2,683,
544,544,544,544,544,544,544,544,544,544,544,544,544,544,544,544,544,544,544,
  544,544,544,544,544,544,544,544,544,1,544,123,2,2,2,682,
544,544,544,544,544,544,544,544,544,544,544,544,544,544,544,544,544,544,544,
  544,544,544,544,544,544,544,544,544,1,544,124,2,2,2,681,
544,544,544,544,544,544,544,544,544,544,544,544,544,544,544,544,544,544,544,
  544,544,544,544,544,544,544,544,544,1,544,125,2,2,2,680,
544,544,544,544,544,544,544,544,544,544,544,544,544,544,544,544,544,544,544,
  544,544,544,544,544,544,544,544,544,1,544,126,2,2,2,679,
544,544,544,544,544,544,544,544,544,544,544,544,544,544,544,544,544,544,544,
  544,544,544,544,544,544,544,544,544,1,544,127,2,2,2,678,
544,544,544,544,544,544,544,544,544,544,544,544,544,544,544,544,544,544,544,
  544,544,544,544,544,544,544,544,544,1,544,128,2,2,2,677,
544,544,544,544,544,544,544,544,544,544,544,544,544,544,544,544,544,544,544,
  544,544,544,544,544,544,544,544,544,1,544,129,2,2,2,676,
544,544,544,544,544,544,544,544,544,544,544,544,544,544,544,544,544,544,544,
  544,544,544,544,544,544,544,544,544,1,544,130,2,2,2,675,
544,544,544,544,544,544,544,544,544,544,544,544,544,544,544,544,544,544,544,
  544,544,544,544,544,544,544,544,544,1,544,131,2,2,2,674,
544,544,544,544,544,544,544,544,544,544,544,544,544,544,544,544,544,544,544,
  544,544,544,544,544,544,544,544,544,1,544,132,2,2,2,673,
544,544,544,544,1,544,133,2,2,2,672,
544,544,544,544,544,544,544,544,1,544,134,2,2,2,671,
544,544,544,544,544,544,544,544,544,544,544,544,544,544,544,544,544,544,544,
  544,544,544,544,544,544,544,544,544,1,544,135,2,2,2,670,
544,544,544,544,544,544,544,544,544,544,544,544,544,544,544,544,544,544,544,
  544,544,544,544,544,544,544,544,544,544,544,544,544,544,544,544,544,544,
  544,544,544,544,544,544,544,544,544,544,544,544,544,544,544,544,544,544,
  544,544,544,544,544,544,544,544,544,544,544,544,544,544,544,544,544,544,
  544,544,544,544,544,544,544,544,544,544,544,544,544,544,544,544,544,544,
  544,544,544,544,544,1,544,136,2,2,2,669,
544,544,544,544,544,544,544,544,544,544,544,544,544,544,544,544,544,544,544,
  544,544,544,544,544,544,544,544,544,544,544,544,544,544,544,544,544,544,
  544,544,544,544,544,544,544,544,544,544,544,544,544,544,544,544,544,544,
  544,544,544,544,544,544,544,544,544,544,544,544,544,544,544,544,544,544,
  544,544,544,544,544,544,544,544,544,544,544,544,544,544,544,544,544,544,
  544,544,544,544,544,1,544,137,2,2,2,668,
544,544,544,544,544,544,544,544,544,544,544,544,544,544,544,544,544,544,544,
  544,544,544,544,544,544,544,544,544,544,544,544,544,544,544,544,544,544,
  544,544,544,544,544,544,544,544,544,544,544,544,544,544,544,544,544,544,
  544,544,544,544,544,544,544,544,544,544,544,544,544,544,544,544,544,544,
  544,544,544,544,544,544,544,544,544,544,544,544,544,544,544,544,544,544,
  544,544,544,544,544,1,544,138,2,2,2,667,
544,544,544,544,544,544,544,544,544,544,544,544,544,544,544,544,544,544,544,
  544,544,544,544,544,544,544,544,544,544,544,544,544,544,544,544,544,544,
  544,544,544,544,544,544,544,544,544,544,544,544,544,544,544,544,544,544,
  544,544,544,544,544,544,544,544,544,544,544,544,544,544,544,544,544,544,
  544,544,544,544,544,544,544,544,544,544,544,544,544,544,544,544,544,544,
  544,544,544,544,544,1,544,139,2,2,2,666,
544,544,544,544,1,544,140,2,2,2,665,
544,544,544,544,544,544,544,544,1,544,141,2,2,2,664,
544,544,544,544,1,544,142,2,2,2,663,
544,544,544,544,544,544,544,544,544,544,544,544,544,544,544,544,544,544,544,
  544,544,544,544,544,544,544,544,544,544,544,544,544,544,544,544,544,544,
  544,544,544,544,544,544,544,544,544,544,544,544,544,544,544,544,544,544,
  544,544,544,544,544,544,544,544,544,544,544,544,544,544,544,544,544,544,
  544,544,544,544,544,544,544,544,544,544,544,544,544,544,544,544,544,544,
  544,544,544,544,544,1,544,143,2,2,2,661,
544,544,544,544,544,544,544,544,544,544,544,544,544,544,544,544,544,544,544,
  544,544,544,544,544,544,544,544,544,1,544,144,2,2,2,660,
544,544,544,544,544,544,544,544,544,544,544,544,544,544,544,544,544,544,544,
  544,544,544,544,544,544,544,544,544,1,544,145,2,2,2,659,
544,544,544,544,544,544,544,544,544,544,544,544,544,544,544,544,544,544,544,
  544,544,544,544,544,544,544,544,544,1,544,146,2,2,2,658,
544,544,544,544,544,544,544,544,544,544,544,544,544,544,544,544,544,544,544,
  544,544,544,544,544,544,544,544,544,1,544,147,2,2,2,657,
544,544,544,544,544,544,544,544,544,544,544,544,544,544,544,544,544,544,544,
  544,544,544,544,544,544,544,544,544,1,544,148,2,2,2,656,
544,544,544,544,544,544,544,544,544,544,544,544,544,544,544,544,544,544,544,
  544,544,544,544,544,544,544,544,544,1,544,149,2,2,2,655,
544,544,544,544,544,544,544,544,544,544,544,544,544,544,544,544,544,544,544,
  544,544,544,544,544,544,544,544,544,1,544,150,2,2,2,654,
544,544,544,544,544,544,544,544,1,544,151,2,2,2,653,
544,544,544,544,544,544,544,544,544,544,544,544,544,544,544,544,544,544,544,
  544,544,544,544,544,544,544,544,544,544,544,544,544,544,544,544,544,544,
  544,544,544,544,544,544,544,544,544,544,544,544,544,544,544,544,544,544,
  544,544,544,544,544,544,544,544,544,544,544,544,544,544,544,544,544,544,
  544,544,544,544,544,544,544,544,544,544,544,544,544,544,544,544,544,544,
  544,544,544,544,544,1,544,152,2,2,2,652,
544,544,544,544,544,544,544,544,544,544,544,544,544,544,544,544,544,544,544,
  544,544,544,544,544,544,544,544,544,544,544,544,544,544,544,544,544,544,
  544,544,544,544,544,544,544,544,544,544,544,544,544,544,544,544,544,544,
  544,544,544,544,544,544,544,544,544,544,544,544,544,544,544,544,544,544,
  544,544,544,544,544,544,544,544,544,544,544,544,544,544,544,544,544,544,
  544,544,544,544,544,1,544,153,2,2,2,651,
544,544,544,544,544,544,544,544,544,544,544,544,544,544,544,544,544,544,544,
  544,544,544,544,544,544,544,544,544,1,544,154,2,2,2,650,
544,544,544,544,544,544,544,544,544,544,544,544,544,544,544,544,544,544,544,
  544,544,544,544,544,544,544,544,544,1,544,155,2,2,2,649,
544,544,544,544,544,544,544,544,544,544,544,544,544,544,544,544,544,544,544,
  544,544,544,544,544,544,544,544,544,1,544,156,2,2,2,648,
544,544,544,544,544,544,544,544,544,544,544,544,544,544,544,544,544,544,544,
  544,544,544,544,544,544,544,544,544,544,544,544,544,544,544,544,544,544,
  544,544,544,544,544,544,544,544,544,544,544,544,544,544,544,544,544,544,
  544,544,544,544,544,544,544,544,544,544,544,544,544,544,544,544,544,544,
  544,544,544,544,544,544,544,544,544,544,544,544,544,544,544,544,544,544,
  544,544,544,544,544,1,544,157,2,2,2,647,
544,544,544,544,544,544,544,544,544,544,544,544,544,544,544,544,544,544,544,
  544,544,544,544,544,544,544,544,544,1,544,158,2,2,2,646,
544,544,544,544,544,544,544,544,1,544,159,2,2,2,645,
544,544,544,544,544,544,544,544,544,544,544,544,544,544,544,544,544,544,544,
  544,544,544,544,544,544,544,544,544,1,544,160,2,2,2,644,
544,544,544,544,544,544,544,544,1,544,161,2,2,2,643,
544,544,544,544,544,544,544,544,1,544,162,2,2,2,642,
544,544,544,544,544,544,544,544,544,544,544,544,544,544,544,544,544,544,544,
  544,544,544,544,544,544,544,544,544,1,544,163,2,2,2,640,
277,277,277,277,277,277,277,277,277,277,277,277,277,277,277,277,277,277,277,
  277,277,277,277,277,277,277,277,277,5,164,5,277,
278,278,278,278,278,278,278,278,5,165,5,278,
279,279,279,279,279,279,279,279,279,279,279,279,279,279,279,279,279,279,279,
  279,279,279,279,279,279,279,279,279,5,166,5,279,
280,280,280,280,280,280,280,280,5,167,5,280,
281,281,5,168,5,281,
282,282,282,282,282,282,282,282,282,282,282,282,282,282,282,282,282,282,282,
  282,282,282,282,282,282,282,282,282,5,169,5,282,
283,283,283,283,283,283,283,283,283,283,283,283,283,283,283,283,283,283,283,
  283,283,283,283,283,283,283,283,283,5,170,5,283,
284,284,284,284,284,284,284,284,284,284,284,284,284,284,284,284,284,284,284,
  284,284,284,284,284,284,284,284,284,5,171,5,284,
285,285,285,285,285,285,285,285,5,172,5,285,
286,286,5,173,5,286,
287,287,287,287,287,5,174,5,287,
288,288,288,288,288,5,175,5,288,
289,289,289,289,289,289,289,289,289,289,289,289,289,289,289,289,289,289,289,
  289,289,289,289,289,289,289,289,289,5,176,5,289,
290,290,290,290,290,290,290,290,290,290,290,290,290,290,290,290,290,290,290,
  290,290,290,290,290,290,290,290,290,5,177,5,290,
291,291,291,291,291,291,291,291,5,178,5,291,
292,292,292,292,292,292,292,292,5,179,5,292,
293,293,293,293,293,293,293,293,5,180,5,293,
294,294,294,294,5,181,5,294,
295,295,295,295,295,295,295,295,295,295,295,295,295,295,295,295,295,295,295,
  295,295,295,295,295,295,295,295,295,5,182,5,295,
296,296,296,296,296,296,296,296,296,296,296,296,296,296,296,296,296,296,296,
  296,296,296,296,296,296,296,296,296,5,183,5,296,
297,297,297,297,297,297,297,297,297,297,297,297,297,297,297,297,297,297,297,
  297,297,297,297,297,297,297,297,297,5,184,5,297,
298,298,5,185,5,298,
299,299,299,299,299,299,299,299,299,299,299,299,299,299,299,299,299,299,299,
  299,299,299,299,299,299,299,299,299,5,186,5,299,
300,300,300,300,300,300,300,300,300,300,300,300,300,300,300,300,300,300,300,
  300,300,300,300,300,300,300,300,300,5,187,5,300,
301,301,301,301,301,301,301,301,301,301,301,301,301,301,301,301,301,301,301,
  301,301,301,301,301,301,301,301,301,5,188,5,301,
302,302,302,302,302,302,302,302,302,302,302,302,302,302,302,302,302,302,302,
  302,302,302,302,302,302,302,302,302,5,189,5,302,
303,303,303,303,303,303,303,303,303,303,303,303,303,303,303,303,303,303,303,
  303,303,303,303,303,303,303,303,303,5,190,5,303,
304,304,304,304,304,304,304,304,304,304,304,304,304,304,304,304,304,304,304,
  304,304,304,304,304,304,304,304,304,5,191,5,304,
305,305,305,305,305,305,305,305,305,305,305,305,305,305,305,305,305,305,305,
  305,305,305,305,305,305,305,305,305,5,192,5,305,
306,306,306,306,306,306,306,306,306,306,306,306,306,306,306,306,306,306,306,
  306,306,306,306,306,306,306,306,306,5,193,5,306,
307,307,307,307,307,307,307,307,307,307,307,307,307,307,307,307,307,307,307,
  307,307,307,307,307,307,307,307,307,5,194,5,307,
308,308,308,308,308,308,308,308,308,308,308,308,308,308,308,308,308,308,308,
  308,308,308,308,308,308,308,308,308,5,195,5,308,
309,309,309,309,309,309,309,309,309,309,309,309,309,309,309,309,309,309,309,
  309,309,309,309,309,309,309,309,309,5,196,5,309,
310,310,310,310,310,310,310,310,310,310,310,310,310,310,310,310,310,310,310,
  310,310,310,310,310,310,310,310,310,5,197,5,310,
311,311,311,311,311,311,311,311,311,311,311,311,311,311,311,311,311,311,311,
  311,311,311,311,311,311,311,311,311,5,198,5,311,
312,312,312,312,312,312,312,312,312,312,312,312,312,312,312,312,312,312,312,
  312,312,312,312,312,312,312,312,312,5,199,5,312,
313,313,313,313,5,200,5,313,
314,314,314,314,314,314,314,314,5,201,5,314,
315,315,315,315,315,315,315,315,315,315,315,315,315,315,315,315,315,315,315,
  315,315,315,315,315,315,315,315,315,5,202,5,315,
316,316,316,316,5,203,5,316,
317,317,317,317,317,317,317,317,5,204,5,317,
318,318,318,318,5,205,5,318,
319,319,319,319,319,319,319,319,319,319,319,319,319,319,319,319,319,319,319,
  319,319,319,319,319,319,319,319,319,5,206,5,319,
320,320,320,320,320,320,320,320,320,320,320,320,320,320,320,320,320,320,320,
  320,320,320,320,320,320,320,320,320,5,207,5,320,
321,321,321,321,321,321,321,321,321,321,321,321,321,321,321,321,321,321,321,
  321,321,321,321,321,321,321,321,321,5,208,5,321,
322,322,322,322,322,322,322,322,322,322,322,322,322,322,322,322,322,322,322,
  322,322,322,322,322,322,322,322,322,5,209,5,322,
323,323,323,323,323,323,323,323,323,323,323,323,323,323,323,323,323,323,323,
  323,323,323,323,323,323,323,323,323,5,210,5,323,
324,324,324,324,324,324,324,324,324,324,324,324,324,324,324,324,324,324,324,
  324,324,324,324,324,324,324,324,324,5,211,5,324,
325,325,325,325,325,325,325,325,325,325,325,325,325,325,325,325,325,325,325,
  325,325,325,325,325,325,325,325,325,5,212,5,325,
326,326,326,326,326,326,326,326,5,213,5,326,
327,327,327,327,327,327,327,327,327,327,327,327,327,327,327,327,327,327,327,
  327,327,327,327,327,327,327,327,327,5,214,5,327,
328,328,328,328,328,328,328,328,328,328,328,328,328,328,328,328,328,328,328,
  328,328,328,328,328,328,328,328,328,5,215,5,328,
329,329,329,329,329,329,329,329,329,329,329,329,329,329,329,329,329,329,329,
  329,329,329,329,329,329,329,329,329,5,216,5,329,
330,330,330,330,330,330,330,330,330,330,330,330,330,330,330,330,330,330,330,
  330,330,330,330,330,330,330,330,330,5,217,5,330,
331,331,331,331,331,331,331,331,5,218,5,331,
332,332,332,332,332,332,332,332,332,332,332,332,332,332,332,332,332,332,332,
  332,332,332,332,332,332,332,332,332,5,219,5,332,
333,333,333,333,333,333,333,333,5,220,5,333,
334,334,334,334,334,334,334,334,5,221,5,334,
335,335,335,335,335,335,335,335,335,335,335,335,335,335,335,335,335,335,335,
  335,335,335,335,335,335,335,335,335,5,222,5,335,
336,336,336,336,336,336,336,336,336,336,336,336,336,336,336,336,336,336,336,
  336,336,336,336,336,336,336,336,336,336,336,336,336,336,336,336,336,336,
  336,336,336,336,336,336,336,336,336,336,336,336,336,336,336,336,336,336,
  336,336,336,336,336,336,336,336,336,336,336,336,336,336,336,336,336,336,
  336,336,336,336,336,336,336,336,336,336,336,336,336,336,336,336,336,336,
  336,17,17,17,17,5,223,5,336,
544,544,544,544,544,544,544,544,544,1,2,224,2,2,2,590,
83,83,83,83,83,83,83,83,82,225,71,267,267,
544,544,544,544,544,544,544,544,544,544,544,544,544,1,2,226,2,2,2,589,
86,86,86,86,86,86,86,86,86,86,86,86,86,227,70,274,
544,544,544,544,1,2,228,2,2,2,588,
544,544,544,544,1,544,229,2,2,2,587,
68,68,68,68,337,230,337,
544,544,1,2,231,2,2,2,585,
338,92,232,66,272,339,65,273,340,
544,544,544,544,1,2,233,2,2,2,583,
544,544,544,544,1,2,234,2,2,2,582,
544,544,544,544,544,544,544,544,544,544,544,544,544,544,544,544,544,544,544,
  544,544,544,544,544,544,544,544,544,1,2,235,2,2,2,581,
83,83,83,83,83,83,83,174,174,342,341,345,83,350,351,352,353,354,355,356,61,
  39,366,67,343,344,184,82,236,162,152,348,349,163,346,164,347,377,379,
  267,378,376,375,374,373,372,371,371,371,370,369,368,151,371,367,365,364,
  363,362,361,360,359,267,358,357,
544,544,544,544,544,544,544,544,544,544,544,544,544,1,2,237,2,2,2,580,
83,83,83,83,83,83,83,83,60,60,60,60,82,238,61,267,267,
544,544,544,544,544,544,544,544,544,544,544,544,544,544,544,544,544,544,544,
  544,544,544,544,544,544,544,544,544,1,2,239,2,2,2,579,
83,83,83,83,83,83,83,174,174,342,341,345,83,350,351,352,353,354,355,356,61,
  39,366,67,343,344,184,82,240,162,152,348,349,163,346,164,347,59,267,376,
  375,374,373,372,371,371,371,370,369,368,151,371,367,365,364,363,362,361,
  360,359,267,358,357,
544,544,544,544,544,544,544,544,544,1,2,241,2,2,2,578,
83,83,83,83,83,83,83,83,82,242,58,267,267,
544,544,544,544,544,544,544,544,544,1,2,243,2,2,2,577,
83,83,83,83,83,83,83,83,82,244,79,380,267,267,
544,544,544,544,544,544,544,544,544,1,2,245,2,2,2,576,
83,83,83,83,83,83,83,83,82,246,79,381,267,267,
544,544,544,544,544,544,544,544,544,1,2,247,2,2,2,575,
83,83,83,83,83,83,83,83,82,248,79,382,267,267,
544,544,544,544,544,544,544,544,544,544,544,544,544,544,544,544,544,544,544,
  544,544,544,544,544,544,544,544,544,544,1,2,249,2,2,2,574,
83,83,83,83,83,83,83,174,174,342,341,383,83,92,350,351,352,353,354,355,356,
  61,39,366,67,343,344,184,82,250,162,152,348,349,163,346,73,272,164,347,
  339,74,75,384,267,376,375,374,373,372,371,371,371,370,369,368,151,371,
  367,365,364,363,362,361,360,359,273,267,340,358,357,
544,544,544,544,544,544,544,544,544,544,544,544,544,544,544,544,544,544,544,
  544,544,544,544,544,544,544,544,544,544,1,2,251,2,2,2,573,
83,83,83,83,83,83,83,174,174,342,341,383,83,92,350,351,352,353,354,355,356,
  61,39,366,67,343,344,184,82,252,162,152,348,349,163,346,73,272,164,347,
  339,74,75,385,267,376,375,374,373,372,371,371,371,370,369,368,151,371,
  367,365,364,363,362,361,360,359,273,267,340,358,357,
544,544,544,544,544,544,544,544,544,544,544,544,544,544,544,544,544,544,544,
  544,544,544,544,544,544,544,544,544,1,2,253,2,2,2,572,
83,83,83,83,83,83,83,174,174,342,341,345,83,350,351,352,353,354,355,356,61,
  39,366,67,343,344,184,82,254,162,152,348,349,163,346,164,347,50,267,376,
  375,374,373,372,371,371,371,370,369,368,151,371,367,365,364,363,362,361,
  360,359,267,358,357,
544,544,544,544,1,2,255,2,2,2,571,
544,544,544,544,1,2,256,2,2,2,570,
544,544,544,544,1,2,257,2,2,2,569,
544,544,544,544,544,544,544,544,544,544,544,544,544,544,544,544,544,544,544,
  544,544,544,544,544,544,544,544,544,1,2,258,2,2,2,568,
83,83,83,83,83,83,83,174,174,342,341,345,83,350,351,352,353,354,355,356,61,
  39,366,67,343,344,184,82,259,162,152,348,349,163,346,164,347,45,267,376,
  375,374,373,372,371,371,371,370,369,368,151,371,367,365,364,363,362,361,
  360,359,267,358,357,
83,83,83,83,83,83,83,174,174,342,341,383,83,92,350,351,352,353,354,355,356,
  61,39,366,67,343,344,184,82,260,162,152,348,349,163,346,73,272,164,347,
  339,74,75,386,267,376,375,374,373,372,371,371,371,370,369,368,151,371,
  367,365,364,363,362,361,360,359,273,267,340,358,357,
83,83,83,83,83,83,83,174,174,342,341,383,83,92,350,351,352,353,354,355,356,
  61,39,366,67,343,344,184,82,261,162,152,348,349,163,346,73,272,164,347,
  339,74,75,387,267,376,375,374,373,372,371,371,371,370,369,368,151,371,
  367,365,364,363,362,361,360,359,273,267,340,358,357,
83,83,83,83,83,83,83,174,174,342,341,345,83,350,351,352,353,354,355,356,61,
  39,366,67,343,344,184,82,262,162,152,348,349,163,346,164,347,49,267,376,
  375,374,373,372,371,371,371,370,369,368,151,371,367,365,364,363,362,361,
  360,359,267,358,357,
544,544,544,544,544,544,544,544,544,544,544,544,544,544,544,544,544,544,544,
  544,544,544,544,544,544,544,544,544,1,2,263,2,2,2,567,
83,83,83,83,83,83,83,174,174,342,341,345,83,350,351,352,353,354,355,356,61,
  39,366,67,343,344,184,82,264,162,152,348,349,163,346,164,347,40,267,376,
  375,374,373,372,371,371,371,370,369,368,151,371,367,365,364,363,362,361,
  360,359,267,358,357,
544,544,544,544,544,544,544,544,544,544,544,544,544,544,544,544,544,544,544,
  544,544,544,544,544,544,544,544,544,1,2,265,2,2,2,566,
83,83,83,83,83,83,83,174,174,342,341,345,83,350,351,352,353,354,355,356,61,
  39,366,67,343,344,184,82,266,162,152,348,349,163,346,164,347,39,267,376,
  375,374,373,372,371,371,371,370,369,368,151,371,367,365,364,363,362,361,
  360,359,267,358,357,
84,84,84,84,84,84,84,85,85,85,84,544,544,544,544,544,544,544,544,544,544,
  544,544,544,544,544,544,544,544,544,544,544,544,544,544,544,1,2,267,2,
  85,2,2,559,
544,544,544,544,1,2,268,2,2,2,558,
544,544,544,544,1,2,269,2,2,2,556,
98,98,388,98,98,98,98,98,98,96,98,98,98,98,98,98,98,98,98,98,98,98,98,98,98,
  98,98,98,98,98,98,98,98,270,
544,544,544,544,1,2,271,2,2,2,553,
93,93,389,93,93,93,93,93,93,93,93,93,93,93,93,93,93,91,93,93,93,93,93,93,93,
  93,93,93,93,93,93,93,93,93,272,
544,544,544,544,544,1,544,273,2,2,2,551,
87,87,87,87,87,87,87,87,87,87,87,87,87,544,544,544,1,2,274,2,2,2,554,
83,83,83,83,83,83,83,174,174,342,341,345,83,350,351,352,353,354,355,356,61,
  39,366,67,343,344,184,82,275,162,152,348,349,163,346,164,347,42,267,376,
  375,374,373,372,371,371,371,370,369,368,151,371,367,365,364,363,362,361,
  360,359,267,358,357,
83,83,83,83,83,83,83,174,174,342,341,345,83,350,351,352,353,354,355,356,61,
  39,366,67,343,344,184,82,276,162,152,348,349,163,346,164,347,41,267,376,
  375,374,373,372,371,371,371,370,369,368,151,371,367,365,364,363,362,361,
  360,359,267,358,357,
83,83,83,83,83,83,83,174,174,342,341,345,83,350,351,352,353,354,355,356,61,
  39,366,67,343,344,184,82,277,162,152,348,349,163,346,164,347,308,267,
  376,375,374,373,372,371,371,371,370,369,368,151,371,367,365,364,363,362,
  361,360,359,267,358,357,
204,201,199,197,203,202,200,198,278,307,390,
83,83,83,83,83,83,83,174,174,342,341,345,83,350,351,352,353,354,355,356,61,
  39,366,67,343,344,184,82,279,162,152,348,349,163,346,164,347,305,267,
  376,375,374,373,372,371,371,371,370,369,368,151,371,367,365,364,363,362,
  361,360,359,267,358,357,
204,201,199,197,203,202,200,198,280,304,390,
391,392,281,214,215,302,
83,83,83,83,83,83,83,174,174,342,341,345,83,350,351,352,353,354,355,356,61,
  39,366,67,343,344,184,82,282,162,152,348,349,163,346,164,347,301,267,
  376,375,374,373,372,371,371,371,370,369,368,151,371,367,365,364,363,362,
  361,360,359,267,358,357,
83,83,83,83,83,83,83,174,174,342,341,345,83,350,351,352,353,354,355,356,61,
  39,366,67,343,344,184,82,283,162,152,348,349,163,346,164,347,297,267,
  376,375,374,373,372,371,371,371,370,369,368,151,371,367,365,364,363,362,
  361,360,359,267,358,357,
83,83,83,83,83,83,83,174,174,342,341,345,83,350,351,352,353,354,355,356,61,
  39,366,67,343,344,184,82,284,162,152,348,349,163,346,164,347,296,267,
  376,375,374,373,372,371,371,371,370,369,368,151,371,367,365,364,363,362,
  361,360,359,267,358,357,
204,201,199,197,203,202,200,198,285,295,390,
53,54,286,293,293,
393,394,395,391,392,287,211,209,210,213,278,212,
393,394,395,391,392,288,211,209,210,213,277,212,
83,83,83,83,83,83,83,174,174,342,341,345,83,350,351,352,353,354,355,356,61,
  39,366,67,343,344,184,82,289,162,152,348,349,163,346,164,347,275,267,
  376,375,374,373,372,371,371,371,370,369,368,151,371,367,365,364,363,362,
  361,360,359,267,358,357,
83,83,83,83,83,83,83,174,174,342,341,345,83,350,351,352,353,354,355,356,61,
  39,366,67,343,344,184,82,290,162,152,348,349,163,346,164,347,274,267,
  376,375,374,373,372,371,371,371,370,369,368,151,371,367,365,364,363,362,
  361,360,359,267,358,357,
204,201,199,197,203,202,200,198,291,273,390,
204,201,199,197,203,202,200,198,292,396,390,
204,201,199,197,203,202,200,198,293,397,390,
207,206,205,208,294,399,398,
83,83,83,83,83,83,83,174,174,342,341,345,83,350,351,352,353,354,355,356,61,
  39,366,67,343,344,184,82,295,162,152,348,349,163,346,164,347,267,267,
  376,375,374,373,372,371,371,371,370,369,368,151,371,367,365,364,363,362,
  361,360,359,267,358,357,
83,83,83,83,83,83,83,174,174,342,341,345,83,350,351,352,353,354,355,356,61,
  39,366,67,343,344,184,82,296,162,152,348,349,163,346,164,347,266,267,
  376,375,374,373,372,371,371,371,370,369,368,151,371,367,365,364,363,362,
  361,360,359,267,358,357,
83,83,83,83,83,83,83,174,174,342,341,345,83,350,351,352,353,354,355,356,61,
  39,366,67,343,344,184,82,297,162,152,348,349,163,346,164,347,265,267,
  376,375,374,373,372,371,371,371,370,369,368,151,371,367,365,364,363,362,
  361,360,359,267,358,357,
391,392,298,214,215,264,
83,83,83,83,83,83,83,174,174,342,341,345,83,350,351,352,353,354,355,356,61,
  39,366,67,343,344,184,82,299,162,152,348,349,163,346,164,347,263,267,
  376,375,374,373,372,371,371,371,370,369,368,151,371,367,365,364,363,362,
  361,360,359,267,358,357,
83,83,83,83,83,83,83,174,174,342,341,345,83,350,351,352,353,354,355,356,61,
  39,366,67,343,344,184,82,300,162,152,348,349,163,346,164,347,262,267,
  376,375,374,373,372,371,371,371,370,369,368,151,371,367,365,364,363,362,
  361,360,359,267,358,357,
83,83,83,83,83,83,83,174,174,342,341,345,83,350,351,352,353,354,355,356,61,
  39,366,67,343,344,184,82,301,162,152,348,349,163,346,164,347,261,267,
  376,375,374,373,372,371,371,371,370,369,368,151,371,367,365,364,363,362,
  361,360,359,267,358,357,
83,83,83,83,83,83,83,174,174,342,341,345,83,350,351,352,353,354,355,356,61,
  39,366,67,343,344,184,82,302,162,152,348,349,163,346,164,347,260,267,
  376,375,374,373,372,371,371,371,370,369,368,151,371,367,365,364,363,362,
  361,360,359,267,358,357,
83,83,83,83,83,83,83,174,174,342,341,345,83,350,351,352,353,354,355,356,61,
  39,366,67,343,344,184,82,303,162,152,348,349,163,346,164,347,259,267,
  376,375,374,373,372,371,371,371,370,369,368,151,371,367,365,364,363,362,
  361,360,359,267,358,357,
83,83,83,83,83,83,83,174,174,342,341,345,83,350,351,352,353,354,355,356,61,
  39,366,67,343,344,184,82,304,162,152,348,349,163,346,164,347,258,267,
  376,375,374,373,372,371,371,371,370,369,368,151,371,367,365,364,363,362,
  361,360,359,267,358,357,
83,83,83,83,83,83,83,174,174,342,341,345,83,350,351,352,353,354,355,356,61,
  39,366,67,343,344,184,82,305,162,152,348,349,163,346,164,347,257,267,
  376,375,374,373,372,371,371,371,370,369,368,151,371,367,365,364,363,362,
  361,360,359,267,358,357,
83,83,83,83,83,83,83,174,174,342,341,345,83,350,351,352,353,354,355,356,61,
  39,366,67,343,344,184,82,306,162,152,348,349,163,346,164,347,256,267,
  376,375,374,373,372,371,371,371,370,369,368,151,371,367,365,364,363,362,
  361,360,359,267,358,357,
83,83,83,83,83,83,83,174,174,342,341,345,83,350,351,352,353,354,355,356,61,
  39,366,67,343,344,184,82,307,162,152,348,349,163,346,164,347,255,267,
  376,375,374,373,372,371,371,371,370,369,368,151,371,367,365,364,363,362,
  361,360,359,267,358,357,
83,83,83,83,83,83,83,174,174,342,341,345,83,350,351,352,353,354,355,356,61,
  39,366,67,343,344,184,82,308,162,152,348,349,163,346,164,347,254,267,
  376,375,374,373,372,371,371,371,370,369,368,151,371,367,365,364,363,362,
  361,360,359,267,358,357,
83,83,83,83,83,83,83,174,174,342,341,345,83,350,351,352,353,354,355,356,61,
  39,366,67,343,344,184,82,309,162,152,348,349,163,346,164,347,253,267,
  376,375,374,373,372,371,371,371,370,369,368,151,371,367,365,364,363,362,
  361,360,359,267,358,357,
83,83,83,83,83,83,83,174,174,342,341,345,83,350,351,352,353,354,355,356,61,
  39,366,67,343,344,184,82,310,162,152,348,349,163,346,164,347,252,267,
  376,375,374,373,372,371,371,371,370,369,368,151,371,367,365,364,363,362,
  361,360,359,267,358,357,
83,83,83,83,83,83,83,174,174,342,341,345,83,350,351,352,353,354,355,356,61,
  39,366,67,343,344,184,82,311,162,152,348,349,163,346,164,347,251,267,
  376,375,374,373,372,371,371,371,370,369,368,151,371,367,365,364,363,362,
  361,360,359,267,358,357,
83,83,83,83,83,83,83,174,174,342,341,345,83,350,351,352,353,354,355,356,61,
  39,366,67,343,344,184,82,312,162,152,348,349,163,346,164,347,250,267,
  376,375,374,373,372,371,371,371,370,369,368,151,371,367,365,364,363,362,
  361,360,359,267,358,357,
207,206,205,208,313,249,398,
204,201,199,197,203,202,200,198,314,248,390,
83,83,83,83,83,83,83,174,174,342,341,345,83,350,351,352,353,354,355,356,61,
  39,366,67,343,344,184,82,315,162,152,348,349,163,346,164,347,247,267,
  376,375,374,373,372,371,371,371,370,369,368,151,371,367,365,364,363,362,
  361,360,359,267,358,357,
207,206,205,208,316,242,398,
204,201,199,197,203,202,200,198,317,241,390,
207,206,205,208,318,240,398,
83,83,83,83,83,83,83,174,174,342,341,345,83,350,351,352,353,354,355,356,61,
  39,366,67,343,344,184,82,319,162,152,348,349,163,346,164,347,238,267,
  376,375,374,373,372,371,371,371,370,369,368,151,371,367,365,364,363,362,
  361,360,359,267,358,357,
83,83,83,83,83,83,83,174,174,342,341,345,83,350,351,352,353,354,355,356,61,
  39,366,67,343,344,184,82,320,162,152,348,349,163,346,164,347,237,267,
  376,375,374,373,372,371,371,371,370,369,368,151,371,367,365,364,363,362,
  361,360,359,267,358,357,
83,83,83,83,83,83,83,174,174,342,341,345,83,350,351,352,353,354,355,356,61,
  39,366,67,343,344,184,82,321,162,152,348,349,163,346,164,347,236,267,
  376,375,374,373,372,371,371,371,370,369,368,151,371,367,365,364,363,362,
  361,360,359,267,358,357,
83,83,83,83,83,83,83,174,174,342,341,345,83,350,351,352,353,354,355,356,61,
  39,366,67,343,344,184,82,322,162,152,348,349,163,346,164,347,235,267,
  376,375,374,373,372,371,371,371,370,369,368,151,371,367,365,364,363,362,
  361,360,359,267,358,357,
83,83,83,83,83,83,83,174,174,342,341,345,83,350,351,352,353,354,355,356,61,
  39,366,67,343,344,184,82,323,162,152,348,349,163,346,164,347,234,267,
  376,375,374,373,372,371,371,371,370,369,368,151,371,367,365,364,363,362,
  361,360,359,267,358,357,
83,83,83,83,83,83,83,174,174,342,341,345,83,350,351,352,353,354,355,356,61,
  39,366,67,343,344,184,82,324,162,152,348,349,163,346,164,347,233,267,
  376,375,374,373,372,371,371,371,370,369,368,151,371,367,365,364,363,362,
  361,360,359,267,358,357,
83,83,83,83,83,83,83,174,174,342,341,345,83,350,351,352,353,354,355,356,61,
  39,366,67,343,344,184,82,325,162,152,348,349,163,346,164,347,232,267,
  376,375,374,373,372,371,371,371,370,369,368,151,371,367,365,364,363,362,
  361,360,359,267,358,357,
204,201,199,197,203,202,200,198,326,231,390,
83,83,83,83,83,83,83,174,174,342,341,345,83,350,351,352,353,354,355,356,61,
  39,366,67,343,344,184,82,327,162,152,348,349,163,346,164,347,228,267,
  376,375,374,373,372,371,371,371,370,369,368,151,371,367,365,364,363,362,
  361,360,359,267,358,357,
83,83,83,83,83,83,83,174,174,342,341,345,83,350,351,352,353,354,355,356,61,
  39,366,67,343,344,184,82,328,162,152,348,349,163,346,164,347,227,267,
  376,375,374,373,372,371,371,371,370,369,368,151,371,367,365,364,363,362,
  361,360,359,267,358,357,
83,83,83,83,83,83,83,174,174,342,341,345,83,350,351,352,353,354,355,356,61,
  39,366,67,343,344,184,82,329,162,152,348,349,163,346,164,347,226,267,
  376,375,374,373,372,371,371,371,370,369,368,151,371,367,365,364,363,362,
  361,360,359,267,358,357,
83,83,83,83,83,83,83,174,174,342,341,345,83,350,351,352,353,354,355,356,61,
  39,366,67,343,344,184,82,330,162,152,348,349,163,346,164,347,224,267,
  376,375,374,373,372,371,371,371,370,369,368,151,371,367,365,364,363,362,
  361,360,359,267,358,357,
204,201,199,197,203,202,200,198,331,223,390,
83,83,83,83,83,83,83,174,174,342,341,345,83,350,351,352,353,354,355,356,61,
  39,366,67,343,344,184,82,332,162,152,348,349,163,346,164,347,222,267,
  376,375,374,373,372,371,371,371,370,369,368,151,371,367,365,364,363,362,
  361,360,359,267,358,357,
204,201,199,197,203,202,200,198,333,221,390,
204,201,199,197,203,202,200,198,334,220,390,
83,83,83,83,83,83,83,174,174,342,341,345,83,350,351,352,353,354,355,356,61,
  39,366,67,343,344,184,82,335,162,152,348,349,163,346,164,347,219,267,
  376,375,374,373,372,371,371,371,370,369,368,151,371,367,365,364,363,362,
  361,360,359,267,358,357,
73,74,75,76,77,78,79,80,81,82,83,84,85,86,87,88,89,90,91,92,93,94,95,96,97,
  98,99,100,101,102,103,104,105,106,107,108,109,110,111,112,113,114,115,
  116,117,118,119,120,121,122,123,124,125,126,127,128,129,130,131,132,133,
  134,135,136,137,138,139,140,141,142,143,144,145,146,147,148,149,150,151,
  152,153,154,155,156,157,158,159,160,161,162,163,67,72,217,218,222,221,
  220,219,218,217,225,216,215,214,229,230,213,212,211,210,209,208,207,206,
  239,205,204,203,243,244,245,246,202,201,200,199,198,197,196,195,194,193,
  192,191,190,189,188,187,186,185,184,183,182,268,181,180,179,272,178,177,
  176,276,175,174,279,280,281,282,283,284,285,286,287,288,289,290,291,292,
  173,294,172,171,170,298,299,300,169,168,303,167,166,306,165,164,309,
174,174,342,345,343,344,184,16,337,67,348,400,164,347,358,
185,401,185,185,185,185,185,185,185,185,185,185,185,185,185,185,185,185,185,
  185,185,185,185,185,185,185,185,185,185,185,185,185,191,185,338,402,
185,401,185,185,185,185,185,185,185,185,185,185,185,185,88,185,185,185,185,
  185,185,185,185,185,185,185,185,185,185,185,185,185,185,191,185,339,90,
544,544,544,544,544,1,544,340,2,2,2,584,
195,195,195,341,
174,403,174,174,174,174,174,174,174,174,174,174,174,174,174,174,174,174,174,
  174,174,174,174,174,174,174,174,174,174,174,174,174,174,174,174,174,174,
  174,174,174,174,174,174,174,174,174,174,174,174,174,174,174,174,174,174,
  174,174,174,174,174,174,174,174,174,174,174,174,174,174,174,174,174,174,
  174,174,174,174,174,174,174,174,174,174,174,174,174,174,174,174,174,174,
  174,174,174,174,174,174,174,174,174,174,174,174,174,174,174,174,174,174,
  174,174,174,174,174,174,174,174,174,174,174,174,174,174,174,174,174,174,
  174,174,174,174,174,174,174,174,174,174,174,174,174,174,174,342,
173,173,173,343,
172,172,172,344,
185,401,185,185,185,185,185,185,185,185,185,185,185,185,185,185,185,185,185,
  185,185,185,185,185,185,185,185,185,185,185,185,185,191,185,345,404,
196,196,196,193,
181,181,181,181,167,347,
179,179,179,179,166,166,166,166,166,166,166,166,166,166,166,166,166,166,166,
  166,166,166,166,166,166,166,166,166,166,166,166,166,166,166,166,166,166,
  166,166,166,166,166,166,166,166,166,166,166,166,166,166,166,166,166,166,
  166,166,166,166,166,166,166,166,166,166,166,166,166,166,166,166,166,166,
  166,166,166,166,166,166,166,166,166,166,166,166,166,166,166,166,166,166,
  166,166,166,166,166,166,166,166,166,166,166,166,166,166,166,166,166,166,
  166,166,166,166,166,166,166,166,166,166,166,166,166,166,166,166,166,166,
  166,166,166,166,166,166,166,348,
176,180,175,175,175,194,177,165,165,165,165,165,165,165,165,165,165,165,165,
  165,165,165,165,165,165,165,165,165,165,165,165,165,165,165,165,165,165,
  165,165,165,165,165,165,165,165,165,165,165,165,165,165,165,165,165,165,
  165,165,165,165,165,165,165,165,165,165,165,165,165,165,165,165,168,171,
  169,165,165,165,165,165,165,165,165,165,165,165,165,165,165,165,165,165,
  165,165,165,165,165,165,165,165,165,165,165,165,165,165,165,165,165,165,
  170,165,165,349,
544,1,2,350,2,2,2,633,
544,1,2,351,2,2,2,632,
544,1,2,352,2,2,2,631,
544,1,2,353,2,2,2,630,
544,1,2,354,2,2,2,629,
544,1,2,355,2,2,2,628,
544,1,2,356,2,2,2,627,
544,544,544,544,544,544,544,544,544,544,544,544,544,544,544,544,544,544,544,
  544,544,544,544,544,544,544,544,544,544,544,544,544,544,544,544,544,544,
  544,544,544,544,544,544,544,544,544,544,544,544,544,544,544,544,544,544,
  544,544,544,544,544,544,544,544,544,544,544,544,544,544,544,544,544,544,
  544,544,544,544,544,544,544,544,544,544,544,544,544,544,544,544,544,544,
  544,544,544,544,544,544,544,544,544,544,544,544,544,544,544,544,544,544,
  544,544,544,544,544,544,544,544,544,544,544,544,544,544,544,544,544,544,
  544,1,544,357,2,2,2,634,
544,544,544,544,544,544,544,544,544,544,544,544,544,544,544,544,544,544,544,
  544,544,544,544,544,544,544,544,544,544,544,544,544,544,544,544,544,544,
  544,544,544,544,544,544,544,544,544,544,544,544,544,544,544,544,544,544,
  544,544,544,544,544,544,544,544,544,544,544,544,544,544,544,544,544,544,
  544,544,544,544,544,544,544,544,544,544,544,544,544,544,544,544,544,544,
  544,544,544,544,544,544,544,544,544,544,544,544,544,544,544,544,544,544,
  544,544,544,544,544,544,544,544,544,544,544,544,544,544,544,544,544,544,
  544,1,2,358,2,2,2,586,
61,359,405,
61,360,406,
61,361,407,
61,362,408,
61,363,409,
61,364,410,
61,365,411,
544,544,544,544,544,544,544,544,544,544,544,544,544,544,544,544,544,544,544,
  544,544,544,544,544,544,1,2,366,2,2,2,623,
83,83,83,83,83,83,83,174,174,342,341,345,83,350,351,352,353,354,355,356,61,
  39,366,67,343,344,184,82,367,162,152,348,349,163,346,164,347,412,267,
  376,375,374,373,372,371,371,371,370,369,368,151,371,367,365,364,363,362,
  361,360,359,267,358,357,
83,83,83,83,83,83,83,174,174,342,341,345,83,350,351,352,353,354,355,356,61,
  343,344,184,82,368,162,152,348,349,163,346,164,347,267,150,151,150,367,
  365,364,363,362,361,360,359,267,358,357,
83,83,83,83,83,83,83,174,174,342,341,345,83,350,351,352,353,354,355,356,61,
  343,344,184,82,369,162,152,348,349,163,346,164,347,267,149,151,149,367,
  365,364,363,362,361,360,359,267,358,357,
83,83,83,83,83,83,83,174,174,342,341,345,83,350,351,352,353,354,355,356,61,
  343,344,184,82,370,162,152,348,349,163,346,164,347,267,148,151,148,367,
  365,364,363,362,361,360,359,267,358,357,
413,415,64,55,59,138,419,418,417,416,414,
57,58,133,421,420,
422,424,426,428,130,429,427,425,423,
430,63,127,432,431,
433,42,124,435,434,
436,40,123,438,437,
439,440,441,442,443,444,445,446,447,448,449,450,451,103,103,103,103,103,377,
  107,107,107,107,110,110,110,113,113,113,116,116,116,119,119,119,122,122,
  122,
83,83,83,83,83,83,83,174,174,342,341,345,83,350,351,352,353,354,355,356,61,
  39,366,67,343,344,184,82,378,162,152,348,349,163,346,164,347,102,267,
  376,375,374,373,372,371,371,371,370,369,368,151,371,367,365,364,363,362,
  361,360,359,267,358,357,
17,17,17,17,5,379,5,62,
452,57,57,57,57,5,380,5,452,
452,56,56,56,56,5,381,5,452,
452,55,55,55,55,5,382,5,452,
185,401,185,185,185,185,185,185,185,185,185,185,185,185,185,185,185,185,185,
  185,185,185,185,185,185,185,185,185,185,185,185,185,191,185,383,453,
454,17,17,17,17,5,384,5,454,
455,17,17,17,17,5,385,5,455,
456,17,17,17,17,5,386,5,456,
457,17,17,17,17,5,387,5,457,
99,99,99,99,99,99,99,99,99,99,99,99,99,99,99,99,99,99,99,99,99,99,99,99,99,
  99,99,99,99,99,99,100,99,99,99,388,
94,94,94,94,94,94,94,94,94,94,94,94,94,94,94,94,94,94,94,94,94,94,94,94,94,
  94,94,94,94,94,94,95,94,94,94,389,
544,544,544,544,544,544,544,544,544,544,544,544,544,544,544,544,544,544,544,
  544,544,544,544,544,544,544,544,544,544,544,544,544,544,544,544,544,544,
  544,544,544,544,544,544,544,544,544,544,544,544,544,544,544,544,544,544,
  544,544,544,544,544,544,544,544,544,544,544,544,544,544,544,544,544,544,
  544,544,544,544,544,544,544,544,544,544,544,544,544,544,544,544,544,544,
  544,544,544,544,544,544,1,544,390,2,2,2,641,
544,544,544,544,544,544,544,544,544,544,544,544,544,544,544,544,544,544,544,
  544,544,544,544,544,544,544,544,544,544,544,544,544,544,544,544,544,544,
  544,544,544,544,544,544,544,544,544,544,544,544,544,544,544,544,544,544,
  544,544,544,544,544,544,544,544,544,544,544,544,544,544,544,544,544,544,
  544,544,544,544,544,544,544,544,544,544,544,544,544,544,544,544,544,544,
  544,544,544,544,544,1,544,391,2,2,2,636,
544,544,544,544,544,544,544,544,544,544,544,544,544,544,544,544,544,544,544,
  544,544,544,544,544,544,544,544,544,544,544,544,544,544,544,544,544,544,
  544,544,544,544,544,544,544,544,544,544,544,544,544,544,544,544,544,544,
  544,544,544,544,544,544,544,544,544,544,544,544,544,544,544,544,544,544,
  544,544,544,544,544,544,544,544,544,544,544,544,544,544,544,544,544,544,
  544,544,544,544,544,1,544,392,2,2,2,635,
544,544,544,544,544,544,544,544,544,544,544,544,544,544,544,544,544,544,544,
  544,544,544,544,544,544,544,544,544,544,544,544,544,544,544,544,544,544,
  544,544,544,544,544,544,544,544,544,544,544,544,544,544,544,544,544,544,
  544,544,544,544,544,544,544,544,544,544,544,544,544,544,544,544,544,544,
  544,544,544,544,544,544,544,544,544,544,544,544,544,544,544,544,544,544,
  544,544,544,544,544,1,544,393,2,2,2,639,
544,544,544,544,544,544,544,544,544,544,544,544,544,544,544,544,544,544,544,
  544,544,544,544,544,544,544,544,544,544,544,544,544,544,544,544,544,544,
  544,544,544,544,544,544,544,544,544,544,544,544,544,544,544,544,544,544,
  544,544,544,544,544,544,544,544,544,544,544,544,544,544,544,544,544,544,
  544,544,544,544,544,544,544,544,544,544,544,544,544,544,544,544,544,544,
  544,544,544,544,544,1,544,394,2,2,2,638,
544,544,544,544,544,544,544,544,544,544,544,544,544,544,544,544,544,544,544,
  544,544,544,544,544,544,544,544,544,544,544,544,544,544,544,544,544,544,
  544,544,544,544,544,544,544,544,544,544,544,544,544,544,544,544,544,544,
  544,544,544,544,544,544,544,544,544,544,544,544,544,544,544,544,544,544,
  544,544,544,544,544,544,544,544,544,544,544,544,544,544,544,544,544,544,
  544,544,544,544,544,1,544,395,2,2,2,637,
458,5,396,5,458,
459,5,397,5,459,
544,544,544,544,544,544,544,544,544,544,544,544,544,544,544,544,544,544,544,
  544,544,544,544,544,544,544,544,544,544,544,544,544,544,544,544,544,544,
  544,544,544,544,544,544,544,544,544,544,544,544,544,544,544,544,544,544,
  544,544,544,544,544,544,544,544,544,544,544,544,544,544,544,544,544,544,
  544,544,544,544,544,544,544,544,544,544,544,544,544,544,544,544,544,544,
  544,544,544,544,544,1,544,398,2,2,2,662,
460,5,399,5,460,
176,180,175,175,175,177,168,171,169,165,165,165,165,170,165,165,400,
186,188,189,187,190,192,401,
185,401,185,185,185,185,185,185,185,185,185,185,185,185,185,185,185,185,185,
  185,185,185,185,185,185,185,185,185,185,185,185,185,191,185,402,89,
178,178,178,178,403,
185,401,185,185,185,185,185,185,185,185,185,185,185,185,182,185,185,185,185,
  185,185,185,185,185,185,185,185,185,185,185,185,185,185,191,185,404,461,
83,83,83,83,83,83,83,174,174,342,341,345,83,350,351,352,353,354,355,356,61,
  39,366,67,343,344,184,82,405,162,152,348,349,163,346,164,347,462,267,
  376,375,374,373,372,371,371,371,370,369,368,151,371,367,365,364,363,362,
  361,360,359,267,358,357,
83,83,83,83,83,83,83,174,174,342,341,345,83,350,351,352,353,354,355,356,61,
  39,366,67,343,344,184,82,406,162,152,348,349,163,346,164,347,463,267,
  376,375,374,373,372,371,371,371,370,369,368,151,371,367,365,364,363,362,
  361,360,359,267,358,357,
83,83,83,83,83,83,83,174,174,342,341,345,83,350,351,352,353,354,355,356,61,
  39,366,67,343,344,184,82,407,162,152,348,349,163,346,164,347,464,267,
  376,375,374,373,372,371,371,371,370,369,368,151,371,367,365,364,363,362,
  361,360,359,267,358,357,
83,83,83,83,83,83,83,174,174,342,341,345,83,350,351,352,353,354,355,356,61,
  39,366,67,343,344,184,82,408,162,152,348,349,163,346,164,347,465,267,
  376,375,374,373,372,371,371,371,370,369,368,151,371,367,365,364,363,362,
  361,360,359,267,358,357,
83,83,83,83,83,83,83,174,174,342,341,345,83,350,351,352,353,354,355,356,61,
  39,366,67,343,344,184,82,409,162,152,348,349,163,346,164,347,466,267,
  376,375,374,373,372,371,371,371,370,369,368,151,371,367,365,364,363,362,
  361,360,359,267,358,357,
83,83,83,83,83,83,83,174,174,342,341,345,83,350,351,352,353,354,355,356,61,
  39,366,67,343,344,184,82,410,162,152,348,349,163,346,164,347,467,267,
  376,375,374,373,372,371,371,371,370,369,368,151,371,367,365,364,363,362,
  361,360,359,267,358,357,
83,83,83,83,83,83,83,174,174,342,341,345,83,350,351,352,353,354,355,356,61,
  39,366,67,343,344,184,82,411,162,152,348,349,163,346,164,347,468,267,
  376,375,374,373,372,371,371,371,370,369,368,151,371,367,365,364,363,362,
  361,360,359,267,358,357,
60,412,469,
544,544,544,544,544,544,544,544,544,544,544,544,544,544,544,544,544,544,544,
  544,544,544,544,544,544,1,544,413,2,2,2,621,
470,470,470,470,470,470,470,470,470,470,470,470,470,470,470,470,470,470,470,
  470,470,470,470,470,470,5,414,5,470,
544,544,544,544,544,544,544,544,544,544,544,544,544,544,544,544,544,544,544,
  544,544,544,544,544,544,1,544,415,2,2,2,620,
471,471,471,471,471,471,471,471,471,471,471,471,471,471,471,471,471,471,471,
  471,471,471,471,471,471,5,416,5,471,
472,472,472,472,472,472,472,472,472,472,472,472,472,472,472,472,472,472,472,
  472,472,472,472,472,472,5,417,5,472,
473,473,473,473,473,473,473,473,473,473,473,473,473,473,473,473,473,473,473,
  473,473,473,473,473,473,5,418,5,473,
474,474,474,474,474,474,474,474,474,474,474,474,474,474,474,474,474,474,474,
  474,474,474,474,474,474,5,419,5,474,
475,475,475,475,475,475,475,475,475,475,475,475,475,475,475,475,475,475,475,
  475,475,475,475,475,475,475,475,475,5,420,5,475,
476,476,476,476,476,476,476,476,476,476,476,476,476,476,476,476,476,476,476,
  476,476,476,476,476,476,476,476,476,5,421,5,476,
544,544,544,544,544,544,544,544,544,544,544,544,544,544,544,544,544,544,544,
  544,544,544,544,544,544,544,544,544,1,544,422,2,2,2,614,
477,477,477,477,477,477,477,477,477,477,477,477,477,477,477,477,477,477,477,
  477,477,477,477,477,477,477,477,477,5,423,5,477,
544,544,544,544,544,544,544,544,544,544,544,544,544,544,544,544,544,544,544,
  544,544,544,544,544,544,544,544,544,1,544,424,2,2,2,613,
478,478,478,478,478,478,478,478,478,478,478,478,478,478,478,478,478,478,478,
  478,478,478,478,478,478,478,478,478,5,425,5,478,
544,544,544,544,544,544,544,544,544,544,544,544,544,544,544,544,544,544,544,
  544,544,544,544,544,544,544,544,544,1,544,426,2,2,2,612,
479,479,479,479,479,479,479,479,479,479,479,479,479,479,479,479,479,479,479,
  479,479,479,479,479,479,479,479,479,5,427,5,479,
544,544,544,544,544,544,544,544,544,544,544,544,544,544,544,544,544,544,544,
  544,544,544,544,544,544,544,544,544,1,544,428,2,2,2,611,
480,480,480,480,480,480,480,480,480,480,480,480,480,480,480,480,480,480,480,
  480,480,480,480,480,480,480,480,480,5,429,5,480,
544,544,544,544,544,544,544,544,544,544,544,544,544,544,544,544,544,544,544,
  544,544,544,544,544,544,544,544,544,1,544,430,2,2,2,610,
481,481,481,481,481,481,481,481,481,481,481,481,481,481,481,481,481,481,481,
  481,481,481,481,481,481,481,481,481,5,431,5,481,
482,482,482,482,482,482,482,482,482,482,482,482,482,482,482,482,482,482,482,
  482,482,482,482,482,482,482,482,482,5,432,5,482,
544,544,544,544,544,544,544,544,544,544,544,544,544,544,544,544,544,544,544,
  544,544,544,544,544,544,544,544,544,1,544,433,2,2,2,608,
483,483,483,483,483,483,483,483,483,483,483,483,483,483,483,483,483,483,483,
  483,483,483,483,483,483,483,483,483,5,434,5,483,
484,484,484,484,484,484,484,484,484,484,484,484,484,484,484,484,484,484,484,
  484,484,484,484,484,484,484,484,484,5,435,5,484,
544,544,544,544,544,544,544,544,544,544,544,544,544,544,544,544,544,544,544,
  544,544,544,544,544,544,544,544,544,1,544,436,2,2,2,606,
485,485,485,485,485,485,485,485,485,485,485,485,485,485,485,485,485,485,485,
  485,485,485,485,485,485,485,485,485,5,437,5,485,
486,486,486,486,486,486,486,486,486,486,486,486,486,486,486,486,486,486,486,
  486,486,486,486,486,486,486,486,486,5,438,5,486,
544,544,544,544,544,544,544,544,544,544,544,544,544,544,544,544,544,544,544,
  544,544,544,544,544,544,544,544,544,1,2,439,2,2,2,604,
544,544,544,544,544,544,544,544,544,544,544,544,544,544,544,544,544,544,544,
  544,544,544,544,544,544,544,544,544,1,2,440,2,2,2,603,
544,544,544,544,544,544,544,544,544,544,544,544,544,544,544,544,544,544,544,
  544,544,544,544,544,544,544,544,544,1,2,441,2,2,2,602,
544,544,544,544,544,544,544,544,544,544,544,544,544,544,544,544,544,544,544,
  544,544,544,544,544,544,544,544,544,1,2,442,2,2,2,601,
544,544,544,544,544,544,544,544,544,544,544,544,544,544,544,544,544,544,544,
  544,544,544,544,544,544,544,544,544,1,2,443,2,2,2,600,
544,544,544,544,544,544,544,544,544,544,544,544,544,544,544,544,544,544,544,
  544,544,544,544,544,544,544,544,544,1,2,444,2,2,2,599,
544,544,544,544,544,544,544,544,544,544,544,544,544,544,544,544,544,544,544,
  544,544,544,544,544,544,544,544,544,1,2,445,2,2,2,598,
544,544,544,544,544,544,544,544,544,544,544,544,544,544,544,544,544,544,544,
  544,544,544,544,544,544,544,544,544,1,2,446,2,2,2,597,
544,544,544,544,544,544,544,544,544,544,544,544,544,544,544,544,544,544,544,
  544,544,544,544,544,544,544,544,544,1,2,447,2,2,2,596,
544,544,544,544,544,544,544,544,544,544,544,544,544,544,544,544,544,544,544,
  544,544,544,544,544,544,544,544,544,1,2,448,2,2,2,595,
544,544,544,544,544,544,544,544,544,544,544,544,544,544,544,544,544,544,544,
  544,544,544,544,544,544,544,544,544,1,2,449,2,2,2,594,
544,544,544,544,544,544,544,544,544,544,544,544,544,544,544,544,544,544,544,
  544,544,544,544,544,544,544,544,544,1,2,450,2,2,2,593,
544,544,544,544,544,544,544,544,544,544,544,544,544,544,544,544,544,544,544,
  544,544,544,544,544,544,544,544,544,1,2,451,2,2,2,592,
487,452,488,
185,401,185,185,185,185,185,185,185,185,185,185,185,185,182,185,185,185,185,
  185,185,185,185,185,185,185,185,185,185,185,185,185,185,191,185,453,489,
487,54,54,54,54,454,490,
487,52,52,52,52,455,490,
487,53,53,53,53,456,490,
487,51,51,51,51,457,490,
487,458,491,
487,459,492,
10,460,493,
183,461,
60,462,161,
60,463,160,
60,464,159,
60,465,158,
60,466,157,
60,467,156,
60,468,155,
17,17,17,17,17,17,17,17,17,17,17,17,17,17,17,17,17,17,17,17,17,17,17,17,17,
  17,17,17,17,17,17,17,17,17,17,17,17,17,17,17,17,17,17,17,17,17,17,17,17,
  17,17,17,17,17,17,17,17,17,17,17,17,17,17,17,17,17,17,17,17,17,17,17,17,
  17,17,17,17,17,17,17,17,17,17,17,17,17,17,17,17,17,17,17,17,17,17,17,17,
  17,17,17,17,17,17,17,17,17,17,17,17,17,17,17,17,17,17,17,17,17,17,17,17,
  17,17,17,17,17,17,17,5,469,5,154,
83,83,83,83,83,83,83,174,174,342,341,345,83,350,351,352,353,354,355,356,61,
  343,344,184,82,470,162,152,348,349,163,346,164,347,267,146,151,146,367,
  365,364,363,362,361,360,359,267,358,357,
83,83,83,83,83,83,83,174,174,342,341,345,83,350,351,352,353,354,355,356,61,
  343,344,184,82,471,162,152,348,349,163,346,164,347,267,145,151,145,367,
  365,364,363,362,361,360,359,267,358,357,
83,83,83,83,83,83,83,174,174,342,341,345,83,350,351,352,353,354,355,356,61,
  343,344,184,82,472,162,152,348,349,163,346,164,347,267,144,151,144,367,
  365,364,363,362,361,360,359,267,358,357,
83,83,83,83,83,83,83,174,174,342,341,345,83,350,351,352,353,354,355,356,61,
  343,344,184,82,473,162,152,348,349,163,346,164,347,267,143,151,143,367,
  365,364,363,362,361,360,359,267,358,357,
83,83,83,83,83,83,83,174,174,342,341,345,83,350,351,352,353,354,355,356,61,
  343,344,184,82,474,162,152,348,349,163,346,164,347,267,142,151,142,367,
  365,364,363,362,361,360,359,267,358,357,
83,83,83,83,83,83,83,174,174,342,341,345,83,350,351,352,353,354,355,356,61,
  39,366,67,343,344,184,82,475,162,152,348,349,163,346,164,347,267,494,
  494,494,370,369,368,151,494,367,365,364,363,362,361,360,359,267,358,357,
83,83,83,83,83,83,83,174,174,342,341,345,83,350,351,352,353,354,355,356,61,
  39,366,67,343,344,184,82,476,162,152,348,349,163,346,164,347,267,495,
  495,495,370,369,368,151,495,367,365,364,363,362,361,360,359,267,358,357,
83,83,83,83,83,83,83,174,174,342,341,345,83,350,351,352,353,354,355,356,61,
  39,366,67,343,344,184,82,477,162,152,348,349,163,346,164,347,267,496,
  371,371,371,370,369,368,151,371,367,365,364,363,362,361,360,359,267,358,
  357,
83,83,83,83,83,83,83,174,174,342,341,345,83,350,351,352,353,354,355,356,61,
  39,366,67,343,344,184,82,478,162,152,348,349,163,346,164,347,267,497,
  371,371,371,370,369,368,151,371,367,365,364,363,362,361,360,359,267,358,
  357,
83,83,83,83,83,83,83,174,174,342,341,345,83,350,351,352,353,354,355,356,61,
  39,366,67,343,344,184,82,479,162,152,348,349,163,346,164,347,267,498,
  371,371,371,370,369,368,151,371,367,365,364,363,362,361,360,359,267,358,
  357,
83,83,83,83,83,83,83,174,174,342,341,345,83,350,351,352,353,354,355,356,61,
  39,366,67,343,344,184,82,480,162,152,348,349,163,346,164,347,267,499,
  371,371,371,370,369,368,151,371,367,365,364,363,362,361,360,359,267,358,
  357,
83,83,83,83,83,83,83,174,174,342,341,345,83,350,351,352,353,354,355,356,61,
  39,366,67,343,344,184,82,481,162,152,348,349,163,346,164,347,267,500,
  372,371,371,371,370,369,368,151,371,367,365,364,363,362,361,360,359,267,
  358,357,
83,83,83,83,83,83,83,174,174,342,341,345,83,350,351,352,353,354,355,356,61,
  39,366,67,343,344,184,82,482,162,152,348,349,163,346,164,347,267,501,
  372,371,371,371,370,369,368,151,371,367,365,364,363,362,361,360,359,267,
  358,357,
83,83,83,83,83,83,83,174,174,342,341,345,83,350,351,352,353,354,355,356,61,
  39,366,67,343,344,184,82,483,162,152,348,349,163,346,164,347,267,502,
  373,372,371,371,371,370,369,368,151,371,367,365,364,363,362,361,360,359,
  267,358,357,
83,83,83,83,83,83,83,174,174,342,341,345,83,350,351,352,353,354,355,356,61,
  39,366,67,343,344,184,82,484,162,152,348,349,163,346,164,347,267,503,
  373,372,371,371,371,370,369,368,151,371,367,365,364,363,362,361,360,359,
  267,358,357,
83,83,83,83,83,83,83,174,174,342,341,345,83,350,351,352,353,354,355,356,61,
  39,366,67,343,344,184,82,485,162,152,348,349,163,346,164,347,267,504,
  374,373,372,371,371,371,370,369,368,151,371,367,365,364,363,362,361,360,
  359,267,358,357,
83,83,83,83,83,83,83,174,174,342,341,345,83,350,351,352,353,354,355,356,61,
  39,366,67,343,344,184,82,486,162,152,348,349,163,346,164,347,267,505,
  374,373,372,371,371,371,370,369,368,151,371,367,365,364,363,362,361,360,
  359,267,358,357,
544,544,544,544,544,544,544,544,544,544,544,544,544,544,544,544,544,544,544,
  544,544,544,544,544,544,544,544,544,544,544,544,544,544,544,544,544,544,
  1,544,487,2,2,2,591,
506,506,506,506,506,506,506,506,506,5,488,5,506,
89,89,89,89,89,89,89,89,89,89,89,89,89,89,183,89,89,89,89,89,89,89,89,89,89,
  89,89,89,89,89,89,89,89,89,89,489,
507,507,507,507,507,507,507,507,507,507,507,507,507,507,507,507,507,507,507,
  507,507,507,507,507,507,507,507,507,507,5,490,5,507,
508,508,508,508,508,508,508,508,508,508,508,508,508,508,508,508,508,508,508,
  508,508,508,508,508,508,508,508,508,5,491,5,508,
509,509,509,509,509,509,509,509,5,492,5,509,
510,510,510,510,510,510,510,510,510,510,510,510,510,510,510,510,510,510,510,
  510,510,510,510,510,510,510,510,510,5,493,5,510,
140,140,140,140,140,140,140,140,140,140,140,140,140,140,140,140,140,140,140,
  140,140,140,140,140,140,140,140,140,140,140,140,140,140,140,140,140,140,
  140,140,140,140,140,140,140,140,140,140,140,140,140,140,140,140,140,140,
  140,140,140,140,140,140,140,140,140,140,140,140,140,140,140,140,140,140,
  140,140,140,140,140,140,140,140,140,140,140,140,140,140,140,140,140,140,
  140,140,413,415,64,55,59,140,140,140,140,140,140,140,140,140,140,140,
  140,140,140,140,140,140,140,140,140,140,140,140,140,140,140,140,140,140,
  140,140,494,419,418,417,416,414,
139,139,139,139,139,139,139,139,139,139,139,139,139,139,139,139,139,139,139,
  139,139,139,139,139,139,139,139,139,139,139,139,139,139,139,139,139,139,
  139,139,139,139,139,139,139,139,139,139,139,139,139,139,139,139,139,139,
  139,139,139,139,139,139,139,139,139,139,139,139,139,139,139,139,139,139,
  139,139,139,139,139,139,139,139,139,139,139,139,139,139,139,139,139,139,
  139,139,413,415,64,55,59,139,139,139,139,139,139,139,139,139,139,139,
  139,139,139,139,139,139,139,139,139,139,139,139,139,139,139,139,139,139,
  139,139,495,419,418,417,416,414,
137,137,137,137,137,137,137,137,137,137,137,137,137,137,137,137,137,137,137,
  137,137,137,137,137,137,137,137,137,137,137,137,137,137,137,137,137,137,
  137,137,137,137,137,137,137,137,137,137,137,137,137,137,137,137,137,137,
  137,137,137,137,137,137,137,137,137,137,137,137,137,137,137,137,137,137,
  137,137,137,137,137,137,137,137,137,137,137,137,137,137,137,137,137,137,
  137,137,57,58,137,137,137,137,137,137,137,137,137,137,137,137,137,137,
  137,137,137,137,137,137,137,137,137,137,137,137,137,137,137,496,421,420,
136,136,136,136,136,136,136,136,136,136,136,136,136,136,136,136,136,136,136,
  136,136,136,136,136,136,136,136,136,136,136,136,136,136,136,136,136,136,
  136,136,136,136,136,136,136,136,136,136,136,136,136,136,136,136,136,136,
  136,136,136,136,136,136,136,136,136,136,136,136,136,136,136,136,136,136,
  136,136,136,136,136,136,136,136,136,136,136,136,136,136,136,136,136,136,
  136,136,57,58,136,136,136,136,136,136,136,136,136,136,136,136,136,136,
  136,136,136,136,136,136,136,136,136,136,136,136,136,136,136,497,421,420,
135,135,135,135,135,135,135,135,135,135,135,135,135,135,135,135,135,135,135,
  135,135,135,135,135,135,135,135,135,135,135,135,135,135,135,135,135,135,
  135,135,135,135,135,135,135,135,135,135,135,135,135,135,135,135,135,135,
  135,135,135,135,135,135,135,135,135,135,135,135,135,135,135,135,135,135,
  135,135,135,135,135,135,135,135,135,135,135,135,135,135,135,135,135,135,
  135,135,57,58,135,135,135,135,135,135,135,135,135,135,135,135,135,135,
  135,135,135,135,135,135,135,135,135,135,135,135,135,135,135,498,421,420,
134,134,134,134,134,134,134,134,134,134,134,134,134,134,134,134,134,134,134,
  134,134,134,134,134,134,134,134,134,134,134,134,134,134,134,134,134,134,
  134,134,134,134,134,134,134,134,134,134,134,134,134,134,134,134,134,134,
  134,134,134,134,134,134,134,134,134,134,134,134,134,134,134,134,134,134,
  134,134,134,134,134,134,134,134,134,134,134,134,134,134,134,134,134,134,
  134,134,57,58,134,134,134,134,134,134,134,134,134,134,134,134,134,134,
  134,134,134,134,134,134,134,134,134,134,134,134,134,134,134,499,421,420,
132,132,132,132,132,132,132,132,132,132,132,132,132,132,132,132,132,132,132,
  132,132,132,132,132,132,132,132,132,132,132,132,132,132,132,132,132,132,
  132,132,132,132,132,132,132,132,132,132,132,132,132,132,132,132,132,132,
  132,132,132,132,132,132,132,132,132,132,132,132,132,132,132,132,132,132,
  132,132,132,132,132,132,132,132,132,132,132,132,132,132,132,132,132,132,
  132,132,422,424,426,428,132,132,132,132,132,132,132,132,132,132,132,132,
  132,132,132,132,132,132,132,132,132,132,132,132,132,500,429,427,425,423,
131,131,131,131,131,131,131,131,131,131,131,131,131,131,131,131,131,131,131,
  131,131,131,131,131,131,131,131,131,131,131,131,131,131,131,131,131,131,
  131,131,131,131,131,131,131,131,131,131,131,131,131,131,131,131,131,131,
  131,131,131,131,131,131,131,131,131,131,131,131,131,131,131,131,131,131,
  131,131,131,131,131,131,131,131,131,131,131,131,131,131,131,131,131,131,
  131,131,422,424,426,428,131,131,131,131,131,131,131,131,131,131,131,131,
  131,131,131,131,131,131,131,131,131,131,131,131,131,501,429,427,425,423,
129,129,129,129,129,129,129,129,129,129,129,129,129,129,129,129,129,129,129,
  129,129,129,129,129,129,129,129,129,129,129,129,129,129,129,129,129,129,
  129,129,129,129,129,129,129,129,129,129,129,129,129,129,129,129,129,129,
  129,129,129,129,129,129,129,129,129,129,129,129,129,129,129,129,129,129,
  129,129,129,129,129,129,129,129,129,129,129,129,129,129,129,129,129,129,
  129,129,430,63,129,129,129,129,129,129,129,129,129,129,129,129,129,129,
  129,129,129,129,129,129,129,129,129,502,432,431,
128,128,128,128,128,128,128,128,128,128,128,128,128,128,128,128,128,128,128,
  128,128,128,128,128,128,128,128,128,128,128,128,128,128,128,128,128,128,
  128,128,128,128,128,128,128,128,128,128,128,128,128,128,128,128,128,128,
  128,128,128,128,128,128,128,128,128,128,128,128,128,128,128,128,128,128,
  128,128,128,128,128,128,128,128,128,128,128,128,128,128,128,128,128,128,
  128,128,430,63,128,128,128,128,128,128,128,128,128,128,128,128,128,128,
  128,128,128,128,128,128,128,128,128,503,432,431,
126,126,126,126,126,126,126,126,126,126,126,126,126,126,126,126,126,126,126,
  126,126,126,126,126,126,126,126,126,126,126,126,126,126,126,126,126,126,
  126,126,126,126,126,126,126,126,126,126,126,126,126,126,126,126,126,126,
  126,126,126,126,126,126,126,126,126,126,126,126,126,126,126,126,126,126,
  126,126,126,126,126,126,126,126,126,126,126,126,126,126,126,126,126,126,
  126,126,433,42,126,126,126,126,126,126,126,126,126,126,126,126,126,126,
  126,126,126,126,126,126,126,504,435,434,
125,125,125,125,125,125,125,125,125,125,125,125,125,125,125,125,125,125,125,
  125,125,125,125,125,125,125,125,125,125,125,125,125,125,125,125,125,125,
  125,125,125,125,125,125,125,125,125,125,125,125,125,125,125,125,125,125,
  125,125,125,125,125,125,125,125,125,125,125,125,125,125,125,125,125,125,
  125,125,125,125,125,125,125,125,125,125,125,125,125,125,125,125,125,125,
  125,125,433,42,125,125,125,125,125,125,125,125,125,125,125,125,125,125,
  125,125,125,125,125,125,125,505,435,434,
83,83,83,83,83,83,83,83,82,506,80,267,267,
83,83,83,83,83,83,83,174,174,342,341,383,83,92,350,351,352,353,354,355,356,
  61,39,366,67,343,344,184,82,507,162,152,348,349,163,346,77,272,164,347,
  339,78,76,267,376,375,374,373,372,371,371,371,370,369,368,151,371,367,
  365,364,363,362,361,360,359,273,267,340,358,357,
83,83,83,83,83,83,83,174,174,342,341,345,83,350,351,352,353,354,355,356,61,
  39,366,67,343,344,184,82,508,162,152,348,349,163,346,164,347,271,267,
  376,375,374,373,372,371,371,371,370,369,368,151,371,367,365,364,363,362,
  361,360,359,267,358,357,
204,201,199,197,203,202,200,198,509,270,390,
83,83,83,83,83,83,83,174,174,342,341,345,83,350,351,352,353,354,355,356,61,
  39,366,67,343,344,184,82,510,162,152,348,349,163,346,164,347,269,267,
  376,375,374,373,372,371,371,371,370,369,368,151,371,367,365,364,363,362,
  361,360,359,267,358,357,

};


static const unsigned short ag_sbt[] = {
     0, 141, 178, 183, 327, 364, 366, 369, 527, 670, 812, 863, 904, 945,
  1016,1087,1158,1178,1194,1210,1221,1232,1248,1264,1273,1282,1304,1429,
  1656,1667,1680,1693,1706,1719,1724,1733,1757,1878,1888,1929,1979,2030,
  2071,2122,2163,2204,2245,2286,2327,2368,2409,2450,2491,2532,2665,2798,
  2848,2889,2940,2991,3041,3197,3248,3289,3340,3390,3431,3472,3614,3655,
  3723,3726,3729,3732,3835,3870,3885,3988,4023,4038,4141,4150,4185,4288,
  4391,4494,4529,4564,4579,4682,4691,4794,4897,5000,5103,5206,5309,5412,
  5515,5618,5721,5824,5927,6030,6133,6145,6157,6260,6295,6330,6345,6448,
  6463,6478,6489,6592,6627,6662,6697,6706,6741,6776,6811,6846,6881,6916,
  6951,6986,7021,7056,7091,7126,7161,7196,7207,7222,7257,7360,7463,7566,
  7669,7680,7695,7706,7809,7844,7879,7914,7949,7984,8019,8054,8069,8172,
  8275,8310,8345,8380,8483,8518,8533,8568,8583,8598,8633,8665,8677,8709,
  8721,8727,8759,8791,8823,8835,8841,8850,8859,8891,8923,8935,8947,8959,
  8967,8999,9031,9063,9069,9101,9133,9165,9197,9229,9261,9293,9325,9357,
  9389,9421,9453,9485,9517,9525,9537,9569,9577,9589,9597,9629,9661,9693,
  9725,9757,9789,9821,9833,9865,9897,9929,9961,9973,10005,10017,10029,
  10061,10161,10177,10190,10210,10226,10237,10248,10255,10264,10273,10284,
  10295,10330,10395,10415,10432,10467,10530,10546,10559,10575,10589,10605,
  10619,10635,10649,10685,10756,10792,10863,10898,10961,10972,10983,10994,
  11029,11092,11163,11234,11297,11332,11395,11430,11493,11537,11548,11559,
  11593,11604,11639,11651,11674,11737,11800,11863,11874,11937,11948,11954,
  12017,12080,12143,12154,12159,12171,12183,12246,12309,12320,12331,12342,
  12349,12412,12475,12538,12544,12607,12670,12733,12796,12859,12922,12985,
  13048,13111,13174,13237,13300,13363,13426,13433,13444,13507,13514,13525,
  13532,13595,13658,13721,13784,13847,13910,13973,13984,14047,14110,14173,
  14236,14247,14310,14321,14332,14395,14581,14596,14632,14669,14681,14685,
  14828,14832,14836,14872,14876,14882,15017,15130,15138,15146,15154,15162,
  15170,15178,15186,15321,15456,15459,15462,15465,15468,15471,15474,15477,
  15509,15572,15621,15670,15719,15730,15735,15744,15749,15754,15759,15797,
  15860,15868,15877,15886,15895,15931,15940,15949,15958,15967,16003,16039,
  16143,16246,16349,16452,16555,16658,16663,16668,16771,16776,16793,16800,
  16836,16841,16878,16941,17004,17067,17130,17193,17256,17319,17322,17354,
  17383,17415,17444,17473,17502,17531,17563,17595,17630,17662,17697,17729,
  17764,17796,17831,17863,17898,17930,17962,17997,18029,18061,18096,18128,
  18160,18195,18230,18265,18300,18335,18370,18405,18440,18475,18510,18545,
  18580,18615,18618,18655,18662,18669,18676,18683,18686,18689,18692,18694,
  18697,18700,18703,18706,18709,18712,18715,18847,18896,18945,18994,19043,
  19092,19149,19206,19264,19322,19380,19438,19497,19556,19616,19676,19737,
  19798,19842,19855,19891,19924,19956,19968,20000,20135,20270,20397,20524,
  20651,20778,20905,21032,21153,21274,21393,21512,21525,21595,21658,21669,
  21732
};


static const unsigned short ag_sbe[] = {
   136, 176, 180, 320, 363, 365, 367, 504, 665, 807, 858, 899, 940, 980,
  1051,1122,1172,1189,1205,1216,1227,1243,1259,1268,1277,1299,1424,1541,
  1661,1676,1689,1702,1715,1721,1726,1748,1875,1882,1924,1974,2025,2066,
  2117,2158,2199,2240,2281,2322,2363,2404,2445,2486,2527,2660,2793,2843,
  2884,2935,2986,3036,3192,3243,3284,3335,3385,3426,3467,3609,3650,3689,
  3724,3727,3730,3830,3865,3880,3983,4018,4033,4136,4145,4180,4283,4386,
  4489,4524,4559,4574,4677,4686,4789,4892,4995,5098,5201,5304,5407,5510,
  5613,5716,5819,5922,6025,6128,6140,6152,6255,6290,6325,6340,6443,6458,
  6473,6484,6587,6622,6657,6692,6701,6736,6771,6806,6841,6876,6911,6946,
  6981,7016,7051,7086,7121,7156,7191,7202,7217,7252,7355,7458,7561,7664,
  7675,7690,7701,7804,7839,7874,7909,7944,7979,8014,8049,8064,8167,8270,
  8305,8340,8375,8478,8513,8528,8563,8578,8593,8628,8662,8674,8706,8718,
  8724,8756,8788,8820,8832,8838,8847,8856,8888,8920,8932,8944,8956,8964,
  8996,9028,9060,9066,9098,9130,9162,9194,9226,9258,9290,9322,9354,9386,
  9418,9450,9482,9514,9522,9534,9566,9574,9586,9594,9626,9658,9690,9722,
  9754,9786,9818,9830,9862,9894,9926,9958,9970,10002,10014,10026,10058,
  10158,10172,10186,10205,10223,10232,10243,10253,10259,10266,10279,10290,
  10325,10358,10410,10428,10462,10495,10541,10555,10570,10584,10600,10614,
  10630,10644,10680,10714,10787,10821,10893,10926,10967,10978,10989,11024,
  11057,11121,11192,11262,11327,11360,11425,11458,11531,11543,11554,11592,
  11599,11638,11646,11669,11702,11765,11828,11871,11902,11945,11950,11982,
  12045,12108,12151,12156,12164,12176,12211,12274,12317,12328,12339,12346,
  12377,12440,12503,12540,12572,12635,12698,12761,12824,12887,12950,13013,
  13076,13139,13202,13265,13328,13391,13430,13441,13472,13511,13522,13529,
  13560,13623,13686,13749,13812,13875,13938,13981,14012,14075,14138,14201,
  14244,14275,14318,14329,14360,14487,14589,14630,14667,14676,14684,14827,
  14831,14835,14870,14875,14881,15016,15129,15133,15141,15149,15157,15165,
  15173,15181,15316,15451,15457,15460,15463,15466,15469,15472,15475,15504,
  15537,15597,15646,15695,15724,15732,15739,15746,15751,15756,15777,15825,
  15865,15874,15883,15892,15929,15937,15946,15955,15964,16002,16038,16138,
  16241,16344,16447,16550,16653,16660,16665,16766,16773,16792,16799,16834,
  16840,16876,16906,16969,17032,17095,17158,17221,17284,17320,17349,17380,
  17410,17441,17470,17499,17528,17560,17592,17625,17659,17692,17726,17759,
  17793,17826,17860,17893,17927,17959,17992,18026,18058,18091,18125,18157,
  18190,18225,18260,18295,18330,18365,18400,18435,18470,18505,18540,18575,
  18610,18616,18653,18660,18667,18674,18681,18684,18687,18690,18693,18695,
  18698,18701,18704,18707,18710,18713,18844,18872,18921,18970,19019,19068,
  19120,19177,19234,19292,19350,19408,19466,19525,19584,19644,19704,19765,
  19837,19852,19890,19921,19953,19965,19997,20129,20264,20394,20521,20648,
  20775,20900,21027,21150,21271,21390,21509,21521,21554,21623,21666,21697,21732
};


static const unsigned char ag_fl[] = {
  2,1,1,2,2,1,2,0,1,3,3,3,1,2,3,1,2,0,1,3,3,2,2,2,2,1,2,2,2,2,2,2,2,2,2,
  1,1,2,2,3,3,3,3,0,1,3,2,2,2,3,3,4,4,4,4,3,3,3,3,3,2,3,4,2,2,3,3,4,2,2,
  3,3,3,1,1,1,5,5,5,1,5,1,1,1,2,2,1,2,2,3,2,2,1,2,3,3,2,1,2,3,3,2,2,1,1,
  1,1,2,1,1,2,1,1,2,1,1,2,1,1,2,1,1,2,1,1,4,4,1,4,4,1,4,4,1,4,4,4,4,1,4,
  4,1,4,4,4,4,4,1,2,2,2,1,1,1,4,4,4,4,4,4,4,4,1,1,1,1,1,2,2,2,2,2,2,2,1,
  2,2,2,3,2,2,2,3,4,1,1,2,2,2,2,2,1,2,1,2,2,2,1,1,1,1,1,1,1,1,1,1,1,1,1,
  1,1,1,1,1,1,1,3,3,3,3,3,3,3,3,1,3,3,3,1,1,3,3,3,3,3,3,3,3,1,3,3,3,1,1,
  1,1,3,3,3,3,3,3,3,3,3,3,3,3,3,3,3,3,3,3,3,3,3,1,7,7,7,1,3,3,3,1,3,3,1,
  1,1,1,1,1,1,1,1,1,1,1,1,1,3,1,3,3,3,1,1,1,3,3,1,3,3,1,3,3,1,1,1,1,1,1,
  1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,
  1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,
  1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,
  1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,
  1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,
  1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,
  1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,2,0,1,2,2,2,2,2,2,2,2,2,2,2,2,2,2,
  2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,
  2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,
  2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,
  2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,
  2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,
  2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2
};

static const unsigned short ag_ptt[] = {
    0, 24, 25, 25, 22, 30, 30, 31, 31, 27, 27, 27, 37, 37,  2, 39, 39, 40,
   40, 23, 23, 23, 23, 23, 23, 41, 41, 41, 41, 41, 41, 41, 41, 41, 41, 41,
   41, 41, 41, 41, 41, 41, 41, 58, 58, 41, 41, 41, 41, 41, 41, 41, 41, 41,
   41, 41, 41, 41, 41, 41, 41, 41, 41, 41, 41, 41, 41, 41, 41, 41, 41, 41,
   41, 65, 65, 65, 65, 65, 65, 68, 68,299,299, 85, 85, 85,294,294,324, 19,
   19,291, 13, 13, 13, 13,293, 15, 15, 15, 15,290, 75, 75,103,103,103, 99,
  106,106, 99,109,109, 99,112,112, 99,115,115, 99,118,118, 99, 56,119,119,
  119,120,120,120,123,123,123,126,126,126,126,126,129,129,129,134,134,134,
  134,134,134,137,137,137,137,139,139,139,139,148,148,148,148,148,148,148,
  147,147,326,326,326,326,326,326,326,326,  8,  8,  8,  8,  8,  8,  7,  7,
   18, 18, 16, 16, 16, 17, 17, 17, 17, 17, 17, 17, 17,374, 10, 10, 10,381,
  381,381,381,381,381,381,381,402,402,402,402,182,182,182,182,182,184,184,
   83, 83, 83,185,185,185,185,185,185,185,185,185,185,185,185,185,185,185,
  185,185,185,185,185,185,185,185,185,185,185,185,185,185,185,185,185,185,
  185,185,185,185,185,185,185,185,185,185,185,185,185,185,185,185,185,185,
  185,185,185,185,185,185,185,185,185,185,185,185,185,185,185,185,185,185,
  185,185,185,185,185,185,185,185,185,185,185,185,185,185,185,185,185,185,
  185,185,185,185, 32, 88, 29, 29, 29, 29, 29, 29, 29, 29, 29, 29, 29, 29,
   29, 29, 29, 29, 29, 29, 29, 29, 29, 29, 29, 29, 29, 29, 29, 29, 29, 29,
   29, 29, 29, 29, 36, 36, 36, 36, 36, 36, 36, 36, 36, 36, 36, 36, 36, 36,
   36, 36, 36, 36, 36, 36, 36, 36, 36, 36, 36, 36, 36, 36, 36, 36, 36, 36,
   36, 36, 36, 87, 87, 87, 87, 87, 87, 87, 87, 89, 89, 89, 89, 89, 89, 89,
   89, 89, 89, 89, 89, 89, 92, 92, 92, 92, 92, 92, 92, 92, 92, 92, 92, 92,
   92, 92, 92, 92, 92, 92, 92, 92, 92, 92, 92, 92, 92, 92, 92, 92, 92, 92,
   92, 92, 94, 94, 94, 94, 94, 94, 94, 94, 94, 94, 94, 94, 94, 94, 94, 94,
   94, 94, 94, 94, 94, 94, 94, 94, 94, 94, 94, 94, 94, 94, 94, 94, 94, 94,
   97, 97, 97, 97, 97, 97, 97, 97, 97, 97, 97, 97, 97, 97, 97, 97, 97, 97,
   97, 97, 97, 97, 97, 97, 97, 97, 97, 97, 97, 97, 97,162,162,162,167,167,
  167,167,170,170,170,170,170,170,170,170,170,170,170,170,170,170,170,170,
  170,170,170,170,170,170,170,170,170,170,170,170,170,170,170,170,261,261,
  283,283,284,284,285,285, 28, 33, 34, 42, 12, 11, 44, 14, 20, 45, 47, 46,
   48,  4, 49, 50, 51, 52, 53, 54, 55, 57, 59, 60, 61, 62, 63, 64, 66, 67,
   69, 70, 71, 72, 73, 74, 76, 77, 21, 78,  3, 79, 80, 81, 82, 84,100,101,
  102,104,105,107,108,110,111,113,114,116,117,121,122,124,125,127,128,130,
  131,132,133,135,136,138,140,141,142,143,144,145,146,150,149,151,152,153,
  154,155,156,157,  9,159,161,158,183,180,186,  5,187,188,189,190,191,192,
  193,194,195,196,197,198,199,200,201,202,203,204,205,206,  6,207,208,209,
  210,211,212,213,214,215,216,217,218,219,220,221,222,223,224,225,226,227,
  228,229,230,231,232,233,234,235,236,237,238,239,240,241,242,243,244,245,
  246,247,248,249,250,251,252,253,254,255,256,257,258,259,260,262,263,264,
  265,266,267,268,269,270,271,272,273,274,275,276,277,278, 91,164, 90,175,
  165,279,280, 96, 95,168,281,171,173,172,166, 93,163,282
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
    case 49: ag_rp_53(); break;
    case 50: ag_rp_54(V(0,int)); break;
    case 51: ag_rp_55(V(1,int)); break;
    case 52: ag_rp_56(V(1,int)); break;
    case 53: ag_rp_57(V(0,int)); break;
    case 54: ag_rp_58(V(1,int)); break;
    case 55: ag_rp_59(V(1,int), V(2,int)); break;
    case 56: ag_rp_60(V(1,int)); break;
    case 57: ag_rp_61(); break;
    case 58: ag_rp_62(V(1,int)); break;
    case 59: ag_rp_63(V(2,int)); break;
    case 60: ag_rp_64(); break;
    case 61: ag_rp_65(); break;
    case 62: ag_rp_66(V(1,int)); break;
    case 63: ag_rp_67(V(2,int)); break;
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
    case 92: ag_rp_96(); break;
    case 93: ag_rp_97(V(0,double)); break;
    case 94: ag_rp_98(); break;
    case 95: ag_rp_99(); break;
    case 96: ag_rp_100(); break;
    case 97: ag_rp_101(); break;
    case 98: ag_rp_102(); break;
    case 99: ag_rp_103(); break;
    case 100: ag_rp_104(); break;
    case 101: ag_rp_105(); break;
    case 102: V(0,double) = ag_rp_106(V(0,int)); break;
    case 103: V(0,double) = ag_rp_107(V(0,double)); break;
    case 104: V(0,int) = ag_rp_108(V(0,int)); break;
    case 105: V(0,int) = ag_rp_109(); break;
    case 106: V(0,int) = ag_rp_110(); break;
    case 107: V(0,int) = ag_rp_111(); break;
    case 108: V(0,int) = ag_rp_112(); break;
    case 109: V(0,int) = ag_rp_113(); break;
    case 110: V(0,int) = ag_rp_114(); break;
    case 111: V(0,int) = ag_rp_115(); break;
    case 112: ag_rp_116(V(1,int)); break;
    case 113: ag_rp_117(V(1,int)); break;
    case 114: ag_rp_118(V(0,int)); break;
    case 115: ag_rp_119(V(1,int)); break;
    case 116: ag_rp_120(V(2,int)); break;
    case 117: ag_rp_121(V(1,int)); break;
    case 118: ag_rp_122(V(1,int)); break;
    case 119: ag_rp_123(V(1,int)); break;
    case 120: V(0,int) = ag_rp_124(V(1,int)); break;
    case 121: V(0,int) = ag_rp_125(V(1,int), V(2,int)); break;
    case 122: V(0,int) = ag_rp_126(); break;
    case 123: V(0,int) = ag_rp_127(V(0,int)); break;
    case 124: V(0,int) = ag_rp_128(); break;
    case 125: V(0,int) = ag_rp_129(); break;
    case 126: V(0,int) = ag_rp_130(); break;
    case 127: V(0,int) = ag_rp_131(); break;
    case 128: V(0,int) = ag_rp_132(); break;
    case 129: V(0,int) = ag_rp_133(); break;
    case 130: V(0,int) = ag_rp_134(); break;
    case 131: V(0,double) = ag_rp_135(); break;
    case 132: V(0,double) = ag_rp_136(V(1,int)); break;
    case 133: V(0,double) = ag_rp_137(V(0,double), V(1,int)); break;
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
    case 225: ag_rp_229(); break;
    case 226: ag_rp_230(); break;
    case 227: ag_rp_231(V(2,int)); break;
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
    case 242: ag_rp_246(); break;
    case 243: ag_rp_247(); break;
  }
}

#define TOKEN_NAMES a85parse_token_names
const char *const a85parse_token_names[492] = {
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
  "",
  "literal string",
  "label",
  "",
  "include string",
  "",
  "ascii integer",
  "escape char",
  "asm hex value",
  "",
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
  "error",
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
  "",
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
  "",
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
    0,0,  2,1,  0,0,  0,0,  0,0,  0,0, 23,1, 23,1,  0,0,  0,0,  0,0,  0,0,
   27,1, 27,1, 27,1, 27,1, 41,1, 41,1, 41,1, 41,1, 41,1, 41,1, 41,1, 41,1,
   41,1, 41,1, 41,1, 41,1, 41,1, 41,1, 41,1, 41,1, 41,1, 41,1, 41,1, 41,1,
   41,1, 23,2, 29,1,  0,0,  0,0, 29,1,  0,0, 29,1, 29,1, 29,1, 29,1, 29,1,
   29,1, 29,1, 29,1, 29,1, 29,1,  0,0,  0,0,  0,0, 29,1,  0,0,  0,0,  0,0,
    0,0,  0,0, 29,1,  0,0,  0,0, 29,1, 29,1,  0,0, 29,1,  0,0, 27,2, 27,2,
   27,2,185,1,185,1,185,1,185,1,185,1,185,1,185,1,185,1,185,1,185,1,185,1,
  185,1,185,1,185,1,185,1,185,1,185,1,185,1,185,1,185,1,185,1,185,1,185,1,
  185,1,185,1,185,1,185,1,185,1,185,1,185,1,185,1,185,1,185,1,185,1,185,1,
  185,1,185,1,185,1,185,1,185,1,185,1,185,1,185,1,185,1,185,1,185,1,185,1,
  185,1,185,1,185,1,185,1,185,1,185,1,185,1,185,1,185,1,185,1,185,1,185,1,
  185,1,185,1,185,1,185,1,185,1,185,1,185,1,185,1,185,1,185,1,185,1,185,1,
  185,1,185,1,185,1,185,1,185,1,185,1,185,1,185,1,185,1,185,1,185,1,185,1,
  185,1,185,1,185,1,185,1,185,1,185,1,185,1,185,1,185,1,185,1,185,1,185,1,
  185,1,185,1,185,1,185,1,185,1,185,1,185,1,185,1,185,1,185,1,185,1,185,1,
  185,1,185,1,185,1,185,1,185,1,185,1,185,1,185,1,185,1,185,1,185,1,185,1,
  185,1,185,1,185,1,185,1,185,1,185,1,185,1,185,1,185,1,185,1,185,1,185,1,
  185,1,185,1,185,1,185,1,185,1,185,1,185,1,185,1,185,1,185,1,185,1,185,1,
  185,1,185,1,185,1,185,1,185,1,185,1,185,1, 83,1,  0,0, 41,2,  0,0, 41,2,
    0,0,  0,0, 41,2,  0,0, 41,2,  0,0,  0,0,  0,0, 41,2,  0,0, 41,2,  0,0,
   41,2,  0,0, 41,2,  0,0, 41,2,  0,0, 41,2,  0,0, 41,2,  0,0, 41,2,  0,0,
   41,2,  0,0, 41,2,  0,0,  0,0,  0,0,  0,0, 41,2, 41,2, 41,2, 41,2,  0,0,
   41,2,  0,0, 41,2,  0,0,  0,0,  0,0,293,1,  0,0,291,1,  0,0,  0,0, 41,2,
   41,2,185,2,185,2,185,2,185,2,185,2,185,2,185,2,185,2,185,2,185,2,185,2,
  185,2,185,2,185,2,185,2,185,2,185,2,185,2,185,2,185,2,185,2,185,2,185,2,
  185,2,185,2,185,2,185,2,185,2,185,2,185,2,185,2,185,2,185,2,185,2,185,2,
  185,2,185,2,185,2,185,2,185,2,185,2,185,2,185,2,185,2,185,2,185,2,185,2,
  185,2,185,2,185,2,185,2,185,2,185,2,185,2,185,2,185,2,185,2,185,2,185,2,
   83,2,  0,0,324,1,324,1,  0,0,374,1,  7,1,  8,1,  8,1, 16,1,374,1,326,1,
    7,1,147,1,148,1,148,1,148,1,148,1,148,1,148,1,148,1,147,1,  0,0,148,1,
  148,1,148,1,148,1,148,1,148,1,148,1,137,1,139,1,137,1,137,1,137,1,134,1,
  129,1,126,1,123,1,120,1,119,1, 75,1, 75,1, 41,3, 68,1, 68,1, 68,1,  0,0,
    0,0,  0,0,  0,0,  0,0,293,2,291,2,  0,0,  0,0,  0,0,182,1,182,1,182,1,
  185,3,185,3,  0,0,185,3,326,1, 17,1,324,2,  7,2, 16,2,148,2,148,2,148,2,
  148,2,148,2,148,2,148,2,139,2,  0,0,134,2,  0,0,134,2,134,2,134,2,134,2,
  129,2,129,2,  0,0,126,2,  0,0,126,2,  0,0,126,2,  0,0,126,2,  0,0,123,2,
  123,2,  0,0,120,2,120,2,  0,0,119,2,119,2,  0,0,  0,0,  0,0,  0,0,  0,0,
    0,0,  0,0,  0,0,  0,0,  0,0,  0,0,  0,0,  0,0, 68,2,  0,0,  0,0,  0,0,
    0,0,  0,0,185,4,185,4,185,4, 16,3,148,3,148,3,148,3,148,3,148,3,148,3,
  148,3,139,3,134,3,134,3,134,3,134,3,134,3,129,3,129,3,126,3,126,3,126,3,
  126,3,123,3,123,3,120,3,120,3,119,3,119,3,  0,0, 68,3, 16,3,  0,0,185,5,
  185,5,185,5,134,1,134,1,129,1,129,1,129,1,129,1,126,1,126,1,123,1,123,1,
  120,1,120,1, 68,4,  0,0,185,6,185,6,185,6
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


{
  int ag_sx, ag_t;

  ag_sx = (PCB).ssx;
  (PCB).ss[ag_sx] = (PCB).sn;
  do {
    while (ag_sx && ag_ctn[2*(ag_snd = (PCB).ss[ag_sx])] == 0) ag_sx--;
    if (ag_sx) {
      ag_t = ag_ctn[2*ag_snd];
      ag_sx -= ag_ctn[2*ag_snd +1];
      ag_snd = (PCB).ss[ag_sx];
    }
    else {
      ag_snd = 0;
      ag_t = ag_ptt[0];
    }
  } while (ag_sx && *TOKEN_NAMES[ag_t]==0);
  if (*TOKEN_NAMES[ag_t] == 0) ag_t = 0;
  (PCB).error_frame_ssx = ag_sx;
  (PCB).error_frame_token = (a85parse_token_type) ag_t;
}


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


static int ag_action_1_er_proc(void);
static int ag_action_2_er_proc(void);
static int ag_action_3_er_proc(void);
static int ag_action_4_er_proc(void);

static int (*const  ag_er_procs_scan[])(void) = {
  ag_action_1_er_proc,
  ag_action_2_er_proc,
  ag_action_3_er_proc,
  ag_action_4_er_proc
};


static void ag_error_resynch(void) {
  int ag_k;
  int ag_ssx = (PCB).ssx;

  ag_diagnose();
  SYNTAX_ERROR;
  if ((PCB).exit_flag != AG_RUNNING_CODE) return;
  while (1) {
    ag_k = ag_sbt[(PCB).sn];
    while (ag_tstt[ag_k] != 43 && ag_tstt[ag_k]) ag_k++;
    if (ag_tstt[ag_k] || (PCB).ssx == 0) break;
    (PCB).sn = (PCB).ss[--(PCB).ssx];
  }
  if (ag_tstt[ag_k] == 0) {
    (PCB).sn = PCB.ss[(PCB).ssx = ag_ssx];
    (PCB).exit_flag = AG_SYNTAX_ERROR_CODE;
    return;
  }
  ag_k = ag_sbt[(PCB).sn];
  while (ag_tstt[ag_k] != 43 && ag_tstt[ag_k]) ag_k++;
  (PCB).ag_ap = ag_pstt[ag_k];
  (ag_er_procs_scan[ag_astt[ag_k]])();
  while (1) {
    ag_k = ag_sbt[(PCB).sn];
    while (ag_tstt[ag_k] != (const unsigned short) (PCB).token_number && ag_tstt[ag_k])
      ag_k++;
    if (ag_tstt[ag_k] && ag_astt[ag_k] != ag_action_10) break;
    if ((PCB).token_number == 26)
       {(PCB).exit_flag = AG_SYNTAX_ERROR_CODE; return;}
    {(PCB).rx = 1; ag_track();}
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
  (PCB).rx = 0;
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
    if ((ag_s_procs_scan[ag_astt[ag_t1]])() == 0) break;
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
    if ((ag_s_procs_scan[ag_astt[ag_t1]])() == 0) break;
  }
  return 0;
}

static int ag_action_8_proc(void) {
  int ag_k = ag_sbt[(PCB).sn];
  while (ag_tstt[ag_k] != 43 && ag_tstt[ag_k]) ag_k++;
  if (ag_tstt[ag_k] == 0) ag_undo();
  (PCB).rx = 0;
  ag_error_resynch();
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
    if ((ag_r_procs_scan[ag_astt[ag_t1]])() == 0) break;
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
    if ((ag_r_procs_scan[ag_astt[ag_t1]])() == 0) break;
  }
  return (PCB).exit_flag == AG_RUNNING_CODE;
}


static int ag_action_2_er_proc(void) {
  (PCB).btsx = 0, (PCB).drt = -1;
  (*(int *) &(PCB).vs[(PCB).ssx]) = *(PCB).lab;
  (PCB).ssx++;
  (PCB).sn = (PCB).ag_ap;
  return 0;
}

static int ag_action_1_er_proc(void) {
  (PCB).btsx = 0, (PCB).drt = -1;
  (PCB).exit_flag = AG_SUCCESS_CODE;
  return 0;
}

static int ag_action_4_er_proc(void) {
  int ag_sd = ag_fl[(PCB).ag_ap] - 1;
  (PCB).btsx = 0, (PCB).drt = -1;
  (PCB).reduction_token = (a85parse_token_type) ag_ptt[(PCB).ag_ap];
  if (ag_sd) (PCB).sn = (PCB).ss[(PCB).ssx -= ag_sd];
  else (PCB).ss[(PCB).ssx] = (PCB).sn;
  while ((PCB).exit_flag == AG_RUNNING_CODE) {
    unsigned ag_t1 = ag_sbe[(PCB).sn] + 1;
    unsigned ag_t2 = ag_sbt[(PCB).sn+1] - 1;
    do {
      unsigned ag_tx = (ag_t1 + ag_t2)/2;
      if (ag_tstt[ag_tx] < (const unsigned short)(PCB).reduction_token) ag_t1 = ag_tx + 1;
      else ag_t2 = ag_tx;
    } while (ag_t1 < ag_t2);
    (PCB).ag_ap = ag_pstt[ag_t1];
    if ((ag_s_procs_scan[ag_astt[ag_t1]])() == 0) break;
  }
  return 0;
}

static int ag_action_3_er_proc(void) {
  int ag_sd = ag_fl[(PCB).ag_ap] - 1;
  (PCB).btsx = 0, (PCB).drt = -1;
  if (ag_sd) (PCB).sn = (PCB).ss[(PCB).ssx -= ag_sd];
  else (PCB).ss[(PCB).ssx] = (PCB).sn;
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
    if ((ag_s_procs_scan[ag_astt[ag_t1]])() == 0) break;
  }
  return 0;
}


void init_a85parse(void) {
  (PCB).rx = (PCB).fx = 0;
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
    (ag_gt_procs_scan[ag_astt[ag_t1]])();
  }
}


