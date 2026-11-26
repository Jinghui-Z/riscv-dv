#ifndef BACKEND_IT_VMODULE_H
#define BACKEND_IT_VMODULE_H

#include "backend_it_0911_trap.h"

/* v_module.sv uses the legacy address/data ALU command transport. */
enum {
    BE_VM_MAIN = 0x1000,
    BE_VM_WDT_START = 0x1004,
    BE_VM_WDT_END = 0x1008,
    BE_VM_QUEUE = 0x1020,
    BE_VM_DELAY = 0x1028,
    BE_VM_DEBUG = 0x1048,
    BE_VM_MEI = 1U << 11,
    BE_VM_SPINS = 100000
};

static inline void be_vm_write(unsigned offset, uint32_t data)
{
    uint64_t address = 0xffff000100000000ULL | offset;
    uint64_t value = 0xffff000000000000ULL | data;
    uint64_t status = CSRR(CSR_MSTATUS);
    /* An IRQ handler also uses this paired transport: do not let it replace
     * the address between the two commands. No NMI is armed by this helper. */
    CSRC(CSR_MSTATUS, MSTATUS_MIE);
    asm volatile(BE_ASM_BEGIN
        "fence.i\nadd zero,zero,%0\nfence.i\nadd zero,zero,%1\nfence.i\n"
        BE_ASM_END :: "r"(address), "r"(value) : "memory");
    if (status & MSTATUS_MIE) CSRS(CSR_MSTATUS, MSTATUS_MIE);
}

static inline bool be_vm_pending(bool high)
{
    for (unsigned i = 0; i < BE_VM_SPINS; ++i)
        if (!!(CSRR(CSR_MIP) & BE_VM_MEI) == high) return true;
    return false;
}

static inline bool be_vm_clear(void)
{
    /* Cancel an outstanding delayed command before clearing its level. */
    be_vm_write(BE_VM_DELAY, 0);
    be_vm_write(BE_VM_MAIN, 0);
    bool clear = be_vm_pending(false);
    be_vm_write(BE_VM_DEBUG, 0);
    return clear;
}

static inline void be_vm_mei(unsigned delay, bool pulse)
{
    be_vm_write(BE_VM_DELAY, delay);
    /* The force RHS follows queue_mode_cnt even after work_done. Counter
     * wrap selects slot 0 again: both slot 0 and the trailing slots must be
     * zero, or a supposedly finite pulse becomes a persistent level. */
    be_vm_write(pulse ? BE_VM_QUEUE : BE_VM_MAIN,
        pulse ? ((1U << 13) | (1U << 26)) : BE_VM_MEI);
}

static inline void be_vm_watchdog(unsigned cycles)
{
    be_vm_write(BE_VM_WDT_START, 0);
    be_vm_write(BE_VM_WDT_END, cycles);
}

static inline bool be_vm_wait_traps(unsigned count)
{
    for (unsigned i = 0; i < BE_VM_SPINS; ++i) {
        if (be_it_traps.count >= count) return true;
        asm volatile("nop" ::: "memory");
    }
    return false;
}

bool be_it_vmodule_available(void);
bool be_it_vmodule_irq_probe(unsigned delay, bool wfi);
bool be_it_vmodule_ecc_prepare(void);
bool be_it_vmodule_ecc_probe(void);

#endif
