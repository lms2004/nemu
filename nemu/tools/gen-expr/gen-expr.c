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

#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <time.h>
#include <assert.h>
#include <string.h>

// this should be enough
static char buf[65536] = {};
static char code_buf[65536 + 1024] = {}; // a little larger than `buf`
static char *code_format =
    "#include <stdio.h>\n"
    "#include <signal.h>\n"
    "#include <setjmp.h>\n"
    "#include <stdlib.h>\n\n"  // Add this line
    "jmp_buf env;\n"
    "void handler(int signal) {\n"
    "    longjmp(env, 2);\n"
    "}\n\n"
    "int main(int argc, char *argv[]) {\n"
    "    {\n"
    "        struct sigaction sa = {};\n"
    "        sa.sa_handler = handler;\n"
    "        if (sigaction(SIGFPE, &sa, NULL) == -1) {\n"
    "            perror(\"sigaction\");\n"
    "        }\n"
    "    }\n\n"
    "    if (0 == setjmp(env)) {\n"
    "        unsigned result = %s;  // Example character constant\n"
    "        printf(\"%%u\", result);  // Print the result\n"
    "    } else {\n"
    "        printf(\"%%u\", 65535);\n"
    "    }\n\n"
    "    return 1;\n"
    "}";


int buf_i = 0;

static void gen(char c) {
  buf[buf_i++] = c;
}

static void gen_num() {
  int f = rand() % 10;
  if (f != 0) {
    buf[buf_i++] = f + '0';
  }else{
    gen('0');
    return ;
  }

  for (int i = 0; i < rand() % 2; i++) {
    buf[buf_i++] = rand() % 10 + '0';
  }
}

static void gen_space() {
  for (int i = 0; i < rand() % 3; i++) {
    buf[buf_i++] = ' ';
  }
}

static void gen_op() {
  switch (rand() % 4) {
    case 0:
      buf[buf_i] = '+';
      break;
    case 1:
      buf[buf_i] = '-';
      break;
    case 2:
      buf[buf_i] = '*';
      break;
    default:
      buf[buf_i] = '/';
      break;
  }
  buf_i++;
}

static void gen_rand_expr() {
  if (buf_i > 3000) {
    gen_num();
    return;
  }
  gen_space();
  switch (rand() % 3) {
    case 0:
      gen_num();
      break;
    case 1:
      gen('(');
      gen_rand_expr();
      gen_op();
      gen_rand_expr();
      gen(')');
      break;
    case 2:
      gen('(');
      gen_rand_expr();
      gen(')');
      break;
  }
}

int main(int argc, char *argv[]) {
  int seed = time(0);
  srand(seed);
  int loop = 1;
  if (argc > 1) {
    sscanf(argv[1], "%d", &loop);
  }
  int i;
  for (i = 0; i < loop; i++) {
    gen_rand_expr();
    buf[buf_i] = '\0';

    snprintf(code_buf, sizeof(code_buf), code_format, buf);

    FILE *fp = fopen("/tmp/.code.c", "w");
    assert(fp != NULL);
    fputs(code_buf, fp);
    fclose(fp);

    int ret = system("gcc /tmp/.code.c -o /tmp/.expr");
    if (ret != 0) continue;

    fp = popen("/tmp/.expr", "r");
    assert(fp != NULL);

    int result;
    ret = fscanf(fp, "%d", &result);
    pclose(fp);

    buf_i = 0;

    if (result == 65535) {
      loop++;
      continue;
    }

    printf("%u %s\n", result, buf);
  }
  return 0;
}
