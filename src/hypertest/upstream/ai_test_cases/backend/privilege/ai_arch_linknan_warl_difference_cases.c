#include <rvh_test.h>
#include <stdint.h>

#define AI_WARL_MNSTATUS_MNPP_MASK        (3ULL << 11U)
#define AI_WARL_MNSTATUS_MNPP_S           (1ULL << 11U)
#define AI_WARL_MNSTATUS_MNPP_RESERVED    (2ULL << 11U)
#define AI_WARL_ENVCFG_CBIE_MASK          (3ULL << 4U)
#define AI_WARL_ENVCFG_CBIE_FLUSH         (1ULL << 4U)
#define AI_WARL_ENVCFG_CBIE_RESERVED      (2ULL << 4U)
#define AI_WARL_PMA_CFG_LOCKED_OFF        0x80ULL
#define AI_WARL_PMA_CFG_REWRITE_ATTEMPT   0x47ULL
#define AI_WARL_TRIGGER_TYPE_SHIFT        60U
#define AI_WARL_TRIGGER_TYPE_MCONTROL6    6ULL
#define AI_WARL_TRIGGER_MATCH_SHIFT       7U
#define AI_WARL_TRIGGER_ACTION_SHIFT      12U
#define AI_WARL_TRIGGER_MATCH_MASK        (0xfULL << AI_WARL_TRIGGER_MATCH_SHIFT)
#define AI_WARL_TRIGGER_ACTION_MASK       (0xfULL << AI_WARL_TRIGGER_ACTION_SHIFT)
#define AI_WARL_TRIGGER_MSU_EXECUTE       0x5cULL
#define AI_WARL_TRIGGER_ILLEGAL_MATCHES   0xfff2ULL
#define AI_WARL_TRIGGER_ILLEGAL_ACTIONS   0xfffcULL
#define AI_WARL_TRIGGER_NUM               4ULL

static inline void ai_warl_clear_trigger_slot(uint64_t slot)
{
    CSRW(CSR_TSELECT, slot);
    CSRW(CSR_TDATA1, 0ULL);
    CSRW(CSR_TDATA2, 0ULL);
}

bool ai_arch_mnstatus_mnpp_reserved_encoding_no_update_corner()
{
    TEST_START();

    goto_priv(PRIV_M);
    const uint64_t saved_mnstatus = CSRR(CSR_MNSTATUS);
    const uint64_t baseline_write =
        (saved_mnstatus & ~AI_WARL_MNSTATUS_MNPP_MASK) |
        AI_WARL_MNSTATUS_MNPP_S;

    TEST_SETUP_EXCEPT();
    CSRW(CSR_MNSTATUS, baseline_write);
    const uint64_t baseline = CSRR(CSR_MNSTATUS);
    CSRW(CSR_MNSTATUS,
        (baseline & ~AI_WARL_MNSTATUS_MNPP_MASK) |
        AI_WARL_MNSTATUS_MNPP_RESERVED);
    const uint64_t after_reserved = CSRR(CSR_MNSTATUS);

    TEST_ASSERT("writing reserved mnstatus.MNPP=2 does not trap and leaves the previous legal MNPP value unchanged",
        excpt.triggered == false &&
        (baseline & AI_WARL_MNSTATUS_MNPP_MASK) ==
            AI_WARL_MNSTATUS_MNPP_S &&
        (after_reserved & AI_WARL_MNSTATUS_MNPP_MASK) ==
            AI_WARL_MNSTATUS_MNPP_S,
        "baseline=0x%llx after=0x%llx",
        (unsigned long long)baseline,
        (unsigned long long)after_reserved);

    CSRW(CSR_MNSTATUS, saved_mnstatus);
    TEST_END();
}

bool ai_arch_menvcfg_cbie_reserved_encoding_no_update_corner()
{
    TEST_START();

    goto_priv(PRIV_M);
    const uint64_t saved_menvcfg = CSRR(CSR_MENVCFG);
    const uint64_t baseline_write =
        (saved_menvcfg & ~AI_WARL_ENVCFG_CBIE_MASK) |
        AI_WARL_ENVCFG_CBIE_FLUSH;

    TEST_SETUP_EXCEPT();
    CSRW(CSR_MENVCFG, baseline_write);
    const uint64_t baseline = CSRR(CSR_MENVCFG);
    CSRW(CSR_MENVCFG,
        (baseline & ~AI_WARL_ENVCFG_CBIE_MASK) |
        AI_WARL_ENVCFG_CBIE_RESERVED);
    const uint64_t after_reserved = CSRR(CSR_MENVCFG);

    TEST_ASSERT("writing reserved menvcfg.CBIE=2 does not trap",
        excpt.triggered == false);
    TEST_ASSERT("reserved menvcfg.CBIE=2 is WARL no-effect and preserves the previous legal Flush encoding",
        (baseline & AI_WARL_ENVCFG_CBIE_MASK) ==
            AI_WARL_ENVCFG_CBIE_FLUSH &&
        (after_reserved & AI_WARL_ENVCFG_CBIE_MASK) ==
            AI_WARL_ENVCFG_CBIE_FLUSH,
        "baseline=0x%llx after=0x%llx",
        (unsigned long long)baseline,
        (unsigned long long)after_reserved);

    CSRW(CSR_MENVCFG, saved_menvcfg);
    TEST_END();
}

bool ai_linknan_priv_pmacfg_locked_entry_ignores_config_and_address_writes_corner()
{
    TEST_START();

    const uint64_t initial_addr = 0x0000000000040000ULL;
    const uint64_t first_blocked_addr = 0x0000000000060000ULL;
    const uint64_t second_blocked_addr = 0x0000000000080000ULL;

    goto_priv(PRIV_M);
    const uint64_t saved_pmacfg0 = CSRR(CSR_PMACFG0);
    const uint64_t unlocked_cfg = saved_pmacfg0 & ~0xffULL;

    CSRW(CSR_PMACFG0, unlocked_cfg);
    CSRW(CSR_PMAADDR0, initial_addr);
    CSRW(CSR_PMACFG0, unlocked_cfg | AI_WARL_PMA_CFG_LOCKED_OFF);
    const uint64_t locked_cfg = CSRR(CSR_PMACFG0);
    const uint64_t locked_addr = CSRR(CSR_PMAADDR0);

    TEST_SETUP_EXCEPT();
    CSRW(CSR_PMAADDR0, first_blocked_addr);
    const uint64_t addr_after_direct_write = CSRR(CSR_PMAADDR0);
    CSRW(CSR_PMACFG0, unlocked_cfg | AI_WARL_PMA_CFG_REWRITE_ATTEMPT);
    const uint64_t cfg_after_rewrite = CSRR(CSR_PMACFG0);
    CSRW(CSR_PMAADDR0, second_blocked_addr);
    const uint64_t addr_after_cfg_rewrite = CSRR(CSR_PMAADDR0);

    TEST_ASSERT("PMA entry0 accepts L=1 with A=OFF before its lock behavior is probed",
        (locked_cfg & 0xffULL) == AI_WARL_PMA_CFG_LOCKED_OFF);
    TEST_ASSERT("a PMA entry lock blocks its own pmaaddr writes both before and after a configuration rewrite attempt",
        excpt.triggered == false &&
        locked_addr == initial_addr &&
        addr_after_direct_write == locked_addr &&
        addr_after_cfg_rewrite == locked_addr,
        "locked=0x%llx first=0x%llx second=0x%llx",
        (unsigned long long)locked_addr,
        (unsigned long long)addr_after_direct_write,
        (unsigned long long)addr_after_cfg_rewrite);
    TEST_ASSERT("a locked PMA configuration byte ignores a later write that tries to clear L and replace the other fields",
        (cfg_after_rewrite & 0xffULL) == AI_WARL_PMA_CFG_LOCKED_OFF,
        "locked_cfg=0x%llx after=0x%llx attempted_byte=0x%llx",
        (unsigned long long)locked_cfg,
        (unsigned long long)cfg_after_rewrite,
        (unsigned long long)AI_WARL_PMA_CFG_REWRITE_ATTEMPT);

    /* PMA lock bits are reset-only; this case must run in an isolated ELF. */
    TEST_END();
}

bool ai_arch_trigger_mcontrol6_illegal_match_action_warl_corner()
{
    TEST_START();

    const uint64_t base =
        (AI_WARL_TRIGGER_TYPE_MCONTROL6 << AI_WARL_TRIGGER_TYPE_SHIFT) |
        AI_WARL_TRIGGER_MSU_EXECUTE;
    uint64_t normalized_match_mask = 0ULL;
    uint64_t normalized_action_mask = 0ULL;

    goto_priv(PRIV_M);
    for (uint64_t slot = 0; slot < AI_WARL_TRIGGER_NUM; ++slot) {
        ai_warl_clear_trigger_slot(slot);
    }
    CSRW(CSR_TSELECT, 0ULL);
    CSRW(CSR_TDATA2, 0ULL);

    TEST_SETUP_EXCEPT();
    for (uint64_t match = 0; match < 16ULL; ++match) {
        if (match == 0ULL || match == 2ULL || match == 3ULL) {
            continue;
        }
        CSRW(CSR_TDATA1, base | (match << AI_WARL_TRIGGER_MATCH_SHIFT));
        const uint64_t readback = CSRR(CSR_TDATA1);
        if (((readback >> AI_WARL_TRIGGER_TYPE_SHIFT) & 0xfULL) ==
                AI_WARL_TRIGGER_TYPE_MCONTROL6 &&
            (readback & AI_WARL_TRIGGER_MATCH_MASK) == 0ULL) {
            normalized_match_mask |= 1ULL << match;
        }
    }

    for (uint64_t action = 2ULL; action < 16ULL; ++action) {
        CSRW(CSR_TDATA1, base | (action << AI_WARL_TRIGGER_ACTION_SHIFT));
        const uint64_t readback = CSRR(CSR_TDATA1);
        if (((readback >> AI_WARL_TRIGGER_TYPE_SHIFT) & 0xfULL) ==
                AI_WARL_TRIGGER_TYPE_MCONTROL6 &&
            (readback & AI_WARL_TRIGGER_ACTION_MASK) == 0ULL) {
            normalized_action_mask |= 1ULL << action;
        }
    }

    TEST_ASSERT("all mcontrol6 MATCH encodings other than EQ, GE and LT are WARL-normalized to EQ without disabling the trigger",
        excpt.triggered == false &&
        normalized_match_mask == AI_WARL_TRIGGER_ILLEGAL_MATCHES,
        "normalized_match_mask=0x%llx expected=0x%llx",
        (unsigned long long)normalized_match_mask,
        (unsigned long long)AI_WARL_TRIGGER_ILLEGAL_MATCHES);
    TEST_ASSERT("all unsupported mcontrol6 ACTION encodings 2 through 15 are WARL-normalized to breakpoint action 0",
        excpt.triggered == false &&
        normalized_action_mask == AI_WARL_TRIGGER_ILLEGAL_ACTIONS,
        "normalized_action_mask=0x%llx expected=0x%llx",
        (unsigned long long)normalized_action_mask,
        (unsigned long long)AI_WARL_TRIGGER_ILLEGAL_ACTIONS);

    for (uint64_t slot = 0; slot < AI_WARL_TRIGGER_NUM; ++slot) {
        ai_warl_clear_trigger_slot(slot);
    }
    TEST_END();
}
