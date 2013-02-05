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

#ifndef A85PARSE_H_1359819567
#include "a85parse.h"
#endif

#ifndef A85PARSE_H_1359819567
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

/*  Line 706, C:/Projects/VirtualT/src/a85parse.syn */
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
	gAsm->preprocessor() : 0; }
	
//	fgetc(gAsm->m_fd) : 0; if ((PCB).input_code == 13) (PCB).input_code = fgetc(gAsm->m_fd);\
//	}

#define SYNTAX_ERROR { syntax_error(TOKEN_NAMES[(PCB).error_frame_token]); }

/*
#define SYNTAX_ERROR {MString string;  if ((PCB).error_frame_token != 0) \
    string.Format("Error in line %d(%s), column%d:  Malformed %s - %s", \
    (PCB).line, gFilename, (PCB).column, \*
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

#define ag_rp_3() (gAsm->directive_set((const char *) -1))

#define ag_rp_4() (gAsm->include(ss[ss_idx--]))

#define ag_rp_5() (gAsm->include(ss[ss_idx--]))

#define ag_rp_6() (gAsm->include(ss[ss_idx--]))

#define ag_rp_7() (gAsm->directive_cdseg(seg, page))

#define ag_rp_8() (gAsm->directive_cdseg(seg, page))

#define ag_rp_9() (handle_error())

#define ag_rp_10() (gAsm->include(ss[ss_idx--]))

#define ag_rp_11() (gAsm->include(ss[ss_idx--]))

#define ag_rp_12() (gAsm->pragma_list())

#define ag_rp_13() (gAsm->pragma_hex())

#define ag_rp_14() (gAsm->pragma_entry(ss[ss_idx--]))

#define ag_rp_15() (gAsm->pragma_verilog())

#define ag_rp_16() (gAsm->pragma_extended())

#define ag_rp_17() (gAsm->preproc_ifdef(ss[ss_idx--]))

#define ag_rp_18() (gAsm->preproc_if())

#define ag_rp_19() (gAsm->preproc_elif())

#define ag_rp_20() (gAsm->preproc_ifndef(ss[ss_idx--]))

#define ag_rp_21() (gAsm->preproc_else())

#define ag_rp_22() (gAsm->preproc_endif())

static void ag_rp_23(void) {
/* Line 128, C:/Projects/VirtualT/src/a85parse.syn */
if (gAsm->preproc_error(ss[ss_idx--])) gAbort = TRUE;
}

#define ag_rp_24() (gAsm->directive_printf(ss[ss_idx--], FALSE))

#define ag_rp_25() (gAsm->directive_printf("%d"))

#define ag_rp_26() (gAsm->directive_printf(ss[ss_idx--], FALSE))

#define ag_rp_27() (gAsm->directive_printf(ss[ss_idx--]))

#define ag_rp_28() (gAsm->preproc_undef(ss[ss_idx--]))

static void ag_rp_29(void) {
/* Line 136, C:/Projects/VirtualT/src/a85parse.syn */
 if (gMacroStack[ms_idx] != 0) { delete gMacroStack[ms_idx]; \
															gMacroStack[ms_idx] = 0; } \
														gMacro = gMacroStack[--ms_idx]; \
														gMacro->m_DefString = ss[ss_idx--]; \
														gAsm->preproc_define(); \
														gMacroStack[ms_idx] = 0; gDefine = 0; 
}

static void ag_rp_30(void) {
/* Line 150, C:/Projects/VirtualT/src/a85parse.syn */
 gMacro->m_ParamList = gExpList; \
															gMacro->m_Name = ss[ss_idx--]; \
															gExpList = new VTObArray; \
															gMacroStack[ms_idx++] = gMacro; \
															if (gAsm->preproc_macro()) \
																PCB.reduction_token = a85parse_WS_token; \
															gMacro = new CMacro; 
}

static void ag_rp_31(void) {
/* Line 157, C:/Projects/VirtualT/src/a85parse.syn */
 gMacro->m_Name = ss[ss_idx--]; gMacroStack[ms_idx++] = gMacro; \
															if (gAsm->preproc_macro()) \
																PCB.reduction_token = a85parse_WS_token; \
															gMacro = new CMacro; 
}

static void ag_rp_32(int c) {
/* Line 163, C:/Projects/VirtualT/src/a85parse.syn */
 ss[++ss_idx][0] = c; ss[ss_idx][1] = 0; ss_len = 1; 
}

static void ag_rp_33(int c) {
/* Line 164, C:/Projects/VirtualT/src/a85parse.syn */
 ss[ss_idx][ss_len++] = c; ss[ss_idx][ss_len] = 0; 
}

static void ag_rp_34(void) {
/* Line 165, C:/Projects/VirtualT/src/a85parse.syn */
 ss[ss_idx][ss_len++] = '\n'; ss[ss_idx][ss_len] = 0; 
}

static void ag_rp_35(void) {
/* Line 168, C:/Projects/VirtualT/src/a85parse.syn */
 page = 0; seg = CSEG; 
}

static void ag_rp_36(void) {
/* Line 169, C:/Projects/VirtualT/src/a85parse.syn */
 page = 0; seg = DSEG; 
}

#define ag_rp_37(p) (page = p)

#define ag_rp_38() (gAsm->pragma_list())

#define ag_rp_39() (gAsm->pragma_hex())

#define ag_rp_40() (gAsm->pragma_verilog())

#define ag_rp_41() (gAsm->pragma_entry(ss[ss_idx--]))

#define ag_rp_42() (gAsm->pragma_extended())

#define ag_rp_43() (gAsm->directive_org())

#define ag_rp_44() (gAsm->directive_aseg())

#define ag_rp_45() (gAsm->directive_ds())

#define ag_rp_46() (gAsm->directive_db())

#define ag_rp_47() (gAsm->directive_dw())

#define ag_rp_48() (gAsm->directive_public())

#define ag_rp_49() (gAsm->directive_extern())

#define ag_rp_50() (gAsm->directive_extern())

#define ag_rp_51() (gAsm->directive_module(ss[ss_idx--]))

#define ag_rp_52() (gAsm->directive_name(ss[ss_idx--]))

#define ag_rp_53() (gAsm->directive_stkln())

#define ag_rp_54() (gAsm->directive_echo())

#define ag_rp_55() (gAsm->directive_echo(ss[ss_idx--]))

#define ag_rp_56() (gAsm->directive_fill())

#define ag_rp_57() (gAsm->directive_printf(ss[ss_idx--], FALSE))

#define ag_rp_58() (gAsm->directive_printf("%d"))

#define ag_rp_59() (gAsm->directive_printf(ss[ss_idx--], FALSE))

#define ag_rp_60() (gAsm->directive_printf(ss[ss_idx--]))

#define ag_rp_61() (gAsm->directive_end(""))

#define ag_rp_62() (gAsm->directive_end(ss[ss_idx--]))

#define ag_rp_63() (gAsm->directive_if())

#define ag_rp_64() (gAsm->directive_else())

#define ag_rp_65() (gAsm->directive_endif())

#define ag_rp_66() (gAsm->directive_endian(1))

#define ag_rp_67() (gAsm->directive_endian(0))

#define ag_rp_68() (gAsm->directive_title(ss[ss_idx--]))

#define ag_rp_69() (gAsm->directive_title(ss[ss_idx--]))

#define ag_rp_70() (gAsm->directive_page(-1))

#define ag_rp_71() (gAsm->directive_sym())

#define ag_rp_72() (gAsm->directive_link(ss[ss_idx--]))

#define ag_rp_73() (gAsm->directive_maclib(ss[ss_idx--]))

#define ag_rp_74() (gAsm->directive_page(page))

#define ag_rp_75() (page = 60)

#define ag_rp_76(n) (page = n)

#define ag_rp_77() (expression_list_literal())

#define ag_rp_78() (expression_list_literal())

#define ag_rp_79() (expression_list_equation())

#define ag_rp_80() (expression_list_equation())

#define ag_rp_81() (expression_list_literal())

#define ag_rp_82() (expression_list_literal())

#define ag_rp_83() (gNameList->Add(ss[ss_idx--]))

#define ag_rp_84() (gNameList->Add(ss[ss_idx--]))

static void ag_rp_85(void) {
/* Line 253, C:/Projects/VirtualT/src/a85parse.syn */
 strcpy(ss[++ss_idx], "$"); ss_len = 1; 
}

static void ag_rp_86(void) {
/* Line 254, C:/Projects/VirtualT/src/a85parse.syn */
 strcpy(ss[++ss_idx], "&"); ss_len = 1; 
}

static void ag_rp_87(int c) {
/* Line 257, C:/Projects/VirtualT/src/a85parse.syn */
 ss[++ss_idx][0] = c; ss[ss_idx][1] = 0; ss_len = 1; \
														if (PCB.column == 2) ss_addr = gAsm->m_ActiveSeg->m_Address; 
}

static void ag_rp_88(int c) {
/* Line 259, C:/Projects/VirtualT/src/a85parse.syn */
 ss[ss_idx][ss_len++] = c; ss[ss_idx][ss_len] = 0; 
}

static void ag_rp_89(int c) {
/* Line 260, C:/Projects/VirtualT/src/a85parse.syn */
 ss[ss_idx][ss_len++] = c; ss[ss_idx][ss_len] = 0; 
}

static void ag_rp_90(int c) {
/* Line 263, C:/Projects/VirtualT/src/a85parse.syn */
 ss[++ss_idx][0] = c; ss[ss_idx][1] = 0; ss_len = 1; 
}

static void ag_rp_91(int c) {
/* Line 264, C:/Projects/VirtualT/src/a85parse.syn */
 ss[ss_idx][ss_len++] = c; ss[ss_idx][ss_len] = 0; 
}

static void ag_rp_92(int ch1, int ch2) {
/* Line 270, C:/Projects/VirtualT/src/a85parse.syn */
 ss_idx++; ss_len = 2; sprintf(ss[ss_idx], "%c%c", ch1, ch2); 
}

static void ag_rp_93(int c) {
/* Line 271, C:/Projects/VirtualT/src/a85parse.syn */
 ss[ss_idx][ss_len++] = c; ss[ss_idx][ss_len] = 0; 
}

static void ag_rp_94(void) {
/* Line 278, C:/Projects/VirtualT/src/a85parse.syn */
 ss_idx++; ss_len = 0; 
}

static void ag_rp_95(int c) {
/* Line 279, C:/Projects/VirtualT/src/a85parse.syn */
 ss[ss_idx][ss_len++] = c; ss[ss_idx][ss_len] = 0; 
}

#define ag_rp_96(n) (n)

#define ag_rp_97() ('\\')

#define ag_rp_98() ('\n')

#define ag_rp_99() ('\t')

#define ag_rp_100() ('\r')

#define ag_rp_101() ('\0')

#define ag_rp_102() ('"')

#define ag_rp_103() (0x08)

#define ag_rp_104() (0x0C)

#define ag_rp_105(n1, n2, n3) ((n1-'0') * 64 + (n2-'0') * 8 + n3-'0')

#define ag_rp_106(n1, n2) (chtoh(n1) * 16 + chtoh(n2))

#define ag_rp_107(n1) (chtoh(n1))

static void ag_rp_108(void) {
/* Line 299, C:/Projects/VirtualT/src/a85parse.syn */
 ss[ss_idx][ss_len++] = '>'; ss[ss_idx][ss_len] = 0; 
}

static void ag_rp_109(void) {
/* Line 302, C:/Projects/VirtualT/src/a85parse.syn */
 ss[++ss_idx][0] = '<'; ss[ss_idx][1] = 0; ss_len = 1; 
}

static void ag_rp_110(int c) {
/* Line 303, C:/Projects/VirtualT/src/a85parse.syn */
 ss[ss_idx][ss_len++] = c; ss[ss_idx][ss_len] = 0; 
}

static void ag_rp_111(int c) {
/* Line 304, C:/Projects/VirtualT/src/a85parse.syn */
 ss[ss_idx][ss_len++] = '\\'; ss[ss_idx][ss_len++] = c; ss[ss_idx][ss_len] = 0; 
}

#define ag_rp_112() (gAsm->label(ss[ss_idx--]))

#define ag_rp_113() (gAsm->label(ss[ss_idx--]))

#define ag_rp_114() (gAsm->label(".bss"))

#define ag_rp_115() (gAsm->label(".text"))

#define ag_rp_116() (gAsm->label(".data"))

#define ag_rp_117() (PAGE)

#define ag_rp_118() (INPAGE)

#define ag_rp_119() (condition(-1))

#define ag_rp_120() (condition(COND_NOCMP))

#define ag_rp_121() (condition(COND_EQ))

#define ag_rp_122() (condition(COND_NE))

#define ag_rp_123() (condition(COND_GE))

#define ag_rp_124() (condition(COND_LE))

#define ag_rp_125() (condition(COND_GT))

#define ag_rp_126() (condition(COND_LT))

#define ag_rp_127() (gEq->Add(RPN_BITOR))

#define ag_rp_128() (gEq->Add(RPN_BITOR))

#define ag_rp_129() (gEq->Add(RPN_BITXOR))

#define ag_rp_130() (gEq->Add(RPN_BITXOR))

#define ag_rp_131() (gEq->Add(RPN_BITAND))

#define ag_rp_132() (gEq->Add(RPN_BITAND))

#define ag_rp_133() (gEq->Add(RPN_LEFTSHIFT))

#define ag_rp_134() (gEq->Add(RPN_LEFTSHIFT))

#define ag_rp_135() (gEq->Add(RPN_RIGHTSHIFT))

#define ag_rp_136() (gEq->Add(RPN_RIGHTSHIFT))

#define ag_rp_137() (gEq->Add(RPN_ADD))

#define ag_rp_138() (gEq->Add(RPN_SUBTRACT))

#define ag_rp_139() (gEq->Add(RPN_MULTIPLY))

#define ag_rp_140() (gEq->Add(RPN_DIVIDE))

#define ag_rp_141() (gEq->Add(RPN_MODULUS))

#define ag_rp_142() (gEq->Add(RPN_MODULUS))

#define ag_rp_143() (gEq->Add(RPN_EXPONENT))

#define ag_rp_144() (gEq->Add(RPN_NOT))

#define ag_rp_145() (gEq->Add(RPN_NOT))

#define ag_rp_146() (gEq->Add(RPN_BITNOT))

#define ag_rp_147() (gEq->Add(RPN_NEGATE))

#define ag_rp_148(n) (gEq->Add((double) n))

static void ag_rp_149(void) {
/* Line 395, C:/Projects/VirtualT/src/a85parse.syn */
 delete gMacro; gMacro = gMacroStack[ms_idx-1]; \
														gMacroStack[ms_idx--] = 0; if (gMacro->m_ParamList == 0) \
														{\
															gEq->Add(gMacro->m_Name); gMacro->m_Name = ""; }\
														else { \
															gEq->Add((VTObject *) gMacro); gMacro = new CMacro; \
														} 
}

#define ag_rp_150() (gEq->Add(RPN_FLOOR))

#define ag_rp_151() (gEq->Add(RPN_CEIL))

#define ag_rp_152() (gEq->Add(RPN_LN))

#define ag_rp_153() (gEq->Add(RPN_LOG))

#define ag_rp_154() (gEq->Add(RPN_SQRT))

#define ag_rp_155() (gEq->Add(RPN_IP))

#define ag_rp_156() (gEq->Add(RPN_FP))

#define ag_rp_157() (gEq->Add(RPN_HIGH))

#define ag_rp_158() (gEq->Add(RPN_LOW))

#define ag_rp_159() (gEq->Add(RPN_PAGE))

#define ag_rp_160() (gEq->Add(RPN_DEFINED, ss[ss_idx--]))

#define ag_rp_161(n) (n)

#define ag_rp_162(r) (r)

#define ag_rp_163(n) (n)

#define ag_rp_164() (conv_to_dec())

#define ag_rp_165() (conv_to_hex())

#define ag_rp_166() (conv_to_bin())

#define ag_rp_167() (conv_to_oct())

#define ag_rp_168() (conv_to_hex())

#define ag_rp_169() (conv_to_hex())

#define ag_rp_170() (conv_to_bin())

#define ag_rp_171() (conv_to_oct())

#define ag_rp_172() (conv_to_dec())

static void ag_rp_173(int n) {
/* Line 442, C:/Projects/VirtualT/src/a85parse.syn */
 int_len = 1; integer[0] = n; integer[1] = 0; 
}

static void ag_rp_174(int n) {
/* Line 443, C:/Projects/VirtualT/src/a85parse.syn */
 int_len = 2; integer[0] = '-', integer[1] = n; integer[2] = 0; 
}

static void ag_rp_175(int n) {
/* Line 444, C:/Projects/VirtualT/src/a85parse.syn */
 int_len = 1; integer[0] = n; integer[1] = 0; 
}

static void ag_rp_176(int n) {
/* Line 445, C:/Projects/VirtualT/src/a85parse.syn */
 integer[int_len++] = n; integer[int_len] = 0; 
}

static void ag_rp_177(int n) {
/* Line 450, C:/Projects/VirtualT/src/a85parse.syn */
 int_len = 1; integer[0] = n; integer[1] = 0; 
}

static void ag_rp_178(int n) {
/* Line 451, C:/Projects/VirtualT/src/a85parse.syn */
 int_len = 1; integer[0] = n; integer[1] = 0; 
}

static void ag_rp_179(int n) {
/* Line 452, C:/Projects/VirtualT/src/a85parse.syn */
 integer[int_len++] = n; integer[int_len] = 0; 
}

static void ag_rp_180(int n) {
/* Line 455, C:/Projects/VirtualT/src/a85parse.syn */
 integer[int_len++] = n; integer[int_len] = 0; 
}

static void ag_rp_181(int n) {
/* Line 456, C:/Projects/VirtualT/src/a85parse.syn */
 integer[int_len++] = n; integer[int_len] = 0; 
}

static void ag_rp_182(int n) {
/* Line 459, C:/Projects/VirtualT/src/a85parse.syn */
 integer[int_len++] = n; integer[int_len] = 0; 
}

static void ag_rp_183(int n) {
/* Line 460, C:/Projects/VirtualT/src/a85parse.syn */
 integer[int_len++] = n; integer[int_len] = 0; 
}

static void ag_rp_184(int n) {
/* Line 463, C:/Projects/VirtualT/src/a85parse.syn */
 integer[int_len++] = n; integer[int_len] = 0; 
}

static void ag_rp_185(int n) {
/* Line 464, C:/Projects/VirtualT/src/a85parse.syn */
 integer[int_len++] = n; integer[int_len] = 0; 
}

#define ag_rp_186(n) (n)

#define ag_rp_187(n1, n2) ((n1 << 8) | n2)

#define ag_rp_188() ('\\')

#define ag_rp_189(n) (n)

#define ag_rp_190() ('\\')

#define ag_rp_191() ('\n')

#define ag_rp_192() ('\t')

#define ag_rp_193() ('\r')

#define ag_rp_194() ('\0')

#define ag_rp_195() ('\'')

#define ag_rp_196() ('\'')

static double ag_rp_197(void) {
/* Line 485, C:/Projects/VirtualT/src/a85parse.syn */
 gDivisor = 1.0; return (double) conv_to_dec(); 
}

static double ag_rp_198(int d) {
/* Line 486, C:/Projects/VirtualT/src/a85parse.syn */
 gDivisor = 10.0; return ((double) (d - '0') / gDivisor); 
}

static double ag_rp_199(double r, int d) {
/* Line 487, C:/Projects/VirtualT/src/a85parse.syn */
 gDivisor *= 10.0; return (r + (double) (d - '0') / gDivisor); 
}

#define ag_rp_200() (reg[reg_cnt++] = '0')

#define ag_rp_201() (reg[reg_cnt++] = '1')

#define ag_rp_202() (reg[reg_cnt++] = '2')

#define ag_rp_203() (reg[reg_cnt++] = '3')

#define ag_rp_204() (reg[reg_cnt++] = '4')

#define ag_rp_205() (reg[reg_cnt++] = '5')

#define ag_rp_206() (reg[reg_cnt++] = '6')

#define ag_rp_207() (reg[reg_cnt++] = '7')

#define ag_rp_208() (reg[reg_cnt++] = '0')

#define ag_rp_209() (reg[reg_cnt++] = '1')

#define ag_rp_210() (reg[reg_cnt++] = '2')

#define ag_rp_211() (reg[reg_cnt++] = '3')

#define ag_rp_212() (reg[reg_cnt++] = '0')

#define ag_rp_213() (reg[reg_cnt++] = '1')

#define ag_rp_214() (reg[reg_cnt++] = '2')

#define ag_rp_215() (reg[reg_cnt++] = '3')

#define ag_rp_216() (reg[reg_cnt++] = '3')

#define ag_rp_217() (reg[reg_cnt++] = '0')

#define ag_rp_218() (reg[reg_cnt++] = '1')

#define ag_rp_219() (reg[reg_cnt++] = '0')

#define ag_rp_220() (reg[reg_cnt++] = '1')

#define ag_rp_221() (reg[reg_cnt++] = '2')

#define ag_rp_222() (reg[reg_cnt++] = '3')

#define ag_rp_223() (reg[reg_cnt++] = '4')

#define ag_rp_224() (reg[reg_cnt++] = '5')

#define ag_rp_225() (gAsm->opcode_arg_0		(OPCODE_ASHR))

#define ag_rp_226() (gAsm->opcode_arg_0		(OPCODE_CMA))

#define ag_rp_227() (gAsm->opcode_arg_0		(OPCODE_CMC))

#define ag_rp_228() (gAsm->opcode_arg_0		(OPCODE_DAA))

#define ag_rp_229() (gAsm->opcode_arg_0		(OPCODE_DI))

#define ag_rp_230() (gAsm->opcode_arg_0		(OPCODE_DSUB))

#define ag_rp_231() (gAsm->opcode_arg_0		(OPCODE_EI))

#define ag_rp_232() (gAsm->opcode_arg_0		(OPCODE_HLT))

#define ag_rp_233() (gAsm->opcode_arg_0		(OPCODE_LHLX))

#define ag_rp_234() (gAsm->opcode_arg_0		(OPCODE_NOP))

#define ag_rp_235() (gAsm->opcode_arg_0		(OPCODE_PCHL))

#define ag_rp_236() (gAsm->opcode_arg_0		(OPCODE_RAL))

#define ag_rp_237() (gAsm->opcode_arg_0		(OPCODE_RAR))

#define ag_rp_238() (gAsm->opcode_arg_0		(OPCODE_RC))

#define ag_rp_239() (gAsm->opcode_arg_0		(OPCODE_RET))

#define ag_rp_240() (gAsm->opcode_arg_0		(OPCODE_RIM))

#define ag_rp_241() (gAsm->opcode_arg_0		(OPCODE_RLC))

#define ag_rp_242() (gAsm->opcode_arg_0		(OPCODE_RLDE))

#define ag_rp_243() (gAsm->opcode_arg_0		(OPCODE_RM))

#define ag_rp_244() (gAsm->opcode_arg_0		(OPCODE_RNC))

#define ag_rp_245() (gAsm->opcode_arg_0		(OPCODE_RNZ))

#define ag_rp_246() (gAsm->opcode_arg_0		(OPCODE_RP))

#define ag_rp_247() (gAsm->opcode_arg_0		(OPCODE_RPE))

#define ag_rp_248() (gAsm->opcode_arg_0		(OPCODE_RPO))

#define ag_rp_249() (gAsm->opcode_arg_0		(OPCODE_RRC))

#define ag_rp_250() (gAsm->opcode_arg_0		(OPCODE_RSTV))

#define ag_rp_251() (gAsm->opcode_arg_0		(OPCODE_RZ))

#define ag_rp_252() (gAsm->opcode_arg_0		(OPCODE_SHLX))

#define ag_rp_253() (gAsm->opcode_arg_0		(OPCODE_SIM))

#define ag_rp_254() (gAsm->opcode_arg_0		(OPCODE_SPHL))

#define ag_rp_255() (gAsm->opcode_arg_0		(OPCODE_STC))

#define ag_rp_256() (gAsm->opcode_arg_0		(OPCODE_XCHG))

#define ag_rp_257() (gAsm->opcode_arg_0		(OPCODE_XTHL))

#define ag_rp_258() (gAsm->opcode_arg_0		(OPCODE_LRET))

#define ag_rp_259() (gAsm->opcode_arg_1reg		(OPCODE_STAX))

#define ag_rp_260() (gAsm->opcode_arg_1reg		(OPCODE_LDAX))

#define ag_rp_261() (gAsm->opcode_arg_1reg		(OPCODE_POP))

#define ag_rp_262() (gAsm->opcode_arg_1reg		(OPCODE_PUSH))

#define ag_rp_263() (gAsm->opcode_arg_1reg_2byte	(OPCODE_LPOP))

#define ag_rp_264() (gAsm->opcode_arg_1reg_2byte	(OPCODE_LPUSH))

#define ag_rp_265() (gAsm->opcode_arg_1reg		(OPCODE_ADC))

#define ag_rp_266() (gAsm->opcode_arg_1reg		(OPCODE_ADD))

#define ag_rp_267() (gAsm->opcode_arg_1reg		(OPCODE_ANA))

#define ag_rp_268() (gAsm->opcode_arg_1reg		(OPCODE_CMP))

#define ag_rp_269() (gAsm->opcode_arg_1reg		(OPCODE_DCR))

#define ag_rp_270() (gAsm->opcode_arg_1reg		(OPCODE_INR))

#define ag_rp_271() (gAsm->opcode_arg_2reg		(OPCODE_MOV))

#define ag_rp_272() (gAsm->opcode_arg_1reg		(OPCODE_ORA))

#define ag_rp_273() (gAsm->opcode_arg_1reg		(OPCODE_SBB))

#define ag_rp_274() (gAsm->opcode_arg_1reg		(OPCODE_SUB))

#define ag_rp_275() (gAsm->opcode_arg_1reg		(OPCODE_XRA))

#define ag_rp_276() (gAsm->opcode_arg_1reg_2byte	(OPCODE_SPG))

#define ag_rp_277() (gAsm->opcode_arg_1reg_2byte	(OPCODE_RPG))

#define ag_rp_278() (gAsm->opcode_arg_1reg		(OPCODE_DAD))

#define ag_rp_279() (gAsm->opcode_arg_1reg		(OPCODE_DCX))

#define ag_rp_280() (gAsm->opcode_arg_1reg		(OPCODE_INX))

#define ag_rp_281() (gAsm->opcode_arg_1reg_equ16(OPCODE_LXI))

#define ag_rp_282() (gAsm->opcode_arg_1reg_equ8(OPCODE_MVI))

#define ag_rp_283() (gAsm->opcode_arg_1reg_equ16(OPCODE_SPI))

#define ag_rp_284(c) (gAsm->opcode_arg_imm		(OPCODE_RST, c))

#define ag_rp_285() (gAsm->opcode_arg_equ8		(OPCODE_ACI))

#define ag_rp_286() (gAsm->opcode_arg_equ8		(OPCODE_ADI))

#define ag_rp_287() (gAsm->opcode_arg_equ8		(OPCODE_ANI))

#define ag_rp_288() (gAsm->opcode_arg_equ16	(OPCODE_CALL))

#define ag_rp_289() (gAsm->opcode_arg_equ16	(OPCODE_CC))

#define ag_rp_290() (gAsm->opcode_arg_equ16	(OPCODE_CM))

#define ag_rp_291() (gAsm->opcode_arg_equ16	(OPCODE_CNC))

#define ag_rp_292() (gAsm->opcode_arg_equ16	(OPCODE_CNZ))

#define ag_rp_293() (gAsm->opcode_arg_equ16	(OPCODE_CP))

#define ag_rp_294() (gAsm->opcode_arg_equ16	(OPCODE_CPE))

#define ag_rp_295() (gAsm->opcode_arg_equ8		(OPCODE_CPI))

#define ag_rp_296() (gAsm->opcode_arg_equ16	(OPCODE_CPO))

#define ag_rp_297() (gAsm->opcode_arg_equ16	(OPCODE_CZ))

#define ag_rp_298() (gAsm->opcode_arg_equ8		(OPCODE_IN))

#define ag_rp_299() (gAsm->opcode_arg_equ16	(OPCODE_JC))

#define ag_rp_300() (gAsm->opcode_arg_equ16	(OPCODE_JD))

#define ag_rp_301() (gAsm->opcode_arg_equ16	(OPCODE_JM))

#define ag_rp_302() (gAsm->opcode_arg_equ16	(OPCODE_JMP))

#define ag_rp_303() (gAsm->opcode_arg_equ16	(OPCODE_JNC))

#define ag_rp_304() (gAsm->opcode_arg_equ16	(OPCODE_JND))

#define ag_rp_305() (gAsm->opcode_arg_equ16	(OPCODE_JNZ))

#define ag_rp_306() (gAsm->opcode_arg_equ16	(OPCODE_JP))

#define ag_rp_307() (gAsm->opcode_arg_equ16	(OPCODE_JPE))

#define ag_rp_308() (gAsm->opcode_arg_equ16	(OPCODE_JPO))

#define ag_rp_309() (gAsm->opcode_arg_equ16	(OPCODE_JZ))

#define ag_rp_310() (gAsm->opcode_arg_equ16	(OPCODE_LDA))

#define ag_rp_311() (gAsm->opcode_arg_equ8		(OPCODE_LDEH))

#define ag_rp_312() (gAsm->opcode_arg_equ8		(OPCODE_LDES))

#define ag_rp_313() (gAsm->opcode_arg_equ16	(OPCODE_LHLD))

#define ag_rp_314() (gAsm->opcode_arg_equ8		(OPCODE_ORI))

#define ag_rp_315() (gAsm->opcode_arg_equ8		(OPCODE_OUT))

#define ag_rp_316() (gAsm->opcode_arg_equ8		(OPCODE_SBI))

#define ag_rp_317() (gAsm->opcode_arg_equ16	(OPCODE_SHLD))

#define ag_rp_318() (gAsm->opcode_arg_equ16	(OPCODE_STA))

#define ag_rp_319() (gAsm->opcode_arg_equ8		(OPCODE_SUI))

#define ag_rp_320() (gAsm->opcode_arg_equ8		(OPCODE_XRI))

#define ag_rp_321() (gAsm->opcode_arg_equ8		(OPCODE_BR))

#define ag_rp_322() (gAsm->opcode_arg_equ16	(OPCODE_BRA))

#define ag_rp_323() (gAsm->opcode_arg_equ16	(OPCODE_BZ))

#define ag_rp_324() (gAsm->opcode_arg_equ16	(OPCODE_BNZ))

#define ag_rp_325() (gAsm->opcode_arg_equ16	(OPCODE_BC))

#define ag_rp_326() (gAsm->opcode_arg_equ16	(OPCODE_BNC))

#define ag_rp_327() (gAsm->opcode_arg_equ16	(OPCODE_BM))

#define ag_rp_328() (gAsm->opcode_arg_equ16	(OPCODE_BP))

#define ag_rp_329() (gAsm->opcode_arg_equ16	(OPCODE_BPE))

#define ag_rp_330() (gAsm->opcode_arg_equ16	(OPCODE_RCALL))

#define ag_rp_331() (gAsm->opcode_arg_equ8		(OPCODE_SBZ))

#define ag_rp_332() (gAsm->opcode_arg_equ8		(OPCODE_SBNZ))

#define ag_rp_333() (gAsm->opcode_arg_equ8		(OPCODE_SBC))

#define ag_rp_334() (gAsm->opcode_arg_equ8		(OPCODE_SBNC))

#define ag_rp_335() (gAsm->opcode_arg_equ24	(OPCODE_LCALL))

#define ag_rp_336() (gAsm->opcode_arg_equ24	(OPCODE_LJMP))


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
    0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  1,  2,  3,  0,  0,  4,  5,
    6,  7,  8,  9,  0,  0,  0, 10, 11, 12, 13, 14, 15, 16,  0,  0, 17, 18,
   19, 20, 21, 22, 23, 24, 25, 26, 27, 28, 29, 30, 31, 32, 33, 34, 35, 36,
    0, 37,  0, 38, 39, 40, 41, 42,  0,  0,  0, 43, 44,  0,  0, 45,  0,  0,
    0, 46,  0,  0, 47, 48, 49, 50, 51, 52, 53, 54, 55, 56, 57, 58, 59, 60,
   61, 62, 63, 64, 65, 66, 67, 68, 69, 70, 71, 72, 73,  0, 74, 75, 76, 77,
   78, 79, 80, 81, 82, 83, 84,  0,  0, 85, 86, 87, 88, 89, 90, 91,  0, 92,
   93,  0, 94, 95,  0, 96, 97, 98, 99,100,101,102,103,104,105,  0,  0,106,
  107,108,109,110,111,  0,112,113,114,115,116,117,118,119,120,  0,  0,  0,
  121,  0,  0,122,  0,  0,123,  0,  0,124,  0,  0,125,  0,  0,126,  0,  0,
  127,128,  0,129,130,  0,131,132,  0,133,134,135,136,  0,137,138,  0,139,
  140,141,142,143,  0,144,145,146,147,148,149,  0,  0,150,151,152,153,154,
  155,156,157,158,159,160,161,162,163,164,165,166,167,168,169,170,  0,  0,
  171,172,173,174,175,176,  0,  0,177,178,179,180,181,182,183,184,185,186,
  187,188,189,190,191,192,193,194,195,196,  0,197,198,199,200,201,202,203,
  204,205,  0,  0,206,207,  0,  0,208,  0,  0,209,  0,  0,210,211,212,213,
  214,215,216,217,218,219,220,221,222,223,224,  0,  0,  0,  0,  0,  0,  0,
    0,  0,  0,  0,  0,  0,  0,  0,  0,225,226,227,228,229,  0,  0,230,231,
  232,  0,  0,  0,233,234,235,236,237,238,239,240,241,  0,  0,242,243,244,
  245,246,247,248,249,250,251,  0,  0,  0,252,253,254,255,256,257,258,259,
  260,261,262,263,264,265,266,267,268,269,270,271,272,273,274,275,276,277,
  278,279,280,  0,281,  0,282,  0,283,284,285,286,287,288,289,290,291,292,
  293,294,295,296,297,298,299,  0,  0,  0,  0,  0,300,301,302,303,  0,  0,
    0,  0,  0,304,305,306,307,308,309,310,  0,  0,  0,311,  0,  0,  0,312,
  313,314,315,316,317,318,319,320,321,322,323,324,325,326,327,328,329,330,
  331,332,333,334,335,336
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
  1,430,  1,431,  1,433,  1,434,  1,440,  1,439,  1,441,  1,443,
  1,444,  1,445,  1,446,  1,447,  1,448,  1,449,  1,450,  1,451,
  1,452,  1,453,  1,454,  1,456,  1,457,  1,458,  1,462,  1,463,
  1,464,  1,467,  1,468,  1,469,  1,470,  1,471,  1,472,  1,473,
  1,474,  1,475,  1,476,  1,477,  1,478,  1,479,  1,480,  1,481,
  1,482,  1,483,  1,484,  1,486,  1,487,  1,488,  1,489,  1,490,
  1,491,  1,134,  1,494,  1,495,  1,497,  1,499,  1,501,  1,503,
  1,505,  1,508,  1,510,  1,512,  1,514,  1,516,  1,522,  1,525,
  1,527,  1,528,  1,529,  1,530,  1,531,  1,532,  1,533,  1,534,
  1,535,  1,537,  1,221,  1,222,  1,234,  1,235,  1,236,  1,237,
  1,238,  1,239,  1,240,  1,540,  1,668,  1,246,  1,248,  1,543,
  1,539,  1,541,  1,542,  1,544,  1,545,  1,546,  1,547,  1,548,
  1,549,  1,550,  1,551,  1,552,  1,553,  1,554,  1,555,  1,556,
  1,557,  1,558,  1,559,  1,560,  1,561,  1,562,  1,563,  1,564,
  1,565,  1,566,  1,567,  1,568,  1,569,  1,570,  1,571,  1,572,
  1,573,  1,574,  1,575,  1,576,  1,577,  1,578,  1,579,  1,580,
  1,581,  1,582,  1,583,  1,584,  1,585,  1,586,  1,587,  1,588,
  1,589,  1,590,  1,591,  1,592,  1,593,  1,595,  1,596,  1,597,
  1,598,  1,599,  1,600,  1,601,  1,602,  1,603,  1,604,  1,605,
  1,606,  1,607,  1,609,  1,610,  1,611,  1,612,  1,613,  1,614,
  1,615,  1,616,  1,617,  1,618,  1,619,  1,620,  1,621,  1,622,
  1,623,  1,624,  1,625,  1,626,  1,627,  1,628,  1,629,  1,630,
  1,631,  1,632,  1,633,  1,634,  1,635,  1,636,  1,637,  1,638,
  1,639,  1,640,  1,641,  1,642,  1,643,  1,644,  1,645,  1,646,
  1,647,  1,648,  1,649,  1,650,  1,651,  1,652,  1,653,  1,654,
  1,655,  1,656,  1,657,  1,658,  1,659,  1,660,  1,661,  1,662,
  1,663,  1,664,  1,665,  1,666,  1,667,  1,669,  1,670,  1,671,
  1,672,  1,673,  1,674,  1,675,  1,676,  1,677,  1,678,  1,679,
0
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
   69, 79,255, 65, 71, 73,255, 82, 85,255, 72,255, 70,255, 84,255, 78,255,
   65, 73,255, 66, 83,255, 65, 67, 79, 82, 83, 85,255, 76, 82,255, 65,255,
   67, 68,255, 67, 90,255, 69, 71, 79,255, 67, 72,255, 86,255, 84,255, 65,
   67, 68, 69, 73, 76, 77, 78, 80, 82, 83, 90,255, 67, 90,255, 66, 67, 73,
   78, 90,255, 69,255, 68, 73, 82, 88,255, 76, 82,255, 71, 72, 73,255, 88,
  255, 65, 67, 75,255, 66, 73,255, 66, 69, 72, 73, 80, 81, 84, 85, 89,255,
   69, 73,255, 65, 73,255, 67, 79, 82, 84,255, 33, 35, 36, 38, 39, 40, 42,
   44, 46, 47, 60, 61, 62, 65, 66, 67, 68, 69, 70, 71, 72, 73, 74, 76, 77,
   78, 79, 80, 82, 83, 84, 85, 86, 87, 88, 92,255, 66, 68, 84,255, 42, 47,
  255, 35, 36, 38, 46, 47,255, 73, 83,255, 67, 76, 78, 82,255, 68, 78,255,
   70, 78,255, 70,255, 84,255, 78,255, 65, 73,255, 82,255, 68, 69, 73, 80,
   85,255, 42, 47,255, 38, 42, 47, 58, 61,255, 61,255, 42, 47,255, 67, 68,
   73,255, 65, 73,255, 69, 72,255, 67, 68, 78, 82, 83,255, 67, 90,255, 69,
  255, 65,255, 67, 76, 77, 78, 80, 82, 89, 90,255, 65, 67, 80,255, 67, 90,
  255, 69, 73, 79,255, 65, 67, 77, 78, 80, 83, 90,255, 65, 68,255, 82, 88,
  255, 72, 83,255, 69, 85,255, 65, 66, 67, 69, 73, 83, 87,255, 73,255, 68,
   84,255, 78, 82,255, 69, 82,255, 84,255, 67, 73, 76, 78, 81, 88,255, 77,
   84,255, 69, 76,255, 82, 88,255, 70, 78,255, 80,255, 53,255, 67, 68, 75,
   88, 90,255, 69, 79,255, 77, 80,255, 53,255, 67, 68, 75, 77, 78, 80, 84,
   88, 90,255, 88,255, 72, 83,255, 65, 69, 72, 83,255, 69,255, 68, 73, 88,
  255, 76,255, 78, 83,255, 79, 85,255, 67, 68, 72, 73, 74, 80, 82, 83, 88,
  255, 68, 86,255, 65, 79, 83, 86,255, 65,255, 80,255, 65, 79,255, 65, 71,
   73,255, 82, 85,255, 70,255, 84,255, 78,255, 73,255, 66, 83,255, 65, 67,
   79, 82, 85,255, 76, 82,255, 65,255, 67, 68,255, 67, 90,255, 69, 71, 79,
  255, 67, 72,255, 86,255, 84,255, 65, 67, 68, 69, 73, 76, 77, 78, 80, 82,
   83, 90,255, 67, 90,255, 66, 67, 73, 78, 90,255, 69,255, 68, 73, 82, 88,
  255, 76,255, 71, 72, 73,255, 88,255, 65, 67, 75,255, 66, 73,255, 66, 69,
   72, 73, 80, 84, 85, 89,255, 69, 73,255, 65, 73,255, 67, 82, 84,255, 36,
   38, 42, 47, 61, 65, 66, 67, 68, 69, 70, 72, 73, 74, 76, 77, 78, 79, 80,
   82, 83, 84, 86, 87, 88,255, 42, 47,255, 85,255, 76,255, 67,255, 78,255,
   47, 73,255, 61,255, 42, 47,255, 67, 68, 73,255, 65, 73,255, 69, 72,255,
   67, 68, 78, 82, 83,255, 67, 90,255, 69,255, 65,255, 67, 76, 77, 78, 80,
   82, 89, 90,255, 65, 67, 80,255, 67, 90,255, 69, 73, 79,255, 65, 67, 77,
   78, 80, 83, 90,255, 65, 68,255, 82, 88,255, 72, 83,255, 69, 85,255, 65,
   66, 67, 69, 73, 83, 87,255, 73,255, 68, 84,255, 78, 82,255, 69, 82,255,
   84,255, 67, 73, 76, 78, 88,255, 77, 84,255, 69, 76,255, 85,255, 76,255,
   67, 82, 88,255, 70, 78,255, 80,255, 53,255, 67, 68, 75, 88, 90,255, 69,
   79,255, 77, 80,255, 53,255, 67, 68, 75, 77, 78, 80, 84, 88, 90,255, 88,
  255, 72, 83,255, 65, 69, 72, 83,255, 69,255, 68, 73, 88,255, 76,255, 78,
   83,255, 79, 85,255, 67, 68, 72, 73, 74, 80, 82, 83, 88,255, 68, 86,255,
   65, 79, 83, 86,255, 65,255, 80,255, 65, 79,255, 65, 71, 73,255, 82, 85,
  255, 70,255, 84,255, 78,255, 73,255, 66, 83,255, 65, 67, 79, 82, 85,255,
   76, 82,255, 65,255, 67, 68,255, 67, 90,255, 69, 71, 79,255, 67, 72,255,
   86,255, 84,255, 65, 67, 68, 69, 73, 76, 77, 78, 80, 82, 83, 90,255, 67,
   90,255, 66, 67, 73, 78, 90,255, 69,255, 68, 73, 82, 88,255, 76,255, 71,
   72, 73,255, 88,255, 65, 67, 75,255, 66, 73,255, 66, 72, 73, 80, 84, 85,
   89,255, 69, 73,255, 65, 73,255, 67, 82, 84,255, 36, 38, 42, 47, 65, 66,
   67, 68, 69, 70, 72, 73, 74, 76, 77, 78, 79, 80, 82, 83, 84, 86, 87, 88,
  255, 36, 38,255, 76, 80,255, 71, 87,255, 78, 79,255, 36, 38, 39, 67, 68,
   70, 72, 73, 76, 78, 80, 83,255, 42, 47,255, 47,255, 78, 88,255, 69, 72,
   76, 86,255, 66, 68, 84,255, 42, 47,255, 85,255, 76,255, 67,255, 78,255,
   35, 36, 38, 42, 46, 47, 73,255, 47, 61,255, 42, 47,255, 76, 89,255, 69,
  255, 66, 83, 87,255, 68, 84,255, 78, 82,255, 69, 82,255, 84,255, 67, 78,
   81, 88,255, 85,255, 76,255, 67,255, 78,255, 78, 83,255, 73, 83,255, 65,
   79, 83,255, 65, 79,255, 70,255, 84,255, 78,255, 73,255, 65, 82, 85,255,
   69, 84, 89,255, 69, 73,255, 36, 42, 47, 65, 66, 67, 68, 69, 70, 72, 73,
   76, 77, 78, 79, 80, 83, 84, 86, 87, 92,255, 85,255, 76,255, 67,255, 78,
  255, 73,255, 42, 47,255, 42, 47,255, 42, 47, 92,255, 42, 47,255, 47, 73,
   80,255, 42, 47,255, 33, 47,255, 67,255, 69, 88,255, 76,255, 66, 68, 72,
   80, 83,255, 40, 65, 66, 67, 68, 69, 72, 76, 77,255, 67,255, 69,255, 76,
  255, 66, 68, 72, 83,255, 67,255, 69,255, 76,255, 65, 66, 68, 72, 80,255,
   67,255, 69,255, 66, 68,255, 61,255, 42, 47,255, 60, 61,255, 61,255, 61,
   62,255, 33, 42, 44, 47, 60, 61, 62,255, 61,255, 42, 47,255, 60, 61,255,
   61,255, 61, 62,255, 69, 84,255, 69, 84,255, 76, 82,255, 72,255, 33, 42,
   44, 47, 60, 61, 62, 65, 69, 71, 76, 77, 78, 79, 83, 88,255, 76, 89,255,
   69,255, 66, 83, 87,255, 68, 84,255, 78, 82,255, 69, 82,255, 84,255, 67,
   78, 88,255, 78, 83,255, 73, 83,255, 65, 79, 83,255, 65, 79,255, 70,255,
   84,255, 78,255, 73,255, 65, 82, 85,255, 84, 89,255, 69, 73,255, 36, 42,
   65, 66, 67, 68, 69, 70, 72, 76, 77, 78, 79, 80, 83, 84, 86, 87,255, 42,
   47,255, 44, 47,255, 61,255, 42, 47,255, 60, 61,255, 61,255, 61, 62,255,
   69, 84,255, 69, 84,255, 82,255, 76, 82,255, 72,255, 33, 42, 44, 47, 60,
   61, 62, 65, 69, 71, 76, 77, 78, 79, 81, 83, 88,255, 39,255, 61,255, 42,
   47,255, 60, 61,255, 61,255, 61, 62,255, 69, 84,255, 69, 84,255, 82,255,
   76, 82,255, 72,255, 33, 42, 44, 47, 60, 61, 62, 71, 76, 77, 78, 79, 81,
   83, 88,255, 42, 47,255, 76, 80,255, 71, 87,255, 78, 79,255, 36, 38, 39,
   42, 47, 67, 68, 70, 72, 73, 76, 78, 80, 83, 92,255, 76, 80,255, 71, 87,
  255, 78, 79,255, 36, 38, 39, 67, 68, 70, 72, 73, 76, 80, 83,255, 42, 47,
  255, 76, 80,255, 71, 87,255, 78, 79,255, 36, 38, 39, 42, 47, 67, 68, 70,
   72, 73, 76, 80, 83, 92,255, 61,255, 60, 61,255, 61,255, 61, 62,255, 69,
   84,255, 69, 84,255, 76, 82,255, 72,255, 33, 42, 44, 60, 61, 62, 65, 69,
   71, 76, 77, 78, 79, 83, 88,255, 61,255, 42, 47,255, 60, 61,255, 61,255,
   61, 62,255, 69, 84,255, 69, 84,255, 76, 82,255, 72,255, 33, 44, 47, 60,
   61, 62, 65, 69, 71, 76, 78, 79, 83, 88,255, 61,255, 42, 47,255, 61,255,
   61,255, 61,255, 69, 84,255, 69, 84,255, 33, 44, 47, 60, 61, 62, 65, 69,
   71, 76, 78, 79, 88,255, 61,255, 42, 47,255, 61,255, 61,255, 61,255, 69,
   84,255, 69, 84,255, 33, 44, 47, 60, 61, 62, 69, 71, 76, 78, 79, 88,255,
   61,255, 42, 47,255, 61,255, 61,255, 61,255, 69, 84,255, 69, 84,255, 33,
   44, 47, 60, 61, 62, 69, 71, 76, 78, 79,255, 42, 47,255, 61,255, 61,255,
   61,255, 69, 84,255, 69, 84,255, 33, 47, 60, 61, 62, 69, 71, 76, 78,255,
   61,255, 42, 47,255, 42, 47,255, 60, 61,255, 61,255, 61, 62,255, 69, 84,
  255, 69, 84,255, 76, 82,255, 72,255, 33, 42, 44, 47, 60, 61, 62, 65, 69,
   71, 76, 77, 78, 79, 83, 88, 92,255, 76, 89,255, 69,255, 66, 83, 87,255,
   68, 84,255, 78, 82,255, 69, 82,255, 84,255, 67, 78, 81, 88,255, 78, 83,
  255, 73, 83,255, 65, 79, 83,255, 65, 79,255, 70,255, 84,255, 78,255, 73,
  255, 65, 82, 85,255, 69, 84, 89,255, 69, 73,255, 36, 42, 65, 66, 67, 68,
   69, 70, 72, 76, 77, 78, 79, 80, 83, 84, 86, 87,255, 39,255, 67, 68, 73,
  255, 65, 73,255, 67, 68, 78, 82, 83,255, 67, 90,255, 69,255, 65,255, 67,
   77, 78, 80, 82, 90,255, 65, 67, 80,255, 67, 90,255, 69, 73, 79,255, 65,
   67, 77, 78, 80, 90,255, 65, 68,255, 82, 88,255, 72, 83,255, 65, 67, 69,
   73, 83,255, 77, 84,255, 76,255, 82, 88,255, 78,255, 80,255, 53,255, 67,
   68, 75, 88, 90,255, 69, 79,255, 77, 80,255, 53,255, 67, 68, 75, 77, 78,
   80, 84, 88, 90,255, 88,255, 72, 83,255, 65, 69, 72, 83,255, 69,255, 68,
   73, 88,255, 76,255, 79, 85,255, 67, 68, 72, 74, 80, 82, 88,255, 79, 86,
  255, 65, 73,255, 82, 85,255, 67, 79, 85,255, 76, 82,255, 65,255, 67, 68,
  255, 67, 90,255, 69, 71, 79,255, 67, 72,255, 86,255, 84,255, 65, 67, 68,
   69, 73, 76, 77, 78, 80, 82, 83, 90,255, 67, 90,255, 66, 67, 73, 78, 90,
  255, 69,255, 68, 73, 82, 88,255, 76,255, 71, 72, 73,255, 88,255, 65, 67,
  255, 66, 73,255, 66, 72, 73, 80, 84, 85,255, 65, 73,255, 67, 82, 84,255,
   65, 66, 67, 68, 69, 72, 73, 74, 76, 77, 78, 79, 80, 82, 83, 88,255, 42,
   47,255, 36, 38, 47,255, 42, 47,255, 33, 44, 47,255, 44,255, 42, 47,255,
   47, 79, 81,255, 92,255, 69,255, 69,255, 76, 80,255, 73,255, 71, 87,255,
   78, 79,255, 36, 38, 39, 40, 65, 66, 67, 68, 69, 70, 72, 73, 76, 77, 78,
   80, 83,255, 33,255, 42, 47,255, 47, 92,255
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
  7,4,5,4,6,4,2,4,7,2,4,7,7,4,7,6,7,2,7,2,4,5,5,4,7,4,5,7,4,5,5,4,5,5,5,
  4,5,7,4,5,4,6,4,2,6,7,7,7,2,5,2,6,2,2,5,4,5,5,4,5,5,5,2,5,4,5,4,6,5,5,
  5,4,6,5,4,5,7,5,4,5,4,6,5,7,4,5,5,4,2,7,2,7,6,7,2,2,7,4,7,7,4,5,5,4,7,
  7,2,7,4,6,0,6,0,3,3,2,0,2,2,1,1,1,6,6,6,6,6,2,2,6,2,2,6,6,2,2,2,2,2,2,
  7,7,7,2,3,4,3,3,3,4,0,0,4,0,5,0,2,2,4,7,7,4,7,2,7,7,4,7,7,4,6,7,4,5,4,
  6,4,2,4,7,2,4,2,4,7,2,2,2,7,4,0,0,4,0,3,2,0,0,4,0,4,0,0,4,5,5,5,4,5,5,
  4,7,7,4,7,2,2,7,2,4,5,5,4,5,4,5,4,5,7,5,2,6,6,7,5,4,5,5,5,4,5,5,4,5,5,
  5,4,7,5,6,2,6,7,5,4,5,5,4,5,5,4,7,7,4,7,7,4,2,5,2,2,5,6,5,4,7,4,6,7,4,
  7,7,4,2,7,4,2,4,7,5,7,2,7,2,4,7,5,4,7,2,4,5,5,4,5,6,4,5,4,5,4,5,5,5,6,
  5,4,5,5,4,5,5,4,5,4,5,5,5,6,2,6,2,6,5,4,5,4,5,5,4,6,2,7,7,4,5,4,6,5,5,
  4,2,4,7,7,4,7,7,4,7,2,2,2,7,2,7,7,7,4,7,5,4,7,2,7,7,4,7,4,6,4,7,2,4,5,
  5,5,4,2,7,4,5,4,6,4,2,4,2,4,7,7,4,7,7,7,2,2,4,5,5,4,7,4,5,7,4,5,5,4,5,
  5,5,4,5,7,4,5,4,6,4,2,6,7,7,7,2,5,2,6,2,2,5,4,5,5,4,5,5,5,2,5,4,5,4,6,
  5,5,5,4,2,4,5,7,5,4,5,4,6,5,7,4,5,5,4,2,7,2,7,2,2,2,7,4,7,7,4,5,5,4,7,
  2,7,4,6,0,3,2,0,2,2,2,2,2,7,2,2,2,2,2,2,2,2,2,2,2,7,7,2,4,0,0,4,7,4,6,
  4,2,4,2,4,2,2,4,0,4,0,0,4,5,5,5,4,5,5,4,7,7,4,7,2,2,7,2,4,5,5,4,5,4,5,
  4,5,7,5,2,6,6,7,5,4,5,5,5,4,5,5,4,5,5,5,4,7,5,6,2,6,7,5,4,5,5,4,5,5,4,
  7,7,4,7,7,4,2,5,2,2,5,6,5,4,7,4,6,7,4,7,7,4,2,7,4,2,4,7,5,7,2,2,4,7,5,
  4,7,2,4,7,4,6,4,2,5,5,4,5,6,4,5,4,5,4,5,5,5,6,5,4,5,5,4,5,5,4,5,4,5,5,
  5,6,2,6,2,6,5,4,5,4,5,5,4,6,2,7,7,4,5,4,6,5,5,4,2,4,7,7,4,7,7,4,7,2,2,
  2,7,2,7,7,7,4,7,5,4,7,2,7,7,4,7,4,6,4,7,2,4,5,5,5,4,2,7,4,5,4,6,4,2,4,
  2,4,7,7,4,7,7,7,2,2,4,5,5,4,7,4,5,7,4,5,5,4,5,5,5,4,5,7,4,5,4,6,4,2,6,
  7,7,7,2,5,2,6,2,2,5,4,5,5,4,5,5,5,2,5,4,5,4,6,5,5,5,4,2,4,5,7,5,4,5,4,
  6,5,7,4,5,5,4,2,2,7,2,2,2,7,4,7,7,4,5,5,4,7,2,7,4,6,0,3,2,2,2,2,2,2,7,
  2,2,2,2,2,2,2,2,2,2,2,7,7,2,4,5,0,4,7,5,4,5,5,4,5,2,4,5,0,3,7,7,2,7,7,
  2,7,7,7,4,0,0,4,2,4,7,7,4,2,7,7,7,4,3,3,3,4,0,0,4,7,4,6,4,2,4,2,4,0,5,
  0,3,2,2,2,4,0,0,4,0,0,4,7,7,4,7,4,5,6,5,4,5,7,4,7,7,4,2,7,4,2,4,7,2,7,
  2,4,7,4,6,4,2,4,2,4,7,7,4,2,7,4,7,7,7,4,7,7,4,5,4,6,4,2,4,2,4,7,2,7,4,
  7,7,7,4,7,7,4,3,2,2,7,2,7,2,2,7,7,2,2,2,2,7,2,2,2,7,7,3,4,7,4,6,4,2,4,
  2,4,2,4,3,3,4,0,0,4,3,2,3,4,0,0,4,2,7,7,4,0,0,4,5,2,4,5,4,5,5,4,5,4,6,
  6,6,7,7,4,3,5,5,5,5,5,5,5,5,4,5,4,5,4,5,4,6,6,6,7,4,5,4,5,4,5,4,5,6,6,
  6,7,4,5,4,5,4,6,6,4,0,4,0,0,4,0,0,4,0,4,0,0,4,6,3,0,2,1,1,1,4,0,4,0,0,
  4,0,0,4,0,4,0,0,4,5,5,4,5,5,4,5,5,4,2,4,6,3,0,2,1,1,1,7,7,2,2,7,7,7,2,
  7,4,7,7,4,7,4,5,6,5,4,5,7,4,7,7,4,2,7,4,2,4,7,2,2,4,7,7,4,2,7,4,7,7,7,
  4,7,7,4,5,4,6,4,2,4,2,4,7,2,7,4,7,7,4,7,7,4,3,3,7,2,7,2,2,7,7,2,2,2,7,
  2,2,2,7,7,4,0,0,4,0,2,4,0,4,0,0,4,0,0,4,0,4,0,0,4,5,5,4,5,5,4,5,4,5,5,
  4,2,4,6,3,0,2,1,1,1,7,7,2,2,7,7,6,5,2,7,4,3,4,0,4,0,0,4,0,0,4,0,4,0,0,
  4,5,5,4,5,5,4,5,4,5,5,4,2,4,6,3,0,2,1,1,1,2,2,7,7,6,5,2,7,4,0,0,4,7,5,
  4,5,5,4,5,2,4,5,0,3,3,2,7,7,2,7,7,2,7,7,7,3,4,7,5,4,5,5,4,5,2,4,5,0,3,
  7,7,2,7,7,2,7,7,4,0,0,4,7,5,4,5,5,4,5,2,4,5,0,3,3,2,7,7,2,7,7,2,7,7,3,
  4,0,4,0,0,4,0,4,0,0,4,5,5,4,5,5,4,5,5,4,2,4,6,3,0,1,1,1,7,7,2,2,7,7,7,
  2,7,4,0,4,0,0,4,0,0,4,0,4,0,0,4,5,5,4,5,5,4,5,5,4,2,4,6,0,2,1,1,1,7,7,
  2,2,7,7,2,7,4,0,4,0,0,4,0,4,0,4,0,4,5,5,4,5,5,4,6,0,2,1,1,1,7,7,2,2,7,
  7,7,4,0,4,0,0,4,0,4,0,4,0,4,5,5,4,5,5,4,6,0,2,1,1,1,7,2,2,7,7,7,4,0,4,
  0,0,4,0,4,0,4,0,4,5,5,4,5,5,4,6,0,2,1,1,1,7,2,2,7,7,4,0,0,4,0,4,0,4,0,
  4,5,5,4,5,5,4,3,2,1,1,1,7,2,2,7,4,0,4,0,0,4,0,0,4,0,0,4,0,4,0,0,4,5,5,
  4,5,5,4,5,5,4,2,4,6,2,0,2,1,1,1,7,7,2,2,7,7,7,2,7,3,4,7,7,4,7,4,5,6,5,
  4,5,7,4,7,7,4,2,7,4,2,4,7,2,7,2,4,7,7,4,2,7,4,7,7,7,4,7,7,4,5,4,6,4,2,
  4,2,4,7,2,7,4,7,7,7,4,7,7,4,3,3,7,2,7,2,2,7,7,2,2,2,7,2,2,2,7,7,4,3,4,
  5,5,5,4,5,5,4,7,2,2,7,7,4,5,5,4,5,4,5,4,5,5,2,6,6,5,4,5,5,5,4,5,5,4,5,
  5,5,4,7,5,6,2,6,5,4,5,5,4,5,5,4,7,7,4,2,2,2,5,7,4,7,5,4,2,4,5,5,4,6,4,
  5,4,5,4,5,5,5,6,5,4,5,5,4,5,5,4,5,4,5,5,5,6,2,6,2,6,5,4,5,4,5,5,4,6,2,
  7,7,4,5,4,6,5,5,4,2,4,7,7,4,7,2,2,7,2,7,7,4,7,7,4,5,5,4,2,7,4,7,7,7,4,
  5,5,4,7,4,5,7,4,5,5,4,5,5,5,4,5,7,4,5,4,6,4,2,6,7,7,7,2,5,2,6,2,2,5,4,
  5,5,4,5,5,5,2,5,4,5,4,6,5,5,5,4,2,4,5,7,5,4,5,4,6,5,4,5,5,4,2,2,7,2,2,
  2,4,5,5,4,7,2,7,4,2,2,2,2,7,2,2,2,2,2,7,2,2,2,2,2,4,0,0,4,5,0,2,4,0,0,
  4,5,0,2,4,0,4,0,0,4,2,5,5,4,3,4,7,4,7,4,7,5,4,7,4,5,5,4,5,2,4,5,0,3,3,
  5,5,6,6,5,2,6,7,6,5,7,7,7,4,5,4,0,0,4,2,3,4
};

static const unsigned short ag_key_parm[] = {
    0,162,164,163,  0,427,424,  0,  4,  0,  6,  0,  0,  0,  0,  0,438, 98,
  135,  0,  0,  0,  0,498,  0,465,  0,523,426,466,  0,162,164,163,  0,427,
  424,  0,513,502,  0,496,  0,500,515,  0,282,284,324,  0,286,118,326,  0,
   50,184,  0,322,  0,  0,188,  0,  0,426,424,  0,432,  0,420,  0,168, 54,
  428,  0,430,418, 58,422,  0,192,194,288,  0,334,336,  0,340,342,344,  0,
  328,330,130,332,  0,338, 44,346,  0,196,308,  0,290,310,  0,146,  0, 42,
    0,  0,  0,  0,  0,  0,392,398,  0, 46,200,  0,  0, 56,  0,170,198, 52,
   62,178,  0, 24, 28,  0, 30,  0, 80, 14,  0,  0,  0, 18, 70,  0,  0, 68,
    0,  0,  0, 34,204,  0,  0,102, 32,  0,  0, 78,128,140,  0,106,110,  0,
  202,206,  0, 12,142,172,  0, 20, 26,  0,  4,  0,  6,  0,  0,100,292,312,
    0, 22,348,138,  0,364,  0,370,  0,366,376,374,368,378,  0,382,384,  0,
  356,372,  0,354,  0,350,360,358,362,  0,380,  0,352,386,  0,272,  0,390,
  396,  0,388,  0,394,400,  0,212,  0,402,210,208,  0,  0,  0, 92, 10,  0,
  134,144,  0,278,280,  0,444,  0,108,  0,  0,446,132,  0,  0,268, 84,112,
  314,  0, 72,  0,124,294,  0, 94,  0, 82,316,  0, 88,  0,214,126,  0, 74,
  104,  0,  0,296, 48,404,  0,114,406,  0,216,  0, 38,  0, 36,  0,  0,  0,
    8,  0,  0, 66,276,  0, 96,180,274,  0,176,  0,  0,218,220,  0,434,  0,
  228,230,  0,236,238,  0,242,306,244,  0,246,190,  0,248,  0,320,  0,  0,
  222,232,224,226,  0,234,  0,240,  0,  0,250,  0,442,438,  0,298,440,408,
    0,436,  0,256,  0,410,254,186,252,  0,120,122,  0,304,260,318,  0,270,
    0,412,262, 76,  0,300,414,  0,  0,  2,  0,258,174,136,  0,  0, 90,  0,
   60, 86,  0,302,416,  0,264,116,  0,266,  0,182,438, 98,135,231,241,  0,
  493,  0,  0,506,432,504,166,152,154,156,158,  0,  0,160,  0,  0,162,164,
    0,  0,  0,  0,  0,  0, 40, 16, 64,  0,461,  0,162,164,163,  0,427,424,
    0,438, 98,135,  0,  0,  0, 24, 28,  0, 34,  0, 30, 32,  0, 20, 26,  0,
   22,  4,  0, 38,  0, 36,  0,  0,  0,  8,  0,  0,  0,  0, 42,  0,  0,  0,
   40,  0,427,424,  0,135,466,  0,161,432,  0,465,  0,427,424,  0,282,284,
  324,  0,286,326,  0, 50,184,  0,322,  0,  0,188,  0,  0,426,424,  0,432,
    0,420,  0,168, 54,428,  0,430,418, 58,422,  0,192,194,288,  0,334,336,
    0,340,342,344,  0,328,330,332,  0,338, 44,346,  0,196,308,  0,290,310,
    0,392,398,  0, 46,200,  0,  0, 56,  0,  0,198, 52, 62,  0, 30,  0, 80,
   14,  0, 18, 70,  0,  0, 68,  0,  0,  0, 34,204, 28,  0,  0,  0,  0,202,
  206,  0, 12,  0,  0,292,312,  0, 22,348,  0,364,  0,370,  0,366,376,374,
  368,378,  0,382,384,  0,356,372,  0,354,  0,350,360,358,362,  0,380,  0,
  352,386,  0,272,  0,390,396,  0,388,  0,394,400,  0,212,  0,402,210,208,
    0,  0,  0, 92, 10,  0,278,280,  0,444,  0,  0,  0,446,  0,268, 84,314,
    0, 72,294,  0, 94,  0, 82,316,  0, 88,  0,214,  0, 74,  0,  0,296, 48,
  404,  0,  0,406,  0, 38,  0, 36,  0,  0,  0,  0,  0, 66,276,  0, 96,216,
  274,  0,  0,  0,218,220,  0,434,  0,228,230,  0,236,238,  0,242,306,244,
    0,246,190,  0,248,  0,320,  0,  0,222,232,224,226,  0,234,  0,240,  0,
    0,250,  0,442,438,  0,298,440,408,  0,436,  0,256,  0,410,254,186,252,
    0,  0,  0,304,260,318,  0,270,  0,412,262, 76,  0,300,414,  0,  0,  2,
    0,258,  0,  0,  0, 90,  0, 60, 86,  0,302,416,  0,264,  0,266,  0, 98,
  135,466,  0,432,  0,  0,  0,  0,  0, 78,  0,  0,  0,  0,  0,  0,  0,  0,
    0,  0,  0, 16, 64,  0,  0,427,424,  0,  4,  0,  6,  0,  0,  0,  0,  0,
    0,  0,  0,465,  0,427,424,  0,282,284,324,  0,286,326,  0, 50,184,  0,
  322,  0,  0,188,  0,  0,426,424,  0,432,  0,420,  0,168, 54,428,  0,430,
  418, 58,422,  0,192,194,288,  0,334,336,  0,340,342,344,  0,328,330,332,
    0,338, 44,346,  0,196,308,  0,290,310,  0,392,398,  0, 46,200,  0,  0,
   56,  0,  0,198, 52, 62,  0, 30,  0, 80, 14,  0, 18, 70,  0,  0, 68,  0,
    0,  0, 34,204, 28,  0,  0,  0,202,206,  0, 12,  0,  0,  4,  0,  6,  0,
    0,292,312,  0, 22,348,  0,364,  0,370,  0,366,376,374,368,378,  0,382,
  384,  0,356,372,  0,354,  0,350,360,358,362,  0,380,  0,352,386,  0,272,
    0,390,396,  0,388,  0,394,400,  0,212,  0,402,210,208,  0,  0,  0, 92,
   10,  0,278,280,  0,444,  0,  0,  0,446,  0,268, 84,314,  0, 72,294,  0,
   94,  0, 82,316,  0, 88,  0,214,  0, 74,  0,  0,296, 48,404,  0,  0,406,
    0, 38,  0, 36,  0,  0,  0,  0,  0, 66,276,  0, 96,216,274,  0,  0,  0,
  218,220,  0,434,  0,228,230,  0,236,238,  0,242,306,244,  0,246,190,  0,
  248,  0,320,  0,  0,222,232,224,226,  0,234,  0,240,  0,  0,250,  0,442,
  438,  0,298,440,408,  0,436,  0,256,  0,410,254,186,252,  0,  0,  0,304,
  260,318,  0,270,  0,412,262, 76,  0,300,414,  0,  0,  0,258,  0,  0,  0,
   90,  0, 60, 86,  0,302,416,  0,264,  0,266,  0, 98,135,466,  0,  0,  0,
    0,  0,  0, 78,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0, 16, 64,  0,
    0, 98,135,  0,128,140,  0,134,144,  0,132,  0,  0, 98,135,231,130,146,
    0,142,138,  0,126, 96,136,  0,427,424,  0,  0,  0, 14, 18,  0,  0, 12,
   10, 16,  0,162,164,163,  0,427,424,  0,  4,  0,  6,  0,  0,  0,  0,  0,
  438, 98,135,426,  0,  0,  0,  0,426,466,  0,427,424,  0, 54, 58,  0, 46,
    0, 56, 52, 62,  0, 80, 14,  0, 18, 70,  0,  0, 68,  0,  0,  0, 34,  0,
    0,  0,  0,  4,  0,  6,  0,  0,  0,  0,  0, 92, 10,  0,  0, 84,  0, 94,
   72, 82,  0, 74, 88,  0, 38,  0, 36,  0,  0,  0,  0,  0, 96,  0, 66,  0,
    2, 76, 90,  0, 60, 86,  0,465,  0,  0, 50,  0, 44,  0,  0, 78, 12,  0,
    0,  0,  0, 48,  0,  0,  0, 16, 64,461,  0,  4,  0,  6,  0,  0,  0,  0,
    0,  0,  0,426,427,  0,427,424,  0,426,  0,461,  0,427,424,  0,  0,100,
   96,  0,427,424,  0,182,  0,  0,168,  0,170,178,  0,172,  0,152,156,160,
  180,174,  0,241,166,152,154,156,158,160,162,164,  0,168,  0,170,  0,172,
    0,152,156,160,174,  0,168,  0,170,  0,172,  0,166,152,156,160,176,  0,
  168,  0,170,  0,152,156,  0,498,  0,427,424,  0,513,502,  0,496,  0,500,
  515,  0,182,523,493,  0,506,432,504,  0,498,  0,427,424,  0,513,502,  0,
  496,  0,500,515,  0,106,110,  0,108,112,  0,120,122,  0,  0,  0,182,523,
  493,  0,506,432,504,118,102,  0,  0,124,104,114,  0,116,  0, 54, 58,  0,
   46,  0, 56, 52, 62,  0, 80, 14,  0, 18, 70,  0,  0, 68,  0,  0,  0, 34,
    0,  0,  0, 92, 10,  0,  0, 84,  0, 94, 72, 82,  0, 74, 88,  0, 38,  0,
   36,  0,  0,  0,  0,  0, 96,  0, 66,  0, 76, 90,  0, 60, 86,  0,465,466,
   50,  0, 44,  0,  0, 78, 12,  0,  0,  0, 48,  0,  0,  0, 16, 64,  0,427,
  424,  0,493,  0,  0,498,  0,427,424,  0,513,502,  0,496,  0,500,515,  0,
  106,110,  0,108,112,  0,114,  0,120,122,  0,  0,  0,182,523,493,  0,506,
  432,504,118,102,  0,  0,124,104,150,148,  0,116,  0,233,  0,498,  0,427,
  424,  0,513,502,  0,496,  0,500,515,  0,106,110,  0,108,112,  0,114,  0,
  120,122,  0,  0,  0,182,523,493,  0,506,432,504,  0,  0,124,104,150,148,
    0,116,  0,427,424,  0,128,140,  0,134,144,  0,132,  0,  0, 98,135,231,
  426,  0,130,146,  0,142,138,  0,126, 96,136,461,  0,128,140,  0,134,144,
    0,132,  0,  0, 98,135,231,130,146,  0,142,138,  0, 96,136,  0,427,424,
    0,128,140,  0,134,144,  0,132,  0,  0, 98,135,231,426,  0,130,146,  0,
  142,138,  0, 96,136,461,  0,498,  0,513,502,  0,496,  0,500,515,  0,106,
  110,  0,108,112,  0,120,122,  0,  0,  0,182,523,493,506,432,504,118,102,
    0,  0,124,104,114,  0,116,  0,498,  0,427,424,  0,513,502,  0,496,  0,
  500,515,  0,106,110,  0,108,112,  0,120,122,  0,  0,  0,182,493,  0,506,
  432,504,118,102,  0,  0,104,114,  0,116,  0,498,  0,427,424,  0,502,  0,
  496,  0,500,  0,106,110,  0,108,112,  0,182,493,  0,506,432,504,118,102,
    0,  0,104,114,116,  0,498,  0,427,424,  0,502,  0,496,  0,500,  0,106,
  110,  0,108,112,  0,182,493,  0,506,432,504,102,  0,  0,104,114,116,  0,
  498,  0,427,424,  0,502,  0,496,  0,500,  0,106,110,  0,108,112,  0,182,
  493,  0,506,432,504,102,  0,  0,104,114,  0,427,424,  0,502,  0,496,  0,
  500,  0,106,110,  0,108,112,  0,498,  0,506,432,504,102,  0,  0,104,  0,
  498,  0,523,426,  0,427,424,  0,513,502,  0,496,  0,500,515,  0,106,110,
    0,108,112,  0,120,122,  0,  0,  0,182,  0,493,  0,506,432,504,118,102,
    0,  0,124,104,114,  0,116,461,  0, 54, 58,  0, 46,  0, 56, 52, 62,  0,
   80, 14,  0, 18, 70,  0,  0, 68,  0,  0,  0, 34,  0,  0,  0,  0, 92, 10,
    0,  0, 84,  0, 94, 72, 82,  0, 74, 88,  0, 38,  0, 36,  0,  0,  0,  0,
    0, 96,  0, 66,  0,  2, 76, 90,  0, 60, 86,  0,465,466, 50,  0, 44,  0,
    0, 78, 12,  0,  0,  0, 48,  0,  0,  0, 16, 64,  0,231,  0,282,284,324,
    0,286,326,  0,322,  0,  0,188,184,  0,426,424,  0,432,  0,420,  0,168,
  428,  0,430,418,422,  0,192,194,288,  0,334,336,  0,340,342,344,  0,328,
  330,332,  0,338,346,  0,196,308,  0,290,310,  0,392,398,  0,  0,  0,  0,
  198,200,  0,202,206,  0,  0,  0,292,312,  0,348,  0,364,  0,370,  0,366,
  376,374,368,378,  0,382,384,  0,356,372,  0,354,  0,350,360,358,362,  0,
  380,  0,352,386,  0,272,  0,390,396,  0,388,  0,394,400,  0,212,  0,402,
  210,208,  0,  0,  0,278,280,  0,444,  0,  0,446,  0,268,314,  0,294,316,
    0,296,404,  0,  0,406,  0,216,274,276,  0,218,220,  0,434,  0,228,230,
    0,236,238,  0,242,306,244,  0,246,190,  0,248,  0,320,  0,  0,222,232,
  224,226,  0,234,  0,240,  0,  0,250,  0,442,438,  0,298,440,408,  0,436,
    0,256,  0,410,254,186,252,  0,  0,  0,304,260,318,  0,270,  0,412,262,
    0,300,414,  0,  0,  0,258,  0,  0,  0,  0,302,416,  0,264,  0,266,  0,
    0,  0,  0,  0,204,  0,  0,  0,  0,  0,214,  0,  0,  0,  0,  0,  0,427,
  424,  0, 98,135,  0,  0,427,424,  0,182,493,  0,  0,493,  0,427,424,  0,
    0,150,148,  0,461,  0,130,  0,146,  0,128,140,  0,142,  0,134,144,  0,
  132,  0,  0, 98,135,231,241,166,152,154,156,158,  0,160,138,162,164,126,
   96,136,  0,182,  0,427,424,  0,  0,461,  0
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
    0,266,  0,  0,  0,  0,  0,273,171,  0,176,  0,  0,  0,282,  0,284,  0,
  180,286,  0,186,190,  0,173,280,178,288,184,291,  0,  0,  0,  0,192,  0,
    0,202,  0,  0,  0,  0,  0,  0,  0,  0,  0,204,  0,  0,  0,319,  0,301,
  304,195,198,200,306,  0,309,312,316,321,  0,  0,  0,  0,  0,  0,  0,  0,
  336,  0,  0,  0,  0,345,  0,  0,  0,  0,347,  0,  0,  0,210,  0,  0,  0,
    0,359,  0,215,  0,  0,  0,  0,339,206,352,208,355,212,361,365,218,  0,
  220,223,  0,  0,  0,  0,243,246,381,248,  0, 23,  0, 25,  0, 14, 17, 27,
    0, 31, 35, 38, 41, 43, 57, 70, 90,120,147,155,159,165,181,203,240,259,
  269,277,294,323,368,378,227,232,239,384,251,  0,253,256,260,  0,  0,  0,
    0,  0,  0,  0,426,430,  0,273,275,  0,270,439,277,281,  0,285,288,  0,
  447,292,  0,  0,  0,453,  0,455,  0,298,457,  0,459,  0,264,442,450,462,
  302,  0,  0,  0,  0,  0,307,470,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,
    0,  0,  0,  0,  0,316,318,  0,311,484,488,313,491,  0,  0,  0,  0,  0,
    0,  0,  0,  0,320,  0,500,503,505,324,  0,  0,  0,  0,  0,  0,  0,  0,
    0,  0,  0,  0,  0,327,  0,516,520,523,330,  0,  0,  0,  0,  0,  0,  0,
    0,333,335,  0,337,339,  0,535,  0,538,541,  0,544,  0,  0,347,  0,555,
  349,  0,354,358,  0,560,360,  0,563,  0,341,  0,344,557,352,566,  0,368,
    0,  0,366,575,  0,  0,  0,  0,  0,581,  0,  0,  0,  0,  0,  0,  0,  0,
  589,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,587,591,597,600,
  603,  0,  0,  0,  0,  0,  0,  0,615,617,375,377,  0,  0,  0,625,  0,  0,
    0,627,  0,379,381,  0,386,388,  0,371,620,631,633,383,636,391,394,400,
    0,407,  0,  0,402,649,411,417,  0,422,  0,657,  0,419,659,  0,  0,  0,
    0,  0,664,425,  0,  0,  0,671,  0,673,  0,675,  0,435,439,  0,427,430,
  433,677,679,  0,  0,  0,  0,441,  0,  0,451,  0,  0,  0,  0,  0,  0,  0,
    0,  0,453,  0,  0,  0,706,  0,688,691,444,447,449,693,  0,696,699,703,
  708,  0,  0,  0,  0,  0,  0,  0,  0,723,  0,  0,  0,  0,732,  0,  0,  0,
    0,734,  0,  0,459,  0,  0,  0,  0,745,  0,461,  0,  0,  0,  0,726,455,
  739,457,741,747,751,464,  0,466,469,  0,  0,  0,  0,484,766,487,  0,479,
    0,309,481,  0,494,507,527,547,568,362,578,584,605,639,652,661,668,682,
  710,754,763,473,480,769,  0,  0,  0,  0,490,  0,802,  0,804,  0,806,  0,
  799,808,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,500,502,  0,
  495,818,822,497,825,  0,  0,  0,  0,  0,  0,  0,  0,  0,504,  0,834,837,
  839,508,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,511,  0,850,
  854,857,514,  0,  0,  0,  0,  0,  0,  0,  0,517,519,  0,521,523,  0,869,
    0,872,875,  0,878,  0,  0,531,  0,889,533,  0,536,540,  0,894,542,  0,
  897,  0,525,  0,528,891,900,  0,550,  0,  0,548,908,  0,553,  0,914,  0,
  916,  0,  0,  0,  0,918,  0,  0,  0,  0,  0,  0,  0,  0,927,  0,  0,  0,
    0,  0,  0,  0,  0,  0,  0,  0,  0,  0,925,929,935,938,941,  0,  0,  0,
    0,  0,  0,  0,953,955,560,562,  0,  0,  0,963,  0,  0,  0,965,  0,564,
  566,  0,571,573,  0,556,958,969,971,568,974,576,579,585,  0,592,  0,  0,
  587,987,596,602,  0,607,  0,995,  0,604,997,  0,  0,  0,  0,  0,1002,610,
    0,  0,  0,1009,  0,1011,  0,1013,  0,620,624,  0,612,615,618,1015,1017,
    0,  0,  0,  0,626,  0,  0,636,  0,  0,  0,  0,  0,  0,  0,  0,  0,638,
    0,  0,  0,1044,  0,1026,1029,629,632,634,1031,  0,1034,1037,1041,1046,
    0,  0,  0,  0,  0,  0,  0,  0,1061,  0,  0,  0,  0,1070,  0,  0,  0,
    0,1072,  0,  0,642,  0,  0,  0,  0,1083,  0,644,  0,  0,  0,  0,1064,
  1077,640,1079,1085,1089,647,  0,649,652,  0,  0,  0,  0,667,1103,670,  0,
  813,  0,493,815,828,841,861,881,902,544,911,922,943,977,990,999,1006,1020,
  1048,1092,1100,656,663,1106,  0,  0,  0,  0,687,  0,  0,  0,  0,  0,  0,
  1141,  0,  0,  0,673,676,680,1138,691,695,1144,697,700,704,  0,  0,  0,
    0,1160,  0,708,712,  0,1165,719,722,726,  0,735,738,742,  0,  0,  0,
    0,746,  0,1180,  0,1182,  0,1184,  0,  0,  0,  0,733,1173,1177,1186,
    0,  0,  0,  0,  0,  0,  0,755,759,  0,766,  0,  0,1205,  0,  0,  0,771,
    0,776,780,  0,1214,782,  0,1217,  0,768,1211,774,1220,  0,791,  0,1227,
    0,1229,  0,1231,  0,794,796,  0,1235,798,  0,804,809,814,  0,820,823,
    0,  0,  0,1248,  0,1250,  0,1252,  0,831,1254,834,  0,839,841,845,  0,
  847,850,  0,749,1196,1199,751,1202,762,1207,1222,784,788,1233,1238,1241,
  1245,828,1256,1260,1264,854,861,865,  0,867,  0,1289,  0,1291,  0,1293,
    0,1295,  0,870,872,  0,  0,  0,  0,874,1302,876,  0,  0,  0,  0,1309,
  878,884,  0,  0,  0,  0,  0,1316,  0,  0,  0,  0,  0,  0,  0,  0,1322,
  1324,1327,888,890,  0,892,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,
    0,  0,  0,  0,1345,1347,1349,896,  0,  0,  0,  0,  0,  0,  0,  0,1356,
  1358,1360,898,  0,  0,  0,  0,  0,1368,1370,  0,  0,  0,  0,  0,  0,  0,
    0,  0,  0,  0,  0,  0,  0,1375,901,  0,1377,1380,1383,1385,  0,  0,  0,
    0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,
    0,  0,1415,  0,1396,903,  0,1398,1401,1404,1406,905,908,1409,1412,910,
  913,915,1418,917,  0,928,932,  0,939,  0,  0,1440,  0,  0,  0,944,  0,
  947,951,  0,1449,953,  0,1452,  0,941,1446,1455,  0,962,964,  0,1461,966,
    0,972,977,982,  0,988,991,  0,  0,  0,1474,  0,1476,  0,1478,  0,999,
  1480,1002,  0,1007,1011,  0,1013,1016,  0,920,922,924,1437,935,1442,1457,
  955,959,1464,1467,1471,996,1482,1486,1489,1020,1027,  0,  0,  0,  0,  0,
  1511,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,
    0,  0,  0,  0,  0,  0,  0,  0,1538,  0,1517,1031,  0,1519,1522,1525,
  1527,1033,1036,1530,1533,1038,1041,1536,  0,1541,1043,  0,1046,  0,  0,
    0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,
    0,  0,  0,  0,  0,1584,  0,1563,1048,  0,1565,1568,1571,1573,1576,1579,
  1050,1053,1582,  0,1587,1055,  0,  0,  0,  0,1074,  0,  0,  0,  0,  0,
    0,1611,  0,  0,  0,1058,1061,1605,1063,1067,1608,1078,1082,1614,1084,
  1087,1091,1095,  0,1111,  0,  0,  0,  0,  0,  0,1636,  0,  0,  0,1097,
  1100,1104,1633,1115,1119,1639,1121,1125,  0,  0,  0,  0,1145,  0,  0,  0,
    0,  0,  0,1660,  0,  0,  0,1129,1132,1654,1134,1138,1657,1149,1153,1663,
  1155,1159,1163,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,
    0,  0,  0,  0,  0,  0,1697,  0,1681,1165,  0,1683,1686,1688,1167,1170,
  1691,1694,1172,1175,1177,1700,1179,  0,  0,  0,  0,  0,  0,  0,  0,  0,
    0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,1737,  0,1718,
    0,1720,1723,1726,1728,1182,1185,1731,1734,1187,1189,1740,1191,  0,  0,
    0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,1757,  0,
  1759,1762,1764,1766,1194,1197,1768,1771,1199,1201,1203,  0,  0,  0,  0,
    0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,1788,  0,1790,
  1793,1795,1797,1206,1799,1802,1208,1210,1212,  0,  0,  0,  0,  0,  0,  0,
    0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,1818,  0,1820,1823,1825,1827,
  1215,1829,1832,1217,1219,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,
    0,  0,  0,  0,1221,1847,1850,1852,1854,1223,1856,1859,1225,  0,  0,  0,
    0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,
    0,  0,  0,  0,  0,1894,  0,1872,1874,  0,1877,1880,1883,1885,1227,1230,
  1888,1891,1232,1235,1237,1897,1239,1242,  0,1252,1256,  0,1263,  0,  0,
  1920,  0,  0,  0,1268,  0,1273,1277,  0,1929,1279,  0,1932,  0,1265,1926,
  1271,1935,  0,1288,1290,  0,1942,1292,  0,1298,1303,1308,  0,1314,1317,
    0,  0,  0,1955,  0,1957,  0,1959,  0,1325,1961,1328,  0,1333,1335,1339,
    0,1341,1344,  0,1244,1246,1248,1917,1259,1922,1937,1281,1285,1945,1948,
  1952,1322,1963,1967,1971,1348,1355,  0,1359,  0,  0,  0,  0,  0,  0,  0,
    0,1362,1995,1999,1364,1367,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,2008,
  2011,2013,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,1370,  0,
  2022,2026,2029,  0,  0,  0,  0,  0,  0,  0,  0,1373,1375,  0,2040,2043,
  2046,  0,1377,  0,1382,  0,  0,2055,  0,  0,  0,  0,2060,  0,  0,  0,  0,
    0,  0,  0,  0,2067,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,
  2065,2069,2075,2078,2081,  0,  0,  0,  0,  0,  0,  0,2093,2095,1389,1391,
    0,  0,  0,2103,  0,  0,  0,2105,  0,1396,1398,  0,1385,2098,2109,1393,
  2111,1401,1404,  0,1406,1408,  0,  0,  0,  0,2125,1413,  0,1415,1418,1420,
    0,  0,  0,  0,1423,  0,  0,1433,  0,  0,  0,  0,  0,  0,  0,  0,  0,
  1435,  0,  0,  0,2153,  0,2135,2138,1426,1429,1431,2140,  0,2143,2146,
  2150,2155,  0,  0,  0,  0,  0,  0,  0,  0,2170,  0,  0,  0,  0,2179,  0,
    0,  0,  0,2181,  0,  0,1439,  0,  0,  0,  0,2192,  0,  0,  0,  0,  0,
  2173,2186,1437,2188,2194,2197,  0,  0,  0,  0,1441,2207,1444,  0,2002,
  2015,2033,2049,1380,2058,2063,2083,2114,2122,1410,2128,2131,2157,2200,
  2210,  0,  0,  0,  0,  0,  0,2231,  0,  0,  0,  0,  0,  0,2238,  0,  0,
    0,  0,  0,  0,2247,  0,  0,  0,1447,  0,1456,  0,1459,  0,1465,  0,  0,
  1469,  0,  0,  0,  0,  0,2265,  0,  0,  0,1449,1452,  0,  0,2256,2258,
    0,2260,2263,1472,2268,  0,1474,1477,1481,  0,  0,  0,  0,  0,  0,2291,
  1485,  0
};

static const unsigned short ag_key_index[] = {
   16,389,433,464,473,464,773,  0,810,773,1110,433,1135,1135,  0,  0,1147,
    0,1163,1163,1135,1147,1147,1135,1168,  0,1135,1135,  0,  0,1147,  0,
  1163,1163,1135,1147,1147,1135,1168,  0,1188,1267,1297,1299,1305,  0,1299,
    0,  0,773,1312,1319,1319,1319,1319,1319,1319,1319,1319,1319,1319,1319,
  1319,1319,1319,1329,1335,1351,1147,1147,1147,1147,1147,1147,1147,1147,
  1147,1147,1147,1147,1147,1147,1147,1147,1319,1319,1319,1319,1319,1319,
  1319,1319,1319,1319,1319,1319,1319,1319,1319,1319,1319,1319,1319,1319,
  1319,1319,1319,1319,1319,1319,1319,1319,1319,  0,1329,1335,1351,1147,1147,
  1147,1147,1147,1147,1147,1147,1147,1147,1147,1147,1147,1147,1147,1147,
  1147,1147,1147,1147,1147,1147,1147,1147,1147,1147,1147,1147,1147,1147,
  1147,1147,1147,1147,1147,1147,1147,1147,1147,1147,1147,1147,1147,1147,
  1147,1147,1147,1147,1362,1362,1362,1362,1372,1372,1351,1351,1351,1329,
  1329,1335,1335,1335,1335,1335,1335,1335,1335,1335,1335,1335,  0,  0,  0,
    0,1147,1147,1147,1147,1147,1147,1147,1147,1147,1147,1147,1147,1147,1147,
  1147,1147,1147,1147,1147,1147,1147,1147,1147,1147,1147,1147,1147,1147,
  1147,1147,1147,1147,1147,1147,1147,1147,1147,1147,1147,1147,1147,1147,
  1147,1147,1147,1147,1147,1147,1147,1147,1147,1147,1362,1362,1362,1362,
  1372,1372,1351,1351,1351,1329,1329,1335,1335,1335,1335,1335,1335,1335,
  1335,1335,1335,1335,1388,1319,1420,1163,1319,1147,1492,1163,  0,1163,  0,
    0,1147,  0,1514,1163,1163,1163,1163,  0,  0,  0,1420,1543,  0,1561,1420,
    0,1420,1420,1420,1589,  0,1163,  0,  0,  0,  0,  0,  0,  0,  0,  0,1420,
  1420,1617,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,1642,1666,1642,1666,
  1147,1642,1642,1642,1642,1420,1702,1742,1742,1774,1805,1835,1862,1147,
  1163,1163,1163,1163,1163,1163,1163,1135,1135,1163,1163,1163,1163,  0,1163,
  1163,1163,  0,  0,1305,1617,1305,1617,1299,1305,1305,1305,1305,1305,1305,
  1305,1305,1305,1305,1305,1305,1305,1305,1305,1305,1305,1305,1305,1666,
  1617,1617,1617,1666,1899,1305,1617,1666,1305,1305,1305,1188,  0,  0,  0,
  1147,1147,1974,1163,1312,1329,1335,1351,  0,  0,  0,  0,1147,1147,1147,
  1147,1147,1147,1147,1147,1147,1147,1147,1147,1147,1147,1147,1147,1147,
  1147,1147,1147,1147,1147,1147,1147,1147,1147,1147,1147,1147,1147,1147,
  1147,1147,1147,1147,1147,1147,1147,1147,1147,1147,1147,1147,1147,1147,
  1147,1147,1147,1147,1147,1147,1147,1362,1362,1362,1362,1372,1372,1351,
  1351,1351,1329,1329,1335,1335,1335,1335,1335,1335,1335,1335,1335,1335,
  1335,1147,1993,2214,2214,1135,1135,  0,  0,1163,1163,  0,  0,1163,1163,
  2234,2234,  0,  0,1147,1147,1147,1147,1147,1135,1135,1135,1135,1135,1135,
  1135,1135,1135,1135,1147,1147,1147,1147,1147,1147,1147,1147,1147,1147,
  1163,1147,1147,1147,1147,1135,1312,1312,1163,  0,1163,  0,1163,1163,1163,
    0,1163,1163,  0,  0,1561,1420,1420,  0,  0,  0,  0,  0,  0,  0,  0,  0,
    0,  0,1147,  0,1642,1642,1642,1642,1642,1642,1642,1147,1147,1147,1147,
  1147,1147,1147,1147,1147,1147,1147,1147,1147,1147,1147,1147,1147,1147,
  1147,1147,1147,1147,1147,1147,1147,1147,1147,1147,1147,1147,1147,1163,
  1163,1163,1163,1163,1135,1163,1163,  0,1163,1163,  0,1163,1163,1163,1163,
  1147,1147,1147,1147,1163,1163,1163,1319,1319,1319,2241,1319,1147,1147,
  1147,1319,1319,2245,1561,1561,1514,2245,  0,2250,2214,1561,  0,1147,1147,
  1514,1514,1514,1514,1514,2254,1147,1163,1163,1163,  0,  0,  0,  0,1135,
  1147,1147,1147,1147,1147,1147,1147,1147,1147,1147,  0,1642,1642,1642,1642,
  1642,1147,1147,1147,1147,1147,1147,1147,1147,1147,1147,1147,1147,1163,
  1163,1163,1163,1163,1163,1163,1163,1147,1147,1147,2245,1561,2271,1147,
  2289,1319,1561,1147,1514,1135,2294,2294,1163,1147,  0,  0,1135,  0,  0,
    0,  0,  0,  0,  0,  0,  0,  0,  0,1702,1702,1742,1742,1742,1742,1742,
  1742,1774,1774,1805,1805,1163,1163,1163,1335,1561,1147,1147,1135,1163,
    0,  0,  0,1335,1163,  0
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
87,0, 76,73,67,0, 72,0, 76,76,0, 69,76,0, 84,0, 77,0, 69,0, 
76,0, 84,0, 77,0, 76,0, 82,84,0, 76,78,0, 77,0, 88,84,0, 
84,76,69,0, 78,68,69,70,0, 69,82,73,76,79,71,0, 79,82,68,0, 
72,71,0, 82,0, 72,76,0, 10,0, 83,83,0, 65,84,65,0, 69,88,84,0, 
69,70,73,78,69,0, 72,79,0, 70,0, 69,0, 68,73,70,0, 82,79,82,0, 
69,70,0, 68,69,70,0, 67,76,85,68,69,0, 71,77,65,0, 78,68,69,70,0, 
61,0, 61,0, 73,0, 72,76,0, 71,0, 82,0, 79,67,75,0, 84,69,0, 
76,76,0, 69,71,0, 76,0, 80,0, 71,0, 66,0, 72,79,0, 83,69,0, 
70,0, 82,89,0, 85,0, 68,69,68,0, 78,0, 78,0, 73,76,76,0, 88,0, 
66,67,0, 65,76,76,0, 73,0, 73,0, 75,0, 84,0, 77,80,0, 80,0, 
83,72,0, 69,84,0, 70,73,82,83,84,0, 73,0, 67,76,73,66,0, 
85,76,69,0, 70,73,82,83,84,0, 73,0, 77,69,0, 71,69,0, 84,0, 
71,69,0, 72,76,0, 80,0, 76,73,67,0, 72,0, 76,76,0, 69,76,0, 
84,0, 77,0, 69,0, 76,0, 84,0, 77,0, 76,0, 76,78,0, 77,0, 
88,84,0, 84,76,69,0, 69,82,73,76,79,71,0, 79,82,68,0, 72,71,0, 
72,76,0, 68,69,0, 61,0, 73,0, 72,76,0, 71,0, 82,0, 79,67,75,0, 
84,69,0, 76,76,0, 69,71,0, 76,0, 80,0, 71,0, 66,0, 72,79,0, 
83,69,0, 70,0, 82,89,0, 68,69,68,0, 78,0, 78,0, 73,76,76,0, 
88,0, 66,67,0, 68,69,0, 65,76,76,0, 73,0, 73,0, 75,0, 84,0, 
77,80,0, 80,0, 83,72,0, 69,84,0, 70,73,82,83,84,0, 73,0, 
67,76,73,66,0, 85,76,69,0, 70,73,82,83,84,0, 73,0, 77,69,0, 
71,69,0, 84,0, 71,69,0, 72,76,0, 80,0, 76,73,67,0, 72,0, 
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
77,69,0, 80,65,71,69,0, 82,71,0, 71,69,0, 66,76,73,67,0, 84,0, 
75,76,78,0, 77,0, 88,84,0, 84,76,69,0, 69,82,73,76,79,71,0, 
79,82,68,0, 10,0, 68,69,0, 47,0, 42,0, 47,0, 10,0, 
78,80,65,71,69,0, 65,71,69,0, 67,0, 80,0, 72,76,41,0, 80,0, 
83,87,0, 42,0, 42,0, 78,68,0, 81,0, 79,68,0, 69,0, 82,0, 
79,82,0, 61,0, 61,0, 83,69,71,0, 79,67,75,0, 84,69,0, 
83,69,71,0, 71,0, 72,79,0, 82,89,0, 68,69,68,0, 78,0, 78,0, 
73,76,76,0, 69,88,0, 75,0, 84,0, 70,73,82,83,84,0, 
67,76,73,66,0, 68,85,76,69,0, 70,73,82,83,84,0, 77,69,0, 
80,65,71,69,0, 82,71,0, 71,69,0, 66,76,73,67,0, 75,76,78,0, 
77,0, 88,84,0, 84,76,69,0, 69,82,73,76,79,71,0, 79,82,68,0, 
42,0, 78,68,0, 81,0, 79,68,0, 69,0, 79,82,0, 39,0, 42,0, 
79,68,0, 69,0, 79,82,0, 92,39,0, 47,0, 69,73,76,0, 
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
80,65,71,69,0, 82,71,0, 71,69,0, 66,76,73,67,0, 84,0, 
75,76,78,0, 77,0, 88,84,0, 84,76,69,0, 69,82,73,76,79,71,0, 
79,82,68,0, 92,39,0, 73,0, 72,76,0, 72,82,0, 76,76,0, 76,0, 
80,0, 85,66,0, 73,0, 66,67,0, 65,76,76,0, 73,0, 73,0, 77,80,0, 
80,0, 83,72,0, 69,84,0, 73,0, 86,0, 73,0, 79,80,0, 84,0, 
72,76,0, 80,0, 83,72,0, 76,76,0, 69,76,0, 84,0, 77,0, 69,0, 
76,0, 77,0, 76,0, 72,71,0, 72,76,0, 10,0, 92,39,0, 72,76,41,0, 
73,76,0, 70,73,78,69,68,0, 79,79,82,0, 71,72,0, 80,0, 79,84,0, 
65,71,69,0, 81,82,84,0, 10,0, 
};
#define AG_TCV(x) (((int)(x) >= -1 && (int)(x) <= 255) ? ag_tcv[(x) + 1] : 0)

static const unsigned short ag_tcv[] = {
   38, 38,680,680,680,680,680,680,680,680,  1,423,680,680,680,680,680,680,
  680,680,680,680,680,680,680,680,680,680,680,680,680,680,680,  1,524,681,
  680,682,521,511,683,460,459,519,517,455,518,429,520,684,685,686,686,687,
  687,687,687,688,688,680,425,689,680,690,691,692,693,694,693,695,693,696,
  697,698,697,697,697,697,697,699,697,697,697,700,697,701,697,697,697,702,
  697,697,680,703,680,509,704,692,693,694,693,695,693,696,697,698,697,697,
  697,697,697,699,697,697,697,700,697,701,697,697,697,702,697,697,691,507,
  691,526,705,705,705,705,705,705,705,705,705,705,705,705,705,705,705,705,
  705,705,705,705,705,705,705,705,705,705,705,705,705,705,705,705,705,705,
  705,705,705,705,705,705,705,705,705,705,705,705,705,705,705,705,705,705,
  705,705,705,705,705,705,705,705,705,705,705,705,705,705,705,705,705,705,
  705,705,705,705,705,705,705,705,705,705,705,705,705,705,705,705,705,705,
  705,705,705,705,705,705,705,705,705,705,705,705,705,705,705,705,705,705,
  705,705,705,705,705,705,705,705,705,705,705,705,705,705,705,705,705,705,
  705,705,705,705,705
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
704,702,701,700,699,698,697,696,695,694,693,691,682,438,434,433,429,427,425,
  424,423,164,163,162,135,134,38,1,0,51,52,
1,0,
704,702,701,700,699,698,697,696,695,694,693,691,682,438,429,427,425,424,423,
  164,163,162,135,134,64,38,1,0,11,34,35,36,37,51,52,53,65,66,133,428,442,
458,457,456,454,453,452,451,450,449,448,447,446,440,433,1,0,51,52,
704,702,701,700,699,698,697,696,695,694,693,691,688,687,686,685,684,682,466,
  432,429,427,425,424,423,161,135,38,1,0,138,
458,457,456,454,453,452,451,450,449,448,447,446,440,433,0,60,67,68,74,76,78,
  79,80,81,82,83,84,85,87,88,
704,702,701,700,699,698,697,696,695,694,693,691,682,679,678,677,676,675,674,
  673,672,671,670,669,668,667,666,665,664,663,662,661,660,659,658,657,656,
  655,654,653,652,651,650,649,648,647,646,645,644,643,642,641,640,639,638,
  637,636,635,634,633,632,631,630,629,628,627,626,625,624,623,622,621,620,
  619,618,617,616,615,614,613,612,611,610,609,607,606,605,604,603,602,601,
  600,599,598,597,596,595,593,592,591,590,589,588,587,586,585,584,583,582,
  581,580,579,578,577,576,575,574,573,572,571,570,569,568,567,566,565,564,
  563,562,561,560,559,558,557,556,555,554,553,552,551,550,549,548,547,546,
  545,491,490,489,488,487,486,484,483,482,481,480,479,478,477,476,475,474,
  473,472,471,470,469,468,467,466,465,464,463,462,456,454,453,451,450,447,
  445,444,443,441,439,432,431,430,429,427,425,424,423,135,134,38,1,0,51,
  52,
423,0,45,
434,433,429,427,425,424,423,38,0,39,40,41,45,46,47,49,54,55,
704,702,701,700,699,698,697,696,695,694,693,691,682,679,678,677,676,675,674,
  673,672,671,670,669,668,667,666,665,664,663,662,661,660,659,658,657,656,
  655,654,653,652,651,650,649,648,647,646,645,644,643,642,641,640,639,638,
  637,636,635,634,633,632,631,630,629,628,627,626,625,624,623,622,621,620,
  619,618,617,616,615,614,613,612,611,610,609,607,606,605,604,603,602,601,
  600,599,598,597,596,595,593,592,591,590,589,588,587,586,585,584,583,582,
  581,580,579,578,577,576,575,574,573,572,571,570,569,568,567,566,565,564,
  563,562,561,560,559,558,557,556,555,554,553,552,551,550,549,548,547,546,
  545,491,490,489,488,487,486,484,483,482,481,480,479,478,477,476,475,474,
  473,472,471,470,469,468,467,466,465,464,463,462,456,454,453,451,450,447,
  445,444,443,441,439,432,431,430,429,427,425,424,423,135,134,38,1,0,51,
  52,
704,702,701,700,699,698,697,696,695,694,693,691,682,679,678,677,676,675,674,
  673,672,671,670,669,668,667,666,665,664,663,662,661,660,659,658,657,656,
  655,654,653,652,651,650,649,648,647,646,645,644,643,642,641,640,639,638,
  637,636,635,634,633,632,631,630,629,628,627,626,625,624,623,622,621,620,
  619,618,617,616,615,614,613,612,611,610,609,607,606,605,604,603,602,601,
  600,599,598,597,596,595,593,592,591,590,589,588,587,586,585,584,583,582,
  581,580,579,578,577,576,575,574,573,572,571,570,569,568,567,566,565,564,
  563,562,561,560,559,558,557,556,555,554,553,552,551,550,549,548,547,546,
  545,491,490,489,488,487,486,484,483,482,481,480,479,478,477,476,475,474,
  473,472,471,470,469,468,467,466,465,464,463,462,456,454,453,451,450,447,
  445,444,443,441,439,434,433,429,427,425,424,423,135,134,38,1,0,21,30,31,
  32,33,45,54,55,63,76,80,81,89,97,133,244,257,258,259,260,261,262,263,
  264,265,266,268,269,270,271,272,273,274,275,276,277,278,279,280,281,282,
  283,284,285,286,287,288,289,290,291,292,293,294,295,296,297,298,299,300,
  301,302,303,304,305,306,307,308,309,310,311,312,313,314,315,316,317,318,
  319,320,321,322,323,324,325,326,327,328,329,330,331,332,333,334,335,336,
  337,338,339,340,341,342,343,344,346,347,348,349,350,351,352,353,354,355,
  356,357,358,359,360,361,362,363,364,365,366,367,368,369,370,371,372,373,
  374,375,376,377,378,379,380,381,382,383,384,385,386,387,388,389,390,391,
  392,393,394,395,396,397,398,399,400,401,402,403,404,405,406,407,408,409,
  410,411,412,442,
704,702,701,700,699,698,697,696,695,694,693,691,682,438,429,427,425,424,423,
  164,163,162,135,134,64,38,1,0,11,35,36,51,52,53,65,66,133,428,442,
704,702,701,700,699,698,697,696,695,694,693,691,682,135,134,1,0,51,52,
704,702,701,700,699,698,697,696,695,694,693,691,682,135,134,1,0,51,52,
681,1,0,51,52,
681,1,0,51,52,
704,702,701,700,699,698,697,696,695,694,693,692,691,688,687,686,685,684,683,
  682,681,537,535,534,533,532,531,530,529,528,527,526,525,524,521,518,517,
  491,460,429,231,135,134,1,0,51,52,
681,1,0,51,52,
427,425,424,423,38,1,0,51,52,
427,425,424,423,38,1,0,51,52,
704,702,701,700,699,698,697,696,695,694,693,691,682,135,134,1,0,51,52,
704,702,701,700,699,698,697,696,695,694,693,692,691,688,687,686,685,684,683,
  682,537,535,534,533,532,531,530,529,528,527,526,525,524,521,518,517,491,
  460,429,231,135,134,1,0,51,52,
704,702,701,700,699,698,697,696,695,694,693,692,691,688,687,686,685,684,683,
  682,537,535,534,533,532,531,530,529,528,527,526,525,524,521,518,517,491,
  460,429,231,135,134,1,0,51,52,
704,702,701,700,699,698,697,696,695,694,693,691,682,135,134,1,0,51,52,
445,444,443,441,439,1,0,51,52,
704,703,702,701,700,699,698,697,696,695,694,693,689,688,687,686,685,684,681,
  520,429,1,0,51,52,
704,702,701,700,699,698,697,696,695,694,693,691,682,135,134,0,21,89,133,442,
704,702,701,700,699,698,697,696,695,694,693,691,682,135,134,0,21,133,442,
681,1,0,51,52,
681,1,0,51,52,
704,702,701,700,699,698,697,696,695,694,693,692,691,688,687,686,685,684,683,
  682,681,537,535,534,533,532,531,530,529,528,527,526,525,524,521,518,517,
  491,460,429,231,135,134,1,0,51,52,
681,0,10,12,435,
427,425,424,423,38,1,0,51,52,
427,425,424,423,38,1,0,51,52,
704,702,701,700,699,698,697,696,695,694,693,691,682,135,134,0,21,133,442,
704,702,701,700,699,698,697,696,695,694,693,692,691,688,687,686,685,684,683,
  682,537,535,534,533,532,531,530,529,528,527,526,525,524,521,518,517,491,
  460,429,231,135,134,0,2,6,7,8,9,15,17,21,23,24,25,26,27,28,29,57,77,89,
  91,131,133,166,196,198,203,204,205,206,207,208,209,210,211,212,213,214,
  215,216,217,218,219,442,492,538,
704,702,701,700,699,698,697,696,695,694,693,692,691,688,687,686,685,684,683,
  682,537,535,534,533,532,531,530,529,528,527,526,525,524,521,518,517,491,
  460,429,231,135,134,0,2,6,7,8,9,15,17,21,23,24,25,26,27,28,29,57,77,89,
  91,131,133,166,196,198,203,204,205,206,207,208,209,210,211,212,213,214,
  215,216,217,218,219,442,492,538,
704,702,701,700,699,698,697,696,695,694,693,691,682,135,134,0,21,133,442,
445,444,443,441,439,0,69,70,71,72,73,
689,681,0,10,12,13,14,435,436,
705,704,703,702,701,700,699,698,697,696,695,694,693,692,691,690,689,688,687,
  686,685,684,683,682,681,680,526,524,521,520,519,518,517,511,509,507,460,
  459,455,438,434,433,429,427,426,425,424,423,164,163,162,135,134,38,1,0,
  51,52,
705,704,703,702,701,700,699,698,697,696,695,694,693,692,691,690,689,688,687,
  686,685,684,683,682,681,680,526,524,521,520,519,518,517,511,509,507,491,
  490,489,488,487,486,484,483,482,481,480,479,478,477,476,475,474,473,472,
  471,470,469,468,467,466,465,464,463,462,461,460,459,456,455,454,453,445,
  444,443,441,439,434,433,431,430,429,427,426,425,424,423,38,1,0,51,52,
434,433,0,60,61,62,
705,704,703,702,701,700,699,698,697,696,695,694,693,692,691,690,689,688,687,
  686,685,684,683,682,681,680,526,524,521,520,519,518,517,511,509,507,460,
  459,455,429,427,426,425,423,1,0,51,52,
705,704,703,702,701,700,699,698,697,696,695,694,693,692,691,690,689,688,687,
  686,685,684,683,682,681,680,526,524,521,520,519,518,517,511,509,507,461,
  460,459,455,429,427,426,425,424,423,38,1,0,51,52,
705,704,703,702,701,700,699,698,697,696,695,694,693,692,691,690,689,688,687,
  686,685,684,683,682,681,680,526,524,521,520,519,518,517,511,509,507,460,
  459,455,429,425,423,1,0,51,52,
705,704,703,702,701,700,699,698,697,696,695,694,693,692,691,690,689,688,687,
  686,685,684,683,682,681,680,526,524,521,520,519,518,517,511,509,507,460,
  459,455,429,427,426,425,423,1,0,40,45,46,47,48,49,54,86,91,93,140,141,
  143,145,146,147,148,149,150,154,157,158,185,187,189,195,196,197,199,200,
  203,205,220,224,225,229,413,414,415,416,417,418,419,420,421,422,
705,704,703,702,701,700,699,698,697,696,695,694,693,692,691,690,689,688,687,
  686,685,684,683,682,681,680,526,524,521,520,519,518,517,511,509,507,460,
  459,455,429,425,423,1,0,43,44,46,54,86,91,93,140,141,143,145,146,147,
  148,149,150,154,157,158,185,187,189,195,196,197,199,200,203,205,220,224,
  225,229,413,414,415,416,417,418,419,420,421,422,
705,704,703,702,701,700,699,698,697,696,695,694,693,692,691,690,689,688,687,
  686,685,684,683,682,681,680,526,524,521,520,519,518,517,511,509,507,460,
  459,455,429,425,423,1,0,43,44,46,54,86,91,93,140,141,143,145,146,147,
  148,149,150,154,157,158,185,187,189,195,196,197,199,200,203,205,220,224,
  225,229,413,414,415,416,417,418,419,420,421,422,
704,702,701,700,699,698,697,696,695,694,693,691,682,679,678,677,676,675,674,
  673,672,671,670,669,668,667,666,665,664,663,662,661,660,659,658,657,656,
  655,654,653,652,651,650,649,648,647,646,645,644,643,642,641,640,639,638,
  637,636,635,634,633,632,631,630,629,628,627,626,625,624,623,622,621,620,
  619,618,617,616,615,614,613,612,611,610,609,607,606,605,604,603,602,601,
  600,599,598,597,596,595,593,592,591,590,589,588,587,586,585,584,583,582,
  581,580,579,578,577,576,575,574,573,572,571,570,569,568,567,566,565,564,
  563,562,561,560,559,558,557,556,555,554,553,552,551,550,549,548,547,546,
  545,491,490,489,488,487,486,484,483,482,481,480,479,478,477,476,475,474,
  473,472,471,470,469,468,467,466,465,464,463,462,456,454,453,451,450,447,
  445,444,443,441,439,432,431,430,429,427,425,424,423,135,134,38,0,21,30,
  31,32,33,39,40,41,45,46,47,49,54,55,59,63,76,80,81,89,97,133,244,257,
  258,259,260,261,262,263,264,265,266,268,269,270,271,272,273,274,275,276,
  277,278,279,280,281,282,283,284,285,286,287,288,289,290,291,292,293,294,
  295,296,297,298,299,300,301,302,303,304,305,306,307,308,309,310,311,312,
  313,314,315,316,317,318,319,320,321,322,323,324,325,326,327,328,329,330,
  331,332,333,334,335,336,337,338,339,340,341,342,343,344,346,347,348,349,
  350,351,352,353,354,355,356,357,358,359,360,361,362,363,364,365,366,367,
  368,369,370,371,372,373,374,375,376,377,378,379,380,381,382,383,384,385,
  386,387,388,389,390,391,392,393,394,395,396,397,398,399,400,401,402,403,
  404,405,406,407,408,409,410,411,412,442,
494,491,427,425,424,423,38,1,0,51,52,
544,427,425,424,423,38,1,0,51,52,
544,427,425,424,423,38,1,0,51,52,
544,427,425,424,423,38,1,0,51,52,
544,427,425,424,423,38,1,0,51,52,
544,427,425,424,423,38,1,0,51,52,
544,427,425,424,423,38,1,0,51,52,
544,427,425,424,423,38,1,0,51,52,
544,427,425,424,423,38,1,0,51,52,
544,427,425,424,423,38,1,0,51,52,
544,427,425,424,423,38,1,0,51,52,
544,427,425,424,423,38,1,0,51,52,
544,427,425,424,423,38,1,0,51,52,
544,427,425,424,423,38,1,0,51,52,
544,427,425,424,423,38,1,0,51,52,
668,543,542,541,248,246,238,236,234,1,0,51,52,
540,241,240,239,238,237,236,235,234,1,0,51,52,
668,543,248,246,238,236,234,1,0,51,52,
704,702,701,700,699,698,697,696,695,694,693,692,691,688,687,686,685,684,683,
  682,537,535,534,533,532,531,530,529,528,527,526,525,524,521,518,517,491,
  460,429,231,135,134,1,0,51,52,
704,702,701,700,699,698,697,696,695,694,693,692,691,688,687,686,685,684,683,
  682,537,535,534,533,532,531,530,529,528,527,526,525,524,521,518,517,491,
  460,429,231,135,134,1,0,51,52,
704,702,701,700,699,698,697,696,695,694,693,692,691,688,687,686,685,684,683,
  682,537,535,534,533,532,531,530,529,528,527,526,525,524,521,518,517,491,
  460,429,231,135,134,1,0,51,52,
704,702,701,700,699,698,697,696,695,694,693,692,691,688,687,686,685,684,683,
  682,537,535,534,533,532,531,530,529,528,527,526,525,524,521,518,517,491,
  460,429,231,135,134,1,0,51,52,
704,702,701,700,699,698,697,696,695,694,693,692,691,688,687,686,685,684,683,
  682,537,535,534,533,532,531,530,529,528,527,526,525,524,521,518,517,491,
  460,429,231,135,134,1,0,51,52,
704,702,701,700,699,698,697,696,695,694,693,692,691,688,687,686,685,684,683,
  682,537,535,534,533,532,531,530,529,528,527,526,525,524,521,518,517,491,
  460,429,231,135,134,1,0,51,52,
704,702,701,700,699,698,697,696,695,694,693,692,691,688,687,686,685,684,683,
  682,537,535,534,533,532,531,530,529,528,527,526,525,524,521,518,517,491,
  460,429,231,135,134,1,0,51,52,
704,702,701,700,699,698,697,696,695,694,693,692,691,688,687,686,685,684,683,
  682,537,535,534,533,532,531,530,529,528,527,526,525,524,521,518,517,491,
  460,429,231,135,134,1,0,51,52,
704,702,701,700,699,698,697,696,695,694,693,692,691,688,687,686,685,684,683,
  682,537,535,534,533,532,531,530,529,528,527,526,525,524,521,518,517,491,
  460,429,231,135,134,1,0,51,52,
704,702,701,700,699,698,697,696,695,694,693,692,691,688,687,686,685,684,683,
  682,537,535,534,533,532,531,530,529,528,527,526,525,524,521,518,517,491,
  460,429,231,135,134,1,0,51,52,
704,702,701,700,699,698,697,696,695,694,693,692,691,688,687,686,685,684,683,
  682,537,535,534,533,532,531,530,529,528,527,526,525,524,521,518,517,491,
  460,429,231,135,134,1,0,51,52,
704,702,701,700,699,698,697,696,695,694,693,692,691,688,687,686,685,684,683,
  682,537,535,534,533,532,531,530,529,528,527,526,525,524,521,518,517,491,
  460,429,231,135,134,1,0,51,52,
704,702,701,700,699,698,697,696,695,694,693,692,691,688,687,686,685,684,683,
  682,537,535,534,533,532,531,530,529,528,527,526,525,524,521,518,517,491,
  460,429,231,135,134,1,0,51,52,
704,702,701,700,699,698,697,696,695,694,693,692,691,688,687,686,685,684,683,
  682,537,535,534,533,532,531,530,529,528,527,526,525,524,521,518,517,491,
  460,429,231,135,134,1,0,51,52,
704,702,701,700,699,698,697,696,695,694,693,692,691,688,687,686,685,684,683,
  682,537,535,534,533,532,531,530,529,528,527,526,525,524,521,518,517,491,
  460,429,231,135,134,1,0,51,52,
704,702,701,700,699,698,697,696,695,694,693,692,691,688,687,686,685,684,683,
  682,537,535,534,533,532,531,530,529,528,527,526,525,524,521,518,517,491,
  460,429,231,135,134,1,0,51,52,
544,427,425,424,423,38,1,0,51,52,
544,427,425,424,423,38,1,0,51,52,
544,427,425,424,423,38,1,0,51,52,
544,427,425,424,423,38,1,0,51,52,
544,427,425,424,423,38,1,0,51,52,
544,427,425,424,423,38,1,0,51,52,
544,427,425,424,423,38,1,0,51,52,
544,427,425,424,423,38,1,0,51,52,
544,427,425,424,423,38,1,0,51,52,
544,427,425,424,423,38,1,0,51,52,
544,427,425,424,423,38,1,0,51,52,
544,427,425,424,423,38,1,0,51,52,
544,427,425,424,423,38,1,0,51,52,
544,427,425,424,423,38,1,0,51,52,
544,427,425,424,423,38,1,0,51,52,
544,427,425,424,423,38,1,0,51,52,
544,427,425,424,423,38,1,0,51,52,
544,427,425,424,423,38,1,0,51,52,
544,427,425,424,423,38,1,0,51,52,
544,427,425,424,423,38,1,0,51,52,
544,427,425,424,423,38,1,0,51,52,
544,427,425,424,423,38,1,0,51,52,
544,427,425,424,423,38,1,0,51,52,
544,427,425,424,423,38,1,0,51,52,
544,427,425,424,423,38,1,0,51,52,
544,427,425,424,423,38,1,0,51,52,
544,427,425,424,423,38,1,0,51,52,
544,427,425,424,423,38,1,0,51,52,
544,427,425,424,423,38,1,0,51,52,
687,686,685,684,1,0,51,52,
668,543,542,541,248,246,238,236,234,1,0,51,52,
540,241,240,239,238,237,236,235,234,1,0,51,52,
668,543,248,246,238,236,234,1,0,51,52,
704,702,701,700,699,698,697,696,695,694,693,692,691,688,687,686,685,684,683,
  682,537,535,534,533,532,531,530,529,528,527,526,525,524,521,518,517,491,
  460,429,231,135,134,1,0,51,52,
704,702,701,700,699,698,697,696,695,694,693,692,691,688,687,686,685,684,683,
  682,537,535,534,533,532,531,530,529,528,527,526,525,524,521,518,517,491,
  460,429,231,135,134,1,0,51,52,
704,702,701,700,699,698,697,696,695,694,693,692,691,688,687,686,685,684,683,
  682,537,535,534,533,532,531,530,529,528,527,526,525,524,521,518,517,491,
  460,429,231,135,134,1,0,51,52,
704,702,701,700,699,698,697,696,695,694,693,692,691,688,687,686,685,684,683,
  682,537,535,534,533,532,531,530,529,528,527,526,525,524,521,518,517,491,
  460,429,231,135,134,1,0,51,52,
704,702,701,700,699,698,697,696,695,694,693,692,691,688,687,686,685,684,683,
  682,537,535,534,533,532,531,530,529,528,527,526,525,524,521,518,517,491,
  460,429,231,135,134,1,0,51,52,
704,702,701,700,699,698,697,696,695,694,693,692,691,688,687,686,685,684,683,
  682,537,535,534,533,532,531,530,529,528,527,526,525,524,521,518,517,491,
  460,429,231,135,134,1,0,51,52,
704,702,701,700,699,698,697,696,695,694,693,692,691,688,687,686,685,684,683,
  682,537,535,534,533,532,531,530,529,528,527,526,525,524,521,518,517,491,
  460,429,231,135,134,1,0,51,52,
704,702,701,700,699,698,697,696,695,694,693,692,691,688,687,686,685,684,683,
  682,537,535,534,533,532,531,530,529,528,527,526,525,524,521,518,517,491,
  460,429,231,135,134,1,0,51,52,
704,702,701,700,699,698,697,696,695,694,693,692,691,688,687,686,685,684,683,
  682,537,535,534,533,532,531,530,529,528,527,526,525,524,521,518,517,491,
  460,429,231,135,134,1,0,51,52,
704,702,701,700,699,698,697,696,695,694,693,692,691,688,687,686,685,684,683,
  682,537,535,534,533,532,531,530,529,528,527,526,525,524,521,518,517,491,
  460,429,231,135,134,1,0,51,52,
704,702,701,700,699,698,697,696,695,694,693,692,691,688,687,686,685,684,683,
  682,537,535,534,533,532,531,530,529,528,527,526,525,524,521,518,517,491,
  460,429,231,135,134,1,0,51,52,
704,702,701,700,699,698,697,696,695,694,693,692,691,688,687,686,685,684,683,
  682,537,535,534,533,532,531,530,529,528,527,526,525,524,521,518,517,491,
  460,429,231,135,134,1,0,51,52,
704,702,701,700,699,698,697,696,695,694,693,692,691,688,687,686,685,684,683,
  682,537,535,534,533,532,531,530,529,528,527,526,525,524,521,518,517,491,
  460,429,231,135,134,1,0,51,52,
704,702,701,700,699,698,697,696,695,694,693,692,691,688,687,686,685,684,683,
  682,537,535,534,533,532,531,530,529,528,527,526,525,524,521,518,517,491,
  460,429,231,135,134,1,0,51,52,
704,702,701,700,699,698,697,696,695,694,693,692,691,688,687,686,685,684,683,
  682,537,535,534,533,532,531,530,529,528,527,526,525,524,521,518,517,491,
  460,429,231,135,134,1,0,51,52,
704,702,701,700,699,698,697,696,695,694,693,692,691,688,687,686,685,684,683,
  682,537,535,534,533,532,531,530,529,528,527,526,525,524,521,518,517,491,
  460,429,231,135,134,1,0,51,52,
704,702,701,700,699,698,697,696,695,694,693,692,691,688,687,686,685,684,683,
  682,537,535,534,533,532,531,530,529,528,527,526,525,524,521,518,517,491,
  460,429,231,135,134,1,0,51,52,
704,702,701,700,699,698,697,696,695,694,693,692,691,688,687,686,685,684,683,
  682,537,535,534,533,532,531,530,529,528,527,526,525,524,521,518,517,491,
  460,429,231,135,134,1,0,51,52,
704,702,701,700,699,698,697,696,695,694,693,692,691,688,687,686,685,684,683,
  682,537,535,534,533,532,531,530,529,528,527,526,525,524,521,518,517,491,
  460,429,231,135,134,1,0,51,52,
704,702,701,700,699,698,697,696,695,694,693,692,691,688,687,686,685,684,683,
  682,537,535,534,533,532,531,530,529,528,527,526,525,524,521,518,517,491,
  460,429,231,135,134,1,0,51,52,
704,702,701,700,699,698,697,696,695,694,693,692,691,688,687,686,685,684,683,
  682,537,535,534,533,532,531,530,529,528,527,526,525,524,521,518,517,491,
  460,429,231,135,134,1,0,51,52,
704,702,701,700,699,698,697,696,695,694,693,692,691,688,687,686,685,684,683,
  682,537,535,534,533,532,531,530,529,528,527,526,525,524,521,518,517,491,
  460,429,231,135,134,1,0,51,52,
704,702,701,700,699,698,697,696,695,694,693,692,691,688,687,686,685,684,683,
  682,537,535,534,533,532,531,530,529,528,527,526,525,524,521,518,517,491,
  460,429,231,135,134,1,0,51,52,
704,702,701,700,699,698,697,696,695,694,693,692,691,688,687,686,685,684,683,
  682,537,535,534,533,532,531,530,529,528,527,526,525,524,521,518,517,491,
  460,429,231,135,134,1,0,51,52,
704,702,701,700,699,698,697,696,695,694,693,692,691,688,687,686,685,684,683,
  682,537,535,534,533,532,531,530,529,528,527,526,525,524,521,518,517,491,
  460,429,231,135,134,1,0,51,52,
704,702,701,700,699,698,697,696,695,694,693,692,691,688,687,686,685,684,683,
  682,537,535,534,533,532,531,530,529,528,527,526,525,524,521,518,517,491,
  460,429,231,135,134,1,0,51,52,
704,702,701,700,699,698,697,696,695,694,693,692,691,688,687,686,685,684,683,
  682,537,535,534,533,532,531,530,529,528,527,526,525,524,521,518,517,491,
  460,429,231,135,134,1,0,51,52,
704,702,701,700,699,698,697,696,695,694,693,692,691,688,687,686,685,684,683,
  682,537,535,534,533,532,531,530,529,528,527,526,525,524,521,518,517,491,
  460,429,231,135,134,1,0,51,52,
704,702,701,700,699,698,697,696,695,694,693,692,691,688,687,686,685,684,683,
  682,537,535,534,533,532,531,530,529,528,527,526,525,524,521,518,517,491,
  460,429,231,135,134,1,0,51,52,
704,702,701,700,699,698,697,696,695,694,693,692,691,688,687,686,685,684,683,
  682,537,535,534,533,532,531,530,529,528,527,526,525,524,521,518,517,491,
  460,429,231,135,134,1,0,51,52,
704,702,701,700,699,698,697,696,695,694,693,692,691,688,687,686,685,684,683,
  682,537,535,534,533,532,531,530,529,528,527,526,525,524,521,518,517,491,
  460,429,231,135,134,1,0,51,52,
704,702,701,700,699,698,697,696,695,694,693,692,691,688,687,686,685,684,683,
  682,537,535,534,533,532,531,530,529,528,527,526,525,524,521,518,517,491,
  460,429,231,135,134,1,0,51,52,
704,702,701,700,699,698,697,696,695,694,693,692,691,688,687,686,685,684,683,
  682,537,535,534,533,532,531,530,529,528,527,526,525,524,521,518,517,491,
  460,429,231,135,134,1,0,51,52,
704,702,701,700,699,698,697,696,695,694,693,692,691,688,687,686,685,684,683,
  682,537,535,534,533,532,531,530,529,528,527,526,525,524,521,518,517,491,
  460,429,231,135,134,1,0,51,52,
704,702,701,700,699,698,697,696,695,694,693,692,691,688,687,686,685,684,683,
  682,537,535,534,533,532,531,530,529,528,527,526,525,524,521,518,517,491,
  460,429,231,135,134,1,0,51,52,
704,702,701,700,699,698,697,696,695,694,693,692,691,688,687,686,685,684,683,
  682,537,535,534,533,532,531,530,529,528,527,526,525,524,521,518,517,491,
  460,429,231,135,134,1,0,51,52,
704,702,701,700,699,698,697,696,695,694,693,692,691,688,687,686,685,684,683,
  682,537,535,534,533,532,531,530,529,528,527,526,525,524,521,518,517,491,
  460,429,231,135,134,1,0,51,52,
704,702,701,700,699,698,697,696,695,694,693,692,691,688,687,686,685,684,683,
  682,537,535,534,533,532,531,530,529,528,527,526,525,524,521,518,517,491,
  460,429,231,135,134,1,0,51,52,
704,702,701,700,699,698,697,696,695,694,693,692,691,688,687,686,685,684,683,
  682,537,535,534,533,532,531,530,529,528,527,526,525,524,521,518,517,491,
  460,429,231,135,134,1,0,51,52,
704,702,701,700,699,698,697,696,695,694,693,692,691,688,687,686,685,684,683,
  682,537,535,534,533,532,531,530,529,528,527,526,525,524,521,518,517,491,
  460,429,231,135,134,1,0,51,52,
704,702,701,700,699,698,697,696,695,694,693,692,691,688,687,686,685,684,683,
  682,537,535,534,533,532,531,530,529,528,527,526,525,524,521,518,517,491,
  460,429,231,135,134,1,0,51,52,
704,702,701,700,699,698,697,696,695,694,693,692,691,688,687,686,685,684,683,
  682,537,535,534,533,532,531,530,529,528,527,526,525,524,521,518,517,491,
  460,429,231,135,134,1,0,51,52,
704,702,701,700,699,698,697,696,695,694,693,692,691,688,687,686,685,684,683,
  682,537,535,534,533,532,531,530,529,528,527,526,525,524,521,518,517,491,
  460,429,231,135,134,1,0,51,52,
704,702,701,700,699,698,697,696,695,694,693,692,691,688,687,686,685,684,683,
  682,537,535,534,533,532,531,530,529,528,527,526,525,524,521,518,517,491,
  460,429,231,135,134,1,0,51,52,
704,702,701,700,699,698,697,696,695,694,693,692,691,688,687,686,685,684,683,
  682,537,535,534,533,532,531,530,529,528,527,526,525,524,521,518,517,491,
  460,429,231,135,134,1,0,51,52,
704,702,701,700,699,698,697,696,695,694,693,692,691,688,687,686,685,684,683,
  682,537,535,534,533,532,531,530,529,528,527,526,525,524,521,518,517,491,
  460,429,231,135,134,1,0,51,52,
704,702,701,700,699,698,697,696,695,694,693,692,691,688,687,686,685,684,683,
  682,537,535,534,533,532,531,530,529,528,527,526,525,524,521,518,517,491,
  460,429,231,135,134,1,0,51,52,
704,702,701,700,699,698,697,696,695,694,693,692,691,688,687,686,685,684,683,
  682,537,535,534,533,532,531,530,529,528,527,526,525,524,521,518,517,491,
  460,429,231,135,134,1,0,51,52,
668,540,539,248,246,238,236,234,1,0,51,52,
668,540,539,248,246,238,236,234,1,0,51,52,
668,540,539,248,246,238,236,234,1,0,51,52,
668,540,539,248,246,238,236,234,1,0,51,52,
668,246,236,234,1,0,51,52,
668,246,236,234,1,0,51,52,
668,543,248,246,238,236,234,1,0,51,52,
668,543,248,246,238,236,234,1,0,51,52,
668,543,248,246,238,236,234,1,0,51,52,
668,543,542,541,248,246,238,236,234,1,0,51,52,
668,543,542,541,248,246,238,236,234,1,0,51,52,
540,241,240,239,238,237,236,235,234,1,0,51,52,
540,241,240,239,238,237,236,235,234,1,0,51,52,
540,241,240,239,238,237,236,235,234,1,0,51,52,
540,241,240,239,238,237,236,235,234,1,0,51,52,
540,241,240,239,238,237,236,235,234,1,0,51,52,
540,241,240,239,238,237,236,235,234,1,0,51,52,
540,241,240,239,238,237,236,235,234,1,0,51,52,
540,241,240,239,238,237,236,235,234,1,0,51,52,
540,241,240,239,238,237,236,235,234,1,0,51,52,
540,241,240,239,238,237,236,235,234,1,0,51,52,
540,241,240,239,238,237,236,235,234,1,0,51,52,
687,686,685,684,1,0,51,52,
455,1,0,51,52,
455,1,0,51,52,
455,1,0,51,52,
704,702,701,700,699,698,697,696,695,694,693,692,691,688,687,686,685,684,683,
  682,537,535,534,533,532,531,530,529,528,527,526,525,524,521,518,517,491,
  460,429,231,135,134,1,0,51,52,
704,702,701,700,699,698,697,696,695,694,693,692,691,688,687,686,685,684,683,
  682,537,535,534,533,532,531,530,529,528,527,526,525,524,521,518,517,491,
  460,429,231,135,134,1,0,51,52,
704,702,701,700,699,698,697,696,695,694,693,692,691,688,687,686,685,684,683,
  682,537,535,534,533,532,531,530,529,528,527,526,525,524,521,518,517,491,
  460,429,231,135,134,1,0,51,52,
704,702,701,700,699,698,697,696,695,694,693,692,691,688,687,686,685,684,683,
  682,537,535,534,533,532,531,530,529,528,527,526,525,524,521,518,517,491,
  460,429,231,135,134,1,0,51,52,
704,702,701,700,699,698,697,696,695,694,693,692,691,688,687,686,685,684,683,
  682,537,535,534,533,532,531,530,529,528,527,526,525,524,521,518,517,491,
  460,429,231,135,134,1,0,51,52,
704,702,701,700,699,698,697,696,695,694,693,692,691,688,687,686,685,684,683,
  682,537,535,534,533,532,531,530,529,528,527,526,525,524,521,518,517,491,
  460,429,231,135,134,1,0,51,52,
704,702,701,700,699,698,697,696,695,694,693,692,691,688,687,686,685,684,683,
  682,537,535,534,533,532,531,530,529,528,527,526,525,524,521,518,517,491,
  460,429,231,135,134,1,0,51,52,
704,702,701,700,699,698,697,696,695,694,693,692,691,688,687,686,685,684,683,
  682,537,535,534,533,532,531,530,529,528,527,526,525,524,521,518,517,491,
  460,429,231,135,134,1,0,51,52,
704,702,701,700,699,698,697,696,695,694,693,692,691,688,687,686,685,684,683,
  682,537,535,534,533,532,531,530,529,528,527,526,525,524,521,518,517,491,
  460,429,231,135,134,1,0,51,52,
704,702,701,700,699,698,697,696,695,694,693,692,691,688,687,686,685,684,683,
  682,537,535,534,533,532,531,530,529,528,527,526,525,524,521,518,517,491,
  460,429,231,135,134,1,0,51,52,
704,702,701,700,699,698,697,696,695,694,693,692,691,688,687,686,685,684,683,
  682,537,535,534,533,532,531,530,529,528,527,526,525,524,521,518,517,491,
  460,429,231,135,134,1,0,51,52,
704,702,701,700,699,698,697,696,695,694,693,692,691,688,687,686,685,684,683,
  682,537,535,534,533,532,531,530,529,528,527,526,525,524,521,518,517,491,
  460,429,231,135,134,1,0,51,52,
704,702,701,700,699,698,697,696,695,694,693,692,691,688,687,686,685,684,683,
  682,537,535,534,533,532,531,530,529,528,527,526,525,524,521,518,517,491,
  460,429,231,135,134,1,0,51,52,
704,702,701,700,699,698,697,696,695,694,693,692,691,688,687,686,685,684,683,
  682,537,535,534,533,532,531,530,529,528,527,526,525,524,521,518,517,491,
  460,429,231,135,134,1,0,51,52,
704,702,701,700,699,698,697,696,695,694,693,692,691,688,687,686,685,684,683,
  682,537,535,534,533,532,531,530,529,528,527,526,525,524,521,518,517,491,
  460,429,231,135,134,1,0,51,52,
704,702,701,700,699,698,697,696,695,694,693,692,691,688,687,686,685,684,683,
  682,537,535,534,533,532,531,530,529,528,527,526,525,524,521,518,517,491,
  460,429,231,135,134,1,0,51,52,
704,702,701,700,699,698,697,696,695,694,693,692,691,688,687,686,685,684,683,
  682,537,535,534,533,532,531,530,529,528,527,526,525,524,521,518,517,491,
  460,429,231,135,134,1,0,51,52,
704,702,701,700,699,698,697,696,695,694,693,692,691,688,687,686,685,684,683,
  682,537,535,534,533,532,531,530,529,528,527,526,525,524,521,518,517,491,
  460,429,231,135,134,1,0,51,52,
704,702,701,700,699,698,697,696,695,694,693,692,691,688,687,686,685,684,683,
  682,537,535,534,533,532,531,530,529,528,527,526,525,524,521,518,517,491,
  460,429,231,135,134,1,0,51,52,
704,702,701,700,699,698,697,696,695,694,693,692,691,688,687,686,685,684,683,
  682,537,535,534,533,532,531,530,529,528,527,526,525,524,521,518,517,491,
  460,429,231,135,134,1,0,51,52,
704,702,701,700,699,698,697,696,695,694,693,692,691,688,687,686,685,684,683,
  682,537,535,534,533,532,531,530,529,528,527,526,525,524,521,518,517,491,
  460,429,231,135,134,1,0,51,52,
704,702,701,700,699,698,697,696,695,694,693,692,691,688,687,686,685,684,683,
  682,537,535,534,533,532,531,530,529,528,527,526,525,524,521,518,517,491,
  460,429,231,135,134,1,0,51,52,
704,702,701,700,699,698,697,696,695,694,693,692,691,688,687,686,685,684,683,
  682,537,535,534,533,532,531,530,529,528,527,526,525,524,521,518,517,491,
  460,429,231,135,134,1,0,51,52,
704,702,701,700,699,698,697,696,695,694,693,692,691,688,687,686,685,684,683,
  682,537,535,534,533,532,531,530,529,528,527,526,525,524,521,518,517,491,
  460,429,231,135,134,1,0,51,52,
704,702,701,700,699,698,697,696,695,694,693,692,691,688,687,686,685,684,683,
  682,537,535,534,533,532,531,530,529,528,527,526,525,524,521,518,517,491,
  460,429,231,135,134,1,0,51,52,
704,702,701,700,699,698,697,696,695,694,693,692,691,688,687,686,685,684,683,
  682,537,535,534,533,532,531,530,529,528,527,526,525,524,521,518,517,491,
  460,429,231,135,134,1,0,51,52,
704,702,701,700,699,698,697,696,695,694,693,692,691,688,687,686,685,684,683,
  682,537,535,534,533,532,531,530,529,528,527,526,525,524,521,518,517,491,
  460,429,231,135,134,1,0,51,52,
704,702,701,700,699,698,697,696,695,694,693,692,691,688,687,686,685,684,683,
  682,537,535,534,533,532,531,530,529,528,527,526,525,524,521,518,517,491,
  460,429,231,135,134,1,0,51,52,
704,702,701,700,699,698,697,696,695,694,693,692,691,688,687,686,685,684,683,
  682,537,535,534,533,532,531,530,529,528,527,526,525,524,521,518,517,491,
  460,429,231,135,134,1,0,51,52,
704,702,701,700,699,698,697,696,695,694,693,692,691,688,687,686,685,684,683,
  682,537,535,534,533,532,531,530,529,528,527,526,525,524,521,518,517,491,
  460,429,231,135,134,1,0,51,52,
704,702,701,700,699,698,697,696,695,694,693,692,691,688,687,686,685,684,683,
  682,537,535,534,533,532,531,530,529,528,527,526,525,524,521,518,517,491,
  460,429,231,135,134,1,0,51,52,
704,702,701,700,699,698,697,696,695,694,693,692,691,688,687,686,685,684,683,
  682,537,535,534,533,532,531,530,529,528,527,526,525,524,521,518,517,491,
  460,429,231,135,134,1,0,51,52,
704,702,701,700,699,698,697,696,695,694,693,692,691,688,687,686,685,684,683,
  682,537,535,534,533,532,531,530,529,528,527,526,525,524,521,518,517,491,
  460,429,231,135,134,1,0,51,52,
704,702,701,700,699,698,697,696,695,694,693,692,691,688,687,686,685,684,683,
  682,537,535,534,533,532,531,530,529,528,527,526,525,524,521,518,517,491,
  460,429,231,135,134,1,0,51,52,
704,702,701,700,699,698,697,696,695,694,693,692,691,688,687,686,685,684,683,
  682,537,535,534,533,532,531,530,529,528,527,526,525,524,521,518,517,491,
  460,429,231,135,134,1,0,51,52,
704,702,701,700,699,698,697,696,695,694,693,692,691,688,687,686,685,684,683,
  682,537,535,534,533,532,531,530,529,528,527,526,525,524,521,518,517,491,
  460,429,231,135,134,1,0,51,52,
704,702,701,700,699,698,697,696,695,694,693,692,691,688,687,686,685,684,683,
  682,537,535,534,533,532,531,530,529,528,527,526,525,524,521,518,517,491,
  460,429,231,135,134,1,0,51,52,
704,702,701,700,699,698,697,696,695,694,693,692,691,688,687,686,685,684,683,
  682,537,535,534,533,532,531,530,529,528,527,526,525,524,521,518,517,491,
  460,429,231,135,134,1,0,51,52,
704,702,701,700,699,698,697,696,695,694,693,692,691,688,687,686,685,684,683,
  682,537,535,534,533,532,531,530,529,528,527,526,525,524,521,518,517,491,
  460,429,231,135,134,1,0,51,52,
704,702,701,700,699,698,697,696,695,694,693,692,691,688,687,686,685,684,683,
  682,537,535,534,533,532,531,530,529,528,527,526,525,524,521,518,517,491,
  460,429,231,135,134,1,0,51,52,
704,702,701,700,699,698,697,696,695,694,693,692,691,688,687,686,685,684,683,
  682,537,535,534,533,532,531,530,529,528,527,526,525,524,521,518,517,491,
  460,429,231,135,134,1,0,51,52,
704,702,701,700,699,698,697,696,695,694,693,692,691,688,687,686,685,684,683,
  682,537,535,534,533,532,531,530,529,528,527,526,525,524,521,518,517,491,
  460,429,231,135,134,1,0,51,52,
704,702,701,700,699,698,697,696,695,694,693,692,691,688,687,686,685,684,683,
  682,537,535,534,533,532,531,530,529,528,527,526,525,524,521,518,517,491,
  460,429,231,135,134,1,0,51,52,
704,702,701,700,699,698,697,696,695,694,693,692,691,688,687,686,685,684,683,
  682,537,535,534,533,532,531,530,529,528,527,526,525,524,521,518,517,491,
  460,429,231,135,134,1,0,51,52,
704,702,701,700,699,698,697,696,695,694,693,692,691,688,687,686,685,684,683,
  682,537,535,534,533,532,531,530,529,528,527,526,525,524,521,518,517,491,
  460,429,231,135,134,1,0,51,52,
704,702,701,700,699,698,697,696,695,694,693,692,691,688,687,686,685,684,683,
  682,537,535,534,533,532,531,530,529,528,527,526,525,524,521,518,517,491,
  460,429,231,135,134,1,0,51,52,
704,702,701,700,699,698,697,696,695,694,693,692,691,688,687,686,685,684,683,
  682,537,535,534,533,532,531,530,529,528,527,526,525,524,521,518,517,491,
  460,429,231,135,134,1,0,51,52,
704,702,701,700,699,698,697,696,695,694,693,692,691,688,687,686,685,684,683,
  682,537,535,534,533,532,531,530,529,528,527,526,525,524,521,518,517,491,
  460,429,231,135,134,1,0,51,52,
704,702,701,700,699,698,697,696,695,694,693,692,691,688,687,686,685,684,683,
  682,537,535,534,533,532,531,530,529,528,527,526,525,524,521,518,517,491,
  460,429,231,135,134,1,0,51,52,
704,702,701,700,699,698,697,696,695,694,693,692,691,688,687,686,685,684,683,
  682,537,535,534,533,532,531,530,529,528,527,526,525,524,521,518,517,491,
  460,429,231,135,134,1,0,51,52,
704,702,701,700,699,698,697,696,695,694,693,692,691,688,687,686,685,684,683,
  682,537,535,534,533,532,531,530,529,528,527,526,525,524,521,518,517,491,
  460,429,231,135,134,1,0,51,52,
704,702,701,700,699,698,697,696,695,694,693,692,691,688,687,686,685,684,683,
  682,537,535,534,533,532,531,530,529,528,527,526,525,524,521,518,517,491,
  460,429,231,135,134,1,0,51,52,
668,540,539,248,246,238,236,234,1,0,51,52,
668,540,539,248,246,238,236,234,1,0,51,52,
668,540,539,248,246,238,236,234,1,0,51,52,
668,540,539,248,246,238,236,234,1,0,51,52,
668,246,236,234,1,0,51,52,
668,246,236,234,1,0,51,52,
668,543,248,246,238,236,234,1,0,51,52,
668,543,248,246,238,236,234,1,0,51,52,
668,543,248,246,238,236,234,1,0,51,52,
668,543,542,541,248,246,238,236,234,1,0,51,52,
668,543,542,541,248,246,238,236,234,1,0,51,52,
540,241,240,239,238,237,236,235,234,1,0,51,52,
540,241,240,239,238,237,236,235,234,1,0,51,52,
540,241,240,239,238,237,236,235,234,1,0,51,52,
540,241,240,239,238,237,236,235,234,1,0,51,52,
540,241,240,239,238,237,236,235,234,1,0,51,52,
540,241,240,239,238,237,236,235,234,1,0,51,52,
540,241,240,239,238,237,236,235,234,1,0,51,52,
540,241,240,239,238,237,236,235,234,1,0,51,52,
540,241,240,239,238,237,236,235,234,1,0,51,52,
540,241,240,239,238,237,236,235,234,1,0,51,52,
540,241,240,239,238,237,236,235,234,1,0,51,52,
705,704,702,701,700,699,698,697,696,695,694,693,692,691,690,689,688,687,686,
  685,684,683,682,681,680,544,526,524,523,521,520,519,518,517,515,513,511,
  509,507,506,504,502,500,498,496,493,460,459,455,432,429,427,425,424,423,
  38,1,0,51,52,138,
544,427,425,424,423,38,1,0,51,52,
705,704,702,701,700,699,698,697,696,695,694,693,692,691,690,689,688,687,686,
  685,684,683,682,681,680,544,526,524,523,522,521,520,519,518,517,516,515,
  514,513,512,511,510,509,508,507,506,505,504,503,502,501,500,499,498,497,
  496,495,493,460,459,455,432,429,427,425,424,423,38,1,0,91,
427,425,424,423,38,1,0,51,
544,0,267,
704,702,701,700,699,698,697,696,695,694,693,692,691,688,687,686,685,684,683,
  682,537,535,534,533,532,531,530,529,528,527,526,525,524,521,518,517,491,
  460,429,231,135,134,0,2,6,7,8,9,15,17,21,23,24,25,26,27,28,29,57,77,89,
  91,131,133,166,196,198,203,204,205,206,207,208,209,210,211,212,213,214,
  215,216,217,218,219,442,492,538,
491,490,489,488,487,486,484,483,482,481,480,479,478,477,476,475,474,473,472,
  471,470,469,468,467,466,465,464,463,462,456,454,453,445,444,443,441,439,
  0,69,70,71,72,73,83,84,85,98,99,100,101,102,103,104,105,106,107,108,109,
  110,111,112,113,114,115,117,118,119,120,121,122,123,124,125,126,127,128,
  129,130,131,
427,425,424,423,38,1,0,51,52,
705,704,702,701,700,699,698,697,696,695,694,693,692,691,690,689,688,687,686,
  685,684,683,682,681,680,526,524,521,520,519,518,517,511,509,507,460,459,
  455,429,425,1,0,51,52,
427,425,424,423,38,1,0,51,52,
681,0,10,12,435,
681,0,10,12,435,
704,702,701,700,699,698,697,696,695,694,693,692,691,688,687,686,685,684,683,
  682,681,537,535,534,533,532,531,530,529,528,527,526,525,524,521,518,517,
  491,460,429,231,135,134,0,2,6,7,8,9,10,12,15,17,21,23,24,25,26,27,28,29,
  57,89,91,131,133,196,198,203,204,205,206,207,208,209,210,211,212,213,
  214,215,216,217,218,219,435,442,492,538,
704,703,702,701,700,699,698,697,696,695,694,693,692,691,690,689,688,687,686,
  685,684,683,682,681,680,526,524,521,520,519,518,517,511,509,507,460,459,
  455,429,425,423,1,0,142,
493,459,455,427,425,424,423,38,1,0,51,52,
427,425,424,423,38,1,0,51,52,
427,425,424,423,38,0,39,40,41,45,46,47,49,
427,425,424,423,38,0,39,40,41,45,46,47,49,
427,425,424,423,38,1,0,51,52,
688,687,686,685,684,0,
687,686,685,684,0,
685,684,0,
704,702,701,700,699,698,697,696,695,694,693,691,688,687,686,685,684,682,544,
  523,522,521,520,519,518,517,516,515,514,513,512,511,510,509,508,507,506,
  505,504,503,502,501,500,499,498,497,496,495,493,460,459,432,429,427,425,
  424,423,38,1,0,
704,702,698,696,695,694,693,688,687,686,685,684,682,544,523,522,521,520,519,
  518,517,516,515,514,513,512,511,510,509,508,507,506,505,504,503,502,501,
  500,499,498,497,496,495,493,459,432,429,427,425,424,423,222,221,38,1,0,
688,687,686,685,684,0,
704,703,702,701,700,699,698,697,696,695,694,693,692,691,690,689,688,687,686,
  685,684,682,681,680,526,524,521,520,519,518,517,511,509,507,460,459,455,
  429,425,423,233,1,0,16,
688,687,686,685,684,0,
698,696,695,694,693,688,687,686,685,684,0,
687,686,685,684,544,523,522,521,520,519,518,517,516,515,514,513,512,511,510,
  509,508,507,506,505,504,503,502,501,500,499,498,497,496,495,493,459,432,
  427,425,424,423,38,1,0,
685,684,544,523,522,521,520,519,518,517,516,515,514,513,512,511,510,509,508,
  507,506,505,504,503,502,501,500,499,498,497,496,495,493,459,432,427,425,
  424,423,38,1,0,
696,695,694,693,688,687,686,685,684,544,523,522,521,520,519,518,517,516,515,
  514,513,512,511,510,509,508,507,506,505,504,503,502,501,500,499,498,497,
  496,495,493,459,432,427,425,424,423,38,1,0,
704,698,696,695,694,693,688,687,686,685,684,682,544,523,522,521,520,519,518,
  517,516,515,514,513,511,510,509,508,507,506,505,504,503,502,501,500,499,
  498,497,496,493,459,432,429,427,425,424,423,222,221,38,1,0,223,
460,1,0,51,52,
460,427,425,424,423,38,1,0,51,52,
460,1,0,51,52,
460,1,0,51,52,
460,1,0,51,52,
460,1,0,51,52,
460,1,0,51,52,
460,1,0,51,52,
460,1,0,51,52,
460,1,0,51,52,
460,1,0,51,52,
544,523,522,521,520,519,518,517,516,515,514,513,512,511,510,509,508,507,506,
  505,504,503,502,501,500,499,498,497,496,495,493,459,432,427,425,424,423,
  38,1,0,51,52,
544,523,522,521,520,519,518,517,516,515,514,513,512,511,510,509,508,507,506,
  505,504,503,502,501,500,499,498,497,496,495,493,459,432,427,425,424,423,
  38,1,0,51,52,
705,704,703,702,701,700,699,698,697,696,695,694,693,692,691,690,689,688,687,
  686,685,684,683,682,681,680,537,535,534,533,532,531,530,529,528,527,526,
  525,524,521,520,519,518,517,511,509,507,491,461,460,459,455,429,427,426,
  425,424,423,231,135,134,38,1,0,51,52,
460,1,0,51,52,
460,1,0,51,52,
460,1,0,51,52,
460,1,0,51,52,
460,1,0,51,52,
460,1,0,51,52,
460,1,0,51,52,
460,1,0,51,52,
460,1,0,51,52,
460,1,0,51,52,
460,1,0,51,52,
704,702,701,700,699,698,697,696,695,694,693,692,691,688,687,686,685,684,683,
  682,537,535,534,533,532,531,530,529,528,527,521,518,517,491,460,429,231,
  135,134,1,0,51,52,
705,704,703,702,701,700,699,698,697,696,695,694,693,692,691,690,689,688,687,
  686,685,684,683,682,681,680,537,535,534,533,532,531,530,529,528,527,526,
  524,521,520,519,518,517,511,509,507,491,461,460,459,455,429,427,426,425,
  424,423,231,135,134,38,1,0,51,52,
704,702,701,700,699,698,697,696,695,694,693,692,691,688,687,686,685,684,683,
  682,537,535,534,533,532,531,530,529,528,527,521,518,517,491,460,429,231,
  135,134,1,0,51,52,
705,704,703,702,701,700,699,698,697,696,695,694,693,692,691,690,689,688,687,
  686,685,684,683,682,681,680,537,535,534,533,532,531,530,529,528,527,526,
  524,521,520,519,518,517,511,509,507,491,461,460,459,455,429,427,426,425,
  424,423,231,135,134,38,1,0,51,52,
704,702,701,700,699,698,697,696,695,694,693,692,691,688,687,686,685,684,683,
  682,537,535,534,533,532,531,530,529,528,527,526,525,524,521,518,517,491,
  460,429,231,135,134,1,0,51,52,
704,702,701,700,699,698,697,696,695,694,693,692,691,688,687,686,685,684,683,
  682,537,535,534,533,532,531,530,529,528,527,521,518,517,491,460,429,231,
  135,134,0,2,6,7,8,9,15,17,21,89,91,131,133,198,206,207,208,209,210,211,
  212,213,214,215,216,217,218,219,442,492,538,
704,702,701,700,699,698,697,696,695,694,693,692,691,688,687,686,685,684,683,
  682,537,535,534,533,532,531,530,529,528,527,521,518,517,491,460,429,231,
  135,134,0,2,6,7,8,9,15,17,21,89,91,131,133,198,206,207,208,209,210,211,
  212,213,214,215,216,217,218,219,442,492,538,
704,702,701,700,699,698,697,696,695,694,693,692,691,688,687,686,685,684,683,
  682,537,535,534,533,532,531,530,529,528,527,521,518,517,491,460,429,231,
  135,134,0,2,6,7,8,9,15,17,21,89,91,131,133,198,206,207,208,209,210,211,
  212,213,214,215,216,217,218,219,442,492,538,
704,702,701,700,699,698,697,696,695,694,693,692,691,688,687,686,685,684,683,
  682,537,535,534,533,532,531,530,529,528,527,521,518,517,491,460,429,231,
  135,134,0,2,6,7,8,9,15,17,21,89,91,131,133,198,206,207,208,209,210,211,
  212,213,214,215,216,217,218,219,442,492,538,
544,523,522,521,520,519,518,517,516,515,514,513,512,511,510,509,508,507,506,
  505,504,503,502,501,500,499,498,497,496,495,493,459,432,427,425,424,423,
  38,1,0,51,52,
544,523,522,521,520,519,518,517,516,515,514,513,512,511,510,509,508,507,506,
  505,504,503,502,501,500,499,498,497,496,495,493,459,432,425,423,38,1,0,
  51,52,197,199,200,201,202,
544,518,517,516,515,514,513,512,511,510,509,508,507,506,505,504,503,502,501,
  500,499,498,497,496,495,493,459,432,427,425,424,423,38,1,0,51,52,195,
  196,
544,516,515,514,513,512,511,510,509,508,507,506,505,504,503,502,501,500,499,
  498,497,496,495,493,459,432,427,425,424,423,38,1,0,51,52,191,192,193,
  194,
544,512,511,510,509,508,507,506,505,504,503,502,501,500,499,498,497,496,495,
  493,459,432,427,425,424,423,38,1,0,51,52,189,190,
544,510,509,508,507,506,505,504,503,502,501,500,499,498,497,496,495,493,459,
  432,427,425,424,423,38,1,0,51,52,187,188,
508,507,0,185,186,
506,505,504,503,502,501,500,499,498,497,496,495,432,427,425,424,423,38,1,0,
  59,167,168,169,170,171,172,173,174,175,176,177,178,179,180,181,182,183,
  184,
704,702,701,700,699,698,697,696,695,694,693,692,691,688,687,686,685,684,683,
  682,537,535,534,533,532,531,530,529,528,527,526,525,524,521,518,517,491,
  460,429,231,135,134,0,2,6,7,8,9,15,17,21,23,24,25,26,27,28,29,57,89,91,
  131,133,196,198,203,204,205,206,207,208,209,210,211,212,213,214,215,216,
  217,218,219,442,492,538,
427,425,424,423,38,1,0,51,52,
427,425,424,423,38,1,0,51,52,
427,425,424,423,38,1,0,75,
427,425,424,423,38,1,0,51,52,
427,425,424,423,38,1,0,51,52,
427,425,424,423,38,1,0,51,52,
427,425,424,423,38,1,0,51,52,
704,702,701,700,699,698,697,696,695,694,693,691,682,135,134,1,0,51,52,
704,702,701,700,699,698,697,696,695,694,693,691,682,135,134,1,0,51,52,
427,425,424,423,38,1,0,51,52,
427,425,424,423,38,1,0,51,52,
427,425,424,423,38,1,0,51,52,
427,425,424,423,38,1,0,51,52,
705,704,703,702,701,700,699,698,697,696,695,694,693,692,691,690,689,688,687,
  686,685,684,683,682,680,526,524,521,520,519,518,517,511,509,507,460,459,
  455,429,425,1,0,
427,425,424,423,38,1,0,51,52,
427,425,424,423,38,1,0,51,52,
427,425,424,423,38,1,0,51,52,
704,703,702,701,700,699,698,697,696,695,694,693,689,688,687,686,685,684,681,
  520,429,1,0,51,52,
704,703,702,701,700,699,698,697,696,695,694,693,689,688,687,686,685,684,681,
  520,429,0,10,12,13,14,19,435,436,437,
705,704,703,702,701,700,699,698,697,696,695,694,693,692,691,690,689,688,687,
  686,685,684,683,682,681,680,526,524,521,520,519,518,517,511,509,507,461,
  460,459,455,429,427,426,425,424,423,38,1,0,51,52,
705,704,703,702,701,700,699,698,697,696,695,694,693,692,691,690,689,688,687,
  686,685,684,683,682,681,680,537,535,534,533,532,531,530,529,528,527,526,
  525,524,521,520,519,518,517,511,509,507,491,461,460,459,455,429,427,426,
  425,424,423,231,135,134,38,1,0,51,52,
705,704,703,702,701,700,699,698,697,696,695,694,693,692,691,690,689,688,687,
  686,685,684,683,682,681,680,526,524,521,520,519,518,517,511,509,507,461,
  460,459,455,429,427,426,425,424,423,38,1,0,51,52,
705,704,703,702,701,700,699,698,697,696,695,694,693,692,691,690,689,688,687,
  686,685,684,683,682,681,680,537,535,534,533,532,531,530,529,528,527,526,
  525,524,521,520,519,518,517,511,509,507,491,461,460,459,455,429,427,426,
  425,424,423,231,135,134,38,1,0,51,52,
705,704,703,702,701,700,699,698,697,696,695,694,693,692,691,690,689,688,687,
  686,685,684,683,682,681,680,526,524,521,520,519,518,517,511,509,507,460,
  459,455,429,427,426,425,423,1,0,51,52,
705,704,703,702,701,700,699,698,697,696,695,694,693,692,691,690,689,688,687,
  686,685,684,683,682,681,680,526,524,521,520,519,518,517,511,509,507,461,
  460,459,455,429,427,426,425,424,423,38,1,0,51,52,
705,704,703,702,701,700,699,698,697,696,695,694,693,692,691,690,689,688,687,
  686,685,684,683,682,681,680,526,524,521,520,519,518,517,511,509,507,461,
  460,459,455,429,427,426,425,424,423,38,1,0,51,52,
705,704,703,702,701,700,699,698,697,696,695,694,693,692,691,690,689,688,687,
  686,685,684,683,682,681,680,526,524,521,520,519,518,517,511,509,507,461,
  460,459,455,429,427,426,425,424,423,38,1,0,51,52,
705,704,703,702,701,700,699,698,697,696,695,694,693,692,691,690,689,688,687,
  686,685,684,683,682,681,680,526,524,521,520,519,518,517,511,509,507,461,
  460,459,455,429,427,426,425,424,423,38,1,0,51,52,
705,704,703,702,701,700,699,698,697,696,695,694,693,692,691,690,689,688,687,
  686,685,684,683,682,681,680,526,524,521,520,519,518,517,511,509,507,461,
  460,459,455,429,427,426,425,424,423,38,1,0,51,52,
705,704,703,702,701,700,699,698,697,696,695,694,693,692,691,690,689,688,687,
  686,685,684,683,682,681,680,526,524,521,520,519,518,517,511,509,507,461,
  460,459,455,429,427,426,425,424,423,38,1,0,51,52,
705,704,703,702,701,700,699,698,697,696,695,694,693,692,691,690,689,688,687,
  686,685,684,683,682,681,680,526,524,521,520,519,518,517,511,509,507,461,
  460,459,455,429,427,426,425,424,423,38,1,0,51,52,
705,704,703,702,701,700,699,698,697,696,695,694,693,692,691,690,689,688,687,
  686,685,684,683,682,681,680,526,524,521,520,519,518,517,511,509,507,461,
  460,459,455,429,427,426,425,424,423,38,1,0,51,52,
705,704,703,702,701,700,699,698,697,696,695,694,693,692,691,690,689,688,687,
  686,685,684,683,682,681,680,526,524,521,520,519,518,517,511,509,507,461,
  460,459,455,429,427,426,425,424,423,38,1,0,51,52,
705,704,703,702,701,700,699,698,697,696,695,694,693,692,691,690,689,688,687,
  686,685,684,683,682,681,680,526,524,521,520,519,518,517,511,509,507,461,
  460,459,455,429,427,426,425,424,423,38,1,0,51,52,
705,704,703,702,701,700,699,698,697,696,695,694,693,692,691,690,689,688,687,
  686,685,684,683,682,681,680,526,524,521,520,519,518,517,511,509,507,461,
  460,459,455,429,427,426,425,424,423,38,1,0,51,52,
705,704,703,702,701,700,699,698,697,696,695,694,693,692,691,690,689,688,687,
  686,685,684,683,682,681,680,526,524,521,520,519,518,517,511,509,507,461,
  460,459,455,429,427,426,425,424,423,38,1,0,51,52,
705,704,703,702,701,700,699,698,697,696,695,694,693,692,691,690,689,688,687,
  686,685,684,683,682,681,680,526,524,521,520,519,518,517,511,509,507,461,
  460,459,455,429,427,426,425,424,423,38,1,0,51,52,
705,704,703,702,701,700,699,698,697,696,695,694,693,692,691,690,689,688,687,
  686,685,684,683,682,681,680,526,524,521,520,519,518,517,511,509,507,461,
  460,459,455,429,427,426,425,424,423,38,1,0,51,52,
705,704,703,702,701,700,699,698,697,696,695,694,693,692,691,690,689,688,687,
  686,685,684,683,682,681,680,526,524,521,520,519,518,517,511,509,507,461,
  460,459,455,429,427,426,425,424,423,38,1,0,51,52,
705,704,703,702,701,700,699,698,697,696,695,694,693,692,691,690,689,688,687,
  686,685,684,683,682,681,680,526,524,521,520,519,518,517,511,509,507,461,
  460,459,455,429,427,426,425,424,423,38,1,0,51,52,
705,704,703,702,701,700,699,698,697,696,695,694,693,692,691,690,689,688,687,
  686,685,684,683,682,681,680,526,524,521,520,519,518,517,511,509,507,461,
  460,459,455,429,427,426,425,424,423,38,1,0,51,52,
705,704,703,702,701,700,699,698,697,696,695,694,693,692,691,690,689,688,687,
  686,685,684,683,682,681,680,526,524,521,520,519,518,517,511,509,507,461,
  460,459,455,429,427,426,425,424,423,38,1,0,51,52,
705,704,703,702,701,700,699,698,697,696,695,694,693,692,691,690,689,688,687,
  686,685,684,683,682,681,680,526,524,521,520,519,518,517,511,509,507,461,
  460,459,455,429,427,426,425,424,423,38,1,0,51,52,
705,704,703,702,701,700,699,698,697,696,695,694,693,692,691,690,689,688,687,
  686,685,684,683,682,681,680,537,535,534,533,532,531,530,529,528,527,526,
  524,521,520,519,518,517,511,509,507,491,461,460,459,455,429,427,426,425,
  424,423,231,135,134,38,1,0,51,52,
705,704,703,702,701,700,699,698,697,696,695,694,693,692,691,690,689,688,687,
  686,685,684,683,682,681,680,537,535,534,533,532,531,530,529,528,527,526,
  525,524,521,520,519,518,517,511,509,507,491,461,460,459,455,429,427,426,
  425,424,423,231,135,134,38,1,0,51,52,
705,704,703,702,701,700,699,698,697,696,695,694,693,692,691,690,689,688,687,
  686,685,684,683,682,681,680,537,535,534,533,532,531,530,529,528,527,526,
  525,524,521,520,519,518,517,511,509,507,491,461,460,459,455,429,427,426,
  425,424,423,231,135,134,38,1,0,51,52,
705,704,703,702,701,700,699,698,697,696,695,694,693,692,691,690,689,688,687,
  686,685,684,683,682,681,680,537,535,534,533,532,531,530,529,528,527,526,
  525,524,521,520,519,518,517,511,509,507,491,461,460,459,455,429,427,426,
  425,424,423,231,135,134,38,1,0,51,52,
705,704,703,702,701,700,699,698,697,696,695,694,693,692,691,690,689,688,687,
  686,685,684,683,682,681,680,537,535,534,533,532,531,530,529,528,527,526,
  524,521,520,519,518,517,511,509,507,491,461,460,459,455,429,427,426,425,
  424,423,231,135,134,38,1,0,51,52,
705,704,703,702,701,700,699,698,697,696,695,694,693,692,691,690,689,688,687,
  686,685,684,683,682,681,680,544,526,524,523,522,521,520,519,518,517,516,
  515,514,513,512,511,510,509,508,507,506,505,504,503,502,501,500,499,498,
  497,496,495,493,461,460,459,455,432,429,427,426,425,424,423,38,1,0,51,
  52,
705,704,703,702,701,700,699,698,697,696,695,694,693,692,691,690,689,688,687,
  686,685,684,683,682,681,680,526,524,521,520,519,518,517,511,509,507,461,
  460,459,455,429,427,426,425,424,423,38,1,0,51,52,
705,704,703,702,701,700,699,698,697,696,695,694,693,692,691,690,689,688,687,
  686,685,684,683,682,681,680,537,535,534,533,532,531,530,529,528,527,526,
  525,524,521,520,519,518,517,511,509,507,491,461,460,459,455,429,427,426,
  425,424,423,231,135,134,38,1,0,51,52,
705,704,703,702,701,700,699,698,697,696,695,694,693,692,691,690,689,688,687,
  686,685,684,683,682,681,680,537,535,534,533,532,531,530,529,528,527,526,
  524,521,520,519,518,517,511,509,507,491,461,460,459,455,429,427,426,425,
  424,423,231,135,134,38,1,0,51,52,
705,704,703,702,701,700,699,698,697,696,695,694,693,692,691,690,689,688,687,
  686,685,684,683,682,681,680,526,524,521,520,519,518,517,511,509,507,461,
  460,459,455,429,427,426,425,424,423,38,1,0,51,52,
705,704,703,702,701,700,699,698,697,696,695,694,693,692,691,690,689,688,687,
  686,685,684,683,682,681,680,526,524,521,520,519,518,517,511,509,507,461,
  460,459,455,429,427,426,425,424,423,38,1,0,51,52,
705,704,703,702,701,700,699,698,697,696,695,694,693,692,691,690,689,688,687,
  686,685,684,683,682,681,680,526,524,521,520,519,518,517,511,509,507,461,
  460,459,455,429,427,426,425,424,423,38,1,0,51,52,
705,704,703,702,701,700,699,698,697,696,695,694,693,692,691,690,689,688,687,
  686,685,684,683,682,681,680,526,524,521,520,519,518,517,511,509,507,460,
  459,455,438,434,433,429,427,426,425,424,423,164,163,162,135,134,38,1,0,
  51,52,
705,704,703,702,701,700,699,698,697,696,695,694,693,692,691,690,689,688,687,
  686,685,684,683,682,681,680,526,524,521,520,519,518,517,511,509,507,460,
  459,455,429,425,1,0,46,54,86,91,93,140,141,143,145,146,147,148,149,150,
  154,157,158,185,187,189,195,196,197,199,200,203,205,220,224,225,229,413,
  414,415,416,417,418,419,420,421,422,
423,0,45,
423,0,45,
704,702,701,700,699,698,697,696,695,694,693,692,691,688,687,686,685,684,683,
  682,537,535,534,533,532,531,530,529,528,527,526,525,524,521,518,517,491,
  460,429,231,135,134,1,0,51,52,
704,702,701,700,699,698,697,696,695,694,693,692,691,688,687,686,685,684,683,
  682,537,535,534,533,532,531,530,529,528,527,526,525,524,521,518,517,491,
  460,429,231,135,134,0,2,6,7,8,9,15,17,21,23,24,25,26,27,28,29,57,89,91,
  131,133,196,198,203,204,205,206,207,208,209,210,211,212,213,214,215,216,
  217,218,219,442,492,538,
491,490,489,488,487,486,484,483,482,481,480,479,478,477,476,475,474,473,472,
  471,470,469,468,467,466,465,464,463,462,456,454,453,445,444,443,441,439,
  431,430,0,56,58,69,70,71,72,73,83,84,85,98,99,100,101,102,103,104,105,
  106,107,108,109,110,111,112,113,114,115,117,118,119,120,121,122,123,124,
  125,126,127,128,129,130,131,
427,425,424,423,38,1,0,51,52,
494,491,427,425,424,423,38,0,22,39,40,41,45,46,47,49,131,165,
668,543,542,541,248,246,238,236,234,0,245,247,249,250,254,255,256,
540,241,240,239,238,237,236,235,234,0,4,242,594,
668,543,248,246,238,236,234,0,5,245,247,249,608,
687,686,685,684,0,345,
455,0,86,
455,0,86,
455,0,86,
704,702,701,700,699,698,697,696,695,694,693,692,691,688,687,686,685,684,683,
  682,537,535,534,533,532,531,530,529,528,527,526,525,524,521,518,517,491,
  460,429,231,135,134,0,2,6,7,8,9,15,17,21,23,24,25,26,27,28,29,57,89,91,
  131,133,196,198,203,204,205,206,207,208,209,210,211,212,213,214,215,216,
  217,218,219,442,492,538,
704,702,701,700,699,698,697,696,695,694,693,692,691,688,687,686,685,684,683,
  682,537,535,534,533,532,531,530,529,528,527,526,525,524,521,518,517,491,
  460,429,231,135,134,0,2,6,7,8,9,15,17,21,23,24,25,26,27,28,29,57,89,91,
  131,133,196,198,203,204,205,206,207,208,209,210,211,212,213,214,215,216,
  217,218,219,442,492,538,
704,702,701,700,699,698,697,696,695,694,693,692,691,688,687,686,685,684,683,
  682,537,535,534,533,532,531,530,529,528,527,526,525,524,521,518,517,491,
  460,429,231,135,134,0,2,6,7,8,9,15,17,21,23,24,25,26,27,28,29,57,89,91,
  131,133,196,198,203,204,205,206,207,208,209,210,211,212,213,214,215,216,
  217,218,219,442,492,538,
704,702,701,700,699,698,697,696,695,694,693,692,691,688,687,686,685,684,683,
  682,537,535,534,533,532,531,530,529,528,527,526,525,524,521,518,517,491,
  460,429,231,135,134,0,2,6,7,8,9,15,17,21,23,24,25,26,27,28,29,57,89,91,
  131,133,196,198,203,204,205,206,207,208,209,210,211,212,213,214,215,216,
  217,218,219,442,492,538,
704,702,701,700,699,698,697,696,695,694,693,692,691,688,687,686,685,684,683,
  682,537,535,534,533,532,531,530,529,528,527,526,525,524,521,518,517,491,
  460,429,231,135,134,0,2,6,7,8,9,15,17,21,23,24,25,26,27,28,29,57,89,91,
  131,133,196,198,203,204,205,206,207,208,209,210,211,212,213,214,215,216,
  217,218,219,442,492,538,
704,702,701,700,699,698,697,696,695,694,693,692,691,688,687,686,685,684,683,
  682,537,535,534,533,532,531,530,529,528,527,526,525,524,521,518,517,491,
  460,429,231,135,134,0,2,6,7,8,9,15,17,21,23,24,25,26,27,28,29,57,89,91,
  131,133,196,198,203,204,205,206,207,208,209,210,211,212,213,214,215,216,
  217,218,219,442,492,538,
704,702,701,700,699,698,697,696,695,694,693,692,691,688,687,686,685,684,683,
  682,537,535,534,533,532,531,530,529,528,527,526,525,524,521,518,517,491,
  460,429,231,135,134,0,2,6,7,8,9,15,17,21,23,24,25,26,27,28,29,57,89,91,
  131,133,196,198,203,204,205,206,207,208,209,210,211,212,213,214,215,216,
  217,218,219,442,492,538,
704,702,701,700,699,698,697,696,695,694,693,692,691,688,687,686,685,684,683,
  682,537,535,534,533,532,531,530,529,528,527,526,525,524,521,518,517,491,
  460,429,231,135,134,0,2,6,7,8,9,15,17,21,23,24,25,26,27,28,29,57,89,91,
  131,133,196,198,203,204,205,206,207,208,209,210,211,212,213,214,215,216,
  217,218,219,442,492,538,
704,702,701,700,699,698,697,696,695,694,693,692,691,688,687,686,685,684,683,
  682,537,535,534,533,532,531,530,529,528,527,526,525,524,521,518,517,491,
  460,429,231,135,134,0,2,6,7,8,9,15,17,21,23,24,25,26,27,28,29,57,89,91,
  131,133,196,198,203,204,205,206,207,208,209,210,211,212,213,214,215,216,
  217,218,219,442,492,538,
704,702,701,700,699,698,697,696,695,694,693,692,691,688,687,686,685,684,683,
  682,537,535,534,533,532,531,530,529,528,527,526,525,524,521,518,517,491,
  460,429,231,135,134,0,2,6,7,8,9,15,17,21,23,24,25,26,27,28,29,57,89,91,
  131,133,196,198,203,204,205,206,207,208,209,210,211,212,213,214,215,216,
  217,218,219,442,492,538,
704,702,701,700,699,698,697,696,695,694,693,692,691,688,687,686,685,684,683,
  682,537,535,534,533,532,531,530,529,528,527,526,525,524,521,518,517,491,
  460,429,231,135,134,0,2,6,7,8,9,15,17,21,23,24,25,26,27,28,29,57,89,91,
  131,133,196,198,203,204,205,206,207,208,209,210,211,212,213,214,215,216,
  217,218,219,442,492,538,
704,702,701,700,699,698,697,696,695,694,693,692,691,688,687,686,685,684,683,
  682,537,535,534,533,532,531,530,529,528,527,526,525,524,521,518,517,491,
  460,429,231,135,134,0,2,6,7,8,9,15,17,21,23,24,25,26,27,28,29,57,89,91,
  131,133,196,198,203,204,205,206,207,208,209,210,211,212,213,214,215,216,
  217,218,219,442,492,538,
704,702,701,700,699,698,697,696,695,694,693,692,691,688,687,686,685,684,683,
  682,537,535,534,533,532,531,530,529,528,527,526,525,524,521,518,517,491,
  460,429,231,135,134,0,2,6,7,8,9,15,17,21,23,24,25,26,27,28,29,57,89,91,
  131,133,196,198,203,204,205,206,207,208,209,210,211,212,213,214,215,216,
  217,218,219,442,492,538,
704,702,701,700,699,698,697,696,695,694,693,692,691,688,687,686,685,684,683,
  682,537,535,534,533,532,531,530,529,528,527,526,525,524,521,518,517,491,
  460,429,231,135,134,0,2,6,7,8,9,15,17,21,23,24,25,26,27,28,29,57,89,91,
  131,133,196,198,203,204,205,206,207,208,209,210,211,212,213,214,215,216,
  217,218,219,442,492,538,
704,702,701,700,699,698,697,696,695,694,693,692,691,688,687,686,685,684,683,
  682,537,535,534,533,532,531,530,529,528,527,526,525,524,521,518,517,491,
  460,429,231,135,134,0,2,6,7,8,9,15,17,21,23,24,25,26,27,28,29,57,89,91,
  131,133,196,198,203,204,205,206,207,208,209,210,211,212,213,214,215,216,
  217,218,219,442,492,538,
704,702,701,700,699,698,697,696,695,694,693,692,691,688,687,686,685,684,683,
  682,537,535,534,533,532,531,530,529,528,527,526,525,524,521,518,517,491,
  460,429,231,135,134,0,2,6,7,8,9,15,17,21,23,24,25,26,27,28,29,57,89,91,
  131,133,196,198,203,204,205,206,207,208,209,210,211,212,213,214,215,216,
  217,218,219,442,492,538,
704,702,701,700,699,698,697,696,695,694,693,692,691,688,687,686,685,684,683,
  682,537,535,534,533,532,531,530,529,528,527,526,525,524,521,518,517,491,
  460,429,231,135,134,0,2,6,7,8,9,15,17,21,23,24,25,26,27,28,29,57,89,91,
  131,133,196,198,203,204,205,206,207,208,209,210,211,212,213,214,215,216,
  217,218,219,442,492,538,
704,702,701,700,699,698,697,696,695,694,693,692,691,688,687,686,685,684,683,
  682,537,535,534,533,532,531,530,529,528,527,526,525,524,521,518,517,491,
  460,429,231,135,134,0,2,6,7,8,9,15,17,21,23,24,25,26,27,28,29,57,89,91,
  131,133,196,198,203,204,205,206,207,208,209,210,211,212,213,214,215,216,
  217,218,219,442,492,538,
704,702,701,700,699,698,697,696,695,694,693,692,691,688,687,686,685,684,683,
  682,537,535,534,533,532,531,530,529,528,527,526,525,524,521,518,517,491,
  460,429,231,135,134,0,2,6,7,8,9,15,17,21,23,24,25,26,27,28,29,57,89,91,
  131,133,196,198,203,204,205,206,207,208,209,210,211,212,213,214,215,216,
  217,218,219,442,492,538,
704,702,701,700,699,698,697,696,695,694,693,692,691,688,687,686,685,684,683,
  682,537,535,534,533,532,531,530,529,528,527,526,525,524,521,518,517,491,
  460,429,231,135,134,0,2,6,7,8,9,15,17,21,23,24,25,26,27,28,29,57,89,91,
  131,133,196,198,203,204,205,206,207,208,209,210,211,212,213,214,215,216,
  217,218,219,442,492,538,
704,702,701,700,699,698,697,696,695,694,693,692,691,688,687,686,685,684,683,
  682,537,535,534,533,532,531,530,529,528,527,526,525,524,521,518,517,491,
  460,429,231,135,134,0,2,6,7,8,9,15,17,21,23,24,25,26,27,28,29,57,89,91,
  131,133,196,198,203,204,205,206,207,208,209,210,211,212,213,214,215,216,
  217,218,219,442,492,538,
704,702,701,700,699,698,697,696,695,694,693,692,691,688,687,686,685,684,683,
  682,537,535,534,533,532,531,530,529,528,527,526,525,524,521,518,517,491,
  460,429,231,135,134,0,2,6,7,8,9,15,17,21,23,24,25,26,27,28,29,57,89,91,
  131,133,196,198,203,204,205,206,207,208,209,210,211,212,213,214,215,216,
  217,218,219,442,492,538,
704,702,701,700,699,698,697,696,695,694,693,692,691,688,687,686,685,684,683,
  682,537,535,534,533,532,531,530,529,528,527,526,525,524,521,518,517,491,
  460,429,231,135,134,0,2,6,7,8,9,15,17,21,23,24,25,26,27,28,29,57,89,91,
  131,133,196,198,203,204,205,206,207,208,209,210,211,212,213,214,215,216,
  217,218,219,442,492,538,
704,702,701,700,699,698,697,696,695,694,693,692,691,688,687,686,685,684,683,
  682,537,535,534,533,532,531,530,529,528,527,526,525,524,521,518,517,491,
  460,429,231,135,134,0,2,6,7,8,9,15,17,21,23,24,25,26,27,28,29,57,89,91,
  131,133,196,198,203,204,205,206,207,208,209,210,211,212,213,214,215,216,
  217,218,219,442,492,538,
704,702,701,700,699,698,697,696,695,694,693,692,691,688,687,686,685,684,683,
  682,537,535,534,533,532,531,530,529,528,527,526,525,524,521,518,517,491,
  460,429,231,135,134,0,2,6,7,8,9,15,17,21,23,24,25,26,27,28,29,57,89,91,
  131,133,196,198,203,204,205,206,207,208,209,210,211,212,213,214,215,216,
  217,218,219,442,492,538,
704,702,701,700,699,698,697,696,695,694,693,692,691,688,687,686,685,684,683,
  682,537,535,534,533,532,531,530,529,528,527,526,525,524,521,518,517,491,
  460,429,231,135,134,0,2,6,7,8,9,15,17,21,23,24,25,26,27,28,29,57,89,91,
  131,133,196,198,203,204,205,206,207,208,209,210,211,212,213,214,215,216,
  217,218,219,442,492,538,
704,702,701,700,699,698,697,696,695,694,693,692,691,688,687,686,685,684,683,
  682,537,535,534,533,532,531,530,529,528,527,526,525,524,521,518,517,491,
  460,429,231,135,134,0,2,6,7,8,9,15,17,21,23,24,25,26,27,28,29,57,89,91,
  131,133,196,198,203,204,205,206,207,208,209,210,211,212,213,214,215,216,
  217,218,219,442,492,538,
704,702,701,700,699,698,697,696,695,694,693,692,691,688,687,686,685,684,683,
  682,537,535,534,533,532,531,530,529,528,527,526,525,524,521,518,517,491,
  460,429,231,135,134,0,2,6,7,8,9,15,17,21,23,24,25,26,27,28,29,57,89,91,
  131,133,196,198,203,204,205,206,207,208,209,210,211,212,213,214,215,216,
  217,218,219,442,492,538,
704,702,701,700,699,698,697,696,695,694,693,692,691,688,687,686,685,684,683,
  682,537,535,534,533,532,531,530,529,528,527,526,525,524,521,518,517,491,
  460,429,231,135,134,0,2,6,7,8,9,15,17,21,23,24,25,26,27,28,29,57,89,91,
  131,133,196,198,203,204,205,206,207,208,209,210,211,212,213,214,215,216,
  217,218,219,442,492,538,
704,702,701,700,699,698,697,696,695,694,693,692,691,688,687,686,685,684,683,
  682,537,535,534,533,532,531,530,529,528,527,526,525,524,521,518,517,491,
  460,429,231,135,134,0,2,6,7,8,9,15,17,21,23,24,25,26,27,28,29,57,89,91,
  131,133,196,198,203,204,205,206,207,208,209,210,211,212,213,214,215,216,
  217,218,219,442,492,538,
704,702,701,700,699,698,697,696,695,694,693,692,691,688,687,686,685,684,683,
  682,537,535,534,533,532,531,530,529,528,527,526,525,524,521,518,517,491,
  460,429,231,135,134,0,2,6,7,8,9,15,17,21,23,24,25,26,27,28,29,57,89,91,
  131,133,196,198,203,204,205,206,207,208,209,210,211,212,213,214,215,216,
  217,218,219,442,492,538,
704,702,701,700,699,698,697,696,695,694,693,692,691,688,687,686,685,684,683,
  682,537,535,534,533,532,531,530,529,528,527,526,525,524,521,518,517,491,
  460,429,231,135,134,0,2,6,7,8,9,15,17,21,23,24,25,26,27,28,29,57,89,91,
  131,133,196,198,203,204,205,206,207,208,209,210,211,212,213,214,215,216,
  217,218,219,442,492,538,
704,702,701,700,699,698,697,696,695,694,693,692,691,688,687,686,685,684,683,
  682,537,535,534,533,532,531,530,529,528,527,526,525,524,521,518,517,491,
  460,429,231,135,134,0,2,6,7,8,9,15,17,21,23,24,25,26,27,28,29,57,89,91,
  131,133,196,198,203,204,205,206,207,208,209,210,211,212,213,214,215,216,
  217,218,219,442,492,538,
704,702,701,700,699,698,697,696,695,694,693,692,691,688,687,686,685,684,683,
  682,537,535,534,533,532,531,530,529,528,527,526,525,524,521,518,517,491,
  460,429,231,135,134,0,2,6,7,8,9,15,17,21,23,24,25,26,27,28,29,57,89,91,
  131,133,196,198,203,204,205,206,207,208,209,210,211,212,213,214,215,216,
  217,218,219,442,492,538,
704,702,701,700,699,698,697,696,695,694,693,692,691,688,687,686,685,684,683,
  682,537,535,534,533,532,531,530,529,528,527,526,525,524,521,518,517,491,
  460,429,231,135,134,0,2,6,7,8,9,15,17,21,23,24,25,26,27,28,29,57,89,91,
  131,133,196,198,203,204,205,206,207,208,209,210,211,212,213,214,215,216,
  217,218,219,442,492,538,
704,702,701,700,699,698,697,696,695,694,693,692,691,688,687,686,685,684,683,
  682,537,535,534,533,532,531,530,529,528,527,526,525,524,521,518,517,491,
  460,429,231,135,134,0,2,6,7,8,9,15,17,21,23,24,25,26,27,28,29,57,89,91,
  131,133,196,198,203,204,205,206,207,208,209,210,211,212,213,214,215,216,
  217,218,219,442,492,538,
704,702,701,700,699,698,697,696,695,694,693,692,691,688,687,686,685,684,683,
  682,537,535,534,533,532,531,530,529,528,527,526,525,524,521,518,517,491,
  460,429,231,135,134,0,2,6,7,8,9,15,17,21,23,24,25,26,27,28,29,57,89,91,
  131,133,196,198,203,204,205,206,207,208,209,210,211,212,213,214,215,216,
  217,218,219,442,492,538,
704,702,701,700,699,698,697,696,695,694,693,692,691,688,687,686,685,684,683,
  682,537,535,534,533,532,531,530,529,528,527,526,525,524,521,518,517,491,
  460,429,231,135,134,0,2,6,7,8,9,15,17,21,23,24,25,26,27,28,29,57,89,91,
  131,133,196,198,203,204,205,206,207,208,209,210,211,212,213,214,215,216,
  217,218,219,442,492,538,
704,702,701,700,699,698,697,696,695,694,693,692,691,688,687,686,685,684,683,
  682,537,535,534,533,532,531,530,529,528,527,526,525,524,521,518,517,491,
  460,429,231,135,134,0,2,6,7,8,9,15,17,21,23,24,25,26,27,28,29,57,89,91,
  131,133,196,198,203,204,205,206,207,208,209,210,211,212,213,214,215,216,
  217,218,219,442,492,538,
704,702,701,700,699,698,697,696,695,694,693,692,691,688,687,686,685,684,683,
  682,537,535,534,533,532,531,530,529,528,527,526,525,524,521,518,517,491,
  460,429,231,135,134,0,2,6,7,8,9,15,17,21,23,24,25,26,27,28,29,57,89,91,
  131,133,196,198,203,204,205,206,207,208,209,210,211,212,213,214,215,216,
  217,218,219,442,492,538,
704,702,701,700,699,698,697,696,695,694,693,692,691,688,687,686,685,684,683,
  682,537,535,534,533,532,531,530,529,528,527,526,525,524,521,518,517,491,
  460,429,231,135,134,0,2,6,7,8,9,15,17,21,23,24,25,26,27,28,29,57,89,91,
  131,133,196,198,203,204,205,206,207,208,209,210,211,212,213,214,215,216,
  217,218,219,442,492,538,
704,702,701,700,699,698,697,696,695,694,693,692,691,688,687,686,685,684,683,
  682,537,535,534,533,532,531,530,529,528,527,526,525,524,521,518,517,491,
  460,429,231,135,134,0,2,6,7,8,9,15,17,21,23,24,25,26,27,28,29,57,89,91,
  131,133,196,198,203,204,205,206,207,208,209,210,211,212,213,214,215,216,
  217,218,219,442,492,538,
704,702,701,700,699,698,697,696,695,694,693,692,691,688,687,686,685,684,683,
  682,537,535,534,533,532,531,530,529,528,527,526,525,524,521,518,517,491,
  460,429,231,135,134,0,2,6,7,8,9,15,17,21,23,24,25,26,27,28,29,57,89,91,
  131,133,196,198,203,204,205,206,207,208,209,210,211,212,213,214,215,216,
  217,218,219,442,492,538,
704,702,701,700,699,698,697,696,695,694,693,692,691,688,687,686,685,684,683,
  682,537,535,534,533,532,531,530,529,528,527,526,525,524,521,518,517,491,
  460,429,231,135,134,0,2,6,7,8,9,15,17,21,23,24,25,26,27,28,29,57,89,91,
  131,133,196,198,203,204,205,206,207,208,209,210,211,212,213,214,215,216,
  217,218,219,442,492,538,
704,702,701,700,699,698,697,696,695,694,693,692,691,688,687,686,685,684,683,
  682,537,535,534,533,532,531,530,529,528,527,526,525,524,521,518,517,491,
  460,429,231,135,134,0,2,6,7,8,9,15,17,21,23,24,25,26,27,28,29,57,89,91,
  131,133,196,198,203,204,205,206,207,208,209,210,211,212,213,214,215,216,
  217,218,219,442,492,538,
704,702,701,700,699,698,697,696,695,694,693,692,691,688,687,686,685,684,683,
  682,537,535,534,533,532,531,530,529,528,527,526,525,524,521,518,517,491,
  460,429,231,135,134,0,2,6,7,8,9,15,17,21,23,24,25,26,27,28,29,57,89,91,
  131,133,196,198,203,204,205,206,207,208,209,210,211,212,213,214,215,216,
  217,218,219,442,492,538,
704,702,701,700,699,698,697,696,695,694,693,692,691,688,687,686,685,684,683,
  682,537,535,534,533,532,531,530,529,528,527,526,525,524,521,518,517,491,
  460,429,231,135,134,0,2,6,7,8,9,15,17,21,23,24,25,26,27,28,29,57,89,91,
  131,133,196,198,203,204,205,206,207,208,209,210,211,212,213,214,215,216,
  217,218,219,442,492,538,
704,702,701,700,699,698,697,696,695,694,693,692,691,688,687,686,685,684,683,
  682,537,535,534,533,532,531,530,529,528,527,526,525,524,521,518,517,491,
  460,429,231,135,134,0,2,6,7,8,9,15,17,21,23,24,25,26,27,28,29,57,89,91,
  131,133,196,198,203,204,205,206,207,208,209,210,211,212,213,214,215,216,
  217,218,219,442,492,538,
704,702,701,700,699,698,697,696,695,694,693,692,691,688,687,686,685,684,683,
  682,537,535,534,533,532,531,530,529,528,527,526,525,524,521,518,517,491,
  460,429,231,135,134,0,2,6,7,8,9,15,17,21,23,24,25,26,27,28,29,57,89,91,
  131,133,196,198,203,204,205,206,207,208,209,210,211,212,213,214,215,216,
  217,218,219,442,492,538,
704,702,701,700,699,698,697,696,695,694,693,692,691,688,687,686,685,684,683,
  682,537,535,534,533,532,531,530,529,528,527,526,525,524,521,518,517,491,
  460,429,231,135,134,0,2,6,7,8,9,15,17,21,23,24,25,26,27,28,29,57,89,91,
  131,133,196,198,203,204,205,206,207,208,209,210,211,212,213,214,215,216,
  217,218,219,442,492,538,
704,702,701,700,699,698,697,696,695,694,693,692,691,688,687,686,685,684,683,
  682,537,535,534,533,532,531,530,529,528,527,526,525,524,521,518,517,491,
  460,429,231,135,134,0,2,6,7,8,9,15,17,21,23,24,25,26,27,28,29,57,89,91,
  131,133,196,198,203,204,205,206,207,208,209,210,211,212,213,214,215,216,
  217,218,219,442,492,538,
704,702,701,700,699,698,697,696,695,694,693,692,691,688,687,686,685,684,683,
  682,537,535,534,533,532,531,530,529,528,527,526,525,524,521,518,517,491,
  460,429,231,135,134,0,2,6,7,8,9,15,17,21,23,24,25,26,27,28,29,57,89,91,
  131,133,196,198,203,204,205,206,207,208,209,210,211,212,213,214,215,216,
  217,218,219,442,492,538,
668,540,539,248,246,238,236,234,0,243,245,247,249,251,252,
668,540,539,248,246,238,236,234,0,243,245,247,249,251,252,
668,540,539,248,246,238,236,234,0,243,245,247,249,251,252,
668,540,539,248,246,238,236,234,0,243,245,247,249,251,252,
668,246,236,234,0,245,247,253,
668,246,236,234,0,245,247,253,
668,543,248,246,238,236,234,0,5,245,247,249,608,
668,543,248,246,238,236,234,0,5,245,247,249,608,
668,543,248,246,238,236,234,0,5,245,247,249,608,
668,543,542,541,248,246,238,236,234,0,245,247,249,250,254,255,256,
668,543,542,541,248,246,238,236,234,0,245,247,249,250,254,255,256,
540,241,240,239,238,237,236,235,234,0,4,242,594,
540,241,240,239,238,237,236,235,234,0,4,242,594,
540,241,240,239,238,237,236,235,234,0,4,242,594,
540,241,240,239,238,237,236,235,234,0,4,242,594,
540,241,240,239,238,237,236,235,234,0,4,242,594,
540,241,240,239,238,237,236,235,234,0,4,242,594,
540,241,240,239,238,237,236,235,234,0,4,242,594,
540,241,240,239,238,237,236,235,234,0,4,242,594,
540,241,240,239,238,237,236,235,234,0,4,242,594,
540,241,240,239,238,237,236,235,234,0,4,242,594,
540,241,240,239,238,237,236,235,234,0,4,242,594,
704,702,701,700,699,698,697,696,695,694,693,692,691,688,687,686,685,684,683,
  682,681,537,535,534,533,532,531,530,529,528,527,526,525,524,521,518,517,
  491,460,429,231,135,134,0,2,6,7,8,9,10,12,15,17,18,20,21,23,24,25,26,27,
  28,29,57,89,91,92,131,133,196,198,203,204,205,206,207,208,209,210,211,
  212,213,214,215,216,217,218,219,435,442,485,492,538,
692,688,687,686,685,684,683,682,521,518,517,231,1,0,2,6,7,15,17,218,219,492,
679,678,677,676,675,674,673,672,671,670,669,668,667,666,665,664,663,662,661,
  660,659,658,657,656,655,654,653,652,651,650,649,648,647,646,645,644,643,
  642,641,640,639,638,637,636,635,634,633,632,631,630,629,628,627,626,625,
  624,623,622,621,620,619,618,617,616,615,614,613,612,611,610,609,607,606,
  605,604,603,602,601,600,599,598,597,596,595,593,592,591,590,589,588,587,
  586,585,584,583,582,581,580,579,578,577,576,575,574,573,572,571,570,569,
  568,567,566,565,564,563,562,561,560,559,558,557,556,555,554,553,552,551,
  550,549,548,547,546,545,1,0,51,52,
679,678,677,676,675,674,673,672,671,670,669,668,667,666,665,664,663,662,661,
  660,659,658,657,656,655,654,653,652,651,650,649,648,647,646,645,644,643,
  642,641,640,639,638,637,636,635,634,633,632,631,630,629,628,627,626,625,
  624,623,622,621,620,619,618,617,616,615,614,613,612,611,610,609,607,606,
  605,604,603,602,601,600,599,598,597,596,595,593,592,591,590,589,588,587,
  586,585,584,583,582,581,580,579,578,577,576,575,574,573,572,571,570,569,
  568,567,566,565,564,563,562,561,560,559,558,557,556,555,554,553,552,551,
  550,549,548,547,546,545,1,0,51,52,
704,702,701,700,699,698,697,696,695,694,693,691,682,135,134,1,0,51,52,
704,702,701,700,699,698,697,696,695,694,693,691,682,135,134,0,21,133,442,
704,703,702,701,700,699,698,697,696,695,694,693,688,687,686,685,684,520,429,
  1,0,51,52,
704,703,702,701,700,699,698,697,696,695,694,693,688,687,686,685,684,520,429,
  0,19,437,
427,425,424,423,38,1,0,51,52,
427,425,424,423,38,1,0,51,52,
683,681,1,0,51,52,
683,681,0,10,12,18,20,435,485,
427,425,424,423,38,1,0,51,52,
427,425,424,423,38,1,0,51,52,
704,702,701,700,699,698,697,696,695,694,693,691,682,427,425,424,423,135,134,
  38,1,0,51,52,
704,702,701,700,699,698,697,696,695,694,693,691,682,427,425,424,423,135,134,
  38,1,0,21,133,442,
681,0,10,12,435,
681,0,10,12,435,
704,702,701,700,699,698,697,696,695,694,693,692,691,688,687,686,685,684,683,
  682,681,537,535,534,533,532,531,530,529,528,527,526,525,524,521,518,517,
  491,460,429,231,135,134,1,0,51,52,
704,702,701,700,699,698,697,696,695,694,693,692,691,688,687,686,685,684,683,
  682,681,537,535,534,533,532,531,530,529,528,527,526,525,524,521,518,517,
  491,460,429,231,135,134,1,0,51,52,
704,702,701,700,699,698,697,696,695,694,693,692,691,688,687,686,685,684,683,
  682,681,537,535,534,533,532,531,530,529,528,527,526,525,524,521,518,517,
  491,460,429,231,135,134,1,0,2,6,7,8,9,10,12,15,17,21,23,24,25,26,27,28,
  29,51,52,57,89,91,131,133,196,198,203,204,205,206,207,208,209,210,211,
  212,213,214,215,216,217,218,219,435,442,492,538,
704,702,701,700,699,698,697,696,695,694,693,692,691,688,687,686,685,684,683,
  682,537,535,534,533,532,531,530,529,528,527,526,525,524,521,518,517,491,
  460,429,231,135,134,1,0,51,52,
704,702,701,700,699,698,697,696,695,694,693,692,691,688,687,686,685,684,683,
  682,537,535,534,533,532,531,530,529,528,527,526,525,524,521,518,517,491,
  460,429,231,135,134,0,2,6,7,8,9,15,17,21,23,24,25,26,27,28,29,57,89,91,
  131,133,196,198,203,204,205,206,207,208,209,210,211,212,213,214,215,216,
  217,218,219,442,492,538,
704,702,701,700,699,698,697,696,695,694,693,691,682,135,134,1,0,51,52,
704,702,701,700,699,698,697,696,695,694,693,691,682,135,134,0,21,133,442,
704,702,701,700,699,698,697,696,695,694,693,691,682,135,134,1,0,51,52,
704,702,701,700,699,698,697,696,695,694,693,691,682,135,134,0,21,133,442,
704,702,701,700,699,698,697,696,695,694,693,691,682,135,134,1,0,51,52,
704,702,701,700,699,698,697,696,695,694,693,691,682,135,134,0,21,116,133,
  442,
704,702,701,700,699,698,697,696,695,694,693,691,682,135,134,1,0,51,52,
704,702,701,700,699,698,697,696,695,694,693,691,682,135,134,0,21,116,133,
  442,
704,702,701,700,699,698,697,696,695,694,693,691,682,135,134,1,0,51,52,
704,702,701,700,699,698,697,696,695,694,693,691,682,135,134,0,21,116,133,
  442,
704,702,701,700,699,698,697,696,695,694,693,692,691,688,687,686,685,684,683,
  682,681,537,535,534,533,532,531,530,529,528,527,526,525,524,521,518,517,
  491,460,429,231,135,134,1,0,51,52,
704,702,701,700,699,698,697,696,695,694,693,692,691,688,687,686,685,684,683,
  682,681,537,535,534,533,532,531,530,529,528,527,526,525,524,521,518,517,
  491,460,429,231,135,134,1,0,51,52,
704,702,701,700,699,698,697,696,695,694,693,692,691,688,687,686,685,684,683,
  682,681,537,535,534,533,532,531,530,529,528,527,526,525,524,521,518,517,
  491,460,429,231,135,134,0,2,6,7,8,9,10,12,15,17,18,20,21,23,24,25,26,27,
  28,29,57,89,91,92,131,133,196,198,203,204,205,206,207,208,209,210,211,
  212,213,214,215,216,217,218,219,435,442,485,492,538,
704,702,701,700,699,698,697,696,695,694,693,692,691,688,687,686,685,684,683,
  682,681,537,535,534,533,532,531,530,529,528,527,526,525,524,521,518,517,
  491,460,429,231,135,134,1,0,51,52,
704,702,701,700,699,698,697,696,695,694,693,692,691,688,687,686,685,684,683,
  682,681,537,535,534,533,532,531,530,529,528,527,526,525,524,521,518,517,
  491,460,429,231,135,134,1,0,51,52,
704,702,701,700,699,698,697,696,695,694,693,692,691,688,687,686,685,684,683,
  682,681,537,535,534,533,532,531,530,529,528,527,526,525,524,521,518,517,
  491,460,429,231,135,134,1,0,51,52,
704,702,701,700,699,698,697,696,695,694,693,692,691,688,687,686,685,684,683,
  682,681,537,535,534,533,532,531,530,529,528,527,526,525,524,521,518,517,
  491,460,429,231,135,134,0,2,6,7,8,9,10,12,15,17,18,20,21,23,24,25,26,27,
  28,29,57,89,91,92,131,133,196,198,203,204,205,206,207,208,209,210,211,
  212,213,214,215,216,217,218,219,435,442,485,492,538,
704,702,701,700,699,698,697,696,695,694,693,692,691,688,687,686,685,684,683,
  682,537,535,534,533,532,531,530,529,528,527,526,525,524,521,518,517,491,
  460,429,231,135,134,1,0,51,52,
704,702,701,700,699,698,697,696,695,694,693,692,691,688,687,686,685,684,683,
  682,537,535,534,533,532,531,530,529,528,527,526,525,524,521,518,517,491,
  460,429,231,135,134,1,0,51,52,
704,702,701,700,699,698,697,696,695,694,693,692,691,688,687,686,685,684,683,
  682,537,535,534,533,532,531,530,529,528,527,526,525,524,521,518,517,491,
  460,429,231,135,134,0,2,6,7,8,9,15,17,21,23,24,25,26,27,28,29,57,89,91,
  131,133,196,198,203,204,205,206,207,208,209,210,211,212,213,214,215,216,
  217,218,219,442,492,538,
427,425,424,423,38,1,0,51,52,
704,702,701,700,699,698,697,696,695,694,693,692,691,688,687,686,685,684,683,
  682,537,535,534,533,532,531,530,529,528,527,526,525,524,521,518,517,491,
  460,429,231,135,134,1,0,51,52,
704,702,701,700,699,698,697,696,695,694,693,692,691,688,687,686,685,684,683,
  682,537,535,534,533,532,531,530,529,528,527,526,525,524,521,518,517,491,
  460,429,231,135,134,1,0,51,52,
704,702,701,700,699,698,697,696,695,694,693,692,691,688,687,686,685,684,683,
  682,537,535,534,533,532,531,530,529,528,527,526,525,524,521,518,517,491,
  460,429,231,135,134,1,0,51,52,
704,702,701,700,699,698,697,696,695,694,693,692,691,688,687,686,685,684,683,
  682,537,535,534,533,532,531,530,529,528,527,526,525,524,521,518,517,491,
  460,429,231,135,134,0,2,6,7,8,9,15,17,21,23,24,25,26,27,28,29,57,89,91,
  131,133,196,198,203,204,205,206,207,208,209,210,211,212,213,214,215,216,
  217,218,219,442,492,538,
704,702,701,700,699,698,697,696,695,694,693,691,682,135,134,0,21,133,442,
494,491,427,425,424,423,38,1,0,51,52,
494,491,427,425,424,423,38,1,0,51,52,
427,425,424,423,38,0,39,40,41,45,46,47,49,
705,704,702,701,700,699,698,697,696,695,694,693,692,691,690,689,688,687,686,
  685,684,683,682,681,680,526,524,521,520,519,518,517,511,509,507,460,459,
  455,429,425,0,46,54,86,90,91,93,140,141,145,146,147,148,149,150,154,157,
  158,185,187,189,195,196,197,199,200,203,205,220,224,225,229,413,414,415,
  416,417,418,419,420,421,422,
427,425,424,423,38,0,39,40,41,45,46,47,49,
455,0,86,
427,425,424,423,38,1,0,51,52,
427,425,424,423,38,1,0,51,52,
427,425,424,423,38,1,0,51,52,
703,702,701,700,699,696,694,686,685,684,682,681,423,0,155,
427,425,424,423,38,0,39,40,41,45,46,47,49,
427,425,424,423,38,0,39,40,41,45,46,47,49,
696,695,694,693,688,687,686,685,684,0,
703,701,700,699,684,683,0,
704,703,702,701,700,699,698,697,696,695,694,693,692,691,690,689,688,687,686,
  685,684,683,682,681,680,526,524,521,520,519,518,517,511,509,507,460,459,
  455,429,425,423,233,1,0,16,
698,696,695,694,693,688,687,686,685,684,544,523,522,521,520,519,518,517,516,
  515,514,513,512,511,510,509,508,507,506,505,504,503,502,501,500,499,498,
  497,496,495,493,459,432,427,425,424,423,38,1,0,
698,696,695,694,693,688,687,686,685,684,544,523,522,521,520,519,518,517,516,
  515,514,513,512,511,510,509,508,507,506,505,504,503,502,501,500,499,498,
  497,496,495,493,459,432,427,425,424,423,38,1,0,
460,0,91,
460,0,91,
460,0,91,
460,0,91,
460,0,91,
460,0,91,
460,0,91,
460,0,91,
460,0,91,
460,0,91,
460,0,91,
704,702,701,700,699,698,697,696,695,694,693,692,691,688,687,686,685,684,683,
  682,537,535,534,533,532,531,530,529,528,527,526,525,524,521,518,517,491,
  460,429,231,135,134,0,2,6,7,8,9,15,17,21,23,24,25,26,27,28,29,57,89,91,
  131,133,196,198,203,204,205,206,207,208,209,210,211,212,213,214,215,216,
  217,218,219,442,492,538,
688,687,686,685,684,0,
704,702,701,700,699,698,697,696,695,694,693,692,691,688,687,686,685,684,683,
  682,537,535,534,533,532,531,530,529,528,527,521,518,517,491,460,429,231,
  135,134,1,0,51,52,
704,702,701,700,699,698,697,696,695,694,693,692,691,688,687,686,685,684,683,
  682,537,535,534,533,532,531,530,529,528,527,521,518,517,491,460,429,231,
  135,134,1,0,51,52,
704,702,701,700,699,698,697,696,695,694,693,692,691,688,687,686,685,684,683,
  682,537,535,534,533,532,531,530,529,528,527,521,518,517,491,460,429,231,
  135,134,1,0,51,52,
704,702,701,700,699,698,697,696,695,694,693,692,691,688,687,686,685,684,683,
  682,537,535,534,533,532,531,530,529,528,527,521,518,517,491,460,429,231,
  135,134,1,0,51,52,
704,702,701,700,699,698,697,696,695,694,693,692,691,688,687,686,685,684,683,
  682,537,535,534,533,532,531,530,529,528,527,521,518,517,491,460,429,231,
  135,134,1,0,51,52,
704,702,701,700,699,698,697,696,695,694,693,692,691,688,687,686,685,684,683,
  682,537,535,534,533,532,531,530,529,528,527,521,518,517,491,460,429,231,
  135,134,1,0,51,52,
704,702,701,700,699,698,697,696,695,694,693,692,691,688,687,686,685,684,683,
  682,537,535,534,533,532,531,530,529,528,527,521,518,517,491,460,429,231,
  135,134,1,0,51,52,
704,702,701,700,699,698,697,696,695,694,693,692,691,688,687,686,685,684,683,
  682,537,535,534,533,532,531,530,529,528,527,526,525,524,521,518,517,491,
  460,429,231,135,134,1,0,51,52,
704,702,701,700,699,698,697,696,695,694,693,692,691,688,687,686,685,684,683,
  682,537,535,534,533,532,531,530,529,528,527,526,525,524,521,518,517,491,
  460,429,231,135,134,1,0,51,52,
704,702,701,700,699,698,697,696,695,694,693,692,691,688,687,686,685,684,683,
  682,537,535,534,533,532,531,530,529,528,527,526,525,524,521,518,517,491,
  460,429,231,135,134,1,0,51,52,
704,702,701,700,699,698,697,696,695,694,693,692,691,688,687,686,685,684,683,
  682,537,535,534,533,532,531,530,529,528,527,526,525,524,521,518,517,491,
  460,429,231,135,134,1,0,51,52,
704,702,701,700,699,698,697,696,695,694,693,692,691,688,687,686,685,684,683,
  682,537,535,534,533,532,531,530,529,528,527,526,525,524,521,518,517,491,
  460,429,231,135,134,1,0,51,52,
704,702,701,700,699,698,697,696,695,694,693,692,691,688,687,686,685,684,683,
  682,537,535,534,533,532,531,530,529,528,527,526,525,524,521,518,517,491,
  460,429,231,135,134,1,0,51,52,
704,702,701,700,699,698,697,696,695,694,693,692,691,688,687,686,685,684,683,
  682,537,535,534,533,532,531,530,529,528,527,526,525,524,521,518,517,491,
  460,429,231,135,134,1,0,51,52,
704,702,701,700,699,698,697,696,695,694,693,692,691,688,687,686,685,684,683,
  682,537,535,534,533,532,531,530,529,528,527,526,525,524,521,518,517,491,
  460,429,231,135,134,1,0,51,52,
704,702,701,700,699,698,697,696,695,694,693,692,691,688,687,686,685,684,683,
  682,537,535,534,533,532,531,530,529,528,527,526,525,524,521,518,517,491,
  460,429,231,135,134,1,0,51,52,
704,702,701,700,699,698,697,696,695,694,693,692,691,688,687,686,685,684,683,
  682,537,535,534,533,532,531,530,529,528,527,526,525,524,521,518,517,491,
  460,429,231,135,134,1,0,51,52,
704,702,701,700,699,698,697,696,695,694,693,692,691,688,687,686,685,684,683,
  682,537,535,534,533,532,531,530,529,528,527,526,525,524,521,518,517,491,
  460,429,231,135,134,1,0,51,52,
704,702,701,700,699,698,697,696,695,694,693,692,691,688,687,686,685,684,683,
  682,537,535,534,533,532,531,530,529,528,527,526,525,524,521,518,517,491,
  460,429,231,135,134,1,0,51,52,
704,702,701,700,699,698,697,696,695,694,693,692,691,688,687,686,685,684,683,
  682,537,535,534,533,532,531,530,529,528,527,526,525,524,521,518,517,491,
  460,429,231,135,134,1,0,51,52,
704,702,701,700,699,698,697,696,695,694,693,692,691,688,687,686,685,684,683,
  682,537,535,534,533,532,531,530,529,528,527,526,525,524,521,518,517,491,
  460,429,231,135,134,1,0,51,52,
704,702,701,700,699,698,697,696,695,694,693,692,691,688,687,686,685,684,683,
  682,537,535,534,533,532,531,530,529,528,527,526,525,524,521,518,517,491,
  460,429,231,135,134,1,0,51,52,
704,702,701,700,699,698,697,696,695,694,693,692,691,688,687,686,685,684,683,
  682,537,535,534,533,532,531,530,529,528,527,526,525,524,521,518,517,491,
  460,429,231,135,134,1,0,51,52,
704,702,701,700,699,698,697,696,695,694,693,692,691,688,687,686,685,684,683,
  682,537,535,534,533,532,531,530,529,528,527,526,525,524,521,518,517,491,
  460,429,231,135,134,1,0,51,52,
704,702,701,700,699,698,697,696,695,694,693,692,691,688,687,686,685,684,683,
  682,537,535,534,533,532,531,530,529,528,527,526,525,524,521,518,517,491,
  460,429,231,135,134,1,0,51,52,
704,702,701,700,699,698,697,696,695,694,693,692,691,688,687,686,685,684,683,
  682,537,535,534,533,532,531,530,529,528,527,526,525,524,521,518,517,491,
  460,429,231,135,134,1,0,51,52,
704,702,701,700,699,698,697,696,695,694,693,692,691,688,687,686,685,684,683,
  682,537,535,534,533,532,531,530,529,528,527,526,525,524,521,518,517,491,
  460,429,231,135,134,1,0,51,52,
704,702,701,700,699,698,697,696,695,694,693,692,691,688,687,686,685,684,683,
  682,537,535,534,533,532,531,530,529,528,527,526,525,524,521,518,517,491,
  460,429,231,135,134,1,0,51,52,
704,702,701,700,699,698,697,696,695,694,693,692,691,688,687,686,685,684,683,
  682,537,535,534,533,532,531,530,529,528,527,526,525,524,521,518,517,491,
  460,429,231,135,134,1,0,51,52,
704,702,701,700,699,698,697,696,695,694,693,692,691,688,687,686,685,684,683,
  682,537,535,534,533,532,531,530,529,528,527,526,525,524,521,518,517,491,
  460,429,231,135,134,1,0,51,52,
704,702,701,700,699,698,697,696,695,694,693,692,691,688,687,686,685,684,683,
  682,537,535,534,533,532,531,530,529,528,527,526,525,524,521,518,517,491,
  460,429,231,135,134,1,0,51,52,
704,702,701,700,699,698,697,696,695,694,693,692,691,688,687,686,685,684,683,
  682,537,535,534,533,532,531,530,529,528,527,526,525,524,521,518,517,491,
  460,429,231,135,134,1,0,51,52,
704,702,701,700,699,698,697,696,695,694,693,692,691,688,687,686,685,684,683,
  682,537,535,534,533,532,531,530,529,528,527,526,525,524,521,518,517,491,
  460,429,231,135,134,1,0,51,52,
704,702,701,700,699,698,697,696,695,694,693,692,691,688,687,686,685,684,683,
  682,537,535,534,533,532,531,530,529,528,527,526,525,524,521,518,517,491,
  460,429,231,135,134,1,0,51,52,
704,702,701,700,699,698,697,696,695,694,693,692,691,688,687,686,685,684,683,
  682,537,535,534,533,532,531,530,529,528,527,526,525,524,521,518,517,491,
  460,429,231,135,134,1,0,51,52,
704,702,701,700,699,698,697,696,695,694,693,692,691,688,687,686,685,684,683,
  682,537,535,534,533,532,531,530,529,528,527,526,525,524,521,518,517,491,
  460,429,231,135,134,1,0,51,52,
704,702,701,700,699,698,697,696,695,694,693,692,691,688,687,686,685,684,683,
  682,537,535,534,533,532,531,530,529,528,527,526,525,524,521,518,517,491,
  460,429,231,135,134,1,0,51,52,
704,702,701,700,699,698,697,696,695,694,693,692,691,688,687,686,685,684,683,
  682,537,535,534,533,532,531,530,529,528,527,526,525,524,521,518,517,491,
  460,429,231,135,134,1,0,51,52,
427,425,424,423,38,0,39,40,41,45,46,47,49,
427,425,424,423,38,0,39,40,41,45,46,47,49,
427,425,424,423,38,0,39,40,41,45,46,47,49,
427,425,424,423,38,0,39,40,41,45,46,47,49,
427,425,424,423,38,0,39,40,41,45,46,47,49,
704,702,701,700,699,698,697,696,695,694,693,691,682,135,134,0,21,133,442,
427,425,424,423,38,0,39,40,41,45,46,47,49,
427,425,424,423,38,0,39,40,41,45,46,47,49,
705,704,703,702,701,700,699,698,697,696,695,694,693,692,691,690,689,688,687,
  686,685,684,683,682,681,680,526,524,521,520,519,518,517,511,509,507,460,
  459,455,429,425,423,1,0,
427,425,424,423,38,0,39,40,41,45,46,47,49,
427,425,424,423,38,0,39,40,41,45,46,47,49,
704,703,702,701,700,699,698,697,696,695,694,693,688,687,686,685,684,520,429,
  425,423,38,1,0,51,52,
427,425,424,423,38,1,0,51,52,
427,425,424,423,38,1,0,51,52,
427,425,424,423,38,1,0,51,52,
427,425,424,423,38,1,0,51,52,
704,702,701,700,699,698,697,696,695,694,693,692,691,688,687,686,685,684,683,
  682,537,535,534,533,532,531,530,529,528,527,526,525,524,521,518,517,491,
  460,429,231,135,134,1,0,51,52,
704,702,701,700,699,698,697,696,695,694,693,692,691,688,687,686,685,684,683,
  682,537,535,534,533,532,531,530,529,528,527,526,525,524,521,518,517,491,
  460,429,231,135,134,0,2,6,7,8,9,15,17,21,23,24,25,26,27,28,29,57,89,91,
  131,133,196,198,203,204,205,206,207,208,209,210,211,212,213,214,215,216,
  217,218,219,442,492,538,
704,702,701,700,699,698,697,696,695,694,693,692,691,688,687,686,685,684,683,
  682,537,535,534,533,532,531,530,529,528,527,526,525,524,521,518,517,491,
  460,429,231,135,134,1,0,51,52,
704,702,701,700,699,698,697,696,695,694,693,692,691,688,687,686,685,684,683,
  682,537,535,534,533,532,531,530,529,528,527,526,525,524,521,518,517,491,
  460,429,231,135,134,0,2,6,7,8,9,15,17,21,23,24,25,26,27,28,29,57,89,91,
  131,133,196,198,203,204,205,206,207,208,209,210,211,212,213,214,215,216,
  217,218,219,442,492,538,
427,425,424,423,38,0,39,40,41,45,46,47,49,
427,425,424,423,38,1,0,51,52,
427,425,424,423,38,1,0,51,52,
544,455,427,425,424,423,38,1,0,51,52,
544,455,427,425,424,423,38,1,0,51,52,
544,455,427,425,424,423,38,1,0,51,52,
544,493,455,427,425,424,423,38,1,0,51,52,
544,455,427,425,424,423,38,1,0,51,52,
704,702,701,700,699,698,697,696,695,694,693,692,691,688,687,686,685,684,683,
  682,537,535,534,533,532,531,530,529,528,527,526,525,524,521,518,517,491,
  460,429,231,135,134,1,0,51,52,
704,702,701,700,699,698,697,696,695,694,693,692,691,688,687,686,685,684,683,
  682,537,535,534,533,532,531,530,529,528,527,526,525,524,521,518,517,491,
  460,429,231,135,134,1,0,51,52,
704,702,701,700,699,698,697,696,695,694,693,692,691,688,687,686,685,684,683,
  682,537,535,534,533,532,531,530,529,528,527,526,525,524,521,518,517,491,
  460,429,231,135,134,1,0,51,52,
544,427,425,424,423,38,1,0,51,52,
544,427,425,424,423,38,1,0,51,52,
493,1,0,51,52,
704,703,702,701,700,699,698,697,696,695,694,693,692,691,690,689,688,687,686,
  685,684,682,681,680,526,524,521,520,519,518,517,511,509,507,460,459,455,
  429,425,423,233,1,0,16,
704,703,702,701,700,699,698,697,696,695,694,693,692,691,690,689,688,687,686,
  685,684,683,682,681,680,526,524,521,520,519,518,517,511,509,507,460,459,
  455,429,425,423,233,1,0,16,
493,459,427,425,424,423,38,1,0,51,52,
493,459,0,93,132,
696,695,694,693,688,687,686,685,684,0,
704,698,696,695,694,693,688,687,686,685,684,682,427,425,424,423,222,221,38,
  1,0,223,
679,678,677,676,675,674,673,672,671,670,669,668,667,666,665,664,663,662,661,
  660,659,658,657,656,655,654,653,652,651,650,649,648,647,646,645,644,643,
  642,641,640,639,638,637,636,635,634,633,632,631,630,629,628,627,626,625,
  624,623,622,621,620,619,618,617,616,615,614,613,612,611,610,609,607,606,
  605,604,603,602,601,600,599,598,597,596,595,593,592,591,590,589,588,587,
  586,585,584,583,582,581,580,579,578,577,576,575,574,573,572,571,570,569,
  568,567,566,565,564,563,562,561,560,559,558,557,556,555,554,553,552,551,
  550,549,548,547,546,545,0,31,32,244,257,258,259,260,261,262,263,264,265,
  266,268,269,270,271,272,273,274,275,276,277,278,279,280,281,282,283,284,
  285,286,287,288,289,290,291,292,293,294,295,296,297,298,299,300,301,302,
  303,304,305,306,307,308,309,310,311,312,313,314,315,316,317,318,319,320,
  321,322,323,324,325,326,327,328,329,330,331,332,333,334,335,336,337,338,
  339,340,341,342,343,344,346,347,348,349,350,351,352,353,354,355,356,357,
  358,359,360,361,362,363,364,365,366,367,368,369,370,371,372,373,374,375,
  376,377,378,379,380,381,382,383,384,385,386,387,388,389,390,391,392,393,
  394,395,396,397,398,399,400,401,402,403,404,405,406,407,408,409,410,411,
  412,
704,703,702,701,700,699,698,697,696,695,694,693,692,691,690,689,688,687,686,
  685,684,682,681,680,526,524,521,520,519,518,517,511,509,507,460,459,455,
  429,425,423,233,1,0,16,
455,0,86,
704,702,701,700,699,698,697,696,695,694,693,692,691,688,687,686,685,684,683,
  682,681,537,535,534,533,532,531,530,529,528,527,526,525,524,521,518,517,
  491,460,429,231,135,134,0,2,6,7,8,9,10,12,15,17,18,20,21,23,24,25,26,27,
  28,29,57,89,91,92,131,133,196,198,203,204,205,206,207,208,209,210,211,
  212,213,214,215,216,217,218,219,435,442,485,492,538,
704,702,701,700,699,698,697,696,695,694,693,692,691,688,687,686,685,684,683,
  682,681,537,535,534,533,532,531,530,529,528,527,526,525,524,521,518,517,
  491,460,429,231,135,134,0,2,6,7,8,9,10,12,15,17,21,23,24,25,26,27,28,29,
  57,89,91,131,133,196,198,203,204,205,206,207,208,209,210,211,212,213,
  214,215,216,217,218,219,435,442,492,538,
493,427,425,424,423,38,1,0,132,
493,427,425,424,423,38,1,0,132,
493,427,425,424,423,38,1,0,132,
493,427,425,424,423,38,1,0,132,
493,427,425,424,423,38,1,0,132,
705,704,702,701,700,699,698,697,696,695,694,693,692,691,690,689,688,687,686,
  685,684,683,682,681,680,526,524,521,520,519,518,517,511,509,507,461,460,
  459,455,429,425,423,38,1,0,46,51,52,54,86,91,93,96,140,141,145,146,147,
  148,149,150,154,157,158,185,187,189,195,196,197,199,200,203,205,220,224,
  225,229,413,414,415,416,417,418,419,420,421,422,
704,702,701,700,699,698,697,696,695,694,693,692,691,688,687,686,685,684,683,
  682,537,535,534,533,532,531,530,529,528,527,526,525,524,521,518,517,491,
  460,429,231,135,134,1,0,51,52,
427,425,424,423,38,0,39,40,41,45,46,47,49,
427,425,424,423,38,0,39,40,41,45,46,47,49,
427,425,424,423,38,0,39,40,41,45,46,47,49,
696,695,694,693,688,687,686,685,684,0,
688,687,686,685,684,0,
704,703,702,701,700,699,698,697,696,695,694,693,692,691,690,689,688,687,686,
  685,684,683,682,681,680,526,524,521,520,519,518,517,511,509,507,460,459,
  455,429,425,423,1,0,
683,0,
704,702,701,700,699,698,697,696,695,694,693,691,682,135,134,1,0,51,52,
704,702,701,700,699,698,697,696,695,694,693,692,691,688,687,686,685,684,683,
  682,537,535,534,533,532,531,530,529,528,527,526,525,524,521,518,517,491,
  460,429,231,135,134,0,2,6,7,8,9,15,17,21,23,24,25,26,27,28,29,57,89,91,
  131,133,196,198,203,204,205,206,207,208,209,210,211,212,213,214,215,216,
  217,218,219,442,492,538,
704,702,701,700,699,698,697,696,695,694,693,692,691,688,687,686,685,684,683,
  682,537,535,534,533,532,531,530,529,528,527,526,525,524,521,518,517,491,
  460,429,231,135,134,0,2,6,7,8,9,15,17,21,23,24,25,26,27,28,29,57,89,91,
  131,133,196,198,203,204,205,206,207,208,209,210,211,212,213,214,215,216,
  217,218,219,442,492,538,
704,702,701,700,699,698,697,696,695,694,693,692,691,688,687,686,685,684,683,
  682,537,535,534,533,532,531,530,529,528,527,526,525,524,521,518,517,491,
  460,429,231,135,134,0,2,6,7,8,9,15,17,21,23,24,25,26,27,28,29,57,89,91,
  131,133,196,198,203,204,205,206,207,208,209,210,211,212,213,214,215,216,
  217,218,219,442,492,538,
704,702,701,700,699,698,697,696,695,694,693,692,691,688,687,686,685,684,683,
  682,537,535,534,533,532,531,530,529,528,527,526,525,524,521,518,517,491,
  460,429,231,135,134,0,2,6,7,8,9,15,17,21,23,24,25,26,27,28,29,57,89,91,
  131,133,196,198,203,204,205,206,207,208,209,210,211,212,213,214,215,216,
  217,218,219,442,492,538,
704,702,701,700,699,698,697,696,695,694,693,692,691,688,687,686,685,684,683,
  682,537,535,534,533,532,531,530,529,528,527,526,525,524,521,518,517,491,
  460,429,231,135,134,0,2,6,7,8,9,15,17,21,23,24,25,26,27,28,29,57,89,91,
  131,133,196,198,203,204,205,206,207,208,209,210,211,212,213,214,215,216,
  217,218,219,442,492,538,
704,702,701,700,699,698,697,696,695,694,693,692,691,688,687,686,685,684,683,
  682,537,535,534,533,532,531,530,529,528,527,526,525,524,521,518,517,491,
  460,429,231,135,134,0,2,6,7,8,9,15,17,21,23,24,25,26,27,28,29,57,89,91,
  131,133,196,198,203,204,205,206,207,208,209,210,211,212,213,214,215,216,
  217,218,219,442,492,538,
704,702,701,700,699,698,697,696,695,694,693,692,691,688,687,686,685,684,683,
  682,537,535,534,533,532,531,530,529,528,527,526,525,524,521,518,517,491,
  460,429,231,135,134,0,2,6,7,8,9,15,17,21,23,24,25,26,27,28,29,57,89,91,
  131,133,196,198,203,204,205,206,207,208,209,210,211,212,213,214,215,216,
  217,218,219,442,492,538,
704,702,701,700,699,698,697,696,695,694,693,692,691,688,687,686,685,684,683,
  682,537,535,534,533,532,531,530,529,528,527,526,525,524,521,518,517,491,
  460,429,231,135,134,0,2,6,7,8,9,15,17,21,23,24,25,26,27,28,29,57,89,91,
  131,133,196,198,203,204,205,206,207,208,209,210,211,212,213,214,215,216,
  217,218,219,442,492,538,
704,702,701,700,699,698,697,696,695,694,693,692,691,688,687,686,685,684,683,
  682,537,535,534,533,532,531,530,529,528,527,526,525,524,521,518,517,491,
  460,429,231,135,134,0,2,6,7,8,9,15,17,21,23,24,25,26,27,28,29,57,89,91,
  131,133,196,198,203,204,205,206,207,208,209,210,211,212,213,214,215,216,
  217,218,219,442,492,538,
704,702,701,700,699,698,697,696,695,694,693,692,691,688,687,686,685,684,683,
  682,537,535,534,533,532,531,530,529,528,527,526,525,524,521,518,517,491,
  460,429,231,135,134,0,2,6,7,8,9,15,17,21,23,24,25,26,27,28,29,57,89,91,
  131,133,196,198,203,204,205,206,207,208,209,210,211,212,213,214,215,216,
  217,218,219,442,492,538,
459,1,0,51,52,
704,702,701,700,699,698,697,696,695,694,693,692,691,688,687,686,685,684,683,
  682,537,535,534,533,532,531,530,529,528,527,521,518,517,491,460,429,231,
  135,134,0,2,6,7,8,9,15,17,21,89,91,131,133,198,206,207,208,209,210,211,
  212,213,214,215,216,217,218,219,442,492,538,
704,702,701,700,699,698,697,696,695,694,693,692,691,688,687,686,685,684,683,
  682,537,535,534,533,532,531,530,529,528,527,521,518,517,491,460,429,231,
  135,134,0,2,6,7,8,9,15,17,21,89,91,131,133,198,206,207,208,209,210,211,
  212,213,214,215,216,217,218,219,442,492,538,
704,702,701,700,699,698,697,696,695,694,693,692,691,688,687,686,685,684,683,
  682,537,535,534,533,532,531,530,529,528,527,521,518,517,491,460,429,231,
  135,134,0,2,6,7,8,9,15,17,21,89,91,131,133,198,206,207,208,209,210,211,
  212,213,214,215,216,217,218,219,442,492,538,
704,702,701,700,699,698,697,696,695,694,693,692,691,688,687,686,685,684,683,
  682,537,535,534,533,532,531,530,529,528,527,521,518,517,491,460,429,231,
  135,134,0,2,6,7,8,9,15,17,21,89,91,131,133,198,206,207,208,209,210,211,
  212,213,214,215,216,217,218,219,442,492,538,
704,702,701,700,699,698,697,696,695,694,693,692,691,688,687,686,685,684,683,
  682,537,535,534,533,532,531,530,529,528,527,521,518,517,491,460,429,231,
  135,134,0,2,6,7,8,9,15,17,21,89,91,131,133,198,206,207,208,209,210,211,
  212,213,214,215,216,217,218,219,442,492,538,
704,702,701,700,699,698,697,696,695,694,693,692,691,688,687,686,685,684,683,
  682,537,535,534,533,532,531,530,529,528,527,526,525,524,521,518,517,491,
  460,429,231,135,134,0,2,6,7,8,9,15,17,21,25,29,89,91,131,133,196,198,
  203,204,205,206,207,208,209,210,211,212,213,214,215,216,217,218,219,442,
  492,538,
704,702,701,700,699,698,697,696,695,694,693,692,691,688,687,686,685,684,683,
  682,537,535,534,533,532,531,530,529,528,527,526,525,524,521,518,517,491,
  460,429,231,135,134,0,2,6,7,8,9,15,17,21,25,29,89,91,131,133,196,198,
  203,204,205,206,207,208,209,210,211,212,213,214,215,216,217,218,219,442,
  492,538,
704,702,701,700,699,698,697,696,695,694,693,692,691,688,687,686,685,684,683,
  682,537,535,534,533,532,531,530,529,528,527,526,525,524,521,518,517,491,
  460,429,231,135,134,0,2,6,7,8,9,15,17,21,25,27,29,89,91,131,133,196,198,
  203,204,205,206,207,208,209,210,211,212,213,214,215,216,217,218,219,442,
  492,538,
704,702,701,700,699,698,697,696,695,694,693,692,691,688,687,686,685,684,683,
  682,537,535,534,533,532,531,530,529,528,527,526,525,524,521,518,517,491,
  460,429,231,135,134,0,2,6,7,8,9,15,17,21,25,27,29,89,91,131,133,196,198,
  203,204,205,206,207,208,209,210,211,212,213,214,215,216,217,218,219,442,
  492,538,
704,702,701,700,699,698,697,696,695,694,693,692,691,688,687,686,685,684,683,
  682,537,535,534,533,532,531,530,529,528,527,526,525,524,521,518,517,491,
  460,429,231,135,134,0,2,6,7,8,9,15,17,21,25,27,29,89,91,131,133,196,198,
  203,204,205,206,207,208,209,210,211,212,213,214,215,216,217,218,219,442,
  492,538,
704,702,701,700,699,698,697,696,695,694,693,692,691,688,687,686,685,684,683,
  682,537,535,534,533,532,531,530,529,528,527,526,525,524,521,518,517,491,
  460,429,231,135,134,0,2,6,7,8,9,15,17,21,25,27,29,89,91,131,133,196,198,
  203,204,205,206,207,208,209,210,211,212,213,214,215,216,217,218,219,442,
  492,538,
704,702,701,700,699,698,697,696,695,694,693,692,691,688,687,686,685,684,683,
  682,537,535,534,533,532,531,530,529,528,527,526,525,524,521,518,517,491,
  460,429,231,135,134,0,2,6,7,8,9,15,17,21,24,25,27,29,89,91,131,133,196,
  198,203,204,205,206,207,208,209,210,211,212,213,214,215,216,217,218,219,
  442,492,538,
704,702,701,700,699,698,697,696,695,694,693,692,691,688,687,686,685,684,683,
  682,537,535,534,533,532,531,530,529,528,527,526,525,524,521,518,517,491,
  460,429,231,135,134,0,2,6,7,8,9,15,17,21,24,25,27,29,89,91,131,133,196,
  198,203,204,205,206,207,208,209,210,211,212,213,214,215,216,217,218,219,
  442,492,538,
704,702,701,700,699,698,697,696,695,694,693,692,691,688,687,686,685,684,683,
  682,537,535,534,533,532,531,530,529,528,527,526,525,524,521,518,517,491,
  460,429,231,135,134,0,2,6,7,8,9,15,17,21,23,24,25,27,29,89,91,131,133,
  196,198,203,204,205,206,207,208,209,210,211,212,213,214,215,216,217,218,
  219,442,492,538,
704,702,701,700,699,698,697,696,695,694,693,692,691,688,687,686,685,684,683,
  682,537,535,534,533,532,531,530,529,528,527,526,525,524,521,518,517,491,
  460,429,231,135,134,0,2,6,7,8,9,15,17,21,23,24,25,27,29,89,91,131,133,
  196,198,203,204,205,206,207,208,209,210,211,212,213,214,215,216,217,218,
  219,442,492,538,
704,702,701,700,699,698,697,696,695,694,693,692,691,688,687,686,685,684,683,
  682,537,535,534,533,532,531,530,529,528,527,526,525,524,521,518,517,491,
  460,429,231,135,134,0,2,6,7,8,9,15,17,21,23,24,25,27,28,29,89,91,131,
  133,196,198,203,204,205,206,207,208,209,210,211,212,213,214,215,216,217,
  218,219,442,492,538,
704,702,701,700,699,698,697,696,695,694,693,692,691,688,687,686,685,684,683,
  682,537,535,534,533,532,531,530,529,528,527,526,525,524,521,518,517,491,
  460,429,231,135,134,0,2,6,7,8,9,15,17,21,23,24,25,27,28,29,89,91,131,
  133,196,198,203,204,205,206,207,208,209,210,211,212,213,214,215,216,217,
  218,219,442,492,538,
427,425,424,423,38,1,0,51,52,
427,425,424,423,38,0,39,40,41,45,46,47,49,
427,425,424,423,38,0,39,40,41,45,46,47,49,
427,425,424,423,38,0,39,40,41,45,46,47,49,
427,425,424,423,38,0,39,40,41,45,46,47,49,
427,425,424,423,38,1,0,51,52,
427,425,424,423,38,1,0,51,52,
427,425,424,423,38,0,39,40,41,45,46,47,49,
704,702,701,700,699,698,697,696,695,694,693,692,691,688,687,686,685,684,683,
  682,537,535,534,533,532,531,530,529,528,527,526,525,524,521,518,517,491,
  460,429,231,135,134,0,2,6,7,8,9,15,17,21,23,24,25,26,27,28,29,57,89,91,
  131,133,196,198,203,204,205,206,207,208,209,210,211,212,213,214,215,216,
  217,218,219,442,492,538,
704,702,701,700,699,698,697,696,695,694,693,692,691,688,687,686,685,684,683,
  682,537,535,534,533,532,531,530,529,528,527,526,525,524,521,518,517,491,
  460,429,231,135,134,0,2,6,7,8,9,15,17,21,23,24,25,26,27,28,29,57,89,91,
  131,133,196,198,203,204,205,206,207,208,209,210,211,212,213,214,215,216,
  217,218,219,442,492,538,
704,702,701,700,699,698,697,696,695,694,693,692,691,688,687,686,685,684,683,
  682,537,535,534,533,532,531,530,529,528,527,526,525,524,521,518,517,491,
  460,429,231,135,134,0,2,6,7,8,9,15,17,21,23,24,25,26,27,28,29,57,89,91,
  131,133,196,198,203,204,205,206,207,208,209,210,211,212,213,214,215,216,
  217,218,219,442,492,538,
493,0,132,
704,703,702,701,700,699,698,697,696,695,694,693,692,691,690,689,688,687,686,
  685,684,683,682,681,680,526,524,521,520,519,518,517,511,509,507,460,459,
  455,429,425,423,233,1,0,16,
704,702,701,700,699,698,697,696,695,694,693,692,691,688,687,686,685,684,683,
  682,681,540,537,535,534,533,532,531,530,529,528,527,526,525,524,521,518,
  517,491,460,429,241,240,239,238,237,236,235,234,231,135,134,1,0,51,52,
704,702,701,700,699,698,697,696,695,694,693,692,691,688,687,686,685,684,683,
  682,681,537,535,534,533,532,531,530,529,528,527,526,525,524,521,518,517,
  491,460,429,231,135,134,1,0,51,52,
544,0,267,
544,427,425,424,423,38,1,0,51,52,
704,703,702,701,700,699,698,697,696,695,694,693,692,691,690,689,688,687,686,
  685,684,682,681,680,526,524,521,520,519,518,517,511,509,507,460,459,455,
  429,425,423,233,1,0,16,
704,702,701,700,699,698,697,696,695,694,693,692,691,688,687,686,685,684,683,
  682,537,535,534,533,532,531,530,529,528,527,526,525,524,521,518,517,491,
  460,429,231,135,134,1,0,51,52,
493,427,425,424,423,38,1,0,132,
704,702,701,700,699,698,697,696,695,694,693,691,682,135,134,1,0,51,52,
705,704,702,701,700,699,698,697,696,695,694,693,692,691,690,689,688,687,686,
  685,684,683,682,681,680,526,524,521,520,519,518,517,511,509,507,461,460,
  459,455,429,427,425,424,423,38,1,0,51,52,
705,704,702,701,700,699,698,697,696,695,694,693,692,691,690,689,688,687,686,
  685,684,683,682,681,680,526,524,521,520,519,518,517,511,509,507,461,460,
  459,455,429,427,425,424,423,38,1,0,
427,425,424,423,38,0,39,40,41,45,46,47,49,
704,702,701,700,699,698,697,696,695,694,693,692,691,688,687,686,685,684,683,
  682,537,535,534,533,532,531,530,529,528,527,526,525,524,521,518,517,491,
  460,429,231,135,134,0,2,6,7,8,9,15,17,21,23,24,25,26,27,28,29,57,89,91,
  131,133,196,198,203,204,205,206,207,208,209,210,211,212,213,214,215,216,
  217,218,219,442,492,538,
704,703,702,701,700,699,698,697,696,695,694,693,692,691,690,689,688,687,686,
  685,684,683,682,681,680,526,524,521,520,519,518,517,511,509,507,460,459,
  455,429,425,423,1,0,
688,687,686,685,684,0,
704,702,701,700,699,698,697,696,695,694,693,691,682,135,134,0,3,133,442,536,
459,0,93,
459,0,93,
459,0,93,
459,0,93,
459,0,93,
459,0,93,
459,0,93,
459,0,93,
459,0,93,
459,0,93,
459,0,93,
544,523,522,521,520,519,518,517,516,515,514,513,512,511,510,509,508,507,506,
  505,504,503,502,501,500,499,498,497,496,495,493,459,432,425,423,38,1,0,
  197,199,200,201,202,
544,523,522,521,520,519,518,517,516,515,514,513,512,511,510,509,508,507,506,
  505,504,503,502,501,500,499,498,497,496,495,493,459,432,425,423,38,1,0,
  197,199,200,201,202,
544,518,517,516,515,514,513,512,511,510,509,508,507,506,505,504,503,502,501,
  500,499,498,497,496,495,493,459,432,427,425,424,423,38,1,0,195,196,
544,518,517,516,515,514,513,512,511,510,509,508,507,506,505,504,503,502,501,
  500,499,498,497,496,495,493,459,432,427,425,424,423,38,1,0,195,196,
544,518,517,516,515,514,513,512,511,510,509,508,507,506,505,504,503,502,501,
  500,499,498,497,496,495,493,459,432,427,425,424,423,38,1,0,195,196,
544,518,517,516,515,514,513,512,511,510,509,508,507,506,505,504,503,502,501,
  500,499,498,497,496,495,493,459,432,427,425,424,423,38,1,0,195,196,
544,516,515,514,513,512,511,510,509,508,507,506,505,504,503,502,501,500,499,
  498,497,496,495,493,459,432,427,425,424,423,38,1,0,191,192,193,194,
544,516,515,514,513,512,511,510,509,508,507,506,505,504,503,502,501,500,499,
  498,497,496,495,493,459,432,427,425,424,423,38,1,0,191,192,193,194,
544,512,511,510,509,508,507,506,505,504,503,502,501,500,499,498,497,496,495,
  493,459,432,427,425,424,423,38,1,0,189,190,
544,512,511,510,509,508,507,506,505,504,503,502,501,500,499,498,497,496,495,
  493,459,432,427,425,424,423,38,1,0,189,190,
544,510,509,508,507,506,505,504,503,502,501,500,499,498,497,496,495,493,459,
  432,427,425,424,423,38,1,0,187,188,
544,510,509,508,507,506,505,504,503,502,501,500,499,498,497,496,495,493,459,
  432,427,425,424,423,38,1,0,187,188,
427,425,424,423,38,0,39,40,41,45,46,47,49,
427,425,424,423,38,0,39,40,41,45,46,47,49,
427,425,424,423,38,0,39,40,41,45,46,47,49,
540,241,240,239,238,237,236,235,234,1,0,51,52,
704,703,702,701,700,699,698,697,696,695,694,693,692,691,690,689,688,687,686,
  685,684,683,682,681,680,526,524,521,520,519,518,517,511,509,507,460,459,
  455,429,425,423,233,1,0,
704,702,701,700,699,698,697,696,695,694,693,692,691,688,687,686,685,684,683,
  682,681,537,535,534,533,532,531,530,529,528,527,526,525,524,521,518,517,
  491,460,429,231,135,134,0,2,6,7,8,9,10,12,15,17,18,20,21,23,24,25,26,27,
  28,29,57,89,91,131,133,196,198,203,204,205,206,207,208,209,210,211,212,
  213,214,215,216,217,218,219,435,442,485,492,538,
704,702,701,700,699,698,697,696,695,694,693,692,691,688,687,686,685,684,683,
  682,537,535,534,533,532,531,530,529,528,527,526,525,524,521,518,517,491,
  460,429,231,135,134,0,2,6,7,8,9,15,17,21,23,24,25,26,27,28,29,57,89,91,
  131,133,196,198,203,204,205,206,207,208,209,210,211,212,213,214,215,216,
  217,218,219,442,492,538,
704,702,701,700,699,698,697,696,695,694,693,691,682,135,134,0,21,133,442,
427,425,424,423,38,1,0,51,52,
704,702,701,700,699,698,697,696,695,694,693,691,688,687,686,685,684,682,459,
  429,1,0,51,52,138,
459,1,0,51,52,
459,1,0,51,52,
540,241,240,239,238,237,236,235,234,0,4,242,594,
427,425,424,423,38,0,39,40,41,45,46,47,49,
459,0,93,

};


static unsigned const char ag_astt[28270] = {
  8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,7,1,1,9,5,2,2,2,2,
  2,2,2,2,2,2,2,2,2,1,8,8,8,8,8,2,2,2,2,2,1,8,1,7,1,0,1,1,1,1,1,1,1,1,1,1,1,
  5,5,5,5,5,5,5,5,5,5,5,5,5,5,1,7,1,3,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,4,
  4,2,4,4,4,4,2,4,4,4,7,2,1,1,1,1,1,1,1,1,1,1,1,1,1,1,7,1,3,1,1,1,1,1,1,1,1,
  1,1,1,1,1,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,
  5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,
  5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,
  5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,
  5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,
  5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,1,7,1,3,1,7,2,8,8,1,1,1,1,1,3,7,3,3,1,3,
  1,1,1,1,1,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,
  8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,
  8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,
  8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,
  8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,
  8,8,8,8,8,8,8,5,5,8,8,8,8,8,8,8,8,1,7,1,1,2,2,2,2,2,2,2,2,2,2,2,2,2,1,1,1,
  1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,
  1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,
  1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,
  1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,
  8,8,8,8,8,8,8,8,8,8,8,8,8,8,1,1,1,8,8,8,8,8,5,5,1,5,5,5,1,2,2,5,9,7,1,1,1,
  1,1,3,1,1,2,1,2,2,1,1,1,1,1,1,1,1,1,1,1,1,1,1,2,2,2,2,2,2,2,2,2,2,2,2,2,2,
  2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,1,1,1,
  1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,
  1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,
  1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,2,2,2,2,2,2,2,2,2,2,2,2,2,1,8,8,8,
  8,8,2,2,2,2,2,1,3,1,7,1,3,3,1,1,3,1,1,1,1,1,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,
  1,7,1,3,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,1,7,1,3,5,5,7,1,3,5,5,7,1,3,5,5,5,5,
  5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,
  5,5,5,7,1,3,5,1,7,1,3,5,5,5,5,5,5,7,1,3,5,5,5,5,5,5,7,1,3,5,5,5,5,5,5,5,5,
  5,5,5,5,5,5,5,1,7,1,3,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,
  5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,1,7,1,3,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,
  5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,1,7,1,3,5,5,5,5,5,5,5,5,
  5,5,5,5,5,5,5,1,7,1,3,5,5,5,5,5,1,7,1,3,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,
  5,5,5,5,1,7,1,3,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,7,1,1,1,1,2,2,2,2,2,2,2,2,2,
  2,2,2,2,2,2,7,1,1,1,8,1,7,1,1,8,1,7,1,1,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,
  8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,1,7,1,1,2,7,1,1,1,8,8,
  8,8,8,1,7,1,1,8,8,8,8,8,1,7,1,1,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,7,1,1,1,2,2,
  2,2,2,2,2,2,2,2,2,1,2,2,2,2,2,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,
  2,2,2,7,2,1,1,2,1,2,1,1,1,1,1,1,1,1,1,1,1,2,1,1,1,1,1,1,1,1,1,2,1,1,1,1,1,
  1,1,1,1,1,1,1,1,1,1,1,2,2,2,2,2,2,2,2,2,2,2,1,2,2,2,2,2,1,1,1,1,1,1,1,1,1,
  1,1,1,1,1,1,1,1,1,1,1,1,1,2,2,2,7,2,1,1,2,1,2,1,1,1,1,1,1,1,1,1,1,1,2,1,1,
  1,1,1,1,1,1,1,2,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,2,2,2,2,2,2,2,2,2,2,2,2,2,
  2,2,7,1,1,1,1,1,1,1,1,7,1,1,1,1,1,2,2,7,1,1,1,1,1,1,5,5,5,5,5,5,5,5,5,5,5,
  5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,
  5,5,5,5,5,5,5,7,1,3,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,
  5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,
  5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,7,1,3,1,1,7,1,1,1,5,5,5,
  5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,
  5,5,5,5,5,7,1,3,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,
  5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,7,1,3,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,
  5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,7,1,3,1,1,1,1,1,1,
  1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,
  1,9,7,3,3,3,1,3,1,3,3,3,3,3,3,3,3,3,3,3,3,3,3,3,3,3,3,3,3,3,3,3,3,3,3,3,3,
  3,3,3,3,3,3,3,3,3,3,3,3,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,
  1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,8,1,7,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,
  1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,
  1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,8,1,7,1,1,1,1,1,
  1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,
  1,2,2,2,2,2,2,2,2,2,2,2,2,2,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,
  1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,
  1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,
  1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,8,
  8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,1,1,1,8,8,8,
  8,8,1,8,8,1,1,1,1,1,2,2,3,7,1,1,1,1,1,3,3,1,3,1,1,1,1,1,1,2,1,2,2,1,1,1,1,
  1,1,1,1,1,1,1,1,1,1,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,
  2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,
  1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,
  1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,
  1,1,1,1,1,1,1,8,8,8,8,8,8,8,1,7,1,1,5,5,5,5,5,5,5,7,1,3,5,5,5,5,5,5,5,7,1,
  3,5,5,5,5,5,5,5,7,1,3,5,5,5,5,5,5,5,7,1,3,5,5,5,5,5,5,5,7,1,3,5,5,5,5,5,5,
  5,7,1,3,5,5,5,5,5,5,5,7,1,3,5,5,5,5,5,5,5,7,1,3,5,5,5,5,5,5,5,7,1,3,5,5,5,
  5,5,5,5,7,1,3,5,5,5,5,5,5,5,7,1,3,5,5,5,5,5,5,5,7,1,3,5,5,5,5,5,5,5,7,1,3,
  5,5,5,5,5,5,5,7,1,3,5,5,5,5,5,5,5,5,5,5,7,1,3,5,5,5,5,5,5,5,5,5,5,7,1,3,5,
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
  5,5,5,7,1,3,5,5,5,5,5,5,5,7,1,3,5,5,5,5,5,5,5,7,1,3,5,5,5,5,5,5,5,7,1,3,5,
  5,5,5,5,5,5,7,1,3,5,5,5,5,5,5,5,7,1,3,5,5,5,5,5,5,5,7,1,3,5,5,5,5,5,5,5,7,
  1,3,5,5,5,5,5,5,5,7,1,3,5,5,5,5,5,5,5,7,1,3,5,5,5,5,5,5,5,7,1,3,5,5,5,5,5,
  5,5,7,1,3,5,5,5,5,5,5,5,7,1,3,5,5,5,5,5,5,5,7,1,3,5,5,5,5,5,5,5,7,1,3,5,5,
  5,5,5,5,5,7,1,3,5,5,5,5,5,5,5,7,1,3,5,5,5,5,5,5,5,7,1,3,5,5,5,5,5,5,5,7,1,
  3,5,5,5,5,5,5,5,7,1,3,5,5,5,5,5,5,5,7,1,3,5,5,5,5,5,5,5,7,1,3,5,5,5,5,5,5,
  5,7,1,3,5,5,5,5,5,5,5,7,1,3,5,5,5,5,5,5,5,7,1,3,5,5,5,5,5,5,5,7,1,3,5,5,5,
  5,5,5,5,7,1,3,5,5,5,5,5,5,5,7,1,3,5,5,5,5,5,5,5,7,1,3,5,5,5,5,5,5,5,7,1,3,
  5,5,5,5,5,7,1,3,8,8,8,8,8,8,8,8,8,1,7,1,1,8,8,8,8,8,8,8,8,8,1,7,1,1,8,8,8,
  8,8,8,8,1,7,1,1,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,
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
  5,5,5,5,5,7,1,3,5,5,5,5,5,5,5,5,5,7,1,3,5,5,5,5,5,5,5,5,5,7,1,3,5,5,5,5,5,
  5,5,5,5,7,1,3,5,5,5,5,5,7,1,3,5,5,5,5,5,7,1,3,5,5,5,5,5,5,5,5,7,1,3,5,5,5,
  5,5,5,5,5,7,1,3,5,5,5,5,5,5,5,5,7,1,3,5,5,5,5,5,5,5,5,5,5,7,1,3,5,5,5,5,5,
  5,5,5,5,5,7,1,3,5,5,5,5,5,5,5,5,5,5,7,1,3,5,5,5,5,5,5,5,5,5,5,7,1,3,5,5,5,
  5,5,5,5,5,5,5,7,1,3,5,5,5,5,5,5,5,5,5,5,7,1,3,5,5,5,5,5,5,5,5,5,5,7,1,3,5,
  5,5,5,5,5,5,5,5,5,7,1,3,5,5,5,5,5,5,5,5,5,5,7,1,3,5,5,5,5,5,5,5,5,5,5,7,1,
  3,5,5,5,5,5,5,5,5,5,5,7,1,3,5,5,5,5,5,5,5,5,5,5,7,1,3,5,5,5,5,5,5,5,5,5,5,
  7,1,3,8,8,8,8,1,7,1,1,8,1,7,1,1,8,1,7,1,1,8,1,7,1,1,8,8,8,8,8,8,8,8,8,8,8,
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
  8,8,8,8,8,8,8,8,8,1,7,1,1,8,8,8,8,8,8,8,8,1,7,1,1,8,8,8,8,8,8,8,8,1,7,1,1,
  8,8,8,8,8,8,8,8,1,7,1,1,8,8,8,8,8,8,8,8,1,7,1,1,8,8,8,8,1,7,1,1,8,8,8,8,1,
  7,1,1,8,8,8,8,8,8,8,1,7,1,1,8,8,8,8,8,8,8,1,7,1,1,8,8,8,8,8,8,8,1,7,1,1,8,
  8,8,8,8,8,8,8,8,1,7,1,1,8,8,8,8,8,8,8,8,8,1,7,1,1,8,8,8,8,8,8,8,8,8,1,7,1,
  1,8,8,8,8,8,8,8,8,8,1,7,1,1,8,8,8,8,8,8,8,8,8,1,7,1,1,8,8,8,8,8,8,8,8,8,1,
  7,1,1,8,8,8,8,8,8,8,8,8,1,7,1,1,8,8,8,8,8,8,8,8,8,1,7,1,1,8,8,8,8,8,8,8,8,
  8,1,7,1,1,8,8,8,8,8,8,8,8,8,1,7,1,1,8,8,8,8,8,8,8,8,8,1,7,1,1,8,8,8,8,8,8,
  8,8,8,1,7,1,1,8,8,8,8,8,8,8,8,8,1,7,1,1,5,2,2,2,2,2,2,2,2,2,2,2,5,2,5,5,2,
  2,2,2,2,5,2,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,2,5,5,5,
  5,5,1,7,1,3,2,5,5,5,5,5,5,1,7,1,3,4,4,4,4,4,4,4,4,4,4,4,4,4,4,4,4,4,4,4,4,
  4,4,4,4,4,4,4,4,4,4,4,4,4,4,4,4,4,4,4,4,4,4,4,4,4,4,4,4,4,4,4,4,4,4,4,4,4,
  4,1,4,4,4,4,4,4,4,4,4,4,7,1,4,4,4,4,4,1,7,1,1,5,1,2,2,2,2,2,2,2,2,2,2,2,1,
  2,2,2,2,2,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,2,2,2,7,2,1,1,2,1,2,
  1,1,1,1,1,1,1,1,1,1,2,2,1,1,1,1,1,1,1,1,1,2,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,
  1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,
  1,7,2,2,1,2,2,1,1,1,2,2,1,1,1,1,2,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,2,2,
  1,2,2,1,1,2,8,8,8,8,8,1,7,1,1,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,
  8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,1,7,1,1,8,8,8,8,8,1,7,1,1,2,7,1,1,1,2,
  7,1,1,1,2,2,2,2,2,2,2,2,2,2,2,1,2,2,2,2,2,1,1,1,2,1,1,1,1,1,1,1,1,1,1,1,1,
  1,1,1,1,1,1,1,2,2,2,7,2,1,1,2,1,1,1,2,1,1,1,1,1,1,1,1,1,1,2,1,1,1,1,1,1,1,
  1,2,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,2,1,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,
  2,2,2,2,2,3,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,7,2,5,5,5,5,5,5,5,5,5,7,1,
  3,8,8,8,8,8,1,7,1,1,1,1,1,1,2,7,2,2,1,2,1,1,1,1,1,1,1,2,7,2,2,1,2,1,1,1,8,
  8,8,8,8,1,7,1,1,2,2,2,2,2,7,2,2,2,2,7,2,2,7,4,4,4,4,4,4,4,2,2,2,2,4,2,2,2,
  2,2,4,4,4,4,4,4,4,4,4,4,4,4,4,4,4,4,4,4,4,4,4,4,4,4,4,4,4,4,4,4,4,4,4,4,4,
  4,4,4,4,4,4,4,7,4,1,4,4,4,4,4,4,4,4,4,4,4,4,4,4,4,4,4,4,4,4,4,4,4,4,4,4,4,
  4,4,4,4,4,4,4,4,4,4,4,4,4,4,4,4,4,4,4,4,4,4,4,4,4,4,7,2,2,2,2,2,7,2,1,2,2,
  2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,
  2,7,1,10,10,10,10,10,5,2,10,10,10,10,10,10,10,10,10,7,10,10,10,10,4,4,4,4,
  4,4,4,4,4,4,4,4,4,4,4,4,4,4,4,4,4,4,4,4,4,4,4,4,4,4,4,4,4,4,4,4,4,4,4,7,10,
  10,4,4,4,4,4,4,4,4,4,4,4,4,4,4,4,4,4,4,4,4,4,4,4,4,4,4,4,4,4,4,4,4,4,4,4,4,
  4,4,4,7,10,10,10,10,10,10,10,10,10,4,4,4,4,4,4,4,4,4,4,4,4,4,4,4,4,4,4,4,4,
  4,4,4,4,4,4,4,4,4,4,4,4,4,4,4,4,4,4,4,7,9,2,2,1,1,2,10,10,10,10,10,9,4,4,4,
  4,4,4,4,4,4,4,4,4,4,4,4,4,4,4,4,4,4,4,4,4,4,4,4,4,4,4,4,2,4,4,4,4,2,2,4,4,
  7,2,5,5,7,1,3,5,5,5,5,5,5,5,7,1,3,5,5,7,1,3,5,5,7,1,3,5,5,7,1,3,5,5,7,1,3,
  5,5,7,1,3,5,5,7,1,3,5,5,7,1,3,5,5,7,1,3,5,5,7,1,3,5,5,5,5,5,5,5,5,5,5,5,5,
  5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,7,1,3,5,5,5,5,5,5,5,
  5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,1,7,1,3,5,5,
  5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,
  5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,7,1,3,8,1,7,1,1,8,1,7,1,1,
  8,1,7,1,1,8,1,7,1,1,8,1,7,1,1,8,1,7,1,1,8,1,7,1,1,8,1,7,1,1,8,1,7,1,1,8,1,
  7,1,1,8,1,7,1,1,5,5,5,5,5,5,5,5,5,5,5,5,5,2,2,2,2,2,5,5,5,5,5,5,5,5,5,5,5,
  5,5,5,5,5,5,5,5,5,5,1,7,1,3,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,
  5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,
  5,5,7,1,3,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,
  5,5,5,5,5,5,5,1,7,1,3,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,
  5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,7,
  1,3,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,
  8,8,8,8,8,8,8,1,7,1,1,2,2,2,2,2,2,2,2,2,2,2,1,2,2,2,2,2,1,1,1,1,1,1,1,1,1,
  1,1,1,1,1,1,1,1,1,1,2,2,2,7,2,1,1,2,1,2,1,1,2,1,1,1,2,2,2,1,1,1,1,1,1,1,1,
  1,1,1,1,1,1,1,2,2,2,2,2,2,2,2,2,2,2,1,2,2,2,2,2,1,1,1,1,1,1,1,1,1,1,1,1,1,
  1,1,1,1,1,1,2,2,2,7,2,1,1,2,1,2,1,1,2,1,1,1,2,2,2,1,1,1,1,1,1,1,1,1,1,1,1,
  1,1,1,2,2,2,2,2,2,2,2,2,2,2,1,2,2,2,2,2,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,
  1,1,2,2,2,7,2,1,1,2,1,2,1,1,2,1,1,1,2,2,2,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,2,
  2,2,2,2,2,2,2,2,2,2,1,2,2,2,2,2,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,2,2,
  2,7,2,1,1,2,1,2,1,1,2,1,1,1,2,2,2,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,5,5,5,5,5,
  5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,1,7,1,3,
  5,1,1,1,1,1,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,1,
  7,1,3,1,1,1,1,1,5,1,1,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,
  5,5,5,5,1,7,1,3,1,1,5,1,1,1,1,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,
  5,5,5,5,1,7,1,3,1,1,1,1,5,1,1,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,
  5,5,1,7,1,3,1,1,5,1,1,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,1,7,1,3,
  1,1,1,1,5,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,4,4,4,4,4,4,7,2,2,2,2,2,2,2,2,2,2,
  2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,1,2,2,2,2,2,1,1,1,1,1,1,1,1,1,1,1,
  1,1,1,1,1,1,1,1,1,1,1,2,2,2,7,2,1,1,2,1,2,1,1,1,1,1,1,1,1,1,2,2,1,1,1,1,1,
  1,1,1,2,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,8,8,8,8,8,1,7,1,1,8,8,8,8,8,1,7,1,
  1,8,8,8,8,8,1,7,1,5,5,5,5,5,5,7,1,3,8,8,8,8,8,1,7,1,1,5,5,5,5,5,5,7,1,3,8,
  8,8,8,8,1,7,1,1,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,7,1,3,8,8,8,8,8,8,8,8,8,8,
  8,8,8,8,8,1,7,1,1,5,5,5,5,5,5,7,1,3,8,8,8,8,8,1,7,1,1,5,5,5,5,5,5,7,1,3,8,
  8,8,8,8,1,7,1,1,10,10,1,10,10,10,10,10,10,10,10,10,10,10,10,2,10,10,10,10,
  10,10,10,10,10,10,10,10,10,10,10,10,10,10,10,10,10,10,10,10,10,7,5,5,5,5,5,
  5,7,1,3,8,8,8,8,8,1,7,1,1,8,8,8,8,8,1,7,1,1,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,
  5,5,5,5,5,5,1,7,1,3,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,7,1,1,1,1,1,
  1,1,1,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,
  5,5,5,5,5,5,5,5,5,5,5,5,5,5,7,1,3,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,
  5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,
  5,5,5,5,5,5,7,1,3,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,
  5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,7,1,3,5,5,5,5,5,5,5,5,5,5,5,5,5,5,
  5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,
  5,5,5,5,5,5,5,5,5,5,5,5,7,1,3,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,
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
  5,7,1,3,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,
  5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,7,1,3,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,
  5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,7,1,3,5,5,5,5,5,
  5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,
  5,5,5,5,5,5,7,1,3,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,
  5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,7,1,3,5,5,5,5,5,5,5,5,5,5,5,5,5,5,
  5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,7,1,3,
  5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,
  5,5,5,5,5,5,5,5,5,5,5,7,1,3,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,
  5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,7,1,3,5,5,5,5,5,5,5,5,5,
  5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,
  5,5,7,1,3,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,
  5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,7,1,3,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,
  5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,7,1,3,5,5,5,5,
  5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,
  5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,7,1,3,5,5,5,5,5,5,5,5,5,5,5,5,5,
  5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,
  5,5,5,5,5,5,5,5,5,5,5,5,5,7,1,3,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,
  5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,
  5,5,5,5,5,7,1,3,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,
  5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,7,1,3,
  5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,
  5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,7,1,3,5,5,5,5,5,5,5,5,5,
  5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,
  5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,7,1,3,5,5,5,5,5,5,5,5,
  5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,
  5,5,5,7,1,3,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,
  5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,7,1,3,5,5,
  5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,
  5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,7,1,3,5,5,5,5,5,5,5,5,5,5,5,
  5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,
  7,1,3,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,
  5,5,5,5,5,5,5,5,5,5,5,5,5,5,7,1,3,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,
  5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,7,1,3,5,5,5,5,5,5,
  5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,
  5,5,5,5,5,5,5,5,5,5,5,5,7,1,3,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,
  1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,9,5,3,3,3,3,3,3,3,3,3,3,3,3,3,3,3,3,
  3,3,3,3,3,3,3,3,3,3,3,3,3,3,3,3,3,3,3,3,3,3,3,3,3,1,7,3,1,7,3,5,5,5,5,5,5,
  5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,1,
  7,1,3,2,2,2,2,2,2,2,2,2,2,2,1,2,2,2,2,2,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,
  1,1,1,1,1,2,2,2,7,2,1,1,2,1,2,1,1,1,1,1,1,1,1,1,1,2,1,1,1,1,1,1,1,1,2,1,1,
  1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,
  1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,7,1,1,2,2,1,2,2,1,1,1,2,2,1,1,1,1,2,1,1,1,
  1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,2,2,1,2,2,1,1,2,8,8,8,8,8,1,7,1,1,1,1,1,1,1,
  1,3,7,1,3,3,1,3,1,1,1,2,2,2,1,1,1,2,2,2,2,2,7,2,2,2,2,3,2,2,2,2,2,2,2,2,2,
  2,2,7,3,2,1,2,2,2,2,2,2,2,7,3,2,2,2,1,2,2,2,2,7,2,1,7,1,1,7,1,1,7,1,2,2,2,
  2,2,2,2,2,2,2,2,1,2,2,2,2,2,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,2,
  2,2,7,2,1,1,2,1,2,1,1,1,1,1,1,1,1,1,2,2,1,1,1,1,1,1,1,1,2,1,1,1,1,1,1,1,1,
  1,1,1,1,1,1,1,1,2,2,2,2,2,2,2,2,2,2,2,1,2,2,2,2,2,1,1,1,1,1,1,1,1,1,1,1,1,
  1,1,1,1,1,1,1,1,1,1,2,2,2,7,2,1,1,2,1,2,1,1,1,1,1,1,1,1,1,2,2,1,1,1,1,1,1,
  1,1,2,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,2,2,2,2,2,2,2,2,2,2,2,1,2,2,2,2,2,1,
  1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,2,2,2,7,2,1,1,2,1,2,1,1,1,1,1,1,
  1,1,1,2,2,1,1,1,1,1,1,1,1,2,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,2,2,2,2,2,2,2,
  2,2,2,2,1,2,2,2,2,2,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,2,2,2,7,2,
  1,1,2,1,2,1,1,1,1,1,1,1,1,1,2,2,1,1,1,1,1,1,1,1,2,1,1,1,1,1,1,1,1,1,1,1,1,
  1,1,1,1,2,2,2,2,2,2,2,2,2,2,2,1,2,2,2,2,2,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,
  1,1,1,1,1,1,2,2,2,7,2,1,1,2,1,2,1,1,1,1,1,1,1,1,1,2,2,1,1,1,1,1,1,1,1,2,1,
  1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,2,2,2,2,2,2,2,2,2,2,2,1,2,2,2,2,2,1,1,1,1,1,
  1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,2,2,2,7,2,1,1,2,1,2,1,1,1,1,1,1,1,1,1,2,
  2,1,1,1,1,1,1,1,1,2,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,2,2,2,2,2,2,2,2,2,2,2,
  1,2,2,2,2,2,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,2,2,2,7,2,1,1,2,1,
  2,1,1,1,1,1,1,1,1,1,2,2,1,1,1,1,1,1,1,1,2,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,
  2,2,2,2,2,2,2,2,2,2,2,1,2,2,2,2,2,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,
  1,1,2,2,2,7,2,1,1,2,1,2,1,1,1,1,1,1,1,1,1,2,2,1,1,1,1,1,1,1,1,2,1,1,1,1,1,
  1,1,1,1,1,1,1,1,1,1,1,2,2,2,2,2,2,2,2,2,2,2,1,2,2,2,2,2,1,1,1,1,1,1,1,1,1,
  1,1,1,1,1,1,1,1,1,1,1,1,1,2,2,2,7,2,1,1,2,1,2,1,1,1,1,1,1,1,1,1,2,2,1,1,1,
  1,1,1,1,1,2,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,2,2,2,2,2,2,2,2,2,2,2,1,2,2,2,
  2,2,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,2,2,2,7,2,1,1,2,1,2,1,1,1,
  1,1,1,1,1,1,2,2,1,1,1,1,1,1,1,1,2,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,2,2,2,2,
  2,2,2,2,2,2,2,1,2,2,2,2,2,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,2,2,
  2,7,2,1,1,2,1,2,1,1,1,1,1,1,1,1,1,2,2,1,1,1,1,1,1,1,1,2,1,1,1,1,1,1,1,1,1,
  1,1,1,1,1,1,1,2,2,2,2,2,2,2,2,2,2,2,1,2,2,2,2,2,1,1,1,1,1,1,1,1,1,1,1,1,1,
  1,1,1,1,1,1,1,1,1,2,2,2,7,2,1,1,2,1,2,1,1,1,1,1,1,1,1,1,2,2,1,1,1,1,1,1,1,
  1,2,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,2,2,2,2,2,2,2,2,2,2,2,1,2,2,2,2,2,1,1,
  1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,2,2,2,7,2,1,1,2,1,2,1,1,1,1,1,1,1,
  1,1,2,2,1,1,1,1,1,1,1,1,2,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,2,2,2,2,2,2,2,2,
  2,2,2,1,2,2,2,2,2,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,2,2,2,7,2,1,
  1,2,1,2,1,1,1,1,1,1,1,1,1,2,2,1,1,1,1,1,1,1,1,2,1,1,1,1,1,1,1,1,1,1,1,1,1,
  1,1,1,2,2,2,2,2,2,2,2,2,2,2,1,2,2,2,2,2,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,
  1,1,1,1,1,2,2,2,7,2,1,1,2,1,2,1,1,1,1,1,1,1,1,1,2,2,1,1,1,1,1,1,1,1,2,1,1,
  1,1,1,1,1,1,1,1,1,1,1,1,1,1,2,2,2,2,2,2,2,2,2,2,2,1,2,2,2,2,2,1,1,1,1,1,1,
  1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,2,2,2,7,2,1,1,2,1,2,1,1,1,1,1,1,1,1,1,2,2,
  1,1,1,1,1,1,1,1,2,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,2,2,2,2,2,2,2,2,2,2,2,1,
  2,2,2,2,2,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,2,2,2,7,2,1,1,2,1,2,
  1,1,1,1,1,1,1,1,1,2,2,1,1,1,1,1,1,1,1,2,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,2,
  2,2,2,2,2,2,2,2,2,2,1,2,2,2,2,2,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,
  1,2,2,2,7,2,1,1,2,1,2,1,1,1,1,1,1,1,1,1,2,2,1,1,1,1,1,1,1,1,2,1,1,1,1,1,1,
  1,1,1,1,1,1,1,1,1,1,2,2,2,2,2,2,2,2,2,2,2,1,2,2,2,2,2,1,1,1,1,1,1,1,1,1,1,
  1,1,1,1,1,1,1,1,1,1,1,1,2,2,2,7,2,1,1,2,1,2,1,1,1,1,1,1,1,1,1,2,2,1,1,1,1,
  1,1,1,1,2,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,2,2,2,2,2,2,2,2,2,2,2,1,2,2,2,2,
  2,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,2,2,2,7,2,1,1,2,1,2,1,1,1,1,
  1,1,1,1,1,2,2,1,1,1,1,1,1,1,1,2,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,2,2,2,2,2,
  2,2,2,2,2,2,1,2,2,2,2,2,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,2,2,2,
  7,2,1,1,2,1,2,1,1,1,1,1,1,1,1,1,2,2,1,1,1,1,1,1,1,1,2,1,1,1,1,1,1,1,1,1,1,
  1,1,1,1,1,1,2,2,2,2,2,2,2,2,2,2,2,1,2,2,2,2,2,1,1,1,1,1,1,1,1,1,1,1,1,1,1,
  1,1,1,1,1,1,1,1,2,2,2,7,2,1,1,2,1,2,1,1,1,1,1,1,1,1,1,2,2,1,1,1,1,1,1,1,1,
  2,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,2,2,2,2,2,2,2,2,2,2,2,1,2,2,2,2,2,1,1,1,
  1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,2,2,2,7,2,1,1,2,1,2,1,1,1,1,1,1,1,1,
  1,2,2,1,1,1,1,1,1,1,1,2,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,2,2,2,2,2,2,2,2,2,
  2,2,1,2,2,2,2,2,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,2,2,2,7,2,1,1,
  2,1,2,1,1,1,1,1,1,1,1,1,2,2,1,1,1,1,1,1,1,1,2,1,1,1,1,1,1,1,1,1,1,1,1,1,1,
  1,1,2,2,2,2,2,2,2,2,2,2,2,1,2,2,2,2,2,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,
  1,1,1,1,2,2,2,7,2,1,1,2,1,2,1,1,1,1,1,1,1,1,1,2,2,1,1,1,1,1,1,1,1,2,1,1,1,
  1,1,1,1,1,1,1,1,1,1,1,1,1,2,2,2,2,2,2,2,2,2,2,2,1,2,2,2,2,2,1,1,1,1,1,1,1,
  1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,2,2,2,7,2,1,1,2,1,2,1,1,1,1,1,1,1,1,1,2,2,1,
  1,1,1,1,1,1,1,2,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,2,2,2,2,2,2,2,2,2,2,2,1,2,
  2,2,2,2,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,2,2,2,7,2,1,1,2,1,2,1,
  1,1,1,1,1,1,1,1,2,2,1,1,1,1,1,1,1,1,2,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,2,2,
  2,2,2,2,2,2,2,2,2,1,2,2,2,2,2,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,
  2,2,2,7,2,1,1,2,1,2,1,1,1,1,1,1,1,1,1,2,2,1,1,1,1,1,1,1,1,2,1,1,1,1,1,1,1,
  1,1,1,1,1,1,1,1,1,2,2,2,2,2,2,2,2,2,2,2,1,2,2,2,2,2,1,1,1,1,1,1,1,1,1,1,1,
  1,1,1,1,1,1,1,1,1,1,1,2,2,2,7,2,1,1,2,1,2,1,1,1,1,1,1,1,1,1,2,2,1,1,1,1,1,
  1,1,1,2,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,2,2,2,2,2,2,2,2,2,2,2,1,2,2,2,2,2,
  1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,2,2,2,7,2,1,1,2,1,2,1,1,1,1,1,
  1,1,1,1,2,2,1,1,1,1,1,1,1,1,2,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,2,2,2,2,2,2,
  2,2,2,2,2,1,2,2,2,2,2,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,2,2,2,7,
  2,1,1,2,1,2,1,1,1,1,1,1,1,1,1,2,2,1,1,1,1,1,1,1,1,2,1,1,1,1,1,1,1,1,1,1,1,
  1,1,1,1,1,2,2,2,2,2,2,2,2,2,2,2,1,2,2,2,2,2,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,
  1,1,1,1,1,1,1,2,2,2,7,2,1,1,2,1,2,1,1,1,1,1,1,1,1,1,2,2,1,1,1,1,1,1,1,1,2,
  1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,2,2,2,2,2,2,2,2,2,2,2,1,2,2,2,2,2,1,1,1,1,
  1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,2,2,2,7,2,1,1,2,1,2,1,1,1,1,1,1,1,1,1,
  2,2,1,1,1,1,1,1,1,1,2,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,2,2,2,2,2,2,2,2,2,2,
  2,1,2,2,2,2,2,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,2,2,2,7,2,1,1,2,
  1,2,1,1,1,1,1,1,1,1,1,2,2,1,1,1,1,1,1,1,1,2,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,
  1,2,2,2,2,2,2,2,2,2,2,2,1,2,2,2,2,2,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,
  1,1,1,2,2,2,7,2,1,1,2,1,2,1,1,1,1,1,1,1,1,1,2,2,1,1,1,1,1,1,1,1,2,1,1,1,1,
  1,1,1,1,1,1,1,1,1,1,1,1,2,2,2,2,2,2,2,2,2,2,2,1,2,2,2,2,2,1,1,1,1,1,1,1,1,
  1,1,1,1,1,1,1,1,1,1,1,1,1,1,2,2,2,7,2,1,1,2,1,2,1,1,1,1,1,1,1,1,1,2,2,1,1,
  1,1,1,1,1,1,2,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,2,2,2,2,2,2,2,2,2,2,2,1,2,2,
  2,2,2,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,2,2,2,7,2,1,1,2,1,2,1,1,
  1,1,1,1,1,1,1,2,2,1,1,1,1,1,1,1,1,2,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,2,2,2,
  2,2,2,2,2,2,2,2,1,2,2,2,2,2,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,2,
  2,2,7,2,1,1,2,1,2,1,1,1,1,1,1,1,1,1,2,2,1,1,1,1,1,1,1,1,2,1,1,1,1,1,1,1,1,
  1,1,1,1,1,1,1,1,2,2,2,2,2,2,2,2,2,2,2,1,2,2,2,2,2,1,1,1,1,1,1,1,1,1,1,1,1,
  1,1,1,1,1,1,1,1,1,1,2,2,2,7,2,1,1,2,1,2,1,1,1,1,1,1,1,1,1,2,2,1,1,1,1,1,1,
  1,1,2,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,2,2,2,2,2,2,2,2,2,2,2,1,2,2,2,2,2,1,
  1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,2,2,2,7,2,1,1,2,1,2,1,1,1,1,1,1,
  1,1,1,2,2,1,1,1,1,1,1,1,1,2,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,2,2,2,2,2,2,2,
  2,2,2,2,1,2,2,2,2,2,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,2,2,2,7,2,
  1,1,2,1,2,1,1,1,1,1,1,1,1,1,2,2,1,1,1,1,1,1,1,1,2,1,1,1,1,1,1,1,1,1,1,1,1,
  1,1,1,1,2,2,2,2,2,2,2,2,2,2,2,1,2,2,2,2,2,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,
  1,1,1,1,1,1,2,2,2,7,2,1,1,2,1,2,1,1,1,1,1,1,1,1,1,2,2,1,1,1,1,1,1,1,1,2,1,
  1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,2,2,2,2,2,2,2,2,2,2,2,1,2,2,2,2,2,1,1,1,1,1,
  1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,2,2,2,7,2,1,1,2,1,2,1,1,1,1,1,1,1,1,1,2,
  2,1,1,1,1,1,1,1,1,2,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,2,2,2,2,2,2,2,2,2,2,2,
  1,2,2,2,2,2,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,2,2,2,7,2,1,1,2,1,
  2,1,1,1,1,1,1,1,1,1,2,2,1,1,1,1,1,1,1,1,2,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,
  2,2,2,2,2,2,2,2,2,2,2,1,2,2,2,2,2,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,
  1,1,2,2,2,7,2,1,1,2,1,2,1,1,1,1,1,1,1,1,1,2,2,1,1,1,1,1,1,1,1,2,1,1,1,1,1,
  1,1,1,1,1,1,1,1,1,1,1,2,2,2,2,2,2,2,2,2,2,2,1,2,2,2,2,2,1,1,1,1,1,1,1,1,1,
  1,1,1,1,1,1,1,1,1,1,1,1,1,2,2,2,7,2,1,1,2,1,2,1,1,1,1,1,1,1,1,1,2,2,1,1,1,
  1,1,1,1,1,2,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,2,2,2,2,2,2,2,2,2,2,2,1,2,2,2,
  2,2,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,2,2,2,7,2,1,1,2,1,2,1,1,1,
  1,1,1,1,1,1,2,2,1,1,1,1,1,1,1,1,2,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,2,2,2,2,
  2,2,2,2,2,2,2,1,2,2,2,2,2,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,2,2,
  2,7,2,1,1,2,1,2,1,1,1,1,1,1,1,1,1,2,2,1,1,1,1,1,1,1,1,2,1,1,1,1,1,1,1,1,1,
  1,1,1,1,1,1,1,2,2,2,2,2,2,2,2,2,2,2,1,2,2,2,2,2,1,1,1,1,1,1,1,1,1,1,1,1,1,
  1,1,1,1,1,1,1,1,1,2,2,2,7,2,1,1,2,1,2,1,1,1,1,1,1,1,1,1,2,2,1,1,1,1,1,1,1,
  1,2,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,2,2,2,2,2,2,2,2,2,2,2,1,2,2,2,2,2,1,1,
  1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,2,2,2,7,2,1,1,2,1,2,1,1,1,1,1,1,1,
  1,1,2,2,1,1,1,1,1,1,1,1,2,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,2,2,2,2,2,2,2,2,
  2,2,2,1,2,2,2,2,2,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,2,2,2,7,2,1,
  1,2,1,2,1,1,1,1,1,1,1,1,1,2,2,1,1,1,1,1,1,1,1,2,1,1,1,1,1,1,1,1,1,1,1,1,1,
  1,1,1,2,2,2,2,2,2,2,2,2,2,2,1,2,2,2,2,2,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,
  1,1,1,1,1,2,2,2,7,2,1,1,2,1,2,1,1,1,1,1,1,1,1,1,2,2,1,1,1,1,1,1,1,1,2,1,1,
  1,1,1,1,1,1,1,1,1,1,1,1,1,1,2,1,1,2,2,2,2,2,7,2,2,2,2,2,2,2,1,1,2,2,2,2,2,
  7,2,2,2,2,2,2,2,1,1,2,2,2,2,2,7,2,2,2,2,2,2,2,1,1,2,2,2,2,2,7,2,2,2,2,2,2,
  2,2,2,2,7,2,2,2,2,2,2,2,7,2,2,2,2,2,2,2,2,2,2,7,2,2,2,2,1,2,2,2,2,2,2,2,7,
  2,2,2,2,1,2,2,2,2,2,2,2,7,2,2,2,2,1,2,1,1,1,2,2,2,2,2,7,2,2,2,2,2,2,2,2,1,
  1,1,2,2,2,2,2,7,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,7,2,2,1,2,2,2,2,2,2,2,2,2,
  7,2,2,1,2,2,2,2,2,2,2,2,2,7,2,2,1,2,2,2,2,2,2,2,2,2,7,2,2,1,2,2,2,2,2,2,2,
  2,2,7,1,2,1,2,2,2,2,2,2,2,2,2,7,2,2,1,2,2,2,2,2,2,2,2,2,7,2,2,1,2,2,2,2,2,
  2,2,2,2,7,2,2,1,2,2,2,2,2,2,2,2,2,7,2,2,1,2,2,2,2,2,2,2,2,2,7,2,2,1,2,2,2,
  2,2,2,2,2,2,7,2,2,1,2,2,2,2,2,2,2,2,2,2,2,1,2,2,2,2,2,1,1,1,2,1,1,1,1,1,1,
  1,1,1,1,1,1,1,1,1,1,1,1,1,2,2,2,7,2,1,1,2,1,2,1,2,1,1,2,1,1,1,1,1,1,1,1,2,
  2,1,1,1,1,1,1,1,1,1,2,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,2,2,2,2,1,1,1,
  1,1,1,2,9,7,2,1,1,2,1,1,1,1,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,
  5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,
  5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,
  5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,
  7,1,3,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,
  8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,
  8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,
  8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,1,7,1,1,5,5,5,5,5,5,5,5,
  5,5,5,5,5,5,5,1,7,1,3,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,7,2,1,1,5,5,5,5,5,5,5,
  5,5,5,5,5,5,5,5,5,5,5,5,1,7,1,3,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,7,2,
  1,5,5,5,5,5,5,7,1,3,5,5,5,5,5,5,7,1,3,5,5,1,7,1,3,1,2,7,2,1,1,2,1,1,5,5,5,
  5,5,5,7,1,3,5,5,5,5,5,5,7,1,3,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,7,
  1,3,2,2,2,2,2,2,2,2,2,2,2,2,2,4,4,4,4,2,2,4,4,7,2,1,1,2,7,1,1,1,2,7,2,1,1,
  5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,
  5,5,5,5,5,5,5,7,1,3,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,
  8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,1,7,1,1,2,2,2,2,2,2,2,2,2,2,2,1,2,2,2,2,2,
  1,1,1,2,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,2,2,2,1,7,2,1,1,2,1,2,1,2,1,
  1,1,1,1,1,1,1,1,1,1,2,2,1,1,1,1,1,1,1,1,2,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,
  1,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,
  5,5,5,5,5,5,1,7,1,3,2,2,2,2,2,2,2,2,2,2,2,1,2,2,2,2,2,1,1,1,1,1,1,1,1,1,1,
  1,1,1,1,1,1,1,1,1,1,1,1,2,2,2,7,2,1,1,2,1,2,1,1,1,1,1,1,1,1,1,2,2,1,1,1,1,
  1,1,1,1,2,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,1,
  7,1,3,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,7,2,1,1,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,
  1,7,1,3,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,7,2,1,1,5,5,5,5,5,5,5,5,5,5,5,5,5,5,
  5,1,7,1,3,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,7,2,1,1,1,5,5,5,5,5,5,5,5,5,5,5,5,
  5,5,5,1,7,1,3,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,7,2,1,1,1,5,5,5,5,5,5,5,5,5,5,
  5,5,5,5,5,1,7,1,3,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,7,2,1,1,1,5,5,5,5,5,5,5,5,
  5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,1,7,
  1,3,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,
  5,5,5,5,5,5,5,5,1,7,1,3,2,2,2,2,2,2,2,2,2,2,2,1,2,2,2,2,2,1,1,1,2,1,1,1,1,
  1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,2,2,2,7,2,1,1,2,1,2,1,2,1,1,2,1,1,1,1,1,1,1,
  1,2,2,1,1,1,1,1,1,1,1,1,2,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,5,5,5,5,5,5,
  5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,
  1,7,1,3,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,
  5,5,5,5,5,5,5,5,5,5,1,7,1,3,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,
  5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,1,7,1,3,2,2,2,2,2,2,2,2,2,2,2,1,2,
  2,2,2,2,1,1,1,2,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,2,2,2,7,2,1,1,2,1,2,
  1,2,1,1,2,1,1,1,1,1,1,1,1,2,2,1,1,1,1,1,1,1,1,1,2,1,1,1,1,1,1,1,1,1,1,1,1,
  1,1,1,1,1,1,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,
  5,5,5,5,5,5,5,5,5,5,5,1,7,1,3,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,
  5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,1,7,1,3,2,2,2,2,2,2,2,2,2,2,2,1,2,
  2,2,2,2,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,2,2,2,7,2,1,1,2,1,2,1,
  1,1,1,1,1,1,1,1,2,2,1,1,1,1,1,1,1,1,2,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,5,5,
  5,5,5,5,7,1,3,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,
  5,5,5,5,5,5,5,5,5,5,5,5,1,7,1,3,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,
  5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,1,7,1,3,5,5,5,5,5,5,5,5,5,5,5,5,
  5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,1,7,1,3,2,2,2,
  2,2,2,2,2,2,2,2,1,2,2,2,2,2,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,2,
  2,2,7,2,1,1,2,1,2,1,1,1,1,1,1,1,1,1,2,2,1,1,1,1,1,1,1,1,2,1,1,1,1,1,1,1,1,
  1,1,1,1,1,1,1,1,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,7,2,1,1,5,5,5,5,5,5,5,5,7,1,
  3,5,5,5,5,5,5,5,5,7,1,3,1,1,1,1,3,7,3,3,1,3,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,
  1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,7,2,2,2,1,2,2,2,2,
  2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,1,1,1,1,
  2,7,2,2,1,2,1,1,1,1,7,1,8,8,8,8,8,1,7,1,1,8,8,8,8,8,1,7,1,1,8,8,8,8,8,1,7,
  1,1,2,1,2,2,2,2,2,1,1,1,1,2,3,7,1,1,1,1,1,2,7,2,2,1,2,1,1,1,1,1,1,1,2,7,2,
  2,1,2,1,1,1,2,2,2,2,2,2,2,2,2,7,2,2,2,2,2,2,7,2,1,2,2,2,2,2,2,2,2,2,2,2,2,
  2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,7,1,4,4,4,4,4,4,
  4,4,4,4,4,4,4,4,4,4,4,4,4,4,4,4,4,4,4,4,4,4,4,4,4,4,4,4,4,4,4,4,4,4,4,4,4,
  4,4,4,4,4,4,7,4,4,4,4,4,4,4,4,4,4,4,4,4,4,4,4,4,4,4,4,4,4,4,4,4,4,4,4,4,4,
  4,4,4,4,4,4,4,4,4,4,4,4,4,4,4,4,4,4,4,7,1,7,1,1,7,1,1,7,1,1,7,1,1,7,1,1,7,
  1,1,7,1,1,7,1,1,7,1,1,7,1,1,7,1,2,2,2,2,2,2,2,2,2,2,2,1,2,2,2,2,2,1,1,1,1,
  1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,2,2,2,7,2,1,1,2,1,2,1,1,1,1,1,1,1,1,1,
  1,2,1,1,1,1,1,1,1,1,2,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,2,2,2,2,2,7,5,5,5,5,
  5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,7,
  1,3,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,
  8,8,8,8,1,7,1,1,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,
  5,5,5,5,5,5,5,5,5,5,5,7,1,3,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,
  8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,1,7,1,1,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,
  8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,1,7,1,1,8,8,8,8,8,8,8,8,8,8,8,
  8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,1,7,1,1,8,8,8,8,8,
  8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,1,7,1,
  1,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,
  8,8,8,8,8,8,1,7,1,1,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,
  8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,1,7,1,1,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,
  5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,7,1,3,8,8,8,8,8,8,8,8,8,
  8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,1,7,1,1,
  5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,
  5,5,5,5,5,5,7,1,3,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,
  8,8,8,8,8,8,8,8,8,8,8,8,8,8,1,7,1,1,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,
  5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,7,1,3,8,8,8,8,8,8,8,8,8,8,
  8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,1,7,1,1,5,
  5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,
  5,5,5,5,5,7,1,3,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,
  8,8,8,8,8,8,8,8,8,8,8,8,8,1,7,1,1,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,
  5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,7,1,3,8,8,8,8,8,8,8,8,8,8,8,
  8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,1,7,1,1,8,8,
  8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,
  8,8,8,1,7,1,1,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,
  5,5,5,5,5,5,5,5,5,5,5,5,5,7,1,3,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,
  8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,1,7,1,1,8,8,8,8,8,8,8,8,8,8,8,8,
  8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,1,7,1,1,5,5,5,
  5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,
  5,5,5,7,1,3,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,
  8,8,8,8,8,8,8,8,8,8,8,1,7,1,1,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,
  8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,1,7,1,1,5,5,5,5,5,5,5,5,5,5,5,5,5,
  5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,1,7,1,3,5,5,5,5,
  5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,
  5,1,7,1,3,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,
  5,5,5,5,5,5,5,5,5,5,1,7,1,3,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,
  5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,1,7,1,3,5,5,5,5,5,5,5,5,5,5,5,5,5,5,
  5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,1,7,1,3,5,5,5,5,5,
  5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,
  1,7,1,3,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,
  5,5,5,5,5,5,5,5,5,1,7,1,3,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,
  5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,1,7,1,3,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,
  5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,1,7,1,3,5,5,5,5,5,5,
  5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,1,
  7,1,3,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,
  5,5,5,5,5,5,5,5,1,7,1,3,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,
  5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,1,7,1,3,1,1,1,1,2,7,2,2,1,2,1,1,1,1,1,1,
  1,2,7,2,2,1,2,1,1,1,1,1,1,1,2,7,2,2,1,2,1,1,1,1,1,1,1,2,7,2,2,1,2,1,1,1,1,
  1,1,1,2,7,2,2,1,2,1,1,1,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,7,1,1,1,1,1,1,1,2,7,
  2,2,1,2,1,1,1,1,1,1,1,2,7,2,2,1,2,1,1,1,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,
  2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,3,2,7,1,1,1,1,2,7,2,2,1,2,
  1,1,1,1,1,1,1,2,7,2,2,1,2,1,1,1,10,10,10,10,10,10,10,10,10,10,10,10,10,10,
  10,10,10,10,10,5,5,5,5,7,1,3,8,8,8,8,8,1,7,1,1,8,8,8,8,8,1,7,1,1,8,8,8,8,8,
  1,7,1,1,8,8,8,8,8,1,7,1,1,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,
  5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,1,7,1,3,2,2,2,2,2,2,2,2,2,2,2,1,2,2,2,
  2,2,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,2,2,2,7,2,1,1,2,1,2,1,1,1,
  1,1,1,1,1,1,1,2,1,1,1,1,1,1,1,1,2,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,5,5,5,5,
  5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,
  5,1,7,1,3,2,2,2,2,2,2,2,2,2,2,2,1,2,2,2,2,2,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,
  1,1,1,1,1,1,1,2,2,2,7,2,1,1,2,1,2,1,1,1,1,1,1,1,1,1,1,2,1,1,1,1,1,1,1,1,2,
  1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,3,7,3,3,1,3,1,1,1,5,5,5,5,5,5,7,1,
  3,8,8,8,8,8,1,7,1,1,5,5,5,5,5,5,5,5,7,1,3,5,5,5,5,5,5,5,5,7,1,3,5,5,5,5,5,
  5,5,5,7,1,3,5,5,5,5,5,5,5,5,5,7,1,3,5,5,5,5,5,5,5,5,7,1,3,8,8,8,8,8,8,8,8,
  8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,1,7,1,
  1,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,
  8,8,8,8,8,8,1,7,1,1,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,
  8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,1,7,1,1,5,5,5,5,5,5,5,7,1,3,5,5,5,5,5,5,5,7,
  1,3,8,1,7,1,1,2,1,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,
  2,2,2,2,2,2,2,2,2,2,2,2,7,1,2,1,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,3,2,
  2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,7,2,5,5,5,5,5,5,5,5,7,1,3,1,1,7,2,
  1,2,2,2,2,2,2,2,2,2,7,9,2,2,1,1,2,10,10,10,10,10,9,4,4,4,4,2,2,4,4,7,2,1,1,
  1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,
  1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,
  1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,
  1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,7,1,1,1,1,1,1,1,1,1,1,1,1,1,2,2,2,
  2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,
  2,2,2,2,2,2,2,2,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,
  1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,
  1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,2,1,2,2,2,2,2,
  2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,7,1,
  1,7,1,2,2,2,2,2,2,2,2,2,2,2,1,2,2,2,2,2,1,1,1,2,1,1,1,1,1,1,1,1,1,1,1,1,1,
  1,1,1,1,1,1,2,2,2,7,2,1,1,2,1,2,1,2,1,1,2,1,1,1,1,1,1,1,1,2,2,1,1,1,1,1,1,
  1,1,1,2,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,2,2,2,2,2,2,2,2,2,2,2,1,2,2,2,
  2,2,1,1,1,2,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,2,2,2,7,2,1,1,2,1,2,1,2,
  1,1,1,1,1,1,1,1,1,2,2,1,1,1,1,1,1,1,1,2,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,
  1,4,4,4,4,4,4,7,1,1,4,4,4,4,4,4,7,1,1,4,4,4,4,4,4,7,1,1,4,4,4,4,4,4,7,1,1,
  4,4,4,4,4,4,7,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,
  1,1,1,1,1,1,1,1,1,1,1,1,8,8,1,7,2,1,1,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,
  2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,
  8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,1,7,1,1,1,1,1,1,2,7,
  2,2,1,2,1,1,1,1,1,1,1,2,7,2,2,1,2,1,1,1,1,1,1,1,2,7,2,2,1,2,1,1,1,1,1,1,1,
  1,1,1,1,1,7,1,1,1,1,1,7,4,4,4,4,4,4,4,4,4,4,4,4,4,4,4,4,1,1,1,1,1,4,4,4,4,
  4,4,4,4,4,4,4,4,4,4,4,4,4,4,4,4,4,7,2,7,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,1,7,
  1,1,2,2,2,2,2,2,2,2,2,2,2,1,2,2,2,2,2,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,
  1,1,1,1,2,2,2,7,2,1,1,2,1,2,1,1,1,1,1,1,1,1,1,1,2,1,1,1,1,1,1,1,1,2,1,1,1,
  1,1,1,1,1,1,1,1,1,1,1,1,1,2,2,2,2,2,2,2,2,2,2,2,1,2,2,2,2,2,1,1,1,1,1,1,1,
  1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,2,2,2,7,2,1,1,2,1,2,1,1,1,1,1,1,1,1,1,1,2,1,
  1,1,1,1,1,1,1,2,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,2,2,2,2,2,2,2,2,2,2,2,1,2,
  2,2,2,2,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,2,2,2,7,2,1,1,2,1,2,1,
  1,1,1,1,1,1,1,1,1,2,1,1,1,1,1,1,1,1,2,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,2,2,
  2,2,2,2,2,2,2,2,2,1,2,2,2,2,2,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,
  2,2,2,7,2,1,1,2,1,2,1,1,1,1,1,1,1,1,1,1,2,1,1,1,1,1,1,1,1,2,1,1,1,1,1,1,1,
  1,1,1,1,1,1,1,1,1,2,2,2,2,2,2,2,2,2,2,2,1,2,2,2,2,2,1,1,1,1,1,1,1,1,1,1,1,
  1,1,1,1,1,1,1,1,1,1,1,2,2,2,7,2,1,1,2,1,2,1,1,1,1,1,1,1,1,1,1,2,1,1,1,1,1,
  1,1,1,2,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,2,2,2,2,2,2,2,2,2,2,2,1,2,2,2,2,2,
  1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,2,2,2,7,2,1,1,2,1,2,1,1,1,1,1,
  1,1,1,1,1,2,1,1,1,1,1,1,1,1,2,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,2,2,2,2,2,2,
  2,2,2,2,2,1,2,2,2,2,2,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,2,2,2,7,
  2,1,1,2,1,2,1,1,1,1,1,1,1,1,1,1,2,1,1,1,1,1,1,1,1,2,1,1,1,1,1,1,1,1,1,1,1,
  1,1,1,1,1,2,2,2,2,2,2,2,2,2,2,2,1,2,2,2,2,2,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,
  1,1,1,1,1,1,1,2,2,2,7,2,1,1,2,1,2,1,1,1,1,1,1,1,1,1,1,2,1,1,1,1,1,1,1,1,2,
  1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,2,2,2,2,2,2,2,2,2,2,2,1,2,2,2,2,2,1,1,1,1,
  1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,2,2,2,7,2,1,1,2,1,2,1,1,1,1,1,1,1,1,1,
  1,2,1,1,1,1,1,1,1,1,2,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,2,2,2,2,2,2,2,2,2,2,
  2,1,2,2,2,2,2,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,2,2,2,7,2,1,1,2,
  1,2,1,1,1,1,1,1,1,1,1,1,2,1,1,1,1,1,1,1,1,2,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,
  1,8,1,7,1,1,2,2,2,2,2,2,2,2,2,2,2,1,2,2,2,2,2,1,1,1,1,1,1,1,1,1,1,1,1,1,1,
  1,1,1,1,1,2,2,2,7,2,1,1,2,1,2,1,1,2,1,1,1,2,2,2,1,1,1,1,1,1,1,1,1,1,1,1,1,
  1,1,2,2,2,2,2,2,2,2,2,2,2,1,2,2,2,2,2,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,
  1,2,2,2,7,2,1,1,2,1,2,1,1,2,1,1,1,2,2,2,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,2,2,
  2,2,2,2,2,2,2,2,2,1,2,2,2,2,2,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,2,2,2,
  7,2,1,1,2,1,2,1,1,2,1,1,1,2,2,2,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,2,2,2,2,2,2,
  2,2,2,2,2,1,2,2,2,2,2,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,2,2,2,7,2,1,1,
  2,1,2,1,1,2,1,1,1,2,2,2,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,2,2,2,2,2,2,2,2,2,2,
  2,1,2,2,2,2,2,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,2,2,2,7,2,1,1,2,1,2,1,
  1,2,1,1,1,2,2,2,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,2,2,2,2,2,2,2,2,2,2,2,1,2,2,
  2,2,2,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,2,2,2,7,2,1,1,2,1,2,1,1,
  1,1,2,1,1,1,1,1,1,1,1,2,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,2,2,2,2,2,2,2,2,2,
  2,2,1,2,2,2,2,2,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,2,2,2,7,2,1,1,
  2,1,2,1,1,1,1,2,1,1,1,1,1,1,1,1,2,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,2,2,2,2,
  2,2,2,2,2,2,2,1,2,2,2,2,2,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,2,2,
  2,7,2,1,1,2,1,2,1,1,1,1,1,2,1,1,1,1,1,1,1,1,2,1,1,1,1,1,1,1,1,1,1,1,1,1,1,
  1,1,2,2,2,2,2,2,2,2,2,2,2,1,2,2,2,2,2,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,
  1,1,1,1,2,2,2,7,2,1,1,2,1,2,1,1,1,1,1,2,1,1,1,1,1,1,1,1,2,1,1,1,1,1,1,1,1,
  1,1,1,1,1,1,1,1,2,2,2,2,2,2,2,2,2,2,2,1,2,2,2,2,2,1,1,1,1,1,1,1,1,1,1,1,1,
  1,1,1,1,1,1,1,1,1,1,2,2,2,7,2,1,1,2,1,2,1,1,1,1,1,2,1,1,1,1,1,1,1,1,2,1,1,
  1,1,1,1,1,1,1,1,1,1,1,1,1,1,2,2,2,2,2,2,2,2,2,2,2,1,2,2,2,2,2,1,1,1,1,1,1,
  1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,2,2,2,7,2,1,1,2,1,2,1,1,1,1,1,2,1,1,1,1,1,
  1,1,1,2,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,2,2,2,2,2,2,2,2,2,2,2,1,2,2,2,2,2,
  1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,2,2,2,7,2,1,1,2,1,2,1,1,1,1,1,
  1,2,1,1,1,1,1,1,1,1,2,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,2,2,2,2,2,2,2,2,2,2,
  2,1,2,2,2,2,2,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,2,2,2,7,2,1,1,2,
  1,2,1,1,1,1,1,1,2,1,1,1,1,1,1,1,1,2,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,2,2,2,
  2,2,2,2,2,2,2,2,1,2,2,2,2,2,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,2,
  2,2,7,2,1,1,2,1,2,1,1,1,1,1,1,1,2,1,1,1,1,1,1,1,1,2,1,1,1,1,1,1,1,1,1,1,1,
  1,1,1,1,1,2,2,2,2,2,2,2,2,2,2,2,1,2,2,2,2,2,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,
  1,1,1,1,1,1,1,2,2,2,7,2,1,1,2,1,2,1,1,1,1,1,1,1,2,1,1,1,1,1,1,1,1,2,1,1,1,
  1,1,1,1,1,1,1,1,1,1,1,1,1,2,2,2,2,2,2,2,2,2,2,2,1,2,2,2,2,2,1,1,1,1,1,1,1,
  1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,2,2,2,7,2,1,1,2,1,2,1,1,1,1,1,1,1,1,2,1,1,1,
  1,1,1,1,1,2,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,2,2,2,2,2,2,2,2,2,2,2,1,2,2,2,
  2,2,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,2,2,2,7,2,1,1,2,1,2,1,1,1,
  1,1,1,1,1,2,1,1,1,1,1,1,1,1,2,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,8,8,8,8,8,1,
  7,1,1,1,1,1,1,2,7,2,2,1,2,1,1,1,1,1,1,1,2,7,2,2,1,2,1,1,1,1,1,1,1,2,7,2,2,
  1,2,1,1,1,1,1,1,1,2,7,2,2,1,2,1,1,1,8,8,8,8,8,1,7,1,1,8,8,8,8,8,1,7,1,1,1,
  1,1,1,2,7,2,2,1,2,1,1,1,2,2,2,2,2,2,2,2,2,2,2,1,2,2,2,2,2,1,1,1,1,1,1,1,1,
  1,1,1,1,1,1,1,1,1,1,1,1,1,1,2,2,2,7,2,1,1,2,1,2,1,1,1,1,1,1,1,1,1,2,2,1,1,
  1,1,1,1,1,1,2,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,2,2,2,2,2,2,2,2,2,2,2,1,2,2,
  2,2,2,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,2,2,2,7,2,1,1,2,1,2,1,1,
  1,1,1,1,1,1,1,2,2,1,1,1,1,1,1,1,1,2,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,2,2,2,
  2,2,2,2,2,2,2,2,1,2,2,2,2,2,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,2,
  2,2,7,2,1,1,2,1,2,1,1,1,1,1,1,1,1,1,2,2,1,1,1,1,1,1,1,1,2,1,1,1,1,1,1,1,1,
  1,1,1,1,1,1,1,1,1,7,1,2,1,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,
  2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,7,1,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,
  5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,7,1,
  3,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,
  8,8,8,8,8,8,8,1,7,1,1,1,7,1,5,5,5,5,5,5,1,7,1,3,2,1,2,2,2,2,2,2,2,2,2,2,2,
  2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,7,2,8,8,8,8,8,8,
  8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,1,
  7,1,1,1,4,4,4,4,4,4,7,1,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,1,7,1,1,5,5,5,5,5,5,
  5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,
  5,5,5,7,1,3,4,4,4,4,4,4,4,4,4,4,4,4,4,4,4,4,4,4,4,4,4,4,4,4,4,4,4,4,4,4,4,
  4,4,4,4,4,4,4,4,4,5,5,5,5,5,5,7,1,1,1,1,2,7,2,2,1,2,1,1,1,2,2,2,2,2,2,2,2,
  2,2,2,1,2,2,2,2,2,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,2,2,2,7,2,1,
  1,2,1,2,1,1,1,1,1,1,1,1,1,1,2,1,1,1,1,1,1,1,1,2,1,1,1,1,1,1,1,1,1,1,1,1,1,
  1,1,1,4,4,4,4,4,4,4,4,2,2,2,2,4,4,4,4,2,2,2,2,2,4,4,4,4,4,4,4,4,4,4,4,4,4,
  4,4,4,4,4,4,4,4,7,2,2,2,2,2,7,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,7,1,1,1,1,1,7,
  2,1,7,2,1,7,2,1,7,2,1,7,2,1,7,2,1,7,2,1,7,2,1,7,2,1,7,2,1,7,3,4,1,1,1,1,1,
  4,4,4,4,4,4,4,4,4,4,4,4,4,4,4,4,4,4,4,4,4,4,4,4,4,4,4,4,4,4,4,7,1,1,1,1,1,
  4,1,1,1,1,1,4,4,4,4,4,4,4,4,4,4,4,4,4,4,4,4,4,4,4,4,4,4,4,4,4,4,4,4,4,4,4,
  7,1,1,1,1,1,4,1,1,4,4,4,4,4,4,4,4,4,4,4,4,4,4,4,4,4,4,4,4,4,4,4,4,4,4,4,4,
  4,4,4,7,1,1,4,1,1,4,4,4,4,4,4,4,4,4,4,4,4,4,4,4,4,4,4,4,4,4,4,4,4,4,4,4,4,
  4,4,4,7,1,1,4,1,1,4,4,4,4,4,4,4,4,4,4,4,4,4,4,4,4,4,4,4,4,4,4,4,4,4,4,4,4,
  4,4,4,7,1,1,4,1,1,4,4,4,4,4,4,4,4,4,4,4,4,4,4,4,4,4,4,4,4,4,4,4,4,4,4,4,4,
  4,4,4,7,1,1,4,1,1,1,1,4,4,4,4,4,4,4,4,4,4,4,4,4,4,4,4,4,4,4,4,4,4,4,4,4,4,
  4,7,1,1,1,1,4,1,1,1,1,4,4,4,4,4,4,4,4,4,4,4,4,4,4,4,4,4,4,4,4,4,4,4,4,4,4,
  4,7,1,1,1,1,4,1,1,4,4,4,4,4,4,4,4,4,4,4,4,4,4,4,4,4,4,4,4,4,4,4,4,4,7,1,1,
  4,1,1,4,4,4,4,4,4,4,4,4,4,4,4,4,4,4,4,4,4,4,4,4,4,4,4,4,7,1,1,4,1,1,4,4,4,
  4,4,4,4,4,4,4,4,4,4,4,4,4,4,4,4,4,4,4,4,7,1,1,4,1,1,4,4,4,4,4,4,4,4,4,4,4,
  4,4,4,4,4,4,4,4,4,4,4,4,7,1,1,1,1,1,1,2,7,2,2,1,2,1,1,1,1,1,1,1,2,7,2,2,1,
  2,1,1,1,1,1,1,1,2,7,2,2,1,2,1,1,1,8,8,8,8,8,8,8,8,8,1,7,1,1,4,4,4,4,4,4,4,
  4,4,4,4,4,4,4,4,4,4,4,4,4,4,2,4,4,4,4,4,4,4,4,4,4,4,4,4,4,4,4,4,4,4,4,4,7,
  2,2,2,2,2,2,2,2,2,2,2,1,2,2,2,2,2,1,1,1,2,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,
  1,1,1,2,2,2,7,2,1,1,2,1,2,1,2,1,1,2,1,1,1,1,1,1,1,1,2,2,1,1,1,1,1,1,1,1,2,
  1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,2,2,2,2,2,2,2,2,2,2,2,1,2,2,2,2,2,1,1,
  1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,2,2,2,7,2,1,1,2,1,2,1,1,1,1,1,1,1,
  1,1,2,2,1,1,1,1,1,1,1,1,2,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,2,2,2,2,2,2,2,2,
  2,2,2,2,2,2,2,7,2,1,1,8,8,8,8,8,1,7,1,1,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,
  2,5,2,1,7,1,3,2,5,5,7,1,3,8,1,7,1,1,2,2,2,2,2,2,2,2,2,7,2,2,1,1,1,1,1,2,7,
  2,2,1,2,1,1,1,1,7,2
};


static const unsigned short ag_pstt[] = {
2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,0,1,2,
19,21,
137,137,137,137,137,137,137,137,137,137,137,137,137,3,8,8,8,8,8,172,171,170,
  136,135,7,8,10,2,9,0,11,11,11,10,8,11,5,5,4,6,4,
20,20,20,20,20,20,20,20,20,20,20,20,20,20,1,3,1,893,
138,138,138,138,138,138,138,138,138,138,138,138,139,139,139,139,139,138,169,
  169,138,169,169,169,169,168,169,169,169,4,139,
12,13,14,15,16,17,18,19,20,21,22,23,24,25,5,39,42,38,37,36,35,34,33,32,31,
  30,29,28,27,26,
20,20,20,20,20,20,20,20,20,20,20,20,20,20,20,20,20,20,20,20,20,20,20,20,20,
  20,20,20,20,20,20,20,20,20,20,20,20,20,20,20,20,20,20,20,20,20,20,20,20,
  20,20,20,20,20,20,20,20,20,20,20,20,20,20,20,20,20,20,20,20,20,20,20,20,
  20,20,20,20,20,20,20,20,20,20,20,20,20,20,20,20,20,20,20,20,20,20,20,20,
  20,20,20,20,20,20,20,20,20,20,20,20,20,20,20,20,20,20,20,20,20,20,20,20,
  20,20,20,20,20,20,20,20,20,20,20,20,20,20,20,20,20,20,20,20,20,20,20,20,
  20,20,20,20,20,20,20,20,20,20,20,20,20,20,20,20,20,20,20,20,20,20,20,20,
  20,20,20,20,20,20,20,20,20,20,20,20,20,20,20,20,20,20,20,20,20,20,20,20,
  20,20,20,20,1,6,1,883,
40,7,39,
42,42,41,43,44,45,40,25,8,25,25,48,25,47,46,46,42,42,
49,49,49,49,49,49,49,49,49,49,49,49,49,49,49,49,49,49,49,49,49,49,49,49,49,
  49,49,49,49,49,49,49,49,49,49,49,49,49,49,49,49,49,49,49,49,49,49,49,49,
  49,49,49,49,49,49,49,49,49,49,49,49,49,49,49,49,49,49,49,49,49,49,49,49,
  49,49,49,49,49,49,49,49,49,49,49,49,49,49,49,49,49,49,49,49,49,49,49,49,
  49,49,49,49,49,49,49,49,49,49,49,49,49,49,49,49,49,49,49,49,49,49,49,49,
  49,49,49,49,49,49,49,49,49,49,49,49,49,49,49,49,49,49,49,49,49,49,49,49,
  49,49,49,49,49,49,49,49,49,49,49,49,49,49,49,49,49,49,49,49,49,49,49,49,
  49,49,49,49,49,49,49,49,49,49,49,49,49,49,49,49,49,49,20,20,49,49,49,49,
  49,49,49,49,1,9,1,49,
137,137,137,137,137,137,137,137,137,137,137,137,137,117,118,119,120,121,122,
  123,124,125,126,127,128,129,130,131,132,133,134,135,136,137,138,139,140,
  68,69,70,71,72,73,141,142,143,144,145,146,74,75,76,77,78,147,148,149,79,
  80,81,82,83,150,151,152,153,154,155,156,157,158,159,160,161,162,163,164,
  113,65,66,67,171,172,173,174,175,176,177,178,179,180,181,182,183,184,
  185,186,165,166,167,168,169,170,84,85,86,87,88,89,51,52,53,90,91,92,93,
  94,95,96,97,98,54,55,99,100,101,102,103,104,105,106,56,57,58,107,108,59,
  60,109,110,111,112,61,62,63,64,271,271,271,271,271,271,271,271,271,271,
  271,271,271,271,271,271,271,271,271,271,271,271,271,271,271,271,271,271,
  271,271,271,271,18,19,22,271,271,271,271,271,21,21,41,21,21,21,40,136,
  135,21,19,10,267,268,269,266,272,40,271,271,37,270,111,112,272,50,265,
  202,266,266,266,266,266,266,266,266,266,266,333,333,333,333,333,334,335,
  336,337,340,340,340,341,342,346,346,346,346,347,348,349,350,351,352,353,
  354,357,357,357,358,359,360,361,362,363,364,365,366,370,370,370,370,371,
  372,373,374,375,376,248,247,246,245,244,243,264,263,262,261,260,259,258,
  257,256,255,254,253,252,251,250,249,190,116,189,115,188,114,187,242,241,
  240,239,238,237,236,235,234,233,232,231,230,229,228,227,227,227,227,227,
  227,226,225,224,223,223,223,223,223,223,222,221,220,219,218,217,216,216,
  216,216,215,215,215,215,214,213,212,211,210,209,208,207,206,205,204,203,
  201,200,199,198,197,196,195,194,193,192,191,265,
137,137,137,137,137,137,137,137,137,137,137,137,137,3,8,8,8,8,8,172,171,170,
  136,135,7,4,10,11,9,3,3,10,8,3,5,5,4,6,4,
20,20,20,20,20,20,20,20,20,20,20,20,20,20,20,1,12,1,913,
20,20,20,20,20,20,20,20,20,20,20,20,20,20,20,1,13,1,912,
20,20,14,1,911,
20,20,15,1,909,
20,20,20,20,20,20,20,20,20,20,20,20,20,20,20,20,20,20,20,20,20,20,20,20,20,
  20,20,20,20,20,20,20,20,20,20,20,20,20,20,20,20,20,20,20,16,1,908,
20,1,17,1,907,
20,20,20,20,20,20,18,1,906,
20,20,20,20,20,20,19,1,905,
20,20,20,20,20,20,20,20,20,20,20,20,20,20,20,1,20,1,904,
20,20,20,20,20,20,20,20,20,20,20,20,20,20,20,20,20,20,20,20,20,20,20,20,20,
  20,20,20,20,20,20,20,20,20,20,20,20,20,20,20,20,20,1,21,1,903,
20,20,20,20,20,20,20,20,20,20,20,20,20,20,20,20,20,20,20,20,20,20,20,20,20,
  20,20,20,20,20,20,20,20,20,20,20,20,20,20,20,20,20,1,22,1,902,
20,20,20,20,20,20,20,20,20,20,20,20,20,20,20,1,23,1,901,
20,20,20,20,20,1,24,1,895,
20,20,20,20,20,20,20,20,20,20,20,20,20,20,20,20,20,20,20,20,20,1,25,1,888,
137,137,137,137,137,137,137,137,137,137,137,137,137,136,135,26,267,273,265,
  265,
137,137,137,137,137,137,137,137,137,137,137,137,137,136,135,27,274,265,265,
275,1,28,1,275,
276,1,29,1,276,
277,277,277,277,277,277,277,277,277,277,277,277,277,277,277,277,277,277,277,
  277,277,277,277,277,277,277,277,277,277,277,277,277,277,277,277,277,277,
  277,277,277,277,277,277,1,30,1,277,
146,31,280,278,279,
281,281,281,281,281,1,32,1,281,
282,282,282,282,282,1,33,1,282,
137,137,137,137,137,137,137,137,137,137,137,137,137,136,135,34,283,265,265,
137,137,137,137,137,137,137,137,137,137,137,285,137,256,256,256,256,288,290,
  287,297,299,300,301,302,303,304,305,306,307,323,324,325,286,322,289,298,
  310,284,271,136,135,35,240,295,296,241,291,242,292,267,335,334,332,337,
  333,336,331,338,340,226,326,312,265,339,327,331,330,329,328,225,331,321,
  320,319,318,317,316,315,314,313,311,294,293,265,309,308,
137,137,137,137,137,137,137,137,137,137,137,285,137,256,256,256,256,288,290,
  287,297,299,300,301,302,303,304,305,306,307,323,324,325,286,322,289,298,
  310,284,271,136,135,36,240,295,296,241,291,242,292,267,335,334,332,337,
  333,336,331,338,341,226,326,312,265,339,327,331,330,329,328,225,331,321,
  320,319,318,317,316,315,314,313,311,294,293,265,309,308,
137,137,137,137,137,137,137,137,137,137,137,137,137,136,135,37,342,265,265,
343,345,347,349,351,38,352,350,348,346,344,
164,146,39,356,278,355,353,279,354,
20,20,20,20,20,20,20,20,20,20,20,20,20,20,20,20,20,20,20,20,20,20,20,20,20,
  20,20,20,20,20,20,20,20,20,20,20,20,20,20,20,20,20,20,20,20,20,20,20,20,
  20,20,20,20,20,20,40,1,878,
20,20,20,20,20,20,20,20,20,20,20,20,20,20,20,20,20,20,20,20,20,20,20,20,20,
  20,20,20,20,20,20,20,20,20,20,20,20,20,20,20,20,20,20,20,20,20,20,20,20,
  20,20,20,20,20,20,20,20,20,20,20,20,20,20,20,20,20,20,20,20,20,20,20,20,
  20,20,20,20,20,20,20,20,20,20,20,20,20,20,20,20,41,1,884,
357,25,42,358,358,358,
20,20,20,20,20,20,20,20,20,20,20,20,20,20,20,20,20,20,20,20,20,20,20,20,20,
  20,20,20,20,20,20,20,20,20,20,20,20,20,20,20,20,20,20,20,20,43,1,882,
20,20,20,20,20,20,20,20,20,20,20,20,20,20,20,20,20,20,20,20,20,20,20,20,20,
  20,20,20,20,20,20,20,20,20,20,20,20,20,20,20,20,20,20,20,20,20,20,20,44,
  1,880,
20,20,20,20,20,20,20,20,20,20,20,20,20,20,20,20,20,20,20,20,20,20,20,20,20,
  20,20,20,20,20,20,20,20,20,20,20,20,20,20,20,20,20,20,45,1,879,
359,361,363,364,365,366,367,368,369,370,371,372,373,374,375,376,377,378,379,
  380,381,382,389,392,393,394,323,325,391,383,387,384,386,390,362,360,310,
  388,385,41,43,395,44,40,16,46,17,16,16,46,14,46,16,16,16,16,16,16,16,16,
  16,16,16,16,16,16,16,16,16,16,16,16,16,16,16,16,16,16,16,16,16,16,16,16,
  16,16,16,16,16,16,16,16,
359,361,363,364,365,366,367,368,369,370,371,372,373,374,375,376,377,378,379,
  380,381,382,389,392,393,394,323,325,391,383,387,384,386,390,362,360,310,
  388,385,41,44,397,396,47,396,397,396,396,396,396,396,396,396,396,396,
  396,396,396,396,396,396,396,396,396,396,396,396,396,396,396,396,396,396,
  396,396,396,396,396,396,396,396,396,396,396,396,396,396,
359,361,363,364,365,366,367,368,369,370,371,372,373,374,375,376,377,378,379,
  380,381,382,389,392,393,394,323,325,391,383,387,384,386,390,362,360,310,
  388,385,41,44,398,396,48,396,398,396,396,396,396,396,396,396,396,396,
  396,396,396,396,396,396,396,396,396,396,396,396,396,396,396,396,396,396,
  396,396,396,396,396,396,396,396,396,396,396,396,396,396,
137,137,137,137,137,137,137,137,137,137,137,137,137,117,118,119,120,121,122,
  123,124,125,126,127,128,129,130,131,132,133,134,135,136,137,138,139,140,
  68,69,70,71,72,73,141,142,143,144,145,146,74,75,76,77,78,147,148,149,79,
  80,81,82,83,150,151,152,153,154,155,156,157,158,159,160,161,162,163,164,
  113,65,66,67,171,172,173,174,175,176,177,178,179,180,181,182,183,184,
  185,186,165,166,167,168,169,170,84,85,86,87,88,89,51,52,53,90,91,92,93,
  94,95,96,97,98,54,55,99,100,101,102,103,104,105,106,56,57,58,107,108,59,
  60,109,110,111,112,61,62,63,64,401,401,401,401,401,401,401,401,401,401,
  401,401,401,401,401,401,401,401,401,401,401,401,401,401,401,401,401,401,
  401,401,401,401,18,19,22,401,401,401,401,401,399,401,401,41,43,44,45,40,
  136,135,24,49,267,268,269,266,402,24,24,48,24,47,46,46,401,401,400,38,
  270,111,112,402,50,265,202,266,266,266,266,266,266,266,266,266,266,333,
  333,333,333,333,334,335,336,337,340,340,340,341,342,346,346,346,346,347,
  348,349,350,351,352,353,354,357,357,357,358,359,360,361,362,363,364,365,
  366,370,370,370,370,371,372,373,374,375,376,248,247,246,245,244,243,264,
  263,262,261,260,259,258,257,256,255,254,253,252,251,250,249,190,116,189,
  115,188,114,187,242,241,240,239,238,237,236,235,234,233,232,231,230,229,
  228,227,227,227,227,227,227,226,225,224,223,223,223,223,223,223,222,221,
  220,219,218,217,216,216,216,216,215,215,215,215,214,213,212,211,210,209,
  208,207,206,205,204,203,201,200,199,198,197,196,195,194,193,192,191,265,
403,403,403,403,403,403,403,1,50,1,403,
20,20,20,20,20,20,20,51,1,1036,
20,20,20,20,20,20,20,52,1,1035,
20,20,20,20,20,20,20,53,1,1034,
20,20,20,20,20,20,20,54,1,1024,
20,20,20,20,20,20,20,55,1,1023,
20,20,20,20,20,20,20,56,1,1014,
20,20,20,20,20,20,20,57,1,1013,
20,20,20,20,20,20,20,58,1,1012,
20,20,20,20,20,20,20,59,1,1009,
20,20,20,20,20,20,20,60,1,1008,
20,20,20,20,20,20,20,61,1,1003,
20,20,20,20,20,20,20,62,1,1002,
20,20,20,20,20,20,20,63,1,1001,
20,20,20,20,20,20,20,64,1,1000,
20,20,20,20,20,20,20,20,20,20,65,1,1069,
20,20,20,20,20,20,20,20,20,20,66,1,1068,
20,20,20,20,20,20,20,20,67,1,1067,
20,20,20,20,20,20,20,20,20,20,20,20,20,20,20,20,20,20,20,20,20,20,20,20,20,
  20,20,20,20,20,20,20,20,20,20,20,20,20,20,20,20,20,20,68,1,1110,
20,20,20,20,20,20,20,20,20,20,20,20,20,20,20,20,20,20,20,20,20,20,20,20,20,
  20,20,20,20,20,20,20,20,20,20,20,20,20,20,20,20,20,20,69,1,1109,
20,20,20,20,20,20,20,20,20,20,20,20,20,20,20,20,20,20,20,20,20,20,20,20,20,
  20,20,20,20,20,20,20,20,20,20,20,20,20,20,20,20,20,20,70,1,1108,
20,20,20,20,20,20,20,20,20,20,20,20,20,20,20,20,20,20,20,20,20,20,20,20,20,
  20,20,20,20,20,20,20,20,20,20,20,20,20,20,20,20,20,20,71,1,1107,
20,20,20,20,20,20,20,20,20,20,20,20,20,20,20,20,20,20,20,20,20,20,20,20,20,
  20,20,20,20,20,20,20,20,20,20,20,20,20,20,20,20,20,20,72,1,1106,
20,20,20,20,20,20,20,20,20,20,20,20,20,20,20,20,20,20,20,20,20,20,20,20,20,
  20,20,20,20,20,20,20,20,20,20,20,20,20,20,20,20,20,20,73,1,1105,
20,20,20,20,20,20,20,20,20,20,20,20,20,20,20,20,20,20,20,20,20,20,20,20,20,
  20,20,20,20,20,20,20,20,20,20,20,20,20,20,20,20,20,20,74,1,1098,
20,20,20,20,20,20,20,20,20,20,20,20,20,20,20,20,20,20,20,20,20,20,20,20,20,
  20,20,20,20,20,20,20,20,20,20,20,20,20,20,20,20,20,20,75,1,1097,
20,20,20,20,20,20,20,20,20,20,20,20,20,20,20,20,20,20,20,20,20,20,20,20,20,
  20,20,20,20,20,20,20,20,20,20,20,20,20,20,20,20,20,20,76,1,1096,
20,20,20,20,20,20,20,20,20,20,20,20,20,20,20,20,20,20,20,20,20,20,20,20,20,
  20,20,20,20,20,20,20,20,20,20,20,20,20,20,20,20,20,20,77,1,1095,
20,20,20,20,20,20,20,20,20,20,20,20,20,20,20,20,20,20,20,20,20,20,20,20,20,
  20,20,20,20,20,20,20,20,20,20,20,20,20,20,20,20,20,20,78,1,1094,
20,20,20,20,20,20,20,20,20,20,20,20,20,20,20,20,20,20,20,20,20,20,20,20,20,
  20,20,20,20,20,20,20,20,20,20,20,20,20,20,20,20,20,20,79,1,1090,
20,20,20,20,20,20,20,20,20,20,20,20,20,20,20,20,20,20,20,20,20,20,20,20,20,
  20,20,20,20,20,20,20,20,20,20,20,20,20,20,20,20,20,20,80,1,1089,
20,20,20,20,20,20,20,20,20,20,20,20,20,20,20,20,20,20,20,20,20,20,20,20,20,
  20,20,20,20,20,20,20,20,20,20,20,20,20,20,20,20,20,20,81,1,1088,
20,20,20,20,20,20,20,20,20,20,20,20,20,20,20,20,20,20,20,20,20,20,20,20,20,
  20,20,20,20,20,20,20,20,20,20,20,20,20,20,20,20,20,20,82,1,1087,
20,20,20,20,20,20,20,20,20,20,20,20,20,20,20,20,20,20,20,20,20,20,20,20,20,
  20,20,20,20,20,20,20,20,20,20,20,20,20,20,20,20,20,20,83,1,1086,
20,20,20,20,20,20,20,84,1,1042,
20,20,20,20,20,20,20,85,1,1041,
20,20,20,20,20,20,20,86,1,1040,
20,20,20,20,20,20,20,87,1,1039,
20,20,20,20,20,20,20,88,1,1038,
20,20,20,20,20,20,20,89,1,1037,
20,20,20,20,20,20,20,90,1,1033,
20,20,20,20,20,20,20,91,1,1032,
20,20,20,20,20,20,20,92,1,1031,
20,20,20,20,20,20,20,93,1,1030,
20,20,20,20,20,20,20,94,1,1029,
20,20,20,20,20,20,20,95,1,1028,
20,20,20,20,20,20,20,96,1,1027,
20,20,20,20,20,20,20,97,1,1026,
20,20,20,20,20,20,20,98,1,1025,
20,20,20,20,20,20,20,99,1,1022,
20,20,20,20,20,20,20,100,1,1021,
20,20,20,20,20,20,20,101,1,1020,
20,20,20,20,20,20,20,102,1,1019,
20,20,20,20,20,20,20,103,1,1018,
20,20,20,20,20,20,20,104,1,1017,
20,20,20,20,20,20,20,105,1,1016,
20,20,20,20,20,20,20,106,1,1015,
20,20,20,20,20,20,20,107,1,1011,
20,20,20,20,20,20,20,108,1,1010,
20,20,20,20,20,20,20,109,1,1007,
20,20,20,20,20,20,20,110,1,1006,
20,20,20,20,20,20,20,111,1,1005,
20,20,20,20,20,20,20,112,1,1004,
20,20,20,20,20,113,1,1070,
404,404,404,404,404,404,404,404,404,1,114,1,404,
405,405,405,405,405,405,405,405,405,1,115,1,405,
406,406,406,406,406,406,406,1,116,1,406,
20,20,20,20,20,20,20,20,20,20,20,20,20,20,20,20,20,20,20,20,20,20,20,20,20,
  20,20,20,20,20,20,20,20,20,20,20,20,20,20,20,20,20,20,117,1,1134,
20,20,20,20,20,20,20,20,20,20,20,20,20,20,20,20,20,20,20,20,20,20,20,20,20,
  20,20,20,20,20,20,20,20,20,20,20,20,20,20,20,20,20,20,118,1,1133,
20,20,20,20,20,20,20,20,20,20,20,20,20,20,20,20,20,20,20,20,20,20,20,20,20,
  20,20,20,20,20,20,20,20,20,20,20,20,20,20,20,20,20,20,119,1,1132,
20,20,20,20,20,20,20,20,20,20,20,20,20,20,20,20,20,20,20,20,20,20,20,20,20,
  20,20,20,20,20,20,20,20,20,20,20,20,20,20,20,20,20,20,120,1,1131,
20,20,20,20,20,20,20,20,20,20,20,20,20,20,20,20,20,20,20,20,20,20,20,20,20,
  20,20,20,20,20,20,20,20,20,20,20,20,20,20,20,20,20,20,121,1,1130,
20,20,20,20,20,20,20,20,20,20,20,20,20,20,20,20,20,20,20,20,20,20,20,20,20,
  20,20,20,20,20,20,20,20,20,20,20,20,20,20,20,20,20,20,122,1,1129,
20,20,20,20,20,20,20,20,20,20,20,20,20,20,20,20,20,20,20,20,20,20,20,20,20,
  20,20,20,20,20,20,20,20,20,20,20,20,20,20,20,20,20,20,123,1,1128,
20,20,20,20,20,20,20,20,20,20,20,20,20,20,20,20,20,20,20,20,20,20,20,20,20,
  20,20,20,20,20,20,20,20,20,20,20,20,20,20,20,20,20,20,124,1,1127,
20,20,20,20,20,20,20,20,20,20,20,20,20,20,20,20,20,20,20,20,20,20,20,20,20,
  20,20,20,20,20,20,20,20,20,20,20,20,20,20,20,20,20,20,125,1,1126,
20,20,20,20,20,20,20,20,20,20,20,20,20,20,20,20,20,20,20,20,20,20,20,20,20,
  20,20,20,20,20,20,20,20,20,20,20,20,20,20,20,20,20,20,126,1,1125,
20,20,20,20,20,20,20,20,20,20,20,20,20,20,20,20,20,20,20,20,20,20,20,20,20,
  20,20,20,20,20,20,20,20,20,20,20,20,20,20,20,20,20,20,127,1,1124,
20,20,20,20,20,20,20,20,20,20,20,20,20,20,20,20,20,20,20,20,20,20,20,20,20,
  20,20,20,20,20,20,20,20,20,20,20,20,20,20,20,20,20,20,128,1,1123,
20,20,20,20,20,20,20,20,20,20,20,20,20,20,20,20,20,20,20,20,20,20,20,20,20,
  20,20,20,20,20,20,20,20,20,20,20,20,20,20,20,20,20,20,129,1,1122,
20,20,20,20,20,20,20,20,20,20,20,20,20,20,20,20,20,20,20,20,20,20,20,20,20,
  20,20,20,20,20,20,20,20,20,20,20,20,20,20,20,20,20,20,130,1,1121,
20,20,20,20,20,20,20,20,20,20,20,20,20,20,20,20,20,20,20,20,20,20,20,20,20,
  20,20,20,20,20,20,20,20,20,20,20,20,20,20,20,20,20,20,131,1,1120,
20,20,20,20,20,20,20,20,20,20,20,20,20,20,20,20,20,20,20,20,20,20,20,20,20,
  20,20,20,20,20,20,20,20,20,20,20,20,20,20,20,20,20,20,132,1,1119,
20,20,20,20,20,20,20,20,20,20,20,20,20,20,20,20,20,20,20,20,20,20,20,20,20,
  20,20,20,20,20,20,20,20,20,20,20,20,20,20,20,20,20,20,133,1,1118,
20,20,20,20,20,20,20,20,20,20,20,20,20,20,20,20,20,20,20,20,20,20,20,20,20,
  20,20,20,20,20,20,20,20,20,20,20,20,20,20,20,20,20,20,134,1,1117,
20,20,20,20,20,20,20,20,20,20,20,20,20,20,20,20,20,20,20,20,20,20,20,20,20,
  20,20,20,20,20,20,20,20,20,20,20,20,20,20,20,20,20,20,135,1,1116,
20,20,20,20,20,20,20,20,20,20,20,20,20,20,20,20,20,20,20,20,20,20,20,20,20,
  20,20,20,20,20,20,20,20,20,20,20,20,20,20,20,20,20,20,136,1,1115,
20,20,20,20,20,20,20,20,20,20,20,20,20,20,20,20,20,20,20,20,20,20,20,20,20,
  20,20,20,20,20,20,20,20,20,20,20,20,20,20,20,20,20,20,137,1,1114,
20,20,20,20,20,20,20,20,20,20,20,20,20,20,20,20,20,20,20,20,20,20,20,20,20,
  20,20,20,20,20,20,20,20,20,20,20,20,20,20,20,20,20,20,138,1,1113,
20,20,20,20,20,20,20,20,20,20,20,20,20,20,20,20,20,20,20,20,20,20,20,20,20,
  20,20,20,20,20,20,20,20,20,20,20,20,20,20,20,20,20,20,139,1,1112,
20,20,20,20,20,20,20,20,20,20,20,20,20,20,20,20,20,20,20,20,20,20,20,20,20,
  20,20,20,20,20,20,20,20,20,20,20,20,20,20,20,20,20,20,140,1,1111,
20,20,20,20,20,20,20,20,20,20,20,20,20,20,20,20,20,20,20,20,20,20,20,20,20,
  20,20,20,20,20,20,20,20,20,20,20,20,20,20,20,20,20,20,141,1,1104,
20,20,20,20,20,20,20,20,20,20,20,20,20,20,20,20,20,20,20,20,20,20,20,20,20,
  20,20,20,20,20,20,20,20,20,20,20,20,20,20,20,20,20,20,142,1,1103,
20,20,20,20,20,20,20,20,20,20,20,20,20,20,20,20,20,20,20,20,20,20,20,20,20,
  20,20,20,20,20,20,20,20,20,20,20,20,20,20,20,20,20,20,143,1,1102,
20,20,20,20,20,20,20,20,20,20,20,20,20,20,20,20,20,20,20,20,20,20,20,20,20,
  20,20,20,20,20,20,20,20,20,20,20,20,20,20,20,20,20,20,144,1,1101,
20,20,20,20,20,20,20,20,20,20,20,20,20,20,20,20,20,20,20,20,20,20,20,20,20,
  20,20,20,20,20,20,20,20,20,20,20,20,20,20,20,20,20,20,145,1,1100,
20,20,20,20,20,20,20,20,20,20,20,20,20,20,20,20,20,20,20,20,20,20,20,20,20,
  20,20,20,20,20,20,20,20,20,20,20,20,20,20,20,20,20,20,146,1,1099,
20,20,20,20,20,20,20,20,20,20,20,20,20,20,20,20,20,20,20,20,20,20,20,20,20,
  20,20,20,20,20,20,20,20,20,20,20,20,20,20,20,20,20,20,147,1,1093,
20,20,20,20,20,20,20,20,20,20,20,20,20,20,20,20,20,20,20,20,20,20,20,20,20,
  20,20,20,20,20,20,20,20,20,20,20,20,20,20,20,20,20,20,148,1,1092,
20,20,20,20,20,20,20,20,20,20,20,20,20,20,20,20,20,20,20,20,20,20,20,20,20,
  20,20,20,20,20,20,20,20,20,20,20,20,20,20,20,20,20,20,149,1,1091,
20,20,20,20,20,20,20,20,20,20,20,20,20,20,20,20,20,20,20,20,20,20,20,20,20,
  20,20,20,20,20,20,20,20,20,20,20,20,20,20,20,20,20,20,150,1,1085,
20,20,20,20,20,20,20,20,20,20,20,20,20,20,20,20,20,20,20,20,20,20,20,20,20,
  20,20,20,20,20,20,20,20,20,20,20,20,20,20,20,20,20,20,151,1,1084,
20,20,20,20,20,20,20,20,20,20,20,20,20,20,20,20,20,20,20,20,20,20,20,20,20,
  20,20,20,20,20,20,20,20,20,20,20,20,20,20,20,20,20,20,152,1,1083,
20,20,20,20,20,20,20,20,20,20,20,20,20,20,20,20,20,20,20,20,20,20,20,20,20,
  20,20,20,20,20,20,20,20,20,20,20,20,20,20,20,20,20,20,153,1,1082,
20,20,20,20,20,20,20,20,20,20,20,20,20,20,20,20,20,20,20,20,20,20,20,20,20,
  20,20,20,20,20,20,20,20,20,20,20,20,20,20,20,20,20,20,154,1,1081,
20,20,20,20,20,20,20,20,20,20,20,20,20,20,20,20,20,20,20,20,20,20,20,20,20,
  20,20,20,20,20,20,20,20,20,20,20,20,20,20,20,20,20,20,155,1,1080,
20,20,20,20,20,20,20,20,20,20,20,20,20,20,20,20,20,20,20,20,20,20,20,20,20,
  20,20,20,20,20,20,20,20,20,20,20,20,20,20,20,20,20,20,156,1,1079,
20,20,20,20,20,20,20,20,20,20,20,20,20,20,20,20,20,20,20,20,20,20,20,20,20,
  20,20,20,20,20,20,20,20,20,20,20,20,20,20,20,20,20,20,157,1,1078,
20,20,20,20,20,20,20,20,20,20,20,20,20,20,20,20,20,20,20,20,20,20,20,20,20,
  20,20,20,20,20,20,20,20,20,20,20,20,20,20,20,20,20,20,158,1,1077,
20,20,20,20,20,20,20,20,20,20,20,20,20,20,20,20,20,20,20,20,20,20,20,20,20,
  20,20,20,20,20,20,20,20,20,20,20,20,20,20,20,20,20,20,159,1,1076,
20,20,20,20,20,20,20,20,20,20,20,20,20,20,20,20,20,20,20,20,20,20,20,20,20,
  20,20,20,20,20,20,20,20,20,20,20,20,20,20,20,20,20,20,160,1,1075,
20,20,20,20,20,20,20,20,20,20,20,20,20,20,20,20,20,20,20,20,20,20,20,20,20,
  20,20,20,20,20,20,20,20,20,20,20,20,20,20,20,20,20,20,161,1,1074,
20,20,20,20,20,20,20,20,20,20,20,20,20,20,20,20,20,20,20,20,20,20,20,20,20,
  20,20,20,20,20,20,20,20,20,20,20,20,20,20,20,20,20,20,162,1,1073,
20,20,20,20,20,20,20,20,20,20,20,20,20,20,20,20,20,20,20,20,20,20,20,20,20,
  20,20,20,20,20,20,20,20,20,20,20,20,20,20,20,20,20,20,163,1,1072,
20,20,20,20,20,20,20,20,20,20,20,20,20,20,20,20,20,20,20,20,20,20,20,20,20,
  20,20,20,20,20,20,20,20,20,20,20,20,20,20,20,20,20,20,164,1,1071,
20,20,20,20,20,20,20,20,20,165,1,1048,
20,20,20,20,20,20,20,20,20,166,1,1047,
20,20,20,20,20,20,20,20,20,167,1,1046,
20,20,20,20,20,20,20,20,20,168,1,1045,
20,20,20,20,20,169,1,1044,
20,20,20,20,20,170,1,1043,
20,20,20,20,20,20,20,20,171,1,1066,
20,20,20,20,20,20,20,20,172,1,1065,
20,20,20,20,20,20,20,20,173,1,1064,
20,20,20,20,20,20,20,20,20,20,174,1,1062,
20,20,20,20,20,20,20,20,20,20,175,1,1061,
20,20,20,20,20,20,20,20,20,20,176,1,1060,
20,20,20,20,20,20,20,20,20,20,177,1,1059,
20,20,20,20,20,20,20,20,20,20,178,1,1058,
20,20,20,20,20,20,20,20,20,20,179,1,1057,
20,20,20,20,20,20,20,20,20,20,180,1,1056,
20,20,20,20,20,20,20,20,20,20,181,1,1055,
20,20,20,20,20,20,20,20,20,20,182,1,1054,
20,20,20,20,20,20,20,20,20,20,183,1,1053,
20,20,20,20,20,20,20,20,20,20,184,1,1052,
20,20,20,20,20,20,20,20,20,20,185,1,1051,
20,20,20,20,20,20,20,20,20,20,186,1,1050,
407,407,407,407,1,187,1,407,
408,1,188,1,408,
409,1,189,1,409,
410,1,190,1,410,
411,411,411,411,411,411,411,411,411,411,411,411,411,411,411,411,411,411,411,
  411,411,411,411,411,411,411,411,411,411,411,411,411,411,411,411,411,411,
  411,411,411,411,411,1,191,1,411,
412,412,412,412,412,412,412,412,412,412,412,412,412,412,412,412,412,412,412,
  412,412,412,412,412,412,412,412,412,412,412,412,412,412,412,412,412,412,
  412,412,412,412,412,1,192,1,412,
413,413,413,413,413,413,413,413,413,413,413,413,413,413,413,413,413,413,413,
  413,413,413,413,413,413,413,413,413,413,413,413,413,413,413,413,413,413,
  413,413,413,413,413,1,193,1,413,
414,414,414,414,414,414,414,414,414,414,414,414,414,414,414,414,414,414,414,
  414,414,414,414,414,414,414,414,414,414,414,414,414,414,414,414,414,414,
  414,414,414,414,414,1,194,1,414,
415,415,415,415,415,415,415,415,415,415,415,415,415,415,415,415,415,415,415,
  415,415,415,415,415,415,415,415,415,415,415,415,415,415,415,415,415,415,
  415,415,415,415,415,1,195,1,415,
416,416,416,416,416,416,416,416,416,416,416,416,416,416,416,416,416,416,416,
  416,416,416,416,416,416,416,416,416,416,416,416,416,416,416,416,416,416,
  416,416,416,416,416,1,196,1,416,
417,417,417,417,417,417,417,417,417,417,417,417,417,417,417,417,417,417,417,
  417,417,417,417,417,417,417,417,417,417,417,417,417,417,417,417,417,417,
  417,417,417,417,417,1,197,1,417,
418,418,418,418,418,418,418,418,418,418,418,418,418,418,418,418,418,418,418,
  418,418,418,418,418,418,418,418,418,418,418,418,418,418,418,418,418,418,
  418,418,418,418,418,1,198,1,418,
419,419,419,419,419,419,419,419,419,419,419,419,419,419,419,419,419,419,419,
  419,419,419,419,419,419,419,419,419,419,419,419,419,419,419,419,419,419,
  419,419,419,419,419,1,199,1,419,
420,420,420,420,420,420,420,420,420,420,420,420,420,420,420,420,420,420,420,
  420,420,420,420,420,420,420,420,420,420,420,420,420,420,420,420,420,420,
  420,420,420,420,420,1,200,1,420,
421,421,421,421,421,421,421,421,421,421,421,421,421,421,421,421,421,421,421,
  421,421,421,421,421,421,421,421,421,421,421,421,421,421,421,421,421,421,
  421,421,421,421,421,1,201,1,421,
422,422,422,422,422,422,422,422,422,422,422,422,422,422,422,422,422,422,422,
  422,422,422,422,422,422,422,422,422,422,422,422,422,422,422,422,422,422,
  422,422,422,422,422,1,202,1,422,
423,423,423,423,423,423,423,423,423,423,423,423,423,423,423,423,423,423,423,
  423,423,423,423,423,423,423,423,423,423,423,423,423,423,423,423,423,423,
  423,423,423,423,423,1,203,1,423,
424,424,424,424,424,424,424,424,424,424,424,424,424,424,424,424,424,424,424,
  424,424,424,424,424,424,424,424,424,424,424,424,424,424,424,424,424,424,
  424,424,424,424,424,1,204,1,424,
425,425,425,425,425,425,425,425,425,425,425,425,425,425,425,425,425,425,425,
  425,425,425,425,425,425,425,425,425,425,425,425,425,425,425,425,425,425,
  425,425,425,425,425,1,205,1,425,
426,426,426,426,426,426,426,426,426,426,426,426,426,426,426,426,426,426,426,
  426,426,426,426,426,426,426,426,426,426,426,426,426,426,426,426,426,426,
  426,426,426,426,426,1,206,1,426,
427,427,427,427,427,427,427,427,427,427,427,427,427,427,427,427,427,427,427,
  427,427,427,427,427,427,427,427,427,427,427,427,427,427,427,427,427,427,
  427,427,427,427,427,1,207,1,427,
428,428,428,428,428,428,428,428,428,428,428,428,428,428,428,428,428,428,428,
  428,428,428,428,428,428,428,428,428,428,428,428,428,428,428,428,428,428,
  428,428,428,428,428,1,208,1,428,
429,429,429,429,429,429,429,429,429,429,429,429,429,429,429,429,429,429,429,
  429,429,429,429,429,429,429,429,429,429,429,429,429,429,429,429,429,429,
  429,429,429,429,429,1,209,1,429,
430,430,430,430,430,430,430,430,430,430,430,430,430,430,430,430,430,430,430,
  430,430,430,430,430,430,430,430,430,430,430,430,430,430,430,430,430,430,
  430,430,430,430,430,1,210,1,430,
431,431,431,431,431,431,431,431,431,431,431,431,431,431,431,431,431,431,431,
  431,431,431,431,431,431,431,431,431,431,431,431,431,431,431,431,431,431,
  431,431,431,431,431,1,211,1,431,
432,432,432,432,432,432,432,432,432,432,432,432,432,432,432,432,432,432,432,
  432,432,432,432,432,432,432,432,432,432,432,432,432,432,432,432,432,432,
  432,432,432,432,432,1,212,1,432,
433,433,433,433,433,433,433,433,433,433,433,433,433,433,433,433,433,433,433,
  433,433,433,433,433,433,433,433,433,433,433,433,433,433,433,433,433,433,
  433,433,433,433,433,1,213,1,433,
434,434,434,434,434,434,434,434,434,434,434,434,434,434,434,434,434,434,434,
  434,434,434,434,434,434,434,434,434,434,434,434,434,434,434,434,434,434,
  434,434,434,434,434,1,214,1,434,
435,435,435,435,435,435,435,435,435,435,435,435,435,435,435,435,435,435,435,
  435,435,435,435,435,435,435,435,435,435,435,435,435,435,435,435,435,435,
  435,435,435,435,435,1,215,1,435,
436,436,436,436,436,436,436,436,436,436,436,436,436,436,436,436,436,436,436,
  436,436,436,436,436,436,436,436,436,436,436,436,436,436,436,436,436,436,
  436,436,436,436,436,1,216,1,436,
437,437,437,437,437,437,437,437,437,437,437,437,437,437,437,437,437,437,437,
  437,437,437,437,437,437,437,437,437,437,437,437,437,437,437,437,437,437,
  437,437,437,437,437,1,217,1,437,
438,438,438,438,438,438,438,438,438,438,438,438,438,438,438,438,438,438,438,
  438,438,438,438,438,438,438,438,438,438,438,438,438,438,438,438,438,438,
  438,438,438,438,438,1,218,1,438,
439,439,439,439,439,439,439,439,439,439,439,439,439,439,439,439,439,439,439,
  439,439,439,439,439,439,439,439,439,439,439,439,439,439,439,439,439,439,
  439,439,439,439,439,1,219,1,439,
440,440,440,440,440,440,440,440,440,440,440,440,440,440,440,440,440,440,440,
  440,440,440,440,440,440,440,440,440,440,440,440,440,440,440,440,440,440,
  440,440,440,440,440,1,220,1,440,
441,441,441,441,441,441,441,441,441,441,441,441,441,441,441,441,441,441,441,
  441,441,441,441,441,441,441,441,441,441,441,441,441,441,441,441,441,441,
  441,441,441,441,441,1,221,1,441,
442,442,442,442,442,442,442,442,442,442,442,442,442,442,442,442,442,442,442,
  442,442,442,442,442,442,442,442,442,442,442,442,442,442,442,442,442,442,
  442,442,442,442,442,1,222,1,442,
443,443,443,443,443,443,443,443,443,443,443,443,443,443,443,443,443,443,443,
  443,443,443,443,443,443,443,443,443,443,443,443,443,443,443,443,443,443,
  443,443,443,443,443,1,223,1,443,
444,444,444,444,444,444,444,444,444,444,444,444,444,444,444,444,444,444,444,
  444,444,444,444,444,444,444,444,444,444,444,444,444,444,444,444,444,444,
  444,444,444,444,444,1,224,1,444,
445,445,445,445,445,445,445,445,445,445,445,445,445,445,445,445,445,445,445,
  445,445,445,445,445,445,445,445,445,445,445,445,445,445,445,445,445,445,
  445,445,445,445,445,1,225,1,445,
446,446,446,446,446,446,446,446,446,446,446,446,446,446,446,446,446,446,446,
  446,446,446,446,446,446,446,446,446,446,446,446,446,446,446,446,446,446,
  446,446,446,446,446,1,226,1,446,
447,447,447,447,447,447,447,447,447,447,447,447,447,447,447,447,447,447,447,
  447,447,447,447,447,447,447,447,447,447,447,447,447,447,447,447,447,447,
  447,447,447,447,447,1,227,1,447,
448,448,448,448,448,448,448,448,448,448,448,448,448,448,448,448,448,448,448,
  448,448,448,448,448,448,448,448,448,448,448,448,448,448,448,448,448,448,
  448,448,448,448,448,1,228,1,448,
449,449,449,449,449,449,449,449,449,449,449,449,449,449,449,449,449,449,449,
  449,449,449,449,449,449,449,449,449,449,449,449,449,449,449,449,449,449,
  449,449,449,449,449,1,229,1,449,
450,450,450,450,450,450,450,450,450,450,450,450,450,450,450,450,450,450,450,
  450,450,450,450,450,450,450,450,450,450,450,450,450,450,450,450,450,450,
  450,450,450,450,450,1,230,1,450,
451,451,451,451,451,451,451,451,451,451,451,451,451,451,451,451,451,451,451,
  451,451,451,451,451,451,451,451,451,451,451,451,451,451,451,451,451,451,
  451,451,451,451,451,1,231,1,451,
452,452,452,452,452,452,452,452,452,452,452,452,452,452,452,452,452,452,452,
  452,452,452,452,452,452,452,452,452,452,452,452,452,452,452,452,452,452,
  452,452,452,452,452,1,232,1,452,
453,453,453,453,453,453,453,453,453,453,453,453,453,453,453,453,453,453,453,
  453,453,453,453,453,453,453,453,453,453,453,453,453,453,453,453,453,453,
  453,453,453,453,453,1,233,1,453,
454,454,454,454,454,454,454,454,454,454,454,454,454,454,454,454,454,454,454,
  454,454,454,454,454,454,454,454,454,454,454,454,454,454,454,454,454,454,
  454,454,454,454,454,1,234,1,454,
455,455,455,455,455,455,455,455,455,455,455,455,455,455,455,455,455,455,455,
  455,455,455,455,455,455,455,455,455,455,455,455,455,455,455,455,455,455,
  455,455,455,455,455,1,235,1,455,
456,456,456,456,456,456,456,456,456,456,456,456,456,456,456,456,456,456,456,
  456,456,456,456,456,456,456,456,456,456,456,456,456,456,456,456,456,456,
  456,456,456,456,456,1,236,1,456,
457,457,457,457,457,457,457,457,457,457,457,457,457,457,457,457,457,457,457,
  457,457,457,457,457,457,457,457,457,457,457,457,457,457,457,457,457,457,
  457,457,457,457,457,1,237,1,457,
458,458,458,458,458,458,458,458,458,458,458,458,458,458,458,458,458,458,458,
  458,458,458,458,458,458,458,458,458,458,458,458,458,458,458,458,458,458,
  458,458,458,458,458,1,238,1,458,
459,459,459,459,459,459,459,459,459,459,459,459,459,459,459,459,459,459,459,
  459,459,459,459,459,459,459,459,459,459,459,459,459,459,459,459,459,459,
  459,459,459,459,459,1,239,1,459,
460,460,460,460,460,460,460,460,460,460,460,460,460,460,460,460,460,460,460,
  460,460,460,460,460,460,460,460,460,460,460,460,460,460,460,460,460,460,
  460,460,460,460,460,1,240,1,460,
461,461,461,461,461,461,461,461,461,461,461,461,461,461,461,461,461,461,461,
  461,461,461,461,461,461,461,461,461,461,461,461,461,461,461,461,461,461,
  461,461,461,461,461,1,241,1,461,
462,462,462,462,462,462,462,462,462,462,462,462,462,462,462,462,462,462,462,
  462,462,462,462,462,462,462,462,462,462,462,462,462,462,462,462,462,462,
  462,462,462,462,462,1,242,1,462,
463,463,463,463,463,463,463,463,1,243,1,463,
464,464,464,464,464,464,464,464,1,244,1,464,
465,465,465,465,465,465,465,465,1,245,1,465,
466,466,466,466,466,466,466,466,1,246,1,466,
467,467,467,467,1,247,1,467,
468,468,468,468,1,248,1,468,
469,469,469,469,469,469,469,1,249,1,469,
470,470,470,470,470,470,470,1,250,1,470,
471,471,471,471,471,471,471,1,251,1,471,
472,472,472,472,472,472,472,472,472,1,252,1,472,
473,473,473,473,473,473,473,473,473,1,253,1,473,
474,474,474,474,474,474,474,474,474,1,254,1,474,
475,475,475,475,475,475,475,475,475,1,255,1,475,
476,476,476,476,476,476,476,476,476,1,256,1,476,
477,477,477,477,477,477,477,477,477,1,257,1,477,
478,478,478,478,478,478,478,478,478,1,258,1,478,
479,479,479,479,479,479,479,479,479,1,259,1,479,
480,480,480,480,480,480,480,480,480,1,260,1,480,
481,481,481,481,481,481,481,481,481,1,261,1,481,
482,482,482,482,482,482,482,482,482,1,262,1,482,
483,483,483,483,483,483,483,483,483,1,263,1,483,
484,484,484,484,484,484,484,484,484,1,264,1,484,
20,138,138,138,138,138,138,138,138,138,138,138,20,138,20,20,139,139,139,139,
  139,20,138,20,20,20,20,20,20,20,20,20,20,20,20,20,20,20,20,20,20,20,20,
  20,20,20,20,20,20,20,138,20,20,20,20,20,1,265,1,897,139,
20,20,20,20,20,20,1,266,1,317,
66,66,66,66,66,66,66,66,66,66,66,66,66,66,66,66,66,66,66,66,66,66,66,66,66,
  66,66,66,66,66,66,66,66,66,66,66,66,66,66,66,66,66,66,66,66,66,66,66,66,
  66,66,66,66,66,66,66,66,66,310,66,66,66,66,66,66,66,66,66,66,267,485,
122,122,122,122,122,486,268,486,
487,121,488,
137,137,137,137,137,137,137,137,137,137,137,285,137,256,256,256,256,288,290,
  287,297,299,300,301,302,303,304,305,306,307,323,324,325,286,322,289,298,
  310,284,271,136,135,270,240,295,296,241,291,242,292,267,335,334,332,337,
  333,336,331,338,110,226,326,312,265,339,327,331,330,329,328,225,331,321,
  320,319,318,317,316,315,314,313,311,294,293,265,309,308,
298,489,491,493,494,495,497,498,499,503,506,508,510,512,514,516,518,519,521,
  522,523,525,526,528,529,530,531,534,535,14,15,16,343,345,347,349,351,
  271,75,76,533,77,79,505,502,501,70,71,532,532,532,532,84,527,527,527,
  524,524,524,524,520,520,520,517,515,513,511,509,507,504,500,113,114,496,
  117,118,492,490,123,
536,536,536,536,536,1,272,1,536,
537,537,537,537,537,537,537,537,537,537,537,537,537,537,537,537,537,537,537,
  537,537,537,537,537,537,537,537,537,537,537,537,537,537,537,537,537,537,
  537,537,537,1,273,1,537,
538,538,538,538,538,1,274,1,538,
146,275,539,278,279,
146,276,540,278,279,
137,137,137,137,137,137,137,137,137,137,137,285,137,256,256,256,256,288,290,
  287,146,297,299,300,301,302,303,304,305,306,307,323,324,325,286,322,289,
  298,310,284,271,136,135,277,240,295,296,241,291,542,278,242,292,267,335,
  334,332,337,333,336,331,541,226,326,312,265,327,331,330,329,328,225,331,
  321,320,319,318,317,316,315,314,313,311,294,293,279,265,309,308,
149,543,149,149,149,149,149,149,149,149,149,149,149,149,149,149,149,149,149,
  149,149,149,149,145,149,149,149,149,149,149,149,149,149,149,149,149,149,
  149,149,149,149,149,278,147,
20,20,20,20,20,20,20,20,20,279,1,890,
544,544,544,544,544,1,280,1,544,
43,44,45,40,57,281,57,57,48,57,47,46,46,
43,44,45,40,56,282,56,56,48,56,47,46,46,
545,545,545,545,545,1,283,1,545,
282,282,282,282,282,284,
267,267,267,267,285,
265,265,286,
137,137,137,137,137,137,137,261,261,261,261,137,261,261,261,261,261,137,137,
  137,137,137,137,137,137,137,137,137,137,137,137,137,137,137,137,137,137,
  137,137,137,137,137,137,137,137,137,137,137,137,137,137,137,137,137,137,
  137,137,137,137,287,
256,546,256,256,256,256,256,256,256,256,256,256,256,256,256,256,256,256,256,
  256,256,256,256,256,256,256,256,256,256,256,256,256,256,256,256,256,256,
  256,256,256,256,256,256,256,256,256,256,256,256,256,256,256,256,256,256,
  288,
254,254,254,254,254,289,
272,547,272,272,272,272,272,272,272,272,272,272,272,272,272,272,272,272,272,
  272,272,272,272,272,272,272,272,272,272,272,272,272,272,272,272,272,272,
  272,272,272,278,272,290,548,
283,283,283,283,283,280,
247,264,264,264,264,264,264,264,264,264,292,
268,268,268,268,246,246,246,246,246,246,246,246,246,246,246,246,246,246,246,
  246,246,246,246,246,246,246,246,246,246,246,246,246,246,246,246,246,246,
  246,246,246,246,246,246,293,
266,266,245,245,245,245,245,245,245,245,245,245,245,245,245,245,245,245,245,
  245,245,245,245,245,245,245,245,245,245,245,245,245,245,245,245,245,245,
  245,245,245,245,294,
262,262,262,262,262,262,262,262,262,244,244,244,244,244,244,244,244,244,244,
  244,244,244,244,244,244,244,244,244,244,244,244,244,244,244,244,244,244,
  244,244,244,244,244,244,244,244,244,244,244,295,
258,248,263,549,550,263,257,257,257,257,257,259,243,243,243,243,243,243,243,
  243,243,243,243,243,243,243,243,243,243,243,243,243,243,243,243,243,243,
  243,243,243,243,243,243,281,243,243,243,243,252,252,243,243,296,252,
20,20,297,1,992,
20,20,20,20,20,20,20,298,1,946,
20,20,299,1,990,
20,20,300,1,989,
20,20,301,1,988,
20,20,302,1,987,
20,20,303,1,986,
20,20,304,1,985,
20,20,305,1,984,
20,20,306,1,983,
20,20,307,1,982,
20,20,20,20,20,20,20,20,20,20,20,20,20,20,20,20,20,20,20,20,20,20,20,20,20,
  20,20,20,20,20,20,20,20,20,20,20,20,20,20,308,1,993,
20,20,20,20,20,20,20,20,20,20,20,20,20,20,20,20,20,20,20,20,20,20,20,20,20,
  20,20,20,20,20,20,20,20,20,20,20,20,20,1,309,1,947,
20,20,20,20,20,20,20,20,20,20,20,20,20,20,20,20,20,20,20,20,20,20,20,20,20,
  20,20,20,20,20,20,20,20,20,20,20,20,20,20,20,20,20,20,20,20,20,20,20,20,
  20,20,20,20,20,20,20,20,20,20,20,20,20,20,310,1,915,
551,1,311,1,551,
552,1,312,1,552,
553,1,313,1,553,
554,1,314,1,554,
555,1,315,1,555,
556,1,316,1,556,
557,1,317,1,557,
558,1,318,1,558,
559,1,319,1,559,
560,1,320,1,560,
561,1,321,1,561,
20,20,20,20,20,20,20,20,20,20,20,20,20,255,255,255,255,255,20,20,20,20,20,
  20,20,20,20,20,20,20,20,20,20,20,20,20,20,20,20,1,322,1,973,
20,20,20,20,20,20,20,20,20,20,20,20,20,20,20,20,20,20,20,20,20,20,20,20,20,
  20,20,20,20,20,20,20,20,20,20,20,20,20,20,20,20,20,20,20,20,20,20,20,20,
  20,20,20,20,20,20,20,20,20,20,20,20,20,323,1,981,
20,20,20,20,20,20,20,20,20,20,20,20,20,20,20,20,20,20,20,20,20,20,20,20,20,
  20,20,20,20,20,20,20,20,20,20,20,20,20,20,1,324,1,980,
20,20,20,20,20,20,20,20,20,20,20,20,20,20,20,20,20,20,20,20,20,20,20,20,20,
  20,20,20,20,20,20,20,20,20,20,20,20,20,20,20,20,20,20,20,20,20,20,20,20,
  20,20,20,20,20,20,20,20,20,20,20,20,20,325,1,979,
562,562,562,562,562,562,562,562,562,562,562,562,562,562,562,562,562,562,562,
  562,562,562,562,562,562,562,562,562,562,562,562,562,562,562,562,562,562,
  562,562,562,562,562,1,326,1,562,
137,137,137,137,137,137,137,137,137,137,137,285,137,256,256,256,256,288,290,
  287,297,299,300,301,302,303,304,305,306,307,286,563,289,298,310,284,271,
  136,135,327,240,295,296,241,291,242,292,267,226,326,312,265,224,225,224,
  321,320,319,318,317,316,315,314,313,311,294,293,265,309,308,
137,137,137,137,137,137,137,137,137,137,137,285,137,256,256,256,256,288,290,
  287,297,299,300,301,302,303,304,305,306,307,286,563,289,298,310,284,271,
  136,135,328,240,295,296,241,291,242,292,267,226,326,312,265,223,225,223,
  321,320,319,318,317,316,315,314,313,311,294,293,265,309,308,
137,137,137,137,137,137,137,137,137,137,137,285,137,256,256,256,256,288,290,
  287,297,299,300,301,302,303,304,305,306,307,286,563,289,298,310,284,271,
  136,135,329,240,295,296,241,291,242,292,267,226,326,312,265,222,225,222,
  321,320,319,318,317,316,315,314,313,311,294,293,265,309,308,
137,137,137,137,137,137,137,137,137,137,137,285,137,256,256,256,256,288,290,
  287,297,299,300,301,302,303,304,305,306,307,286,563,289,298,310,284,271,
  136,135,330,240,295,296,241,291,242,292,267,226,326,312,265,221,225,221,
  321,320,319,318,317,316,315,314,313,311,294,293,265,309,308,
20,20,20,20,20,20,20,20,20,20,20,20,20,20,20,20,20,20,20,20,20,20,20,20,20,
  20,20,20,20,20,20,20,20,20,20,20,20,20,1,331,1,214,
20,564,566,391,383,387,20,20,20,20,20,20,20,20,20,20,20,20,20,20,20,20,20,
  20,20,20,20,20,20,20,20,20,20,20,20,20,1,332,1,211,570,569,568,567,565,
20,384,386,20,20,20,20,20,20,20,20,20,20,20,20,20,20,20,20,20,20,20,20,20,
  20,20,20,20,20,20,20,20,20,1,333,1,206,572,571,
20,573,575,577,579,20,20,20,20,20,20,20,20,20,20,20,20,20,20,20,20,20,20,20,
  20,20,20,20,20,20,20,1,334,1,203,580,578,576,574,
20,581,390,20,20,20,20,20,20,20,20,20,20,20,20,20,20,20,20,20,20,20,20,20,
  20,20,20,1,335,1,200,583,582,
20,584,362,20,20,20,20,20,20,20,20,20,20,20,20,20,20,20,20,20,20,20,20,20,
  20,1,336,1,197,586,585,
587,360,196,589,588,
590,591,592,593,594,595,596,597,598,599,600,601,399,176,176,176,176,176,176,
  338,180,180,180,180,183,183,183,186,186,186,189,189,189,192,192,192,195,
  195,195,
137,137,137,137,137,137,137,137,137,137,137,285,137,256,256,256,256,288,290,
  287,297,299,300,301,302,303,304,305,306,307,323,324,325,286,322,289,298,
  310,284,271,136,135,339,240,295,296,241,291,242,292,267,335,334,332,337,
  333,336,331,175,226,326,312,265,327,331,330,329,328,225,331,321,320,319,
  318,317,316,315,314,313,311,294,293,265,309,308,
602,602,602,602,602,1,340,1,602,
603,603,603,603,603,1,341,1,603,
604,604,604,604,604,604,342,604,
20,20,20,20,20,20,343,1,900,
605,605,605,605,605,1,344,1,605,
20,20,20,20,20,20,345,1,899,
606,606,606,606,606,1,346,1,606,
20,20,20,20,20,20,20,20,20,20,20,20,20,20,20,20,347,1,898,
607,607,607,607,607,607,607,607,607,607,607,607,607,607,607,1,348,1,607,
20,20,20,20,20,20,349,1,896,
608,608,608,608,608,1,350,1,608,
20,20,20,20,20,20,351,1,894,
609,609,609,609,609,1,352,1,609,
165,165,610,165,165,165,165,165,165,165,165,165,165,165,165,163,165,165,165,
  165,165,165,165,165,165,165,165,165,165,165,165,165,165,165,165,165,165,
  165,165,165,165,353,
20,20,20,20,20,20,354,1,891,
611,611,611,611,611,1,355,1,611,
612,612,612,612,612,1,356,1,612,
20,20,20,20,20,20,20,20,20,20,20,20,20,20,20,20,20,20,20,20,20,1,357,1,889,
140,140,140,140,140,140,140,140,140,140,140,140,164,140,140,140,140,140,146,
  140,140,358,616,278,615,353,614,279,354,613,
20,20,20,20,20,20,20,20,20,20,20,20,20,20,20,20,20,20,20,20,20,20,20,20,20,
  20,20,20,20,20,20,20,20,20,20,20,20,20,20,20,20,20,20,20,20,20,20,20,
  359,1,1160,
20,20,20,20,20,20,20,20,20,20,20,20,20,20,20,20,20,20,20,20,20,20,20,20,20,
  20,20,20,20,20,20,20,20,20,20,20,20,20,20,20,20,20,20,20,20,20,20,20,20,
  20,20,20,20,20,20,20,20,20,20,20,20,20,20,360,1,962,
20,20,20,20,20,20,20,20,20,20,20,20,20,20,20,20,20,20,20,20,20,20,20,20,20,
  20,20,20,20,20,20,20,20,20,20,20,20,20,20,20,20,20,20,20,20,20,20,20,
  361,1,1159,
20,20,20,20,20,20,20,20,20,20,20,20,20,20,20,20,20,20,20,20,20,20,20,20,20,
  20,20,20,20,20,20,20,20,20,20,20,20,20,20,20,20,20,20,20,20,20,20,20,20,
  20,20,20,20,20,20,20,20,20,20,20,20,20,20,362,1,964,
20,20,20,20,20,20,20,20,20,20,20,20,20,20,20,20,20,20,20,20,20,20,20,20,20,
  20,20,20,20,20,20,20,20,20,20,20,20,20,20,20,20,20,20,20,20,363,1,1158,
20,20,20,20,20,20,20,20,20,20,20,20,20,20,20,20,20,20,20,20,20,20,20,20,20,
  20,20,20,20,20,20,20,20,20,20,20,20,20,20,20,20,20,20,20,20,20,20,20,
  364,1,1157,
20,20,20,20,20,20,20,20,20,20,20,20,20,20,20,20,20,20,20,20,20,20,20,20,20,
  20,20,20,20,20,20,20,20,20,20,20,20,20,20,20,20,20,20,20,20,20,20,20,
  365,1,1156,
20,20,20,20,20,20,20,20,20,20,20,20,20,20,20,20,20,20,20,20,20,20,20,20,20,
  20,20,20,20,20,20,20,20,20,20,20,20,20,20,20,20,20,20,20,20,20,20,20,
  366,1,1155,
20,20,20,20,20,20,20,20,20,20,20,20,20,20,20,20,20,20,20,20,20,20,20,20,20,
  20,20,20,20,20,20,20,20,20,20,20,20,20,20,20,20,20,20,20,20,20,20,20,
  367,1,1154,
20,20,20,20,20,20,20,20,20,20,20,20,20,20,20,20,20,20,20,20,20,20,20,20,20,
  20,20,20,20,20,20,20,20,20,20,20,20,20,20,20,20,20,20,20,20,20,20,20,
  368,1,1153,
20,20,20,20,20,20,20,20,20,20,20,20,20,20,20,20,20,20,20,20,20,20,20,20,20,
  20,20,20,20,20,20,20,20,20,20,20,20,20,20,20,20,20,20,20,20,20,20,20,
  369,1,1152,
20,20,20,20,20,20,20,20,20,20,20,20,20,20,20,20,20,20,20,20,20,20,20,20,20,
  20,20,20,20,20,20,20,20,20,20,20,20,20,20,20,20,20,20,20,20,20,20,20,
  370,1,1151,
20,20,20,20,20,20,20,20,20,20,20,20,20,20,20,20,20,20,20,20,20,20,20,20,20,
  20,20,20,20,20,20,20,20,20,20,20,20,20,20,20,20,20,20,20,20,20,20,20,
  371,1,1150,
20,20,20,20,20,20,20,20,20,20,20,20,20,20,20,20,20,20,20,20,20,20,20,20,20,
  20,20,20,20,20,20,20,20,20,20,20,20,20,20,20,20,20,20,20,20,20,20,20,
  372,1,1149,
20,20,20,20,20,20,20,20,20,20,20,20,20,20,20,20,20,20,20,20,20,20,20,20,20,
  20,20,20,20,20,20,20,20,20,20,20,20,20,20,20,20,20,20,20,20,20,20,20,
  373,1,1148,
20,20,20,20,20,20,20,20,20,20,20,20,20,20,20,20,20,20,20,20,20,20,20,20,20,
  20,20,20,20,20,20,20,20,20,20,20,20,20,20,20,20,20,20,20,20,20,20,20,
  374,1,1147,
20,20,20,20,20,20,20,20,20,20,20,20,20,20,20,20,20,20,20,20,20,20,20,20,20,
  20,20,20,20,20,20,20,20,20,20,20,20,20,20,20,20,20,20,20,20,20,20,20,
  375,1,1146,
20,20,20,20,20,20,20,20,20,20,20,20,20,20,20,20,20,20,20,20,20,20,20,20,20,
  20,20,20,20,20,20,20,20,20,20,20,20,20,20,20,20,20,20,20,20,20,20,20,
  376,1,1145,
20,20,20,20,20,20,20,20,20,20,20,20,20,20,20,20,20,20,20,20,20,20,20,20,20,
  20,20,20,20,20,20,20,20,20,20,20,20,20,20,20,20,20,20,20,20,20,20,20,
  377,1,1144,
20,20,20,20,20,20,20,20,20,20,20,20,20,20,20,20,20,20,20,20,20,20,20,20,20,
  20,20,20,20,20,20,20,20,20,20,20,20,20,20,20,20,20,20,20,20,20,20,20,
  378,1,1143,
20,20,20,20,20,20,20,20,20,20,20,20,20,20,20,20,20,20,20,20,20,20,20,20,20,
  20,20,20,20,20,20,20,20,20,20,20,20,20,20,20,20,20,20,20,20,20,20,20,
  379,1,1142,
20,20,20,20,20,20,20,20,20,20,20,20,20,20,20,20,20,20,20,20,20,20,20,20,20,
  20,20,20,20,20,20,20,20,20,20,20,20,20,20,20,20,20,20,20,20,20,20,20,
  380,1,1141,
20,20,20,20,20,20,20,20,20,20,20,20,20,20,20,20,20,20,20,20,20,20,20,20,20,
  20,20,20,20,20,20,20,20,20,20,20,20,20,20,20,20,20,20,20,20,20,20,20,
  381,1,1140,
20,20,20,20,20,20,20,20,20,20,20,20,20,20,20,20,20,20,20,20,20,20,20,20,20,
  20,20,20,20,20,20,20,20,20,20,20,20,20,20,20,20,20,20,20,20,20,20,20,
  382,1,1139,
20,20,20,20,20,20,20,20,20,20,20,20,20,20,20,20,20,20,20,20,20,20,20,20,20,
  20,20,20,20,20,20,20,20,20,20,20,20,20,20,20,20,20,20,20,20,20,20,20,20,
  20,20,20,20,20,20,20,20,20,20,20,20,20,383,1,975,
20,20,20,20,20,20,20,20,20,20,20,20,20,20,20,20,20,20,20,20,20,20,20,20,20,
  20,20,20,20,20,20,20,20,20,20,20,20,20,20,20,20,20,20,20,20,20,20,20,20,
  20,20,20,20,20,20,20,20,20,20,20,20,20,20,384,1,973,
20,20,20,20,20,20,20,20,20,20,20,20,20,20,20,20,20,20,20,20,20,20,20,20,20,
  20,20,20,20,20,20,20,20,20,20,20,20,20,20,20,20,20,20,20,20,20,20,20,20,
  20,20,20,20,20,20,20,20,20,20,20,20,20,20,385,1,910,
20,20,20,20,20,20,20,20,20,20,20,20,20,20,20,20,20,20,20,20,20,20,20,20,20,
  20,20,20,20,20,20,20,20,20,20,20,20,20,20,20,20,20,20,20,20,20,20,20,20,
  20,20,20,20,20,20,20,20,20,20,20,20,20,20,386,1,972,
20,20,20,20,20,20,20,20,20,20,20,20,20,20,20,20,20,20,20,20,20,20,20,20,20,
  20,20,20,20,20,20,20,20,20,20,20,20,20,20,20,20,20,20,20,20,20,20,20,20,
  20,20,20,20,20,20,20,20,20,20,20,20,20,387,1,974,
20,20,20,20,20,20,20,20,20,20,20,20,20,20,20,20,20,20,20,20,20,20,20,20,20,
  20,20,20,20,20,20,20,20,20,20,20,20,20,20,20,20,20,20,20,20,20,20,20,20,
  20,20,20,20,20,20,20,20,20,20,20,20,20,20,20,20,20,20,20,20,20,20,20,
  388,1,914,
20,20,20,20,20,20,20,20,20,20,20,20,20,20,20,20,20,20,20,20,20,20,20,20,20,
  20,20,20,20,20,20,20,20,20,20,20,20,20,20,20,20,20,20,20,20,20,20,20,
  389,1,1138,
20,20,20,20,20,20,20,20,20,20,20,20,20,20,20,20,20,20,20,20,20,20,20,20,20,
  20,20,20,20,20,20,20,20,20,20,20,20,20,20,20,20,20,20,20,20,20,20,20,20,
  20,20,20,20,20,20,20,20,20,20,20,20,20,20,390,1,966,
20,20,20,20,20,20,20,20,20,20,20,20,20,20,20,20,20,20,20,20,20,20,20,20,20,
  20,20,20,20,20,20,20,20,20,20,20,20,20,20,20,20,20,20,20,20,20,20,20,20,
  20,20,20,20,20,20,20,20,20,20,20,20,20,391,1,976,
20,20,20,20,20,20,20,20,20,20,20,20,20,20,20,20,20,20,20,20,20,20,20,20,20,
  20,20,20,20,20,20,20,20,20,20,20,20,20,20,20,20,20,20,20,20,20,20,20,
  392,1,1137,
20,20,20,20,20,20,20,20,20,20,20,20,20,20,20,20,20,20,20,20,20,20,20,20,20,
  20,20,20,20,20,20,20,20,20,20,20,20,20,20,20,20,20,20,20,20,20,20,20,
  393,1,1136,
20,20,20,20,20,20,20,20,20,20,20,20,20,20,20,20,20,20,20,20,20,20,20,20,20,
  20,20,20,20,20,20,20,20,20,20,20,20,20,20,20,20,20,20,20,20,20,20,20,
  394,1,1135,
20,20,20,20,20,20,20,20,20,20,20,20,20,20,20,20,20,20,20,20,20,20,20,20,20,
  20,20,20,20,20,20,20,20,20,20,20,20,20,20,20,20,20,20,20,20,20,20,20,20,
  20,20,20,20,20,20,395,1,881,
359,361,363,364,365,366,367,368,369,370,371,372,373,374,375,376,377,378,379,
  380,381,382,389,392,393,394,323,325,391,383,387,384,386,390,362,360,310,
  388,385,41,44,7,9,7,7,7,7,7,7,7,7,7,7,7,7,7,7,7,7,7,7,7,7,7,7,7,7,7,7,7,
  7,7,7,7,7,7,7,7,7,7,7,7,7,7,
40,397,11,
40,398,10,
20,20,20,20,20,20,20,20,20,20,20,20,20,20,20,20,20,20,20,20,20,20,20,20,20,
  20,20,20,20,20,20,20,20,20,20,20,20,20,20,20,20,20,1,399,1,887,
137,137,137,137,137,137,137,137,137,137,137,285,137,256,256,256,256,288,290,
  287,297,299,300,301,302,303,304,305,306,307,323,324,325,286,322,289,298,
  310,284,271,136,135,400,240,295,296,241,291,242,292,267,335,334,332,337,
  333,336,331,617,226,326,312,265,327,331,330,329,328,225,331,321,320,319,
  318,317,316,315,314,313,311,294,293,265,309,308,
298,489,491,493,494,495,497,498,499,503,506,508,510,512,514,516,518,519,521,
  522,523,525,526,528,529,530,531,534,535,14,15,16,343,345,347,349,351,
  618,620,401,621,619,75,76,533,77,79,505,502,501,70,71,532,532,532,532,
  84,527,527,527,524,524,524,524,520,520,520,517,515,513,511,509,507,504,
  500,113,114,496,117,118,492,490,123,
622,622,622,622,622,1,402,1,622,
623,298,43,44,45,40,72,403,624,72,72,48,72,47,46,46,173,174,
311,625,626,627,313,312,313,312,311,404,311,312,313,316,403,314,315,
293,292,292,289,288,287,286,285,284,405,401,292,628,
296,303,302,299,302,299,296,406,399,296,299,302,629,
405,405,405,405,407,405,
385,408,630,
385,409,631,
385,410,632,
137,137,137,137,137,137,137,137,137,137,137,285,137,256,256,256,256,288,290,
  287,297,299,300,301,302,303,304,305,306,307,323,324,325,286,322,289,298,
  310,284,271,136,135,411,240,295,296,241,291,242,292,267,335,334,332,337,
  333,336,331,473,226,326,312,265,327,331,330,329,328,225,331,321,320,319,
  318,317,316,315,314,313,311,294,293,265,309,308,
137,137,137,137,137,137,137,137,137,137,137,285,137,256,256,256,256,288,290,
  287,297,299,300,301,302,303,304,305,306,307,323,324,325,286,322,289,298,
  310,284,271,136,135,412,240,295,296,241,291,242,292,267,335,334,332,337,
  333,336,331,472,226,326,312,265,327,331,330,329,328,225,331,321,320,319,
  318,317,316,315,314,313,311,294,293,265,309,308,
137,137,137,137,137,137,137,137,137,137,137,285,137,256,256,256,256,288,290,
  287,297,299,300,301,302,303,304,305,306,307,323,324,325,286,322,289,298,
  310,284,271,136,135,413,240,295,296,241,291,242,292,267,335,334,332,337,
  333,336,331,471,226,326,312,265,327,331,330,329,328,225,331,321,320,319,
  318,317,316,315,314,313,311,294,293,265,309,308,
137,137,137,137,137,137,137,137,137,137,137,285,137,256,256,256,256,288,290,
  287,297,299,300,301,302,303,304,305,306,307,323,324,325,286,322,289,298,
  310,284,271,136,135,414,240,295,296,241,291,242,292,267,335,334,332,337,
  333,336,331,470,226,326,312,265,327,331,330,329,328,225,331,321,320,319,
  318,317,316,315,314,313,311,294,293,265,309,308,
137,137,137,137,137,137,137,137,137,137,137,285,137,256,256,256,256,288,290,
  287,297,299,300,301,302,303,304,305,306,307,323,324,325,286,322,289,298,
  310,284,271,136,135,415,240,295,296,241,291,242,292,267,335,334,332,337,
  333,336,331,469,226,326,312,265,327,331,330,329,328,225,331,321,320,319,
  318,317,316,315,314,313,311,294,293,265,309,308,
137,137,137,137,137,137,137,137,137,137,137,285,137,256,256,256,256,288,290,
  287,297,299,300,301,302,303,304,305,306,307,323,324,325,286,322,289,298,
  310,284,271,136,135,416,240,295,296,241,291,242,292,267,335,334,332,337,
  333,336,331,468,226,326,312,265,327,331,330,329,328,225,331,321,320,319,
  318,317,316,315,314,313,311,294,293,265,309,308,
137,137,137,137,137,137,137,137,137,137,137,285,137,256,256,256,256,288,290,
  287,297,299,300,301,302,303,304,305,306,307,323,324,325,286,322,289,298,
  310,284,271,136,135,417,240,295,296,241,291,242,292,267,335,334,332,337,
  333,336,331,467,226,326,312,265,327,331,330,329,328,225,331,321,320,319,
  318,317,316,315,314,313,311,294,293,265,309,308,
137,137,137,137,137,137,137,137,137,137,137,285,137,256,256,256,256,288,290,
  287,297,299,300,301,302,303,304,305,306,307,323,324,325,286,322,289,298,
  310,284,271,136,135,418,240,295,296,241,291,242,292,267,335,334,332,337,
  333,336,331,466,226,326,312,265,327,331,330,329,328,225,331,321,320,319,
  318,317,316,315,314,313,311,294,293,265,309,308,
137,137,137,137,137,137,137,137,137,137,137,285,137,256,256,256,256,288,290,
  287,297,299,300,301,302,303,304,305,306,307,323,324,325,286,322,289,298,
  310,284,271,136,135,419,240,295,296,241,291,242,292,267,335,334,332,337,
  333,336,331,465,226,326,312,265,327,331,330,329,328,225,331,321,320,319,
  318,317,316,315,314,313,311,294,293,265,309,308,
137,137,137,137,137,137,137,137,137,137,137,285,137,256,256,256,256,288,290,
  287,297,299,300,301,302,303,304,305,306,307,323,324,325,286,322,289,298,
  310,284,271,136,135,420,240,295,296,241,291,242,292,267,335,334,332,337,
  333,336,331,464,226,326,312,265,327,331,330,329,328,225,331,321,320,319,
  318,317,316,315,314,313,311,294,293,265,309,308,
137,137,137,137,137,137,137,137,137,137,137,285,137,256,256,256,256,288,290,
  287,297,299,300,301,302,303,304,305,306,307,323,324,325,286,322,289,298,
  310,284,271,136,135,421,240,295,296,241,291,242,292,267,335,334,332,337,
  333,336,331,463,226,326,312,265,327,331,330,329,328,225,331,321,320,319,
  318,317,316,315,314,313,311,294,293,265,309,308,
137,137,137,137,137,137,137,137,137,137,137,285,137,256,256,256,256,288,290,
  287,297,299,300,301,302,303,304,305,306,307,323,324,325,286,322,289,298,
  310,284,271,136,135,422,240,295,296,241,291,242,292,267,335,334,332,337,
  333,336,331,462,226,326,312,265,327,331,330,329,328,225,331,321,320,319,
  318,317,316,315,314,313,311,294,293,265,309,308,
137,137,137,137,137,137,137,137,137,137,137,285,137,256,256,256,256,288,290,
  287,297,299,300,301,302,303,304,305,306,307,323,324,325,286,322,289,298,
  310,284,271,136,135,423,240,295,296,241,291,242,292,267,335,334,332,337,
  333,336,331,461,226,326,312,265,327,331,330,329,328,225,331,321,320,319,
  318,317,316,315,314,313,311,294,293,265,309,308,
137,137,137,137,137,137,137,137,137,137,137,285,137,256,256,256,256,288,290,
  287,297,299,300,301,302,303,304,305,306,307,323,324,325,286,322,289,298,
  310,284,271,136,135,424,240,295,296,241,291,242,292,267,335,334,332,337,
  333,336,331,460,226,326,312,265,327,331,330,329,328,225,331,321,320,319,
  318,317,316,315,314,313,311,294,293,265,309,308,
137,137,137,137,137,137,137,137,137,137,137,285,137,256,256,256,256,288,290,
  287,297,299,300,301,302,303,304,305,306,307,323,324,325,286,322,289,298,
  310,284,271,136,135,425,240,295,296,241,291,242,292,267,335,334,332,337,
  333,336,331,459,226,326,312,265,327,331,330,329,328,225,331,321,320,319,
  318,317,316,315,314,313,311,294,293,265,309,308,
137,137,137,137,137,137,137,137,137,137,137,285,137,256,256,256,256,288,290,
  287,297,299,300,301,302,303,304,305,306,307,323,324,325,286,322,289,298,
  310,284,271,136,135,426,240,295,296,241,291,242,292,267,335,334,332,337,
  333,336,331,458,226,326,312,265,327,331,330,329,328,225,331,321,320,319,
  318,317,316,315,314,313,311,294,293,265,309,308,
137,137,137,137,137,137,137,137,137,137,137,285,137,256,256,256,256,288,290,
  287,297,299,300,301,302,303,304,305,306,307,323,324,325,286,322,289,298,
  310,284,271,136,135,427,240,295,296,241,291,242,292,267,335,334,332,337,
  333,336,331,457,226,326,312,265,327,331,330,329,328,225,331,321,320,319,
  318,317,316,315,314,313,311,294,293,265,309,308,
137,137,137,137,137,137,137,137,137,137,137,285,137,256,256,256,256,288,290,
  287,297,299,300,301,302,303,304,305,306,307,323,324,325,286,322,289,298,
  310,284,271,136,135,428,240,295,296,241,291,242,292,267,335,334,332,337,
  333,336,331,456,226,326,312,265,327,331,330,329,328,225,331,321,320,319,
  318,317,316,315,314,313,311,294,293,265,309,308,
137,137,137,137,137,137,137,137,137,137,137,285,137,256,256,256,256,288,290,
  287,297,299,300,301,302,303,304,305,306,307,323,324,325,286,322,289,298,
  310,284,271,136,135,429,240,295,296,241,291,242,292,267,335,334,332,337,
  333,336,331,455,226,326,312,265,327,331,330,329,328,225,331,321,320,319,
  318,317,316,315,314,313,311,294,293,265,309,308,
137,137,137,137,137,137,137,137,137,137,137,285,137,256,256,256,256,288,290,
  287,297,299,300,301,302,303,304,305,306,307,323,324,325,286,322,289,298,
  310,284,271,136,135,430,240,295,296,241,291,242,292,267,335,334,332,337,
  333,336,331,454,226,326,312,265,327,331,330,329,328,225,331,321,320,319,
  318,317,316,315,314,313,311,294,293,265,309,308,
137,137,137,137,137,137,137,137,137,137,137,285,137,256,256,256,256,288,290,
  287,297,299,300,301,302,303,304,305,306,307,323,324,325,286,322,289,298,
  310,284,271,136,135,431,240,295,296,241,291,242,292,267,335,334,332,337,
  333,336,331,453,226,326,312,265,327,331,330,329,328,225,331,321,320,319,
  318,317,316,315,314,313,311,294,293,265,309,308,
137,137,137,137,137,137,137,137,137,137,137,285,137,256,256,256,256,288,290,
  287,297,299,300,301,302,303,304,305,306,307,323,324,325,286,322,289,298,
  310,284,271,136,135,432,240,295,296,241,291,242,292,267,335,334,332,337,
  333,336,331,452,226,326,312,265,327,331,330,329,328,225,331,321,320,319,
  318,317,316,315,314,313,311,294,293,265,309,308,
137,137,137,137,137,137,137,137,137,137,137,285,137,256,256,256,256,288,290,
  287,297,299,300,301,302,303,304,305,306,307,323,324,325,286,322,289,298,
  310,284,271,136,135,433,240,295,296,241,291,242,292,267,335,334,332,337,
  333,336,331,451,226,326,312,265,327,331,330,329,328,225,331,321,320,319,
  318,317,316,315,314,313,311,294,293,265,309,308,
137,137,137,137,137,137,137,137,137,137,137,285,137,256,256,256,256,288,290,
  287,297,299,300,301,302,303,304,305,306,307,323,324,325,286,322,289,298,
  310,284,271,136,135,434,240,295,296,241,291,242,292,267,335,334,332,337,
  333,336,331,450,226,326,312,265,327,331,330,329,328,225,331,321,320,319,
  318,317,316,315,314,313,311,294,293,265,309,308,
137,137,137,137,137,137,137,137,137,137,137,285,137,256,256,256,256,288,290,
  287,297,299,300,301,302,303,304,305,306,307,323,324,325,286,322,289,298,
  310,284,271,136,135,435,240,295,296,241,291,242,292,267,335,334,332,337,
  333,336,331,449,226,326,312,265,327,331,330,329,328,225,331,321,320,319,
  318,317,316,315,314,313,311,294,293,265,309,308,
137,137,137,137,137,137,137,137,137,137,137,285,137,256,256,256,256,288,290,
  287,297,299,300,301,302,303,304,305,306,307,323,324,325,286,322,289,298,
  310,284,271,136,135,436,240,295,296,241,291,242,292,267,335,334,332,337,
  333,336,331,445,226,326,312,265,327,331,330,329,328,225,331,321,320,319,
  318,317,316,315,314,313,311,294,293,265,309,308,
137,137,137,137,137,137,137,137,137,137,137,285,137,256,256,256,256,288,290,
  287,297,299,300,301,302,303,304,305,306,307,323,324,325,286,322,289,298,
  310,284,271,136,135,437,240,295,296,241,291,242,292,267,335,334,332,337,
  333,336,331,441,226,326,312,265,327,331,330,329,328,225,331,321,320,319,
  318,317,316,315,314,313,311,294,293,265,309,308,
137,137,137,137,137,137,137,137,137,137,137,285,137,256,256,256,256,288,290,
  287,297,299,300,301,302,303,304,305,306,307,323,324,325,286,322,289,298,
  310,284,271,136,135,438,240,295,296,241,291,242,292,267,335,334,332,337,
  333,336,331,440,226,326,312,265,327,331,330,329,328,225,331,321,320,319,
  318,317,316,315,314,313,311,294,293,265,309,308,
137,137,137,137,137,137,137,137,137,137,137,285,137,256,256,256,256,288,290,
  287,297,299,300,301,302,303,304,305,306,307,323,324,325,286,322,289,298,
  310,284,271,136,135,439,240,295,296,241,291,242,292,267,335,334,332,337,
  333,336,331,439,226,326,312,265,327,331,330,329,328,225,331,321,320,319,
  318,317,316,315,314,313,311,294,293,265,309,308,
137,137,137,137,137,137,137,137,137,137,137,285,137,256,256,256,256,288,290,
  287,297,299,300,301,302,303,304,305,306,307,323,324,325,286,322,289,298,
  310,284,271,136,135,440,240,295,296,241,291,242,292,267,335,334,332,337,
  333,336,331,438,226,326,312,265,327,331,330,329,328,225,331,321,320,319,
  318,317,316,315,314,313,311,294,293,265,309,308,
137,137,137,137,137,137,137,137,137,137,137,285,137,256,256,256,256,288,290,
  287,297,299,300,301,302,303,304,305,306,307,323,324,325,286,322,289,298,
  310,284,271,136,135,441,240,295,296,241,291,242,292,267,335,334,332,337,
  333,336,331,437,226,326,312,265,327,331,330,329,328,225,331,321,320,319,
  318,317,316,315,314,313,311,294,293,265,309,308,
137,137,137,137,137,137,137,137,137,137,137,285,137,256,256,256,256,288,290,
  287,297,299,300,301,302,303,304,305,306,307,323,324,325,286,322,289,298,
  310,284,271,136,135,442,240,295,296,241,291,242,292,267,335,334,332,337,
  333,336,331,436,226,326,312,265,327,331,330,329,328,225,331,321,320,319,
  318,317,316,315,314,313,311,294,293,265,309,308,
137,137,137,137,137,137,137,137,137,137,137,285,137,256,256,256,256,288,290,
  287,297,299,300,301,302,303,304,305,306,307,323,324,325,286,322,289,298,
  310,284,271,136,135,443,240,295,296,241,291,242,292,267,335,334,332,337,
  333,336,331,435,226,326,312,265,327,331,330,329,328,225,331,321,320,319,
  318,317,316,315,314,313,311,294,293,265,309,308,
137,137,137,137,137,137,137,137,137,137,137,285,137,256,256,256,256,288,290,
  287,297,299,300,301,302,303,304,305,306,307,323,324,325,286,322,289,298,
  310,284,271,136,135,444,240,295,296,241,291,242,292,267,335,334,332,337,
  333,336,331,429,226,326,312,265,327,331,330,329,328,225,331,321,320,319,
  318,317,316,315,314,313,311,294,293,265,309,308,
137,137,137,137,137,137,137,137,137,137,137,285,137,256,256,256,256,288,290,
  287,297,299,300,301,302,303,304,305,306,307,323,324,325,286,322,289,298,
  310,284,271,136,135,445,240,295,296,241,291,242,292,267,335,334,332,337,
  333,336,331,428,226,326,312,265,327,331,330,329,328,225,331,321,320,319,
  318,317,316,315,314,313,311,294,293,265,309,308,
137,137,137,137,137,137,137,137,137,137,137,285,137,256,256,256,256,288,290,
  287,297,299,300,301,302,303,304,305,306,307,323,324,325,286,322,289,298,
  310,284,271,136,135,446,240,295,296,241,291,242,292,267,335,334,332,337,
  333,336,331,427,226,326,312,265,327,331,330,329,328,225,331,321,320,319,
  318,317,316,315,314,313,311,294,293,265,309,308,
137,137,137,137,137,137,137,137,137,137,137,285,137,256,256,256,256,288,290,
  287,297,299,300,301,302,303,304,305,306,307,323,324,325,286,322,289,298,
  310,284,271,136,135,447,240,295,296,241,291,242,292,267,335,334,332,337,
  333,336,331,426,226,326,312,265,327,331,330,329,328,225,331,321,320,319,
  318,317,316,315,314,313,311,294,293,265,309,308,
137,137,137,137,137,137,137,137,137,137,137,285,137,256,256,256,256,288,290,
  287,297,299,300,301,302,303,304,305,306,307,323,324,325,286,322,289,298,
  310,284,271,136,135,448,240,295,296,241,291,242,292,267,335,334,332,337,
  333,336,331,420,226,326,312,265,327,331,330,329,328,225,331,321,320,319,
  318,317,316,315,314,313,311,294,293,265,309,308,
137,137,137,137,137,137,137,137,137,137,137,285,137,256,256,256,256,288,290,
  287,297,299,300,301,302,303,304,305,306,307,323,324,325,286,322,289,298,
  310,284,271,136,135,449,240,295,296,241,291,242,292,267,335,334,332,337,
  333,336,331,419,226,326,312,265,327,331,330,329,328,225,331,321,320,319,
  318,317,316,315,314,313,311,294,293,265,309,308,
137,137,137,137,137,137,137,137,137,137,137,285,137,256,256,256,256,288,290,
  287,297,299,300,301,302,303,304,305,306,307,323,324,325,286,322,289,298,
  310,284,271,136,135,450,240,295,296,241,291,242,292,267,335,334,332,337,
  333,336,331,418,226,326,312,265,327,331,330,329,328,225,331,321,320,319,
  318,317,316,315,314,313,311,294,293,265,309,308,
137,137,137,137,137,137,137,137,137,137,137,285,137,256,256,256,256,288,290,
  287,297,299,300,301,302,303,304,305,306,307,323,324,325,286,322,289,298,
  310,284,271,136,135,451,240,295,296,241,291,242,292,267,335,334,332,337,
  333,336,331,417,226,326,312,265,327,331,330,329,328,225,331,321,320,319,
  318,317,316,315,314,313,311,294,293,265,309,308,
137,137,137,137,137,137,137,137,137,137,137,285,137,256,256,256,256,288,290,
  287,297,299,300,301,302,303,304,305,306,307,323,324,325,286,322,289,298,
  310,284,271,136,135,452,240,295,296,241,291,242,292,267,335,334,332,337,
  333,336,331,416,226,326,312,265,327,331,330,329,328,225,331,321,320,319,
  318,317,316,315,314,313,311,294,293,265,309,308,
137,137,137,137,137,137,137,137,137,137,137,285,137,256,256,256,256,288,290,
  287,297,299,300,301,302,303,304,305,306,307,323,324,325,286,322,289,298,
  310,284,271,136,135,453,240,295,296,241,291,242,292,267,335,334,332,337,
  333,336,331,415,226,326,312,265,327,331,330,329,328,225,331,321,320,319,
  318,317,316,315,314,313,311,294,293,265,309,308,
137,137,137,137,137,137,137,137,137,137,137,285,137,256,256,256,256,288,290,
  287,297,299,300,301,302,303,304,305,306,307,323,324,325,286,322,289,298,
  310,284,271,136,135,454,240,295,296,241,291,242,292,267,335,334,332,337,
  333,336,331,414,226,326,312,265,327,331,330,329,328,225,331,321,320,319,
  318,317,316,315,314,313,311,294,293,265,309,308,
137,137,137,137,137,137,137,137,137,137,137,285,137,256,256,256,256,288,290,
  287,297,299,300,301,302,303,304,305,306,307,323,324,325,286,322,289,298,
  310,284,271,136,135,455,240,295,296,241,291,242,292,267,335,334,332,337,
  333,336,331,413,226,326,312,265,327,331,330,329,328,225,331,321,320,319,
  318,317,316,315,314,313,311,294,293,265,309,308,
137,137,137,137,137,137,137,137,137,137,137,285,137,256,256,256,256,288,290,
  287,297,299,300,301,302,303,304,305,306,307,323,324,325,286,322,289,298,
  310,284,271,136,135,456,240,295,296,241,291,242,292,267,335,334,332,337,
  333,336,331,412,226,326,312,265,327,331,330,329,328,225,331,321,320,319,
  318,317,316,315,314,313,311,294,293,265,309,308,
137,137,137,137,137,137,137,137,137,137,137,285,137,256,256,256,256,288,290,
  287,297,299,300,301,302,303,304,305,306,307,323,324,325,286,322,289,298,
  310,284,271,136,135,457,240,295,296,241,291,242,292,267,335,334,332,337,
  333,336,331,411,226,326,312,265,327,331,330,329,328,225,331,321,320,319,
  318,317,316,315,314,313,311,294,293,265,309,308,
137,137,137,137,137,137,137,137,137,137,137,285,137,256,256,256,256,288,290,
  287,297,299,300,301,302,303,304,305,306,307,323,324,325,286,322,289,298,
  310,284,271,136,135,458,240,295,296,241,291,242,292,267,335,334,332,337,
  333,336,331,410,226,326,312,265,327,331,330,329,328,225,331,321,320,319,
  318,317,316,315,314,313,311,294,293,265,309,308,
137,137,137,137,137,137,137,137,137,137,137,285,137,256,256,256,256,288,290,
  287,297,299,300,301,302,303,304,305,306,307,323,324,325,286,322,289,298,
  310,284,271,136,135,459,240,295,296,241,291,242,292,267,335,334,332,337,
  333,336,331,409,226,326,312,265,327,331,330,329,328,225,331,321,320,319,
  318,317,316,315,314,313,311,294,293,265,309,308,
137,137,137,137,137,137,137,137,137,137,137,285,137,256,256,256,256,288,290,
  287,297,299,300,301,302,303,304,305,306,307,323,324,325,286,322,289,298,
  310,284,271,136,135,460,240,295,296,241,291,242,292,267,335,334,332,337,
  333,336,331,408,226,326,312,265,327,331,330,329,328,225,331,321,320,319,
  318,317,316,315,314,313,311,294,293,265,309,308,
137,137,137,137,137,137,137,137,137,137,137,285,137,256,256,256,256,288,290,
  287,297,299,300,301,302,303,304,305,306,307,323,324,325,286,322,289,298,
  310,284,271,136,135,461,240,295,296,241,291,242,292,267,335,334,332,337,
  333,336,331,407,226,326,312,265,327,331,330,329,328,225,331,321,320,319,
  318,317,316,315,314,313,311,294,293,265,309,308,
137,137,137,137,137,137,137,137,137,137,137,285,137,256,256,256,256,288,290,
  287,297,299,300,301,302,303,304,305,306,307,323,324,325,286,322,289,298,
  310,284,271,136,135,462,240,295,296,241,291,242,292,267,335,334,332,337,
  333,336,331,406,226,326,312,265,327,331,330,329,328,225,331,321,320,319,
  318,317,316,315,314,313,311,294,293,265,309,308,
304,633,634,306,305,306,305,304,463,308,304,305,306,382,307,
304,633,634,306,305,306,305,304,464,308,304,305,306,381,307,
304,633,634,306,305,306,305,304,465,308,304,305,306,380,307,
304,633,634,306,305,306,305,304,466,308,304,305,306,379,307,
309,310,310,309,467,309,310,378,
309,310,310,309,468,309,310,377,
296,303,302,299,302,299,296,469,398,296,299,302,629,
296,303,302,299,302,299,296,470,397,296,299,302,629,
296,303,302,299,302,299,296,471,396,296,299,302,629,
311,625,626,627,313,312,313,312,311,472,311,312,313,316,395,314,315,
311,625,626,627,313,312,313,312,311,473,311,312,313,316,394,314,315,
293,292,292,289,288,287,286,285,284,474,393,292,628,
293,292,292,289,288,287,286,285,284,475,392,292,628,
293,292,292,289,288,287,286,285,284,476,391,292,628,
293,292,292,289,288,287,286,285,284,477,390,292,628,
293,292,292,289,288,287,286,285,284,478,635,292,628,
293,292,292,289,288,287,286,285,284,479,388,292,628,
293,292,292,289,288,287,286,285,284,480,387,292,628,
293,292,292,289,288,287,286,285,284,481,386,292,628,
293,292,292,289,288,287,286,285,284,482,385,292,628,
293,292,292,289,288,287,286,285,284,483,384,292,628,
293,292,292,289,288,287,286,285,284,484,383,292,628,
137,137,137,137,137,137,137,137,137,137,137,285,137,256,256,256,256,288,636,
  287,146,297,299,300,301,302,303,304,305,306,307,323,324,325,286,322,289,
  298,310,284,271,136,135,485,240,295,296,241,291,125,278,242,292,637,126,
  267,335,334,332,337,333,336,331,127,226,326,639,312,265,327,331,330,329,
  328,225,331,321,320,319,318,317,316,315,314,313,311,294,293,279,265,638,
  309,308,
285,256,256,256,256,288,290,640,286,563,289,271,19,486,124,295,641,242,292,
  294,293,309,
20,20,20,20,20,20,20,20,20,20,20,20,20,20,20,20,20,20,20,20,20,20,20,20,20,
  20,20,20,20,20,20,20,20,20,20,20,20,20,20,20,20,20,20,20,20,20,20,20,20,
  20,20,20,20,20,20,20,20,20,20,20,20,20,20,20,20,20,20,20,20,20,20,20,20,
  20,20,20,20,20,20,20,20,20,20,20,20,20,20,20,20,20,20,20,20,20,20,20,20,
  20,20,20,20,20,20,20,20,20,20,20,20,20,20,20,20,20,20,20,20,20,20,20,20,
  20,20,20,20,20,20,20,20,20,20,20,20,20,487,1,999,
642,642,642,642,642,642,642,642,642,642,642,642,642,642,642,642,642,642,642,
  642,642,642,642,642,642,642,642,642,642,642,642,642,642,642,642,642,642,
  642,642,642,642,642,642,642,642,642,642,642,642,642,642,642,642,642,642,
  642,642,642,642,642,642,642,642,642,642,642,642,642,642,642,642,642,642,
  642,642,642,642,642,642,642,642,642,642,642,642,642,642,642,642,642,642,
  642,642,642,642,642,642,642,642,642,642,642,642,642,642,642,642,642,642,
  642,642,642,642,642,642,642,642,642,642,642,642,642,642,642,642,642,642,
  642,642,642,642,642,642,1,488,1,642,
20,20,20,20,20,20,20,20,20,20,20,20,20,20,20,1,489,1,945,
137,137,137,137,137,137,137,137,137,137,137,137,137,136,135,490,120,265,265,
20,20,20,20,20,20,20,20,20,20,20,20,20,20,20,20,20,20,20,1,491,1,944,
140,140,140,140,140,140,140,140,140,140,140,140,140,140,140,140,140,140,140,
  492,119,613,
20,20,20,20,20,20,493,1,943,
20,20,20,20,20,20,494,1,942,
20,20,1,495,1,941,
643,146,496,116,278,637,115,279,638,
20,20,20,20,20,20,497,1,939,
20,20,20,20,20,20,498,1,938,
20,20,20,20,20,20,20,20,20,20,20,20,20,20,20,20,20,20,20,20,20,499,1,937,
137,137,137,137,137,137,137,137,137,137,137,137,137,108,108,108,108,136,135,
  108,108,500,109,265,265,
146,501,644,278,279,
146,502,106,278,279,
20,20,20,20,20,20,20,20,20,20,20,20,20,20,20,20,20,20,20,20,20,20,20,20,20,
  20,20,20,20,20,20,20,20,20,20,20,20,20,20,20,20,20,20,20,503,1,936,
645,645,645,645,645,645,645,645,645,645,645,645,645,645,645,645,645,645,645,
  645,645,645,645,645,645,645,645,645,645,645,645,645,645,645,645,645,645,
  645,645,645,645,645,645,1,504,1,645,
137,137,137,137,137,137,137,137,137,137,137,285,137,256,256,256,256,288,290,
  287,146,297,299,300,301,302,303,304,305,306,307,323,324,325,286,322,289,
  298,310,284,271,136,135,1,505,240,295,296,241,291,102,278,242,292,267,
  335,334,332,337,333,336,331,1,646,101,226,326,312,265,327,331,330,329,
  328,225,331,321,320,319,318,317,316,315,314,313,311,294,293,279,265,309,
  308,
20,20,20,20,20,20,20,20,20,20,20,20,20,20,20,20,20,20,20,20,20,20,20,20,20,
  20,20,20,20,20,20,20,20,20,20,20,20,20,20,20,20,20,1,506,1,935,
137,137,137,137,137,137,137,137,137,137,137,285,137,256,256,256,256,288,290,
  287,297,299,300,301,302,303,304,305,306,307,323,324,325,286,322,289,298,
  310,284,271,136,135,507,240,295,296,241,291,242,292,267,335,334,332,337,
  333,336,331,100,226,326,312,265,327,331,330,329,328,225,331,321,320,319,
  318,317,316,315,314,313,311,294,293,265,309,308,
20,20,20,20,20,20,20,20,20,20,20,20,20,20,20,1,508,1,934,
137,137,137,137,137,137,137,137,137,137,137,137,137,136,135,509,99,265,265,
20,20,20,20,20,20,20,20,20,20,20,20,20,20,20,1,510,1,933,
137,137,137,137,137,137,137,137,137,137,137,137,137,136,135,511,98,265,265,
20,20,20,20,20,20,20,20,20,20,20,20,20,20,20,1,512,1,932,
137,137,137,137,137,137,137,137,137,137,137,137,137,136,135,513,131,647,265,
  265,
20,20,20,20,20,20,20,20,20,20,20,20,20,20,20,1,514,1,931,
137,137,137,137,137,137,137,137,137,137,137,137,137,136,135,515,131,648,265,
  265,
20,20,20,20,20,20,20,20,20,20,20,20,20,20,20,1,516,1,930,
137,137,137,137,137,137,137,137,137,137,137,137,137,136,135,517,131,649,265,
  265,
20,20,20,20,20,20,20,20,20,20,20,20,20,20,20,20,20,20,20,20,20,20,20,20,20,
  20,20,20,20,20,20,20,20,20,20,20,20,20,20,20,20,20,20,1,518,1,929,
20,20,20,20,20,20,20,20,20,20,20,20,20,20,20,20,20,20,20,20,20,20,20,20,20,
  20,20,20,20,20,20,20,20,20,20,20,20,20,20,20,20,20,20,1,519,1,928,
137,137,137,137,137,137,137,137,137,137,137,285,137,256,256,256,256,288,636,
  287,146,297,299,300,301,302,303,304,305,306,307,323,324,325,286,322,289,
  298,310,284,271,136,135,520,240,295,296,241,291,125,278,242,292,637,126,
  267,335,334,332,337,333,336,331,127,226,326,650,312,265,327,331,330,329,
  328,225,331,321,320,319,318,317,316,315,314,313,311,294,293,279,265,638,
  309,308,
20,20,20,20,20,20,20,20,20,20,20,20,20,20,20,20,20,20,20,20,20,20,20,20,20,
  20,20,20,20,20,20,20,20,20,20,20,20,20,20,20,20,20,20,1,521,1,927,
20,20,20,20,20,20,20,20,20,20,20,20,20,20,20,20,20,20,20,20,20,20,20,20,20,
  20,20,20,20,20,20,20,20,20,20,20,20,20,20,20,20,20,20,1,522,1,926,
20,20,20,20,20,20,20,20,20,20,20,20,20,20,20,20,20,20,20,20,20,20,20,20,20,
  20,20,20,20,20,20,20,20,20,20,20,20,20,20,20,20,20,20,1,523,1,925,
137,137,137,137,137,137,137,137,137,137,137,285,137,256,256,256,256,288,636,
  287,146,297,299,300,301,302,303,304,305,306,307,323,324,325,286,322,289,
  298,310,284,271,136,135,524,240,295,296,241,291,125,278,242,292,637,126,
  267,335,334,332,337,333,336,331,127,226,326,651,312,265,327,331,330,329,
  328,225,331,321,320,319,318,317,316,315,314,313,311,294,293,279,265,638,
  309,308,
20,20,20,20,20,20,20,20,20,20,20,20,20,20,20,20,20,20,20,20,20,20,20,20,20,
  20,20,20,20,20,20,20,20,20,20,20,20,20,20,20,20,20,1,525,1,924,
20,20,20,20,20,20,20,20,20,20,20,20,20,20,20,20,20,20,20,20,20,20,20,20,20,
  20,20,20,20,20,20,20,20,20,20,20,20,20,20,20,20,20,1,526,1,923,
137,137,137,137,137,137,137,137,137,137,137,285,137,256,256,256,256,288,290,
  287,297,299,300,301,302,303,304,305,306,307,323,324,325,286,322,289,298,
  310,284,271,136,135,527,240,295,296,241,291,242,292,267,335,334,332,337,
  333,336,331,87,226,326,312,265,327,331,330,329,328,225,331,321,320,319,
  318,317,316,315,314,313,311,294,293,265,309,308,
20,20,20,20,20,20,528,1,922,
20,20,20,20,20,20,20,20,20,20,20,20,20,20,20,20,20,20,20,20,20,20,20,20,20,
  20,20,20,20,20,20,20,20,20,20,20,20,20,20,20,20,20,1,529,1,921,
20,20,20,20,20,20,20,20,20,20,20,20,20,20,20,20,20,20,20,20,20,20,20,20,20,
  20,20,20,20,20,20,20,20,20,20,20,20,20,20,20,20,20,1,530,1,920,
20,20,20,20,20,20,20,20,20,20,20,20,20,20,20,20,20,20,20,20,20,20,20,20,20,
  20,20,20,20,20,20,20,20,20,20,20,20,20,20,20,20,20,1,531,1,919,
137,137,137,137,137,137,137,137,137,137,137,285,137,256,256,256,256,288,290,
  287,297,299,300,301,302,303,304,305,306,307,323,324,325,286,322,289,298,
  310,284,271,136,135,532,240,295,296,241,291,242,292,267,335,334,332,337,
  333,336,331,83,226,326,312,265,327,331,330,329,328,225,331,321,320,319,
  318,317,316,315,314,313,311,294,293,265,309,308,
137,137,137,137,137,137,137,137,137,137,137,137,137,136,135,533,78,265,265,
20,20,20,20,20,20,20,20,534,1,918,
20,20,20,20,20,20,20,20,535,1,917,
43,44,45,40,22,536,22,22,48,22,47,46,46,
359,361,364,365,366,367,368,369,370,371,372,373,374,375,376,377,378,379,380,
  381,382,389,392,393,394,323,325,391,383,387,384,386,390,362,360,310,388,
  385,41,44,537,67,67,67,652,67,67,67,67,67,67,67,67,67,67,67,67,67,67,67,
  67,67,67,67,67,67,67,67,67,67,67,67,67,67,67,67,67,67,67,67,67,67,
43,44,45,40,63,538,63,63,48,63,47,46,46,
385,539,653,
654,654,654,654,654,1,540,1,654,
655,655,655,655,655,1,541,1,655,
656,656,656,656,656,1,542,1,656,
150,657,152,153,151,157,156,658,658,659,657,155,148,543,657,
43,44,45,40,58,544,58,58,48,58,47,46,46,
43,44,45,40,55,545,55,55,48,55,47,46,46,
260,260,260,260,260,260,260,260,260,546,
273,275,276,274,277,279,547,
272,547,272,272,272,272,272,272,272,272,272,272,272,272,272,272,272,272,272,
  272,272,269,272,272,272,272,272,272,272,272,272,272,272,272,272,272,272,
  272,272,272,272,278,272,548,660,
263,263,263,263,263,263,263,263,263,263,253,253,253,253,253,253,253,253,253,
  253,253,253,253,253,253,253,253,253,253,253,253,253,253,253,253,253,253,
  253,253,253,253,253,253,253,253,253,253,253,253,549,
263,263,263,263,263,263,263,263,263,263,249,249,249,249,249,249,249,249,249,
  249,249,249,249,249,249,249,249,249,249,249,249,249,249,249,249,249,249,
  249,249,249,249,249,249,249,249,249,249,249,249,550,
310,551,661,
310,552,662,
310,553,663,
310,554,664,
310,555,665,
310,556,666,
310,557,667,
310,558,668,
310,559,669,
310,560,670,
310,561,671,
137,137,137,137,137,137,137,137,137,137,137,285,137,256,256,256,256,288,290,
  287,297,299,300,301,302,303,304,305,306,307,323,324,325,286,322,289,298,
  310,284,271,136,135,562,240,295,296,241,291,242,292,267,335,334,332,337,
  333,336,331,672,226,326,312,265,327,331,330,329,328,225,331,321,320,319,
  318,317,316,315,314,313,311,294,293,265,309,308,
255,255,255,255,255,563,
20,20,20,20,20,20,20,20,20,20,20,20,20,20,20,20,20,20,20,20,20,20,20,20,20,
  20,20,20,20,20,20,20,20,20,20,20,20,20,20,20,564,1,978,
673,673,673,673,673,673,673,673,673,673,673,673,673,673,673,673,673,673,673,
  673,673,673,673,673,673,673,673,673,673,673,673,673,673,673,673,673,673,
  673,673,1,565,1,673,
20,20,20,20,20,20,20,20,20,20,20,20,20,20,20,20,20,20,20,20,20,20,20,20,20,
  20,20,20,20,20,20,20,20,20,20,20,20,20,20,20,566,1,977,
674,674,674,674,674,674,674,674,674,674,674,674,674,674,674,674,674,674,674,
  674,674,674,674,674,674,674,674,674,674,674,674,674,674,674,674,674,674,
  674,674,1,567,1,674,
675,675,675,675,675,675,675,675,675,675,675,675,675,675,675,675,675,675,675,
  675,675,675,675,675,675,675,675,675,675,675,675,675,675,675,675,675,675,
  675,675,1,568,1,675,
676,676,676,676,676,676,676,676,676,676,676,676,676,676,676,676,676,676,676,
  676,676,676,676,676,676,676,676,676,676,676,676,676,676,676,676,676,676,
  676,676,1,569,1,676,
677,677,677,677,677,677,677,677,677,677,677,677,677,677,677,677,677,677,677,
  677,677,677,677,677,677,677,677,677,677,677,677,677,677,677,677,677,677,
  677,677,1,570,1,677,
678,678,678,678,678,678,678,678,678,678,678,678,678,678,678,678,678,678,678,
  678,678,678,678,678,678,678,678,678,678,678,678,678,678,678,678,678,678,
  678,678,678,678,678,1,571,1,678,
679,679,679,679,679,679,679,679,679,679,679,679,679,679,679,679,679,679,679,
  679,679,679,679,679,679,679,679,679,679,679,679,679,679,679,679,679,679,
  679,679,679,679,679,1,572,1,679,
20,20,20,20,20,20,20,20,20,20,20,20,20,20,20,20,20,20,20,20,20,20,20,20,20,
  20,20,20,20,20,20,20,20,20,20,20,20,20,20,20,20,20,20,573,1,971,
680,680,680,680,680,680,680,680,680,680,680,680,680,680,680,680,680,680,680,
  680,680,680,680,680,680,680,680,680,680,680,680,680,680,680,680,680,680,
  680,680,680,680,680,1,574,1,680,
20,20,20,20,20,20,20,20,20,20,20,20,20,20,20,20,20,20,20,20,20,20,20,20,20,
  20,20,20,20,20,20,20,20,20,20,20,20,20,20,20,20,20,20,575,1,970,
681,681,681,681,681,681,681,681,681,681,681,681,681,681,681,681,681,681,681,
  681,681,681,681,681,681,681,681,681,681,681,681,681,681,681,681,681,681,
  681,681,681,681,681,1,576,1,681,
20,20,20,20,20,20,20,20,20,20,20,20,20,20,20,20,20,20,20,20,20,20,20,20,20,
  20,20,20,20,20,20,20,20,20,20,20,20,20,20,20,20,20,20,577,1,969,
682,682,682,682,682,682,682,682,682,682,682,682,682,682,682,682,682,682,682,
  682,682,682,682,682,682,682,682,682,682,682,682,682,682,682,682,682,682,
  682,682,682,682,682,1,578,1,682,
20,20,20,20,20,20,20,20,20,20,20,20,20,20,20,20,20,20,20,20,20,20,20,20,20,
  20,20,20,20,20,20,20,20,20,20,20,20,20,20,20,20,20,20,579,1,968,
683,683,683,683,683,683,683,683,683,683,683,683,683,683,683,683,683,683,683,
  683,683,683,683,683,683,683,683,683,683,683,683,683,683,683,683,683,683,
  683,683,683,683,683,1,580,1,683,
20,20,20,20,20,20,20,20,20,20,20,20,20,20,20,20,20,20,20,20,20,20,20,20,20,
  20,20,20,20,20,20,20,20,20,20,20,20,20,20,20,20,20,20,581,1,967,
684,684,684,684,684,684,684,684,684,684,684,684,684,684,684,684,684,684,684,
  684,684,684,684,684,684,684,684,684,684,684,684,684,684,684,684,684,684,
  684,684,684,684,684,1,582,1,684,
685,685,685,685,685,685,685,685,685,685,685,685,685,685,685,685,685,685,685,
  685,685,685,685,685,685,685,685,685,685,685,685,685,685,685,685,685,685,
  685,685,685,685,685,1,583,1,685,
20,20,20,20,20,20,20,20,20,20,20,20,20,20,20,20,20,20,20,20,20,20,20,20,20,
  20,20,20,20,20,20,20,20,20,20,20,20,20,20,20,20,20,20,584,1,965,
686,686,686,686,686,686,686,686,686,686,686,686,686,686,686,686,686,686,686,
  686,686,686,686,686,686,686,686,686,686,686,686,686,686,686,686,686,686,
  686,686,686,686,686,1,585,1,686,
687,687,687,687,687,687,687,687,687,687,687,687,687,687,687,687,687,687,687,
  687,687,687,687,687,687,687,687,687,687,687,687,687,687,687,687,687,687,
  687,687,687,687,687,1,586,1,687,
20,20,20,20,20,20,20,20,20,20,20,20,20,20,20,20,20,20,20,20,20,20,20,20,20,
  20,20,20,20,20,20,20,20,20,20,20,20,20,20,20,20,20,20,587,1,963,
688,688,688,688,688,688,688,688,688,688,688,688,688,688,688,688,688,688,688,
  688,688,688,688,688,688,688,688,688,688,688,688,688,688,688,688,688,688,
  688,688,688,688,688,1,588,1,688,
689,689,689,689,689,689,689,689,689,689,689,689,689,689,689,689,689,689,689,
  689,689,689,689,689,689,689,689,689,689,689,689,689,689,689,689,689,689,
  689,689,689,689,689,1,589,1,689,
20,20,20,20,20,20,20,20,20,20,20,20,20,20,20,20,20,20,20,20,20,20,20,20,20,
  20,20,20,20,20,20,20,20,20,20,20,20,20,20,20,20,20,1,590,1,961,
20,20,20,20,20,20,20,20,20,20,20,20,20,20,20,20,20,20,20,20,20,20,20,20,20,
  20,20,20,20,20,20,20,20,20,20,20,20,20,20,20,20,20,1,591,1,960,
20,20,20,20,20,20,20,20,20,20,20,20,20,20,20,20,20,20,20,20,20,20,20,20,20,
  20,20,20,20,20,20,20,20,20,20,20,20,20,20,20,20,20,1,592,1,959,
20,20,20,20,20,20,20,20,20,20,20,20,20,20,20,20,20,20,20,20,20,20,20,20,20,
  20,20,20,20,20,20,20,20,20,20,20,20,20,20,20,20,20,1,593,1,958,
20,20,20,20,20,20,20,20,20,20,20,20,20,20,20,20,20,20,20,20,20,20,20,20,20,
  20,20,20,20,20,20,20,20,20,20,20,20,20,20,20,20,20,1,594,1,957,
20,20,20,20,20,20,20,20,20,20,20,20,20,20,20,20,20,20,20,20,20,20,20,20,20,
  20,20,20,20,20,20,20,20,20,20,20,20,20,20,20,20,20,1,595,1,956,
20,20,20,20,20,20,20,20,20,20,20,20,20,20,20,20,20,20,20,20,20,20,20,20,20,
  20,20,20,20,20,20,20,20,20,20,20,20,20,20,20,20,20,1,596,1,955,
20,20,20,20,20,20,20,20,20,20,20,20,20,20,20,20,20,20,20,20,20,20,20,20,20,
  20,20,20,20,20,20,20,20,20,20,20,20,20,20,20,20,20,1,597,1,954,
20,20,20,20,20,20,20,20,20,20,20,20,20,20,20,20,20,20,20,20,20,20,20,20,20,
  20,20,20,20,20,20,20,20,20,20,20,20,20,20,20,20,20,1,598,1,953,
20,20,20,20,20,20,20,20,20,20,20,20,20,20,20,20,20,20,20,20,20,20,20,20,20,
  20,20,20,20,20,20,20,20,20,20,20,20,20,20,20,20,20,1,599,1,952,
20,20,20,20,20,20,20,20,20,20,20,20,20,20,20,20,20,20,20,20,20,20,20,20,20,
  20,20,20,20,20,20,20,20,20,20,20,20,20,20,20,20,20,1,600,1,951,
20,20,20,20,20,20,20,20,20,20,20,20,20,20,20,20,20,20,20,20,20,20,20,20,20,
  20,20,20,20,20,20,20,20,20,20,20,20,20,20,20,20,20,1,601,1,950,
43,44,45,40,54,602,54,54,48,54,47,46,46,
43,44,45,40,53,603,53,53,48,53,47,46,46,
43,44,45,40,52,604,52,52,48,52,47,46,46,
43,44,45,40,49,605,49,49,48,49,47,46,46,
43,44,45,40,48,606,48,48,48,48,47,46,46,
137,137,137,137,137,137,137,137,137,137,137,137,137,136,135,607,690,265,265,
43,44,45,40,46,608,46,46,48,46,47,46,46,
43,44,45,40,45,609,45,45,48,45,47,46,46,
166,166,166,166,166,166,166,166,166,166,166,166,166,166,166,166,166,166,166,
  166,166,166,166,166,166,166,166,166,166,166,166,166,166,166,166,166,166,
  166,166,166,166,167,166,610,
43,44,45,40,44,611,44,44,48,44,47,46,46,
43,44,45,40,43,612,43,43,48,43,47,46,46,
141,141,141,141,141,141,141,141,141,141,141,141,141,141,141,141,141,141,141,
  20,20,20,20,613,1,892,
691,691,691,691,691,1,614,1,691,
692,692,692,692,692,1,615,1,692,
693,693,693,693,693,1,616,1,693,
694,694,694,694,694,1,617,1,694,
20,20,20,20,20,20,20,20,20,20,20,20,20,20,20,20,20,20,20,20,20,20,20,20,20,
  20,20,20,20,20,20,20,20,20,20,20,20,20,20,20,20,20,1,618,1,886,
137,137,137,137,137,137,137,137,137,137,137,285,137,256,256,256,256,288,290,
  287,297,299,300,301,302,303,304,305,306,307,323,324,325,286,322,289,298,
  310,284,271,136,135,619,240,295,296,241,291,242,292,267,335,334,332,337,
  333,336,331,695,226,326,312,265,327,331,330,329,328,225,331,321,320,319,
  318,317,316,315,314,313,311,294,293,265,309,308,
20,20,20,20,20,20,20,20,20,20,20,20,20,20,20,20,20,20,20,20,20,20,20,20,20,
  20,20,20,20,20,20,20,20,20,20,20,20,20,20,20,20,20,1,620,1,885,
137,137,137,137,137,137,137,137,137,137,137,285,137,256,256,256,256,288,290,
  287,297,299,300,301,302,303,304,305,306,307,323,324,325,286,322,289,298,
  310,284,271,136,135,621,240,295,296,241,291,242,292,267,335,334,332,337,
  333,336,331,696,226,326,312,265,327,331,330,329,328,225,331,321,320,319,
  318,317,316,315,314,313,311,294,293,265,309,308,
43,44,45,40,23,622,23,23,48,23,47,46,46,
20,20,20,20,20,20,623,1,949,
697,697,697,697,697,1,624,1,697,
20,20,20,20,20,20,20,20,625,1,998,
20,20,20,20,20,20,20,20,626,1,997,
20,20,20,20,20,20,20,20,627,1,996,
20,20,20,20,20,20,20,20,20,628,1,1049,
20,20,20,20,20,20,20,20,629,1,1063,
698,698,698,698,698,698,698,698,698,698,698,698,698,698,698,698,698,698,698,
  698,698,698,698,698,698,698,698,698,698,698,698,698,698,698,698,698,698,
  698,698,698,698,698,1,630,1,698,
699,699,699,699,699,699,699,699,699,699,699,699,699,699,699,699,699,699,699,
  699,699,699,699,699,699,699,699,699,699,699,699,699,699,699,699,699,699,
  699,699,699,699,699,1,631,1,699,
700,700,700,700,700,700,700,700,700,700,700,700,700,700,700,700,700,700,700,
  700,700,700,700,700,700,700,700,700,700,700,700,700,700,700,700,700,700,
  700,700,700,700,700,1,632,1,700,
20,20,20,20,20,20,20,633,1,995,
20,20,20,20,20,20,20,634,1,994,
701,1,635,1,701,
272,547,272,272,272,272,272,272,272,272,272,272,272,272,272,272,272,272,272,
  272,272,272,272,272,272,272,272,272,272,272,272,272,272,272,272,272,272,
  272,272,272,278,272,636,702,
272,547,272,272,272,272,272,272,272,272,272,272,272,272,272,272,272,272,272,
  272,272,142,272,272,272,272,272,272,272,272,272,272,272,272,272,272,272,
  272,272,272,272,278,272,637,144,
20,20,20,20,20,20,20,20,638,1,940,
703,388,639,65,704,
261,261,261,261,261,261,261,261,261,640,
258,248,263,549,550,263,257,257,257,257,257,259,243,243,243,243,252,252,243,
  243,641,252,
117,118,119,120,121,122,123,124,125,126,127,128,129,130,131,132,133,134,135,
  136,137,138,139,140,68,69,70,71,72,73,141,142,143,144,145,146,74,75,76,
  77,78,147,148,149,79,80,81,82,83,150,151,152,153,154,155,156,157,158,
  159,160,161,162,163,164,113,65,66,67,171,172,173,174,175,176,177,178,
  179,180,181,182,183,184,185,186,165,166,167,168,169,170,84,85,86,87,88,
  89,51,52,53,90,91,92,93,94,95,96,97,98,54,55,99,100,101,102,103,104,105,
  106,56,57,58,107,108,59,60,109,110,111,112,61,62,63,64,642,705,706,202,
  706,706,706,706,706,706,706,706,706,706,333,333,333,333,333,334,335,336,
  337,340,340,340,341,342,346,346,346,346,347,348,349,350,351,352,353,354,
  357,357,357,358,359,360,361,362,363,364,365,366,370,370,370,370,371,372,
  373,374,375,376,248,247,246,245,244,243,264,263,262,261,260,259,258,257,
  256,255,254,253,252,251,250,249,190,116,189,115,188,114,187,242,241,240,
  239,238,237,236,235,234,233,232,231,230,229,228,227,227,227,227,227,227,
  226,225,224,223,223,223,223,223,223,222,221,220,219,218,217,216,216,216,
  216,215,215,215,215,214,213,212,211,210,209,208,207,206,205,204,203,201,
  200,199,198,197,196,195,194,193,192,191,
272,547,272,272,272,272,272,272,272,272,272,272,272,272,272,272,272,272,272,
  272,272,272,272,272,272,272,272,272,272,272,272,272,272,272,272,272,272,
  272,272,272,278,272,643,707,
385,644,708,
137,137,137,137,137,137,137,137,137,137,137,285,137,256,256,256,256,288,636,
  287,146,297,299,300,301,302,303,304,305,306,307,323,324,325,286,322,289,
  298,310,284,271,136,135,645,240,295,296,241,291,125,278,242,292,637,126,
  267,335,334,332,337,333,336,331,127,226,326,709,312,265,327,331,330,329,
  328,225,331,321,320,319,318,317,316,315,314,313,311,294,293,279,265,638,
  309,308,
137,137,137,137,137,137,137,137,137,137,137,285,137,256,256,256,256,288,290,
  287,146,297,299,300,301,302,303,304,305,306,307,323,324,325,286,322,289,
  298,310,284,271,136,135,646,240,295,296,241,291,104,278,242,292,267,335,
  334,332,337,333,336,331,105,226,326,312,265,327,331,330,329,328,225,331,
  321,320,319,318,317,316,315,314,313,311,294,293,279,265,309,308,
703,97,97,97,97,97,97,647,710,
703,96,96,96,96,96,96,648,710,
703,95,95,95,95,95,95,649,710,
703,94,94,94,94,94,94,650,704,
703,91,91,91,91,91,91,651,704,
359,361,364,365,366,367,368,369,370,371,372,373,374,375,376,377,378,379,380,
  381,382,389,392,393,394,323,325,391,383,387,384,386,390,362,360,711,310,
  388,385,41,44,713,713,712,652,68,1,713,68,68,68,68,69,68,68,68,68,68,68,
  68,68,68,68,68,68,68,68,68,68,68,68,68,68,68,68,68,68,68,68,68,68,68,68,
  68,68,68,68,68,
714,714,714,714,714,714,714,714,714,714,714,714,714,714,714,714,714,714,714,
  714,714,714,714,714,714,714,714,714,714,714,714,714,714,714,714,714,714,
  714,714,714,714,714,1,653,1,714,
43,44,45,40,61,654,61,61,48,61,47,46,46,
43,44,45,40,60,655,60,60,48,60,47,46,46,
43,44,45,40,59,656,59,59,48,59,47,46,46,
715,715,715,715,715,715,715,715,715,657,
716,716,716,716,716,658,
154,154,154,154,154,154,154,154,154,154,154,154,154,154,154,154,716,716,716,
  716,716,154,154,154,154,154,154,154,154,154,154,154,154,154,154,154,154,
  154,154,154,154,154,659,
270,660,
717,717,717,717,717,717,717,717,717,717,717,717,717,717,717,1,661,1,717,
137,137,137,137,137,137,137,137,137,137,137,285,137,256,256,256,256,288,290,
  287,297,299,300,301,302,303,304,305,306,307,323,324,325,286,322,289,298,
  310,284,271,136,135,662,240,295,296,241,291,242,292,267,335,334,332,337,
  333,336,331,718,226,326,312,265,327,331,330,329,328,225,331,321,320,319,
  318,317,316,315,314,313,311,294,293,265,309,308,
137,137,137,137,137,137,137,137,137,137,137,285,137,256,256,256,256,288,290,
  287,297,299,300,301,302,303,304,305,306,307,323,324,325,286,322,289,298,
  310,284,271,136,135,663,240,295,296,241,291,242,292,267,335,334,332,337,
  333,336,331,719,226,326,312,265,327,331,330,329,328,225,331,321,320,319,
  318,317,316,315,314,313,311,294,293,265,309,308,
137,137,137,137,137,137,137,137,137,137,137,285,137,256,256,256,256,288,290,
  287,297,299,300,301,302,303,304,305,306,307,323,324,325,286,322,289,298,
  310,284,271,136,135,664,240,295,296,241,291,242,292,267,335,334,332,337,
  333,336,331,720,226,326,312,265,327,331,330,329,328,225,331,321,320,319,
  318,317,316,315,314,313,311,294,293,265,309,308,
137,137,137,137,137,137,137,137,137,137,137,285,137,256,256,256,256,288,290,
  287,297,299,300,301,302,303,304,305,306,307,323,324,325,286,322,289,298,
  310,284,271,136,135,665,240,295,296,241,291,242,292,267,335,334,332,337,
  333,336,331,721,226,326,312,265,327,331,330,329,328,225,331,321,320,319,
  318,317,316,315,314,313,311,294,293,265,309,308,
137,137,137,137,137,137,137,137,137,137,137,285,137,256,256,256,256,288,290,
  287,297,299,300,301,302,303,304,305,306,307,323,324,325,286,322,289,298,
  310,284,271,136,135,666,240,295,296,241,291,242,292,267,335,334,332,337,
  333,336,331,722,226,326,312,265,327,331,330,329,328,225,331,321,320,319,
  318,317,316,315,314,313,311,294,293,265,309,308,
137,137,137,137,137,137,137,137,137,137,137,285,137,256,256,256,256,288,290,
  287,297,299,300,301,302,303,304,305,306,307,323,324,325,286,322,289,298,
  310,284,271,136,135,667,240,295,296,241,291,242,292,267,335,334,332,337,
  333,336,331,723,226,326,312,265,327,331,330,329,328,225,331,321,320,319,
  318,317,316,315,314,313,311,294,293,265,309,308,
137,137,137,137,137,137,137,137,137,137,137,285,137,256,256,256,256,288,290,
  287,297,299,300,301,302,303,304,305,306,307,323,324,325,286,322,289,298,
  310,284,271,136,135,668,240,295,296,241,291,242,292,267,335,334,332,337,
  333,336,331,724,226,326,312,265,327,331,330,329,328,225,331,321,320,319,
  318,317,316,315,314,313,311,294,293,265,309,308,
137,137,137,137,137,137,137,137,137,137,137,285,137,256,256,256,256,288,290,
  287,297,299,300,301,302,303,304,305,306,307,323,324,325,286,322,289,298,
  310,284,271,136,135,669,240,295,296,241,291,242,292,267,335,334,332,337,
  333,336,331,725,226,326,312,265,327,331,330,329,328,225,331,321,320,319,
  318,317,316,315,314,313,311,294,293,265,309,308,
137,137,137,137,137,137,137,137,137,137,137,285,137,256,256,256,256,288,290,
  287,297,299,300,301,302,303,304,305,306,307,323,324,325,286,322,289,298,
  310,284,271,136,135,670,240,295,296,241,291,242,292,267,335,334,332,337,
  333,336,331,726,226,326,312,265,327,331,330,329,328,225,331,321,320,319,
  318,317,316,315,314,313,311,294,293,265,309,308,
137,137,137,137,137,137,137,137,137,137,137,285,137,256,256,256,256,288,290,
  287,297,299,300,301,302,303,304,305,306,307,323,324,325,286,322,289,298,
  310,284,271,136,135,671,240,295,296,241,291,242,292,267,335,334,332,337,
  333,336,331,727,226,326,312,265,327,331,330,329,328,225,331,321,320,319,
  318,317,316,315,314,313,311,294,293,265,309,308,
728,1,672,1,728,
137,137,137,137,137,137,137,137,137,137,137,285,137,256,256,256,256,288,290,
  287,297,299,300,301,302,303,304,305,306,307,286,563,289,298,310,284,271,
  136,135,673,240,295,296,241,291,242,292,267,226,326,312,265,219,225,219,
  321,320,319,318,317,316,315,314,313,311,294,293,265,309,308,
137,137,137,137,137,137,137,137,137,137,137,285,137,256,256,256,256,288,290,
  287,297,299,300,301,302,303,304,305,306,307,286,563,289,298,310,284,271,
  136,135,674,240,295,296,241,291,242,292,267,226,326,312,265,218,225,218,
  321,320,319,318,317,316,315,314,313,311,294,293,265,309,308,
137,137,137,137,137,137,137,137,137,137,137,285,137,256,256,256,256,288,290,
  287,297,299,300,301,302,303,304,305,306,307,286,563,289,298,310,284,271,
  136,135,675,240,295,296,241,291,242,292,267,226,326,312,265,217,225,217,
  321,320,319,318,317,316,315,314,313,311,294,293,265,309,308,
137,137,137,137,137,137,137,137,137,137,137,285,137,256,256,256,256,288,290,
  287,297,299,300,301,302,303,304,305,306,307,286,563,289,298,310,284,271,
  136,135,676,240,295,296,241,291,242,292,267,226,326,312,265,216,225,216,
  321,320,319,318,317,316,315,314,313,311,294,293,265,309,308,
137,137,137,137,137,137,137,137,137,137,137,285,137,256,256,256,256,288,290,
  287,297,299,300,301,302,303,304,305,306,307,286,563,289,298,310,284,271,
  136,135,677,240,295,296,241,291,242,292,267,226,326,312,265,215,225,215,
  321,320,319,318,317,316,315,314,313,311,294,293,265,309,308,
137,137,137,137,137,137,137,137,137,137,137,285,137,256,256,256,256,288,290,
  287,297,299,300,301,302,303,304,305,306,307,323,324,325,286,322,289,298,
  310,284,271,136,135,678,240,295,296,241,291,242,292,267,729,331,226,326,
  312,265,327,331,330,329,328,225,331,321,320,319,318,317,316,315,314,313,
  311,294,293,265,309,308,
137,137,137,137,137,137,137,137,137,137,137,285,137,256,256,256,256,288,290,
  287,297,299,300,301,302,303,304,305,306,307,323,324,325,286,322,289,298,
  310,284,271,136,135,679,240,295,296,241,291,242,292,267,730,331,226,326,
  312,265,327,331,330,329,328,225,331,321,320,319,318,317,316,315,314,313,
  311,294,293,265,309,308,
137,137,137,137,137,137,137,137,137,137,137,285,137,256,256,256,256,288,290,
  287,297,299,300,301,302,303,304,305,306,307,323,324,325,286,322,289,298,
  310,284,271,136,135,680,240,295,296,241,291,242,292,267,332,731,331,226,
  326,312,265,327,331,330,329,328,225,331,321,320,319,318,317,316,315,314,
  313,311,294,293,265,309,308,
137,137,137,137,137,137,137,137,137,137,137,285,137,256,256,256,256,288,290,
  287,297,299,300,301,302,303,304,305,306,307,323,324,325,286,322,289,298,
  310,284,271,136,135,681,240,295,296,241,291,242,292,267,332,732,331,226,
  326,312,265,327,331,330,329,328,225,331,321,320,319,318,317,316,315,314,
  313,311,294,293,265,309,308,
137,137,137,137,137,137,137,137,137,137,137,285,137,256,256,256,256,288,290,
  287,297,299,300,301,302,303,304,305,306,307,323,324,325,286,322,289,298,
  310,284,271,136,135,682,240,295,296,241,291,242,292,267,332,733,331,226,
  326,312,265,327,331,330,329,328,225,331,321,320,319,318,317,316,315,314,
  313,311,294,293,265,309,308,
137,137,137,137,137,137,137,137,137,137,137,285,137,256,256,256,256,288,290,
  287,297,299,300,301,302,303,304,305,306,307,323,324,325,286,322,289,298,
  310,284,271,136,135,683,240,295,296,241,291,242,292,267,332,734,331,226,
  326,312,265,327,331,330,329,328,225,331,321,320,319,318,317,316,315,314,
  313,311,294,293,265,309,308,
137,137,137,137,137,137,137,137,137,137,137,285,137,256,256,256,256,288,290,
  287,297,299,300,301,302,303,304,305,306,307,323,324,325,286,322,289,298,
  310,284,271,136,135,684,240,295,296,241,291,242,292,267,735,332,333,331,
  226,326,312,265,327,331,330,329,328,225,331,321,320,319,318,317,316,315,
  314,313,311,294,293,265,309,308,
137,137,137,137,137,137,137,137,137,137,137,285,137,256,256,256,256,288,290,
  287,297,299,300,301,302,303,304,305,306,307,323,324,325,286,322,289,298,
  310,284,271,136,135,685,240,295,296,241,291,242,292,267,736,332,333,331,
  226,326,312,265,327,331,330,329,328,225,331,321,320,319,318,317,316,315,
  314,313,311,294,293,265,309,308,
137,137,137,137,137,137,137,137,137,137,137,285,137,256,256,256,256,288,290,
  287,297,299,300,301,302,303,304,305,306,307,323,324,325,286,322,289,298,
  310,284,271,136,135,686,240,295,296,241,291,242,292,267,737,334,332,333,
  331,226,326,312,265,327,331,330,329,328,225,331,321,320,319,318,317,316,
  315,314,313,311,294,293,265,309,308,
137,137,137,137,137,137,137,137,137,137,137,285,137,256,256,256,256,288,290,
  287,297,299,300,301,302,303,304,305,306,307,323,324,325,286,322,289,298,
  310,284,271,136,135,687,240,295,296,241,291,242,292,267,738,334,332,333,
  331,226,326,312,265,327,331,330,329,328,225,331,321,320,319,318,317,316,
  315,314,313,311,294,293,265,309,308,
137,137,137,137,137,137,137,137,137,137,137,285,137,256,256,256,256,288,290,
  287,297,299,300,301,302,303,304,305,306,307,323,324,325,286,322,289,298,
  310,284,271,136,135,688,240,295,296,241,291,242,292,267,335,334,332,333,
  739,331,226,326,312,265,327,331,330,329,328,225,331,321,320,319,318,317,
  316,315,314,313,311,294,293,265,309,308,
137,137,137,137,137,137,137,137,137,137,137,285,137,256,256,256,256,288,290,
  287,297,299,300,301,302,303,304,305,306,307,323,324,325,286,322,289,298,
  310,284,271,136,135,689,240,295,296,241,291,242,292,267,335,334,332,333,
  740,331,226,326,312,265,327,331,330,329,328,225,331,321,320,319,318,317,
  316,315,314,313,311,294,293,265,309,308,
741,741,741,741,741,1,690,1,741,
43,44,45,40,36,691,36,36,48,36,47,46,46,
43,44,45,40,35,692,35,35,48,35,47,46,46,
43,44,45,40,34,693,34,34,48,34,47,46,46,
43,44,45,40,31,694,31,31,48,31,47,46,46,
742,742,742,742,742,1,695,1,742,
743,743,743,743,743,1,696,1,743,
43,44,45,40,73,697,73,73,48,73,47,46,46,
137,137,137,137,137,137,137,137,137,137,137,285,137,256,256,256,256,288,290,
  287,297,299,300,301,302,303,304,305,306,307,323,324,325,286,322,289,298,
  310,284,271,136,135,698,240,295,296,241,291,242,292,267,335,334,332,337,
  333,336,331,404,226,326,312,265,327,331,330,329,328,225,331,321,320,319,
  318,317,316,315,314,313,311,294,293,265,309,308,
137,137,137,137,137,137,137,137,137,137,137,285,137,256,256,256,256,288,290,
  287,297,299,300,301,302,303,304,305,306,307,323,324,325,286,322,289,298,
  310,284,271,136,135,699,240,295,296,241,291,242,292,267,335,334,332,337,
  333,336,331,402,226,326,312,265,327,331,330,329,328,225,331,321,320,319,
  318,317,316,315,314,313,311,294,293,265,309,308,
137,137,137,137,137,137,137,137,137,137,137,285,137,256,256,256,256,288,290,
  287,297,299,300,301,302,303,304,305,306,307,323,324,325,286,322,289,298,
  310,284,271,136,135,700,240,295,296,241,291,242,292,267,335,334,332,337,
  333,336,331,400,226,326,312,265,327,331,330,329,328,225,331,321,320,319,
  318,317,316,315,314,313,311,294,293,265,309,308,
703,701,744,
272,547,272,272,272,272,272,272,272,272,272,272,272,272,272,272,272,272,272,
  272,272,269,272,272,272,272,272,272,272,272,272,272,272,272,272,272,272,
  272,272,272,272,278,272,702,745,
20,20,20,20,20,20,20,20,20,20,20,20,20,20,20,20,20,20,20,20,20,20,20,20,20,
  20,20,20,20,20,20,20,20,20,20,20,20,20,20,20,20,20,20,20,20,20,20,20,20,
  20,20,20,20,703,1,948,
746,746,746,746,746,746,746,746,746,746,746,746,746,746,746,746,746,746,746,
  746,746,746,746,746,746,746,746,746,746,746,746,746,746,746,746,746,746,
  746,746,746,746,746,746,1,704,1,746,
487,705,488,
20,328,328,328,328,328,1,706,1,317,
272,547,272,272,272,272,272,272,272,272,272,272,272,272,272,272,272,272,272,
  272,272,272,272,272,272,272,272,272,272,272,272,272,272,272,272,272,272,
  272,272,272,278,272,707,143,
747,747,747,747,747,747,747,747,747,747,747,747,747,747,747,747,747,747,747,
  747,747,747,747,747,747,747,747,747,747,747,747,747,747,747,747,747,747,
  747,747,747,747,747,1,708,1,747,
703,103,103,103,103,103,103,709,704,
748,748,748,748,748,748,748,748,748,748,748,748,748,748,748,1,710,1,748,
20,20,20,20,20,20,20,20,20,20,20,20,20,20,20,20,20,20,20,20,20,20,20,20,20,
  20,20,20,20,20,20,20,20,20,20,20,20,20,20,20,20,20,20,20,20,20,711,1,
  916,
68,68,68,68,68,68,68,68,68,68,68,68,68,68,68,68,68,68,68,68,68,68,68,68,68,
  68,68,68,68,68,68,68,68,68,68,68,68,68,68,68,18,18,18,18,18,18,712,
43,44,45,40,64,713,64,64,48,64,47,46,46,
137,137,137,137,137,137,137,137,137,137,137,285,137,256,256,256,256,288,290,
  287,297,299,300,301,302,303,304,305,306,307,323,324,325,286,322,289,298,
  310,284,271,136,135,714,240,295,296,241,291,242,292,267,335,334,332,337,
  333,336,331,749,226,326,312,265,327,331,330,329,328,225,331,321,320,319,
  318,317,316,315,314,313,311,294,293,265,309,308,
162,162,162,162,162,162,162,162,161,161,161,161,162,162,162,162,161,161,161,
  161,161,162,162,162,162,162,162,162,162,162,162,162,162,162,162,162,162,
  162,162,162,162,162,715,
158,158,158,158,158,716,
137,137,137,137,137,137,137,137,137,137,137,137,137,136,135,717,752,750,750,
  751,
388,718,238,
388,719,237,
388,720,236,
388,721,235,
388,722,234,
388,723,233,
388,724,232,
388,725,231,
388,726,230,
388,727,229,
388,728,228,
213,564,566,391,383,387,213,213,213,213,213,213,213,213,213,213,213,213,213,
  213,213,213,213,213,213,213,213,213,213,213,213,213,213,213,213,213,213,
  729,570,569,568,567,565,
212,564,566,391,383,387,212,212,212,212,212,212,212,212,212,212,212,212,212,
  212,212,212,212,212,212,212,212,212,212,212,212,212,212,212,212,212,212,
  730,570,569,568,567,565,
210,384,386,210,210,210,210,210,210,210,210,210,210,210,210,210,210,210,210,
  210,210,210,210,210,210,210,210,210,210,210,210,210,210,210,731,572,571,
209,384,386,209,209,209,209,209,209,209,209,209,209,209,209,209,209,209,209,
  209,209,209,209,209,209,209,209,209,209,209,209,209,209,209,732,572,571,
208,384,386,208,208,208,208,208,208,208,208,208,208,208,208,208,208,208,208,
  208,208,208,208,208,208,208,208,208,208,208,208,208,208,208,733,572,571,
207,384,386,207,207,207,207,207,207,207,207,207,207,207,207,207,207,207,207,
  207,207,207,207,207,207,207,207,207,207,207,207,207,207,207,734,572,571,
205,573,575,577,579,205,205,205,205,205,205,205,205,205,205,205,205,205,205,
  205,205,205,205,205,205,205,205,205,205,205,205,205,735,580,578,576,574,
204,573,575,577,579,204,204,204,204,204,204,204,204,204,204,204,204,204,204,
  204,204,204,204,204,204,204,204,204,204,204,204,204,736,580,578,576,574,
202,581,390,202,202,202,202,202,202,202,202,202,202,202,202,202,202,202,202,
  202,202,202,202,202,202,202,202,202,737,583,582,
201,581,390,201,201,201,201,201,201,201,201,201,201,201,201,201,201,201,201,
  201,201,201,201,201,201,201,201,201,738,583,582,
199,584,362,199,199,199,199,199,199,199,199,199,199,199,199,199,199,199,199,
  199,199,199,199,199,199,199,739,586,585,
198,584,362,198,198,198,198,198,198,198,198,198,198,198,198,198,198,198,198,
  198,198,198,198,198,198,198,740,586,585,
43,44,45,40,47,741,47,47,48,47,47,46,46,
43,44,45,40,30,742,30,30,48,30,47,46,46,
43,44,45,40,29,743,29,29,48,29,47,46,46,
753,753,753,753,753,753,753,753,753,1,744,1,753,
143,143,143,143,143,143,143,143,143,143,143,143,143,143,143,143,143,143,143,
  143,143,270,143,143,143,143,143,143,143,143,143,143,143,143,143,143,143,
  143,143,143,143,143,143,745,
137,137,137,137,137,137,137,137,137,137,137,285,137,256,256,256,256,288,636,
  287,146,297,299,300,301,302,303,304,305,306,307,323,324,325,286,322,289,
  298,310,284,271,136,135,746,240,295,296,241,291,129,278,242,292,637,130,
  267,335,334,332,337,333,336,331,128,226,326,312,265,327,331,330,329,328,
  225,331,321,320,319,318,317,316,315,314,313,311,294,293,279,265,638,309,
  308,
137,137,137,137,137,137,137,137,137,137,137,285,137,256,256,256,256,288,290,
  287,297,299,300,301,302,303,304,305,306,307,323,324,325,286,322,289,298,
  310,284,271,136,135,747,240,295,296,241,291,242,292,267,335,334,332,337,
  333,336,331,107,226,326,312,265,327,331,330,329,328,225,331,321,320,319,
  318,317,316,315,314,313,311,294,293,265,309,308,
137,137,137,137,137,137,137,137,137,137,137,137,137,136,135,748,132,265,265,
754,754,754,754,754,1,749,1,754,
138,138,138,138,138,138,138,138,138,138,138,138,139,139,139,139,139,138,20,
  138,1,750,1,133,139,
20,20,751,1,991,
755,1,752,1,755,
293,292,292,289,288,287,286,285,284,753,389,292,628,
43,44,45,40,62,754,62,62,48,62,47,46,46,
388,755,239,

};


static const unsigned short ag_sbt[] = {
     0,  31,  33,  74,  92, 123, 153, 354, 357, 375, 576, 945, 984,1003,
  1022,1027,1032,1079,1084,1093,1102,1121,1167,1213,1232,1241,1266,1286,
  1305,1310,1315,1362,1367,1376,1385,1404,1491,1578,1597,1608,1617,1675,
  1767,1773,1821,1872,1918,2010,2097,2184,2560,2571,2581,2591,2601,2611,
  2621,2631,2641,2651,2661,2671,2681,2691,2701,2711,2724,2737,2748,2794,
  2840,2886,2932,2978,3024,3070,3116,3162,3208,3254,3300,3346,3392,3438,
  3484,3494,3504,3514,3524,3534,3544,3554,3564,3574,3584,3594,3604,3614,
  3624,3634,3644,3654,3664,3674,3684,3694,3704,3714,3724,3734,3744,3754,
  3764,3774,3782,3795,3808,3819,3865,3911,3957,4003,4049,4095,4141,4187,
  4233,4279,4325,4371,4417,4463,4509,4555,4601,4647,4693,4739,4785,4831,
  4877,4923,4969,5015,5061,5107,5153,5199,5245,5291,5337,5383,5429,5475,
  5521,5567,5613,5659,5705,5751,5797,5843,5889,5935,5981,6027,6039,6051,
  6063,6075,6083,6091,6102,6113,6124,6137,6150,6163,6176,6189,6202,6215,
  6228,6241,6254,6267,6280,6293,6301,6306,6311,6316,6362,6408,6454,6500,
  6546,6592,6638,6684,6730,6776,6822,6868,6914,6960,7006,7052,7098,7144,
  7190,7236,7282,7328,7374,7420,7466,7512,7558,7604,7650,7696,7742,7788,
  7834,7880,7926,7972,8018,8064,8110,8156,8202,8248,8294,8340,8386,8432,
  8478,8524,8570,8616,8662,8708,8720,8732,8744,8756,8764,8772,8783,8794,
  8805,8818,8831,8844,8857,8870,8883,8896,8909,8922,8935,8948,8961,8974,
  9035,9045,9116,9124,9127,9214,9293,9302,9346,9355,9360,9365,9454,9498,
  9510,9519,9532,9545,9554,9560,9565,9568,9628,9684,9690,9734,9740,9751,
  9795,9837,9886,9940,9945,9955,9960,9965,9970,9975,9980,9985,9990,9995,
  10000,10042,10084,10150,10155,10160,10165,10170,10175,10180,10185,10190,
  10195,10200,10205,10248,10313,10356,10421,10467,10537,10607,10677,10747,
  10789,10834,10873,10912,10945,10976,10981,11020,11105,11114,11123,11131,
  11140,11149,11158,11167,11186,11205,11214,11223,11232,11241,11283,11292,
  11301,11310,11335,11365,11416,11482,11533,11599,11647,11698,11749,11800,
  11851,11902,11953,12004,12055,12106,12157,12208,12259,12310,12361,12412,
  12463,12514,12565,12616,12681,12747,12813,12879,12944,13019,13070,13136,
  13201,13252,13303,13354,13412,13496,13499,13502,13548,13633,13716,13725,
  13743,13760,13773,13786,13792,13795,13798,13801,13886,13971,14056,14141,
  14226,14311,14396,14481,14566,14651,14736,14821,14906,14991,15076,15161,
  15246,15331,15416,15501,15586,15671,15756,15841,15926,16011,16096,16181,
  16266,16351,16436,16521,16606,16691,16776,16861,16946,17031,17116,17201,
  17286,17371,17456,17541,17626,17711,17796,17881,17966,18051,18136,18221,
  18236,18251,18266,18281,18289,18297,18310,18323,18336,18353,18370,18383,
  18396,18409,18422,18435,18448,18461,18474,18487,18500,18513,18606,18628,
  18765,18902,18921,18940,18963,18985,18994,19003,19009,19018,19027,19036,
  19060,19085,19090,19095,19142,19189,19281,19327,19412,19431,19450,19469,
  19488,19507,19527,19546,19566,19585,19605,19652,19699,19792,19839,19886,
  19933,20026,20072,20118,20203,20212,20258,20304,20350,20435,20454,20465,
  20476,20489,20571,20584,20587,20596,20605,20614,20629,20642,20655,20665,
  20672,20717,20767,20817,20820,20823,20826,20829,20832,20835,20838,20841,
  20844,20847,20850,20935,20941,20984,21027,21070,21113,21156,21199,21242,
  21288,21334,21380,21426,21472,21518,21564,21610,21656,21702,21748,21794,
  21840,21886,21932,21978,22024,22070,22116,22162,22208,22254,22300,22346,
  22392,22438,22484,22530,22576,22622,22668,22681,22694,22707,22720,22733,
  22752,22765,22778,22822,22835,22848,22874,22883,22892,22901,22910,22956,
  23041,23087,23172,23185,23194,23203,23214,23225,23236,23248,23259,23305,
  23351,23397,23407,23417,23422,23466,23511,23522,23527,23537,23559,23850,
  23894,23897,23990,24079,24088,24097,24106,24115,24124,24212,24258,24271,
  24284,24297,24307,24313,24356,24358,24377,24462,24547,24632,24717,24802,
  24887,24972,25057,25142,25227,25232,25302,25372,25442,25512,25582,25661,
  25740,25820,25900,25980,26060,26141,26222,26304,26386,26469,26552,26561,
  26574,26587,26600,26613,26622,26631,26644,26729,26814,26899,26902,26947,
  27003,27050,27053,27063,27107,27153,27162,27181,27230,27277,27290,27375,
  27418,27424,27444,27447,27450,27453,27456,27459,27462,27465,27468,27471,
  27474,27477,27520,27563,27600,27637,27674,27711,27748,27785,27816,27847,
  27876,27905,27918,27931,27944,27957,28001,28093,28178,28197,28206,28231,
  28236,28241,28254,28267,28270
};


static const unsigned short ag_sbe[] = {
    28,  32,  60,  89, 121, 137, 351, 355, 365, 573, 773, 972,1000,1019,
  1024,1029,1076,1081,1090,1099,1118,1164,1210,1229,1238,1263,1281,1301,
  1307,1312,1359,1363,1373,1382,1400,1446,1533,1593,1602,1610,1672,1764,
  1769,1818,1869,1915,1963,2053,2140,2381,2568,2578,2588,2598,2608,2618,
  2628,2638,2648,2658,2668,2678,2688,2698,2708,2721,2734,2745,2791,2837,
  2883,2929,2975,3021,3067,3113,3159,3205,3251,3297,3343,3389,3435,3481,
  3491,3501,3511,3521,3531,3541,3551,3561,3571,3581,3591,3601,3611,3621,
  3631,3641,3651,3661,3671,3681,3691,3701,3711,3721,3731,3741,3751,3761,
  3771,3779,3792,3805,3816,3862,3908,3954,4000,4046,4092,4138,4184,4230,
  4276,4322,4368,4414,4460,4506,4552,4598,4644,4690,4736,4782,4828,4874,
  4920,4966,5012,5058,5104,5150,5196,5242,5288,5334,5380,5426,5472,5518,
  5564,5610,5656,5702,5748,5794,5840,5886,5932,5978,6024,6036,6048,6060,
  6072,6080,6088,6099,6110,6121,6134,6147,6160,6173,6186,6199,6212,6225,
  6238,6251,6264,6277,6290,6298,6303,6308,6313,6359,6405,6451,6497,6543,
  6589,6635,6681,6727,6773,6819,6865,6911,6957,7003,7049,7095,7141,7187,
  7233,7279,7325,7371,7417,7463,7509,7555,7601,7647,7693,7739,7785,7831,
  7877,7923,7969,8015,8061,8107,8153,8199,8245,8291,8337,8383,8429,8475,
  8521,8567,8613,8659,8705,8717,8729,8741,8753,8761,8769,8780,8791,8802,
  8815,8828,8841,8854,8867,8880,8893,8906,8919,8932,8945,8958,8971,9031,
  9042,9114,9122,9125,9169,9251,9299,9343,9352,9356,9361,9408,9496,9507,
  9516,9524,9537,9551,9559,9564,9567,9627,9683,9689,9732,9739,9750,9794,
  9836,9885,9938,9942,9952,9957,9962,9967,9972,9977,9982,9987,9992,9997,
  10039,10081,10147,10152,10157,10162,10167,10172,10177,10182,10187,10192,
  10197,10202,10245,10310,10353,10418,10464,10506,10576,10646,10716,10786,
  10826,10868,10905,10940,10971,10978,11000,11062,11111,11120,11129,11137,
  11146,11155,11164,11183,11202,11211,11220,11229,11238,11282,11289,11298,
  11307,11332,11356,11413,11479,11530,11596,11644,11695,11746,11797,11848,
  11899,11950,12001,12052,12103,12154,12205,12256,12307,12358,12409,12460,
  12511,12562,12613,12678,12744,12810,12876,12941,13016,13067,13133,13198,
  13249,13300,13351,13409,13454,13497,13500,13545,13590,13672,13722,13732,
  13752,13769,13780,13790,13793,13796,13799,13843,13928,14013,14098,14183,
  14268,14353,14438,14523,14608,14693,14778,14863,14948,15033,15118,15203,
  15288,15373,15458,15543,15628,15713,15798,15883,15968,16053,16138,16223,
  16308,16393,16478,16563,16648,16733,16818,16903,16988,17073,17158,17243,
  17328,17413,17498,17583,17668,17753,17838,17923,18008,18093,18178,18229,
  18244,18259,18274,18285,18293,18304,18317,18330,18345,18362,18379,18392,
  18405,18418,18431,18444,18457,18470,18483,18496,18509,18556,18619,18762,
  18899,18918,18936,18960,18982,18991,19000,19006,19011,19024,19033,19057,
  19081,19086,19091,19139,19186,19233,19324,19369,19428,19446,19466,19484,
  19504,19522,19543,19561,19582,19600,19649,19696,19742,19836,19883,19930,
  19976,20069,20115,20160,20209,20255,20301,20347,20392,20450,20462,20473,
  20481,20529,20576,20585,20593,20602,20611,20627,20634,20647,20664,20671,
  20715,20766,20816,20818,20821,20824,20827,20830,20833,20836,20839,20842,
  20845,20848,20892,20940,20981,21024,21067,21110,21153,21196,21239,21285,
  21331,21377,21423,21469,21515,21561,21607,21653,21699,21745,21791,21837,
  21883,21929,21975,22021,22067,22113,22159,22205,22251,22297,22343,22389,
  22435,22481,22527,22573,22619,22665,22673,22686,22699,22712,22725,22748,
  22757,22770,22821,22827,22840,22871,22880,22889,22898,22907,22953,22998,
  23084,23129,23177,23191,23200,23211,23222,23233,23245,23256,23302,23348,
  23394,23404,23414,23419,23464,23509,23519,23524,23536,23557,23692,23892,
  23895,23940,24033,24086,24095,24104,24113,24122,24168,24255,24263,24276,
  24289,24306,24312,24355,24357,24374,24419,24504,24589,24674,24759,24844,
  24929,25014,25099,25184,25229,25271,25341,25411,25481,25551,25624,25703,
  25782,25862,25942,26022,26102,26183,26264,26346,26428,26511,26558,26566,
  26579,26592,26605,26619,26628,26636,26686,26771,26856,26900,26945,27000,
  27047,27051,27060,27105,27150,27160,27178,27227,27276,27282,27332,27417,
  27423,27439,27445,27448,27451,27454,27457,27460,27463,27466,27469,27472,
  27475,27514,27557,27597,27634,27671,27708,27743,27780,27813,27844,27873,
  27902,27910,27923,27936,27954,28000,28044,28135,28193,28203,28227,28233,
  28238,28250,28259,28268,28270
};


static const unsigned char ag_fl[] = {
  2,1,1,2,2,1,1,2,0,1,3,3,1,1,2,1,2,2,1,2,0,1,4,5,3,2,1,0,1,7,7,6,1,1,6,
  6,6,2,3,2,2,1,2,4,4,4,4,6,4,4,0,1,4,4,4,4,3,3,4,5,5,5,8,4,6,4,1,1,2,2,
  2,2,3,5,1,2,2,2,3,2,1,1,1,3,2,1,1,3,1,1,1,3,1,1,3,3,3,3,3,3,3,3,3,4,4,
  4,3,6,2,3,2,1,1,2,2,3,3,2,2,3,3,1,1,2,3,1,1,1,4,4,4,1,4,2,1,1,1,1,2,2,
  1,2,2,3,2,2,1,2,3,1,2,2,2,2,2,2,2,2,4,1,1,4,3,2,1,2,3,3,2,1,1,1,1,1,1,
  2,1,1,1,1,2,1,1,2,1,1,2,1,1,2,1,1,2,1,1,2,1,2,4,4,2,4,4,2,4,4,2,4,4,4,
  4,2,4,4,2,4,4,4,4,4,1,2,2,2,2,1,1,1,5,5,5,5,5,5,5,5,5,5,5,7,1,1,1,1,1,
  1,1,2,2,2,1,1,2,2,2,2,1,2,2,2,3,2,2,2,2,2,2,2,2,3,4,1,1,2,2,2,2,2,1,2,
  1,2,2,2,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,
  1,1,2,1,1,1,1,1,1,1,1,1,1,4,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,
  1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,3,3,3,3,3,3,3,3,
  3,3,3,3,7,3,3,3,3,3,3,3,3,3,3,5,3,5,3,5,3,3,3,3,3,3,3,3,3,3,3,3,3,3,3,
  3,1,1,1,1,1,3,3,3,3,1,1,1,1,1,3,3,3,3,3,3,3,1,1,1,3,1,1,1,3,3,3,3,3,3,
  3,3,3,3,3,3,3,3,3,3,3,3,3,3,3,3,3,3,3,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,
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
  1,1,1,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,
  2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,
  2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,
  2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,
  2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,
  2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,
  2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,
  2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,
  2,2,2,2,2,2
};

static const unsigned short ag_ptt[] = {
    0, 36, 37, 37, 34, 39, 43, 43, 44, 44, 39, 39, 39, 39, 40, 47, 47, 47,
   51, 51, 52, 52, 35, 35, 35, 35, 35, 55, 55, 35, 35, 35, 62, 62, 35, 35,
   35, 35, 35, 35, 35, 65, 53, 67, 67, 67, 67, 67, 67, 67, 75, 75, 67, 67,
   67, 67, 67, 67, 67, 67, 67, 67, 67, 67, 67, 89, 89, 90, 90, 90, 97, 97,
   63, 63, 33, 33, 33, 33, 33, 33,103,103,103, 33, 33,107,107, 33,111,111,
  111, 33,114,114, 33, 33, 33, 33, 33, 33, 33, 33, 33, 33, 33, 33, 33, 33,
   33, 33, 33, 33, 33, 33, 33, 33, 33, 33, 33, 33, 33, 33, 33, 30, 30, 92,
   92, 92, 92, 92, 92,116,116,536,442,442,442,133,133,133,437,437,485, 18,
   18,435, 12, 12, 12,142,142,142,142,142,142,142,142,142,142,155,155,142,
  142,436, 14, 14, 14, 14,428,428,428,428,428, 22, 22, 77, 77,169,169,169,
  166,172,172,166,175,175,166,178,178,166,181,181,166,184,184,166, 57, 26,
   26, 26, 28, 28, 28, 23, 23, 23, 24, 24, 24, 24, 24, 27, 27, 27, 25, 25,
   25, 25, 25, 25, 29, 29, 29, 29, 29,198,198,198,198,207,207,207,207,207,
  207,207,207,207,207,207,206,206,492,492,492,492,492,492,492,492,223,223,
  492,492,  7,  7,  7,  7,  7,  7,  6,  6,  6, 17, 17,218,218,219,219, 15,
   15, 15, 16, 16, 16, 16, 16, 16, 16, 16,538,  9,  9,  9,594,594,594,594,
  594,594,242,242,594,594,245,245,608,247,247,608,249,249,608,608,251,251,
  251,251,251,253,253,254,254,254,254,254,254, 31, 32, 32, 32, 32, 32, 32,
   32, 32, 32, 32, 32,272,272,272,272,266,266,266,266,266,279,279,266,266,
  266,285,285,285,266,266,266,266,266,266,266,266,266,296,296,266,266,266,
  266,266,266,266,266,266,266,309,309,309,266,266,266,266,266,266,266,259,
  259,260,260,260,260,257,257,257,257,257,257,257,257,257,257,257,257,257,
  258,258,258,338,262,340,263,342,264,265,261,261,261,261,261,261,261,261,
  261,261,261,261,261,261,261,366,366,366,366,366,261,261,261,261,375,375,
  375,375,375,261,261,261,261,261,261,261,385,385,385,261,389,389,389,261,
  261,261,261,261,261,261,261,261,261,261,261,261,261,261,261,261,261,261,
  261,261,261,261,261,261,138,345,153,226, 42, 42, 42, 42, 42, 42, 42, 42,
   42, 42, 42, 42, 42, 42, 42, 42, 42, 42, 42, 42, 42, 42, 42, 42, 42, 42,
   42, 42, 42, 42, 42, 42, 42, 42, 42, 42, 42, 42, 42, 42, 42, 42, 50, 50,
   50, 50, 50, 50, 50, 50, 50, 50, 50, 50, 50, 50, 50, 50, 50, 50, 50, 50,
   50, 50, 50, 50, 50, 50, 50, 50, 50, 50, 50, 50, 50, 50, 50, 50, 50, 50,
   50, 50, 50, 50, 50, 94, 94, 94, 94, 94, 94, 94, 94, 94, 94, 94, 94, 94,
   94, 94, 94, 94, 94, 94, 94, 94, 94, 94, 94, 94, 94, 94, 94, 94, 94, 94,
   94, 94, 94, 94, 94, 94, 94, 94, 94, 95, 95, 95, 95, 95, 95, 95, 95, 95,
   95, 95, 95, 95, 95, 95, 95, 95, 95, 95, 95, 95, 95, 95, 95, 95, 95, 95,
   95, 95, 95, 95, 95, 95, 95, 95, 95, 95, 95, 95, 95, 95,136,136,136,136,
  136,136,136,136,136,136,136,136,136,137,137,137,137,137,137,137,137,137,
  137,137,137,137,137,139,139,139,139,139,139,139,139,139,139,139,139,139,
  139,139,139,139,139,139,144,144,144,144,144,144,144,144,144,144,144,144,
  144,144,144,144,144,144,144,144,144,144,144,144,144,144,144,144,144,144,
  144,144,144,144,144,144,144,144,144,144,151,151,151,152,152,152,152,152,
  156,156,156,156,156,156,156,156,156,159,159,159,159,159,159,159,159,159,
  159,159,159,159,159,159,159,159,159,159,159,159,159,159,159,159,159,159,
  159,159,159,159,159,159,159,159,159,159,159,159,160,160,160,160,160,160,
  160,160,160,160,160,160,160,160,160,160,160,160,160,160,160,160,160,160,
  160,160,160,160,160,160,160,160,160,160,160,160,160,160,160,160,160,160,
  227,227,227,227,228,228,230,230,230,230,232,232,232,232,232,232,232,232,
  232,232,232,232,232,232,232,232,232,232,232,232,232,232,232,232,232,232,
  232,232,232,232,232,232,232,232,232,232,232,232,232,232, 45, 41, 46, 48,
   49, 11, 54, 56, 58, 59, 60, 61, 10, 13, 19, 66, 69, 68, 70, 21, 71, 72,
   73, 74, 76, 78, 79, 80, 81, 82, 83, 84, 86, 85, 87, 88, 93, 91, 96, 98,
   99,100,101,102,104,105,106,108,109,110,112,113,115,117,118,119,120,121,
  122,123,124,125, 20,126,127,128,129,130,131,  2,132,165,167,168,170,171,
  173,174,176,177,179,180,182,183,185,186,187,188,189,190,191,192,193,194,
  195,196,197,199,200,201,202,203,204,205,208,209,210,211,212,213,214,215,
  216,  3,217,  8,252,243,255,256,250,267,268,269,270,271,273,274,275,276,
  277,278,280,281,282,283,284,286,287,288,289,290,291,292,293,294,295,297,
  298,299,300,301,302,303,304,305,306,307,308,310,311,312,313,314,315,316,
  317,318,319,320,321,  4,322,323,324,325,326,327,328,329,330,331,332,333,
  334,  5,335,336,337,339,341,343,344,346,347,348,349,350,351,352,353,354,
  355,356,357,358,359,360,361,362,363,364,365,367,368,369,370,371,372,373,
  374,376,377,378,379,380,381,382,383,384,386,387,388,390,391,392,393,394,
  395,396,397,398,399,400,401,244,402,403,404,405,406,407,408,409,410,411,
  412,414,141,154,140,148,415,416,417,418,158,157,419,229,420,149,224,150,
  421,220,145,147,146,413,143,225,422
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
    case 29: ag_rp_29(); break;
    case 30: ag_rp_30(); break;
    case 31: ag_rp_31(); break;
    case 32: ag_rp_32(V(0,int)); break;
    case 33: ag_rp_33(V(1,int)); break;
    case 34: ag_rp_34(); break;
    case 35: ag_rp_35(); break;
    case 36: ag_rp_36(); break;
    case 37: ag_rp_37(V(2,int)); break;
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
    case 70: ag_rp_70(); break;
    case 71: ag_rp_71(); break;
    case 72: ag_rp_72(); break;
    case 73: ag_rp_73(); break;
    case 74: ag_rp_74(); break;
    case 75: ag_rp_75(); break;
    case 76: ag_rp_76(V(2,int)); break;
    case 77: ag_rp_77(); break;
    case 78: ag_rp_78(); break;
    case 79: ag_rp_79(); break;
    case 80: ag_rp_80(); break;
    case 81: ag_rp_81(); break;
    case 82: ag_rp_82(); break;
    case 83: ag_rp_83(); break;
    case 84: ag_rp_84(); break;
    case 85: ag_rp_85(); break;
    case 86: ag_rp_86(); break;
    case 87: ag_rp_87(V(0,int)); break;
    case 88: ag_rp_88(V(1,int)); break;
    case 89: ag_rp_89(V(1,int)); break;
    case 90: ag_rp_90(V(0,int)); break;
    case 91: ag_rp_91(V(1,int)); break;
    case 92: ag_rp_92(V(1,int), V(2,int)); break;
    case 93: ag_rp_93(V(1,int)); break;
    case 94: ag_rp_94(); break;
    case 95: ag_rp_95(V(1,int)); break;
    case 96: V(0,int) = ag_rp_96(V(0,int)); break;
    case 97: V(0,int) = ag_rp_97(); break;
    case 98: V(0,int) = ag_rp_98(); break;
    case 99: V(0,int) = ag_rp_99(); break;
    case 100: V(0,int) = ag_rp_100(); break;
    case 101: V(0,int) = ag_rp_101(); break;
    case 102: V(0,int) = ag_rp_102(); break;
    case 103: V(0,int) = ag_rp_103(); break;
    case 104: V(0,int) = ag_rp_104(); break;
    case 105: V(0,int) = ag_rp_105(V(1,int), V(2,int), V(3,int)); break;
    case 106: V(0,int) = ag_rp_106(V(2,int), V(3,int)); break;
    case 107: V(0,int) = ag_rp_107(V(2,int)); break;
    case 108: ag_rp_108(); break;
    case 109: ag_rp_109(); break;
    case 110: ag_rp_110(V(1,int)); break;
    case 111: ag_rp_111(V(2,int)); break;
    case 112: ag_rp_112(); break;
    case 113: ag_rp_113(); break;
    case 114: ag_rp_114(); break;
    case 115: ag_rp_115(); break;
    case 116: ag_rp_116(); break;
    case 117: V(0,int) = ag_rp_117(); break;
    case 118: V(0,int) = ag_rp_118(); break;
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
    case 142: ag_rp_142(); break;
    case 143: ag_rp_143(); break;
    case 144: ag_rp_144(); break;
    case 145: ag_rp_145(); break;
    case 146: ag_rp_146(); break;
    case 147: ag_rp_147(); break;
    case 148: ag_rp_148(V(0,double)); break;
    case 149: ag_rp_149(); break;
    case 150: ag_rp_150(); break;
    case 151: ag_rp_151(); break;
    case 152: ag_rp_152(); break;
    case 153: ag_rp_153(); break;
    case 154: ag_rp_154(); break;
    case 155: ag_rp_155(); break;
    case 156: ag_rp_156(); break;
    case 157: ag_rp_157(); break;
    case 158: ag_rp_158(); break;
    case 159: ag_rp_159(); break;
    case 160: ag_rp_160(); break;
    case 161: V(0,double) = ag_rp_161(V(0,int)); break;
    case 162: V(0,double) = ag_rp_162(V(0,double)); break;
    case 163: V(0,int) = ag_rp_163(V(0,int)); break;
    case 164: V(0,int) = ag_rp_164(); break;
    case 165: V(0,int) = ag_rp_165(); break;
    case 166: V(0,int) = ag_rp_166(); break;
    case 167: V(0,int) = ag_rp_167(); break;
    case 168: V(0,int) = ag_rp_168(); break;
    case 169: V(0,int) = ag_rp_169(); break;
    case 170: V(0,int) = ag_rp_170(); break;
    case 171: V(0,int) = ag_rp_171(); break;
    case 172: V(0,int) = ag_rp_172(); break;
    case 173: ag_rp_173(V(1,int)); break;
    case 174: ag_rp_174(V(1,int)); break;
    case 175: ag_rp_175(V(0,int)); break;
    case 176: ag_rp_176(V(1,int)); break;
    case 177: ag_rp_177(V(2,int)); break;
    case 178: ag_rp_178(V(1,int)); break;
    case 179: ag_rp_179(V(1,int)); break;
    case 180: ag_rp_180(V(1,int)); break;
    case 181: ag_rp_181(V(1,int)); break;
    case 182: ag_rp_182(V(1,int)); break;
    case 183: ag_rp_183(V(1,int)); break;
    case 184: ag_rp_184(V(1,int)); break;
    case 185: ag_rp_185(V(1,int)); break;
    case 186: V(0,int) = ag_rp_186(V(1,int)); break;
    case 187: V(0,int) = ag_rp_187(V(1,int), V(2,int)); break;
    case 188: V(0,int) = ag_rp_188(); break;
    case 189: V(0,int) = ag_rp_189(V(0,int)); break;
    case 190: V(0,int) = ag_rp_190(); break;
    case 191: V(0,int) = ag_rp_191(); break;
    case 192: V(0,int) = ag_rp_192(); break;
    case 193: V(0,int) = ag_rp_193(); break;
    case 194: V(0,int) = ag_rp_194(); break;
    case 195: V(0,int) = ag_rp_195(); break;
    case 196: V(0,int) = ag_rp_196(); break;
    case 197: V(0,double) = ag_rp_197(); break;
    case 198: V(0,double) = ag_rp_198(V(1,int)); break;
    case 199: V(0,double) = ag_rp_199(V(0,double), V(1,int)); break;
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
    case 278: ag_rp_278(); break;
    case 279: ag_rp_279(); break;
    case 280: ag_rp_280(); break;
    case 281: ag_rp_281(); break;
    case 282: ag_rp_282(); break;
    case 283: ag_rp_283(); break;
    case 284: ag_rp_284(V(2,int)); break;
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
    case 331: ag_rp_331(); break;
    case 332: ag_rp_332(); break;
    case 333: ag_rp_333(); break;
    case 334: ag_rp_334(); break;
    case 335: ag_rp_335(); break;
    case 336: ag_rp_336(); break;
  }
}

#define TOKEN_NAMES a85parse_token_names
const char *const a85parse_token_names[706] = {
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
  "\"=\"",
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
  "\"ECHO\"",
  "\"PRINT\"",
  "\"PRINTF\"",
  "','",
  "\"UNDEF\"",
  "\"DEFINE\"",
  "macro",
  "macro expansion",
  "'('",
  "",
  "')'",
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
  "\"FILL\"",
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
  "'\\n'",
  "\"//\"",
  "';'",
  "\"*/\"",
  "\"/*\"",
  "label",
  "'.'",
  "\"EQU\"",
  "\"SET\"",
  "\"=\"",
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
  "\"ECHO\"",
  "\"PRINT\"",
  "','",
  "\"PRINTF\"",
  "\"UNDEF\"",
  "\"DEFINE\"",
  "')'",
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
  "\"FILL\"",
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
    0,0,  0,0,  0,0, 65,1,428,1, 53,1, 35,1, 35,1, 35,1, 35,1, 35,1,  0,0,
   67,1, 67,1,  0,0,  0,0,  0,0, 67,1,  0,0,  0,0, 67,1, 67,1,  0,0, 67,1,
   67,1,  0,0, 67,1, 67,1, 67,1, 67,1, 67,1, 67,1, 67,1, 67,1, 67,1, 67,1,
   67,1, 67,1, 67,1, 67,1,  0,0,  0,0, 35,2, 47,1,  0,0, 39,1, 40,1, 39,1,
   39,1, 35,2, 63,1,266,1,266,1,266,1,266,1,266,1,266,1,266,1,266,1,266,1,
  266,1,266,1,266,1,266,1,266,1,342,1,340,1,338,1,261,1,261,1,261,1,261,1,
  261,1,261,1,261,1,261,1,261,1,261,1,261,1,261,1,261,1,261,1,261,1,261,1,
  266,1,266,1,266,1,266,1,266,1,266,1,266,1,266,1,266,1,266,1,266,1,266,1,
  266,1,266,1,266,1,266,1,266,1,266,1,266,1,266,1,266,1,266,1,266,1,266,1,
  266,1,266,1,266,1,266,1,266,1,265,1,342,1,340,1,338,1,261,1,261,1,261,1,
  261,1,261,1,261,1,261,1,261,1,261,1,261,1,261,1,261,1,261,1,261,1,261,1,
  261,1,261,1,261,1,261,1,261,1,261,1,261,1,261,1,261,1,261,1,261,1,261,1,
  261,1,261,1,261,1,261,1,261,1,261,1,261,1,261,1,261,1,261,1,261,1,261,1,
  261,1,261,1,261,1,261,1,261,1,261,1,261,1,261,1,261,1,260,1,260,1,260,1,
  260,1,259,1,259,1,258,1,258,1,258,1,257,1,257,1,257,1,257,1,257,1,257,1,
  257,1,257,1,257,1,257,1,257,1,257,1,257,1,265,1,264,1,263,1,262,1,261,1,
  261,1,261,1,261,1,261,1,261,1,261,1,261,1,261,1,261,1,261,1,261,1,261,1,
  261,1,261,1,261,1,261,1,261,1,261,1,261,1,261,1,261,1,261,1,261,1,261,1,
  261,1,261,1,261,1,261,1,261,1,261,1,261,1,261,1,261,1,261,1,261,1,261,1,
  261,1,261,1,261,1,261,1,261,1,261,1,261,1,261,1,261,1,261,1,261,1,261,1,
  261,1,261,1,261,1,260,1,260,1,260,1,260,1,259,1,259,1,258,1,258,1,258,1,
  257,1,257,1,257,1,257,1,257,1,257,1,257,1,257,1,257,1,257,1,257,1,257,1,
  257,1,  0,0, 32,1, 89,1, 30,1, 32,1, 33,1, 33,1, 35,2, 67,2, 67,2, 67,2,
   67,2, 67,2,435,1,  0,0, 67,2, 67,2, 67,2, 67,2,538,1,219,1,218,1,  6,1,
    6,1,  7,1, 15,1,538,1,492,1,219,1,218,1,  6,1,206,1,207,1,  0,0,207,1,
  207,1,207,1,207,1,207,1,207,1,207,1,207,1,207,1,206,1,  0,0,  0,0,207,1,
  207,1,207,1,207,1,207,1,207,1,207,1,207,1,207,1,207,1,207,1, 29,1,  0,0,
   29,1,  0,0,198,1, 29,1, 29,1, 29,1, 29,1, 25,1, 27,1, 24,1, 23,1, 28,1,
   26,1, 26,1, 77,1, 77,1, 67,2, 67,2, 67,2,  0,0, 67,2,  0,0, 67,2,  0,0,
   67,2,  0,0, 67,2,  0,0, 67,2,436,1,  0,0, 67,2, 67,2,  0,0, 35,3,  0,0,
    0,0,  0,0,  0,0,  0,0,  0,0,  0,0,  0,0,  0,0,  0,0,  0,0,  0,0,  0,0,
    0,0,  0,0,  0,0,  0,0,  0,0,  0,0,  0,0,  0,0,  0,0,  0,0,  0,0,  0,0,
    0,0,  0,0,  0,0,  0,0,  0,0,  0,0,  0,0,  0,0,  0,0,  0,0,  0,0,  0,0,
    0,0, 39,2, 39,2,  0,0, 35,3, 33,1, 35,3, 63,2,342,2,340,2,338,2,265,2,
  264,2,263,2,262,2,261,2,261,2,261,2,261,2,261,2,261,2,261,2,261,2,261,2,
  261,2,261,2,261,2,261,2,261,2,261,2,261,2,261,2,261,2,261,2,261,2,261,2,
  261,2,261,2,261,2,261,2,261,2,261,2,261,2,261,2,261,2,261,2,261,2,261,2,
  261,2,261,2,261,2,261,2,261,2,261,2,261,2,261,2,261,2,261,2,261,2,261,2,
  261,2,261,2,261,2,261,2,261,2,261,2,261,2,260,2,260,2,260,2,260,2,259,2,
  259,2,258,2,258,2,258,2,257,2,257,2,257,2,257,2,257,2,257,2,257,2,257,2,
  257,2,257,2,257,2,257,2,257,2, 89,2,  0,0,  0,0, 32,2,  0,0, 33,2,  0,0,
   33,2,  0,0,  0,0,  0,0, 33,2,  0,0,  0,0,  0,0, 33,2, 33,2, 33,2,  0,0,
   33,2, 33,2,  0,0, 33,2,  0,0, 33,2,  0,0, 33,2,  0,0, 33,2,  0,0, 33,2,
    0,0, 33,2,  0,0,  0,0, 33,2,  0,0,  0,0,  0,0, 33,2,  0,0,  0,0, 33,2,
    0,0,  0,0,  0,0,  0,0, 33,2, 33,2,  0,0,  0,0, 35,3, 67,3, 67,3, 67,3,
   67,3, 67,3, 67,3,  0,0, 67,3, 67,3,  6,2, 16,1, 15,2,  0,0,  0,0,207,2,
  207,2,207,2,207,2,207,2,207,2,207,2,207,2,207,2,207,2,207,2,198,2,  7,1,
    0,0, 25,2,  0,0, 25,2, 25,2, 25,2, 25,2, 27,2, 27,2,  0,0, 24,2,  0,0,
   24,2,  0,0, 24,2,  0,0, 24,2,  0,0, 23,2, 23,2,  0,0, 28,2, 28,2,  0,0,
   26,2, 26,2,  0,0,  0,0,  0,0,  0,0,  0,0,  0,0,  0,0,  0,0,  0,0,  0,0,
    0,0,  0,0, 67,3, 67,3, 67,3, 67,3, 67,3, 67,3, 67,3, 67,3,436,2, 67,3,
   67,3,  0,0, 35,4, 35,4, 35,4, 35,4,  0,0, 35,4,  0,0, 35,4, 35,4, 22,1,
   63,3,254,1,254,1,254,1,  0,0,  0,0,264,3,263,3,262,3,251,1,251,1,257,3,
    0,0,485,1,  0,0,  0,0,  6,1,492,1, 32,3,485,1, 33,3, 33,3, 33,3,116,1,
  116,1,116,1,  0,0,  0,0, 90,1, 67,4, 67,4, 67,4, 67,4,  0,0,  0,0,  0,0,
   15,3,207,3,207,3,207,3,207,3,207,3,207,3,207,3,207,3,207,3,207,3,207,3,
  198,3, 25,3, 25,3, 25,3, 25,3, 25,3, 27,3, 27,3, 24,3, 24,3, 24,3, 24,3,
   23,3, 23,3, 28,3, 28,3, 26,3, 26,3, 67,4, 35,5, 35,5, 35,5, 35,5, 35,5,
   35,5, 63,4,264,4,263,4,262,4,257,4,  0,0,  0,0,  0,0, 32,1, 32,1,485,2,
   33,4,  0,0,116,2,  0,0,  0,0, 67,5, 67,5,  0,0,  0,0,207,4,207,4,207,4,
  207,4,207,4,207,4,207,4,207,4,207,4,207,4,207,4,198,4, 25,1, 25,1, 27,1,
   27,1, 27,1, 27,1, 24,1, 24,1, 23,1, 23,1, 28,1, 28,1, 67,5, 35,6, 35,6,
  257,5, 15,3,  0,0, 33,5,116,3, 67,6,536,1,  0,0,207,5,257,6, 67,7,207,6
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
    while (ag_tstt[ag_k] != 64 && ag_tstt[ag_k]) ag_k++;
    if (ag_tstt[ag_k] || (PCB).ssx == 0) break;
    (PCB).sn = (PCB).ss[--(PCB).ssx];
  }
  if (ag_tstt[ag_k] == 0) {
    (PCB).sn = PCB.ss[(PCB).ssx = ag_ssx];
    (PCB).exit_flag = AG_SYNTAX_ERROR_CODE;
    return;
  }
  ag_k = ag_sbt[(PCB).sn];
  while (ag_tstt[ag_k] != 64 && ag_tstt[ag_k]) ag_k++;
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
  while (ag_tstt[ag_k] != 64 && ag_tstt[ag_k]) ag_k++;
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


