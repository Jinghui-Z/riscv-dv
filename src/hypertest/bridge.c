// Copyright 2026
// SPDX-License-Identifier: Apache-2.0
// Bare-metal adapter for the original hypertest case functions.
#include <rvh_test.h>
#include <stdarg.h>

// These renames apply only while compiling upstream/src/rvh_test.c. The
// bridge and the original case objects must resolve the public wrappers below.
#ifdef reset_state
#undef reset_state
#endif
#ifdef goto_priv
#undef goto_priv
#endif

extern unsigned real_priv;
extern unsigned test_num;
extern uint64_t ecall_args[2];
extern void mhandler_entry(void);
extern void hshandler_entry(void);
extern void dv_hyp_legacy_goto_priv(int target_priv);
extern void dv_hypertest_fail(void) __attribute__((noreturn));
extern unsigned char __dv_hypertest_bss_start[];
extern unsigned char __dv_hypertest_bss_end[];

// The normal main/test_table driver is deliberately not linked. These symbols
// satisfy the unused globals in rvh_test.c without registering a second suite.
test_func_t _test_table;
test_func_t _test_table_size;

static inline uint64_t dv_hypertest_misa(void)
{
    uint64_t value;
    __asm__ volatile("csrr %0, misa" : "=r"(value));
    return value;
}

// The bridge clears BSS for every injected case. A freestanding byte memset
// makes large source-case pools spend most of their ISS budget in this loop.
// Use aligned RV64 stores without assuming the linker symbols are aligned.
static void dv_hypertest_clear_bss(void)
{
    uintptr_t cursor = (uintptr_t)__dv_hypertest_bss_start;
    uintptr_t end = (uintptr_t)__dv_hypertest_bss_end;

    while (cursor < end && (cursor & (sizeof(uint64_t) - 1)) != 0) {
        *(unsigned char *)cursor++ = 0;
    }
    while (end - cursor >= 8 * sizeof(uint64_t)) {
        __asm__ volatile(
            "sd zero,  0(%0)\n"
            "sd zero,  8(%0)\n"
            "sd zero, 16(%0)\n"
            "sd zero, 24(%0)\n"
            "sd zero, 32(%0)\n"
            "sd zero, 40(%0)\n"
            "sd zero, 48(%0)\n"
            "sd zero, 56(%0)"
            :: "r"(cursor) : "memory");
        cursor += 8 * sizeof(uint64_t);
    }
    while (end - cursor >= sizeof(uint64_t)) {
        __asm__ volatile("sd zero, 0(%0)" :: "r"(cursor) : "memory");
        cursor += sizeof(uint64_t);
    }
    while (cursor < end) {
        *(unsigned char *)cursor++ = 0;
    }
}

void goto_priv(int target_priv)
{
    // HS and HU are the upstream names for ordinary S/U when H is absent.
    // VS/VU require actual virtualization; never silently alias them to S/U.
    if (target_priv < PRIV_VU || target_priv >= PRIV_MAX ||
        ((target_priv == PRIV_VS || target_priv == PRIV_VU) &&
         !(dv_hypertest_misa() & (1ULL << ('H' - 'A'))))) {
        dv_hypertest_fail();
    }
    dv_hyp_legacy_goto_priv(target_priv);
}

void reset_state(void)
{
    goto_priv(PRIV_M);
    rnmi_handler_enabled = false;
    double_trap_enabled = false;
    // Match the upstream MSU reset, omitting optional PMP, H and Smrnmi CSR
    // accesses. A test explicitly exercising one of them keeps its own accesses
    // and must be admitted by the target requirements manifest.
    __asm__ volatile(
        "csrw mstatus, zero\n"
        "csrw medeleg, zero\n"
        "csrw mideleg, zero\n"
        "csrw mie, zero\n"
        "csrw mip, zero\n"
        "csrw mscratch, zero\n"
        "csrw mepc, zero\n"
        "csrw mtval, zero\n"
        "csrw sstatus, zero\n"
        "csrw sip, zero\n"
        "csrw sie, zero\n"
        "csrw sscratch, zero\n"
        "csrw sepc, zero\n"
        "csrw scause, zero\n"
        "csrw stval, zero\n"
        "csrw satp, zero\n"
        "sfence.vma\n" ::: "memory");
    CSRC(CSR_MENVCFG, MENVCFG_CBIE11);
    CSRW(CSR_MTVEC, (uintptr_t)mhandler_entry);
    CSRW(CSR_STVEC, (uintptr_t)hshandler_entry);
    CSRS(CSR_MEDELEG, 1ULL << 8);
    if (dv_hypertest_misa() & (1ULL << ('V' - 'A'))) {
        CSRS(CSR_MSTATUS, 1ULL << 9);
        __asm__ volatile("csrw vstart, zero\ncsrw vcsr, zero" ::: "memory");
    }
}

// Called after the assembly wrapper has saved the outer architectural state.
// Return the original boolean result: TEST_ASSERT failures must never be lost.
int dv_hypertest_run_case(test_func_t function)
{
    if (function == NULL) {
        dv_hypertest_fail();
    }
    // RTL binary loaders need not initialize NOBITS memory. Start every case
    // with clean C zero-initialized storage, including private harness flags.
    // The independent bridge stack and outer-frame slot lie outside this range.
    dv_hypertest_clear_bss();
    curr_priv = PRIV_M;
    real_priv = PRIV_M;
    test_num = 0;
    ecall_args[0] = 0;
    ecall_args[1] = 0;
    memset(&excpt, 0, sizeof(excpt));
    rnmi_handler_enabled = false;
    double_trap_enabled = false;
    reset_state();
    return function() ? 1 : 0;
}

// No terminal syscall is a valid return from an embedded case, even exit(0).
// The upstream ERROR macro can call exit(0), so success cannot use its status.
void exit(int status)
{
    (void)status;
    dv_hypertest_fail();
}

void abort(void)
{
    dv_hypertest_fail();
}

void tohost_exit(uintptr_t status)
{
    (void)status;
    dv_hypertest_fail();
}

void __assert_func(const char *file, int line, const char *function, const char *expression)
{
    (void)file;
    (void)line;
    (void)function;
    (void)expression;
    dv_hypertest_fail();
}

void __stack_chk_fail(void)
{
    dv_hypertest_fail();
}

// LOG_LEVEL=0 keeps the original assertion computation and boolean result.
// Diagnostics are intentionally silent: no UART/HTIF service is needed while
// the random program owns its tohost protocol.
int printf(const char *format, ...)
{
    (void)format;
    return 0;
}

int puts(const char *string)
{
    (void)string;
    return 0;
}

int putchar(int character)
{
    return (unsigned char)character;
}

void printstr(const char *string)
{
    (void)string;
}

size_t strlen(const char *string)
{
    size_t length = 0;
    while (string[length] != '\0') {
        ++length;
    }
    return length;
}

size_t strnlen(const char *string, size_t maximum)
{
    size_t length = 0;
    while (length < maximum && string[length] != '\0') {
        ++length;
    }
    return length;
}

int strcmp(const char *left, const char *right)
{
    while (*left != '\0' && *left == *right) {
        ++left;
        ++right;
    }
    return (unsigned char)*left - (unsigned char)*right;
}

void *memcpy(void *destination, const void *source, size_t length)
{
    unsigned char *out = destination;
    const unsigned char *in = source;
    for (size_t i = 0; i < length; ++i) {
        out[i] = in[i];
    }
    return destination;
}

void *memset(void *destination, int value, size_t length)
{
    unsigned char *out = destination;
    for (size_t i = 0; i < length; ++i) {
        out[i] = (unsigned char)value;
    }
    return destination;
}

int memcmp(const void *left, const void *right, size_t length)
{
    const unsigned char *a = left;
    const unsigned char *b = right;
    for (size_t i = 0; i < length; ++i) {
        if (a[i] != b[i]) {
            return a[i] - b[i];
        }
    }
    return 0;
}

void *memmove(void *destination, const void *source, size_t length)
{
    unsigned char *out = destination;
    const unsigned char *in = source;
    if ((uintptr_t)out < (uintptr_t)in) {
        for (size_t i = 0; i < length; ++i) {
            out[i] = in[i];
        }
    } else {
        while (length != 0) {
            --length;
            out[length] = in[length];
        }
    }
    return destination;
}
