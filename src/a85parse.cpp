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

#ifndef A85PARSE_H_1357391036
#include "a85parse.h"
#endif

#ifndef A85PARSE_H_1357391036
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

/*  Line 695, C:/Projects/VirtualT/src/a85parse.syn */
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
CMacro*					gMacro = 0;				// Pointer to active macro
MStringArray*			gNameList = 0;			// Pointer to active name list
int						gDefine = 0;			// Indicated in a #define reduction
CCondition*				gCond = 0;				// Pointer to active condition
const char*				gFilename = 0;			// Pointer to the filename
char					ss[32][256];			// String Stack;
CMacro*					gMacroStack[32];		// Macro Stack;
int						ms_idx = 0;				// Macro Stack Index
char					integer[64];			// Integer storage space
char					int_len = 0;			// Integer string length
int						ss_idx = 0;				// String Stack Index
int						ss_len = 0;				// SS string length 
int						ss_addr = 0;			// Address at start of literal name
double                  gDivisor = 1.0;			// Current divisor for float converwions
int						gTabSize = 4;
char					reg[10];				// Register arguments
int						reg_cnt = 0;			// Register Arg count
int						label = 0;				// Number of labels on string stack
int						name_list_cnt = 0;		// Number of strings in name list
int						ex_cnt = 0;				// Number of expressions in expression list
int						gAbort = 0;				// Abort on #error
int						page, seg;
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
		int		c;
		
		// Check string accumulator size
		int stringCount;
		if ((stringCount = ss_idx) != 0)
		{
			MString string;
			string.Format("Internal Design Parse Error - %d string(s) left on stack!",
				stringCount);
			gAsm->m_Errors.Add(string);
			for (c = 0; c < stringCount; c++)
			{
				string.Format("    %s", ss[ss_idx--]);
				gAsm->m_Errors.Add(string);
			}
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

	// Make the given assembler a global
	gAsm = pAsm;

	// Insure no active equation left over from a previous run
	if (gEq != 0)
		delete gEq;
	if (gExpList != 0)
		delete gExpList;
	if (gNameList != 0)
		delete gNameList;
	if (gCond != 0)
		delete gCond;
	if (gMacro != 0)
		delete gMacro;

	// Allocate an active equation to be used during parsing
	gEq = new CRpnEquation;
	gExpList = new VTObArray;
	gNameList = new MStringArray;
	gCond = new CCondition;
	gMacro = new CMacro;
	gAsm->m_FileIndex = gAsm->m_Filenames.Add(filename);
	gFilename = filename;

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
	gAbort = 0;
	
	// Reset the Macro stack
	for (int c = 0; c < 32; c++)
		gMacroStack[c] = 0;
	ms_idx = 0;

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

void handle_error(void)
{
	char	msg[512];
	
	sprintf(msg, "Error token = %d", (PCB).error_frame_token);
}

void syntax_error(const char *token_name)
{
	MString		string;
	int			eolMsg;
	int			expected_immediate = 0;
	int			expected_reg = 0;

	// Determine if this is the "unexpected eol" message
	eolMsg = !strcmp("Unexpected '\\n'", (PCB).error_message);

	switch ((PCB).error_frame_token)
	{
	// Handle general statement syntax error
	case a85parse_statement_token:
	    string.Format("Error in line %d(%s), column %d:  Malformed %s - %s",
			(PCB).line, gFilename, (PCB).column,
			token_name, (PCB).error_message);
		break;

	// Handle improper LXI instruction error
	case a85parse_lxi_inst_start_token:
		if (reg_cnt == 0)
			expected_reg = 16;
		else
			expected_immediate = 1;
		break;

	// Handle improper MVI instruction error
	case a85parse_mvi_inst_start_token:
		if (reg_cnt == 0)
			expected_reg = 8;
		else
			expected_immediate = 1;
		break;

	// Handle improper 16-bit register argument error
	case a85parse_sixteen_bit_reg_inst_token:
	case a85parse_stack_reg_inst_token:
		expected_reg = 16;
		break;

	// Handle improper 8-bit register argument error
	case a85parse_eight_bit_reg_inst_token:
		expected_reg = 8;
		break;

	case a85parse_lxi_inst_token:
	case a85parse_mvi_inst_token:
	case a85parse_immediate_operand_inst_token:
		expected_immediate = 1;
		break;

	case a85parse_rst_inst_token:
		if (eolMsg)
			string.Format("Error in line %d(%s), column %d:  Expected RST number",
				(PCB).line, gFilename, (PCB).column);
		else
			string.Format("Error in line %d(%s), column %d:  Invalid RST number",
				(PCB).line, gFilename, (PCB).column, (PCB).error_message);
		break;
		
	case a85parse_preproc_start_token:
		if (eolMsg)
			string.Format("Error in line %d(%s), column %d:  Expected preprocessor directive",
				(PCB).line, gFilename, (PCB).column);
		else
			string.Format("Error in line %d(%s), column %d:  Unknown preprocessor directive",
				(PCB).line, gFilename, (PCB).column, (PCB).error_message);
		break;
		
	case a85parse_preproc_inst_token:
		string.Format("Error in line %d(%s), column %d:  Invalid preprocessor directive syntax",
				(PCB).line, gFilename, (PCB).column, (PCB).error_message);
		break;
		
	default:
		if (eolMsg)
			string.Format("Error in line %d(%s), column %d:  Unexpected end of line",
				(PCB).line, gFilename, (PCB).column);
		else
			string.Format("Error in line %d(%s), column %d:  %s",
				(PCB).line, gFilename, (PCB).column, (PCB).error_message);
		break;
	}

	if (expected_reg == 16)
	{
		if (eolMsg)
			string.Format("Error in line %d(%s), column %d:  Expected 16-bit register",
				(PCB).line, gFilename, (PCB).column);
		else
			string.Format("Error in line %d(%s), column %d:  Invalid 16-bit register",
				(PCB).line, gFilename, (PCB).column);
	}
	else if (expected_reg == 8)
	{
		if (eolMsg)
			string.Format("Error in line %d(%s), column %d:  Expected 8-bit register",
				(PCB).line, gFilename, (PCB).column);
		else
			string.Format("Error in line %d(%s), column %d:  Invalid 8-bit register",
				(PCB).line, gFilename, (PCB).column);
	}
	
	if (expected_immediate)
	{
		if (eolMsg)
			string.Format("Error in line %d(%s), column %d:  Expected immediate operand",
				(PCB).line, gFilename, (PCB).column);
		else
			string.Format("Error in line %d(%s), column %d:  Invalid immediate operand",
				(PCB).line, gFilename, (PCB).column);
	}

	if (!gAbort) 
		gAsm->m_Errors.Add(string); 

	// Clear the string stack
	ss_idx = 0; 
	ss_len = 0;
}

// Define macros to handle input and errors

#define TAB_SPACING gTabSize

#define GET_INPUT {(PCB).input_code = (gAsm->m_fd != 0 && !gAbort) ? \
	fgetc(gAsm->m_fd) : 0; if ((PCB).input_code == 13) (PCB).input_code = fgetc(gAsm->m_fd);\
	}

#define SYNTAX_ERROR { syntax_error(TOKEN_NAMES[(PCB).error_frame_token]); }

/*
#define SYNTAX_ERROR {MString string;  if ((PCB).error_frame_token != 0) \
    string.Format("Error in line %d(%s), column%d:  Malformed %s - %s", \
    (PCB).line, gFilename, (PCB).column, \
    TOKEN_NAMES[(PCB).error_frame_token], (PCB).error_message); \
  else if (strcmp("Unexpected '\\n'", (PCB).error_message) == 0) \
    string.Format("Error in line %d(%s), column %d:  Unexpected end of line", \
    (PCB).line, gFilename, (PCB).column); \
  else \
    string.Format("Error in line %d(%s), column %d:  %s", (PCB).line, gFilenaeme, \
    (PCB).column, (PCB).error_message); \
  if (!gAbort) gAsm->m_Errors.Add(string); ss_idx = 0; ss_len = 0; }
*/

#define PARSER_STACK_OVERFLOW {MString string;  string.Format(\
   "\nParser stack overflow, line %d, column %d",\
   (PCB).line, (PCB).column); if (!gAbort) gAsm->m_Errors.Add(string);}

#define REDUCTION_TOKEN_ERROR {MString string;  string.Format(\
    "\nReduction token error, line %d, column %d", \
    (PCB).line, (PCB).column); if (!gAbort) gAsm->m_Errors.Add(string);}

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

int chtoh(char ch)
{
	if (ch < 'A')
		return ch - '0';
	if (ch < 'a')
		return ch = 'A' + 10;
	return ch - 'a' + 10;
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

#define ag_rp_1() (gAsm->equate((const char *) -1))

#define ag_rp_2() (gAsm->directive_set((const char *) -1))

#define ag_rp_3() (gAsm->include(ss[ss_idx--]))

#define ag_rp_4() (gAsm->include(ss[ss_idx--]))

#define ag_rp_5() (gAsm->include(ss[ss_idx--]))

#define ag_rp_6() (gAsm->directive_cdseg(seg, page))

#define ag_rp_7() (gAsm->directive_cdseg(seg, page))

#define ag_rp_8() (handle_error())

#define ag_rp_9() (gAsm->include(ss[ss_idx--]))

#define ag_rp_10() (gAsm->include(ss[ss_idx--]))

#define ag_rp_11() (gAsm->pragma_list())

#define ag_rp_12() (gAsm->pragma_hex())

#define ag_rp_13() (gAsm->pragma_entry(ss[ss_idx--]))

#define ag_rp_14() (gAsm->pragma_verilog())

#define ag_rp_15() (gAsm->pragma_extended())

#define ag_rp_16() (gAsm->preproc_ifdef(ss[ss_idx--]))

#define ag_rp_17() (gAsm->preproc_if())

#define ag_rp_18() (gAsm->preproc_elif())

#define ag_rp_19() (gAsm->preproc_ifndef(ss[ss_idx--]))

#define ag_rp_20() (gAsm->preproc_else())

#define ag_rp_21() (gAsm->preproc_endif())

static void ag_rp_22(void) {
/* Line 125, C:/Projects/VirtualT/src/a85parse.syn */
if (gAsm->preproc_error(ss[ss_idx--])) gAbort = TRUE;
}

#define ag_rp_23() (gAsm->preproc_undef(ss[ss_idx--]))

static void ag_rp_24(void) {
/* Line 128, C:/Projects/VirtualT/src/a85parse.syn */
 if (gMacroStack[ms_idx] != 0) { delete gMacroStack[ms_idx]; \
															gMacroStack[ms_idx] = 0; } \
														gMacro = gMacroStack[--ms_idx]; \
														gMacro->m_DefString = ss[ss_idx--]; \
														gAsm->preproc_define(); \
														gMacroStack[ms_idx] = 0; gDefine = 0; 
}

static void ag_rp_25(void) {
/* Line 136, C:/Projects/VirtualT/src/a85parse.syn */
 if (gAsm->preproc_macro()) \
															PCB.reduction_token = a85parse_WS_token; 
}

static void ag_rp_26(void) {
/* Line 138, C:/Projects/VirtualT/src/a85parse.syn */
 if (gAsm->preproc_macro()) \
															PCB.reduction_token = a85parse_WS_token; 
}

static void ag_rp_27(void) {
/* Line 142, C:/Projects/VirtualT/src/a85parse.syn */
 gMacro->m_ParamList = gExpList; \
															gMacro->m_Name = ss[ss_idx--]; \
															gExpList = new VTObArray; \
															gMacroStack[ms_idx++] = gMacro; \
															if (gAsm->preproc_macro()) \
																PCB.reduction_token = a85parse_WS_token; \
															gMacro = new CMacro; 
}

static void ag_rp_28(void) {
/* Line 149, C:/Projects/VirtualT/src/a85parse.syn */
 gMacro->m_Name = ss[ss_idx--]; gMacroStack[ms_idx++] = gMacro; \
															if (gAsm->preproc_macro()) \
																PCB.reduction_token = a85parse_WS_token; \
															gMacro = new CMacro; 
}

static void ag_rp_29(int c) {
/* Line 155, C:/Projects/VirtualT/src/a85parse.syn */
 ss[++ss_idx][0] = c; ss[ss_idx][1] = 0; ss_len = 1; 
}

static void ag_rp_30(int c) {
/* Line 156, C:/Projects/VirtualT/src/a85parse.syn */
 ss[ss_idx][ss_len++] = c; ss[ss_idx][ss_len] = 0; 
}

static void ag_rp_31(void) {
/* Line 157, C:/Projects/VirtualT/src/a85parse.syn */
 ss[ss_idx][ss_len++] = '\n'; ss[ss_idx][ss_len] = 0; 
}

static void ag_rp_32(void) {
/* Line 160, C:/Projects/VirtualT/src/a85parse.syn */
 page = 0; seg = CSEG; 
}

static void ag_rp_33(void) {
/* Line 161, C:/Projects/VirtualT/src/a85parse.syn */
 page = 0; seg = DSEG; 
}

#define ag_rp_34(p) (page = p)

#define ag_rp_35() (gAsm->pragma_list())

#define ag_rp_36() (gAsm->pragma_hex())

#define ag_rp_37() (gAsm->pragma_verilog())

#define ag_rp_38() (gAsm->pragma_entry(ss[ss_idx--]))

#define ag_rp_39() (gAsm->pragma_extended())

#define ag_rp_40() (gAsm->directive_org())

#define ag_rp_41() (gAsm->directive_aseg())

#define ag_rp_42() (gAsm->directive_ds())

#define ag_rp_43() (gAsm->directive_db())

#define ag_rp_44() (gAsm->directive_dw())

#define ag_rp_45() (gAsm->directive_public())

#define ag_rp_46() (gAsm->directive_extern())

#define ag_rp_47() (gAsm->directive_extern())

#define ag_rp_48() (gAsm->directive_module(ss[ss_idx--]))

#define ag_rp_49() (gAsm->directive_name(ss[ss_idx--]))

#define ag_rp_50() (gAsm->directive_stkln())

#define ag_rp_51() (gAsm->directive_echo())

#define ag_rp_52() (gAsm->directive_echo(ss[ss_idx--]))

#define ag_rp_53() (gAsm->directive_fill())

#define ag_rp_54() (gAsm->directive_printf(ss[ss_idx--]))

#define ag_rp_55() (gAsm->directive_end(""))

#define ag_rp_56() (gAsm->directive_end(ss[ss_idx--]))

#define ag_rp_57() (gAsm->directive_if())

#define ag_rp_58() (gAsm->directive_else())

#define ag_rp_59() (gAsm->directive_endif())

#define ag_rp_60() (gAsm->directive_endian(1))

#define ag_rp_61() (gAsm->directive_endian(0))

#define ag_rp_62() (gAsm->directive_title(ss[ss_idx--]))

#define ag_rp_63() (gAsm->directive_title(ss[ss_idx--]))

#define ag_rp_64() (gAsm->directive_page(-1))

#define ag_rp_65() (gAsm->directive_sym())

#define ag_rp_66() (gAsm->directive_link(ss[ss_idx--]))

#define ag_rp_67() (gAsm->directive_maclib(ss[ss_idx--]))

#define ag_rp_68() (gAsm->directive_page(page))

#define ag_rp_69() (page = 60)

#define ag_rp_70(n) (page = n)

#define ag_rp_71() (expression_list_literal())

#define ag_rp_72() (expression_list_literal())

#define ag_rp_73() (expression_list_equation())

#define ag_rp_74() (expression_list_equation())

#define ag_rp_75() (expression_list_literal())

#define ag_rp_76() (expression_list_literal())

#define ag_rp_77() (gNameList->Add(ss[ss_idx--]))

#define ag_rp_78() (gNameList->Add(ss[ss_idx--]))

static void ag_rp_79(void) {
/* Line 242, C:/Projects/VirtualT/src/a85parse.syn */
 strcpy(ss[++ss_idx], "$"); ss_len = 1; 
}

static void ag_rp_80(void) {
/* Line 243, C:/Projects/VirtualT/src/a85parse.syn */
 strcpy(ss[++ss_idx], "&"); ss_len = 1; 
}

static void ag_rp_81(int c) {
/* Line 246, C:/Projects/VirtualT/src/a85parse.syn */
 ss[++ss_idx][0] = c; ss[ss_idx][1] = 0; ss_len = 1; \
														if (PCB.column == 2) ss_addr = gAsm->m_ActiveSeg->m_Address; 
}

static void ag_rp_82(int c) {
/* Line 248, C:/Projects/VirtualT/src/a85parse.syn */
 ss[ss_idx][ss_len++] = c; ss[ss_idx][ss_len] = 0; 
}

static void ag_rp_83(int c) {
/* Line 249, C:/Projects/VirtualT/src/a85parse.syn */
 ss[ss_idx][ss_len++] = c; ss[ss_idx][ss_len] = 0; 
}

static void ag_rp_84(int c) {
/* Line 252, C:/Projects/VirtualT/src/a85parse.syn */
 ss[++ss_idx][0] = c; ss[ss_idx][1] = 0; ss_len = 1; 
}

static void ag_rp_85(int c) {
/* Line 253, C:/Projects/VirtualT/src/a85parse.syn */
 ss[ss_idx][ss_len++] = c; ss[ss_idx][ss_len] = 0; 
}

static void ag_rp_86(int ch1, int ch2) {
/* Line 259, C:/Projects/VirtualT/src/a85parse.syn */
 ss_idx++; ss_len = 2; sprintf(ss[ss_idx], "%c%c", ch1, ch2); 
}

static void ag_rp_87(int c) {
/* Line 260, C:/Projects/VirtualT/src/a85parse.syn */
 ss[ss_idx][ss_len++] = c; ss[ss_idx][ss_len] = 0; 
}

static void ag_rp_88(void) {
/* Line 267, C:/Projects/VirtualT/src/a85parse.syn */
 ss_idx++; ss_len = 0; 
}

static void ag_rp_89(int c) {
/* Line 268, C:/Projects/VirtualT/src/a85parse.syn */
 ss[ss_idx][ss_len++] = c; ss[ss_idx][ss_len] = 0; 
}

#define ag_rp_90(n) (n)

#define ag_rp_91() ('\\')

#define ag_rp_92() ('\n')

#define ag_rp_93() ('\t')

#define ag_rp_94() ('\r')

#define ag_rp_95() ('\0')

#define ag_rp_96() ('"')

#define ag_rp_97() (0x08)

#define ag_rp_98() (0x0C)

#define ag_rp_99(n1, n2, n3) ((n1-'0') * 64 + (n2-'0') * 8 + n3-'0')

#define ag_rp_100(n1, n2) (chtoh(n1) * 16 + chtoh(n2))

#define ag_rp_101(n1) (chtoh(n1))

static void ag_rp_102(void) {
/* Line 288, C:/Projects/VirtualT/src/a85parse.syn */
 ss[ss_idx][ss_len++] = '>'; ss[ss_idx][ss_len] = 0; 
}

static void ag_rp_103(void) {
/* Line 291, C:/Projects/VirtualT/src/a85parse.syn */
 ss[++ss_idx][0] = '<'; ss[ss_idx][1] = 0; ss_len = 1; 
}

static void ag_rp_104(int c) {
/* Line 292, C:/Projects/VirtualT/src/a85parse.syn */
 ss[ss_idx][ss_len++] = c; ss[ss_idx][ss_len] = 0; 
}

static void ag_rp_105(int c) {
/* Line 293, C:/Projects/VirtualT/src/a85parse.syn */
 ss[ss_idx][ss_len++] = '\\'; ss[ss_idx][ss_len++] = c; ss[ss_idx][ss_len] = 0; 
}

#define ag_rp_106() (gAsm->label(ss[ss_idx--]))

#define ag_rp_107() (gAsm->label(ss[ss_idx--]))

#define ag_rp_108() (gAsm->label(".bss"))

#define ag_rp_109() (gAsm->label(".text"))

#define ag_rp_110() (gAsm->label(".data"))

#define ag_rp_111() (PAGE)

#define ag_rp_112() (INPAGE)

#define ag_rp_113() (condition(-1))

#define ag_rp_114() (condition(COND_NOCMP))

#define ag_rp_115() (condition(COND_EQ))

#define ag_rp_116() (condition(COND_NE))

#define ag_rp_117() (condition(COND_GE))

#define ag_rp_118() (condition(COND_LE))

#define ag_rp_119() (condition(COND_GT))

#define ag_rp_120() (condition(COND_LT))

#define ag_rp_121() (gEq->Add(RPN_BITOR))

#define ag_rp_122() (gEq->Add(RPN_BITOR))

#define ag_rp_123() (gEq->Add(RPN_BITXOR))

#define ag_rp_124() (gEq->Add(RPN_BITXOR))

#define ag_rp_125() (gEq->Add(RPN_BITAND))

#define ag_rp_126() (gEq->Add(RPN_BITAND))

#define ag_rp_127() (gEq->Add(RPN_LEFTSHIFT))

#define ag_rp_128() (gEq->Add(RPN_LEFTSHIFT))

#define ag_rp_129() (gEq->Add(RPN_RIGHTSHIFT))

#define ag_rp_130() (gEq->Add(RPN_RIGHTSHIFT))

#define ag_rp_131() (gEq->Add(RPN_ADD))

#define ag_rp_132() (gEq->Add(RPN_SUBTRACT))

#define ag_rp_133() (gEq->Add(RPN_MULTIPLY))

#define ag_rp_134() (gEq->Add(RPN_DIVIDE))

#define ag_rp_135() (gEq->Add(RPN_MODULUS))

#define ag_rp_136() (gEq->Add(RPN_MODULUS))

#define ag_rp_137() (gEq->Add(RPN_EXPONENT))

#define ag_rp_138() (gEq->Add(RPN_NOT))

#define ag_rp_139() (gEq->Add(RPN_NOT))

#define ag_rp_140() (gEq->Add(RPN_BITNOT))

#define ag_rp_141() (gEq->Add(RPN_NEGATE))

#define ag_rp_142(n) (gEq->Add((double) n))

static void ag_rp_143(void) {
/* Line 384, C:/Projects/VirtualT/src/a85parse.syn */
 delete gMacro; gMacro = gMacroStack[ms_idx-1]; \
														gMacroStack[ms_idx--] = 0; if (gMacro->m_ParamList == 0) \
														{\
															gEq->Add(gMacro->m_Name); gMacro->m_Name = ""; }\
														else { \
															gEq->Add((VTObject *) gMacro); gMacro = new CMacro; \
														} 
}

#define ag_rp_144() (gEq->Add(RPN_FLOOR))

#define ag_rp_145() (gEq->Add(RPN_CEIL))

#define ag_rp_146() (gEq->Add(RPN_LN))

#define ag_rp_147() (gEq->Add(RPN_LOG))

#define ag_rp_148() (gEq->Add(RPN_SQRT))

#define ag_rp_149() (gEq->Add(RPN_IP))

#define ag_rp_150() (gEq->Add(RPN_FP))

#define ag_rp_151() (gEq->Add(RPN_HIGH))

#define ag_rp_152() (gEq->Add(RPN_LOW))

#define ag_rp_153() (gEq->Add(RPN_PAGE))

#define ag_rp_154() (gEq->Add(RPN_DEFINED, ss[ss_idx--]))

#define ag_rp_155(n) (n)

#define ag_rp_156(r) (r)

#define ag_rp_157(n) (n)

#define ag_rp_158() (conv_to_dec())

#define ag_rp_159() (conv_to_hex())

#define ag_rp_160() (conv_to_bin())

#define ag_rp_161() (conv_to_oct())

#define ag_rp_162() (conv_to_hex())

#define ag_rp_163() (conv_to_hex())

#define ag_rp_164() (conv_to_bin())

#define ag_rp_165() (conv_to_oct())

#define ag_rp_166() (conv_to_dec())

static void ag_rp_167(int n) {
/* Line 431, C:/Projects/VirtualT/src/a85parse.syn */
 int_len = 1; integer[0] = n; integer[1] = 0; 
}

static void ag_rp_168(int n) {
/* Line 432, C:/Projects/VirtualT/src/a85parse.syn */
 int_len = 2; integer[0] = '-', integer[1] = n; integer[2] = 0; 
}

static void ag_rp_169(int n) {
/* Line 433, C:/Projects/VirtualT/src/a85parse.syn */
 int_len = 1; integer[0] = n; integer[1] = 0; 
}

static void ag_rp_170(int n) {
/* Line 434, C:/Projects/VirtualT/src/a85parse.syn */
 integer[int_len++] = n; integer[int_len] = 0; 
}

static void ag_rp_171(int n) {
/* Line 439, C:/Projects/VirtualT/src/a85parse.syn */
 int_len = 1; integer[0] = n; integer[1] = 0; 
}

static void ag_rp_172(int n) {
/* Line 440, C:/Projects/VirtualT/src/a85parse.syn */
 int_len = 1; integer[0] = n; integer[1] = 0; 
}

static void ag_rp_173(int n) {
/* Line 441, C:/Projects/VirtualT/src/a85parse.syn */
 integer[int_len++] = n; integer[int_len] = 0; 
}

static void ag_rp_174(int n) {
/* Line 444, C:/Projects/VirtualT/src/a85parse.syn */
 integer[int_len++] = n; integer[int_len] = 0; 
}

static void ag_rp_175(int n) {
/* Line 445, C:/Projects/VirtualT/src/a85parse.syn */
 integer[int_len++] = n; integer[int_len] = 0; 
}

static void ag_rp_176(int n) {
/* Line 448, C:/Projects/VirtualT/src/a85parse.syn */
 integer[int_len++] = n; integer[int_len] = 0; 
}

static void ag_rp_177(int n) {
/* Line 449, C:/Projects/VirtualT/src/a85parse.syn */
 integer[int_len++] = n; integer[int_len] = 0; 
}

static void ag_rp_178(int n) {
/* Line 452, C:/Projects/VirtualT/src/a85parse.syn */
 integer[int_len++] = n; integer[int_len] = 0; 
}

static void ag_rp_179(int n) {
/* Line 453, C:/Projects/VirtualT/src/a85parse.syn */
 integer[int_len++] = n; integer[int_len] = 0; 
}

#define ag_rp_180(n) (n)

#define ag_rp_181(n1, n2) ((n1 << 8) | n2)

#define ag_rp_182() ('\\')

#define ag_rp_183(n) (n)

#define ag_rp_184() ('\\')

#define ag_rp_185() ('\n')

#define ag_rp_186() ('\t')

#define ag_rp_187() ('\r')

#define ag_rp_188() ('\0')

#define ag_rp_189() ('\'')

#define ag_rp_190() ('\'')

static double ag_rp_191(void) {
/* Line 474, C:/Projects/VirtualT/src/a85parse.syn */
 gDivisor = 1.0; return (double) conv_to_dec(); 
}

static double ag_rp_192(int d) {
/* Line 475, C:/Projects/VirtualT/src/a85parse.syn */
 gDivisor = 10.0; return ((double) (d - '0') / gDivisor); 
}

static double ag_rp_193(double r, int d) {
/* Line 476, C:/Projects/VirtualT/src/a85parse.syn */
 gDivisor *= 10.0; return (r + (double) (d - '0') / gDivisor); 
}

#define ag_rp_194() (reg[reg_cnt++] = '0')

#define ag_rp_195() (reg[reg_cnt++] = '1')

#define ag_rp_196() (reg[reg_cnt++] = '2')

#define ag_rp_197() (reg[reg_cnt++] = '3')

#define ag_rp_198() (reg[reg_cnt++] = '4')

#define ag_rp_199() (reg[reg_cnt++] = '5')

#define ag_rp_200() (reg[reg_cnt++] = '6')

#define ag_rp_201() (reg[reg_cnt++] = '7')

#define ag_rp_202() (reg[reg_cnt++] = '0')

#define ag_rp_203() (reg[reg_cnt++] = '1')

#define ag_rp_204() (reg[reg_cnt++] = '2')

#define ag_rp_205() (reg[reg_cnt++] = '3')

#define ag_rp_206() (reg[reg_cnt++] = '0')

#define ag_rp_207() (reg[reg_cnt++] = '1')

#define ag_rp_208() (reg[reg_cnt++] = '2')

#define ag_rp_209() (reg[reg_cnt++] = '3')

#define ag_rp_210() (reg[reg_cnt++] = '3')

#define ag_rp_211() (reg[reg_cnt++] = '0')

#define ag_rp_212() (reg[reg_cnt++] = '1')

#define ag_rp_213() (reg[reg_cnt++] = '0')

#define ag_rp_214() (reg[reg_cnt++] = '1')

#define ag_rp_215() (reg[reg_cnt++] = '2')

#define ag_rp_216() (reg[reg_cnt++] = '3')

#define ag_rp_217() (reg[reg_cnt++] = '4')

#define ag_rp_218() (reg[reg_cnt++] = '5')

#define ag_rp_219() (gAsm->opcode_arg_0		(OPCODE_ASHR))

#define ag_rp_220() (gAsm->opcode_arg_0		(OPCODE_CMA))

#define ag_rp_221() (gAsm->opcode_arg_0		(OPCODE_CMC))

#define ag_rp_222() (gAsm->opcode_arg_0		(OPCODE_DAA))

#define ag_rp_223() (gAsm->opcode_arg_0		(OPCODE_DI))

#define ag_rp_224() (gAsm->opcode_arg_0		(OPCODE_DSUB))

#define ag_rp_225() (gAsm->opcode_arg_0		(OPCODE_EI))

#define ag_rp_226() (gAsm->opcode_arg_0		(OPCODE_HLT))

#define ag_rp_227() (gAsm->opcode_arg_0		(OPCODE_LHLX))

#define ag_rp_228() (gAsm->opcode_arg_0		(OPCODE_NOP))

#define ag_rp_229() (gAsm->opcode_arg_0		(OPCODE_PCHL))

#define ag_rp_230() (gAsm->opcode_arg_0		(OPCODE_RAL))

#define ag_rp_231() (gAsm->opcode_arg_0		(OPCODE_RAR))

#define ag_rp_232() (gAsm->opcode_arg_0		(OPCODE_RC))

#define ag_rp_233() (gAsm->opcode_arg_0		(OPCODE_RET))

#define ag_rp_234() (gAsm->opcode_arg_0		(OPCODE_RIM))

#define ag_rp_235() (gAsm->opcode_arg_0		(OPCODE_RLC))

#define ag_rp_236() (gAsm->opcode_arg_0		(OPCODE_RLDE))

#define ag_rp_237() (gAsm->opcode_arg_0		(OPCODE_RM))

#define ag_rp_238() (gAsm->opcode_arg_0		(OPCODE_RNC))

#define ag_rp_239() (gAsm->opcode_arg_0		(OPCODE_RNZ))

#define ag_rp_240() (gAsm->opcode_arg_0		(OPCODE_RP))

#define ag_rp_241() (gAsm->opcode_arg_0		(OPCODE_RPE))

#define ag_rp_242() (gAsm->opcode_arg_0		(OPCODE_RPO))

#define ag_rp_243() (gAsm->opcode_arg_0		(OPCODE_RRC))

#define ag_rp_244() (gAsm->opcode_arg_0		(OPCODE_RSTV))

#define ag_rp_245() (gAsm->opcode_arg_0		(OPCODE_RZ))

#define ag_rp_246() (gAsm->opcode_arg_0		(OPCODE_SHLX))

#define ag_rp_247() (gAsm->opcode_arg_0		(OPCODE_SIM))

#define ag_rp_248() (gAsm->opcode_arg_0		(OPCODE_SPHL))

#define ag_rp_249() (gAsm->opcode_arg_0		(OPCODE_STC))

#define ag_rp_250() (gAsm->opcode_arg_0		(OPCODE_XCHG))

#define ag_rp_251() (gAsm->opcode_arg_0		(OPCODE_XTHL))

#define ag_rp_252() (gAsm->opcode_arg_0		(OPCODE_LRET))

#define ag_rp_253() (gAsm->opcode_arg_1reg		(OPCODE_STAX))

#define ag_rp_254() (gAsm->opcode_arg_1reg		(OPCODE_LDAX))

#define ag_rp_255() (gAsm->opcode_arg_1reg		(OPCODE_POP))

#define ag_rp_256() (gAsm->opcode_arg_1reg		(OPCODE_PUSH))

#define ag_rp_257() (gAsm->opcode_arg_1reg_2byte	(OPCODE_LPOP))

#define ag_rp_258() (gAsm->opcode_arg_1reg_2byte	(OPCODE_LPUSH))

#define ag_rp_259() (gAsm->opcode_arg_1reg		(OPCODE_ADC))

#define ag_rp_260() (gAsm->opcode_arg_1reg		(OPCODE_ADD))

#define ag_rp_261() (gAsm->opcode_arg_1reg		(OPCODE_ANA))

#define ag_rp_262() (gAsm->opcode_arg_1reg		(OPCODE_CMP))

#define ag_rp_263() (gAsm->opcode_arg_1reg		(OPCODE_DCR))

#define ag_rp_264() (gAsm->opcode_arg_1reg		(OPCODE_INR))

#define ag_rp_265() (gAsm->opcode_arg_2reg		(OPCODE_MOV))

#define ag_rp_266() (gAsm->opcode_arg_1reg		(OPCODE_ORA))

#define ag_rp_267() (gAsm->opcode_arg_1reg		(OPCODE_SBB))

#define ag_rp_268() (gAsm->opcode_arg_1reg		(OPCODE_SUB))

#define ag_rp_269() (gAsm->opcode_arg_1reg		(OPCODE_XRA))

#define ag_rp_270() (gAsm->opcode_arg_1reg_2byte	(OPCODE_SPG))

#define ag_rp_271() (gAsm->opcode_arg_1reg_2byte	(OPCODE_RPG))

#define ag_rp_272() (gAsm->opcode_arg_1reg		(OPCODE_DAD))

#define ag_rp_273() (gAsm->opcode_arg_1reg		(OPCODE_DCX))

#define ag_rp_274() (gAsm->opcode_arg_1reg		(OPCODE_INX))

#define ag_rp_275() (gAsm->opcode_arg_1reg_equ16(OPCODE_LXI))

#define ag_rp_276() (gAsm->opcode_arg_1reg_equ8(OPCODE_MVI))

#define ag_rp_277() (gAsm->opcode_arg_1reg_equ16(OPCODE_SPI))

#define ag_rp_278(c) (gAsm->opcode_arg_imm		(OPCODE_RST, c))

#define ag_rp_279() (gAsm->opcode_arg_equ8		(OPCODE_ACI))

#define ag_rp_280() (gAsm->opcode_arg_equ8		(OPCODE_ADI))

#define ag_rp_281() (gAsm->opcode_arg_equ8		(OPCODE_ANI))

#define ag_rp_282() (gAsm->opcode_arg_equ16	(OPCODE_CALL))

#define ag_rp_283() (gAsm->opcode_arg_equ16	(OPCODE_CC))

#define ag_rp_284() (gAsm->opcode_arg_equ16	(OPCODE_CM))

#define ag_rp_285() (gAsm->opcode_arg_equ16	(OPCODE_CNC))

#define ag_rp_286() (gAsm->opcode_arg_equ16	(OPCODE_CNZ))

#define ag_rp_287() (gAsm->opcode_arg_equ16	(OPCODE_CP))

#define ag_rp_288() (gAsm->opcode_arg_equ16	(OPCODE_CPE))

#define ag_rp_289() (gAsm->opcode_arg_equ8		(OPCODE_CPI))

#define ag_rp_290() (gAsm->opcode_arg_equ16	(OPCODE_CPO))

#define ag_rp_291() (gAsm->opcode_arg_equ16	(OPCODE_CZ))

#define ag_rp_292() (gAsm->opcode_arg_equ8		(OPCODE_IN))

#define ag_rp_293() (gAsm->opcode_arg_equ16	(OPCODE_JC))

#define ag_rp_294() (gAsm->opcode_arg_equ16	(OPCODE_JD))

#define ag_rp_295() (gAsm->opcode_arg_equ16	(OPCODE_JM))

#define ag_rp_296() (gAsm->opcode_arg_equ16	(OPCODE_JMP))

#define ag_rp_297() (gAsm->opcode_arg_equ16	(OPCODE_JNC))

#define ag_rp_298() (gAsm->opcode_arg_equ16	(OPCODE_JND))

#define ag_rp_299() (gAsm->opcode_arg_equ16	(OPCODE_JNZ))

#define ag_rp_300() (gAsm->opcode_arg_equ16	(OPCODE_JP))

#define ag_rp_301() (gAsm->opcode_arg_equ16	(OPCODE_JPE))

#define ag_rp_302() (gAsm->opcode_arg_equ16	(OPCODE_JPO))

#define ag_rp_303() (gAsm->opcode_arg_equ16	(OPCODE_JZ))

#define ag_rp_304() (gAsm->opcode_arg_equ16	(OPCODE_LDA))

#define ag_rp_305() (gAsm->opcode_arg_equ8		(OPCODE_LDEH))

#define ag_rp_306() (gAsm->opcode_arg_equ8		(OPCODE_LDES))

#define ag_rp_307() (gAsm->opcode_arg_equ16	(OPCODE_LHLD))

#define ag_rp_308() (gAsm->opcode_arg_equ8		(OPCODE_ORI))

#define ag_rp_309() (gAsm->opcode_arg_equ8		(OPCODE_OUT))

#define ag_rp_310() (gAsm->opcode_arg_equ8		(OPCODE_SBI))

#define ag_rp_311() (gAsm->opcode_arg_equ16	(OPCODE_SHLD))

#define ag_rp_312() (gAsm->opcode_arg_equ16	(OPCODE_STA))

#define ag_rp_313() (gAsm->opcode_arg_equ8		(OPCODE_SUI))

#define ag_rp_314() (gAsm->opcode_arg_equ8		(OPCODE_XRI))

#define ag_rp_315() (gAsm->opcode_arg_equ8		(OPCODE_BR))

#define ag_rp_316() (gAsm->opcode_arg_equ16	(OPCODE_BRA))

#define ag_rp_317() (gAsm->opcode_arg_equ16	(OPCODE_BZ))

#define ag_rp_318() (gAsm->opcode_arg_equ16	(OPCODE_BNZ))

#define ag_rp_319() (gAsm->opcode_arg_equ16	(OPCODE_BC))

#define ag_rp_320() (gAsm->opcode_arg_equ16	(OPCODE_BNC))

#define ag_rp_321() (gAsm->opcode_arg_equ16	(OPCODE_BM))

#define ag_rp_322() (gAsm->opcode_arg_equ16	(OPCODE_BP))

#define ag_rp_323() (gAsm->opcode_arg_equ16	(OPCODE_BPE))

#define ag_rp_324() (gAsm->opcode_arg_equ16	(OPCODE_RCALL))

#define ag_rp_325() (gAsm->opcode_arg_equ8		(OPCODE_SBZ))

#define ag_rp_326() (gAsm->opcode_arg_equ8		(OPCODE_SBNZ))

#define ag_rp_327() (gAsm->opcode_arg_equ8		(OPCODE_SBC))

#define ag_rp_328() (gAsm->opcode_arg_equ8		(OPCODE_SBNC))

#define ag_rp_329() (gAsm->opcode_arg_equ24	(OPCODE_LCALL))

#define ag_rp_330() (gAsm->opcode_arg_equ24	(OPCODE_LJMP))


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

static const unsigned short ag_rpx[] = {
    0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,
    0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  1,  2,  0,  0,  3,  4,  5,  6,
    7,  8,  0,  0,  9, 10, 11, 12, 13, 14, 15,  0,  0, 16, 17, 18, 19, 20,
   21, 22, 23, 24, 25, 26, 27, 28, 29, 30, 31, 32, 33,  0, 34,  0, 35, 36,
   37, 38, 39,  0,  0,  0, 40, 41,  0,  0, 42,  0,  0,  0, 43,  0,  0, 44,
   45, 46, 47, 48, 49, 50, 51, 52, 53, 54, 55, 56, 57, 58, 59, 60, 61, 62,
   63, 64, 65, 66, 67,  0, 68, 69, 70, 71, 72, 73, 74, 75, 76, 77, 78,  0,
    0, 79, 80, 81, 82, 83, 84, 85,  0, 86, 87,  0, 88, 89,  0, 90, 91, 92,
   93, 94, 95, 96, 97, 98, 99,  0,  0,100,101,102,103,104,105,  0,106,107,
  108,109,110,111,112,113,114,  0,  0,  0,115,  0,  0,116,  0,  0,117,  0,
    0,118,  0,  0,119,  0,  0,120,  0,  0,121,122,  0,123,124,  0,125,126,
    0,127,128,129,130,  0,131,132,  0,133,134,135,136,137,  0,138,139,140,
  141,142,143,  0,  0,144,145,146,147,148,149,150,151,152,153,154,155,156,
  157,158,159,160,161,162,163,164,  0,  0,165,166,167,168,169,170,  0,  0,
  171,172,173,174,175,176,177,178,179,180,181,182,183,184,185,186,187,188,
  189,190,  0,191,192,193,194,195,196,197,198,199,  0,  0,200,201,  0,  0,
  202,  0,  0,203,  0,  0,204,205,206,207,208,209,210,211,212,213,214,215,
  216,217,218,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,
    0,219,220,221,222,223,  0,  0,224,225,226,  0,  0,  0,227,228,229,230,
  231,232,233,234,235,  0,  0,236,237,238,239,240,241,242,243,244,245,  0,
    0,  0,246,247,248,249,250,251,252,253,254,255,256,257,258,259,260,261,
  262,263,264,265,266,267,268,269,270,271,272,273,274,  0,275,  0,276,  0,
  277,278,279,280,281,282,283,284,285,286,287,288,289,290,291,292,293,  0,
    0,  0,  0,  0,294,295,296,297,  0,  0,  0,  0,  0,298,299,300,301,302,
  303,304,  0,  0,  0,305,  0,  0,  0,306,307,308,309,310,311,312,313,314,
  315,316,317,318,319,320,321,322,323,324,325,326,327,328,329,330
};

static const unsigned char ag_key_itt[] = {
 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 1, 0, 0, 1, 0, 0, 0, 0, 0, 0, 1, 0, 1, 0,
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
  1,432,  1,433,  1,434,  1,435,  1,441,  1,440,  1,442,  1,444,
  1,445,  1,446,  1,447,  1,448,  1,449,  1,450,  1,451,  1,452,
  1,453,  1,454,  1,455,  1,460,  1,461,  1,462,  1,465,  1,466,
  1,467,  1,468,  1,469,  1,470,  1,471,  1,472,  1,473,  1,474,
  1,475,  1,476,  1,477,  1,478,  1,479,  1,480,  1,482,  1,483,
  1,484,  1,485,  1,487,  1,488,  1,489,  1,490,  1,491,  1,492,
  1,134,  1,495,  1,496,  1,499,  1,501,  1,503,  1,505,  1,507,
  1,510,  1,512,  1,514,  1,516,  1,518,  1,524,  1,527,  1,529,
  1,530,  1,531,  1,532,  1,533,  1,534,  1,535,  1,536,  1,537,
  1,539,  1,222,  1,223,  1,235,  1,236,  1,237,  1,238,  1,239,
  1,240,  1,241,  1,542,  1,670,  1,247,  1,249,  1,545,  1,541,
  1,543,  1,544,  1,546,  1,547,  1,548,  1,549,  1,550,  1,551,
  1,552,  1,553,  1,554,  1,555,  1,556,  1,557,  1,558,  1,559,
  1,560,  1,561,  1,562,  1,563,  1,564,  1,565,  1,566,  1,567,
  1,568,  1,569,  1,570,  1,571,  1,572,  1,573,  1,574,  1,575,
  1,576,  1,577,  1,578,  1,579,  1,580,  1,581,  1,582,  1,583,
  1,584,  1,585,  1,586,  1,587,  1,588,  1,589,  1,590,  1,591,
  1,592,  1,593,  1,594,  1,595,  1,597,  1,598,  1,599,  1,600,
  1,601,  1,602,  1,603,  1,604,  1,605,  1,606,  1,607,  1,608,
  1,609,  1,611,  1,612,  1,613,  1,614,  1,615,  1,616,  1,617,
  1,618,  1,619,  1,620,  1,621,  1,622,  1,623,  1,624,  1,625,
  1,626,  1,627,  1,628,  1,629,  1,630,  1,631,  1,632,  1,633,
  1,634,  1,635,  1,636,  1,637,  1,638,  1,639,  1,640,  1,641,
  1,642,  1,643,  1,644,  1,645,  1,646,  1,647,  1,648,  1,649,
  1,650,  1,651,  1,652,  1,653,  1,654,  1,655,  1,656,  1,657,
  1,658,  1,659,  1,660,  1,661,  1,662,  1,663,  1,664,  1,665,
  1,666,  1,667,  1,668,  1,669,  1,671,  1,672,  1,673,  1,674,
  1,675,  1,676,  1,677,  1,678,  1,679,  1,680,  1,681,0
};

static const unsigned char ag_key_ch[] = {
    0, 66, 68, 84,255, 42, 47,255, 85,255, 76,255, 67,255, 78,255, 35, 36,
   38, 46, 47, 73,255, 61,255, 61,255, 42, 47, 61,255, 66, 68, 84,255, 42,
   47,255, 60, 61,255, 61,255, 61, 62,255, 67, 68, 73,255, 65, 68, 73,255,
   69, 72,255, 67, 68, 78, 82, 83,255, 67, 90,255, 69,255, 65,255, 67, 76,
   77, 78, 80, 82, 89, 90,255, 65, 67, 80,255, 67, 90,255, 69, 73, 79,255,
   65, 67, 69, 77, 78, 80, 83, 90,255, 65, 68,255, 82, 88,255, 68,255, 69,
  255, 78,255, 73,255, 70, 72, 83,255, 69, 85,255, 65, 66, 67, 69, 73, 83,
   87, 88,255, 73, 83,255, 73,255, 68, 84,255, 85,255, 78, 82,255, 69, 82,
  255, 84,255, 67, 73, 76, 78, 81, 82, 88,255, 73, 76, 80,255, 69, 84,255,
   77, 84,255, 69, 73, 76,255, 68, 78,255, 85,255, 76,255, 67, 80, 82, 88,
  255, 70, 78, 80,255, 80,255, 53,255, 67, 68, 75, 88, 90,255, 69, 79,255,
   77, 80,255, 53,255, 67, 68, 75, 77, 78, 80, 84, 88, 90,255, 88,255, 72,
   83,255, 65, 69, 72, 83,255, 69,255, 68, 73, 88,255, 76,255, 78, 83,255,
   71, 87,255, 79, 85,255, 67, 68, 69, 72, 73, 74, 78, 79, 80, 82, 83, 84,
   88,255, 85,255, 68, 86,255, 65, 79, 83, 86,255, 65,255, 80, 84,255, 65,
   69, 79,255, 65, 71, 73,255, 82, 85,255, 72,255, 65, 73,255, 66, 83,255,
   65, 67, 79, 82, 83, 85,255, 76, 82,255, 65,255, 67, 68,255, 67, 90,255,
   69, 71, 79,255, 67, 72,255, 86,255, 84,255, 65, 67, 68, 69, 73, 76, 77,
   78, 80, 82, 83, 90,255, 67, 90,255, 66, 67, 73, 78, 90,255, 69,255, 68,
   73, 82, 88,255, 76, 82,255, 71, 72, 73,255, 88,255, 65, 67, 75,255, 66,
   73,255, 66, 69, 72, 73, 80, 81, 84, 85, 89,255, 69, 73,255, 65, 73,255,
   67, 79, 82, 84,255, 33, 35, 36, 38, 39, 40, 42, 44, 46, 47, 60, 61, 62,
   65, 66, 67, 68, 69, 70, 71, 72, 73, 74, 76, 77, 78, 79, 80, 82, 83, 84,
   85, 86, 87, 88, 92,255, 66, 68, 84,255, 42, 47,255, 35, 36, 38, 46, 47,
  255, 73, 83,255, 76, 78, 82,255, 68, 78,255, 70, 78,255, 68, 69, 73, 80,
   85,255, 42, 47,255, 38, 42, 47, 58,255, 61,255, 42, 47,255, 67, 68, 73,
  255, 65, 73,255, 69, 72,255, 67, 68, 78, 82, 83,255, 67, 90,255, 69,255,
   65,255, 67, 76, 77, 78, 80, 82, 89, 90,255, 65, 67, 80,255, 67, 90,255,
   69, 73, 79,255, 65, 67, 77, 78, 80, 83, 90,255, 65, 68,255, 82, 88,255,
   72, 83,255, 69, 85,255, 65, 66, 67, 69, 73, 83, 87,255, 73,255, 68, 84,
  255, 78, 82,255, 69, 82,255, 84,255, 67, 73, 76, 78, 81, 88,255, 77, 84,
  255, 69, 76,255, 82, 88,255, 70, 78,255, 80,255, 53,255, 67, 68, 75, 88,
   90,255, 69, 79,255, 77, 80,255, 53,255, 67, 68, 75, 77, 78, 80, 84, 88,
   90,255, 88,255, 72, 83,255, 65, 69, 72, 83,255, 69,255, 68, 73, 88,255,
   76,255, 78, 83,255, 79, 85,255, 67, 68, 72, 73, 74, 80, 82, 83, 88,255,
   68, 86,255, 65, 79, 83, 86,255, 65,255, 80,255, 65, 79,255, 65, 71, 73,
  255, 82, 85,255, 66, 83,255, 65, 67, 79, 82, 85,255, 76, 82,255, 65,255,
   67, 68,255, 67, 90,255, 69, 71, 79,255, 67, 72,255, 86,255, 84,255, 65,
   67, 68, 69, 73, 76, 77, 78, 80, 82, 83, 90,255, 67, 90,255, 66, 67, 73,
   78, 90,255, 69,255, 68, 73, 82, 88,255, 76,255, 71, 72, 73,255, 88,255,
   65, 67, 75,255, 66, 73,255, 66, 69, 72, 73, 80, 84, 85, 89,255, 69, 73,
  255, 65, 73,255, 67, 82, 84,255, 36, 38, 42, 47, 65, 66, 67, 68, 69, 70,
   72, 73, 74, 76, 77, 78, 79, 80, 82, 83, 84, 86, 87, 88,255, 42, 47,255,
   85,255, 76,255, 67,255, 78,255, 47, 73,255, 61,255, 42, 47,255, 67, 68,
   73,255, 65, 73,255, 69, 72,255, 67, 68, 78, 82, 83,255, 67, 90,255, 69,
  255, 65,255, 67, 76, 77, 78, 80, 82, 89, 90,255, 65, 67, 80,255, 67, 90,
  255, 69, 73, 79,255, 65, 67, 77, 78, 80, 83, 90,255, 65, 68,255, 82, 88,
  255, 72, 83,255, 69, 85,255, 65, 66, 67, 69, 73, 83, 87,255, 73,255, 68,
   84,255, 78, 82,255, 69, 82,255, 84,255, 67, 73, 76, 78, 88,255, 77, 84,
  255, 69, 76,255, 85,255, 76,255, 67, 82, 88,255, 70, 78,255, 80,255, 53,
  255, 67, 68, 75, 88, 90,255, 69, 79,255, 77, 80,255, 53,255, 67, 68, 75,
   77, 78, 80, 84, 88, 90,255, 88,255, 72, 83,255, 65, 69, 72, 83,255, 69,
  255, 68, 73, 88,255, 76,255, 78, 83,255, 79, 85,255, 67, 68, 72, 73, 74,
   80, 82, 83, 88,255, 68, 86,255, 65, 79, 83, 86,255, 65,255, 80,255, 65,
   79,255, 65, 71, 73,255, 82, 85,255, 66, 83,255, 65, 67, 79, 82, 85,255,
   76, 82,255, 65,255, 67, 68,255, 67, 90,255, 69, 71, 79,255, 67, 72,255,
   86,255, 84,255, 65, 67, 68, 69, 73, 76, 77, 78, 80, 82, 83, 90,255, 67,
   90,255, 66, 67, 73, 78, 90,255, 69,255, 68, 73, 82, 88,255, 76,255, 71,
   72, 73,255, 88,255, 65, 67, 75,255, 66, 73,255, 66, 72, 73, 80, 84, 85,
   89,255, 69, 73,255, 65, 73,255, 67, 82, 84,255, 36, 38, 42, 47, 65, 66,
   67, 68, 69, 70, 72, 73, 74, 76, 77, 78, 79, 80, 82, 83, 84, 86, 87, 88,
  255, 36, 38,255, 42, 47,255, 47,255, 76, 80,255, 71, 87,255, 78, 79,255,
   36, 38, 39, 67, 68, 70, 72, 73, 76, 78, 80, 83,255, 78, 88,255, 69, 72,
   76, 86,255, 66, 68, 84,255, 42, 47,255, 85,255, 76,255, 67,255, 78,255,
   35, 36, 38, 42, 46, 47, 73,255, 47, 61,255, 42, 47,255, 76, 89,255, 69,
  255, 66, 83, 87,255, 68, 84,255, 78, 82,255, 69, 82,255, 84,255, 67, 78,
   81, 88,255, 85,255, 76,255, 67,255, 78,255, 78, 83,255, 73, 83,255, 65,
   79, 83,255, 65, 79,255, 65, 82, 85,255, 69, 84, 89,255, 69, 73,255, 36,
   42, 47, 65, 66, 67, 68, 69, 70, 72, 73, 76, 77, 78, 79, 80, 83, 84, 86,
   87, 92,255, 85,255, 76,255, 67,255, 78,255, 73,255, 42, 47,255, 42, 47,
  255, 42, 47, 92,255, 42, 47,255, 47, 73, 80,255, 42, 47,255, 33, 47,255,
   67,255, 69, 88,255, 76,255, 66, 68, 72, 80, 83,255, 40, 65, 66, 67, 68,
   69, 72, 76, 77,255, 67,255, 69,255, 76,255, 66, 68, 72, 83,255, 67,255,
   69,255, 76,255, 65, 66, 68, 72, 80,255, 67,255, 69,255, 66, 68,255, 61,
  255, 42, 47,255, 60, 61,255, 61,255, 61, 62,255, 33, 42, 44, 47, 60, 61,
   62,255, 61,255, 42, 47,255, 60, 61,255, 61,255, 61, 62,255, 69, 84,255,
   69, 84,255, 76, 82,255, 72,255, 33, 42, 44, 47, 60, 61, 62, 65, 69, 71,
   76, 77, 78, 79, 83, 88,255, 76, 89,255, 69,255, 66, 83, 87,255, 68, 84,
  255, 78, 82,255, 69, 82,255, 84,255, 67, 78, 88,255, 78, 83,255, 73, 83,
  255, 65, 79, 83,255, 65, 79,255, 65, 82, 85,255, 84, 89,255, 69, 73,255,
   36, 42, 65, 66, 67, 68, 69, 70, 72, 76, 77, 78, 79, 80, 83, 84, 86, 87,
  255, 42, 47,255, 44, 47,255, 61,255, 42, 47,255, 60, 61,255, 61,255, 61,
   62,255, 69, 84,255, 69, 84,255, 82,255, 76, 82,255, 72,255, 33, 42, 44,
   47, 60, 61, 62, 65, 69, 71, 76, 77, 78, 79, 81, 83, 88,255, 39,255, 61,
  255, 42, 47,255, 60, 61,255, 61,255, 61, 62,255, 69, 84,255, 69, 84,255,
   82,255, 76, 82,255, 72,255, 33, 42, 44, 47, 60, 61, 62, 71, 76, 77, 78,
   79, 81, 83, 88,255, 42, 47,255, 76, 80,255, 71, 87,255, 78, 79,255, 36,
   38, 39, 42, 47, 67, 68, 70, 72, 73, 76, 78, 80, 83, 92,255, 76, 80,255,
   71, 87,255, 78, 79,255, 36, 38, 39, 67, 68, 70, 72, 73, 76, 80, 83,255,
   42, 47,255, 76, 80,255, 71, 87,255, 78, 79,255, 36, 38, 39, 42, 47, 67,
   68, 70, 72, 73, 76, 80, 83, 92,255, 61,255, 60, 61,255, 61,255, 61, 62,
  255, 69, 84,255, 69, 84,255, 76, 82,255, 72,255, 33, 42, 44, 60, 61, 62,
   65, 69, 71, 76, 77, 78, 79, 83, 88,255, 61,255, 42, 47,255, 60, 61,255,
   61,255, 61, 62,255, 69, 84,255, 69, 84,255, 76, 82,255, 72,255, 33, 44,
   47, 60, 61, 62, 65, 69, 71, 76, 78, 79, 83, 88,255, 61,255, 42, 47,255,
   61,255, 61,255, 61,255, 69, 84,255, 69, 84,255, 33, 44, 47, 60, 61, 62,
   65, 69, 71, 76, 78, 79, 88,255, 61,255, 42, 47,255, 61,255, 61,255, 61,
  255, 69, 84,255, 69, 84,255, 33, 44, 47, 60, 61, 62, 69, 71, 76, 78, 79,
   88,255, 61,255, 42, 47,255, 61,255, 61,255, 61,255, 69, 84,255, 69, 84,
  255, 33, 44, 47, 60, 61, 62, 69, 71, 76, 78, 79,255, 42, 47,255, 61,255,
   61,255, 61,255, 69, 84,255, 69, 84,255, 33, 47, 60, 61, 62, 69, 71, 76,
   78,255, 61,255, 42, 47,255, 42, 47,255, 60, 61,255, 61,255, 61, 62,255,
   69, 84,255, 69, 84,255, 76, 82,255, 72,255, 33, 42, 44, 47, 60, 61, 62,
   65, 69, 71, 76, 77, 78, 79, 83, 88, 92,255, 76, 89,255, 69,255, 66, 83,
   87,255, 68, 84,255, 78, 82,255, 69, 82,255, 84,255, 67, 78, 81, 88,255,
   78, 83,255, 73, 83,255, 65, 79, 83,255, 65, 79,255, 65, 82, 85,255, 69,
   84, 89,255, 69, 73,255, 36, 42, 65, 66, 67, 68, 69, 70, 72, 76, 77, 78,
   79, 80, 83, 84, 86, 87,255, 39,255, 67, 68, 73,255, 65, 73,255, 67, 68,
   78, 82, 83,255, 67, 90,255, 69,255, 65,255, 67, 77, 78, 80, 82, 90,255,
   65, 67, 80,255, 67, 90,255, 69, 73, 79,255, 65, 67, 77, 78, 80, 90,255,
   65, 68,255, 82, 88,255, 72, 83,255, 65, 67, 69, 73, 83,255, 77, 84,255,
   76,255, 82, 88,255, 78,255, 80,255, 53,255, 67, 68, 75, 88, 90,255, 69,
   79,255, 77, 80,255, 53,255, 67, 68, 75, 77, 78, 80, 84, 88, 90,255, 88,
  255, 72, 83,255, 65, 69, 72, 83,255, 69,255, 68, 73, 88,255, 76,255, 79,
   85,255, 67, 68, 72, 74, 80, 82, 88,255, 79, 86,255, 65, 73,255, 82, 85,
  255, 67, 79, 85,255, 76, 82,255, 65,255, 67, 68,255, 67, 90,255, 69, 71,
   79,255, 67, 72,255, 86,255, 84,255, 65, 67, 68, 69, 73, 76, 77, 78, 80,
   82, 83, 90,255, 67, 90,255, 66, 67, 73, 78, 90,255, 69,255, 68, 73, 82,
   88,255, 76,255, 71, 72, 73,255, 88,255, 65, 67,255, 66, 73,255, 66, 72,
   73, 80, 84, 85,255, 65, 73,255, 67, 82, 84,255, 65, 66, 67, 68, 69, 72,
   73, 74, 76, 77, 78, 79, 80, 82, 83, 88,255, 42, 47,255, 36, 38, 47,255,
   42, 47,255, 33, 44, 47,255, 44,255, 42, 47,255, 47, 79, 81,255, 92,255,
   69,255, 69,255, 76, 80,255, 73,255, 71, 87,255, 78, 79,255, 36, 38, 39,
   40, 65, 66, 67, 68, 69, 70, 72, 73, 76, 77, 78, 80, 83,255, 33,255, 42,
   47,255, 47, 92,255
};

static const unsigned char ag_key_act[] = {
  0,3,3,3,4,0,0,4,7,4,6,4,2,4,2,4,0,5,0,2,2,2,4,0,4,0,4,0,0,0,4,3,3,3,4,
  0,0,4,0,0,4,0,4,0,0,4,5,5,5,4,5,5,5,4,7,7,4,7,2,2,7,2,4,5,5,4,5,4,5,4,
  5,7,5,2,6,6,7,5,4,5,5,5,4,5,5,4,5,5,5,4,7,5,7,6,2,6,7,5,4,5,5,4,5,5,4,
  5,4,6,4,2,4,2,4,2,7,7,4,7,7,4,2,5,2,6,5,6,5,5,4,7,7,4,7,4,6,7,4,5,4,7,
  7,4,2,7,4,2,4,7,5,2,2,6,7,2,4,7,7,5,4,5,5,4,7,5,4,7,7,6,4,7,7,4,7,4,6,
  4,2,7,5,5,4,6,6,5,4,5,4,5,4,5,5,5,6,5,4,5,5,4,5,5,4,5,4,5,5,5,6,2,6,2,
  6,5,4,5,4,5,5,4,6,2,7,7,4,5,4,6,5,5,4,2,4,7,7,4,5,5,4,7,7,4,7,2,5,2,2,
  7,5,2,2,7,7,5,7,4,7,4,6,5,4,7,2,7,7,4,7,4,6,5,4,7,5,2,4,5,5,5,4,6,7,4,
  7,4,7,7,4,7,7,4,7,6,7,2,7,2,4,5,5,4,7,4,5,7,4,5,5,4,5,5,5,4,5,7,4,5,4,
  6,4,2,6,7,7,7,2,5,2,6,2,2,5,4,5,5,4,5,5,5,2,5,4,5,4,6,5,5,5,4,6,5,4,5,
  7,5,4,5,4,6,5,7,4,5,5,4,2,7,2,7,6,7,2,2,7,4,7,7,4,5,5,4,7,7,2,7,4,6,0,
  6,0,3,3,2,0,2,2,1,1,1,6,6,6,6,6,2,2,6,2,2,6,6,2,2,2,2,2,2,7,7,7,2,3,4,
  3,3,3,4,0,0,4,0,5,0,2,2,4,7,7,4,2,7,7,4,7,7,4,6,7,4,7,2,2,7,7,4,0,0,4,
  0,3,2,0,4,0,4,0,0,4,5,5,5,4,5,5,4,7,7,4,7,2,2,7,2,4,5,5,4,5,4,5,4,5,7,
  5,2,6,6,7,5,4,5,5,5,4,5,5,4,5,5,5,4,7,5,6,2,6,7,5,4,5,5,4,5,5,4,7,7,4,
  7,7,4,2,5,2,2,5,6,5,4,7,4,6,7,4,7,7,4,2,7,4,2,4,7,5,7,2,7,2,4,7,5,4,7,
  2,4,5,5,4,5,6,4,5,4,5,4,5,5,5,6,5,4,5,5,4,5,5,4,5,4,5,5,5,6,2,6,2,6,5,
  4,5,4,5,5,4,6,2,7,7,4,5,4,6,5,5,4,2,4,7,7,4,7,7,4,7,2,2,2,7,2,7,7,7,4,
  7,5,4,7,2,7,7,4,7,4,6,4,7,2,4,5,5,5,4,2,7,4,7,7,4,7,7,7,7,2,4,5,5,4,7,
  4,5,7,4,5,5,4,5,5,5,4,5,7,4,5,4,6,4,2,6,7,7,7,2,5,2,6,2,2,5,4,5,5,4,5,
  5,5,2,5,4,5,4,6,5,5,5,4,2,4,5,7,5,4,5,4,6,5,7,4,5,5,4,2,7,2,7,2,2,2,7,
  4,7,7,4,5,5,4,7,2,7,4,6,0,3,2,2,2,2,2,2,7,2,2,2,2,2,2,2,2,2,2,2,7,7,2,
  4,0,0,4,7,4,6,4,2,4,2,4,2,2,4,0,4,0,0,4,5,5,5,4,5,5,4,7,7,4,7,2,2,7,2,
  4,5,5,4,5,4,5,4,5,7,5,2,6,6,7,5,4,5,5,5,4,5,5,4,5,5,5,4,7,5,6,2,6,7,5,
  4,5,5,4,5,5,4,7,7,4,7,7,4,2,5,2,2,5,6,5,4,7,4,6,7,4,7,7,4,2,7,4,2,4,7,
  5,7,2,2,4,7,5,4,7,2,4,7,4,6,4,2,5,5,4,5,6,4,5,4,5,4,5,5,5,6,5,4,5,5,4,
  5,5,4,5,4,5,5,5,6,2,6,2,6,5,4,5,4,5,5,4,6,2,7,7,4,5,4,6,5,5,4,2,4,7,7,
  4,7,7,4,7,2,2,2,7,2,7,7,7,4,7,5,4,7,2,7,7,4,7,4,6,4,7,2,4,5,5,5,4,2,7,
  4,7,7,4,7,7,7,7,2,4,5,5,4,7,4,5,7,4,5,5,4,5,5,5,4,5,7,4,5,4,6,4,2,6,7,
  7,7,2,5,2,6,2,2,5,4,5,5,4,5,5,5,2,5,4,5,4,6,5,5,5,4,2,4,5,7,5,4,5,4,6,
  5,7,4,5,5,4,2,2,7,2,2,2,7,4,7,7,4,5,5,4,7,2,7,4,6,0,3,2,2,2,2,2,2,7,2,
  2,2,2,2,2,2,2,2,2,2,7,7,2,4,5,0,4,0,0,4,2,4,7,5,4,5,5,4,5,2,4,5,0,3,7,
  7,2,7,7,2,7,7,7,4,7,7,4,2,7,7,7,4,3,3,3,4,0,0,4,7,4,6,4,2,4,2,4,0,5,0,
  3,2,2,2,4,0,0,4,0,0,4,7,7,4,7,4,5,6,5,4,5,7,4,7,7,4,2,7,4,2,4,7,2,7,2,
  4,7,4,6,4,2,4,2,4,7,7,4,2,7,4,7,7,7,4,7,7,4,7,7,7,4,7,7,7,4,7,7,4,3,2,
  2,7,2,7,2,2,7,7,2,2,2,2,7,2,2,2,7,7,3,4,7,4,6,4,2,4,2,4,2,4,3,3,4,0,0,
  4,3,2,3,4,0,0,4,2,7,7,4,0,0,4,5,2,4,5,4,5,5,4,5,4,6,6,6,7,7,4,3,5,5,5,
  5,5,5,5,5,4,5,4,5,4,5,4,6,6,6,7,4,5,4,5,4,5,4,5,6,6,6,7,4,5,4,5,4,6,6,
  4,0,4,0,0,4,0,0,4,0,4,0,0,4,6,3,0,2,1,1,1,4,0,4,0,0,4,0,0,4,0,4,0,0,4,
  5,5,4,5,5,4,5,5,4,2,4,6,3,0,2,1,1,1,7,7,2,2,7,7,7,2,7,4,7,7,4,7,4,5,6,
  5,4,5,7,4,7,7,4,2,7,4,2,4,7,2,2,4,7,7,4,2,7,4,7,7,7,4,7,7,4,7,7,7,4,7,
  7,4,7,7,4,3,3,7,2,7,2,2,7,7,2,2,2,7,2,2,2,7,7,4,0,0,4,0,2,4,0,4,0,0,4,
  0,0,4,0,4,0,0,4,5,5,4,5,5,4,5,4,5,5,4,2,4,6,3,0,2,1,1,1,7,7,2,2,7,7,6,
  5,2,7,4,3,4,0,4,0,0,4,0,0,4,0,4,0,0,4,5,5,4,5,5,4,5,4,5,5,4,2,4,6,3,0,
  2,1,1,1,2,2,7,7,6,5,2,7,4,0,0,4,7,5,4,5,5,4,5,2,4,5,0,3,3,2,7,7,2,7,7,
  2,7,7,7,3,4,7,5,4,5,5,4,5,2,4,5,0,3,7,7,2,7,7,2,7,7,4,0,0,4,7,5,4,5,5,
  4,5,2,4,5,0,3,3,2,7,7,2,7,7,2,7,7,3,4,0,4,0,0,4,0,4,0,0,4,5,5,4,5,5,4,
  5,5,4,2,4,6,3,0,1,1,1,7,7,2,2,7,7,7,2,7,4,0,4,0,0,4,0,0,4,0,4,0,0,4,5,
  5,4,5,5,4,5,5,4,2,4,6,0,2,1,1,1,7,7,2,2,7,7,2,7,4,0,4,0,0,4,0,4,0,4,0,
  4,5,5,4,5,5,4,6,0,2,1,1,1,7,7,2,2,7,7,7,4,0,4,0,0,4,0,4,0,4,0,4,5,5,4,
  5,5,4,6,0,2,1,1,1,7,2,2,7,7,7,4,0,4,0,0,4,0,4,0,4,0,4,5,5,4,5,5,4,6,0,
  2,1,1,1,7,2,2,7,7,4,0,0,4,0,4,0,4,0,4,5,5,4,5,5,4,3,2,1,1,1,7,2,2,7,4,
  0,4,0,0,4,0,0,4,0,0,4,0,4,0,0,4,5,5,4,5,5,4,5,5,4,2,4,6,2,0,2,1,1,1,7,
  7,2,2,7,7,7,2,7,3,4,7,7,4,7,4,5,6,5,4,5,7,4,7,7,4,2,7,4,2,4,7,2,7,2,4,
  7,7,4,2,7,4,7,7,7,4,7,7,4,7,7,7,4,7,7,7,4,7,7,4,3,3,7,2,7,2,2,7,7,2,2,
  2,7,2,2,2,7,7,4,3,4,5,5,5,4,5,5,4,7,2,2,7,7,4,5,5,4,5,4,5,4,5,5,2,6,6,
  5,4,5,5,5,4,5,5,4,5,5,5,4,7,5,6,2,6,5,4,5,5,4,5,5,4,7,7,4,2,2,2,5,7,4,
  7,5,4,2,4,5,5,4,6,4,5,4,5,4,5,5,5,6,5,4,5,5,4,5,5,4,5,4,5,5,5,6,2,6,2,
  6,5,4,5,4,5,5,4,6,2,7,7,4,5,4,6,5,5,4,2,4,7,7,4,7,2,2,7,2,7,7,4,7,7,4,
  5,5,4,2,7,4,7,7,7,4,5,5,4,7,4,5,7,4,5,5,4,5,5,5,4,5,7,4,5,4,6,4,2,6,7,
  7,7,2,5,2,6,2,2,5,4,5,5,4,5,5,5,2,5,4,5,4,6,5,5,5,4,2,4,5,7,5,4,5,4,6,
  5,4,5,5,4,2,2,7,2,2,2,4,5,5,4,7,2,7,4,2,2,2,2,7,2,2,2,2,2,7,2,2,2,2,2,
  4,0,0,4,5,0,2,4,0,0,4,5,0,2,4,0,4,0,0,4,2,5,5,4,3,4,7,4,7,4,7,5,4,7,4,
  5,5,4,5,2,4,5,0,3,3,5,5,6,6,5,2,6,7,6,5,7,7,7,4,5,4,0,0,4,2,3,4
};

static const unsigned short ag_key_parm[] = {
    0,162,164,163,  0,429,426,  0,  4,  0,  6,  0,  0,  0,  0,  0,439, 96,
  135,  0,  0,  0,  0,500,  0,463,  0,525,428,464,  0,162,164,163,  0,429,
  426,  0,515,504,  0,497,  0,502,517,  0,280,282,322,  0,284,116,324,  0,
   44,182,  0,320,  0,  0,186,  0,  0,424,422,  0,430,  0,418,  0,166, 48,
  426,  0,428,416, 52,420,  0,190,192,286,  0,332,334,  0,338,340,342,  0,
  326,328,128,330,  0,336, 38,344,  0,194,306,  0,288,308,  0,144,  0, 36,
    0,  0,  0,  0,  0,  0,390,396,  0, 40,198,  0,  0, 50,  0,168,196, 46,
   56,176,  0, 24, 28,  0, 30,  0, 78, 14,  0,  0,  0, 18, 64,  0,  0, 62,
    0,  0,  0, 72,202,  0,  0,100, 32,  0,  0, 74,126,138,  0,104,108,  0,
  200,204,  0, 12,140,170,  0, 20, 26,  0,  4,  0,  6,  0,  0, 98,290,310,
    0, 22,346,136,  0,362,  0,368,  0,364,374,372,366,376,  0,380,382,  0,
  354,370,  0,352,  0,348,358,356,360,  0,378,  0,350,384,  0,270,  0,388,
  394,  0,386,  0,392,398,  0,210,  0,400,208,206,  0,  0,  0, 90, 10,  0,
  132,142,  0,276,278,  0,442,  0,106,  0,  0,444,130,  0,  0,266, 82,110,
  312,  0, 66,  0,122,292,  0, 92,  0, 80,314,  0, 86,  0,212,124,  0, 68,
  102,  0,  0,294, 42,402,  0,112,404,  0,214,  0,  8, 76,  0, 60,274,  0,
   94,178,272,  0,174,  0,  0,216,218,  0,432,  0,226,228,  0,234,236,  0,
  240,304,242,  0,244,188,  0,246,  0,318,  0,  0,220,230,222,224,  0,232,
    0,238,  0,  0,248,  0,440,436,  0,296,438,406,  0,434,  0,254,  0,408,
  252,184,250,  0,118,120,  0,302,258,316,  0,268,  0,410,260, 70,  0,298,
  412,  0,  0,  2,  0,256,172,134,  0,  0, 88,  0, 54, 84,  0,300,414,  0,
  262,114,  0,264,  0,180,439, 96,135,232,242,  0,494,  0,  0,508,498,506,
  164,150,152,154,156,  0,  0,158,  0,  0,160,162,  0,  0,  0,  0,  0,  0,
   34, 16, 58,  0,459,  0,162,164,163,  0,429,426,  0,439, 96,135,  0,  0,
    0, 24, 28,  0,  0, 30, 32,  0, 20, 26,  0, 22,  4,  0, 36,  0,  0,  8,
   34,  0,429,426,  0,135,464,  0,161,  0,463,  0,429,426,  0,280,282,322,
    0,284,324,  0, 44,182,  0,320,  0,  0,186,  0,  0,424,422,  0,430,  0,
  418,  0,166, 48,426,  0,428,416, 52,420,  0,190,192,286,  0,332,334,  0,
  338,340,342,  0,326,328,330,  0,336, 38,344,  0,194,306,  0,288,308,  0,
  390,396,  0, 40,198,  0,  0, 50,  0,  0,196, 46, 56,  0, 30,  0, 78, 14,
    0, 18, 64,  0,  0, 62,  0,  0,  0, 72,202, 28,  0,  0,  0,  0,200,204,
    0, 12,  0,  0,290,310,  0, 22,346,  0,362,  0,368,  0,364,374,372,366,
  376,  0,380,382,  0,354,370,  0,352,  0,348,358,356,360,  0,378,  0,350,
  384,  0,270,  0,388,394,  0,386,  0,392,398,  0,210,  0,400,208,206,  0,
    0,  0, 90, 10,  0,276,278,  0,442,  0,  0,  0,444,  0,266, 82,312,  0,
   66,292,  0, 92,  0, 80,314,  0, 86,  0,212,  0, 68,  0,  0,294, 42,402,
    0,  0,404,  0, 60,274,  0, 94,214,272, 76,  0,  0,216,218,  0,432,  0,
  226,228,  0,234,236,  0,240,304,242,  0,244,188,  0,246,  0,318,  0,  0,
  220,230,222,224,  0,232,  0,238,  0,  0,248,  0,440,436,  0,296,438,406,
    0,434,  0,254,  0,408,252,184,250,  0,  0,  0,302,258,316,  0,268,  0,
  410,260, 70,  0,298,412,  0,  0,  2,  0,256,  0,  0,  0, 88,  0, 54, 84,
    0,300,414,  0,262,  0,264,  0, 96,135,464,  0,  0,  0,  0,  0,  0, 74,
    0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0, 16, 58,  0,  0,429,426,  0,
    4,  0,  6,  0,  0,  0,  0,  0,  0,  0,  0,463,  0,429,426,  0,280,282,
  322,  0,284,324,  0, 44,182,  0,320,  0,  0,186,  0,  0,424,422,  0,430,
    0,418,  0,166, 48,426,  0,428,416, 52,420,  0,190,192,286,  0,332,334,
    0,338,340,342,  0,326,328,330,  0,336, 38,344,  0,194,306,  0,288,308,
    0,390,396,  0, 40,198,  0,  0, 50,  0,  0,196, 46, 56,  0, 30,  0, 78,
   14,  0, 18, 64,  0,  0, 62,  0,  0,  0, 72,202, 28,  0,  0,  0,200,204,
    0, 12,  0,  0,  4,  0,  6,  0,  0,290,310,  0, 22,346,  0,362,  0,368,
    0,364,374,372,366,376,  0,380,382,  0,354,370,  0,352,  0,348,358,356,
  360,  0,378,  0,350,384,  0,270,  0,388,394,  0,386,  0,392,398,  0,210,
    0,400,208,206,  0,  0,  0, 90, 10,  0,276,278,  0,442,  0,  0,  0,444,
    0,266, 82,312,  0, 66,292,  0, 92,  0, 80,314,  0, 86,  0,212,  0, 68,
    0,  0,294, 42,402,  0,  0,404,  0, 60,274,  0, 94,214,272, 76,  0,  0,
  216,218,  0,432,  0,226,228,  0,234,236,  0,240,304,242,  0,244,188,  0,
  246,  0,318,  0,  0,220,230,222,224,  0,232,  0,238,  0,  0,248,  0,440,
  436,  0,296,438,406,  0,434,  0,254,  0,408,252,184,250,  0,  0,  0,302,
  258,316,  0,268,  0,410,260, 70,  0,298,412,  0,  0,  0,256,  0,  0,  0,
   88,  0, 54, 84,  0,300,414,  0,262,  0,264,  0, 96,135,464,  0,  0,  0,
    0,  0,  0, 74,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0, 16, 58,  0,
    0, 96,135,  0,429,426,  0,  0,  0,126,138,  0,132,142,  0,130,  0,  0,
   96,135,232,128,144,  0,140,136,  0,124, 94,134,  0, 14, 18,  0,  0, 12,
   10, 16,  0,162,164,163,  0,429,426,  0,  4,  0,  6,  0,  0,  0,  0,  0,
  439, 96,135,428,  0,  0,  0,  0,428,464,  0,429,426,  0, 48, 52,  0, 40,
    0, 50, 46, 56,  0, 78, 14,  0, 18, 64,  0,  0, 62,  0,  0,  0, 72,  0,
    0,  0,  0,  4,  0,  6,  0,  0,  0,  0,  0, 90, 10,  0,  0, 82,  0, 92,
   66, 80,  0, 68, 86,  0, 94, 76, 60,  0,  2, 70, 88,  0, 54, 84,  0,463,
    0,  0, 44,  0, 38,  0,  0, 74, 12,  0,  0,  0,  0, 42,  0,  0,  0, 16,
   58,459,  0,  4,  0,  6,  0,  0,  0,  0,  0,  0,  0,428,429,  0,429,426,
    0,428,  0,459,  0,429,426,  0,  0, 98, 94,  0,429,426,  0,180,  0,  0,
  166,  0,168,176,  0,170,  0,150,154,158,178,172,  0,242,164,150,152,154,
  156,158,160,162,  0,166,  0,168,  0,170,  0,150,154,158,172,  0,166,  0,
  168,  0,170,  0,164,150,154,158,174,  0,166,  0,168,  0,150,154,  0,500,
    0,429,426,  0,515,504,  0,497,  0,502,517,  0,180,525,494,  0,508,498,
  506,  0,500,  0,429,426,  0,515,504,  0,497,  0,502,517,  0,104,108,  0,
  106,110,  0,118,120,  0,  0,  0,180,525,494,  0,508,498,506,116,100,  0,
    0,122,102,112,  0,114,  0, 48, 52,  0, 40,  0, 50, 46, 56,  0, 78, 14,
    0, 18, 64,  0,  0, 62,  0,  0,  0, 72,  0,  0,  0, 90, 10,  0,  0, 82,
    0, 92, 66, 80,  0, 68, 86,  0, 94, 76, 60,  0, 70, 88,  0, 54, 84,  0,
  463,464, 44,  0, 38,  0,  0, 74, 12,  0,  0,  0, 42,  0,  0,  0, 16, 58,
    0,429,426,  0,494,  0,  0,500,  0,429,426,  0,515,504,  0,497,  0,502,
  517,  0,104,108,  0,106,110,  0,112,  0,118,120,  0,  0,  0,180,525,494,
    0,508,498,506,116,100,  0,  0,122,102,148,146,  0,114,  0,234,  0,500,
    0,429,426,  0,515,504,  0,497,  0,502,517,  0,104,108,  0,106,110,  0,
  112,  0,118,120,  0,  0,  0,180,525,494,  0,508,498,506,  0,  0,122,102,
  148,146,  0,114,  0,429,426,  0,126,138,  0,132,142,  0,130,  0,  0, 96,
  135,232,428,  0,128,144,  0,140,136,  0,124, 94,134,459,  0,126,138,  0,
  132,142,  0,130,  0,  0, 96,135,232,128,144,  0,140,136,  0, 94,134,  0,
  429,426,  0,126,138,  0,132,142,  0,130,  0,  0, 96,135,232,428,  0,128,
  144,  0,140,136,  0, 94,134,459,  0,500,  0,515,504,  0,497,  0,502,517,
    0,104,108,  0,106,110,  0,118,120,  0,  0,  0,180,525,494,508,498,506,
  116,100,  0,  0,122,102,112,  0,114,  0,500,  0,429,426,  0,515,504,  0,
  497,  0,502,517,  0,104,108,  0,106,110,  0,118,120,  0,  0,  0,180,494,
    0,508,498,506,116,100,  0,  0,102,112,  0,114,  0,500,  0,429,426,  0,
  504,  0,497,  0,502,  0,104,108,  0,106,110,  0,180,494,  0,508,498,506,
  116,100,  0,  0,102,112,114,  0,500,  0,429,426,  0,504,  0,497,  0,502,
    0,104,108,  0,106,110,  0,180,494,  0,508,498,506,100,  0,  0,102,112,
  114,  0,500,  0,429,426,  0,504,  0,497,  0,502,  0,104,108,  0,106,110,
    0,180,494,  0,508,498,506,100,  0,  0,102,112,  0,429,426,  0,504,  0,
  497,  0,502,  0,104,108,  0,106,110,  0,500,  0,508,498,506,100,  0,  0,
  102,  0,500,  0,525,428,  0,429,426,  0,515,504,  0,497,  0,502,517,  0,
  104,108,  0,106,110,  0,118,120,  0,  0,  0,180,  0,494,  0,508,498,506,
  116,100,  0,  0,122,102,112,  0,114,459,  0, 48, 52,  0, 40,  0, 50, 46,
   56,  0, 78, 14,  0, 18, 64,  0,  0, 62,  0,  0,  0, 72,  0,  0,  0,  0,
   90, 10,  0,  0, 82,  0, 92, 66, 80,  0, 68, 86,  0, 94, 76, 60,  0,  2,
   70, 88,  0, 54, 84,  0,463,464, 44,  0, 38,  0,  0, 74, 12,  0,  0,  0,
   42,  0,  0,  0, 16, 58,  0,232,  0,280,282,322,  0,284,324,  0,320,  0,
    0,186,182,  0,424,422,  0,430,  0,418,  0,166,426,  0,428,416,420,  0,
  190,192,286,  0,332,334,  0,338,340,342,  0,326,328,330,  0,336,344,  0,
  194,306,  0,288,308,  0,390,396,  0,  0,  0,  0,196,198,  0,200,204,  0,
    0,  0,290,310,  0,346,  0,362,  0,368,  0,364,374,372,366,376,  0,380,
  382,  0,354,370,  0,352,  0,348,358,356,360,  0,378,  0,350,384,  0,270,
    0,388,394,  0,386,  0,392,398,  0,210,  0,400,208,206,  0,  0,  0,276,
  278,  0,442,  0,  0,444,  0,266,312,  0,292,314,  0,294,402,  0,  0,404,
    0,214,272,274,  0,216,218,  0,432,  0,226,228,  0,234,236,  0,240,304,
  242,  0,244,188,  0,246,  0,318,  0,  0,220,230,222,224,  0,232,  0,238,
    0,  0,248,  0,440,436,  0,296,438,406,  0,434,  0,254,  0,408,252,184,
  250,  0,  0,  0,302,258,316,  0,268,  0,410,260,  0,298,412,  0,  0,  0,
  256,  0,  0,  0,  0,300,414,  0,262,  0,264,  0,  0,  0,  0,  0,202,  0,
    0,  0,  0,  0,212,  0,  0,  0,  0,  0,  0,429,426,  0, 96,135,  0,  0,
  429,426,  0,180,494,  0,  0,494,  0,429,426,  0,  0,148,146,  0,459,  0,
  128,  0,144,  0,126,138,  0,140,  0,132,142,  0,130,  0,  0, 96,135,232,
  242,164,150,152,154,156,  0,158,136,160,162,124, 94,134,  0,180,  0,429,
  426,  0,  0,459,  0
};

static const unsigned short ag_key_jmp[] = {
    0,  0,  3,  7,  0,  0,  0,  0, 11,  0,  8,  0, 10,  0, 12,  0,  0,  0,
    0,  1,  5, 14,  0,  0,  0,  0,  0,  0,  0,  0,  0, 21, 24, 28,  0,  0,
    0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,
   37, 39,  0, 32, 46, 50, 34, 54,  0,  0,  0,  0,  0,  0,  0,  0,  0, 41,
    0, 63, 66, 68, 45,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,
   48,  0, 51, 79, 83, 86, 54,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,105,
    0,107,  0,109,  0,111, 57, 59,  0, 61, 63,  0, 99,  0,102,113,  0,117,
    0,  0,  0, 68, 70,  0, 72,  0,132, 74,  0,  0,  0, 81, 85,  0,139, 87,
    0,142,  0, 65,  0,129,134,137, 77,145,  0, 89, 92,  0,  0,  0,  0,  0,
  101,  0,  0, 96, 98,162,  0,104,107,  0,111,  0,172,  0,174,114,  0,  0,
    0,169,176,  0,  0,  0,  0,  0,  0,  0,  0,  0,187,  0,  0,  0,  0,  0,
    0,  0,  0,  0,  0,  0,  0,  0,185,189,195,198,201,  0,  0,  0,  0,  0,
    0,  0,213,215,122,124,  0,  0,  0,223,  0,  0,  0,225,  0,126,128,  0,
    0,  0,  0,133,135,  0,118,218,  0,229,231,130,  0,234,237,138,141,  0,
  147,  0,154,  0,254,  0,  0,149,256,157,163,  0,168,  0,264,  0,  0,165,
    0,266,  0,  0,  0,  0,  0,273,171,  0,176,  0,180,184,  0,190,194,  0,
  173,280,178,282,188,285,  0,  0,  0,  0,196,  0,  0,206,  0,  0,  0,  0,
    0,  0,  0,  0,  0,208,  0,  0,  0,313,  0,295,298,199,202,204,300,  0,
  303,306,310,315,  0,  0,  0,  0,  0,  0,  0,  0,330,  0,  0,  0,  0,339,
    0,  0,  0,  0,341,  0,  0,  0,214,  0,  0,  0,  0,353,  0,219,  0,  0,
    0,  0,333,210,346,212,349,216,355,359,222,  0,224,227,  0,  0,  0,  0,
  247,250,375,252,  0, 23,  0, 25,  0, 14, 17, 27,  0, 31, 35, 38, 41, 43,
   57, 70, 90,120,147,155,159,165,181,203,240,259,269,277,288,317,362,372,
  231,236,243,378,255,  0,257,260,264,  0,  0,  0,  0,  0,  0,  0,420,424,
    0,274,276,  0,433,278,282,  0,286,289,  0,440,293,  0,268,436,443,299,
  305,  0,  0,  0,  0,  0,310,452,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,
    0,  0,  0,  0,319,321,  0,314,465,469,316,472,  0,  0,  0,  0,  0,  0,
    0,  0,  0,323,  0,481,484,486,327,  0,  0,  0,  0,  0,  0,  0,  0,  0,
    0,  0,  0,  0,330,  0,497,501,504,333,  0,  0,  0,  0,  0,  0,  0,  0,
  336,338,  0,340,342,  0,516,  0,519,522,  0,525,  0,  0,350,  0,536,352,
    0,357,361,  0,541,363,  0,544,  0,344,  0,347,538,355,547,  0,371,  0,
    0,369,556,  0,  0,  0,  0,  0,562,  0,  0,  0,  0,  0,  0,  0,  0,570,
    0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,568,572,578,581,584,
    0,  0,  0,  0,  0,  0,  0,596,598,378,380,  0,  0,  0,606,  0,  0,  0,
  608,  0,382,384,  0,389,391,  0,374,601,612,614,386,617,394,397,403,  0,
  410,  0,  0,405,630,414,420,  0,425,  0,638,  0,422,640,  0,  0,  0,  0,
    0,645,428,  0,443,447,  0,430,433,436,438,652,  0,  0,  0,  0,449,  0,
    0,459,  0,  0,  0,  0,  0,  0,  0,  0,  0,461,  0,  0,  0,679,  0,661,
  664,452,455,457,666,  0,669,672,676,681,  0,  0,  0,  0,  0,  0,  0,  0,
  696,  0,  0,  0,  0,705,  0,  0,  0,  0,707,  0,  0,467,  0,  0,  0,  0,
  718,  0,469,  0,  0,  0,  0,699,463,712,465,714,720,724,472,  0,474,477,
    0,  0,  0,  0,492,739,495,  0,460,  0,312,462,475,488,508,528,549,365,
  559,565,586,620,633,642,649,655,683,727,736,481,488,742,  0,  0,  0,  0,
  498,  0,774,  0,776,  0,778,  0,771,780,  0,  0,  0,  0,  0,  0,  0,  0,
    0,  0,  0,  0,  0,508,510,  0,503,790,794,505,797,  0,  0,  0,  0,  0,
    0,  0,  0,  0,512,  0,806,809,811,516,  0,  0,  0,  0,  0,  0,  0,  0,
    0,  0,  0,  0,  0,519,  0,822,826,829,522,  0,  0,  0,  0,  0,  0,  0,
    0,525,527,  0,529,531,  0,841,  0,844,847,  0,850,  0,  0,539,  0,861,
  541,  0,544,548,  0,866,550,  0,869,  0,533,  0,536,863,872,  0,558,  0,
    0,556,880,  0,561,  0,886,  0,888,  0,  0,  0,  0,890,  0,  0,  0,  0,
    0,  0,  0,  0,899,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,
  897,901,907,910,913,  0,  0,  0,  0,  0,  0,  0,925,927,568,570,  0,  0,
    0,935,  0,  0,  0,937,  0,572,574,  0,579,581,  0,564,930,941,943,576,
  946,584,587,593,  0,600,  0,  0,595,959,604,610,  0,615,  0,967,  0,612,
  969,  0,  0,  0,  0,  0,974,618,  0,633,637,  0,620,623,626,628,981,  0,
    0,  0,  0,639,  0,  0,649,  0,  0,  0,  0,  0,  0,  0,  0,  0,651,  0,
    0,  0,1008,  0,990,993,642,645,647,995,  0,998,1001,1005,1010,  0,  0,
    0,  0,  0,  0,  0,  0,1025,  0,  0,  0,  0,1034,  0,  0,  0,  0,1036,
    0,  0,655,  0,  0,  0,  0,1047,  0,657,  0,  0,  0,  0,1028,1041,653,
  1043,1049,1053,660,  0,662,665,  0,  0,  0,  0,680,1067,683,  0,785,  0,
  501,787,800,813,833,853,874,552,883,894,915,949,962,971,978,984,1012,1056,
  1064,669,676,1070,  0,  0,  0,  0,  0,  0,  0,1102,  0,700,  0,  0,  0,
    0,  0,  0,1110,  0,  0,  0,686,689,693,1107,704,708,1113,710,713,717,
    0,721,725,  0,1129,732,735,739,  0,748,751,755,  0,  0,  0,  0,759,  0,
  1144,  0,1146,  0,1148,  0,  0,  0,  0,746,1137,1141,1150,  0,  0,  0,
    0,  0,  0,  0,768,772,  0,779,  0,  0,1169,  0,  0,  0,784,  0,789,793,
    0,1178,795,  0,1181,  0,781,1175,787,1184,  0,804,  0,1191,  0,1193,
    0,1195,  0,807,809,  0,1199,811,  0,817,822,827,  0,833,836,  0,844,
  847,852,  0,857,859,863,  0,865,868,  0,762,1160,1163,764,1166,775,1171,
  1186,797,801,1197,1202,1205,1209,841,1212,1216,1220,872,879,883,  0,885,
    0,1245,  0,1247,  0,1249,  0,1251,  0,888,890,  0,  0,  0,  0,892,1258,
  894,  0,  0,  0,  0,1265,896,902,  0,  0,  0,  0,  0,1272,  0,  0,  0,
    0,  0,  0,  0,  0,1278,1280,1283,906,908,  0,910,  0,  0,  0,  0,  0,
    0,  0,  0,  0,  0,  0,  0,  0,  0,  0,1301,1303,1305,914,  0,  0,  0,
    0,  0,  0,  0,  0,1312,1314,1316,916,  0,  0,  0,  0,  0,1324,1326,  0,
    0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,1331,919,  0,1333,
  1336,1339,1341,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,
    0,  0,  0,  0,  0,  0,  0,  0,  0,1371,  0,1352,921,  0,1354,1357,1360,
  1362,923,926,1365,1368,928,931,933,1374,935,  0,946,950,  0,957,  0,  0,
  1396,  0,  0,  0,962,  0,965,969,  0,1405,971,  0,1408,  0,959,1402,1411,
    0,980,982,  0,1417,984,  0,990,995,1000,  0,1006,1009,  0,1017,1020,
  1025,  0,1030,1034,  0,1036,1039,  0,938,940,942,1393,953,1398,1413,973,
  977,1420,1423,1427,1014,1430,1434,1437,1043,1050,  0,  0,  0,  0,  0,1459,
    0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,
    0,  0,  0,  0,  0,  0,  0,1486,  0,1465,1054,  0,1467,1470,1473,1475,
  1056,1059,1478,1481,1061,1064,1484,  0,1489,1066,  0,1069,  0,  0,  0,
    0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,
    0,  0,  0,  0,1532,  0,1511,1071,  0,1513,1516,1519,1521,1524,1527,1073,
  1076,1530,  0,1535,1078,  0,  0,  0,  0,1097,  0,  0,  0,  0,  0,  0,1559,
    0,  0,  0,1081,1084,1553,1086,1090,1556,1101,1105,1562,1107,1110,1114,
  1118,  0,1134,  0,  0,  0,  0,  0,  0,1584,  0,  0,  0,1120,1123,1127,
  1581,1138,1142,1587,1144,1148,  0,  0,  0,  0,1168,  0,  0,  0,  0,  0,
    0,1608,  0,  0,  0,1152,1155,1602,1157,1161,1605,1172,1176,1611,1178,
  1182,1186,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,
    0,  0,  0,  0,  0,1645,  0,1629,1188,  0,1631,1634,1636,1190,1193,1639,
  1642,1195,1198,1200,1648,1202,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,
    0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,1685,  0,1666,  0,
  1668,1671,1674,1676,1205,1208,1679,1682,1210,1212,1688,1214,  0,  0,  0,
    0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,1705,  0,1707,
  1710,1712,1714,1217,1220,1716,1719,1222,1224,1226,  0,  0,  0,  0,  0,
    0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,1736,  0,1738,1741,
  1743,1745,1229,1747,1750,1231,1233,1235,  0,  0,  0,  0,  0,  0,  0,  0,
    0,  0,  0,  0,  0,  0,  0,  0,  0,  0,1766,  0,1768,1771,1773,1775,1238,
  1777,1780,1240,1242,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,
    0,  0,  0,1244,1795,1798,1800,1802,1246,1804,1807,1248,  0,  0,  0,  0,
    0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,
    0,  0,  0,  0,1842,  0,1820,1822,  0,1825,1828,1831,1833,1250,1253,1836,
  1839,1255,1258,1260,1845,1262,1265,  0,1275,1279,  0,1286,  0,  0,1868,
    0,  0,  0,1291,  0,1296,1300,  0,1877,1302,  0,1880,  0,1288,1874,1294,
  1883,  0,1311,1313,  0,1890,1315,  0,1321,1326,1331,  0,1337,1340,  0,
  1348,1351,1356,  0,1361,1363,1367,  0,1369,1372,  0,1267,1269,1271,1865,
  1282,1870,1885,1304,1308,1893,1896,1900,1345,1903,1907,1911,1376,1383,
    0,1387,  0,  0,  0,  0,  0,  0,  0,  0,1390,1935,1939,1392,1395,  0,
    0,  0,  0,  0,  0,  0,  0,  0,  0,1948,1951,1953,  0,  0,  0,  0,  0,
    0,  0,  0,  0,  0,  0,  0,  0,1398,  0,1962,1966,1969,  0,  0,  0,  0,
    0,  0,  0,  0,1401,1403,  0,1980,1983,1986,  0,1405,  0,1410,  0,  0,
  1995,  0,  0,  0,  0,2000,  0,  0,  0,  0,  0,  0,  0,  0,2007,  0,  0,
    0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,2005,2009,2015,2018,2021,  0,
    0,  0,  0,  0,  0,  0,2033,2035,1417,1419,  0,  0,  0,2043,  0,  0,  0,
  2045,  0,1424,1426,  0,1413,2038,2049,1421,2051,1429,1432,  0,1434,1436,
    0,  0,  0,  0,2065,1441,  0,1443,1446,1448,  0,  0,  0,  0,1451,  0,
    0,1461,  0,  0,  0,  0,  0,  0,  0,  0,  0,1463,  0,  0,  0,2093,  0,
  2075,2078,1454,1457,1459,2080,  0,2083,2086,2090,2095,  0,  0,  0,  0,
    0,  0,  0,  0,2110,  0,  0,  0,  0,2119,  0,  0,  0,  0,2121,  0,  0,
  1467,  0,  0,  0,  0,2132,  0,  0,  0,  0,  0,2113,2126,1465,2128,2134,
  2137,  0,  0,  0,  0,1469,2147,1472,  0,1942,1955,1973,1989,1408,1998,
  2003,2023,2054,2062,1438,2068,2071,2097,2140,2150,  0,  0,  0,  0,  0,
    0,2171,  0,  0,  0,  0,  0,  0,2178,  0,  0,  0,  0,  0,  0,2187,  0,
    0,  0,1475,  0,1484,  0,1487,  0,1493,  0,  0,1497,  0,  0,  0,  0,  0,
  2205,  0,  0,  0,1477,1480,  0,  0,2196,2198,  0,2200,2203,1500,2208,  0,
  1502,1505,1509,  0,  0,  0,  0,  0,  0,2231,1513,  0
};

static const unsigned short ag_key_index[] = {
   16,383,427,446,455,446,746,  0,782,746,1074,427,1099,1099,  0,1105,1105,
  1099,1116,1116,1099,1132,  0,1099,1099,  0,1105,1105,1099,1116,1116,1099,
  1132,  0,1152,1223,1253,1255,1261,  0,1255,  0,  0,746,1268,1275,1275,
  1275,1275,1275,1275,1275,1275,1275,1275,1275,1275,1275,1275,1285,1291,
  1307,1116,1116,1116,1116,1116,1116,1116,1116,1116,1116,1116,1116,1116,
  1116,1116,1116,1275,1275,1275,1275,1275,1275,1275,1275,1275,1275,1275,
  1275,1275,1275,1275,1275,1275,1275,1275,1275,1275,1275,1275,1275,1275,
  1275,1275,1275,1275,  0,1285,1291,1307,1116,1116,1116,1116,1116,1116,1116,
  1116,1116,1116,1116,1116,1116,1116,1116,1116,1116,1116,1116,1116,1116,
  1116,1116,1116,1116,1116,1116,1116,1116,1116,1116,1116,1116,1116,1116,
  1116,1116,1116,1116,1116,1116,1116,1116,1116,1116,1116,1116,1116,1318,
  1318,1318,1318,1328,1328,1307,1307,1307,1285,1285,1291,1291,1291,1291,
  1291,1291,1291,1291,1291,1291,1291,  0,  0,  0,  0,1116,1116,1116,1116,
  1116,1116,1116,1116,1116,1116,1116,1116,1116,1116,1116,1116,1116,1116,
  1116,1116,1116,1116,1116,1116,1116,1116,1116,1116,1116,1116,1116,1116,
  1116,1116,1116,1116,1116,1116,1116,1116,1116,1116,1116,1116,1116,1116,
  1116,1116,1116,1116,1116,1116,1318,1318,1318,1318,1328,1328,1307,1307,
  1307,1285,1285,1291,1291,1291,1291,1291,1291,1291,1291,1291,1291,1291,
  1344,1275,1376,1105,1275,1116,1440,1105,  0,  0,1105,  0,1462,1105,1105,
  1105,1105,  0,  0,  0,1376,1491,  0,1509,1376,  0,1376,1376,1376,1537,
    0,1105,  0,  0,  0,  0,  0,  0,  0,  0,  0,1376,1376,1565,  0,  0,  0,
    0,  0,  0,  0,  0,  0,  0,  0,1590,1614,1590,1614,1116,1590,1590,1590,
  1590,1376,1650,1690,1690,1722,1753,1783,1810,1116,1105,1105,1105,1105,
  1105,1105,1105,1099,1099,1105,1105,1105,1105,  0,1105,1105,1105,  0,  0,
  1261,1565,1261,1565,1255,1261,1261,1261,1261,1261,1261,1261,1261,1261,
  1261,1261,1261,1261,1261,1261,1261,1261,1261,1261,1614,1565,1565,1565,
  1614,1847,1261,1565,1614,1261,1255,1261,1261,1152,  0,  0,  0,1914,1105,
  1268,1285,1291,1307,  0,  0,  0,  0,1116,1116,1116,1116,1116,1116,1116,
  1116,1116,1116,1116,1116,1116,1116,1116,1116,1116,1116,1116,1116,1116,
  1116,1116,1116,1116,1116,1116,1116,1116,1116,1116,1116,1116,1116,1116,
  1116,1116,1116,1116,1116,1116,1116,1116,1116,1116,1116,1116,1116,1116,
  1116,1116,1116,1318,1318,1318,1318,1328,1328,1307,1307,1307,1285,1285,
  1291,1291,1291,1291,1291,1291,1291,1291,1291,1291,1291,1116,1933,2154,
  2154,1099,1099,  0,  0,1105,1105,  0,  0,1105,1105,2174,2174,  0,  0,1116,
  1116,1116,1116,1116,1116,1099,1099,1099,1099,1099,1099,1099,1099,1099,
  1099,1116,1116,1116,1116,1116,1116,1116,1116,1116,1116,1105,1116,1116,
  1116,1116,1099,1268,1268,1105,  0,  0,1105,  0,1105,1105,  0,  0,1509,
  1376,1376,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,1116,  0,1590,1590,
  1590,1590,1590,1590,1590,1116,1116,1116,1116,1116,1116,1116,1116,1116,
  1116,1116,1116,1116,1116,1116,1116,1116,1116,1116,1116,1116,1116,1116,
  1116,1116,1116,1116,1116,1116,1116,1116,1116,1105,1105,1105,1105,1105,
  1099,1105,1105,  0,1105,1105,  0,1105,1105,1105,1116,1116,1116,1116,1105,
  1105,1105,1275,1275,1275,2181,1275,1116,1116,1116,1275,1275,2185,1509,
  1509,1462,2185,  0,2190,2154,1509,  0,1116,1462,1462,1462,1462,1462,  0,
    0,2194,  0,  0,  0,  0,1099,1116,1116,1116,1116,1116,1116,1116,1116,
  1116,1116,  0,1590,1590,1590,1590,1590,1116,1116,1116,1116,1116,1116,1116,
  1116,1116,1116,1116,1116,1105,1105,1105,1105,1105,1105,1105,1116,1116,
  1116,2185,1509,2211,1116,2229,1275,1509,1116,1462,1099,2234,2234,1105,
    0,  0,1099,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,1650,1650,1690,
  1690,1690,1690,1690,1690,1722,1722,1753,1753,1105,1105,1105,1291,1509,
  1116,1116,1099,  0,  0,  0,1291,  0
};

static const unsigned char ag_key_ends[] = {
83,83,0, 65,84,65,0, 69,88,84,0, 68,69,0, 92,39,0, 72,76,41,0, 
83,83,0, 65,84,65,0, 69,88,84,0, 73,0, 72,76,0, 71,0, 82,0, 
79,67,75,0, 84,69,0, 76,76,0, 73,76,0, 69,71,0, 76,0, 80,0, 
71,0, 66,0, 72,79,0, 70,0, 69,0, 70,0, 82,89,0, 82,79,82,0, 
68,69,68,0, 78,0, 78,0, 76,76,0, 79,79,82,0, 88,0, 71,72,0, 
66,67,0, 69,70,0, 68,69,70,0, 68,69,0, 65,71,69,0, 65,76,76,0, 
73,0, 73,0, 75,0, 84,0, 77,80,0, 80,0, 83,72,0, 69,84,0, 
70,73,82,83,84,0, 73,0, 67,76,73,66,0, 76,69,0, 70,73,82,83,84,0, 
73,0, 77,69,0, 71,69,0, 84,0, 71,69,0, 76,0, 80,0, 71,77,65,0, 
78,84,70,0, 87,0, 76,73,67,0, 72,0, 76,76,0, 69,76,0, 84,0, 
77,0, 69,0, 76,0, 84,0, 77,0, 76,0, 82,84,0, 76,78,0, 77,0, 
88,84,0, 84,76,69,0, 78,68,69,70,0, 69,82,73,76,79,71,0, 
79,82,68,0, 72,71,0, 82,0, 72,76,0, 10,0, 83,83,0, 65,84,65,0, 
69,88,84,0, 69,70,73,78,69,0, 70,0, 69,0, 68,73,70,0, 
82,79,82,0, 69,70,0, 68,69,70,0, 67,76,85,68,69,0, 
82,65,71,77,65,0, 78,68,69,70,0, 61,0, 61,0, 73,0, 72,76,0, 
71,0, 82,0, 79,67,75,0, 84,69,0, 76,76,0, 69,71,0, 76,0, 80,0, 
71,0, 66,0, 72,79,0, 83,69,0, 70,0, 82,89,0, 85,0, 68,69,68,0, 
78,0, 78,0, 73,76,76,0, 88,0, 66,67,0, 65,76,76,0, 73,0, 73,0, 
75,0, 84,0, 77,80,0, 80,0, 83,72,0, 69,84,0, 70,73,82,83,84,0, 
73,0, 67,76,73,66,0, 85,76,69,0, 70,73,82,83,84,0, 73,0, 
77,69,0, 71,69,0, 84,0, 71,69,0, 72,76,0, 80,0, 73,78,84,70,0, 
76,73,67,0, 72,0, 76,76,0, 69,76,0, 84,0, 77,0, 69,0, 76,0, 
84,0, 77,0, 76,0, 76,78,0, 77,0, 88,84,0, 84,76,69,0, 
69,82,73,76,79,71,0, 79,82,68,0, 72,71,0, 72,76,0, 68,69,0, 
61,0, 73,0, 72,76,0, 71,0, 82,0, 79,67,75,0, 84,69,0, 76,76,0, 
69,71,0, 76,0, 80,0, 71,0, 66,0, 72,79,0, 83,69,0, 70,0, 
82,89,0, 68,69,68,0, 78,0, 78,0, 73,76,76,0, 88,0, 66,67,0, 
68,69,0, 65,76,76,0, 73,0, 73,0, 75,0, 84,0, 77,80,0, 80,0, 
83,72,0, 69,84,0, 70,73,82,83,84,0, 73,0, 67,76,73,66,0, 
85,76,69,0, 70,73,82,83,84,0, 73,0, 77,69,0, 71,69,0, 84,0, 
71,69,0, 72,76,0, 80,0, 73,78,84,70,0, 76,73,67,0, 72,0, 
76,76,0, 69,76,0, 84,0, 77,0, 69,0, 76,0, 77,0, 76,0, 
76,78,0, 77,0, 88,84,0, 84,76,69,0, 69,82,73,76,79,71,0, 
79,82,68,0, 72,71,0, 72,76,0, 92,39,0, 69,73,76,0, 
69,70,73,78,69,68,0, 79,79,82,0, 73,71,72,0, 80,0, 79,84,0, 
65,71,69,0, 81,82,84,0, 84,82,89,0, 84,69,78,68,69,68,0, 69,88,0, 
73,83,84,0, 69,82,73,76,79,71,0, 47,0, 83,83,0, 65,84,65,0, 
69,88,84,0, 68,69,0, 61,0, 83,69,71,0, 79,67,75,0, 84,69,0, 
83,69,71,0, 71,0, 72,79,0, 82,89,0, 85,0, 68,69,68,0, 78,0, 
78,0, 73,76,76,0, 69,88,0, 68,69,0, 75,0, 84,0, 
70,73,82,83,84,0, 67,76,73,66,0, 68,85,76,69,0, 70,73,82,83,84,0, 
77,69,0, 80,65,71,69,0, 82,71,0, 71,69,0, 73,78,84,70,0, 
66,76,73,67,0, 84,0, 75,76,78,0, 77,0, 88,84,0, 84,76,69,0, 
69,82,73,76,79,71,0, 79,82,68,0, 10,0, 68,69,0, 47,0, 42,0, 
47,0, 10,0, 78,80,65,71,69,0, 65,71,69,0, 67,0, 80,0, 
72,76,41,0, 80,0, 83,87,0, 42,0, 42,0, 78,68,0, 81,0, 79,68,0, 
69,0, 82,0, 79,82,0, 61,0, 61,0, 83,69,71,0, 79,67,75,0, 
84,69,0, 83,69,71,0, 71,0, 72,79,0, 82,89,0, 68,69,68,0, 78,0, 
78,0, 73,76,76,0, 69,88,0, 75,0, 84,0, 70,73,82,83,84,0, 
67,76,73,66,0, 68,85,76,69,0, 70,73,82,83,84,0, 77,69,0, 
80,65,71,69,0, 82,71,0, 71,69,0, 73,78,84,70,0, 66,76,73,67,0, 
75,76,78,0, 77,0, 88,84,0, 84,76,69,0, 69,82,73,76,79,71,0, 
79,82,68,0, 42,0, 78,68,0, 81,0, 79,68,0, 69,0, 79,82,0, 39,0, 
42,0, 79,68,0, 69,0, 79,82,0, 92,39,0, 47,0, 69,73,76,0, 
69,70,73,78,69,68,0, 79,79,82,0, 73,71,72,0, 80,0, 79,84,0, 
65,71,69,0, 81,82,84,0, 10,0, 92,39,0, 69,73,76,0, 
69,70,73,78,69,68,0, 79,79,82,0, 73,71,72,0, 80,0, 65,71,69,0, 
81,82,84,0, 92,39,0, 47,0, 69,73,76,0, 69,70,73,78,69,68,0, 
79,79,82,0, 73,71,72,0, 80,0, 65,71,69,0, 81,82,84,0, 10,0, 
42,0, 78,68,0, 81,0, 79,68,0, 69,0, 82,0, 79,82,0, 78,68,0, 
81,0, 69,0, 82,0, 79,82,0, 78,68,0, 81,0, 69,0, 82,0, 
79,82,0, 81,0, 69,0, 82,0, 79,82,0, 81,0, 69,0, 82,0, 61,0, 
81,0, 69,0, 78,68,0, 81,0, 79,68,0, 69,0, 82,0, 79,82,0, 
10,0, 61,0, 61,0, 83,69,71,0, 79,67,75,0, 84,69,0, 83,69,71,0, 
71,0, 72,79,0, 82,89,0, 85,0, 68,69,68,0, 78,0, 78,0, 
73,76,76,0, 69,88,0, 75,0, 84,0, 70,73,82,83,84,0, 
67,76,73,66,0, 68,85,76,69,0, 70,73,82,83,84,0, 77,69,0, 
80,65,71,69,0, 82,71,0, 71,69,0, 73,78,84,70,0, 66,76,73,67,0, 
84,0, 75,76,78,0, 77,0, 88,84,0, 84,76,69,0, 
69,82,73,76,79,71,0, 79,82,68,0, 92,39,0, 73,0, 72,76,0, 
72,82,0, 76,76,0, 76,0, 80,0, 85,66,0, 73,0, 66,67,0, 
65,76,76,0, 73,0, 73,0, 77,80,0, 80,0, 83,72,0, 69,84,0, 73,0, 
86,0, 73,0, 79,80,0, 84,0, 72,76,0, 80,0, 83,72,0, 76,76,0, 
69,76,0, 84,0, 77,0, 69,0, 76,0, 77,0, 76,0, 72,71,0, 
72,76,0, 10,0, 92,39,0, 72,76,41,0, 73,76,0, 70,73,78,69,68,0, 
79,79,82,0, 71,72,0, 80,0, 79,84,0, 65,71,69,0, 81,82,84,0, 
10,0, 
};
#define AG_TCV(x) (((int)(x) >= -1 && (int)(x) <= 255) ? ag_tcv[(x) + 1] : 0)

static const unsigned short ag_tcv[] = {
   38, 38,682,682,682,682,682,682,682,682,  1,425,682,682,682,682,682,682,
  682,682,682,682,682,682,682,682,682,682,682,682,682,682,682,  1,526,683,
  684,685,523,513,686,458,456,521,519,481,520,431,522,687,688,689,689,690,
  690,690,690,691,691,682,427,692,682,693,694,695,696,697,696,698,696,699,
  700,701,700,700,700,700,700,702,700,700,700,703,700,704,700,700,700,705,
  700,700,682,706,682,511,707,695,696,697,696,698,696,699,700,701,700,700,
  700,700,700,702,700,700,700,703,700,704,700,700,700,705,700,700,694,509,
  694,528,708,708,708,708,708,708,708,708,708,708,708,708,708,708,708,708,
  708,708,708,708,708,708,708,708,708,708,708,708,708,708,708,708,708,708,
  708,708,708,708,708,708,708,708,708,708,708,708,708,708,708,708,708,708,
  708,708,708,708,708,708,708,708,708,708,708,708,708,708,708,708,708,708,
  708,708,708,708,708,708,708,708,708,708,708,708,708,708,708,708,708,708,
  708,708,708,708,708,708,708,708,708,708,708,708,708,708,708,708,708,708,
  708,708,708,708,708,708,708,708,708,708,708,708,708,708,708,708,708,708,
  708,708,708,708,708
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
707,705,704,703,702,701,700,699,698,697,696,694,685,439,435,434,431,429,427,
  426,425,164,163,162,135,134,1,0,51,52,
1,0,
707,705,704,703,702,701,700,699,698,697,696,694,685,439,431,429,427,426,425,
  164,163,162,135,134,63,1,0,11,34,35,36,37,51,52,53,64,65,133,430,443,
455,454,453,452,451,450,449,448,447,441,434,1,0,51,52,
707,705,704,703,702,701,700,699,698,697,696,694,691,690,689,688,687,685,464,
  431,429,427,426,425,161,135,1,0,138,
455,454,453,452,451,450,449,448,447,441,434,0,59,66,67,73,75,77,78,79,80,81,
  82,83,
707,705,704,703,702,701,700,699,698,697,696,694,685,681,680,679,678,677,676,
  675,674,673,672,671,670,669,668,667,666,665,664,663,662,661,660,659,658,
  657,656,655,654,653,652,651,650,649,648,647,646,645,644,643,642,641,640,
  639,638,637,636,635,634,633,632,631,630,629,628,627,626,625,624,623,622,
  621,620,619,618,617,616,615,614,613,612,611,609,608,607,606,605,604,603,
  602,601,600,599,598,597,595,594,593,592,591,590,589,588,587,586,585,584,
  583,582,581,580,579,578,577,576,575,574,573,572,571,570,569,568,567,566,
  565,564,563,562,561,560,559,558,557,556,555,554,553,552,551,550,549,548,
  547,492,491,490,489,488,487,485,484,483,482,480,479,478,477,476,475,474,
  473,472,471,470,469,468,467,466,465,464,463,462,461,460,452,451,448,446,
  445,444,442,440,433,432,431,429,427,426,425,135,134,1,0,51,52,
425,0,45,
435,434,431,429,427,426,425,0,39,40,41,45,46,47,49,54,55,
707,705,704,703,702,701,700,699,698,697,696,694,685,681,680,679,678,677,676,
  675,674,673,672,671,670,669,668,667,666,665,664,663,662,661,660,659,658,
  657,656,655,654,653,652,651,650,649,648,647,646,645,644,643,642,641,640,
  639,638,637,636,635,634,633,632,631,630,629,628,627,626,625,624,623,622,
  621,620,619,618,617,616,615,614,613,612,611,609,608,607,606,605,604,603,
  602,601,600,599,598,597,595,594,593,592,591,590,589,588,587,586,585,584,
  583,582,581,580,579,578,577,576,575,574,573,572,571,570,569,568,567,566,
  565,564,563,562,561,560,559,558,557,556,555,554,553,552,551,550,549,548,
  547,492,491,490,489,488,487,485,484,483,482,480,479,478,477,476,475,474,
  473,472,471,470,469,468,467,466,465,464,463,462,461,460,452,451,448,446,
  445,444,442,440,433,432,431,429,427,426,425,135,134,1,0,51,52,
707,705,704,703,702,701,700,699,698,697,696,694,685,681,680,679,678,677,676,
  675,674,673,672,671,670,669,668,667,666,665,664,663,662,661,660,659,658,
  657,656,655,654,653,652,651,650,649,648,647,646,645,644,643,642,641,640,
  639,638,637,636,635,634,633,632,631,630,629,628,627,626,625,624,623,622,
  621,620,619,618,617,616,615,614,613,612,611,609,608,607,606,605,604,603,
  602,601,600,599,598,597,595,594,593,592,591,590,589,588,587,586,585,584,
  583,582,581,580,579,578,577,576,575,574,573,572,571,570,569,568,567,566,
  565,564,563,562,561,560,559,558,557,556,555,554,553,552,551,550,549,548,
  547,492,491,490,489,488,487,485,484,483,482,480,479,478,477,476,475,474,
  473,472,471,470,469,468,467,466,465,464,463,462,461,460,452,451,448,446,
  445,444,442,440,435,434,431,429,427,426,425,135,134,1,0,21,30,31,32,33,
  54,55,62,75,79,80,89,94,133,245,258,259,260,261,262,263,264,265,266,267,
  269,270,271,272,273,274,275,276,277,278,279,280,281,282,283,284,285,286,
  287,288,289,290,291,292,293,294,295,296,297,298,299,300,301,302,303,304,
  305,306,307,308,309,310,311,312,313,314,315,316,317,318,319,320,321,322,
  323,324,325,326,327,328,329,330,331,332,333,334,335,336,337,338,339,340,
  341,342,343,344,345,347,348,349,350,351,352,353,354,355,356,357,358,359,
  360,361,362,363,364,365,366,367,368,369,370,371,372,373,374,375,376,377,
  378,379,380,381,382,383,384,385,386,387,388,389,390,391,392,393,394,395,
  396,397,398,399,400,401,402,403,404,405,406,407,408,409,410,411,412,413,
  443,
707,705,704,703,702,701,700,699,698,697,696,694,685,439,431,429,427,426,425,
  164,163,162,135,134,63,38,1,0,11,35,36,51,52,53,64,65,133,430,443,
707,705,704,703,702,701,700,699,698,697,696,694,685,135,134,1,0,51,52,
707,705,704,703,702,701,700,699,698,697,696,694,685,135,134,1,0,51,52,
683,1,0,51,52,
429,427,426,425,1,0,51,52,
429,427,426,425,1,0,51,52,
707,705,704,703,702,701,700,699,698,697,696,694,685,135,134,1,0,51,52,
707,705,704,703,702,701,700,699,698,697,696,695,694,691,690,689,688,687,686,
  685,539,537,536,535,534,533,532,531,530,529,528,527,526,523,520,519,492,
  458,431,232,135,134,1,0,51,52,
707,705,704,703,702,701,700,699,698,697,696,695,694,691,690,689,688,687,686,
  685,539,537,536,535,534,533,532,531,530,529,528,527,526,523,520,519,492,
  458,431,232,135,134,1,0,51,52,
707,705,704,703,702,701,700,699,698,697,696,694,685,135,134,1,0,51,52,
446,445,444,442,440,1,0,51,52,
707,706,705,704,703,702,701,700,699,698,697,696,692,691,690,689,688,687,683,
  522,431,1,0,51,52,
707,705,704,703,702,701,700,699,698,697,696,694,685,135,134,0,21,84,133,443,
707,705,704,703,702,701,700,699,698,697,696,694,685,135,134,0,21,133,443,
683,0,10,12,436,
429,427,426,425,1,0,51,52,
429,427,426,425,1,0,51,52,
707,705,704,703,702,701,700,699,698,697,696,694,685,135,134,0,21,133,443,
707,705,704,703,702,701,700,699,698,697,696,695,694,691,690,689,688,687,686,
  685,539,537,536,535,534,533,532,531,530,529,528,527,526,523,520,519,492,
  458,431,232,135,134,0,2,6,7,8,9,15,17,21,23,24,25,26,27,28,29,57,76,86,
  89,131,133,166,197,199,204,205,206,207,208,209,210,211,212,213,214,215,
  216,217,218,219,220,443,493,540,
707,705,704,703,702,701,700,699,698,697,696,695,694,691,690,689,688,687,686,
  685,539,537,536,535,534,533,532,531,530,529,528,527,526,523,520,519,492,
  458,431,232,135,134,0,2,6,7,8,9,15,17,21,23,24,25,26,27,28,29,57,76,86,
  89,131,133,166,197,199,204,205,206,207,208,209,210,211,212,213,214,215,
  216,217,218,219,220,443,493,540,
707,705,704,703,702,701,700,699,698,697,696,694,685,135,134,0,21,133,443,
446,445,444,442,440,0,68,69,70,71,72,
692,683,0,10,12,13,14,436,437,
708,707,706,705,704,703,702,701,700,699,698,697,696,695,694,693,692,691,690,
  689,688,687,686,685,684,683,682,528,526,523,522,521,520,519,513,511,509,
  481,458,456,439,435,434,431,429,428,427,426,425,164,163,162,135,134,38,
  1,0,51,52,
708,707,706,705,704,703,702,701,700,699,698,697,696,695,694,693,692,691,690,
  689,688,687,686,685,684,683,682,528,526,523,522,521,520,519,513,511,509,
  492,491,490,489,488,487,485,484,483,482,481,480,479,478,477,476,475,474,
  473,472,471,470,469,468,467,466,465,464,463,462,461,460,459,458,456,446,
  445,444,442,440,435,434,433,432,431,429,428,427,426,425,1,0,51,52,
435,434,0,59,60,61,
708,707,706,705,704,703,702,701,700,699,698,697,696,695,694,693,692,691,690,
  689,688,687,686,685,684,683,682,528,526,523,522,521,520,519,513,511,509,
  481,458,456,431,429,428,427,425,1,0,51,52,
708,707,706,705,704,703,702,701,700,699,698,697,696,695,694,693,692,691,690,
  689,688,687,686,685,684,683,682,528,526,523,522,521,520,519,513,511,509,
  481,459,458,456,431,429,428,427,426,425,1,0,51,52,
708,707,706,705,704,703,702,701,700,699,698,697,696,695,694,693,692,691,690,
  689,688,687,686,685,684,683,682,528,526,523,522,521,520,519,513,511,509,
  481,458,456,431,427,425,1,0,51,52,
708,707,706,705,704,703,702,701,700,699,698,697,696,695,694,693,692,691,690,
  689,688,687,686,685,684,683,682,528,526,523,522,521,520,519,513,511,509,
  481,458,456,431,429,428,427,425,1,0,40,45,46,47,48,49,54,86,88,122,140,
  141,143,145,146,147,148,149,150,154,157,158,186,188,190,196,197,198,200,
  201,204,206,221,225,226,230,414,415,416,417,418,419,420,421,422,423,424,
708,707,706,705,704,703,702,701,700,699,698,697,696,695,694,693,692,691,690,
  689,688,687,686,685,684,683,682,528,526,523,522,521,520,519,513,511,509,
  481,458,456,431,427,425,1,0,43,44,46,54,86,88,122,140,141,143,145,146,
  147,148,149,150,154,157,158,186,188,190,196,197,198,200,201,204,206,221,
  225,226,230,414,415,416,417,418,419,420,421,422,423,424,
708,707,706,705,704,703,702,701,700,699,698,697,696,695,694,693,692,691,690,
  689,688,687,686,685,684,683,682,528,526,523,522,521,520,519,513,511,509,
  481,458,456,431,427,425,1,0,43,44,46,54,86,88,122,140,141,143,145,146,
  147,148,149,150,154,157,158,186,188,190,196,197,198,200,201,204,206,221,
  225,226,230,414,415,416,417,418,419,420,421,422,423,424,
707,705,704,703,702,701,700,699,698,697,696,694,685,681,680,679,678,677,676,
  675,674,673,672,671,670,669,668,667,666,665,664,663,662,661,660,659,658,
  657,656,655,654,653,652,651,650,649,648,647,646,645,644,643,642,641,640,
  639,638,637,636,635,634,633,632,631,630,629,628,627,626,625,624,623,622,
  621,620,619,618,617,616,615,614,613,612,611,609,608,607,606,605,604,603,
  602,601,600,599,598,597,595,594,593,592,591,590,589,588,587,586,585,584,
  583,582,581,580,579,578,577,576,575,574,573,572,571,570,569,568,567,566,
  565,564,563,562,561,560,559,558,557,556,555,554,553,552,551,550,549,548,
  547,492,491,490,489,488,487,485,484,483,482,480,479,478,477,476,475,474,
  473,472,471,470,469,468,467,466,465,464,463,462,461,460,452,451,448,446,
  445,444,442,440,433,432,431,429,427,426,425,135,134,0,21,30,31,32,33,39,
  40,41,45,46,47,49,54,55,62,75,79,80,89,94,133,245,258,259,260,261,262,
  263,264,265,266,267,269,270,271,272,273,274,275,276,277,278,279,280,281,
  282,283,284,285,286,287,288,289,290,291,292,293,294,295,296,297,298,299,
  300,301,302,303,304,305,306,307,308,309,310,311,312,313,314,315,316,317,
  318,319,320,321,322,323,324,325,326,327,328,329,330,331,332,333,334,335,
  336,337,338,339,340,341,342,343,344,345,347,348,349,350,351,352,353,354,
  355,356,357,358,359,360,361,362,363,364,365,366,367,368,369,370,371,372,
  373,374,375,376,377,378,379,380,381,382,383,384,385,386,387,388,389,390,
  391,392,393,394,395,396,397,398,399,400,401,402,403,404,405,406,407,408,
  409,410,411,412,413,443,
495,492,429,427,426,425,1,0,51,52,
546,429,427,426,425,1,0,51,52,
546,429,427,426,425,1,0,51,52,
546,429,427,426,425,1,0,51,52,
546,429,427,426,425,1,0,51,52,
546,429,427,426,425,1,0,51,52,
546,429,427,426,425,1,0,51,52,
546,429,427,426,425,1,0,51,52,
546,429,427,426,425,1,0,51,52,
546,429,427,426,425,1,0,51,52,
546,429,427,426,425,1,0,51,52,
546,429,427,426,425,1,0,51,52,
546,429,427,426,425,1,0,51,52,
546,429,427,426,425,1,0,51,52,
546,429,427,426,425,1,0,51,52,
670,545,544,543,249,247,239,237,235,1,0,51,52,
542,242,241,240,239,238,237,236,235,1,0,51,52,
670,545,249,247,239,237,235,1,0,51,52,
707,705,704,703,702,701,700,699,698,697,696,695,694,691,690,689,688,687,686,
  685,539,537,536,535,534,533,532,531,530,529,528,527,526,523,520,519,492,
  458,431,232,135,134,1,0,51,52,
707,705,704,703,702,701,700,699,698,697,696,695,694,691,690,689,688,687,686,
  685,539,537,536,535,534,533,532,531,530,529,528,527,526,523,520,519,492,
  458,431,232,135,134,1,0,51,52,
707,705,704,703,702,701,700,699,698,697,696,695,694,691,690,689,688,687,686,
  685,539,537,536,535,534,533,532,531,530,529,528,527,526,523,520,519,492,
  458,431,232,135,134,1,0,51,52,
707,705,704,703,702,701,700,699,698,697,696,695,694,691,690,689,688,687,686,
  685,539,537,536,535,534,533,532,531,530,529,528,527,526,523,520,519,492,
  458,431,232,135,134,1,0,51,52,
707,705,704,703,702,701,700,699,698,697,696,695,694,691,690,689,688,687,686,
  685,539,537,536,535,534,533,532,531,530,529,528,527,526,523,520,519,492,
  458,431,232,135,134,1,0,51,52,
707,705,704,703,702,701,700,699,698,697,696,695,694,691,690,689,688,687,686,
  685,539,537,536,535,534,533,532,531,530,529,528,527,526,523,520,519,492,
  458,431,232,135,134,1,0,51,52,
707,705,704,703,702,701,700,699,698,697,696,695,694,691,690,689,688,687,686,
  685,539,537,536,535,534,533,532,531,530,529,528,527,526,523,520,519,492,
  458,431,232,135,134,1,0,51,52,
707,705,704,703,702,701,700,699,698,697,696,695,694,691,690,689,688,687,686,
  685,539,537,536,535,534,533,532,531,530,529,528,527,526,523,520,519,492,
  458,431,232,135,134,1,0,51,52,
707,705,704,703,702,701,700,699,698,697,696,695,694,691,690,689,688,687,686,
  685,539,537,536,535,534,533,532,531,530,529,528,527,526,523,520,519,492,
  458,431,232,135,134,1,0,51,52,
707,705,704,703,702,701,700,699,698,697,696,695,694,691,690,689,688,687,686,
  685,539,537,536,535,534,533,532,531,530,529,528,527,526,523,520,519,492,
  458,431,232,135,134,1,0,51,52,
707,705,704,703,702,701,700,699,698,697,696,695,694,691,690,689,688,687,686,
  685,539,537,536,535,534,533,532,531,530,529,528,527,526,523,520,519,492,
  458,431,232,135,134,1,0,51,52,
707,705,704,703,702,701,700,699,698,697,696,695,694,691,690,689,688,687,686,
  685,539,537,536,535,534,533,532,531,530,529,528,527,526,523,520,519,492,
  458,431,232,135,134,1,0,51,52,
707,705,704,703,702,701,700,699,698,697,696,695,694,691,690,689,688,687,686,
  685,539,537,536,535,534,533,532,531,530,529,528,527,526,523,520,519,492,
  458,431,232,135,134,1,0,51,52,
707,705,704,703,702,701,700,699,698,697,696,695,694,691,690,689,688,687,686,
  685,539,537,536,535,534,533,532,531,530,529,528,527,526,523,520,519,492,
  458,431,232,135,134,1,0,51,52,
707,705,704,703,702,701,700,699,698,697,696,695,694,691,690,689,688,687,686,
  685,539,537,536,535,534,533,532,531,530,529,528,527,526,523,520,519,492,
  458,431,232,135,134,1,0,51,52,
707,705,704,703,702,701,700,699,698,697,696,695,694,691,690,689,688,687,686,
  685,539,537,536,535,534,533,532,531,530,529,528,527,526,523,520,519,492,
  458,431,232,135,134,1,0,51,52,
546,429,427,426,425,1,0,51,52,
546,429,427,426,425,1,0,51,52,
546,429,427,426,425,1,0,51,52,
546,429,427,426,425,1,0,51,52,
546,429,427,426,425,1,0,51,52,
546,429,427,426,425,1,0,51,52,
546,429,427,426,425,1,0,51,52,
546,429,427,426,425,1,0,51,52,
546,429,427,426,425,1,0,51,52,
546,429,427,426,425,1,0,51,52,
546,429,427,426,425,1,0,51,52,
546,429,427,426,425,1,0,51,52,
546,429,427,426,425,1,0,51,52,
546,429,427,426,425,1,0,51,52,
546,429,427,426,425,1,0,51,52,
546,429,427,426,425,1,0,51,52,
546,429,427,426,425,1,0,51,52,
546,429,427,426,425,1,0,51,52,
546,429,427,426,425,1,0,51,52,
546,429,427,426,425,1,0,51,52,
546,429,427,426,425,1,0,51,52,
546,429,427,426,425,1,0,51,52,
546,429,427,426,425,1,0,51,52,
546,429,427,426,425,1,0,51,52,
546,429,427,426,425,1,0,51,52,
546,429,427,426,425,1,0,51,52,
546,429,427,426,425,1,0,51,52,
546,429,427,426,425,1,0,51,52,
546,429,427,426,425,1,0,51,52,
690,689,688,687,1,0,51,52,
670,545,544,543,249,247,239,237,235,1,0,51,52,
542,242,241,240,239,238,237,236,235,1,0,51,52,
670,545,249,247,239,237,235,1,0,51,52,
707,705,704,703,702,701,700,699,698,697,696,695,694,691,690,689,688,687,686,
  685,539,537,536,535,534,533,532,531,530,529,528,527,526,523,520,519,492,
  458,431,232,135,134,1,0,51,52,
707,705,704,703,702,701,700,699,698,697,696,695,694,691,690,689,688,687,686,
  685,539,537,536,535,534,533,532,531,530,529,528,527,526,523,520,519,492,
  458,431,232,135,134,1,0,51,52,
707,705,704,703,702,701,700,699,698,697,696,695,694,691,690,689,688,687,686,
  685,539,537,536,535,534,533,532,531,530,529,528,527,526,523,520,519,492,
  458,431,232,135,134,1,0,51,52,
707,705,704,703,702,701,700,699,698,697,696,695,694,691,690,689,688,687,686,
  685,539,537,536,535,534,533,532,531,530,529,528,527,526,523,520,519,492,
  458,431,232,135,134,1,0,51,52,
707,705,704,703,702,701,700,699,698,697,696,695,694,691,690,689,688,687,686,
  685,539,537,536,535,534,533,532,531,530,529,528,527,526,523,520,519,492,
  458,431,232,135,134,1,0,51,52,
707,705,704,703,702,701,700,699,698,697,696,695,694,691,690,689,688,687,686,
  685,539,537,536,535,534,533,532,531,530,529,528,527,526,523,520,519,492,
  458,431,232,135,134,1,0,51,52,
707,705,704,703,702,701,700,699,698,697,696,695,694,691,690,689,688,687,686,
  685,539,537,536,535,534,533,532,531,530,529,528,527,526,523,520,519,492,
  458,431,232,135,134,1,0,51,52,
707,705,704,703,702,701,700,699,698,697,696,695,694,691,690,689,688,687,686,
  685,539,537,536,535,534,533,532,531,530,529,528,527,526,523,520,519,492,
  458,431,232,135,134,1,0,51,52,
707,705,704,703,702,701,700,699,698,697,696,695,694,691,690,689,688,687,686,
  685,539,537,536,535,534,533,532,531,530,529,528,527,526,523,520,519,492,
  458,431,232,135,134,1,0,51,52,
707,705,704,703,702,701,700,699,698,697,696,695,694,691,690,689,688,687,686,
  685,539,537,536,535,534,533,532,531,530,529,528,527,526,523,520,519,492,
  458,431,232,135,134,1,0,51,52,
707,705,704,703,702,701,700,699,698,697,696,695,694,691,690,689,688,687,686,
  685,539,537,536,535,534,533,532,531,530,529,528,527,526,523,520,519,492,
  458,431,232,135,134,1,0,51,52,
707,705,704,703,702,701,700,699,698,697,696,695,694,691,690,689,688,687,686,
  685,539,537,536,535,534,533,532,531,530,529,528,527,526,523,520,519,492,
  458,431,232,135,134,1,0,51,52,
707,705,704,703,702,701,700,699,698,697,696,695,694,691,690,689,688,687,686,
  685,539,537,536,535,534,533,532,531,530,529,528,527,526,523,520,519,492,
  458,431,232,135,134,1,0,51,52,
707,705,704,703,702,701,700,699,698,697,696,695,694,691,690,689,688,687,686,
  685,539,537,536,535,534,533,532,531,530,529,528,527,526,523,520,519,492,
  458,431,232,135,134,1,0,51,52,
707,705,704,703,702,701,700,699,698,697,696,695,694,691,690,689,688,687,686,
  685,539,537,536,535,534,533,532,531,530,529,528,527,526,523,520,519,492,
  458,431,232,135,134,1,0,51,52,
707,705,704,703,702,701,700,699,698,697,696,695,694,691,690,689,688,687,686,
  685,539,537,536,535,534,533,532,531,530,529,528,527,526,523,520,519,492,
  458,431,232,135,134,1,0,51,52,
707,705,704,703,702,701,700,699,698,697,696,695,694,691,690,689,688,687,686,
  685,539,537,536,535,534,533,532,531,530,529,528,527,526,523,520,519,492,
  458,431,232,135,134,1,0,51,52,
707,705,704,703,702,701,700,699,698,697,696,695,694,691,690,689,688,687,686,
  685,539,537,536,535,534,533,532,531,530,529,528,527,526,523,520,519,492,
  458,431,232,135,134,1,0,51,52,
707,705,704,703,702,701,700,699,698,697,696,695,694,691,690,689,688,687,686,
  685,539,537,536,535,534,533,532,531,530,529,528,527,526,523,520,519,492,
  458,431,232,135,134,1,0,51,52,
707,705,704,703,702,701,700,699,698,697,696,695,694,691,690,689,688,687,686,
  685,539,537,536,535,534,533,532,531,530,529,528,527,526,523,520,519,492,
  458,431,232,135,134,1,0,51,52,
707,705,704,703,702,701,700,699,698,697,696,695,694,691,690,689,688,687,686,
  685,539,537,536,535,534,533,532,531,530,529,528,527,526,523,520,519,492,
  458,431,232,135,134,1,0,51,52,
707,705,704,703,702,701,700,699,698,697,696,695,694,691,690,689,688,687,686,
  685,539,537,536,535,534,533,532,531,530,529,528,527,526,523,520,519,492,
  458,431,232,135,134,1,0,51,52,
707,705,704,703,702,701,700,699,698,697,696,695,694,691,690,689,688,687,686,
  685,539,537,536,535,534,533,532,531,530,529,528,527,526,523,520,519,492,
  458,431,232,135,134,1,0,51,52,
707,705,704,703,702,701,700,699,698,697,696,695,694,691,690,689,688,687,686,
  685,539,537,536,535,534,533,532,531,530,529,528,527,526,523,520,519,492,
  458,431,232,135,134,1,0,51,52,
707,705,704,703,702,701,700,699,698,697,696,695,694,691,690,689,688,687,686,
  685,539,537,536,535,534,533,532,531,530,529,528,527,526,523,520,519,492,
  458,431,232,135,134,1,0,51,52,
707,705,704,703,702,701,700,699,698,697,696,695,694,691,690,689,688,687,686,
  685,539,537,536,535,534,533,532,531,530,529,528,527,526,523,520,519,492,
  458,431,232,135,134,1,0,51,52,
707,705,704,703,702,701,700,699,698,697,696,695,694,691,690,689,688,687,686,
  685,539,537,536,535,534,533,532,531,530,529,528,527,526,523,520,519,492,
  458,431,232,135,134,1,0,51,52,
707,705,704,703,702,701,700,699,698,697,696,695,694,691,690,689,688,687,686,
  685,539,537,536,535,534,533,532,531,530,529,528,527,526,523,520,519,492,
  458,431,232,135,134,1,0,51,52,
707,705,704,703,702,701,700,699,698,697,696,695,694,691,690,689,688,687,686,
  685,539,537,536,535,534,533,532,531,530,529,528,527,526,523,520,519,492,
  458,431,232,135,134,1,0,51,52,
707,705,704,703,702,701,700,699,698,697,696,695,694,691,690,689,688,687,686,
  685,539,537,536,535,534,533,532,531,530,529,528,527,526,523,520,519,492,
  458,431,232,135,134,1,0,51,52,
707,705,704,703,702,701,700,699,698,697,696,695,694,691,690,689,688,687,686,
  685,539,537,536,535,534,533,532,531,530,529,528,527,526,523,520,519,492,
  458,431,232,135,134,1,0,51,52,
707,705,704,703,702,701,700,699,698,697,696,695,694,691,690,689,688,687,686,
  685,539,537,536,535,534,533,532,531,530,529,528,527,526,523,520,519,492,
  458,431,232,135,134,1,0,51,52,
707,705,704,703,702,701,700,699,698,697,696,695,694,691,690,689,688,687,686,
  685,539,537,536,535,534,533,532,531,530,529,528,527,526,523,520,519,492,
  458,431,232,135,134,1,0,51,52,
707,705,704,703,702,701,700,699,698,697,696,695,694,691,690,689,688,687,686,
  685,539,537,536,535,534,533,532,531,530,529,528,527,526,523,520,519,492,
  458,431,232,135,134,1,0,51,52,
707,705,704,703,702,701,700,699,698,697,696,695,694,691,690,689,688,687,686,
  685,539,537,536,535,534,533,532,531,530,529,528,527,526,523,520,519,492,
  458,431,232,135,134,1,0,51,52,
707,705,704,703,702,701,700,699,698,697,696,695,694,691,690,689,688,687,686,
  685,539,537,536,535,534,533,532,531,530,529,528,527,526,523,520,519,492,
  458,431,232,135,134,1,0,51,52,
707,705,704,703,702,701,700,699,698,697,696,695,694,691,690,689,688,687,686,
  685,539,537,536,535,534,533,532,531,530,529,528,527,526,523,520,519,492,
  458,431,232,135,134,1,0,51,52,
707,705,704,703,702,701,700,699,698,697,696,695,694,691,690,689,688,687,686,
  685,539,537,536,535,534,533,532,531,530,529,528,527,526,523,520,519,492,
  458,431,232,135,134,1,0,51,52,
707,705,704,703,702,701,700,699,698,697,696,695,694,691,690,689,688,687,686,
  685,539,537,536,535,534,533,532,531,530,529,528,527,526,523,520,519,492,
  458,431,232,135,134,1,0,51,52,
707,705,704,703,702,701,700,699,698,697,696,695,694,691,690,689,688,687,686,
  685,539,537,536,535,534,533,532,531,530,529,528,527,526,523,520,519,492,
  458,431,232,135,134,1,0,51,52,
707,705,704,703,702,701,700,699,698,697,696,695,694,691,690,689,688,687,686,
  685,539,537,536,535,534,533,532,531,530,529,528,527,526,523,520,519,492,
  458,431,232,135,134,1,0,51,52,
707,705,704,703,702,701,700,699,698,697,696,695,694,691,690,689,688,687,686,
  685,539,537,536,535,534,533,532,531,530,529,528,527,526,523,520,519,492,
  458,431,232,135,134,1,0,51,52,
707,705,704,703,702,701,700,699,698,697,696,695,694,691,690,689,688,687,686,
  685,539,537,536,535,534,533,532,531,530,529,528,527,526,523,520,519,492,
  458,431,232,135,134,1,0,51,52,
707,705,704,703,702,701,700,699,698,697,696,695,694,691,690,689,688,687,686,
  685,539,537,536,535,534,533,532,531,530,529,528,527,526,523,520,519,492,
  458,431,232,135,134,1,0,51,52,
707,705,704,703,702,701,700,699,698,697,696,695,694,691,690,689,688,687,686,
  685,539,537,536,535,534,533,532,531,530,529,528,527,526,523,520,519,492,
  458,431,232,135,134,1,0,51,52,
707,705,704,703,702,701,700,699,698,697,696,695,694,691,690,689,688,687,686,
  685,539,537,536,535,534,533,532,531,530,529,528,527,526,523,520,519,492,
  458,431,232,135,134,1,0,51,52,
707,705,704,703,702,701,700,699,698,697,696,695,694,691,690,689,688,687,686,
  685,539,537,536,535,534,533,532,531,530,529,528,527,526,523,520,519,492,
  458,431,232,135,134,1,0,51,52,
707,705,704,703,702,701,700,699,698,697,696,695,694,691,690,689,688,687,686,
  685,539,537,536,535,534,533,532,531,530,529,528,527,526,523,520,519,492,
  458,431,232,135,134,1,0,51,52,
670,542,541,249,247,239,237,235,1,0,51,52,
670,542,541,249,247,239,237,235,1,0,51,52,
670,542,541,249,247,239,237,235,1,0,51,52,
670,542,541,249,247,239,237,235,1,0,51,52,
670,247,237,235,1,0,51,52,
670,247,237,235,1,0,51,52,
670,545,249,247,239,237,235,1,0,51,52,
670,545,249,247,239,237,235,1,0,51,52,
670,545,249,247,239,237,235,1,0,51,52,
670,545,544,543,249,247,239,237,235,1,0,51,52,
670,545,544,543,249,247,239,237,235,1,0,51,52,
542,242,241,240,239,238,237,236,235,1,0,51,52,
542,242,241,240,239,238,237,236,235,1,0,51,52,
542,242,241,240,239,238,237,236,235,1,0,51,52,
542,242,241,240,239,238,237,236,235,1,0,51,52,
542,242,241,240,239,238,237,236,235,1,0,51,52,
542,242,241,240,239,238,237,236,235,1,0,51,52,
542,242,241,240,239,238,237,236,235,1,0,51,52,
542,242,241,240,239,238,237,236,235,1,0,51,52,
542,242,241,240,239,238,237,236,235,1,0,51,52,
542,242,241,240,239,238,237,236,235,1,0,51,52,
542,242,241,240,239,238,237,236,235,1,0,51,52,
690,689,688,687,1,0,51,52,
481,1,0,51,52,
481,1,0,51,52,
481,1,0,51,52,
707,705,704,703,702,701,700,699,698,697,696,695,694,691,690,689,688,687,686,
  685,539,537,536,535,534,533,532,531,530,529,528,527,526,523,520,519,492,
  458,431,232,135,134,1,0,51,52,
707,705,704,703,702,701,700,699,698,697,696,695,694,691,690,689,688,687,686,
  685,539,537,536,535,534,533,532,531,530,529,528,527,526,523,520,519,492,
  458,431,232,135,134,1,0,51,52,
707,705,704,703,702,701,700,699,698,697,696,695,694,691,690,689,688,687,686,
  685,539,537,536,535,534,533,532,531,530,529,528,527,526,523,520,519,492,
  458,431,232,135,134,1,0,51,52,
707,705,704,703,702,701,700,699,698,697,696,695,694,691,690,689,688,687,686,
  685,539,537,536,535,534,533,532,531,530,529,528,527,526,523,520,519,492,
  458,431,232,135,134,1,0,51,52,
707,705,704,703,702,701,700,699,698,697,696,695,694,691,690,689,688,687,686,
  685,539,537,536,535,534,533,532,531,530,529,528,527,526,523,520,519,492,
  458,431,232,135,134,1,0,51,52,
707,705,704,703,702,701,700,699,698,697,696,695,694,691,690,689,688,687,686,
  685,539,537,536,535,534,533,532,531,530,529,528,527,526,523,520,519,492,
  458,431,232,135,134,1,0,51,52,
707,705,704,703,702,701,700,699,698,697,696,695,694,691,690,689,688,687,686,
  685,539,537,536,535,534,533,532,531,530,529,528,527,526,523,520,519,492,
  458,431,232,135,134,1,0,51,52,
707,705,704,703,702,701,700,699,698,697,696,695,694,691,690,689,688,687,686,
  685,539,537,536,535,534,533,532,531,530,529,528,527,526,523,520,519,492,
  458,431,232,135,134,1,0,51,52,
707,705,704,703,702,701,700,699,698,697,696,695,694,691,690,689,688,687,686,
  685,539,537,536,535,534,533,532,531,530,529,528,527,526,523,520,519,492,
  458,431,232,135,134,1,0,51,52,
707,705,704,703,702,701,700,699,698,697,696,695,694,691,690,689,688,687,686,
  685,539,537,536,535,534,533,532,531,530,529,528,527,526,523,520,519,492,
  458,431,232,135,134,1,0,51,52,
707,705,704,703,702,701,700,699,698,697,696,695,694,691,690,689,688,687,686,
  685,539,537,536,535,534,533,532,531,530,529,528,527,526,523,520,519,492,
  458,431,232,135,134,1,0,51,52,
707,705,704,703,702,701,700,699,698,697,696,695,694,691,690,689,688,687,686,
  685,539,537,536,535,534,533,532,531,530,529,528,527,526,523,520,519,492,
  458,431,232,135,134,1,0,51,52,
707,705,704,703,702,701,700,699,698,697,696,695,694,691,690,689,688,687,686,
  685,539,537,536,535,534,533,532,531,530,529,528,527,526,523,520,519,492,
  458,431,232,135,134,1,0,51,52,
707,705,704,703,702,701,700,699,698,697,696,695,694,691,690,689,688,687,686,
  685,539,537,536,535,534,533,532,531,530,529,528,527,526,523,520,519,492,
  458,431,232,135,134,1,0,51,52,
707,705,704,703,702,701,700,699,698,697,696,695,694,691,690,689,688,687,686,
  685,539,537,536,535,534,533,532,531,530,529,528,527,526,523,520,519,492,
  458,431,232,135,134,1,0,51,52,
707,705,704,703,702,701,700,699,698,697,696,695,694,691,690,689,688,687,686,
  685,539,537,536,535,534,533,532,531,530,529,528,527,526,523,520,519,492,
  458,431,232,135,134,1,0,51,52,
707,705,704,703,702,701,700,699,698,697,696,695,694,691,690,689,688,687,686,
  685,539,537,536,535,534,533,532,531,530,529,528,527,526,523,520,519,492,
  458,431,232,135,134,1,0,51,52,
707,705,704,703,702,701,700,699,698,697,696,695,694,691,690,689,688,687,686,
  685,539,537,536,535,534,533,532,531,530,529,528,527,526,523,520,519,492,
  458,431,232,135,134,1,0,51,52,
707,705,704,703,702,701,700,699,698,697,696,695,694,691,690,689,688,687,686,
  685,539,537,536,535,534,533,532,531,530,529,528,527,526,523,520,519,492,
  458,431,232,135,134,1,0,51,52,
707,705,704,703,702,701,700,699,698,697,696,695,694,691,690,689,688,687,686,
  685,539,537,536,535,534,533,532,531,530,529,528,527,526,523,520,519,492,
  458,431,232,135,134,1,0,51,52,
707,705,704,703,702,701,700,699,698,697,696,695,694,691,690,689,688,687,686,
  685,539,537,536,535,534,533,532,531,530,529,528,527,526,523,520,519,492,
  458,431,232,135,134,1,0,51,52,
707,705,704,703,702,701,700,699,698,697,696,695,694,691,690,689,688,687,686,
  685,539,537,536,535,534,533,532,531,530,529,528,527,526,523,520,519,492,
  458,431,232,135,134,1,0,51,52,
707,705,704,703,702,701,700,699,698,697,696,695,694,691,690,689,688,687,686,
  685,539,537,536,535,534,533,532,531,530,529,528,527,526,523,520,519,492,
  458,431,232,135,134,1,0,51,52,
707,705,704,703,702,701,700,699,698,697,696,695,694,691,690,689,688,687,686,
  685,539,537,536,535,534,533,532,531,530,529,528,527,526,523,520,519,492,
  458,431,232,135,134,1,0,51,52,
707,705,704,703,702,701,700,699,698,697,696,695,694,691,690,689,688,687,686,
  685,539,537,536,535,534,533,532,531,530,529,528,527,526,523,520,519,492,
  458,431,232,135,134,1,0,51,52,
707,705,704,703,702,701,700,699,698,697,696,695,694,691,690,689,688,687,686,
  685,539,537,536,535,534,533,532,531,530,529,528,527,526,523,520,519,492,
  458,431,232,135,134,1,0,51,52,
707,705,704,703,702,701,700,699,698,697,696,695,694,691,690,689,688,687,686,
  685,539,537,536,535,534,533,532,531,530,529,528,527,526,523,520,519,492,
  458,431,232,135,134,1,0,51,52,
707,705,704,703,702,701,700,699,698,697,696,695,694,691,690,689,688,687,686,
  685,539,537,536,535,534,533,532,531,530,529,528,527,526,523,520,519,492,
  458,431,232,135,134,1,0,51,52,
707,705,704,703,702,701,700,699,698,697,696,695,694,691,690,689,688,687,686,
  685,539,537,536,535,534,533,532,531,530,529,528,527,526,523,520,519,492,
  458,431,232,135,134,1,0,51,52,
707,705,704,703,702,701,700,699,698,697,696,695,694,691,690,689,688,687,686,
  685,539,537,536,535,534,533,532,531,530,529,528,527,526,523,520,519,492,
  458,431,232,135,134,1,0,51,52,
707,705,704,703,702,701,700,699,698,697,696,695,694,691,690,689,688,687,686,
  685,539,537,536,535,534,533,532,531,530,529,528,527,526,523,520,519,492,
  458,431,232,135,134,1,0,51,52,
707,705,704,703,702,701,700,699,698,697,696,695,694,691,690,689,688,687,686,
  685,539,537,536,535,534,533,532,531,530,529,528,527,526,523,520,519,492,
  458,431,232,135,134,1,0,51,52,
707,705,704,703,702,701,700,699,698,697,696,695,694,691,690,689,688,687,686,
  685,539,537,536,535,534,533,532,531,530,529,528,527,526,523,520,519,492,
  458,431,232,135,134,1,0,51,52,
707,705,704,703,702,701,700,699,698,697,696,695,694,691,690,689,688,687,686,
  685,539,537,536,535,534,533,532,531,530,529,528,527,526,523,520,519,492,
  458,431,232,135,134,1,0,51,52,
707,705,704,703,702,701,700,699,698,697,696,695,694,691,690,689,688,687,686,
  685,539,537,536,535,534,533,532,531,530,529,528,527,526,523,520,519,492,
  458,431,232,135,134,1,0,51,52,
707,705,704,703,702,701,700,699,698,697,696,695,694,691,690,689,688,687,686,
  685,539,537,536,535,534,533,532,531,530,529,528,527,526,523,520,519,492,
  458,431,232,135,134,1,0,51,52,
707,705,704,703,702,701,700,699,698,697,696,695,694,691,690,689,688,687,686,
  685,539,537,536,535,534,533,532,531,530,529,528,527,526,523,520,519,492,
  458,431,232,135,134,1,0,51,52,
707,705,704,703,702,701,700,699,698,697,696,695,694,691,690,689,688,687,686,
  685,539,537,536,535,534,533,532,531,530,529,528,527,526,523,520,519,492,
  458,431,232,135,134,1,0,51,52,
707,705,704,703,702,701,700,699,698,697,696,695,694,691,690,689,688,687,686,
  685,539,537,536,535,534,533,532,531,530,529,528,527,526,523,520,519,492,
  458,431,232,135,134,1,0,51,52,
707,705,704,703,702,701,700,699,698,697,696,695,694,691,690,689,688,687,686,
  685,539,537,536,535,534,533,532,531,530,529,528,527,526,523,520,519,492,
  458,431,232,135,134,1,0,51,52,
707,705,704,703,702,701,700,699,698,697,696,695,694,691,690,689,688,687,686,
  685,539,537,536,535,534,533,532,531,530,529,528,527,526,523,520,519,492,
  458,431,232,135,134,1,0,51,52,
707,705,704,703,702,701,700,699,698,697,696,695,694,691,690,689,688,687,686,
  685,539,537,536,535,534,533,532,531,530,529,528,527,526,523,520,519,492,
  458,431,232,135,134,1,0,51,52,
707,705,704,703,702,701,700,699,698,697,696,695,694,691,690,689,688,687,686,
  685,539,537,536,535,534,533,532,531,530,529,528,527,526,523,520,519,492,
  458,431,232,135,134,1,0,51,52,
707,705,704,703,702,701,700,699,698,697,696,695,694,691,690,689,688,687,686,
  685,539,537,536,535,534,533,532,531,530,529,528,527,526,523,520,519,492,
  458,431,232,135,134,1,0,51,52,
707,705,704,703,702,701,700,699,698,697,696,695,694,691,690,689,688,687,686,
  685,539,537,536,535,534,533,532,531,530,529,528,527,526,523,520,519,492,
  458,431,232,135,134,1,0,51,52,
707,705,704,703,702,701,700,699,698,697,696,695,694,691,690,689,688,687,686,
  685,539,537,536,535,534,533,532,531,530,529,528,527,526,523,520,519,492,
  458,431,232,135,134,1,0,51,52,
707,705,704,703,702,701,700,699,698,697,696,695,694,691,690,689,688,687,686,
  685,539,537,536,535,534,533,532,531,530,529,528,527,526,523,520,519,492,
  458,431,232,135,134,1,0,51,52,
707,705,704,703,702,701,700,699,698,697,696,695,694,691,690,689,688,687,686,
  685,539,537,536,535,534,533,532,531,530,529,528,527,526,523,520,519,492,
  458,431,232,135,134,1,0,51,52,
707,705,704,703,702,701,700,699,698,697,696,695,694,691,690,689,688,687,686,
  685,539,537,536,535,534,533,532,531,530,529,528,527,526,523,520,519,492,
  458,431,232,135,134,1,0,51,52,
707,705,704,703,702,701,700,699,698,697,696,695,694,691,690,689,688,687,686,
  685,539,537,536,535,534,533,532,531,530,529,528,527,526,523,520,519,492,
  458,431,232,135,134,1,0,51,52,
707,705,704,703,702,701,700,699,698,697,696,695,694,691,690,689,688,687,686,
  685,539,537,536,535,534,533,532,531,530,529,528,527,526,523,520,519,492,
  458,431,232,135,134,1,0,51,52,
707,705,704,703,702,701,700,699,698,697,696,695,694,691,690,689,688,687,686,
  685,539,537,536,535,534,533,532,531,530,529,528,527,526,523,520,519,492,
  458,431,232,135,134,1,0,51,52,
670,542,541,249,247,239,237,235,1,0,51,52,
670,542,541,249,247,239,237,235,1,0,51,52,
670,542,541,249,247,239,237,235,1,0,51,52,
670,542,541,249,247,239,237,235,1,0,51,52,
670,247,237,235,1,0,51,52,
670,247,237,235,1,0,51,52,
670,545,249,247,239,237,235,1,0,51,52,
670,545,249,247,239,237,235,1,0,51,52,
670,545,249,247,239,237,235,1,0,51,52,
670,545,544,543,249,247,239,237,235,1,0,51,52,
670,545,544,543,249,247,239,237,235,1,0,51,52,
542,242,241,240,239,238,237,236,235,1,0,51,52,
542,242,241,240,239,238,237,236,235,1,0,51,52,
542,242,241,240,239,238,237,236,235,1,0,51,52,
542,242,241,240,239,238,237,236,235,1,0,51,52,
542,242,241,240,239,238,237,236,235,1,0,51,52,
542,242,241,240,239,238,237,236,235,1,0,51,52,
542,242,241,240,239,238,237,236,235,1,0,51,52,
542,242,241,240,239,238,237,236,235,1,0,51,52,
542,242,241,240,239,238,237,236,235,1,0,51,52,
542,242,241,240,239,238,237,236,235,1,0,51,52,
542,242,241,240,239,238,237,236,235,1,0,51,52,
708,707,705,704,703,702,701,700,699,698,697,696,695,694,693,692,691,690,689,
  688,687,686,685,683,682,546,528,526,525,523,522,521,520,519,517,515,513,
  511,509,508,506,504,502,500,498,497,494,481,458,456,431,429,427,426,425,
  1,0,51,52,138,
546,429,427,426,425,1,0,51,52,
546,525,524,523,522,521,520,519,518,517,516,515,514,513,512,511,510,509,508,
  507,506,505,504,503,502,501,500,499,498,497,496,494,458,456,429,427,426,
  425,1,0,86,
429,427,426,425,1,0,51,
546,0,268,
707,705,704,703,702,701,700,699,698,697,696,695,694,691,690,689,688,687,686,
  685,539,537,536,535,534,533,532,531,530,529,528,527,526,523,520,519,492,
  458,431,232,135,134,0,2,6,7,8,9,15,17,21,23,24,25,26,27,28,29,57,76,86,
  89,131,133,166,197,199,204,205,206,207,208,209,210,211,212,213,214,215,
  216,217,218,219,220,443,493,540,
492,491,490,489,488,487,485,484,483,482,480,479,478,477,476,475,474,473,472,
  471,470,469,468,467,466,465,464,463,462,461,460,446,445,444,442,440,0,
  68,69,70,71,72,95,96,97,98,99,100,101,102,103,104,105,106,107,108,109,
  110,111,112,114,115,116,117,118,119,120,121,123,124,125,126,127,128,129,
  130,131,
429,427,426,425,1,0,51,52,
708,707,705,704,703,702,701,700,699,698,697,696,695,694,693,692,691,690,689,
  688,687,686,685,683,682,528,526,523,522,521,520,519,513,511,509,481,458,
  456,431,427,1,0,86,
708,707,705,704,703,702,701,700,699,698,697,696,695,694,693,692,691,690,689,
  688,687,686,685,683,682,528,526,523,522,521,520,519,513,511,509,481,458,
  456,431,427,1,0,51,52,
429,427,426,425,1,0,51,52,
707,706,705,704,703,702,701,700,699,698,697,696,695,694,693,692,691,690,689,
  688,687,686,685,684,683,682,528,526,523,522,521,520,519,513,511,509,481,
  458,456,431,427,425,1,0,142,
494,481,456,429,427,426,425,1,0,51,52,
429,427,426,425,1,0,51,52,
429,427,426,425,0,39,40,41,45,46,47,49,
429,427,426,425,0,39,40,41,45,46,47,49,
429,427,426,425,1,0,51,52,
691,690,689,688,687,0,
690,689,688,687,0,
688,687,0,
707,705,704,703,702,701,700,699,698,697,696,694,691,690,689,688,687,685,546,
  525,524,523,522,521,520,519,518,517,516,515,514,513,512,511,510,509,508,
  507,506,505,504,503,502,501,500,499,498,497,496,494,458,456,431,429,427,
  426,425,1,0,
707,705,701,699,698,697,696,691,690,689,688,687,685,546,525,524,523,522,521,
  520,519,518,517,516,515,514,513,512,511,510,509,508,507,506,505,504,503,
  502,501,500,499,498,497,496,494,456,431,429,427,426,425,223,222,1,0,
691,690,689,688,687,0,
707,706,705,704,703,702,701,700,699,698,697,696,695,694,693,692,691,690,689,
  688,687,685,684,683,682,528,526,523,522,521,520,519,513,511,509,481,458,
  456,431,427,425,234,1,0,16,
691,690,689,688,687,0,
701,699,698,697,696,691,690,689,688,687,0,
690,689,688,687,546,525,524,523,522,521,520,519,518,517,516,515,514,513,512,
  511,510,509,508,507,506,505,504,503,502,501,500,499,498,497,496,494,456,
  429,427,426,425,1,0,
688,687,546,525,524,523,522,521,520,519,518,517,516,515,514,513,512,511,510,
  509,508,507,506,505,504,503,502,501,500,499,498,497,496,494,456,429,427,
  426,425,1,0,
699,698,697,696,691,690,689,688,687,546,525,524,523,522,521,520,519,518,517,
  516,515,514,513,512,511,510,509,508,507,506,505,504,503,502,501,500,499,
  498,497,496,494,456,429,427,426,425,1,0,
707,701,699,698,697,696,691,690,689,688,687,685,546,525,524,523,522,521,520,
  519,518,517,516,515,513,512,511,510,509,508,507,506,505,504,503,502,501,
  500,499,498,497,494,456,431,429,427,426,425,223,222,1,0,224,
458,1,0,51,52,
458,429,427,426,425,1,0,51,52,
458,1,0,51,52,
458,1,0,51,52,
458,1,0,51,52,
458,1,0,51,52,
458,1,0,51,52,
458,1,0,51,52,
458,1,0,51,52,
458,1,0,51,52,
458,1,0,51,52,
546,525,524,523,522,521,520,519,518,517,516,515,514,513,512,511,510,509,508,
  507,506,505,504,503,502,501,500,499,498,497,496,494,456,429,427,426,425,
  1,0,51,52,
546,525,524,523,522,521,520,519,518,517,516,515,514,513,512,511,510,509,508,
  507,506,505,504,503,502,501,500,499,498,497,496,494,456,429,427,426,425,
  1,0,51,52,
708,707,706,705,704,703,702,701,700,699,698,697,696,695,694,693,692,691,690,
  689,688,687,686,685,684,683,682,539,537,536,535,534,533,532,531,530,529,
  528,527,526,523,522,521,520,519,513,511,509,492,481,459,458,457,456,431,
  429,428,427,426,425,232,135,134,1,0,51,52,
458,1,0,51,52,
458,1,0,51,52,
458,1,0,51,52,
458,1,0,51,52,
458,1,0,51,52,
458,1,0,51,52,
458,1,0,51,52,
458,1,0,51,52,
458,1,0,51,52,
458,1,0,51,52,
458,1,0,51,52,
707,705,704,703,702,701,700,699,698,697,696,695,694,691,690,689,688,687,686,
  685,539,537,536,535,534,533,532,531,530,529,523,520,519,492,458,431,232,
  135,134,1,0,51,52,
708,707,706,705,704,703,702,701,700,699,698,697,696,695,694,693,692,691,690,
  689,688,687,686,685,684,683,682,539,537,536,535,534,533,532,531,530,529,
  528,526,523,522,521,520,519,513,511,509,492,481,459,458,456,431,429,428,
  427,426,425,232,135,134,1,0,51,52,
707,705,704,703,702,701,700,699,698,697,696,695,694,691,690,689,688,687,686,
  685,539,537,536,535,534,533,532,531,530,529,523,520,519,492,458,431,232,
  135,134,1,0,51,52,
708,707,706,705,704,703,702,701,700,699,698,697,696,695,694,693,692,691,690,
  689,688,687,686,685,684,683,682,539,537,536,535,534,533,532,531,530,529,
  528,526,523,522,521,520,519,513,511,509,492,481,459,458,456,431,429,428,
  427,426,425,232,135,134,1,0,51,52,
707,705,704,703,702,701,700,699,698,697,696,695,694,691,690,689,688,687,686,
  685,539,537,536,535,534,533,532,531,530,529,528,527,526,523,520,519,492,
  458,431,232,135,134,1,0,51,52,
707,705,704,703,702,701,700,699,698,697,696,695,694,691,690,689,688,687,686,
  685,539,537,536,535,534,533,532,531,530,529,523,520,519,492,458,431,232,
  135,134,0,2,6,7,8,9,15,17,21,86,89,131,133,199,207,208,209,210,211,212,
  213,214,215,216,217,218,219,220,443,493,540,
707,705,704,703,702,701,700,699,698,697,696,695,694,691,690,689,688,687,686,
  685,539,537,536,535,534,533,532,531,530,529,523,520,519,492,458,431,232,
  135,134,0,2,6,7,8,9,15,17,21,86,89,131,133,199,207,208,209,210,211,212,
  213,214,215,216,217,218,219,220,443,493,540,
707,705,704,703,702,701,700,699,698,697,696,695,694,691,690,689,688,687,686,
  685,539,537,536,535,534,533,532,531,530,529,523,520,519,492,458,431,232,
  135,134,0,2,6,7,8,9,15,17,21,86,89,131,133,199,207,208,209,210,211,212,
  213,214,215,216,217,218,219,220,443,493,540,
707,705,704,703,702,701,700,699,698,697,696,695,694,691,690,689,688,687,686,
  685,539,537,536,535,534,533,532,531,530,529,523,520,519,492,458,431,232,
  135,134,0,2,6,7,8,9,15,17,21,86,89,131,133,199,207,208,209,210,211,212,
  213,214,215,216,217,218,219,220,443,493,540,
546,525,524,523,522,521,520,519,518,517,516,515,514,513,512,511,510,509,508,
  507,506,505,504,503,502,501,500,499,498,497,496,494,456,429,427,426,425,
  1,0,51,52,
546,525,524,523,522,521,520,519,518,517,516,515,514,513,512,511,510,509,508,
  507,506,505,504,503,502,501,500,499,498,497,496,494,456,427,425,1,0,51,
  52,198,200,201,202,203,
546,520,519,518,517,516,515,514,513,512,511,510,509,508,507,506,505,504,503,
  502,501,500,499,498,497,496,494,456,429,427,426,425,1,0,51,52,196,197,
546,518,517,516,515,514,513,512,511,510,509,508,507,506,505,504,503,502,501,
  500,499,498,497,496,494,456,429,427,426,425,1,0,51,52,192,193,194,195,
546,514,513,512,511,510,509,508,507,506,505,504,503,502,501,500,499,498,497,
  496,494,456,429,427,426,425,1,0,51,52,190,191,
546,512,511,510,509,508,507,506,505,504,503,502,501,500,499,498,497,496,494,
  456,429,427,426,425,1,0,51,52,188,189,
510,509,0,186,187,
508,507,506,505,504,503,502,501,500,499,498,497,496,429,427,426,425,1,0,167,
  168,169,170,171,172,173,174,175,176,177,178,179,180,181,182,183,184,185,
707,705,704,703,702,701,700,699,698,697,696,695,694,691,690,689,688,687,686,
  685,539,537,536,535,534,533,532,531,530,529,528,527,526,523,520,519,492,
  458,431,232,135,134,0,2,6,7,8,9,15,17,21,23,24,25,26,27,28,29,57,86,89,
  131,133,197,199,204,205,206,207,208,209,210,211,212,213,214,215,216,217,
  218,219,220,443,493,540,
429,427,426,425,1,0,51,52,
429,427,426,425,1,0,51,52,
429,427,426,425,1,0,74,
429,427,426,425,1,0,51,52,
429,427,426,425,1,0,51,52,
429,427,426,425,1,0,51,52,
429,427,426,425,1,0,51,52,
707,705,704,703,702,701,700,699,698,697,696,694,685,135,134,1,0,51,52,
707,705,704,703,702,701,700,699,698,697,696,694,685,135,134,1,0,51,52,
429,427,426,425,1,0,51,52,
429,427,426,425,1,0,51,52,
429,427,426,425,1,0,51,52,
429,427,426,425,1,0,51,52,
708,707,706,705,704,703,702,701,700,699,698,697,696,695,694,693,692,691,690,
  689,688,687,686,685,684,682,528,526,523,522,521,520,519,513,511,509,481,
  458,456,431,427,1,0,
429,427,426,425,1,0,51,52,
429,427,426,425,1,0,51,52,
429,427,426,425,1,0,51,52,
707,706,705,704,703,702,701,700,699,698,697,696,692,691,690,689,688,687,683,
  522,431,1,0,51,52,
707,706,705,704,703,702,701,700,699,698,697,696,692,691,690,689,688,687,683,
  522,431,0,10,12,13,14,19,436,437,438,
708,707,706,705,704,703,702,701,700,699,698,697,696,695,694,693,692,691,690,
  689,688,687,686,685,684,683,682,528,526,523,522,521,520,519,513,511,509,
  481,459,458,456,431,429,428,427,426,425,1,0,51,52,
708,707,706,705,704,703,702,701,700,699,698,697,696,695,694,693,692,691,690,
  689,688,687,686,685,684,683,682,539,537,536,535,534,533,532,531,530,529,
  528,527,526,523,522,521,520,519,513,511,509,492,481,459,458,456,431,429,
  428,427,426,425,232,135,134,1,0,51,52,
708,707,706,705,704,703,702,701,700,699,698,697,696,695,694,693,692,691,690,
  689,688,687,686,685,684,683,682,528,526,523,522,521,520,519,513,511,509,
  481,459,458,456,431,429,428,427,426,425,1,0,51,52,
708,707,706,705,704,703,702,701,700,699,698,697,696,695,694,693,692,691,690,
  689,688,687,686,685,684,683,682,539,537,536,535,534,533,532,531,530,529,
  528,527,526,523,522,521,520,519,513,511,509,492,481,459,458,456,431,429,
  428,427,426,425,232,135,134,1,0,51,52,
708,707,706,705,704,703,702,701,700,699,698,697,696,695,694,693,692,691,690,
  689,688,687,686,685,684,683,682,528,526,523,522,521,520,519,513,511,509,
  481,458,456,431,429,428,427,425,1,0,51,52,
708,707,706,705,704,703,702,701,700,699,698,697,696,695,694,693,692,691,690,
  689,688,687,686,685,684,683,682,528,526,523,522,521,520,519,513,511,509,
  481,459,458,456,431,429,428,427,426,425,1,0,51,52,
708,707,706,705,704,703,702,701,700,699,698,697,696,695,694,693,692,691,690,
  689,688,687,686,685,684,683,682,528,526,523,522,521,520,519,513,511,509,
  481,459,458,456,431,429,428,427,426,425,1,0,51,52,
708,707,706,705,704,703,702,701,700,699,698,697,696,695,694,693,692,691,690,
  689,688,687,686,685,684,683,682,528,526,523,522,521,520,519,513,511,509,
  481,459,458,456,431,429,428,427,426,425,1,0,51,52,
708,707,706,705,704,703,702,701,700,699,698,697,696,695,694,693,692,691,690,
  689,688,687,686,685,684,683,682,528,526,523,522,521,520,519,513,511,509,
  481,459,458,456,431,429,428,427,426,425,1,0,51,52,
708,707,706,705,704,703,702,701,700,699,698,697,696,695,694,693,692,691,690,
  689,688,687,686,685,684,683,682,528,526,523,522,521,520,519,513,511,509,
  481,459,458,456,431,429,428,427,426,425,1,0,51,52,
708,707,706,705,704,703,702,701,700,699,698,697,696,695,694,693,692,691,690,
  689,688,687,686,685,684,683,682,528,526,523,522,521,520,519,513,511,509,
  481,459,458,456,431,429,428,427,426,425,1,0,51,52,
708,707,706,705,704,703,702,701,700,699,698,697,696,695,694,693,692,691,690,
  689,688,687,686,685,684,683,682,528,526,523,522,521,520,519,513,511,509,
  481,459,458,456,431,429,428,427,426,425,1,0,51,52,
708,707,706,705,704,703,702,701,700,699,698,697,696,695,694,693,692,691,690,
  689,688,687,686,685,684,683,682,528,526,523,522,521,520,519,513,511,509,
  481,459,458,456,431,429,428,427,426,425,1,0,51,52,
708,707,706,705,704,703,702,701,700,699,698,697,696,695,694,693,692,691,690,
  689,688,687,686,685,684,683,682,528,526,523,522,521,520,519,513,511,509,
  481,459,458,456,431,429,428,427,426,425,1,0,51,52,
708,707,706,705,704,703,702,701,700,699,698,697,696,695,694,693,692,691,690,
  689,688,687,686,685,684,683,682,528,526,523,522,521,520,519,513,511,509,
  481,459,458,456,431,429,428,427,426,425,1,0,51,52,
708,707,706,705,704,703,702,701,700,699,698,697,696,695,694,693,692,691,690,
  689,688,687,686,685,684,683,682,528,526,523,522,521,520,519,513,511,509,
  481,459,458,456,431,429,428,427,426,425,1,0,51,52,
708,707,706,705,704,703,702,701,700,699,698,697,696,695,694,693,692,691,690,
  689,688,687,686,685,684,683,682,528,526,523,522,521,520,519,513,511,509,
  481,459,458,456,431,429,428,427,426,425,1,0,51,52,
708,707,706,705,704,703,702,701,700,699,698,697,696,695,694,693,692,691,690,
  689,688,687,686,685,684,683,682,528,526,523,522,521,520,519,513,511,509,
  481,459,458,456,431,429,428,427,426,425,1,0,51,52,
708,707,706,705,704,703,702,701,700,699,698,697,696,695,694,693,692,691,690,
  689,688,687,686,685,684,683,682,528,526,523,522,521,520,519,513,511,509,
  481,459,458,456,431,429,428,427,426,425,1,0,51,52,
708,707,706,705,704,703,702,701,700,699,698,697,696,695,694,693,692,691,690,
  689,688,687,686,685,684,683,682,528,526,523,522,521,520,519,513,511,509,
  481,459,458,456,431,429,428,427,426,425,1,0,51,52,
708,707,706,705,704,703,702,701,700,699,698,697,696,695,694,693,692,691,690,
  689,688,687,686,685,684,683,682,528,526,523,522,521,520,519,513,511,509,
  481,459,458,456,431,429,428,427,426,425,1,0,51,52,
708,707,706,705,704,703,702,701,700,699,698,697,696,695,694,693,692,691,690,
  689,688,687,686,685,684,683,682,528,526,523,522,521,520,519,513,511,509,
  481,459,458,456,431,429,428,427,426,425,1,0,51,52,
708,707,706,705,704,703,702,701,700,699,698,697,696,695,694,693,692,691,690,
  689,688,687,686,685,684,683,682,528,526,523,522,521,520,519,513,511,509,
  481,459,458,456,431,429,428,427,426,425,1,0,51,52,
708,707,706,705,704,703,702,701,700,699,698,697,696,695,694,693,692,691,690,
  689,688,687,686,685,684,683,682,528,526,523,522,521,520,519,513,511,509,
  481,459,458,456,431,429,428,427,426,425,1,0,51,52,
708,707,706,705,704,703,702,701,700,699,698,697,696,695,694,693,692,691,690,
  689,688,687,686,685,684,683,682,539,537,536,535,534,533,532,531,530,529,
  528,526,523,522,521,520,519,513,511,509,492,481,459,458,456,431,429,428,
  427,426,425,232,135,134,1,0,51,52,
708,707,706,705,704,703,702,701,700,699,698,697,696,695,694,693,692,691,690,
  689,688,687,686,685,684,683,682,539,537,536,535,534,533,532,531,530,529,
  528,527,526,523,522,521,520,519,513,511,509,492,481,459,458,456,431,429,
  428,427,426,425,232,135,134,1,0,51,52,
708,707,706,705,704,703,702,701,700,699,698,697,696,695,694,693,692,691,690,
  689,688,687,686,685,684,683,682,539,537,536,535,534,533,532,531,530,529,
  528,527,526,523,522,521,520,519,513,511,509,492,481,459,458,456,431,429,
  428,427,426,425,232,135,134,1,0,51,52,
708,707,706,705,704,703,702,701,700,699,698,697,696,695,694,693,692,691,690,
  689,688,687,686,685,684,683,682,539,537,536,535,534,533,532,531,530,529,
  528,527,526,523,522,521,520,519,513,511,509,492,481,459,458,456,431,429,
  428,427,426,425,232,135,134,1,0,51,52,
708,707,706,705,704,703,702,701,700,699,698,697,696,695,694,693,692,691,690,
  689,688,687,686,685,684,683,682,539,537,536,535,534,533,532,531,530,529,
  528,526,523,522,521,520,519,513,511,509,492,481,459,458,456,431,429,428,
  427,426,425,232,135,134,1,0,51,52,
708,707,706,705,704,703,702,701,700,699,698,697,696,695,694,693,692,691,690,
  689,688,687,686,685,684,683,682,546,528,526,525,524,523,522,521,520,519,
  518,517,516,515,514,513,512,511,510,509,508,507,506,505,504,503,502,501,
  500,499,498,497,496,494,481,459,458,456,431,429,428,427,426,425,1,0,51,
  52,
708,707,706,705,704,703,702,701,700,699,698,697,696,695,694,693,692,691,690,
  689,688,687,686,685,684,683,682,528,526,523,522,521,520,519,513,511,509,
  481,459,458,456,431,429,428,427,426,425,1,0,51,52,
708,707,706,705,704,703,702,701,700,699,698,697,696,695,694,693,692,691,690,
  689,688,687,686,685,684,683,682,539,537,536,535,534,533,532,531,530,529,
  528,527,526,523,522,521,520,519,513,511,509,492,481,459,458,456,431,429,
  428,427,426,425,232,135,134,1,0,51,52,
708,707,706,705,704,703,702,701,700,699,698,697,696,695,694,693,692,691,690,
  689,688,687,686,685,684,683,682,539,537,536,535,534,533,532,531,530,529,
  528,526,523,522,521,520,519,513,511,509,492,481,459,458,456,431,429,428,
  427,426,425,232,135,134,1,0,51,52,
708,707,706,705,704,703,702,701,700,699,698,697,696,695,694,693,692,691,690,
  689,688,687,686,685,684,683,682,528,526,523,522,521,520,519,513,511,509,
  481,459,458,456,431,429,428,427,426,425,1,0,51,52,
708,707,706,705,704,703,702,701,700,699,698,697,696,695,694,693,692,691,690,
  689,688,687,686,685,684,683,682,528,526,523,522,521,520,519,513,511,509,
  481,458,456,431,429,428,427,425,1,0,51,52,
708,707,706,705,704,703,702,701,700,699,698,697,696,695,694,693,692,691,690,
  689,688,687,686,685,684,683,682,528,526,523,522,521,520,519,513,511,509,
  481,459,458,456,431,429,428,427,426,425,1,0,51,52,
708,707,706,705,704,703,702,701,700,699,698,697,696,695,694,693,692,691,690,
  689,688,687,686,685,684,683,682,528,526,523,522,521,520,519,513,511,509,
  481,459,458,456,431,429,428,427,426,425,1,0,51,52,
708,707,706,705,704,703,702,701,700,699,698,697,696,695,694,693,692,691,690,
  689,688,687,686,685,684,683,682,528,526,523,522,521,520,519,513,511,509,
  481,458,456,439,435,434,431,429,428,427,426,425,164,163,162,135,134,38,
  1,0,51,52,
708,707,706,705,704,703,702,701,700,699,698,697,696,695,694,693,692,691,690,
  689,688,687,686,685,684,683,682,528,526,523,522,521,520,519,513,511,509,
  481,458,456,431,427,1,0,46,54,86,88,122,140,141,143,145,146,147,148,149,
  150,154,157,158,186,188,190,196,197,198,200,201,204,206,221,225,226,230,
  414,415,416,417,418,419,420,421,422,423,424,
425,0,45,
425,0,45,
492,491,490,489,488,487,485,484,483,482,480,479,478,477,476,475,474,473,472,
  471,470,469,468,467,466,465,464,463,462,461,460,446,445,444,442,440,433,
  432,0,56,58,68,69,70,71,72,95,96,97,98,99,100,101,102,103,104,105,106,
  107,108,109,110,111,112,114,115,116,117,118,119,120,121,123,124,125,126,
  127,128,129,130,131,
429,427,426,425,1,0,51,52,
495,492,429,427,426,425,0,22,39,40,41,45,46,47,49,131,165,
670,545,544,543,249,247,239,237,235,0,246,248,250,251,255,256,257,
542,242,241,240,239,238,237,236,235,0,4,243,596,
670,545,249,247,239,237,235,0,5,246,248,250,610,
690,689,688,687,0,346,
481,0,122,
481,0,122,
481,0,122,
707,705,704,703,702,701,700,699,698,697,696,695,694,691,690,689,688,687,686,
  685,539,537,536,535,534,533,532,531,530,529,528,527,526,523,520,519,492,
  458,431,232,135,134,0,2,6,7,8,9,15,17,21,23,24,25,26,27,28,29,57,86,89,
  131,133,197,199,204,205,206,207,208,209,210,211,212,213,214,215,216,217,
  218,219,220,443,493,540,
707,705,704,703,702,701,700,699,698,697,696,695,694,691,690,689,688,687,686,
  685,539,537,536,535,534,533,532,531,530,529,528,527,526,523,520,519,492,
  458,431,232,135,134,0,2,6,7,8,9,15,17,21,23,24,25,26,27,28,29,57,86,89,
  131,133,197,199,204,205,206,207,208,209,210,211,212,213,214,215,216,217,
  218,219,220,443,493,540,
707,705,704,703,702,701,700,699,698,697,696,695,694,691,690,689,688,687,686,
  685,539,537,536,535,534,533,532,531,530,529,528,527,526,523,520,519,492,
  458,431,232,135,134,0,2,6,7,8,9,15,17,21,23,24,25,26,27,28,29,57,86,89,
  131,133,197,199,204,205,206,207,208,209,210,211,212,213,214,215,216,217,
  218,219,220,443,493,540,
707,705,704,703,702,701,700,699,698,697,696,695,694,691,690,689,688,687,686,
  685,539,537,536,535,534,533,532,531,530,529,528,527,526,523,520,519,492,
  458,431,232,135,134,0,2,6,7,8,9,15,17,21,23,24,25,26,27,28,29,57,86,89,
  131,133,197,199,204,205,206,207,208,209,210,211,212,213,214,215,216,217,
  218,219,220,443,493,540,
707,705,704,703,702,701,700,699,698,697,696,695,694,691,690,689,688,687,686,
  685,539,537,536,535,534,533,532,531,530,529,528,527,526,523,520,519,492,
  458,431,232,135,134,0,2,6,7,8,9,15,17,21,23,24,25,26,27,28,29,57,86,89,
  131,133,197,199,204,205,206,207,208,209,210,211,212,213,214,215,216,217,
  218,219,220,443,493,540,
707,705,704,703,702,701,700,699,698,697,696,695,694,691,690,689,688,687,686,
  685,539,537,536,535,534,533,532,531,530,529,528,527,526,523,520,519,492,
  458,431,232,135,134,0,2,6,7,8,9,15,17,21,23,24,25,26,27,28,29,57,86,89,
  131,133,197,199,204,205,206,207,208,209,210,211,212,213,214,215,216,217,
  218,219,220,443,493,540,
707,705,704,703,702,701,700,699,698,697,696,695,694,691,690,689,688,687,686,
  685,539,537,536,535,534,533,532,531,530,529,528,527,526,523,520,519,492,
  458,431,232,135,134,0,2,6,7,8,9,15,17,21,23,24,25,26,27,28,29,57,86,89,
  131,133,197,199,204,205,206,207,208,209,210,211,212,213,214,215,216,217,
  218,219,220,443,493,540,
707,705,704,703,702,701,700,699,698,697,696,695,694,691,690,689,688,687,686,
  685,539,537,536,535,534,533,532,531,530,529,528,527,526,523,520,519,492,
  458,431,232,135,134,0,2,6,7,8,9,15,17,21,23,24,25,26,27,28,29,57,86,89,
  131,133,197,199,204,205,206,207,208,209,210,211,212,213,214,215,216,217,
  218,219,220,443,493,540,
707,705,704,703,702,701,700,699,698,697,696,695,694,691,690,689,688,687,686,
  685,539,537,536,535,534,533,532,531,530,529,528,527,526,523,520,519,492,
  458,431,232,135,134,0,2,6,7,8,9,15,17,21,23,24,25,26,27,28,29,57,86,89,
  131,133,197,199,204,205,206,207,208,209,210,211,212,213,214,215,216,217,
  218,219,220,443,493,540,
707,705,704,703,702,701,700,699,698,697,696,695,694,691,690,689,688,687,686,
  685,539,537,536,535,534,533,532,531,530,529,528,527,526,523,520,519,492,
  458,431,232,135,134,0,2,6,7,8,9,15,17,21,23,24,25,26,27,28,29,57,86,89,
  131,133,197,199,204,205,206,207,208,209,210,211,212,213,214,215,216,217,
  218,219,220,443,493,540,
707,705,704,703,702,701,700,699,698,697,696,695,694,691,690,689,688,687,686,
  685,539,537,536,535,534,533,532,531,530,529,528,527,526,523,520,519,492,
  458,431,232,135,134,0,2,6,7,8,9,15,17,21,23,24,25,26,27,28,29,57,86,89,
  131,133,197,199,204,205,206,207,208,209,210,211,212,213,214,215,216,217,
  218,219,220,443,493,540,
707,705,704,703,702,701,700,699,698,697,696,695,694,691,690,689,688,687,686,
  685,539,537,536,535,534,533,532,531,530,529,528,527,526,523,520,519,492,
  458,431,232,135,134,0,2,6,7,8,9,15,17,21,23,24,25,26,27,28,29,57,86,89,
  131,133,197,199,204,205,206,207,208,209,210,211,212,213,214,215,216,217,
  218,219,220,443,493,540,
707,705,704,703,702,701,700,699,698,697,696,695,694,691,690,689,688,687,686,
  685,539,537,536,535,534,533,532,531,530,529,528,527,526,523,520,519,492,
  458,431,232,135,134,0,2,6,7,8,9,15,17,21,23,24,25,26,27,28,29,57,86,89,
  131,133,197,199,204,205,206,207,208,209,210,211,212,213,214,215,216,217,
  218,219,220,443,493,540,
707,705,704,703,702,701,700,699,698,697,696,695,694,691,690,689,688,687,686,
  685,539,537,536,535,534,533,532,531,530,529,528,527,526,523,520,519,492,
  458,431,232,135,134,0,2,6,7,8,9,15,17,21,23,24,25,26,27,28,29,57,86,89,
  131,133,197,199,204,205,206,207,208,209,210,211,212,213,214,215,216,217,
  218,219,220,443,493,540,
707,705,704,703,702,701,700,699,698,697,696,695,694,691,690,689,688,687,686,
  685,539,537,536,535,534,533,532,531,530,529,528,527,526,523,520,519,492,
  458,431,232,135,134,0,2,6,7,8,9,15,17,21,23,24,25,26,27,28,29,57,86,89,
  131,133,197,199,204,205,206,207,208,209,210,211,212,213,214,215,216,217,
  218,219,220,443,493,540,
707,705,704,703,702,701,700,699,698,697,696,695,694,691,690,689,688,687,686,
  685,539,537,536,535,534,533,532,531,530,529,528,527,526,523,520,519,492,
  458,431,232,135,134,0,2,6,7,8,9,15,17,21,23,24,25,26,27,28,29,57,86,89,
  131,133,197,199,204,205,206,207,208,209,210,211,212,213,214,215,216,217,
  218,219,220,443,493,540,
707,705,704,703,702,701,700,699,698,697,696,695,694,691,690,689,688,687,686,
  685,539,537,536,535,534,533,532,531,530,529,528,527,526,523,520,519,492,
  458,431,232,135,134,0,2,6,7,8,9,15,17,21,23,24,25,26,27,28,29,57,86,89,
  131,133,197,199,204,205,206,207,208,209,210,211,212,213,214,215,216,217,
  218,219,220,443,493,540,
707,705,704,703,702,701,700,699,698,697,696,695,694,691,690,689,688,687,686,
  685,539,537,536,535,534,533,532,531,530,529,528,527,526,523,520,519,492,
  458,431,232,135,134,0,2,6,7,8,9,15,17,21,23,24,25,26,27,28,29,57,86,89,
  131,133,197,199,204,205,206,207,208,209,210,211,212,213,214,215,216,217,
  218,219,220,443,493,540,
707,705,704,703,702,701,700,699,698,697,696,695,694,691,690,689,688,687,686,
  685,539,537,536,535,534,533,532,531,530,529,528,527,526,523,520,519,492,
  458,431,232,135,134,0,2,6,7,8,9,15,17,21,23,24,25,26,27,28,29,57,86,89,
  131,133,197,199,204,205,206,207,208,209,210,211,212,213,214,215,216,217,
  218,219,220,443,493,540,
707,705,704,703,702,701,700,699,698,697,696,695,694,691,690,689,688,687,686,
  685,539,537,536,535,534,533,532,531,530,529,528,527,526,523,520,519,492,
  458,431,232,135,134,0,2,6,7,8,9,15,17,21,23,24,25,26,27,28,29,57,86,89,
  131,133,197,199,204,205,206,207,208,209,210,211,212,213,214,215,216,217,
  218,219,220,443,493,540,
707,705,704,703,702,701,700,699,698,697,696,695,694,691,690,689,688,687,686,
  685,539,537,536,535,534,533,532,531,530,529,528,527,526,523,520,519,492,
  458,431,232,135,134,0,2,6,7,8,9,15,17,21,23,24,25,26,27,28,29,57,86,89,
  131,133,197,199,204,205,206,207,208,209,210,211,212,213,214,215,216,217,
  218,219,220,443,493,540,
707,705,704,703,702,701,700,699,698,697,696,695,694,691,690,689,688,687,686,
  685,539,537,536,535,534,533,532,531,530,529,528,527,526,523,520,519,492,
  458,431,232,135,134,0,2,6,7,8,9,15,17,21,23,24,25,26,27,28,29,57,86,89,
  131,133,197,199,204,205,206,207,208,209,210,211,212,213,214,215,216,217,
  218,219,220,443,493,540,
707,705,704,703,702,701,700,699,698,697,696,695,694,691,690,689,688,687,686,
  685,539,537,536,535,534,533,532,531,530,529,528,527,526,523,520,519,492,
  458,431,232,135,134,0,2,6,7,8,9,15,17,21,23,24,25,26,27,28,29,57,86,89,
  131,133,197,199,204,205,206,207,208,209,210,211,212,213,214,215,216,217,
  218,219,220,443,493,540,
707,705,704,703,702,701,700,699,698,697,696,695,694,691,690,689,688,687,686,
  685,539,537,536,535,534,533,532,531,530,529,528,527,526,523,520,519,492,
  458,431,232,135,134,0,2,6,7,8,9,15,17,21,23,24,25,26,27,28,29,57,86,89,
  131,133,197,199,204,205,206,207,208,209,210,211,212,213,214,215,216,217,
  218,219,220,443,493,540,
707,705,704,703,702,701,700,699,698,697,696,695,694,691,690,689,688,687,686,
  685,539,537,536,535,534,533,532,531,530,529,528,527,526,523,520,519,492,
  458,431,232,135,134,0,2,6,7,8,9,15,17,21,23,24,25,26,27,28,29,57,86,89,
  131,133,197,199,204,205,206,207,208,209,210,211,212,213,214,215,216,217,
  218,219,220,443,493,540,
707,705,704,703,702,701,700,699,698,697,696,695,694,691,690,689,688,687,686,
  685,539,537,536,535,534,533,532,531,530,529,528,527,526,523,520,519,492,
  458,431,232,135,134,0,2,6,7,8,9,15,17,21,23,24,25,26,27,28,29,57,86,89,
  131,133,197,199,204,205,206,207,208,209,210,211,212,213,214,215,216,217,
  218,219,220,443,493,540,
707,705,704,703,702,701,700,699,698,697,696,695,694,691,690,689,688,687,686,
  685,539,537,536,535,534,533,532,531,530,529,528,527,526,523,520,519,492,
  458,431,232,135,134,0,2,6,7,8,9,15,17,21,23,24,25,26,27,28,29,57,86,89,
  131,133,197,199,204,205,206,207,208,209,210,211,212,213,214,215,216,217,
  218,219,220,443,493,540,
707,705,704,703,702,701,700,699,698,697,696,695,694,691,690,689,688,687,686,
  685,539,537,536,535,534,533,532,531,530,529,528,527,526,523,520,519,492,
  458,431,232,135,134,0,2,6,7,8,9,15,17,21,23,24,25,26,27,28,29,57,86,89,
  131,133,197,199,204,205,206,207,208,209,210,211,212,213,214,215,216,217,
  218,219,220,443,493,540,
707,705,704,703,702,701,700,699,698,697,696,695,694,691,690,689,688,687,686,
  685,539,537,536,535,534,533,532,531,530,529,528,527,526,523,520,519,492,
  458,431,232,135,134,0,2,6,7,8,9,15,17,21,23,24,25,26,27,28,29,57,86,89,
  131,133,197,199,204,205,206,207,208,209,210,211,212,213,214,215,216,217,
  218,219,220,443,493,540,
707,705,704,703,702,701,700,699,698,697,696,695,694,691,690,689,688,687,686,
  685,539,537,536,535,534,533,532,531,530,529,528,527,526,523,520,519,492,
  458,431,232,135,134,0,2,6,7,8,9,15,17,21,23,24,25,26,27,28,29,57,86,89,
  131,133,197,199,204,205,206,207,208,209,210,211,212,213,214,215,216,217,
  218,219,220,443,493,540,
707,705,704,703,702,701,700,699,698,697,696,695,694,691,690,689,688,687,686,
  685,539,537,536,535,534,533,532,531,530,529,528,527,526,523,520,519,492,
  458,431,232,135,134,0,2,6,7,8,9,15,17,21,23,24,25,26,27,28,29,57,86,89,
  131,133,197,199,204,205,206,207,208,209,210,211,212,213,214,215,216,217,
  218,219,220,443,493,540,
707,705,704,703,702,701,700,699,698,697,696,695,694,691,690,689,688,687,686,
  685,539,537,536,535,534,533,532,531,530,529,528,527,526,523,520,519,492,
  458,431,232,135,134,0,2,6,7,8,9,15,17,21,23,24,25,26,27,28,29,57,86,89,
  131,133,197,199,204,205,206,207,208,209,210,211,212,213,214,215,216,217,
  218,219,220,443,493,540,
707,705,704,703,702,701,700,699,698,697,696,695,694,691,690,689,688,687,686,
  685,539,537,536,535,534,533,532,531,530,529,528,527,526,523,520,519,492,
  458,431,232,135,134,0,2,6,7,8,9,15,17,21,23,24,25,26,27,28,29,57,86,89,
  131,133,197,199,204,205,206,207,208,209,210,211,212,213,214,215,216,217,
  218,219,220,443,493,540,
707,705,704,703,702,701,700,699,698,697,696,695,694,691,690,689,688,687,686,
  685,539,537,536,535,534,533,532,531,530,529,528,527,526,523,520,519,492,
  458,431,232,135,134,0,2,6,7,8,9,15,17,21,23,24,25,26,27,28,29,57,86,89,
  131,133,197,199,204,205,206,207,208,209,210,211,212,213,214,215,216,217,
  218,219,220,443,493,540,
707,705,704,703,702,701,700,699,698,697,696,695,694,691,690,689,688,687,686,
  685,539,537,536,535,534,533,532,531,530,529,528,527,526,523,520,519,492,
  458,431,232,135,134,0,2,6,7,8,9,15,17,21,23,24,25,26,27,28,29,57,86,89,
  131,133,197,199,204,205,206,207,208,209,210,211,212,213,214,215,216,217,
  218,219,220,443,493,540,
707,705,704,703,702,701,700,699,698,697,696,695,694,691,690,689,688,687,686,
  685,539,537,536,535,534,533,532,531,530,529,528,527,526,523,520,519,492,
  458,431,232,135,134,0,2,6,7,8,9,15,17,21,23,24,25,26,27,28,29,57,86,89,
  131,133,197,199,204,205,206,207,208,209,210,211,212,213,214,215,216,217,
  218,219,220,443,493,540,
707,705,704,703,702,701,700,699,698,697,696,695,694,691,690,689,688,687,686,
  685,539,537,536,535,534,533,532,531,530,529,528,527,526,523,520,519,492,
  458,431,232,135,134,0,2,6,7,8,9,15,17,21,23,24,25,26,27,28,29,57,86,89,
  131,133,197,199,204,205,206,207,208,209,210,211,212,213,214,215,216,217,
  218,219,220,443,493,540,
707,705,704,703,702,701,700,699,698,697,696,695,694,691,690,689,688,687,686,
  685,539,537,536,535,534,533,532,531,530,529,528,527,526,523,520,519,492,
  458,431,232,135,134,0,2,6,7,8,9,15,17,21,23,24,25,26,27,28,29,57,86,89,
  131,133,197,199,204,205,206,207,208,209,210,211,212,213,214,215,216,217,
  218,219,220,443,493,540,
707,705,704,703,702,701,700,699,698,697,696,695,694,691,690,689,688,687,686,
  685,539,537,536,535,534,533,532,531,530,529,528,527,526,523,520,519,492,
  458,431,232,135,134,0,2,6,7,8,9,15,17,21,23,24,25,26,27,28,29,57,86,89,
  131,133,197,199,204,205,206,207,208,209,210,211,212,213,214,215,216,217,
  218,219,220,443,493,540,
707,705,704,703,702,701,700,699,698,697,696,695,694,691,690,689,688,687,686,
  685,539,537,536,535,534,533,532,531,530,529,528,527,526,523,520,519,492,
  458,431,232,135,134,0,2,6,7,8,9,15,17,21,23,24,25,26,27,28,29,57,86,89,
  131,133,197,199,204,205,206,207,208,209,210,211,212,213,214,215,216,217,
  218,219,220,443,493,540,
707,705,704,703,702,701,700,699,698,697,696,695,694,691,690,689,688,687,686,
  685,539,537,536,535,534,533,532,531,530,529,528,527,526,523,520,519,492,
  458,431,232,135,134,0,2,6,7,8,9,15,17,21,23,24,25,26,27,28,29,57,86,89,
  131,133,197,199,204,205,206,207,208,209,210,211,212,213,214,215,216,217,
  218,219,220,443,493,540,
707,705,704,703,702,701,700,699,698,697,696,695,694,691,690,689,688,687,686,
  685,539,537,536,535,534,533,532,531,530,529,528,527,526,523,520,519,492,
  458,431,232,135,134,0,2,6,7,8,9,15,17,21,23,24,25,26,27,28,29,57,86,89,
  131,133,197,199,204,205,206,207,208,209,210,211,212,213,214,215,216,217,
  218,219,220,443,493,540,
707,705,704,703,702,701,700,699,698,697,696,695,694,691,690,689,688,687,686,
  685,539,537,536,535,534,533,532,531,530,529,528,527,526,523,520,519,492,
  458,431,232,135,134,0,2,6,7,8,9,15,17,21,23,24,25,26,27,28,29,57,86,89,
  131,133,197,199,204,205,206,207,208,209,210,211,212,213,214,215,216,217,
  218,219,220,443,493,540,
707,705,704,703,702,701,700,699,698,697,696,695,694,691,690,689,688,687,686,
  685,539,537,536,535,534,533,532,531,530,529,528,527,526,523,520,519,492,
  458,431,232,135,134,0,2,6,7,8,9,15,17,21,23,24,25,26,27,28,29,57,86,89,
  131,133,197,199,204,205,206,207,208,209,210,211,212,213,214,215,216,217,
  218,219,220,443,493,540,
707,705,704,703,702,701,700,699,698,697,696,695,694,691,690,689,688,687,686,
  685,539,537,536,535,534,533,532,531,530,529,528,527,526,523,520,519,492,
  458,431,232,135,134,0,2,6,7,8,9,15,17,21,23,24,25,26,27,28,29,57,86,89,
  131,133,197,199,204,205,206,207,208,209,210,211,212,213,214,215,216,217,
  218,219,220,443,493,540,
707,705,704,703,702,701,700,699,698,697,696,695,694,691,690,689,688,687,686,
  685,539,537,536,535,534,533,532,531,530,529,528,527,526,523,520,519,492,
  458,431,232,135,134,0,2,6,7,8,9,15,17,21,23,24,25,26,27,28,29,57,86,89,
  131,133,197,199,204,205,206,207,208,209,210,211,212,213,214,215,216,217,
  218,219,220,443,493,540,
707,705,704,703,702,701,700,699,698,697,696,695,694,691,690,689,688,687,686,
  685,539,537,536,535,534,533,532,531,530,529,528,527,526,523,520,519,492,
  458,431,232,135,134,0,2,6,7,8,9,15,17,21,23,24,25,26,27,28,29,57,86,89,
  131,133,197,199,204,205,206,207,208,209,210,211,212,213,214,215,216,217,
  218,219,220,443,493,540,
707,705,704,703,702,701,700,699,698,697,696,695,694,691,690,689,688,687,686,
  685,539,537,536,535,534,533,532,531,530,529,528,527,526,523,520,519,492,
  458,431,232,135,134,0,2,6,7,8,9,15,17,21,23,24,25,26,27,28,29,57,86,89,
  131,133,197,199,204,205,206,207,208,209,210,211,212,213,214,215,216,217,
  218,219,220,443,493,540,
707,705,704,703,702,701,700,699,698,697,696,695,694,691,690,689,688,687,686,
  685,539,537,536,535,534,533,532,531,530,529,528,527,526,523,520,519,492,
  458,431,232,135,134,0,2,6,7,8,9,15,17,21,23,24,25,26,27,28,29,57,86,89,
  131,133,197,199,204,205,206,207,208,209,210,211,212,213,214,215,216,217,
  218,219,220,443,493,540,
707,705,704,703,702,701,700,699,698,697,696,695,694,691,690,689,688,687,686,
  685,539,537,536,535,534,533,532,531,530,529,528,527,526,523,520,519,492,
  458,431,232,135,134,0,2,6,7,8,9,15,17,21,23,24,25,26,27,28,29,57,86,89,
  131,133,197,199,204,205,206,207,208,209,210,211,212,213,214,215,216,217,
  218,219,220,443,493,540,
707,705,704,703,702,701,700,699,698,697,696,695,694,691,690,689,688,687,686,
  685,539,537,536,535,534,533,532,531,530,529,528,527,526,523,520,519,492,
  458,431,232,135,134,0,2,6,7,8,9,15,17,21,23,24,25,26,27,28,29,57,86,89,
  131,133,197,199,204,205,206,207,208,209,210,211,212,213,214,215,216,217,
  218,219,220,443,493,540,
707,705,704,703,702,701,700,699,698,697,696,695,694,691,690,689,688,687,686,
  685,539,537,536,535,534,533,532,531,530,529,528,527,526,523,520,519,492,
  458,431,232,135,134,0,2,6,7,8,9,15,17,21,23,24,25,26,27,28,29,57,86,89,
  131,133,197,199,204,205,206,207,208,209,210,211,212,213,214,215,216,217,
  218,219,220,443,493,540,
670,542,541,249,247,239,237,235,0,244,246,248,250,252,253,
670,542,541,249,247,239,237,235,0,244,246,248,250,252,253,
670,542,541,249,247,239,237,235,0,244,246,248,250,252,253,
670,542,541,249,247,239,237,235,0,244,246,248,250,252,253,
670,247,237,235,0,246,248,254,
670,247,237,235,0,246,248,254,
670,545,249,247,239,237,235,0,5,246,248,250,610,
670,545,249,247,239,237,235,0,5,246,248,250,610,
670,545,249,247,239,237,235,0,5,246,248,250,610,
670,545,544,543,249,247,239,237,235,0,246,248,250,251,255,256,257,
670,545,544,543,249,247,239,237,235,0,246,248,250,251,255,256,257,
542,242,241,240,239,238,237,236,235,0,4,243,596,
542,242,241,240,239,238,237,236,235,0,4,243,596,
542,242,241,240,239,238,237,236,235,0,4,243,596,
542,242,241,240,239,238,237,236,235,0,4,243,596,
542,242,241,240,239,238,237,236,235,0,4,243,596,
542,242,241,240,239,238,237,236,235,0,4,243,596,
542,242,241,240,239,238,237,236,235,0,4,243,596,
542,242,241,240,239,238,237,236,235,0,4,243,596,
542,242,241,240,239,238,237,236,235,0,4,243,596,
542,242,241,240,239,238,237,236,235,0,4,243,596,
542,242,241,240,239,238,237,236,235,0,4,243,596,
707,705,704,703,702,701,700,699,698,697,696,695,694,691,690,689,688,687,686,
  685,683,539,537,536,535,534,533,532,531,530,529,528,527,526,523,520,519,
  492,458,431,232,135,134,0,2,6,7,8,9,10,12,15,17,18,20,21,23,24,25,26,27,
  28,29,57,86,89,90,131,133,197,199,204,205,206,207,208,209,210,211,212,
  213,214,215,216,217,218,219,220,436,443,486,493,540,
695,691,690,689,688,687,686,685,523,520,519,232,1,0,2,6,7,15,17,219,220,493,
681,680,679,678,677,676,675,674,673,672,671,670,669,668,667,666,665,664,663,
  662,661,660,659,658,657,656,655,654,653,652,651,650,649,648,647,646,645,
  644,643,642,641,640,639,638,637,636,635,634,633,632,631,630,629,628,627,
  626,625,624,623,622,621,620,619,618,617,616,615,614,613,612,611,609,608,
  607,606,605,604,603,602,601,600,599,598,597,595,594,593,592,591,590,589,
  588,587,586,585,584,583,582,581,580,579,578,577,576,575,574,573,572,571,
  570,569,568,567,566,565,564,563,562,561,560,559,558,557,556,555,554,553,
  552,551,550,549,548,547,1,0,51,52,
681,680,679,678,677,676,675,674,673,672,671,670,669,668,667,666,665,664,663,
  662,661,660,659,658,657,656,655,654,653,652,651,650,649,648,647,646,645,
  644,643,642,641,640,639,638,637,636,635,634,633,632,631,630,629,628,627,
  626,625,624,623,622,621,620,619,618,617,616,615,614,613,612,611,609,608,
  607,606,605,604,603,602,601,600,599,598,597,595,594,593,592,591,590,589,
  588,587,586,585,584,583,582,581,580,579,578,577,576,575,574,573,572,571,
  570,569,568,567,566,565,564,563,562,561,560,559,558,557,556,555,554,553,
  552,551,550,549,548,547,1,0,51,52,
707,705,704,703,702,701,700,699,698,697,696,694,685,135,134,1,0,51,52,
707,705,704,703,702,701,700,699,698,697,696,694,685,135,134,0,21,133,443,
707,706,705,704,703,702,701,700,699,698,697,696,691,690,689,688,687,522,431,
  1,0,51,52,
707,706,705,704,703,702,701,700,699,698,697,696,691,690,689,688,687,522,431,
  0,19,438,
429,427,426,425,1,0,51,52,
429,427,426,425,1,0,51,52,
686,683,1,0,51,52,
686,683,0,10,12,18,20,436,486,
429,427,426,425,1,0,51,52,
429,427,426,425,1,0,51,52,
707,705,704,703,702,701,700,699,698,697,696,694,685,429,427,426,425,135,134,
  1,0,51,52,
707,705,704,703,702,701,700,699,698,697,696,694,685,429,427,426,425,135,134,
  1,0,21,133,443,
683,1,0,51,52,
683,0,10,12,436,
707,705,704,703,702,701,700,699,698,697,696,695,694,691,690,689,688,687,686,
  685,683,539,537,536,535,534,533,532,531,530,529,528,527,526,523,520,519,
  492,458,431,232,135,134,1,0,51,52,
707,705,704,703,702,701,700,699,698,697,696,695,694,691,690,689,688,687,686,
  685,683,539,537,536,535,534,533,532,531,530,529,528,527,526,523,520,519,
  492,458,431,232,135,134,1,0,51,52,
707,705,704,703,702,701,700,699,698,697,696,695,694,691,690,689,688,687,686,
  685,683,539,537,536,535,534,533,532,531,530,529,528,527,526,523,520,519,
  492,458,431,232,135,134,1,0,51,52,
707,705,704,703,702,701,700,699,698,697,696,695,694,691,690,689,688,687,686,
  685,683,539,537,536,535,534,533,532,531,530,529,528,527,526,523,520,519,
  492,458,431,232,135,134,0,2,6,7,8,9,10,12,15,17,21,23,24,25,26,27,28,29,
  57,86,89,131,133,197,199,204,205,206,207,208,209,210,211,212,213,214,
  215,216,217,218,219,220,436,443,493,540,
707,705,704,703,702,701,700,699,698,697,696,695,694,691,690,689,688,687,686,
  685,539,537,536,535,534,533,532,531,530,529,528,527,526,523,520,519,492,
  458,431,232,135,134,1,0,51,52,
707,705,704,703,702,701,700,699,698,697,696,695,694,691,690,689,688,687,686,
  685,539,537,536,535,534,533,532,531,530,529,528,527,526,523,520,519,492,
  458,431,232,135,134,0,2,6,7,8,9,15,17,21,23,24,25,26,27,28,29,57,86,89,
  131,133,197,199,204,205,206,207,208,209,210,211,212,213,214,215,216,217,
  218,219,220,443,493,540,
707,705,704,703,702,701,700,699,698,697,696,694,685,135,134,1,0,51,52,
707,705,704,703,702,701,700,699,698,697,696,694,685,135,134,0,21,133,443,
707,705,704,703,702,701,700,699,698,697,696,694,685,135,134,1,0,51,52,
707,705,704,703,702,701,700,699,698,697,696,694,685,135,134,0,21,133,443,
707,705,704,703,702,701,700,699,698,697,696,694,685,135,134,1,0,51,52,
707,705,704,703,702,701,700,699,698,697,696,694,685,135,134,0,21,113,133,
  443,
707,705,704,703,702,701,700,699,698,697,696,694,685,135,134,1,0,51,52,
707,705,704,703,702,701,700,699,698,697,696,694,685,135,134,0,21,113,133,
  443,
707,705,704,703,702,701,700,699,698,697,696,694,685,135,134,1,0,51,52,
707,705,704,703,702,701,700,699,698,697,696,694,685,135,134,0,21,113,133,
  443,
707,705,704,703,702,701,700,699,698,697,696,695,694,691,690,689,688,687,686,
  685,683,539,537,536,535,534,533,532,531,530,529,528,527,526,523,520,519,
  492,458,431,232,135,134,1,0,51,52,
707,705,704,703,702,701,700,699,698,697,696,695,694,691,690,689,688,687,686,
  685,683,539,537,536,535,534,533,532,531,530,529,528,527,526,523,520,519,
  492,458,431,232,135,134,1,0,51,52,
707,705,704,703,702,701,700,699,698,697,696,695,694,691,690,689,688,687,686,
  685,683,539,537,536,535,534,533,532,531,530,529,528,527,526,523,520,519,
  492,458,431,232,135,134,0,2,6,7,8,9,10,12,15,17,18,20,21,23,24,25,26,27,
  28,29,57,86,89,90,131,133,197,199,204,205,206,207,208,209,210,211,212,
  213,214,215,216,217,218,219,220,436,443,486,493,540,
707,705,704,703,702,701,700,699,698,697,696,695,694,691,690,689,688,687,686,
  685,683,539,537,536,535,534,533,532,531,530,529,528,527,526,523,520,519,
  492,458,431,232,135,134,1,0,51,52,
707,705,704,703,702,701,700,699,698,697,696,695,694,691,690,689,688,687,686,
  685,683,539,537,536,535,534,533,532,531,530,529,528,527,526,523,520,519,
  492,458,431,232,135,134,1,0,51,52,
707,705,704,703,702,701,700,699,698,697,696,695,694,691,690,689,688,687,686,
  685,683,539,537,536,535,534,533,532,531,530,529,528,527,526,523,520,519,
  492,458,431,232,135,134,1,0,51,52,
707,705,704,703,702,701,700,699,698,697,696,695,694,691,690,689,688,687,686,
  685,683,539,537,536,535,534,533,532,531,530,529,528,527,526,523,520,519,
  492,458,431,232,135,134,0,2,6,7,8,9,10,12,15,17,18,20,21,23,24,25,26,27,
  28,29,57,86,89,90,131,133,197,199,204,205,206,207,208,209,210,211,212,
  213,214,215,216,217,218,219,220,436,443,486,493,540,
707,705,704,703,702,701,700,699,698,697,696,695,694,691,690,689,688,687,686,
  685,539,537,536,535,534,533,532,531,530,529,528,527,526,523,520,519,492,
  458,431,232,135,134,1,0,51,52,
707,705,704,703,702,701,700,699,698,697,696,695,694,691,690,689,688,687,686,
  685,539,537,536,535,534,533,532,531,530,529,528,527,526,523,520,519,492,
  458,431,232,135,134,1,0,51,52,
707,705,704,703,702,701,700,699,698,697,696,695,694,691,690,689,688,687,686,
  685,539,537,536,535,534,533,532,531,530,529,528,527,526,523,520,519,492,
  458,431,232,135,134,0,2,6,7,8,9,15,17,21,23,24,25,26,27,28,29,57,86,89,
  131,133,197,199,204,205,206,207,208,209,210,211,212,213,214,215,216,217,
  218,219,220,443,493,540,
429,427,426,425,1,0,51,52,
707,705,704,703,702,701,700,699,698,697,696,695,694,691,690,689,688,687,686,
  685,539,537,536,535,534,533,532,531,530,529,528,527,526,523,520,519,492,
  458,431,232,135,134,1,0,51,52,
707,705,704,703,702,701,700,699,698,697,696,695,694,691,690,689,688,687,686,
  685,539,537,536,535,534,533,532,531,530,529,528,527,526,523,520,519,492,
  458,431,232,135,134,1,0,51,52,
707,705,704,703,702,701,700,699,698,697,696,695,694,691,690,689,688,687,686,
  685,539,537,536,535,534,533,532,531,530,529,528,527,526,523,520,519,492,
  458,431,232,135,134,1,0,51,52,
707,705,704,703,702,701,700,699,698,697,696,695,694,691,690,689,688,687,686,
  685,539,537,536,535,534,533,532,531,530,529,528,527,526,523,520,519,492,
  458,431,232,135,134,0,2,6,7,8,9,15,17,21,23,24,25,26,27,28,29,57,86,89,
  131,133,197,199,204,205,206,207,208,209,210,211,212,213,214,215,216,217,
  218,219,220,443,493,540,
707,705,704,703,702,701,700,699,698,697,696,694,685,135,134,0,21,133,443,
495,492,429,427,426,425,1,0,51,52,
495,492,429,427,426,425,1,0,51,52,
429,427,426,425,0,39,40,41,45,46,47,49,
457,0,87,
708,707,705,704,703,702,701,700,699,698,697,696,695,694,693,692,691,690,689,
  688,687,686,685,683,682,528,526,523,522,521,520,519,513,511,509,481,458,
  456,431,427,0,46,54,85,86,88,122,140,141,145,146,147,148,149,150,154,
  157,158,186,188,190,196,197,198,200,201,204,206,221,225,226,230,414,415,
  417,418,419,420,421,422,423,424,
429,427,426,425,0,39,40,41,45,46,47,49,
706,705,704,703,702,699,697,689,688,687,685,683,425,0,155,
429,427,426,425,0,39,40,41,45,46,47,49,
429,427,426,425,0,39,40,41,45,46,47,49,
699,698,697,696,691,690,689,688,687,0,
706,704,703,702,687,686,0,
707,706,705,704,703,702,701,700,699,698,697,696,695,694,693,692,691,690,689,
  688,687,686,685,684,683,682,528,526,523,522,521,520,519,513,511,509,481,
  458,456,431,427,425,234,1,0,16,
701,699,698,697,696,691,690,689,688,687,546,525,524,523,522,521,520,519,518,
  517,516,515,514,513,512,511,510,509,508,507,506,505,504,503,502,501,500,
  499,498,497,496,494,456,429,427,426,425,1,0,
701,699,698,697,696,691,690,689,688,687,546,525,524,523,522,521,520,519,518,
  517,516,515,514,513,512,511,510,509,508,507,506,505,504,503,502,501,500,
  499,498,497,496,494,456,429,427,426,425,1,0,
458,0,86,
458,0,86,
458,0,86,
458,0,86,
458,0,86,
458,0,86,
458,0,86,
458,0,86,
458,0,86,
458,0,86,
458,0,86,
707,705,704,703,702,701,700,699,698,697,696,695,694,691,690,689,688,687,686,
  685,539,537,536,535,534,533,532,531,530,529,528,527,526,523,520,519,492,
  458,431,232,135,134,0,2,6,7,8,9,15,17,21,23,24,25,26,27,28,29,57,86,89,
  131,133,197,199,204,205,206,207,208,209,210,211,212,213,214,215,216,217,
  218,219,220,443,493,540,
691,690,689,688,687,0,
707,705,704,703,702,701,700,699,698,697,696,695,694,691,690,689,688,687,686,
  685,539,537,536,535,534,533,532,531,530,529,523,520,519,492,458,431,232,
  135,134,1,0,51,52,
707,705,704,703,702,701,700,699,698,697,696,695,694,691,690,689,688,687,686,
  685,539,537,536,535,534,533,532,531,530,529,523,520,519,492,458,431,232,
  135,134,1,0,51,52,
707,705,704,703,702,701,700,699,698,697,696,695,694,691,690,689,688,687,686,
  685,539,537,536,535,534,533,532,531,530,529,523,520,519,492,458,431,232,
  135,134,1,0,51,52,
707,705,704,703,702,701,700,699,698,697,696,695,694,691,690,689,688,687,686,
  685,539,537,536,535,534,533,532,531,530,529,523,520,519,492,458,431,232,
  135,134,1,0,51,52,
707,705,704,703,702,701,700,699,698,697,696,695,694,691,690,689,688,687,686,
  685,539,537,536,535,534,533,532,531,530,529,523,520,519,492,458,431,232,
  135,134,1,0,51,52,
707,705,704,703,702,701,700,699,698,697,696,695,694,691,690,689,688,687,686,
  685,539,537,536,535,534,533,532,531,530,529,523,520,519,492,458,431,232,
  135,134,1,0,51,52,
707,705,704,703,702,701,700,699,698,697,696,695,694,691,690,689,688,687,686,
  685,539,537,536,535,534,533,532,531,530,529,523,520,519,492,458,431,232,
  135,134,1,0,51,52,
707,705,704,703,702,701,700,699,698,697,696,695,694,691,690,689,688,687,686,
  685,539,537,536,535,534,533,532,531,530,529,528,527,526,523,520,519,492,
  458,431,232,135,134,1,0,51,52,
707,705,704,703,702,701,700,699,698,697,696,695,694,691,690,689,688,687,686,
  685,539,537,536,535,534,533,532,531,530,529,528,527,526,523,520,519,492,
  458,431,232,135,134,1,0,51,52,
707,705,704,703,702,701,700,699,698,697,696,695,694,691,690,689,688,687,686,
  685,539,537,536,535,534,533,532,531,530,529,528,527,526,523,520,519,492,
  458,431,232,135,134,1,0,51,52,
707,705,704,703,702,701,700,699,698,697,696,695,694,691,690,689,688,687,686,
  685,539,537,536,535,534,533,532,531,530,529,528,527,526,523,520,519,492,
  458,431,232,135,134,1,0,51,52,
707,705,704,703,702,701,700,699,698,697,696,695,694,691,690,689,688,687,686,
  685,539,537,536,535,534,533,532,531,530,529,528,527,526,523,520,519,492,
  458,431,232,135,134,1,0,51,52,
707,705,704,703,702,701,700,699,698,697,696,695,694,691,690,689,688,687,686,
  685,539,537,536,535,534,533,532,531,530,529,528,527,526,523,520,519,492,
  458,431,232,135,134,1,0,51,52,
707,705,704,703,702,701,700,699,698,697,696,695,694,691,690,689,688,687,686,
  685,539,537,536,535,534,533,532,531,530,529,528,527,526,523,520,519,492,
  458,431,232,135,134,1,0,51,52,
707,705,704,703,702,701,700,699,698,697,696,695,694,691,690,689,688,687,686,
  685,539,537,536,535,534,533,532,531,530,529,528,527,526,523,520,519,492,
  458,431,232,135,134,1,0,51,52,
707,705,704,703,702,701,700,699,698,697,696,695,694,691,690,689,688,687,686,
  685,539,537,536,535,534,533,532,531,530,529,528,527,526,523,520,519,492,
  458,431,232,135,134,1,0,51,52,
707,705,704,703,702,701,700,699,698,697,696,695,694,691,690,689,688,687,686,
  685,539,537,536,535,534,533,532,531,530,529,528,527,526,523,520,519,492,
  458,431,232,135,134,1,0,51,52,
707,705,704,703,702,701,700,699,698,697,696,695,694,691,690,689,688,687,686,
  685,539,537,536,535,534,533,532,531,530,529,528,527,526,523,520,519,492,
  458,431,232,135,134,1,0,51,52,
707,705,704,703,702,701,700,699,698,697,696,695,694,691,690,689,688,687,686,
  685,539,537,536,535,534,533,532,531,530,529,528,527,526,523,520,519,492,
  458,431,232,135,134,1,0,51,52,
707,705,704,703,702,701,700,699,698,697,696,695,694,691,690,689,688,687,686,
  685,539,537,536,535,534,533,532,531,530,529,528,527,526,523,520,519,492,
  458,431,232,135,134,1,0,51,52,
707,705,704,703,702,701,700,699,698,697,696,695,694,691,690,689,688,687,686,
  685,539,537,536,535,534,533,532,531,530,529,528,527,526,523,520,519,492,
  458,431,232,135,134,1,0,51,52,
707,705,704,703,702,701,700,699,698,697,696,695,694,691,690,689,688,687,686,
  685,539,537,536,535,534,533,532,531,530,529,528,527,526,523,520,519,492,
  458,431,232,135,134,1,0,51,52,
707,705,704,703,702,701,700,699,698,697,696,695,694,691,690,689,688,687,686,
  685,539,537,536,535,534,533,532,531,530,529,528,527,526,523,520,519,492,
  458,431,232,135,134,1,0,51,52,
707,705,704,703,702,701,700,699,698,697,696,695,694,691,690,689,688,687,686,
  685,539,537,536,535,534,533,532,531,530,529,528,527,526,523,520,519,492,
  458,431,232,135,134,1,0,51,52,
707,705,704,703,702,701,700,699,698,697,696,695,694,691,690,689,688,687,686,
  685,539,537,536,535,534,533,532,531,530,529,528,527,526,523,520,519,492,
  458,431,232,135,134,1,0,51,52,
707,705,704,703,702,701,700,699,698,697,696,695,694,691,690,689,688,687,686,
  685,539,537,536,535,534,533,532,531,530,529,528,527,526,523,520,519,492,
  458,431,232,135,134,1,0,51,52,
707,705,704,703,702,701,700,699,698,697,696,695,694,691,690,689,688,687,686,
  685,539,537,536,535,534,533,532,531,530,529,528,527,526,523,520,519,492,
  458,431,232,135,134,1,0,51,52,
707,705,704,703,702,701,700,699,698,697,696,695,694,691,690,689,688,687,686,
  685,539,537,536,535,534,533,532,531,530,529,528,527,526,523,520,519,492,
  458,431,232,135,134,1,0,51,52,
707,705,704,703,702,701,700,699,698,697,696,695,694,691,690,689,688,687,686,
  685,539,537,536,535,534,533,532,531,530,529,528,527,526,523,520,519,492,
  458,431,232,135,134,1,0,51,52,
707,705,704,703,702,701,700,699,698,697,696,695,694,691,690,689,688,687,686,
  685,539,537,536,535,534,533,532,531,530,529,528,527,526,523,520,519,492,
  458,431,232,135,134,1,0,51,52,
707,705,704,703,702,701,700,699,698,697,696,695,694,691,690,689,688,687,686,
  685,539,537,536,535,534,533,532,531,530,529,528,527,526,523,520,519,492,
  458,431,232,135,134,1,0,51,52,
707,705,704,703,702,701,700,699,698,697,696,695,694,691,690,689,688,687,686,
  685,539,537,536,535,534,533,532,531,530,529,528,527,526,523,520,519,492,
  458,431,232,135,134,1,0,51,52,
707,705,704,703,702,701,700,699,698,697,696,695,694,691,690,689,688,687,686,
  685,539,537,536,535,534,533,532,531,530,529,528,527,526,523,520,519,492,
  458,431,232,135,134,1,0,51,52,
707,705,704,703,702,701,700,699,698,697,696,695,694,691,690,689,688,687,686,
  685,539,537,536,535,534,533,532,531,530,529,528,527,526,523,520,519,492,
  458,431,232,135,134,1,0,51,52,
707,705,704,703,702,701,700,699,698,697,696,695,694,691,690,689,688,687,686,
  685,539,537,536,535,534,533,532,531,530,529,528,527,526,523,520,519,492,
  458,431,232,135,134,1,0,51,52,
707,705,704,703,702,701,700,699,698,697,696,695,694,691,690,689,688,687,686,
  685,539,537,536,535,534,533,532,531,530,529,528,527,526,523,520,519,492,
  458,431,232,135,134,1,0,51,52,
707,705,704,703,702,701,700,699,698,697,696,695,694,691,690,689,688,687,686,
  685,539,537,536,535,534,533,532,531,530,529,528,527,526,523,520,519,492,
  458,431,232,135,134,1,0,51,52,
707,705,704,703,702,701,700,699,698,697,696,695,694,691,690,689,688,687,686,
  685,539,537,536,535,534,533,532,531,530,529,528,527,526,523,520,519,492,
  458,431,232,135,134,1,0,51,52,
707,705,704,703,702,701,700,699,698,697,696,695,694,691,690,689,688,687,686,
  685,539,537,536,535,534,533,532,531,530,529,528,527,526,523,520,519,492,
  458,431,232,135,134,1,0,51,52,
429,427,426,425,0,39,40,41,45,46,47,49,
429,427,426,425,0,39,40,41,45,46,47,49,
429,427,426,425,0,39,40,41,45,46,47,49,
429,427,426,425,0,39,40,41,45,46,47,49,
429,427,426,425,0,39,40,41,45,46,47,49,
707,705,704,703,702,701,700,699,698,697,696,694,685,135,134,0,21,133,443,
429,427,426,425,0,39,40,41,45,46,47,49,
429,427,426,425,0,39,40,41,45,46,47,49,
708,707,706,705,704,703,702,701,700,699,698,697,696,695,694,693,692,691,690,
  689,688,687,686,685,684,683,682,528,526,523,522,521,520,519,513,511,509,
  481,458,456,431,427,425,1,0,
429,427,426,425,0,39,40,41,45,46,47,49,
429,427,426,425,0,39,40,41,45,46,47,49,
707,706,705,704,703,702,701,700,699,698,697,696,691,690,689,688,687,522,431,
  427,425,1,0,51,52,
429,427,426,425,1,0,51,52,
429,427,426,425,1,0,51,52,
429,427,426,425,1,0,51,52,
707,705,704,703,702,701,700,699,698,697,696,695,694,691,690,689,688,687,686,
  685,539,537,536,535,534,533,532,531,530,529,528,527,526,523,520,519,492,
  458,431,232,135,134,1,0,51,52,
707,705,704,703,702,701,700,699,698,697,696,695,694,691,690,689,688,687,686,
  685,539,537,536,535,534,533,532,531,530,529,528,527,526,523,520,519,492,
  458,431,232,135,134,0,2,6,7,8,9,15,17,21,23,24,25,26,27,28,29,57,86,89,
  131,133,197,199,204,205,206,207,208,209,210,211,212,213,214,215,216,217,
  218,219,220,443,493,540,
707,705,704,703,702,701,700,699,698,697,696,695,694,691,690,689,688,687,686,
  685,539,537,536,535,534,533,532,531,530,529,528,527,526,523,520,519,492,
  458,431,232,135,134,1,0,51,52,
707,705,704,703,702,701,700,699,698,697,696,695,694,691,690,689,688,687,686,
  685,539,537,536,535,534,533,532,531,530,529,528,527,526,523,520,519,492,
  458,431,232,135,134,0,2,6,7,8,9,15,17,21,23,24,25,26,27,28,29,57,86,89,
  131,133,197,199,204,205,206,207,208,209,210,211,212,213,214,215,216,217,
  218,219,220,443,493,540,
429,427,426,425,0,39,40,41,45,46,47,49,
429,427,426,425,1,0,51,52,
429,427,426,425,1,0,51,52,
546,481,429,427,426,425,1,0,51,52,
546,481,429,427,426,425,1,0,51,52,
546,481,429,427,426,425,1,0,51,52,
546,494,481,429,427,426,425,1,0,51,52,
546,481,429,427,426,425,1,0,51,52,
707,705,704,703,702,701,700,699,698,697,696,695,694,691,690,689,688,687,686,
  685,539,537,536,535,534,533,532,531,530,529,528,527,526,523,520,519,492,
  458,431,232,135,134,1,0,51,52,
707,705,704,703,702,701,700,699,698,697,696,695,694,691,690,689,688,687,686,
  685,539,537,536,535,534,533,532,531,530,529,528,527,526,523,520,519,492,
  458,431,232,135,134,1,0,51,52,
707,705,704,703,702,701,700,699,698,697,696,695,694,691,690,689,688,687,686,
  685,539,537,536,535,534,533,532,531,530,529,528,527,526,523,520,519,492,
  458,431,232,135,134,1,0,51,52,
546,429,427,426,425,1,0,51,52,
546,429,427,426,425,1,0,51,52,
494,1,0,51,52,
707,706,705,704,703,702,701,700,699,698,697,696,695,694,693,692,691,690,689,
  688,687,685,684,683,682,528,526,523,522,521,520,519,513,511,509,481,458,
  456,431,427,425,234,1,0,16,
707,706,705,704,703,702,701,700,699,698,697,696,695,694,693,692,691,690,689,
  688,687,686,685,684,683,682,528,526,523,522,521,520,519,513,511,509,481,
  458,456,431,427,425,234,1,0,16,
494,456,429,427,426,425,1,0,51,52,
494,456,0,88,132,
699,698,697,696,691,690,689,688,687,0,
707,701,699,698,697,696,691,690,689,688,687,685,429,427,426,425,223,222,1,0,
  224,
681,680,679,678,677,676,675,674,673,672,671,670,669,668,667,666,665,664,663,
  662,661,660,659,658,657,656,655,654,653,652,651,650,649,648,647,646,645,
  644,643,642,641,640,639,638,637,636,635,634,633,632,631,630,629,628,627,
  626,625,624,623,622,621,620,619,618,617,616,615,614,613,612,611,609,608,
  607,606,605,604,603,602,601,600,599,598,597,595,594,593,592,591,590,589,
  588,587,586,585,584,583,582,581,580,579,578,577,576,575,574,573,572,571,
  570,569,568,567,566,565,564,563,562,561,560,559,558,557,556,555,554,553,
  552,551,550,549,548,547,0,31,32,245,258,259,260,261,262,263,264,265,266,
  267,269,270,271,272,273,274,275,276,277,278,279,280,281,282,283,284,285,
  286,287,288,289,290,291,292,293,294,295,296,297,298,299,300,301,302,303,
  304,305,306,307,308,309,310,311,312,313,314,315,316,317,318,319,320,321,
  322,323,324,325,326,327,328,329,330,331,332,333,334,335,336,337,338,339,
  340,341,342,343,344,345,347,348,349,350,351,352,353,354,355,356,357,358,
  359,360,361,362,363,364,365,366,367,368,369,370,371,372,373,374,375,376,
  377,378,379,380,381,382,383,384,385,386,387,388,389,390,391,392,393,394,
  395,396,397,398,399,400,401,402,403,404,405,406,407,408,409,410,411,412,
  413,
707,706,705,704,703,702,701,700,699,698,697,696,695,694,693,692,691,690,689,
  688,687,685,684,683,682,528,526,523,522,521,520,519,513,511,509,481,458,
  456,431,427,425,234,1,0,16,
481,0,122,
707,705,704,703,702,701,700,699,698,697,696,695,694,691,690,689,688,687,686,
  685,683,539,537,536,535,534,533,532,531,530,529,528,527,526,523,520,519,
  492,458,431,232,135,134,0,2,6,7,8,9,10,12,15,17,18,20,21,23,24,25,26,27,
  28,29,57,86,89,90,131,133,197,199,204,205,206,207,208,209,210,211,212,
  213,214,215,216,217,218,219,220,436,443,486,493,540,
494,429,427,426,425,1,0,132,
494,429,427,426,425,1,0,132,
494,429,427,426,425,1,0,132,
494,429,427,426,425,1,0,132,
494,429,427,426,425,1,0,132,
456,1,0,51,52,
456,0,88,
708,707,705,704,703,702,701,700,699,698,697,696,695,694,693,692,691,690,689,
  688,687,686,685,683,682,528,526,523,522,521,520,519,513,511,509,481,459,
  458,456,431,427,425,1,0,46,51,52,54,86,88,93,122,140,141,145,146,147,
  148,149,150,154,157,158,186,188,190,196,197,198,200,201,204,206,221,225,
  226,230,414,415,417,418,419,420,421,422,423,424,
699,698,697,696,691,690,689,688,687,0,
691,690,689,688,687,0,
707,706,705,704,703,702,701,700,699,698,697,696,695,694,693,692,691,690,689,
  688,687,686,685,684,683,682,528,526,523,522,521,520,519,513,511,509,481,
  458,456,431,427,425,1,0,
686,0,
707,705,704,703,702,701,700,699,698,697,696,694,685,135,134,1,0,51,52,
707,705,704,703,702,701,700,699,698,697,696,695,694,691,690,689,688,687,686,
  685,539,537,536,535,534,533,532,531,530,529,528,527,526,523,520,519,492,
  458,431,232,135,134,0,2,6,7,8,9,15,17,21,23,24,25,26,27,28,29,57,86,89,
  131,133,197,199,204,205,206,207,208,209,210,211,212,213,214,215,216,217,
  218,219,220,443,493,540,
707,705,704,703,702,701,700,699,698,697,696,695,694,691,690,689,688,687,686,
  685,539,537,536,535,534,533,532,531,530,529,528,527,526,523,520,519,492,
  458,431,232,135,134,0,2,6,7,8,9,15,17,21,23,24,25,26,27,28,29,57,86,89,
  131,133,197,199,204,205,206,207,208,209,210,211,212,213,214,215,216,217,
  218,219,220,443,493,540,
707,705,704,703,702,701,700,699,698,697,696,695,694,691,690,689,688,687,686,
  685,539,537,536,535,534,533,532,531,530,529,528,527,526,523,520,519,492,
  458,431,232,135,134,0,2,6,7,8,9,15,17,21,23,24,25,26,27,28,29,57,86,89,
  131,133,197,199,204,205,206,207,208,209,210,211,212,213,214,215,216,217,
  218,219,220,443,493,540,
707,705,704,703,702,701,700,699,698,697,696,695,694,691,690,689,688,687,686,
  685,539,537,536,535,534,533,532,531,530,529,528,527,526,523,520,519,492,
  458,431,232,135,134,0,2,6,7,8,9,15,17,21,23,24,25,26,27,28,29,57,86,89,
  131,133,197,199,204,205,206,207,208,209,210,211,212,213,214,215,216,217,
  218,219,220,443,493,540,
707,705,704,703,702,701,700,699,698,697,696,695,694,691,690,689,688,687,686,
  685,539,537,536,535,534,533,532,531,530,529,528,527,526,523,520,519,492,
  458,431,232,135,134,0,2,6,7,8,9,15,17,21,23,24,25,26,27,28,29,57,86,89,
  131,133,197,199,204,205,206,207,208,209,210,211,212,213,214,215,216,217,
  218,219,220,443,493,540,
707,705,704,703,702,701,700,699,698,697,696,695,694,691,690,689,688,687,686,
  685,539,537,536,535,534,533,532,531,530,529,528,527,526,523,520,519,492,
  458,431,232,135,134,0,2,6,7,8,9,15,17,21,23,24,25,26,27,28,29,57,86,89,
  131,133,197,199,204,205,206,207,208,209,210,211,212,213,214,215,216,217,
  218,219,220,443,493,540,
707,705,704,703,702,701,700,699,698,697,696,695,694,691,690,689,688,687,686,
  685,539,537,536,535,534,533,532,531,530,529,528,527,526,523,520,519,492,
  458,431,232,135,134,0,2,6,7,8,9,15,17,21,23,24,25,26,27,28,29,57,86,89,
  131,133,197,199,204,205,206,207,208,209,210,211,212,213,214,215,216,217,
  218,219,220,443,493,540,
707,705,704,703,702,701,700,699,698,697,696,695,694,691,690,689,688,687,686,
  685,539,537,536,535,534,533,532,531,530,529,528,527,526,523,520,519,492,
  458,431,232,135,134,0,2,6,7,8,9,15,17,21,23,24,25,26,27,28,29,57,86,89,
  131,133,197,199,204,205,206,207,208,209,210,211,212,213,214,215,216,217,
  218,219,220,443,493,540,
707,705,704,703,702,701,700,699,698,697,696,695,694,691,690,689,688,687,686,
  685,539,537,536,535,534,533,532,531,530,529,528,527,526,523,520,519,492,
  458,431,232,135,134,0,2,6,7,8,9,15,17,21,23,24,25,26,27,28,29,57,86,89,
  131,133,197,199,204,205,206,207,208,209,210,211,212,213,214,215,216,217,
  218,219,220,443,493,540,
707,705,704,703,702,701,700,699,698,697,696,695,694,691,690,689,688,687,686,
  685,539,537,536,535,534,533,532,531,530,529,528,527,526,523,520,519,492,
  458,431,232,135,134,0,2,6,7,8,9,15,17,21,23,24,25,26,27,28,29,57,86,89,
  131,133,197,199,204,205,206,207,208,209,210,211,212,213,214,215,216,217,
  218,219,220,443,493,540,
456,1,0,51,52,
707,705,704,703,702,701,700,699,698,697,696,695,694,691,690,689,688,687,686,
  685,539,537,536,535,534,533,532,531,530,529,523,520,519,492,458,431,232,
  135,134,0,2,6,7,8,9,15,17,21,86,89,131,133,199,207,208,209,210,211,212,
  213,214,215,216,217,218,219,220,443,493,540,
707,705,704,703,702,701,700,699,698,697,696,695,694,691,690,689,688,687,686,
  685,539,537,536,535,534,533,532,531,530,529,523,520,519,492,458,431,232,
  135,134,0,2,6,7,8,9,15,17,21,86,89,131,133,199,207,208,209,210,211,212,
  213,214,215,216,217,218,219,220,443,493,540,
707,705,704,703,702,701,700,699,698,697,696,695,694,691,690,689,688,687,686,
  685,539,537,536,535,534,533,532,531,530,529,523,520,519,492,458,431,232,
  135,134,0,2,6,7,8,9,15,17,21,86,89,131,133,199,207,208,209,210,211,212,
  213,214,215,216,217,218,219,220,443,493,540,
707,705,704,703,702,701,700,699,698,697,696,695,694,691,690,689,688,687,686,
  685,539,537,536,535,534,533,532,531,530,529,523,520,519,492,458,431,232,
  135,134,0,2,6,7,8,9,15,17,21,86,89,131,133,199,207,208,209,210,211,212,
  213,214,215,216,217,218,219,220,443,493,540,
707,705,704,703,702,701,700,699,698,697,696,695,694,691,690,689,688,687,686,
  685,539,537,536,535,534,533,532,531,530,529,523,520,519,492,458,431,232,
  135,134,0,2,6,7,8,9,15,17,21,86,89,131,133,199,207,208,209,210,211,212,
  213,214,215,216,217,218,219,220,443,493,540,
707,705,704,703,702,701,700,699,698,697,696,695,694,691,690,689,688,687,686,
  685,539,537,536,535,534,533,532,531,530,529,528,527,526,523,520,519,492,
  458,431,232,135,134,0,2,6,7,8,9,15,17,21,25,29,86,89,131,133,197,199,
  204,205,206,207,208,209,210,211,212,213,214,215,216,217,218,219,220,443,
  493,540,
707,705,704,703,702,701,700,699,698,697,696,695,694,691,690,689,688,687,686,
  685,539,537,536,535,534,533,532,531,530,529,528,527,526,523,520,519,492,
  458,431,232,135,134,0,2,6,7,8,9,15,17,21,25,29,86,89,131,133,197,199,
  204,205,206,207,208,209,210,211,212,213,214,215,216,217,218,219,220,443,
  493,540,
707,705,704,703,702,701,700,699,698,697,696,695,694,691,690,689,688,687,686,
  685,539,537,536,535,534,533,532,531,530,529,528,527,526,523,520,519,492,
  458,431,232,135,134,0,2,6,7,8,9,15,17,21,25,27,29,86,89,131,133,197,199,
  204,205,206,207,208,209,210,211,212,213,214,215,216,217,218,219,220,443,
  493,540,
707,705,704,703,702,701,700,699,698,697,696,695,694,691,690,689,688,687,686,
  685,539,537,536,535,534,533,532,531,530,529,528,527,526,523,520,519,492,
  458,431,232,135,134,0,2,6,7,8,9,15,17,21,25,27,29,86,89,131,133,197,199,
  204,205,206,207,208,209,210,211,212,213,214,215,216,217,218,219,220,443,
  493,540,
707,705,704,703,702,701,700,699,698,697,696,695,694,691,690,689,688,687,686,
  685,539,537,536,535,534,533,532,531,530,529,528,527,526,523,520,519,492,
  458,431,232,135,134,0,2,6,7,8,9,15,17,21,25,27,29,86,89,131,133,197,199,
  204,205,206,207,208,209,210,211,212,213,214,215,216,217,218,219,220,443,
  493,540,
707,705,704,703,702,701,700,699,698,697,696,695,694,691,690,689,688,687,686,
  685,539,537,536,535,534,533,532,531,530,529,528,527,526,523,520,519,492,
  458,431,232,135,134,0,2,6,7,8,9,15,17,21,25,27,29,86,89,131,133,197,199,
  204,205,206,207,208,209,210,211,212,213,214,215,216,217,218,219,220,443,
  493,540,
707,705,704,703,702,701,700,699,698,697,696,695,694,691,690,689,688,687,686,
  685,539,537,536,535,534,533,532,531,530,529,528,527,526,523,520,519,492,
  458,431,232,135,134,0,2,6,7,8,9,15,17,21,24,25,27,29,86,89,131,133,197,
  199,204,205,206,207,208,209,210,211,212,213,214,215,216,217,218,219,220,
  443,493,540,
707,705,704,703,702,701,700,699,698,697,696,695,694,691,690,689,688,687,686,
  685,539,537,536,535,534,533,532,531,530,529,528,527,526,523,520,519,492,
  458,431,232,135,134,0,2,6,7,8,9,15,17,21,24,25,27,29,86,89,131,133,197,
  199,204,205,206,207,208,209,210,211,212,213,214,215,216,217,218,219,220,
  443,493,540,
707,705,704,703,702,701,700,699,698,697,696,695,694,691,690,689,688,687,686,
  685,539,537,536,535,534,533,532,531,530,529,528,527,526,523,520,519,492,
  458,431,232,135,134,0,2,6,7,8,9,15,17,21,23,24,25,27,29,86,89,131,133,
  197,199,204,205,206,207,208,209,210,211,212,213,214,215,216,217,218,219,
  220,443,493,540,
707,705,704,703,702,701,700,699,698,697,696,695,694,691,690,689,688,687,686,
  685,539,537,536,535,534,533,532,531,530,529,528,527,526,523,520,519,492,
  458,431,232,135,134,0,2,6,7,8,9,15,17,21,23,24,25,27,29,86,89,131,133,
  197,199,204,205,206,207,208,209,210,211,212,213,214,215,216,217,218,219,
  220,443,493,540,
707,705,704,703,702,701,700,699,698,697,696,695,694,691,690,689,688,687,686,
  685,539,537,536,535,534,533,532,531,530,529,528,527,526,523,520,519,492,
  458,431,232,135,134,0,2,6,7,8,9,15,17,21,23,24,25,27,28,29,86,89,131,
  133,197,199,204,205,206,207,208,209,210,211,212,213,214,215,216,217,218,
  219,220,443,493,540,
707,705,704,703,702,701,700,699,698,697,696,695,694,691,690,689,688,687,686,
  685,539,537,536,535,534,533,532,531,530,529,528,527,526,523,520,519,492,
  458,431,232,135,134,0,2,6,7,8,9,15,17,21,23,24,25,27,28,29,86,89,131,
  133,197,199,204,205,206,207,208,209,210,211,212,213,214,215,216,217,218,
  219,220,443,493,540,
429,427,426,425,1,0,51,52,
429,427,426,425,0,39,40,41,45,46,47,49,
429,427,426,425,0,39,40,41,45,46,47,49,
429,427,426,425,0,39,40,41,45,46,47,49,
429,427,426,425,1,0,51,52,
429,427,426,425,1,0,51,52,
429,427,426,425,0,39,40,41,45,46,47,49,
707,705,704,703,702,701,700,699,698,697,696,695,694,691,690,689,688,687,686,
  685,539,537,536,535,534,533,532,531,530,529,528,527,526,523,520,519,492,
  458,431,232,135,134,0,2,6,7,8,9,15,17,21,23,24,25,26,27,28,29,57,86,89,
  131,133,197,199,204,205,206,207,208,209,210,211,212,213,214,215,216,217,
  218,219,220,443,493,540,
707,705,704,703,702,701,700,699,698,697,696,695,694,691,690,689,688,687,686,
  685,539,537,536,535,534,533,532,531,530,529,528,527,526,523,520,519,492,
  458,431,232,135,134,0,2,6,7,8,9,15,17,21,23,24,25,26,27,28,29,57,86,89,
  131,133,197,199,204,205,206,207,208,209,210,211,212,213,214,215,216,217,
  218,219,220,443,493,540,
707,705,704,703,702,701,700,699,698,697,696,695,694,691,690,689,688,687,686,
  685,539,537,536,535,534,533,532,531,530,529,528,527,526,523,520,519,492,
  458,431,232,135,134,0,2,6,7,8,9,15,17,21,23,24,25,26,27,28,29,57,86,89,
  131,133,197,199,204,205,206,207,208,209,210,211,212,213,214,215,216,217,
  218,219,220,443,493,540,
494,0,132,
707,706,705,704,703,702,701,700,699,698,697,696,695,694,693,692,691,690,689,
  688,687,686,685,684,683,682,528,526,523,522,521,520,519,513,511,509,481,
  458,456,431,427,425,234,1,0,16,
707,705,704,703,702,701,700,699,698,697,696,695,694,691,690,689,688,687,686,
  685,683,542,539,537,536,535,534,533,532,531,530,529,528,527,526,523,520,
  519,492,458,431,242,241,240,239,238,237,236,235,232,135,134,1,0,51,52,
707,705,704,703,702,701,700,699,698,697,696,695,694,691,690,689,688,687,686,
  685,683,539,537,536,535,534,533,532,531,530,529,528,527,526,523,520,519,
  492,458,431,232,135,134,1,0,51,52,
546,0,268,
546,429,427,426,425,1,0,51,52,
707,706,705,704,703,702,701,700,699,698,697,696,695,694,693,692,691,690,689,
  688,687,685,684,683,682,528,526,523,522,521,520,519,513,511,509,481,458,
  456,431,427,425,234,1,0,16,
707,705,704,703,702,701,700,699,698,697,696,695,694,691,690,689,688,687,686,
  685,539,537,536,535,534,533,532,531,530,529,528,527,526,523,520,519,492,
  458,431,232,135,134,1,0,51,52,
494,429,427,426,425,1,0,132,
707,705,704,703,702,701,700,699,698,697,696,694,685,135,134,1,0,51,52,
708,707,705,704,703,702,701,700,699,698,697,696,695,694,693,692,691,690,689,
  688,687,686,685,683,682,528,526,523,522,521,520,519,513,511,509,481,459,
  458,456,431,429,427,426,425,1,0,51,52,
708,707,705,704,703,702,701,700,699,698,697,696,695,694,693,692,691,690,689,
  688,687,686,685,683,682,528,526,523,522,521,520,519,513,511,509,481,459,
  458,456,431,429,427,426,425,1,0,
429,427,426,425,0,39,40,41,45,46,47,49,
707,706,705,704,703,702,701,700,699,698,697,696,695,694,693,692,691,690,689,
  688,687,686,685,684,683,682,528,526,523,522,521,520,519,513,511,509,481,
  458,456,431,427,425,1,0,
691,690,689,688,687,0,
707,705,704,703,702,701,700,699,698,697,696,694,685,135,134,0,3,133,443,538,
456,0,88,
456,0,88,
456,0,88,
456,0,88,
456,0,88,
456,0,88,
456,0,88,
456,0,88,
456,0,88,
456,0,88,
456,0,88,
546,525,524,523,522,521,520,519,518,517,516,515,514,513,512,511,510,509,508,
  507,506,505,504,503,502,501,500,499,498,497,496,494,456,427,425,1,0,198,
  200,201,202,203,
546,525,524,523,522,521,520,519,518,517,516,515,514,513,512,511,510,509,508,
  507,506,505,504,503,502,501,500,499,498,497,496,494,456,427,425,1,0,198,
  200,201,202,203,
546,520,519,518,517,516,515,514,513,512,511,510,509,508,507,506,505,504,503,
  502,501,500,499,498,497,496,494,456,429,427,426,425,1,0,196,197,
546,520,519,518,517,516,515,514,513,512,511,510,509,508,507,506,505,504,503,
  502,501,500,499,498,497,496,494,456,429,427,426,425,1,0,196,197,
546,520,519,518,517,516,515,514,513,512,511,510,509,508,507,506,505,504,503,
  502,501,500,499,498,497,496,494,456,429,427,426,425,1,0,196,197,
546,520,519,518,517,516,515,514,513,512,511,510,509,508,507,506,505,504,503,
  502,501,500,499,498,497,496,494,456,429,427,426,425,1,0,196,197,
546,518,517,516,515,514,513,512,511,510,509,508,507,506,505,504,503,502,501,
  500,499,498,497,496,494,456,429,427,426,425,1,0,192,193,194,195,
546,518,517,516,515,514,513,512,511,510,509,508,507,506,505,504,503,502,501,
  500,499,498,497,496,494,456,429,427,426,425,1,0,192,193,194,195,
546,514,513,512,511,510,509,508,507,506,505,504,503,502,501,500,499,498,497,
  496,494,456,429,427,426,425,1,0,190,191,
546,514,513,512,511,510,509,508,507,506,505,504,503,502,501,500,499,498,497,
  496,494,456,429,427,426,425,1,0,190,191,
546,512,511,510,509,508,507,506,505,504,503,502,501,500,499,498,497,496,494,
  456,429,427,426,425,1,0,188,189,
546,512,511,510,509,508,507,506,505,504,503,502,501,500,499,498,497,496,494,
  456,429,427,426,425,1,0,188,189,
429,427,426,425,0,39,40,41,45,46,47,49,
429,427,426,425,0,39,40,41,45,46,47,49,
429,427,426,425,0,39,40,41,45,46,47,49,
542,242,241,240,239,238,237,236,235,1,0,51,52,
707,706,705,704,703,702,701,700,699,698,697,696,695,694,693,692,691,690,689,
  688,687,686,685,684,683,682,528,526,523,522,521,520,519,513,511,509,481,
  458,456,431,427,425,234,1,0,
707,705,704,703,702,701,700,699,698,697,696,695,694,691,690,689,688,687,686,
  685,683,539,537,536,535,534,533,532,531,530,529,528,527,526,523,520,519,
  492,458,431,232,135,134,0,2,6,7,8,9,10,12,15,17,18,20,21,23,24,25,26,27,
  28,29,57,86,89,131,133,197,199,204,205,206,207,208,209,210,211,212,213,
  214,215,216,217,218,219,220,436,443,486,493,540,
707,705,704,703,702,701,700,699,698,697,696,695,694,691,690,689,688,687,686,
  685,539,537,536,535,534,533,532,531,530,529,528,527,526,523,520,519,492,
  458,431,232,135,134,0,2,6,7,8,9,15,17,21,23,24,25,26,27,28,29,57,86,89,
  131,133,197,199,204,205,206,207,208,209,210,211,212,213,214,215,216,217,
  218,219,220,443,493,540,
707,705,704,703,702,701,700,699,698,697,696,694,685,135,134,0,21,133,443,
707,705,704,703,702,701,700,699,698,697,696,694,691,690,689,688,687,685,456,
  431,1,0,51,52,138,
456,1,0,51,52,
456,1,0,51,52,
542,242,241,240,239,238,237,236,235,0,4,243,596,
456,0,88,

};


static unsigned const char ag_astt[27588] = {
  8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,7,1,1,9,5,2,2,2,2,2,
  2,2,2,2,2,2,2,2,1,8,8,8,8,8,2,2,2,2,2,1,1,7,1,0,1,1,1,1,1,1,1,1,1,1,1,5,5,
  5,5,5,5,5,5,5,5,5,1,7,1,3,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,4,2,4,4,4,4,
  2,4,4,7,2,1,1,1,1,1,1,1,1,1,1,1,7,1,3,1,1,1,1,1,1,1,1,1,1,5,5,5,5,5,5,5,5,
  5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,
  5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,
  5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,
  5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,
  5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,
  5,1,7,1,3,1,7,2,8,8,1,1,1,1,1,7,3,3,1,3,1,1,1,1,1,8,8,8,8,8,8,8,8,8,8,8,8,
  8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,
  8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,
  8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,
  8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,
  8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,5,5,8,8,8,8,8,8,8,1,7,1,
  1,2,2,2,2,2,2,2,2,2,2,2,2,2,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,
  1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,
  1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,
  1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,8,
  8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,1,1,1,8,8,8,8,
  8,5,5,1,5,5,5,5,2,2,9,7,1,1,1,1,1,1,1,2,1,2,2,1,1,1,1,1,1,1,1,1,1,1,1,1,1,
  2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,
  2,2,2,2,2,2,2,2,2,2,2,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,
  1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,
  1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,2,2,2,
  2,2,2,2,2,2,2,2,2,2,1,8,8,8,8,8,2,2,2,2,2,1,3,1,7,1,3,3,1,1,3,1,1,1,1,1,5,
  5,5,5,5,5,5,5,5,5,5,5,5,5,5,1,7,1,3,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,1,7,1,3,
  5,1,7,1,3,5,5,5,5,5,7,1,3,5,5,5,5,5,7,1,3,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,1,
  7,1,3,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,
  5,5,5,5,5,5,5,5,1,7,1,3,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,
  5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,1,7,1,3,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,1,
  7,1,3,5,5,5,5,5,1,7,1,3,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,1,7,1,3,
  2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,7,1,1,1,1,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,7,1,
  1,1,2,7,1,1,1,8,8,8,8,1,7,1,1,8,8,8,8,1,7,1,1,2,2,2,2,2,2,2,2,2,2,2,2,2,2,
  2,7,1,1,1,2,2,2,2,2,2,2,2,2,2,2,1,2,2,2,2,2,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,
  1,1,1,1,1,1,1,2,2,2,7,2,1,1,2,1,2,1,1,1,1,1,1,1,1,1,1,1,1,2,1,1,1,1,1,1,1,
  1,2,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,2,2,2,2,2,2,2,2,2,2,2,1,2,2,2,2,2,1,1,
  1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,2,2,2,7,2,1,1,2,1,2,1,1,1,1,1,1,1,
  1,1,1,1,1,2,1,1,1,1,1,1,1,1,2,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,2,2,2,2,2,2,
  2,2,2,2,2,2,2,2,2,7,1,1,1,1,1,1,1,1,7,1,1,1,1,1,2,2,7,1,1,1,1,1,1,5,5,5,5,
  5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,
  5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,7,1,3,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,
  5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,
  5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,7,1,3,1,1,
  7,1,1,1,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,
  5,5,5,5,5,5,5,5,5,5,5,5,5,7,1,3,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,
  5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,7,1,3,5,5,5,5,5,5,5,
  5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,
  7,1,3,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,
  1,1,1,1,1,1,1,1,1,1,1,9,7,3,3,3,1,3,1,3,3,3,3,3,3,3,3,3,3,3,3,3,3,3,3,3,3,
  3,3,3,3,3,3,3,3,3,3,3,3,3,3,3,3,3,3,3,3,3,3,3,1,1,1,1,1,1,1,1,1,1,1,1,1,1,
  1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,8,1,7,1,1,1,1,1,1,
  1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,
  1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,
  1,1,1,1,1,1,8,1,7,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,
  1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,2,2,2,2,2,2,2,2,2,2,2,2,2,1,1,1,1,1,1,1,1,
  1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,
  1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,
  1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,
  1,1,1,1,1,1,1,1,1,1,1,1,1,1,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,
  8,8,8,8,8,8,8,8,1,1,1,8,8,8,8,8,8,8,1,1,1,1,1,2,2,7,1,1,1,1,1,3,3,1,3,1,1,
  1,1,1,2,1,2,2,1,1,1,1,1,1,1,1,1,1,1,1,1,1,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,
  2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,1,1,1,1,1,
  1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,
  1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,
  1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,8,8,8,8,8,8,1,7,1,1,5,5,5,5,5,5,7,1,3,
  5,5,5,5,5,5,7,1,3,5,5,5,5,5,5,7,1,3,5,5,5,5,5,5,7,1,3,5,5,5,5,5,5,7,1,3,5,
  5,5,5,5,5,7,1,3,5,5,5,5,5,5,7,1,3,5,5,5,5,5,5,7,1,3,5,5,5,5,5,5,7,1,3,5,5,
  5,5,5,5,7,1,3,5,5,5,5,5,5,7,1,3,5,5,5,5,5,5,7,1,3,5,5,5,5,5,5,7,1,3,5,5,5,
  5,5,5,7,1,3,5,5,5,5,5,5,5,5,5,5,7,1,3,5,5,5,5,5,5,5,5,5,5,7,1,3,5,5,5,5,5,
  5,5,5,7,1,3,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,
  5,5,5,5,5,5,5,5,5,5,5,5,7,1,3,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,
  5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,7,1,3,5,5,5,5,5,5,5,5,5,5,5,5,5,
  5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,7,1,3,5,5,5,5,
  5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,
  5,5,7,1,3,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,
  5,5,5,5,5,5,5,5,5,5,5,7,1,3,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,
  5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,7,1,3,5,5,5,5,5,5,5,5,5,5,5,5,5,5,
  5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,7,1,3,5,5,5,5,5,
  5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,
  5,7,1,3,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,
  5,5,5,5,5,5,5,5,5,5,7,1,3,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,
  5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,7,1,3,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,
  5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,7,1,3,5,5,5,5,5,5,
  5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,
  7,1,3,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,
  5,5,5,5,5,5,5,5,5,7,1,3,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,
  5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,7,1,3,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,
  5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,7,1,3,5,5,5,5,5,5,5,
  5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,7,
  1,3,5,5,5,5,5,5,7,1,3,5,5,5,5,5,5,7,1,3,5,5,5,5,5,5,7,1,3,5,5,5,5,5,5,7,1,
  3,5,5,5,5,5,5,7,1,3,5,5,5,5,5,5,7,1,3,5,5,5,5,5,5,7,1,3,5,5,5,5,5,5,7,1,3,
  5,5,5,5,5,5,7,1,3,5,5,5,5,5,5,7,1,3,5,5,5,5,5,5,7,1,3,5,5,5,5,5,5,7,1,3,5,
  5,5,5,5,5,7,1,3,5,5,5,5,5,5,7,1,3,5,5,5,5,5,5,7,1,3,5,5,5,5,5,5,7,1,3,5,5,
  5,5,5,5,7,1,3,5,5,5,5,5,5,7,1,3,5,5,5,5,5,5,7,1,3,5,5,5,5,5,5,7,1,3,5,5,5,
  5,5,5,7,1,3,5,5,5,5,5,5,7,1,3,5,5,5,5,5,5,7,1,3,5,5,5,5,5,5,7,1,3,5,5,5,5,
  5,5,7,1,3,5,5,5,5,5,5,7,1,3,5,5,5,5,5,5,7,1,3,5,5,5,5,5,5,7,1,3,5,5,5,5,5,
  5,7,1,3,5,5,5,5,5,7,1,3,8,8,8,8,8,8,8,8,8,1,7,1,1,8,8,8,8,8,8,8,8,8,1,7,1,
  1,8,8,8,8,8,8,8,1,7,1,1,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,
  5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,7,1,3,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,
  5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,7,1,3,5,5,5,5,5,5,5,
  5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,7,
  1,3,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,
  5,5,5,5,5,5,5,5,7,1,3,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,
  5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,7,1,3,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,
  5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,7,1,3,5,5,5,5,5,5,5,5,
  5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,7,1,
  3,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,
  5,5,5,5,5,5,5,7,1,3,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,
  5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,7,1,3,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,
  5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,7,1,3,5,5,5,5,5,5,5,5,5,
  5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,7,1,3,
  5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,
  5,5,5,5,5,5,7,1,3,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,
  5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,7,1,3,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,
  5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,7,1,3,5,5,5,5,5,5,5,5,5,5,
  5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,7,1,3,5,
  5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,
  5,5,5,5,5,7,1,3,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,
  5,5,5,5,5,5,5,5,5,5,5,5,5,5,7,1,3,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,
  5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,7,1,3,5,5,5,5,5,5,5,5,5,5,5,
  5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,7,1,3,5,5,
  5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,
  5,5,5,5,7,1,3,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,
  5,5,5,5,5,5,5,5,5,5,5,5,5,7,1,3,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,
  5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,7,1,3,5,5,5,5,5,5,5,5,5,5,5,5,
  5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,7,1,3,5,5,5,
  5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,
  5,5,5,7,1,3,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,
  5,5,5,5,5,5,5,5,5,5,5,5,7,1,3,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,
  5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,7,1,3,5,5,5,5,5,5,5,5,5,5,5,5,5,
  5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,7,1,3,5,5,5,5,
  5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,
  5,5,7,1,3,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,
  5,5,5,5,5,5,5,5,5,5,5,7,1,3,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,
  5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,7,1,3,5,5,5,5,5,5,5,5,5,5,5,5,5,5,
  5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,7,1,3,5,5,5,5,5,
  5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,
  5,7,1,3,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,
  5,5,5,5,5,5,5,5,5,5,7,1,3,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,
  5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,7,1,3,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,
  5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,7,1,3,5,5,5,5,5,5,
  5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,
  7,1,3,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,
  5,5,5,5,5,5,5,5,5,7,1,3,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,
  5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,7,1,3,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,
  5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,7,1,3,5,5,5,5,5,5,5,
  5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,7,
  1,3,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,
  5,5,5,5,5,5,5,5,7,1,3,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,
  5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,7,1,3,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,
  5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,7,1,3,5,5,5,5,5,5,5,5,
  5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,7,1,
  3,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,
  5,5,5,5,5,5,5,7,1,3,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,
  5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,7,1,3,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,
  5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,7,1,3,5,5,5,5,5,5,5,5,5,
  5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,7,1,3,
  5,5,5,5,5,5,5,5,5,7,1,3,5,5,5,5,5,5,5,5,5,7,1,3,5,5,5,5,5,5,5,5,5,7,1,3,5,
  5,5,5,5,5,5,5,5,7,1,3,5,5,5,5,5,7,1,3,5,5,5,5,5,7,1,3,5,5,5,5,5,5,5,5,7,1,
  3,5,5,5,5,5,5,5,5,7,1,3,5,5,5,5,5,5,5,5,7,1,3,5,5,5,5,5,5,5,5,5,5,7,1,3,5,
  5,5,5,5,5,5,5,5,5,7,1,3,5,5,5,5,5,5,5,5,5,5,7,1,3,5,5,5,5,5,5,5,5,5,5,7,1,
  3,5,5,5,5,5,5,5,5,5,5,7,1,3,5,5,5,5,5,5,5,5,5,5,7,1,3,5,5,5,5,5,5,5,5,5,5,
  7,1,3,5,5,5,5,5,5,5,5,5,5,7,1,3,5,5,5,5,5,5,5,5,5,5,7,1,3,5,5,5,5,5,5,5,5,
  5,5,7,1,3,5,5,5,5,5,5,5,5,5,5,7,1,3,5,5,5,5,5,5,5,5,5,5,7,1,3,5,5,5,5,5,5,
  5,5,5,5,7,1,3,8,8,8,8,1,7,1,1,8,1,7,1,1,8,1,7,1,1,8,1,7,1,1,8,8,8,8,8,8,8,
  8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,1,7,
  1,1,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,
  8,8,8,8,8,8,8,1,7,1,1,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,
  8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,1,7,1,1,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,
  8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,1,7,1,1,8,8,8,8,8,8,8,8,
  8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,1,7,1,
  1,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,
  8,8,8,8,8,8,1,7,1,1,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,
  8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,1,7,1,1,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,
  8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,1,7,1,1,8,8,8,8,8,8,8,8,8,
  8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,1,7,1,1,
  8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,
  8,8,8,8,8,1,7,1,1,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,
  8,8,8,8,8,8,8,8,8,8,8,8,8,8,1,7,1,1,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,
  8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,1,7,1,1,8,8,8,8,8,8,8,8,8,8,
  8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,1,7,1,1,8,
  8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,
  8,8,8,8,1,7,1,1,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,
  8,8,8,8,8,8,8,8,8,8,8,8,8,1,7,1,1,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,
  8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,1,7,1,1,8,8,8,8,8,8,8,8,8,8,8,
  8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,1,7,1,1,8,8,
  8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,
  8,8,8,1,7,1,1,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,
  8,8,8,8,8,8,8,8,8,8,8,8,1,7,1,1,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,
  8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,1,7,1,1,8,8,8,8,8,8,8,8,8,8,8,8,
  8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,1,7,1,1,8,8,8,
  8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,
  8,8,1,7,1,1,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,
  8,8,8,8,8,8,8,8,8,8,8,1,7,1,1,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,
  8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,1,7,1,1,8,8,8,8,8,8,8,8,8,8,8,8,8,
  8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,1,7,1,1,8,8,8,8,
  8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,
  8,1,7,1,1,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,
  8,8,8,8,8,8,8,8,8,8,1,7,1,1,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,
  8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,1,7,1,1,8,8,8,8,8,8,8,8,8,8,8,8,8,8,
  8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,1,7,1,1,8,8,8,8,8,
  8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,
  1,7,1,1,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,
  8,8,8,8,8,8,8,8,8,1,7,1,1,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,
  8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,1,7,1,1,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,
  8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,1,7,1,1,8,8,8,8,8,8,
  8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,1,
  7,1,1,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,
  8,8,8,8,8,8,8,8,1,7,1,1,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,
  8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,1,7,1,1,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,
  8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,1,7,1,1,8,8,8,8,8,8,8,
  8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,1,7,
  1,1,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,
  8,8,8,8,8,8,8,1,7,1,1,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,
  8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,1,7,1,1,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,
  8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,1,7,1,1,8,8,8,8,8,8,8,8,
  8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,1,7,1,
  1,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,
  8,8,8,8,8,8,1,7,1,1,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,
  8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,1,7,1,1,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,
  8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,1,7,1,1,8,8,8,8,8,8,8,8,8,
  8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,1,7,1,1,
  8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,
  8,8,8,8,8,1,7,1,1,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,
  8,8,8,8,8,8,8,8,8,8,8,8,8,8,1,7,1,1,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,
  8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,1,7,1,1,8,8,8,8,8,8,8,8,8,8,
  8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,1,7,1,1,8,
  8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,
  8,8,8,8,1,7,1,1,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,
  8,8,8,8,8,8,8,8,8,8,8,8,8,1,7,1,1,8,8,8,8,8,8,8,8,1,7,1,1,8,8,8,8,8,8,8,8,
  1,7,1,1,8,8,8,8,8,8,8,8,1,7,1,1,8,8,8,8,8,8,8,8,1,7,1,1,8,8,8,8,1,7,1,1,8,
  8,8,8,1,7,1,1,8,8,8,8,8,8,8,1,7,1,1,8,8,8,8,8,8,8,1,7,1,1,8,8,8,8,8,8,8,1,
  7,1,1,8,8,8,8,8,8,8,8,8,1,7,1,1,8,8,8,8,8,8,8,8,8,1,7,1,1,8,8,8,8,8,8,8,8,
  8,1,7,1,1,8,8,8,8,8,8,8,8,8,1,7,1,1,8,8,8,8,8,8,8,8,8,1,7,1,1,8,8,8,8,8,8,
  8,8,8,1,7,1,1,8,8,8,8,8,8,8,8,8,1,7,1,1,8,8,8,8,8,8,8,8,8,1,7,1,1,8,8,8,8,
  8,8,8,8,8,1,7,1,1,8,8,8,8,8,8,8,8,8,1,7,1,1,8,8,8,8,8,8,8,8,8,1,7,1,1,8,8,
  8,8,8,8,8,8,8,1,7,1,1,8,8,8,8,8,8,8,8,8,1,7,1,1,5,2,2,2,2,2,2,2,2,2,2,2,5,
  2,5,5,2,2,2,2,2,5,2,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,
  2,5,5,5,5,1,7,1,3,2,5,5,5,5,5,1,7,1,3,4,4,4,4,4,4,4,4,4,4,4,4,4,4,4,4,4,4,
  4,4,4,4,4,4,4,4,4,4,4,4,4,4,1,4,4,4,4,4,4,7,1,4,4,4,4,1,7,1,1,5,1,2,2,2,2,
  2,2,2,2,2,2,2,1,2,2,2,2,2,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,2,2,
  2,7,2,1,1,2,1,2,1,1,1,1,1,1,1,1,1,1,2,1,2,1,1,1,1,1,1,1,1,2,1,1,1,1,1,1,1,
  1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,
  1,1,1,1,1,1,1,1,7,2,2,1,2,2,2,2,1,1,1,1,2,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,
  1,1,1,1,2,2,1,2,2,1,1,2,8,8,8,8,1,7,1,1,4,4,4,4,4,4,4,4,4,4,4,4,4,4,4,4,4,
  4,4,4,4,4,4,4,4,4,4,4,4,4,4,4,4,4,4,4,1,4,4,4,4,7,1,8,8,8,8,8,8,8,8,8,8,8,
  8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,1,7,1,1,8,8,8,8,
  1,7,1,1,2,1,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,3,2,2,2,2,2,2,2,2,
  2,2,2,2,2,2,2,2,2,2,7,2,5,5,5,5,5,5,5,5,7,1,3,8,8,8,8,1,7,1,1,1,1,1,1,7,2,
  2,1,2,1,1,1,1,1,1,1,7,2,2,1,2,1,1,1,8,8,8,8,1,7,1,1,2,2,2,2,2,7,2,2,2,2,7,
  2,2,7,4,4,4,4,4,4,4,2,2,2,2,4,2,2,2,2,2,4,4,4,4,4,4,4,4,4,4,4,4,4,4,4,4,4,
  4,4,4,4,4,4,4,4,4,4,4,4,4,4,4,4,4,4,4,4,4,4,4,4,7,4,1,4,4,4,4,4,4,4,4,4,4,
  4,4,4,4,4,4,4,4,4,4,4,4,4,4,4,4,4,4,4,4,4,4,4,4,4,4,4,4,4,4,4,4,4,4,4,4,4,
  4,4,4,4,4,7,2,2,2,2,2,7,2,1,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,
  2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,7,1,10,10,10,10,10,5,2,10,10,10,10,10,
  10,10,10,10,7,10,10,10,10,4,4,4,4,4,4,4,4,4,4,4,4,4,4,4,4,4,4,4,4,4,4,4,4,
  4,4,4,4,4,4,4,4,4,4,4,4,4,4,7,10,10,4,4,4,4,4,4,4,4,4,4,4,4,4,4,4,4,4,4,4,
  4,4,4,4,4,4,4,4,4,4,4,4,4,4,4,4,4,4,4,7,10,10,10,10,10,10,10,10,10,4,4,4,4,
  4,4,4,4,4,4,4,4,4,4,4,4,4,4,4,4,4,4,4,4,4,4,4,4,4,4,4,4,4,4,4,4,4,4,7,9,2,
  2,1,1,2,10,10,10,10,10,9,4,4,4,4,4,4,4,4,4,4,4,4,4,4,4,4,4,4,4,4,4,4,4,4,4,
  4,4,4,4,4,4,2,4,4,4,4,2,2,4,7,2,5,5,7,1,3,5,5,5,5,5,5,7,1,3,5,5,7,1,3,5,5,
  7,1,3,5,5,7,1,3,5,5,7,1,3,5,5,7,1,3,5,5,7,1,3,5,5,7,1,3,5,5,7,1,3,5,5,7,1,
  3,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,
  5,5,7,1,3,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,
  5,5,5,5,5,1,7,1,3,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,
  5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,7,
  1,3,8,1,7,1,1,8,1,7,1,1,8,1,7,1,1,8,1,7,1,1,8,1,7,1,1,8,1,7,1,1,8,1,7,1,1,
  8,1,7,1,1,8,1,7,1,1,8,1,7,1,1,8,1,7,1,1,5,5,5,5,5,5,5,5,5,5,5,5,5,2,2,2,2,
  2,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,1,7,1,3,5,5,5,5,5,5,5,5,5,5,5,
  5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,
  5,5,5,5,5,5,5,5,5,5,5,5,5,5,7,1,3,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,
  5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,1,7,1,3,5,5,5,5,5,5,5,5,5,5,5,5,5,5,
  5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,
  5,5,5,5,5,5,5,5,5,5,5,7,1,3,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,
  8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,1,7,1,1,2,2,2,2,2,2,2,2,2,2,2,1,2,2,
  2,2,2,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,2,2,2,7,2,1,1,2,1,2,1,1,1,2,1,
  1,2,2,2,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,2,2,2,2,2,2,2,2,2,2,2,1,2,2,2,2,2,1,
  1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,2,2,2,7,2,1,1,2,1,2,1,1,1,2,1,1,2,2,2,
  1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,2,2,2,2,2,2,2,2,2,2,2,1,2,2,2,2,2,1,1,1,1,1,
  1,1,1,1,1,1,1,1,1,1,1,1,1,1,2,2,2,7,2,1,1,2,1,2,1,1,1,2,1,1,2,2,2,1,1,1,1,
  1,1,1,1,1,1,1,1,1,1,1,2,2,2,2,2,2,2,2,2,2,2,1,2,2,2,2,2,1,1,1,1,1,1,1,1,1,
  1,1,1,1,1,1,1,1,1,1,2,2,2,7,2,1,1,2,1,2,1,1,1,2,1,1,2,2,2,1,1,1,1,1,1,1,1,
  1,1,1,1,1,1,1,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,
  5,5,5,5,5,5,5,1,7,1,3,5,1,1,1,1,1,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,
  5,5,5,5,5,5,5,5,5,1,7,1,3,1,1,1,1,1,5,1,1,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,
  5,5,5,5,5,5,5,5,5,5,5,5,5,1,7,1,3,1,1,5,1,1,1,1,5,5,5,5,5,5,5,5,5,5,5,5,5,
  5,5,5,5,5,5,5,5,5,5,5,5,1,7,1,3,1,1,1,1,5,1,1,5,5,5,5,5,5,5,5,5,5,5,5,5,5,
  5,5,5,5,5,5,5,5,5,1,7,1,3,1,1,5,1,1,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,
  5,5,1,7,1,3,1,1,1,1,5,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,4,4,4,4,4,7,2,2,2,2,2,
  2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,1,2,2,2,2,2,1,1,1,1,1,1,
  1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,2,2,2,7,2,1,1,2,1,2,1,1,1,1,1,1,1,1,1,2,1,
  2,1,1,1,1,1,1,1,2,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,8,8,8,8,1,7,1,1,8,8,8,8,
  1,7,1,1,8,8,8,8,1,7,1,5,5,5,5,5,7,1,3,8,8,8,8,1,7,1,1,5,5,5,5,5,7,1,3,8,8,
  8,8,1,7,1,1,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,7,1,3,8,8,8,8,8,8,8,8,8,8,8,8,
  8,8,8,1,7,1,1,5,5,5,5,5,7,1,3,8,8,8,8,1,7,1,1,5,5,5,5,5,7,1,3,8,8,8,8,1,7,
  1,1,10,10,1,10,10,10,10,10,10,10,10,10,10,10,10,2,10,10,10,10,10,10,10,10,
  10,10,10,10,10,10,10,10,10,10,10,10,10,10,10,10,10,10,7,5,5,5,5,5,7,1,3,8,
  8,8,8,1,7,1,1,8,8,8,8,1,7,1,1,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,1,
  7,1,3,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,7,1,1,1,1,1,1,1,1,5,5,5,5,
  5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,
  5,5,5,5,5,5,5,7,1,3,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,
  5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,7,
  1,3,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,
  5,5,5,5,5,5,5,5,5,5,5,5,5,7,1,3,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,
  5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,
  5,5,5,5,5,7,1,3,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,
  5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,7,1,3,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,
  5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,7,1,3,5,5,5,
  5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,
  5,5,5,5,5,5,5,5,7,1,3,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,
  5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,7,1,3,5,5,5,5,5,5,5,5,5,5,5,5,
  5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,7,
  1,3,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,
  5,5,5,5,5,5,5,5,5,5,5,5,5,7,1,3,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,
  5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,7,1,3,5,5,5,5,5,5,5,
  5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,
  5,5,5,5,7,1,3,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,
  5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,7,1,3,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,
  5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,7,1,3,5,5,
  5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,
  5,5,5,5,5,5,5,5,5,7,1,3,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,
  5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,7,1,3,5,5,5,5,5,5,5,5,5,5,5,
  5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,
  7,1,3,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,
  5,5,5,5,5,5,5,5,5,5,5,5,5,5,7,1,3,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,
  5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,7,1,3,5,5,5,5,5,5,
  5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,
  5,5,5,5,5,7,1,3,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,
  5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,7,1,3,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,
  5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,7,1,3,5,
  5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,
  5,5,5,5,5,5,5,5,5,5,7,1,3,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,
  5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,7,1,3,5,5,5,5,5,5,5,5,5,5,
  5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,
  5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,7,1,3,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,
  5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,
  5,5,5,5,5,5,5,7,1,3,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,
  5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,7,
  1,3,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,
  5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,7,1,3,5,5,5,5,5,5,
  5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,
  5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,7,1,3,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,
  5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,
  5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,7,1,3,5,5,5,5,5,5,5,5,5,5,5,5,5,5,
  5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,7,1,3,
  5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,
  5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,7,1,3,5,5,5,5,5,5,5,5,
  5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,
  5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,7,1,3,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,
  5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,7,1,3,5,5,5,
  5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,
  5,5,5,5,5,5,7,1,3,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,
  5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,7,1,3,5,5,5,5,5,5,5,5,5,5,5,5,5,5,
  5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,7,1,3,
  5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,
  5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,7,1,3,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,
  1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,9,5,3,3,3,3,3,3,3,3,
  3,3,3,3,3,3,3,3,3,3,3,3,3,3,3,3,3,3,3,3,3,3,3,3,3,3,3,3,3,3,3,3,3,3,1,7,3,
  1,7,3,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,
  1,1,1,1,7,1,1,2,2,1,2,2,2,2,1,1,1,1,2,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,
  1,1,2,2,1,2,2,1,1,2,8,8,8,8,1,7,1,1,1,1,1,1,1,1,7,1,3,3,1,3,1,1,1,2,2,2,1,
  1,1,2,2,2,2,2,7,2,2,2,2,3,2,2,2,2,2,2,2,2,2,2,2,7,3,2,1,2,2,2,2,2,2,2,7,3,
  2,2,2,1,2,2,2,2,7,2,1,7,1,1,7,1,1,7,1,2,2,2,2,2,2,2,2,2,2,2,1,2,2,2,2,2,1,
  1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,2,2,2,7,2,1,1,2,1,2,1,1,1,1,1,1,
  1,1,1,2,1,2,1,1,1,1,1,1,1,2,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,2,2,2,2,2,2,2,
  2,2,2,2,1,2,2,2,2,2,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,2,2,2,7,2,
  1,1,2,1,2,1,1,1,1,1,1,1,1,1,2,1,2,1,1,1,1,1,1,1,2,1,1,1,1,1,1,1,1,1,1,1,1,
  1,1,1,1,2,2,2,2,2,2,2,2,2,2,2,1,2,2,2,2,2,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,
  1,1,1,1,1,1,2,2,2,7,2,1,1,2,1,2,1,1,1,1,1,1,1,1,1,2,1,2,1,1,1,1,1,1,1,2,1,
  1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,2,2,2,2,2,2,2,2,2,2,2,1,2,2,2,2,2,1,1,1,1,1,
  1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,2,2,2,7,2,1,1,2,1,2,1,1,1,1,1,1,1,1,1,2,
  1,2,1,1,1,1,1,1,1,2,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,2,2,2,2,2,2,2,2,2,2,2,
  1,2,2,2,2,2,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,2,2,2,7,2,1,1,2,1,
  2,1,1,1,1,1,1,1,1,1,2,1,2,1,1,1,1,1,1,1,2,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,
  2,2,2,2,2,2,2,2,2,2,2,1,2,2,2,2,2,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,
  1,1,2,2,2,7,2,1,1,2,1,2,1,1,1,1,1,1,1,1,1,2,1,2,1,1,1,1,1,1,1,2,1,1,1,1,1,
  1,1,1,1,1,1,1,1,1,1,1,2,2,2,2,2,2,2,2,2,2,2,1,2,2,2,2,2,1,1,1,1,1,1,1,1,1,
  1,1,1,1,1,1,1,1,1,1,1,1,1,2,2,2,7,2,1,1,2,1,2,1,1,1,1,1,1,1,1,1,2,1,2,1,1,
  1,1,1,1,1,2,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,2,2,2,2,2,2,2,2,2,2,2,1,2,2,2,
  2,2,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,2,2,2,7,2,1,1,2,1,2,1,1,1,
  1,1,1,1,1,1,2,1,2,1,1,1,1,1,1,1,2,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,2,2,2,2,
  2,2,2,2,2,2,2,1,2,2,2,2,2,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,2,2,
  2,7,2,1,1,2,1,2,1,1,1,1,1,1,1,1,1,2,1,2,1,1,1,1,1,1,1,2,1,1,1,1,1,1,1,1,1,
  1,1,1,1,1,1,1,2,2,2,2,2,2,2,2,2,2,2,1,2,2,2,2,2,1,1,1,1,1,1,1,1,1,1,1,1,1,
  1,1,1,1,1,1,1,1,1,2,2,2,7,2,1,1,2,1,2,1,1,1,1,1,1,1,1,1,2,1,2,1,1,1,1,1,1,
  1,2,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,2,2,2,2,2,2,2,2,2,2,2,1,2,2,2,2,2,1,1,
  1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,2,2,2,7,2,1,1,2,1,2,1,1,1,1,1,1,1,
  1,1,2,1,2,1,1,1,1,1,1,1,2,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,2,2,2,2,2,2,2,2,
  2,2,2,1,2,2,2,2,2,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,2,2,2,7,2,1,
  1,2,1,2,1,1,1,1,1,1,1,1,1,2,1,2,1,1,1,1,1,1,1,2,1,1,1,1,1,1,1,1,1,1,1,1,1,
  1,1,1,2,2,2,2,2,2,2,2,2,2,2,1,2,2,2,2,2,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,
  1,1,1,1,1,2,2,2,7,2,1,1,2,1,2,1,1,1,1,1,1,1,1,1,2,1,2,1,1,1,1,1,1,1,2,1,1,
  1,1,1,1,1,1,1,1,1,1,1,1,1,1,2,2,2,2,2,2,2,2,2,2,2,1,2,2,2,2,2,1,1,1,1,1,1,
  1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,2,2,2,7,2,1,1,2,1,2,1,1,1,1,1,1,1,1,1,2,1,
  2,1,1,1,1,1,1,1,2,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,2,2,2,2,2,2,2,2,2,2,2,1,
  2,2,2,2,2,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,2,2,2,7,2,1,1,2,1,2,
  1,1,1,1,1,1,1,1,1,2,1,2,1,1,1,1,1,1,1,2,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,2,
  2,2,2,2,2,2,2,2,2,2,1,2,2,2,2,2,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,
  1,2,2,2,7,2,1,1,2,1,2,1,1,1,1,1,1,1,1,1,2,1,2,1,1,1,1,1,1,1,2,1,1,1,1,1,1,
  1,1,1,1,1,1,1,1,1,1,2,2,2,2,2,2,2,2,2,2,2,1,2,2,2,2,2,1,1,1,1,1,1,1,1,1,1,
  1,1,1,1,1,1,1,1,1,1,1,1,2,2,2,7,2,1,1,2,1,2,1,1,1,1,1,1,1,1,1,2,1,2,1,1,1,
  1,1,1,1,2,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,2,2,2,2,2,2,2,2,2,2,2,1,2,2,2,2,
  2,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,2,2,2,7,2,1,1,2,1,2,1,1,1,1,
  1,1,1,1,1,2,1,2,1,1,1,1,1,1,1,2,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,2,2,2,2,2,
  2,2,2,2,2,2,1,2,2,2,2,2,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,2,2,2,
  7,2,1,1,2,1,2,1,1,1,1,1,1,1,1,1,2,1,2,1,1,1,1,1,1,1,2,1,1,1,1,1,1,1,1,1,1,
  1,1,1,1,1,1,2,2,2,2,2,2,2,2,2,2,2,1,2,2,2,2,2,1,1,1,1,1,1,1,1,1,1,1,1,1,1,
  1,1,1,1,1,1,1,1,2,2,2,7,2,1,1,2,1,2,1,1,1,1,1,1,1,1,1,2,1,2,1,1,1,1,1,1,1,
  2,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,2,2,2,2,2,2,2,2,2,2,2,1,2,2,2,2,2,1,1,1,
  1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,2,2,2,7,2,1,1,2,1,2,1,1,1,1,1,1,1,1,
  1,2,1,2,1,1,1,1,1,1,1,2,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,2,2,2,2,2,2,2,2,2,
  2,2,1,2,2,2,2,2,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,2,2,2,7,2,1,1,
  2,1,2,1,1,1,1,1,1,1,1,1,2,1,2,1,1,1,1,1,1,1,2,1,1,1,1,1,1,1,1,1,1,1,1,1,1,
  1,1,2,2,2,2,2,2,2,2,2,2,2,1,2,2,2,2,2,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,
  1,1,1,1,2,2,2,7,2,1,1,2,1,2,1,1,1,1,1,1,1,1,1,2,1,2,1,1,1,1,1,1,1,2,1,1,1,
  1,1,1,1,1,1,1,1,1,1,1,1,1,2,2,2,2,2,2,2,2,2,2,2,1,2,2,2,2,2,1,1,1,1,1,1,1,
  1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,2,2,2,7,2,1,1,2,1,2,1,1,1,1,1,1,1,1,1,2,1,2,
  1,1,1,1,1,1,1,2,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,2,2,2,2,2,2,2,2,2,2,2,1,2,
  2,2,2,2,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,2,2,2,7,2,1,1,2,1,2,1,
  1,1,1,1,1,1,1,1,2,1,2,1,1,1,1,1,1,1,2,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,2,2,
  2,2,2,2,2,2,2,2,2,1,2,2,2,2,2,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,
  2,2,2,7,2,1,1,2,1,2,1,1,1,1,1,1,1,1,1,2,1,2,1,1,1,1,1,1,1,2,1,1,1,1,1,1,1,
  1,1,1,1,1,1,1,1,1,2,2,2,2,2,2,2,2,2,2,2,1,2,2,2,2,2,1,1,1,1,1,1,1,1,1,1,1,
  1,1,1,1,1,1,1,1,1,1,1,2,2,2,7,2,1,1,2,1,2,1,1,1,1,1,1,1,1,1,2,1,2,1,1,1,1,
  1,1,1,2,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,2,2,2,2,2,2,2,2,2,2,2,1,2,2,2,2,2,
  1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,2,2,2,7,2,1,1,2,1,2,1,1,1,1,1,
  1,1,1,1,2,1,2,1,1,1,1,1,1,1,2,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,2,2,2,2,2,2,
  2,2,2,2,2,1,2,2,2,2,2,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,2,2,2,7,
  2,1,1,2,1,2,1,1,1,1,1,1,1,1,1,2,1,2,1,1,1,1,1,1,1,2,1,1,1,1,1,1,1,1,1,1,1,
  1,1,1,1,1,2,2,2,2,2,2,2,2,2,2,2,1,2,2,2,2,2,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,
  1,1,1,1,1,1,1,2,2,2,7,2,1,1,2,1,2,1,1,1,1,1,1,1,1,1,2,1,2,1,1,1,1,1,1,1,2,
  1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,2,2,2,2,2,2,2,2,2,2,2,1,2,2,2,2,2,1,1,1,1,
  1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,2,2,2,7,2,1,1,2,1,2,1,1,1,1,1,1,1,1,1,
  2,1,2,1,1,1,1,1,1,1,2,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,2,2,2,2,2,2,2,2,2,2,
  2,1,2,2,2,2,2,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,2,2,2,7,2,1,1,2,
  1,2,1,1,1,1,1,1,1,1,1,2,1,2,1,1,1,1,1,1,1,2,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,
  1,2,2,2,2,2,2,2,2,2,2,2,1,2,2,2,2,2,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,
  1,1,1,2,2,2,7,2,1,1,2,1,2,1,1,1,1,1,1,1,1,1,2,1,2,1,1,1,1,1,1,1,2,1,1,1,1,
  1,1,1,1,1,1,1,1,1,1,1,1,2,2,2,2,2,2,2,2,2,2,2,1,2,2,2,2,2,1,1,1,1,1,1,1,1,
  1,1,1,1,1,1,1,1,1,1,1,1,1,1,2,2,2,7,2,1,1,2,1,2,1,1,1,1,1,1,1,1,1,2,1,2,1,
  1,1,1,1,1,1,2,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,2,2,2,2,2,2,2,2,2,2,2,1,2,2,
  2,2,2,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,2,2,2,7,2,1,1,2,1,2,1,1,
  1,1,1,1,1,1,1,2,1,2,1,1,1,1,1,1,1,2,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,2,2,2,
  2,2,2,2,2,2,2,2,1,2,2,2,2,2,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,2,
  2,2,7,2,1,1,2,1,2,1,1,1,1,1,1,1,1,1,2,1,2,1,1,1,1,1,1,1,2,1,1,1,1,1,1,1,1,
  1,1,1,1,1,1,1,1,2,2,2,2,2,2,2,2,2,2,2,1,2,2,2,2,2,1,1,1,1,1,1,1,1,1,1,1,1,
  1,1,1,1,1,1,1,1,1,1,2,2,2,7,2,1,1,2,1,2,1,1,1,1,1,1,1,1,1,2,1,2,1,1,1,1,1,
  1,1,2,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,2,2,2,2,2,2,2,2,2,2,2,1,2,2,2,2,2,1,
  1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,2,2,2,7,2,1,1,2,1,2,1,1,1,1,1,1,
  1,1,1,2,1,2,1,1,1,1,1,1,1,2,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,2,2,2,2,2,2,2,
  2,2,2,2,1,2,2,2,2,2,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,2,2,2,7,2,
  1,1,2,1,2,1,1,1,1,1,1,1,1,1,2,1,2,1,1,1,1,1,1,1,2,1,1,1,1,1,1,1,1,1,1,1,1,
  1,1,1,1,2,2,2,2,2,2,2,2,2,2,2,1,2,2,2,2,2,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,
  1,1,1,1,1,1,2,2,2,7,2,1,1,2,1,2,1,1,1,1,1,1,1,1,1,2,1,2,1,1,1,1,1,1,1,2,1,
  1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,2,2,2,2,2,2,2,2,2,2,2,1,2,2,2,2,2,1,1,1,1,1,
  1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,2,2,2,7,2,1,1,2,1,2,1,1,1,1,1,1,1,1,1,2,
  1,2,1,1,1,1,1,1,1,2,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,2,2,2,2,2,2,2,2,2,2,2,
  1,2,2,2,2,2,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,2,2,2,7,2,1,1,2,1,
  2,1,1,1,1,1,1,1,1,1,2,1,2,1,1,1,1,1,1,1,2,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,
  2,2,2,2,2,2,2,2,2,2,2,1,2,2,2,2,2,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,
  1,1,2,2,2,7,2,1,1,2,1,2,1,1,1,1,1,1,1,1,1,2,1,2,1,1,1,1,1,1,1,2,1,1,1,1,1,
  1,1,1,1,1,1,1,1,1,1,1,2,2,2,2,2,2,2,2,2,2,2,1,2,2,2,2,2,1,1,1,1,1,1,1,1,1,
  1,1,1,1,1,1,1,1,1,1,1,1,1,2,2,2,7,2,1,1,2,1,2,1,1,1,1,1,1,1,1,1,2,1,2,1,1,
  1,1,1,1,1,2,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,2,2,2,2,2,2,2,2,2,2,2,1,2,2,2,
  2,2,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,2,2,2,7,2,1,1,2,1,2,1,1,1,
  1,1,1,1,1,1,2,1,2,1,1,1,1,1,1,1,2,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,2,2,2,2,
  2,2,2,2,2,2,2,1,2,2,2,2,2,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,2,2,
  2,7,2,1,1,2,1,2,1,1,1,1,1,1,1,1,1,2,1,2,1,1,1,1,1,1,1,2,1,1,1,1,1,1,1,1,1,
  1,1,1,1,1,1,1,2,2,2,2,2,2,2,2,2,2,2,1,2,2,2,2,2,1,1,1,1,1,1,1,1,1,1,1,1,1,
  1,1,1,1,1,1,1,1,1,2,2,2,7,2,1,1,2,1,2,1,1,1,1,1,1,1,1,1,2,1,2,1,1,1,1,1,1,
  1,2,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,2,2,2,2,2,2,2,2,2,2,2,1,2,2,2,2,2,1,1,
  1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,2,2,2,7,2,1,1,2,1,2,1,1,1,1,1,1,1,
  1,1,2,1,2,1,1,1,1,1,1,1,2,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,2,2,2,2,2,2,2,2,
  2,2,2,1,2,2,2,2,2,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,2,2,2,7,2,1,
  1,2,1,2,1,1,1,1,1,1,1,1,1,2,1,2,1,1,1,1,1,1,1,2,1,1,1,1,1,1,1,1,1,1,1,1,1,
  1,1,1,2,2,2,2,2,2,2,2,2,2,2,1,2,2,2,2,2,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,
  1,1,1,1,1,2,2,2,7,2,1,1,2,1,2,1,1,1,1,1,1,1,1,1,2,1,2,1,1,1,1,1,1,1,2,1,1,
  1,1,1,1,1,1,1,1,1,1,1,1,1,1,2,2,2,2,2,2,2,2,2,2,2,1,2,2,2,2,2,1,1,1,1,1,1,
  1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,2,2,2,7,2,1,1,2,1,2,1,1,1,1,1,1,1,1,1,2,1,
  2,1,1,1,1,1,1,1,2,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,2,2,2,2,2,2,2,2,2,2,2,1,
  2,2,2,2,2,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,2,2,2,7,2,1,1,2,1,2,
  1,1,1,1,1,1,1,1,1,2,1,2,1,1,1,1,1,1,1,2,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,2,
  1,1,2,2,2,2,2,7,2,2,2,2,2,2,2,1,1,2,2,2,2,2,7,2,2,2,2,2,2,2,1,1,2,2,2,2,2,
  7,2,2,2,2,2,2,2,1,1,2,2,2,2,2,7,2,2,2,2,2,2,2,2,2,2,7,2,2,2,2,2,2,2,7,2,2,
  2,2,2,2,2,2,2,2,7,2,2,2,2,1,2,2,2,2,2,2,2,7,2,2,2,2,1,2,2,2,2,2,2,2,7,2,2,
  2,2,1,2,1,1,1,2,2,2,2,2,7,2,2,2,2,2,2,2,2,1,1,1,2,2,2,2,2,7,2,2,2,2,2,2,2,
  2,2,2,2,2,2,2,2,2,7,2,2,1,2,2,2,2,2,2,2,2,2,7,2,2,1,2,2,2,2,2,2,2,2,2,7,2,
  2,1,2,2,2,2,2,2,2,2,2,7,2,2,1,2,2,2,2,2,2,2,2,2,7,1,2,1,2,2,2,2,2,2,2,2,2,
  7,2,2,1,2,2,2,2,2,2,2,2,2,7,2,2,1,2,2,2,2,2,2,2,2,2,7,2,2,1,2,2,2,2,2,2,2,
  2,2,7,2,2,1,2,2,2,2,2,2,2,2,2,7,2,2,1,2,2,2,2,2,2,2,2,2,7,2,2,1,2,2,2,2,2,
  2,2,2,2,2,2,1,2,2,2,2,2,1,1,1,2,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,2,2,
  2,7,2,1,1,2,1,2,1,2,1,1,2,1,1,1,1,1,1,1,1,2,1,2,1,1,1,1,1,1,1,1,2,1,1,1,1,
  1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,2,2,2,2,1,1,1,1,1,1,2,9,7,2,1,1,2,1,1,1,1,5,
  5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,
  5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,
  5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,
  5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,7,1,3,8,8,8,8,8,8,8,8,8,8,8,8,
  8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,
  8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,
  8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,
  8,8,8,8,8,8,8,8,8,8,1,7,1,1,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,1,7,1,3,2,2,2,2,
  2,2,2,2,2,2,2,2,2,2,2,7,2,1,1,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,1,7,1,
  3,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,7,2,1,5,5,5,5,5,7,1,3,5,5,5,5,5,7,
  1,3,5,5,1,7,1,3,1,2,7,2,1,1,2,1,1,5,5,5,5,5,7,1,3,5,5,5,5,5,7,1,3,5,5,5,5,
  5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,7,1,3,2,2,2,2,2,2,2,2,2,2,2,2,2,4,4,4,4,2,
  2,4,7,2,1,1,5,1,7,1,3,2,7,1,1,1,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,
  5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,7,1,3,8,8,8,8,8,8,8,8,8,8,8,
  8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,1,7,1,1,5,
  5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,
  5,5,5,5,5,1,7,1,3,2,2,2,2,2,2,2,2,2,2,2,1,2,2,2,2,2,1,1,1,2,1,1,1,1,1,1,1,
  1,1,1,1,1,1,1,1,1,1,1,1,2,2,2,7,2,1,1,2,1,2,1,2,1,1,1,1,1,1,1,1,1,2,1,2,1,
  1,1,1,1,1,1,2,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,5,5,5,5,5,5,5,5,5,5,5,5,5,
  5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,1,7,1,3,2,2,2,2,
  2,2,2,2,2,2,2,1,2,2,2,2,2,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,2,2,
  2,7,2,1,1,2,1,2,1,1,1,1,1,1,1,1,1,2,1,2,1,1,1,1,1,1,1,2,1,1,1,1,1,1,1,1,1,
  1,1,1,1,1,1,1,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,1,7,1,3,2,2,2,2,2,2,2,2,2,2,2,
  2,2,2,2,7,2,1,1,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,1,7,1,3,2,2,2,2,2,2,2,2,2,2,
  2,2,2,2,2,7,2,1,1,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,1,7,1,3,2,2,2,2,2,2,2,2,2,
  2,2,2,2,2,2,7,2,1,1,1,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,1,7,1,3,2,2,2,2,2,2,2,
  2,2,2,2,2,2,2,2,7,2,1,1,1,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,1,7,1,3,2,2,2,2,2,
  2,2,2,2,2,2,2,2,2,2,7,2,1,1,1,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,
  5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,1,7,1,3,5,5,5,5,5,5,5,5,5,5,5,5,
  5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,1,7,1,3,2,2,
  2,2,2,2,2,2,2,2,2,1,2,2,2,2,2,1,1,1,2,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,
  1,2,2,2,7,2,1,1,2,1,2,1,2,1,1,2,1,1,1,1,1,1,1,1,2,1,2,1,1,1,1,1,1,1,1,2,1,
  1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,
  5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,1,7,1,3,5,5,5,5,5,5,5,5,5,5,
  5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,1,7,1,3,
  5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,
  5,5,5,5,5,5,1,7,1,3,2,2,2,2,2,2,2,2,2,2,2,1,2,2,2,2,2,1,1,1,2,1,1,1,1,1,1,
  1,1,1,1,1,1,1,1,1,1,1,1,1,2,2,2,7,2,1,1,2,1,2,1,2,1,1,2,1,1,1,1,1,1,1,1,2,
  1,2,1,1,1,1,1,1,1,1,2,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,5,5,5,5,5,5,5,5,
  5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,1,7,1,
  3,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,
  5,5,5,5,5,5,1,7,1,3,2,2,2,2,2,2,2,2,2,2,2,1,2,2,2,2,2,1,1,1,1,1,1,1,1,1,1,
  1,1,1,1,1,1,1,1,1,1,1,1,2,2,2,7,2,1,1,2,1,2,1,1,1,1,1,1,1,1,1,2,1,2,1,1,1,
  1,1,1,1,2,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,5,5,5,5,5,7,1,3,5,5,5,5,5,5,5,5,
  5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,1,7,1,
  3,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,
  5,5,5,5,5,5,1,7,1,3,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,
  5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,1,7,1,3,2,2,2,2,2,2,2,2,2,2,2,1,2,2,2,2,2,1,
  1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,2,2,2,7,2,1,1,2,1,2,1,1,1,1,1,1,
  1,1,1,2,1,2,1,1,1,1,1,1,1,2,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,2,2,2,2,2,2,2,
  2,2,2,2,2,2,2,2,7,2,1,1,5,5,5,5,5,5,5,7,1,3,5,5,5,5,5,5,5,7,1,3,1,1,1,1,7,
  3,3,1,3,1,1,1,1,7,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,
  1,1,1,1,1,1,1,1,1,1,1,1,1,7,2,2,1,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,
  2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,1,1,1,1,7,2,2,1,2,1,1,1,2,1,2,2,2,2,2,
  1,1,1,1,2,3,7,1,1,1,1,1,7,2,2,1,2,1,1,1,1,1,1,1,7,2,2,1,2,1,1,1,2,2,2,2,2,
  2,2,2,2,7,2,2,2,2,2,2,7,2,1,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,
  2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,7,1,4,4,4,4,4,4,4,4,4,4,4,4,4,4,4,4,
  4,4,4,4,4,4,4,4,4,4,4,4,4,4,4,4,4,4,4,4,4,4,4,4,4,4,4,4,4,4,4,4,7,4,4,4,4,
  4,4,4,4,4,4,4,4,4,4,4,4,4,4,4,4,4,4,4,4,4,4,4,4,4,4,4,4,4,4,4,4,4,4,4,4,4,
  4,4,4,4,4,4,4,7,1,7,1,1,7,1,1,7,1,1,7,1,1,7,1,1,7,1,1,7,1,1,7,1,1,7,1,1,7,
  1,1,7,1,2,2,2,2,2,2,2,2,2,2,2,1,2,2,2,2,2,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,
  1,1,1,1,1,1,2,2,2,7,2,1,1,2,1,2,1,1,1,1,1,1,1,1,1,1,1,2,1,1,1,1,1,1,1,2,1,
  1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,2,2,2,2,2,7,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,
  5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,7,1,3,8,8,8,8,8,8,8,8,8,8,
  8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,1,7,1,1,5,5,5,5,
  5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,7,
  1,3,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,
  8,8,8,8,1,7,1,1,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,
  8,8,8,8,8,8,8,8,8,8,1,7,1,1,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,
  8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,1,7,1,1,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,
  8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,1,7,1,1,8,8,8,8,8,8,8,8,8,8,8,
  8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,1,7,1,1,8,8,
  8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,
  8,8,8,1,7,1,1,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,
  5,5,5,5,5,5,5,5,5,5,5,5,5,7,1,3,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,
  8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,1,7,1,1,5,5,5,5,5,5,5,5,5,5,5,5,
  5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,7,1,3,8,8,8,
  8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,
  8,8,1,7,1,1,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,
  5,5,5,5,5,5,5,5,5,5,5,5,7,1,3,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,
  8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,1,7,1,1,5,5,5,5,5,5,5,5,5,5,5,5,5,
  5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,7,1,3,8,8,8,8,
  8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,
  8,1,7,1,1,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,
  5,5,5,5,5,5,5,5,5,5,5,7,1,3,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,
  8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,1,7,1,1,8,8,8,8,8,8,8,8,8,8,8,8,8,8,
  8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,1,7,1,1,5,5,5,5,5,
  5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,
  5,7,1,3,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,
  8,8,8,8,8,8,8,8,8,1,7,1,1,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,
  8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,1,7,1,1,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,
  5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,7,1,3,8,8,8,8,8,8,
  8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,1,
  7,1,1,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,
  8,8,8,8,8,8,8,8,1,7,1,1,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,
  5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,1,7,1,3,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,
  5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,1,7,1,3,5,5,5,5,5,5,5,
  5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,1,7,
  1,3,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,
  5,5,5,5,5,5,5,1,7,1,3,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,
  5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,1,7,1,3,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,
  5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,1,7,1,3,5,5,5,5,5,5,5,5,
  5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,1,7,1,
  3,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,
  5,5,5,5,5,5,1,7,1,3,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,
  5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,1,7,1,3,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,
  5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,1,7,1,3,5,5,5,5,5,5,5,5,5,
  5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,1,7,1,3,
  5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,
  5,5,5,5,5,1,7,1,3,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,
  5,5,5,5,5,5,5,5,5,5,5,5,5,5,1,7,1,3,1,1,1,1,7,2,2,1,2,1,1,1,1,1,1,1,7,2,2,
  1,2,1,1,1,1,1,1,1,7,2,2,1,2,1,1,1,1,1,1,1,7,2,2,1,2,1,1,1,1,1,1,1,7,2,2,1,
  2,1,1,1,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,7,1,1,1,1,1,1,1,7,2,2,1,2,1,1,1,1,1,
  1,1,7,2,2,1,2,1,1,1,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,
  2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,3,2,7,1,1,1,1,7,2,2,1,2,1,1,1,1,1,1,1,7,2,2,
  1,2,1,1,1,10,10,10,10,10,10,10,10,10,10,10,10,10,10,10,10,10,10,10,5,5,5,7,
  1,3,8,8,8,8,1,7,1,1,8,8,8,8,1,7,1,1,8,8,8,8,1,7,1,1,5,5,5,5,5,5,5,5,5,5,5,
  5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,1,7,1,3,2,2,
  2,2,2,2,2,2,2,2,2,1,2,2,2,2,2,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,
  2,2,2,7,2,1,1,2,1,2,1,1,1,1,1,1,1,1,1,1,1,2,1,1,1,1,1,1,1,2,1,1,1,1,1,1,1,
  1,1,1,1,1,1,1,1,1,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,
  5,5,5,5,5,5,5,5,5,5,5,5,5,5,1,7,1,3,2,2,2,2,2,2,2,2,2,2,2,1,2,2,2,2,2,1,1,
  1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,2,2,2,7,2,1,1,2,1,2,1,1,1,1,1,1,1,
  1,1,1,1,2,1,1,1,1,1,1,1,2,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,7,3,3,1,
  3,1,1,1,5,5,5,5,5,7,1,3,8,8,8,8,1,7,1,1,5,5,5,5,5,5,5,7,1,3,5,5,5,5,5,5,5,
  7,1,3,5,5,5,5,5,5,5,7,1,3,5,5,5,5,5,5,5,5,7,1,3,5,5,5,5,5,5,5,7,1,3,8,8,8,
  8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,
  8,8,1,7,1,1,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,
  8,8,8,8,8,8,8,8,8,8,8,1,7,1,1,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,
  8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,1,7,1,1,5,5,5,5,5,5,7,1,3,5,5,5,5,
  5,5,7,1,3,8,1,7,1,1,2,1,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,
  2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,7,1,2,1,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,
  2,2,3,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,7,2,5,5,5,5,5,5,5,7,1,3,
  1,1,7,2,1,2,2,2,2,2,2,2,2,2,7,9,2,2,1,1,2,10,10,10,10,10,9,4,4,4,4,2,2,4,7,
  2,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,
  1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,
  1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,
  1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,7,1,1,1,1,1,1,1,1,1,1,1,1,1,
  2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,
  2,2,2,2,2,2,2,2,2,2,2,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,
  1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,
  1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,2,1,2,2,
  2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,
  2,2,7,1,1,7,1,2,2,2,2,2,2,2,2,2,2,2,1,2,2,2,2,2,1,1,1,2,1,1,1,1,1,1,1,1,1,
  1,1,1,1,1,1,1,1,1,1,2,2,2,7,2,1,1,2,1,2,1,2,1,1,2,1,1,1,1,1,1,1,1,2,1,2,1,
  1,1,1,1,1,1,1,2,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,4,4,4,4,4,7,1,1,4,4,
  4,4,4,7,1,1,4,4,4,4,4,7,1,1,4,4,4,4,4,7,1,1,4,4,4,4,4,7,1,5,1,7,1,3,1,7,2,
  1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,
  1,1,1,1,8,1,7,2,1,1,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,
  2,2,2,2,2,2,2,2,2,2,2,2,2,1,1,1,1,1,1,1,1,1,7,1,1,1,1,1,7,4,4,4,4,4,4,4,4,
  4,4,4,4,4,4,4,4,1,1,1,1,1,4,4,4,4,4,4,4,4,4,4,4,4,4,4,4,4,4,4,4,4,4,4,7,2,
  7,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,1,7,1,1,2,2,2,2,2,2,2,2,2,2,2,1,2,2,2,2,2,
  1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,2,2,2,7,2,1,1,2,1,2,1,1,1,1,1,
  1,1,1,1,1,1,2,1,1,1,1,1,1,1,2,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,2,2,2,2,2,2,
  2,2,2,2,2,1,2,2,2,2,2,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,2,2,2,7,
  2,1,1,2,1,2,1,1,1,1,1,1,1,1,1,1,1,2,1,1,1,1,1,1,1,2,1,1,1,1,1,1,1,1,1,1,1,
  1,1,1,1,1,2,2,2,2,2,2,2,2,2,2,2,1,2,2,2,2,2,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,
  1,1,1,1,1,1,1,2,2,2,7,2,1,1,2,1,2,1,1,1,1,1,1,1,1,1,1,1,2,1,1,1,1,1,1,1,2,
  1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,2,2,2,2,2,2,2,2,2,2,2,1,2,2,2,2,2,1,1,1,1,
  1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,2,2,2,7,2,1,1,2,1,2,1,1,1,1,1,1,1,1,1,
  1,1,2,1,1,1,1,1,1,1,2,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,2,2,2,2,2,2,2,2,2,2,
  2,1,2,2,2,2,2,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,2,2,2,7,2,1,1,2,
  1,2,1,1,1,1,1,1,1,1,1,1,1,2,1,1,1,1,1,1,1,2,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,
  1,2,2,2,2,2,2,2,2,2,2,2,1,2,2,2,2,2,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,
  1,1,1,2,2,2,7,2,1,1,2,1,2,1,1,1,1,1,1,1,1,1,1,1,2,1,1,1,1,1,1,1,2,1,1,1,1,
  1,1,1,1,1,1,1,1,1,1,1,1,2,2,2,2,2,2,2,2,2,2,2,1,2,2,2,2,2,1,1,1,1,1,1,1,1,
  1,1,1,1,1,1,1,1,1,1,1,1,1,1,2,2,2,7,2,1,1,2,1,2,1,1,1,1,1,1,1,1,1,1,1,2,1,
  1,1,1,1,1,1,2,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,2,2,2,2,2,2,2,2,2,2,2,1,2,2,
  2,2,2,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,2,2,2,7,2,1,1,2,1,2,1,1,
  1,1,1,1,1,1,1,1,1,2,1,1,1,1,1,1,1,2,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,2,2,2,
  2,2,2,2,2,2,2,2,1,2,2,2,2,2,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,2,
  2,2,7,2,1,1,2,1,2,1,1,1,1,1,1,1,1,1,1,1,2,1,1,1,1,1,1,1,2,1,1,1,1,1,1,1,1,
  1,1,1,1,1,1,1,1,2,2,2,2,2,2,2,2,2,2,2,1,2,2,2,2,2,1,1,1,1,1,1,1,1,1,1,1,1,
  1,1,1,1,1,1,1,1,1,1,2,2,2,7,2,1,1,2,1,2,1,1,1,1,1,1,1,1,1,1,1,2,1,1,1,1,1,
  1,1,2,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,8,1,7,1,1,2,2,2,2,2,2,2,2,2,2,2,1,2,
  2,2,2,2,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,2,2,2,7,2,1,1,2,1,2,1,1,1,2,
  1,1,2,2,2,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,2,2,2,2,2,2,2,2,2,2,2,1,2,2,2,2,2,
  1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,2,2,2,7,2,1,1,2,1,2,1,1,1,2,1,1,2,2,
  2,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,2,2,2,2,2,2,2,2,2,2,2,1,2,2,2,2,2,1,1,1,1,
  1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,2,2,2,7,2,1,1,2,1,2,1,1,1,2,1,1,2,2,2,1,1,1,
  1,1,1,1,1,1,1,1,1,1,1,1,2,2,2,2,2,2,2,2,2,2,2,1,2,2,2,2,2,1,1,1,1,1,1,1,1,
  1,1,1,1,1,1,1,1,1,1,1,2,2,2,7,2,1,1,2,1,2,1,1,1,2,1,1,2,2,2,1,1,1,1,1,1,1,
  1,1,1,1,1,1,1,1,2,2,2,2,2,2,2,2,2,2,2,1,2,2,2,2,2,1,1,1,1,1,1,1,1,1,1,1,1,
  1,1,1,1,1,1,1,2,2,2,7,2,1,1,2,1,2,1,1,1,2,1,1,2,2,2,1,1,1,1,1,1,1,1,1,1,1,
  1,1,1,1,2,2,2,2,2,2,2,2,2,2,2,1,2,2,2,2,2,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,
  1,1,1,1,1,1,2,2,2,7,2,1,1,2,1,2,1,1,1,1,1,2,1,1,1,1,1,1,1,2,1,1,1,1,1,1,1,
  1,1,1,1,1,1,1,1,1,2,2,2,2,2,2,2,2,2,2,2,1,2,2,2,2,2,1,1,1,1,1,1,1,1,1,1,1,
  1,1,1,1,1,1,1,1,1,1,1,2,2,2,7,2,1,1,2,1,2,1,1,1,1,1,2,1,1,1,1,1,1,1,2,1,1,
  1,1,1,1,1,1,1,1,1,1,1,1,1,1,2,2,2,2,2,2,2,2,2,2,2,1,2,2,2,2,2,1,1,1,1,1,1,
  1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,2,2,2,7,2,1,1,2,1,2,1,1,1,1,1,1,2,1,1,1,1,
  1,1,1,2,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,2,2,2,2,2,2,2,2,2,2,2,1,2,2,2,2,2,
  1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,2,2,2,7,2,1,1,2,1,2,1,1,1,1,1,
  1,2,1,1,1,1,1,1,1,2,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,2,2,2,2,2,2,2,2,2,2,2,
  1,2,2,2,2,2,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,2,2,2,7,2,1,1,2,1,
  2,1,1,1,1,1,1,2,1,1,1,1,1,1,1,2,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,2,2,2,2,2,
  2,2,2,2,2,2,1,2,2,2,2,2,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,2,2,2,
  7,2,1,1,2,1,2,1,1,1,1,1,1,2,1,1,1,1,1,1,1,2,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,
  1,2,2,2,2,2,2,2,2,2,2,2,1,2,2,2,2,2,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,
  1,1,1,2,2,2,7,2,1,1,2,1,2,1,1,1,1,1,1,1,2,1,1,1,1,1,1,1,2,1,1,1,1,1,1,1,1,
  1,1,1,1,1,1,1,1,2,2,2,2,2,2,2,2,2,2,2,1,2,2,2,2,2,1,1,1,1,1,1,1,1,1,1,1,1,
  1,1,1,1,1,1,1,1,1,1,2,2,2,7,2,1,1,2,1,2,1,1,1,1,1,1,1,2,1,1,1,1,1,1,1,2,1,
  1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,2,2,2,2,2,2,2,2,2,2,2,1,2,2,2,2,2,1,1,1,1,1,
  1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,2,2,2,7,2,1,1,2,1,2,1,1,1,1,1,1,1,1,2,1,
  1,1,1,1,1,1,2,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,2,2,2,2,2,2,2,2,2,2,2,1,2,2,
  2,2,2,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,2,2,2,7,2,1,1,2,1,2,1,1,
  1,1,1,1,1,1,2,1,1,1,1,1,1,1,2,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,2,2,2,2,2,2,
  2,2,2,2,2,1,2,2,2,2,2,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,2,2,2,7,
  2,1,1,2,1,2,1,1,1,1,1,1,1,1,1,2,1,1,1,1,1,1,1,2,1,1,1,1,1,1,1,1,1,1,1,1,1,
  1,1,1,2,2,2,2,2,2,2,2,2,2,2,1,2,2,2,2,2,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,
  1,1,1,1,1,2,2,2,7,2,1,1,2,1,2,1,1,1,1,1,1,1,1,1,2,1,1,1,1,1,1,1,2,1,1,1,1,
  1,1,1,1,1,1,1,1,1,1,1,1,8,8,8,8,1,7,1,1,1,1,1,1,7,2,2,1,2,1,1,1,1,1,1,1,7,
  2,2,1,2,1,1,1,1,1,1,1,7,2,2,1,2,1,1,1,8,8,8,8,1,7,1,1,8,8,8,8,1,7,1,1,1,1,
  1,1,7,2,2,1,2,1,1,1,2,2,2,2,2,2,2,2,2,2,2,1,2,2,2,2,2,1,1,1,1,1,1,1,1,1,1,
  1,1,1,1,1,1,1,1,1,1,1,1,2,2,2,7,2,1,1,2,1,2,1,1,1,1,1,1,1,1,1,2,1,2,1,1,1,
  1,1,1,1,2,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,2,2,2,2,2,2,2,2,2,2,2,1,2,2,2,2,
  2,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,2,2,2,7,2,1,1,2,1,2,1,1,1,1,
  1,1,1,1,1,2,1,2,1,1,1,1,1,1,1,2,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,2,2,2,2,2,
  2,2,2,2,2,2,1,2,2,2,2,2,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,2,2,2,
  7,2,1,1,2,1,2,1,1,1,1,1,1,1,1,1,2,1,2,1,1,1,1,1,1,1,2,1,1,1,1,1,1,1,1,1,1,
  1,1,1,1,1,1,1,7,1,2,1,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,
  2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,7,1,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,
  5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,7,1,3,
  8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,
  8,8,8,8,8,8,1,7,1,1,1,7,1,5,5,5,5,5,1,7,1,3,2,1,2,2,2,2,2,2,2,2,2,2,2,2,2,
  2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,7,2,8,8,8,8,8,8,8,
  8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,1,7,
  1,1,1,4,4,4,4,4,7,1,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,1,7,1,1,5,5,5,5,5,5,5,5,
  5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,
  7,1,3,4,4,4,4,4,4,4,4,4,4,4,4,4,4,4,4,4,4,4,4,4,4,4,4,4,4,4,4,4,4,4,4,4,4,
  4,4,4,4,4,4,5,5,5,5,5,7,1,1,1,1,7,2,2,1,2,1,1,1,4,4,4,4,4,4,4,4,2,2,2,2,4,
  4,4,4,2,2,2,2,2,4,4,4,4,4,4,4,4,4,4,4,4,4,4,4,4,4,4,4,4,4,4,7,2,2,2,2,2,7,
  2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,7,1,1,1,1,1,7,2,1,7,2,1,7,2,1,7,2,1,7,2,1,7,
  2,1,7,2,1,7,2,1,7,2,1,7,2,1,7,3,4,1,1,1,1,1,4,4,4,4,4,4,4,4,4,4,4,4,4,4,4,
  4,4,4,4,4,4,4,4,4,4,4,4,4,4,4,7,1,1,1,1,1,4,1,1,1,1,1,4,4,4,4,4,4,4,4,4,4,
  4,4,4,4,4,4,4,4,4,4,4,4,4,4,4,4,4,4,4,4,7,1,1,1,1,1,4,1,1,4,4,4,4,4,4,4,4,
  4,4,4,4,4,4,4,4,4,4,4,4,4,4,4,4,4,4,4,4,4,4,7,1,1,4,1,1,4,4,4,4,4,4,4,4,4,
  4,4,4,4,4,4,4,4,4,4,4,4,4,4,4,4,4,4,4,4,4,7,1,1,4,1,1,4,4,4,4,4,4,4,4,4,4,
  4,4,4,4,4,4,4,4,4,4,4,4,4,4,4,4,4,4,4,4,7,1,1,4,1,1,4,4,4,4,4,4,4,4,4,4,4,
  4,4,4,4,4,4,4,4,4,4,4,4,4,4,4,4,4,4,4,7,1,1,4,1,1,1,1,4,4,4,4,4,4,4,4,4,4,
  4,4,4,4,4,4,4,4,4,4,4,4,4,4,4,4,7,1,1,1,1,4,1,1,1,1,4,4,4,4,4,4,4,4,4,4,4,
  4,4,4,4,4,4,4,4,4,4,4,4,4,4,4,7,1,1,1,1,4,1,1,4,4,4,4,4,4,4,4,4,4,4,4,4,4,
  4,4,4,4,4,4,4,4,4,4,7,1,1,4,1,1,4,4,4,4,4,4,4,4,4,4,4,4,4,4,4,4,4,4,4,4,4,
  4,4,4,7,1,1,4,1,1,4,4,4,4,4,4,4,4,4,4,4,4,4,4,4,4,4,4,4,4,4,4,7,1,1,4,1,1,
  4,4,4,4,4,4,4,4,4,4,4,4,4,4,4,4,4,4,4,4,4,4,7,1,1,1,1,1,1,7,2,2,1,2,1,1,1,
  1,1,1,1,7,2,2,1,2,1,1,1,1,1,1,1,7,2,2,1,2,1,1,1,8,8,8,8,8,8,8,8,8,1,7,1,1,
  4,4,4,4,4,4,4,4,4,4,4,4,4,4,4,4,4,4,4,4,4,2,4,4,4,4,4,4,4,4,4,4,4,4,4,4,4,
  4,4,4,4,4,4,4,7,2,2,2,2,2,2,2,2,2,2,2,1,2,2,2,2,2,1,1,1,2,1,1,1,1,1,1,1,1,
  1,1,1,1,1,1,1,1,1,1,1,2,2,2,7,2,1,1,2,1,2,1,2,1,1,2,1,1,1,1,1,1,1,1,2,1,2,
  1,1,1,1,1,1,1,2,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,2,2,2,2,2,2,2,2,2,2,2,
  1,2,2,2,2,2,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,2,2,2,7,2,1,1,2,1,
  2,1,1,1,1,1,1,1,1,1,2,1,2,1,1,1,1,1,1,1,2,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,
  2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,7,2,1,1,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,
  5,2,1,7,1,3,2,5,5,7,1,3,8,1,7,1,1,2,2,2,2,2,2,2,2,2,7,2,2,1,1,7,2
};


static const unsigned short ag_pstt[] = {
2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,0,1,2,
18,20,
129,129,129,129,129,129,129,129,129,129,129,129,129,3,8,8,8,8,8,164,163,162,
  128,127,7,10,2,9,0,11,11,11,10,8,11,5,5,4,6,4,
19,19,19,19,19,19,19,19,19,19,19,1,3,1,890,
130,130,130,130,130,130,130,130,130,130,130,130,131,131,131,131,131,130,161,
  130,161,161,161,161,160,161,161,4,131,
12,13,14,15,16,17,18,19,20,21,22,5,33,39,32,31,30,29,28,27,26,25,24,23,
19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,
  19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,
  19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,
  19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,
  19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,
  19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,
  19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,
  19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,
  19,1,6,1,881,
34,7,37,
36,36,35,37,38,39,34,8,24,24,42,24,41,40,40,36,36,
43,43,43,43,43,43,43,43,43,43,43,43,43,43,43,43,43,43,43,43,43,43,43,43,43,
  43,43,43,43,43,43,43,43,43,43,43,43,43,43,43,43,43,43,43,43,43,43,43,43,
  43,43,43,43,43,43,43,43,43,43,43,43,43,43,43,43,43,43,43,43,43,43,43,43,
  43,43,43,43,43,43,43,43,43,43,43,43,43,43,43,43,43,43,43,43,43,43,43,43,
  43,43,43,43,43,43,43,43,43,43,43,43,43,43,43,43,43,43,43,43,43,43,43,43,
  43,43,43,43,43,43,43,43,43,43,43,43,43,43,43,43,43,43,43,43,43,43,43,43,
  43,43,43,43,43,43,43,43,43,43,43,43,43,43,43,43,43,43,43,43,43,43,43,43,
  43,43,43,43,43,43,43,43,43,43,43,43,43,43,43,43,19,19,43,43,43,43,43,43,
  43,1,9,1,43,
129,129,129,129,129,129,129,129,129,129,129,129,129,111,112,113,114,115,116,
  117,118,119,120,121,122,123,124,125,126,127,128,129,130,131,132,133,134,
  62,63,64,65,66,67,135,136,137,138,139,140,68,69,70,71,72,141,142,143,73,
  74,75,76,77,144,145,146,147,148,149,150,151,152,153,154,155,156,157,158,
  107,59,60,61,165,166,167,168,169,170,171,172,173,174,175,176,177,178,
  179,180,159,160,161,162,163,164,78,79,80,81,82,83,45,46,47,84,85,86,87,
  88,89,90,91,92,48,49,93,94,95,96,97,98,99,100,50,51,52,101,102,53,54,
  103,104,105,106,55,56,57,58,265,265,265,265,265,265,265,265,265,265,265,
  265,265,265,265,265,265,265,265,265,265,265,265,265,265,265,265,265,265,
  265,265,15,16,19,265,265,265,265,265,20,20,35,20,20,20,20,128,127,18,10,
  261,262,263,260,266,265,265,35,264,103,104,266,44,259,196,260,260,260,
  260,260,260,260,260,260,260,325,325,325,325,325,326,327,328,329,332,332,
  332,333,334,338,338,338,338,339,340,341,342,343,344,345,346,349,349,349,
  350,351,352,353,354,355,356,357,358,362,362,362,362,363,364,365,366,367,
  368,242,241,240,239,238,237,258,257,256,255,254,253,252,251,250,249,248,
  247,246,245,244,243,184,110,183,109,182,108,181,236,235,234,233,232,231,
  230,229,228,227,226,225,224,223,222,221,221,221,221,221,221,220,219,218,
  217,217,217,217,217,217,216,215,214,213,212,211,210,210,210,210,209,209,
  209,209,208,207,206,205,204,203,202,201,200,199,198,197,195,194,193,192,
  191,190,189,188,187,186,185,259,
129,129,129,129,129,129,129,129,129,129,129,129,129,3,8,8,8,8,8,164,163,162,
  128,127,7,4,10,11,9,3,3,10,8,3,5,5,4,6,4,
19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,1,12,1,906,
19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,1,13,1,905,
19,1,14,1,904,
19,19,19,19,19,15,1,903,
19,19,19,19,19,16,1,902,
19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,1,17,1,901,
19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,
  19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,1,18,1,900,
19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,
  19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,1,19,1,899,
19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,1,20,1,898,
19,19,19,19,19,1,21,1,892,
19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,1,22,1,885,
129,129,129,129,129,129,129,129,129,129,129,129,129,128,127,23,267,268,259,
  259,
129,129,129,129,129,129,129,129,129,129,129,129,129,128,127,24,269,259,259,
138,25,272,270,271,
273,273,273,273,1,26,1,273,
274,274,274,274,1,27,1,274,
129,129,129,129,129,129,129,129,129,129,129,129,129,128,127,28,275,259,259,
129,129,129,129,129,129,129,129,129,129,129,277,129,248,248,248,248,280,282,
  279,289,291,292,293,294,295,296,297,298,299,315,316,317,278,314,281,290,
  302,276,263,128,127,29,232,287,288,233,283,234,284,261,327,326,324,329,
  325,328,323,330,332,318,218,304,259,331,319,323,322,321,320,217,323,313,
  312,311,310,309,308,307,306,305,303,286,285,259,301,300,
129,129,129,129,129,129,129,129,129,129,129,277,129,248,248,248,248,280,282,
  279,289,291,292,293,294,295,296,297,298,299,315,316,317,278,314,281,290,
  302,276,263,128,127,30,232,287,288,233,283,234,284,261,327,326,324,329,
  325,328,323,330,333,318,218,304,259,331,319,323,322,321,320,217,323,313,
  312,311,310,309,308,307,306,305,303,286,285,259,301,300,
129,129,129,129,129,129,129,129,129,129,129,129,129,128,127,31,334,259,259,
335,337,339,341,343,32,344,342,340,338,336,
156,138,33,348,270,347,345,271,346,
19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,
  19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,
  19,19,19,19,19,19,19,34,1,876,
19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,
  19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,
  19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,
  19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,35,1,882,
349,22,36,350,350,350,
19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,
  19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,37,1,880,
19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,
  19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,38,
  1,878,
19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,
  19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,39,1,877,
351,353,355,356,357,358,359,360,361,362,363,364,365,366,367,368,369,370,371,
  372,373,374,381,384,385,386,387,315,317,383,375,379,376,378,382,354,352,
  377,302,380,35,37,388,38,34,15,40,16,15,15,40,13,40,15,15,15,15,15,15,
  15,15,15,15,15,15,15,15,15,15,15,15,15,15,15,15,15,15,15,15,15,15,15,15,
  15,15,15,15,15,15,15,15,15,15,15,
351,353,355,356,357,358,359,360,361,362,363,364,365,366,367,368,369,370,371,
  372,373,374,381,384,385,386,387,315,317,383,375,379,376,378,382,354,352,
  377,302,380,35,38,390,389,41,389,390,389,389,389,389,389,389,389,389,
  389,389,389,389,389,389,389,389,389,389,389,389,389,389,389,389,389,389,
  389,389,389,389,389,389,389,389,389,389,389,389,389,389,389,389,
351,353,355,356,357,358,359,360,361,362,363,364,365,366,367,368,369,370,371,
  372,373,374,381,384,385,386,387,315,317,383,375,379,376,378,382,354,352,
  377,302,380,35,38,391,389,42,389,391,389,389,389,389,389,389,389,389,
  389,389,389,389,389,389,389,389,389,389,389,389,389,389,389,389,389,389,
  389,389,389,389,389,389,389,389,389,389,389,389,389,389,389,389,
129,129,129,129,129,129,129,129,129,129,129,129,129,111,112,113,114,115,116,
  117,118,119,120,121,122,123,124,125,126,127,128,129,130,131,132,133,134,
  62,63,64,65,66,67,135,136,137,138,139,140,68,69,70,71,72,141,142,143,73,
  74,75,76,77,144,145,146,147,148,149,150,151,152,153,154,155,156,157,158,
  107,59,60,61,165,166,167,168,169,170,171,172,173,174,175,176,177,178,
  179,180,159,160,161,162,163,164,78,79,80,81,82,83,45,46,47,84,85,86,87,
  88,89,90,91,92,48,49,93,94,95,96,97,98,99,100,50,51,52,101,102,53,54,
  103,104,105,106,55,56,57,58,392,392,392,392,392,392,392,392,392,392,392,
  392,392,392,392,392,392,392,392,392,392,392,392,392,392,392,392,392,392,
  392,392,15,16,19,392,392,392,392,392,392,392,35,37,38,39,34,128,127,43,
  261,262,263,260,393,23,23,42,23,41,40,40,392,392,36,264,103,104,393,44,
  259,196,260,260,260,260,260,260,260,260,260,260,325,325,325,325,325,326,
  327,328,329,332,332,332,333,334,338,338,338,338,339,340,341,342,343,344,
  345,346,349,349,349,350,351,352,353,354,355,356,357,358,362,362,362,362,
  363,364,365,366,367,368,242,241,240,239,238,237,258,257,256,255,254,253,
  252,251,250,249,248,247,246,245,244,243,184,110,183,109,182,108,181,236,
  235,234,233,232,231,230,229,228,227,226,225,224,223,222,221,221,221,221,
  221,221,220,219,218,217,217,217,217,217,217,216,215,214,213,212,211,210,
  210,210,210,209,209,209,209,208,207,206,205,204,203,202,201,200,199,198,
  197,195,194,193,192,191,190,189,188,187,186,185,259,
394,394,394,394,394,394,1,44,1,394,
19,19,19,19,19,19,45,1,1034,
19,19,19,19,19,19,46,1,1033,
19,19,19,19,19,19,47,1,1032,
19,19,19,19,19,19,48,1,1022,
19,19,19,19,19,19,49,1,1021,
19,19,19,19,19,19,50,1,1012,
19,19,19,19,19,19,51,1,1011,
19,19,19,19,19,19,52,1,1010,
19,19,19,19,19,19,53,1,1007,
19,19,19,19,19,19,54,1,1006,
19,19,19,19,19,19,55,1,1001,
19,19,19,19,19,19,56,1,1000,
19,19,19,19,19,19,57,1,999,
19,19,19,19,19,19,58,1,998,
19,19,19,19,19,19,19,19,19,19,59,1,1067,
19,19,19,19,19,19,19,19,19,19,60,1,1066,
19,19,19,19,19,19,19,19,61,1,1065,
19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,
  19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,62,1,1108,
19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,
  19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,63,1,1107,
19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,
  19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,64,1,1106,
19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,
  19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,65,1,1105,
19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,
  19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,66,1,1104,
19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,
  19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,67,1,1103,
19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,
  19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,68,1,1096,
19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,
  19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,69,1,1095,
19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,
  19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,70,1,1094,
19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,
  19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,71,1,1093,
19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,
  19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,72,1,1092,
19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,
  19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,73,1,1088,
19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,
  19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,74,1,1087,
19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,
  19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,75,1,1086,
19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,
  19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,76,1,1085,
19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,
  19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,77,1,1084,
19,19,19,19,19,19,78,1,1040,
19,19,19,19,19,19,79,1,1039,
19,19,19,19,19,19,80,1,1038,
19,19,19,19,19,19,81,1,1037,
19,19,19,19,19,19,82,1,1036,
19,19,19,19,19,19,83,1,1035,
19,19,19,19,19,19,84,1,1031,
19,19,19,19,19,19,85,1,1030,
19,19,19,19,19,19,86,1,1029,
19,19,19,19,19,19,87,1,1028,
19,19,19,19,19,19,88,1,1027,
19,19,19,19,19,19,89,1,1026,
19,19,19,19,19,19,90,1,1025,
19,19,19,19,19,19,91,1,1024,
19,19,19,19,19,19,92,1,1023,
19,19,19,19,19,19,93,1,1020,
19,19,19,19,19,19,94,1,1019,
19,19,19,19,19,19,95,1,1018,
19,19,19,19,19,19,96,1,1017,
19,19,19,19,19,19,97,1,1016,
19,19,19,19,19,19,98,1,1015,
19,19,19,19,19,19,99,1,1014,
19,19,19,19,19,19,100,1,1013,
19,19,19,19,19,19,101,1,1009,
19,19,19,19,19,19,102,1,1008,
19,19,19,19,19,19,103,1,1005,
19,19,19,19,19,19,104,1,1004,
19,19,19,19,19,19,105,1,1003,
19,19,19,19,19,19,106,1,1002,
19,19,19,19,19,107,1,1068,
395,395,395,395,395,395,395,395,395,1,108,1,395,
396,396,396,396,396,396,396,396,396,1,109,1,396,
397,397,397,397,397,397,397,1,110,1,397,
19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,
  19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,111,1,1132,
19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,
  19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,112,1,1131,
19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,
  19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,113,1,1130,
19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,
  19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,114,1,1129,
19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,
  19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,115,1,1128,
19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,
  19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,116,1,1127,
19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,
  19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,117,1,1126,
19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,
  19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,118,1,1125,
19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,
  19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,119,1,1124,
19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,
  19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,120,1,1123,
19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,
  19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,121,1,1122,
19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,
  19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,122,1,1121,
19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,
  19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,123,1,1120,
19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,
  19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,124,1,1119,
19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,
  19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,125,1,1118,
19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,
  19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,126,1,1117,
19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,
  19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,127,1,1116,
19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,
  19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,128,1,1115,
19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,
  19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,129,1,1114,
19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,
  19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,130,1,1113,
19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,
  19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,131,1,1112,
19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,
  19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,132,1,1111,
19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,
  19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,133,1,1110,
19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,
  19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,134,1,1109,
19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,
  19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,135,1,1102,
19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,
  19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,136,1,1101,
19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,
  19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,137,1,1100,
19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,
  19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,138,1,1099,
19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,
  19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,139,1,1098,
19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,
  19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,140,1,1097,
19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,
  19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,141,1,1091,
19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,
  19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,142,1,1090,
19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,
  19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,143,1,1089,
19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,
  19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,144,1,1083,
19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,
  19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,145,1,1082,
19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,
  19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,146,1,1081,
19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,
  19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,147,1,1080,
19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,
  19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,148,1,1079,
19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,
  19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,149,1,1078,
19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,
  19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,150,1,1077,
19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,
  19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,151,1,1076,
19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,
  19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,152,1,1075,
19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,
  19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,153,1,1074,
19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,
  19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,154,1,1073,
19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,
  19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,155,1,1072,
19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,
  19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,156,1,1071,
19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,
  19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,157,1,1070,
19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,
  19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,158,1,1069,
19,19,19,19,19,19,19,19,19,159,1,1046,
19,19,19,19,19,19,19,19,19,160,1,1045,
19,19,19,19,19,19,19,19,19,161,1,1044,
19,19,19,19,19,19,19,19,19,162,1,1043,
19,19,19,19,19,163,1,1042,
19,19,19,19,19,164,1,1041,
19,19,19,19,19,19,19,19,165,1,1064,
19,19,19,19,19,19,19,19,166,1,1063,
19,19,19,19,19,19,19,19,167,1,1062,
19,19,19,19,19,19,19,19,19,19,168,1,1060,
19,19,19,19,19,19,19,19,19,19,169,1,1059,
19,19,19,19,19,19,19,19,19,19,170,1,1058,
19,19,19,19,19,19,19,19,19,19,171,1,1057,
19,19,19,19,19,19,19,19,19,19,172,1,1056,
19,19,19,19,19,19,19,19,19,19,173,1,1055,
19,19,19,19,19,19,19,19,19,19,174,1,1054,
19,19,19,19,19,19,19,19,19,19,175,1,1053,
19,19,19,19,19,19,19,19,19,19,176,1,1052,
19,19,19,19,19,19,19,19,19,19,177,1,1051,
19,19,19,19,19,19,19,19,19,19,178,1,1050,
19,19,19,19,19,19,19,19,19,19,179,1,1049,
19,19,19,19,19,19,19,19,19,19,180,1,1048,
398,398,398,398,1,181,1,398,
399,1,182,1,399,
400,1,183,1,400,
401,1,184,1,401,
402,402,402,402,402,402,402,402,402,402,402,402,402,402,402,402,402,402,402,
  402,402,402,402,402,402,402,402,402,402,402,402,402,402,402,402,402,402,
  402,402,402,402,402,1,185,1,402,
403,403,403,403,403,403,403,403,403,403,403,403,403,403,403,403,403,403,403,
  403,403,403,403,403,403,403,403,403,403,403,403,403,403,403,403,403,403,
  403,403,403,403,403,1,186,1,403,
404,404,404,404,404,404,404,404,404,404,404,404,404,404,404,404,404,404,404,
  404,404,404,404,404,404,404,404,404,404,404,404,404,404,404,404,404,404,
  404,404,404,404,404,1,187,1,404,
405,405,405,405,405,405,405,405,405,405,405,405,405,405,405,405,405,405,405,
  405,405,405,405,405,405,405,405,405,405,405,405,405,405,405,405,405,405,
  405,405,405,405,405,1,188,1,405,
406,406,406,406,406,406,406,406,406,406,406,406,406,406,406,406,406,406,406,
  406,406,406,406,406,406,406,406,406,406,406,406,406,406,406,406,406,406,
  406,406,406,406,406,1,189,1,406,
407,407,407,407,407,407,407,407,407,407,407,407,407,407,407,407,407,407,407,
  407,407,407,407,407,407,407,407,407,407,407,407,407,407,407,407,407,407,
  407,407,407,407,407,1,190,1,407,
408,408,408,408,408,408,408,408,408,408,408,408,408,408,408,408,408,408,408,
  408,408,408,408,408,408,408,408,408,408,408,408,408,408,408,408,408,408,
  408,408,408,408,408,1,191,1,408,
409,409,409,409,409,409,409,409,409,409,409,409,409,409,409,409,409,409,409,
  409,409,409,409,409,409,409,409,409,409,409,409,409,409,409,409,409,409,
  409,409,409,409,409,1,192,1,409,
410,410,410,410,410,410,410,410,410,410,410,410,410,410,410,410,410,410,410,
  410,410,410,410,410,410,410,410,410,410,410,410,410,410,410,410,410,410,
  410,410,410,410,410,1,193,1,410,
411,411,411,411,411,411,411,411,411,411,411,411,411,411,411,411,411,411,411,
  411,411,411,411,411,411,411,411,411,411,411,411,411,411,411,411,411,411,
  411,411,411,411,411,1,194,1,411,
412,412,412,412,412,412,412,412,412,412,412,412,412,412,412,412,412,412,412,
  412,412,412,412,412,412,412,412,412,412,412,412,412,412,412,412,412,412,
  412,412,412,412,412,1,195,1,412,
413,413,413,413,413,413,413,413,413,413,413,413,413,413,413,413,413,413,413,
  413,413,413,413,413,413,413,413,413,413,413,413,413,413,413,413,413,413,
  413,413,413,413,413,1,196,1,413,
414,414,414,414,414,414,414,414,414,414,414,414,414,414,414,414,414,414,414,
  414,414,414,414,414,414,414,414,414,414,414,414,414,414,414,414,414,414,
  414,414,414,414,414,1,197,1,414,
415,415,415,415,415,415,415,415,415,415,415,415,415,415,415,415,415,415,415,
  415,415,415,415,415,415,415,415,415,415,415,415,415,415,415,415,415,415,
  415,415,415,415,415,1,198,1,415,
416,416,416,416,416,416,416,416,416,416,416,416,416,416,416,416,416,416,416,
  416,416,416,416,416,416,416,416,416,416,416,416,416,416,416,416,416,416,
  416,416,416,416,416,1,199,1,416,
417,417,417,417,417,417,417,417,417,417,417,417,417,417,417,417,417,417,417,
  417,417,417,417,417,417,417,417,417,417,417,417,417,417,417,417,417,417,
  417,417,417,417,417,1,200,1,417,
418,418,418,418,418,418,418,418,418,418,418,418,418,418,418,418,418,418,418,
  418,418,418,418,418,418,418,418,418,418,418,418,418,418,418,418,418,418,
  418,418,418,418,418,1,201,1,418,
419,419,419,419,419,419,419,419,419,419,419,419,419,419,419,419,419,419,419,
  419,419,419,419,419,419,419,419,419,419,419,419,419,419,419,419,419,419,
  419,419,419,419,419,1,202,1,419,
420,420,420,420,420,420,420,420,420,420,420,420,420,420,420,420,420,420,420,
  420,420,420,420,420,420,420,420,420,420,420,420,420,420,420,420,420,420,
  420,420,420,420,420,1,203,1,420,
421,421,421,421,421,421,421,421,421,421,421,421,421,421,421,421,421,421,421,
  421,421,421,421,421,421,421,421,421,421,421,421,421,421,421,421,421,421,
  421,421,421,421,421,1,204,1,421,
422,422,422,422,422,422,422,422,422,422,422,422,422,422,422,422,422,422,422,
  422,422,422,422,422,422,422,422,422,422,422,422,422,422,422,422,422,422,
  422,422,422,422,422,1,205,1,422,
423,423,423,423,423,423,423,423,423,423,423,423,423,423,423,423,423,423,423,
  423,423,423,423,423,423,423,423,423,423,423,423,423,423,423,423,423,423,
  423,423,423,423,423,1,206,1,423,
424,424,424,424,424,424,424,424,424,424,424,424,424,424,424,424,424,424,424,
  424,424,424,424,424,424,424,424,424,424,424,424,424,424,424,424,424,424,
  424,424,424,424,424,1,207,1,424,
425,425,425,425,425,425,425,425,425,425,425,425,425,425,425,425,425,425,425,
  425,425,425,425,425,425,425,425,425,425,425,425,425,425,425,425,425,425,
  425,425,425,425,425,1,208,1,425,
426,426,426,426,426,426,426,426,426,426,426,426,426,426,426,426,426,426,426,
  426,426,426,426,426,426,426,426,426,426,426,426,426,426,426,426,426,426,
  426,426,426,426,426,1,209,1,426,
427,427,427,427,427,427,427,427,427,427,427,427,427,427,427,427,427,427,427,
  427,427,427,427,427,427,427,427,427,427,427,427,427,427,427,427,427,427,
  427,427,427,427,427,1,210,1,427,
428,428,428,428,428,428,428,428,428,428,428,428,428,428,428,428,428,428,428,
  428,428,428,428,428,428,428,428,428,428,428,428,428,428,428,428,428,428,
  428,428,428,428,428,1,211,1,428,
429,429,429,429,429,429,429,429,429,429,429,429,429,429,429,429,429,429,429,
  429,429,429,429,429,429,429,429,429,429,429,429,429,429,429,429,429,429,
  429,429,429,429,429,1,212,1,429,
430,430,430,430,430,430,430,430,430,430,430,430,430,430,430,430,430,430,430,
  430,430,430,430,430,430,430,430,430,430,430,430,430,430,430,430,430,430,
  430,430,430,430,430,1,213,1,430,
431,431,431,431,431,431,431,431,431,431,431,431,431,431,431,431,431,431,431,
  431,431,431,431,431,431,431,431,431,431,431,431,431,431,431,431,431,431,
  431,431,431,431,431,1,214,1,431,
432,432,432,432,432,432,432,432,432,432,432,432,432,432,432,432,432,432,432,
  432,432,432,432,432,432,432,432,432,432,432,432,432,432,432,432,432,432,
  432,432,432,432,432,1,215,1,432,
433,433,433,433,433,433,433,433,433,433,433,433,433,433,433,433,433,433,433,
  433,433,433,433,433,433,433,433,433,433,433,433,433,433,433,433,433,433,
  433,433,433,433,433,1,216,1,433,
434,434,434,434,434,434,434,434,434,434,434,434,434,434,434,434,434,434,434,
  434,434,434,434,434,434,434,434,434,434,434,434,434,434,434,434,434,434,
  434,434,434,434,434,1,217,1,434,
435,435,435,435,435,435,435,435,435,435,435,435,435,435,435,435,435,435,435,
  435,435,435,435,435,435,435,435,435,435,435,435,435,435,435,435,435,435,
  435,435,435,435,435,1,218,1,435,
436,436,436,436,436,436,436,436,436,436,436,436,436,436,436,436,436,436,436,
  436,436,436,436,436,436,436,436,436,436,436,436,436,436,436,436,436,436,
  436,436,436,436,436,1,219,1,436,
437,437,437,437,437,437,437,437,437,437,437,437,437,437,437,437,437,437,437,
  437,437,437,437,437,437,437,437,437,437,437,437,437,437,437,437,437,437,
  437,437,437,437,437,1,220,1,437,
438,438,438,438,438,438,438,438,438,438,438,438,438,438,438,438,438,438,438,
  438,438,438,438,438,438,438,438,438,438,438,438,438,438,438,438,438,438,
  438,438,438,438,438,1,221,1,438,
439,439,439,439,439,439,439,439,439,439,439,439,439,439,439,439,439,439,439,
  439,439,439,439,439,439,439,439,439,439,439,439,439,439,439,439,439,439,
  439,439,439,439,439,1,222,1,439,
440,440,440,440,440,440,440,440,440,440,440,440,440,440,440,440,440,440,440,
  440,440,440,440,440,440,440,440,440,440,440,440,440,440,440,440,440,440,
  440,440,440,440,440,1,223,1,440,
441,441,441,441,441,441,441,441,441,441,441,441,441,441,441,441,441,441,441,
  441,441,441,441,441,441,441,441,441,441,441,441,441,441,441,441,441,441,
  441,441,441,441,441,1,224,1,441,
442,442,442,442,442,442,442,442,442,442,442,442,442,442,442,442,442,442,442,
  442,442,442,442,442,442,442,442,442,442,442,442,442,442,442,442,442,442,
  442,442,442,442,442,1,225,1,442,
443,443,443,443,443,443,443,443,443,443,443,443,443,443,443,443,443,443,443,
  443,443,443,443,443,443,443,443,443,443,443,443,443,443,443,443,443,443,
  443,443,443,443,443,1,226,1,443,
444,444,444,444,444,444,444,444,444,444,444,444,444,444,444,444,444,444,444,
  444,444,444,444,444,444,444,444,444,444,444,444,444,444,444,444,444,444,
  444,444,444,444,444,1,227,1,444,
445,445,445,445,445,445,445,445,445,445,445,445,445,445,445,445,445,445,445,
  445,445,445,445,445,445,445,445,445,445,445,445,445,445,445,445,445,445,
  445,445,445,445,445,1,228,1,445,
446,446,446,446,446,446,446,446,446,446,446,446,446,446,446,446,446,446,446,
  446,446,446,446,446,446,446,446,446,446,446,446,446,446,446,446,446,446,
  446,446,446,446,446,1,229,1,446,
447,447,447,447,447,447,447,447,447,447,447,447,447,447,447,447,447,447,447,
  447,447,447,447,447,447,447,447,447,447,447,447,447,447,447,447,447,447,
  447,447,447,447,447,1,230,1,447,
448,448,448,448,448,448,448,448,448,448,448,448,448,448,448,448,448,448,448,
  448,448,448,448,448,448,448,448,448,448,448,448,448,448,448,448,448,448,
  448,448,448,448,448,1,231,1,448,
449,449,449,449,449,449,449,449,449,449,449,449,449,449,449,449,449,449,449,
  449,449,449,449,449,449,449,449,449,449,449,449,449,449,449,449,449,449,
  449,449,449,449,449,1,232,1,449,
450,450,450,450,450,450,450,450,450,450,450,450,450,450,450,450,450,450,450,
  450,450,450,450,450,450,450,450,450,450,450,450,450,450,450,450,450,450,
  450,450,450,450,450,1,233,1,450,
451,451,451,451,451,451,451,451,451,451,451,451,451,451,451,451,451,451,451,
  451,451,451,451,451,451,451,451,451,451,451,451,451,451,451,451,451,451,
  451,451,451,451,451,1,234,1,451,
452,452,452,452,452,452,452,452,452,452,452,452,452,452,452,452,452,452,452,
  452,452,452,452,452,452,452,452,452,452,452,452,452,452,452,452,452,452,
  452,452,452,452,452,1,235,1,452,
453,453,453,453,453,453,453,453,453,453,453,453,453,453,453,453,453,453,453,
  453,453,453,453,453,453,453,453,453,453,453,453,453,453,453,453,453,453,
  453,453,453,453,453,1,236,1,453,
454,454,454,454,454,454,454,454,1,237,1,454,
455,455,455,455,455,455,455,455,1,238,1,455,
456,456,456,456,456,456,456,456,1,239,1,456,
457,457,457,457,457,457,457,457,1,240,1,457,
458,458,458,458,1,241,1,458,
459,459,459,459,1,242,1,459,
460,460,460,460,460,460,460,1,243,1,460,
461,461,461,461,461,461,461,1,244,1,461,
462,462,462,462,462,462,462,1,245,1,462,
463,463,463,463,463,463,463,463,463,1,246,1,463,
464,464,464,464,464,464,464,464,464,1,247,1,464,
465,465,465,465,465,465,465,465,465,1,248,1,465,
466,466,466,466,466,466,466,466,466,1,249,1,466,
467,467,467,467,467,467,467,467,467,1,250,1,467,
468,468,468,468,468,468,468,468,468,1,251,1,468,
469,469,469,469,469,469,469,469,469,1,252,1,469,
470,470,470,470,470,470,470,470,470,1,253,1,470,
471,471,471,471,471,471,471,471,471,1,254,1,471,
472,472,472,472,472,472,472,472,472,1,255,1,472,
473,473,473,473,473,473,473,473,473,1,256,1,473,
474,474,474,474,474,474,474,474,474,1,257,1,474,
475,475,475,475,475,475,475,475,475,1,258,1,475,
19,130,130,130,130,130,130,130,130,130,130,130,19,130,19,19,131,131,131,131,
  131,19,130,19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,
  19,19,19,19,19,19,19,130,19,19,19,19,1,259,1,894,131,
19,19,19,19,19,1,260,1,309,
61,61,61,61,61,61,61,61,61,61,61,61,61,61,61,61,61,61,61,61,61,61,61,61,61,
  61,61,61,61,61,61,61,302,61,61,61,61,61,61,261,476,
114,114,114,114,477,262,477,
478,113,479,
129,129,129,129,129,129,129,129,129,129,129,277,129,248,248,248,248,280,282,
  279,289,291,292,293,294,295,296,297,298,299,315,316,317,278,314,281,290,
  302,276,263,128,127,264,232,287,288,233,283,234,284,261,327,326,324,329,
  325,328,323,330,102,318,218,304,259,331,319,323,322,321,320,217,323,313,
  312,311,310,309,308,307,306,305,303,286,285,259,301,300,
290,480,482,484,485,486,488,489,490,492,494,496,498,500,502,504,506,508,510,
  511,513,514,515,517,518,520,521,522,523,526,527,335,337,339,341,343,265,
  70,71,525,72,74,65,66,524,524,524,524,79,519,519,519,516,516,516,516,
  512,512,512,509,507,505,503,501,499,497,495,493,491,105,106,487,109,110,
  483,481,115,
528,528,528,528,1,266,1,528,
59,59,59,59,59,59,59,59,59,59,59,59,59,59,59,59,59,59,59,59,59,59,59,59,59,
  59,59,59,59,59,59,59,59,59,59,59,302,59,59,59,59,267,529,
530,530,530,530,530,530,530,530,530,530,530,530,530,530,530,530,530,530,530,
  530,530,530,530,530,530,530,530,530,530,530,530,530,530,530,530,530,530,
  530,530,530,1,268,1,530,
531,531,531,531,1,269,1,531,
141,532,141,141,141,141,141,141,141,141,141,141,141,141,141,141,141,141,141,
  141,141,141,141,141,137,141,141,141,141,141,141,141,141,141,141,141,141,
  141,141,141,141,141,141,270,139,
19,19,19,19,19,19,19,19,271,1,887,
533,533,533,533,1,272,1,533,
37,38,39,34,273,54,54,42,54,41,40,40,
37,38,39,34,274,53,53,42,53,41,40,40,
534,534,534,534,1,275,1,534,
274,274,274,274,274,276,
259,259,259,259,277,
257,257,278,
129,129,129,129,129,129,129,253,253,253,253,129,253,253,253,253,253,129,129,
  129,129,129,129,129,129,129,129,129,129,129,129,129,129,129,129,129,129,
  129,129,129,129,129,129,129,129,129,129,129,129,129,129,129,129,129,129,
  129,129,129,279,
248,535,248,248,248,248,248,248,248,248,248,248,248,248,248,248,248,248,248,
  248,248,248,248,248,248,248,248,248,248,248,248,248,248,248,248,248,248,
  248,248,248,248,248,248,248,248,248,248,248,248,248,248,248,248,248,280,
246,246,246,246,246,281,
264,536,264,264,264,264,264,264,264,264,264,264,264,264,264,264,264,264,264,
  264,264,264,264,264,264,264,264,264,264,264,264,264,264,264,264,264,264,
  264,264,264,264,270,264,282,537,
275,275,275,275,275,272,
239,256,256,256,256,256,256,256,256,256,284,
260,260,260,260,238,238,238,238,238,238,238,238,238,238,238,238,238,238,238,
  238,238,238,238,238,238,238,238,238,238,238,238,238,238,238,238,238,238,
  238,238,238,238,238,285,
258,258,237,237,237,237,237,237,237,237,237,237,237,237,237,237,237,237,237,
  237,237,237,237,237,237,237,237,237,237,237,237,237,237,237,237,237,237,
  237,237,237,286,
254,254,254,254,254,254,254,254,254,236,236,236,236,236,236,236,236,236,236,
  236,236,236,236,236,236,236,236,236,236,236,236,236,236,236,236,236,236,
  236,236,236,236,236,236,236,236,236,236,287,
250,240,255,538,539,255,249,249,249,249,249,251,235,235,235,235,235,235,235,
  235,235,235,235,235,235,235,235,235,235,235,235,235,235,235,235,235,235,
  235,235,235,235,235,235,273,235,235,235,235,244,244,235,288,244,
19,19,289,1,990,
19,19,19,19,19,19,290,1,943,
19,19,291,1,988,
19,19,292,1,987,
19,19,293,1,986,
19,19,294,1,985,
19,19,295,1,984,
19,19,296,1,983,
19,19,297,1,982,
19,19,298,1,981,
19,19,299,1,980,
19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,
  19,19,19,19,19,19,19,19,19,19,19,19,19,300,1,991,
19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,
  19,19,19,19,19,19,19,19,19,19,19,19,1,301,1,944,
19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,
  19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,
  19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,302,1,909,
540,1,303,1,540,
541,1,304,1,541,
542,1,305,1,542,
543,1,306,1,543,
544,1,307,1,544,
545,1,308,1,545,
546,1,309,1,546,
547,1,310,1,547,
548,1,311,1,548,
549,1,312,1,549,
550,1,313,1,550,
19,19,19,19,19,19,19,19,19,19,19,19,19,247,247,247,247,247,19,19,19,19,19,
  19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,1,314,1,971,
19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,
  19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,
  19,19,19,19,19,19,19,19,19,19,19,19,19,315,1,979,
19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,
  19,19,19,19,19,19,19,19,19,19,19,19,19,19,1,316,1,978,
19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,
  19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,
  19,19,19,19,19,19,19,19,19,19,19,19,19,317,1,977,
551,551,551,551,551,551,551,551,551,551,551,551,551,551,551,551,551,551,551,
  551,551,551,551,551,551,551,551,551,551,551,551,551,551,551,551,551,551,
  551,551,551,551,551,1,318,1,551,
129,129,129,129,129,129,129,129,129,129,129,277,129,248,248,248,248,280,282,
  279,289,291,292,293,294,295,296,297,298,299,278,552,281,290,302,276,263,
  128,127,319,232,287,288,233,283,234,284,261,318,218,304,259,216,217,216,
  313,312,311,310,309,308,307,306,305,303,286,285,259,301,300,
129,129,129,129,129,129,129,129,129,129,129,277,129,248,248,248,248,280,282,
  279,289,291,292,293,294,295,296,297,298,299,278,552,281,290,302,276,263,
  128,127,320,232,287,288,233,283,234,284,261,318,218,304,259,215,217,215,
  313,312,311,310,309,308,307,306,305,303,286,285,259,301,300,
129,129,129,129,129,129,129,129,129,129,129,277,129,248,248,248,248,280,282,
  279,289,291,292,293,294,295,296,297,298,299,278,552,281,290,302,276,263,
  128,127,321,232,287,288,233,283,234,284,261,318,218,304,259,214,217,214,
  313,312,311,310,309,308,307,306,305,303,286,285,259,301,300,
129,129,129,129,129,129,129,129,129,129,129,277,129,248,248,248,248,280,282,
  279,289,291,292,293,294,295,296,297,298,299,278,552,281,290,302,276,263,
  128,127,322,232,287,288,233,283,234,284,261,318,218,304,259,213,217,213,
  313,312,311,310,309,308,307,306,305,303,286,285,259,301,300,
19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,
  19,19,19,19,19,19,19,19,19,19,19,19,1,323,1,206,
19,553,555,383,375,379,19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,
  19,19,19,19,19,19,19,19,19,19,19,19,1,324,1,203,559,558,557,556,554,
19,376,378,19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,
  19,19,19,19,19,19,19,19,1,325,1,198,561,560,
19,562,564,566,568,19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,
  19,19,19,19,19,19,1,326,1,195,569,567,565,563,
19,570,382,19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,
  19,19,1,327,1,192,572,571,
19,573,354,19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,1,
  328,1,189,575,574,
576,352,188,578,577,
579,580,581,582,583,584,585,586,587,588,589,590,591,168,168,168,168,168,330,
  172,172,172,172,175,175,175,178,178,178,181,181,181,184,184,184,187,187,
  187,
129,129,129,129,129,129,129,129,129,129,129,277,129,248,248,248,248,280,282,
  279,289,291,292,293,294,295,296,297,298,299,315,316,317,278,314,281,290,
  302,276,263,128,127,331,232,287,288,233,283,234,284,261,327,326,324,329,
  325,328,323,167,318,218,304,259,319,323,322,321,320,217,323,313,312,311,
  310,309,308,307,306,305,303,286,285,259,301,300,
592,592,592,592,1,332,1,592,
593,593,593,593,1,333,1,593,
594,594,594,594,594,334,594,
19,19,19,19,19,335,1,897,
595,595,595,595,1,336,1,595,
19,19,19,19,19,337,1,896,
596,596,596,596,1,338,1,596,
19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,339,1,895,
597,597,597,597,597,597,597,597,597,597,597,597,597,597,597,1,340,1,597,
19,19,19,19,19,341,1,893,
598,598,598,598,1,342,1,598,
19,19,19,19,19,343,1,891,
599,599,599,599,1,344,1,599,
157,157,600,157,157,157,157,157,157,157,157,157,157,157,157,155,157,157,157,
  157,157,157,157,157,157,157,157,157,157,157,157,157,157,157,157,157,157,
  157,157,157,157,157,345,
19,19,19,19,19,346,1,888,
601,601,601,601,1,347,1,601,
602,602,602,602,1,348,1,602,
19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,1,349,1,886,
132,132,132,132,132,132,132,132,132,132,132,132,156,132,132,132,132,132,138,
  132,132,350,606,270,605,345,604,271,346,603,
19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,
  19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,
  351,1,1159,
19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,
  19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,
  19,19,19,19,19,19,19,19,19,19,19,19,19,19,352,1,960,
19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,
  19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,
  353,1,1158,
19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,
  19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,
  19,19,19,19,19,19,19,19,19,19,19,19,19,19,354,1,962,
19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,
  19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,355,1,
  1157,
19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,
  19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,
  356,1,1156,
19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,
  19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,
  357,1,1155,
19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,
  19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,
  358,1,1154,
19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,
  19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,
  359,1,1153,
19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,
  19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,
  360,1,1152,
19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,
  19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,
  361,1,1151,
19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,
  19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,
  362,1,1150,
19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,
  19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,
  363,1,1149,
19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,
  19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,
  364,1,1148,
19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,
  19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,
  365,1,1147,
19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,
  19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,
  366,1,1146,
19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,
  19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,
  367,1,1145,
19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,
  19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,
  368,1,1144,
19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,
  19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,
  369,1,1143,
19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,
  19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,
  370,1,1142,
19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,
  19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,
  371,1,1141,
19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,
  19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,
  372,1,1140,
19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,
  19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,
  373,1,1139,
19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,
  19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,
  374,1,1138,
19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,
  19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,
  19,19,19,19,19,19,19,19,19,19,19,19,19,375,1,973,
19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,
  19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,
  19,19,19,19,19,19,19,19,19,19,19,19,19,19,376,1,971,
19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,
  19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,
  19,19,19,19,19,19,19,19,19,19,19,19,19,19,377,1,932,
19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,
  19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,
  19,19,19,19,19,19,19,19,19,19,19,19,19,19,378,1,970,
19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,
  19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,
  19,19,19,19,19,19,19,19,19,19,19,19,19,379,1,972,
19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,
  19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,
  19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,
  380,1,907,
19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,
  19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,
  381,1,1137,
19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,
  19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,
  19,19,19,19,19,19,19,19,19,19,19,19,19,19,382,1,964,
19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,
  19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,
  19,19,19,19,19,19,19,19,19,19,19,19,19,383,1,974,
19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,
  19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,
  384,1,1136,
19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,
  19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,385,1,
  1135,
19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,
  19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,
  386,1,1134,
19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,
  19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,
  387,1,1133,
19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,
  19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,
  19,19,19,19,19,19,19,388,1,879,
351,353,355,356,357,358,359,360,361,362,363,364,365,366,367,368,369,370,371,
  372,373,374,381,384,385,386,387,315,317,383,375,379,376,378,382,354,352,
  377,302,380,35,38,7,9,7,7,7,7,7,7,7,7,7,7,7,7,7,7,7,7,7,7,7,7,7,7,7,7,7,
  7,7,7,7,7,7,7,7,7,7,7,7,7,7,7,7,7,
34,390,11,
34,391,10,
290,480,482,484,485,486,488,489,490,492,494,496,498,500,502,504,506,508,510,
  511,513,514,515,517,518,520,521,522,523,526,527,335,337,339,341,343,607,
  609,392,610,608,70,71,525,72,74,65,66,524,524,524,524,79,519,519,519,
  516,516,516,516,512,512,512,509,507,505,503,501,499,497,495,493,491,105,
  106,487,109,110,483,481,115,
611,611,611,611,1,393,1,611,
612,290,37,38,39,34,394,613,67,67,42,67,41,40,40,165,166,
303,614,615,616,305,304,305,304,303,395,303,304,305,308,395,306,307,
285,284,284,281,280,279,278,277,276,396,393,284,617,
288,295,294,291,294,291,288,397,391,288,291,294,618,
397,397,397,397,398,397,
377,399,619,
377,400,620,
377,401,621,
129,129,129,129,129,129,129,129,129,129,129,277,129,248,248,248,248,280,282,
  279,289,291,292,293,294,295,296,297,298,299,315,316,317,278,314,281,290,
  302,276,263,128,127,402,232,287,288,233,283,234,284,261,327,326,324,329,
  325,328,323,465,318,218,304,259,319,323,322,321,320,217,323,313,312,311,
  310,309,308,307,306,305,303,286,285,259,301,300,
129,129,129,129,129,129,129,129,129,129,129,277,129,248,248,248,248,280,282,
  279,289,291,292,293,294,295,296,297,298,299,315,316,317,278,314,281,290,
  302,276,263,128,127,403,232,287,288,233,283,234,284,261,327,326,324,329,
  325,328,323,464,318,218,304,259,319,323,322,321,320,217,323,313,312,311,
  310,309,308,307,306,305,303,286,285,259,301,300,
129,129,129,129,129,129,129,129,129,129,129,277,129,248,248,248,248,280,282,
  279,289,291,292,293,294,295,296,297,298,299,315,316,317,278,314,281,290,
  302,276,263,128,127,404,232,287,288,233,283,234,284,261,327,326,324,329,
  325,328,323,463,318,218,304,259,319,323,322,321,320,217,323,313,312,311,
  310,309,308,307,306,305,303,286,285,259,301,300,
129,129,129,129,129,129,129,129,129,129,129,277,129,248,248,248,248,280,282,
  279,289,291,292,293,294,295,296,297,298,299,315,316,317,278,314,281,290,
  302,276,263,128,127,405,232,287,288,233,283,234,284,261,327,326,324,329,
  325,328,323,462,318,218,304,259,319,323,322,321,320,217,323,313,312,311,
  310,309,308,307,306,305,303,286,285,259,301,300,
129,129,129,129,129,129,129,129,129,129,129,277,129,248,248,248,248,280,282,
  279,289,291,292,293,294,295,296,297,298,299,315,316,317,278,314,281,290,
  302,276,263,128,127,406,232,287,288,233,283,234,284,261,327,326,324,329,
  325,328,323,461,318,218,304,259,319,323,322,321,320,217,323,313,312,311,
  310,309,308,307,306,305,303,286,285,259,301,300,
129,129,129,129,129,129,129,129,129,129,129,277,129,248,248,248,248,280,282,
  279,289,291,292,293,294,295,296,297,298,299,315,316,317,278,314,281,290,
  302,276,263,128,127,407,232,287,288,233,283,234,284,261,327,326,324,329,
  325,328,323,460,318,218,304,259,319,323,322,321,320,217,323,313,312,311,
  310,309,308,307,306,305,303,286,285,259,301,300,
129,129,129,129,129,129,129,129,129,129,129,277,129,248,248,248,248,280,282,
  279,289,291,292,293,294,295,296,297,298,299,315,316,317,278,314,281,290,
  302,276,263,128,127,408,232,287,288,233,283,234,284,261,327,326,324,329,
  325,328,323,459,318,218,304,259,319,323,322,321,320,217,323,313,312,311,
  310,309,308,307,306,305,303,286,285,259,301,300,
129,129,129,129,129,129,129,129,129,129,129,277,129,248,248,248,248,280,282,
  279,289,291,292,293,294,295,296,297,298,299,315,316,317,278,314,281,290,
  302,276,263,128,127,409,232,287,288,233,283,234,284,261,327,326,324,329,
  325,328,323,458,318,218,304,259,319,323,322,321,320,217,323,313,312,311,
  310,309,308,307,306,305,303,286,285,259,301,300,
129,129,129,129,129,129,129,129,129,129,129,277,129,248,248,248,248,280,282,
  279,289,291,292,293,294,295,296,297,298,299,315,316,317,278,314,281,290,
  302,276,263,128,127,410,232,287,288,233,283,234,284,261,327,326,324,329,
  325,328,323,457,318,218,304,259,319,323,322,321,320,217,323,313,312,311,
  310,309,308,307,306,305,303,286,285,259,301,300,
129,129,129,129,129,129,129,129,129,129,129,277,129,248,248,248,248,280,282,
  279,289,291,292,293,294,295,296,297,298,299,315,316,317,278,314,281,290,
  302,276,263,128,127,411,232,287,288,233,283,234,284,261,327,326,324,329,
  325,328,323,456,318,218,304,259,319,323,322,321,320,217,323,313,312,311,
  310,309,308,307,306,305,303,286,285,259,301,300,
129,129,129,129,129,129,129,129,129,129,129,277,129,248,248,248,248,280,282,
  279,289,291,292,293,294,295,296,297,298,299,315,316,317,278,314,281,290,
  302,276,263,128,127,412,232,287,288,233,283,234,284,261,327,326,324,329,
  325,328,323,455,318,218,304,259,319,323,322,321,320,217,323,313,312,311,
  310,309,308,307,306,305,303,286,285,259,301,300,
129,129,129,129,129,129,129,129,129,129,129,277,129,248,248,248,248,280,282,
  279,289,291,292,293,294,295,296,297,298,299,315,316,317,278,314,281,290,
  302,276,263,128,127,413,232,287,288,233,283,234,284,261,327,326,324,329,
  325,328,323,454,318,218,304,259,319,323,322,321,320,217,323,313,312,311,
  310,309,308,307,306,305,303,286,285,259,301,300,
129,129,129,129,129,129,129,129,129,129,129,277,129,248,248,248,248,280,282,
  279,289,291,292,293,294,295,296,297,298,299,315,316,317,278,314,281,290,
  302,276,263,128,127,414,232,287,288,233,283,234,284,261,327,326,324,329,
  325,328,323,453,318,218,304,259,319,323,322,321,320,217,323,313,312,311,
  310,309,308,307,306,305,303,286,285,259,301,300,
129,129,129,129,129,129,129,129,129,129,129,277,129,248,248,248,248,280,282,
  279,289,291,292,293,294,295,296,297,298,299,315,316,317,278,314,281,290,
  302,276,263,128,127,415,232,287,288,233,283,234,284,261,327,326,324,329,
  325,328,323,452,318,218,304,259,319,323,322,321,320,217,323,313,312,311,
  310,309,308,307,306,305,303,286,285,259,301,300,
129,129,129,129,129,129,129,129,129,129,129,277,129,248,248,248,248,280,282,
  279,289,291,292,293,294,295,296,297,298,299,315,316,317,278,314,281,290,
  302,276,263,128,127,416,232,287,288,233,283,234,284,261,327,326,324,329,
  325,328,323,451,318,218,304,259,319,323,322,321,320,217,323,313,312,311,
  310,309,308,307,306,305,303,286,285,259,301,300,
129,129,129,129,129,129,129,129,129,129,129,277,129,248,248,248,248,280,282,
  279,289,291,292,293,294,295,296,297,298,299,315,316,317,278,314,281,290,
  302,276,263,128,127,417,232,287,288,233,283,234,284,261,327,326,324,329,
  325,328,323,450,318,218,304,259,319,323,322,321,320,217,323,313,312,311,
  310,309,308,307,306,305,303,286,285,259,301,300,
129,129,129,129,129,129,129,129,129,129,129,277,129,248,248,248,248,280,282,
  279,289,291,292,293,294,295,296,297,298,299,315,316,317,278,314,281,290,
  302,276,263,128,127,418,232,287,288,233,283,234,284,261,327,326,324,329,
  325,328,323,449,318,218,304,259,319,323,322,321,320,217,323,313,312,311,
  310,309,308,307,306,305,303,286,285,259,301,300,
129,129,129,129,129,129,129,129,129,129,129,277,129,248,248,248,248,280,282,
  279,289,291,292,293,294,295,296,297,298,299,315,316,317,278,314,281,290,
  302,276,263,128,127,419,232,287,288,233,283,234,284,261,327,326,324,329,
  325,328,323,448,318,218,304,259,319,323,322,321,320,217,323,313,312,311,
  310,309,308,307,306,305,303,286,285,259,301,300,
129,129,129,129,129,129,129,129,129,129,129,277,129,248,248,248,248,280,282,
  279,289,291,292,293,294,295,296,297,298,299,315,316,317,278,314,281,290,
  302,276,263,128,127,420,232,287,288,233,283,234,284,261,327,326,324,329,
  325,328,323,447,318,218,304,259,319,323,322,321,320,217,323,313,312,311,
  310,309,308,307,306,305,303,286,285,259,301,300,
129,129,129,129,129,129,129,129,129,129,129,277,129,248,248,248,248,280,282,
  279,289,291,292,293,294,295,296,297,298,299,315,316,317,278,314,281,290,
  302,276,263,128,127,421,232,287,288,233,283,234,284,261,327,326,324,329,
  325,328,323,446,318,218,304,259,319,323,322,321,320,217,323,313,312,311,
  310,309,308,307,306,305,303,286,285,259,301,300,
129,129,129,129,129,129,129,129,129,129,129,277,129,248,248,248,248,280,282,
  279,289,291,292,293,294,295,296,297,298,299,315,316,317,278,314,281,290,
  302,276,263,128,127,422,232,287,288,233,283,234,284,261,327,326,324,329,
  325,328,323,445,318,218,304,259,319,323,322,321,320,217,323,313,312,311,
  310,309,308,307,306,305,303,286,285,259,301,300,
129,129,129,129,129,129,129,129,129,129,129,277,129,248,248,248,248,280,282,
  279,289,291,292,293,294,295,296,297,298,299,315,316,317,278,314,281,290,
  302,276,263,128,127,423,232,287,288,233,283,234,284,261,327,326,324,329,
  325,328,323,444,318,218,304,259,319,323,322,321,320,217,323,313,312,311,
  310,309,308,307,306,305,303,286,285,259,301,300,
129,129,129,129,129,129,129,129,129,129,129,277,129,248,248,248,248,280,282,
  279,289,291,292,293,294,295,296,297,298,299,315,316,317,278,314,281,290,
  302,276,263,128,127,424,232,287,288,233,283,234,284,261,327,326,324,329,
  325,328,323,443,318,218,304,259,319,323,322,321,320,217,323,313,312,311,
  310,309,308,307,306,305,303,286,285,259,301,300,
129,129,129,129,129,129,129,129,129,129,129,277,129,248,248,248,248,280,282,
  279,289,291,292,293,294,295,296,297,298,299,315,316,317,278,314,281,290,
  302,276,263,128,127,425,232,287,288,233,283,234,284,261,327,326,324,329,
  325,328,323,442,318,218,304,259,319,323,322,321,320,217,323,313,312,311,
  310,309,308,307,306,305,303,286,285,259,301,300,
129,129,129,129,129,129,129,129,129,129,129,277,129,248,248,248,248,280,282,
  279,289,291,292,293,294,295,296,297,298,299,315,316,317,278,314,281,290,
  302,276,263,128,127,426,232,287,288,233,283,234,284,261,327,326,324,329,
  325,328,323,441,318,218,304,259,319,323,322,321,320,217,323,313,312,311,
  310,309,308,307,306,305,303,286,285,259,301,300,
129,129,129,129,129,129,129,129,129,129,129,277,129,248,248,248,248,280,282,
  279,289,291,292,293,294,295,296,297,298,299,315,316,317,278,314,281,290,
  302,276,263,128,127,427,232,287,288,233,283,234,284,261,327,326,324,329,
  325,328,323,437,318,218,304,259,319,323,322,321,320,217,323,313,312,311,
  310,309,308,307,306,305,303,286,285,259,301,300,
129,129,129,129,129,129,129,129,129,129,129,277,129,248,248,248,248,280,282,
  279,289,291,292,293,294,295,296,297,298,299,315,316,317,278,314,281,290,
  302,276,263,128,127,428,232,287,288,233,283,234,284,261,327,326,324,329,
  325,328,323,433,318,218,304,259,319,323,322,321,320,217,323,313,312,311,
  310,309,308,307,306,305,303,286,285,259,301,300,
129,129,129,129,129,129,129,129,129,129,129,277,129,248,248,248,248,280,282,
  279,289,291,292,293,294,295,296,297,298,299,315,316,317,278,314,281,290,
  302,276,263,128,127,429,232,287,288,233,283,234,284,261,327,326,324,329,
  325,328,323,432,318,218,304,259,319,323,322,321,320,217,323,313,312,311,
  310,309,308,307,306,305,303,286,285,259,301,300,
129,129,129,129,129,129,129,129,129,129,129,277,129,248,248,248,248,280,282,
  279,289,291,292,293,294,295,296,297,298,299,315,316,317,278,314,281,290,
  302,276,263,128,127,430,232,287,288,233,283,234,284,261,327,326,324,329,
  325,328,323,431,318,218,304,259,319,323,322,321,320,217,323,313,312,311,
  310,309,308,307,306,305,303,286,285,259,301,300,
129,129,129,129,129,129,129,129,129,129,129,277,129,248,248,248,248,280,282,
  279,289,291,292,293,294,295,296,297,298,299,315,316,317,278,314,281,290,
  302,276,263,128,127,431,232,287,288,233,283,234,284,261,327,326,324,329,
  325,328,323,430,318,218,304,259,319,323,322,321,320,217,323,313,312,311,
  310,309,308,307,306,305,303,286,285,259,301,300,
129,129,129,129,129,129,129,129,129,129,129,277,129,248,248,248,248,280,282,
  279,289,291,292,293,294,295,296,297,298,299,315,316,317,278,314,281,290,
  302,276,263,128,127,432,232,287,288,233,283,234,284,261,327,326,324,329,
  325,328,323,429,318,218,304,259,319,323,322,321,320,217,323,313,312,311,
  310,309,308,307,306,305,303,286,285,259,301,300,
129,129,129,129,129,129,129,129,129,129,129,277,129,248,248,248,248,280,282,
  279,289,291,292,293,294,295,296,297,298,299,315,316,317,278,314,281,290,
  302,276,263,128,127,433,232,287,288,233,283,234,284,261,327,326,324,329,
  325,328,323,428,318,218,304,259,319,323,322,321,320,217,323,313,312,311,
  310,309,308,307,306,305,303,286,285,259,301,300,
129,129,129,129,129,129,129,129,129,129,129,277,129,248,248,248,248,280,282,
  279,289,291,292,293,294,295,296,297,298,299,315,316,317,278,314,281,290,
  302,276,263,128,127,434,232,287,288,233,283,234,284,261,327,326,324,329,
  325,328,323,427,318,218,304,259,319,323,322,321,320,217,323,313,312,311,
  310,309,308,307,306,305,303,286,285,259,301,300,
129,129,129,129,129,129,129,129,129,129,129,277,129,248,248,248,248,280,282,
  279,289,291,292,293,294,295,296,297,298,299,315,316,317,278,314,281,290,
  302,276,263,128,127,435,232,287,288,233,283,234,284,261,327,326,324,329,
  325,328,323,421,318,218,304,259,319,323,322,321,320,217,323,313,312,311,
  310,309,308,307,306,305,303,286,285,259,301,300,
129,129,129,129,129,129,129,129,129,129,129,277,129,248,248,248,248,280,282,
  279,289,291,292,293,294,295,296,297,298,299,315,316,317,278,314,281,290,
  302,276,263,128,127,436,232,287,288,233,283,234,284,261,327,326,324,329,
  325,328,323,420,318,218,304,259,319,323,322,321,320,217,323,313,312,311,
  310,309,308,307,306,305,303,286,285,259,301,300,
129,129,129,129,129,129,129,129,129,129,129,277,129,248,248,248,248,280,282,
  279,289,291,292,293,294,295,296,297,298,299,315,316,317,278,314,281,290,
  302,276,263,128,127,437,232,287,288,233,283,234,284,261,327,326,324,329,
  325,328,323,419,318,218,304,259,319,323,322,321,320,217,323,313,312,311,
  310,309,308,307,306,305,303,286,285,259,301,300,
129,129,129,129,129,129,129,129,129,129,129,277,129,248,248,248,248,280,282,
  279,289,291,292,293,294,295,296,297,298,299,315,316,317,278,314,281,290,
  302,276,263,128,127,438,232,287,288,233,283,234,284,261,327,326,324,329,
  325,328,323,418,318,218,304,259,319,323,322,321,320,217,323,313,312,311,
  310,309,308,307,306,305,303,286,285,259,301,300,
129,129,129,129,129,129,129,129,129,129,129,277,129,248,248,248,248,280,282,
  279,289,291,292,293,294,295,296,297,298,299,315,316,317,278,314,281,290,
  302,276,263,128,127,439,232,287,288,233,283,234,284,261,327,326,324,329,
  325,328,323,412,318,218,304,259,319,323,322,321,320,217,323,313,312,311,
  310,309,308,307,306,305,303,286,285,259,301,300,
129,129,129,129,129,129,129,129,129,129,129,277,129,248,248,248,248,280,282,
  279,289,291,292,293,294,295,296,297,298,299,315,316,317,278,314,281,290,
  302,276,263,128,127,440,232,287,288,233,283,234,284,261,327,326,324,329,
  325,328,323,411,318,218,304,259,319,323,322,321,320,217,323,313,312,311,
  310,309,308,307,306,305,303,286,285,259,301,300,
129,129,129,129,129,129,129,129,129,129,129,277,129,248,248,248,248,280,282,
  279,289,291,292,293,294,295,296,297,298,299,315,316,317,278,314,281,290,
  302,276,263,128,127,441,232,287,288,233,283,234,284,261,327,326,324,329,
  325,328,323,410,318,218,304,259,319,323,322,321,320,217,323,313,312,311,
  310,309,308,307,306,305,303,286,285,259,301,300,
129,129,129,129,129,129,129,129,129,129,129,277,129,248,248,248,248,280,282,
  279,289,291,292,293,294,295,296,297,298,299,315,316,317,278,314,281,290,
  302,276,263,128,127,442,232,287,288,233,283,234,284,261,327,326,324,329,
  325,328,323,409,318,218,304,259,319,323,322,321,320,217,323,313,312,311,
  310,309,308,307,306,305,303,286,285,259,301,300,
129,129,129,129,129,129,129,129,129,129,129,277,129,248,248,248,248,280,282,
  279,289,291,292,293,294,295,296,297,298,299,315,316,317,278,314,281,290,
  302,276,263,128,127,443,232,287,288,233,283,234,284,261,327,326,324,329,
  325,328,323,408,318,218,304,259,319,323,322,321,320,217,323,313,312,311,
  310,309,308,307,306,305,303,286,285,259,301,300,
129,129,129,129,129,129,129,129,129,129,129,277,129,248,248,248,248,280,282,
  279,289,291,292,293,294,295,296,297,298,299,315,316,317,278,314,281,290,
  302,276,263,128,127,444,232,287,288,233,283,234,284,261,327,326,324,329,
  325,328,323,407,318,218,304,259,319,323,322,321,320,217,323,313,312,311,
  310,309,308,307,306,305,303,286,285,259,301,300,
129,129,129,129,129,129,129,129,129,129,129,277,129,248,248,248,248,280,282,
  279,289,291,292,293,294,295,296,297,298,299,315,316,317,278,314,281,290,
  302,276,263,128,127,445,232,287,288,233,283,234,284,261,327,326,324,329,
  325,328,323,406,318,218,304,259,319,323,322,321,320,217,323,313,312,311,
  310,309,308,307,306,305,303,286,285,259,301,300,
129,129,129,129,129,129,129,129,129,129,129,277,129,248,248,248,248,280,282,
  279,289,291,292,293,294,295,296,297,298,299,315,316,317,278,314,281,290,
  302,276,263,128,127,446,232,287,288,233,283,234,284,261,327,326,324,329,
  325,328,323,405,318,218,304,259,319,323,322,321,320,217,323,313,312,311,
  310,309,308,307,306,305,303,286,285,259,301,300,
129,129,129,129,129,129,129,129,129,129,129,277,129,248,248,248,248,280,282,
  279,289,291,292,293,294,295,296,297,298,299,315,316,317,278,314,281,290,
  302,276,263,128,127,447,232,287,288,233,283,234,284,261,327,326,324,329,
  325,328,323,404,318,218,304,259,319,323,322,321,320,217,323,313,312,311,
  310,309,308,307,306,305,303,286,285,259,301,300,
129,129,129,129,129,129,129,129,129,129,129,277,129,248,248,248,248,280,282,
  279,289,291,292,293,294,295,296,297,298,299,315,316,317,278,314,281,290,
  302,276,263,128,127,448,232,287,288,233,283,234,284,261,327,326,324,329,
  325,328,323,403,318,218,304,259,319,323,322,321,320,217,323,313,312,311,
  310,309,308,307,306,305,303,286,285,259,301,300,
129,129,129,129,129,129,129,129,129,129,129,277,129,248,248,248,248,280,282,
  279,289,291,292,293,294,295,296,297,298,299,315,316,317,278,314,281,290,
  302,276,263,128,127,449,232,287,288,233,283,234,284,261,327,326,324,329,
  325,328,323,402,318,218,304,259,319,323,322,321,320,217,323,313,312,311,
  310,309,308,307,306,305,303,286,285,259,301,300,
129,129,129,129,129,129,129,129,129,129,129,277,129,248,248,248,248,280,282,
  279,289,291,292,293,294,295,296,297,298,299,315,316,317,278,314,281,290,
  302,276,263,128,127,450,232,287,288,233,283,234,284,261,327,326,324,329,
  325,328,323,401,318,218,304,259,319,323,322,321,320,217,323,313,312,311,
  310,309,308,307,306,305,303,286,285,259,301,300,
129,129,129,129,129,129,129,129,129,129,129,277,129,248,248,248,248,280,282,
  279,289,291,292,293,294,295,296,297,298,299,315,316,317,278,314,281,290,
  302,276,263,128,127,451,232,287,288,233,283,234,284,261,327,326,324,329,
  325,328,323,400,318,218,304,259,319,323,322,321,320,217,323,313,312,311,
  310,309,308,307,306,305,303,286,285,259,301,300,
129,129,129,129,129,129,129,129,129,129,129,277,129,248,248,248,248,280,282,
  279,289,291,292,293,294,295,296,297,298,299,315,316,317,278,314,281,290,
  302,276,263,128,127,452,232,287,288,233,283,234,284,261,327,326,324,329,
  325,328,323,399,318,218,304,259,319,323,322,321,320,217,323,313,312,311,
  310,309,308,307,306,305,303,286,285,259,301,300,
129,129,129,129,129,129,129,129,129,129,129,277,129,248,248,248,248,280,282,
  279,289,291,292,293,294,295,296,297,298,299,315,316,317,278,314,281,290,
  302,276,263,128,127,453,232,287,288,233,283,234,284,261,327,326,324,329,
  325,328,323,398,318,218,304,259,319,323,322,321,320,217,323,313,312,311,
  310,309,308,307,306,305,303,286,285,259,301,300,
296,622,623,298,297,298,297,296,454,300,296,297,298,374,299,
296,622,623,298,297,298,297,296,455,300,296,297,298,373,299,
296,622,623,298,297,298,297,296,456,300,296,297,298,372,299,
296,622,623,298,297,298,297,296,457,300,296,297,298,371,299,
301,302,302,301,458,301,302,370,
301,302,302,301,459,301,302,369,
288,295,294,291,294,291,288,460,390,288,291,294,618,
288,295,294,291,294,291,288,461,389,288,291,294,618,
288,295,294,291,294,291,288,462,388,288,291,294,618,
303,614,615,616,305,304,305,304,303,463,303,304,305,308,387,306,307,
303,614,615,616,305,304,305,304,303,464,303,304,305,308,386,306,307,
285,284,284,281,280,279,278,277,276,465,385,284,617,
285,284,284,281,280,279,278,277,276,466,384,284,617,
285,284,284,281,280,279,278,277,276,467,383,284,617,
285,284,284,281,280,279,278,277,276,468,382,284,617,
285,284,284,281,280,279,278,277,276,469,624,284,617,
285,284,284,281,280,279,278,277,276,470,380,284,617,
285,284,284,281,280,279,278,277,276,471,379,284,617,
285,284,284,281,280,279,278,277,276,472,378,284,617,
285,284,284,281,280,279,278,277,276,473,377,284,617,
285,284,284,281,280,279,278,277,276,474,376,284,617,
285,284,284,281,280,279,278,277,276,475,375,284,617,
129,129,129,129,129,129,129,129,129,129,129,277,129,248,248,248,248,280,625,
  279,138,289,291,292,293,294,295,296,297,298,299,315,316,317,278,314,281,
  290,302,276,263,128,127,476,232,287,288,233,283,117,270,234,284,626,118,
  261,327,326,324,329,325,328,323,119,318,218,628,304,259,319,323,322,321,
  320,217,323,313,312,311,310,309,308,307,306,305,303,286,285,271,259,627,
  301,300,
277,248,248,248,248,280,282,629,278,552,281,263,18,477,116,287,630,234,284,
  286,285,301,
19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,
  19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,
  19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,
  19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,
  19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,
  19,19,19,19,19,19,19,19,19,19,19,19,19,478,1,997,
631,631,631,631,631,631,631,631,631,631,631,631,631,631,631,631,631,631,631,
  631,631,631,631,631,631,631,631,631,631,631,631,631,631,631,631,631,631,
  631,631,631,631,631,631,631,631,631,631,631,631,631,631,631,631,631,631,
  631,631,631,631,631,631,631,631,631,631,631,631,631,631,631,631,631,631,
  631,631,631,631,631,631,631,631,631,631,631,631,631,631,631,631,631,631,
  631,631,631,631,631,631,631,631,631,631,631,631,631,631,631,631,631,631,
  631,631,631,631,631,631,631,631,631,631,631,631,631,631,631,631,631,631,
  631,631,631,631,631,631,1,479,1,631,
19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,1,480,1,942,
129,129,129,129,129,129,129,129,129,129,129,129,129,128,127,481,112,259,259,
19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,1,482,1,941,
132,132,132,132,132,132,132,132,132,132,132,132,132,132,132,132,132,132,132,
  483,111,603,
19,19,19,19,19,484,1,940,
19,19,19,19,19,485,1,939,
19,19,1,486,1,938,
632,138,487,108,270,626,107,271,627,
19,19,19,19,19,488,1,936,
19,19,19,19,19,489,1,935,
19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,490,1,934,
129,129,129,129,129,129,129,129,129,129,129,129,129,100,100,100,100,128,127,
  100,491,101,259,259,
19,1,492,1,933,
138,493,633,270,271,
19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,
  19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,494,1,931,
634,634,634,634,634,634,634,634,634,634,634,634,634,634,634,634,634,634,634,
  634,634,634,634,634,634,634,634,634,634,634,634,634,634,634,634,634,634,
  634,634,634,634,634,634,1,495,1,634,
19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,
  19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,1,496,1,930,
129,129,129,129,129,129,129,129,129,129,129,277,129,248,248,248,248,280,282,
  279,138,289,291,292,293,294,295,296,297,298,299,315,316,317,278,314,281,
  290,302,276,263,128,127,497,232,287,288,233,283,97,270,234,284,261,327,
  326,324,329,325,328,323,96,318,218,304,259,319,323,322,321,320,217,323,
  313,312,311,310,309,308,307,306,305,303,286,285,271,259,301,300,
19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,
  19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,1,498,1,929,
129,129,129,129,129,129,129,129,129,129,129,277,129,248,248,248,248,280,282,
  279,289,291,292,293,294,295,296,297,298,299,315,316,317,278,314,281,290,
  302,276,263,128,127,499,232,287,288,233,283,234,284,261,327,326,324,329,
  325,328,323,95,318,218,304,259,319,323,322,321,320,217,323,313,312,311,
  310,309,308,307,306,305,303,286,285,259,301,300,
19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,1,500,1,928,
129,129,129,129,129,129,129,129,129,129,129,129,129,128,127,501,94,259,259,
19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,1,502,1,927,
129,129,129,129,129,129,129,129,129,129,129,129,129,128,127,503,93,259,259,
19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,1,504,1,926,
129,129,129,129,129,129,129,129,129,129,129,129,129,128,127,505,123,635,259,
  259,
19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,1,506,1,925,
129,129,129,129,129,129,129,129,129,129,129,129,129,128,127,507,123,636,259,
  259,
19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,1,508,1,924,
129,129,129,129,129,129,129,129,129,129,129,129,129,128,127,509,123,637,259,
  259,
19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,
  19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,1,510,1,923,
19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,
  19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,1,511,1,922,
129,129,129,129,129,129,129,129,129,129,129,277,129,248,248,248,248,280,625,
  279,138,289,291,292,293,294,295,296,297,298,299,315,316,317,278,314,281,
  290,302,276,263,128,127,512,232,287,288,233,283,117,270,234,284,626,118,
  261,327,326,324,329,325,328,323,119,318,218,638,304,259,319,323,322,321,
  320,217,323,313,312,311,310,309,308,307,306,305,303,286,285,271,259,627,
  301,300,
19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,
  19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,1,513,1,921,
19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,
  19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,1,514,1,920,
19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,
  19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,1,515,1,919,
129,129,129,129,129,129,129,129,129,129,129,277,129,248,248,248,248,280,625,
  279,138,289,291,292,293,294,295,296,297,298,299,315,316,317,278,314,281,
  290,302,276,263,128,127,516,232,287,288,233,283,117,270,234,284,626,118,
  261,327,326,324,329,325,328,323,119,318,218,639,304,259,319,323,322,321,
  320,217,323,313,312,311,310,309,308,307,306,305,303,286,285,271,259,627,
  301,300,
19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,
  19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,1,517,1,918,
19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,
  19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,1,518,1,917,
129,129,129,129,129,129,129,129,129,129,129,277,129,248,248,248,248,280,282,
  279,289,291,292,293,294,295,296,297,298,299,315,316,317,278,314,281,290,
  302,276,263,128,127,519,232,287,288,233,283,234,284,261,327,326,324,329,
  325,328,323,82,318,218,304,259,319,323,322,321,320,217,323,313,312,311,
  310,309,308,307,306,305,303,286,285,259,301,300,
19,19,19,19,19,520,1,916,
19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,
  19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,1,521,1,915,
19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,
  19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,1,522,1,914,
19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,
  19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,1,523,1,913,
129,129,129,129,129,129,129,129,129,129,129,277,129,248,248,248,248,280,282,
  279,289,291,292,293,294,295,296,297,298,299,315,316,317,278,314,281,290,
  302,276,263,128,127,524,232,287,288,233,283,234,284,261,327,326,324,329,
  325,328,323,78,318,218,304,259,319,323,322,321,320,217,323,313,312,311,
  310,309,308,307,306,305,303,286,285,259,301,300,
129,129,129,129,129,129,129,129,129,129,129,129,129,128,127,525,73,259,259,
19,19,19,19,19,19,19,526,1,912,
19,19,19,19,19,19,19,527,1,911,
37,38,39,34,528,21,21,42,21,41,40,40,
640,529,641,
351,353,356,357,358,359,360,361,362,363,364,365,366,367,368,369,370,371,372,
  373,374,381,384,386,387,315,317,383,375,379,376,378,382,354,352,377,302,
  380,35,38,530,62,62,642,62,62,62,62,62,62,62,62,62,62,62,62,62,62,62,62,
  62,62,62,62,62,62,62,62,62,62,62,62,62,62,62,62,62,62,62,62,62,62,
37,38,39,34,531,56,56,42,56,41,40,40,
142,643,144,145,143,149,148,644,644,645,643,147,140,532,643,
37,38,39,34,533,55,55,42,55,41,40,40,
37,38,39,34,534,52,52,42,52,41,40,40,
252,252,252,252,252,252,252,252,252,535,
265,267,268,266,269,271,536,
264,536,264,264,264,264,264,264,264,264,264,264,264,264,264,264,264,264,264,
  264,264,261,264,264,264,264,264,264,264,264,264,264,264,264,264,264,264,
  264,264,264,264,264,270,264,537,646,
255,255,255,255,255,255,255,255,255,255,245,245,245,245,245,245,245,245,245,
  245,245,245,245,245,245,245,245,245,245,245,245,245,245,245,245,245,245,
  245,245,245,245,245,245,245,245,245,245,245,538,
255,255,255,255,255,255,255,255,255,255,241,241,241,241,241,241,241,241,241,
  241,241,241,241,241,241,241,241,241,241,241,241,241,241,241,241,241,241,
  241,241,241,241,241,241,241,241,241,241,241,539,
302,540,647,
302,541,648,
302,542,649,
302,543,650,
302,544,651,
302,545,652,
302,546,653,
302,547,654,
302,548,655,
302,549,656,
302,550,657,
129,129,129,129,129,129,129,129,129,129,129,277,129,248,248,248,248,280,282,
  279,289,291,292,293,294,295,296,297,298,299,315,316,317,278,314,281,290,
  302,276,263,128,127,551,232,287,288,233,283,234,284,261,327,326,324,329,
  325,328,323,658,318,218,304,259,319,323,322,321,320,217,323,313,312,311,
  310,309,308,307,306,305,303,286,285,259,301,300,
247,247,247,247,247,552,
19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,
  19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,553,1,976,
659,659,659,659,659,659,659,659,659,659,659,659,659,659,659,659,659,659,659,
  659,659,659,659,659,659,659,659,659,659,659,659,659,659,659,659,659,659,
  659,659,1,554,1,659,
19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,
  19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,555,1,975,
660,660,660,660,660,660,660,660,660,660,660,660,660,660,660,660,660,660,660,
  660,660,660,660,660,660,660,660,660,660,660,660,660,660,660,660,660,660,
  660,660,1,556,1,660,
661,661,661,661,661,661,661,661,661,661,661,661,661,661,661,661,661,661,661,
  661,661,661,661,661,661,661,661,661,661,661,661,661,661,661,661,661,661,
  661,661,1,557,1,661,
662,662,662,662,662,662,662,662,662,662,662,662,662,662,662,662,662,662,662,
  662,662,662,662,662,662,662,662,662,662,662,662,662,662,662,662,662,662,
  662,662,1,558,1,662,
663,663,663,663,663,663,663,663,663,663,663,663,663,663,663,663,663,663,663,
  663,663,663,663,663,663,663,663,663,663,663,663,663,663,663,663,663,663,
  663,663,1,559,1,663,
664,664,664,664,664,664,664,664,664,664,664,664,664,664,664,664,664,664,664,
  664,664,664,664,664,664,664,664,664,664,664,664,664,664,664,664,664,664,
  664,664,664,664,664,1,560,1,664,
665,665,665,665,665,665,665,665,665,665,665,665,665,665,665,665,665,665,665,
  665,665,665,665,665,665,665,665,665,665,665,665,665,665,665,665,665,665,
  665,665,665,665,665,1,561,1,665,
19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,
  19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,562,1,969,
666,666,666,666,666,666,666,666,666,666,666,666,666,666,666,666,666,666,666,
  666,666,666,666,666,666,666,666,666,666,666,666,666,666,666,666,666,666,
  666,666,666,666,666,1,563,1,666,
19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,
  19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,564,1,968,
667,667,667,667,667,667,667,667,667,667,667,667,667,667,667,667,667,667,667,
  667,667,667,667,667,667,667,667,667,667,667,667,667,667,667,667,667,667,
  667,667,667,667,667,1,565,1,667,
19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,
  19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,566,1,967,
668,668,668,668,668,668,668,668,668,668,668,668,668,668,668,668,668,668,668,
  668,668,668,668,668,668,668,668,668,668,668,668,668,668,668,668,668,668,
  668,668,668,668,668,1,567,1,668,
19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,
  19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,568,1,966,
669,669,669,669,669,669,669,669,669,669,669,669,669,669,669,669,669,669,669,
  669,669,669,669,669,669,669,669,669,669,669,669,669,669,669,669,669,669,
  669,669,669,669,669,1,569,1,669,
19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,
  19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,570,1,965,
670,670,670,670,670,670,670,670,670,670,670,670,670,670,670,670,670,670,670,
  670,670,670,670,670,670,670,670,670,670,670,670,670,670,670,670,670,670,
  670,670,670,670,670,1,571,1,670,
671,671,671,671,671,671,671,671,671,671,671,671,671,671,671,671,671,671,671,
  671,671,671,671,671,671,671,671,671,671,671,671,671,671,671,671,671,671,
  671,671,671,671,671,1,572,1,671,
19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,
  19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,573,1,963,
672,672,672,672,672,672,672,672,672,672,672,672,672,672,672,672,672,672,672,
  672,672,672,672,672,672,672,672,672,672,672,672,672,672,672,672,672,672,
  672,672,672,672,672,1,574,1,672,
673,673,673,673,673,673,673,673,673,673,673,673,673,673,673,673,673,673,673,
  673,673,673,673,673,673,673,673,673,673,673,673,673,673,673,673,673,673,
  673,673,673,673,673,1,575,1,673,
19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,
  19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,576,1,961,
674,674,674,674,674,674,674,674,674,674,674,674,674,674,674,674,674,674,674,
  674,674,674,674,674,674,674,674,674,674,674,674,674,674,674,674,674,674,
  674,674,674,674,674,1,577,1,674,
675,675,675,675,675,675,675,675,675,675,675,675,675,675,675,675,675,675,675,
  675,675,675,675,675,675,675,675,675,675,675,675,675,675,675,675,675,675,
  675,675,675,675,675,1,578,1,675,
19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,
  19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,1,579,1,959,
19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,
  19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,1,580,1,958,
19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,
  19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,1,581,1,957,
19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,
  19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,1,582,1,956,
19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,
  19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,1,583,1,955,
19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,
  19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,1,584,1,954,
19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,
  19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,1,585,1,953,
19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,
  19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,1,586,1,952,
19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,
  19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,1,587,1,951,
19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,
  19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,1,588,1,950,
19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,
  19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,1,589,1,949,
19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,
  19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,1,590,1,948,
19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,
  19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,1,591,1,947,
37,38,39,34,592,51,51,42,51,41,40,40,
37,38,39,34,593,50,50,42,50,41,40,40,
37,38,39,34,594,49,49,42,49,41,40,40,
37,38,39,34,595,46,46,42,46,41,40,40,
37,38,39,34,596,45,45,42,45,41,40,40,
129,129,129,129,129,129,129,129,129,129,129,129,129,128,127,597,676,259,259,
37,38,39,34,598,43,43,42,43,41,40,40,
37,38,39,34,599,42,42,42,42,41,40,40,
158,158,158,158,158,158,158,158,158,158,158,158,158,158,158,158,158,158,158,
  158,158,158,158,158,158,158,158,158,158,158,158,158,158,158,158,158,158,
  158,158,158,158,158,159,158,600,
37,38,39,34,601,41,41,42,41,41,40,40,
37,38,39,34,602,40,40,42,40,41,40,40,
133,133,133,133,133,133,133,133,133,133,133,133,133,133,133,133,133,133,133,
  19,19,19,603,1,889,
677,677,677,677,1,604,1,677,
678,678,678,678,1,605,1,678,
679,679,679,679,1,606,1,679,
19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,
  19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,1,607,1,884,
129,129,129,129,129,129,129,129,129,129,129,277,129,248,248,248,248,280,282,
  279,289,291,292,293,294,295,296,297,298,299,315,316,317,278,314,281,290,
  302,276,263,128,127,608,232,287,288,233,283,234,284,261,327,326,324,329,
  325,328,323,680,318,218,304,259,319,323,322,321,320,217,323,313,312,311,
  310,309,308,307,306,305,303,286,285,259,301,300,
19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,
  19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,1,609,1,883,
129,129,129,129,129,129,129,129,129,129,129,277,129,248,248,248,248,280,282,
  279,289,291,292,293,294,295,296,297,298,299,315,316,317,278,314,281,290,
  302,276,263,128,127,610,232,287,288,233,283,234,284,261,327,326,324,329,
  325,328,323,681,318,218,304,259,319,323,322,321,320,217,323,313,312,311,
  310,309,308,307,306,305,303,286,285,259,301,300,
37,38,39,34,611,22,22,42,22,41,40,40,
19,19,19,19,19,612,1,946,
682,682,682,682,1,613,1,682,
19,19,19,19,19,19,19,614,1,996,
19,19,19,19,19,19,19,615,1,995,
19,19,19,19,19,19,19,616,1,994,
19,19,19,19,19,19,19,19,617,1,1047,
19,19,19,19,19,19,19,618,1,1061,
683,683,683,683,683,683,683,683,683,683,683,683,683,683,683,683,683,683,683,
  683,683,683,683,683,683,683,683,683,683,683,683,683,683,683,683,683,683,
  683,683,683,683,683,1,619,1,683,
684,684,684,684,684,684,684,684,684,684,684,684,684,684,684,684,684,684,684,
  684,684,684,684,684,684,684,684,684,684,684,684,684,684,684,684,684,684,
  684,684,684,684,684,1,620,1,684,
685,685,685,685,685,685,685,685,685,685,685,685,685,685,685,685,685,685,685,
  685,685,685,685,685,685,685,685,685,685,685,685,685,685,685,685,685,685,
  685,685,685,685,685,1,621,1,685,
19,19,19,19,19,19,622,1,993,
19,19,19,19,19,19,623,1,992,
686,1,624,1,686,
264,536,264,264,264,264,264,264,264,264,264,264,264,264,264,264,264,264,264,
  264,264,264,264,264,264,264,264,264,264,264,264,264,264,264,264,264,264,
  264,264,264,264,270,264,625,687,
264,536,264,264,264,264,264,264,264,264,264,264,264,264,264,264,264,264,264,
  264,264,134,264,264,264,264,264,264,264,264,264,264,264,264,264,264,264,
  264,264,264,264,264,270,264,626,136,
19,19,19,19,19,19,19,627,1,937,
688,380,628,60,689,
253,253,253,253,253,253,253,253,253,629,
250,240,255,538,539,255,249,249,249,249,249,251,235,235,235,235,244,244,235,
  630,244,
111,112,113,114,115,116,117,118,119,120,121,122,123,124,125,126,127,128,129,
  130,131,132,133,134,62,63,64,65,66,67,135,136,137,138,139,140,68,69,70,
  71,72,141,142,143,73,74,75,76,77,144,145,146,147,148,149,150,151,152,
  153,154,155,156,157,158,107,59,60,61,165,166,167,168,169,170,171,172,
  173,174,175,176,177,178,179,180,159,160,161,162,163,164,78,79,80,81,82,
  83,45,46,47,84,85,86,87,88,89,90,91,92,48,49,93,94,95,96,97,98,99,100,
  50,51,52,101,102,53,54,103,104,105,106,55,56,57,58,631,690,691,196,691,
  691,691,691,691,691,691,691,691,691,325,325,325,325,325,326,327,328,329,
  332,332,332,333,334,338,338,338,338,339,340,341,342,343,344,345,346,349,
  349,349,350,351,352,353,354,355,356,357,358,362,362,362,362,363,364,365,
  366,367,368,242,241,240,239,238,237,258,257,256,255,254,253,252,251,250,
  249,248,247,246,245,244,243,184,110,183,109,182,108,181,236,235,234,233,
  232,231,230,229,228,227,226,225,224,223,222,221,221,221,221,221,221,220,
  219,218,217,217,217,217,217,217,216,215,214,213,212,211,210,210,210,210,
  209,209,209,209,208,207,206,205,204,203,202,201,200,199,198,197,195,194,
  193,192,191,190,189,188,187,186,185,
264,536,264,264,264,264,264,264,264,264,264,264,264,264,264,264,264,264,264,
  264,264,264,264,264,264,264,264,264,264,264,264,264,264,264,264,264,264,
  264,264,264,264,270,264,632,692,
377,633,693,
129,129,129,129,129,129,129,129,129,129,129,277,129,248,248,248,248,280,625,
  279,138,289,291,292,293,294,295,296,297,298,299,315,316,317,278,314,281,
  290,302,276,263,128,127,634,232,287,288,233,283,117,270,234,284,626,118,
  261,327,326,324,329,325,328,323,119,318,218,694,304,259,319,323,322,321,
  320,217,323,313,312,311,310,309,308,307,306,305,303,286,285,271,259,627,
  301,300,
688,92,92,92,92,92,635,695,
688,91,91,91,91,91,636,695,
688,90,90,90,90,90,637,695,
688,89,89,89,89,89,638,689,
688,86,86,86,86,86,639,689,
19,1,640,1,908,
380,641,58,
351,353,356,357,358,359,360,361,362,363,364,365,366,367,368,369,370,371,372,
  373,374,381,384,386,387,315,317,383,375,379,376,378,382,354,352,377,696,
  302,380,35,38,698,697,642,63,1,698,63,63,63,64,63,63,63,63,63,63,63,63,
  63,63,63,63,63,63,63,63,63,63,63,63,63,63,63,63,63,63,63,63,63,63,63,63,
  63,63,63,63,
699,699,699,699,699,699,699,699,699,643,
700,700,700,700,700,644,
146,146,146,146,146,146,146,146,146,146,146,146,146,146,146,146,700,700,700,
  700,700,146,146,146,146,146,146,146,146,146,146,146,146,146,146,146,146,
  146,146,146,146,146,146,645,
262,646,
701,701,701,701,701,701,701,701,701,701,701,701,701,701,701,1,647,1,701,
129,129,129,129,129,129,129,129,129,129,129,277,129,248,248,248,248,280,282,
  279,289,291,292,293,294,295,296,297,298,299,315,316,317,278,314,281,290,
  302,276,263,128,127,648,232,287,288,233,283,234,284,261,327,326,324,329,
  325,328,323,702,318,218,304,259,319,323,322,321,320,217,323,313,312,311,
  310,309,308,307,306,305,303,286,285,259,301,300,
129,129,129,129,129,129,129,129,129,129,129,277,129,248,248,248,248,280,282,
  279,289,291,292,293,294,295,296,297,298,299,315,316,317,278,314,281,290,
  302,276,263,128,127,649,232,287,288,233,283,234,284,261,327,326,324,329,
  325,328,323,703,318,218,304,259,319,323,322,321,320,217,323,313,312,311,
  310,309,308,307,306,305,303,286,285,259,301,300,
129,129,129,129,129,129,129,129,129,129,129,277,129,248,248,248,248,280,282,
  279,289,291,292,293,294,295,296,297,298,299,315,316,317,278,314,281,290,
  302,276,263,128,127,650,232,287,288,233,283,234,284,261,327,326,324,329,
  325,328,323,704,318,218,304,259,319,323,322,321,320,217,323,313,312,311,
  310,309,308,307,306,305,303,286,285,259,301,300,
129,129,129,129,129,129,129,129,129,129,129,277,129,248,248,248,248,280,282,
  279,289,291,292,293,294,295,296,297,298,299,315,316,317,278,314,281,290,
  302,276,263,128,127,651,232,287,288,233,283,234,284,261,327,326,324,329,
  325,328,323,705,318,218,304,259,319,323,322,321,320,217,323,313,312,311,
  310,309,308,307,306,305,303,286,285,259,301,300,
129,129,129,129,129,129,129,129,129,129,129,277,129,248,248,248,248,280,282,
  279,289,291,292,293,294,295,296,297,298,299,315,316,317,278,314,281,290,
  302,276,263,128,127,652,232,287,288,233,283,234,284,261,327,326,324,329,
  325,328,323,706,318,218,304,259,319,323,322,321,320,217,323,313,312,311,
  310,309,308,307,306,305,303,286,285,259,301,300,
129,129,129,129,129,129,129,129,129,129,129,277,129,248,248,248,248,280,282,
  279,289,291,292,293,294,295,296,297,298,299,315,316,317,278,314,281,290,
  302,276,263,128,127,653,232,287,288,233,283,234,284,261,327,326,324,329,
  325,328,323,707,318,218,304,259,319,323,322,321,320,217,323,313,312,311,
  310,309,308,307,306,305,303,286,285,259,301,300,
129,129,129,129,129,129,129,129,129,129,129,277,129,248,248,248,248,280,282,
  279,289,291,292,293,294,295,296,297,298,299,315,316,317,278,314,281,290,
  302,276,263,128,127,654,232,287,288,233,283,234,284,261,327,326,324,329,
  325,328,323,708,318,218,304,259,319,323,322,321,320,217,323,313,312,311,
  310,309,308,307,306,305,303,286,285,259,301,300,
129,129,129,129,129,129,129,129,129,129,129,277,129,248,248,248,248,280,282,
  279,289,291,292,293,294,295,296,297,298,299,315,316,317,278,314,281,290,
  302,276,263,128,127,655,232,287,288,233,283,234,284,261,327,326,324,329,
  325,328,323,709,318,218,304,259,319,323,322,321,320,217,323,313,312,311,
  310,309,308,307,306,305,303,286,285,259,301,300,
129,129,129,129,129,129,129,129,129,129,129,277,129,248,248,248,248,280,282,
  279,289,291,292,293,294,295,296,297,298,299,315,316,317,278,314,281,290,
  302,276,263,128,127,656,232,287,288,233,283,234,284,261,327,326,324,329,
  325,328,323,710,318,218,304,259,319,323,322,321,320,217,323,313,312,311,
  310,309,308,307,306,305,303,286,285,259,301,300,
129,129,129,129,129,129,129,129,129,129,129,277,129,248,248,248,248,280,282,
  279,289,291,292,293,294,295,296,297,298,299,315,316,317,278,314,281,290,
  302,276,263,128,127,657,232,287,288,233,283,234,284,261,327,326,324,329,
  325,328,323,711,318,218,304,259,319,323,322,321,320,217,323,313,312,311,
  310,309,308,307,306,305,303,286,285,259,301,300,
712,1,658,1,712,
129,129,129,129,129,129,129,129,129,129,129,277,129,248,248,248,248,280,282,
  279,289,291,292,293,294,295,296,297,298,299,278,552,281,290,302,276,263,
  128,127,659,232,287,288,233,283,234,284,261,318,218,304,259,211,217,211,
  313,312,311,310,309,308,307,306,305,303,286,285,259,301,300,
129,129,129,129,129,129,129,129,129,129,129,277,129,248,248,248,248,280,282,
  279,289,291,292,293,294,295,296,297,298,299,278,552,281,290,302,276,263,
  128,127,660,232,287,288,233,283,234,284,261,318,218,304,259,210,217,210,
  313,312,311,310,309,308,307,306,305,303,286,285,259,301,300,
129,129,129,129,129,129,129,129,129,129,129,277,129,248,248,248,248,280,282,
  279,289,291,292,293,294,295,296,297,298,299,278,552,281,290,302,276,263,
  128,127,661,232,287,288,233,283,234,284,261,318,218,304,259,209,217,209,
  313,312,311,310,309,308,307,306,305,303,286,285,259,301,300,
129,129,129,129,129,129,129,129,129,129,129,277,129,248,248,248,248,280,282,
  279,289,291,292,293,294,295,296,297,298,299,278,552,281,290,302,276,263,
  128,127,662,232,287,288,233,283,234,284,261,318,218,304,259,208,217,208,
  313,312,311,310,309,308,307,306,305,303,286,285,259,301,300,
129,129,129,129,129,129,129,129,129,129,129,277,129,248,248,248,248,280,282,
  279,289,291,292,293,294,295,296,297,298,299,278,552,281,290,302,276,263,
  128,127,663,232,287,288,233,283,234,284,261,318,218,304,259,207,217,207,
  313,312,311,310,309,308,307,306,305,303,286,285,259,301,300,
129,129,129,129,129,129,129,129,129,129,129,277,129,248,248,248,248,280,282,
  279,289,291,292,293,294,295,296,297,298,299,315,316,317,278,314,281,290,
  302,276,263,128,127,664,232,287,288,233,283,234,284,261,713,323,318,218,
  304,259,319,323,322,321,320,217,323,313,312,311,310,309,308,307,306,305,
  303,286,285,259,301,300,
129,129,129,129,129,129,129,129,129,129,129,277,129,248,248,248,248,280,282,
  279,289,291,292,293,294,295,296,297,298,299,315,316,317,278,314,281,290,
  302,276,263,128,127,665,232,287,288,233,283,234,284,261,714,323,318,218,
  304,259,319,323,322,321,320,217,323,313,312,311,310,309,308,307,306,305,
  303,286,285,259,301,300,
129,129,129,129,129,129,129,129,129,129,129,277,129,248,248,248,248,280,282,
  279,289,291,292,293,294,295,296,297,298,299,315,316,317,278,314,281,290,
  302,276,263,128,127,666,232,287,288,233,283,234,284,261,324,715,323,318,
  218,304,259,319,323,322,321,320,217,323,313,312,311,310,309,308,307,306,
  305,303,286,285,259,301,300,
129,129,129,129,129,129,129,129,129,129,129,277,129,248,248,248,248,280,282,
  279,289,291,292,293,294,295,296,297,298,299,315,316,317,278,314,281,290,
  302,276,263,128,127,667,232,287,288,233,283,234,284,261,324,716,323,318,
  218,304,259,319,323,322,321,320,217,323,313,312,311,310,309,308,307,306,
  305,303,286,285,259,301,300,
129,129,129,129,129,129,129,129,129,129,129,277,129,248,248,248,248,280,282,
  279,289,291,292,293,294,295,296,297,298,299,315,316,317,278,314,281,290,
  302,276,263,128,127,668,232,287,288,233,283,234,284,261,324,717,323,318,
  218,304,259,319,323,322,321,320,217,323,313,312,311,310,309,308,307,306,
  305,303,286,285,259,301,300,
129,129,129,129,129,129,129,129,129,129,129,277,129,248,248,248,248,280,282,
  279,289,291,292,293,294,295,296,297,298,299,315,316,317,278,314,281,290,
  302,276,263,128,127,669,232,287,288,233,283,234,284,261,324,718,323,318,
  218,304,259,319,323,322,321,320,217,323,313,312,311,310,309,308,307,306,
  305,303,286,285,259,301,300,
129,129,129,129,129,129,129,129,129,129,129,277,129,248,248,248,248,280,282,
  279,289,291,292,293,294,295,296,297,298,299,315,316,317,278,314,281,290,
  302,276,263,128,127,670,232,287,288,233,283,234,284,261,719,324,325,323,
  318,218,304,259,319,323,322,321,320,217,323,313,312,311,310,309,308,307,
  306,305,303,286,285,259,301,300,
129,129,129,129,129,129,129,129,129,129,129,277,129,248,248,248,248,280,282,
  279,289,291,292,293,294,295,296,297,298,299,315,316,317,278,314,281,290,
  302,276,263,128,127,671,232,287,288,233,283,234,284,261,720,324,325,323,
  318,218,304,259,319,323,322,321,320,217,323,313,312,311,310,309,308,307,
  306,305,303,286,285,259,301,300,
129,129,129,129,129,129,129,129,129,129,129,277,129,248,248,248,248,280,282,
  279,289,291,292,293,294,295,296,297,298,299,315,316,317,278,314,281,290,
  302,276,263,128,127,672,232,287,288,233,283,234,284,261,721,326,324,325,
  323,318,218,304,259,319,323,322,321,320,217,323,313,312,311,310,309,308,
  307,306,305,303,286,285,259,301,300,
129,129,129,129,129,129,129,129,129,129,129,277,129,248,248,248,248,280,282,
  279,289,291,292,293,294,295,296,297,298,299,315,316,317,278,314,281,290,
  302,276,263,128,127,673,232,287,288,233,283,234,284,261,722,326,324,325,
  323,318,218,304,259,319,323,322,321,320,217,323,313,312,311,310,309,308,
  307,306,305,303,286,285,259,301,300,
129,129,129,129,129,129,129,129,129,129,129,277,129,248,248,248,248,280,282,
  279,289,291,292,293,294,295,296,297,298,299,315,316,317,278,314,281,290,
  302,276,263,128,127,674,232,287,288,233,283,234,284,261,327,326,324,325,
  723,323,318,218,304,259,319,323,322,321,320,217,323,313,312,311,310,309,
  308,307,306,305,303,286,285,259,301,300,
129,129,129,129,129,129,129,129,129,129,129,277,129,248,248,248,248,280,282,
  279,289,291,292,293,294,295,296,297,298,299,315,316,317,278,314,281,290,
  302,276,263,128,127,675,232,287,288,233,283,234,284,261,327,326,324,325,
  724,323,318,218,304,259,319,323,322,321,320,217,323,313,312,311,310,309,
  308,307,306,305,303,286,285,259,301,300,
725,725,725,725,1,676,1,725,
37,38,39,34,677,34,34,42,34,41,40,40,
37,38,39,34,678,33,33,42,33,41,40,40,
37,38,39,34,679,32,32,42,32,41,40,40,
726,726,726,726,1,680,1,726,
727,727,727,727,1,681,1,727,
37,38,39,34,682,68,68,42,68,41,40,40,
129,129,129,129,129,129,129,129,129,129,129,277,129,248,248,248,248,280,282,
  279,289,291,292,293,294,295,296,297,298,299,315,316,317,278,314,281,290,
  302,276,263,128,127,683,232,287,288,233,283,234,284,261,327,326,324,329,
  325,328,323,396,318,218,304,259,319,323,322,321,320,217,323,313,312,311,
  310,309,308,307,306,305,303,286,285,259,301,300,
129,129,129,129,129,129,129,129,129,129,129,277,129,248,248,248,248,280,282,
  279,289,291,292,293,294,295,296,297,298,299,315,316,317,278,314,281,290,
  302,276,263,128,127,684,232,287,288,233,283,234,284,261,327,326,324,329,
  325,328,323,394,318,218,304,259,319,323,322,321,320,217,323,313,312,311,
  310,309,308,307,306,305,303,286,285,259,301,300,
129,129,129,129,129,129,129,129,129,129,129,277,129,248,248,248,248,280,282,
  279,289,291,292,293,294,295,296,297,298,299,315,316,317,278,314,281,290,
  302,276,263,128,127,685,232,287,288,233,283,234,284,261,327,326,324,329,
  325,328,323,392,318,218,304,259,319,323,322,321,320,217,323,313,312,311,
  310,309,308,307,306,305,303,286,285,259,301,300,
688,686,728,
264,536,264,264,264,264,264,264,264,264,264,264,264,264,264,264,264,264,264,
  264,264,261,264,264,264,264,264,264,264,264,264,264,264,264,264,264,264,
  264,264,264,264,264,270,264,687,729,
19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,
  19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,
  19,19,19,19,688,1,945,
730,730,730,730,730,730,730,730,730,730,730,730,730,730,730,730,730,730,730,
  730,730,730,730,730,730,730,730,730,730,730,730,730,730,730,730,730,730,
  730,730,730,730,730,730,1,689,1,730,
478,690,479,
19,320,320,320,320,1,691,1,309,
264,536,264,264,264,264,264,264,264,264,264,264,264,264,264,264,264,264,264,
  264,264,264,264,264,264,264,264,264,264,264,264,264,264,264,264,264,264,
  264,264,264,264,270,264,692,135,
731,731,731,731,731,731,731,731,731,731,731,731,731,731,731,731,731,731,731,
  731,731,731,731,731,731,731,731,731,731,731,731,731,731,731,731,731,731,
  731,731,731,731,731,1,693,1,731,
688,98,98,98,98,98,694,689,
732,732,732,732,732,732,732,732,732,732,732,732,732,732,732,1,695,1,732,
19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,
  19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,696,1,910,
63,63,63,63,63,63,63,63,63,63,63,63,63,63,63,63,63,63,63,63,63,63,63,63,63,
  63,63,63,63,63,63,63,63,63,63,63,63,63,63,63,17,17,17,17,17,697,
37,38,39,34,698,57,57,42,57,41,40,40,
154,154,154,154,154,154,154,154,153,153,153,153,154,154,154,154,153,153,153,
  153,153,154,154,154,154,154,154,154,154,154,154,154,154,154,154,154,154,
  154,154,154,154,154,154,699,
150,150,150,150,150,700,
129,129,129,129,129,129,129,129,129,129,129,129,129,128,127,701,735,733,733,
  734,
380,702,230,
380,703,229,
380,704,228,
380,705,227,
380,706,226,
380,707,225,
380,708,224,
380,709,223,
380,710,222,
380,711,221,
380,712,220,
205,553,555,383,375,379,205,205,205,205,205,205,205,205,205,205,205,205,205,
  205,205,205,205,205,205,205,205,205,205,205,205,205,205,205,205,205,713,
  559,558,557,556,554,
204,553,555,383,375,379,204,204,204,204,204,204,204,204,204,204,204,204,204,
  204,204,204,204,204,204,204,204,204,204,204,204,204,204,204,204,204,714,
  559,558,557,556,554,
202,376,378,202,202,202,202,202,202,202,202,202,202,202,202,202,202,202,202,
  202,202,202,202,202,202,202,202,202,202,202,202,202,202,715,561,560,
201,376,378,201,201,201,201,201,201,201,201,201,201,201,201,201,201,201,201,
  201,201,201,201,201,201,201,201,201,201,201,201,201,201,716,561,560,
200,376,378,200,200,200,200,200,200,200,200,200,200,200,200,200,200,200,200,
  200,200,200,200,200,200,200,200,200,200,200,200,200,200,717,561,560,
199,376,378,199,199,199,199,199,199,199,199,199,199,199,199,199,199,199,199,
  199,199,199,199,199,199,199,199,199,199,199,199,199,199,718,561,560,
197,562,564,566,568,197,197,197,197,197,197,197,197,197,197,197,197,197,197,
  197,197,197,197,197,197,197,197,197,197,197,197,719,569,567,565,563,
196,562,564,566,568,196,196,196,196,196,196,196,196,196,196,196,196,196,196,
  196,196,196,196,196,196,196,196,196,196,196,196,720,569,567,565,563,
194,570,382,194,194,194,194,194,194,194,194,194,194,194,194,194,194,194,194,
  194,194,194,194,194,194,194,194,721,572,571,
193,570,382,193,193,193,193,193,193,193,193,193,193,193,193,193,193,193,193,
  193,193,193,193,193,193,193,193,722,572,571,
191,573,354,191,191,191,191,191,191,191,191,191,191,191,191,191,191,191,191,
  191,191,191,191,191,191,723,575,574,
190,573,354,190,190,190,190,190,190,190,190,190,190,190,190,190,190,190,190,
  190,190,190,190,190,190,724,575,574,
37,38,39,34,725,44,44,42,44,41,40,40,
37,38,39,34,726,29,29,42,29,41,40,40,
37,38,39,34,727,28,28,42,28,41,40,40,
736,736,736,736,736,736,736,736,736,1,728,1,736,
135,135,135,135,135,135,135,135,135,135,135,135,135,135,135,135,135,135,135,
  135,135,262,135,135,135,135,135,135,135,135,135,135,135,135,135,135,135,
  135,135,135,135,135,135,135,729,
129,129,129,129,129,129,129,129,129,129,129,277,129,248,248,248,248,280,625,
  279,138,289,291,292,293,294,295,296,297,298,299,315,316,317,278,314,281,
  290,302,276,263,128,127,730,232,287,288,233,283,121,270,234,284,626,122,
  261,327,326,324,329,325,328,323,120,318,218,304,259,319,323,322,321,320,
  217,323,313,312,311,310,309,308,307,306,305,303,286,285,271,259,627,301,
  300,
129,129,129,129,129,129,129,129,129,129,129,277,129,248,248,248,248,280,282,
  279,289,291,292,293,294,295,296,297,298,299,315,316,317,278,314,281,290,
  302,276,263,128,127,731,232,287,288,233,283,234,284,261,327,326,324,329,
  325,328,323,99,318,218,304,259,319,323,322,321,320,217,323,313,312,311,
  310,309,308,307,306,305,303,286,285,259,301,300,
129,129,129,129,129,129,129,129,129,129,129,129,129,128,127,732,124,259,259,
130,130,130,130,130,130,130,130,130,130,130,130,131,131,131,131,131,130,19,
  130,1,733,1,125,131,
19,19,734,1,989,
737,1,735,1,737,
285,284,284,281,280,279,278,277,276,736,381,284,617,
380,737,231,

};


static const unsigned short ag_sbt[] = {
     0,  30,  32,  72,  87, 116, 140, 338, 341, 358, 556, 922, 961, 980,
   999,1004,1012,1020,1039,1085,1131,1150,1159,1184,1204,1223,1228,1236,
  1244,1263,1350,1437,1456,1467,1476,1535,1626,1632,1681,1732,1779,1873,
  1962,2051,2423,2433,2442,2451,2460,2469,2478,2487,2496,2505,2514,2523,
  2532,2541,2550,2559,2572,2585,2596,2642,2688,2734,2780,2826,2872,2918,
  2964,3010,3056,3102,3148,3194,3240,3286,3332,3341,3350,3359,3368,3377,
  3386,3395,3404,3413,3422,3431,3440,3449,3458,3467,3476,3485,3494,3503,
  3512,3521,3530,3539,3548,3557,3566,3575,3584,3593,3601,3614,3627,3638,
  3684,3730,3776,3822,3868,3914,3960,4006,4052,4098,4144,4190,4236,4282,
  4328,4374,4420,4466,4512,4558,4604,4650,4696,4742,4788,4834,4880,4926,
  4972,5018,5064,5110,5156,5202,5248,5294,5340,5386,5432,5478,5524,5570,
  5616,5662,5708,5754,5800,5846,5858,5870,5882,5894,5902,5910,5921,5932,
  5943,5956,5969,5982,5995,6008,6021,6034,6047,6060,6073,6086,6099,6112,
  6120,6125,6130,6135,6181,6227,6273,6319,6365,6411,6457,6503,6549,6595,
  6641,6687,6733,6779,6825,6871,6917,6963,7009,7055,7101,7147,7193,7239,
  7285,7331,7377,7423,7469,7515,7561,7607,7653,7699,7745,7791,7837,7883,
  7929,7975,8021,8067,8113,8159,8205,8251,8297,8343,8389,8435,8481,8527,
  8539,8551,8563,8575,8583,8591,8602,8613,8624,8637,8650,8663,8676,8689,
  8702,8715,8728,8741,8754,8767,8780,8793,8853,8862,8903,8910,8913,9000,
  9077,9085,9128,9172,9180,9225,9236,9244,9256,9268,9276,9282,9287,9290,
  9349,9404,9410,9455,9461,9472,9515,9556,9604,9657,9662,9671,9676,9681,
  9686,9691,9696,9701,9706,9711,9716,9757,9798,9865,9870,9875,9880,9885,
  9890,9895,9900,9905,9910,9915,9920,9963,10028,10071,10136,10182,10252,
  10322,10392,10462,10503,10547,10585,10623,10655,10685,10690,10728,10813,
  10821,10829,10836,10844,10852,10860,10868,10887,10906,10914,10922,10930,
  10938,10981,10989,10997,11005,11030,11060,11111,11177,11228,11294,11343,
  11394,11445,11496,11547,11598,11649,11700,11751,11802,11853,11904,11955,
  12006,12057,12108,12159,12210,12261,12312,12377,12443,12509,12575,12640,
  12715,12766,12832,12897,12948,12997,13048,13099,13158,13244,13247,13250,
  13331,13339,13356,13373,13386,13399,13405,13408,13411,13414,13499,13584,
  13669,13754,13839,13924,14009,14094,14179,14264,14349,14434,14519,14604,
  14689,14774,14859,14944,15029,15114,15199,15284,15369,15454,15539,15624,
  15709,15794,15879,15964,16049,16134,16219,16304,16389,16474,16559,16644,
  16729,16814,16899,16984,17069,17154,17239,17324,17409,17494,17579,17664,
  17749,17834,17849,17864,17879,17894,17902,17910,17923,17936,17949,17966,
  17983,17996,18009,18022,18035,18048,18061,18074,18087,18100,18113,18126,
  18219,18241,18378,18515,18534,18553,18576,18598,18606,18614,18620,18629,
  18637,18645,18668,18692,18697,18702,18749,18796,18843,18932,18978,19063,
  19082,19101,19120,19139,19158,19178,19197,19217,19236,19256,19303,19350,
  19443,19490,19537,19584,19677,19723,19769,19854,19862,19908,19954,20000,
  20085,20104,20114,20124,20136,20139,20221,20233,20248,20260,20272,20282,
  20289,20335,20384,20433,20436,20439,20442,20445,20448,20451,20454,20457,
  20460,20463,20466,20551,20557,20600,20643,20686,20729,20772,20815,20858,
  20904,20950,20996,21042,21088,21134,21180,21226,21272,21318,21364,21410,
  21456,21502,21548,21594,21640,21686,21732,21778,21824,21870,21916,21962,
  22008,22054,22100,22146,22192,22238,22284,22330,22342,22354,22366,22378,
  22390,22409,22421,22433,22478,22490,22502,22527,22535,22543,22551,22597,
  22682,22728,22813,22825,22833,22841,22851,22861,22871,22882,22892,22938,
  22984,23030,23039,23048,23053,23098,23144,23154,23159,23169,23190,23481,
  23526,23529,23622,23630,23638,23646,23654,23662,23667,23670,23757,23767,
  23773,23817,23819,23838,23923,24008,24093,24178,24263,24348,24433,24518,
  24603,24688,24693,24763,24833,24903,24973,25043,25122,25201,25281,25361,
  25441,25521,25602,25683,25765,25847,25930,26013,26021,26033,26045,26057,
  26065,26073,26085,26170,26255,26340,26343,26389,26445,26492,26495,26504,
  26549,26595,26603,26622,26670,26716,26728,26772,26778,26798,26801,26804,
  26807,26810,26813,26816,26819,26822,26825,26828,26831,26873,26915,26951,
  26987,27023,27059,27095,27131,27161,27191,27219,27247,27259,27271,27283,
  27296,27341,27433,27518,27537,27562,27567,27572,27585,27588
};


static const unsigned short ag_sbe[] = {
    27,  31,  58,  84, 114, 127, 335, 339, 348, 553, 751, 949, 977, 996,
  1001,1009,1017,1036,1082,1128,1147,1156,1181,1199,1219,1224,1233,1241,
  1259,1305,1392,1452,1461,1469,1532,1623,1628,1678,1729,1776,1825,1917,
  2006,2245,2430,2439,2448,2457,2466,2475,2484,2493,2502,2511,2520,2529,
  2538,2547,2556,2569,2582,2593,2639,2685,2731,2777,2823,2869,2915,2961,
  3007,3053,3099,3145,3191,3237,3283,3329,3338,3347,3356,3365,3374,3383,
  3392,3401,3410,3419,3428,3437,3446,3455,3464,3473,3482,3491,3500,3509,
  3518,3527,3536,3545,3554,3563,3572,3581,3590,3598,3611,3624,3635,3681,
  3727,3773,3819,3865,3911,3957,4003,4049,4095,4141,4187,4233,4279,4325,
  4371,4417,4463,4509,4555,4601,4647,4693,4739,4785,4831,4877,4923,4969,
  5015,5061,5107,5153,5199,5245,5291,5337,5383,5429,5475,5521,5567,5613,
  5659,5705,5751,5797,5843,5855,5867,5879,5891,5899,5907,5918,5929,5940,
  5953,5966,5979,5992,6005,6018,6031,6044,6057,6070,6083,6096,6109,6117,
  6122,6127,6132,6178,6224,6270,6316,6362,6408,6454,6500,6546,6592,6638,
  6684,6730,6776,6822,6868,6914,6960,7006,7052,7098,7144,7190,7236,7282,
  7328,7374,7420,7466,7512,7558,7604,7650,7696,7742,7788,7834,7880,7926,
  7972,8018,8064,8110,8156,8202,8248,8294,8340,8386,8432,8478,8524,8536,
  8548,8560,8572,8580,8588,8599,8610,8621,8634,8647,8660,8673,8686,8699,
  8712,8725,8738,8751,8764,8777,8790,8849,8859,8901,8908,8911,8955,9036,
  9082,9126,9169,9177,9223,9233,9241,9248,9260,9273,9281,9286,9289,9348,
  9403,9409,9453,9460,9471,9514,9555,9603,9655,9659,9668,9673,9678,9683,
  9688,9693,9698,9703,9708,9713,9754,9795,9862,9867,9872,9877,9882,9887,
  9892,9897,9902,9907,9912,9917,9960,10025,10068,10133,10179,10221,10291,
  10361,10431,10500,10539,10580,10616,10650,10680,10687,10708,10770,10818,
  10826,10834,10841,10849,10857,10865,10884,10903,10911,10919,10927,10935,
  10980,10986,10994,11002,11027,11051,11108,11174,11225,11291,11340,11391,
  11442,11493,11544,11595,11646,11697,11748,11799,11850,11901,11952,12003,
  12054,12105,12156,12207,12258,12309,12374,12440,12506,12572,12637,12712,
  12763,12829,12894,12945,12994,13045,13096,13155,13201,13245,13248,13288,
  13336,13345,13365,13382,13393,13403,13406,13409,13412,13456,13541,13626,
  13711,13796,13881,13966,14051,14136,14221,14306,14391,14476,14561,14646,
  14731,14816,14901,14986,15071,15156,15241,15326,15411,15496,15581,15666,
  15751,15836,15921,16006,16091,16176,16261,16346,16431,16516,16601,16686,
  16771,16856,16941,17026,17111,17196,17281,17366,17451,17536,17621,17706,
  17791,17842,17857,17872,17887,17898,17906,17917,17930,17943,17958,17975,
  17992,18005,18018,18031,18044,18057,18070,18083,18096,18109,18122,18169,
  18232,18375,18512,18531,18549,18573,18595,18603,18611,18617,18622,18634,
  18642,18665,18688,18694,18698,18746,18793,18840,18886,18975,19020,19079,
  19097,19117,19135,19155,19173,19194,19212,19233,19251,19300,19347,19393,
  19487,19534,19581,19627,19720,19766,19811,19859,19905,19951,19997,20042,
  20100,20111,20121,20128,20137,20179,20225,20246,20252,20264,20281,20288,
  20333,20383,20432,20434,20437,20440,20443,20446,20449,20452,20455,20458,
  20461,20464,20508,20556,20597,20640,20683,20726,20769,20812,20855,20901,
  20947,20993,21039,21085,21131,21177,21223,21269,21315,21361,21407,21453,
  21499,21545,21591,21637,21683,21729,21775,21821,21867,21913,21959,22005,
  22051,22097,22143,22189,22235,22281,22327,22334,22346,22358,22370,22382,
  22405,22413,22425,22477,22482,22494,22524,22532,22540,22548,22594,22639,
  22725,22770,22817,22830,22838,22848,22858,22868,22879,22889,22935,22981,
  23027,23036,23045,23050,23096,23142,23151,23156,23168,23188,23323,23524,
  23527,23572,23628,23636,23644,23652,23660,23664,23668,23713,23766,23772,
  23816,23818,23835,23880,23965,24050,24135,24220,24305,24390,24475,24560,
  24645,24690,24732,24802,24872,24942,25012,25085,25164,25243,25323,25403,
  25483,25563,25644,25725,25807,25889,25972,26018,26025,26037,26049,26062,
  26070,26077,26127,26212,26297,26341,26387,26442,26489,26493,26501,26547,
  26592,26601,26619,26667,26715,26720,26771,26777,26793,26799,26802,26805,
  26808,26811,26814,26817,26820,26823,26826,26829,26867,26909,26948,26984,
  27020,27056,27090,27126,27158,27188,27216,27244,27251,27263,27275,27293,
  27340,27384,27475,27533,27558,27564,27569,27581,27586,27588
};


static const unsigned char ag_fl[] = {
  2,1,1,2,2,1,1,2,0,1,3,3,1,2,1,2,2,1,2,0,1,4,5,3,2,1,0,1,7,7,1,1,6,6,6,
  2,3,2,1,2,4,4,4,4,6,4,4,0,1,4,4,4,4,3,3,4,4,6,4,1,4,1,1,2,2,2,2,3,5,1,
  2,2,2,3,2,1,1,1,3,2,1,1,3,1,1,1,3,1,1,3,3,3,3,3,3,3,3,3,4,6,2,3,2,1,1,
  2,2,3,3,2,2,3,3,1,1,2,3,1,1,1,4,4,4,1,4,2,1,1,1,1,2,2,1,2,2,3,2,2,1,2,
  3,1,2,2,2,2,2,2,2,2,4,1,1,4,3,2,1,2,3,3,2,1,1,1,1,1,1,2,1,1,1,1,2,1,1,
  2,1,1,2,1,1,2,1,1,2,1,1,2,1,2,4,4,2,4,4,2,4,4,2,4,4,4,4,2,4,4,2,4,4,4,
  4,4,1,2,2,2,2,1,1,1,5,5,5,5,5,5,5,5,5,5,5,7,1,1,1,1,1,1,1,2,2,2,1,1,2,
  2,2,2,1,2,2,2,3,2,2,2,2,2,2,2,2,3,4,1,1,2,2,2,2,2,1,2,1,2,2,2,1,1,1,1,
  1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,2,1,1,1,1,1,
  1,1,1,1,1,4,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,
  1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,3,3,3,3,3,3,3,3,3,3,3,3,7,3,3,3,
  3,3,3,3,3,3,3,5,3,5,3,5,3,3,3,3,3,3,3,3,3,3,3,3,3,3,3,3,1,1,1,1,1,3,3,
  3,3,1,1,1,1,1,3,3,3,3,3,3,3,1,1,1,3,1,1,1,3,3,3,3,3,3,3,3,3,3,3,3,3,3,
  3,3,3,3,3,3,3,3,3,3,3,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,
  1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,
  1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,
  1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,
  1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,
  1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,
  1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,
  1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,
  1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,
  1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,
  1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,
  1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,
  1,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,
  2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,
  2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,
  2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,
  2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,
  2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,
  2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,
  2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,
  2,2,2,2,2
};

static const unsigned short ag_ptt[] = {
    0, 36, 37, 37, 34, 39, 43, 43, 44, 44, 39, 39, 39, 40, 47, 47, 47, 51,
   51, 52, 52, 35, 35, 35, 35, 35, 55, 55, 35, 35, 61, 61, 35, 35, 35, 35,
   35, 35, 64, 53, 66, 66, 66, 66, 66, 66, 66, 74, 74, 66, 66, 66, 66, 66,
   66, 66, 66, 66, 84, 84, 89, 89, 85, 85, 85, 94, 94, 62, 62, 33, 33, 33,
   33, 33, 33,100,100,100, 33, 33,104,104, 33,108,108,108, 33,111,111, 33,
   33, 33, 33, 33, 33, 33, 33, 33, 33, 33, 33, 33, 33, 33, 33, 33, 33, 33,
   33, 33, 33, 33, 33, 33, 33, 30, 30, 90, 90, 90, 90, 90, 90,113,113,538,
  443,443,443,133,133,133,438,438,486, 18, 18,436, 12, 12, 12,142,142,142,
  142,142,142,142,142,142,142,155,155,142,142,437, 14, 14, 14, 14,430,430,
  430,430,430, 22, 22, 76, 76,170,170,170,166,173,173,166,176,176,166,179,
  179,166,182,182,166,185,185,166, 57, 26, 26, 26, 28, 28, 28, 23, 23, 23,
   24, 24, 24, 24, 24, 27, 27, 27, 25, 25, 25, 25, 25, 25, 29, 29, 29, 29,
   29,199,199,199,199,208,208,208,208,208,208,208,208,208,208,208,207,207,
  493,493,493,493,493,493,493,493,224,224,493,493,  7,  7,  7,  7,  7,  7,
    6,  6,  6, 17, 17,219,219,220,220, 15, 15, 15, 16, 16, 16, 16, 16, 16,
   16, 16,540,  9,  9,  9,596,596,596,596,596,596,243,243,596,596,246,246,
  610,248,248,610,250,250,610,610,252,252,252,252,252,254,254,255,255,255,
  255,255,255, 31, 32, 32, 32, 32, 32, 32, 32, 32, 32, 32, 32,273,273,273,
  273,267,267,267,267,267,280,280,267,267,267,286,286,286,267,267,267,267,
  267,267,267,267,267,297,297,267,267,267,267,267,267,267,267,267,267,310,
  310,310,267,267,267,267,267,267,267,260,260,261,261,261,261,258,258,258,
  258,258,258,258,258,258,258,258,258,258,259,259,259,339,263,341,264,343,
  265,266,262,262,262,262,262,262,262,262,262,262,262,262,262,262,262,367,
  367,367,367,367,262,262,262,262,376,376,376,376,376,262,262,262,262,262,
  262,262,386,386,386,262,390,390,390,262,262,262,262,262,262,262,262,262,
  262,262,262,262,262,262,262,262,262,262,262,262,262,262,262,262,138,346,
  153,227, 42, 42, 42, 42, 42, 42, 42, 42, 42, 42, 42, 42, 42, 42, 42, 42,
   42, 42, 42, 42, 42, 42, 42, 42, 42, 42, 42, 42, 42, 42, 42, 42, 42, 42,
   42, 42, 42, 42, 42, 42, 42, 42, 42, 50, 50, 50, 50, 50, 50, 50, 50, 50,
   50, 50, 50, 50, 50, 50, 50, 50, 50, 50, 50, 50, 50, 50, 50, 50, 50, 50,
   50, 50, 50, 50, 50, 50, 50, 50, 50, 50, 50, 50, 50, 50, 50, 50, 50, 91,
   91, 91, 91, 91, 91, 91, 91, 91, 91, 91, 91, 91, 91, 91, 91, 91, 91, 91,
   91, 91, 91, 91, 91, 91, 91, 91, 91, 91, 91, 91, 91, 91, 91, 91, 91, 91,
   91, 91, 91, 92, 92, 92, 92, 92, 92, 92, 92, 92, 92, 92, 92, 92, 92, 92,
   92, 92, 92, 92, 92, 92, 92, 92, 92, 92, 92, 92, 92, 92, 92, 92, 92, 92,
   92, 92, 92, 92, 92, 92, 92, 92,136,136,136,136,136,136,136,136,136,136,
  136,136,136,137,137,137,137,137,137,137,137,137,137,137,137,137,137,139,
  139,139,139,139,139,139,139,139,139,139,139,139,139,139,139,139,139,139,
  144,144,144,144,144,144,144,144,144,144,144,144,144,144,144,144,144,144,
  144,144,144,144,144,144,144,144,144,144,144,144,144,144,144,144,144,144,
  144,144,144,144,144,151,151,151,152,152,152,152,152,156,156,156,156,156,
  156,156,156,156,159,159,159,159,159,159,159,159,159,159,159,159,159,159,
  159,159,159,159,159,159,159,159,159,159,159,159,159,159,159,159,159,159,
  159,159,159,159,159,159,159,159,160,160,160,160,160,160,160,160,160,160,
  160,160,160,160,160,160,160,160,160,160,160,160,160,160,160,160,160,160,
  160,160,160,160,160,160,160,160,160,160,160,160,160,160,160,228,228,228,
  228,229,229,231,231,231,231,233,233,233,233,233,233,233,233,233,233,233,
  233,233,233,233,233,233,233,233,233,233,233,233,233,233,233,233,233,233,
  233,233,233,233,233,233,233,233,233,233,233,233, 45, 41, 46, 48, 49, 11,
   54, 56, 58, 59, 60, 10, 13, 19, 65, 68, 67, 69, 21, 70, 71, 72, 73, 75,
   77, 78, 79, 80, 81, 82, 83, 88, 87, 86, 93, 95, 96, 97, 98, 99,101,102,
  103,105,106,107,109,110,112,114,115,116,117,118,119,120,122,121,123,124,
  125, 20,126,127,128,129,130,131,  2,132,165,167,168,169,171,172,174,175,
  177,178,180,181,183,184,186,187,188,189,190,191,192,193,194,195,196,197,
  198,200,201,202,203,204,205,206,209,210,211,212,213,214,215,216,217,  3,
  218,  8,253,244,256,257,251,268,269,270,271,272,274,275,276,277,278,279,
  281,282,283,284,285,287,288,289,290,291,292,293,294,295,296,298,299,300,
  301,302,303,304,305,306,307,308,309,311,312,313,314,315,316,317,318,319,
  320,321,322,  4,323,324,325,326,327,328,329,330,331,332,333,334,335,  5,
  336,337,338,340,342,344,345,347,348,349,350,351,352,353,354,355,356,357,
  358,359,360,361,362,363,364,365,366,368,369,370,371,372,373,374,375,377,
  378,379,380,381,382,383,384,385,387,388,389,391,392,393,394,395,396,397,
  398,399,400,401,402,245,403,404,405,406,407,408,409,410,411,412,413,415,
  141,416,154,140,148,417,418,419,420,158,157,421,230,422,149,225,150,423,
  221,145,147,146,414,143,226,424
};


static void ag_ra(void)
{
  switch(ag_rpx[(PCB).ag_ap]) {
    case 1: ag_rp_1(); break;
    case 2: ag_rp_2(); break;
    case 3: ag_rp_3(); break;
    case 4: ag_rp_4(); break;
    case 5: ag_rp_5(); break;
    case 6: ag_rp_6(); break;
    case 7: ag_rp_7(); break;
    case 8: ag_rp_8(); break;
    case 9: ag_rp_9(); break;
    case 10: ag_rp_10(); break;
    case 11: ag_rp_11(); break;
    case 12: ag_rp_12(); break;
    case 13: ag_rp_13(); break;
    case 14: ag_rp_14(); break;
    case 15: ag_rp_15(); break;
    case 16: ag_rp_16(); break;
    case 17: ag_rp_17(); break;
    case 18: ag_rp_18(); break;
    case 19: ag_rp_19(); break;
    case 20: ag_rp_20(); break;
    case 21: ag_rp_21(); break;
    case 22: ag_rp_22(); break;
    case 23: ag_rp_23(); break;
    case 24: ag_rp_24(); break;
    case 25: ag_rp_25(); break;
    case 26: ag_rp_26(); break;
    case 27: ag_rp_27(); break;
    case 28: ag_rp_28(); break;
    case 29: ag_rp_29(V(0,int)); break;
    case 30: ag_rp_30(V(1,int)); break;
    case 31: ag_rp_31(); break;
    case 32: ag_rp_32(); break;
    case 33: ag_rp_33(); break;
    case 34: ag_rp_34(V(2,int)); break;
    case 35: ag_rp_35(); break;
    case 36: ag_rp_36(); break;
    case 37: ag_rp_37(); break;
    case 38: ag_rp_38(); break;
    case 39: ag_rp_39(); break;
    case 40: ag_rp_40(); break;
    case 41: ag_rp_41(); break;
    case 42: ag_rp_42(); break;
    case 43: ag_rp_43(); break;
    case 44: ag_rp_44(); break;
    case 45: ag_rp_45(); break;
    case 46: ag_rp_46(); break;
    case 47: ag_rp_47(); break;
    case 48: ag_rp_48(); break;
    case 49: ag_rp_49(); break;
    case 50: ag_rp_50(); break;
    case 51: ag_rp_51(); break;
    case 52: ag_rp_52(); break;
    case 53: ag_rp_53(); break;
    case 54: ag_rp_54(); break;
    case 55: ag_rp_55(); break;
    case 56: ag_rp_56(); break;
    case 57: ag_rp_57(); break;
    case 58: ag_rp_58(); break;
    case 59: ag_rp_59(); break;
    case 60: ag_rp_60(); break;
    case 61: ag_rp_61(); break;
    case 62: ag_rp_62(); break;
    case 63: ag_rp_63(); break;
    case 64: ag_rp_64(); break;
    case 65: ag_rp_65(); break;
    case 66: ag_rp_66(); break;
    case 67: ag_rp_67(); break;
    case 68: ag_rp_68(); break;
    case 69: ag_rp_69(); break;
    case 70: ag_rp_70(V(2,int)); break;
    case 71: ag_rp_71(); break;
    case 72: ag_rp_72(); break;
    case 73: ag_rp_73(); break;
    case 74: ag_rp_74(); break;
    case 75: ag_rp_75(); break;
    case 76: ag_rp_76(); break;
    case 77: ag_rp_77(); break;
    case 78: ag_rp_78(); break;
    case 79: ag_rp_79(); break;
    case 80: ag_rp_80(); break;
    case 81: ag_rp_81(V(0,int)); break;
    case 82: ag_rp_82(V(1,int)); break;
    case 83: ag_rp_83(V(1,int)); break;
    case 84: ag_rp_84(V(0,int)); break;
    case 85: ag_rp_85(V(1,int)); break;
    case 86: ag_rp_86(V(1,int), V(2,int)); break;
    case 87: ag_rp_87(V(1,int)); break;
    case 88: ag_rp_88(); break;
    case 89: ag_rp_89(V(1,int)); break;
    case 90: V(0,int) = ag_rp_90(V(0,int)); break;
    case 91: V(0,int) = ag_rp_91(); break;
    case 92: V(0,int) = ag_rp_92(); break;
    case 93: V(0,int) = ag_rp_93(); break;
    case 94: V(0,int) = ag_rp_94(); break;
    case 95: V(0,int) = ag_rp_95(); break;
    case 96: V(0,int) = ag_rp_96(); break;
    case 97: V(0,int) = ag_rp_97(); break;
    case 98: V(0,int) = ag_rp_98(); break;
    case 99: V(0,int) = ag_rp_99(V(1,int), V(2,int), V(3,int)); break;
    case 100: V(0,int) = ag_rp_100(V(2,int), V(3,int)); break;
    case 101: V(0,int) = ag_rp_101(V(2,int)); break;
    case 102: ag_rp_102(); break;
    case 103: ag_rp_103(); break;
    case 104: ag_rp_104(V(1,int)); break;
    case 105: ag_rp_105(V(2,int)); break;
    case 106: ag_rp_106(); break;
    case 107: ag_rp_107(); break;
    case 108: ag_rp_108(); break;
    case 109: ag_rp_109(); break;
    case 110: ag_rp_110(); break;
    case 111: V(0,int) = ag_rp_111(); break;
    case 112: V(0,int) = ag_rp_112(); break;
    case 113: ag_rp_113(); break;
    case 114: ag_rp_114(); break;
    case 115: ag_rp_115(); break;
    case 116: ag_rp_116(); break;
    case 117: ag_rp_117(); break;
    case 118: ag_rp_118(); break;
    case 119: ag_rp_119(); break;
    case 120: ag_rp_120(); break;
    case 121: ag_rp_121(); break;
    case 122: ag_rp_122(); break;
    case 123: ag_rp_123(); break;
    case 124: ag_rp_124(); break;
    case 125: ag_rp_125(); break;
    case 126: ag_rp_126(); break;
    case 127: ag_rp_127(); break;
    case 128: ag_rp_128(); break;
    case 129: ag_rp_129(); break;
    case 130: ag_rp_130(); break;
    case 131: ag_rp_131(); break;
    case 132: ag_rp_132(); break;
    case 133: ag_rp_133(); break;
    case 134: ag_rp_134(); break;
    case 135: ag_rp_135(); break;
    case 136: ag_rp_136(); break;
    case 137: ag_rp_137(); break;
    case 138: ag_rp_138(); break;
    case 139: ag_rp_139(); break;
    case 140: ag_rp_140(); break;
    case 141: ag_rp_141(); break;
    case 142: ag_rp_142(V(0,double)); break;
    case 143: ag_rp_143(); break;
    case 144: ag_rp_144(); break;
    case 145: ag_rp_145(); break;
    case 146: ag_rp_146(); break;
    case 147: ag_rp_147(); break;
    case 148: ag_rp_148(); break;
    case 149: ag_rp_149(); break;
    case 150: ag_rp_150(); break;
    case 151: ag_rp_151(); break;
    case 152: ag_rp_152(); break;
    case 153: ag_rp_153(); break;
    case 154: ag_rp_154(); break;
    case 155: V(0,double) = ag_rp_155(V(0,int)); break;
    case 156: V(0,double) = ag_rp_156(V(0,double)); break;
    case 157: V(0,int) = ag_rp_157(V(0,int)); break;
    case 158: V(0,int) = ag_rp_158(); break;
    case 159: V(0,int) = ag_rp_159(); break;
    case 160: V(0,int) = ag_rp_160(); break;
    case 161: V(0,int) = ag_rp_161(); break;
    case 162: V(0,int) = ag_rp_162(); break;
    case 163: V(0,int) = ag_rp_163(); break;
    case 164: V(0,int) = ag_rp_164(); break;
    case 165: V(0,int) = ag_rp_165(); break;
    case 166: V(0,int) = ag_rp_166(); break;
    case 167: ag_rp_167(V(1,int)); break;
    case 168: ag_rp_168(V(1,int)); break;
    case 169: ag_rp_169(V(0,int)); break;
    case 170: ag_rp_170(V(1,int)); break;
    case 171: ag_rp_171(V(2,int)); break;
    case 172: ag_rp_172(V(1,int)); break;
    case 173: ag_rp_173(V(1,int)); break;
    case 174: ag_rp_174(V(1,int)); break;
    case 175: ag_rp_175(V(1,int)); break;
    case 176: ag_rp_176(V(1,int)); break;
    case 177: ag_rp_177(V(1,int)); break;
    case 178: ag_rp_178(V(1,int)); break;
    case 179: ag_rp_179(V(1,int)); break;
    case 180: V(0,int) = ag_rp_180(V(1,int)); break;
    case 181: V(0,int) = ag_rp_181(V(1,int), V(2,int)); break;
    case 182: V(0,int) = ag_rp_182(); break;
    case 183: V(0,int) = ag_rp_183(V(0,int)); break;
    case 184: V(0,int) = ag_rp_184(); break;
    case 185: V(0,int) = ag_rp_185(); break;
    case 186: V(0,int) = ag_rp_186(); break;
    case 187: V(0,int) = ag_rp_187(); break;
    case 188: V(0,int) = ag_rp_188(); break;
    case 189: V(0,int) = ag_rp_189(); break;
    case 190: V(0,int) = ag_rp_190(); break;
    case 191: V(0,double) = ag_rp_191(); break;
    case 192: V(0,double) = ag_rp_192(V(1,int)); break;
    case 193: V(0,double) = ag_rp_193(V(0,double), V(1,int)); break;
    case 194: ag_rp_194(); break;
    case 195: ag_rp_195(); break;
    case 196: ag_rp_196(); break;
    case 197: ag_rp_197(); break;
    case 198: ag_rp_198(); break;
    case 199: ag_rp_199(); break;
    case 200: ag_rp_200(); break;
    case 201: ag_rp_201(); break;
    case 202: ag_rp_202(); break;
    case 203: ag_rp_203(); break;
    case 204: ag_rp_204(); break;
    case 205: ag_rp_205(); break;
    case 206: ag_rp_206(); break;
    case 207: ag_rp_207(); break;
    case 208: ag_rp_208(); break;
    case 209: ag_rp_209(); break;
    case 210: ag_rp_210(); break;
    case 211: ag_rp_211(); break;
    case 212: ag_rp_212(); break;
    case 213: ag_rp_213(); break;
    case 214: ag_rp_214(); break;
    case 215: ag_rp_215(); break;
    case 216: ag_rp_216(); break;
    case 217: ag_rp_217(); break;
    case 218: ag_rp_218(); break;
    case 219: ag_rp_219(); break;
    case 220: ag_rp_220(); break;
    case 221: ag_rp_221(); break;
    case 222: ag_rp_222(); break;
    case 223: ag_rp_223(); break;
    case 224: ag_rp_224(); break;
    case 225: ag_rp_225(); break;
    case 226: ag_rp_226(); break;
    case 227: ag_rp_227(); break;
    case 228: ag_rp_228(); break;
    case 229: ag_rp_229(); break;
    case 230: ag_rp_230(); break;
    case 231: ag_rp_231(); break;
    case 232: ag_rp_232(); break;
    case 233: ag_rp_233(); break;
    case 234: ag_rp_234(); break;
    case 235: ag_rp_235(); break;
    case 236: ag_rp_236(); break;
    case 237: ag_rp_237(); break;
    case 238: ag_rp_238(); break;
    case 239: ag_rp_239(); break;
    case 240: ag_rp_240(); break;
    case 241: ag_rp_241(); break;
    case 242: ag_rp_242(); break;
    case 243: ag_rp_243(); break;
    case 244: ag_rp_244(); break;
    case 245: ag_rp_245(); break;
    case 246: ag_rp_246(); break;
    case 247: ag_rp_247(); break;
    case 248: ag_rp_248(); break;
    case 249: ag_rp_249(); break;
    case 250: ag_rp_250(); break;
    case 251: ag_rp_251(); break;
    case 252: ag_rp_252(); break;
    case 253: ag_rp_253(); break;
    case 254: ag_rp_254(); break;
    case 255: ag_rp_255(); break;
    case 256: ag_rp_256(); break;
    case 257: ag_rp_257(); break;
    case 258: ag_rp_258(); break;
    case 259: ag_rp_259(); break;
    case 260: ag_rp_260(); break;
    case 261: ag_rp_261(); break;
    case 262: ag_rp_262(); break;
    case 263: ag_rp_263(); break;
    case 264: ag_rp_264(); break;
    case 265: ag_rp_265(); break;
    case 266: ag_rp_266(); break;
    case 267: ag_rp_267(); break;
    case 268: ag_rp_268(); break;
    case 269: ag_rp_269(); break;
    case 270: ag_rp_270(); break;
    case 271: ag_rp_271(); break;
    case 272: ag_rp_272(); break;
    case 273: ag_rp_273(); break;
    case 274: ag_rp_274(); break;
    case 275: ag_rp_275(); break;
    case 276: ag_rp_276(); break;
    case 277: ag_rp_277(); break;
    case 278: ag_rp_278(V(2,int)); break;
    case 279: ag_rp_279(); break;
    case 280: ag_rp_280(); break;
    case 281: ag_rp_281(); break;
    case 282: ag_rp_282(); break;
    case 283: ag_rp_283(); break;
    case 284: ag_rp_284(); break;
    case 285: ag_rp_285(); break;
    case 286: ag_rp_286(); break;
    case 287: ag_rp_287(); break;
    case 288: ag_rp_288(); break;
    case 289: ag_rp_289(); break;
    case 290: ag_rp_290(); break;
    case 291: ag_rp_291(); break;
    case 292: ag_rp_292(); break;
    case 293: ag_rp_293(); break;
    case 294: ag_rp_294(); break;
    case 295: ag_rp_295(); break;
    case 296: ag_rp_296(); break;
    case 297: ag_rp_297(); break;
    case 298: ag_rp_298(); break;
    case 299: ag_rp_299(); break;
    case 300: ag_rp_300(); break;
    case 301: ag_rp_301(); break;
    case 302: ag_rp_302(); break;
    case 303: ag_rp_303(); break;
    case 304: ag_rp_304(); break;
    case 305: ag_rp_305(); break;
    case 306: ag_rp_306(); break;
    case 307: ag_rp_307(); break;
    case 308: ag_rp_308(); break;
    case 309: ag_rp_309(); break;
    case 310: ag_rp_310(); break;
    case 311: ag_rp_311(); break;
    case 312: ag_rp_312(); break;
    case 313: ag_rp_313(); break;
    case 314: ag_rp_314(); break;
    case 315: ag_rp_315(); break;
    case 316: ag_rp_316(); break;
    case 317: ag_rp_317(); break;
    case 318: ag_rp_318(); break;
    case 319: ag_rp_319(); break;
    case 320: ag_rp_320(); break;
    case 321: ag_rp_321(); break;
    case 322: ag_rp_322(); break;
    case 323: ag_rp_323(); break;
    case 324: ag_rp_324(); break;
    case 325: ag_rp_325(); break;
    case 326: ag_rp_326(); break;
    case 327: ag_rp_327(); break;
    case 328: ag_rp_328(); break;
    case 329: ag_rp_329(); break;
    case 330: ag_rp_330(); break;
  }
}

#define TOKEN_NAMES a85parse_token_names
const char *const a85parse_token_names[709] = {
  "a85parse",
  "WS",
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
  "literal name nows",
  "pagespec",
  "and exp",
  "shift exp",
  "multiplicative exp",
  "inclusive or exp",
  "additive exp",
  "exclusive or exp",
  "urinary exp",
  "page exp",
  "instruction list",
  "instruction",
  "expression",
  "a85parse",
  "statement",
  "",
  "",
  "eof",
  "comment",
  "cstyle comment",
  "\"//\"",
  "any text char",
  "",
  "",
  "'\\n'",
  "';'",
  "cstyle comment head",
  "\"*/\"",
  "\"/*\"",
  "",
  "",
  "",
  "preproc inst",
  "'.'",
  "",
  "\"EQU\"",
  "equation",
  "\"SET\"",
  "\"INCLUDE\"",
  "\"INCL\"",
  "",
  "cdseg statement",
  "error",
  "preproc start",
  "\"#\"",
  "preprocessor directive",
  "\"PRAGMA\"",
  "\"LIST\"",
  "\"HEX\"",
  "\"ENTRY\"",
  "\"VERILOG\"",
  "\"EXTENDED\"",
  "\"IFDEF\"",
  "",
  "\"IF\"",
  "condition",
  "\"ELIF\"",
  "\"IFNDEF\"",
  "\"ELSE\"",
  "\"ENDIF\"",
  "\"ERROR\"",
  "\"UNDEF\"",
  "\"DEFINE\"",
  "macro definition",
  "macro expansion",
  "'('",
  "parameter list",
  "')'",
  "macro",
  "",
  "",
  "define chars",
  "\"\\\\\\n\"",
  "cdseg statement start",
  "\"CSEG\"",
  "\"DSEG\"",
  "\"ORG\"",
  "\"$=\"",
  "\"*=\"",
  "",
  "\"ASEG\"",
  "\"DS\"",
  "\"BLOCK\"",
  "",
  "\"DB\"",
  "\"BYTE\"",
  "\"TEXT\"",
  "",
  "\"DW\"",
  "\"WORD\"",
  "",
  "\"PUBLIC\"",
  "name list",
  "\"EXTRN\"",
  "\"EXTERN\"",
  "\"MODULE\"",
  "\"NAME\"",
  "\"STKLN\"",
  "\"ECHO\"",
  "\"FILL\"",
  "\"PRINTF\"",
  "','",
  "\"END\"",
  "\"MSFIRST\"",
  "\"LSFIRST\"",
  "\"TITLE\"",
  "\"NOPAGE\"",
  "\"SYM\"",
  "\"LINK\"",
  "\"MACLIB\"",
  "\"PAGE\"",
  "\",\"",
  "literal alpha",
  "\"$\"",
  "\"&\"",
  "",
  "",
  "digit",
  "asm incl char",
  "'\\''",
  "'\\\"'",
  "",
  "'\\\\'",
  "",
  "'n'",
  "'t'",
  "'r'",
  "'0'",
  "'b'",
  "'f'",
  "",
  "",
  "'x'",
  "'$'",
  "",
  "hex digit",
  "'>'",
  "'<'",
  "",
  "",
  "\":\"",
  "\".BSS\"",
  "\".TEXT\"",
  "\".DATA\"",
  "\"INPAGE\"",
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
  "\"FLOOR\"",
  "\"CEIL\"",
  "\"LN\"",
  "\"LOG\"",
  "\"SQRT\"",
  "\"IP\"",
  "\"FP\"",
  "\"HIGH\"",
  "\"LOW\"",
  "\"DEFINED\"",
  "binary integer",
  "octal integer",
  "",
  "\"Q\"",
  "\"O\"",
  "",
  "'d'",
  "'_'",
  "",
  "",
  "",
  "'@'",
  "",
  "\"\\'\\\\\\'\"",
  "",
  "\"\\'\\'\"",
  "\"B\"",
  "\"C\"",
  "\"D\"",
  "\"E\"",
  "\"H\"",
  "\"L\"",
  "\"M\"",
  "\"(HL)\"",
  "",
  "\"A\"",
  "\"BC\"",
  "",
  "\"DE\"",
  "",
  "\"HL\"",
  "",
  "\"SP\"",
  "stack register",
  "\"PSW\"",
  "bd register",
  "page register",
  "\"DX\"",
  "\"PC\"",
  "eight bit reg inst",
  "sixteen bit reg inst",
  "bd reg inst",
  "stack reg inst",
  "immediate operand inst",
  "lxi inst",
  "mvi inst",
  "spi inst",
  "rst inst",
  "no arg inst",
  "\"!\"",
  "\"ASHR\"",
  "\"SHLR\"",
  "\"ARHL\"",
  "\"RRHL\"",
  "",
  "\"CMA\"",
  "\"CMC\"",
  "\"DAA\"",
  "\"DI\"",
  "\"DSUB\"",
  "\"HLMBC\"",
  "",
  "\"EI\"",
  "\"HLT\"",
  "\"LHLX\"",
  "\"LHLI\"",
  "\"LHLDE\"",
  "",
  "\"NOP\"",
  "\"PCHL\"",
  "\"RAL\"",
  "\"RAR\"",
  "\"RC\"",
  "\"RET\"",
  "\"RIM\"",
  "\"RLC\"",
  "\"RLDE\"",
  "\"RDEL\"",
  "",
  "\"RM\"",
  "\"RNC\"",
  "\"RNZ\"",
  "\"RP\"",
  "\"RPE\"",
  "\"RPO\"",
  "\"RRC\"",
  "\"RSTV\"",
  "\"RZ\"",
  "\"SHLX\"",
  "\"SHLI\"",
  "\"SHLDE\"",
  "",
  "\"SIM\"",
  "\"SPHL\"",
  "\"STC\"",
  "\"XCHG\"",
  "\"XTHL\"",
  "\"LRET\"",
  "\"STAX\"",
  "\"LDAX\"",
  "\"POP\"",
  "\"PUSH\"",
  "\"LPOP\"",
  "\"LPUSH\"",
  "\"ADC\"",
  "\"ADD\"",
  "\"ANA\"",
  "\"CMP\"",
  "\"DCR\"",
  "\"INR\"",
  "\"MOV\"",
  "\"ORA\"",
  "\"SBB\"",
  "\"SUB\"",
  "\"XRA\"",
  "\"SPG\"",
  "\"RPG\"",
  "\"DAD\"",
  "\"DCX\"",
  "\"INX\"",
  "lxi inst start",
  "\"LXI\"",
  "mvi inst start",
  "\"MVI\"",
  "spi inst start",
  "\"SPI\"",
  "\"RST\"",
  "rst arg",
  "\"ACI\"",
  "\"ADI\"",
  "\"ANI\"",
  "\"CALL\"",
  "\"CC\"",
  "\"CM\"",
  "\"CNC\"",
  "\"CNZ\"",
  "\"CP\"",
  "\"CPE\"",
  "\"CPI\"",
  "\"CPO\"",
  "\"CZ\"",
  "\"IN\"",
  "\"JC\"",
  "\"JX\"",
  "\"JX5\"",
  "\"JTM\"",
  "\"JK\"",
  "\"JD\"",
  "",
  "\"JM\"",
  "\"JMP\"",
  "\"JNC\"",
  "\"JNX\"",
  "\"JNX5\"",
  "\"JTP\"",
  "\"JNK\"",
  "\"JND\"",
  "",
  "\"JNZ\"",
  "\"JP\"",
  "\"JPE\"",
  "\"JPO\"",
  "\"JZ\"",
  "\"LDA\"",
  "\"LDEH\"",
  "\"DEHL\"",
  "\"LDHI\"",
  "",
  "\"LDES\"",
  "\"DESP\"",
  "\"LDSI\"",
  "",
  "\"LHLD\"",
  "\"ORI\"",
  "\"OUT\"",
  "\"SBI\"",
  "\"SHLD\"",
  "\"STA\"",
  "\"SUI\"",
  "\"XRI\"",
  "\"BR\"",
  "\"BRA\"",
  "\"BZ\"",
  "\"BNZ\"",
  "\"BNC\"",
  "\"BM\"",
  "\"BP\"",
  "\"BPE\"",
  "\"RCALL\"",
  "\"SBZ\"",
  "\"SBNZ\"",
  "\"SBC\"",
  "\"SBNC\"",
  "\"LCALL\"",
  "\"LJMP\"",
  "",
  "",
  "",
  "",
  "",
  "",
  "",
  "",
  "",
  "",
  "",
  "'\\n'",
  "\"//\"",
  "';'",
  "\"*/\"",
  "\"/*\"",
  "label",
  "'.'",
  "\"EQU\"",
  "\"SET\"",
  "\"INCLUDE\"",
  "\"INCL\"",
  "literal string",
  "include string",
  "asm include",
  "\"#\"",
  "\"LIST\"",
  "\"PRAGMA\"",
  "\"HEX\"",
  "literal name nows",
  "\"ENTRY\"",
  "\"VERILOG\"",
  "\"EXTENDED\"",
  "\"IFDEF\"",
  "\"IF\"",
  "\"ELIF\"",
  "\"IFNDEF\"",
  "\"ELSE\"",
  "\"ENDIF\"",
  "\"ERROR\"",
  "\"UNDEF\"",
  "\"DEFINE\"",
  "')'",
  "parameter list",
  "'('",
  "\"\\\\\\n\"",
  "\"CSEG\"",
  "\"DSEG\"",
  "\"ORG\"",
  "\"$=\"",
  "\"*=\"",
  "\"ASEG\"",
  "\"DS\"",
  "\"BLOCK\"",
  "\"DB\"",
  "\"BYTE\"",
  "\"TEXT\"",
  "\"DW\"",
  "\"WORD\"",
  "\"PUBLIC\"",
  "\"EXTRN\"",
  "\"EXTERN\"",
  "\"MODULE\"",
  "\"NAME\"",
  "\"STKLN\"",
  "\"ECHO\"",
  "\"FILL\"",
  "','",
  "\"PRINTF\"",
  "\"END\"",
  "\"MSFIRST\"",
  "\"LSFIRST\"",
  "singlequote string",
  "\"TITLE\"",
  "\"NOPAGE\"",
  "\"SYM\"",
  "\"LINK\"",
  "\"MACLIB\"",
  "\"PAGE\"",
  "integer",
  "\",\"",
  "\"INPAGE\"",
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
  "\"FLOOR\"",
  "\"CEIL\"",
  "\"LN\"",
  "\"LOG\"",
  "\"SQRT\"",
  "\"IP\"",
  "\"FP\"",
  "\"HIGH\"",
  "\"LOW\"",
  "literal name",
  "\"DEFINED\"",
  "real",
  "\"PSW\"",
  "\"A\"",
  "\"DX\"",
  "\"PC\"",
  "\"SP\"",
  "\"!\"",
  "\"ASHR\"",
  "\"SHLR\"",
  "\"ARHL\"",
  "\"RRHL\"",
  "\"CMA\"",
  "\"CMC\"",
  "\"DAA\"",
  "\"DI\"",
  "\"DSUB\"",
  "\"HLMBC\"",
  "\"EI\"",
  "\"HLT\"",
  "\"LHLX\"",
  "\"LHLI\"",
  "\"LHLDE\"",
  "\"NOP\"",
  "\"PCHL\"",
  "\"RAL\"",
  "\"RAR\"",
  "\"RC\"",
  "\"RET\"",
  "\"RIM\"",
  "\"RLC\"",
  "\"RLDE\"",
  "\"RDEL\"",
  "\"RM\"",
  "\"RNC\"",
  "\"RNZ\"",
  "\"RP\"",
  "\"RPE\"",
  "\"RPO\"",
  "\"RRC\"",
  "\"RSTV\"",
  "\"RZ\"",
  "\"SHLX\"",
  "\"SHLI\"",
  "\"SHLDE\"",
  "\"SIM\"",
  "\"SPHL\"",
  "\"STC\"",
  "\"XCHG\"",
  "\"XTHL\"",
  "\"LRET\"",
  "\"STAX\"",
  "\"LDAX\"",
  "\"POP\"",
  "\"PUSH\"",
  "\"LPOP\"",
  "\"LPUSH\"",
  "register 8 bit",
  "\"ADC\"",
  "\"ADD\"",
  "\"ANA\"",
  "\"CMP\"",
  "\"DCR\"",
  "\"INR\"",
  "\"MOV\"",
  "\"ORA\"",
  "\"SBB\"",
  "\"SUB\"",
  "\"XRA\"",
  "\"SPG\"",
  "\"RPG\"",
  "register 16 bit",
  "\"DAD\"",
  "\"DCX\"",
  "\"INX\"",
  "\"LXI\"",
  "\"MVI\"",
  "\"SPI\"",
  "\"RST\"",
  "\"ACI\"",
  "\"ADI\"",
  "\"ANI\"",
  "\"CALL\"",
  "\"CC\"",
  "\"CM\"",
  "\"CNC\"",
  "\"CNZ\"",
  "\"CP\"",
  "\"CPE\"",
  "\"CPI\"",
  "\"CPO\"",
  "\"CZ\"",
  "\"IN\"",
  "\"JC\"",
  "\"JX\"",
  "\"JX5\"",
  "\"JTM\"",
  "\"JK\"",
  "\"JD\"",
  "\"JM\"",
  "\"JMP\"",
  "\"JNC\"",
  "\"JNX\"",
  "\"JNX5\"",
  "\"JTP\"",
  "\"JNK\"",
  "\"JND\"",
  "\"JNZ\"",
  "\"JP\"",
  "\"JPE\"",
  "\"JPO\"",
  "\"JZ\"",
  "\"LDA\"",
  "\"LDEH\"",
  "\"DEHL\"",
  "\"LDHI\"",
  "\"LDES\"",
  "\"DESP\"",
  "\"LDSI\"",
  "\"LHLD\"",
  "\"ORI\"",
  "\"OUT\"",
  "\"SBI\"",
  "\"SHLD\"",
  "\"STA\"",
  "\"SUI\"",
  "\"XRI\"",
  "\"BR\"",
  "\"BRA\"",
  "\"BZ\"",
  "\"BNZ\"",
  "\"BC\"",
  "\"BNC\"",
  "\"BM\"",
  "\"BP\"",
  "\"BPE\"",
  "\"RCALL\"",
  "\"SBZ\"",
  "\"SBNZ\"",
  "\"SBC\"",
  "\"SBNC\"",
  "\"LCALL\"",
  "\"LJMP\"",
  "",
  "'\\\"'",
  "",
  "'$'",
  "'\\''",
  "'0'",
  "",
  "",
  "",
  "",
  "'<'",
  "'>'",
  "",
  "'@'",
  "",
  "'b'",
  "'d'",
  "'f'",
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
    0,0,  0,0,  0,0, 64,1,430,1, 53,1, 35,1, 35,1, 35,1, 35,1, 35,1,  0,0,
   66,1, 66,1, 66,1,  0,0,  0,0, 66,1, 66,1,  0,0, 66,1, 66,1,  0,0, 66,1,
   66,1, 66,1, 66,1, 66,1, 66,1, 66,1, 66,1, 66,1, 66,1, 66,1,  0,0,  0,0,
   35,2, 47,1,  0,0, 39,1, 40,1, 39,1, 39,1, 35,2, 62,1,267,1,267,1,267,1,
  267,1,267,1,267,1,267,1,267,1,267,1,267,1,267,1,267,1,267,1,267,1,343,1,
  341,1,339,1,262,1,262,1,262,1,262,1,262,1,262,1,262,1,262,1,262,1,262,1,
  262,1,262,1,262,1,262,1,262,1,262,1,267,1,267,1,267,1,267,1,267,1,267,1,
  267,1,267,1,267,1,267,1,267,1,267,1,267,1,267,1,267,1,267,1,267,1,267,1,
  267,1,267,1,267,1,267,1,267,1,267,1,267,1,267,1,267,1,267,1,267,1,266,1,
  343,1,341,1,339,1,262,1,262,1,262,1,262,1,262,1,262,1,262,1,262,1,262,1,
  262,1,262,1,262,1,262,1,262,1,262,1,262,1,262,1,262,1,262,1,262,1,262,1,
  262,1,262,1,262,1,262,1,262,1,262,1,262,1,262,1,262,1,262,1,262,1,262,1,
  262,1,262,1,262,1,262,1,262,1,262,1,262,1,262,1,262,1,262,1,262,1,262,1,
  262,1,262,1,262,1,261,1,261,1,261,1,261,1,260,1,260,1,259,1,259,1,259,1,
  258,1,258,1,258,1,258,1,258,1,258,1,258,1,258,1,258,1,258,1,258,1,258,1,
  258,1,266,1,265,1,264,1,263,1,262,1,262,1,262,1,262,1,262,1,262,1,262,1,
  262,1,262,1,262,1,262,1,262,1,262,1,262,1,262,1,262,1,262,1,262,1,262,1,
  262,1,262,1,262,1,262,1,262,1,262,1,262,1,262,1,262,1,262,1,262,1,262,1,
  262,1,262,1,262,1,262,1,262,1,262,1,262,1,262,1,262,1,262,1,262,1,262,1,
  262,1,262,1,262,1,262,1,262,1,262,1,262,1,262,1,262,1,261,1,261,1,261,1,
  261,1,260,1,260,1,259,1,259,1,259,1,258,1,258,1,258,1,258,1,258,1,258,1,
  258,1,258,1,258,1,258,1,258,1,258,1,258,1,  0,0, 32,1, 89,1, 30,1, 32,1,
   33,1, 33,1, 35,2, 84,1, 66,2, 66,2,436,1,  0,0, 66,2, 66,2, 66,2, 66,2,
  540,1,220,1,219,1,  6,1,  6,1,  7,1, 15,1,540,1,493,1,220,1,219,1,  6,1,
  207,1,208,1,  0,0,208,1,208,1,208,1,208,1,208,1,208,1,208,1,208,1,208,1,
  207,1,  0,0,  0,0,208,1,208,1,208,1,208,1,208,1,208,1,208,1,208,1,208,1,
  208,1,208,1, 29,1,  0,0, 29,1,  0,0,199,1, 29,1, 29,1, 29,1, 29,1, 25,1,
   27,1, 24,1, 23,1, 28,1, 26,1, 26,1, 76,1, 76,1, 66,2, 66,2, 66,2,  0,0,
   66,2,  0,0, 66,2,  0,0, 66,2,  0,0, 66,2,  0,0, 66,2,437,1,  0,0, 66,2,
   66,2,  0,0, 35,3,  0,0,  0,0,  0,0,  0,0,  0,0,  0,0,  0,0,  0,0,  0,0,
    0,0,  0,0,  0,0,  0,0,  0,0,  0,0,  0,0,  0,0,  0,0,  0,0,  0,0,  0,0,
    0,0,  0,0,  0,0,  0,0,  0,0,  0,0,  0,0,  0,0,  0,0,  0,0,  0,0,  0,0,
    0,0,  0,0,  0,0,  0,0,  0,0,  0,0, 39,2, 39,2, 33,1, 35,3, 62,2,343,2,
  341,2,339,2,266,2,265,2,264,2,263,2,262,2,262,2,262,2,262,2,262,2,262,2,
  262,2,262,2,262,2,262,2,262,2,262,2,262,2,262,2,262,2,262,2,262,2,262,2,
  262,2,262,2,262,2,262,2,262,2,262,2,262,2,262,2,262,2,262,2,262,2,262,2,
  262,2,262,2,262,2,262,2,262,2,262,2,262,2,262,2,262,2,262,2,262,2,262,2,
  262,2,262,2,262,2,262,2,262,2,262,2,262,2,262,2,262,2,262,2,261,2,261,2,
  261,2,261,2,260,2,260,2,259,2,259,2,259,2,258,2,258,2,258,2,258,2,258,2,
  258,2,258,2,258,2,258,2,258,2,258,2,258,2,258,2, 89,2,  0,0,  0,0, 32,2,
    0,0, 33,2,  0,0, 33,2,  0,0,  0,0,  0,0, 33,2,  0,0,  0,0,  0,0, 33,2,
    0,0, 33,2,  0,0, 33,2,  0,0, 33,2,  0,0, 33,2,  0,0, 33,2,  0,0, 33,2,
    0,0, 33,2,  0,0, 33,2,  0,0, 33,2,  0,0,  0,0, 33,2,  0,0,  0,0,  0,0,
   33,2,  0,0,  0,0, 33,2,  0,0,  0,0,  0,0,  0,0, 33,2, 33,2,  0,0,  0,0,
   35,3, 84,2, 66,3, 66,3,  0,0, 66,3, 66,3,  6,2, 16,1, 15,2,  0,0,  0,0,
  208,2,208,2,208,2,208,2,208,2,208,2,208,2,208,2,208,2,208,2,208,2,199,2,
    7,1,  0,0, 25,2,  0,0, 25,2, 25,2, 25,2, 25,2, 27,2, 27,2,  0,0, 24,2,
    0,0, 24,2,  0,0, 24,2,  0,0, 24,2,  0,0, 23,2, 23,2,  0,0, 28,2, 28,2,
    0,0, 26,2, 26,2,  0,0,  0,0,  0,0,  0,0,  0,0,  0,0,  0,0,  0,0,  0,0,
    0,0,  0,0,  0,0,  0,0, 66,3, 66,3, 66,3, 66,3, 66,3, 66,3, 66,3, 66,3,
  437,2, 66,3, 66,3,  0,0, 35,4, 35,4, 35,4,  0,0, 35,4,  0,0, 35,4, 35,4,
   22,1, 62,3,255,1,255,1,255,1,  0,0,  0,0,265,3,264,3,263,3,252,1,252,1,
  258,3,  0,0,486,1,  0,0,  0,0,  6,1,493,1, 32,3,486,1, 33,3, 33,3,113,1,
  113,1,113,1,  0,0,  0,0,  0,0, 84,3, 85,1,  0,0,  0,0,  0,0, 15,3,208,3,
  208,3,208,3,208,3,208,3,208,3,208,3,208,3,208,3,208,3,208,3,199,3, 25,3,
   25,3, 25,3, 25,3, 25,3, 27,3, 27,3, 24,3, 24,3, 24,3, 24,3, 23,3, 23,3,
   28,3, 28,3, 26,3, 26,3, 66,4, 35,5, 35,5, 35,5, 35,5, 35,5, 62,4,265,4,
  264,4,263,4,258,4,  0,0,  0,0,  0,0, 32,1, 32,1,486,2, 33,4,  0,0,113,2,
    0,0,  0,0, 66,5,  0,0,  0,0,208,4,208,4,208,4,208,4,208,4,208,4,208,4,
  208,4,208,4,208,4,208,4,199,4, 25,1, 25,1, 27,1, 27,1, 27,1, 27,1, 24,1,
   24,1, 23,1, 23,1, 28,1, 28,1, 66,5, 35,6, 35,6,258,5, 15,3,  0,0, 33,5,
  113,3,538,1,  0,0,208,5,258,6,208,6
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
    while (ag_tstt[ag_k] != 63 && ag_tstt[ag_k]) ag_k++;
    if (ag_tstt[ag_k] || (PCB).ssx == 0) break;
    (PCB).sn = (PCB).ss[--(PCB).ssx];
  }
  if (ag_tstt[ag_k] == 0) {
    (PCB).sn = PCB.ss[(PCB).ssx = ag_ssx];
    (PCB).exit_flag = AG_SYNTAX_ERROR_CODE;
    return;
  }
  ag_k = ag_sbt[(PCB).sn];
  while (ag_tstt[ag_k] != 63 && ag_tstt[ag_k]) ag_k++;
  (PCB).ag_ap = ag_pstt[ag_k];
  (ag_er_procs_scan[ag_astt[ag_k]])();
  while (1) {
    ag_k = ag_sbt[(PCB).sn];
    while (ag_tstt[ag_k] != (const unsigned short) (PCB).token_number && ag_tstt[ag_k])
      ag_k++;
    if (ag_tstt[ag_k] && ag_astt[ag_k] != ag_action_10) break;
    if ((PCB).token_number == 38)
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
  while (ag_tstt[ag_k] != 63 && ag_tstt[ag_k]) ag_k++;
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


