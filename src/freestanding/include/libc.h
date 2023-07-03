/*
 * Copyright (c) 2023 Arm Limited. All rights reserved.
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */

#pragma once

#include "types.h"

// Kernel ABI constants
#define SYS_EXIT_GROUP 94
#define SYS_WRITE 64
#define AT_CHERI_EXEC_RW_CAP 60
#define AT_CHERI_EXEC_RX_CAP 61

// Some useful builtins
#define va_start(v,l)   __builtin_va_start(v,l)
#define va_end(v)       __builtin_va_end(v)
#define va_arg(v,l)     __builtin_va_arg(v,l)
#define alloca(n)       __builtin_alloca(n)

// Syscall wrappers
void exit(int code) __attribute__((noreturn));
ssize_t write(int fd, const void *buf, size_t count);

// Formatted output
int printf(const char *fmt, ...);
int sprintf(char *dst, const char *fmt, ...);

// String manipulation
size_t strlen(const char *str);
char *strcpy(char *dst, const char *src);
int strcmp(const char *lhs, const char *rhs);
void *memset(void *dst, int c, size_t len);
void *memcpy(void *dst, const void *src, size_t len);

// Static init functions
int init_cap_relocs(const auxv_t *auxv);
int init_morello_relative(const auxv_t *auxv);
