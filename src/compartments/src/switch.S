/*
 * Copyright (c) 2023 Arm Limited. All rights reserved.
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */

#include "asm.h"

DEC(__prot_start)
DEC(__prot_end)
DEC(__brs_switch_end)

#define code    c27
#define data    c28
#define tmp     c29
#define target  c30
#define secret  c3

FUN(__brs_switch):
    sub     csp, csp, #(1*32)
    stp     c29, c30, [csp, #(0*32)]
    adr     c29, __brs_switch_end
    alignu  c29, c29, #4
    ldp     c30, c29, [c29]
    brs     c29, c29, c0
__prot_start:
    mov     c0, c29
    mov     c29, csp
    blr     c30
__prot_end:                             // this label can be used to implement
    ldp     c29, c30, [csp, #(0*32)]    // a return BRS branch
    add     csp, csp, #(1*32)
    ret
    udf     #0
__brs_switch_end:
END(__brs_switch)
