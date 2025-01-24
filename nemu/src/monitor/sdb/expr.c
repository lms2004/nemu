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

static Token tokens[32] __attribute__((used)) = {};
static int nr_token __attribute__((used))  = 0;

/* For token_match */
static Token stack[32] __attribute__((used)) = {};
static int stack_top __attribute__((used))  = 0;


static bool make_token(char *e) {
  int position = 0;
  int i;
  regmatch_t pmatch;

  nr_token = 0;

  while (e[position] != '\0') {
    /* Try all rules one by one. */
    for (i = 0; i < NR_REGEX; i++) {
      if (regexec(&re[i], e + position, 1, &pmatch, 0) == 0 && pmatch.rm_so == 0) {
        char *substr_start = e + position;
        int substr_len = pmatch.rm_eo;

        Log("match rules[%d] = \"%s\" at position %d with len %d: %.*s",
            i, rules[i].regex, position, substr_len, substr_len, substr_start);

        position += substr_len;

        switch (rules[i].token_type) {
          case TK_NOTYPE: tokens[nr_token].type = TK_NOTYPE; break;
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


word_t expr(char *e, bool *success) {
  if (!make_token(e)) {
    *success = false;
    return 0;
  }

  // int expr_value = 0;

  for(int i = 0;i < nr_token;i++){

  }

  return 0;
}
