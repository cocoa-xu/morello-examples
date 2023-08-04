/*
 * Copyright (c) 2023 Arm Limited. All rights reserved.
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */

typedef unsigned long size_t;
typedef long ssize_t;
typedef _Bool bool;

#include "morello.h"

#define NULL ((void *)0)

size_t morello_get_length(const void * __capability cap)
{
    bool is_null = cap == NULL && __builtin_cheri_tag_get(cap) == 0ul;
    return is_null ? 0ul : __builtin_cheri_length_get(cap);
}

const void * __capability morello_get_limit(const void * __capability cap)
{
    return __builtin_cheri_address_set(cap, __builtin_cheri_base_get(cap)) + morello_get_length(cap);
}

size_t morello_get_tail(const void * __capability cap)
{
    size_t address = __builtin_cheri_address_get(cap);
    size_t base = __builtin_cheri_base_get(cap);
    size_t limit = morello_get_length(cap) + base;
    if (base <= address && address < limit) {
        return limit - address;
    } else {
        return 0ul;
    }
}

bool morello_in_bounds(const void * __capability cap)
{
    size_t address = __builtin_cheri_address_get(cap);
    size_t base = __builtin_cheri_base_get(cap);
    return base <= address && address < (base + __builtin_cheri_length_get(cap));
}

bool morello_is_valid(const void * __capability cap)
{
    return __builtin_cheri_tag_get(cap) != 0 && morello_in_bounds(cap);
}

#define TEST(x, f) (((x) & (f)) == (f))

bool morello_is_local(const void * __capability cap)
{
    return !TEST(__builtin_cheri_perms_get(cap), PERM_GLOBAL);
}
