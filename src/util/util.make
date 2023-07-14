# Copyright (c) 2023 Arm Limited. All rights reserved.
#
# SPDX-License-Identifier: BSD-3-Clause

override util_this := $(lastword $(MAKEFILE_LIST))
override util_curdir := $(realpath $(dir $(util_this)))
override util_project := $(notdir $(util_curdir))

override util_objects = \
	$(OBJDIR)/$(util_project)/capprint.c.o \
	$(OBJDIR)/$(util_project)/morello.c.o

main: $(OBJDIR)/libutil.a

$(util_objects): CFLAGS += -nostdinc

$(OBJDIR)/libutil.a: $(util_objects)
	$(create-archive)

$(util_objects): $(util_this)
