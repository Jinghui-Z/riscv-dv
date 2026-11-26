#ifndef AI_MICRO_MMODE_MEMBLOCK_MAB_BACK_TO_BACK_COMMON_H
#define AI_MICRO_MMODE_MEMBLOCK_MAB_BACK_TO_BACK_COMMON_H

#include <rvh_test.h>
#include <page_tables.h>
#include <stdint.h>

/* Public case prototypes. Definitions live in the partNN sources. */
bool ai_micro_m_crosspage_4b4b_load_fault_repair_back_to_back_load_no_stale_merge_corner();
bool ai_micro_m_crosspage_1b7b_fault_then_4b4b_recovery_no_stale_merge_template_corner();
bool ai_micro_m_crosspage_4b4b_store_fault_repair_back_to_back_store_no_stale_split_corner();
bool ai_micro_m_crosspage_1b7b_store_fault_then_4b4b_recovery_no_stale_split_template_corner();
bool ai_micro_m_crosspage_4b4b_store_fault_then_other_pair_1b7b_recovery_no_stale_split_template_corner();
bool ai_micro_m_crosspage_4b4b_load_fault_then_other_pair_1b7b_recovery_no_stale_merge_template_corner();
bool ai_micro_m_crosspage_1b7b_load_fault_then_other_pair_4b4b_recovery_no_stale_merge_template_corner();
bool ai_micro_m_crosspage_1b7b_store_fault_then_other_pair_4b4b_recovery_no_stale_split_template_corner();
bool ai_micro_m_crosspage_4b4b_fault_then_1b7b_recovery_no_stale_merge_template_corner();
bool ai_micro_m_crosspage_4b4b_store_fault_then_1b7b_recovery_no_stale_split_template_corner();
bool ai_micro_m_crosspage_4b4b_fault_then_1b7b_refault_then_1b7b_recovery_no_stale_merge_template_corner();
bool ai_micro_m_crosspage_4b4b_store_fault_then_1b7b_refault_then_1b7b_recovery_no_stale_split_template_corner();
bool ai_micro_m_crosspage_1b7b_fault_then_4b4b_refault_then_4b4b_recovery_no_stale_merge_template_corner();
bool ai_micro_m_crosspage_1b7b_store_fault_then_4b4b_refault_then_4b4b_recovery_no_stale_split_template_corner();
bool ai_micro_m_crosspage_4b4b_fault_then_other_pair_1b7b_refault_then_1b7b_recovery_no_stale_merge_template_corner();
bool ai_micro_m_crosspage_4b4b_store_fault_then_other_pair_1b7b_refault_then_1b7b_recovery_no_stale_split_template_corner();
bool ai_micro_m_crosspage_1b7b_fault_then_other_pair_4b4b_refault_then_4b4b_recovery_no_stale_merge_template_corner();
bool ai_micro_m_crosspage_1b7b_store_fault_then_other_pair_4b4b_refault_then_4b4b_recovery_no_stale_split_template_corner();
bool ai_micro_m_crosspage_4b4b_fault_then_other_pair_4b4b_refault_then_4b4b_recovery_no_stale_merge_template_corner();
bool ai_micro_m_crosspage_4b4b_store_fault_then_other_pair_4b4b_refault_then_4b4b_recovery_no_stale_split_template_corner();
bool ai_micro_m_crosspage_1b7b_fault_then_other_pair_1b7b_refault_then_1b7b_recovery_no_stale_merge_template_corner();
bool ai_micro_m_crosspage_1b7b_store_fault_then_other_pair_1b7b_refault_then_1b7b_recovery_no_stale_split_template_corner();
bool ai_micro_m_crosspage_4b4b_fault_then_other_pair_4b4b_recovery_no_stale_merge_template_corner();
bool ai_micro_m_crosspage_4b4b_store_fault_then_other_pair_4b4b_recovery_no_stale_split_template_corner();
bool ai_micro_m_crosspage_1b7b_fault_then_other_pair_1b7b_recovery_no_stale_merge_template_corner();
bool ai_micro_m_crosspage_1b7b_store_fault_then_other_pair_1b7b_recovery_no_stale_split_template_corner();
bool ai_micro_m_crosspage_2b6b_fault_then_other_pair_2b6b_recovery_no_stale_merge_template_corner();
bool ai_micro_m_crosspage_2b6b_store_fault_then_other_pair_2b6b_recovery_no_stale_split_template_corner();
bool ai_micro_m_crosspage_3b5b_fault_then_other_pair_3b5b_recovery_no_stale_merge_template_corner();
bool ai_micro_m_crosspage_3b5b_store_fault_then_other_pair_3b5b_recovery_no_stale_split_template_corner();
bool ai_micro_m_crosspage_3b5b_fault_then_other_pair_2b6b_recovery_no_stale_merge_template_corner();
bool ai_micro_m_crosspage_3b5b_store_fault_then_other_pair_2b6b_recovery_no_stale_split_template_corner();
bool ai_micro_m_crosspage_3b5b_fault_then_other_pair_1b7b_recovery_no_stale_merge_template_corner();
bool ai_micro_m_crosspage_3b5b_store_fault_then_other_pair_1b7b_recovery_no_stale_split_template_corner();
bool ai_micro_m_crosspage_2b6b_fault_then_other_pair_1b7b_recovery_no_stale_merge_template_corner();
bool ai_micro_m_crosspage_2b6b_store_fault_then_other_pair_1b7b_recovery_no_stale_split_template_corner();
bool ai_micro_m_crosspage_4b4b_fault_then_other_pair_3b5b_recovery_no_stale_merge_template_corner();
bool ai_micro_m_crosspage_4b4b_store_fault_then_other_pair_3b5b_recovery_no_stale_split_template_corner();
bool ai_micro_m_crosspage_4b4b_fault_then_other_pair_2b6b_recovery_no_stale_merge_template_corner();
bool ai_micro_m_crosspage_4b4b_store_fault_then_other_pair_2b6b_recovery_no_stale_split_template_corner();
bool ai_micro_m_crosspage_4b4b_fault_then_other_pair_3b5b_refault_then_3b5b_recovery_no_stale_merge_template_corner();
bool ai_micro_m_crosspage_4b4b_store_fault_then_other_pair_3b5b_refault_then_3b5b_recovery_no_stale_split_template_corner();
bool ai_micro_m_crosspage_4b4b_fault_then_other_pair_2b6b_refault_then_2b6b_recovery_no_stale_merge_template_corner();
bool ai_micro_m_crosspage_4b4b_store_fault_then_other_pair_2b6b_refault_then_2b6b_recovery_no_stale_split_template_corner();
bool ai_micro_m_crosspage_3b5b_fault_then_other_pair_3b5b_refault_then_3b5b_recovery_no_stale_merge_template_corner();
bool ai_micro_m_crosspage_3b5b_store_fault_then_other_pair_3b5b_refault_then_3b5b_recovery_no_stale_split_template_corner();
bool ai_micro_m_crosspage_2b6b_fault_then_other_pair_2b6b_refault_then_2b6b_recovery_no_stale_merge_template_corner();
bool ai_micro_m_crosspage_2b6b_store_fault_then_other_pair_2b6b_refault_then_2b6b_recovery_no_stale_split_template_corner();
bool ai_micro_m_crosspage_2b6b_fault_then_other_pair_4b4b_refault_then_4b4b_recovery_no_stale_merge_template_corner();
bool ai_micro_m_crosspage_2b6b_store_fault_then_other_pair_4b4b_refault_then_4b4b_recovery_no_stale_split_template_corner();
bool ai_micro_m_crosspage_3b5b_fault_then_other_pair_4b4b_refault_then_4b4b_recovery_no_stale_merge_template_corner();
bool ai_micro_m_crosspage_3b5b_store_fault_then_other_pair_4b4b_refault_then_4b4b_recovery_no_stale_split_template_corner();
bool ai_micro_m_crosspage_2b6b_fault_then_other_pair_3b5b_refault_then_3b5b_recovery_no_stale_merge_template_corner();
bool ai_micro_m_crosspage_2b6b_store_fault_then_other_pair_3b5b_refault_then_3b5b_recovery_no_stale_split_template_corner();
bool ai_micro_m_crosspage_3b5b_fault_then_other_pair_2b6b_refault_then_2b6b_recovery_no_stale_merge_template_corner();
bool ai_micro_m_crosspage_3b5b_store_fault_then_other_pair_2b6b_refault_then_2b6b_recovery_no_stale_split_template_corner();
bool ai_micro_m_crosspage_5b3b_fault_then_other_pair_5b3b_refault_then_5b3b_recovery_no_stale_merge_template_corner();
bool ai_micro_m_crosspage_5b3b_store_fault_then_other_pair_5b3b_refault_then_5b3b_recovery_no_stale_split_template_corner();
bool ai_micro_m_crosspage_6b2b_fault_then_other_pair_6b2b_refault_then_6b2b_recovery_no_stale_merge_template_corner();
bool ai_micro_m_crosspage_6b2b_store_fault_then_other_pair_6b2b_refault_then_6b2b_recovery_no_stale_split_template_corner();
bool ai_micro_m_crosspage_7b1b_fault_then_other_pair_7b1b_refault_then_7b1b_recovery_no_stale_merge_template_corner();
bool ai_micro_m_crosspage_7b1b_store_fault_then_other_pair_7b1b_refault_then_7b1b_recovery_no_stale_split_template_corner();
bool ai_micro_m_crosspage_7b1b_fault_then_other_pair_6b2b_refault_then_6b2b_recovery_no_stale_merge_template_corner();
bool ai_micro_m_crosspage_7b1b_store_fault_then_other_pair_6b2b_refault_then_6b2b_recovery_no_stale_split_template_corner();
bool ai_micro_m_crosspage_6b2b_fault_then_other_pair_5b3b_refault_then_5b3b_recovery_no_stale_merge_template_corner();
bool ai_micro_m_crosspage_6b2b_store_fault_then_other_pair_5b3b_refault_then_5b3b_recovery_no_stale_split_template_corner();
bool ai_micro_m_crosspage_5b3b_fault_then_other_pair_4b4b_refault_then_4b4b_recovery_no_stale_merge_template_corner();
bool ai_micro_m_crosspage_5b3b_store_fault_then_other_pair_4b4b_refault_then_4b4b_recovery_no_stale_split_template_corner();
bool ai_micro_m_crosspage_4b4b_fault_then_other_pair_5b3b_refault_then_5b3b_recovery_no_stale_merge_template_corner();
bool ai_micro_m_crosspage_4b4b_store_fault_then_other_pair_5b3b_refault_then_5b3b_recovery_no_stale_split_template_corner();
bool ai_micro_m_crosspage_5b3b_fault_then_other_pair_6b2b_refault_then_6b2b_recovery_no_stale_merge_template_corner();
bool ai_micro_m_crosspage_5b3b_store_fault_then_other_pair_6b2b_refault_then_6b2b_recovery_no_stale_split_template_corner();
bool ai_micro_m_crosspage_6b2b_fault_then_other_pair_7b1b_refault_then_7b1b_recovery_no_stale_merge_template_corner();
bool ai_micro_m_crosspage_6b2b_store_fault_then_other_pair_7b1b_refault_then_7b1b_recovery_no_stale_split_template_corner();
bool ai_micro_m_crosspage_2b6b_fault_then_other_pair_1b7b_refault_then_1b7b_recovery_no_stale_merge_template_corner();
bool ai_micro_m_crosspage_2b6b_store_fault_then_other_pair_1b7b_refault_then_1b7b_recovery_no_stale_split_template_corner();

#define AI_MICRO_MAB_B2B_PMP_TOR_RWX    0x0fULL
#define AI_MICRO_MAB_B2B_PMP_TOR_X_ONLY 0x0cULL
#define AI_MICRO_MAB_B2B_PMP_TOR_R_ONLY 0x09ULL

static inline pte_t *ai_micro_mab_b2b_hs_leaf_entry(enum test_page page)
{
#ifdef sv48
    return &hspt[4][page];
#else
    return &hspt[2][page];
#endif
}

static inline void ai_micro_mab_b2b_set_leaf(enum test_page page,
    uint64_t flags)
{
    *ai_micro_mab_b2b_hs_leaf_entry(page) =
        (phys_page_base(page) >> 2) | flags;
}

static inline void ai_micro_mab_b2b_prepare_supervisor_translation(void)
{
    goto_priv(PRIV_M);
    hspt_init();
    sfence_vma();
}

static inline void ai_micro_mab_b2b_restore_pmp_full_access(void)
{
    goto_priv(PRIV_M);
    CSRW(CSR_PMPCFG0, 0ULL);
    CSRW(CSR_PMPADDR0, (uint64_t)-1);
    CSRW(CSR_PMPADDR1, 0ULL);
    CSRW(CSR_PMPADDR2, 0ULL);
    CSRW(CSR_PMPCFG0, AI_MICRO_MAB_B2B_PMP_TOR_RWX);
}

static inline void ai_micro_mab_b2b_clear_machine_access_mode(void)
{
    goto_priv(PRIV_M);
    CSRC(CSR_MSTATUS, MSTATUS_MPRV);
    CSRC(CSR_MSTATUS, 3ULL << 11);
    CSRW(CSR_SATP, 0ULL);
}
static inline void ai_micro_mab_b2b_configure_pmp_window(
    uintptr_t deny_start, uintptr_t deny_end, uint64_t deny_cfg)
{
    goto_priv(PRIV_M);
    CSRW(CSR_PMPCFG0, 0ULL);
    CSRW(CSR_PMPADDR0, deny_start >> 2);
    CSRW(CSR_PMPADDR1, deny_end >> 2);
    CSRW(CSR_PMPADDR2, (uint64_t)-1);
    CSRW(CSR_PMPCFG0, AI_MICRO_MAB_B2B_PMP_TOR_RWX |
        (deny_cfg << 8) | (AI_MICRO_MAB_B2B_PMP_TOR_RWX << 16));
}

static inline void ai_micro_mab_b2b_reset_env(void)
{
    ai_micro_mab_b2b_restore_pmp_full_access();
    ai_micro_mab_b2b_clear_machine_access_mode();
}

static inline void ai_micro_mab_b2b_enable_mprv_s(void)
{
    goto_priv(PRIV_M);
    CSRC(CSR_MSTATUS, 3ULL << 11);
    CSRS(CSR_MSTATUS, MSTATUS_MPP);
    CSRS(CSR_MSTATUS, MSTATUS_MPRV);
}

static inline uintptr_t ai_micro_mab_b2b_cross_page_4b4b_vaddr(
    enum test_page low_page)
{
    return hs_page_base(low_page) + PAGE_SIZE - 4U;
}

static inline uintptr_t ai_micro_mab_b2b_cross_page_vaddr(
    enum test_page low_page, unsigned low_bytes)
{
    return hs_page_base(low_page) + PAGE_SIZE - (uintptr_t)low_bytes;
}

static inline uintptr_t ai_micro_mab_b2b_cross_page_low_phys(
    enum test_page low_page)
{
    return phys_page_base(low_page) + PAGE_SIZE - sizeof(uint64_t);
}

static inline uintptr_t ai_micro_mab_b2b_cross_page_high_phys(
    enum test_page high_page)
{
    return phys_page_base(high_page);
}

static inline uint64_t ai_micro_mab_b2b_expected_4b4b_load(
    uint64_t low_dword, uint64_t high_dword)
{
    uint32_t low_half = (uint32_t)(low_dword >> 32);
    uint32_t high_half = (uint32_t)(high_dword & 0xffffffffULL);
    return ((uint64_t)high_half << 32) | (uint64_t)low_half;
}

static inline uint64_t ai_micro_mab_b2b_expected_cross_page_ld(
    uint64_t low_dword, uint64_t high_dword, unsigned low_bytes)
{
    uint64_t result = 0ULL;
    const unsigned low_start = 8U - low_bytes;

    for (unsigned i = 0; i < low_bytes; ++i) {
        result |= ((low_dword >> (8U * (low_start + i))) & 0xffULL) <<
                  (8U * i);
    }

    for (unsigned i = 0; i < 8U - low_bytes; ++i) {
        result |= ((high_dword >> (8U * i)) & 0xffULL) <<
                  (8U * (low_bytes + i));
    }

    return result;
}

static inline uint64_t ai_micro_mab_b2b_expected_low_after_4b_store(
    uint64_t orig_low, uint64_t store_val)
{
    return (orig_low & 0x00000000ffffffffULL) |
           ((store_val & 0x00000000ffffffffULL) << 32);
}

static inline uint64_t ai_micro_mab_b2b_expected_high_after_4b_store(
    uint64_t orig_high, uint64_t store_val)
{
    return (orig_high & 0xffffffff00000000ULL) |
           ((store_val >> 32) & 0x00000000ffffffffULL);
}

static inline uint64_t ai_micro_mab_b2b_expected_low_after_cross_page_store(
    uint64_t orig_low, uint64_t store_val, unsigned low_bytes)
{
    uint64_t expected = orig_low;
    const unsigned low_start = 8U - low_bytes;

    for (unsigned i = 0; i < low_bytes; ++i) {
        const uint64_t lane_mask = 0xffULL << (8U * (low_start + i));
        const uint64_t lane_value =
            ((store_val >> (8U * i)) & 0xffULL) << (8U * (low_start + i));
        expected = (expected & ~lane_mask) | lane_value;
    }

    return expected;
}

static inline uint64_t ai_micro_mab_b2b_expected_high_after_cross_page_store(
    uint64_t orig_high, uint64_t store_val, unsigned low_bytes)
{
    uint64_t expected = orig_high;
    const unsigned high_bytes = 8U - low_bytes;

    for (unsigned i = 0; i < high_bytes; ++i) {
        const uint64_t lane_mask = 0xffULL << (8U * i);
        const uint64_t lane_value =
            ((store_val >> (8U * (low_bytes + i))) & 0xffULL) << (8U * i);
        expected = (expected & ~lane_mask) | lane_value;
    }

    return expected;
}

#endif /* AI_MICRO_MMODE_MEMBLOCK_MAB_BACK_TO_BACK_COMMON_H */
