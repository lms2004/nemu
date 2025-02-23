#***************************************************************************************
# Copyright (c) 2014-2024 Zihao Yu, Nanjing University
#
# NEMU is licensed under Mulan PSL v2.
# You can use this software according to the terms and conditions of the Mulan PSL v2.
# You may obtain a copy of Mulan PSL v2 at:
#          http://license.coscl.org.cn/MulanPSL2
#
# THIS SOFTWARE IS PROVIDED ON AN "AS IS" BASIS, WITHOUT WARRANTIES OF ANY KIND,
# EITHER EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO NON-INFRINGEMENT,
# MERCHANTABILITY OR FIT FOR A PARTICULAR PURPOSE.
#
# See the Mulan PSL v2 for more details.
#**************************************************************************************/

# NAME = gen-expr
# SRCS = gen-expr.c
# include $(NEMU_HOME)/scripts/build.mk


WORK_DIR  = $(shell pwd)
BUILD_DIR = $(WORK_DIR)/build
SRC_I_DIR = $(BUILD_DIR)/src-i-$(NAME)$(SO)

SRCS = test.c

SRC_I = $(SRCS:%.c=$(SRC_I_DIR)/%.i)

$(SRC_I_DIR)/%.i: %.c
	@echo + CC $<
	@mkdir -p $(dir $@)
	@$(CC) -E -o $@ $<



t:$(SRC_I) 
	echo "done"
