/*
 * Copyright (c) 2023 Arm Limited. All rights reserved.
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */

#pragma once

#include "perms.h"

/**
 * Returns length of a capability:
 * length = limit - base
 * Length of a NULL capability is returned as 0.
 */
size_t morello_get_length(const void *cap);
/**
 * Returns limit of the capability as a pointer.
 * Note that limit for a NULL pointer is returned as NULL.
 */
const void *morello_get_limit(const void *cap);

/**
 * Returns difference between the address and the limit
 * of a capability: tail = limit - (base + offset) if this
 * capability is in bounds, otherwise, returns NULL.
 */
size_t morello_get_tail(const void *cap);

/**
 * Returns true if capability is in bounds.
 */
bool morello_in_bounds(const void *cap);

/**
 * Returns true if a capability is dereferenceable:
 * both capability's tag is 1 and the capability is
 * in bounds.
 */
bool morello_is_valid(const void *cap);

/**
 * Returns true if capability is local.
 */
bool morello_is_local(const void *cap);

/**
 * Convert capability into string representation.
 */
const char *cap_to_str(char *dst, const void *cap);
const char *cap_perms_to_str(char *dst, const void *cap);
const char *cap_seal_to_str(char *dst, const void *cap);

#if defined(__GNUC__) && !defined(__clang__) && defined(__CHERI_PURE_CAPABILITY__)
// GCC does not provide this builtin.
inline static void *__builtin_cheri_stack_get()
{
    void *ret;
    __asm__ volatile ("mov %0, csp" : "=C"(ret));
    return ret;
}
#endif

/**
 * Access CID register.
 */
inline static void *__builtin_cheri_cid_get()
{
    void *ret;
    __asm__ volatile ("mrs %0, CID_EL0" : "=C"(ret));
    return ret;
}

/**
 * Create LPB-sealed sentry.
 */
inline static void *morello_lpb_sentry_create(void *cap)
{
    void *ret;
    __asm__ ("seal %0, %1, lpb" : "=C"(ret) : "C"(cap));
    return ret;
}

/**
 * Create LB-sealed sentry.
 */
inline static void *morello_lb_sentry_create(void *cap)
{
    void *ret;
    __asm__ ("seal %0, %1, lb" : "=C"(ret) : "C"(cap));
    return ret;
}

/**
 * Checks if given permissions are present in the capability.
 */
inline static bool morello_check_perms(const void *cap, size_t perms)
{
    return (__builtin_cheri_perms_get(cap) | ~perms) == ~0;
}
