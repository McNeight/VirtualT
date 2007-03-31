#ifndef A85PARSE_H_1147178743
#define A85PARSE_H_1147178743

typedef union {
  long alignment;
  char ag_vt_2[sizeof(int)];
  char ag_vt_4[sizeof(double)];
} a85parse_vs_type;

typedef enum {
  a85parse_WS_token = 1, a85parse_cstyle_comment_token,
  a85parse_hex_integer_token = 7, a85parse_decimal_integer_token,
  a85parse_simple_real_token = 10, a85parse_string_chars_token = 13,
  a85parse_include_chars_token = 15, a85parse_ascii_integer_token,
  a85parse_escape_char_token, a85parse_asm_hex_value_token,
  a85parse_singlequote_chars_token, a85parse_a85parse_token = 22,
  a85parse_statement_token, a85parse_eof_token = 26, a85parse_comment_token,
  a85parse_any_text_char_token = 29, a85parse_newline_token = 32,
  a85parse_expression_token = 41, a85parse_equation_token = 55,
  a85parse_expression_list_token = 64, a85parse_name_list_token = 67,
  a85parse_condition_token = 74, a85parse_instruction_list_token = 82,
  a85parse_literal_alpha_token = 84, a85parse_digit_token = 87,
  a85parse_asm_incl_char_token, a85parse_string_char_token = 91,
  a85parse_condition_start_token = 98,
  a85parse_inclusive_or_exp_token = 118, a85parse_exclusive_or_exp_token,
  a85parse_and_exp_token = 122, a85parse_shift_exp_token = 125,
  a85parse_additive_exp_token = 128,
  a85parse_multiplicative_exp_token = 133, a85parse_urinary_exp_token = 136,
  a85parse_primary_exp_token = 138, a85parse_value_token = 146,
  a85parse_function_token, a85parse_hex_digit_token = 166,
  a85parse_stack_register_token = 181, a85parse_bd_register_token = 183,
  a85parse_instruction_token, a85parse_rst_arg_token = 260,
  a85parse_label_token = 289, a85parse_literal_string_token,
  a85parse_include_string_token = 292, a85parse_asm_include_token,
  a85parse_literal_name_token = 298,
  a85parse_singlequote_string_token = 323, a85parse_integer_token = 325,
  a85parse_real_token = 373, a85parse_register_8_bit_token = 380,
  a85parse_register_16_bit_token = 401
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
  int (* const  *gt_procs)(void);
  int (* const *r_procs)(void);
  int (* const *s_procs)(void);
  int lab[9], rx, fx;
  const unsigned char *key_sp;
  int save_index, key_state;
  int ag_error_depth, ag_min_depth, ag_tmp_depth;
  int ag_rss[2*128], ag_lrss;
  char ag_msg[82];
  int ag_resynch_active;
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

