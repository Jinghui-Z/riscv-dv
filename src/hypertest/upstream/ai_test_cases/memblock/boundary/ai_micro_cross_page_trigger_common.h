#ifndef AI_MICRO_CROSS_PAGE_TRIGGER_COMMON_H
#define AI_MICRO_CROSS_PAGE_TRIGGER_COMMON_H

#include <rvh_test.h>
#include <page_tables.h>
#include <stdint.h>

/* Public case prototypes. Definitions live in the partNN sources. */
bool ai_micro_hs_vector_store_trigger_earlier_than_high_half_access_fault_repeated_breakpoint_corner();
bool ai_micro_hs_vector_store_low_half_access_fault_suppresses_later_high_half_trigger_corner();
bool ai_micro_hs_vector_store_same_elem_misalign_and_trigger_tie_breakpoint_corner();
bool ai_micro_hs_vector_store_earlier_misalign_suppresses_later_elem_trigger_corner();
bool ai_micro_hs_load_cross_page_second_half_address_trigger_ignored_spike_corner();
bool ai_micro_hs_fetch_after_scalar_cross_page_store_fence_i_execute_trigger_repeated_breakpoint_then_completion_adjacent_preserved_corner();
bool ai_micro_hs_fetch_after_scalar_cross_page_store_fence_i_execute_trigger_repeated_breakpoint_then_no_x_page_fault_adjacent_preserved_corner();
bool ai_micro_hs_fetch_after_scalar_cross_page_store_fence_i_execute_trigger_repeated_breakpoint_then_grant_x_executes_adjacent_preserved_corner();
bool ai_micro_hs_fetch_after_scalar_cross_page_store_fence_i_execute_trigger_repeated_breakpoint_then_noncanonical_fetch_page_fault_adjacent_preserved_corner();
bool ai_micro_hs_fetch_after_scalar_cross_page_store_fence_i_execute_trigger_repeated_breakpoint_then_all_ones_fetch_page_fault_adjacent_preserved_corner();
bool ai_micro_hs_fetch_after_scalar_cross_page_store_fence_i_execute_trigger_repeated_breakpoint_then_leaf_overflow_fetch_access_fault_adjacent_preserved_corner();
bool ai_micro_hs_fetch_after_scalar_cross_page_store_fence_i_execute_trigger_repeated_breakpoint_then_leaf_overflow_fix_leaf_executes_adjacent_preserved_corner();
bool ai_micro_hs_fetch_after_vector_cross_page_store_fence_i_execute_trigger_repeated_breakpoint_then_completion_adjacent_preserved_corner();
bool ai_micro_hs_fetch_after_vector_cross_page_store_fence_i_execute_trigger_repeated_breakpoint_then_no_x_page_fault_adjacent_preserved_corner();
bool ai_micro_hs_fetch_after_vector_cross_page_store_fence_i_execute_trigger_repeated_breakpoint_then_grant_x_executes_adjacent_preserved_corner();
bool ai_micro_hs_fetch_after_vector_cross_page_store_fence_i_execute_trigger_repeated_breakpoint_then_noncanonical_fetch_page_fault_adjacent_preserved_corner();
bool ai_micro_hs_fetch_after_vector_cross_page_store_fence_i_execute_trigger_repeated_breakpoint_then_all_ones_fetch_page_fault_adjacent_preserved_corner();
bool ai_micro_hs_fetch_after_vector_cross_page_store_fence_i_execute_trigger_repeated_breakpoint_then_leaf_overflow_fetch_access_fault_adjacent_preserved_corner();
bool ai_micro_hs_fetch_after_vector_cross_page_store_fence_i_execute_trigger_repeated_breakpoint_then_leaf_overflow_fix_leaf_executes_adjacent_preserved_corner();
bool ai_micro_hs_fetch_after_scalar_cross_page_store_after_vector_cross_page_store_fence_i_execute_trigger_repeated_breakpoint_then_completion_adjacent_preserved_corner();
bool ai_micro_hs_fetch_after_scalar_cross_page_store_after_vector_cross_page_store_fence_i_execute_trigger_repeated_breakpoint_then_no_x_page_fault_adjacent_preserved_corner();
bool ai_micro_hs_fetch_after_scalar_cross_page_store_after_vector_cross_page_store_fence_i_execute_trigger_repeated_breakpoint_then_grant_x_executes_adjacent_preserved_corner();
bool ai_micro_hs_fetch_after_scalar_cross_page_store_after_vector_cross_page_store_fence_i_execute_trigger_repeated_breakpoint_then_leaf_overflow_fetch_access_fault_adjacent_preserved_corner();
bool ai_micro_hs_fetch_after_scalar_cross_page_store_after_vector_cross_page_store_fence_i_execute_trigger_repeated_breakpoint_then_leaf_overflow_fix_leaf_executes_adjacent_preserved_corner();
bool ai_micro_hs_fetch_after_scalar_cross_page_store_after_vector_cross_page_store_fence_i_execute_trigger_repeated_breakpoint_then_noncanonical_fetch_page_fault_adjacent_preserved_corner();
bool ai_micro_hs_fetch_after_scalar_cross_page_store_after_vector_cross_page_store_fence_i_execute_trigger_repeated_breakpoint_then_all_ones_fetch_page_fault_adjacent_preserved_corner();
bool ai_micro_hs_load_cross_page_dual_address_trigger_only_original_matches_spike_corner();
bool ai_micro_hs_load_cross_page_dual_address_trigger_repeated_breakpoint_then_completion_spike_corner();
bool ai_micro_hs_store_cross_page_second_half_address_trigger_ignored_spike_corner();
bool ai_micro_hs_store_cross_page_dual_address_trigger_only_original_matches_spike_corner();
bool ai_micro_hs_store_cross_page_dual_address_trigger_repeated_breakpoint_then_completion_spike_corner();
bool ai_micro_hs_load_cross_page_execute_and_address_trigger_both_breakpoint_corner();
bool ai_micro_hs_load_cross_page_execute_and_address_trigger_repeated_breakpoint_then_completion_corner();
bool ai_micro_hs_load_cross_page_execute_and_address_trigger_repeated_breakpoint_then_completion_adjacent_preserved_corner();
bool ai_micro_hs_scalar_cross_page_load_after_vector_cross_page_execute_and_address_trigger_repeated_breakpoint_then_completion_corner();
bool ai_micro_hs_scalar_cross_page_load_after_vector_cross_page_execute_and_address_trigger_repeated_breakpoint_then_completion_adjacent_preserved_corner();
bool ai_micro_hs_scalar_cross_page_load_after_vector_cross_page_store_execute_and_address_trigger_repeated_breakpoint_then_completion_corner();
bool ai_micro_hs_scalar_cross_page_load_after_vector_cross_page_store_execute_and_address_trigger_repeated_breakpoint_then_completion_adjacent_preserved_corner();
bool ai_micro_hs_scalar_cross_page_load_after_vector_cross_page_store_execute_and_address_trigger_page_fault_repeated_breakpoint_then_valid_load_recovers_corner();
bool ai_micro_hs_scalar_cross_page_load_after_vector_cross_page_store_execute_and_address_trigger_page_fault_repeated_breakpoint_then_valid_load_adjacent_preserved_corner();
bool ai_micro_hs_scalar_cross_page_load_after_vector_cross_page_store_execute_and_address_trigger_access_fault_repeated_breakpoint_then_valid_load_recovers_corner();
bool ai_micro_hs_scalar_cross_page_load_after_vector_cross_page_store_execute_and_address_trigger_access_fault_repeated_breakpoint_then_valid_load_adjacent_preserved_corner();
bool ai_micro_hs_store_cross_page_execute_and_address_trigger_both_breakpoint_corner();
bool ai_micro_hs_store_cross_page_execute_and_address_trigger_repeated_breakpoint_then_completion_corner();
bool ai_micro_hs_store_cross_page_execute_and_address_trigger_repeated_breakpoint_then_completion_adjacent_preserved_corner();
bool ai_micro_hs_scalar_cross_page_store_after_vector_cross_page_execute_and_address_trigger_repeated_breakpoint_then_completion_corner();
bool ai_micro_hs_scalar_cross_page_store_after_vector_cross_page_execute_and_address_trigger_repeated_breakpoint_then_completion_adjacent_preserved_corner();
bool ai_micro_hs_scalar_cross_page_store_after_vector_cross_page_load_execute_and_address_trigger_repeated_breakpoint_then_completion_corner();
bool ai_micro_hs_scalar_cross_page_store_after_vector_cross_page_load_execute_and_address_trigger_repeated_breakpoint_then_completion_adjacent_preserved_corner();
bool ai_micro_hs_scalar_cross_page_store_after_vector_cross_page_load_execute_and_address_trigger_page_fault_repeated_breakpoint_then_valid_store_recovers_corner();
bool ai_micro_hs_scalar_cross_page_store_after_vector_cross_page_load_execute_and_address_trigger_page_fault_repeated_breakpoint_then_valid_store_adjacent_preserved_corner();
bool ai_micro_hs_scalar_cross_page_store_after_vector_cross_page_load_execute_and_address_trigger_access_fault_repeated_breakpoint_then_valid_store_recovers_corner();
bool ai_micro_hs_scalar_cross_page_store_after_vector_cross_page_load_execute_and_address_trigger_access_fault_repeated_breakpoint_then_valid_store_adjacent_preserved_corner();
bool ai_micro_hs_load_cross_page_execute_trigger_with_second_half_address_slot_ignored_spike_corner();
bool ai_micro_hs_load_cross_page_execute_trigger_with_second_half_address_slot_repeated_breakpoint_then_completion_spike_corner();
bool ai_micro_hs_load_cross_page_execute_trigger_with_second_half_address_slot_repeated_breakpoint_then_completion_adjacent_preserved_spike_corner();
bool ai_micro_hs_store_cross_page_execute_trigger_with_second_half_address_slot_ignored_spike_corner();
bool ai_micro_hs_store_cross_page_execute_trigger_with_second_half_address_slot_repeated_breakpoint_then_completion_spike_corner();
bool ai_micro_hs_store_cross_page_execute_trigger_with_second_half_address_slot_repeated_breakpoint_then_completion_adjacent_preserved_spike_corner();
bool ai_micro_hs_load_cross_page_high_half_execute_trigger_with_second_half_address_slot_page_fault_spike_corner();
bool ai_micro_hs_load_cross_page_high_half_execute_trigger_with_second_half_address_slot_page_fault_repeated_breakpoint_then_fault_spike_corner();
bool ai_micro_hs_load_cross_page_high_half_execute_trigger_with_second_half_address_slot_page_fault_repeated_breakpoint_then_fault_adjacent_preserved_spike_corner();
bool ai_micro_hs_load_cross_page_high_half_execute_trigger_with_second_half_address_slot_page_fault_repeated_breakpoint_then_fix_high_half_recovers_spike_corner();
bool ai_micro_hs_load_cross_page_high_half_execute_trigger_with_second_half_address_slot_access_fault_spike_corner();
bool ai_micro_hs_load_cross_page_high_half_execute_trigger_with_second_half_address_slot_access_fault_repeated_breakpoint_then_fault_spike_corner();
bool ai_micro_hs_load_cross_page_high_half_execute_trigger_with_second_half_address_slot_access_fault_repeated_breakpoint_then_fault_adjacent_preserved_spike_corner();
bool ai_micro_hs_load_cross_page_high_half_execute_trigger_with_second_half_address_slot_access_fault_repeated_breakpoint_then_restore_pmp_recovers_spike_corner();
bool ai_micro_hs_store_cross_page_high_half_execute_trigger_with_second_half_address_slot_page_fault_no_side_effect_spike_corner();
bool ai_micro_hs_store_cross_page_high_half_execute_trigger_with_second_half_address_slot_page_fault_repeated_breakpoint_then_fault_no_side_effect_spike_corner();
bool ai_micro_hs_store_cross_page_high_half_exec_2nd_addr_slot_page_fault_repeated_bkp_no_side_effect_adjacent_spike_corner();
bool ai_micro_hs_store_cross_page_high_half_execute_trigger_with_second_half_address_slot_page_fault_repeated_breakpoint_then_fix_high_half_recovers_spike_corner();
bool ai_micro_hs_store_cross_page_high_half_execute_trigger_with_second_half_address_slot_access_fault_no_side_effect_spike_corner();
bool ai_micro_hs_store_cross_page_high_half_execute_trigger_with_second_half_address_slot_access_fault_repeated_breakpoint_then_fault_no_side_effect_spike_corner();
bool ai_micro_hs_store_cross_page_high_half_exec_2nd_addr_slot_access_fault_repeated_bkp_no_side_effect_adjacent_spike_corner();
bool ai_micro_hs_store_cross_page_high_half_execute_trigger_with_second_half_address_slot_access_fault_repeated_breakpoint_then_restore_pmp_recovers_spike_corner();
bool ai_micro_hs_load_cross_page_low_half_execute_and_address_trigger_page_fault_priority_corner();
bool ai_micro_hs_load_cross_page_low_half_execute_trigger_with_second_half_address_slot_page_fault_spike_corner();
bool ai_micro_hs_load_cross_page_low_half_execute_trigger_with_second_half_address_slot_page_fault_repeated_breakpoint_then_fault_spike_corner();
bool ai_micro_hs_load_cross_page_low_half_execute_and_address_trigger_access_fault_priority_corner();
bool ai_micro_hs_load_cross_page_low_half_execute_trigger_with_second_half_address_slot_access_fault_spike_corner();
bool ai_micro_hs_load_cross_page_low_half_execute_trigger_with_second_half_address_slot_access_fault_repeated_breakpoint_then_fault_spike_corner();
bool ai_micro_hs_load_cross_page_high_half_execute_and_address_trigger_page_fault_priority_corner();
bool ai_micro_hs_load_cross_page_high_half_execute_and_address_trigger_access_fault_priority_corner();
bool ai_micro_hs_store_cross_page_low_half_execute_and_address_trigger_page_fault_priority_corner();
bool ai_micro_hs_store_cross_page_low_half_execute_trigger_with_second_half_address_slot_page_fault_spike_corner();
bool ai_micro_hs_store_cross_page_low_half_execute_trigger_with_second_half_address_slot_page_fault_repeated_breakpoint_then_fault_no_side_effect_spike_corner();
bool ai_micro_hs_store_cross_page_low_half_execute_and_address_trigger_access_fault_priority_corner();
bool ai_micro_hs_store_cross_page_low_half_execute_trigger_with_second_half_address_slot_access_fault_spike_corner();
bool ai_micro_hs_store_cross_page_low_half_execute_trigger_with_second_half_address_slot_access_fault_repeated_breakpoint_then_fault_no_side_effect_spike_corner();
bool ai_micro_hs_store_cross_page_high_half_execute_and_address_trigger_page_fault_no_side_effect_corner();
bool ai_micro_hs_store_cross_page_high_half_execute_and_address_trigger_access_fault_no_side_effect_corner();
bool ai_micro_hs_load_cross_page_low_half_execute_and_address_trigger_page_fault_repeated_breakpoint_then_fault_corner();
bool ai_micro_hs_load_cross_page_low_half_execute_and_address_trigger_page_fault_repeated_breakpoint_then_fault_adjacent_preserved_corner();
bool ai_micro_hs_load_cross_page_low_half_execute_and_address_trigger_access_fault_repeated_breakpoint_then_fault_corner();
bool ai_micro_hs_load_cross_page_low_half_execute_and_address_trigger_access_fault_repeated_breakpoint_then_fault_adjacent_preserved_corner();
bool ai_micro_hs_load_cross_page_high_half_execute_and_address_trigger_page_fault_repeated_breakpoint_then_fault_corner();
bool ai_micro_hs_load_cross_page_high_half_execute_and_address_trigger_page_fault_repeated_breakpoint_then_fault_adjacent_preserved_corner();
bool ai_micro_hs_load_cross_page_high_half_execute_and_address_trigger_page_fault_repeated_breakpoint_then_fix_high_half_recovers_corner();
bool ai_micro_hs_load_cross_page_high_half_execute_and_address_trigger_page_fault_repeated_breakpoint_then_fix_high_half_adjacent_preserved_corner();
bool ai_micro_hs_load_cross_page_high_half_execute_and_address_trigger_access_fault_repeated_breakpoint_then_fault_corner();
bool ai_micro_hs_load_cross_page_high_half_execute_and_address_trigger_access_fault_repeated_breakpoint_then_fault_adjacent_preserved_corner();
bool ai_micro_hs_load_cross_page_high_half_execute_and_address_trigger_access_fault_repeated_breakpoint_then_restore_pmp_recovers_corner();
bool ai_micro_hs_load_cross_page_high_half_execute_and_address_trigger_access_fault_repeated_breakpoint_then_restore_pmp_adjacent_preserved_corner();
bool ai_micro_hs_store_cross_page_low_half_execute_and_address_trigger_page_fault_repeated_breakpoint_then_fault_no_side_effect_corner();
bool ai_micro_hs_store_cross_page_low_half_execute_and_address_trigger_page_fault_repeated_breakpoint_then_fault_no_side_effect_adjacent_preserved_corner();
bool ai_micro_hs_store_cross_page_low_half_execute_and_address_trigger_access_fault_repeated_breakpoint_then_fault_no_side_effect_corner();
bool ai_micro_hs_store_cross_page_low_half_execute_and_address_trigger_access_fault_repeated_breakpoint_then_fault_no_side_effect_adjacent_preserved_corner();
bool ai_micro_hs_store_cross_page_high_half_execute_and_address_trigger_page_fault_repeated_breakpoint_then_fault_no_side_effect_corner();
bool ai_micro_hs_store_cross_page_high_half_execute_and_address_trigger_page_fault_repeated_breakpoint_then_fault_no_side_effect_adjacent_preserved_corner();
bool ai_micro_hs_store_cross_page_high_half_execute_and_address_trigger_page_fault_repeated_breakpoint_then_fix_high_half_recovers_corner();
bool ai_micro_hs_store_cross_page_high_half_execute_and_address_trigger_page_fault_repeated_breakpoint_then_fix_high_half_adjacent_preserved_corner();
bool ai_micro_hs_store_cross_page_high_half_execute_and_address_trigger_access_fault_repeated_breakpoint_then_fault_no_side_effect_corner();
bool ai_micro_hs_store_cross_page_high_half_execute_and_address_trigger_access_fault_repeated_breakpoint_then_fault_no_side_effect_adjacent_preserved_corner();
bool ai_micro_hs_store_cross_page_high_half_execute_and_address_trigger_access_fault_repeated_breakpoint_then_restore_pmp_recovers_corner();
bool ai_micro_hs_store_cross_page_high_half_execute_and_address_trigger_access_fault_repeated_breakpoint_then_restore_pmp_adjacent_preserved_corner();
bool ai_micro_hs_load_cross_page_low_half_dual_address_trigger_page_fault_priority_spike_corner();
bool ai_micro_hs_load_cross_page_low_half_dual_address_trigger_access_fault_priority_spike_corner();
bool ai_micro_hs_load_cross_page_high_half_dual_address_trigger_page_fault_priority_spike_corner();
bool ai_micro_hs_load_cross_page_high_half_dual_address_trigger_access_fault_priority_spike_corner();
bool ai_micro_hs_load_cross_page_high_half_dual_address_trigger_page_fault_repeated_breakpoint_then_fix_high_half_recovers_spike_corner();
bool ai_micro_hs_load_cross_page_high_half_dual_address_trigger_access_fault_repeated_breakpoint_then_restore_pmp_recovers_spike_corner();
bool ai_micro_hs_store_cross_page_low_half_dual_address_trigger_page_fault_no_side_effect_spike_corner();
bool ai_micro_hs_store_cross_page_low_half_dual_address_trigger_access_fault_no_side_effect_spike_corner();
bool ai_micro_hs_store_cross_page_high_half_dual_address_trigger_page_fault_no_side_effect_spike_corner();
bool ai_micro_hs_store_cross_page_high_half_dual_address_trigger_access_fault_no_side_effect_spike_corner();
bool ai_micro_hs_store_cross_page_high_half_dual_address_trigger_page_fault_repeated_breakpoint_then_fix_high_half_recovers_spike_corner();
bool ai_micro_hs_store_cross_page_high_half_dual_address_trigger_access_fault_repeated_breakpoint_then_restore_pmp_recovers_spike_corner();
bool ai_micro_hs_load_cross_page_high_half_dual_address_trigger_page_fault_repeated_breakpoint_then_fix_high_half_adjacent_preserved_spike_corner();
bool ai_micro_hs_load_cross_page_high_half_dual_address_trigger_access_fault_repeated_breakpoint_then_restore_pmp_adjacent_preserved_spike_corner();
bool ai_micro_hs_store_cross_page_high_half_dual_address_trigger_page_fault_repeated_breakpoint_then_fix_high_half_adjacent_preserved_spike_corner();
bool ai_micro_hs_store_cross_page_high_half_dual_address_trigger_access_fault_repeated_breakpoint_then_restore_pmp_adjacent_preserved_spike_corner();

#define AI_MICRO_CROSS_PAGE_MCONTROL6_MSU_BASE 0x6000000000000058ULL
#define AI_MICRO_CROSS_PAGE_MCONTROL6_LOAD 0x1ULL
#define AI_MICRO_CROSS_PAGE_MCONTROL6_STORE 0x2ULL
#define AI_MICRO_CROSS_PAGE_MCONTROL6_EXECUTE 0x4ULL

#define AI_MICRO_PMP_TOR_RWX 0x0fULL
#define AI_MICRO_PMP_TOR_NO_R 0x0eULL
#define AI_MICRO_PMP_TOR_NO_W 0x0dULL

static inline uintptr_t ai_micro_cross_page_trigger_addr(enum test_page low_page, uintptr_t low_bytes)
{
    return hs_page_base(low_page) + PAGE_SIZE - low_bytes;
}

static inline uintptr_t ai_micro_cross_page_trigger_page_end_phys(enum test_page page)
{
    return phys_page_base(page) + PAGE_SIZE - sizeof(uint64_t);
}

static inline void ai_micro_cross_page_trigger_write_bytes(uintptr_t addr,
    const uint8_t *bytes, unsigned len)
{
    for (unsigned i = 0; i < len; ++i) {
        write8(addr + i, bytes[i]);
    }
}

static inline bool ai_micro_cross_page_trigger_bytes_match(uintptr_t addr,
    const uint8_t *bytes, unsigned len)
{
    for (unsigned i = 0; i < len; ++i) {
        if (read8(addr + i) != bytes[i]) {
            return false;
        }
    }
    return true;
}

static inline pte_t *ai_micro_cross_page_hs_leaf_entry(enum test_page page)
{
#ifdef sv48
    return &hspt[4][page];
#else
    return &hspt[2][page];
#endif
}

static inline void ai_micro_cross_page_set_hs_leaf(enum test_page page,
    uintptr_t paddr,
    uint64_t flags)
{
    *ai_micro_cross_page_hs_leaf_entry(page) = (paddr >> 2) | flags;
}

static inline uintptr_t ai_micro_cross_page_trigger_make_noncanonical_vaddr(uintptr_t canonical_vaddr)
{
#ifdef sv48
    uintptr_t bad_vaddr = canonical_vaddr;
    uintptr_t bit47 = (canonical_vaddr >> 47) & 1ULL;

    if (bit47) {
        bad_vaddr &= ~(1ULL << 48);
    } else {
        bad_vaddr |= (1ULL << 48);
    }
#else
    uintptr_t bad_vaddr = canonical_vaddr;
    uintptr_t bit38 = (canonical_vaddr >> 38) & 1ULL;

    if (bit38) {
        bad_vaddr &= ~(1ULL << 39);
    } else {
        bad_vaddr |= (1ULL << 39);
    }
#endif

    return bad_vaddr;
}

static inline void ai_micro_cross_page_clear_mcontrol6_slot(size_t slot)
{
    goto_priv(PRIV_M);
    CSRW(CSR_TSELECT, slot);
    asm volatile("csrw tdata1, x0");
    CSRW(CSR_TDATA2, 0);
}

static inline uint64_t ai_micro_cross_page_mcontrol6_tdata1(uint64_t access_bits)
{
    return AI_MICRO_CROSS_PAGE_MCONTROL6_MSU_BASE | access_bits;
}

static inline void ai_micro_cross_page_arm_mcontrol6_slot(size_t slot, uintptr_t addr,
    uint64_t access_bits)
{
    goto_priv(PRIV_M);
    CSRW(CSR_TSELECT, slot);
    asm volatile("csrw tdata1, x0");
    CSRW(CSR_TDATA2, addr);
    CSRW(CSR_TDATA1, ai_micro_cross_page_mcontrol6_tdata1(access_bits));
}

static inline uint64_t ai_micro_cross_page_enable_machine_breakpoint_delivery(void)
{
    goto_priv(PRIV_M);
    uint64_t saved_mstatus = CSRR(CSR_MSTATUS);
    CSRS(CSR_MSTATUS, MSTATUS_MIE);
    return saved_mstatus;
}

static inline void ai_micro_cross_page_restore_machine_status(uint64_t saved_mstatus)
{
    goto_priv(PRIV_M);
    CSRW(CSR_MSTATUS, saved_mstatus);
}

enum ai_micro_cross_page_repeated_followup_mode {
    AI_MICRO_CROSS_PAGE_FOLLOWUP_NONCANONICAL = 0,
    AI_MICRO_CROSS_PAGE_FOLLOWUP_ALL_ONES,
    AI_MICRO_CROSS_PAGE_FOLLOWUP_LEAF_OVERFLOW_FAULT,
    AI_MICRO_CROSS_PAGE_FOLLOWUP_LEAF_OVERFLOW_FIX,
};

static inline void ai_micro_cross_page_configure_pmp_window(uintptr_t deny_start,
    uintptr_t deny_end, uint64_t deny_cfg)
{
    goto_priv(PRIV_M);
    CSRW(CSR_PMPCFG0, 0ULL);
    CSRW(CSR_PMPADDR0, deny_start >> 2);
    CSRW(CSR_PMPADDR1, deny_end >> 2);
    CSRW(CSR_PMPADDR2, (uint64_t)-1);
    CSRW(CSR_PMPCFG0,
        AI_MICRO_PMP_TOR_RWX | (deny_cfg << 8) | (AI_MICRO_PMP_TOR_RWX << 16));
}

static inline void ai_micro_cross_page_clear_pmp_window(void)
{
    goto_priv(PRIV_M);
    CSRW(CSR_PMPCFG0, 0ULL);
    CSRW(CSR_PMPADDR0, (uint64_t)-1);
    CSRW(CSR_PMPCFG0, AI_MICRO_PMP_TOR_RWX);
}

static inline uintptr_t ai_micro_cross_page_trigger_pbmt_nc_backing_paddr(void)
{
    return phys_page_base(X);
}

static inline uintptr_t ai_micro_cross_page_trigger_prepare_pbmt_nc_alias(
    enum test_page page)
{
    goto_priv(PRIV_M);
    CSRS(CSR_MENVCFG, MENVCFG_PBMTE);
    hspt_init();
    pbmt_hspt_to_x(page);
    sfence_vma();
    return hs_page_base(page);
}

static inline void ai_micro_cross_page_trigger_reset_pbmt_nc_env(void)
{
    goto_priv(PRIV_M);
    CSRC(CSR_MENVCFG, MENVCFG_PBMTE);
    hspt_init();
    sfence_vma();
}

static inline void ai_micro_cross_page_fence_i(void)
{
    asm volatile(
        ".option push\n\t"
        ".option norvc\n\t"
        ".4byte 0x0000100f\n\t"
        ".option pop\n\t"
        :
        :
        : "memory");
}

static bool ai_micro_cross_page_vector_store_trigger_vs_access_fault_priority(
    bool trigger_earlier)
{
    const enum test_page low_page = VSRW_GRW;
    const enum test_page high_page = VSRW_GRX;
    const enum test_page src_page = VSRWX_GRWX;
    const uintptr_t boundary_vaddr = ai_micro_cross_page_trigger_addr(low_page, 4U);
    const uintptr_t trigger_addr = trigger_earlier ? boundary_vaddr : (boundary_vaddr + 4U);
    const uintptr_t low_phys_start = phys_page_base(low_page) + PAGE_SIZE - 4U;
    const uintptr_t high_phys_start = phys_page_base(high_page);
    const uintptr_t second_half_vaddr = boundary_vaddr + 4U;
    const uintptr_t src_vaddr = hs_page_base(src_page) + (trigger_earlier ? 0x360U : 0x370U);
    const uintptr_t src_phys = phys_page_base(src_page) + (trigger_earlier ? 0x360U : 0x370U);
    const uintptr_t deny_start = trigger_earlier ? high_phys_start : phys_page_base(low_page);
    const uintptr_t deny_end = deny_start + PAGE_SIZE;
    const uintptr_t sentinel_before_addr = low_phys_start - 1U;
    const uintptr_t sentinel_after_addr = high_phys_start + 4U;
    const uint8_t initial_boundary[8] = {
        0x41U, 0x52U, 0x63U, 0x74U,
        0x85U, 0x96U, 0xa7U, 0xb8U
    };
    const uint8_t src_pattern[8] = {
        0x10U, 0x21U, 0x32U, 0x43U,
        0x54U, 0x65U, 0x76U, 0x87U
    };
    const uint8_t sentinel_before = trigger_earlier ? 0x4dU : 0x5dU;
    const uint8_t sentinel_after = trigger_earlier ? 0xd4U : 0xe5U;
    bool test_status = true;
    uint64_t saved_mstatus = 0;

    goto_priv(PRIV_M);
    ai_micro_cross_page_trigger_write_bytes(low_phys_start, initial_boundary, 4U);
    ai_micro_cross_page_trigger_write_bytes(high_phys_start, initial_boundary + 4U, 4U);
    ai_micro_cross_page_trigger_write_bytes(src_phys, src_pattern, 8U);
    write8(sentinel_before_addr, sentinel_before);
    write8(sentinel_after_addr, sentinel_after);

    goto_priv(PRIV_HS);
    hspt_init();

    goto_priv(PRIV_M);
    ai_micro_cross_page_set_hs_leaf(low_page, phys_page_base(low_page),
        PTE_V | PTE_AD | PTE_RWX);
    ai_micro_cross_page_set_hs_leaf(high_page, phys_page_base(high_page),
        PTE_V | PTE_AD | PTE_RWX);
    sfence_vma();
    ai_micro_cross_page_configure_pmp_window(deny_start, deny_end, AI_MICRO_PMP_TOR_NO_W);
    ai_micro_cross_page_clear_mcontrol6_slot(0);
    ai_micro_cross_page_clear_mcontrol6_slot(1);
    ai_micro_cross_page_arm_mcontrol6_slot(0, trigger_addr, AI_MICRO_CROSS_PAGE_MCONTROL6_STORE);

    saved_mstatus = ai_micro_cross_page_enable_machine_breakpoint_delivery();

    goto_priv(PRIV_HS);
    TEST_SETUP_EXCEPT();
    vle8_to_v6((const uint8_t *)src_vaddr, 8);
    TEST_ASSERT("a vector unit-stride load can stage the eight-byte payload before the vector-store trigger-vs-access-fault priority probe starts",
        excpt.triggered == false
    );

    TEST_SETUP_EXCEPT();
    vse8_from_v6((uint8_t *)boundary_vaddr, 8);
    if (trigger_earlier) {
        TEST_ASSERT("when a low-half vector-store element is armed for address trigger and only a later high-half element loses write permission, the repeated cross-page store still reports breakpoint first",
            excpt.triggered == true &&
            excpt.cause == CAUSE_BKP
        );
    } else {
        TEST_ASSERT("when the low-half vector-store element loses write permission before a later high-half element reaches its address trigger, the repeated cross-page store still reports store access fault first",
            excpt.triggered == true &&
            excpt.cause == CAUSE_SAF &&
            excpt.tval == boundary_vaddr
        );
    }

    goto_priv(PRIV_M);
    TEST_ASSERT("after the first vector-store trigger-vs-access-fault priority probe, both boundary fragments and the outer sentinels stay unchanged while the earlier winner is being checked",
        ai_micro_cross_page_trigger_bytes_match(low_phys_start, initial_boundary, 4U) &&
        ai_micro_cross_page_trigger_bytes_match(high_phys_start, initial_boundary + 4U, 4U) &&
        read8(sentinel_before_addr) == sentinel_before &&
        read8(sentinel_after_addr) == sentinel_after
    );
    ai_micro_cross_page_restore_machine_status(saved_mstatus);

    goto_priv(PRIV_HS);
    hspt_init();

    goto_priv(PRIV_M);
    ai_micro_cross_page_set_hs_leaf(low_page, phys_page_base(low_page),
        PTE_V | PTE_AD | PTE_RWX);
    ai_micro_cross_page_set_hs_leaf(high_page, phys_page_base(high_page),
        PTE_V | PTE_AD | PTE_RWX);
    sfence_vma();

    saved_mstatus = ai_micro_cross_page_enable_machine_breakpoint_delivery();

    goto_priv(PRIV_HS);
    TEST_SETUP_EXCEPT();
    vle8_to_v6((const uint8_t *)src_vaddr, 8);
    TEST_ASSERT("the vector unit-stride load can restage the same eight-byte payload before the second repeated vector-store priority probe",
        excpt.triggered == false
    );

    TEST_SETUP_EXCEPT();
    vse8_from_v6((uint8_t *)boundary_vaddr, 8);
    if (trigger_earlier) {
        TEST_ASSERT("on the second identical cross-page vector-store shape, the earlier low-half address trigger still wins before the later high-half access fault is exposed",
            excpt.triggered == true &&
            excpt.cause == CAUSE_BKP
        );
    } else {
        TEST_ASSERT("on the second identical cross-page vector-store shape, the earlier low-half access fault still suppresses the later high-half address trigger",
            excpt.triggered == true &&
            excpt.cause == CAUSE_SAF &&
            excpt.tval == boundary_vaddr
        );
    }

    goto_priv(PRIV_M);
    TEST_ASSERT("after the second repeated vector-store trigger-vs-access-fault priority probe, both boundary fragments and the outer sentinels still stay unchanged",
        ai_micro_cross_page_trigger_bytes_match(low_phys_start, initial_boundary, 4U) &&
        ai_micro_cross_page_trigger_bytes_match(high_phys_start, initial_boundary + 4U, 4U) &&
        read8(sentinel_before_addr) == sentinel_before &&
        read8(sentinel_after_addr) == sentinel_after
    );
    ai_micro_cross_page_restore_machine_status(saved_mstatus);

    if (trigger_earlier) {
        goto_priv(PRIV_HS);
        hspt_init();

        goto_priv(PRIV_M);
        ai_micro_cross_page_set_hs_leaf(low_page, phys_page_base(low_page),
            PTE_V | PTE_AD | PTE_RWX);
        ai_micro_cross_page_set_hs_leaf(high_page, phys_page_base(high_page),
            PTE_V | PTE_AD | PTE_RWX);
        sfence_vma();
        ai_micro_cross_page_clear_mcontrol6_slot(0);

        goto_priv(PRIV_HS);
        TEST_SETUP_EXCEPT();
        vle8_to_v6((const uint8_t *)src_vaddr, 8);
        TEST_ASSERT("after clearing the earlier low-half address trigger, the same eight-byte payload can still be restaged before the fallback later access-fault probe",
            excpt.triggered == false
        );

        TEST_SETUP_EXCEPT();
        vse8_from_v6((uint8_t *)boundary_vaddr, 8);
        TEST_ASSERT("after clearing the earlier low-half address trigger, the same cross-page vector store falls back to the later high-half store access fault",
            excpt.triggered == true &&
            excpt.cause == CAUSE_SAF &&
            excpt.tval == second_half_vaddr
        );

        goto_priv(PRIV_M);
        TEST_ASSERT("after the fallback high-half access-fault vector store, only the low-half bytes become visible while the denied high-half bytes and outer sentinels stay unchanged",
            ai_micro_cross_page_trigger_bytes_match(low_phys_start, src_pattern, 4U) &&
            ai_micro_cross_page_trigger_bytes_match(high_phys_start, initial_boundary + 4U, 4U) &&
            read8(sentinel_before_addr) == sentinel_before &&
            read8(sentinel_after_addr) == sentinel_after
        );
    } else {
        goto_priv(PRIV_M);
        ai_micro_cross_page_clear_pmp_window();
    }

    goto_priv(PRIV_M);
    ai_micro_cross_page_clear_mcontrol6_slot(0);
    ai_micro_cross_page_clear_mcontrol6_slot(1);
    ai_micro_cross_page_clear_pmp_window();

    return true;
}

static bool ai_micro_cross_page_vector_store_trigger_vs_misalign_priority(
    bool trigger_same_element)
{
    const enum test_page dst_page = VSRWXPbmt1_GURWXPbmt0;
    const enum test_page src_page = SWITCH1;
    const enum test_page scratch_page = SWITCH2;
    const uintptr_t window_phys =
        ai_micro_cross_page_trigger_pbmt_nc_backing_paddr() + 0x180U;
    const uintptr_t target_vaddr_base = hs_page_base(dst_page) + 0x180U;
    const uintptr_t misaligned_vaddr = target_vaddr_base + 1U;
    const uintptr_t trigger_addr = trigger_same_element ?
        misaligned_vaddr : (misaligned_vaddr + sizeof(uint64_t));
    const uintptr_t sentinel_before_addr = window_phys - 1U;
    const uintptr_t sentinel_after_addr = window_phys + 24U;
    const uintptr_t src_vaddr = hs_page_base(src_page) +
        (trigger_same_element ? 0x200U : 0x240U);
    const uintptr_t src_phys = phys_page_base(src_page) +
        (trigger_same_element ? 0x200U : 0x240U);
    const uintptr_t scratch_vaddr = hs_page_base(scratch_page) +
        (trigger_same_element ? 0x280U : 0x2c0U);
    const uintptr_t scratch_phys = phys_page_base(scratch_page) +
        (trigger_same_element ? 0x280U : 0x2c0U);
    const uint8_t initial_window[24] = {
        0x31U, 0x42U, 0x53U, 0x64U, 0x75U, 0x86U, 0x97U, 0xa8U,
        0xb9U, 0xcaU, 0xdbU, 0xecU, 0xfdU, 0x0eU, 0x1fU, 0x20U,
        0x31U, 0x42U, 0x53U, 0x64U, 0x75U, 0x86U, 0x97U, 0xa8U
    };
    const uint8_t src_pattern[16] = {
        0x10U, 0x32U, 0x54U, 0x76U, 0x98U, 0xbaU, 0xdcU, 0xfeU,
        0x89U, 0x67U, 0x45U, 0x23U, 0x01U, 0xefU, 0xcdU, 0xabU
    };
    const uint8_t scratch_seed[16] = {
        0xe1U, 0xd2U, 0xc3U, 0xb4U, 0xa5U, 0x96U, 0x87U, 0x78U,
        0x69U, 0x5aU, 0x4bU, 0x3cU, 0x2dU, 0x1eU, 0x0fU, 0xf0U
    };
    const uint8_t sentinel_before = trigger_same_element ? 0x4cU : 0x5cU;
    const uint8_t sentinel_after = trigger_same_element ? 0xc4U : 0xd5U;
    bool test_status = true;
    uint64_t saved_mstatus = 0;

    goto_priv(PRIV_M);
    ai_micro_cross_page_trigger_write_bytes(window_phys, initial_window,
        sizeof(initial_window));
    ai_micro_cross_page_trigger_write_bytes(src_phys, src_pattern,
        sizeof(src_pattern));
    ai_micro_cross_page_trigger_write_bytes(scratch_phys, scratch_seed,
        sizeof(scratch_seed));
    write8(sentinel_before_addr, sentinel_before);
    write8(sentinel_after_addr, sentinel_after);

    goto_priv(PRIV_HS);
    ai_micro_cross_page_trigger_prepare_pbmt_nc_alias(dst_page);

    ai_micro_cross_page_clear_mcontrol6_slot(0);
    ai_micro_cross_page_clear_mcontrol6_slot(1);
    ai_micro_cross_page_arm_mcontrol6_slot(0, trigger_addr,
        AI_MICRO_CROSS_PAGE_MCONTROL6_STORE);
    saved_mstatus = ai_micro_cross_page_enable_machine_breakpoint_delivery();

    goto_priv(PRIV_HS);
    TEST_SETUP_EXCEPT();
    vle64_to_v6((const uint64_t *)src_vaddr, 2);
    TEST_ASSERT("a vector e64 load can stage the reference payload before the PBMT=NC misaligned vector-store trigger-vs-access-fault probe starts",
        excpt.triggered == false
    );

    TEST_SETUP_EXCEPT();
    vse64_from_v6((uint64_t *)misaligned_vaddr, 2);
    if (trigger_same_element) {
        TEST_ASSERT("when the store-address trigger is armed on the same first misaligned PBMT=NC vector-store element, breakpoint still wins over the equal-element access-fault report",
            excpt.triggered == true &&
            excpt.cause == CAUSE_BKP
        );
    } else {
        TEST_ASSERT("when only a later second-element address trigger is armed on the same misaligned PBMT=NC vector-store shape, the earlier first-element store access-fault report still wins",
            excpt.triggered == true &&
            excpt.cause == CAUSE_SAF &&
            excpt.tval == misaligned_vaddr
        );
    }

    goto_priv(PRIV_M);
    TEST_ASSERT("after the first PBMT=NC misaligned vector-store trigger-vs-access-fault probe, the touched window and both outer sentinels still stay unchanged",
        ai_micro_cross_page_trigger_bytes_match(window_phys, initial_window,
            sizeof(initial_window)) &&
        read8(sentinel_before_addr) == sentinel_before &&
        read8(sentinel_after_addr) == sentinel_after
    );
    ai_micro_cross_page_restore_machine_status(saved_mstatus);

    goto_priv(PRIV_HS);
    ai_micro_cross_page_trigger_prepare_pbmt_nc_alias(dst_page);

    saved_mstatus = ai_micro_cross_page_enable_machine_breakpoint_delivery();

    goto_priv(PRIV_HS);
    TEST_SETUP_EXCEPT();
    vle64_to_v6((const uint64_t *)src_vaddr, 2);
    TEST_ASSERT("the vector e64 payload can be restaged before the second repeated PBMT=NC misaligned vector-store trigger-vs-access-fault probe",
        excpt.triggered == false
    );

    TEST_SETUP_EXCEPT();
    vse64_from_v6((uint64_t *)misaligned_vaddr, 2);
    if (trigger_same_element) {
        TEST_ASSERT("on the second identical misaligned PBMT=NC vector-store shape, the same-element address trigger still wins before the equal-element access fault is reported",
            excpt.triggered == true &&
            excpt.cause == CAUSE_BKP
        );
    } else {
        TEST_ASSERT("on the second identical misaligned PBMT=NC vector-store shape, the earlier first-element access fault still suppresses the later second-element address trigger",
            excpt.triggered == true &&
            excpt.cause == CAUSE_SAF &&
            excpt.tval == misaligned_vaddr
        );
    }

    goto_priv(PRIV_M);
    TEST_ASSERT("after the second repeated PBMT=NC misaligned vector-store trigger-vs-access-fault probe, the touched window and both outer sentinels still stay unchanged",
        ai_micro_cross_page_trigger_bytes_match(window_phys, initial_window,
            sizeof(initial_window)) &&
        read8(sentinel_before_addr) == sentinel_before &&
        read8(sentinel_after_addr) == sentinel_after
    );
    ai_micro_cross_page_restore_machine_status(saved_mstatus);

    if (trigger_same_element) {
        ai_micro_cross_page_clear_mcontrol6_slot(0);
    }

    goto_priv(PRIV_HS);
    hspt_init();
    sfence_vma();

    TEST_SETUP_EXCEPT();
    vle64_to_v6((const uint64_t *)src_vaddr, 2);
    TEST_ASSERT("after the PBMT=NC misaligned vector-store trigger-vs-access-fault probes, the same vector e64 payload can still be restaged for a clean aligned follow-up store",
        excpt.triggered == false
    );

    TEST_SETUP_EXCEPT();
    vse64_from_v6((uint64_t *)scratch_vaddr, 2);
    TEST_ASSERT("after the PBMT=NC misaligned vector-store trigger-vs-access-fault probes, a clean aligned vector e64 store still completes normally",
        excpt.triggered == false
    );

    fence_iorw_iorw();
    goto_priv(PRIV_M);
    TEST_ASSERT("after the PBMT=NC misaligned vector-store trigger-vs-access-fault probes, the clean aligned follow-up store copies only the staged payload into scratch and leaves the original misaligned target window untouched",
        ai_micro_cross_page_trigger_bytes_match(scratch_phys, src_pattern,
            sizeof(src_pattern)) &&
        ai_micro_cross_page_trigger_bytes_match(window_phys, initial_window,
            sizeof(initial_window)) &&
        read8(sentinel_before_addr) == sentinel_before &&
        read8(sentinel_after_addr) == sentinel_after
    );

    ai_micro_cross_page_clear_mcontrol6_slot(0);
    ai_micro_cross_page_clear_mcontrol6_slot(1);
    ai_micro_cross_page_trigger_reset_pbmt_nc_env();
    return true;
}

static bool ai_micro_cross_page_scalar_store_repeated_breakpoint_followup(
    enum ai_micro_cross_page_repeated_followup_mode mode)
{
    enum test_page low_page = VSRWX_GRWX;
    enum test_page high_page = VSRWX_GRW;
    const uint8_t warm_pattern[8] = {
        0x13, 0x05, 0x30, 0x00,
        0x67, 0x80, 0x00, 0x00
    };
    const uint8_t patched_pattern[8] = {
        0x13, 0x05, 0x70, 0x00,
        0x67, 0x80, 0x00, 0x00
    };
    const uintptr_t vaddr = ai_micro_cross_page_trigger_addr(low_page, 4U);
    const uintptr_t second_half_vaddr = vaddr + 4U;
    const uintptr_t low_phys_start = phys_page_base(low_page) + PAGE_SIZE - 4U;
    const uintptr_t sentinel_before_addr = low_phys_start - 1U;
    const uintptr_t sentinel_after_addr = phys_page_base(high_page) + 4U;
    uint64_t (*fetch_func)(void) = (uint64_t (*)(void))vaddr;
    uint64_t observed = 0;
    uint64_t saved_mstatus = 0;
    uintptr_t bad_vaddr = 0;
    uintptr_t fetch_target = 0;
    uint8_t sentinel_before = 0;
    uint8_t sentinel_after = 0;
    bool test_status = true;

    switch (mode) {
    case AI_MICRO_CROSS_PAGE_FOLLOWUP_NONCANONICAL:
        sentinel_before = 0x5dU;
        sentinel_after = 0xd5U;
        bad_vaddr = ai_micro_cross_page_trigger_make_noncanonical_vaddr(vaddr);
        break;
    case AI_MICRO_CROSS_PAGE_FOLLOWUP_ALL_ONES:
        sentinel_before = 0x5eU;
        sentinel_after = 0xe5U;
        bad_vaddr = (uintptr_t)-1;
        fetch_target = bad_vaddr & ~1ULL;
        break;
    case AI_MICRO_CROSS_PAGE_FOLLOWUP_LEAF_OVERFLOW_FAULT:
        sentinel_before = 0x5fU;
        sentinel_after = 0xf5U;
        break;
    case AI_MICRO_CROSS_PAGE_FOLLOWUP_LEAF_OVERFLOW_FIX:
        sentinel_before = 0x6aU;
        sentinel_after = 0xa6U;
        break;
    }

    goto_priv(PRIV_M);
    ai_micro_cross_page_trigger_write_bytes(low_phys_start, warm_pattern, 8U);
    write8(sentinel_before_addr, sentinel_before);
    write8(sentinel_after_addr, sentinel_after);

    goto_priv(PRIV_HS);
    hspt_init();

    goto_priv(PRIV_M);
    ai_micro_cross_page_set_hs_leaf(low_page, phys_page_base(low_page),
        PTE_V | PTE_AD | PTE_RWX);
    ai_micro_cross_page_set_hs_leaf(high_page, phys_page_base(high_page),
        PTE_V | PTE_AD | PTE_RWX);
    sfence_vma();

    goto_priv(PRIV_HS);
    ai_micro_cross_page_fence_i();

    TEST_SETUP_EXCEPT();
    observed = fetch_func();
    TEST_ASSERT("before the scalar cross-page repeated execute-trigger follow-up probe rewrites the two-page executable HS block, the warmed fetch path still returns the original value",
        excpt.triggered == false &&
        observed == 3ULL
    );

    TEST_SETUP_EXCEPT();
    sd(vaddr, 0x0000806700700513ULL);
    TEST_ASSERT("a scalar cross-page store can rewrite the two-page executable HS block before the repeated execute-trigger follow-up probe",
        excpt.triggered == false
    );

    goto_priv(PRIV_M);
    TEST_ASSERT("after the scalar cross-page store rewrites the executable HS block, the patched instruction bytes are visible in backing memory and the adjacent sentinels stay unchanged before the repeated execute-trigger follow-up probe",
        ai_micro_cross_page_trigger_bytes_match(low_phys_start, patched_pattern, 8U) &&
        read8(sentinel_before_addr) == sentinel_before &&
        read8(sentinel_after_addr) == sentinel_after
    );

    goto_priv(PRIV_HS);
    ai_micro_cross_page_fence_i();
    ai_micro_cross_page_clear_mcontrol6_slot(0);
    ai_micro_cross_page_clear_mcontrol6_slot(1);
    ai_micro_cross_page_arm_mcontrol6_slot(0, vaddr, AI_MICRO_CROSS_PAGE_MCONTROL6_EXECUTE);
    saved_mstatus = ai_micro_cross_page_enable_machine_breakpoint_delivery();
    goto_priv(PRIV_HS);

    TEST_SETUP_EXCEPT();
    fetch_func();
    TEST_ASSERT("after the scalar cross-page store and fence.i, the first fetch to the rewritten executable HS block still reports breakpoint before the later follow-up state is exposed",
        excpt.triggered == true &&
        excpt.cause == CAUSE_BKP
    );

    goto_priv(PRIV_M);
    TEST_ASSERT("the first trapped fetch in the scalar cross-page repeated execute-trigger follow-up corner still leaves the patched bytes and adjacent sentinels unchanged",
        ai_micro_cross_page_trigger_bytes_match(low_phys_start, patched_pattern, 8U) &&
        read8(sentinel_before_addr) == sentinel_before &&
        read8(sentinel_after_addr) == sentinel_after
    );
    ai_micro_cross_page_restore_machine_status(saved_mstatus);

    saved_mstatus = ai_micro_cross_page_enable_machine_breakpoint_delivery();
    goto_priv(PRIV_HS);
    TEST_SETUP_EXCEPT();
    fetch_func();
    TEST_ASSERT("after the scalar cross-page store and fence.i, the second fetch to the rewritten executable HS block still reports breakpoint while the execute-PC trigger remains armed before the later follow-up state is exposed",
        excpt.triggered == true &&
        excpt.cause == CAUSE_BKP
    );

    goto_priv(PRIV_M);
    TEST_ASSERT("the second trapped fetch in the scalar cross-page repeated execute-trigger follow-up corner still leaves the patched bytes and adjacent sentinels unchanged",
        ai_micro_cross_page_trigger_bytes_match(low_phys_start, patched_pattern, 8U) &&
        read8(sentinel_before_addr) == sentinel_before &&
        read8(sentinel_after_addr) == sentinel_after
    );
    ai_micro_cross_page_restore_machine_status(saved_mstatus);
    ai_micro_cross_page_clear_mcontrol6_slot(0);

    switch (mode) {
    case AI_MICRO_CROSS_PAGE_FOLLOWUP_NONCANONICAL:
        goto_priv(PRIV_HS);
        ai_micro_cross_page_fence_i();
        TEST_SETUP_EXCEPT();
        TEST_EXEC_EXCEPT(bad_vaddr);
        TEST_ASSERT("after clearing the execute-PC trigger, the same scalar-rewritten two-page executable HS block falls back to instruction page fault for the non-canonical fetch target with the raw VA in tval",
            excpt.triggered == true &&
            excpt.cause == CAUSE_IPF &&
            excpt.tval == bad_vaddr
        );

        goto_priv(PRIV_M);
        TEST_ASSERT("after repeated execute-trigger breakpoint hits and the later non-canonical fetch page fault, the patched bytes remain intact and the adjacent sentinels stay unchanged in the scalar cross-page corner",
            ai_micro_cross_page_trigger_bytes_match(low_phys_start, patched_pattern, 8U) &&
            read8(sentinel_before_addr) == sentinel_before &&
            read8(sentinel_after_addr) == sentinel_after
        );

        goto_priv(PRIV_HS);
        TEST_SETUP_EXCEPT();
        observed = fetch_func();
        TEST_ASSERT("after the scalar cross-page execute-trigger plus non-canonical fetch fault sequence, a subsequent canonical fetch still executes the patched instruction stream",
            excpt.triggered == false &&
            observed == 7ULL
        );

        goto_priv(PRIV_M);
        TEST_ASSERT("after canonical recovery from the scalar cross-page execute-trigger plus non-canonical fetch corner, the patched bytes remain intact and the adjacent sentinels stay unchanged",
            ai_micro_cross_page_trigger_bytes_match(low_phys_start, patched_pattern, 8U) &&
            read8(sentinel_before_addr) == sentinel_before &&
            read8(sentinel_after_addr) == sentinel_after
        );
        break;
    case AI_MICRO_CROSS_PAGE_FOLLOWUP_ALL_ONES:
        goto_priv(PRIV_HS);
        ai_micro_cross_page_fence_i();
        TEST_SETUP_EXCEPT();
        TEST_EXEC_EXCEPT(bad_vaddr);
        TEST_ASSERT("after clearing the execute-PC trigger, the same scalar-rewritten two-page executable HS block falls back to instruction page fault for the all-ones fetch target with the JALR-cleared target in tval",
            excpt.triggered == true &&
            excpt.cause == CAUSE_IPF &&
            excpt.tval == fetch_target
        );

        goto_priv(PRIV_M);
        TEST_ASSERT("after repeated execute-trigger breakpoint hits and the later all-ones fetch page fault, the patched bytes remain intact and the adjacent sentinels stay unchanged in the scalar cross-page corner",
            ai_micro_cross_page_trigger_bytes_match(low_phys_start, patched_pattern, 8U) &&
            read8(sentinel_before_addr) == sentinel_before &&
            read8(sentinel_after_addr) == sentinel_after
        );

        goto_priv(PRIV_HS);
        TEST_SETUP_EXCEPT();
        observed = fetch_func();
        TEST_ASSERT("after the scalar cross-page execute-trigger plus all-ones fetch fault sequence, a subsequent canonical fetch still executes the patched instruction stream",
            excpt.triggered == false &&
            observed == 7ULL
        );

        goto_priv(PRIV_M);
        TEST_ASSERT("after canonical recovery from the scalar cross-page execute-trigger plus all-ones fetch corner, the patched bytes remain intact and the adjacent sentinels stay unchanged",
            ai_micro_cross_page_trigger_bytes_match(low_phys_start, patched_pattern, 8U) &&
            read8(sentinel_before_addr) == sentinel_before &&
            read8(sentinel_after_addr) == sentinel_after
        );
        break;
    case AI_MICRO_CROSS_PAGE_FOLLOWUP_LEAF_OVERFLOW_FAULT:
        goto_priv(PRIV_M);
        *ai_micro_cross_page_hs_leaf_entry(high_page) |= (1ULL << 46);
        sfence_vma();

        goto_priv(PRIV_HS);
        ai_micro_cross_page_fence_i();
        TEST_SETUP_EXCEPT();
        TEST_EXEC_EXCEPT(vaddr);
        TEST_ASSERT("after clearing the execute-PC trigger, the same scalar-rewritten two-page executable HS block falls back to instruction access fault when the upper-page leaf PPN overflows",
            excpt.triggered == true &&
            excpt.cause == CAUSE_IAF &&
            excpt.tval == second_half_vaddr
        );

        goto_priv(PRIV_M);
        TEST_ASSERT("after repeated execute-trigger breakpoint hits and the later upper-page leaf-overflow fetch access fault, the patched bytes remain intact and the adjacent sentinels stay unchanged in the scalar cross-page corner",
            ai_micro_cross_page_trigger_bytes_match(low_phys_start, patched_pattern, 8U) &&
            read8(sentinel_before_addr) == sentinel_before &&
            read8(sentinel_after_addr) == sentinel_after
        );
        break;
    case AI_MICRO_CROSS_PAGE_FOLLOWUP_LEAF_OVERFLOW_FIX:
        goto_priv(PRIV_M);
        *ai_micro_cross_page_hs_leaf_entry(high_page) |= (1ULL << 46);
        sfence_vma();

        goto_priv(PRIV_HS);
        ai_micro_cross_page_fence_i();
        TEST_SETUP_EXCEPT();
        TEST_EXEC_EXCEPT(vaddr);
        TEST_ASSERT("after clearing the execute-PC trigger, the same scalar-rewritten two-page executable HS block first falls back to instruction access fault when the upper-page leaf PPN overflows",
            excpt.triggered == true &&
            excpt.cause == CAUSE_IAF &&
            excpt.tval == second_half_vaddr
        );

        goto_priv(PRIV_M);
        TEST_ASSERT("the upper-page leaf-overflow fetch access fault before repair still leaves the patched bytes and the adjacent sentinels unchanged in the scalar cross-page execute-trigger corner",
            ai_micro_cross_page_trigger_bytes_match(low_phys_start, patched_pattern, 8U) &&
            read8(sentinel_before_addr) == sentinel_before &&
            read8(sentinel_after_addr) == sentinel_after
        );
        ai_micro_cross_page_set_hs_leaf(low_page, phys_page_base(low_page),
            PTE_V | PTE_AD | PTE_RWX);
        ai_micro_cross_page_set_hs_leaf(high_page, phys_page_base(high_page),
            PTE_V | PTE_AD | PTE_RWX);
        sfence_vma();

        goto_priv(PRIV_HS);
        ai_micro_cross_page_fence_i();
        TEST_SETUP_EXCEPT();
        observed = fetch_func();
        TEST_ASSERT("after repairing the upper-page leaf entry, the scalar-rewritten two-page executable HS block completes normally again and returns the patched value",
            excpt.triggered == false &&
            observed == 7ULL
        );

        goto_priv(PRIV_M);
        TEST_ASSERT("after repeated execute-trigger breakpoint hits, the later upper-page leaf-overflow fetch access fault, and final leaf repair, the patched bytes remain intact and the adjacent sentinels stay unchanged in the scalar cross-page corner",
            ai_micro_cross_page_trigger_bytes_match(low_phys_start, patched_pattern, 8U) &&
            read8(sentinel_before_addr) == sentinel_before &&
            read8(sentinel_after_addr) == sentinel_after
        );
        break;
    }

    goto_priv(PRIV_HS);
    hspt_init();
    sfence_vma();

    return test_status;
}

static bool ai_micro_cross_page_vector_store_repeated_breakpoint_followup(
    enum ai_micro_cross_page_repeated_followup_mode mode)
{
    enum test_page low_page = VSRWX_GRWX;
    enum test_page high_page = VSRWX_GRW;
    const uint8_t warm_pattern[8] = {
        0x13, 0x05, 0x30, 0x00,
        0x67, 0x80, 0x00, 0x00
    };
    const uint8_t patched_pattern[8] = {
        0x13, 0x05, 0x70, 0x00,
        0x67, 0x80, 0x00, 0x00
    };
    const uintptr_t source_vaddr = hs_page_base(VSRW_GRWX) + 0x1f0U;
    const uintptr_t source_phys = phys_page_base(VSRW_GRWX) + 0x1f0U;
    const uintptr_t vaddr = ai_micro_cross_page_trigger_addr(low_page, 4U);
    const uintptr_t second_half_vaddr = vaddr + 4U;
    const uintptr_t low_phys_start = phys_page_base(low_page) + PAGE_SIZE - 4U;
    const uintptr_t sentinel_before_addr = low_phys_start - 1U;
    const uintptr_t sentinel_after_addr = phys_page_base(high_page) + 4U;
    uint64_t (*fetch_func)(void) = (uint64_t (*)(void))vaddr;
    uint64_t observed = 0;
    uint64_t saved_mstatus = 0;
    uintptr_t bad_vaddr = 0;
    uintptr_t fetch_target = 0;
    uint8_t sentinel_before = 0;
    uint8_t sentinel_after = 0;
    bool test_status = true;

    switch (mode) {
    case AI_MICRO_CROSS_PAGE_FOLLOWUP_NONCANONICAL:
        sentinel_before = 0x3fU;
        sentinel_after = 0xf3U;
        bad_vaddr = ai_micro_cross_page_trigger_make_noncanonical_vaddr(vaddr);
        break;
    case AI_MICRO_CROSS_PAGE_FOLLOWUP_ALL_ONES:
        sentinel_before = 0x40U;
        sentinel_after = 0xa4U;
        bad_vaddr = (uintptr_t)-1;
        fetch_target = bad_vaddr & ~1ULL;
        break;
    case AI_MICRO_CROSS_PAGE_FOLLOWUP_LEAF_OVERFLOW_FAULT:
        sentinel_before = 0x41U;
        sentinel_after = 0xb4U;
        break;
    case AI_MICRO_CROSS_PAGE_FOLLOWUP_LEAF_OVERFLOW_FIX:
        sentinel_before = 0x42U;
        sentinel_after = 0xc4U;
        break;
    }

    goto_priv(PRIV_M);
    ai_micro_cross_page_trigger_write_bytes(low_phys_start, warm_pattern, 8U);
    ai_micro_cross_page_trigger_write_bytes(source_phys, patched_pattern, 8U);
    write8(sentinel_before_addr, sentinel_before);
    write8(sentinel_after_addr, sentinel_after);

    goto_priv(PRIV_HS);
    hspt_init();

    goto_priv(PRIV_M);
    ai_micro_cross_page_set_hs_leaf(low_page, phys_page_base(low_page),
        PTE_V | PTE_AD | PTE_RWX);
    ai_micro_cross_page_set_hs_leaf(high_page, phys_page_base(high_page),
        PTE_V | PTE_AD | PTE_RWX);
    sfence_vma();

    goto_priv(PRIV_HS);
    ai_micro_cross_page_fence_i();

    TEST_SETUP_EXCEPT();
    observed = fetch_func();
    TEST_ASSERT("before the vector cross-page repeated execute-trigger follow-up probe rewrites the two-page executable HS block, the warmed fetch path still returns the original value",
        excpt.triggered == false &&
        observed == 3ULL
    );

    TEST_SETUP_EXCEPT();
    vle8_to_v6((const uint8_t *)source_vaddr, 8);
    TEST_ASSERT("a vector unit-stride load can stage the replacement instruction bytes before the vector cross-page repeated execute-trigger follow-up probe",
        excpt.triggered == false
    );

    TEST_SETUP_EXCEPT();
    vse8_from_v6((uint8_t *)vaddr, 8);
    TEST_ASSERT("a vector unit-stride store can rewrite the two-page executable HS block across the page boundary before the repeated execute-trigger follow-up probe",
        excpt.triggered == false
    );

    goto_priv(PRIV_M);
    TEST_ASSERT("after the vector cross-page store rewrites the executable HS block, the patched instruction bytes are visible in backing memory and the adjacent sentinels stay unchanged before the repeated execute-trigger follow-up probe",
        ai_micro_cross_page_trigger_bytes_match(low_phys_start, patched_pattern, 8U) &&
        read8(sentinel_before_addr) == sentinel_before &&
        read8(sentinel_after_addr) == sentinel_after
    );

    goto_priv(PRIV_HS);
    ai_micro_cross_page_fence_i();
    ai_micro_cross_page_clear_mcontrol6_slot(0);
    ai_micro_cross_page_clear_mcontrol6_slot(1);
    ai_micro_cross_page_arm_mcontrol6_slot(0, vaddr, AI_MICRO_CROSS_PAGE_MCONTROL6_EXECUTE);
    saved_mstatus = ai_micro_cross_page_enable_machine_breakpoint_delivery();
    goto_priv(PRIV_HS);

    TEST_SETUP_EXCEPT();
    fetch_func();
    TEST_ASSERT("after the vector cross-page store and fence.i, the first fetch to the rewritten executable HS block still reports breakpoint before the later follow-up state is exposed",
        excpt.triggered == true &&
        excpt.cause == CAUSE_BKP
    );

    goto_priv(PRIV_M);
    TEST_ASSERT("the first trapped fetch in the vector cross-page repeated execute-trigger follow-up corner still leaves the patched bytes and adjacent sentinels unchanged",
        ai_micro_cross_page_trigger_bytes_match(low_phys_start, patched_pattern, 8U) &&
        read8(sentinel_before_addr) == sentinel_before &&
        read8(sentinel_after_addr) == sentinel_after
    );
    ai_micro_cross_page_restore_machine_status(saved_mstatus);

    saved_mstatus = ai_micro_cross_page_enable_machine_breakpoint_delivery();
    goto_priv(PRIV_HS);
    TEST_SETUP_EXCEPT();
    fetch_func();
    TEST_ASSERT("after the vector cross-page store and fence.i, the second fetch to the rewritten executable HS block still reports breakpoint while the execute-PC trigger remains armed before the later follow-up state is exposed",
        excpt.triggered == true &&
        excpt.cause == CAUSE_BKP
    );

    goto_priv(PRIV_M);
    TEST_ASSERT("the second trapped fetch in the vector cross-page repeated execute-trigger follow-up corner still leaves the patched bytes and adjacent sentinels unchanged",
        ai_micro_cross_page_trigger_bytes_match(low_phys_start, patched_pattern, 8U) &&
        read8(sentinel_before_addr) == sentinel_before &&
        read8(sentinel_after_addr) == sentinel_after
    );
    ai_micro_cross_page_restore_machine_status(saved_mstatus);
    ai_micro_cross_page_clear_mcontrol6_slot(0);

    switch (mode) {
    case AI_MICRO_CROSS_PAGE_FOLLOWUP_NONCANONICAL:
        goto_priv(PRIV_HS);
        ai_micro_cross_page_fence_i();
        TEST_SETUP_EXCEPT();
        TEST_EXEC_EXCEPT(bad_vaddr);
        TEST_ASSERT("after clearing the execute-PC trigger, the same vector-rewritten two-page executable HS block falls back to instruction page fault for the non-canonical fetch target with the raw VA in tval",
            excpt.triggered == true &&
            excpt.cause == CAUSE_IPF &&
            excpt.tval == bad_vaddr
        );

        goto_priv(PRIV_M);
        TEST_ASSERT("after repeated execute-trigger breakpoint hits and the later non-canonical fetch page fault, the patched bytes remain intact and the adjacent sentinels stay unchanged in the vector cross-page corner",
            ai_micro_cross_page_trigger_bytes_match(low_phys_start, patched_pattern, 8U) &&
            read8(sentinel_before_addr) == sentinel_before &&
            read8(sentinel_after_addr) == sentinel_after
        );

        goto_priv(PRIV_HS);
        TEST_SETUP_EXCEPT();
        observed = fetch_func();
        TEST_ASSERT("after the vector cross-page execute-trigger plus non-canonical fetch fault sequence, a subsequent canonical fetch still executes the patched instruction stream",
            excpt.triggered == false &&
            observed == 7ULL
        );

        goto_priv(PRIV_M);
        TEST_ASSERT("after canonical recovery from the vector cross-page execute-trigger plus non-canonical fetch corner, the patched bytes remain intact and the adjacent sentinels stay unchanged",
            ai_micro_cross_page_trigger_bytes_match(low_phys_start, patched_pattern, 8U) &&
            read8(sentinel_before_addr) == sentinel_before &&
            read8(sentinel_after_addr) == sentinel_after
        );
        break;
    case AI_MICRO_CROSS_PAGE_FOLLOWUP_ALL_ONES:
        goto_priv(PRIV_HS);
        ai_micro_cross_page_fence_i();
        TEST_SETUP_EXCEPT();
        TEST_EXEC_EXCEPT(bad_vaddr);
        TEST_ASSERT("after clearing the execute-PC trigger, the same vector-rewritten two-page executable HS block falls back to instruction page fault for the all-ones fetch target with the JALR-cleared target in tval",
            excpt.triggered == true &&
            excpt.cause == CAUSE_IPF &&
            excpt.tval == fetch_target
        );

        goto_priv(PRIV_M);
        TEST_ASSERT("after repeated execute-trigger breakpoint hits and the later all-ones fetch page fault, the patched bytes remain intact and the adjacent sentinels stay unchanged in the vector cross-page corner",
            ai_micro_cross_page_trigger_bytes_match(low_phys_start, patched_pattern, 8U) &&
            read8(sentinel_before_addr) == sentinel_before &&
            read8(sentinel_after_addr) == sentinel_after
        );

        goto_priv(PRIV_HS);
        TEST_SETUP_EXCEPT();
        observed = fetch_func();
        TEST_ASSERT("after the vector cross-page execute-trigger plus all-ones fetch fault sequence, a subsequent canonical fetch still executes the patched instruction stream",
            excpt.triggered == false &&
            observed == 7ULL
        );

        goto_priv(PRIV_M);
        TEST_ASSERT("after canonical recovery from the vector cross-page execute-trigger plus all-ones fetch corner, the patched bytes remain intact and the adjacent sentinels stay unchanged",
            ai_micro_cross_page_trigger_bytes_match(low_phys_start, patched_pattern, 8U) &&
            read8(sentinel_before_addr) == sentinel_before &&
            read8(sentinel_after_addr) == sentinel_after
        );
        break;
    case AI_MICRO_CROSS_PAGE_FOLLOWUP_LEAF_OVERFLOW_FAULT:
        goto_priv(PRIV_M);
        *ai_micro_cross_page_hs_leaf_entry(high_page) |= (1ULL << 46);
        sfence_vma();

        goto_priv(PRIV_HS);
        ai_micro_cross_page_fence_i();
        TEST_SETUP_EXCEPT();
        TEST_EXEC_EXCEPT(vaddr);
        TEST_ASSERT("after clearing the execute-PC trigger, the same vector-rewritten two-page executable HS block falls back to instruction access fault when the upper-page leaf PPN overflows",
            excpt.triggered == true &&
            excpt.cause == CAUSE_IAF &&
            excpt.tval == second_half_vaddr
        );

        goto_priv(PRIV_M);
        TEST_ASSERT("after repeated execute-trigger breakpoint hits and the later upper-page leaf-overflow fetch access fault, the patched bytes remain intact and the adjacent sentinels stay unchanged in the vector cross-page corner",
            ai_micro_cross_page_trigger_bytes_match(low_phys_start, patched_pattern, 8U) &&
            read8(sentinel_before_addr) == sentinel_before &&
            read8(sentinel_after_addr) == sentinel_after
        );
        break;
    case AI_MICRO_CROSS_PAGE_FOLLOWUP_LEAF_OVERFLOW_FIX:
        goto_priv(PRIV_M);
        *ai_micro_cross_page_hs_leaf_entry(high_page) |= (1ULL << 46);
        sfence_vma();

        goto_priv(PRIV_HS);
        ai_micro_cross_page_fence_i();
        TEST_SETUP_EXCEPT();
        TEST_EXEC_EXCEPT(vaddr);
        TEST_ASSERT("after clearing the execute-PC trigger, the same vector-rewritten two-page executable HS block first falls back to instruction access fault when the upper-page leaf PPN overflows",
            excpt.triggered == true &&
            excpt.cause == CAUSE_IAF &&
            excpt.tval == second_half_vaddr
        );

        goto_priv(PRIV_M);
        TEST_ASSERT("the upper-page leaf-overflow fetch access fault before repair still leaves the patched bytes and the adjacent sentinels unchanged in the vector cross-page execute-trigger corner",
            ai_micro_cross_page_trigger_bytes_match(low_phys_start, patched_pattern, 8U) &&
            read8(sentinel_before_addr) == sentinel_before &&
            read8(sentinel_after_addr) == sentinel_after
        );
        ai_micro_cross_page_set_hs_leaf(low_page, phys_page_base(low_page),
            PTE_V | PTE_AD | PTE_RWX);
        ai_micro_cross_page_set_hs_leaf(high_page, phys_page_base(high_page),
            PTE_V | PTE_AD | PTE_RWX);
        sfence_vma();

        goto_priv(PRIV_HS);
        ai_micro_cross_page_fence_i();
        TEST_SETUP_EXCEPT();
        observed = fetch_func();
        TEST_ASSERT("after repairing the upper-page leaf entry, the vector-rewritten two-page executable HS block completes normally again and returns the patched value",
            excpt.triggered == false &&
            observed == 7ULL
        );

        goto_priv(PRIV_M);
        TEST_ASSERT("after repeated execute-trigger breakpoint hits, the later upper-page leaf-overflow fetch access fault, and final leaf repair, the patched bytes remain intact and the adjacent sentinels stay unchanged in the vector cross-page corner",
            ai_micro_cross_page_trigger_bytes_match(low_phys_start, patched_pattern, 8U) &&
            read8(sentinel_before_addr) == sentinel_before &&
            read8(sentinel_after_addr) == sentinel_after
        );
        break;
    }

    goto_priv(PRIV_HS);
    hspt_init();
    sfence_vma();

    return test_status;
}

__attribute__((naked, noinline, aligned(4))) static uint64_t ai_micro_cross_page_execute_load_once(
    uintptr_t addr)
{
    asm volatile(
        ".option push\n"
        ".option norvc\n"
        "ld a0, 0(a0)\n"
        "ret\n"
        ".option pop\n");
}

__attribute__((naked, noinline, aligned(4))) static void ai_micro_cross_page_execute_store_once(
    uintptr_t addr, uint64_t value)
{
    asm volatile(
        ".option push\n"
        ".option norvc\n"
        "sd a1, 0(a0)\n"
        "ret\n"
        ".option pop\n");
}

static bool ai_micro_cross_page_scalar_load_after_vector_store_execute_and_address_trigger_repeated_breakpoint_then_completion(
    bool check_adjacent)
{
    enum test_page low_page = VSRW_GRW;
    enum test_page high_page = VSRW_GRX;
    const uint8_t source_pattern[16] = {
        0x20, 0x21, 0x22, 0x23,
        0x24, 0x25, 0x26, 0x27,
        0x28, 0x29, 0x2a, 0x2b,
        0x2c, 0x2d, 0x2e, 0x2f
    };
    uintptr_t source_vaddr = hs_page_base(VSRWX_GRWX) + 0x320U;
    uintptr_t source_phys = phys_page_base(VSRWX_GRWX) + 0x320U;
    uintptr_t vector_vaddr = ai_micro_cross_page_trigger_addr(low_page, 8U);
    uintptr_t scalar_vaddr = ai_micro_cross_page_trigger_addr(low_page, 4U);
    uintptr_t low_phys = ai_micro_cross_page_trigger_page_end_phys(low_page);
    uintptr_t high_phys = phys_page_base(high_page);
    uintptr_t sentinel_before_addr = low_phys - 1U;
    uintptr_t sentinel_after_addr = high_phys + 8U;
    uintptr_t trigger_pc = (uintptr_t)ai_micro_cross_page_execute_load_once;
    const uint8_t sentinel_before = 0x2cU;
    const uint8_t sentinel_after = 0xc2U;
    uint64_t saved_mstatus = 0;
    uint64_t value = 0;
    bool test_status = true;

    goto_priv(PRIV_M);
    ai_micro_cross_page_trigger_write_bytes(source_phys, source_pattern, 16U);
    write8(sentinel_before_addr, sentinel_before);
    write8(sentinel_after_addr, sentinel_after);

    goto_priv(PRIV_HS);
    hspt_init();
    sfence_vma();

    TEST_SETUP_EXCEPT();
    vle8_to_v6((const uint8_t *)source_vaddr, 16);
    TEST_ASSERT("a vector unit-stride load can stage a 16-byte payload before the mixed vector-store then scalar cross-page trigger-load probe",
        excpt.triggered == false
    );

    TEST_SETUP_EXCEPT();
    vse8_from_v6((uint8_t *)vector_vaddr, 16);
    TEST_ASSERT("a vector unit-stride store can write the staged payload across two HS pages before the scalar trigger-load probe",
        excpt.triggered == false
    );

    goto_priv(PRIV_M);
    TEST_ASSERT("after the vector cross-page store, the boundary bytes match the staged payload before the scalar trigger-load probe",
        ai_micro_cross_page_trigger_bytes_match(low_phys, source_pattern, 8U) &&
        ai_micro_cross_page_trigger_bytes_match(high_phys, source_pattern + 8U, 8U)
    );
    if (check_adjacent) {
        TEST_ASSERT("after the vector cross-page store, the adjacent sentinels stay unchanged before the scalar trigger-load probe",
            read8(sentinel_before_addr) == sentinel_before &&
            read8(sentinel_after_addr) == sentinel_after
        );
    }

    goto_priv(PRIV_HS);
    hspt_init();
    sfence_vma();

    ai_micro_cross_page_clear_mcontrol6_slot(0);
    ai_micro_cross_page_clear_mcontrol6_slot(1);
    ai_micro_cross_page_arm_mcontrol6_slot(0, trigger_pc,
        AI_MICRO_CROSS_PAGE_MCONTROL6_EXECUTE);
    ai_micro_cross_page_arm_mcontrol6_slot(1, scalar_vaddr,
        AI_MICRO_CROSS_PAGE_MCONTROL6_LOAD);
    saved_mstatus = ai_micro_cross_page_enable_machine_breakpoint_delivery();

    goto_priv(PRIV_HS);
    TEST_SETUP_EXCEPT();
    ai_micro_cross_page_execute_load_once(scalar_vaddr);
    TEST_ASSERT("after a vector cross-page store touches the shared path, the first scalar dual-trigger split-load probe still reports breakpoint",
        excpt.triggered == true &&
        excpt.cause == CAUSE_BKP
    );

    goto_priv(PRIV_M);
    TEST_ASSERT("the first trapped scalar cross-page trigger-load probe still leaves the vector-written boundary bytes unchanged",
        ai_micro_cross_page_trigger_bytes_match(low_phys, source_pattern, 8U) &&
        ai_micro_cross_page_trigger_bytes_match(high_phys, source_pattern + 8U, 8U)
    );
    if (check_adjacent) {
        TEST_ASSERT("the first trapped scalar cross-page trigger-load probe still leaves the adjacent sentinels unchanged",
            read8(sentinel_before_addr) == sentinel_before &&
            read8(sentinel_after_addr) == sentinel_after
        );
    }
    ai_micro_cross_page_restore_machine_status(saved_mstatus);

    saved_mstatus = ai_micro_cross_page_enable_machine_breakpoint_delivery();
    goto_priv(PRIV_HS);
    TEST_SETUP_EXCEPT();
    ai_micro_cross_page_execute_load_once(scalar_vaddr);
    TEST_ASSERT("after the earlier vector cross-page store, the second scalar dual-trigger split-load probe still reports breakpoint",
        excpt.triggered == true &&
        excpt.cause == CAUSE_BKP
    );

    goto_priv(PRIV_M);
    TEST_ASSERT("the second trapped scalar cross-page trigger-load probe still leaves the vector-written boundary bytes unchanged",
        ai_micro_cross_page_trigger_bytes_match(low_phys, source_pattern, 8U) &&
        ai_micro_cross_page_trigger_bytes_match(high_phys, source_pattern + 8U, 8U)
    );
    if (check_adjacent) {
        TEST_ASSERT("the second trapped scalar cross-page trigger-load probe still leaves the adjacent sentinels unchanged",
            read8(sentinel_before_addr) == sentinel_before &&
            read8(sentinel_after_addr) == sentinel_after
        );
    }
    ai_micro_cross_page_restore_machine_status(saved_mstatus);

    ai_micro_cross_page_clear_mcontrol6_slot(0);
    saved_mstatus = ai_micro_cross_page_enable_machine_breakpoint_delivery();
    goto_priv(PRIV_HS);
    TEST_SETUP_EXCEPT();
    ai_micro_cross_page_execute_load_once(scalar_vaddr);
    TEST_ASSERT("after two prior breakpoint hits and an earlier vector cross-page store, clearing only the execute-PC slot still leaves the scalar cross-page load address trigger active",
        excpt.triggered == true &&
        excpt.cause == CAUSE_BKP
    );

    goto_priv(PRIV_M);
    TEST_ASSERT("with only the address trigger slot armed, the scalar cross-page load probe still leaves the vector-written boundary bytes unchanged",
        ai_micro_cross_page_trigger_bytes_match(low_phys, source_pattern, 8U) &&
        ai_micro_cross_page_trigger_bytes_match(high_phys, source_pattern + 8U, 8U)
    );
    if (check_adjacent) {
        TEST_ASSERT("with only the address trigger slot armed, the scalar cross-page load probe still leaves the adjacent sentinels unchanged",
            read8(sentinel_before_addr) == sentinel_before &&
            read8(sentinel_after_addr) == sentinel_after
        );
    }
    ai_micro_cross_page_restore_machine_status(saved_mstatus);

    ai_micro_cross_page_clear_mcontrol6_slot(1);
    goto_priv(PRIV_HS);
    TEST_SETUP_EXCEPT();
    value = ai_micro_cross_page_execute_load_once(scalar_vaddr);
    TEST_ASSERT("after clearing both trigger slots, the scalar cross-page load still completes normally and returns the payload written earlier by the vector cross-page store",
        excpt.triggered == false &&
        value == 0x2b2a292827262524ULL
    );

    goto_priv(PRIV_M);
    TEST_ASSERT("after the vector-store then scalar-load cross-page trigger sequence, the shared boundary bytes still match the vector-written payload",
        ai_micro_cross_page_trigger_bytes_match(low_phys, source_pattern, 8U) &&
        ai_micro_cross_page_trigger_bytes_match(high_phys, source_pattern + 8U, 8U)
    );
    if (check_adjacent) {
        TEST_ASSERT("after the vector-store then scalar-load cross-page trigger sequence, the adjacent sentinels still match their expected values",
            read8(sentinel_before_addr) == sentinel_before &&
            read8(sentinel_after_addr) == sentinel_after
        );
    }

    goto_priv(PRIV_HS);
    hspt_init();
    sfence_vma();

    return test_status;
}

static bool ai_micro_cross_page_scalar_load_after_vector_store_execute_and_address_trigger_repeated_breakpoint_then_fault_recovers(
    bool use_access_fault,
    bool check_adjacent)
{
    enum test_page low_page = VSRW_GRW;
    enum test_page high_page = VSRW_GRX;
    const uint8_t source_pattern[16] = {
        0x20, 0x21, 0x22, 0x23,
        0x24, 0x25, 0x26, 0x27,
        0x28, 0x29, 0x2a, 0x2b,
        0x2c, 0x2d, 0x2e, 0x2f
    };
    const uint64_t expected_value = 0x2b2a292827262524ULL;
    uintptr_t source_vaddr = hs_page_base(VSRWX_GRWX) + 0x340U;
    uintptr_t source_phys = phys_page_base(VSRWX_GRWX) + 0x340U;
    uintptr_t vector_vaddr = ai_micro_cross_page_trigger_addr(low_page, 8U);
    uintptr_t scalar_vaddr = ai_micro_cross_page_trigger_addr(low_page, 4U);
    uintptr_t second_half_vaddr = scalar_vaddr + 4U;
    uintptr_t low_phys = ai_micro_cross_page_trigger_page_end_phys(low_page);
    uintptr_t high_phys = phys_page_base(high_page);
    uintptr_t sentinel_before_addr = low_phys - 1U;
    uintptr_t sentinel_after_addr = high_phys + 8U;
    uintptr_t trigger_pc = (uintptr_t)ai_micro_cross_page_execute_load_once;
    uintptr_t deny_start = high_phys;
    uintptr_t deny_end = deny_start + PAGE_SIZE;
    const uint8_t sentinel_before = use_access_fault ? 0x3cU : 0x2cU;
    const uint8_t sentinel_after = use_access_fault ? 0xd2U : 0xc2U;
    const uint64_t expected_cause = use_access_fault ? CAUSE_LAF : CAUSE_LPF;
    uint64_t saved_mstatus = 0;
    uint64_t value = 0;
    bool test_status = true;

    goto_priv(PRIV_M);
    ai_micro_cross_page_trigger_write_bytes(source_phys, source_pattern, 16U);
    write8(sentinel_before_addr, sentinel_before);
    write8(sentinel_after_addr, sentinel_after);

    goto_priv(PRIV_HS);
    hspt_init();
    sfence_vma();

    TEST_SETUP_EXCEPT();
    vle8_to_v6((const uint8_t *)source_vaddr, 16);
    TEST_ASSERT("a vector unit-stride load can stage a 16-byte payload before the mixed vector-store then scalar cross-page repeated-breakpoint fault-recovery load probe starts",
        excpt.triggered == false
    );

    TEST_SETUP_EXCEPT();
    vse8_from_v6((uint8_t *)vector_vaddr, 16);
    TEST_ASSERT("a vector unit-stride store can write the staged payload across the same HS page pair before the scalar repeated-breakpoint fault-recovery load probe",
        excpt.triggered == false
    );

    goto_priv(PRIV_M);
    TEST_ASSERT("after the vector cross-page store, the shared boundary bytes match the staged payload before the scalar repeated-breakpoint fault-recovery load probe",
        ai_micro_cross_page_trigger_bytes_match(low_phys, source_pattern, 8U) &&
        ai_micro_cross_page_trigger_bytes_match(high_phys, source_pattern + 8U, 8U)
    );
    if (check_adjacent) {
        TEST_ASSERT("after the vector cross-page store, the adjacent sentinels stay unchanged before the scalar repeated-breakpoint fault-recovery load probe",
            read8(sentinel_before_addr) == sentinel_before &&
            read8(sentinel_after_addr) == sentinel_after
        );
    }

    if (use_access_fault) {
        ai_micro_cross_page_configure_pmp_window(deny_start, deny_end, AI_MICRO_PMP_TOR_NO_R);
    } else {
        *ai_micro_cross_page_hs_leaf_entry(high_page) = 0;
        sfence_vma();
    }

    ai_micro_cross_page_clear_mcontrol6_slot(0);
    ai_micro_cross_page_clear_mcontrol6_slot(1);
    ai_micro_cross_page_arm_mcontrol6_slot(0, trigger_pc,
        AI_MICRO_CROSS_PAGE_MCONTROL6_EXECUTE);
    ai_micro_cross_page_arm_mcontrol6_slot(1, scalar_vaddr,
        AI_MICRO_CROSS_PAGE_MCONTROL6_LOAD);
    saved_mstatus = ai_micro_cross_page_enable_machine_breakpoint_delivery();

    goto_priv(PRIV_HS);
    TEST_SETUP_EXCEPT();
    ai_micro_cross_page_execute_load_once(scalar_vaddr);
    TEST_ASSERT("after a vector cross-page store touches the shared path, the first scalar dual-trigger split-load probe still reports breakpoint before the later fault-recovery path",
        excpt.triggered == true &&
        excpt.cause == CAUSE_BKP
    );

    goto_priv(PRIV_M);
    TEST_ASSERT("the first trapped scalar cross-page trigger-load probe still leaves the vector-written boundary bytes unchanged before the later fault-recovery path",
        ai_micro_cross_page_trigger_bytes_match(low_phys, source_pattern, 8U) &&
        ai_micro_cross_page_trigger_bytes_match(high_phys, source_pattern + 8U, 8U)
    );
    if (check_adjacent) {
        TEST_ASSERT("the first trapped scalar cross-page trigger-load probe still leaves the adjacent sentinels unchanged before the later fault-recovery path",
            read8(sentinel_before_addr) == sentinel_before &&
            read8(sentinel_after_addr) == sentinel_after
        );
    }
    ai_micro_cross_page_restore_machine_status(saved_mstatus);

    saved_mstatus = ai_micro_cross_page_enable_machine_breakpoint_delivery();
    goto_priv(PRIV_HS);
    TEST_SETUP_EXCEPT();
    ai_micro_cross_page_execute_load_once(scalar_vaddr);
    TEST_ASSERT("after the earlier vector cross-page store, the second scalar dual-trigger split-load probe still reports breakpoint before the later fault-recovery path",
        excpt.triggered == true &&
        excpt.cause == CAUSE_BKP
    );

    goto_priv(PRIV_M);
    TEST_ASSERT("the second trapped scalar cross-page trigger-load probe still leaves the vector-written boundary bytes unchanged before the later fault-recovery path",
        ai_micro_cross_page_trigger_bytes_match(low_phys, source_pattern, 8U) &&
        ai_micro_cross_page_trigger_bytes_match(high_phys, source_pattern + 8U, 8U)
    );
    if (check_adjacent) {
        TEST_ASSERT("the second trapped scalar cross-page trigger-load probe still leaves the adjacent sentinels unchanged before the later fault-recovery path",
            read8(sentinel_before_addr) == sentinel_before &&
            read8(sentinel_after_addr) == sentinel_after
        );
    }
    ai_micro_cross_page_restore_machine_status(saved_mstatus);

    ai_micro_cross_page_clear_mcontrol6_slot(0);
    saved_mstatus = ai_micro_cross_page_enable_machine_breakpoint_delivery();
    goto_priv(PRIV_HS);
    TEST_SETUP_EXCEPT();
    ai_micro_cross_page_execute_load_once(scalar_vaddr);
    TEST_ASSERT("after two prior breakpoint hits and an earlier vector cross-page store, clearing only the execute-PC slot still leaves the scalar cross-page load address trigger active before the later fault-recovery path",
        excpt.triggered == true &&
        excpt.cause == CAUSE_BKP
    );

    goto_priv(PRIV_M);
    TEST_ASSERT("with only the address trigger slot armed, the scalar cross-page load probe still leaves the vector-written boundary bytes unchanged before the later fault-recovery path",
        ai_micro_cross_page_trigger_bytes_match(low_phys, source_pattern, 8U) &&
        ai_micro_cross_page_trigger_bytes_match(high_phys, source_pattern + 8U, 8U)
    );
    if (check_adjacent) {
        TEST_ASSERT("with only the address trigger slot armed, the scalar cross-page load probe still leaves the adjacent sentinels unchanged before the later fault-recovery path",
            read8(sentinel_before_addr) == sentinel_before &&
            read8(sentinel_after_addr) == sentinel_after
        );
    }
    ai_micro_cross_page_restore_machine_status(saved_mstatus);

    ai_micro_cross_page_clear_mcontrol6_slot(1);
    goto_priv(PRIV_HS);
    TEST_SETUP_EXCEPT();
    ai_micro_cross_page_execute_load_once(scalar_vaddr);
    TEST_ASSERT((use_access_fault ?
        "after clearing both trigger slots, the scalar cross-page load first falls back to high-half load access fault with the true second-half start address before PMP permission is restored" :
        "after clearing both trigger slots, the scalar cross-page load first falls back to high-half load page fault with the true second-half start address before the upper leaf is restored"),
        excpt.triggered == true &&
        excpt.cause == expected_cause &&
        excpt.tval == second_half_vaddr
    );

    goto_priv(PRIV_M);
    TEST_ASSERT((use_access_fault ?
        "the fallback cross-page load access-fault path still leaves the vector-written boundary bytes unchanged" :
        "the fallback cross-page load page-fault path still leaves the vector-written boundary bytes unchanged"),
        ai_micro_cross_page_trigger_bytes_match(low_phys, source_pattern, 8U) &&
        ai_micro_cross_page_trigger_bytes_match(high_phys, source_pattern + 8U, 8U)
    );
    if (check_adjacent) {
        TEST_ASSERT((use_access_fault ?
            "the fallback cross-page load access-fault path still leaves the adjacent sentinels unchanged" :
            "the fallback cross-page load page-fault path still leaves the adjacent sentinels unchanged"),
            read8(sentinel_before_addr) == sentinel_before &&
            read8(sentinel_after_addr) == sentinel_after
        );
    }

    goto_priv(PRIV_M);
    if (use_access_fault) {
        ai_micro_cross_page_clear_pmp_window();
    }

    goto_priv(PRIV_HS);
    hspt_init();
    sfence_vma();

    TEST_SETUP_EXCEPT();
    value = ai_micro_cross_page_execute_load_once(scalar_vaddr);
    TEST_ASSERT((use_access_fault ?
        "after restoring PMP read permission, the mixed vector-store then scalar-load path completes normally again and returns the vector-written payload" :
        "after restoring the upper leaf, the mixed vector-store then scalar-load path completes normally again and returns the vector-written payload"),
        excpt.triggered == false &&
        value == expected_value
    );

    goto_priv(PRIV_M);
    TEST_ASSERT((use_access_fault ?
        "after the repeated-breakpoint plus access-fault recovery load sequence in the vector-store then scalar-load corner, the shared boundary bytes still match the vector-written payload" :
        "after the repeated-breakpoint plus page-fault recovery load sequence in the vector-store then scalar-load corner, the shared boundary bytes still match the vector-written payload"),
        ai_micro_cross_page_trigger_bytes_match(low_phys, source_pattern, 8U) &&
        ai_micro_cross_page_trigger_bytes_match(high_phys, source_pattern + 8U, 8U)
    );
    if (check_adjacent) {
        TEST_ASSERT((use_access_fault ?
            "after the repeated-breakpoint plus access-fault recovery load sequence in the vector-store then scalar-load corner, the adjacent sentinels still match their expected values" :
            "after the repeated-breakpoint plus page-fault recovery load sequence in the vector-store then scalar-load corner, the adjacent sentinels still match their expected values"),
            read8(sentinel_before_addr) == sentinel_before &&
            read8(sentinel_after_addr) == sentinel_after
        );
    }

    goto_priv(PRIV_HS);
    hspt_init();
    sfence_vma();

    return test_status;
}

static bool ai_micro_cross_page_scalar_store_after_vector_load_execute_and_address_trigger_repeated_breakpoint_then_completion(
    bool check_adjacent)
{
    enum test_page low_page = VSRW_GRW;
    enum test_page high_page = VSRW_GRX;
    const uint8_t pattern[16] = {
        0x40, 0x41, 0x42, 0x43,
        0x44, 0x45, 0x46, 0x47,
        0x48, 0x49, 0x4a, 0x4b,
        0x4c, 0x4d, 0x4e, 0x4f
    };
    const uint8_t expected_final[16] = {
        0x40, 0x41, 0x42, 0x43,
        0xef, 0xcd, 0xab, 0x89,
        0x67, 0x45, 0x23, 0x01,
        0x4c, 0x4d, 0x4e, 0x4f
    };
    uintptr_t vector_vaddr = ai_micro_cross_page_trigger_addr(low_page, 8U);
    uintptr_t scalar_vaddr = ai_micro_cross_page_trigger_addr(low_page, 4U);
    uintptr_t low_phys = ai_micro_cross_page_trigger_page_end_phys(low_page);
    uintptr_t high_phys = phys_page_base(high_page);
    uintptr_t scratch_vaddr = hs_page_base(VSRWX_GRWX) + 0x80U;
    uintptr_t scratch_phys = phys_page_base(VSRWX_GRWX) + 0x80U;
    uintptr_t sentinel_before_addr = low_phys - 1U;
    uintptr_t sentinel_after_addr = high_phys + 8U;
    uintptr_t trigger_pc = (uintptr_t)ai_micro_cross_page_execute_store_once;
    const uint8_t sentinel_before = 0x4cU;
    const uint8_t sentinel_after = 0xd4U;
    const uint8_t scratch_before = 0x6bU;
    const uint8_t scratch_after = 0xb6U;
    uint64_t saved_mstatus = 0;
    uint64_t value = 0;
    bool test_status = true;

    goto_priv(PRIV_M);
    ai_micro_cross_page_trigger_write_bytes(low_phys, pattern, 8U);
    ai_micro_cross_page_trigger_write_bytes(high_phys, pattern + 8U, 8U);
    write8(sentinel_before_addr, sentinel_before);
    write8(sentinel_after_addr, sentinel_after);
    write8(scratch_phys - 1U, scratch_before);
    write8(scratch_phys + 16U, scratch_after);

    goto_priv(PRIV_HS);
    hspt_init();
    sfence_vma();

    TEST_SETUP_EXCEPT();
    vle8_to_v6((const uint8_t *)vector_vaddr, 16);
    TEST_ASSERT("a vector unit-stride load can pull a 16-byte payload across the same HS page pair before the mixed vector-load then scalar cross-page trigger-store probe",
        excpt.triggered == false
    );

    TEST_SETUP_EXCEPT();
    vse8_from_v6((uint8_t *)scratch_vaddr, 16);
    TEST_ASSERT("storing the vector cross-page payload into a scratch page preserves the original 16-byte byte sequence before the scalar trigger-store probe",
        excpt.triggered == false
    );

    goto_priv(PRIV_M);
    TEST_ASSERT("the scratch copy after the vector cross-page load still matches the exact boundary payload before the scalar trigger-store probe",
        ai_micro_cross_page_trigger_bytes_match(scratch_phys, pattern, 16U) &&
        read8(scratch_phys - 1U) == scratch_before &&
        read8(scratch_phys + 16U) == scratch_after
    );
    if (check_adjacent) {
        TEST_ASSERT("after the vector cross-page load, the shared boundary bytes and adjacent sentinels stay unchanged before the scalar trigger-store probe",
            ai_micro_cross_page_trigger_bytes_match(low_phys, pattern, 8U) &&
            ai_micro_cross_page_trigger_bytes_match(high_phys, pattern + 8U, 8U) &&
            read8(sentinel_before_addr) == sentinel_before &&
            read8(sentinel_after_addr) == sentinel_after
        );
    }

    goto_priv(PRIV_HS);
    hspt_init();
    sfence_vma();

    ai_micro_cross_page_clear_mcontrol6_slot(0);
    ai_micro_cross_page_clear_mcontrol6_slot(1);
    ai_micro_cross_page_arm_mcontrol6_slot(0, trigger_pc,
        AI_MICRO_CROSS_PAGE_MCONTROL6_EXECUTE);
    ai_micro_cross_page_arm_mcontrol6_slot(1, scalar_vaddr,
        AI_MICRO_CROSS_PAGE_MCONTROL6_STORE);
    saved_mstatus = ai_micro_cross_page_enable_machine_breakpoint_delivery();

    goto_priv(PRIV_HS);
    TEST_SETUP_EXCEPT();
    ai_micro_cross_page_execute_store_once(scalar_vaddr, 0x0123456789abcdefULL);
    TEST_ASSERT("after a vector cross-page load touches the shared path, the first scalar dual-trigger split-store probe still reports breakpoint",
        excpt.triggered == true &&
        excpt.cause == CAUSE_BKP
    );

    goto_priv(PRIV_M);
    TEST_ASSERT("the first trapped scalar cross-page trigger-store probe still leaves the shared boundary bytes unchanged after the prior vector load",
        ai_micro_cross_page_trigger_bytes_match(low_phys, pattern, 8U) &&
        ai_micro_cross_page_trigger_bytes_match(high_phys, pattern + 8U, 8U)
    );
    if (check_adjacent) {
        TEST_ASSERT("the first trapped scalar cross-page trigger-store probe still leaves the adjacent sentinels unchanged",
            read8(sentinel_before_addr) == sentinel_before &&
            read8(sentinel_after_addr) == sentinel_after
        );
    }
    ai_micro_cross_page_restore_machine_status(saved_mstatus);

    saved_mstatus = ai_micro_cross_page_enable_machine_breakpoint_delivery();
    goto_priv(PRIV_HS);
    TEST_SETUP_EXCEPT();
    ai_micro_cross_page_execute_store_once(scalar_vaddr, 0x0123456789abcdefULL);
    TEST_ASSERT("after the earlier vector cross-page load, the second scalar dual-trigger split-store probe still reports breakpoint",
        excpt.triggered == true &&
        excpt.cause == CAUSE_BKP
    );

    goto_priv(PRIV_M);
    TEST_ASSERT("the second trapped scalar cross-page trigger-store probe still leaves the shared boundary bytes unchanged after the prior vector load",
        ai_micro_cross_page_trigger_bytes_match(low_phys, pattern, 8U) &&
        ai_micro_cross_page_trigger_bytes_match(high_phys, pattern + 8U, 8U)
    );
    if (check_adjacent) {
        TEST_ASSERT("the second trapped scalar cross-page trigger-store probe still leaves the adjacent sentinels unchanged",
            read8(sentinel_before_addr) == sentinel_before &&
            read8(sentinel_after_addr) == sentinel_after
        );
    }
    ai_micro_cross_page_restore_machine_status(saved_mstatus);

    ai_micro_cross_page_clear_mcontrol6_slot(0);
    saved_mstatus = ai_micro_cross_page_enable_machine_breakpoint_delivery();
    goto_priv(PRIV_HS);
    TEST_SETUP_EXCEPT();
    ai_micro_cross_page_execute_store_once(scalar_vaddr, 0x0123456789abcdefULL);
    TEST_ASSERT("after two prior breakpoint hits and an earlier vector cross-page load, clearing only the execute-PC slot still leaves the scalar cross-page store address trigger active",
        excpt.triggered == true &&
        excpt.cause == CAUSE_BKP
    );

    goto_priv(PRIV_M);
    TEST_ASSERT("with only the address trigger slot armed, the scalar cross-page store probe still leaves the shared boundary bytes unchanged after the prior vector load",
        ai_micro_cross_page_trigger_bytes_match(low_phys, pattern, 8U) &&
        ai_micro_cross_page_trigger_bytes_match(high_phys, pattern + 8U, 8U)
    );
    if (check_adjacent) {
        TEST_ASSERT("with only the address trigger slot armed, the scalar cross-page store probe still leaves the adjacent sentinels unchanged",
            read8(sentinel_before_addr) == sentinel_before &&
            read8(sentinel_after_addr) == sentinel_after
        );
    }
    ai_micro_cross_page_restore_machine_status(saved_mstatus);

    ai_micro_cross_page_clear_mcontrol6_slot(1);
    goto_priv(PRIV_HS);
    TEST_SETUP_EXCEPT();
    ai_micro_cross_page_execute_store_once(scalar_vaddr, 0x0123456789abcdefULL);
    TEST_ASSERT("after clearing both trigger slots, the scalar cross-page store still commits normally even though a vector cross-page load touched the shared path earlier",
        excpt.triggered == false
    );

    goto_priv(PRIV_M);
    TEST_ASSERT("after the vector-load then scalar-store cross-page trigger sequence, the final boundary bytes reflect the expected overwrite order",
        ai_micro_cross_page_trigger_bytes_match(low_phys, expected_final, 8U) &&
        ai_micro_cross_page_trigger_bytes_match(high_phys, expected_final + 8U, 8U)
    );
    if (check_adjacent) {
        TEST_ASSERT("after the vector-load then scalar-store cross-page trigger sequence, the shared boundary sentinels and scratch sentinels stay unchanged",
            read8(sentinel_before_addr) == sentinel_before &&
            read8(sentinel_after_addr) == sentinel_after &&
            read8(scratch_phys - 1U) == scratch_before &&
            read8(scratch_phys + 16U) == scratch_after
        );
    }

    goto_priv(PRIV_HS);
    TEST_SETUP_EXCEPT();
    value = ld(scalar_vaddr);
    TEST_ASSERT("after the vector-load then scalar-store cross-page trigger sequence, a following scalar split-load still sees the latest scalar store payload immediately",
        excpt.triggered == false &&
        value == 0x0123456789abcdefULL
    );

    goto_priv(PRIV_HS);
    hspt_init();
    sfence_vma();

    return test_status;
}

static bool ai_micro_cross_page_scalar_store_after_vector_load_execute_and_address_trigger_repeated_breakpoint_then_fault_recovers(
    bool use_access_fault,
    bool check_adjacent)
{
    enum test_page low_page = VSRW_GRW;
    enum test_page high_page = VSRW_GRX;
    const uint8_t pattern[16] = {
        0x40, 0x41, 0x42, 0x43,
        0x44, 0x45, 0x46, 0x47,
        0x48, 0x49, 0x4a, 0x4b,
        0x4c, 0x4d, 0x4e, 0x4f
    };
    const uint8_t expected_final[16] = {
        0x40, 0x41, 0x42, 0x43,
        0xef, 0xcd, 0xab, 0x89,
        0x67, 0x45, 0x23, 0x01,
        0x4c, 0x4d, 0x4e, 0x4f
    };
    const uint64_t store_value = 0x0123456789abcdefULL;
    const uint64_t expected_low_after_fault = 0x4746454443424140ULL;
    uintptr_t vector_vaddr = ai_micro_cross_page_trigger_addr(low_page, 8U);
    uintptr_t scalar_vaddr = ai_micro_cross_page_trigger_addr(low_page, 4U);
    uintptr_t high_half_vaddr = scalar_vaddr + 4U;
    uintptr_t low_phys = ai_micro_cross_page_trigger_page_end_phys(low_page);
    uintptr_t high_phys = phys_page_base(high_page);
    uintptr_t scratch_vaddr = hs_page_base(VSRWX_GRWX) + 0x80U;
    uintptr_t scratch_phys = phys_page_base(VSRWX_GRWX) + 0x80U;
    uintptr_t sentinel_before_addr = low_phys - 1U;
    uintptr_t sentinel_after_addr = high_phys + 8U;
    uintptr_t trigger_pc = (uintptr_t)ai_micro_cross_page_execute_store_once;
    uintptr_t deny_start = high_phys;
    uintptr_t deny_end = deny_start + PAGE_SIZE;
    const uint8_t sentinel_before = use_access_fault ? 0x5cU : 0x4cU;
    const uint8_t sentinel_after = use_access_fault ? 0xe4U : 0xd4U;
    const uint8_t scratch_before = use_access_fault ? 0x7bU : 0x6bU;
    const uint8_t scratch_after = use_access_fault ? 0xc6U : 0xb6U;
    const uint64_t expected_cause = use_access_fault ? CAUSE_SAF : CAUSE_SPF;
    uint64_t saved_mstatus = 0;
    uint64_t value = 0;
    bool test_status = true;

    goto_priv(PRIV_M);
    ai_micro_cross_page_trigger_write_bytes(low_phys, pattern, 8U);
    ai_micro_cross_page_trigger_write_bytes(high_phys, pattern + 8U, 8U);
    write8(sentinel_before_addr, sentinel_before);
    write8(sentinel_after_addr, sentinel_after);
    write8(scratch_phys - 1U, scratch_before);
    write8(scratch_phys + 16U, scratch_after);

    goto_priv(PRIV_HS);
    hspt_init();
    sfence_vma();

    TEST_SETUP_EXCEPT();
    vle8_to_v6((const uint8_t *)vector_vaddr, 16);
    TEST_ASSERT("a vector unit-stride load can pull a 16-byte payload across the same HS page pair before the mixed vector-load then scalar repeated-breakpoint fault-recovery store probe starts",
        excpt.triggered == false
    );

    TEST_SETUP_EXCEPT();
    vse8_from_v6((uint8_t *)scratch_vaddr, 16);
    TEST_ASSERT("storing the vector cross-page payload into a scratch page preserves the original 16-byte byte sequence before the scalar repeated-breakpoint fault-recovery store probe",
        excpt.triggered == false
    );

    goto_priv(PRIV_M);
    TEST_ASSERT("the scratch copy after the vector cross-page load still matches the exact boundary payload before the scalar repeated-breakpoint fault-recovery store probe",
        ai_micro_cross_page_trigger_bytes_match(scratch_phys, pattern, 16U) &&
        read8(scratch_phys - 1U) == scratch_before &&
        read8(scratch_phys + 16U) == scratch_after
    );
    if (check_adjacent) {
        TEST_ASSERT("after the vector cross-page load, the shared boundary bytes and adjacent sentinels stay unchanged before the scalar repeated-breakpoint fault-recovery store probe",
            ai_micro_cross_page_trigger_bytes_match(low_phys, pattern, 8U) &&
            ai_micro_cross_page_trigger_bytes_match(high_phys, pattern + 8U, 8U) &&
            read8(sentinel_before_addr) == sentinel_before &&
            read8(sentinel_after_addr) == sentinel_after
        );
    }

    goto_priv(PRIV_M);
    if (use_access_fault) {
        ai_micro_cross_page_configure_pmp_window(deny_start, deny_end, AI_MICRO_PMP_TOR_NO_W);
    } else {
        *ai_micro_cross_page_hs_leaf_entry(high_page) = 0;
        sfence_vma();
    }

    ai_micro_cross_page_clear_mcontrol6_slot(0);
    ai_micro_cross_page_clear_mcontrol6_slot(1);
    ai_micro_cross_page_arm_mcontrol6_slot(0, trigger_pc,
        AI_MICRO_CROSS_PAGE_MCONTROL6_EXECUTE);
    ai_micro_cross_page_arm_mcontrol6_slot(1, scalar_vaddr,
        AI_MICRO_CROSS_PAGE_MCONTROL6_STORE);
    saved_mstatus = ai_micro_cross_page_enable_machine_breakpoint_delivery();

    goto_priv(PRIV_HS);
    TEST_SETUP_EXCEPT();
    ai_micro_cross_page_execute_store_once(scalar_vaddr, store_value);
    TEST_ASSERT("after a vector cross-page load touches the shared path, the first scalar dual-trigger split-store probe still reports breakpoint before the later fault-recovery path",
        excpt.triggered == true &&
        excpt.cause == CAUSE_BKP
    );

    goto_priv(PRIV_M);
    TEST_ASSERT("the first trapped scalar cross-page trigger-store probe still leaves the shared boundary bytes unchanged before the later fault-recovery path",
        ai_micro_cross_page_trigger_bytes_match(low_phys, pattern, 8U) &&
        ai_micro_cross_page_trigger_bytes_match(high_phys, pattern + 8U, 8U)
    );
    if (check_adjacent) {
        TEST_ASSERT("the first trapped scalar cross-page trigger-store probe still leaves the shared boundary sentinels and scratch sentinels unchanged before the later fault-recovery path",
            read8(sentinel_before_addr) == sentinel_before &&
            read8(sentinel_after_addr) == sentinel_after &&
            read8(scratch_phys - 1U) == scratch_before &&
            read8(scratch_phys + 16U) == scratch_after
        );
    }
    ai_micro_cross_page_restore_machine_status(saved_mstatus);

    saved_mstatus = ai_micro_cross_page_enable_machine_breakpoint_delivery();
    goto_priv(PRIV_HS);
    TEST_SETUP_EXCEPT();
    ai_micro_cross_page_execute_store_once(scalar_vaddr, store_value);
    TEST_ASSERT("after the earlier vector cross-page load, the second scalar dual-trigger split-store probe still reports breakpoint before the later fault-recovery path",
        excpt.triggered == true &&
        excpt.cause == CAUSE_BKP
    );

    goto_priv(PRIV_M);
    TEST_ASSERT("the second trapped scalar cross-page trigger-store probe still leaves the shared boundary bytes unchanged before the later fault-recovery path",
        ai_micro_cross_page_trigger_bytes_match(low_phys, pattern, 8U) &&
        ai_micro_cross_page_trigger_bytes_match(high_phys, pattern + 8U, 8U)
    );
    if (check_adjacent) {
        TEST_ASSERT("the second trapped scalar cross-page trigger-store probe still leaves the shared boundary sentinels and scratch sentinels unchanged before the later fault-recovery path",
            read8(sentinel_before_addr) == sentinel_before &&
            read8(sentinel_after_addr) == sentinel_after &&
            read8(scratch_phys - 1U) == scratch_before &&
            read8(scratch_phys + 16U) == scratch_after
        );
    }
    ai_micro_cross_page_restore_machine_status(saved_mstatus);

    ai_micro_cross_page_clear_mcontrol6_slot(0);
    saved_mstatus = ai_micro_cross_page_enable_machine_breakpoint_delivery();
    goto_priv(PRIV_HS);
    TEST_SETUP_EXCEPT();
    ai_micro_cross_page_execute_store_once(scalar_vaddr, store_value);
    TEST_ASSERT("after two prior breakpoint hits and an earlier vector cross-page load, clearing only the execute-PC slot still leaves the scalar cross-page store address trigger active before the later fault-recovery path",
        excpt.triggered == true &&
        excpt.cause == CAUSE_BKP
    );

    goto_priv(PRIV_M);
    TEST_ASSERT("with only the address trigger slot armed, the scalar cross-page store probe still leaves the shared boundary bytes unchanged before the later fault-recovery path",
        ai_micro_cross_page_trigger_bytes_match(low_phys, pattern, 8U) &&
        ai_micro_cross_page_trigger_bytes_match(high_phys, pattern + 8U, 8U)
    );
    if (check_adjacent) {
        TEST_ASSERT("with only the address trigger slot armed, the scalar cross-page store probe still leaves the shared boundary sentinels and scratch sentinels unchanged before the later fault-recovery path",
            read8(sentinel_before_addr) == sentinel_before &&
            read8(sentinel_after_addr) == sentinel_after &&
            read8(scratch_phys - 1U) == scratch_before &&
            read8(scratch_phys + 16U) == scratch_after
        );
    }
    ai_micro_cross_page_restore_machine_status(saved_mstatus);

    ai_micro_cross_page_clear_mcontrol6_slot(1);
    goto_priv(PRIV_HS);
    TEST_SETUP_EXCEPT();
    ai_micro_cross_page_execute_store_once(scalar_vaddr, store_value);
    TEST_ASSERT((use_access_fault ?
        "after clearing both trigger slots, the scalar cross-page store first falls back to high-half store access fault with the true second-half start address before PMP permission is restored" :
        "after clearing both trigger slots, the scalar cross-page store first falls back to high-half store page fault with the true second-half start address before the upper leaf is restored"),
        excpt.triggered == true &&
        excpt.cause == expected_cause &&
        excpt.tval == high_half_vaddr
    );

    goto_priv(PRIV_M);
    TEST_ASSERT((use_access_fault ?
        "the fallback cross-page store access-fault path exposes no low-half store side effect while leaving the upper page bytes unchanged" :
        "the fallback cross-page store page-fault path exposes no low-half store side effect while leaving the upper page bytes unchanged"),
        read64(low_phys) == expected_low_after_fault &&
        ai_micro_cross_page_trigger_bytes_match(high_phys, pattern + 8U, 8U)
    );
    if (check_adjacent) {
        TEST_ASSERT((use_access_fault ?
            "the fallback cross-page store access-fault path still leaves the shared boundary sentinels and scratch sentinels unchanged" :
            "the fallback cross-page store page-fault path still leaves the shared boundary sentinels and scratch sentinels unchanged"),
            read8(sentinel_before_addr) == sentinel_before &&
            read8(sentinel_after_addr) == sentinel_after &&
            read8(scratch_phys - 1U) == scratch_before &&
            read8(scratch_phys + 16U) == scratch_after
        );
    }

    goto_priv(PRIV_M);
    if (use_access_fault) {
        ai_micro_cross_page_clear_pmp_window();
    }
    ai_micro_cross_page_trigger_write_bytes(low_phys, pattern, 8U);
    ai_micro_cross_page_trigger_write_bytes(high_phys, pattern + 8U, 8U);

    goto_priv(PRIV_HS);
    hspt_init();
    sfence_vma();

    TEST_SETUP_EXCEPT();
    ai_micro_cross_page_execute_store_once(scalar_vaddr, store_value);
    TEST_ASSERT((use_access_fault ?
        "after restoring PMP write permission, the mixed vector-load then scalar-store path commits normally again" :
        "after restoring the upper leaf, the mixed vector-load then scalar-store path commits normally again"),
        excpt.triggered == false
    );

    goto_priv(PRIV_M);
    TEST_ASSERT((use_access_fault ?
        "after the repeated-breakpoint plus access-fault recovery store sequence in the vector-load then scalar-store corner, the final boundary bytes reflect the expected overwrite order" :
        "after the repeated-breakpoint plus page-fault recovery store sequence in the vector-load then scalar-store corner, the final boundary bytes reflect the expected overwrite order"),
        ai_micro_cross_page_trigger_bytes_match(low_phys, expected_final, 8U) &&
        ai_micro_cross_page_trigger_bytes_match(high_phys, expected_final + 8U, 8U)
    );
    if (check_adjacent) {
        TEST_ASSERT((use_access_fault ?
            "after the repeated-breakpoint plus access-fault recovery store sequence in the vector-load then scalar-store corner, the shared boundary sentinels and scratch sentinels still match their expected values" :
            "after the repeated-breakpoint plus page-fault recovery store sequence in the vector-load then scalar-store corner, the shared boundary sentinels and scratch sentinels still match their expected values"),
            read8(sentinel_before_addr) == sentinel_before &&
            read8(sentinel_after_addr) == sentinel_after &&
            read8(scratch_phys - 1U) == scratch_before &&
            read8(scratch_phys + 16U) == scratch_after
        );
    }

    goto_priv(PRIV_HS);
    TEST_SETUP_EXCEPT();
    value = ld(scalar_vaddr);
    TEST_ASSERT((use_access_fault ?
        "after the repeated-breakpoint plus access-fault recovery store sequence in the vector-load then scalar-store corner, a following scalar split-load still sees the latest scalar store payload immediately" :
        "after the repeated-breakpoint plus page-fault recovery store sequence in the vector-load then scalar-store corner, a following scalar split-load still sees the latest scalar store payload immediately"),
        excpt.triggered == false &&
        value == store_value
    );

    goto_priv(PRIV_HS);
    hspt_init();
    sfence_vma();

    return test_status;
}

#endif /* AI_MICRO_CROSS_PAGE_TRIGGER_COMMON_H */
