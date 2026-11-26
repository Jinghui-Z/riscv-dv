#include <rvh_test.h>
#include <page_tables.h>
#include <stdint.h>

/*
 * VModule-only basic interrupt smoke cases.
 *
 * These cases intentionally do not use per-case #if VMODULE branches. Runner
 * selection owns the boundary: current ordinary LinkNan/Spike must not
 * register or run these cases. A valid run needs a VModule-capable
 * AP-IT/special-run/RTL environment with a whole-ELF VMODULE=1 build.
 * VMODULE=1 also changes the common interrupt handler source-clear path in
 * src/rvh_test.c, so VModule builds are not interchangeable with ordinary
 * CSR-pending interrupt gates.
 */

#define AI_VMODULE_REG_MAIN_INTERRUPT 0x1000ULL
#define AI_VMODULE_REG_INTERRUPT_DELAY 0x1028ULL
#define AI_VMODULE_MAIN_MSI 0x00000008ULL
#define AI_VMODULE_MAIN_MTI 0x00000080ULL
#define AI_VMODULE_MAIN_MEI 0x00000800ULL

#define AI_VMODULE_MIE_MTIE (1ULL << 7)
#define AI_VMODULE_MIE_STIE (1ULL << 5)
#define AI_VMODULE_MIP_STIP (1ULL << 5)
#define AI_VMODULE_CAUSE_MTI (CAUSE_INT_BIT | 7ULL)
#define AI_VMODULE_STIP_WRITE_ROUNDS 1024ULL

static inline void ai_vmodule_write_reg(uint64_t offset, uint64_t data)
{
    const uint64_t addr_cmd = 0xffff000100000000ULL | (offset & 0xffffffffULL);
    const uint64_t data_cmd = 0xffff000000000000ULL | (data & 0xffffffffULL);

    /* Keep x1/x2 intact in C; VModule only requires rd=x0 for the two writes. */
    asm volatile(
        "mv t0, %[addr]\n\t"
        "mv t1, %[data]\n\t"
        "fence.i\n\t"
        "add x0, x0, t0\n\t"
        "fence.i\n\t"
        "add x0, x0, t1\n\t"
        "fence.i\n\t"
        :
        : [addr] "r"(addr_cmd),
          [data] "r"(data_cmd)
        : "t0", "t1", "memory");
}

static inline void ai_vmodule_clear_main_interrupt(void)
{
    ai_vmodule_write_reg(AI_VMODULE_REG_MAIN_INTERRUPT, 0ULL);
}

static inline void ai_vmodule_set_delay(uint64_t delay_cycles)
{
    ai_vmodule_write_reg(AI_VMODULE_REG_INTERRUPT_DELAY, delay_cycles);
}

static inline void ai_vmodule_restore_m_interrupt_state(uint64_t saved_mstatus,
    uint64_t saved_mie, uint64_t saved_mideleg)
{
    CSRC(CSR_MSTATUS, MSTATUS_MIE);
    CSRW(CSR_MIE, saved_mie);
    CSRW(CSR_MIDELEG, saved_mideleg);
    CSRW(CSR_MIP, 0ULL);
    CSRW(CSR_MSTATUS, saved_mstatus);
}

static inline void ai_vmodule_prepare_m_interrupt(uint64_t enable_bit)
{
    goto_priv(PRIV_M);
    CSRC(CSR_MSTATUS, MSTATUS_MIE);
    CSRW(CSR_MIDELEG, 0ULL);
    CSRW(CSR_MIP, 0ULL);
    CSRW(CSR_MIE, enable_bit);
    CSRS(CSR_MSTATUS, MSTATUS_MIE);
}

static inline bool ai_vmodule_spin_until_trap(uint64_t max_iter)
{
    for (uint64_t i = 0; i < max_iter && !excpt.triggered; ++i) {
        asm volatile(
            ".option push\n\t"
            ".option norvc\n\t"
            "addi x0, x0, 0\n\t"
            ".option pop\n\t"
            ::: "memory");
    }

    return excpt.triggered;
}

static inline uint64_t ai_vmodule_write_mip_stip_until_trap(uint64_t max_rounds)
{
    volatile struct exception *const trap_state = &excpt;
    uint64_t rounds = 0ULL;

    for (; rounds < max_rounds && !trap_state->triggered; ++rounds) {
        asm volatile(
            ".option push\n\t"
            ".option norvc\n\t"
            "csrs mip, %[stip]\n\t"
            "csrs mip, %[stip]\n\t"
            "csrs mip, %[stip]\n\t"
            "csrs mip, %[stip]\n\t"
            ".option pop\n\t"
            :
            : [stip] "r"(AI_VMODULE_MIP_STIP)
            : "memory");
    }

    return rounds;
}

static inline uint64_t ai_vmodule_write_mip_stip_until_meip_pending(
    uint64_t max_rounds,
    uint64_t *observed_mip)
{
    volatile struct exception *const trap_state = &excpt;
    uint64_t rounds = 0ULL;
    uint64_t mip_snapshot = 0ULL;

    for (; rounds < max_rounds && !trap_state->triggered; ++rounds) {
        asm volatile(
            ".option push\n\t"
            ".option norvc\n\t"
            "csrs mip, %[stip]\n\t"
            "csrs mip, %[stip]\n\t"
            "csrs mip, %[stip]\n\t"
            "csrs mip, %[stip]\n\t"
            ".option pop\n\t"
            :
            : [stip] "r"(AI_VMODULE_MIP_STIP)
            : "memory");

        mip_snapshot = CSRR(CSR_MIP);
        if(mip_snapshot & MIP_MEIP){
            ++rounds;
            break;
        }
    }

    if(observed_mip != NULL){
        *observed_mip = mip_snapshot;
    }

    return rounds;
}

bool ai_vmodule_m_mei_basic_trap_corner()
{
    TEST_START();

    const uint64_t saved_mstatus = CSRR(CSR_MSTATUS);
    const uint64_t saved_mie = CSRR(CSR_MIE);
    const uint64_t saved_mideleg = CSRR(CSR_MIDELEG);

    ai_vmodule_clear_main_interrupt();
    ai_vmodule_set_delay(0ULL);
    TEST_SETUP_EXCEPT();
    ai_vmodule_prepare_m_interrupt(MIE_MEIE);
    ai_vmodule_write_reg(AI_VMODULE_REG_MAIN_INTERRUPT, AI_VMODULE_MAIN_MEI);
    const bool trapped = ai_vmodule_spin_until_trap(256ULL);

    ai_vmodule_clear_main_interrupt();
    ai_vmodule_restore_m_interrupt_state(saved_mstatus, saved_mie, saved_mideleg);

    TEST_ASSERT("VModule vm_reg_0 MEI injection is taken as one M-mode machine external interrupt",
        trapped &&
        excpt.triggered &&
        excpt.priv == PRIV_M &&
        excpt.cause == CAUSE_MEI);

    TEST_END(ai_vmodule_m_mei_basic_trap_corner);
}

bool ai_vmodule_m_msi_basic_trap_corner()
{
    TEST_START();

    const uint64_t saved_mstatus = CSRR(CSR_MSTATUS);
    const uint64_t saved_mie = CSRR(CSR_MIE);
    const uint64_t saved_mideleg = CSRR(CSR_MIDELEG);

    ai_vmodule_clear_main_interrupt();
    ai_vmodule_set_delay(0ULL);
    TEST_SETUP_EXCEPT();
    ai_vmodule_prepare_m_interrupt(MIE_MSIE);
    ai_vmodule_write_reg(AI_VMODULE_REG_MAIN_INTERRUPT, AI_VMODULE_MAIN_MSI);
    const bool trapped = ai_vmodule_spin_until_trap(256ULL);

    ai_vmodule_clear_main_interrupt();
    ai_vmodule_restore_m_interrupt_state(saved_mstatus, saved_mie, saved_mideleg);

    TEST_ASSERT("VModule vm_reg_0 MSI injection is taken as one M-mode machine software interrupt",
        trapped &&
        excpt.triggered &&
        excpt.priv == PRIV_M &&
        excpt.cause == CAUSE_MSI);

    TEST_END(ai_vmodule_m_msi_basic_trap_corner);
}

bool ai_vmodule_m_mti_basic_trap_corner()
{
    TEST_START();

    const uint64_t saved_mstatus = CSRR(CSR_MSTATUS);
    const uint64_t saved_mie = CSRR(CSR_MIE);
    const uint64_t saved_mideleg = CSRR(CSR_MIDELEG);

    ai_vmodule_clear_main_interrupt();
    ai_vmodule_set_delay(0ULL);
    TEST_SETUP_EXCEPT();
    ai_vmodule_prepare_m_interrupt(AI_VMODULE_MIE_MTIE);
    ai_vmodule_write_reg(AI_VMODULE_REG_MAIN_INTERRUPT, AI_VMODULE_MAIN_MTI);
    const bool trapped = ai_vmodule_spin_until_trap(256ULL);

    ai_vmodule_clear_main_interrupt();
    ai_vmodule_restore_m_interrupt_state(saved_mstatus, saved_mie, saved_mideleg);

    TEST_ASSERT("VModule vm_reg_0 MTI injection is taken as one M-mode machine timer interrupt",
        trapped &&
        excpt.triggered &&
        excpt.priv == PRIV_M &&
        excpt.cause == AI_VMODULE_CAUSE_MTI);

    TEST_END(ai_vmodule_m_mti_basic_trap_corner);
}

bool ai_vmodule_m_mei_delayed_trap_progress_corner()
{
    TEST_START();

    const uint64_t saved_mstatus = CSRR(CSR_MSTATUS);
    const uint64_t saved_mie = CSRR(CSR_MIE);
    const uint64_t saved_mideleg = CSRR(CSR_MIDELEG);
    const uintptr_t marker_addr = phys_page_base(VSRWX_GRWX) + 0x380U;
    const uint64_t marker_before = 0x1122334455667788ULL;
    const uint64_t marker_after = 0x8877665544332211ULL;
    uint64_t observed_before_trap = 0ULL;

    write64(marker_addr, marker_before);
    fence_iorw_iorw();

    ai_vmodule_clear_main_interrupt();
    ai_vmodule_set_delay(16ULL);
    TEST_SETUP_EXCEPT();
    ai_vmodule_prepare_m_interrupt(MIE_MEIE);
    ai_vmodule_write_reg(AI_VMODULE_REG_MAIN_INTERRUPT, AI_VMODULE_MAIN_MEI);

    observed_before_trap = read64(marker_addr);
    write64(marker_addr, marker_after);
    fence_iorw_iorw();
    const bool trapped = ai_vmodule_spin_until_trap(512ULL);

    ai_vmodule_clear_main_interrupt();
    ai_vmodule_set_delay(0ULL);
    ai_vmodule_restore_m_interrupt_state(saved_mstatus, saved_mie, saved_mideleg);

    TEST_ASSERT("VModule delayed MEI still reaches the M-mode handler after ordinary memory progress",
        trapped &&
        excpt.triggered &&
        excpt.priv == PRIV_M &&
        excpt.cause == CAUSE_MEI);
    TEST_ASSERT("ordinary memory operations around the delayed VModule interrupt are not skipped or duplicated",
        observed_before_trap == marker_before &&
        read64(marker_addr) == marker_after);

    TEST_END(ai_vmodule_m_mei_delayed_trap_progress_corner);
}

bool ai_vmodule_m_mei_during_repeated_mip_stip_writes_corner()
{
    TEST_START();

    const uint64_t saved_mstatus = CSRR(CSR_MSTATUS);
    const uint64_t saved_mie = CSRR(CSR_MIE);
    const uint64_t saved_mideleg = CSRR(CSR_MIDELEG);
    const uint64_t saved_menvcfg = CSRR(CSR_MENVCFG);
    uint64_t observed_mip = 0ULL;

    ai_vmodule_clear_main_interrupt();
    ai_vmodule_set_delay(128ULL);
    TEST_SETUP_EXCEPT();
    goto_priv(PRIV_M);
    CSRC(CSR_MSTATUS, MSTATUS_MIE);
    CSRC(CSR_MENVCFG, MENVCFG_STCE);
    CSRW(CSR_MIDELEG, 0ULL);
    CSRW(CSR_MIP, 0ULL);
    CSRW(CSR_MIE, 0ULL);
    CSRS(CSR_MSTATUS, MSTATUS_MIE);
    ai_vmodule_write_reg(AI_VMODULE_REG_MAIN_INTERRUPT, AI_VMODULE_MAIN_MEI);

    const uint64_t stip_rounds =
        ai_vmodule_write_mip_stip_until_meip_pending(
            AI_VMODULE_STIP_WRITE_ROUNDS,
            &observed_mip);

    CSRC(CSR_MSTATUS, MSTATUS_MIE);
    ai_vmodule_clear_main_interrupt();
    ai_vmodule_set_delay(0ULL);
    CSRW(CSR_MENVCFG, saved_menvcfg);
    ai_vmodule_restore_m_interrupt_state(saved_mstatus, saved_mie, saved_mideleg);

    TEST_ASSERT("MEIE and STIE disabled keep the VModule MEIP pending injection from entering the handler",
        excpt.triggered == false &&
        (observed_mip & MIP_MEIP) &&
        (observed_mip & AI_VMODULE_MIP_STIP));
    TEST_ASSERT("VModule MEIP pending is observed while repeated CSR writes set mip.STIP",
        stip_rounds > 0ULL &&
        stip_rounds < AI_VMODULE_STIP_WRITE_ROUNDS);

    TEST_END(ai_vmodule_m_mei_during_repeated_mip_stip_writes_corner);
}
