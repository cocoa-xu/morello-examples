/*
 * Copyright (c) 2023 Arm Limited. All rights reserved.
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */

#include "morello.h"
#include "libc.h"

size_t morello_get_length(const void *cap)
{
    return cap == NULL ? 0ul : __builtin_cheri_length_get(cap);
}

const void *morello_get_limit(const void *cap)
{
    return __builtin_cheri_address_set(cap, __builtin_cheri_base_get(cap)) + morello_get_length(cap);
}

size_t morello_get_tail(const void *cap)
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

bool morello_in_bounds(const void *cap)
{
    size_t address = __builtin_cheri_address_get(cap);
    size_t base = __builtin_cheri_base_get(cap);
    return base <= address && address < (base + morello_get_length(cap));
}

bool morello_is_valid(const void *cap)
{
    return __builtin_cheri_tag_get(cap) != 0 && morello_in_bounds(cap);
}

#define TEST(x, f) (((x) & (f)) == (f))

bool morello_is_local(const void *cap)
{
    return !TEST(__builtin_cheri_perms_get(cap), PERM_GLOBAL);
}

static const char permnames[] = {
    'G',
    'r', 'R', 'M',
    'w', 'W', 'L',
    'x', 'E', 'S',
    's', 'u',
    'I',
    'C',
    'V',
    '1', '2', '3',
    0
};

static size_t permbits[] = {
    PERM_GLOBAL,
    PERM_LOAD, PERM_LOAD_CAP, PERM_MUTABLE_LOAD,
    PERM_STORE, PERM_STORE_CAP, PERM_STORE_LOCAL_CAP,
    PERM_EXECUTE, PERM_EXECUTIVE, PERM_SYSTEM,
    PERM_SEAL, PERM_UNSEAL,
    PERM_CAP_INVOKE,
    PERM_CMPT_ID,
    PERM_VMEM,
    PERM_USER_1, PERM_USER_2, PERM_USER_3,
    0ul
};

const char *cap_perms_to_str(char *dst, const void *cap)
{
    const char *res = dst;
    size_t perms = __builtin_cheri_perms_get(cap);
    const char *lim = (char *)morello_get_limit(dst);
    size_t *p = permbits;
    const char *n = permnames;
    for(; dst < lim && *p && *n; n++, p++, dst++) {
        *dst = TEST(perms, *p) ? *n : '-';
    }
    if (morello_in_bounds(dst)) {
        *dst = '\0';
    }
    return res;
}

const char *cap_seal_to_str(char *dst, const void *cap)
{
    const char *res = dst;
    ptrdiff_t otype = __builtin_cheri_type_get(cap) & 0x7fff;
    switch (otype) {
        case 0:
            strcpy(dst, "none");
            break;
        case 1:
            strcpy(dst, "rb");
            break;
        case 2:
            strcpy(dst, "lpb");
            break;
        case 3:
            strcpy(dst, "lb");
            break;
        default:
            sprintf(dst, "%04x", otype);
    }
    return res;
}
