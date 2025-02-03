/***************************************************************************************
* Copyright (c) 2014-2024 Zihao Yu, Nanjing University
*
* NEMU is licensed under Mulan PSL v2.
* You can use this software according to the terms and conditions of the Mulan PSL v2.
* You may obtain a copy of Mulan PSL v2 at:
*          http://license.coscl.org.cn/MulanPSL2
*
* THIS SOFTWARE IS PROVIDED ON AN "AS IS" BASIS, WITHOUT WARRANTIES OF ANY KIND,
* EITHER EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO NON-INFRINGEMENT,
* MERCHANTABILITY OR FIT FOR A PARTICULAR PURPOSE.
*
* See the Mulan PSL v2 for more details.
***************************************************************************************/

#include <isa.h>

/* We use the POSIX regex functions to process regular expressions.
 * Type 'man regex' for more information about POSIX regex functions.
 */
#include <regex.h>
#include <string.h>
#include <stdlib.h>

enum {
  TK_NOTYPE = 256, TK_NUM, TK_PLUS, TK_SUB, 
  TK_MUL, TK_DIV, TK_EQ, TK_LQUOTE, TK_RQUOTE,

};

static struct rule {
  const char *regex;
  int token_type;
} rules[] = {

  {" +", TK_NOTYPE},    // spaces
  {"\\+", TK_PLUS},     // plus
  {"==", TK_EQ},        // equal
  {"[0-9]+", TK_NUM},     // number
  {"-", TK_SUB},        // sub
  {"\\*", TK_MUL},      // mul
  {"/", TK_DIV},        // div
  {"\\(", TK_LQUOTE},     // left quote
  {"\\)", TK_RQUOTE},     // right quote
  {""}
};

#define NR_REGEX ARRLEN(rules)

static regex_t re[NR_REGEX] = {};

/* Rules are used for many times.
 * Therefore we compile them only once before any usage.
 */
void init_regex() {
  int i;
  char error_msg[128];
  int ret;

  for (i = 0; i < NR_REGEX; i ++) {
    ret = regcomp(&re[i], rules[i].regex, REG_EXTENDED);
    if (ret != 0) {
      regerror(ret, &re[i], error_msg, 128);
      panic("regex compilation failed: %s\n%s", error_msg, rules[i].regex);
    }
  }
}

typedef struct token {
  int type;
  char str[32];
} Token;

static Token tokens[4096] __attribute__((used)) = {};
static int nr_token __attribute__((used))  = 0;

/* For token_match */
Token stack[4096]  = {};
int stack_top  = -1;


static bool make_token(char *e) {
  int position = 0;
  int i;
  regmatch_t pmatch;

  nr_token = 0;

  while (e[position] != '\0') {
    /* Try all rules one by one. */
    for (i = 0; i < NR_REGEX; i++) {
      if(e[position] == '\0'){
        break;
      }

      if (regexec(&re[i], e + position, 1, &pmatch, 0) == 0 && pmatch.rm_so == 0) {
        char *substr_start = e + position;
        int substr_len = pmatch.rm_eo;

        Log("match rules[%d] = \"%s\" at position %d with len %d: %.*s",
            i, rules[i].regex, position, substr_len, substr_len, substr_start);

        position += substr_len;

        switch (rules[i].token_type) {
          case TK_NOTYPE: continue;     // ignore spaces
          case TK_NUM: tokens[nr_token].type = TK_NUM; break;
          case TK_PLUS: tokens[nr_token].type = TK_PLUS; break;
          case TK_SUB: tokens[nr_token].type = TK_SUB; break;
          case TK_MUL: tokens[nr_token].type = TK_MUL; break;
          case TK_DIV: tokens[nr_token].type = TK_DIV; break;
          case TK_EQ: tokens[nr_token].type = TK_EQ; break;
          case TK_LQUOTE: tokens[nr_token].type = TK_LQUOTE; break;
          case TK_RQUOTE: tokens[nr_token].type = TK_RQUOTE; break;
          default: printf("Unknow token type\n"); return false;
        }

        /* copy token_str to tokens_arr */
        strncpy(tokens[nr_token].str, substr_start, substr_len);

        nr_token++;
        break;
      }
    }

    if (i == NR_REGEX) {
      printf("no match at position %d\n%s\n%*.s^\n", position, e, position, "");
      return false;
    }
  }

  return true;
}

int eval_expr(char* error){
  // Stack_top start with -1
  if(stack_top < 0){
    strcat(error, "stack is empty ");
    return -1;
  }

  // First elem
  char* first = stack[stack_top--].str;
  int rhs = atoi(first);

  // First elem is not number
  if(rhs == 0 && first[0] != '0'){
    strcat(error, "Elem in the end isn't number");
    return -1;
  }


  int rop = -1;
  
  int has_r_num = 1;
  int has_r_op = 0;
  
  Token Num; 
  /* l op r
    num --  has_r_op  (true) (num op.type rhs) -> rhs 
        
        --  has_r_num (true)  return -1
        
        reset: 
          has_r_num -> 1
          has_r_op -> 0

    op  --  has_r_op  (true)  return -1

        --  has_r_num (true)  
                              rop = op.type
                      (false)
                              return -1
        reset:
          has_r_num = 0
          has_r_op = 1
  */ 

  while(stack_top >= 0){
      switch(stack[stack_top].type){
        case TK_NUM:

          if(has_r_num == 1){
            strcat(error, "num next to num");
            return -1;
          }

          if(has_r_op == 1){
            int lnum = atoi(stack[stack_top].str);
            /* Diff ops */
            switch(rop){
              case TK_PLUS:
                rhs = lnum + rhs;
                break;
              case TK_SUB:
                rhs = lnum - rhs;
                break;
              case TK_MUL:
                rhs = lnum * rhs;
                break;
              case TK_DIV:
                rhs = lnum / rhs;
                break;
            }
          }

          /* reset  */
          has_r_num = 1;
          has_r_op = 0;
          break;
        case TK_PLUS: case TK_DIV: case TK_MUL: case TK_SUB:

          if(has_r_num == 0 || has_r_op == 1){
            strcat(error, "op next to op OR op next to nothing");
            return -1;
          }

          rop = stack[stack_top].type;

          /* reset */
          has_r_num = 0;  
          has_r_op = 1;
          break;
        
        /* optional quote match_end*/
        case TK_LQUOTE:
          /* Op is in the end */
          if(has_r_op == 1){
            strcat(error, "op next to op OR op next to nothing");
            return -1;
          }

          /* push newElem (replace Lquote)*/
          snprintf(Num.str, sizeof(Num.str), "%d", rhs);
          Num.type = TK_NUM;
          
          stack[stack_top] = Num;

          Log("Quote_sub_expr_value = %d", rhs);
          return rhs; 
      }
      
      
      // Iter to elems 
      stack_top--;
    }

    /* Op is in the end */
    if(has_r_op == 1){
      return -1;
    }
  
    /* push newElem */
    snprintf(Num.str, sizeof(Num.str), "%d", rhs);
    Num.type = TK_NUM;
    
    stack[++stack_top] = Num;

    Log("Sub_expr_value = %d", rhs);
    return rhs; 
}

word_t expr(char *e, char* error) {
  if (!make_token(e)) {
    strcat(error, "make_token failed");
    return 0;
  }

  // Init global variable
  stack_top = -1;

  unsigned expr_value = 0;
  int quote_flag = 0;


  /* With quote */
  for(int i = 0;i < nr_token;i++){

    /* quote_flag */
    if(quote_flag == 0 && tokens[i].type == TK_LQUOTE){
      quote_flag = 1;
    }

    /* not Rquote into stack */
    if(tokens[i].type != TK_RQUOTE){
      stack[++stack_top] = tokens[i];
    }
    else{
      int sub_expr_value = eval_expr(error);
      if(sub_expr_value == -1){
        strcat(error, "(With quote)");
        return 0;
      }
      expr_value += sub_expr_value;
    }
  }

  expr_value = eval_expr(error);
  if(expr_value == -1){
    strcat(error, "(Without quote)");
    return 0;
  }

  Log("Expr_value = %u", expr_value);
  return expr_value;
}
