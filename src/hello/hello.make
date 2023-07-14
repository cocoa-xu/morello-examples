# Copyright (c) 2023 Arm Limited. All rights reserved.
#
# SPDX-License-Identifier: BSD-3-Clause

override hello_project = $(notdir $(realpath $(dir $(lastword $(MAKEFILE_LIST)))))

ifeq (clang,$(COMPILER_FAMILY))
override HELLO_LFLAGS = -lunwind -lc++abi -Wl,-warning-limit=1
else
override HELLO_LFLAGS =
endif

main: $(BINDIR)/hello

$(BINDIR)/hello: $(OBJDIR)/$(hello_project)/hello.c.o | $(BINDIR)
	$(CC) $(LFLAGS) $^ -o $@ -static

$(BINDIR)/hellocpp: $(OBJDIR)/$(hello_project)/hello.cpp.o | $(BINDIR)
	$(CXX) $(LFLAGS) $^ -o $@ $(HELLO_LFLAGS) -static
