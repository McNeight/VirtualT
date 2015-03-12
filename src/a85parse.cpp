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

#ifndef A85PARSE_H_1425877737
#include "a85parse.h"
#endif

#ifndef A85PARSE_H_1425877737
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

/*  Line 707, C:/Projects/VirtualT/src/a85parse.syn */
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
/* Line 254, C:/Projects/VirtualT/src/a85parse.syn */
 strcpy(ss[++ss_idx], "&"); ss_len = 1; 
}

static void ag_rp_86(int c) {
/* Line 257, C:/Projects/VirtualT/src/a85parse.syn */
 ss[++ss_idx][0] = c; ss[ss_idx][1] = 0; ss_len = 1; \
														if (PCB.column == 2) ss_addr = gAsm->m_ActiveSeg->m_Address; 
}

static void ag_rp_87(int c) {
/* Line 259, C:/Projects/VirtualT/src/a85parse.syn */
 ss[ss_idx][ss_len++] = c; ss[ss_idx][ss_len] = 0; 
}

static void ag_rp_88(int c) {
/* Line 260, C:/Projects/VirtualT/src/a85parse.syn */
 ss[ss_idx][ss_len++] = c; ss[ss_idx][ss_len] = 0; 
}

static void ag_rp_89(int c) {
/* Line 263, C:/Projects/VirtualT/src/a85parse.syn */
 ss[++ss_idx][0] = c; ss[ss_idx][1] = 0; ss_len = 1; 
}

static void ag_rp_90(int c) {
/* Line 264, C:/Projects/VirtualT/src/a85parse.syn */
 ss[ss_idx][ss_len++] = c; ss[ss_idx][ss_len] = 0; 
}

static void ag_rp_91(int ch1, int ch2) {
/* Line 270, C:/Projects/VirtualT/src/a85parse.syn */
 ss_idx++; ss_len = 2; sprintf(ss[ss_idx], "%c%c", ch1, ch2); 
}

static void ag_rp_92(int c) {
/* Line 271, C:/Projects/VirtualT/src/a85parse.syn */
 ss[ss_idx][ss_len++] = c; ss[ss_idx][ss_len] = 0; 
}

static void ag_rp_93(void) {
/* Line 278, C:/Projects/VirtualT/src/a85parse.syn */
 ss_idx++; ss_len = 0; 
}

static void ag_rp_94(int c) {
/* Line 279, C:/Projects/VirtualT/src/a85parse.syn */
 ss[ss_idx][ss_len++] = c; ss[ss_idx][ss_len] = 0; 
}

#define ag_rp_95(n) (n)

#define ag_rp_96() ('\\')

#define ag_rp_97() ('\n')

#define ag_rp_98() ('\t')

#define ag_rp_99() ('\r')

#define ag_rp_100() ('\0')

#define ag_rp_101() ('"')

#define ag_rp_102() (0x08)

#define ag_rp_103() (0x0C)

#define ag_rp_104(n1, n2, n3) ((n1-'0') * 64 + (n2-'0') * 8 + n3-'0')

#define ag_rp_105(n1, n2) (chtoh(n1) * 16 + chtoh(n2))

#define ag_rp_106(n1) (chtoh(n1))

static void ag_rp_107(void) {
/* Line 299, C:/Projects/VirtualT/src/a85parse.syn */
 ss[ss_idx][ss_len++] = '>'; ss[ss_idx][ss_len] = 0; 
}

static void ag_rp_108(void) {
/* Line 302, C:/Projects/VirtualT/src/a85parse.syn */
 ss[++ss_idx][0] = '<'; ss[ss_idx][1] = 0; ss_len = 1; 
}

static void ag_rp_109(int c) {
/* Line 303, C:/Projects/VirtualT/src/a85parse.syn */
 ss[ss_idx][ss_len++] = c; ss[ss_idx][ss_len] = 0; 
}

static void ag_rp_110(int c) {
/* Line 304, C:/Projects/VirtualT/src/a85parse.syn */
 ss[ss_idx][ss_len++] = '\\'; ss[ss_idx][ss_len++] = c; ss[ss_idx][ss_len] = 0; 
}

#define ag_rp_111() (gAsm->label(ss[ss_idx--]))

#define ag_rp_112() (gAsm->label(ss[ss_idx--]))

#define ag_rp_113() (gAsm->label(".bss"))

#define ag_rp_114() (gAsm->label(".text"))

#define ag_rp_115() (gAsm->label(".data"))

#define ag_rp_116() (PAGE)

#define ag_rp_117() (INPAGE)

#define ag_rp_118() (condition(-1))

#define ag_rp_119() (condition(COND_NOCMP))

#define ag_rp_120() (condition(COND_EQ))

#define ag_rp_121() (condition(COND_NE))

#define ag_rp_122() (condition(COND_GE))

#define ag_rp_123() (condition(COND_LE))

#define ag_rp_124() (condition(COND_GT))

#define ag_rp_125() (condition(COND_LT))

#define ag_rp_126() (gEq->Add(RPN_BITOR))

#define ag_rp_127() (gEq->Add(RPN_BITOR))

#define ag_rp_128() (gEq->Add(RPN_BITXOR))

#define ag_rp_129() (gEq->Add(RPN_BITXOR))

#define ag_rp_130() (gEq->Add(RPN_BITAND))

#define ag_rp_131() (gEq->Add(RPN_BITAND))

#define ag_rp_132() (gEq->Add(RPN_LEFTSHIFT))

#define ag_rp_133() (gEq->Add(RPN_LEFTSHIFT))

#define ag_rp_134() (gEq->Add(RPN_RIGHTSHIFT))

#define ag_rp_135() (gEq->Add(RPN_RIGHTSHIFT))

#define ag_rp_136() (gEq->Add(RPN_ADD))

#define ag_rp_137() (gEq->Add(RPN_SUBTRACT))

#define ag_rp_138() (gEq->Add(RPN_MULTIPLY))

#define ag_rp_139() (gEq->Add(RPN_DIVIDE))

#define ag_rp_140() (gEq->Add(RPN_MODULUS))

#define ag_rp_141() (gEq->Add(RPN_MODULUS))

#define ag_rp_142() (gEq->Add(RPN_EXPONENT))

#define ag_rp_143() (gEq->Add(RPN_NOT))

#define ag_rp_144() (gEq->Add(RPN_NOT))

#define ag_rp_145() (gEq->Add(RPN_BITNOT))

#define ag_rp_146() (gEq->Add(RPN_NEGATE))

#define ag_rp_147(n) (gEq->Add((double) n))

static void ag_rp_148(void) {
/* Line 395, C:/Projects/VirtualT/src/a85parse.syn */
 delete gMacro; gMacro = gMacroStack[ms_idx-1]; \
														gMacroStack[ms_idx--] = 0; if (gMacro->m_ParamList == 0) \
														{\
															gEq->Add(gMacro->m_Name); gMacro->m_Name = ""; }\
														else { \
															gEq->Add((VTObject *) gMacro); gMacro = new CMacro; \
														} 
}

#define ag_rp_149() (gEq->Add("$"))

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
/* Line 443, C:/Projects/VirtualT/src/a85parse.syn */
 int_len = 1; integer[0] = n; integer[1] = 0; 
}

static void ag_rp_174(int n) {
/* Line 444, C:/Projects/VirtualT/src/a85parse.syn */
 int_len = 2; integer[0] = '-', integer[1] = n; integer[2] = 0; 
}

static void ag_rp_175(int n) {
/* Line 445, C:/Projects/VirtualT/src/a85parse.syn */
 int_len = 1; integer[0] = n; integer[1] = 0; 
}

static void ag_rp_176(int n) {
/* Line 446, C:/Projects/VirtualT/src/a85parse.syn */
 integer[int_len++] = n; integer[int_len] = 0; 
}

static void ag_rp_177(int n) {
/* Line 451, C:/Projects/VirtualT/src/a85parse.syn */
 int_len = 1; integer[0] = n; integer[1] = 0; 
}

static void ag_rp_178(int n) {
/* Line 452, C:/Projects/VirtualT/src/a85parse.syn */
 int_len = 1; integer[0] = n; integer[1] = 0; 
}

static void ag_rp_179(int n) {
/* Line 453, C:/Projects/VirtualT/src/a85parse.syn */
 integer[int_len++] = n; integer[int_len] = 0; 
}

static void ag_rp_180(int n) {
/* Line 456, C:/Projects/VirtualT/src/a85parse.syn */
 integer[int_len++] = n; integer[int_len] = 0; 
}

static void ag_rp_181(int n) {
/* Line 457, C:/Projects/VirtualT/src/a85parse.syn */
 integer[int_len++] = n; integer[int_len] = 0; 
}

static void ag_rp_182(int n) {
/* Line 460, C:/Projects/VirtualT/src/a85parse.syn */
 integer[int_len++] = n; integer[int_len] = 0; 
}

static void ag_rp_183(int n) {
/* Line 461, C:/Projects/VirtualT/src/a85parse.syn */
 integer[int_len++] = n; integer[int_len] = 0; 
}

static void ag_rp_184(int n) {
/* Line 464, C:/Projects/VirtualT/src/a85parse.syn */
 integer[int_len++] = n; integer[int_len] = 0; 
}

static void ag_rp_185(int n) {
/* Line 465, C:/Projects/VirtualT/src/a85parse.syn */
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
/* Line 486, C:/Projects/VirtualT/src/a85parse.syn */
 gDivisor = 1.0; return (double) conv_to_dec(); 
}

static double ag_rp_198(int d) {
/* Line 487, C:/Projects/VirtualT/src/a85parse.syn */
 gDivisor = 10.0; return ((double) (d - '0') / gDivisor); 
}

static double ag_rp_199(double r, int d) {
/* Line 488, C:/Projects/VirtualT/src/a85parse.syn */
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
   78, 79, 80, 81, 82, 83, 84,  0,  0, 85, 86, 87, 88, 89, 90,  0, 91, 92,
    0, 93, 94,  0, 95, 96, 97, 98, 99,100,101,102,103,104,  0,  0,105,106,
  107,108,109,110,  0,111,112,113,114,115,116,117,118,119,  0,  0,  0,120,
    0,  0,121,  0,  0,122,  0,  0,123,  0,  0,124,  0,  0,125,  0,  0,126,
  127,  0,128,129,  0,130,131,  0,132,133,134,135,  0,136,137,  0,138,139,
  140,141,142,  0,143,144,145,146,147,148,149,  0,  0,150,151,152,153,154,
  155,156,157,158,159,160,161,162,163,164,165,166,167,168,169,170,  0,  0,
  171,172,173,174,175,176,  0,177,178,179,180,181,182,183,184,185,186,187,
  188,189,190,191,192,193,194,195,196,  0,197,198,199,200,201,202,203,204,
  205,  0,  0,206,207,  0,  0,208,  0,  0,209,  0,  0,210,211,212,213,214,
  215,216,217,218,219,220,221,222,223,224,  0,  0,  0,  0,  0,  0,  0,  0,
    0,  0,  0,  0,  0,  0,  0,  0,225,226,227,228,229,  0,  0,230,231,232,
    0,  0,  0,233,234,235,236,237,238,239,240,241,  0,  0,242,243,244,245,
  246,247,248,249,250,251,  0,  0,  0,252,253,254,255,256,257,258,259,260,
  261,262,263,264,265,266,267,268,269,270,271,272,273,274,275,276,277,278,
  279,280,  0,281,  0,282,  0,283,284,285,286,287,288,289,290,291,292,293,
  294,295,296,297,298,299,  0,  0,  0,  0,  0,300,301,302,303,  0,  0,  0,
    0,  0,304,305,306,307,308,309,310,  0,  0,  0,311,  0,  0,  0,312,313,
  314,315,316,317,318,319,320,321,322,323,324,325,326,327,328,329,330,331,
  332,333,334,335,336
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
  1,429,  1,430,  1,432,  1,433,  1,439,  1,438,  1,440,  1,442,
  1,443,  1,444,  1,445,  1,446,  1,447,  1,448,  1,449,  1,450,
  1,451,  1,452,  1,453,  1,455,  1,456,  1,457,  1,461,  1,462,
  1,463,  1,466,  1,467,  1,468,  1,469,  1,470,  1,471,  1,472,
  1,473,  1,474,  1,475,  1,476,  1,477,  1,478,  1,479,  1,480,
  1,481,  1,482,  1,483,  1,485,  1,486,  1,487,  1,488,  1,489,
  1,490,  1,493,  1,494,  1,496,  1,498,  1,500,  1,502,  1,504,
  1,507,  1,509,  1,511,  1,513,  1,515,  1,521,  1,524,  1,527,
  1,528,  1,529,  1,530,  1,531,  1,532,  1,533,  1,534,  1,535,
  1,537,  1,220,  1,221,  1,233,  1,234,  1,235,  1,236,  1,237,
  1,238,  1,239,  1,540,  1,668,  1,245,  1,247,  1,543,  1,539,
  1,541,  1,542,  1,544,  1,545,  1,546,  1,547,  1,548,  1,549,
  1,550,  1,551,  1,552,  1,553,  1,554,  1,555,  1,556,  1,557,
  1,558,  1,559,  1,560,  1,561,  1,562,  1,563,  1,564,  1,565,
  1,566,  1,567,  1,568,  1,569,  1,570,  1,571,  1,572,  1,573,
  1,574,  1,575,  1,576,  1,577,  1,578,  1,579,  1,580,  1,581,
  1,582,  1,583,  1,584,  1,585,  1,586,  1,587,  1,588,  1,589,
  1,590,  1,591,  1,592,  1,593,  1,595,  1,596,  1,597,  1,598,
  1,599,  1,600,  1,601,  1,602,  1,603,  1,604,  1,605,  1,606,
  1,607,  1,609,  1,610,  1,611,  1,612,  1,613,  1,614,  1,615,
  1,616,  1,617,  1,618,  1,619,  1,620,  1,621,  1,622,  1,623,
  1,624,  1,625,  1,626,  1,627,  1,628,  1,629,  1,630,  1,631,
  1,632,  1,633,  1,634,  1,635,  1,636,  1,637,  1,638,  1,639,
  1,640,  1,641,  1,642,  1,643,  1,644,  1,645,  1,646,  1,647,
  1,648,  1,649,  1,650,  1,651,  1,652,  1,653,  1,654,  1,655,
  1,656,  1,657,  1,658,  1,659,  1,660,  1,661,  1,662,  1,663,
  1,664,  1,665,  1,666,  1,667,  1,669,  1,670,  1,671,  1,672,
  1,673,  1,674,  1,675,  1,676,  1,677,  1,678,  1,679,0
};

static const unsigned char ag_key_ch[] = {
    0, 66, 68, 84,255, 42, 47,255, 85,255, 76,255, 67,255, 78,255, 35, 38,
   46, 47, 73,255, 61,255, 42, 47, 61,255, 66, 68, 84,255, 42, 47,255, 60,
   61,255, 61,255, 61, 62,255, 67, 68, 73,255, 65, 68, 73,255, 69, 72,255,
   67, 68, 78, 82, 83,255, 67, 90,255, 69,255, 65,255, 67, 76, 77, 78, 80,
   82, 89, 90,255, 65, 67, 80,255, 67, 90,255, 69, 73, 79,255, 65, 67, 69,
   77, 78, 80, 83, 90,255, 65, 68,255, 82, 88,255, 68,255, 69,255, 78,255,
   73,255, 70, 72, 83,255, 69, 85,255, 65, 66, 67, 69, 73, 83, 87, 88,255,
   73, 83,255, 73,255, 68, 84,255, 85,255, 78, 82,255, 69, 82,255, 84,255,
   67, 73, 76, 78, 81, 82, 88,255, 73, 76, 80,255, 69, 84,255, 77, 84,255,
   69, 73, 76,255, 68, 78,255, 85,255, 76,255, 67, 80, 82, 88,255, 70, 78,
   80,255, 80,255, 53,255, 67, 68, 75, 88, 90,255, 69, 79,255, 77, 80,255,
   53,255, 67, 68, 75, 77, 78, 80, 84, 88, 90,255, 88,255, 72, 83,255, 65,
   69, 72, 83,255, 69,255, 68, 73, 88,255, 76,255, 78, 83,255, 71, 87,255,
   79, 85,255, 67, 68, 69, 72, 73, 74, 78, 79, 80, 82, 83, 84, 88,255, 85,
  255, 68, 86,255, 65, 79, 83, 86,255, 65,255, 80, 84,255, 65, 69, 79,255,
   65, 71, 73,255, 82, 85,255, 72,255, 70,255, 84,255, 78,255, 65, 73,255,
   66, 83,255, 65, 67, 79, 82, 83, 85,255, 76, 82,255, 65,255, 67, 68,255,
   67, 90,255, 69, 71, 79,255, 67, 72,255, 86,255, 84,255, 65, 67, 68, 69,
   73, 76, 77, 78, 80, 82, 83, 90,255, 67, 90,255, 66, 67, 73, 78, 90,255,
   69,255, 68, 73, 82, 88,255, 76, 82,255, 71, 72, 73,255, 88,255, 65, 67,
   75,255, 66, 73,255, 66, 69, 72, 73, 80, 81, 84, 85, 89,255, 69, 73,255,
   65, 73,255, 67, 79, 82, 84,255, 33, 35, 36, 38, 39, 40, 42, 44, 46, 47,
   60, 61, 62, 65, 66, 67, 68, 69, 70, 71, 72, 73, 74, 76, 77, 78, 79, 80,
   82, 83, 84, 85, 86, 87, 88, 92,255, 66, 68, 84,255, 42, 47,255, 35, 38,
   46, 47,255, 73, 83,255, 67, 76, 78, 82,255, 68, 78,255, 70, 78,255, 70,
  255, 84,255, 78,255, 65, 73,255, 82,255, 68, 69, 73, 80, 85,255, 42, 47,
  255, 36, 38, 42, 47, 58, 61,255, 42, 47,255, 67, 68, 73,255, 65, 73,255,
   69, 72,255, 67, 68, 78, 82, 83,255, 67, 90,255, 69,255, 65,255, 67, 76,
   77, 78, 80, 82, 89, 90,255, 65, 67, 80,255, 67, 90,255, 69, 73, 79,255,
   65, 67, 77, 78, 80, 83, 90,255, 65, 68,255, 82, 88,255, 72, 83,255, 69,
   85,255, 65, 66, 67, 69, 73, 83, 87,255, 73,255, 68, 84,255, 78, 82,255,
   69, 82,255, 84,255, 67, 73, 76, 78, 81, 88,255, 77, 84,255, 69, 76,255,
   82, 88,255, 70, 78,255, 80,255, 53,255, 67, 68, 75, 88, 90,255, 69, 79,
  255, 77, 80,255, 53,255, 67, 68, 75, 77, 78, 80, 84, 88, 90,255, 88,255,
   72, 83,255, 65, 69, 72, 83,255, 69,255, 68, 73, 88,255, 76,255, 78, 83,
  255, 79, 85,255, 67, 68, 72, 73, 74, 80, 82, 83, 88,255, 68, 86,255, 65,
   79, 83, 86,255, 65,255, 80,255, 65, 79,255, 65, 71, 73,255, 82, 85,255,
   70,255, 84,255, 78,255, 73,255, 66, 83,255, 65, 67, 79, 82, 85,255, 76,
   82,255, 65,255, 67, 68,255, 67, 90,255, 69, 71, 79,255, 67, 72,255, 86,
  255, 84,255, 65, 67, 68, 69, 73, 76, 77, 78, 80, 82, 83, 90,255, 67, 90,
  255, 66, 67, 73, 78, 90,255, 69,255, 68, 73, 82, 88,255, 76,255, 71, 72,
   73,255, 88,255, 65, 67, 75,255, 66, 73,255, 66, 69, 72, 73, 80, 84, 85,
   89,255, 69, 73,255, 65, 73,255, 67, 82, 84,255, 36, 38, 42, 47, 61, 65,
   66, 67, 68, 69, 70, 72, 73, 74, 76, 77, 78, 79, 80, 82, 83, 84, 86, 87,
   88,255, 42, 47,255, 85,255, 76,255, 67,255, 78,255, 47, 73,255, 42, 47,
  255, 67, 68, 73,255, 65, 73,255, 69, 72,255, 67, 68, 78, 82, 83,255, 67,
   90,255, 69,255, 65,255, 67, 76, 77, 78, 80, 82, 89, 90,255, 65, 67, 80,
  255, 67, 90,255, 69, 73, 79,255, 65, 67, 77, 78, 80, 83, 90,255, 65, 68,
  255, 82, 88,255, 72, 83,255, 69, 85,255, 65, 66, 67, 69, 73, 83, 87,255,
   73,255, 68, 84,255, 78, 82,255, 69, 82,255, 84,255, 67, 73, 76, 78, 88,
  255, 77, 84,255, 69, 76,255, 85,255, 76,255, 67, 82, 88,255, 70, 78,255,
   80,255, 53,255, 67, 68, 75, 88, 90,255, 69, 79,255, 77, 80,255, 53,255,
   67, 68, 75, 77, 78, 80, 84, 88, 90,255, 88,255, 72, 83,255, 65, 69, 72,
   83,255, 69,255, 68, 73, 88,255, 76,255, 78, 83,255, 79, 85,255, 67, 68,
   72, 73, 74, 80, 82, 83, 88,255, 68, 86,255, 65, 79, 83, 86,255, 65,255,
   80,255, 65, 79,255, 65, 71, 73,255, 82, 85,255, 70,255, 84,255, 78,255,
   73,255, 66, 83,255, 65, 67, 79, 82, 85,255, 76, 82,255, 65,255, 67, 68,
  255, 67, 90,255, 69, 71, 79,255, 67, 72,255, 86,255, 84,255, 65, 67, 68,
   69, 73, 76, 77, 78, 80, 82, 83, 90,255, 67, 90,255, 66, 67, 73, 78, 90,
  255, 69,255, 68, 73, 82, 88,255, 76,255, 71, 72, 73,255, 88,255, 65, 67,
   75,255, 66, 73,255, 66, 72, 73, 80, 84, 85, 89,255, 69, 73,255, 65, 73,
  255, 67, 82, 84,255, 36, 38, 42, 47, 65, 66, 67, 68, 69, 70, 72, 73, 74,
   76, 77, 78, 79, 80, 82, 83, 84, 86, 87, 88,255, 38,255, 76, 80,255, 71,
   87,255, 78, 79,255, 38, 39, 67, 68, 70, 72, 73, 76, 78, 80, 83,255, 42,
   47,255, 47,255, 78, 88,255, 69, 72, 76, 86,255, 66, 68, 84,255, 42, 47,
  255, 85,255, 76,255, 67,255, 78,255, 35, 38, 42, 46, 47, 73,255, 47, 61,
  255, 42, 47,255, 76, 89,255, 69,255, 66, 83, 87,255, 68, 84,255, 78, 82,
  255, 69, 82,255, 84,255, 67, 78, 81, 88,255, 85,255, 76,255, 67,255, 78,
  255, 78, 83,255, 73, 83,255, 65, 79, 83,255, 65, 79,255, 70,255, 84,255,
   78,255, 73,255, 65, 82, 85,255, 69, 84, 89,255, 69, 73,255, 36, 42, 47,
   65, 66, 67, 68, 69, 70, 72, 73, 76, 77, 78, 79, 80, 83, 84, 86, 87, 92,
  255, 85,255, 76,255, 67,255, 78,255, 73,255, 42, 47,255, 42, 47,255, 42,
   47, 92,255, 42, 47,255, 47, 73, 80,255, 42, 47,255, 33, 47,255, 67,255,
   69, 88,255, 76,255, 66, 68, 72, 80, 83,255, 40, 65, 66, 67, 68, 69, 72,
   76, 77,255, 67,255, 69,255, 76,255, 66, 68, 72, 83,255, 67,255, 69,255,
   76,255, 65, 66, 68, 72, 80,255, 67,255, 69,255, 66, 68,255, 61,255, 42,
   47,255, 60, 61,255, 61,255, 61, 62,255, 33, 42, 44, 47, 60, 61, 62,255,
   61,255, 42, 47,255, 60, 61,255, 61,255, 61, 62,255, 69, 84,255, 69, 84,
  255, 76, 82,255, 72,255, 33, 42, 44, 47, 60, 61, 62, 65, 69, 71, 76, 77,
   78, 79, 83, 88,255, 76, 89,255, 69,255, 66, 83, 87,255, 68, 84,255, 78,
   82,255, 69, 82,255, 84,255, 67, 78, 88,255, 78, 83,255, 73, 83,255, 65,
   79, 83,255, 65, 79,255, 70,255, 84,255, 78,255, 73,255, 65, 82, 85,255,
   84, 89,255, 69, 73,255, 36, 42, 65, 66, 67, 68, 69, 70, 72, 76, 77, 78,
   79, 80, 83, 84, 86, 87,255, 42, 47,255, 44, 47,255, 61,255, 42, 47,255,
   60, 61,255, 61,255, 61, 62,255, 69, 84,255, 69, 84,255, 82,255, 76, 82,
  255, 72,255, 33, 42, 44, 47, 60, 61, 62, 65, 69, 71, 76, 77, 78, 79, 81,
   83, 88,255, 39,255, 61,255, 42, 47,255, 60, 61,255, 61,255, 61, 62,255,
   69, 84,255, 69, 84,255, 82,255, 76, 82,255, 72,255, 33, 42, 44, 47, 60,
   61, 62, 71, 76, 77, 78, 79, 81, 83, 88,255, 42, 47,255, 76, 80,255, 71,
   87,255, 78, 79,255, 38, 39, 42, 47, 67, 68, 70, 72, 73, 76, 78, 80, 83,
   92,255, 61,255, 42, 47,255, 60, 61,255, 61,255, 61, 62,255, 69, 84,255,
   69, 84,255, 76, 82,255, 72,255, 33, 42, 44, 47, 60, 61, 62, 71, 76, 77,
   78, 79, 83, 88,255, 76, 80,255, 71, 87,255, 78, 79,255, 38, 39, 67, 68,
   70, 72, 73, 76, 80, 83,255, 42, 47,255, 76, 80,255, 71, 87,255, 78, 79,
  255, 38, 39, 42, 47, 67, 68, 70, 72, 73, 76, 80, 83, 92,255, 61,255, 60,
   61,255, 61,255, 61, 62,255, 69, 84,255, 69, 84,255, 76, 82,255, 72,255,
   33, 42, 44, 60, 61, 62, 65, 69, 71, 76, 77, 78, 79, 83, 88,255, 61,255,
   42, 47,255, 60, 61,255, 61,255, 61, 62,255, 69, 84,255, 69, 84,255, 76,
   82,255, 72,255, 33, 44, 47, 60, 61, 62, 65, 69, 71, 76, 78, 79, 83, 88,
  255, 61,255, 42, 47,255, 61,255, 61,255, 61,255, 69, 84,255, 69, 84,255,
   33, 44, 47, 60, 61, 62, 65, 69, 71, 76, 78, 79, 88,255, 61,255, 42, 47,
  255, 61,255, 61,255, 61,255, 69, 84,255, 69, 84,255, 33, 44, 47, 60, 61,
   62, 69, 71, 76, 78, 79, 88,255, 61,255, 42, 47,255, 61,255, 61,255, 61,
  255, 69, 84,255, 69, 84,255, 33, 44, 47, 60, 61, 62, 69, 71, 76, 78, 79,
  255, 42, 47,255, 61,255, 61,255, 61,255, 69, 84,255, 69, 84,255, 33, 47,
   60, 61, 62, 69, 71, 76, 78,255, 61,255, 42, 47,255, 42, 47,255, 60, 61,
  255, 61,255, 61, 62,255, 69, 84,255, 69, 84,255, 76, 82,255, 72,255, 33,
   42, 44, 47, 60, 61, 62, 65, 69, 71, 76, 77, 78, 79, 83, 88, 92,255, 76,
   89,255, 69,255, 66, 83, 87,255, 68, 84,255, 78, 82,255, 69, 82,255, 84,
  255, 67, 78, 81, 88,255, 78, 83,255, 73, 83,255, 65, 79, 83,255, 65, 79,
  255, 70,255, 84,255, 78,255, 73,255, 65, 82, 85,255, 69, 84, 89,255, 69,
   73,255, 36, 42, 65, 66, 67, 68, 69, 70, 72, 76, 77, 78, 79, 80, 83, 84,
   86, 87,255, 39,255, 67, 68, 73,255, 65, 73,255, 67, 68, 78, 82, 83,255,
   67, 90,255, 69,255, 65,255, 67, 77, 78, 80, 82, 90,255, 65, 67, 80,255,
   67, 90,255, 69, 73, 79,255, 65, 67, 77, 78, 80, 90,255, 65, 68,255, 82,
   88,255, 72, 83,255, 65, 67, 69, 73, 83,255, 77, 84,255, 76,255, 82, 88,
  255, 78,255, 80,255, 53,255, 67, 68, 75, 88, 90,255, 69, 79,255, 77, 80,
  255, 53,255, 67, 68, 75, 77, 78, 80, 84, 88, 90,255, 88,255, 72, 83,255,
   65, 69, 72, 83,255, 69,255, 68, 73, 88,255, 76,255, 79, 85,255, 67, 68,
   72, 74, 80, 82, 88,255, 79, 86,255, 65, 73,255, 82, 85,255, 67, 79, 85,
  255, 76, 82,255, 65,255, 67, 68,255, 67, 90,255, 69, 71, 79,255, 67, 72,
  255, 86,255, 84,255, 65, 67, 68, 69, 73, 76, 77, 78, 80, 82, 83, 90,255,
   67, 90,255, 66, 67, 73, 78, 90,255, 69,255, 68, 73, 82, 88,255, 76,255,
   71, 72, 73,255, 88,255, 65, 67,255, 66, 73,255, 66, 72, 73, 80, 84, 85,
  255, 65, 73,255, 67, 82, 84,255, 65, 66, 67, 68, 69, 72, 73, 74, 76, 77,
   78, 79, 80, 82, 83, 88,255, 42, 47,255, 38, 47,255, 42, 47,255, 33, 44,
   47,255, 44,255, 42, 47,255, 47, 79, 81,255, 92,255, 69,255, 69,255, 76,
   80,255, 73,255, 71, 87,255, 78, 79,255, 38, 39, 40, 65, 66, 67, 68, 69,
   70, 72, 73, 76, 77, 78, 80, 83,255, 33,255, 42, 47,255, 47, 92,255
};

static const unsigned char ag_key_act[] = {
  0,3,3,3,4,0,0,4,7,4,6,4,2,4,2,4,0,0,2,2,2,4,0,4,0,0,0,4,3,3,3,4,0,0,4,
  0,0,4,0,4,0,0,4,5,5,5,4,5,5,5,4,7,7,4,7,2,2,7,2,4,5,5,4,5,4,5,4,5,7,5,
  2,6,6,7,5,4,5,5,5,4,5,5,4,5,5,5,4,7,5,7,6,2,6,7,5,4,5,5,4,5,5,4,5,4,6,
  4,2,4,2,4,2,7,7,4,7,7,4,2,5,2,6,5,6,5,5,4,7,7,4,7,4,6,7,4,5,4,7,7,4,2,
  7,4,2,4,7,5,2,2,6,7,2,4,7,7,5,4,5,5,4,7,5,4,7,7,6,4,7,7,4,7,4,6,4,2,7,
  5,5,4,6,6,5,4,5,4,5,4,5,5,5,6,5,4,5,5,4,5,5,4,5,4,5,5,5,6,2,6,2,6,5,4,
  5,4,5,5,4,6,2,7,7,4,5,4,6,5,5,4,2,4,7,7,4,5,5,4,7,7,4,7,2,5,2,2,7,5,2,
  2,7,7,5,7,4,7,4,6,5,4,7,2,7,7,4,7,4,6,5,4,7,5,2,4,5,5,5,4,6,7,4,7,4,5,
  4,6,4,2,4,7,2,4,7,7,4,7,6,7,2,7,2,4,5,5,4,7,4,5,7,4,5,5,4,5,5,5,4,5,7,
  4,5,4,6,4,2,6,7,7,7,2,5,2,6,2,2,5,4,5,5,4,5,5,5,2,5,4,5,4,6,5,5,5,4,6,
  5,4,5,7,5,4,5,4,6,5,7,4,5,5,4,2,7,2,7,6,7,2,2,7,4,7,7,4,5,5,4,7,7,2,7,
  4,6,0,3,0,3,3,2,0,2,2,1,1,1,6,6,6,6,6,2,2,6,2,2,6,6,2,2,2,2,2,2,7,7,7,
  2,3,4,3,3,3,4,0,0,4,0,0,2,2,4,7,7,4,7,2,7,7,4,7,7,4,6,7,4,5,4,6,4,2,4,
  7,2,4,2,4,7,2,2,2,7,4,0,0,4,3,0,3,2,0,0,4,0,0,4,5,5,5,4,5,5,4,7,7,4,7,
  2,2,7,2,4,5,5,4,5,4,5,4,5,7,5,2,6,6,7,5,4,5,5,5,4,5,5,4,5,5,5,4,7,5,6,
  2,6,7,5,4,5,5,4,5,5,4,7,7,4,7,7,4,2,5,2,2,5,6,5,4,7,4,6,7,4,7,7,4,2,7,
  4,2,4,7,5,7,2,7,2,4,7,5,4,7,2,4,5,5,4,5,6,4,5,4,5,4,5,5,5,6,5,4,5,5,4,
  5,5,4,5,4,5,5,5,6,2,6,2,6,5,4,5,4,5,5,4,6,2,7,7,4,5,4,6,5,5,4,2,4,7,7,
  4,7,7,4,7,2,2,2,7,2,7,7,7,4,7,5,4,7,2,7,7,4,7,4,6,4,7,2,4,5,5,5,4,2,7,
  4,5,4,6,4,2,4,2,4,7,7,4,7,7,7,2,2,4,5,5,4,7,4,5,7,4,5,5,4,5,5,5,4,5,7,
  4,5,4,6,4,2,6,7,7,7,2,5,2,6,2,2,5,4,5,5,4,5,5,5,2,5,4,5,4,6,5,5,5,4,2,
  4,5,7,5,4,5,4,6,5,7,4,5,5,4,2,7,2,7,2,2,2,7,4,7,7,4,5,5,4,7,2,7,4,3,0,
  3,2,0,2,2,2,2,2,7,2,2,2,2,2,2,2,2,2,2,2,7,7,2,4,0,0,4,7,4,6,4,2,4,2,4,
  2,2,4,0,0,4,5,5,5,4,5,5,4,7,7,4,7,2,2,7,2,4,5,5,4,5,4,5,4,5,7,5,2,6,6,
  7,5,4,5,5,5,4,5,5,4,5,5,5,4,7,5,6,2,6,7,5,4,5,5,4,5,5,4,7,7,4,7,7,4,2,
  5,2,2,5,6,5,4,7,4,6,7,4,7,7,4,2,7,4,2,4,7,5,7,2,2,4,7,5,4,7,2,4,7,4,6,
  4,2,5,5,4,5,6,4,5,4,5,4,5,5,5,6,5,4,5,5,4,5,5,4,5,4,5,5,5,6,2,6,2,6,5,
  4,5,4,5,5,4,6,2,7,7,4,5,4,6,5,5,4,2,4,7,7,4,7,7,4,7,2,2,2,7,2,7,7,7,4,
  7,5,4,7,2,7,7,4,7,4,6,4,7,2,4,5,5,5,4,2,7,4,5,4,6,4,2,4,2,4,7,7,4,7,7,
  7,2,2,4,5,5,4,7,4,5,7,4,5,5,4,5,5,5,4,5,7,4,5,4,6,4,2,6,7,7,7,2,5,2,6,
  2,2,5,4,5,5,4,5,5,5,2,5,4,5,4,6,5,5,5,4,2,4,5,7,5,4,5,4,6,5,7,4,5,5,4,
  2,2,7,2,2,2,7,4,7,7,4,5,5,4,7,2,7,4,3,0,3,2,2,2,2,2,2,7,2,2,2,2,2,2,2,
  2,2,2,2,7,7,2,4,0,4,7,5,4,5,5,4,5,2,4,0,3,7,7,2,7,7,2,7,7,7,4,0,0,4,2,
  4,7,7,4,2,7,7,7,4,3,3,3,4,0,0,4,7,4,6,4,2,4,2,4,0,0,3,2,2,2,4,0,0,4,0,
  0,4,7,7,4,7,4,5,6,5,4,5,7,4,7,7,4,2,7,4,2,4,7,2,7,2,4,7,4,6,4,2,4,2,4,
  7,7,4,2,7,4,7,7,7,4,7,7,4,5,4,6,4,2,4,2,4,7,2,7,4,7,7,7,4,7,7,4,3,2,2,
  7,2,7,2,2,7,7,2,2,2,2,7,2,2,2,7,7,3,4,7,4,6,4,2,4,2,4,2,4,3,3,4,0,0,4,
  3,2,3,4,0,0,4,2,7,7,4,0,0,4,5,2,4,5,4,5,5,4,5,4,6,6,6,7,7,4,3,5,5,5,5,
  5,5,5,5,4,5,4,5,4,5,4,6,6,6,7,4,5,4,5,4,5,4,5,6,6,6,7,4,5,4,5,4,6,6,4,
  0,4,0,0,4,0,0,4,0,4,0,0,4,6,3,0,2,1,1,1,4,0,4,0,0,4,0,0,4,0,4,0,0,4,5,
  5,4,5,5,4,5,5,4,2,4,6,3,0,2,1,1,1,7,7,2,2,7,7,7,2,7,4,7,7,4,7,4,5,6,5,
  4,5,7,4,7,7,4,2,7,4,2,4,7,2,2,4,7,7,4,2,7,4,7,7,7,4,7,7,4,5,4,6,4,2,4,
  2,4,7,2,7,4,7,7,4,7,7,4,3,3,7,2,7,2,2,7,7,2,2,2,7,2,2,2,7,7,4,0,0,4,0,
  2,4,0,4,0,0,4,0,0,4,0,4,0,0,4,5,5,4,5,5,4,5,4,5,5,4,2,4,6,3,0,2,1,1,1,
  7,7,2,2,7,7,6,5,2,7,4,3,4,0,4,0,0,4,0,0,4,0,4,0,0,4,5,5,4,5,5,4,5,4,5,
  5,4,2,4,6,3,0,2,1,1,1,2,2,7,7,6,5,2,7,4,0,0,4,7,5,4,5,5,4,5,2,4,0,3,3,
  2,7,7,2,7,7,2,7,7,7,3,4,0,4,0,0,4,0,0,4,0,4,0,0,4,5,5,4,5,5,4,5,5,4,2,
  4,6,3,0,2,1,1,1,2,2,7,7,7,2,7,4,7,5,4,5,5,4,5,2,4,0,3,7,7,2,7,7,2,7,7,
  4,0,0,4,7,5,4,5,5,4,5,2,4,0,3,3,2,7,7,2,7,7,2,7,7,3,4,0,4,0,0,4,0,4,0,
  0,4,5,5,4,5,5,4,5,5,4,2,4,6,3,0,1,1,1,7,7,2,2,7,7,7,2,7,4,0,4,0,0,4,0,
  0,4,0,4,0,0,4,5,5,4,5,5,4,5,5,4,2,4,6,0,2,1,1,1,7,7,2,2,7,7,2,7,4,0,4,
  0,0,4,0,4,0,4,0,4,5,5,4,5,5,4,6,0,2,1,1,1,7,7,2,2,7,7,7,4,0,4,0,0,4,0,
  4,0,4,0,4,5,5,4,5,5,4,6,0,2,1,1,1,7,2,2,7,7,7,4,0,4,0,0,4,0,4,0,4,0,4,
  5,5,4,5,5,4,6,0,2,1,1,1,7,2,2,7,7,4,0,0,4,0,4,0,4,0,4,5,5,4,5,5,4,3,2,
  1,1,1,7,2,2,7,4,0,4,0,0,4,0,0,4,0,0,4,0,4,0,0,4,5,5,4,5,5,4,5,5,4,2,4,
  6,2,0,2,1,1,1,7,7,2,2,7,7,7,2,7,3,4,7,7,4,7,4,5,6,5,4,5,7,4,7,7,4,2,7,
  4,2,4,7,2,7,2,4,7,7,4,2,7,4,7,7,7,4,7,7,4,5,4,6,4,2,4,2,4,7,2,7,4,7,7,
  7,4,7,7,4,3,3,7,2,7,2,2,7,7,2,2,2,7,2,2,2,7,7,4,3,4,5,5,5,4,5,5,4,7,2,
  2,7,7,4,5,5,4,5,4,5,4,5,5,2,6,6,5,4,5,5,5,4,5,5,4,5,5,5,4,7,5,6,2,6,5,
  4,5,5,4,5,5,4,7,7,4,2,2,2,5,7,4,7,5,4,2,4,5,5,4,6,4,5,4,5,4,5,5,5,6,5,
  4,5,5,4,5,5,4,5,4,5,5,5,6,2,6,2,6,5,4,5,4,5,5,4,6,2,7,7,4,5,4,6,5,5,4,
  2,4,7,7,4,7,2,2,7,2,7,7,4,7,7,4,5,5,4,2,7,4,7,7,7,4,5,5,4,7,4,5,7,4,5,
  5,4,5,5,5,4,5,7,4,5,4,6,4,2,6,7,7,7,2,5,2,6,2,2,5,4,5,5,4,5,5,5,2,5,4,
  5,4,6,5,5,5,4,2,4,5,7,5,4,5,4,6,5,4,5,5,4,2,2,7,2,2,2,4,5,5,4,7,2,7,4,
  2,2,2,2,7,2,2,2,2,2,7,2,2,2,2,2,4,0,0,4,0,2,4,0,0,4,5,0,2,4,0,4,0,0,4,
  2,5,5,4,3,4,7,4,7,4,7,5,4,7,4,5,5,4,5,2,4,0,3,3,5,5,6,6,5,2,6,7,6,5,7,
  7,7,4,5,4,0,0,4,2,3,4
};

static const unsigned short ag_key_parm[] = {
    0,161,163,162,  0,426,423,  0,  4,  0,  6,  0,  0,  0,  0,  0,437,134,
    0,  0,  0,  0,497,  0,522,425,465,  0,161,163,162,  0,426,423,  0,512,
  501,  0,495,  0,499,514,  0,280,282,322,  0,284,116,324,  0, 50,182,  0,
  320,  0,  0,186,  0,  0,424,422,  0,430,  0,418,  0,166, 54,426,  0,428,
  416, 58,420,  0,190,192,286,  0,332,334,  0,338,340,342,  0,326,328,128,
  330,  0,336, 44,344,  0,194,306,  0,288,308,  0,144,  0, 42,  0,  0,  0,
    0,  0,  0,390,396,  0, 46,198,  0,  0, 56,  0,168,196, 52, 62,176,  0,
   24, 28,  0, 30,  0, 80, 14,  0,  0,  0, 18, 70,  0,  0, 68,  0,  0,  0,
   34,202,  0,  0,100, 32,  0,  0, 78,126,138,  0,104,108,  0,200,204,  0,
   12,140,170,  0, 20, 26,  0,  4,  0,  6,  0,  0, 98,290,310,  0, 22,346,
  136,  0,362,  0,368,  0,364,374,372,366,376,  0,380,382,  0,354,370,  0,
  352,  0,348,358,356,360,  0,378,  0,350,384,  0,270,  0,388,394,  0,386,
    0,392,398,  0,210,  0,400,208,206,  0,  0,  0, 92, 10,  0,132,142,  0,
  276,278,  0,442,  0,106,  0,  0,444,130,  0,  0,266, 84,110,312,  0, 72,
    0,122,292,  0, 94,  0, 82,314,  0, 88,  0,212,124,  0, 74,102,  0,  0,
  294, 48,402,  0,112,404,  0,214,  0, 38,  0, 36,  0,  0,  0,  8,  0,  0,
   66,274,  0, 96,178,272,  0,174,  0,  0,216,218,  0,432,  0,226,228,  0,
  234,236,  0,240,304,242,  0,244,188,  0,246,  0,318,  0,  0,220,230,222,
  224,  0,232,  0,238,  0,  0,248,  0,440,436,  0,296,438,406,  0,434,  0,
  254,  0,408,252,184,250,  0,118,120,  0,302,258,316,  0,268,  0,410,260,
   76,  0,298,412,  0,  0,  2,  0,256,172,134,  0,  0, 90,  0, 60, 86,  0,
  300,414,  0,262,114,  0,264,  0,180,437,464,134,230,240,  0,492,  0,  0,
  505,431,503,164,150,152,154,156,  0,  0,158,  0,  0,160,162,  0,  0,  0,
    0,  0,  0, 40, 16, 64,  0,460,  0,161,163,162,  0,426,423,  0,437,134,
    0,  0,  0, 24, 28,  0, 34,  0, 30, 32,  0, 20, 26,  0, 22,  4,  0, 38,
    0, 36,  0,  0,  0,  8,  0,  0,  0,  0, 42,  0,  0,  0, 40,  0,426,423,
    0,464,134,465,  0,160,431,  0,426,423,  0,280,282,322,  0,284,324,  0,
   50,182,  0,320,  0,  0,186,  0,  0,424,422,  0,430,  0,418,  0,166, 54,
  426,  0,428,416, 58,420,  0,190,192,286,  0,332,334,  0,338,340,342,  0,
  326,328,330,  0,336, 44,344,  0,194,306,  0,288,308,  0,390,396,  0, 46,
  198,  0,  0, 56,  0,  0,196, 52, 62,  0, 30,  0, 80, 14,  0, 18, 70,  0,
    0, 68,  0,  0,  0, 34,202, 28,  0,  0,  0,  0,200,204,  0, 12,  0,  0,
  290,310,  0, 22,346,  0,362,  0,368,  0,364,374,372,366,376,  0,380,382,
    0,354,370,  0,352,  0,348,358,356,360,  0,378,  0,350,384,  0,270,  0,
  388,394,  0,386,  0,392,398,  0,210,  0,400,208,206,  0,  0,  0, 92, 10,
    0,276,278,  0,442,  0,  0,  0,444,  0,266, 84,312,  0, 72,292,  0, 94,
    0, 82,314,  0, 88,  0,212,  0, 74,  0,  0,294, 48,402,  0,  0,404,  0,
   38,  0, 36,  0,  0,  0,  0,  0, 66,274,  0, 96,214,272,  0,  0,  0,216,
  218,  0,432,  0,226,228,  0,234,236,  0,240,304,242,  0,244,188,  0,246,
    0,318,  0,  0,220,230,222,224,  0,232,  0,238,  0,  0,248,  0,440,436,
    0,296,438,406,  0,434,  0,254,  0,408,252,184,250,  0,  0,  0,302,258,
  316,  0,268,  0,410,260, 76,  0,298,412,  0,  0,  2,  0,256,  0,  0,  0,
   90,  0, 60, 86,  0,300,414,  0,262,  0,264,  0,464,134,465,  0,431,  0,
    0,  0,  0,  0, 78,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0, 16, 64,
    0,  0,426,423,  0,  4,  0,  6,  0,  0,  0,  0,  0,  0,  0,  0,426,423,
    0,280,282,322,  0,284,324,  0, 50,182,  0,320,  0,  0,186,  0,  0,424,
  422,  0,430,  0,418,  0,166, 54,426,  0,428,416, 58,420,  0,190,192,286,
    0,332,334,  0,338,340,342,  0,326,328,330,  0,336, 44,344,  0,194,306,
    0,288,308,  0,390,396,  0, 46,198,  0,  0, 56,  0,  0,196, 52, 62,  0,
   30,  0, 80, 14,  0, 18, 70,  0,  0, 68,  0,  0,  0, 34,202, 28,  0,  0,
    0,200,204,  0, 12,  0,  0,  4,  0,  6,  0,  0,290,310,  0, 22,346,  0,
  362,  0,368,  0,364,374,372,366,376,  0,380,382,  0,354,370,  0,352,  0,
  348,358,356,360,  0,378,  0,350,384,  0,270,  0,388,394,  0,386,  0,392,
  398,  0,210,  0,400,208,206,  0,  0,  0, 92, 10,  0,276,278,  0,442,  0,
    0,  0,444,  0,266, 84,312,  0, 72,292,  0, 94,  0, 82,314,  0, 88,  0,
  212,  0, 74,  0,  0,294, 48,402,  0,  0,404,  0, 38,  0, 36,  0,  0,  0,
    0,  0, 66,274,  0, 96,214,272,  0,  0,  0,216,218,  0,432,  0,226,228,
    0,234,236,  0,240,304,242,  0,244,188,  0,246,  0,318,  0,  0,220,230,
  222,224,  0,232,  0,238,  0,  0,248,  0,440,436,  0,296,438,406,  0,434,
    0,254,  0,408,252,184,250,  0,  0,  0,302,258,316,  0,268,  0,410,260,
   76,  0,298,412,  0,  0,  0,256,  0,  0,  0, 90,  0, 60, 86,  0,300,414,
    0,262,  0,264,  0,464,134,465,  0,  0,  0,  0,  0,  0, 78,  0,  0,  0,
    0,  0,  0,  0,  0,  0,  0,  0, 16, 64,  0,  0,134,  0,126,138,  0,132,
  142,  0,130,  0,  0,134,230,128,144,  0,140,136,  0,124, 96,134,  0,426,
  423,  0,  0,  0, 14, 18,  0,  0, 12, 10, 16,  0,161,163,162,  0,426,423,
    0,  4,  0,  6,  0,  0,  0,  0,  0,437,134,425,  0,  0,  0,  0,425,465,
    0,426,423,  0, 54, 58,  0, 46,  0, 56, 52, 62,  0, 80, 14,  0, 18, 70,
    0,  0, 68,  0,  0,  0, 34,  0,  0,  0,  0,  4,  0,  6,  0,  0,  0,  0,
    0, 92, 10,  0,  0, 84,  0, 94, 72, 82,  0, 74, 88,  0, 38,  0, 36,  0,
    0,  0,  0,  0, 96,  0, 66,  0,  2, 76, 90,  0, 60, 86,  0,464,  0,  0,
   50,  0, 44,  0,  0, 78, 12,  0,  0,  0,  0, 48,  0,  0,  0, 16, 64,460,
    0,  4,  0,  6,  0,  0,  0,  0,  0,  0,  0,425,426,  0,426,423,  0,425,
    0,460,  0,426,423,  0,  0, 98, 96,  0,426,423,  0,180,  0,  0,166,  0,
  168,176,  0,170,  0,150,154,158,178,172,  0,240,164,150,152,154,156,158,
  160,162,  0,166,  0,168,  0,170,  0,150,154,158,172,  0,166,  0,168,  0,
  170,  0,164,150,154,158,174,  0,166,  0,168,  0,150,154,  0,497,  0,426,
  423,  0,512,501,  0,495,  0,499,514,  0,180,522,492,  0,505,431,503,  0,
  497,  0,426,423,  0,512,501,  0,495,  0,499,514,  0,104,108,  0,106,110,
    0,118,120,  0,  0,  0,180,522,492,  0,505,431,503,116,100,  0,  0,122,
  102,112,  0,114,  0, 54, 58,  0, 46,  0, 56, 52, 62,  0, 80, 14,  0, 18,
   70,  0,  0, 68,  0,  0,  0, 34,  0,  0,  0, 92, 10,  0,  0, 84,  0, 94,
   72, 82,  0, 74, 88,  0, 38,  0, 36,  0,  0,  0,  0,  0, 96,  0, 66,  0,
   76, 90,  0, 60, 86,  0,464,465, 50,  0, 44,  0,  0, 78, 12,  0,  0,  0,
   48,  0,  0,  0, 16, 64,  0,426,423,  0,492,  0,  0,497,  0,426,423,  0,
  512,501,  0,495,  0,499,514,  0,104,108,  0,106,110,  0,112,  0,118,120,
    0,  0,  0,180,522,492,  0,505,431,503,116,100,  0,  0,122,102,148,146,
    0,114,  0,232,  0,497,  0,426,423,  0,512,501,  0,495,  0,499,514,  0,
  104,108,  0,106,110,  0,112,  0,118,120,  0,  0,  0,180,522,492,  0,505,
  431,503,  0,  0,122,102,148,146,  0,114,  0,426,423,  0,126,138,  0,132,
  142,  0,130,  0,  0,134,230,425,  0,128,144,  0,140,136,  0,124, 96,134,
  460,  0,497,  0,426,423,  0,512,501,  0,495,  0,499,514,  0,104,108,  0,
  106,110,  0,118,120,  0,  0,  0,180,522,492,  0,505,431,503,  0,  0,122,
  102,112,  0,114,  0,126,138,  0,132,142,  0,130,  0,  0,134,230,128,144,
    0,140,136,  0, 96,134,  0,426,423,  0,126,138,  0,132,142,  0,130,  0,
    0,134,230,425,  0,128,144,  0,140,136,  0, 96,134,460,  0,497,  0,512,
  501,  0,495,  0,499,514,  0,104,108,  0,106,110,  0,118,120,  0,  0,  0,
  180,522,492,505,431,503,116,100,  0,  0,122,102,112,  0,114,  0,497,  0,
  426,423,  0,512,501,  0,495,  0,499,514,  0,104,108,  0,106,110,  0,118,
  120,  0,  0,  0,180,492,  0,505,431,503,116,100,  0,  0,102,112,  0,114,
    0,497,  0,426,423,  0,501,  0,495,  0,499,  0,104,108,  0,106,110,  0,
  180,492,  0,505,431,503,116,100,  0,  0,102,112,114,  0,497,  0,426,423,
    0,501,  0,495,  0,499,  0,104,108,  0,106,110,  0,180,492,  0,505,431,
  503,100,  0,  0,102,112,114,  0,497,  0,426,423,  0,501,  0,495,  0,499,
    0,104,108,  0,106,110,  0,180,492,  0,505,431,503,100,  0,  0,102,112,
    0,426,423,  0,501,  0,495,  0,499,  0,104,108,  0,106,110,  0,497,  0,
  505,431,503,100,  0,  0,102,  0,497,  0,522,425,  0,426,423,  0,512,501,
    0,495,  0,499,514,  0,104,108,  0,106,110,  0,118,120,  0,  0,  0,180,
    0,492,  0,505,431,503,116,100,  0,  0,122,102,112,  0,114,460,  0, 54,
   58,  0, 46,  0, 56, 52, 62,  0, 80, 14,  0, 18, 70,  0,  0, 68,  0,  0,
    0, 34,  0,  0,  0,  0, 92, 10,  0,  0, 84,  0, 94, 72, 82,  0, 74, 88,
    0, 38,  0, 36,  0,  0,  0,  0,  0, 96,  0, 66,  0,  2, 76, 90,  0, 60,
   86,  0,464,465, 50,  0, 44,  0,  0, 78, 12,  0,  0,  0, 48,  0,  0,  0,
   16, 64,  0,230,  0,280,282,322,  0,284,324,  0,320,  0,  0,186,182,  0,
  424,422,  0,430,  0,418,  0,166,426,  0,428,416,420,  0,190,192,286,  0,
  332,334,  0,338,340,342,  0,326,328,330,  0,336,344,  0,194,306,  0,288,
  308,  0,390,396,  0,  0,  0,  0,196,198,  0,200,204,  0,  0,  0,290,310,
    0,346,  0,362,  0,368,  0,364,374,372,366,376,  0,380,382,  0,354,370,
    0,352,  0,348,358,356,360,  0,378,  0,350,384,  0,270,  0,388,394,  0,
  386,  0,392,398,  0,210,  0,400,208,206,  0,  0,  0,276,278,  0,442,  0,
    0,444,  0,266,312,  0,292,314,  0,294,402,  0,  0,404,  0,214,272,274,
    0,216,218,  0,432,  0,226,228,  0,234,236,  0,240,304,242,  0,244,188,
    0,246,  0,318,  0,  0,220,230,222,224,  0,232,  0,238,  0,  0,248,  0,
  440,436,  0,296,438,406,  0,434,  0,254,  0,408,252,184,250,  0,  0,  0,
  302,258,316,  0,268,  0,410,260,  0,298,412,  0,  0,  0,256,  0,  0,  0,
    0,300,414,  0,262,  0,264,  0,  0,  0,  0,  0,202,  0,  0,  0,  0,  0,
  212,  0,  0,  0,  0,  0,  0,426,423,  0,134,  0,  0,426,423,  0,180,492,
    0,  0,492,  0,426,423,  0,  0,148,146,  0,460,  0,128,  0,144,  0,126,
  138,  0,140,  0,132,142,  0,130,  0,  0,134,230,240,164,150,152,154,156,
    0,158,136,160,162,124, 96,134,  0,180,  0,426,423,  0,  0,460,  0
};

static const unsigned short ag_key_jmp[] = {
    0,  0,  3,  7,  0,  0,  0,  0, 11,  0,  8,  0, 10,  0, 12,  0,  0,  0,
    1,  5, 14,  0,  0,  0,  0,  0,  0,  0, 23, 26, 30,  0,  0,  0,  0,  0,
    0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0, 39, 41,  0,
   34, 43, 47, 36, 51,  0,  0,  0,  0,  0,  0,  0,  0,  0, 43,  0, 60, 63,
   65, 47,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0, 50,  0, 53,
   76, 80, 83, 56,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,102,  0,104,  0,
  106,  0,108, 59, 61,  0, 63, 65,  0, 96,  0, 99,110,  0,114,  0,  0,  0,
   70, 72,  0, 74,  0,129, 76,  0,  0,  0, 83, 87,  0,136, 89,  0,139,  0,
   67,  0,126,131,134, 79,142,  0, 91, 94,  0,  0,  0,  0,  0,103,  0,  0,
   98,100,159,  0,106,109,  0,113,  0,169,  0,171,116,  0,  0,  0,166,173,
    0,  0,  0,  0,  0,  0,  0,  0,  0,184,  0,  0,  0,  0,  0,  0,  0,  0,
    0,  0,  0,  0,  0,182,186,192,195,198,  0,  0,  0,  0,  0,  0,  0,210,
  212,124,126,  0,  0,  0,220,  0,  0,  0,222,  0,128,130,  0,  0,  0,  0,
  135,137,  0,120,215,  0,226,228,132,  0,231,234,140,143,  0,149,  0,156,
    0,251,  0,  0,151,253,159,165,  0,170,  0,261,  0,  0,167,  0,263,  0,
    0,  0,  0,  0,270,173,  0,178,  0,  0,  0,279,  0,281,  0,182,283,  0,
  188,192,  0,175,277,180,285,186,288,  0,  0,  0,  0,194,  0,  0,204,  0,
    0,  0,  0,  0,  0,  0,  0,  0,206,  0,  0,  0,316,  0,298,301,197,200,
  202,303,  0,306,309,313,318,  0,  0,  0,  0,  0,  0,  0,  0,333,  0,  0,
    0,  0,342,  0,  0,  0,  0,344,  0,  0,  0,212,  0,  0,  0,  0,356,  0,
  217,  0,  0,  0,  0,336,208,349,210,352,214,358,362,220,  0,222,225,  0,
    0,  0,  0,245,248,378,250,  0, 22,  0, 14,  0, 16, 19, 24,  0, 28, 32,
   35, 38, 40, 54, 67, 87,117,144,152,156,162,178,200,237,256,266,274,291,
  320,365,375,229,234,241,381,253,  0,255,258,262,  0,  0,  0,  0,  0,  0,
  423,427,  0,275,277,  0,272,435,279,283,  0,287,290,  0,443,294,  0,  0,
    0,449,  0,451,  0,300,453,  0,455,  0,266,438,446,458,304,  0,  0,  0,
    0,309,  0,311,466,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,
  322,324,  0,317,479,483,319,486,  0,  0,  0,  0,  0,  0,  0,  0,  0,326,
    0,495,498,500,330,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,
  333,  0,511,515,518,336,  0,  0,  0,  0,  0,  0,  0,  0,339,341,  0,343,
  345,  0,530,  0,533,536,  0,539,  0,  0,353,  0,550,355,  0,360,364,  0,
  555,366,  0,558,  0,347,  0,350,552,358,561,  0,374,  0,  0,372,570,  0,
    0,  0,  0,  0,576,  0,  0,  0,  0,  0,  0,  0,  0,584,  0,  0,  0,  0,
    0,  0,  0,  0,  0,  0,  0,  0,  0,582,586,592,595,598,  0,  0,  0,  0,
    0,  0,  0,610,612,381,383,  0,  0,  0,620,  0,  0,  0,622,  0,385,387,
    0,392,394,  0,377,615,626,628,389,631,397,400,406,  0,413,  0,  0,408,
  644,417,423,  0,428,  0,652,  0,425,654,  0,  0,  0,  0,  0,659,431,  0,
    0,  0,666,  0,668,  0,670,  0,441,445,  0,433,436,439,672,674,  0,  0,
    0,  0,447,  0,  0,457,  0,  0,  0,  0,  0,  0,  0,  0,  0,459,  0,  0,
    0,701,  0,683,686,450,453,455,688,  0,691,694,698,703,  0,  0,  0,  0,
    0,  0,  0,  0,718,  0,  0,  0,  0,727,  0,  0,  0,  0,729,  0,  0,465,
    0,  0,  0,  0,740,  0,467,  0,  0,  0,  0,721,461,734,463,736,742,746,
  470,  0,472,475,  0,  0,  0,  0,490,761,493,  0,313,  0,315,476,  0,489,
  502,522,542,563,368,573,579,600,634,647,656,663,677,705,749,758,479,486,
  764,  0,  0,  0,  0,496,  0,797,  0,799,  0,801,  0,794,803,  0,  0,  0,
    0,  0,  0,  0,  0,  0,  0,  0,508,510,  0,503,811,815,505,818,  0,  0,
    0,  0,  0,  0,  0,  0,  0,512,  0,827,830,832,516,  0,  0,  0,  0,  0,
    0,  0,  0,  0,  0,  0,  0,  0,519,  0,843,847,850,522,  0,  0,  0,  0,
    0,  0,  0,  0,525,527,  0,529,531,  0,862,  0,865,868,  0,871,  0,  0,
  539,  0,882,541,  0,544,548,  0,887,550,  0,890,  0,533,  0,536,884,893,
    0,558,  0,  0,556,901,  0,561,  0,907,  0,909,  0,  0,  0,  0,911,  0,
    0,  0,  0,  0,  0,  0,  0,920,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,
    0,  0,  0,918,922,928,931,934,  0,  0,  0,  0,  0,  0,  0,946,948,568,
  570,  0,  0,  0,956,  0,  0,  0,958,  0,572,574,  0,579,581,  0,564,951,
  962,964,576,967,584,587,593,  0,600,  0,  0,595,980,604,610,  0,615,  0,
  988,  0,612,990,  0,  0,  0,  0,  0,995,618,  0,  0,  0,1002,  0,1004,
    0,1006,  0,628,632,  0,620,623,626,1008,1010,  0,  0,  0,  0,634,  0,
    0,644,  0,  0,  0,  0,  0,  0,  0,  0,  0,646,  0,  0,  0,1037,  0,1019,
  1022,637,640,642,1024,  0,1027,1030,1034,1039,  0,  0,  0,  0,  0,  0,
    0,  0,1054,  0,  0,  0,  0,1063,  0,  0,  0,  0,1065,  0,  0,650,  0,
    0,  0,  0,1076,  0,652,  0,  0,  0,  0,1057,1070,648,1072,1078,1082,
  655,  0,657,660,  0,  0,  0,  0,675,1096,678,  0,499,  0,501,808,821,834,
  854,874,895,552,904,915,936,970,983,992,999,1013,1041,1085,1093,664,671,
  1099,  0,  0,  0,695,  0,  0,  0,  0,  0,  0,1133,  0,  0,681,684,688,
  1130,699,703,1136,705,708,712,  0,  0,  0,  0,1151,  0,716,720,  0,1156,
  727,730,734,  0,743,746,750,  0,  0,  0,  0,754,  0,1171,  0,1173,  0,
  1175,  0,  0,  0,741,1164,1168,1177,  0,  0,  0,  0,  0,  0,  0,763,767,
    0,774,  0,  0,1195,  0,  0,  0,779,  0,784,788,  0,1204,790,  0,1207,
    0,776,1201,782,1210,  0,799,  0,1217,  0,1219,  0,1221,  0,802,804,  0,
  1225,806,  0,812,817,822,  0,828,831,  0,  0,  0,1238,  0,1240,  0,1242,
    0,839,1244,842,  0,847,849,853,  0,855,858,  0,757,1186,1189,759,1192,
  770,1197,1212,792,796,1223,1228,1231,1235,836,1246,1250,1254,862,869,873,
    0,875,  0,1279,  0,1281,  0,1283,  0,1285,  0,878,880,  0,  0,  0,  0,
  882,1292,884,  0,  0,  0,  0,1299,886,892,  0,  0,  0,  0,  0,1306,  0,
    0,  0,  0,  0,  0,  0,  0,1312,1314,1317,896,898,  0,900,  0,  0,  0,
    0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,1335,1337,1339,904,  0,
    0,  0,  0,  0,  0,  0,  0,1346,1348,1350,906,  0,  0,  0,  0,  0,1358,
  1360,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,1365,909,
    0,1367,1370,1373,1375,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,
    0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,1405,  0,1386,911,  0,1388,
  1391,1394,1396,913,916,1399,1402,918,921,923,1408,925,  0,936,940,  0,
  947,  0,  0,1430,  0,  0,  0,952,  0,955,959,  0,1439,961,  0,1442,  0,
  949,1436,1445,  0,970,972,  0,1451,974,  0,980,985,990,  0,996,999,  0,
    0,  0,1464,  0,1466,  0,1468,  0,1007,1470,1010,  0,1015,1019,  0,1021,
  1024,  0,928,930,932,1427,943,1432,1447,963,967,1454,1457,1461,1004,1472,
  1476,1479,1028,1035,  0,  0,  0,  0,  0,1501,  0,  0,  0,  0,  0,  0,  0,
    0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,
  1528,  0,1507,1039,  0,1509,1512,1515,1517,1041,1044,1520,1523,1046,1049,
  1526,  0,1531,1051,  0,1054,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,
    0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,1574,  0,1553,
  1056,  0,1555,1558,1561,1563,1566,1569,1058,1061,1572,  0,1577,1063,  0,
    0,  0,  0,1082,  0,  0,  0,  0,  0,  0,1601,  0,  0,1066,1069,1595,1071,
  1075,1598,1086,1090,1604,1092,1095,1099,1103,  0,  0,  0,  0,  0,  0,  0,
    0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,1641,  0,
  1622,1105,  0,1624,1627,1630,1632,1635,1638,1107,1110,1112,1644,1114,  0,
  1131,  0,  0,  0,  0,  0,  0,1664,  0,  0,1117,1120,1124,1661,1135,1139,
  1667,1141,1145,  0,  0,  0,  0,1165,  0,  0,  0,  0,  0,  0,1687,  0,  0,
  1149,1152,1681,1154,1158,1684,1169,1173,1690,1175,1179,1183,  0,  0,  0,
    0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,1723,
    0,1707,1185,  0,1709,1712,1714,1187,1190,1717,1720,1192,1195,1197,1726,
  1199,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,
    0,  0,  0,  0,  0,  0,1763,  0,1744,  0,1746,1749,1752,1754,1202,1205,
  1757,1760,1207,1209,1766,1211,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,
    0,  0,  0,  0,  0,  0,  0,  0,1783,  0,1785,1788,1790,1792,1214,1217,
  1794,1797,1219,1221,1223,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,
    0,  0,  0,  0,  0,  0,1814,  0,1816,1819,1821,1823,1226,1825,1828,1228,
  1230,1232,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,
    0,  0,  0,1844,  0,1846,1849,1851,1853,1235,1855,1858,1237,1239,  0,
    0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,1241,1873,
  1876,1878,1880,1243,1882,1885,1245,  0,  0,  0,  0,  0,  0,  0,  0,  0,
    0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,1920,
    0,1898,1900,  0,1903,1906,1909,1911,1247,1250,1914,1917,1252,1255,1257,
  1923,1259,1262,  0,1272,1276,  0,1283,  0,  0,1946,  0,  0,  0,1288,  0,
  1293,1297,  0,1955,1299,  0,1958,  0,1285,1952,1291,1961,  0,1308,1310,
    0,1968,1312,  0,1318,1323,1328,  0,1334,1337,  0,  0,  0,1981,  0,1983,
    0,1985,  0,1345,1987,1348,  0,1353,1355,1359,  0,1361,1364,  0,1264,
  1266,1268,1943,1279,1948,1963,1301,1305,1971,1974,1978,1342,1989,1993,
  1997,1368,1375,  0,1379,  0,  0,  0,  0,  0,  0,  0,  0,1382,2021,2025,
  1384,1387,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,2034,2037,2039,  0,  0,
    0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,1390,  0,2048,2052,2055,  0,
    0,  0,  0,  0,  0,  0,  0,1393,1395,  0,2066,2069,2072,  0,1397,  0,
  1402,  0,  0,2081,  0,  0,  0,  0,2086,  0,  0,  0,  0,  0,  0,  0,  0,
  2093,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,2091,2095,2101,
  2104,2107,  0,  0,  0,  0,  0,  0,  0,2119,2121,1409,1411,  0,  0,  0,
  2129,  0,  0,  0,2131,  0,1416,1418,  0,1405,2124,2135,1413,2137,1421,
  1424,  0,1426,1428,  0,  0,  0,  0,2151,1433,  0,1435,1438,1440,  0,  0,
    0,  0,1443,  0,  0,1453,  0,  0,  0,  0,  0,  0,  0,  0,  0,1455,  0,
    0,  0,2179,  0,2161,2164,1446,1449,1451,2166,  0,2169,2172,2176,2181,
    0,  0,  0,  0,  0,  0,  0,  0,2196,  0,  0,  0,  0,2205,  0,  0,  0,
    0,2207,  0,  0,1459,  0,  0,  0,  0,2218,  0,  0,  0,  0,  0,2199,2212,
  1457,2214,2220,2223,  0,  0,  0,  0,1461,2233,1464,  0,2028,2041,2059,
  2075,1400,2084,2089,2109,2140,2148,1430,2154,2157,2183,2226,2236,  0,  0,
    0,  0,  0,2257,  0,  0,  0,  0,  0,  0,2263,  0,  0,  0,  0,  0,  0,
  2272,  0,  0,  0,1467,  0,1476,  0,1479,  0,1485,  0,  0,1489,  0,  0,
    0,  0,  0,2290,  0,  0,1469,1472,  0,  0,2281,2283,  0,2285,2288,1492,
  2293,  0,1494,1497,1501,  0,  0,  0,  0,  0,  0,2315,1505,  0
};

static const unsigned short ag_key_index[] = {
   16,386,430,460,469,460,768,  0,805,768,1103,430,1128,1128,  0,  0,1139,
    0,1154,1154,1128,1139,1139,1128,1159,  0,1128,1128,  0,  0,1139,  0,
  1154,1154,1128,1139,1139,1128,1159,  0,1179,1257,1287,1289,1295,  0,1289,
    0,  0,768,1302,1309,1309,1309,1309,1309,1309,1309,1309,1309,1309,1309,
  1309,1309,1309,1319,1325,1341,1139,1139,1139,1139,1139,1139,1139,1139,
  1139,1139,1139,1139,1139,1139,1139,1139,1309,1309,1309,1309,1309,1309,
  1309,1309,1309,1309,1309,1309,1309,1309,1309,1309,1309,1309,1309,1309,
  1309,1309,1309,1309,1309,1309,1309,1309,1309,  0,1319,1325,1341,1139,1139,
  1139,1139,1139,1139,1139,1139,1139,1139,1139,1139,1139,1139,1139,1139,
  1139,1139,1139,1139,1139,1139,1139,1139,1139,1139,1139,1139,1139,1139,
  1139,1139,1139,1139,1139,1139,1139,1139,1139,1139,1139,1139,1139,1139,
  1139,1139,1139,1139,1352,1352,1352,1352,1362,1362,1341,1341,1341,1319,
  1319,1325,1325,1325,1325,1325,1325,1325,1325,1325,1325,1325,  0,  0,  0,
    0,1139,1139,1139,1139,1139,1139,1139,1139,1139,1139,1139,1139,1139,1139,
  1139,1139,1139,1139,1139,1139,1139,1139,1139,1139,1139,1139,1139,1139,
  1139,1139,1139,1139,1139,1139,1139,1139,1139,1139,1139,1139,1139,1139,
  1139,1139,1139,1139,1139,1139,1139,1139,1139,1139,1352,1352,1352,1352,
  1362,1362,1341,1341,1341,1319,1319,1325,1325,1325,1325,1325,1325,1325,
  1325,1325,1325,1325,1378,1309,1410,1154,1309,1139,1482,1154,  0,1154,  0,
    0,1139,  0,1504,1154,1154,1154,1154,  0,  0,  0,1533,  0,1551,1410,  0,
  1410,1410,1410,1579,  0,1154,  0,  0,  0,  0,  0,  0,  0,  0,  0,1410,
  1410,1607,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,1646,1670,1693,1670,
  1693,1139,1670,1670,1670,1670,1410,1728,1768,1768,1800,1831,1861,1888,
  1139,1154,1154,1154,1154,1154,1154,1154,1128,1128,1154,1154,1154,1154,
    0,1154,1154,1154,  0,  0,1295,1607,1295,1607,1289,1295,1295,1295,1295,
  1295,1295,1295,1295,1295,1295,1295,1295,1295,1295,1295,1295,1295,1295,
  1295,1693,1607,1607,1607,1693,1925,1295,1607,1693,1295,1295,1295,1179,
    0,  0,  0,1139,1139,2000,1154,1302,1319,1325,1341,  0,  0,  0,  0,1139,
  1139,1139,1139,1139,1139,1139,1139,1139,1139,1139,1139,1139,1139,1139,
  1139,1139,1139,1139,1139,1139,1139,1139,1139,1139,1139,1139,1139,1139,
  1139,1139,1139,1139,1139,1139,1139,1139,1139,1139,1139,1139,1139,1139,
  1139,1139,1139,1139,1139,1139,1139,1139,1139,1352,1352,1352,1352,1362,
  1362,1341,1341,1341,1319,1319,1325,1325,1325,1325,1325,1325,1325,1325,
  1325,1325,1325,1139,2019,2240,2240,1128,1128,  0,  0,1154,1154,  0,  0,
  1154,1154,2260,2260,  0,  0,1139,1139,1139,1139,1139,1128,1128,1128,1128,
  1128,1128,1128,1128,1128,1128,1139,1139,1139,1139,1139,1139,1139,1139,
  1139,1139,1154,1139,1139,1139,1139,1128,1302,1302,1154,  0,1154,  0,1154,
  1154,1154,  0,1154,1154,  0,  0,1551,1410,1410,  0,  0,  0,  0,  0,  0,
    0,  0,  0,  0,  0,1139,  0,1670,1670,1670,1670,1670,1670,1670,1139,1139,
  1139,1139,1139,1139,1139,1139,1139,1139,1139,1139,1139,1139,1139,1139,
  1139,1139,1139,1139,1139,1139,1139,1139,1139,1139,1139,1139,1139,1139,
  1139,1154,1154,1154,1154,1154,1128,1154,1154,  0,1154,1154,  0,1154,1154,
  1154,1154,1139,1139,1139,1139,1154,1154,1154,1309,1309,1309,2266,1309,
  1139,1139,1139,1309,1309,2270,1551,1551,1504,2270,  0,2275,2240,1551,  0,
  1139,1139,1504,1504,1504,1504,1504,2279,1139,1154,1154,1154,  0,  0,  0,
    0,1128,1139,1139,1139,1139,1139,1139,1139,1139,1139,1139,  0,1670,1670,
  1670,1670,1670,1139,1139,1139,1139,1139,1139,1139,1139,1139,1139,1139,
  1139,1154,1154,1154,1154,1154,1154,1154,1154,1139,1139,1139,2270,1551,
  2296,1139,2313,1309,1551,1139,1504,1128,2318,2318,1154,1139,  0,  0,1128,
    0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,1728,1728,1768,1768,1768,1768,
  1768,1768,1800,1800,1831,1831,1154,1154,1154,1325,1551,1139,1139,1128,
  1154,  0,  0,  0,1325,1154,  0
};

static const unsigned char ag_key_ends[] = {
83,83,0, 65,84,65,0, 69,88,84,0, 68,69,0, 61,0, 92,39,0, 
72,76,41,0, 83,83,0, 65,84,65,0, 69,88,84,0, 73,0, 72,76,0, 
71,0, 82,0, 79,67,75,0, 84,69,0, 76,76,0, 73,76,0, 69,71,0, 
76,0, 80,0, 71,0, 66,0, 72,79,0, 70,0, 69,0, 70,0, 82,89,0, 
82,79,82,0, 68,69,68,0, 78,0, 78,0, 76,76,0, 79,79,82,0, 88,0, 
71,72,0, 66,67,0, 69,70,0, 68,69,70,0, 68,69,0, 65,71,69,0, 
65,76,76,0, 73,0, 73,0, 75,0, 84,0, 77,80,0, 80,0, 83,72,0, 
69,84,0, 70,73,82,83,84,0, 73,0, 67,76,73,66,0, 76,69,0, 
70,73,82,83,84,0, 73,0, 77,69,0, 71,69,0, 84,0, 71,69,0, 76,0, 
80,0, 71,77,65,0, 87,0, 76,73,67,0, 72,0, 76,76,0, 69,76,0, 
84,0, 77,0, 69,0, 76,0, 84,0, 77,0, 76,0, 82,84,0, 76,78,0, 
77,0, 88,84,0, 84,76,69,0, 78,68,69,70,0, 69,82,73,76,79,71,0, 
79,82,68,0, 72,71,0, 82,0, 72,76,0, 10,0, 83,83,0, 65,84,65,0, 
69,88,84,0, 69,70,73,78,69,0, 72,79,0, 70,0, 69,0, 68,73,70,0, 
82,79,82,0, 69,70,0, 68,69,70,0, 67,76,85,68,69,0, 71,77,65,0, 
78,68,69,70,0, 61,0, 61,0, 61,0, 61,0, 73,0, 72,76,0, 71,0, 
82,0, 79,67,75,0, 84,69,0, 76,76,0, 69,71,0, 76,0, 80,0, 71,0, 
66,0, 72,79,0, 83,69,0, 70,0, 82,89,0, 85,0, 68,69,68,0, 78,0, 
78,0, 73,76,76,0, 88,0, 66,67,0, 65,76,76,0, 73,0, 73,0, 75,0, 
84,0, 77,80,0, 80,0, 83,72,0, 69,84,0, 70,73,82,83,84,0, 73,0, 
67,76,73,66,0, 85,76,69,0, 70,73,82,83,84,0, 73,0, 77,69,0, 
71,69,0, 84,0, 71,69,0, 72,76,0, 80,0, 76,73,67,0, 72,0, 
76,76,0, 69,76,0, 84,0, 77,0, 69,0, 76,0, 84,0, 77,0, 76,0, 
76,78,0, 77,0, 88,84,0, 84,76,69,0, 69,82,73,76,79,71,0, 
79,82,68,0, 72,71,0, 72,76,0, 68,69,0, 61,0, 61,0, 73,0, 
72,76,0, 71,0, 82,0, 79,67,75,0, 84,69,0, 76,76,0, 69,71,0, 
76,0, 80,0, 71,0, 66,0, 72,79,0, 83,69,0, 70,0, 82,89,0, 
68,69,68,0, 78,0, 78,0, 73,76,76,0, 88,0, 66,67,0, 68,69,0, 
65,76,76,0, 73,0, 73,0, 75,0, 84,0, 77,80,0, 80,0, 83,72,0, 
69,84,0, 70,73,82,83,84,0, 73,0, 67,76,73,66,0, 85,76,69,0, 
70,73,82,83,84,0, 73,0, 77,69,0, 71,69,0, 84,0, 71,69,0, 
72,76,0, 80,0, 76,73,67,0, 72,0, 76,76,0, 69,76,0, 84,0, 77,0, 
69,0, 76,0, 77,0, 76,0, 76,78,0, 77,0, 88,84,0, 84,76,69,0, 
69,82,73,76,79,71,0, 79,82,68,0, 72,71,0, 72,76,0, 92,39,0, 
69,73,76,0, 69,70,73,78,69,68,0, 79,79,82,0, 73,71,72,0, 80,0, 
79,84,0, 65,71,69,0, 81,82,84,0, 84,82,89,0, 84,69,78,68,69,68,0, 
69,88,0, 73,83,84,0, 69,82,73,76,79,71,0, 47,0, 83,83,0, 
65,84,65,0, 69,88,84,0, 68,69,0, 61,0, 83,69,71,0, 79,67,75,0, 
84,69,0, 83,69,71,0, 71,0, 72,79,0, 82,89,0, 85,0, 68,69,68,0, 
78,0, 78,0, 73,76,76,0, 69,88,0, 68,69,0, 75,0, 84,0, 
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
65,71,69,0, 81,82,84,0, 10,0, 42,0, 79,68,0, 69,0, 82,0, 
79,82,0, 92,39,0, 69,73,76,0, 69,70,73,78,69,68,0, 79,79,82,0, 
73,71,72,0, 80,0, 65,71,69,0, 81,82,84,0, 92,39,0, 47,0, 
69,73,76,0, 69,70,73,78,69,68,0, 79,79,82,0, 73,71,72,0, 80,0, 
65,71,69,0, 81,82,84,0, 10,0, 42,0, 78,68,0, 81,0, 79,68,0, 
69,0, 82,0, 79,82,0, 78,68,0, 81,0, 69,0, 82,0, 79,82,0, 
78,68,0, 81,0, 69,0, 82,0, 79,82,0, 81,0, 69,0, 82,0, 
79,82,0, 81,0, 69,0, 82,0, 61,0, 81,0, 69,0, 78,68,0, 81,0, 
79,68,0, 69,0, 82,0, 79,82,0, 10,0, 61,0, 61,0, 83,69,71,0, 
79,67,75,0, 84,69,0, 83,69,71,0, 71,0, 72,79,0, 82,89,0, 85,0, 
68,69,68,0, 78,0, 78,0, 73,76,76,0, 69,88,0, 75,0, 84,0, 
70,73,82,83,84,0, 67,76,73,66,0, 68,85,76,69,0, 70,73,82,83,84,0, 
77,69,0, 80,65,71,69,0, 82,71,0, 71,69,0, 66,76,73,67,0, 84,0, 
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
   38, 38,680,680,680,680,680,680,680,680,  1,422,680,680,680,680,680,680,
  680,680,680,680,680,680,680,680,680,680,680,680,680,680,680,  1,523,681,
  680,526,520,510,682,459,458,518,516,454,517,428,519,683,684,685,685,686,
  686,686,686,687,687,680,424,688,680,689,690,691,692,693,692,694,692,695,
  696,697,696,696,696,696,696,698,696,696,696,699,696,700,696,696,696,701,
  696,696,680,702,680,508,703,691,692,693,692,694,692,695,696,697,696,696,
  696,696,696,698,696,696,696,699,696,700,696,696,696,701,696,696,690,506,
  690,525,704,704,704,704,704,704,704,704,704,704,704,704,704,704,704,704,
  704,704,704,704,704,704,704,704,704,704,704,704,704,704,704,704,704,704,
  704,704,704,704,704,704,704,704,704,704,704,704,704,704,704,704,704,704,
  704,704,704,704,704,704,704,704,704,704,704,704,704,704,704,704,704,704,
  704,704,704,704,704,704,704,704,704,704,704,704,704,704,704,704,704,704,
  704,704,704,704,704,704,704,704,704,704,704,704,704,704,704,704,704,704,
  704,704,704,704,704,704,704,704,704,704,704,704,704,704,704,704,704,704,
  704,704,704,704,704
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
703,701,700,699,698,697,696,695,694,693,692,690,437,433,432,428,426,424,423,
  422,163,162,161,134,38,1,0,51,52,
1,0,
703,701,700,699,698,697,696,695,694,693,692,690,437,428,426,424,423,422,163,
  162,161,134,64,38,1,0,11,34,35,36,37,51,52,53,65,66,133,427,441,
457,456,455,453,452,451,450,449,448,447,446,445,439,432,1,0,51,52,
703,701,700,699,698,697,696,695,694,693,692,690,687,686,685,684,683,465,464,
  431,428,426,424,423,422,160,134,38,1,0,137,
457,456,455,453,452,451,450,449,448,447,446,445,439,432,0,60,67,68,74,76,78,
  79,80,81,82,83,84,85,87,88,
703,701,700,699,698,697,696,695,694,693,692,690,679,678,677,676,675,674,673,
  672,671,670,669,668,667,666,665,664,663,662,661,660,659,658,657,656,655,
  654,653,652,651,650,649,648,647,646,645,644,643,642,641,640,639,638,637,
  636,635,634,633,632,631,630,629,628,627,626,625,624,623,622,621,620,619,
  618,617,616,615,614,613,612,611,610,609,607,606,605,604,603,602,601,600,
  599,598,597,596,595,593,592,591,590,589,588,587,586,585,584,583,582,581,
  580,579,578,577,576,575,574,573,572,571,570,569,568,567,566,565,564,563,
  562,561,560,559,558,557,556,555,554,553,552,551,550,549,548,547,546,545,
  490,489,488,487,486,485,483,482,481,480,479,478,477,476,475,474,473,472,
  471,470,469,468,467,466,465,464,463,462,461,455,453,452,450,449,446,444,
  443,442,440,438,431,430,429,428,426,424,423,422,134,38,1,0,51,52,
422,0,45,
433,432,428,426,424,423,422,38,0,39,40,41,45,46,47,49,54,55,
703,701,700,699,698,697,696,695,694,693,692,690,679,678,677,676,675,674,673,
  672,671,670,669,668,667,666,665,664,663,662,661,660,659,658,657,656,655,
  654,653,652,651,650,649,648,647,646,645,644,643,642,641,640,639,638,637,
  636,635,634,633,632,631,630,629,628,627,626,625,624,623,622,621,620,619,
  618,617,616,615,614,613,612,611,610,609,607,606,605,604,603,602,601,600,
  599,598,597,596,595,593,592,591,590,589,588,587,586,585,584,583,582,581,
  580,579,578,577,576,575,574,573,572,571,570,569,568,567,566,565,564,563,
  562,561,560,559,558,557,556,555,554,553,552,551,550,549,548,547,546,545,
  490,489,488,487,486,485,483,482,481,480,479,478,477,476,475,474,473,472,
  471,470,469,468,467,466,465,464,463,462,461,455,453,452,450,449,446,444,
  443,442,440,438,431,430,429,428,426,424,423,422,134,38,1,0,51,52,
703,701,700,699,698,697,696,695,694,693,692,690,679,678,677,676,675,674,673,
  672,671,670,669,668,667,666,665,664,663,662,661,660,659,658,657,656,655,
  654,653,652,651,650,649,648,647,646,645,644,643,642,641,640,639,638,637,
  636,635,634,633,632,631,630,629,628,627,626,625,624,623,622,621,620,619,
  618,617,616,615,614,613,612,611,610,609,607,606,605,604,603,602,601,600,
  599,598,597,596,595,593,592,591,590,589,588,587,586,585,584,583,582,581,
  580,579,578,577,576,575,574,573,572,571,570,569,568,567,566,565,564,563,
  562,561,560,559,558,557,556,555,554,553,552,551,550,549,548,547,546,545,
  490,489,488,487,486,485,483,482,481,480,479,478,477,476,475,474,473,472,
  471,470,469,468,467,466,465,464,463,462,461,455,453,452,450,449,446,444,
  443,442,440,438,433,432,428,426,424,423,422,134,38,1,0,21,30,31,32,33,
  45,54,55,63,76,80,81,89,97,133,243,256,257,258,259,260,261,262,263,264,
  265,267,268,269,270,271,272,273,274,275,276,277,278,279,280,281,282,283,
  284,285,286,287,288,289,290,291,292,293,294,295,296,297,298,299,300,301,
  302,303,304,305,306,307,308,309,310,311,312,313,314,315,316,317,318,319,
  320,321,322,323,324,325,326,327,328,329,330,331,332,333,334,335,336,337,
  338,339,340,341,342,343,345,346,347,348,349,350,351,352,353,354,355,356,
  357,358,359,360,361,362,363,364,365,366,367,368,369,370,371,372,373,374,
  375,376,377,378,379,380,381,382,383,384,385,386,387,388,389,390,391,392,
  393,394,395,396,397,398,399,400,401,402,403,404,405,406,407,408,409,410,
  411,441,
703,701,700,699,698,697,696,695,694,693,692,690,437,428,426,424,423,422,163,
  162,161,134,64,38,1,0,11,35,36,51,52,53,65,66,133,427,441,
703,701,700,699,698,697,696,695,694,693,692,690,134,1,0,51,52,
703,701,700,699,698,697,696,695,694,693,692,690,134,1,0,51,52,
681,1,0,51,52,
681,1,0,51,52,
703,701,700,699,698,697,696,695,694,693,692,691,690,687,686,685,684,683,682,
  681,537,535,534,533,532,531,530,529,528,527,526,525,524,523,520,517,516,
  490,459,428,230,134,1,0,51,52,
681,1,0,51,52,
426,424,423,422,38,1,0,51,52,
426,424,423,422,38,1,0,51,52,
703,701,700,699,698,697,696,695,694,693,692,690,134,1,0,51,52,
703,701,700,699,698,697,696,695,694,693,692,691,690,687,686,685,684,683,682,
  537,535,534,533,532,531,530,529,528,527,526,525,524,523,520,517,516,490,
  459,428,230,134,1,0,51,52,
703,701,700,699,698,697,696,695,694,693,692,691,690,687,686,685,684,683,682,
  537,535,534,533,532,531,530,529,528,527,526,525,524,523,520,517,516,490,
  459,428,230,134,1,0,51,52,
703,701,700,699,698,697,696,695,694,693,692,690,134,1,0,51,52,
444,443,442,440,438,1,0,51,52,
703,702,701,700,699,698,697,696,695,694,693,692,688,687,686,685,684,683,681,
  519,428,1,0,51,52,
703,701,700,699,698,697,696,695,694,693,692,690,134,0,21,89,133,441,
703,701,700,699,698,697,696,695,694,693,692,690,134,0,21,133,441,
681,1,0,51,52,
681,1,0,51,52,
703,701,700,699,698,697,696,695,694,693,692,691,690,687,686,685,684,683,682,
  681,537,535,534,533,532,531,530,529,528,527,526,525,524,523,520,517,516,
  490,459,428,230,134,1,0,51,52,
681,0,10,12,434,
426,424,423,422,38,1,0,51,52,
426,424,423,422,38,1,0,51,52,
703,701,700,699,698,697,696,695,694,693,692,690,134,0,21,133,441,
703,701,700,699,698,697,696,695,694,693,692,691,690,687,686,685,684,683,682,
  537,535,534,533,532,531,530,529,528,527,526,525,524,523,520,517,516,490,
  459,428,230,134,0,2,6,7,8,9,15,17,21,23,24,25,26,27,28,29,57,77,89,91,
  131,133,153,165,195,197,202,203,204,205,206,207,208,209,210,211,212,213,
  214,215,216,217,218,441,491,538,
703,701,700,699,698,697,696,695,694,693,692,691,690,687,686,685,684,683,682,
  537,535,534,533,532,531,530,529,528,527,526,525,524,523,520,517,516,490,
  459,428,230,134,0,2,6,7,8,9,15,17,21,23,24,25,26,27,28,29,57,77,89,91,
  131,133,153,165,195,197,202,203,204,205,206,207,208,209,210,211,212,213,
  214,215,216,217,218,441,491,538,
703,701,700,699,698,697,696,695,694,693,692,690,134,0,21,133,441,
444,443,442,440,438,0,69,70,71,72,73,
688,681,0,10,12,13,14,434,435,
704,703,702,701,700,699,698,697,696,695,694,693,692,691,690,689,688,687,686,
  685,684,683,682,681,680,526,525,523,520,519,518,517,516,510,508,506,459,
  458,454,437,433,432,428,426,425,424,423,422,163,162,161,134,38,1,0,51,
  52,
704,703,702,701,700,699,698,697,696,695,694,693,692,691,690,689,688,687,686,
  685,684,683,682,681,680,526,525,523,520,519,518,517,516,510,508,506,490,
  489,488,487,486,485,483,482,481,480,479,478,477,476,475,474,473,472,471,
  470,469,468,467,466,465,464,463,462,461,460,459,458,455,454,453,452,444,
  443,442,440,438,433,432,430,429,428,426,425,424,423,422,38,1,0,51,52,
433,432,0,60,61,62,
704,703,702,701,700,699,698,697,696,695,694,693,692,691,690,689,688,687,686,
  685,684,683,682,681,680,526,525,523,520,519,518,517,516,510,508,506,459,
  458,454,428,426,425,424,422,1,0,51,52,
704,703,702,701,700,699,698,697,696,695,694,693,692,691,690,689,688,687,686,
  685,684,683,682,681,680,526,525,523,520,519,518,517,516,510,508,506,460,
  459,458,454,428,426,425,424,423,422,38,1,0,51,52,
704,703,702,701,700,699,698,697,696,695,694,693,692,691,690,689,688,687,686,
  685,684,683,682,681,680,526,525,523,520,519,518,517,516,510,508,506,459,
  458,454,428,424,422,1,0,51,52,
704,703,702,701,700,699,698,697,696,695,694,693,692,691,690,689,688,687,686,
  685,684,683,682,681,680,526,525,523,520,519,518,517,516,510,508,506,459,
  458,454,428,426,425,424,422,1,0,40,45,46,47,48,49,54,86,91,93,139,140,
  142,144,145,146,147,148,149,153,156,157,184,186,188,194,195,196,198,199,
  202,204,219,223,224,228,412,413,414,415,416,417,418,419,420,421,
704,703,702,701,700,699,698,697,696,695,694,693,692,691,690,689,688,687,686,
  685,684,683,682,681,680,526,525,523,520,519,518,517,516,510,508,506,459,
  458,454,428,424,422,1,0,43,44,46,54,86,91,93,139,140,142,144,145,146,
  147,148,149,153,156,157,184,186,188,194,195,196,198,199,202,204,219,223,
  224,228,412,413,414,415,416,417,418,419,420,421,
704,703,702,701,700,699,698,697,696,695,694,693,692,691,690,689,688,687,686,
  685,684,683,682,681,680,526,525,523,520,519,518,517,516,510,508,506,459,
  458,454,428,424,422,1,0,43,44,46,54,86,91,93,139,140,142,144,145,146,
  147,148,149,153,156,157,184,186,188,194,195,196,198,199,202,204,219,223,
  224,228,412,413,414,415,416,417,418,419,420,421,
703,701,700,699,698,697,696,695,694,693,692,690,679,678,677,676,675,674,673,
  672,671,670,669,668,667,666,665,664,663,662,661,660,659,658,657,656,655,
  654,653,652,651,650,649,648,647,646,645,644,643,642,641,640,639,638,637,
  636,635,634,633,632,631,630,629,628,627,626,625,624,623,622,621,620,619,
  618,617,616,615,614,613,612,611,610,609,607,606,605,604,603,602,601,600,
  599,598,597,596,595,593,592,591,590,589,588,587,586,585,584,583,582,581,
  580,579,578,577,576,575,574,573,572,571,570,569,568,567,566,565,564,563,
  562,561,560,559,558,557,556,555,554,553,552,551,550,549,548,547,546,545,
  490,489,488,487,486,485,483,482,481,480,479,478,477,476,475,474,473,472,
  471,470,469,468,467,466,465,464,463,462,461,455,453,452,450,449,446,444,
  443,442,440,438,431,430,429,428,426,424,423,422,134,38,0,21,30,31,32,33,
  39,40,41,45,46,47,49,54,55,59,63,76,80,81,89,97,133,243,256,257,258,259,
  260,261,262,263,264,265,267,268,269,270,271,272,273,274,275,276,277,278,
  279,280,281,282,283,284,285,286,287,288,289,290,291,292,293,294,295,296,
  297,298,299,300,301,302,303,304,305,306,307,308,309,310,311,312,313,314,
  315,316,317,318,319,320,321,322,323,324,325,326,327,328,329,330,331,332,
  333,334,335,336,337,338,339,340,341,342,343,345,346,347,348,349,350,351,
  352,353,354,355,356,357,358,359,360,361,362,363,364,365,366,367,368,369,
  370,371,372,373,374,375,376,377,378,379,380,381,382,383,384,385,386,387,
  388,389,390,391,392,393,394,395,396,397,398,399,400,401,402,403,404,405,
  406,407,408,409,410,411,441,
493,490,426,424,423,422,38,1,0,51,52,
544,426,424,423,422,38,1,0,51,52,
544,426,424,423,422,38,1,0,51,52,
544,426,424,423,422,38,1,0,51,52,
544,426,424,423,422,38,1,0,51,52,
544,426,424,423,422,38,1,0,51,52,
544,426,424,423,422,38,1,0,51,52,
544,426,424,423,422,38,1,0,51,52,
544,426,424,423,422,38,1,0,51,52,
544,426,424,423,422,38,1,0,51,52,
544,426,424,423,422,38,1,0,51,52,
544,426,424,423,422,38,1,0,51,52,
544,426,424,423,422,38,1,0,51,52,
544,426,424,423,422,38,1,0,51,52,
544,426,424,423,422,38,1,0,51,52,
668,543,542,541,247,245,237,235,233,1,0,51,52,
540,240,239,238,237,236,235,234,233,1,0,51,52,
668,543,247,245,237,235,233,1,0,51,52,
703,701,700,699,698,697,696,695,694,693,692,691,690,687,686,685,684,683,682,
  537,535,534,533,532,531,530,529,528,527,526,525,524,523,520,517,516,490,
  459,428,230,134,1,0,51,52,
703,701,700,699,698,697,696,695,694,693,692,691,690,687,686,685,684,683,682,
  537,535,534,533,532,531,530,529,528,527,526,525,524,523,520,517,516,490,
  459,428,230,134,1,0,51,52,
703,701,700,699,698,697,696,695,694,693,692,691,690,687,686,685,684,683,682,
  537,535,534,533,532,531,530,529,528,527,526,525,524,523,520,517,516,490,
  459,428,230,134,1,0,51,52,
703,701,700,699,698,697,696,695,694,693,692,691,690,687,686,685,684,683,682,
  537,535,534,533,532,531,530,529,528,527,526,525,524,523,520,517,516,490,
  459,428,230,134,1,0,51,52,
703,701,700,699,698,697,696,695,694,693,692,691,690,687,686,685,684,683,682,
  537,535,534,533,532,531,530,529,528,527,526,525,524,523,520,517,516,490,
  459,428,230,134,1,0,51,52,
703,701,700,699,698,697,696,695,694,693,692,691,690,687,686,685,684,683,682,
  537,535,534,533,532,531,530,529,528,527,526,525,524,523,520,517,516,490,
  459,428,230,134,1,0,51,52,
703,701,700,699,698,697,696,695,694,693,692,691,690,687,686,685,684,683,682,
  537,535,534,533,532,531,530,529,528,527,526,525,524,523,520,517,516,490,
  459,428,230,134,1,0,51,52,
703,701,700,699,698,697,696,695,694,693,692,691,690,687,686,685,684,683,682,
  537,535,534,533,532,531,530,529,528,527,526,525,524,523,520,517,516,490,
  459,428,230,134,1,0,51,52,
703,701,700,699,698,697,696,695,694,693,692,691,690,687,686,685,684,683,682,
  537,535,534,533,532,531,530,529,528,527,526,525,524,523,520,517,516,490,
  459,428,230,134,1,0,51,52,
703,701,700,699,698,697,696,695,694,693,692,691,690,687,686,685,684,683,682,
  537,535,534,533,532,531,530,529,528,527,526,525,524,523,520,517,516,490,
  459,428,230,134,1,0,51,52,
703,701,700,699,698,697,696,695,694,693,692,691,690,687,686,685,684,683,682,
  537,535,534,533,532,531,530,529,528,527,526,525,524,523,520,517,516,490,
  459,428,230,134,1,0,51,52,
703,701,700,699,698,697,696,695,694,693,692,691,690,687,686,685,684,683,682,
  537,535,534,533,532,531,530,529,528,527,526,525,524,523,520,517,516,490,
  459,428,230,134,1,0,51,52,
703,701,700,699,698,697,696,695,694,693,692,691,690,687,686,685,684,683,682,
  537,535,534,533,532,531,530,529,528,527,526,525,524,523,520,517,516,490,
  459,428,230,134,1,0,51,52,
703,701,700,699,698,697,696,695,694,693,692,691,690,687,686,685,684,683,682,
  537,535,534,533,532,531,530,529,528,527,526,525,524,523,520,517,516,490,
  459,428,230,134,1,0,51,52,
703,701,700,699,698,697,696,695,694,693,692,691,690,687,686,685,684,683,682,
  537,535,534,533,532,531,530,529,528,527,526,525,524,523,520,517,516,490,
  459,428,230,134,1,0,51,52,
703,701,700,699,698,697,696,695,694,693,692,691,690,687,686,685,684,683,682,
  537,535,534,533,532,531,530,529,528,527,526,525,524,523,520,517,516,490,
  459,428,230,134,1,0,51,52,
544,426,424,423,422,38,1,0,51,52,
544,426,424,423,422,38,1,0,51,52,
544,426,424,423,422,38,1,0,51,52,
544,426,424,423,422,38,1,0,51,52,
544,426,424,423,422,38,1,0,51,52,
544,426,424,423,422,38,1,0,51,52,
544,426,424,423,422,38,1,0,51,52,
544,426,424,423,422,38,1,0,51,52,
544,426,424,423,422,38,1,0,51,52,
544,426,424,423,422,38,1,0,51,52,
544,426,424,423,422,38,1,0,51,52,
544,426,424,423,422,38,1,0,51,52,
544,426,424,423,422,38,1,0,51,52,
544,426,424,423,422,38,1,0,51,52,
544,426,424,423,422,38,1,0,51,52,
544,426,424,423,422,38,1,0,51,52,
544,426,424,423,422,38,1,0,51,52,
544,426,424,423,422,38,1,0,51,52,
544,426,424,423,422,38,1,0,51,52,
544,426,424,423,422,38,1,0,51,52,
544,426,424,423,422,38,1,0,51,52,
544,426,424,423,422,38,1,0,51,52,
544,426,424,423,422,38,1,0,51,52,
544,426,424,423,422,38,1,0,51,52,
544,426,424,423,422,38,1,0,51,52,
544,426,424,423,422,38,1,0,51,52,
544,426,424,423,422,38,1,0,51,52,
544,426,424,423,422,38,1,0,51,52,
544,426,424,423,422,38,1,0,51,52,
686,685,684,683,1,0,51,52,
668,543,542,541,247,245,237,235,233,1,0,51,52,
540,240,239,238,237,236,235,234,233,1,0,51,52,
668,543,247,245,237,235,233,1,0,51,52,
703,701,700,699,698,697,696,695,694,693,692,691,690,687,686,685,684,683,682,
  537,535,534,533,532,531,530,529,528,527,526,525,524,523,520,517,516,490,
  459,428,230,134,1,0,51,52,
703,701,700,699,698,697,696,695,694,693,692,691,690,687,686,685,684,683,682,
  537,535,534,533,532,531,530,529,528,527,526,525,524,523,520,517,516,490,
  459,428,230,134,1,0,51,52,
703,701,700,699,698,697,696,695,694,693,692,691,690,687,686,685,684,683,682,
  537,535,534,533,532,531,530,529,528,527,526,525,524,523,520,517,516,490,
  459,428,230,134,1,0,51,52,
703,701,700,699,698,697,696,695,694,693,692,691,690,687,686,685,684,683,682,
  537,535,534,533,532,531,530,529,528,527,526,525,524,523,520,517,516,490,
  459,428,230,134,1,0,51,52,
703,701,700,699,698,697,696,695,694,693,692,691,690,687,686,685,684,683,682,
  537,535,534,533,532,531,530,529,528,527,526,525,524,523,520,517,516,490,
  459,428,230,134,1,0,51,52,
703,701,700,699,698,697,696,695,694,693,692,691,690,687,686,685,684,683,682,
  537,535,534,533,532,531,530,529,528,527,526,525,524,523,520,517,516,490,
  459,428,230,134,1,0,51,52,
703,701,700,699,698,697,696,695,694,693,692,691,690,687,686,685,684,683,682,
  537,535,534,533,532,531,530,529,528,527,526,525,524,523,520,517,516,490,
  459,428,230,134,1,0,51,52,
703,701,700,699,698,697,696,695,694,693,692,691,690,687,686,685,684,683,682,
  537,535,534,533,532,531,530,529,528,527,526,525,524,523,520,517,516,490,
  459,428,230,134,1,0,51,52,
703,701,700,699,698,697,696,695,694,693,692,691,690,687,686,685,684,683,682,
  537,535,534,533,532,531,530,529,528,527,526,525,524,523,520,517,516,490,
  459,428,230,134,1,0,51,52,
703,701,700,699,698,697,696,695,694,693,692,691,690,687,686,685,684,683,682,
  537,535,534,533,532,531,530,529,528,527,526,525,524,523,520,517,516,490,
  459,428,230,134,1,0,51,52,
703,701,700,699,698,697,696,695,694,693,692,691,690,687,686,685,684,683,682,
  537,535,534,533,532,531,530,529,528,527,526,525,524,523,520,517,516,490,
  459,428,230,134,1,0,51,52,
703,701,700,699,698,697,696,695,694,693,692,691,690,687,686,685,684,683,682,
  537,535,534,533,532,531,530,529,528,527,526,525,524,523,520,517,516,490,
  459,428,230,134,1,0,51,52,
703,701,700,699,698,697,696,695,694,693,692,691,690,687,686,685,684,683,682,
  537,535,534,533,532,531,530,529,528,527,526,525,524,523,520,517,516,490,
  459,428,230,134,1,0,51,52,
703,701,700,699,698,697,696,695,694,693,692,691,690,687,686,685,684,683,682,
  537,535,534,533,532,531,530,529,528,527,526,525,524,523,520,517,516,490,
  459,428,230,134,1,0,51,52,
703,701,700,699,698,697,696,695,694,693,692,691,690,687,686,685,684,683,682,
  537,535,534,533,532,531,530,529,528,527,526,525,524,523,520,517,516,490,
  459,428,230,134,1,0,51,52,
703,701,700,699,698,697,696,695,694,693,692,691,690,687,686,685,684,683,682,
  537,535,534,533,532,531,530,529,528,527,526,525,524,523,520,517,516,490,
  459,428,230,134,1,0,51,52,
703,701,700,699,698,697,696,695,694,693,692,691,690,687,686,685,684,683,682,
  537,535,534,533,532,531,530,529,528,527,526,525,524,523,520,517,516,490,
  459,428,230,134,1,0,51,52,
703,701,700,699,698,697,696,695,694,693,692,691,690,687,686,685,684,683,682,
  537,535,534,533,532,531,530,529,528,527,526,525,524,523,520,517,516,490,
  459,428,230,134,1,0,51,52,
703,701,700,699,698,697,696,695,694,693,692,691,690,687,686,685,684,683,682,
  537,535,534,533,532,531,530,529,528,527,526,525,524,523,520,517,516,490,
  459,428,230,134,1,0,51,52,
703,701,700,699,698,697,696,695,694,693,692,691,690,687,686,685,684,683,682,
  537,535,534,533,532,531,530,529,528,527,526,525,524,523,520,517,516,490,
  459,428,230,134,1,0,51,52,
703,701,700,699,698,697,696,695,694,693,692,691,690,687,686,685,684,683,682,
  537,535,534,533,532,531,530,529,528,527,526,525,524,523,520,517,516,490,
  459,428,230,134,1,0,51,52,
703,701,700,699,698,697,696,695,694,693,692,691,690,687,686,685,684,683,682,
  537,535,534,533,532,531,530,529,528,527,526,525,524,523,520,517,516,490,
  459,428,230,134,1,0,51,52,
703,701,700,699,698,697,696,695,694,693,692,691,690,687,686,685,684,683,682,
  537,535,534,533,532,531,530,529,528,527,526,525,524,523,520,517,516,490,
  459,428,230,134,1,0,51,52,
703,701,700,699,698,697,696,695,694,693,692,691,690,687,686,685,684,683,682,
  537,535,534,533,532,531,530,529,528,527,526,525,524,523,520,517,516,490,
  459,428,230,134,1,0,51,52,
703,701,700,699,698,697,696,695,694,693,692,691,690,687,686,685,684,683,682,
  537,535,534,533,532,531,530,529,528,527,526,525,524,523,520,517,516,490,
  459,428,230,134,1,0,51,52,
703,701,700,699,698,697,696,695,694,693,692,691,690,687,686,685,684,683,682,
  537,535,534,533,532,531,530,529,528,527,526,525,524,523,520,517,516,490,
  459,428,230,134,1,0,51,52,
703,701,700,699,698,697,696,695,694,693,692,691,690,687,686,685,684,683,682,
  537,535,534,533,532,531,530,529,528,527,526,525,524,523,520,517,516,490,
  459,428,230,134,1,0,51,52,
703,701,700,699,698,697,696,695,694,693,692,691,690,687,686,685,684,683,682,
  537,535,534,533,532,531,530,529,528,527,526,525,524,523,520,517,516,490,
  459,428,230,134,1,0,51,52,
703,701,700,699,698,697,696,695,694,693,692,691,690,687,686,685,684,683,682,
  537,535,534,533,532,531,530,529,528,527,526,525,524,523,520,517,516,490,
  459,428,230,134,1,0,51,52,
703,701,700,699,698,697,696,695,694,693,692,691,690,687,686,685,684,683,682,
  537,535,534,533,532,531,530,529,528,527,526,525,524,523,520,517,516,490,
  459,428,230,134,1,0,51,52,
703,701,700,699,698,697,696,695,694,693,692,691,690,687,686,685,684,683,682,
  537,535,534,533,532,531,530,529,528,527,526,525,524,523,520,517,516,490,
  459,428,230,134,1,0,51,52,
703,701,700,699,698,697,696,695,694,693,692,691,690,687,686,685,684,683,682,
  537,535,534,533,532,531,530,529,528,527,526,525,524,523,520,517,516,490,
  459,428,230,134,1,0,51,52,
703,701,700,699,698,697,696,695,694,693,692,691,690,687,686,685,684,683,682,
  537,535,534,533,532,531,530,529,528,527,526,525,524,523,520,517,516,490,
  459,428,230,134,1,0,51,52,
703,701,700,699,698,697,696,695,694,693,692,691,690,687,686,685,684,683,682,
  537,535,534,533,532,531,530,529,528,527,526,525,524,523,520,517,516,490,
  459,428,230,134,1,0,51,52,
703,701,700,699,698,697,696,695,694,693,692,691,690,687,686,685,684,683,682,
  537,535,534,533,532,531,530,529,528,527,526,525,524,523,520,517,516,490,
  459,428,230,134,1,0,51,52,
703,701,700,699,698,697,696,695,694,693,692,691,690,687,686,685,684,683,682,
  537,535,534,533,532,531,530,529,528,527,526,525,524,523,520,517,516,490,
  459,428,230,134,1,0,51,52,
703,701,700,699,698,697,696,695,694,693,692,691,690,687,686,685,684,683,682,
  537,535,534,533,532,531,530,529,528,527,526,525,524,523,520,517,516,490,
  459,428,230,134,1,0,51,52,
703,701,700,699,698,697,696,695,694,693,692,691,690,687,686,685,684,683,682,
  537,535,534,533,532,531,530,529,528,527,526,525,524,523,520,517,516,490,
  459,428,230,134,1,0,51,52,
703,701,700,699,698,697,696,695,694,693,692,691,690,687,686,685,684,683,682,
  537,535,534,533,532,531,530,529,528,527,526,525,524,523,520,517,516,490,
  459,428,230,134,1,0,51,52,
703,701,700,699,698,697,696,695,694,693,692,691,690,687,686,685,684,683,682,
  537,535,534,533,532,531,530,529,528,527,526,525,524,523,520,517,516,490,
  459,428,230,134,1,0,51,52,
703,701,700,699,698,697,696,695,694,693,692,691,690,687,686,685,684,683,682,
  537,535,534,533,532,531,530,529,528,527,526,525,524,523,520,517,516,490,
  459,428,230,134,1,0,51,52,
703,701,700,699,698,697,696,695,694,693,692,691,690,687,686,685,684,683,682,
  537,535,534,533,532,531,530,529,528,527,526,525,524,523,520,517,516,490,
  459,428,230,134,1,0,51,52,
703,701,700,699,698,697,696,695,694,693,692,691,690,687,686,685,684,683,682,
  537,535,534,533,532,531,530,529,528,527,526,525,524,523,520,517,516,490,
  459,428,230,134,1,0,51,52,
703,701,700,699,698,697,696,695,694,693,692,691,690,687,686,685,684,683,682,
  537,535,534,533,532,531,530,529,528,527,526,525,524,523,520,517,516,490,
  459,428,230,134,1,0,51,52,
703,701,700,699,698,697,696,695,694,693,692,691,690,687,686,685,684,683,682,
  537,535,534,533,532,531,530,529,528,527,526,525,524,523,520,517,516,490,
  459,428,230,134,1,0,51,52,
703,701,700,699,698,697,696,695,694,693,692,691,690,687,686,685,684,683,682,
  537,535,534,533,532,531,530,529,528,527,526,525,524,523,520,517,516,490,
  459,428,230,134,1,0,51,52,
703,701,700,699,698,697,696,695,694,693,692,691,690,687,686,685,684,683,682,
  537,535,534,533,532,531,530,529,528,527,526,525,524,523,520,517,516,490,
  459,428,230,134,1,0,51,52,
703,701,700,699,698,697,696,695,694,693,692,691,690,687,686,685,684,683,682,
  537,535,534,533,532,531,530,529,528,527,526,525,524,523,520,517,516,490,
  459,428,230,134,1,0,51,52,
668,540,539,247,245,237,235,233,1,0,51,52,
668,540,539,247,245,237,235,233,1,0,51,52,
668,540,539,247,245,237,235,233,1,0,51,52,
668,540,539,247,245,237,235,233,1,0,51,52,
668,245,235,233,1,0,51,52,
668,245,235,233,1,0,51,52,
668,543,247,245,237,235,233,1,0,51,52,
668,543,247,245,237,235,233,1,0,51,52,
668,543,247,245,237,235,233,1,0,51,52,
668,543,542,541,247,245,237,235,233,1,0,51,52,
668,543,542,541,247,245,237,235,233,1,0,51,52,
540,240,239,238,237,236,235,234,233,1,0,51,52,
540,240,239,238,237,236,235,234,233,1,0,51,52,
540,240,239,238,237,236,235,234,233,1,0,51,52,
540,240,239,238,237,236,235,234,233,1,0,51,52,
540,240,239,238,237,236,235,234,233,1,0,51,52,
540,240,239,238,237,236,235,234,233,1,0,51,52,
540,240,239,238,237,236,235,234,233,1,0,51,52,
540,240,239,238,237,236,235,234,233,1,0,51,52,
540,240,239,238,237,236,235,234,233,1,0,51,52,
540,240,239,238,237,236,235,234,233,1,0,51,52,
540,240,239,238,237,236,235,234,233,1,0,51,52,
686,685,684,683,1,0,51,52,
454,1,0,51,52,
454,1,0,51,52,
454,1,0,51,52,
703,701,700,699,698,697,696,695,694,693,692,691,690,687,686,685,684,683,682,
  537,535,534,533,532,531,530,529,528,527,526,525,524,523,520,517,516,490,
  459,428,230,134,1,0,51,52,
703,701,700,699,698,697,696,695,694,693,692,691,690,687,686,685,684,683,682,
  537,535,534,533,532,531,530,529,528,527,526,525,524,523,520,517,516,490,
  459,428,230,134,1,0,51,52,
703,701,700,699,698,697,696,695,694,693,692,691,690,687,686,685,684,683,682,
  537,535,534,533,532,531,530,529,528,527,526,525,524,523,520,517,516,490,
  459,428,230,134,1,0,51,52,
703,701,700,699,698,697,696,695,694,693,692,691,690,687,686,685,684,683,682,
  537,535,534,533,532,531,530,529,528,527,526,525,524,523,520,517,516,490,
  459,428,230,134,1,0,51,52,
703,701,700,699,698,697,696,695,694,693,692,691,690,687,686,685,684,683,682,
  537,535,534,533,532,531,530,529,528,527,526,525,524,523,520,517,516,490,
  459,428,230,134,1,0,51,52,
703,701,700,699,698,697,696,695,694,693,692,691,690,687,686,685,684,683,682,
  537,535,534,533,532,531,530,529,528,527,526,525,524,523,520,517,516,490,
  459,428,230,134,1,0,51,52,
703,701,700,699,698,697,696,695,694,693,692,691,690,687,686,685,684,683,682,
  537,535,534,533,532,531,530,529,528,527,526,525,524,523,520,517,516,490,
  459,428,230,134,1,0,51,52,
703,701,700,699,698,697,696,695,694,693,692,691,690,687,686,685,684,683,682,
  537,535,534,533,532,531,530,529,528,527,526,525,524,523,520,517,516,490,
  459,428,230,134,1,0,51,52,
703,701,700,699,698,697,696,695,694,693,692,691,690,687,686,685,684,683,682,
  537,535,534,533,532,531,530,529,528,527,526,525,524,523,520,517,516,490,
  459,428,230,134,1,0,51,52,
703,701,700,699,698,697,696,695,694,693,692,691,690,687,686,685,684,683,682,
  537,535,534,533,532,531,530,529,528,527,526,525,524,523,520,517,516,490,
  459,428,230,134,1,0,51,52,
703,701,700,699,698,697,696,695,694,693,692,691,690,687,686,685,684,683,682,
  537,535,534,533,532,531,530,529,528,527,526,525,524,523,520,517,516,490,
  459,428,230,134,1,0,51,52,
703,701,700,699,698,697,696,695,694,693,692,691,690,687,686,685,684,683,682,
  537,535,534,533,532,531,530,529,528,527,526,525,524,523,520,517,516,490,
  459,428,230,134,1,0,51,52,
703,701,700,699,698,697,696,695,694,693,692,691,690,687,686,685,684,683,682,
  537,535,534,533,532,531,530,529,528,527,526,525,524,523,520,517,516,490,
  459,428,230,134,1,0,51,52,
703,701,700,699,698,697,696,695,694,693,692,691,690,687,686,685,684,683,682,
  537,535,534,533,532,531,530,529,528,527,526,525,524,523,520,517,516,490,
  459,428,230,134,1,0,51,52,
703,701,700,699,698,697,696,695,694,693,692,691,690,687,686,685,684,683,682,
  537,535,534,533,532,531,530,529,528,527,526,525,524,523,520,517,516,490,
  459,428,230,134,1,0,51,52,
703,701,700,699,698,697,696,695,694,693,692,691,690,687,686,685,684,683,682,
  537,535,534,533,532,531,530,529,528,527,526,525,524,523,520,517,516,490,
  459,428,230,134,1,0,51,52,
703,701,700,699,698,697,696,695,694,693,692,691,690,687,686,685,684,683,682,
  537,535,534,533,532,531,530,529,528,527,526,525,524,523,520,517,516,490,
  459,428,230,134,1,0,51,52,
703,701,700,699,698,697,696,695,694,693,692,691,690,687,686,685,684,683,682,
  537,535,534,533,532,531,530,529,528,527,526,525,524,523,520,517,516,490,
  459,428,230,134,1,0,51,52,
703,701,700,699,698,697,696,695,694,693,692,691,690,687,686,685,684,683,682,
  537,535,534,533,532,531,530,529,528,527,526,525,524,523,520,517,516,490,
  459,428,230,134,1,0,51,52,
703,701,700,699,698,697,696,695,694,693,692,691,690,687,686,685,684,683,682,
  537,535,534,533,532,531,530,529,528,527,526,525,524,523,520,517,516,490,
  459,428,230,134,1,0,51,52,
703,701,700,699,698,697,696,695,694,693,692,691,690,687,686,685,684,683,682,
  537,535,534,533,532,531,530,529,528,527,526,525,524,523,520,517,516,490,
  459,428,230,134,1,0,51,52,
703,701,700,699,698,697,696,695,694,693,692,691,690,687,686,685,684,683,682,
  537,535,534,533,532,531,530,529,528,527,526,525,524,523,520,517,516,490,
  459,428,230,134,1,0,51,52,
703,701,700,699,698,697,696,695,694,693,692,691,690,687,686,685,684,683,682,
  537,535,534,533,532,531,530,529,528,527,526,525,524,523,520,517,516,490,
  459,428,230,134,1,0,51,52,
703,701,700,699,698,697,696,695,694,693,692,691,690,687,686,685,684,683,682,
  537,535,534,533,532,531,530,529,528,527,526,525,524,523,520,517,516,490,
  459,428,230,134,1,0,51,52,
703,701,700,699,698,697,696,695,694,693,692,691,690,687,686,685,684,683,682,
  537,535,534,533,532,531,530,529,528,527,526,525,524,523,520,517,516,490,
  459,428,230,134,1,0,51,52,
703,701,700,699,698,697,696,695,694,693,692,691,690,687,686,685,684,683,682,
  537,535,534,533,532,531,530,529,528,527,526,525,524,523,520,517,516,490,
  459,428,230,134,1,0,51,52,
703,701,700,699,698,697,696,695,694,693,692,691,690,687,686,685,684,683,682,
  537,535,534,533,532,531,530,529,528,527,526,525,524,523,520,517,516,490,
  459,428,230,134,1,0,51,52,
703,701,700,699,698,697,696,695,694,693,692,691,690,687,686,685,684,683,682,
  537,535,534,533,532,531,530,529,528,527,526,525,524,523,520,517,516,490,
  459,428,230,134,1,0,51,52,
703,701,700,699,698,697,696,695,694,693,692,691,690,687,686,685,684,683,682,
  537,535,534,533,532,531,530,529,528,527,526,525,524,523,520,517,516,490,
  459,428,230,134,1,0,51,52,
703,701,700,699,698,697,696,695,694,693,692,691,690,687,686,685,684,683,682,
  537,535,534,533,532,531,530,529,528,527,526,525,524,523,520,517,516,490,
  459,428,230,134,1,0,51,52,
703,701,700,699,698,697,696,695,694,693,692,691,690,687,686,685,684,683,682,
  537,535,534,533,532,531,530,529,528,527,526,525,524,523,520,517,516,490,
  459,428,230,134,1,0,51,52,
703,701,700,699,698,697,696,695,694,693,692,691,690,687,686,685,684,683,682,
  537,535,534,533,532,531,530,529,528,527,526,525,524,523,520,517,516,490,
  459,428,230,134,1,0,51,52,
703,701,700,699,698,697,696,695,694,693,692,691,690,687,686,685,684,683,682,
  537,535,534,533,532,531,530,529,528,527,526,525,524,523,520,517,516,490,
  459,428,230,134,1,0,51,52,
703,701,700,699,698,697,696,695,694,693,692,691,690,687,686,685,684,683,682,
  537,535,534,533,532,531,530,529,528,527,526,525,524,523,520,517,516,490,
  459,428,230,134,1,0,51,52,
703,701,700,699,698,697,696,695,694,693,692,691,690,687,686,685,684,683,682,
  537,535,534,533,532,531,530,529,528,527,526,525,524,523,520,517,516,490,
  459,428,230,134,1,0,51,52,
703,701,700,699,698,697,696,695,694,693,692,691,690,687,686,685,684,683,682,
  537,535,534,533,532,531,530,529,528,527,526,525,524,523,520,517,516,490,
  459,428,230,134,1,0,51,52,
703,701,700,699,698,697,696,695,694,693,692,691,690,687,686,685,684,683,682,
  537,535,534,533,532,531,530,529,528,527,526,525,524,523,520,517,516,490,
  459,428,230,134,1,0,51,52,
703,701,700,699,698,697,696,695,694,693,692,691,690,687,686,685,684,683,682,
  537,535,534,533,532,531,530,529,528,527,526,525,524,523,520,517,516,490,
  459,428,230,134,1,0,51,52,
703,701,700,699,698,697,696,695,694,693,692,691,690,687,686,685,684,683,682,
  537,535,534,533,532,531,530,529,528,527,526,525,524,523,520,517,516,490,
  459,428,230,134,1,0,51,52,
703,701,700,699,698,697,696,695,694,693,692,691,690,687,686,685,684,683,682,
  537,535,534,533,532,531,530,529,528,527,526,525,524,523,520,517,516,490,
  459,428,230,134,1,0,51,52,
703,701,700,699,698,697,696,695,694,693,692,691,690,687,686,685,684,683,682,
  537,535,534,533,532,531,530,529,528,527,526,525,524,523,520,517,516,490,
  459,428,230,134,1,0,51,52,
703,701,700,699,698,697,696,695,694,693,692,691,690,687,686,685,684,683,682,
  537,535,534,533,532,531,530,529,528,527,526,525,524,523,520,517,516,490,
  459,428,230,134,1,0,51,52,
703,701,700,699,698,697,696,695,694,693,692,691,690,687,686,685,684,683,682,
  537,535,534,533,532,531,530,529,528,527,526,525,524,523,520,517,516,490,
  459,428,230,134,1,0,51,52,
703,701,700,699,698,697,696,695,694,693,692,691,690,687,686,685,684,683,682,
  537,535,534,533,532,531,530,529,528,527,526,525,524,523,520,517,516,490,
  459,428,230,134,1,0,51,52,
703,701,700,699,698,697,696,695,694,693,692,691,690,687,686,685,684,683,682,
  537,535,534,533,532,531,530,529,528,527,526,525,524,523,520,517,516,490,
  459,428,230,134,1,0,51,52,
703,701,700,699,698,697,696,695,694,693,692,691,690,687,686,685,684,683,682,
  537,535,534,533,532,531,530,529,528,527,526,525,524,523,520,517,516,490,
  459,428,230,134,1,0,51,52,
703,701,700,699,698,697,696,695,694,693,692,691,690,687,686,685,684,683,682,
  537,535,534,533,532,531,530,529,528,527,526,525,524,523,520,517,516,490,
  459,428,230,134,1,0,51,52,
703,701,700,699,698,697,696,695,694,693,692,691,690,687,686,685,684,683,682,
  537,535,534,533,532,531,530,529,528,527,526,525,524,523,520,517,516,490,
  459,428,230,134,1,0,51,52,
703,701,700,699,698,697,696,695,694,693,692,691,690,687,686,685,684,683,682,
  537,535,534,533,532,531,530,529,528,527,526,525,524,523,520,517,516,490,
  459,428,230,134,1,0,51,52,
703,701,700,699,698,697,696,695,694,693,692,691,690,687,686,685,684,683,682,
  537,535,534,533,532,531,530,529,528,527,526,525,524,523,520,517,516,490,
  459,428,230,134,1,0,51,52,
703,701,700,699,698,697,696,695,694,693,692,691,690,687,686,685,684,683,682,
  537,535,534,533,532,531,530,529,528,527,526,525,524,523,520,517,516,490,
  459,428,230,134,1,0,51,52,
703,701,700,699,698,697,696,695,694,693,692,691,690,687,686,685,684,683,682,
  537,535,534,533,532,531,530,529,528,527,526,525,524,523,520,517,516,490,
  459,428,230,134,1,0,51,52,
668,540,539,247,245,237,235,233,1,0,51,52,
668,540,539,247,245,237,235,233,1,0,51,52,
668,540,539,247,245,237,235,233,1,0,51,52,
668,540,539,247,245,237,235,233,1,0,51,52,
668,245,235,233,1,0,51,52,
668,245,235,233,1,0,51,52,
668,543,247,245,237,235,233,1,0,51,52,
668,543,247,245,237,235,233,1,0,51,52,
668,543,247,245,237,235,233,1,0,51,52,
668,543,542,541,247,245,237,235,233,1,0,51,52,
668,543,542,541,247,245,237,235,233,1,0,51,52,
540,240,239,238,237,236,235,234,233,1,0,51,52,
540,240,239,238,237,236,235,234,233,1,0,51,52,
540,240,239,238,237,236,235,234,233,1,0,51,52,
540,240,239,238,237,236,235,234,233,1,0,51,52,
540,240,239,238,237,236,235,234,233,1,0,51,52,
540,240,239,238,237,236,235,234,233,1,0,51,52,
540,240,239,238,237,236,235,234,233,1,0,51,52,
540,240,239,238,237,236,235,234,233,1,0,51,52,
540,240,239,238,237,236,235,234,233,1,0,51,52,
540,240,239,238,237,236,235,234,233,1,0,51,52,
540,240,239,238,237,236,235,234,233,1,0,51,52,
704,703,701,700,699,698,697,696,695,694,693,692,691,690,689,688,687,686,685,
  684,683,682,681,680,544,526,525,523,522,520,519,518,517,516,514,512,510,
  508,506,505,503,501,499,497,495,492,459,458,454,431,428,426,424,423,422,
  38,1,0,51,52,137,
544,426,424,423,422,38,1,0,51,52,
704,703,701,700,699,698,697,696,695,694,693,692,691,690,689,688,687,686,685,
  684,683,682,681,680,544,526,525,523,522,521,520,519,518,517,516,515,514,
  513,512,511,510,509,508,507,506,505,504,503,502,501,500,499,498,497,496,
  495,494,492,459,458,454,431,428,426,424,423,422,38,1,0,91,
426,424,423,422,38,1,0,51,
544,0,266,
703,701,700,699,698,697,696,695,694,693,692,691,690,687,686,685,684,683,682,
  537,535,534,533,532,531,530,529,528,527,526,525,524,523,520,517,516,490,
  459,428,230,134,0,2,6,7,8,9,15,17,21,23,24,25,26,27,28,29,57,77,89,91,
  131,133,153,165,195,197,202,203,204,205,206,207,208,209,210,211,212,213,
  214,215,216,217,218,441,491,538,
490,489,488,487,486,485,483,482,481,480,479,478,477,476,475,474,473,472,471,
  470,469,468,467,466,465,464,463,462,461,455,453,452,444,443,442,440,438,
  0,69,70,71,72,73,83,84,85,98,99,100,101,102,103,104,105,106,107,108,109,
  110,111,112,113,114,115,117,118,119,120,121,122,123,124,125,126,127,128,
  129,130,131,
426,424,423,422,38,1,0,51,52,
704,703,701,700,699,698,697,696,695,694,693,692,691,690,689,688,687,686,685,
  684,683,682,681,680,526,525,523,520,519,518,517,516,510,508,506,459,458,
  454,428,424,1,0,51,52,
426,424,423,422,38,1,0,51,52,
681,0,10,12,434,
681,0,10,12,434,
703,701,700,699,698,697,696,695,694,693,692,691,690,687,686,685,684,683,682,
  681,537,535,534,533,532,531,530,529,528,527,526,525,524,523,520,517,516,
  490,459,428,230,134,0,2,6,7,8,9,10,12,15,17,21,23,24,25,26,27,28,29,57,
  89,91,131,133,153,195,197,202,203,204,205,206,207,208,209,210,211,212,
  213,214,215,216,217,218,434,441,491,538,
703,702,701,700,699,698,697,696,695,694,693,692,691,690,689,688,687,686,685,
  684,683,682,681,680,526,525,523,520,519,518,517,516,510,508,506,459,458,
  454,428,424,422,1,0,141,
492,458,454,426,424,423,422,38,1,0,51,52,
426,424,423,422,38,1,0,51,52,
426,424,423,422,38,0,39,40,41,45,46,47,49,
426,424,423,422,38,0,39,40,41,45,46,47,49,
426,424,423,422,38,1,0,51,52,
687,686,685,684,683,0,
686,685,684,683,0,
684,683,0,
703,701,697,695,694,693,692,687,686,685,684,683,544,522,521,520,519,518,517,
  516,515,514,513,512,511,510,509,508,507,506,505,504,503,502,501,500,499,
  498,497,496,495,494,492,458,431,428,426,424,423,422,221,220,38,1,0,
687,686,685,684,683,0,
703,702,701,700,699,698,697,696,695,694,693,692,691,690,689,688,687,686,685,
  684,683,681,680,526,525,523,520,519,518,517,516,510,508,506,459,458,454,
  428,424,422,232,1,0,16,
687,686,685,684,683,0,
697,695,694,693,692,687,686,685,684,683,0,
686,685,684,683,544,522,521,520,519,518,517,516,515,514,513,512,511,510,509,
  508,507,506,505,504,503,502,501,500,499,498,497,496,495,494,492,458,431,
  426,424,423,422,38,1,0,
684,683,544,522,521,520,519,518,517,516,515,514,513,512,511,510,509,508,507,
  506,505,504,503,502,501,500,499,498,497,496,495,494,492,458,431,426,424,
  423,422,38,1,0,
695,694,693,692,687,686,685,684,683,544,522,521,520,519,518,517,516,515,514,
  513,512,511,510,509,508,507,506,505,504,503,502,501,500,499,498,497,496,
  495,494,492,458,431,426,424,423,422,38,1,0,
703,697,695,694,693,692,687,686,685,684,683,544,522,521,520,519,518,517,516,
  515,514,513,512,510,509,508,507,506,505,504,503,502,501,500,499,498,497,
  496,495,492,458,431,428,426,424,423,422,221,220,38,1,0,222,
459,1,0,51,52,
459,426,424,423,422,38,1,0,51,52,
459,1,0,51,52,
459,1,0,51,52,
459,1,0,51,52,
459,1,0,51,52,
459,1,0,51,52,
459,1,0,51,52,
459,1,0,51,52,
459,1,0,51,52,
459,1,0,51,52,
544,522,521,520,519,518,517,516,515,514,513,512,511,510,509,508,507,506,505,
  504,503,502,501,500,499,498,497,496,495,494,492,458,431,426,424,423,422,
  38,1,0,51,52,
544,522,521,520,519,518,517,516,515,514,513,512,511,510,509,508,507,506,505,
  504,503,502,501,500,499,498,497,496,495,494,492,458,431,426,424,423,422,
  38,1,0,51,52,
704,703,702,701,700,699,698,697,696,695,694,693,692,691,690,689,688,687,686,
  685,684,683,682,681,680,537,535,534,533,532,531,530,529,528,527,526,525,
  524,523,520,519,518,517,516,510,508,506,490,460,459,458,454,428,426,425,
  424,423,422,230,134,38,1,0,51,52,
459,1,0,51,52,
459,1,0,51,52,
459,1,0,51,52,
459,1,0,51,52,
459,1,0,51,52,
459,1,0,51,52,
459,1,0,51,52,
459,1,0,51,52,
459,1,0,51,52,
459,1,0,51,52,
459,1,0,51,52,
695,694,693,692,687,686,685,684,683,544,522,521,520,519,518,517,516,515,514,
  513,512,510,509,508,507,506,505,504,503,502,501,500,499,498,497,496,495,
  492,458,431,426,424,423,422,38,1,0,51,52,
703,701,700,699,698,697,696,695,694,693,692,691,690,687,686,685,684,683,682,
  537,535,534,533,532,531,530,529,528,527,526,520,517,516,490,459,428,230,
  134,1,0,51,52,
704,703,702,701,700,699,698,697,696,695,694,693,692,691,690,689,688,687,686,
  685,684,683,682,681,680,537,535,534,533,532,531,530,529,528,527,526,525,
  523,520,519,518,517,516,510,508,506,490,460,459,458,454,428,426,425,424,
  423,422,230,134,38,1,0,51,52,
703,701,700,699,698,697,696,695,694,693,692,691,690,687,686,685,684,683,682,
  537,535,534,533,532,531,530,529,528,527,526,520,517,516,490,459,428,230,
  134,1,0,51,52,
704,703,702,701,700,699,698,697,696,695,694,693,692,691,690,689,688,687,686,
  685,684,683,682,681,680,537,535,534,533,532,531,530,529,528,527,526,525,
  523,520,519,518,517,516,510,508,506,490,460,459,458,454,428,426,425,424,
  423,422,230,134,38,1,0,51,52,
703,701,700,699,698,697,696,695,694,693,692,691,690,687,686,685,684,683,682,
  537,535,534,533,532,531,530,529,528,527,526,525,524,523,520,517,516,490,
  459,428,230,134,1,0,51,52,
703,701,700,699,698,697,696,695,694,693,692,691,690,687,686,685,684,683,682,
  537,535,534,533,532,531,530,529,528,527,526,520,517,516,490,459,428,230,
  134,0,2,6,7,8,9,15,17,21,89,91,131,133,153,197,205,206,207,208,209,210,
  211,212,213,214,215,216,217,218,441,491,538,
703,701,700,699,698,697,696,695,694,693,692,691,690,687,686,685,684,683,682,
  537,535,534,533,532,531,530,529,528,527,526,520,517,516,490,459,428,230,
  134,0,2,6,7,8,9,15,17,21,89,91,131,133,153,197,205,206,207,208,209,210,
  211,212,213,214,215,216,217,218,441,491,538,
703,701,700,699,698,697,696,695,694,693,692,691,690,687,686,685,684,683,682,
  537,535,534,533,532,531,530,529,528,527,526,520,517,516,490,459,428,230,
  134,0,2,6,7,8,9,15,17,21,89,91,131,133,153,197,205,206,207,208,209,210,
  211,212,213,214,215,216,217,218,441,491,538,
703,701,700,699,698,697,696,695,694,693,692,691,690,687,686,685,684,683,682,
  537,535,534,533,532,531,530,529,528,527,526,520,517,516,490,459,428,230,
  134,0,2,6,7,8,9,15,17,21,89,91,131,133,153,197,205,206,207,208,209,210,
  211,212,213,214,215,216,217,218,441,491,538,
544,522,521,520,519,518,517,516,515,514,513,512,511,510,509,508,507,506,505,
  504,503,502,501,500,499,498,497,496,495,494,492,458,431,426,424,423,422,
  38,1,0,51,52,
544,522,521,520,519,518,517,516,515,514,513,512,511,510,509,508,507,506,505,
  504,503,502,501,500,499,498,497,496,495,494,492,458,431,424,422,38,1,0,
  51,52,196,198,199,200,201,
544,517,516,515,514,513,512,511,510,509,508,507,506,505,504,503,502,501,500,
  499,498,497,496,495,494,492,458,431,426,424,423,422,38,1,0,51,52,194,
  195,
544,515,514,513,512,511,510,509,508,507,506,505,504,503,502,501,500,499,498,
  497,496,495,494,492,458,431,426,424,423,422,38,1,0,51,52,190,191,192,
  193,
544,511,510,509,508,507,506,505,504,503,502,501,500,499,498,497,496,495,494,
  492,458,431,426,424,423,422,38,1,0,51,52,188,189,
544,509,508,507,506,505,504,503,502,501,500,499,498,497,496,495,494,492,458,
  431,426,424,423,422,38,1,0,51,52,186,187,
507,506,0,184,185,
505,504,503,502,501,500,499,498,497,496,495,494,431,426,424,423,422,38,1,0,
  59,166,167,168,169,170,171,172,173,174,175,176,177,178,179,180,181,182,
  183,
703,701,700,699,698,697,696,695,694,693,692,691,690,687,686,685,684,683,682,
  537,535,534,533,532,531,530,529,528,527,526,525,524,523,520,517,516,490,
  459,428,230,134,0,2,6,7,8,9,15,17,21,23,24,25,26,27,28,29,57,89,91,131,
  133,153,195,197,202,203,204,205,206,207,208,209,210,211,212,213,214,215,
  216,217,218,441,491,538,
426,424,423,422,38,1,0,51,52,
426,424,423,422,38,1,0,51,52,
426,424,423,422,38,1,0,75,
426,424,423,422,38,1,0,51,52,
426,424,423,422,38,1,0,51,52,
426,424,423,422,38,1,0,51,52,
426,424,423,422,38,1,0,51,52,
703,701,700,699,698,697,696,695,694,693,692,690,134,1,0,51,52,
703,701,700,699,698,697,696,695,694,693,692,690,134,1,0,51,52,
426,424,423,422,38,1,0,51,52,
426,424,423,422,38,1,0,51,52,
426,424,423,422,38,1,0,51,52,
426,424,423,422,38,1,0,51,52,
704,703,702,701,700,699,698,697,696,695,694,693,692,691,690,689,688,687,686,
  685,684,683,682,680,526,525,523,520,519,518,517,516,510,508,506,459,458,
  454,428,424,1,0,
426,424,423,422,38,1,0,51,52,
426,424,423,422,38,1,0,51,52,
426,424,423,422,38,1,0,51,52,
703,702,701,700,699,698,697,696,695,694,693,692,688,687,686,685,684,683,681,
  519,428,1,0,51,52,
703,702,701,700,699,698,697,696,695,694,693,692,688,687,686,685,684,683,681,
  519,428,0,10,12,13,14,19,434,435,436,
704,703,702,701,700,699,698,697,696,695,694,693,692,691,690,689,688,687,686,
  685,684,683,682,681,680,526,525,523,520,519,518,517,516,510,508,506,460,
  459,458,454,428,426,425,424,423,422,38,1,0,51,52,
704,703,702,701,700,699,698,697,696,695,694,693,692,691,690,689,688,687,686,
  685,684,683,682,681,680,537,535,534,533,532,531,530,529,528,527,526,525,
  524,523,520,519,518,517,516,510,508,506,490,460,459,458,454,428,426,425,
  424,423,422,230,134,38,1,0,51,52,
704,703,702,701,700,699,698,697,696,695,694,693,692,691,690,689,688,687,686,
  685,684,683,682,681,680,526,525,523,520,519,518,517,516,510,508,506,460,
  459,458,454,428,426,425,424,423,422,38,1,0,51,52,
704,703,702,701,700,699,698,697,696,695,694,693,692,691,690,689,688,687,686,
  685,684,683,682,681,680,537,535,534,533,532,531,530,529,528,527,526,525,
  524,523,520,519,518,517,516,510,508,506,490,460,459,458,454,428,426,425,
  424,423,422,230,134,38,1,0,51,52,
704,703,702,701,700,699,698,697,696,695,694,693,692,691,690,689,688,687,686,
  685,684,683,682,681,680,526,525,523,520,519,518,517,516,510,508,506,459,
  458,454,428,426,425,424,422,1,0,51,52,
704,703,702,701,700,699,698,697,696,695,694,693,692,691,690,689,688,687,686,
  685,684,683,682,681,680,526,525,523,520,519,518,517,516,510,508,506,460,
  459,458,454,428,426,425,424,423,422,38,1,0,51,52,
704,703,702,701,700,699,698,697,696,695,694,693,692,691,690,689,688,687,686,
  685,684,683,682,681,680,526,525,523,520,519,518,517,516,510,508,506,460,
  459,458,454,428,426,425,424,423,422,38,1,0,51,52,
704,703,702,701,700,699,698,697,696,695,694,693,692,691,690,689,688,687,686,
  685,684,683,682,681,680,526,525,523,520,519,518,517,516,510,508,506,460,
  459,458,454,428,426,425,424,423,422,38,1,0,51,52,
704,703,702,701,700,699,698,697,696,695,694,693,692,691,690,689,688,687,686,
  685,684,683,682,681,680,526,525,523,520,519,518,517,516,510,508,506,460,
  459,458,454,428,426,425,424,423,422,38,1,0,51,52,
704,703,702,701,700,699,698,697,696,695,694,693,692,691,690,689,688,687,686,
  685,684,683,682,681,680,526,525,523,520,519,518,517,516,510,508,506,460,
  459,458,454,428,426,425,424,423,422,38,1,0,51,52,
704,703,702,701,700,699,698,697,696,695,694,693,692,691,690,689,688,687,686,
  685,684,683,682,681,680,526,525,523,520,519,518,517,516,510,508,506,460,
  459,458,454,428,426,425,424,423,422,38,1,0,51,52,
704,703,702,701,700,699,698,697,696,695,694,693,692,691,690,689,688,687,686,
  685,684,683,682,681,680,526,525,523,520,519,518,517,516,510,508,506,460,
  459,458,454,428,426,425,424,423,422,38,1,0,51,52,
704,703,702,701,700,699,698,697,696,695,694,693,692,691,690,689,688,687,686,
  685,684,683,682,681,680,526,525,523,520,519,518,517,516,510,508,506,460,
  459,458,454,428,426,425,424,423,422,38,1,0,51,52,
704,703,702,701,700,699,698,697,696,695,694,693,692,691,690,689,688,687,686,
  685,684,683,682,681,680,526,525,523,520,519,518,517,516,510,508,506,460,
  459,458,454,428,426,425,424,423,422,38,1,0,51,52,
704,703,702,701,700,699,698,697,696,695,694,693,692,691,690,689,688,687,686,
  685,684,683,682,681,680,526,525,523,520,519,518,517,516,510,508,506,460,
  459,458,454,428,426,425,424,423,422,38,1,0,51,52,
704,703,702,701,700,699,698,697,696,695,694,693,692,691,690,689,688,687,686,
  685,684,683,682,681,680,526,525,523,520,519,518,517,516,510,508,506,460,
  459,458,454,428,426,425,424,423,422,38,1,0,51,52,
704,703,702,701,700,699,698,697,696,695,694,693,692,691,690,689,688,687,686,
  685,684,683,682,681,680,526,525,523,520,519,518,517,516,510,508,506,460,
  459,458,454,428,426,425,424,423,422,38,1,0,51,52,
704,703,702,701,700,699,698,697,696,695,694,693,692,691,690,689,688,687,686,
  685,684,683,682,681,680,526,525,523,520,519,518,517,516,510,508,506,460,
  459,458,454,428,426,425,424,423,422,38,1,0,51,52,
704,703,702,701,700,699,698,697,696,695,694,693,692,691,690,689,688,687,686,
  685,684,683,682,681,680,526,525,523,520,519,518,517,516,510,508,506,460,
  459,458,454,428,426,425,424,423,422,38,1,0,51,52,
704,703,702,701,700,699,698,697,696,695,694,693,692,691,690,689,688,687,686,
  685,684,683,682,681,680,526,525,523,520,519,518,517,516,510,508,506,460,
  459,458,454,428,426,425,424,423,422,38,1,0,51,52,
704,703,702,701,700,699,698,697,696,695,694,693,692,691,690,689,688,687,686,
  685,684,683,682,681,680,526,525,523,520,519,518,517,516,510,508,506,460,
  459,458,454,428,426,425,424,423,422,38,1,0,51,52,
704,703,702,701,700,699,698,697,696,695,694,693,692,691,690,689,688,687,686,
  685,684,683,682,681,680,526,525,523,520,519,518,517,516,510,508,506,460,
  459,458,454,428,426,425,424,423,422,38,1,0,51,52,
704,703,702,701,700,699,698,697,696,695,694,693,692,691,690,689,688,687,686,
  685,684,683,682,681,680,526,525,523,520,519,518,517,516,510,508,506,460,
  459,458,454,428,426,425,424,423,422,38,1,0,51,52,
704,703,702,701,700,699,698,697,696,695,694,693,692,691,690,689,688,687,686,
  685,684,683,682,681,680,526,525,523,520,519,518,517,516,510,508,506,460,
  459,458,454,428,426,425,424,423,422,38,1,0,51,52,
704,703,702,701,700,699,698,697,696,695,694,693,692,691,690,689,688,687,686,
  685,684,683,682,681,680,537,535,534,533,532,531,530,529,528,527,526,525,
  523,520,519,518,517,516,510,508,506,490,460,459,458,454,428,426,425,424,
  423,422,230,134,38,1,0,51,52,
704,703,702,701,700,699,698,697,696,695,694,693,692,691,690,689,688,687,686,
  685,684,683,682,681,680,537,535,534,533,532,531,530,529,528,527,526,525,
  524,523,520,519,518,517,516,510,508,506,490,460,459,458,454,428,426,425,
  424,423,422,230,134,38,1,0,51,52,
704,703,702,701,700,699,698,697,696,695,694,693,692,691,690,689,688,687,686,
  685,684,683,682,681,680,537,535,534,533,532,531,530,529,528,527,526,525,
  524,523,520,519,518,517,516,510,508,506,490,460,459,458,454,428,426,425,
  424,423,422,230,134,38,1,0,51,52,
704,703,702,701,700,699,698,697,696,695,694,693,692,691,690,689,688,687,686,
  685,684,683,682,681,680,537,535,534,533,532,531,530,529,528,527,526,525,
  524,523,520,519,518,517,516,510,508,506,490,460,459,458,454,428,426,425,
  424,423,422,230,134,38,1,0,51,52,
704,703,702,701,700,699,698,697,696,695,694,693,692,691,690,689,688,687,686,
  685,684,683,682,681,680,537,535,534,533,532,531,530,529,528,527,526,525,
  523,520,519,518,517,516,510,508,506,490,460,459,458,454,428,426,425,424,
  423,422,230,134,38,1,0,51,52,
704,703,702,701,700,699,698,697,696,695,694,693,692,691,690,689,688,687,686,
  685,684,683,682,681,680,544,526,525,523,522,521,520,519,518,517,516,515,
  514,513,512,511,510,509,508,507,506,505,504,503,502,501,500,499,498,497,
  496,495,494,492,460,459,458,454,431,428,426,425,424,423,422,38,1,0,51,
  52,
704,703,702,701,700,699,698,697,696,695,694,693,692,691,690,689,688,687,686,
  685,684,683,682,681,680,526,525,523,520,519,518,517,516,510,508,506,460,
  459,458,454,428,426,425,424,423,422,38,1,0,51,52,
704,703,702,701,700,699,698,697,696,695,694,693,692,691,690,689,688,687,686,
  685,684,683,682,681,680,537,535,534,533,532,531,530,529,528,527,526,525,
  524,523,520,519,518,517,516,510,508,506,490,460,459,458,454,428,426,425,
  424,423,422,230,134,38,1,0,51,52,
704,703,702,701,700,699,698,697,696,695,694,693,692,691,690,689,688,687,686,
  685,684,683,682,681,680,537,535,534,533,532,531,530,529,528,527,526,525,
  523,520,519,518,517,516,510,508,506,490,460,459,458,454,428,426,425,424,
  423,422,230,134,38,1,0,51,52,
704,703,702,701,700,699,698,697,696,695,694,693,692,691,690,689,688,687,686,
  685,684,683,682,681,680,526,525,523,520,519,518,517,516,510,508,506,460,
  459,458,454,428,426,425,424,423,422,38,1,0,51,52,
704,703,702,701,700,699,698,697,696,695,694,693,692,691,690,689,688,687,686,
  685,684,683,682,681,680,526,525,523,520,519,518,517,516,510,508,506,460,
  459,458,454,428,426,425,424,423,422,38,1,0,51,52,
704,703,702,701,700,699,698,697,696,695,694,693,692,691,690,689,688,687,686,
  685,684,683,682,681,680,526,525,523,520,519,518,517,516,510,508,506,460,
  459,458,454,428,426,425,424,423,422,38,1,0,51,52,
704,703,702,701,700,699,698,697,696,695,694,693,692,691,690,689,688,687,686,
  685,684,683,682,681,680,526,525,523,520,519,518,517,516,510,508,506,459,
  458,454,437,433,432,428,426,425,424,423,422,163,162,161,134,38,1,0,51,
  52,
704,703,702,701,700,699,698,697,696,695,694,693,692,691,690,689,688,687,686,
  685,684,683,682,681,680,526,525,523,520,519,518,517,516,510,508,506,459,
  458,454,428,424,1,0,46,54,86,91,93,139,140,142,144,145,146,147,148,149,
  153,156,157,184,186,188,194,195,196,198,199,202,204,219,223,224,228,412,
  413,414,415,416,417,418,419,420,421,
422,0,45,
422,0,45,
703,701,700,699,698,697,696,695,694,693,692,691,690,687,686,685,684,683,682,
  537,535,534,533,532,531,530,529,528,527,526,525,524,523,520,517,516,490,
  459,428,230,134,1,0,51,52,
703,701,700,699,698,697,696,695,694,693,692,691,690,687,686,685,684,683,682,
  537,535,534,533,532,531,530,529,528,527,526,525,524,523,520,517,516,490,
  459,428,230,134,0,2,6,7,8,9,15,17,21,23,24,25,26,27,28,29,57,89,91,131,
  133,153,195,197,202,203,204,205,206,207,208,209,210,211,212,213,214,215,
  216,217,218,441,491,538,
490,489,488,487,486,485,483,482,481,480,479,478,477,476,475,474,473,472,471,
  470,469,468,467,466,465,464,463,462,461,455,453,452,444,443,442,440,438,
  430,429,0,56,58,69,70,71,72,73,83,84,85,98,99,100,101,102,103,104,105,
  106,107,108,109,110,111,112,113,114,115,117,118,119,120,121,122,123,124,
  125,126,127,128,129,130,131,
426,424,423,422,38,1,0,51,52,
493,490,426,424,423,422,38,0,22,39,40,41,45,46,47,49,131,164,
668,543,542,541,247,245,237,235,233,0,244,246,248,249,253,254,255,
540,240,239,238,237,236,235,234,233,0,4,241,594,
668,543,247,245,237,235,233,0,5,244,246,248,608,
686,685,684,683,0,344,
454,0,86,
454,0,86,
454,0,86,
703,701,700,699,698,697,696,695,694,693,692,691,690,687,686,685,684,683,682,
  537,535,534,533,532,531,530,529,528,527,526,525,524,523,520,517,516,490,
  459,428,230,134,0,2,6,7,8,9,15,17,21,23,24,25,26,27,28,29,57,89,91,131,
  133,153,195,197,202,203,204,205,206,207,208,209,210,211,212,213,214,215,
  216,217,218,441,491,538,
703,701,700,699,698,697,696,695,694,693,692,691,690,687,686,685,684,683,682,
  537,535,534,533,532,531,530,529,528,527,526,525,524,523,520,517,516,490,
  459,428,230,134,0,2,6,7,8,9,15,17,21,23,24,25,26,27,28,29,57,89,91,131,
  133,153,195,197,202,203,204,205,206,207,208,209,210,211,212,213,214,215,
  216,217,218,441,491,538,
703,701,700,699,698,697,696,695,694,693,692,691,690,687,686,685,684,683,682,
  537,535,534,533,532,531,530,529,528,527,526,525,524,523,520,517,516,490,
  459,428,230,134,0,2,6,7,8,9,15,17,21,23,24,25,26,27,28,29,57,89,91,131,
  133,153,195,197,202,203,204,205,206,207,208,209,210,211,212,213,214,215,
  216,217,218,441,491,538,
703,701,700,699,698,697,696,695,694,693,692,691,690,687,686,685,684,683,682,
  537,535,534,533,532,531,530,529,528,527,526,525,524,523,520,517,516,490,
  459,428,230,134,0,2,6,7,8,9,15,17,21,23,24,25,26,27,28,29,57,89,91,131,
  133,153,195,197,202,203,204,205,206,207,208,209,210,211,212,213,214,215,
  216,217,218,441,491,538,
703,701,700,699,698,697,696,695,694,693,692,691,690,687,686,685,684,683,682,
  537,535,534,533,532,531,530,529,528,527,526,525,524,523,520,517,516,490,
  459,428,230,134,0,2,6,7,8,9,15,17,21,23,24,25,26,27,28,29,57,89,91,131,
  133,153,195,197,202,203,204,205,206,207,208,209,210,211,212,213,214,215,
  216,217,218,441,491,538,
703,701,700,699,698,697,696,695,694,693,692,691,690,687,686,685,684,683,682,
  537,535,534,533,532,531,530,529,528,527,526,525,524,523,520,517,516,490,
  459,428,230,134,0,2,6,7,8,9,15,17,21,23,24,25,26,27,28,29,57,89,91,131,
  133,153,195,197,202,203,204,205,206,207,208,209,210,211,212,213,214,215,
  216,217,218,441,491,538,
703,701,700,699,698,697,696,695,694,693,692,691,690,687,686,685,684,683,682,
  537,535,534,533,532,531,530,529,528,527,526,525,524,523,520,517,516,490,
  459,428,230,134,0,2,6,7,8,9,15,17,21,23,24,25,26,27,28,29,57,89,91,131,
  133,153,195,197,202,203,204,205,206,207,208,209,210,211,212,213,214,215,
  216,217,218,441,491,538,
703,701,700,699,698,697,696,695,694,693,692,691,690,687,686,685,684,683,682,
  537,535,534,533,532,531,530,529,528,527,526,525,524,523,520,517,516,490,
  459,428,230,134,0,2,6,7,8,9,15,17,21,23,24,25,26,27,28,29,57,89,91,131,
  133,153,195,197,202,203,204,205,206,207,208,209,210,211,212,213,214,215,
  216,217,218,441,491,538,
703,701,700,699,698,697,696,695,694,693,692,691,690,687,686,685,684,683,682,
  537,535,534,533,532,531,530,529,528,527,526,525,524,523,520,517,516,490,
  459,428,230,134,0,2,6,7,8,9,15,17,21,23,24,25,26,27,28,29,57,89,91,131,
  133,153,195,197,202,203,204,205,206,207,208,209,210,211,212,213,214,215,
  216,217,218,441,491,538,
703,701,700,699,698,697,696,695,694,693,692,691,690,687,686,685,684,683,682,
  537,535,534,533,532,531,530,529,528,527,526,525,524,523,520,517,516,490,
  459,428,230,134,0,2,6,7,8,9,15,17,21,23,24,25,26,27,28,29,57,89,91,131,
  133,153,195,197,202,203,204,205,206,207,208,209,210,211,212,213,214,215,
  216,217,218,441,491,538,
703,701,700,699,698,697,696,695,694,693,692,691,690,687,686,685,684,683,682,
  537,535,534,533,532,531,530,529,528,527,526,525,524,523,520,517,516,490,
  459,428,230,134,0,2,6,7,8,9,15,17,21,23,24,25,26,27,28,29,57,89,91,131,
  133,153,195,197,202,203,204,205,206,207,208,209,210,211,212,213,214,215,
  216,217,218,441,491,538,
703,701,700,699,698,697,696,695,694,693,692,691,690,687,686,685,684,683,682,
  537,535,534,533,532,531,530,529,528,527,526,525,524,523,520,517,516,490,
  459,428,230,134,0,2,6,7,8,9,15,17,21,23,24,25,26,27,28,29,57,89,91,131,
  133,153,195,197,202,203,204,205,206,207,208,209,210,211,212,213,214,215,
  216,217,218,441,491,538,
703,701,700,699,698,697,696,695,694,693,692,691,690,687,686,685,684,683,682,
  537,535,534,533,532,531,530,529,528,527,526,525,524,523,520,517,516,490,
  459,428,230,134,0,2,6,7,8,9,15,17,21,23,24,25,26,27,28,29,57,89,91,131,
  133,153,195,197,202,203,204,205,206,207,208,209,210,211,212,213,214,215,
  216,217,218,441,491,538,
703,701,700,699,698,697,696,695,694,693,692,691,690,687,686,685,684,683,682,
  537,535,534,533,532,531,530,529,528,527,526,525,524,523,520,517,516,490,
  459,428,230,134,0,2,6,7,8,9,15,17,21,23,24,25,26,27,28,29,57,89,91,131,
  133,153,195,197,202,203,204,205,206,207,208,209,210,211,212,213,214,215,
  216,217,218,441,491,538,
703,701,700,699,698,697,696,695,694,693,692,691,690,687,686,685,684,683,682,
  537,535,534,533,532,531,530,529,528,527,526,525,524,523,520,517,516,490,
  459,428,230,134,0,2,6,7,8,9,15,17,21,23,24,25,26,27,28,29,57,89,91,131,
  133,153,195,197,202,203,204,205,206,207,208,209,210,211,212,213,214,215,
  216,217,218,441,491,538,
703,701,700,699,698,697,696,695,694,693,692,691,690,687,686,685,684,683,682,
  537,535,534,533,532,531,530,529,528,527,526,525,524,523,520,517,516,490,
  459,428,230,134,0,2,6,7,8,9,15,17,21,23,24,25,26,27,28,29,57,89,91,131,
  133,153,195,197,202,203,204,205,206,207,208,209,210,211,212,213,214,215,
  216,217,218,441,491,538,
703,701,700,699,698,697,696,695,694,693,692,691,690,687,686,685,684,683,682,
  537,535,534,533,532,531,530,529,528,527,526,525,524,523,520,517,516,490,
  459,428,230,134,0,2,6,7,8,9,15,17,21,23,24,25,26,27,28,29,57,89,91,131,
  133,153,195,197,202,203,204,205,206,207,208,209,210,211,212,213,214,215,
  216,217,218,441,491,538,
703,701,700,699,698,697,696,695,694,693,692,691,690,687,686,685,684,683,682,
  537,535,534,533,532,531,530,529,528,527,526,525,524,523,520,517,516,490,
  459,428,230,134,0,2,6,7,8,9,15,17,21,23,24,25,26,27,28,29,57,89,91,131,
  133,153,195,197,202,203,204,205,206,207,208,209,210,211,212,213,214,215,
  216,217,218,441,491,538,
703,701,700,699,698,697,696,695,694,693,692,691,690,687,686,685,684,683,682,
  537,535,534,533,532,531,530,529,528,527,526,525,524,523,520,517,516,490,
  459,428,230,134,0,2,6,7,8,9,15,17,21,23,24,25,26,27,28,29,57,89,91,131,
  133,153,195,197,202,203,204,205,206,207,208,209,210,211,212,213,214,215,
  216,217,218,441,491,538,
703,701,700,699,698,697,696,695,694,693,692,691,690,687,686,685,684,683,682,
  537,535,534,533,532,531,530,529,528,527,526,525,524,523,520,517,516,490,
  459,428,230,134,0,2,6,7,8,9,15,17,21,23,24,25,26,27,28,29,57,89,91,131,
  133,153,195,197,202,203,204,205,206,207,208,209,210,211,212,213,214,215,
  216,217,218,441,491,538,
703,701,700,699,698,697,696,695,694,693,692,691,690,687,686,685,684,683,682,
  537,535,534,533,532,531,530,529,528,527,526,525,524,523,520,517,516,490,
  459,428,230,134,0,2,6,7,8,9,15,17,21,23,24,25,26,27,28,29,57,89,91,131,
  133,153,195,197,202,203,204,205,206,207,208,209,210,211,212,213,214,215,
  216,217,218,441,491,538,
703,701,700,699,698,697,696,695,694,693,692,691,690,687,686,685,684,683,682,
  537,535,534,533,532,531,530,529,528,527,526,525,524,523,520,517,516,490,
  459,428,230,134,0,2,6,7,8,9,15,17,21,23,24,25,26,27,28,29,57,89,91,131,
  133,153,195,197,202,203,204,205,206,207,208,209,210,211,212,213,214,215,
  216,217,218,441,491,538,
703,701,700,699,698,697,696,695,694,693,692,691,690,687,686,685,684,683,682,
  537,535,534,533,532,531,530,529,528,527,526,525,524,523,520,517,516,490,
  459,428,230,134,0,2,6,7,8,9,15,17,21,23,24,25,26,27,28,29,57,89,91,131,
  133,153,195,197,202,203,204,205,206,207,208,209,210,211,212,213,214,215,
  216,217,218,441,491,538,
703,701,700,699,698,697,696,695,694,693,692,691,690,687,686,685,684,683,682,
  537,535,534,533,532,531,530,529,528,527,526,525,524,523,520,517,516,490,
  459,428,230,134,0,2,6,7,8,9,15,17,21,23,24,25,26,27,28,29,57,89,91,131,
  133,153,195,197,202,203,204,205,206,207,208,209,210,211,212,213,214,215,
  216,217,218,441,491,538,
703,701,700,699,698,697,696,695,694,693,692,691,690,687,686,685,684,683,682,
  537,535,534,533,532,531,530,529,528,527,526,525,524,523,520,517,516,490,
  459,428,230,134,0,2,6,7,8,9,15,17,21,23,24,25,26,27,28,29,57,89,91,131,
  133,153,195,197,202,203,204,205,206,207,208,209,210,211,212,213,214,215,
  216,217,218,441,491,538,
703,701,700,699,698,697,696,695,694,693,692,691,690,687,686,685,684,683,682,
  537,535,534,533,532,531,530,529,528,527,526,525,524,523,520,517,516,490,
  459,428,230,134,0,2,6,7,8,9,15,17,21,23,24,25,26,27,28,29,57,89,91,131,
  133,153,195,197,202,203,204,205,206,207,208,209,210,211,212,213,214,215,
  216,217,218,441,491,538,
703,701,700,699,698,697,696,695,694,693,692,691,690,687,686,685,684,683,682,
  537,535,534,533,532,531,530,529,528,527,526,525,524,523,520,517,516,490,
  459,428,230,134,0,2,6,7,8,9,15,17,21,23,24,25,26,27,28,29,57,89,91,131,
  133,153,195,197,202,203,204,205,206,207,208,209,210,211,212,213,214,215,
  216,217,218,441,491,538,
703,701,700,699,698,697,696,695,694,693,692,691,690,687,686,685,684,683,682,
  537,535,534,533,532,531,530,529,528,527,526,525,524,523,520,517,516,490,
  459,428,230,134,0,2,6,7,8,9,15,17,21,23,24,25,26,27,28,29,57,89,91,131,
  133,153,195,197,202,203,204,205,206,207,208,209,210,211,212,213,214,215,
  216,217,218,441,491,538,
703,701,700,699,698,697,696,695,694,693,692,691,690,687,686,685,684,683,682,
  537,535,534,533,532,531,530,529,528,527,526,525,524,523,520,517,516,490,
  459,428,230,134,0,2,6,7,8,9,15,17,21,23,24,25,26,27,28,29,57,89,91,131,
  133,153,195,197,202,203,204,205,206,207,208,209,210,211,212,213,214,215,
  216,217,218,441,491,538,
703,701,700,699,698,697,696,695,694,693,692,691,690,687,686,685,684,683,682,
  537,535,534,533,532,531,530,529,528,527,526,525,524,523,520,517,516,490,
  459,428,230,134,0,2,6,7,8,9,15,17,21,23,24,25,26,27,28,29,57,89,91,131,
  133,153,195,197,202,203,204,205,206,207,208,209,210,211,212,213,214,215,
  216,217,218,441,491,538,
703,701,700,699,698,697,696,695,694,693,692,691,690,687,686,685,684,683,682,
  537,535,534,533,532,531,530,529,528,527,526,525,524,523,520,517,516,490,
  459,428,230,134,0,2,6,7,8,9,15,17,21,23,24,25,26,27,28,29,57,89,91,131,
  133,153,195,197,202,203,204,205,206,207,208,209,210,211,212,213,214,215,
  216,217,218,441,491,538,
703,701,700,699,698,697,696,695,694,693,692,691,690,687,686,685,684,683,682,
  537,535,534,533,532,531,530,529,528,527,526,525,524,523,520,517,516,490,
  459,428,230,134,0,2,6,7,8,9,15,17,21,23,24,25,26,27,28,29,57,89,91,131,
  133,153,195,197,202,203,204,205,206,207,208,209,210,211,212,213,214,215,
  216,217,218,441,491,538,
703,701,700,699,698,697,696,695,694,693,692,691,690,687,686,685,684,683,682,
  537,535,534,533,532,531,530,529,528,527,526,525,524,523,520,517,516,490,
  459,428,230,134,0,2,6,7,8,9,15,17,21,23,24,25,26,27,28,29,57,89,91,131,
  133,153,195,197,202,203,204,205,206,207,208,209,210,211,212,213,214,215,
  216,217,218,441,491,538,
703,701,700,699,698,697,696,695,694,693,692,691,690,687,686,685,684,683,682,
  537,535,534,533,532,531,530,529,528,527,526,525,524,523,520,517,516,490,
  459,428,230,134,0,2,6,7,8,9,15,17,21,23,24,25,26,27,28,29,57,89,91,131,
  133,153,195,197,202,203,204,205,206,207,208,209,210,211,212,213,214,215,
  216,217,218,441,491,538,
703,701,700,699,698,697,696,695,694,693,692,691,690,687,686,685,684,683,682,
  537,535,534,533,532,531,530,529,528,527,526,525,524,523,520,517,516,490,
  459,428,230,134,0,2,6,7,8,9,15,17,21,23,24,25,26,27,28,29,57,89,91,131,
  133,153,195,197,202,203,204,205,206,207,208,209,210,211,212,213,214,215,
  216,217,218,441,491,538,
703,701,700,699,698,697,696,695,694,693,692,691,690,687,686,685,684,683,682,
  537,535,534,533,532,531,530,529,528,527,526,525,524,523,520,517,516,490,
  459,428,230,134,0,2,6,7,8,9,15,17,21,23,24,25,26,27,28,29,57,89,91,131,
  133,153,195,197,202,203,204,205,206,207,208,209,210,211,212,213,214,215,
  216,217,218,441,491,538,
703,701,700,699,698,697,696,695,694,693,692,691,690,687,686,685,684,683,682,
  537,535,534,533,532,531,530,529,528,527,526,525,524,523,520,517,516,490,
  459,428,230,134,0,2,6,7,8,9,15,17,21,23,24,25,26,27,28,29,57,89,91,131,
  133,153,195,197,202,203,204,205,206,207,208,209,210,211,212,213,214,215,
  216,217,218,441,491,538,
703,701,700,699,698,697,696,695,694,693,692,691,690,687,686,685,684,683,682,
  537,535,534,533,532,531,530,529,528,527,526,525,524,523,520,517,516,490,
  459,428,230,134,0,2,6,7,8,9,15,17,21,23,24,25,26,27,28,29,57,89,91,131,
  133,153,195,197,202,203,204,205,206,207,208,209,210,211,212,213,214,215,
  216,217,218,441,491,538,
703,701,700,699,698,697,696,695,694,693,692,691,690,687,686,685,684,683,682,
  537,535,534,533,532,531,530,529,528,527,526,525,524,523,520,517,516,490,
  459,428,230,134,0,2,6,7,8,9,15,17,21,23,24,25,26,27,28,29,57,89,91,131,
  133,153,195,197,202,203,204,205,206,207,208,209,210,211,212,213,214,215,
  216,217,218,441,491,538,
703,701,700,699,698,697,696,695,694,693,692,691,690,687,686,685,684,683,682,
  537,535,534,533,532,531,530,529,528,527,526,525,524,523,520,517,516,490,
  459,428,230,134,0,2,6,7,8,9,15,17,21,23,24,25,26,27,28,29,57,89,91,131,
  133,153,195,197,202,203,204,205,206,207,208,209,210,211,212,213,214,215,
  216,217,218,441,491,538,
703,701,700,699,698,697,696,695,694,693,692,691,690,687,686,685,684,683,682,
  537,535,534,533,532,531,530,529,528,527,526,525,524,523,520,517,516,490,
  459,428,230,134,0,2,6,7,8,9,15,17,21,23,24,25,26,27,28,29,57,89,91,131,
  133,153,195,197,202,203,204,205,206,207,208,209,210,211,212,213,214,215,
  216,217,218,441,491,538,
703,701,700,699,698,697,696,695,694,693,692,691,690,687,686,685,684,683,682,
  537,535,534,533,532,531,530,529,528,527,526,525,524,523,520,517,516,490,
  459,428,230,134,0,2,6,7,8,9,15,17,21,23,24,25,26,27,28,29,57,89,91,131,
  133,153,195,197,202,203,204,205,206,207,208,209,210,211,212,213,214,215,
  216,217,218,441,491,538,
703,701,700,699,698,697,696,695,694,693,692,691,690,687,686,685,684,683,682,
  537,535,534,533,532,531,530,529,528,527,526,525,524,523,520,517,516,490,
  459,428,230,134,0,2,6,7,8,9,15,17,21,23,24,25,26,27,28,29,57,89,91,131,
  133,153,195,197,202,203,204,205,206,207,208,209,210,211,212,213,214,215,
  216,217,218,441,491,538,
703,701,700,699,698,697,696,695,694,693,692,691,690,687,686,685,684,683,682,
  537,535,534,533,532,531,530,529,528,527,526,525,524,523,520,517,516,490,
  459,428,230,134,0,2,6,7,8,9,15,17,21,23,24,25,26,27,28,29,57,89,91,131,
  133,153,195,197,202,203,204,205,206,207,208,209,210,211,212,213,214,215,
  216,217,218,441,491,538,
703,701,700,699,698,697,696,695,694,693,692,691,690,687,686,685,684,683,682,
  537,535,534,533,532,531,530,529,528,527,526,525,524,523,520,517,516,490,
  459,428,230,134,0,2,6,7,8,9,15,17,21,23,24,25,26,27,28,29,57,89,91,131,
  133,153,195,197,202,203,204,205,206,207,208,209,210,211,212,213,214,215,
  216,217,218,441,491,538,
703,701,700,699,698,697,696,695,694,693,692,691,690,687,686,685,684,683,682,
  537,535,534,533,532,531,530,529,528,527,526,525,524,523,520,517,516,490,
  459,428,230,134,0,2,6,7,8,9,15,17,21,23,24,25,26,27,28,29,57,89,91,131,
  133,153,195,197,202,203,204,205,206,207,208,209,210,211,212,213,214,215,
  216,217,218,441,491,538,
703,701,700,699,698,697,696,695,694,693,692,691,690,687,686,685,684,683,682,
  537,535,534,533,532,531,530,529,528,527,526,525,524,523,520,517,516,490,
  459,428,230,134,0,2,6,7,8,9,15,17,21,23,24,25,26,27,28,29,57,89,91,131,
  133,153,195,197,202,203,204,205,206,207,208,209,210,211,212,213,214,215,
  216,217,218,441,491,538,
703,701,700,699,698,697,696,695,694,693,692,691,690,687,686,685,684,683,682,
  537,535,534,533,532,531,530,529,528,527,526,525,524,523,520,517,516,490,
  459,428,230,134,0,2,6,7,8,9,15,17,21,23,24,25,26,27,28,29,57,89,91,131,
  133,153,195,197,202,203,204,205,206,207,208,209,210,211,212,213,214,215,
  216,217,218,441,491,538,
703,701,700,699,698,697,696,695,694,693,692,691,690,687,686,685,684,683,682,
  537,535,534,533,532,531,530,529,528,527,526,525,524,523,520,517,516,490,
  459,428,230,134,0,2,6,7,8,9,15,17,21,23,24,25,26,27,28,29,57,89,91,131,
  133,153,195,197,202,203,204,205,206,207,208,209,210,211,212,213,214,215,
  216,217,218,441,491,538,
703,701,700,699,698,697,696,695,694,693,692,691,690,687,686,685,684,683,682,
  537,535,534,533,532,531,530,529,528,527,526,525,524,523,520,517,516,490,
  459,428,230,134,0,2,6,7,8,9,15,17,21,23,24,25,26,27,28,29,57,89,91,131,
  133,153,195,197,202,203,204,205,206,207,208,209,210,211,212,213,214,215,
  216,217,218,441,491,538,
703,701,700,699,698,697,696,695,694,693,692,691,690,687,686,685,684,683,682,
  537,535,534,533,532,531,530,529,528,527,526,525,524,523,520,517,516,490,
  459,428,230,134,0,2,6,7,8,9,15,17,21,23,24,25,26,27,28,29,57,89,91,131,
  133,153,195,197,202,203,204,205,206,207,208,209,210,211,212,213,214,215,
  216,217,218,441,491,538,
703,701,700,699,698,697,696,695,694,693,692,691,690,687,686,685,684,683,682,
  537,535,534,533,532,531,530,529,528,527,526,525,524,523,520,517,516,490,
  459,428,230,134,0,2,6,7,8,9,15,17,21,23,24,25,26,27,28,29,57,89,91,131,
  133,153,195,197,202,203,204,205,206,207,208,209,210,211,212,213,214,215,
  216,217,218,441,491,538,
668,540,539,247,245,237,235,233,0,242,244,246,248,250,251,
668,540,539,247,245,237,235,233,0,242,244,246,248,250,251,
668,540,539,247,245,237,235,233,0,242,244,246,248,250,251,
668,540,539,247,245,237,235,233,0,242,244,246,248,250,251,
668,245,235,233,0,244,246,252,
668,245,235,233,0,244,246,252,
668,543,247,245,237,235,233,0,5,244,246,248,608,
668,543,247,245,237,235,233,0,5,244,246,248,608,
668,543,247,245,237,235,233,0,5,244,246,248,608,
668,543,542,541,247,245,237,235,233,0,244,246,248,249,253,254,255,
668,543,542,541,247,245,237,235,233,0,244,246,248,249,253,254,255,
540,240,239,238,237,236,235,234,233,0,4,241,594,
540,240,239,238,237,236,235,234,233,0,4,241,594,
540,240,239,238,237,236,235,234,233,0,4,241,594,
540,240,239,238,237,236,235,234,233,0,4,241,594,
540,240,239,238,237,236,235,234,233,0,4,241,594,
540,240,239,238,237,236,235,234,233,0,4,241,594,
540,240,239,238,237,236,235,234,233,0,4,241,594,
540,240,239,238,237,236,235,234,233,0,4,241,594,
540,240,239,238,237,236,235,234,233,0,4,241,594,
540,240,239,238,237,236,235,234,233,0,4,241,594,
540,240,239,238,237,236,235,234,233,0,4,241,594,
703,701,700,699,698,697,696,695,694,693,692,691,690,687,686,685,684,683,682,
  681,537,535,534,533,532,531,530,529,528,527,526,525,524,523,520,517,516,
  490,459,428,230,134,0,2,6,7,8,9,10,12,15,17,18,20,21,23,24,25,26,27,28,
  29,57,89,91,92,131,133,153,195,197,202,203,204,205,206,207,208,209,210,
  211,212,213,214,215,216,217,218,434,441,484,491,538,
691,687,686,685,684,683,682,526,520,517,516,230,1,0,2,6,7,15,17,217,218,491,
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
703,701,700,699,698,697,696,695,694,693,692,690,134,1,0,51,52,
703,701,700,699,698,697,696,695,694,693,692,690,134,0,21,133,441,
703,702,701,700,699,698,697,696,695,694,693,692,687,686,685,684,683,519,428,
  1,0,51,52,
703,702,701,700,699,698,697,696,695,694,693,692,687,686,685,684,683,519,428,
  0,19,436,
426,424,423,422,38,1,0,51,52,
426,424,423,422,38,1,0,51,52,
682,681,1,0,51,52,
682,681,0,10,12,18,20,434,484,
426,424,423,422,38,1,0,51,52,
426,424,423,422,38,1,0,51,52,
703,701,700,699,698,697,696,695,694,693,692,690,426,424,423,422,134,38,1,0,
  51,52,
703,701,700,699,698,697,696,695,694,693,692,690,426,424,423,422,134,38,1,0,
  21,133,441,
681,0,10,12,434,
681,0,10,12,434,
703,701,700,699,698,697,696,695,694,693,692,691,690,687,686,685,684,683,682,
  681,537,535,534,533,532,531,530,529,528,527,526,525,524,523,520,517,516,
  490,459,428,230,134,1,0,51,52,
703,701,700,699,698,697,696,695,694,693,692,691,690,687,686,685,684,683,682,
  681,537,535,534,533,532,531,530,529,528,527,526,525,524,523,520,517,516,
  490,459,428,230,134,1,0,51,52,
703,701,700,699,698,697,696,695,694,693,692,691,690,687,686,685,684,683,682,
  681,537,535,534,533,532,531,530,529,528,527,526,525,524,523,520,517,516,
  490,459,428,230,134,1,0,2,6,7,8,9,10,12,15,17,21,23,24,25,26,27,28,29,
  51,52,57,89,91,131,133,153,195,197,202,203,204,205,206,207,208,209,210,
  211,212,213,214,215,216,217,218,434,441,491,538,
703,701,700,699,698,697,696,695,694,693,692,691,690,687,686,685,684,683,682,
  537,535,534,533,532,531,530,529,528,527,526,525,524,523,520,517,516,490,
  459,428,230,134,1,0,51,52,
703,701,700,699,698,697,696,695,694,693,692,691,690,687,686,685,684,683,682,
  537,535,534,533,532,531,530,529,528,527,526,525,524,523,520,517,516,490,
  459,428,230,134,0,2,6,7,8,9,15,17,21,23,24,25,26,27,28,29,57,89,91,131,
  133,153,195,197,202,203,204,205,206,207,208,209,210,211,212,213,214,215,
  216,217,218,441,491,538,
703,701,700,699,698,697,696,695,694,693,692,690,134,1,0,51,52,
703,701,700,699,698,697,696,695,694,693,692,690,134,0,21,133,441,
703,701,700,699,698,697,696,695,694,693,692,690,134,1,0,51,52,
703,701,700,699,698,697,696,695,694,693,692,690,134,0,21,133,441,
703,701,700,699,698,697,696,695,694,693,692,690,134,1,0,51,52,
703,701,700,699,698,697,696,695,694,693,692,690,134,0,21,116,133,441,
703,701,700,699,698,697,696,695,694,693,692,690,134,1,0,51,52,
703,701,700,699,698,697,696,695,694,693,692,690,134,0,21,116,133,441,
703,701,700,699,698,697,696,695,694,693,692,690,134,1,0,51,52,
703,701,700,699,698,697,696,695,694,693,692,690,134,0,21,116,133,441,
703,701,700,699,698,697,696,695,694,693,692,691,690,687,686,685,684,683,682,
  681,537,535,534,533,532,531,530,529,528,527,526,525,524,523,520,517,516,
  490,459,428,230,134,1,0,51,52,
703,701,700,699,698,697,696,695,694,693,692,691,690,687,686,685,684,683,682,
  681,537,535,534,533,532,531,530,529,528,527,526,525,524,523,520,517,516,
  490,459,428,230,134,1,0,51,52,
703,701,700,699,698,697,696,695,694,693,692,691,690,687,686,685,684,683,682,
  681,537,535,534,533,532,531,530,529,528,527,526,525,524,523,520,517,516,
  490,459,428,230,134,0,2,6,7,8,9,10,12,15,17,18,20,21,23,24,25,26,27,28,
  29,57,89,91,92,131,133,153,195,197,202,203,204,205,206,207,208,209,210,
  211,212,213,214,215,216,217,218,434,441,484,491,538,
703,701,700,699,698,697,696,695,694,693,692,691,690,687,686,685,684,683,682,
  681,537,535,534,533,532,531,530,529,528,527,526,525,524,523,520,517,516,
  490,459,428,230,134,1,0,51,52,
703,701,700,699,698,697,696,695,694,693,692,691,690,687,686,685,684,683,682,
  681,537,535,534,533,532,531,530,529,528,527,526,525,524,523,520,517,516,
  490,459,428,230,134,1,0,51,52,
703,701,700,699,698,697,696,695,694,693,692,691,690,687,686,685,684,683,682,
  681,537,535,534,533,532,531,530,529,528,527,526,525,524,523,520,517,516,
  490,459,428,230,134,1,0,51,52,
703,701,700,699,698,697,696,695,694,693,692,691,690,687,686,685,684,683,682,
  681,537,535,534,533,532,531,530,529,528,527,526,525,524,523,520,517,516,
  490,459,428,230,134,0,2,6,7,8,9,10,12,15,17,18,20,21,23,24,25,26,27,28,
  29,57,89,91,92,131,133,153,195,197,202,203,204,205,206,207,208,209,210,
  211,212,213,214,215,216,217,218,434,441,484,491,538,
703,701,700,699,698,697,696,695,694,693,692,691,690,687,686,685,684,683,682,
  537,535,534,533,532,531,530,529,528,527,526,525,524,523,520,517,516,490,
  459,428,230,134,1,0,51,52,
703,701,700,699,698,697,696,695,694,693,692,691,690,687,686,685,684,683,682,
  537,535,534,533,532,531,530,529,528,527,526,525,524,523,520,517,516,490,
  459,428,230,134,1,0,51,52,
703,701,700,699,698,697,696,695,694,693,692,691,690,687,686,685,684,683,682,
  537,535,534,533,532,531,530,529,528,527,526,525,524,523,520,517,516,490,
  459,428,230,134,0,2,6,7,8,9,15,17,21,23,24,25,26,27,28,29,57,89,91,131,
  133,153,195,197,202,203,204,205,206,207,208,209,210,211,212,213,214,215,
  216,217,218,441,491,538,
426,424,423,422,38,1,0,51,52,
703,701,700,699,698,697,696,695,694,693,692,691,690,687,686,685,684,683,682,
  537,535,534,533,532,531,530,529,528,527,526,525,524,523,520,517,516,490,
  459,428,230,134,1,0,51,52,
703,701,700,699,698,697,696,695,694,693,692,691,690,687,686,685,684,683,682,
  537,535,534,533,532,531,530,529,528,527,526,525,524,523,520,517,516,490,
  459,428,230,134,1,0,51,52,
703,701,700,699,698,697,696,695,694,693,692,691,690,687,686,685,684,683,682,
  537,535,534,533,532,531,530,529,528,527,526,525,524,523,520,517,516,490,
  459,428,230,134,1,0,51,52,
703,701,700,699,698,697,696,695,694,693,692,691,690,687,686,685,684,683,682,
  537,535,534,533,532,531,530,529,528,527,526,525,524,523,520,517,516,490,
  459,428,230,134,0,2,6,7,8,9,15,17,21,23,24,25,26,27,28,29,57,89,91,131,
  133,153,195,197,202,203,204,205,206,207,208,209,210,211,212,213,214,215,
  216,217,218,441,491,538,
703,701,700,699,698,697,696,695,694,693,692,690,134,0,21,133,441,
493,490,426,424,423,422,38,1,0,51,52,
493,490,426,424,423,422,38,1,0,51,52,
426,424,423,422,38,0,39,40,41,45,46,47,49,
704,703,701,700,699,698,697,696,695,694,693,692,691,690,689,688,687,686,685,
  684,683,682,681,680,526,525,523,520,519,518,517,516,510,508,506,459,458,
  454,428,424,0,46,54,86,90,91,93,139,140,144,145,146,147,148,149,153,156,
  157,184,186,188,194,195,196,198,199,202,204,219,223,224,228,412,413,414,
  415,416,417,418,419,420,421,
426,424,423,422,38,0,39,40,41,45,46,47,49,
454,0,86,
426,424,423,422,38,1,0,51,52,
426,424,423,422,38,1,0,51,52,
426,424,423,422,38,1,0,51,52,
702,701,700,699,698,695,693,685,684,683,681,526,422,0,154,
426,424,423,422,38,0,39,40,41,45,46,47,49,
426,424,423,422,38,0,39,40,41,45,46,47,49,
695,694,693,692,687,686,685,684,683,0,
702,700,699,698,683,682,0,
703,702,701,700,699,698,697,696,695,694,693,692,691,690,689,688,687,686,685,
  684,683,682,681,680,526,525,523,520,519,518,517,516,510,508,506,459,458,
  454,428,424,422,232,1,0,16,
697,695,694,693,692,687,686,685,684,683,544,522,521,520,519,518,517,516,515,
  514,513,512,511,510,509,508,507,506,505,504,503,502,501,500,499,498,497,
  496,495,494,492,458,431,426,424,423,422,38,1,0,
697,695,694,693,692,687,686,685,684,683,544,522,521,520,519,518,517,516,515,
  514,513,512,511,510,509,508,507,506,505,504,503,502,501,500,499,498,497,
  496,495,494,492,458,431,426,424,423,422,38,1,0,
459,0,91,
459,0,91,
459,0,91,
459,0,91,
459,0,91,
459,0,91,
459,0,91,
459,0,91,
459,0,91,
459,0,91,
459,0,91,
703,701,700,699,698,697,696,695,694,693,692,691,690,687,686,685,684,683,682,
  537,535,534,533,532,531,530,529,528,527,526,525,524,523,520,517,516,490,
  459,428,230,134,0,2,6,7,8,9,15,17,21,23,24,25,26,27,28,29,57,89,91,131,
  133,153,195,197,202,203,204,205,206,207,208,209,210,211,212,213,214,215,
  216,217,218,441,491,538,
687,686,685,684,683,0,
703,701,700,699,698,697,696,695,694,693,692,691,690,687,686,685,684,683,682,
  537,535,534,533,532,531,530,529,528,527,526,520,517,516,490,459,428,230,
  134,1,0,51,52,
703,701,700,699,698,697,696,695,694,693,692,691,690,687,686,685,684,683,682,
  537,535,534,533,532,531,530,529,528,527,526,520,517,516,490,459,428,230,
  134,1,0,51,52,
703,701,700,699,698,697,696,695,694,693,692,691,690,687,686,685,684,683,682,
  537,535,534,533,532,531,530,529,528,527,526,520,517,516,490,459,428,230,
  134,1,0,51,52,
703,701,700,699,698,697,696,695,694,693,692,691,690,687,686,685,684,683,682,
  537,535,534,533,532,531,530,529,528,527,526,520,517,516,490,459,428,230,
  134,1,0,51,52,
703,701,700,699,698,697,696,695,694,693,692,691,690,687,686,685,684,683,682,
  537,535,534,533,532,531,530,529,528,527,526,520,517,516,490,459,428,230,
  134,1,0,51,52,
703,701,700,699,698,697,696,695,694,693,692,691,690,687,686,685,684,683,682,
  537,535,534,533,532,531,530,529,528,527,526,520,517,516,490,459,428,230,
  134,1,0,51,52,
703,701,700,699,698,697,696,695,694,693,692,691,690,687,686,685,684,683,682,
  537,535,534,533,532,531,530,529,528,527,526,520,517,516,490,459,428,230,
  134,1,0,51,52,
703,701,700,699,698,697,696,695,694,693,692,691,690,687,686,685,684,683,682,
  537,535,534,533,532,531,530,529,528,527,526,525,524,523,520,517,516,490,
  459,428,230,134,1,0,51,52,
703,701,700,699,698,697,696,695,694,693,692,691,690,687,686,685,684,683,682,
  537,535,534,533,532,531,530,529,528,527,526,525,524,523,520,517,516,490,
  459,428,230,134,1,0,51,52,
703,701,700,699,698,697,696,695,694,693,692,691,690,687,686,685,684,683,682,
  537,535,534,533,532,531,530,529,528,527,526,525,524,523,520,517,516,490,
  459,428,230,134,1,0,51,52,
703,701,700,699,698,697,696,695,694,693,692,691,690,687,686,685,684,683,682,
  537,535,534,533,532,531,530,529,528,527,526,525,524,523,520,517,516,490,
  459,428,230,134,1,0,51,52,
703,701,700,699,698,697,696,695,694,693,692,691,690,687,686,685,684,683,682,
  537,535,534,533,532,531,530,529,528,527,526,525,524,523,520,517,516,490,
  459,428,230,134,1,0,51,52,
703,701,700,699,698,697,696,695,694,693,692,691,690,687,686,685,684,683,682,
  537,535,534,533,532,531,530,529,528,527,526,525,524,523,520,517,516,490,
  459,428,230,134,1,0,51,52,
703,701,700,699,698,697,696,695,694,693,692,691,690,687,686,685,684,683,682,
  537,535,534,533,532,531,530,529,528,527,526,525,524,523,520,517,516,490,
  459,428,230,134,1,0,51,52,
703,701,700,699,698,697,696,695,694,693,692,691,690,687,686,685,684,683,682,
  537,535,534,533,532,531,530,529,528,527,526,525,524,523,520,517,516,490,
  459,428,230,134,1,0,51,52,
703,701,700,699,698,697,696,695,694,693,692,691,690,687,686,685,684,683,682,
  537,535,534,533,532,531,530,529,528,527,526,525,524,523,520,517,516,490,
  459,428,230,134,1,0,51,52,
703,701,700,699,698,697,696,695,694,693,692,691,690,687,686,685,684,683,682,
  537,535,534,533,532,531,530,529,528,527,526,525,524,523,520,517,516,490,
  459,428,230,134,1,0,51,52,
703,701,700,699,698,697,696,695,694,693,692,691,690,687,686,685,684,683,682,
  537,535,534,533,532,531,530,529,528,527,526,525,524,523,520,517,516,490,
  459,428,230,134,1,0,51,52,
703,701,700,699,698,697,696,695,694,693,692,691,690,687,686,685,684,683,682,
  537,535,534,533,532,531,530,529,528,527,526,525,524,523,520,517,516,490,
  459,428,230,134,1,0,51,52,
703,701,700,699,698,697,696,695,694,693,692,691,690,687,686,685,684,683,682,
  537,535,534,533,532,531,530,529,528,527,526,525,524,523,520,517,516,490,
  459,428,230,134,1,0,51,52,
703,701,700,699,698,697,696,695,694,693,692,691,690,687,686,685,684,683,682,
  537,535,534,533,532,531,530,529,528,527,526,525,524,523,520,517,516,490,
  459,428,230,134,1,0,51,52,
703,701,700,699,698,697,696,695,694,693,692,691,690,687,686,685,684,683,682,
  537,535,534,533,532,531,530,529,528,527,526,525,524,523,520,517,516,490,
  459,428,230,134,1,0,51,52,
703,701,700,699,698,697,696,695,694,693,692,691,690,687,686,685,684,683,682,
  537,535,534,533,532,531,530,529,528,527,526,525,524,523,520,517,516,490,
  459,428,230,134,1,0,51,52,
703,701,700,699,698,697,696,695,694,693,692,691,690,687,686,685,684,683,682,
  537,535,534,533,532,531,530,529,528,527,526,525,524,523,520,517,516,490,
  459,428,230,134,1,0,51,52,
703,701,700,699,698,697,696,695,694,693,692,691,690,687,686,685,684,683,682,
  537,535,534,533,532,531,530,529,528,527,526,525,524,523,520,517,516,490,
  459,428,230,134,1,0,51,52,
703,701,700,699,698,697,696,695,694,693,692,691,690,687,686,685,684,683,682,
  537,535,534,533,532,531,530,529,528,527,526,525,524,523,520,517,516,490,
  459,428,230,134,1,0,51,52,
703,701,700,699,698,697,696,695,694,693,692,691,690,687,686,685,684,683,682,
  537,535,534,533,532,531,530,529,528,527,526,525,524,523,520,517,516,490,
  459,428,230,134,1,0,51,52,
703,701,700,699,698,697,696,695,694,693,692,691,690,687,686,685,684,683,682,
  537,535,534,533,532,531,530,529,528,527,526,525,524,523,520,517,516,490,
  459,428,230,134,1,0,51,52,
703,701,700,699,698,697,696,695,694,693,692,691,690,687,686,685,684,683,682,
  537,535,534,533,532,531,530,529,528,527,526,525,524,523,520,517,516,490,
  459,428,230,134,1,0,51,52,
703,701,700,699,698,697,696,695,694,693,692,691,690,687,686,685,684,683,682,
  537,535,534,533,532,531,530,529,528,527,526,525,524,523,520,517,516,490,
  459,428,230,134,1,0,51,52,
703,701,700,699,698,697,696,695,694,693,692,691,690,687,686,685,684,683,682,
  537,535,534,533,532,531,530,529,528,527,526,525,524,523,520,517,516,490,
  459,428,230,134,1,0,51,52,
703,701,700,699,698,697,696,695,694,693,692,691,690,687,686,685,684,683,682,
  537,535,534,533,532,531,530,529,528,527,526,525,524,523,520,517,516,490,
  459,428,230,134,1,0,51,52,
703,701,700,699,698,697,696,695,694,693,692,691,690,687,686,685,684,683,682,
  537,535,534,533,532,531,530,529,528,527,526,525,524,523,520,517,516,490,
  459,428,230,134,1,0,51,52,
703,701,700,699,698,697,696,695,694,693,692,691,690,687,686,685,684,683,682,
  537,535,534,533,532,531,530,529,528,527,526,525,524,523,520,517,516,490,
  459,428,230,134,1,0,51,52,
703,701,700,699,698,697,696,695,694,693,692,691,690,687,686,685,684,683,682,
  537,535,534,533,532,531,530,529,528,527,526,525,524,523,520,517,516,490,
  459,428,230,134,1,0,51,52,
703,701,700,699,698,697,696,695,694,693,692,691,690,687,686,685,684,683,682,
  537,535,534,533,532,531,530,529,528,527,526,525,524,523,520,517,516,490,
  459,428,230,134,1,0,51,52,
703,701,700,699,698,697,696,695,694,693,692,691,690,687,686,685,684,683,682,
  537,535,534,533,532,531,530,529,528,527,526,525,524,523,520,517,516,490,
  459,428,230,134,1,0,51,52,
703,701,700,699,698,697,696,695,694,693,692,691,690,687,686,685,684,683,682,
  537,535,534,533,532,531,530,529,528,527,526,525,524,523,520,517,516,490,
  459,428,230,134,1,0,51,52,
426,424,423,422,38,0,39,40,41,45,46,47,49,
426,424,423,422,38,0,39,40,41,45,46,47,49,
426,424,423,422,38,0,39,40,41,45,46,47,49,
426,424,423,422,38,0,39,40,41,45,46,47,49,
426,424,423,422,38,0,39,40,41,45,46,47,49,
703,701,700,699,698,697,696,695,694,693,692,690,134,0,21,133,441,
426,424,423,422,38,0,39,40,41,45,46,47,49,
426,424,423,422,38,0,39,40,41,45,46,47,49,
704,703,702,701,700,699,698,697,696,695,694,693,692,691,690,689,688,687,686,
  685,684,683,682,681,680,526,525,523,520,519,518,517,516,510,508,506,459,
  458,454,428,424,422,1,0,
426,424,423,422,38,0,39,40,41,45,46,47,49,
426,424,423,422,38,0,39,40,41,45,46,47,49,
703,702,701,700,699,698,697,696,695,694,693,692,687,686,685,684,683,519,428,
  424,422,38,1,0,51,52,
426,424,423,422,38,1,0,51,52,
426,424,423,422,38,1,0,51,52,
426,424,423,422,38,1,0,51,52,
426,424,423,422,38,1,0,51,52,
703,701,700,699,698,697,696,695,694,693,692,691,690,687,686,685,684,683,682,
  537,535,534,533,532,531,530,529,528,527,526,525,524,523,520,517,516,490,
  459,428,230,134,1,0,51,52,
703,701,700,699,698,697,696,695,694,693,692,691,690,687,686,685,684,683,682,
  537,535,534,533,532,531,530,529,528,527,526,525,524,523,520,517,516,490,
  459,428,230,134,0,2,6,7,8,9,15,17,21,23,24,25,26,27,28,29,57,89,91,131,
  133,153,195,197,202,203,204,205,206,207,208,209,210,211,212,213,214,215,
  216,217,218,441,491,538,
703,701,700,699,698,697,696,695,694,693,692,691,690,687,686,685,684,683,682,
  537,535,534,533,532,531,530,529,528,527,526,525,524,523,520,517,516,490,
  459,428,230,134,1,0,51,52,
703,701,700,699,698,697,696,695,694,693,692,691,690,687,686,685,684,683,682,
  537,535,534,533,532,531,530,529,528,527,526,525,524,523,520,517,516,490,
  459,428,230,134,0,2,6,7,8,9,15,17,21,23,24,25,26,27,28,29,57,89,91,131,
  133,153,195,197,202,203,204,205,206,207,208,209,210,211,212,213,214,215,
  216,217,218,441,491,538,
426,424,423,422,38,0,39,40,41,45,46,47,49,
426,424,423,422,38,1,0,51,52,
426,424,423,422,38,1,0,51,52,
544,454,426,424,423,422,38,1,0,51,52,
544,454,426,424,423,422,38,1,0,51,52,
544,454,426,424,423,422,38,1,0,51,52,
544,492,454,426,424,423,422,38,1,0,51,52,
544,454,426,424,423,422,38,1,0,51,52,
703,701,700,699,698,697,696,695,694,693,692,691,690,687,686,685,684,683,682,
  537,535,534,533,532,531,530,529,528,527,526,525,524,523,520,517,516,490,
  459,428,230,134,1,0,51,52,
703,701,700,699,698,697,696,695,694,693,692,691,690,687,686,685,684,683,682,
  537,535,534,533,532,531,530,529,528,527,526,525,524,523,520,517,516,490,
  459,428,230,134,1,0,51,52,
703,701,700,699,698,697,696,695,694,693,692,691,690,687,686,685,684,683,682,
  537,535,534,533,532,531,530,529,528,527,526,525,524,523,520,517,516,490,
  459,428,230,134,1,0,51,52,
544,426,424,423,422,38,1,0,51,52,
544,426,424,423,422,38,1,0,51,52,
492,1,0,51,52,
703,702,701,700,699,698,697,696,695,694,693,692,691,690,689,688,687,686,685,
  684,683,681,680,526,525,523,520,519,518,517,516,510,508,506,459,458,454,
  428,424,422,232,1,0,16,
703,702,701,700,699,698,697,696,695,694,693,692,691,690,689,688,687,686,685,
  684,683,682,681,680,526,525,523,520,519,518,517,516,510,508,506,459,458,
  454,428,424,422,232,1,0,16,
492,458,426,424,423,422,38,1,0,51,52,
492,458,0,93,132,
695,694,693,692,687,686,685,684,683,0,
703,697,695,694,693,692,687,686,685,684,683,426,424,423,422,221,220,38,1,0,
  222,
679,678,677,676,675,674,673,672,671,670,669,668,667,666,665,664,663,662,661,
  660,659,658,657,656,655,654,653,652,651,650,649,648,647,646,645,644,643,
  642,641,640,639,638,637,636,635,634,633,632,631,630,629,628,627,626,625,
  624,623,622,621,620,619,618,617,616,615,614,613,612,611,610,609,607,606,
  605,604,603,602,601,600,599,598,597,596,595,593,592,591,590,589,588,587,
  586,585,584,583,582,581,580,579,578,577,576,575,574,573,572,571,570,569,
  568,567,566,565,564,563,562,561,560,559,558,557,556,555,554,553,552,551,
  550,549,548,547,546,545,0,31,32,243,256,257,258,259,260,261,262,263,264,
  265,267,268,269,270,271,272,273,274,275,276,277,278,279,280,281,282,283,
  284,285,286,287,288,289,290,291,292,293,294,295,296,297,298,299,300,301,
  302,303,304,305,306,307,308,309,310,311,312,313,314,315,316,317,318,319,
  320,321,322,323,324,325,326,327,328,329,330,331,332,333,334,335,336,337,
  338,339,340,341,342,343,345,346,347,348,349,350,351,352,353,354,355,356,
  357,358,359,360,361,362,363,364,365,366,367,368,369,370,371,372,373,374,
  375,376,377,378,379,380,381,382,383,384,385,386,387,388,389,390,391,392,
  393,394,395,396,397,398,399,400,401,402,403,404,405,406,407,408,409,410,
  411,
703,702,701,700,699,698,697,696,695,694,693,692,691,690,689,688,687,686,685,
  684,683,681,680,526,525,523,520,519,518,517,516,510,508,506,459,458,454,
  428,424,422,232,1,0,16,
454,0,86,
703,701,700,699,698,697,696,695,694,693,692,691,690,687,686,685,684,683,682,
  681,537,535,534,533,532,531,530,529,528,527,526,525,524,523,520,517,516,
  490,459,428,230,134,0,2,6,7,8,9,10,12,15,17,18,20,21,23,24,25,26,27,28,
  29,57,89,91,92,131,133,153,195,197,202,203,204,205,206,207,208,209,210,
  211,212,213,214,215,216,217,218,434,441,484,491,538,
703,701,700,699,698,697,696,695,694,693,692,691,690,687,686,685,684,683,682,
  681,537,535,534,533,532,531,530,529,528,527,526,525,524,523,520,517,516,
  490,459,428,230,134,0,2,6,7,8,9,10,12,15,17,21,23,24,25,26,27,28,29,57,
  89,91,131,133,153,195,197,202,203,204,205,206,207,208,209,210,211,212,
  213,214,215,216,217,218,434,441,491,538,
492,426,424,423,422,38,1,0,132,
492,426,424,423,422,38,1,0,132,
492,426,424,423,422,38,1,0,132,
492,426,424,423,422,38,1,0,132,
492,426,424,423,422,38,1,0,132,
704,703,701,700,699,698,697,696,695,694,693,692,691,690,689,688,687,686,685,
  684,683,682,681,680,526,525,523,520,519,518,517,516,510,508,506,460,459,
  458,454,428,424,422,38,1,0,46,51,52,54,86,91,93,96,139,140,144,145,146,
  147,148,149,153,156,157,184,186,188,194,195,196,198,199,202,204,219,223,
  224,228,412,413,414,415,416,417,418,419,420,421,
703,701,700,699,698,697,696,695,694,693,692,691,690,687,686,685,684,683,682,
  537,535,534,533,532,531,530,529,528,527,526,525,524,523,520,517,516,490,
  459,428,230,134,1,0,51,52,
426,424,423,422,38,0,39,40,41,45,46,47,49,
426,424,423,422,38,0,39,40,41,45,46,47,49,
426,424,423,422,38,0,39,40,41,45,46,47,49,
695,694,693,692,687,686,685,684,683,0,
687,686,685,684,683,0,
703,702,701,700,699,698,697,696,695,694,693,692,691,690,689,688,687,686,685,
  684,683,682,681,680,526,525,523,520,519,518,517,516,510,508,506,459,458,
  454,428,424,422,1,0,
682,0,
703,701,700,699,698,697,696,695,694,693,692,690,134,1,0,51,52,
703,701,700,699,698,697,696,695,694,693,692,691,690,687,686,685,684,683,682,
  537,535,534,533,532,531,530,529,528,527,526,525,524,523,520,517,516,490,
  459,428,230,134,0,2,6,7,8,9,15,17,21,23,24,25,26,27,28,29,57,89,91,131,
  133,153,195,197,202,203,204,205,206,207,208,209,210,211,212,213,214,215,
  216,217,218,441,491,538,
703,701,700,699,698,697,696,695,694,693,692,691,690,687,686,685,684,683,682,
  537,535,534,533,532,531,530,529,528,527,526,525,524,523,520,517,516,490,
  459,428,230,134,0,2,6,7,8,9,15,17,21,23,24,25,26,27,28,29,57,89,91,131,
  133,153,195,197,202,203,204,205,206,207,208,209,210,211,212,213,214,215,
  216,217,218,441,491,538,
703,701,700,699,698,697,696,695,694,693,692,691,690,687,686,685,684,683,682,
  537,535,534,533,532,531,530,529,528,527,526,525,524,523,520,517,516,490,
  459,428,230,134,0,2,6,7,8,9,15,17,21,23,24,25,26,27,28,29,57,89,91,131,
  133,153,195,197,202,203,204,205,206,207,208,209,210,211,212,213,214,215,
  216,217,218,441,491,538,
703,701,700,699,698,697,696,695,694,693,692,691,690,687,686,685,684,683,682,
  537,535,534,533,532,531,530,529,528,527,526,525,524,523,520,517,516,490,
  459,428,230,134,0,2,6,7,8,9,15,17,21,23,24,25,26,27,28,29,57,89,91,131,
  133,153,195,197,202,203,204,205,206,207,208,209,210,211,212,213,214,215,
  216,217,218,441,491,538,
703,701,700,699,698,697,696,695,694,693,692,691,690,687,686,685,684,683,682,
  537,535,534,533,532,531,530,529,528,527,526,525,524,523,520,517,516,490,
  459,428,230,134,0,2,6,7,8,9,15,17,21,23,24,25,26,27,28,29,57,89,91,131,
  133,153,195,197,202,203,204,205,206,207,208,209,210,211,212,213,214,215,
  216,217,218,441,491,538,
703,701,700,699,698,697,696,695,694,693,692,691,690,687,686,685,684,683,682,
  537,535,534,533,532,531,530,529,528,527,526,525,524,523,520,517,516,490,
  459,428,230,134,0,2,6,7,8,9,15,17,21,23,24,25,26,27,28,29,57,89,91,131,
  133,153,195,197,202,203,204,205,206,207,208,209,210,211,212,213,214,215,
  216,217,218,441,491,538,
703,701,700,699,698,697,696,695,694,693,692,691,690,687,686,685,684,683,682,
  537,535,534,533,532,531,530,529,528,527,526,525,524,523,520,517,516,490,
  459,428,230,134,0,2,6,7,8,9,15,17,21,23,24,25,26,27,28,29,57,89,91,131,
  133,153,195,197,202,203,204,205,206,207,208,209,210,211,212,213,214,215,
  216,217,218,441,491,538,
703,701,700,699,698,697,696,695,694,693,692,691,690,687,686,685,684,683,682,
  537,535,534,533,532,531,530,529,528,527,526,525,524,523,520,517,516,490,
  459,428,230,134,0,2,6,7,8,9,15,17,21,23,24,25,26,27,28,29,57,89,91,131,
  133,153,195,197,202,203,204,205,206,207,208,209,210,211,212,213,214,215,
  216,217,218,441,491,538,
703,701,700,699,698,697,696,695,694,693,692,691,690,687,686,685,684,683,682,
  537,535,534,533,532,531,530,529,528,527,526,525,524,523,520,517,516,490,
  459,428,230,134,0,2,6,7,8,9,15,17,21,23,24,25,26,27,28,29,57,89,91,131,
  133,153,195,197,202,203,204,205,206,207,208,209,210,211,212,213,214,215,
  216,217,218,441,491,538,
703,701,700,699,698,697,696,695,694,693,692,691,690,687,686,685,684,683,682,
  537,535,534,533,532,531,530,529,528,527,526,525,524,523,520,517,516,490,
  459,428,230,134,0,2,6,7,8,9,15,17,21,23,24,25,26,27,28,29,57,89,91,131,
  133,153,195,197,202,203,204,205,206,207,208,209,210,211,212,213,214,215,
  216,217,218,441,491,538,
458,1,0,51,52,
703,701,700,699,698,697,696,695,694,693,692,691,690,687,686,685,684,683,682,
  537,535,534,533,532,531,530,529,528,527,526,520,517,516,490,459,428,230,
  134,0,2,6,7,8,9,15,17,21,89,91,131,133,153,197,205,206,207,208,209,210,
  211,212,213,214,215,216,217,218,441,491,538,
703,701,700,699,698,697,696,695,694,693,692,691,690,687,686,685,684,683,682,
  537,535,534,533,532,531,530,529,528,527,526,520,517,516,490,459,428,230,
  134,0,2,6,7,8,9,15,17,21,89,91,131,133,153,197,205,206,207,208,209,210,
  211,212,213,214,215,216,217,218,441,491,538,
703,701,700,699,698,697,696,695,694,693,692,691,690,687,686,685,684,683,682,
  537,535,534,533,532,531,530,529,528,527,526,520,517,516,490,459,428,230,
  134,0,2,6,7,8,9,15,17,21,89,91,131,133,153,197,205,206,207,208,209,210,
  211,212,213,214,215,216,217,218,441,491,538,
703,701,700,699,698,697,696,695,694,693,692,691,690,687,686,685,684,683,682,
  537,535,534,533,532,531,530,529,528,527,526,520,517,516,490,459,428,230,
  134,0,2,6,7,8,9,15,17,21,89,91,131,133,153,197,205,206,207,208,209,210,
  211,212,213,214,215,216,217,218,441,491,538,
703,701,700,699,698,697,696,695,694,693,692,691,690,687,686,685,684,683,682,
  537,535,534,533,532,531,530,529,528,527,526,520,517,516,490,459,428,230,
  134,0,2,6,7,8,9,15,17,21,89,91,131,133,153,197,205,206,207,208,209,210,
  211,212,213,214,215,216,217,218,441,491,538,
703,701,700,699,698,697,696,695,694,693,692,691,690,687,686,685,684,683,682,
  537,535,534,533,532,531,530,529,528,527,526,525,524,523,520,517,516,490,
  459,428,230,134,0,2,6,7,8,9,15,17,21,25,29,89,91,131,133,153,195,197,
  202,203,204,205,206,207,208,209,210,211,212,213,214,215,216,217,218,441,
  491,538,
703,701,700,699,698,697,696,695,694,693,692,691,690,687,686,685,684,683,682,
  537,535,534,533,532,531,530,529,528,527,526,525,524,523,520,517,516,490,
  459,428,230,134,0,2,6,7,8,9,15,17,21,25,29,89,91,131,133,153,195,197,
  202,203,204,205,206,207,208,209,210,211,212,213,214,215,216,217,218,441,
  491,538,
703,701,700,699,698,697,696,695,694,693,692,691,690,687,686,685,684,683,682,
  537,535,534,533,532,531,530,529,528,527,526,525,524,523,520,517,516,490,
  459,428,230,134,0,2,6,7,8,9,15,17,21,25,27,29,89,91,131,133,153,195,197,
  202,203,204,205,206,207,208,209,210,211,212,213,214,215,216,217,218,441,
  491,538,
703,701,700,699,698,697,696,695,694,693,692,691,690,687,686,685,684,683,682,
  537,535,534,533,532,531,530,529,528,527,526,525,524,523,520,517,516,490,
  459,428,230,134,0,2,6,7,8,9,15,17,21,25,27,29,89,91,131,133,153,195,197,
  202,203,204,205,206,207,208,209,210,211,212,213,214,215,216,217,218,441,
  491,538,
703,701,700,699,698,697,696,695,694,693,692,691,690,687,686,685,684,683,682,
  537,535,534,533,532,531,530,529,528,527,526,525,524,523,520,517,516,490,
  459,428,230,134,0,2,6,7,8,9,15,17,21,25,27,29,89,91,131,133,153,195,197,
  202,203,204,205,206,207,208,209,210,211,212,213,214,215,216,217,218,441,
  491,538,
703,701,700,699,698,697,696,695,694,693,692,691,690,687,686,685,684,683,682,
  537,535,534,533,532,531,530,529,528,527,526,525,524,523,520,517,516,490,
  459,428,230,134,0,2,6,7,8,9,15,17,21,25,27,29,89,91,131,133,153,195,197,
  202,203,204,205,206,207,208,209,210,211,212,213,214,215,216,217,218,441,
  491,538,
703,701,700,699,698,697,696,695,694,693,692,691,690,687,686,685,684,683,682,
  537,535,534,533,532,531,530,529,528,527,526,525,524,523,520,517,516,490,
  459,428,230,134,0,2,6,7,8,9,15,17,21,24,25,27,29,89,91,131,133,153,195,
  197,202,203,204,205,206,207,208,209,210,211,212,213,214,215,216,217,218,
  441,491,538,
703,701,700,699,698,697,696,695,694,693,692,691,690,687,686,685,684,683,682,
  537,535,534,533,532,531,530,529,528,527,526,525,524,523,520,517,516,490,
  459,428,230,134,0,2,6,7,8,9,15,17,21,24,25,27,29,89,91,131,133,153,195,
  197,202,203,204,205,206,207,208,209,210,211,212,213,214,215,216,217,218,
  441,491,538,
703,701,700,699,698,697,696,695,694,693,692,691,690,687,686,685,684,683,682,
  537,535,534,533,532,531,530,529,528,527,526,525,524,523,520,517,516,490,
  459,428,230,134,0,2,6,7,8,9,15,17,21,23,24,25,27,29,89,91,131,133,153,
  195,197,202,203,204,205,206,207,208,209,210,211,212,213,214,215,216,217,
  218,441,491,538,
703,701,700,699,698,697,696,695,694,693,692,691,690,687,686,685,684,683,682,
  537,535,534,533,532,531,530,529,528,527,526,525,524,523,520,517,516,490,
  459,428,230,134,0,2,6,7,8,9,15,17,21,23,24,25,27,29,89,91,131,133,153,
  195,197,202,203,204,205,206,207,208,209,210,211,212,213,214,215,216,217,
  218,441,491,538,
703,701,700,699,698,697,696,695,694,693,692,691,690,687,686,685,684,683,682,
  537,535,534,533,532,531,530,529,528,527,526,525,524,523,520,517,516,490,
  459,428,230,134,0,2,6,7,8,9,15,17,21,23,24,25,27,28,29,89,91,131,133,
  153,195,197,202,203,204,205,206,207,208,209,210,211,212,213,214,215,216,
  217,218,441,491,538,
703,701,700,699,698,697,696,695,694,693,692,691,690,687,686,685,684,683,682,
  537,535,534,533,532,531,530,529,528,527,526,525,524,523,520,517,516,490,
  459,428,230,134,0,2,6,7,8,9,15,17,21,23,24,25,27,28,29,89,91,131,133,
  153,195,197,202,203,204,205,206,207,208,209,210,211,212,213,214,215,216,
  217,218,441,491,538,
426,424,423,422,38,1,0,51,52,
426,424,423,422,38,0,39,40,41,45,46,47,49,
426,424,423,422,38,0,39,40,41,45,46,47,49,
426,424,423,422,38,0,39,40,41,45,46,47,49,
426,424,423,422,38,0,39,40,41,45,46,47,49,
426,424,423,422,38,1,0,51,52,
426,424,423,422,38,1,0,51,52,
426,424,423,422,38,0,39,40,41,45,46,47,49,
703,701,700,699,698,697,696,695,694,693,692,691,690,687,686,685,684,683,682,
  537,535,534,533,532,531,530,529,528,527,526,525,524,523,520,517,516,490,
  459,428,230,134,0,2,6,7,8,9,15,17,21,23,24,25,26,27,28,29,57,89,91,131,
  133,153,195,197,202,203,204,205,206,207,208,209,210,211,212,213,214,215,
  216,217,218,441,491,538,
703,701,700,699,698,697,696,695,694,693,692,691,690,687,686,685,684,683,682,
  537,535,534,533,532,531,530,529,528,527,526,525,524,523,520,517,516,490,
  459,428,230,134,0,2,6,7,8,9,15,17,21,23,24,25,26,27,28,29,57,89,91,131,
  133,153,195,197,202,203,204,205,206,207,208,209,210,211,212,213,214,215,
  216,217,218,441,491,538,
703,701,700,699,698,697,696,695,694,693,692,691,690,687,686,685,684,683,682,
  537,535,534,533,532,531,530,529,528,527,526,525,524,523,520,517,516,490,
  459,428,230,134,0,2,6,7,8,9,15,17,21,23,24,25,26,27,28,29,57,89,91,131,
  133,153,195,197,202,203,204,205,206,207,208,209,210,211,212,213,214,215,
  216,217,218,441,491,538,
492,0,132,
703,702,701,700,699,698,697,696,695,694,693,692,691,690,689,688,687,686,685,
  684,683,682,681,680,526,525,523,520,519,518,517,516,510,508,506,459,458,
  454,428,424,422,232,1,0,16,
703,701,700,699,698,697,696,695,694,693,692,691,690,687,686,685,684,683,682,
  681,540,537,535,534,533,532,531,530,529,528,527,526,525,524,523,520,517,
  516,490,459,428,240,239,238,237,236,235,234,233,230,134,1,0,51,52,
703,701,700,699,698,697,696,695,694,693,692,691,690,687,686,685,684,683,682,
  681,537,535,534,533,532,531,530,529,528,527,526,525,524,523,520,517,516,
  490,459,428,230,134,1,0,51,52,
544,0,266,
544,426,424,423,422,38,1,0,51,52,
703,702,701,700,699,698,697,696,695,694,693,692,691,690,689,688,687,686,685,
  684,683,681,680,526,525,523,520,519,518,517,516,510,508,506,459,458,454,
  428,424,422,232,1,0,16,
703,701,700,699,698,697,696,695,694,693,692,691,690,687,686,685,684,683,682,
  537,535,534,533,532,531,530,529,528,527,526,525,524,523,520,517,516,490,
  459,428,230,134,1,0,51,52,
492,426,424,423,422,38,1,0,132,
703,701,700,699,698,697,696,695,694,693,692,690,134,1,0,51,52,
704,703,701,700,699,698,697,696,695,694,693,692,691,690,689,688,687,686,685,
  684,683,682,681,680,526,525,523,520,519,518,517,516,510,508,506,460,459,
  458,454,428,426,424,423,422,38,1,0,51,52,
704,703,701,700,699,698,697,696,695,694,693,692,691,690,689,688,687,686,685,
  684,683,682,681,680,526,525,523,520,519,518,517,516,510,508,506,460,459,
  458,454,428,426,424,423,422,38,1,0,
426,424,423,422,38,0,39,40,41,45,46,47,49,
703,701,700,699,698,697,696,695,694,693,692,691,690,687,686,685,684,683,682,
  537,535,534,533,532,531,530,529,528,527,526,525,524,523,520,517,516,490,
  459,428,230,134,0,2,6,7,8,9,15,17,21,23,24,25,26,27,28,29,57,89,91,131,
  133,153,195,197,202,203,204,205,206,207,208,209,210,211,212,213,214,215,
  216,217,218,441,491,538,
703,702,701,700,699,698,697,696,695,694,693,692,691,690,689,688,687,686,685,
  684,683,682,681,680,526,525,523,520,519,518,517,516,510,508,506,459,458,
  454,428,424,422,1,0,
687,686,685,684,683,0,
703,701,700,699,698,697,696,695,694,693,692,690,134,0,3,133,441,536,
458,0,93,
458,0,93,
458,0,93,
458,0,93,
458,0,93,
458,0,93,
458,0,93,
458,0,93,
458,0,93,
458,0,93,
458,0,93,
544,522,521,520,519,518,517,516,515,514,513,512,511,510,509,508,507,506,505,
  504,503,502,501,500,499,498,497,496,495,494,492,458,431,424,422,38,1,0,
  196,198,199,200,201,
544,522,521,520,519,518,517,516,515,514,513,512,511,510,509,508,507,506,505,
  504,503,502,501,500,499,498,497,496,495,494,492,458,431,424,422,38,1,0,
  196,198,199,200,201,
544,517,516,515,514,513,512,511,510,509,508,507,506,505,504,503,502,501,500,
  499,498,497,496,495,494,492,458,431,426,424,423,422,38,1,0,194,195,
544,517,516,515,514,513,512,511,510,509,508,507,506,505,504,503,502,501,500,
  499,498,497,496,495,494,492,458,431,426,424,423,422,38,1,0,194,195,
544,517,516,515,514,513,512,511,510,509,508,507,506,505,504,503,502,501,500,
  499,498,497,496,495,494,492,458,431,426,424,423,422,38,1,0,194,195,
544,517,516,515,514,513,512,511,510,509,508,507,506,505,504,503,502,501,500,
  499,498,497,496,495,494,492,458,431,426,424,423,422,38,1,0,194,195,
544,515,514,513,512,511,510,509,508,507,506,505,504,503,502,501,500,499,498,
  497,496,495,494,492,458,431,426,424,423,422,38,1,0,190,191,192,193,
544,515,514,513,512,511,510,509,508,507,506,505,504,503,502,501,500,499,498,
  497,496,495,494,492,458,431,426,424,423,422,38,1,0,190,191,192,193,
544,511,510,509,508,507,506,505,504,503,502,501,500,499,498,497,496,495,494,
  492,458,431,426,424,423,422,38,1,0,188,189,
544,511,510,509,508,507,506,505,504,503,502,501,500,499,498,497,496,495,494,
  492,458,431,426,424,423,422,38,1,0,188,189,
544,509,508,507,506,505,504,503,502,501,500,499,498,497,496,495,494,492,458,
  431,426,424,423,422,38,1,0,186,187,
544,509,508,507,506,505,504,503,502,501,500,499,498,497,496,495,494,492,458,
  431,426,424,423,422,38,1,0,186,187,
426,424,423,422,38,0,39,40,41,45,46,47,49,
426,424,423,422,38,0,39,40,41,45,46,47,49,
426,424,423,422,38,0,39,40,41,45,46,47,49,
540,240,239,238,237,236,235,234,233,1,0,51,52,
703,702,701,700,699,698,697,696,695,694,693,692,691,690,689,688,687,686,685,
  684,683,682,681,680,526,525,523,520,519,518,517,516,510,508,506,459,458,
  454,428,424,422,232,1,0,
703,701,700,699,698,697,696,695,694,693,692,691,690,687,686,685,684,683,682,
  681,537,535,534,533,532,531,530,529,528,527,526,525,524,523,520,517,516,
  490,459,428,230,134,0,2,6,7,8,9,10,12,15,17,18,20,21,23,24,25,26,27,28,
  29,57,89,91,131,133,153,195,197,202,203,204,205,206,207,208,209,210,211,
  212,213,214,215,216,217,218,434,441,484,491,538,
703,701,700,699,698,697,696,695,694,693,692,691,690,687,686,685,684,683,682,
  537,535,534,533,532,531,530,529,528,527,526,525,524,523,520,517,516,490,
  459,428,230,134,0,2,6,7,8,9,15,17,21,23,24,25,26,27,28,29,57,89,91,131,
  133,153,195,197,202,203,204,205,206,207,208,209,210,211,212,213,214,215,
  216,217,218,441,491,538,
703,701,700,699,698,697,696,695,694,693,692,690,134,0,21,133,441,
426,424,423,422,38,1,0,51,52,
703,701,700,699,698,697,696,695,694,693,692,690,687,686,685,684,683,458,428,
  1,0,51,52,137,
458,1,0,51,52,
458,1,0,51,52,
540,240,239,238,237,236,235,234,233,0,4,241,594,
426,424,423,422,38,0,39,40,41,45,46,47,49,
458,0,93,

};


static unsigned const char ag_astt[27983] = {
  8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,7,1,1,9,5,2,2,2,2,2,2,
  2,2,2,2,2,2,1,8,8,8,8,8,2,2,2,2,1,8,1,7,1,0,1,1,1,1,1,1,1,1,1,1,1,5,5,5,5,
  5,5,5,5,5,5,5,5,5,5,1,7,1,3,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,4,4,4,2,4,4,
  4,4,2,4,4,4,7,2,1,1,1,1,1,1,1,1,1,1,1,1,1,1,7,1,3,1,1,1,1,1,1,1,1,1,1,1,1,
  1,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,
  5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,
  5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,
  5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,
  5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,
  5,5,5,5,5,5,5,5,5,5,5,1,7,1,3,1,7,2,8,8,1,1,1,1,1,3,7,3,3,1,3,1,1,1,1,1,8,
  8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,
  8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,
  8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,
  8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,
  8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,
  5,5,8,8,8,8,8,8,8,1,7,1,1,2,2,2,2,2,2,2,2,2,2,2,2,1,1,1,1,1,1,1,1,1,1,1,1,
  1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,
  1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,
  1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,
  1,1,1,1,1,1,1,1,1,1,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,
  8,8,8,8,8,1,1,1,8,8,8,8,8,5,5,1,5,5,5,1,2,5,9,7,1,1,1,1,1,3,1,1,2,1,2,2,1,
  1,1,1,1,1,1,1,1,1,1,1,1,1,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,
  2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,1,1,1,1,1,1,1,1,1,1,1,1,1,
  1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,
  1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,
  1,1,1,1,1,1,1,1,1,1,2,2,2,2,2,2,2,2,2,2,2,2,1,8,8,8,8,8,2,2,2,2,1,3,1,7,1,
  3,3,1,1,3,1,1,1,1,1,5,5,5,5,5,5,5,5,5,5,5,5,5,1,7,1,3,5,5,5,5,5,5,5,5,5,5,
  5,5,5,1,7,1,3,5,5,7,1,3,5,5,7,1,3,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,
  5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,7,1,3,5,1,7,1,3,5,5,5,5,5,5,
  7,1,3,5,5,5,5,5,5,7,1,3,5,5,5,5,5,5,5,5,5,5,5,5,5,1,7,1,3,5,5,5,5,5,5,5,5,
  5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,1,7,1,3,
  5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,
  5,5,5,5,1,7,1,3,5,5,5,5,5,5,5,5,5,5,5,5,5,1,7,1,3,5,5,5,5,5,1,7,1,3,5,5,5,
  5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,1,7,1,3,2,2,2,2,2,2,2,2,2,2,2,2,2,7,1,
  1,1,1,2,2,2,2,2,2,2,2,2,2,2,2,2,7,1,1,1,8,1,7,1,1,8,1,7,1,1,8,8,8,8,8,8,8,
  8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,1,7,
  1,1,2,7,1,1,1,8,8,8,8,8,1,7,1,1,8,8,8,8,8,1,7,1,1,2,2,2,2,2,2,2,2,2,2,2,2,
  2,7,1,1,1,2,2,2,2,2,2,2,2,2,2,2,1,2,2,2,2,2,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,
  1,1,1,1,1,1,1,2,2,7,2,1,1,2,1,2,1,1,1,1,1,1,1,1,1,1,1,2,1,1,1,2,1,1,1,1,1,
  1,2,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,2,2,2,2,2,2,2,2,2,2,2,1,2,2,2,2,2,1,1,
  1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,2,2,7,2,1,1,2,1,2,1,1,1,1,1,1,1,1,
  1,1,1,2,1,1,1,2,1,1,1,1,1,1,2,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,2,2,2,2,2,2,
  2,2,2,2,2,2,2,7,1,1,1,1,1,1,1,1,7,1,1,1,1,1,2,2,7,1,1,1,1,1,1,5,5,5,5,5,5,
  5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,
  5,5,5,5,5,5,5,5,5,5,5,7,1,3,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,
  5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,
  5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,7,1,3,1,1,7,1,1,
  1,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,
  5,5,5,5,5,5,5,5,5,7,1,3,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,
  5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,7,1,3,5,5,5,5,5,5,5,5,5,5,5,
  5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,7,1,3,1,1,
  1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,
  1,1,1,1,1,9,7,3,3,3,1,3,1,3,3,3,3,3,3,3,3,3,3,3,3,3,3,3,3,3,3,3,3,3,3,3,3,
  3,3,3,3,3,3,3,3,3,3,3,3,3,3,3,3,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,
  1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,8,1,7,1,1,1,1,1,1,1,1,1,1,1,1,1,1,
  1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,
  1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,8,1,7,1,
  1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,
  1,1,1,1,1,2,2,2,2,2,2,2,2,2,2,2,2,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,
  1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,
  1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,
  1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,
  1,1,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,1,1,1,
  8,8,8,8,8,1,8,8,1,1,1,1,1,2,3,7,1,1,1,1,1,3,3,1,3,1,1,1,1,1,1,2,1,2,2,1,1,
  1,1,1,1,1,1,1,1,1,1,1,1,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,
  2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,1,1,1,1,1,1,1,1,1,1,1,1,1,1,
  1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,
  1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,
  1,1,1,1,1,1,1,1,1,8,8,8,8,8,8,8,1,7,1,1,5,5,5,5,5,5,5,7,1,3,5,5,5,5,5,5,5,
  7,1,3,5,5,5,5,5,5,5,7,1,3,5,5,5,5,5,5,5,7,1,3,5,5,5,5,5,5,5,7,1,3,5,5,5,5,
  5,5,5,7,1,3,5,5,5,5,5,5,5,7,1,3,5,5,5,5,5,5,5,7,1,3,5,5,5,5,5,5,5,7,1,3,5,
  5,5,5,5,5,5,7,1,3,5,5,5,5,5,5,5,7,1,3,5,5,5,5,5,5,5,7,1,3,5,5,5,5,5,5,5,7,
  1,3,5,5,5,5,5,5,5,7,1,3,5,5,5,5,5,5,5,5,5,5,7,1,3,5,5,5,5,5,5,5,5,5,5,7,1,
  3,5,5,5,5,5,5,5,5,7,1,3,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,
  5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,7,1,3,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,
  5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,7,1,3,5,5,5,5,5,5,5,5,5,
  5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,7,1,3,5,
  5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,
  5,5,5,5,7,1,3,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,
  5,5,5,5,5,5,5,5,5,5,5,5,7,1,3,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,
  5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,7,1,3,5,5,5,5,5,5,5,5,5,5,5,5,5,5,
  5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,7,1,3,5,5,5,5,5,5,
  5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,7,
  1,3,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,
  5,5,5,5,5,5,5,7,1,3,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,
  5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,7,1,3,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,
  5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,7,1,3,5,5,5,5,5,5,5,5,5,5,5,
  5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,7,1,3,5,5,5,
  5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,
  5,5,7,1,3,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,
  5,5,5,5,5,5,5,5,5,5,7,1,3,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,
  5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,7,1,3,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,
  5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,7,1,3,5,5,5,5,5,5,5,7,
  1,3,5,5,5,5,5,5,5,7,1,3,5,5,5,5,5,5,5,7,1,3,5,5,5,5,5,5,5,7,1,3,5,5,5,5,5,
  5,5,7,1,3,5,5,5,5,5,5,5,7,1,3,5,5,5,5,5,5,5,7,1,3,5,5,5,5,5,5,5,7,1,3,5,5,
  5,5,5,5,5,7,1,3,5,5,5,5,5,5,5,7,1,3,5,5,5,5,5,5,5,7,1,3,5,5,5,5,5,5,5,7,1,
  3,5,5,5,5,5,5,5,7,1,3,5,5,5,5,5,5,5,7,1,3,5,5,5,5,5,5,5,7,1,3,5,5,5,5,5,5,
  5,7,1,3,5,5,5,5,5,5,5,7,1,3,5,5,5,5,5,5,5,7,1,3,5,5,5,5,5,5,5,7,1,3,5,5,5,
  5,5,5,5,7,1,3,5,5,5,5,5,5,5,7,1,3,5,5,5,5,5,5,5,7,1,3,5,5,5,5,5,5,5,7,1,3,
  5,5,5,5,5,5,5,7,1,3,5,5,5,5,5,5,5,7,1,3,5,5,5,5,5,5,5,7,1,3,5,5,5,5,5,5,5,
  7,1,3,5,5,5,5,5,5,5,7,1,3,5,5,5,5,5,5,5,7,1,3,5,5,5,5,5,7,1,3,8,8,8,8,8,8,
  8,8,8,1,7,1,1,8,8,8,8,8,8,8,8,8,1,7,1,1,8,8,8,8,8,8,8,1,7,1,1,5,5,5,5,5,5,
  5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,7,
  1,3,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,
  5,5,5,5,5,5,5,7,1,3,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,
  5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,7,1,3,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,
  5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,7,1,3,5,5,5,5,5,5,5,5,5,5,5,
  5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,7,1,3,5,5,5,
  5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,
  5,5,7,1,3,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,
  5,5,5,5,5,5,5,5,5,5,7,1,3,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,
  5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,7,1,3,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,
  5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,7,1,3,5,5,5,5,5,5,5,5,
  5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,7,1,3,
  5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,
  5,5,5,5,5,7,1,3,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,
  5,5,5,5,5,5,5,5,5,5,5,5,5,7,1,3,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,
  5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,7,1,3,5,5,5,5,5,5,5,5,5,5,5,5,5,
  5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,7,1,3,5,5,5,5,5,
  5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,
  7,1,3,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,
  5,5,5,5,5,5,5,5,7,1,3,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,
  5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,7,1,3,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,
  5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,7,1,3,5,5,5,5,5,5,5,5,5,5,
  5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,7,1,3,5,5,
  5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,
  5,5,5,7,1,3,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,
  5,5,5,5,5,5,5,5,5,5,5,7,1,3,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,
  5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,7,1,3,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,
  5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,7,1,3,5,5,5,5,5,5,5,
  5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,7,1,
  3,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,
  5,5,5,5,5,5,7,1,3,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,
  5,5,5,5,5,5,5,5,5,5,5,5,5,5,7,1,3,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,
  5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,7,1,3,5,5,5,5,5,5,5,5,5,5,5,5,
  5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,7,1,3,5,5,5,5,
  5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,
  5,7,1,3,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,
  5,5,5,5,5,5,5,5,5,7,1,3,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,
  5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,7,1,3,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,
  5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,7,1,3,5,5,5,5,5,5,5,5,5,
  5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,7,1,3,5,
  5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,
  5,5,5,5,7,1,3,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,
  5,5,5,5,5,5,5,5,5,5,5,5,7,1,3,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,
  5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,7,1,3,5,5,5,5,5,5,5,5,5,5,5,5,5,5,
  5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,7,1,3,5,5,5,5,5,5,
  5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,7,
  1,3,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,
  5,5,5,5,5,5,5,7,1,3,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,
  5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,7,1,3,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,
  5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,7,1,3,5,5,5,5,5,5,5,5,5,5,5,
  5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,7,1,3,5,5,5,
  5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,
  5,5,7,1,3,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,
  5,5,5,5,5,5,5,5,5,5,7,1,3,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,
  5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,7,1,3,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,
  5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,7,1,3,5,5,5,5,5,5,5,5,
  5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,7,1,3,
  5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,
  5,5,5,5,5,7,1,3,5,5,5,5,5,5,5,5,5,7,1,3,5,5,5,5,5,5,5,5,5,7,1,3,5,5,5,5,5,
  5,5,5,5,7,1,3,5,5,5,5,5,5,5,5,5,7,1,3,5,5,5,5,5,7,1,3,5,5,5,5,5,7,1,3,5,5,
  5,5,5,5,5,5,7,1,3,5,5,5,5,5,5,5,5,7,1,3,5,5,5,5,5,5,5,5,7,1,3,5,5,5,5,5,5,
  5,5,5,5,7,1,3,5,5,5,5,5,5,5,5,5,5,7,1,3,5,5,5,5,5,5,5,5,5,5,7,1,3,5,5,5,5,
  5,5,5,5,5,5,7,1,3,5,5,5,5,5,5,5,5,5,5,7,1,3,5,5,5,5,5,5,5,5,5,5,7,1,3,5,5,
  5,5,5,5,5,5,5,5,7,1,3,5,5,5,5,5,5,5,5,5,5,7,1,3,5,5,5,5,5,5,5,5,5,5,7,1,3,
  5,5,5,5,5,5,5,5,5,5,7,1,3,5,5,5,5,5,5,5,5,5,5,7,1,3,5,5,5,5,5,5,5,5,5,5,7,
  1,3,5,5,5,5,5,5,5,5,5,5,7,1,3,8,8,8,8,1,7,1,1,8,1,7,1,1,8,1,7,1,1,8,1,7,1,
  1,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,
  8,8,8,8,8,1,7,1,1,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,
  8,8,8,8,8,8,8,8,8,8,8,8,8,1,7,1,1,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,
  8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,1,7,1,1,8,8,8,8,8,8,8,8,8,8,8,8,
  8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,1,7,1,1,8,8,8,8,
  8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,
  1,7,1,1,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,
  8,8,8,8,8,8,8,8,1,7,1,1,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,
  8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,1,7,1,1,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,
  8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,1,7,1,1,8,8,8,8,8,8,8,8,8,
  8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,1,7,1,1,8,
  8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,
  8,8,8,1,7,1,1,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,
  8,8,8,8,8,8,8,8,8,8,8,1,7,1,1,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,
  8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,1,7,1,1,8,8,8,8,8,8,8,8,8,8,8,8,8,8,
  8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,1,7,1,1,8,8,8,8,8,8,
  8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,1,7,
  1,1,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,
  8,8,8,8,8,8,1,7,1,1,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,
  8,8,8,8,8,8,8,8,8,8,8,8,8,8,1,7,1,1,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,
  8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,1,7,1,1,8,8,8,8,8,8,8,8,8,8,8,
  8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,1,7,1,1,8,8,8,
  8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,
  8,1,7,1,1,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,
  8,8,8,8,8,8,8,8,8,1,7,1,1,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,
  8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,1,7,1,1,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,
  8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,1,7,1,1,8,8,8,8,8,8,8,8,
  8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,1,7,1,1,
  8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,
  8,8,8,8,1,7,1,1,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,
  8,8,8,8,8,8,8,8,8,8,8,8,1,7,1,1,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,
  8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,1,7,1,1,8,8,8,8,8,8,8,8,8,8,8,8,8,
  8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,1,7,1,1,8,8,8,8,8,
  8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,1,
  7,1,1,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,
  8,8,8,8,8,8,8,1,7,1,1,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,
  8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,1,7,1,1,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,
  8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,1,7,1,1,8,8,8,8,8,8,8,8,8,8,
  8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,1,7,1,1,8,8,
  8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,
  8,8,1,7,1,1,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,
  8,8,8,8,8,8,8,8,8,8,1,7,1,1,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,
  8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,1,7,1,1,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,
  8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,1,7,1,1,8,8,8,8,8,8,8,
  8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,1,7,1,
  1,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,
  8,8,8,8,8,1,7,1,1,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,
  8,8,8,8,8,8,8,8,8,8,8,8,8,1,7,1,1,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,
  8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,1,7,1,1,8,8,8,8,8,8,8,8,8,8,8,8,
  8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,1,7,1,1,8,8,8,8,
  8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,
  1,7,1,1,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,
  8,8,8,8,8,8,8,8,1,7,1,1,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,
  8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,1,7,1,1,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,
  8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,1,7,1,1,8,8,8,8,8,8,8,8,8,
  8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,1,7,1,1,8,
  8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,
  8,8,8,1,7,1,1,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,
  8,8,8,8,8,8,8,8,8,8,8,1,7,1,1,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,
  8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,1,7,1,1,8,8,8,8,8,8,8,8,8,8,8,8,8,8,
  8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,1,7,1,1,8,8,8,8,8,8,
  8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,1,7,
  1,1,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,
  8,8,8,8,8,8,1,7,1,1,8,8,8,8,8,8,8,8,1,7,1,1,8,8,8,8,8,8,8,8,1,7,1,1,8,8,8,
  8,8,8,8,8,1,7,1,1,8,8,8,8,8,8,8,8,1,7,1,1,8,8,8,8,1,7,1,1,8,8,8,8,1,7,1,1,
  8,8,8,8,8,8,8,1,7,1,1,8,8,8,8,8,8,8,1,7,1,1,8,8,8,8,8,8,8,1,7,1,1,8,8,8,8,
  8,8,8,8,8,1,7,1,1,8,8,8,8,8,8,8,8,8,1,7,1,1,8,8,8,8,8,8,8,8,8,1,7,1,1,8,8,
  8,8,8,8,8,8,8,1,7,1,1,8,8,8,8,8,8,8,8,8,1,7,1,1,8,8,8,8,8,8,8,8,8,1,7,1,1,
  8,8,8,8,8,8,8,8,8,1,7,1,1,8,8,8,8,8,8,8,8,8,1,7,1,1,8,8,8,8,8,8,8,8,8,1,7,
  1,1,8,8,8,8,8,8,8,8,8,1,7,1,1,8,8,8,8,8,8,8,8,8,1,7,1,1,8,8,8,8,8,8,8,8,8,
  1,7,1,1,8,8,8,8,8,8,8,8,8,1,7,1,1,5,2,2,2,2,2,2,2,2,2,2,2,5,2,5,5,2,2,2,2,
  2,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,2,5,5,5,5,5,1,
  7,1,3,2,5,5,5,5,5,5,1,7,1,3,4,4,4,4,4,4,4,4,4,4,4,4,4,4,4,4,4,4,4,4,4,4,4,
  4,4,4,4,4,4,4,4,4,4,4,4,4,4,4,4,4,4,4,4,4,4,4,4,4,4,4,4,4,4,4,4,4,4,4,1,4,
  4,4,4,4,4,4,4,4,4,7,1,4,4,4,4,4,1,7,1,1,5,1,2,2,2,2,2,2,2,2,2,2,2,1,2,2,2,
  2,2,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,2,2,7,2,1,1,2,1,2,1,1,1,1,
  1,1,1,1,1,1,2,2,1,1,1,2,1,1,1,1,1,1,2,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,
  1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,7,2,
  2,1,2,2,1,1,1,2,2,1,1,1,1,2,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,2,2,1,2,2,
  1,1,2,8,8,8,8,8,1,7,1,1,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,
  8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,1,7,1,1,8,8,8,8,8,1,7,1,1,2,7,1,1,1,2,7,1,1,
  1,2,2,2,2,2,2,2,2,2,2,2,1,2,2,2,2,2,1,1,2,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,
  1,1,1,1,2,2,7,2,1,1,2,1,1,1,2,1,1,1,1,1,1,1,1,1,1,2,1,1,1,2,1,1,1,1,1,2,1,
  1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,2,1,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,
  2,3,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,7,2,5,5,5,5,5,5,5,5,5,7,1,3,8,8,
  8,8,8,1,7,1,1,1,1,1,1,2,7,2,2,1,2,1,1,1,1,1,1,1,2,7,2,2,1,2,1,1,1,8,8,8,8,
  8,1,7,1,1,2,2,2,2,2,7,2,2,2,2,7,2,2,7,4,1,4,4,4,4,4,4,4,4,4,4,4,4,4,4,4,4,
  4,4,4,4,4,4,4,4,4,4,4,4,4,4,4,4,4,4,4,4,4,4,4,4,4,4,4,4,4,4,4,4,4,4,4,4,7,
  2,2,2,2,2,7,2,1,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,
  2,2,2,2,2,2,2,2,2,2,2,7,1,10,10,10,10,10,5,2,10,10,10,10,10,10,10,10,10,7,
  10,10,10,10,4,4,4,4,4,4,4,4,4,4,4,4,4,4,4,4,4,4,4,4,4,4,4,4,4,4,4,4,4,4,4,
  4,4,4,4,4,4,4,4,7,10,10,4,4,4,4,4,4,4,4,4,4,4,4,4,4,4,4,4,4,4,4,4,4,4,4,4,
  4,4,4,4,4,4,4,4,4,4,4,4,4,4,7,10,10,10,10,10,10,10,10,10,4,4,4,4,4,4,4,4,4,
  4,4,4,4,4,4,4,4,4,4,4,4,4,4,4,4,4,4,4,4,4,4,4,4,4,4,4,4,4,4,7,9,2,2,1,1,2,
  10,10,10,10,10,4,4,4,4,4,4,4,4,4,4,4,4,4,4,4,4,4,4,4,4,4,4,4,4,4,4,4,4,4,4,
  4,2,4,4,4,4,2,2,4,4,7,2,5,5,7,1,3,5,5,5,5,5,5,5,7,1,3,5,5,7,1,3,5,5,7,1,3,
  5,5,7,1,3,5,5,7,1,3,5,5,7,1,3,5,5,7,1,3,5,5,7,1,3,5,5,7,1,3,5,5,7,1,3,5,5,
  5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,
  7,1,3,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,
  5,5,5,5,1,7,1,3,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,
  5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,7,1,3,8,
  1,7,1,1,8,1,7,1,1,8,1,7,1,1,8,1,7,1,1,8,1,7,1,1,8,1,7,1,1,8,1,7,1,1,8,1,7,
  1,1,8,1,7,1,1,8,1,7,1,1,8,1,7,1,1,2,2,2,2,2,2,2,2,2,5,5,5,5,5,5,5,5,5,5,5,
  5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,7,1,3,5,5,5,5,5,5,5,5,
  5,5,5,5,5,2,2,2,2,2,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,1,7,1,3,5,5,5,
  5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,
  5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,7,1,3,5,5,5,5,5,5,5,5,5,5,5,5,5,
  5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,1,7,1,3,5,5,5,5,5,5,5,5,
  5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,
  5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,7,1,3,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,
  8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,1,7,1,1,2,2,2,2,2,2,2,2,2,2,
  2,1,2,2,2,2,2,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,2,2,7,2,1,1,2,1,2,1,1,
  2,1,1,1,2,2,2,2,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,2,2,2,2,2,2,2,2,2,2,2,1,2,2,
  2,2,2,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,2,2,7,2,1,1,2,1,2,1,1,2,1,1,1,
  2,2,2,2,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,2,2,2,2,2,2,2,2,2,2,2,1,2,2,2,2,2,1,
  1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,2,2,7,2,1,1,2,1,2,1,1,2,1,1,1,2,2,2,2,
  1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,2,2,2,2,2,2,2,2,2,2,2,1,2,2,2,2,2,1,1,1,1,1,
  1,1,1,1,1,1,1,1,1,1,1,1,1,1,2,2,7,2,1,1,2,1,2,1,1,2,1,1,1,2,2,2,2,1,1,1,1,
  1,1,1,1,1,1,1,1,1,1,1,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,
  5,5,5,5,5,5,5,5,5,5,5,5,1,7,1,3,5,1,1,1,1,1,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,
  5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,1,7,1,3,1,1,1,1,1,5,1,1,5,5,5,5,5,5,5,5,5,5,
  5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,1,7,1,3,1,1,5,1,1,1,1,5,5,5,5,5,5,
  5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,1,7,1,3,1,1,1,1,5,1,1,5,5,5,5,5,5,
  5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,1,7,1,3,1,1,5,1,1,5,5,5,5,5,5,5,5,5,5,
  5,5,5,5,5,5,5,5,5,5,5,5,1,7,1,3,1,1,1,1,5,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,4,
  4,4,4,4,4,7,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,1,
  2,2,2,2,2,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,2,2,7,2,1,1,2,1,2,1,
  1,1,1,1,1,1,1,1,2,2,1,1,1,2,1,1,1,1,1,2,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,8,
  8,8,8,8,1,7,1,1,8,8,8,8,8,1,7,1,1,8,8,8,8,8,1,7,1,5,5,5,5,5,5,7,1,3,8,8,8,
  8,8,1,7,1,1,5,5,5,5,5,5,7,1,3,8,8,8,8,8,1,7,1,1,5,5,5,5,5,5,5,5,5,5,5,5,5,
  5,7,1,3,8,8,8,8,8,8,8,8,8,8,8,8,8,1,7,1,1,5,5,5,5,5,5,7,1,3,8,8,8,8,8,1,7,
  1,1,5,5,5,5,5,5,7,1,3,8,8,8,8,8,1,7,1,1,10,10,1,10,10,10,10,10,10,10,10,10,
  10,10,10,2,10,10,10,10,10,10,10,10,10,10,10,10,10,10,10,10,10,10,10,10,10,
  10,10,10,10,7,5,5,5,5,5,5,7,1,3,8,8,8,8,8,1,7,1,1,8,8,8,8,8,1,7,1,1,5,5,5,
  5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,1,7,1,3,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,
  2,2,2,2,2,2,7,1,1,1,1,1,1,1,1,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,
  5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,7,1,3,5,5,5,5,5,5,5,5,
  5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,
  5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,7,1,3,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,
  5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,7,1,3,5,5,5,
  5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,
  5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,7,1,3,5,5,5,5,5,5,5,5,5,5,5,5,
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
  5,5,5,5,5,5,5,7,1,3,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,
  5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,7,1,3,5,5,5,5,5,5,5,5,5,5,5,5,5,
  5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,7,1,
  3,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,
  5,5,5,5,5,5,5,5,5,5,5,5,7,1,3,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,
  5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,7,1,3,5,5,5,5,5,5,5,5,
  5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,
  5,5,5,7,1,3,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,
  5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,7,1,3,5,5,5,5,
  5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,
  5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,7,1,3,5,5,5,5,5,5,5,5,5,5,5,5,5,
  5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,
  5,5,5,5,5,5,5,5,5,5,5,5,7,1,3,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,
  5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,
  5,5,5,7,1,3,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,
  5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,7,1,3,5,5,5,5,
  5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,
  5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,7,1,3,5,5,5,
  5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,
  5,5,5,5,5,5,5,5,7,1,3,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,
  5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,7,
  1,3,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,
  5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,7,1,3,5,5,5,5,5,5,5,5,
  5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,
  5,5,5,7,1,3,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,
  5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,7,1,3,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,
  5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,7,1,3,5,5,5,
  5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,
  5,5,5,5,5,5,5,5,5,5,5,5,5,5,7,1,3,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,
  1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,9,5,3,3,3,3,3,3,3,3,3,3,3,3,3,3,
  3,3,3,3,3,3,3,3,3,3,3,3,3,3,3,3,3,3,3,3,3,3,3,3,3,3,3,1,7,3,1,7,3,5,5,5,5,
  5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,
  1,7,1,3,2,2,2,2,2,2,2,2,2,2,2,1,2,2,2,2,2,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,
  1,1,1,1,1,1,2,2,7,2,1,1,2,1,2,1,1,1,1,1,1,1,1,1,1,2,1,1,1,2,1,1,1,1,1,2,1,
  1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,
  1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,7,1,1,2,2,1,2,2,1,1,1,2,2,1,1,1,1,2,1,1,
  1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,2,2,1,2,2,1,1,2,8,8,8,8,8,1,7,1,1,1,1,1,1,
  1,1,3,7,1,3,3,1,3,1,1,1,2,2,2,1,1,1,2,2,2,2,2,7,2,2,2,2,3,2,2,2,2,2,2,2,2,
  2,2,2,7,3,2,1,2,2,2,2,2,2,2,7,3,2,2,2,1,2,2,2,2,7,2,1,7,1,1,7,1,1,7,1,2,2,
  2,2,2,2,2,2,2,2,2,1,2,2,2,2,2,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,
  2,2,7,2,1,1,2,1,2,1,1,1,1,1,1,1,1,1,2,2,1,1,1,2,1,1,1,1,1,2,1,1,1,1,1,1,1,
  1,1,1,1,1,1,1,1,1,2,2,2,2,2,2,2,2,2,2,2,1,2,2,2,2,2,1,1,1,1,1,1,1,1,1,1,1,
  1,1,1,1,1,1,1,1,1,1,1,2,2,7,2,1,1,2,1,2,1,1,1,1,1,1,1,1,1,2,2,1,1,1,2,1,1,
  1,1,1,2,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,2,2,2,2,2,2,2,2,2,2,2,1,2,2,2,2,2,
  1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,2,2,7,2,1,1,2,1,2,1,1,1,1,1,1,
  1,1,1,2,2,1,1,1,2,1,1,1,1,1,2,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,2,2,2,2,2,2,
  2,2,2,2,2,1,2,2,2,2,2,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,2,2,7,2,
  1,1,2,1,2,1,1,1,1,1,1,1,1,1,2,2,1,1,1,2,1,1,1,1,1,2,1,1,1,1,1,1,1,1,1,1,1,
  1,1,1,1,1,2,2,2,2,2,2,2,2,2,2,2,1,2,2,2,2,2,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,
  1,1,1,1,1,1,1,2,2,7,2,1,1,2,1,2,1,1,1,1,1,1,1,1,1,2,2,1,1,1,2,1,1,1,1,1,2,
  1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,2,2,2,2,2,2,2,2,2,2,2,1,2,2,2,2,2,1,1,1,1,
  1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,2,2,7,2,1,1,2,1,2,1,1,1,1,1,1,1,1,1,2,
  2,1,1,1,2,1,1,1,1,1,2,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,2,2,2,2,2,2,2,2,2,2,
  2,1,2,2,2,2,2,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,2,2,7,2,1,1,2,1,
  2,1,1,1,1,1,1,1,1,1,2,2,1,1,1,2,1,1,1,1,1,2,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,
  1,2,2,2,2,2,2,2,2,2,2,2,1,2,2,2,2,2,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,
  1,1,1,2,2,7,2,1,1,2,1,2,1,1,1,1,1,1,1,1,1,2,2,1,1,1,2,1,1,1,1,1,2,1,1,1,1,
  1,1,1,1,1,1,1,1,1,1,1,1,2,2,2,2,2,2,2,2,2,2,2,1,2,2,2,2,2,1,1,1,1,1,1,1,1,
  1,1,1,1,1,1,1,1,1,1,1,1,1,1,2,2,7,2,1,1,2,1,2,1,1,1,1,1,1,1,1,1,2,2,1,1,1,
  2,1,1,1,1,1,2,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,2,2,2,2,2,2,2,2,2,2,2,1,2,2,
  2,2,2,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,2,2,7,2,1,1,2,1,2,1,1,1,
  1,1,1,1,1,1,2,2,1,1,1,2,1,1,1,1,1,2,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,2,2,2,
  2,2,2,2,2,2,2,2,1,2,2,2,2,2,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,2,
  2,7,2,1,1,2,1,2,1,1,1,1,1,1,1,1,1,2,2,1,1,1,2,1,1,1,1,1,2,1,1,1,1,1,1,1,1,
  1,1,1,1,1,1,1,1,2,2,2,2,2,2,2,2,2,2,2,1,2,2,2,2,2,1,1,1,1,1,1,1,1,1,1,1,1,
  1,1,1,1,1,1,1,1,1,1,2,2,7,2,1,1,2,1,2,1,1,1,1,1,1,1,1,1,2,2,1,1,1,2,1,1,1,
  1,1,2,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,2,2,2,2,2,2,2,2,2,2,2,1,2,2,2,2,2,1,
  1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,2,2,7,2,1,1,2,1,2,1,1,1,1,1,1,1,
  1,1,2,2,1,1,1,2,1,1,1,1,1,2,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,2,2,2,2,2,2,2,
  2,2,2,2,1,2,2,2,2,2,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,2,2,7,2,1,
  1,2,1,2,1,1,1,1,1,1,1,1,1,2,2,1,1,1,2,1,1,1,1,1,2,1,1,1,1,1,1,1,1,1,1,1,1,
  1,1,1,1,2,2,2,2,2,2,2,2,2,2,2,1,2,2,2,2,2,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,
  1,1,1,1,1,1,2,2,7,2,1,1,2,1,2,1,1,1,1,1,1,1,1,1,2,2,1,1,1,2,1,1,1,1,1,2,1,
  1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,2,2,2,2,2,2,2,2,2,2,2,1,2,2,2,2,2,1,1,1,1,1,
  1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,2,2,7,2,1,1,2,1,2,1,1,1,1,1,1,1,1,1,2,2,
  1,1,1,2,1,1,1,1,1,2,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,2,2,2,2,2,2,2,2,2,2,2,
  1,2,2,2,2,2,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,2,2,7,2,1,1,2,1,2,
  1,1,1,1,1,1,1,1,1,2,2,1,1,1,2,1,1,1,1,1,2,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,
  2,2,2,2,2,2,2,2,2,2,2,1,2,2,2,2,2,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,
  1,1,2,2,7,2,1,1,2,1,2,1,1,1,1,1,1,1,1,1,2,2,1,1,1,2,1,1,1,1,1,2,1,1,1,1,1,
  1,1,1,1,1,1,1,1,1,1,1,2,2,2,2,2,2,2,2,2,2,2,1,2,2,2,2,2,1,1,1,1,1,1,1,1,1,
  1,1,1,1,1,1,1,1,1,1,1,1,1,2,2,7,2,1,1,2,1,2,1,1,1,1,1,1,1,1,1,2,2,1,1,1,2,
  1,1,1,1,1,2,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,2,2,2,2,2,2,2,2,2,2,2,1,2,2,2,
  2,2,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,2,2,7,2,1,1,2,1,2,1,1,1,1,
  1,1,1,1,1,2,2,1,1,1,2,1,1,1,1,1,2,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,2,2,2,2,
  2,2,2,2,2,2,2,1,2,2,2,2,2,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,2,2,
  7,2,1,1,2,1,2,1,1,1,1,1,1,1,1,1,2,2,1,1,1,2,1,1,1,1,1,2,1,1,1,1,1,1,1,1,1,
  1,1,1,1,1,1,1,2,2,2,2,2,2,2,2,2,2,2,1,2,2,2,2,2,1,1,1,1,1,1,1,1,1,1,1,1,1,
  1,1,1,1,1,1,1,1,1,2,2,7,2,1,1,2,1,2,1,1,1,1,1,1,1,1,1,2,2,1,1,1,2,1,1,1,1,
  1,2,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,2,2,2,2,2,2,2,2,2,2,2,1,2,2,2,2,2,1,1,
  1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,2,2,7,2,1,1,2,1,2,1,1,1,1,1,1,1,1,
  1,2,2,1,1,1,2,1,1,1,1,1,2,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,2,2,2,2,2,2,2,2,
  2,2,2,1,2,2,2,2,2,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,2,2,7,2,1,1,
  2,1,2,1,1,1,1,1,1,1,1,1,2,2,1,1,1,2,1,1,1,1,1,2,1,1,1,1,1,1,1,1,1,1,1,1,1,
  1,1,1,2,2,2,2,2,2,2,2,2,2,2,1,2,2,2,2,2,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,
  1,1,1,1,1,2,2,7,2,1,1,2,1,2,1,1,1,1,1,1,1,1,1,2,2,1,1,1,2,1,1,1,1,1,2,1,1,
  1,1,1,1,1,1,1,1,1,1,1,1,1,1,2,2,2,2,2,2,2,2,2,2,2,1,2,2,2,2,2,1,1,1,1,1,1,
  1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,2,2,7,2,1,1,2,1,2,1,1,1,1,1,1,1,1,1,2,2,1,
  1,1,2,1,1,1,1,1,2,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,2,2,2,2,2,2,2,2,2,2,2,1,
  2,2,2,2,2,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,2,2,7,2,1,1,2,1,2,1,
  1,1,1,1,1,1,1,1,2,2,1,1,1,2,1,1,1,1,1,2,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,2,
  2,2,2,2,2,2,2,2,2,2,1,2,2,2,2,2,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,
  1,2,2,7,2,1,1,2,1,2,1,1,1,1,1,1,1,1,1,2,2,1,1,1,2,1,1,1,1,1,2,1,1,1,1,1,1,
  1,1,1,1,1,1,1,1,1,1,2,2,2,2,2,2,2,2,2,2,2,1,2,2,2,2,2,1,1,1,1,1,1,1,1,1,1,
  1,1,1,1,1,1,1,1,1,1,1,1,2,2,7,2,1,1,2,1,2,1,1,1,1,1,1,1,1,1,2,2,1,1,1,2,1,
  1,1,1,1,2,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,2,2,2,2,2,2,2,2,2,2,2,1,2,2,2,2,
  2,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,2,2,7,2,1,1,2,1,2,1,1,1,1,1,
  1,1,1,1,2,2,1,1,1,2,1,1,1,1,1,2,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,2,2,2,2,2,
  2,2,2,2,2,2,1,2,2,2,2,2,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,2,2,7,
  2,1,1,2,1,2,1,1,1,1,1,1,1,1,1,2,2,1,1,1,2,1,1,1,1,1,2,1,1,1,1,1,1,1,1,1,1,
  1,1,1,1,1,1,2,2,2,2,2,2,2,2,2,2,2,1,2,2,2,2,2,1,1,1,1,1,1,1,1,1,1,1,1,1,1,
  1,1,1,1,1,1,1,1,2,2,7,2,1,1,2,1,2,1,1,1,1,1,1,1,1,1,2,2,1,1,1,2,1,1,1,1,1,
  2,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,2,2,2,2,2,2,2,2,2,2,2,1,2,2,2,2,2,1,1,1,
  1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,2,2,7,2,1,1,2,1,2,1,1,1,1,1,1,1,1,1,
  2,2,1,1,1,2,1,1,1,1,1,2,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,2,2,2,2,2,2,2,2,2,
  2,2,1,2,2,2,2,2,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,2,2,7,2,1,1,2,
  1,2,1,1,1,1,1,1,1,1,1,2,2,1,1,1,2,1,1,1,1,1,2,1,1,1,1,1,1,1,1,1,1,1,1,1,1,
  1,1,2,2,2,2,2,2,2,2,2,2,2,1,2,2,2,2,2,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,
  1,1,1,1,2,2,7,2,1,1,2,1,2,1,1,1,1,1,1,1,1,1,2,2,1,1,1,2,1,1,1,1,1,2,1,1,1,
  1,1,1,1,1,1,1,1,1,1,1,1,1,2,2,2,2,2,2,2,2,2,2,2,1,2,2,2,2,2,1,1,1,1,1,1,1,
  1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,2,2,7,2,1,1,2,1,2,1,1,1,1,1,1,1,1,1,2,2,1,1,
  1,2,1,1,1,1,1,2,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,2,2,2,2,2,2,2,2,2,2,2,1,2,
  2,2,2,2,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,2,2,7,2,1,1,2,1,2,1,1,
  1,1,1,1,1,1,1,2,2,1,1,1,2,1,1,1,1,1,2,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,2,2,
  2,2,2,2,2,2,2,2,2,1,2,2,2,2,2,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,
  2,2,7,2,1,1,2,1,2,1,1,1,1,1,1,1,1,1,2,2,1,1,1,2,1,1,1,1,1,2,1,1,1,1,1,1,1,
  1,1,1,1,1,1,1,1,1,2,2,2,2,2,2,2,2,2,2,2,1,2,2,2,2,2,1,1,1,1,1,1,1,1,1,1,1,
  1,1,1,1,1,1,1,1,1,1,1,2,2,7,2,1,1,2,1,2,1,1,1,1,1,1,1,1,1,2,2,1,1,1,2,1,1,
  1,1,1,2,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,2,2,2,2,2,2,2,2,2,2,2,1,2,2,2,2,2,
  1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,2,2,7,2,1,1,2,1,2,1,1,1,1,1,1,
  1,1,1,2,2,1,1,1,2,1,1,1,1,1,2,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,2,2,2,2,2,2,
  2,2,2,2,2,1,2,2,2,2,2,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,2,2,7,2,
  1,1,2,1,2,1,1,1,1,1,1,1,1,1,2,2,1,1,1,2,1,1,1,1,1,2,1,1,1,1,1,1,1,1,1,1,1,
  1,1,1,1,1,2,2,2,2,2,2,2,2,2,2,2,1,2,2,2,2,2,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,
  1,1,1,1,1,1,1,2,2,7,2,1,1,2,1,2,1,1,1,1,1,1,1,1,1,2,2,1,1,1,2,1,1,1,1,1,2,
  1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,2,2,2,2,2,2,2,2,2,2,2,1,2,2,2,2,2,1,1,1,1,
  1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,2,2,7,2,1,1,2,1,2,1,1,1,1,1,1,1,1,1,2,
  2,1,1,1,2,1,1,1,1,1,2,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,2,2,2,2,2,2,2,2,2,2,
  2,1,2,2,2,2,2,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,2,2,7,2,1,1,2,1,
  2,1,1,1,1,1,1,1,1,1,2,2,1,1,1,2,1,1,1,1,1,2,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,
  1,2,2,2,2,2,2,2,2,2,2,2,1,2,2,2,2,2,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,
  1,1,1,2,2,7,2,1,1,2,1,2,1,1,1,1,1,1,1,1,1,2,2,1,1,1,2,1,1,1,1,1,2,1,1,1,1,
  1,1,1,1,1,1,1,1,1,1,1,1,2,2,2,2,2,2,2,2,2,2,2,1,2,2,2,2,2,1,1,1,1,1,1,1,1,
  1,1,1,1,1,1,1,1,1,1,1,1,1,1,2,2,7,2,1,1,2,1,2,1,1,1,1,1,1,1,1,1,2,2,1,1,1,
  2,1,1,1,1,1,2,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,2,2,2,2,2,2,2,2,2,2,2,1,2,2,
  2,2,2,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,2,2,7,2,1,1,2,1,2,1,1,1,
  1,1,1,1,1,1,2,2,1,1,1,2,1,1,1,1,1,2,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,2,2,2,
  2,2,2,2,2,2,2,2,1,2,2,2,2,2,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,2,
  2,7,2,1,1,2,1,2,1,1,1,1,1,1,1,1,1,2,2,1,1,1,2,1,1,1,1,1,2,1,1,1,1,1,1,1,1,
  1,1,1,1,1,1,1,1,2,2,2,2,2,2,2,2,2,2,2,1,2,2,2,2,2,1,1,1,1,1,1,1,1,1,1,1,1,
  1,1,1,1,1,1,1,1,1,1,2,2,7,2,1,1,2,1,2,1,1,1,1,1,1,1,1,1,2,2,1,1,1,2,1,1,1,
  1,1,2,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,2,2,2,2,2,2,2,2,2,2,2,1,2,2,2,2,2,1,
  1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,2,2,7,2,1,1,2,1,2,1,1,1,1,1,1,1,
  1,1,2,2,1,1,1,2,1,1,1,1,1,2,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,2,2,2,2,2,2,2,
  2,2,2,2,1,2,2,2,2,2,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,2,2,7,2,1,
  1,2,1,2,1,1,1,1,1,1,1,1,1,2,2,1,1,1,2,1,1,1,1,1,2,1,1,1,1,1,1,1,1,1,1,1,1,
  1,1,1,1,2,2,2,2,2,2,2,2,2,2,2,1,2,2,2,2,2,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,
  1,1,1,1,1,1,2,2,7,2,1,1,2,1,2,1,1,1,1,1,1,1,1,1,2,2,1,1,1,2,1,1,1,1,1,2,1,
  1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,2,1,1,2,2,2,2,2,7,2,2,2,2,2,2,2,1,1,2,2,2,2,
  2,7,2,2,2,2,2,2,2,1,1,2,2,2,2,2,7,2,2,2,2,2,2,2,1,1,2,2,2,2,2,7,2,2,2,2,2,
  2,2,2,2,2,7,2,2,2,2,2,2,2,7,2,2,2,2,2,2,2,2,2,2,7,2,2,2,2,1,2,2,2,2,2,2,2,
  7,2,2,2,2,1,2,2,2,2,2,2,2,7,2,2,2,2,1,2,1,1,1,2,2,2,2,2,7,2,2,2,2,2,2,2,2,
  1,1,1,2,2,2,2,2,7,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,7,2,2,1,2,2,2,2,2,2,2,2,
  2,7,2,2,1,2,2,2,2,2,2,2,2,2,7,2,2,1,2,2,2,2,2,2,2,2,2,7,2,2,1,2,2,2,2,2,2,
  2,2,2,7,1,2,1,2,2,2,2,2,2,2,2,2,7,2,2,1,2,2,2,2,2,2,2,2,2,7,2,2,1,2,2,2,2,
  2,2,2,2,2,7,2,2,1,2,2,2,2,2,2,2,2,2,7,2,2,1,2,2,2,2,2,2,2,2,2,7,2,2,1,2,2,
  2,2,2,2,2,2,2,7,2,2,1,2,2,2,2,2,2,2,2,2,2,2,1,2,2,2,2,2,1,1,2,1,1,1,1,1,1,
  1,1,1,1,1,1,1,1,1,1,1,1,1,1,2,2,7,2,1,1,2,1,2,1,2,1,1,2,1,1,1,1,1,1,1,1,2,
  2,1,1,1,1,2,1,1,1,1,1,2,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,2,2,2,2,1,1,
  1,1,1,1,2,9,7,2,1,1,2,1,1,1,1,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,
  5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,
  5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,
  5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,
  5,7,1,3,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,
  8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,
  8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,
  8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,1,7,1,1,5,5,5,5,5,5,5,
  5,5,5,5,5,5,1,7,1,3,2,2,2,2,2,2,2,2,2,2,2,2,2,7,2,1,1,5,5,5,5,5,5,5,5,5,5,
  5,5,5,5,5,5,5,5,5,1,7,1,3,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,7,2,1,5,5,
  5,5,5,5,7,1,3,5,5,5,5,5,5,7,1,3,5,5,1,7,1,3,1,2,7,2,1,1,2,1,1,5,5,5,5,5,5,
  7,1,3,5,5,5,5,5,5,7,1,3,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,7,1,3,2,2,2,
  2,2,2,2,2,2,2,2,2,4,4,4,4,2,4,4,7,2,1,1,2,7,1,1,1,2,7,2,1,1,5,5,5,5,5,5,5,
  5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,7,
  1,3,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,
  8,8,8,8,8,8,8,1,7,1,1,2,2,2,2,2,2,2,2,2,2,2,1,2,2,2,2,2,1,1,2,1,1,1,1,1,1,
  1,1,1,1,1,1,1,1,1,1,1,1,1,1,2,2,1,7,2,1,1,2,1,2,1,2,1,1,1,1,1,1,1,1,1,1,1,
  2,2,1,1,1,2,1,1,1,1,1,2,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,5,5,5,5,5,5,5,5,
  5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,1,7,1,3,
  2,2,2,2,2,2,2,2,2,2,2,1,2,2,2,2,2,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,
  1,1,2,2,7,2,1,1,2,1,2,1,1,1,1,1,1,1,1,1,2,2,1,1,1,2,1,1,1,1,1,2,1,1,1,1,1,
  1,1,1,1,1,1,1,1,1,1,1,5,5,5,5,5,5,5,5,5,5,5,5,5,1,7,1,3,2,2,2,2,2,2,2,2,2,
  2,2,2,2,7,2,1,1,5,5,5,5,5,5,5,5,5,5,5,5,5,1,7,1,3,2,2,2,2,2,2,2,2,2,2,2,2,
  2,7,2,1,1,5,5,5,5,5,5,5,5,5,5,5,5,5,1,7,1,3,2,2,2,2,2,2,2,2,2,2,2,2,2,7,2,
  1,1,1,5,5,5,5,5,5,5,5,5,5,5,5,5,1,7,1,3,2,2,2,2,2,2,2,2,2,2,2,2,2,7,2,1,1,
  1,5,5,5,5,5,5,5,5,5,5,5,5,5,1,7,1,3,2,2,2,2,2,2,2,2,2,2,2,2,2,7,2,1,1,1,5,
  5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,
  5,5,5,5,1,7,1,3,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,
  5,5,5,5,5,5,5,5,5,5,5,5,5,1,7,1,3,2,2,2,2,2,2,2,2,2,2,2,1,2,2,2,2,2,1,1,2,
  1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,2,2,7,2,1,1,2,1,2,1,2,1,1,2,1,1,1,
  1,1,1,1,1,2,2,1,1,1,1,2,1,1,1,1,1,2,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,5,
  5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,
  5,5,5,5,1,7,1,3,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,
  5,5,5,5,5,5,5,5,5,5,5,5,5,1,7,1,3,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,
  5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,1,7,1,3,2,2,2,2,2,2,2,2,2,2,2,
  1,2,2,2,2,2,1,1,2,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,2,2,7,2,1,1,2,1,
  2,1,2,1,1,2,1,1,1,1,1,1,1,1,2,2,1,1,1,1,2,1,1,1,1,1,2,1,1,1,1,1,1,1,1,1,1,
  1,1,1,1,1,1,1,1,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,
  5,5,5,5,5,5,5,5,5,5,5,5,1,7,1,3,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,
  5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,1,7,1,3,2,2,2,2,2,2,2,2,2,2,2,1,2,
  2,2,2,2,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,2,2,7,2,1,1,2,1,2,1,1,
  1,1,1,1,1,1,1,2,2,1,1,1,2,1,1,1,1,1,2,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,5,5,
  5,5,5,5,7,1,3,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,
  5,5,5,5,5,5,5,5,5,5,5,1,7,1,3,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,
  5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,1,7,1,3,5,5,5,5,5,5,5,5,5,5,5,5,5,5,
  5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,1,7,1,3,2,2,2,2,2,2,
  2,2,2,2,2,1,2,2,2,2,2,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,2,2,7,2,
  1,1,2,1,2,1,1,1,1,1,1,1,1,1,2,2,1,1,1,2,1,1,1,1,1,2,1,1,1,1,1,1,1,1,1,1,1,
  1,1,1,1,1,2,2,2,2,2,2,2,2,2,2,2,2,2,7,2,1,1,5,5,5,5,5,5,5,5,7,1,3,5,5,5,5,
  5,5,5,5,7,1,3,1,1,1,1,3,7,3,3,1,3,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,
  1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,7,2,2,2,1,2,2,2,2,2,2,2,2,2,
  2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,1,1,1,1,2,7,2,2,1,
  2,1,1,1,1,7,1,8,8,8,8,8,1,7,1,1,8,8,8,8,8,1,7,1,1,8,8,8,8,8,1,7,1,1,2,1,2,
  2,2,2,2,1,1,1,2,1,3,7,1,1,1,1,1,2,7,2,2,1,2,1,1,1,1,1,1,1,2,7,2,2,1,2,1,1,
  1,2,2,2,2,2,2,2,2,2,7,2,2,2,2,2,2,7,2,1,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,
  2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,7,1,4,4,4,4,4,4,4,4,4,4,4,
  4,4,4,4,4,4,4,4,4,4,4,4,4,4,4,4,4,4,4,4,4,4,4,4,4,4,4,4,4,4,4,4,4,4,4,4,4,
  4,7,4,4,4,4,4,4,4,4,4,4,4,4,4,4,4,4,4,4,4,4,4,4,4,4,4,4,4,4,4,4,4,4,4,4,4,
  4,4,4,4,4,4,4,4,4,4,4,4,4,4,7,1,7,1,1,7,1,1,7,1,1,7,1,1,7,1,1,7,1,1,7,1,1,
  7,1,1,7,1,1,7,1,1,7,1,2,2,2,2,2,2,2,2,2,2,2,1,2,2,2,2,2,1,1,1,1,1,1,1,1,1,
  1,1,1,1,1,1,1,1,1,1,1,1,1,2,2,7,2,1,1,2,1,2,1,1,1,1,1,1,1,1,1,1,2,1,1,1,2,
  1,1,1,1,1,2,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,2,2,2,2,2,7,5,5,5,5,5,5,5,5,5,
  5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,7,1,3,8,8,8,8,
  8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,1,7,1,
  1,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,
  5,5,5,7,1,3,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,
  8,8,8,8,8,8,8,1,7,1,1,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,
  8,8,8,8,8,8,8,8,8,8,8,8,1,7,1,1,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,
  8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,1,7,1,1,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,
  8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,1,7,1,1,8,8,8,8,8,8,8,8,8,8,8,
  8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,1,7,1,1,8,8,8,
  8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,
  8,1,7,1,1,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,
  5,5,5,5,5,5,5,5,5,5,7,1,3,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,
  8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,1,7,1,1,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,
  5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,7,1,3,8,8,8,8,8,8,8,8,
  8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,1,7,1,1,
  5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,
  5,5,5,5,5,7,1,3,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,
  8,8,8,8,8,8,8,8,8,8,8,8,1,7,1,1,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,
  5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,7,1,3,8,8,8,8,8,8,8,8,8,8,8,8,8,
  8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,1,7,1,1,5,5,5,5,5,
  5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,
  7,1,3,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,
  8,8,8,8,8,8,8,1,7,1,1,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,
  8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,1,7,1,1,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,
  5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,7,1,3,8,8,8,8,8,8,8,8,8,8,
  8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,1,7,1,1,8,8,
  8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,
  8,8,1,7,1,1,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,
  5,5,5,5,5,5,5,5,5,5,5,7,1,3,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,
  8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,1,7,1,1,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,
  8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,1,7,1,1,5,5,5,5,5,5,5,
  5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,1,7,1,
  3,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,
  5,5,5,5,5,1,7,1,3,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,
  5,5,5,5,5,5,5,5,5,5,5,5,5,1,7,1,3,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,
  5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,1,7,1,3,5,5,5,5,5,5,5,5,5,5,5,5,
  5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,1,7,1,3,5,5,5,5,
  5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,
  1,7,1,3,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,
  5,5,5,5,5,5,5,5,1,7,1,3,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,
  5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,1,7,1,3,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,
  5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,1,7,1,3,5,5,5,5,5,5,5,5,5,
  5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,1,7,1,3,5,
  5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,
  5,5,5,1,7,1,3,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,
  5,5,5,5,5,5,5,5,5,5,5,1,7,1,3,1,1,1,1,2,7,2,2,1,2,1,1,1,1,1,1,1,2,7,2,2,1,
  2,1,1,1,1,1,1,1,2,7,2,2,1,2,1,1,1,1,1,1,1,2,7,2,2,1,2,1,1,1,1,1,1,1,2,7,2,
  2,1,2,1,1,1,2,2,2,2,2,2,2,2,2,2,2,2,2,7,1,1,1,1,1,1,1,2,7,2,2,1,2,1,1,1,1,
  1,1,1,2,7,2,2,1,2,1,1,1,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,
  2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,3,2,7,1,1,1,1,2,7,2,2,1,2,1,1,1,1,1,1,1,2,
  7,2,2,1,2,1,1,1,10,10,10,10,10,10,10,10,10,10,10,10,10,10,10,10,10,10,10,5,
  5,5,5,7,1,3,8,8,8,8,8,1,7,1,1,8,8,8,8,8,1,7,1,1,8,8,8,8,8,1,7,1,1,8,8,8,8,
  8,1,7,1,1,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,
  5,5,5,5,5,5,5,5,5,1,7,1,3,2,2,2,2,2,2,2,2,2,2,2,1,2,2,2,2,2,1,1,1,1,1,1,1,
  1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,2,2,7,2,1,1,2,1,2,1,1,1,1,1,1,1,1,1,1,2,1,1,
  1,2,1,1,1,1,1,2,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,5,5,5,5,5,5,5,5,5,5,5,5,5,
  5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,1,7,1,3,2,2,2,2,2,
  2,2,2,2,2,2,1,2,2,2,2,2,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,2,2,7,
  2,1,1,2,1,2,1,1,1,1,1,1,1,1,1,1,2,1,1,1,2,1,1,1,1,1,2,1,1,1,1,1,1,1,1,1,1,
  1,1,1,1,1,1,1,1,1,1,3,7,3,3,1,3,1,1,1,5,5,5,5,5,5,7,1,3,8,8,8,8,8,1,7,1,1,
  5,5,5,5,5,5,5,5,7,1,3,5,5,5,5,5,5,5,5,7,1,3,5,5,5,5,5,5,5,5,7,1,3,5,5,5,5,
  5,5,5,5,5,7,1,3,5,5,5,5,5,5,5,5,7,1,3,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,
  8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,1,7,1,1,8,8,8,8,8,8,8,8,8,8,
  8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,1,7,1,1,8,8,
  8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,
  8,8,1,7,1,1,5,5,5,5,5,5,5,7,1,3,5,5,5,5,5,5,5,7,1,3,8,1,7,1,1,2,1,2,2,2,2,
  2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,7,
  1,2,1,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,3,2,2,2,2,2,2,2,2,2,2,2,2,2,2,
  2,2,2,2,2,2,2,7,2,5,5,5,5,5,5,5,5,7,1,3,1,1,7,2,1,2,2,2,2,2,2,2,2,2,7,9,2,
  2,1,1,2,10,10,10,10,10,4,4,4,4,2,2,4,4,7,2,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,
  1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,
  1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,
  1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,
  1,1,1,1,1,1,7,1,1,1,1,1,1,1,1,1,1,1,1,1,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,
  2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,1,1,1,1,1,1,
  1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,
  1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,
  1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,2,1,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,
  2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,7,1,1,7,1,2,2,2,2,2,2,2,2,2,2,2,
  1,2,2,2,2,2,1,1,2,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,2,2,7,2,1,1,2,1,
  2,1,2,1,1,2,1,1,1,1,1,1,1,1,2,2,1,1,1,1,2,1,1,1,1,1,2,1,1,1,1,1,1,1,1,1,1,
  1,1,1,1,1,1,1,1,2,2,2,2,2,2,2,2,2,2,2,1,2,2,2,2,2,1,1,2,1,1,1,1,1,1,1,1,1,
  1,1,1,1,1,1,1,1,1,1,1,2,2,7,2,1,1,2,1,2,1,2,1,1,1,1,1,1,1,1,1,2,2,1,1,1,2,
  1,1,1,1,1,2,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,4,4,4,4,4,4,7,1,1,4,4,4,4,
  4,4,7,1,1,4,4,4,4,4,4,7,1,1,4,4,4,4,4,4,7,1,1,4,4,4,4,4,4,7,1,1,1,1,1,1,1,
  1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,8,8,
  1,7,2,1,1,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,
  2,2,2,2,2,2,2,2,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,
  8,8,8,8,8,8,8,8,8,8,8,8,1,7,1,1,1,1,1,1,2,7,2,2,1,2,1,1,1,1,1,1,1,2,7,2,2,
  1,2,1,1,1,1,1,1,1,2,7,2,2,1,2,1,1,1,1,1,1,1,1,1,1,1,1,7,1,1,1,1,1,7,4,4,4,
  4,4,4,4,4,4,4,4,4,4,4,4,4,1,1,1,1,1,4,4,4,4,4,4,4,4,4,4,4,4,4,4,4,4,4,4,4,
  4,4,7,2,7,8,8,8,8,8,8,8,8,8,8,8,8,8,1,7,1,1,2,2,2,2,2,2,2,2,2,2,2,1,2,2,2,
  2,2,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,2,2,7,2,1,1,2,1,2,1,1,1,1,
  1,1,1,1,1,1,2,1,1,1,2,1,1,1,1,1,2,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,2,2,2,2,
  2,2,2,2,2,2,2,1,2,2,2,2,2,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,2,2,
  7,2,1,1,2,1,2,1,1,1,1,1,1,1,1,1,1,2,1,1,1,2,1,1,1,1,1,2,1,1,1,1,1,1,1,1,1,
  1,1,1,1,1,1,1,2,2,2,2,2,2,2,2,2,2,2,1,2,2,2,2,2,1,1,1,1,1,1,1,1,1,1,1,1,1,
  1,1,1,1,1,1,1,1,1,2,2,7,2,1,1,2,1,2,1,1,1,1,1,1,1,1,1,1,2,1,1,1,2,1,1,1,1,
  1,2,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,2,2,2,2,2,2,2,2,2,2,2,1,2,2,2,2,2,1,1,
  1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,2,2,7,2,1,1,2,1,2,1,1,1,1,1,1,1,1,
  1,1,2,1,1,1,2,1,1,1,1,1,2,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,2,2,2,2,2,2,2,2,
  2,2,2,1,2,2,2,2,2,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,2,2,7,2,1,1,
  2,1,2,1,1,1,1,1,1,1,1,1,1,2,1,1,1,2,1,1,1,1,1,2,1,1,1,1,1,1,1,1,1,1,1,1,1,
  1,1,1,2,2,2,2,2,2,2,2,2,2,2,1,2,2,2,2,2,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,
  1,1,1,1,1,2,2,7,2,1,1,2,1,2,1,1,1,1,1,1,1,1,1,1,2,1,1,1,2,1,1,1,1,1,2,1,1,
  1,1,1,1,1,1,1,1,1,1,1,1,1,1,2,2,2,2,2,2,2,2,2,2,2,1,2,2,2,2,2,1,1,1,1,1,1,
  1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,2,2,7,2,1,1,2,1,2,1,1,1,1,1,1,1,1,1,1,2,1,
  1,1,2,1,1,1,1,1,2,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,2,2,2,2,2,2,2,2,2,2,2,1,
  2,2,2,2,2,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,2,2,7,2,1,1,2,1,2,1,
  1,1,1,1,1,1,1,1,1,2,1,1,1,2,1,1,1,1,1,2,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,2,
  2,2,2,2,2,2,2,2,2,2,1,2,2,2,2,2,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,
  1,2,2,7,2,1,1,2,1,2,1,1,1,1,1,1,1,1,1,1,2,1,1,1,2,1,1,1,1,1,2,1,1,1,1,1,1,
  1,1,1,1,1,1,1,1,1,1,2,2,2,2,2,2,2,2,2,2,2,1,2,2,2,2,2,1,1,1,1,1,1,1,1,1,1,
  1,1,1,1,1,1,1,1,1,1,1,1,2,2,7,2,1,1,2,1,2,1,1,1,1,1,1,1,1,1,1,2,1,1,1,2,1,
  1,1,1,1,2,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,8,1,7,1,1,2,2,2,2,2,2,2,2,2,2,2,
  1,2,2,2,2,2,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,2,2,7,2,1,1,2,1,2,1,1,2,
  1,1,1,2,2,2,2,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,2,2,2,2,2,2,2,2,2,2,2,1,2,2,2,
  2,2,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,2,2,7,2,1,1,2,1,2,1,1,2,1,1,1,2,
  2,2,2,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,2,2,2,2,2,2,2,2,2,2,2,1,2,2,2,2,2,1,1,
  1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,2,2,7,2,1,1,2,1,2,1,1,2,1,1,1,2,2,2,2,1,
  1,1,1,1,1,1,1,1,1,1,1,1,1,1,2,2,2,2,2,2,2,2,2,2,2,1,2,2,2,2,2,1,1,1,1,1,1,
  1,1,1,1,1,1,1,1,1,1,1,1,1,2,2,7,2,1,1,2,1,2,1,1,2,1,1,1,2,2,2,2,1,1,1,1,1,
  1,1,1,1,1,1,1,1,1,1,2,2,2,2,2,2,2,2,2,2,2,1,2,2,2,2,2,1,1,1,1,1,1,1,1,1,1,
  1,1,1,1,1,1,1,1,1,2,2,7,2,1,1,2,1,2,1,1,2,1,1,1,2,2,2,2,1,1,1,1,1,1,1,1,1,
  1,1,1,1,1,1,2,2,2,2,2,2,2,2,2,2,2,1,2,2,2,2,2,1,1,1,1,1,1,1,1,1,1,1,1,1,1,
  1,1,1,1,1,1,1,1,2,2,7,2,1,1,2,1,2,1,1,1,1,2,1,1,1,2,1,1,1,1,1,2,1,1,1,1,1,
  1,1,1,1,1,1,1,1,1,1,1,2,2,2,2,2,2,2,2,2,2,2,1,2,2,2,2,2,1,1,1,1,1,1,1,1,1,
  1,1,1,1,1,1,1,1,1,1,1,1,1,2,2,7,2,1,1,2,1,2,1,1,1,1,2,1,1,1,2,1,1,1,1,1,2,
  1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,2,2,2,2,2,2,2,2,2,2,2,1,2,2,2,2,2,1,1,1,1,
  1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,2,2,7,2,1,1,2,1,2,1,1,1,1,1,2,1,1,1,2,
  1,1,1,1,1,2,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,2,2,2,2,2,2,2,2,2,2,2,1,2,2,2,
  2,2,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,2,2,7,2,1,1,2,1,2,1,1,1,1,
  1,2,1,1,1,2,1,1,1,1,1,2,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,2,2,2,2,2,2,2,2,2,
  2,2,1,2,2,2,2,2,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,2,2,7,2,1,1,2,
  1,2,1,1,1,1,1,2,1,1,1,2,1,1,1,1,1,2,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,2,2,2,
  2,2,2,2,2,2,2,2,1,2,2,2,2,2,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,2,
  2,7,2,1,1,2,1,2,1,1,1,1,1,2,1,1,1,2,1,1,1,1,1,2,1,1,1,1,1,1,1,1,1,1,1,1,1,
  1,1,1,2,2,2,2,2,2,2,2,2,2,2,1,2,2,2,2,2,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,
  1,1,1,1,1,2,2,7,2,1,1,2,1,2,1,1,1,1,1,1,2,1,1,1,2,1,1,1,1,1,2,1,1,1,1,1,1,
  1,1,1,1,1,1,1,1,1,1,2,2,2,2,2,2,2,2,2,2,2,1,2,2,2,2,2,1,1,1,1,1,1,1,1,1,1,
  1,1,1,1,1,1,1,1,1,1,1,1,2,2,7,2,1,1,2,1,2,1,1,1,1,1,1,2,1,1,1,2,1,1,1,1,1,
  2,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,2,2,2,2,2,2,2,2,2,2,2,1,2,2,2,2,2,1,1,1,
  1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,2,2,7,2,1,1,2,1,2,1,1,1,1,1,1,1,2,1,
  1,1,2,1,1,1,1,1,2,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,2,2,2,2,2,2,2,2,2,2,2,1,
  2,2,2,2,2,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,2,2,7,2,1,1,2,1,2,1,
  1,1,1,1,1,1,2,1,1,1,2,1,1,1,1,1,2,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,2,2,2,2,
  2,2,2,2,2,2,2,1,2,2,2,2,2,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,2,2,
  7,2,1,1,2,1,2,1,1,1,1,1,1,1,1,2,1,1,1,2,1,1,1,1,1,2,1,1,1,1,1,1,1,1,1,1,1,
  1,1,1,1,1,2,2,2,2,2,2,2,2,2,2,2,1,2,2,2,2,2,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,
  1,1,1,1,1,1,1,2,2,7,2,1,1,2,1,2,1,1,1,1,1,1,1,1,2,1,1,1,2,1,1,1,1,1,2,1,1,
  1,1,1,1,1,1,1,1,1,1,1,1,1,1,8,8,8,8,8,1,7,1,1,1,1,1,1,2,7,2,2,1,2,1,1,1,1,
  1,1,1,2,7,2,2,1,2,1,1,1,1,1,1,1,2,7,2,2,1,2,1,1,1,1,1,1,1,2,7,2,2,1,2,1,1,
  1,8,8,8,8,8,1,7,1,1,8,8,8,8,8,1,7,1,1,1,1,1,1,2,7,2,2,1,2,1,1,1,2,2,2,2,2,
  2,2,2,2,2,2,1,2,2,2,2,2,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,2,2,7,
  2,1,1,2,1,2,1,1,1,1,1,1,1,1,1,2,2,1,1,1,2,1,1,1,1,1,2,1,1,1,1,1,1,1,1,1,1,
  1,1,1,1,1,1,2,2,2,2,2,2,2,2,2,2,2,1,2,2,2,2,2,1,1,1,1,1,1,1,1,1,1,1,1,1,1,
  1,1,1,1,1,1,1,1,2,2,7,2,1,1,2,1,2,1,1,1,1,1,1,1,1,1,2,2,1,1,1,2,1,1,1,1,1,
  2,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,2,2,2,2,2,2,2,2,2,2,2,1,2,2,2,2,2,1,1,1,
  1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,2,2,7,2,1,1,2,1,2,1,1,1,1,1,1,1,1,1,
  2,2,1,1,1,2,1,1,1,1,1,2,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,7,1,2,1,2,2,2,2,
  2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,
  7,1,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,
  5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,7,1,3,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,
  8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,1,7,1,1,1,7,1,5,5,5,5,5,
  5,1,7,1,3,2,1,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,
  2,2,2,2,2,2,2,2,2,2,7,2,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,
  8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,1,7,1,1,1,4,4,4,4,4,4,7,1,8,8,8,8,8,8,8,8,
  8,8,8,8,8,1,7,1,1,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,
  5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,7,1,3,4,4,4,4,4,4,4,4,4,4,4,4,4,4,4,4,
  4,4,4,4,4,4,4,4,4,4,4,4,4,4,4,4,4,4,4,4,4,4,4,4,5,5,5,5,5,5,7,1,1,1,1,2,7,
  2,2,1,2,1,1,1,2,2,2,2,2,2,2,2,2,2,2,1,2,2,2,2,2,1,1,1,1,1,1,1,1,1,1,1,1,1,
  1,1,1,1,1,1,1,1,1,2,2,7,2,1,1,2,1,2,1,1,1,1,1,1,1,1,1,1,2,1,1,1,2,1,1,1,1,
  1,2,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,4,4,4,4,4,4,4,4,2,2,2,2,4,4,4,4,2,2,2,
  2,2,4,4,4,4,4,4,4,4,4,4,4,4,4,4,4,4,4,4,4,4,4,7,2,2,2,2,2,7,2,2,2,2,2,2,2,
  2,2,2,2,2,2,7,1,1,1,1,1,7,2,1,7,2,1,7,2,1,7,2,1,7,2,1,7,2,1,7,2,1,7,2,1,7,
  2,1,7,2,1,7,3,4,1,1,1,1,1,4,4,4,4,4,4,4,4,4,4,4,4,4,4,4,4,4,4,4,4,4,4,4,4,
  4,4,4,4,4,4,4,7,1,1,1,1,1,4,1,1,1,1,1,4,4,4,4,4,4,4,4,4,4,4,4,4,4,4,4,4,4,
  4,4,4,4,4,4,4,4,4,4,4,4,4,7,1,1,1,1,1,4,1,1,4,4,4,4,4,4,4,4,4,4,4,4,4,4,4,
  4,4,4,4,4,4,4,4,4,4,4,4,4,4,4,4,7,1,1,4,1,1,4,4,4,4,4,4,4,4,4,4,4,4,4,4,4,
  4,4,4,4,4,4,4,4,4,4,4,4,4,4,4,4,7,1,1,4,1,1,4,4,4,4,4,4,4,4,4,4,4,4,4,4,4,
  4,4,4,4,4,4,4,4,4,4,4,4,4,4,4,4,7,1,1,4,1,1,4,4,4,4,4,4,4,4,4,4,4,4,4,4,4,
  4,4,4,4,4,4,4,4,4,4,4,4,4,4,4,4,7,1,1,4,1,1,1,1,4,4,4,4,4,4,4,4,4,4,4,4,4,
  4,4,4,4,4,4,4,4,4,4,4,4,4,4,7,1,1,1,1,4,1,1,1,1,4,4,4,4,4,4,4,4,4,4,4,4,4,
  4,4,4,4,4,4,4,4,4,4,4,4,4,4,7,1,1,1,1,4,1,1,4,4,4,4,4,4,4,4,4,4,4,4,4,4,4,
  4,4,4,4,4,4,4,4,4,4,7,1,1,4,1,1,4,4,4,4,4,4,4,4,4,4,4,4,4,4,4,4,4,4,4,4,4,
  4,4,4,4,7,1,1,4,1,1,4,4,4,4,4,4,4,4,4,4,4,4,4,4,4,4,4,4,4,4,4,4,4,7,1,1,4,
  1,1,4,4,4,4,4,4,4,4,4,4,4,4,4,4,4,4,4,4,4,4,4,4,4,7,1,1,1,1,1,1,2,7,2,2,1,
  2,1,1,1,1,1,1,1,2,7,2,2,1,2,1,1,1,1,1,1,1,2,7,2,2,1,2,1,1,1,8,8,8,8,8,8,8,
  8,8,1,7,1,1,4,4,4,4,4,4,4,4,4,4,4,4,4,4,4,4,4,4,4,4,4,2,4,4,4,4,4,4,4,4,4,
  4,4,4,4,4,4,4,4,4,4,4,4,7,2,2,2,2,2,2,2,2,2,2,2,1,2,2,2,2,2,1,1,2,1,1,1,1,
  1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,2,2,7,2,1,1,2,1,2,1,2,1,1,2,1,1,1,1,1,1,1,
  1,2,2,1,1,1,2,1,1,1,1,1,2,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,2,2,2,2,2,2,
  2,2,2,2,2,1,2,2,2,2,2,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,2,2,7,2,
  1,1,2,1,2,1,1,1,1,1,1,1,1,1,2,2,1,1,1,2,1,1,1,1,1,2,1,1,1,1,1,1,1,1,1,1,1,
  1,1,1,1,1,2,2,2,2,2,2,2,2,2,2,2,2,2,7,2,1,1,8,8,8,8,8,1,7,1,1,2,2,2,2,2,2,
  2,2,2,2,2,2,2,2,2,2,2,5,2,1,7,1,3,2,5,5,7,1,3,8,1,7,1,1,2,2,2,2,2,2,2,2,2,
  7,2,2,1,1,1,1,1,2,7,2,2,1,2,1,1,1,1,7,2
};


static const unsigned short ag_pstt[] = {
2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,0,1,2,
19,21,
136,136,136,136,136,136,136,136,136,136,136,136,3,8,8,8,8,8,171,170,169,135,
  7,8,10,2,9,0,11,11,11,10,8,11,5,5,4,6,4,
20,20,20,20,20,20,20,20,20,20,20,20,20,20,1,3,1,890,
137,137,137,137,137,137,137,137,137,137,137,137,138,138,138,138,138,168,168,
  168,137,168,168,168,168,167,168,168,168,4,138,
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
  20,20,1,6,1,880,
40,7,39,
42,42,41,43,44,45,40,25,8,25,25,48,25,47,46,46,42,42,
49,49,49,49,49,49,49,49,49,49,49,49,49,49,49,49,49,49,49,49,49,49,49,49,49,
  49,49,49,49,49,49,49,49,49,49,49,49,49,49,49,49,49,49,49,49,49,49,49,49,
  49,49,49,49,49,49,49,49,49,49,49,49,49,49,49,49,49,49,49,49,49,49,49,49,
  49,49,49,49,49,49,49,49,49,49,49,49,49,49,49,49,49,49,49,49,49,49,49,49,
  49,49,49,49,49,49,49,49,49,49,49,49,49,49,49,49,49,49,49,49,49,49,49,49,
  49,49,49,49,49,49,49,49,49,49,49,49,49,49,49,49,49,49,49,49,49,49,49,49,
  49,49,49,49,49,49,49,49,49,49,49,49,49,49,49,49,49,49,49,49,49,49,49,49,
  49,49,49,49,49,49,49,49,49,49,49,49,49,49,49,49,49,20,20,49,49,49,49,49,
  49,49,1,9,1,49,
136,136,136,136,136,136,136,136,136,136,136,136,117,118,119,120,121,122,123,
  124,125,126,127,128,129,130,131,132,133,134,135,136,137,138,139,140,68,
  69,70,71,72,73,141,142,143,144,145,146,74,75,76,77,78,147,148,149,79,80,
  81,82,83,150,151,152,153,154,155,156,157,158,159,160,161,162,163,164,
  113,65,66,67,171,172,173,174,175,176,177,178,179,180,181,182,183,184,
  185,186,165,166,167,168,169,170,84,85,86,87,88,89,51,52,53,90,91,92,93,
  94,95,96,97,98,54,55,99,100,101,102,103,104,105,106,56,57,58,107,108,59,
  60,109,110,111,112,61,62,63,64,271,271,271,271,271,271,271,271,271,271,
  271,271,271,271,271,271,271,271,271,271,271,271,271,271,271,271,271,271,
  271,271,271,271,18,19,22,271,271,271,271,271,21,21,41,21,21,21,40,135,
  21,19,10,267,268,269,266,272,40,271,271,37,270,111,112,272,50,265,202,
  266,266,266,266,266,266,266,266,266,266,332,332,332,332,332,333,334,335,
  336,339,339,339,340,341,345,345,345,345,346,347,348,349,350,351,352,353,
  356,356,356,357,358,359,360,361,362,363,364,365,369,369,369,369,370,371,
  372,373,374,375,248,247,246,245,244,243,264,263,262,261,260,259,258,257,
  256,255,254,253,252,251,250,249,190,116,189,115,188,114,187,242,241,240,
  239,238,237,236,235,234,233,232,231,230,229,228,227,227,227,227,227,227,
  226,225,224,223,223,223,223,223,223,222,221,220,219,218,217,216,216,216,
  216,215,215,215,215,214,213,212,211,210,209,208,207,206,205,204,203,201,
  200,199,198,197,196,195,194,193,192,191,265,
136,136,136,136,136,136,136,136,136,136,136,136,3,8,8,8,8,8,171,170,169,135,
  7,4,10,11,9,3,3,10,8,3,5,5,4,6,4,
20,20,20,20,20,20,20,20,20,20,20,20,20,1,12,1,910,
20,20,20,20,20,20,20,20,20,20,20,20,20,1,13,1,909,
20,20,14,1,908,
20,20,15,1,906,
20,20,20,20,20,20,20,20,20,20,20,20,20,20,20,20,20,20,20,20,20,20,20,20,20,
  20,20,20,20,20,20,20,20,20,20,20,20,20,20,20,20,20,20,16,1,905,
20,1,17,1,904,
20,20,20,20,20,20,18,1,903,
20,20,20,20,20,20,19,1,902,
20,20,20,20,20,20,20,20,20,20,20,20,20,1,20,1,901,
20,20,20,20,20,20,20,20,20,20,20,20,20,20,20,20,20,20,20,20,20,20,20,20,20,
  20,20,20,20,20,20,20,20,20,20,20,20,20,20,20,20,1,21,1,900,
20,20,20,20,20,20,20,20,20,20,20,20,20,20,20,20,20,20,20,20,20,20,20,20,20,
  20,20,20,20,20,20,20,20,20,20,20,20,20,20,20,20,1,22,1,899,
20,20,20,20,20,20,20,20,20,20,20,20,20,1,23,1,898,
20,20,20,20,20,1,24,1,892,
20,20,20,20,20,20,20,20,20,20,20,20,20,20,20,20,20,20,20,20,20,1,25,1,885,
136,136,136,136,136,136,136,136,136,136,136,136,135,26,267,273,265,265,
136,136,136,136,136,136,136,136,136,136,136,136,135,27,274,265,265,
275,1,28,1,275,
276,1,29,1,276,
277,277,277,277,277,277,277,277,277,277,277,277,277,277,277,277,277,277,277,
  277,277,277,277,277,277,277,277,277,277,277,277,277,277,277,277,277,277,
  277,277,277,277,277,1,30,1,277,
145,31,280,278,279,
281,281,281,281,281,1,32,1,281,
282,282,282,282,282,1,33,1,282,
136,136,136,136,136,136,136,136,136,136,136,136,135,34,283,265,265,
136,136,136,136,136,136,136,136,136,136,136,285,136,256,256,256,256,287,289,
  296,298,299,300,301,302,303,304,305,306,321,323,324,325,286,322,288,297,
  309,284,270,135,35,240,294,295,241,290,242,291,267,335,334,332,337,333,
  336,331,338,340,225,326,311,265,226,339,327,331,330,329,328,224,331,320,
  319,318,317,316,315,314,313,312,310,293,292,265,308,307,
136,136,136,136,136,136,136,136,136,136,136,285,136,256,256,256,256,287,289,
  296,298,299,300,301,302,303,304,305,306,321,323,324,325,286,322,288,297,
  309,284,270,135,36,240,294,295,241,290,242,291,267,335,334,332,337,333,
  336,331,338,341,225,326,311,265,226,339,327,331,330,329,328,224,331,320,
  319,318,317,316,315,314,313,312,310,293,292,265,308,307,
136,136,136,136,136,136,136,136,136,136,136,136,135,37,342,265,265,
343,345,347,349,351,38,352,350,348,346,344,
163,145,39,356,278,355,353,279,354,
20,20,20,20,20,20,20,20,20,20,20,20,20,20,20,20,20,20,20,20,20,20,20,20,20,
  20,20,20,20,20,20,20,20,20,20,20,20,20,20,20,20,20,20,20,20,20,20,20,20,
  20,20,20,20,20,40,1,875,
20,20,20,20,20,20,20,20,20,20,20,20,20,20,20,20,20,20,20,20,20,20,20,20,20,
  20,20,20,20,20,20,20,20,20,20,20,20,20,20,20,20,20,20,20,20,20,20,20,20,
  20,20,20,20,20,20,20,20,20,20,20,20,20,20,20,20,20,20,20,20,20,20,20,20,
  20,20,20,20,20,20,20,20,20,20,20,20,20,20,20,20,41,1,881,
357,25,42,358,358,358,
20,20,20,20,20,20,20,20,20,20,20,20,20,20,20,20,20,20,20,20,20,20,20,20,20,
  20,20,20,20,20,20,20,20,20,20,20,20,20,20,20,20,20,20,20,20,43,1,879,
20,20,20,20,20,20,20,20,20,20,20,20,20,20,20,20,20,20,20,20,20,20,20,20,20,
  20,20,20,20,20,20,20,20,20,20,20,20,20,20,20,20,20,20,20,20,20,20,20,44,
  1,877,
20,20,20,20,20,20,20,20,20,20,20,20,20,20,20,20,20,20,20,20,20,20,20,20,20,
  20,20,20,20,20,20,20,20,20,20,20,20,20,20,20,20,20,20,45,1,876,
359,361,363,364,365,366,367,368,369,370,371,372,373,374,375,376,377,378,379,
  380,381,382,389,393,394,392,323,325,391,383,387,384,386,390,362,360,309,
  388,385,41,43,395,44,40,16,46,17,16,16,46,14,46,16,16,16,16,16,16,16,16,
  16,16,16,16,16,16,16,16,16,16,16,16,16,16,16,16,16,16,16,16,16,16,16,16,
  16,16,16,16,16,16,16,16,
359,361,363,364,365,366,367,368,369,370,371,372,373,374,375,376,377,378,379,
  380,381,382,389,393,394,392,323,325,391,383,387,384,386,390,362,360,309,
  388,385,41,44,397,396,47,396,397,396,396,396,396,396,396,396,396,396,
  396,396,396,396,396,396,396,396,396,396,396,396,396,396,396,396,396,396,
  396,396,396,396,396,396,396,396,396,396,396,396,396,396,
359,361,363,364,365,366,367,368,369,370,371,372,373,374,375,376,377,378,379,
  380,381,382,389,393,394,392,323,325,391,383,387,384,386,390,362,360,309,
  388,385,41,44,398,396,48,396,398,396,396,396,396,396,396,396,396,396,
  396,396,396,396,396,396,396,396,396,396,396,396,396,396,396,396,396,396,
  396,396,396,396,396,396,396,396,396,396,396,396,396,396,
136,136,136,136,136,136,136,136,136,136,136,136,117,118,119,120,121,122,123,
  124,125,126,127,128,129,130,131,132,133,134,135,136,137,138,139,140,68,
  69,70,71,72,73,141,142,143,144,145,146,74,75,76,77,78,147,148,149,79,80,
  81,82,83,150,151,152,153,154,155,156,157,158,159,160,161,162,163,164,
  113,65,66,67,171,172,173,174,175,176,177,178,179,180,181,182,183,184,
  185,186,165,166,167,168,169,170,84,85,86,87,88,89,51,52,53,90,91,92,93,
  94,95,96,97,98,54,55,99,100,101,102,103,104,105,106,56,57,58,107,108,59,
  60,109,110,111,112,61,62,63,64,401,401,401,401,401,401,401,401,401,401,
  401,401,401,401,401,401,401,401,401,401,401,401,401,401,401,401,401,401,
  401,401,401,401,18,19,22,401,401,401,401,401,399,401,401,41,43,44,45,40,
  135,24,49,267,268,269,266,402,24,24,48,24,47,46,46,401,401,400,38,270,
  111,112,402,50,265,202,266,266,266,266,266,266,266,266,266,266,332,332,
  332,332,332,333,334,335,336,339,339,339,340,341,345,345,345,345,346,347,
  348,349,350,351,352,353,356,356,356,357,358,359,360,361,362,363,364,365,
  369,369,369,369,370,371,372,373,374,375,248,247,246,245,244,243,264,263,
  262,261,260,259,258,257,256,255,254,253,252,251,250,249,190,116,189,115,
  188,114,187,242,241,240,239,238,237,236,235,234,233,232,231,230,229,228,
  227,227,227,227,227,227,226,225,224,223,223,223,223,223,223,222,221,220,
  219,218,217,216,216,216,216,215,215,215,215,214,213,212,211,210,209,208,
  207,206,205,204,203,201,200,199,198,197,196,195,194,193,192,191,265,
403,403,403,403,403,403,403,1,50,1,403,
20,20,20,20,20,20,20,51,1,1034,
20,20,20,20,20,20,20,52,1,1033,
20,20,20,20,20,20,20,53,1,1032,
20,20,20,20,20,20,20,54,1,1022,
20,20,20,20,20,20,20,55,1,1021,
20,20,20,20,20,20,20,56,1,1012,
20,20,20,20,20,20,20,57,1,1011,
20,20,20,20,20,20,20,58,1,1010,
20,20,20,20,20,20,20,59,1,1007,
20,20,20,20,20,20,20,60,1,1006,
20,20,20,20,20,20,20,61,1,1001,
20,20,20,20,20,20,20,62,1,1000,
20,20,20,20,20,20,20,63,1,999,
20,20,20,20,20,20,20,64,1,998,
20,20,20,20,20,20,20,20,20,20,65,1,1067,
20,20,20,20,20,20,20,20,20,20,66,1,1066,
20,20,20,20,20,20,20,20,67,1,1065,
20,20,20,20,20,20,20,20,20,20,20,20,20,20,20,20,20,20,20,20,20,20,20,20,20,
  20,20,20,20,20,20,20,20,20,20,20,20,20,20,20,20,20,68,1,1108,
20,20,20,20,20,20,20,20,20,20,20,20,20,20,20,20,20,20,20,20,20,20,20,20,20,
  20,20,20,20,20,20,20,20,20,20,20,20,20,20,20,20,20,69,1,1107,
20,20,20,20,20,20,20,20,20,20,20,20,20,20,20,20,20,20,20,20,20,20,20,20,20,
  20,20,20,20,20,20,20,20,20,20,20,20,20,20,20,20,20,70,1,1106,
20,20,20,20,20,20,20,20,20,20,20,20,20,20,20,20,20,20,20,20,20,20,20,20,20,
  20,20,20,20,20,20,20,20,20,20,20,20,20,20,20,20,20,71,1,1105,
20,20,20,20,20,20,20,20,20,20,20,20,20,20,20,20,20,20,20,20,20,20,20,20,20,
  20,20,20,20,20,20,20,20,20,20,20,20,20,20,20,20,20,72,1,1104,
20,20,20,20,20,20,20,20,20,20,20,20,20,20,20,20,20,20,20,20,20,20,20,20,20,
  20,20,20,20,20,20,20,20,20,20,20,20,20,20,20,20,20,73,1,1103,
20,20,20,20,20,20,20,20,20,20,20,20,20,20,20,20,20,20,20,20,20,20,20,20,20,
  20,20,20,20,20,20,20,20,20,20,20,20,20,20,20,20,20,74,1,1096,
20,20,20,20,20,20,20,20,20,20,20,20,20,20,20,20,20,20,20,20,20,20,20,20,20,
  20,20,20,20,20,20,20,20,20,20,20,20,20,20,20,20,20,75,1,1095,
20,20,20,20,20,20,20,20,20,20,20,20,20,20,20,20,20,20,20,20,20,20,20,20,20,
  20,20,20,20,20,20,20,20,20,20,20,20,20,20,20,20,20,76,1,1094,
20,20,20,20,20,20,20,20,20,20,20,20,20,20,20,20,20,20,20,20,20,20,20,20,20,
  20,20,20,20,20,20,20,20,20,20,20,20,20,20,20,20,20,77,1,1093,
20,20,20,20,20,20,20,20,20,20,20,20,20,20,20,20,20,20,20,20,20,20,20,20,20,
  20,20,20,20,20,20,20,20,20,20,20,20,20,20,20,20,20,78,1,1092,
20,20,20,20,20,20,20,20,20,20,20,20,20,20,20,20,20,20,20,20,20,20,20,20,20,
  20,20,20,20,20,20,20,20,20,20,20,20,20,20,20,20,20,79,1,1088,
20,20,20,20,20,20,20,20,20,20,20,20,20,20,20,20,20,20,20,20,20,20,20,20,20,
  20,20,20,20,20,20,20,20,20,20,20,20,20,20,20,20,20,80,1,1087,
20,20,20,20,20,20,20,20,20,20,20,20,20,20,20,20,20,20,20,20,20,20,20,20,20,
  20,20,20,20,20,20,20,20,20,20,20,20,20,20,20,20,20,81,1,1086,
20,20,20,20,20,20,20,20,20,20,20,20,20,20,20,20,20,20,20,20,20,20,20,20,20,
  20,20,20,20,20,20,20,20,20,20,20,20,20,20,20,20,20,82,1,1085,
20,20,20,20,20,20,20,20,20,20,20,20,20,20,20,20,20,20,20,20,20,20,20,20,20,
  20,20,20,20,20,20,20,20,20,20,20,20,20,20,20,20,20,83,1,1084,
20,20,20,20,20,20,20,84,1,1040,
20,20,20,20,20,20,20,85,1,1039,
20,20,20,20,20,20,20,86,1,1038,
20,20,20,20,20,20,20,87,1,1037,
20,20,20,20,20,20,20,88,1,1036,
20,20,20,20,20,20,20,89,1,1035,
20,20,20,20,20,20,20,90,1,1031,
20,20,20,20,20,20,20,91,1,1030,
20,20,20,20,20,20,20,92,1,1029,
20,20,20,20,20,20,20,93,1,1028,
20,20,20,20,20,20,20,94,1,1027,
20,20,20,20,20,20,20,95,1,1026,
20,20,20,20,20,20,20,96,1,1025,
20,20,20,20,20,20,20,97,1,1024,
20,20,20,20,20,20,20,98,1,1023,
20,20,20,20,20,20,20,99,1,1020,
20,20,20,20,20,20,20,100,1,1019,
20,20,20,20,20,20,20,101,1,1018,
20,20,20,20,20,20,20,102,1,1017,
20,20,20,20,20,20,20,103,1,1016,
20,20,20,20,20,20,20,104,1,1015,
20,20,20,20,20,20,20,105,1,1014,
20,20,20,20,20,20,20,106,1,1013,
20,20,20,20,20,20,20,107,1,1009,
20,20,20,20,20,20,20,108,1,1008,
20,20,20,20,20,20,20,109,1,1005,
20,20,20,20,20,20,20,110,1,1004,
20,20,20,20,20,20,20,111,1,1003,
20,20,20,20,20,20,20,112,1,1002,
20,20,20,20,20,113,1,1068,
404,404,404,404,404,404,404,404,404,1,114,1,404,
405,405,405,405,405,405,405,405,405,1,115,1,405,
406,406,406,406,406,406,406,1,116,1,406,
20,20,20,20,20,20,20,20,20,20,20,20,20,20,20,20,20,20,20,20,20,20,20,20,20,
  20,20,20,20,20,20,20,20,20,20,20,20,20,20,20,20,20,117,1,1132,
20,20,20,20,20,20,20,20,20,20,20,20,20,20,20,20,20,20,20,20,20,20,20,20,20,
  20,20,20,20,20,20,20,20,20,20,20,20,20,20,20,20,20,118,1,1131,
20,20,20,20,20,20,20,20,20,20,20,20,20,20,20,20,20,20,20,20,20,20,20,20,20,
  20,20,20,20,20,20,20,20,20,20,20,20,20,20,20,20,20,119,1,1130,
20,20,20,20,20,20,20,20,20,20,20,20,20,20,20,20,20,20,20,20,20,20,20,20,20,
  20,20,20,20,20,20,20,20,20,20,20,20,20,20,20,20,20,120,1,1129,
20,20,20,20,20,20,20,20,20,20,20,20,20,20,20,20,20,20,20,20,20,20,20,20,20,
  20,20,20,20,20,20,20,20,20,20,20,20,20,20,20,20,20,121,1,1128,
20,20,20,20,20,20,20,20,20,20,20,20,20,20,20,20,20,20,20,20,20,20,20,20,20,
  20,20,20,20,20,20,20,20,20,20,20,20,20,20,20,20,20,122,1,1127,
20,20,20,20,20,20,20,20,20,20,20,20,20,20,20,20,20,20,20,20,20,20,20,20,20,
  20,20,20,20,20,20,20,20,20,20,20,20,20,20,20,20,20,123,1,1126,
20,20,20,20,20,20,20,20,20,20,20,20,20,20,20,20,20,20,20,20,20,20,20,20,20,
  20,20,20,20,20,20,20,20,20,20,20,20,20,20,20,20,20,124,1,1125,
20,20,20,20,20,20,20,20,20,20,20,20,20,20,20,20,20,20,20,20,20,20,20,20,20,
  20,20,20,20,20,20,20,20,20,20,20,20,20,20,20,20,20,125,1,1124,
20,20,20,20,20,20,20,20,20,20,20,20,20,20,20,20,20,20,20,20,20,20,20,20,20,
  20,20,20,20,20,20,20,20,20,20,20,20,20,20,20,20,20,126,1,1123,
20,20,20,20,20,20,20,20,20,20,20,20,20,20,20,20,20,20,20,20,20,20,20,20,20,
  20,20,20,20,20,20,20,20,20,20,20,20,20,20,20,20,20,127,1,1122,
20,20,20,20,20,20,20,20,20,20,20,20,20,20,20,20,20,20,20,20,20,20,20,20,20,
  20,20,20,20,20,20,20,20,20,20,20,20,20,20,20,20,20,128,1,1121,
20,20,20,20,20,20,20,20,20,20,20,20,20,20,20,20,20,20,20,20,20,20,20,20,20,
  20,20,20,20,20,20,20,20,20,20,20,20,20,20,20,20,20,129,1,1120,
20,20,20,20,20,20,20,20,20,20,20,20,20,20,20,20,20,20,20,20,20,20,20,20,20,
  20,20,20,20,20,20,20,20,20,20,20,20,20,20,20,20,20,130,1,1119,
20,20,20,20,20,20,20,20,20,20,20,20,20,20,20,20,20,20,20,20,20,20,20,20,20,
  20,20,20,20,20,20,20,20,20,20,20,20,20,20,20,20,20,131,1,1118,
20,20,20,20,20,20,20,20,20,20,20,20,20,20,20,20,20,20,20,20,20,20,20,20,20,
  20,20,20,20,20,20,20,20,20,20,20,20,20,20,20,20,20,132,1,1117,
20,20,20,20,20,20,20,20,20,20,20,20,20,20,20,20,20,20,20,20,20,20,20,20,20,
  20,20,20,20,20,20,20,20,20,20,20,20,20,20,20,20,20,133,1,1116,
20,20,20,20,20,20,20,20,20,20,20,20,20,20,20,20,20,20,20,20,20,20,20,20,20,
  20,20,20,20,20,20,20,20,20,20,20,20,20,20,20,20,20,134,1,1115,
20,20,20,20,20,20,20,20,20,20,20,20,20,20,20,20,20,20,20,20,20,20,20,20,20,
  20,20,20,20,20,20,20,20,20,20,20,20,20,20,20,20,20,135,1,1114,
20,20,20,20,20,20,20,20,20,20,20,20,20,20,20,20,20,20,20,20,20,20,20,20,20,
  20,20,20,20,20,20,20,20,20,20,20,20,20,20,20,20,20,136,1,1113,
20,20,20,20,20,20,20,20,20,20,20,20,20,20,20,20,20,20,20,20,20,20,20,20,20,
  20,20,20,20,20,20,20,20,20,20,20,20,20,20,20,20,20,137,1,1112,
20,20,20,20,20,20,20,20,20,20,20,20,20,20,20,20,20,20,20,20,20,20,20,20,20,
  20,20,20,20,20,20,20,20,20,20,20,20,20,20,20,20,20,138,1,1111,
20,20,20,20,20,20,20,20,20,20,20,20,20,20,20,20,20,20,20,20,20,20,20,20,20,
  20,20,20,20,20,20,20,20,20,20,20,20,20,20,20,20,20,139,1,1110,
20,20,20,20,20,20,20,20,20,20,20,20,20,20,20,20,20,20,20,20,20,20,20,20,20,
  20,20,20,20,20,20,20,20,20,20,20,20,20,20,20,20,20,140,1,1109,
20,20,20,20,20,20,20,20,20,20,20,20,20,20,20,20,20,20,20,20,20,20,20,20,20,
  20,20,20,20,20,20,20,20,20,20,20,20,20,20,20,20,20,141,1,1102,
20,20,20,20,20,20,20,20,20,20,20,20,20,20,20,20,20,20,20,20,20,20,20,20,20,
  20,20,20,20,20,20,20,20,20,20,20,20,20,20,20,20,20,142,1,1101,
20,20,20,20,20,20,20,20,20,20,20,20,20,20,20,20,20,20,20,20,20,20,20,20,20,
  20,20,20,20,20,20,20,20,20,20,20,20,20,20,20,20,20,143,1,1100,
20,20,20,20,20,20,20,20,20,20,20,20,20,20,20,20,20,20,20,20,20,20,20,20,20,
  20,20,20,20,20,20,20,20,20,20,20,20,20,20,20,20,20,144,1,1099,
20,20,20,20,20,20,20,20,20,20,20,20,20,20,20,20,20,20,20,20,20,20,20,20,20,
  20,20,20,20,20,20,20,20,20,20,20,20,20,20,20,20,20,145,1,1098,
20,20,20,20,20,20,20,20,20,20,20,20,20,20,20,20,20,20,20,20,20,20,20,20,20,
  20,20,20,20,20,20,20,20,20,20,20,20,20,20,20,20,20,146,1,1097,
20,20,20,20,20,20,20,20,20,20,20,20,20,20,20,20,20,20,20,20,20,20,20,20,20,
  20,20,20,20,20,20,20,20,20,20,20,20,20,20,20,20,20,147,1,1091,
20,20,20,20,20,20,20,20,20,20,20,20,20,20,20,20,20,20,20,20,20,20,20,20,20,
  20,20,20,20,20,20,20,20,20,20,20,20,20,20,20,20,20,148,1,1090,
20,20,20,20,20,20,20,20,20,20,20,20,20,20,20,20,20,20,20,20,20,20,20,20,20,
  20,20,20,20,20,20,20,20,20,20,20,20,20,20,20,20,20,149,1,1089,
20,20,20,20,20,20,20,20,20,20,20,20,20,20,20,20,20,20,20,20,20,20,20,20,20,
  20,20,20,20,20,20,20,20,20,20,20,20,20,20,20,20,20,150,1,1083,
20,20,20,20,20,20,20,20,20,20,20,20,20,20,20,20,20,20,20,20,20,20,20,20,20,
  20,20,20,20,20,20,20,20,20,20,20,20,20,20,20,20,20,151,1,1082,
20,20,20,20,20,20,20,20,20,20,20,20,20,20,20,20,20,20,20,20,20,20,20,20,20,
  20,20,20,20,20,20,20,20,20,20,20,20,20,20,20,20,20,152,1,1081,
20,20,20,20,20,20,20,20,20,20,20,20,20,20,20,20,20,20,20,20,20,20,20,20,20,
  20,20,20,20,20,20,20,20,20,20,20,20,20,20,20,20,20,153,1,1080,
20,20,20,20,20,20,20,20,20,20,20,20,20,20,20,20,20,20,20,20,20,20,20,20,20,
  20,20,20,20,20,20,20,20,20,20,20,20,20,20,20,20,20,154,1,1079,
20,20,20,20,20,20,20,20,20,20,20,20,20,20,20,20,20,20,20,20,20,20,20,20,20,
  20,20,20,20,20,20,20,20,20,20,20,20,20,20,20,20,20,155,1,1078,
20,20,20,20,20,20,20,20,20,20,20,20,20,20,20,20,20,20,20,20,20,20,20,20,20,
  20,20,20,20,20,20,20,20,20,20,20,20,20,20,20,20,20,156,1,1077,
20,20,20,20,20,20,20,20,20,20,20,20,20,20,20,20,20,20,20,20,20,20,20,20,20,
  20,20,20,20,20,20,20,20,20,20,20,20,20,20,20,20,20,157,1,1076,
20,20,20,20,20,20,20,20,20,20,20,20,20,20,20,20,20,20,20,20,20,20,20,20,20,
  20,20,20,20,20,20,20,20,20,20,20,20,20,20,20,20,20,158,1,1075,
20,20,20,20,20,20,20,20,20,20,20,20,20,20,20,20,20,20,20,20,20,20,20,20,20,
  20,20,20,20,20,20,20,20,20,20,20,20,20,20,20,20,20,159,1,1074,
20,20,20,20,20,20,20,20,20,20,20,20,20,20,20,20,20,20,20,20,20,20,20,20,20,
  20,20,20,20,20,20,20,20,20,20,20,20,20,20,20,20,20,160,1,1073,
20,20,20,20,20,20,20,20,20,20,20,20,20,20,20,20,20,20,20,20,20,20,20,20,20,
  20,20,20,20,20,20,20,20,20,20,20,20,20,20,20,20,20,161,1,1072,
20,20,20,20,20,20,20,20,20,20,20,20,20,20,20,20,20,20,20,20,20,20,20,20,20,
  20,20,20,20,20,20,20,20,20,20,20,20,20,20,20,20,20,162,1,1071,
20,20,20,20,20,20,20,20,20,20,20,20,20,20,20,20,20,20,20,20,20,20,20,20,20,
  20,20,20,20,20,20,20,20,20,20,20,20,20,20,20,20,20,163,1,1070,
20,20,20,20,20,20,20,20,20,20,20,20,20,20,20,20,20,20,20,20,20,20,20,20,20,
  20,20,20,20,20,20,20,20,20,20,20,20,20,20,20,20,20,164,1,1069,
20,20,20,20,20,20,20,20,20,165,1,1046,
20,20,20,20,20,20,20,20,20,166,1,1045,
20,20,20,20,20,20,20,20,20,167,1,1044,
20,20,20,20,20,20,20,20,20,168,1,1043,
20,20,20,20,20,169,1,1042,
20,20,20,20,20,170,1,1041,
20,20,20,20,20,20,20,20,171,1,1064,
20,20,20,20,20,20,20,20,172,1,1063,
20,20,20,20,20,20,20,20,173,1,1062,
20,20,20,20,20,20,20,20,20,20,174,1,1060,
20,20,20,20,20,20,20,20,20,20,175,1,1059,
20,20,20,20,20,20,20,20,20,20,176,1,1058,
20,20,20,20,20,20,20,20,20,20,177,1,1057,
20,20,20,20,20,20,20,20,20,20,178,1,1056,
20,20,20,20,20,20,20,20,20,20,179,1,1055,
20,20,20,20,20,20,20,20,20,20,180,1,1054,
20,20,20,20,20,20,20,20,20,20,181,1,1053,
20,20,20,20,20,20,20,20,20,20,182,1,1052,
20,20,20,20,20,20,20,20,20,20,183,1,1051,
20,20,20,20,20,20,20,20,20,20,184,1,1050,
20,20,20,20,20,20,20,20,20,20,185,1,1049,
20,20,20,20,20,20,20,20,20,20,186,1,1048,
407,407,407,407,1,187,1,407,
408,1,188,1,408,
409,1,189,1,409,
410,1,190,1,410,
411,411,411,411,411,411,411,411,411,411,411,411,411,411,411,411,411,411,411,
  411,411,411,411,411,411,411,411,411,411,411,411,411,411,411,411,411,411,
  411,411,411,411,1,191,1,411,
412,412,412,412,412,412,412,412,412,412,412,412,412,412,412,412,412,412,412,
  412,412,412,412,412,412,412,412,412,412,412,412,412,412,412,412,412,412,
  412,412,412,412,1,192,1,412,
413,413,413,413,413,413,413,413,413,413,413,413,413,413,413,413,413,413,413,
  413,413,413,413,413,413,413,413,413,413,413,413,413,413,413,413,413,413,
  413,413,413,413,1,193,1,413,
414,414,414,414,414,414,414,414,414,414,414,414,414,414,414,414,414,414,414,
  414,414,414,414,414,414,414,414,414,414,414,414,414,414,414,414,414,414,
  414,414,414,414,1,194,1,414,
415,415,415,415,415,415,415,415,415,415,415,415,415,415,415,415,415,415,415,
  415,415,415,415,415,415,415,415,415,415,415,415,415,415,415,415,415,415,
  415,415,415,415,1,195,1,415,
416,416,416,416,416,416,416,416,416,416,416,416,416,416,416,416,416,416,416,
  416,416,416,416,416,416,416,416,416,416,416,416,416,416,416,416,416,416,
  416,416,416,416,1,196,1,416,
417,417,417,417,417,417,417,417,417,417,417,417,417,417,417,417,417,417,417,
  417,417,417,417,417,417,417,417,417,417,417,417,417,417,417,417,417,417,
  417,417,417,417,1,197,1,417,
418,418,418,418,418,418,418,418,418,418,418,418,418,418,418,418,418,418,418,
  418,418,418,418,418,418,418,418,418,418,418,418,418,418,418,418,418,418,
  418,418,418,418,1,198,1,418,
419,419,419,419,419,419,419,419,419,419,419,419,419,419,419,419,419,419,419,
  419,419,419,419,419,419,419,419,419,419,419,419,419,419,419,419,419,419,
  419,419,419,419,1,199,1,419,
420,420,420,420,420,420,420,420,420,420,420,420,420,420,420,420,420,420,420,
  420,420,420,420,420,420,420,420,420,420,420,420,420,420,420,420,420,420,
  420,420,420,420,1,200,1,420,
421,421,421,421,421,421,421,421,421,421,421,421,421,421,421,421,421,421,421,
  421,421,421,421,421,421,421,421,421,421,421,421,421,421,421,421,421,421,
  421,421,421,421,1,201,1,421,
422,422,422,422,422,422,422,422,422,422,422,422,422,422,422,422,422,422,422,
  422,422,422,422,422,422,422,422,422,422,422,422,422,422,422,422,422,422,
  422,422,422,422,1,202,1,422,
423,423,423,423,423,423,423,423,423,423,423,423,423,423,423,423,423,423,423,
  423,423,423,423,423,423,423,423,423,423,423,423,423,423,423,423,423,423,
  423,423,423,423,1,203,1,423,
424,424,424,424,424,424,424,424,424,424,424,424,424,424,424,424,424,424,424,
  424,424,424,424,424,424,424,424,424,424,424,424,424,424,424,424,424,424,
  424,424,424,424,1,204,1,424,
425,425,425,425,425,425,425,425,425,425,425,425,425,425,425,425,425,425,425,
  425,425,425,425,425,425,425,425,425,425,425,425,425,425,425,425,425,425,
  425,425,425,425,1,205,1,425,
426,426,426,426,426,426,426,426,426,426,426,426,426,426,426,426,426,426,426,
  426,426,426,426,426,426,426,426,426,426,426,426,426,426,426,426,426,426,
  426,426,426,426,1,206,1,426,
427,427,427,427,427,427,427,427,427,427,427,427,427,427,427,427,427,427,427,
  427,427,427,427,427,427,427,427,427,427,427,427,427,427,427,427,427,427,
  427,427,427,427,1,207,1,427,
428,428,428,428,428,428,428,428,428,428,428,428,428,428,428,428,428,428,428,
  428,428,428,428,428,428,428,428,428,428,428,428,428,428,428,428,428,428,
  428,428,428,428,1,208,1,428,
429,429,429,429,429,429,429,429,429,429,429,429,429,429,429,429,429,429,429,
  429,429,429,429,429,429,429,429,429,429,429,429,429,429,429,429,429,429,
  429,429,429,429,1,209,1,429,
430,430,430,430,430,430,430,430,430,430,430,430,430,430,430,430,430,430,430,
  430,430,430,430,430,430,430,430,430,430,430,430,430,430,430,430,430,430,
  430,430,430,430,1,210,1,430,
431,431,431,431,431,431,431,431,431,431,431,431,431,431,431,431,431,431,431,
  431,431,431,431,431,431,431,431,431,431,431,431,431,431,431,431,431,431,
  431,431,431,431,1,211,1,431,
432,432,432,432,432,432,432,432,432,432,432,432,432,432,432,432,432,432,432,
  432,432,432,432,432,432,432,432,432,432,432,432,432,432,432,432,432,432,
  432,432,432,432,1,212,1,432,
433,433,433,433,433,433,433,433,433,433,433,433,433,433,433,433,433,433,433,
  433,433,433,433,433,433,433,433,433,433,433,433,433,433,433,433,433,433,
  433,433,433,433,1,213,1,433,
434,434,434,434,434,434,434,434,434,434,434,434,434,434,434,434,434,434,434,
  434,434,434,434,434,434,434,434,434,434,434,434,434,434,434,434,434,434,
  434,434,434,434,1,214,1,434,
435,435,435,435,435,435,435,435,435,435,435,435,435,435,435,435,435,435,435,
  435,435,435,435,435,435,435,435,435,435,435,435,435,435,435,435,435,435,
  435,435,435,435,1,215,1,435,
436,436,436,436,436,436,436,436,436,436,436,436,436,436,436,436,436,436,436,
  436,436,436,436,436,436,436,436,436,436,436,436,436,436,436,436,436,436,
  436,436,436,436,1,216,1,436,
437,437,437,437,437,437,437,437,437,437,437,437,437,437,437,437,437,437,437,
  437,437,437,437,437,437,437,437,437,437,437,437,437,437,437,437,437,437,
  437,437,437,437,1,217,1,437,
438,438,438,438,438,438,438,438,438,438,438,438,438,438,438,438,438,438,438,
  438,438,438,438,438,438,438,438,438,438,438,438,438,438,438,438,438,438,
  438,438,438,438,1,218,1,438,
439,439,439,439,439,439,439,439,439,439,439,439,439,439,439,439,439,439,439,
  439,439,439,439,439,439,439,439,439,439,439,439,439,439,439,439,439,439,
  439,439,439,439,1,219,1,439,
440,440,440,440,440,440,440,440,440,440,440,440,440,440,440,440,440,440,440,
  440,440,440,440,440,440,440,440,440,440,440,440,440,440,440,440,440,440,
  440,440,440,440,1,220,1,440,
441,441,441,441,441,441,441,441,441,441,441,441,441,441,441,441,441,441,441,
  441,441,441,441,441,441,441,441,441,441,441,441,441,441,441,441,441,441,
  441,441,441,441,1,221,1,441,
442,442,442,442,442,442,442,442,442,442,442,442,442,442,442,442,442,442,442,
  442,442,442,442,442,442,442,442,442,442,442,442,442,442,442,442,442,442,
  442,442,442,442,1,222,1,442,
443,443,443,443,443,443,443,443,443,443,443,443,443,443,443,443,443,443,443,
  443,443,443,443,443,443,443,443,443,443,443,443,443,443,443,443,443,443,
  443,443,443,443,1,223,1,443,
444,444,444,444,444,444,444,444,444,444,444,444,444,444,444,444,444,444,444,
  444,444,444,444,444,444,444,444,444,444,444,444,444,444,444,444,444,444,
  444,444,444,444,1,224,1,444,
445,445,445,445,445,445,445,445,445,445,445,445,445,445,445,445,445,445,445,
  445,445,445,445,445,445,445,445,445,445,445,445,445,445,445,445,445,445,
  445,445,445,445,1,225,1,445,
446,446,446,446,446,446,446,446,446,446,446,446,446,446,446,446,446,446,446,
  446,446,446,446,446,446,446,446,446,446,446,446,446,446,446,446,446,446,
  446,446,446,446,1,226,1,446,
447,447,447,447,447,447,447,447,447,447,447,447,447,447,447,447,447,447,447,
  447,447,447,447,447,447,447,447,447,447,447,447,447,447,447,447,447,447,
  447,447,447,447,1,227,1,447,
448,448,448,448,448,448,448,448,448,448,448,448,448,448,448,448,448,448,448,
  448,448,448,448,448,448,448,448,448,448,448,448,448,448,448,448,448,448,
  448,448,448,448,1,228,1,448,
449,449,449,449,449,449,449,449,449,449,449,449,449,449,449,449,449,449,449,
  449,449,449,449,449,449,449,449,449,449,449,449,449,449,449,449,449,449,
  449,449,449,449,1,229,1,449,
450,450,450,450,450,450,450,450,450,450,450,450,450,450,450,450,450,450,450,
  450,450,450,450,450,450,450,450,450,450,450,450,450,450,450,450,450,450,
  450,450,450,450,1,230,1,450,
451,451,451,451,451,451,451,451,451,451,451,451,451,451,451,451,451,451,451,
  451,451,451,451,451,451,451,451,451,451,451,451,451,451,451,451,451,451,
  451,451,451,451,1,231,1,451,
452,452,452,452,452,452,452,452,452,452,452,452,452,452,452,452,452,452,452,
  452,452,452,452,452,452,452,452,452,452,452,452,452,452,452,452,452,452,
  452,452,452,452,1,232,1,452,
453,453,453,453,453,453,453,453,453,453,453,453,453,453,453,453,453,453,453,
  453,453,453,453,453,453,453,453,453,453,453,453,453,453,453,453,453,453,
  453,453,453,453,1,233,1,453,
454,454,454,454,454,454,454,454,454,454,454,454,454,454,454,454,454,454,454,
  454,454,454,454,454,454,454,454,454,454,454,454,454,454,454,454,454,454,
  454,454,454,454,1,234,1,454,
455,455,455,455,455,455,455,455,455,455,455,455,455,455,455,455,455,455,455,
  455,455,455,455,455,455,455,455,455,455,455,455,455,455,455,455,455,455,
  455,455,455,455,1,235,1,455,
456,456,456,456,456,456,456,456,456,456,456,456,456,456,456,456,456,456,456,
  456,456,456,456,456,456,456,456,456,456,456,456,456,456,456,456,456,456,
  456,456,456,456,1,236,1,456,
457,457,457,457,457,457,457,457,457,457,457,457,457,457,457,457,457,457,457,
  457,457,457,457,457,457,457,457,457,457,457,457,457,457,457,457,457,457,
  457,457,457,457,1,237,1,457,
458,458,458,458,458,458,458,458,458,458,458,458,458,458,458,458,458,458,458,
  458,458,458,458,458,458,458,458,458,458,458,458,458,458,458,458,458,458,
  458,458,458,458,1,238,1,458,
459,459,459,459,459,459,459,459,459,459,459,459,459,459,459,459,459,459,459,
  459,459,459,459,459,459,459,459,459,459,459,459,459,459,459,459,459,459,
  459,459,459,459,1,239,1,459,
460,460,460,460,460,460,460,460,460,460,460,460,460,460,460,460,460,460,460,
  460,460,460,460,460,460,460,460,460,460,460,460,460,460,460,460,460,460,
  460,460,460,460,1,240,1,460,
461,461,461,461,461,461,461,461,461,461,461,461,461,461,461,461,461,461,461,
  461,461,461,461,461,461,461,461,461,461,461,461,461,461,461,461,461,461,
  461,461,461,461,1,241,1,461,
462,462,462,462,462,462,462,462,462,462,462,462,462,462,462,462,462,462,462,
  462,462,462,462,462,462,462,462,462,462,462,462,462,462,462,462,462,462,
  462,462,462,462,1,242,1,462,
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
20,137,137,137,137,137,137,137,137,137,137,137,20,137,20,20,138,138,138,138,
  138,20,20,20,20,20,20,20,20,20,20,20,20,20,20,20,20,20,20,20,20,20,20,
  20,20,20,20,20,20,20,137,20,20,20,20,20,1,265,1,894,138,
20,20,20,20,20,20,1,266,1,316,
66,66,66,66,66,66,66,66,66,66,66,66,66,66,66,66,66,66,66,66,66,66,66,66,66,
  66,66,66,66,66,66,66,66,66,66,66,66,66,66,66,66,66,66,66,66,66,66,66,66,
  66,66,66,66,66,66,66,66,66,309,66,66,66,66,66,66,66,66,66,66,267,485,
122,122,122,122,122,486,268,486,
487,121,488,
136,136,136,136,136,136,136,136,136,136,136,285,136,256,256,256,256,287,289,
  296,298,299,300,301,302,303,304,305,306,321,323,324,325,286,322,288,297,
  309,284,270,135,270,240,294,295,241,290,242,291,267,335,334,332,337,333,
  336,331,338,110,225,326,311,265,226,339,327,331,330,329,328,224,331,320,
  319,318,317,316,315,314,313,312,310,293,292,265,308,307,
297,489,491,493,494,495,497,498,499,503,506,508,510,512,514,516,518,519,521,
  522,523,525,526,528,529,530,531,534,535,14,15,16,343,345,347,349,351,
  271,75,76,533,77,79,505,502,501,70,71,532,532,532,532,84,527,527,527,
  524,524,524,524,520,520,520,517,515,513,511,509,507,504,500,113,114,496,
  117,118,492,490,123,
536,536,536,536,536,1,272,1,536,
537,537,537,537,537,537,537,537,537,537,537,537,537,537,537,537,537,537,537,
  537,537,537,537,537,537,537,537,537,537,537,537,537,537,537,537,537,537,
  537,537,537,1,273,1,537,
538,538,538,538,538,1,274,1,538,
145,275,539,278,279,
145,276,540,278,279,
136,136,136,136,136,136,136,136,136,136,136,285,136,256,256,256,256,287,289,
  145,296,298,299,300,301,302,303,304,305,306,321,323,324,325,286,322,288,
  297,309,284,270,135,277,240,294,295,241,290,542,278,242,291,267,335,334,
  332,337,333,336,331,541,225,326,311,265,226,327,331,330,329,328,224,331,
  320,319,318,317,316,315,314,313,312,310,293,292,279,265,308,307,
148,543,148,148,148,148,148,148,148,148,148,148,148,148,148,148,148,148,148,
  148,148,148,144,148,148,148,148,148,148,148,148,148,148,148,148,148,148,
  148,148,148,148,148,278,146,
20,20,20,20,20,20,20,20,20,279,1,887,
544,544,544,544,544,1,280,1,544,
43,44,45,40,57,281,57,57,48,57,47,46,46,
43,44,45,40,56,282,56,56,48,56,47,46,46,
545,545,545,545,545,1,283,1,545,
281,281,281,281,281,284,
266,266,266,266,285,
264,264,286,
256,546,256,256,256,256,256,256,256,256,256,256,256,256,256,256,256,256,256,
  256,256,256,256,256,256,256,256,256,256,256,256,256,256,256,256,256,256,
  256,256,256,256,256,256,256,256,256,256,256,256,256,256,256,256,256,287,
254,254,254,254,254,288,
271,547,271,271,271,271,271,271,271,271,271,271,271,271,271,271,271,271,271,
  271,271,271,271,271,271,271,271,271,271,271,271,271,271,271,271,271,271,
  271,271,271,277,271,289,548,
282,282,282,282,282,279,
247,263,263,263,263,263,263,263,263,263,291,
267,267,267,267,246,246,246,246,246,246,246,246,246,246,246,246,246,246,246,
  246,246,246,246,246,246,246,246,246,246,246,246,246,246,246,246,246,246,
  246,246,246,246,246,246,292,
265,265,245,245,245,245,245,245,245,245,245,245,245,245,245,245,245,245,245,
  245,245,245,245,245,245,245,245,245,245,245,245,245,245,245,245,245,245,
  245,245,245,245,293,
261,261,261,261,261,261,261,261,261,244,244,244,244,244,244,244,244,244,244,
  244,244,244,244,244,244,244,244,244,244,244,244,244,244,244,244,244,244,
  244,244,244,244,244,244,244,244,244,244,244,294,
258,248,262,549,550,262,257,257,257,257,257,243,243,243,243,243,243,243,243,
  243,243,243,243,243,243,243,243,243,243,243,243,243,243,243,243,243,243,
  243,243,243,243,243,280,243,243,243,243,252,252,243,243,295,252,
20,20,296,1,990,
20,20,20,20,20,20,20,297,1,943,
20,20,298,1,988,
20,20,299,1,987,
20,20,300,1,986,
20,20,301,1,985,
20,20,302,1,984,
20,20,303,1,983,
20,20,304,1,982,
20,20,305,1,981,
20,20,306,1,980,
20,20,20,20,20,20,20,20,20,20,20,20,20,20,20,20,20,20,20,20,20,20,20,20,20,
  20,20,20,20,20,20,20,20,20,20,20,20,20,20,307,1,991,
20,20,20,20,20,20,20,20,20,20,20,20,20,20,20,20,20,20,20,20,20,20,20,20,20,
  20,20,20,20,20,20,20,20,20,20,20,20,20,1,308,1,944,
20,20,20,20,20,20,20,20,20,20,20,20,20,20,20,20,20,20,20,20,20,20,20,20,20,
  20,20,20,20,20,20,20,20,20,20,20,20,20,20,20,20,20,20,20,20,20,20,20,20,
  20,20,20,20,20,20,20,20,20,20,20,20,20,309,1,912,
551,1,310,1,551,
552,1,311,1,552,
553,1,312,1,553,
554,1,313,1,554,
555,1,314,1,555,
556,1,315,1,556,
557,1,316,1,557,
558,1,317,1,558,
559,1,318,1,559,
560,1,319,1,560,
561,1,320,1,561,
260,260,260,260,260,260,260,260,260,20,20,20,20,20,20,20,20,20,20,20,20,20,
  20,20,20,20,20,20,20,20,20,20,20,20,20,20,20,20,20,20,20,20,20,20,20,20,
  321,1,979,
20,20,20,20,20,20,20,20,20,20,20,20,20,255,255,255,255,255,20,20,20,20,20,
  20,20,20,20,20,20,20,20,20,20,20,20,20,20,20,1,322,1,970,
20,20,20,20,20,20,20,20,20,20,20,20,20,20,20,20,20,20,20,20,20,20,20,20,20,
  20,20,20,20,20,20,20,20,20,20,20,20,20,20,20,20,20,20,20,20,20,20,20,20,
  20,20,20,20,20,20,20,20,20,20,20,20,323,1,978,
20,20,20,20,20,20,20,20,20,20,20,20,20,20,20,20,20,20,20,20,20,20,20,20,20,
  20,20,20,20,20,20,20,20,20,20,20,20,20,1,324,1,977,
20,20,20,20,20,20,20,20,20,20,20,20,20,20,20,20,20,20,20,20,20,20,20,20,20,
  20,20,20,20,20,20,20,20,20,20,20,20,20,20,20,20,20,20,20,20,20,20,20,20,
  20,20,20,20,20,20,20,20,20,20,20,20,325,1,976,
562,562,562,562,562,562,562,562,562,562,562,562,562,562,562,562,562,562,562,
  562,562,562,562,562,562,562,562,562,562,562,562,562,562,562,562,562,562,
  562,562,562,562,1,326,1,562,
136,136,136,136,136,136,136,136,136,136,136,285,136,256,256,256,256,287,289,
  296,298,299,300,301,302,303,304,305,306,321,286,563,288,297,309,284,270,
  135,327,240,294,295,241,290,242,291,267,225,326,311,265,226,223,224,223,
  320,319,318,317,316,315,314,313,312,310,293,292,265,308,307,
136,136,136,136,136,136,136,136,136,136,136,285,136,256,256,256,256,287,289,
  296,298,299,300,301,302,303,304,305,306,321,286,563,288,297,309,284,270,
  135,328,240,294,295,241,290,242,291,267,225,326,311,265,226,222,224,222,
  320,319,318,317,316,315,314,313,312,310,293,292,265,308,307,
136,136,136,136,136,136,136,136,136,136,136,285,136,256,256,256,256,287,289,
  296,298,299,300,301,302,303,304,305,306,321,286,563,288,297,309,284,270,
  135,329,240,294,295,241,290,242,291,267,225,326,311,265,226,221,224,221,
  320,319,318,317,316,315,314,313,312,310,293,292,265,308,307,
136,136,136,136,136,136,136,136,136,136,136,285,136,256,256,256,256,287,289,
  296,298,299,300,301,302,303,304,305,306,321,286,563,288,297,309,284,270,
  135,330,240,294,295,241,290,242,291,267,225,326,311,265,226,220,224,220,
  320,319,318,317,316,315,314,313,312,310,293,292,265,308,307,
20,20,20,20,20,20,20,20,20,20,20,20,20,20,20,20,20,20,20,20,20,20,20,20,20,
  20,20,20,20,20,20,20,20,20,20,20,20,20,1,331,1,213,
20,564,566,391,383,387,20,20,20,20,20,20,20,20,20,20,20,20,20,20,20,20,20,
  20,20,20,20,20,20,20,20,20,20,20,20,20,1,332,1,210,570,569,568,567,565,
20,384,386,20,20,20,20,20,20,20,20,20,20,20,20,20,20,20,20,20,20,20,20,20,
  20,20,20,20,20,20,20,20,20,1,333,1,205,572,571,
20,573,575,577,579,20,20,20,20,20,20,20,20,20,20,20,20,20,20,20,20,20,20,20,
  20,20,20,20,20,20,20,1,334,1,202,580,578,576,574,
20,581,390,20,20,20,20,20,20,20,20,20,20,20,20,20,20,20,20,20,20,20,20,20,
  20,20,20,1,335,1,199,583,582,
20,584,362,20,20,20,20,20,20,20,20,20,20,20,20,20,20,20,20,20,20,20,20,20,
  20,1,336,1,196,586,585,
587,360,195,589,588,
590,591,592,593,594,595,596,597,598,599,600,601,399,175,175,175,175,175,175,
  338,179,179,179,179,182,182,182,185,185,185,188,188,188,191,191,191,194,
  194,194,
136,136,136,136,136,136,136,136,136,136,136,285,136,256,256,256,256,287,289,
  296,298,299,300,301,302,303,304,305,306,321,323,324,325,286,322,288,297,
  309,284,270,135,339,240,294,295,241,290,242,291,267,335,334,332,337,333,
  336,331,174,225,326,311,265,226,327,331,330,329,328,224,331,320,319,318,
  317,316,315,314,313,312,310,293,292,265,308,307,
602,602,602,602,602,1,340,1,602,
603,603,603,603,603,1,341,1,603,
604,604,604,604,604,604,342,604,
20,20,20,20,20,20,343,1,897,
605,605,605,605,605,1,344,1,605,
20,20,20,20,20,20,345,1,896,
606,606,606,606,606,1,346,1,606,
20,20,20,20,20,20,20,20,20,20,20,20,20,20,347,1,895,
607,607,607,607,607,607,607,607,607,607,607,607,607,1,348,1,607,
20,20,20,20,20,20,349,1,893,
608,608,608,608,608,1,350,1,608,
20,20,20,20,20,20,351,1,891,
609,609,609,609,609,1,352,1,609,
164,164,610,164,164,164,164,164,164,164,164,164,164,164,164,162,164,164,164,
  164,164,164,164,164,164,164,164,164,164,164,164,164,164,164,164,164,164,
  164,164,164,164,353,
20,20,20,20,20,20,354,1,888,
611,611,611,611,611,1,355,1,611,
612,612,612,612,612,1,356,1,612,
20,20,20,20,20,20,20,20,20,20,20,20,20,20,20,20,20,20,20,20,20,1,357,1,886,
139,139,139,139,139,139,139,139,139,139,139,139,163,139,139,139,139,139,145,
  139,139,358,616,278,615,353,614,279,354,613,
20,20,20,20,20,20,20,20,20,20,20,20,20,20,20,20,20,20,20,20,20,20,20,20,20,
  20,20,20,20,20,20,20,20,20,20,20,20,20,20,20,20,20,20,20,20,20,20,20,
  359,1,1157,
20,20,20,20,20,20,20,20,20,20,20,20,20,20,20,20,20,20,20,20,20,20,20,20,20,
  20,20,20,20,20,20,20,20,20,20,20,20,20,20,20,20,20,20,20,20,20,20,20,20,
  20,20,20,20,20,20,20,20,20,20,20,20,20,360,1,959,
20,20,20,20,20,20,20,20,20,20,20,20,20,20,20,20,20,20,20,20,20,20,20,20,20,
  20,20,20,20,20,20,20,20,20,20,20,20,20,20,20,20,20,20,20,20,20,20,20,
  361,1,1156,
20,20,20,20,20,20,20,20,20,20,20,20,20,20,20,20,20,20,20,20,20,20,20,20,20,
  20,20,20,20,20,20,20,20,20,20,20,20,20,20,20,20,20,20,20,20,20,20,20,20,
  20,20,20,20,20,20,20,20,20,20,20,20,20,362,1,961,
20,20,20,20,20,20,20,20,20,20,20,20,20,20,20,20,20,20,20,20,20,20,20,20,20,
  20,20,20,20,20,20,20,20,20,20,20,20,20,20,20,20,20,20,20,20,363,1,1155,
20,20,20,20,20,20,20,20,20,20,20,20,20,20,20,20,20,20,20,20,20,20,20,20,20,
  20,20,20,20,20,20,20,20,20,20,20,20,20,20,20,20,20,20,20,20,20,20,20,
  364,1,1154,
20,20,20,20,20,20,20,20,20,20,20,20,20,20,20,20,20,20,20,20,20,20,20,20,20,
  20,20,20,20,20,20,20,20,20,20,20,20,20,20,20,20,20,20,20,20,20,20,20,
  365,1,1153,
20,20,20,20,20,20,20,20,20,20,20,20,20,20,20,20,20,20,20,20,20,20,20,20,20,
  20,20,20,20,20,20,20,20,20,20,20,20,20,20,20,20,20,20,20,20,20,20,20,
  366,1,1152,
20,20,20,20,20,20,20,20,20,20,20,20,20,20,20,20,20,20,20,20,20,20,20,20,20,
  20,20,20,20,20,20,20,20,20,20,20,20,20,20,20,20,20,20,20,20,20,20,20,
  367,1,1151,
20,20,20,20,20,20,20,20,20,20,20,20,20,20,20,20,20,20,20,20,20,20,20,20,20,
  20,20,20,20,20,20,20,20,20,20,20,20,20,20,20,20,20,20,20,20,20,20,20,
  368,1,1150,
20,20,20,20,20,20,20,20,20,20,20,20,20,20,20,20,20,20,20,20,20,20,20,20,20,
  20,20,20,20,20,20,20,20,20,20,20,20,20,20,20,20,20,20,20,20,20,20,20,
  369,1,1149,
20,20,20,20,20,20,20,20,20,20,20,20,20,20,20,20,20,20,20,20,20,20,20,20,20,
  20,20,20,20,20,20,20,20,20,20,20,20,20,20,20,20,20,20,20,20,20,20,20,
  370,1,1148,
20,20,20,20,20,20,20,20,20,20,20,20,20,20,20,20,20,20,20,20,20,20,20,20,20,
  20,20,20,20,20,20,20,20,20,20,20,20,20,20,20,20,20,20,20,20,20,20,20,
  371,1,1147,
20,20,20,20,20,20,20,20,20,20,20,20,20,20,20,20,20,20,20,20,20,20,20,20,20,
  20,20,20,20,20,20,20,20,20,20,20,20,20,20,20,20,20,20,20,20,20,20,20,
  372,1,1146,
20,20,20,20,20,20,20,20,20,20,20,20,20,20,20,20,20,20,20,20,20,20,20,20,20,
  20,20,20,20,20,20,20,20,20,20,20,20,20,20,20,20,20,20,20,20,20,20,20,
  373,1,1145,
20,20,20,20,20,20,20,20,20,20,20,20,20,20,20,20,20,20,20,20,20,20,20,20,20,
  20,20,20,20,20,20,20,20,20,20,20,20,20,20,20,20,20,20,20,20,20,20,20,
  374,1,1144,
20,20,20,20,20,20,20,20,20,20,20,20,20,20,20,20,20,20,20,20,20,20,20,20,20,
  20,20,20,20,20,20,20,20,20,20,20,20,20,20,20,20,20,20,20,20,20,20,20,
  375,1,1143,
20,20,20,20,20,20,20,20,20,20,20,20,20,20,20,20,20,20,20,20,20,20,20,20,20,
  20,20,20,20,20,20,20,20,20,20,20,20,20,20,20,20,20,20,20,20,20,20,20,
  376,1,1142,
20,20,20,20,20,20,20,20,20,20,20,20,20,20,20,20,20,20,20,20,20,20,20,20,20,
  20,20,20,20,20,20,20,20,20,20,20,20,20,20,20,20,20,20,20,20,20,20,20,
  377,1,1141,
20,20,20,20,20,20,20,20,20,20,20,20,20,20,20,20,20,20,20,20,20,20,20,20,20,
  20,20,20,20,20,20,20,20,20,20,20,20,20,20,20,20,20,20,20,20,20,20,20,
  378,1,1140,
20,20,20,20,20,20,20,20,20,20,20,20,20,20,20,20,20,20,20,20,20,20,20,20,20,
  20,20,20,20,20,20,20,20,20,20,20,20,20,20,20,20,20,20,20,20,20,20,20,
  379,1,1139,
20,20,20,20,20,20,20,20,20,20,20,20,20,20,20,20,20,20,20,20,20,20,20,20,20,
  20,20,20,20,20,20,20,20,20,20,20,20,20,20,20,20,20,20,20,20,20,20,20,
  380,1,1138,
20,20,20,20,20,20,20,20,20,20,20,20,20,20,20,20,20,20,20,20,20,20,20,20,20,
  20,20,20,20,20,20,20,20,20,20,20,20,20,20,20,20,20,20,20,20,20,20,20,
  381,1,1137,
20,20,20,20,20,20,20,20,20,20,20,20,20,20,20,20,20,20,20,20,20,20,20,20,20,
  20,20,20,20,20,20,20,20,20,20,20,20,20,20,20,20,20,20,20,20,20,20,20,
  382,1,1136,
20,20,20,20,20,20,20,20,20,20,20,20,20,20,20,20,20,20,20,20,20,20,20,20,20,
  20,20,20,20,20,20,20,20,20,20,20,20,20,20,20,20,20,20,20,20,20,20,20,20,
  20,20,20,20,20,20,20,20,20,20,20,20,383,1,972,
20,20,20,20,20,20,20,20,20,20,20,20,20,20,20,20,20,20,20,20,20,20,20,20,20,
  20,20,20,20,20,20,20,20,20,20,20,20,20,20,20,20,20,20,20,20,20,20,20,20,
  20,20,20,20,20,20,20,20,20,20,20,20,20,384,1,970,
20,20,20,20,20,20,20,20,20,20,20,20,20,20,20,20,20,20,20,20,20,20,20,20,20,
  20,20,20,20,20,20,20,20,20,20,20,20,20,20,20,20,20,20,20,20,20,20,20,20,
  20,20,20,20,20,20,20,20,20,20,20,20,20,385,1,907,
20,20,20,20,20,20,20,20,20,20,20,20,20,20,20,20,20,20,20,20,20,20,20,20,20,
  20,20,20,20,20,20,20,20,20,20,20,20,20,20,20,20,20,20,20,20,20,20,20,20,
  20,20,20,20,20,20,20,20,20,20,20,20,20,386,1,969,
20,20,20,20,20,20,20,20,20,20,20,20,20,20,20,20,20,20,20,20,20,20,20,20,20,
  20,20,20,20,20,20,20,20,20,20,20,20,20,20,20,20,20,20,20,20,20,20,20,20,
  20,20,20,20,20,20,20,20,20,20,20,20,387,1,971,
20,20,20,20,20,20,20,20,20,20,20,20,20,20,20,20,20,20,20,20,20,20,20,20,20,
  20,20,20,20,20,20,20,20,20,20,20,20,20,20,20,20,20,20,20,20,20,20,20,20,
  20,20,20,20,20,20,20,20,20,20,20,20,20,20,20,20,20,20,20,20,20,20,20,
  388,1,911,
20,20,20,20,20,20,20,20,20,20,20,20,20,20,20,20,20,20,20,20,20,20,20,20,20,
  20,20,20,20,20,20,20,20,20,20,20,20,20,20,20,20,20,20,20,20,20,20,20,
  389,1,1135,
20,20,20,20,20,20,20,20,20,20,20,20,20,20,20,20,20,20,20,20,20,20,20,20,20,
  20,20,20,20,20,20,20,20,20,20,20,20,20,20,20,20,20,20,20,20,20,20,20,20,
  20,20,20,20,20,20,20,20,20,20,20,20,20,390,1,963,
20,20,20,20,20,20,20,20,20,20,20,20,20,20,20,20,20,20,20,20,20,20,20,20,20,
  20,20,20,20,20,20,20,20,20,20,20,20,20,20,20,20,20,20,20,20,20,20,20,20,
  20,20,20,20,20,20,20,20,20,20,20,20,391,1,973,
20,20,20,20,20,20,20,20,20,20,20,20,20,20,20,20,20,20,20,20,20,20,20,20,20,
  20,20,20,20,20,20,20,20,20,20,20,20,20,20,20,20,20,20,20,20,20,20,20,
  392,1,979,
20,20,20,20,20,20,20,20,20,20,20,20,20,20,20,20,20,20,20,20,20,20,20,20,20,
  20,20,20,20,20,20,20,20,20,20,20,20,20,20,20,20,20,20,20,20,20,20,20,
  393,1,1134,
20,20,20,20,20,20,20,20,20,20,20,20,20,20,20,20,20,20,20,20,20,20,20,20,20,
  20,20,20,20,20,20,20,20,20,20,20,20,20,20,20,20,20,20,20,20,20,20,20,
  394,1,1133,
20,20,20,20,20,20,20,20,20,20,20,20,20,20,20,20,20,20,20,20,20,20,20,20,20,
  20,20,20,20,20,20,20,20,20,20,20,20,20,20,20,20,20,20,20,20,20,20,20,20,
  20,20,20,20,20,395,1,878,
359,361,363,364,365,366,367,368,369,370,371,372,373,374,375,376,377,378,379,
  380,381,382,389,393,394,392,323,325,391,383,387,384,386,390,362,360,309,
  388,385,41,44,7,9,7,7,7,7,7,7,7,7,7,7,7,7,7,7,7,7,7,7,7,7,7,7,7,7,7,7,7,
  7,7,7,7,7,7,7,7,7,7,7,7,7,7,
40,397,11,
40,398,10,
20,20,20,20,20,20,20,20,20,20,20,20,20,20,20,20,20,20,20,20,20,20,20,20,20,
  20,20,20,20,20,20,20,20,20,20,20,20,20,20,20,20,1,399,1,884,
136,136,136,136,136,136,136,136,136,136,136,285,136,256,256,256,256,287,289,
  296,298,299,300,301,302,303,304,305,306,321,323,324,325,286,322,288,297,
  309,284,270,135,400,240,294,295,241,290,242,291,267,335,334,332,337,333,
  336,331,617,225,326,311,265,226,327,331,330,329,328,224,331,320,319,318,
  317,316,315,314,313,312,310,293,292,265,308,307,
297,489,491,493,494,495,497,498,499,503,506,508,510,512,514,516,518,519,521,
  522,523,525,526,528,529,530,531,534,535,14,15,16,343,345,347,349,351,
  618,620,401,621,619,75,76,533,77,79,505,502,501,70,71,532,532,532,532,
  84,527,527,527,524,524,524,524,520,520,520,517,515,513,511,509,507,504,
  500,113,114,496,117,118,492,490,123,
622,622,622,622,622,1,402,1,622,
623,297,43,44,45,40,72,403,624,72,72,48,72,47,46,46,172,173,
310,625,626,627,312,311,312,311,310,404,310,311,312,315,402,313,314,
292,291,291,288,287,286,285,284,283,405,400,291,628,
295,302,301,298,301,298,295,406,398,295,298,301,629,
404,404,404,404,407,404,
385,408,630,
385,409,631,
385,410,632,
136,136,136,136,136,136,136,136,136,136,136,285,136,256,256,256,256,287,289,
  296,298,299,300,301,302,303,304,305,306,321,323,324,325,286,322,288,297,
  309,284,270,135,411,240,294,295,241,290,242,291,267,335,334,332,337,333,
  336,331,472,225,326,311,265,226,327,331,330,329,328,224,331,320,319,318,
  317,316,315,314,313,312,310,293,292,265,308,307,
136,136,136,136,136,136,136,136,136,136,136,285,136,256,256,256,256,287,289,
  296,298,299,300,301,302,303,304,305,306,321,323,324,325,286,322,288,297,
  309,284,270,135,412,240,294,295,241,290,242,291,267,335,334,332,337,333,
  336,331,471,225,326,311,265,226,327,331,330,329,328,224,331,320,319,318,
  317,316,315,314,313,312,310,293,292,265,308,307,
136,136,136,136,136,136,136,136,136,136,136,285,136,256,256,256,256,287,289,
  296,298,299,300,301,302,303,304,305,306,321,323,324,325,286,322,288,297,
  309,284,270,135,413,240,294,295,241,290,242,291,267,335,334,332,337,333,
  336,331,470,225,326,311,265,226,327,331,330,329,328,224,331,320,319,318,
  317,316,315,314,313,312,310,293,292,265,308,307,
136,136,136,136,136,136,136,136,136,136,136,285,136,256,256,256,256,287,289,
  296,298,299,300,301,302,303,304,305,306,321,323,324,325,286,322,288,297,
  309,284,270,135,414,240,294,295,241,290,242,291,267,335,334,332,337,333,
  336,331,469,225,326,311,265,226,327,331,330,329,328,224,331,320,319,318,
  317,316,315,314,313,312,310,293,292,265,308,307,
136,136,136,136,136,136,136,136,136,136,136,285,136,256,256,256,256,287,289,
  296,298,299,300,301,302,303,304,305,306,321,323,324,325,286,322,288,297,
  309,284,270,135,415,240,294,295,241,290,242,291,267,335,334,332,337,333,
  336,331,468,225,326,311,265,226,327,331,330,329,328,224,331,320,319,318,
  317,316,315,314,313,312,310,293,292,265,308,307,
136,136,136,136,136,136,136,136,136,136,136,285,136,256,256,256,256,287,289,
  296,298,299,300,301,302,303,304,305,306,321,323,324,325,286,322,288,297,
  309,284,270,135,416,240,294,295,241,290,242,291,267,335,334,332,337,333,
  336,331,467,225,326,311,265,226,327,331,330,329,328,224,331,320,319,318,
  317,316,315,314,313,312,310,293,292,265,308,307,
136,136,136,136,136,136,136,136,136,136,136,285,136,256,256,256,256,287,289,
  296,298,299,300,301,302,303,304,305,306,321,323,324,325,286,322,288,297,
  309,284,270,135,417,240,294,295,241,290,242,291,267,335,334,332,337,333,
  336,331,466,225,326,311,265,226,327,331,330,329,328,224,331,320,319,318,
  317,316,315,314,313,312,310,293,292,265,308,307,
136,136,136,136,136,136,136,136,136,136,136,285,136,256,256,256,256,287,289,
  296,298,299,300,301,302,303,304,305,306,321,323,324,325,286,322,288,297,
  309,284,270,135,418,240,294,295,241,290,242,291,267,335,334,332,337,333,
  336,331,465,225,326,311,265,226,327,331,330,329,328,224,331,320,319,318,
  317,316,315,314,313,312,310,293,292,265,308,307,
136,136,136,136,136,136,136,136,136,136,136,285,136,256,256,256,256,287,289,
  296,298,299,300,301,302,303,304,305,306,321,323,324,325,286,322,288,297,
  309,284,270,135,419,240,294,295,241,290,242,291,267,335,334,332,337,333,
  336,331,464,225,326,311,265,226,327,331,330,329,328,224,331,320,319,318,
  317,316,315,314,313,312,310,293,292,265,308,307,
136,136,136,136,136,136,136,136,136,136,136,285,136,256,256,256,256,287,289,
  296,298,299,300,301,302,303,304,305,306,321,323,324,325,286,322,288,297,
  309,284,270,135,420,240,294,295,241,290,242,291,267,335,334,332,337,333,
  336,331,463,225,326,311,265,226,327,331,330,329,328,224,331,320,319,318,
  317,316,315,314,313,312,310,293,292,265,308,307,
136,136,136,136,136,136,136,136,136,136,136,285,136,256,256,256,256,287,289,
  296,298,299,300,301,302,303,304,305,306,321,323,324,325,286,322,288,297,
  309,284,270,135,421,240,294,295,241,290,242,291,267,335,334,332,337,333,
  336,331,462,225,326,311,265,226,327,331,330,329,328,224,331,320,319,318,
  317,316,315,314,313,312,310,293,292,265,308,307,
136,136,136,136,136,136,136,136,136,136,136,285,136,256,256,256,256,287,289,
  296,298,299,300,301,302,303,304,305,306,321,323,324,325,286,322,288,297,
  309,284,270,135,422,240,294,295,241,290,242,291,267,335,334,332,337,333,
  336,331,461,225,326,311,265,226,327,331,330,329,328,224,331,320,319,318,
  317,316,315,314,313,312,310,293,292,265,308,307,
136,136,136,136,136,136,136,136,136,136,136,285,136,256,256,256,256,287,289,
  296,298,299,300,301,302,303,304,305,306,321,323,324,325,286,322,288,297,
  309,284,270,135,423,240,294,295,241,290,242,291,267,335,334,332,337,333,
  336,331,460,225,326,311,265,226,327,331,330,329,328,224,331,320,319,318,
  317,316,315,314,313,312,310,293,292,265,308,307,
136,136,136,136,136,136,136,136,136,136,136,285,136,256,256,256,256,287,289,
  296,298,299,300,301,302,303,304,305,306,321,323,324,325,286,322,288,297,
  309,284,270,135,424,240,294,295,241,290,242,291,267,335,334,332,337,333,
  336,331,459,225,326,311,265,226,327,331,330,329,328,224,331,320,319,318,
  317,316,315,314,313,312,310,293,292,265,308,307,
136,136,136,136,136,136,136,136,136,136,136,285,136,256,256,256,256,287,289,
  296,298,299,300,301,302,303,304,305,306,321,323,324,325,286,322,288,297,
  309,284,270,135,425,240,294,295,241,290,242,291,267,335,334,332,337,333,
  336,331,458,225,326,311,265,226,327,331,330,329,328,224,331,320,319,318,
  317,316,315,314,313,312,310,293,292,265,308,307,
136,136,136,136,136,136,136,136,136,136,136,285,136,256,256,256,256,287,289,
  296,298,299,300,301,302,303,304,305,306,321,323,324,325,286,322,288,297,
  309,284,270,135,426,240,294,295,241,290,242,291,267,335,334,332,337,333,
  336,331,457,225,326,311,265,226,327,331,330,329,328,224,331,320,319,318,
  317,316,315,314,313,312,310,293,292,265,308,307,
136,136,136,136,136,136,136,136,136,136,136,285,136,256,256,256,256,287,289,
  296,298,299,300,301,302,303,304,305,306,321,323,324,325,286,322,288,297,
  309,284,270,135,427,240,294,295,241,290,242,291,267,335,334,332,337,333,
  336,331,456,225,326,311,265,226,327,331,330,329,328,224,331,320,319,318,
  317,316,315,314,313,312,310,293,292,265,308,307,
136,136,136,136,136,136,136,136,136,136,136,285,136,256,256,256,256,287,289,
  296,298,299,300,301,302,303,304,305,306,321,323,324,325,286,322,288,297,
  309,284,270,135,428,240,294,295,241,290,242,291,267,335,334,332,337,333,
  336,331,455,225,326,311,265,226,327,331,330,329,328,224,331,320,319,318,
  317,316,315,314,313,312,310,293,292,265,308,307,
136,136,136,136,136,136,136,136,136,136,136,285,136,256,256,256,256,287,289,
  296,298,299,300,301,302,303,304,305,306,321,323,324,325,286,322,288,297,
  309,284,270,135,429,240,294,295,241,290,242,291,267,335,334,332,337,333,
  336,331,454,225,326,311,265,226,327,331,330,329,328,224,331,320,319,318,
  317,316,315,314,313,312,310,293,292,265,308,307,
136,136,136,136,136,136,136,136,136,136,136,285,136,256,256,256,256,287,289,
  296,298,299,300,301,302,303,304,305,306,321,323,324,325,286,322,288,297,
  309,284,270,135,430,240,294,295,241,290,242,291,267,335,334,332,337,333,
  336,331,453,225,326,311,265,226,327,331,330,329,328,224,331,320,319,318,
  317,316,315,314,313,312,310,293,292,265,308,307,
136,136,136,136,136,136,136,136,136,136,136,285,136,256,256,256,256,287,289,
  296,298,299,300,301,302,303,304,305,306,321,323,324,325,286,322,288,297,
  309,284,270,135,431,240,294,295,241,290,242,291,267,335,334,332,337,333,
  336,331,452,225,326,311,265,226,327,331,330,329,328,224,331,320,319,318,
  317,316,315,314,313,312,310,293,292,265,308,307,
136,136,136,136,136,136,136,136,136,136,136,285,136,256,256,256,256,287,289,
  296,298,299,300,301,302,303,304,305,306,321,323,324,325,286,322,288,297,
  309,284,270,135,432,240,294,295,241,290,242,291,267,335,334,332,337,333,
  336,331,451,225,326,311,265,226,327,331,330,329,328,224,331,320,319,318,
  317,316,315,314,313,312,310,293,292,265,308,307,
136,136,136,136,136,136,136,136,136,136,136,285,136,256,256,256,256,287,289,
  296,298,299,300,301,302,303,304,305,306,321,323,324,325,286,322,288,297,
  309,284,270,135,433,240,294,295,241,290,242,291,267,335,334,332,337,333,
  336,331,450,225,326,311,265,226,327,331,330,329,328,224,331,320,319,318,
  317,316,315,314,313,312,310,293,292,265,308,307,
136,136,136,136,136,136,136,136,136,136,136,285,136,256,256,256,256,287,289,
  296,298,299,300,301,302,303,304,305,306,321,323,324,325,286,322,288,297,
  309,284,270,135,434,240,294,295,241,290,242,291,267,335,334,332,337,333,
  336,331,449,225,326,311,265,226,327,331,330,329,328,224,331,320,319,318,
  317,316,315,314,313,312,310,293,292,265,308,307,
136,136,136,136,136,136,136,136,136,136,136,285,136,256,256,256,256,287,289,
  296,298,299,300,301,302,303,304,305,306,321,323,324,325,286,322,288,297,
  309,284,270,135,435,240,294,295,241,290,242,291,267,335,334,332,337,333,
  336,331,448,225,326,311,265,226,327,331,330,329,328,224,331,320,319,318,
  317,316,315,314,313,312,310,293,292,265,308,307,
136,136,136,136,136,136,136,136,136,136,136,285,136,256,256,256,256,287,289,
  296,298,299,300,301,302,303,304,305,306,321,323,324,325,286,322,288,297,
  309,284,270,135,436,240,294,295,241,290,242,291,267,335,334,332,337,333,
  336,331,444,225,326,311,265,226,327,331,330,329,328,224,331,320,319,318,
  317,316,315,314,313,312,310,293,292,265,308,307,
136,136,136,136,136,136,136,136,136,136,136,285,136,256,256,256,256,287,289,
  296,298,299,300,301,302,303,304,305,306,321,323,324,325,286,322,288,297,
  309,284,270,135,437,240,294,295,241,290,242,291,267,335,334,332,337,333,
  336,331,440,225,326,311,265,226,327,331,330,329,328,224,331,320,319,318,
  317,316,315,314,313,312,310,293,292,265,308,307,
136,136,136,136,136,136,136,136,136,136,136,285,136,256,256,256,256,287,289,
  296,298,299,300,301,302,303,304,305,306,321,323,324,325,286,322,288,297,
  309,284,270,135,438,240,294,295,241,290,242,291,267,335,334,332,337,333,
  336,331,439,225,326,311,265,226,327,331,330,329,328,224,331,320,319,318,
  317,316,315,314,313,312,310,293,292,265,308,307,
136,136,136,136,136,136,136,136,136,136,136,285,136,256,256,256,256,287,289,
  296,298,299,300,301,302,303,304,305,306,321,323,324,325,286,322,288,297,
  309,284,270,135,439,240,294,295,241,290,242,291,267,335,334,332,337,333,
  336,331,438,225,326,311,265,226,327,331,330,329,328,224,331,320,319,318,
  317,316,315,314,313,312,310,293,292,265,308,307,
136,136,136,136,136,136,136,136,136,136,136,285,136,256,256,256,256,287,289,
  296,298,299,300,301,302,303,304,305,306,321,323,324,325,286,322,288,297,
  309,284,270,135,440,240,294,295,241,290,242,291,267,335,334,332,337,333,
  336,331,437,225,326,311,265,226,327,331,330,329,328,224,331,320,319,318,
  317,316,315,314,313,312,310,293,292,265,308,307,
136,136,136,136,136,136,136,136,136,136,136,285,136,256,256,256,256,287,289,
  296,298,299,300,301,302,303,304,305,306,321,323,324,325,286,322,288,297,
  309,284,270,135,441,240,294,295,241,290,242,291,267,335,334,332,337,333,
  336,331,436,225,326,311,265,226,327,331,330,329,328,224,331,320,319,318,
  317,316,315,314,313,312,310,293,292,265,308,307,
136,136,136,136,136,136,136,136,136,136,136,285,136,256,256,256,256,287,289,
  296,298,299,300,301,302,303,304,305,306,321,323,324,325,286,322,288,297,
  309,284,270,135,442,240,294,295,241,290,242,291,267,335,334,332,337,333,
  336,331,435,225,326,311,265,226,327,331,330,329,328,224,331,320,319,318,
  317,316,315,314,313,312,310,293,292,265,308,307,
136,136,136,136,136,136,136,136,136,136,136,285,136,256,256,256,256,287,289,
  296,298,299,300,301,302,303,304,305,306,321,323,324,325,286,322,288,297,
  309,284,270,135,443,240,294,295,241,290,242,291,267,335,334,332,337,333,
  336,331,434,225,326,311,265,226,327,331,330,329,328,224,331,320,319,318,
  317,316,315,314,313,312,310,293,292,265,308,307,
136,136,136,136,136,136,136,136,136,136,136,285,136,256,256,256,256,287,289,
  296,298,299,300,301,302,303,304,305,306,321,323,324,325,286,322,288,297,
  309,284,270,135,444,240,294,295,241,290,242,291,267,335,334,332,337,333,
  336,331,428,225,326,311,265,226,327,331,330,329,328,224,331,320,319,318,
  317,316,315,314,313,312,310,293,292,265,308,307,
136,136,136,136,136,136,136,136,136,136,136,285,136,256,256,256,256,287,289,
  296,298,299,300,301,302,303,304,305,306,321,323,324,325,286,322,288,297,
  309,284,270,135,445,240,294,295,241,290,242,291,267,335,334,332,337,333,
  336,331,427,225,326,311,265,226,327,331,330,329,328,224,331,320,319,318,
  317,316,315,314,313,312,310,293,292,265,308,307,
136,136,136,136,136,136,136,136,136,136,136,285,136,256,256,256,256,287,289,
  296,298,299,300,301,302,303,304,305,306,321,323,324,325,286,322,288,297,
  309,284,270,135,446,240,294,295,241,290,242,291,267,335,334,332,337,333,
  336,331,426,225,326,311,265,226,327,331,330,329,328,224,331,320,319,318,
  317,316,315,314,313,312,310,293,292,265,308,307,
136,136,136,136,136,136,136,136,136,136,136,285,136,256,256,256,256,287,289,
  296,298,299,300,301,302,303,304,305,306,321,323,324,325,286,322,288,297,
  309,284,270,135,447,240,294,295,241,290,242,291,267,335,334,332,337,333,
  336,331,425,225,326,311,265,226,327,331,330,329,328,224,331,320,319,318,
  317,316,315,314,313,312,310,293,292,265,308,307,
136,136,136,136,136,136,136,136,136,136,136,285,136,256,256,256,256,287,289,
  296,298,299,300,301,302,303,304,305,306,321,323,324,325,286,322,288,297,
  309,284,270,135,448,240,294,295,241,290,242,291,267,335,334,332,337,333,
  336,331,419,225,326,311,265,226,327,331,330,329,328,224,331,320,319,318,
  317,316,315,314,313,312,310,293,292,265,308,307,
136,136,136,136,136,136,136,136,136,136,136,285,136,256,256,256,256,287,289,
  296,298,299,300,301,302,303,304,305,306,321,323,324,325,286,322,288,297,
  309,284,270,135,449,240,294,295,241,290,242,291,267,335,334,332,337,333,
  336,331,418,225,326,311,265,226,327,331,330,329,328,224,331,320,319,318,
  317,316,315,314,313,312,310,293,292,265,308,307,
136,136,136,136,136,136,136,136,136,136,136,285,136,256,256,256,256,287,289,
  296,298,299,300,301,302,303,304,305,306,321,323,324,325,286,322,288,297,
  309,284,270,135,450,240,294,295,241,290,242,291,267,335,334,332,337,333,
  336,331,417,225,326,311,265,226,327,331,330,329,328,224,331,320,319,318,
  317,316,315,314,313,312,310,293,292,265,308,307,
136,136,136,136,136,136,136,136,136,136,136,285,136,256,256,256,256,287,289,
  296,298,299,300,301,302,303,304,305,306,321,323,324,325,286,322,288,297,
  309,284,270,135,451,240,294,295,241,290,242,291,267,335,334,332,337,333,
  336,331,416,225,326,311,265,226,327,331,330,329,328,224,331,320,319,318,
  317,316,315,314,313,312,310,293,292,265,308,307,
136,136,136,136,136,136,136,136,136,136,136,285,136,256,256,256,256,287,289,
  296,298,299,300,301,302,303,304,305,306,321,323,324,325,286,322,288,297,
  309,284,270,135,452,240,294,295,241,290,242,291,267,335,334,332,337,333,
  336,331,415,225,326,311,265,226,327,331,330,329,328,224,331,320,319,318,
  317,316,315,314,313,312,310,293,292,265,308,307,
136,136,136,136,136,136,136,136,136,136,136,285,136,256,256,256,256,287,289,
  296,298,299,300,301,302,303,304,305,306,321,323,324,325,286,322,288,297,
  309,284,270,135,453,240,294,295,241,290,242,291,267,335,334,332,337,333,
  336,331,414,225,326,311,265,226,327,331,330,329,328,224,331,320,319,318,
  317,316,315,314,313,312,310,293,292,265,308,307,
136,136,136,136,136,136,136,136,136,136,136,285,136,256,256,256,256,287,289,
  296,298,299,300,301,302,303,304,305,306,321,323,324,325,286,322,288,297,
  309,284,270,135,454,240,294,295,241,290,242,291,267,335,334,332,337,333,
  336,331,413,225,326,311,265,226,327,331,330,329,328,224,331,320,319,318,
  317,316,315,314,313,312,310,293,292,265,308,307,
136,136,136,136,136,136,136,136,136,136,136,285,136,256,256,256,256,287,289,
  296,298,299,300,301,302,303,304,305,306,321,323,324,325,286,322,288,297,
  309,284,270,135,455,240,294,295,241,290,242,291,267,335,334,332,337,333,
  336,331,412,225,326,311,265,226,327,331,330,329,328,224,331,320,319,318,
  317,316,315,314,313,312,310,293,292,265,308,307,
136,136,136,136,136,136,136,136,136,136,136,285,136,256,256,256,256,287,289,
  296,298,299,300,301,302,303,304,305,306,321,323,324,325,286,322,288,297,
  309,284,270,135,456,240,294,295,241,290,242,291,267,335,334,332,337,333,
  336,331,411,225,326,311,265,226,327,331,330,329,328,224,331,320,319,318,
  317,316,315,314,313,312,310,293,292,265,308,307,
136,136,136,136,136,136,136,136,136,136,136,285,136,256,256,256,256,287,289,
  296,298,299,300,301,302,303,304,305,306,321,323,324,325,286,322,288,297,
  309,284,270,135,457,240,294,295,241,290,242,291,267,335,334,332,337,333,
  336,331,410,225,326,311,265,226,327,331,330,329,328,224,331,320,319,318,
  317,316,315,314,313,312,310,293,292,265,308,307,
136,136,136,136,136,136,136,136,136,136,136,285,136,256,256,256,256,287,289,
  296,298,299,300,301,302,303,304,305,306,321,323,324,325,286,322,288,297,
  309,284,270,135,458,240,294,295,241,290,242,291,267,335,334,332,337,333,
  336,331,409,225,326,311,265,226,327,331,330,329,328,224,331,320,319,318,
  317,316,315,314,313,312,310,293,292,265,308,307,
136,136,136,136,136,136,136,136,136,136,136,285,136,256,256,256,256,287,289,
  296,298,299,300,301,302,303,304,305,306,321,323,324,325,286,322,288,297,
  309,284,270,135,459,240,294,295,241,290,242,291,267,335,334,332,337,333,
  336,331,408,225,326,311,265,226,327,331,330,329,328,224,331,320,319,318,
  317,316,315,314,313,312,310,293,292,265,308,307,
136,136,136,136,136,136,136,136,136,136,136,285,136,256,256,256,256,287,289,
  296,298,299,300,301,302,303,304,305,306,321,323,324,325,286,322,288,297,
  309,284,270,135,460,240,294,295,241,290,242,291,267,335,334,332,337,333,
  336,331,407,225,326,311,265,226,327,331,330,329,328,224,331,320,319,318,
  317,316,315,314,313,312,310,293,292,265,308,307,
136,136,136,136,136,136,136,136,136,136,136,285,136,256,256,256,256,287,289,
  296,298,299,300,301,302,303,304,305,306,321,323,324,325,286,322,288,297,
  309,284,270,135,461,240,294,295,241,290,242,291,267,335,334,332,337,333,
  336,331,406,225,326,311,265,226,327,331,330,329,328,224,331,320,319,318,
  317,316,315,314,313,312,310,293,292,265,308,307,
136,136,136,136,136,136,136,136,136,136,136,285,136,256,256,256,256,287,289,
  296,298,299,300,301,302,303,304,305,306,321,323,324,325,286,322,288,297,
  309,284,270,135,462,240,294,295,241,290,242,291,267,335,334,332,337,333,
  336,331,405,225,326,311,265,226,327,331,330,329,328,224,331,320,319,318,
  317,316,315,314,313,312,310,293,292,265,308,307,
303,633,634,305,304,305,304,303,463,307,303,304,305,381,306,
303,633,634,305,304,305,304,303,464,307,303,304,305,380,306,
303,633,634,305,304,305,304,303,465,307,303,304,305,379,306,
303,633,634,305,304,305,304,303,466,307,303,304,305,378,306,
308,309,309,308,467,308,309,377,
308,309,309,308,468,308,309,376,
295,302,301,298,301,298,295,469,397,295,298,301,629,
295,302,301,298,301,298,295,470,396,295,298,301,629,
295,302,301,298,301,298,295,471,395,295,298,301,629,
310,625,626,627,312,311,312,311,310,472,310,311,312,315,394,313,314,
310,625,626,627,312,311,312,311,310,473,310,311,312,315,393,313,314,
292,291,291,288,287,286,285,284,283,474,392,291,628,
292,291,291,288,287,286,285,284,283,475,391,291,628,
292,291,291,288,287,286,285,284,283,476,390,291,628,
292,291,291,288,287,286,285,284,283,477,389,291,628,
292,291,291,288,287,286,285,284,283,478,635,291,628,
292,291,291,288,287,286,285,284,283,479,387,291,628,
292,291,291,288,287,286,285,284,283,480,386,291,628,
292,291,291,288,287,286,285,284,283,481,385,291,628,
292,291,291,288,287,286,285,284,283,482,384,291,628,
292,291,291,288,287,286,285,284,283,483,383,291,628,
292,291,291,288,287,286,285,284,283,484,382,291,628,
136,136,136,136,136,136,136,136,136,136,136,285,136,256,256,256,256,287,636,
  145,296,298,299,300,301,302,303,304,305,306,321,323,324,325,286,322,288,
  297,309,284,270,135,485,240,294,295,241,290,125,278,242,291,637,126,267,
  335,334,332,337,333,336,331,127,225,326,639,311,265,226,327,331,330,329,
  328,224,331,320,319,318,317,316,315,314,313,312,310,293,292,279,265,638,
  308,307,
285,256,256,256,256,287,289,640,286,563,288,270,19,486,124,294,641,242,291,
  293,292,308,
20,20,20,20,20,20,20,20,20,20,20,20,20,20,20,20,20,20,20,20,20,20,20,20,20,
  20,20,20,20,20,20,20,20,20,20,20,20,20,20,20,20,20,20,20,20,20,20,20,20,
  20,20,20,20,20,20,20,20,20,20,20,20,20,20,20,20,20,20,20,20,20,20,20,20,
  20,20,20,20,20,20,20,20,20,20,20,20,20,20,20,20,20,20,20,20,20,20,20,20,
  20,20,20,20,20,20,20,20,20,20,20,20,20,20,20,20,20,20,20,20,20,20,20,20,
  20,20,20,20,20,20,20,20,20,20,20,20,20,487,1,997,
642,642,642,642,642,642,642,642,642,642,642,642,642,642,642,642,642,642,642,
  642,642,642,642,642,642,642,642,642,642,642,642,642,642,642,642,642,642,
  642,642,642,642,642,642,642,642,642,642,642,642,642,642,642,642,642,642,
  642,642,642,642,642,642,642,642,642,642,642,642,642,642,642,642,642,642,
  642,642,642,642,642,642,642,642,642,642,642,642,642,642,642,642,642,642,
  642,642,642,642,642,642,642,642,642,642,642,642,642,642,642,642,642,642,
  642,642,642,642,642,642,642,642,642,642,642,642,642,642,642,642,642,642,
  642,642,642,642,642,642,1,488,1,642,
20,20,20,20,20,20,20,20,20,20,20,20,20,1,489,1,942,
136,136,136,136,136,136,136,136,136,136,136,136,135,490,120,265,265,
20,20,20,20,20,20,20,20,20,20,20,20,20,20,20,20,20,20,20,1,491,1,941,
139,139,139,139,139,139,139,139,139,139,139,139,139,139,139,139,139,139,139,
  492,119,613,
20,20,20,20,20,20,493,1,940,
20,20,20,20,20,20,494,1,939,
20,20,1,495,1,938,
643,145,496,116,278,637,115,279,638,
20,20,20,20,20,20,497,1,936,
20,20,20,20,20,20,498,1,935,
20,20,20,20,20,20,20,20,20,20,20,20,20,20,20,20,20,20,20,499,1,934,
136,136,136,136,136,136,136,136,136,136,136,136,108,108,108,108,135,108,108,
  500,109,265,265,
145,501,644,278,279,
145,502,106,278,279,
20,20,20,20,20,20,20,20,20,20,20,20,20,20,20,20,20,20,20,20,20,20,20,20,20,
  20,20,20,20,20,20,20,20,20,20,20,20,20,20,20,20,20,20,503,1,933,
645,645,645,645,645,645,645,645,645,645,645,645,645,645,645,645,645,645,645,
  645,645,645,645,645,645,645,645,645,645,645,645,645,645,645,645,645,645,
  645,645,645,645,645,1,504,1,645,
136,136,136,136,136,136,136,136,136,136,136,285,136,256,256,256,256,287,289,
  145,296,298,299,300,301,302,303,304,305,306,321,323,324,325,286,322,288,
  297,309,284,270,135,1,505,240,294,295,241,290,102,278,242,291,267,335,
  334,332,337,333,336,331,1,646,101,225,326,311,265,226,327,331,330,329,
  328,224,331,320,319,318,317,316,315,314,313,312,310,293,292,279,265,308,
  307,
20,20,20,20,20,20,20,20,20,20,20,20,20,20,20,20,20,20,20,20,20,20,20,20,20,
  20,20,20,20,20,20,20,20,20,20,20,20,20,20,20,20,1,506,1,932,
136,136,136,136,136,136,136,136,136,136,136,285,136,256,256,256,256,287,289,
  296,298,299,300,301,302,303,304,305,306,321,323,324,325,286,322,288,297,
  309,284,270,135,507,240,294,295,241,290,242,291,267,335,334,332,337,333,
  336,331,100,225,326,311,265,226,327,331,330,329,328,224,331,320,319,318,
  317,316,315,314,313,312,310,293,292,265,308,307,
20,20,20,20,20,20,20,20,20,20,20,20,20,1,508,1,931,
136,136,136,136,136,136,136,136,136,136,136,136,135,509,99,265,265,
20,20,20,20,20,20,20,20,20,20,20,20,20,1,510,1,930,
136,136,136,136,136,136,136,136,136,136,136,136,135,511,98,265,265,
20,20,20,20,20,20,20,20,20,20,20,20,20,1,512,1,929,
136,136,136,136,136,136,136,136,136,136,136,136,135,513,131,647,265,265,
20,20,20,20,20,20,20,20,20,20,20,20,20,1,514,1,928,
136,136,136,136,136,136,136,136,136,136,136,136,135,515,131,648,265,265,
20,20,20,20,20,20,20,20,20,20,20,20,20,1,516,1,927,
136,136,136,136,136,136,136,136,136,136,136,136,135,517,131,649,265,265,
20,20,20,20,20,20,20,20,20,20,20,20,20,20,20,20,20,20,20,20,20,20,20,20,20,
  20,20,20,20,20,20,20,20,20,20,20,20,20,20,20,20,20,1,518,1,926,
20,20,20,20,20,20,20,20,20,20,20,20,20,20,20,20,20,20,20,20,20,20,20,20,20,
  20,20,20,20,20,20,20,20,20,20,20,20,20,20,20,20,20,1,519,1,925,
136,136,136,136,136,136,136,136,136,136,136,285,136,256,256,256,256,287,636,
  145,296,298,299,300,301,302,303,304,305,306,321,323,324,325,286,322,288,
  297,309,284,270,135,520,240,294,295,241,290,125,278,242,291,637,126,267,
  335,334,332,337,333,336,331,127,225,326,650,311,265,226,327,331,330,329,
  328,224,331,320,319,318,317,316,315,314,313,312,310,293,292,279,265,638,
  308,307,
20,20,20,20,20,20,20,20,20,20,20,20,20,20,20,20,20,20,20,20,20,20,20,20,20,
  20,20,20,20,20,20,20,20,20,20,20,20,20,20,20,20,20,1,521,1,924,
20,20,20,20,20,20,20,20,20,20,20,20,20,20,20,20,20,20,20,20,20,20,20,20,20,
  20,20,20,20,20,20,20,20,20,20,20,20,20,20,20,20,20,1,522,1,923,
20,20,20,20,20,20,20,20,20,20,20,20,20,20,20,20,20,20,20,20,20,20,20,20,20,
  20,20,20,20,20,20,20,20,20,20,20,20,20,20,20,20,20,1,523,1,922,
136,136,136,136,136,136,136,136,136,136,136,285,136,256,256,256,256,287,636,
  145,296,298,299,300,301,302,303,304,305,306,321,323,324,325,286,322,288,
  297,309,284,270,135,524,240,294,295,241,290,125,278,242,291,637,126,267,
  335,334,332,337,333,336,331,127,225,326,651,311,265,226,327,331,330,329,
  328,224,331,320,319,318,317,316,315,314,313,312,310,293,292,279,265,638,
  308,307,
20,20,20,20,20,20,20,20,20,20,20,20,20,20,20,20,20,20,20,20,20,20,20,20,20,
  20,20,20,20,20,20,20,20,20,20,20,20,20,20,20,20,1,525,1,921,
20,20,20,20,20,20,20,20,20,20,20,20,20,20,20,20,20,20,20,20,20,20,20,20,20,
  20,20,20,20,20,20,20,20,20,20,20,20,20,20,20,20,1,526,1,920,
136,136,136,136,136,136,136,136,136,136,136,285,136,256,256,256,256,287,289,
  296,298,299,300,301,302,303,304,305,306,321,323,324,325,286,322,288,297,
  309,284,270,135,527,240,294,295,241,290,242,291,267,335,334,332,337,333,
  336,331,87,225,326,311,265,226,327,331,330,329,328,224,331,320,319,318,
  317,316,315,314,313,312,310,293,292,265,308,307,
20,20,20,20,20,20,528,1,919,
20,20,20,20,20,20,20,20,20,20,20,20,20,20,20,20,20,20,20,20,20,20,20,20,20,
  20,20,20,20,20,20,20,20,20,20,20,20,20,20,20,20,1,529,1,918,
20,20,20,20,20,20,20,20,20,20,20,20,20,20,20,20,20,20,20,20,20,20,20,20,20,
  20,20,20,20,20,20,20,20,20,20,20,20,20,20,20,20,1,530,1,917,
20,20,20,20,20,20,20,20,20,20,20,20,20,20,20,20,20,20,20,20,20,20,20,20,20,
  20,20,20,20,20,20,20,20,20,20,20,20,20,20,20,20,1,531,1,916,
136,136,136,136,136,136,136,136,136,136,136,285,136,256,256,256,256,287,289,
  296,298,299,300,301,302,303,304,305,306,321,323,324,325,286,322,288,297,
  309,284,270,135,532,240,294,295,241,290,242,291,267,335,334,332,337,333,
  336,331,83,225,326,311,265,226,327,331,330,329,328,224,331,320,319,318,
  317,316,315,314,313,312,310,293,292,265,308,307,
136,136,136,136,136,136,136,136,136,136,136,136,135,533,78,265,265,
20,20,20,20,20,20,20,20,534,1,915,
20,20,20,20,20,20,20,20,535,1,914,
43,44,45,40,22,536,22,22,48,22,47,46,46,
359,361,364,365,366,367,368,369,370,371,372,373,374,375,376,377,378,379,380,
  381,382,389,393,394,392,323,325,391,383,387,384,386,390,362,360,309,388,
  385,41,44,537,67,67,67,652,67,67,67,67,67,67,67,67,67,67,67,67,67,67,67,
  67,67,67,67,67,67,67,67,67,67,67,67,67,67,67,67,67,67,67,67,67,67,
43,44,45,40,63,538,63,63,48,63,47,46,46,
385,539,653,
654,654,654,654,654,1,540,1,654,
655,655,655,655,655,1,541,1,655,
656,656,656,656,656,1,542,1,656,
149,657,151,152,150,156,155,658,658,659,154,657,147,543,657,
43,44,45,40,58,544,58,58,48,58,47,46,46,
43,44,45,40,55,545,55,55,48,55,47,46,46,
259,259,259,259,259,259,259,259,259,546,
272,274,275,273,276,278,547,
271,547,271,271,271,271,271,271,271,271,271,271,271,271,271,271,271,271,271,
  271,271,268,271,271,271,271,271,271,271,271,271,271,271,271,271,271,271,
  271,271,271,271,277,271,548,660,
262,262,262,262,262,262,262,262,262,262,253,253,253,253,253,253,253,253,253,
  253,253,253,253,253,253,253,253,253,253,253,253,253,253,253,253,253,253,
  253,253,253,253,253,253,253,253,253,253,253,253,549,
262,262,262,262,262,262,262,262,262,262,249,249,249,249,249,249,249,249,249,
  249,249,249,249,249,249,249,249,249,249,249,249,249,249,249,249,249,249,
  249,249,249,249,249,249,249,249,249,249,249,249,550,
309,551,661,
309,552,662,
309,553,663,
309,554,664,
309,555,665,
309,556,666,
309,557,667,
309,558,668,
309,559,669,
309,560,670,
309,561,671,
136,136,136,136,136,136,136,136,136,136,136,285,136,256,256,256,256,287,289,
  296,298,299,300,301,302,303,304,305,306,321,323,324,325,286,322,288,297,
  309,284,270,135,562,240,294,295,241,290,242,291,267,335,334,332,337,333,
  336,331,672,225,326,311,265,226,327,331,330,329,328,224,331,320,319,318,
  317,316,315,314,313,312,310,293,292,265,308,307,
255,255,255,255,255,563,
20,20,20,20,20,20,20,20,20,20,20,20,20,20,20,20,20,20,20,20,20,20,20,20,20,
  20,20,20,20,20,20,20,20,20,20,20,20,20,20,564,1,975,
673,673,673,673,673,673,673,673,673,673,673,673,673,673,673,673,673,673,673,
  673,673,673,673,673,673,673,673,673,673,673,673,673,673,673,673,673,673,
  673,1,565,1,673,
20,20,20,20,20,20,20,20,20,20,20,20,20,20,20,20,20,20,20,20,20,20,20,20,20,
  20,20,20,20,20,20,20,20,20,20,20,20,20,20,566,1,974,
674,674,674,674,674,674,674,674,674,674,674,674,674,674,674,674,674,674,674,
  674,674,674,674,674,674,674,674,674,674,674,674,674,674,674,674,674,674,
  674,1,567,1,674,
675,675,675,675,675,675,675,675,675,675,675,675,675,675,675,675,675,675,675,
  675,675,675,675,675,675,675,675,675,675,675,675,675,675,675,675,675,675,
  675,1,568,1,675,
676,676,676,676,676,676,676,676,676,676,676,676,676,676,676,676,676,676,676,
  676,676,676,676,676,676,676,676,676,676,676,676,676,676,676,676,676,676,
  676,1,569,1,676,
677,677,677,677,677,677,677,677,677,677,677,677,677,677,677,677,677,677,677,
  677,677,677,677,677,677,677,677,677,677,677,677,677,677,677,677,677,677,
  677,1,570,1,677,
678,678,678,678,678,678,678,678,678,678,678,678,678,678,678,678,678,678,678,
  678,678,678,678,678,678,678,678,678,678,678,678,678,678,678,678,678,678,
  678,678,678,678,1,571,1,678,
679,679,679,679,679,679,679,679,679,679,679,679,679,679,679,679,679,679,679,
  679,679,679,679,679,679,679,679,679,679,679,679,679,679,679,679,679,679,
  679,679,679,679,1,572,1,679,
20,20,20,20,20,20,20,20,20,20,20,20,20,20,20,20,20,20,20,20,20,20,20,20,20,
  20,20,20,20,20,20,20,20,20,20,20,20,20,20,20,20,20,573,1,968,
680,680,680,680,680,680,680,680,680,680,680,680,680,680,680,680,680,680,680,
  680,680,680,680,680,680,680,680,680,680,680,680,680,680,680,680,680,680,
  680,680,680,680,1,574,1,680,
20,20,20,20,20,20,20,20,20,20,20,20,20,20,20,20,20,20,20,20,20,20,20,20,20,
  20,20,20,20,20,20,20,20,20,20,20,20,20,20,20,20,20,575,1,967,
681,681,681,681,681,681,681,681,681,681,681,681,681,681,681,681,681,681,681,
  681,681,681,681,681,681,681,681,681,681,681,681,681,681,681,681,681,681,
  681,681,681,681,1,576,1,681,
20,20,20,20,20,20,20,20,20,20,20,20,20,20,20,20,20,20,20,20,20,20,20,20,20,
  20,20,20,20,20,20,20,20,20,20,20,20,20,20,20,20,20,577,1,966,
682,682,682,682,682,682,682,682,682,682,682,682,682,682,682,682,682,682,682,
  682,682,682,682,682,682,682,682,682,682,682,682,682,682,682,682,682,682,
  682,682,682,682,1,578,1,682,
20,20,20,20,20,20,20,20,20,20,20,20,20,20,20,20,20,20,20,20,20,20,20,20,20,
  20,20,20,20,20,20,20,20,20,20,20,20,20,20,20,20,20,579,1,965,
683,683,683,683,683,683,683,683,683,683,683,683,683,683,683,683,683,683,683,
  683,683,683,683,683,683,683,683,683,683,683,683,683,683,683,683,683,683,
  683,683,683,683,1,580,1,683,
20,20,20,20,20,20,20,20,20,20,20,20,20,20,20,20,20,20,20,20,20,20,20,20,20,
  20,20,20,20,20,20,20,20,20,20,20,20,20,20,20,20,20,581,1,964,
684,684,684,684,684,684,684,684,684,684,684,684,684,684,684,684,684,684,684,
  684,684,684,684,684,684,684,684,684,684,684,684,684,684,684,684,684,684,
  684,684,684,684,1,582,1,684,
685,685,685,685,685,685,685,685,685,685,685,685,685,685,685,685,685,685,685,
  685,685,685,685,685,685,685,685,685,685,685,685,685,685,685,685,685,685,
  685,685,685,685,1,583,1,685,
20,20,20,20,20,20,20,20,20,20,20,20,20,20,20,20,20,20,20,20,20,20,20,20,20,
  20,20,20,20,20,20,20,20,20,20,20,20,20,20,20,20,20,584,1,962,
686,686,686,686,686,686,686,686,686,686,686,686,686,686,686,686,686,686,686,
  686,686,686,686,686,686,686,686,686,686,686,686,686,686,686,686,686,686,
  686,686,686,686,1,585,1,686,
687,687,687,687,687,687,687,687,687,687,687,687,687,687,687,687,687,687,687,
  687,687,687,687,687,687,687,687,687,687,687,687,687,687,687,687,687,687,
  687,687,687,687,1,586,1,687,
20,20,20,20,20,20,20,20,20,20,20,20,20,20,20,20,20,20,20,20,20,20,20,20,20,
  20,20,20,20,20,20,20,20,20,20,20,20,20,20,20,20,20,587,1,960,
688,688,688,688,688,688,688,688,688,688,688,688,688,688,688,688,688,688,688,
  688,688,688,688,688,688,688,688,688,688,688,688,688,688,688,688,688,688,
  688,688,688,688,1,588,1,688,
689,689,689,689,689,689,689,689,689,689,689,689,689,689,689,689,689,689,689,
  689,689,689,689,689,689,689,689,689,689,689,689,689,689,689,689,689,689,
  689,689,689,689,1,589,1,689,
20,20,20,20,20,20,20,20,20,20,20,20,20,20,20,20,20,20,20,20,20,20,20,20,20,
  20,20,20,20,20,20,20,20,20,20,20,20,20,20,20,20,1,590,1,958,
20,20,20,20,20,20,20,20,20,20,20,20,20,20,20,20,20,20,20,20,20,20,20,20,20,
  20,20,20,20,20,20,20,20,20,20,20,20,20,20,20,20,1,591,1,957,
20,20,20,20,20,20,20,20,20,20,20,20,20,20,20,20,20,20,20,20,20,20,20,20,20,
  20,20,20,20,20,20,20,20,20,20,20,20,20,20,20,20,1,592,1,956,
20,20,20,20,20,20,20,20,20,20,20,20,20,20,20,20,20,20,20,20,20,20,20,20,20,
  20,20,20,20,20,20,20,20,20,20,20,20,20,20,20,20,1,593,1,955,
20,20,20,20,20,20,20,20,20,20,20,20,20,20,20,20,20,20,20,20,20,20,20,20,20,
  20,20,20,20,20,20,20,20,20,20,20,20,20,20,20,20,1,594,1,954,
20,20,20,20,20,20,20,20,20,20,20,20,20,20,20,20,20,20,20,20,20,20,20,20,20,
  20,20,20,20,20,20,20,20,20,20,20,20,20,20,20,20,1,595,1,953,
20,20,20,20,20,20,20,20,20,20,20,20,20,20,20,20,20,20,20,20,20,20,20,20,20,
  20,20,20,20,20,20,20,20,20,20,20,20,20,20,20,20,1,596,1,952,
20,20,20,20,20,20,20,20,20,20,20,20,20,20,20,20,20,20,20,20,20,20,20,20,20,
  20,20,20,20,20,20,20,20,20,20,20,20,20,20,20,20,1,597,1,951,
20,20,20,20,20,20,20,20,20,20,20,20,20,20,20,20,20,20,20,20,20,20,20,20,20,
  20,20,20,20,20,20,20,20,20,20,20,20,20,20,20,20,1,598,1,950,
20,20,20,20,20,20,20,20,20,20,20,20,20,20,20,20,20,20,20,20,20,20,20,20,20,
  20,20,20,20,20,20,20,20,20,20,20,20,20,20,20,20,1,599,1,949,
20,20,20,20,20,20,20,20,20,20,20,20,20,20,20,20,20,20,20,20,20,20,20,20,20,
  20,20,20,20,20,20,20,20,20,20,20,20,20,20,20,20,1,600,1,948,
20,20,20,20,20,20,20,20,20,20,20,20,20,20,20,20,20,20,20,20,20,20,20,20,20,
  20,20,20,20,20,20,20,20,20,20,20,20,20,20,20,20,1,601,1,947,
43,44,45,40,54,602,54,54,48,54,47,46,46,
43,44,45,40,53,603,53,53,48,53,47,46,46,
43,44,45,40,52,604,52,52,48,52,47,46,46,
43,44,45,40,49,605,49,49,48,49,47,46,46,
43,44,45,40,48,606,48,48,48,48,47,46,46,
136,136,136,136,136,136,136,136,136,136,136,136,135,607,690,265,265,
43,44,45,40,46,608,46,46,48,46,47,46,46,
43,44,45,40,45,609,45,45,48,45,47,46,46,
165,165,165,165,165,165,165,165,165,165,165,165,165,165,165,165,165,165,165,
  165,165,165,165,165,165,165,165,165,165,165,165,165,165,165,165,165,165,
  165,165,165,165,166,165,610,
43,44,45,40,44,611,44,44,48,44,47,46,46,
43,44,45,40,43,612,43,43,48,43,47,46,46,
140,140,140,140,140,140,140,140,140,140,140,140,140,140,140,140,140,140,140,
  20,20,20,20,613,1,889,
691,691,691,691,691,1,614,1,691,
692,692,692,692,692,1,615,1,692,
693,693,693,693,693,1,616,1,693,
694,694,694,694,694,1,617,1,694,
20,20,20,20,20,20,20,20,20,20,20,20,20,20,20,20,20,20,20,20,20,20,20,20,20,
  20,20,20,20,20,20,20,20,20,20,20,20,20,20,20,20,1,618,1,883,
136,136,136,136,136,136,136,136,136,136,136,285,136,256,256,256,256,287,289,
  296,298,299,300,301,302,303,304,305,306,321,323,324,325,286,322,288,297,
  309,284,270,135,619,240,294,295,241,290,242,291,267,335,334,332,337,333,
  336,331,695,225,326,311,265,226,327,331,330,329,328,224,331,320,319,318,
  317,316,315,314,313,312,310,293,292,265,308,307,
20,20,20,20,20,20,20,20,20,20,20,20,20,20,20,20,20,20,20,20,20,20,20,20,20,
  20,20,20,20,20,20,20,20,20,20,20,20,20,20,20,20,1,620,1,882,
136,136,136,136,136,136,136,136,136,136,136,285,136,256,256,256,256,287,289,
  296,298,299,300,301,302,303,304,305,306,321,323,324,325,286,322,288,297,
  309,284,270,135,621,240,294,295,241,290,242,291,267,335,334,332,337,333,
  336,331,696,225,326,311,265,226,327,331,330,329,328,224,331,320,319,318,
  317,316,315,314,313,312,310,293,292,265,308,307,
43,44,45,40,23,622,23,23,48,23,47,46,46,
20,20,20,20,20,20,623,1,946,
697,697,697,697,697,1,624,1,697,
20,20,20,20,20,20,20,20,625,1,996,
20,20,20,20,20,20,20,20,626,1,995,
20,20,20,20,20,20,20,20,627,1,994,
20,20,20,20,20,20,20,20,20,628,1,1047,
20,20,20,20,20,20,20,20,629,1,1061,
698,698,698,698,698,698,698,698,698,698,698,698,698,698,698,698,698,698,698,
  698,698,698,698,698,698,698,698,698,698,698,698,698,698,698,698,698,698,
  698,698,698,698,1,630,1,698,
699,699,699,699,699,699,699,699,699,699,699,699,699,699,699,699,699,699,699,
  699,699,699,699,699,699,699,699,699,699,699,699,699,699,699,699,699,699,
  699,699,699,699,1,631,1,699,
700,700,700,700,700,700,700,700,700,700,700,700,700,700,700,700,700,700,700,
  700,700,700,700,700,700,700,700,700,700,700,700,700,700,700,700,700,700,
  700,700,700,700,1,632,1,700,
20,20,20,20,20,20,20,633,1,993,
20,20,20,20,20,20,20,634,1,992,
701,1,635,1,701,
271,547,271,271,271,271,271,271,271,271,271,271,271,271,271,271,271,271,271,
  271,271,271,271,271,271,271,271,271,271,271,271,271,271,271,271,271,271,
  271,271,271,277,271,636,702,
271,547,271,271,271,271,271,271,271,271,271,271,271,271,271,271,271,271,271,
  271,271,141,271,271,271,271,271,271,271,271,271,271,271,271,271,271,271,
  271,271,271,271,277,271,637,143,
20,20,20,20,20,20,20,20,638,1,937,
703,388,639,65,704,
260,260,260,260,260,260,260,260,260,640,
258,248,262,549,550,262,257,257,257,257,257,243,243,243,243,252,252,243,243,
  641,252,
117,118,119,120,121,122,123,124,125,126,127,128,129,130,131,132,133,134,135,
  136,137,138,139,140,68,69,70,71,72,73,141,142,143,144,145,146,74,75,76,
  77,78,147,148,149,79,80,81,82,83,150,151,152,153,154,155,156,157,158,
  159,160,161,162,163,164,113,65,66,67,171,172,173,174,175,176,177,178,
  179,180,181,182,183,184,185,186,165,166,167,168,169,170,84,85,86,87,88,
  89,51,52,53,90,91,92,93,94,95,96,97,98,54,55,99,100,101,102,103,104,105,
  106,56,57,58,107,108,59,60,109,110,111,112,61,62,63,64,642,705,706,202,
  706,706,706,706,706,706,706,706,706,706,332,332,332,332,332,333,334,335,
  336,339,339,339,340,341,345,345,345,345,346,347,348,349,350,351,352,353,
  356,356,356,357,358,359,360,361,362,363,364,365,369,369,369,369,370,371,
  372,373,374,375,248,247,246,245,244,243,264,263,262,261,260,259,258,257,
  256,255,254,253,252,251,250,249,190,116,189,115,188,114,187,242,241,240,
  239,238,237,236,235,234,233,232,231,230,229,228,227,227,227,227,227,227,
  226,225,224,223,223,223,223,223,223,222,221,220,219,218,217,216,216,216,
  216,215,215,215,215,214,213,212,211,210,209,208,207,206,205,204,203,201,
  200,199,198,197,196,195,194,193,192,191,
271,547,271,271,271,271,271,271,271,271,271,271,271,271,271,271,271,271,271,
  271,271,271,271,271,271,271,271,271,271,271,271,271,271,271,271,271,271,
  271,271,271,277,271,643,707,
385,644,708,
136,136,136,136,136,136,136,136,136,136,136,285,136,256,256,256,256,287,636,
  145,296,298,299,300,301,302,303,304,305,306,321,323,324,325,286,322,288,
  297,309,284,270,135,645,240,294,295,241,290,125,278,242,291,637,126,267,
  335,334,332,337,333,336,331,127,225,326,709,311,265,226,327,331,330,329,
  328,224,331,320,319,318,317,316,315,314,313,312,310,293,292,279,265,638,
  308,307,
136,136,136,136,136,136,136,136,136,136,136,285,136,256,256,256,256,287,289,
  145,296,298,299,300,301,302,303,304,305,306,321,323,324,325,286,322,288,
  297,309,284,270,135,646,240,294,295,241,290,104,278,242,291,267,335,334,
  332,337,333,336,331,105,225,326,311,265,226,327,331,330,329,328,224,331,
  320,319,318,317,316,315,314,313,312,310,293,292,279,265,308,307,
703,97,97,97,97,97,97,647,710,
703,96,96,96,96,96,96,648,710,
703,95,95,95,95,95,95,649,710,
703,94,94,94,94,94,94,650,704,
703,91,91,91,91,91,91,651,704,
359,361,364,365,366,367,368,369,370,371,372,373,374,375,376,377,378,379,380,
  381,382,389,393,394,392,323,325,391,383,387,384,386,390,362,360,711,309,
  388,385,41,44,713,713,712,652,68,1,713,68,68,68,68,69,68,68,68,68,68,68,
  68,68,68,68,68,68,68,68,68,68,68,68,68,68,68,68,68,68,68,68,68,68,68,68,
  68,68,68,68,68,
714,714,714,714,714,714,714,714,714,714,714,714,714,714,714,714,714,714,714,
  714,714,714,714,714,714,714,714,714,714,714,714,714,714,714,714,714,714,
  714,714,714,714,1,653,1,714,
43,44,45,40,61,654,61,61,48,61,47,46,46,
43,44,45,40,60,655,60,60,48,60,47,46,46,
43,44,45,40,59,656,59,59,48,59,47,46,46,
715,715,715,715,715,715,715,715,715,657,
716,716,716,716,716,658,
153,153,153,153,153,153,153,153,153,153,153,153,153,153,153,153,716,716,716,
  716,716,153,153,153,153,153,153,153,153,153,153,153,153,153,153,153,153,
  153,153,153,153,153,659,
269,660,
717,717,717,717,717,717,717,717,717,717,717,717,717,1,661,1,717,
136,136,136,136,136,136,136,136,136,136,136,285,136,256,256,256,256,287,289,
  296,298,299,300,301,302,303,304,305,306,321,323,324,325,286,322,288,297,
  309,284,270,135,662,240,294,295,241,290,242,291,267,335,334,332,337,333,
  336,331,718,225,326,311,265,226,327,331,330,329,328,224,331,320,319,318,
  317,316,315,314,313,312,310,293,292,265,308,307,
136,136,136,136,136,136,136,136,136,136,136,285,136,256,256,256,256,287,289,
  296,298,299,300,301,302,303,304,305,306,321,323,324,325,286,322,288,297,
  309,284,270,135,663,240,294,295,241,290,242,291,267,335,334,332,337,333,
  336,331,719,225,326,311,265,226,327,331,330,329,328,224,331,320,319,318,
  317,316,315,314,313,312,310,293,292,265,308,307,
136,136,136,136,136,136,136,136,136,136,136,285,136,256,256,256,256,287,289,
  296,298,299,300,301,302,303,304,305,306,321,323,324,325,286,322,288,297,
  309,284,270,135,664,240,294,295,241,290,242,291,267,335,334,332,337,333,
  336,331,720,225,326,311,265,226,327,331,330,329,328,224,331,320,319,318,
  317,316,315,314,313,312,310,293,292,265,308,307,
136,136,136,136,136,136,136,136,136,136,136,285,136,256,256,256,256,287,289,
  296,298,299,300,301,302,303,304,305,306,321,323,324,325,286,322,288,297,
  309,284,270,135,665,240,294,295,241,290,242,291,267,335,334,332,337,333,
  336,331,721,225,326,311,265,226,327,331,330,329,328,224,331,320,319,318,
  317,316,315,314,313,312,310,293,292,265,308,307,
136,136,136,136,136,136,136,136,136,136,136,285,136,256,256,256,256,287,289,
  296,298,299,300,301,302,303,304,305,306,321,323,324,325,286,322,288,297,
  309,284,270,135,666,240,294,295,241,290,242,291,267,335,334,332,337,333,
  336,331,722,225,326,311,265,226,327,331,330,329,328,224,331,320,319,318,
  317,316,315,314,313,312,310,293,292,265,308,307,
136,136,136,136,136,136,136,136,136,136,136,285,136,256,256,256,256,287,289,
  296,298,299,300,301,302,303,304,305,306,321,323,324,325,286,322,288,297,
  309,284,270,135,667,240,294,295,241,290,242,291,267,335,334,332,337,333,
  336,331,723,225,326,311,265,226,327,331,330,329,328,224,331,320,319,318,
  317,316,315,314,313,312,310,293,292,265,308,307,
136,136,136,136,136,136,136,136,136,136,136,285,136,256,256,256,256,287,289,
  296,298,299,300,301,302,303,304,305,306,321,323,324,325,286,322,288,297,
  309,284,270,135,668,240,294,295,241,290,242,291,267,335,334,332,337,333,
  336,331,724,225,326,311,265,226,327,331,330,329,328,224,331,320,319,318,
  317,316,315,314,313,312,310,293,292,265,308,307,
136,136,136,136,136,136,136,136,136,136,136,285,136,256,256,256,256,287,289,
  296,298,299,300,301,302,303,304,305,306,321,323,324,325,286,322,288,297,
  309,284,270,135,669,240,294,295,241,290,242,291,267,335,334,332,337,333,
  336,331,725,225,326,311,265,226,327,331,330,329,328,224,331,320,319,318,
  317,316,315,314,313,312,310,293,292,265,308,307,
136,136,136,136,136,136,136,136,136,136,136,285,136,256,256,256,256,287,289,
  296,298,299,300,301,302,303,304,305,306,321,323,324,325,286,322,288,297,
  309,284,270,135,670,240,294,295,241,290,242,291,267,335,334,332,337,333,
  336,331,726,225,326,311,265,226,327,331,330,329,328,224,331,320,319,318,
  317,316,315,314,313,312,310,293,292,265,308,307,
136,136,136,136,136,136,136,136,136,136,136,285,136,256,256,256,256,287,289,
  296,298,299,300,301,302,303,304,305,306,321,323,324,325,286,322,288,297,
  309,284,270,135,671,240,294,295,241,290,242,291,267,335,334,332,337,333,
  336,331,727,225,326,311,265,226,327,331,330,329,328,224,331,320,319,318,
  317,316,315,314,313,312,310,293,292,265,308,307,
728,1,672,1,728,
136,136,136,136,136,136,136,136,136,136,136,285,136,256,256,256,256,287,289,
  296,298,299,300,301,302,303,304,305,306,321,286,563,288,297,309,284,270,
  135,673,240,294,295,241,290,242,291,267,225,326,311,265,226,218,224,218,
  320,319,318,317,316,315,314,313,312,310,293,292,265,308,307,
136,136,136,136,136,136,136,136,136,136,136,285,136,256,256,256,256,287,289,
  296,298,299,300,301,302,303,304,305,306,321,286,563,288,297,309,284,270,
  135,674,240,294,295,241,290,242,291,267,225,326,311,265,226,217,224,217,
  320,319,318,317,316,315,314,313,312,310,293,292,265,308,307,
136,136,136,136,136,136,136,136,136,136,136,285,136,256,256,256,256,287,289,
  296,298,299,300,301,302,303,304,305,306,321,286,563,288,297,309,284,270,
  135,675,240,294,295,241,290,242,291,267,225,326,311,265,226,216,224,216,
  320,319,318,317,316,315,314,313,312,310,293,292,265,308,307,
136,136,136,136,136,136,136,136,136,136,136,285,136,256,256,256,256,287,289,
  296,298,299,300,301,302,303,304,305,306,321,286,563,288,297,309,284,270,
  135,676,240,294,295,241,290,242,291,267,225,326,311,265,226,215,224,215,
  320,319,318,317,316,315,314,313,312,310,293,292,265,308,307,
136,136,136,136,136,136,136,136,136,136,136,285,136,256,256,256,256,287,289,
  296,298,299,300,301,302,303,304,305,306,321,286,563,288,297,309,284,270,
  135,677,240,294,295,241,290,242,291,267,225,326,311,265,226,214,224,214,
  320,319,318,317,316,315,314,313,312,310,293,292,265,308,307,
136,136,136,136,136,136,136,136,136,136,136,285,136,256,256,256,256,287,289,
  296,298,299,300,301,302,303,304,305,306,321,323,324,325,286,322,288,297,
  309,284,270,135,678,240,294,295,241,290,242,291,267,729,331,225,326,311,
  265,226,327,331,330,329,328,224,331,320,319,318,317,316,315,314,313,312,
  310,293,292,265,308,307,
136,136,136,136,136,136,136,136,136,136,136,285,136,256,256,256,256,287,289,
  296,298,299,300,301,302,303,304,305,306,321,323,324,325,286,322,288,297,
  309,284,270,135,679,240,294,295,241,290,242,291,267,730,331,225,326,311,
  265,226,327,331,330,329,328,224,331,320,319,318,317,316,315,314,313,312,
  310,293,292,265,308,307,
136,136,136,136,136,136,136,136,136,136,136,285,136,256,256,256,256,287,289,
  296,298,299,300,301,302,303,304,305,306,321,323,324,325,286,322,288,297,
  309,284,270,135,680,240,294,295,241,290,242,291,267,332,731,331,225,326,
  311,265,226,327,331,330,329,328,224,331,320,319,318,317,316,315,314,313,
  312,310,293,292,265,308,307,
136,136,136,136,136,136,136,136,136,136,136,285,136,256,256,256,256,287,289,
  296,298,299,300,301,302,303,304,305,306,321,323,324,325,286,322,288,297,
  309,284,270,135,681,240,294,295,241,290,242,291,267,332,732,331,225,326,
  311,265,226,327,331,330,329,328,224,331,320,319,318,317,316,315,314,313,
  312,310,293,292,265,308,307,
136,136,136,136,136,136,136,136,136,136,136,285,136,256,256,256,256,287,289,
  296,298,299,300,301,302,303,304,305,306,321,323,324,325,286,322,288,297,
  309,284,270,135,682,240,294,295,241,290,242,291,267,332,733,331,225,326,
  311,265,226,327,331,330,329,328,224,331,320,319,318,317,316,315,314,313,
  312,310,293,292,265,308,307,
136,136,136,136,136,136,136,136,136,136,136,285,136,256,256,256,256,287,289,
  296,298,299,300,301,302,303,304,305,306,321,323,324,325,286,322,288,297,
  309,284,270,135,683,240,294,295,241,290,242,291,267,332,734,331,225,326,
  311,265,226,327,331,330,329,328,224,331,320,319,318,317,316,315,314,313,
  312,310,293,292,265,308,307,
136,136,136,136,136,136,136,136,136,136,136,285,136,256,256,256,256,287,289,
  296,298,299,300,301,302,303,304,305,306,321,323,324,325,286,322,288,297,
  309,284,270,135,684,240,294,295,241,290,242,291,267,735,332,333,331,225,
  326,311,265,226,327,331,330,329,328,224,331,320,319,318,317,316,315,314,
  313,312,310,293,292,265,308,307,
136,136,136,136,136,136,136,136,136,136,136,285,136,256,256,256,256,287,289,
  296,298,299,300,301,302,303,304,305,306,321,323,324,325,286,322,288,297,
  309,284,270,135,685,240,294,295,241,290,242,291,267,736,332,333,331,225,
  326,311,265,226,327,331,330,329,328,224,331,320,319,318,317,316,315,314,
  313,312,310,293,292,265,308,307,
136,136,136,136,136,136,136,136,136,136,136,285,136,256,256,256,256,287,289,
  296,298,299,300,301,302,303,304,305,306,321,323,324,325,286,322,288,297,
  309,284,270,135,686,240,294,295,241,290,242,291,267,737,334,332,333,331,
  225,326,311,265,226,327,331,330,329,328,224,331,320,319,318,317,316,315,
  314,313,312,310,293,292,265,308,307,
136,136,136,136,136,136,136,136,136,136,136,285,136,256,256,256,256,287,289,
  296,298,299,300,301,302,303,304,305,306,321,323,324,325,286,322,288,297,
  309,284,270,135,687,240,294,295,241,290,242,291,267,738,334,332,333,331,
  225,326,311,265,226,327,331,330,329,328,224,331,320,319,318,317,316,315,
  314,313,312,310,293,292,265,308,307,
136,136,136,136,136,136,136,136,136,136,136,285,136,256,256,256,256,287,289,
  296,298,299,300,301,302,303,304,305,306,321,323,324,325,286,322,288,297,
  309,284,270,135,688,240,294,295,241,290,242,291,267,335,334,332,333,739,
  331,225,326,311,265,226,327,331,330,329,328,224,331,320,319,318,317,316,
  315,314,313,312,310,293,292,265,308,307,
136,136,136,136,136,136,136,136,136,136,136,285,136,256,256,256,256,287,289,
  296,298,299,300,301,302,303,304,305,306,321,323,324,325,286,322,288,297,
  309,284,270,135,689,240,294,295,241,290,242,291,267,335,334,332,333,740,
  331,225,326,311,265,226,327,331,330,329,328,224,331,320,319,318,317,316,
  315,314,313,312,310,293,292,265,308,307,
741,741,741,741,741,1,690,1,741,
43,44,45,40,36,691,36,36,48,36,47,46,46,
43,44,45,40,35,692,35,35,48,35,47,46,46,
43,44,45,40,34,693,34,34,48,34,47,46,46,
43,44,45,40,31,694,31,31,48,31,47,46,46,
742,742,742,742,742,1,695,1,742,
743,743,743,743,743,1,696,1,743,
43,44,45,40,73,697,73,73,48,73,47,46,46,
136,136,136,136,136,136,136,136,136,136,136,285,136,256,256,256,256,287,289,
  296,298,299,300,301,302,303,304,305,306,321,323,324,325,286,322,288,297,
  309,284,270,135,698,240,294,295,241,290,242,291,267,335,334,332,337,333,
  336,331,403,225,326,311,265,226,327,331,330,329,328,224,331,320,319,318,
  317,316,315,314,313,312,310,293,292,265,308,307,
136,136,136,136,136,136,136,136,136,136,136,285,136,256,256,256,256,287,289,
  296,298,299,300,301,302,303,304,305,306,321,323,324,325,286,322,288,297,
  309,284,270,135,699,240,294,295,241,290,242,291,267,335,334,332,337,333,
  336,331,401,225,326,311,265,226,327,331,330,329,328,224,331,320,319,318,
  317,316,315,314,313,312,310,293,292,265,308,307,
136,136,136,136,136,136,136,136,136,136,136,285,136,256,256,256,256,287,289,
  296,298,299,300,301,302,303,304,305,306,321,323,324,325,286,322,288,297,
  309,284,270,135,700,240,294,295,241,290,242,291,267,335,334,332,337,333,
  336,331,399,225,326,311,265,226,327,331,330,329,328,224,331,320,319,318,
  317,316,315,314,313,312,310,293,292,265,308,307,
703,701,744,
271,547,271,271,271,271,271,271,271,271,271,271,271,271,271,271,271,271,271,
  271,271,268,271,271,271,271,271,271,271,271,271,271,271,271,271,271,271,
  271,271,271,271,277,271,702,745,
20,20,20,20,20,20,20,20,20,20,20,20,20,20,20,20,20,20,20,20,20,20,20,20,20,
  20,20,20,20,20,20,20,20,20,20,20,20,20,20,20,20,20,20,20,20,20,20,20,20,
  20,20,20,703,1,945,
746,746,746,746,746,746,746,746,746,746,746,746,746,746,746,746,746,746,746,
  746,746,746,746,746,746,746,746,746,746,746,746,746,746,746,746,746,746,
  746,746,746,746,746,1,704,1,746,
487,705,488,
20,327,327,327,327,327,1,706,1,316,
271,547,271,271,271,271,271,271,271,271,271,271,271,271,271,271,271,271,271,
  271,271,271,271,271,271,271,271,271,271,271,271,271,271,271,271,271,271,
  271,271,271,277,271,707,142,
747,747,747,747,747,747,747,747,747,747,747,747,747,747,747,747,747,747,747,
  747,747,747,747,747,747,747,747,747,747,747,747,747,747,747,747,747,747,
  747,747,747,747,1,708,1,747,
703,103,103,103,103,103,103,709,704,
748,748,748,748,748,748,748,748,748,748,748,748,748,1,710,1,748,
20,20,20,20,20,20,20,20,20,20,20,20,20,20,20,20,20,20,20,20,20,20,20,20,20,
  20,20,20,20,20,20,20,20,20,20,20,20,20,20,20,20,20,20,20,20,20,711,1,
  913,
68,68,68,68,68,68,68,68,68,68,68,68,68,68,68,68,68,68,68,68,68,68,68,68,68,
  68,68,68,68,68,68,68,68,68,68,68,68,68,68,68,18,18,18,18,18,18,712,
43,44,45,40,64,713,64,64,48,64,47,46,46,
136,136,136,136,136,136,136,136,136,136,136,285,136,256,256,256,256,287,289,
  296,298,299,300,301,302,303,304,305,306,321,323,324,325,286,322,288,297,
  309,284,270,135,714,240,294,295,241,290,242,291,267,335,334,332,337,333,
  336,331,749,225,326,311,265,226,327,331,330,329,328,224,331,320,319,318,
  317,316,315,314,313,312,310,293,292,265,308,307,
161,161,161,161,161,161,161,161,160,160,160,160,161,161,161,161,160,160,160,
  160,160,161,161,161,161,161,161,161,161,161,161,161,161,161,161,161,161,
  161,161,161,161,161,715,
157,157,157,157,157,716,
136,136,136,136,136,136,136,136,136,136,136,136,135,717,752,750,750,751,
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
212,564,566,391,383,387,212,212,212,212,212,212,212,212,212,212,212,212,212,
  212,212,212,212,212,212,212,212,212,212,212,212,212,212,212,212,212,212,
  729,570,569,568,567,565,
211,564,566,391,383,387,211,211,211,211,211,211,211,211,211,211,211,211,211,
  211,211,211,211,211,211,211,211,211,211,211,211,211,211,211,211,211,211,
  730,570,569,568,567,565,
209,384,386,209,209,209,209,209,209,209,209,209,209,209,209,209,209,209,209,
  209,209,209,209,209,209,209,209,209,209,209,209,209,209,209,731,572,571,
208,384,386,208,208,208,208,208,208,208,208,208,208,208,208,208,208,208,208,
  208,208,208,208,208,208,208,208,208,208,208,208,208,208,208,732,572,571,
207,384,386,207,207,207,207,207,207,207,207,207,207,207,207,207,207,207,207,
  207,207,207,207,207,207,207,207,207,207,207,207,207,207,207,733,572,571,
206,384,386,206,206,206,206,206,206,206,206,206,206,206,206,206,206,206,206,
  206,206,206,206,206,206,206,206,206,206,206,206,206,206,206,734,572,571,
204,573,575,577,579,204,204,204,204,204,204,204,204,204,204,204,204,204,204,
  204,204,204,204,204,204,204,204,204,204,204,204,204,735,580,578,576,574,
203,573,575,577,579,203,203,203,203,203,203,203,203,203,203,203,203,203,203,
  203,203,203,203,203,203,203,203,203,203,203,203,203,736,580,578,576,574,
201,581,390,201,201,201,201,201,201,201,201,201,201,201,201,201,201,201,201,
  201,201,201,201,201,201,201,201,201,737,583,582,
200,581,390,200,200,200,200,200,200,200,200,200,200,200,200,200,200,200,200,
  200,200,200,200,200,200,200,200,200,738,583,582,
198,584,362,198,198,198,198,198,198,198,198,198,198,198,198,198,198,198,198,
  198,198,198,198,198,198,198,739,586,585,
197,584,362,197,197,197,197,197,197,197,197,197,197,197,197,197,197,197,197,
  197,197,197,197,197,197,197,740,586,585,
43,44,45,40,47,741,47,47,48,47,47,46,46,
43,44,45,40,30,742,30,30,48,30,47,46,46,
43,44,45,40,29,743,29,29,48,29,47,46,46,
753,753,753,753,753,753,753,753,753,1,744,1,753,
142,142,142,142,142,142,142,142,142,142,142,142,142,142,142,142,142,142,142,
  142,142,269,142,142,142,142,142,142,142,142,142,142,142,142,142,142,142,
  142,142,142,142,142,142,745,
136,136,136,136,136,136,136,136,136,136,136,285,136,256,256,256,256,287,636,
  145,296,298,299,300,301,302,303,304,305,306,321,323,324,325,286,322,288,
  297,309,284,270,135,746,240,294,295,241,290,129,278,242,291,637,130,267,
  335,334,332,337,333,336,331,128,225,326,311,265,226,327,331,330,329,328,
  224,331,320,319,318,317,316,315,314,313,312,310,293,292,279,265,638,308,
  307,
136,136,136,136,136,136,136,136,136,136,136,285,136,256,256,256,256,287,289,
  296,298,299,300,301,302,303,304,305,306,321,323,324,325,286,322,288,297,
  309,284,270,135,747,240,294,295,241,290,242,291,267,335,334,332,337,333,
  336,331,107,225,326,311,265,226,327,331,330,329,328,224,331,320,319,318,
  317,316,315,314,313,312,310,293,292,265,308,307,
136,136,136,136,136,136,136,136,136,136,136,136,135,748,132,265,265,
754,754,754,754,754,1,749,1,754,
137,137,137,137,137,137,137,137,137,137,137,137,138,138,138,138,138,20,137,
  1,750,1,133,138,
20,20,751,1,989,
755,1,752,1,755,
292,291,291,288,287,286,285,284,283,753,388,291,628,
43,44,45,40,62,754,62,62,48,62,47,46,46,
388,755,239,

};


static const unsigned short ag_sbt[] = {
     0,  29,  31,  70,  88, 119, 149, 348, 351, 369, 568, 935, 972, 989,
  1006,1011,1016,1062,1067,1076,1085,1102,1147,1192,1209,1218,1243,1261,
  1278,1283,1288,1334,1339,1348,1357,1374,1461,1548,1565,1576,1585,1642,
  1734,1740,1788,1839,1885,1977,2064,2151,2525,2536,2546,2556,2566,2576,
  2586,2596,2606,2616,2626,2636,2646,2656,2666,2676,2689,2702,2713,2758,
  2803,2848,2893,2938,2983,3028,3073,3118,3163,3208,3253,3298,3343,3388,
  3433,3443,3453,3463,3473,3483,3493,3503,3513,3523,3533,3543,3553,3563,
  3573,3583,3593,3603,3613,3623,3633,3643,3653,3663,3673,3683,3693,3703,
  3713,3723,3731,3744,3757,3768,3813,3858,3903,3948,3993,4038,4083,4128,
  4173,4218,4263,4308,4353,4398,4443,4488,4533,4578,4623,4668,4713,4758,
  4803,4848,4893,4938,4983,5028,5073,5118,5163,5208,5253,5298,5343,5388,
  5433,5478,5523,5568,5613,5658,5703,5748,5793,5838,5883,5928,5940,5952,
  5964,5976,5984,5992,6003,6014,6025,6038,6051,6064,6077,6090,6103,6116,
  6129,6142,6155,6168,6181,6194,6202,6207,6212,6217,6262,6307,6352,6397,
  6442,6487,6532,6577,6622,6667,6712,6757,6802,6847,6892,6937,6982,7027,
  7072,7117,7162,7207,7252,7297,7342,7387,7432,7477,7522,7567,7612,7657,
  7702,7747,7792,7837,7882,7927,7972,8017,8062,8107,8152,8197,8242,8287,
  8332,8377,8422,8467,8512,8557,8569,8581,8593,8605,8613,8621,8632,8643,
  8654,8667,8680,8693,8706,8719,8732,8745,8758,8771,8784,8797,8810,8823,
  8884,8894,8965,8973,8976,9063,9142,9151,9195,9204,9209,9214,9303,9347,
  9359,9368,9381,9394,9403,9409,9414,9417,9472,9478,9522,9528,9539,9583,
  9625,9674,9727,9732,9742,9747,9752,9757,9762,9767,9772,9777,9782,9787,
  9829,9871,9936,9941,9946,9951,9956,9961,9966,9971,9976,9981,9986,9991,
  10040,10082,10146,10188,10252,10297,10367,10437,10507,10577,10619,10664,
  10703,10742,10775,10806,10811,10850,10935,10944,10953,10961,10970,10979,
  10988,10997,11014,11031,11040,11049,11058,11067,11109,11118,11127,11136,
  11161,11191,11242,11307,11358,11423,11471,11522,11573,11624,11675,11726,
  11777,11828,11879,11930,11981,12032,12083,12134,12185,12236,12287,12338,
  12389,12440,12504,12569,12634,12699,12763,12838,12889,12954,13018,13069,
  13120,13171,13228,13312,13315,13318,13363,13448,13531,13540,13558,13575,
  13588,13601,13607,13610,13613,13616,13701,13786,13871,13956,14041,14126,
  14211,14296,14381,14466,14551,14636,14721,14806,14891,14976,15061,15146,
  15231,15316,15401,15486,15571,15656,15741,15826,15911,15996,16081,16166,
  16251,16336,16421,16506,16591,16676,16761,16846,16931,17016,17101,17186,
  17271,17356,17441,17526,17611,17696,17781,17866,17951,18036,18051,18066,
  18081,18096,18104,18112,18125,18138,18151,18168,18185,18198,18211,18224,
  18237,18250,18263,18276,18289,18302,18315,18328,18421,18443,18580,18717,
  18734,18751,18774,18796,18805,18814,18820,18829,18838,18847,18869,18892,
  18897,18902,18948,18994,19086,19131,19216,19233,19250,19267,19284,19301,
  19319,19336,19354,19371,19389,19435,19481,19574,19620,19666,19712,19805,
  19850,19895,19980,19989,20034,20079,20124,20209,20226,20237,20248,20261,
  20343,20356,20359,20368,20377,20386,20401,20414,20427,20437,20444,20489,
  20539,20589,20592,20595,20598,20601,20604,20607,20610,20613,20616,20619,
  20622,20707,20713,20755,20797,20839,20881,20923,20965,21007,21052,21097,
  21142,21187,21232,21277,21322,21367,21412,21457,21502,21547,21592,21637,
  21682,21727,21772,21817,21862,21907,21952,21997,22042,22087,22132,22177,
  22222,22267,22312,22357,22402,22415,22428,22441,22454,22467,22484,22497,
  22510,22554,22567,22580,22606,22615,22624,22633,22642,22687,22772,22817,
  22902,22915,22924,22933,22944,22955,22966,22978,22989,23034,23079,23124,
  23134,23144,23149,23193,23238,23249,23254,23264,23285,23576,23620,23623,
  23716,23805,23814,23823,23832,23841,23850,23938,23983,23996,24009,24022,
  24032,24038,24081,24083,24100,24185,24270,24355,24440,24525,24610,24695,
  24780,24865,24950,24955,25025,25095,25165,25235,25305,25384,25463,25543,
  25623,25703,25783,25864,25945,26027,26109,26192,26275,26284,26297,26310,
  26323,26336,26345,26354,26367,26452,26537,26622,26625,26670,26725,26771,
  26774,26784,26828,26873,26882,26899,26948,26995,27008,27093,27136,27142,
  27160,27163,27166,27169,27172,27175,27178,27181,27184,27187,27190,27193,
  27236,27279,27316,27353,27390,27427,27464,27501,27532,27563,27592,27621,
  27634,27647,27660,27673,27717,27809,27894,27911,27920,27944,27949,27954,
  27967,27980,27983
};


static const unsigned short ag_sbe[] = {
    26,  30,  56,  85, 117, 133, 345, 349, 359, 565, 763, 960, 986,1003,
  1008,1013,1059,1064,1073,1082,1099,1144,1189,1206,1215,1240,1256,1274,
  1280,1285,1331,1335,1345,1354,1370,1415,1502,1561,1570,1578,1639,1731,
  1736,1785,1836,1882,1930,2020,2107,2346,2533,2543,2553,2563,2573,2583,
  2593,2603,2613,2623,2633,2643,2653,2663,2673,2686,2699,2710,2755,2800,
  2845,2890,2935,2980,3025,3070,3115,3160,3205,3250,3295,3340,3385,3430,
  3440,3450,3460,3470,3480,3490,3500,3510,3520,3530,3540,3550,3560,3570,
  3580,3590,3600,3610,3620,3630,3640,3650,3660,3670,3680,3690,3700,3710,
  3720,3728,3741,3754,3765,3810,3855,3900,3945,3990,4035,4080,4125,4170,
  4215,4260,4305,4350,4395,4440,4485,4530,4575,4620,4665,4710,4755,4800,
  4845,4890,4935,4980,5025,5070,5115,5160,5205,5250,5295,5340,5385,5430,
  5475,5520,5565,5610,5655,5700,5745,5790,5835,5880,5925,5937,5949,5961,
  5973,5981,5989,6000,6011,6022,6035,6048,6061,6074,6087,6100,6113,6126,
  6139,6152,6165,6178,6191,6199,6204,6209,6214,6259,6304,6349,6394,6439,
  6484,6529,6574,6619,6664,6709,6754,6799,6844,6889,6934,6979,7024,7069,
  7114,7159,7204,7249,7294,7339,7384,7429,7474,7519,7564,7609,7654,7699,
  7744,7789,7834,7879,7924,7969,8014,8059,8104,8149,8194,8239,8284,8329,
  8374,8419,8464,8509,8554,8566,8578,8590,8602,8610,8618,8629,8640,8651,
  8664,8677,8690,8703,8716,8729,8742,8755,8768,8781,8794,8807,8820,8880,
  8891,8963,8971,8974,9017,9100,9148,9192,9201,9205,9210,9256,9345,9356,
  9365,9373,9386,9400,9408,9413,9416,9471,9477,9520,9527,9538,9582,9624,
  9673,9725,9729,9739,9744,9749,9754,9759,9764,9769,9774,9779,9784,9826,
  9868,9933,9938,9943,9948,9953,9958,9963,9968,9973,9978,9983,9988,10037,
  10079,10143,10185,10249,10294,10335,10405,10475,10545,10616,10656,10698,
  10735,10770,10801,10808,10830,10891,10941,10950,10959,10967,10976,10985,
  10994,11011,11028,11037,11046,11055,11064,11108,11115,11124,11133,11158,
  11182,11239,11304,11355,11420,11468,11519,11570,11621,11672,11723,11774,
  11825,11876,11927,11978,12029,12080,12131,12182,12233,12284,12335,12386,
  12437,12501,12566,12631,12696,12760,12835,12886,12951,13015,13066,13117,
  13168,13225,13270,13313,13316,13360,13404,13487,13537,13547,13567,13584,
  13595,13605,13608,13611,13614,13657,13742,13827,13912,13997,14082,14167,
  14252,14337,14422,14507,14592,14677,14762,14847,14932,15017,15102,15187,
  15272,15357,15442,15527,15612,15697,15782,15867,15952,16037,16122,16207,
  16292,16377,16462,16547,16632,16717,16802,16887,16972,17057,17142,17227,
  17312,17397,17482,17567,17652,17737,17822,17907,17992,18044,18059,18074,
  18089,18100,18108,18119,18132,18145,18160,18177,18194,18207,18220,18233,
  18246,18259,18272,18285,18298,18311,18324,18370,18434,18577,18714,18731,
  18747,18771,18793,18802,18811,18817,18822,18835,18844,18866,18888,18893,
  18898,18945,18991,19037,19128,19172,19230,19246,19264,19280,19298,19314,
  19333,19349,19368,19384,19432,19478,19523,19617,19663,19709,19754,19847,
  19892,19936,19986,20031,20076,20121,20165,20222,20234,20245,20253,20301,
  20348,20357,20365,20374,20383,20399,20406,20419,20436,20443,20487,20538,
  20588,20590,20593,20596,20599,20602,20605,20608,20611,20614,20617,20620,
  20663,20712,20752,20794,20836,20878,20920,20962,21004,21049,21094,21139,
  21184,21229,21274,21319,21364,21409,21454,21499,21544,21589,21634,21679,
  21724,21769,21814,21859,21904,21949,21994,22039,22084,22129,22174,22219,
  22264,22309,22354,22399,22407,22420,22433,22446,22459,22480,22489,22502,
  22553,22559,22572,22603,22612,22621,22630,22639,22684,22728,22814,22858,
  22907,22921,22930,22941,22952,22963,22975,22986,23031,23076,23121,23131,
  23141,23146,23191,23236,23246,23251,23263,23283,23418,23618,23621,23665,
  23758,23812,23821,23830,23839,23848,23894,23980,23988,24001,24014,24031,
  24037,24080,24082,24097,24141,24226,24311,24396,24481,24566,24651,24736,
  24821,24906,24952,24993,25063,25133,25203,25273,25346,25425,25504,25584,
  25664,25744,25824,25905,25986,26068,26150,26233,26281,26289,26302,26315,
  26328,26342,26351,26359,26408,26493,26578,26623,26668,26722,26768,26772,
  26781,26826,26870,26880,26896,26945,26994,27000,27049,27135,27141,27155,
  27161,27164,27167,27170,27173,27176,27179,27182,27185,27188,27191,27230,
  27273,27313,27350,27387,27424,27459,27496,27529,27560,27589,27618,27626,
  27639,27652,27670,27716,27759,27850,27907,27917,27940,27946,27951,27963,
  27972,27981,27983
};


static const unsigned char ag_fl[] = {
  2,1,1,2,2,1,1,2,0,1,3,3,1,1,2,1,2,2,1,2,0,1,4,5,3,2,1,0,1,7,7,6,1,1,6,
  6,6,2,3,2,2,1,2,4,4,4,4,6,4,4,0,1,4,4,4,4,3,3,4,5,5,5,8,4,6,4,1,1,2,2,
  2,2,3,5,1,2,2,2,3,2,1,1,1,3,2,1,1,3,1,1,1,3,1,1,3,3,3,3,3,3,3,3,3,4,4,
  4,3,6,2,3,2,1,1,2,2,3,3,2,2,3,3,1,1,2,3,1,1,1,4,4,4,1,4,2,1,1,1,2,2,1,
  2,2,3,2,2,1,2,3,1,2,2,2,2,2,2,2,2,4,1,1,4,3,2,1,2,3,3,2,1,1,1,1,1,1,2,
  1,1,1,1,2,1,1,2,1,1,2,1,1,2,1,1,2,1,1,2,1,2,4,4,2,4,4,2,4,4,2,4,4,4,4,
  2,4,4,2,4,4,4,4,4,1,2,2,2,2,1,1,1,1,5,5,5,5,5,5,5,5,5,5,5,7,1,1,1,1,1,
  1,1,2,2,2,1,1,2,2,2,2,1,2,2,3,2,2,2,2,2,2,2,2,3,4,1,1,2,2,2,2,2,1,2,1,
  2,2,2,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,
  1,2,1,1,1,1,1,1,1,1,1,1,4,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,
  1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,3,3,3,3,3,3,3,3,3,
  3,3,3,7,3,3,3,3,3,3,3,3,3,3,5,3,5,3,5,3,3,3,3,3,3,3,3,3,3,3,3,3,3,3,3,
  1,1,1,1,1,3,3,3,3,1,1,1,1,1,3,3,3,3,3,3,3,1,1,1,3,1,1,1,3,3,3,3,3,3,3,
  3,3,3,3,3,3,3,3,3,3,3,3,3,3,3,3,3,3,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,
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
  2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,
  2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,
  2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,
  2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,
  2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,
  2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,
  2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,
  2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,
  2,2,2
};

static const unsigned short ag_ptt[] = {
    0, 36, 37, 37, 34, 39, 43, 43, 44, 44, 39, 39, 39, 39, 40, 47, 47, 47,
   51, 51, 52, 52, 35, 35, 35, 35, 35, 55, 55, 35, 35, 35, 62, 62, 35, 35,
   35, 35, 35, 35, 35, 65, 53, 67, 67, 67, 67, 67, 67, 67, 75, 75, 67, 67,
   67, 67, 67, 67, 67, 67, 67, 67, 67, 67, 67, 89, 89, 90, 90, 90, 97, 97,
   63, 63, 33, 33, 33, 33, 33, 33,103,103,103, 33, 33,107,107, 33,111,111,
  111, 33,114,114, 33, 33, 33, 33, 33, 33, 33, 33, 33, 33, 33, 33, 33, 33,
   33, 33, 33, 33, 33, 33, 33, 33, 33, 33, 33, 33, 33, 33, 33, 30, 30, 92,
   92, 92, 92, 92, 92,116,116,536,441,441,133,133,133,436,436,484, 18, 18,
  434, 12, 12, 12,141,141,141,141,141,141,141,141,141,141,154,154,141,141,
  435, 14, 14, 14, 14,427,427,427,427,427, 22, 22, 77, 77,168,168,168,165,
  171,171,165,174,174,165,177,177,165,180,180,165,183,183,165, 57, 26, 26,
   26, 28, 28, 28, 23, 23, 23, 24, 24, 24, 24, 24, 27, 27, 27, 25, 25, 25,
   25, 25, 25, 29, 29, 29, 29, 29,197,197,197,197,197,206,206,206,206,206,
  206,206,206,206,206,206,205,205,491,491,491,491,491,491,491,491,222,222,
  491,491,  7,  7,  7,  7,  7,  6,  6,  6, 17, 17,217,217,218,218, 15, 15,
   15, 16, 16, 16, 16, 16, 16, 16, 16,538,  9,  9,  9,594,594,594,594,594,
  594,241,241,594,594,244,244,608,246,246,608,248,248,608,608,250,250,250,
  250,250,252,252,253,253,253,253,253,253, 31, 32, 32, 32, 32, 32, 32, 32,
   32, 32, 32, 32,271,271,271,271,265,265,265,265,265,278,278,265,265,265,
  284,284,284,265,265,265,265,265,265,265,265,265,295,295,265,265,265,265,
  265,265,265,265,265,265,308,308,308,265,265,265,265,265,265,265,258,258,
  259,259,259,259,256,256,256,256,256,256,256,256,256,256,256,256,256,257,
  257,257,337,261,339,262,341,263,264,260,260,260,260,260,260,260,260,260,
  260,260,260,260,260,260,365,365,365,365,365,260,260,260,260,374,374,374,
  374,374,260,260,260,260,260,260,260,384,384,384,260,388,388,388,260,260,
  260,260,260,260,260,260,260,260,260,260,260,260,260,260,260,260,260,260,
  260,260,260,260,260,137,344,152,225, 42, 42, 42, 42, 42, 42, 42, 42, 42,
   42, 42, 42, 42, 42, 42, 42, 42, 42, 42, 42, 42, 42, 42, 42, 42, 42, 42,
   42, 42, 42, 42, 42, 42, 42, 42, 42, 42, 42, 42, 42, 42, 42, 50, 50, 50,
   50, 50, 50, 50, 50, 50, 50, 50, 50, 50, 50, 50, 50, 50, 50, 50, 50, 50,
   50, 50, 50, 50, 50, 50, 50, 50, 50, 50, 50, 50, 50, 50, 50, 50, 50, 50,
   50, 50, 50, 50, 94, 94, 94, 94, 94, 94, 94, 94, 94, 94, 94, 94, 94, 94,
   94, 94, 94, 94, 94, 94, 94, 94, 94, 94, 94, 94, 94, 94, 94, 94, 94, 94,
   94, 94, 94, 94, 94, 94, 94, 94, 95, 95, 95, 95, 95, 95, 95, 95, 95, 95,
   95, 95, 95, 95, 95, 95, 95, 95, 95, 95, 95, 95, 95, 95, 95, 95, 95, 95,
   95, 95, 95, 95, 95, 95, 95, 95, 95, 95, 95, 95, 95,135,135,135,135,135,
  135,135,135,135,135,135,135,136,136,136,136,136,136,136,136,136,136,136,
  136,136,138,138,138,138,138,138,138,138,138,138,138,138,138,138,138,138,
  138,138,138,143,143,143,143,143,143,143,143,143,143,143,143,143,143,143,
  143,143,143,143,143,143,143,143,143,143,143,143,143,143,143,143,143,143,
  143,143,143,143,143,143,143,150,150,150,151,151,151,151,151,155,155,155,
  155,155,155,155,155,155,158,158,158,158,158,158,158,158,158,158,158,158,
  158,158,158,158,158,158,158,158,158,158,158,158,158,158,158,158,158,158,
  158,158,158,158,158,158,158,158,158,159,159,159,159,159,159,159,159,159,
  159,159,159,159,159,159,159,159,159,159,159,159,159,159,159,159,159,159,
  159,159,159,159,159,159,159,159,159,159,159,159,159,159,159,226,226,226,
  226,227,227,229,229,229,229,231,231,231,231,231,231,231,231,231,231,231,
  231,231,231,231,231,231,231,231,231,231,231,231,231,231,231,231,231,231,
  231,231,231,231,231,231,231,231,231,231,231, 45, 41, 46, 48, 49, 11, 54,
   56, 58, 59, 60, 61, 10, 13, 19, 66, 69, 68, 70, 21, 71, 72, 73, 74, 76,
   78, 79, 80, 81, 82, 83, 84, 86, 85, 87, 88, 93, 91, 96, 98, 99,100,101,
  102,104,105,106,108,109,110,112,113,115,117,118,119,120,121,122,123,124,
  125, 20,126,127,128,129,130,131,  2,132,164,166,167,169,170,172,173,175,
  176,178,179,181,182,184,185,186,187,188,189,190,191,192,193,194,195,196,
  198,199,200,201,202,203,204,153,207,208,209,210,211,212,213,214,215,  3,
  216,  8,251,242,254,255,249,266,267,268,269,270,272,273,274,275,276,277,
  279,280,281,282,283,285,286,287,288,289,290,291,292,293,294,296,297,298,
  299,300,301,302,303,304,305,306,307,309,310,311,312,313,314,315,316,317,
  318,319,320,  4,321,322,323,324,325,326,327,328,329,330,331,332,333,  5,
  334,335,336,338,340,342,343,345,346,347,348,349,350,351,352,353,354,355,
  356,357,358,359,360,361,362,363,364,366,367,368,369,370,371,372,373,375,
  376,377,378,379,380,381,382,383,385,386,387,389,390,391,392,393,394,395,
  396,397,398,399,400,243,401,402,403,404,405,406,407,408,409,410,411,413,
  140,139,147,414,415,416,417,157,156,418,228,419,148,223,149,420,219,144,
  146,145,412,142,224,421
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
    case 86: ag_rp_86(V(0,int)); break;
    case 87: ag_rp_87(V(1,int)); break;
    case 88: ag_rp_88(V(1,int)); break;
    case 89: ag_rp_89(V(0,int)); break;
    case 90: ag_rp_90(V(1,int)); break;
    case 91: ag_rp_91(V(1,int), V(2,int)); break;
    case 92: ag_rp_92(V(1,int)); break;
    case 93: ag_rp_93(); break;
    case 94: ag_rp_94(V(1,int)); break;
    case 95: V(0,int) = ag_rp_95(V(0,int)); break;
    case 96: V(0,int) = ag_rp_96(); break;
    case 97: V(0,int) = ag_rp_97(); break;
    case 98: V(0,int) = ag_rp_98(); break;
    case 99: V(0,int) = ag_rp_99(); break;
    case 100: V(0,int) = ag_rp_100(); break;
    case 101: V(0,int) = ag_rp_101(); break;
    case 102: V(0,int) = ag_rp_102(); break;
    case 103: V(0,int) = ag_rp_103(); break;
    case 104: V(0,int) = ag_rp_104(V(1,int), V(2,int), V(3,int)); break;
    case 105: V(0,int) = ag_rp_105(V(2,int), V(3,int)); break;
    case 106: V(0,int) = ag_rp_106(V(2,int)); break;
    case 107: ag_rp_107(); break;
    case 108: ag_rp_108(); break;
    case 109: ag_rp_109(V(1,int)); break;
    case 110: ag_rp_110(V(2,int)); break;
    case 111: ag_rp_111(); break;
    case 112: ag_rp_112(); break;
    case 113: ag_rp_113(); break;
    case 114: ag_rp_114(); break;
    case 115: ag_rp_115(); break;
    case 116: V(0,int) = ag_rp_116(); break;
    case 117: V(0,int) = ag_rp_117(); break;
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
    case 142: ag_rp_142(); break;
    case 143: ag_rp_143(); break;
    case 144: ag_rp_144(); break;
    case 145: ag_rp_145(); break;
    case 146: ag_rp_146(); break;
    case 147: ag_rp_147(V(0,double)); break;
    case 148: ag_rp_148(); break;
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
const char *const a85parse_token_names[705] = {
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
  "'$'",
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
    0,0,  0,0,  0,0, 65,1,427,1, 53,1, 35,1, 35,1, 35,1, 35,1, 35,1,  0,0,
   67,1, 67,1,  0,0,  0,0,  0,0, 67,1,  0,0,  0,0, 67,1, 67,1,  0,0, 67,1,
   67,1,  0,0, 67,1, 67,1, 67,1, 67,1, 67,1, 67,1, 67,1, 67,1, 67,1, 67,1,
   67,1, 67,1, 67,1, 67,1,  0,0,  0,0, 35,2, 47,1,  0,0, 39,1, 40,1, 39,1,
   39,1, 35,2, 63,1,265,1,265,1,265,1,265,1,265,1,265,1,265,1,265,1,265,1,
  265,1,265,1,265,1,265,1,265,1,341,1,339,1,337,1,260,1,260,1,260,1,260,1,
  260,1,260,1,260,1,260,1,260,1,260,1,260,1,260,1,260,1,260,1,260,1,260,1,
  265,1,265,1,265,1,265,1,265,1,265,1,265,1,265,1,265,1,265,1,265,1,265,1,
  265,1,265,1,265,1,265,1,265,1,265,1,265,1,265,1,265,1,265,1,265,1,265,1,
  265,1,265,1,265,1,265,1,265,1,264,1,341,1,339,1,337,1,260,1,260,1,260,1,
  260,1,260,1,260,1,260,1,260,1,260,1,260,1,260,1,260,1,260,1,260,1,260,1,
  260,1,260,1,260,1,260,1,260,1,260,1,260,1,260,1,260,1,260,1,260,1,260,1,
  260,1,260,1,260,1,260,1,260,1,260,1,260,1,260,1,260,1,260,1,260,1,260,1,
  260,1,260,1,260,1,260,1,260,1,260,1,260,1,260,1,260,1,259,1,259,1,259,1,
  259,1,258,1,258,1,257,1,257,1,257,1,256,1,256,1,256,1,256,1,256,1,256,1,
  256,1,256,1,256,1,256,1,256,1,256,1,256,1,264,1,263,1,262,1,261,1,260,1,
  260,1,260,1,260,1,260,1,260,1,260,1,260,1,260,1,260,1,260,1,260,1,260,1,
  260,1,260,1,260,1,260,1,260,1,260,1,260,1,260,1,260,1,260,1,260,1,260,1,
  260,1,260,1,260,1,260,1,260,1,260,1,260,1,260,1,260,1,260,1,260,1,260,1,
  260,1,260,1,260,1,260,1,260,1,260,1,260,1,260,1,260,1,260,1,260,1,260,1,
  260,1,260,1,260,1,259,1,259,1,259,1,259,1,258,1,258,1,257,1,257,1,257,1,
  256,1,256,1,256,1,256,1,256,1,256,1,256,1,256,1,256,1,256,1,256,1,256,1,
  256,1,  0,0, 32,1, 89,1, 30,1, 32,1, 33,1, 33,1, 35,2, 67,2, 67,2, 67,2,
   67,2, 67,2,434,1,  0,0, 67,2, 67,2, 67,2, 67,2,538,1,218,1,217,1,  6,1,
    7,1, 15,1,538,1,491,1,218,1,217,1,  6,1,205,1,206,1,  0,0,206,1,206,1,
  206,1,206,1,206,1,206,1,206,1,206,1,206,1,205,1,  0,0,  0,0,206,1,206,1,
  206,1,206,1,206,1,206,1,206,1,206,1,206,1,206,1,206,1,197,1, 29,1,  0,0,
   29,1,  0,0,197,1, 29,1, 29,1, 29,1, 29,1, 25,1, 27,1, 24,1, 23,1, 28,1,
   26,1, 26,1, 77,1, 77,1, 67,2, 67,2, 67,2,  0,0, 67,2,  0,0, 67,2,  0,0,
   67,2,  0,0, 67,2,  0,0, 67,2,435,1,  0,0, 67,2, 67,2,  0,0, 35,3,  0,0,
    0,0,  0,0,  0,0,  0,0,  0,0,  0,0,  0,0,  0,0,  0,0,  0,0,  0,0,  0,0,
    0,0,  0,0,  0,0,  0,0,  0,0,  0,0,  0,0,  0,0,  0,0,  0,0,  0,0,  0,0,
    0,0,  0,0,  0,0,  0,0,  0,0,  0,0,  0,0,  0,0,  0,0,  0,0,  0,0,  0,0,
    0,0, 39,2, 39,2,  0,0, 35,3, 33,1, 35,3, 63,2,341,2,339,2,337,2,264,2,
  263,2,262,2,261,2,260,2,260,2,260,2,260,2,260,2,260,2,260,2,260,2,260,2,
  260,2,260,2,260,2,260,2,260,2,260,2,260,2,260,2,260,2,260,2,260,2,260,2,
  260,2,260,2,260,2,260,2,260,2,260,2,260,2,260,2,260,2,260,2,260,2,260,2,
  260,2,260,2,260,2,260,2,260,2,260,2,260,2,260,2,260,2,260,2,260,2,260,2,
  260,2,260,2,260,2,260,2,260,2,260,2,260,2,259,2,259,2,259,2,259,2,258,2,
  258,2,257,2,257,2,257,2,256,2,256,2,256,2,256,2,256,2,256,2,256,2,256,2,
  256,2,256,2,256,2,256,2,256,2, 89,2,  0,0,  0,0, 32,2,  0,0, 33,2,  0,0,
   33,2,  0,0,  0,0,  0,0, 33,2,  0,0,  0,0,  0,0, 33,2, 33,2, 33,2,  0,0,
   33,2, 33,2,  0,0, 33,2,  0,0, 33,2,  0,0, 33,2,  0,0, 33,2,  0,0, 33,2,
    0,0, 33,2,  0,0,  0,0, 33,2,  0,0,  0,0,  0,0, 33,2,  0,0,  0,0, 33,2,
    0,0,  0,0,  0,0,  0,0, 33,2, 33,2,  0,0,  0,0, 35,3, 67,3, 67,3, 67,3,
   67,3, 67,3, 67,3,  0,0, 67,3, 67,3,  6,2, 16,1, 15,2,  0,0,  0,0,206,2,
  206,2,206,2,206,2,206,2,206,2,206,2,206,2,206,2,206,2,206,2,197,2,  7,1,
    0,0, 25,2,  0,0, 25,2, 25,2, 25,2, 25,2, 27,2, 27,2,  0,0, 24,2,  0,0,
   24,2,  0,0, 24,2,  0,0, 24,2,  0,0, 23,2, 23,2,  0,0, 28,2, 28,2,  0,0,
   26,2, 26,2,  0,0,  0,0,  0,0,  0,0,  0,0,  0,0,  0,0,  0,0,  0,0,  0,0,
    0,0,  0,0, 67,3, 67,3, 67,3, 67,3, 67,3, 67,3, 67,3, 67,3,435,2, 67,3,
   67,3,  0,0, 35,4, 35,4, 35,4, 35,4,  0,0, 35,4,  0,0, 35,4, 35,4, 22,1,
   63,3,253,1,253,1,253,1,  0,0,  0,0,263,3,262,3,261,3,250,1,250,1,256,3,
    0,0,484,1,  0,0,  0,0,  6,1,491,1, 32,3,484,1, 33,3, 33,3, 33,3,116,1,
  116,1,116,1,  0,0,  0,0, 90,1, 67,4, 67,4, 67,4, 67,4,  0,0,  0,0,  0,0,
   15,3,206,3,206,3,206,3,206,3,206,3,206,3,206,3,206,3,206,3,206,3,206,3,
  197,3, 25,3, 25,3, 25,3, 25,3, 25,3, 27,3, 27,3, 24,3, 24,3, 24,3, 24,3,
   23,3, 23,3, 28,3, 28,3, 26,3, 26,3, 67,4, 35,5, 35,5, 35,5, 35,5, 35,5,
   35,5, 63,4,263,4,262,4,261,4,256,4,  0,0,  0,0,  0,0, 32,1, 32,1,484,2,
   33,4,  0,0,116,2,  0,0,  0,0, 67,5, 67,5,  0,0,  0,0,206,4,206,4,206,4,
  206,4,206,4,206,4,206,4,206,4,206,4,206,4,206,4,197,4, 25,1, 25,1, 27,1,
   27,1, 27,1, 27,1, 24,1, 24,1, 23,1, 23,1, 28,1, 28,1, 67,5, 35,6, 35,6,
  256,5, 15,3,  0,0, 33,5,116,3, 67,6,536,1,  0,0,206,5,256,6, 67,7,206,6
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


