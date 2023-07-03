/*
 * Copyright (c) 2023 Arm Limited. All rights reserved.
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */

#include "libc.h"
#include "morello.h"

__attribute__((used))
void _start(int argc, char *argv[], char *envp[], auxv_t *auxv)
{
    init_cap_relocs(auxv);
    init_morello_relative(auxv);

    for (const auxv_t *entry = auxv; entry->type; entry++) {
        if (morello_is_valid(entry->ptr)) {
            printf("%4lu: %+#p\n", entry->type, entry->ptr);
        } else {
            printf("%4lu: %016lx\n", entry->type, entry->val);
        }
    }

    exit(0);
}
