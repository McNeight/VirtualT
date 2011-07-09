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

#ifndef A85PARSE_H_1277850585
#include "a85parse.h"
#endif

#ifndef A85PARSE_H_1277850585
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

/*  Line 648, C:/Projects/VirtualT/src/a85parse.syn */
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

#define ag_rp_13() (gAsm->preproc_ifdef(ss[ss_idx--]))

#define ag_rp_14() (gAsm->preproc_if())

#define ag_rp_15() (gAsm->preproc_elif())

#define ag_rp_16() (gAsm->preproc_ifndef(ss[ss_idx--]))

#define ag_rp_17() (gAsm->preproc_else())

#define ag_rp_18() (gAsm->preproc_endif())

static void ag_rp_19(void) {
/* Line 122, C:/Projects/VirtualT/src/a85parse.syn */
if (gAsm->preproc_error(ss[ss_idx--])) gAbort = TRUE;
}

#define ag_rp_20() (gAsm->preproc_undef(ss[ss_idx--]))

static void ag_rp_21(void) {
/* Line 125, C:/Projects/VirtualT/src/a85parse.syn */
 if (gMacroStack[ms_idx] != 0) { delete gMacroStack[ms_idx]; \
															gMacroStack[ms_idx] = 0; } \
														gMacro = gMacroStack[--ms_idx]; \
														gMacro->m_DefString = ss[ss_idx--]; \
														gAsm->preproc_define(); \
														gMacroStack[ms_idx] = 0; gDefine = 0; 
}

static void ag_rp_22(void) {
/* Line 133, C:/Projects/VirtualT/src/a85parse.syn */
 if (gAsm->preproc_macro()) \
															PCB.reduction_token = a85parse_WS_token; 
}

static void ag_rp_23(void) {
/* Line 135, C:/Projects/VirtualT/src/a85parse.syn */
 if (gAsm->preproc_macro()) \
															PCB.reduction_token = a85parse_WS_token; 
}

static void ag_rp_24(void) {
/* Line 139, C:/Projects/VirtualT/src/a85parse.syn */
 gMacro->m_ParamList = gExpList; \
															gMacro->m_Name = ss[ss_idx--]; \
															gExpList = new VTObArray; \
															gMacroStack[ms_idx++] = gMacro; \
															if (gAsm->preproc_macro()) \
																PCB.reduction_token = a85parse_WS_token; \
															gMacro = new CMacro; 
}

static void ag_rp_25(void) {
/* Line 146, C:/Projects/VirtualT/src/a85parse.syn */
 gMacro->m_Name = ss[ss_idx--]; gMacroStack[ms_idx++] = gMacro; \
															if (gAsm->preproc_macro()) \
																PCB.reduction_token = a85parse_WS_token; \
															gMacro = new CMacro; 
}

static void ag_rp_26(int c) {
/* Line 152, C:/Projects/VirtualT/src/a85parse.syn */
 ss[++ss_idx][0] = c; ss[ss_idx][1] = 0; ss_len = 1; 
}

static void ag_rp_27(int c) {
/* Line 153, C:/Projects/VirtualT/src/a85parse.syn */
 ss[ss_idx][ss_len++] = c; ss[ss_idx][ss_len] = 0; 
}

static void ag_rp_28(void) {
/* Line 154, C:/Projects/VirtualT/src/a85parse.syn */
 ss[ss_idx][ss_len++] = '\n'; ss[ss_idx][ss_len] = 0; 
}

static void ag_rp_29(void) {
/* Line 157, C:/Projects/VirtualT/src/a85parse.syn */
 page = 0; seg = CSEG; 
}

static void ag_rp_30(void) {
/* Line 158, C:/Projects/VirtualT/src/a85parse.syn */
 page = 0; seg = DSEG; 
}

#define ag_rp_31(p) (page = p)

#define ag_rp_32() (gAsm->pragma_list())

#define ag_rp_33() (gAsm->pragma_hex())

#define ag_rp_34() (gAsm->directive_org())

#define ag_rp_35() (gAsm->directive_aseg())

#define ag_rp_36() (gAsm->directive_ds())

#define ag_rp_37() (gAsm->directive_db())

#define ag_rp_38() (gAsm->directive_dw())

#define ag_rp_39() (gAsm->directive_public())

#define ag_rp_40() (gAsm->directive_extern())

#define ag_rp_41() (gAsm->directive_extern())

#define ag_rp_42() (gAsm->directive_module(ss[ss_idx--]))

#define ag_rp_43() (gAsm->directive_name(ss[ss_idx--]))

#define ag_rp_44() (gAsm->directive_stkln())

#define ag_rp_45() (gAsm->directive_echo())

#define ag_rp_46() (gAsm->directive_echo(ss[ss_idx--]))

#define ag_rp_47() (gAsm->directive_fill())

#define ag_rp_48() (gAsm->directive_printf(ss[ss_idx--]))

#define ag_rp_49() (gAsm->directive_end(""))

#define ag_rp_50() (gAsm->directive_end(ss[ss_idx--]))

#define ag_rp_51() (gAsm->directive_if())

#define ag_rp_52() (gAsm->directive_else())

#define ag_rp_53() (gAsm->directive_endif())

#define ag_rp_54() (gAsm->directive_endian(1))

#define ag_rp_55() (gAsm->directive_endian(0))

#define ag_rp_56() (gAsm->directive_title(ss[ss_idx--]))

#define ag_rp_57() (gAsm->directive_title(ss[ss_idx--]))

#define ag_rp_58() (gAsm->directive_page(-1))

#define ag_rp_59() (gAsm->directive_sym())

#define ag_rp_60() (gAsm->directive_link(ss[ss_idx--]))

#define ag_rp_61() (gAsm->directive_maclib(ss[ss_idx--]))

#define ag_rp_62() (gAsm->directive_page(page))

#define ag_rp_63() (page = 60)

#define ag_rp_64(n) (page = n)

#define ag_rp_65() (expression_list_literal())

#define ag_rp_66() (expression_list_literal())

#define ag_rp_67() (expression_list_equation())

#define ag_rp_68() (expression_list_equation())

#define ag_rp_69() (expression_list_literal())

#define ag_rp_70() (expression_list_literal())

#define ag_rp_71() (gNameList->Add(ss[ss_idx--]))

#define ag_rp_72() (gNameList->Add(ss[ss_idx--]))

static void ag_rp_73(void) {
/* Line 236, C:/Projects/VirtualT/src/a85parse.syn */
 strcpy(ss[++ss_idx], "$"); ss_len = 1; 
}

static void ag_rp_74(void) {
/* Line 237, C:/Projects/VirtualT/src/a85parse.syn */
 strcpy(ss[++ss_idx], "&"); ss_len = 1; 
}

static void ag_rp_75(int c) {
/* Line 240, C:/Projects/VirtualT/src/a85parse.syn */
 ss[++ss_idx][0] = c; ss[ss_idx][1] = 0; ss_len = 1; \
														if (PCB.column == 2) ss_addr = gAsm->m_ActiveSeg->m_Address; 
}

static void ag_rp_76(int c) {
/* Line 242, C:/Projects/VirtualT/src/a85parse.syn */
 ss[ss_idx][ss_len++] = c; ss[ss_idx][ss_len] = 0; 
}

static void ag_rp_77(int c) {
/* Line 243, C:/Projects/VirtualT/src/a85parse.syn */
 ss[ss_idx][ss_len++] = c; ss[ss_idx][ss_len] = 0; 
}

static void ag_rp_78(int c) {
/* Line 246, C:/Projects/VirtualT/src/a85parse.syn */
 ss[++ss_idx][0] = c; ss[ss_idx][1] = 0; ss_len = 1; 
}

static void ag_rp_79(int c) {
/* Line 247, C:/Projects/VirtualT/src/a85parse.syn */
 ss[ss_idx][ss_len++] = c; ss[ss_idx][ss_len] = 0; 
}

static void ag_rp_80(int ch1, int ch2) {
/* Line 253, C:/Projects/VirtualT/src/a85parse.syn */
 ss_idx++; ss_len = 2; sprintf(ss[ss_idx], "%c%c", ch1, ch2); 
}

static void ag_rp_81(int c) {
/* Line 254, C:/Projects/VirtualT/src/a85parse.syn */
 ss[ss_idx][ss_len++] = c; ss[ss_idx][ss_len] = 0; 
}

static void ag_rp_82(void) {
/* Line 261, C:/Projects/VirtualT/src/a85parse.syn */
 ss_idx++; ss_len = 0; 
}

static void ag_rp_83(int c) {
/* Line 262, C:/Projects/VirtualT/src/a85parse.syn */
 ss[ss_idx][ss_len++] = c; ss[ss_idx][ss_len] = 0; 
}

#define ag_rp_84(n) (n)

#define ag_rp_85() ('\\')

#define ag_rp_86() ('\n')

#define ag_rp_87() ('\t')

#define ag_rp_88() ('\r')

#define ag_rp_89() ('\0')

#define ag_rp_90() ('"')

#define ag_rp_91() (0x08)

#define ag_rp_92() (0x0C)

#define ag_rp_93(n1, n2, n3) ((n1-'0') * 64 + (n2-'0') * 8 + n3-'0')

#define ag_rp_94(n1, n2) (chtoh(n1) * 16 + chtoh(n2))

#define ag_rp_95(n1) (chtoh(n1))

static void ag_rp_96(void) {
/* Line 282, C:/Projects/VirtualT/src/a85parse.syn */
 ss[ss_idx][ss_len++] = '>'; ss[ss_idx][ss_len] = 0; 
}

static void ag_rp_97(void) {
/* Line 285, C:/Projects/VirtualT/src/a85parse.syn */
 ss[++ss_idx][0] = '<'; ss[ss_idx][1] = 0; ss_len = 1; 
}

static void ag_rp_98(int c) {
/* Line 286, C:/Projects/VirtualT/src/a85parse.syn */
 ss[ss_idx][ss_len++] = c; ss[ss_idx][ss_len] = 0; 
}

static void ag_rp_99(int c) {
/* Line 287, C:/Projects/VirtualT/src/a85parse.syn */
 ss[ss_idx][ss_len++] = '\\'; ss[ss_idx][ss_len++] = c; ss[ss_idx][ss_len] = 0; 
}

#define ag_rp_100() (gAsm->label(ss[ss_idx--]))

#define ag_rp_101() (gAsm->label(ss[ss_idx--]))

#define ag_rp_102() (PAGE)

#define ag_rp_103() (INPAGE)

#define ag_rp_104() (condition(-1))

#define ag_rp_105() (condition(COND_NOCMP))

#define ag_rp_106() (condition(COND_EQ))

#define ag_rp_107() (condition(COND_NE))

#define ag_rp_108() (condition(COND_GE))

#define ag_rp_109() (condition(COND_LE))

#define ag_rp_110() (condition(COND_GT))

#define ag_rp_111() (condition(COND_LT))

#define ag_rp_112() (gEq->Add(RPN_BITOR))

#define ag_rp_113() (gEq->Add(RPN_BITOR))

#define ag_rp_114() (gEq->Add(RPN_BITXOR))

#define ag_rp_115() (gEq->Add(RPN_BITXOR))

#define ag_rp_116() (gEq->Add(RPN_BITAND))

#define ag_rp_117() (gEq->Add(RPN_BITAND))

#define ag_rp_118() (gEq->Add(RPN_LEFTSHIFT))

#define ag_rp_119() (gEq->Add(RPN_LEFTSHIFT))

#define ag_rp_120() (gEq->Add(RPN_RIGHTSHIFT))

#define ag_rp_121() (gEq->Add(RPN_RIGHTSHIFT))

#define ag_rp_122() (gEq->Add(RPN_ADD))

#define ag_rp_123() (gEq->Add(RPN_SUBTRACT))

#define ag_rp_124() (gEq->Add(RPN_MULTIPLY))

#define ag_rp_125() (gEq->Add(RPN_DIVIDE))

#define ag_rp_126() (gEq->Add(RPN_MODULUS))

#define ag_rp_127() (gEq->Add(RPN_MODULUS))

#define ag_rp_128() (gEq->Add(RPN_EXPONENT))

#define ag_rp_129() (gEq->Add(RPN_NOT))

#define ag_rp_130() (gEq->Add(RPN_NOT))

#define ag_rp_131() (gEq->Add(RPN_BITNOT))

#define ag_rp_132() (gEq->Add(RPN_NEGATE))

#define ag_rp_133(n) (gEq->Add((double) n))

static void ag_rp_134(void) {
/* Line 375, C:/Projects/VirtualT/src/a85parse.syn */
 delete gMacro; gMacro = gMacroStack[ms_idx-1]; \
														gMacroStack[ms_idx--] = 0; if (gMacro->m_ParamList == 0) \
														{\
															gEq->Add(gMacro->m_Name); gMacro->m_Name = ""; }\
														else { \
															gEq->Add((VTObject *) gMacro); gMacro = new CMacro; \
														} 
}

#define ag_rp_135() (gEq->Add(RPN_FLOOR))

#define ag_rp_136() (gEq->Add(RPN_CEIL))

#define ag_rp_137() (gEq->Add(RPN_LN))

#define ag_rp_138() (gEq->Add(RPN_LOG))

#define ag_rp_139() (gEq->Add(RPN_SQRT))

#define ag_rp_140() (gEq->Add(RPN_IP))

#define ag_rp_141() (gEq->Add(RPN_FP))

#define ag_rp_142() (gEq->Add(RPN_HIGH))

#define ag_rp_143() (gEq->Add(RPN_LOW))

#define ag_rp_144() (gEq->Add(RPN_DEFINED, ss[ss_idx--]))

#define ag_rp_145(n) (n)

#define ag_rp_146(r) (r)

#define ag_rp_147(n) (n)

#define ag_rp_148() (conv_to_dec())

#define ag_rp_149() (conv_to_hex())

#define ag_rp_150() (conv_to_bin())

#define ag_rp_151() (conv_to_oct())

#define ag_rp_152() (conv_to_hex())

#define ag_rp_153() (conv_to_hex())

#define ag_rp_154() (conv_to_bin())

#define ag_rp_155() (conv_to_oct())

#define ag_rp_156() (conv_to_dec())

static void ag_rp_157(int n) {
/* Line 421, C:/Projects/VirtualT/src/a85parse.syn */
 int_len = 1; integer[0] = n; integer[1] = 0; 
}

static void ag_rp_158(int n) {
/* Line 422, C:/Projects/VirtualT/src/a85parse.syn */
 int_len = 2; integer[0] = '-', integer[1] = n; integer[2] = 0; 
}

static void ag_rp_159(int n) {
/* Line 423, C:/Projects/VirtualT/src/a85parse.syn */
 int_len = 1; integer[0] = n; integer[1] = 0; 
}

static void ag_rp_160(int n) {
/* Line 424, C:/Projects/VirtualT/src/a85parse.syn */
 integer[int_len++] = n; integer[int_len] = 0; 
}

static void ag_rp_161(int n) {
/* Line 429, C:/Projects/VirtualT/src/a85parse.syn */
 int_len = 1; integer[0] = n; integer[1] = 0; 
}

static void ag_rp_162(int n) {
/* Line 430, C:/Projects/VirtualT/src/a85parse.syn */
 int_len = 1; integer[0] = n; integer[1] = 0; 
}

static void ag_rp_163(int n) {
/* Line 431, C:/Projects/VirtualT/src/a85parse.syn */
 integer[int_len++] = n; integer[int_len] = 0; 
}

static void ag_rp_164(int n) {
/* Line 434, C:/Projects/VirtualT/src/a85parse.syn */
 integer[int_len++] = n; integer[int_len] = 0; 
}

static void ag_rp_165(int n) {
/* Line 435, C:/Projects/VirtualT/src/a85parse.syn */
 integer[int_len++] = n; integer[int_len] = 0; 
}

static void ag_rp_166(int n) {
/* Line 438, C:/Projects/VirtualT/src/a85parse.syn */
 integer[int_len++] = n; integer[int_len] = 0; 
}

static void ag_rp_167(int n) {
/* Line 439, C:/Projects/VirtualT/src/a85parse.syn */
 integer[int_len++] = n; integer[int_len] = 0; 
}

static void ag_rp_168(int n) {
/* Line 442, C:/Projects/VirtualT/src/a85parse.syn */
 integer[int_len++] = n; integer[int_len] = 0; 
}

static void ag_rp_169(int n) {
/* Line 443, C:/Projects/VirtualT/src/a85parse.syn */
 integer[int_len++] = n; integer[int_len] = 0; 
}

#define ag_rp_170(n) (n)

#define ag_rp_171(n1, n2) ((n1 << 8) | n2)

#define ag_rp_172() ('\\')

#define ag_rp_173(n) (n)

#define ag_rp_174() ('\\')

#define ag_rp_175() ('\n')

#define ag_rp_176() ('\t')

#define ag_rp_177() ('\r')

#define ag_rp_178() ('\0')

#define ag_rp_179() ('\'')

#define ag_rp_180() ('\'')

static double ag_rp_181(void) {
/* Line 464, C:/Projects/VirtualT/src/a85parse.syn */
 gDivisor = 1.0; return (double) conv_to_dec(); 
}

static double ag_rp_182(int d) {
/* Line 465, C:/Projects/VirtualT/src/a85parse.syn */
 gDivisor = 10.0; return ((double) (d - '0') / gDivisor); 
}

static double ag_rp_183(double r, int d) {
/* Line 466, C:/Projects/VirtualT/src/a85parse.syn */
 gDivisor *= 10.0; return (r + (double) (d - '0') / gDivisor); 
}

#define ag_rp_184() (reg[reg_cnt++] = '0')

#define ag_rp_185() (reg[reg_cnt++] = '1')

#define ag_rp_186() (reg[reg_cnt++] = '2')

#define ag_rp_187() (reg[reg_cnt++] = '3')

#define ag_rp_188() (reg[reg_cnt++] = '4')

#define ag_rp_189() (reg[reg_cnt++] = '5')

#define ag_rp_190() (reg[reg_cnt++] = '6')

#define ag_rp_191() (reg[reg_cnt++] = '7')

#define ag_rp_192() (reg[reg_cnt++] = '0')

#define ag_rp_193() (reg[reg_cnt++] = '1')

#define ag_rp_194() (reg[reg_cnt++] = '2')

#define ag_rp_195() (reg[reg_cnt++] = '3')

#define ag_rp_196() (reg[reg_cnt++] = '0')

#define ag_rp_197() (reg[reg_cnt++] = '1')

#define ag_rp_198() (reg[reg_cnt++] = '2')

#define ag_rp_199() (reg[reg_cnt++] = '3')

#define ag_rp_200() (reg[reg_cnt++] = '3')

#define ag_rp_201() (reg[reg_cnt++] = '0')

#define ag_rp_202() (reg[reg_cnt++] = '1')

#define ag_rp_203() (gAsm->opcode_arg_0		(OPCODE_ASHR))

#define ag_rp_204() (gAsm->opcode_arg_0		(OPCODE_CMA))

#define ag_rp_205() (gAsm->opcode_arg_0		(OPCODE_CMC))

#define ag_rp_206() (gAsm->opcode_arg_0		(OPCODE_DAA))

#define ag_rp_207() (gAsm->opcode_arg_0		(OPCODE_DI))

#define ag_rp_208() (gAsm->opcode_arg_0		(OPCODE_DSUB))

#define ag_rp_209() (gAsm->opcode_arg_0		(OPCODE_EI))

#define ag_rp_210() (gAsm->opcode_arg_0		(OPCODE_HLT))

#define ag_rp_211() (gAsm->opcode_arg_0		(OPCODE_LHLX))

#define ag_rp_212() (gAsm->opcode_arg_0		(OPCODE_NOP))

#define ag_rp_213() (gAsm->opcode_arg_0		(OPCODE_PCHL))

#define ag_rp_214() (gAsm->opcode_arg_0		(OPCODE_RAL))

#define ag_rp_215() (gAsm->opcode_arg_0		(OPCODE_RAR))

#define ag_rp_216() (gAsm->opcode_arg_0		(OPCODE_RC))

#define ag_rp_217() (gAsm->opcode_arg_0		(OPCODE_RET))

#define ag_rp_218() (gAsm->opcode_arg_0		(OPCODE_RIM))

#define ag_rp_219() (gAsm->opcode_arg_0		(OPCODE_RLC))

#define ag_rp_220() (gAsm->opcode_arg_0		(OPCODE_RLDE))

#define ag_rp_221() (gAsm->opcode_arg_0		(OPCODE_RM))

#define ag_rp_222() (gAsm->opcode_arg_0		(OPCODE_RNC))

#define ag_rp_223() (gAsm->opcode_arg_0		(OPCODE_RNZ))

#define ag_rp_224() (gAsm->opcode_arg_0		(OPCODE_RP))

#define ag_rp_225() (gAsm->opcode_arg_0		(OPCODE_RPE))

#define ag_rp_226() (gAsm->opcode_arg_0		(OPCODE_RPO))

#define ag_rp_227() (gAsm->opcode_arg_0		(OPCODE_RRC))

#define ag_rp_228() (gAsm->opcode_arg_0		(OPCODE_RSTV))

#define ag_rp_229() (gAsm->opcode_arg_0		(OPCODE_RZ))

#define ag_rp_230() (gAsm->opcode_arg_0		(OPCODE_SHLX))

#define ag_rp_231() (gAsm->opcode_arg_0		(OPCODE_SIM))

#define ag_rp_232() (gAsm->opcode_arg_0		(OPCODE_SPHL))

#define ag_rp_233() (gAsm->opcode_arg_0		(OPCODE_STC))

#define ag_rp_234() (gAsm->opcode_arg_0		(OPCODE_XCHG))

#define ag_rp_235() (gAsm->opcode_arg_0		(OPCODE_XTHL))

#define ag_rp_236() (gAsm->opcode_arg_1reg		(OPCODE_STAX))

#define ag_rp_237() (gAsm->opcode_arg_1reg		(OPCODE_LDAX))

#define ag_rp_238() (gAsm->opcode_arg_1reg		(OPCODE_POP))

#define ag_rp_239() (gAsm->opcode_arg_1reg		(OPCODE_PUSH))

#define ag_rp_240() (gAsm->opcode_arg_1reg		(OPCODE_ADC))

#define ag_rp_241() (gAsm->opcode_arg_1reg		(OPCODE_ADD))

#define ag_rp_242() (gAsm->opcode_arg_1reg		(OPCODE_ANA))

#define ag_rp_243() (gAsm->opcode_arg_1reg		(OPCODE_CMP))

#define ag_rp_244() (gAsm->opcode_arg_1reg		(OPCODE_DCR))

#define ag_rp_245() (gAsm->opcode_arg_1reg		(OPCODE_INR))

#define ag_rp_246() (gAsm->opcode_arg_2reg		(OPCODE_MOV))

#define ag_rp_247() (gAsm->opcode_arg_1reg		(OPCODE_ORA))

#define ag_rp_248() (gAsm->opcode_arg_1reg		(OPCODE_SBB))

#define ag_rp_249() (gAsm->opcode_arg_1reg		(OPCODE_SUB))

#define ag_rp_250() (gAsm->opcode_arg_1reg		(OPCODE_XRA))

#define ag_rp_251() (gAsm->opcode_arg_1reg		(OPCODE_DAD))

#define ag_rp_252() (gAsm->opcode_arg_1reg		(OPCODE_DCX))

#define ag_rp_253() (gAsm->opcode_arg_1reg		(OPCODE_INX))

#define ag_rp_254() (gAsm->opcode_arg_1reg_equ16(OPCODE_LXI))

#define ag_rp_255() (gAsm->opcode_arg_1reg_equ8(OPCODE_MVI))

#define ag_rp_256(c) (gAsm->opcode_arg_imm		(OPCODE_RST, c))

#define ag_rp_257() (gAsm->opcode_arg_equ8		(OPCODE_ACI))

#define ag_rp_258() (gAsm->opcode_arg_equ8		(OPCODE_ADI))

#define ag_rp_259() (gAsm->opcode_arg_equ8		(OPCODE_ANI))

#define ag_rp_260() (gAsm->opcode_arg_equ16	(OPCODE_CALL))

#define ag_rp_261() (gAsm->opcode_arg_equ16	(OPCODE_CC))

#define ag_rp_262() (gAsm->opcode_arg_equ16	(OPCODE_CM))

#define ag_rp_263() (gAsm->opcode_arg_equ16	(OPCODE_CNC))

#define ag_rp_264() (gAsm->opcode_arg_equ16	(OPCODE_CNZ))

#define ag_rp_265() (gAsm->opcode_arg_equ16	(OPCODE_CP))

#define ag_rp_266() (gAsm->opcode_arg_equ16	(OPCODE_CPE))

#define ag_rp_267() (gAsm->opcode_arg_equ8		(OPCODE_CPI))

#define ag_rp_268() (gAsm->opcode_arg_equ16	(OPCODE_CPO))

#define ag_rp_269() (gAsm->opcode_arg_equ16	(OPCODE_CZ))

#define ag_rp_270() (gAsm->opcode_arg_equ8		(OPCODE_IN))

#define ag_rp_271() (gAsm->opcode_arg_equ16	(OPCODE_JC))

#define ag_rp_272() (gAsm->opcode_arg_equ16	(OPCODE_JD))

#define ag_rp_273() (gAsm->opcode_arg_equ16	(OPCODE_JM))

#define ag_rp_274() (gAsm->opcode_arg_equ16	(OPCODE_JMP))

#define ag_rp_275() (gAsm->opcode_arg_equ16	(OPCODE_JNC))

#define ag_rp_276() (gAsm->opcode_arg_equ16	(OPCODE_JND))

#define ag_rp_277() (gAsm->opcode_arg_equ16	(OPCODE_JNZ))

#define ag_rp_278() (gAsm->opcode_arg_equ16	(OPCODE_JP))

#define ag_rp_279() (gAsm->opcode_arg_equ16	(OPCODE_JPE))

#define ag_rp_280() (gAsm->opcode_arg_equ16	(OPCODE_JPO))

#define ag_rp_281() (gAsm->opcode_arg_equ16	(OPCODE_JZ))

#define ag_rp_282() (gAsm->opcode_arg_equ16	(OPCODE_LDA))

#define ag_rp_283() (gAsm->opcode_arg_equ8		(OPCODE_LDEH))

#define ag_rp_284() (gAsm->opcode_arg_equ8		(OPCODE_LDES))

#define ag_rp_285() (gAsm->opcode_arg_equ16	(OPCODE_LHLD))

#define ag_rp_286() (gAsm->opcode_arg_equ8		(OPCODE_ORI))

#define ag_rp_287() (gAsm->opcode_arg_equ8		(OPCODE_OUT))

#define ag_rp_288() (gAsm->opcode_arg_equ8		(OPCODE_SBI))

#define ag_rp_289() (gAsm->opcode_arg_equ16	(OPCODE_SHLD))

#define ag_rp_290() (gAsm->opcode_arg_equ16	(OPCODE_STA))

#define ag_rp_291() (gAsm->opcode_arg_equ8		(OPCODE_SUI))

#define ag_rp_292() (gAsm->opcode_arg_equ8		(OPCODE_XRI))


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
    7,  8,  0,  0,  9, 10, 11, 12,  0,  0, 13, 14, 15, 16, 17, 18, 19, 20,
   21, 22, 23, 24, 25, 26, 27, 28, 29, 30,  0, 31,  0, 32, 33,  0,  0,  0,
   34, 35,  0,  0, 36,  0,  0,  0, 37,  0,  0, 38, 39, 40, 41, 42, 43, 44,
   45, 46, 47, 48, 49, 50, 51, 52, 53, 54, 55, 56, 57, 58, 59, 60, 61,  0,
   62, 63, 64, 65, 66, 67, 68, 69, 70, 71, 72,  0,  0, 73, 74, 75, 76, 77,
   78, 79,  0, 80, 81,  0, 82, 83,  0, 84, 85, 86, 87, 88, 89, 90, 91, 92,
   93,  0,  0, 94, 95, 96, 97, 98, 99,  0,100,101,102,103,104,105,  0,  0,
    0,106,  0,  0,107,  0,  0,108,  0,  0,109,  0,  0,110,  0,  0,111,  0,
    0,112,113,  0,114,115,  0,116,117,  0,118,119,120,121,  0,122,123,  0,
  124,125,126,127,128,  0,129,130,131,132,133,134,  0,  0,135,136,137,138,
  139,140,141,142,143,144,145,146,147,148,149,150,151,152,153,154,  0,  0,
  155,156,157,158,159,160,  0,  0,161,162,163,164,165,166,167,168,169,170,
  171,172,173,174,175,176,177,178,179,180,  0,181,182,183,184,185,186,187,
  188,189,  0,  0,190,191,  0,  0,192,  0,  0,193,  0,  0,194,195,196,197,
  198,199,200,201,202,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,
    0,  0,203,204,205,206,207,  0,  0,208,209,210,  0,  0,  0,211,212,213,
  214,215,216,217,218,219,  0,  0,220,221,222,223,224,225,226,227,228,229,
    0,  0,  0,230,231,232,233,234,235,236,237,238,239,240,241,242,243,244,
  245,246,247,248,249,250,251,252,253,  0,254,  0,255,256,257,258,259,260,
  261,262,263,264,265,266,267,268,269,270,271,  0,  0,  0,  0,  0,272,273,
  274,275,  0,  0,  0,  0,  0,276,277,278,279,280,281,282,  0,  0,  0,283,
    0,  0,284,285,286,287,288,289,290,291,292
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
  1,399,  1,400,  1,401,  1,402,  1,408,  1,407,  1,409,  1,411,
  1,412,  1,413,  1,414,  1,415,  1,416,  1,417,  1,418,  1,419,
  1,424,  1,425,  1,426,  1,429,  1,430,  1,431,  1,432,  1,433,
  1,434,  1,435,  1,436,  1,437,  1,438,  1,439,  1,440,  1,441,
  1,442,  1,443,  1,444,  1,446,  1,447,  1,448,  1,449,  1,451,
  1,452,  1,453,  1,454,  1,455,  1,456,  1,131,  1,459,  1,460,
  1,463,  1,465,  1,467,  1,469,  1,471,  1,474,  1,476,  1,478,
  1,480,  1,482,  1,488,  1,491,  1,493,  1,494,  1,495,  1,496,
  1,497,  1,498,  1,499,  1,500,  1,501,  1,503,  1,216,  1,217,
  1,229,  1,230,  1,231,  1,232,  1,233,  1,234,  1,235,  1,506,
  1,239,  1,241,  1,243,  1,245,  1,505,  1,507,  1,508,  1,509,
  1,510,  1,511,  1,512,  1,513,  1,514,  1,515,  1,516,  1,517,
  1,518,  1,519,  1,520,  1,521,  1,522,  1,523,  1,524,  1,525,
  1,526,  1,527,  1,528,  1,529,  1,530,  1,531,  1,532,  1,533,
  1,534,  1,535,  1,536,  1,537,  1,538,  1,539,  1,540,  1,541,
  1,542,  1,543,  1,544,  1,545,  1,546,  1,547,  1,548,  1,549,
  1,550,  1,551,  1,552,  1,553,  1,555,  1,556,  1,557,  1,558,
  1,559,  1,560,  1,561,  1,562,  1,563,  1,564,  1,565,  1,567,
  1,568,  1,569,  1,570,  1,571,  1,572,  1,573,  1,574,  1,575,
  1,576,  1,577,  1,578,  1,579,  1,580,  1,581,  1,582,  1,583,
  1,584,  1,585,  1,586,  1,587,  1,588,  1,589,  1,590,  1,591,
  1,592,  1,593,  1,594,  1,595,  1,596,  1,597,  1,598,  1,599,
  1,600,  1,601,  1,602,  1,603,  1,604,  1,605,  1,606,  1,607,
  1,608,  1,609,  1,610,  1,611,  1,612,  1,613,  1,614,  1,615,
  1,616,  1,617,  1,618,  1,619,0
};

static const unsigned char ag_key_ch[] = {
    0, 42, 47,255, 85,255, 76,255, 67,255, 78,255, 35, 36, 38, 47, 73,255,
   61,255, 61,255, 42, 47, 61,255, 42, 47,255, 60, 61,255, 61,255, 61, 62,
  255, 67, 68, 73,255, 65, 68, 73,255, 69, 72,255, 67, 68, 78, 82, 83,255,
   67, 76, 89,255, 65, 67, 80,255, 67, 90,255, 69, 73, 79,255, 65, 67, 69,
   77, 78, 80, 83, 90,255, 65, 68,255, 82, 88,255, 68,255, 69,255, 78,255,
   73,255, 70, 72, 83,255, 69, 85,255, 65, 66, 67, 69, 73, 83, 87,255, 73,
   83,255, 73,255, 68,255, 85,255, 69, 82,255, 84,255, 67, 73, 76, 78, 81,
   82, 88,255, 73, 76, 80,255, 69, 84,255, 77, 84,255, 69, 73, 76,255, 68,
   78,255, 85,255, 76,255, 67, 80, 82, 88,255, 70, 78, 80,255, 80,255, 53,
  255, 67, 68, 75, 88, 90,255, 69, 79,255, 77, 80,255, 53,255, 67, 68, 75,
   77, 78, 80, 84, 88, 90,255, 88,255, 72, 83,255, 65, 69, 72,255, 69,255,
   68, 73, 88,255, 76,255, 78, 83,255, 71, 87,255, 68, 69, 72, 73, 78, 79,
   83, 84, 88,255, 85,255, 68, 86,255, 65, 79, 83, 86,255, 65,255, 80, 84,
  255, 65, 69, 79,255, 65, 71, 73,255, 82, 85,255, 65, 73,255, 66, 83,255,
   65, 67, 79, 82, 83, 85,255, 76, 82,255, 67, 68,255, 67, 90,255, 69, 79,
  255, 67, 72,255, 86,255, 84,255, 65, 67, 68, 69, 73, 76, 77, 78, 80, 82,
   83, 90,255, 66, 73,255, 69,255, 68, 73, 82, 88,255, 76, 82,255, 72,255,
   88,255, 65, 67, 75,255, 66, 73,255, 66, 69, 72, 73, 80, 81, 84, 85, 89,
  255, 69, 73,255, 65, 73,255, 67, 79, 82, 84,255, 33, 35, 36, 38, 39, 40,
   42, 44, 47, 60, 61, 62, 65, 66, 67, 68, 69, 70, 71, 72, 73, 74, 76, 77,
   78, 79, 80, 82, 83, 84, 85, 87, 88, 92,255, 42, 47,255, 35, 36, 38, 47,
  255, 73, 83,255, 76, 78, 82,255, 68, 78,255, 70, 78,255, 68, 69, 73, 80,
   85,255, 42, 47,255, 38, 42, 47, 58,255, 61,255, 42, 47,255, 67, 68, 73,
  255, 65, 73,255, 69, 72,255, 67, 68, 78, 82, 83,255, 76, 89,255, 65, 67,
   80,255, 67, 90,255, 69, 73, 79,255, 65, 67, 77, 78, 80, 83, 90,255, 65,
   68,255, 82, 88,255, 72, 83,255, 69, 85,255, 65, 66, 67, 69, 73, 83, 87,
  255, 73,255, 68,255, 69, 82,255, 84,255, 67, 73, 76, 78, 81, 88,255, 77,
   84,255, 69, 76,255, 82, 88,255, 70, 78,255, 80,255, 53,255, 67, 68, 75,
   88, 90,255, 69, 79,255, 77, 80,255, 53,255, 67, 68, 75, 77, 78, 80, 84,
   88, 90,255, 88,255, 72, 83,255, 65, 69, 72,255, 69,255, 68, 73, 88,255,
   76,255, 78, 83,255, 68, 72, 73, 83, 88,255, 68, 86,255, 65, 79, 83, 86,
  255, 65,255, 80,255, 65, 79,255, 65, 71, 73,255, 82, 85,255, 66, 83,255,
   65, 67, 79, 82, 85,255, 76, 82,255, 67, 68,255, 67, 90,255, 69, 79,255,
   67, 72,255, 86,255, 84,255, 65, 67, 68, 69, 73, 76, 77, 78, 80, 82, 83,
   90,255, 66, 73,255, 69,255, 68, 73, 82, 88,255, 76,255, 88,255, 65, 67,
   75,255, 66, 73,255, 66, 69, 72, 73, 80, 84, 85, 89,255, 69, 73,255, 65,
   73,255, 67, 82, 84,255, 36, 38, 42, 47, 65, 66, 67, 68, 69, 70, 72, 73,
   74, 76, 77, 78, 79, 80, 82, 83, 84, 87, 88,255, 42, 47,255, 85,255, 76,
  255, 67,255, 78,255, 47, 73,255, 61,255, 42, 47,255, 67, 68, 73,255, 65,
   73,255, 69, 72,255, 67, 68, 78, 82, 83,255, 76, 89,255, 65, 67, 80,255,
   67, 90,255, 69, 73, 79,255, 65, 67, 77, 78, 80, 83, 90,255, 65, 68,255,
   82, 88,255, 72, 83,255, 69, 85,255, 65, 66, 67, 69, 73, 83, 87,255, 73,
  255, 68,255, 69, 82,255, 84,255, 67, 73, 76, 78, 88,255, 77, 84,255, 69,
   76,255, 85,255, 76,255, 67, 82, 88,255, 70, 78,255, 80,255, 53,255, 67,
   68, 75, 88, 90,255, 69, 79,255, 77, 80,255, 53,255, 67, 68, 75, 77, 78,
   80, 84, 88, 90,255, 88,255, 72, 83,255, 65, 69, 72,255, 69,255, 68, 73,
   88,255, 76,255, 78, 83,255, 68, 72, 73, 83, 88,255, 68, 86,255, 65, 79,
   83, 86,255, 65,255, 80,255, 65, 79,255, 65, 71, 73,255, 82, 85,255, 66,
   83,255, 65, 67, 79, 82, 85,255, 76, 82,255, 67, 68,255, 67, 90,255, 69,
   79,255, 67, 72,255, 86,255, 84,255, 65, 67, 68, 69, 73, 76, 77, 78, 80,
   82, 83, 90,255, 66, 73,255, 69,255, 68, 73, 82, 88,255, 76,255, 88,255,
   65, 67, 75,255, 66, 73,255, 66, 72, 73, 80, 84, 85, 89,255, 69, 73,255,
   65, 73,255, 67, 82, 84,255, 36, 38, 42, 47, 65, 66, 67, 68, 69, 70, 72,
   73, 74, 76, 77, 78, 79, 80, 82, 83, 84, 87, 88,255, 36, 38,255, 42, 47,
  255, 47,255, 76, 80,255, 71, 87,255, 78, 79,255, 36, 38, 39, 67, 68, 70,
   72, 73, 76, 78, 83,255, 72, 76,255, 42, 47,255, 85,255, 76,255, 67,255,
   78,255, 35, 36, 38, 42, 47, 73,255, 47, 61,255, 42, 47,255, 76, 89,255,
   69,255, 66, 83, 87,255, 69, 82,255, 84,255, 67, 78, 81, 88,255, 85,255,
   76,255, 67,255, 78,255, 78, 83,255, 73, 83,255, 65, 79, 83,255, 65, 79,
  255, 65, 82, 85,255, 69, 84, 89,255, 69, 73,255, 36, 42, 47, 65, 66, 67,
   68, 69, 70, 72, 73, 76, 77, 78, 79, 80, 83, 84, 87, 92,255, 85,255, 76,
  255, 67,255, 78,255, 73,255, 42, 47,255, 42, 47,255, 42, 47, 92,255, 42,
   47,255, 47, 73, 80,255, 42, 47,255, 33, 47,255, 40, 65, 66, 67, 68, 69,
   72, 76, 77,255, 67,255, 69,255, 76,255, 66, 68, 72, 83,255, 67,255, 69,
  255, 76,255, 65, 66, 68, 72, 80,255, 67,255, 69,255, 66, 68,255, 61,255,
   42, 47,255, 60, 61,255, 61,255, 61, 62,255, 33, 42, 44, 47, 60, 61, 62,
  255, 61,255, 42, 47,255, 60, 61,255, 61,255, 61, 62,255, 69, 84,255, 69,
   84,255, 76, 82,255, 72,255, 33, 42, 44, 47, 60, 61, 62, 65, 69, 71, 76,
   77, 78, 79, 83, 88,255, 76, 89,255, 69,255, 66, 83, 87,255, 69, 82,255,
   84,255, 67, 78, 88,255, 78, 83,255, 73, 83,255, 65, 79, 83,255, 65, 79,
  255, 65, 82, 85,255, 84, 89,255, 69, 73,255, 36, 42, 65, 66, 67, 68, 69,
   70, 72, 76, 77, 78, 79, 80, 83, 84, 87,255, 42, 47,255, 44, 47,255, 61,
  255, 42, 47,255, 60, 61,255, 61,255, 61, 62,255, 69, 84,255, 69, 84,255,
   82,255, 76, 82,255, 72,255, 33, 42, 44, 47, 60, 61, 62, 65, 69, 71, 76,
   77, 78, 79, 81, 83, 88,255, 39,255, 61,255, 42, 47,255, 60, 61,255, 61,
  255, 61, 62,255, 69, 84,255, 69, 84,255, 82,255, 76, 82,255, 72,255, 33,
   42, 44, 47, 60, 61, 62, 71, 76, 77, 78, 79, 81, 83, 88,255, 42, 47,255,
   76, 80,255, 71, 87,255, 78, 79,255, 36, 38, 39, 42, 47, 67, 68, 70, 72,
   73, 76, 78, 83, 92,255, 76, 80,255, 71, 87,255, 78, 79,255, 36, 38, 39,
   67, 68, 70, 72, 73, 76, 83,255, 42, 47,255, 76, 80,255, 71, 87,255, 78,
   79,255, 36, 38, 39, 42, 47, 67, 68, 70, 72, 73, 76, 83, 92,255, 61,255,
   60, 61,255, 61,255, 61, 62,255, 69, 84,255, 69, 84,255, 76, 82,255, 72,
  255, 33, 42, 44, 60, 61, 62, 65, 69, 71, 76, 77, 78, 79, 83, 88,255, 61,
  255, 42, 47,255, 60, 61,255, 61,255, 61, 62,255, 69, 84,255, 69, 84,255,
   76, 82,255, 72,255, 33, 44, 47, 60, 61, 62, 65, 69, 71, 76, 78, 79, 83,
   88,255, 61,255, 42, 47,255, 61,255, 61,255, 61,255, 69, 84,255, 69, 84,
  255, 33, 44, 47, 60, 61, 62, 65, 69, 71, 76, 78, 79, 88,255, 61,255, 42,
   47,255, 61,255, 61,255, 61,255, 69, 84,255, 69, 84,255, 33, 44, 47, 60,
   61, 62, 69, 71, 76, 78, 79, 88,255, 61,255, 42, 47,255, 61,255, 61,255,
   61,255, 69, 84,255, 69, 84,255, 33, 44, 47, 60, 61, 62, 69, 71, 76, 78,
   79,255, 42, 47,255, 61,255, 61,255, 61,255, 69, 84,255, 69, 84,255, 33,
   47, 60, 61, 62, 69, 71, 76, 78,255, 61,255, 42, 47,255, 42, 47,255, 60,
   61,255, 61,255, 61, 62,255, 69, 84,255, 69, 84,255, 76, 82,255, 72,255,
   33, 42, 44, 47, 60, 61, 62, 65, 69, 71, 76, 77, 78, 79, 83, 88, 92,255,
   76, 89,255, 69,255, 66, 83, 87,255, 69, 82,255, 84,255, 67, 78, 81, 88,
  255, 78, 83,255, 73, 83,255, 65, 79, 83,255, 65, 79,255, 65, 82, 85,255,
   69, 84, 89,255, 69, 73,255, 36, 42, 65, 66, 67, 68, 69, 70, 72, 76, 77,
   78, 79, 80, 83, 84, 87,255, 39,255, 67, 68, 73,255, 65, 73,255, 67, 68,
   78, 82, 83,255, 65, 67, 80,255, 67, 90,255, 69, 73, 79,255, 65, 67, 77,
   78, 80, 90,255, 65, 68,255, 82, 88,255, 72, 83,255, 65, 67, 69, 73, 83,
  255, 77, 84,255, 76,255, 82, 88,255, 78,255, 80,255, 53,255, 67, 68, 75,
   88, 90,255, 69, 79,255, 77, 80,255, 53,255, 67, 68, 75, 77, 78, 80, 84,
   88, 90,255, 88,255, 72, 83,255, 65, 69, 72,255, 69,255, 68, 73, 88,255,
   76,255, 68, 72, 88,255, 79, 86,255, 65, 73,255, 82, 85,255, 67, 79, 85,
  255, 76, 82,255, 67, 68,255, 67, 90,255, 69, 79,255, 67, 72,255, 86,255,
   84,255, 65, 67, 68, 69, 73, 76, 77, 78, 80, 82, 83, 90,255, 66, 73,255,
   69,255, 68, 73, 82, 88,255, 76,255, 88,255, 65, 67,255, 66, 73,255, 66,
   72, 73, 80, 84, 85,255, 65, 73,255, 67, 82, 84,255, 65, 67, 68, 69, 72,
   73, 74, 76, 77, 78, 79, 80, 82, 83, 88,255, 42, 47,255, 36, 38, 47,255,
   42, 47,255, 33, 44, 47,255, 44,255, 42, 47,255, 47, 79, 81,255, 92,255,
   69,255, 69,255, 76, 80,255, 73,255, 71, 87,255, 78, 79,255, 36, 38, 39,
   40, 65, 66, 67, 68, 69, 70, 72, 73, 76, 77, 78, 83,255, 33,255, 42, 47,
  255, 47, 92,255
};

static const unsigned char ag_key_act[] = {
  0,0,0,4,7,4,6,4,2,4,2,4,0,5,0,2,2,4,0,4,0,4,0,0,0,4,0,0,4,0,0,4,0,4,0,
  0,4,5,5,5,4,5,5,5,4,7,7,4,7,2,2,7,2,4,5,7,7,4,5,5,5,4,5,5,4,5,5,5,4,7,
  5,7,6,2,6,7,5,4,5,5,4,5,5,4,5,4,6,4,2,4,2,4,2,7,7,4,7,7,4,2,5,2,6,5,6,
  5,4,7,7,4,7,4,6,4,5,4,7,7,4,2,4,7,5,2,2,6,7,2,4,7,7,5,4,5,5,4,7,5,4,7,
  7,6,4,7,7,4,7,4,6,4,2,7,5,5,4,6,6,5,4,5,4,5,4,5,5,5,6,5,4,5,5,4,5,5,4,
  5,4,5,5,5,6,2,6,2,6,5,4,5,4,5,5,4,6,2,7,4,5,4,6,5,5,4,2,4,7,7,4,5,5,4,
  2,5,2,2,5,2,7,5,7,4,7,4,6,5,4,7,2,7,7,4,7,4,6,5,4,7,5,2,4,5,5,5,4,6,7,
  4,7,7,4,7,7,4,7,7,7,2,7,2,4,5,5,4,5,7,4,5,5,4,5,5,4,5,7,4,5,4,6,4,2,5,
  7,7,7,2,5,2,6,2,2,5,4,5,5,4,5,4,6,5,5,5,4,6,5,4,7,4,5,4,6,5,7,4,5,5,4,
  2,7,2,7,6,7,2,2,7,4,7,7,4,5,5,4,7,7,2,7,4,6,0,6,0,3,3,2,0,2,1,1,1,6,6,
  6,6,6,2,2,6,2,2,6,6,2,2,2,2,2,2,7,7,2,3,4,0,0,4,0,5,0,2,4,7,7,4,2,7,7,
  4,7,7,4,6,7,4,7,2,2,7,7,4,0,0,4,0,3,2,0,4,0,4,0,0,4,5,5,5,4,5,5,4,7,7,
  4,7,2,2,7,2,4,7,7,4,5,5,5,4,5,5,4,5,5,5,4,7,5,6,2,6,7,5,4,5,5,4,5,5,4,
  7,7,4,7,7,4,2,5,2,2,5,6,5,4,7,4,6,4,7,7,4,2,4,7,5,7,2,7,2,4,7,5,4,7,2,
  4,5,5,4,5,6,4,5,4,5,4,5,5,5,6,5,4,5,5,4,5,5,4,5,4,5,5,5,6,2,6,2,6,5,4,
  5,4,5,5,4,6,2,7,4,5,4,6,5,5,4,2,4,7,7,4,2,2,2,7,7,4,7,5,4,7,2,7,7,4,7,
  4,6,4,7,2,4,5,5,5,4,2,7,4,7,7,4,7,7,7,7,2,4,5,5,4,5,7,4,5,5,4,5,5,4,5,
  7,4,5,4,6,4,2,5,7,7,7,2,5,2,6,2,2,5,4,5,5,4,5,4,6,5,5,5,4,2,4,5,4,6,5,
  7,4,5,5,4,2,7,2,7,7,2,2,7,4,7,7,4,5,5,4,7,2,7,4,6,0,3,2,2,2,2,2,2,7,2,
  2,2,2,2,2,2,2,2,2,2,7,2,4,0,0,4,7,4,6,4,2,4,2,4,2,2,4,0,4,0,0,4,5,5,5,
  4,5,5,4,7,7,4,7,2,2,7,2,4,7,7,4,5,5,5,4,5,5,4,5,5,5,4,7,5,6,2,6,7,5,4,
  5,5,4,5,5,4,7,7,4,7,7,4,2,5,2,2,5,6,5,4,7,4,6,4,7,7,4,2,4,7,5,7,2,2,4,
  7,5,4,7,2,4,7,4,6,4,2,5,5,4,5,6,4,5,4,5,4,5,5,5,6,5,4,5,5,4,5,5,4,5,4,
  5,5,5,6,2,6,2,6,5,4,5,4,5,5,4,6,2,7,4,5,4,6,5,5,4,2,4,7,7,4,2,2,2,7,7,
  4,7,5,4,7,2,7,7,4,7,4,6,4,7,2,4,5,5,5,4,2,7,4,7,7,4,7,7,7,7,2,4,5,5,4,
  5,7,4,5,5,4,5,5,4,5,7,4,5,4,6,4,2,5,7,7,7,2,5,2,6,2,2,5,4,5,5,4,5,4,6,
  5,5,5,4,2,4,5,4,6,5,7,4,5,5,4,2,2,7,7,2,2,7,4,7,7,4,5,5,4,7,2,7,4,6,0,
  3,2,2,2,2,2,2,7,2,2,2,2,2,2,2,2,2,2,2,7,2,4,5,0,4,0,0,4,2,4,7,5,4,5,5,
  4,5,2,4,5,0,3,7,7,2,7,7,2,7,7,4,7,7,4,0,0,4,7,4,6,4,2,4,2,4,0,5,0,3,2,
  2,4,0,0,4,0,0,4,7,7,4,7,4,5,6,5,4,7,7,4,2,4,7,7,7,2,4,7,4,6,4,2,4,2,4,
  7,7,4,2,7,4,7,7,7,4,7,7,4,7,7,7,4,7,7,7,4,7,7,4,3,2,2,7,2,7,2,2,7,7,2,
  2,2,2,7,2,2,2,7,3,4,7,4,6,4,2,4,2,4,2,4,3,3,4,0,0,4,3,2,3,4,0,0,4,2,7,
  7,4,0,0,4,5,2,4,3,5,5,5,5,5,5,5,5,4,5,4,5,4,5,4,6,6,6,7,4,5,4,5,4,5,4,
  5,6,6,6,7,4,5,4,5,4,6,6,4,0,4,0,0,4,0,0,4,0,4,0,0,4,6,3,0,2,1,1,1,4,0,
  4,0,0,4,0,0,4,0,4,0,0,4,5,5,4,5,5,4,5,5,4,2,4,6,3,0,2,1,1,1,7,7,2,2,7,
  7,7,2,7,4,7,7,4,7,4,5,6,5,4,7,7,4,2,4,7,7,2,4,7,7,4,2,7,4,7,7,7,4,7,7,
  4,7,7,7,4,7,7,4,7,7,4,3,3,7,2,7,2,2,7,7,2,2,2,7,2,2,2,7,4,0,0,4,0,2,4,
  0,4,0,0,4,0,0,4,0,4,0,0,4,5,5,4,5,5,4,5,4,5,5,4,2,4,6,3,0,2,1,1,1,7,7,
  2,2,7,7,6,5,2,7,4,3,4,0,4,0,0,4,0,0,4,0,4,0,0,4,5,5,4,5,5,4,5,4,5,5,4,
  2,4,6,3,0,2,1,1,1,2,2,7,7,6,5,2,7,4,0,0,4,7,5,4,5,5,4,5,2,4,5,0,3,3,2,
  7,7,2,7,7,2,7,7,3,4,7,5,4,5,5,4,5,2,4,5,0,3,7,7,2,7,7,2,7,4,0,0,4,7,5,
  4,5,5,4,5,2,4,5,0,3,3,2,7,7,2,7,7,2,7,3,4,0,4,0,0,4,0,4,0,0,4,5,5,4,5,
  5,4,5,5,4,2,4,6,3,0,1,1,1,7,7,2,2,7,7,7,2,7,4,0,4,0,0,4,0,0,4,0,4,0,0,
  4,5,5,4,5,5,4,5,5,4,2,4,6,0,2,1,1,1,7,7,2,2,7,7,2,7,4,0,4,0,0,4,0,4,0,
  4,0,4,5,5,4,5,5,4,6,0,2,1,1,1,7,7,2,2,7,7,7,4,0,4,0,0,4,0,4,0,4,0,4,5,
  5,4,5,5,4,6,0,2,1,1,1,7,2,2,7,7,7,4,0,4,0,0,4,0,4,0,4,0,4,5,5,4,5,5,4,
  6,0,2,1,1,1,7,2,2,7,7,4,0,0,4,0,4,0,4,0,4,5,5,4,5,5,4,3,2,1,1,1,7,2,2,
  7,4,0,4,0,0,4,0,0,4,0,0,4,0,4,0,0,4,5,5,4,5,5,4,5,5,4,2,4,6,2,0,2,1,1,
  1,7,7,2,2,7,7,7,2,7,3,4,7,7,4,7,4,5,6,5,4,7,7,4,2,4,7,7,7,2,4,7,7,4,2,
  7,4,7,7,7,4,7,7,4,7,7,7,4,7,7,7,4,7,7,4,3,3,7,2,7,2,2,7,7,2,2,2,7,2,2,
  2,7,4,3,4,5,5,5,4,5,5,4,7,2,2,7,7,4,5,5,5,4,5,5,4,5,5,5,4,7,5,6,2,6,5,
  4,5,5,4,5,5,4,7,7,4,2,2,2,5,7,4,7,5,4,2,4,5,5,4,6,4,5,4,5,4,5,5,5,6,5,
  4,5,5,4,5,5,4,5,4,5,5,5,6,2,6,2,6,5,4,5,4,5,5,4,6,2,7,4,5,4,6,5,5,4,2,
  4,2,2,7,4,7,7,4,5,5,4,2,7,4,7,7,7,4,5,5,4,5,7,4,5,5,4,5,5,4,5,7,4,5,4,
  6,4,2,5,7,7,7,2,5,2,6,2,2,5,4,5,5,4,5,4,6,5,5,5,4,2,4,5,4,6,5,4,5,5,4,
  2,2,7,7,2,2,4,5,5,4,7,2,7,4,2,2,2,7,2,2,2,2,2,7,2,2,2,2,2,4,0,0,4,5,0,
  2,4,0,0,4,5,0,2,4,0,4,0,0,4,2,5,5,4,3,4,7,4,7,4,7,5,4,7,4,5,5,4,5,2,4,
  5,0,3,3,5,5,6,6,5,2,6,7,6,5,7,7,4,5,4,0,0,4,2,3,4
};

static const unsigned short ag_key_parm[] = {
    0,396,393,  0,  4,  0,  6,  0,  0,  0,  0,  0,406, 90,132,  0,  0,  0,
  464,  0,427,  0,489,395,428,  0,396,393,  0,479,468,  0,461,  0,466,481,
    0,264,266,300,  0,268,110,302,  0, 38,172,  0,298,  0,  0,176,  0,  0,
  160, 42, 46,  0,180,182,270,  0,310,312,  0,316,318,320,  0,304,306,122,
  308,  0,314, 32,322,  0,184,286,  0,272,288,  0,138,  0, 30,  0,  0,  0,
    0,  0,  0,368,374,  0, 34,188,  0,  0, 44,  0,162,186, 40, 50,  0, 18,
   22,  0, 24,  0, 72,  0,  0,  0, 58, 56,  0,  0,  0, 66,192,  0,  0, 94,
   26,  0,  0, 68,120,132,  0, 98,102,  0,190,194,  0, 12,134,164,  0, 14,
   20,  0,  4,  0,  6,  0,  0, 92,274,290,  0, 16,324,130,  0,340,  0,346,
    0,342,352,350,344,354,  0,358,360,  0,332,348,  0,330,  0,326,336,334,
  338,  0,356,  0,328,362,  0,258,  0,366,372,  0,364,  0,370,  0,200,  0,
  376,198,196,  0,  0,  0, 84, 10,  0,126,136,  0,  0,100,  0,  0,124,  0,
   76,104,292,  0, 60,  0,116,276,  0, 86,  0, 74,294,  0, 80,  0,202,118,
    0, 62, 96,  0,  0,278, 36,378,  0,106,380,  0,  8, 70,  0, 54,262,  0,
   88,204,260,  0,168,  0,  0,206,208,  0,216,218,  0,224,226,  0,230,232,
    0,234,178,  0,236,  0,296,  0,  0,210,220,212,214,  0,222,  0,228,  0,
    0,238,  0,280,382,  0,244,  0,384,242,174,240,  0,112,114,  0,248,  0,
  256,  0,386,250, 64,  0,282,388,  0,  0,  2,  0,246,166,128,  0,  0, 82,
    0, 48, 78,  0,284,390,  0,252,108,  0,254,  0,170,406, 90,132,226,236,
    0,458,  0,472,462,470,158,144,146,148,150,  0,  0,152,  0,  0,154,156,
    0,  0,  0,  0,  0,  0, 28, 52,  0,423,  0,396,393,  0,406, 90,132,  0,
    0, 18, 22,  0,  0, 24, 26,  0, 14, 20,  0, 16,  4,  0, 30,  0,  0,  8,
   28,  0,396,393,  0,132,428,  0,158,  0,427,  0,396,393,  0,264,266,300,
    0,268,302,  0, 38,172,  0,298,  0,  0,176,  0,  0, 42, 46,  0,180,182,
  270,  0,310,312,  0,316,318,320,  0,304,306,308,  0,314, 32,322,  0,184,
  286,  0,272,288,  0,368,374,  0, 34,188,  0,  0, 44,  0,  0,186, 40, 50,
    0, 24,  0, 72,  0, 58, 56,  0,  0,  0, 66,192, 22,  0,  0,  0,  0,190,
  194,  0, 12,  0,  0,274,290,  0, 16,324,  0,340,  0,346,  0,342,352,350,
  344,354,  0,358,360,  0,332,348,  0,330,  0,326,336,334,338,  0,356,  0,
  328,362,  0,258,  0,366,372,  0,364,  0,370,  0,200,  0,376,198,196,  0,
    0,  0, 84, 10,  0,  0,  0,  0, 76,292,  0, 60,276,  0, 86,  0, 74,294,
    0, 80,  0,202,  0, 62,  0,  0,278, 36,378,  0,  0,380,  0, 54,262,  0,
   88,204,260, 70,  0,  0,206,208,  0,216,218,  0,224,226,  0,230,232,  0,
  234,178,  0,236,  0,296,  0,  0,210,220,212,214,  0,222,  0,228,  0,  0,
  238,  0,280,382,  0,244,  0,384,242,174,240,  0,  0,  0,256,  0,386,250,
   64,  0,282,388,  0,  0,  2,  0,246,248,  0,  0, 82,  0, 48, 78,  0,284,
  390,  0,252,  0,254,  0, 90,132,428,  0,  0,  0,  0,  0,  0, 68,  0,  0,
    0,  0,  0,  0,  0,  0,  0,  0,  0, 52,  0,  0,396,393,  0,  4,  0,  6,
    0,  0,  0,  0,  0,  0,  0,  0,427,  0,396,393,  0,264,266,300,  0,268,
  302,  0, 38,172,  0,298,  0,  0,176,  0,  0, 42, 46,  0,180,182,270,  0,
  310,312,  0,316,318,320,  0,304,306,308,  0,314, 32,322,  0,184,286,  0,
  272,288,  0,368,374,  0, 34,188,  0,  0, 44,  0,  0,186, 40, 50,  0, 24,
    0, 72,  0, 58, 56,  0,  0,  0, 66,192, 22,  0,  0,  0,190,194,  0, 12,
    0,  0,  4,  0,  6,  0,  0,274,290,  0, 16,324,  0,340,  0,346,  0,342,
  352,350,344,354,  0,358,360,  0,332,348,  0,330,  0,326,336,334,338,  0,
  356,  0,328,362,  0,258,  0,366,372,  0,364,  0,370,  0,200,  0,376,198,
  196,  0,  0,  0, 84, 10,  0,  0,  0,  0, 76,292,  0, 60,276,  0, 86,  0,
   74,294,  0, 80,  0,202,  0, 62,  0,  0,278, 36,378,  0,  0,380,  0, 54,
  262,  0, 88,204,260, 70,  0,  0,206,208,  0,216,218,  0,224,226,  0,230,
  232,  0,234,178,  0,236,  0,296,  0,  0,210,220,212,214,  0,222,  0,228,
    0,  0,238,  0,280,382,  0,244,  0,384,242,174,240,  0,  0,  0,256,  0,
  386,250, 64,  0,282,388,  0,  0,  0,246,248,  0,  0, 82,  0, 48, 78,  0,
  284,390,  0,252,  0,254,  0, 90,132,428,  0,  0,  0,  0,  0,  0, 68,  0,
    0,  0,  0,  0,  0,  0,  0,  0,  0,  0, 52,  0,  0, 90,132,  0,396,393,
    0,  0,  0,120,132,  0,126,136,  0,124,  0,  0, 90,132,226,122,138,  0,
  134,130,  0,118,128,  0, 12, 10,  0,396,393,  0,  4,  0,  6,  0,  0,  0,
    0,  0,406, 90,132,395,  0,  0,  0,395,428,  0,396,393,  0, 42, 46,  0,
   34,  0, 44, 40, 50,  0, 58, 56,  0,  0,  0, 66, 72,  0,  0,  0,  4,  0,
    6,  0,  0,  0,  0,  0, 84, 10,  0,  0, 76,  0, 86, 60, 74,  0, 62, 80,
    0, 88, 70, 54,  0,  2, 64, 82,  0, 48, 78,  0,427,  0,  0, 38,  0, 32,
    0,  0, 68, 12,  0,  0,  0,  0, 36,  0,  0,  0, 52,423,  0,  4,  0,  6,
    0,  0,  0,  0,  0,  0,  0,395,396,  0,396,393,  0,395,  0,423,  0,396,
  393,  0,  0, 92, 88,  0,396,393,  0,170,  0,  0,236,158,144,146,148,150,
  152,154,156,  0,160,  0,162,  0,164,  0,144,148,152,166,  0,160,  0,162,
    0,164,  0,158,144,148,152,168,  0,160,  0,162,  0,144,148,  0,464,  0,
  396,393,  0,479,468,  0,461,  0,466,481,  0,170,489,458,  0,472,462,470,
    0,464,  0,396,393,  0,479,468,  0,461,  0,466,481,  0, 98,102,  0,100,
  104,  0,112,114,  0,  0,  0,170,489,458,  0,472,462,470,110, 94,  0,  0,
  116, 96,106,  0,108,  0, 42, 46,  0, 34,  0, 44, 40, 50,  0, 58, 56,  0,
    0,  0, 66, 72,  0,  0, 84, 10,  0,  0, 76,  0, 86, 60, 74,  0, 62, 80,
    0, 88, 70, 54,  0, 64, 82,  0, 48, 78,  0,427,428, 38,  0, 32,  0,  0,
   68, 12,  0,  0,  0, 36,  0,  0,  0, 52,  0,396,393,  0,458,  0,  0,464,
    0,396,393,  0,479,468,  0,461,  0,466,481,  0, 98,102,  0,100,104,  0,
  106,  0,112,114,  0,  0,  0,170,489,458,  0,472,462,470,110, 94,  0,  0,
  116, 96,142,140,  0,108,  0,228,  0,464,  0,396,393,  0,479,468,  0,461,
    0,466,481,  0, 98,102,  0,100,104,  0,106,  0,112,114,  0,  0,  0,170,
  489,458,  0,472,462,470,  0,  0,116, 96,142,140,  0,108,  0,396,393,  0,
  120,132,  0,126,136,  0,124,  0,  0, 90,132,226,395,  0,122,138,  0,134,
  130,  0,118,128,423,  0,120,132,  0,126,136,  0,124,  0,  0, 90,132,226,
  122,138,  0,134,130,  0,128,  0,396,393,  0,120,132,  0,126,136,  0,124,
    0,  0, 90,132,226,395,  0,122,138,  0,134,130,  0,128,423,  0,464,  0,
  479,468,  0,461,  0,466,481,  0, 98,102,  0,100,104,  0,112,114,  0,  0,
    0,170,489,458,472,462,470,110, 94,  0,  0,116, 96,106,  0,108,  0,464,
    0,396,393,  0,479,468,  0,461,  0,466,481,  0, 98,102,  0,100,104,  0,
  112,114,  0,  0,  0,170,458,  0,472,462,470,110, 94,  0,  0, 96,106,  0,
  108,  0,464,  0,396,393,  0,468,  0,461,  0,466,  0, 98,102,  0,100,104,
    0,170,458,  0,472,462,470,110, 94,  0,  0, 96,106,108,  0,464,  0,396,
  393,  0,468,  0,461,  0,466,  0, 98,102,  0,100,104,  0,170,458,  0,472,
  462,470, 94,  0,  0, 96,106,108,  0,464,  0,396,393,  0,468,  0,461,  0,
  466,  0, 98,102,  0,100,104,  0,170,458,  0,472,462,470, 94,  0,  0, 96,
  106,  0,396,393,  0,468,  0,461,  0,466,  0, 98,102,  0,100,104,  0,464,
    0,472,462,470, 94,  0,  0, 96,  0,464,  0,489,395,  0,396,393,  0,479,
  468,  0,461,  0,466,481,  0, 98,102,  0,100,104,  0,112,114,  0,  0,  0,
  170,  0,458,  0,472,462,470,110, 94,  0,  0,116, 96,106,  0,108,423,  0,
   42, 46,  0, 34,  0, 44, 40, 50,  0, 58, 56,  0,  0,  0, 66, 72,  0,  0,
    0, 84, 10,  0,  0, 76,  0, 86, 60, 74,  0, 62, 80,  0, 88, 70, 54,  0,
    2, 64, 82,  0, 48, 78,  0,427,428, 38,  0, 32,  0,  0, 68, 12,  0,  0,
    0, 36,  0,  0,  0, 52,  0,226,  0,264,266,300,  0,268,302,  0,298,  0,
    0,176,172,  0,180,182,270,  0,310,312,  0,316,318,320,  0,304,306,308,
    0,314,322,  0,184,286,  0,272,288,  0,368,374,  0,  0,  0,  0,186,188,
    0,190,194,  0,  0,  0,274,290,  0,324,  0,340,  0,346,  0,342,352,350,
  344,354,  0,358,360,  0,332,348,  0,330,  0,326,336,334,338,  0,356,  0,
  328,362,  0,258,  0,366,372,  0,364,  0,370,  0,200,  0,376,198,196,  0,
    0,  0,  0,  0,292,  0,276,294,  0,278,378,  0,  0,380,  0,204,260,262,
    0,206,208,  0,216,218,  0,224,226,  0,230,232,  0,234,178,  0,236,  0,
  296,  0,  0,210,220,212,214,  0,222,  0,228,  0,  0,238,  0,280,382,  0,
  244,  0,384,242,174,240,  0,  0,  0,256,  0,386,250,  0,282,388,  0,  0,
    0,246,248,  0,  0,  0,284,390,  0,252,  0,254,  0,  0,  0,  0,192,  0,
    0,  0,  0,  0,202,  0,  0,  0,  0,  0,  0,396,393,  0, 90,132,  0,  0,
  396,393,  0,170,458,  0,  0,458,  0,396,393,  0,  0,142,140,  0,423,  0,
  122,  0,138,  0,120,132,  0,134,  0,126,136,  0,124,  0,  0, 90,132,226,
  236,158,144,146,148,150,  0,152,130,154,156,118,128,  0,170,  0,396,393,
    0,  0,423,  0
};

static const unsigned short ag_key_jmp[] = {
    0,  0,  0,  0,  0,  0,  4,  0,  6,  0,  8,  0,  0,  0,  0,  1, 10,  0,
    0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,
    0,  0,  0,  0,  0,  0,  0,  0,  0, 15, 17,  0, 10, 37, 41, 12, 45,  0,
    0, 19, 23,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0, 26,  0, 29,
   58, 62, 65, 32,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0, 84,  0, 86,  0,
   88,  0, 90, 35, 37,  0, 39, 41,  0, 78,  0, 81, 92,  0, 96,  0,  0, 46,
   48,  0, 50,  0,110,  0,  0,  0, 56, 59,  0,116,  0, 43,  0,107,112,114,
   52,119,  0, 61, 64,  0,  0,  0,  0,  0, 73,  0,  0, 68, 70,136,  0, 76,
   79,  0, 83,  0,146,  0,148, 86,  0,  0,  0,143,150,  0,  0,  0,  0,  0,
    0,  0,  0,  0,161,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,
  159,163,169,172,175,  0,  0,  0,  0,  0,  0,  0,187,189, 90,  0,  0,  0,
  196,  0,  0,  0,198,  0, 92, 94,  0,  0,  0,  0,192,  0,202,204,  0,207,
   96,  0,102,  0,109,  0,220,  0,  0,104,222,112,118,  0,123,  0,230,  0,
    0,120,  0,232,  0,  0,  0,  0,  0,239,126,  0,136,140,  0,146,150,  0,
  128,131,134,246,144,249,  0,  0,  0,  0,  0,159,  0,  0,  0,  0,  0,  0,
    0,  0,161,  0,  0,  0,274,  0,259,  0,152,155,157,262,  0,265,268,271,
  276,  0,  0,  0,  0,  0,  0,  0,294,  0,  0,  0,  0,296,  0,  0,167,  0,
    0,  0,306,  0,172,  0,  0,  0,  0,291,163,301,165,304,169,308,312,175,
    0,177,180,  0,  0,  0,  0,193,196,328,198,  0, 18,  0, 20,  0,  3,  6,
   22,  0, 26, 29, 32, 34, 48, 54, 69, 99,121,129,133,139,155,177,210,225,
  235,243,252,278,315,325,184,189,331,201,  0,  0,  0,  0,  0,  0,  0,371,
    0,209,211,  0,379,213,217,  0,221,224,  0,386,228,  0,203,382,389,234,
  240,  0,  0,  0,  0,  0,245,398,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,
    0,  0,  0,  0,254,256,  0,249,411,415,251,418,  0,258,262,  0,  0,  0,
    0,  0,  0,  0,  0,  0,  0,  0,  0,265,  0,430,434,437,268,  0,  0,  0,
    0,  0,  0,  0,  0,271,273,  0,275,277,  0,449,  0,452,455,  0,458,  0,
    0,285,  0,469,  0,289,292,  0,473,  0,279,  0,282,471,287,476,  0,300,
    0,  0,298,485,  0,  0,  0,  0,  0,491,  0,  0,  0,  0,  0,  0,  0,  0,
  499,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,497,501,507,510,
  513,  0,  0,  0,  0,  0,  0,  0,525,527,303,  0,  0,  0,534,  0,  0,  0,
  536,  0,305,307,  0,530,540,542,309,315,  0,322,  0,  0,317,551,326,332,
    0,337,  0,559,  0,334,561,  0,  0,  0,  0,  0,566,340,  0,355,359,  0,
  342,345,348,350,573,  0,  0,  0,  0,  0,368,  0,  0,  0,  0,  0,  0,  0,
    0,370,  0,  0,  0,597,  0,582,  0,361,364,366,585,  0,588,591,594,599,
    0,  0,  0,  0,  0,  0,  0,617,  0,  0,  0,  0,619,  0,  0,  0,626,  0,
  379,  0,  0,  0,  0,614,372,624,374,376,628,632,382,  0,384,387,  0,  0,
    0,  0,395,647,398,  0,406,  0,247,408,421,427,441,461,478,294,488,494,
  515,545,554,563,570,576,601,635,644,391,650,  0,  0,  0,  0,401,  0,681,
    0,683,  0,685,  0,678,687,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,
    0,  0,411,413,  0,406,697,701,408,704,  0,415,419,  0,  0,  0,  0,  0,
    0,  0,  0,  0,  0,  0,  0,422,  0,716,720,723,425,  0,  0,  0,  0,  0,
    0,  0,  0,428,430,  0,432,434,  0,735,  0,738,741,  0,744,  0,  0,442,
    0,755,  0,444,447,  0,759,  0,436,  0,439,757,762,  0,455,  0,  0,453,
  770,  0,458,  0,776,  0,778,  0,  0,  0,  0,780,  0,  0,  0,  0,  0,  0,
    0,  0,789,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,787,791,
  797,800,803,  0,  0,  0,  0,  0,  0,  0,815,817,461,  0,  0,  0,824,  0,
    0,  0,826,  0,463,465,  0,820,830,832,467,473,  0,480,  0,  0,475,841,
  484,490,  0,495,  0,849,  0,492,851,  0,  0,  0,  0,  0,856,498,  0,513,
  517,  0,500,503,506,508,863,  0,  0,  0,  0,  0,526,  0,  0,  0,  0,  0,
    0,  0,  0,528,  0,  0,  0,887,  0,872,  0,519,522,524,875,  0,878,881,
  884,889,  0,  0,  0,  0,  0,  0,  0,907,  0,  0,  0,  0,909,  0,  0,  0,
  916,  0,535,  0,  0,  0,  0,904,914,530,532,918,922,538,  0,540,543,  0,
    0,  0,  0,551,936,554,  0,692,  0,404,694,707,713,727,747,764,449,773,
  784,805,835,844,853,860,866,891,925,933,547,939,  0,  0,  0,  0,  0,  0,
    0,970,  0,571,  0,  0,  0,  0,  0,  0,978,  0,  0,  0,557,560,564,975,
  575,579,981,581,584,  0,588,591,  0,  0,  0,  0,597,  0,1002,  0,1004,
    0,1006,  0,  0,  0,  0,595,999,1008,  0,  0,  0,  0,  0,  0,  0,606,
  610,  0,617,  0,  0,1026,  0,  0,626,629,  0,1032,  0,619,622,624,1035,
    0,638,  0,1042,  0,1044,  0,1046,  0,641,643,  0,1050,645,  0,651,656,
  661,  0,667,670,  0,678,681,686,  0,691,693,697,  0,699,702,  0,600,1017,
  1020,602,1023,613,1028,1037,631,635,1048,1053,1056,1060,675,1063,1067,
  1071,706,710,  0,712,  0,1095,  0,1097,  0,1099,  0,1101,  0,715,717,  0,
    0,  0,  0,719,1108,721,  0,  0,  0,  0,1115,723,729,  0,  0,  0,  0,
    0,1122,  0,733,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,
    0,1138,1140,1142,737,  0,  0,  0,  0,  0,  0,  0,  0,1149,1151,1153,
  739,  0,  0,  0,  0,  0,1161,1163,  0,  0,  0,  0,  0,  0,  0,  0,  0,
    0,  0,  0,  0,  0,1168,742,  0,1170,1173,1176,1178,  0,  0,  0,  0,  0,
    0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,
  1208,  0,1189,744,  0,1191,1194,1197,1199,746,749,1202,1205,751,754,756,
  1211,758,  0,769,773,  0,780,  0,  0,1233,  0,  0,787,790,  0,1239,  0,
  782,785,1242,  0,799,801,  0,1248,803,  0,809,814,819,  0,825,828,  0,
  836,839,844,  0,849,853,  0,855,858,  0,761,763,765,1230,776,1235,1244,
  792,796,1251,1254,1258,833,1261,1265,1268,862,  0,  0,  0,  0,  0,1289,
    0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,
    0,  0,  0,  0,  0,  0,  0,1316,  0,1295,866,  0,1297,1300,1303,1305,
  868,871,1308,1311,873,876,1314,  0,1319,878,  0,881,  0,  0,  0,  0,  0,
    0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,
    0,  0,1362,  0,1341,883,  0,1343,1346,1349,1351,1354,1357,885,888,1360,
    0,1365,890,  0,  0,  0,  0,909,  0,  0,  0,  0,  0,  0,1389,  0,  0,
    0,893,896,1383,898,902,1386,913,917,1392,919,922,926,  0,942,  0,  0,
    0,  0,  0,  0,1413,  0,  0,  0,928,931,935,1410,946,950,1416,952,  0,
    0,  0,  0,972,  0,  0,  0,  0,  0,  0,1436,  0,  0,  0,956,959,1430,
  961,965,1433,976,980,1439,982,986,  0,  0,  0,  0,  0,  0,  0,  0,  0,
    0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,1472,  0,1456,988,  0,1458,
  1461,1463,990,993,1466,1469,995,998,1000,1475,1002,  0,  0,  0,  0,  0,
    0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,
  1512,  0,1493,  0,1495,1498,1501,1503,1005,1008,1506,1509,1010,1012,1515,
  1014,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,
    0,1532,  0,1534,1537,1539,1541,1017,1020,1543,1546,1022,1024,1026,  0,
    0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,1563,
    0,1565,1568,1570,1572,1029,1574,1577,1031,1033,1035,  0,  0,  0,  0,
    0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,1593,  0,1595,
  1598,1600,1602,1038,1604,1607,1040,1042,  0,  0,  0,  0,  0,  0,  0,  0,
    0,  0,  0,  0,  0,  0,  0,  0,1044,1622,1625,1627,1629,1046,1631,1634,
  1048,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,
    0,  0,  0,  0,  0,  0,  0,  0,  0,1669,  0,1647,1649,  0,1652,1655,1658,
  1660,1050,1053,1663,1666,1055,1058,1060,1672,1062,1065,  0,1075,1079,  0,
  1086,  0,  0,1695,  0,  0,1095,1098,  0,1701,  0,1088,1091,1093,1704,  0,
  1107,1109,  0,1711,1111,  0,1117,1122,1127,  0,1133,1136,  0,1144,1147,
  1152,  0,1157,1159,1163,  0,1165,1168,  0,1067,1069,1071,1692,1082,1697,
  1706,1100,1104,1714,1717,1721,1141,1724,1728,1732,1172,  0,1176,  0,  0,
    0,  0,  0,  0,  0,  0,1179,1755,1759,1181,1184,  0,  0,  0,  0,  0,  0,
    0,  0,  0,  0,  0,  0,1187,  0,1768,1772,1775,  0,  0,  0,  0,  0,  0,
    0,  0,1190,1192,  0,1786,1789,1792,  0,1194,  0,1199,  0,  0,1801,  0,
    0,  0,  0,1806,  0,  0,  0,  0,  0,  0,  0,  0,1813,  0,  0,  0,  0,
    0,  0,  0,  0,  0,  0,  0,  0,  0,1811,1815,1821,1824,1827,  0,  0,  0,
    0,  0,  0,  0,1839,1841,1202,  0,  0,  0,1848,  0,  0,  0,1850,  0,1844,
  1854,1204,  0,1206,1208,  0,  0,  0,  0,1863,1213,  0,1215,1218,1220,  0,
    0,  0,  0,  0,1230,  0,  0,  0,  0,  0,  0,  0,  0,1232,  0,  0,  0,
  1888,  0,1873,  0,1223,1226,1228,1876,  0,1879,1882,1885,1890,  0,  0,
    0,  0,  0,  0,  0,1908,  0,  0,  0,  0,1910,  0,  0,  0,1917,  0,  0,
    0,  0,  0,1905,1915,1234,1236,1919,1922,  0,  0,  0,  0,1239,1932,1242,
    0,1762,1779,1795,1197,1804,1809,1829,1856,1860,1210,1866,1869,1892,1925,
  1935,  0,  0,  0,  0,  0,  0,1955,  0,  0,  0,  0,  0,  0,1962,  0,  0,
    0,  0,  0,  0,1971,  0,  0,  0,1245,  0,1254,  0,1257,  0,1263,  0,  0,
  1267,  0,  0,  0,  0,  0,1989,  0,  0,  0,1247,1250,  0,  0,1980,1982,
    0,1984,1987,1270,1992,  0,1272,1275,  0,  0,  0,  0,  0,  0,2014,1279,
    0
};

static const unsigned short ag_key_index[] = {
   12,336,374,392,401,392,654,  0,689,654,943,374,967,967,  0,973,973,967,
  984,984,967,996,  0,967,967,  0,973,973,967,984,984,967,996,  0,1010,1074,
  1103,1105,1111,  0,1105,  0,  0,654,1118,1125,1125,1125,1125,1125,1125,
  1125,1125,1125,1125,1125,1125,1125,1125,1128,1144,984,984,984,984,984,
  984,984,984,984,984,984,984,984,984,984,1125,1125,1125,1125,1125,1125,
  1125,1125,1125,1125,1125,1125,1125,1125,1125,1125,1125,1125,1125,1125,
  1125,1125,1125,1125,1125,1125,1125,1125,  0,1128,1144,984,984,984,984,
  984,984,984,984,984,984,984,984,984,984,984,984,984,984,984,984,984,984,
  984,984,984,984,984,984,984,984,984,984,1155,1155,1165,1165,1144,1144,
  1144,1128,1128,1128,1128,1128,1128,1128,1128,1128,1128,1128,  0,  0,  0,
  984,984,984,984,984,984,984,984,984,984,984,984,984,984,984,984,984,984,
  984,984,984,984,984,984,984,984,984,984,984,984,984,984,984,984,984,984,
  1155,1155,1165,1165,1144,1144,1144,1128,1128,1128,1128,1128,1128,1128,
  1128,1128,1128,1128,1181,1125,1213,973,1125,984,1271,973,  0,  0,973,  0,
  1292,973,973,973,973,  0,  0,  0,1213,1321,  0,1339,1213,  0,1213,1213,
  1213,1367,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,1213,1213,1395,  0,  0,
    0,  0,  0,  0,  0,  0,  0,  0,1419,1442,1419,1442,984,1419,1419,1419,
  1419,1213,1477,1517,1517,1549,1580,1610,1637,984,973,973,973,973,973,973,
  973,  0,973,973,973,  0,  0,1111,1395,1111,1395,1105,1111,1111,1111,1111,
  1111,1111,1111,1111,1111,1111,1111,1111,1111,1111,1111,1111,1111,1111,
  1111,1442,1395,1395,1395,1442,1674,1111,1395,1442,1111,1105,1111,1111,
  1010,  0,  0,  0,1735,973,1118,1128,1144,  0,  0,  0,984,984,984,984,984,
  984,984,984,984,984,984,984,984,984,984,984,984,984,984,984,984,984,984,
  984,984,984,984,984,984,984,984,984,984,984,984,984,1155,1155,1165,1165,
  1144,1144,1144,1128,1128,1128,1128,1128,1128,1128,1128,1128,1128,1128,
  984,1753,1939,1939,973,967,967,  0,  0,973,973,  0,  0,973,973,1958,1958,
    0,  0,984,984,984,984,984,984,967,967,967,967,967,967,967,967,967,967,
  984,984,984,984,984,984,984,984,984,984,973,984,984,984,984,1118,1118,
  973,  0,  0,973,  0,973,973,  0,  0,1339,1213,1213,  0,  0,  0,  0,  0,
    0,  0,  0,  0,  0,984,  0,1419,1419,1419,1419,1419,1419,1419,984,984,
  984,984,984,984,984,984,984,984,984,984,984,984,984,984,984,984,984,984,
  984,984,984,984,984,984,984,984,984,984,984,984,973,973,973,973,973,  0,
  973,973,  0,973,973,973,984,984,984,984,973,973,973,1965,1125,984,984,
  1125,1125,1969,1339,1339,1292,1969,  0,1974,1939,1339,  0,984,1292,1292,
  1292,1292,1292,  0,  0,1978,  0,  0,  0,  0,967,984,984,984,984,984,984,
  984,984,984,  0,1419,1419,1419,1419,1419,984,984,984,984,984,984,984,984,
  984,984,984,984,973,973,973,973,973,973,984,984,1969,1339,1995,984,2012,
  1125,1339,984,1292,967,2017,2017,973,  0,  0,967,  0,  0,  0,  0,  0,  0,
    0,  0,  0,  0,1477,1477,1517,1517,1517,1517,1517,1517,1549,1549,1580,
  1580,973,973,1128,1339,984,984,967,  0,  0,  0,1128,  0
};

static const unsigned char ag_key_ends[] = {
68,69,0, 92,39,0, 72,76,41,0, 73,0, 72,76,0, 71,0, 82,0, 
79,67,75,0, 84,69,0, 76,76,0, 73,76,0, 69,71,0, 76,0, 80,0, 
71,0, 66,0, 72,79,0, 70,0, 69,0, 70,0, 82,79,82,0, 82,78,0, 
78,0, 76,76,0, 79,79,82,0, 88,0, 71,72,0, 66,67,0, 69,70,0, 
68,69,70,0, 68,69,0, 65,71,69,0, 73,0, 75,0, 84,0, 
70,73,82,83,84,0, 73,0, 67,76,73,66,0, 76,69,0, 70,73,82,83,84,0, 
73,0, 77,69,0, 71,69,0, 84,0, 71,69,0, 72,76,0, 80,0, 
71,77,65,0, 78,84,70,0, 87,0, 76,73,67,0, 72,0, 69,76,0, 84,0, 
77,0, 69,0, 76,0, 84,0, 77,0, 76,0, 82,84,0, 76,78,0, 77,0, 
88,84,0, 84,76,69,0, 78,68,69,70,0, 79,82,68,0, 72,71,0, 82,0, 
72,76,0, 10,0, 69,70,73,78,69,0, 70,0, 69,0, 68,73,70,0, 
82,79,82,0, 69,70,0, 68,69,70,0, 67,76,85,68,69,0, 
82,65,71,77,65,0, 78,68,69,70,0, 61,0, 61,0, 73,0, 72,76,0, 
71,0, 82,0, 79,67,75,0, 84,69,0, 76,76,0, 69,71,0, 76,0, 80,0, 
71,0, 66,0, 72,79,0, 83,69,0, 70,0, 85,0, 82,78,0, 78,0, 
73,76,76,0, 88,0, 66,67,0, 73,0, 75,0, 84,0, 70,73,82,83,84,0, 
73,0, 67,76,73,66,0, 85,76,69,0, 70,73,82,83,84,0, 73,0, 
77,69,0, 71,69,0, 84,0, 71,69,0, 72,76,0, 80,0, 73,78,84,70,0, 
76,73,67,0, 72,0, 69,76,0, 84,0, 77,0, 69,0, 76,0, 84,0, 
77,0, 72,76,0, 76,78,0, 77,0, 88,84,0, 84,76,69,0, 79,82,68,0, 
72,71,0, 72,76,0, 68,69,0, 61,0, 73,0, 72,76,0, 71,0, 82,0, 
79,67,75,0, 84,69,0, 76,76,0, 69,71,0, 76,0, 80,0, 71,0, 66,0, 
72,79,0, 83,69,0, 70,0, 82,78,0, 78,0, 73,76,76,0, 88,0, 
66,67,0, 68,69,0, 73,0, 75,0, 84,0, 70,73,82,83,84,0, 73,0, 
67,76,73,66,0, 85,76,69,0, 70,73,82,83,84,0, 73,0, 77,69,0, 
71,69,0, 84,0, 71,69,0, 72,76,0, 80,0, 73,78,84,70,0, 
76,73,67,0, 72,0, 69,76,0, 84,0, 77,0, 69,0, 76,0, 77,0, 
72,76,0, 76,78,0, 77,0, 88,84,0, 84,76,69,0, 79,82,68,0, 
72,71,0, 72,76,0, 92,39,0, 69,73,76,0, 69,70,73,78,69,68,0, 
79,79,82,0, 73,71,72,0, 80,0, 79,84,0, 81,82,84,0, 69,88,0, 
73,83,84,0, 47,0, 68,69,0, 61,0, 83,69,71,0, 79,67,75,0, 
84,69,0, 83,69,71,0, 71,0, 72,79,0, 68,0, 85,0, 82,78,0, 78,0, 
73,76,76,0, 69,88,0, 68,69,0, 75,0, 84,0, 70,73,82,83,84,0, 
67,76,73,66,0, 68,85,76,69,0, 70,73,82,83,84,0, 77,69,0, 
80,65,71,69,0, 82,71,0, 71,69,0, 73,78,84,70,0, 66,76,73,67,0, 
84,0, 75,76,78,0, 77,0, 88,84,0, 84,76,69,0, 79,82,68,0, 10,0, 
68,69,0, 47,0, 42,0, 47,0, 10,0, 78,80,65,71,69,0, 65,71,69,0, 
72,76,41,0, 80,0, 83,87,0, 42,0, 42,0, 78,68,0, 81,0, 79,68,0, 
69,0, 82,0, 79,82,0, 61,0, 61,0, 83,69,71,0, 79,67,75,0, 
84,69,0, 83,69,71,0, 71,0, 72,79,0, 68,0, 82,78,0, 78,0, 
73,76,76,0, 69,88,0, 75,0, 84,0, 70,73,82,83,84,0, 
67,76,73,66,0, 68,85,76,69,0, 70,73,82,83,84,0, 77,69,0, 
80,65,71,69,0, 82,71,0, 71,69,0, 73,78,84,70,0, 66,76,73,67,0, 
75,76,78,0, 77,0, 88,84,0, 84,76,69,0, 79,82,68,0, 42,0, 
78,68,0, 81,0, 79,68,0, 69,0, 79,82,0, 39,0, 42,0, 79,68,0, 
69,0, 79,82,0, 92,39,0, 47,0, 69,73,76,0, 69,70,73,78,69,68,0, 
79,79,82,0, 73,71,72,0, 80,0, 79,84,0, 81,82,84,0, 10,0, 
92,39,0, 69,73,76,0, 69,70,73,78,69,68,0, 79,79,82,0, 73,71,72,0, 
80,0, 81,82,84,0, 92,39,0, 47,0, 69,73,76,0, 
69,70,73,78,69,68,0, 79,79,82,0, 73,71,72,0, 80,0, 81,82,84,0, 
10,0, 42,0, 78,68,0, 81,0, 79,68,0, 69,0, 82,0, 79,82,0, 
78,68,0, 81,0, 69,0, 82,0, 79,82,0, 78,68,0, 81,0, 69,0, 
82,0, 79,82,0, 81,0, 69,0, 82,0, 79,82,0, 81,0, 69,0, 82,0, 
61,0, 81,0, 69,0, 78,68,0, 81,0, 79,68,0, 69,0, 82,0, 
79,82,0, 10,0, 61,0, 61,0, 83,69,71,0, 79,67,75,0, 84,69,0, 
83,69,71,0, 71,0, 72,79,0, 68,0, 85,0, 82,78,0, 78,0, 
73,76,76,0, 69,88,0, 75,0, 84,0, 70,73,82,83,84,0, 
67,76,73,66,0, 68,85,76,69,0, 70,73,82,83,84,0, 77,69,0, 
80,65,71,69,0, 82,71,0, 71,69,0, 73,78,84,70,0, 66,76,73,67,0, 
84,0, 75,76,78,0, 77,0, 88,84,0, 84,76,69,0, 79,82,68,0, 
92,39,0, 73,0, 72,76,0, 72,82,0, 76,76,0, 76,0, 80,0, 85,66,0, 
73,0, 66,67,0, 73,0, 73,0, 86,0, 73,0, 79,80,0, 84,0, 
72,76,0, 80,0, 83,72,0, 69,76,0, 84,0, 77,0, 69,0, 76,0, 
77,0, 72,76,0, 72,71,0, 72,76,0, 10,0, 92,39,0, 72,76,41,0, 
73,76,0, 70,73,78,69,68,0, 79,79,82,0, 71,72,0, 80,0, 79,84,0, 
81,82,84,0, 10,0, 
};
#define AG_TCV(x) (((int)(x) >= -1 && (int)(x) <= 255) ? ag_tcv[(x) + 1] : 0)

static const unsigned short ag_tcv[] = {
   38, 38,620,620,620,620,620,620,620,620,  1,392,620,620,620,620,620,620,
  620,620,620,620,620,620,620,620,620,620,620,620,620,620,620,  1,490,621,
  622,623,487,477,624,422,420,485,483,445,484,398,486,625,626,627,627,628,
  628,628,628,629,629,620,394,630,620,631,632,633,634,635,634,636,634,637,
  638,639,638,638,638,638,638,640,638,638,638,641,638,642,638,638,638,643,
  638,638,620,644,620,475,645,633,634,635,634,636,634,637,638,639,638,638,
  638,638,638,640,638,638,638,641,638,642,638,638,638,643,638,638,632,473,
  632,492,646,646,646,646,646,646,646,646,646,646,646,646,646,646,646,646,
  646,646,646,646,646,646,646,646,646,646,646,646,646,646,646,646,646,646,
  646,646,646,646,646,646,646,646,646,646,646,646,646,646,646,646,646,646,
  646,646,646,646,646,646,646,646,646,646,646,646,646,646,646,646,646,646,
  646,646,646,646,646,646,646,646,646,646,646,646,646,646,646,646,646,646,
  646,646,646,646,646,646,646,646,646,646,646,646,646,646,646,646,646,646,
  646,646,646,646,646,646,646,646,646,646,646,646,646,646,646,646,646,646,
  646,646,646,646,646
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
645,643,642,641,640,639,638,637,636,635,634,632,623,406,402,401,398,396,394,
  393,392,132,131,1,0,51,52,
1,0,
645,643,642,641,640,639,638,637,636,635,634,632,623,406,398,396,394,393,392,
  132,131,63,1,0,11,34,35,36,37,51,52,53,64,65,130,397,410,
419,418,417,416,415,414,413,412,411,408,401,1,0,51,52,
645,643,642,641,640,639,638,637,636,635,634,632,629,628,627,626,625,623,428,
  398,396,394,393,392,158,132,1,0,135,
419,418,417,416,415,414,413,412,411,408,401,0,59,66,67,70,72,74,75,76,77,78,
  79,80,
645,643,642,641,640,639,638,637,636,635,634,632,623,619,618,617,616,615,614,
  613,612,611,610,609,608,607,606,605,604,603,602,601,600,599,598,597,596,
  595,594,593,592,591,590,589,588,587,586,585,584,583,582,581,580,579,578,
  577,576,575,574,573,572,571,570,569,568,567,565,564,563,562,561,560,559,
  558,557,556,555,553,552,551,550,549,548,547,546,545,544,543,542,541,540,
  539,538,537,536,535,534,533,532,531,530,529,528,527,526,525,524,523,522,
  521,520,519,518,517,516,515,514,513,512,511,510,509,508,456,455,454,453,
  452,451,449,448,447,446,444,443,442,441,440,439,438,437,436,435,434,433,
  432,431,430,429,428,427,426,425,424,416,415,412,409,407,400,399,398,396,
  394,393,392,132,131,1,0,51,52,
392,0,45,
402,401,398,396,394,393,392,0,39,40,41,45,46,47,49,54,55,
645,643,642,641,640,639,638,637,636,635,634,632,623,619,618,617,616,615,614,
  613,612,611,610,609,608,607,606,605,604,603,602,601,600,599,598,597,596,
  595,594,593,592,591,590,589,588,587,586,585,584,583,582,581,580,579,578,
  577,576,575,574,573,572,571,570,569,568,567,565,564,563,562,561,560,559,
  558,557,556,555,553,552,551,550,549,548,547,546,545,544,543,542,541,540,
  539,538,537,536,535,534,533,532,531,530,529,528,527,526,525,524,523,522,
  521,520,519,518,517,516,515,514,513,512,511,510,509,508,456,455,454,453,
  452,451,449,448,447,446,444,443,442,441,440,439,438,437,436,435,434,433,
  432,431,430,429,428,427,426,425,424,416,415,412,409,407,400,399,398,396,
  394,393,392,132,131,1,0,51,52,
645,643,642,641,640,639,638,637,636,635,634,632,623,619,618,617,616,615,614,
  613,612,611,610,609,608,607,606,605,604,603,602,601,600,599,598,597,596,
  595,594,593,592,591,590,589,588,587,586,585,584,583,582,581,580,579,578,
  577,576,575,574,573,572,571,570,569,568,567,565,564,563,562,561,560,559,
  558,557,556,555,553,552,551,550,549,548,547,546,545,544,543,542,541,540,
  539,538,537,536,535,534,533,532,531,530,529,528,527,526,525,524,523,522,
  521,520,519,518,517,516,515,514,513,512,511,510,509,508,456,455,454,453,
  452,451,449,448,447,446,444,443,442,441,440,439,438,437,436,435,434,433,
  432,431,430,429,428,427,426,425,424,416,415,412,409,407,402,401,398,396,
  394,393,392,132,131,1,0,21,30,31,32,33,54,55,62,72,76,77,86,91,130,249,
  250,251,252,253,254,255,256,257,259,260,261,262,263,264,265,266,267,268,
  269,270,271,272,273,274,275,276,277,278,279,280,281,282,283,284,285,286,
  287,288,289,290,291,292,293,294,295,296,297,298,299,300,301,302,303,304,
  305,306,307,308,309,310,311,312,313,314,315,316,317,318,319,320,321,322,
  323,324,325,326,327,328,330,331,332,333,334,335,336,337,338,339,340,341,
  342,343,344,345,346,347,348,349,350,351,352,353,354,355,356,357,358,359,
  360,361,362,363,364,365,366,367,368,369,370,371,372,373,374,375,376,377,
  378,379,380,410,
645,643,642,641,640,639,638,637,636,635,634,632,623,406,398,396,394,393,392,
  132,131,63,38,1,0,11,35,36,51,52,53,64,65,130,397,410,
645,643,642,641,640,639,638,637,636,635,634,632,623,132,131,1,0,51,52,
645,643,642,641,640,639,638,637,636,635,634,632,623,132,131,1,0,51,52,
621,1,0,51,52,
396,394,393,392,1,0,51,52,
396,394,393,392,1,0,51,52,
645,643,642,641,640,639,638,637,636,635,634,632,623,132,131,1,0,51,52,
645,643,642,641,640,639,638,637,636,635,634,633,632,629,628,627,626,625,624,
  623,503,501,500,499,498,497,496,495,494,493,492,491,490,487,484,483,422,
  398,226,132,131,1,0,51,52,
645,643,642,641,640,639,638,637,636,635,634,633,632,629,628,627,626,625,624,
  623,503,501,500,499,498,497,496,495,494,493,492,491,490,487,484,483,422,
  398,226,132,131,1,0,51,52,
645,643,642,641,640,639,638,637,636,635,634,632,623,132,131,1,0,51,52,
409,407,1,0,51,52,
645,644,643,642,641,640,639,638,637,636,635,634,630,629,628,627,626,625,621,
  486,398,1,0,51,52,
645,643,642,641,640,639,638,637,636,635,634,632,623,132,131,0,21,81,130,410,
645,643,642,641,640,639,638,637,636,635,634,632,623,132,131,0,21,130,410,
621,0,10,12,403,
396,394,393,392,1,0,51,52,
396,394,393,392,1,0,51,52,
645,643,642,641,640,639,638,637,636,635,634,632,623,132,131,0,21,130,410,
645,643,642,641,640,639,638,637,636,635,634,633,632,629,628,627,626,625,624,
  623,503,501,500,499,498,497,496,495,494,493,492,491,490,487,484,483,422,
  398,226,132,131,0,2,6,7,8,9,15,17,21,23,24,25,26,27,28,29,57,73,83,86,
  130,160,191,193,198,199,200,201,202,203,204,205,206,207,208,209,210,211,
  212,213,214,410,457,504,
645,643,642,641,640,639,638,637,636,635,634,633,632,629,628,627,626,625,624,
  623,503,501,500,499,498,497,496,495,494,493,492,491,490,487,484,483,422,
  398,226,132,131,0,2,6,7,8,9,15,17,21,23,24,25,26,27,28,29,57,73,83,86,
  130,160,191,193,198,199,200,201,202,203,204,205,206,207,208,209,210,211,
  212,213,214,410,457,504,
645,643,642,641,640,639,638,637,636,635,634,632,623,132,131,0,21,130,410,
409,407,0,68,69,
630,621,0,10,12,13,14,403,404,
646,645,644,643,642,641,640,639,638,637,636,635,634,633,632,631,630,629,628,
  627,626,625,624,623,622,621,620,492,490,487,486,485,484,483,477,475,473,
  445,422,420,406,402,401,398,396,395,394,393,392,132,131,38,1,0,51,52,
646,645,644,643,642,641,640,639,638,637,636,635,634,633,632,631,630,629,628,
  627,626,625,624,623,622,621,620,492,490,487,486,485,484,483,477,475,473,
  456,455,454,453,452,451,449,448,447,446,445,444,443,442,441,440,439,438,
  437,436,435,434,433,432,431,430,429,428,427,426,425,424,423,422,420,409,
  407,402,401,400,399,398,396,395,394,393,392,1,0,51,52,
402,401,0,59,60,61,
646,645,644,643,642,641,640,639,638,637,636,635,634,633,632,631,630,629,628,
  627,626,625,624,623,622,621,620,492,490,487,486,485,484,483,477,475,473,
  445,422,420,398,396,395,394,392,1,0,51,52,
646,645,644,643,642,641,640,639,638,637,636,635,634,633,632,631,630,629,628,
  627,626,625,624,623,622,621,620,492,490,487,486,485,484,483,477,475,473,
  445,423,422,420,398,396,395,394,393,392,1,0,51,52,
646,645,644,643,642,641,640,639,638,637,636,635,634,633,632,631,630,629,628,
  627,626,625,624,623,622,621,620,492,490,487,486,485,484,483,477,475,473,
  445,422,420,398,394,392,1,0,51,52,
646,645,644,643,642,641,640,639,638,637,636,635,634,633,632,631,630,629,628,
  627,626,625,624,623,622,621,620,492,490,487,486,485,484,483,477,475,473,
  445,422,420,398,396,395,394,392,1,0,40,45,46,47,48,49,54,83,85,119,137,
  138,140,142,143,144,145,146,147,151,154,155,180,182,184,190,191,192,194,
  195,198,200,215,219,220,224,381,382,383,384,385,386,387,388,389,390,391,
646,645,644,643,642,641,640,639,638,637,636,635,634,633,632,631,630,629,628,
  627,626,625,624,623,622,621,620,492,490,487,486,485,484,483,477,475,473,
  445,422,420,398,394,392,1,0,43,44,46,54,83,85,119,137,138,140,142,143,
  144,145,146,147,151,154,155,180,182,184,190,191,192,194,195,198,200,215,
  219,220,224,381,382,383,384,385,386,387,388,389,390,391,
646,645,644,643,642,641,640,639,638,637,636,635,634,633,632,631,630,629,628,
  627,626,625,624,623,622,621,620,492,490,487,486,485,484,483,477,475,473,
  445,422,420,398,394,392,1,0,43,44,46,54,83,85,119,137,138,140,142,143,
  144,145,146,147,151,154,155,180,182,184,190,191,192,194,195,198,200,215,
  219,220,224,381,382,383,384,385,386,387,388,389,390,391,
645,643,642,641,640,639,638,637,636,635,634,632,623,619,618,617,616,615,614,
  613,612,611,610,609,608,607,606,605,604,603,602,601,600,599,598,597,596,
  595,594,593,592,591,590,589,588,587,586,585,584,583,582,581,580,579,578,
  577,576,575,574,573,572,571,570,569,568,567,565,564,563,562,561,560,559,
  558,557,556,555,553,552,551,550,549,548,547,546,545,544,543,542,541,540,
  539,538,537,536,535,534,533,532,531,530,529,528,527,526,525,524,523,522,
  521,520,519,518,517,516,515,514,513,512,511,510,509,508,456,455,454,453,
  452,451,449,448,447,446,444,443,442,441,440,439,438,437,436,435,434,433,
  432,431,430,429,428,427,426,425,424,416,415,412,409,407,400,399,398,396,
  394,393,392,132,131,0,21,30,31,32,33,39,40,41,45,46,47,49,54,55,62,72,
  76,77,86,91,130,249,250,251,252,253,254,255,256,257,259,260,261,262,263,
  264,265,266,267,268,269,270,271,272,273,274,275,276,277,278,279,280,281,
  282,283,284,285,286,287,288,289,290,291,292,293,294,295,296,297,298,299,
  300,301,302,303,304,305,306,307,308,309,310,311,312,313,314,315,316,317,
  318,319,320,321,322,323,324,325,326,327,328,330,331,332,333,334,335,336,
  337,338,339,340,341,342,343,344,345,346,347,348,349,350,351,352,353,354,
  355,356,357,358,359,360,361,362,363,364,365,366,367,368,369,370,371,372,
  373,374,375,376,377,378,379,380,410,
459,456,396,394,393,392,1,0,51,52,
507,396,394,393,392,1,0,51,52,
507,396,394,393,392,1,0,51,52,
507,396,394,393,392,1,0,51,52,
507,396,394,393,392,1,0,51,52,
507,396,394,393,392,1,0,51,52,
507,396,394,393,392,1,0,51,52,
507,396,394,393,392,1,0,51,52,
507,396,394,393,392,1,0,51,52,
507,396,394,393,392,1,0,51,52,
507,396,394,393,392,1,0,51,52,
507,396,394,393,392,1,0,51,52,
507,396,394,393,392,1,0,51,52,
507,396,394,393,392,1,0,51,52,
507,396,394,393,392,1,0,51,52,
506,236,235,234,233,232,231,230,229,1,0,51,52,
245,243,241,239,233,231,229,1,0,51,52,
645,643,642,641,640,639,638,637,636,635,634,633,632,629,628,627,626,625,624,
  623,503,501,500,499,498,497,496,495,494,493,492,491,490,487,484,483,422,
  398,226,132,131,1,0,51,52,
645,643,642,641,640,639,638,637,636,635,634,633,632,629,628,627,626,625,624,
  623,503,501,500,499,498,497,496,495,494,493,492,491,490,487,484,483,422,
  398,226,132,131,1,0,51,52,
645,643,642,641,640,639,638,637,636,635,634,633,632,629,628,627,626,625,624,
  623,503,501,500,499,498,497,496,495,494,493,492,491,490,487,484,483,422,
  398,226,132,131,1,0,51,52,
645,643,642,641,640,639,638,637,636,635,634,633,632,629,628,627,626,625,624,
  623,503,501,500,499,498,497,496,495,494,493,492,491,490,487,484,483,422,
  398,226,132,131,1,0,51,52,
645,643,642,641,640,639,638,637,636,635,634,633,632,629,628,627,626,625,624,
  623,503,501,500,499,498,497,496,495,494,493,492,491,490,487,484,483,422,
  398,226,132,131,1,0,51,52,
645,643,642,641,640,639,638,637,636,635,634,633,632,629,628,627,626,625,624,
  623,503,501,500,499,498,497,496,495,494,493,492,491,490,487,484,483,422,
  398,226,132,131,1,0,51,52,
645,643,642,641,640,639,638,637,636,635,634,633,632,629,628,627,626,625,624,
  623,503,501,500,499,498,497,496,495,494,493,492,491,490,487,484,483,422,
  398,226,132,131,1,0,51,52,
645,643,642,641,640,639,638,637,636,635,634,633,632,629,628,627,626,625,624,
  623,503,501,500,499,498,497,496,495,494,493,492,491,490,487,484,483,422,
  398,226,132,131,1,0,51,52,
645,643,642,641,640,639,638,637,636,635,634,633,632,629,628,627,626,625,624,
  623,503,501,500,499,498,497,496,495,494,493,492,491,490,487,484,483,422,
  398,226,132,131,1,0,51,52,
645,643,642,641,640,639,638,637,636,635,634,633,632,629,628,627,626,625,624,
  623,503,501,500,499,498,497,496,495,494,493,492,491,490,487,484,483,422,
  398,226,132,131,1,0,51,52,
645,643,642,641,640,639,638,637,636,635,634,633,632,629,628,627,626,625,624,
  623,503,501,500,499,498,497,496,495,494,493,492,491,490,487,484,483,422,
  398,226,132,131,1,0,51,52,
645,643,642,641,640,639,638,637,636,635,634,633,632,629,628,627,626,625,624,
  623,503,501,500,499,498,497,496,495,494,493,492,491,490,487,484,483,422,
  398,226,132,131,1,0,51,52,
645,643,642,641,640,639,638,637,636,635,634,633,632,629,628,627,626,625,624,
  623,503,501,500,499,498,497,496,495,494,493,492,491,490,487,484,483,422,
  398,226,132,131,1,0,51,52,
645,643,642,641,640,639,638,637,636,635,634,633,632,629,628,627,626,625,624,
  623,503,501,500,499,498,497,496,495,494,493,492,491,490,487,484,483,422,
  398,226,132,131,1,0,51,52,
645,643,642,641,640,639,638,637,636,635,634,633,632,629,628,627,626,625,624,
  623,503,501,500,499,498,497,496,495,494,493,492,491,490,487,484,483,422,
  398,226,132,131,1,0,51,52,
507,396,394,393,392,1,0,51,52,
507,396,394,393,392,1,0,51,52,
507,396,394,393,392,1,0,51,52,
507,396,394,393,392,1,0,51,52,
507,396,394,393,392,1,0,51,52,
507,396,394,393,392,1,0,51,52,
507,396,394,393,392,1,0,51,52,
507,396,394,393,392,1,0,51,52,
507,396,394,393,392,1,0,51,52,
507,396,394,393,392,1,0,51,52,
507,396,394,393,392,1,0,51,52,
507,396,394,393,392,1,0,51,52,
507,396,394,393,392,1,0,51,52,
507,396,394,393,392,1,0,51,52,
507,396,394,393,392,1,0,51,52,
507,396,394,393,392,1,0,51,52,
507,396,394,393,392,1,0,51,52,
507,396,394,393,392,1,0,51,52,
507,396,394,393,392,1,0,51,52,
507,396,394,393,392,1,0,51,52,
507,396,394,393,392,1,0,51,52,
507,396,394,393,392,1,0,51,52,
507,396,394,393,392,1,0,51,52,
507,396,394,393,392,1,0,51,52,
507,396,394,393,392,1,0,51,52,
507,396,394,393,392,1,0,51,52,
507,396,394,393,392,1,0,51,52,
507,396,394,393,392,1,0,51,52,
628,627,626,625,1,0,51,52,
506,236,235,234,233,232,231,230,229,1,0,51,52,
245,243,241,239,233,231,229,1,0,51,52,
645,643,642,641,640,639,638,637,636,635,634,633,632,629,628,627,626,625,624,
  623,503,501,500,499,498,497,496,495,494,493,492,491,490,487,484,483,422,
  398,226,132,131,1,0,51,52,
645,643,642,641,640,639,638,637,636,635,634,633,632,629,628,627,626,625,624,
  623,503,501,500,499,498,497,496,495,494,493,492,491,490,487,484,483,422,
  398,226,132,131,1,0,51,52,
645,643,642,641,640,639,638,637,636,635,634,633,632,629,628,627,626,625,624,
  623,503,501,500,499,498,497,496,495,494,493,492,491,490,487,484,483,422,
  398,226,132,131,1,0,51,52,
645,643,642,641,640,639,638,637,636,635,634,633,632,629,628,627,626,625,624,
  623,503,501,500,499,498,497,496,495,494,493,492,491,490,487,484,483,422,
  398,226,132,131,1,0,51,52,
645,643,642,641,640,639,638,637,636,635,634,633,632,629,628,627,626,625,624,
  623,503,501,500,499,498,497,496,495,494,493,492,491,490,487,484,483,422,
  398,226,132,131,1,0,51,52,
645,643,642,641,640,639,638,637,636,635,634,633,632,629,628,627,626,625,624,
  623,503,501,500,499,498,497,496,495,494,493,492,491,490,487,484,483,422,
  398,226,132,131,1,0,51,52,
645,643,642,641,640,639,638,637,636,635,634,633,632,629,628,627,626,625,624,
  623,503,501,500,499,498,497,496,495,494,493,492,491,490,487,484,483,422,
  398,226,132,131,1,0,51,52,
645,643,642,641,640,639,638,637,636,635,634,633,632,629,628,627,626,625,624,
  623,503,501,500,499,498,497,496,495,494,493,492,491,490,487,484,483,422,
  398,226,132,131,1,0,51,52,
645,643,642,641,640,639,638,637,636,635,634,633,632,629,628,627,626,625,624,
  623,503,501,500,499,498,497,496,495,494,493,492,491,490,487,484,483,422,
  398,226,132,131,1,0,51,52,
645,643,642,641,640,639,638,637,636,635,634,633,632,629,628,627,626,625,624,
  623,503,501,500,499,498,497,496,495,494,493,492,491,490,487,484,483,422,
  398,226,132,131,1,0,51,52,
645,643,642,641,640,639,638,637,636,635,634,633,632,629,628,627,626,625,624,
  623,503,501,500,499,498,497,496,495,494,493,492,491,490,487,484,483,422,
  398,226,132,131,1,0,51,52,
645,643,642,641,640,639,638,637,636,635,634,633,632,629,628,627,626,625,624,
  623,503,501,500,499,498,497,496,495,494,493,492,491,490,487,484,483,422,
  398,226,132,131,1,0,51,52,
645,643,642,641,640,639,638,637,636,635,634,633,632,629,628,627,626,625,624,
  623,503,501,500,499,498,497,496,495,494,493,492,491,490,487,484,483,422,
  398,226,132,131,1,0,51,52,
645,643,642,641,640,639,638,637,636,635,634,633,632,629,628,627,626,625,624,
  623,503,501,500,499,498,497,496,495,494,493,492,491,490,487,484,483,422,
  398,226,132,131,1,0,51,52,
645,643,642,641,640,639,638,637,636,635,634,633,632,629,628,627,626,625,624,
  623,503,501,500,499,498,497,496,495,494,493,492,491,490,487,484,483,422,
  398,226,132,131,1,0,51,52,
645,643,642,641,640,639,638,637,636,635,634,633,632,629,628,627,626,625,624,
  623,503,501,500,499,498,497,496,495,494,493,492,491,490,487,484,483,422,
  398,226,132,131,1,0,51,52,
645,643,642,641,640,639,638,637,636,635,634,633,632,629,628,627,626,625,624,
  623,503,501,500,499,498,497,496,495,494,493,492,491,490,487,484,483,422,
  398,226,132,131,1,0,51,52,
645,643,642,641,640,639,638,637,636,635,634,633,632,629,628,627,626,625,624,
  623,503,501,500,499,498,497,496,495,494,493,492,491,490,487,484,483,422,
  398,226,132,131,1,0,51,52,
645,643,642,641,640,639,638,637,636,635,634,633,632,629,628,627,626,625,624,
  623,503,501,500,499,498,497,496,495,494,493,492,491,490,487,484,483,422,
  398,226,132,131,1,0,51,52,
645,643,642,641,640,639,638,637,636,635,634,633,632,629,628,627,626,625,624,
  623,503,501,500,499,498,497,496,495,494,493,492,491,490,487,484,483,422,
  398,226,132,131,1,0,51,52,
645,643,642,641,640,639,638,637,636,635,634,633,632,629,628,627,626,625,624,
  623,503,501,500,499,498,497,496,495,494,493,492,491,490,487,484,483,422,
  398,226,132,131,1,0,51,52,
645,643,642,641,640,639,638,637,636,635,634,633,632,629,628,627,626,625,624,
  623,503,501,500,499,498,497,496,495,494,493,492,491,490,487,484,483,422,
  398,226,132,131,1,0,51,52,
645,643,642,641,640,639,638,637,636,635,634,633,632,629,628,627,626,625,624,
  623,503,501,500,499,498,497,496,495,494,493,492,491,490,487,484,483,422,
  398,226,132,131,1,0,51,52,
645,643,642,641,640,639,638,637,636,635,634,633,632,629,628,627,626,625,624,
  623,503,501,500,499,498,497,496,495,494,493,492,491,490,487,484,483,422,
  398,226,132,131,1,0,51,52,
645,643,642,641,640,639,638,637,636,635,634,633,632,629,628,627,626,625,624,
  623,503,501,500,499,498,497,496,495,494,493,492,491,490,487,484,483,422,
  398,226,132,131,1,0,51,52,
645,643,642,641,640,639,638,637,636,635,634,633,632,629,628,627,626,625,624,
  623,503,501,500,499,498,497,496,495,494,493,492,491,490,487,484,483,422,
  398,226,132,131,1,0,51,52,
645,643,642,641,640,639,638,637,636,635,634,633,632,629,628,627,626,625,624,
  623,503,501,500,499,498,497,496,495,494,493,492,491,490,487,484,483,422,
  398,226,132,131,1,0,51,52,
645,643,642,641,640,639,638,637,636,635,634,633,632,629,628,627,626,625,624,
  623,503,501,500,499,498,497,496,495,494,493,492,491,490,487,484,483,422,
  398,226,132,131,1,0,51,52,
645,643,642,641,640,639,638,637,636,635,634,633,632,629,628,627,626,625,624,
  623,503,501,500,499,498,497,496,495,494,493,492,491,490,487,484,483,422,
  398,226,132,131,1,0,51,52,
645,643,642,641,640,639,638,637,636,635,634,633,632,629,628,627,626,625,624,
  623,503,501,500,499,498,497,496,495,494,493,492,491,490,487,484,483,422,
  398,226,132,131,1,0,51,52,
645,643,642,641,640,639,638,637,636,635,634,633,632,629,628,627,626,625,624,
  623,503,501,500,499,498,497,496,495,494,493,492,491,490,487,484,483,422,
  398,226,132,131,1,0,51,52,
645,643,642,641,640,639,638,637,636,635,634,633,632,629,628,627,626,625,624,
  623,503,501,500,499,498,497,496,495,494,493,492,491,490,487,484,483,422,
  398,226,132,131,1,0,51,52,
506,505,243,241,239,233,231,229,1,0,51,52,
506,505,243,241,239,233,231,229,1,0,51,52,
241,239,231,229,1,0,51,52,
241,239,231,229,1,0,51,52,
245,243,241,239,233,231,229,1,0,51,52,
245,243,241,239,233,231,229,1,0,51,52,
245,243,241,239,233,231,229,1,0,51,52,
506,236,235,234,233,232,231,230,229,1,0,51,52,
506,236,235,234,233,232,231,230,229,1,0,51,52,
506,236,235,234,233,232,231,230,229,1,0,51,52,
506,236,235,234,233,232,231,230,229,1,0,51,52,
506,236,235,234,233,232,231,230,229,1,0,51,52,
506,236,235,234,233,232,231,230,229,1,0,51,52,
506,236,235,234,233,232,231,230,229,1,0,51,52,
506,236,235,234,233,232,231,230,229,1,0,51,52,
506,236,235,234,233,232,231,230,229,1,0,51,52,
506,236,235,234,233,232,231,230,229,1,0,51,52,
506,236,235,234,233,232,231,230,229,1,0,51,52,
628,627,626,625,1,0,51,52,
445,1,0,51,52,
445,1,0,51,52,
645,643,642,641,640,639,638,637,636,635,634,633,632,629,628,627,626,625,624,
  623,503,501,500,499,498,497,496,495,494,493,492,491,490,487,484,483,422,
  398,226,132,131,1,0,51,52,
645,643,642,641,640,639,638,637,636,635,634,633,632,629,628,627,626,625,624,
  623,503,501,500,499,498,497,496,495,494,493,492,491,490,487,484,483,422,
  398,226,132,131,1,0,51,52,
645,643,642,641,640,639,638,637,636,635,634,633,632,629,628,627,626,625,624,
  623,503,501,500,499,498,497,496,495,494,493,492,491,490,487,484,483,422,
  398,226,132,131,1,0,51,52,
645,643,642,641,640,639,638,637,636,635,634,633,632,629,628,627,626,625,624,
  623,503,501,500,499,498,497,496,495,494,493,492,491,490,487,484,483,422,
  398,226,132,131,1,0,51,52,
645,643,642,641,640,639,638,637,636,635,634,633,632,629,628,627,626,625,624,
  623,503,501,500,499,498,497,496,495,494,493,492,491,490,487,484,483,422,
  398,226,132,131,1,0,51,52,
645,643,642,641,640,639,638,637,636,635,634,633,632,629,628,627,626,625,624,
  623,503,501,500,499,498,497,496,495,494,493,492,491,490,487,484,483,422,
  398,226,132,131,1,0,51,52,
645,643,642,641,640,639,638,637,636,635,634,633,632,629,628,627,626,625,624,
  623,503,501,500,499,498,497,496,495,494,493,492,491,490,487,484,483,422,
  398,226,132,131,1,0,51,52,
645,643,642,641,640,639,638,637,636,635,634,633,632,629,628,627,626,625,624,
  623,503,501,500,499,498,497,496,495,494,493,492,491,490,487,484,483,422,
  398,226,132,131,1,0,51,52,
645,643,642,641,640,639,638,637,636,635,634,633,632,629,628,627,626,625,624,
  623,503,501,500,499,498,497,496,495,494,493,492,491,490,487,484,483,422,
  398,226,132,131,1,0,51,52,
645,643,642,641,640,639,638,637,636,635,634,633,632,629,628,627,626,625,624,
  623,503,501,500,499,498,497,496,495,494,493,492,491,490,487,484,483,422,
  398,226,132,131,1,0,51,52,
645,643,642,641,640,639,638,637,636,635,634,633,632,629,628,627,626,625,624,
  623,503,501,500,499,498,497,496,495,494,493,492,491,490,487,484,483,422,
  398,226,132,131,1,0,51,52,
645,643,642,641,640,639,638,637,636,635,634,633,632,629,628,627,626,625,624,
  623,503,501,500,499,498,497,496,495,494,493,492,491,490,487,484,483,422,
  398,226,132,131,1,0,51,52,
645,643,642,641,640,639,638,637,636,635,634,633,632,629,628,627,626,625,624,
  623,503,501,500,499,498,497,496,495,494,493,492,491,490,487,484,483,422,
  398,226,132,131,1,0,51,52,
645,643,642,641,640,639,638,637,636,635,634,633,632,629,628,627,626,625,624,
  623,503,501,500,499,498,497,496,495,494,493,492,491,490,487,484,483,422,
  398,226,132,131,1,0,51,52,
645,643,642,641,640,639,638,637,636,635,634,633,632,629,628,627,626,625,624,
  623,503,501,500,499,498,497,496,495,494,493,492,491,490,487,484,483,422,
  398,226,132,131,1,0,51,52,
645,643,642,641,640,639,638,637,636,635,634,633,632,629,628,627,626,625,624,
  623,503,501,500,499,498,497,496,495,494,493,492,491,490,487,484,483,422,
  398,226,132,131,1,0,51,52,
645,643,642,641,640,639,638,637,636,635,634,633,632,629,628,627,626,625,624,
  623,503,501,500,499,498,497,496,495,494,493,492,491,490,487,484,483,422,
  398,226,132,131,1,0,51,52,
645,643,642,641,640,639,638,637,636,635,634,633,632,629,628,627,626,625,624,
  623,503,501,500,499,498,497,496,495,494,493,492,491,490,487,484,483,422,
  398,226,132,131,1,0,51,52,
645,643,642,641,640,639,638,637,636,635,634,633,632,629,628,627,626,625,624,
  623,503,501,500,499,498,497,496,495,494,493,492,491,490,487,484,483,422,
  398,226,132,131,1,0,51,52,
645,643,642,641,640,639,638,637,636,635,634,633,632,629,628,627,626,625,624,
  623,503,501,500,499,498,497,496,495,494,493,492,491,490,487,484,483,422,
  398,226,132,131,1,0,51,52,
645,643,642,641,640,639,638,637,636,635,634,633,632,629,628,627,626,625,624,
  623,503,501,500,499,498,497,496,495,494,493,492,491,490,487,484,483,422,
  398,226,132,131,1,0,51,52,
645,643,642,641,640,639,638,637,636,635,634,633,632,629,628,627,626,625,624,
  623,503,501,500,499,498,497,496,495,494,493,492,491,490,487,484,483,422,
  398,226,132,131,1,0,51,52,
645,643,642,641,640,639,638,637,636,635,634,633,632,629,628,627,626,625,624,
  623,503,501,500,499,498,497,496,495,494,493,492,491,490,487,484,483,422,
  398,226,132,131,1,0,51,52,
645,643,642,641,640,639,638,637,636,635,634,633,632,629,628,627,626,625,624,
  623,503,501,500,499,498,497,496,495,494,493,492,491,490,487,484,483,422,
  398,226,132,131,1,0,51,52,
645,643,642,641,640,639,638,637,636,635,634,633,632,629,628,627,626,625,624,
  623,503,501,500,499,498,497,496,495,494,493,492,491,490,487,484,483,422,
  398,226,132,131,1,0,51,52,
645,643,642,641,640,639,638,637,636,635,634,633,632,629,628,627,626,625,624,
  623,503,501,500,499,498,497,496,495,494,493,492,491,490,487,484,483,422,
  398,226,132,131,1,0,51,52,
645,643,642,641,640,639,638,637,636,635,634,633,632,629,628,627,626,625,624,
  623,503,501,500,499,498,497,496,495,494,493,492,491,490,487,484,483,422,
  398,226,132,131,1,0,51,52,
645,643,642,641,640,639,638,637,636,635,634,633,632,629,628,627,626,625,624,
  623,503,501,500,499,498,497,496,495,494,493,492,491,490,487,484,483,422,
  398,226,132,131,1,0,51,52,
645,643,642,641,640,639,638,637,636,635,634,633,632,629,628,627,626,625,624,
  623,503,501,500,499,498,497,496,495,494,493,492,491,490,487,484,483,422,
  398,226,132,131,1,0,51,52,
645,643,642,641,640,639,638,637,636,635,634,633,632,629,628,627,626,625,624,
  623,503,501,500,499,498,497,496,495,494,493,492,491,490,487,484,483,422,
  398,226,132,131,1,0,51,52,
645,643,642,641,640,639,638,637,636,635,634,633,632,629,628,627,626,625,624,
  623,503,501,500,499,498,497,496,495,494,493,492,491,490,487,484,483,422,
  398,226,132,131,1,0,51,52,
645,643,642,641,640,639,638,637,636,635,634,633,632,629,628,627,626,625,624,
  623,503,501,500,499,498,497,496,495,494,493,492,491,490,487,484,483,422,
  398,226,132,131,1,0,51,52,
645,643,642,641,640,639,638,637,636,635,634,633,632,629,628,627,626,625,624,
  623,503,501,500,499,498,497,496,495,494,493,492,491,490,487,484,483,422,
  398,226,132,131,1,0,51,52,
645,643,642,641,640,639,638,637,636,635,634,633,632,629,628,627,626,625,624,
  623,503,501,500,499,498,497,496,495,494,493,492,491,490,487,484,483,422,
  398,226,132,131,1,0,51,52,
645,643,642,641,640,639,638,637,636,635,634,633,632,629,628,627,626,625,624,
  623,503,501,500,499,498,497,496,495,494,493,492,491,490,487,484,483,422,
  398,226,132,131,1,0,51,52,
645,643,642,641,640,639,638,637,636,635,634,633,632,629,628,627,626,625,624,
  623,503,501,500,499,498,497,496,495,494,493,492,491,490,487,484,483,422,
  398,226,132,131,1,0,51,52,
506,505,243,241,239,233,231,229,1,0,51,52,
506,505,243,241,239,233,231,229,1,0,51,52,
241,239,231,229,1,0,51,52,
241,239,231,229,1,0,51,52,
245,243,241,239,233,231,229,1,0,51,52,
245,243,241,239,233,231,229,1,0,51,52,
245,243,241,239,233,231,229,1,0,51,52,
506,236,235,234,233,232,231,230,229,1,0,51,52,
506,236,235,234,233,232,231,230,229,1,0,51,52,
506,236,235,234,233,232,231,230,229,1,0,51,52,
506,236,235,234,233,232,231,230,229,1,0,51,52,
506,236,235,234,233,232,231,230,229,1,0,51,52,
506,236,235,234,233,232,231,230,229,1,0,51,52,
506,236,235,234,233,232,231,230,229,1,0,51,52,
506,236,235,234,233,232,231,230,229,1,0,51,52,
506,236,235,234,233,232,231,230,229,1,0,51,52,
506,236,235,234,233,232,231,230,229,1,0,51,52,
506,236,235,234,233,232,231,230,229,1,0,51,52,
646,645,643,642,641,640,639,638,637,636,635,634,633,632,631,630,629,628,627,
  626,625,624,623,621,620,507,492,490,489,487,486,485,484,483,481,479,477,
  475,473,472,470,468,466,464,462,461,458,445,422,420,398,396,394,393,392,
  1,0,51,52,135,
507,396,394,393,392,1,0,51,52,
507,489,488,487,486,485,484,483,482,481,480,479,478,477,476,475,474,473,472,
  471,470,469,468,467,466,465,464,463,462,461,460,458,422,420,396,394,393,
  392,1,0,83,
396,394,393,392,1,0,51,
507,0,258,
645,643,642,641,640,639,638,637,636,635,634,633,632,629,628,627,626,625,624,
  623,503,501,500,499,498,497,496,495,494,493,492,491,490,487,484,483,422,
  398,226,132,131,0,2,6,7,8,9,15,17,21,23,24,25,26,27,28,29,57,73,83,86,
  130,160,191,193,198,199,200,201,202,203,204,205,206,207,208,209,210,211,
  212,213,214,410,457,504,
456,455,454,453,452,451,449,448,447,446,444,443,442,441,440,439,438,437,436,
  435,434,433,432,431,430,429,428,427,426,425,424,409,407,0,68,69,92,93,
  94,95,96,97,98,99,100,101,102,103,104,105,106,107,108,109,111,112,113,
  114,115,116,117,118,120,121,122,123,124,125,126,127,128,
396,394,393,392,1,0,51,52,
646,645,643,642,641,640,639,638,637,636,635,634,633,632,631,630,629,628,627,
  626,625,624,623,621,620,492,490,487,486,485,484,483,477,475,473,445,422,
  420,398,394,1,0,83,
646,645,643,642,641,640,639,638,637,636,635,634,633,632,631,630,629,628,627,
  626,625,624,623,621,620,492,490,487,486,485,484,483,477,475,473,445,422,
  420,398,394,1,0,51,52,
396,394,393,392,1,0,51,52,
645,644,643,642,641,640,639,638,637,636,635,634,633,632,631,630,629,628,627,
  626,625,624,623,622,621,620,492,490,487,486,485,484,483,477,475,473,445,
  422,420,398,394,392,1,0,139,
458,445,420,396,394,393,392,1,0,51,52,
396,394,393,392,1,0,51,52,
396,394,393,392,0,39,40,41,45,46,47,49,
396,394,393,392,0,39,40,41,45,46,47,49,
396,394,393,392,1,0,51,52,
629,628,627,626,625,0,
628,627,626,625,0,
626,625,0,
645,643,642,641,640,639,638,637,636,635,634,632,629,628,627,626,625,623,507,
  489,488,487,486,485,484,483,482,481,480,479,478,477,476,475,474,473,472,
  471,470,469,468,467,466,465,464,463,462,461,460,458,422,420,398,396,394,
  393,392,1,0,
645,643,639,637,636,635,634,629,628,627,626,625,623,507,489,488,487,486,485,
  484,483,482,481,480,479,478,477,476,475,474,473,472,471,470,469,468,467,
  466,465,464,463,462,461,460,458,420,398,396,394,393,392,217,216,1,0,
629,628,627,626,625,0,
645,644,643,642,641,640,639,638,637,636,635,634,633,632,631,630,629,628,627,
  626,625,623,622,621,620,492,490,487,486,485,484,483,477,475,473,445,422,
  420,398,394,392,228,1,0,16,
629,628,627,626,625,0,
639,637,636,635,634,629,628,627,626,625,0,
628,627,626,625,507,489,488,487,486,485,484,483,482,481,480,479,478,477,476,
  475,474,473,472,471,470,469,468,467,466,465,464,463,462,461,460,458,420,
  396,394,393,392,1,0,
626,625,507,489,488,487,486,485,484,483,482,481,480,479,478,477,476,475,474,
  473,472,471,470,469,468,467,466,465,464,463,462,461,460,458,420,396,394,
  393,392,1,0,
637,636,635,634,629,628,627,626,625,507,489,488,487,486,485,484,483,482,481,
  480,479,478,477,476,475,474,473,472,471,470,469,468,467,466,465,464,463,
  462,461,460,458,420,396,394,393,392,1,0,
645,639,637,636,635,634,629,628,627,626,625,623,507,489,488,487,486,485,484,
  483,482,481,480,479,477,476,475,474,473,472,471,470,469,468,467,466,465,
  464,463,462,461,458,420,398,396,394,393,392,217,216,1,0,218,
422,1,0,51,52,
422,1,0,51,52,
422,1,0,51,52,
422,1,0,51,52,
422,1,0,51,52,
422,1,0,51,52,
422,1,0,51,52,
422,1,0,51,52,
422,1,0,51,52,
422,1,0,51,52,
507,489,488,487,486,485,484,483,482,481,480,479,478,477,476,475,474,473,472,
  471,470,469,468,467,466,465,464,463,462,461,460,458,420,396,394,393,392,
  1,0,51,52,
507,489,488,487,486,485,484,483,482,481,480,479,478,477,476,475,474,473,472,
  471,470,469,468,467,466,465,464,463,462,461,460,458,420,396,394,393,392,
  1,0,51,52,
646,645,644,643,642,641,640,639,638,637,636,635,634,633,632,631,630,629,628,
  627,626,625,624,623,622,621,620,503,501,500,499,498,497,496,495,494,493,
  492,491,490,487,486,485,484,483,477,475,473,445,423,422,421,420,398,396,
  395,394,393,392,226,132,131,1,0,51,52,
422,1,0,51,52,
422,1,0,51,52,
422,1,0,51,52,
422,1,0,51,52,
422,1,0,51,52,
422,1,0,51,52,
422,1,0,51,52,
422,1,0,51,52,
422,1,0,51,52,
422,1,0,51,52,
645,643,642,641,640,639,638,637,636,635,634,633,632,629,628,627,626,625,624,
  623,503,501,500,499,498,497,496,495,494,493,487,484,483,422,398,226,132,
  131,1,0,51,52,
646,645,644,643,642,641,640,639,638,637,636,635,634,633,632,631,630,629,628,
  627,626,625,624,623,622,621,620,503,501,500,499,498,497,496,495,494,493,
  492,490,487,486,485,484,483,477,475,473,445,423,422,420,398,396,395,394,
  393,392,226,132,131,1,0,51,52,
645,643,642,641,640,639,638,637,636,635,634,633,632,629,628,627,626,625,624,
  623,503,501,500,499,498,497,496,495,494,493,487,484,483,422,398,226,132,
  131,1,0,51,52,
646,645,644,643,642,641,640,639,638,637,636,635,634,633,632,631,630,629,628,
  627,626,625,624,623,622,621,620,503,501,500,499,498,497,496,495,494,493,
  492,490,487,486,485,484,483,477,475,473,445,423,422,420,398,396,395,394,
  393,392,226,132,131,1,0,51,52,
645,643,642,641,640,639,638,637,636,635,634,633,632,629,628,627,626,625,624,
  623,503,501,500,499,498,497,496,495,494,493,492,491,490,487,484,483,422,
  398,226,132,131,1,0,51,52,
645,643,642,641,640,639,638,637,636,635,634,633,632,629,628,627,626,625,624,
  623,503,501,500,499,498,497,496,495,494,493,487,484,483,422,398,226,132,
  131,0,2,6,7,8,9,15,17,21,83,86,130,193,201,202,203,204,205,206,207,208,
  209,210,211,212,213,214,410,457,504,
645,643,642,641,640,639,638,637,636,635,634,633,632,629,628,627,626,625,624,
  623,503,501,500,499,498,497,496,495,494,493,487,484,483,422,398,226,132,
  131,0,2,6,7,8,9,15,17,21,83,86,130,193,201,202,203,204,205,206,207,208,
  209,210,211,212,213,214,410,457,504,
645,643,642,641,640,639,638,637,636,635,634,633,632,629,628,627,626,625,624,
  623,503,501,500,499,498,497,496,495,494,493,487,484,483,422,398,226,132,
  131,0,2,6,7,8,9,15,17,21,83,86,130,193,201,202,203,204,205,206,207,208,
  209,210,211,212,213,214,410,457,504,
645,643,642,641,640,639,638,637,636,635,634,633,632,629,628,627,626,625,624,
  623,503,501,500,499,498,497,496,495,494,493,487,484,483,422,398,226,132,
  131,0,2,6,7,8,9,15,17,21,83,86,130,193,201,202,203,204,205,206,207,208,
  209,210,211,212,213,214,410,457,504,
507,489,488,487,486,485,484,483,482,481,480,479,478,477,476,475,474,473,472,
  471,470,469,468,467,466,465,464,463,462,461,460,458,420,396,394,393,392,
  1,0,51,52,
507,489,488,487,486,485,484,483,482,481,480,479,478,477,476,475,474,473,472,
  471,470,469,468,467,466,465,464,463,462,461,460,458,420,394,392,1,0,51,
  52,192,194,195,196,197,
507,484,483,482,481,480,479,478,477,476,475,474,473,472,471,470,469,468,467,
  466,465,464,463,462,461,460,458,420,396,394,393,392,1,0,51,52,190,191,
507,482,481,480,479,478,477,476,475,474,473,472,471,470,469,468,467,466,465,
  464,463,462,461,460,458,420,396,394,393,392,1,0,51,52,186,187,188,189,
507,478,477,476,475,474,473,472,471,470,469,468,467,466,465,464,463,462,461,
  460,458,420,396,394,393,392,1,0,51,52,184,185,
507,476,475,474,473,472,471,470,469,468,467,466,465,464,463,462,461,460,458,
  420,396,394,393,392,1,0,51,52,182,183,
474,473,0,180,181,
472,471,470,469,468,467,466,465,464,463,462,461,460,396,394,393,392,1,0,161,
  162,163,164,165,166,167,168,169,170,171,172,173,174,175,176,177,178,179,
645,643,642,641,640,639,638,637,636,635,634,633,632,629,628,627,626,625,624,
  623,503,501,500,499,498,497,496,495,494,493,492,491,490,487,484,483,422,
  398,226,132,131,0,2,6,7,8,9,15,17,21,23,24,25,26,27,28,29,57,83,86,130,
  191,193,198,199,200,201,202,203,204,205,206,207,208,209,210,211,212,213,
  214,410,457,504,
396,394,393,392,1,0,51,52,
396,394,393,392,1,0,51,52,
396,394,393,392,1,0,71,
396,394,393,392,1,0,51,52,
396,394,393,392,1,0,51,52,
396,394,393,392,1,0,51,52,
396,394,393,392,1,0,51,52,
646,645,644,643,642,641,640,639,638,637,636,635,634,633,632,631,630,629,628,
  627,626,625,624,623,622,620,492,490,487,486,485,484,483,477,475,473,445,
  422,420,398,394,1,0,
396,394,393,392,1,0,51,52,
396,394,393,392,1,0,51,52,
396,394,393,392,1,0,51,52,
645,644,643,642,641,640,639,638,637,636,635,634,630,629,628,627,626,625,621,
  486,398,1,0,51,52,
645,644,643,642,641,640,639,638,637,636,635,634,630,629,628,627,626,625,621,
  486,398,0,10,12,13,14,19,403,404,405,
646,645,644,643,642,641,640,639,638,637,636,635,634,633,632,631,630,629,628,
  627,626,625,624,623,622,621,620,492,490,487,486,485,484,483,477,475,473,
  445,423,422,420,398,396,395,394,393,392,1,0,51,52,
646,645,644,643,642,641,640,639,638,637,636,635,634,633,632,631,630,629,628,
  627,626,625,624,623,622,621,620,503,501,500,499,498,497,496,495,494,493,
  492,491,490,487,486,485,484,483,477,475,473,445,423,422,420,398,396,395,
  394,393,392,226,132,131,1,0,51,52,
646,645,644,643,642,641,640,639,638,637,636,635,634,633,632,631,630,629,628,
  627,626,625,624,623,622,621,620,492,490,487,486,485,484,483,477,475,473,
  445,423,422,420,398,396,395,394,393,392,1,0,51,52,
646,645,644,643,642,641,640,639,638,637,636,635,634,633,632,631,630,629,628,
  627,626,625,624,623,622,621,620,503,501,500,499,498,497,496,495,494,493,
  492,491,490,487,486,485,484,483,477,475,473,445,423,422,420,398,396,395,
  394,393,392,226,132,131,1,0,51,52,
646,645,644,643,642,641,640,639,638,637,636,635,634,633,632,631,630,629,628,
  627,626,625,624,623,622,621,620,492,490,487,486,485,484,483,477,475,473,
  445,422,420,398,396,395,394,392,1,0,51,52,
646,645,644,643,642,641,640,639,638,637,636,635,634,633,632,631,630,629,628,
  627,626,625,624,623,622,621,620,492,490,487,486,485,484,483,477,475,473,
  445,423,422,420,398,396,395,394,393,392,1,0,51,52,
646,645,644,643,642,641,640,639,638,637,636,635,634,633,632,631,630,629,628,
  627,626,625,624,623,622,621,620,492,490,487,486,485,484,483,477,475,473,
  445,423,422,420,398,396,395,394,393,392,1,0,51,52,
646,645,644,643,642,641,640,639,638,637,636,635,634,633,632,631,630,629,628,
  627,626,625,624,623,622,621,620,492,490,487,486,485,484,483,477,475,473,
  445,423,422,420,398,396,395,394,393,392,1,0,51,52,
646,645,644,643,642,641,640,639,638,637,636,635,634,633,632,631,630,629,628,
  627,626,625,624,623,622,621,620,492,490,487,486,485,484,483,477,475,473,
  445,423,422,420,398,396,395,394,393,392,1,0,51,52,
646,645,644,643,642,641,640,639,638,637,636,635,634,633,632,631,630,629,628,
  627,626,625,624,623,622,621,620,492,490,487,486,485,484,483,477,475,473,
  445,423,422,420,398,396,395,394,393,392,1,0,51,52,
646,645,644,643,642,641,640,639,638,637,636,635,634,633,632,631,630,629,628,
  627,626,625,624,623,622,621,620,492,490,487,486,485,484,483,477,475,473,
  445,423,422,420,398,396,395,394,393,392,1,0,51,52,
646,645,644,643,642,641,640,639,638,637,636,635,634,633,632,631,630,629,628,
  627,626,625,624,623,622,621,620,492,490,487,486,485,484,483,477,475,473,
  445,423,422,420,398,396,395,394,393,392,1,0,51,52,
646,645,644,643,642,641,640,639,638,637,636,635,634,633,632,631,630,629,628,
  627,626,625,624,623,622,621,620,492,490,487,486,485,484,483,477,475,473,
  445,423,422,420,398,396,395,394,393,392,1,0,51,52,
646,645,644,643,642,641,640,639,638,637,636,635,634,633,632,631,630,629,628,
  627,626,625,624,623,622,621,620,492,490,487,486,485,484,483,477,475,473,
  445,423,422,420,398,396,395,394,393,392,1,0,51,52,
646,645,644,643,642,641,640,639,638,637,636,635,634,633,632,631,630,629,628,
  627,626,625,624,623,622,621,620,492,490,487,486,485,484,483,477,475,473,
  445,423,422,420,398,396,395,394,393,392,1,0,51,52,
646,645,644,643,642,641,640,639,638,637,636,635,634,633,632,631,630,629,628,
  627,626,625,624,623,622,621,620,492,490,487,486,485,484,483,477,475,473,
  445,423,422,420,398,396,395,394,393,392,1,0,51,52,
646,645,644,643,642,641,640,639,638,637,636,635,634,633,632,631,630,629,628,
  627,626,625,624,623,622,621,620,492,490,487,486,485,484,483,477,475,473,
  445,423,422,420,398,396,395,394,393,392,1,0,51,52,
646,645,644,643,642,641,640,639,638,637,636,635,634,633,632,631,630,629,628,
  627,626,625,624,623,622,621,620,492,490,487,486,485,484,483,477,475,473,
  445,423,422,420,398,396,395,394,393,392,1,0,51,52,
646,645,644,643,642,641,640,639,638,637,636,635,634,633,632,631,630,629,628,
  627,626,625,624,623,622,621,620,492,490,487,486,485,484,483,477,475,473,
  445,423,422,420,398,396,395,394,393,392,1,0,51,52,
646,645,644,643,642,641,640,639,638,637,636,635,634,633,632,631,630,629,628,
  627,626,625,624,623,622,621,620,492,490,487,486,485,484,483,477,475,473,
  445,423,422,420,398,396,395,394,393,392,1,0,51,52,
646,645,644,643,642,641,640,639,638,637,636,635,634,633,632,631,630,629,628,
  627,626,625,624,623,622,621,620,492,490,487,486,485,484,483,477,475,473,
  445,423,422,420,398,396,395,394,393,392,1,0,51,52,
646,645,644,643,642,641,640,639,638,637,636,635,634,633,632,631,630,629,628,
  627,626,625,624,623,622,621,620,492,490,487,486,485,484,483,477,475,473,
  445,423,422,420,398,396,395,394,393,392,1,0,51,52,
646,645,644,643,642,641,640,639,638,637,636,635,634,633,632,631,630,629,628,
  627,626,625,624,623,622,621,620,492,490,487,486,485,484,483,477,475,473,
  445,423,422,420,398,396,395,394,393,392,1,0,51,52,
646,645,644,643,642,641,640,639,638,637,636,635,634,633,632,631,630,629,628,
  627,626,625,624,623,622,621,620,492,490,487,486,485,484,483,477,475,473,
  445,423,422,420,398,396,395,394,393,392,1,0,51,52,
646,645,644,643,642,641,640,639,638,637,636,635,634,633,632,631,630,629,628,
  627,626,625,624,623,622,621,620,503,501,500,499,498,497,496,495,494,493,
  492,490,487,486,485,484,483,477,475,473,445,423,422,420,398,396,395,394,
  393,392,226,132,131,1,0,51,52,
646,645,644,643,642,641,640,639,638,637,636,635,634,633,632,631,630,629,628,
  627,626,625,624,623,622,621,620,503,501,500,499,498,497,496,495,494,493,
  492,491,490,487,486,485,484,483,477,475,473,445,423,422,420,398,396,395,
  394,393,392,226,132,131,1,0,51,52,
646,645,644,643,642,641,640,639,638,637,636,635,634,633,632,631,630,629,628,
  627,626,625,624,623,622,621,620,503,501,500,499,498,497,496,495,494,493,
  492,491,490,487,486,485,484,483,477,475,473,445,423,422,420,398,396,395,
  394,393,392,226,132,131,1,0,51,52,
646,645,644,643,642,641,640,639,638,637,636,635,634,633,632,631,630,629,628,
  627,626,625,624,623,622,621,620,503,501,500,499,498,497,496,495,494,493,
  492,491,490,487,486,485,484,483,477,475,473,445,423,422,420,398,396,395,
  394,393,392,226,132,131,1,0,51,52,
646,645,644,643,642,641,640,639,638,637,636,635,634,633,632,631,630,629,628,
  627,626,625,624,623,622,621,620,503,501,500,499,498,497,496,495,494,493,
  492,490,487,486,485,484,483,477,475,473,445,423,422,420,398,396,395,394,
  393,392,226,132,131,1,0,51,52,
646,645,644,643,642,641,640,639,638,637,636,635,634,633,632,631,630,629,628,
  627,626,625,624,623,622,621,620,507,492,490,489,488,487,486,485,484,483,
  482,481,480,479,478,477,476,475,474,473,472,471,470,469,468,467,466,465,
  464,463,462,461,460,458,445,423,422,420,398,396,395,394,393,392,1,0,51,
  52,
646,645,644,643,642,641,640,639,638,637,636,635,634,633,632,631,630,629,628,
  627,626,625,624,623,622,621,620,492,490,487,486,485,484,483,477,475,473,
  445,423,422,420,398,396,395,394,393,392,1,0,51,52,
646,645,644,643,642,641,640,639,638,637,636,635,634,633,632,631,630,629,628,
  627,626,625,624,623,622,621,620,503,501,500,499,498,497,496,495,494,493,
  492,491,490,487,486,485,484,483,477,475,473,445,423,422,420,398,396,395,
  394,393,392,226,132,131,1,0,51,52,
646,645,644,643,642,641,640,639,638,637,636,635,634,633,632,631,630,629,628,
  627,626,625,624,623,622,621,620,503,501,500,499,498,497,496,495,494,493,
  492,490,487,486,485,484,483,477,475,473,445,423,422,420,398,396,395,394,
  393,392,226,132,131,1,0,51,52,
646,645,644,643,642,641,640,639,638,637,636,635,634,633,632,631,630,629,628,
  627,626,625,624,623,622,621,620,492,490,487,486,485,484,483,477,475,473,
  445,423,422,420,398,396,395,394,393,392,1,0,51,52,
646,645,644,643,642,641,640,639,638,637,636,635,634,633,632,631,630,629,628,
  627,626,625,624,623,622,621,620,492,490,487,486,485,484,483,477,475,473,
  445,422,420,398,396,395,394,392,1,0,51,52,
646,645,644,643,642,641,640,639,638,637,636,635,634,633,632,631,630,629,628,
  627,626,625,624,623,622,621,620,492,490,487,486,485,484,483,477,475,473,
  445,423,422,420,398,396,395,394,393,392,1,0,51,52,
646,645,644,643,642,641,640,639,638,637,636,635,634,633,632,631,630,629,628,
  627,626,625,624,623,622,621,620,492,490,487,486,485,484,483,477,475,473,
  445,423,422,420,398,396,395,394,393,392,1,0,51,52,
646,645,644,643,642,641,640,639,638,637,636,635,634,633,632,631,630,629,628,
  627,626,625,624,623,622,621,620,492,490,487,486,485,484,483,477,475,473,
  445,422,420,406,402,401,398,396,395,394,393,392,132,131,38,1,0,51,52,
646,645,644,643,642,641,640,639,638,637,636,635,634,633,632,631,630,629,628,
  627,626,625,624,623,622,621,620,492,490,487,486,485,484,483,477,475,473,
  445,422,420,398,394,1,0,46,54,83,85,119,137,138,140,142,143,144,145,146,
  147,151,154,155,180,182,184,190,191,192,194,195,198,200,215,219,220,224,
  381,382,383,384,385,386,387,388,389,390,391,
392,0,45,
392,0,45,
456,455,454,453,452,451,449,448,447,446,444,443,442,441,440,439,438,437,436,
  435,434,433,432,431,430,429,428,427,426,425,424,409,407,400,399,0,56,58,
  68,69,92,93,94,95,96,97,98,99,100,101,102,103,104,105,106,107,108,109,
  111,112,113,114,115,116,117,118,120,121,122,123,124,125,126,127,128,
396,394,393,392,1,0,51,52,
459,456,396,394,393,392,0,22,39,40,41,45,46,47,49,128,159,
506,236,235,234,233,232,231,230,229,0,4,237,554,
245,243,241,239,233,231,229,0,5,240,242,244,566,
628,627,626,625,0,329,
445,0,119,
445,0,119,
645,643,642,641,640,639,638,637,636,635,634,633,632,629,628,627,626,625,624,
  623,503,501,500,499,498,497,496,495,494,493,492,491,490,487,484,483,422,
  398,226,132,131,0,2,6,7,8,9,15,17,21,23,24,25,26,27,28,29,57,83,86,130,
  191,193,198,199,200,201,202,203,204,205,206,207,208,209,210,211,212,213,
  214,410,457,504,
645,643,642,641,640,639,638,637,636,635,634,633,632,629,628,627,626,625,624,
  623,503,501,500,499,498,497,496,495,494,493,492,491,490,487,484,483,422,
  398,226,132,131,0,2,6,7,8,9,15,17,21,23,24,25,26,27,28,29,57,83,86,130,
  191,193,198,199,200,201,202,203,204,205,206,207,208,209,210,211,212,213,
  214,410,457,504,
645,643,642,641,640,639,638,637,636,635,634,633,632,629,628,627,626,625,624,
  623,503,501,500,499,498,497,496,495,494,493,492,491,490,487,484,483,422,
  398,226,132,131,0,2,6,7,8,9,15,17,21,23,24,25,26,27,28,29,57,83,86,130,
  191,193,198,199,200,201,202,203,204,205,206,207,208,209,210,211,212,213,
  214,410,457,504,
645,643,642,641,640,639,638,637,636,635,634,633,632,629,628,627,626,625,624,
  623,503,501,500,499,498,497,496,495,494,493,492,491,490,487,484,483,422,
  398,226,132,131,0,2,6,7,8,9,15,17,21,23,24,25,26,27,28,29,57,83,86,130,
  191,193,198,199,200,201,202,203,204,205,206,207,208,209,210,211,212,213,
  214,410,457,504,
645,643,642,641,640,639,638,637,636,635,634,633,632,629,628,627,626,625,624,
  623,503,501,500,499,498,497,496,495,494,493,492,491,490,487,484,483,422,
  398,226,132,131,0,2,6,7,8,9,15,17,21,23,24,25,26,27,28,29,57,83,86,130,
  191,193,198,199,200,201,202,203,204,205,206,207,208,209,210,211,212,213,
  214,410,457,504,
645,643,642,641,640,639,638,637,636,635,634,633,632,629,628,627,626,625,624,
  623,503,501,500,499,498,497,496,495,494,493,492,491,490,487,484,483,422,
  398,226,132,131,0,2,6,7,8,9,15,17,21,23,24,25,26,27,28,29,57,83,86,130,
  191,193,198,199,200,201,202,203,204,205,206,207,208,209,210,211,212,213,
  214,410,457,504,
645,643,642,641,640,639,638,637,636,635,634,633,632,629,628,627,626,625,624,
  623,503,501,500,499,498,497,496,495,494,493,492,491,490,487,484,483,422,
  398,226,132,131,0,2,6,7,8,9,15,17,21,23,24,25,26,27,28,29,57,83,86,130,
  191,193,198,199,200,201,202,203,204,205,206,207,208,209,210,211,212,213,
  214,410,457,504,
645,643,642,641,640,639,638,637,636,635,634,633,632,629,628,627,626,625,624,
  623,503,501,500,499,498,497,496,495,494,493,492,491,490,487,484,483,422,
  398,226,132,131,0,2,6,7,8,9,15,17,21,23,24,25,26,27,28,29,57,83,86,130,
  191,193,198,199,200,201,202,203,204,205,206,207,208,209,210,211,212,213,
  214,410,457,504,
645,643,642,641,640,639,638,637,636,635,634,633,632,629,628,627,626,625,624,
  623,503,501,500,499,498,497,496,495,494,493,492,491,490,487,484,483,422,
  398,226,132,131,0,2,6,7,8,9,15,17,21,23,24,25,26,27,28,29,57,83,86,130,
  191,193,198,199,200,201,202,203,204,205,206,207,208,209,210,211,212,213,
  214,410,457,504,
645,643,642,641,640,639,638,637,636,635,634,633,632,629,628,627,626,625,624,
  623,503,501,500,499,498,497,496,495,494,493,492,491,490,487,484,483,422,
  398,226,132,131,0,2,6,7,8,9,15,17,21,23,24,25,26,27,28,29,57,83,86,130,
  191,193,198,199,200,201,202,203,204,205,206,207,208,209,210,211,212,213,
  214,410,457,504,
645,643,642,641,640,639,638,637,636,635,634,633,632,629,628,627,626,625,624,
  623,503,501,500,499,498,497,496,495,494,493,492,491,490,487,484,483,422,
  398,226,132,131,0,2,6,7,8,9,15,17,21,23,24,25,26,27,28,29,57,83,86,130,
  191,193,198,199,200,201,202,203,204,205,206,207,208,209,210,211,212,213,
  214,410,457,504,
645,643,642,641,640,639,638,637,636,635,634,633,632,629,628,627,626,625,624,
  623,503,501,500,499,498,497,496,495,494,493,492,491,490,487,484,483,422,
  398,226,132,131,0,2,6,7,8,9,15,17,21,23,24,25,26,27,28,29,57,83,86,130,
  191,193,198,199,200,201,202,203,204,205,206,207,208,209,210,211,212,213,
  214,410,457,504,
645,643,642,641,640,639,638,637,636,635,634,633,632,629,628,627,626,625,624,
  623,503,501,500,499,498,497,496,495,494,493,492,491,490,487,484,483,422,
  398,226,132,131,0,2,6,7,8,9,15,17,21,23,24,25,26,27,28,29,57,83,86,130,
  191,193,198,199,200,201,202,203,204,205,206,207,208,209,210,211,212,213,
  214,410,457,504,
645,643,642,641,640,639,638,637,636,635,634,633,632,629,628,627,626,625,624,
  623,503,501,500,499,498,497,496,495,494,493,492,491,490,487,484,483,422,
  398,226,132,131,0,2,6,7,8,9,15,17,21,23,24,25,26,27,28,29,57,83,86,130,
  191,193,198,199,200,201,202,203,204,205,206,207,208,209,210,211,212,213,
  214,410,457,504,
645,643,642,641,640,639,638,637,636,635,634,633,632,629,628,627,626,625,624,
  623,503,501,500,499,498,497,496,495,494,493,492,491,490,487,484,483,422,
  398,226,132,131,0,2,6,7,8,9,15,17,21,23,24,25,26,27,28,29,57,83,86,130,
  191,193,198,199,200,201,202,203,204,205,206,207,208,209,210,211,212,213,
  214,410,457,504,
645,643,642,641,640,639,638,637,636,635,634,633,632,629,628,627,626,625,624,
  623,503,501,500,499,498,497,496,495,494,493,492,491,490,487,484,483,422,
  398,226,132,131,0,2,6,7,8,9,15,17,21,23,24,25,26,27,28,29,57,83,86,130,
  191,193,198,199,200,201,202,203,204,205,206,207,208,209,210,211,212,213,
  214,410,457,504,
645,643,642,641,640,639,638,637,636,635,634,633,632,629,628,627,626,625,624,
  623,503,501,500,499,498,497,496,495,494,493,492,491,490,487,484,483,422,
  398,226,132,131,0,2,6,7,8,9,15,17,21,23,24,25,26,27,28,29,57,83,86,130,
  191,193,198,199,200,201,202,203,204,205,206,207,208,209,210,211,212,213,
  214,410,457,504,
645,643,642,641,640,639,638,637,636,635,634,633,632,629,628,627,626,625,624,
  623,503,501,500,499,498,497,496,495,494,493,492,491,490,487,484,483,422,
  398,226,132,131,0,2,6,7,8,9,15,17,21,23,24,25,26,27,28,29,57,83,86,130,
  191,193,198,199,200,201,202,203,204,205,206,207,208,209,210,211,212,213,
  214,410,457,504,
645,643,642,641,640,639,638,637,636,635,634,633,632,629,628,627,626,625,624,
  623,503,501,500,499,498,497,496,495,494,493,492,491,490,487,484,483,422,
  398,226,132,131,0,2,6,7,8,9,15,17,21,23,24,25,26,27,28,29,57,83,86,130,
  191,193,198,199,200,201,202,203,204,205,206,207,208,209,210,211,212,213,
  214,410,457,504,
645,643,642,641,640,639,638,637,636,635,634,633,632,629,628,627,626,625,624,
  623,503,501,500,499,498,497,496,495,494,493,492,491,490,487,484,483,422,
  398,226,132,131,0,2,6,7,8,9,15,17,21,23,24,25,26,27,28,29,57,83,86,130,
  191,193,198,199,200,201,202,203,204,205,206,207,208,209,210,211,212,213,
  214,410,457,504,
645,643,642,641,640,639,638,637,636,635,634,633,632,629,628,627,626,625,624,
  623,503,501,500,499,498,497,496,495,494,493,492,491,490,487,484,483,422,
  398,226,132,131,0,2,6,7,8,9,15,17,21,23,24,25,26,27,28,29,57,83,86,130,
  191,193,198,199,200,201,202,203,204,205,206,207,208,209,210,211,212,213,
  214,410,457,504,
645,643,642,641,640,639,638,637,636,635,634,633,632,629,628,627,626,625,624,
  623,503,501,500,499,498,497,496,495,494,493,492,491,490,487,484,483,422,
  398,226,132,131,0,2,6,7,8,9,15,17,21,23,24,25,26,27,28,29,57,83,86,130,
  191,193,198,199,200,201,202,203,204,205,206,207,208,209,210,211,212,213,
  214,410,457,504,
645,643,642,641,640,639,638,637,636,635,634,633,632,629,628,627,626,625,624,
  623,503,501,500,499,498,497,496,495,494,493,492,491,490,487,484,483,422,
  398,226,132,131,0,2,6,7,8,9,15,17,21,23,24,25,26,27,28,29,57,83,86,130,
  191,193,198,199,200,201,202,203,204,205,206,207,208,209,210,211,212,213,
  214,410,457,504,
645,643,642,641,640,639,638,637,636,635,634,633,632,629,628,627,626,625,624,
  623,503,501,500,499,498,497,496,495,494,493,492,491,490,487,484,483,422,
  398,226,132,131,0,2,6,7,8,9,15,17,21,23,24,25,26,27,28,29,57,83,86,130,
  191,193,198,199,200,201,202,203,204,205,206,207,208,209,210,211,212,213,
  214,410,457,504,
645,643,642,641,640,639,638,637,636,635,634,633,632,629,628,627,626,625,624,
  623,503,501,500,499,498,497,496,495,494,493,492,491,490,487,484,483,422,
  398,226,132,131,0,2,6,7,8,9,15,17,21,23,24,25,26,27,28,29,57,83,86,130,
  191,193,198,199,200,201,202,203,204,205,206,207,208,209,210,211,212,213,
  214,410,457,504,
645,643,642,641,640,639,638,637,636,635,634,633,632,629,628,627,626,625,624,
  623,503,501,500,499,498,497,496,495,494,493,492,491,490,487,484,483,422,
  398,226,132,131,0,2,6,7,8,9,15,17,21,23,24,25,26,27,28,29,57,83,86,130,
  191,193,198,199,200,201,202,203,204,205,206,207,208,209,210,211,212,213,
  214,410,457,504,
645,643,642,641,640,639,638,637,636,635,634,633,632,629,628,627,626,625,624,
  623,503,501,500,499,498,497,496,495,494,493,492,491,490,487,484,483,422,
  398,226,132,131,0,2,6,7,8,9,15,17,21,23,24,25,26,27,28,29,57,83,86,130,
  191,193,198,199,200,201,202,203,204,205,206,207,208,209,210,211,212,213,
  214,410,457,504,
645,643,642,641,640,639,638,637,636,635,634,633,632,629,628,627,626,625,624,
  623,503,501,500,499,498,497,496,495,494,493,492,491,490,487,484,483,422,
  398,226,132,131,0,2,6,7,8,9,15,17,21,23,24,25,26,27,28,29,57,83,86,130,
  191,193,198,199,200,201,202,203,204,205,206,207,208,209,210,211,212,213,
  214,410,457,504,
645,643,642,641,640,639,638,637,636,635,634,633,632,629,628,627,626,625,624,
  623,503,501,500,499,498,497,496,495,494,493,492,491,490,487,484,483,422,
  398,226,132,131,0,2,6,7,8,9,15,17,21,23,24,25,26,27,28,29,57,83,86,130,
  191,193,198,199,200,201,202,203,204,205,206,207,208,209,210,211,212,213,
  214,410,457,504,
645,643,642,641,640,639,638,637,636,635,634,633,632,629,628,627,626,625,624,
  623,503,501,500,499,498,497,496,495,494,493,492,491,490,487,484,483,422,
  398,226,132,131,0,2,6,7,8,9,15,17,21,23,24,25,26,27,28,29,57,83,86,130,
  191,193,198,199,200,201,202,203,204,205,206,207,208,209,210,211,212,213,
  214,410,457,504,
645,643,642,641,640,639,638,637,636,635,634,633,632,629,628,627,626,625,624,
  623,503,501,500,499,498,497,496,495,494,493,492,491,490,487,484,483,422,
  398,226,132,131,0,2,6,7,8,9,15,17,21,23,24,25,26,27,28,29,57,83,86,130,
  191,193,198,199,200,201,202,203,204,205,206,207,208,209,210,211,212,213,
  214,410,457,504,
645,643,642,641,640,639,638,637,636,635,634,633,632,629,628,627,626,625,624,
  623,503,501,500,499,498,497,496,495,494,493,492,491,490,487,484,483,422,
  398,226,132,131,0,2,6,7,8,9,15,17,21,23,24,25,26,27,28,29,57,83,86,130,
  191,193,198,199,200,201,202,203,204,205,206,207,208,209,210,211,212,213,
  214,410,457,504,
645,643,642,641,640,639,638,637,636,635,634,633,632,629,628,627,626,625,624,
  623,503,501,500,499,498,497,496,495,494,493,492,491,490,487,484,483,422,
  398,226,132,131,0,2,6,7,8,9,15,17,21,23,24,25,26,27,28,29,57,83,86,130,
  191,193,198,199,200,201,202,203,204,205,206,207,208,209,210,211,212,213,
  214,410,457,504,
645,643,642,641,640,639,638,637,636,635,634,633,632,629,628,627,626,625,624,
  623,503,501,500,499,498,497,496,495,494,493,492,491,490,487,484,483,422,
  398,226,132,131,0,2,6,7,8,9,15,17,21,23,24,25,26,27,28,29,57,83,86,130,
  191,193,198,199,200,201,202,203,204,205,206,207,208,209,210,211,212,213,
  214,410,457,504,
645,643,642,641,640,639,638,637,636,635,634,633,632,629,628,627,626,625,624,
  623,503,501,500,499,498,497,496,495,494,493,492,491,490,487,484,483,422,
  398,226,132,131,0,2,6,7,8,9,15,17,21,23,24,25,26,27,28,29,57,83,86,130,
  191,193,198,199,200,201,202,203,204,205,206,207,208,209,210,211,212,213,
  214,410,457,504,
645,643,642,641,640,639,638,637,636,635,634,633,632,629,628,627,626,625,624,
  623,503,501,500,499,498,497,496,495,494,493,492,491,490,487,484,483,422,
  398,226,132,131,0,2,6,7,8,9,15,17,21,23,24,25,26,27,28,29,57,83,86,130,
  191,193,198,199,200,201,202,203,204,205,206,207,208,209,210,211,212,213,
  214,410,457,504,
506,505,243,241,239,233,231,229,0,238,240,242,244,246,247,
506,505,243,241,239,233,231,229,0,238,240,242,244,246,247,
241,239,231,229,0,240,242,248,
241,239,231,229,0,240,242,248,
245,243,241,239,233,231,229,0,5,240,242,244,566,
245,243,241,239,233,231,229,0,5,240,242,244,566,
245,243,241,239,233,231,229,0,5,240,242,244,566,
506,236,235,234,233,232,231,230,229,0,4,237,554,
506,236,235,234,233,232,231,230,229,0,4,237,554,
506,236,235,234,233,232,231,230,229,0,4,237,554,
506,236,235,234,233,232,231,230,229,0,4,237,554,
506,236,235,234,233,232,231,230,229,0,4,237,554,
506,236,235,234,233,232,231,230,229,0,4,237,554,
506,236,235,234,233,232,231,230,229,0,4,237,554,
506,236,235,234,233,232,231,230,229,0,4,237,554,
506,236,235,234,233,232,231,230,229,0,4,237,554,
506,236,235,234,233,232,231,230,229,0,4,237,554,
506,236,235,234,233,232,231,230,229,0,4,237,554,
645,643,642,641,640,639,638,637,636,635,634,633,632,629,628,627,626,625,624,
  623,621,503,501,500,499,498,497,496,495,494,493,492,491,490,487,484,483,
  422,398,226,132,131,0,2,6,7,8,9,10,12,15,17,18,20,21,23,24,25,26,27,28,
  29,57,83,86,87,130,191,193,198,199,200,201,202,203,204,205,206,207,208,
  209,210,211,212,213,214,403,410,450,457,504,
633,629,628,627,626,625,624,623,487,484,483,226,1,0,2,6,7,15,17,213,214,457,
619,618,617,616,615,614,613,612,611,610,609,608,607,606,605,604,603,602,601,
  600,599,598,597,596,595,594,593,592,591,590,589,588,587,586,585,584,583,
  582,581,580,579,578,577,576,575,574,573,572,571,570,569,568,567,565,564,
  563,562,561,560,559,558,557,556,555,553,552,551,550,549,548,547,546,545,
  544,543,542,541,540,539,538,537,536,535,534,533,532,531,530,529,528,527,
  526,525,524,523,522,521,520,519,518,517,516,515,514,513,512,511,510,509,
  508,1,0,51,52,
619,618,617,616,615,614,613,612,611,610,609,608,607,606,605,604,603,602,601,
  600,599,598,597,596,595,594,593,592,591,590,589,588,587,586,585,584,583,
  582,581,580,579,578,577,576,575,574,573,572,571,570,569,568,567,565,564,
  563,562,561,560,559,558,557,556,555,553,552,551,550,549,548,547,546,545,
  544,543,542,541,540,539,538,537,536,535,534,533,532,531,530,529,528,527,
  526,525,524,523,522,521,520,519,518,517,516,515,514,513,512,511,510,509,
  508,1,0,51,52,
396,394,393,392,1,0,51,52,
645,643,642,641,640,639,638,637,636,635,634,632,623,132,131,1,0,51,52,
645,643,642,641,640,639,638,637,636,635,634,632,623,132,131,0,21,130,410,
645,644,643,642,641,640,639,638,637,636,635,634,629,628,627,626,625,486,398,
  1,0,51,52,
645,644,643,642,641,640,639,638,637,636,635,634,629,628,627,626,625,486,398,
  0,19,405,
396,394,393,392,1,0,51,52,
396,394,393,392,1,0,51,52,
624,621,1,0,51,52,
624,621,0,10,12,18,20,403,450,
396,394,393,392,1,0,51,52,
396,394,393,392,1,0,51,52,
645,643,642,641,640,639,638,637,636,635,634,632,623,396,394,393,392,132,131,
  1,0,51,52,
645,643,642,641,640,639,638,637,636,635,634,632,623,396,394,393,392,132,131,
  1,0,21,130,410,
621,1,0,51,52,
621,0,10,12,403,
645,643,642,641,640,639,638,637,636,635,634,633,632,629,628,627,626,625,624,
  623,621,503,501,500,499,498,497,496,495,494,493,492,491,490,487,484,483,
  422,398,226,132,131,1,0,51,52,
645,643,642,641,640,639,638,637,636,635,634,633,632,629,628,627,626,625,624,
  623,621,503,501,500,499,498,497,496,495,494,493,492,491,490,487,484,483,
  422,398,226,132,131,1,0,51,52,
645,643,642,641,640,639,638,637,636,635,634,633,632,629,628,627,626,625,624,
  623,621,503,501,500,499,498,497,496,495,494,493,492,491,490,487,484,483,
  422,398,226,132,131,1,0,51,52,
645,643,642,641,640,639,638,637,636,635,634,633,632,629,628,627,626,625,624,
  623,621,503,501,500,499,498,497,496,495,494,493,492,491,490,487,484,483,
  422,398,226,132,131,0,2,6,7,8,9,10,12,15,17,21,23,24,25,26,27,28,29,57,
  83,86,130,191,193,198,199,200,201,202,203,204,205,206,207,208,209,210,
  211,212,213,214,403,410,457,504,
645,643,642,641,640,639,638,637,636,635,634,633,632,629,628,627,626,625,624,
  623,503,501,500,499,498,497,496,495,494,493,492,491,490,487,484,483,422,
  398,226,132,131,1,0,51,52,
645,643,642,641,640,639,638,637,636,635,634,633,632,629,628,627,626,625,624,
  623,503,501,500,499,498,497,496,495,494,493,492,491,490,487,484,483,422,
  398,226,132,131,0,2,6,7,8,9,15,17,21,23,24,25,26,27,28,29,57,83,86,130,
  191,193,198,199,200,201,202,203,204,205,206,207,208,209,210,211,212,213,
  214,410,457,504,
645,643,642,641,640,639,638,637,636,635,634,632,623,132,131,1,0,51,52,
645,643,642,641,640,639,638,637,636,635,634,632,623,132,131,0,21,130,410,
645,643,642,641,640,639,638,637,636,635,634,632,623,132,131,1,0,51,52,
645,643,642,641,640,639,638,637,636,635,634,632,623,132,131,0,21,130,410,
645,643,642,641,640,639,638,637,636,635,634,632,623,132,131,1,0,51,52,
645,643,642,641,640,639,638,637,636,635,634,632,623,132,131,0,21,110,130,
  410,
645,643,642,641,640,639,638,637,636,635,634,632,623,132,131,1,0,51,52,
645,643,642,641,640,639,638,637,636,635,634,632,623,132,131,0,21,110,130,
  410,
645,643,642,641,640,639,638,637,636,635,634,632,623,132,131,1,0,51,52,
645,643,642,641,640,639,638,637,636,635,634,632,623,132,131,0,21,110,130,
  410,
645,643,642,641,640,639,638,637,636,635,634,633,632,629,628,627,626,625,624,
  623,621,503,501,500,499,498,497,496,495,494,493,492,491,490,487,484,483,
  422,398,226,132,131,1,0,51,52,
645,643,642,641,640,639,638,637,636,635,634,633,632,629,628,627,626,625,624,
  623,621,503,501,500,499,498,497,496,495,494,493,492,491,490,487,484,483,
  422,398,226,132,131,1,0,51,52,
645,643,642,641,640,639,638,637,636,635,634,633,632,629,628,627,626,625,624,
  623,621,503,501,500,499,498,497,496,495,494,493,492,491,490,487,484,483,
  422,398,226,132,131,0,2,6,7,8,9,10,12,15,17,18,20,21,23,24,25,26,27,28,
  29,57,83,86,87,130,191,193,198,199,200,201,202,203,204,205,206,207,208,
  209,210,211,212,213,214,403,410,450,457,504,
645,643,642,641,640,639,638,637,636,635,634,633,632,629,628,627,626,625,624,
  623,621,503,501,500,499,498,497,496,495,494,493,492,491,490,487,484,483,
  422,398,226,132,131,1,0,51,52,
645,643,642,641,640,639,638,637,636,635,634,633,632,629,628,627,626,625,624,
  623,621,503,501,500,499,498,497,496,495,494,493,492,491,490,487,484,483,
  422,398,226,132,131,1,0,51,52,
645,643,642,641,640,639,638,637,636,635,634,633,632,629,628,627,626,625,624,
  623,621,503,501,500,499,498,497,496,495,494,493,492,491,490,487,484,483,
  422,398,226,132,131,1,0,51,52,
645,643,642,641,640,639,638,637,636,635,634,633,632,629,628,627,626,625,624,
  623,621,503,501,500,499,498,497,496,495,494,493,492,491,490,487,484,483,
  422,398,226,132,131,0,2,6,7,8,9,10,12,15,17,18,20,21,23,24,25,26,27,28,
  29,57,83,86,87,130,191,193,198,199,200,201,202,203,204,205,206,207,208,
  209,210,211,212,213,214,403,410,450,457,504,
645,643,642,641,640,639,638,637,636,635,634,633,632,629,628,627,626,625,624,
  623,503,501,500,499,498,497,496,495,494,493,492,491,490,487,484,483,422,
  398,226,132,131,1,0,51,52,
645,643,642,641,640,639,638,637,636,635,634,633,632,629,628,627,626,625,624,
  623,503,501,500,499,498,497,496,495,494,493,492,491,490,487,484,483,422,
  398,226,132,131,1,0,51,52,
645,643,642,641,640,639,638,637,636,635,634,633,632,629,628,627,626,625,624,
  623,503,501,500,499,498,497,496,495,494,493,492,491,490,487,484,483,422,
  398,226,132,131,0,2,6,7,8,9,15,17,21,23,24,25,26,27,28,29,57,83,86,130,
  191,193,198,199,200,201,202,203,204,205,206,207,208,209,210,211,212,213,
  214,410,457,504,
396,394,393,392,1,0,51,52,
645,643,642,641,640,639,638,637,636,635,634,633,632,629,628,627,626,625,624,
  623,503,501,500,499,498,497,496,495,494,493,492,491,490,487,484,483,422,
  398,226,132,131,1,0,51,52,
645,643,642,641,640,639,638,637,636,635,634,633,632,629,628,627,626,625,624,
  623,503,501,500,499,498,497,496,495,494,493,492,491,490,487,484,483,422,
  398,226,132,131,1,0,51,52,
645,643,642,641,640,639,638,637,636,635,634,633,632,629,628,627,626,625,624,
  623,503,501,500,499,498,497,496,495,494,493,492,491,490,487,484,483,422,
  398,226,132,131,1,0,51,52,
645,643,642,641,640,639,638,637,636,635,634,633,632,629,628,627,626,625,624,
  623,503,501,500,499,498,497,496,495,494,493,492,491,490,487,484,483,422,
  398,226,132,131,0,2,6,7,8,9,15,17,21,23,24,25,26,27,28,29,57,83,86,130,
  191,193,198,199,200,201,202,203,204,205,206,207,208,209,210,211,212,213,
  214,410,457,504,
459,456,396,394,393,392,1,0,51,52,
459,456,396,394,393,392,1,0,51,52,
396,394,393,392,0,39,40,41,45,46,47,49,
421,0,84,
646,645,643,642,641,640,639,638,637,636,635,634,633,632,631,630,629,628,627,
  626,625,624,623,621,620,492,490,487,486,485,484,483,477,475,473,445,422,
  420,398,394,0,46,54,82,83,85,119,137,138,142,143,144,145,146,147,151,
  154,155,180,182,184,190,191,192,194,195,198,200,215,219,220,224,381,382,
  384,385,386,387,388,389,390,391,
396,394,393,392,0,39,40,41,45,46,47,49,
644,643,642,641,640,637,635,627,626,625,623,621,392,0,152,
396,394,393,392,0,39,40,41,45,46,47,49,
396,394,393,392,0,39,40,41,45,46,47,49,
637,636,635,634,629,628,627,626,625,0,
644,642,641,640,625,624,0,
645,644,643,642,641,640,639,638,637,636,635,634,633,632,631,630,629,628,627,
  626,625,624,623,622,621,620,492,490,487,486,485,484,483,477,475,473,445,
  422,420,398,394,392,228,1,0,16,
639,637,636,635,634,629,628,627,626,625,507,489,488,487,486,485,484,483,482,
  481,480,479,478,477,476,475,474,473,472,471,470,469,468,467,466,465,464,
  463,462,461,460,458,420,396,394,393,392,1,0,
639,637,636,635,634,629,628,627,626,625,507,489,488,487,486,485,484,483,482,
  481,480,479,478,477,476,475,474,473,472,471,470,469,468,467,466,465,464,
  463,462,461,460,458,420,396,394,393,392,1,0,
422,0,83,
422,0,83,
422,0,83,
422,0,83,
422,0,83,
422,0,83,
422,0,83,
422,0,83,
422,0,83,
422,0,83,
645,643,642,641,640,639,638,637,636,635,634,633,632,629,628,627,626,625,624,
  623,503,501,500,499,498,497,496,495,494,493,492,491,490,487,484,483,422,
  398,226,132,131,0,2,6,7,8,9,15,17,21,23,24,25,26,27,28,29,57,83,86,130,
  191,193,198,199,200,201,202,203,204,205,206,207,208,209,210,211,212,213,
  214,410,457,504,
629,628,627,626,625,0,
645,643,642,641,640,639,638,637,636,635,634,633,632,629,628,627,626,625,624,
  623,503,501,500,499,498,497,496,495,494,493,487,484,483,422,398,226,132,
  131,1,0,51,52,
645,643,642,641,640,639,638,637,636,635,634,633,632,629,628,627,626,625,624,
  623,503,501,500,499,498,497,496,495,494,493,487,484,483,422,398,226,132,
  131,1,0,51,52,
645,643,642,641,640,639,638,637,636,635,634,633,632,629,628,627,626,625,624,
  623,503,501,500,499,498,497,496,495,494,493,487,484,483,422,398,226,132,
  131,1,0,51,52,
645,643,642,641,640,639,638,637,636,635,634,633,632,629,628,627,626,625,624,
  623,503,501,500,499,498,497,496,495,494,493,487,484,483,422,398,226,132,
  131,1,0,51,52,
645,643,642,641,640,639,638,637,636,635,634,633,632,629,628,627,626,625,624,
  623,503,501,500,499,498,497,496,495,494,493,487,484,483,422,398,226,132,
  131,1,0,51,52,
645,643,642,641,640,639,638,637,636,635,634,633,632,629,628,627,626,625,624,
  623,503,501,500,499,498,497,496,495,494,493,487,484,483,422,398,226,132,
  131,1,0,51,52,
645,643,642,641,640,639,638,637,636,635,634,633,632,629,628,627,626,625,624,
  623,503,501,500,499,498,497,496,495,494,493,487,484,483,422,398,226,132,
  131,1,0,51,52,
645,643,642,641,640,639,638,637,636,635,634,633,632,629,628,627,626,625,624,
  623,503,501,500,499,498,497,496,495,494,493,492,491,490,487,484,483,422,
  398,226,132,131,1,0,51,52,
645,643,642,641,640,639,638,637,636,635,634,633,632,629,628,627,626,625,624,
  623,503,501,500,499,498,497,496,495,494,493,492,491,490,487,484,483,422,
  398,226,132,131,1,0,51,52,
645,643,642,641,640,639,638,637,636,635,634,633,632,629,628,627,626,625,624,
  623,503,501,500,499,498,497,496,495,494,493,492,491,490,487,484,483,422,
  398,226,132,131,1,0,51,52,
645,643,642,641,640,639,638,637,636,635,634,633,632,629,628,627,626,625,624,
  623,503,501,500,499,498,497,496,495,494,493,492,491,490,487,484,483,422,
  398,226,132,131,1,0,51,52,
645,643,642,641,640,639,638,637,636,635,634,633,632,629,628,627,626,625,624,
  623,503,501,500,499,498,497,496,495,494,493,492,491,490,487,484,483,422,
  398,226,132,131,1,0,51,52,
645,643,642,641,640,639,638,637,636,635,634,633,632,629,628,627,626,625,624,
  623,503,501,500,499,498,497,496,495,494,493,492,491,490,487,484,483,422,
  398,226,132,131,1,0,51,52,
645,643,642,641,640,639,638,637,636,635,634,633,632,629,628,627,626,625,624,
  623,503,501,500,499,498,497,496,495,494,493,492,491,490,487,484,483,422,
  398,226,132,131,1,0,51,52,
645,643,642,641,640,639,638,637,636,635,634,633,632,629,628,627,626,625,624,
  623,503,501,500,499,498,497,496,495,494,493,492,491,490,487,484,483,422,
  398,226,132,131,1,0,51,52,
645,643,642,641,640,639,638,637,636,635,634,633,632,629,628,627,626,625,624,
  623,503,501,500,499,498,497,496,495,494,493,492,491,490,487,484,483,422,
  398,226,132,131,1,0,51,52,
645,643,642,641,640,639,638,637,636,635,634,633,632,629,628,627,626,625,624,
  623,503,501,500,499,498,497,496,495,494,493,492,491,490,487,484,483,422,
  398,226,132,131,1,0,51,52,
645,643,642,641,640,639,638,637,636,635,634,633,632,629,628,627,626,625,624,
  623,503,501,500,499,498,497,496,495,494,493,492,491,490,487,484,483,422,
  398,226,132,131,1,0,51,52,
645,643,642,641,640,639,638,637,636,635,634,633,632,629,628,627,626,625,624,
  623,503,501,500,499,498,497,496,495,494,493,492,491,490,487,484,483,422,
  398,226,132,131,1,0,51,52,
645,643,642,641,640,639,638,637,636,635,634,633,632,629,628,627,626,625,624,
  623,503,501,500,499,498,497,496,495,494,493,492,491,490,487,484,483,422,
  398,226,132,131,1,0,51,52,
645,643,642,641,640,639,638,637,636,635,634,633,632,629,628,627,626,625,624,
  623,503,501,500,499,498,497,496,495,494,493,492,491,490,487,484,483,422,
  398,226,132,131,1,0,51,52,
645,643,642,641,640,639,638,637,636,635,634,633,632,629,628,627,626,625,624,
  623,503,501,500,499,498,497,496,495,494,493,492,491,490,487,484,483,422,
  398,226,132,131,1,0,51,52,
645,643,642,641,640,639,638,637,636,635,634,633,632,629,628,627,626,625,624,
  623,503,501,500,499,498,497,496,495,494,493,492,491,490,487,484,483,422,
  398,226,132,131,1,0,51,52,
645,643,642,641,640,639,638,637,636,635,634,633,632,629,628,627,626,625,624,
  623,503,501,500,499,498,497,496,495,494,493,492,491,490,487,484,483,422,
  398,226,132,131,1,0,51,52,
645,643,642,641,640,639,638,637,636,635,634,633,632,629,628,627,626,625,624,
  623,503,501,500,499,498,497,496,495,494,493,492,491,490,487,484,483,422,
  398,226,132,131,1,0,51,52,
645,643,642,641,640,639,638,637,636,635,634,633,632,629,628,627,626,625,624,
  623,503,501,500,499,498,497,496,495,494,493,492,491,490,487,484,483,422,
  398,226,132,131,1,0,51,52,
645,643,642,641,640,639,638,637,636,635,634,633,632,629,628,627,626,625,624,
  623,503,501,500,499,498,497,496,495,494,493,492,491,490,487,484,483,422,
  398,226,132,131,1,0,51,52,
645,643,642,641,640,639,638,637,636,635,634,633,632,629,628,627,626,625,624,
  623,503,501,500,499,498,497,496,495,494,493,492,491,490,487,484,483,422,
  398,226,132,131,1,0,51,52,
645,643,642,641,640,639,638,637,636,635,634,633,632,629,628,627,626,625,624,
  623,503,501,500,499,498,497,496,495,494,493,492,491,490,487,484,483,422,
  398,226,132,131,1,0,51,52,
645,643,642,641,640,639,638,637,636,635,634,633,632,629,628,627,626,625,624,
  623,503,501,500,499,498,497,496,495,494,493,492,491,490,487,484,483,422,
  398,226,132,131,1,0,51,52,
645,643,642,641,640,639,638,637,636,635,634,633,632,629,628,627,626,625,624,
  623,503,501,500,499,498,497,496,495,494,493,492,491,490,487,484,483,422,
  398,226,132,131,1,0,51,52,
645,643,642,641,640,639,638,637,636,635,634,633,632,629,628,627,626,625,624,
  623,503,501,500,499,498,497,496,495,494,493,492,491,490,487,484,483,422,
  398,226,132,131,1,0,51,52,
645,643,642,641,640,639,638,637,636,635,634,633,632,629,628,627,626,625,624,
  623,503,501,500,499,498,497,496,495,494,493,492,491,490,487,484,483,422,
  398,226,132,131,1,0,51,52,
645,643,642,641,640,639,638,637,636,635,634,633,632,629,628,627,626,625,624,
  623,503,501,500,499,498,497,496,495,494,493,492,491,490,487,484,483,422,
  398,226,132,131,1,0,51,52,
645,643,642,641,640,639,638,637,636,635,634,633,632,629,628,627,626,625,624,
  623,503,501,500,499,498,497,496,495,494,493,492,491,490,487,484,483,422,
  398,226,132,131,1,0,51,52,
645,643,642,641,640,639,638,637,636,635,634,633,632,629,628,627,626,625,624,
  623,503,501,500,499,498,497,496,495,494,493,492,491,490,487,484,483,422,
  398,226,132,131,1,0,51,52,
645,643,642,641,640,639,638,637,636,635,634,633,632,629,628,627,626,625,624,
  623,503,501,500,499,498,497,496,495,494,493,492,491,490,487,484,483,422,
  398,226,132,131,1,0,51,52,
645,643,642,641,640,639,638,637,636,635,634,633,632,629,628,627,626,625,624,
  623,503,501,500,499,498,497,496,495,494,493,492,491,490,487,484,483,422,
  398,226,132,131,1,0,51,52,
645,643,642,641,640,639,638,637,636,635,634,633,632,629,628,627,626,625,624,
  623,503,501,500,499,498,497,496,495,494,493,492,491,490,487,484,483,422,
  398,226,132,131,1,0,51,52,
396,394,393,392,0,39,40,41,45,46,47,49,
396,394,393,392,0,39,40,41,45,46,47,49,
396,394,393,392,0,39,40,41,45,46,47,49,
396,394,393,392,0,39,40,41,45,46,47,49,
396,394,393,392,0,39,40,41,45,46,47,49,
646,645,644,643,642,641,640,639,638,637,636,635,634,633,632,631,630,629,628,
  627,626,625,624,623,622,621,620,492,490,487,486,485,484,483,477,475,473,
  445,422,420,398,394,392,1,0,
396,394,393,392,0,39,40,41,45,46,47,49,
396,394,393,392,0,39,40,41,45,46,47,49,
645,644,643,642,641,640,639,638,637,636,635,634,629,628,627,626,625,486,398,
  394,392,1,0,51,52,
396,394,393,392,1,0,51,52,
396,394,393,392,1,0,51,52,
396,394,393,392,1,0,51,52,
645,643,642,641,640,639,638,637,636,635,634,633,632,629,628,627,626,625,624,
  623,503,501,500,499,498,497,496,495,494,493,492,491,490,487,484,483,422,
  398,226,132,131,1,0,51,52,
645,643,642,641,640,639,638,637,636,635,634,633,632,629,628,627,626,625,624,
  623,503,501,500,499,498,497,496,495,494,493,492,491,490,487,484,483,422,
  398,226,132,131,0,2,6,7,8,9,15,17,21,23,24,25,26,27,28,29,57,83,86,130,
  191,193,198,199,200,201,202,203,204,205,206,207,208,209,210,211,212,213,
  214,410,457,504,
645,643,642,641,640,639,638,637,636,635,634,633,632,629,628,627,626,625,624,
  623,503,501,500,499,498,497,496,495,494,493,492,491,490,487,484,483,422,
  398,226,132,131,1,0,51,52,
645,643,642,641,640,639,638,637,636,635,634,633,632,629,628,627,626,625,624,
  623,503,501,500,499,498,497,496,495,494,493,492,491,490,487,484,483,422,
  398,226,132,131,0,2,6,7,8,9,15,17,21,23,24,25,26,27,28,29,57,83,86,130,
  191,193,198,199,200,201,202,203,204,205,206,207,208,209,210,211,212,213,
  214,410,457,504,
396,394,393,392,0,39,40,41,45,46,47,49,
396,394,393,392,1,0,51,52,
396,394,393,392,1,0,51,52,
507,458,445,396,394,393,392,1,0,51,52,
507,445,396,394,393,392,1,0,51,52,
645,643,642,641,640,639,638,637,636,635,634,633,632,629,628,627,626,625,624,
  623,503,501,500,499,498,497,496,495,494,493,492,491,490,487,484,483,422,
  398,226,132,131,1,0,51,52,
645,643,642,641,640,639,638,637,636,635,634,633,632,629,628,627,626,625,624,
  623,503,501,500,499,498,497,496,495,494,493,492,491,490,487,484,483,422,
  398,226,132,131,1,0,51,52,
507,396,394,393,392,1,0,51,52,
507,396,394,393,392,1,0,51,52,
458,1,0,51,52,
645,644,643,642,641,640,639,638,637,636,635,634,633,632,631,630,629,628,627,
  626,625,623,622,621,620,492,490,487,486,485,484,483,477,475,473,445,422,
  420,398,394,392,228,1,0,16,
645,644,643,642,641,640,639,638,637,636,635,634,633,632,631,630,629,628,627,
  626,625,624,623,622,621,620,492,490,487,486,485,484,483,477,475,473,445,
  422,420,398,394,392,228,1,0,16,
458,420,396,394,393,392,1,0,51,52,
458,420,0,85,129,
637,636,635,634,629,628,627,626,625,0,
645,639,637,636,635,634,629,628,627,626,625,623,396,394,393,392,217,216,1,0,
  218,
619,618,617,616,615,614,613,612,611,610,609,608,607,606,605,604,603,602,601,
  600,599,598,597,596,595,594,593,592,591,590,589,588,587,586,585,584,583,
  582,581,580,579,578,577,576,575,574,573,572,571,570,569,568,567,565,564,
  563,562,561,560,559,558,557,556,555,553,552,551,550,549,548,547,546,545,
  544,543,542,541,540,539,538,537,536,535,534,533,532,531,530,529,528,527,
  526,525,524,523,522,521,520,519,518,517,516,515,514,513,512,511,510,509,
  508,0,31,32,249,250,251,252,253,254,255,256,257,259,260,261,262,263,264,
  265,266,267,268,269,270,271,272,273,274,275,276,277,278,279,280,281,282,
  283,284,285,286,287,288,289,290,291,292,293,294,295,296,297,298,299,300,
  301,302,303,304,305,306,307,308,309,310,311,312,313,314,315,316,317,318,
  319,320,321,322,323,324,325,326,327,328,330,331,332,333,334,335,336,337,
  338,339,340,341,342,343,344,345,346,347,348,349,350,351,352,353,354,355,
  356,357,358,359,360,361,362,363,364,365,366,367,368,369,370,371,372,373,
  374,375,376,377,378,379,380,
645,644,643,642,641,640,639,638,637,636,635,634,633,632,631,630,629,628,627,
  626,625,623,622,621,620,492,490,487,486,485,484,483,477,475,473,445,422,
  420,398,394,392,228,1,0,16,
445,0,119,
645,643,642,641,640,639,638,637,636,635,634,633,632,629,628,627,626,625,624,
  623,621,503,501,500,499,498,497,496,495,494,493,492,491,490,487,484,483,
  422,398,226,132,131,0,2,6,7,8,9,10,12,15,17,18,20,21,23,24,25,26,27,28,
  29,57,83,86,87,130,191,193,198,199,200,201,202,203,204,205,206,207,208,
  209,210,211,212,213,214,403,410,450,457,504,
458,396,394,393,392,1,0,129,
458,396,394,393,392,1,0,129,
458,396,394,393,392,1,0,129,
458,396,394,393,392,1,0,129,
458,396,394,393,392,1,0,129,
420,1,0,51,52,
420,0,85,
646,645,643,642,641,640,639,638,637,636,635,634,633,632,631,630,629,628,627,
  626,625,624,623,621,620,492,490,487,486,485,484,483,477,475,473,445,423,
  422,420,398,394,392,1,0,46,51,52,54,83,85,90,119,137,138,142,143,144,
  145,146,147,151,154,155,180,182,184,190,191,192,194,195,198,200,215,219,
  220,224,381,382,384,385,386,387,388,389,390,391,
637,636,635,634,629,628,627,626,625,0,
629,628,627,626,625,0,
645,644,643,642,641,640,639,638,637,636,635,634,633,632,631,630,629,628,627,
  626,625,624,623,622,621,620,492,490,487,486,485,484,483,477,475,473,445,
  422,420,398,394,392,1,0,
624,0,
645,643,642,641,640,639,638,637,636,635,634,632,623,132,131,1,0,51,52,
645,643,642,641,640,639,638,637,636,635,634,633,632,629,628,627,626,625,624,
  623,503,501,500,499,498,497,496,495,494,493,492,491,490,487,484,483,422,
  398,226,132,131,0,2,6,7,8,9,15,17,21,23,24,25,26,27,28,29,57,83,86,130,
  191,193,198,199,200,201,202,203,204,205,206,207,208,209,210,211,212,213,
  214,410,457,504,
645,643,642,641,640,639,638,637,636,635,634,633,632,629,628,627,626,625,624,
  623,503,501,500,499,498,497,496,495,494,493,492,491,490,487,484,483,422,
  398,226,132,131,0,2,6,7,8,9,15,17,21,23,24,25,26,27,28,29,57,83,86,130,
  191,193,198,199,200,201,202,203,204,205,206,207,208,209,210,211,212,213,
  214,410,457,504,
645,643,642,641,640,639,638,637,636,635,634,633,632,629,628,627,626,625,624,
  623,503,501,500,499,498,497,496,495,494,493,492,491,490,487,484,483,422,
  398,226,132,131,0,2,6,7,8,9,15,17,21,23,24,25,26,27,28,29,57,83,86,130,
  191,193,198,199,200,201,202,203,204,205,206,207,208,209,210,211,212,213,
  214,410,457,504,
645,643,642,641,640,639,638,637,636,635,634,633,632,629,628,627,626,625,624,
  623,503,501,500,499,498,497,496,495,494,493,492,491,490,487,484,483,422,
  398,226,132,131,0,2,6,7,8,9,15,17,21,23,24,25,26,27,28,29,57,83,86,130,
  191,193,198,199,200,201,202,203,204,205,206,207,208,209,210,211,212,213,
  214,410,457,504,
645,643,642,641,640,639,638,637,636,635,634,633,632,629,628,627,626,625,624,
  623,503,501,500,499,498,497,496,495,494,493,492,491,490,487,484,483,422,
  398,226,132,131,0,2,6,7,8,9,15,17,21,23,24,25,26,27,28,29,57,83,86,130,
  191,193,198,199,200,201,202,203,204,205,206,207,208,209,210,211,212,213,
  214,410,457,504,
645,643,642,641,640,639,638,637,636,635,634,633,632,629,628,627,626,625,624,
  623,503,501,500,499,498,497,496,495,494,493,492,491,490,487,484,483,422,
  398,226,132,131,0,2,6,7,8,9,15,17,21,23,24,25,26,27,28,29,57,83,86,130,
  191,193,198,199,200,201,202,203,204,205,206,207,208,209,210,211,212,213,
  214,410,457,504,
645,643,642,641,640,639,638,637,636,635,634,633,632,629,628,627,626,625,624,
  623,503,501,500,499,498,497,496,495,494,493,492,491,490,487,484,483,422,
  398,226,132,131,0,2,6,7,8,9,15,17,21,23,24,25,26,27,28,29,57,83,86,130,
  191,193,198,199,200,201,202,203,204,205,206,207,208,209,210,211,212,213,
  214,410,457,504,
645,643,642,641,640,639,638,637,636,635,634,633,632,629,628,627,626,625,624,
  623,503,501,500,499,498,497,496,495,494,493,492,491,490,487,484,483,422,
  398,226,132,131,0,2,6,7,8,9,15,17,21,23,24,25,26,27,28,29,57,83,86,130,
  191,193,198,199,200,201,202,203,204,205,206,207,208,209,210,211,212,213,
  214,410,457,504,
645,643,642,641,640,639,638,637,636,635,634,633,632,629,628,627,626,625,624,
  623,503,501,500,499,498,497,496,495,494,493,492,491,490,487,484,483,422,
  398,226,132,131,0,2,6,7,8,9,15,17,21,23,24,25,26,27,28,29,57,83,86,130,
  191,193,198,199,200,201,202,203,204,205,206,207,208,209,210,211,212,213,
  214,410,457,504,
420,1,0,51,52,
645,643,642,641,640,639,638,637,636,635,634,633,632,629,628,627,626,625,624,
  623,503,501,500,499,498,497,496,495,494,493,487,484,483,422,398,226,132,
  131,0,2,6,7,8,9,15,17,21,83,86,130,193,201,202,203,204,205,206,207,208,
  209,210,211,212,213,214,410,457,504,
645,643,642,641,640,639,638,637,636,635,634,633,632,629,628,627,626,625,624,
  623,503,501,500,499,498,497,496,495,494,493,487,484,483,422,398,226,132,
  131,0,2,6,7,8,9,15,17,21,83,86,130,193,201,202,203,204,205,206,207,208,
  209,210,211,212,213,214,410,457,504,
645,643,642,641,640,639,638,637,636,635,634,633,632,629,628,627,626,625,624,
  623,503,501,500,499,498,497,496,495,494,493,487,484,483,422,398,226,132,
  131,0,2,6,7,8,9,15,17,21,83,86,130,193,201,202,203,204,205,206,207,208,
  209,210,211,212,213,214,410,457,504,
645,643,642,641,640,639,638,637,636,635,634,633,632,629,628,627,626,625,624,
  623,503,501,500,499,498,497,496,495,494,493,487,484,483,422,398,226,132,
  131,0,2,6,7,8,9,15,17,21,83,86,130,193,201,202,203,204,205,206,207,208,
  209,210,211,212,213,214,410,457,504,
645,643,642,641,640,639,638,637,636,635,634,633,632,629,628,627,626,625,624,
  623,503,501,500,499,498,497,496,495,494,493,487,484,483,422,398,226,132,
  131,0,2,6,7,8,9,15,17,21,83,86,130,193,201,202,203,204,205,206,207,208,
  209,210,211,212,213,214,410,457,504,
645,643,642,641,640,639,638,637,636,635,634,633,632,629,628,627,626,625,624,
  623,503,501,500,499,498,497,496,495,494,493,492,491,490,487,484,483,422,
  398,226,132,131,0,2,6,7,8,9,15,17,21,25,29,83,86,130,191,193,198,199,
  200,201,202,203,204,205,206,207,208,209,210,211,212,213,214,410,457,504,
645,643,642,641,640,639,638,637,636,635,634,633,632,629,628,627,626,625,624,
  623,503,501,500,499,498,497,496,495,494,493,492,491,490,487,484,483,422,
  398,226,132,131,0,2,6,7,8,9,15,17,21,25,29,83,86,130,191,193,198,199,
  200,201,202,203,204,205,206,207,208,209,210,211,212,213,214,410,457,504,
645,643,642,641,640,639,638,637,636,635,634,633,632,629,628,627,626,625,624,
  623,503,501,500,499,498,497,496,495,494,493,492,491,490,487,484,483,422,
  398,226,132,131,0,2,6,7,8,9,15,17,21,25,27,29,83,86,130,191,193,198,199,
  200,201,202,203,204,205,206,207,208,209,210,211,212,213,214,410,457,504,
645,643,642,641,640,639,638,637,636,635,634,633,632,629,628,627,626,625,624,
  623,503,501,500,499,498,497,496,495,494,493,492,491,490,487,484,483,422,
  398,226,132,131,0,2,6,7,8,9,15,17,21,25,27,29,83,86,130,191,193,198,199,
  200,201,202,203,204,205,206,207,208,209,210,211,212,213,214,410,457,504,
645,643,642,641,640,639,638,637,636,635,634,633,632,629,628,627,626,625,624,
  623,503,501,500,499,498,497,496,495,494,493,492,491,490,487,484,483,422,
  398,226,132,131,0,2,6,7,8,9,15,17,21,25,27,29,83,86,130,191,193,198,199,
  200,201,202,203,204,205,206,207,208,209,210,211,212,213,214,410,457,504,
645,643,642,641,640,639,638,637,636,635,634,633,632,629,628,627,626,625,624,
  623,503,501,500,499,498,497,496,495,494,493,492,491,490,487,484,483,422,
  398,226,132,131,0,2,6,7,8,9,15,17,21,25,27,29,83,86,130,191,193,198,199,
  200,201,202,203,204,205,206,207,208,209,210,211,212,213,214,410,457,504,
645,643,642,641,640,639,638,637,636,635,634,633,632,629,628,627,626,625,624,
  623,503,501,500,499,498,497,496,495,494,493,492,491,490,487,484,483,422,
  398,226,132,131,0,2,6,7,8,9,15,17,21,24,25,27,29,83,86,130,191,193,198,
  199,200,201,202,203,204,205,206,207,208,209,210,211,212,213,214,410,457,
  504,
645,643,642,641,640,639,638,637,636,635,634,633,632,629,628,627,626,625,624,
  623,503,501,500,499,498,497,496,495,494,493,492,491,490,487,484,483,422,
  398,226,132,131,0,2,6,7,8,9,15,17,21,24,25,27,29,83,86,130,191,193,198,
  199,200,201,202,203,204,205,206,207,208,209,210,211,212,213,214,410,457,
  504,
645,643,642,641,640,639,638,637,636,635,634,633,632,629,628,627,626,625,624,
  623,503,501,500,499,498,497,496,495,494,493,492,491,490,487,484,483,422,
  398,226,132,131,0,2,6,7,8,9,15,17,21,23,24,25,27,29,83,86,130,191,193,
  198,199,200,201,202,203,204,205,206,207,208,209,210,211,212,213,214,410,
  457,504,
645,643,642,641,640,639,638,637,636,635,634,633,632,629,628,627,626,625,624,
  623,503,501,500,499,498,497,496,495,494,493,492,491,490,487,484,483,422,
  398,226,132,131,0,2,6,7,8,9,15,17,21,23,24,25,27,29,83,86,130,191,193,
  198,199,200,201,202,203,204,205,206,207,208,209,210,211,212,213,214,410,
  457,504,
645,643,642,641,640,639,638,637,636,635,634,633,632,629,628,627,626,625,624,
  623,503,501,500,499,498,497,496,495,494,493,492,491,490,487,484,483,422,
  398,226,132,131,0,2,6,7,8,9,15,17,21,23,24,25,27,28,29,83,86,130,191,
  193,198,199,200,201,202,203,204,205,206,207,208,209,210,211,212,213,214,
  410,457,504,
645,643,642,641,640,639,638,637,636,635,634,633,632,629,628,627,626,625,624,
  623,503,501,500,499,498,497,496,495,494,493,492,491,490,487,484,483,422,
  398,226,132,131,0,2,6,7,8,9,15,17,21,23,24,25,27,28,29,83,86,130,191,
  193,198,199,200,201,202,203,204,205,206,207,208,209,210,211,212,213,214,
  410,457,504,
396,394,393,392,0,39,40,41,45,46,47,49,
396,394,393,392,0,39,40,41,45,46,47,49,
396,394,393,392,0,39,40,41,45,46,47,49,
396,394,393,392,1,0,51,52,
396,394,393,392,1,0,51,52,
396,394,393,392,0,39,40,41,45,46,47,49,
645,643,642,641,640,639,638,637,636,635,634,633,632,629,628,627,626,625,624,
  623,503,501,500,499,498,497,496,495,494,493,492,491,490,487,484,483,422,
  398,226,132,131,0,2,6,7,8,9,15,17,21,23,24,25,26,27,28,29,57,83,86,130,
  191,193,198,199,200,201,202,203,204,205,206,207,208,209,210,211,212,213,
  214,410,457,504,
645,643,642,641,640,639,638,637,636,635,634,633,632,629,628,627,626,625,624,
  623,503,501,500,499,498,497,496,495,494,493,492,491,490,487,484,483,422,
  398,226,132,131,0,2,6,7,8,9,15,17,21,23,24,25,26,27,28,29,57,83,86,130,
  191,193,198,199,200,201,202,203,204,205,206,207,208,209,210,211,212,213,
  214,410,457,504,
458,0,129,
645,644,643,642,641,640,639,638,637,636,635,634,633,632,631,630,629,628,627,
  626,625,624,623,622,621,620,492,490,487,486,485,484,483,477,475,473,445,
  422,420,398,394,392,228,1,0,16,
645,643,642,641,640,639,638,637,636,635,634,633,632,629,628,627,626,625,624,
  623,621,506,503,501,500,499,498,497,496,495,494,493,492,491,490,487,484,
  483,422,398,236,235,234,233,232,231,230,229,226,132,131,1,0,51,52,
645,643,642,641,640,639,638,637,636,635,634,633,632,629,628,627,626,625,624,
  623,621,503,501,500,499,498,497,496,495,494,493,492,491,490,487,484,483,
  422,398,226,132,131,1,0,51,52,
507,0,258,
507,396,394,393,392,1,0,51,52,
645,644,643,642,641,640,639,638,637,636,635,634,633,632,631,630,629,628,627,
  626,625,623,622,621,620,492,490,487,486,485,484,483,477,475,473,445,422,
  420,398,394,392,228,1,0,16,
645,643,642,641,640,639,638,637,636,635,634,633,632,629,628,627,626,625,624,
  623,503,501,500,499,498,497,496,495,494,493,492,491,490,487,484,483,422,
  398,226,132,131,1,0,51,52,
458,396,394,393,392,1,0,129,
645,643,642,641,640,639,638,637,636,635,634,632,623,132,131,1,0,51,52,
646,645,643,642,641,640,639,638,637,636,635,634,633,632,631,630,629,628,627,
  626,625,624,623,621,620,492,490,487,486,485,484,483,477,475,473,445,423,
  422,420,398,396,394,393,392,1,0,51,52,
646,645,643,642,641,640,639,638,637,636,635,634,633,632,631,630,629,628,627,
  626,625,624,623,621,620,492,490,487,486,485,484,483,477,475,473,445,423,
  422,420,398,396,394,393,392,1,0,
396,394,393,392,0,39,40,41,45,46,47,49,
645,644,643,642,641,640,639,638,637,636,635,634,633,632,631,630,629,628,627,
  626,625,624,623,622,621,620,492,490,487,486,485,484,483,477,475,473,445,
  422,420,398,394,392,1,0,
629,628,627,626,625,0,
645,643,642,641,640,639,638,637,636,635,634,632,623,132,131,0,3,130,410,502,
420,0,85,
420,0,85,
420,0,85,
420,0,85,
420,0,85,
420,0,85,
420,0,85,
420,0,85,
420,0,85,
420,0,85,
507,489,488,487,486,485,484,483,482,481,480,479,478,477,476,475,474,473,472,
  471,470,469,468,467,466,465,464,463,462,461,460,458,420,394,392,1,0,192,
  194,195,196,197,
507,489,488,487,486,485,484,483,482,481,480,479,478,477,476,475,474,473,472,
  471,470,469,468,467,466,465,464,463,462,461,460,458,420,394,392,1,0,192,
  194,195,196,197,
507,484,483,482,481,480,479,478,477,476,475,474,473,472,471,470,469,468,467,
  466,465,464,463,462,461,460,458,420,396,394,393,392,1,0,190,191,
507,484,483,482,481,480,479,478,477,476,475,474,473,472,471,470,469,468,467,
  466,465,464,463,462,461,460,458,420,396,394,393,392,1,0,190,191,
507,484,483,482,481,480,479,478,477,476,475,474,473,472,471,470,469,468,467,
  466,465,464,463,462,461,460,458,420,396,394,393,392,1,0,190,191,
507,484,483,482,481,480,479,478,477,476,475,474,473,472,471,470,469,468,467,
  466,465,464,463,462,461,460,458,420,396,394,393,392,1,0,190,191,
507,482,481,480,479,478,477,476,475,474,473,472,471,470,469,468,467,466,465,
  464,463,462,461,460,458,420,396,394,393,392,1,0,186,187,188,189,
507,482,481,480,479,478,477,476,475,474,473,472,471,470,469,468,467,466,465,
  464,463,462,461,460,458,420,396,394,393,392,1,0,186,187,188,189,
507,478,477,476,475,474,473,472,471,470,469,468,467,466,465,464,463,462,461,
  460,458,420,396,394,393,392,1,0,184,185,
507,478,477,476,475,474,473,472,471,470,469,468,467,466,465,464,463,462,461,
  460,458,420,396,394,393,392,1,0,184,185,
507,476,475,474,473,472,471,470,469,468,467,466,465,464,463,462,461,460,458,
  420,396,394,393,392,1,0,182,183,
507,476,475,474,473,472,471,470,469,468,467,466,465,464,463,462,461,460,458,
  420,396,394,393,392,1,0,182,183,
396,394,393,392,0,39,40,41,45,46,47,49,
396,394,393,392,0,39,40,41,45,46,47,49,
506,236,235,234,233,232,231,230,229,1,0,51,52,
645,644,643,642,641,640,639,638,637,636,635,634,633,632,631,630,629,628,627,
  626,625,624,623,622,621,620,492,490,487,486,485,484,483,477,475,473,445,
  422,420,398,394,392,228,1,0,
645,643,642,641,640,639,638,637,636,635,634,633,632,629,628,627,626,625,624,
  623,621,503,501,500,499,498,497,496,495,494,493,492,491,490,487,484,483,
  422,398,226,132,131,0,2,6,7,8,9,10,12,15,17,18,20,21,23,24,25,26,27,28,
  29,57,83,86,130,191,193,198,199,200,201,202,203,204,205,206,207,208,209,
  210,211,212,213,214,403,410,450,457,504,
645,643,642,641,640,639,638,637,636,635,634,633,632,629,628,627,626,625,624,
  623,503,501,500,499,498,497,496,495,494,493,492,491,490,487,484,483,422,
  398,226,132,131,0,2,6,7,8,9,15,17,21,23,24,25,26,27,28,29,57,83,86,130,
  191,193,198,199,200,201,202,203,204,205,206,207,208,209,210,211,212,213,
  214,410,457,504,
645,643,642,641,640,639,638,637,636,635,634,632,623,132,131,0,21,130,410,
645,643,642,641,640,639,638,637,636,635,634,632,629,628,627,626,625,623,420,
  398,1,0,51,52,135,
420,1,0,51,52,
420,1,0,51,52,
506,236,235,234,233,232,231,230,229,0,4,237,554,
420,0,85,

};


static unsigned const char ag_astt[23459] = {
  8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,7,1,1,9,5,2,2,2,2,2,2,2,2,
  2,2,2,2,2,1,8,8,8,8,8,2,2,1,1,7,1,0,1,1,1,1,1,1,1,1,1,1,1,5,5,5,5,5,5,5,5,
  5,5,5,1,7,1,3,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,4,2,4,4,4,4,2,4,4,7,2,1,
  1,1,1,1,1,1,1,1,1,1,7,1,3,1,1,1,1,1,1,1,1,1,1,5,5,5,5,5,5,5,5,5,5,5,5,5,5,
  5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,
  5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,
  5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,
  5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,
  5,5,5,5,5,5,1,7,1,3,1,7,2,8,8,1,1,1,1,1,7,3,3,1,3,1,1,1,1,1,8,8,8,8,8,8,8,
  8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,
  8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,
  8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,
  8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,
  8,8,8,8,5,5,8,8,8,8,8,8,8,1,7,1,1,2,2,2,2,2,2,2,2,2,2,2,2,2,1,1,1,1,1,1,1,
  1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,
  1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,
  1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,8,8,8,8,8,8,8,8,
  8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,1,1,1,8,8,5,5,1,5,5,5,5,2,2,
  9,7,1,1,1,1,1,1,1,2,1,2,2,1,1,1,1,1,1,1,1,1,1,1,1,2,2,2,2,2,2,2,2,2,2,2,2,
  2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,1,1,
  1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,
  1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,2,
  2,2,2,2,2,2,2,2,2,2,2,2,1,8,8,8,8,8,2,2,1,3,1,7,1,3,3,1,1,3,1,1,1,1,1,5,5,
  5,5,5,5,5,5,5,5,5,5,5,5,5,1,7,1,3,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,1,7,1,3,5,
  1,7,1,3,5,5,5,5,5,7,1,3,5,5,5,5,5,7,1,3,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,1,7,
  1,3,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,
  5,5,5,5,5,5,1,7,1,3,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,
  5,5,5,5,5,5,5,5,5,5,5,5,5,5,1,7,1,3,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,1,7,1,3,
  5,5,1,7,1,3,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,1,7,1,3,2,2,2,2,2,2,
  2,2,2,2,2,2,2,2,2,7,1,1,1,1,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,7,1,1,1,2,7,1,1,
  1,8,8,8,8,1,7,1,1,8,8,8,8,1,7,1,1,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,7,1,1,1,2,
  2,2,2,2,2,2,2,2,2,2,1,2,2,2,2,2,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,
  2,2,2,7,2,1,1,2,1,2,1,1,1,1,1,1,1,1,1,1,1,1,2,1,1,1,1,1,1,1,2,1,1,1,1,1,1,
  1,1,1,1,1,1,1,1,1,1,2,2,2,2,2,2,2,2,2,2,2,1,2,2,2,2,2,1,1,1,1,1,1,1,1,1,1,
  1,1,1,1,1,1,1,1,1,1,1,2,2,2,7,2,1,1,2,1,2,1,1,1,1,1,1,1,1,1,1,1,1,2,1,1,1,
  1,1,1,1,2,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,7,
  1,1,1,1,1,7,1,1,2,2,7,1,1,1,1,1,1,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,
  5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,7,1,3,5,
  5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,
  5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,
  5,5,5,5,5,5,5,5,5,5,7,1,3,1,1,7,1,1,1,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,
  5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,7,1,3,5,5,5,5,5,5,
  5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,
  5,5,5,5,5,7,1,3,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,
  5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,7,1,3,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,
  1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,9,7,3,3,3,1,3,1,3,3,3,
  3,3,3,3,3,3,3,3,3,3,3,3,3,3,3,3,3,3,3,3,3,3,3,3,3,3,3,3,3,3,3,3,3,3,3,3,3,
  3,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,
  1,1,1,1,1,1,8,1,7,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,
  1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,
  1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,8,1,7,1,1,1,1,1,1,1,1,1,1,1,1,1,
  1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,2,2,2,2,2,2,
  2,2,2,2,2,2,2,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,
  1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,
  1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,
  1,1,1,1,1,1,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,
  1,1,1,8,8,8,8,1,1,1,1,1,2,2,7,1,1,1,1,1,3,3,1,3,1,1,1,1,1,2,1,2,2,1,1,1,1,
  1,1,1,1,1,1,1,1,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,
  2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,
  1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,
  1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,8,8,8,8,8,8,1,7,1,1,5,5,5,5,5,5,7,1,
  3,5,5,5,5,5,5,7,1,3,5,5,5,5,5,5,7,1,3,5,5,5,5,5,5,7,1,3,5,5,5,5,5,5,7,1,3,
  5,5,5,5,5,5,7,1,3,5,5,5,5,5,5,7,1,3,5,5,5,5,5,5,7,1,3,5,5,5,5,5,5,7,1,3,5,
  5,5,5,5,5,7,1,3,5,5,5,5,5,5,7,1,3,5,5,5,5,5,5,7,1,3,5,5,5,5,5,5,7,1,3,5,5,
  5,5,5,5,7,1,3,5,5,5,5,5,5,5,5,5,5,7,1,3,5,5,5,5,5,5,5,5,7,1,3,5,5,5,5,5,5,
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
  7,1,3,5,5,5,5,5,5,7,1,3,5,5,5,5,5,5,7,1,3,5,5,5,5,5,5,7,1,3,5,5,5,5,5,5,7,
  1,3,5,5,5,5,5,5,7,1,3,5,5,5,5,5,5,7,1,3,5,5,5,5,5,5,7,1,3,5,5,5,5,5,5,7,1,
  3,5,5,5,5,5,5,7,1,3,5,5,5,5,5,5,7,1,3,5,5,5,5,5,5,7,1,3,5,5,5,5,5,5,7,1,3,
  5,5,5,5,5,5,7,1,3,5,5,5,5,5,5,7,1,3,5,5,5,5,5,5,7,1,3,5,5,5,5,5,5,7,1,3,5,
  5,5,5,5,5,7,1,3,5,5,5,5,5,5,7,1,3,5,5,5,5,5,5,7,1,3,5,5,5,5,5,5,7,1,3,5,5,
  5,5,5,5,7,1,3,5,5,5,5,5,5,7,1,3,5,5,5,5,5,5,7,1,3,5,5,5,5,5,5,7,1,3,5,5,5,
  5,5,5,7,1,3,5,5,5,5,5,5,7,1,3,5,5,5,5,5,5,7,1,3,5,5,5,5,5,5,7,1,3,5,5,5,5,
  5,7,1,3,8,8,8,8,8,8,8,8,8,1,7,1,1,8,8,8,8,8,8,8,1,7,1,1,5,5,5,5,5,5,5,5,5,
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
  5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,7,1,3,5,5,5,5,5,5,5,5,5,7,1,3,
  5,5,5,5,5,5,5,5,5,7,1,3,5,5,5,5,5,7,1,3,5,5,5,5,5,7,1,3,5,5,5,5,5,5,5,5,7,
  1,3,5,5,5,5,5,5,5,5,7,1,3,5,5,5,5,5,5,5,5,7,1,3,5,5,5,5,5,5,5,5,5,5,7,1,3,
  5,5,5,5,5,5,5,5,5,5,7,1,3,5,5,5,5,5,5,5,5,5,5,7,1,3,5,5,5,5,5,5,5,5,5,5,7,
  1,3,5,5,5,5,5,5,5,5,5,5,7,1,3,5,5,5,5,5,5,5,5,5,5,7,1,3,5,5,5,5,5,5,5,5,5,
  5,7,1,3,5,5,5,5,5,5,5,5,5,5,7,1,3,5,5,5,5,5,5,5,5,5,5,7,1,3,5,5,5,5,5,5,5,
  5,5,5,7,1,3,5,5,5,5,5,5,5,5,5,5,7,1,3,8,8,8,8,1,7,1,1,8,1,7,1,1,8,1,7,1,1,
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
  8,8,8,8,8,8,1,7,1,1,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,
  8,8,8,8,8,8,8,8,8,8,8,8,8,8,1,7,1,1,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,
  8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,1,7,1,1,8,8,8,8,8,8,8,8,8,8,8,
  8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,1,7,1,1,8,8,8,
  8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,
  8,1,7,1,1,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,
  8,8,8,8,8,8,8,8,8,1,7,1,1,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,
  8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,1,7,1,1,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,
  8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,1,7,1,1,8,8,8,8,8,8,8,8,
  1,7,1,1,8,8,8,8,8,8,8,8,1,7,1,1,8,8,8,8,1,7,1,1,8,8,8,8,1,7,1,1,8,8,8,8,8,
  8,8,1,7,1,1,8,8,8,8,8,8,8,1,7,1,1,8,8,8,8,8,8,8,1,7,1,1,8,8,8,8,8,8,8,8,8,
  1,7,1,1,8,8,8,8,8,8,8,8,8,1,7,1,1,8,8,8,8,8,8,8,8,8,1,7,1,1,8,8,8,8,8,8,8,
  8,8,1,7,1,1,8,8,8,8,8,8,8,8,8,1,7,1,1,8,8,8,8,8,8,8,8,8,1,7,1,1,8,8,8,8,8,
  8,8,8,8,1,7,1,1,8,8,8,8,8,8,8,8,8,1,7,1,1,8,8,8,8,8,8,8,8,8,1,7,1,1,8,8,8,
  8,8,8,8,8,8,1,7,1,1,8,8,8,8,8,8,8,8,8,1,7,1,1,5,2,2,2,2,2,2,2,2,2,2,2,5,2,
  5,5,2,2,2,2,2,5,2,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,2,
  5,5,5,5,1,7,1,3,2,5,5,5,5,5,1,7,1,3,4,4,4,4,4,4,4,4,4,4,4,4,4,4,4,4,4,4,4,
  4,4,4,4,4,4,4,4,4,4,4,4,4,1,4,4,4,4,4,4,7,1,4,4,4,4,1,7,1,1,5,1,2,2,2,2,2,
  2,2,2,2,2,2,1,2,2,2,2,2,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,2,2,2,7,
  2,1,1,2,1,2,1,1,1,1,1,1,1,1,1,1,2,1,2,1,1,1,1,1,1,1,2,1,1,1,1,1,1,1,1,1,1,
  1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,
  1,1,7,2,2,2,2,1,1,1,1,2,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,2,2,1,2,2,
  1,1,2,8,8,8,8,1,7,1,1,4,4,4,4,4,4,4,4,4,4,4,4,4,4,4,4,4,4,4,4,4,4,4,4,4,4,
  4,4,4,4,4,4,4,4,4,4,1,4,4,4,4,7,1,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,
  8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,1,7,1,1,8,8,8,8,1,7,1,1,2,1,2,2,2,
  2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,3,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,
  2,7,2,5,5,5,5,5,5,5,5,7,1,3,8,8,8,8,1,7,1,1,1,1,1,1,7,2,2,1,2,1,1,1,1,1,1,
  1,7,2,2,1,2,1,1,1,8,8,8,8,1,7,1,1,2,2,2,2,2,7,2,2,2,2,7,2,2,7,4,4,4,4,4,4,
  4,2,2,2,2,4,2,2,2,2,2,4,4,4,4,4,4,4,4,4,4,4,4,4,4,4,4,4,4,4,4,4,4,4,4,4,4,
  4,4,4,4,4,4,4,4,4,4,4,4,4,4,4,7,4,1,4,4,4,4,4,4,4,4,4,4,4,4,4,4,4,4,4,4,4,
  4,4,4,4,4,4,4,4,4,4,4,4,4,4,4,4,4,4,4,4,4,4,4,4,4,4,4,4,4,4,4,4,4,7,2,2,2,
  2,2,7,2,1,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,
  2,2,2,2,2,2,2,2,2,7,1,10,10,10,10,10,5,2,10,10,10,10,10,10,10,10,10,7,10,
  10,10,10,4,4,4,4,4,4,4,4,4,4,4,4,4,4,4,4,4,4,4,4,4,4,4,4,4,4,4,4,4,4,4,4,4,
  4,4,4,4,4,7,10,10,4,4,4,4,4,4,4,4,4,4,4,4,4,4,4,4,4,4,4,4,4,4,4,4,4,4,4,4,
  4,4,4,4,4,4,4,4,4,4,7,10,10,10,10,10,10,10,10,10,4,4,4,4,4,4,4,4,4,4,4,4,4,
  4,4,4,4,4,4,4,4,4,4,4,4,4,4,4,4,4,4,4,4,4,4,4,4,4,7,9,2,2,1,1,2,10,10,10,
  10,10,9,4,4,4,4,4,4,4,4,4,4,4,4,4,4,4,4,4,4,4,4,4,4,4,4,4,4,4,4,4,4,4,2,4,
  4,4,4,2,2,4,7,2,5,5,7,1,3,5,5,7,1,3,5,5,7,1,3,5,5,7,1,3,5,5,7,1,3,5,5,7,1,
  3,5,5,7,1,3,5,5,7,1,3,5,5,7,1,3,5,5,7,1,3,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,
  5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,7,1,3,5,5,5,5,5,5,5,5,5,5,5,5,
  5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,1,7,1,3,5,5,5,5,5,5,5,5,
  5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,
  5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,7,1,3,8,1,7,1,1,8,1,7,1,1,8,1,7,1,1,8,
  1,7,1,1,8,1,7,1,1,8,1,7,1,1,8,1,7,1,1,8,1,7,1,1,8,1,7,1,1,8,1,7,1,1,5,5,5,
  5,5,5,5,5,5,5,5,5,5,2,2,2,2,2,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,1,7,
  1,3,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,
  5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,7,1,3,5,5,5,5,5,5,5,5,
  5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,1,7,1,3,5,5,5,
  5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,
  5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,7,1,3,8,8,8,8,8,8,8,8,8,8,8,8,8,
  8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,1,7,1,1,2,2,2,2,2,
  2,2,2,2,2,2,1,2,2,2,2,2,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,2,2,2,7,2,1,1,
  2,1,2,1,1,1,2,1,2,2,2,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,2,2,2,2,2,2,2,2,2,2,2,
  1,2,2,2,2,2,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,2,2,2,7,2,1,1,2,1,2,1,1,1,
  2,1,2,2,2,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,2,2,2,2,2,2,2,2,2,2,2,1,2,2,2,2,2,
  1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,2,2,2,7,2,1,1,2,1,2,1,1,1,2,1,2,2,2,1,
  1,1,1,1,1,1,1,1,1,1,1,1,1,1,2,2,2,2,2,2,2,2,2,2,2,1,2,2,2,2,2,1,1,1,1,1,1,
  1,1,1,1,1,1,1,1,1,1,1,1,2,2,2,7,2,1,1,2,1,2,1,1,1,2,1,2,2,2,1,1,1,1,1,1,1,
  1,1,1,1,1,1,1,1,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,
  5,5,5,5,5,5,5,5,1,7,1,3,5,1,1,1,1,1,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,
  5,5,5,5,5,5,5,5,5,5,1,7,1,3,1,1,1,1,1,5,1,1,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,
  5,5,5,5,5,5,5,5,5,5,5,5,5,5,1,7,1,3,1,1,5,1,1,1,1,5,5,5,5,5,5,5,5,5,5,5,5,
  5,5,5,5,5,5,5,5,5,5,5,5,5,1,7,1,3,1,1,1,1,5,1,1,5,5,5,5,5,5,5,5,5,5,5,5,5,
  5,5,5,5,5,5,5,5,5,5,1,7,1,3,1,1,5,1,1,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,
  5,5,5,1,7,1,3,1,1,1,1,5,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,4,4,4,4,4,7,2,2,2,2,
  2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,1,2,2,2,2,2,1,1,1,1,1,
  1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,2,2,2,7,2,1,1,2,1,2,1,1,1,1,1,1,1,1,1,2,1,
  2,1,1,1,1,1,1,2,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,8,8,8,8,1,7,1,1,8,8,8,8,1,
  7,1,1,8,8,8,8,1,7,1,5,5,5,5,5,7,1,3,8,8,8,8,1,7,1,1,5,5,5,5,5,7,1,3,8,8,8,
  8,1,7,1,1,10,10,1,10,10,10,10,10,10,10,10,10,10,10,10,2,10,10,10,10,10,10,
  10,10,10,10,10,10,10,10,10,10,10,10,10,10,10,10,10,10,10,10,7,5,5,5,5,5,7,
  1,3,8,8,8,8,1,7,1,1,8,8,8,8,1,7,1,1,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,
  5,5,1,7,1,3,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,7,1,1,1,1,1,1,1,1,5,
  5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,
  5,5,5,5,5,5,5,5,5,5,7,1,3,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,
  5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,
  5,7,1,3,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,
  5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,7,1,3,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,
  5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,
  5,5,5,5,5,5,7,1,3,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,
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
  5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,7,1,3,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,
  5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,
  5,5,5,5,5,5,7,1,3,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,
  5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,7,1,3,
  5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,
  5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,7,1,3,5,5,5,5,5,5,5,5,5,
  5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,
  5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,7,1,3,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,
  5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,
  5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,7,1,3,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,
  5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,7,1,3,5,5,5,5,
  5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,
  5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,7,1,3,5,5,5,5,5,5,5,5,5,5,5,5,5,
  5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,
  5,5,5,5,5,5,5,5,5,5,5,7,1,3,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,
  5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,7,1,3,5,5,5,5,5,5,5,5,5,
  5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,
  7,1,3,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,
  5,5,5,5,5,5,5,5,5,5,5,5,5,5,7,1,3,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,
  5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,7,1,3,5,5,5,5,5,5,
  5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,
  5,5,5,5,5,5,5,5,5,5,7,1,3,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,
  1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,9,5,3,3,3,3,3,3,3,3,3,3,3,3,3,3,3,3,3,
  3,3,3,3,3,3,3,3,3,3,3,3,3,3,3,3,3,3,3,3,3,3,3,3,3,1,7,3,1,7,3,1,1,1,1,1,1,
  1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,7,1,1,2,2,2,2,1,
  1,1,1,2,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,2,2,1,2,2,1,1,2,8,8,8,8,1,
  7,1,1,1,1,1,1,1,1,7,1,3,3,1,3,1,1,1,2,2,2,2,2,2,2,2,2,2,2,7,3,2,1,2,2,2,2,
  2,2,2,7,3,2,2,2,1,2,2,2,2,7,2,1,7,1,1,7,1,2,2,2,2,2,2,2,2,2,2,2,1,2,2,2,2,
  2,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,2,2,2,7,2,1,1,2,1,2,1,1,1,1,1,
  1,1,1,1,2,1,2,1,1,1,1,1,1,2,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,2,2,2,2,2,2,2,
  2,2,2,2,1,2,2,2,2,2,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,2,2,2,7,2,1,
  1,2,1,2,1,1,1,1,1,1,1,1,1,2,1,2,1,1,1,1,1,1,2,1,1,1,1,1,1,1,1,1,1,1,1,1,1,
  1,1,2,2,2,2,2,2,2,2,2,2,2,1,2,2,2,2,2,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,
  1,1,1,2,2,2,7,2,1,1,2,1,2,1,1,1,1,1,1,1,1,1,2,1,2,1,1,1,1,1,1,2,1,1,1,1,1,
  1,1,1,1,1,1,1,1,1,1,1,2,2,2,2,2,2,2,2,2,2,2,1,2,2,2,2,2,1,1,1,1,1,1,1,1,1,
  1,1,1,1,1,1,1,1,1,1,1,1,2,2,2,7,2,1,1,2,1,2,1,1,1,1,1,1,1,1,1,2,1,2,1,1,1,
  1,1,1,2,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,2,2,2,2,2,2,2,2,2,2,2,1,2,2,2,2,2,
  1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,2,2,2,7,2,1,1,2,1,2,1,1,1,1,1,1,
  1,1,1,2,1,2,1,1,1,1,1,1,2,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,2,2,2,2,2,2,2,2,
  2,2,2,1,2,2,2,2,2,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,2,2,2,7,2,1,1,
  2,1,2,1,1,1,1,1,1,1,1,1,2,1,2,1,1,1,1,1,1,2,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,
  1,2,2,2,2,2,2,2,2,2,2,2,1,2,2,2,2,2,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,
  1,1,2,2,2,7,2,1,1,2,1,2,1,1,1,1,1,1,1,1,1,2,1,2,1,1,1,1,1,1,2,1,1,1,1,1,1,
  1,1,1,1,1,1,1,1,1,1,2,2,2,2,2,2,2,2,2,2,2,1,2,2,2,2,2,1,1,1,1,1,1,1,1,1,1,
  1,1,1,1,1,1,1,1,1,1,1,2,2,2,7,2,1,1,2,1,2,1,1,1,1,1,1,1,1,1,2,1,2,1,1,1,1,
  1,1,2,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,2,2,2,2,2,2,2,2,2,2,2,1,2,2,2,2,2,1,
  1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,2,2,2,7,2,1,1,2,1,2,1,1,1,1,1,1,1,
  1,1,2,1,2,1,1,1,1,1,1,2,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,2,2,2,2,2,2,2,2,2,
  2,2,1,2,2,2,2,2,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,2,2,2,7,2,1,1,2,
  1,2,1,1,1,1,1,1,1,1,1,2,1,2,1,1,1,1,1,1,2,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,
  2,2,2,2,2,2,2,2,2,2,2,1,2,2,2,2,2,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,
  1,2,2,2,7,2,1,1,2,1,2,1,1,1,1,1,1,1,1,1,2,1,2,1,1,1,1,1,1,2,1,1,1,1,1,1,1,
  1,1,1,1,1,1,1,1,1,2,2,2,2,2,2,2,2,2,2,2,1,2,2,2,2,2,1,1,1,1,1,1,1,1,1,1,1,
  1,1,1,1,1,1,1,1,1,1,2,2,2,7,2,1,1,2,1,2,1,1,1,1,1,1,1,1,1,2,1,2,1,1,1,1,1,
  1,2,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,2,2,2,2,2,2,2,2,2,2,2,1,2,2,2,2,2,1,1,
  1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,2,2,2,7,2,1,1,2,1,2,1,1,1,1,1,1,1,1,
  1,2,1,2,1,1,1,1,1,1,2,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,2,2,2,2,2,2,2,2,2,2,
  2,1,2,2,2,2,2,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,2,2,2,7,2,1,1,2,1,
  2,1,1,1,1,1,1,1,1,1,2,1,2,1,1,1,1,1,1,2,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,2,
  2,2,2,2,2,2,2,2,2,2,1,2,2,2,2,2,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,
  2,2,2,7,2,1,1,2,1,2,1,1,1,1,1,1,1,1,1,2,1,2,1,1,1,1,1,1,2,1,1,1,1,1,1,1,1,
  1,1,1,1,1,1,1,1,2,2,2,2,2,2,2,2,2,2,2,1,2,2,2,2,2,1,1,1,1,1,1,1,1,1,1,1,1,
  1,1,1,1,1,1,1,1,1,2,2,2,7,2,1,1,2,1,2,1,1,1,1,1,1,1,1,1,2,1,2,1,1,1,1,1,1,
  2,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,2,2,2,2,2,2,2,2,2,2,2,1,2,2,2,2,2,1,1,1,
  1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,2,2,2,7,2,1,1,2,1,2,1,1,1,1,1,1,1,1,1,
  2,1,2,1,1,1,1,1,1,2,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,2,2,2,2,2,2,2,2,2,2,2,
  1,2,2,2,2,2,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,2,2,2,7,2,1,1,2,1,2,
  1,1,1,1,1,1,1,1,1,2,1,2,1,1,1,1,1,1,2,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,2,2,
  2,2,2,2,2,2,2,2,2,1,2,2,2,2,2,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,2,
  2,2,7,2,1,1,2,1,2,1,1,1,1,1,1,1,1,1,2,1,2,1,1,1,1,1,1,2,1,1,1,1,1,1,1,1,1,
  1,1,1,1,1,1,1,2,2,2,2,2,2,2,2,2,2,2,1,2,2,2,2,2,1,1,1,1,1,1,1,1,1,1,1,1,1,
  1,1,1,1,1,1,1,1,2,2,2,7,2,1,1,2,1,2,1,1,1,1,1,1,1,1,1,2,1,2,1,1,1,1,1,1,2,
  1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,2,2,2,2,2,2,2,2,2,2,2,1,2,2,2,2,2,1,1,1,1,
  1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,2,2,2,7,2,1,1,2,1,2,1,1,1,1,1,1,1,1,1,2,
  1,2,1,1,1,1,1,1,2,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,2,2,2,2,2,2,2,2,2,2,2,1,
  2,2,2,2,2,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,2,2,2,7,2,1,1,2,1,2,1,
  1,1,1,1,1,1,1,1,2,1,2,1,1,1,1,1,1,2,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,2,2,2,
  2,2,2,2,2,2,2,2,1,2,2,2,2,2,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,2,2,
  2,7,2,1,1,2,1,2,1,1,1,1,1,1,1,1,1,2,1,2,1,1,1,1,1,1,2,1,1,1,1,1,1,1,1,1,1,
  1,1,1,1,1,1,2,2,2,2,2,2,2,2,2,2,2,1,2,2,2,2,2,1,1,1,1,1,1,1,1,1,1,1,1,1,1,
  1,1,1,1,1,1,1,2,2,2,7,2,1,1,2,1,2,1,1,1,1,1,1,1,1,1,2,1,2,1,1,1,1,1,1,2,1,
  1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,2,2,2,2,2,2,2,2,2,2,2,1,2,2,2,2,2,1,1,1,1,1,
  1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,2,2,2,7,2,1,1,2,1,2,1,1,1,1,1,1,1,1,1,2,1,
  2,1,1,1,1,1,1,2,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,2,2,2,2,2,2,2,2,2,2,2,1,2,
  2,2,2,2,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,2,2,2,7,2,1,1,2,1,2,1,1,
  1,1,1,1,1,1,1,2,1,2,1,1,1,1,1,1,2,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,2,2,2,2,
  2,2,2,2,2,2,2,1,2,2,2,2,2,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,2,2,2,
  7,2,1,1,2,1,2,1,1,1,1,1,1,1,1,1,2,1,2,1,1,1,1,1,1,2,1,1,1,1,1,1,1,1,1,1,1,
  1,1,1,1,1,2,2,2,2,2,2,2,2,2,2,2,1,2,2,2,2,2,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,
  1,1,1,1,1,1,2,2,2,7,2,1,1,2,1,2,1,1,1,1,1,1,1,1,1,2,1,2,1,1,1,1,1,1,2,1,1,
  1,1,1,1,1,1,1,1,1,1,1,1,1,1,2,2,2,2,2,2,2,2,2,2,2,1,2,2,2,2,2,1,1,1,1,1,1,
  1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,2,2,2,7,2,1,1,2,1,2,1,1,1,1,1,1,1,1,1,2,1,2,
  1,1,1,1,1,1,2,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,2,2,2,2,2,2,2,2,2,2,2,1,2,2,
  2,2,2,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,2,2,2,7,2,1,1,2,1,2,1,1,1,
  1,1,1,1,1,1,2,1,2,1,1,1,1,1,1,2,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,2,2,2,2,2,
  2,2,2,2,2,2,1,2,2,2,2,2,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,2,2,2,7,
  2,1,1,2,1,2,1,1,1,1,1,1,1,1,1,2,1,2,1,1,1,1,1,1,2,1,1,1,1,1,1,1,1,1,1,1,1,
  1,1,1,1,2,2,2,2,2,2,2,2,2,2,2,1,2,2,2,2,2,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,
  1,1,1,1,1,2,2,2,7,2,1,1,2,1,2,1,1,1,1,1,1,1,1,1,2,1,2,1,1,1,1,1,1,2,1,1,1,
  1,1,1,1,1,1,1,1,1,1,1,1,1,2,2,2,2,2,2,2,2,2,2,2,1,2,2,2,2,2,1,1,1,1,1,1,1,
  1,1,1,1,1,1,1,1,1,1,1,1,1,1,2,2,2,7,2,1,1,2,1,2,1,1,1,1,1,1,1,1,1,2,1,2,1,
  1,1,1,1,1,2,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,2,2,2,2,2,2,2,2,2,2,2,1,2,2,2,
  2,2,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,2,2,2,7,2,1,1,2,1,2,1,1,1,1,
  1,1,1,1,1,2,1,2,1,1,1,1,1,1,2,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,2,2,2,2,2,2,
  2,2,2,2,2,1,2,2,2,2,2,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,2,2,2,7,2,
  1,1,2,1,2,1,1,1,1,1,1,1,1,1,2,1,2,1,1,1,1,1,1,2,1,1,1,1,1,1,1,1,1,1,1,1,1,
  1,1,1,2,2,2,2,2,2,2,2,2,2,2,1,2,2,2,2,2,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,
  1,1,1,1,2,2,2,7,2,1,1,2,1,2,1,1,1,1,1,1,1,1,1,2,1,2,1,1,1,1,1,1,2,1,1,1,1,
  1,1,1,1,1,1,1,1,1,1,1,1,1,1,2,2,2,2,2,2,7,2,2,2,2,2,2,1,1,2,2,2,2,2,2,7,2,
  2,2,2,2,2,2,2,2,2,7,2,2,2,2,2,2,2,7,2,2,2,2,2,2,2,2,2,2,7,2,2,2,2,1,2,2,2,
  2,2,2,2,7,2,2,2,2,1,2,2,2,2,2,2,2,7,2,2,2,2,1,2,2,2,2,2,2,2,2,2,7,2,2,1,2,
  2,2,2,2,2,2,2,2,7,2,2,1,2,2,2,2,2,2,2,2,2,7,2,2,1,2,2,2,2,2,2,2,2,2,7,2,2,
  1,2,2,2,2,2,2,2,2,2,7,1,2,1,2,2,2,2,2,2,2,2,2,7,2,2,1,2,2,2,2,2,2,2,2,2,7,
  2,2,1,2,2,2,2,2,2,2,2,2,7,2,2,1,2,2,2,2,2,2,2,2,2,7,2,2,1,2,2,2,2,2,2,2,2,
  2,7,2,2,1,2,2,2,2,2,2,2,2,2,7,2,2,1,2,2,2,2,2,2,2,2,2,2,2,1,2,2,2,2,2,1,1,
  1,2,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,2,2,2,7,2,1,1,2,1,2,1,2,1,1,2,1,1,
  1,1,1,1,1,1,2,1,2,1,1,1,1,1,1,1,2,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,2,
  2,2,2,1,1,1,1,1,1,2,9,7,2,1,1,2,1,1,1,1,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,
  5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,
  5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,
  5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,7,1,3,8,8,8,8,8,8,8,8,8,8,8,8,8,8,
  8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,
  8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,
  8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,1,7,1,1,5,5,5,5,5,7,1,3,5,5,5,
  5,5,5,5,5,5,5,5,5,5,5,5,1,7,1,3,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,7,2,1,1,5,5,
  5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,1,7,1,3,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,
  2,2,2,7,2,1,5,5,5,5,5,7,1,3,5,5,5,5,5,7,1,3,5,5,1,7,1,3,1,2,7,2,1,1,2,1,1,
  5,5,5,5,5,7,1,3,5,5,5,5,5,7,1,3,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,7,
  1,3,2,2,2,2,2,2,2,2,2,2,2,2,2,4,4,4,4,2,2,4,7,2,1,1,5,1,7,1,3,2,7,1,1,1,5,
  5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,
  5,5,5,5,5,7,1,3,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,
  8,8,8,8,8,8,8,8,8,8,8,8,8,1,7,1,1,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,
  5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,1,7,1,3,2,2,2,2,2,2,2,2,2,2,2,
  1,2,2,2,2,2,1,1,1,2,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,2,2,2,7,2,1,1,2,1,
  2,1,2,1,1,1,1,1,1,1,1,1,2,1,2,1,1,1,1,1,1,2,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,
  1,1,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,
  5,5,5,5,5,5,1,7,1,3,2,2,2,2,2,2,2,2,2,2,2,1,2,2,2,2,2,1,1,1,1,1,1,1,1,1,1,
  1,1,1,1,1,1,1,1,1,1,1,2,2,2,7,2,1,1,2,1,2,1,1,1,1,1,1,1,1,1,2,1,2,1,1,1,1,
  1,1,2,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,1,7,1,
  3,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,7,2,1,1,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,1,7,
  1,3,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,7,2,1,1,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,1,
  7,1,3,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,7,2,1,1,1,5,5,5,5,5,5,5,5,5,5,5,5,5,5,
  5,1,7,1,3,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,7,2,1,1,1,5,5,5,5,5,5,5,5,5,5,5,5,
  5,5,5,1,7,1,3,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,7,2,1,1,1,5,5,5,5,5,5,5,5,5,5,
  5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,1,7,1,3,5,
  5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,
  5,5,5,5,1,7,1,3,2,2,2,2,2,2,2,2,2,2,2,1,2,2,2,2,2,1,1,1,2,1,1,1,1,1,1,1,1,
  1,1,1,1,1,1,1,1,1,1,2,2,2,7,2,1,1,2,1,2,1,2,1,1,2,1,1,1,1,1,1,1,1,2,1,2,1,
  1,1,1,1,1,1,2,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,5,5,5,5,5,5,5,5,5,5,5,5,
  5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,1,7,1,3,5,5,5,
  5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,
  5,5,1,7,1,3,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,
  5,5,5,5,5,5,5,5,5,5,5,1,7,1,3,2,2,2,2,2,2,2,2,2,2,2,1,2,2,2,2,2,1,1,1,2,1,
  1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,2,2,2,7,2,1,1,2,1,2,1,2,1,1,2,1,1,1,1,1,
  1,1,1,2,1,2,1,1,1,1,1,1,1,2,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,5,5,5,5,5,
  5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,1,
  7,1,3,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,
  5,5,5,5,5,5,5,1,7,1,3,2,2,2,2,2,2,2,2,2,2,2,1,2,2,2,2,2,1,1,1,1,1,1,1,1,1,
  1,1,1,1,1,1,1,1,1,1,1,1,2,2,2,7,2,1,1,2,1,2,1,1,1,1,1,1,1,1,1,2,1,2,1,1,1,
  1,1,1,2,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,5,5,5,5,5,7,1,3,5,5,5,5,5,5,5,5,5,
  5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,1,7,1,3,5,
  5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,
  5,5,5,1,7,1,3,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,
  5,5,5,5,5,5,5,5,5,5,5,1,7,1,3,2,2,2,2,2,2,2,2,2,2,2,1,2,2,2,2,2,1,1,1,1,1,
  1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,2,2,2,7,2,1,1,2,1,2,1,1,1,1,1,1,1,1,1,2,1,
  2,1,1,1,1,1,1,2,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,5,5,5,5,5,5,5,7,1,3,5,5,5,
  5,5,5,5,7,1,3,1,1,1,1,7,3,3,1,3,1,1,1,1,7,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,
  1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,7,2,2,1,2,2,2,2,2,2,2,2,
  2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,1,1,1,1,7,2,2,
  1,2,1,1,1,2,1,2,2,2,2,2,1,1,1,1,2,3,7,1,1,1,1,1,7,2,2,1,2,1,1,1,1,1,1,1,7,
  2,2,1,2,1,1,1,2,2,2,2,2,2,2,2,2,7,2,2,2,2,2,2,7,2,1,2,2,2,2,2,2,2,2,2,2,2,
  2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,7,1,4,4,4,4,
  4,4,4,4,4,4,4,4,4,4,4,4,4,4,4,4,4,4,4,4,4,4,4,4,4,4,4,4,4,4,4,4,4,4,4,4,4,
  4,4,4,4,4,4,4,7,4,4,4,4,4,4,4,4,4,4,4,4,4,4,4,4,4,4,4,4,4,4,4,4,4,4,4,4,4,
  4,4,4,4,4,4,4,4,4,4,4,4,4,4,4,4,4,4,4,7,1,7,1,1,7,1,1,7,1,1,7,1,1,7,1,1,7,
  1,1,7,1,1,7,1,1,7,1,1,7,1,2,2,2,2,2,2,2,2,2,2,2,1,2,2,2,2,2,1,1,1,1,1,1,1,
  1,1,1,1,1,1,1,1,1,1,1,1,1,1,2,2,2,7,2,1,1,2,1,2,1,1,1,1,1,1,1,1,1,1,1,2,1,
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
  5,5,5,5,5,5,5,5,5,5,5,1,7,1,3,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,
  5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,1,7,1,3,1,1,1,1,7,2,2,1,2,1,1,1,1,1,
  1,1,7,2,2,1,2,1,1,1,1,1,1,1,7,2,2,1,2,1,1,1,1,1,1,1,7,2,2,1,2,1,1,1,1,1,1,
  1,7,2,2,1,2,1,1,1,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,
  2,2,2,2,2,2,2,2,2,2,2,2,2,2,3,2,7,1,1,1,1,7,2,2,1,2,1,1,1,1,1,1,1,7,2,2,1,
  2,1,1,1,10,10,10,10,10,10,10,10,10,10,10,10,10,10,10,10,10,10,10,5,5,5,7,1,
  3,8,8,8,8,1,7,1,1,8,8,8,8,1,7,1,1,8,8,8,8,1,7,1,1,5,5,5,5,5,5,5,5,5,5,5,5,
  5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,1,7,1,3,2,2,2,2,
  2,2,2,2,2,2,2,1,2,2,2,2,2,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,2,2,2,
  7,2,1,1,2,1,2,1,1,1,1,1,1,1,1,1,1,1,2,1,1,1,1,1,1,2,1,1,1,1,1,1,1,1,1,1,1,
  1,1,1,1,1,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,
  5,5,5,5,5,5,5,5,5,1,7,1,3,2,2,2,2,2,2,2,2,2,2,2,1,2,2,2,2,2,1,1,1,1,1,1,1,
  1,1,1,1,1,1,1,1,1,1,1,1,1,1,2,2,2,7,2,1,1,2,1,2,1,1,1,1,1,1,1,1,1,1,1,2,1,
  1,1,1,1,1,2,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,7,3,3,1,3,1,1,1,5,5,5,
  5,5,7,1,3,8,8,8,8,1,7,1,1,5,5,5,5,5,5,5,5,7,1,3,5,5,5,5,5,5,5,7,1,3,8,8,8,
  8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,
  8,1,7,1,1,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,
  8,8,8,8,8,8,8,8,8,1,7,1,1,5,5,5,5,5,5,7,1,3,5,5,5,5,5,5,7,1,3,8,1,7,1,1,2,
  1,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,
  2,2,2,2,2,7,1,2,1,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,3,2,2,2,2,2,2,2,2,
  2,2,2,2,2,2,2,2,2,2,2,2,2,2,7,2,5,5,5,5,5,5,5,7,1,3,1,1,7,2,1,2,2,2,2,2,2,
  2,2,2,7,9,2,2,1,1,2,10,10,10,10,10,9,4,4,4,4,2,2,4,7,2,1,1,1,1,1,1,1,1,1,1,
  1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,
  1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,
  1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,7,1,1,1,1,1,1,1,1,1,1,
  1,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,
  2,2,2,2,2,2,2,2,2,2,2,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,
  1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,
  1,1,1,1,1,1,1,1,1,1,1,2,1,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,
  2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,7,1,1,7,1,2,2,2,2,2,2,2,2,2,2,2,1,2,2,2,
  2,2,1,1,1,2,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,2,2,2,7,2,1,1,2,1,2,1,2,1,
  1,2,1,1,1,1,1,1,1,1,2,1,2,1,1,1,1,1,1,1,2,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,
  1,1,1,4,4,4,4,4,7,1,1,4,4,4,4,4,7,1,1,4,4,4,4,4,7,1,1,4,4,4,4,4,7,1,1,4,4,
  4,4,4,7,1,5,1,7,1,3,1,7,2,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,
  1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,8,1,7,2,1,1,2,2,2,2,2,2,2,2,2,2,2,2,2,2,
  2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,1,1,1,1,1,1,1,1,1,7,1,
  1,1,1,1,7,4,4,4,4,4,4,4,4,4,4,4,4,4,4,4,4,1,1,1,1,1,4,4,4,4,4,4,4,4,4,4,4,
  4,4,4,4,4,4,4,4,4,4,4,7,2,7,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,1,7,1,1,2,2,2,2,
  2,2,2,2,2,2,2,1,2,2,2,2,2,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,2,2,2,
  7,2,1,1,2,1,2,1,1,1,1,1,1,1,1,1,1,1,2,1,1,1,1,1,1,2,1,1,1,1,1,1,1,1,1,1,1,
  1,1,1,1,1,2,2,2,2,2,2,2,2,2,2,2,1,2,2,2,2,2,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,
  1,1,1,1,1,1,2,2,2,7,2,1,1,2,1,2,1,1,1,1,1,1,1,1,1,1,1,2,1,1,1,1,1,1,2,1,1,
  1,1,1,1,1,1,1,1,1,1,1,1,1,1,2,2,2,2,2,2,2,2,2,2,2,1,2,2,2,2,2,1,1,1,1,1,1,
  1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,2,2,2,7,2,1,1,2,1,2,1,1,1,1,1,1,1,1,1,1,1,2,
  1,1,1,1,1,1,2,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,2,2,2,2,2,2,2,2,2,2,2,1,2,2,
  2,2,2,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,2,2,2,7,2,1,1,2,1,2,1,1,1,
  1,1,1,1,1,1,1,1,2,1,1,1,1,1,1,2,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,2,2,2,2,2,
  2,2,2,2,2,2,1,2,2,2,2,2,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,2,2,2,7,
  2,1,1,2,1,2,1,1,1,1,1,1,1,1,1,1,1,2,1,1,1,1,1,1,2,1,1,1,1,1,1,1,1,1,1,1,1,
  1,1,1,1,2,2,2,2,2,2,2,2,2,2,2,1,2,2,2,2,2,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,
  1,1,1,1,1,2,2,2,7,2,1,1,2,1,2,1,1,1,1,1,1,1,1,1,1,1,2,1,1,1,1,1,1,2,1,1,1,
  1,1,1,1,1,1,1,1,1,1,1,1,1,2,2,2,2,2,2,2,2,2,2,2,1,2,2,2,2,2,1,1,1,1,1,1,1,
  1,1,1,1,1,1,1,1,1,1,1,1,1,1,2,2,2,7,2,1,1,2,1,2,1,1,1,1,1,1,1,1,1,1,1,2,1,
  1,1,1,1,1,2,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,2,2,2,2,2,2,2,2,2,2,2,1,2,2,2,
  2,2,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,2,2,2,7,2,1,1,2,1,2,1,1,1,1,
  1,1,1,1,1,1,1,2,1,1,1,1,1,1,2,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,2,2,2,2,2,2,
  2,2,2,2,2,1,2,2,2,2,2,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,2,2,2,7,2,
  1,1,2,1,2,1,1,1,1,1,1,1,1,1,1,1,2,1,1,1,1,1,1,2,1,1,1,1,1,1,1,1,1,1,1,1,1,
  1,1,1,8,1,7,1,1,2,2,2,2,2,2,2,2,2,2,2,1,2,2,2,2,2,1,1,1,1,1,1,1,1,1,1,1,1,
  1,1,1,1,1,1,2,2,2,7,2,1,1,2,1,2,1,1,1,2,1,2,2,2,1,1,1,1,1,1,1,1,1,1,1,1,1,
  1,1,2,2,2,2,2,2,2,2,2,2,2,1,2,2,2,2,2,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,
  2,2,2,7,2,1,1,2,1,2,1,1,1,2,1,2,2,2,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,2,2,2,2,
  2,2,2,2,2,2,2,1,2,2,2,2,2,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,2,2,2,7,2,1,
  1,2,1,2,1,1,1,2,1,2,2,2,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,2,2,2,2,2,2,2,2,2,2,
  2,1,2,2,2,2,2,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,2,2,2,7,2,1,1,2,1,2,1,1,
  1,2,1,2,2,2,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,2,2,2,2,2,2,2,2,2,2,2,1,2,2,2,2,
  2,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,2,2,2,7,2,1,1,2,1,2,1,1,1,2,1,2,2,2,
  1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,2,2,2,2,2,2,2,2,2,2,2,1,2,2,2,2,2,1,1,1,1,1,
  1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,2,2,2,7,2,1,1,2,1,2,1,1,1,1,1,2,1,1,1,1,1,
  1,2,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,2,2,2,2,2,2,2,2,2,2,2,1,2,2,2,2,2,1,1,
  1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,2,2,2,7,2,1,1,2,1,2,1,1,1,1,1,2,1,1,
  1,1,1,1,2,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,2,2,2,2,2,2,2,2,2,2,2,1,2,2,2,2,
  2,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,2,2,2,7,2,1,1,2,1,2,1,1,1,1,1,
  1,2,1,1,1,1,1,1,2,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,2,2,2,2,2,2,2,2,2,2,2,1,
  2,2,2,2,2,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,2,2,2,7,2,1,1,2,1,2,1,
  1,1,1,1,1,2,1,1,1,1,1,1,2,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,2,2,2,2,2,2,2,2,
  2,2,2,1,2,2,2,2,2,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,2,2,2,7,2,1,1,
  2,1,2,1,1,1,1,1,1,2,1,1,1,1,1,1,2,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,2,2,2,2,
  2,2,2,2,2,2,2,1,2,2,2,2,2,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,2,2,2,
  7,2,1,1,2,1,2,1,1,1,1,1,1,2,1,1,1,1,1,1,2,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,
  2,2,2,2,2,2,2,2,2,2,2,1,2,2,2,2,2,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,
  1,2,2,2,7,2,1,1,2,1,2,1,1,1,1,1,1,1,2,1,1,1,1,1,1,2,1,1,1,1,1,1,1,1,1,1,1,
  1,1,1,1,1,2,2,2,2,2,2,2,2,2,2,2,1,2,2,2,2,2,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,
  1,1,1,1,1,1,2,2,2,7,2,1,1,2,1,2,1,1,1,1,1,1,1,2,1,1,1,1,1,1,2,1,1,1,1,1,1,
  1,1,1,1,1,1,1,1,1,1,2,2,2,2,2,2,2,2,2,2,2,1,2,2,2,2,2,1,1,1,1,1,1,1,1,1,1,
  1,1,1,1,1,1,1,1,1,1,1,2,2,2,7,2,1,1,2,1,2,1,1,1,1,1,1,1,1,2,1,1,1,1,1,1,2,
  1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,2,2,2,2,2,2,2,2,2,2,2,1,2,2,2,2,2,1,1,1,1,
  1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,2,2,2,7,2,1,1,2,1,2,1,1,1,1,1,1,1,1,2,1,
  1,1,1,1,1,2,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,2,2,2,2,2,2,2,2,2,2,2,1,2,2,2,
  2,2,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,2,2,2,7,2,1,1,2,1,2,1,1,1,1,
  1,1,1,1,1,2,1,1,1,1,1,1,2,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,2,2,2,2,2,2,2,2,
  2,2,2,1,2,2,2,2,2,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,2,2,2,7,2,1,1,
  2,1,2,1,1,1,1,1,1,1,1,1,2,1,1,1,1,1,1,2,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,
  1,1,1,7,2,2,1,2,1,1,1,1,1,1,1,7,2,2,1,2,1,1,1,1,1,1,1,7,2,2,1,2,1,1,1,8,8,
  8,8,1,7,1,1,8,8,8,8,1,7,1,1,1,1,1,1,7,2,2,1,2,1,1,1,2,2,2,2,2,2,2,2,2,2,2,
  1,2,2,2,2,2,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,2,2,2,7,2,1,1,2,1,2,
  1,1,1,1,1,1,1,1,1,2,1,2,1,1,1,1,1,1,2,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,2,2,
  2,2,2,2,2,2,2,2,2,1,2,2,2,2,2,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,2,
  2,2,7,2,1,1,2,1,2,1,1,1,1,1,1,1,1,1,2,1,2,1,1,1,1,1,1,2,1,1,1,1,1,1,1,1,1,
  1,1,1,1,1,1,1,1,7,1,2,1,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,
  2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,7,1,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,
  5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,7,1,3,
  8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,
  8,8,8,8,8,1,7,1,1,1,7,1,5,5,5,5,5,1,7,1,3,2,1,2,2,2,2,2,2,2,2,2,2,2,2,2,2,
  2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,7,2,8,8,8,8,8,8,8,8,
  8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,1,7,1,1,
  1,4,4,4,4,4,7,1,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,1,7,1,1,5,5,5,5,5,5,5,5,5,5,
  5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,7,1,
  3,4,4,4,4,4,4,4,4,4,4,4,4,4,4,4,4,4,4,4,4,4,4,4,4,4,4,4,4,4,4,4,4,4,4,4,4,
  4,4,4,4,5,5,5,5,5,7,1,1,1,1,7,2,2,1,2,1,1,1,4,4,4,4,4,4,4,4,2,2,2,2,4,4,4,
  4,2,2,2,2,2,4,4,4,4,4,4,4,4,4,4,4,4,4,4,4,4,4,4,4,4,4,4,7,2,2,2,2,2,7,2,2,
  2,2,2,2,2,2,2,2,2,2,2,2,2,7,1,1,1,1,1,7,2,1,7,2,1,7,2,1,7,2,1,7,2,1,7,2,1,
  7,2,1,7,2,1,7,2,1,7,3,4,1,1,1,1,1,4,4,4,4,4,4,4,4,4,4,4,4,4,4,4,4,4,4,4,4,
  4,4,4,4,4,4,4,4,4,4,7,1,1,1,1,1,4,1,1,1,1,1,4,4,4,4,4,4,4,4,4,4,4,4,4,4,4,
  4,4,4,4,4,4,4,4,4,4,4,4,4,4,4,7,1,1,1,1,1,4,1,1,4,4,4,4,4,4,4,4,4,4,4,4,4,
  4,4,4,4,4,4,4,4,4,4,4,4,4,4,4,4,4,7,1,1,4,1,1,4,4,4,4,4,4,4,4,4,4,4,4,4,4,
  4,4,4,4,4,4,4,4,4,4,4,4,4,4,4,4,7,1,1,4,1,1,4,4,4,4,4,4,4,4,4,4,4,4,4,4,4,
  4,4,4,4,4,4,4,4,4,4,4,4,4,4,4,7,1,1,4,1,1,4,4,4,4,4,4,4,4,4,4,4,4,4,4,4,4,
  4,4,4,4,4,4,4,4,4,4,4,4,4,4,7,1,1,4,1,1,1,1,4,4,4,4,4,4,4,4,4,4,4,4,4,4,4,
  4,4,4,4,4,4,4,4,4,4,4,7,1,1,1,1,4,1,1,1,1,4,4,4,4,4,4,4,4,4,4,4,4,4,4,4,4,
  4,4,4,4,4,4,4,4,4,4,7,1,1,1,1,4,1,1,4,4,4,4,4,4,4,4,4,4,4,4,4,4,4,4,4,4,4,
  4,4,4,4,4,7,1,1,4,1,1,4,4,4,4,4,4,4,4,4,4,4,4,4,4,4,4,4,4,4,4,4,4,4,4,7,1,
  1,4,1,1,4,4,4,4,4,4,4,4,4,4,4,4,4,4,4,4,4,4,4,4,4,4,7,1,1,4,1,1,4,4,4,4,4,
  4,4,4,4,4,4,4,4,4,4,4,4,4,4,4,4,4,7,1,1,1,1,1,1,7,2,2,1,2,1,1,1,1,1,1,1,7,
  2,2,1,2,1,1,1,8,8,8,8,8,8,8,8,8,1,7,1,1,4,4,4,4,4,4,4,4,4,4,4,4,4,4,4,4,4,
  4,4,4,4,2,4,4,4,4,4,4,4,4,4,4,4,4,4,4,4,4,4,4,4,4,4,4,7,2,2,2,2,2,2,2,2,2,
  2,2,1,2,2,2,2,2,1,1,1,2,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,2,2,2,7,2,1,1,
  2,1,2,1,2,1,1,2,1,1,1,1,1,1,1,1,2,1,2,1,1,1,1,1,1,2,1,1,1,1,1,1,1,1,1,1,1,
  1,1,1,1,1,1,1,2,2,2,2,2,2,2,2,2,2,2,1,2,2,2,2,2,1,1,1,1,1,1,1,1,1,1,1,1,1,
  1,1,1,1,1,1,1,1,2,2,2,7,2,1,1,2,1,2,1,1,1,1,1,1,1,1,1,2,1,2,1,1,1,1,1,1,2,
  1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,7,2,1,1,2,2,
  2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,5,2,1,7,1,3,2,5,5,7,1,3,8,1,7,1,1,2,2,2,2,
  2,2,2,2,2,7,2,2,1,1,7,2
};


static const unsigned short ag_pstt[] = {
2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,0,1,2,
18,20,
123,123,123,123,123,123,123,123,123,123,123,123,123,3,8,8,8,8,8,122,121,7,
  10,2,9,0,11,11,11,10,8,11,5,5,4,6,4,
19,19,19,19,19,19,19,19,19,19,19,1,3,1,849,
124,124,124,124,124,124,124,124,124,124,124,124,125,125,125,125,125,124,155,
  124,155,155,155,155,154,155,155,4,125,
12,13,14,15,16,17,18,19,20,21,22,5,33,39,32,31,30,29,28,27,26,25,24,23,
19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,
  19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,
  19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,
  19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,
  19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,
  19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,
  19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,1,
  6,1,840,
34,7,37,
36,36,35,37,38,39,34,8,24,24,42,24,41,40,40,36,36,
43,43,43,43,43,43,43,43,43,43,43,43,43,43,43,43,43,43,43,43,43,43,43,43,43,
  43,43,43,43,43,43,43,43,43,43,43,43,43,43,43,43,43,43,43,43,43,43,43,43,
  43,43,43,43,43,43,43,43,43,43,43,43,43,43,43,43,43,43,43,43,43,43,43,43,
  43,43,43,43,43,43,43,43,43,43,43,43,43,43,43,43,43,43,43,43,43,43,43,43,
  43,43,43,43,43,43,43,43,43,43,43,43,43,43,43,43,43,43,43,43,43,43,43,43,
  43,43,43,43,43,43,43,43,43,43,43,43,43,43,43,43,43,43,43,43,43,43,43,43,
  43,43,43,43,43,43,43,43,43,43,43,43,43,43,19,19,43,43,43,43,43,43,43,1,
  9,1,43,
123,123,123,123,123,123,123,123,123,123,123,123,123,107,108,109,110,111,112,
  113,114,61,62,63,64,65,115,116,117,118,119,120,66,67,68,69,70,121,122,
  123,71,72,73,74,75,124,125,126,127,128,129,130,131,132,133,134,135,136,
  137,138,104,59,60,143,144,145,146,147,148,149,150,151,152,153,154,155,
  156,139,140,141,142,76,77,78,79,80,45,46,47,81,82,83,84,85,86,87,88,89,
  48,49,90,91,92,93,94,95,96,97,50,51,52,98,99,53,54,100,101,102,103,55,
  56,57,58,220,220,220,220,220,220,220,220,220,220,220,220,220,220,220,
  220,220,220,220,220,220,220,220,220,220,220,220,220,220,220,220,15,16,
  19,220,220,20,20,35,20,20,20,20,122,121,18,10,216,217,218,215,221,220,
  220,35,219,97,98,221,44,214,215,215,215,215,215,215,215,215,215,308,308,
  308,308,308,309,310,311,312,315,315,315,316,317,321,321,321,321,322,323,
  324,325,326,327,328,329,332,332,332,333,334,335,336,337,338,339,340,341,
  345,345,345,345,346,347,348,349,350,199,198,197,196,213,212,211,210,209,
  208,207,206,205,204,203,202,201,200,159,106,158,105,157,195,194,193,192,
  191,190,189,188,187,186,185,184,183,182,181,180,180,180,180,180,180,179,
  178,177,176,176,176,176,176,176,175,174,173,172,171,170,169,169,169,169,
  168,168,168,167,166,165,164,163,162,161,160,214,
123,123,123,123,123,123,123,123,123,123,123,123,123,3,8,8,8,8,8,122,121,7,4,
  10,11,9,3,3,10,8,3,5,5,4,6,4,
19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,1,12,1,862,
19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,1,13,1,861,
19,1,14,1,860,
19,19,19,19,19,15,1,859,
19,19,19,19,19,16,1,858,
19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,1,17,1,857,
19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,
  19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,1,18,1,856,
19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,
  19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,1,19,1,855,
19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,1,20,1,854,
19,19,1,21,1,851,
19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,1,22,1,844,
123,123,123,123,123,123,123,123,123,123,123,123,123,122,121,23,222,223,214,
  214,
123,123,123,123,123,123,123,123,123,123,123,123,123,122,121,24,224,214,214,
132,25,227,225,226,
228,228,228,228,1,26,1,228,
229,229,229,229,1,27,1,229,
123,123,123,123,123,123,123,123,123,123,123,123,123,122,121,28,230,214,214,
123,123,123,123,123,123,123,123,123,123,123,232,123,238,238,238,238,235,237,
  234,244,245,246,247,248,249,250,251,252,253,268,269,270,233,267,236,256,
  231,253,122,121,29,222,242,243,223,238,224,239,216,280,279,277,282,278,
  281,276,283,285,271,209,214,284,272,276,275,274,273,208,276,266,265,264,
  263,262,261,260,259,258,257,241,240,214,255,254,
123,123,123,123,123,123,123,123,123,123,123,232,123,238,238,238,238,235,237,
  234,244,245,246,247,248,249,250,251,252,253,268,269,270,233,267,236,256,
  231,253,122,121,30,222,242,243,223,238,224,239,216,280,279,277,282,278,
  281,276,283,286,271,209,214,284,272,276,275,274,273,208,276,266,265,264,
  263,262,261,260,259,258,257,241,240,214,255,254,
123,123,123,123,123,123,123,123,123,123,123,123,123,122,121,31,287,214,214,
288,290,32,291,289,
150,132,33,295,225,294,292,226,293,
19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,
  19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,
  19,19,19,19,34,1,835,
19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,
  19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,
  19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,
  19,19,19,19,19,19,19,19,19,19,19,19,35,1,841,
296,22,36,297,297,297,
19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,
  19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,37,1,839,
19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,
  19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,38,
  1,837,
19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,
  19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,39,1,836,
298,300,302,303,304,305,306,307,308,309,310,311,312,313,314,315,316,317,318,
  319,320,321,328,331,332,333,334,268,270,330,322,326,323,325,329,301,299,
  324,256,327,35,37,335,38,34,15,40,16,15,15,40,13,40,15,15,15,15,15,15,
  15,15,15,15,15,15,15,15,15,15,15,15,15,15,15,15,15,15,15,15,15,15,15,15,
  15,15,15,15,15,15,15,15,15,15,15,
298,300,302,303,304,305,306,307,308,309,310,311,312,313,314,315,316,317,318,
  319,320,321,328,331,332,333,334,268,270,330,322,326,323,325,329,301,299,
  324,256,327,35,38,337,336,41,336,337,336,336,336,336,336,336,336,336,
  336,336,336,336,336,336,336,336,336,336,336,336,336,336,336,336,336,336,
  336,336,336,336,336,336,336,336,336,336,336,336,336,336,336,336,
298,300,302,303,304,305,306,307,308,309,310,311,312,313,314,315,316,317,318,
  319,320,321,328,331,332,333,334,268,270,330,322,326,323,325,329,301,299,
  324,256,327,35,38,338,336,42,336,338,336,336,336,336,336,336,336,336,
  336,336,336,336,336,336,336,336,336,336,336,336,336,336,336,336,336,336,
  336,336,336,336,336,336,336,336,336,336,336,336,336,336,336,336,
123,123,123,123,123,123,123,123,123,123,123,123,123,107,108,109,110,111,112,
  113,114,61,62,63,64,65,115,116,117,118,119,120,66,67,68,69,70,121,122,
  123,71,72,73,74,75,124,125,126,127,128,129,130,131,132,133,134,135,136,
  137,138,104,59,60,143,144,145,146,147,148,149,150,151,152,153,154,155,
  156,139,140,141,142,76,77,78,79,80,45,46,47,81,82,83,84,85,86,87,88,89,
  48,49,90,91,92,93,94,95,96,97,50,51,52,98,99,53,54,100,101,102,103,55,
  56,57,58,339,339,339,339,339,339,339,339,339,339,339,339,339,339,339,
  339,339,339,339,339,339,339,339,339,339,339,339,339,339,339,339,15,16,
  19,339,339,339,339,35,37,38,39,34,122,121,43,216,217,218,215,340,23,23,
  42,23,41,40,40,339,339,36,219,97,98,340,44,214,215,215,215,215,215,215,
  215,215,215,308,308,308,308,308,309,310,311,312,315,315,315,316,317,321,
  321,321,321,322,323,324,325,326,327,328,329,332,332,332,333,334,335,336,
  337,338,339,340,341,345,345,345,345,346,347,348,349,350,199,198,197,196,
  213,212,211,210,209,208,207,206,205,204,203,202,201,200,159,106,158,105,
  157,195,194,193,192,191,190,189,188,187,186,185,184,183,182,181,180,180,
  180,180,180,180,179,178,177,176,176,176,176,176,176,175,174,173,172,171,
  170,169,169,169,169,168,168,168,167,166,165,164,163,162,161,160,214,
341,341,341,341,341,341,1,44,1,341,
19,19,19,19,19,19,45,1,987,
19,19,19,19,19,19,46,1,986,
19,19,19,19,19,19,47,1,985,
19,19,19,19,19,19,48,1,975,
19,19,19,19,19,19,49,1,974,
19,19,19,19,19,19,50,1,965,
19,19,19,19,19,19,51,1,964,
19,19,19,19,19,19,52,1,963,
19,19,19,19,19,19,53,1,960,
19,19,19,19,19,19,54,1,959,
19,19,19,19,19,19,55,1,954,
19,19,19,19,19,19,56,1,953,
19,19,19,19,19,19,57,1,952,
19,19,19,19,19,19,58,1,951,
19,19,19,19,19,19,19,19,19,19,59,1,1014,
19,19,19,19,19,19,19,19,60,1,1013,
19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,
  19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,61,1,1054,
19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,
  19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,62,1,1053,
19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,
  19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,63,1,1052,
19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,
  19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,64,1,1051,
19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,
  19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,65,1,1050,
19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,
  19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,66,1,1043,
19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,
  19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,67,1,1042,
19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,
  19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,68,1,1041,
19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,
  19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,69,1,1040,
19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,
  19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,70,1,1039,
19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,
  19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,71,1,1035,
19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,
  19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,72,1,1034,
19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,
  19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,73,1,1033,
19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,
  19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,74,1,1032,
19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,
  19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,75,1,1031,
19,19,19,19,19,19,76,1,992,
19,19,19,19,19,19,77,1,991,
19,19,19,19,19,19,78,1,990,
19,19,19,19,19,19,79,1,989,
19,19,19,19,19,19,80,1,988,
19,19,19,19,19,19,81,1,984,
19,19,19,19,19,19,82,1,983,
19,19,19,19,19,19,83,1,982,
19,19,19,19,19,19,84,1,981,
19,19,19,19,19,19,85,1,980,
19,19,19,19,19,19,86,1,979,
19,19,19,19,19,19,87,1,978,
19,19,19,19,19,19,88,1,977,
19,19,19,19,19,19,89,1,976,
19,19,19,19,19,19,90,1,973,
19,19,19,19,19,19,91,1,972,
19,19,19,19,19,19,92,1,971,
19,19,19,19,19,19,93,1,970,
19,19,19,19,19,19,94,1,969,
19,19,19,19,19,19,95,1,968,
19,19,19,19,19,19,96,1,967,
19,19,19,19,19,19,97,1,966,
19,19,19,19,19,19,98,1,962,
19,19,19,19,19,19,99,1,961,
19,19,19,19,19,19,100,1,958,
19,19,19,19,19,19,101,1,957,
19,19,19,19,19,19,102,1,956,
19,19,19,19,19,19,103,1,955,
19,19,19,19,19,104,1,1015,
342,342,342,342,342,342,342,342,342,1,105,1,342,
343,343,343,343,343,343,343,1,106,1,343,
19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,
  19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,107,1,1062,
19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,
  19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,108,1,1061,
19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,
  19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,109,1,1060,
19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,
  19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,110,1,1059,
19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,
  19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,111,1,1058,
19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,
  19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,112,1,1057,
19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,
  19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,113,1,1056,
19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,
  19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,114,1,1055,
19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,
  19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,115,1,1049,
19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,
  19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,116,1,1048,
19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,
  19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,117,1,1047,
19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,
  19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,118,1,1046,
19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,
  19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,119,1,1045,
19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,
  19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,120,1,1044,
19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,
  19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,121,1,1038,
19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,
  19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,122,1,1037,
19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,
  19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,123,1,1036,
19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,
  19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,124,1,1030,
19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,
  19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,125,1,1029,
19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,
  19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,126,1,1028,
19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,
  19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,127,1,1027,
19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,
  19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,128,1,1026,
19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,
  19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,129,1,1025,
19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,
  19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,130,1,1024,
19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,
  19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,131,1,1023,
19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,
  19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,132,1,1022,
19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,
  19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,133,1,1021,
19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,
  19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,134,1,1020,
19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,
  19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,135,1,1019,
19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,
  19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,136,1,1018,
19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,
  19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,137,1,1017,
19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,
  19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,138,1,1016,
19,19,19,19,19,19,19,19,19,139,1,996,
19,19,19,19,19,19,19,19,19,140,1,995,
19,19,19,19,19,141,1,994,
19,19,19,19,19,142,1,993,
19,19,19,19,19,19,19,19,143,1,1012,
19,19,19,19,19,19,19,19,144,1,1011,
19,19,19,19,19,19,19,19,145,1,1010,
19,19,19,19,19,19,19,19,19,19,146,1,1008,
19,19,19,19,19,19,19,19,19,19,147,1,1007,
19,19,19,19,19,19,19,19,19,19,148,1,1006,
19,19,19,19,19,19,19,19,19,19,149,1,1005,
19,19,19,19,19,19,19,19,19,19,150,1,1004,
19,19,19,19,19,19,19,19,19,19,151,1,1003,
19,19,19,19,19,19,19,19,19,19,152,1,1002,
19,19,19,19,19,19,19,19,19,19,153,1,1001,
19,19,19,19,19,19,19,19,19,19,154,1,1000,
19,19,19,19,19,19,19,19,19,19,155,1,999,
19,19,19,19,19,19,19,19,19,19,156,1,998,
344,344,344,344,1,157,1,344,
345,1,158,1,345,
346,1,159,1,346,
347,347,347,347,347,347,347,347,347,347,347,347,347,347,347,347,347,347,347,
  347,347,347,347,347,347,347,347,347,347,347,347,347,347,347,347,347,347,
  347,347,347,347,1,160,1,347,
348,348,348,348,348,348,348,348,348,348,348,348,348,348,348,348,348,348,348,
  348,348,348,348,348,348,348,348,348,348,348,348,348,348,348,348,348,348,
  348,348,348,348,1,161,1,348,
349,349,349,349,349,349,349,349,349,349,349,349,349,349,349,349,349,349,349,
  349,349,349,349,349,349,349,349,349,349,349,349,349,349,349,349,349,349,
  349,349,349,349,1,162,1,349,
350,350,350,350,350,350,350,350,350,350,350,350,350,350,350,350,350,350,350,
  350,350,350,350,350,350,350,350,350,350,350,350,350,350,350,350,350,350,
  350,350,350,350,1,163,1,350,
351,351,351,351,351,351,351,351,351,351,351,351,351,351,351,351,351,351,351,
  351,351,351,351,351,351,351,351,351,351,351,351,351,351,351,351,351,351,
  351,351,351,351,1,164,1,351,
352,352,352,352,352,352,352,352,352,352,352,352,352,352,352,352,352,352,352,
  352,352,352,352,352,352,352,352,352,352,352,352,352,352,352,352,352,352,
  352,352,352,352,1,165,1,352,
353,353,353,353,353,353,353,353,353,353,353,353,353,353,353,353,353,353,353,
  353,353,353,353,353,353,353,353,353,353,353,353,353,353,353,353,353,353,
  353,353,353,353,1,166,1,353,
354,354,354,354,354,354,354,354,354,354,354,354,354,354,354,354,354,354,354,
  354,354,354,354,354,354,354,354,354,354,354,354,354,354,354,354,354,354,
  354,354,354,354,1,167,1,354,
355,355,355,355,355,355,355,355,355,355,355,355,355,355,355,355,355,355,355,
  355,355,355,355,355,355,355,355,355,355,355,355,355,355,355,355,355,355,
  355,355,355,355,1,168,1,355,
356,356,356,356,356,356,356,356,356,356,356,356,356,356,356,356,356,356,356,
  356,356,356,356,356,356,356,356,356,356,356,356,356,356,356,356,356,356,
  356,356,356,356,1,169,1,356,
357,357,357,357,357,357,357,357,357,357,357,357,357,357,357,357,357,357,357,
  357,357,357,357,357,357,357,357,357,357,357,357,357,357,357,357,357,357,
  357,357,357,357,1,170,1,357,
358,358,358,358,358,358,358,358,358,358,358,358,358,358,358,358,358,358,358,
  358,358,358,358,358,358,358,358,358,358,358,358,358,358,358,358,358,358,
  358,358,358,358,1,171,1,358,
359,359,359,359,359,359,359,359,359,359,359,359,359,359,359,359,359,359,359,
  359,359,359,359,359,359,359,359,359,359,359,359,359,359,359,359,359,359,
  359,359,359,359,1,172,1,359,
360,360,360,360,360,360,360,360,360,360,360,360,360,360,360,360,360,360,360,
  360,360,360,360,360,360,360,360,360,360,360,360,360,360,360,360,360,360,
  360,360,360,360,1,173,1,360,
361,361,361,361,361,361,361,361,361,361,361,361,361,361,361,361,361,361,361,
  361,361,361,361,361,361,361,361,361,361,361,361,361,361,361,361,361,361,
  361,361,361,361,1,174,1,361,
362,362,362,362,362,362,362,362,362,362,362,362,362,362,362,362,362,362,362,
  362,362,362,362,362,362,362,362,362,362,362,362,362,362,362,362,362,362,
  362,362,362,362,1,175,1,362,
363,363,363,363,363,363,363,363,363,363,363,363,363,363,363,363,363,363,363,
  363,363,363,363,363,363,363,363,363,363,363,363,363,363,363,363,363,363,
  363,363,363,363,1,176,1,363,
364,364,364,364,364,364,364,364,364,364,364,364,364,364,364,364,364,364,364,
  364,364,364,364,364,364,364,364,364,364,364,364,364,364,364,364,364,364,
  364,364,364,364,1,177,1,364,
365,365,365,365,365,365,365,365,365,365,365,365,365,365,365,365,365,365,365,
  365,365,365,365,365,365,365,365,365,365,365,365,365,365,365,365,365,365,
  365,365,365,365,1,178,1,365,
366,366,366,366,366,366,366,366,366,366,366,366,366,366,366,366,366,366,366,
  366,366,366,366,366,366,366,366,366,366,366,366,366,366,366,366,366,366,
  366,366,366,366,1,179,1,366,
367,367,367,367,367,367,367,367,367,367,367,367,367,367,367,367,367,367,367,
  367,367,367,367,367,367,367,367,367,367,367,367,367,367,367,367,367,367,
  367,367,367,367,1,180,1,367,
368,368,368,368,368,368,368,368,368,368,368,368,368,368,368,368,368,368,368,
  368,368,368,368,368,368,368,368,368,368,368,368,368,368,368,368,368,368,
  368,368,368,368,1,181,1,368,
369,369,369,369,369,369,369,369,369,369,369,369,369,369,369,369,369,369,369,
  369,369,369,369,369,369,369,369,369,369,369,369,369,369,369,369,369,369,
  369,369,369,369,1,182,1,369,
370,370,370,370,370,370,370,370,370,370,370,370,370,370,370,370,370,370,370,
  370,370,370,370,370,370,370,370,370,370,370,370,370,370,370,370,370,370,
  370,370,370,370,1,183,1,370,
371,371,371,371,371,371,371,371,371,371,371,371,371,371,371,371,371,371,371,
  371,371,371,371,371,371,371,371,371,371,371,371,371,371,371,371,371,371,
  371,371,371,371,1,184,1,371,
372,372,372,372,372,372,372,372,372,372,372,372,372,372,372,372,372,372,372,
  372,372,372,372,372,372,372,372,372,372,372,372,372,372,372,372,372,372,
  372,372,372,372,1,185,1,372,
373,373,373,373,373,373,373,373,373,373,373,373,373,373,373,373,373,373,373,
  373,373,373,373,373,373,373,373,373,373,373,373,373,373,373,373,373,373,
  373,373,373,373,1,186,1,373,
374,374,374,374,374,374,374,374,374,374,374,374,374,374,374,374,374,374,374,
  374,374,374,374,374,374,374,374,374,374,374,374,374,374,374,374,374,374,
  374,374,374,374,1,187,1,374,
375,375,375,375,375,375,375,375,375,375,375,375,375,375,375,375,375,375,375,
  375,375,375,375,375,375,375,375,375,375,375,375,375,375,375,375,375,375,
  375,375,375,375,1,188,1,375,
376,376,376,376,376,376,376,376,376,376,376,376,376,376,376,376,376,376,376,
  376,376,376,376,376,376,376,376,376,376,376,376,376,376,376,376,376,376,
  376,376,376,376,1,189,1,376,
377,377,377,377,377,377,377,377,377,377,377,377,377,377,377,377,377,377,377,
  377,377,377,377,377,377,377,377,377,377,377,377,377,377,377,377,377,377,
  377,377,377,377,1,190,1,377,
378,378,378,378,378,378,378,378,378,378,378,378,378,378,378,378,378,378,378,
  378,378,378,378,378,378,378,378,378,378,378,378,378,378,378,378,378,378,
  378,378,378,378,1,191,1,378,
379,379,379,379,379,379,379,379,379,379,379,379,379,379,379,379,379,379,379,
  379,379,379,379,379,379,379,379,379,379,379,379,379,379,379,379,379,379,
  379,379,379,379,1,192,1,379,
380,380,380,380,380,380,380,380,380,380,380,380,380,380,380,380,380,380,380,
  380,380,380,380,380,380,380,380,380,380,380,380,380,380,380,380,380,380,
  380,380,380,380,1,193,1,380,
381,381,381,381,381,381,381,381,381,381,381,381,381,381,381,381,381,381,381,
  381,381,381,381,381,381,381,381,381,381,381,381,381,381,381,381,381,381,
  381,381,381,381,1,194,1,381,
382,382,382,382,382,382,382,382,382,382,382,382,382,382,382,382,382,382,382,
  382,382,382,382,382,382,382,382,382,382,382,382,382,382,382,382,382,382,
  382,382,382,382,1,195,1,382,
383,383,383,383,383,383,383,383,1,196,1,383,
384,384,384,384,384,384,384,384,1,197,1,384,
385,385,385,385,1,198,1,385,
386,386,386,386,1,199,1,386,
387,387,387,387,387,387,387,1,200,1,387,
388,388,388,388,388,388,388,1,201,1,388,
389,389,389,389,389,389,389,1,202,1,389,
390,390,390,390,390,390,390,390,390,1,203,1,390,
391,391,391,391,391,391,391,391,391,1,204,1,391,
392,392,392,392,392,392,392,392,392,1,205,1,392,
393,393,393,393,393,393,393,393,393,1,206,1,393,
394,394,394,394,394,394,394,394,394,1,207,1,394,
395,395,395,395,395,395,395,395,395,1,208,1,395,
396,396,396,396,396,396,396,396,396,1,209,1,396,
397,397,397,397,397,397,397,397,397,1,210,1,397,
398,398,398,398,398,398,398,398,398,1,211,1,398,
399,399,399,399,399,399,399,399,399,1,212,1,399,
400,400,400,400,400,400,400,400,400,1,213,1,400,
19,124,124,124,124,124,124,124,124,124,124,124,19,124,19,19,125,125,125,125,
  125,19,124,19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,
  19,19,19,19,19,19,19,124,19,19,19,19,1,214,1,853,125,
19,19,19,19,19,1,215,1,293,
58,58,58,58,58,58,58,58,58,58,58,58,58,58,58,58,58,58,58,58,58,58,58,58,58,
  58,58,58,58,58,58,58,256,58,58,58,58,58,58,216,401,
108,108,108,108,402,217,402,
403,107,404,
123,123,123,123,123,123,123,123,123,123,123,232,123,238,238,238,238,235,237,
  234,244,245,246,247,248,249,250,251,252,253,268,269,270,233,267,236,256,
  231,253,122,121,219,222,242,243,223,238,224,239,216,280,279,277,282,278,
  281,276,283,96,271,209,214,284,272,276,275,274,273,208,276,266,265,264,
  263,262,261,260,259,258,257,241,240,214,255,254,
405,406,408,410,411,412,414,415,416,418,420,422,424,426,428,430,432,434,436,
  437,439,440,441,443,444,446,447,448,449,451,452,288,290,220,67,68,62,63,
  450,450,450,450,73,445,445,445,442,442,442,442,438,438,438,435,433,431,
  429,427,425,423,421,419,417,99,100,413,103,104,409,407,109,
453,453,453,453,1,221,1,453,
56,56,56,56,56,56,56,56,56,56,56,56,56,56,56,56,56,56,56,56,56,56,56,56,56,
  56,56,56,56,56,56,56,56,56,56,56,256,56,56,56,56,222,454,
455,455,455,455,455,455,455,455,455,455,455,455,455,455,455,455,455,455,455,
  455,455,455,455,455,455,455,455,455,455,455,455,455,455,455,455,455,455,
  455,455,455,1,223,1,455,
456,456,456,456,1,224,1,456,
135,457,135,135,135,135,135,135,135,135,135,135,135,135,135,135,135,135,135,
  135,135,135,135,135,131,135,135,135,135,135,135,135,135,135,135,135,135,
  135,135,135,135,135,135,225,133,
19,19,19,19,19,19,19,19,226,1,846,
458,458,458,458,1,227,1,458,
37,38,39,34,228,51,51,42,51,41,40,40,
37,38,39,34,229,50,50,42,50,41,40,40,
459,459,459,459,1,230,1,459,
264,264,264,264,264,231,
249,249,249,249,232,
247,247,233,
123,123,123,123,123,123,123,243,243,243,243,123,243,243,243,243,243,123,123,
  123,123,123,123,123,123,123,123,123,123,123,123,123,123,123,123,123,123,
  123,123,123,123,123,123,123,123,123,123,123,123,123,123,123,123,123,123,
  123,123,123,234,
238,460,238,238,238,238,238,238,238,238,238,238,238,238,238,238,238,238,238,
  238,238,238,238,238,238,238,238,238,238,238,238,238,238,238,238,238,238,
  238,238,238,238,238,238,238,238,238,238,238,238,238,238,238,238,238,235,
236,236,236,236,236,236,
254,461,254,254,254,254,254,254,254,254,254,254,254,254,254,254,254,254,254,
  254,254,254,254,254,254,254,254,254,254,254,254,254,254,254,254,254,254,
  254,254,254,254,260,254,237,462,
265,265,265,265,265,262,
229,246,246,246,246,246,246,246,246,246,239,
250,250,250,250,228,228,228,228,228,228,228,228,228,228,228,228,228,228,228,
  228,228,228,228,228,228,228,228,228,228,228,228,228,228,228,228,228,228,
  228,228,228,228,228,240,
248,248,227,227,227,227,227,227,227,227,227,227,227,227,227,227,227,227,227,
  227,227,227,227,227,227,227,227,227,227,227,227,227,227,227,227,227,227,
  227,227,227,241,
244,244,244,244,244,244,244,244,244,226,226,226,226,226,226,226,226,226,226,
  226,226,226,226,226,226,226,226,226,226,226,226,226,226,226,226,226,226,
  226,226,226,226,226,226,226,226,226,226,242,
240,230,245,463,464,245,239,239,239,239,239,241,225,225,225,225,225,225,225,
  225,225,225,225,225,225,225,225,225,225,225,225,225,225,225,225,225,225,
  225,225,225,225,225,225,263,225,225,225,225,234,234,225,243,234,
19,19,244,1,946,
19,19,245,1,944,
19,19,246,1,943,
19,19,247,1,942,
19,19,248,1,941,
19,19,249,1,940,
19,19,250,1,939,
19,19,251,1,938,
19,19,252,1,937,
19,19,253,1,936,
19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,
  19,19,19,19,19,19,19,19,19,19,19,19,19,254,1,947,
19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,
  19,19,19,19,19,19,19,19,19,19,19,19,1,255,1,900,
19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,
  19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,
  19,19,19,19,19,19,19,19,19,19,19,19,19,19,256,1,865,
465,1,257,1,465,
466,1,258,1,466,
467,1,259,1,467,
468,1,260,1,468,
469,1,261,1,469,
470,1,262,1,470,
471,1,263,1,471,
472,1,264,1,472,
473,1,265,1,473,
474,1,266,1,474,
19,19,19,19,19,19,19,19,19,19,19,19,19,237,237,237,237,237,19,19,19,19,19,
  19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,1,267,1,927,
19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,
  19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,
  19,19,19,19,19,19,19,19,19,19,19,19,268,1,935,
19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,
  19,19,19,19,19,19,19,19,19,19,19,19,19,1,269,1,934,
19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,
  19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,
  19,19,19,19,19,19,19,19,19,19,19,19,270,1,933,
475,475,475,475,475,475,475,475,475,475,475,475,475,475,475,475,475,475,475,
  475,475,475,475,475,475,475,475,475,475,475,475,475,475,475,475,475,475,
  475,475,475,475,1,271,1,475,
123,123,123,123,123,123,123,123,123,123,123,232,123,238,238,238,238,235,237,
  234,244,245,246,247,248,249,250,251,252,253,233,476,236,256,231,253,122,
  121,272,222,242,243,223,238,224,239,216,271,209,214,207,208,207,266,265,
  264,263,262,261,260,259,258,257,241,240,214,255,254,
123,123,123,123,123,123,123,123,123,123,123,232,123,238,238,238,238,235,237,
  234,244,245,246,247,248,249,250,251,252,253,233,476,236,256,231,253,122,
  121,273,222,242,243,223,238,224,239,216,271,209,214,206,208,206,266,265,
  264,263,262,261,260,259,258,257,241,240,214,255,254,
123,123,123,123,123,123,123,123,123,123,123,232,123,238,238,238,238,235,237,
  234,244,245,246,247,248,249,250,251,252,253,233,476,236,256,231,253,122,
  121,274,222,242,243,223,238,224,239,216,271,209,214,205,208,205,266,265,
  264,263,262,261,260,259,258,257,241,240,214,255,254,
123,123,123,123,123,123,123,123,123,123,123,232,123,238,238,238,238,235,237,
  234,244,245,246,247,248,249,250,251,252,253,233,476,236,256,231,253,122,
  121,275,222,242,243,223,238,224,239,216,271,209,214,204,208,204,266,265,
  264,263,262,261,260,259,258,257,241,240,214,255,254,
19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,
  19,19,19,19,19,19,19,19,19,19,19,19,1,276,1,197,
19,477,479,330,322,326,19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,
  19,19,19,19,19,19,19,19,19,19,19,19,1,277,1,194,483,482,481,480,478,
19,323,325,19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,
  19,19,19,19,19,19,19,19,1,278,1,189,485,484,
19,486,488,490,492,19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,
  19,19,19,19,19,19,1,279,1,186,493,491,489,487,
19,494,329,19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,
  19,19,1,280,1,183,496,495,
19,497,301,19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,1,
  281,1,180,499,498,
500,299,179,502,501,
503,504,505,506,507,508,509,510,511,512,513,514,515,159,159,159,159,159,283,
  163,163,163,163,166,166,166,169,169,169,172,172,172,175,175,175,178,178,
  178,
123,123,123,123,123,123,123,123,123,123,123,232,123,238,238,238,238,235,237,
  234,244,245,246,247,248,249,250,251,252,253,268,269,270,233,267,236,256,
  231,253,122,121,284,222,242,243,223,238,224,239,216,280,279,277,282,278,
  281,276,158,271,209,214,272,276,275,274,273,208,276,266,265,264,263,262,
  261,260,259,258,257,241,240,214,255,254,
516,516,516,516,1,285,1,516,
517,517,517,517,1,286,1,517,
518,518,518,518,518,287,518,
19,19,19,19,19,288,1,852,
519,519,519,519,1,289,1,519,
19,19,19,19,19,290,1,850,
520,520,520,520,1,291,1,520,
151,151,521,151,151,151,151,151,151,151,151,151,151,151,151,149,151,151,151,
  151,151,151,151,151,151,151,151,151,151,151,151,151,151,151,151,151,151,
  151,151,151,151,151,292,
19,19,19,19,19,293,1,847,
522,522,522,522,1,294,1,522,
523,523,523,523,1,295,1,523,
19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,1,296,1,845,
126,126,126,126,126,126,126,126,126,126,126,126,150,126,126,126,126,126,132,
  126,126,297,527,225,526,292,525,226,293,524,
19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,
  19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,
  298,1,1089,
19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,
  19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,
  19,19,19,19,19,19,19,19,19,19,19,19,19,299,1,916,
19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,
  19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,
  300,1,1088,
19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,
  19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,
  19,19,19,19,19,19,19,19,19,19,19,19,19,301,1,918,
19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,
  19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,302,1,
  1087,
19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,
  19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,
  303,1,1086,
19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,
  19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,
  304,1,1085,
19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,
  19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,
  305,1,1084,
19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,
  19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,
  306,1,1083,
19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,
  19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,
  307,1,1082,
19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,
  19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,
  308,1,1081,
19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,
  19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,
  309,1,1080,
19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,
  19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,
  310,1,1079,
19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,
  19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,
  311,1,1078,
19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,
  19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,
  312,1,1077,
19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,
  19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,
  313,1,1076,
19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,
  19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,
  314,1,1075,
19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,
  19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,
  315,1,1074,
19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,
  19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,
  316,1,1073,
19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,
  19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,
  317,1,1072,
19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,
  19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,
  318,1,1071,
19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,
  19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,
  319,1,1070,
19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,
  19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,
  320,1,1069,
19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,
  19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,
  321,1,1068,
19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,
  19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,
  19,19,19,19,19,19,19,19,19,19,19,19,322,1,929,
19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,
  19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,
  19,19,19,19,19,19,19,19,19,19,19,19,19,323,1,927,
19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,
  19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,
  19,19,19,19,19,19,19,19,19,19,19,19,19,324,1,888,
19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,
  19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,
  19,19,19,19,19,19,19,19,19,19,19,19,19,325,1,926,
19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,
  19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,
  19,19,19,19,19,19,19,19,19,19,19,19,326,1,928,
19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,
  19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,
  19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,
  327,1,863,
19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,
  19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,
  328,1,1067,
19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,
  19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,
  19,19,19,19,19,19,19,19,19,19,19,19,19,329,1,920,
19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,
  19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,
  19,19,19,19,19,19,19,19,19,19,19,19,330,1,930,
19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,
  19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,
  331,1,1066,
19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,
  19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,332,1,
  1065,
19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,
  19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,
  333,1,1064,
19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,
  19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,
  334,1,1063,
19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,
  19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,
  19,19,19,19,335,1,838,
298,300,302,303,304,305,306,307,308,309,310,311,312,313,314,315,316,317,318,
  319,320,321,328,331,332,333,334,268,270,330,322,326,323,325,329,301,299,
  324,256,327,35,38,7,9,7,7,7,7,7,7,7,7,7,7,7,7,7,7,7,7,7,7,7,7,7,7,7,7,7,
  7,7,7,7,7,7,7,7,7,7,7,7,7,7,7,7,7,
34,337,11,
34,338,10,
405,406,408,410,411,412,414,415,416,418,420,422,424,426,428,430,432,434,436,
  437,439,440,441,443,444,446,447,448,449,451,452,288,290,528,530,339,531,
  529,67,68,62,63,450,450,450,450,73,445,445,445,442,442,442,442,438,438,
  438,435,433,431,429,427,425,423,421,419,417,99,100,413,103,104,409,407,
  109,
532,532,532,532,1,340,1,532,
533,405,37,38,39,34,341,534,64,64,42,64,41,40,40,156,157,
275,274,274,271,270,269,268,267,266,342,371,274,535,
285,284,281,278,284,281,278,343,369,278,281,284,536,
373,373,373,373,344,373,
324,345,537,
324,346,538,
123,123,123,123,123,123,123,123,123,123,123,232,123,238,238,238,238,235,237,
  234,244,245,246,247,248,249,250,251,252,253,268,269,270,233,267,236,256,
  231,253,122,121,347,222,242,243,223,238,224,239,216,280,279,277,282,278,
  281,276,424,271,209,214,272,276,275,274,273,208,276,266,265,264,263,262,
  261,260,259,258,257,241,240,214,255,254,
123,123,123,123,123,123,123,123,123,123,123,232,123,238,238,238,238,235,237,
  234,244,245,246,247,248,249,250,251,252,253,268,269,270,233,267,236,256,
  231,253,122,121,348,222,242,243,223,238,224,239,216,280,279,277,282,278,
  281,276,423,271,209,214,272,276,275,274,273,208,276,266,265,264,263,262,
  261,260,259,258,257,241,240,214,255,254,
123,123,123,123,123,123,123,123,123,123,123,232,123,238,238,238,238,235,237,
  234,244,245,246,247,248,249,250,251,252,253,268,269,270,233,267,236,256,
  231,253,122,121,349,222,242,243,223,238,224,239,216,280,279,277,282,278,
  281,276,422,271,209,214,272,276,275,274,273,208,276,266,265,264,263,262,
  261,260,259,258,257,241,240,214,255,254,
123,123,123,123,123,123,123,123,123,123,123,232,123,238,238,238,238,235,237,
  234,244,245,246,247,248,249,250,251,252,253,268,269,270,233,267,236,256,
  231,253,122,121,350,222,242,243,223,238,224,239,216,280,279,277,282,278,
  281,276,421,271,209,214,272,276,275,274,273,208,276,266,265,264,263,262,
  261,260,259,258,257,241,240,214,255,254,
123,123,123,123,123,123,123,123,123,123,123,232,123,238,238,238,238,235,237,
  234,244,245,246,247,248,249,250,251,252,253,268,269,270,233,267,236,256,
  231,253,122,121,351,222,242,243,223,238,224,239,216,280,279,277,282,278,
  281,276,420,271,209,214,272,276,275,274,273,208,276,266,265,264,263,262,
  261,260,259,258,257,241,240,214,255,254,
123,123,123,123,123,123,123,123,123,123,123,232,123,238,238,238,238,235,237,
  234,244,245,246,247,248,249,250,251,252,253,268,269,270,233,267,236,256,
  231,253,122,121,352,222,242,243,223,238,224,239,216,280,279,277,282,278,
  281,276,419,271,209,214,272,276,275,274,273,208,276,266,265,264,263,262,
  261,260,259,258,257,241,240,214,255,254,
123,123,123,123,123,123,123,123,123,123,123,232,123,238,238,238,238,235,237,
  234,244,245,246,247,248,249,250,251,252,253,268,269,270,233,267,236,256,
  231,253,122,121,353,222,242,243,223,238,224,239,216,280,279,277,282,278,
  281,276,418,271,209,214,272,276,275,274,273,208,276,266,265,264,263,262,
  261,260,259,258,257,241,240,214,255,254,
123,123,123,123,123,123,123,123,123,123,123,232,123,238,238,238,238,235,237,
  234,244,245,246,247,248,249,250,251,252,253,268,269,270,233,267,236,256,
  231,253,122,121,354,222,242,243,223,238,224,239,216,280,279,277,282,278,
  281,276,417,271,209,214,272,276,275,274,273,208,276,266,265,264,263,262,
  261,260,259,258,257,241,240,214,255,254,
123,123,123,123,123,123,123,123,123,123,123,232,123,238,238,238,238,235,237,
  234,244,245,246,247,248,249,250,251,252,253,268,269,270,233,267,236,256,
  231,253,122,121,355,222,242,243,223,238,224,239,216,280,279,277,282,278,
  281,276,416,271,209,214,272,276,275,274,273,208,276,266,265,264,263,262,
  261,260,259,258,257,241,240,214,255,254,
123,123,123,123,123,123,123,123,123,123,123,232,123,238,238,238,238,235,237,
  234,244,245,246,247,248,249,250,251,252,253,268,269,270,233,267,236,256,
  231,253,122,121,356,222,242,243,223,238,224,239,216,280,279,277,282,278,
  281,276,413,271,209,214,272,276,275,274,273,208,276,266,265,264,263,262,
  261,260,259,258,257,241,240,214,255,254,
123,123,123,123,123,123,123,123,123,123,123,232,123,238,238,238,238,235,237,
  234,244,245,246,247,248,249,250,251,252,253,268,269,270,233,267,236,256,
  231,253,122,121,357,222,242,243,223,238,224,239,216,280,279,277,282,278,
  281,276,409,271,209,214,272,276,275,274,273,208,276,266,265,264,263,262,
  261,260,259,258,257,241,240,214,255,254,
123,123,123,123,123,123,123,123,123,123,123,232,123,238,238,238,238,235,237,
  234,244,245,246,247,248,249,250,251,252,253,268,269,270,233,267,236,256,
  231,253,122,121,358,222,242,243,223,238,224,239,216,280,279,277,282,278,
  281,276,408,271,209,214,272,276,275,274,273,208,276,266,265,264,263,262,
  261,260,259,258,257,241,240,214,255,254,
123,123,123,123,123,123,123,123,123,123,123,232,123,238,238,238,238,235,237,
  234,244,245,246,247,248,249,250,251,252,253,268,269,270,233,267,236,256,
  231,253,122,121,359,222,242,243,223,238,224,239,216,280,279,277,282,278,
  281,276,407,271,209,214,272,276,275,274,273,208,276,266,265,264,263,262,
  261,260,259,258,257,241,240,214,255,254,
123,123,123,123,123,123,123,123,123,123,123,232,123,238,238,238,238,235,237,
  234,244,245,246,247,248,249,250,251,252,253,268,269,270,233,267,236,256,
  231,253,122,121,360,222,242,243,223,238,224,239,216,280,279,277,282,278,
  281,276,406,271,209,214,272,276,275,274,273,208,276,266,265,264,263,262,
  261,260,259,258,257,241,240,214,255,254,
123,123,123,123,123,123,123,123,123,123,123,232,123,238,238,238,238,235,237,
  234,244,245,246,247,248,249,250,251,252,253,268,269,270,233,267,236,256,
  231,253,122,121,361,222,242,243,223,238,224,239,216,280,279,277,282,278,
  281,276,405,271,209,214,272,276,275,274,273,208,276,266,265,264,263,262,
  261,260,259,258,257,241,240,214,255,254,
123,123,123,123,123,123,123,123,123,123,123,232,123,238,238,238,238,235,237,
  234,244,245,246,247,248,249,250,251,252,253,268,269,270,233,267,236,256,
  231,253,122,121,362,222,242,243,223,238,224,239,216,280,279,277,282,278,
  281,276,404,271,209,214,272,276,275,274,273,208,276,266,265,264,263,262,
  261,260,259,258,257,241,240,214,255,254,
123,123,123,123,123,123,123,123,123,123,123,232,123,238,238,238,238,235,237,
  234,244,245,246,247,248,249,250,251,252,253,268,269,270,233,267,236,256,
  231,253,122,121,363,222,242,243,223,238,224,239,216,280,279,277,282,278,
  281,276,403,271,209,214,272,276,275,274,273,208,276,266,265,264,263,262,
  261,260,259,258,257,241,240,214,255,254,
123,123,123,123,123,123,123,123,123,123,123,232,123,238,238,238,238,235,237,
  234,244,245,246,247,248,249,250,251,252,253,268,269,270,233,267,236,256,
  231,253,122,121,364,222,242,243,223,238,224,239,216,280,279,277,282,278,
  281,276,397,271,209,214,272,276,275,274,273,208,276,266,265,264,263,262,
  261,260,259,258,257,241,240,214,255,254,
123,123,123,123,123,123,123,123,123,123,123,232,123,238,238,238,238,235,237,
  234,244,245,246,247,248,249,250,251,252,253,268,269,270,233,267,236,256,
  231,253,122,121,365,222,242,243,223,238,224,239,216,280,279,277,282,278,
  281,276,396,271,209,214,272,276,275,274,273,208,276,266,265,264,263,262,
  261,260,259,258,257,241,240,214,255,254,
123,123,123,123,123,123,123,123,123,123,123,232,123,238,238,238,238,235,237,
  234,244,245,246,247,248,249,250,251,252,253,268,269,270,233,267,236,256,
  231,253,122,121,366,222,242,243,223,238,224,239,216,280,279,277,282,278,
  281,276,395,271,209,214,272,276,275,274,273,208,276,266,265,264,263,262,
  261,260,259,258,257,241,240,214,255,254,
123,123,123,123,123,123,123,123,123,123,123,232,123,238,238,238,238,235,237,
  234,244,245,246,247,248,249,250,251,252,253,268,269,270,233,267,236,256,
  231,253,122,121,367,222,242,243,223,238,224,239,216,280,279,277,282,278,
  281,276,394,271,209,214,272,276,275,274,273,208,276,266,265,264,263,262,
  261,260,259,258,257,241,240,214,255,254,
123,123,123,123,123,123,123,123,123,123,123,232,123,238,238,238,238,235,237,
  234,244,245,246,247,248,249,250,251,252,253,268,269,270,233,267,236,256,
  231,253,122,121,368,222,242,243,223,238,224,239,216,280,279,277,282,278,
  281,276,388,271,209,214,272,276,275,274,273,208,276,266,265,264,263,262,
  261,260,259,258,257,241,240,214,255,254,
123,123,123,123,123,123,123,123,123,123,123,232,123,238,238,238,238,235,237,
  234,244,245,246,247,248,249,250,251,252,253,268,269,270,233,267,236,256,
  231,253,122,121,369,222,242,243,223,238,224,239,216,280,279,277,282,278,
  281,276,387,271,209,214,272,276,275,274,273,208,276,266,265,264,263,262,
  261,260,259,258,257,241,240,214,255,254,
123,123,123,123,123,123,123,123,123,123,123,232,123,238,238,238,238,235,237,
  234,244,245,246,247,248,249,250,251,252,253,268,269,270,233,267,236,256,
  231,253,122,121,370,222,242,243,223,238,224,239,216,280,279,277,282,278,
  281,276,386,271,209,214,272,276,275,274,273,208,276,266,265,264,263,262,
  261,260,259,258,257,241,240,214,255,254,
123,123,123,123,123,123,123,123,123,123,123,232,123,238,238,238,238,235,237,
  234,244,245,246,247,248,249,250,251,252,253,268,269,270,233,267,236,256,
  231,253,122,121,371,222,242,243,223,238,224,239,216,280,279,277,282,278,
  281,276,385,271,209,214,272,276,275,274,273,208,276,266,265,264,263,262,
  261,260,259,258,257,241,240,214,255,254,
123,123,123,123,123,123,123,123,123,123,123,232,123,238,238,238,238,235,237,
  234,244,245,246,247,248,249,250,251,252,253,268,269,270,233,267,236,256,
  231,253,122,121,372,222,242,243,223,238,224,239,216,280,279,277,282,278,
  281,276,384,271,209,214,272,276,275,274,273,208,276,266,265,264,263,262,
  261,260,259,258,257,241,240,214,255,254,
123,123,123,123,123,123,123,123,123,123,123,232,123,238,238,238,238,235,237,
  234,244,245,246,247,248,249,250,251,252,253,268,269,270,233,267,236,256,
  231,253,122,121,373,222,242,243,223,238,224,239,216,280,279,277,282,278,
  281,276,383,271,209,214,272,276,275,274,273,208,276,266,265,264,263,262,
  261,260,259,258,257,241,240,214,255,254,
123,123,123,123,123,123,123,123,123,123,123,232,123,238,238,238,238,235,237,
  234,244,245,246,247,248,249,250,251,252,253,268,269,270,233,267,236,256,
  231,253,122,121,374,222,242,243,223,238,224,239,216,280,279,277,282,278,
  281,276,382,271,209,214,272,276,275,274,273,208,276,266,265,264,263,262,
  261,260,259,258,257,241,240,214,255,254,
123,123,123,123,123,123,123,123,123,123,123,232,123,238,238,238,238,235,237,
  234,244,245,246,247,248,249,250,251,252,253,268,269,270,233,267,236,256,
  231,253,122,121,375,222,242,243,223,238,224,239,216,280,279,277,282,278,
  281,276,381,271,209,214,272,276,275,274,273,208,276,266,265,264,263,262,
  261,260,259,258,257,241,240,214,255,254,
123,123,123,123,123,123,123,123,123,123,123,232,123,238,238,238,238,235,237,
  234,244,245,246,247,248,249,250,251,252,253,268,269,270,233,267,236,256,
  231,253,122,121,376,222,242,243,223,238,224,239,216,280,279,277,282,278,
  281,276,380,271,209,214,272,276,275,274,273,208,276,266,265,264,263,262,
  261,260,259,258,257,241,240,214,255,254,
123,123,123,123,123,123,123,123,123,123,123,232,123,238,238,238,238,235,237,
  234,244,245,246,247,248,249,250,251,252,253,268,269,270,233,267,236,256,
  231,253,122,121,377,222,242,243,223,238,224,239,216,280,279,277,282,278,
  281,276,379,271,209,214,272,276,275,274,273,208,276,266,265,264,263,262,
  261,260,259,258,257,241,240,214,255,254,
123,123,123,123,123,123,123,123,123,123,123,232,123,238,238,238,238,235,237,
  234,244,245,246,247,248,249,250,251,252,253,268,269,270,233,267,236,256,
  231,253,122,121,378,222,242,243,223,238,224,239,216,280,279,277,282,278,
  281,276,378,271,209,214,272,276,275,274,273,208,276,266,265,264,263,262,
  261,260,259,258,257,241,240,214,255,254,
123,123,123,123,123,123,123,123,123,123,123,232,123,238,238,238,238,235,237,
  234,244,245,246,247,248,249,250,251,252,253,268,269,270,233,267,236,256,
  231,253,122,121,379,222,242,243,223,238,224,239,216,280,279,277,282,278,
  281,276,377,271,209,214,272,276,275,274,273,208,276,266,265,264,263,262,
  261,260,259,258,257,241,240,214,255,254,
123,123,123,123,123,123,123,123,123,123,123,232,123,238,238,238,238,235,237,
  234,244,245,246,247,248,249,250,251,252,253,268,269,270,233,267,236,256,
  231,253,122,121,380,222,242,243,223,238,224,239,216,280,279,277,282,278,
  281,276,376,271,209,214,272,276,275,274,273,208,276,266,265,264,263,262,
  261,260,259,258,257,241,240,214,255,254,
123,123,123,123,123,123,123,123,123,123,123,232,123,238,238,238,238,235,237,
  234,244,245,246,247,248,249,250,251,252,253,268,269,270,233,267,236,256,
  231,253,122,121,381,222,242,243,223,238,224,239,216,280,279,277,282,278,
  281,276,375,271,209,214,272,276,275,274,273,208,276,266,265,264,263,262,
  261,260,259,258,257,241,240,214,255,254,
123,123,123,123,123,123,123,123,123,123,123,232,123,238,238,238,238,235,237,
  234,244,245,246,247,248,249,250,251,252,253,268,269,270,233,267,236,256,
  231,253,122,121,382,222,242,243,223,238,224,239,216,280,279,277,282,278,
  281,276,374,271,209,214,272,276,275,274,273,208,276,266,265,264,263,262,
  261,260,259,258,257,241,240,214,255,254,
539,540,288,287,286,288,287,286,383,290,286,287,288,354,289,
539,540,288,287,286,288,287,286,384,290,286,287,288,353,289,
292,291,292,291,385,291,292,352,
292,291,292,291,386,291,292,351,
285,284,281,278,284,281,278,387,368,278,281,284,536,
285,284,281,278,284,281,278,388,367,278,281,284,536,
285,284,281,278,284,281,278,389,366,278,281,284,536,
275,274,274,271,270,269,268,267,266,390,365,274,535,
275,274,274,271,270,269,268,267,266,391,364,274,535,
275,274,274,271,270,269,268,267,266,392,363,274,535,
275,274,274,271,270,269,268,267,266,393,362,274,535,
275,274,274,271,270,269,268,267,266,394,541,274,535,
275,274,274,271,270,269,268,267,266,395,360,274,535,
275,274,274,271,270,269,268,267,266,396,359,274,535,
275,274,274,271,270,269,268,267,266,397,358,274,535,
275,274,274,271,270,269,268,267,266,398,357,274,535,
275,274,274,271,270,269,268,267,266,399,356,274,535,
275,274,274,271,270,269,268,267,266,400,355,274,535,
123,123,123,123,123,123,123,123,123,123,123,232,123,238,238,238,238,235,542,
  234,132,244,245,246,247,248,249,250,251,252,253,268,269,270,233,267,236,
  256,231,253,122,121,401,222,242,243,223,238,111,225,224,239,543,112,216,
  280,279,277,282,278,281,276,113,271,209,545,214,272,276,275,274,273,208,
  276,266,265,264,263,262,261,260,259,258,257,241,240,226,214,544,255,254,
232,238,238,238,238,235,237,546,233,476,236,253,18,402,110,242,547,224,239,
  241,240,255,
19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,
  19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,
  19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,
  19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,
  19,19,19,19,19,19,19,19,19,19,19,19,19,19,403,1,950,
548,548,548,548,548,548,548,548,548,548,548,548,548,548,548,548,548,548,548,
  548,548,548,548,548,548,548,548,548,548,548,548,548,548,548,548,548,548,
  548,548,548,548,548,548,548,548,548,548,548,548,548,548,548,548,548,548,
  548,548,548,548,548,548,548,548,548,548,548,548,548,548,548,548,548,548,
  548,548,548,548,548,548,548,548,548,548,548,548,548,548,548,548,548,548,
  548,548,548,548,548,548,548,548,548,548,548,548,548,548,548,548,548,548,
  548,1,404,1,548,
19,19,19,19,19,405,1,899,
19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,1,406,1,898,
123,123,123,123,123,123,123,123,123,123,123,123,123,122,121,407,106,214,214,
19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,1,408,1,897,
126,126,126,126,126,126,126,126,126,126,126,126,126,126,126,126,126,126,126,
  409,105,524,
19,19,19,19,19,410,1,896,
19,19,19,19,19,411,1,895,
19,19,1,412,1,894,
549,132,413,102,225,543,101,226,544,
19,19,19,19,19,414,1,892,
19,19,19,19,19,415,1,891,
19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,416,1,890,
123,123,123,123,123,123,123,123,123,123,123,123,123,94,94,94,94,122,121,94,
  417,95,214,214,
19,1,418,1,889,
132,419,550,225,226,
19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,
  19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,420,1,887,
551,551,551,551,551,551,551,551,551,551,551,551,551,551,551,551,551,551,551,
  551,551,551,551,551,551,551,551,551,551,551,551,551,551,551,551,551,551,
  551,551,551,551,551,1,421,1,551,
19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,
  19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,1,422,1,886,
123,123,123,123,123,123,123,123,123,123,123,232,123,238,238,238,238,235,237,
  234,132,244,245,246,247,248,249,250,251,252,253,268,269,270,233,267,236,
  256,231,253,122,121,423,222,242,243,223,238,91,225,224,239,216,280,279,
  277,282,278,281,276,90,271,209,214,272,276,275,274,273,208,276,266,265,
  264,263,262,261,260,259,258,257,241,240,226,214,255,254,
19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,
  19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,1,424,1,885,
123,123,123,123,123,123,123,123,123,123,123,232,123,238,238,238,238,235,237,
  234,244,245,246,247,248,249,250,251,252,253,268,269,270,233,267,236,256,
  231,253,122,121,425,222,242,243,223,238,224,239,216,280,279,277,282,278,
  281,276,89,271,209,214,272,276,275,274,273,208,276,266,265,264,263,262,
  261,260,259,258,257,241,240,214,255,254,
19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,1,426,1,884,
123,123,123,123,123,123,123,123,123,123,123,123,123,122,121,427,88,214,214,
19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,1,428,1,883,
123,123,123,123,123,123,123,123,123,123,123,123,123,122,121,429,87,214,214,
19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,1,430,1,882,
123,123,123,123,123,123,123,123,123,123,123,123,123,122,121,431,117,552,214,
  214,
19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,1,432,1,881,
123,123,123,123,123,123,123,123,123,123,123,123,123,122,121,433,117,553,214,
  214,
19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,1,434,1,880,
123,123,123,123,123,123,123,123,123,123,123,123,123,122,121,435,117,554,214,
  214,
19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,
  19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,1,436,1,879,
19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,
  19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,1,437,1,878,
123,123,123,123,123,123,123,123,123,123,123,232,123,238,238,238,238,235,542,
  234,132,244,245,246,247,248,249,250,251,252,253,268,269,270,233,267,236,
  256,231,253,122,121,438,222,242,243,223,238,111,225,224,239,543,112,216,
  280,279,277,282,278,281,276,113,271,209,555,214,272,276,275,274,273,208,
  276,266,265,264,263,262,261,260,259,258,257,241,240,226,214,544,255,254,
19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,
  19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,1,439,1,877,
19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,
  19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,1,440,1,876,
19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,
  19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,1,441,1,875,
123,123,123,123,123,123,123,123,123,123,123,232,123,238,238,238,238,235,542,
  234,132,244,245,246,247,248,249,250,251,252,253,268,269,270,233,267,236,
  256,231,253,122,121,442,222,242,243,223,238,111,225,224,239,543,112,216,
  280,279,277,282,278,281,276,113,271,209,556,214,272,276,275,274,273,208,
  276,266,265,264,263,262,261,260,259,258,257,241,240,226,214,544,255,254,
19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,
  19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,1,443,1,874,
19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,
  19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,1,444,1,873,
123,123,123,123,123,123,123,123,123,123,123,232,123,238,238,238,238,235,237,
  234,244,245,246,247,248,249,250,251,252,253,268,269,270,233,267,236,256,
  231,253,122,121,445,222,242,243,223,238,224,239,216,280,279,277,282,278,
  281,276,76,271,209,214,272,276,275,274,273,208,276,266,265,264,263,262,
  261,260,259,258,257,241,240,214,255,254,
19,19,19,19,19,446,1,872,
19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,
  19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,1,447,1,871,
19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,
  19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,1,448,1,870,
19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,
  19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,1,449,1,869,
123,123,123,123,123,123,123,123,123,123,123,232,123,238,238,238,238,235,237,
  234,244,245,246,247,248,249,250,251,252,253,268,269,270,233,267,236,256,
  231,253,122,121,450,222,242,243,223,238,224,239,216,280,279,277,282,278,
  281,276,72,271,209,214,272,276,275,274,273,208,276,266,265,264,263,262,
  261,260,259,258,257,241,240,214,255,254,
19,19,19,19,19,19,19,451,1,868,
19,19,19,19,19,19,19,452,1,867,
37,38,39,34,453,21,21,42,21,41,40,40,
557,454,558,
298,300,303,304,305,306,307,308,309,310,311,312,313,314,315,316,317,318,319,
  320,321,328,331,333,334,268,270,330,322,326,323,325,329,301,299,324,256,
  327,35,38,455,59,59,559,59,59,59,59,59,59,59,59,59,59,59,59,59,59,59,59,
  59,59,59,59,59,59,59,59,59,59,59,59,59,59,59,59,59,59,59,59,59,59,
37,38,39,34,456,53,53,42,53,41,40,40,
136,560,138,139,137,143,142,561,561,562,560,141,134,457,560,
37,38,39,34,458,52,52,42,52,41,40,40,
37,38,39,34,459,49,49,42,49,41,40,40,
242,242,242,242,242,242,242,242,242,460,
255,257,258,256,259,261,461,
254,461,254,254,254,254,254,254,254,254,254,254,254,254,254,254,254,254,254,
  254,254,251,254,254,254,254,254,254,254,254,254,254,254,254,254,254,254,
  254,254,254,254,254,260,254,462,563,
245,245,245,245,245,245,245,245,245,245,235,235,235,235,235,235,235,235,235,
  235,235,235,235,235,235,235,235,235,235,235,235,235,235,235,235,235,235,
  235,235,235,235,235,235,235,235,235,235,235,463,
245,245,245,245,245,245,245,245,245,245,231,231,231,231,231,231,231,231,231,
  231,231,231,231,231,231,231,231,231,231,231,231,231,231,231,231,231,231,
  231,231,231,231,231,231,231,231,231,231,231,464,
256,465,564,
256,466,565,
256,467,566,
256,468,567,
256,469,568,
256,470,569,
256,471,570,
256,472,571,
256,473,572,
256,474,573,
123,123,123,123,123,123,123,123,123,123,123,232,123,238,238,238,238,235,237,
  234,244,245,246,247,248,249,250,251,252,253,268,269,270,233,267,236,256,
  231,253,122,121,475,222,242,243,223,238,224,239,216,280,279,277,282,278,
  281,276,574,271,209,214,272,276,275,274,273,208,276,266,265,264,263,262,
  261,260,259,258,257,241,240,214,255,254,
237,237,237,237,237,476,
19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,
  19,19,19,19,19,19,19,19,19,19,19,19,19,19,477,1,932,
575,575,575,575,575,575,575,575,575,575,575,575,575,575,575,575,575,575,575,
  575,575,575,575,575,575,575,575,575,575,575,575,575,575,575,575,575,575,
  575,1,478,1,575,
19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,
  19,19,19,19,19,19,19,19,19,19,19,19,19,19,479,1,931,
576,576,576,576,576,576,576,576,576,576,576,576,576,576,576,576,576,576,576,
  576,576,576,576,576,576,576,576,576,576,576,576,576,576,576,576,576,576,
  576,1,480,1,576,
577,577,577,577,577,577,577,577,577,577,577,577,577,577,577,577,577,577,577,
  577,577,577,577,577,577,577,577,577,577,577,577,577,577,577,577,577,577,
  577,1,481,1,577,
578,578,578,578,578,578,578,578,578,578,578,578,578,578,578,578,578,578,578,
  578,578,578,578,578,578,578,578,578,578,578,578,578,578,578,578,578,578,
  578,1,482,1,578,
579,579,579,579,579,579,579,579,579,579,579,579,579,579,579,579,579,579,579,
  579,579,579,579,579,579,579,579,579,579,579,579,579,579,579,579,579,579,
  579,1,483,1,579,
580,580,580,580,580,580,580,580,580,580,580,580,580,580,580,580,580,580,580,
  580,580,580,580,580,580,580,580,580,580,580,580,580,580,580,580,580,580,
  580,580,580,580,1,484,1,580,
581,581,581,581,581,581,581,581,581,581,581,581,581,581,581,581,581,581,581,
  581,581,581,581,581,581,581,581,581,581,581,581,581,581,581,581,581,581,
  581,581,581,581,1,485,1,581,
19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,
  19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,486,1,925,
582,582,582,582,582,582,582,582,582,582,582,582,582,582,582,582,582,582,582,
  582,582,582,582,582,582,582,582,582,582,582,582,582,582,582,582,582,582,
  582,582,582,582,1,487,1,582,
19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,
  19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,488,1,924,
583,583,583,583,583,583,583,583,583,583,583,583,583,583,583,583,583,583,583,
  583,583,583,583,583,583,583,583,583,583,583,583,583,583,583,583,583,583,
  583,583,583,583,1,489,1,583,
19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,
  19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,490,1,923,
584,584,584,584,584,584,584,584,584,584,584,584,584,584,584,584,584,584,584,
  584,584,584,584,584,584,584,584,584,584,584,584,584,584,584,584,584,584,
  584,584,584,584,1,491,1,584,
19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,
  19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,492,1,922,
585,585,585,585,585,585,585,585,585,585,585,585,585,585,585,585,585,585,585,
  585,585,585,585,585,585,585,585,585,585,585,585,585,585,585,585,585,585,
  585,585,585,585,1,493,1,585,
19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,
  19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,494,1,921,
586,586,586,586,586,586,586,586,586,586,586,586,586,586,586,586,586,586,586,
  586,586,586,586,586,586,586,586,586,586,586,586,586,586,586,586,586,586,
  586,586,586,586,1,495,1,586,
587,587,587,587,587,587,587,587,587,587,587,587,587,587,587,587,587,587,587,
  587,587,587,587,587,587,587,587,587,587,587,587,587,587,587,587,587,587,
  587,587,587,587,1,496,1,587,
19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,
  19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,497,1,919,
588,588,588,588,588,588,588,588,588,588,588,588,588,588,588,588,588,588,588,
  588,588,588,588,588,588,588,588,588,588,588,588,588,588,588,588,588,588,
  588,588,588,588,1,498,1,588,
589,589,589,589,589,589,589,589,589,589,589,589,589,589,589,589,589,589,589,
  589,589,589,589,589,589,589,589,589,589,589,589,589,589,589,589,589,589,
  589,589,589,589,1,499,1,589,
19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,
  19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,500,1,917,
590,590,590,590,590,590,590,590,590,590,590,590,590,590,590,590,590,590,590,
  590,590,590,590,590,590,590,590,590,590,590,590,590,590,590,590,590,590,
  590,590,590,590,1,501,1,590,
591,591,591,591,591,591,591,591,591,591,591,591,591,591,591,591,591,591,591,
  591,591,591,591,591,591,591,591,591,591,591,591,591,591,591,591,591,591,
  591,591,591,591,1,502,1,591,
19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,
  19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,1,503,1,915,
19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,
  19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,1,504,1,914,
19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,
  19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,1,505,1,913,
19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,
  19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,1,506,1,912,
19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,
  19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,1,507,1,911,
19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,
  19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,1,508,1,910,
19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,
  19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,1,509,1,909,
19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,
  19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,1,510,1,908,
19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,
  19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,1,511,1,907,
19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,
  19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,1,512,1,906,
19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,
  19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,1,513,1,905,
19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,
  19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,1,514,1,904,
19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,
  19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,1,515,1,903,
37,38,39,34,516,48,48,42,48,41,40,40,
37,38,39,34,517,47,47,42,47,41,40,40,
37,38,39,34,518,46,46,42,46,41,40,40,
37,38,39,34,519,43,43,42,43,41,40,40,
37,38,39,34,520,42,42,42,42,41,40,40,
152,152,152,152,152,152,152,152,152,152,152,152,152,152,152,152,152,152,152,
  152,152,152,152,152,152,152,152,152,152,152,152,152,152,152,152,152,152,
  152,152,152,152,152,153,152,521,
37,38,39,34,522,41,41,42,41,41,40,40,
37,38,39,34,523,40,40,42,40,41,40,40,
127,127,127,127,127,127,127,127,127,127,127,127,127,127,127,127,127,127,127,
  19,19,19,524,1,848,
592,592,592,592,1,525,1,592,
593,593,593,593,1,526,1,593,
594,594,594,594,1,527,1,594,
19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,
  19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,1,528,1,843,
123,123,123,123,123,123,123,123,123,123,123,232,123,238,238,238,238,235,237,
  234,244,245,246,247,248,249,250,251,252,253,268,269,270,233,267,236,256,
  231,253,122,121,529,222,242,243,223,238,224,239,216,280,279,277,282,278,
  281,276,595,271,209,214,272,276,275,274,273,208,276,266,265,264,263,262,
  261,260,259,258,257,241,240,214,255,254,
19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,
  19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,1,530,1,842,
123,123,123,123,123,123,123,123,123,123,123,232,123,238,238,238,238,235,237,
  234,244,245,246,247,248,249,250,251,252,253,268,269,270,233,267,236,256,
  231,253,122,121,531,222,242,243,223,238,224,239,216,280,279,277,282,278,
  281,276,596,271,209,214,272,276,275,274,273,208,276,266,265,264,263,262,
  261,260,259,258,257,241,240,214,255,254,
37,38,39,34,532,22,22,42,22,41,40,40,
19,19,19,19,19,533,1,902,
597,597,597,597,1,534,1,597,
19,19,19,19,19,19,19,19,535,1,997,
19,19,19,19,19,19,19,536,1,1009,
598,598,598,598,598,598,598,598,598,598,598,598,598,598,598,598,598,598,598,
  598,598,598,598,598,598,598,598,598,598,598,598,598,598,598,598,598,598,
  598,598,598,598,1,537,1,598,
599,599,599,599,599,599,599,599,599,599,599,599,599,599,599,599,599,599,599,
  599,599,599,599,599,599,599,599,599,599,599,599,599,599,599,599,599,599,
  599,599,599,599,1,538,1,599,
19,19,19,19,19,19,539,1,949,
19,19,19,19,19,19,540,1,948,
600,1,541,1,600,
254,461,254,254,254,254,254,254,254,254,254,254,254,254,254,254,254,254,254,
  254,254,254,254,254,254,254,254,254,254,254,254,254,254,254,254,254,254,
  254,254,254,254,260,254,542,601,
254,461,254,254,254,254,254,254,254,254,254,254,254,254,254,254,254,254,254,
  254,254,128,254,254,254,254,254,254,254,254,254,254,254,254,254,254,254,
  254,254,254,254,254,260,254,543,130,
19,19,19,19,19,19,19,544,1,893,
602,327,545,57,603,
243,243,243,243,243,243,243,243,243,546,
240,230,245,463,464,245,239,239,239,239,239,241,225,225,225,225,234,234,225,
  547,234,
107,108,109,110,111,112,113,114,61,62,63,64,65,115,116,117,118,119,120,66,
  67,68,69,70,121,122,123,71,72,73,74,75,124,125,126,127,128,129,130,131,
  132,133,134,135,136,137,138,104,59,60,143,144,145,146,147,148,149,150,
  151,152,153,154,155,156,139,140,141,142,76,77,78,79,80,45,46,47,81,82,
  83,84,85,86,87,88,89,48,49,90,91,92,93,94,95,96,97,50,51,52,98,99,53,54,
  100,101,102,103,55,56,57,58,548,604,605,605,605,605,605,605,605,605,605,
  605,308,308,308,308,308,309,310,311,312,315,315,315,316,317,321,321,321,
  321,322,323,324,325,326,327,328,329,332,332,332,333,334,335,336,337,338,
  339,340,341,345,345,345,345,346,347,348,349,350,199,198,197,196,213,212,
  211,210,209,208,207,206,205,204,203,202,201,200,159,106,158,105,157,195,
  194,193,192,191,190,189,188,187,186,185,184,183,182,181,180,180,180,180,
  180,180,179,178,177,176,176,176,176,176,176,175,174,173,172,171,170,169,
  169,169,169,168,168,168,167,166,165,164,163,162,161,160,
254,461,254,254,254,254,254,254,254,254,254,254,254,254,254,254,254,254,254,
  254,254,254,254,254,254,254,254,254,254,254,254,254,254,254,254,254,254,
  254,254,254,254,260,254,549,606,
324,550,607,
123,123,123,123,123,123,123,123,123,123,123,232,123,238,238,238,238,235,542,
  234,132,244,245,246,247,248,249,250,251,252,253,268,269,270,233,267,236,
  256,231,253,122,121,551,222,242,243,223,238,111,225,224,239,543,112,216,
  280,279,277,282,278,281,276,113,271,209,608,214,272,276,275,274,273,208,
  276,266,265,264,263,262,261,260,259,258,257,241,240,226,214,544,255,254,
602,86,86,86,86,86,552,609,
602,85,85,85,85,85,553,609,
602,84,84,84,84,84,554,609,
602,83,83,83,83,83,555,603,
602,80,80,80,80,80,556,603,
19,1,557,1,864,
327,558,55,
298,300,303,304,305,306,307,308,309,310,311,312,313,314,315,316,317,318,319,
  320,321,328,331,333,334,268,270,330,322,326,323,325,329,301,299,324,610,
  256,327,35,38,612,611,559,60,1,612,60,60,60,61,60,60,60,60,60,60,60,60,
  60,60,60,60,60,60,60,60,60,60,60,60,60,60,60,60,60,60,60,60,60,60,60,60,
  60,60,60,60,
613,613,613,613,613,613,613,613,613,560,
614,614,614,614,614,561,
140,140,140,140,140,140,140,140,140,140,140,140,140,140,140,140,614,614,614,
  614,614,140,140,140,140,140,140,140,140,140,140,140,140,140,140,140,140,
  140,140,140,140,140,140,562,
252,563,
615,615,615,615,615,615,615,615,615,615,615,615,615,615,615,1,564,1,615,
123,123,123,123,123,123,123,123,123,123,123,232,123,238,238,238,238,235,237,
  234,244,245,246,247,248,249,250,251,252,253,268,269,270,233,267,236,256,
  231,253,122,121,565,222,242,243,223,238,224,239,216,280,279,277,282,278,
  281,276,616,271,209,214,272,276,275,274,273,208,276,266,265,264,263,262,
  261,260,259,258,257,241,240,214,255,254,
123,123,123,123,123,123,123,123,123,123,123,232,123,238,238,238,238,235,237,
  234,244,245,246,247,248,249,250,251,252,253,268,269,270,233,267,236,256,
  231,253,122,121,566,222,242,243,223,238,224,239,216,280,279,277,282,278,
  281,276,617,271,209,214,272,276,275,274,273,208,276,266,265,264,263,262,
  261,260,259,258,257,241,240,214,255,254,
123,123,123,123,123,123,123,123,123,123,123,232,123,238,238,238,238,235,237,
  234,244,245,246,247,248,249,250,251,252,253,268,269,270,233,267,236,256,
  231,253,122,121,567,222,242,243,223,238,224,239,216,280,279,277,282,278,
  281,276,618,271,209,214,272,276,275,274,273,208,276,266,265,264,263,262,
  261,260,259,258,257,241,240,214,255,254,
123,123,123,123,123,123,123,123,123,123,123,232,123,238,238,238,238,235,237,
  234,244,245,246,247,248,249,250,251,252,253,268,269,270,233,267,236,256,
  231,253,122,121,568,222,242,243,223,238,224,239,216,280,279,277,282,278,
  281,276,619,271,209,214,272,276,275,274,273,208,276,266,265,264,263,262,
  261,260,259,258,257,241,240,214,255,254,
123,123,123,123,123,123,123,123,123,123,123,232,123,238,238,238,238,235,237,
  234,244,245,246,247,248,249,250,251,252,253,268,269,270,233,267,236,256,
  231,253,122,121,569,222,242,243,223,238,224,239,216,280,279,277,282,278,
  281,276,620,271,209,214,272,276,275,274,273,208,276,266,265,264,263,262,
  261,260,259,258,257,241,240,214,255,254,
123,123,123,123,123,123,123,123,123,123,123,232,123,238,238,238,238,235,237,
  234,244,245,246,247,248,249,250,251,252,253,268,269,270,233,267,236,256,
  231,253,122,121,570,222,242,243,223,238,224,239,216,280,279,277,282,278,
  281,276,621,271,209,214,272,276,275,274,273,208,276,266,265,264,263,262,
  261,260,259,258,257,241,240,214,255,254,
123,123,123,123,123,123,123,123,123,123,123,232,123,238,238,238,238,235,237,
  234,244,245,246,247,248,249,250,251,252,253,268,269,270,233,267,236,256,
  231,253,122,121,571,222,242,243,223,238,224,239,216,280,279,277,282,278,
  281,276,622,271,209,214,272,276,275,274,273,208,276,266,265,264,263,262,
  261,260,259,258,257,241,240,214,255,254,
123,123,123,123,123,123,123,123,123,123,123,232,123,238,238,238,238,235,237,
  234,244,245,246,247,248,249,250,251,252,253,268,269,270,233,267,236,256,
  231,253,122,121,572,222,242,243,223,238,224,239,216,280,279,277,282,278,
  281,276,623,271,209,214,272,276,275,274,273,208,276,266,265,264,263,262,
  261,260,259,258,257,241,240,214,255,254,
123,123,123,123,123,123,123,123,123,123,123,232,123,238,238,238,238,235,237,
  234,244,245,246,247,248,249,250,251,252,253,268,269,270,233,267,236,256,
  231,253,122,121,573,222,242,243,223,238,224,239,216,280,279,277,282,278,
  281,276,624,271,209,214,272,276,275,274,273,208,276,266,265,264,263,262,
  261,260,259,258,257,241,240,214,255,254,
625,1,574,1,625,
123,123,123,123,123,123,123,123,123,123,123,232,123,238,238,238,238,235,237,
  234,244,245,246,247,248,249,250,251,252,253,233,476,236,256,231,253,122,
  121,575,222,242,243,223,238,224,239,216,271,209,214,202,208,202,266,265,
  264,263,262,261,260,259,258,257,241,240,214,255,254,
123,123,123,123,123,123,123,123,123,123,123,232,123,238,238,238,238,235,237,
  234,244,245,246,247,248,249,250,251,252,253,233,476,236,256,231,253,122,
  121,576,222,242,243,223,238,224,239,216,271,209,214,201,208,201,266,265,
  264,263,262,261,260,259,258,257,241,240,214,255,254,
123,123,123,123,123,123,123,123,123,123,123,232,123,238,238,238,238,235,237,
  234,244,245,246,247,248,249,250,251,252,253,233,476,236,256,231,253,122,
  121,577,222,242,243,223,238,224,239,216,271,209,214,200,208,200,266,265,
  264,263,262,261,260,259,258,257,241,240,214,255,254,
123,123,123,123,123,123,123,123,123,123,123,232,123,238,238,238,238,235,237,
  234,244,245,246,247,248,249,250,251,252,253,233,476,236,256,231,253,122,
  121,578,222,242,243,223,238,224,239,216,271,209,214,199,208,199,266,265,
  264,263,262,261,260,259,258,257,241,240,214,255,254,
123,123,123,123,123,123,123,123,123,123,123,232,123,238,238,238,238,235,237,
  234,244,245,246,247,248,249,250,251,252,253,233,476,236,256,231,253,122,
  121,579,222,242,243,223,238,224,239,216,271,209,214,198,208,198,266,265,
  264,263,262,261,260,259,258,257,241,240,214,255,254,
123,123,123,123,123,123,123,123,123,123,123,232,123,238,238,238,238,235,237,
  234,244,245,246,247,248,249,250,251,252,253,268,269,270,233,267,236,256,
  231,253,122,121,580,222,242,243,223,238,224,239,216,626,276,271,209,214,
  272,276,275,274,273,208,276,266,265,264,263,262,261,260,259,258,257,241,
  240,214,255,254,
123,123,123,123,123,123,123,123,123,123,123,232,123,238,238,238,238,235,237,
  234,244,245,246,247,248,249,250,251,252,253,268,269,270,233,267,236,256,
  231,253,122,121,581,222,242,243,223,238,224,239,216,627,276,271,209,214,
  272,276,275,274,273,208,276,266,265,264,263,262,261,260,259,258,257,241,
  240,214,255,254,
123,123,123,123,123,123,123,123,123,123,123,232,123,238,238,238,238,235,237,
  234,244,245,246,247,248,249,250,251,252,253,268,269,270,233,267,236,256,
  231,253,122,121,582,222,242,243,223,238,224,239,216,277,628,276,271,209,
  214,272,276,275,274,273,208,276,266,265,264,263,262,261,260,259,258,257,
  241,240,214,255,254,
123,123,123,123,123,123,123,123,123,123,123,232,123,238,238,238,238,235,237,
  234,244,245,246,247,248,249,250,251,252,253,268,269,270,233,267,236,256,
  231,253,122,121,583,222,242,243,223,238,224,239,216,277,629,276,271,209,
  214,272,276,275,274,273,208,276,266,265,264,263,262,261,260,259,258,257,
  241,240,214,255,254,
123,123,123,123,123,123,123,123,123,123,123,232,123,238,238,238,238,235,237,
  234,244,245,246,247,248,249,250,251,252,253,268,269,270,233,267,236,256,
  231,253,122,121,584,222,242,243,223,238,224,239,216,277,630,276,271,209,
  214,272,276,275,274,273,208,276,266,265,264,263,262,261,260,259,258,257,
  241,240,214,255,254,
123,123,123,123,123,123,123,123,123,123,123,232,123,238,238,238,238,235,237,
  234,244,245,246,247,248,249,250,251,252,253,268,269,270,233,267,236,256,
  231,253,122,121,585,222,242,243,223,238,224,239,216,277,631,276,271,209,
  214,272,276,275,274,273,208,276,266,265,264,263,262,261,260,259,258,257,
  241,240,214,255,254,
123,123,123,123,123,123,123,123,123,123,123,232,123,238,238,238,238,235,237,
  234,244,245,246,247,248,249,250,251,252,253,268,269,270,233,267,236,256,
  231,253,122,121,586,222,242,243,223,238,224,239,216,632,277,278,276,271,
  209,214,272,276,275,274,273,208,276,266,265,264,263,262,261,260,259,258,
  257,241,240,214,255,254,
123,123,123,123,123,123,123,123,123,123,123,232,123,238,238,238,238,235,237,
  234,244,245,246,247,248,249,250,251,252,253,268,269,270,233,267,236,256,
  231,253,122,121,587,222,242,243,223,238,224,239,216,633,277,278,276,271,
  209,214,272,276,275,274,273,208,276,266,265,264,263,262,261,260,259,258,
  257,241,240,214,255,254,
123,123,123,123,123,123,123,123,123,123,123,232,123,238,238,238,238,235,237,
  234,244,245,246,247,248,249,250,251,252,253,268,269,270,233,267,236,256,
  231,253,122,121,588,222,242,243,223,238,224,239,216,634,279,277,278,276,
  271,209,214,272,276,275,274,273,208,276,266,265,264,263,262,261,260,259,
  258,257,241,240,214,255,254,
123,123,123,123,123,123,123,123,123,123,123,232,123,238,238,238,238,235,237,
  234,244,245,246,247,248,249,250,251,252,253,268,269,270,233,267,236,256,
  231,253,122,121,589,222,242,243,223,238,224,239,216,635,279,277,278,276,
  271,209,214,272,276,275,274,273,208,276,266,265,264,263,262,261,260,259,
  258,257,241,240,214,255,254,
123,123,123,123,123,123,123,123,123,123,123,232,123,238,238,238,238,235,237,
  234,244,245,246,247,248,249,250,251,252,253,268,269,270,233,267,236,256,
  231,253,122,121,590,222,242,243,223,238,224,239,216,280,279,277,278,636,
  276,271,209,214,272,276,275,274,273,208,276,266,265,264,263,262,261,260,
  259,258,257,241,240,214,255,254,
123,123,123,123,123,123,123,123,123,123,123,232,123,238,238,238,238,235,237,
  234,244,245,246,247,248,249,250,251,252,253,268,269,270,233,267,236,256,
  231,253,122,121,591,222,242,243,223,238,224,239,216,280,279,277,278,637,
  276,271,209,214,272,276,275,274,273,208,276,266,265,264,263,262,261,260,
  259,258,257,241,240,214,255,254,
37,38,39,34,592,34,34,42,34,41,40,40,
37,38,39,34,593,33,33,42,33,41,40,40,
37,38,39,34,594,32,32,42,32,41,40,40,
638,638,638,638,1,595,1,638,
639,639,639,639,1,596,1,639,
37,38,39,34,597,65,65,42,65,41,40,40,
123,123,123,123,123,123,123,123,123,123,123,232,123,238,238,238,238,235,237,
  234,244,245,246,247,248,249,250,251,252,253,268,269,270,233,267,236,256,
  231,253,122,121,598,222,242,243,223,238,224,239,216,280,279,277,282,278,
  281,276,372,271,209,214,272,276,275,274,273,208,276,266,265,264,263,262,
  261,260,259,258,257,241,240,214,255,254,
123,123,123,123,123,123,123,123,123,123,123,232,123,238,238,238,238,235,237,
  234,244,245,246,247,248,249,250,251,252,253,268,269,270,233,267,236,256,
  231,253,122,121,599,222,242,243,223,238,224,239,216,280,279,277,282,278,
  281,276,370,271,209,214,272,276,275,274,273,208,276,266,265,264,263,262,
  261,260,259,258,257,241,240,214,255,254,
602,600,640,
254,461,254,254,254,254,254,254,254,254,254,254,254,254,254,254,254,254,254,
  254,254,251,254,254,254,254,254,254,254,254,254,254,254,254,254,254,254,
  254,254,254,254,254,260,254,601,641,
19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,
  19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,
  19,19,19,602,1,901,
642,642,642,642,642,642,642,642,642,642,642,642,642,642,642,642,642,642,642,
  642,642,642,642,642,642,642,642,642,642,642,642,642,642,642,642,642,642,
  642,642,642,642,642,1,603,1,642,
403,604,404,
19,303,303,303,303,1,605,1,293,
254,461,254,254,254,254,254,254,254,254,254,254,254,254,254,254,254,254,254,
  254,254,254,254,254,254,254,254,254,254,254,254,254,254,254,254,254,254,
  254,254,254,254,260,254,606,129,
643,643,643,643,643,643,643,643,643,643,643,643,643,643,643,643,643,643,643,
  643,643,643,643,643,643,643,643,643,643,643,643,643,643,643,643,643,643,
  643,643,643,643,1,607,1,643,
602,92,92,92,92,92,608,603,
644,644,644,644,644,644,644,644,644,644,644,644,644,644,644,1,609,1,644,
19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,
  19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,610,1,866,
60,60,60,60,60,60,60,60,60,60,60,60,60,60,60,60,60,60,60,60,60,60,60,60,60,
  60,60,60,60,60,60,60,60,60,60,60,60,60,60,60,17,17,17,17,17,611,
37,38,39,34,612,54,54,42,54,41,40,40,
148,148,148,148,148,148,148,148,147,147,147,147,148,148,148,148,147,147,147,
  147,147,148,148,148,148,148,148,148,148,148,148,148,148,148,148,148,148,
  148,148,148,148,148,148,613,
144,144,144,144,144,614,
123,123,123,123,123,123,123,123,123,123,123,123,123,122,121,615,647,645,645,
  646,
327,616,220,
327,617,219,
327,618,218,
327,619,217,
327,620,216,
327,621,215,
327,622,214,
327,623,213,
327,624,212,
327,625,211,
196,477,479,330,322,326,196,196,196,196,196,196,196,196,196,196,196,196,196,
  196,196,196,196,196,196,196,196,196,196,196,196,196,196,196,196,196,626,
  483,482,481,480,478,
195,477,479,330,322,326,195,195,195,195,195,195,195,195,195,195,195,195,195,
  195,195,195,195,195,195,195,195,195,195,195,195,195,195,195,195,195,627,
  483,482,481,480,478,
193,323,325,193,193,193,193,193,193,193,193,193,193,193,193,193,193,193,193,
  193,193,193,193,193,193,193,193,193,193,193,193,193,193,628,485,484,
192,323,325,192,192,192,192,192,192,192,192,192,192,192,192,192,192,192,192,
  192,192,192,192,192,192,192,192,192,192,192,192,192,192,629,485,484,
191,323,325,191,191,191,191,191,191,191,191,191,191,191,191,191,191,191,191,
  191,191,191,191,191,191,191,191,191,191,191,191,191,191,630,485,484,
190,323,325,190,190,190,190,190,190,190,190,190,190,190,190,190,190,190,190,
  190,190,190,190,190,190,190,190,190,190,190,190,190,190,631,485,484,
188,486,488,490,492,188,188,188,188,188,188,188,188,188,188,188,188,188,188,
  188,188,188,188,188,188,188,188,188,188,188,188,632,493,491,489,487,
187,486,488,490,492,187,187,187,187,187,187,187,187,187,187,187,187,187,187,
  187,187,187,187,187,187,187,187,187,187,187,187,633,493,491,489,487,
185,494,329,185,185,185,185,185,185,185,185,185,185,185,185,185,185,185,185,
  185,185,185,185,185,185,185,185,634,496,495,
184,494,329,184,184,184,184,184,184,184,184,184,184,184,184,184,184,184,184,
  184,184,184,184,184,184,184,184,635,496,495,
182,497,301,182,182,182,182,182,182,182,182,182,182,182,182,182,182,182,182,
  182,182,182,182,182,182,636,499,498,
181,497,301,181,181,181,181,181,181,181,181,181,181,181,181,181,181,181,181,
  181,181,181,181,181,181,637,499,498,
37,38,39,34,638,29,29,42,29,41,40,40,
37,38,39,34,639,28,28,42,28,41,40,40,
648,648,648,648,648,648,648,648,648,1,640,1,648,
129,129,129,129,129,129,129,129,129,129,129,129,129,129,129,129,129,129,129,
  129,129,252,129,129,129,129,129,129,129,129,129,129,129,129,129,129,129,
  129,129,129,129,129,129,129,641,
123,123,123,123,123,123,123,123,123,123,123,232,123,238,238,238,238,235,542,
  234,132,244,245,246,247,248,249,250,251,252,253,268,269,270,233,267,236,
  256,231,253,122,121,642,222,242,243,223,238,115,225,224,239,543,116,216,
  280,279,277,282,278,281,276,114,271,209,214,272,276,275,274,273,208,276,
  266,265,264,263,262,261,260,259,258,257,241,240,226,214,544,255,254,
123,123,123,123,123,123,123,123,123,123,123,232,123,238,238,238,238,235,237,
  234,244,245,246,247,248,249,250,251,252,253,268,269,270,233,267,236,256,
  231,253,122,121,643,222,242,243,223,238,224,239,216,280,279,277,282,278,
  281,276,93,271,209,214,272,276,275,274,273,208,276,266,265,264,263,262,
  261,260,259,258,257,241,240,214,255,254,
123,123,123,123,123,123,123,123,123,123,123,123,123,122,121,644,118,214,214,
124,124,124,124,124,124,124,124,124,124,124,124,125,125,125,125,125,124,19,
  124,1,645,1,119,125,
19,19,646,1,945,
649,1,647,1,649,
275,274,274,271,270,269,268,267,266,648,361,274,535,
327,649,221,

};


static const unsigned short ag_sbt[] = {
     0,  27,  29,  66,  81, 110, 134, 306, 309, 326, 498, 813, 849, 868,
   887, 892, 900, 908, 927, 972,1017,1036,1042,1067,1087,1106,1111,1119,
  1127,1146,1231,1316,1335,1340,1349,1405,1493,1499,1548,1599,1646,1740,
  1829,1918,2239,2249,2258,2267,2276,2285,2294,2303,2312,2321,2330,2339,
  2348,2357,2366,2375,2388,2399,2444,2489,2534,2579,2624,2669,2714,2759,
  2804,2849,2894,2939,2984,3029,3074,3083,3092,3101,3110,3119,3128,3137,
  3146,3155,3164,3173,3182,3191,3200,3209,3218,3227,3236,3245,3254,3263,
  3272,3281,3290,3299,3308,3317,3326,3334,3347,3358,3403,3448,3493,3538,
  3583,3628,3673,3718,3763,3808,3853,3898,3943,3988,4033,4078,4123,4168,
  4213,4258,4303,4348,4393,4438,4483,4528,4573,4618,4663,4708,4753,4798,
  4810,4822,4830,4838,4849,4860,4871,4884,4897,4910,4923,4936,4949,4962,
  4975,4988,5001,5014,5022,5027,5032,5077,5122,5167,5212,5257,5302,5347,
  5392,5437,5482,5527,5572,5617,5662,5707,5752,5797,5842,5887,5932,5977,
  6022,6067,6112,6157,6202,6247,6292,6337,6382,6427,6472,6517,6562,6607,
  6652,6664,6676,6684,6692,6703,6714,6725,6738,6751,6764,6777,6790,6803,
  6816,6829,6842,6855,6868,6928,6937,6978,6985,6988,7073,7144,7152,7195,
  7239,7247,7292,7303,7311,7323,7335,7343,7349,7354,7357,7416,7471,7477,
  7522,7528,7539,7582,7623,7671,7724,7729,7734,7739,7744,7749,7754,7759,
  7764,7769,7774,7815,7856,7922,7927,7932,7937,7942,7947,7952,7957,7962,
  7967,7972,8014,8078,8120,8184,8229,8297,8365,8433,8501,8542,8586,8624,
  8662,8694,8724,8729,8767,8850,8858,8866,8873,8881,8889,8897,8905,8948,
  8956,8964,8972,8997,9027,9078,9143,9194,9259,9308,9359,9410,9461,9512,
  9563,9614,9665,9716,9767,9818,9869,9920,9971,10022,10073,10124,10175,
  10226,10277,10341,10406,10471,10536,10600,10675,10726,10791,10855,10906,
  10955,11006,11057,11113,11199,11202,11205,11280,11288,11305,11318,11331,
  11337,11340,11343,11426,11509,11592,11675,11758,11841,11924,12007,12090,
  12173,12256,12339,12422,12505,12588,12671,12754,12837,12920,13003,13086,
  13169,13252,13335,13418,13501,13584,13667,13750,13833,13916,13999,14082,
  14165,14248,14331,14346,14361,14369,14377,14390,14403,14416,14429,14442,
  14455,14468,14481,14494,14507,14520,14533,14546,14559,14650,14672,14786,
  14900,14908,14927,14946,14969,14991,14999,15007,15013,15022,15030,15038,
  15061,15085,15090,15095,15141,15187,15233,15320,15365,15448,15467,15486,
  15505,15524,15543,15563,15582,15602,15621,15641,15687,15733,15824,15870,
  15916,15962,16053,16098,16143,16226,16234,16279,16324,16369,16452,16462,
  16472,16484,16487,16569,16581,16596,16608,16620,16630,16637,16683,16732,
  16781,16784,16787,16790,16793,16796,16799,16802,16805,16808,16811,16894,
  16900,16942,16984,17026,17068,17110,17152,17194,17239,17284,17329,17374,
  17419,17464,17509,17554,17599,17644,17689,17734,17779,17824,17869,17914,
  17959,18004,18049,18094,18139,18184,18229,18274,18319,18364,18409,18454,
  18499,18544,18589,18634,18646,18658,18670,18682,18694,18739,18751,18763,
  18788,18796,18804,18812,18857,18940,18985,19068,19080,19088,19096,19107,
  19117,19162,19207,19216,19225,19230,19275,19321,19331,19336,19346,19367,
  19610,19655,19658,19749,19757,19765,19773,19781,19789,19794,19797,19884,
  19894,19900,19944,19946,19965,20048,20131,20214,20297,20380,20463,20546,
  20629,20712,20717,20785,20853,20921,20989,21057,21134,21211,21289,21367,
  21445,21523,21602,21681,21761,21841,21922,22003,22015,22027,22039,22047,
  22055,22067,22150,22233,22236,22282,22337,22383,22386,22395,22440,22485,
  22493,22512,22560,22606,22618,22662,22668,22688,22691,22694,22697,22700,
  22703,22706,22709,22712,22715,22718,22760,22802,22838,22874,22910,22946,
  22982,23018,23048,23078,23106,23134,23146,23158,23171,23216,23306,23389,
  23408,23433,23438,23443,23456,23459
};


static const unsigned short ag_sbe[] = {
    24,  28,  52,  78, 108, 121, 303, 307, 316, 495, 667, 837, 865, 884,
   889, 897, 905, 924, 969,1014,1033,1039,1064,1082,1102,1107,1116,1124,
  1142,1187,1272,1331,1337,1342,1402,1490,1495,1545,1596,1643,1692,1784,
  1873,2086,2246,2255,2264,2273,2282,2291,2300,2309,2318,2327,2336,2345,
  2354,2363,2372,2385,2396,2441,2486,2531,2576,2621,2666,2711,2756,2801,
  2846,2891,2936,2981,3026,3071,3080,3089,3098,3107,3116,3125,3134,3143,
  3152,3161,3170,3179,3188,3197,3206,3215,3224,3233,3242,3251,3260,3269,
  3278,3287,3296,3305,3314,3323,3331,3344,3355,3400,3445,3490,3535,3580,
  3625,3670,3715,3760,3805,3850,3895,3940,3985,4030,4075,4120,4165,4210,
  4255,4300,4345,4390,4435,4480,4525,4570,4615,4660,4705,4750,4795,4807,
  4819,4827,4835,4846,4857,4868,4881,4894,4907,4920,4933,4946,4959,4972,
  4985,4998,5011,5019,5024,5029,5074,5119,5164,5209,5254,5299,5344,5389,
  5434,5479,5524,5569,5614,5659,5704,5749,5794,5839,5884,5929,5974,6019,
  6064,6109,6154,6199,6244,6289,6334,6379,6424,6469,6514,6559,6604,6649,
  6661,6673,6681,6689,6700,6711,6722,6735,6748,6761,6774,6787,6800,6813,
  6826,6839,6852,6865,6924,6934,6976,6983,6986,7029,7106,7149,7193,7236,
  7244,7290,7300,7308,7315,7327,7340,7348,7353,7356,7415,7470,7476,7520,
  7527,7538,7581,7622,7670,7722,7726,7731,7736,7741,7746,7751,7756,7761,
  7766,7771,7812,7853,7919,7924,7929,7934,7939,7944,7949,7954,7959,7964,
  7969,8011,8075,8117,8181,8226,8267,8335,8403,8471,8539,8578,8619,8655,
  8689,8719,8726,8747,8808,8855,8863,8871,8878,8886,8894,8902,8947,8953,
  8961,8969,8994,9018,9075,9140,9191,9256,9305,9356,9407,9458,9509,9560,
  9611,9662,9713,9764,9815,9866,9917,9968,10019,10070,10121,10172,10223,
  10274,10338,10403,10468,10533,10597,10672,10723,10788,10852,10903,10952,
  11003,11054,11110,11156,11200,11203,11240,11285,11294,11314,11325,11335,
  11338,11341,11384,11467,11550,11633,11716,11799,11882,11965,12048,12131,
  12214,12297,12380,12463,12546,12629,12712,12795,12878,12961,13044,13127,
  13210,13293,13376,13459,13542,13625,13708,13791,13874,13957,14040,14123,
  14206,14289,14339,14354,14365,14373,14384,14397,14410,14425,14438,14451,
  14464,14477,14490,14503,14516,14529,14542,14555,14601,14663,14783,14897,
  14905,14924,14942,14966,14988,14996,15004,15010,15015,15027,15035,15058,
  15081,15087,15091,15138,15184,15230,15275,15362,15406,15464,15482,15502,
  15520,15540,15558,15579,15597,15618,15636,15684,15730,15775,15867,15913,
  15959,16004,16095,16140,16184,16231,16276,16321,16366,16410,16459,16469,
  16476,16485,16527,16573,16594,16600,16612,16629,16636,16681,16731,16780,
  16782,16785,16788,16791,16794,16797,16800,16803,16806,16809,16852,16899,
  16939,16981,17023,17065,17107,17149,17191,17236,17281,17326,17371,17416,
  17461,17506,17551,17596,17641,17686,17731,17776,17821,17866,17911,17956,
  18001,18046,18091,18136,18181,18226,18271,18316,18361,18406,18451,18496,
  18541,18586,18631,18638,18650,18662,18674,18686,18738,18743,18755,18785,
  18793,18801,18809,18854,18898,18982,19026,19072,19085,19093,19104,19114,
  19159,19204,19213,19222,19227,19273,19319,19328,19333,19345,19365,19477,
  19653,19656,19700,19755,19763,19771,19779,19787,19791,19795,19840,19893,
  19899,19943,19945,19962,20006,20089,20172,20255,20338,20421,20504,20587,
  20670,20714,20755,20823,20891,20959,21027,21098,21175,21252,21330,21408,
  21486,21564,21643,21722,21802,21882,21963,22007,22019,22031,22044,22052,
  22059,22108,22191,22234,22280,22334,22380,22384,22392,22438,22482,22491,
  22509,22557,22605,22610,22661,22667,22683,22689,22692,22695,22698,22701,
  22704,22707,22710,22713,22716,22754,22796,22835,22871,22907,22943,22977,
  23013,23045,23075,23103,23131,23138,23150,23168,23215,23258,23347,23404,
  23429,23435,23440,23452,23457,23459
};


static const unsigned char ag_fl[] = {
  2,1,1,2,2,1,1,2,0,1,3,3,1,2,1,2,2,1,2,0,1,4,5,3,2,1,0,1,7,7,1,1,6,6,6,
  2,3,2,1,2,4,4,4,4,0,1,4,4,4,4,3,3,4,4,6,4,1,4,1,1,2,2,2,2,3,5,1,2,2,1,
  1,1,3,2,1,1,3,1,1,1,3,1,1,3,3,3,3,3,3,3,3,3,4,6,2,3,2,1,1,2,2,3,3,2,2,
  3,3,1,1,2,3,1,1,1,4,4,4,1,4,2,1,1,1,1,2,2,1,2,2,3,2,2,1,2,3,1,2,2,2,2,
  2,2,2,2,4,1,1,4,3,2,1,2,3,3,2,1,1,1,2,1,1,1,1,2,1,1,2,1,1,2,1,1,2,1,1,
  2,1,1,2,1,2,4,4,2,4,4,2,4,4,2,4,4,4,4,2,4,4,2,4,4,4,4,4,1,2,2,2,2,1,1,
  1,5,5,5,5,5,5,5,5,5,5,7,1,1,1,1,1,1,1,2,2,2,1,1,2,2,2,2,1,2,2,2,3,2,2,
  2,2,2,2,2,2,3,4,1,1,2,2,2,2,2,1,2,1,2,2,2,1,1,1,1,1,1,1,1,1,1,1,1,1,1,
  1,1,1,1,1,1,1,1,1,1,1,1,1,2,1,1,1,1,1,1,1,1,1,4,1,1,1,1,1,1,1,1,1,1,1,
  1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,
  1,3,3,3,3,3,3,3,3,3,3,7,3,3,3,3,3,3,3,3,5,3,5,3,3,3,3,3,3,3,3,3,3,3,3,
  3,3,3,3,1,1,1,1,1,3,3,3,3,1,1,1,1,1,3,3,3,3,3,3,3,1,1,1,3,1,1,3,3,3,3,
  3,3,3,3,3,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,
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
  1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,2,2,2,2,2,
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
   35, 35, 64, 53, 66, 66, 66, 66, 71, 71, 66, 66, 66, 66, 66, 66, 66, 66,
   66, 81, 81, 86, 86, 82, 82, 82, 91, 91, 62, 62, 33, 33, 33, 97, 97, 97,
   33, 33,101,101, 33,105,105,105, 33,108,108, 33, 33, 33, 33, 33, 33, 33,
   33, 33, 33, 33, 33, 33, 33, 33, 33, 33, 33, 33, 33, 33, 33, 33, 33, 33,
   33, 30, 30, 87, 87, 87, 87, 87, 87,110,110,502,410,410,410,130,130,130,
  405,405,450, 18, 18,403, 12, 12, 12,139,139,139,139,139,139,139,139,139,
  139,152,152,139,139,404, 14, 14, 14, 14,397,397, 22, 22, 73, 73,164,164,
  164,160,167,167,160,170,170,160,173,173,160,176,176,160,179,179,160, 57,
   26, 26, 26, 28, 28, 28, 23, 23, 23, 24, 24, 24, 24, 24, 27, 27, 27, 25,
   25, 25, 25, 25, 25, 29, 29, 29, 29, 29,193,193,193,193,202,202,202,202,
  202,202,202,202,202,202,201,201,457,457,457,457,457,457,457,457,218,218,
  457,457,  7,  7,  7,  7,  7,  7,  6,  6,  6, 17, 17,213,213,214,214, 15,
   15, 15, 16, 16, 16, 16, 16, 16, 16, 16,504,  9,  9,  9,554,554,554,554,
  554,554,237,237,554,554,240,240,566,242,242,566,244,244,566,566,246,246,
  246,246,246,248,248, 31, 32, 32, 32, 32, 32, 32, 32, 32, 32, 32,263,263,
  263,263,257,257,257,257,257,270,270,257,257,257,276,276,276,257,257,257,
  257,257,257,257,257,257,287,287,257,257,257,257,257,257,257,257,257,257,
  300,300,300,257,257,257,257,257,257,251,251,252,252,249,249,249,249,249,
  249,249,249,249,249,249,250,250,250,324,254,326,255,256,253,253,253,253,
  253,253,253,253,253,253,253,253,253,253,253,350,350,350,350,350,253,253,
  253,253,359,359,359,359,359,253,253,253,253,253,253,253,369,369,369,253,
  372,372,253,253,253,253,253,253,253,253,253,135,329,150,221, 42, 42, 42,
   42, 42, 42, 42, 42, 42, 42, 42, 42, 42, 42, 42, 42, 42, 42, 42, 42, 42,
   42, 42, 42, 42, 42, 42, 42, 42, 42, 42, 42, 42, 42, 42, 42, 42, 42, 42,
   42, 42, 42, 42, 50, 50, 50, 50, 50, 50, 50, 50, 50, 50, 50, 50, 50, 50,
   50, 50, 50, 50, 50, 50, 50, 50, 50, 50, 50, 50, 50, 50, 50, 50, 50, 50,
   50, 50, 50, 50, 50, 50, 50, 50, 50, 50, 50, 50, 88, 88, 88, 88, 88, 88,
   88, 88, 88, 88, 88, 88, 88, 88, 88, 88, 88, 88, 88, 88, 88, 88, 88, 88,
   88, 88, 88, 88, 88, 88, 88, 88, 88, 88, 88, 88, 88, 88, 88, 88, 89, 89,
   89, 89, 89, 89, 89, 89, 89, 89, 89, 89, 89, 89, 89, 89, 89, 89, 89, 89,
   89, 89, 89, 89, 89, 89, 89, 89, 89, 89, 89, 89, 89, 89, 89, 89, 89, 89,
   89, 89, 89,133,133,133,133,133,133,133,133,133,133,133,133,133,134,134,
  134,134,134,134,134,134,134,134,134,134,134,134,136,136,136,136,136,136,
  136,136,136,136,136,136,136,136,136,136,136,136,136,141,141,141,141,141,
  141,141,141,141,141,141,141,141,141,141,141,141,141,141,141,141,141,141,
  141,141,141,141,141,141,141,141,141,141,141,141,141,141,141,141,141,141,
  148,148,148,149,149,149,149,149,153,153,153,153,153,153,153,153,153,156,
  156,156,156,156,156,156,156,156,156,156,156,156,156,156,156,156,156,156,
  156,156,156,156,156,156,156,156,156,156,156,156,156,156,156,156,156,156,
  156,156,156,157,157,157,157,157,157,157,157,157,157,157,157,157,157,157,
  157,157,157,157,157,157,157,157,157,157,157,157,157,157,157,157,157,157,
  157,157,157,157,157,157,157,157,157,157,222,222,222,222,223,223,225,225,
  225,225,227,227,227,227,227,227,227,227,227,227,227,227,227,227,227,227,
  227,227,227,227,227,227,227,227,227,227,227,227,227,227,227,227,227,227,
  227,227,227,227,227,227,227, 45, 41, 46, 48, 49, 11, 54, 56, 58, 59, 60,
   10, 13, 19, 65, 68, 67, 69, 21, 70, 72, 74, 75, 76, 77, 78, 79, 80, 85,
   84, 83, 90, 92, 93, 94, 95, 96, 98, 99,100,102,103,104,106,107,109,111,
  112,113,114,115,116,117,119,118,120,121,122, 20,123,124,125,126,127,128,
    2,129,159,161,162,163,165,166,168,169,171,172,174,175,177,178,180,181,
  182,183,184,185,186,187,188,189,190,191,192,194,195,196,197,198,199,200,
  203,204,205,206,207,208,209,210,211,  3,212,  8,247,238,258,259,260,261,
  262,264,265,266,267,268,269,271,272,273,274,275,277,278,279,280,281,282,
  283,284,285,286,288,289,290,291,292,293,294,295,296,297,298,299,301,302,
  303,304,305,306,307,308,309,  4,310,311,312,313,314,315,316,317,318,319,
  320,  5,321,322,323,325,327,328,330,331,332,333,334,335,336,337,338,339,
  340,341,342,343,344,345,346,347,348,349,351,352,353,354,355,356,357,358,
  360,361,362,363,364,365,366,367,368,370,371,373,374,375,376,377,378,379,
  380,382,138,383,151,137,145,384,385,386,387,155,154,388,224,389,146,219,
  147,390,215,142,144,143,381,140,220,391
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
    case 26: ag_rp_26(V(0,int)); break;
    case 27: ag_rp_27(V(1,int)); break;
    case 28: ag_rp_28(); break;
    case 29: ag_rp_29(); break;
    case 30: ag_rp_30(); break;
    case 31: ag_rp_31(V(2,int)); break;
    case 32: ag_rp_32(); break;
    case 33: ag_rp_33(); break;
    case 34: ag_rp_34(); break;
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
    case 64: ag_rp_64(V(2,int)); break;
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
    case 75: ag_rp_75(V(0,int)); break;
    case 76: ag_rp_76(V(1,int)); break;
    case 77: ag_rp_77(V(1,int)); break;
    case 78: ag_rp_78(V(0,int)); break;
    case 79: ag_rp_79(V(1,int)); break;
    case 80: ag_rp_80(V(1,int), V(2,int)); break;
    case 81: ag_rp_81(V(1,int)); break;
    case 82: ag_rp_82(); break;
    case 83: ag_rp_83(V(1,int)); break;
    case 84: V(0,int) = ag_rp_84(V(0,int)); break;
    case 85: V(0,int) = ag_rp_85(); break;
    case 86: V(0,int) = ag_rp_86(); break;
    case 87: V(0,int) = ag_rp_87(); break;
    case 88: V(0,int) = ag_rp_88(); break;
    case 89: V(0,int) = ag_rp_89(); break;
    case 90: V(0,int) = ag_rp_90(); break;
    case 91: V(0,int) = ag_rp_91(); break;
    case 92: V(0,int) = ag_rp_92(); break;
    case 93: V(0,int) = ag_rp_93(V(1,int), V(2,int), V(3,int)); break;
    case 94: V(0,int) = ag_rp_94(V(2,int), V(3,int)); break;
    case 95: V(0,int) = ag_rp_95(V(2,int)); break;
    case 96: ag_rp_96(); break;
    case 97: ag_rp_97(); break;
    case 98: ag_rp_98(V(1,int)); break;
    case 99: ag_rp_99(V(2,int)); break;
    case 100: ag_rp_100(); break;
    case 101: ag_rp_101(); break;
    case 102: V(0,int) = ag_rp_102(); break;
    case 103: V(0,int) = ag_rp_103(); break;
    case 104: ag_rp_104(); break;
    case 105: ag_rp_105(); break;
    case 106: ag_rp_106(); break;
    case 107: ag_rp_107(); break;
    case 108: ag_rp_108(); break;
    case 109: ag_rp_109(); break;
    case 110: ag_rp_110(); break;
    case 111: ag_rp_111(); break;
    case 112: ag_rp_112(); break;
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
    case 133: ag_rp_133(V(0,double)); break;
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
    case 145: V(0,double) = ag_rp_145(V(0,int)); break;
    case 146: V(0,double) = ag_rp_146(V(0,double)); break;
    case 147: V(0,int) = ag_rp_147(V(0,int)); break;
    case 148: V(0,int) = ag_rp_148(); break;
    case 149: V(0,int) = ag_rp_149(); break;
    case 150: V(0,int) = ag_rp_150(); break;
    case 151: V(0,int) = ag_rp_151(); break;
    case 152: V(0,int) = ag_rp_152(); break;
    case 153: V(0,int) = ag_rp_153(); break;
    case 154: V(0,int) = ag_rp_154(); break;
    case 155: V(0,int) = ag_rp_155(); break;
    case 156: V(0,int) = ag_rp_156(); break;
    case 157: ag_rp_157(V(1,int)); break;
    case 158: ag_rp_158(V(1,int)); break;
    case 159: ag_rp_159(V(0,int)); break;
    case 160: ag_rp_160(V(1,int)); break;
    case 161: ag_rp_161(V(2,int)); break;
    case 162: ag_rp_162(V(1,int)); break;
    case 163: ag_rp_163(V(1,int)); break;
    case 164: ag_rp_164(V(1,int)); break;
    case 165: ag_rp_165(V(1,int)); break;
    case 166: ag_rp_166(V(1,int)); break;
    case 167: ag_rp_167(V(1,int)); break;
    case 168: ag_rp_168(V(1,int)); break;
    case 169: ag_rp_169(V(1,int)); break;
    case 170: V(0,int) = ag_rp_170(V(1,int)); break;
    case 171: V(0,int) = ag_rp_171(V(1,int), V(2,int)); break;
    case 172: V(0,int) = ag_rp_172(); break;
    case 173: V(0,int) = ag_rp_173(V(0,int)); break;
    case 174: V(0,int) = ag_rp_174(); break;
    case 175: V(0,int) = ag_rp_175(); break;
    case 176: V(0,int) = ag_rp_176(); break;
    case 177: V(0,int) = ag_rp_177(); break;
    case 178: V(0,int) = ag_rp_178(); break;
    case 179: V(0,int) = ag_rp_179(); break;
    case 180: V(0,int) = ag_rp_180(); break;
    case 181: V(0,double) = ag_rp_181(); break;
    case 182: V(0,double) = ag_rp_182(V(1,int)); break;
    case 183: V(0,double) = ag_rp_183(V(0,double), V(1,int)); break;
    case 184: ag_rp_184(); break;
    case 185: ag_rp_185(); break;
    case 186: ag_rp_186(); break;
    case 187: ag_rp_187(); break;
    case 188: ag_rp_188(); break;
    case 189: ag_rp_189(); break;
    case 190: ag_rp_190(); break;
    case 191: ag_rp_191(); break;
    case 192: ag_rp_192(); break;
    case 193: ag_rp_193(); break;
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
    case 256: ag_rp_256(V(2,int)); break;
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
    case 284: ag_rp_284(); break;
    case 285: ag_rp_285(); break;
    case 286: ag_rp_286(); break;
    case 287: ag_rp_287(); break;
    case 288: ag_rp_288(); break;
    case 289: ag_rp_289(); break;
    case 290: ag_rp_290(); break;
    case 291: ag_rp_291(); break;
    case 292: ag_rp_292(); break;
  }
}

#define TOKEN_NAMES a85parse_token_names
const char *const a85parse_token_names[647] = {
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
  "eight bit reg inst",
  "sixteen bit reg inst",
  "bd reg inst",
  "stack reg inst",
  "immediate operand inst",
  "lxi inst",
  "mvi inst",
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
  "\"STAX\"",
  "\"LDAX\"",
  "\"POP\"",
  "\"PUSH\"",
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
  "\"DAD\"",
  "\"DCX\"",
  "\"INX\"",
  "lxi inst start",
  "\"LXI\"",
  "mvi inst start",
  "\"MVI\"",
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
  "",
  "\"LHLD\"",
  "\"ORI\"",
  "\"OUT\"",
  "\"SBI\"",
  "\"SHLD\"",
  "\"STA\"",
  "\"SUI\"",
  "\"XRI\"",
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
  "\"STAX\"",
  "\"LDAX\"",
  "\"POP\"",
  "\"PUSH\"",
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
  "register 16 bit",
  "\"DAD\"",
  "\"DCX\"",
  "\"INX\"",
  "\"LXI\"",
  "\"MVI\"",
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
  "\"LHLD\"",
  "\"ORI\"",
  "\"OUT\"",
  "\"SBI\"",
  "\"SHLD\"",
  "\"STA\"",
  "\"SUI\"",
  "\"XRI\"",
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
    0,0,  0,0,  0,0, 64,1,397,1, 53,1, 35,1, 35,1, 35,1, 35,1, 35,1,  0,0,
   66,1, 66,1, 66,1,  0,0,  0,0, 66,1, 66,1,  0,0, 66,1, 66,1,  0,0, 66,1,
   66,1, 66,1, 66,1, 66,1, 66,1, 66,1, 66,1, 66,1, 66,1, 66,1,  0,0,  0,0,
   35,2, 47,1,  0,0, 39,1, 40,1, 39,1, 39,1, 35,2, 62,1,257,1,257,1,257,1,
  257,1,257,1,257,1,257,1,257,1,257,1,257,1,257,1,257,1,257,1,257,1,326,1,
  324,1,253,1,253,1,253,1,253,1,253,1,253,1,253,1,253,1,253,1,253,1,253,1,
  253,1,253,1,253,1,253,1,257,1,257,1,257,1,257,1,257,1,257,1,257,1,257,1,
  257,1,257,1,257,1,257,1,257,1,257,1,257,1,257,1,257,1,257,1,257,1,257,1,
  257,1,257,1,257,1,257,1,257,1,257,1,257,1,257,1,256,1,326,1,324,1,253,1,
  253,1,253,1,253,1,253,1,253,1,253,1,253,1,253,1,253,1,253,1,253,1,253,1,
  253,1,253,1,253,1,253,1,253,1,253,1,253,1,253,1,253,1,253,1,253,1,253,1,
  253,1,253,1,253,1,253,1,253,1,253,1,253,1,252,1,252,1,251,1,251,1,250,1,
  250,1,250,1,249,1,249,1,249,1,249,1,249,1,249,1,249,1,249,1,249,1,249,1,
  249,1,256,1,255,1,254,1,253,1,253,1,253,1,253,1,253,1,253,1,253,1,253,1,
  253,1,253,1,253,1,253,1,253,1,253,1,253,1,253,1,253,1,253,1,253,1,253,1,
  253,1,253,1,253,1,253,1,253,1,253,1,253,1,253,1,253,1,253,1,253,1,253,1,
  253,1,253,1,253,1,253,1,252,1,252,1,251,1,251,1,250,1,250,1,250,1,249,1,
  249,1,249,1,249,1,249,1,249,1,249,1,249,1,249,1,249,1,249,1,  0,0, 32,1,
   86,1, 30,1, 32,1, 33,1, 33,1, 35,2, 81,1, 66,2, 66,2,403,1,  0,0, 66,2,
   66,2, 66,2, 66,2,504,1,214,1,213,1,  6,1,  6,1,  7,1, 15,1,504,1,457,1,
  214,1,213,1,  6,1,201,1,202,1,202,1,202,1,202,1,202,1,202,1,202,1,202,1,
  202,1,202,1,201,1,  0,0,  0,0,202,1,202,1,202,1,202,1,202,1,202,1,202,1,
  202,1,202,1,202,1, 29,1,  0,0, 29,1,  0,0,193,1, 29,1, 29,1, 29,1, 29,1,
   25,1, 27,1, 24,1, 23,1, 28,1, 26,1, 26,1, 73,1, 73,1, 66,2, 66,2, 66,2,
    0,0, 66,2,  0,0, 66,2,404,1,  0,0, 66,2, 66,2,  0,0, 35,3,  0,0,  0,0,
    0,0,  0,0,  0,0,  0,0,  0,0,  0,0,  0,0,  0,0,  0,0,  0,0,  0,0,  0,0,
    0,0,  0,0,  0,0,  0,0,  0,0,  0,0,  0,0,  0,0,  0,0,  0,0,  0,0,  0,0,
    0,0,  0,0,  0,0,  0,0,  0,0,  0,0,  0,0,  0,0,  0,0,  0,0,  0,0,  0,0,
    0,0, 39,2, 39,2, 33,1, 35,3, 62,2,326,2,324,2,256,2,255,2,254,2,253,2,
  253,2,253,2,253,2,253,2,253,2,253,2,253,2,253,2,253,2,253,2,253,2,253,2,
  253,2,253,2,253,2,253,2,253,2,253,2,253,2,253,2,253,2,253,2,253,2,253,2,
  253,2,253,2,253,2,253,2,253,2,253,2,253,2,253,2,253,2,253,2,253,2,252,2,
  252,2,251,2,251,2,250,2,250,2,250,2,249,2,249,2,249,2,249,2,249,2,249,2,
  249,2,249,2,249,2,249,2,249,2, 86,2,  0,0,  0,0, 32,2,  0,0,  0,0, 33,2,
    0,0, 33,2,  0,0,  0,0,  0,0, 33,2,  0,0,  0,0,  0,0, 33,2,  0,0, 33,2,
    0,0, 33,2,  0,0, 33,2,  0,0, 33,2,  0,0, 33,2,  0,0, 33,2,  0,0, 33,2,
    0,0, 33,2,  0,0, 33,2,  0,0,  0,0, 33,2,  0,0,  0,0,  0,0, 33,2,  0,0,
    0,0, 33,2,  0,0,  0,0,  0,0,  0,0, 33,2,  0,0,  0,0, 35,3, 81,2, 66,3,
   66,3,  0,0, 66,3, 66,3,  6,2, 16,1, 15,2,  0,0,  0,0,202,2,202,2,202,2,
  202,2,202,2,202,2,202,2,202,2,202,2,202,2,193,2,  7,1,  0,0, 25,2,  0,0,
   25,2, 25,2, 25,2, 25,2, 27,2, 27,2,  0,0, 24,2,  0,0, 24,2,  0,0, 24,2,
    0,0, 24,2,  0,0, 23,2, 23,2,  0,0, 28,2, 28,2,  0,0, 26,2, 26,2,  0,0,
    0,0,  0,0,  0,0,  0,0,  0,0,  0,0,  0,0,  0,0,  0,0,  0,0,  0,0,  0,0,
   66,3, 66,3, 66,3, 66,3, 66,3,404,2, 66,3, 66,3,  0,0, 35,4, 35,4, 35,4,
    0,0, 35,4,  0,0, 35,4, 35,4, 22,1, 62,3,  0,0,  0,0,255,3,254,3,246,1,
  246,1,249,3,  0,0,450,1,  0,0,  0,0,  6,1,457,1, 32,3,450,1, 33,3, 33,3,
  110,1,110,1,110,1,  0,0,  0,0,  0,0, 81,3, 82,1,  0,0,  0,0,  0,0, 15,3,
  202,3,202,3,202,3,202,3,202,3,202,3,202,3,202,3,202,3,202,3,193,3, 25,3,
   25,3, 25,3, 25,3, 25,3, 27,3, 27,3, 24,3, 24,3, 24,3, 24,3, 23,3, 23,3,
   28,3, 28,3, 26,3, 26,3, 35,5, 35,5, 35,5, 35,5, 35,5, 62,4,255,4,254,4,
  249,4,  0,0,  0,0,  0,0, 32,1, 32,1,450,2, 33,4,  0,0,110,2,  0,0,  0,0,
   66,5,  0,0,  0,0,202,4,202,4,202,4,202,4,202,4,202,4,202,4,202,4,202,4,
  202,4,193,4, 25,1, 25,1, 27,1, 27,1, 27,1, 27,1, 24,1, 24,1, 23,1, 23,1,
   28,1, 28,1, 35,6, 35,6,249,5, 15,3,  0,0, 33,5,110,3,502,1,  0,0,202,5,
  249,6,202,6
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


