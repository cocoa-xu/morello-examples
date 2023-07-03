/*
 * Copyright (c) 2023 Arm Limited. All rights reserved.
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */

#include "libc.h"
#include "morello.h"

void exit(int code)
{
    register intptr_t c8 __asm__("c8") = SYS_EXIT_GROUP;
    register intptr_t c0 __asm__("c0") = code;
    __asm__ __volatile__ ("svc 0\n" : "=C"(c0) : "C"(c8), "0"(c0));
    __builtin_unreachable();
}

ssize_t write(int fd, const void *buf, size_t count)
{
    size_t tail = morello_get_tail(buf);
    if (count > tail) {
        count = tail;
    }
    if (count == 0ul) {
        return 0ul;
    }
    register intptr_t c8 __asm__("c8") = SYS_WRITE;
    register intptr_t c0 __asm__("c0") = fd;
    register intptr_t c1 __asm__("c1") = (intptr_t)buf;
    register intptr_t c2 __asm__("c2") = count;
    __asm__ __volatile__ ("svc 0\n" : "=C"(c0) : "C"(c8), "0"(c0), "C"(c1), "C"(c2));
    return c0;
}
