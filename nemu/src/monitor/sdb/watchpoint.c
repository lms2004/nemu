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

#include "sdb.h"

#define NR_WP 32

typedef struct watchpoint {
  int NO;
  struct watchpoint *next;
  int hits;
  char expr[32];
  
  word_t stable_value;

} WP;

static WP wp_pool[NR_WP] = {};
static WP *head = NULL, *free_ = NULL;

void init_wp_pool() {
  int i;
  for (i = 0; i < NR_WP; i ++) {
    wp_pool[i].NO = i;
    wp_pool[i].next = (i == NR_WP - 1 ? NULL : &wp_pool[i + 1]);
    wp_pool[i].hits = 0;
    wp_pool[i].expr[0] = '\0';
    wp_pool[i].stable_value = 0;
  }

  head = NULL;
  free_ = wp_pool;
}


void wp_display() {
  if(head == NULL) {
    printf("No watchpoint.\n");
    return;
  }
  printf("Num     Type                    What\n");
  while(head != NULL) {
    printf("%d      hw watchpoint           %s\n", head->NO, head->expr);
    head = head->next;
  }
}

WP* new_wp(char* args) {
  if(free_ == NULL) {
    printf("No enough watchpoint.\n");
    assert(0);
  } 
  // pop free_ table
  WP *wp = free_;
  free_ = free_->next;

  /* 
  * set watchpoint
  *   stable_value 
  *   raw_expr
  */
  strcpy(wp->expr, args);

  char* error = calloc(128, sizeof(char));
  wp->stable_value = wp_expr(args, error);

  // push to head table
  if(head == NULL){
    head = wp;
    wp->next = NULL;
  } else {
    head->next = wp;
    wp->next = NULL;
  }
  return wp;
}

void free_wp(WP *wp){
  WP* curr = head;
  WP* prev = NULL;

  // find the watchpoint
  while(curr != wp){
    prev = curr;
    curr = curr->next;
  }

  if(curr == NULL){
    printf("No such watchpoint to free\n");
    return;
  }

  // whether the watchpoint has prev
  if(curr == head){
    head = NULL;
  }else{
    prev->next = curr->next;
  }

  // push to free_ table
  curr->next = free_;
  free_ = curr;
}


int scan_wp(){
  WP* curr = head;
  int flag = 0;
  while(curr != NULL){
    char* error = calloc(128, sizeof(char));
    word_t expr_value = wp_expr(curr->expr, error);
    if(expr_value != curr->stable_value){
      curr->stable_value = expr_value;
      flag = 1;
    }
    curr = curr->next;
  }
  return flag;
}