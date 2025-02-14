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
#include <signal.h>
#include <setjmp.h>


/* Implement the following function */
word_t vaddr_read(vaddr_t addr, int len);

enum {
  // Object
  TK_NOTYPE = 256, TK_NUM, TK_ADDR, TK_HEX, TK_REG,
  
  // Operator
  TK_PLUS, TK_SUB, TK_MUL, TK_DIV,
  TK_EQ, TK_MINUS, TK_DEREF, TK_NEQ, TK_AND, 
  
  // Quote
  TK_LQUOTE, TK_RQUOTE,
};

static struct rule {
  const char *regex;
  int token_type;
} rules[] = {

  {" +", TK_NOTYPE},    // spaces
  {"\\+", TK_PLUS},     // plus
  {"[0-9]+", TK_NUM},     // number
  {"-", TK_SUB},        // sub
  {"\\*", TK_MUL},      // mul
  {"/", TK_DIV},        // div
  {"\\(", TK_LQUOTE},     // left quote
  {"\\)", TK_RQUOTE},     // right quote

  {"$[a-zA-Z]+", TK_REG},
  {"!=", TK_NEQ},
  {"==", TK_EQ},        // equal
  {"&&", TK_AND},
  {"0x[0-9a-fA-F]+", TK_HEX},
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

static Token tokens[8192] __attribute__((used)) = {};
static int nr_token __attribute__((used))  = 0;

/* For token_match */
Token stack[8192]  = {};
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
          case TK_NOTYPE: continue;
          case TK_NUM: tokens[nr_token].type = TK_NUM; break;
          case TK_PLUS: tokens[nr_token].type = TK_PLUS; break;
          case TK_SUB: tokens[nr_token].type = TK_SUB; break;
          case TK_MUL: tokens[nr_token].type = TK_MUL; break;
          case TK_DIV: tokens[nr_token].type = TK_DIV; break;
          case TK_EQ: tokens[nr_token].type = TK_EQ; break;
          case TK_LQUOTE: tokens[nr_token].type = TK_LQUOTE; break;
          case TK_RQUOTE: tokens[nr_token].type = TK_RQUOTE; break;
          case TK_REG: tokens[nr_token].type = TK_REG; break;
          case TK_NEQ: tokens[nr_token].type = TK_NEQ; break;
          case TK_AND: tokens[nr_token].type = TK_AND; break;
          case TK_HEX: tokens[nr_token].type = TK_HEX; break;
          case TK_ADDR: tokens[nr_token].type = TK_ADDR; break;
          default: printf("Unknow token type\n"); return false;
        }

        /* copy token_str to tokens_arr */
        strncpy(tokens[nr_token].str, substr_start, substr_len);
        tokens[nr_token].str[substr_len] = '\0';
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


jmp_buf env;
int SIGFPE_flag = 0;

void sigsegv_handler(int signal) {
  printf("SIGFPE signal\n");
  SIGFPE_flag = 1;
  longjmp(env, 1);
}

int eval_expr(char* error){
  /* Wrong expr */
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
      // register SIGFPE signal
      struct sigaction sa = {};
      memset(&sa, 0, sizeof(sa));
      sa.sa_flags = SA_NODEFER;
      sa.sa_handler = sigsegv_handler;
      if (sigaction(SIGFPE, &sa, NULL) == -1) {
          perror("sigaction");
      }

      switch(stack[stack_top].type){
        case TK_NUM:
          /* Wrong expr */
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

                /* catch SIGFPE signal */
                if(0 == setjmp(env)){
                  rhs = lnum / rhs;
                }else{
                  if(SIGFPE_flag == 1){
                    strcat(error, "Divide by zero");
                    return -1;
                  }
                }

                break;
            }
          }

          /* reset  */
          has_r_num = 1;
          has_r_op = 0;
          break;
        case TK_PLUS: case TK_DIV: case TK_MUL: case TK_SUB:
          /* Wrong expr */
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
          /* Wrong expr */
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

    /* Wrong expr */
    if(has_r_op == 1){
      strcat(error, "Op is in the end");
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

  int sub_flag = 0;
  int space_flag = 0;

  for(int i = 0;i < nr_token;i++){
    /* match elem */
    switch (tokens[i].type)
    {
    case TK_NOTYPE:
      space_flag = 1;
      continue;;
    case TK_SUB:
      sub_flag = 1;
      stack[++stack_top] = tokens[i];
      continue;
    case TK_RQUOTE:
      int sub_expr_value = eval_expr(error);
      if(strcmp(error, "") != 0){
        return 0;
      }

      expr_value += sub_expr_value;
      
      break;
    case TK_NUM:
      if(!space_flag && sub_flag){
        char str[64] = "-";
        strcat(str, tokens[i].str);
        strncpy(tokens[i].str, str, 32);
        // replace the sub_op
        stack[stack_top] = tokens[i]; 
      }else{
        stack[++stack_top] = tokens[i]; 
      }

      break;
    default:
      stack[++stack_top] = tokens[i];

      break;
    }
    /* reset */
    sub_flag = 0;
    space_flag = 0;
  }

  expr_value = eval_expr(error);
  if(strcmp(error, "") != 0){
    return 0;
  }

  Log("Expr_value = %u", expr_value);
  return expr_value;
}


int check_parentheses(int l, int r){
  if(tokens[l].type != TK_LQUOTE){
    return 0;
  }

  while(l < r){
    if(tokens[l].type == TK_RQUOTE && l != r){
      return 0;
    }
    l++;
  }
  
  return 1;
}


word_t eval(char* error, int l, int r) {
  if(l > r){
    return 0;
  } 
  else if(l == r){
    int num = atoi(tokens[l].str);

    if(num == 0 && tokens[l].str[0] != '0'){
      strcat(error, "single elem is not number");
      return 0;
    }
    return num;
  }
  else if(check_parentheses(l , r)){
    return eval(error, l + 1, r - 1);
  }
  else{
    /* Find op */
    for(int i = l;i <= r;i++){
      int op = tokens[i].type;
      if(op == TK_PLUS || op == TK_MUL || op == TK_DIV || op == TK_SUB){
        int val1 = eval(error, l, i - 1);
        int val2 = eval(error, i + 1, r);
        
        int sub_expr_value;
        switch (op)
        {
        case TK_PLUS:
          sub_expr_value = val1 + val2;
          break;
        case TK_MUL:
          sub_expr_value = val1 * val2;
          break;
        case TK_DIV:
          sub_expr_value = val1 / val2;
          break;
        case TK_SUB:
          sub_expr_value = val1 - val2;
          break;
        default:
          assert(0);
        }
        Log("Sub_expr_value = %d", sub_expr_value);
        return sub_expr_value;
      }
    }

    if(tokens[l].type == TK_MINUS){
      if(l + 1 < r && tokens[l + 1].type == TK_NUM){
        return 0 - atoi(tokens[l + 1].str) + eval(error, l + 2, r);
      }
      return 0 - eval(error, l + 1, r);
    }

    if(tokens[l].type == TK_DEREF){
      return vaddr_read(eval(error, l + 1, r), 4);
    }
  }

  return 0;
}


word_t wp_expr(char *e, char* error){
  if (!make_token(e)) {
    strcat(error, "make_token failed");
    return 0;
  }

  for(int i = 0;i < nr_token;i++){
    int IsOp = (i == 0 || (TK_PLUS <= tokens[i - 1].type && tokens[i - 1].type <= TK_AND));

    if (tokens[i].type == TK_MUL && IsOp)  {
      tokens[i].type = TK_DEREF;
    }

    if (tokens[i].type == TK_SUB && IsOp){
      tokens[i].type = TK_MINUS;
    }
  }


  word_t expr_value = eval(error, 0, nr_token - 1);

  if(strcmp(error, "") != 0){
    return 0;
  }

  Log("Expr_value = %u", expr_value);
  return expr_value;
}





