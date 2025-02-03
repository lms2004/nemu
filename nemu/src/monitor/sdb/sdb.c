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
#include <cpu/cpu.h>
#include <readline/readline.h>
#include <readline/history.h>
#include "sdb.h"

static int is_batch_mode = false;

void init_regex();
void init_wp_pool();
void wp_display();
word_t vaddr_read(vaddr_t addr, int len);

/* We use the `readline' library to provide more flexibility to read from stdin. */
static char* rl_gets() {
  static char *line_read = NULL;

  if (line_read) {
    free(line_read);
    line_read = NULL;
  }

  line_read = readline("(nemu) ");

  if (line_read && *line_read) {
    add_history(line_read);
  }

  return line_read;
}

static int cmd_c(char *args) {
  cpu_exec(-1);
  return 0;
}

static int cmd_q(char *args) {
  return -1;
}

static int cmd_si(char* args);

static int cmd_info(char* args);

static int cmd_x(char* args);

static int cmd_p(char *args);

static int cmd_test(char* path);

static int cmd_help(char *args);



static struct {
  const char *name;
  const char *description;
  int (*handler) (char *);
} cmd_table [] = {
  { "help", "Display information about all supported commands", cmd_help },
  { "c", "Continue the execution of the program", cmd_c },
  { "q", "Exit NEMU", cmd_q },
  { "si", "Step execute", cmd_si},
  { "info", "Print program information", cmd_info},
  { "x", "Scan memory", cmd_x},
  { "p", "eval expr", cmd_p},
  { "test", "test command", cmd_test}
};

#define NR_CMD ARRLEN(cmd_table)

static int cmd_si(char* args){
  if(args == NULL){
    /* no argument given */
    printf("si need args, eg. si 10\n");
  }else{
    char *arg = strtok(NULL, " ");
    int N = atoi(arg);

    cpu_exec(N);
  }
  return 0;
}

static int cmd_info(char* args){
  if(args == NULL){
    /* no argument given */
    printf("info need args, eg. info r\n");
  }else{
    char *arg = strtok(NULL, " ");
    /* print regs info */
    if(strcmp(arg, "r") == 0){
      isa_reg_display();
    }
    else if(strcmp(arg, "w") == 0){
    /* print watchpoint info */
      wp_display();
    }
    else{
      printf("Unknown command '%s'\n", arg);
    }
  }
  return 0;

}

static int cmd_x(char* args){
  if(args == NULL){
    /* no argument given */
    printf("x command:  need N and ($reg | addr), eg. x 10 $esp\n");
  }else{
    char *arg = strtok(NULL, " ");
    int N = atoi(arg);

    /* check addr */
    if((arg = strtok(NULL, " ")) == NULL){
      printf("x command:  need N and ($reg | addr), eg. x 10 $esp\n");
      return 0;
    }

    vaddr_t addr;
    /* parse addr */
    if(arg[0] == '$'){
      /* get reg value */
      bool success = false;
      addr = isa_reg_str2val(arg + 1, &success);

      if(!success){
        printf("Invalid reg name\n");
        return 0;
      }

    }else {
      /* get addr value */
      addr = strtol(arg, NULL, 16);
    }

    /* scan memory */
    for(int i = 0;i < N;i++){
      printf("%x: %x ",addr + 4*i, vaddr_read(addr, 4));
      if(i % 4 == 3) printf("\n");
    }printf("\n");

  }
  return 0;
}

static int cmd_p(char *args){
  if(args == NULL){
    /* no argument given */
    printf("p command:  eg. p (expr) \n");
    return 0;
  }
  args = strtok(NULL, "");

  char* error = calloc(128, sizeof(char));

  expr(args, error);
  

  if(strcmp(error, "") != 0){
    Error("%s", error);
  }

  return 0;
}

static int cmd_test(char* path){
  Log("Test cases: %s", path);
  FILE *fp = fopen(path, "r");
  int case_i = 0;

  char* args = calloc(128, sizeof(char));
  char* R_expr_value = calloc(32, sizeof(char)); 

  while(fscanf(fp, "%s %s", R_expr_value, args) != EOF){
    Log(" Test case %d: %s", case_i++,args);

    char* error = calloc(128, sizeof(char));

    word_t R_value = atoi(R_expr_value);

    word_t expr_value = expr(args, error);

    if(strcmp(error, "") != 0){
      Error("Test_case %d: %s",  case_i, error);
      return 0;
    }

    if(R_value != expr_value){
      Error("Test_case %d: Not match: %u   %u", case_i, R_value, expr_value);
      return 0;
    }
  }

  return 0;
}


static int cmd_help(char *args) {
  /* extract the first argument */
  char *arg = strtok(NULL, " ");
  int i;

  if (arg == NULL) {
    /* no argument given */
    for (i = 0; i < NR_CMD; i ++) {
      printf("%s - %s\n", cmd_table[i].name, cmd_table[i].description);
    }
  }
  else {
    for (i = 0; i < NR_CMD; i ++) {
      if (strcmp(arg, cmd_table[i].name) == 0) {
        printf("%s - %s\n", cmd_table[i].name, cmd_table[i].description);
        return 0;
      }
    }
    printf("Unknown command '%s'\n", arg);
  }
  return 0;
}

void sdb_set_batch_mode() {
  is_batch_mode = true;
}

void sdb_mainloop() {
  if (is_batch_mode) {
    cmd_c(NULL);
    return;
  }

  for (char *str; (str = rl_gets()) != NULL; ) {
    char *str_end = str + strlen(str);

    /* extract the first token as the command */
    char *cmd = strtok(str, " ");
    if (cmd == NULL) { continue; }

    /* treat the remaining string as the arguments,
     * which may need further parsing
     */
    char *args = cmd + strlen(cmd) + 1;
    if (args >= str_end) {
      args = NULL;
    }

#ifdef CONFIG_DEVICE
    extern void sdl_clear_event_queue();
    sdl_clear_event_queue();
#endif

    int i;
    for (i = 0; i < NR_CMD; i ++) {
      if (strcmp(cmd, cmd_table[i].name) == 0) {
        if (cmd_table[i].handler(args) < 0) { return; }
        break;
      }
    }

    if (i == NR_CMD) { printf("Unknown command '%s'\n", cmd); }
  }
}

void init_sdb() {
  /* Compile the regular expressions. */
  init_regex();

  /* Initialize the watchpoint pool. */
  init_wp_pool();
}
