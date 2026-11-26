#include <rvh_test.h>
#include <page_tables.h>
#include <stdint.h>

#define AI_ARCH_PRIV_CY (1ULL << 0)
#define AI_ARCH_PRIV_TM (1ULL << 1)
#define AI_ARCH_PRIV_IR (1ULL << 2)
#define AI_ARCH_PRIV_HPM3 (1ULL << 3)
#define AI_ARCH_PRIV_HPM31 (1ULL << 31)
#define AI_ARCH_PRIV_COUNTER_BITS \
    (AI_ARCH_PRIV_CY | AI_ARCH_PRIV_TM | AI_ARCH_PRIV_IR | \
     AI_ARCH_PRIV_HPM3 | AI_ARCH_PRIV_HPM31)

#define AI_ARCH_PRIV_MSTATUS_VS_OFF    (9U)
#define AI_ARCH_PRIV_MSTATUS_VS_MASK   (3ULL << AI_ARCH_PRIV_MSTATUS_VS_OFF)
#define AI_ARCH_PRIV_MSTATUS_VS_INITIAL (1ULL << AI_ARCH_PRIV_MSTATUS_VS_OFF)
#define AI_ARCH_PRIV_MSTATUS_VS_CLEAN  (2ULL << AI_ARCH_PRIV_MSTATUS_VS_OFF)
#define AI_ARCH_PRIV_MSTATUS_VS_DIRTY  (3ULL << AI_ARCH_PRIV_MSTATUS_VS_OFF)
#define AI_ARCH_PRIV_MSTATUS_XS_MASK   SSTATUS_XS_MSK
#define AI_ARCH_PRIV_MENVCFG_LPE       (1ULL << 2)
#define AI_ARCH_PRIV_MENVCFG_SSE       (1ULL << 3)
#define AI_ARCH_PRIV_MENVCFG_LPE_SSE \
    (AI_ARCH_PRIV_MENVCFG_LPE | AI_ARCH_PRIV_MENVCFG_SSE)
#define AI_ARCH_PRIV_MSTATUS_MPP_MASK (3ULL << 11)
#define AI_ARCH_PRIV_MSTATUS_MPP_M    (3ULL << 11)
#define AI_ARCH_PRIV_PTE_SVNAPOT      (1ULL << 63)

#ifndef MSTATEEN_SE0
#define MSTATEEN_SE0 (1ULL << 63)
#endif

static volatile uint64_t ai_arch_priv_csr_sink;
static uint64_t ai_arch_priv_xret_lrsc_words[4] __attribute__((aligned(64)));

__attribute__((noinline)) static void ai_arch_priv_csr_spin(uint64_t rounds)
{
    for (volatile uint64_t i = 0; i < rounds; ++i) {
        ai_arch_priv_csr_sink += (i + 0x9e3779b97f4a7c15ULL);
        asm volatile("addi x0, x0, 0" ::: "memory");
    }
}

static inline uint64_t ai_arch_priv_read_cycle(uint64_t sentinel)
{
    uint64_t value = sentinel;

    asm volatile("csrr %0, cycle" : "+r"(value) :: "memory");
    return value;
}

static inline uint64_t ai_arch_priv_read_time(uint64_t sentinel)
{
    uint64_t value = sentinel;

    asm volatile("csrr %0, time" : "+r"(value) :: "memory");
    return value;
}

static inline uint64_t ai_arch_priv_read_instret(uint64_t sentinel)
{
    uint64_t value = sentinel;

    asm volatile("csrr %0, instret" : "+r"(value) :: "memory");
    return value;
}

static inline uint64_t ai_arch_priv_read_hpmcounter3(uint64_t sentinel)
{
    uint64_t value = sentinel;

    asm volatile("csrr %0, hpmcounter3" : "+r"(value) :: "memory");
    return value;
}

static inline uint64_t ai_arch_priv_read_hpmcounter31(uint64_t sentinel)
{
    uint64_t value = sentinel;

    asm volatile("csrr %0, hpmcounter31" : "+r"(value) :: "memory");
    return value;
}

static inline void ai_arch_priv_write_mconfigptr(uint64_t value)
{
    asm volatile("csrw mconfigptr, %0" :: "rK"(value) : "memory");
}

static inline void ai_arch_priv_write_sstateen0(uint64_t value)
{
    asm volatile("csrw sstateen0, %0" :: "rK"(value) : "memory");
}

static inline uint64_t ai_arch_priv_read_sstateen0(uint64_t sentinel)
{
    uint64_t value = sentinel;

    asm volatile("csrr %0, sstateen0" : "+r"(value) :: "memory");
    return value;
}

static inline void ai_arch_priv_sinval_vma(void)
{
    asm volatile(
        ".insn r 0x73, 0x0, 0x0B, x0, x0, x0\n\t"
        ::: "memory");
}

static inline void ai_arch_priv_sfence_w_inval(void)
{
    asm volatile(".word 0x18000073\n\t" ::: "memory");
}

static inline void ai_arch_priv_sfence_inval_ir(void)
{
    asm volatile(".word 0x18100073\n\t" ::: "memory");
}

static inline pte_t *ai_arch_priv_hs_leaf_entry(enum test_page page)
{
#ifdef sv48
    return &hspt[4][page];
#else
    return &hspt[2][page];
#endif
}

static inline void ai_arch_priv_mret_sc_d_sequence(uintptr_t addr,
    uint64_t payload, uint64_t *lr_value, uint64_t *sc_status)
{
    asm volatile(
        ".option push\n\t"
        ".option norvc\n\t"
        "lr.d %[lr], 0(%[addr])\n\t"
        "lla t0, 1f\n\t"
        "csrw mepc, t0\n\t"
        "mret\n\t"
        "1:\n\t"
        "sc.d %[status], %[payload], 0(%[addr])\n\t"
        ".option pop\n\t"
        : [lr] "=&r"(*lr_value), [status] "=&r"(*sc_status)
        : [addr] "r"(addr), [payload] "r"(payload)
        : "t0", "memory");
}

static inline void ai_arch_priv_sret_sc_d_sequence(uintptr_t addr,
    uint64_t payload, uint64_t *lr_value, uint64_t *sc_status)
{
    asm volatile(
        ".option push\n\t"
        ".option norvc\n\t"
        "lr.d %[lr], 0(%[addr])\n\t"
        "lla t0, 1f\n\t"
        "csrw sepc, t0\n\t"
        "sret\n\t"
        "1:\n\t"
        "sc.d %[status], %[payload], 0(%[addr])\n\t"
        ".option pop\n\t"
        : [lr] "=&r"(*lr_value), [status] "=&r"(*sc_status)
        : [addr] "r"(addr), [payload] "r"(payload)
        : "t0", "memory");
}

bool ai_arch_mconfigptr_ro_mtval_warl_mseccfg_rw_corner()
{
    TEST_START();

    goto_priv(PRIV_M);
    const uint64_t saved_mtval = CSRR(CSR_MTVAL);
    const uint64_t saved_mseccfg = CSRR(CSR_MSECCFG);
    const uint64_t saved_mstatus = CSRR(CSR_MSTATUS);
    const uint64_t saved_mconfigptr = CSRR(CSR_MCONFIGPTR);
    const uint64_t mtval_pattern = 0x123456789abcdef0ULL;

    TEST_SETUP_EXCEPT();
    CSRW(CSR_MTVAL, mtval_pattern);
    const uint64_t mtval_after_write = CSRR(CSR_MTVAL);
    TEST_ASSERT("M-mode can write mtval as a WARL CSR and read back a stable implemented value",
        excpt.triggered == false &&
        mtval_after_write == CSRR(CSR_MTVAL));

    TEST_SETUP_EXCEPT();
    CSRW(CSR_MSECCFG, saved_mseccfg ^ MSECCFG_PMM3);
    const uint64_t mseccfg_after_toggle = CSRR(CSR_MSECCFG);
    TEST_ASSERT("mseccfg accepts M-mode CSR writes and exposes a stable WARL readback",
        excpt.triggered == false &&
        mseccfg_after_toggle == CSRR(CSR_MSECCFG));

    TEST_SETUP_EXCEPT();
    ai_arch_priv_write_mconfigptr(saved_mconfigptr ^ 0x40ULL);
    TEST_ASSERT("mconfigptr is read-only and rejects CSR writes with illegal instruction",
        excpt.triggered == true &&
        excpt.cause == CAUSE_ILI);

    TEST_ASSERT("the illegal mconfigptr write does not modify the read-only CSR value",
        CSRR(CSR_MCONFIGPTR) == saved_mconfigptr);

    CSRW(CSR_MTVAL, saved_mtval);
    CSRW(CSR_MSECCFG, saved_mseccfg);
    CSRW(CSR_MSTATUS, saved_mstatus);

    TEST_END(ai_arch_mconfigptr_ro_mtval_warl_mseccfg_rw_corner);
}

bool ai_arch_counter_write_and_shadow_access_matrix_corner()
{
    TEST_START();

    goto_priv(PRIV_M);
    const uint64_t saved_mcounteren = CSRR(CSR_MCOUNTEREN);
    const uint64_t saved_scounteren = CSRR(CSR_SCOUNTEREN);
    const uint64_t saved_mcountinhibit = CSRR(CSR_MCOUNTINHIBIT);

    CSRS(CSR_MCOUNTINHIBIT, AI_ARCH_PRIV_COUNTER_BITS);
    CSRW(CSR_MCYCLE, 0x1000ULL);
    CSRW(CSR_MINSTRET, 0x2000ULL);
    CSRW(CSR_MHPMCOUNTER3, 0x3000ULL);
    CSRW(CSR_MHPMCOUNTER31, 0x31000ULL);

    TEST_ASSERT("M-mode can seed mcycle, minstret, and HPM counter endpoints through CSR writes",
        CSRR(CSR_MCYCLE) == 0x1000ULL &&
        CSRR(CSR_MINSTRET) == 0x2000ULL &&
        CSRR(CSR_MHPMCOUNTER3) == 0x3000ULL &&
        CSRR(CSR_MHPMCOUNTER31) == 0x31000ULL);

    CSRW(CSR_MCOUNTEREN, AI_ARCH_PRIV_COUNTER_BITS);
    CSRW(CSR_SCOUNTEREN, AI_ARCH_PRIV_COUNTER_BITS);

    goto_priv(PRIV_HU);
    TEST_SETUP_EXCEPT();
    const uint64_t cycle_open = ai_arch_priv_read_cycle(0x1111ULL);
    TEST_ASSERT("HU cycle is trap-free when mcounteren.CY and scounteren.CY are both set",
        excpt.triggered == false &&
        cycle_open >= 0x1000ULL);

    TEST_SETUP_EXCEPT();
    const uint64_t time_open = ai_arch_priv_read_time(0x2222ULL);
    TEST_ASSERT("HU time is trap-free when mcounteren.TM and scounteren.TM are both set",
        excpt.triggered == false &&
        time_open != 0x2222ULL);

    TEST_SETUP_EXCEPT();
    const uint64_t instret_open = ai_arch_priv_read_instret(0x3333ULL);
    TEST_ASSERT("HU instret is trap-free when mcounteren.IR and scounteren.IR are both set",
        excpt.triggered == false &&
        instret_open >= 0x2000ULL);

    TEST_SETUP_EXCEPT();
    const uint64_t hpm_open = ai_arch_priv_read_hpmcounter3(0x4444ULL);
    TEST_ASSERT("HU hpmcounter3 is trap-free when mcounteren.HPM3 and scounteren.HPM3 are both set",
        excpt.triggered == false &&
        hpm_open >= 0x3000ULL);

    TEST_SETUP_EXCEPT();
    const uint64_t hpm31_open = ai_arch_priv_read_hpmcounter31(0x4445ULL);
    TEST_ASSERT("HU hpmcounter31 is trap-free when mcounteren.HPM31 and scounteren.HPM31 are both set",
        excpt.triggered == false &&
        hpm31_open >= 0x31000ULL);

    goto_priv(PRIV_M);
    CSRC(CSR_MCOUNTEREN, AI_ARCH_PRIV_COUNTER_BITS);
    CSRC(CSR_SCOUNTEREN, AI_ARCH_PRIV_COUNTER_BITS);
    CSRC(CSR_MCOUNTINHIBIT, AI_ARCH_PRIV_COUNTER_BITS);
    const uint64_t mcycle_before = CSRR(CSR_MCYCLE);
    ai_arch_priv_csr_spin(32);
    const uint64_t mcycle_after = CSRR(CSR_MCYCLE);
    TEST_ASSERT("clearing counteren removes lower-privilege access but does not stop the underlying mcycle counter",
        mcycle_after > mcycle_before);

    goto_priv(PRIV_HU);
    TEST_SETUP_EXCEPT();
    const uint64_t cycle_blocked = ai_arch_priv_read_cycle(0x5555ULL);
    TEST_ASSERT("HU cycle traps with illegal instruction when mcounteren.CY is clear",
        excpt.triggered == true &&
        excpt.cause == CAUSE_ILI &&
        cycle_blocked == 0x5555ULL);

    TEST_SETUP_EXCEPT();
    const uint64_t time_blocked = ai_arch_priv_read_time(0x6666ULL);
    TEST_ASSERT("HU time traps with illegal instruction when mcounteren.TM is clear",
        excpt.triggered == true &&
        excpt.cause == CAUSE_ILI &&
        time_blocked == 0x6666ULL);

    TEST_SETUP_EXCEPT();
    const uint64_t instret_blocked = ai_arch_priv_read_instret(0x7777ULL);
    TEST_ASSERT("HU instret traps with illegal instruction when mcounteren.IR is clear",
        excpt.triggered == true &&
        excpt.cause == CAUSE_ILI &&
        instret_blocked == 0x7777ULL);

    TEST_SETUP_EXCEPT();
    const uint64_t hpm_blocked = ai_arch_priv_read_hpmcounter3(0x8888ULL);
    TEST_ASSERT("HU hpmcounter3 traps with illegal instruction when mcounteren.HPM3 is clear",
        excpt.triggered == true &&
        excpt.cause == CAUSE_ILI &&
        hpm_blocked == 0x8888ULL);

    TEST_SETUP_EXCEPT();
    const uint64_t hpm31_blocked = ai_arch_priv_read_hpmcounter31(0x9999ULL);
    TEST_ASSERT("HU hpmcounter31 traps with illegal instruction when mcounteren.HPM31 is clear",
        excpt.triggered == true &&
        excpt.cause == CAUSE_ILI &&
        hpm31_blocked == 0x9999ULL);

    goto_priv(PRIV_M);
    CSRW(CSR_MCOUNTEREN, saved_mcounteren);
    CSRW(CSR_SCOUNTEREN, saved_scounteren);
    CSRW(CSR_MCOUNTINHIBIT, saved_mcountinhibit);

    TEST_END(ai_arch_counter_write_and_shadow_access_matrix_corner);
}

bool ai_arch_mstateen_se0_controls_sstateen_access_corner()
{
    TEST_START();

    goto_priv(PRIV_M);
    const uint64_t saved_mstateen0 = CSRR(CSR_MSTATEEN0);
    const uint64_t saved_sstateen0 = CSRR(CSR_SSTATEEN0);
    const uint64_t hs_sentinel = 0x1234ULL;
    const uint64_t hu_sentinel = 0x5678ULL;

    CSRC(CSR_MSTATEEN0, MSTATEEN_SE0);

    goto_priv(PRIV_HS);
    TEST_SETUP_EXCEPT();
    const uint64_t hs_blocked = ai_arch_priv_read_sstateen0(hs_sentinel);
    TEST_ASSERT("HS access to sstateen0 traps when mstateen0.SE0 is clear",
        excpt.triggered == true &&
        excpt.cause == CAUSE_ILI &&
        hs_blocked == hs_sentinel);

    goto_priv(PRIV_HU);
    TEST_SETUP_EXCEPT();
    const uint64_t hu_blocked = ai_arch_priv_read_sstateen0(hu_sentinel);
    TEST_ASSERT("HU access to sstateen0 traps when mstateen0.SE0 is clear",
        excpt.triggered == true &&
        excpt.cause == CAUSE_ILI &&
        hu_blocked == hu_sentinel);

    goto_priv(PRIV_M);
    TEST_SETUP_EXCEPT();
    (void)ai_arch_priv_read_sstateen0(0ULL);
    TEST_ASSERT("M-mode access to sstateen0 remains legal even when mstateen0.SE0 is clear",
        excpt.triggered == false);

    CSRS(CSR_MSTATEEN0, MSTATEEN_SE0);

    goto_priv(PRIV_HS);
    TEST_SETUP_EXCEPT();
    const uint64_t hs_open = ai_arch_priv_read_sstateen0(0ULL);
    TEST_ASSERT("HS can read sstateen0 when mstateen0.SE0 is set",
        excpt.triggered == false &&
        hs_open == CSRR(CSR_SSTATEEN0));

    goto_priv(PRIV_M);
    CSRS(CSR_MSTATEEN0, MSTATEEN_SE0);
    CSRC(CSR_SSTATEEN0, MSTATEEN_SE0);

    goto_priv(PRIV_HU);
    TEST_SETUP_EXCEPT();
    const uint64_t hu_still_blocked = ai_arch_priv_read_sstateen0(0x9abcULL);
    TEST_ASSERT("HU still cannot access sstateen0 directly after only mstateen0.SE0 is opened",
        excpt.triggered == true &&
        excpt.cause == CAUSE_ILI &&
        hu_still_blocked == 0x9abcULL);

    goto_priv(PRIV_M);
    TEST_SETUP_EXCEPT();
    ai_arch_priv_write_sstateen0(saved_sstateen0);
    TEST_ASSERT("M-mode can restore sstateen0 through CSR write",
        excpt.triggered == false);

    CSRW(CSR_SSTATEEN0, saved_sstateen0);
    CSRW(CSR_MSTATEEN0, saved_mstateen0);

    TEST_END(ai_arch_mstateen_se0_controls_sstateen_access_corner);
}

bool ai_arch_mstatus_vs_field_writable_and_off_illegal_corner()
{
    TEST_START();

    goto_priv(PRIV_M);
    const uint64_t saved_mstatus = CSRR(CSR_MSTATUS);
    const uint64_t saved_vstart = CSRR(CSR_VSTART);

    CSRC(CSR_MSTATUS, AI_ARCH_PRIV_MSTATUS_VS_MASK);
    CSRS(CSR_MSTATUS, AI_ARCH_PRIV_MSTATUS_VS_INITIAL);
    TEST_ASSERT("mstatus.VS is writable to Initial when vector registers are implemented",
        (CSRR(CSR_MSTATUS) & AI_ARCH_PRIV_MSTATUS_VS_MASK) ==
            AI_ARCH_PRIV_MSTATUS_VS_INITIAL);

    CSRC(CSR_MSTATUS, AI_ARCH_PRIV_MSTATUS_VS_MASK);
    CSRS(CSR_MSTATUS, AI_ARCH_PRIV_MSTATUS_VS_CLEAN);
    TEST_ASSERT("mstatus.VS is writable to Clean when vector registers are implemented",
        (CSRR(CSR_MSTATUS) & AI_ARCH_PRIV_MSTATUS_VS_MASK) ==
            AI_ARCH_PRIV_MSTATUS_VS_CLEAN);

    CSRC(CSR_MSTATUS, AI_ARCH_PRIV_MSTATUS_VS_MASK);
    CSRS(CSR_MSTATUS, AI_ARCH_PRIV_MSTATUS_VS_DIRTY);
    TEST_ASSERT("mstatus.VS is writable to Dirty and therefore is not read-only zero",
        (CSRR(CSR_MSTATUS) & AI_ARCH_PRIV_MSTATUS_VS_MASK) ==
            AI_ARCH_PRIV_MSTATUS_VS_DIRTY);

    CSRC(CSR_MSTATUS, AI_ARCH_PRIV_MSTATUS_VS_MASK);

    TEST_SETUP_EXCEPT();
    asm volatile(
        ".option push\n\t"
        ".option norvc\n\t"
        "vsetvli t0, zero, e8, m1, ta, ma\n\t"
        ".option pop\n\t"
        :
        :
        : "t0", "memory");
    TEST_ASSERT("with mstatus.VS=Off, vector configuration raises illegal instruction",
        excpt.triggered == true &&
        excpt.cause == CAUSE_ILI &&
        (CSRR(CSR_MSTATUS) & AI_ARCH_PRIV_MSTATUS_VS_MASK) == 0ULL);

    CSRW(CSR_MSTATUS, saved_mstatus);
    CSRW(CSR_VSTART, saved_vstart);

    TEST_END(ai_arch_mstatus_vs_field_writable_and_off_illegal_corner);
}

bool ai_arch_mstatus_xs_sdt_dte_ro_zero_corner()
{
    TEST_START();

    goto_priv(PRIV_M);
    const uint64_t saved_mstatus = CSRR(CSR_MSTATUS);
    const uint64_t saved_sstatus = CSRR(CSR_SSTATUS);
    const uint64_t saved_vsstatus = CSRR(CSR_VSSTATUS);
    const uint64_t saved_menvcfg = CSRR(CSR_MENVCFG);
    const uint64_t saved_henvcfg = CSRR(CSR_HENVCFG);

    CSRS(CSR_MSTATUS, AI_ARCH_PRIV_MSTATUS_XS_MASK);
    TEST_ASSERT("mstatus.XS remains read-only zero when no user extension state is implemented",
        (CSRR(CSR_MSTATUS) & AI_ARCH_PRIV_MSTATUS_XS_MASK) == 0ULL);

    CSRS(CSR_SSTATUS, SSTATUS_SDT);
    TEST_ASSERT("sstatus.SDT is read-only zero when Ssdbltrp is disabled",
        (CSRR(CSR_SSTATUS) & SSTATUS_SDT) == 0ULL);

    CSRS(CSR_VSSTATUS, VSSTATUS_SDT);
    TEST_ASSERT("vsstatus.SDT is read-only zero when Ssdbltrp is disabled",
        (CSRR(CSR_VSSTATUS) & VSSTATUS_SDT) == 0ULL);

    CSRS(CSR_MENVCFG, MENVCFG_DTE);
    TEST_ASSERT("menvcfg.DTE is read-only zero when Smdbltrp is not enabled",
        (CSRR(CSR_MENVCFG) & MENVCFG_DTE) == 0ULL);

    CSRS(CSR_HENVCFG, MENVCFG_DTE);
    TEST_ASSERT("henvcfg.DTE is read-only zero when Ssdbltrp is not enabled below M-mode",
        (CSRR(CSR_HENVCFG) & MENVCFG_DTE) == 0ULL);

    CSRW(CSR_HENVCFG, saved_henvcfg);
    CSRW(CSR_MENVCFG, saved_menvcfg);
    CSRW(CSR_VSSTATUS, saved_vsstatus);
    CSRW(CSR_SSTATUS, saved_sstatus);
    CSRW(CSR_MSTATUS, saved_mstatus);

    TEST_END(ai_arch_mstatus_xs_sdt_dte_ro_zero_corner);
}

bool ai_arch_menvcfg_lpe_sse_ro_zero_corner()
{
    TEST_START();

    goto_priv(PRIV_M);
    const uint64_t saved_menvcfg = CSRR(CSR_MENVCFG);
    const uint64_t saved_henvcfg = CSRR(CSR_HENVCFG);
    const uint64_t saved_senvcfg = CSRR(CSR_SENVCFG);

    CSRS(CSR_MENVCFG, AI_ARCH_PRIV_MENVCFG_LPE_SSE);
    TEST_ASSERT("menvcfg.LPE remains read-only zero when Zicfilp is disabled in this profile",
        (CSRR(CSR_MENVCFG) & AI_ARCH_PRIV_MENVCFG_LPE) == 0ULL);
    TEST_ASSERT("menvcfg.SSE remains read-only zero when Zicfiss is disabled in this profile",
        (CSRR(CSR_MENVCFG) & AI_ARCH_PRIV_MENVCFG_SSE) == 0ULL);

    CSRS(CSR_HENVCFG, AI_ARCH_PRIV_MENVCFG_LPE_SSE);
    TEST_ASSERT("henvcfg.LPE/SSE stay read-only zero while the corresponding M-level enables are zero",
        (CSRR(CSR_HENVCFG) & AI_ARCH_PRIV_MENVCFG_LPE_SSE) == 0ULL);

    CSRS(CSR_SENVCFG, AI_ARCH_PRIV_MENVCFG_LPE_SSE);
    TEST_ASSERT("senvcfg.LPE/SSE stay read-only zero while the corresponding higher-level enables are zero",
        (CSRR(CSR_SENVCFG) & AI_ARCH_PRIV_MENVCFG_LPE_SSE) == 0ULL);

    CSRW(CSR_SENVCFG, saved_senvcfg);
    CSRW(CSR_HENVCFG, saved_henvcfg);
    CSRW(CSR_MENVCFG, saved_menvcfg);

    TEST_END(ai_arch_menvcfg_lpe_sse_ro_zero_corner);
}

bool ai_arch_svinval_tvm_and_fence_pair_privilege_corner()
{
    TEST_START();

    goto_priv(PRIV_M);
    const uint64_t saved_mstatus = CSRR(CSR_MSTATUS);
    const uint64_t saved_satp = CSRR(CSR_SATP);

    CSRW(CSR_SATP, 0ULL);
    CSRC(CSR_MSTATUS, MSTATUS_TVM);

    goto_priv(PRIV_HS);
    TEST_SETUP_EXCEPT();
    ai_arch_priv_sinval_vma();
    TEST_ASSERT("HS sinval.vma retires normally when mstatus.TVM is clear",
        excpt.triggered == false);

    goto_priv(PRIV_M);
    CSRS(CSR_MSTATUS, MSTATUS_TVM);

    goto_priv(PRIV_HS);
    TEST_SETUP_EXCEPT();
    ai_arch_priv_sinval_vma();
    TEST_ASSERT("HS sinval.vma reports illegal instruction when mstatus.TVM is set",
        excpt.triggered == true &&
        excpt.cause == CAUSE_ILI);

    TEST_SETUP_EXCEPT();
    ai_arch_priv_sfence_w_inval();
    TEST_ASSERT("HS sfence.w.inval is not blocked by mstatus.TVM",
        excpt.triggered == false);

    TEST_SETUP_EXCEPT();
    ai_arch_priv_sfence_inval_ir();
    TEST_ASSERT("HS sfence.inval.ir is not blocked by mstatus.TVM",
        excpt.triggered == false);

    goto_priv(PRIV_M);
    TEST_SETUP_EXCEPT();
    ai_arch_priv_sinval_vma();
    TEST_ASSERT("M-mode sinval.vma remains legal even while mstatus.TVM is set",
        excpt.triggered == false &&
        CSRR(CSR_SATP) == 0ULL);

    CSRW(CSR_SATP, saved_satp);
    CSRW(CSR_MSTATUS, saved_mstatus);

    TEST_END(ai_arch_svinval_tvm_and_fence_pair_privilege_corner);
}

bool ai_arch_xret_preserves_lr_reservation_nhv51_corner()
{
    TEST_START();

    const uintptr_t m_addr = (uintptr_t)&ai_arch_priv_xret_lrsc_words[0];
    const uintptr_t s_addr = (uintptr_t)&ai_arch_priv_xret_lrsc_words[2];
    const uint64_t m_initial = 0x787265746d6c7201ULL;
    const uint64_t m_payload = 0x787265746d736302ULL;
    const uint64_t s_initial = 0x78726574736c7203ULL;
    const uint64_t s_payload = 0x7872657473736304ULL;

    goto_priv(PRIV_M);
    const uint64_t saved_mstatus = CSRR(CSR_MSTATUS);
    const uint64_t saved_sstatus = CSRR(CSR_SSTATUS);
    const uint64_t saved_mepc = CSRR(CSR_MEPC);
    const uint64_t saved_sepc = CSRR(CSR_SEPC);

    ai_arch_priv_xret_lrsc_words[0] = m_initial;
    ai_arch_priv_xret_lrsc_words[1] = 0x1122334455667788ULL;
    ai_arch_priv_xret_lrsc_words[2] = s_initial;
    ai_arch_priv_xret_lrsc_words[3] = 0x8877665544332211ULL;

    CSRC(CSR_MSTATUS, AI_ARCH_PRIV_MSTATUS_MPP_MASK);
    CSRS(CSR_MSTATUS, AI_ARCH_PRIV_MSTATUS_MPP_M);

    uint64_t m_lr_value = 0ULL;
    uint64_t m_sc_status = 1ULL;
    TEST_SETUP_EXCEPT();
    ai_arch_priv_mret_sc_d_sequence(m_addr, m_payload, &m_lr_value,
        &m_sc_status);
    TEST_ASSERT("NHV5.1 MRET does not clear an outstanding doubleword LR reservation before the following SC.D",
        excpt.triggered == false &&
        m_lr_value == m_initial &&
        m_sc_status == 0ULL &&
        ai_arch_priv_xret_lrsc_words[0] == m_payload &&
        ai_arch_priv_xret_lrsc_words[1] == 0x1122334455667788ULL);

    goto_priv(PRIV_HS);
    CSRC(CSR_MSTATUS, MSTATUS_TVM);
    CSRC(CSR_SSTATUS, SSTATUS_SPP_BIT);
    CSRS(CSR_SSTATUS, SSTATUS_SPP_BIT);

    uint64_t s_lr_value = 0ULL;
    uint64_t s_sc_status = 1ULL;
    TEST_SETUP_EXCEPT();
    ai_arch_priv_sret_sc_d_sequence(s_addr, s_payload, &s_lr_value,
        &s_sc_status);
    TEST_ASSERT("NHV5.1 SRET does not clear an outstanding doubleword LR reservation before the following SC.D",
        excpt.triggered == false &&
        s_lr_value == s_initial &&
        s_sc_status == 0ULL &&
        ai_arch_priv_xret_lrsc_words[2] == s_payload &&
        ai_arch_priv_xret_lrsc_words[3] == 0x8877665544332211ULL);

    goto_priv(PRIV_M);
    CSRW(CSR_SEPC, saved_sepc);
    CSRW(CSR_MEPC, saved_mepc);
    CSRW(CSR_SSTATUS, saved_sstatus);
    CSRW(CSR_MSTATUS, saved_mstatus);

    TEST_END(ai_arch_xret_preserves_lr_reservation_nhv51_corner);
}

bool ai_arch_svnapot_bit63_leaf_pte_page_fault_corner()
{
    TEST_START();

    const enum test_page page = VSRWX_GRWX;
    const uintptr_t load_vaddr = hs_page_base(page) + 0x6c0U;
    const uintptr_t store_vaddr = hs_page_base(page) + 0x6c8U;
    const uintptr_t fetch_vaddr = hs_page_base(page) + 0x6d0U;
    const uintptr_t load_paddr = phys_page_base(page) + 0x6c0U;
    const uintptr_t store_paddr = phys_page_base(page) + 0x6c8U;
    const uint64_t load_seed = 0x736e61706f746c64ULL;
    const uint64_t store_seed = 0x736e61706f747374ULL;

    goto_priv(PRIV_M);
    const uint64_t saved_mstatus = CSRR(CSR_MSTATUS);
    const uint64_t saved_sstatus = CSRR(CSR_SSTATUS);
    const uint64_t saved_satp = CSRR(CSR_SATP);
    write64(load_paddr, load_seed);
    write64(store_paddr, store_seed);
    CSRC(CSR_MSTATUS, MSTATUS_TVM);
    CSRC(CSR_SSTATUS, SSTATUS_SUM | SSTATUS_MXR);

    goto_priv(PRIV_HS);
    hspt_init();
    CSRC(CSR_SSTATUS, SSTATUS_SUM | SSTATUS_MXR);
    sfence_vma();

    goto_priv(PRIV_M);
    pte_t *leaf_entry = ai_arch_priv_hs_leaf_entry(page);
    const uint64_t legal_leaf = *leaf_entry;
    *leaf_entry = legal_leaf | AI_ARCH_PRIV_PTE_SVNAPOT;
    const uint64_t svnapot_leaf = *leaf_entry;
    sfence_vma();
    TEST_ASSERT("the HS leaf under test is valid and has only Svnapot bit63 added before the fault probes",
        (legal_leaf & (PTE_V | PTE_RWX)) == (PTE_V | PTE_RWX) &&
        (legal_leaf & AI_ARCH_PRIV_PTE_SVNAPOT) == 0ULL &&
        (svnapot_leaf & AI_ARCH_PRIV_PTE_SVNAPOT) != 0ULL);

    goto_priv(PRIV_HS);
    TEST_SETUP_EXCEPT();
    ld(load_vaddr);
    TEST_ASSERT("when Svnapot is not implemented, HS load through a leaf PTE with bit63 set reports load page fault",
        excpt.triggered == true &&
        excpt.cause == CAUSE_LPF &&
        excpt.tval == load_vaddr);

    TEST_SETUP_EXCEPT();
    sd(store_vaddr, 0xa5a55a5adeadbeefULL);
    TEST_ASSERT("when Svnapot is not implemented, HS store through a leaf PTE with bit63 set reports store page fault",
        excpt.triggered == true &&
        excpt.cause == CAUSE_SPF &&
        excpt.tval == store_vaddr);

    TEST_SETUP_EXCEPT();
    TEST_EXEC_EXCEPT(fetch_vaddr);
    TEST_ASSERT("when Svnapot is not implemented, HS fetch through a leaf PTE with bit63 set reports instruction page fault",
        excpt.triggered == true &&
        excpt.cause == CAUSE_IPF &&
        excpt.tval == fetch_vaddr);

    goto_priv(PRIV_M);
    TEST_ASSERT("Svnapot bit63 fault probes leave the PTE and backing data unchanged",
        *leaf_entry == svnapot_leaf &&
        read64(load_paddr) == load_seed &&
        read64(store_paddr) == store_seed);

    *leaf_entry = legal_leaf;
    sfence_vma();
    CSRW(CSR_SATP, saved_satp);
    CSRW(CSR_SSTATUS, saved_sstatus);
    CSRW(CSR_MSTATUS, saved_mstatus);

    TEST_END(ai_arch_svnapot_bit63_leaf_pte_page_fault_corner);
}
