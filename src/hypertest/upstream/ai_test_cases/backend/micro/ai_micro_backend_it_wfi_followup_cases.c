#include "backend_it_vmodule.h"

#define BE_WFI_SRNCTL 0x5c4

/* nhv5_1_ap; manual; reason_code: D-MANUAL-NONGATE.
 * No default registration. Requires the scenario-specific monitor contracts
 * described in runs/backend_uncover_20261008/wfi_work/integration.md.
 * JTAG owns Debug CSR accesses; L2 state comes only from real transactions.
 */
extern const uint32_t be_it_followup_step_wfi[];
asm(".pushsection .text.be_it_followup_step,\"ax\",@progbits\n"
    BE_ASM_BEGIN ".balign 64\n"
    ".global be_it_followup_step_wfi\nbe_it_followup_step_wfi: wfi\n"
    /* Single-step stops before this instruction. The debugger restores the
     * original parked DPC, so this island never consumes a C return address. */
    "1: nop\nj 1b\n" BE_ASM_END ".popsection\n");

static volatile uint64_t wfi_tail_data[10][8] __attribute__((aligned(4096)));

bool ai_micro_backend_it_wfi_single_step(void)
{
    TEST_START();
    be_it_enable();
    if (!be_it_vmodule_available()) return false;
    be_it_trap_install();
    bool source_ok = be_vm_clear();
    uint64_t saved_srnctl = CSRR(BE_WFI_SRNCTL);
    CSRS(BE_WFI_SRNCTL, 4);
    backend_it_control.target_pc = (uintptr_t)be_it_followup_step_wfi;
    backend_it_control.target_addr = (uintptr_t)be_it_followup_step_wfi + 4;
    backend_it_control.auxiliary = 0;
    if (!source_ok || !be_it_prepare_scenario(757, BE_IT_SC_DEBUG_WFI_STEP, 0)) {
        CSRW(BE_WFI_SRNCTL, saved_srnctl);
        be_it_trap_restore();
        return false;
    }
    if (!(backend_it_control.capabilities & BE_IT_CAP_DEBUG)) {
        CSRW(BE_WFI_SRNCTL, saved_srnctl);
        be_it_trap_restore();
        return false;
    }
    volatile uint64_t resumed = 0;
    be_it_activate();
    /* The agent halts this normal call path, executes one isolated WFI and
     * restores the stopped PC. Finish waits for checked/resume and the
     * WFI-commit -> successor single-step witness in this same epoch. */
    bool checked = be_it_finish();
    resumed++;
    source_ok &= be_vm_clear();
    uint64_t traps = be_it_traps.count;
    CSRW(BE_WFI_SRNCTL, saved_srnctl);
    be_it_trap_restore();
    TEST_ASSERT("JTAG stepped WFI, checked cause4/DPC and restored normal execution", checked);
    TEST_ASSERT("WFI single step resumes its parked frame once without an M trap",
        source_ok && resumed == 1 && traps == 0);
    TEST_END("ai_micro_backend_it_wfi_single_step");
}

/* Keep one descriptor PC across the finite scan; cloning this inline asm
 * would duplicate the global WFI label. */
__attribute__((optimize("no-peel-loops", "no-unswitch-loops", "no-split-loops")))
bool ai_micro_backend_it_wfi_l2_tail_scan(void)
{
    TEST_START();
    be_it_enable();
    if (!be_it_vmodule_available()) return false;
    be_it_trap_install();
    bool source_ok = be_vm_clear();
    uint64_t saved_srnctl = CSRR(BE_WFI_SRNCTL);
    uint64_t saved_prefetch = CSRR(0x5c1);
    CSRS(BE_WFI_SRNCTL, 4);
    CSRW(0x5c1, saved_prefetch & ~1ULL);
    asm volatile("fence.i" ::: "memory");
    uintptr_t wfi_pc;
    asm volatile("lla %0,be_it_followup_l2_wfi" : "=r"(wfi_pc));
    backend_it_control.target_pc = wfi_pc;
    backend_it_control.target_addr = (uintptr_t)&wfi_tail_data[2][0];
    backend_it_control.auxiliary = 2048;
    if (!source_ok || !be_it_begin_scenario(753, BE_IT_SC_WFI_L2_TAIL_SCAN, 64)) {
        CSRW(0x5c1, saved_prefetch);
        CSRW(BE_WFI_SRNCTL, saved_srnctl);
        be_it_trap_restore();
        return false;
    }
    wfi_tail_data[0][0] = 73;
    wfi_tail_data[1][0] = 0;
    wfi_tail_data[1][1] = 0x123456789abcdef0ULL;
    bool outcomes_ok = true;
    unsigned completed = 0;
    for (unsigned phase = 0; phase < 64; ++phase) {
        CSRC(CSR_MSTATUS, MSTATUS_MIE);
        CSRW(CSR_MIE, 0);
        be_it_traps.count = 0;
        be_it_traps.event[0].cause = UINT64_MAX;
        be_it_traps.defer_irq_clear = 0;
        for (unsigned line = 2; line < 10; ++line) {
            wfi_tail_data[line][0] = 0;
            wfi_tail_data[line][1] = 0xfeed0000ULL + line;
            asm volatile("cbo.flush 0(%0)" :: "r"(&wfi_tail_data[line][0]) : "memory");
        }
        asm volatile("fence rw,rw" ::: "memory");
        be_vm_watchdog(2000000);
        /* An early pending IRQ cancels wfiReq before both safe inputs rise.
         * The late source only bounds WFI; it is not the L2-only oracle. */
        be_vm_mei(2048, false);
        CSRW(CSR_MIE, BE_VM_MEI);
        CSRS(CSR_MSTATUS, MSTATUS_MIE);
        asm volatile(BE_ASM_BEGIN
            "ld t0,0(%[p])\n"
            "sd t0,128(%[p])\nsd t0,192(%[p])\nsd t0,256(%[p])\n"
            "sd t0,320(%[p])\nsd t0,384(%[p])\nsd t0,448(%[p])\n"
            "sd t0,512(%[p])\nsd t0,576(%[p])\n"
            "mv t1,%[phase]\nbeqz t1,2f\n"
            "1: addi t1,t1,-1\nbnez t1,1b\n2:\n"
            /* No CSR or fence drains the real tail between stores and WFI. */
            ".global be_it_followup_l2_wfi\nbe_it_followup_l2_wfi: wfi\n"
            "addi t0,t0,1\n"
            "ld t1,64(%[p])\naddi t1,t1,1\nsd t1,64(%[p])\n"
            BE_ASM_END :: [p] "r"(wfi_tail_data), [phase] "r"((uint64_t)phase)
            : "t0", "t1", "memory");
        bool got_irq = be_vm_wait_traps(1);
        CSRC(CSR_MSTATUS, MSTATUS_MIE);
        bool pending_clear = be_vm_pending(false);
        source_ok &= be_vm_clear();
        bool data_ok = wfi_tail_data[0][0] == 73 &&
            wfi_tail_data[1][0] == phase + 1 &&
            wfi_tail_data[1][1] == 0x123456789abcdef0ULL;
        for (unsigned line = 2; line < 10; ++line)
            data_ok &= wfi_tail_data[line][0] == 73 &&
                wfi_tail_data[line][1] == 0xfeed0000ULL + line;
        bool attempt_ok = got_irq && pending_clear && source_ok && data_ok &&
            be_it_traps.count == 1 &&
            be_it_traps.event[0].cause == ((1ULL << 63) | 11);
        outcomes_ok &= attempt_ok;
        completed++;
        if (!attempt_ok) {
            printf("\nBACKEND_IT_WFI_TAIL phase=%u traps=%lu cause=%lx marker=%lu pending_clear=%u data=%u\n",
                phase, be_it_traps.count, be_it_traps.event[0].cause,
                wfi_tail_data[1][0], pending_clear, data_ok);
            break;
        }
    }
    be_vm_watchdog(UINT32_MAX);
    /* The 0x7003 contract must require req && FE_safe && MEM_safe && L2_busy.
     * The old row753 IRQ sequence is deliberately not a substitute. */
    bool window_ok = be_it_finish();
    source_ok &= be_vm_clear();
    CSRW(0x5c1, saved_prefetch);
    CSRW(BE_WFI_SRNCTL, saved_srnctl);
    be_it_trap_restore();
    TEST_ASSERT("64 bounded WFI attempts preserve eight cold stores and guards, one IRQ/marker each",
        outcomes_ok && source_ok && completed == 64);
    TEST_ASSERT("WFI window observed FE_safe=MEM_safe=1 with a real outstanding L2 tail",
        window_ok);
    TEST_END("ai_micro_backend_it_wfi_l2_tail_scan");
}
