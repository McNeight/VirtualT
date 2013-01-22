#ifndef A85PARSE_H_1357391036
#define A85PARSE_H_1357391036

typedef union {
  long alignment;
  char ag_vt_2[sizeof(int)];
  char ag_vt_4[sizeof(double)];
} a85parse_vs_type;

typedef enum {
  a85parse_WS_token = 1, a85parse_hex_integer_token = 6,
  a85parse_decimal_integer_token, a85parse_simple_real_token = 9,
  a85parse_string_chars_token = 12, a85parse_include_chars_token = 14,
  a85parse_ascii_integer_token, a85parse_escape_char_token,
  a85parse_asm_hex_value_token, a85parse_singlequote_chars_token,
  a85parse_pagespec_token = 22, a85parse_and_exp_token,
  a85parse_shift_exp_token, a85parse_multiplicative_exp_token,
  a85parse_inclusive_or_exp_token, a85parse_additive_exp_token,
  a85parse_exclusive_or_exp_token, a85parse_urinary_exp_token,
  a85parse_page_exp_token, a85parse_instruction_list_token,
  a85parse_instruction_token, a85parse_expression_token,
  a85parse_a85parse_token, a85parse_statement_token,
  a85parse_eof_token = 38, a85parse_comment_token,
  a85parse_cstyle_comment_token, a85parse_any_text_char_token = 42,
  a85parse_cstyle_comment_head_token = 47, a85parse_preproc_inst_token = 53,
  a85parse_equation_token = 57, a85parse_cdseg_statement_token = 62,
  a85parse_error_token, a85parse_preproc_start_token,
  a85parse_preprocessor_directive_token = 66, a85parse_condition_token = 76,
  a85parse_macro_definition_token = 84, a85parse_macro_expansion_token,
  a85parse_macro_token = 89, a85parse_expression_list_token,
  a85parse_define_chars_token = 92,
  a85parse_cdseg_statement_start_token = 94, a85parse_name_list_token = 113,
  a85parse_literal_alpha_token = 133, a85parse_digit_token = 138,
  a85parse_asm_incl_char_token, a85parse_str_escape_char_token = 142,
  a85parse_hex_digit_token = 156, a85parse_condition_start_token = 166,
  a85parse_primary_exp_token = 199, a85parse_value_token = 207,
  a85parse_function_token, a85parse_binary_integer_token = 219,
  a85parse_octal_integer_token, a85parse_stack_register_token = 252,
  a85parse_bd_register_token = 254, a85parse_page_register_token,
  a85parse_eight_bit_reg_inst_token = 258,
  a85parse_sixteen_bit_reg_inst_token, a85parse_bd_reg_inst_token,
  a85parse_stack_reg_inst_token, a85parse_immediate_operand_inst_token,
  a85parse_lxi_inst_token, a85parse_mvi_inst_token, a85parse_spi_inst_token,
  a85parse_rst_inst_token, a85parse_no_arg_inst_token,
  a85parse_lxi_inst_start_token = 339, a85parse_mvi_inst_start_token = 341,
  a85parse_spi_inst_start_token = 343, a85parse_rst_arg_token = 346,
  a85parse_label_token = 430, a85parse_literal_string_token = 436,
  a85parse_include_string_token, a85parse_asm_include_token,
  a85parse_literal_name_nows_token = 443,
  a85parse_parameter_list_token = 457,
  a85parse_singlequote_string_token = 486, a85parse_integer_token = 493,
  a85parse_literal_name_token = 538, a85parse_real_token = 540,
  a85parse_register_8_bit_token = 596, a85parse_register_16_bit_token = 610
} a85parse_token_type;

typedef struct a85parse_pcb_struct{
  a85parse_token_type token_number, reduction_token, error_frame_token;
  int input_code;
  int input_value;
  int line, column;
  int ssx, sn, error_frame_ssx;
  int drt, dssx, dsn;
  int ss[128];
  a85parse_vs_type vs[128];
  int ag_ap;
  char *error_message;
  char read_flag;
  char exit_flag;
  int bts[128], btsx;
  int lab[9], rx, fx;
  const unsigned char *key_sp;
  int save_index, key_state;
  char ag_msg[82];
} a85parse_pcb_type;

#ifndef PRULE_CONTEXT
#define PRULE_CONTEXT(pcb)  (&((pcb).cs[(pcb).ssx]))
#define PERROR_CONTEXT(pcb) ((pcb).cs[(pcb).error_frame_ssx])
#define PCONTEXT(pcb)       ((pcb).cs[(pcb).ssx])
#endif

#ifndef AG_RUNNING_CODE_CODE
/* PCB.exit_flag values */
#define AG_RUNNING_CODE         0
#define AG_SUCCESS_CODE         1
#define AG_SYNTAX_ERROR_CODE    2
#define AG_REDUCTION_ERROR_CODE 3
#define AG_STACK_ERROR_CODE     4
#define AG_SEMANTIC_ERROR_CODE  5
#endif

extern a85parse_pcb_type a85parse_pcb;
void init_a85parse(void);
void a85parse(void);
#endif

