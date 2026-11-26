#include <rvh_test.h>
#include <stdint.h>

#define AI_COND_MSTATUS_MPP_MASK       (3ULL << 11U)
#define AI_COND_MSTATUS_MPP_S          (1ULL << 11U)
#define AI_COND_MSTATUS_MPP_RESERVED   (2ULL << 11U)
#define AI_COND_ENVCFG_CBIE_MASK       (3ULL << 4U)
#define AI_COND_ENVCFG_CBIE_FLUSH      (1ULL << 4U)
#define AI_COND_ENVCFG_CBIE_RESERVED   (2ULL << 4U)
#define AI_COND_PMA_CFG_TOR_RWX_LOCKED 0x8fULL
#define AI_COND_PMP_LOCK_BYTES         0x8080808080808080ULL
#define AI_COND_PMP_LOCK_RSVD_BYTES    0xe0e0e0e0e0e0e0e0ULL
#define AI_COND_PMP_RW_RESERVED_BYTES  0x0202020202020202ULL

bool ai_arch_mstatus_mpp_reserved_encoding_no_update_corner()
{
    TEST_START();

    goto_priv(PRIV_M);
    const uint64_t saved_mstatus = CSRR(CSR_MSTATUS);
    const uint64_t baseline_write =
        (saved_mstatus & ~AI_COND_MSTATUS_MPP_MASK) |
        AI_COND_MSTATUS_MPP_S;

    CSRW(CSR_MSTATUS, baseline_write);
    const uint64_t baseline = CSRR(CSR_MSTATUS);
    TEST_SETUP_EXCEPT();
    CSRW(CSR_MSTATUS,
        (baseline & ~AI_COND_MSTATUS_MPP_MASK) |
        AI_COND_MSTATUS_MPP_RESERVED);
    const uint64_t after_reserved = CSRR(CSR_MSTATUS);

    TEST_ASSERT("writing reserved mstatus.MPP=2 does not trap and leaves the previous legal MPP value unchanged",
        excpt.triggered == false &&
        (baseline & AI_COND_MSTATUS_MPP_MASK) ==
            AI_COND_MSTATUS_MPP_S &&
        (after_reserved & AI_COND_MSTATUS_MPP_MASK) ==
            AI_COND_MSTATUS_MPP_S,
        "baseline=0x%llx after=0x%llx",
        (unsigned long long)baseline,
        (unsigned long long)after_reserved);

    CSRW(CSR_MSTATUS, saved_mstatus);
    TEST_END();
}

bool ai_linknan_priv_pma_next_locked_tor_blocks_previous_addr_write_corner()
{
    TEST_START();

    const uint64_t first_addr = 0x00100000ULL >> 2U;
    const uint64_t next_addr = 0x00200000ULL >> 2U;
    const uint64_t blocked_write = 0x00180000ULL >> 2U;

    goto_priv(PRIV_M);
    const uint64_t saved_pmacfg0 = CSRR(CSR_PMACFG0);
    const uint64_t unlocked_cfg = saved_pmacfg0 & ~0xffffULL;

    CSRW(CSR_PMACFG0, unlocked_cfg);
    CSRW(CSR_PMAADDR0, first_addr);
    CSRW(CSR_PMAADDR1, next_addr);
    CSRW(CSR_PMACFG0, unlocked_cfg |
        (AI_COND_PMA_CFG_TOR_RWX_LOCKED << 8U));
    const uint64_t locked_cfg = CSRR(CSR_PMACFG0);
    const uint64_t before_blocked_write = CSRR(CSR_PMAADDR0);

    TEST_SETUP_EXCEPT();
    CSRW(CSR_PMAADDR0, blocked_write);
    const uint64_t after_blocked_write = CSRR(CSR_PMAADDR0);

    TEST_ASSERT("PMA entry1 is programmed L=1 and A=TOR before probing pmaaddr0 forward lock propagation",
        (locked_cfg & 0xff00ULL) ==
            (AI_COND_PMA_CFG_TOR_RWX_LOCKED << 8U));
    TEST_ASSERT("a locked TOR PMA entry prevents writes to the previous pmaaddr that supplies its lower bound",
        excpt.triggered == false &&
        before_blocked_write == first_addr &&
        after_blocked_write == before_blocked_write &&
        after_blocked_write != blocked_write,
        "before=0x%llx after=0x%llx attempted=0x%llx",
        (unsigned long long)before_blocked_write,
        (unsigned long long)after_blocked_write,
        (unsigned long long)blocked_write);

    /* PMA lock bits are reset-only; this case must run in an isolated ELF. */
    TEST_END();
}

bool ai_arch_pmpcfg2_locked_entries_ignore_reserved_bits_write_corner()
{
    TEST_START();

    goto_priv(PRIV_M);
    CSRW(CSR_PMPCFG2, 0ULL);
    CSRW(CSR_PMPCFG2, AI_COND_PMP_LOCK_BYTES);
    const uint64_t locked = CSRR(CSR_PMPCFG2);

    TEST_SETUP_EXCEPT();
    CSRW(CSR_PMPCFG2, AI_COND_PMP_LOCK_RSVD_BYTES);
    const uint64_t after_reserved_write = CSRR(CSR_PMPCFG2);

    TEST_ASSERT("all PMP8 through PMP15 entries become locked with A=OFF",
        locked == AI_COND_PMP_LOCK_BYTES);
    TEST_ASSERT("after PMP8 through PMP15 are locked, writing one to each pmpcfg2 byte bits 6 and 5 is ignored",
        excpt.triggered == false &&
        after_reserved_write == AI_COND_PMP_LOCK_BYTES,
        "locked=0x%llx after=0x%llx",
        (unsigned long long)locked,
        (unsigned long long)after_reserved_write);

    /* PMP lock bits are reset-only; this case must run in an isolated ELF. */
    TEST_END();
}

bool ai_arch_pmpcfg_reserved_rw_write_warl_zero_corner()
{
    TEST_START();

    goto_priv(PRIV_M);
    const uint64_t saved_pmpcfg0 = CSRR(CSR_PMPCFG0);
    const uint64_t saved_pmpcfg2 = CSRR(CSR_PMPCFG2);
    const uint64_t pmpcfg0_probe =
        (saved_pmpcfg0 & 0xffULL) |
        (AI_COND_PMP_RW_RESERVED_BYTES & ~0xffULL);

    TEST_SETUP_EXCEPT();
    CSRW(CSR_PMPCFG0, pmpcfg0_probe);
    CSRW(CSR_PMPCFG2, AI_COND_PMP_RW_RESERVED_BYTES);
    const uint64_t pmpcfg0_after = CSRR(CSR_PMPCFG0);
    const uint64_t pmpcfg2_after = CSRR(CSR_PMPCFG2);

    TEST_ASSERT("writing R=0 W=1 to unlocked pmpcfg bytes is accepted as WARL input without trapping",
        excpt.triggered == false);
    TEST_ASSERT("PMP entries normalize the reserved R=0 W=1 combination so neither R nor W remains set",
        (pmpcfg0_after & 0x0303030303030300ULL) == 0ULL &&
        (pmpcfg2_after & 0x0303030303030303ULL) == 0ULL,
        "pmpcfg0=0x%llx pmpcfg2=0x%llx",
        (unsigned long long)pmpcfg0_after,
        (unsigned long long)pmpcfg2_after);

    CSRW(CSR_PMPCFG2, saved_pmpcfg2);
    CSRW(CSR_PMPCFG0, saved_pmpcfg0);
    TEST_END();
}

bool ai_arch_senvcfg_cbie_reserved_encoding_no_update_corner()
{
    TEST_START();

    goto_priv(PRIV_M);
    const uint64_t saved_senvcfg = CSRR(CSR_SENVCFG);
    CSRW(CSR_SENVCFG,
        (saved_senvcfg & ~AI_COND_ENVCFG_CBIE_MASK) |
        AI_COND_ENVCFG_CBIE_FLUSH);
    const uint64_t baseline = CSRR(CSR_SENVCFG);

    TEST_SETUP_EXCEPT();
    CSRW(CSR_SENVCFG,
        (baseline & ~AI_COND_ENVCFG_CBIE_MASK) |
        AI_COND_ENVCFG_CBIE_RESERVED);
    const uint64_t after_reserved = CSRR(CSR_SENVCFG);

    TEST_ASSERT("writing reserved senvcfg.CBIE=2 does not trap",
        excpt.triggered == false);
    TEST_ASSERT("reserved senvcfg.CBIE=2 is WARL no-effect and preserves the previous legal Flush encoding",
        (baseline & AI_COND_ENVCFG_CBIE_MASK) ==
            AI_COND_ENVCFG_CBIE_FLUSH &&
        (after_reserved & AI_COND_ENVCFG_CBIE_MASK) ==
            AI_COND_ENVCFG_CBIE_FLUSH,
        "baseline=0x%llx after=0x%llx",
        (unsigned long long)baseline,
        (unsigned long long)after_reserved);

    CSRW(CSR_SENVCFG, saved_senvcfg);
    TEST_END();
}
