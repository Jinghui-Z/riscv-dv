#include <rvh_test.h>
#include <stdint.h>

#define AI_ARCH_PRIV_COUNTER_CY (1ULL << 0)
#define AI_ARCH_PRIV_COUNTER_TM (1ULL << 1)
#define AI_ARCH_PRIV_COUNTER_IR (1ULL << 2)
#define AI_ARCH_PRIV_COUNTER_CY_IR \
    (AI_ARCH_PRIV_COUNTER_CY | AI_ARCH_PRIV_COUNTER_IR)
#ifndef AI_ARCH_PRIV_CLINT_BASE
#define AI_ARCH_PRIV_CLINT_BASE 0x02000000UL
#endif
#define AI_ARCH_PRIV_CLINT_MTIMECMP0 (AI_ARCH_PRIV_CLINT_BASE + 0x4000U)
#define AI_ARCH_PRIV_CLINT_MTIME     (AI_ARCH_PRIV_CLINT_BASE + 0xbff8U)
#define AI_ARCH_PRIV_MIP_MTIP        (1ULL << 7)

static volatile uint64_t ai_arch_priv_counter_sink;

__attribute__((noinline)) static void ai_arch_priv_counter_spin(uint64_t rounds)
{
    for (volatile uint64_t i = 0; i < rounds; ++i) {
        ai_arch_priv_counter_sink += (i ^ 0x5a5a5a5aULL);
        asm volatile("addi x0, x0, 0" ::: "memory");
    }
}

static inline uint64_t ai_arch_priv_user_read_cycle(uint64_t sentinel)
{
    uint64_t value = sentinel;

    asm volatile("csrr %0, cycle" : "+r"(value) :: "memory");
    return value;
}

static inline uint64_t ai_arch_priv_user_read_instret(uint64_t sentinel)
{
    uint64_t value = sentinel;

    asm volatile("csrr %0, instret" : "+r"(value) :: "memory");
    return value;
}

static inline uint64_t ai_arch_priv_counter_read64(uintptr_t addr)
{
    return *(volatile uint64_t *)addr;
}

static inline void ai_arch_priv_counter_write64(uintptr_t addr, uint64_t value)
{
    *(volatile uint64_t *)addr = value;
}

bool ai_arch_mcounteren_disable_does_not_stop_mcycle_minstret_corner()
{
    TEST_START();

    goto_priv(PRIV_M);
    const uint64_t saved_mcounteren = CSRR(CSR_MCOUNTEREN);
    const uint64_t saved_scounteren = CSRR(CSR_SCOUNTEREN);
    const uint64_t saved_mcountinhibit = CSRR(CSR_MCOUNTINHIBIT);

    CSRC(CSR_MCOUNTINHIBIT, AI_ARCH_PRIV_COUNTER_CY_IR);
    CSRS(CSR_MCOUNTEREN, AI_ARCH_PRIV_COUNTER_CY_IR);
    CSRS(CSR_SCOUNTEREN, AI_ARCH_PRIV_COUNTER_CY_IR);

    goto_priv(PRIV_HU);
    TEST_SETUP_EXCEPT();
    const uint64_t user_cycle_before =
        ai_arch_priv_user_read_cycle(0xdead0000ULL);
    TEST_ASSERT("with mcounteren/scounteren CY set, HU can read the cycle CSR shadow without trapping",
        excpt.triggered == false);

    TEST_SETUP_EXCEPT();
    const uint64_t user_instret_before =
        ai_arch_priv_user_read_instret(0xbeef0000ULL);
    TEST_ASSERT("with mcounteren/scounteren IR set, HU can read the instret CSR shadow without trapping",
        excpt.triggered == false);

    goto_priv(PRIV_M);
    const uint64_t mcycle_before_disable = CSRR(CSR_MCYCLE);
    const uint64_t minstret_before_disable = CSRR(CSR_MINSTRET);

    CSRC(CSR_MCOUNTEREN, AI_ARCH_PRIV_COUNTER_CY_IR);
    CSRC(CSR_SCOUNTEREN, AI_ARCH_PRIV_COUNTER_CY_IR);
    ai_arch_priv_counter_spin(64);

    const uint64_t mcycle_after_disabled_spin = CSRR(CSR_MCYCLE);
    const uint64_t minstret_after_disabled_spin = CSRR(CSR_MINSTRET);
    TEST_ASSERT("clearing mcounteren.CY only removes lower-privilege access and does not stop the underlying mcycle counter",
        mcycle_after_disabled_spin > mcycle_before_disable);
    TEST_ASSERT("clearing mcounteren.IR only removes lower-privilege access and does not stop the underlying minstret counter",
        minstret_after_disabled_spin > minstret_before_disable);

    goto_priv(PRIV_HU);
    TEST_SETUP_EXCEPT();
    const uint64_t trapped_cycle =
        ai_arch_priv_user_read_cycle(0x1111222233334444ULL);
    TEST_ASSERT("with mcounteren.CY clear, HU cycle reads trap as illegal instructions",
        excpt.triggered == true &&
        excpt.cause == CAUSE_ILI &&
        trapped_cycle == 0x1111222233334444ULL);

    goto_priv(PRIV_HU);
    TEST_SETUP_EXCEPT();
    const uint64_t trapped_instret =
        ai_arch_priv_user_read_instret(0x5555666677778888ULL);
    TEST_ASSERT("with mcounteren.IR clear, HU instret reads trap as illegal instructions",
        excpt.triggered == true &&
        excpt.cause == CAUSE_ILI &&
        trapped_instret == 0x5555666677778888ULL);

    goto_priv(PRIV_M);
    CSRS(CSR_MCOUNTEREN, AI_ARCH_PRIV_COUNTER_CY_IR);
    CSRS(CSR_SCOUNTEREN, AI_ARCH_PRIV_COUNTER_CY_IR);

    goto_priv(PRIV_HU);
    TEST_SETUP_EXCEPT();
    const uint64_t user_cycle_after_reenable =
        ai_arch_priv_user_read_cycle(0xdead1111ULL);
    TEST_ASSERT("after mcounteren.CY is restored, HU cycle reads are trap-free and observe the continuing mcycle shadow",
        excpt.triggered == false &&
        user_cycle_after_reenable >= mcycle_after_disabled_spin &&
        user_cycle_after_reenable >= user_cycle_before);

    TEST_SETUP_EXCEPT();
    const uint64_t user_instret_after_reenable =
        ai_arch_priv_user_read_instret(0xbeef1111ULL);
    TEST_ASSERT("after mcounteren.IR is restored, HU instret reads are trap-free and observe the continuing minstret shadow",
        excpt.triggered == false &&
        user_instret_after_reenable >= minstret_after_disabled_spin &&
        user_instret_after_reenable >= user_instret_before);

    goto_priv(PRIV_M);
    CSRW(CSR_MCOUNTEREN, saved_mcounteren);
    CSRW(CSR_SCOUNTEREN, saved_scounteren);
    CSRW(CSR_MCOUNTINHIBIT, saved_mcountinhibit);

    TEST_END(ai_arch_mcounteren_disable_does_not_stop_mcycle_minstret_corner);
}

bool ai_arch_mtime_mtimecmp_mmio_mtip_shadow_corner()
{
    TEST_START();

    const uintptr_t mtime_addr = AI_ARCH_PRIV_CLINT_MTIME;
    const uintptr_t mtimecmp_addr = AI_ARCH_PRIV_CLINT_MTIMECMP0;

    goto_priv(PRIV_M);
    const uint64_t saved_mie = CSRR(CSR_MIE);
    const uint64_t saved_mip = CSRR(CSR_MIP);
    const uint64_t saved_mcounteren = CSRR(CSR_MCOUNTEREN);
    const uint64_t saved_scounteren = CSRR(CSR_SCOUNTEREN);
    const uint64_t saved_mtime = ai_arch_priv_counter_read64(mtime_addr);
    const uint64_t saved_mtimecmp = ai_arch_priv_counter_read64(mtimecmp_addr);

    CSRC(CSR_MIE, AI_ARCH_PRIV_MIP_MTIP);
    CSRS(CSR_MCOUNTEREN, AI_ARCH_PRIV_COUNTER_TM);
    CSRS(CSR_SCOUNTEREN, AI_ARCH_PRIV_COUNTER_TM);

    TEST_ASSERT("mtime and mtimecmp use naturally aligned 64-bit CLINT MMIO slots",
        (mtime_addr & (sizeof(uint64_t) - 1U)) == 0U &&
        (mtimecmp_addr & (sizeof(uint64_t) - 1U)) == 0U);

    const uint64_t programmed_mtime = saved_mtime + 0x1000ULL;
    const uint64_t programmed_cmp_late = programmed_mtime + 0x1000ULL;
    const uint64_t programmed_cmp_early = programmed_mtime - 0x10ULL;

    ai_arch_priv_counter_write64(mtimecmp_addr, programmed_cmp_late);
    ai_arch_priv_counter_write64(mtime_addr, programmed_mtime);
    fence_iorw_iorw();

    const uint64_t mtime_after_write =
        ai_arch_priv_counter_read64(mtime_addr);
    const uint64_t mtimecmp_after_write =
        ai_arch_priv_counter_read64(mtimecmp_addr);
    const uint64_t time_shadow_after_write = CSRR(CSR_TIME);

    TEST_ASSERT("64-bit aligned MMIO writes update mtime/mtimecmp and the time CSR shadows mtime monotonically",
        mtime_after_write >= programmed_mtime &&
        mtimecmp_after_write == programmed_cmp_late &&
        time_shadow_after_write >= mtime_after_write);

    ai_arch_priv_counter_write64(mtimecmp_addr, programmed_cmp_early);
    fence_iorw_iorw();
    uint64_t mtip_after_early_cmp = 0ULL;
    for (uint32_t i = 0; i < 128U; ++i) {
        mtip_after_early_cmp = CSRR(CSR_MIP) & AI_ARCH_PRIV_MIP_MTIP;
        if (mtip_after_early_cmp != 0ULL) {
            break;
        }
        ai_arch_priv_counter_spin(1);
    }

    ai_arch_priv_counter_write64(mtimecmp_addr,
        ai_arch_priv_counter_read64(mtime_addr) + 0x100000ULL);
    fence_iorw_iorw();
    uint64_t mtip_after_late_cmp = AI_ARCH_PRIV_MIP_MTIP;
    for (uint32_t i = 0; i < 128U; ++i) {
        mtip_after_late_cmp = CSRR(CSR_MIP) & AI_ARCH_PRIV_MIP_MTIP;
        if (mtip_after_late_cmp == 0ULL) {
            break;
        }
        ai_arch_priv_counter_spin(1);
    }

    TEST_ASSERT("moving mtimecmp below then above mtime eventually reflects in MTIP without requiring an immediate transition",
        mtip_after_early_cmp != 0ULL &&
        mtip_after_late_cmp == 0ULL);

    ai_arch_priv_counter_write64(mtimecmp_addr, saved_mtimecmp);
    ai_arch_priv_counter_write64(mtime_addr, saved_mtime);
    fence_iorw_iorw();
    CSRW(CSR_MCOUNTEREN, saved_mcounteren);
    CSRW(CSR_SCOUNTEREN, saved_scounteren);
    CSRW(CSR_MIE, saved_mie);
    if ((saved_mip & AI_ARCH_PRIV_MIP_MTIP) != 0ULL) {
        CSRS(CSR_MIP, AI_ARCH_PRIV_MIP_MTIP);
    } else {
        CSRC(CSR_MIP, AI_ARCH_PRIV_MIP_MTIP);
    }

    TEST_END(ai_arch_mtime_mtimecmp_mmio_mtip_shadow_corner);
}
