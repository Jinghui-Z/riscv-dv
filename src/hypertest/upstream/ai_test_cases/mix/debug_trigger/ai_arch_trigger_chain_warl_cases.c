#include <rvh_test.h>
#include <stdint.h>

#define AI_CHAIN_TYPE_MCONTROL6   0x6000000000000000ULL
#define AI_CHAIN_EXECUTE_MSU      0x5cULL
#define AI_CHAIN_BIT              0x800ULL
#define AI_CHAIN_TRIGGER_NUM      4ULL

static inline uint64_t ai_chain_mcontrol6_value(bool chain)
{
    return AI_CHAIN_TYPE_MCONTROL6 | AI_CHAIN_EXECUTE_MSU |
        (chain ? AI_CHAIN_BIT : 0ULL);
}

static inline void ai_chain_clear_slot(uint64_t slot)
{
    CSRW(CSR_TSELECT, slot);
    CSRW(CSR_TDATA1, 0ULL);
    CSRW(CSR_TDATA2, 0ULL);
}

static inline uint64_t ai_chain_read_chain(uint64_t slot)
{
    CSRW(CSR_TSELECT, slot);
    return CSRR(CSR_TDATA1) & AI_CHAIN_BIT;
}

static inline void ai_chain_write_chain(uint64_t slot, bool chain)
{
    CSRW(CSR_TSELECT, slot);
    CSRW(CSR_TDATA1, ai_chain_mcontrol6_value(chain));
}

bool ai_arch_trigger_adjacent_chain_warl_corner()
{
    TEST_START();

    goto_priv(PRIV_M);
    TEST_SETUP_EXCEPT();
    uint64_t accepted_first_mask = 0ULL;
    uint64_t rejected_second_mask = 0ULL;

    for (uint64_t first = 0; first + 1ULL < AI_CHAIN_TRIGGER_NUM;
        ++first) {
        for (uint64_t slot = 0; slot < AI_CHAIN_TRIGGER_NUM; ++slot) {
            ai_chain_clear_slot(slot);
        }

        ai_chain_write_chain(first, true);
        if (ai_chain_read_chain(first) == AI_CHAIN_BIT) {
            accepted_first_mask |= 1ULL << first;
        }

        ai_chain_write_chain(first + 1ULL, true);
        if (ai_chain_read_chain(first + 1ULL) == 0ULL &&
            ai_chain_read_chain(first) == AI_CHAIN_BIT) {
            rejected_second_mask |= 1ULL << first;
        }
    }

    TEST_ASSERT("each trigger slot 0 through 2 independently accepts chain=1 as the first member of a two-level chain",
        excpt.triggered == false && accepted_first_mask == 0x7ULL,
        "accepted_mask=0x%llx",
        (unsigned long long)accepted_first_mask);
    TEST_ASSERT("for every adjacent pair 0-1, 1-2 and 2-3, writing chain=1 to the second slot is WARL-cleared because two consecutive chain bits would exceed the two-level limit",
        excpt.triggered == false && rejected_second_mask == 0x7ULL,
        "rejected_mask=0x%llx",
        (unsigned long long)rejected_second_mask);

    for (uint64_t slot = 0; slot < AI_CHAIN_TRIGGER_NUM; ++slot) {
        ai_chain_clear_slot(slot);
    }
    TEST_END();
}
