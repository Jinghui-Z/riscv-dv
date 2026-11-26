#include "backend_it_vmodule.h"
#include "../../frontend/fetch/frontend_it_vmodule.h"

static uint32_t vm_ecc_code[1024] __attribute__((aligned(4096)));

bool be_it_vmodule_ecc_prepare(void)
{
    backend_it_control.target_pc = (uintptr_t)&vm_ecc_code[128];
    return be_it_vmodule_available();
}

bool be_it_vmodule_available(void)
{
#if VMODULE
    return true;
#else
    printf("BACKEND_VMODULE requires a whole-ELF VMODULE=1 build\n");
    return false;
#endif
}

bool be_it_vmodule_irq_probe(unsigned delay, bool wfi)
{
    if (!be_it_vmodule_available()) return false;
    TEST_SETUP_EXCEPT();
    be_it_trap_install();
    bool ok = be_vm_clear();
    be_vm_watchdog(2000000);
    CSRW(CSR_MIE, BE_VM_MEI);
    be_vm_mei(delay, false);
    uint64_t marker;
    /* Keep global MIE clear until after WFI. Otherwise an early interrupt
     * can be serviced/cleared before WFI and leave no source to wake it. */
    if (wfi)
        asm volatile(BE_ASM_BEGIN "wfi\nli %0,1\n" BE_ASM_END : "=r"(marker) :: "memory");
    else {
        ok &= be_vm_pending(true);
        marker = 1;
    }
    CSRS(CSR_MSTATUS, MSTATUS_MIE);
    ok &= be_vm_wait_traps(1);
    CSRC(CSR_MSTATUS, MSTATUS_MIE);
    ok &= be_it_traps.count == 1 && be_it_traps.event[0].cause == CAUSE_MEI && marker == 1;
    /* The custom handler must clear the physical source, not only mask MIE. */
    ok &= be_vm_pending(false);
    CSRW(CSR_MIE, BE_VM_MEI);
    CSRS(CSR_MSTATUS, MSTATUS_MIE);
    for (unsigned i = 0; i < 256; ++i) asm volatile("nop" ::: "memory");
    CSRC(CSR_MSTATUS, MSTATUS_MIE);
    ok &= be_it_traps.count == 1;
    ok &= be_vm_clear();
    be_vm_watchdog(UINT32_MAX);
    be_it_trap_restore();
    return ok;
}

static uint64_t vm_ecc_call(uintptr_t pc)
{
    uint64_t value;
    asm volatile(BE_ASM_BEGIN "lla t0,1f\nsd t0,%[fault]\nli a0,41\n"
        "1: jalr ra,0(%[pc])\nmv %[out],a0\n" BE_ASM_END
        : [out] "=r"(value), [fault] "=m"(excpt.fault_inst)
        : [pc] "r"(pc) : "t0", "a0", "ra", "memory");
    excpt.fault_inst = 0;
    return value;
}

bool be_it_vmodule_ecc_probe(void)
{
    if (!be_it_vmodule_available()) return false;
    uintptr_t pc = (uintptr_t)&vm_ecc_code[128];
    uint64_t fetchctl = CSRR(0x9e0), mnstatus = CSRR(CSR_MNSTATUS);
    uint64_t mncause = CSRR(CSR_MNCAUSE), mnepc = CSRR(CSR_MNEPC);
    bool saved_rnmi = rnmi_handler_enabled;
    rnmi_handler_enabled = true;
    CSRS(CSR_MNSTATUS, MNSTATUS_NMIE);
    vm_ecc_code[128] = 0x00150513;
    vm_ecc_code[129] = 0x00008067;
    backend_it_control.target_pc = pc;
    bool ok = true;
    for (unsigned attempt = 0; attempt < 2; ++attempt) {
        /* Configure before warming: the legacy configuration fences evict
         * ICache. ARM must use the atomic frontend command without fence.i. */
        fe_vm_addresses(pc, pc + 64, pc);
        CSRC(0x9e0, 1);
        TEST_SETUP_EXCEPT();
        ok &= vm_ecc_call(pc) == 42 && !excpt.triggered;
        CSRS(0x9e0, 1);
        TEST_SETUP_EXCEPT();
        for (unsigned warm = 0; warm < 8; ++warm)
            ok &= vm_ecc_call(pc) == 42 && !excpt.triggered;
        CSRW(CSR_MNCAUSE, 0);
        CSRW(CSR_MNEPC, 0);
        TEST_SETUP_EXCEPT();
        fe_vm_raw(6, 4);
        uint64_t value = vm_ecc_call(pc);
        /* A trained target may already be fetched before ARM reaches the
         * ALU. Keep requesting it until a post-ARM fetch consumes the ECC. */
        for (unsigned spin = 0; !excpt.triggered && !CSRR(CSR_MNCAUSE) && spin < BE_VM_SPINS; ++spin)
            value = vm_ecc_call(pc);
        bool iaf = excpt.triggered && excpt.cause == CAUSE_IAF &&
            excpt.epc == pc && excpt.tval == pc && value == 41;
        /* RNMI is asynchronous; mnepc need not equal the corrupted fetch PC. */
        bool rnmi = !excpt.triggered && CSRR(CSR_MNCAUSE) == (CAUSE_INT_BIT | 43ULL) &&
            CSRR(CSR_MNEPC) != 0 && !(CSRR(CSR_MNEPC) & 1) && value == 42;
        ok &= iaf || rnmi;
        /* CHECK is observed at ALU execution, so serialize it against the
         * call/wait branches. Only ARM must preserve warmed ICache lines. */
        fe_vm_config(7, 1);
        fe_vm_config(1, 0);
        TEST_SETUP_EXCEPT();
        ok &= vm_ecc_call(pc) == 42 && !excpt.triggered;
    }
    CSRW(0x9e0, fetchctl);
    CSRW(CSR_MNSTATUS, mnstatus);
    CSRW(CSR_MNCAUSE, mncause);
    CSRW(CSR_MNEPC, mnepc);
    rnmi_handler_enabled = saved_rnmi;
    return ok;
}
