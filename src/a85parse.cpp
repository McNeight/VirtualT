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

#ifndef A85PARSE_H_1359807348
#include "a85parse.h"
#endif

#ifndef A85PARSE_H_1359807348
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

/*  Line 700, C:/Projects/VirtualT/src/a85parse.syn */
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
/* Line 126, C:/Projects/VirtualT/src/a85parse.syn */
if (gAsm->preproc_error(ss[ss_idx--])) gAbort = TRUE;
}

#define ag_rp_24() (gAsm->directive_printf(ss[ss_idx--], FALSE))

#define ag_rp_25() (gAsm->directive_printf(ss[ss_idx--]))

#define ag_rp_26() (gAsm->preproc_undef(ss[ss_idx--]))

static void ag_rp_27(void) {
/* Line 132, C:/Projects/VirtualT/src/a85parse.syn */
 if (gMacroStack[ms_idx] != 0) { delete gMacroStack[ms_idx]; \
															gMacroStack[ms_idx] = 0; } \
														gMacro = gMacroStack[--ms_idx]; \
														gMacro->m_DefString = ss[ss_idx--]; \
														gAsm->preproc_define(); \
														gMacroStack[ms_idx] = 0; gDefine = 0; 
}

static void ag_rp_28(void) {
/* Line 146, C:/Projects/VirtualT/src/a85parse.syn */
 gMacro->m_ParamList = gExpList; \
															gMacro->m_Name = ss[ss_idx--]; \
															gExpList = new VTObArray; \
															gMacroStack[ms_idx++] = gMacro; \
															if (gAsm->preproc_macro()) \
																PCB.reduction_token = a85parse_WS_token; \
															gMacro = new CMacro; 
}

static void ag_rp_29(void) {
/* Line 153, C:/Projects/VirtualT/src/a85parse.syn */
 gMacro->m_Name = ss[ss_idx--]; gMacroStack[ms_idx++] = gMacro; \
															if (gAsm->preproc_macro()) \
																PCB.reduction_token = a85parse_WS_token; \
															gMacro = new CMacro; 
}

static void ag_rp_30(int c) {
/* Line 159, C:/Projects/VirtualT/src/a85parse.syn */
 ss[++ss_idx][0] = c; ss[ss_idx][1] = 0; ss_len = 1; 
}

static void ag_rp_31(int c) {
/* Line 160, C:/Projects/VirtualT/src/a85parse.syn */
 ss[ss_idx][ss_len++] = c; ss[ss_idx][ss_len] = 0; 
}

static void ag_rp_32(void) {
/* Line 161, C:/Projects/VirtualT/src/a85parse.syn */
 ss[ss_idx][ss_len++] = '\n'; ss[ss_idx][ss_len] = 0; 
}

static void ag_rp_33(void) {
/* Line 164, C:/Projects/VirtualT/src/a85parse.syn */
 page = 0; seg = CSEG; 
}

static void ag_rp_34(void) {
/* Line 165, C:/Projects/VirtualT/src/a85parse.syn */
 page = 0; seg = DSEG; 
}

#define ag_rp_35(p) (page = p)

#define ag_rp_36() (gAsm->pragma_list())

#define ag_rp_37() (gAsm->pragma_hex())

#define ag_rp_38() (gAsm->pragma_verilog())

#define ag_rp_39() (gAsm->pragma_entry(ss[ss_idx--]))

#define ag_rp_40() (gAsm->pragma_extended())

#define ag_rp_41() (gAsm->directive_org())

#define ag_rp_42() (gAsm->directive_aseg())

#define ag_rp_43() (gAsm->directive_ds())

#define ag_rp_44() (gAsm->directive_db())

#define ag_rp_45() (gAsm->directive_dw())

#define ag_rp_46() (gAsm->directive_public())

#define ag_rp_47() (gAsm->directive_extern())

#define ag_rp_48() (gAsm->directive_extern())

#define ag_rp_49() (gAsm->directive_module(ss[ss_idx--]))

#define ag_rp_50() (gAsm->directive_name(ss[ss_idx--]))

#define ag_rp_51() (gAsm->directive_stkln())

#define ag_rp_52() (gAsm->directive_echo())

#define ag_rp_53() (gAsm->directive_echo(ss[ss_idx--]))

#define ag_rp_54() (gAsm->directive_fill())

#define ag_rp_55() (gAsm->directive_printf(ss[ss_idx--], FALSE))

#define ag_rp_56() (gAsm->directive_printf(ss[ss_idx--]))

#define ag_rp_57() (gAsm->directive_end(""))

#define ag_rp_58() (gAsm->directive_end(ss[ss_idx--]))

#define ag_rp_59() (gAsm->directive_if())

#define ag_rp_60() (gAsm->directive_else())

#define ag_rp_61() (gAsm->directive_endif())

#define ag_rp_62() (gAsm->directive_endian(1))

#define ag_rp_63() (gAsm->directive_endian(0))

#define ag_rp_64() (gAsm->directive_title(ss[ss_idx--]))

#define ag_rp_65() (gAsm->directive_title(ss[ss_idx--]))

#define ag_rp_66() (gAsm->directive_page(-1))

#define ag_rp_67() (gAsm->directive_sym())

#define ag_rp_68() (gAsm->directive_link(ss[ss_idx--]))

#define ag_rp_69() (gAsm->directive_maclib(ss[ss_idx--]))

#define ag_rp_70() (gAsm->directive_page(page))

#define ag_rp_71() (page = 60)

#define ag_rp_72(n) (page = n)

#define ag_rp_73() (expression_list_literal())

#define ag_rp_74() (expression_list_literal())

#define ag_rp_75() (expression_list_equation())

#define ag_rp_76() (expression_list_equation())

#define ag_rp_77() (expression_list_literal())

#define ag_rp_78() (expression_list_literal())

#define ag_rp_79() (gNameList->Add(ss[ss_idx--]))

#define ag_rp_80() (gNameList->Add(ss[ss_idx--]))

static void ag_rp_81(void) {
/* Line 247, C:/Projects/VirtualT/src/a85parse.syn */
 strcpy(ss[++ss_idx], "$"); ss_len = 1; 
}

static void ag_rp_82(void) {
/* Line 248, C:/Projects/VirtualT/src/a85parse.syn */
 strcpy(ss[++ss_idx], "&"); ss_len = 1; 
}

static void ag_rp_83(int c) {
/* Line 251, C:/Projects/VirtualT/src/a85parse.syn */
 ss[++ss_idx][0] = c; ss[ss_idx][1] = 0; ss_len = 1; \
														if (PCB.column == 2) ss_addr = gAsm->m_ActiveSeg->m_Address; 
}

static void ag_rp_84(int c) {
/* Line 253, C:/Projects/VirtualT/src/a85parse.syn */
 ss[ss_idx][ss_len++] = c; ss[ss_idx][ss_len] = 0; 
}

static void ag_rp_85(int c) {
/* Line 254, C:/Projects/VirtualT/src/a85parse.syn */
 ss[ss_idx][ss_len++] = c; ss[ss_idx][ss_len] = 0; 
}

static void ag_rp_86(int c) {
/* Line 257, C:/Projects/VirtualT/src/a85parse.syn */
 ss[++ss_idx][0] = c; ss[ss_idx][1] = 0; ss_len = 1; 
}

static void ag_rp_87(int c) {
/* Line 258, C:/Projects/VirtualT/src/a85parse.syn */
 ss[ss_idx][ss_len++] = c; ss[ss_idx][ss_len] = 0; 
}

static void ag_rp_88(int ch1, int ch2) {
/* Line 264, C:/Projects/VirtualT/src/a85parse.syn */
 ss_idx++; ss_len = 2; sprintf(ss[ss_idx], "%c%c", ch1, ch2); 
}

static void ag_rp_89(int c) {
/* Line 265, C:/Projects/VirtualT/src/a85parse.syn */
 ss[ss_idx][ss_len++] = c; ss[ss_idx][ss_len] = 0; 
}

static void ag_rp_90(void) {
/* Line 272, C:/Projects/VirtualT/src/a85parse.syn */
 ss_idx++; ss_len = 0; 
}

static void ag_rp_91(int c) {
/* Line 273, C:/Projects/VirtualT/src/a85parse.syn */
 ss[ss_idx][ss_len++] = c; ss[ss_idx][ss_len] = 0; 
}

#define ag_rp_92(n) (n)

#define ag_rp_93() ('\\')

#define ag_rp_94() ('\n')

#define ag_rp_95() ('\t')

#define ag_rp_96() ('\r')

#define ag_rp_97() ('\0')

#define ag_rp_98() ('"')

#define ag_rp_99() (0x08)

#define ag_rp_100() (0x0C)

#define ag_rp_101(n1, n2, n3) ((n1-'0') * 64 + (n2-'0') * 8 + n3-'0')

#define ag_rp_102(n1, n2) (chtoh(n1) * 16 + chtoh(n2))

#define ag_rp_103(n1) (chtoh(n1))

static void ag_rp_104(void) {
/* Line 293, C:/Projects/VirtualT/src/a85parse.syn */
 ss[ss_idx][ss_len++] = '>'; ss[ss_idx][ss_len] = 0; 
}

static void ag_rp_105(void) {
/* Line 296, C:/Projects/VirtualT/src/a85parse.syn */
 ss[++ss_idx][0] = '<'; ss[ss_idx][1] = 0; ss_len = 1; 
}

static void ag_rp_106(int c) {
/* Line 297, C:/Projects/VirtualT/src/a85parse.syn */
 ss[ss_idx][ss_len++] = c; ss[ss_idx][ss_len] = 0; 
}

static void ag_rp_107(int c) {
/* Line 298, C:/Projects/VirtualT/src/a85parse.syn */
 ss[ss_idx][ss_len++] = '\\'; ss[ss_idx][ss_len++] = c; ss[ss_idx][ss_len] = 0; 
}

#define ag_rp_108() (gAsm->label(ss[ss_idx--]))

#define ag_rp_109() (gAsm->label(ss[ss_idx--]))

#define ag_rp_110() (gAsm->label(".bss"))

#define ag_rp_111() (gAsm->label(".text"))

#define ag_rp_112() (gAsm->label(".data"))

#define ag_rp_113() (PAGE)

#define ag_rp_114() (INPAGE)

#define ag_rp_115() (condition(-1))

#define ag_rp_116() (condition(COND_NOCMP))

#define ag_rp_117() (condition(COND_EQ))

#define ag_rp_118() (condition(COND_NE))

#define ag_rp_119() (condition(COND_GE))

#define ag_rp_120() (condition(COND_LE))

#define ag_rp_121() (condition(COND_GT))

#define ag_rp_122() (condition(COND_LT))

#define ag_rp_123() (gEq->Add(RPN_BITOR))

#define ag_rp_124() (gEq->Add(RPN_BITOR))

#define ag_rp_125() (gEq->Add(RPN_BITXOR))

#define ag_rp_126() (gEq->Add(RPN_BITXOR))

#define ag_rp_127() (gEq->Add(RPN_BITAND))

#define ag_rp_128() (gEq->Add(RPN_BITAND))

#define ag_rp_129() (gEq->Add(RPN_LEFTSHIFT))

#define ag_rp_130() (gEq->Add(RPN_LEFTSHIFT))

#define ag_rp_131() (gEq->Add(RPN_RIGHTSHIFT))

#define ag_rp_132() (gEq->Add(RPN_RIGHTSHIFT))

#define ag_rp_133() (gEq->Add(RPN_ADD))

#define ag_rp_134() (gEq->Add(RPN_SUBTRACT))

#define ag_rp_135() (gEq->Add(RPN_MULTIPLY))

#define ag_rp_136() (gEq->Add(RPN_DIVIDE))

#define ag_rp_137() (gEq->Add(RPN_MODULUS))

#define ag_rp_138() (gEq->Add(RPN_MODULUS))

#define ag_rp_139() (gEq->Add(RPN_EXPONENT))

#define ag_rp_140() (gEq->Add(RPN_NOT))

#define ag_rp_141() (gEq->Add(RPN_NOT))

#define ag_rp_142() (gEq->Add(RPN_BITNOT))

#define ag_rp_143() (gEq->Add(RPN_NEGATE))

#define ag_rp_144(n) (gEq->Add((double) n))

static void ag_rp_145(void) {
/* Line 389, C:/Projects/VirtualT/src/a85parse.syn */
 delete gMacro; gMacro = gMacroStack[ms_idx-1]; \
														gMacroStack[ms_idx--] = 0; if (gMacro->m_ParamList == 0) \
														{\
															gEq->Add(gMacro->m_Name); gMacro->m_Name = ""; }\
														else { \
															gEq->Add((VTObject *) gMacro); gMacro = new CMacro; \
														} 
}

#define ag_rp_146() (gEq->Add(RPN_FLOOR))

#define ag_rp_147() (gEq->Add(RPN_CEIL))

#define ag_rp_148() (gEq->Add(RPN_LN))

#define ag_rp_149() (gEq->Add(RPN_LOG))

#define ag_rp_150() (gEq->Add(RPN_SQRT))

#define ag_rp_151() (gEq->Add(RPN_IP))

#define ag_rp_152() (gEq->Add(RPN_FP))

#define ag_rp_153() (gEq->Add(RPN_HIGH))

#define ag_rp_154() (gEq->Add(RPN_LOW))

#define ag_rp_155() (gEq->Add(RPN_PAGE))

#define ag_rp_156() (gEq->Add(RPN_DEFINED, ss[ss_idx--]))

#define ag_rp_157(n) (n)

#define ag_rp_158(r) (r)

#define ag_rp_159(n) (n)

#define ag_rp_160() (conv_to_dec())

#define ag_rp_161() (conv_to_hex())

#define ag_rp_162() (conv_to_bin())

#define ag_rp_163() (conv_to_oct())

#define ag_rp_164() (conv_to_hex())

#define ag_rp_165() (conv_to_hex())

#define ag_rp_166() (conv_to_bin())

#define ag_rp_167() (conv_to_oct())

#define ag_rp_168() (conv_to_dec())

static void ag_rp_169(int n) {
/* Line 436, C:/Projects/VirtualT/src/a85parse.syn */
 int_len = 1; integer[0] = n; integer[1] = 0; 
}

static void ag_rp_170(int n) {
/* Line 437, C:/Projects/VirtualT/src/a85parse.syn */
 int_len = 2; integer[0] = '-', integer[1] = n; integer[2] = 0; 
}

static void ag_rp_171(int n) {
/* Line 438, C:/Projects/VirtualT/src/a85parse.syn */
 int_len = 1; integer[0] = n; integer[1] = 0; 
}

static void ag_rp_172(int n) {
/* Line 439, C:/Projects/VirtualT/src/a85parse.syn */
 integer[int_len++] = n; integer[int_len] = 0; 
}

static void ag_rp_173(int n) {
/* Line 444, C:/Projects/VirtualT/src/a85parse.syn */
 int_len = 1; integer[0] = n; integer[1] = 0; 
}

static void ag_rp_174(int n) {
/* Line 445, C:/Projects/VirtualT/src/a85parse.syn */
 int_len = 1; integer[0] = n; integer[1] = 0; 
}

static void ag_rp_175(int n) {
/* Line 446, C:/Projects/VirtualT/src/a85parse.syn */
 integer[int_len++] = n; integer[int_len] = 0; 
}

static void ag_rp_176(int n) {
/* Line 449, C:/Projects/VirtualT/src/a85parse.syn */
 integer[int_len++] = n; integer[int_len] = 0; 
}

static void ag_rp_177(int n) {
/* Line 450, C:/Projects/VirtualT/src/a85parse.syn */
 integer[int_len++] = n; integer[int_len] = 0; 
}

static void ag_rp_178(int n) {
/* Line 453, C:/Projects/VirtualT/src/a85parse.syn */
 integer[int_len++] = n; integer[int_len] = 0; 
}

static void ag_rp_179(int n) {
/* Line 454, C:/Projects/VirtualT/src/a85parse.syn */
 integer[int_len++] = n; integer[int_len] = 0; 
}

static void ag_rp_180(int n) {
/* Line 457, C:/Projects/VirtualT/src/a85parse.syn */
 integer[int_len++] = n; integer[int_len] = 0; 
}

static void ag_rp_181(int n) {
/* Line 458, C:/Projects/VirtualT/src/a85parse.syn */
 integer[int_len++] = n; integer[int_len] = 0; 
}

#define ag_rp_182(n) (n)

#define ag_rp_183(n1, n2) ((n1 << 8) | n2)

#define ag_rp_184() ('\\')

#define ag_rp_185(n) (n)

#define ag_rp_186() ('\\')

#define ag_rp_187() ('\n')

#define ag_rp_188() ('\t')

#define ag_rp_189() ('\r')

#define ag_rp_190() ('\0')

#define ag_rp_191() ('\'')

#define ag_rp_192() ('\'')

static double ag_rp_193(void) {
/* Line 479, C:/Projects/VirtualT/src/a85parse.syn */
 gDivisor = 1.0; return (double) conv_to_dec(); 
}

static double ag_rp_194(int d) {
/* Line 480, C:/Projects/VirtualT/src/a85parse.syn */
 gDivisor = 10.0; return ((double) (d - '0') / gDivisor); 
}

static double ag_rp_195(double r, int d) {
/* Line 481, C:/Projects/VirtualT/src/a85parse.syn */
 gDivisor *= 10.0; return (r + (double) (d - '0') / gDivisor); 
}

#define ag_rp_196() (reg[reg_cnt++] = '0')

#define ag_rp_197() (reg[reg_cnt++] = '1')

#define ag_rp_198() (reg[reg_cnt++] = '2')

#define ag_rp_199() (reg[reg_cnt++] = '3')

#define ag_rp_200() (reg[reg_cnt++] = '4')

#define ag_rp_201() (reg[reg_cnt++] = '5')

#define ag_rp_202() (reg[reg_cnt++] = '6')

#define ag_rp_203() (reg[reg_cnt++] = '7')

#define ag_rp_204() (reg[reg_cnt++] = '0')

#define ag_rp_205() (reg[reg_cnt++] = '1')

#define ag_rp_206() (reg[reg_cnt++] = '2')

#define ag_rp_207() (reg[reg_cnt++] = '3')

#define ag_rp_208() (reg[reg_cnt++] = '0')

#define ag_rp_209() (reg[reg_cnt++] = '1')

#define ag_rp_210() (reg[reg_cnt++] = '2')

#define ag_rp_211() (reg[reg_cnt++] = '3')

#define ag_rp_212() (reg[reg_cnt++] = '3')

#define ag_rp_213() (reg[reg_cnt++] = '0')

#define ag_rp_214() (reg[reg_cnt++] = '1')

#define ag_rp_215() (reg[reg_cnt++] = '0')

#define ag_rp_216() (reg[reg_cnt++] = '1')

#define ag_rp_217() (reg[reg_cnt++] = '2')

#define ag_rp_218() (reg[reg_cnt++] = '3')

#define ag_rp_219() (reg[reg_cnt++] = '4')

#define ag_rp_220() (reg[reg_cnt++] = '5')

#define ag_rp_221() (gAsm->opcode_arg_0		(OPCODE_ASHR))

#define ag_rp_222() (gAsm->opcode_arg_0		(OPCODE_CMA))

#define ag_rp_223() (gAsm->opcode_arg_0		(OPCODE_CMC))

#define ag_rp_224() (gAsm->opcode_arg_0		(OPCODE_DAA))

#define ag_rp_225() (gAsm->opcode_arg_0		(OPCODE_DI))

#define ag_rp_226() (gAsm->opcode_arg_0		(OPCODE_DSUB))

#define ag_rp_227() (gAsm->opcode_arg_0		(OPCODE_EI))

#define ag_rp_228() (gAsm->opcode_arg_0		(OPCODE_HLT))

#define ag_rp_229() (gAsm->opcode_arg_0		(OPCODE_LHLX))

#define ag_rp_230() (gAsm->opcode_arg_0		(OPCODE_NOP))

#define ag_rp_231() (gAsm->opcode_arg_0		(OPCODE_PCHL))

#define ag_rp_232() (gAsm->opcode_arg_0		(OPCODE_RAL))

#define ag_rp_233() (gAsm->opcode_arg_0		(OPCODE_RAR))

#define ag_rp_234() (gAsm->opcode_arg_0		(OPCODE_RC))

#define ag_rp_235() (gAsm->opcode_arg_0		(OPCODE_RET))

#define ag_rp_236() (gAsm->opcode_arg_0		(OPCODE_RIM))

#define ag_rp_237() (gAsm->opcode_arg_0		(OPCODE_RLC))

#define ag_rp_238() (gAsm->opcode_arg_0		(OPCODE_RLDE))

#define ag_rp_239() (gAsm->opcode_arg_0		(OPCODE_RM))

#define ag_rp_240() (gAsm->opcode_arg_0		(OPCODE_RNC))

#define ag_rp_241() (gAsm->opcode_arg_0		(OPCODE_RNZ))

#define ag_rp_242() (gAsm->opcode_arg_0		(OPCODE_RP))

#define ag_rp_243() (gAsm->opcode_arg_0		(OPCODE_RPE))

#define ag_rp_244() (gAsm->opcode_arg_0		(OPCODE_RPO))

#define ag_rp_245() (gAsm->opcode_arg_0		(OPCODE_RRC))

#define ag_rp_246() (gAsm->opcode_arg_0		(OPCODE_RSTV))

#define ag_rp_247() (gAsm->opcode_arg_0		(OPCODE_RZ))

#define ag_rp_248() (gAsm->opcode_arg_0		(OPCODE_SHLX))

#define ag_rp_249() (gAsm->opcode_arg_0		(OPCODE_SIM))

#define ag_rp_250() (gAsm->opcode_arg_0		(OPCODE_SPHL))

#define ag_rp_251() (gAsm->opcode_arg_0		(OPCODE_STC))

#define ag_rp_252() (gAsm->opcode_arg_0		(OPCODE_XCHG))

#define ag_rp_253() (gAsm->opcode_arg_0		(OPCODE_XTHL))

#define ag_rp_254() (gAsm->opcode_arg_0		(OPCODE_LRET))

#define ag_rp_255() (gAsm->opcode_arg_1reg		(OPCODE_STAX))

#define ag_rp_256() (gAsm->opcode_arg_1reg		(OPCODE_LDAX))

#define ag_rp_257() (gAsm->opcode_arg_1reg		(OPCODE_POP))

#define ag_rp_258() (gAsm->opcode_arg_1reg		(OPCODE_PUSH))

#define ag_rp_259() (gAsm->opcode_arg_1reg_2byte	(OPCODE_LPOP))

#define ag_rp_260() (gAsm->opcode_arg_1reg_2byte	(OPCODE_LPUSH))

#define ag_rp_261() (gAsm->opcode_arg_1reg		(OPCODE_ADC))

#define ag_rp_262() (gAsm->opcode_arg_1reg		(OPCODE_ADD))

#define ag_rp_263() (gAsm->opcode_arg_1reg		(OPCODE_ANA))

#define ag_rp_264() (gAsm->opcode_arg_1reg		(OPCODE_CMP))

#define ag_rp_265() (gAsm->opcode_arg_1reg		(OPCODE_DCR))

#define ag_rp_266() (gAsm->opcode_arg_1reg		(OPCODE_INR))

#define ag_rp_267() (gAsm->opcode_arg_2reg		(OPCODE_MOV))

#define ag_rp_268() (gAsm->opcode_arg_1reg		(OPCODE_ORA))

#define ag_rp_269() (gAsm->opcode_arg_1reg		(OPCODE_SBB))

#define ag_rp_270() (gAsm->opcode_arg_1reg		(OPCODE_SUB))

#define ag_rp_271() (gAsm->opcode_arg_1reg		(OPCODE_XRA))

#define ag_rp_272() (gAsm->opcode_arg_1reg_2byte	(OPCODE_SPG))

#define ag_rp_273() (gAsm->opcode_arg_1reg_2byte	(OPCODE_RPG))

#define ag_rp_274() (gAsm->opcode_arg_1reg		(OPCODE_DAD))

#define ag_rp_275() (gAsm->opcode_arg_1reg		(OPCODE_DCX))

#define ag_rp_276() (gAsm->opcode_arg_1reg		(OPCODE_INX))

#define ag_rp_277() (gAsm->opcode_arg_1reg_equ16(OPCODE_LXI))

#define ag_rp_278() (gAsm->opcode_arg_1reg_equ8(OPCODE_MVI))

#define ag_rp_279() (gAsm->opcode_arg_1reg_equ16(OPCODE_SPI))

#define ag_rp_280(c) (gAsm->opcode_arg_imm		(OPCODE_RST, c))

#define ag_rp_281() (gAsm->opcode_arg_equ8		(OPCODE_ACI))

#define ag_rp_282() (gAsm->opcode_arg_equ8		(OPCODE_ADI))

#define ag_rp_283() (gAsm->opcode_arg_equ8		(OPCODE_ANI))

#define ag_rp_284() (gAsm->opcode_arg_equ16	(OPCODE_CALL))

#define ag_rp_285() (gAsm->opcode_arg_equ16	(OPCODE_CC))

#define ag_rp_286() (gAsm->opcode_arg_equ16	(OPCODE_CM))

#define ag_rp_287() (gAsm->opcode_arg_equ16	(OPCODE_CNC))

#define ag_rp_288() (gAsm->opcode_arg_equ16	(OPCODE_CNZ))

#define ag_rp_289() (gAsm->opcode_arg_equ16	(OPCODE_CP))

#define ag_rp_290() (gAsm->opcode_arg_equ16	(OPCODE_CPE))

#define ag_rp_291() (gAsm->opcode_arg_equ8		(OPCODE_CPI))

#define ag_rp_292() (gAsm->opcode_arg_equ16	(OPCODE_CPO))

#define ag_rp_293() (gAsm->opcode_arg_equ16	(OPCODE_CZ))

#define ag_rp_294() (gAsm->opcode_arg_equ8		(OPCODE_IN))

#define ag_rp_295() (gAsm->opcode_arg_equ16	(OPCODE_JC))

#define ag_rp_296() (gAsm->opcode_arg_equ16	(OPCODE_JD))

#define ag_rp_297() (gAsm->opcode_arg_equ16	(OPCODE_JM))

#define ag_rp_298() (gAsm->opcode_arg_equ16	(OPCODE_JMP))

#define ag_rp_299() (gAsm->opcode_arg_equ16	(OPCODE_JNC))

#define ag_rp_300() (gAsm->opcode_arg_equ16	(OPCODE_JND))

#define ag_rp_301() (gAsm->opcode_arg_equ16	(OPCODE_JNZ))

#define ag_rp_302() (gAsm->opcode_arg_equ16	(OPCODE_JP))

#define ag_rp_303() (gAsm->opcode_arg_equ16	(OPCODE_JPE))

#define ag_rp_304() (gAsm->opcode_arg_equ16	(OPCODE_JPO))

#define ag_rp_305() (gAsm->opcode_arg_equ16	(OPCODE_JZ))

#define ag_rp_306() (gAsm->opcode_arg_equ16	(OPCODE_LDA))

#define ag_rp_307() (gAsm->opcode_arg_equ8		(OPCODE_LDEH))

#define ag_rp_308() (gAsm->opcode_arg_equ8		(OPCODE_LDES))

#define ag_rp_309() (gAsm->opcode_arg_equ16	(OPCODE_LHLD))

#define ag_rp_310() (gAsm->opcode_arg_equ8		(OPCODE_ORI))

#define ag_rp_311() (gAsm->opcode_arg_equ8		(OPCODE_OUT))

#define ag_rp_312() (gAsm->opcode_arg_equ8		(OPCODE_SBI))

#define ag_rp_313() (gAsm->opcode_arg_equ16	(OPCODE_SHLD))

#define ag_rp_314() (gAsm->opcode_arg_equ16	(OPCODE_STA))

#define ag_rp_315() (gAsm->opcode_arg_equ8		(OPCODE_SUI))

#define ag_rp_316() (gAsm->opcode_arg_equ8		(OPCODE_XRI))

#define ag_rp_317() (gAsm->opcode_arg_equ8		(OPCODE_BR))

#define ag_rp_318() (gAsm->opcode_arg_equ16	(OPCODE_BRA))

#define ag_rp_319() (gAsm->opcode_arg_equ16	(OPCODE_BZ))

#define ag_rp_320() (gAsm->opcode_arg_equ16	(OPCODE_BNZ))

#define ag_rp_321() (gAsm->opcode_arg_equ16	(OPCODE_BC))

#define ag_rp_322() (gAsm->opcode_arg_equ16	(OPCODE_BNC))

#define ag_rp_323() (gAsm->opcode_arg_equ16	(OPCODE_BM))

#define ag_rp_324() (gAsm->opcode_arg_equ16	(OPCODE_BP))

#define ag_rp_325() (gAsm->opcode_arg_equ16	(OPCODE_BPE))

#define ag_rp_326() (gAsm->opcode_arg_equ16	(OPCODE_RCALL))

#define ag_rp_327() (gAsm->opcode_arg_equ8		(OPCODE_SBZ))

#define ag_rp_328() (gAsm->opcode_arg_equ8		(OPCODE_SBNZ))

#define ag_rp_329() (gAsm->opcode_arg_equ8		(OPCODE_SBC))

#define ag_rp_330() (gAsm->opcode_arg_equ8		(OPCODE_SBNC))

#define ag_rp_331() (gAsm->opcode_arg_equ24	(OPCODE_LCALL))

#define ag_rp_332() (gAsm->opcode_arg_equ24	(OPCODE_LJMP))


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
    0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  1,  2,  3,  0,  0,  4,  5,  6,
    7,  8,  9,  0,  0, 10, 11, 12, 13, 14, 15, 16,  0,  0, 17, 18, 19, 20,
   21, 22, 23, 24, 25, 26, 27, 28, 29, 30, 31, 32, 33, 34,  0, 35,  0, 36,
   37, 38, 39, 40,  0,  0,  0, 41, 42,  0,  0, 43,  0,  0,  0, 44,  0,  0,
   45, 46, 47, 48, 49, 50, 51, 52, 53, 54, 55, 56, 57, 58, 59, 60, 61, 62,
   63, 64, 65, 66, 67, 68, 69,  0, 70, 71, 72, 73, 74, 75, 76, 77, 78, 79,
   80,  0,  0, 81, 82, 83, 84, 85, 86, 87,  0, 88, 89,  0, 90, 91,  0, 92,
   93, 94, 95, 96, 97, 98, 99,100,101,  0,  0,102,103,104,105,106,107,  0,
  108,109,110,111,112,113,114,115,116,  0,  0,  0,117,  0,  0,118,  0,  0,
  119,  0,  0,120,  0,  0,121,  0,  0,122,  0,  0,123,124,  0,125,126,  0,
  127,128,  0,129,130,131,132,  0,133,134,  0,135,136,137,138,139,  0,140,
  141,142,143,144,145,  0,  0,146,147,148,149,150,151,152,153,154,155,156,
  157,158,159,160,161,162,163,164,165,166,  0,  0,167,168,169,170,171,172,
    0,  0,173,174,175,176,177,178,179,180,181,182,183,184,185,186,187,188,
  189,190,191,192,  0,193,194,195,196,197,198,199,200,201,  0,  0,202,203,
    0,  0,204,  0,  0,205,  0,  0,206,207,208,209,210,211,212,213,214,215,
  216,217,218,219,220,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,
    0,  0,  0,221,222,223,224,225,  0,  0,226,227,228,  0,  0,  0,229,230,
  231,232,233,234,235,236,237,  0,  0,238,239,240,241,242,243,244,245,246,
  247,  0,  0,  0,248,249,250,251,252,253,254,255,256,257,258,259,260,261,
  262,263,264,265,266,267,268,269,270,271,272,273,274,275,276,  0,277,  0,
  278,  0,279,280,281,282,283,284,285,286,287,288,289,290,291,292,293,294,
  295,  0,  0,  0,  0,  0,296,297,298,299,  0,  0,  0,  0,  0,300,301,302,
  303,304,305,306,  0,  0,  0,307,  0,  0,  0,308,309,310,311,312,313,314,
  315,316,317,318,319,320,321,322,323,324,325,326,327,328,329,330,331,332
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
  1,452,  1,453,  1,455,  1,456,  1,457,  1,461,  1,462,  1,463,
  1,466,  1,467,  1,468,  1,469,  1,470,  1,471,  1,472,  1,473,
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
  255, 35, 36, 38, 46, 47,255, 73, 83,255, 76, 78, 82,255, 68, 78,255, 70,
   78,255, 70,255, 84,255, 78,255, 65, 73,255, 82,255, 68, 69, 73, 80, 85,
  255, 42, 47,255, 38, 42, 47, 58, 61,255, 61,255, 42, 47,255, 67, 68, 73,
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
  255, 82, 85,255, 70,255, 84,255, 78,255, 73,255, 66, 83,255, 65, 67, 79,
   82, 85,255, 76, 82,255, 65,255, 67, 68,255, 67, 90,255, 69, 71, 79,255,
   67, 72,255, 86,255, 84,255, 65, 67, 68, 69, 73, 76, 77, 78, 80, 82, 83,
   90,255, 67, 90,255, 66, 67, 73, 78, 90,255, 69,255, 68, 73, 82, 88,255,
   76,255, 71, 72, 73,255, 88,255, 65, 67, 75,255, 66, 73,255, 66, 69, 72,
   73, 80, 84, 85, 89,255, 69, 73,255, 65, 73,255, 67, 82, 84,255, 36, 38,
   42, 47, 61, 65, 66, 67, 68, 69, 70, 72, 73, 74, 76, 77, 78, 79, 80, 82,
   83, 84, 86, 87, 88,255, 42, 47,255, 85,255, 76,255, 67,255, 78,255, 47,
   73,255, 61,255, 42, 47,255, 67, 68, 73,255, 65, 73,255, 69, 72,255, 67,
   68, 78, 82, 83,255, 67, 90,255, 69,255, 65,255, 67, 76, 77, 78, 80, 82,
   89, 90,255, 65, 67, 80,255, 67, 90,255, 69, 73, 79,255, 65, 67, 77, 78,
   80, 83, 90,255, 65, 68,255, 82, 88,255, 72, 83,255, 69, 85,255, 65, 66,
   67, 69, 73, 83, 87,255, 73,255, 68, 84,255, 78, 82,255, 69, 82,255, 84,
  255, 67, 73, 76, 78, 88,255, 77, 84,255, 69, 76,255, 85,255, 76,255, 67,
   82, 88,255, 70, 78,255, 80,255, 53,255, 67, 68, 75, 88, 90,255, 69, 79,
  255, 77, 80,255, 53,255, 67, 68, 75, 77, 78, 80, 84, 88, 90,255, 88,255,
   72, 83,255, 65, 69, 72, 83,255, 69,255, 68, 73, 88,255, 76,255, 78, 83,
  255, 79, 85,255, 67, 68, 72, 73, 74, 80, 82, 83, 88,255, 68, 86,255, 65,
   79, 83, 86,255, 65,255, 80,255, 65, 79,255, 65, 71, 73,255, 82, 85,255,
   70,255, 84,255, 78,255, 73,255, 66, 83,255, 65, 67, 79, 82, 85,255, 76,
   82,255, 65,255, 67, 68,255, 67, 90,255, 69, 71, 79,255, 67, 72,255, 86,
  255, 84,255, 65, 67, 68, 69, 73, 76, 77, 78, 80, 82, 83, 90,255, 67, 90,
  255, 66, 67, 73, 78, 90,255, 69,255, 68, 73, 82, 88,255, 76,255, 71, 72,
   73,255, 88,255, 65, 67, 75,255, 66, 73,255, 66, 72, 73, 80, 84, 85, 89,
  255, 69, 73,255, 65, 73,255, 67, 82, 84,255, 36, 38, 42, 47, 65, 66, 67,
   68, 69, 70, 72, 73, 74, 76, 77, 78, 79, 80, 82, 83, 84, 86, 87, 88,255,
   36, 38,255, 42, 47,255, 47,255, 76, 80,255, 71, 87,255, 78, 79,255, 36,
   38, 39, 67, 68, 70, 72, 73, 76, 78, 80, 83,255, 78, 88,255, 69, 72, 76,
   86,255, 66, 68, 84,255, 42, 47,255, 85,255, 76,255, 67,255, 78,255, 35,
   36, 38, 42, 46, 47, 73,255, 47, 61,255, 42, 47,255, 76, 89,255, 69,255,
   66, 83, 87,255, 68, 84,255, 78, 82,255, 69, 82,255, 84,255, 67, 78, 81,
   88,255, 85,255, 76,255, 67,255, 78,255, 78, 83,255, 73, 83,255, 65, 79,
   83,255, 65, 79,255, 70,255, 84,255, 78,255, 73,255, 65, 82, 85,255, 69,
   84, 89,255, 69, 73,255, 36, 42, 47, 65, 66, 67, 68, 69, 70, 72, 73, 76,
   77, 78, 79, 80, 83, 84, 86, 87, 92,255, 85,255, 76,255, 67,255, 78,255,
   73,255, 42, 47,255, 42, 47,255, 42, 47, 92,255, 42, 47,255, 47, 73, 80,
  255, 42, 47,255, 33, 47,255, 67,255, 69, 88,255, 76,255, 66, 68, 72, 80,
   83,255, 40, 65, 66, 67, 68, 69, 72, 76, 77,255, 67,255, 69,255, 76,255,
   66, 68, 72, 83,255, 67,255, 69,255, 76,255, 65, 66, 68, 72, 80,255, 67,
  255, 69,255, 66, 68,255, 61,255, 42, 47,255, 60, 61,255, 61,255, 61, 62,
  255, 33, 42, 44, 47, 60, 61, 62,255, 61,255, 42, 47,255, 60, 61,255, 61,
  255, 61, 62,255, 69, 84,255, 69, 84,255, 76, 82,255, 72,255, 33, 42, 44,
   47, 60, 61, 62, 65, 69, 71, 76, 77, 78, 79, 83, 88,255, 76, 89,255, 69,
  255, 66, 83, 87,255, 68, 84,255, 78, 82,255, 69, 82,255, 84,255, 67, 78,
   88,255, 78, 83,255, 73, 83,255, 65, 79, 83,255, 65, 79,255, 70,255, 84,
  255, 78,255, 73,255, 65, 82, 85,255, 84, 89,255, 69, 73,255, 36, 42, 65,
   66, 67, 68, 69, 70, 72, 76, 77, 78, 79, 80, 83, 84, 86, 87,255, 42, 47,
  255, 44, 47,255, 61,255, 42, 47,255, 60, 61,255, 61,255, 61, 62,255, 69,
   84,255, 69, 84,255, 82,255, 76, 82,255, 72,255, 33, 42, 44, 47, 60, 61,
   62, 65, 69, 71, 76, 77, 78, 79, 81, 83, 88,255, 39,255, 61,255, 42, 47,
  255, 60, 61,255, 61,255, 61, 62,255, 69, 84,255, 69, 84,255, 82,255, 76,
   82,255, 72,255, 33, 42, 44, 47, 60, 61, 62, 71, 76, 77, 78, 79, 81, 83,
   88,255, 42, 47,255, 76, 80,255, 71, 87,255, 78, 79,255, 36, 38, 39, 42,
   47, 67, 68, 70, 72, 73, 76, 78, 80, 83, 92,255, 76, 80,255, 71, 87,255,
   78, 79,255, 36, 38, 39, 67, 68, 70, 72, 73, 76, 80, 83,255, 42, 47,255,
   76, 80,255, 71, 87,255, 78, 79,255, 36, 38, 39, 42, 47, 67, 68, 70, 72,
   73, 76, 80, 83, 92,255, 61,255, 60, 61,255, 61,255, 61, 62,255, 69, 84,
  255, 69, 84,255, 76, 82,255, 72,255, 33, 42, 44, 60, 61, 62, 65, 69, 71,
   76, 77, 78, 79, 83, 88,255, 61,255, 42, 47,255, 60, 61,255, 61,255, 61,
   62,255, 69, 84,255, 69, 84,255, 76, 82,255, 72,255, 33, 44, 47, 60, 61,
   62, 65, 69, 71, 76, 78, 79, 83, 88,255, 61,255, 42, 47,255, 61,255, 61,
  255, 61,255, 69, 84,255, 69, 84,255, 33, 44, 47, 60, 61, 62, 65, 69, 71,
   76, 78, 79, 88,255, 61,255, 42, 47,255, 61,255, 61,255, 61,255, 69, 84,
  255, 69, 84,255, 33, 44, 47, 60, 61, 62, 69, 71, 76, 78, 79, 88,255, 61,
  255, 42, 47,255, 61,255, 61,255, 61,255, 69, 84,255, 69, 84,255, 33, 44,
   47, 60, 61, 62, 69, 71, 76, 78, 79,255, 42, 47,255, 61,255, 61,255, 61,
  255, 69, 84,255, 69, 84,255, 33, 47, 60, 61, 62, 69, 71, 76, 78,255, 61,
  255, 42, 47,255, 42, 47,255, 60, 61,255, 61,255, 61, 62,255, 69, 84,255,
   69, 84,255, 76, 82,255, 72,255, 33, 42, 44, 47, 60, 61, 62, 65, 69, 71,
   76, 77, 78, 79, 83, 88, 92,255, 76, 89,255, 69,255, 66, 83, 87,255, 68,
   84,255, 78, 82,255, 69, 82,255, 84,255, 67, 78, 81, 88,255, 78, 83,255,
   73, 83,255, 65, 79, 83,255, 65, 79,255, 70,255, 84,255, 78,255, 73,255,
   65, 82, 85,255, 69, 84, 89,255, 69, 73,255, 36, 42, 65, 66, 67, 68, 69,
   70, 72, 76, 77, 78, 79, 80, 83, 84, 86, 87,255, 39,255, 67, 68, 73,255,
   65, 73,255, 67, 68, 78, 82, 83,255, 67, 90,255, 69,255, 65,255, 67, 77,
   78, 80, 82, 90,255, 65, 67, 80,255, 67, 90,255, 69, 73, 79,255, 65, 67,
   77, 78, 80, 90,255, 65, 68,255, 82, 88,255, 72, 83,255, 65, 67, 69, 73,
   83,255, 77, 84,255, 76,255, 82, 88,255, 78,255, 80,255, 53,255, 67, 68,
   75, 88, 90,255, 69, 79,255, 77, 80,255, 53,255, 67, 68, 75, 77, 78, 80,
   84, 88, 90,255, 88,255, 72, 83,255, 65, 69, 72, 83,255, 69,255, 68, 73,
   88,255, 76,255, 79, 85,255, 67, 68, 72, 74, 80, 82, 88,255, 79, 86,255,
   65, 73,255, 82, 85,255, 67, 79, 85,255, 76, 82,255, 65,255, 67, 68,255,
   67, 90,255, 69, 71, 79,255, 67, 72,255, 86,255, 84,255, 65, 67, 68, 69,
   73, 76, 77, 78, 80, 82, 83, 90,255, 67, 90,255, 66, 67, 73, 78, 90,255,
   69,255, 68, 73, 82, 88,255, 76,255, 71, 72, 73,255, 88,255, 65, 67,255,
   66, 73,255, 66, 72, 73, 80, 84, 85,255, 65, 73,255, 67, 82, 84,255, 65,
   66, 67, 68, 69, 72, 73, 74, 76, 77, 78, 79, 80, 82, 83, 88,255, 42, 47,
  255, 36, 38, 47,255, 42, 47,255, 33, 44, 47,255, 44,255, 42, 47,255, 47,
   79, 81,255, 92,255, 69,255, 69,255, 76, 80,255, 73,255, 71, 87,255, 78,
   79,255, 36, 38, 39, 40, 65, 66, 67, 68, 69, 70, 72, 73, 76, 77, 78, 80,
   83,255, 33,255, 42, 47,255, 47, 92,255
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
  7,7,7,2,3,4,3,3,3,4,0,0,4,0,5,0,2,2,4,7,7,4,2,7,7,4,7,7,4,6,7,4,5,4,6,
  4,2,4,7,2,4,2,4,7,2,2,2,7,4,0,0,4,0,3,2,0,0,4,0,4,0,0,4,5,5,5,4,5,5,4,
  7,7,4,7,2,2,7,2,4,5,5,4,5,4,5,4,5,7,5,2,6,6,7,5,4,5,5,5,4,5,5,4,5,5,5,
  4,7,5,6,2,6,7,5,4,5,5,4,5,5,4,7,7,4,7,7,4,2,5,2,2,5,6,5,4,7,4,6,7,4,7,
  7,4,2,7,4,2,4,7,5,7,2,7,2,4,7,5,4,7,2,4,5,5,4,5,6,4,5,4,5,4,5,5,5,6,5,
  4,5,5,4,5,5,4,5,4,5,5,5,6,2,6,2,6,5,4,5,4,5,5,4,6,2,7,7,4,5,4,6,5,5,4,
  2,4,7,7,4,7,7,4,7,2,2,2,7,2,7,7,7,4,7,5,4,7,2,7,7,4,7,4,6,4,7,2,4,5,5,
  5,4,2,7,4,5,4,6,4,2,4,2,4,7,7,4,7,7,7,2,2,4,5,5,4,7,4,5,7,4,5,5,4,5,5,
  5,4,5,7,4,5,4,6,4,2,6,7,7,7,2,5,2,6,2,2,5,4,5,5,4,5,5,5,2,5,4,5,4,6,5,
  5,5,4,2,4,5,7,5,4,5,4,6,5,7,4,5,5,4,2,7,2,7,2,2,2,7,4,7,7,4,5,5,4,7,2,
  7,4,6,0,3,2,0,2,2,2,2,2,7,2,2,2,2,2,2,2,2,2,2,2,7,7,2,4,0,0,4,7,4,6,4,
  2,4,2,4,2,2,4,0,4,0,0,4,5,5,5,4,5,5,4,7,7,4,7,2,2,7,2,4,5,5,4,5,4,5,4,
  5,7,5,2,6,6,7,5,4,5,5,5,4,5,5,4,5,5,5,4,7,5,6,2,6,7,5,4,5,5,4,5,5,4,7,
  7,4,7,7,4,2,5,2,2,5,6,5,4,7,4,6,7,4,7,7,4,2,7,4,2,4,7,5,7,2,2,4,7,5,4,
  7,2,4,7,4,6,4,2,5,5,4,5,6,4,5,4,5,4,5,5,5,6,5,4,5,5,4,5,5,4,5,4,5,5,5,
  6,2,6,2,6,5,4,5,4,5,5,4,6,2,7,7,4,5,4,6,5,5,4,2,4,7,7,4,7,7,4,7,2,2,2,
  7,2,7,7,7,4,7,5,4,7,2,7,7,4,7,4,6,4,7,2,4,5,5,5,4,2,7,4,5,4,6,4,2,4,2,
  4,7,7,4,7,7,7,2,2,4,5,5,4,7,4,5,7,4,5,5,4,5,5,5,4,5,7,4,5,4,6,4,2,6,7,
  7,7,2,5,2,6,2,2,5,4,5,5,4,5,5,5,2,5,4,5,4,6,5,5,5,4,2,4,5,7,5,4,5,4,6,
  5,7,4,5,5,4,2,2,7,2,2,2,7,4,7,7,4,5,5,4,7,2,7,4,6,0,3,2,2,2,2,2,2,7,2,
  2,2,2,2,2,2,2,2,2,2,7,7,2,4,5,0,4,0,0,4,2,4,7,5,4,5,5,4,5,2,4,5,0,3,7,
  7,2,7,7,2,7,7,7,4,7,7,4,2,7,7,7,4,3,3,3,4,0,0,4,7,4,6,4,2,4,2,4,0,5,0,
  3,2,2,2,4,0,0,4,0,0,4,7,7,4,7,4,5,6,5,4,5,7,4,7,7,4,2,7,4,2,4,7,2,7,2,
  4,7,4,6,4,2,4,2,4,7,7,4,2,7,4,7,7,7,4,7,7,4,5,4,6,4,2,4,2,4,7,2,7,4,7,
  7,7,4,7,7,4,3,2,2,7,2,7,2,2,7,7,2,2,2,2,7,2,2,2,7,7,3,4,7,4,6,4,2,4,2,
  4,2,4,3,3,4,0,0,4,3,2,3,4,0,0,4,2,7,7,4,0,0,4,5,2,4,5,4,5,5,4,5,4,6,6,
  6,7,7,4,3,5,5,5,5,5,5,5,5,4,5,4,5,4,5,4,6,6,6,7,4,5,4,5,4,5,4,5,6,6,6,
  7,4,5,4,5,4,6,6,4,0,4,0,0,4,0,0,4,0,4,0,0,4,6,3,0,2,1,1,1,4,0,4,0,0,4,
  0,0,4,0,4,0,0,4,5,5,4,5,5,4,5,5,4,2,4,6,3,0,2,1,1,1,7,7,2,2,7,7,7,2,7,
  4,7,7,4,7,4,5,6,5,4,5,7,4,7,7,4,2,7,4,2,4,7,2,2,4,7,7,4,2,7,4,7,7,7,4,
  7,7,4,5,4,6,4,2,4,2,4,7,2,7,4,7,7,4,7,7,4,3,3,7,2,7,2,2,7,7,2,2,2,7,2,
  2,2,7,7,4,0,0,4,0,2,4,0,4,0,0,4,0,0,4,0,4,0,0,4,5,5,4,5,5,4,5,4,5,5,4,
  2,4,6,3,0,2,1,1,1,7,7,2,2,7,7,6,5,2,7,4,3,4,0,4,0,0,4,0,0,4,0,4,0,0,4,
  5,5,4,5,5,4,5,4,5,5,4,2,4,6,3,0,2,1,1,1,2,2,7,7,6,5,2,7,4,0,0,4,7,5,4,
  5,5,4,5,2,4,5,0,3,3,2,7,7,2,7,7,2,7,7,7,3,4,7,5,4,5,5,4,5,2,4,5,0,3,7,
  7,2,7,7,2,7,7,4,0,0,4,7,5,4,5,5,4,5,2,4,5,0,3,3,2,7,7,2,7,7,2,7,7,3,4,
  0,4,0,0,4,0,4,0,0,4,5,5,4,5,5,4,5,5,4,2,4,6,3,0,1,1,1,7,7,2,2,7,7,7,2,
  7,4,0,4,0,0,4,0,0,4,0,4,0,0,4,5,5,4,5,5,4,5,5,4,2,4,6,0,2,1,1,1,7,7,2,
  2,7,7,2,7,4,0,4,0,0,4,0,4,0,4,0,4,5,5,4,5,5,4,6,0,2,1,1,1,7,7,2,2,7,7,
  7,4,0,4,0,0,4,0,4,0,4,0,4,5,5,4,5,5,4,6,0,2,1,1,1,7,2,2,7,7,7,4,0,4,0,
  0,4,0,4,0,4,0,4,5,5,4,5,5,4,6,0,2,1,1,1,7,2,2,7,7,4,0,0,4,0,4,0,4,0,4,
  5,5,4,5,5,4,3,2,1,1,1,7,2,2,7,4,0,4,0,0,4,0,0,4,0,0,4,0,4,0,0,4,5,5,4,
  5,5,4,5,5,4,2,4,6,2,0,2,1,1,1,7,7,2,2,7,7,7,2,7,3,4,7,7,4,7,4,5,6,5,4,
  5,7,4,7,7,4,2,7,4,2,4,7,2,7,2,4,7,7,4,2,7,4,7,7,7,4,7,7,4,5,4,6,4,2,4,
  2,4,7,2,7,4,7,7,7,4,7,7,4,3,3,7,2,7,2,2,7,7,2,2,2,7,2,2,2,7,7,4,3,4,5,
  5,5,4,5,5,4,7,2,2,7,7,4,5,5,4,5,4,5,4,5,5,2,6,6,5,4,5,5,5,4,5,5,4,5,5,
  5,4,7,5,6,2,6,5,4,5,5,4,5,5,4,7,7,4,2,2,2,5,7,4,7,5,4,2,4,5,5,4,6,4,5,
  4,5,4,5,5,5,6,5,4,5,5,4,5,5,4,5,4,5,5,5,6,2,6,2,6,5,4,5,4,5,5,4,6,2,7,
  7,4,5,4,6,5,5,4,2,4,7,7,4,7,2,2,7,2,7,7,4,7,7,4,5,5,4,2,7,4,7,7,7,4,5,
  5,4,7,4,5,7,4,5,5,4,5,5,5,4,5,7,4,5,4,6,4,2,6,7,7,7,2,5,2,6,2,2,5,4,5,
  5,4,5,5,5,2,5,4,5,4,6,5,5,5,4,2,4,5,7,5,4,5,4,6,5,4,5,5,4,2,2,7,2,2,2,
  4,5,5,4,7,2,7,4,2,2,2,2,7,2,2,2,2,2,7,2,2,2,2,2,4,0,0,4,5,0,2,4,0,0,4,
  5,0,2,4,0,4,0,0,4,2,5,5,4,3,4,7,4,7,4,7,5,4,7,4,5,5,4,5,2,4,5,0,3,3,5,
  5,6,6,5,2,6,7,6,5,7,7,7,4,5,4,0,0,4,2,3,4
};

static const unsigned short ag_key_parm[] = {
    0,162,164,163,  0,427,424,  0,  4,  0,  6,  0,  0,  0,  0,  0,438, 98,
  135,  0,  0,  0,  0,498,  0,464,  0,523,426,465,  0,162,164,163,  0,427,
  424,  0,513,502,  0,496,  0,500,515,  0,282,284,324,  0,286,118,326,  0,
   48,184,  0,322,  0,  0,188,  0,  0,426,424,  0,432,  0,420,  0,168, 52,
  428,  0,430,418, 56,422,  0,192,194,288,  0,334,336,  0,340,342,344,  0,
  328,330,130,332,  0,338, 42,346,  0,196,308,  0,290,310,  0,146,  0, 40,
    0,  0,  0,  0,  0,  0,392,398,  0, 44,200,  0,  0, 54,  0,170,198, 50,
   60,178,  0, 24, 28,  0, 30,  0, 80, 14,  0,  0,  0, 18, 68,  0,  0, 66,
    0,  0,  0, 76,204,  0,  0,102, 32,  0,  0, 78,128,140,  0,106,110,  0,
  202,206,  0, 12,142,172,  0, 20, 26,  0,  4,  0,  6,  0,  0,100,292,312,
    0, 22,348,138,  0,364,  0,370,  0,366,376,374,368,378,  0,382,384,  0,
  356,372,  0,354,  0,350,360,358,362,  0,380,  0,352,386,  0,272,  0,390,
  396,  0,388,  0,394,400,  0,212,  0,402,210,208,  0,  0,  0, 92, 10,  0,
  134,144,  0,278,280,  0,444,  0,108,  0,  0,446,132,  0,  0,268, 84,112,
  314,  0, 70,  0,124,294,  0, 94,  0, 82,316,  0, 88,  0,214,126,  0, 72,
  104,  0,  0,296, 46,404,  0,114,406,  0,216,  0, 36,  0, 34,  0,  0,  0,
    8,  0,  0, 64,276,  0, 96,180,274,  0,176,  0,  0,218,220,  0,434,  0,
  228,230,  0,236,238,  0,242,306,244,  0,246,190,  0,248,  0,320,  0,  0,
  222,232,224,226,  0,234,  0,240,  0,  0,250,  0,442,438,  0,298,440,408,
    0,436,  0,256,  0,410,254,186,252,  0,120,122,  0,304,260,318,  0,270,
    0,412,262, 74,  0,300,414,  0,  0,  2,  0,258,174,136,  0,  0, 90,  0,
   58, 86,  0,302,416,  0,264,116,  0,266,  0,182,438, 98,135,231,241,  0,
  493,  0,  0,506,432,504,166,152,154,156,158,  0,  0,160,  0,  0,162,164,
    0,  0,  0,  0,  0,  0, 38, 16, 62,  0,460,  0,162,164,163,  0,427,424,
    0,438, 98,135,  0,  0,  0, 24, 28,  0,  0, 30, 32,  0, 20, 26,  0, 22,
    4,  0, 36,  0, 34,  0,  0,  0,  8,  0,  0,  0,  0, 40,  0,  0,  0, 38,
    0,427,424,  0,135,465,  0,161,432,  0,464,  0,427,424,  0,282,284,324,
    0,286,326,  0, 48,184,  0,322,  0,  0,188,  0,  0,426,424,  0,432,  0,
  420,  0,168, 52,428,  0,430,418, 56,422,  0,192,194,288,  0,334,336,  0,
  340,342,344,  0,328,330,332,  0,338, 42,346,  0,196,308,  0,290,310,  0,
  392,398,  0, 44,200,  0,  0, 54,  0,  0,198, 50, 60,  0, 30,  0, 80, 14,
    0, 18, 68,  0,  0, 66,  0,  0,  0, 76,204, 28,  0,  0,  0,  0,202,206,
    0, 12,  0,  0,292,312,  0, 22,348,  0,364,  0,370,  0,366,376,374,368,
  378,  0,382,384,  0,356,372,  0,354,  0,350,360,358,362,  0,380,  0,352,
  386,  0,272,  0,390,396,  0,388,  0,394,400,  0,212,  0,402,210,208,  0,
    0,  0, 92, 10,  0,278,280,  0,444,  0,  0,  0,446,  0,268, 84,314,  0,
   70,294,  0, 94,  0, 82,316,  0, 88,  0,214,  0, 72,  0,  0,296, 46,404,
    0,  0,406,  0, 36,  0, 34,  0,  0,  0,  0,  0, 64,276,  0, 96,216,274,
    0,  0,  0,218,220,  0,434,  0,228,230,  0,236,238,  0,242,306,244,  0,
  246,190,  0,248,  0,320,  0,  0,222,232,224,226,  0,234,  0,240,  0,  0,
  250,  0,442,438,  0,298,440,408,  0,436,  0,256,  0,410,254,186,252,  0,
    0,  0,304,260,318,  0,270,  0,412,262, 74,  0,300,414,  0,  0,  2,  0,
  258,  0,  0,  0, 90,  0, 58, 86,  0,302,416,  0,264,  0,266,  0, 98,135,
  465,  0,432,  0,  0,  0,  0,  0, 78,  0,  0,  0,  0,  0,  0,  0,  0,  0,
    0,  0, 16, 62,  0,  0,427,424,  0,  4,  0,  6,  0,  0,  0,  0,  0,  0,
    0,  0,464,  0,427,424,  0,282,284,324,  0,286,326,  0, 48,184,  0,322,
    0,  0,188,  0,  0,426,424,  0,432,  0,420,  0,168, 52,428,  0,430,418,
   56,422,  0,192,194,288,  0,334,336,  0,340,342,344,  0,328,330,332,  0,
  338, 42,346,  0,196,308,  0,290,310,  0,392,398,  0, 44,200,  0,  0, 54,
    0,  0,198, 50, 60,  0, 30,  0, 80, 14,  0, 18, 68,  0,  0, 66,  0,  0,
    0, 76,204, 28,  0,  0,  0,202,206,  0, 12,  0,  0,  4,  0,  6,  0,  0,
  292,312,  0, 22,348,  0,364,  0,370,  0,366,376,374,368,378,  0,382,384,
    0,356,372,  0,354,  0,350,360,358,362,  0,380,  0,352,386,  0,272,  0,
  390,396,  0,388,  0,394,400,  0,212,  0,402,210,208,  0,  0,  0, 92, 10,
    0,278,280,  0,444,  0,  0,  0,446,  0,268, 84,314,  0, 70,294,  0, 94,
    0, 82,316,  0, 88,  0,214,  0, 72,  0,  0,296, 46,404,  0,  0,406,  0,
   36,  0, 34,  0,  0,  0,  0,  0, 64,276,  0, 96,216,274,  0,  0,  0,218,
  220,  0,434,  0,228,230,  0,236,238,  0,242,306,244,  0,246,190,  0,248,
    0,320,  0,  0,222,232,224,226,  0,234,  0,240,  0,  0,250,  0,442,438,
    0,298,440,408,  0,436,  0,256,  0,410,254,186,252,  0,  0,  0,304,260,
  318,  0,270,  0,412,262, 74,  0,300,414,  0,  0,  0,258,  0,  0,  0, 90,
    0, 58, 86,  0,302,416,  0,264,  0,266,  0, 98,135,465,  0,  0,  0,  0,
    0,  0, 78,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0, 16, 62,  0,  0,
   98,135,  0,427,424,  0,  0,  0,128,140,  0,134,144,  0,132,  0,  0, 98,
  135,231,130,146,  0,142,138,  0,126, 96,136,  0, 14, 18,  0,  0, 12, 10,
   16,  0,162,164,163,  0,427,424,  0,  4,  0,  6,  0,  0,  0,  0,  0,438,
   98,135,426,  0,  0,  0,  0,426,465,  0,427,424,  0, 52, 56,  0, 44,  0,
   54, 50, 60,  0, 80, 14,  0, 18, 68,  0,  0, 66,  0,  0,  0, 76,  0,  0,
    0,  0,  4,  0,  6,  0,  0,  0,  0,  0, 92, 10,  0,  0, 84,  0, 94, 70,
   82,  0, 72, 88,  0, 36,  0, 34,  0,  0,  0,  0,  0, 96,  0, 64,  0,  2,
   74, 90,  0, 58, 86,  0,464,  0,  0, 48,  0, 42,  0,  0, 78, 12,  0,  0,
    0,  0, 46,  0,  0,  0, 16, 62,460,  0,  4,  0,  6,  0,  0,  0,  0,  0,
    0,  0,426,427,  0,427,424,  0,426,  0,460,  0,427,424,  0,  0,100, 96,
    0,427,424,  0,182,  0,  0,168,  0,170,178,  0,172,  0,152,156,160,180,
  174,  0,241,166,152,154,156,158,160,162,164,  0,168,  0,170,  0,172,  0,
  152,156,160,174,  0,168,  0,170,  0,172,  0,166,152,156,160,176,  0,168,
    0,170,  0,152,156,  0,498,  0,427,424,  0,513,502,  0,496,  0,500,515,
    0,182,523,493,  0,506,432,504,  0,498,  0,427,424,  0,513,502,  0,496,
    0,500,515,  0,106,110,  0,108,112,  0,120,122,  0,  0,  0,182,523,493,
    0,506,432,504,118,102,  0,  0,124,104,114,  0,116,  0, 52, 56,  0, 44,
    0, 54, 50, 60,  0, 80, 14,  0, 18, 68,  0,  0, 66,  0,  0,  0, 76,  0,
    0,  0, 92, 10,  0,  0, 84,  0, 94, 70, 82,  0, 72, 88,  0, 36,  0, 34,
    0,  0,  0,  0,  0, 96,  0, 64,  0, 74, 90,  0, 58, 86,  0,464,465, 48,
    0, 42,  0,  0, 78, 12,  0,  0,  0, 46,  0,  0,  0, 16, 62,  0,427,424,
    0,493,  0,  0,498,  0,427,424,  0,513,502,  0,496,  0,500,515,  0,106,
  110,  0,108,112,  0,114,  0,120,122,  0,  0,  0,182,523,493,  0,506,432,
  504,118,102,  0,  0,124,104,150,148,  0,116,  0,233,  0,498,  0,427,424,
    0,513,502,  0,496,  0,500,515,  0,106,110,  0,108,112,  0,114,  0,120,
  122,  0,  0,  0,182,523,493,  0,506,432,504,  0,  0,124,104,150,148,  0,
  116,  0,427,424,  0,128,140,  0,134,144,  0,132,  0,  0, 98,135,231,426,
    0,130,146,  0,142,138,  0,126, 96,136,460,  0,128,140,  0,134,144,  0,
  132,  0,  0, 98,135,231,130,146,  0,142,138,  0, 96,136,  0,427,424,  0,
  128,140,  0,134,144,  0,132,  0,  0, 98,135,231,426,  0,130,146,  0,142,
  138,  0, 96,136,460,  0,498,  0,513,502,  0,496,  0,500,515,  0,106,110,
    0,108,112,  0,120,122,  0,  0,  0,182,523,493,506,432,504,118,102,  0,
    0,124,104,114,  0,116,  0,498,  0,427,424,  0,513,502,  0,496,  0,500,
  515,  0,106,110,  0,108,112,  0,120,122,  0,  0,  0,182,493,  0,506,432,
  504,118,102,  0,  0,104,114,  0,116,  0,498,  0,427,424,  0,502,  0,496,
    0,500,  0,106,110,  0,108,112,  0,182,493,  0,506,432,504,118,102,  0,
    0,104,114,116,  0,498,  0,427,424,  0,502,  0,496,  0,500,  0,106,110,
    0,108,112,  0,182,493,  0,506,432,504,102,  0,  0,104,114,116,  0,498,
    0,427,424,  0,502,  0,496,  0,500,  0,106,110,  0,108,112,  0,182,493,
    0,506,432,504,102,  0,  0,104,114,  0,427,424,  0,502,  0,496,  0,500,
    0,106,110,  0,108,112,  0,498,  0,506,432,504,102,  0,  0,104,  0,498,
    0,523,426,  0,427,424,  0,513,502,  0,496,  0,500,515,  0,106,110,  0,
  108,112,  0,120,122,  0,  0,  0,182,  0,493,  0,506,432,504,118,102,  0,
    0,124,104,114,  0,116,460,  0, 52, 56,  0, 44,  0, 54, 50, 60,  0, 80,
   14,  0, 18, 68,  0,  0, 66,  0,  0,  0, 76,  0,  0,  0,  0, 92, 10,  0,
    0, 84,  0, 94, 70, 82,  0, 72, 88,  0, 36,  0, 34,  0,  0,  0,  0,  0,
   96,  0, 64,  0,  2, 74, 90,  0, 58, 86,  0,464,465, 48,  0, 42,  0,  0,
   78, 12,  0,  0,  0, 46,  0,  0,  0, 16, 62,  0,231,  0,282,284,324,  0,
  286,326,  0,322,  0,  0,188,184,  0,426,424,  0,432,  0,420,  0,168,428,
    0,430,418,422,  0,192,194,288,  0,334,336,  0,340,342,344,  0,328,330,
  332,  0,338,346,  0,196,308,  0,290,310,  0,392,398,  0,  0,  0,  0,198,
  200,  0,202,206,  0,  0,  0,292,312,  0,348,  0,364,  0,370,  0,366,376,
  374,368,378,  0,382,384,  0,356,372,  0,354,  0,350,360,358,362,  0,380,
    0,352,386,  0,272,  0,390,396,  0,388,  0,394,400,  0,212,  0,402,210,
  208,  0,  0,  0,278,280,  0,444,  0,  0,446,  0,268,314,  0,294,316,  0,
  296,404,  0,  0,406,  0,216,274,276,  0,218,220,  0,434,  0,228,230,  0,
  236,238,  0,242,306,244,  0,246,190,  0,248,  0,320,  0,  0,222,232,224,
  226,  0,234,  0,240,  0,  0,250,  0,442,438,  0,298,440,408,  0,436,  0,
  256,  0,410,254,186,252,  0,  0,  0,304,260,318,  0,270,  0,412,262,  0,
  300,414,  0,  0,  0,258,  0,  0,  0,  0,302,416,  0,264,  0,266,  0,  0,
    0,  0,  0,204,  0,  0,  0,  0,  0,214,  0,  0,  0,  0,  0,  0,427,424,
    0, 98,135,  0,  0,427,424,  0,182,493,  0,  0,493,  0,427,424,  0,  0,
  150,148,  0,460,  0,130,  0,146,  0,128,140,  0,142,  0,134,144,  0,132,
    0,  0, 98,135,231,241,166,152,154,156,158,  0,160,138,162,164,126, 96,
  136,  0,182,  0,427,424,  0,  0,460,  0
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
    0,  0,  0,  0,426,430,  0,270,272,  0,439,274,278,  0,282,285,  0,446,
  289,  0,  0,  0,452,  0,454,  0,295,456,  0,458,  0,264,442,449,461,299,
    0,  0,  0,  0,  0,304,469,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,
    0,  0,  0,  0,313,315,  0,308,483,487,310,490,  0,  0,  0,  0,  0,  0,
    0,  0,  0,317,  0,499,502,504,321,  0,  0,  0,  0,  0,  0,  0,  0,  0,
    0,  0,  0,  0,324,  0,515,519,522,327,  0,  0,  0,  0,  0,  0,  0,  0,
  330,332,  0,334,336,  0,534,  0,537,540,  0,543,  0,  0,344,  0,554,346,
    0,351,355,  0,559,357,  0,562,  0,338,  0,341,556,349,565,  0,365,  0,
    0,363,574,  0,  0,  0,  0,  0,580,  0,  0,  0,  0,  0,  0,  0,  0,588,
    0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,586,590,596,599,602,
    0,  0,  0,  0,  0,  0,  0,614,616,372,374,  0,  0,  0,624,  0,  0,  0,
  626,  0,376,378,  0,383,385,  0,368,619,630,632,380,635,388,391,397,  0,
  404,  0,  0,399,648,408,414,  0,419,  0,656,  0,416,658,  0,  0,  0,  0,
    0,663,422,  0,  0,  0,670,  0,672,  0,674,  0,432,436,  0,424,427,430,
  676,678,  0,  0,  0,  0,438,  0,  0,448,  0,  0,  0,  0,  0,  0,  0,  0,
    0,450,  0,  0,  0,705,  0,687,690,441,444,446,692,  0,695,698,702,707,
    0,  0,  0,  0,  0,  0,  0,  0,722,  0,  0,  0,  0,731,  0,  0,  0,  0,
  733,  0,  0,456,  0,  0,  0,  0,744,  0,458,  0,  0,  0,  0,725,452,738,
  454,740,746,750,461,  0,463,466,  0,  0,  0,  0,481,765,484,  0,478,  0,
  306,480,  0,493,506,526,546,567,359,577,583,604,638,651,660,667,681,709,
  753,762,470,477,768,  0,  0,  0,  0,487,  0,801,  0,803,  0,805,  0,798,
  807,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,497,499,  0,492,
  817,821,494,824,  0,  0,  0,  0,  0,  0,  0,  0,  0,501,  0,833,836,838,
  505,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,508,  0,849,853,
  856,511,  0,  0,  0,  0,  0,  0,  0,  0,514,516,  0,518,520,  0,868,  0,
  871,874,  0,877,  0,  0,528,  0,888,530,  0,533,537,  0,893,539,  0,896,
    0,522,  0,525,890,899,  0,547,  0,  0,545,907,  0,550,  0,913,  0,915,
    0,  0,  0,  0,917,  0,  0,  0,  0,  0,  0,  0,  0,926,  0,  0,  0,  0,
    0,  0,  0,  0,  0,  0,  0,  0,  0,924,928,934,937,940,  0,  0,  0,  0,
    0,  0,  0,952,954,557,559,  0,  0,  0,962,  0,  0,  0,964,  0,561,563,
    0,568,570,  0,553,957,968,970,565,973,573,576,582,  0,589,  0,  0,584,
  986,593,599,  0,604,  0,994,  0,601,996,  0,  0,  0,  0,  0,1001,607,  0,
    0,  0,1008,  0,1010,  0,1012,  0,617,621,  0,609,612,615,1014,1016,  0,
    0,  0,  0,623,  0,  0,633,  0,  0,  0,  0,  0,  0,  0,  0,  0,635,  0,
    0,  0,1043,  0,1025,1028,626,629,631,1030,  0,1033,1036,1040,1045,  0,
    0,  0,  0,  0,  0,  0,  0,1060,  0,  0,  0,  0,1069,  0,  0,  0,  0,
  1071,  0,  0,639,  0,  0,  0,  0,1082,  0,641,  0,  0,  0,  0,1063,1076,
  637,1078,1084,1088,644,  0,646,649,  0,  0,  0,  0,664,1102,667,  0,812,
    0,490,814,827,840,860,880,901,541,910,921,942,976,989,998,1005,1019,
  1047,1091,1099,653,660,1105,  0,  0,  0,  0,  0,  0,  0,1137,  0,684,  0,
    0,  0,  0,  0,  0,1145,  0,  0,  0,670,673,677,1142,688,692,1148,694,
  697,701,  0,705,709,  0,1164,716,719,723,  0,732,735,739,  0,  0,  0,  0,
  743,  0,1179,  0,1181,  0,1183,  0,  0,  0,  0,730,1172,1176,1185,  0,
    0,  0,  0,  0,  0,  0,752,756,  0,763,  0,  0,1204,  0,  0,  0,768,  0,
  773,777,  0,1213,779,  0,1216,  0,765,1210,771,1219,  0,788,  0,1226,  0,
  1228,  0,1230,  0,791,793,  0,1234,795,  0,801,806,811,  0,817,820,  0,
    0,  0,1247,  0,1249,  0,1251,  0,828,1253,831,  0,836,838,842,  0,844,
  847,  0,746,1195,1198,748,1201,759,1206,1221,781,785,1232,1237,1240,1244,
  825,1255,1259,1263,851,858,862,  0,864,  0,1288,  0,1290,  0,1292,  0,
  1294,  0,867,869,  0,  0,  0,  0,871,1301,873,  0,  0,  0,  0,1308,875,
  881,  0,  0,  0,  0,  0,1315,  0,  0,  0,  0,  0,  0,  0,  0,1321,1323,
  1326,885,887,  0,889,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,
    0,  0,1344,1346,1348,893,  0,  0,  0,  0,  0,  0,  0,  0,1355,1357,1359,
  895,  0,  0,  0,  0,  0,1367,1369,  0,  0,  0,  0,  0,  0,  0,  0,  0,
    0,  0,  0,  0,  0,1374,898,  0,1376,1379,1382,1384,  0,  0,  0,  0,  0,
    0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,
  1414,  0,1395,900,  0,1397,1400,1403,1405,902,905,1408,1411,907,910,912,
  1417,914,  0,925,929,  0,936,  0,  0,1439,  0,  0,  0,941,  0,944,948,
    0,1448,950,  0,1451,  0,938,1445,1454,  0,959,961,  0,1460,963,  0,969,
  974,979,  0,985,988,  0,  0,  0,1473,  0,1475,  0,1477,  0,996,1479,999,
    0,1004,1008,  0,1010,1013,  0,917,919,921,1436,932,1441,1456,952,956,
  1463,1466,1470,993,1481,1485,1488,1017,1024,  0,  0,  0,  0,  0,1510,  0,
    0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,
    0,  0,  0,  0,  0,  0,1537,  0,1516,1028,  0,1518,1521,1524,1526,1030,
  1033,1529,1532,1035,1038,1535,  0,1540,1040,  0,1043,  0,  0,  0,  0,  0,
    0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,
    0,  0,1583,  0,1562,1045,  0,1564,1567,1570,1572,1575,1578,1047,1050,
  1581,  0,1586,1052,  0,  0,  0,  0,1071,  0,  0,  0,  0,  0,  0,1610,  0,
    0,  0,1055,1058,1604,1060,1064,1607,1075,1079,1613,1081,1084,1088,1092,
    0,1108,  0,  0,  0,  0,  0,  0,1635,  0,  0,  0,1094,1097,1101,1632,
  1112,1116,1638,1118,1122,  0,  0,  0,  0,1142,  0,  0,  0,  0,  0,  0,
  1659,  0,  0,  0,1126,1129,1653,1131,1135,1656,1146,1150,1662,1152,1156,
  1160,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,
    0,  0,  0,1696,  0,1680,1162,  0,1682,1685,1687,1164,1167,1690,1693,
  1169,1172,1174,1699,1176,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,
    0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,1736,  0,1717,  0,1719,1722,
  1725,1727,1179,1182,1730,1733,1184,1186,1739,1188,  0,  0,  0,  0,  0,
    0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,1756,  0,1758,1761,
  1763,1765,1191,1194,1767,1770,1196,1198,1200,  0,  0,  0,  0,  0,  0,  0,
    0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,1787,  0,1789,1792,1794,1796,
  1203,1798,1801,1205,1207,1209,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,
    0,  0,  0,  0,  0,  0,  0,  0,1817,  0,1819,1822,1824,1826,1212,1828,
  1831,1214,1216,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,
    0,  0,1218,1846,1849,1851,1853,1220,1855,1858,1222,  0,  0,  0,  0,  0,
    0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,
    0,  0,  0,1893,  0,1871,1873,  0,1876,1879,1882,1884,1224,1227,1887,
  1890,1229,1232,1234,1896,1236,1239,  0,1249,1253,  0,1260,  0,  0,1919,
    0,  0,  0,1265,  0,1270,1274,  0,1928,1276,  0,1931,  0,1262,1925,1268,
  1934,  0,1285,1287,  0,1941,1289,  0,1295,1300,1305,  0,1311,1314,  0,
    0,  0,1954,  0,1956,  0,1958,  0,1322,1960,1325,  0,1330,1332,1336,  0,
  1338,1341,  0,1241,1243,1245,1916,1256,1921,1936,1278,1282,1944,1947,1951,
  1319,1962,1966,1970,1345,1352,  0,1356,  0,  0,  0,  0,  0,  0,  0,  0,
  1359,1994,1998,1361,1364,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,2007,
  2010,2012,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,1367,  0,
  2021,2025,2028,  0,  0,  0,  0,  0,  0,  0,  0,1370,1372,  0,2039,2042,
  2045,  0,1374,  0,1379,  0,  0,2054,  0,  0,  0,  0,2059,  0,  0,  0,  0,
    0,  0,  0,  0,2066,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,
  2064,2068,2074,2077,2080,  0,  0,  0,  0,  0,  0,  0,2092,2094,1386,1388,
    0,  0,  0,2102,  0,  0,  0,2104,  0,1393,1395,  0,1382,2097,2108,1390,
  2110,1398,1401,  0,1403,1405,  0,  0,  0,  0,2124,1410,  0,1412,1415,1417,
    0,  0,  0,  0,1420,  0,  0,1430,  0,  0,  0,  0,  0,  0,  0,  0,  0,
  1432,  0,  0,  0,2152,  0,2134,2137,1423,1426,1428,2139,  0,2142,2145,
  2149,2154,  0,  0,  0,  0,  0,  0,  0,  0,2169,  0,  0,  0,  0,2178,  0,
    0,  0,  0,2180,  0,  0,1436,  0,  0,  0,  0,2191,  0,  0,  0,  0,  0,
  2172,2185,1434,2187,2193,2196,  0,  0,  0,  0,1438,2206,1441,  0,2001,
  2014,2032,2048,1377,2057,2062,2082,2113,2121,1407,2127,2130,2156,2199,
  2209,  0,  0,  0,  0,  0,  0,2230,  0,  0,  0,  0,  0,  0,2237,  0,  0,
    0,  0,  0,  0,2246,  0,  0,  0,1444,  0,1453,  0,1456,  0,1462,  0,  0,
  1466,  0,  0,  0,  0,  0,2264,  0,  0,  0,1446,1449,  0,  0,2255,2257,
    0,2259,2262,1469,2267,  0,1471,1474,1478,  0,  0,  0,  0,  0,  0,2290,
  1482,  0
};

static const unsigned short ag_key_index[] = {
   16,389,433,463,472,463,772,  0,809,772,1109,433,1134,1134,  0,  0,  0,
  1140,1140,1134,1151,1151,1134,1167,  0,1134,1134,  0,  0,  0,1140,1140,
  1134,1151,1151,1134,1167,  0,1187,1266,1296,1298,1304,  0,1298,  0,  0,
  772,1311,1318,1318,1318,1318,1318,1318,1318,1318,1318,1318,1318,1318,1318,
  1318,1328,1334,1350,1151,1151,1151,1151,1151,1151,1151,1151,1151,1151,
  1151,1151,1151,1151,1151,1151,1318,1318,1318,1318,1318,1318,1318,1318,
  1318,1318,1318,1318,1318,1318,1318,1318,1318,1318,1318,1318,1318,1318,
  1318,1318,1318,1318,1318,1318,1318,  0,1328,1334,1350,1151,1151,1151,1151,
  1151,1151,1151,1151,1151,1151,1151,1151,1151,1151,1151,1151,1151,1151,
  1151,1151,1151,1151,1151,1151,1151,1151,1151,1151,1151,1151,1151,1151,
  1151,1151,1151,1151,1151,1151,1151,1151,1151,1151,1151,1151,1151,1151,
  1151,1151,1361,1361,1361,1361,1371,1371,1350,1350,1350,1328,1328,1334,
  1334,1334,1334,1334,1334,1334,1334,1334,1334,1334,  0,  0,  0,  0,1151,
  1151,1151,1151,1151,1151,1151,1151,1151,1151,1151,1151,1151,1151,1151,
  1151,1151,1151,1151,1151,1151,1151,1151,1151,1151,1151,1151,1151,1151,
  1151,1151,1151,1151,1151,1151,1151,1151,1151,1151,1151,1151,1151,1151,
  1151,1151,1151,1151,1151,1151,1151,1151,1151,1361,1361,1361,1361,1371,
  1371,1350,1350,1350,1328,1328,1334,1334,1334,1334,1334,1334,1334,1334,
  1334,1334,1334,1387,1318,1419,1140,1318,1151,1491,1140,  0,1140,  0,1513,
    0,1140,1140,1140,1140,1140,  0,  0,  0,1419,1542,  0,1560,1419,  0,1419,
  1419,1419,1588,  0,1140,  0,  0,  0,  0,  0,  0,  0,  0,  0,1419,1419,
  1616,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,1641,1665,1641,1665,1151,
  1641,1641,1641,1641,1419,1701,1741,1741,1773,1804,1834,1861,1151,1140,
  1140,1140,1140,1140,1140,1140,1134,1134,1140,1140,1140,1140,  0,1140,1140,
  1140,  0,  0,1304,1616,1304,1616,1298,1304,1304,1304,1304,1304,1304,1304,
  1304,1304,1304,1304,1304,1304,1304,1304,1304,1304,1304,1304,1665,1616,
  1616,1616,1665,1898,1304,1616,1665,1304,1304,1304,1187,  0,  0,  0,1151,
  1151,1973,1140,1311,1328,1334,1350,  0,  0,  0,  0,1151,1151,1151,1151,
  1151,1151,1151,1151,1151,1151,1151,1151,1151,1151,1151,1151,1151,1151,
  1151,1151,1151,1151,1151,1151,1151,1151,1151,1151,1151,1151,1151,1151,
  1151,1151,1151,1151,1151,1151,1151,1151,1151,1151,1151,1151,1151,1151,
  1151,1151,1151,1151,1151,1151,1361,1361,1361,1361,1371,1371,1350,1350,
  1350,1328,1328,1334,1334,1334,1334,1334,1334,1334,1334,1334,1334,1334,
  1151,1992,2213,2213,1134,1134,  0,  0,1140,1140,  0,  0,1140,1140,2233,
  2233,  0,  0,1151,1151,1151,1151,1151,1151,1134,1134,1134,1134,1134,1134,
  1134,1134,1134,1134,1151,1151,1151,1151,1151,1151,1151,1151,1151,1151,
  1140,1151,1151,1151,1151,1134,1311,1311,1140,  0,1140,  0,1151,1140,1140,
  1140,  0,  0,1560,1419,1419,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,
  1151,  0,1641,1641,1641,1641,1641,1641,1641,1151,1151,1151,1151,1151,1151,
  1151,1151,1151,1151,1151,1151,1151,1151,1151,1151,1151,1151,1151,1151,
  1151,1151,1151,1151,1151,1151,1151,1151,1151,1151,1151,1140,1140,1140,
  1140,1140,1134,1140,1140,  0,1140,1140,  0,1140,1140,1140,1140,1151,1151,
  1151,1151,1140,1140,1140,1318,1318,1318,2240,1318,1151,1151,1151,1318,
  1318,2244,1560,1560,1513,2244,  0,2249,2213,1560,  0,1151,1513,1513,1513,
  1513,1513,2253,  0,  0,  0,1151,  0,1134,1151,1151,1151,1151,1151,1151,
  1151,1151,1151,1151,  0,1641,1641,1641,1641,1641,1151,1151,1151,1151,1151,
  1151,1151,1151,1151,1151,1151,1151,1140,1140,1140,1140,1140,1140,1140,
  1140,1151,1151,1151,2244,1560,2270,1151,2288,1318,1560,1151,1513,1134,
  2293,2293,1140,  0,  0,1140,1134,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,
    0,1701,1701,1741,1741,1741,1741,1741,1741,1773,1773,1804,1804,1140,1140,
  1140,1334,1560,1151,1151,1134,1140,  0,  0,  0,1334,  0
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
69,70,73,78,69,0, 70,0, 69,0, 68,73,70,0, 82,79,82,0, 69,70,0, 
68,69,70,0, 67,76,85,68,69,0, 71,77,65,0, 78,68,69,70,0, 61,0, 
61,0, 73,0, 72,76,0, 71,0, 82,0, 79,67,75,0, 84,69,0, 76,76,0, 
69,71,0, 76,0, 80,0, 71,0, 66,0, 72,79,0, 83,69,0, 70,0, 
82,89,0, 85,0, 68,69,68,0, 78,0, 78,0, 73,76,76,0, 88,0, 
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
  680,682,521,511,683,459,458,519,517,454,518,429,520,684,685,686,686,687,
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
  424,423,164,163,162,135,134,1,0,51,52,
1,0,
704,702,701,700,699,698,697,696,695,694,693,691,682,438,429,427,425,424,423,
  164,163,162,135,134,64,1,0,11,34,35,36,37,51,52,53,65,66,133,428,442,
457,456,455,453,452,451,450,449,448,447,446,440,433,1,0,51,52,
704,702,701,700,699,698,697,696,695,694,693,691,688,687,686,685,684,682,465,
  432,429,427,425,424,423,161,135,1,0,138,
457,456,455,453,452,451,450,449,448,447,446,440,433,0,60,67,68,74,76,78,79,
  80,81,82,83,84,86,87,
704,702,701,700,699,698,697,696,695,694,693,691,682,679,678,677,676,675,674,
  673,672,671,670,669,668,667,666,665,664,663,662,661,660,659,658,657,656,
  655,654,653,652,651,650,649,648,647,646,645,644,643,642,641,640,639,638,
  637,636,635,634,633,632,631,630,629,628,627,626,625,624,623,622,621,620,
  619,618,617,616,615,614,613,612,611,610,609,607,606,605,604,603,602,601,
  600,599,598,597,596,595,593,592,591,590,589,588,587,586,585,584,583,582,
  581,580,579,578,577,576,575,574,573,572,571,570,569,568,567,566,565,564,
  563,562,561,560,559,558,557,556,555,554,553,552,551,550,549,548,547,546,
  545,491,490,489,488,487,486,484,483,482,481,480,479,478,477,476,475,474,
  473,472,471,470,469,468,467,466,465,464,463,462,461,455,453,451,450,447,
  445,444,443,441,439,432,431,430,429,427,425,424,423,135,134,1,0,51,52,
423,0,45,
434,433,429,427,425,424,423,0,39,40,41,45,46,47,49,54,55,
704,702,701,700,699,698,697,696,695,694,693,691,682,679,678,677,676,675,674,
  673,672,671,670,669,668,667,666,665,664,663,662,661,660,659,658,657,656,
  655,654,653,652,651,650,649,648,647,646,645,644,643,642,641,640,639,638,
  637,636,635,634,633,632,631,630,629,628,627,626,625,624,623,622,621,620,
  619,618,617,616,615,614,613,612,611,610,609,607,606,605,604,603,602,601,
  600,599,598,597,596,595,593,592,591,590,589,588,587,586,585,584,583,582,
  581,580,579,578,577,576,575,574,573,572,571,570,569,568,567,566,565,564,
  563,562,561,560,559,558,557,556,555,554,553,552,551,550,549,548,547,546,
  545,491,490,489,488,487,486,484,483,482,481,480,479,478,477,476,475,474,
  473,472,471,470,469,468,467,466,465,464,463,462,461,455,453,451,450,447,
  445,444,443,441,439,432,431,430,429,427,425,424,423,135,134,1,0,51,52,
704,702,701,700,699,698,697,696,695,694,693,691,682,679,678,677,676,675,674,
  673,672,671,670,669,668,667,666,665,664,663,662,661,660,659,658,657,656,
  655,654,653,652,651,650,649,648,647,646,645,644,643,642,641,640,639,638,
  637,636,635,634,633,632,631,630,629,628,627,626,625,624,623,622,621,620,
  619,618,617,616,615,614,613,612,611,610,609,607,606,605,604,603,602,601,
  600,599,598,597,596,595,593,592,591,590,589,588,587,586,585,584,583,582,
  581,580,579,578,577,576,575,574,573,572,571,570,569,568,567,566,565,564,
  563,562,561,560,559,558,557,556,555,554,553,552,551,550,549,548,547,546,
  545,491,490,489,488,487,486,484,483,482,481,480,479,478,477,476,475,474,
  473,472,471,470,469,468,467,466,465,464,463,462,461,455,453,451,450,447,
  445,444,443,441,439,434,433,429,427,425,424,423,135,134,1,0,21,30,31,32,
  33,54,55,63,76,80,81,88,96,133,244,257,258,259,260,261,262,263,264,265,
  266,268,269,270,271,272,273,274,275,276,277,278,279,280,281,282,283,284,
  285,286,287,288,289,290,291,292,293,294,295,296,297,298,299,300,301,302,
  303,304,305,306,307,308,309,310,311,312,313,314,315,316,317,318,319,320,
  321,322,323,324,325,326,327,328,329,330,331,332,333,334,335,336,337,338,
  339,340,341,342,343,344,346,347,348,349,350,351,352,353,354,355,356,357,
  358,359,360,361,362,363,364,365,366,367,368,369,370,371,372,373,374,375,
  376,377,378,379,380,381,382,383,384,385,386,387,388,389,390,391,392,393,
  394,395,396,397,398,399,400,401,402,403,404,405,406,407,408,409,410,411,
  412,442,
704,702,701,700,699,698,697,696,695,694,693,691,682,438,429,427,425,424,423,
  164,163,162,135,134,64,38,1,0,11,35,36,51,52,53,65,66,133,428,442,
704,702,701,700,699,698,697,696,695,694,693,691,682,135,134,1,0,51,52,
704,702,701,700,699,698,697,696,695,694,693,691,682,135,134,1,0,51,52,
681,1,0,51,52,
681,1,0,51,52,
681,1,0,51,52,
427,425,424,423,1,0,51,52,
427,425,424,423,1,0,51,52,
704,702,701,700,699,698,697,696,695,694,693,691,682,135,134,1,0,51,52,
704,702,701,700,699,698,697,696,695,694,693,692,691,688,687,686,685,684,683,
  682,537,535,534,533,532,531,530,529,528,527,526,525,524,521,518,517,491,
  459,429,231,135,134,1,0,51,52,
704,702,701,700,699,698,697,696,695,694,693,692,691,688,687,686,685,684,683,
  682,537,535,534,533,532,531,530,529,528,527,526,525,524,521,518,517,491,
  459,429,231,135,134,1,0,51,52,
704,702,701,700,699,698,697,696,695,694,693,691,682,135,134,1,0,51,52,
445,444,443,441,439,1,0,51,52,
704,703,702,701,700,699,698,697,696,695,694,693,689,688,687,686,685,684,681,
  520,429,1,0,51,52,
704,702,701,700,699,698,697,696,695,694,693,691,682,135,134,0,21,88,133,442,
704,702,701,700,699,698,697,696,695,694,693,691,682,135,134,0,21,133,442,
681,0,10,12,435,
681,0,10,12,435,
681,0,10,12,435,
427,425,424,423,1,0,51,52,
427,425,424,423,1,0,51,52,
704,702,701,700,699,698,697,696,695,694,693,691,682,135,134,0,21,133,442,
704,702,701,700,699,698,697,696,695,694,693,692,691,688,687,686,685,684,683,
  682,537,535,534,533,532,531,530,529,528,527,526,525,524,521,518,517,491,
  459,429,231,135,134,0,2,6,7,8,9,15,17,21,23,24,25,26,27,28,29,57,77,88,
  90,131,133,166,196,198,203,204,205,206,207,208,209,210,211,212,213,214,
  215,216,217,218,219,442,492,538,
704,702,701,700,699,698,697,696,695,694,693,692,691,688,687,686,685,684,683,
  682,537,535,534,533,532,531,530,529,528,527,526,525,524,521,518,517,491,
  459,429,231,135,134,0,2,6,7,8,9,15,17,21,23,24,25,26,27,28,29,57,77,88,
  90,131,133,166,196,198,203,204,205,206,207,208,209,210,211,212,213,214,
  215,216,217,218,219,442,492,538,
704,702,701,700,699,698,697,696,695,694,693,691,682,135,134,0,21,133,442,
445,444,443,441,439,0,69,70,71,72,73,
689,681,0,10,12,13,14,435,436,
705,704,703,702,701,700,699,698,697,696,695,694,693,692,691,690,689,688,687,
  686,685,684,683,682,681,680,526,524,521,520,519,518,517,511,509,507,459,
  458,454,438,434,433,429,427,426,425,424,423,164,163,162,135,134,38,1,0,
  51,52,
705,704,703,702,701,700,699,698,697,696,695,694,693,692,691,690,689,688,687,
  686,685,684,683,682,681,680,526,524,521,520,519,518,517,511,509,507,491,
  490,489,488,487,486,484,483,482,481,480,479,478,477,476,475,474,473,472,
  471,470,469,468,467,466,465,464,463,462,461,460,459,458,455,454,453,445,
  444,443,441,439,434,433,431,430,429,427,426,425,424,423,1,0,51,52,
434,433,0,60,61,62,
705,704,703,702,701,700,699,698,697,696,695,694,693,692,691,690,689,688,687,
  686,685,684,683,682,681,680,526,524,521,520,519,518,517,511,509,507,459,
  458,454,429,427,426,425,423,1,0,51,52,
705,704,703,702,701,700,699,698,697,696,695,694,693,692,691,690,689,688,687,
  686,685,684,683,682,681,680,526,524,521,520,519,518,517,511,509,507,460,
  459,458,454,429,427,426,425,424,423,1,0,51,52,
705,704,703,702,701,700,699,698,697,696,695,694,693,692,691,690,689,688,687,
  686,685,684,683,682,681,680,526,524,521,520,519,518,517,511,509,507,459,
  458,454,429,425,423,1,0,51,52,
705,704,703,702,701,700,699,698,697,696,695,694,693,692,691,690,689,688,687,
  686,685,684,683,682,681,680,526,524,521,520,519,518,517,511,509,507,459,
  458,454,429,427,426,425,423,1,0,40,45,46,47,48,49,54,85,90,92,140,141,
  143,145,146,147,148,149,150,154,157,158,185,187,189,195,196,197,199,200,
  203,205,220,224,225,229,413,414,415,416,417,418,419,420,421,422,
705,704,703,702,701,700,699,698,697,696,695,694,693,692,691,690,689,688,687,
  686,685,684,683,682,681,680,526,524,521,520,519,518,517,511,509,507,459,
  458,454,429,425,423,1,0,43,44,46,54,85,90,92,140,141,143,145,146,147,
  148,149,150,154,157,158,185,187,189,195,196,197,199,200,203,205,220,224,
  225,229,413,414,415,416,417,418,419,420,421,422,
705,704,703,702,701,700,699,698,697,696,695,694,693,692,691,690,689,688,687,
  686,685,684,683,682,681,680,526,524,521,520,519,518,517,511,509,507,459,
  458,454,429,425,423,1,0,43,44,46,54,85,90,92,140,141,143,145,146,147,
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
  473,472,471,470,469,468,467,466,465,464,463,462,461,455,453,451,450,447,
  445,444,443,441,439,432,431,430,429,427,425,424,423,135,134,0,21,30,31,
  32,33,39,40,41,45,46,47,49,54,55,59,63,76,80,81,88,96,133,244,257,258,
  259,260,261,262,263,264,265,266,268,269,270,271,272,273,274,275,276,277,
  278,279,280,281,282,283,284,285,286,287,288,289,290,291,292,293,294,295,
  296,297,298,299,300,301,302,303,304,305,306,307,308,309,310,311,312,313,
  314,315,316,317,318,319,320,321,322,323,324,325,326,327,328,329,330,331,
  332,333,334,335,336,337,338,339,340,341,342,343,344,346,347,348,349,350,
  351,352,353,354,355,356,357,358,359,360,361,362,363,364,365,366,367,368,
  369,370,371,372,373,374,375,376,377,378,379,380,381,382,383,384,385,386,
  387,388,389,390,391,392,393,394,395,396,397,398,399,400,401,402,403,404,
  405,406,407,408,409,410,411,412,442,
494,491,427,425,424,423,1,0,51,52,
544,427,425,424,423,1,0,51,52,
544,427,425,424,423,1,0,51,52,
544,427,425,424,423,1,0,51,52,
544,427,425,424,423,1,0,51,52,
544,427,425,424,423,1,0,51,52,
544,427,425,424,423,1,0,51,52,
544,427,425,424,423,1,0,51,52,
544,427,425,424,423,1,0,51,52,
544,427,425,424,423,1,0,51,52,
544,427,425,424,423,1,0,51,52,
544,427,425,424,423,1,0,51,52,
544,427,425,424,423,1,0,51,52,
544,427,425,424,423,1,0,51,52,
544,427,425,424,423,1,0,51,52,
668,543,542,541,248,246,238,236,234,1,0,51,52,
540,241,240,239,238,237,236,235,234,1,0,51,52,
668,543,248,246,238,236,234,1,0,51,52,
704,702,701,700,699,698,697,696,695,694,693,692,691,688,687,686,685,684,683,
  682,537,535,534,533,532,531,530,529,528,527,526,525,524,521,518,517,491,
  459,429,231,135,134,1,0,51,52,
704,702,701,700,699,698,697,696,695,694,693,692,691,688,687,686,685,684,683,
  682,537,535,534,533,532,531,530,529,528,527,526,525,524,521,518,517,491,
  459,429,231,135,134,1,0,51,52,
704,702,701,700,699,698,697,696,695,694,693,692,691,688,687,686,685,684,683,
  682,537,535,534,533,532,531,530,529,528,527,526,525,524,521,518,517,491,
  459,429,231,135,134,1,0,51,52,
704,702,701,700,699,698,697,696,695,694,693,692,691,688,687,686,685,684,683,
  682,537,535,534,533,532,531,530,529,528,527,526,525,524,521,518,517,491,
  459,429,231,135,134,1,0,51,52,
704,702,701,700,699,698,697,696,695,694,693,692,691,688,687,686,685,684,683,
  682,537,535,534,533,532,531,530,529,528,527,526,525,524,521,518,517,491,
  459,429,231,135,134,1,0,51,52,
704,702,701,700,699,698,697,696,695,694,693,692,691,688,687,686,685,684,683,
  682,537,535,534,533,532,531,530,529,528,527,526,525,524,521,518,517,491,
  459,429,231,135,134,1,0,51,52,
704,702,701,700,699,698,697,696,695,694,693,692,691,688,687,686,685,684,683,
  682,537,535,534,533,532,531,530,529,528,527,526,525,524,521,518,517,491,
  459,429,231,135,134,1,0,51,52,
704,702,701,700,699,698,697,696,695,694,693,692,691,688,687,686,685,684,683,
  682,537,535,534,533,532,531,530,529,528,527,526,525,524,521,518,517,491,
  459,429,231,135,134,1,0,51,52,
704,702,701,700,699,698,697,696,695,694,693,692,691,688,687,686,685,684,683,
  682,537,535,534,533,532,531,530,529,528,527,526,525,524,521,518,517,491,
  459,429,231,135,134,1,0,51,52,
704,702,701,700,699,698,697,696,695,694,693,692,691,688,687,686,685,684,683,
  682,537,535,534,533,532,531,530,529,528,527,526,525,524,521,518,517,491,
  459,429,231,135,134,1,0,51,52,
704,702,701,700,699,698,697,696,695,694,693,692,691,688,687,686,685,684,683,
  682,537,535,534,533,532,531,530,529,528,527,526,525,524,521,518,517,491,
  459,429,231,135,134,1,0,51,52,
704,702,701,700,699,698,697,696,695,694,693,692,691,688,687,686,685,684,683,
  682,537,535,534,533,532,531,530,529,528,527,526,525,524,521,518,517,491,
  459,429,231,135,134,1,0,51,52,
704,702,701,700,699,698,697,696,695,694,693,692,691,688,687,686,685,684,683,
  682,537,535,534,533,532,531,530,529,528,527,526,525,524,521,518,517,491,
  459,429,231,135,134,1,0,51,52,
704,702,701,700,699,698,697,696,695,694,693,692,691,688,687,686,685,684,683,
  682,537,535,534,533,532,531,530,529,528,527,526,525,524,521,518,517,491,
  459,429,231,135,134,1,0,51,52,
704,702,701,700,699,698,697,696,695,694,693,692,691,688,687,686,685,684,683,
  682,537,535,534,533,532,531,530,529,528,527,526,525,524,521,518,517,491,
  459,429,231,135,134,1,0,51,52,
704,702,701,700,699,698,697,696,695,694,693,692,691,688,687,686,685,684,683,
  682,537,535,534,533,532,531,530,529,528,527,526,525,524,521,518,517,491,
  459,429,231,135,134,1,0,51,52,
544,427,425,424,423,1,0,51,52,
544,427,425,424,423,1,0,51,52,
544,427,425,424,423,1,0,51,52,
544,427,425,424,423,1,0,51,52,
544,427,425,424,423,1,0,51,52,
544,427,425,424,423,1,0,51,52,
544,427,425,424,423,1,0,51,52,
544,427,425,424,423,1,0,51,52,
544,427,425,424,423,1,0,51,52,
544,427,425,424,423,1,0,51,52,
544,427,425,424,423,1,0,51,52,
544,427,425,424,423,1,0,51,52,
544,427,425,424,423,1,0,51,52,
544,427,425,424,423,1,0,51,52,
544,427,425,424,423,1,0,51,52,
544,427,425,424,423,1,0,51,52,
544,427,425,424,423,1,0,51,52,
544,427,425,424,423,1,0,51,52,
544,427,425,424,423,1,0,51,52,
544,427,425,424,423,1,0,51,52,
544,427,425,424,423,1,0,51,52,
544,427,425,424,423,1,0,51,52,
544,427,425,424,423,1,0,51,52,
544,427,425,424,423,1,0,51,52,
544,427,425,424,423,1,0,51,52,
544,427,425,424,423,1,0,51,52,
544,427,425,424,423,1,0,51,52,
544,427,425,424,423,1,0,51,52,
544,427,425,424,423,1,0,51,52,
687,686,685,684,1,0,51,52,
668,543,542,541,248,246,238,236,234,1,0,51,52,
540,241,240,239,238,237,236,235,234,1,0,51,52,
668,543,248,246,238,236,234,1,0,51,52,
704,702,701,700,699,698,697,696,695,694,693,692,691,688,687,686,685,684,683,
  682,537,535,534,533,532,531,530,529,528,527,526,525,524,521,518,517,491,
  459,429,231,135,134,1,0,51,52,
704,702,701,700,699,698,697,696,695,694,693,692,691,688,687,686,685,684,683,
  682,537,535,534,533,532,531,530,529,528,527,526,525,524,521,518,517,491,
  459,429,231,135,134,1,0,51,52,
704,702,701,700,699,698,697,696,695,694,693,692,691,688,687,686,685,684,683,
  682,537,535,534,533,532,531,530,529,528,527,526,525,524,521,518,517,491,
  459,429,231,135,134,1,0,51,52,
704,702,701,700,699,698,697,696,695,694,693,692,691,688,687,686,685,684,683,
  682,537,535,534,533,532,531,530,529,528,527,526,525,524,521,518,517,491,
  459,429,231,135,134,1,0,51,52,
704,702,701,700,699,698,697,696,695,694,693,692,691,688,687,686,685,684,683,
  682,537,535,534,533,532,531,530,529,528,527,526,525,524,521,518,517,491,
  459,429,231,135,134,1,0,51,52,
704,702,701,700,699,698,697,696,695,694,693,692,691,688,687,686,685,684,683,
  682,537,535,534,533,532,531,530,529,528,527,526,525,524,521,518,517,491,
  459,429,231,135,134,1,0,51,52,
704,702,701,700,699,698,697,696,695,694,693,692,691,688,687,686,685,684,683,
  682,537,535,534,533,532,531,530,529,528,527,526,525,524,521,518,517,491,
  459,429,231,135,134,1,0,51,52,
704,702,701,700,699,698,697,696,695,694,693,692,691,688,687,686,685,684,683,
  682,537,535,534,533,532,531,530,529,528,527,526,525,524,521,518,517,491,
  459,429,231,135,134,1,0,51,52,
704,702,701,700,699,698,697,696,695,694,693,692,691,688,687,686,685,684,683,
  682,537,535,534,533,532,531,530,529,528,527,526,525,524,521,518,517,491,
  459,429,231,135,134,1,0,51,52,
704,702,701,700,699,698,697,696,695,694,693,692,691,688,687,686,685,684,683,
  682,537,535,534,533,532,531,530,529,528,527,526,525,524,521,518,517,491,
  459,429,231,135,134,1,0,51,52,
704,702,701,700,699,698,697,696,695,694,693,692,691,688,687,686,685,684,683,
  682,537,535,534,533,532,531,530,529,528,527,526,525,524,521,518,517,491,
  459,429,231,135,134,1,0,51,52,
704,702,701,700,699,698,697,696,695,694,693,692,691,688,687,686,685,684,683,
  682,537,535,534,533,532,531,530,529,528,527,526,525,524,521,518,517,491,
  459,429,231,135,134,1,0,51,52,
704,702,701,700,699,698,697,696,695,694,693,692,691,688,687,686,685,684,683,
  682,537,535,534,533,532,531,530,529,528,527,526,525,524,521,518,517,491,
  459,429,231,135,134,1,0,51,52,
704,702,701,700,699,698,697,696,695,694,693,692,691,688,687,686,685,684,683,
  682,537,535,534,533,532,531,530,529,528,527,526,525,524,521,518,517,491,
  459,429,231,135,134,1,0,51,52,
704,702,701,700,699,698,697,696,695,694,693,692,691,688,687,686,685,684,683,
  682,537,535,534,533,532,531,530,529,528,527,526,525,524,521,518,517,491,
  459,429,231,135,134,1,0,51,52,
704,702,701,700,699,698,697,696,695,694,693,692,691,688,687,686,685,684,683,
  682,537,535,534,533,532,531,530,529,528,527,526,525,524,521,518,517,491,
  459,429,231,135,134,1,0,51,52,
704,702,701,700,699,698,697,696,695,694,693,692,691,688,687,686,685,684,683,
  682,537,535,534,533,532,531,530,529,528,527,526,525,524,521,518,517,491,
  459,429,231,135,134,1,0,51,52,
704,702,701,700,699,698,697,696,695,694,693,692,691,688,687,686,685,684,683,
  682,537,535,534,533,532,531,530,529,528,527,526,525,524,521,518,517,491,
  459,429,231,135,134,1,0,51,52,
704,702,701,700,699,698,697,696,695,694,693,692,691,688,687,686,685,684,683,
  682,537,535,534,533,532,531,530,529,528,527,526,525,524,521,518,517,491,
  459,429,231,135,134,1,0,51,52,
704,702,701,700,699,698,697,696,695,694,693,692,691,688,687,686,685,684,683,
  682,537,535,534,533,532,531,530,529,528,527,526,525,524,521,518,517,491,
  459,429,231,135,134,1,0,51,52,
704,702,701,700,699,698,697,696,695,694,693,692,691,688,687,686,685,684,683,
  682,537,535,534,533,532,531,530,529,528,527,526,525,524,521,518,517,491,
  459,429,231,135,134,1,0,51,52,
704,702,701,700,699,698,697,696,695,694,693,692,691,688,687,686,685,684,683,
  682,537,535,534,533,532,531,530,529,528,527,526,525,524,521,518,517,491,
  459,429,231,135,134,1,0,51,52,
704,702,701,700,699,698,697,696,695,694,693,692,691,688,687,686,685,684,683,
  682,537,535,534,533,532,531,530,529,528,527,526,525,524,521,518,517,491,
  459,429,231,135,134,1,0,51,52,
704,702,701,700,699,698,697,696,695,694,693,692,691,688,687,686,685,684,683,
  682,537,535,534,533,532,531,530,529,528,527,526,525,524,521,518,517,491,
  459,429,231,135,134,1,0,51,52,
704,702,701,700,699,698,697,696,695,694,693,692,691,688,687,686,685,684,683,
  682,537,535,534,533,532,531,530,529,528,527,526,525,524,521,518,517,491,
  459,429,231,135,134,1,0,51,52,
704,702,701,700,699,698,697,696,695,694,693,692,691,688,687,686,685,684,683,
  682,537,535,534,533,532,531,530,529,528,527,526,525,524,521,518,517,491,
  459,429,231,135,134,1,0,51,52,
704,702,701,700,699,698,697,696,695,694,693,692,691,688,687,686,685,684,683,
  682,537,535,534,533,532,531,530,529,528,527,526,525,524,521,518,517,491,
  459,429,231,135,134,1,0,51,52,
704,702,701,700,699,698,697,696,695,694,693,692,691,688,687,686,685,684,683,
  682,537,535,534,533,532,531,530,529,528,527,526,525,524,521,518,517,491,
  459,429,231,135,134,1,0,51,52,
704,702,701,700,699,698,697,696,695,694,693,692,691,688,687,686,685,684,683,
  682,537,535,534,533,532,531,530,529,528,527,526,525,524,521,518,517,491,
  459,429,231,135,134,1,0,51,52,
704,702,701,700,699,698,697,696,695,694,693,692,691,688,687,686,685,684,683,
  682,537,535,534,533,532,531,530,529,528,527,526,525,524,521,518,517,491,
  459,429,231,135,134,1,0,51,52,
704,702,701,700,699,698,697,696,695,694,693,692,691,688,687,686,685,684,683,
  682,537,535,534,533,532,531,530,529,528,527,526,525,524,521,518,517,491,
  459,429,231,135,134,1,0,51,52,
704,702,701,700,699,698,697,696,695,694,693,692,691,688,687,686,685,684,683,
  682,537,535,534,533,532,531,530,529,528,527,526,525,524,521,518,517,491,
  459,429,231,135,134,1,0,51,52,
704,702,701,700,699,698,697,696,695,694,693,692,691,688,687,686,685,684,683,
  682,537,535,534,533,532,531,530,529,528,527,526,525,524,521,518,517,491,
  459,429,231,135,134,1,0,51,52,
704,702,701,700,699,698,697,696,695,694,693,692,691,688,687,686,685,684,683,
  682,537,535,534,533,532,531,530,529,528,527,526,525,524,521,518,517,491,
  459,429,231,135,134,1,0,51,52,
704,702,701,700,699,698,697,696,695,694,693,692,691,688,687,686,685,684,683,
  682,537,535,534,533,532,531,530,529,528,527,526,525,524,521,518,517,491,
  459,429,231,135,134,1,0,51,52,
704,702,701,700,699,698,697,696,695,694,693,692,691,688,687,686,685,684,683,
  682,537,535,534,533,532,531,530,529,528,527,526,525,524,521,518,517,491,
  459,429,231,135,134,1,0,51,52,
704,702,701,700,699,698,697,696,695,694,693,692,691,688,687,686,685,684,683,
  682,537,535,534,533,532,531,530,529,528,527,526,525,524,521,518,517,491,
  459,429,231,135,134,1,0,51,52,
704,702,701,700,699,698,697,696,695,694,693,692,691,688,687,686,685,684,683,
  682,537,535,534,533,532,531,530,529,528,527,526,525,524,521,518,517,491,
  459,429,231,135,134,1,0,51,52,
704,702,701,700,699,698,697,696,695,694,693,692,691,688,687,686,685,684,683,
  682,537,535,534,533,532,531,530,529,528,527,526,525,524,521,518,517,491,
  459,429,231,135,134,1,0,51,52,
704,702,701,700,699,698,697,696,695,694,693,692,691,688,687,686,685,684,683,
  682,537,535,534,533,532,531,530,529,528,527,526,525,524,521,518,517,491,
  459,429,231,135,134,1,0,51,52,
704,702,701,700,699,698,697,696,695,694,693,692,691,688,687,686,685,684,683,
  682,537,535,534,533,532,531,530,529,528,527,526,525,524,521,518,517,491,
  459,429,231,135,134,1,0,51,52,
704,702,701,700,699,698,697,696,695,694,693,692,691,688,687,686,685,684,683,
  682,537,535,534,533,532,531,530,529,528,527,526,525,524,521,518,517,491,
  459,429,231,135,134,1,0,51,52,
704,702,701,700,699,698,697,696,695,694,693,692,691,688,687,686,685,684,683,
  682,537,535,534,533,532,531,530,529,528,527,526,525,524,521,518,517,491,
  459,429,231,135,134,1,0,51,52,
704,702,701,700,699,698,697,696,695,694,693,692,691,688,687,686,685,684,683,
  682,537,535,534,533,532,531,530,529,528,527,526,525,524,521,518,517,491,
  459,429,231,135,134,1,0,51,52,
704,702,701,700,699,698,697,696,695,694,693,692,691,688,687,686,685,684,683,
  682,537,535,534,533,532,531,530,529,528,527,526,525,524,521,518,517,491,
  459,429,231,135,134,1,0,51,52,
704,702,701,700,699,698,697,696,695,694,693,692,691,688,687,686,685,684,683,
  682,537,535,534,533,532,531,530,529,528,527,526,525,524,521,518,517,491,
  459,429,231,135,134,1,0,51,52,
704,702,701,700,699,698,697,696,695,694,693,692,691,688,687,686,685,684,683,
  682,537,535,534,533,532,531,530,529,528,527,526,525,524,521,518,517,491,
  459,429,231,135,134,1,0,51,52,
704,702,701,700,699,698,697,696,695,694,693,692,691,688,687,686,685,684,683,
  682,537,535,534,533,532,531,530,529,528,527,526,525,524,521,518,517,491,
  459,429,231,135,134,1,0,51,52,
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
454,1,0,51,52,
454,1,0,51,52,
454,1,0,51,52,
704,702,701,700,699,698,697,696,695,694,693,692,691,688,687,686,685,684,683,
  682,537,535,534,533,532,531,530,529,528,527,526,525,524,521,518,517,491,
  459,429,231,135,134,1,0,51,52,
704,702,701,700,699,698,697,696,695,694,693,692,691,688,687,686,685,684,683,
  682,537,535,534,533,532,531,530,529,528,527,526,525,524,521,518,517,491,
  459,429,231,135,134,1,0,51,52,
704,702,701,700,699,698,697,696,695,694,693,692,691,688,687,686,685,684,683,
  682,537,535,534,533,532,531,530,529,528,527,526,525,524,521,518,517,491,
  459,429,231,135,134,1,0,51,52,
704,702,701,700,699,698,697,696,695,694,693,692,691,688,687,686,685,684,683,
  682,537,535,534,533,532,531,530,529,528,527,526,525,524,521,518,517,491,
  459,429,231,135,134,1,0,51,52,
704,702,701,700,699,698,697,696,695,694,693,692,691,688,687,686,685,684,683,
  682,537,535,534,533,532,531,530,529,528,527,526,525,524,521,518,517,491,
  459,429,231,135,134,1,0,51,52,
704,702,701,700,699,698,697,696,695,694,693,692,691,688,687,686,685,684,683,
  682,537,535,534,533,532,531,530,529,528,527,526,525,524,521,518,517,491,
  459,429,231,135,134,1,0,51,52,
704,702,701,700,699,698,697,696,695,694,693,692,691,688,687,686,685,684,683,
  682,537,535,534,533,532,531,530,529,528,527,526,525,524,521,518,517,491,
  459,429,231,135,134,1,0,51,52,
704,702,701,700,699,698,697,696,695,694,693,692,691,688,687,686,685,684,683,
  682,537,535,534,533,532,531,530,529,528,527,526,525,524,521,518,517,491,
  459,429,231,135,134,1,0,51,52,
704,702,701,700,699,698,697,696,695,694,693,692,691,688,687,686,685,684,683,
  682,537,535,534,533,532,531,530,529,528,527,526,525,524,521,518,517,491,
  459,429,231,135,134,1,0,51,52,
704,702,701,700,699,698,697,696,695,694,693,692,691,688,687,686,685,684,683,
  682,537,535,534,533,532,531,530,529,528,527,526,525,524,521,518,517,491,
  459,429,231,135,134,1,0,51,52,
704,702,701,700,699,698,697,696,695,694,693,692,691,688,687,686,685,684,683,
  682,537,535,534,533,532,531,530,529,528,527,526,525,524,521,518,517,491,
  459,429,231,135,134,1,0,51,52,
704,702,701,700,699,698,697,696,695,694,693,692,691,688,687,686,685,684,683,
  682,537,535,534,533,532,531,530,529,528,527,526,525,524,521,518,517,491,
  459,429,231,135,134,1,0,51,52,
704,702,701,700,699,698,697,696,695,694,693,692,691,688,687,686,685,684,683,
  682,537,535,534,533,532,531,530,529,528,527,526,525,524,521,518,517,491,
  459,429,231,135,134,1,0,51,52,
704,702,701,700,699,698,697,696,695,694,693,692,691,688,687,686,685,684,683,
  682,537,535,534,533,532,531,530,529,528,527,526,525,524,521,518,517,491,
  459,429,231,135,134,1,0,51,52,
704,702,701,700,699,698,697,696,695,694,693,692,691,688,687,686,685,684,683,
  682,537,535,534,533,532,531,530,529,528,527,526,525,524,521,518,517,491,
  459,429,231,135,134,1,0,51,52,
704,702,701,700,699,698,697,696,695,694,693,692,691,688,687,686,685,684,683,
  682,537,535,534,533,532,531,530,529,528,527,526,525,524,521,518,517,491,
  459,429,231,135,134,1,0,51,52,
704,702,701,700,699,698,697,696,695,694,693,692,691,688,687,686,685,684,683,
  682,537,535,534,533,532,531,530,529,528,527,526,525,524,521,518,517,491,
  459,429,231,135,134,1,0,51,52,
704,702,701,700,699,698,697,696,695,694,693,692,691,688,687,686,685,684,683,
  682,537,535,534,533,532,531,530,529,528,527,526,525,524,521,518,517,491,
  459,429,231,135,134,1,0,51,52,
704,702,701,700,699,698,697,696,695,694,693,692,691,688,687,686,685,684,683,
  682,537,535,534,533,532,531,530,529,528,527,526,525,524,521,518,517,491,
  459,429,231,135,134,1,0,51,52,
704,702,701,700,699,698,697,696,695,694,693,692,691,688,687,686,685,684,683,
  682,537,535,534,533,532,531,530,529,528,527,526,525,524,521,518,517,491,
  459,429,231,135,134,1,0,51,52,
704,702,701,700,699,698,697,696,695,694,693,692,691,688,687,686,685,684,683,
  682,537,535,534,533,532,531,530,529,528,527,526,525,524,521,518,517,491,
  459,429,231,135,134,1,0,51,52,
704,702,701,700,699,698,697,696,695,694,693,692,691,688,687,686,685,684,683,
  682,537,535,534,533,532,531,530,529,528,527,526,525,524,521,518,517,491,
  459,429,231,135,134,1,0,51,52,
704,702,701,700,699,698,697,696,695,694,693,692,691,688,687,686,685,684,683,
  682,537,535,534,533,532,531,530,529,528,527,526,525,524,521,518,517,491,
  459,429,231,135,134,1,0,51,52,
704,702,701,700,699,698,697,696,695,694,693,692,691,688,687,686,685,684,683,
  682,537,535,534,533,532,531,530,529,528,527,526,525,524,521,518,517,491,
  459,429,231,135,134,1,0,51,52,
704,702,701,700,699,698,697,696,695,694,693,692,691,688,687,686,685,684,683,
  682,537,535,534,533,532,531,530,529,528,527,526,525,524,521,518,517,491,
  459,429,231,135,134,1,0,51,52,
704,702,701,700,699,698,697,696,695,694,693,692,691,688,687,686,685,684,683,
  682,537,535,534,533,532,531,530,529,528,527,526,525,524,521,518,517,491,
  459,429,231,135,134,1,0,51,52,
704,702,701,700,699,698,697,696,695,694,693,692,691,688,687,686,685,684,683,
  682,537,535,534,533,532,531,530,529,528,527,526,525,524,521,518,517,491,
  459,429,231,135,134,1,0,51,52,
704,702,701,700,699,698,697,696,695,694,693,692,691,688,687,686,685,684,683,
  682,537,535,534,533,532,531,530,529,528,527,526,525,524,521,518,517,491,
  459,429,231,135,134,1,0,51,52,
704,702,701,700,699,698,697,696,695,694,693,692,691,688,687,686,685,684,683,
  682,537,535,534,533,532,531,530,529,528,527,526,525,524,521,518,517,491,
  459,429,231,135,134,1,0,51,52,
704,702,701,700,699,698,697,696,695,694,693,692,691,688,687,686,685,684,683,
  682,537,535,534,533,532,531,530,529,528,527,526,525,524,521,518,517,491,
  459,429,231,135,134,1,0,51,52,
704,702,701,700,699,698,697,696,695,694,693,692,691,688,687,686,685,684,683,
  682,537,535,534,533,532,531,530,529,528,527,526,525,524,521,518,517,491,
  459,429,231,135,134,1,0,51,52,
704,702,701,700,699,698,697,696,695,694,693,692,691,688,687,686,685,684,683,
  682,537,535,534,533,532,531,530,529,528,527,526,525,524,521,518,517,491,
  459,429,231,135,134,1,0,51,52,
704,702,701,700,699,698,697,696,695,694,693,692,691,688,687,686,685,684,683,
  682,537,535,534,533,532,531,530,529,528,527,526,525,524,521,518,517,491,
  459,429,231,135,134,1,0,51,52,
704,702,701,700,699,698,697,696,695,694,693,692,691,688,687,686,685,684,683,
  682,537,535,534,533,532,531,530,529,528,527,526,525,524,521,518,517,491,
  459,429,231,135,134,1,0,51,52,
704,702,701,700,699,698,697,696,695,694,693,692,691,688,687,686,685,684,683,
  682,537,535,534,533,532,531,530,529,528,527,526,525,524,521,518,517,491,
  459,429,231,135,134,1,0,51,52,
704,702,701,700,699,698,697,696,695,694,693,692,691,688,687,686,685,684,683,
  682,537,535,534,533,532,531,530,529,528,527,526,525,524,521,518,517,491,
  459,429,231,135,134,1,0,51,52,
704,702,701,700,699,698,697,696,695,694,693,692,691,688,687,686,685,684,683,
  682,537,535,534,533,532,531,530,529,528,527,526,525,524,521,518,517,491,
  459,429,231,135,134,1,0,51,52,
704,702,701,700,699,698,697,696,695,694,693,692,691,688,687,686,685,684,683,
  682,537,535,534,533,532,531,530,529,528,527,526,525,524,521,518,517,491,
  459,429,231,135,134,1,0,51,52,
704,702,701,700,699,698,697,696,695,694,693,692,691,688,687,686,685,684,683,
  682,537,535,534,533,532,531,530,529,528,527,526,525,524,521,518,517,491,
  459,429,231,135,134,1,0,51,52,
704,702,701,700,699,698,697,696,695,694,693,692,691,688,687,686,685,684,683,
  682,537,535,534,533,532,531,530,529,528,527,526,525,524,521,518,517,491,
  459,429,231,135,134,1,0,51,52,
704,702,701,700,699,698,697,696,695,694,693,692,691,688,687,686,685,684,683,
  682,537,535,534,533,532,531,530,529,528,527,526,525,524,521,518,517,491,
  459,429,231,135,134,1,0,51,52,
704,702,701,700,699,698,697,696,695,694,693,692,691,688,687,686,685,684,683,
  682,537,535,534,533,532,531,530,529,528,527,526,525,524,521,518,517,491,
  459,429,231,135,134,1,0,51,52,
704,702,701,700,699,698,697,696,695,694,693,692,691,688,687,686,685,684,683,
  682,537,535,534,533,532,531,530,529,528,527,526,525,524,521,518,517,491,
  459,429,231,135,134,1,0,51,52,
704,702,701,700,699,698,697,696,695,694,693,692,691,688,687,686,685,684,683,
  682,537,535,534,533,532,531,530,529,528,527,526,525,524,521,518,517,491,
  459,429,231,135,134,1,0,51,52,
704,702,701,700,699,698,697,696,695,694,693,692,691,688,687,686,685,684,683,
  682,537,535,534,533,532,531,530,529,528,527,526,525,524,521,518,517,491,
  459,429,231,135,134,1,0,51,52,
704,702,701,700,699,698,697,696,695,694,693,692,691,688,687,686,685,684,683,
  682,537,535,534,533,532,531,530,529,528,527,526,525,524,521,518,517,491,
  459,429,231,135,134,1,0,51,52,
704,702,701,700,699,698,697,696,695,694,693,692,691,688,687,686,685,684,683,
  682,537,535,534,533,532,531,530,529,528,527,526,525,524,521,518,517,491,
  459,429,231,135,134,1,0,51,52,
704,702,701,700,699,698,697,696,695,694,693,692,691,688,687,686,685,684,683,
  682,537,535,534,533,532,531,530,529,528,527,526,525,524,521,518,517,491,
  459,429,231,135,134,1,0,51,52,
704,702,701,700,699,698,697,696,695,694,693,692,691,688,687,686,685,684,683,
  682,537,535,534,533,532,531,530,529,528,527,526,525,524,521,518,517,491,
  459,429,231,135,134,1,0,51,52,
704,702,701,700,699,698,697,696,695,694,693,692,691,688,687,686,685,684,683,
  682,537,535,534,533,532,531,530,529,528,527,526,525,524,521,518,517,491,
  459,429,231,135,134,1,0,51,52,
704,702,701,700,699,698,697,696,695,694,693,692,691,688,687,686,685,684,683,
  682,537,535,534,533,532,531,530,529,528,527,526,525,524,521,518,517,491,
  459,429,231,135,134,1,0,51,52,
704,702,701,700,699,698,697,696,695,694,693,692,691,688,687,686,685,684,683,
  682,537,535,534,533,532,531,530,529,528,527,526,525,524,521,518,517,491,
  459,429,231,135,134,1,0,51,52,
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
  509,507,506,504,502,500,498,496,493,459,458,454,432,429,427,425,424,423,
  1,0,51,52,138,
544,427,425,424,423,1,0,51,52,
705,704,702,701,700,699,698,697,696,695,694,693,692,691,690,689,688,687,686,
  685,684,683,682,681,680,544,526,524,523,522,521,520,519,518,517,516,515,
  514,513,512,511,510,509,508,507,506,505,504,503,502,501,500,499,498,497,
  496,495,493,459,458,454,432,429,427,425,424,423,1,0,90,
427,425,424,423,1,0,51,
544,0,267,
704,702,701,700,699,698,697,696,695,694,693,692,691,688,687,686,685,684,683,
  682,537,535,534,533,532,531,530,529,528,527,526,525,524,521,518,517,491,
  459,429,231,135,134,0,2,6,7,8,9,15,17,21,23,24,25,26,27,28,29,57,77,88,
  90,131,133,166,196,198,203,204,205,206,207,208,209,210,211,212,213,214,
  215,216,217,218,219,442,492,538,
491,490,489,488,487,486,484,483,482,481,480,479,478,477,476,475,474,473,472,
  471,470,469,468,467,466,465,464,463,462,461,455,453,445,444,443,441,439,
  0,69,70,71,72,73,83,84,97,98,99,100,101,102,103,104,105,106,107,108,109,
  110,111,112,113,114,116,117,118,119,120,121,122,123,124,125,126,127,128,
  129,130,131,
427,425,424,423,1,0,51,52,
705,704,702,701,700,699,698,697,696,695,694,693,692,691,690,689,688,687,686,
  685,684,683,682,681,680,526,524,521,520,519,518,517,511,509,507,459,458,
  454,429,425,1,0,51,52,
427,425,424,423,1,0,51,52,
704,703,702,701,700,699,698,697,696,695,694,693,692,691,690,689,688,687,686,
  685,684,683,682,681,680,526,524,521,520,519,518,517,511,509,507,459,458,
  454,429,425,423,1,0,142,
493,458,454,427,425,424,423,1,0,51,52,
454,0,85,
427,425,424,423,1,0,51,52,
427,425,424,423,1,0,51,52,
427,425,424,423,0,39,40,41,45,46,47,49,
427,425,424,423,0,39,40,41,45,46,47,49,
427,425,424,423,1,0,51,52,
688,687,686,685,684,0,
687,686,685,684,0,
685,684,0,
704,702,701,700,699,698,697,696,695,694,693,691,688,687,686,685,684,682,544,
  523,522,521,520,519,518,517,516,515,514,513,512,511,510,509,508,507,506,
  505,504,503,502,501,500,499,498,497,496,495,493,459,458,432,429,427,425,
  424,423,1,0,
704,702,698,696,695,694,693,688,687,686,685,684,682,544,523,522,521,520,519,
  518,517,516,515,514,513,512,511,510,509,508,507,506,505,504,503,502,501,
  500,499,498,497,496,495,493,458,432,429,427,425,424,423,222,221,1,0,
688,687,686,685,684,0,
704,703,702,701,700,699,698,697,696,695,694,693,692,691,690,689,688,687,686,
  685,684,682,681,680,526,524,521,520,519,518,517,511,509,507,459,458,454,
  429,425,423,233,1,0,16,
688,687,686,685,684,0,
698,696,695,694,693,688,687,686,685,684,0,
687,686,685,684,544,523,522,521,520,519,518,517,516,515,514,513,512,511,510,
  509,508,507,506,505,504,503,502,501,500,499,498,497,496,495,493,458,432,
  427,425,424,423,1,0,
685,684,544,523,522,521,520,519,518,517,516,515,514,513,512,511,510,509,508,
  507,506,505,504,503,502,501,500,499,498,497,496,495,493,458,432,427,425,
  424,423,1,0,
696,695,694,693,688,687,686,685,684,544,523,522,521,520,519,518,517,516,515,
  514,513,512,511,510,509,508,507,506,505,504,503,502,501,500,499,498,497,
  496,495,493,458,432,427,425,424,423,1,0,
704,698,696,695,694,693,688,687,686,685,684,682,544,523,522,521,520,519,518,
  517,516,515,514,513,511,510,509,508,507,506,505,504,503,502,501,500,499,
  498,497,496,493,458,432,429,427,425,424,423,222,221,1,0,223,
459,1,0,51,52,
459,427,425,424,423,1,0,51,52,
459,1,0,51,52,
459,1,0,51,52,
459,1,0,51,52,
459,1,0,51,52,
459,1,0,51,52,
459,1,0,51,52,
459,1,0,51,52,
459,1,0,51,52,
459,1,0,51,52,
544,523,522,521,520,519,518,517,516,515,514,513,512,511,510,509,508,507,506,
  505,504,503,502,501,500,499,498,497,496,495,493,458,432,427,425,424,423,
  1,0,51,52,
544,523,522,521,520,519,518,517,516,515,514,513,512,511,510,509,508,507,506,
  505,504,503,502,501,500,499,498,497,496,495,493,458,432,427,425,424,423,
  1,0,51,52,
705,704,703,702,701,700,699,698,697,696,695,694,693,692,691,690,689,688,687,
  686,685,684,683,682,681,680,537,535,534,533,532,531,530,529,528,527,526,
  525,524,521,520,519,518,517,511,509,507,491,460,459,458,454,429,427,426,
  425,424,423,231,135,134,1,0,51,52,
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
704,702,701,700,699,698,697,696,695,694,693,692,691,688,687,686,685,684,683,
  682,537,535,534,533,532,531,530,529,528,527,521,518,517,491,459,429,231,
  135,134,1,0,51,52,
705,704,703,702,701,700,699,698,697,696,695,694,693,692,691,690,689,688,687,
  686,685,684,683,682,681,680,537,535,534,533,532,531,530,529,528,527,526,
  524,521,520,519,518,517,511,509,507,491,460,459,458,454,429,427,426,425,
  424,423,231,135,134,1,0,51,52,
704,702,701,700,699,698,697,696,695,694,693,692,691,688,687,686,685,684,683,
  682,537,535,534,533,532,531,530,529,528,527,521,518,517,491,459,429,231,
  135,134,1,0,51,52,
705,704,703,702,701,700,699,698,697,696,695,694,693,692,691,690,689,688,687,
  686,685,684,683,682,681,680,537,535,534,533,532,531,530,529,528,527,526,
  524,521,520,519,518,517,511,509,507,491,460,459,458,454,429,427,426,425,
  424,423,231,135,134,1,0,51,52,
704,702,701,700,699,698,697,696,695,694,693,692,691,688,687,686,685,684,683,
  682,537,535,534,533,532,531,530,529,528,527,526,525,524,521,518,517,491,
  459,429,231,135,134,1,0,51,52,
704,702,701,700,699,698,697,696,695,694,693,692,691,688,687,686,685,684,683,
  682,537,535,534,533,532,531,530,529,528,527,521,518,517,491,459,429,231,
  135,134,0,2,6,7,8,9,15,17,21,88,90,131,133,198,206,207,208,209,210,211,
  212,213,214,215,216,217,218,219,442,492,538,
704,702,701,700,699,698,697,696,695,694,693,692,691,688,687,686,685,684,683,
  682,537,535,534,533,532,531,530,529,528,527,521,518,517,491,459,429,231,
  135,134,0,2,6,7,8,9,15,17,21,88,90,131,133,198,206,207,208,209,210,211,
  212,213,214,215,216,217,218,219,442,492,538,
704,702,701,700,699,698,697,696,695,694,693,692,691,688,687,686,685,684,683,
  682,537,535,534,533,532,531,530,529,528,527,521,518,517,491,459,429,231,
  135,134,0,2,6,7,8,9,15,17,21,88,90,131,133,198,206,207,208,209,210,211,
  212,213,214,215,216,217,218,219,442,492,538,
704,702,701,700,699,698,697,696,695,694,693,692,691,688,687,686,685,684,683,
  682,537,535,534,533,532,531,530,529,528,527,521,518,517,491,459,429,231,
  135,134,0,2,6,7,8,9,15,17,21,88,90,131,133,198,206,207,208,209,210,211,
  212,213,214,215,216,217,218,219,442,492,538,
544,523,522,521,520,519,518,517,516,515,514,513,512,511,510,509,508,507,506,
  505,504,503,502,501,500,499,498,497,496,495,493,458,432,427,425,424,423,
  1,0,51,52,
544,523,522,521,520,519,518,517,516,515,514,513,512,511,510,509,508,507,506,
  505,504,503,502,501,500,499,498,497,496,495,493,458,432,425,423,1,0,51,
  52,197,199,200,201,202,
544,518,517,516,515,514,513,512,511,510,509,508,507,506,505,504,503,502,501,
  500,499,498,497,496,495,493,458,432,427,425,424,423,1,0,51,52,195,196,
544,516,515,514,513,512,511,510,509,508,507,506,505,504,503,502,501,500,499,
  498,497,496,495,493,458,432,427,425,424,423,1,0,51,52,191,192,193,194,
544,512,511,510,509,508,507,506,505,504,503,502,501,500,499,498,497,496,495,
  493,458,432,427,425,424,423,1,0,51,52,189,190,
544,510,509,508,507,506,505,504,503,502,501,500,499,498,497,496,495,493,458,
  432,427,425,424,423,1,0,51,52,187,188,
508,507,0,185,186,
506,505,504,503,502,501,500,499,498,497,496,495,432,427,425,424,423,1,0,59,
  167,168,169,170,171,172,173,174,175,176,177,178,179,180,181,182,183,184,
704,702,701,700,699,698,697,696,695,694,693,692,691,688,687,686,685,684,683,
  682,537,535,534,533,532,531,530,529,528,527,526,525,524,521,518,517,491,
  459,429,231,135,134,0,2,6,7,8,9,15,17,21,23,24,25,26,27,28,29,57,88,90,
  131,133,196,198,203,204,205,206,207,208,209,210,211,212,213,214,215,216,
  217,218,219,442,492,538,
427,425,424,423,1,0,51,52,
427,425,424,423,1,0,51,52,
427,425,424,423,1,0,75,
427,425,424,423,1,0,51,52,
427,425,424,423,1,0,51,52,
427,425,424,423,1,0,51,52,
427,425,424,423,1,0,51,52,
704,702,701,700,699,698,697,696,695,694,693,691,682,135,134,1,0,51,52,
704,702,701,700,699,698,697,696,695,694,693,691,682,135,134,1,0,51,52,
427,425,424,423,1,0,51,52,
427,425,424,423,1,0,51,52,
427,425,424,423,1,0,51,52,
427,425,424,423,1,0,51,52,
705,704,703,702,701,700,699,698,697,696,695,694,693,692,691,690,689,688,687,
  686,685,684,683,682,680,526,524,521,520,519,518,517,511,509,507,459,458,
  454,429,425,1,0,
427,425,424,423,1,0,51,52,
427,425,424,423,1,0,51,52,
427,425,424,423,1,0,51,52,
704,703,702,701,700,699,698,697,696,695,694,693,689,688,687,686,685,684,681,
  520,429,1,0,51,52,
704,703,702,701,700,699,698,697,696,695,694,693,689,688,687,686,685,684,681,
  520,429,0,10,12,13,14,19,435,436,437,
705,704,703,702,701,700,699,698,697,696,695,694,693,692,691,690,689,688,687,
  686,685,684,683,682,681,680,526,524,521,520,519,518,517,511,509,507,460,
  459,458,454,429,427,426,425,424,423,1,0,51,52,
705,704,703,702,701,700,699,698,697,696,695,694,693,692,691,690,689,688,687,
  686,685,684,683,682,681,680,537,535,534,533,532,531,530,529,528,527,526,
  525,524,521,520,519,518,517,511,509,507,491,460,459,458,454,429,427,426,
  425,424,423,231,135,134,1,0,51,52,
705,704,703,702,701,700,699,698,697,696,695,694,693,692,691,690,689,688,687,
  686,685,684,683,682,681,680,526,524,521,520,519,518,517,511,509,507,460,
  459,458,454,429,427,426,425,424,423,1,0,51,52,
705,704,703,702,701,700,699,698,697,696,695,694,693,692,691,690,689,688,687,
  686,685,684,683,682,681,680,537,535,534,533,532,531,530,529,528,527,526,
  525,524,521,520,519,518,517,511,509,507,491,460,459,458,454,429,427,426,
  425,424,423,231,135,134,1,0,51,52,
705,704,703,702,701,700,699,698,697,696,695,694,693,692,691,690,689,688,687,
  686,685,684,683,682,681,680,526,524,521,520,519,518,517,511,509,507,459,
  458,454,429,427,426,425,423,1,0,51,52,
705,704,703,702,701,700,699,698,697,696,695,694,693,692,691,690,689,688,687,
  686,685,684,683,682,681,680,526,524,521,520,519,518,517,511,509,507,460,
  459,458,454,429,427,426,425,424,423,1,0,51,52,
705,704,703,702,701,700,699,698,697,696,695,694,693,692,691,690,689,688,687,
  686,685,684,683,682,681,680,526,524,521,520,519,518,517,511,509,507,460,
  459,458,454,429,427,426,425,424,423,1,0,51,52,
705,704,703,702,701,700,699,698,697,696,695,694,693,692,691,690,689,688,687,
  686,685,684,683,682,681,680,526,524,521,520,519,518,517,511,509,507,460,
  459,458,454,429,427,426,425,424,423,1,0,51,52,
705,704,703,702,701,700,699,698,697,696,695,694,693,692,691,690,689,688,687,
  686,685,684,683,682,681,680,526,524,521,520,519,518,517,511,509,507,460,
  459,458,454,429,427,426,425,424,423,1,0,51,52,
705,704,703,702,701,700,699,698,697,696,695,694,693,692,691,690,689,688,687,
  686,685,684,683,682,681,680,526,524,521,520,519,518,517,511,509,507,460,
  459,458,454,429,427,426,425,424,423,1,0,51,52,
705,704,703,702,701,700,699,698,697,696,695,694,693,692,691,690,689,688,687,
  686,685,684,683,682,681,680,526,524,521,520,519,518,517,511,509,507,460,
  459,458,454,429,427,426,425,424,423,1,0,51,52,
705,704,703,702,701,700,699,698,697,696,695,694,693,692,691,690,689,688,687,
  686,685,684,683,682,681,680,526,524,521,520,519,518,517,511,509,507,460,
  459,458,454,429,427,426,425,424,423,1,0,51,52,
705,704,703,702,701,700,699,698,697,696,695,694,693,692,691,690,689,688,687,
  686,685,684,683,682,681,680,526,524,521,520,519,518,517,511,509,507,460,
  459,458,454,429,427,426,425,424,423,1,0,51,52,
705,704,703,702,701,700,699,698,697,696,695,694,693,692,691,690,689,688,687,
  686,685,684,683,682,681,680,526,524,521,520,519,518,517,511,509,507,460,
  459,458,454,429,427,426,425,424,423,1,0,51,52,
705,704,703,702,701,700,699,698,697,696,695,694,693,692,691,690,689,688,687,
  686,685,684,683,682,681,680,526,524,521,520,519,518,517,511,509,507,460,
  459,458,454,429,427,426,425,424,423,1,0,51,52,
705,704,703,702,701,700,699,698,697,696,695,694,693,692,691,690,689,688,687,
  686,685,684,683,682,681,680,526,524,521,520,519,518,517,511,509,507,460,
  459,458,454,429,427,426,425,424,423,1,0,51,52,
705,704,703,702,701,700,699,698,697,696,695,694,693,692,691,690,689,688,687,
  686,685,684,683,682,681,680,526,524,521,520,519,518,517,511,509,507,460,
  459,458,454,429,427,426,425,424,423,1,0,51,52,
705,704,703,702,701,700,699,698,697,696,695,694,693,692,691,690,689,688,687,
  686,685,684,683,682,681,680,526,524,521,520,519,518,517,511,509,507,460,
  459,458,454,429,427,426,425,424,423,1,0,51,52,
705,704,703,702,701,700,699,698,697,696,695,694,693,692,691,690,689,688,687,
  686,685,684,683,682,681,680,526,524,521,520,519,518,517,511,509,507,460,
  459,458,454,429,427,426,425,424,423,1,0,51,52,
705,704,703,702,701,700,699,698,697,696,695,694,693,692,691,690,689,688,687,
  686,685,684,683,682,681,680,526,524,521,520,519,518,517,511,509,507,460,
  459,458,454,429,427,426,425,424,423,1,0,51,52,
705,704,703,702,701,700,699,698,697,696,695,694,693,692,691,690,689,688,687,
  686,685,684,683,682,681,680,526,524,521,520,519,518,517,511,509,507,460,
  459,458,454,429,427,426,425,424,423,1,0,51,52,
705,704,703,702,701,700,699,698,697,696,695,694,693,692,691,690,689,688,687,
  686,685,684,683,682,681,680,526,524,521,520,519,518,517,511,509,507,460,
  459,458,454,429,427,426,425,424,423,1,0,51,52,
705,704,703,702,701,700,699,698,697,696,695,694,693,692,691,690,689,688,687,
  686,685,684,683,682,681,680,526,524,521,520,519,518,517,511,509,507,460,
  459,458,454,429,427,426,425,424,423,1,0,51,52,
705,704,703,702,701,700,699,698,697,696,695,694,693,692,691,690,689,688,687,
  686,685,684,683,682,681,680,526,524,521,520,519,518,517,511,509,507,460,
  459,458,454,429,427,426,425,424,423,1,0,51,52,
705,704,703,702,701,700,699,698,697,696,695,694,693,692,691,690,689,688,687,
  686,685,684,683,682,681,680,537,535,534,533,532,531,530,529,528,527,526,
  524,521,520,519,518,517,511,509,507,491,460,459,458,454,429,427,426,425,
  424,423,231,135,134,1,0,51,52,
705,704,703,702,701,700,699,698,697,696,695,694,693,692,691,690,689,688,687,
  686,685,684,683,682,681,680,537,535,534,533,532,531,530,529,528,527,526,
  525,524,521,520,519,518,517,511,509,507,491,460,459,458,454,429,427,426,
  425,424,423,231,135,134,1,0,51,52,
705,704,703,702,701,700,699,698,697,696,695,694,693,692,691,690,689,688,687,
  686,685,684,683,682,681,680,537,535,534,533,532,531,530,529,528,527,526,
  525,524,521,520,519,518,517,511,509,507,491,460,459,458,454,429,427,426,
  425,424,423,231,135,134,1,0,51,52,
705,704,703,702,701,700,699,698,697,696,695,694,693,692,691,690,689,688,687,
  686,685,684,683,682,681,680,537,535,534,533,532,531,530,529,528,527,526,
  525,524,521,520,519,518,517,511,509,507,491,460,459,458,454,429,427,426,
  425,424,423,231,135,134,1,0,51,52,
705,704,703,702,701,700,699,698,697,696,695,694,693,692,691,690,689,688,687,
  686,685,684,683,682,681,680,537,535,534,533,532,531,530,529,528,527,526,
  524,521,520,519,518,517,511,509,507,491,460,459,458,454,429,427,426,425,
  424,423,231,135,134,1,0,51,52,
705,704,703,702,701,700,699,698,697,696,695,694,693,692,691,690,689,688,687,
  686,685,684,683,682,681,680,544,526,524,523,522,521,520,519,518,517,516,
  515,514,513,512,511,510,509,508,507,506,505,504,503,502,501,500,499,498,
  497,496,495,493,460,459,458,454,432,429,427,426,425,424,423,1,0,51,52,
705,704,703,702,701,700,699,698,697,696,695,694,693,692,691,690,689,688,687,
  686,685,684,683,682,681,680,526,524,521,520,519,518,517,511,509,507,460,
  459,458,454,429,427,426,425,424,423,1,0,51,52,
705,704,703,702,701,700,699,698,697,696,695,694,693,692,691,690,689,688,687,
  686,685,684,683,682,681,680,537,535,534,533,532,531,530,529,528,527,526,
  525,524,521,520,519,518,517,511,509,507,491,460,459,458,454,429,427,426,
  425,424,423,231,135,134,1,0,51,52,
705,704,703,702,701,700,699,698,697,696,695,694,693,692,691,690,689,688,687,
  686,685,684,683,682,681,680,537,535,534,533,532,531,530,529,528,527,526,
  524,521,520,519,518,517,511,509,507,491,460,459,458,454,429,427,426,425,
  424,423,231,135,134,1,0,51,52,
705,704,703,702,701,700,699,698,697,696,695,694,693,692,691,690,689,688,687,
  686,685,684,683,682,681,680,526,524,521,520,519,518,517,511,509,507,460,
  459,458,454,429,427,426,425,424,423,1,0,51,52,
705,704,703,702,701,700,699,698,697,696,695,694,693,692,691,690,689,688,687,
  686,685,684,683,682,681,680,526,524,521,520,519,518,517,511,509,507,460,
  459,458,454,429,427,426,425,424,423,1,0,51,52,
705,704,703,702,701,700,699,698,697,696,695,694,693,692,691,690,689,688,687,
  686,685,684,683,682,681,680,526,524,521,520,519,518,517,511,509,507,460,
  459,458,454,429,427,426,425,424,423,1,0,51,52,
705,704,703,702,701,700,699,698,697,696,695,694,693,692,691,690,689,688,687,
  686,685,684,683,682,681,680,526,524,521,520,519,518,517,511,509,507,459,
  458,454,438,434,433,429,427,426,425,424,423,164,163,162,135,134,38,1,0,
  51,52,
705,704,703,702,701,700,699,698,697,696,695,694,693,692,691,690,689,688,687,
  686,685,684,683,682,681,680,526,524,521,520,519,518,517,511,509,507,459,
  458,454,429,425,1,0,46,54,85,90,92,140,141,143,145,146,147,148,149,150,
  154,157,158,185,187,189,195,196,197,199,200,203,205,220,224,225,229,413,
  414,415,416,417,418,419,420,421,422,
423,0,45,
423,0,45,
704,702,701,700,699,698,697,696,695,694,693,692,691,688,687,686,685,684,683,
  682,537,535,534,533,532,531,530,529,528,527,526,525,524,521,518,517,491,
  459,429,231,135,134,1,0,51,52,
704,702,701,700,699,698,697,696,695,694,693,692,691,688,687,686,685,684,683,
  682,537,535,534,533,532,531,530,529,528,527,526,525,524,521,518,517,491,
  459,429,231,135,134,0,2,6,7,8,9,15,17,21,23,24,25,26,27,28,29,57,88,90,
  131,133,196,198,203,204,205,206,207,208,209,210,211,212,213,214,215,216,
  217,218,219,442,492,538,
491,490,489,488,487,486,484,483,482,481,480,479,478,477,476,475,474,473,472,
  471,470,469,468,467,466,465,464,463,462,461,455,453,445,444,443,441,439,
  431,430,0,56,58,69,70,71,72,73,83,84,97,98,99,100,101,102,103,104,105,
  106,107,108,109,110,111,112,113,114,116,117,118,119,120,121,122,123,124,
  125,126,127,128,129,130,131,
427,425,424,423,1,0,51,52,
494,491,427,425,424,423,0,22,39,40,41,45,46,47,49,131,165,
668,543,542,541,248,246,238,236,234,0,245,247,249,250,254,255,256,
540,241,240,239,238,237,236,235,234,0,4,242,594,
668,543,248,246,238,236,234,0,5,245,247,249,608,
687,686,685,684,0,345,
454,0,85,
454,0,85,
454,0,85,
704,702,701,700,699,698,697,696,695,694,693,692,691,688,687,686,685,684,683,
  682,537,535,534,533,532,531,530,529,528,527,526,525,524,521,518,517,491,
  459,429,231,135,134,0,2,6,7,8,9,15,17,21,23,24,25,26,27,28,29,57,88,90,
  131,133,196,198,203,204,205,206,207,208,209,210,211,212,213,214,215,216,
  217,218,219,442,492,538,
704,702,701,700,699,698,697,696,695,694,693,692,691,688,687,686,685,684,683,
  682,537,535,534,533,532,531,530,529,528,527,526,525,524,521,518,517,491,
  459,429,231,135,134,0,2,6,7,8,9,15,17,21,23,24,25,26,27,28,29,57,88,90,
  131,133,196,198,203,204,205,206,207,208,209,210,211,212,213,214,215,216,
  217,218,219,442,492,538,
704,702,701,700,699,698,697,696,695,694,693,692,691,688,687,686,685,684,683,
  682,537,535,534,533,532,531,530,529,528,527,526,525,524,521,518,517,491,
  459,429,231,135,134,0,2,6,7,8,9,15,17,21,23,24,25,26,27,28,29,57,88,90,
  131,133,196,198,203,204,205,206,207,208,209,210,211,212,213,214,215,216,
  217,218,219,442,492,538,
704,702,701,700,699,698,697,696,695,694,693,692,691,688,687,686,685,684,683,
  682,537,535,534,533,532,531,530,529,528,527,526,525,524,521,518,517,491,
  459,429,231,135,134,0,2,6,7,8,9,15,17,21,23,24,25,26,27,28,29,57,88,90,
  131,133,196,198,203,204,205,206,207,208,209,210,211,212,213,214,215,216,
  217,218,219,442,492,538,
704,702,701,700,699,698,697,696,695,694,693,692,691,688,687,686,685,684,683,
  682,537,535,534,533,532,531,530,529,528,527,526,525,524,521,518,517,491,
  459,429,231,135,134,0,2,6,7,8,9,15,17,21,23,24,25,26,27,28,29,57,88,90,
  131,133,196,198,203,204,205,206,207,208,209,210,211,212,213,214,215,216,
  217,218,219,442,492,538,
704,702,701,700,699,698,697,696,695,694,693,692,691,688,687,686,685,684,683,
  682,537,535,534,533,532,531,530,529,528,527,526,525,524,521,518,517,491,
  459,429,231,135,134,0,2,6,7,8,9,15,17,21,23,24,25,26,27,28,29,57,88,90,
  131,133,196,198,203,204,205,206,207,208,209,210,211,212,213,214,215,216,
  217,218,219,442,492,538,
704,702,701,700,699,698,697,696,695,694,693,692,691,688,687,686,685,684,683,
  682,537,535,534,533,532,531,530,529,528,527,526,525,524,521,518,517,491,
  459,429,231,135,134,0,2,6,7,8,9,15,17,21,23,24,25,26,27,28,29,57,88,90,
  131,133,196,198,203,204,205,206,207,208,209,210,211,212,213,214,215,216,
  217,218,219,442,492,538,
704,702,701,700,699,698,697,696,695,694,693,692,691,688,687,686,685,684,683,
  682,537,535,534,533,532,531,530,529,528,527,526,525,524,521,518,517,491,
  459,429,231,135,134,0,2,6,7,8,9,15,17,21,23,24,25,26,27,28,29,57,88,90,
  131,133,196,198,203,204,205,206,207,208,209,210,211,212,213,214,215,216,
  217,218,219,442,492,538,
704,702,701,700,699,698,697,696,695,694,693,692,691,688,687,686,685,684,683,
  682,537,535,534,533,532,531,530,529,528,527,526,525,524,521,518,517,491,
  459,429,231,135,134,0,2,6,7,8,9,15,17,21,23,24,25,26,27,28,29,57,88,90,
  131,133,196,198,203,204,205,206,207,208,209,210,211,212,213,214,215,216,
  217,218,219,442,492,538,
704,702,701,700,699,698,697,696,695,694,693,692,691,688,687,686,685,684,683,
  682,537,535,534,533,532,531,530,529,528,527,526,525,524,521,518,517,491,
  459,429,231,135,134,0,2,6,7,8,9,15,17,21,23,24,25,26,27,28,29,57,88,90,
  131,133,196,198,203,204,205,206,207,208,209,210,211,212,213,214,215,216,
  217,218,219,442,492,538,
704,702,701,700,699,698,697,696,695,694,693,692,691,688,687,686,685,684,683,
  682,537,535,534,533,532,531,530,529,528,527,526,525,524,521,518,517,491,
  459,429,231,135,134,0,2,6,7,8,9,15,17,21,23,24,25,26,27,28,29,57,88,90,
  131,133,196,198,203,204,205,206,207,208,209,210,211,212,213,214,215,216,
  217,218,219,442,492,538,
704,702,701,700,699,698,697,696,695,694,693,692,691,688,687,686,685,684,683,
  682,537,535,534,533,532,531,530,529,528,527,526,525,524,521,518,517,491,
  459,429,231,135,134,0,2,6,7,8,9,15,17,21,23,24,25,26,27,28,29,57,88,90,
  131,133,196,198,203,204,205,206,207,208,209,210,211,212,213,214,215,216,
  217,218,219,442,492,538,
704,702,701,700,699,698,697,696,695,694,693,692,691,688,687,686,685,684,683,
  682,537,535,534,533,532,531,530,529,528,527,526,525,524,521,518,517,491,
  459,429,231,135,134,0,2,6,7,8,9,15,17,21,23,24,25,26,27,28,29,57,88,90,
  131,133,196,198,203,204,205,206,207,208,209,210,211,212,213,214,215,216,
  217,218,219,442,492,538,
704,702,701,700,699,698,697,696,695,694,693,692,691,688,687,686,685,684,683,
  682,537,535,534,533,532,531,530,529,528,527,526,525,524,521,518,517,491,
  459,429,231,135,134,0,2,6,7,8,9,15,17,21,23,24,25,26,27,28,29,57,88,90,
  131,133,196,198,203,204,205,206,207,208,209,210,211,212,213,214,215,216,
  217,218,219,442,492,538,
704,702,701,700,699,698,697,696,695,694,693,692,691,688,687,686,685,684,683,
  682,537,535,534,533,532,531,530,529,528,527,526,525,524,521,518,517,491,
  459,429,231,135,134,0,2,6,7,8,9,15,17,21,23,24,25,26,27,28,29,57,88,90,
  131,133,196,198,203,204,205,206,207,208,209,210,211,212,213,214,215,216,
  217,218,219,442,492,538,
704,702,701,700,699,698,697,696,695,694,693,692,691,688,687,686,685,684,683,
  682,537,535,534,533,532,531,530,529,528,527,526,525,524,521,518,517,491,
  459,429,231,135,134,0,2,6,7,8,9,15,17,21,23,24,25,26,27,28,29,57,88,90,
  131,133,196,198,203,204,205,206,207,208,209,210,211,212,213,214,215,216,
  217,218,219,442,492,538,
704,702,701,700,699,698,697,696,695,694,693,692,691,688,687,686,685,684,683,
  682,537,535,534,533,532,531,530,529,528,527,526,525,524,521,518,517,491,
  459,429,231,135,134,0,2,6,7,8,9,15,17,21,23,24,25,26,27,28,29,57,88,90,
  131,133,196,198,203,204,205,206,207,208,209,210,211,212,213,214,215,216,
  217,218,219,442,492,538,
704,702,701,700,699,698,697,696,695,694,693,692,691,688,687,686,685,684,683,
  682,537,535,534,533,532,531,530,529,528,527,526,525,524,521,518,517,491,
  459,429,231,135,134,0,2,6,7,8,9,15,17,21,23,24,25,26,27,28,29,57,88,90,
  131,133,196,198,203,204,205,206,207,208,209,210,211,212,213,214,215,216,
  217,218,219,442,492,538,
704,702,701,700,699,698,697,696,695,694,693,692,691,688,687,686,685,684,683,
  682,537,535,534,533,532,531,530,529,528,527,526,525,524,521,518,517,491,
  459,429,231,135,134,0,2,6,7,8,9,15,17,21,23,24,25,26,27,28,29,57,88,90,
  131,133,196,198,203,204,205,206,207,208,209,210,211,212,213,214,215,216,
  217,218,219,442,492,538,
704,702,701,700,699,698,697,696,695,694,693,692,691,688,687,686,685,684,683,
  682,537,535,534,533,532,531,530,529,528,527,526,525,524,521,518,517,491,
  459,429,231,135,134,0,2,6,7,8,9,15,17,21,23,24,25,26,27,28,29,57,88,90,
  131,133,196,198,203,204,205,206,207,208,209,210,211,212,213,214,215,216,
  217,218,219,442,492,538,
704,702,701,700,699,698,697,696,695,694,693,692,691,688,687,686,685,684,683,
  682,537,535,534,533,532,531,530,529,528,527,526,525,524,521,518,517,491,
  459,429,231,135,134,0,2,6,7,8,9,15,17,21,23,24,25,26,27,28,29,57,88,90,
  131,133,196,198,203,204,205,206,207,208,209,210,211,212,213,214,215,216,
  217,218,219,442,492,538,
704,702,701,700,699,698,697,696,695,694,693,692,691,688,687,686,685,684,683,
  682,537,535,534,533,532,531,530,529,528,527,526,525,524,521,518,517,491,
  459,429,231,135,134,0,2,6,7,8,9,15,17,21,23,24,25,26,27,28,29,57,88,90,
  131,133,196,198,203,204,205,206,207,208,209,210,211,212,213,214,215,216,
  217,218,219,442,492,538,
704,702,701,700,699,698,697,696,695,694,693,692,691,688,687,686,685,684,683,
  682,537,535,534,533,532,531,530,529,528,527,526,525,524,521,518,517,491,
  459,429,231,135,134,0,2,6,7,8,9,15,17,21,23,24,25,26,27,28,29,57,88,90,
  131,133,196,198,203,204,205,206,207,208,209,210,211,212,213,214,215,216,
  217,218,219,442,492,538,
704,702,701,700,699,698,697,696,695,694,693,692,691,688,687,686,685,684,683,
  682,537,535,534,533,532,531,530,529,528,527,526,525,524,521,518,517,491,
  459,429,231,135,134,0,2,6,7,8,9,15,17,21,23,24,25,26,27,28,29,57,88,90,
  131,133,196,198,203,204,205,206,207,208,209,210,211,212,213,214,215,216,
  217,218,219,442,492,538,
704,702,701,700,699,698,697,696,695,694,693,692,691,688,687,686,685,684,683,
  682,537,535,534,533,532,531,530,529,528,527,526,525,524,521,518,517,491,
  459,429,231,135,134,0,2,6,7,8,9,15,17,21,23,24,25,26,27,28,29,57,88,90,
  131,133,196,198,203,204,205,206,207,208,209,210,211,212,213,214,215,216,
  217,218,219,442,492,538,
704,702,701,700,699,698,697,696,695,694,693,692,691,688,687,686,685,684,683,
  682,537,535,534,533,532,531,530,529,528,527,526,525,524,521,518,517,491,
  459,429,231,135,134,0,2,6,7,8,9,15,17,21,23,24,25,26,27,28,29,57,88,90,
  131,133,196,198,203,204,205,206,207,208,209,210,211,212,213,214,215,216,
  217,218,219,442,492,538,
704,702,701,700,699,698,697,696,695,694,693,692,691,688,687,686,685,684,683,
  682,537,535,534,533,532,531,530,529,528,527,526,525,524,521,518,517,491,
  459,429,231,135,134,0,2,6,7,8,9,15,17,21,23,24,25,26,27,28,29,57,88,90,
  131,133,196,198,203,204,205,206,207,208,209,210,211,212,213,214,215,216,
  217,218,219,442,492,538,
704,702,701,700,699,698,697,696,695,694,693,692,691,688,687,686,685,684,683,
  682,537,535,534,533,532,531,530,529,528,527,526,525,524,521,518,517,491,
  459,429,231,135,134,0,2,6,7,8,9,15,17,21,23,24,25,26,27,28,29,57,88,90,
  131,133,196,198,203,204,205,206,207,208,209,210,211,212,213,214,215,216,
  217,218,219,442,492,538,
704,702,701,700,699,698,697,696,695,694,693,692,691,688,687,686,685,684,683,
  682,537,535,534,533,532,531,530,529,528,527,526,525,524,521,518,517,491,
  459,429,231,135,134,0,2,6,7,8,9,15,17,21,23,24,25,26,27,28,29,57,88,90,
  131,133,196,198,203,204,205,206,207,208,209,210,211,212,213,214,215,216,
  217,218,219,442,492,538,
704,702,701,700,699,698,697,696,695,694,693,692,691,688,687,686,685,684,683,
  682,537,535,534,533,532,531,530,529,528,527,526,525,524,521,518,517,491,
  459,429,231,135,134,0,2,6,7,8,9,15,17,21,23,24,25,26,27,28,29,57,88,90,
  131,133,196,198,203,204,205,206,207,208,209,210,211,212,213,214,215,216,
  217,218,219,442,492,538,
704,702,701,700,699,698,697,696,695,694,693,692,691,688,687,686,685,684,683,
  682,537,535,534,533,532,531,530,529,528,527,526,525,524,521,518,517,491,
  459,429,231,135,134,0,2,6,7,8,9,15,17,21,23,24,25,26,27,28,29,57,88,90,
  131,133,196,198,203,204,205,206,207,208,209,210,211,212,213,214,215,216,
  217,218,219,442,492,538,
704,702,701,700,699,698,697,696,695,694,693,692,691,688,687,686,685,684,683,
  682,537,535,534,533,532,531,530,529,528,527,526,525,524,521,518,517,491,
  459,429,231,135,134,0,2,6,7,8,9,15,17,21,23,24,25,26,27,28,29,57,88,90,
  131,133,196,198,203,204,205,206,207,208,209,210,211,212,213,214,215,216,
  217,218,219,442,492,538,
704,702,701,700,699,698,697,696,695,694,693,692,691,688,687,686,685,684,683,
  682,537,535,534,533,532,531,530,529,528,527,526,525,524,521,518,517,491,
  459,429,231,135,134,0,2,6,7,8,9,15,17,21,23,24,25,26,27,28,29,57,88,90,
  131,133,196,198,203,204,205,206,207,208,209,210,211,212,213,214,215,216,
  217,218,219,442,492,538,
704,702,701,700,699,698,697,696,695,694,693,692,691,688,687,686,685,684,683,
  682,537,535,534,533,532,531,530,529,528,527,526,525,524,521,518,517,491,
  459,429,231,135,134,0,2,6,7,8,9,15,17,21,23,24,25,26,27,28,29,57,88,90,
  131,133,196,198,203,204,205,206,207,208,209,210,211,212,213,214,215,216,
  217,218,219,442,492,538,
704,702,701,700,699,698,697,696,695,694,693,692,691,688,687,686,685,684,683,
  682,537,535,534,533,532,531,530,529,528,527,526,525,524,521,518,517,491,
  459,429,231,135,134,0,2,6,7,8,9,15,17,21,23,24,25,26,27,28,29,57,88,90,
  131,133,196,198,203,204,205,206,207,208,209,210,211,212,213,214,215,216,
  217,218,219,442,492,538,
704,702,701,700,699,698,697,696,695,694,693,692,691,688,687,686,685,684,683,
  682,537,535,534,533,532,531,530,529,528,527,526,525,524,521,518,517,491,
  459,429,231,135,134,0,2,6,7,8,9,15,17,21,23,24,25,26,27,28,29,57,88,90,
  131,133,196,198,203,204,205,206,207,208,209,210,211,212,213,214,215,216,
  217,218,219,442,492,538,
704,702,701,700,699,698,697,696,695,694,693,692,691,688,687,686,685,684,683,
  682,537,535,534,533,532,531,530,529,528,527,526,525,524,521,518,517,491,
  459,429,231,135,134,0,2,6,7,8,9,15,17,21,23,24,25,26,27,28,29,57,88,90,
  131,133,196,198,203,204,205,206,207,208,209,210,211,212,213,214,215,216,
  217,218,219,442,492,538,
704,702,701,700,699,698,697,696,695,694,693,692,691,688,687,686,685,684,683,
  682,537,535,534,533,532,531,530,529,528,527,526,525,524,521,518,517,491,
  459,429,231,135,134,0,2,6,7,8,9,15,17,21,23,24,25,26,27,28,29,57,88,90,
  131,133,196,198,203,204,205,206,207,208,209,210,211,212,213,214,215,216,
  217,218,219,442,492,538,
704,702,701,700,699,698,697,696,695,694,693,692,691,688,687,686,685,684,683,
  682,537,535,534,533,532,531,530,529,528,527,526,525,524,521,518,517,491,
  459,429,231,135,134,0,2,6,7,8,9,15,17,21,23,24,25,26,27,28,29,57,88,90,
  131,133,196,198,203,204,205,206,207,208,209,210,211,212,213,214,215,216,
  217,218,219,442,492,538,
704,702,701,700,699,698,697,696,695,694,693,692,691,688,687,686,685,684,683,
  682,537,535,534,533,532,531,530,529,528,527,526,525,524,521,518,517,491,
  459,429,231,135,134,0,2,6,7,8,9,15,17,21,23,24,25,26,27,28,29,57,88,90,
  131,133,196,198,203,204,205,206,207,208,209,210,211,212,213,214,215,216,
  217,218,219,442,492,538,
704,702,701,700,699,698,697,696,695,694,693,692,691,688,687,686,685,684,683,
  682,537,535,534,533,532,531,530,529,528,527,526,525,524,521,518,517,491,
  459,429,231,135,134,0,2,6,7,8,9,15,17,21,23,24,25,26,27,28,29,57,88,90,
  131,133,196,198,203,204,205,206,207,208,209,210,211,212,213,214,215,216,
  217,218,219,442,492,538,
704,702,701,700,699,698,697,696,695,694,693,692,691,688,687,686,685,684,683,
  682,537,535,534,533,532,531,530,529,528,527,526,525,524,521,518,517,491,
  459,429,231,135,134,0,2,6,7,8,9,15,17,21,23,24,25,26,27,28,29,57,88,90,
  131,133,196,198,203,204,205,206,207,208,209,210,211,212,213,214,215,216,
  217,218,219,442,492,538,
704,702,701,700,699,698,697,696,695,694,693,692,691,688,687,686,685,684,683,
  682,537,535,534,533,532,531,530,529,528,527,526,525,524,521,518,517,491,
  459,429,231,135,134,0,2,6,7,8,9,15,17,21,23,24,25,26,27,28,29,57,88,90,
  131,133,196,198,203,204,205,206,207,208,209,210,211,212,213,214,215,216,
  217,218,219,442,492,538,
704,702,701,700,699,698,697,696,695,694,693,692,691,688,687,686,685,684,683,
  682,537,535,534,533,532,531,530,529,528,527,526,525,524,521,518,517,491,
  459,429,231,135,134,0,2,6,7,8,9,15,17,21,23,24,25,26,27,28,29,57,88,90,
  131,133,196,198,203,204,205,206,207,208,209,210,211,212,213,214,215,216,
  217,218,219,442,492,538,
704,702,701,700,699,698,697,696,695,694,693,692,691,688,687,686,685,684,683,
  682,537,535,534,533,532,531,530,529,528,527,526,525,524,521,518,517,491,
  459,429,231,135,134,0,2,6,7,8,9,15,17,21,23,24,25,26,27,28,29,57,88,90,
  131,133,196,198,203,204,205,206,207,208,209,210,211,212,213,214,215,216,
  217,218,219,442,492,538,
704,702,701,700,699,698,697,696,695,694,693,692,691,688,687,686,685,684,683,
  682,537,535,534,533,532,531,530,529,528,527,526,525,524,521,518,517,491,
  459,429,231,135,134,0,2,6,7,8,9,15,17,21,23,24,25,26,27,28,29,57,88,90,
  131,133,196,198,203,204,205,206,207,208,209,210,211,212,213,214,215,216,
  217,218,219,442,492,538,
704,702,701,700,699,698,697,696,695,694,693,692,691,688,687,686,685,684,683,
  682,537,535,534,533,532,531,530,529,528,527,526,525,524,521,518,517,491,
  459,429,231,135,134,0,2,6,7,8,9,15,17,21,23,24,25,26,27,28,29,57,88,90,
  131,133,196,198,203,204,205,206,207,208,209,210,211,212,213,214,215,216,
  217,218,219,442,492,538,
704,702,701,700,699,698,697,696,695,694,693,692,691,688,687,686,685,684,683,
  682,537,535,534,533,532,531,530,529,528,527,526,525,524,521,518,517,491,
  459,429,231,135,134,0,2,6,7,8,9,15,17,21,23,24,25,26,27,28,29,57,88,90,
  131,133,196,198,203,204,205,206,207,208,209,210,211,212,213,214,215,216,
  217,218,219,442,492,538,
704,702,701,700,699,698,697,696,695,694,693,692,691,688,687,686,685,684,683,
  682,537,535,534,533,532,531,530,529,528,527,526,525,524,521,518,517,491,
  459,429,231,135,134,0,2,6,7,8,9,15,17,21,23,24,25,26,27,28,29,57,88,90,
  131,133,196,198,203,204,205,206,207,208,209,210,211,212,213,214,215,216,
  217,218,219,442,492,538,
704,702,701,700,699,698,697,696,695,694,693,692,691,688,687,686,685,684,683,
  682,537,535,534,533,532,531,530,529,528,527,526,525,524,521,518,517,491,
  459,429,231,135,134,0,2,6,7,8,9,15,17,21,23,24,25,26,27,28,29,57,88,90,
  131,133,196,198,203,204,205,206,207,208,209,210,211,212,213,214,215,216,
  217,218,219,442,492,538,
704,702,701,700,699,698,697,696,695,694,693,692,691,688,687,686,685,684,683,
  682,537,535,534,533,532,531,530,529,528,527,526,525,524,521,518,517,491,
  459,429,231,135,134,0,2,6,7,8,9,15,17,21,23,24,25,26,27,28,29,57,88,90,
  131,133,196,198,203,204,205,206,207,208,209,210,211,212,213,214,215,216,
  217,218,219,442,492,538,
704,702,701,700,699,698,697,696,695,694,693,692,691,688,687,686,685,684,683,
  682,537,535,534,533,532,531,530,529,528,527,526,525,524,521,518,517,491,
  459,429,231,135,134,0,2,6,7,8,9,15,17,21,23,24,25,26,27,28,29,57,88,90,
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
  491,459,429,231,135,134,0,2,6,7,8,9,10,12,15,17,18,20,21,23,24,25,26,27,
  28,29,57,88,90,91,131,133,196,198,203,204,205,206,207,208,209,210,211,
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
427,425,424,423,1,0,51,52,
427,425,424,423,1,0,51,52,
683,681,1,0,51,52,
683,681,0,10,12,18,20,435,485,
427,425,424,423,1,0,51,52,
427,425,424,423,1,0,51,52,
704,702,701,700,699,698,697,696,695,694,693,691,682,427,425,424,423,135,134,
  1,0,51,52,
704,702,701,700,699,698,697,696,695,694,693,691,682,427,425,424,423,135,134,
  1,0,21,133,442,
681,0,10,12,435,
681,0,10,12,435,
704,702,701,700,699,698,697,696,695,694,693,692,691,688,687,686,685,684,683,
  682,681,537,535,534,533,532,531,530,529,528,527,526,525,524,521,518,517,
  491,459,429,231,135,134,1,0,51,52,
704,702,701,700,699,698,697,696,695,694,693,692,691,688,687,686,685,684,683,
  682,681,537,535,534,533,532,531,530,529,528,527,526,525,524,521,518,517,
  491,459,429,231,135,134,1,0,51,52,
704,702,701,700,699,698,697,696,695,694,693,692,691,688,687,686,685,684,683,
  682,681,537,535,534,533,532,531,530,529,528,527,526,525,524,521,518,517,
  491,459,429,231,135,134,1,0,51,52,
704,702,701,700,699,698,697,696,695,694,693,692,691,688,687,686,685,684,683,
  682,681,537,535,534,533,532,531,530,529,528,527,526,525,524,521,518,517,
  491,459,429,231,135,134,0,2,6,7,8,9,10,12,15,17,21,23,24,25,26,27,28,29,
  57,88,90,131,133,196,198,203,204,205,206,207,208,209,210,211,212,213,
  214,215,216,217,218,219,435,442,492,538,
704,702,701,700,699,698,697,696,695,694,693,692,691,688,687,686,685,684,683,
  682,537,535,534,533,532,531,530,529,528,527,526,525,524,521,518,517,491,
  459,429,231,135,134,1,0,51,52,
704,702,701,700,699,698,697,696,695,694,693,692,691,688,687,686,685,684,683,
  682,537,535,534,533,532,531,530,529,528,527,526,525,524,521,518,517,491,
  459,429,231,135,134,0,2,6,7,8,9,15,17,21,23,24,25,26,27,28,29,57,88,90,
  131,133,196,198,203,204,205,206,207,208,209,210,211,212,213,214,215,216,
  217,218,219,442,492,538,
704,702,701,700,699,698,697,696,695,694,693,691,682,135,134,1,0,51,52,
704,702,701,700,699,698,697,696,695,694,693,691,682,135,134,0,21,133,442,
704,702,701,700,699,698,697,696,695,694,693,691,682,135,134,1,0,51,52,
704,702,701,700,699,698,697,696,695,694,693,691,682,135,134,0,21,133,442,
704,702,701,700,699,698,697,696,695,694,693,691,682,135,134,1,0,51,52,
704,702,701,700,699,698,697,696,695,694,693,691,682,135,134,0,21,115,133,
  442,
704,702,701,700,699,698,697,696,695,694,693,691,682,135,134,1,0,51,52,
704,702,701,700,699,698,697,696,695,694,693,691,682,135,134,0,21,115,133,
  442,
704,702,701,700,699,698,697,696,695,694,693,691,682,135,134,1,0,51,52,
704,702,701,700,699,698,697,696,695,694,693,691,682,135,134,0,21,115,133,
  442,
704,702,701,700,699,698,697,696,695,694,693,692,691,688,687,686,685,684,683,
  682,681,537,535,534,533,532,531,530,529,528,527,526,525,524,521,518,517,
  491,459,429,231,135,134,1,0,51,52,
704,702,701,700,699,698,697,696,695,694,693,692,691,688,687,686,685,684,683,
  682,681,537,535,534,533,532,531,530,529,528,527,526,525,524,521,518,517,
  491,459,429,231,135,134,1,0,51,52,
704,702,701,700,699,698,697,696,695,694,693,692,691,688,687,686,685,684,683,
  682,681,537,535,534,533,532,531,530,529,528,527,526,525,524,521,518,517,
  491,459,429,231,135,134,0,2,6,7,8,9,10,12,15,17,18,20,21,23,24,25,26,27,
  28,29,57,88,90,91,131,133,196,198,203,204,205,206,207,208,209,210,211,
  212,213,214,215,216,217,218,219,435,442,485,492,538,
704,702,701,700,699,698,697,696,695,694,693,692,691,688,687,686,685,684,683,
  682,681,537,535,534,533,532,531,530,529,528,527,526,525,524,521,518,517,
  491,459,429,231,135,134,1,0,51,52,
704,702,701,700,699,698,697,696,695,694,693,692,691,688,687,686,685,684,683,
  682,681,537,535,534,533,532,531,530,529,528,527,526,525,524,521,518,517,
  491,459,429,231,135,134,1,0,51,52,
704,702,701,700,699,698,697,696,695,694,693,692,691,688,687,686,685,684,683,
  682,681,537,535,534,533,532,531,530,529,528,527,526,525,524,521,518,517,
  491,459,429,231,135,134,1,0,51,52,
704,702,701,700,699,698,697,696,695,694,693,692,691,688,687,686,685,684,683,
  682,681,537,535,534,533,532,531,530,529,528,527,526,525,524,521,518,517,
  491,459,429,231,135,134,0,2,6,7,8,9,10,12,15,17,18,20,21,23,24,25,26,27,
  28,29,57,88,90,91,131,133,196,198,203,204,205,206,207,208,209,210,211,
  212,213,214,215,216,217,218,219,435,442,485,492,538,
704,702,701,700,699,698,697,696,695,694,693,692,691,688,687,686,685,684,683,
  682,537,535,534,533,532,531,530,529,528,527,526,525,524,521,518,517,491,
  459,429,231,135,134,1,0,51,52,
704,702,701,700,699,698,697,696,695,694,693,692,691,688,687,686,685,684,683,
  682,537,535,534,533,532,531,530,529,528,527,526,525,524,521,518,517,491,
  459,429,231,135,134,1,0,51,52,
704,702,701,700,699,698,697,696,695,694,693,692,691,688,687,686,685,684,683,
  682,537,535,534,533,532,531,530,529,528,527,526,525,524,521,518,517,491,
  459,429,231,135,134,0,2,6,7,8,9,15,17,21,23,24,25,26,27,28,29,57,88,90,
  131,133,196,198,203,204,205,206,207,208,209,210,211,212,213,214,215,216,
  217,218,219,442,492,538,
427,425,424,423,1,0,51,52,
704,702,701,700,699,698,697,696,695,694,693,692,691,688,687,686,685,684,683,
  682,537,535,534,533,532,531,530,529,528,527,526,525,524,521,518,517,491,
  459,429,231,135,134,1,0,51,52,
704,702,701,700,699,698,697,696,695,694,693,692,691,688,687,686,685,684,683,
  682,537,535,534,533,532,531,530,529,528,527,526,525,524,521,518,517,491,
  459,429,231,135,134,1,0,51,52,
704,702,701,700,699,698,697,696,695,694,693,692,691,688,687,686,685,684,683,
  682,537,535,534,533,532,531,530,529,528,527,526,525,524,521,518,517,491,
  459,429,231,135,134,1,0,51,52,
704,702,701,700,699,698,697,696,695,694,693,692,691,688,687,686,685,684,683,
  682,537,535,534,533,532,531,530,529,528,527,526,525,524,521,518,517,491,
  459,429,231,135,134,0,2,6,7,8,9,15,17,21,23,24,25,26,27,28,29,57,88,90,
  131,133,196,198,203,204,205,206,207,208,209,210,211,212,213,214,215,216,
  217,218,219,442,492,538,
704,702,701,700,699,698,697,696,695,694,693,691,682,135,134,0,21,133,442,
494,491,427,425,424,423,1,0,51,52,
494,491,427,425,424,423,1,0,51,52,
427,425,424,423,0,39,40,41,45,46,47,49,
705,704,702,701,700,699,698,697,696,695,694,693,692,691,690,689,688,687,686,
  685,684,683,682,681,680,526,524,521,520,519,518,517,511,509,507,459,458,
  454,429,425,0,46,54,85,89,90,92,140,141,145,146,147,148,149,150,154,157,
  158,185,187,189,195,196,197,199,200,203,205,220,224,225,229,413,414,415,
  416,417,418,419,420,421,422,
427,425,424,423,0,39,40,41,45,46,47,49,
703,702,701,700,699,696,694,686,685,684,682,681,423,0,155,
704,702,701,700,699,698,697,696,695,694,693,692,691,688,687,686,685,684,683,
  682,537,535,534,533,532,531,530,529,528,527,526,525,524,521,518,517,491,
  459,429,231,135,134,1,0,51,52,
427,425,424,423,0,39,40,41,45,46,47,49,
427,425,424,423,0,39,40,41,45,46,47,49,
427,425,424,423,0,39,40,41,45,46,47,49,
696,695,694,693,688,687,686,685,684,0,
703,701,700,699,684,683,0,
704,703,702,701,700,699,698,697,696,695,694,693,692,691,690,689,688,687,686,
  685,684,683,682,681,680,526,524,521,520,519,518,517,511,509,507,459,458,
  454,429,425,423,233,1,0,16,
698,696,695,694,693,688,687,686,685,684,544,523,522,521,520,519,518,517,516,
  515,514,513,512,511,510,509,508,507,506,505,504,503,502,501,500,499,498,
  497,496,495,493,458,432,427,425,424,423,1,0,
698,696,695,694,693,688,687,686,685,684,544,523,522,521,520,519,518,517,516,
  515,514,513,512,511,510,509,508,507,506,505,504,503,502,501,500,499,498,
  497,496,495,493,458,432,427,425,424,423,1,0,
459,0,90,
459,0,90,
459,0,90,
459,0,90,
459,0,90,
459,0,90,
459,0,90,
459,0,90,
459,0,90,
459,0,90,
459,0,90,
704,702,701,700,699,698,697,696,695,694,693,692,691,688,687,686,685,684,683,
  682,537,535,534,533,532,531,530,529,528,527,526,525,524,521,518,517,491,
  459,429,231,135,134,0,2,6,7,8,9,15,17,21,23,24,25,26,27,28,29,57,88,90,
  131,133,196,198,203,204,205,206,207,208,209,210,211,212,213,214,215,216,
  217,218,219,442,492,538,
688,687,686,685,684,0,
704,702,701,700,699,698,697,696,695,694,693,692,691,688,687,686,685,684,683,
  682,537,535,534,533,532,531,530,529,528,527,521,518,517,491,459,429,231,
  135,134,1,0,51,52,
704,702,701,700,699,698,697,696,695,694,693,692,691,688,687,686,685,684,683,
  682,537,535,534,533,532,531,530,529,528,527,521,518,517,491,459,429,231,
  135,134,1,0,51,52,
704,702,701,700,699,698,697,696,695,694,693,692,691,688,687,686,685,684,683,
  682,537,535,534,533,532,531,530,529,528,527,521,518,517,491,459,429,231,
  135,134,1,0,51,52,
704,702,701,700,699,698,697,696,695,694,693,692,691,688,687,686,685,684,683,
  682,537,535,534,533,532,531,530,529,528,527,521,518,517,491,459,429,231,
  135,134,1,0,51,52,
704,702,701,700,699,698,697,696,695,694,693,692,691,688,687,686,685,684,683,
  682,537,535,534,533,532,531,530,529,528,527,521,518,517,491,459,429,231,
  135,134,1,0,51,52,
704,702,701,700,699,698,697,696,695,694,693,692,691,688,687,686,685,684,683,
  682,537,535,534,533,532,531,530,529,528,527,521,518,517,491,459,429,231,
  135,134,1,0,51,52,
704,702,701,700,699,698,697,696,695,694,693,692,691,688,687,686,685,684,683,
  682,537,535,534,533,532,531,530,529,528,527,521,518,517,491,459,429,231,
  135,134,1,0,51,52,
704,702,701,700,699,698,697,696,695,694,693,692,691,688,687,686,685,684,683,
  682,537,535,534,533,532,531,530,529,528,527,526,525,524,521,518,517,491,
  459,429,231,135,134,1,0,51,52,
704,702,701,700,699,698,697,696,695,694,693,692,691,688,687,686,685,684,683,
  682,537,535,534,533,532,531,530,529,528,527,526,525,524,521,518,517,491,
  459,429,231,135,134,1,0,51,52,
704,702,701,700,699,698,697,696,695,694,693,692,691,688,687,686,685,684,683,
  682,537,535,534,533,532,531,530,529,528,527,526,525,524,521,518,517,491,
  459,429,231,135,134,1,0,51,52,
704,702,701,700,699,698,697,696,695,694,693,692,691,688,687,686,685,684,683,
  682,537,535,534,533,532,531,530,529,528,527,526,525,524,521,518,517,491,
  459,429,231,135,134,1,0,51,52,
704,702,701,700,699,698,697,696,695,694,693,692,691,688,687,686,685,684,683,
  682,537,535,534,533,532,531,530,529,528,527,526,525,524,521,518,517,491,
  459,429,231,135,134,1,0,51,52,
704,702,701,700,699,698,697,696,695,694,693,692,691,688,687,686,685,684,683,
  682,537,535,534,533,532,531,530,529,528,527,526,525,524,521,518,517,491,
  459,429,231,135,134,1,0,51,52,
704,702,701,700,699,698,697,696,695,694,693,692,691,688,687,686,685,684,683,
  682,537,535,534,533,532,531,530,529,528,527,526,525,524,521,518,517,491,
  459,429,231,135,134,1,0,51,52,
704,702,701,700,699,698,697,696,695,694,693,692,691,688,687,686,685,684,683,
  682,537,535,534,533,532,531,530,529,528,527,526,525,524,521,518,517,491,
  459,429,231,135,134,1,0,51,52,
704,702,701,700,699,698,697,696,695,694,693,692,691,688,687,686,685,684,683,
  682,537,535,534,533,532,531,530,529,528,527,526,525,524,521,518,517,491,
  459,429,231,135,134,1,0,51,52,
704,702,701,700,699,698,697,696,695,694,693,692,691,688,687,686,685,684,683,
  682,537,535,534,533,532,531,530,529,528,527,526,525,524,521,518,517,491,
  459,429,231,135,134,1,0,51,52,
704,702,701,700,699,698,697,696,695,694,693,692,691,688,687,686,685,684,683,
  682,537,535,534,533,532,531,530,529,528,527,526,525,524,521,518,517,491,
  459,429,231,135,134,1,0,51,52,
704,702,701,700,699,698,697,696,695,694,693,692,691,688,687,686,685,684,683,
  682,537,535,534,533,532,531,530,529,528,527,526,525,524,521,518,517,491,
  459,429,231,135,134,1,0,51,52,
704,702,701,700,699,698,697,696,695,694,693,692,691,688,687,686,685,684,683,
  682,537,535,534,533,532,531,530,529,528,527,526,525,524,521,518,517,491,
  459,429,231,135,134,1,0,51,52,
704,702,701,700,699,698,697,696,695,694,693,692,691,688,687,686,685,684,683,
  682,537,535,534,533,532,531,530,529,528,527,526,525,524,521,518,517,491,
  459,429,231,135,134,1,0,51,52,
704,702,701,700,699,698,697,696,695,694,693,692,691,688,687,686,685,684,683,
  682,537,535,534,533,532,531,530,529,528,527,526,525,524,521,518,517,491,
  459,429,231,135,134,1,0,51,52,
704,702,701,700,699,698,697,696,695,694,693,692,691,688,687,686,685,684,683,
  682,537,535,534,533,532,531,530,529,528,527,526,525,524,521,518,517,491,
  459,429,231,135,134,1,0,51,52,
704,702,701,700,699,698,697,696,695,694,693,692,691,688,687,686,685,684,683,
  682,537,535,534,533,532,531,530,529,528,527,526,525,524,521,518,517,491,
  459,429,231,135,134,1,0,51,52,
704,702,701,700,699,698,697,696,695,694,693,692,691,688,687,686,685,684,683,
  682,537,535,534,533,532,531,530,529,528,527,526,525,524,521,518,517,491,
  459,429,231,135,134,1,0,51,52,
704,702,701,700,699,698,697,696,695,694,693,692,691,688,687,686,685,684,683,
  682,537,535,534,533,532,531,530,529,528,527,526,525,524,521,518,517,491,
  459,429,231,135,134,1,0,51,52,
704,702,701,700,699,698,697,696,695,694,693,692,691,688,687,686,685,684,683,
  682,537,535,534,533,532,531,530,529,528,527,526,525,524,521,518,517,491,
  459,429,231,135,134,1,0,51,52,
704,702,701,700,699,698,697,696,695,694,693,692,691,688,687,686,685,684,683,
  682,537,535,534,533,532,531,530,529,528,527,526,525,524,521,518,517,491,
  459,429,231,135,134,1,0,51,52,
704,702,701,700,699,698,697,696,695,694,693,692,691,688,687,686,685,684,683,
  682,537,535,534,533,532,531,530,529,528,527,526,525,524,521,518,517,491,
  459,429,231,135,134,1,0,51,52,
704,702,701,700,699,698,697,696,695,694,693,692,691,688,687,686,685,684,683,
  682,537,535,534,533,532,531,530,529,528,527,526,525,524,521,518,517,491,
  459,429,231,135,134,1,0,51,52,
704,702,701,700,699,698,697,696,695,694,693,692,691,688,687,686,685,684,683,
  682,537,535,534,533,532,531,530,529,528,527,526,525,524,521,518,517,491,
  459,429,231,135,134,1,0,51,52,
704,702,701,700,699,698,697,696,695,694,693,692,691,688,687,686,685,684,683,
  682,537,535,534,533,532,531,530,529,528,527,526,525,524,521,518,517,491,
  459,429,231,135,134,1,0,51,52,
704,702,701,700,699,698,697,696,695,694,693,692,691,688,687,686,685,684,683,
  682,537,535,534,533,532,531,530,529,528,527,526,525,524,521,518,517,491,
  459,429,231,135,134,1,0,51,52,
704,702,701,700,699,698,697,696,695,694,693,692,691,688,687,686,685,684,683,
  682,537,535,534,533,532,531,530,529,528,527,526,525,524,521,518,517,491,
  459,429,231,135,134,1,0,51,52,
704,702,701,700,699,698,697,696,695,694,693,692,691,688,687,686,685,684,683,
  682,537,535,534,533,532,531,530,529,528,527,526,525,524,521,518,517,491,
  459,429,231,135,134,1,0,51,52,
704,702,701,700,699,698,697,696,695,694,693,692,691,688,687,686,685,684,683,
  682,537,535,534,533,532,531,530,529,528,527,526,525,524,521,518,517,491,
  459,429,231,135,134,1,0,51,52,
704,702,701,700,699,698,697,696,695,694,693,692,691,688,687,686,685,684,683,
  682,537,535,534,533,532,531,530,529,528,527,526,525,524,521,518,517,491,
  459,429,231,135,134,1,0,51,52,
704,702,701,700,699,698,697,696,695,694,693,692,691,688,687,686,685,684,683,
  682,537,535,534,533,532,531,530,529,528,527,526,525,524,521,518,517,491,
  459,429,231,135,134,1,0,51,52,
427,425,424,423,0,39,40,41,45,46,47,49,
427,425,424,423,0,39,40,41,45,46,47,49,
427,425,424,423,0,39,40,41,45,46,47,49,
427,425,424,423,0,39,40,41,45,46,47,49,
427,425,424,423,0,39,40,41,45,46,47,49,
704,702,701,700,699,698,697,696,695,694,693,691,682,135,134,0,21,133,442,
427,425,424,423,0,39,40,41,45,46,47,49,
427,425,424,423,0,39,40,41,45,46,47,49,
705,704,703,702,701,700,699,698,697,696,695,694,693,692,691,690,689,688,687,
  686,685,684,683,682,681,680,526,524,521,520,519,518,517,511,509,507,459,
  458,454,429,425,423,1,0,
427,425,424,423,0,39,40,41,45,46,47,49,
427,425,424,423,0,39,40,41,45,46,47,49,
704,703,702,701,700,699,698,697,696,695,694,693,688,687,686,685,684,520,429,
  425,423,1,0,51,52,
427,425,424,423,1,0,51,52,
427,425,424,423,1,0,51,52,
427,425,424,423,1,0,51,52,
427,425,424,423,1,0,51,52,
704,702,701,700,699,698,697,696,695,694,693,692,691,688,687,686,685,684,683,
  682,537,535,534,533,532,531,530,529,528,527,526,525,524,521,518,517,491,
  459,429,231,135,134,1,0,51,52,
704,702,701,700,699,698,697,696,695,694,693,692,691,688,687,686,685,684,683,
  682,537,535,534,533,532,531,530,529,528,527,526,525,524,521,518,517,491,
  459,429,231,135,134,0,2,6,7,8,9,15,17,21,23,24,25,26,27,28,29,57,88,90,
  131,133,196,198,203,204,205,206,207,208,209,210,211,212,213,214,215,216,
  217,218,219,442,492,538,
704,702,701,700,699,698,697,696,695,694,693,692,691,688,687,686,685,684,683,
  682,537,535,534,533,532,531,530,529,528,527,526,525,524,521,518,517,491,
  459,429,231,135,134,1,0,51,52,
704,702,701,700,699,698,697,696,695,694,693,692,691,688,687,686,685,684,683,
  682,537,535,534,533,532,531,530,529,528,527,526,525,524,521,518,517,491,
  459,429,231,135,134,0,2,6,7,8,9,15,17,21,23,24,25,26,27,28,29,57,88,90,
  131,133,196,198,203,204,205,206,207,208,209,210,211,212,213,214,215,216,
  217,218,219,442,492,538,
427,425,424,423,0,39,40,41,45,46,47,49,
427,425,424,423,1,0,51,52,
427,425,424,423,1,0,51,52,
544,454,427,425,424,423,1,0,51,52,
544,454,427,425,424,423,1,0,51,52,
544,454,427,425,424,423,1,0,51,52,
544,493,454,427,425,424,423,1,0,51,52,
544,454,427,425,424,423,1,0,51,52,
704,702,701,700,699,698,697,696,695,694,693,692,691,688,687,686,685,684,683,
  682,537,535,534,533,532,531,530,529,528,527,526,525,524,521,518,517,491,
  459,429,231,135,134,1,0,51,52,
704,702,701,700,699,698,697,696,695,694,693,692,691,688,687,686,685,684,683,
  682,537,535,534,533,532,531,530,529,528,527,526,525,524,521,518,517,491,
  459,429,231,135,134,1,0,51,52,
704,702,701,700,699,698,697,696,695,694,693,692,691,688,687,686,685,684,683,
  682,537,535,534,533,532,531,530,529,528,527,526,525,524,521,518,517,491,
  459,429,231,135,134,1,0,51,52,
544,427,425,424,423,1,0,51,52,
544,427,425,424,423,1,0,51,52,
493,1,0,51,52,
704,703,702,701,700,699,698,697,696,695,694,693,692,691,690,689,688,687,686,
  685,684,682,681,680,526,524,521,520,519,518,517,511,509,507,459,458,454,
  429,425,423,233,1,0,16,
704,703,702,701,700,699,698,697,696,695,694,693,692,691,690,689,688,687,686,
  685,684,683,682,681,680,526,524,521,520,519,518,517,511,509,507,459,458,
  454,429,425,423,233,1,0,16,
493,458,427,425,424,423,1,0,51,52,
493,458,0,92,132,
696,695,694,693,688,687,686,685,684,0,
704,698,696,695,694,693,688,687,686,685,684,682,427,425,424,423,222,221,1,0,
  223,
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
  685,684,682,681,680,526,524,521,520,519,518,517,511,509,507,459,458,454,
  429,425,423,233,1,0,16,
454,0,85,
704,702,701,700,699,698,697,696,695,694,693,692,691,688,687,686,685,684,683,
  682,681,537,535,534,533,532,531,530,529,528,527,526,525,524,521,518,517,
  491,459,429,231,135,134,0,2,6,7,8,9,10,12,15,17,18,20,21,23,24,25,26,27,
  28,29,57,88,90,91,131,133,196,198,203,204,205,206,207,208,209,210,211,
  212,213,214,215,216,217,218,219,435,442,485,492,538,
493,427,425,424,423,1,0,132,
493,427,425,424,423,1,0,132,
493,427,425,424,423,1,0,132,
493,427,425,424,423,1,0,132,
493,427,425,424,423,1,0,132,
705,704,702,701,700,699,698,697,696,695,694,693,692,691,690,689,688,687,686,
  685,684,683,682,681,680,526,524,521,520,519,518,517,511,509,507,460,459,
  458,454,429,425,423,1,0,46,51,52,54,85,90,92,95,140,141,145,146,147,148,
  149,150,154,157,158,185,187,189,195,196,197,199,200,203,205,220,224,225,
  229,413,414,415,416,417,418,419,420,421,422,
696,695,694,693,688,687,686,685,684,0,
688,687,686,685,684,0,
704,703,702,701,700,699,698,697,696,695,694,693,692,691,690,689,688,687,686,
  685,684,683,682,681,680,526,524,521,520,519,518,517,511,509,507,459,458,
  454,429,425,423,1,0,
704,702,701,700,699,698,697,696,695,694,693,692,691,688,687,686,685,684,683,
  682,537,535,534,533,532,531,530,529,528,527,526,525,524,521,518,517,491,
  459,429,231,135,134,0,2,6,7,8,9,15,17,21,23,24,25,26,27,28,29,57,88,90,
  131,133,196,198,203,204,205,206,207,208,209,210,211,212,213,214,215,216,
  217,218,219,442,492,538,
683,0,
704,702,701,700,699,698,697,696,695,694,693,691,682,135,134,1,0,51,52,
704,702,701,700,699,698,697,696,695,694,693,692,691,688,687,686,685,684,683,
  682,537,535,534,533,532,531,530,529,528,527,526,525,524,521,518,517,491,
  459,429,231,135,134,0,2,6,7,8,9,15,17,21,23,24,25,26,27,28,29,57,88,90,
  131,133,196,198,203,204,205,206,207,208,209,210,211,212,213,214,215,216,
  217,218,219,442,492,538,
704,702,701,700,699,698,697,696,695,694,693,692,691,688,687,686,685,684,683,
  682,537,535,534,533,532,531,530,529,528,527,526,525,524,521,518,517,491,
  459,429,231,135,134,0,2,6,7,8,9,15,17,21,23,24,25,26,27,28,29,57,88,90,
  131,133,196,198,203,204,205,206,207,208,209,210,211,212,213,214,215,216,
  217,218,219,442,492,538,
704,702,701,700,699,698,697,696,695,694,693,692,691,688,687,686,685,684,683,
  682,537,535,534,533,532,531,530,529,528,527,526,525,524,521,518,517,491,
  459,429,231,135,134,0,2,6,7,8,9,15,17,21,23,24,25,26,27,28,29,57,88,90,
  131,133,196,198,203,204,205,206,207,208,209,210,211,212,213,214,215,216,
  217,218,219,442,492,538,
704,702,701,700,699,698,697,696,695,694,693,692,691,688,687,686,685,684,683,
  682,537,535,534,533,532,531,530,529,528,527,526,525,524,521,518,517,491,
  459,429,231,135,134,0,2,6,7,8,9,15,17,21,23,24,25,26,27,28,29,57,88,90,
  131,133,196,198,203,204,205,206,207,208,209,210,211,212,213,214,215,216,
  217,218,219,442,492,538,
704,702,701,700,699,698,697,696,695,694,693,692,691,688,687,686,685,684,683,
  682,537,535,534,533,532,531,530,529,528,527,526,525,524,521,518,517,491,
  459,429,231,135,134,0,2,6,7,8,9,15,17,21,23,24,25,26,27,28,29,57,88,90,
  131,133,196,198,203,204,205,206,207,208,209,210,211,212,213,214,215,216,
  217,218,219,442,492,538,
704,702,701,700,699,698,697,696,695,694,693,692,691,688,687,686,685,684,683,
  682,537,535,534,533,532,531,530,529,528,527,526,525,524,521,518,517,491,
  459,429,231,135,134,0,2,6,7,8,9,15,17,21,23,24,25,26,27,28,29,57,88,90,
  131,133,196,198,203,204,205,206,207,208,209,210,211,212,213,214,215,216,
  217,218,219,442,492,538,
704,702,701,700,699,698,697,696,695,694,693,692,691,688,687,686,685,684,683,
  682,537,535,534,533,532,531,530,529,528,527,526,525,524,521,518,517,491,
  459,429,231,135,134,0,2,6,7,8,9,15,17,21,23,24,25,26,27,28,29,57,88,90,
  131,133,196,198,203,204,205,206,207,208,209,210,211,212,213,214,215,216,
  217,218,219,442,492,538,
704,702,701,700,699,698,697,696,695,694,693,692,691,688,687,686,685,684,683,
  682,537,535,534,533,532,531,530,529,528,527,526,525,524,521,518,517,491,
  459,429,231,135,134,0,2,6,7,8,9,15,17,21,23,24,25,26,27,28,29,57,88,90,
  131,133,196,198,203,204,205,206,207,208,209,210,211,212,213,214,215,216,
  217,218,219,442,492,538,
704,702,701,700,699,698,697,696,695,694,693,692,691,688,687,686,685,684,683,
  682,537,535,534,533,532,531,530,529,528,527,526,525,524,521,518,517,491,
  459,429,231,135,134,0,2,6,7,8,9,15,17,21,23,24,25,26,27,28,29,57,88,90,
  131,133,196,198,203,204,205,206,207,208,209,210,211,212,213,214,215,216,
  217,218,219,442,492,538,
704,702,701,700,699,698,697,696,695,694,693,692,691,688,687,686,685,684,683,
  682,537,535,534,533,532,531,530,529,528,527,526,525,524,521,518,517,491,
  459,429,231,135,134,0,2,6,7,8,9,15,17,21,23,24,25,26,27,28,29,57,88,90,
  131,133,196,198,203,204,205,206,207,208,209,210,211,212,213,214,215,216,
  217,218,219,442,492,538,
458,1,0,51,52,
704,702,701,700,699,698,697,696,695,694,693,692,691,688,687,686,685,684,683,
  682,537,535,534,533,532,531,530,529,528,527,521,518,517,491,459,429,231,
  135,134,0,2,6,7,8,9,15,17,21,88,90,131,133,198,206,207,208,209,210,211,
  212,213,214,215,216,217,218,219,442,492,538,
704,702,701,700,699,698,697,696,695,694,693,692,691,688,687,686,685,684,683,
  682,537,535,534,533,532,531,530,529,528,527,521,518,517,491,459,429,231,
  135,134,0,2,6,7,8,9,15,17,21,88,90,131,133,198,206,207,208,209,210,211,
  212,213,214,215,216,217,218,219,442,492,538,
704,702,701,700,699,698,697,696,695,694,693,692,691,688,687,686,685,684,683,
  682,537,535,534,533,532,531,530,529,528,527,521,518,517,491,459,429,231,
  135,134,0,2,6,7,8,9,15,17,21,88,90,131,133,198,206,207,208,209,210,211,
  212,213,214,215,216,217,218,219,442,492,538,
704,702,701,700,699,698,697,696,695,694,693,692,691,688,687,686,685,684,683,
  682,537,535,534,533,532,531,530,529,528,527,521,518,517,491,459,429,231,
  135,134,0,2,6,7,8,9,15,17,21,88,90,131,133,198,206,207,208,209,210,211,
  212,213,214,215,216,217,218,219,442,492,538,
704,702,701,700,699,698,697,696,695,694,693,692,691,688,687,686,685,684,683,
  682,537,535,534,533,532,531,530,529,528,527,521,518,517,491,459,429,231,
  135,134,0,2,6,7,8,9,15,17,21,88,90,131,133,198,206,207,208,209,210,211,
  212,213,214,215,216,217,218,219,442,492,538,
704,702,701,700,699,698,697,696,695,694,693,692,691,688,687,686,685,684,683,
  682,537,535,534,533,532,531,530,529,528,527,526,525,524,521,518,517,491,
  459,429,231,135,134,0,2,6,7,8,9,15,17,21,25,29,88,90,131,133,196,198,
  203,204,205,206,207,208,209,210,211,212,213,214,215,216,217,218,219,442,
  492,538,
704,702,701,700,699,698,697,696,695,694,693,692,691,688,687,686,685,684,683,
  682,537,535,534,533,532,531,530,529,528,527,526,525,524,521,518,517,491,
  459,429,231,135,134,0,2,6,7,8,9,15,17,21,25,29,88,90,131,133,196,198,
  203,204,205,206,207,208,209,210,211,212,213,214,215,216,217,218,219,442,
  492,538,
704,702,701,700,699,698,697,696,695,694,693,692,691,688,687,686,685,684,683,
  682,537,535,534,533,532,531,530,529,528,527,526,525,524,521,518,517,491,
  459,429,231,135,134,0,2,6,7,8,9,15,17,21,25,27,29,88,90,131,133,196,198,
  203,204,205,206,207,208,209,210,211,212,213,214,215,216,217,218,219,442,
  492,538,
704,702,701,700,699,698,697,696,695,694,693,692,691,688,687,686,685,684,683,
  682,537,535,534,533,532,531,530,529,528,527,526,525,524,521,518,517,491,
  459,429,231,135,134,0,2,6,7,8,9,15,17,21,25,27,29,88,90,131,133,196,198,
  203,204,205,206,207,208,209,210,211,212,213,214,215,216,217,218,219,442,
  492,538,
704,702,701,700,699,698,697,696,695,694,693,692,691,688,687,686,685,684,683,
  682,537,535,534,533,532,531,530,529,528,527,526,525,524,521,518,517,491,
  459,429,231,135,134,0,2,6,7,8,9,15,17,21,25,27,29,88,90,131,133,196,198,
  203,204,205,206,207,208,209,210,211,212,213,214,215,216,217,218,219,442,
  492,538,
704,702,701,700,699,698,697,696,695,694,693,692,691,688,687,686,685,684,683,
  682,537,535,534,533,532,531,530,529,528,527,526,525,524,521,518,517,491,
  459,429,231,135,134,0,2,6,7,8,9,15,17,21,25,27,29,88,90,131,133,196,198,
  203,204,205,206,207,208,209,210,211,212,213,214,215,216,217,218,219,442,
  492,538,
704,702,701,700,699,698,697,696,695,694,693,692,691,688,687,686,685,684,683,
  682,537,535,534,533,532,531,530,529,528,527,526,525,524,521,518,517,491,
  459,429,231,135,134,0,2,6,7,8,9,15,17,21,24,25,27,29,88,90,131,133,196,
  198,203,204,205,206,207,208,209,210,211,212,213,214,215,216,217,218,219,
  442,492,538,
704,702,701,700,699,698,697,696,695,694,693,692,691,688,687,686,685,684,683,
  682,537,535,534,533,532,531,530,529,528,527,526,525,524,521,518,517,491,
  459,429,231,135,134,0,2,6,7,8,9,15,17,21,24,25,27,29,88,90,131,133,196,
  198,203,204,205,206,207,208,209,210,211,212,213,214,215,216,217,218,219,
  442,492,538,
704,702,701,700,699,698,697,696,695,694,693,692,691,688,687,686,685,684,683,
  682,537,535,534,533,532,531,530,529,528,527,526,525,524,521,518,517,491,
  459,429,231,135,134,0,2,6,7,8,9,15,17,21,23,24,25,27,29,88,90,131,133,
  196,198,203,204,205,206,207,208,209,210,211,212,213,214,215,216,217,218,
  219,442,492,538,
704,702,701,700,699,698,697,696,695,694,693,692,691,688,687,686,685,684,683,
  682,537,535,534,533,532,531,530,529,528,527,526,525,524,521,518,517,491,
  459,429,231,135,134,0,2,6,7,8,9,15,17,21,23,24,25,27,29,88,90,131,133,
  196,198,203,204,205,206,207,208,209,210,211,212,213,214,215,216,217,218,
  219,442,492,538,
704,702,701,700,699,698,697,696,695,694,693,692,691,688,687,686,685,684,683,
  682,537,535,534,533,532,531,530,529,528,527,526,525,524,521,518,517,491,
  459,429,231,135,134,0,2,6,7,8,9,15,17,21,23,24,25,27,28,29,88,90,131,
  133,196,198,203,204,205,206,207,208,209,210,211,212,213,214,215,216,217,
  218,219,442,492,538,
704,702,701,700,699,698,697,696,695,694,693,692,691,688,687,686,685,684,683,
  682,537,535,534,533,532,531,530,529,528,527,526,525,524,521,518,517,491,
  459,429,231,135,134,0,2,6,7,8,9,15,17,21,23,24,25,27,28,29,88,90,131,
  133,196,198,203,204,205,206,207,208,209,210,211,212,213,214,215,216,217,
  218,219,442,492,538,
427,425,424,423,1,0,51,52,
427,425,424,423,0,39,40,41,45,46,47,49,
427,425,424,423,0,39,40,41,45,46,47,49,
427,425,424,423,0,39,40,41,45,46,47,49,
427,425,424,423,0,39,40,41,45,46,47,49,
427,425,424,423,1,0,51,52,
427,425,424,423,1,0,51,52,
427,425,424,423,0,39,40,41,45,46,47,49,
704,702,701,700,699,698,697,696,695,694,693,692,691,688,687,686,685,684,683,
  682,537,535,534,533,532,531,530,529,528,527,526,525,524,521,518,517,491,
  459,429,231,135,134,0,2,6,7,8,9,15,17,21,23,24,25,26,27,28,29,57,88,90,
  131,133,196,198,203,204,205,206,207,208,209,210,211,212,213,214,215,216,
  217,218,219,442,492,538,
704,702,701,700,699,698,697,696,695,694,693,692,691,688,687,686,685,684,683,
  682,537,535,534,533,532,531,530,529,528,527,526,525,524,521,518,517,491,
  459,429,231,135,134,0,2,6,7,8,9,15,17,21,23,24,25,26,27,28,29,57,88,90,
  131,133,196,198,203,204,205,206,207,208,209,210,211,212,213,214,215,216,
  217,218,219,442,492,538,
704,702,701,700,699,698,697,696,695,694,693,692,691,688,687,686,685,684,683,
  682,537,535,534,533,532,531,530,529,528,527,526,525,524,521,518,517,491,
  459,429,231,135,134,0,2,6,7,8,9,15,17,21,23,24,25,26,27,28,29,57,88,90,
  131,133,196,198,203,204,205,206,207,208,209,210,211,212,213,214,215,216,
  217,218,219,442,492,538,
493,0,132,
704,703,702,701,700,699,698,697,696,695,694,693,692,691,690,689,688,687,686,
  685,684,683,682,681,680,526,524,521,520,519,518,517,511,509,507,459,458,
  454,429,425,423,233,1,0,16,
704,702,701,700,699,698,697,696,695,694,693,692,691,688,687,686,685,684,683,
  682,681,540,537,535,534,533,532,531,530,529,528,527,526,525,524,521,518,
  517,491,459,429,241,240,239,238,237,236,235,234,231,135,134,1,0,51,52,
704,702,701,700,699,698,697,696,695,694,693,692,691,688,687,686,685,684,683,
  682,681,537,535,534,533,532,531,530,529,528,527,526,525,524,521,518,517,
  491,459,429,231,135,134,1,0,51,52,
544,0,267,
544,427,425,424,423,1,0,51,52,
704,703,702,701,700,699,698,697,696,695,694,693,692,691,690,689,688,687,686,
  685,684,682,681,680,526,524,521,520,519,518,517,511,509,507,459,458,454,
  429,425,423,233,1,0,16,
704,702,701,700,699,698,697,696,695,694,693,692,691,688,687,686,685,684,683,
  682,537,535,534,533,532,531,530,529,528,527,526,525,524,521,518,517,491,
  459,429,231,135,134,1,0,51,52,
493,427,425,424,423,1,0,132,
704,702,701,700,699,698,697,696,695,694,693,691,682,135,134,1,0,51,52,
705,704,702,701,700,699,698,697,696,695,694,693,692,691,690,689,688,687,686,
  685,684,683,682,681,680,526,524,521,520,519,518,517,511,509,507,460,459,
  458,454,429,427,425,424,423,1,0,51,52,
705,704,702,701,700,699,698,697,696,695,694,693,692,691,690,689,688,687,686,
  685,684,683,682,681,680,526,524,521,520,519,518,517,511,509,507,460,459,
  458,454,429,427,425,424,423,1,0,
427,425,424,423,0,39,40,41,45,46,47,49,
704,703,702,701,700,699,698,697,696,695,694,693,692,691,690,689,688,687,686,
  685,684,683,682,681,680,526,524,521,520,519,518,517,511,509,507,459,458,
  454,429,425,423,1,0,
688,687,686,685,684,0,
427,425,424,423,1,0,51,52,
704,702,701,700,699,698,697,696,695,694,693,691,682,135,134,0,3,133,442,536,
458,0,92,
458,0,92,
458,0,92,
458,0,92,
458,0,92,
458,0,92,
458,0,92,
458,0,92,
458,0,92,
458,0,92,
458,0,92,
544,523,522,521,520,519,518,517,516,515,514,513,512,511,510,509,508,507,506,
  505,504,503,502,501,500,499,498,497,496,495,493,458,432,425,423,1,0,197,
  199,200,201,202,
544,523,522,521,520,519,518,517,516,515,514,513,512,511,510,509,508,507,506,
  505,504,503,502,501,500,499,498,497,496,495,493,458,432,425,423,1,0,197,
  199,200,201,202,
544,518,517,516,515,514,513,512,511,510,509,508,507,506,505,504,503,502,501,
  500,499,498,497,496,495,493,458,432,427,425,424,423,1,0,195,196,
544,518,517,516,515,514,513,512,511,510,509,508,507,506,505,504,503,502,501,
  500,499,498,497,496,495,493,458,432,427,425,424,423,1,0,195,196,
544,518,517,516,515,514,513,512,511,510,509,508,507,506,505,504,503,502,501,
  500,499,498,497,496,495,493,458,432,427,425,424,423,1,0,195,196,
544,518,517,516,515,514,513,512,511,510,509,508,507,506,505,504,503,502,501,
  500,499,498,497,496,495,493,458,432,427,425,424,423,1,0,195,196,
544,516,515,514,513,512,511,510,509,508,507,506,505,504,503,502,501,500,499,
  498,497,496,495,493,458,432,427,425,424,423,1,0,191,192,193,194,
544,516,515,514,513,512,511,510,509,508,507,506,505,504,503,502,501,500,499,
  498,497,496,495,493,458,432,427,425,424,423,1,0,191,192,193,194,
544,512,511,510,509,508,507,506,505,504,503,502,501,500,499,498,497,496,495,
  493,458,432,427,425,424,423,1,0,189,190,
544,512,511,510,509,508,507,506,505,504,503,502,501,500,499,498,497,496,495,
  493,458,432,427,425,424,423,1,0,189,190,
544,510,509,508,507,506,505,504,503,502,501,500,499,498,497,496,495,493,458,
  432,427,425,424,423,1,0,187,188,
544,510,509,508,507,506,505,504,503,502,501,500,499,498,497,496,495,493,458,
  432,427,425,424,423,1,0,187,188,
427,425,424,423,0,39,40,41,45,46,47,49,
427,425,424,423,0,39,40,41,45,46,47,49,
427,425,424,423,0,39,40,41,45,46,47,49,
540,241,240,239,238,237,236,235,234,1,0,51,52,
704,703,702,701,700,699,698,697,696,695,694,693,692,691,690,689,688,687,686,
  685,684,683,682,681,680,526,524,521,520,519,518,517,511,509,507,459,458,
  454,429,425,423,233,1,0,
704,702,701,700,699,698,697,696,695,694,693,692,691,688,687,686,685,684,683,
  682,681,537,535,534,533,532,531,530,529,528,527,526,525,524,521,518,517,
  491,459,429,231,135,134,0,2,6,7,8,9,10,12,15,17,18,20,21,23,24,25,26,27,
  28,29,57,88,90,131,133,196,198,203,204,205,206,207,208,209,210,211,212,
  213,214,215,216,217,218,219,435,442,485,492,538,
704,702,701,700,699,698,697,696,695,694,693,692,691,688,687,686,685,684,683,
  682,537,535,534,533,532,531,530,529,528,527,526,525,524,521,518,517,491,
  459,429,231,135,134,0,2,6,7,8,9,15,17,21,23,24,25,26,27,28,29,57,88,90,
  131,133,196,198,203,204,205,206,207,208,209,210,211,212,213,214,215,216,
  217,218,219,442,492,538,
704,702,701,700,699,698,697,696,695,694,693,691,682,135,134,0,21,133,442,
427,425,424,423,0,39,40,41,45,46,47,49,
704,702,701,700,699,698,697,696,695,694,693,691,688,687,686,685,684,682,458,
  429,1,0,51,52,138,
458,1,0,51,52,
458,1,0,51,52,
540,241,240,239,238,237,236,235,234,0,4,242,594,
458,0,92,

};


static unsigned const char ag_astt[27766] = {
  8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,7,1,1,9,5,2,2,2,2,2,
  2,2,2,2,2,2,2,2,1,8,8,8,8,8,2,2,2,2,2,1,1,7,1,0,1,1,1,1,1,1,1,1,1,1,1,5,5,
  5,5,5,5,5,5,5,5,5,5,5,1,7,1,3,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,4,4,2,4,
  4,4,4,2,4,4,7,2,1,1,1,1,1,1,1,1,1,1,1,1,1,7,1,3,1,1,1,1,1,1,1,1,1,1,1,1,5,
  5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,
  5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,
  5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,
  5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,
  5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,
  5,5,5,5,5,5,5,5,5,5,1,7,1,3,1,7,2,8,8,1,1,1,1,1,7,3,3,1,3,1,1,1,1,1,8,8,8,
  8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,
  8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,
  8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,
  8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,
  8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,5,
  5,8,8,8,8,8,8,8,1,7,1,1,2,2,2,2,2,2,2,2,2,2,2,2,2,1,1,1,1,1,1,1,1,1,1,1,1,
  1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,
  1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,
  1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,
  1,1,1,1,1,1,1,1,1,1,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,
  8,8,8,8,8,1,1,1,8,8,8,8,8,5,5,1,5,5,5,5,2,2,9,7,1,1,1,1,1,1,1,2,1,2,2,1,1,
  1,1,1,1,1,1,1,1,1,1,1,1,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,
  2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,1,1,1,1,1,1,1,1,1,1,1,1,1,1,
  1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,
  1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,
  1,1,1,1,1,1,1,1,1,2,2,2,2,2,2,2,2,2,2,2,2,2,1,8,8,8,8,8,2,2,2,2,2,1,3,1,7,
  1,3,3,1,1,3,1,1,1,1,1,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,1,7,1,3,5,5,5,5,5,5,5,
  5,5,5,5,5,5,5,5,1,7,1,3,5,1,7,1,3,5,1,7,1,3,5,1,7,1,3,5,5,5,5,5,7,1,3,5,5,
  5,5,5,7,1,3,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,1,7,1,3,5,5,5,5,5,5,5,5,5,5,5,5,
  5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,1,7,1,3,5,5,5,
  5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,
  5,5,1,7,1,3,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,1,7,1,3,5,5,5,5,5,1,7,1,3,5,5,5,
  5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,1,7,1,3,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,
  7,1,1,1,1,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,7,1,1,1,2,7,1,1,1,2,7,1,1,1,2,7,1,
  1,1,8,8,8,8,1,7,1,1,8,8,8,8,1,7,1,1,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,7,1,1,1,
  2,2,2,2,2,2,2,2,2,2,2,1,2,2,2,2,2,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,
  1,1,2,2,2,7,2,1,1,2,1,2,1,1,1,1,1,1,1,1,1,1,1,2,1,1,1,1,1,1,1,1,1,2,1,1,1,
  1,1,1,1,1,1,1,1,1,1,1,1,1,2,2,2,2,2,2,2,2,2,2,2,1,2,2,2,2,2,1,1,1,1,1,1,1,
  1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,2,2,2,7,2,1,1,2,1,2,1,1,1,1,1,1,1,1,1,1,1,2,
  1,1,1,1,1,1,1,1,1,2,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,2,2,2,2,2,2,2,2,2,2,2,
  2,2,2,2,7,1,1,1,1,1,1,1,1,7,1,1,1,1,1,2,2,7,1,1,1,1,1,1,5,5,5,5,5,5,5,5,5,
  5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,
  5,5,5,5,5,5,5,5,5,7,1,3,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,
  5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,
  5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,7,1,3,1,1,7,1,1,1,5,5,
  5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,
  5,5,5,5,5,5,7,1,3,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,
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
  8,8,1,8,8,1,1,1,1,1,2,2,7,1,1,1,1,1,3,3,1,3,1,1,1,1,1,1,2,1,2,2,1,1,1,1,1,
  1,1,1,1,1,1,1,1,1,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,
  2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,
  1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,
  1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,
  1,1,1,1,1,1,8,8,8,8,8,8,1,7,1,1,5,5,5,5,5,5,7,1,3,5,5,5,5,5,5,7,1,3,5,5,5,
  5,5,5,7,1,3,5,5,5,5,5,5,7,1,3,5,5,5,5,5,5,7,1,3,5,5,5,5,5,5,7,1,3,5,5,5,5,
  5,5,7,1,3,5,5,5,5,5,5,7,1,3,5,5,5,5,5,5,7,1,3,5,5,5,5,5,5,7,1,3,5,5,5,5,5,
  5,7,1,3,5,5,5,5,5,5,7,1,3,5,5,5,5,5,5,7,1,3,5,5,5,5,5,5,7,1,3,5,5,5,5,5,5,
  5,5,5,5,7,1,3,5,5,5,5,5,5,5,5,5,5,7,1,3,5,5,5,5,5,5,5,5,7,1,3,5,5,5,5,5,5,
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
  5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,7,1,3,5,5,5,5,5,5,7,1,3,5,
  5,5,5,5,5,7,1,3,5,5,5,5,5,5,7,1,3,5,5,5,5,5,5,7,1,3,5,5,5,5,5,5,7,1,3,5,5,
  5,5,5,5,7,1,3,5,5,5,5,5,5,7,1,3,5,5,5,5,5,5,7,1,3,5,5,5,5,5,5,7,1,3,5,5,5,
  5,5,5,7,1,3,5,5,5,5,5,5,7,1,3,5,5,5,5,5,5,7,1,3,5,5,5,5,5,5,7,1,3,5,5,5,5,
  5,5,7,1,3,5,5,5,5,5,5,7,1,3,5,5,5,5,5,5,7,1,3,5,5,5,5,5,5,7,1,3,5,5,5,5,5,
  5,7,1,3,5,5,5,5,5,5,7,1,3,5,5,5,5,5,5,7,1,3,5,5,5,5,5,5,7,1,3,5,5,5,5,5,5,
  7,1,3,5,5,5,5,5,5,7,1,3,5,5,5,5,5,5,7,1,3,5,5,5,5,5,5,7,1,3,5,5,5,5,5,5,7,
  1,3,5,5,5,5,5,5,7,1,3,5,5,5,5,5,5,7,1,3,5,5,5,5,5,5,7,1,3,5,5,5,5,5,7,1,3,
  8,8,8,8,8,8,8,8,8,1,7,1,1,8,8,8,8,8,8,8,8,8,1,7,1,1,8,8,8,8,8,8,8,1,7,1,1,
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
  5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,7,1,3,5,5,5,5,5,5,5,5,5,7,1,3,
  5,5,5,5,5,5,5,5,5,7,1,3,5,5,5,5,5,5,5,5,5,7,1,3,5,5,5,5,5,5,5,5,5,7,1,3,5,
  5,5,5,5,7,1,3,5,5,5,5,5,7,1,3,5,5,5,5,5,5,5,5,7,1,3,5,5,5,5,5,5,5,5,7,1,3,
  5,5,5,5,5,5,5,5,7,1,3,5,5,5,5,5,5,5,5,5,5,7,1,3,5,5,5,5,5,5,5,5,5,5,7,1,3,
  5,5,5,5,5,5,5,5,5,5,7,1,3,5,5,5,5,5,5,5,5,5,5,7,1,3,5,5,5,5,5,5,5,5,5,5,7,
  1,3,5,5,5,5,5,5,5,5,5,5,7,1,3,5,5,5,5,5,5,5,5,5,5,7,1,3,5,5,5,5,5,5,5,5,5,
  5,7,1,3,5,5,5,5,5,5,5,5,5,5,7,1,3,5,5,5,5,5,5,5,5,5,5,7,1,3,5,5,5,5,5,5,5,
  5,5,5,7,1,3,5,5,5,5,5,5,5,5,5,5,7,1,3,5,5,5,5,5,5,5,5,5,5,7,1,3,8,8,8,8,1,
  7,1,1,8,1,7,1,1,8,1,7,1,1,8,1,7,1,1,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,
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
  8,1,7,1,1,8,8,8,8,8,8,8,8,1,7,1,1,8,8,8,8,8,8,8,8,1,7,1,1,8,8,8,8,8,8,8,8,
  1,7,1,1,8,8,8,8,8,8,8,8,1,7,1,1,8,8,8,8,1,7,1,1,8,8,8,8,1,7,1,1,8,8,8,8,8,
  8,8,1,7,1,1,8,8,8,8,8,8,8,1,7,1,1,8,8,8,8,8,8,8,1,7,1,1,8,8,8,8,8,8,8,8,8,
  1,7,1,1,8,8,8,8,8,8,8,8,8,1,7,1,1,8,8,8,8,8,8,8,8,8,1,7,1,1,8,8,8,8,8,8,8,
  8,8,1,7,1,1,8,8,8,8,8,8,8,8,8,1,7,1,1,8,8,8,8,8,8,8,8,8,1,7,1,1,8,8,8,8,8,
  8,8,8,8,1,7,1,1,8,8,8,8,8,8,8,8,8,1,7,1,1,8,8,8,8,8,8,8,8,8,1,7,1,1,8,8,8,
  8,8,8,8,8,8,1,7,1,1,8,8,8,8,8,8,8,8,8,1,7,1,1,8,8,8,8,8,8,8,8,8,1,7,1,1,8,
  8,8,8,8,8,8,8,8,1,7,1,1,5,2,2,2,2,2,2,2,2,2,2,2,5,2,5,5,2,2,2,2,2,5,2,5,5,
  5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,2,5,5,5,5,1,7,1,3,2,5,5,
  5,5,5,1,7,1,3,4,4,4,4,4,4,4,4,4,4,4,4,4,4,4,4,4,4,4,4,4,4,4,4,4,4,4,4,4,4,
  4,4,4,4,4,4,4,4,4,4,4,4,4,4,4,4,4,4,4,4,4,4,4,4,4,4,4,4,1,4,4,4,4,4,4,4,4,
  4,7,1,4,4,4,4,1,7,1,1,5,1,2,2,2,2,2,2,2,2,2,2,2,1,2,2,2,2,2,1,1,1,1,1,1,1,
  1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,2,2,2,7,2,1,1,2,1,2,1,1,1,1,1,1,1,1,1,1,2,2,
  1,1,1,1,1,1,1,1,1,2,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,
  1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,7,2,2,1,2,2,1,1,2,2,1,
  1,1,1,2,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,2,2,1,2,2,1,1,2,8,8,8,8,1,7,
  1,1,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,
  8,8,8,8,8,1,7,1,1,8,8,8,8,1,7,1,1,2,1,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,
  2,2,2,3,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,7,2,5,5,5,5,5,5,5,5,7,1,3,1,7,
  1,8,8,8,8,1,7,1,1,8,8,8,8,1,7,1,1,1,1,1,1,7,2,2,1,2,1,1,1,1,1,1,1,7,2,2,1,
  2,1,1,1,8,8,8,8,1,7,1,1,2,2,2,2,2,7,2,2,2,2,7,2,2,7,4,4,4,4,4,4,4,2,2,2,2,
  4,2,2,2,2,2,4,4,4,4,4,4,4,4,4,4,4,4,4,4,4,4,4,4,4,4,4,4,4,4,4,4,4,4,4,4,4,
  4,4,4,4,4,4,4,4,4,4,7,4,1,4,4,4,4,4,4,4,4,4,4,4,4,4,4,4,4,4,4,4,4,4,4,4,4,
  4,4,4,4,4,4,4,4,4,4,4,4,4,4,4,4,4,4,4,4,4,4,4,4,4,4,4,4,7,2,2,2,2,2,7,2,1,
  2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,
  2,2,2,7,1,10,10,10,10,10,5,2,10,10,10,10,10,10,10,10,10,7,10,10,10,10,4,4,
  4,4,4,4,4,4,4,4,4,4,4,4,4,4,4,4,4,4,4,4,4,4,4,4,4,4,4,4,4,4,4,4,4,4,4,4,7,
  10,10,4,4,4,4,4,4,4,4,4,4,4,4,4,4,4,4,4,4,4,4,4,4,4,4,4,4,4,4,4,4,4,4,4,4,
  4,4,4,4,7,10,10,10,10,10,10,10,10,10,4,4,4,4,4,4,4,4,4,4,4,4,4,4,4,4,4,4,4,
  4,4,4,4,4,4,4,4,4,4,4,4,4,4,4,4,4,4,4,7,9,2,2,1,1,2,10,10,10,10,10,9,4,4,4,
  4,4,4,4,4,4,4,4,4,4,4,4,4,4,4,4,4,4,4,4,4,4,4,4,4,4,4,4,2,4,4,4,4,2,2,4,7,
  2,5,5,7,1,3,5,5,5,5,5,5,7,1,3,5,5,7,1,3,5,5,7,1,3,5,5,7,1,3,5,5,7,1,3,5,5,
  7,1,3,5,5,7,1,3,5,5,7,1,3,5,5,7,1,3,5,5,7,1,3,5,5,5,5,5,5,5,5,5,5,5,5,5,5,
  5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,7,1,3,5,5,5,5,5,5,5,5,5,5,
  5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,1,7,1,3,5,5,5,5,5,5,
  5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,
  5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,7,1,3,8,1,7,1,1,8,1,7,1,1,8,1,7,1,1,
  8,1,7,1,1,8,1,7,1,1,8,1,7,1,1,8,1,7,1,1,8,1,7,1,1,8,1,7,1,1,8,1,7,1,1,8,1,
  7,1,1,5,5,5,5,5,5,5,5,5,5,5,5,5,2,2,2,2,2,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,
  5,5,5,5,5,1,7,1,3,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,
  5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,7,1,3,5,
  5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,
  5,1,7,1,3,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,
  5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,7,1,3,8,8,8,8,8,
  8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,
  1,7,1,1,2,2,2,2,2,2,2,2,2,2,2,1,2,2,2,2,2,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,
  1,1,1,2,2,2,7,2,1,1,2,1,2,1,1,2,1,1,1,2,2,2,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,
  2,2,2,2,2,2,2,2,2,2,2,1,2,2,2,2,2,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,2,
  2,2,7,2,1,1,2,1,2,1,1,2,1,1,1,2,2,2,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,2,2,2,2,
  2,2,2,2,2,2,2,1,2,2,2,2,2,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,2,2,2,7,2,
  1,1,2,1,2,1,1,2,1,1,1,2,2,2,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,2,2,2,2,2,2,2,2,
  2,2,2,1,2,2,2,2,2,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,2,2,2,7,2,1,1,2,1,
  2,1,1,2,1,1,1,2,2,2,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,5,5,5,5,5,5,5,5,5,5,5,5,
  5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,1,7,1,3,5,1,1,1,1,1,5,5,
  5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,1,7,1,3,1,1,1,1,1,5,
  1,1,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,1,7,1,3,1,1,
  5,1,1,1,1,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,1,7,1,3,1,1,1,
  1,5,1,1,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,1,7,1,3,1,1,5,1,1,5,
  5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,1,7,1,3,1,1,1,1,5,1,1,1,1,1,1,1,1,
  1,1,1,1,1,1,1,4,4,4,4,4,7,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,
  2,2,2,2,2,2,1,2,2,2,2,2,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,2,2,2,
  7,2,1,1,2,1,2,1,1,1,1,1,1,1,1,1,2,2,1,1,1,1,1,1,1,1,2,1,1,1,1,1,1,1,1,1,1,
  1,1,1,1,1,1,8,8,8,8,1,7,1,1,8,8,8,8,1,7,1,1,8,8,8,8,1,7,1,5,5,5,5,5,7,1,3,
  8,8,8,8,1,7,1,1,5,5,5,5,5,7,1,3,8,8,8,8,1,7,1,1,5,5,5,5,5,5,5,5,5,5,5,5,5,
  5,5,5,7,1,3,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,1,7,1,1,5,5,5,5,5,7,1,3,8,8,8,8,
  1,7,1,1,5,5,5,5,5,7,1,3,8,8,8,8,1,7,1,1,10,10,1,10,10,10,10,10,10,10,10,10,
  10,10,10,2,10,10,10,10,10,10,10,10,10,10,10,10,10,10,10,10,10,10,10,10,10,
  10,10,10,10,7,5,5,5,5,5,7,1,3,8,8,8,8,1,7,1,1,8,8,8,8,1,7,1,1,5,5,5,5,5,5,
  5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,1,7,1,3,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,
  2,2,2,7,1,1,1,1,1,1,1,1,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,
  5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,7,1,3,5,5,5,5,5,5,5,5,5,5,5,5,
  5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,
  5,5,5,5,5,5,5,5,5,5,5,5,5,7,1,3,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,
  5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,7,1,3,5,5,5,5,5,5,5,5,
  5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,
  5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,7,1,3,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,
  5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,7,1,3,5,5,5,5,5,5,
  5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,
  5,5,5,5,7,1,3,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,
  5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,7,1,3,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,
  5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,7,1,3,5,5,5,5,
  5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,
  5,5,5,5,5,5,7,1,3,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,
  5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,7,1,3,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,
  5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,7,1,3,5,5,
  5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,
  5,5,5,5,5,5,5,5,7,1,3,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,
  5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,7,1,3,5,5,5,5,5,5,5,5,5,5,5,5,5,
  5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,7,1,3,
  5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,
  5,5,5,5,5,5,5,5,5,5,7,1,3,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,
  5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,7,1,3,5,5,5,5,5,5,5,5,5,5,5,
  5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,7,
  1,3,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,
  5,5,5,5,5,5,5,5,5,5,5,5,7,1,3,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,
  5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,7,1,3,5,5,5,5,5,5,5,5,5,
  5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,
  5,7,1,3,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,
  5,5,5,5,5,5,5,5,5,5,5,5,5,5,7,1,3,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,
  5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,7,1,3,5,5,5,5,5,5,5,
  5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,
  5,5,5,7,1,3,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,
  5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,7,1,3,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,
  5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,
  5,5,5,5,5,5,7,1,3,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,
  5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,7,1,3,
  5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,
  5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,7,1,3,5,5,5,5,5,5,5,5,5,
  5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,
  5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,7,1,3,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,
  5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,
  5,5,5,5,5,5,7,1,3,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,
  5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,
  5,5,5,5,5,5,7,1,3,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,
  5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,7,1,3,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,
  5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,
  5,5,5,5,5,5,5,5,5,5,7,1,3,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,
  5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,
  7,1,3,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,
  5,5,5,5,5,5,5,5,5,5,5,5,5,7,1,3,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,
  5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,7,1,3,5,5,5,5,5,5,5,5,
  5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,
  5,5,7,1,3,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,
  5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,7,1,3,1,1,1,1,1,1,1,1,1,1,1,
  1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,9,5,3,3,3,3,3,
  3,3,3,3,3,3,3,3,3,3,3,3,3,3,3,3,3,3,3,3,3,3,3,3,3,3,3,3,3,3,3,3,3,3,3,3,1,
  7,3,1,7,3,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,
  5,5,5,5,5,5,5,5,5,5,1,7,1,3,2,2,2,2,2,2,2,2,2,2,2,1,2,2,2,2,2,1,1,1,1,1,1,
  1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,2,2,2,7,2,1,1,2,1,2,1,1,1,1,1,1,1,1,1,1,2,
  1,1,1,1,1,1,1,1,2,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,
  1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,7,1,1,2,2,1,2,2,1,1,
  2,2,1,1,1,1,2,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,2,2,1,2,2,1,1,2,8,8,8,
  8,1,7,1,1,1,1,1,1,1,1,7,1,3,3,1,3,1,1,1,2,2,2,1,1,1,2,2,2,2,2,7,2,2,2,2,3,
  2,2,2,2,2,2,2,2,2,2,2,7,3,2,1,2,2,2,2,2,2,2,7,3,2,2,2,1,2,2,2,2,7,2,1,7,1,
  1,7,1,1,7,1,2,2,2,2,2,2,2,2,2,2,2,1,2,2,2,2,2,1,1,1,1,1,1,1,1,1,1,1,1,1,1,
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
  1,1,1,1,1,1,2,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,2,1,1,2,2,2,2,2,7,2,2,2,2,2,
  2,2,1,1,2,2,2,2,2,7,2,2,2,2,2,2,2,1,1,2,2,2,2,2,7,2,2,2,2,2,2,2,1,1,2,2,2,
  2,2,7,2,2,2,2,2,2,2,2,2,2,7,2,2,2,2,2,2,2,7,2,2,2,2,2,2,2,2,2,2,7,2,2,2,2,
  1,2,2,2,2,2,2,2,7,2,2,2,2,1,2,2,2,2,2,2,2,7,2,2,2,2,1,2,1,1,1,2,2,2,2,2,7,
  2,2,2,2,2,2,2,2,1,1,1,2,2,2,2,2,7,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,7,2,2,1,
  2,2,2,2,2,2,2,2,2,7,2,2,1,2,2,2,2,2,2,2,2,2,7,2,2,1,2,2,2,2,2,2,2,2,2,7,2,
  2,1,2,2,2,2,2,2,2,2,2,7,1,2,1,2,2,2,2,2,2,2,2,2,7,2,2,1,2,2,2,2,2,2,2,2,2,
  7,2,2,1,2,2,2,2,2,2,2,2,2,7,2,2,1,2,2,2,2,2,2,2,2,2,7,2,2,1,2,2,2,2,2,2,2,
  2,2,7,2,2,1,2,2,2,2,2,2,2,2,2,7,2,2,1,2,2,2,2,2,2,2,2,2,2,2,1,2,2,2,2,2,1,
  1,1,2,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,2,2,2,7,2,1,1,2,1,2,1,2,1,1,2,
  1,1,1,1,1,1,1,1,2,2,1,1,1,1,1,1,1,1,1,2,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,
  1,1,2,2,2,2,1,1,1,1,1,1,2,9,7,2,1,1,2,1,1,1,1,5,5,5,5,5,5,5,5,5,5,5,5,5,5,
  5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,
  5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,
  5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,
  5,5,5,5,5,5,5,5,5,7,1,3,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,
  8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,
  8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,
  8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,1,7,1,
  1,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,1,7,1,3,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,7,2,
  1,1,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,1,7,1,3,2,2,2,2,2,2,2,2,2,2,2,2,
  2,2,2,2,2,2,2,7,2,1,5,5,5,5,5,7,1,3,5,5,5,5,5,7,1,3,5,5,1,7,1,3,1,2,7,2,1,
  1,2,1,1,5,5,5,5,5,7,1,3,5,5,5,5,5,7,1,3,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,
  5,5,5,7,1,3,2,2,2,2,2,2,2,2,2,2,2,2,2,4,4,4,4,2,2,4,7,2,1,1,2,7,1,1,1,2,7,
  2,1,1,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,
  5,5,5,5,5,5,5,5,5,5,7,1,3,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,
  8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,1,7,1,1,5,5,5,5,5,5,5,5,5,5,5,5,5,5,
  5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,1,7,1,3,2,2,2,2,
  2,2,2,2,2,2,2,1,2,2,2,2,2,1,1,1,2,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,2,
  2,2,7,2,1,1,2,1,2,1,2,1,1,1,1,1,1,1,1,1,2,2,1,1,1,1,1,1,1,1,2,1,1,1,1,1,1,
  1,1,1,1,1,1,1,1,1,1,1,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,
  5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,1,7,1,3,2,2,2,2,2,2,2,2,2,2,2,1,2,2,2,2,2,
  1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,2,2,2,7,2,1,1,2,1,2,1,1,1,1,1,
  1,1,1,1,2,2,1,1,1,1,1,1,1,1,2,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,5,5,5,5,5,5,
  5,5,5,5,5,5,5,5,5,1,7,1,3,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,7,2,1,1,5,5,5,5,5,
  5,5,5,5,5,5,5,5,5,5,1,7,1,3,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,7,2,1,1,5,5,5,5,
  5,5,5,5,5,5,5,5,5,5,5,1,7,1,3,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,7,2,1,1,1,5,5,
  5,5,5,5,5,5,5,5,5,5,5,5,5,1,7,1,3,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,7,2,1,1,1,
  5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,1,7,1,3,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,7,2,1,
  1,1,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,
  5,5,5,5,5,5,5,5,1,7,1,3,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,
  5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,1,7,1,3,2,2,2,2,2,2,2,2,2,2,2,1,2,2,2,
  2,2,1,1,1,2,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,2,2,2,7,2,1,1,2,1,2,1,2,
  1,1,2,1,1,1,1,1,1,1,1,2,2,1,1,1,1,1,1,1,1,1,2,1,1,1,1,1,1,1,1,1,1,1,1,1,1,
  1,1,1,1,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,
  5,5,5,5,5,5,5,5,5,5,1,7,1,3,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,
  5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,1,7,1,3,5,5,5,5,5,5,5,5,5,5,5,5,5,
  5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,1,7,1,3,2,2,2,
  2,2,2,2,2,2,2,2,1,2,2,2,2,2,1,1,1,2,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,
  2,2,2,7,2,1,1,2,1,2,1,2,1,1,2,1,1,1,1,1,1,1,1,2,2,1,1,1,1,1,1,1,1,1,2,1,1,
  1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,
  5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,1,7,1,3,5,5,5,5,5,5,5,5,5,5,5,5,
  5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,1,7,1,3,2,2,2,
  2,2,2,2,2,2,2,2,1,2,2,2,2,2,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,2,
  2,2,7,2,1,1,2,1,2,1,1,1,1,1,1,1,1,1,2,2,1,1,1,1,1,1,1,1,2,1,1,1,1,1,1,1,1,
  1,1,1,1,1,1,1,1,5,5,5,5,5,7,1,3,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,
  5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,1,7,1,3,5,5,5,5,5,5,5,5,5,5,5,5,
  5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,1,7,1,3,5,5,5,
  5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,
  5,5,1,7,1,3,2,2,2,2,2,2,2,2,2,2,2,1,2,2,2,2,2,1,1,1,1,1,1,1,1,1,1,1,1,1,1,
  1,1,1,1,1,1,1,1,2,2,2,7,2,1,1,2,1,2,1,1,1,1,1,1,1,1,1,2,2,1,1,1,1,1,1,1,1,
  2,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,7,2,1,1,5,
  5,5,5,5,5,5,7,1,3,5,5,5,5,5,5,5,7,1,3,1,1,1,1,7,3,3,1,3,1,1,1,1,1,1,1,1,1,
  1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,7,2,2,
  2,1,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,
  2,2,1,1,1,1,7,2,2,1,2,1,1,1,2,1,2,2,2,2,2,1,1,1,1,2,3,7,1,8,8,8,8,8,8,8,8,
  8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,1,7,1,
  1,1,1,1,1,7,2,2,1,2,1,1,1,1,1,1,1,7,2,2,1,2,1,1,1,1,1,1,1,7,2,2,1,2,1,1,1,
  2,2,2,2,2,2,2,2,2,7,2,2,2,2,2,2,7,2,1,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,
  2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,7,1,4,4,4,4,4,4,4,4,4,4,4,4,
  4,4,4,4,4,4,4,4,4,4,4,4,4,4,4,4,4,4,4,4,4,4,4,4,4,4,4,4,4,4,4,4,4,4,4,4,7,
  4,4,4,4,4,4,4,4,4,4,4,4,4,4,4,4,4,4,4,4,4,4,4,4,4,4,4,4,4,4,4,4,4,4,4,4,4,
  4,4,4,4,4,4,4,4,4,4,4,7,1,7,1,1,7,1,1,7,1,1,7,1,1,7,1,1,7,1,1,7,1,1,7,1,1,
  7,1,1,7,1,1,7,1,2,2,2,2,2,2,2,2,2,2,2,1,2,2,2,2,2,1,1,1,1,1,1,1,1,1,1,1,1,
  1,1,1,1,1,1,1,1,1,1,2,2,2,7,2,1,1,2,1,2,1,1,1,1,1,1,1,1,1,1,2,1,1,1,1,1,1,
  1,1,2,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,2,2,2,2,2,7,5,5,5,5,5,5,5,5,5,5,5,5,
  5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,7,1,3,8,8,8,8,8,8,
  8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,1,7,1,1,
  5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,
  5,5,5,7,1,3,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,
  8,8,8,8,8,8,8,8,1,7,1,1,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,
  8,8,8,8,8,8,8,8,8,8,8,8,8,8,1,7,1,1,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,
  8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,1,7,1,1,8,8,8,8,8,8,8,8,8,8,8,8,8,
  8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,1,7,1,1,8,8,8,8,8,8,8,
  8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,1,7,
  1,1,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,
  8,8,8,8,8,8,8,1,7,1,1,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,
  5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,7,1,3,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,
  8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,1,7,1,1,5,5,5,5,5,5,5,5,
  5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,7,1,
  3,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,
  8,8,8,8,8,8,1,7,1,1,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,
  5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,7,1,3,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,
  8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,1,7,1,1,5,5,5,5,5,5,5,5,5,
  5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,7,1,3,
  8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,
  8,8,8,8,8,1,7,1,1,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,
  5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,7,1,3,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,
  8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,1,7,1,1,8,8,8,8,8,8,8,8,8,8,
  8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,1,7,1,1,5,
  5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,
  5,5,5,5,5,7,1,3,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,
  8,8,8,8,8,8,8,8,8,8,8,8,8,1,7,1,1,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,
  8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,1,7,1,1,5,5,5,5,5,5,5,5,5,5,5,
  5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,7,1,3,8,8,
  8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,
  8,8,8,1,7,1,1,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,
  8,8,8,8,8,8,8,8,8,8,8,8,1,7,1,1,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,
  5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,1,7,1,3,5,5,5,5,5,5,5,5,5,5,5,5,
  5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,1,7,1,3,5,5,5,
  5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,
  5,5,1,7,1,3,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,
  5,5,5,5,5,5,5,5,5,5,5,1,7,1,3,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,
  5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,1,7,1,3,5,5,5,5,5,5,5,5,5,5,5,5,5,
  5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,1,7,1,3,5,5,5,5,
  5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,
  5,1,7,1,3,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,
  5,5,5,5,5,5,5,5,5,5,1,7,1,3,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,
  5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,1,7,1,3,5,5,5,5,5,5,5,5,5,5,5,5,5,5,
  5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,1,7,1,3,5,5,5,5,5,
  5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,
  1,7,1,3,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,
  5,5,5,5,5,5,5,5,5,1,7,1,3,1,1,1,1,7,2,2,1,2,1,1,1,1,1,1,1,7,2,2,1,2,1,1,1,
  1,1,1,1,7,2,2,1,2,1,1,1,1,1,1,1,7,2,2,1,2,1,1,1,1,1,1,1,7,2,2,1,2,1,1,1,2,
  2,2,2,2,2,2,2,2,2,2,2,2,2,2,7,1,1,1,1,1,1,1,7,2,2,1,2,1,1,1,1,1,1,1,7,2,2,
  1,2,1,1,1,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,
  2,2,2,2,2,2,2,2,2,3,2,7,1,1,1,1,7,2,2,1,2,1,1,1,1,1,1,1,7,2,2,1,2,1,1,1,10,
  10,10,10,10,10,10,10,10,10,10,10,10,10,10,10,10,10,10,5,5,5,7,1,3,8,8,8,8,
  1,7,1,1,8,8,8,8,1,7,1,1,8,8,8,8,1,7,1,1,8,8,8,8,1,7,1,1,5,5,5,5,5,5,5,5,5,
  5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,1,7,1,3,
  2,2,2,2,2,2,2,2,2,2,2,1,2,2,2,2,2,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,
  1,1,2,2,2,7,2,1,1,2,1,2,1,1,1,1,1,1,1,1,1,1,2,1,1,1,1,1,1,1,1,2,1,1,1,1,1,
  1,1,1,1,1,1,1,1,1,1,1,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,
  5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,1,7,1,3,2,2,2,2,2,2,2,2,2,2,2,1,2,2,2,2,2,
  1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,2,2,2,7,2,1,1,2,1,2,1,1,1,1,1,
  1,1,1,1,1,2,1,1,1,1,1,1,1,1,2,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,7,3,
  3,1,3,1,1,1,5,5,5,5,5,7,1,3,8,8,8,8,1,7,1,1,5,5,5,5,5,5,5,7,1,3,5,5,5,5,5,
  5,5,7,1,3,5,5,5,5,5,5,5,7,1,3,5,5,5,5,5,5,5,5,7,1,3,5,5,5,5,5,5,5,7,1,3,8,
  8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,
  8,8,8,8,1,7,1,1,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,
  8,8,8,8,8,8,8,8,8,8,8,8,8,1,7,1,1,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,
  8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,1,7,1,1,5,5,5,5,5,5,7,1,3,5,5,
  5,5,5,5,7,1,3,8,1,7,1,1,2,1,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,
  2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,7,1,2,1,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,
  2,2,2,3,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,7,2,5,5,5,5,5,5,5,7,1,3,
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
  2,7,1,1,7,1,2,2,2,2,2,2,2,2,2,2,2,1,2,2,2,2,2,1,1,1,2,1,1,1,1,1,1,1,1,1,1,
  1,1,1,1,1,1,1,1,1,2,2,2,7,2,1,1,2,1,2,1,2,1,1,2,1,1,1,1,1,1,1,1,2,2,1,1,1,
  1,1,1,1,1,1,2,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,4,4,4,4,4,7,1,1,4,4,4,
  4,4,7,1,1,4,4,4,4,4,7,1,1,4,4,4,4,4,7,1,1,4,4,4,4,4,7,1,1,1,1,1,1,1,1,1,1,
  1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,8,1,7,2,1,
  1,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,
  2,2,2,2,1,1,1,1,1,1,1,1,1,7,1,1,1,1,1,7,4,4,4,4,4,4,4,4,4,4,4,4,4,4,4,4,1,
  1,1,1,1,4,4,4,4,4,4,4,4,4,4,4,4,4,4,4,4,4,4,4,4,4,7,2,2,2,2,2,2,2,2,2,2,2,
  1,2,2,2,2,2,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,2,2,2,7,2,1,1,2,1,
  2,1,1,1,1,1,1,1,1,1,1,2,1,1,1,1,1,1,1,1,2,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,
  2,7,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,1,7,1,1,2,2,2,2,2,2,2,2,2,2,2,1,2,2,2,2,
  2,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,2,2,2,7,2,1,1,2,1,2,1,1,1,1,
  1,1,1,1,1,1,2,1,1,1,1,1,1,1,1,2,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,2,2,2,2,2,
  2,2,2,2,2,2,1,2,2,2,2,2,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,2,2,2,
  7,2,1,1,2,1,2,1,1,1,1,1,1,1,1,1,1,2,1,1,1,1,1,1,1,1,2,1,1,1,1,1,1,1,1,1,1,
  1,1,1,1,1,1,2,2,2,2,2,2,2,2,2,2,2,1,2,2,2,2,2,1,1,1,1,1,1,1,1,1,1,1,1,1,1,
  1,1,1,1,1,1,1,1,2,2,2,7,2,1,1,2,1,2,1,1,1,1,1,1,1,1,1,1,2,1,1,1,1,1,1,1,1,
  2,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,2,2,2,2,2,2,2,2,2,2,2,1,2,2,2,2,2,1,1,1,
  1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,2,2,2,7,2,1,1,2,1,2,1,1,1,1,1,1,1,1,
  1,1,2,1,1,1,1,1,1,1,1,2,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,2,2,2,2,2,2,2,2,2,
  2,2,1,2,2,2,2,2,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,2,2,2,7,2,1,1,
  2,1,2,1,1,1,1,1,1,1,1,1,1,2,1,1,1,1,1,1,1,1,2,1,1,1,1,1,1,1,1,1,1,1,1,1,1,
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
  1,1,1,2,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,8,1,7,1,1,2,2,2,2,2,2,2,2,2,2,2,1,
  2,2,2,2,2,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,2,2,2,7,2,1,1,2,1,2,1,1,2,
  1,1,1,2,2,2,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,2,2,2,2,2,2,2,2,2,2,2,1,2,2,2,2,
  2,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,2,2,2,7,2,1,1,2,1,2,1,1,2,1,1,1,2,
  2,2,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,2,2,2,2,2,2,2,2,2,2,2,1,2,2,2,2,2,1,1,1,
  1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,2,2,2,7,2,1,1,2,1,2,1,1,2,1,1,1,2,2,2,1,1,
  1,1,1,1,1,1,1,1,1,1,1,1,1,2,2,2,2,2,2,2,2,2,2,2,1,2,2,2,2,2,1,1,1,1,1,1,1,
  1,1,1,1,1,1,1,1,1,1,1,1,2,2,2,7,2,1,1,2,1,2,1,1,2,1,1,1,2,2,2,1,1,1,1,1,1,
  1,1,1,1,1,1,1,1,1,2,2,2,2,2,2,2,2,2,2,2,1,2,2,2,2,2,1,1,1,1,1,1,1,1,1,1,1,
  1,1,1,1,1,1,1,1,2,2,2,7,2,1,1,2,1,2,1,1,2,1,1,1,2,2,2,1,1,1,1,1,1,1,1,1,1,
  1,1,1,1,1,2,2,2,2,2,2,2,2,2,2,2,1,2,2,2,2,2,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,
  1,1,1,1,1,1,1,2,2,2,7,2,1,1,2,1,2,1,1,1,1,2,1,1,1,1,1,1,1,1,2,1,1,1,1,1,1,
  1,1,1,1,1,1,1,1,1,1,2,2,2,2,2,2,2,2,2,2,2,1,2,2,2,2,2,1,1,1,1,1,1,1,1,1,1,
  1,1,1,1,1,1,1,1,1,1,1,1,2,2,2,7,2,1,1,2,1,2,1,1,1,1,2,1,1,1,1,1,1,1,1,2,1,
  1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,2,2,2,2,2,2,2,2,2,2,2,1,2,2,2,2,2,1,1,1,1,1,
  1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,2,2,2,7,2,1,1,2,1,2,1,1,1,1,1,2,1,1,1,1,
  1,1,1,1,2,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,2,2,2,2,2,2,2,2,2,2,2,1,2,2,2,2,
  2,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,2,2,2,7,2,1,1,2,1,2,1,1,1,1,
  1,2,1,1,1,1,1,1,1,1,2,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,2,2,2,2,2,2,2,2,2,2,
  2,1,2,2,2,2,2,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,2,2,2,7,2,1,1,2,
  1,2,1,1,1,1,1,2,1,1,1,1,1,1,1,1,2,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,2,2,2,2,
  2,2,2,2,2,2,2,1,2,2,2,2,2,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,2,2,
  2,7,2,1,1,2,1,2,1,1,1,1,1,2,1,1,1,1,1,1,1,1,2,1,1,1,1,1,1,1,1,1,1,1,1,1,1,
  1,1,2,2,2,2,2,2,2,2,2,2,2,1,2,2,2,2,2,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,
  1,1,1,1,2,2,2,7,2,1,1,2,1,2,1,1,1,1,1,1,2,1,1,1,1,1,1,1,1,2,1,1,1,1,1,1,1,
  1,1,1,1,1,1,1,1,1,2,2,2,2,2,2,2,2,2,2,2,1,2,2,2,2,2,1,1,1,1,1,1,1,1,1,1,1,
  1,1,1,1,1,1,1,1,1,1,1,2,2,2,7,2,1,1,2,1,2,1,1,1,1,1,1,2,1,1,1,1,1,1,1,1,2,
  1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,2,2,2,2,2,2,2,2,2,2,2,1,2,2,2,2,2,1,1,1,1,
  1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,2,2,2,7,2,1,1,2,1,2,1,1,1,1,1,1,1,2,1,
  1,1,1,1,1,1,1,2,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,2,2,2,2,2,2,2,2,2,2,2,1,2,
  2,2,2,2,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,2,2,2,7,2,1,1,2,1,2,1,
  1,1,1,1,1,1,2,1,1,1,1,1,1,1,1,2,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,2,2,2,2,2,
  2,2,2,2,2,2,1,2,2,2,2,2,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,2,2,2,
  7,2,1,1,2,1,2,1,1,1,1,1,1,1,1,2,1,1,1,1,1,1,1,1,2,1,1,1,1,1,1,1,1,1,1,1,1,
  1,1,1,1,2,2,2,2,2,2,2,2,2,2,2,1,2,2,2,2,2,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,
  1,1,1,1,1,1,2,2,2,7,2,1,1,2,1,2,1,1,1,1,1,1,1,1,2,1,1,1,1,1,1,1,1,2,1,1,1,
  1,1,1,1,1,1,1,1,1,1,1,1,1,8,8,8,8,1,7,1,1,1,1,1,1,7,2,2,1,2,1,1,1,1,1,1,1,
  7,2,2,1,2,1,1,1,1,1,1,1,7,2,2,1,2,1,1,1,1,1,1,1,7,2,2,1,2,1,1,1,8,8,8,8,1,
  7,1,1,8,8,8,8,1,7,1,1,1,1,1,1,7,2,2,1,2,1,1,1,2,2,2,2,2,2,2,2,2,2,2,1,2,2,
  2,2,2,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,2,2,2,7,2,1,1,2,1,2,1,1,
  1,1,1,1,1,1,1,2,2,1,1,1,1,1,1,1,1,2,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,2,2,2,
  2,2,2,2,2,2,2,2,1,2,2,2,2,2,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,2,
  2,2,7,2,1,1,2,1,2,1,1,1,1,1,1,1,1,1,2,2,1,1,1,1,1,1,1,1,2,1,1,1,1,1,1,1,1,
  1,1,1,1,1,1,1,1,2,2,2,2,2,2,2,2,2,2,2,1,2,2,2,2,2,1,1,1,1,1,1,1,1,1,1,1,1,
  1,1,1,1,1,1,1,1,1,1,2,2,2,7,2,1,1,2,1,2,1,1,1,1,1,1,1,1,1,2,2,1,1,1,1,1,1,
  1,1,2,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,7,1,2,1,2,2,2,2,2,2,2,2,2,2,2,2,2,
  2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,7,1,5,5,5,5,5,5,5,
  5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,
  5,5,5,5,5,5,5,5,5,7,1,3,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,
  8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,1,7,1,1,1,7,1,5,5,5,5,5,1,7,1,3,2,1,2,
  2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,
  2,2,7,2,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,
  8,8,8,8,8,8,8,8,8,1,7,1,1,1,4,4,4,4,4,7,1,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,1,
  7,1,1,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,
  5,5,5,5,5,5,5,5,5,5,5,7,1,3,4,4,4,4,4,4,4,4,4,4,4,4,4,4,4,4,4,4,4,4,4,4,4,
  4,4,4,4,4,4,4,4,4,4,4,4,4,4,4,4,4,5,5,5,5,5,7,1,1,1,1,7,2,2,1,2,1,1,1,4,4,
  4,4,4,4,4,4,2,2,2,2,4,4,4,4,2,2,2,2,2,4,4,4,4,4,4,4,4,4,4,4,4,4,4,4,4,4,4,
  4,4,4,7,2,2,2,2,2,7,8,8,8,8,1,7,1,1,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,7,1,1,1,
  1,1,7,2,1,7,2,1,7,2,1,7,2,1,7,2,1,7,2,1,7,2,1,7,2,1,7,2,1,7,2,1,7,3,4,1,1,
  1,1,1,4,4,4,4,4,4,4,4,4,4,4,4,4,4,4,4,4,4,4,4,4,4,4,4,4,4,4,4,4,4,7,1,1,1,
  1,1,4,1,1,1,1,1,4,4,4,4,4,4,4,4,4,4,4,4,4,4,4,4,4,4,4,4,4,4,4,4,4,4,4,4,4,
  4,7,1,1,1,1,1,4,1,1,4,4,4,4,4,4,4,4,4,4,4,4,4,4,4,4,4,4,4,4,4,4,4,4,4,4,4,
  4,4,4,7,1,1,4,1,1,4,4,4,4,4,4,4,4,4,4,4,4,4,4,4,4,4,4,4,4,4,4,4,4,4,4,4,4,
  4,4,7,1,1,4,1,1,4,4,4,4,4,4,4,4,4,4,4,4,4,4,4,4,4,4,4,4,4,4,4,4,4,4,4,4,4,
  4,7,1,1,4,1,1,4,4,4,4,4,4,4,4,4,4,4,4,4,4,4,4,4,4,4,4,4,4,4,4,4,4,4,4,4,4,
  7,1,1,4,1,1,1,1,4,4,4,4,4,4,4,4,4,4,4,4,4,4,4,4,4,4,4,4,4,4,4,4,4,4,7,1,1,
  1,1,4,1,1,1,1,4,4,4,4,4,4,4,4,4,4,4,4,4,4,4,4,4,4,4,4,4,4,4,4,4,4,7,1,1,1,
  1,4,1,1,4,4,4,4,4,4,4,4,4,4,4,4,4,4,4,4,4,4,4,4,4,4,4,4,7,1,1,4,1,1,4,4,4,
  4,4,4,4,4,4,4,4,4,4,4,4,4,4,4,4,4,4,4,4,4,7,1,1,4,1,1,4,4,4,4,4,4,4,4,4,4,
  4,4,4,4,4,4,4,4,4,4,4,4,7,1,1,4,1,1,4,4,4,4,4,4,4,4,4,4,4,4,4,4,4,4,4,4,4,
  4,4,4,7,1,1,1,1,1,1,7,2,2,1,2,1,1,1,1,1,1,1,7,2,2,1,2,1,1,1,1,1,1,1,7,2,2,
  1,2,1,1,1,8,8,8,8,8,8,8,8,8,1,7,1,1,4,4,4,4,4,4,4,4,4,4,4,4,4,4,4,4,4,4,4,
  4,4,2,4,4,4,4,4,4,4,4,4,4,4,4,4,4,4,4,4,4,4,4,4,7,2,2,2,2,2,2,2,2,2,2,2,1,
  2,2,2,2,2,1,1,1,2,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,2,2,2,7,2,1,1,2,1,
  2,1,2,1,1,2,1,1,1,1,1,1,1,1,2,2,1,1,1,1,1,1,1,1,2,1,1,1,1,1,1,1,1,1,1,1,1,
  1,1,1,1,1,1,2,2,2,2,2,2,2,2,2,2,2,1,2,2,2,2,2,1,1,1,1,1,1,1,1,1,1,1,1,1,1,
  1,1,1,1,1,1,1,1,2,2,2,7,2,1,1,2,1,2,1,1,1,1,1,1,1,1,1,2,2,1,1,1,1,1,1,1,1,
  2,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,7,2,1,1,1,
  1,1,1,7,2,2,1,2,1,1,1,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,5,2,1,7,1,3,2,5,
  5,7,1,3,8,1,7,1,1,2,2,2,2,2,2,2,2,2,7,2,2,1,1,7,2
};


static const unsigned short ag_pstt[] = {
2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,0,1,2,
18,20,
131,131,131,131,131,131,131,131,131,131,131,131,131,3,8,8,8,8,8,166,165,164,
  130,129,7,10,2,9,0,11,11,11,10,8,11,5,5,4,6,4,
19,19,19,19,19,19,19,19,19,19,19,19,19,1,3,1,887,
132,132,132,132,132,132,132,132,132,132,132,132,133,133,133,133,133,132,163,
  163,132,163,163,163,163,162,163,163,4,133,
12,13,14,15,16,17,18,19,20,21,22,23,24,5,37,40,36,35,34,33,32,31,30,29,28,
  27,26,25,
19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,
  19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,
  19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,
  19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,
  19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,
  19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,
  19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,
  19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,
  19,19,19,1,6,1,877,
38,7,38,
40,40,39,41,42,43,38,8,24,24,46,24,45,44,44,40,40,
47,47,47,47,47,47,47,47,47,47,47,47,47,47,47,47,47,47,47,47,47,47,47,47,47,
  47,47,47,47,47,47,47,47,47,47,47,47,47,47,47,47,47,47,47,47,47,47,47,47,
  47,47,47,47,47,47,47,47,47,47,47,47,47,47,47,47,47,47,47,47,47,47,47,47,
  47,47,47,47,47,47,47,47,47,47,47,47,47,47,47,47,47,47,47,47,47,47,47,47,
  47,47,47,47,47,47,47,47,47,47,47,47,47,47,47,47,47,47,47,47,47,47,47,47,
  47,47,47,47,47,47,47,47,47,47,47,47,47,47,47,47,47,47,47,47,47,47,47,47,
  47,47,47,47,47,47,47,47,47,47,47,47,47,47,47,47,47,47,47,47,47,47,47,47,
  47,47,47,47,47,47,47,47,47,47,47,47,47,47,47,47,47,47,19,19,47,47,47,47,
  47,47,47,1,9,1,47,
131,131,131,131,131,131,131,131,131,131,131,131,131,115,116,117,118,119,120,
  121,122,123,124,125,126,127,128,129,130,131,132,133,134,135,136,137,138,
  66,67,68,69,70,71,139,140,141,142,143,144,72,73,74,75,76,145,146,147,77,
  78,79,80,81,148,149,150,151,152,153,154,155,156,157,158,159,160,161,162,
  111,63,64,65,169,170,171,172,173,174,175,176,177,178,179,180,181,182,
  183,184,163,164,165,166,167,168,82,83,84,85,86,87,49,50,51,88,89,90,91,
  92,93,94,95,96,52,53,97,98,99,100,101,102,103,104,54,55,56,105,106,57,
  58,107,108,109,110,59,60,61,62,269,269,269,269,269,269,269,269,269,269,
  269,269,269,269,269,269,269,269,269,269,269,269,269,269,269,269,269,269,
  269,269,269,269,17,18,21,269,269,269,269,269,20,20,39,20,20,20,20,130,
  129,18,10,265,266,267,264,270,269,269,36,268,105,106,270,48,263,200,264,
  264,264,264,264,264,264,264,264,264,327,327,327,327,327,328,329,330,331,
  334,334,334,335,336,340,340,340,340,341,342,343,344,345,346,347,348,351,
  351,351,352,353,354,355,356,357,358,359,360,364,364,364,364,365,366,367,
  368,369,370,246,245,244,243,242,241,262,261,260,259,258,257,256,255,254,
  253,252,251,250,249,248,247,188,114,187,113,186,112,185,240,239,238,237,
  236,235,234,233,232,231,230,229,228,227,226,225,225,225,225,225,225,224,
  223,222,221,221,221,221,221,221,220,219,218,217,216,215,214,214,214,214,
  213,213,213,213,212,211,210,209,208,207,206,205,204,203,202,201,199,198,
  197,196,195,194,193,192,191,190,189,263,
131,131,131,131,131,131,131,131,131,131,131,131,131,3,8,8,8,8,8,166,165,164,
  130,129,7,4,10,11,9,3,3,10,8,3,5,5,4,6,4,
19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,1,12,1,906,
19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,1,13,1,905,
19,1,14,1,904,
19,1,15,1,902,
19,1,16,1,901,
19,19,19,19,19,17,1,900,
19,19,19,19,19,18,1,899,
19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,1,19,1,898,
19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,
  19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,1,20,1,897,
19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,
  19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,1,21,1,896,
19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,1,22,1,895,
19,19,19,19,19,1,23,1,889,
19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,1,24,1,882,
131,131,131,131,131,131,131,131,131,131,131,131,131,130,129,25,265,271,263,
  263,
131,131,131,131,131,131,131,131,131,131,131,131,131,130,129,26,272,263,263,
140,27,275,273,274,
140,28,276,273,274,
140,29,277,273,274,
278,278,278,278,1,30,1,278,
279,279,279,279,1,31,1,279,
131,131,131,131,131,131,131,131,131,131,131,131,131,130,129,32,280,263,263,
131,131,131,131,131,131,131,131,131,131,131,282,131,250,250,250,250,285,287,
  284,294,296,297,298,299,300,301,302,303,304,320,321,322,283,319,286,295,
  307,281,265,130,129,33,234,292,293,235,288,236,289,265,332,331,329,334,
  330,333,328,335,337,220,323,309,263,336,324,328,327,326,325,219,328,318,
  317,316,315,314,313,312,311,310,308,291,290,263,306,305,
131,131,131,131,131,131,131,131,131,131,131,282,131,250,250,250,250,285,287,
  284,294,296,297,298,299,300,301,302,303,304,320,321,322,283,319,286,295,
  307,281,265,130,129,34,234,292,293,235,288,236,289,265,332,331,329,334,
  330,333,328,335,338,220,323,309,263,336,324,328,327,326,325,219,328,318,
  317,316,315,314,313,312,311,310,308,291,290,263,306,305,
131,131,131,131,131,131,131,131,131,131,131,131,131,130,129,35,339,263,263,
340,342,344,346,348,36,349,347,345,343,341,
158,140,37,353,273,352,350,274,351,
19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,
  19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,
  19,19,19,19,19,19,38,1,872,
19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,
  19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,
  19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,
  19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,39,1,878,
354,24,40,355,355,355,
19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,
  19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,41,1,876,
19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,
  19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,42,1,
  874,
19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,
  19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,43,1,873,
356,358,360,361,362,363,364,365,366,367,368,369,370,371,372,373,374,375,376,
  377,378,379,386,389,390,391,320,322,388,380,384,381,383,387,359,357,307,
  385,382,39,41,392,42,38,15,44,16,15,15,44,13,44,15,15,15,15,15,15,15,15,
  15,15,15,15,15,15,15,15,15,15,15,15,15,15,15,15,15,15,15,15,15,15,15,15,
  15,15,15,15,15,15,15,15,
356,358,360,361,362,363,364,365,366,367,368,369,370,371,372,373,374,375,376,
  377,378,379,386,389,390,391,320,322,388,380,384,381,383,387,359,357,307,
  385,382,39,42,394,393,45,393,394,393,393,393,393,393,393,393,393,393,
  393,393,393,393,393,393,393,393,393,393,393,393,393,393,393,393,393,393,
  393,393,393,393,393,393,393,393,393,393,393,393,393,393,
356,358,360,361,362,363,364,365,366,367,368,369,370,371,372,373,374,375,376,
  377,378,379,386,389,390,391,320,322,388,380,384,381,383,387,359,357,307,
  385,382,39,42,395,393,46,393,395,393,393,393,393,393,393,393,393,393,
  393,393,393,393,393,393,393,393,393,393,393,393,393,393,393,393,393,393,
  393,393,393,393,393,393,393,393,393,393,393,393,393,393,
131,131,131,131,131,131,131,131,131,131,131,131,131,115,116,117,118,119,120,
  121,122,123,124,125,126,127,128,129,130,131,132,133,134,135,136,137,138,
  66,67,68,69,70,71,139,140,141,142,143,144,72,73,74,75,76,145,146,147,77,
  78,79,80,81,148,149,150,151,152,153,154,155,156,157,158,159,160,161,162,
  111,63,64,65,169,170,171,172,173,174,175,176,177,178,179,180,181,182,
  183,184,163,164,165,166,167,168,82,83,84,85,86,87,49,50,51,88,89,90,91,
  92,93,94,95,96,52,53,97,98,99,100,101,102,103,104,54,55,56,105,106,57,
  58,107,108,109,110,59,60,61,62,398,398,398,398,398,398,398,398,398,398,
  398,398,398,398,398,398,398,398,398,398,398,398,398,398,398,398,398,398,
  398,398,398,398,17,18,21,398,398,398,398,398,396,398,398,39,41,42,43,38,
  130,129,47,265,266,267,264,399,23,23,46,23,45,44,44,398,398,397,37,268,
  105,106,399,48,263,200,264,264,264,264,264,264,264,264,264,264,327,327,
  327,327,327,328,329,330,331,334,334,334,335,336,340,340,340,340,341,342,
  343,344,345,346,347,348,351,351,351,352,353,354,355,356,357,358,359,360,
  364,364,364,364,365,366,367,368,369,370,246,245,244,243,242,241,262,261,
  260,259,258,257,256,255,254,253,252,251,250,249,248,247,188,114,187,113,
  186,112,185,240,239,238,237,236,235,234,233,232,231,230,229,228,227,226,
  225,225,225,225,225,225,224,223,222,221,221,221,221,221,221,220,219,218,
  217,216,215,214,214,214,214,213,213,213,213,212,211,210,209,208,207,206,
  205,204,203,202,201,199,198,197,196,195,194,193,192,191,190,189,263,
400,400,400,400,400,400,1,48,1,400,
19,19,19,19,19,19,49,1,1030,
19,19,19,19,19,19,50,1,1029,
19,19,19,19,19,19,51,1,1028,
19,19,19,19,19,19,52,1,1018,
19,19,19,19,19,19,53,1,1017,
19,19,19,19,19,19,54,1,1008,
19,19,19,19,19,19,55,1,1007,
19,19,19,19,19,19,56,1,1006,
19,19,19,19,19,19,57,1,1003,
19,19,19,19,19,19,58,1,1002,
19,19,19,19,19,19,59,1,997,
19,19,19,19,19,19,60,1,996,
19,19,19,19,19,19,61,1,995,
19,19,19,19,19,19,62,1,994,
19,19,19,19,19,19,19,19,19,19,63,1,1063,
19,19,19,19,19,19,19,19,19,19,64,1,1062,
19,19,19,19,19,19,19,19,65,1,1061,
19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,
  19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,66,1,1104,
19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,
  19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,67,1,1103,
19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,
  19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,68,1,1102,
19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,
  19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,69,1,1101,
19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,
  19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,70,1,1100,
19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,
  19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,71,1,1099,
19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,
  19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,72,1,1092,
19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,
  19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,73,1,1091,
19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,
  19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,74,1,1090,
19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,
  19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,75,1,1089,
19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,
  19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,76,1,1088,
19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,
  19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,77,1,1084,
19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,
  19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,78,1,1083,
19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,
  19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,79,1,1082,
19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,
  19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,80,1,1081,
19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,
  19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,81,1,1080,
19,19,19,19,19,19,82,1,1036,
19,19,19,19,19,19,83,1,1035,
19,19,19,19,19,19,84,1,1034,
19,19,19,19,19,19,85,1,1033,
19,19,19,19,19,19,86,1,1032,
19,19,19,19,19,19,87,1,1031,
19,19,19,19,19,19,88,1,1027,
19,19,19,19,19,19,89,1,1026,
19,19,19,19,19,19,90,1,1025,
19,19,19,19,19,19,91,1,1024,
19,19,19,19,19,19,92,1,1023,
19,19,19,19,19,19,93,1,1022,
19,19,19,19,19,19,94,1,1021,
19,19,19,19,19,19,95,1,1020,
19,19,19,19,19,19,96,1,1019,
19,19,19,19,19,19,97,1,1016,
19,19,19,19,19,19,98,1,1015,
19,19,19,19,19,19,99,1,1014,
19,19,19,19,19,19,100,1,1013,
19,19,19,19,19,19,101,1,1012,
19,19,19,19,19,19,102,1,1011,
19,19,19,19,19,19,103,1,1010,
19,19,19,19,19,19,104,1,1009,
19,19,19,19,19,19,105,1,1005,
19,19,19,19,19,19,106,1,1004,
19,19,19,19,19,19,107,1,1001,
19,19,19,19,19,19,108,1,1000,
19,19,19,19,19,19,109,1,999,
19,19,19,19,19,19,110,1,998,
19,19,19,19,19,111,1,1064,
401,401,401,401,401,401,401,401,401,1,112,1,401,
402,402,402,402,402,402,402,402,402,1,113,1,402,
403,403,403,403,403,403,403,1,114,1,403,
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
  19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,135,1,1108,
19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,
  19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,136,1,1107,
19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,
  19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,137,1,1106,
19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,
  19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,138,1,1105,
19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,
  19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,139,1,1098,
19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,
  19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,140,1,1097,
19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,
  19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,141,1,1096,
19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,
  19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,142,1,1095,
19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,
  19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,143,1,1094,
19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,
  19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,144,1,1093,
19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,
  19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,145,1,1087,
19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,
  19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,146,1,1086,
19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,
  19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,147,1,1085,
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
19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,
  19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,159,1,1068,
19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,
  19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,160,1,1067,
19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,
  19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,161,1,1066,
19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,
  19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,162,1,1065,
19,19,19,19,19,19,19,19,19,163,1,1042,
19,19,19,19,19,19,19,19,19,164,1,1041,
19,19,19,19,19,19,19,19,19,165,1,1040,
19,19,19,19,19,19,19,19,19,166,1,1039,
19,19,19,19,19,167,1,1038,
19,19,19,19,19,168,1,1037,
19,19,19,19,19,19,19,19,169,1,1060,
19,19,19,19,19,19,19,19,170,1,1059,
19,19,19,19,19,19,19,19,171,1,1058,
19,19,19,19,19,19,19,19,19,19,172,1,1056,
19,19,19,19,19,19,19,19,19,19,173,1,1055,
19,19,19,19,19,19,19,19,19,19,174,1,1054,
19,19,19,19,19,19,19,19,19,19,175,1,1053,
19,19,19,19,19,19,19,19,19,19,176,1,1052,
19,19,19,19,19,19,19,19,19,19,177,1,1051,
19,19,19,19,19,19,19,19,19,19,178,1,1050,
19,19,19,19,19,19,19,19,19,19,179,1,1049,
19,19,19,19,19,19,19,19,19,19,180,1,1048,
19,19,19,19,19,19,19,19,19,19,181,1,1047,
19,19,19,19,19,19,19,19,19,19,182,1,1046,
19,19,19,19,19,19,19,19,19,19,183,1,1045,
19,19,19,19,19,19,19,19,19,19,184,1,1044,
404,404,404,404,1,185,1,404,
405,1,186,1,405,
406,1,187,1,406,
407,1,188,1,407,
408,408,408,408,408,408,408,408,408,408,408,408,408,408,408,408,408,408,408,
  408,408,408,408,408,408,408,408,408,408,408,408,408,408,408,408,408,408,
  408,408,408,408,408,1,189,1,408,
409,409,409,409,409,409,409,409,409,409,409,409,409,409,409,409,409,409,409,
  409,409,409,409,409,409,409,409,409,409,409,409,409,409,409,409,409,409,
  409,409,409,409,409,1,190,1,409,
410,410,410,410,410,410,410,410,410,410,410,410,410,410,410,410,410,410,410,
  410,410,410,410,410,410,410,410,410,410,410,410,410,410,410,410,410,410,
  410,410,410,410,410,1,191,1,410,
411,411,411,411,411,411,411,411,411,411,411,411,411,411,411,411,411,411,411,
  411,411,411,411,411,411,411,411,411,411,411,411,411,411,411,411,411,411,
  411,411,411,411,411,1,192,1,411,
412,412,412,412,412,412,412,412,412,412,412,412,412,412,412,412,412,412,412,
  412,412,412,412,412,412,412,412,412,412,412,412,412,412,412,412,412,412,
  412,412,412,412,412,1,193,1,412,
413,413,413,413,413,413,413,413,413,413,413,413,413,413,413,413,413,413,413,
  413,413,413,413,413,413,413,413,413,413,413,413,413,413,413,413,413,413,
  413,413,413,413,413,1,194,1,413,
414,414,414,414,414,414,414,414,414,414,414,414,414,414,414,414,414,414,414,
  414,414,414,414,414,414,414,414,414,414,414,414,414,414,414,414,414,414,
  414,414,414,414,414,1,195,1,414,
415,415,415,415,415,415,415,415,415,415,415,415,415,415,415,415,415,415,415,
  415,415,415,415,415,415,415,415,415,415,415,415,415,415,415,415,415,415,
  415,415,415,415,415,1,196,1,415,
416,416,416,416,416,416,416,416,416,416,416,416,416,416,416,416,416,416,416,
  416,416,416,416,416,416,416,416,416,416,416,416,416,416,416,416,416,416,
  416,416,416,416,416,1,197,1,416,
417,417,417,417,417,417,417,417,417,417,417,417,417,417,417,417,417,417,417,
  417,417,417,417,417,417,417,417,417,417,417,417,417,417,417,417,417,417,
  417,417,417,417,417,1,198,1,417,
418,418,418,418,418,418,418,418,418,418,418,418,418,418,418,418,418,418,418,
  418,418,418,418,418,418,418,418,418,418,418,418,418,418,418,418,418,418,
  418,418,418,418,418,1,199,1,418,
419,419,419,419,419,419,419,419,419,419,419,419,419,419,419,419,419,419,419,
  419,419,419,419,419,419,419,419,419,419,419,419,419,419,419,419,419,419,
  419,419,419,419,419,1,200,1,419,
420,420,420,420,420,420,420,420,420,420,420,420,420,420,420,420,420,420,420,
  420,420,420,420,420,420,420,420,420,420,420,420,420,420,420,420,420,420,
  420,420,420,420,420,1,201,1,420,
421,421,421,421,421,421,421,421,421,421,421,421,421,421,421,421,421,421,421,
  421,421,421,421,421,421,421,421,421,421,421,421,421,421,421,421,421,421,
  421,421,421,421,421,1,202,1,421,
422,422,422,422,422,422,422,422,422,422,422,422,422,422,422,422,422,422,422,
  422,422,422,422,422,422,422,422,422,422,422,422,422,422,422,422,422,422,
  422,422,422,422,422,1,203,1,422,
423,423,423,423,423,423,423,423,423,423,423,423,423,423,423,423,423,423,423,
  423,423,423,423,423,423,423,423,423,423,423,423,423,423,423,423,423,423,
  423,423,423,423,423,1,204,1,423,
424,424,424,424,424,424,424,424,424,424,424,424,424,424,424,424,424,424,424,
  424,424,424,424,424,424,424,424,424,424,424,424,424,424,424,424,424,424,
  424,424,424,424,424,1,205,1,424,
425,425,425,425,425,425,425,425,425,425,425,425,425,425,425,425,425,425,425,
  425,425,425,425,425,425,425,425,425,425,425,425,425,425,425,425,425,425,
  425,425,425,425,425,1,206,1,425,
426,426,426,426,426,426,426,426,426,426,426,426,426,426,426,426,426,426,426,
  426,426,426,426,426,426,426,426,426,426,426,426,426,426,426,426,426,426,
  426,426,426,426,426,1,207,1,426,
427,427,427,427,427,427,427,427,427,427,427,427,427,427,427,427,427,427,427,
  427,427,427,427,427,427,427,427,427,427,427,427,427,427,427,427,427,427,
  427,427,427,427,427,1,208,1,427,
428,428,428,428,428,428,428,428,428,428,428,428,428,428,428,428,428,428,428,
  428,428,428,428,428,428,428,428,428,428,428,428,428,428,428,428,428,428,
  428,428,428,428,428,1,209,1,428,
429,429,429,429,429,429,429,429,429,429,429,429,429,429,429,429,429,429,429,
  429,429,429,429,429,429,429,429,429,429,429,429,429,429,429,429,429,429,
  429,429,429,429,429,1,210,1,429,
430,430,430,430,430,430,430,430,430,430,430,430,430,430,430,430,430,430,430,
  430,430,430,430,430,430,430,430,430,430,430,430,430,430,430,430,430,430,
  430,430,430,430,430,1,211,1,430,
431,431,431,431,431,431,431,431,431,431,431,431,431,431,431,431,431,431,431,
  431,431,431,431,431,431,431,431,431,431,431,431,431,431,431,431,431,431,
  431,431,431,431,431,1,212,1,431,
432,432,432,432,432,432,432,432,432,432,432,432,432,432,432,432,432,432,432,
  432,432,432,432,432,432,432,432,432,432,432,432,432,432,432,432,432,432,
  432,432,432,432,432,1,213,1,432,
433,433,433,433,433,433,433,433,433,433,433,433,433,433,433,433,433,433,433,
  433,433,433,433,433,433,433,433,433,433,433,433,433,433,433,433,433,433,
  433,433,433,433,433,1,214,1,433,
434,434,434,434,434,434,434,434,434,434,434,434,434,434,434,434,434,434,434,
  434,434,434,434,434,434,434,434,434,434,434,434,434,434,434,434,434,434,
  434,434,434,434,434,1,215,1,434,
435,435,435,435,435,435,435,435,435,435,435,435,435,435,435,435,435,435,435,
  435,435,435,435,435,435,435,435,435,435,435,435,435,435,435,435,435,435,
  435,435,435,435,435,1,216,1,435,
436,436,436,436,436,436,436,436,436,436,436,436,436,436,436,436,436,436,436,
  436,436,436,436,436,436,436,436,436,436,436,436,436,436,436,436,436,436,
  436,436,436,436,436,1,217,1,436,
437,437,437,437,437,437,437,437,437,437,437,437,437,437,437,437,437,437,437,
  437,437,437,437,437,437,437,437,437,437,437,437,437,437,437,437,437,437,
  437,437,437,437,437,1,218,1,437,
438,438,438,438,438,438,438,438,438,438,438,438,438,438,438,438,438,438,438,
  438,438,438,438,438,438,438,438,438,438,438,438,438,438,438,438,438,438,
  438,438,438,438,438,1,219,1,438,
439,439,439,439,439,439,439,439,439,439,439,439,439,439,439,439,439,439,439,
  439,439,439,439,439,439,439,439,439,439,439,439,439,439,439,439,439,439,
  439,439,439,439,439,1,220,1,439,
440,440,440,440,440,440,440,440,440,440,440,440,440,440,440,440,440,440,440,
  440,440,440,440,440,440,440,440,440,440,440,440,440,440,440,440,440,440,
  440,440,440,440,440,1,221,1,440,
441,441,441,441,441,441,441,441,441,441,441,441,441,441,441,441,441,441,441,
  441,441,441,441,441,441,441,441,441,441,441,441,441,441,441,441,441,441,
  441,441,441,441,441,1,222,1,441,
442,442,442,442,442,442,442,442,442,442,442,442,442,442,442,442,442,442,442,
  442,442,442,442,442,442,442,442,442,442,442,442,442,442,442,442,442,442,
  442,442,442,442,442,1,223,1,442,
443,443,443,443,443,443,443,443,443,443,443,443,443,443,443,443,443,443,443,
  443,443,443,443,443,443,443,443,443,443,443,443,443,443,443,443,443,443,
  443,443,443,443,443,1,224,1,443,
444,444,444,444,444,444,444,444,444,444,444,444,444,444,444,444,444,444,444,
  444,444,444,444,444,444,444,444,444,444,444,444,444,444,444,444,444,444,
  444,444,444,444,444,1,225,1,444,
445,445,445,445,445,445,445,445,445,445,445,445,445,445,445,445,445,445,445,
  445,445,445,445,445,445,445,445,445,445,445,445,445,445,445,445,445,445,
  445,445,445,445,445,1,226,1,445,
446,446,446,446,446,446,446,446,446,446,446,446,446,446,446,446,446,446,446,
  446,446,446,446,446,446,446,446,446,446,446,446,446,446,446,446,446,446,
  446,446,446,446,446,1,227,1,446,
447,447,447,447,447,447,447,447,447,447,447,447,447,447,447,447,447,447,447,
  447,447,447,447,447,447,447,447,447,447,447,447,447,447,447,447,447,447,
  447,447,447,447,447,1,228,1,447,
448,448,448,448,448,448,448,448,448,448,448,448,448,448,448,448,448,448,448,
  448,448,448,448,448,448,448,448,448,448,448,448,448,448,448,448,448,448,
  448,448,448,448,448,1,229,1,448,
449,449,449,449,449,449,449,449,449,449,449,449,449,449,449,449,449,449,449,
  449,449,449,449,449,449,449,449,449,449,449,449,449,449,449,449,449,449,
  449,449,449,449,449,1,230,1,449,
450,450,450,450,450,450,450,450,450,450,450,450,450,450,450,450,450,450,450,
  450,450,450,450,450,450,450,450,450,450,450,450,450,450,450,450,450,450,
  450,450,450,450,450,1,231,1,450,
451,451,451,451,451,451,451,451,451,451,451,451,451,451,451,451,451,451,451,
  451,451,451,451,451,451,451,451,451,451,451,451,451,451,451,451,451,451,
  451,451,451,451,451,1,232,1,451,
452,452,452,452,452,452,452,452,452,452,452,452,452,452,452,452,452,452,452,
  452,452,452,452,452,452,452,452,452,452,452,452,452,452,452,452,452,452,
  452,452,452,452,452,1,233,1,452,
453,453,453,453,453,453,453,453,453,453,453,453,453,453,453,453,453,453,453,
  453,453,453,453,453,453,453,453,453,453,453,453,453,453,453,453,453,453,
  453,453,453,453,453,1,234,1,453,
454,454,454,454,454,454,454,454,454,454,454,454,454,454,454,454,454,454,454,
  454,454,454,454,454,454,454,454,454,454,454,454,454,454,454,454,454,454,
  454,454,454,454,454,1,235,1,454,
455,455,455,455,455,455,455,455,455,455,455,455,455,455,455,455,455,455,455,
  455,455,455,455,455,455,455,455,455,455,455,455,455,455,455,455,455,455,
  455,455,455,455,455,1,236,1,455,
456,456,456,456,456,456,456,456,456,456,456,456,456,456,456,456,456,456,456,
  456,456,456,456,456,456,456,456,456,456,456,456,456,456,456,456,456,456,
  456,456,456,456,456,1,237,1,456,
457,457,457,457,457,457,457,457,457,457,457,457,457,457,457,457,457,457,457,
  457,457,457,457,457,457,457,457,457,457,457,457,457,457,457,457,457,457,
  457,457,457,457,457,1,238,1,457,
458,458,458,458,458,458,458,458,458,458,458,458,458,458,458,458,458,458,458,
  458,458,458,458,458,458,458,458,458,458,458,458,458,458,458,458,458,458,
  458,458,458,458,458,1,239,1,458,
459,459,459,459,459,459,459,459,459,459,459,459,459,459,459,459,459,459,459,
  459,459,459,459,459,459,459,459,459,459,459,459,459,459,459,459,459,459,
  459,459,459,459,459,1,240,1,459,
460,460,460,460,460,460,460,460,1,241,1,460,
461,461,461,461,461,461,461,461,1,242,1,461,
462,462,462,462,462,462,462,462,1,243,1,462,
463,463,463,463,463,463,463,463,1,244,1,463,
464,464,464,464,1,245,1,464,
465,465,465,465,1,246,1,465,
466,466,466,466,466,466,466,1,247,1,466,
467,467,467,467,467,467,467,1,248,1,467,
468,468,468,468,468,468,468,1,249,1,468,
469,469,469,469,469,469,469,469,469,1,250,1,469,
470,470,470,470,470,470,470,470,470,1,251,1,470,
471,471,471,471,471,471,471,471,471,1,252,1,471,
472,472,472,472,472,472,472,472,472,1,253,1,472,
473,473,473,473,473,473,473,473,473,1,254,1,473,
474,474,474,474,474,474,474,474,474,1,255,1,474,
475,475,475,475,475,475,475,475,475,1,256,1,475,
476,476,476,476,476,476,476,476,476,1,257,1,476,
477,477,477,477,477,477,477,477,477,1,258,1,477,
478,478,478,478,478,478,478,478,478,1,259,1,478,
479,479,479,479,479,479,479,479,479,1,260,1,479,
480,480,480,480,480,480,480,480,480,1,261,1,480,
481,481,481,481,481,481,481,481,481,1,262,1,481,
19,132,132,132,132,132,132,132,132,132,132,132,19,132,19,19,133,133,133,133,
  133,19,132,19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,
  19,19,19,19,19,19,19,132,19,19,19,19,1,263,1,891,133,
19,19,19,19,19,1,264,1,311,
62,62,62,62,62,62,62,62,62,62,62,62,62,62,62,62,62,62,62,62,62,62,62,62,62,
  62,62,62,62,62,62,62,62,62,62,62,62,62,62,62,62,62,62,62,62,62,62,62,62,
  62,62,62,62,62,62,62,62,62,307,62,62,62,62,62,62,62,62,62,265,482,
116,116,116,116,483,266,483,
484,115,485,
131,131,131,131,131,131,131,131,131,131,131,282,131,250,250,250,250,285,287,
  284,294,296,297,298,299,300,301,302,303,304,320,321,322,283,319,286,295,
  307,281,265,130,129,268,234,292,293,235,288,236,289,265,332,331,329,334,
  330,333,328,335,104,220,323,309,263,336,324,328,327,326,325,219,328,318,
  317,316,315,314,313,312,311,310,308,291,290,263,306,305,
295,486,488,490,491,492,494,495,496,500,502,504,506,508,510,512,514,516,517,
  519,520,521,523,524,526,527,528,529,532,533,14,15,340,342,344,346,348,
  269,71,72,531,73,75,499,498,66,67,530,530,530,530,80,525,525,525,522,
  522,522,522,518,518,518,515,513,511,509,507,505,503,501,497,107,108,493,
  111,112,489,487,117,
534,534,534,534,1,270,1,534,
535,535,535,535,535,535,535,535,535,535,535,535,535,535,535,535,535,535,535,
  535,535,535,535,535,535,535,535,535,535,535,535,535,535,535,535,535,535,
  535,535,535,1,271,1,535,
536,536,536,536,1,272,1,536,
143,537,143,143,143,143,143,143,143,143,143,143,143,143,143,143,143,143,143,
  143,143,143,143,139,143,143,143,143,143,143,143,143,143,143,143,143,143,
  143,143,143,143,143,273,141,
19,19,19,19,19,19,19,19,274,1,884,
382,275,538,
539,539,539,539,1,276,1,539,
540,540,540,540,1,277,1,540,
41,42,43,38,278,55,55,46,55,45,44,44,
41,42,43,38,279,54,54,46,54,45,44,44,
541,541,541,541,1,280,1,541,
276,276,276,276,276,281,
261,261,261,261,282,
259,259,283,
131,131,131,131,131,131,131,255,255,255,255,131,255,255,255,255,255,131,131,
  131,131,131,131,131,131,131,131,131,131,131,131,131,131,131,131,131,131,
  131,131,131,131,131,131,131,131,131,131,131,131,131,131,131,131,131,131,
  131,131,131,284,
250,542,250,250,250,250,250,250,250,250,250,250,250,250,250,250,250,250,250,
  250,250,250,250,250,250,250,250,250,250,250,250,250,250,250,250,250,250,
  250,250,250,250,250,250,250,250,250,250,250,250,250,250,250,250,250,285,
248,248,248,248,248,286,
266,543,266,266,266,266,266,266,266,266,266,266,266,266,266,266,266,266,266,
  266,266,266,266,266,266,266,266,266,266,266,266,266,266,266,266,266,266,
  266,266,266,272,266,287,544,
277,277,277,277,277,274,
241,258,258,258,258,258,258,258,258,258,289,
262,262,262,262,240,240,240,240,240,240,240,240,240,240,240,240,240,240,240,
  240,240,240,240,240,240,240,240,240,240,240,240,240,240,240,240,240,240,
  240,240,240,240,240,290,
260,260,239,239,239,239,239,239,239,239,239,239,239,239,239,239,239,239,239,
  239,239,239,239,239,239,239,239,239,239,239,239,239,239,239,239,239,239,
  239,239,239,291,
256,256,256,256,256,256,256,256,256,238,238,238,238,238,238,238,238,238,238,
  238,238,238,238,238,238,238,238,238,238,238,238,238,238,238,238,238,238,
  238,238,238,238,238,238,238,238,238,238,292,
252,242,257,545,546,257,251,251,251,251,251,253,237,237,237,237,237,237,237,
  237,237,237,237,237,237,237,237,237,237,237,237,237,237,237,237,237,237,
  237,237,237,237,237,237,275,237,237,237,237,246,246,237,293,246,
19,19,294,1,986,
19,19,19,19,19,19,295,1,940,
19,19,296,1,984,
19,19,297,1,983,
19,19,298,1,982,
19,19,299,1,981,
19,19,300,1,980,
19,19,301,1,979,
19,19,302,1,978,
19,19,303,1,977,
19,19,304,1,976,
19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,
  19,19,19,19,19,19,19,19,19,19,19,19,19,305,1,987,
19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,
  19,19,19,19,19,19,19,19,19,19,19,19,1,306,1,941,
19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,
  19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,
  19,19,19,19,19,19,19,19,19,19,19,19,19,307,1,908,
547,1,308,1,547,
548,1,309,1,548,
549,1,310,1,549,
550,1,311,1,550,
551,1,312,1,551,
552,1,313,1,552,
553,1,314,1,553,
554,1,315,1,554,
555,1,316,1,555,
556,1,317,1,556,
557,1,318,1,557,
19,19,19,19,19,19,19,19,19,19,19,19,19,249,249,249,249,249,19,19,19,19,19,
  19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,1,319,1,967,
19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,
  19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,
  19,19,19,19,19,19,19,19,19,19,19,19,320,1,975,
19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,
  19,19,19,19,19,19,19,19,19,19,19,19,19,19,1,321,1,974,
19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,
  19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,
  19,19,19,19,19,19,19,19,19,19,19,19,322,1,973,
558,558,558,558,558,558,558,558,558,558,558,558,558,558,558,558,558,558,558,
  558,558,558,558,558,558,558,558,558,558,558,558,558,558,558,558,558,558,
  558,558,558,558,558,1,323,1,558,
131,131,131,131,131,131,131,131,131,131,131,282,131,250,250,250,250,285,287,
  284,294,296,297,298,299,300,301,302,303,304,283,559,286,295,307,281,265,
  130,129,324,234,292,293,235,288,236,289,265,220,323,309,263,218,219,218,
  318,317,316,315,314,313,312,311,310,308,291,290,263,306,305,
131,131,131,131,131,131,131,131,131,131,131,282,131,250,250,250,250,285,287,
  284,294,296,297,298,299,300,301,302,303,304,283,559,286,295,307,281,265,
  130,129,325,234,292,293,235,288,236,289,265,220,323,309,263,217,219,217,
  318,317,316,315,314,313,312,311,310,308,291,290,263,306,305,
131,131,131,131,131,131,131,131,131,131,131,282,131,250,250,250,250,285,287,
  284,294,296,297,298,299,300,301,302,303,304,283,559,286,295,307,281,265,
  130,129,326,234,292,293,235,288,236,289,265,220,323,309,263,216,219,216,
  318,317,316,315,314,313,312,311,310,308,291,290,263,306,305,
131,131,131,131,131,131,131,131,131,131,131,282,131,250,250,250,250,285,287,
  284,294,296,297,298,299,300,301,302,303,304,283,559,286,295,307,281,265,
  130,129,327,234,292,293,235,288,236,289,265,220,323,309,263,215,219,215,
  318,317,316,315,314,313,312,311,310,308,291,290,263,306,305,
19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,
  19,19,19,19,19,19,19,19,19,19,19,19,1,328,1,208,
19,560,562,388,380,384,19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,
  19,19,19,19,19,19,19,19,19,19,19,19,1,329,1,205,566,565,564,563,561,
19,381,383,19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,
  19,19,19,19,19,19,19,19,1,330,1,200,568,567,
19,569,571,573,575,19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,
  19,19,19,19,19,19,1,331,1,197,576,574,572,570,
19,577,387,19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,
  19,19,1,332,1,194,579,578,
19,580,359,19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,1,
  333,1,191,582,581,
583,357,190,585,584,
586,587,588,589,590,591,592,593,594,595,596,597,396,170,170,170,170,170,335,
  174,174,174,174,177,177,177,180,180,180,183,183,183,186,186,186,189,189,
  189,
131,131,131,131,131,131,131,131,131,131,131,282,131,250,250,250,250,285,287,
  284,294,296,297,298,299,300,301,302,303,304,320,321,322,283,319,286,295,
  307,281,265,130,129,336,234,292,293,235,288,236,289,265,332,331,329,334,
  330,333,328,169,220,323,309,263,324,328,327,326,325,219,328,318,317,316,
  315,314,313,312,311,310,308,291,290,263,306,305,
598,598,598,598,1,337,1,598,
599,599,599,599,1,338,1,599,
600,600,600,600,600,339,600,
19,19,19,19,19,340,1,894,
601,601,601,601,1,341,1,601,
19,19,19,19,19,342,1,893,
602,602,602,602,1,343,1,602,
19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,344,1,892,
603,603,603,603,603,603,603,603,603,603,603,603,603,603,603,1,345,1,603,
19,19,19,19,19,346,1,890,
604,604,604,604,1,347,1,604,
19,19,19,19,19,348,1,888,
605,605,605,605,1,349,1,605,
159,159,606,159,159,159,159,159,159,159,159,159,159,159,159,157,159,159,159,
  159,159,159,159,159,159,159,159,159,159,159,159,159,159,159,159,159,159,
  159,159,159,159,350,
19,19,19,19,19,351,1,885,
607,607,607,607,1,352,1,607,
608,608,608,608,1,353,1,608,
19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,1,354,1,883,
134,134,134,134,134,134,134,134,134,134,134,134,158,134,134,134,134,134,140,
  134,134,355,612,273,611,350,610,274,351,609,
19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,
  19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,356,1,
  1154,
19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,
  19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,
  19,19,19,19,19,19,19,19,19,19,19,19,19,357,1,956,
19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,
  19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,358,1,
  1153,
19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,
  19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,
  19,19,19,19,19,19,19,19,19,19,19,19,19,359,1,958,
19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,
  19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,360,1,1152,
19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,
  19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,361,1,
  1151,
19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,
  19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,362,1,
  1150,
19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,
  19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,363,1,
  1149,
19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,
  19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,364,1,
  1148,
19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,
  19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,365,1,
  1147,
19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,
  19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,366,1,
  1146,
19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,
  19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,367,1,
  1145,
19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,
  19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,368,1,
  1144,
19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,
  19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,369,1,
  1143,
19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,
  19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,370,1,
  1142,
19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,
  19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,371,1,
  1141,
19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,
  19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,372,1,
  1140,
19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,
  19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,373,1,
  1139,
19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,
  19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,374,1,
  1138,
19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,
  19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,375,1,
  1137,
19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,
  19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,376,1,
  1136,
19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,
  19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,377,1,
  1135,
19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,
  19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,378,1,
  1134,
19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,
  19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,379,1,
  1133,
19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,
  19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,
  19,19,19,19,19,19,19,19,19,19,19,19,380,1,969,
19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,
  19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,
  19,19,19,19,19,19,19,19,19,19,19,19,19,381,1,967,
19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,
  19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,
  19,19,19,19,19,19,19,19,19,19,19,19,19,382,1,903,
19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,
  19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,
  19,19,19,19,19,19,19,19,19,19,19,19,19,383,1,966,
19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,
  19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,
  19,19,19,19,19,19,19,19,19,19,19,19,384,1,968,
19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,
  19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,
  19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,385,1,
  907,
19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,
  19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,386,1,
  1132,
19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,
  19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,
  19,19,19,19,19,19,19,19,19,19,19,19,19,387,1,960,
19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,
  19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,
  19,19,19,19,19,19,19,19,19,19,19,19,388,1,970,
19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,
  19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,389,1,
  1131,
19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,
  19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,390,1,
  1130,
19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,
  19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,391,1,
  1129,
19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,
  19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,
  19,19,19,19,19,19,392,1,875,
356,358,360,361,362,363,364,365,366,367,368,369,370,371,372,373,374,375,376,
  377,378,379,386,389,390,391,320,322,388,380,384,381,383,387,359,357,307,
  385,382,39,42,7,9,7,7,7,7,7,7,7,7,7,7,7,7,7,7,7,7,7,7,7,7,7,7,7,7,7,7,7,
  7,7,7,7,7,7,7,7,7,7,7,7,7,7,
38,394,11,
38,395,10,
19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,
  19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,1,396,1,881,
131,131,131,131,131,131,131,131,131,131,131,282,131,250,250,250,250,285,287,
  284,294,296,297,298,299,300,301,302,303,304,320,321,322,283,319,286,295,
  307,281,265,130,129,397,234,292,293,235,288,236,289,265,332,331,329,334,
  330,333,328,613,220,323,309,263,324,328,327,326,325,219,328,318,317,316,
  315,314,313,312,311,310,308,291,290,263,306,305,
295,486,488,490,491,492,494,495,496,500,502,504,506,508,510,512,514,516,517,
  519,520,521,523,524,526,527,528,529,532,533,14,15,340,342,344,346,348,
  614,616,398,617,615,71,72,531,73,75,499,498,66,67,530,530,530,530,80,
  525,525,525,522,522,522,522,518,518,518,515,513,511,509,507,505,503,501,
  497,107,108,493,111,112,489,487,117,
618,618,618,618,1,399,1,618,
619,295,41,42,43,38,400,620,68,68,46,68,45,44,44,167,168,
305,621,622,623,307,306,307,306,305,401,305,306,307,310,397,308,309,
287,286,286,283,282,281,280,279,278,402,395,286,624,
290,297,296,293,296,293,290,403,393,290,293,296,625,
399,399,399,399,404,399,
382,405,626,
382,406,627,
382,407,628,
131,131,131,131,131,131,131,131,131,131,131,282,131,250,250,250,250,285,287,
  284,294,296,297,298,299,300,301,302,303,304,320,321,322,283,319,286,295,
  307,281,265,130,129,408,234,292,293,235,288,236,289,265,332,331,329,334,
  330,333,328,467,220,323,309,263,324,328,327,326,325,219,328,318,317,316,
  315,314,313,312,311,310,308,291,290,263,306,305,
131,131,131,131,131,131,131,131,131,131,131,282,131,250,250,250,250,285,287,
  284,294,296,297,298,299,300,301,302,303,304,320,321,322,283,319,286,295,
  307,281,265,130,129,409,234,292,293,235,288,236,289,265,332,331,329,334,
  330,333,328,466,220,323,309,263,324,328,327,326,325,219,328,318,317,316,
  315,314,313,312,311,310,308,291,290,263,306,305,
131,131,131,131,131,131,131,131,131,131,131,282,131,250,250,250,250,285,287,
  284,294,296,297,298,299,300,301,302,303,304,320,321,322,283,319,286,295,
  307,281,265,130,129,410,234,292,293,235,288,236,289,265,332,331,329,334,
  330,333,328,465,220,323,309,263,324,328,327,326,325,219,328,318,317,316,
  315,314,313,312,311,310,308,291,290,263,306,305,
131,131,131,131,131,131,131,131,131,131,131,282,131,250,250,250,250,285,287,
  284,294,296,297,298,299,300,301,302,303,304,320,321,322,283,319,286,295,
  307,281,265,130,129,411,234,292,293,235,288,236,289,265,332,331,329,334,
  330,333,328,464,220,323,309,263,324,328,327,326,325,219,328,318,317,316,
  315,314,313,312,311,310,308,291,290,263,306,305,
131,131,131,131,131,131,131,131,131,131,131,282,131,250,250,250,250,285,287,
  284,294,296,297,298,299,300,301,302,303,304,320,321,322,283,319,286,295,
  307,281,265,130,129,412,234,292,293,235,288,236,289,265,332,331,329,334,
  330,333,328,463,220,323,309,263,324,328,327,326,325,219,328,318,317,316,
  315,314,313,312,311,310,308,291,290,263,306,305,
131,131,131,131,131,131,131,131,131,131,131,282,131,250,250,250,250,285,287,
  284,294,296,297,298,299,300,301,302,303,304,320,321,322,283,319,286,295,
  307,281,265,130,129,413,234,292,293,235,288,236,289,265,332,331,329,334,
  330,333,328,462,220,323,309,263,324,328,327,326,325,219,328,318,317,316,
  315,314,313,312,311,310,308,291,290,263,306,305,
131,131,131,131,131,131,131,131,131,131,131,282,131,250,250,250,250,285,287,
  284,294,296,297,298,299,300,301,302,303,304,320,321,322,283,319,286,295,
  307,281,265,130,129,414,234,292,293,235,288,236,289,265,332,331,329,334,
  330,333,328,461,220,323,309,263,324,328,327,326,325,219,328,318,317,316,
  315,314,313,312,311,310,308,291,290,263,306,305,
131,131,131,131,131,131,131,131,131,131,131,282,131,250,250,250,250,285,287,
  284,294,296,297,298,299,300,301,302,303,304,320,321,322,283,319,286,295,
  307,281,265,130,129,415,234,292,293,235,288,236,289,265,332,331,329,334,
  330,333,328,460,220,323,309,263,324,328,327,326,325,219,328,318,317,316,
  315,314,313,312,311,310,308,291,290,263,306,305,
131,131,131,131,131,131,131,131,131,131,131,282,131,250,250,250,250,285,287,
  284,294,296,297,298,299,300,301,302,303,304,320,321,322,283,319,286,295,
  307,281,265,130,129,416,234,292,293,235,288,236,289,265,332,331,329,334,
  330,333,328,459,220,323,309,263,324,328,327,326,325,219,328,318,317,316,
  315,314,313,312,311,310,308,291,290,263,306,305,
131,131,131,131,131,131,131,131,131,131,131,282,131,250,250,250,250,285,287,
  284,294,296,297,298,299,300,301,302,303,304,320,321,322,283,319,286,295,
  307,281,265,130,129,417,234,292,293,235,288,236,289,265,332,331,329,334,
  330,333,328,458,220,323,309,263,324,328,327,326,325,219,328,318,317,316,
  315,314,313,312,311,310,308,291,290,263,306,305,
131,131,131,131,131,131,131,131,131,131,131,282,131,250,250,250,250,285,287,
  284,294,296,297,298,299,300,301,302,303,304,320,321,322,283,319,286,295,
  307,281,265,130,129,418,234,292,293,235,288,236,289,265,332,331,329,334,
  330,333,328,457,220,323,309,263,324,328,327,326,325,219,328,318,317,316,
  315,314,313,312,311,310,308,291,290,263,306,305,
131,131,131,131,131,131,131,131,131,131,131,282,131,250,250,250,250,285,287,
  284,294,296,297,298,299,300,301,302,303,304,320,321,322,283,319,286,295,
  307,281,265,130,129,419,234,292,293,235,288,236,289,265,332,331,329,334,
  330,333,328,456,220,323,309,263,324,328,327,326,325,219,328,318,317,316,
  315,314,313,312,311,310,308,291,290,263,306,305,
131,131,131,131,131,131,131,131,131,131,131,282,131,250,250,250,250,285,287,
  284,294,296,297,298,299,300,301,302,303,304,320,321,322,283,319,286,295,
  307,281,265,130,129,420,234,292,293,235,288,236,289,265,332,331,329,334,
  330,333,328,455,220,323,309,263,324,328,327,326,325,219,328,318,317,316,
  315,314,313,312,311,310,308,291,290,263,306,305,
131,131,131,131,131,131,131,131,131,131,131,282,131,250,250,250,250,285,287,
  284,294,296,297,298,299,300,301,302,303,304,320,321,322,283,319,286,295,
  307,281,265,130,129,421,234,292,293,235,288,236,289,265,332,331,329,334,
  330,333,328,454,220,323,309,263,324,328,327,326,325,219,328,318,317,316,
  315,314,313,312,311,310,308,291,290,263,306,305,
131,131,131,131,131,131,131,131,131,131,131,282,131,250,250,250,250,285,287,
  284,294,296,297,298,299,300,301,302,303,304,320,321,322,283,319,286,295,
  307,281,265,130,129,422,234,292,293,235,288,236,289,265,332,331,329,334,
  330,333,328,453,220,323,309,263,324,328,327,326,325,219,328,318,317,316,
  315,314,313,312,311,310,308,291,290,263,306,305,
131,131,131,131,131,131,131,131,131,131,131,282,131,250,250,250,250,285,287,
  284,294,296,297,298,299,300,301,302,303,304,320,321,322,283,319,286,295,
  307,281,265,130,129,423,234,292,293,235,288,236,289,265,332,331,329,334,
  330,333,328,452,220,323,309,263,324,328,327,326,325,219,328,318,317,316,
  315,314,313,312,311,310,308,291,290,263,306,305,
131,131,131,131,131,131,131,131,131,131,131,282,131,250,250,250,250,285,287,
  284,294,296,297,298,299,300,301,302,303,304,320,321,322,283,319,286,295,
  307,281,265,130,129,424,234,292,293,235,288,236,289,265,332,331,329,334,
  330,333,328,451,220,323,309,263,324,328,327,326,325,219,328,318,317,316,
  315,314,313,312,311,310,308,291,290,263,306,305,
131,131,131,131,131,131,131,131,131,131,131,282,131,250,250,250,250,285,287,
  284,294,296,297,298,299,300,301,302,303,304,320,321,322,283,319,286,295,
  307,281,265,130,129,425,234,292,293,235,288,236,289,265,332,331,329,334,
  330,333,328,450,220,323,309,263,324,328,327,326,325,219,328,318,317,316,
  315,314,313,312,311,310,308,291,290,263,306,305,
131,131,131,131,131,131,131,131,131,131,131,282,131,250,250,250,250,285,287,
  284,294,296,297,298,299,300,301,302,303,304,320,321,322,283,319,286,295,
  307,281,265,130,129,426,234,292,293,235,288,236,289,265,332,331,329,334,
  330,333,328,449,220,323,309,263,324,328,327,326,325,219,328,318,317,316,
  315,314,313,312,311,310,308,291,290,263,306,305,
131,131,131,131,131,131,131,131,131,131,131,282,131,250,250,250,250,285,287,
  284,294,296,297,298,299,300,301,302,303,304,320,321,322,283,319,286,295,
  307,281,265,130,129,427,234,292,293,235,288,236,289,265,332,331,329,334,
  330,333,328,448,220,323,309,263,324,328,327,326,325,219,328,318,317,316,
  315,314,313,312,311,310,308,291,290,263,306,305,
131,131,131,131,131,131,131,131,131,131,131,282,131,250,250,250,250,285,287,
  284,294,296,297,298,299,300,301,302,303,304,320,321,322,283,319,286,295,
  307,281,265,130,129,428,234,292,293,235,288,236,289,265,332,331,329,334,
  330,333,328,447,220,323,309,263,324,328,327,326,325,219,328,318,317,316,
  315,314,313,312,311,310,308,291,290,263,306,305,
131,131,131,131,131,131,131,131,131,131,131,282,131,250,250,250,250,285,287,
  284,294,296,297,298,299,300,301,302,303,304,320,321,322,283,319,286,295,
  307,281,265,130,129,429,234,292,293,235,288,236,289,265,332,331,329,334,
  330,333,328,446,220,323,309,263,324,328,327,326,325,219,328,318,317,316,
  315,314,313,312,311,310,308,291,290,263,306,305,
131,131,131,131,131,131,131,131,131,131,131,282,131,250,250,250,250,285,287,
  284,294,296,297,298,299,300,301,302,303,304,320,321,322,283,319,286,295,
  307,281,265,130,129,430,234,292,293,235,288,236,289,265,332,331,329,334,
  330,333,328,445,220,323,309,263,324,328,327,326,325,219,328,318,317,316,
  315,314,313,312,311,310,308,291,290,263,306,305,
131,131,131,131,131,131,131,131,131,131,131,282,131,250,250,250,250,285,287,
  284,294,296,297,298,299,300,301,302,303,304,320,321,322,283,319,286,295,
  307,281,265,130,129,431,234,292,293,235,288,236,289,265,332,331,329,334,
  330,333,328,444,220,323,309,263,324,328,327,326,325,219,328,318,317,316,
  315,314,313,312,311,310,308,291,290,263,306,305,
131,131,131,131,131,131,131,131,131,131,131,282,131,250,250,250,250,285,287,
  284,294,296,297,298,299,300,301,302,303,304,320,321,322,283,319,286,295,
  307,281,265,130,129,432,234,292,293,235,288,236,289,265,332,331,329,334,
  330,333,328,443,220,323,309,263,324,328,327,326,325,219,328,318,317,316,
  315,314,313,312,311,310,308,291,290,263,306,305,
131,131,131,131,131,131,131,131,131,131,131,282,131,250,250,250,250,285,287,
  284,294,296,297,298,299,300,301,302,303,304,320,321,322,283,319,286,295,
  307,281,265,130,129,433,234,292,293,235,288,236,289,265,332,331,329,334,
  330,333,328,439,220,323,309,263,324,328,327,326,325,219,328,318,317,316,
  315,314,313,312,311,310,308,291,290,263,306,305,
131,131,131,131,131,131,131,131,131,131,131,282,131,250,250,250,250,285,287,
  284,294,296,297,298,299,300,301,302,303,304,320,321,322,283,319,286,295,
  307,281,265,130,129,434,234,292,293,235,288,236,289,265,332,331,329,334,
  330,333,328,435,220,323,309,263,324,328,327,326,325,219,328,318,317,316,
  315,314,313,312,311,310,308,291,290,263,306,305,
131,131,131,131,131,131,131,131,131,131,131,282,131,250,250,250,250,285,287,
  284,294,296,297,298,299,300,301,302,303,304,320,321,322,283,319,286,295,
  307,281,265,130,129,435,234,292,293,235,288,236,289,265,332,331,329,334,
  330,333,328,434,220,323,309,263,324,328,327,326,325,219,328,318,317,316,
  315,314,313,312,311,310,308,291,290,263,306,305,
131,131,131,131,131,131,131,131,131,131,131,282,131,250,250,250,250,285,287,
  284,294,296,297,298,299,300,301,302,303,304,320,321,322,283,319,286,295,
  307,281,265,130,129,436,234,292,293,235,288,236,289,265,332,331,329,334,
  330,333,328,433,220,323,309,263,324,328,327,326,325,219,328,318,317,316,
  315,314,313,312,311,310,308,291,290,263,306,305,
131,131,131,131,131,131,131,131,131,131,131,282,131,250,250,250,250,285,287,
  284,294,296,297,298,299,300,301,302,303,304,320,321,322,283,319,286,295,
  307,281,265,130,129,437,234,292,293,235,288,236,289,265,332,331,329,334,
  330,333,328,432,220,323,309,263,324,328,327,326,325,219,328,318,317,316,
  315,314,313,312,311,310,308,291,290,263,306,305,
131,131,131,131,131,131,131,131,131,131,131,282,131,250,250,250,250,285,287,
  284,294,296,297,298,299,300,301,302,303,304,320,321,322,283,319,286,295,
  307,281,265,130,129,438,234,292,293,235,288,236,289,265,332,331,329,334,
  330,333,328,431,220,323,309,263,324,328,327,326,325,219,328,318,317,316,
  315,314,313,312,311,310,308,291,290,263,306,305,
131,131,131,131,131,131,131,131,131,131,131,282,131,250,250,250,250,285,287,
  284,294,296,297,298,299,300,301,302,303,304,320,321,322,283,319,286,295,
  307,281,265,130,129,439,234,292,293,235,288,236,289,265,332,331,329,334,
  330,333,328,430,220,323,309,263,324,328,327,326,325,219,328,318,317,316,
  315,314,313,312,311,310,308,291,290,263,306,305,
131,131,131,131,131,131,131,131,131,131,131,282,131,250,250,250,250,285,287,
  284,294,296,297,298,299,300,301,302,303,304,320,321,322,283,319,286,295,
  307,281,265,130,129,440,234,292,293,235,288,236,289,265,332,331,329,334,
  330,333,328,429,220,323,309,263,324,328,327,326,325,219,328,318,317,316,
  315,314,313,312,311,310,308,291,290,263,306,305,
131,131,131,131,131,131,131,131,131,131,131,282,131,250,250,250,250,285,287,
  284,294,296,297,298,299,300,301,302,303,304,320,321,322,283,319,286,295,
  307,281,265,130,129,441,234,292,293,235,288,236,289,265,332,331,329,334,
  330,333,328,423,220,323,309,263,324,328,327,326,325,219,328,318,317,316,
  315,314,313,312,311,310,308,291,290,263,306,305,
131,131,131,131,131,131,131,131,131,131,131,282,131,250,250,250,250,285,287,
  284,294,296,297,298,299,300,301,302,303,304,320,321,322,283,319,286,295,
  307,281,265,130,129,442,234,292,293,235,288,236,289,265,332,331,329,334,
  330,333,328,422,220,323,309,263,324,328,327,326,325,219,328,318,317,316,
  315,314,313,312,311,310,308,291,290,263,306,305,
131,131,131,131,131,131,131,131,131,131,131,282,131,250,250,250,250,285,287,
  284,294,296,297,298,299,300,301,302,303,304,320,321,322,283,319,286,295,
  307,281,265,130,129,443,234,292,293,235,288,236,289,265,332,331,329,334,
  330,333,328,421,220,323,309,263,324,328,327,326,325,219,328,318,317,316,
  315,314,313,312,311,310,308,291,290,263,306,305,
131,131,131,131,131,131,131,131,131,131,131,282,131,250,250,250,250,285,287,
  284,294,296,297,298,299,300,301,302,303,304,320,321,322,283,319,286,295,
  307,281,265,130,129,444,234,292,293,235,288,236,289,265,332,331,329,334,
  330,333,328,420,220,323,309,263,324,328,327,326,325,219,328,318,317,316,
  315,314,313,312,311,310,308,291,290,263,306,305,
131,131,131,131,131,131,131,131,131,131,131,282,131,250,250,250,250,285,287,
  284,294,296,297,298,299,300,301,302,303,304,320,321,322,283,319,286,295,
  307,281,265,130,129,445,234,292,293,235,288,236,289,265,332,331,329,334,
  330,333,328,414,220,323,309,263,324,328,327,326,325,219,328,318,317,316,
  315,314,313,312,311,310,308,291,290,263,306,305,
131,131,131,131,131,131,131,131,131,131,131,282,131,250,250,250,250,285,287,
  284,294,296,297,298,299,300,301,302,303,304,320,321,322,283,319,286,295,
  307,281,265,130,129,446,234,292,293,235,288,236,289,265,332,331,329,334,
  330,333,328,413,220,323,309,263,324,328,327,326,325,219,328,318,317,316,
  315,314,313,312,311,310,308,291,290,263,306,305,
131,131,131,131,131,131,131,131,131,131,131,282,131,250,250,250,250,285,287,
  284,294,296,297,298,299,300,301,302,303,304,320,321,322,283,319,286,295,
  307,281,265,130,129,447,234,292,293,235,288,236,289,265,332,331,329,334,
  330,333,328,412,220,323,309,263,324,328,327,326,325,219,328,318,317,316,
  315,314,313,312,311,310,308,291,290,263,306,305,
131,131,131,131,131,131,131,131,131,131,131,282,131,250,250,250,250,285,287,
  284,294,296,297,298,299,300,301,302,303,304,320,321,322,283,319,286,295,
  307,281,265,130,129,448,234,292,293,235,288,236,289,265,332,331,329,334,
  330,333,328,411,220,323,309,263,324,328,327,326,325,219,328,318,317,316,
  315,314,313,312,311,310,308,291,290,263,306,305,
131,131,131,131,131,131,131,131,131,131,131,282,131,250,250,250,250,285,287,
  284,294,296,297,298,299,300,301,302,303,304,320,321,322,283,319,286,295,
  307,281,265,130,129,449,234,292,293,235,288,236,289,265,332,331,329,334,
  330,333,328,410,220,323,309,263,324,328,327,326,325,219,328,318,317,316,
  315,314,313,312,311,310,308,291,290,263,306,305,
131,131,131,131,131,131,131,131,131,131,131,282,131,250,250,250,250,285,287,
  284,294,296,297,298,299,300,301,302,303,304,320,321,322,283,319,286,295,
  307,281,265,130,129,450,234,292,293,235,288,236,289,265,332,331,329,334,
  330,333,328,409,220,323,309,263,324,328,327,326,325,219,328,318,317,316,
  315,314,313,312,311,310,308,291,290,263,306,305,
131,131,131,131,131,131,131,131,131,131,131,282,131,250,250,250,250,285,287,
  284,294,296,297,298,299,300,301,302,303,304,320,321,322,283,319,286,295,
  307,281,265,130,129,451,234,292,293,235,288,236,289,265,332,331,329,334,
  330,333,328,408,220,323,309,263,324,328,327,326,325,219,328,318,317,316,
  315,314,313,312,311,310,308,291,290,263,306,305,
131,131,131,131,131,131,131,131,131,131,131,282,131,250,250,250,250,285,287,
  284,294,296,297,298,299,300,301,302,303,304,320,321,322,283,319,286,295,
  307,281,265,130,129,452,234,292,293,235,288,236,289,265,332,331,329,334,
  330,333,328,407,220,323,309,263,324,328,327,326,325,219,328,318,317,316,
  315,314,313,312,311,310,308,291,290,263,306,305,
131,131,131,131,131,131,131,131,131,131,131,282,131,250,250,250,250,285,287,
  284,294,296,297,298,299,300,301,302,303,304,320,321,322,283,319,286,295,
  307,281,265,130,129,453,234,292,293,235,288,236,289,265,332,331,329,334,
  330,333,328,406,220,323,309,263,324,328,327,326,325,219,328,318,317,316,
  315,314,313,312,311,310,308,291,290,263,306,305,
131,131,131,131,131,131,131,131,131,131,131,282,131,250,250,250,250,285,287,
  284,294,296,297,298,299,300,301,302,303,304,320,321,322,283,319,286,295,
  307,281,265,130,129,454,234,292,293,235,288,236,289,265,332,331,329,334,
  330,333,328,405,220,323,309,263,324,328,327,326,325,219,328,318,317,316,
  315,314,313,312,311,310,308,291,290,263,306,305,
131,131,131,131,131,131,131,131,131,131,131,282,131,250,250,250,250,285,287,
  284,294,296,297,298,299,300,301,302,303,304,320,321,322,283,319,286,295,
  307,281,265,130,129,455,234,292,293,235,288,236,289,265,332,331,329,334,
  330,333,328,404,220,323,309,263,324,328,327,326,325,219,328,318,317,316,
  315,314,313,312,311,310,308,291,290,263,306,305,
131,131,131,131,131,131,131,131,131,131,131,282,131,250,250,250,250,285,287,
  284,294,296,297,298,299,300,301,302,303,304,320,321,322,283,319,286,295,
  307,281,265,130,129,456,234,292,293,235,288,236,289,265,332,331,329,334,
  330,333,328,403,220,323,309,263,324,328,327,326,325,219,328,318,317,316,
  315,314,313,312,311,310,308,291,290,263,306,305,
131,131,131,131,131,131,131,131,131,131,131,282,131,250,250,250,250,285,287,
  284,294,296,297,298,299,300,301,302,303,304,320,321,322,283,319,286,295,
  307,281,265,130,129,457,234,292,293,235,288,236,289,265,332,331,329,334,
  330,333,328,402,220,323,309,263,324,328,327,326,325,219,328,318,317,316,
  315,314,313,312,311,310,308,291,290,263,306,305,
131,131,131,131,131,131,131,131,131,131,131,282,131,250,250,250,250,285,287,
  284,294,296,297,298,299,300,301,302,303,304,320,321,322,283,319,286,295,
  307,281,265,130,129,458,234,292,293,235,288,236,289,265,332,331,329,334,
  330,333,328,401,220,323,309,263,324,328,327,326,325,219,328,318,317,316,
  315,314,313,312,311,310,308,291,290,263,306,305,
131,131,131,131,131,131,131,131,131,131,131,282,131,250,250,250,250,285,287,
  284,294,296,297,298,299,300,301,302,303,304,320,321,322,283,319,286,295,
  307,281,265,130,129,459,234,292,293,235,288,236,289,265,332,331,329,334,
  330,333,328,400,220,323,309,263,324,328,327,326,325,219,328,318,317,316,
  315,314,313,312,311,310,308,291,290,263,306,305,
298,629,630,300,299,300,299,298,460,302,298,299,300,376,301,
298,629,630,300,299,300,299,298,461,302,298,299,300,375,301,
298,629,630,300,299,300,299,298,462,302,298,299,300,374,301,
298,629,630,300,299,300,299,298,463,302,298,299,300,373,301,
303,304,304,303,464,303,304,372,
303,304,304,303,465,303,304,371,
290,297,296,293,296,293,290,466,392,290,293,296,625,
290,297,296,293,296,293,290,467,391,290,293,296,625,
290,297,296,293,296,293,290,468,390,290,293,296,625,
305,621,622,623,307,306,307,306,305,469,305,306,307,310,389,308,309,
305,621,622,623,307,306,307,306,305,470,305,306,307,310,388,308,309,
287,286,286,283,282,281,280,279,278,471,387,286,624,
287,286,286,283,282,281,280,279,278,472,386,286,624,
287,286,286,283,282,281,280,279,278,473,385,286,624,
287,286,286,283,282,281,280,279,278,474,384,286,624,
287,286,286,283,282,281,280,279,278,475,631,286,624,
287,286,286,283,282,281,280,279,278,476,382,286,624,
287,286,286,283,282,281,280,279,278,477,381,286,624,
287,286,286,283,282,281,280,279,278,478,380,286,624,
287,286,286,283,282,281,280,279,278,479,379,286,624,
287,286,286,283,282,281,280,279,278,480,378,286,624,
287,286,286,283,282,281,280,279,278,481,377,286,624,
131,131,131,131,131,131,131,131,131,131,131,282,131,250,250,250,250,285,632,
  284,140,294,296,297,298,299,300,301,302,303,304,320,321,322,283,319,286,
  295,307,281,265,130,129,482,234,292,293,235,288,119,273,236,289,633,120,
  265,332,331,329,334,330,333,328,121,220,323,635,309,263,324,328,327,326,
  325,219,328,318,317,316,315,314,313,312,311,310,308,291,290,274,263,634,
  306,305,
282,250,250,250,250,285,287,636,283,559,286,265,18,483,118,292,637,236,289,
  291,290,306,
19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,
  19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,
  19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,
  19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,
  19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,
  19,19,19,19,19,19,19,19,19,19,19,19,19,484,1,993,
638,638,638,638,638,638,638,638,638,638,638,638,638,638,638,638,638,638,638,
  638,638,638,638,638,638,638,638,638,638,638,638,638,638,638,638,638,638,
  638,638,638,638,638,638,638,638,638,638,638,638,638,638,638,638,638,638,
  638,638,638,638,638,638,638,638,638,638,638,638,638,638,638,638,638,638,
  638,638,638,638,638,638,638,638,638,638,638,638,638,638,638,638,638,638,
  638,638,638,638,638,638,638,638,638,638,638,638,638,638,638,638,638,638,
  638,638,638,638,638,638,638,638,638,638,638,638,638,638,638,638,638,638,
  638,638,638,638,638,638,1,485,1,638,
19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,1,486,1,939,
131,131,131,131,131,131,131,131,131,131,131,131,131,130,129,487,114,263,263,
19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,1,488,1,938,
134,134,134,134,134,134,134,134,134,134,134,134,134,134,134,134,134,134,134,
  489,113,609,
19,19,19,19,19,490,1,937,
19,19,19,19,19,491,1,936,
19,19,1,492,1,935,
639,140,493,110,273,633,109,274,634,
19,19,19,19,19,494,1,933,
19,19,19,19,19,495,1,932,
19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,496,1,931,
131,131,131,131,131,131,131,131,131,131,131,131,131,102,102,102,102,130,129,
  102,497,103,263,263,
140,498,640,273,274,
140,499,100,273,274,
19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,
  19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,500,1,930,
641,641,641,641,641,641,641,641,641,641,641,641,641,641,641,641,641,641,641,
  641,641,641,641,641,641,641,641,641,641,641,641,641,641,641,641,641,641,
  641,641,641,641,641,641,1,501,1,641,
19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,
  19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,1,502,1,929,
131,131,131,131,131,131,131,131,131,131,131,282,131,250,250,250,250,285,287,
  284,140,294,296,297,298,299,300,301,302,303,304,320,321,322,283,319,286,
  295,307,281,265,130,129,503,234,292,293,235,288,98,273,236,289,265,332,
  331,329,334,330,333,328,97,220,323,309,263,324,328,327,326,325,219,328,
  318,317,316,315,314,313,312,311,310,308,291,290,274,263,306,305,
19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,
  19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,1,504,1,928,
131,131,131,131,131,131,131,131,131,131,131,282,131,250,250,250,250,285,287,
  284,294,296,297,298,299,300,301,302,303,304,320,321,322,283,319,286,295,
  307,281,265,130,129,505,234,292,293,235,288,236,289,265,332,331,329,334,
  330,333,328,96,220,323,309,263,324,328,327,326,325,219,328,318,317,316,
  315,314,313,312,311,310,308,291,290,263,306,305,
19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,1,506,1,927,
131,131,131,131,131,131,131,131,131,131,131,131,131,130,129,507,95,263,263,
19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,1,508,1,926,
131,131,131,131,131,131,131,131,131,131,131,131,131,130,129,509,94,263,263,
19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,1,510,1,925,
131,131,131,131,131,131,131,131,131,131,131,131,131,130,129,511,125,642,263,
  263,
19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,1,512,1,924,
131,131,131,131,131,131,131,131,131,131,131,131,131,130,129,513,125,643,263,
  263,
19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,1,514,1,923,
131,131,131,131,131,131,131,131,131,131,131,131,131,130,129,515,125,644,263,
  263,
19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,
  19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,1,516,1,922,
19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,
  19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,1,517,1,921,
131,131,131,131,131,131,131,131,131,131,131,282,131,250,250,250,250,285,632,
  284,140,294,296,297,298,299,300,301,302,303,304,320,321,322,283,319,286,
  295,307,281,265,130,129,518,234,292,293,235,288,119,273,236,289,633,120,
  265,332,331,329,334,330,333,328,121,220,323,645,309,263,324,328,327,326,
  325,219,328,318,317,316,315,314,313,312,311,310,308,291,290,274,263,634,
  306,305,
19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,
  19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,1,519,1,920,
19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,
  19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,1,520,1,919,
19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,
  19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,1,521,1,918,
131,131,131,131,131,131,131,131,131,131,131,282,131,250,250,250,250,285,632,
  284,140,294,296,297,298,299,300,301,302,303,304,320,321,322,283,319,286,
  295,307,281,265,130,129,522,234,292,293,235,288,119,273,236,289,633,120,
  265,332,331,329,334,330,333,328,121,220,323,646,309,263,324,328,327,326,
  325,219,328,318,317,316,315,314,313,312,311,310,308,291,290,274,263,634,
  306,305,
19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,
  19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,1,523,1,917,
19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,
  19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,1,524,1,916,
131,131,131,131,131,131,131,131,131,131,131,282,131,250,250,250,250,285,287,
  284,294,296,297,298,299,300,301,302,303,304,320,321,322,283,319,286,295,
  307,281,265,130,129,525,234,292,293,235,288,236,289,265,332,331,329,334,
  330,333,328,83,220,323,309,263,324,328,327,326,325,219,328,318,317,316,
  315,314,313,312,311,310,308,291,290,263,306,305,
19,19,19,19,19,526,1,915,
19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,
  19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,1,527,1,914,
19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,
  19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,1,528,1,913,
19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,
  19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,1,529,1,912,
131,131,131,131,131,131,131,131,131,131,131,282,131,250,250,250,250,285,287,
  284,294,296,297,298,299,300,301,302,303,304,320,321,322,283,319,286,295,
  307,281,265,130,129,530,234,292,293,235,288,236,289,265,332,331,329,334,
  330,333,328,79,220,323,309,263,324,328,327,326,325,219,328,318,317,316,
  315,314,313,312,311,310,308,291,290,263,306,305,
131,131,131,131,131,131,131,131,131,131,131,131,131,130,129,531,74,263,263,
19,19,19,19,19,19,19,532,1,911,
19,19,19,19,19,19,19,533,1,910,
41,42,43,38,534,21,21,46,21,45,44,44,
356,358,361,362,363,364,365,366,367,368,369,370,371,372,373,374,375,376,377,
  378,379,386,389,390,391,320,322,388,380,384,381,383,387,359,357,307,385,
  382,39,42,535,63,63,63,647,63,63,63,63,63,63,63,63,63,63,63,63,63,63,63,
  63,63,63,63,63,63,63,63,63,63,63,63,63,63,63,63,63,63,63,63,63,63,
41,42,43,38,536,59,59,46,59,45,44,44,
144,648,146,147,145,151,150,649,649,650,648,149,142,537,648,
651,651,651,651,651,651,651,651,651,651,651,651,651,651,651,651,651,651,651,
  651,651,651,651,651,651,651,651,651,651,651,651,651,651,651,651,651,651,
  651,651,651,651,651,1,538,1,651,
41,42,43,38,539,57,57,46,57,45,44,44,
41,42,43,38,540,56,56,46,56,45,44,44,
41,42,43,38,541,53,53,46,53,45,44,44,
254,254,254,254,254,254,254,254,254,542,
267,269,270,268,271,273,543,
266,543,266,266,266,266,266,266,266,266,266,266,266,266,266,266,266,266,266,
  266,266,263,266,266,266,266,266,266,266,266,266,266,266,266,266,266,266,
  266,266,266,266,272,266,544,652,
257,257,257,257,257,257,257,257,257,257,247,247,247,247,247,247,247,247,247,
  247,247,247,247,247,247,247,247,247,247,247,247,247,247,247,247,247,247,
  247,247,247,247,247,247,247,247,247,247,247,545,
257,257,257,257,257,257,257,257,257,257,243,243,243,243,243,243,243,243,243,
  243,243,243,243,243,243,243,243,243,243,243,243,243,243,243,243,243,243,
  243,243,243,243,243,243,243,243,243,243,243,546,
307,547,653,
307,548,654,
307,549,655,
307,550,656,
307,551,657,
307,552,658,
307,553,659,
307,554,660,
307,555,661,
307,556,662,
307,557,663,
131,131,131,131,131,131,131,131,131,131,131,282,131,250,250,250,250,285,287,
  284,294,296,297,298,299,300,301,302,303,304,320,321,322,283,319,286,295,
  307,281,265,130,129,558,234,292,293,235,288,236,289,265,332,331,329,334,
  330,333,328,664,220,323,309,263,324,328,327,326,325,219,328,318,317,316,
  315,314,313,312,311,310,308,291,290,263,306,305,
249,249,249,249,249,559,
19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,
  19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,560,1,972,
665,665,665,665,665,665,665,665,665,665,665,665,665,665,665,665,665,665,665,
  665,665,665,665,665,665,665,665,665,665,665,665,665,665,665,665,665,665,
  665,665,1,561,1,665,
19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,
  19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,562,1,971,
666,666,666,666,666,666,666,666,666,666,666,666,666,666,666,666,666,666,666,
  666,666,666,666,666,666,666,666,666,666,666,666,666,666,666,666,666,666,
  666,666,1,563,1,666,
667,667,667,667,667,667,667,667,667,667,667,667,667,667,667,667,667,667,667,
  667,667,667,667,667,667,667,667,667,667,667,667,667,667,667,667,667,667,
  667,667,1,564,1,667,
668,668,668,668,668,668,668,668,668,668,668,668,668,668,668,668,668,668,668,
  668,668,668,668,668,668,668,668,668,668,668,668,668,668,668,668,668,668,
  668,668,1,565,1,668,
669,669,669,669,669,669,669,669,669,669,669,669,669,669,669,669,669,669,669,
  669,669,669,669,669,669,669,669,669,669,669,669,669,669,669,669,669,669,
  669,669,1,566,1,669,
670,670,670,670,670,670,670,670,670,670,670,670,670,670,670,670,670,670,670,
  670,670,670,670,670,670,670,670,670,670,670,670,670,670,670,670,670,670,
  670,670,670,670,670,1,567,1,670,
671,671,671,671,671,671,671,671,671,671,671,671,671,671,671,671,671,671,671,
  671,671,671,671,671,671,671,671,671,671,671,671,671,671,671,671,671,671,
  671,671,671,671,671,1,568,1,671,
19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,
  19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,569,1,965,
672,672,672,672,672,672,672,672,672,672,672,672,672,672,672,672,672,672,672,
  672,672,672,672,672,672,672,672,672,672,672,672,672,672,672,672,672,672,
  672,672,672,672,672,1,570,1,672,
19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,
  19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,571,1,964,
673,673,673,673,673,673,673,673,673,673,673,673,673,673,673,673,673,673,673,
  673,673,673,673,673,673,673,673,673,673,673,673,673,673,673,673,673,673,
  673,673,673,673,673,1,572,1,673,
19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,
  19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,573,1,963,
674,674,674,674,674,674,674,674,674,674,674,674,674,674,674,674,674,674,674,
  674,674,674,674,674,674,674,674,674,674,674,674,674,674,674,674,674,674,
  674,674,674,674,674,1,574,1,674,
19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,
  19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,575,1,962,
675,675,675,675,675,675,675,675,675,675,675,675,675,675,675,675,675,675,675,
  675,675,675,675,675,675,675,675,675,675,675,675,675,675,675,675,675,675,
  675,675,675,675,675,1,576,1,675,
19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,
  19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,577,1,961,
676,676,676,676,676,676,676,676,676,676,676,676,676,676,676,676,676,676,676,
  676,676,676,676,676,676,676,676,676,676,676,676,676,676,676,676,676,676,
  676,676,676,676,676,1,578,1,676,
677,677,677,677,677,677,677,677,677,677,677,677,677,677,677,677,677,677,677,
  677,677,677,677,677,677,677,677,677,677,677,677,677,677,677,677,677,677,
  677,677,677,677,677,1,579,1,677,
19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,
  19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,580,1,959,
678,678,678,678,678,678,678,678,678,678,678,678,678,678,678,678,678,678,678,
  678,678,678,678,678,678,678,678,678,678,678,678,678,678,678,678,678,678,
  678,678,678,678,678,1,581,1,678,
679,679,679,679,679,679,679,679,679,679,679,679,679,679,679,679,679,679,679,
  679,679,679,679,679,679,679,679,679,679,679,679,679,679,679,679,679,679,
  679,679,679,679,679,1,582,1,679,
19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,
  19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,583,1,957,
680,680,680,680,680,680,680,680,680,680,680,680,680,680,680,680,680,680,680,
  680,680,680,680,680,680,680,680,680,680,680,680,680,680,680,680,680,680,
  680,680,680,680,680,1,584,1,680,
681,681,681,681,681,681,681,681,681,681,681,681,681,681,681,681,681,681,681,
  681,681,681,681,681,681,681,681,681,681,681,681,681,681,681,681,681,681,
  681,681,681,681,681,1,585,1,681,
19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,
  19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,1,586,1,955,
19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,
  19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,1,587,1,954,
19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,
  19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,1,588,1,953,
19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,
  19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,1,589,1,952,
19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,
  19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,1,590,1,951,
19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,
  19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,1,591,1,950,
19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,
  19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,1,592,1,949,
19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,
  19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,1,593,1,948,
19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,
  19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,1,594,1,947,
19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,
  19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,1,595,1,946,
19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,
  19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,1,596,1,945,
19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,
  19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,1,597,1,944,
41,42,43,38,598,52,52,46,52,45,44,44,
41,42,43,38,599,51,51,46,51,45,44,44,
41,42,43,38,600,50,50,46,50,45,44,44,
41,42,43,38,601,47,47,46,47,45,44,44,
41,42,43,38,602,46,46,46,46,45,44,44,
131,131,131,131,131,131,131,131,131,131,131,131,131,130,129,603,682,263,263,
41,42,43,38,604,44,44,46,44,45,44,44,
41,42,43,38,605,43,43,46,43,45,44,44,
160,160,160,160,160,160,160,160,160,160,160,160,160,160,160,160,160,160,160,
  160,160,160,160,160,160,160,160,160,160,160,160,160,160,160,160,160,160,
  160,160,160,160,161,160,606,
41,42,43,38,607,42,42,46,42,45,44,44,
41,42,43,38,608,41,41,46,41,45,44,44,
135,135,135,135,135,135,135,135,135,135,135,135,135,135,135,135,135,135,135,
  19,19,19,609,1,886,
683,683,683,683,1,610,1,683,
684,684,684,684,1,611,1,684,
685,685,685,685,1,612,1,685,
686,686,686,686,1,613,1,686,
19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,
  19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,1,614,1,880,
131,131,131,131,131,131,131,131,131,131,131,282,131,250,250,250,250,285,287,
  284,294,296,297,298,299,300,301,302,303,304,320,321,322,283,319,286,295,
  307,281,265,130,129,615,234,292,293,235,288,236,289,265,332,331,329,334,
  330,333,328,687,220,323,309,263,324,328,327,326,325,219,328,318,317,316,
  315,314,313,312,311,310,308,291,290,263,306,305,
19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,
  19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,1,616,1,879,
131,131,131,131,131,131,131,131,131,131,131,282,131,250,250,250,250,285,287,
  284,294,296,297,298,299,300,301,302,303,304,320,321,322,283,319,286,295,
  307,281,265,130,129,617,234,292,293,235,288,236,289,265,332,331,329,334,
  330,333,328,688,220,323,309,263,324,328,327,326,325,219,328,318,317,316,
  315,314,313,312,311,310,308,291,290,263,306,305,
41,42,43,38,618,22,22,46,22,45,44,44,
19,19,19,19,19,619,1,943,
689,689,689,689,1,620,1,689,
19,19,19,19,19,19,19,621,1,992,
19,19,19,19,19,19,19,622,1,991,
19,19,19,19,19,19,19,623,1,990,
19,19,19,19,19,19,19,19,624,1,1043,
19,19,19,19,19,19,19,625,1,1057,
690,690,690,690,690,690,690,690,690,690,690,690,690,690,690,690,690,690,690,
  690,690,690,690,690,690,690,690,690,690,690,690,690,690,690,690,690,690,
  690,690,690,690,690,1,626,1,690,
691,691,691,691,691,691,691,691,691,691,691,691,691,691,691,691,691,691,691,
  691,691,691,691,691,691,691,691,691,691,691,691,691,691,691,691,691,691,
  691,691,691,691,691,1,627,1,691,
692,692,692,692,692,692,692,692,692,692,692,692,692,692,692,692,692,692,692,
  692,692,692,692,692,692,692,692,692,692,692,692,692,692,692,692,692,692,
  692,692,692,692,692,1,628,1,692,
19,19,19,19,19,19,629,1,989,
19,19,19,19,19,19,630,1,988,
693,1,631,1,693,
266,543,266,266,266,266,266,266,266,266,266,266,266,266,266,266,266,266,266,
  266,266,266,266,266,266,266,266,266,266,266,266,266,266,266,266,266,266,
  266,266,266,272,266,632,694,
266,543,266,266,266,266,266,266,266,266,266,266,266,266,266,266,266,266,266,
  266,266,136,266,266,266,266,266,266,266,266,266,266,266,266,266,266,266,
  266,266,266,266,272,266,633,138,
19,19,19,19,19,19,19,634,1,934,
695,385,635,61,696,
255,255,255,255,255,255,255,255,255,636,
252,242,257,545,546,257,251,251,251,251,251,253,237,237,237,237,246,246,237,
  637,246,
115,116,117,118,119,120,121,122,123,124,125,126,127,128,129,130,131,132,133,
  134,135,136,137,138,66,67,68,69,70,71,139,140,141,142,143,144,72,73,74,
  75,76,145,146,147,77,78,79,80,81,148,149,150,151,152,153,154,155,156,
  157,158,159,160,161,162,111,63,64,65,169,170,171,172,173,174,175,176,
  177,178,179,180,181,182,183,184,163,164,165,166,167,168,82,83,84,85,86,
  87,49,50,51,88,89,90,91,92,93,94,95,96,52,53,97,98,99,100,101,102,103,
  104,54,55,56,105,106,57,58,107,108,109,110,59,60,61,62,638,697,698,200,
  698,698,698,698,698,698,698,698,698,698,327,327,327,327,327,328,329,330,
  331,334,334,334,335,336,340,340,340,340,341,342,343,344,345,346,347,348,
  351,351,351,352,353,354,355,356,357,358,359,360,364,364,364,364,365,366,
  367,368,369,370,246,245,244,243,242,241,262,261,260,259,258,257,256,255,
  254,253,252,251,250,249,248,247,188,114,187,113,186,112,185,240,239,238,
  237,236,235,234,233,232,231,230,229,228,227,226,225,225,225,225,225,225,
  224,223,222,221,221,221,221,221,221,220,219,218,217,216,215,214,214,214,
  214,213,213,213,213,212,211,210,209,208,207,206,205,204,203,202,201,199,
  198,197,196,195,194,193,192,191,190,189,
266,543,266,266,266,266,266,266,266,266,266,266,266,266,266,266,266,266,266,
  266,266,266,266,266,266,266,266,266,266,266,266,266,266,266,266,266,266,
  266,266,266,272,266,639,699,
382,640,700,
131,131,131,131,131,131,131,131,131,131,131,282,131,250,250,250,250,285,632,
  284,140,294,296,297,298,299,300,301,302,303,304,320,321,322,283,319,286,
  295,307,281,265,130,129,641,234,292,293,235,288,119,273,236,289,633,120,
  265,332,331,329,334,330,333,328,121,220,323,701,309,263,324,328,327,326,
  325,219,328,318,317,316,315,314,313,312,311,310,308,291,290,274,263,634,
  306,305,
695,93,93,93,93,93,642,702,
695,92,92,92,92,92,643,702,
695,91,91,91,91,91,644,702,
695,90,90,90,90,90,645,696,
695,87,87,87,87,87,646,696,
356,358,361,362,363,364,365,366,367,368,369,370,371,372,373,374,375,376,377,
  378,379,386,389,390,391,320,322,388,380,384,381,383,387,359,357,703,307,
  385,382,39,42,705,704,647,64,1,705,64,64,64,64,65,64,64,64,64,64,64,64,
  64,64,64,64,64,64,64,64,64,64,64,64,64,64,64,64,64,64,64,64,64,64,64,64,
  64,64,64,64,
706,706,706,706,706,706,706,706,706,648,
707,707,707,707,707,649,
148,148,148,148,148,148,148,148,148,148,148,148,148,148,148,148,707,707,707,
  707,707,148,148,148,148,148,148,148,148,148,148,148,148,148,148,148,148,
  148,148,148,148,148,650,
131,131,131,131,131,131,131,131,131,131,131,282,131,250,250,250,250,285,287,
  284,294,296,297,298,299,300,301,302,303,304,320,321,322,283,319,286,295,
  307,281,265,130,129,651,234,292,293,235,288,236,289,265,332,331,329,334,
  330,333,328,708,220,323,309,263,324,328,327,326,325,219,328,318,317,316,
  315,314,313,312,311,310,308,291,290,263,306,305,
264,652,
709,709,709,709,709,709,709,709,709,709,709,709,709,709,709,1,653,1,709,
131,131,131,131,131,131,131,131,131,131,131,282,131,250,250,250,250,285,287,
  284,294,296,297,298,299,300,301,302,303,304,320,321,322,283,319,286,295,
  307,281,265,130,129,654,234,292,293,235,288,236,289,265,332,331,329,334,
  330,333,328,710,220,323,309,263,324,328,327,326,325,219,328,318,317,316,
  315,314,313,312,311,310,308,291,290,263,306,305,
131,131,131,131,131,131,131,131,131,131,131,282,131,250,250,250,250,285,287,
  284,294,296,297,298,299,300,301,302,303,304,320,321,322,283,319,286,295,
  307,281,265,130,129,655,234,292,293,235,288,236,289,265,332,331,329,334,
  330,333,328,711,220,323,309,263,324,328,327,326,325,219,328,318,317,316,
  315,314,313,312,311,310,308,291,290,263,306,305,
131,131,131,131,131,131,131,131,131,131,131,282,131,250,250,250,250,285,287,
  284,294,296,297,298,299,300,301,302,303,304,320,321,322,283,319,286,295,
  307,281,265,130,129,656,234,292,293,235,288,236,289,265,332,331,329,334,
  330,333,328,712,220,323,309,263,324,328,327,326,325,219,328,318,317,316,
  315,314,313,312,311,310,308,291,290,263,306,305,
131,131,131,131,131,131,131,131,131,131,131,282,131,250,250,250,250,285,287,
  284,294,296,297,298,299,300,301,302,303,304,320,321,322,283,319,286,295,
  307,281,265,130,129,657,234,292,293,235,288,236,289,265,332,331,329,334,
  330,333,328,713,220,323,309,263,324,328,327,326,325,219,328,318,317,316,
  315,314,313,312,311,310,308,291,290,263,306,305,
131,131,131,131,131,131,131,131,131,131,131,282,131,250,250,250,250,285,287,
  284,294,296,297,298,299,300,301,302,303,304,320,321,322,283,319,286,295,
  307,281,265,130,129,658,234,292,293,235,288,236,289,265,332,331,329,334,
  330,333,328,714,220,323,309,263,324,328,327,326,325,219,328,318,317,316,
  315,314,313,312,311,310,308,291,290,263,306,305,
131,131,131,131,131,131,131,131,131,131,131,282,131,250,250,250,250,285,287,
  284,294,296,297,298,299,300,301,302,303,304,320,321,322,283,319,286,295,
  307,281,265,130,129,659,234,292,293,235,288,236,289,265,332,331,329,334,
  330,333,328,715,220,323,309,263,324,328,327,326,325,219,328,318,317,316,
  315,314,313,312,311,310,308,291,290,263,306,305,
131,131,131,131,131,131,131,131,131,131,131,282,131,250,250,250,250,285,287,
  284,294,296,297,298,299,300,301,302,303,304,320,321,322,283,319,286,295,
  307,281,265,130,129,660,234,292,293,235,288,236,289,265,332,331,329,334,
  330,333,328,716,220,323,309,263,324,328,327,326,325,219,328,318,317,316,
  315,314,313,312,311,310,308,291,290,263,306,305,
131,131,131,131,131,131,131,131,131,131,131,282,131,250,250,250,250,285,287,
  284,294,296,297,298,299,300,301,302,303,304,320,321,322,283,319,286,295,
  307,281,265,130,129,661,234,292,293,235,288,236,289,265,332,331,329,334,
  330,333,328,717,220,323,309,263,324,328,327,326,325,219,328,318,317,316,
  315,314,313,312,311,310,308,291,290,263,306,305,
131,131,131,131,131,131,131,131,131,131,131,282,131,250,250,250,250,285,287,
  284,294,296,297,298,299,300,301,302,303,304,320,321,322,283,319,286,295,
  307,281,265,130,129,662,234,292,293,235,288,236,289,265,332,331,329,334,
  330,333,328,718,220,323,309,263,324,328,327,326,325,219,328,318,317,316,
  315,314,313,312,311,310,308,291,290,263,306,305,
131,131,131,131,131,131,131,131,131,131,131,282,131,250,250,250,250,285,287,
  284,294,296,297,298,299,300,301,302,303,304,320,321,322,283,319,286,295,
  307,281,265,130,129,663,234,292,293,235,288,236,289,265,332,331,329,334,
  330,333,328,719,220,323,309,263,324,328,327,326,325,219,328,318,317,316,
  315,314,313,312,311,310,308,291,290,263,306,305,
720,1,664,1,720,
131,131,131,131,131,131,131,131,131,131,131,282,131,250,250,250,250,285,287,
  284,294,296,297,298,299,300,301,302,303,304,283,559,286,295,307,281,265,
  130,129,665,234,292,293,235,288,236,289,265,220,323,309,263,213,219,213,
  318,317,316,315,314,313,312,311,310,308,291,290,263,306,305,
131,131,131,131,131,131,131,131,131,131,131,282,131,250,250,250,250,285,287,
  284,294,296,297,298,299,300,301,302,303,304,283,559,286,295,307,281,265,
  130,129,666,234,292,293,235,288,236,289,265,220,323,309,263,212,219,212,
  318,317,316,315,314,313,312,311,310,308,291,290,263,306,305,
131,131,131,131,131,131,131,131,131,131,131,282,131,250,250,250,250,285,287,
  284,294,296,297,298,299,300,301,302,303,304,283,559,286,295,307,281,265,
  130,129,667,234,292,293,235,288,236,289,265,220,323,309,263,211,219,211,
  318,317,316,315,314,313,312,311,310,308,291,290,263,306,305,
131,131,131,131,131,131,131,131,131,131,131,282,131,250,250,250,250,285,287,
  284,294,296,297,298,299,300,301,302,303,304,283,559,286,295,307,281,265,
  130,129,668,234,292,293,235,288,236,289,265,220,323,309,263,210,219,210,
  318,317,316,315,314,313,312,311,310,308,291,290,263,306,305,
131,131,131,131,131,131,131,131,131,131,131,282,131,250,250,250,250,285,287,
  284,294,296,297,298,299,300,301,302,303,304,283,559,286,295,307,281,265,
  130,129,669,234,292,293,235,288,236,289,265,220,323,309,263,209,219,209,
  318,317,316,315,314,313,312,311,310,308,291,290,263,306,305,
131,131,131,131,131,131,131,131,131,131,131,282,131,250,250,250,250,285,287,
  284,294,296,297,298,299,300,301,302,303,304,320,321,322,283,319,286,295,
  307,281,265,130,129,670,234,292,293,235,288,236,289,265,721,328,220,323,
  309,263,324,328,327,326,325,219,328,318,317,316,315,314,313,312,311,310,
  308,291,290,263,306,305,
131,131,131,131,131,131,131,131,131,131,131,282,131,250,250,250,250,285,287,
  284,294,296,297,298,299,300,301,302,303,304,320,321,322,283,319,286,295,
  307,281,265,130,129,671,234,292,293,235,288,236,289,265,722,328,220,323,
  309,263,324,328,327,326,325,219,328,318,317,316,315,314,313,312,311,310,
  308,291,290,263,306,305,
131,131,131,131,131,131,131,131,131,131,131,282,131,250,250,250,250,285,287,
  284,294,296,297,298,299,300,301,302,303,304,320,321,322,283,319,286,295,
  307,281,265,130,129,672,234,292,293,235,288,236,289,265,329,723,328,220,
  323,309,263,324,328,327,326,325,219,328,318,317,316,315,314,313,312,311,
  310,308,291,290,263,306,305,
131,131,131,131,131,131,131,131,131,131,131,282,131,250,250,250,250,285,287,
  284,294,296,297,298,299,300,301,302,303,304,320,321,322,283,319,286,295,
  307,281,265,130,129,673,234,292,293,235,288,236,289,265,329,724,328,220,
  323,309,263,324,328,327,326,325,219,328,318,317,316,315,314,313,312,311,
  310,308,291,290,263,306,305,
131,131,131,131,131,131,131,131,131,131,131,282,131,250,250,250,250,285,287,
  284,294,296,297,298,299,300,301,302,303,304,320,321,322,283,319,286,295,
  307,281,265,130,129,674,234,292,293,235,288,236,289,265,329,725,328,220,
  323,309,263,324,328,327,326,325,219,328,318,317,316,315,314,313,312,311,
  310,308,291,290,263,306,305,
131,131,131,131,131,131,131,131,131,131,131,282,131,250,250,250,250,285,287,
  284,294,296,297,298,299,300,301,302,303,304,320,321,322,283,319,286,295,
  307,281,265,130,129,675,234,292,293,235,288,236,289,265,329,726,328,220,
  323,309,263,324,328,327,326,325,219,328,318,317,316,315,314,313,312,311,
  310,308,291,290,263,306,305,
131,131,131,131,131,131,131,131,131,131,131,282,131,250,250,250,250,285,287,
  284,294,296,297,298,299,300,301,302,303,304,320,321,322,283,319,286,295,
  307,281,265,130,129,676,234,292,293,235,288,236,289,265,727,329,330,328,
  220,323,309,263,324,328,327,326,325,219,328,318,317,316,315,314,313,312,
  311,310,308,291,290,263,306,305,
131,131,131,131,131,131,131,131,131,131,131,282,131,250,250,250,250,285,287,
  284,294,296,297,298,299,300,301,302,303,304,320,321,322,283,319,286,295,
  307,281,265,130,129,677,234,292,293,235,288,236,289,265,728,329,330,328,
  220,323,309,263,324,328,327,326,325,219,328,318,317,316,315,314,313,312,
  311,310,308,291,290,263,306,305,
131,131,131,131,131,131,131,131,131,131,131,282,131,250,250,250,250,285,287,
  284,294,296,297,298,299,300,301,302,303,304,320,321,322,283,319,286,295,
  307,281,265,130,129,678,234,292,293,235,288,236,289,265,729,331,329,330,
  328,220,323,309,263,324,328,327,326,325,219,328,318,317,316,315,314,313,
  312,311,310,308,291,290,263,306,305,
131,131,131,131,131,131,131,131,131,131,131,282,131,250,250,250,250,285,287,
  284,294,296,297,298,299,300,301,302,303,304,320,321,322,283,319,286,295,
  307,281,265,130,129,679,234,292,293,235,288,236,289,265,730,331,329,330,
  328,220,323,309,263,324,328,327,326,325,219,328,318,317,316,315,314,313,
  312,311,310,308,291,290,263,306,305,
131,131,131,131,131,131,131,131,131,131,131,282,131,250,250,250,250,285,287,
  284,294,296,297,298,299,300,301,302,303,304,320,321,322,283,319,286,295,
  307,281,265,130,129,680,234,292,293,235,288,236,289,265,332,331,329,330,
  731,328,220,323,309,263,324,328,327,326,325,219,328,318,317,316,315,314,
  313,312,311,310,308,291,290,263,306,305,
131,131,131,131,131,131,131,131,131,131,131,282,131,250,250,250,250,285,287,
  284,294,296,297,298,299,300,301,302,303,304,320,321,322,283,319,286,295,
  307,281,265,130,129,681,234,292,293,235,288,236,289,265,332,331,329,330,
  732,328,220,323,309,263,324,328,327,326,325,219,328,318,317,316,315,314,
  313,312,311,310,308,291,290,263,306,305,
733,733,733,733,1,682,1,733,
41,42,43,38,683,35,35,46,35,45,44,44,
41,42,43,38,684,34,34,46,34,45,44,44,
41,42,43,38,685,33,33,46,33,45,44,44,
41,42,43,38,686,30,30,46,30,45,44,44,
734,734,734,734,1,687,1,734,
735,735,735,735,1,688,1,735,
41,42,43,38,689,69,69,46,69,45,44,44,
131,131,131,131,131,131,131,131,131,131,131,282,131,250,250,250,250,285,287,
  284,294,296,297,298,299,300,301,302,303,304,320,321,322,283,319,286,295,
  307,281,265,130,129,690,234,292,293,235,288,236,289,265,332,331,329,334,
  330,333,328,398,220,323,309,263,324,328,327,326,325,219,328,318,317,316,
  315,314,313,312,311,310,308,291,290,263,306,305,
131,131,131,131,131,131,131,131,131,131,131,282,131,250,250,250,250,285,287,
  284,294,296,297,298,299,300,301,302,303,304,320,321,322,283,319,286,295,
  307,281,265,130,129,691,234,292,293,235,288,236,289,265,332,331,329,334,
  330,333,328,396,220,323,309,263,324,328,327,326,325,219,328,318,317,316,
  315,314,313,312,311,310,308,291,290,263,306,305,
131,131,131,131,131,131,131,131,131,131,131,282,131,250,250,250,250,285,287,
  284,294,296,297,298,299,300,301,302,303,304,320,321,322,283,319,286,295,
  307,281,265,130,129,692,234,292,293,235,288,236,289,265,332,331,329,334,
  330,333,328,394,220,323,309,263,324,328,327,326,325,219,328,318,317,316,
  315,314,313,312,311,310,308,291,290,263,306,305,
695,693,736,
266,543,266,266,266,266,266,266,266,266,266,266,266,266,266,266,266,266,266,
  266,266,263,266,266,266,266,266,266,266,266,266,266,266,266,266,266,266,
  266,266,266,266,272,266,694,737,
19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,
  19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,
  19,19,19,19,695,1,942,
738,738,738,738,738,738,738,738,738,738,738,738,738,738,738,738,738,738,738,
  738,738,738,738,738,738,738,738,738,738,738,738,738,738,738,738,738,738,
  738,738,738,738,738,738,1,696,1,738,
484,697,485,
19,322,322,322,322,1,698,1,311,
266,543,266,266,266,266,266,266,266,266,266,266,266,266,266,266,266,266,266,
  266,266,266,266,266,266,266,266,266,266,266,266,266,266,266,266,266,266,
  266,266,266,272,266,699,137,
739,739,739,739,739,739,739,739,739,739,739,739,739,739,739,739,739,739,739,
  739,739,739,739,739,739,739,739,739,739,739,739,739,739,739,739,739,739,
  739,739,739,739,739,1,700,1,739,
695,99,99,99,99,99,701,696,
740,740,740,740,740,740,740,740,740,740,740,740,740,740,740,1,702,1,740,
19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,
  19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,703,1,909,
64,64,64,64,64,64,64,64,64,64,64,64,64,64,64,64,64,64,64,64,64,64,64,64,64,
  64,64,64,64,64,64,64,64,64,64,64,64,64,64,64,17,17,17,17,17,704,
41,42,43,38,705,60,60,46,60,45,44,44,
156,156,156,156,156,156,156,156,155,155,155,155,156,156,156,156,155,155,155,
  155,155,156,156,156,156,156,156,156,156,156,156,156,156,156,156,156,156,
  156,156,156,156,156,706,
152,152,152,152,152,707,
741,741,741,741,1,708,1,741,
131,131,131,131,131,131,131,131,131,131,131,131,131,130,129,709,744,742,742,
  743,
385,710,232,
385,711,231,
385,712,230,
385,713,229,
385,714,228,
385,715,227,
385,716,226,
385,717,225,
385,718,224,
385,719,223,
385,720,222,
207,560,562,388,380,384,207,207,207,207,207,207,207,207,207,207,207,207,207,
  207,207,207,207,207,207,207,207,207,207,207,207,207,207,207,207,207,721,
  566,565,564,563,561,
206,560,562,388,380,384,206,206,206,206,206,206,206,206,206,206,206,206,206,
  206,206,206,206,206,206,206,206,206,206,206,206,206,206,206,206,206,722,
  566,565,564,563,561,
204,381,383,204,204,204,204,204,204,204,204,204,204,204,204,204,204,204,204,
  204,204,204,204,204,204,204,204,204,204,204,204,204,204,723,568,567,
203,381,383,203,203,203,203,203,203,203,203,203,203,203,203,203,203,203,203,
  203,203,203,203,203,203,203,203,203,203,203,203,203,203,724,568,567,
202,381,383,202,202,202,202,202,202,202,202,202,202,202,202,202,202,202,202,
  202,202,202,202,202,202,202,202,202,202,202,202,202,202,725,568,567,
201,381,383,201,201,201,201,201,201,201,201,201,201,201,201,201,201,201,201,
  201,201,201,201,201,201,201,201,201,201,201,201,201,201,726,568,567,
199,569,571,573,575,199,199,199,199,199,199,199,199,199,199,199,199,199,199,
  199,199,199,199,199,199,199,199,199,199,199,199,727,576,574,572,570,
198,569,571,573,575,198,198,198,198,198,198,198,198,198,198,198,198,198,198,
  198,198,198,198,198,198,198,198,198,198,198,198,728,576,574,572,570,
196,577,387,196,196,196,196,196,196,196,196,196,196,196,196,196,196,196,196,
  196,196,196,196,196,196,196,196,729,579,578,
195,577,387,195,195,195,195,195,195,195,195,195,195,195,195,195,195,195,195,
  195,195,195,195,195,195,195,195,730,579,578,
193,580,359,193,193,193,193,193,193,193,193,193,193,193,193,193,193,193,193,
  193,193,193,193,193,193,731,582,581,
192,580,359,192,192,192,192,192,192,192,192,192,192,192,192,192,192,192,192,
  192,192,192,192,192,192,732,582,581,
41,42,43,38,733,45,45,46,45,45,44,44,
41,42,43,38,734,29,29,46,29,45,44,44,
41,42,43,38,735,28,28,46,28,45,44,44,
745,745,745,745,745,745,745,745,745,1,736,1,745,
137,137,137,137,137,137,137,137,137,137,137,137,137,137,137,137,137,137,137,
  137,137,264,137,137,137,137,137,137,137,137,137,137,137,137,137,137,137,
  137,137,137,137,137,137,737,
131,131,131,131,131,131,131,131,131,131,131,282,131,250,250,250,250,285,632,
  284,140,294,296,297,298,299,300,301,302,303,304,320,321,322,283,319,286,
  295,307,281,265,130,129,738,234,292,293,235,288,123,273,236,289,633,124,
  265,332,331,329,334,330,333,328,122,220,323,309,263,324,328,327,326,325,
  219,328,318,317,316,315,314,313,312,311,310,308,291,290,274,263,634,306,
  305,
131,131,131,131,131,131,131,131,131,131,131,282,131,250,250,250,250,285,287,
  284,294,296,297,298,299,300,301,302,303,304,320,321,322,283,319,286,295,
  307,281,265,130,129,739,234,292,293,235,288,236,289,265,332,331,329,334,
  330,333,328,101,220,323,309,263,324,328,327,326,325,219,328,318,317,316,
  315,314,313,312,311,310,308,291,290,263,306,305,
131,131,131,131,131,131,131,131,131,131,131,131,131,130,129,740,126,263,263,
41,42,43,38,741,58,58,46,58,45,44,44,
132,132,132,132,132,132,132,132,132,132,132,132,133,133,133,133,133,132,19,
  132,1,742,1,127,133,
19,19,743,1,985,
746,1,744,1,746,
287,286,286,283,282,281,280,279,278,745,383,286,624,
385,746,233,

};


static const unsigned short ag_sbt[] = {
     0,  30,  32,  72,  89, 119, 147, 347, 350, 367, 567, 934, 973, 992,
  1011,1016,1021,1026,1034,1042,1061,1107,1153,1172,1181,1206,1226,1245,
  1250,1255,1260,1268,1276,1295,1382,1469,1488,1499,1508,1566,1657,1663,
  1711,1761,1807,1899,1986,2073,2448,2458,2467,2476,2485,2494,2503,2512,
  2521,2530,2539,2548,2557,2566,2575,2584,2597,2610,2621,2667,2713,2759,
  2805,2851,2897,2943,2989,3035,3081,3127,3173,3219,3265,3311,3357,3366,
  3375,3384,3393,3402,3411,3420,3429,3438,3447,3456,3465,3474,3483,3492,
  3501,3510,3519,3528,3537,3546,3555,3564,3573,3582,3591,3600,3609,3618,
  3626,3639,3652,3663,3709,3755,3801,3847,3893,3939,3985,4031,4077,4123,
  4169,4215,4261,4307,4353,4399,4445,4491,4537,4583,4629,4675,4721,4767,
  4813,4859,4905,4951,4997,5043,5089,5135,5181,5227,5273,5319,5365,5411,
  5457,5503,5549,5595,5641,5687,5733,5779,5825,5871,5883,5895,5907,5919,
  5927,5935,5946,5957,5968,5981,5994,6007,6020,6033,6046,6059,6072,6085,
  6098,6111,6124,6137,6145,6150,6155,6160,6206,6252,6298,6344,6390,6436,
  6482,6528,6574,6620,6666,6712,6758,6804,6850,6896,6942,6988,7034,7080,
  7126,7172,7218,7264,7310,7356,7402,7448,7494,7540,7586,7632,7678,7724,
  7770,7816,7862,7908,7954,8000,8046,8092,8138,8184,8230,8276,8322,8368,
  8414,8460,8506,8552,8564,8576,8588,8600,8608,8616,8627,8638,8649,8662,
  8675,8688,8701,8714,8727,8740,8753,8766,8779,8792,8805,8818,8878,8887,
  8957,8964,8967,9054,9133,9141,9185,9193,9237,9248,9251,9259,9267,9279,
  9291,9299,9305,9310,9313,9372,9427,9433,9477,9483,9494,9537,9578,9626,
  9679,9684,9693,9698,9703,9708,9713,9718,9723,9728,9733,9738,9779,9820,
  9885,9890,9895,9900,9905,9910,9915,9920,9925,9930,9935,9940,9983,10047,
  10090,10154,10200,10270,10340,10410,10480,10521,10565,10603,10641,10673,
  10703,10708,10746,10831,10839,10847,10854,10862,10870,10878,10886,10905,
  10924,10932,10940,10948,10956,10998,11006,11014,11022,11047,11077,11127,
  11192,11242,11307,11355,11405,11455,11505,11555,11605,11655,11705,11755,
  11805,11855,11905,11955,12005,12055,12105,12155,12205,12255,12305,12369,
  12434,12499,12564,12628,12702,12752,12817,12881,12931,12981,13031,13089,
  13173,13176,13179,13225,13310,13393,13401,13418,13435,13448,13461,13467,
  13470,13473,13476,13561,13646,13731,13816,13901,13986,14071,14156,14241,
  14326,14411,14496,14581,14666,14751,14836,14921,15006,15091,15176,15261,
  15346,15431,15516,15601,15686,15771,15856,15941,16026,16111,16196,16281,
  16366,16451,16536,16621,16706,16791,16876,16961,17046,17131,17216,17301,
  17386,17471,17556,17641,17726,17811,17896,17911,17926,17941,17956,17964,
  17972,17985,17998,18011,18028,18045,18058,18071,18084,18097,18110,18123,
  18136,18149,18162,18175,18188,18281,18303,18440,18577,18596,18615,18638,
  18660,18668,18676,18682,18691,18699,18707,18730,18754,18759,18764,18811,
  18858,18905,18994,19040,19125,19144,19163,19182,19201,19220,19240,19259,
  19279,19298,19318,19365,19412,19505,19552,19599,19646,19739,19785,19831,
  19916,19924,19970,20016,20062,20147,20166,20176,20186,20198,20280,20292,
  20307,20353,20365,20377,20389,20399,20406,20451,20500,20549,20552,20555,
  20558,20561,20564,20567,20570,20573,20576,20579,20582,20667,20673,20716,
  20759,20802,20845,20888,20931,20974,21020,21066,21112,21158,21204,21250,
  21296,21342,21388,21434,21480,21526,21572,21618,21664,21710,21756,21802,
  21848,21894,21940,21986,22032,22078,22124,22170,22216,22262,22308,22354,
  22400,22412,22424,22436,22448,22460,22479,22491,22503,22547,22559,22571,
  22596,22604,22612,22620,22628,22674,22759,22805,22890,22902,22910,22918,
  22928,22938,22948,22959,22969,23015,23061,23107,23116,23125,23130,23174,
  23219,23229,23234,23244,23265,23556,23600,23603,23696,23704,23712,23720,
  23728,23736,23823,23833,23839,23882,23967,23969,23988,24073,24158,24243,
  24328,24413,24498,24583,24668,24753,24838,24843,24913,24983,25053,25123,
  25193,25272,25351,25431,25511,25591,25671,25752,25833,25915,25997,26080,
  26163,26171,26183,26195,26207,26219,26227,26235,26247,26332,26417,26502,
  26505,26550,26606,26653,26656,26665,26709,26755,26763,26782,26830,26876,
  26888,26931,26937,26945,26965,26968,26971,26974,26977,26980,26983,26986,
  26989,26992,26995,26998,27040,27082,27118,27154,27190,27226,27262,27298,
  27328,27358,27386,27414,27426,27438,27450,27463,27507,27599,27684,27703,
  27715,27740,27745,27750,27763,27766
};


static const unsigned short ag_sbe[] = {
    27,  31,  58,  86, 117, 132, 344, 348, 357, 564, 763, 961, 989,1008,
  1013,1018,1023,1031,1039,1058,1104,1150,1169,1178,1203,1221,1241,1246,
  1251,1256,1265,1273,1291,1337,1424,1484,1493,1501,1563,1654,1659,1708,
  1758,1804,1852,1942,2029,2269,2455,2464,2473,2482,2491,2500,2509,2518,
  2527,2536,2545,2554,2563,2572,2581,2594,2607,2618,2664,2710,2756,2802,
  2848,2894,2940,2986,3032,3078,3124,3170,3216,3262,3308,3354,3363,3372,
  3381,3390,3399,3408,3417,3426,3435,3444,3453,3462,3471,3480,3489,3498,
  3507,3516,3525,3534,3543,3552,3561,3570,3579,3588,3597,3606,3615,3623,
  3636,3649,3660,3706,3752,3798,3844,3890,3936,3982,4028,4074,4120,4166,
  4212,4258,4304,4350,4396,4442,4488,4534,4580,4626,4672,4718,4764,4810,
  4856,4902,4948,4994,5040,5086,5132,5178,5224,5270,5316,5362,5408,5454,
  5500,5546,5592,5638,5684,5730,5776,5822,5868,5880,5892,5904,5916,5924,
  5932,5943,5954,5965,5978,5991,6004,6017,6030,6043,6056,6069,6082,6095,
  6108,6121,6134,6142,6147,6152,6157,6203,6249,6295,6341,6387,6433,6479,
  6525,6571,6617,6663,6709,6755,6801,6847,6893,6939,6985,7031,7077,7123,
  7169,7215,7261,7307,7353,7399,7445,7491,7537,7583,7629,7675,7721,7767,
  7813,7859,7905,7951,7997,8043,8089,8135,8181,8227,8273,8319,8365,8411,
  8457,8503,8549,8561,8573,8585,8597,8605,8613,8624,8635,8646,8659,8672,
  8685,8698,8711,8724,8737,8750,8763,8776,8789,8802,8815,8874,8884,8955,
  8962,8965,9009,9091,9138,9182,9190,9235,9245,9249,9256,9264,9271,9283,
  9296,9304,9309,9312,9371,9426,9432,9475,9482,9493,9536,9577,9625,9677,
  9681,9690,9695,9700,9705,9710,9715,9720,9725,9730,9735,9776,9817,9882,
  9887,9892,9897,9902,9907,9912,9917,9922,9927,9932,9937,9980,10044,10087,
  10151,10197,10239,10309,10379,10449,10518,10557,10598,10634,10668,10698,
  10705,10726,10788,10836,10844,10852,10859,10867,10875,10883,10902,10921,
  10929,10937,10945,10953,10997,11003,11011,11019,11044,11068,11124,11189,
  11239,11304,11352,11402,11452,11502,11552,11602,11652,11702,11752,11802,
  11852,11902,11952,12002,12052,12102,12152,12202,12252,12302,12366,12431,
  12496,12561,12625,12699,12749,12814,12878,12928,12978,13028,13086,13131,
  13174,13177,13222,13267,13349,13398,13407,13427,13444,13455,13465,13468,
  13471,13474,13518,13603,13688,13773,13858,13943,14028,14113,14198,14283,
  14368,14453,14538,14623,14708,14793,14878,14963,15048,15133,15218,15303,
  15388,15473,15558,15643,15728,15813,15898,15983,16068,16153,16238,16323,
  16408,16493,16578,16663,16748,16833,16918,17003,17088,17173,17258,17343,
  17428,17513,17598,17683,17768,17853,17904,17919,17934,17949,17960,17968,
  17979,17992,18005,18020,18037,18054,18067,18080,18093,18106,18119,18132,
  18145,18158,18171,18184,18231,18294,18437,18574,18593,18611,18635,18657,
  18665,18673,18679,18684,18696,18704,18727,18750,18755,18760,18808,18855,
  18902,18948,19037,19082,19141,19159,19179,19197,19217,19235,19256,19274,
  19295,19313,19362,19409,19455,19549,19596,19643,19689,19782,19828,19873,
  19921,19967,20013,20059,20104,20162,20173,20183,20190,20238,20284,20305,
  20350,20357,20369,20381,20398,20405,20449,20499,20548,20550,20553,20556,
  20559,20562,20565,20568,20571,20574,20577,20580,20624,20672,20713,20756,
  20799,20842,20885,20928,20971,21017,21063,21109,21155,21201,21247,21293,
  21339,21385,21431,21477,21523,21569,21615,21661,21707,21753,21799,21845,
  21891,21937,21983,22029,22075,22121,22167,22213,22259,22305,22351,22397,
  22404,22416,22428,22440,22452,22475,22483,22495,22546,22551,22563,22593,
  22601,22609,22617,22625,22671,22716,22802,22847,22894,22907,22915,22925,
  22935,22945,22956,22966,23012,23058,23104,23113,23122,23127,23172,23217,
  23226,23231,23243,23263,23398,23598,23601,23646,23702,23710,23718,23726,
  23734,23779,23832,23838,23881,23924,23968,23985,24030,24115,24200,24285,
  24370,24455,24540,24625,24710,24795,24840,24882,24952,25022,25092,25162,
  25235,25314,25393,25473,25553,25633,25713,25794,25875,25957,26039,26122,
  26168,26175,26187,26199,26211,26224,26232,26239,26289,26374,26459,26503,
  26548,26603,26650,26654,26662,26707,26752,26761,26779,26827,26875,26880,
  26930,26936,26942,26960,26966,26969,26972,26975,26978,26981,26984,26987,
  26990,26993,26996,27034,27076,27115,27151,27187,27223,27257,27293,27325,
  27355,27383,27411,27418,27430,27442,27460,27506,27550,27641,27699,27707,
  27736,27742,27747,27759,27764,27766
};


static const unsigned char ag_fl[] = {
  2,1,1,2,2,1,1,2,0,1,3,3,1,2,1,2,2,1,2,0,1,4,5,3,2,1,0,1,7,7,6,1,1,6,6,
  6,2,3,2,1,2,4,4,4,4,6,4,4,0,1,4,4,4,4,3,3,4,4,7,4,6,4,1,1,2,2,2,2,3,5,
  1,2,2,2,3,2,1,1,1,3,2,1,1,3,1,1,1,3,1,1,3,3,3,3,3,3,3,3,3,4,3,6,2,3,2,
  1,1,2,2,3,3,2,2,3,3,1,1,2,3,1,1,1,4,4,4,1,4,2,1,1,1,1,2,2,1,2,2,3,2,2,
  1,2,3,1,2,2,2,2,2,2,2,2,4,1,1,4,3,2,1,2,3,3,2,1,1,1,1,1,1,2,1,1,1,1,2,
  1,1,2,1,1,2,1,1,2,1,1,2,1,1,2,1,2,4,4,2,4,4,2,4,4,2,4,4,4,4,2,4,4,2,4,
  4,4,4,4,1,2,2,2,2,1,1,1,5,5,5,5,5,5,5,5,5,5,5,7,1,1,1,1,1,1,1,2,2,2,1,
  1,2,2,2,2,1,2,2,2,3,2,2,2,2,2,2,2,2,3,4,1,1,2,2,2,2,2,1,2,1,2,2,2,1,1,
  1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,2,1,1,1,
  1,1,1,1,1,1,1,4,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,
  1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,3,3,3,3,3,3,3,3,3,3,3,3,7,3,
  3,3,3,3,3,3,3,3,3,5,3,5,3,5,3,3,3,3,3,3,3,3,3,3,3,3,3,3,3,3,1,1,1,1,1,
  3,3,3,3,1,1,1,1,1,3,3,3,3,3,3,3,1,1,1,3,1,1,1,3,3,3,3,3,3,3,3,3,3,3,3,
  3,3,3,3,3,3,3,3,3,3,3,3,3,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,
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
  1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,2,2,2,
  2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,
  2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,
  2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,
  2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,
  2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,
  2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,
  2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,
  2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2
};

static const unsigned short ag_ptt[] = {
    0, 36, 37, 37, 34, 39, 43, 43, 44, 44, 39, 39, 39, 40, 47, 47, 47, 51,
   51, 52, 52, 35, 35, 35, 35, 35, 55, 55, 35, 35, 35, 62, 62, 35, 35, 35,
   35, 35, 35, 65, 53, 67, 67, 67, 67, 67, 67, 67, 75, 75, 67, 67, 67, 67,
   67, 67, 67, 67, 67, 67, 67, 88, 88, 89, 89, 89, 96, 96, 63, 63, 33, 33,
   33, 33, 33, 33,102,102,102, 33, 33,106,106, 33,110,110,110, 33,113,113,
   33, 33, 33, 33, 33, 33, 33, 33, 33, 33, 33, 33, 33, 33, 33, 33, 33, 33,
   33, 33, 33, 33, 33, 33, 33, 33, 33, 30, 30, 91, 91, 91, 91, 91, 91,115,
  115,536,442,442,442,133,133,133,437,437,485, 18, 18,435, 12, 12, 12,142,
  142,142,142,142,142,142,142,142,142,155,155,142,142,436, 14, 14, 14, 14,
  428,428,428,428,428, 22, 22, 77, 77,169,169,169,166,172,172,166,175,175,
  166,178,178,166,181,181,166,184,184,166, 57, 26, 26, 26, 28, 28, 28, 23,
   23, 23, 24, 24, 24, 24, 24, 27, 27, 27, 25, 25, 25, 25, 25, 25, 29, 29,
   29, 29, 29,198,198,198,198,207,207,207,207,207,207,207,207,207,207,207,
  206,206,492,492,492,492,492,492,492,492,223,223,492,492,  7,  7,  7,  7,
    7,  7,  6,  6,  6, 17, 17,218,218,219,219, 15, 15, 15, 16, 16, 16, 16,
   16, 16, 16, 16,538,  9,  9,  9,594,594,594,594,594,594,242,242,594,594,
  245,245,608,247,247,608,249,249,608,608,251,251,251,251,251,253,253,254,
  254,254,254,254,254, 31, 32, 32, 32, 32, 32, 32, 32, 32, 32, 32, 32,272,
  272,272,272,266,266,266,266,266,279,279,266,266,266,285,285,285,266,266,
  266,266,266,266,266,266,266,296,296,266,266,266,266,266,266,266,266,266,
  266,309,309,309,266,266,266,266,266,266,266,259,259,260,260,260,260,257,
  257,257,257,257,257,257,257,257,257,257,257,257,258,258,258,338,262,340,
  263,342,264,265,261,261,261,261,261,261,261,261,261,261,261,261,261,261,
  261,366,366,366,366,366,261,261,261,261,375,375,375,375,375,261,261,261,
  261,261,261,261,385,385,385,261,389,389,389,261,261,261,261,261,261,261,
  261,261,261,261,261,261,261,261,261,261,261,261,261,261,261,261,261,261,
  138,345,153,226, 42, 42, 42, 42, 42, 42, 42, 42, 42, 42, 42, 42, 42, 42,
   42, 42, 42, 42, 42, 42, 42, 42, 42, 42, 42, 42, 42, 42, 42, 42, 42, 42,
   42, 42, 42, 42, 42, 42, 42, 42, 42, 42, 50, 50, 50, 50, 50, 50, 50, 50,
   50, 50, 50, 50, 50, 50, 50, 50, 50, 50, 50, 50, 50, 50, 50, 50, 50, 50,
   50, 50, 50, 50, 50, 50, 50, 50, 50, 50, 50, 50, 50, 50, 50, 50, 50, 93,
   93, 93, 93, 93, 93, 93, 93, 93, 93, 93, 93, 93, 93, 93, 93, 93, 93, 93,
   93, 93, 93, 93, 93, 93, 93, 93, 93, 93, 93, 93, 93, 93, 93, 93, 93, 93,
   93, 93, 93, 94, 94, 94, 94, 94, 94, 94, 94, 94, 94, 94, 94, 94, 94, 94,
   94, 94, 94, 94, 94, 94, 94, 94, 94, 94, 94, 94, 94, 94, 94, 94, 94, 94,
   94, 94, 94, 94, 94, 94, 94, 94,136,136,136,136,136,136,136,136,136,136,
  136,136,136,137,137,137,137,137,137,137,137,137,137,137,137,137,137,139,
  139,139,139,139,139,139,139,139,139,139,139,139,139,139,139,139,139,139,
  144,144,144,144,144,144,144,144,144,144,144,144,144,144,144,144,144,144,
  144,144,144,144,144,144,144,144,144,144,144,144,144,144,144,144,144,144,
  144,144,144,144,151,151,151,152,152,152,152,152,156,156,156,156,156,156,
  156,156,156,159,159,159,159,159,159,159,159,159,159,159,159,159,159,159,
  159,159,159,159,159,159,159,159,159,159,159,159,159,159,159,159,159,159,
  159,159,159,159,159,159,160,160,160,160,160,160,160,160,160,160,160,160,
  160,160,160,160,160,160,160,160,160,160,160,160,160,160,160,160,160,160,
  160,160,160,160,160,160,160,160,160,160,160,160,227,227,227,227,228,228,
  230,230,230,230,232,232,232,232,232,232,232,232,232,232,232,232,232,232,
  232,232,232,232,232,232,232,232,232,232,232,232,232,232,232,232,232,232,
  232,232,232,232,232,232,232,232, 45, 41, 46, 48, 49, 11, 54, 56, 58, 59,
   60, 61, 10, 13, 19, 66, 69, 68, 70, 21, 71, 72, 73, 74, 76, 78, 79, 80,
   81, 82, 83, 85, 84, 86, 87, 92, 90, 95, 97, 98, 99,100,101,103,104,105,
  107,108,109,111,112,114,116,117,118,119,120,121,122,123,124,125, 20,126,
  127,128,129,130,131,  2,132,165,167,168,170,171,173,174,176,177,179,180,
  182,183,185,186,187,188,189,190,191,192,193,194,195,196,197,199,200,201,
  202,203,204,205,208,209,210,211,212,213,214,215,216,  3,217,  8,252,243,
  255,256,250,267,268,269,270,271,273,274,275,276,277,278,280,281,282,283,
  284,286,287,288,289,290,291,292,293,294,295,297,298,299,300,301,302,303,
  304,305,306,307,308,310,311,312,313,314,315,316,317,318,319,320,321,  4,
  322,323,324,325,326,327,328,329,330,331,332,333,334,  5,335,336,337,339,
  341,343,344,346,347,348,349,350,351,352,353,354,355,356,357,358,359,360,
  361,362,363,364,365,367,368,369,370,371,372,373,374,376,377,378,379,380,
  381,382,383,384,386,387,388,390,391,392,393,394,395,396,397,398,399,400,
  401,244,402,403,404,405,406,407,408,409,410,411,412,414,141,154,140,148,
  415,416,417,418,158,157,419,229,420,149,224,150,421,220,145,147,146,413,
  143,225,422
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
    case 30: ag_rp_30(V(0,int)); break;
    case 31: ag_rp_31(V(1,int)); break;
    case 32: ag_rp_32(); break;
    case 33: ag_rp_33(); break;
    case 34: ag_rp_34(); break;
    case 35: ag_rp_35(V(2,int)); break;
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
    case 70: ag_rp_70(); break;
    case 71: ag_rp_71(); break;
    case 72: ag_rp_72(V(2,int)); break;
    case 73: ag_rp_73(); break;
    case 74: ag_rp_74(); break;
    case 75: ag_rp_75(); break;
    case 76: ag_rp_76(); break;
    case 77: ag_rp_77(); break;
    case 78: ag_rp_78(); break;
    case 79: ag_rp_79(); break;
    case 80: ag_rp_80(); break;
    case 81: ag_rp_81(); break;
    case 82: ag_rp_82(); break;
    case 83: ag_rp_83(V(0,int)); break;
    case 84: ag_rp_84(V(1,int)); break;
    case 85: ag_rp_85(V(1,int)); break;
    case 86: ag_rp_86(V(0,int)); break;
    case 87: ag_rp_87(V(1,int)); break;
    case 88: ag_rp_88(V(1,int), V(2,int)); break;
    case 89: ag_rp_89(V(1,int)); break;
    case 90: ag_rp_90(); break;
    case 91: ag_rp_91(V(1,int)); break;
    case 92: V(0,int) = ag_rp_92(V(0,int)); break;
    case 93: V(0,int) = ag_rp_93(); break;
    case 94: V(0,int) = ag_rp_94(); break;
    case 95: V(0,int) = ag_rp_95(); break;
    case 96: V(0,int) = ag_rp_96(); break;
    case 97: V(0,int) = ag_rp_97(); break;
    case 98: V(0,int) = ag_rp_98(); break;
    case 99: V(0,int) = ag_rp_99(); break;
    case 100: V(0,int) = ag_rp_100(); break;
    case 101: V(0,int) = ag_rp_101(V(1,int), V(2,int), V(3,int)); break;
    case 102: V(0,int) = ag_rp_102(V(2,int), V(3,int)); break;
    case 103: V(0,int) = ag_rp_103(V(2,int)); break;
    case 104: ag_rp_104(); break;
    case 105: ag_rp_105(); break;
    case 106: ag_rp_106(V(1,int)); break;
    case 107: ag_rp_107(V(2,int)); break;
    case 108: ag_rp_108(); break;
    case 109: ag_rp_109(); break;
    case 110: ag_rp_110(); break;
    case 111: ag_rp_111(); break;
    case 112: ag_rp_112(); break;
    case 113: V(0,int) = ag_rp_113(); break;
    case 114: V(0,int) = ag_rp_114(); break;
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
    case 142: ag_rp_142(); break;
    case 143: ag_rp_143(); break;
    case 144: ag_rp_144(V(0,double)); break;
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
    case 155: ag_rp_155(); break;
    case 156: ag_rp_156(); break;
    case 157: V(0,double) = ag_rp_157(V(0,int)); break;
    case 158: V(0,double) = ag_rp_158(V(0,double)); break;
    case 159: V(0,int) = ag_rp_159(V(0,int)); break;
    case 160: V(0,int) = ag_rp_160(); break;
    case 161: V(0,int) = ag_rp_161(); break;
    case 162: V(0,int) = ag_rp_162(); break;
    case 163: V(0,int) = ag_rp_163(); break;
    case 164: V(0,int) = ag_rp_164(); break;
    case 165: V(0,int) = ag_rp_165(); break;
    case 166: V(0,int) = ag_rp_166(); break;
    case 167: V(0,int) = ag_rp_167(); break;
    case 168: V(0,int) = ag_rp_168(); break;
    case 169: ag_rp_169(V(1,int)); break;
    case 170: ag_rp_170(V(1,int)); break;
    case 171: ag_rp_171(V(0,int)); break;
    case 172: ag_rp_172(V(1,int)); break;
    case 173: ag_rp_173(V(2,int)); break;
    case 174: ag_rp_174(V(1,int)); break;
    case 175: ag_rp_175(V(1,int)); break;
    case 176: ag_rp_176(V(1,int)); break;
    case 177: ag_rp_177(V(1,int)); break;
    case 178: ag_rp_178(V(1,int)); break;
    case 179: ag_rp_179(V(1,int)); break;
    case 180: ag_rp_180(V(1,int)); break;
    case 181: ag_rp_181(V(1,int)); break;
    case 182: V(0,int) = ag_rp_182(V(1,int)); break;
    case 183: V(0,int) = ag_rp_183(V(1,int), V(2,int)); break;
    case 184: V(0,int) = ag_rp_184(); break;
    case 185: V(0,int) = ag_rp_185(V(0,int)); break;
    case 186: V(0,int) = ag_rp_186(); break;
    case 187: V(0,int) = ag_rp_187(); break;
    case 188: V(0,int) = ag_rp_188(); break;
    case 189: V(0,int) = ag_rp_189(); break;
    case 190: V(0,int) = ag_rp_190(); break;
    case 191: V(0,int) = ag_rp_191(); break;
    case 192: V(0,int) = ag_rp_192(); break;
    case 193: V(0,double) = ag_rp_193(); break;
    case 194: V(0,double) = ag_rp_194(V(1,int)); break;
    case 195: V(0,double) = ag_rp_195(V(0,double), V(1,int)); break;
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
    case 278: ag_rp_278(); break;
    case 279: ag_rp_279(); break;
    case 280: ag_rp_280(V(2,int)); break;
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
    case 331: ag_rp_331(); break;
    case 332: ag_rp_332(); break;
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
  "\"ECHO\"",
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
  "\"ECHO\"",
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
   67,1, 67,1,  0,0,  0,0, 67,1,  0,0,  0,0, 67,1, 67,1,  0,0, 67,1, 67,1,
    0,0, 67,1, 67,1, 67,1, 67,1, 67,1, 67,1, 67,1, 67,1, 67,1, 67,1, 67,1,
   67,1, 67,1,  0,0,  0,0, 35,2, 47,1,  0,0, 39,1, 40,1, 39,1, 39,1, 35,2,
   63,1,266,1,266,1,266,1,266,1,266,1,266,1,266,1,266,1,266,1,266,1,266,1,
  266,1,266,1,266,1,342,1,340,1,338,1,261,1,261,1,261,1,261,1,261,1,261,1,
  261,1,261,1,261,1,261,1,261,1,261,1,261,1,261,1,261,1,261,1,266,1,266,1,
  266,1,266,1,266,1,266,1,266,1,266,1,266,1,266,1,266,1,266,1,266,1,266,1,
  266,1,266,1,266,1,266,1,266,1,266,1,266,1,266,1,266,1,266,1,266,1,266,1,
  266,1,266,1,266,1,265,1,342,1,340,1,338,1,261,1,261,1,261,1,261,1,261,1,
  261,1,261,1,261,1,261,1,261,1,261,1,261,1,261,1,261,1,261,1,261,1,261,1,
  261,1,261,1,261,1,261,1,261,1,261,1,261,1,261,1,261,1,261,1,261,1,261,1,
  261,1,261,1,261,1,261,1,261,1,261,1,261,1,261,1,261,1,261,1,261,1,261,1,
  261,1,261,1,261,1,261,1,261,1,261,1,261,1,260,1,260,1,260,1,260,1,259,1,
  259,1,258,1,258,1,258,1,257,1,257,1,257,1,257,1,257,1,257,1,257,1,257,1,
  257,1,257,1,257,1,257,1,257,1,265,1,264,1,263,1,262,1,261,1,261,1,261,1,
  261,1,261,1,261,1,261,1,261,1,261,1,261,1,261,1,261,1,261,1,261,1,261,1,
  261,1,261,1,261,1,261,1,261,1,261,1,261,1,261,1,261,1,261,1,261,1,261,1,
  261,1,261,1,261,1,261,1,261,1,261,1,261,1,261,1,261,1,261,1,261,1,261,1,
  261,1,261,1,261,1,261,1,261,1,261,1,261,1,261,1,261,1,261,1,261,1,261,1,
  261,1,260,1,260,1,260,1,260,1,259,1,259,1,258,1,258,1,258,1,257,1,257,1,
  257,1,257,1,257,1,257,1,257,1,257,1,257,1,257,1,257,1,257,1,257,1,  0,0,
   32,1, 88,1, 30,1, 32,1, 33,1, 33,1, 35,2, 67,2, 67,2,435,1,  0,0, 67,2,
   67,2, 67,2, 67,2, 67,2, 67,2,538,1,219,1,218,1,  6,1,  6,1,  7,1, 15,1,
  538,1,492,1,219,1,218,1,  6,1,206,1,207,1,  0,0,207,1,207,1,207,1,207,1,
  207,1,207,1,207,1,207,1,207,1,206,1,  0,0,  0,0,207,1,207,1,207,1,207,1,
  207,1,207,1,207,1,207,1,207,1,207,1,207,1, 29,1,  0,0, 29,1,  0,0,198,1,
   29,1, 29,1, 29,1, 29,1, 25,1, 27,1, 24,1, 23,1, 28,1, 26,1, 26,1, 77,1,
   77,1, 67,2, 67,2, 67,2,  0,0, 67,2,  0,0, 67,2,  0,0, 67,2,  0,0, 67,2,
    0,0, 67,2,436,1,  0,0, 67,2, 67,2,  0,0, 35,3,  0,0,  0,0,  0,0,  0,0,
    0,0,  0,0,  0,0,  0,0,  0,0,  0,0,  0,0,  0,0,  0,0,  0,0,  0,0,  0,0,
    0,0,  0,0,  0,0,  0,0,  0,0,  0,0,  0,0,  0,0,  0,0,  0,0,  0,0,  0,0,
    0,0,  0,0,  0,0,  0,0,  0,0,  0,0,  0,0,  0,0,  0,0,  0,0, 39,2, 39,2,
    0,0, 35,3, 33,1, 35,3, 63,2,342,2,340,2,338,2,265,2,264,2,263,2,262,2,
  261,2,261,2,261,2,261,2,261,2,261,2,261,2,261,2,261,2,261,2,261,2,261,2,
  261,2,261,2,261,2,261,2,261,2,261,2,261,2,261,2,261,2,261,2,261,2,261,2,
  261,2,261,2,261,2,261,2,261,2,261,2,261,2,261,2,261,2,261,2,261,2,261,2,
  261,2,261,2,261,2,261,2,261,2,261,2,261,2,261,2,261,2,261,2,261,2,261,2,
  261,2,261,2,261,2,261,2,260,2,260,2,260,2,260,2,259,2,259,2,258,2,258,2,
  258,2,257,2,257,2,257,2,257,2,257,2,257,2,257,2,257,2,257,2,257,2,257,2,
  257,2,257,2, 88,2,  0,0,  0,0, 32,2,  0,0, 33,2,  0,0, 33,2,  0,0,  0,0,
    0,0, 33,2,  0,0,  0,0,  0,0, 33,2, 33,2, 33,2,  0,0, 33,2,  0,0, 33,2,
    0,0, 33,2,  0,0, 33,2,  0,0, 33,2,  0,0, 33,2,  0,0, 33,2,  0,0, 33,2,
    0,0,  0,0, 33,2,  0,0,  0,0,  0,0, 33,2,  0,0,  0,0, 33,2,  0,0,  0,0,
    0,0,  0,0, 33,2, 33,2,  0,0,  0,0, 35,3, 67,3, 67,3,  0,0, 67,3, 67,3,
   67,3, 67,3,  6,2, 16,1, 15,2,  0,0,  0,0,207,2,207,2,207,2,207,2,207,2,
  207,2,207,2,207,2,207,2,207,2,207,2,198,2,  7,1,  0,0, 25,2,  0,0, 25,2,
   25,2, 25,2, 25,2, 27,2, 27,2,  0,0, 24,2,  0,0, 24,2,  0,0, 24,2,  0,0,
   24,2,  0,0, 23,2, 23,2,  0,0, 28,2, 28,2,  0,0, 26,2, 26,2,  0,0,  0,0,
    0,0,  0,0,  0,0,  0,0,  0,0,  0,0,  0,0,  0,0,  0,0,  0,0, 67,3, 67,3,
   67,3, 67,3, 67,3, 67,3, 67,3, 67,3,436,2, 67,3, 67,3,  0,0, 35,4, 35,4,
   35,4, 35,4,  0,0, 35,4,  0,0, 35,4, 35,4, 22,1, 63,3,254,1,254,1,254,1,
    0,0,  0,0,264,3,263,3,262,3,251,1,251,1,257,3,  0,0,485,1,  0,0,  0,0,
    6,1,492,1, 32,3,485,1, 33,3, 33,3,115,1,115,1,115,1,  0,0,  0,0, 89,1,
    0,0,  0,0,  0,0, 67,4, 15,3,207,3,207,3,207,3,207,3,207,3,207,3,207,3,
  207,3,207,3,207,3,207,3,198,3, 25,3, 25,3, 25,3, 25,3, 25,3, 27,3, 27,3,
   24,3, 24,3, 24,3, 24,3, 23,3, 23,3, 28,3, 28,3, 26,3, 26,3, 67,4, 35,5,
   35,5, 35,5, 35,5, 35,5, 35,5, 63,4,264,4,263,4,262,4,257,4,  0,0,  0,0,
    0,0, 32,1, 32,1,485,2, 33,4,  0,0,115,2,  0,0,  0,0, 67,5,  0,0,  0,0,
   67,5,207,4,207,4,207,4,207,4,207,4,207,4,207,4,207,4,207,4,207,4,207,4,
  198,4, 25,1, 25,1, 27,1, 27,1, 27,1, 27,1, 24,1, 24,1, 23,1, 23,1, 28,1,
   28,1, 67,5, 35,6, 35,6,257,5, 15,3,  0,0, 33,5,115,3, 67,6,536,1,  0,0,
  207,5,257,6,207,6
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


