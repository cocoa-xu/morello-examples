# Copyright (c) 2023 Arm Limited. All rights reserved.
#
# SPDX-License-Identifier: BSD-3-Clause

override cmpt_this := $(lastword $(MAKEFILE_LIST))
override cmpt_curdir := $(realpath $(dir $(cmpt_this)))
override cmpt_project := $(notdir $(cmpt_curdir))

override cmpt_bsp_objects := \
	$(OBJDIR)/$(cmpt_project)/src/manager.c.o

override cmpt_bsp_objfiles = $(cmpt_bsp_objects) \
	$(OBJDIR)/$(cmpt_project)/hellobsp.c.o \
	$(OBJDIR)/$(cmpt_project)/hackpwd.c.o \
	$(OBJDIR)/$(cmpt_project)/nestedcmpt.c.o

$(cmpt_bsp_objfiles): CFLAGS += -I$(cmpt_curdir)/include -I$(cmpt_curdir)/../util

main: $(BINDIR)/hellobsp $(BINDIR)/hackpwd $(BINDIR)/nestedcmpt

$(OBJDIR)/$(cmpt_project)/hackpwd.c.o: CFLAGS += -O0

$(BINDIR)/%: $(OBJDIR)/$(cmpt_project)/%.c.o $(cmpt_bsp_objects) $(OBJDIR)/libutil.a
	$(CC) $(LFLAGS) $^ -o $@ -static

$(cmpt_bsp_objfiles): $(cmpt_this)
