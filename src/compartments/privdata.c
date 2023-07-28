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
#include <cheriintrin.h>

#include "morello.h"

/**
 * Private data struct.
 */
typedef struct {
    unsigned secret;
    void *data;
    void *owning;
    void *sealer;
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
static void init();

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
    init();

    printf("&priv:          %s\n", cap_to_str(NULL, &priv_data));
    printf("priv:           %s\n", cap_to_str(NULL, priv_data));
    printf("priv->data:     %s\n", cap_to_str(NULL, priv_data->data));
    printf("priv->owning:   %s\n", cap_to_str(NULL, priv_data->owning));

    good_fun_t *fn = protect(encrypt_message);

    printf("priv:           %s\n", cap_to_str(NULL, priv_data));
    printf("fn:             %s\n", cap_to_str(NULL, fn));

    const char *message = "hello morello...";
    char buffer[17] = {};
    const char *encrypted = fn(priv_data, buffer, message, 16);

    printf("secret message: %s\n", message);
    printf("encrypted data: %s\n", encrypted);

    const char *decrypted = fn(priv_data, buffer, encrypted, 16);
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

static void init()
{
    size_t pgsz = getpagesize();
    int prot = PROT_READ | PROT_WRITE | PROT_CAP_INVOKE;
    int flags = MAP_PRIVATE | MAP_ANONYMOUS;
    void *mem = mmap(NULL, pgsz, prot, flags, -1, 0);

    typedef struct {
        priv_data_t priv;
        char data[128];
    } __partition_t;

    __partition_t *part = (__partition_t *)cheri_perms_and(mem, RWI_PERMS);
    priv_data = cheri_bounds_set_exact(&part->priv, sizeof(priv_data_t));
    priv_data->secret = 0xcafe1e55;
    priv_data->data = cheri_perms_and(cheri_bounds_set_exact(part->data, 128), RW_PERMS);
    priv_data->owning = mem; // todo: use this owning capability for munmap
    priv_data->sealer = __builtin_cheri_offset_increment(cheri_perms_and(getauxptr(AT_CHERI_SEAL_CAP), PERM_SEAL), 7);
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
    // todo: do something when length of message
    // is not multiple of the key size
    unsigned key = priv->secret;
    const unsigned *src = (const unsigned *)text;
    unsigned *dst = (unsigned *)out;
    size_t processed = 0;
    while(morello_get_tail(src) > sizeof(unsigned)
        && morello_get_tail(dst) > sizeof(unsigned)
        && processed < len) {
        *dst = *src ^ key;
        dst++;
        src++;
        processed += sizeof(unsigned);
    }
    if (morello_in_bounds(out + processed)) {
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
        void *target;
        void *prot_start;
        void *prot_end;
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
    data->prot_end = cheri_seal(code + (_prot_end - _sw_start) + 1, seal);

    // Change memory protection flags:
    mprotect(code, _sw_size, PROT_READ | PROT_EXEC);
    __builtin___clear_cache(code, code + _sw_size);

    // Return callable sentry:
    return cheri_sentry_create(cheri_perms_and(code, RX_PERMS) + 1);
}
