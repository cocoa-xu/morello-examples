/*
 * Copyright (c) 2023 Arm Limited. All rights reserved.
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */

#pragma once

#include "types.h"

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

// Permission bits
#define PERM_GLOBAL             __CHERI_CAP_PERMISSION_GLOBAL__
#define PERM_EXECUTE            __CHERI_CAP_PERMISSION_PERMIT_EXECUTE__
#define PERM_LOAD               __CHERI_CAP_PERMISSION_PERMIT_LOAD__
#define PERM_STORE              __CHERI_CAP_PERMISSION_PERMIT_STORE__
#define PERM_LOAD_CAP           __CHERI_CAP_PERMISSION_PERMIT_LOAD_CAPABILITY__
#define PERM_STORE_CAP          __CHERI_CAP_PERMISSION_PERMIT_STORE_CAPABILITY__
#define PERM_STORE_LOCAL_CAP    __CHERI_CAP_PERMISSION_PERMIT_STORE_LOCAL__
#define PERM_SEAL               __CHERI_CAP_PERMISSION_PERMIT_SEAL__
#define PERM_UNSEAL             __CHERI_CAP_PERMISSION_PERMIT_UNSEAL__
#define PERM_SYSTEM             __CHERI_CAP_PERMISSION_ACCESS_SYSTEM_REGISTERS__
#define PERM_EXECUTIVE          __ARM_CAP_PERMISSION_EXECUTIVE__
#define PERM_MUTABLE_LOAD       __ARM_CAP_PERMISSION_MUTABLE_LOAD__
#define PERM_CMPT_ID            __ARM_CAP_PERMISSION_COMPARTMENT_ID__
#define PERM_CAP_INVOKE         __ARM_CAP_PERMISSION_BRANCH_SEALED_PAIR__
#define PERM_USER_0 (1 << 2)
#define PERM_USER_1 (1 << 3)
#define PERM_USER_2 (1 << 4)
#define PERM_USER_3 (1 << 5)
#define PERM_VMEM PERM_USER_0

// Permission groups
#define READ_CAP_PERMS (PERM_LOAD | PERM_LOAD_CAP | PERM_MUTABLE_LOAD)
#define WRITE_CAP_PERMS (PERM_STORE | PERM_STORE_CAP | PERM_STORE_LOCAL_CAP)
#define EXEC_CAP_PERMS (PERM_EXECUTE | PERM_EXECUTIVE | PERM_SYSTEM)

// Utility functions
const char *cap_perms_to_str(char *dst, const void *cap);
const char *cap_seal_to_str(char *dst, const void *cap);
