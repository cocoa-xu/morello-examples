/*
 * Copyright (c) 2023 Arm Limited. All rights reserved.
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */

#define _GNU_SOURCE

#include <stdio.h>
#include <stddef.h>
#include <unistd.h>
#include <string.h>
#include <stdbool.h>
#include <sys/mman.h>
#include <sys/auxv.h>

#include "morello.h"

/**
 * Private data struct.
 */
typedef struct {
    unsigned secret;
    void *data;
    void *owning;
    void *sealer;
    void *stack;
    void *stack_owning;
} priv_data_t;

/**
 * A pointer to the global object that holds secret
 * information.
 */
static priv_data_t *priv_data;

/**
 * Initialisation function that allocate memory for
 * the global object.
 */
static void init(size_t stack_pages);

/**
 * Malicious code. It can derive a valid capability
 * from one of the ambient or root capabilities and
 * then use it to access the secret information.
 */
static void malware();

/**
 * This is a good function that is supposed to have
 * access to the private information.
 *
 * This function will encrypt the input text of the
 * given length using the xor algorithm and put the
 * result into the out buffer (must be able to hold
 * the same amount of characters).
 *
 * It returns the pointer to the out buffer.
 */
static const char *encrypt_message(const priv_data_t *priv, char *out, const char *text, size_t len);

/**
 * Handy type definitions.
 */
typedef const char *(good_fun_t)(const priv_data_t *, char*, const char *, size_t);

/**
 * This function protects the global pointer.
 */
static good_fun_t *protect(good_fun_t *fn);

int main(int argc, char *argv[])
{
    init(4 /* stack pages */);

    printf("&priv:          %s\n", cap_to_str(NULL, &priv_data));
    printf("priv:           %s\n", cap_to_str(NULL, priv_data));
    printf("priv->data:     %s\n", cap_to_str(NULL, priv_data->data));
    printf("priv->owning:   %s\n", cap_to_str(NULL, priv_data->owning));
    printf("priv->stack:    %s\n", cap_to_str(NULL, priv_data->stack));

    good_fun_t *fn = protect(encrypt_message);

    printf("priv:           %s\n", cap_to_str(NULL, priv_data));
    printf("fn:             %s\n", cap_to_str(NULL, fn));

    size_t priv_addr = cheri_address_get(priv_data);

    const char *message = "hello morello...";
    char buffer[17] = {};
    printf("before...\n");
    printf("csp:            %s\n", cap_to_str(NULL, cheri_csp_get()));
    const char *encrypted = fn(priv_data, buffer, message, 16);
    printf("after...\n");
    printf("csp:            %s\n", cap_to_str(NULL, cheri_csp_get()));

    /**
     * This code looks through the stack to see if an unsealed, valid
     * priv_data capability was spilled during the encrypt function.
     * Note that due to ASLR and compiler optimisations this code may
     * not find the spilled capability and the only definite way to
     * find it would be to use a debugger.
     *
     * To do so in morelloie run:
     *   $ morelloie -debug -break encrypt_message -- build/bin/privdata
     * During Morelloie's run:
     *   $ finish
     *   $ view csp-1024 csp
     * And the unsealed priv_data capability will be one of the printed values.
     *
     * To do so in GDB run:
     *   $ gdb build/bin/privdata
     * During GDB's run:
     *   $ break encrypt_message
     *   $ r
     *   $ n
     *   Repeat until the priv ptr is dereferenced in encrypt_message
     *   $x/64xg $sp
     *   Search the printed stack for the address of priv as printed earlier in the program's run
     *   Find the offset from $sp in which priv is stored
     *   $ p (void * __capability)*(priv_data_t**)($sp + offset)
     *   The last command verifies that the capability in memory is unsealed and valid
     */
    void **sp = cheri_csp_get();
    size_t caps_to_check = 512;
    for(size_t c = 0; c < caps_to_check; ++c, --sp) {
        void *cap_in_stack = *sp;
        if(!cheri_tag_get(cap_in_stack)) {
            continue;
        }

        if(cheri_address_get(cap_in_stack) == priv_addr && !cheri_is_sealed(cap_in_stack)) {
            printf("FOUND SECRET IN STACK!\n");
            printf("Addr: %lx, Cap: %s\n", cheri_address_get(sp), cap_to_str(NULL, cap_in_stack));
            printf("Secret: %x\n", ((priv_data_t*)cap_in_stack)->secret);
            break;
        }

    }

    printf("secret message: %s\n", message);
    printf("encrypted data: %s\n", encrypted);

    printf("before...\n");
    printf("csp:            %s\n", cap_to_str(NULL, cheri_csp_get()));
    const char *decrypted = fn(priv_data, buffer, encrypted, 16);
    printf("after...\n");
    printf("csp:            %s\n", cap_to_str(NULL, cheri_csp_get()));
    printf("decrypted:      %s\n", decrypted);

    malware();

    return 0;
}

#define RW_PERMS (PERM_GLOBAL | READ_CAP_PERMS | WRITE_CAP_PERMS)
#define RX_PERMS (PERM_GLOBAL | READ_CAP_PERMS | EXEC_CAP_PERMS)
#define RWI_PERMS (RW_PERMS | PERM_CAP_INVOKE)
#define RXI_PERMS (RX_PERMS | PERM_CAP_INVOKE)

#ifndef PROT_CAP_INVOKE
#define PROT_CAP_INVOKE 0x2000 // Purecap libc fix-ups
#endif

static void init(size_t stack_pages)
{
    size_t pgsz = getpagesize();
    size_t stack_len = stack_pages * pgsz;
    int prot = PROT_READ | PROT_WRITE | PROT_CAP_INVOKE;
    int stack_prot = PROT_READ | PROT_WRITE;
    int flags = MAP_PRIVATE | MAP_ANONYMOUS;
    void *mem = mmap(NULL, pgsz, prot, flags, -1, 0);
    void *stack_mem = mmap(NULL, stack_len, stack_prot, flags, -1, 0);

    typedef struct {
        priv_data_t priv;
        char data[128];
    } __partition_t;

    __partition_t *part = (__partition_t *)cheri_perms_and(mem, RWI_PERMS);
    priv_data = cheri_bounds_set_exact(&part->priv, sizeof(priv_data_t));
    priv_data->secret = 0xcafe1e55;
    priv_data->data = cheri_perms_and(cheri_bounds_set_exact(part->data, 128), RW_PERMS);
    priv_data->owning = mem; // todo: use this owning capability for munmap
    priv_data->sealer = cheri_perms_and(getauxptr(AT_CHERI_SEAL_CAP), PERM_SEAL) + 7;
    priv_data->stack = cheri_perms_and(cheri_bounds_set_exact(stack_mem, stack_len), RW_PERMS) + stack_len;
    priv_data->stack_owning = stack_mem;
}

static void malware()
{
    // Ambient and root capabilities:
    void *pcc = cheri_pcc_get();
    void *rw = getauxptr(AT_CHERI_EXEC_RW_CAP);
    void *rx = getauxptr(AT_CHERI_EXEC_RX_CAP);
    printf("pcc:            %s\n", cap_to_str(NULL, pcc));
    printf("rx:             %s\n", cap_to_str(NULL, rx));
    printf("rw:             %s\n", cap_to_str(NULL, rw));

    // Try accessing private secret:
    if (cheri_is_sealed(priv_data)) {
        printf("secret:         %s\n", "can't read secret");
    } else {
        printf("secret:         %x\n", priv_data->secret);
    }
}

static const char *encrypt_message(const priv_data_t *priv, char *out, const char *text, size_t len)
{
    printf("inside...\n");
    printf("csp:            %s\n", cap_to_str(NULL, cheri_csp_get()));
    // todo: do something when length of message
    // is not multiple of the key size
    unsigned key = priv->secret;
    const unsigned *src = (const unsigned *)text;
    unsigned *dst = (unsigned *)out;
    size_t processed = 0;
    while(cheri_get_tail(src) > sizeof(unsigned)
        && cheri_get_tail(dst) > sizeof(unsigned)
        && processed < len) {
        *dst = *src ^ key;
        dst++;
        src++;
        processed += sizeof(unsigned);
    }
    if (cheri_in_bounds(out + processed)) {
        out[processed] = '\0';
    }
    return out;
}

extern void __brs_switch();
extern void __brs_switch_end();
extern void __prot_start();
extern void __prot_end();

static good_fun_t *protect(good_fun_t *fn)
{
    // Replace global pointer with its sealed version:
    const void *seal = priv_data->sealer;
    priv_data = cheri_seal(priv_data, seal);

    // Obtain addresses and sizes for code relocation
    const void *rx = getauxptr(AT_CHERI_EXEC_RX_CAP);
    const char *_sw_start = cheri_address_set(rx, cheri_align_down(cheri_address_get(__brs_switch), 4));
    const char *_sw_end = cheri_address_set(rx, cheri_align_down(cheri_address_get(__brs_switch_end), 4));
    const char *_prot_start = cheri_address_set(rx, cheri_align_down(cheri_address_get(__prot_start), 4));
    const char *_prot_end = cheri_address_set(rx, cheri_align_down(cheri_address_get(__prot_end), 4));
    size_t _sw_size = _sw_end - _sw_start;

    typedef struct {
        void *target;       // The "good" function
        void *prot_start;   // BSP-sealed code pointer for BRS instruction
        void *prot_end;     // BSP-sealed code pointer for return BRS instruction (currently unused)
    } cmpt_data_t;

    // Allocate memory for the switch code and the associated data:
    size_t pgsz = getpagesize();
    int prot = PROT_READ | PROT_WRITE | PROT_CAP_INVOKE | PROT_MAX(PROT_READ | PROT_WRITE | PROT_EXEC);
    int flags = MAP_PRIVATE | MAP_ANONYMOUS;
    void *mem = mmap(NULL, pgsz, prot, flags, -1, 0);

    // Derive capabilities for code and data with the right bounds and permissions:
    cmpt_data_t *data = (cmpt_data_t *)cheri_perms_and(cheri_bounds_set_exact(cheri_align_up(mem + _sw_size, sizeof(void *)), sizeof(cmpt_data_t)), RW_PERMS);
    void *code = cheri_bounds_set_exact(mem, (const void *)data - (const void *)mem + sizeof(cmpt_data_t));

    // Relocate switch code:
    memcpy(code, (void *)_sw_start, _sw_size);
    code = cheri_perms_and(code, RXI_PERMS);

    // Fill in switch data:
    data->target = cheri_is_sealed(fn) ? fn : cheri_sentry_create(fn);
    data->prot_start = cheri_seal(code + (_prot_start - _sw_start) + 1, seal);
    // Note: we don't use prot_end yet but it can be utilised to implement
    // a return BRS instruction that, for example, would be needed for stack
    // switching (when the "good" function is called on an isolated stack to
    // mitigate spilling unsealed secret capability):
    data->prot_end = cheri_seal(code + (_prot_end - _sw_start) + 1, seal);

    // Change memory protection flags:
    mprotect(code, _sw_size, PROT_READ | PROT_EXEC);
    __builtin___clear_cache(code, code + _sw_size);

    // Return callable sentry:
    return cheri_sentry_create(cheri_perms_and(code, RX_PERMS) + 1);
}
