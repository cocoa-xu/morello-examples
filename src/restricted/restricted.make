# Copyright (c) 2023 Arm Limited. All rights reserved.
#
# SPDX-License-Identifier: BSD-3-Clause

override strm_this := $(lastword $(MAKEFILE_LIST))
override strm_curdir := $(realpath $(dir $(strm_this)))
override strm_project := $(notdir $(strm_curdir))

override strm_objects = \
	$(OBJDIR)/$(strm_project)/restricted.c.o \
	$(OBJDIR)/$(strm_project)/src/start.S.o \
	$(OBJDIR)/$(strm_project)/src/cman.c.o

# we use -nostdlib when linking executables in this subproject
# hence we need to locate libgcc or libclang_rt.builtins for the
# __clear_cache builtin
ifeq ($(COMPILER_FAMILY),gcc)
override AARCH64_RTLIB = $(shell $(CC) -print-libgcc-file-name)
override PURECAP_RTLIB = $(shell $(CC) -print-libgcc-file-name -march=morello+c64 -mabi=purecap)
endif
ifeq ($(COMPILER_FAMILY),clang)
override AARCH64_RTLIB = $(shell $(CC) -print-libgcc-file-name --target=aarch64-linux-gnu)
override PURECAP_RTLIB = $(shell $(CC) -print-libgcc-file-name --target=aarch64-linux-musl_purecap)
endif

$(OBJDIR)/libfree.a: $(free_objects) $(util_objects)
	$(create-archive)

main: $(BINDIR)/restricted | $(BINDIR)

$(BINDIR)/restricted: $(strm_objects) $(OBJDIR)/libfree.a
	$(CC) -nostdlib -ffreestanding $(FREE_LFLAGS) $^ $(PURECAP_RTLIB) -o $@ -static

$(strm_objects): CFLAGS = $(FREE_CFLAGS) -nostdinc -ffreestanding
$(strm_objects): CFLAGS += -I$(free_curdir)/include -I$(util_curdir) -I$(strm_curdir)/include

$(strm_objects): $(strm_this)

$(OBJDIR)/$(strm_project)/restricted.c.o: CFLAGS += -O0
