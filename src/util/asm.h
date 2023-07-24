/*
 * Copyright (c) 2023 Arm Limited. All rights reserved.
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */

#pragma once

#define FUN(fn) \
    .global fn; \
    .balign 16; \
    .type fn, %function; \
    fn

#define END(fn) \
    .size fn, .-fn;