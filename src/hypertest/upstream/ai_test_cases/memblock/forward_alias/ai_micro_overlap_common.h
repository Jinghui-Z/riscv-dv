#ifndef AI_MICRO_OVERLAP_COMMON_H
#define AI_MICRO_OVERLAP_COMMON_H

#include <rvh_test.h>
#include <page_tables.h>
#include <instructions.h>
#include <stdint.h>

/* Public case prototypes. Definitions live in the partNN sources. */
bool ai_micro_hs_store_then_load_same_vaddr_latest();
bool ai_micro_hs_store_alias_then_load_latest();
bool ai_micro_hs_store_then_load_same_vaddr_partial_merge_latest();
bool ai_micro_hs_same_vaddr_consecutive_partial_stores_merge_corner();
bool ai_micro_hs_same_vaddr_partial_store_merge_adjacent_preserved_corner();
bool ai_micro_hs_same_vaddr_full_then_partial_merge_latest_corner();
bool ai_micro_hs_same_vaddr_partial_then_full_last_writer_wins_corner();
bool ai_micro_hs_same_vaddr_partial_store_forward_narrow_load_sign_zero_extension_corner();
bool ai_micro_hs_same_vaddr_overlapping_partial_store_halfword_forward_byte_age_sign_zero_extension_corner();
bool ai_micro_hs_same_vaddr_overlapping_partial_store_word_forward_byte_age_sign_zero_extension_corner();
bool ai_micro_hs_same_vaddr_overlapping_partial_store_word_forward_youngest_byte_clears_sign_extension_corner();
bool ai_micro_hs_same_vaddr_partial_store_disjoint_narrow_load_no_false_forward_sign_zero_extension_corner();
bool ai_micro_hs_store_alias_partial_merge_latest();
bool ai_micro_hs_store_alias_partial_merge_adjacent_preserved_corner();
bool ai_micro_hs_store_alias_partial_then_full_last_writer_wins_corner();
bool ai_micro_hs_store_alias_full_then_partial_merge_adjacent_preserved_corner();
bool ai_micro_hs_store_triple_alias_then_load_latest();
bool ai_micro_hs_load_alias_pair_cold_consistent();
bool ai_micro_hs_ld_st_ld_ld_distinct_vaddr_store_visible();
bool ai_micro_hs_ldld_replay_with_nearby_wb_no_stuck_regression_corner();
bool ai_micro_hs_ldld_replay_fetch_flush_no_duplicate_writeback_corner();
bool ai_micro_hs_ldld_same_line_different_word_release_no_false_replay_corner();
bool ai_micro_hs_ldld_flushafter_following_store_survives_replay_corner();
bool ai_micro_hs_store_triple_alias_partial_merge_latest_corner();
bool ai_micro_hs_triple_alias_consecutive_stores_last_writer_wins_corner();
bool ai_micro_hs_triple_alias_multi_partial_store_merge_corner();
bool ai_micro_hs_triple_alias_multi_partial_store_merge_adjacent_preserved_corner();
bool ai_micro_hs_triple_alias_full_then_partial_merge_latest_corner();
bool ai_micro_hs_triple_alias_partial_then_full_last_writer_wins_corner();
bool ai_micro_hs_store_load_alias_partial_mask_matchinvalid_sqidx_correct_corner();
bool ai_micro_hs_store_load_alias_same_line_different_word_mask_no_wrong_forward_bytes_corner();
bool ai_micro_hs_store_load_alias_sq_wrap_no_wrong_forward_bytes_corner();
bool ai_micro_hs_sq_forward_one_cycle_late_result_matches_query_epoch_corner();
bool ai_micro_hs_sq_forward_younger_full_store_no_next_epoch_data_leak_corner();
bool ai_micro_hs_loadqueue_raw_late_revoke_no_free_of_reallocated_slot_corner();
bool ai_micro_hs_loadqueue_raw_flushed_store_no_stale_rollback_redirect_corner();
bool ai_micro_hs_stld_violation_store_killed_mid_select_does_not_flush_younger_load_corner();
bool ai_micro_hs_loadqueue_raw_same_rob_two_loads_oldest_uop_selected_corner();
bool ai_micro_hs_stld_violation_same_rob_diff_uopidx_redirect_target_correct_corner();
bool ai_micro_hs_load_vpmatchfail_and_ldld_same_cycle_rollback_priority_correct_corner();
bool ai_micro_hs_load_alias_and_ldld_combined_event_no_wrong_flush_scope_corner();
bool ai_micro_hs_raw_tracker_slot_reuse_old_revoke_must_not_kill_new_load_corner();
bool ai_micro_hs_loadqueue_raw_dual_store_violation_two_targets_no_cross_flush_corner();
bool ai_micro_hs_load_forward_valid_and_invalid_cause_same_epoch_no_cross_cycle_mixup_corner();
bool ai_micro_hs_load_replay_reports_real_blocking_sqidx_not_next_cycle_stale_index_corner();
bool ai_micro_hs_freelist_prealloc_free_then_realloc_ready_and_slot_stay_consistent_corner();
bool ai_micro_hs_raw_replay_queue_same_cycle_free_next_cycle_alloc_no_ghost_full_or_dup_slot_corner();
bool ai_micro_hs_load_replay_sq_ready_ptr_wrap_no_false_release_corner();
bool ai_micro_hs_load_replay_dense_store_boundary_no_stuck_corner();
bool ai_micro_hs_load_replay_escape_zero_threshold_reversed_alias_pair_no_stale_corner();
bool ai_micro_hs_replay_like_load_with_nearby_cross16_mab_wb_no_lost_completion_corner();
bool ai_micro_hs_replay_like_load_with_nearby_cross16_mab_wb_adjacent_preserved_corner();
bool ai_micro_hs_storequeue_older_sbuffer_handoff_then_younger_rewrite_not_dropped_corner();
bool ai_micro_hs_storequeue_older_sbuffer_handoff_then_younger_rewrite_adjacent_preserved_corner();
bool ai_micro_hs_databuffer_two_pair_turnover_no_swap_or_drop_corner();
bool ai_micro_hs_databuffer_two_pair_turnover_adjacent_preserved_corner();
bool ai_micro_hs_storequeue_cross16_pseudo_second_half_then_younger_aligned_batch_no_drop_corner();
bool ai_micro_hs_storequeue_cross16_pseudo_second_half_then_younger_aligned_batch_adjacent_preserved_corner();
bool ai_micro_hs_storequeue_cross16_high_half_then_younger_same_word_rewrite_keeps_youngest_data_corner();
bool ai_micro_hs_storequeue_cross16_high_half_then_younger_same_word_rewrite_adjacent_preserved_corner();
bool ai_micro_hs_storequeue_cross16_scalar_vector_wb_not_false_split_completion_corner();
bool ai_micro_hs_storequeue_cross16_scalar_vector_wb_not_false_split_completion_adjacent_preserved_corner();
bool ai_micro_hs_storequeue_cross16_scalar_vector_wb_same_boundary_next_split_keeps_latest_image_corner();
bool ai_micro_hs_storequeue_cross16_scalar_vector_wb_same_boundary_next_split_adjacent_preserved_corner();
bool ai_micro_hs_storequeue_cross16_scalar_vector_wb_next_boundary_no_cross_bind_corner();
bool ai_micro_hs_storequeue_cross16_scalar_vector_wb_next_boundary_adjacent_preserved_corner();
bool ai_micro_hs_storequeue_cross16_scalar_vector_wb_next_boundary_width_switch_no_stale_template_corner();
bool ai_micro_hs_storequeue_cross16_scalar_vector_wb_next_boundary_width_switch_adjacent_preserved_corner();
bool ai_micro_hs_storequeue_cross16_scalar_vector_wb_next_boundary_narrower_switch_merges_by_byte_age_corner();
bool ai_micro_hs_storequeue_cross16_scalar_vector_wb_next_boundary_narrower_switch_adjacent_preserved_corner();
bool ai_micro_hs_storequeue_cross16_scalar_vector_wb_same_boundary_three_shape_refresh_no_stale_older_generation_corner();
bool ai_micro_hs_storequeue_cross16_scalar_vector_wb_same_boundary_three_shape_adjacent_preserved_corner();
bool ai_micro_hs_storequeue_cross16_low_half_then_younger_same_word_rewrite_keeps_youngest_data_corner();
bool ai_micro_hs_storequeue_cross16_low_half_then_younger_same_word_rewrite_adjacent_preserved_corner();
bool ai_micro_hs_storequeue_consecutive_cross16_boundaries_no_cross_bind_or_drop_corner();
bool ai_micro_hs_storequeue_consecutive_cross16_boundaries_adjacent_preserved_corner();
bool ai_micro_hs_storequeue_consecutive_cross16_same_boundary_keeps_latest_split_image_corner();
bool ai_micro_hs_storequeue_consecutive_cross16_same_boundary_adjacent_preserved_corner();
bool ai_micro_hs_storequeue_consecutive_cross16_same_boundary_width_switch_no_stale_template_corner();
bool ai_micro_hs_storequeue_consecutive_cross16_same_boundary_width_switch_adjacent_preserved_corner();
bool ai_micro_hs_storequeue_consecutive_cross16_same_boundary_narrower_switch_merges_by_byte_age_corner();
bool ai_micro_hs_storequeue_consecutive_cross16_same_boundary_narrower_switch_adjacent_preserved_corner();
bool ai_micro_hs_storequeue_consecutive_cross16_same_boundary_three_shape_refresh_no_stale_older_generation_corner();
bool ai_micro_hs_storequeue_consecutive_cross16_same_boundary_three_shape_adjacent_preserved_corner();
bool ai_micro_hs_databuffer_aligned_pair_then_cross16_split_no_cross_bind_or_drop_corner();
bool ai_micro_hs_databuffer_aligned_pair_then_cross16_split_adjacent_preserved_corner();
bool ai_micro_hs_databuffer_aligned_pair_then_same_boundary_cross16_merges_by_byte_age_corner();
bool ai_micro_hs_databuffer_aligned_pair_then_same_boundary_cross16_adjacent_preserved_corner();
bool ai_micro_hs_databuffer_same_boundary_cross16_then_aligned_pair_fully_refreshes_boundary_corner();
bool ai_micro_hs_databuffer_same_boundary_cross16_then_aligned_pair_adjacent_preserved_corner();
bool ai_micro_hs_databuffer_same_boundary_split_aligned_split_no_oldest_leak_corner();
bool ai_micro_hs_databuffer_same_boundary_split_aligned_split_adjacent_preserved_corner();
bool ai_micro_hs_databuffer_same_boundary_aligned_split_aligned_no_middle_leak_corner();
bool ai_micro_hs_databuffer_same_boundary_aligned_split_aligned_adjacent_preserved_corner();
bool ai_micro_ap10_vpmatch_ldld_raw_mixed_level_marker_manual_smoke_corner();
bool ai_micro_ap10_vector_same_rob_flow_and_scalar_raw_marker_manual_smoke_corner();
bool ai_micro_ap10_wrap_pressure_mixed_level_replay_boundary_manual_smoke_corner();
bool ai_micro_ap10_mixed_redirect_then_cbo_zero_amo_side_effect_manual_smoke_corner();

#define AI_ASSERT_NO_EXCEPT_AND(message, cond) \
    TEST_ASSERT(message, excpt.triggered == false && (cond))

#ifndef CSR_SMBLOCKCTL
#define CSR_SMBLOCKCTL 0x5c3
#endif

#define AI_MICRO_OVERLAP_REPLAY_ESCAPE_ENABLE    (1ULL << 0)
#define AI_MICRO_OVERLAP_REPLAY_ESCAPE_TIME_MASK (0xFULL << 1)

static uint64_t ai_micro_overlap_saved_menvcfg;

static inline pte_t *ai_micro_overlap_hs_leaf_entry(enum test_page page)
{
#ifdef sv48
    return &hspt[4][page];
#else
    return &hspt[2][page];
#endif
}

static inline void ai_micro_overlap_map_alias_to_base(enum test_page alias_page,
    enum test_page base_page)
{
    *ai_micro_overlap_hs_leaf_entry(alias_page) =
        (phys_page_base(base_page) >> 2) | PTE_V | PTE_AD | PTE_RWX;
}

static inline void ai_micro_overlap_make_leaf_access_fault(enum test_page page)
{
    *ai_micro_overlap_hs_leaf_entry(page) |= (1ULL << 46);
}

static inline void ai_micro_overlap_enable_hs_cbo_inval(void)
{
    goto_priv(PRIV_M);
    ai_micro_overlap_saved_menvcfg = CSRR(CSR_MENVCFG);
    CSRW(CSR_MENVCFG,
        (ai_micro_overlap_saved_menvcfg & ~MENVCFG_CBIE11) |
            MENVCFG_CBIE01);
    goto_priv(PRIV_HS);
}

static inline void ai_micro_overlap_disable_hs_cbo_inval(void)
{
    goto_priv(PRIV_M);
    CSRW(CSR_MENVCFG, ai_micro_overlap_saved_menvcfg);
    goto_priv(PRIV_HS);
}

static inline uint64_t ai_micro_overlap_dense_store_pattern(uint64_t seed,
    uintptr_t line_index)
{
    return seed ^ ((0x9e3779b97f4a7c15ULL * (line_index + 1U)) +
        (line_index << 7));
}

static inline void ai_micro_overlap_dense_store_pressure_window(
    uintptr_t page_base_vaddr, uintptr_t start_line, uintptr_t line_count,
    uint64_t first_seed, uint64_t second_seed)
{
    for (uintptr_t i = 0; i < line_count; ++i) {
        uintptr_t line = start_line + i;
        *(volatile uint64_t *)(page_base_vaddr + line * 64U) =
            ai_micro_overlap_dense_store_pattern(first_seed, line);
    }

    for (uintptr_t i = 0; i < line_count; ++i) {
        uintptr_t line = start_line + i;
        *(volatile uint64_t *)(page_base_vaddr + line * 64U) =
            ai_micro_overlap_dense_store_pattern(second_seed, line);
    }
}

static inline void ai_micro_overlap_write_vector_image(uintptr_t base_paddr,
    uint64_t low_word, uint64_t high_word)
{
    write64(base_paddr, low_word);
    write64(base_paddr + 8U, high_word);
}

static inline uint64_t ai_micro_overlap_read_smblockctl(void)
{
    return CSRR(CSR_SMBLOCKCTL);
}

static inline void ai_micro_overlap_write_smblockctl(uint64_t value)
{
    CSRW(CSR_SMBLOCKCTL, value);
}

__attribute__((naked, noinline, aligned(4))) static void
ai_micro_overlap_delayed_store_then_two_loads_capture(uintptr_t store_vaddr,
    uintptr_t load0_vaddr, uintptr_t load1_vaddr, uint64_t store_value,
    uint64_t *load0_out, uint64_t *load1_out)
{
    asm volatile(
        ".option push\n"
        ".option norvc\n"
        "li t0, 0xfedcba9876543210\n"
        "li t1, 7\n"
        "divu t2, t0, t1\n"
        "addi t2, t2, 1\n"
        "divu t2, t0, t2\n"
        "andi t2, t2, 0\n"
        "add t2, t2, a0\n"
        "sd a3, 0(t2)\n"
        "ld t3, 0(a1)\n"
        "ld t4, 0(a2)\n"
        "sd t3, 0(a4)\n"
        "sd t4, 0(a5)\n"
        "ret\n"
        ".option pop\n"
        :
        :
        : "t0", "t1", "t2", "t3", "t4", "memory");
}

__attribute__((naked, noinline, aligned(4))) static void
ai_micro_overlap_delayed_data_store_then_store_then_two_loads_capture(
    uintptr_t delayed_store_vaddr, uintptr_t direct_store_vaddr,
    uintptr_t load0_vaddr, uintptr_t load1_vaddr,
    uint64_t delayed_store_value, uint64_t direct_store_value,
    uint64_t *load0_out, uint64_t *load1_out)
{
    asm volatile(
        ".option push\n"
        ".option norvc\n"
        "li t0, 0xfedcba9876543210\n"
        "li t1, 7\n"
        "divu t2, t0, t1\n"
        "addi t2, t2, 1\n"
        "divu t2, t0, t2\n"
        "andi t2, t2, 0\n"
        "or t2, t2, a4\n"
        "sd t2, 0(a0)\n"
        "sd a5, 0(a1)\n"
        "ld t3, 0(a2)\n"
        "ld t4, 0(a3)\n"
        "sd t3, 0(a6)\n"
        "sd t4, 0(a7)\n"
        "ret\n"
        ".option pop\n"
        :
        :
        : "t0", "t1", "t2", "t3", "t4", "memory");
}

__attribute__((naked, noinline, aligned(4))) static void
ai_micro_overlap_delayed_addr_store_then_store_then_two_loads_capture(
    uintptr_t delayed_store_vaddr, uintptr_t direct_store_vaddr,
    uintptr_t load0_vaddr, uintptr_t load1_vaddr,
    uint64_t delayed_store_value, uint64_t direct_store_value,
    uint64_t *load0_out, uint64_t *load1_out)
{
    asm volatile(
        ".option push\n"
        ".option norvc\n"
        "li t0, 0xfedcba9876543210\n"
        "li t1, 7\n"
        "divu t2, t0, t1\n"
        "addi t2, t2, 1\n"
        "divu t2, t0, t2\n"
        "andi t2, t2, 0\n"
        "add t2, t2, a0\n"
        "sd a4, 0(t2)\n"
        "sd a5, 0(a1)\n"
        "ld t3, 0(a2)\n"
        "ld t4, 0(a3)\n"
        "sd t3, 0(a6)\n"
        "sd t4, 0(a7)\n"
        "ret\n"
        ".option pop\n"
        :
        :
        : "t0", "t1", "t2", "t3", "t4", "memory");
}

__attribute__((naked, noinline, aligned(4))) static void
ai_micro_overlap_delayed_two_stores_then_two_loads_capture(
    uintptr_t store0_vaddr, uintptr_t store1_vaddr,
    uintptr_t load0_vaddr, uintptr_t load1_vaddr,
    uint64_t store0_value, uint64_t store1_value,
    uint64_t *load0_out, uint64_t *load1_out)
{
    asm volatile(
        ".option push\n"
        ".option norvc\n"
        "li t0, 0xfedcba9876543210\n"
        "li t1, 7\n"
        "divu t2, t0, t1\n"
        "addi t2, t2, 1\n"
        "divu t2, t0, t2\n"
        "andi t2, t2, 0\n"
        "add t2, t2, a0\n"
        "sd a4, 0(t2)\n"
        "li t0, 0x123456789abcdef0\n"
        "li t1, 11\n"
        "divu t2, t0, t1\n"
        "addi t2, t2, 1\n"
        "divu t2, t0, t2\n"
        "andi t2, t2, 0\n"
        "add t2, t2, a1\n"
        "sd a5, 0(t2)\n"
        "ld t3, 0(a2)\n"
        "ld t4, 0(a3)\n"
        "sd t3, 0(a6)\n"
        "sd t4, 0(a7)\n"
        "ret\n"
        ".option pop\n"
        :
        :
        : "t0", "t1", "t2", "t3", "t4", "memory");
}

__attribute__((naked, noinline, aligned(4))) static void
ai_micro_overlap_delayed_faulting_store_then_two_loads_capture(
    uintptr_t fault_store_vaddr, uintptr_t load0_vaddr, uintptr_t load1_vaddr,
    uintptr_t marker_vaddr, uint64_t store_value,
    uint64_t *load0_out, uint64_t *load1_out)
{
    asm volatile(
        ".option push\n"
        ".option norvc\n"
        "li t0, 0xfedcba9876543210\n"
        "li t1, 7\n"
        "divu t2, t0, t1\n"
        "addi t2, t2, 1\n"
        "divu t2, t0, t2\n"
        "andi t2, t2, 0\n"
        "add t2, t2, a0\n"
        "sd a4, 0(t2)\n"
        "ld t3, 0(a1)\n"
        "ld t4, 0(a2)\n"
        "sd t3, 0(a5)\n"
        "sd t4, 0(a6)\n"
        "li t5, 1\n"
        "sd t5, 0(a3)\n"
        "ret\n"
        ".option pop\n"
        :
        :
        : "t0", "t1", "t2", "t3", "t4", "t5", "memory");
}

__attribute__((naked, noinline, aligned(4))) static void
ai_micro_overlap_delayed_store_then_marked_two_loads(
    uintptr_t store_vaddr, uintptr_t older_load_vaddr,
    uintptr_t younger_load_vaddr, uintptr_t older_marker_vaddr,
    uintptr_t younger_marker_vaddr, uint64_t store_value)
{
    asm volatile(
        ".option push\n"
        ".option norvc\n"
        "li t0, 0xfedcba9876543210\n"
        "li t1, 7\n"
        "divu t2, t0, t1\n"
        "addi t2, t2, 1\n"
        "divu t2, t0, t2\n"
        "andi t2, t2, 0\n"
        "add t2, t2, a0\n"
        "sd a5, 0(t2)\n"
        "ld t3, 0(a1)\n"
        "sd t3, 0(a3)\n"
        "ld t4, 0(a2)\n"
        "sd t4, 0(a4)\n"
        "ret\n"
        ".option pop\n"
        :
        :
        : "t0", "t1", "t2", "t3", "t4", "memory");
}

__attribute__((naked, noinline, aligned(4))) static void
ai_micro_overlap_delayed_older_load_then_younger_loads_capture(
    uintptr_t older_load_vaddr, uintptr_t younger_same_pa_vaddr,
    uintptr_t nearby_load_vaddr, uint64_t *older_out, uint64_t *younger_out,
    uint64_t *nearby_out)
{
    asm volatile(
        ".option push\n"
        ".option norvc\n"
        "li t0, 0xfedcba9876543210\n"
        "li t1, 7\n"
        "divu t2, t0, t1\n"
        "addi t2, t2, 1\n"
        "divu t2, t0, t2\n"
        "andi t2, t2, 0\n"
        "add t2, t2, a0\n"
        "ld t3, 0(t2)\n"
        "ld t4, 0(a1)\n"
        "ld t5, 0(a2)\n"
        "sd t3, 0(a3)\n"
        "sd t4, 0(a4)\n"
        "sd t5, 0(a5)\n"
        "ret\n"
        ".option pop\n"
        :
        :
        : "t0", "t1", "t2", "t3", "t4", "t5", "memory");
}

__attribute__((naked, noinline, aligned(4))) static void
ai_micro_overlap_delayed_older_load_younger_load_then_marker_store_capture(
    uintptr_t older_load_vaddr, uintptr_t younger_same_pa_vaddr,
    uintptr_t marker_vaddr, uint64_t marker_value,
    uint64_t *older_out, uint64_t *younger_out, uint64_t *marker_out)
{
    asm volatile(
        ".option push\n"
        ".option norvc\n"
        "li t0, 0xfedcba9876543210\n"
        "li t1, 7\n"
        "divu t2, t0, t1\n"
        "addi t2, t2, 1\n"
        "divu t2, t0, t2\n"
        "andi t2, t2, 0\n"
        "add t2, t2, a0\n"
        "ld t3, 0(t2)\n"
        "ld t4, 0(a1)\n"
        "sd a3, 0(a2)\n"
        "ld t5, 0(a2)\n"
        "sd t3, 0(a4)\n"
        "sd t4, 0(a5)\n"
        "sd t5, 0(a6)\n"
        "ret\n"
        ".option pop\n"
        :
        :
        : "t0", "t1", "t2", "t3", "t4", "t5", "memory");
}

static inline uint64_t ai_micro_overlap_cross16_expected_ld_value(
    uint64_t low_pattern, uint64_t high_pattern)
{
    return ((high_pattern & 0x00ffffffffffffffULL) << 8) |
        ((low_pattern >> 56) & 0xffULL);
}

__attribute__((naked, noinline, aligned(4))) static void
ai_micro_overlap_replay_like_mab_window_capture(uintptr_t replay_vaddr,
    uintptr_t misalign_vaddr, uintptr_t aligned_vaddr, uint64_t *replay_out,
    uint64_t *misalign_out, uint64_t *aligned_out)
{
    asm volatile(
        ".option push\n"
        ".option norvc\n"
        "ld t0, 0(a0)\n"
        "ld t1, 0(a1)\n"
        "ld t2, 0(a2)\n"
        "sd t0, 0(a3)\n"
        "sd t1, 0(a4)\n"
        "sd t2, 0(a5)\n"
        "ret\n"
        ".option pop\n"
        :
        :
        : "t0", "t1", "t2", "memory");
}

__attribute__((naked, noinline, aligned(4))) static void
ai_micro_overlap_replay_like_mab_window_delayed_capture(
    uintptr_t replay_vaddr, uintptr_t misalign_vaddr, uintptr_t aligned_vaddr,
    uint64_t *replay_out, uint64_t *misalign_out, uint64_t *aligned_out)
{
    asm volatile(
        ".option push\n"
        ".option norvc\n"
        "ld t0, 0(a0)\n"
        "ld t1, 0(a1)\n"
        "li t2, 0xfedcba9876543210\n"
        "li t3, 7\n"
        "divu t4, t2, t3\n"
        "addi t4, t4, 1\n"
        "divu t4, t2, t4\n"
        "andi t4, t4, 0\n"
        "add t4, t4, a2\n"
        "ld t5, 0(t4)\n"
        "sd t0, 0(a3)\n"
        "sd t1, 0(a4)\n"
        "sd t5, 0(a5)\n"
        "ret\n"
        ".option pop\n"
        :
        :
        : "t0", "t1", "t2", "t3", "t4", "t5", "memory");
}

static bool ai_micro_overlap_replay_like_with_nearby_cross16_mab_window(
    bool check_adjacent)
{
    static const uint64_t replay_value_pattern[] = {
        0x1111222233334444ULL,
        0x5566778899aabbccULL,
        0xa1b2c3d4e5f60718ULL,
    };
    static const uint64_t replay_adjacent_pattern[] = {
        0xdeadbeefcafef00dULL,
        0x0badf00dfacefeedULL,
        0x1122334455667788ULL,
    };
    static const uint64_t misalign_warm_pattern[] = {
        0x0123456789abcdefULL,
        0xfedcba9876543210ULL,
        0x13579bdf2468ace0ULL,
    };
    static const uint64_t misalign_low_pattern[] = {
        0x8877665544332211ULL,
        0x1021324354657687ULL,
        0x55aa996633cc77eeULL,
    };
    static const uint64_t misalign_high_pattern[] = {
        0xa1b2c3d4e5f60718ULL,
        0x98badcfe10325476ULL,
        0x0f1e2d3c4b5a6978ULL,
    };
    static const uint64_t misalign_trailing_pattern[] = {
        0x89abcdef01234567ULL,
        0x76543210fedcba98ULL,
        0xcafebabedeadbeefULL,
    };
    static const uint64_t aligned_warm_pattern[] = {
        0x9999aaaabbbbccccULL,
        0xddddeeeeffff0001ULL,
        0x13572468ace0bdf1ULL,
    };
    static const uint64_t aligned_before_pattern[] = {
        0x7766554433221100ULL,
        0xf0e1d2c3b4a59687ULL,
        0x2468ace013579bdfULL,
    };
    static const uint64_t aligned_value_pattern[] = {
        0x66778899aabbccddULL,
        0x123456789abcdef0ULL,
        0x0ff1ce55aa773311ULL,
    };
    static const uint64_t aligned_after_pattern[] = {
        0x89abcdef01234567ULL,
        0x76543210fedcba98ULL,
        0x55aa33cc77ee11ffULL,
    };
    const enum test_page replay_page = VSRWX_GRWX;
    const enum test_page misalign_page = SWITCH1;
    const enum test_page aligned_page = SWITCH2;
    const enum test_page filler_page = SCRATCHPAD;
    const uintptr_t replay_vaddr = hs_page_base(replay_page) + 0x140U;
    const uintptr_t replay_adjacent_vaddr = replay_vaddr + sizeof(uint64_t);
    const uintptr_t misalign_warm_vaddr = hs_page_base(misalign_page);
    const uintptr_t misalign_vaddr = hs_page_base(misalign_page) + 0x20fU;
    const uintptr_t aligned_warm_vaddr = hs_page_base(aligned_page);
    const uintptr_t aligned_before_vaddr = hs_page_base(aligned_page) + 0x2a0U;
    const uintptr_t aligned_vaddr = hs_page_base(aligned_page) + 0x2a8U;
    const uintptr_t aligned_after_vaddr = hs_page_base(aligned_page) + 0x2b0U;
    const uintptr_t filler_base_vaddr = hs_page_base(filler_page);
    const uintptr_t replay_paddr = phys_page_base(replay_page) + 0x140U;
    const uintptr_t replay_adjacent_paddr = replay_paddr + sizeof(uint64_t);
    const uintptr_t misalign_warm_paddr = phys_page_base(misalign_page);
    const uintptr_t misalign_low_paddr = phys_page_base(misalign_page) + 0x208U;
    const uintptr_t misalign_high_paddr = phys_page_base(misalign_page) + 0x210U;
    const uintptr_t misalign_trailing_paddr = phys_page_base(misalign_page) + 0x218U;
    const uintptr_t aligned_warm_paddr = phys_page_base(aligned_page);
    const uintptr_t aligned_before_paddr = phys_page_base(aligned_page) + 0x2a0U;
    const uintptr_t aligned_paddr = phys_page_base(aligned_page) + 0x2a8U;
    const uintptr_t aligned_after_paddr = phys_page_base(aligned_page) + 0x2b0U;
    const uintptr_t filler_sample0_paddr = phys_page_base(filler_page);
    const uintptr_t filler_sample23_paddr = phys_page_base(filler_page) + 23U * 64U;
    const uintptr_t filler_sample47_paddr = phys_page_base(filler_page) + 47U * 64U;
    bool test_status = true;

    goto_priv(PRIV_HS);
    hspt_init();
    sfence_vma();
    ai_micro_overlap_enable_hs_cbo_inval();

    for (unsigned round = 0; round < 3U; ++round) {
        const uint64_t misalign_expected =
            ai_micro_overlap_cross16_expected_ld_value(
                misalign_low_pattern[round], misalign_high_pattern[round]);
        const uint64_t filler_seed0 =
            replay_value_pattern[round] ^ 0x9e3779b97f4a7c15ULL;
        const uint64_t filler_seed1 =
            aligned_value_pattern[round] ^ 0xd1b54a32d192ed03ULL;
        const uint64_t filler_seed2 =
            misalign_low_pattern[round] ^ 0x94d049bb133111ebULL;
        const uint64_t filler_seed3 =
            replay_adjacent_pattern[round] ^ 0x2545f4914f6cdd1dULL;
        uint64_t warm_replay = 0ULL;
        uint64_t warm_misalign = 0ULL;
        uint64_t warm_aligned = 0ULL;
        uint64_t replay_value = 0ULL;
        uint64_t misalign_value = 0ULL;
        uint64_t aligned_value = 0ULL;

        goto_priv(PRIV_M);
        write64(replay_paddr, replay_value_pattern[round]);
        write64(replay_adjacent_paddr, replay_adjacent_pattern[round]);
        write64(misalign_warm_paddr, misalign_warm_pattern[round]);
        write64(misalign_low_paddr, misalign_low_pattern[round]);
        write64(misalign_high_paddr, misalign_high_pattern[round]);
        write64(misalign_trailing_paddr, misalign_trailing_pattern[round]);
        write64(aligned_warm_paddr, aligned_warm_pattern[round]);
        write64(aligned_before_paddr, aligned_before_pattern[round]);
        write64(aligned_paddr, aligned_value_pattern[round]);
        write64(aligned_after_paddr, aligned_after_pattern[round]);
        write64(filler_sample0_paddr, 0ULL);
        write64(filler_sample23_paddr, 0ULL);
        write64(filler_sample47_paddr, 0ULL);

        goto_priv(PRIV_HS);
        TEST_SETUP_EXCEPT();
        warm_replay = ld(replay_vaddr);
        warm_misalign = ld(misalign_warm_vaddr);
        warm_aligned = ld(aligned_warm_vaddr);
        TEST_ASSERT("before the replay-like plus nearby cross16 shared-window probe starts, the aligned replay target, cross16 source page, and independent aligned page all warm successfully with their own seeded payloads",
            excpt.triggered == false &&
            warm_replay == replay_value_pattern[round] &&
            warm_misalign == misalign_warm_pattern[round] &&
            warm_aligned == aligned_warm_pattern[round]
        );

        TEST_SETUP_EXCEPT();
        cbo_inval(replay_vaddr);
        AI_ASSERT_NO_EXCEPT_AND("an HS cbo.inval on the replay-like target line retires without exception before the first nearby cross16 collision window",
            true
        );

        TEST_SETUP_EXCEPT();
        fence_iorw_iorw();
        AI_ASSERT_NO_EXCEPT_AND("the ordering fence after invalidating the replay-like target line retires without exception before the first nearby cross16 collision window",
            true
        );

        TEST_SETUP_EXCEPT();
        ai_micro_overlap_dense_store_pressure_window(filler_base_vaddr, 0U, 48U,
            filler_seed0, filler_seed1);
        AI_ASSERT_NO_EXCEPT_AND("dense HS store pressure on an unrelated filler page completes before the first replay-like plus nearby cross16 window and refreshes representative filler lines to the second-pass payload",
            ld(filler_base_vaddr + 0U * 64U) ==
                ai_micro_overlap_dense_store_pattern(filler_seed1, 0U) &&
            ld(filler_base_vaddr + 23U * 64U) ==
                ai_micro_overlap_dense_store_pattern(filler_seed1, 23U) &&
            ld(filler_base_vaddr + 47U * 64U) ==
                ai_micro_overlap_dense_store_pattern(filler_seed1, 47U)
        );

        TEST_SETUP_EXCEPT();
        ai_micro_overlap_replay_like_mab_window_capture(replay_vaddr,
            misalign_vaddr, aligned_vaddr, &replay_value, &misalign_value,
            &aligned_value);
        TEST_ASSERT("with a replay-like invalidated load target plus a nearby cacheable cross16 load and an independent aligned load in the same small window, all three visible results stay bound to their own payloads instead of dropping or cross-binding one completion",
            excpt.triggered == false &&
            replay_value == replay_value_pattern[round] &&
            misalign_value == misalign_expected &&
            aligned_value == aligned_value_pattern[round]
        );

        replay_value = 0ULL;
        misalign_value = 0ULL;
        aligned_value = 0ULL;
        TEST_SETUP_EXCEPT();
        cbo_inval(replay_vaddr);
        AI_ASSERT_NO_EXCEPT_AND("a second HS cbo.inval on the replay-like target line retires without exception before the delayed nearby cross16 collision window",
            true
        );

        TEST_SETUP_EXCEPT();
        fence_iorw_iorw();
        AI_ASSERT_NO_EXCEPT_AND("the ordering fence after the second invalidation retires without exception before the delayed nearby cross16 collision window",
            true
        );

        TEST_SETUP_EXCEPT();
        ai_micro_overlap_dense_store_pressure_window(filler_base_vaddr, 0U, 48U,
            filler_seed2, filler_seed3);
        AI_ASSERT_NO_EXCEPT_AND("a second unrelated dense HS store-pressure pass also completes before the delayed nearby cross16 collision window and refreshes the representative filler lines to the new second-pass payload",
            ld(filler_base_vaddr + 0U * 64U) ==
                ai_micro_overlap_dense_store_pattern(filler_seed3, 0U) &&
            ld(filler_base_vaddr + 23U * 64U) ==
                ai_micro_overlap_dense_store_pattern(filler_seed3, 23U) &&
            ld(filler_base_vaddr + 47U * 64U) ==
                ai_micro_overlap_dense_store_pattern(filler_seed3, 47U)
        );

        TEST_SETUP_EXCEPT();
        ai_micro_overlap_replay_like_mab_window_delayed_capture(replay_vaddr,
            misalign_vaddr, aligned_vaddr, &replay_value, &misalign_value,
            &aligned_value);
        TEST_ASSERT("after reopening the replay-like invalidated target and inserting a small delay before the independent aligned load, the delayed nearby cross16 window still returns only the current round's replay, cross16, and aligned payloads",
            excpt.triggered == false &&
            replay_value == replay_value_pattern[round] &&
            misalign_value == misalign_expected &&
            aligned_value == aligned_value_pattern[round]
        );

        TEST_SETUP_EXCEPT();
        uint64_t observed_replay = ld(replay_vaddr);
        AI_ASSERT_NO_EXCEPT_AND("reloading the replay-like target after the two nearby cross16 windows returns only the current round's aligned payload",
            observed_replay == replay_value_pattern[round]
        );

        TEST_SETUP_EXCEPT();
        uint64_t observed_misalign = ld(misalign_vaddr);
        AI_ASSERT_NO_EXCEPT_AND("reloading the nearby cross16 target after the replay-like collision windows returns only the merged boundary payload for the current round",
            observed_misalign == misalign_expected
        );

        TEST_SETUP_EXCEPT();
        uint64_t observed_aligned = ld(aligned_vaddr);
        AI_ASSERT_NO_EXCEPT_AND("reloading the independent aligned target after the replay-like collision windows returns only the current round's aligned payload",
            observed_aligned == aligned_value_pattern[round]
        );

        if (check_adjacent) {
            TEST_SETUP_EXCEPT();
            uint64_t observed_replay_adjacent = ld(replay_adjacent_vaddr);
            AI_ASSERT_NO_EXCEPT_AND("the adjacent replay-target word remains unchanged across the replay-like plus nearby cross16 collision windows",
                observed_replay_adjacent == replay_adjacent_pattern[round]
            );

            TEST_SETUP_EXCEPT();
            uint64_t observed_aligned_before = ld(aligned_before_vaddr);
            AI_ASSERT_NO_EXCEPT_AND("the aligned target's lower adjacent word remains unchanged across the replay-like plus nearby cross16 collision windows",
                observed_aligned_before == aligned_before_pattern[round]
            );

            TEST_SETUP_EXCEPT();
            uint64_t observed_aligned_after = ld(aligned_after_vaddr);
            AI_ASSERT_NO_EXCEPT_AND("the aligned target's upper adjacent word remains unchanged across the replay-like plus nearby cross16 collision windows",
                observed_aligned_after == aligned_after_pattern[round]
            );
        }

        goto_priv(PRIV_M);
        TEST_ASSERT("the replay-like plus nearby cross16 collision probe leaves the replay target, cross16 boundary source words, and independent aligned payload location unchanged after each round",
            read64(replay_paddr) == replay_value_pattern[round] &&
            read64(misalign_warm_paddr) == misalign_warm_pattern[round] &&
            read64(misalign_low_paddr) == misalign_low_pattern[round] &&
            read64(misalign_high_paddr) == misalign_high_pattern[round] &&
            read64(misalign_trailing_paddr) == misalign_trailing_pattern[round] &&
            read64(aligned_warm_paddr) == aligned_warm_pattern[round] &&
            read64(aligned_paddr) == aligned_value_pattern[round] &&
            read64(filler_sample0_paddr) ==
                ai_micro_overlap_dense_store_pattern(filler_seed3, 0U) &&
            read64(filler_sample23_paddr) ==
                ai_micro_overlap_dense_store_pattern(filler_seed3, 23U) &&
            read64(filler_sample47_paddr) ==
                ai_micro_overlap_dense_store_pattern(filler_seed3, 47U) &&
            (!check_adjacent ||
                (read64(replay_adjacent_paddr) == replay_adjacent_pattern[round] &&
                 read64(aligned_before_paddr) == aligned_before_pattern[round] &&
                 read64(aligned_after_paddr) == aligned_after_pattern[round]))
        );

        goto_priv(PRIV_HS);
    }

    ai_micro_overlap_disable_hs_cbo_inval();
    return test_status;
}

static bool ai_micro_overlap_storequeue_older_sbuffer_handoff_rewrite_window(
    bool check_adjacent)
{
    const enum test_page target_page = VSRWX_GRWX;
    const enum test_page filler_page = VSRW_GRW;
    const uintptr_t target_page_vaddr = hs_page_base(target_page);
    const uintptr_t filler_page_vaddr = hs_page_base(filler_page);
    const uintptr_t target_page_paddr = phys_page_base(target_page);
    const uintptr_t filler_page_paddr = phys_page_base(filler_page);
    const uintptr_t hot_vaddr = target_page_vaddr + 0x180U;
    const uintptr_t before_vaddr = hot_vaddr - sizeof(uint64_t);
    const uintptr_t adjacent_vaddr = hot_vaddr + sizeof(uint64_t);
    const uintptr_t after_vaddr = hot_vaddr + 2U * sizeof(uint64_t);
    const uintptr_t hot_paddr = target_page_paddr + 0x180U;
    const uintptr_t before_paddr = hot_paddr - sizeof(uint64_t);
    const uintptr_t adjacent_paddr = hot_paddr + sizeof(uint64_t);
    const uintptr_t after_paddr = hot_paddr + 2U * sizeof(uint64_t);
    const uintptr_t filler_sample0_paddr = filler_page_paddr + 0U * 64U;
    const uintptr_t filler_sample31_paddr = filler_page_paddr + 31U * 64U;
    const uintptr_t filler_sample63_paddr = filler_page_paddr + 63U * 64U;
    const uint64_t before_pattern[] = {
        0x0f1e2d3c4b5a6978ULL,
        0x1122334455667788ULL,
        0x89abcdef01234567ULL,
        0x55aa55aa33cc33ccULL
    };
    const uint64_t older_pattern[] = {
        0x0102030405060708ULL,
        0x1021324354657687ULL,
        0xa1b2c3d4e5f60718ULL,
        0x0badc0ffeec0de01ULL
    };
    const uint64_t younger_pattern[] = {
        0xfedcba9876543210ULL,
        0x8877665544332211ULL,
        0x13579bdf2468ace0ULL,
        0xc001d00dcafef00dULL
    };
    const uint64_t adjacent_pattern[] = {
        0x7766554433221100ULL,
        0xdeadbeefcafebabeULL,
        0x0f0e0d0c0b0a0908ULL,
        0x1234fedc5678ba98ULL
    };
    const uint64_t after_pattern[] = {
        0x8877aabbccddeeffULL,
        0x3141592653589793ULL,
        0xf0e1d2c3b4a59687ULL,
        0xa5a55a5ac3c33c3cULL
    };
    const uint64_t filler_first_seeds[] = {
        0x11c6f8ab93d04e27ULL,
        0x228be91f47a18d5eULL,
        0x3372da8bfb720c95ULL,
        0x4419cbffaf434bccULL
    };
    const uint64_t filler_second_seeds[] = {
        0x55efbc737314da03ULL,
        0x66b6ade727e5193aULL,
        0x777d9e5bdbb65871ULL,
        0x88448fcf8f8797a8ULL
    };
    const uint64_t filler_third_seeds[] = {
        0x991b70435358d6dfULL,
        0xaae261b707291616ULL,
        0xbba9522abbfa554dULL,
        0xcc70439e6fcb9484ULL
    };
    const uint64_t filler_fourth_seeds[] = {
        0xdd473412239cd3bbULL,
        0xee0e2585d76d12f2ULL,
        0xffd516f98b3e5229ULL,
        0x109c076d3f0f9160ULL
    };
    bool test_status = true;

    goto_priv(PRIV_HS);
    hspt_init();
    sfence_vma();

    for (unsigned round = 0; round < 4U; ++round) {
        goto_priv(PRIV_M);
        write64(before_paddr, before_pattern[round]);
        write64(hot_paddr, older_pattern[round] ^ 0xffff0000ffff0000ULL);
        write64(adjacent_paddr, adjacent_pattern[round] ^ 0x00ffff0000ffff00ULL);
        write64(after_paddr, after_pattern[round]);
        write64(filler_sample0_paddr, 0ULL);
        write64(filler_sample31_paddr, 0ULL);
        write64(filler_sample63_paddr, 0ULL);

        goto_priv(PRIV_HS);
        TEST_SETUP_EXCEPT();
        TEST_ASSERT("before the delayed sbuffer-handoff rewrite probe starts, the hot word, same-line guards, and representative filler lines all read back their seeded values",
            excpt.triggered == false &&
            ld(before_vaddr) == before_pattern[round] &&
            ld(hot_vaddr) == (older_pattern[round] ^ 0xffff0000ffff0000ULL) &&
            ld(adjacent_vaddr) == (adjacent_pattern[round] ^ 0x00ffff0000ffff00ULL) &&
            ld(after_vaddr) == after_pattern[round] &&
            ld(filler_page_vaddr + 0U * 64U) == 0ULL &&
            ld(filler_page_vaddr + 31U * 64U) == 0ULL &&
            ld(filler_page_vaddr + 63U * 64U) == 0ULL
        );

        TEST_SETUP_EXCEPT();
        *(volatile uint64_t *)hot_vaddr = older_pattern[round];
        ai_micro_overlap_dense_store_pressure_window(filler_page_vaddr, 0U, 64U,
            filler_first_seeds[round], filler_second_seeds[round]);
        *(volatile uint64_t *)hot_vaddr = younger_pattern[round];
        *(volatile uint64_t *)adjacent_vaddr = adjacent_pattern[round];
        ai_micro_overlap_dense_store_pressure_window(filler_page_vaddr, 0U, 64U,
            filler_third_seeds[round], filler_fourth_seeds[round]);
        uint64_t observed_hot = ld(hot_vaddr);
        uint64_t observed_adjacent = ld(adjacent_vaddr);
        fence_iorw_iorw();
        TEST_ASSERT("after an older aligned store is followed by dense younger store turnover and a final same-line rewrite, the hot word and the younger same-line word still return only the youngest payloads while representative filler lines complete with the latest pressure pattern",
            excpt.triggered == false &&
            observed_hot == younger_pattern[round] &&
            observed_adjacent == adjacent_pattern[round] &&
            ld(filler_page_vaddr + 0U * 64U) ==
                ai_micro_overlap_dense_store_pattern(filler_fourth_seeds[round], 0U) &&
            ld(filler_page_vaddr + 31U * 64U) ==
                ai_micro_overlap_dense_store_pattern(filler_fourth_seeds[round], 31U) &&
            ld(filler_page_vaddr + 63U * 64U) ==
                ai_micro_overlap_dense_store_pattern(filler_fourth_seeds[round], 63U)
        );

        if (check_adjacent) {
            TEST_SETUP_EXCEPT();
            TEST_ASSERT("the lower and upper same-line guard words remain unchanged across the older-handoff plus dense-younger-rewrite window",
                excpt.triggered == false &&
                ld(before_vaddr) == before_pattern[round] &&
                ld(after_vaddr) == after_pattern[round]
            );
        }

        goto_priv(PRIV_M);
        TEST_ASSERT("the delayed sbuffer-handoff rewrite probe leaves backing memory with only the final younger hot-word rewrite, the final adjacent-word store, unchanged guards, and the latest filler payloads",
            read64(before_paddr) == before_pattern[round] &&
            read64(hot_paddr) == younger_pattern[round] &&
            read64(adjacent_paddr) == adjacent_pattern[round] &&
            read64(after_paddr) == after_pattern[round] &&
            read64(filler_sample0_paddr) ==
                ai_micro_overlap_dense_store_pattern(filler_fourth_seeds[round], 0U) &&
            read64(filler_sample31_paddr) ==
                ai_micro_overlap_dense_store_pattern(filler_fourth_seeds[round], 31U) &&
            read64(filler_sample63_paddr) ==
                ai_micro_overlap_dense_store_pattern(filler_fourth_seeds[round], 63U)
        );
    }

    goto_priv(PRIV_HS);
    return test_status;
}

static inline uint64_t ai_micro_overlap_turnover_pattern(uint64_t base,
    unsigned round, unsigned slot)
{
    return base + ((uint64_t)round << 16) + ((uint64_t)slot << 8) +
        (uint64_t)(0x31U + slot);
}

static inline uint64_t ai_micro_overlap_cross16_store_low_word(
    uint64_t original_low, uint64_t store_value)
{
    return (original_low & 0x00ffffffffffffffULL) |
        ((store_value & 0xffULL) << 56);
}

static inline uint64_t ai_micro_overlap_cross16_store_high_word(
    uint64_t original_high, uint64_t store_value)
{
    return (original_high & 0xff00000000000000ULL) |
        ((store_value >> 8) & 0x00ffffffffffffffULL);
}

static inline void ai_micro_overlap_apply_split_store_bytes(uint64_t *low_word,
    uint64_t *high_word, unsigned low_byte_offset, unsigned store_size,
    uint64_t store_value)
{
    for (unsigned byte = 0; byte < store_size; ++byte) {
        uint64_t value_byte = (store_value >> (byte * 8U)) & 0xffULL;
        unsigned absolute = low_byte_offset + byte;
        if (absolute < 8U) {
            *low_word &= ~(0xffULL << (absolute * 8U));
            *low_word |= value_byte << (absolute * 8U);
        } else {
            unsigned high_byte = absolute - 8U;
            *high_word &= ~(0xffULL << (high_byte * 8U));
            *high_word |= value_byte << (high_byte * 8U);
        }
    }
}

static bool ai_micro_overlap_databuffer_two_pair_turnover_window(
    bool check_adjacent)
{
    const enum test_page target_page = VSRWX_GRWX;
    const enum test_page filler_page = VSRW_GRW;
    const uintptr_t target_page_vaddr = hs_page_base(target_page);
    const uintptr_t filler_page_vaddr = hs_page_base(filler_page);
    const uintptr_t target_page_paddr = phys_page_base(target_page);
    const uintptr_t filler_page_paddr = phys_page_base(filler_page);
    const uintptr_t hot_vaddrs[] = {
        target_page_vaddr + 0x118U,
        target_page_vaddr + 0x198U,
        target_page_vaddr + 0x218U,
        target_page_vaddr + 0x298U,
    };
    const uintptr_t hot_paddrs[] = {
        target_page_paddr + 0x118U,
        target_page_paddr + 0x198U,
        target_page_paddr + 0x218U,
        target_page_paddr + 0x298U,
    };
    const uintptr_t before_vaddrs[] = {
        target_page_vaddr + 0x110U,
        target_page_vaddr + 0x190U,
        target_page_vaddr + 0x210U,
        target_page_vaddr + 0x290U,
    };
    const uintptr_t before_paddrs[] = {
        target_page_paddr + 0x110U,
        target_page_paddr + 0x190U,
        target_page_paddr + 0x210U,
        target_page_paddr + 0x290U,
    };
    const uintptr_t after_vaddrs[] = {
        target_page_vaddr + 0x120U,
        target_page_vaddr + 0x1a0U,
        target_page_vaddr + 0x220U,
        target_page_vaddr + 0x2a0U,
    };
    const uintptr_t after_paddrs[] = {
        target_page_paddr + 0x120U,
        target_page_paddr + 0x1a0U,
        target_page_paddr + 0x220U,
        target_page_paddr + 0x2a0U,
    };
    const uintptr_t filler_sample0_paddr = filler_page_paddr + 0U * 64U;
    const uintptr_t filler_sample31_paddr = filler_page_paddr + 31U * 64U;
    const uintptr_t filler_sample63_paddr = filler_page_paddr + 63U * 64U;
    const uint64_t filler_first_seeds[] = {
        0x15c4f8ab93d04e27ULL,
        0x26abe91f47a18d5eULL,
        0x3792da8bfb720c95ULL,
        0x4879cbffaf434bccULL,
    };
    const uint64_t filler_second_seeds[] = {
        0x5960bc737314da03ULL,
        0x6a47ade727e5193aULL,
        0x7b2e9e5bdbb65871ULL,
        0x8c158fcf8f8797a8ULL,
    };
    const uint64_t filler_third_seeds[] = {
        0x9dfc70435358d6dfULL,
        0xaee361b707291616ULL,
        0xbfca522abbfa554dULL,
        0xd0b1439e6fcb9484ULL,
    };
    const uint64_t filler_fourth_seeds[] = {
        0xe1983412239cd3bbULL,
        0xf27f2585d76d12f2ULL,
        0x036616f98b3e5229ULL,
        0x144d076d3f0f9160ULL,
    };
    bool test_status = true;

    goto_priv(PRIV_HS);
    hspt_init();
    sfence_vma();

    for (unsigned round = 0; round < 4U; ++round) {
        uint64_t before_seed[4];
        uint64_t initial_seed[4];
        uint64_t after_seed[4];
        bool initial_ok = true;
        bool guards_ok = true;
        bool backing_ok = true;

        const uint64_t older_a =
            ai_micro_overlap_turnover_pattern(0x1102030405060000ULL, round, 0U);
        const uint64_t older_b =
            ai_micro_overlap_turnover_pattern(0x2203141516170000ULL, round, 1U);
        const uint64_t younger_c =
            ai_micro_overlap_turnover_pattern(0x3304252627280000ULL, round, 2U);
        const uint64_t younger_d =
            ai_micro_overlap_turnover_pattern(0x4405363738390000ULL, round, 3U);
        const uint64_t rewrite_a =
            ai_micro_overlap_turnover_pattern(0x55064748494a0000ULL, round, 0U);
        const uint64_t rewrite_b =
            ai_micro_overlap_turnover_pattern(0x660758595a5b0000ULL, round, 1U);

        goto_priv(PRIV_M);
        for (unsigned i = 0; i < 4U; ++i) {
            before_seed[i] = ai_micro_overlap_turnover_pattern(
                0x0f1e2d3c4b5a0000ULL, round, i);
            initial_seed[i] = ai_micro_overlap_turnover_pattern(
                0x1029384756ab0000ULL, round, i);
            after_seed[i] = ai_micro_overlap_turnover_pattern(
                0x89abcdef01230000ULL, round, i);

            write64(before_paddrs[i], before_seed[i]);
            write64(hot_paddrs[i], initial_seed[i]);
            write64(after_paddrs[i], after_seed[i]);
        }
        write64(filler_sample0_paddr, 0ULL);
        write64(filler_sample31_paddr, 0ULL);
        write64(filler_sample63_paddr, 0ULL);

        goto_priv(PRIV_HS);
        TEST_SETUP_EXCEPT();
        for (unsigned i = 0; i < 4U; ++i) {
            initial_ok = initial_ok &&
                ld(before_vaddrs[i]) == before_seed[i] &&
                ld(hot_vaddrs[i]) == initial_seed[i] &&
                ld(after_vaddrs[i]) == after_seed[i];
        }
        initial_ok = initial_ok &&
            ld(filler_page_vaddr + 0U * 64U) == 0ULL &&
            ld(filler_page_vaddr + 31U * 64U) == 0ULL &&
            ld(filler_page_vaddr + 63U * 64U) == 0ULL;
        TEST_ASSERT("before the databuffer two-pair turnover probe starts, all four hot lines, their same-line guards, and representative filler lines read back their seeded values",
            excpt.triggered == false && initial_ok
        );

        TEST_SETUP_EXCEPT();
        *(volatile uint64_t *)hot_vaddrs[0] = older_a;
        *(volatile uint64_t *)hot_vaddrs[1] = older_b;
        ai_micro_overlap_dense_store_pressure_window(filler_page_vaddr, 0U, 64U,
            filler_first_seeds[round], filler_second_seeds[round]);
        *(volatile uint64_t *)hot_vaddrs[2] = younger_c;
        *(volatile uint64_t *)hot_vaddrs[3] = younger_d;
        ai_micro_overlap_dense_store_pressure_window(filler_page_vaddr, 0U, 64U,
            filler_third_seeds[round], filler_fourth_seeds[round]);
        *(volatile uint64_t *)hot_vaddrs[0] = rewrite_a;
        *(volatile uint64_t *)hot_vaddrs[1] = rewrite_b;

        uint64_t observed_a = ld(hot_vaddrs[0]);
        uint64_t observed_b = ld(hot_vaddrs[1]);
        uint64_t observed_c = ld(hot_vaddrs[2]);
        uint64_t observed_d = ld(hot_vaddrs[3]);
        fence_iorw_iorw();
        TEST_ASSERT("after two-pair store turnover with dense intermediate pressure, the first pair keeps only its final rewrites, the second pair keeps only its own payloads, and the representative filler lines converge to the latest pressure image",
            excpt.triggered == false &&
            observed_a == rewrite_a &&
            observed_b == rewrite_b &&
            observed_c == younger_c &&
            observed_d == younger_d &&
            ld(filler_page_vaddr + 0U * 64U) ==
                ai_micro_overlap_dense_store_pattern(filler_fourth_seeds[round], 0U) &&
            ld(filler_page_vaddr + 31U * 64U) ==
                ai_micro_overlap_dense_store_pattern(filler_fourth_seeds[round], 31U) &&
            ld(filler_page_vaddr + 63U * 64U) ==
                ai_micro_overlap_dense_store_pattern(filler_fourth_seeds[round], 63U)
        );

        if (check_adjacent) {
            TEST_SETUP_EXCEPT();
            for (unsigned i = 0; i < 4U; ++i) {
                guards_ok = guards_ok &&
                    ld(before_vaddrs[i]) == before_seed[i] &&
                    ld(after_vaddrs[i]) == after_seed[i];
            }
            TEST_ASSERT("the databuffer two-pair turnover probe leaves every same-line guard word unchanged across all four hot lines",
                excpt.triggered == false && guards_ok
            );
        }

        goto_priv(PRIV_M);
        for (unsigned i = 0; i < 4U; ++i) {
            uint64_t expected_hot = (i == 0U) ? rewrite_a :
                (i == 1U) ? rewrite_b :
                (i == 2U) ? younger_c : younger_d;
            backing_ok = backing_ok &&
                read64(before_paddrs[i]) == before_seed[i] &&
                read64(hot_paddrs[i]) == expected_hot &&
                read64(after_paddrs[i]) == after_seed[i];
        }
        backing_ok = backing_ok &&
            read64(filler_sample0_paddr) ==
                ai_micro_overlap_dense_store_pattern(filler_fourth_seeds[round], 0U) &&
            read64(filler_sample31_paddr) ==
                ai_micro_overlap_dense_store_pattern(filler_fourth_seeds[round], 31U) &&
            read64(filler_sample63_paddr) ==
                ai_micro_overlap_dense_store_pattern(filler_fourth_seeds[round], 63U);
        TEST_ASSERT("the databuffer two-pair turnover probe leaves backing memory with only the final first-pair rewrites, the intact second-pair payloads, unchanged guards, and the latest filler payloads",
            backing_ok
        );
    }

    goto_priv(PRIV_HS);
    return test_status;
}

static bool ai_micro_overlap_cross16_pseudo_second_half_aligned_batch_window(
    bool check_adjacent)
{
    const enum test_page target_page = VSRWX_GRWX;
    const enum test_page filler_page = VSRW_GRW;
    const uintptr_t target_page_vaddr = hs_page_base(target_page);
    const uintptr_t filler_page_vaddr = hs_page_base(filler_page);
    const uintptr_t target_page_paddr = phys_page_base(target_page);
    const uintptr_t filler_page_paddr = phys_page_base(filler_page);
    const uintptr_t misalign_before_vaddr = target_page_vaddr + 0x100U;
    const uintptr_t misalign_low_vaddr = target_page_vaddr + 0x108U;
    const uintptr_t misalign_store_vaddr = target_page_vaddr + 0x10fU;
    const uintptr_t misalign_high_vaddr = target_page_vaddr + 0x110U;
    const uintptr_t misalign_after_vaddr = target_page_vaddr + 0x118U;
    const uintptr_t younger0_before_vaddr = target_page_vaddr + 0x190U;
    const uintptr_t younger0_vaddr = target_page_vaddr + 0x198U;
    const uintptr_t younger0_after_vaddr = target_page_vaddr + 0x1a0U;
    const uintptr_t younger1_before_vaddr = target_page_vaddr + 0x210U;
    const uintptr_t younger1_vaddr = target_page_vaddr + 0x218U;
    const uintptr_t younger1_after_vaddr = target_page_vaddr + 0x220U;
    const uintptr_t misalign_before_paddr = target_page_paddr + 0x100U;
    const uintptr_t misalign_low_paddr = target_page_paddr + 0x108U;
    const uintptr_t misalign_high_paddr = target_page_paddr + 0x110U;
    const uintptr_t misalign_after_paddr = target_page_paddr + 0x118U;
    const uintptr_t younger0_before_paddr = target_page_paddr + 0x190U;
    const uintptr_t younger0_paddr = target_page_paddr + 0x198U;
    const uintptr_t younger0_after_paddr = target_page_paddr + 0x1a0U;
    const uintptr_t younger1_before_paddr = target_page_paddr + 0x210U;
    const uintptr_t younger1_paddr = target_page_paddr + 0x218U;
    const uintptr_t younger1_after_paddr = target_page_paddr + 0x220U;
    const uintptr_t filler_sample0_paddr = filler_page_paddr + 0U * 64U;
    const uintptr_t filler_sample31_paddr = filler_page_paddr + 31U * 64U;
    const uintptr_t filler_sample63_paddr = filler_page_paddr + 63U * 64U;
    const uint64_t filler_first_seeds[] = {
        0x17d8e4a1b30c5f26ULL,
        0x28b9f573c14d6e47ULL,
        0x399a8645d28e7f68ULL,
        0x4a7b9717e3bf8089ULL,
    };
    const uint64_t filler_second_seeds[] = {
        0x5b5ca8e9f41091aaULL,
        0x6c3db9bb0541a2cbULL,
        0x7d1eca8d1672b3ecULL,
        0x8effdb5f27a3c40dULL,
    };
    const uint64_t filler_third_seeds[] = {
        0x9fe0ec3138d4d52eULL,
        0xb0c1fd034a05e64fULL,
        0xc1a20ed55b36f770ULL,
        0xd2831fa76c680891ULL,
    };
    const uint64_t filler_fourth_seeds[] = {
        0xe36430797d9919b2ULL,
        0xf445414b8eca2ad3ULL,
        0x0526521d9ffb3bf4ULL,
        0x160763efb12c4d15ULL,
    };
    bool test_status = true;

    goto_priv(PRIV_HS);
    hspt_init();
    sfence_vma();

    for (unsigned round = 0; round < 4U; ++round) {
        const uint64_t misalign_before_seed =
            ai_micro_overlap_turnover_pattern(0x010f1e2d3c4b0000ULL, round, 0U);
        const uint64_t misalign_low_seed =
            ai_micro_overlap_turnover_pattern(0x1122334455660000ULL, round, 1U);
        const uint64_t misalign_high_seed =
            ai_micro_overlap_turnover_pattern(0x2233445566770000ULL, round, 2U);
        const uint64_t misalign_after_seed =
            ai_micro_overlap_turnover_pattern(0x3344556677880000ULL, round, 3U);
        const uint64_t younger0_before_seed =
            ai_micro_overlap_turnover_pattern(0x4455667788990000ULL, round, 4U);
        const uint64_t younger0_seed =
            ai_micro_overlap_turnover_pattern(0x5566778899aa0000ULL, round, 5U);
        const uint64_t younger0_after_seed =
            ai_micro_overlap_turnover_pattern(0x66778899aabb0000ULL, round, 6U);
        const uint64_t younger1_before_seed =
            ai_micro_overlap_turnover_pattern(0x778899aabbcc0000ULL, round, 7U);
        const uint64_t younger1_seed =
            ai_micro_overlap_turnover_pattern(0x8899aabbccdd0000ULL, round, 8U);
        const uint64_t younger1_after_seed =
            ai_micro_overlap_turnover_pattern(0x99aabbccddee0000ULL, round, 9U);
        const uint64_t misalign_store_value =
            ai_micro_overlap_turnover_pattern(0xa1b2c3d4e5f60000ULL, round, 10U);
        const uint64_t younger0_value =
            ai_micro_overlap_turnover_pattern(0xb2c3d4e5f6070000ULL, round, 11U);
        const uint64_t younger1_value =
            ai_micro_overlap_turnover_pattern(0xc3d4e5f607180000ULL, round, 12U);
        const uint64_t expected_low =
            ai_micro_overlap_cross16_store_low_word(
                misalign_low_seed, misalign_store_value);
        const uint64_t expected_high =
            ai_micro_overlap_cross16_store_high_word(
                misalign_high_seed, misalign_store_value);
        bool initial_ok;
        bool guards_ok;
        bool backing_ok;

        goto_priv(PRIV_M);
        write64(misalign_before_paddr, misalign_before_seed);
        write64(misalign_low_paddr, misalign_low_seed);
        write64(misalign_high_paddr, misalign_high_seed);
        write64(misalign_after_paddr, misalign_after_seed);
        write64(younger0_before_paddr, younger0_before_seed);
        write64(younger0_paddr, younger0_seed);
        write64(younger0_after_paddr, younger0_after_seed);
        write64(younger1_before_paddr, younger1_before_seed);
        write64(younger1_paddr, younger1_seed);
        write64(younger1_after_paddr, younger1_after_seed);
        write64(filler_sample0_paddr, 0ULL);
        write64(filler_sample31_paddr, 0ULL);
        write64(filler_sample63_paddr, 0ULL);

        goto_priv(PRIV_HS);
        TEST_SETUP_EXCEPT();
        initial_ok =
            ld(misalign_before_vaddr) == misalign_before_seed &&
            ld(misalign_low_vaddr) == misalign_low_seed &&
            ld(misalign_high_vaddr) == misalign_high_seed &&
            ld(misalign_after_vaddr) == misalign_after_seed &&
            ld(younger0_before_vaddr) == younger0_before_seed &&
            ld(younger0_vaddr) == younger0_seed &&
            ld(younger0_after_vaddr) == younger0_after_seed &&
            ld(younger1_before_vaddr) == younger1_before_seed &&
            ld(younger1_vaddr) == younger1_seed &&
            ld(younger1_after_vaddr) == younger1_after_seed &&
            ld(filler_page_vaddr + 0U * 64U) == 0ULL &&
            ld(filler_page_vaddr + 31U * 64U) == 0ULL &&
            ld(filler_page_vaddr + 63U * 64U) == 0ULL;
        TEST_ASSERT("before the cross16 pseudo-second-half plus younger aligned-batch probe starts, the split boundary words, aligned hot lines, guards, and representative filler lines all read back their seeded values",
            excpt.triggered == false && initial_ok
        );

        TEST_SETUP_EXCEPT();
        sd(misalign_store_vaddr, misalign_store_value);
        ai_micro_overlap_dense_store_pressure_window(filler_page_vaddr, 0U, 64U,
            filler_first_seeds[round], filler_second_seeds[round]);
        *(volatile uint64_t *)younger0_vaddr = younger0_value;
        *(volatile uint64_t *)younger1_vaddr = younger1_value;
        ai_micro_overlap_dense_store_pressure_window(filler_page_vaddr, 0U, 64U,
            filler_third_seeds[round], filler_fourth_seeds[round]);

        uint64_t observed_low = ld(misalign_low_vaddr);
        uint64_t observed_high = ld(misalign_high_vaddr);
        uint64_t observed_younger0 = ld(younger0_vaddr);
        uint64_t observed_younger1 = ld(younger1_vaddr);
        fence_iorw_iorw();
        TEST_ASSERT("after a cross16 misaligned store opens the pseudo-second-half path and a younger aligned batch follows under dense turnover, the split boundary words and both younger stores keep only their own payloads while filler pressure converges to the latest image",
            excpt.triggered == false &&
            observed_low == expected_low &&
            observed_high == expected_high &&
            observed_younger0 == younger0_value &&
            observed_younger1 == younger1_value &&
            ld(filler_page_vaddr + 0U * 64U) ==
                ai_micro_overlap_dense_store_pattern(filler_fourth_seeds[round], 0U) &&
            ld(filler_page_vaddr + 31U * 64U) ==
                ai_micro_overlap_dense_store_pattern(filler_fourth_seeds[round], 31U) &&
            ld(filler_page_vaddr + 63U * 64U) ==
                ai_micro_overlap_dense_store_pattern(filler_fourth_seeds[round], 63U)
        );

        if (check_adjacent) {
            TEST_SETUP_EXCEPT();
            guards_ok =
                ld(misalign_before_vaddr) == misalign_before_seed &&
                ld(misalign_after_vaddr) == misalign_after_seed &&
                ld(younger0_before_vaddr) == younger0_before_seed &&
                ld(younger0_after_vaddr) == younger0_after_seed &&
                ld(younger1_before_vaddr) == younger1_before_seed &&
                ld(younger1_after_vaddr) == younger1_after_seed;
            TEST_ASSERT("the cross16 pseudo-second-half plus younger aligned-batch probe leaves every surrounding guard word unchanged",
                excpt.triggered == false && guards_ok
            );
        }

        goto_priv(PRIV_M);
        backing_ok =
            read64(misalign_before_paddr) == misalign_before_seed &&
            read64(misalign_low_paddr) == expected_low &&
            read64(misalign_high_paddr) == expected_high &&
            read64(misalign_after_paddr) == misalign_after_seed &&
            read64(younger0_before_paddr) == younger0_before_seed &&
            read64(younger0_paddr) == younger0_value &&
            read64(younger0_after_paddr) == younger0_after_seed &&
            read64(younger1_before_paddr) == younger1_before_seed &&
            read64(younger1_paddr) == younger1_value &&
            read64(younger1_after_paddr) == younger1_after_seed &&
            read64(filler_sample0_paddr) ==
                ai_micro_overlap_dense_store_pattern(filler_fourth_seeds[round], 0U) &&
            read64(filler_sample31_paddr) ==
                ai_micro_overlap_dense_store_pattern(filler_fourth_seeds[round], 31U) &&
            read64(filler_sample63_paddr) ==
                ai_micro_overlap_dense_store_pattern(filler_fourth_seeds[round], 63U);
        TEST_ASSERT("the cross16 pseudo-second-half plus younger aligned-batch probe leaves backing memory with only the split-store boundary bytes, the younger aligned batch payloads, unchanged guards, and the latest filler image",
            backing_ok
        );
    }

    goto_priv(PRIV_HS);
    return test_status;
}

static bool ai_micro_overlap_cross16_high_half_younger_rewrite_window(
    bool check_adjacent)
{
    const enum test_page target_page = VSRWX_GRWX;
    const enum test_page filler_page = VSRW_GRW;
    const uintptr_t target_page_vaddr = hs_page_base(target_page);
    const uintptr_t filler_page_vaddr = hs_page_base(filler_page);
    const uintptr_t target_page_paddr = phys_page_base(target_page);
    const uintptr_t filler_page_paddr = phys_page_base(filler_page);
    const uintptr_t misalign_before_vaddr = target_page_vaddr + 0x100U;
    const uintptr_t misalign_low_vaddr = target_page_vaddr + 0x108U;
    const uintptr_t misalign_store_vaddr = target_page_vaddr + 0x10fU;
    const uintptr_t misalign_high_vaddr = target_page_vaddr + 0x110U;
    const uintptr_t misalign_after_vaddr = target_page_vaddr + 0x118U;
    const uintptr_t companion_before_vaddr = target_page_vaddr + 0x190U;
    const uintptr_t companion_vaddr = target_page_vaddr + 0x198U;
    const uintptr_t companion_after_vaddr = target_page_vaddr + 0x1a0U;
    const uintptr_t misalign_before_paddr = target_page_paddr + 0x100U;
    const uintptr_t misalign_low_paddr = target_page_paddr + 0x108U;
    const uintptr_t misalign_high_paddr = target_page_paddr + 0x110U;
    const uintptr_t misalign_after_paddr = target_page_paddr + 0x118U;
    const uintptr_t companion_before_paddr = target_page_paddr + 0x190U;
    const uintptr_t companion_paddr = target_page_paddr + 0x198U;
    const uintptr_t companion_after_paddr = target_page_paddr + 0x1a0U;
    const uintptr_t filler_sample0_paddr = filler_page_paddr + 0U * 64U;
    const uintptr_t filler_sample31_paddr = filler_page_paddr + 31U * 64U;
    const uintptr_t filler_sample63_paddr = filler_page_paddr + 63U * 64U;
    const uint64_t filler_first_seeds[] = {
        0x0c91e5a27bd34f16ULL,
        0x1d72f6948ce46037ULL,
        0x2e5307869df57158ULL,
        0x3f341878af068279ULL,
    };
    const uint64_t filler_second_seeds[] = {
        0x5015296ac017939aULL,
        0x60f63a5cd128a4bbULL,
        0x71d74b4ee239b5dcULL,
        0x82b85c40f34ac6fdULL,
    };
    const uint64_t filler_third_seeds[] = {
        0x93996d33045bd81eULL,
        0xa47a7e25156ce93fULL,
        0xb55b8f17267dfa60ULL,
        0xc63ca009378f0b81ULL,
    };
    const uint64_t filler_fourth_seeds[] = {
        0xd71db0fb48a01ca2ULL,
        0xe7fec1ed59b12dc3ULL,
        0xf8dfd2df6ac23ee4ULL,
        0x09c0e3d17bd35005ULL,
    };
    bool test_status = true;

    goto_priv(PRIV_HS);
    hspt_init();
    sfence_vma();

    for (unsigned round = 0; round < 4U; ++round) {
        const uint64_t misalign_before_seed =
            ai_micro_overlap_turnover_pattern(0x1021324354650000ULL, round, 0U);
        const uint64_t misalign_low_seed =
            ai_micro_overlap_turnover_pattern(0x2132435465760000ULL, round, 1U);
        const uint64_t misalign_high_seed =
            ai_micro_overlap_turnover_pattern(0x3243546576870000ULL, round, 2U);
        const uint64_t misalign_after_seed =
            ai_micro_overlap_turnover_pattern(0x4354657687980000ULL, round, 3U);
        const uint64_t companion_before_seed =
            ai_micro_overlap_turnover_pattern(0x5465768798a90000ULL, round, 4U);
        const uint64_t companion_seed =
            ai_micro_overlap_turnover_pattern(0x65768798a9ba0000ULL, round, 5U);
        const uint64_t companion_after_seed =
            ai_micro_overlap_turnover_pattern(0x768798a9bacb0000ULL, round, 6U);
        const uint64_t misalign_store_value =
            ai_micro_overlap_turnover_pattern(0x8798a9bacbdc0000ULL, round, 7U);
        const uint64_t high_rewrite_value =
            ai_micro_overlap_turnover_pattern(0x98a9bacbdced0000ULL, round, 8U);
        const uint64_t companion_value =
            ai_micro_overlap_turnover_pattern(0xa9bacbdcedfe0000ULL, round, 9U);
        const uint64_t expected_low =
            ai_micro_overlap_cross16_store_low_word(
                misalign_low_seed, misalign_store_value);
        bool initial_ok;
        bool guards_ok;
        bool backing_ok;

        goto_priv(PRIV_M);
        write64(misalign_before_paddr, misalign_before_seed);
        write64(misalign_low_paddr, misalign_low_seed);
        write64(misalign_high_paddr, misalign_high_seed);
        write64(misalign_after_paddr, misalign_after_seed);
        write64(companion_before_paddr, companion_before_seed);
        write64(companion_paddr, companion_seed);
        write64(companion_after_paddr, companion_after_seed);
        write64(filler_sample0_paddr, 0ULL);
        write64(filler_sample31_paddr, 0ULL);
        write64(filler_sample63_paddr, 0ULL);

        goto_priv(PRIV_HS);
        TEST_SETUP_EXCEPT();
        initial_ok =
            ld(misalign_before_vaddr) == misalign_before_seed &&
            ld(misalign_low_vaddr) == misalign_low_seed &&
            ld(misalign_high_vaddr) == misalign_high_seed &&
            ld(misalign_after_vaddr) == misalign_after_seed &&
            ld(companion_before_vaddr) == companion_before_seed &&
            ld(companion_vaddr) == companion_seed &&
            ld(companion_after_vaddr) == companion_after_seed &&
            ld(filler_page_vaddr + 0U * 64U) == 0ULL &&
            ld(filler_page_vaddr + 31U * 64U) == 0ULL &&
            ld(filler_page_vaddr + 63U * 64U) == 0ULL;
        TEST_ASSERT("before the cross16 high-half younger-rewrite probe starts, the split boundary words, companion line, guards, and representative filler lines all read back their seeded values",
            excpt.triggered == false && initial_ok
        );

        TEST_SETUP_EXCEPT();
        sd(misalign_store_vaddr, misalign_store_value);
        ai_micro_overlap_dense_store_pressure_window(filler_page_vaddr, 0U, 64U,
            filler_first_seeds[round], filler_second_seeds[round]);
        *(volatile uint64_t *)misalign_high_vaddr = high_rewrite_value;
        *(volatile uint64_t *)companion_vaddr = companion_value;
        ai_micro_overlap_dense_store_pressure_window(filler_page_vaddr, 0U, 64U,
            filler_third_seeds[round], filler_fourth_seeds[round]);

        uint64_t observed_low = ld(misalign_low_vaddr);
        uint64_t observed_high = ld(misalign_high_vaddr);
        uint64_t observed_companion = ld(companion_vaddr);
        fence_iorw_iorw();
        TEST_ASSERT("after a cross16 store opens the pseudo-second-half path and a younger same-word aligned rewrite follows, the low word keeps only the older split byte, the high word keeps only the younger rewrite, the companion store keeps its own payload, and filler pressure converges to the latest image",
            excpt.triggered == false &&
            observed_low == expected_low &&
            observed_high == high_rewrite_value &&
            observed_companion == companion_value &&
            ld(filler_page_vaddr + 0U * 64U) ==
                ai_micro_overlap_dense_store_pattern(filler_fourth_seeds[round], 0U) &&
            ld(filler_page_vaddr + 31U * 64U) ==
                ai_micro_overlap_dense_store_pattern(filler_fourth_seeds[round], 31U) &&
            ld(filler_page_vaddr + 63U * 64U) ==
                ai_micro_overlap_dense_store_pattern(filler_fourth_seeds[round], 63U)
        );

        if (check_adjacent) {
            TEST_SETUP_EXCEPT();
            guards_ok =
                ld(misalign_before_vaddr) == misalign_before_seed &&
                ld(misalign_after_vaddr) == misalign_after_seed &&
                ld(companion_before_vaddr) == companion_before_seed &&
                ld(companion_after_vaddr) == companion_after_seed;
            TEST_ASSERT("the cross16 high-half younger-rewrite probe leaves every surrounding guard word unchanged",
                excpt.triggered == false && guards_ok
            );
        }

        goto_priv(PRIV_M);
        backing_ok =
            read64(misalign_before_paddr) == misalign_before_seed &&
            read64(misalign_low_paddr) == expected_low &&
            read64(misalign_high_paddr) == high_rewrite_value &&
            read64(misalign_after_paddr) == misalign_after_seed &&
            read64(companion_before_paddr) == companion_before_seed &&
            read64(companion_paddr) == companion_value &&
            read64(companion_after_paddr) == companion_after_seed &&
            read64(filler_sample0_paddr) ==
                ai_micro_overlap_dense_store_pattern(filler_fourth_seeds[round], 0U) &&
            read64(filler_sample31_paddr) ==
                ai_micro_overlap_dense_store_pattern(filler_fourth_seeds[round], 31U) &&
            read64(filler_sample63_paddr) ==
                ai_micro_overlap_dense_store_pattern(filler_fourth_seeds[round], 63U);
        TEST_ASSERT("the cross16 high-half younger-rewrite probe leaves backing memory with only the split low-byte update, the younger aligned high-word rewrite, the companion payload, unchanged guards, and the latest filler image",
            backing_ok
        );
    }

    goto_priv(PRIV_HS);
    return test_status;
}

static bool ai_micro_overlap_cross16_scalar_vector_wb_window(bool check_adjacent)
{
    const enum test_page target_page = VSRWX_GRWX;
    const enum test_page filler_page = VSRW_GRW;
    const enum test_page vector_src_page = SWITCH1;
    const enum test_page vector_dst_page = SCRATCHPAD;
    const uintptr_t target_page_vaddr = hs_page_base(target_page);
    const uintptr_t filler_page_vaddr = hs_page_base(filler_page);
    const uintptr_t vector_src_vaddr = hs_page_base(vector_src_page) + 0x300U;
    const uintptr_t vector_dst_vaddr = hs_page_base(vector_dst_page) + 0x380U;
    const uintptr_t target_page_paddr = phys_page_base(target_page);
    const uintptr_t filler_page_paddr = phys_page_base(filler_page);
    const uintptr_t vector_src_paddr = phys_page_base(vector_src_page) + 0x300U;
    const uintptr_t vector_dst_paddr = phys_page_base(vector_dst_page) + 0x380U;
    const uintptr_t misalign_before_vaddr = target_page_vaddr + 0x100U;
    const uintptr_t misalign_low_vaddr = target_page_vaddr + 0x108U;
    const uintptr_t misalign_store_vaddr = target_page_vaddr + 0x10fU;
    const uintptr_t misalign_high_vaddr = target_page_vaddr + 0x110U;
    const uintptr_t misalign_after_vaddr = target_page_vaddr + 0x118U;
    const uintptr_t companion_before_vaddr = target_page_vaddr + 0x190U;
    const uintptr_t companion_vaddr = target_page_vaddr + 0x198U;
    const uintptr_t companion_after_vaddr = target_page_vaddr + 0x1a0U;
    const uintptr_t vector_before_vaddr = vector_dst_vaddr - 1U;
    const uintptr_t vector_after_vaddr = vector_dst_vaddr + 16U;
    const uintptr_t misalign_before_paddr = target_page_paddr + 0x100U;
    const uintptr_t misalign_low_paddr = target_page_paddr + 0x108U;
    const uintptr_t misalign_high_paddr = target_page_paddr + 0x110U;
    const uintptr_t misalign_after_paddr = target_page_paddr + 0x118U;
    const uintptr_t companion_before_paddr = target_page_paddr + 0x190U;
    const uintptr_t companion_paddr = target_page_paddr + 0x198U;
    const uintptr_t companion_after_paddr = target_page_paddr + 0x1a0U;
    const uintptr_t vector_before_paddr = vector_dst_paddr - 1U;
    const uintptr_t vector_after_paddr = vector_dst_paddr + 16U;
    const uintptr_t filler_sample0_paddr = filler_page_paddr + 0U * 64U;
    const uintptr_t filler_sample31_paddr = filler_page_paddr + 31U * 64U;
    const uintptr_t filler_sample63_paddr = filler_page_paddr + 63U * 64U;
    static const uint64_t vector_src_low[] = {
        0x1817161514131211ULL,
        0x2827262524232221ULL,
        0x3837363534333231ULL,
        0x4847464544434241ULL,
    };
    static const uint64_t vector_src_high[] = {
        0x8887868584838281ULL,
        0x9897969594939291ULL,
        0xa8a7a6a5a4a3a2a1ULL,
        0xb8b7b6b5b4b3b2b1ULL,
    };
    static const uint64_t vector_dst_seed_low[] = {
        0xf1e2d3c4b5a69788ULL,
        0xe0d1c2b3a4958677ULL,
        0xd0c1b2a394857667ULL,
        0xc0b1a29384756657ULL,
    };
    static const uint64_t vector_dst_seed_high[] = {
        0x1029384756a5b4c3ULL,
        0x213a495867b6c5d4ULL,
        0x324b5a6978c7d6e5ULL,
        0x435c6b7a89d8e7f6ULL,
    };
    const uint8_t vector_before_sentinel = 0x5cU;
    const uint8_t vector_after_sentinel = 0xc5U;
    const uint64_t filler_first_seeds[] = {
        0x0f1e2d3c4b5a6978ULL,
        0x102f3e4d5c6b7a89ULL,
        0x21405f6e7d8c9babULL,
        0x3251707f8e9dacbcULL,
    };
    const uint64_t filler_second_seeds[] = {
        0x43628190afbecddcULL,
        0x547392a1c0dfeeedULL,
        0x6584a3b2d1ef0ffeULL,
        0x7695b4c3e201102fULL,
    };
    const uint64_t filler_third_seeds[] = {
        0x87a6c5d4f3122140ULL,
        0x98b7d6e503233251ULL,
        0xa9c8e7f614344362ULL,
        0xbad9f80525455473ULL,
    };
    const uint64_t filler_fourth_seeds[] = {
        0xcbeb091636566584ULL,
        0xdcfc1a2747677695ULL,
        0xed0d2b38587887a6ULL,
        0xfe1e3c49698998b7ULL,
    };
    bool test_status = true;

    goto_priv(PRIV_HS);
    hspt_init();
    sfence_vma();

    for (unsigned round = 0; round < 4U; ++round) {
        const uint64_t misalign_before_seed =
            ai_micro_overlap_turnover_pattern(0x1021324354650000ULL, round, 0U);
        const uint64_t misalign_low_seed =
            ai_micro_overlap_turnover_pattern(0x2132435465760000ULL, round, 1U);
        const uint64_t misalign_high_seed =
            ai_micro_overlap_turnover_pattern(0x3243546576870000ULL, round, 2U);
        const uint64_t misalign_after_seed =
            ai_micro_overlap_turnover_pattern(0x4354657687980000ULL, round, 3U);
        const uint64_t companion_before_seed =
            ai_micro_overlap_turnover_pattern(0x5465768798a90000ULL, round, 4U);
        const uint64_t companion_seed =
            ai_micro_overlap_turnover_pattern(0x65768798a9ba0000ULL, round, 5U);
        const uint64_t companion_after_seed =
            ai_micro_overlap_turnover_pattern(0x768798a9bacb0000ULL, round, 6U);
        const uint64_t misalign_store_value =
            ai_micro_overlap_turnover_pattern(0x8798a9bacbdc0000ULL, round, 7U);
        const uint64_t high_rewrite_value =
            ai_micro_overlap_turnover_pattern(0x98a9bacbdced0000ULL, round, 8U);
        const uint64_t companion_value =
            ai_micro_overlap_turnover_pattern(0xa9bacbdcedfe0000ULL, round, 9U);
        const uint64_t expected_low =
            ai_micro_overlap_cross16_store_low_word(
                misalign_low_seed, misalign_store_value);
        bool initial_ok;
        bool guards_ok;
        bool backing_ok;

        goto_priv(PRIV_M);
        write64(misalign_before_paddr, misalign_before_seed);
        write64(misalign_low_paddr, misalign_low_seed);
        write64(misalign_high_paddr, misalign_high_seed);
        write64(misalign_after_paddr, misalign_after_seed);
        write64(companion_before_paddr, companion_before_seed);
        write64(companion_paddr, companion_seed);
        write64(companion_after_paddr, companion_after_seed);
        ai_micro_overlap_write_vector_image(vector_src_paddr,
            vector_src_low[round], vector_src_high[round]);
        ai_micro_overlap_write_vector_image(vector_dst_paddr,
            vector_dst_seed_low[round], vector_dst_seed_high[round]);
        write8(vector_before_paddr, vector_before_sentinel);
        write8(vector_after_paddr, vector_after_sentinel);
        write64(filler_sample0_paddr, 0ULL);
        write64(filler_sample31_paddr, 0ULL);
        write64(filler_sample63_paddr, 0ULL);

        goto_priv(PRIV_HS);
        TEST_SETUP_EXCEPT();
        initial_ok =
            ld(misalign_before_vaddr) == misalign_before_seed &&
            ld(misalign_low_vaddr) == misalign_low_seed &&
            ld(misalign_high_vaddr) == misalign_high_seed &&
            ld(misalign_after_vaddr) == misalign_after_seed &&
            ld(companion_before_vaddr) == companion_before_seed &&
            ld(companion_vaddr) == companion_seed &&
            ld(companion_after_vaddr) == companion_after_seed &&
            ld(vector_dst_vaddr) == vector_dst_seed_low[round] &&
            ld(vector_dst_vaddr + 8U) == vector_dst_seed_high[round] &&
            *(volatile uint8_t *)vector_before_vaddr == vector_before_sentinel &&
            *(volatile uint8_t *)vector_after_vaddr == vector_after_sentinel &&
            ld(filler_page_vaddr + 0U * 64U) == 0ULL &&
            ld(filler_page_vaddr + 31U * 64U) == 0ULL &&
            ld(filler_page_vaddr + 63U * 64U) == 0ULL;
        TEST_ASSERT("before the cross16 scalar plus unrelated vector-writeback probe starts, the split boundary words, vector destination, guards, and representative filler lines all read back their seeded values",
            excpt.triggered == false && initial_ok
        );

        TEST_SETUP_EXCEPT();
        sd(misalign_store_vaddr, misalign_store_value);
        ai_micro_overlap_dense_store_pressure_window(filler_page_vaddr, 0U, 64U,
            filler_first_seeds[round], filler_second_seeds[round]);
        vle8_to_v6((const uint8_t *)vector_src_vaddr, 16);
        vse8_from_v6((uint8_t *)vector_dst_vaddr, 16);
        *(volatile uint64_t *)misalign_high_vaddr = high_rewrite_value;
        *(volatile uint64_t *)companion_vaddr = companion_value;
        ai_micro_overlap_dense_store_pressure_window(filler_page_vaddr, 0U, 64U,
            filler_third_seeds[round], filler_fourth_seeds[round]);

        uint64_t observed_low = ld(misalign_low_vaddr);
        uint64_t observed_high = ld(misalign_high_vaddr);
        uint64_t observed_companion = ld(companion_vaddr);
        uint64_t observed_vector_low = ld(vector_dst_vaddr);
        uint64_t observed_vector_high = ld(vector_dst_vaddr + 8U);
        fence_iorw_iorw();
        TEST_ASSERT("after an older cross16 scalar store is followed by unrelated vector-store writeback pressure and a younger same-word high-word rewrite, the low word keeps only the older split byte, the high word keeps only the younger rewrite, the vector target keeps only its own payload, and filler pressure converges to the latest image",
            excpt.triggered == false &&
            observed_low == expected_low &&
            observed_high == high_rewrite_value &&
            observed_companion == companion_value &&
            observed_vector_low == vector_src_low[round] &&
            observed_vector_high == vector_src_high[round] &&
            ld(filler_page_vaddr + 0U * 64U) ==
                ai_micro_overlap_dense_store_pattern(filler_fourth_seeds[round], 0U) &&
            ld(filler_page_vaddr + 31U * 64U) ==
                ai_micro_overlap_dense_store_pattern(filler_fourth_seeds[round], 31U) &&
            ld(filler_page_vaddr + 63U * 64U) ==
                ai_micro_overlap_dense_store_pattern(filler_fourth_seeds[round], 63U)
        );

        if (check_adjacent) {
            TEST_SETUP_EXCEPT();
            guards_ok =
                ld(misalign_before_vaddr) == misalign_before_seed &&
                ld(misalign_after_vaddr) == misalign_after_seed &&
                ld(companion_before_vaddr) == companion_before_seed &&
                ld(companion_after_vaddr) == companion_after_seed &&
                *(volatile uint8_t *)vector_before_vaddr == vector_before_sentinel &&
                *(volatile uint8_t *)vector_after_vaddr == vector_after_sentinel;
            TEST_ASSERT("the cross16 scalar plus unrelated vector-writeback probe leaves the scalar guard words and vector adjacent sentinels unchanged",
                excpt.triggered == false && guards_ok
            );
        }

        goto_priv(PRIV_M);
        backing_ok =
            read64(misalign_before_paddr) == misalign_before_seed &&
            read64(misalign_low_paddr) == expected_low &&
            read64(misalign_high_paddr) == high_rewrite_value &&
            read64(misalign_after_paddr) == misalign_after_seed &&
            read64(companion_before_paddr) == companion_before_seed &&
            read64(companion_paddr) == companion_value &&
            read64(companion_after_paddr) == companion_after_seed &&
            read64(vector_dst_paddr) == vector_src_low[round] &&
            read64(vector_dst_paddr + 8U) == vector_src_high[round] &&
            read8(vector_before_paddr) == vector_before_sentinel &&
            read8(vector_after_paddr) == vector_after_sentinel &&
            read64(filler_sample0_paddr) ==
                ai_micro_overlap_dense_store_pattern(filler_fourth_seeds[round], 0U) &&
            read64(filler_sample31_paddr) ==
                ai_micro_overlap_dense_store_pattern(filler_fourth_seeds[round], 31U) &&
            read64(filler_sample63_paddr) ==
                ai_micro_overlap_dense_store_pattern(filler_fourth_seeds[round], 63U);
        TEST_ASSERT("the cross16 scalar plus unrelated vector-writeback probe leaves backing memory with only the split low-byte update, the younger aligned high-word rewrite, the companion payload, the independent vector payload, unchanged guards, and the latest filler image",
            backing_ok
        );
    }

    goto_priv(PRIV_HS);
    return test_status;
}

static bool ai_micro_overlap_cross16_vector_wb_same_boundary_refresh_window(
    bool check_adjacent)
{
    const enum test_page target_page = VSRWX_GRWX;
    const enum test_page filler_page = VSRW_GRW;
    const enum test_page vector_src_page = SWITCH1;
    const enum test_page vector_dst_page = SCRATCHPAD;
    const uintptr_t target_page_vaddr = hs_page_base(target_page);
    const uintptr_t filler_page_vaddr = hs_page_base(filler_page);
    const uintptr_t vector_src_vaddr = hs_page_base(vector_src_page) + 0x480U;
    const uintptr_t vector_dst_vaddr = hs_page_base(vector_dst_page) + 0x4c0U;
    const uintptr_t target_page_paddr = phys_page_base(target_page);
    const uintptr_t filler_page_paddr = phys_page_base(filler_page);
    const uintptr_t vector_src_paddr = phys_page_base(vector_src_page) + 0x480U;
    const uintptr_t vector_dst_paddr = phys_page_base(vector_dst_page) + 0x4c0U;
    const uintptr_t before_vaddr = target_page_vaddr + 0x100U;
    const uintptr_t low_vaddr = target_page_vaddr + 0x108U;
    const uintptr_t split_store_vaddr = target_page_vaddr + 0x10fU;
    const uintptr_t high_vaddr = target_page_vaddr + 0x110U;
    const uintptr_t after_vaddr = target_page_vaddr + 0x118U;
    const uintptr_t vector_before_vaddr = vector_dst_vaddr - 1U;
    const uintptr_t vector_after_vaddr = vector_dst_vaddr + 16U;
    const uintptr_t before_paddr = target_page_paddr + 0x100U;
    const uintptr_t low_paddr = target_page_paddr + 0x108U;
    const uintptr_t high_paddr = target_page_paddr + 0x110U;
    const uintptr_t after_paddr = target_page_paddr + 0x118U;
    const uintptr_t vector_before_paddr = vector_dst_paddr - 1U;
    const uintptr_t vector_after_paddr = vector_dst_paddr + 16U;
    const uintptr_t filler_sample0_paddr = filler_page_paddr + 0U * 64U;
    const uintptr_t filler_sample31_paddr = filler_page_paddr + 31U * 64U;
    const uintptr_t filler_sample63_paddr = filler_page_paddr + 63U * 64U;
    static const uint64_t vector_src_low[] = {
        0x1011121314151617ULL,
        0x2021222324252627ULL,
        0x3031323334353637ULL,
        0x4041424344454647ULL,
    };
    static const uint64_t vector_src_high[] = {
        0x8081828384858687ULL,
        0x9091929394959697ULL,
        0xa0a1a2a3a4a5a6a7ULL,
        0xb0b1b2b3b4b5b6b7ULL,
    };
    static const uint64_t vector_dst_seed_low[] = {
        0xf0e1d2c3b4a59687ULL,
        0xe1d2c3b4a5968778ULL,
        0xd2c3b4a596877869ULL,
        0xc3b4a5968778695aULL,
    };
    static const uint64_t vector_dst_seed_high[] = {
        0x13579bdf2468ace0ULL,
        0x2468ace13579bdf1ULL,
        0x3579bdf2468ace02ULL,
        0x468ace13579bdf13ULL,
    };
    const uint8_t vector_before_sentinel = 0x69U;
    const uint8_t vector_after_sentinel = 0x96U;
    const uint64_t filler_first_seeds[] = {
        0x1112131415161718ULL,
        0x2122232425262728ULL,
        0x3132333435363738ULL,
        0x4142434445464748ULL,
    };
    const uint64_t filler_second_seeds[] = {
        0x5152535455565758ULL,
        0x6162636465666768ULL,
        0x7172737475767778ULL,
        0x8182838485868788ULL,
    };
    const uint64_t filler_third_seeds[] = {
        0x9192939495969798ULL,
        0xa1a2a3a4a5a6a7a8ULL,
        0xb1b2b3b4b5b6b7b8ULL,
        0xc1c2c3c4c5c6c7c8ULL,
    };
    const uint64_t filler_fourth_seeds[] = {
        0xd1d2d3d4d5d6d7d8ULL,
        0xe1e2e3e4e5e6e7e8ULL,
        0xf1f2f3f4f5f6f7f8ULL,
        0x0102030405060708ULL,
    };
    bool test_status = true;

    goto_priv(PRIV_HS);
    hspt_init();
    sfence_vma();

    for (unsigned round = 0; round < 4U; ++round) {
        const uint64_t before_seed =
            ai_micro_overlap_turnover_pattern(0x18394a5b6c7d0000ULL, round, 0U);
        const uint64_t low_seed =
            ai_micro_overlap_turnover_pattern(0x294a5b6c7d8e0000ULL, round, 1U);
        const uint64_t high_seed =
            ai_micro_overlap_turnover_pattern(0x3a5b6c7d8e9f0000ULL, round, 2U);
        const uint64_t after_seed =
            ai_micro_overlap_turnover_pattern(0x4b6c7d8e9fb00000ULL, round, 3U);
        const uint64_t older_store_value =
            ai_micro_overlap_turnover_pattern(0x5c7d8e9fb0c10000ULL, round, 4U);
        const uint64_t younger_store_value =
            ai_micro_overlap_turnover_pattern(0x6d8e9fb0c1d20000ULL, round, 5U);
        const uint64_t expected_low =
            ai_micro_overlap_cross16_store_low_word(
                low_seed, younger_store_value);
        const uint64_t expected_high =
            ai_micro_overlap_cross16_store_high_word(
                high_seed, younger_store_value);
        bool initial_ok;
        bool guards_ok;
        bool backing_ok;

        goto_priv(PRIV_M);
        write64(before_paddr, before_seed);
        write64(low_paddr, low_seed);
        write64(high_paddr, high_seed);
        write64(after_paddr, after_seed);
        ai_micro_overlap_write_vector_image(vector_src_paddr,
            vector_src_low[round], vector_src_high[round]);
        ai_micro_overlap_write_vector_image(vector_dst_paddr,
            vector_dst_seed_low[round], vector_dst_seed_high[round]);
        write8(vector_before_paddr, vector_before_sentinel);
        write8(vector_after_paddr, vector_after_sentinel);
        write64(filler_sample0_paddr, 0ULL);
        write64(filler_sample31_paddr, 0ULL);
        write64(filler_sample63_paddr, 0ULL);

        goto_priv(PRIV_HS);
        TEST_SETUP_EXCEPT();
        initial_ok =
            ld(before_vaddr) == before_seed &&
            ld(low_vaddr) == low_seed &&
            ld(high_vaddr) == high_seed &&
            ld(after_vaddr) == after_seed &&
            ld(vector_dst_vaddr) == vector_dst_seed_low[round] &&
            ld(vector_dst_vaddr + 8U) == vector_dst_seed_high[round] &&
            *(volatile uint8_t *)vector_before_vaddr == vector_before_sentinel &&
            *(volatile uint8_t *)vector_after_vaddr == vector_after_sentinel &&
            ld(filler_page_vaddr + 0U * 64U) == 0ULL &&
            ld(filler_page_vaddr + 31U * 64U) == 0ULL &&
            ld(filler_page_vaddr + 63U * 64U) == 0ULL;
        TEST_ASSERT("before the cross16 scalar plus unrelated vector-writeback same-boundary refresh probe starts, the boundary words, vector destination, guards, and representative filler lines all read back their seeded values",
            excpt.triggered == false && initial_ok
        );

        TEST_SETUP_EXCEPT();
        sd(split_store_vaddr, older_store_value);
        ai_micro_overlap_dense_store_pressure_window(filler_page_vaddr, 0U, 64U,
            filler_first_seeds[round], filler_second_seeds[round]);
        vle8_to_v6((const uint8_t *)vector_src_vaddr, 16);
        vse8_from_v6((uint8_t *)vector_dst_vaddr, 16);
        sd(split_store_vaddr, younger_store_value);
        ai_micro_overlap_dense_store_pressure_window(filler_page_vaddr, 0U, 64U,
            filler_third_seeds[round], filler_fourth_seeds[round]);

        uint64_t observed_low = ld(low_vaddr);
        uint64_t observed_high = ld(high_vaddr);
        uint64_t observed_vector_low = ld(vector_dst_vaddr);
        uint64_t observed_vector_high = ld(vector_dst_vaddr + 8U);
        fence_iorw_iorw();
        TEST_ASSERT("after an older cross16 scalar store is followed by unrelated vector-store writeback pressure and then a younger same-boundary cross16 refresh, the final boundary keeps only the younger split image while the vector target keeps only its own payload and filler pressure converges to the latest image",
            excpt.triggered == false &&
            observed_low == expected_low &&
            observed_high == expected_high &&
            observed_vector_low == vector_src_low[round] &&
            observed_vector_high == vector_src_high[round] &&
            ld(filler_page_vaddr + 0U * 64U) ==
                ai_micro_overlap_dense_store_pattern(filler_fourth_seeds[round], 0U) &&
            ld(filler_page_vaddr + 31U * 64U) ==
                ai_micro_overlap_dense_store_pattern(filler_fourth_seeds[round], 31U) &&
            ld(filler_page_vaddr + 63U * 64U) ==
                ai_micro_overlap_dense_store_pattern(filler_fourth_seeds[round], 63U)
        );

        if (check_adjacent) {
            TEST_SETUP_EXCEPT();
            guards_ok =
                ld(before_vaddr) == before_seed &&
                ld(after_vaddr) == after_seed &&
                *(volatile uint8_t *)vector_before_vaddr == vector_before_sentinel &&
                *(volatile uint8_t *)vector_after_vaddr == vector_after_sentinel;
            TEST_ASSERT("the cross16 scalar plus unrelated vector-writeback same-boundary refresh probe leaves the outer guard words and vector adjacent sentinels unchanged",
                excpt.triggered == false && guards_ok
            );
        }

        goto_priv(PRIV_M);
        backing_ok =
            read64(before_paddr) == before_seed &&
            read64(low_paddr) == expected_low &&
            read64(high_paddr) == expected_high &&
            read64(after_paddr) == after_seed &&
            read64(vector_dst_paddr) == vector_src_low[round] &&
            read64(vector_dst_paddr + 8U) == vector_src_high[round] &&
            read8(vector_before_paddr) == vector_before_sentinel &&
            read8(vector_after_paddr) == vector_after_sentinel &&
            read64(filler_sample0_paddr) ==
                ai_micro_overlap_dense_store_pattern(filler_fourth_seeds[round], 0U) &&
            read64(filler_sample31_paddr) ==
                ai_micro_overlap_dense_store_pattern(filler_fourth_seeds[round], 31U) &&
            read64(filler_sample63_paddr) ==
                ai_micro_overlap_dense_store_pattern(filler_fourth_seeds[round], 63U);
        TEST_ASSERT("the cross16 scalar plus unrelated vector-writeback same-boundary refresh probe leaves backing memory with only the younger split image, the independent vector payload, unchanged guards, and the latest filler image",
            backing_ok
        );
    }

    goto_priv(PRIV_HS);
    return test_status;
}

static bool ai_micro_overlap_cross16_vector_wb_cross_boundary_window(
    bool check_adjacent)
{
    const enum test_page target_page = VSRWX_GRWX;
    const enum test_page filler_page = VSRW_GRW;
    const enum test_page vector_src_page = SWITCH1;
    const enum test_page vector_dst_page = SCRATCHPAD;
    const uintptr_t target_page_vaddr = hs_page_base(target_page);
    const uintptr_t filler_page_vaddr = hs_page_base(filler_page);
    const uintptr_t vector_src_vaddr = hs_page_base(vector_src_page) + 0x580U;
    const uintptr_t vector_dst_vaddr = hs_page_base(vector_dst_page) + 0x5c0U;
    const uintptr_t target_page_paddr = phys_page_base(target_page);
    const uintptr_t filler_page_paddr = phys_page_base(filler_page);
    const uintptr_t vector_src_paddr = phys_page_base(vector_src_page) + 0x580U;
    const uintptr_t vector_dst_paddr = phys_page_base(vector_dst_page) + 0x5c0U;
    const uintptr_t first_before_vaddr = target_page_vaddr + 0x100U;
    const uintptr_t first_low_vaddr = target_page_vaddr + 0x108U;
    const uintptr_t first_store_vaddr = target_page_vaddr + 0x10fU;
    const uintptr_t first_high_vaddr = target_page_vaddr + 0x110U;
    const uintptr_t first_after_vaddr = target_page_vaddr + 0x118U;
    const uintptr_t second_before_vaddr = target_page_vaddr + 0x180U;
    const uintptr_t second_low_vaddr = target_page_vaddr + 0x188U;
    const uintptr_t second_store_vaddr = target_page_vaddr + 0x18fU;
    const uintptr_t second_high_vaddr = target_page_vaddr + 0x190U;
    const uintptr_t second_after_vaddr = target_page_vaddr + 0x198U;
    const uintptr_t vector_before_vaddr = vector_dst_vaddr - 1U;
    const uintptr_t vector_after_vaddr = vector_dst_vaddr + 16U;
    const uintptr_t first_before_paddr = target_page_paddr + 0x100U;
    const uintptr_t first_low_paddr = target_page_paddr + 0x108U;
    const uintptr_t first_high_paddr = target_page_paddr + 0x110U;
    const uintptr_t first_after_paddr = target_page_paddr + 0x118U;
    const uintptr_t second_before_paddr = target_page_paddr + 0x180U;
    const uintptr_t second_low_paddr = target_page_paddr + 0x188U;
    const uintptr_t second_high_paddr = target_page_paddr + 0x190U;
    const uintptr_t second_after_paddr = target_page_paddr + 0x198U;
    const uintptr_t vector_before_paddr = vector_dst_paddr - 1U;
    const uintptr_t vector_after_paddr = vector_dst_paddr + 16U;
    const uintptr_t filler_sample0_paddr = filler_page_paddr + 0U * 64U;
    const uintptr_t filler_sample31_paddr = filler_page_paddr + 31U * 64U;
    const uintptr_t filler_sample63_paddr = filler_page_paddr + 63U * 64U;
    static const uint64_t vector_src_low[] = {
        0x11131517191b1d1fULL,
        0x21232527292b2d2fULL,
        0x31333537393b3d3fULL,
        0x41434547494b4d4fULL,
    };
    static const uint64_t vector_src_high[] = {
        0x81838587898b8d8fULL,
        0x91939597999b9d9fULL,
        0xa1a3a5a7a9abadafULL,
        0xb1b3b5b7b9bbbdbfULL,
    };
    static const uint64_t vector_dst_seed_low[] = {
        0xfedcba9876543210ULL,
        0xedcba9876543210fULL,
        0xdcba9876543210feULL,
        0xcba9876543210fedULL,
    };
    static const uint64_t vector_dst_seed_high[] = {
        0x0123456789abcdefULL,
        0x123456789abcdef0ULL,
        0x23456789abcdef01ULL,
        0x3456789abcdef012ULL,
    };
    const uint8_t vector_before_sentinel = 0x3cU;
    const uint8_t vector_after_sentinel = 0xc3U;
    const uint64_t filler_first_seeds[] = {
        0x1425364758697a8bULL,
        0x25364758697a8b9cULL,
        0x364758697a8b9cadULL,
        0x4758697a8b9cadbeULL,
    };
    const uint64_t filler_second_seeds[] = {
        0x58697a8b9cadbecfULL,
        0x697a8b9cadbecfe0ULL,
        0x7a8b9cadbecfe0f1ULL,
        0x8b9cadbecfe0f102ULL,
    };
    const uint64_t filler_third_seeds[] = {
        0x9cadbecfe0f10213ULL,
        0xadbecfe0f1021324ULL,
        0xbecfe0f102132435ULL,
        0xcfe0f10213243546ULL,
    };
    const uint64_t filler_fourth_seeds[] = {
        0xe0f1021324354657ULL,
        0xf102132435465768ULL,
        0x0213243546576879ULL,
        0x132435465768798aULL,
    };
    bool test_status = true;

    goto_priv(PRIV_HS);
    hspt_init();
    sfence_vma();

    for (unsigned round = 0; round < 4U; ++round) {
        const uint64_t first_before_seed =
            ai_micro_overlap_turnover_pattern(0x2031425364750000ULL, round, 0U);
        const uint64_t first_low_seed =
            ai_micro_overlap_turnover_pattern(0x3142536475860000ULL, round, 1U);
        const uint64_t first_high_seed =
            ai_micro_overlap_turnover_pattern(0x4253647586970000ULL, round, 2U);
        const uint64_t first_after_seed =
            ai_micro_overlap_turnover_pattern(0x5364758697a80000ULL, round, 3U);
        const uint64_t second_before_seed =
            ai_micro_overlap_turnover_pattern(0x64758697a8b90000ULL, round, 4U);
        const uint64_t second_low_seed =
            ai_micro_overlap_turnover_pattern(0x758697a8b9ca0000ULL, round, 5U);
        const uint64_t second_high_seed =
            ai_micro_overlap_turnover_pattern(0x8697a8b9cadb0000ULL, round, 6U);
        const uint64_t second_after_seed =
            ai_micro_overlap_turnover_pattern(0x97a8b9cadbec0000ULL, round, 7U);
        const uint64_t first_store_value =
            ai_micro_overlap_turnover_pattern(0xa8b9cadbecfd0000ULL, round, 8U);
        const uint64_t second_store_value =
            ai_micro_overlap_turnover_pattern(0xb9cadbecfd0e0000ULL, round, 9U);
        const uint64_t expected_first_low =
            ai_micro_overlap_cross16_store_low_word(
                first_low_seed, first_store_value);
        const uint64_t expected_first_high =
            ai_micro_overlap_cross16_store_high_word(
                first_high_seed, first_store_value);
        const uint64_t expected_second_low =
            ai_micro_overlap_cross16_store_low_word(
                second_low_seed, second_store_value);
        const uint64_t expected_second_high =
            ai_micro_overlap_cross16_store_high_word(
                second_high_seed, second_store_value);
        bool initial_ok;
        bool guards_ok;
        bool backing_ok;

        goto_priv(PRIV_M);
        write64(first_before_paddr, first_before_seed);
        write64(first_low_paddr, first_low_seed);
        write64(first_high_paddr, first_high_seed);
        write64(first_after_paddr, first_after_seed);
        write64(second_before_paddr, second_before_seed);
        write64(second_low_paddr, second_low_seed);
        write64(second_high_paddr, second_high_seed);
        write64(second_after_paddr, second_after_seed);
        ai_micro_overlap_write_vector_image(vector_src_paddr,
            vector_src_low[round], vector_src_high[round]);
        ai_micro_overlap_write_vector_image(vector_dst_paddr,
            vector_dst_seed_low[round], vector_dst_seed_high[round]);
        write8(vector_before_paddr, vector_before_sentinel);
        write8(vector_after_paddr, vector_after_sentinel);
        write64(filler_sample0_paddr, 0ULL);
        write64(filler_sample31_paddr, 0ULL);
        write64(filler_sample63_paddr, 0ULL);

        goto_priv(PRIV_HS);
        TEST_SETUP_EXCEPT();
        initial_ok =
            ld(first_before_vaddr) == first_before_seed &&
            ld(first_low_vaddr) == first_low_seed &&
            ld(first_high_vaddr) == first_high_seed &&
            ld(first_after_vaddr) == first_after_seed &&
            ld(second_before_vaddr) == second_before_seed &&
            ld(second_low_vaddr) == second_low_seed &&
            ld(second_high_vaddr) == second_high_seed &&
            ld(second_after_vaddr) == second_after_seed &&
            ld(vector_dst_vaddr) == vector_dst_seed_low[round] &&
            ld(vector_dst_vaddr + 8U) == vector_dst_seed_high[round] &&
            *(volatile uint8_t *)vector_before_vaddr == vector_before_sentinel &&
            *(volatile uint8_t *)vector_after_vaddr == vector_after_sentinel &&
            ld(filler_page_vaddr + 0U * 64U) == 0ULL &&
            ld(filler_page_vaddr + 31U * 64U) == 0ULL &&
            ld(filler_page_vaddr + 63U * 64U) == 0ULL;
        TEST_ASSERT("before the cross16 scalar plus unrelated vector-writeback cross-boundary probe starts, both boundary pairs, the vector destination, guards, and representative filler lines all read back their seeded values",
            excpt.triggered == false && initial_ok
        );

        TEST_SETUP_EXCEPT();
        sd(first_store_vaddr, first_store_value);
        ai_micro_overlap_dense_store_pressure_window(filler_page_vaddr, 0U, 64U,
            filler_first_seeds[round], filler_second_seeds[round]);
        vle8_to_v6((const uint8_t *)vector_src_vaddr, 16);
        vse8_from_v6((uint8_t *)vector_dst_vaddr, 16);
        sd(second_store_vaddr, second_store_value);
        ai_micro_overlap_dense_store_pressure_window(filler_page_vaddr, 0U, 64U,
            filler_third_seeds[round], filler_fourth_seeds[round]);

        uint64_t observed_first_low = ld(first_low_vaddr);
        uint64_t observed_first_high = ld(first_high_vaddr);
        uint64_t observed_second_low = ld(second_low_vaddr);
        uint64_t observed_second_high = ld(second_high_vaddr);
        uint64_t observed_vector_low = ld(vector_dst_vaddr);
        uint64_t observed_vector_high = ld(vector_dst_vaddr + 8U);
        fence_iorw_iorw();
        TEST_ASSERT("after an older cross16 scalar store is followed by unrelated vector-store writeback pressure and then a younger cross16 store on another boundary, each boundary keeps only its own split image, the vector target keeps only its own payload, and filler pressure converges to the latest image",
            excpt.triggered == false &&
            observed_first_low == expected_first_low &&
            observed_first_high == expected_first_high &&
            observed_second_low == expected_second_low &&
            observed_second_high == expected_second_high &&
            observed_vector_low == vector_src_low[round] &&
            observed_vector_high == vector_src_high[round] &&
            ld(filler_page_vaddr + 0U * 64U) ==
                ai_micro_overlap_dense_store_pattern(filler_fourth_seeds[round], 0U) &&
            ld(filler_page_vaddr + 31U * 64U) ==
                ai_micro_overlap_dense_store_pattern(filler_fourth_seeds[round], 31U) &&
            ld(filler_page_vaddr + 63U * 64U) ==
                ai_micro_overlap_dense_store_pattern(filler_fourth_seeds[round], 63U)
        );

        if (check_adjacent) {
            TEST_SETUP_EXCEPT();
            guards_ok =
                ld(first_before_vaddr) == first_before_seed &&
                ld(first_after_vaddr) == first_after_seed &&
                ld(second_before_vaddr) == second_before_seed &&
                ld(second_after_vaddr) == second_after_seed &&
                *(volatile uint8_t *)vector_before_vaddr == vector_before_sentinel &&
                *(volatile uint8_t *)vector_after_vaddr == vector_after_sentinel;
            TEST_ASSERT("the cross16 scalar plus unrelated vector-writeback cross-boundary probe leaves every boundary guard word and vector adjacent sentinel unchanged",
                excpt.triggered == false && guards_ok
            );
        }

        goto_priv(PRIV_M);
        backing_ok =
            read64(first_before_paddr) == first_before_seed &&
            read64(first_low_paddr) == expected_first_low &&
            read64(first_high_paddr) == expected_first_high &&
            read64(first_after_paddr) == first_after_seed &&
            read64(second_before_paddr) == second_before_seed &&
            read64(second_low_paddr) == expected_second_low &&
            read64(second_high_paddr) == expected_second_high &&
            read64(second_after_paddr) == second_after_seed &&
            read64(vector_dst_paddr) == vector_src_low[round] &&
            read64(vector_dst_paddr + 8U) == vector_src_high[round] &&
            read8(vector_before_paddr) == vector_before_sentinel &&
            read8(vector_after_paddr) == vector_after_sentinel &&
            read64(filler_sample0_paddr) ==
                ai_micro_overlap_dense_store_pattern(filler_fourth_seeds[round], 0U) &&
            read64(filler_sample31_paddr) ==
                ai_micro_overlap_dense_store_pattern(filler_fourth_seeds[round], 31U) &&
            read64(filler_sample63_paddr) ==
                ai_micro_overlap_dense_store_pattern(filler_fourth_seeds[round], 63U);
        TEST_ASSERT("the cross16 scalar plus unrelated vector-writeback cross-boundary probe leaves backing memory with both correct split boundary images, the independent vector payload, unchanged guards, and the latest filler image",
            backing_ok
        );
    }

    goto_priv(PRIV_HS);
    return test_status;
}

static bool ai_micro_overlap_cross16_vector_wb_cross_boundary_width_switch_window(
    bool check_adjacent)
{
    const enum test_page target_page = VSRWX_GRWX;
    const enum test_page filler_page = VSRW_GRW;
    const enum test_page vector_src_page = SWITCH1;
    const enum test_page vector_dst_page = SCRATCHPAD;
    const uintptr_t target_page_vaddr = hs_page_base(target_page);
    const uintptr_t filler_page_vaddr = hs_page_base(filler_page);
    const uintptr_t vector_src_vaddr = hs_page_base(vector_src_page) + 0x680U;
    const uintptr_t vector_dst_vaddr = hs_page_base(vector_dst_page) + 0x6c0U;
    const uintptr_t target_page_paddr = phys_page_base(target_page);
    const uintptr_t filler_page_paddr = phys_page_base(filler_page);
    const uintptr_t vector_src_paddr = phys_page_base(vector_src_page) + 0x680U;
    const uintptr_t vector_dst_paddr = phys_page_base(vector_dst_page) + 0x6c0U;
    const uintptr_t first_before_vaddr = target_page_vaddr + 0x100U;
    const uintptr_t first_low_vaddr = target_page_vaddr + 0x108U;
    const uintptr_t first_store_vaddr = target_page_vaddr + 0x10dU;
    const uintptr_t first_high_vaddr = target_page_vaddr + 0x110U;
    const uintptr_t first_after_vaddr = target_page_vaddr + 0x118U;
    const uintptr_t second_before_vaddr = target_page_vaddr + 0x180U;
    const uintptr_t second_low_vaddr = target_page_vaddr + 0x188U;
    const uintptr_t second_store_vaddr = target_page_vaddr + 0x189U;
    const uintptr_t second_high_vaddr = target_page_vaddr + 0x190U;
    const uintptr_t second_after_vaddr = target_page_vaddr + 0x198U;
    const uintptr_t vector_before_vaddr = vector_dst_vaddr - 1U;
    const uintptr_t vector_after_vaddr = vector_dst_vaddr + 16U;
    const uintptr_t first_before_paddr = target_page_paddr + 0x100U;
    const uintptr_t first_low_paddr = target_page_paddr + 0x108U;
    const uintptr_t first_high_paddr = target_page_paddr + 0x110U;
    const uintptr_t first_after_paddr = target_page_paddr + 0x118U;
    const uintptr_t second_before_paddr = target_page_paddr + 0x180U;
    const uintptr_t second_low_paddr = target_page_paddr + 0x188U;
    const uintptr_t second_high_paddr = target_page_paddr + 0x190U;
    const uintptr_t second_after_paddr = target_page_paddr + 0x198U;
    const uintptr_t vector_before_paddr = vector_dst_paddr - 1U;
    const uintptr_t vector_after_paddr = vector_dst_paddr + 16U;
    const uintptr_t filler_sample0_paddr = filler_page_paddr + 0U * 64U;
    const uintptr_t filler_sample31_paddr = filler_page_paddr + 31U * 64U;
    const uintptr_t filler_sample63_paddr = filler_page_paddr + 63U * 64U;
    static const uint64_t vector_src_low[] = {
        0x121416181a1c1e20ULL,
        0x222426282a2c2e30ULL,
        0x323436383a3c3e40ULL,
        0x424446484a4c4e50ULL,
    };
    static const uint64_t vector_src_high[] = {
        0x828486888a8c8e90ULL,
        0x929496989a9c9ea0ULL,
        0xa2a4a6a8aaacaeb0ULL,
        0xb2b4b6b8babcbec0ULL,
    };
    static const uint64_t vector_dst_seed_low[] = {
        0x0f1e2d3c4b5a6978ULL,
        0x1f2e3d4c5b6a7988ULL,
        0x2f3e4d5c6b7a8998ULL,
        0x3f4e5d6c7b8a99a8ULL,
    };
    static const uint64_t vector_dst_seed_high[] = {
        0x89abcdef01234567ULL,
        0x9abcdef012345678ULL,
        0xabcdef0123456789ULL,
        0xbcdef0123456789aULL,
    };
    const uint8_t vector_before_sentinel = 0x4bU;
    const uint8_t vector_after_sentinel = 0xb4U;
    const uint64_t filler_first_seeds[] = {
        0x162738495a6b7c8dULL,
        0x2738495a6b7c8d9eULL,
        0x38495a6b7c8d9eafULL,
        0x495a6b7c8d9eafc0ULL,
    };
    const uint64_t filler_second_seeds[] = {
        0x5a6b7c8d9eafc0d1ULL,
        0x6b7c8d9eafc0d1e2ULL,
        0x7c8d9eafc0d1e2f3ULL,
        0x8d9eafc0d1e2f304ULL,
    };
    const uint64_t filler_third_seeds[] = {
        0x9eafc0d1e2f30415ULL,
        0xafc0d1e2f3041526ULL,
        0xc0d1e2f304152637ULL,
        0xd1e2f30415263748ULL,
    };
    const uint64_t filler_fourth_seeds[] = {
        0xe2f3041526374859ULL,
        0xf30415263748596aULL,
        0x0415263748596a7bULL,
        0x15263748596a7b8cULL,
    };
    bool test_status = true;

    goto_priv(PRIV_HS);
    hspt_init();
    sfence_vma();

    for (unsigned round = 0; round < 4U; ++round) {
        const uint64_t first_before_seed =
            ai_micro_overlap_turnover_pattern(0x1021324354650000ULL, round, 0U);
        const uint64_t first_low_seed =
            ai_micro_overlap_turnover_pattern(0x2132435465760000ULL, round, 1U);
        const uint64_t first_high_seed =
            ai_micro_overlap_turnover_pattern(0x3243546576870000ULL, round, 2U);
        const uint64_t first_after_seed =
            ai_micro_overlap_turnover_pattern(0x4354657687980000ULL, round, 3U);
        const uint64_t second_before_seed =
            ai_micro_overlap_turnover_pattern(0x5465768798a90000ULL, round, 4U);
        const uint64_t second_low_seed =
            ai_micro_overlap_turnover_pattern(0x65768798a9ba0000ULL, round, 5U);
        const uint64_t second_high_seed =
            ai_micro_overlap_turnover_pattern(0x768798a9bacb0000ULL, round, 6U);
        const uint64_t second_after_seed =
            ai_micro_overlap_turnover_pattern(0x8798a9bacbdc0000ULL, round, 7U);
        const uint32_t first_store_value =
            (uint32_t)ai_micro_overlap_turnover_pattern(0x98a9bacbdced0000ULL,
                round, 8U);
        const uint64_t second_store_value =
            ai_micro_overlap_turnover_pattern(0xa9bacbdcedfe0000ULL, round, 9U);
        uint64_t expected_first_low = first_low_seed;
        uint64_t expected_first_high = first_high_seed;
        uint64_t expected_second_low = second_low_seed;
        uint64_t expected_second_high = second_high_seed;
        bool initial_ok;
        bool guards_ok;
        bool backing_ok;

        ai_micro_overlap_apply_split_store_bytes(&expected_first_low,
            &expected_first_high, 5U, 4U, first_store_value);
        ai_micro_overlap_apply_split_store_bytes(&expected_second_low,
            &expected_second_high, 1U, 8U, second_store_value);

        goto_priv(PRIV_M);
        write64(first_before_paddr, first_before_seed);
        write64(first_low_paddr, first_low_seed);
        write64(first_high_paddr, first_high_seed);
        write64(first_after_paddr, first_after_seed);
        write64(second_before_paddr, second_before_seed);
        write64(second_low_paddr, second_low_seed);
        write64(second_high_paddr, second_high_seed);
        write64(second_after_paddr, second_after_seed);
        ai_micro_overlap_write_vector_image(vector_src_paddr,
            vector_src_low[round], vector_src_high[round]);
        ai_micro_overlap_write_vector_image(vector_dst_paddr,
            vector_dst_seed_low[round], vector_dst_seed_high[round]);
        write8(vector_before_paddr, vector_before_sentinel);
        write8(vector_after_paddr, vector_after_sentinel);
        write64(filler_sample0_paddr, 0ULL);
        write64(filler_sample31_paddr, 0ULL);
        write64(filler_sample63_paddr, 0ULL);

        goto_priv(PRIV_HS);
        TEST_SETUP_EXCEPT();
        initial_ok =
            ld(first_before_vaddr) == first_before_seed &&
            ld(first_low_vaddr) == first_low_seed &&
            ld(first_high_vaddr) == first_high_seed &&
            ld(first_after_vaddr) == first_after_seed &&
            ld(second_before_vaddr) == second_before_seed &&
            ld(second_low_vaddr) == second_low_seed &&
            ld(second_high_vaddr) == second_high_seed &&
            ld(second_after_vaddr) == second_after_seed &&
            ld(vector_dst_vaddr) == vector_dst_seed_low[round] &&
            ld(vector_dst_vaddr + 8U) == vector_dst_seed_high[round] &&
            *(volatile uint8_t *)vector_before_vaddr == vector_before_sentinel &&
            *(volatile uint8_t *)vector_after_vaddr == vector_after_sentinel &&
            ld(filler_page_vaddr + 0U * 64U) == 0ULL &&
            ld(filler_page_vaddr + 31U * 64U) == 0ULL &&
            ld(filler_page_vaddr + 63U * 64U) == 0ULL;
        TEST_ASSERT("before the cross16 scalar plus unrelated vector-writeback cross-boundary width-switch probe starts, both boundary pairs, the vector destination, guards, and representative filler lines all read back their seeded values",
            excpt.triggered == false && initial_ok
        );

        TEST_SETUP_EXCEPT();
        sw(first_store_vaddr, first_store_value);
        ai_micro_overlap_dense_store_pressure_window(filler_page_vaddr, 0U, 64U,
            filler_first_seeds[round], filler_second_seeds[round]);
        vle8_to_v6((const uint8_t *)vector_src_vaddr, 16);
        vse8_from_v6((uint8_t *)vector_dst_vaddr, 16);
        sd(second_store_vaddr, second_store_value);
        ai_micro_overlap_dense_store_pressure_window(filler_page_vaddr, 0U, 64U,
            filler_third_seeds[round], filler_fourth_seeds[round]);

        uint64_t observed_first_low = ld(first_low_vaddr);
        uint64_t observed_first_high = ld(first_high_vaddr);
        uint64_t observed_second_low = ld(second_low_vaddr);
        uint64_t observed_second_high = ld(second_high_vaddr);
        uint64_t observed_vector_low = ld(vector_dst_vaddr);
        uint64_t observed_vector_high = ld(vector_dst_vaddr + 8U);
        fence_iorw_iorw();
        TEST_ASSERT("after an older cross16 sw is followed by unrelated vector-store writeback pressure and then a younger cross16 sd on another boundary, each boundary keeps only its own expected split template image, the vector target keeps only its own payload, and filler pressure converges to the latest image",
            excpt.triggered == false &&
            observed_first_low == expected_first_low &&
            observed_first_high == expected_first_high &&
            observed_second_low == expected_second_low &&
            observed_second_high == expected_second_high &&
            observed_vector_low == vector_src_low[round] &&
            observed_vector_high == vector_src_high[round] &&
            ld(filler_page_vaddr + 0U * 64U) ==
                ai_micro_overlap_dense_store_pattern(filler_fourth_seeds[round], 0U) &&
            ld(filler_page_vaddr + 31U * 64U) ==
                ai_micro_overlap_dense_store_pattern(filler_fourth_seeds[round], 31U) &&
            ld(filler_page_vaddr + 63U * 64U) ==
                ai_micro_overlap_dense_store_pattern(filler_fourth_seeds[round], 63U)
        );

        if (check_adjacent) {
            TEST_SETUP_EXCEPT();
            guards_ok =
                ld(first_before_vaddr) == first_before_seed &&
                ld(first_after_vaddr) == first_after_seed &&
                ld(second_before_vaddr) == second_before_seed &&
                ld(second_after_vaddr) == second_after_seed &&
                *(volatile uint8_t *)vector_before_vaddr == vector_before_sentinel &&
                *(volatile uint8_t *)vector_after_vaddr == vector_after_sentinel;
            TEST_ASSERT("the cross16 scalar plus unrelated vector-writeback cross-boundary width-switch probe leaves every boundary guard word and vector adjacent sentinel unchanged",
                excpt.triggered == false && guards_ok
            );
        }

        goto_priv(PRIV_M);
        backing_ok =
            read64(first_before_paddr) == first_before_seed &&
            read64(first_low_paddr) == expected_first_low &&
            read64(first_high_paddr) == expected_first_high &&
            read64(first_after_paddr) == first_after_seed &&
            read64(second_before_paddr) == second_before_seed &&
            read64(second_low_paddr) == expected_second_low &&
            read64(second_high_paddr) == expected_second_high &&
            read64(second_after_paddr) == second_after_seed &&
            read64(vector_dst_paddr) == vector_src_low[round] &&
            read64(vector_dst_paddr + 8U) == vector_src_high[round] &&
            read8(vector_before_paddr) == vector_before_sentinel &&
            read8(vector_after_paddr) == vector_after_sentinel &&
            read64(filler_sample0_paddr) ==
                ai_micro_overlap_dense_store_pattern(filler_fourth_seeds[round], 0U) &&
            read64(filler_sample31_paddr) ==
                ai_micro_overlap_dense_store_pattern(filler_fourth_seeds[round], 31U) &&
            read64(filler_sample63_paddr) ==
                ai_micro_overlap_dense_store_pattern(filler_fourth_seeds[round], 63U);
        TEST_ASSERT("the cross16 scalar plus unrelated vector-writeback cross-boundary width-switch probe leaves backing memory with both correct mixed-width split images, the independent vector payload, unchanged guards, and the latest filler image",
            backing_ok
        );
    }

    goto_priv(PRIV_HS);
    return test_status;
}

static bool ai_micro_overlap_cross16_vector_wb_cross_boundary_narrower_switch_window(
    bool check_adjacent)
{
    const enum test_page target_page = VSRWX_GRWX;
    const enum test_page filler_page = VSRW_GRW;
    const enum test_page vector_src_page = SWITCH1;
    const enum test_page vector_dst_page = SCRATCHPAD;
    const uintptr_t target_page_vaddr = hs_page_base(target_page);
    const uintptr_t filler_page_vaddr = hs_page_base(filler_page);
    const uintptr_t vector_src_vaddr = hs_page_base(vector_src_page) + 0x780U;
    const uintptr_t vector_dst_vaddr = hs_page_base(vector_dst_page) + 0x7c0U;
    const uintptr_t target_page_paddr = phys_page_base(target_page);
    const uintptr_t filler_page_paddr = phys_page_base(filler_page);
    const uintptr_t vector_src_paddr = phys_page_base(vector_src_page) + 0x780U;
    const uintptr_t vector_dst_paddr = phys_page_base(vector_dst_page) + 0x7c0U;
    const uintptr_t first_before_vaddr = target_page_vaddr + 0x100U;
    const uintptr_t first_low_vaddr = target_page_vaddr + 0x108U;
    const uintptr_t first_store_vaddr = target_page_vaddr + 0x109U;
    const uintptr_t first_high_vaddr = target_page_vaddr + 0x110U;
    const uintptr_t first_after_vaddr = target_page_vaddr + 0x118U;
    const uintptr_t second_before_vaddr = target_page_vaddr + 0x180U;
    const uintptr_t second_low_vaddr = target_page_vaddr + 0x188U;
    const uintptr_t second_store_vaddr = target_page_vaddr + 0x18dU;
    const uintptr_t second_high_vaddr = target_page_vaddr + 0x190U;
    const uintptr_t second_after_vaddr = target_page_vaddr + 0x198U;
    const uintptr_t vector_before_vaddr = vector_dst_vaddr - 1U;
    const uintptr_t vector_after_vaddr = vector_dst_vaddr + 16U;
    const uintptr_t first_before_paddr = target_page_paddr + 0x100U;
    const uintptr_t first_low_paddr = target_page_paddr + 0x108U;
    const uintptr_t first_high_paddr = target_page_paddr + 0x110U;
    const uintptr_t first_after_paddr = target_page_paddr + 0x118U;
    const uintptr_t second_before_paddr = target_page_paddr + 0x180U;
    const uintptr_t second_low_paddr = target_page_paddr + 0x188U;
    const uintptr_t second_high_paddr = target_page_paddr + 0x190U;
    const uintptr_t second_after_paddr = target_page_paddr + 0x198U;
    const uintptr_t vector_before_paddr = vector_dst_paddr - 1U;
    const uintptr_t vector_after_paddr = vector_dst_paddr + 16U;
    const uintptr_t filler_sample0_paddr = filler_page_paddr + 0U * 64U;
    const uintptr_t filler_sample31_paddr = filler_page_paddr + 31U * 64U;
    const uintptr_t filler_sample63_paddr = filler_page_paddr + 63U * 64U;
    static const uint64_t vector_src_low[] = {
        0x131517191b1d1f21ULL,
        0x232527292b2d2f31ULL,
        0x333537393b3d3f41ULL,
        0x434547494b4d4f51ULL,
    };
    static const uint64_t vector_src_high[] = {
        0x838587898b8d8f91ULL,
        0x939597999b9d9fa1ULL,
        0xa3a5a7a9abadafb1ULL,
        0xb3b5b7b9bbbdbfc1ULL,
    };
    static const uint64_t vector_dst_seed_low[] = {
        0x1020304050607080ULL,
        0x2030405060708090ULL,
        0x30405060708090a0ULL,
        0x405060708090a0b0ULL,
    };
    static const uint64_t vector_dst_seed_high[] = {
        0x9080706050403020ULL,
        0xa090807060504030ULL,
        0xb0a0908070605040ULL,
        0xc0b0a09080706050ULL,
    };
    const uint8_t vector_before_sentinel = 0x2dU;
    const uint8_t vector_after_sentinel = 0xd2U;
    const uint64_t filler_first_seeds[] = {
        0x18293a4b5c6d7e8fULL,
        0x293a4b5c6d7e8fa0ULL,
        0x3a4b5c6d7e8fa0b1ULL,
        0x4b5c6d7e8fa0b1c2ULL,
    };
    const uint64_t filler_second_seeds[] = {
        0x5c6d7e8fa0b1c2d3ULL,
        0x6d7e8fa0b1c2d3e4ULL,
        0x7e8fa0b1c2d3e4f5ULL,
        0x8fa0b1c2d3e4f506ULL,
    };
    const uint64_t filler_third_seeds[] = {
        0xa0b1c2d3e4f50617ULL,
        0xb1c2d3e4f5061728ULL,
        0xc2d3e4f506172839ULL,
        0xd3e4f5061728394aULL,
    };
    const uint64_t filler_fourth_seeds[] = {
        0xe4f5061728394a5bULL,
        0xf5061728394a5b6cULL,
        0x061728394a5b6c7dULL,
        0x1728394a5b6c7d8eULL,
    };
    bool test_status = true;

    goto_priv(PRIV_HS);
    hspt_init();
    sfence_vma();

    for (unsigned round = 0; round < 4U; ++round) {
        const uint64_t first_before_seed =
            ai_micro_overlap_turnover_pattern(0x93a4b5c6d7e80000ULL, round, 0U);
        const uint64_t first_low_seed =
            ai_micro_overlap_turnover_pattern(0xa4b5c6d7e8f90000ULL, round, 1U);
        const uint64_t first_high_seed =
            ai_micro_overlap_turnover_pattern(0xb5c6d7e8f90a0000ULL, round, 2U);
        const uint64_t first_after_seed =
            ai_micro_overlap_turnover_pattern(0xc6d7e8f90a1b0000ULL, round, 3U);
        const uint64_t second_before_seed =
            ai_micro_overlap_turnover_pattern(0xd7e8f90a1b2c0000ULL, round, 4U);
        const uint64_t second_low_seed =
            ai_micro_overlap_turnover_pattern(0xe8f90a1b2c3d0000ULL, round, 5U);
        const uint64_t second_high_seed =
            ai_micro_overlap_turnover_pattern(0xf90a1b2c3d4e0000ULL, round, 6U);
        const uint64_t second_after_seed =
            ai_micro_overlap_turnover_pattern(0x0a1b2c3d4e5f0000ULL, round, 7U);
        const uint64_t first_store_value =
            ai_micro_overlap_turnover_pattern(0x1b2c3d4e5f600000ULL, round, 8U);
        const uint32_t second_store_value =
            (uint32_t)ai_micro_overlap_turnover_pattern(0x2c3d4e5f60710000ULL,
                round, 9U);
        uint64_t expected_first_low = first_low_seed;
        uint64_t expected_first_high = first_high_seed;
        uint64_t expected_second_low = second_low_seed;
        uint64_t expected_second_high = second_high_seed;
        bool initial_ok;
        bool guards_ok;
        bool backing_ok;

        ai_micro_overlap_apply_split_store_bytes(&expected_first_low,
            &expected_first_high, 1U, 8U, first_store_value);
        ai_micro_overlap_apply_split_store_bytes(&expected_second_low,
            &expected_second_high, 5U, 4U, (uint64_t)second_store_value);

        goto_priv(PRIV_M);
        write64(first_before_paddr, first_before_seed);
        write64(first_low_paddr, first_low_seed);
        write64(first_high_paddr, first_high_seed);
        write64(first_after_paddr, first_after_seed);
        write64(second_before_paddr, second_before_seed);
        write64(second_low_paddr, second_low_seed);
        write64(second_high_paddr, second_high_seed);
        write64(second_after_paddr, second_after_seed);
        ai_micro_overlap_write_vector_image(vector_src_paddr,
            vector_src_low[round], vector_src_high[round]);
        ai_micro_overlap_write_vector_image(vector_dst_paddr,
            vector_dst_seed_low[round], vector_dst_seed_high[round]);
        write8(vector_before_paddr, vector_before_sentinel);
        write8(vector_after_paddr, vector_after_sentinel);
        write64(filler_sample0_paddr, 0ULL);
        write64(filler_sample31_paddr, 0ULL);
        write64(filler_sample63_paddr, 0ULL);

        goto_priv(PRIV_HS);
        TEST_SETUP_EXCEPT();
        initial_ok =
            ld(first_before_vaddr) == first_before_seed &&
            ld(first_low_vaddr) == first_low_seed &&
            ld(first_high_vaddr) == first_high_seed &&
            ld(first_after_vaddr) == first_after_seed &&
            ld(second_before_vaddr) == second_before_seed &&
            ld(second_low_vaddr) == second_low_seed &&
            ld(second_high_vaddr) == second_high_seed &&
            ld(second_after_vaddr) == second_after_seed &&
            ld(vector_dst_vaddr) == vector_dst_seed_low[round] &&
            ld(vector_dst_vaddr + 8U) == vector_dst_seed_high[round] &&
            *(volatile uint8_t *)vector_before_vaddr == vector_before_sentinel &&
            *(volatile uint8_t *)vector_after_vaddr == vector_after_sentinel &&
            ld(filler_page_vaddr + 0U * 64U) == 0ULL &&
            ld(filler_page_vaddr + 31U * 64U) == 0ULL &&
            ld(filler_page_vaddr + 63U * 64U) == 0ULL;
        TEST_ASSERT("before the cross16 scalar plus unrelated vector-writeback cross-boundary narrower-switch probe starts, both boundary pairs, the vector destination, guards, and representative filler lines all read back their seeded values",
            excpt.triggered == false && initial_ok
        );

        TEST_SETUP_EXCEPT();
        sd(first_store_vaddr, first_store_value);
        ai_micro_overlap_dense_store_pressure_window(filler_page_vaddr, 0U, 64U,
            filler_first_seeds[round], filler_second_seeds[round]);
        vle8_to_v6((const uint8_t *)vector_src_vaddr, 16);
        vse8_from_v6((uint8_t *)vector_dst_vaddr, 16);
        sw(second_store_vaddr, second_store_value);
        ai_micro_overlap_dense_store_pressure_window(filler_page_vaddr, 0U, 64U,
            filler_third_seeds[round], filler_fourth_seeds[round]);

        uint64_t observed_first_low = ld(first_low_vaddr);
        uint64_t observed_first_high = ld(first_high_vaddr);
        uint64_t observed_second_low = ld(second_low_vaddr);
        uint64_t observed_second_high = ld(second_high_vaddr);
        uint64_t observed_vector_low = ld(vector_dst_vaddr);
        uint64_t observed_vector_high = ld(vector_dst_vaddr + 8U);
        fence_iorw_iorw();
        TEST_ASSERT("after an older cross16 sd is followed by unrelated vector-store writeback pressure and then a younger cross16 sw on another boundary, boundary A keeps only its own wide split image, boundary B keeps only its own narrow byte-age merge, the vector target keeps only its own payload, and filler pressure converges to the latest image",
            excpt.triggered == false &&
            observed_first_low == expected_first_low &&
            observed_first_high == expected_first_high &&
            observed_second_low == expected_second_low &&
            observed_second_high == expected_second_high &&
            observed_vector_low == vector_src_low[round] &&
            observed_vector_high == vector_src_high[round] &&
            ld(filler_page_vaddr + 0U * 64U) ==
                ai_micro_overlap_dense_store_pattern(filler_fourth_seeds[round], 0U) &&
            ld(filler_page_vaddr + 31U * 64U) ==
                ai_micro_overlap_dense_store_pattern(filler_fourth_seeds[round], 31U) &&
            ld(filler_page_vaddr + 63U * 64U) ==
                ai_micro_overlap_dense_store_pattern(filler_fourth_seeds[round], 63U)
        );

        if (check_adjacent) {
            TEST_SETUP_EXCEPT();
            guards_ok =
                ld(first_before_vaddr) == first_before_seed &&
                ld(first_after_vaddr) == first_after_seed &&
                ld(second_before_vaddr) == second_before_seed &&
                ld(second_after_vaddr) == second_after_seed &&
                *(volatile uint8_t *)vector_before_vaddr == vector_before_sentinel &&
                *(volatile uint8_t *)vector_after_vaddr == vector_after_sentinel;
            TEST_ASSERT("the cross16 scalar plus unrelated vector-writeback cross-boundary narrower-switch probe leaves every boundary guard word and vector adjacent sentinel unchanged",
                excpt.triggered == false && guards_ok
            );
        }

        goto_priv(PRIV_M);
        backing_ok =
            read64(first_before_paddr) == first_before_seed &&
            read64(first_low_paddr) == expected_first_low &&
            read64(first_high_paddr) == expected_first_high &&
            read64(first_after_paddr) == first_after_seed &&
            read64(second_before_paddr) == second_before_seed &&
            read64(second_low_paddr) == expected_second_low &&
            read64(second_high_paddr) == expected_second_high &&
            read64(second_after_paddr) == second_after_seed &&
            read64(vector_dst_paddr) == vector_src_low[round] &&
            read64(vector_dst_paddr + 8U) == vector_src_high[round] &&
            read8(vector_before_paddr) == vector_before_sentinel &&
            read8(vector_after_paddr) == vector_after_sentinel &&
            read64(filler_sample0_paddr) ==
                ai_micro_overlap_dense_store_pattern(filler_fourth_seeds[round], 0U) &&
            read64(filler_sample31_paddr) ==
                ai_micro_overlap_dense_store_pattern(filler_fourth_seeds[round], 31U) &&
            read64(filler_sample63_paddr) ==
                ai_micro_overlap_dense_store_pattern(filler_fourth_seeds[round], 63U);
        TEST_ASSERT("the cross16 scalar plus unrelated vector-writeback cross-boundary narrower-switch probe leaves backing memory with boundary A's wide split image, boundary B's correct narrow byte-age merge, the independent vector payload, unchanged guards, and the latest filler image",
            backing_ok
        );
    }

    goto_priv(PRIV_HS);
    return test_status;
}

static bool ai_micro_overlap_cross16_vector_wb_same_boundary_three_shape_window(
    bool check_adjacent)
{
    const enum test_page target_page = VSRWX_GRWX;
    const enum test_page filler_page = VSRW_GRW;
    const enum test_page vector_src_page = SWITCH1;
    const enum test_page vector_dst_page = SCRATCHPAD;
    const uintptr_t target_page_vaddr = hs_page_base(target_page);
    const uintptr_t filler_page_vaddr = hs_page_base(filler_page);
    const uintptr_t vector_src_vaddr = hs_page_base(vector_src_page) + 0x880U;
    const uintptr_t vector_dst_vaddr = hs_page_base(vector_dst_page) + 0x8c0U;
    const uintptr_t target_page_paddr = phys_page_base(target_page);
    const uintptr_t filler_page_paddr = phys_page_base(filler_page);
    const uintptr_t vector_src_paddr = phys_page_base(vector_src_page) + 0x880U;
    const uintptr_t vector_dst_paddr = phys_page_base(vector_dst_page) + 0x8c0U;
    const uintptr_t before_vaddr = target_page_vaddr + 0x100U;
    const uintptr_t low_vaddr = target_page_vaddr + 0x108U;
    const uintptr_t older_store_vaddr = target_page_vaddr + 0x109U;
    const uintptr_t middle_store_vaddr = target_page_vaddr + 0x10dU;
    const uintptr_t younger_store_vaddr = target_page_vaddr + 0x10fU;
    const uintptr_t high_vaddr = target_page_vaddr + 0x110U;
    const uintptr_t after_vaddr = target_page_vaddr + 0x118U;
    const uintptr_t vector_before_vaddr = vector_dst_vaddr - 1U;
    const uintptr_t vector_after_vaddr = vector_dst_vaddr + 16U;
    const uintptr_t before_paddr = target_page_paddr + 0x100U;
    const uintptr_t low_paddr = target_page_paddr + 0x108U;
    const uintptr_t high_paddr = target_page_paddr + 0x110U;
    const uintptr_t after_paddr = target_page_paddr + 0x118U;
    const uintptr_t vector_before_paddr = vector_dst_paddr - 1U;
    const uintptr_t vector_after_paddr = vector_dst_paddr + 16U;
    const uintptr_t filler_sample0_paddr = filler_page_paddr + 0U * 64U;
    const uintptr_t filler_sample31_paddr = filler_page_paddr + 31U * 64U;
    const uintptr_t filler_sample63_paddr = filler_page_paddr + 63U * 64U;
    static const uint64_t vector_src_low[] = {
        0x1416181a1c1e2022ULL,
        0x2426282a2c2e3032ULL,
        0x3436383a3c3e4042ULL,
        0x4446484a4c4e5052ULL,
    };
    static const uint64_t vector_src_high[] = {
        0x8486888a8c8e9092ULL,
        0x9496989a9c9ea0a2ULL,
        0xa4a6a8aaacaeb0b2ULL,
        0xb4b6b8babcbec0c2ULL,
    };
    static const uint64_t vector_dst_seed_low[] = {
        0x89a7c5e301234567ULL,
        0x9ab8d6f412345678ULL,
        0xabc9e70523456789ULL,
        0xbcdaf8163456789aULL,
    };
    static const uint64_t vector_dst_seed_high[] = {
        0x76543210fedcba98ULL,
        0x876543210fedcba9ULL,
        0x9876543210fedcbaULL,
        0xa9876543210fedcbULL,
    };
    const uint8_t vector_before_sentinel = 0x3cU;
    const uint8_t vector_after_sentinel = 0xc3U;
    const uint64_t filler_first_seeds[] = {
        0x15263748596a7b8cULL,
        0x263748596a7b8c9dULL,
        0x3748596a7b8c9daeULL,
        0x48596a7b8c9daebfULL,
    };
    const uint64_t filler_second_seeds[] = {
        0x596a7b8c9daebfd0ULL,
        0x6a7b8c9daebfd0e1ULL,
        0x7b8c9daebfd0e1f2ULL,
        0x8c9daebfd0e1f304ULL,
    };
    const uint64_t filler_third_seeds[] = {
        0x9daebfd0e1f30415ULL,
        0xaebfd0e1f3041526ULL,
        0xbfd0e1f304152637ULL,
        0xd0e1f30415263748ULL,
    };
    const uint64_t filler_fourth_seeds[] = {
        0xe1f3041526374859ULL,
        0xf30415263748596aULL,
        0x0415263748596a7bULL,
        0x15263748596a7b8cULL,
    };
    const uint64_t filler_fifth_seeds[] = {
        0x2738495a6b7c8d9eULL,
        0x38495a6b7c8d9eafULL,
        0x495a6b7c8d9eafc0ULL,
        0x5a6b7c8d9eafc0d1ULL,
    };
    const uint64_t filler_sixth_seeds[] = {
        0x6b7c8d9eafc0d1e2ULL,
        0x7c8d9eafc0d1e2f3ULL,
        0x8d9eafc0d1e2f304ULL,
        0x9eafc0d1e2f30415ULL,
    };
    bool test_status = true;

    goto_priv(PRIV_HS);
    hspt_init();
    sfence_vma();

    for (unsigned round = 0; round < 4U; ++round) {
        const uint64_t before_seed =
            ai_micro_overlap_turnover_pattern(0x16a27c38495b0000ULL, round, 0U);
        const uint64_t low_seed =
            ai_micro_overlap_turnover_pattern(0x27b38d495a6c0000ULL, round, 1U);
        const uint64_t high_seed =
            ai_micro_overlap_turnover_pattern(0x38c49e5a6b7d0000ULL, round, 2U);
        const uint64_t after_seed =
            ai_micro_overlap_turnover_pattern(0x49d5af6b7c8e0000ULL, round, 3U);
        const uint64_t older_store_value =
            ai_micro_overlap_turnover_pattern(0x5ae6b07c8d9f0000ULL, round, 4U);
        const uint32_t middle_store_value =
            (uint32_t)ai_micro_overlap_turnover_pattern(0x6bf7c18d9eb00000ULL,
                round, 5U);
        const uint16_t younger_store_value =
            (uint16_t)ai_micro_overlap_turnover_pattern(0x7c08d29eafc10000ULL,
                round, 6U);
        uint64_t expected_low = low_seed;
        uint64_t expected_high = high_seed;
        bool initial_ok;
        bool guards_ok;
        bool backing_ok;

        ai_micro_overlap_apply_split_store_bytes(&expected_low, &expected_high,
            1U, 8U, older_store_value);
        ai_micro_overlap_apply_split_store_bytes(&expected_low, &expected_high,
            5U, 4U, (uint64_t)middle_store_value);
        ai_micro_overlap_apply_split_store_bytes(&expected_low, &expected_high,
            7U, 2U, (uint64_t)younger_store_value);

        goto_priv(PRIV_M);
        write64(before_paddr, before_seed);
        write64(low_paddr, low_seed);
        write64(high_paddr, high_seed);
        write64(after_paddr, after_seed);
        ai_micro_overlap_write_vector_image(vector_src_paddr,
            vector_src_low[round], vector_src_high[round]);
        ai_micro_overlap_write_vector_image(vector_dst_paddr,
            vector_dst_seed_low[round], vector_dst_seed_high[round]);
        write8(vector_before_paddr, vector_before_sentinel);
        write8(vector_after_paddr, vector_after_sentinel);
        write64(filler_sample0_paddr, 0ULL);
        write64(filler_sample31_paddr, 0ULL);
        write64(filler_sample63_paddr, 0ULL);

        goto_priv(PRIV_HS);
        TEST_SETUP_EXCEPT();
        initial_ok =
            ld(before_vaddr) == before_seed &&
            ld(low_vaddr) == low_seed &&
            ld(high_vaddr) == high_seed &&
            ld(after_vaddr) == after_seed &&
            ld(vector_dst_vaddr) == vector_dst_seed_low[round] &&
            ld(vector_dst_vaddr + 8U) == vector_dst_seed_high[round] &&
            *(volatile uint8_t *)vector_before_vaddr == vector_before_sentinel &&
            *(volatile uint8_t *)vector_after_vaddr == vector_after_sentinel &&
            ld(filler_page_vaddr + 0U * 64U) == 0ULL &&
            ld(filler_page_vaddr + 31U * 64U) == 0ULL &&
            ld(filler_page_vaddr + 63U * 64U) == 0ULL;
        TEST_ASSERT("before the cross16 scalar plus unrelated vector-writeback same-boundary three-shape refresh probe starts, the split boundary words, vector destination, guards, and representative filler lines all read back their seeded values",
            excpt.triggered == false && initial_ok
        );

        TEST_SETUP_EXCEPT();
        sd(older_store_vaddr, older_store_value);
        ai_micro_overlap_dense_store_pressure_window(filler_page_vaddr, 0U, 64U,
            filler_first_seeds[round], filler_second_seeds[round]);
        vle8_to_v6((const uint8_t *)vector_src_vaddr, 16);
        vse8_from_v6((uint8_t *)vector_dst_vaddr, 16);
        sw(middle_store_vaddr, middle_store_value);
        ai_micro_overlap_dense_store_pressure_window(filler_page_vaddr, 0U, 64U,
            filler_third_seeds[round], filler_fourth_seeds[round]);
        sh(younger_store_vaddr, younger_store_value);
        ai_micro_overlap_dense_store_pressure_window(filler_page_vaddr, 0U, 64U,
            filler_fifth_seeds[round], filler_sixth_seeds[round]);

        uint64_t observed_low = ld(low_vaddr);
        uint64_t observed_high = ld(high_vaddr);
        uint64_t observed_vector_low = ld(vector_dst_vaddr);
        uint64_t observed_vector_high = ld(vector_dst_vaddr + 8U);
        fence_iorw_iorw();
        TEST_ASSERT("after an older cross16 sd is followed by unrelated vector-store writeback pressure and then younger same-boundary sw and sh generations, the final boundary keeps the exact three-generation byte-age image, the vector target keeps only its own payload, and filler pressure converges to the latest image",
            excpt.triggered == false &&
            observed_low == expected_low &&
            observed_high == expected_high &&
            observed_vector_low == vector_src_low[round] &&
            observed_vector_high == vector_src_high[round] &&
            ld(filler_page_vaddr + 0U * 64U) ==
                ai_micro_overlap_dense_store_pattern(filler_sixth_seeds[round], 0U) &&
            ld(filler_page_vaddr + 31U * 64U) ==
                ai_micro_overlap_dense_store_pattern(filler_sixth_seeds[round], 31U) &&
            ld(filler_page_vaddr + 63U * 64U) ==
                ai_micro_overlap_dense_store_pattern(filler_sixth_seeds[round], 63U)
        );

        if (check_adjacent) {
            TEST_SETUP_EXCEPT();
            guards_ok =
                ld(before_vaddr) == before_seed &&
                ld(after_vaddr) == after_seed &&
                *(volatile uint8_t *)vector_before_vaddr == vector_before_sentinel &&
                *(volatile uint8_t *)vector_after_vaddr == vector_after_sentinel;
            TEST_ASSERT("the cross16 scalar plus unrelated vector-writeback same-boundary three-shape refresh probe leaves the outer guard words and vector adjacent sentinels unchanged",
                excpt.triggered == false && guards_ok
            );
        }

        goto_priv(PRIV_M);
        backing_ok =
            read64(before_paddr) == before_seed &&
            read64(low_paddr) == expected_low &&
            read64(high_paddr) == expected_high &&
            read64(after_paddr) == after_seed &&
            read64(vector_dst_paddr) == vector_src_low[round] &&
            read64(vector_dst_paddr + 8U) == vector_src_high[round] &&
            read8(vector_before_paddr) == vector_before_sentinel &&
            read8(vector_after_paddr) == vector_after_sentinel &&
            read64(filler_sample0_paddr) ==
                ai_micro_overlap_dense_store_pattern(filler_sixth_seeds[round], 0U) &&
            read64(filler_sample31_paddr) ==
                ai_micro_overlap_dense_store_pattern(filler_sixth_seeds[round], 31U) &&
            read64(filler_sample63_paddr) ==
                ai_micro_overlap_dense_store_pattern(filler_sixth_seeds[round], 63U);
        TEST_ASSERT("the cross16 scalar plus unrelated vector-writeback same-boundary three-shape refresh probe leaves backing memory with the exact final three-generation split image, the independent vector payload, unchanged guards, and the latest filler image",
            backing_ok
        );
    }

    goto_priv(PRIV_HS);
    return test_status;
}

static bool ai_micro_overlap_cross16_low_half_younger_rewrite_window(
    bool check_adjacent)
{
    const enum test_page target_page = VSRWX_GRWX;
    const enum test_page filler_page = VSRW_GRW;
    const uintptr_t target_page_vaddr = hs_page_base(target_page);
    const uintptr_t filler_page_vaddr = hs_page_base(filler_page);
    const uintptr_t target_page_paddr = phys_page_base(target_page);
    const uintptr_t filler_page_paddr = phys_page_base(filler_page);
    const uintptr_t misalign_before_vaddr = target_page_vaddr + 0x100U;
    const uintptr_t misalign_low_vaddr = target_page_vaddr + 0x108U;
    const uintptr_t misalign_store_vaddr = target_page_vaddr + 0x10fU;
    const uintptr_t misalign_high_vaddr = target_page_vaddr + 0x110U;
    const uintptr_t misalign_after_vaddr = target_page_vaddr + 0x118U;
    const uintptr_t companion_before_vaddr = target_page_vaddr + 0x190U;
    const uintptr_t companion_vaddr = target_page_vaddr + 0x198U;
    const uintptr_t companion_after_vaddr = target_page_vaddr + 0x1a0U;
    const uintptr_t misalign_before_paddr = target_page_paddr + 0x100U;
    const uintptr_t misalign_low_paddr = target_page_paddr + 0x108U;
    const uintptr_t misalign_high_paddr = target_page_paddr + 0x110U;
    const uintptr_t misalign_after_paddr = target_page_paddr + 0x118U;
    const uintptr_t companion_before_paddr = target_page_paddr + 0x190U;
    const uintptr_t companion_paddr = target_page_paddr + 0x198U;
    const uintptr_t companion_after_paddr = target_page_paddr + 0x1a0U;
    const uintptr_t filler_sample0_paddr = filler_page_paddr + 0U * 64U;
    const uintptr_t filler_sample31_paddr = filler_page_paddr + 31U * 64U;
    const uintptr_t filler_sample63_paddr = filler_page_paddr + 63U * 64U;
    const uint64_t filler_first_seeds[] = {
        0x1298e4b36cd05a21ULL,
        0x2379f5a57de16b42ULL,
        0x345a06978ef27c63ULL,
        0x453b17899f038d84ULL,
    };
    const uint64_t filler_second_seeds[] = {
        0x561c287bb0149ea5ULL,
        0x66fd396dc125afc6ULL,
        0x77de4a5fd236c0e7ULL,
        0x88bf5b51e347d208ULL,
    };
    const uint64_t filler_third_seeds[] = {
        0x99a06c43f458e329ULL,
        0xaa817d360569f44aULL,
        0xbb628e28167b056bULL,
        0xcc439f1a278c168cULL,
    };
    const uint64_t filler_fourth_seeds[] = {
        0xdd24b00c389d27adULL,
        0xee05c0fe49ae38ceULL,
        0xfee6d1f05abf49efULL,
        0x0fc7e2e26bd05b10ULL,
    };
    bool test_status = true;

    goto_priv(PRIV_HS);
    hspt_init();
    sfence_vma();

    for (unsigned round = 0; round < 4U; ++round) {
        const uint64_t misalign_before_seed =
            ai_micro_overlap_turnover_pattern(0x1a2b3c4d5e6f0000ULL, round, 0U);
        const uint64_t misalign_low_seed =
            ai_micro_overlap_turnover_pattern(0x2b3c4d5e6f700000ULL, round, 1U);
        const uint64_t misalign_high_seed =
            ai_micro_overlap_turnover_pattern(0x3c4d5e6f70810000ULL, round, 2U);
        const uint64_t misalign_after_seed =
            ai_micro_overlap_turnover_pattern(0x4d5e6f7081920000ULL, round, 3U);
        const uint64_t companion_before_seed =
            ai_micro_overlap_turnover_pattern(0x5e6f708192a30000ULL, round, 4U);
        const uint64_t companion_seed =
            ai_micro_overlap_turnover_pattern(0x6f708192a3b40000ULL, round, 5U);
        const uint64_t companion_after_seed =
            ai_micro_overlap_turnover_pattern(0x708192a3b4c50000ULL, round, 6U);
        const uint64_t misalign_store_value =
            ai_micro_overlap_turnover_pattern(0x8192a3b4c5d60000ULL, round, 7U);
        const uint64_t low_rewrite_value =
            ai_micro_overlap_turnover_pattern(0x92a3b4c5d6e70000ULL, round, 8U);
        const uint64_t companion_value =
            ai_micro_overlap_turnover_pattern(0xa3b4c5d6e7f80000ULL, round, 9U);
        const uint64_t expected_high =
            ai_micro_overlap_cross16_store_high_word(
                misalign_high_seed, misalign_store_value);
        bool initial_ok;
        bool guards_ok;
        bool backing_ok;

        goto_priv(PRIV_M);
        write64(misalign_before_paddr, misalign_before_seed);
        write64(misalign_low_paddr, misalign_low_seed);
        write64(misalign_high_paddr, misalign_high_seed);
        write64(misalign_after_paddr, misalign_after_seed);
        write64(companion_before_paddr, companion_before_seed);
        write64(companion_paddr, companion_seed);
        write64(companion_after_paddr, companion_after_seed);
        write64(filler_sample0_paddr, 0ULL);
        write64(filler_sample31_paddr, 0ULL);
        write64(filler_sample63_paddr, 0ULL);

        goto_priv(PRIV_HS);
        TEST_SETUP_EXCEPT();
        initial_ok =
            ld(misalign_before_vaddr) == misalign_before_seed &&
            ld(misalign_low_vaddr) == misalign_low_seed &&
            ld(misalign_high_vaddr) == misalign_high_seed &&
            ld(misalign_after_vaddr) == misalign_after_seed &&
            ld(companion_before_vaddr) == companion_before_seed &&
            ld(companion_vaddr) == companion_seed &&
            ld(companion_after_vaddr) == companion_after_seed &&
            ld(filler_page_vaddr + 0U * 64U) == 0ULL &&
            ld(filler_page_vaddr + 31U * 64U) == 0ULL &&
            ld(filler_page_vaddr + 63U * 64U) == 0ULL;
        TEST_ASSERT("before the cross16 low-half younger-rewrite probe starts, the split boundary words, companion line, guards, and representative filler lines all read back their seeded values",
            excpt.triggered == false && initial_ok
        );

        TEST_SETUP_EXCEPT();
        sd(misalign_store_vaddr, misalign_store_value);
        ai_micro_overlap_dense_store_pressure_window(filler_page_vaddr, 0U, 64U,
            filler_first_seeds[round], filler_second_seeds[round]);
        *(volatile uint64_t *)misalign_low_vaddr = low_rewrite_value;
        *(volatile uint64_t *)companion_vaddr = companion_value;
        ai_micro_overlap_dense_store_pressure_window(filler_page_vaddr, 0U, 64U,
            filler_third_seeds[round], filler_fourth_seeds[round]);

        uint64_t observed_low = ld(misalign_low_vaddr);
        uint64_t observed_high = ld(misalign_high_vaddr);
        uint64_t observed_companion = ld(companion_vaddr);
        fence_iorw_iorw();
        TEST_ASSERT("after a cross16 store opens the real low-half owner path and a younger same-word aligned rewrite follows, the low word keeps only the younger rewrite, the high word keeps only the older split carry bytes, the companion store keeps its own payload, and filler pressure converges to the latest image",
            excpt.triggered == false &&
            observed_low == low_rewrite_value &&
            observed_high == expected_high &&
            observed_companion == companion_value &&
            ld(filler_page_vaddr + 0U * 64U) ==
                ai_micro_overlap_dense_store_pattern(filler_fourth_seeds[round], 0U) &&
            ld(filler_page_vaddr + 31U * 64U) ==
                ai_micro_overlap_dense_store_pattern(filler_fourth_seeds[round], 31U) &&
            ld(filler_page_vaddr + 63U * 64U) ==
                ai_micro_overlap_dense_store_pattern(filler_fourth_seeds[round], 63U)
        );

        if (check_adjacent) {
            TEST_SETUP_EXCEPT();
            guards_ok =
                ld(misalign_before_vaddr) == misalign_before_seed &&
                ld(misalign_after_vaddr) == misalign_after_seed &&
                ld(companion_before_vaddr) == companion_before_seed &&
                ld(companion_after_vaddr) == companion_after_seed;
            TEST_ASSERT("the cross16 low-half younger-rewrite probe leaves every surrounding guard word unchanged",
                excpt.triggered == false && guards_ok
            );
        }

        goto_priv(PRIV_M);
        backing_ok =
            read64(misalign_before_paddr) == misalign_before_seed &&
            read64(misalign_low_paddr) == low_rewrite_value &&
            read64(misalign_high_paddr) == expected_high &&
            read64(misalign_after_paddr) == misalign_after_seed &&
            read64(companion_before_paddr) == companion_before_seed &&
            read64(companion_paddr) == companion_value &&
            read64(companion_after_paddr) == companion_after_seed &&
            read64(filler_sample0_paddr) ==
                ai_micro_overlap_dense_store_pattern(filler_fourth_seeds[round], 0U) &&
            read64(filler_sample31_paddr) ==
                ai_micro_overlap_dense_store_pattern(filler_fourth_seeds[round], 31U) &&
            read64(filler_sample63_paddr) ==
                ai_micro_overlap_dense_store_pattern(filler_fourth_seeds[round], 63U);
        TEST_ASSERT("the cross16 low-half younger-rewrite probe leaves backing memory with only the younger aligned low-word rewrite, the older split carry bytes in the high word, the companion payload, unchanged guards, and the latest filler image",
            backing_ok
        );
    }

    goto_priv(PRIV_HS);
    return test_status;
}

static bool ai_micro_overlap_consecutive_cross16_boundary_window(
    bool check_adjacent)
{
    const enum test_page target_page = VSRWX_GRWX;
    const enum test_page filler_page = VSRW_GRW;
    const uintptr_t target_page_vaddr = hs_page_base(target_page);
    const uintptr_t filler_page_vaddr = hs_page_base(filler_page);
    const uintptr_t target_page_paddr = phys_page_base(target_page);
    const uintptr_t filler_page_paddr = phys_page_base(filler_page);
    const uintptr_t first_before_vaddr = target_page_vaddr + 0x100U;
    const uintptr_t first_low_vaddr = target_page_vaddr + 0x108U;
    const uintptr_t first_store_vaddr = target_page_vaddr + 0x10fU;
    const uintptr_t first_high_vaddr = target_page_vaddr + 0x110U;
    const uintptr_t first_after_vaddr = target_page_vaddr + 0x118U;
    const uintptr_t second_before_vaddr = target_page_vaddr + 0x180U;
    const uintptr_t second_low_vaddr = target_page_vaddr + 0x188U;
    const uintptr_t second_store_vaddr = target_page_vaddr + 0x18fU;
    const uintptr_t second_high_vaddr = target_page_vaddr + 0x190U;
    const uintptr_t second_after_vaddr = target_page_vaddr + 0x198U;
    const uintptr_t first_before_paddr = target_page_paddr + 0x100U;
    const uintptr_t first_low_paddr = target_page_paddr + 0x108U;
    const uintptr_t first_high_paddr = target_page_paddr + 0x110U;
    const uintptr_t first_after_paddr = target_page_paddr + 0x118U;
    const uintptr_t second_before_paddr = target_page_paddr + 0x180U;
    const uintptr_t second_low_paddr = target_page_paddr + 0x188U;
    const uintptr_t second_high_paddr = target_page_paddr + 0x190U;
    const uintptr_t second_after_paddr = target_page_paddr + 0x198U;
    const uintptr_t filler_sample0_paddr = filler_page_paddr + 0U * 64U;
    const uintptr_t filler_sample31_paddr = filler_page_paddr + 31U * 64U;
    const uintptr_t filler_sample63_paddr = filler_page_paddr + 63U * 64U;
    const uint64_t filler_first_seeds[] = {
        0x13a6f9d24cb05871ULL,
        0x24870ac45dc16992ULL,
        0x35681bb66ed27ab3ULL,
        0x46492ca87fe38bd4ULL,
    };
    const uint64_t filler_second_seeds[] = {
        0x572a3d9a90f49cf5ULL,
        0x680b4e8ca205ae16ULL,
        0x78ec5f7eb316bf37ULL,
        0x89cd7070c427d058ULL,
    };
    const uint64_t filler_third_seeds[] = {
        0x9aae8162d538e179ULL,
        0xab8f9254e649f29aULL,
        0xbc70a346f75b03bbULL,
        0xcd51b438086c14dcULL,
    };
    const uint64_t filler_fourth_seeds[] = {
        0xde32c52a197d25fdULL,
        0xef13d61c2a8e371eULL,
        0xfff4e70e3b9f483fULL,
        0x10d5f8004cb05960ULL,
    };
    bool test_status = true;

    goto_priv(PRIV_HS);
    hspt_init();
    sfence_vma();

    for (unsigned round = 0; round < 4U; ++round) {
        const uint64_t first_before_seed =
            ai_micro_overlap_turnover_pattern(0x2031425364750000ULL, round, 0U);
        const uint64_t first_low_seed =
            ai_micro_overlap_turnover_pattern(0x3142536475860000ULL, round, 1U);
        const uint64_t first_high_seed =
            ai_micro_overlap_turnover_pattern(0x4253647586970000ULL, round, 2U);
        const uint64_t first_after_seed =
            ai_micro_overlap_turnover_pattern(0x5364758697a80000ULL, round, 3U);
        const uint64_t second_before_seed =
            ai_micro_overlap_turnover_pattern(0x64758697a8b90000ULL, round, 4U);
        const uint64_t second_low_seed =
            ai_micro_overlap_turnover_pattern(0x758697a8b9ca0000ULL, round, 5U);
        const uint64_t second_high_seed =
            ai_micro_overlap_turnover_pattern(0x8697a8b9cadb0000ULL, round, 6U);
        const uint64_t second_after_seed =
            ai_micro_overlap_turnover_pattern(0x97a8b9cadbec0000ULL, round, 7U);
        const uint64_t first_store_value =
            ai_micro_overlap_turnover_pattern(0xa8b9cadbecfd0000ULL, round, 8U);
        const uint64_t second_store_value =
            ai_micro_overlap_turnover_pattern(0xb9cadbecfd0e0000ULL, round, 9U);
        const uint64_t expected_first_low =
            ai_micro_overlap_cross16_store_low_word(
                first_low_seed, first_store_value);
        const uint64_t expected_first_high =
            ai_micro_overlap_cross16_store_high_word(
                first_high_seed, first_store_value);
        const uint64_t expected_second_low =
            ai_micro_overlap_cross16_store_low_word(
                second_low_seed, second_store_value);
        const uint64_t expected_second_high =
            ai_micro_overlap_cross16_store_high_word(
                second_high_seed, second_store_value);
        bool initial_ok;
        bool guards_ok;
        bool backing_ok;

        goto_priv(PRIV_M);
        write64(first_before_paddr, first_before_seed);
        write64(first_low_paddr, first_low_seed);
        write64(first_high_paddr, first_high_seed);
        write64(first_after_paddr, first_after_seed);
        write64(second_before_paddr, second_before_seed);
        write64(second_low_paddr, second_low_seed);
        write64(second_high_paddr, second_high_seed);
        write64(second_after_paddr, second_after_seed);
        write64(filler_sample0_paddr, 0ULL);
        write64(filler_sample31_paddr, 0ULL);
        write64(filler_sample63_paddr, 0ULL);

        goto_priv(PRIV_HS);
        TEST_SETUP_EXCEPT();
        initial_ok =
            ld(first_before_vaddr) == first_before_seed &&
            ld(first_low_vaddr) == first_low_seed &&
            ld(first_high_vaddr) == first_high_seed &&
            ld(first_after_vaddr) == first_after_seed &&
            ld(second_before_vaddr) == second_before_seed &&
            ld(second_low_vaddr) == second_low_seed &&
            ld(second_high_vaddr) == second_high_seed &&
            ld(second_after_vaddr) == second_after_seed &&
            ld(filler_page_vaddr + 0U * 64U) == 0ULL &&
            ld(filler_page_vaddr + 31U * 64U) == 0ULL &&
            ld(filler_page_vaddr + 63U * 64U) == 0ULL;
        TEST_ASSERT("before the consecutive cross16 split-store turnover probe starts, both boundary pairs, guards, and representative filler lines all read back their seeded values",
            excpt.triggered == false && initial_ok
        );

        TEST_SETUP_EXCEPT();
        sd(first_store_vaddr, first_store_value);
        ai_micro_overlap_dense_store_pressure_window(filler_page_vaddr, 0U, 64U,
            filler_first_seeds[round], filler_second_seeds[round]);
        sd(second_store_vaddr, second_store_value);
        ai_micro_overlap_dense_store_pressure_window(filler_page_vaddr, 0U, 64U,
            filler_third_seeds[round], filler_fourth_seeds[round]);

        uint64_t observed_first_low = ld(first_low_vaddr);
        uint64_t observed_first_high = ld(first_high_vaddr);
        uint64_t observed_second_low = ld(second_low_vaddr);
        uint64_t observed_second_high = ld(second_high_vaddr);
        fence_iorw_iorw();
        TEST_ASSERT("after two consecutive cross16 stores turn over the two-entry databuffer, each boundary keeps only its own split bytes, no half is swapped across boundaries, and filler pressure converges to the latest image",
            excpt.triggered == false &&
            observed_first_low == expected_first_low &&
            observed_first_high == expected_first_high &&
            observed_second_low == expected_second_low &&
            observed_second_high == expected_second_high &&
            ld(filler_page_vaddr + 0U * 64U) ==
                ai_micro_overlap_dense_store_pattern(filler_fourth_seeds[round], 0U) &&
            ld(filler_page_vaddr + 31U * 64U) ==
                ai_micro_overlap_dense_store_pattern(filler_fourth_seeds[round], 31U) &&
            ld(filler_page_vaddr + 63U * 64U) ==
                ai_micro_overlap_dense_store_pattern(filler_fourth_seeds[round], 63U)
        );

        if (check_adjacent) {
            TEST_SETUP_EXCEPT();
            guards_ok =
                ld(first_before_vaddr) == first_before_seed &&
                ld(first_after_vaddr) == first_after_seed &&
                ld(second_before_vaddr) == second_before_seed &&
                ld(second_after_vaddr) == second_after_seed;
            TEST_ASSERT("the consecutive cross16 split-store turnover probe leaves every surrounding guard word unchanged",
                excpt.triggered == false && guards_ok
            );
        }

        goto_priv(PRIV_M);
        backing_ok =
            read64(first_before_paddr) == first_before_seed &&
            read64(first_low_paddr) == expected_first_low &&
            read64(first_high_paddr) == expected_first_high &&
            read64(first_after_paddr) == first_after_seed &&
            read64(second_before_paddr) == second_before_seed &&
            read64(second_low_paddr) == expected_second_low &&
            read64(second_high_paddr) == expected_second_high &&
            read64(second_after_paddr) == second_after_seed &&
            read64(filler_sample0_paddr) ==
                ai_micro_overlap_dense_store_pattern(filler_fourth_seeds[round], 0U) &&
            read64(filler_sample31_paddr) ==
                ai_micro_overlap_dense_store_pattern(filler_fourth_seeds[round], 31U) &&
            read64(filler_sample63_paddr) ==
                ai_micro_overlap_dense_store_pattern(filler_fourth_seeds[round], 63U);
        TEST_ASSERT("the consecutive cross16 split-store turnover probe leaves backing memory with both correct split boundary images, unchanged guards, and the latest filler image",
            backing_ok
        );
    }

    goto_priv(PRIV_HS);
    return test_status;
}

static bool ai_micro_overlap_consecutive_cross16_same_boundary_window(
    bool check_adjacent)
{
    const enum test_page target_page = VSRWX_GRWX;
    const enum test_page filler_page = VSRW_GRW;
    const uintptr_t target_page_vaddr = hs_page_base(target_page);
    const uintptr_t filler_page_vaddr = hs_page_base(filler_page);
    const uintptr_t target_page_paddr = phys_page_base(target_page);
    const uintptr_t filler_page_paddr = phys_page_base(filler_page);
    const uintptr_t before_vaddr = target_page_vaddr + 0x100U;
    const uintptr_t low_vaddr = target_page_vaddr + 0x108U;
    const uintptr_t store_vaddr = target_page_vaddr + 0x10fU;
    const uintptr_t high_vaddr = target_page_vaddr + 0x110U;
    const uintptr_t after_vaddr = target_page_vaddr + 0x118U;
    const uintptr_t before_paddr = target_page_paddr + 0x100U;
    const uintptr_t low_paddr = target_page_paddr + 0x108U;
    const uintptr_t high_paddr = target_page_paddr + 0x110U;
    const uintptr_t after_paddr = target_page_paddr + 0x118U;
    const uintptr_t filler_sample0_paddr = filler_page_paddr + 0U * 64U;
    const uintptr_t filler_sample31_paddr = filler_page_paddr + 31U * 64U;
    const uintptr_t filler_sample63_paddr = filler_page_paddr + 63U * 64U;
    const uint64_t filler_first_seeds[] = {
        0x14b7c8d95e6f7081ULL,
        0x2598d9cb6f8091a2ULL,
        0x3679eabd8091a2c3ULL,
        0x475afbae91a2b3e4ULL,
    };
    const uint64_t filler_second_seeds[] = {
        0x583c0ca0a2b3c405ULL,
        0x691d1d92b3c4d526ULL,
        0x79fe2e84c4d5e647ULL,
        0x8adf3f76d5e6f768ULL,
    };
    const uint64_t filler_third_seeds[] = {
        0x9bc05068e6f80989ULL,
        0xaca1615af7091aaaULL,
        0xbd82724d081a2bcbULL,
        0xce63833f192b3cecULL,
    };
    const uint64_t filler_fourth_seeds[] = {
        0xdf4494312a3c4d0dULL,
        0xf025a5233b4d5e2eULL,
        0x0106b6154c5e6f4fULL,
        0x11e7c7075d6f8070ULL,
    };
    bool test_status = true;

    goto_priv(PRIV_HS);
    hspt_init();
    sfence_vma();

    for (unsigned round = 0; round < 4U; ++round) {
        const uint64_t before_seed =
            ai_micro_overlap_turnover_pattern(0x2f40516273840000ULL, round, 0U);
        const uint64_t low_seed =
            ai_micro_overlap_turnover_pattern(0x4051627384950000ULL, round, 1U);
        const uint64_t high_seed =
            ai_micro_overlap_turnover_pattern(0x5162738495a60000ULL, round, 2U);
        const uint64_t after_seed =
            ai_micro_overlap_turnover_pattern(0x62738495a6b70000ULL, round, 3U);
        const uint64_t older_store_value =
            ai_micro_overlap_turnover_pattern(0x738495a6b7c80000ULL, round, 4U);
        const uint64_t younger_store_value =
            ai_micro_overlap_turnover_pattern(0x8495a6b7c8d90000ULL, round, 5U);
        const uint64_t expected_low =
            ai_micro_overlap_cross16_store_low_word(
                low_seed, younger_store_value);
        const uint64_t expected_high =
            ai_micro_overlap_cross16_store_high_word(
                high_seed, younger_store_value);
        bool initial_ok;
        bool guards_ok;
        bool backing_ok;

        goto_priv(PRIV_M);
        write64(before_paddr, before_seed);
        write64(low_paddr, low_seed);
        write64(high_paddr, high_seed);
        write64(after_paddr, after_seed);
        write64(filler_sample0_paddr, 0ULL);
        write64(filler_sample31_paddr, 0ULL);
        write64(filler_sample63_paddr, 0ULL);

        goto_priv(PRIV_HS);
        TEST_SETUP_EXCEPT();
        initial_ok =
            ld(before_vaddr) == before_seed &&
            ld(low_vaddr) == low_seed &&
            ld(high_vaddr) == high_seed &&
            ld(after_vaddr) == after_seed &&
            ld(filler_page_vaddr + 0U * 64U) == 0ULL &&
            ld(filler_page_vaddr + 31U * 64U) == 0ULL &&
            ld(filler_page_vaddr + 63U * 64U) == 0ULL;
        TEST_ASSERT("before the same-boundary consecutive cross16 refresh probe starts, the split boundary words, guards, and representative filler lines all read back their seeded values",
            excpt.triggered == false && initial_ok
        );

        TEST_SETUP_EXCEPT();
        sd(store_vaddr, older_store_value);
        ai_micro_overlap_dense_store_pressure_window(filler_page_vaddr, 0U, 64U,
            filler_first_seeds[round], filler_second_seeds[round]);
        sd(store_vaddr, younger_store_value);
        ai_micro_overlap_dense_store_pressure_window(filler_page_vaddr, 0U, 64U,
            filler_third_seeds[round], filler_fourth_seeds[round]);

        uint64_t observed_low = ld(low_vaddr);
        uint64_t observed_high = ld(high_vaddr);
        fence_iorw_iorw();
        TEST_ASSERT("after two consecutive cross16 stores reuse the same boundary, the final low and high words keep only the younger split image and filler pressure converges to the latest image",
            excpt.triggered == false &&
            observed_low == expected_low &&
            observed_high == expected_high &&
            ld(filler_page_vaddr + 0U * 64U) ==
                ai_micro_overlap_dense_store_pattern(filler_fourth_seeds[round], 0U) &&
            ld(filler_page_vaddr + 31U * 64U) ==
                ai_micro_overlap_dense_store_pattern(filler_fourth_seeds[round], 31U) &&
            ld(filler_page_vaddr + 63U * 64U) ==
                ai_micro_overlap_dense_store_pattern(filler_fourth_seeds[round], 63U)
        );

        if (check_adjacent) {
            TEST_SETUP_EXCEPT();
            guards_ok =
                ld(before_vaddr) == before_seed &&
                ld(after_vaddr) == after_seed;
            TEST_ASSERT("the same-boundary consecutive cross16 refresh probe leaves every surrounding guard word unchanged",
                excpt.triggered == false && guards_ok
            );
        }

        goto_priv(PRIV_M);
        backing_ok =
            read64(before_paddr) == before_seed &&
            read64(low_paddr) == expected_low &&
            read64(high_paddr) == expected_high &&
            read64(after_paddr) == after_seed &&
            read64(filler_sample0_paddr) ==
                ai_micro_overlap_dense_store_pattern(filler_fourth_seeds[round], 0U) &&
            read64(filler_sample31_paddr) ==
                ai_micro_overlap_dense_store_pattern(filler_fourth_seeds[round], 31U) &&
            read64(filler_sample63_paddr) ==
                ai_micro_overlap_dense_store_pattern(filler_fourth_seeds[round], 63U);
        TEST_ASSERT("the same-boundary consecutive cross16 refresh probe leaves backing memory with only the younger split image, unchanged guards, and the latest filler image",
            backing_ok
        );
    }

    goto_priv(PRIV_HS);
    return test_status;
}

static bool ai_micro_overlap_cross16_same_boundary_width_switch_window(
    bool check_adjacent)
{
    const enum test_page target_page = VSRWX_GRWX;
    const enum test_page filler_page = VSRW_GRW;
    const uintptr_t target_page_vaddr = hs_page_base(target_page);
    const uintptr_t filler_page_vaddr = hs_page_base(filler_page);
    const uintptr_t target_page_paddr = phys_page_base(target_page);
    const uintptr_t filler_page_paddr = phys_page_base(filler_page);
    const uintptr_t before_vaddr = target_page_vaddr + 0x100U;
    const uintptr_t low_vaddr = target_page_vaddr + 0x108U;
    const uintptr_t older_store_vaddr = target_page_vaddr + 0x10dU;
    const uintptr_t younger_store_vaddr = target_page_vaddr + 0x109U;
    const uintptr_t high_vaddr = target_page_vaddr + 0x110U;
    const uintptr_t after_vaddr = target_page_vaddr + 0x118U;
    const uintptr_t before_paddr = target_page_paddr + 0x100U;
    const uintptr_t low_paddr = target_page_paddr + 0x108U;
    const uintptr_t high_paddr = target_page_paddr + 0x110U;
    const uintptr_t after_paddr = target_page_paddr + 0x118U;
    const uintptr_t filler_sample0_paddr = filler_page_paddr + 0U * 64U;
    const uintptr_t filler_sample31_paddr = filler_page_paddr + 31U * 64U;
    const uintptr_t filler_sample63_paddr = filler_page_paddr + 63U * 64U;
    const uint64_t filler_first_seeds[] = {
        0x15c8d9ea6f708192ULL,
        0x26a9eadc8091a2b3ULL,
        0x378afbcf91a2b3d4ULL,
        0x486c0cc1a2b3c4f5ULL,
    };
    const uint64_t filler_second_seeds[] = {
        0x594d1db3b3c4d516ULL,
        0x6a2e2ea5c4d5e637ULL,
        0x7b0f3f97d5e6f758ULL,
        0x8bf05089e6f80979ULL,
    };
    const uint64_t filler_third_seeds[] = {
        0x9cd1617bf7091a9aULL,
        0xadb2726d081a2bbbULL,
        0xbe93835f192b3cdcULL,
        0xcf7494512a3c4dfdULL,
    };
    const uint64_t filler_fourth_seeds[] = {
        0xe055a5433b4d5e1eULL,
        0xf136b6354c5e6f3fULL,
        0x0217c7275d6f8060ULL,
        0x12f8d8196e809181ULL,
    };
    bool test_status = true;

    goto_priv(PRIV_HS);
    hspt_init();
    sfence_vma();

    for (unsigned round = 0; round < 4U; ++round) {
        const uint64_t before_seed =
            ai_micro_overlap_turnover_pattern(0x3a4b5c6d7e8f0000ULL, round, 0U);
        const uint64_t low_seed =
            ai_micro_overlap_turnover_pattern(0x4b5c6d7e8f900000ULL, round, 1U);
        const uint64_t high_seed =
            ai_micro_overlap_turnover_pattern(0x5c6d7e8f90a10000ULL, round, 2U);
        const uint64_t after_seed =
            ai_micro_overlap_turnover_pattern(0x6d7e8f90a1b20000ULL, round, 3U);
        const uint32_t older_store_value =
            (uint32_t)ai_micro_overlap_turnover_pattern(0x7e8f90a1b2c30000ULL,
                round, 4U);
        const uint64_t younger_store_value =
            ai_micro_overlap_turnover_pattern(0x8f90a1b2c3d40000ULL, round, 5U);
        uint64_t expected_low = low_seed;
        uint64_t expected_high = high_seed;
        bool initial_ok;
        bool guards_ok;
        bool backing_ok;

        ai_micro_overlap_apply_split_store_bytes(&expected_low, &expected_high,
            5U, 4U, (uint64_t)older_store_value);
        ai_micro_overlap_apply_split_store_bytes(&expected_low, &expected_high,
            1U, 8U, younger_store_value);

        goto_priv(PRIV_M);
        write64(before_paddr, before_seed);
        write64(low_paddr, low_seed);
        write64(high_paddr, high_seed);
        write64(after_paddr, after_seed);
        write64(filler_sample0_paddr, 0ULL);
        write64(filler_sample31_paddr, 0ULL);
        write64(filler_sample63_paddr, 0ULL);

        goto_priv(PRIV_HS);
        TEST_SETUP_EXCEPT();
        initial_ok =
            ld(before_vaddr) == before_seed &&
            ld(low_vaddr) == low_seed &&
            ld(high_vaddr) == high_seed &&
            ld(after_vaddr) == after_seed &&
            ld(filler_page_vaddr + 0U * 64U) == 0ULL &&
            ld(filler_page_vaddr + 31U * 64U) == 0ULL &&
            ld(filler_page_vaddr + 63U * 64U) == 0ULL;
        TEST_ASSERT("before the same-boundary cross16 width-switch refresh probe starts, the split boundary words, guards, and representative filler lines all read back their seeded values",
            excpt.triggered == false && initial_ok
        );

        TEST_SETUP_EXCEPT();
        sw(older_store_vaddr, older_store_value);
        ai_micro_overlap_dense_store_pressure_window(filler_page_vaddr, 0U, 64U,
            filler_first_seeds[round], filler_second_seeds[round]);
        sd(younger_store_vaddr, younger_store_value);
        ai_micro_overlap_dense_store_pressure_window(filler_page_vaddr, 0U, 64U,
            filler_third_seeds[round], filler_fourth_seeds[round]);

        uint64_t observed_low = ld(low_vaddr);
        uint64_t observed_high = ld(high_vaddr);
        fence_iorw_iorw();
        TEST_ASSERT("after a same-boundary cross16 sw is followed by a same-boundary cross16 sd with a different split shape, the final boundary keeps only the correct byte-age refresh of the younger template and filler pressure converges to the latest image",
            excpt.triggered == false &&
            observed_low == expected_low &&
            observed_high == expected_high &&
            ld(filler_page_vaddr + 0U * 64U) ==
                ai_micro_overlap_dense_store_pattern(filler_fourth_seeds[round], 0U) &&
            ld(filler_page_vaddr + 31U * 64U) ==
                ai_micro_overlap_dense_store_pattern(filler_fourth_seeds[round], 31U) &&
            ld(filler_page_vaddr + 63U * 64U) ==
                ai_micro_overlap_dense_store_pattern(filler_fourth_seeds[round], 63U)
        );

        if (check_adjacent) {
            TEST_SETUP_EXCEPT();
            guards_ok =
                ld(before_vaddr) == before_seed &&
                ld(after_vaddr) == after_seed;
            TEST_ASSERT("the same-boundary cross16 width-switch refresh probe leaves every surrounding guard word unchanged",
                excpt.triggered == false && guards_ok
            );
        }

        goto_priv(PRIV_M);
        backing_ok =
            read64(before_paddr) == before_seed &&
            read64(low_paddr) == expected_low &&
            read64(high_paddr) == expected_high &&
            read64(after_paddr) == after_seed &&
            read64(filler_sample0_paddr) ==
                ai_micro_overlap_dense_store_pattern(filler_fourth_seeds[round], 0U) &&
            read64(filler_sample31_paddr) ==
                ai_micro_overlap_dense_store_pattern(filler_fourth_seeds[round], 31U) &&
            read64(filler_sample63_paddr) ==
                ai_micro_overlap_dense_store_pattern(filler_fourth_seeds[round], 63U);
        TEST_ASSERT("the same-boundary cross16 width-switch refresh probe leaves backing memory with only the expected mixed-width refreshed split image, unchanged guards, and the latest filler image",
            backing_ok
        );
    }

    goto_priv(PRIV_HS);
    return test_status;
}

static bool ai_micro_overlap_cross16_same_boundary_narrower_switch_window(
    bool check_adjacent)
{
    const enum test_page target_page = VSRWX_GRWX;
    const enum test_page filler_page = VSRW_GRW;
    const uintptr_t target_page_vaddr = hs_page_base(target_page);
    const uintptr_t filler_page_vaddr = hs_page_base(filler_page);
    const uintptr_t target_page_paddr = phys_page_base(target_page);
    const uintptr_t filler_page_paddr = phys_page_base(filler_page);
    const uintptr_t before_vaddr = target_page_vaddr + 0x100U;
    const uintptr_t low_vaddr = target_page_vaddr + 0x108U;
    const uintptr_t older_store_vaddr = target_page_vaddr + 0x109U;
    const uintptr_t younger_store_vaddr = target_page_vaddr + 0x10dU;
    const uintptr_t high_vaddr = target_page_vaddr + 0x110U;
    const uintptr_t after_vaddr = target_page_vaddr + 0x118U;
    const uintptr_t before_paddr = target_page_paddr + 0x100U;
    const uintptr_t low_paddr = target_page_paddr + 0x108U;
    const uintptr_t high_paddr = target_page_paddr + 0x110U;
    const uintptr_t after_paddr = target_page_paddr + 0x118U;
    const uintptr_t filler_sample0_paddr = filler_page_paddr + 0U * 64U;
    const uintptr_t filler_sample31_paddr = filler_page_paddr + 31U * 64U;
    const uintptr_t filler_sample63_paddr = filler_page_paddr + 63U * 64U;
    const uint64_t filler_first_seeds[] = {
        0x2386d94cbf617203ULL,
        0x3477ea3ed0828324ULL,
        0x4568fb30e1a39445ULL,
        0x565a0c22f2c4a566ULL,
    };
    const uint64_t filler_second_seeds[] = {
        0x674b1d1403d5b687ULL,
        0x783c2e0614e6c7a8ULL,
        0x892d3ff825f7d8c9ULL,
        0x9a1f50ea3608e9eaULL,
    };
    const uint64_t filler_third_seeds[] = {
        0xab1061dc4719fb0bULL,
        0xbc0172ce582b0c2cULL,
        0xcdf283c0693c1d4dULL,
        0xdee394b27a4d2e6eULL,
    };
    const uint64_t filler_fourth_seeds[] = {
        0xefd4a5a48b5e3f8fULL,
        0xf0c5b6969c6f40b0ULL,
        0x01b6c788ad8051d1ULL,
        0x12a7d87abe9162f2ULL,
    };
    bool test_status = true;

    goto_priv(PRIV_HS);
    hspt_init();
    sfence_vma();

    for (unsigned round = 0; round < 4U; ++round) {
        const uint64_t before_seed =
            ai_micro_overlap_turnover_pattern(0x93a4b5c6d7e80000ULL, round, 0U);
        const uint64_t low_seed =
            ai_micro_overlap_turnover_pattern(0xa4b5c6d7e8f90000ULL, round, 1U);
        const uint64_t high_seed =
            ai_micro_overlap_turnover_pattern(0xb5c6d7e8f90a0000ULL, round, 2U);
        const uint64_t after_seed =
            ai_micro_overlap_turnover_pattern(0xc6d7e8f90a1b0000ULL, round, 3U);
        const uint64_t older_store_value =
            ai_micro_overlap_turnover_pattern(0xd7e8f90a1b2c0000ULL, round, 4U);
        const uint32_t younger_store_value =
            (uint32_t)ai_micro_overlap_turnover_pattern(0xe8f90a1b2c3d0000ULL,
                round, 5U);
        uint64_t expected_low = low_seed;
        uint64_t expected_high = high_seed;
        bool initial_ok;
        bool guards_ok;
        bool backing_ok;

        ai_micro_overlap_apply_split_store_bytes(&expected_low, &expected_high,
            1U, 8U, older_store_value);
        ai_micro_overlap_apply_split_store_bytes(&expected_low, &expected_high,
            5U, 4U, (uint64_t)younger_store_value);

        goto_priv(PRIV_M);
        write64(before_paddr, before_seed);
        write64(low_paddr, low_seed);
        write64(high_paddr, high_seed);
        write64(after_paddr, after_seed);
        write64(filler_sample0_paddr, 0ULL);
        write64(filler_sample31_paddr, 0ULL);
        write64(filler_sample63_paddr, 0ULL);

        goto_priv(PRIV_HS);
        TEST_SETUP_EXCEPT();
        initial_ok =
            ld(before_vaddr) == before_seed &&
            ld(low_vaddr) == low_seed &&
            ld(high_vaddr) == high_seed &&
            ld(after_vaddr) == after_seed &&
            ld(filler_page_vaddr + 0U * 64U) == 0ULL &&
            ld(filler_page_vaddr + 31U * 64U) == 0ULL &&
            ld(filler_page_vaddr + 63U * 64U) == 0ULL;
        TEST_ASSERT("before the same-boundary cross16 narrower-switch probe starts, the split boundary words, guards, and representative filler lines all read back their seeded values",
            excpt.triggered == false && initial_ok
        );

        TEST_SETUP_EXCEPT();
        sd(older_store_vaddr, older_store_value);
        ai_micro_overlap_dense_store_pressure_window(filler_page_vaddr, 0U, 64U,
            filler_first_seeds[round], filler_second_seeds[round]);
        sw(younger_store_vaddr, younger_store_value);
        ai_micro_overlap_dense_store_pressure_window(filler_page_vaddr, 0U, 64U,
            filler_third_seeds[round], filler_fourth_seeds[round]);

        uint64_t observed_low = ld(low_vaddr);
        uint64_t observed_high = ld(high_vaddr);
        fence_iorw_iorw();
        TEST_ASSERT("after a same-boundary wide cross16 sd is followed by a narrower same-boundary cross16 sw, the final boundary keeps the exact byte-age merge with only the younger narrow lanes overwritten",
            excpt.triggered == false &&
            observed_low == expected_low &&
            observed_high == expected_high &&
            ld(filler_page_vaddr + 0U * 64U) ==
                ai_micro_overlap_dense_store_pattern(filler_fourth_seeds[round], 0U) &&
            ld(filler_page_vaddr + 31U * 64U) ==
                ai_micro_overlap_dense_store_pattern(filler_fourth_seeds[round], 31U) &&
            ld(filler_page_vaddr + 63U * 64U) ==
                ai_micro_overlap_dense_store_pattern(filler_fourth_seeds[round], 63U)
        );

        if (check_adjacent) {
            TEST_SETUP_EXCEPT();
            guards_ok =
                ld(before_vaddr) == before_seed &&
                ld(after_vaddr) == after_seed;
            TEST_ASSERT("the same-boundary cross16 narrower-switch probe leaves every surrounding guard word unchanged",
                excpt.triggered == false && guards_ok
            );
        }

        goto_priv(PRIV_M);
        backing_ok =
            read64(before_paddr) == before_seed &&
            read64(low_paddr) == expected_low &&
            read64(high_paddr) == expected_high &&
            read64(after_paddr) == after_seed &&
            read64(filler_sample0_paddr) ==
                ai_micro_overlap_dense_store_pattern(filler_fourth_seeds[round], 0U) &&
            read64(filler_sample31_paddr) ==
                ai_micro_overlap_dense_store_pattern(filler_fourth_seeds[round], 31U) &&
            read64(filler_sample63_paddr) ==
                ai_micro_overlap_dense_store_pattern(filler_fourth_seeds[round], 63U);
        TEST_ASSERT("the same-boundary cross16 narrower-switch probe leaves backing memory with the expected byte-age merge, unchanged guards, and the latest filler image",
            backing_ok
        );
    }

    goto_priv(PRIV_HS);
    return test_status;
}

static bool ai_micro_overlap_cross16_same_boundary_three_shape_window(
    bool check_adjacent)
{
    const enum test_page target_page = VSRWX_GRWX;
    const enum test_page filler_page = VSRW_GRW;
    const uintptr_t target_page_vaddr = hs_page_base(target_page);
    const uintptr_t filler_page_vaddr = hs_page_base(filler_page);
    const uintptr_t target_page_paddr = phys_page_base(target_page);
    const uintptr_t filler_page_paddr = phys_page_base(filler_page);
    const uintptr_t before_vaddr = target_page_vaddr + 0x100U;
    const uintptr_t low_vaddr = target_page_vaddr + 0x108U;
    const uintptr_t older_store_vaddr = target_page_vaddr + 0x109U;
    const uintptr_t middle_store_vaddr = target_page_vaddr + 0x10dU;
    const uintptr_t younger_store_vaddr = target_page_vaddr + 0x10fU;
    const uintptr_t high_vaddr = target_page_vaddr + 0x110U;
    const uintptr_t after_vaddr = target_page_vaddr + 0x118U;
    const uintptr_t before_paddr = target_page_paddr + 0x100U;
    const uintptr_t low_paddr = target_page_paddr + 0x108U;
    const uintptr_t high_paddr = target_page_paddr + 0x110U;
    const uintptr_t after_paddr = target_page_paddr + 0x118U;
    const uintptr_t filler_sample0_paddr = filler_page_paddr + 0U * 64U;
    const uintptr_t filler_sample31_paddr = filler_page_paddr + 31U * 64U;
    const uintptr_t filler_sample63_paddr = filler_page_paddr + 63U * 64U;
    const uint64_t filler_first_seeds[] = {
        0x1425364758697a8bULL,
        0x25364758697a8b9cULL,
        0x364758697a8b9cadULL,
        0x4758697a8b9cadbeULL,
    };
    const uint64_t filler_second_seeds[] = {
        0x58697a8b9cadbecfULL,
        0x697a8b9cadbecfe0ULL,
        0x7a8b9cadbecfe1f1ULL,
        0x8b9cadbecfe1f203ULL,
    };
    const uint64_t filler_third_seeds[] = {
        0x9cadbecfe1f20314ULL,
        0xadbecfe1f2031425ULL,
        0xbecfe1f203142536ULL,
        0xcfe1f20314253647ULL,
    };
    const uint64_t filler_fourth_seeds[] = {
        0xe1f2031425364758ULL,
        0xf203142536475869ULL,
        0x031425364758697aULL,
        0x1425364758697a8bULL,
    };
    const uint64_t filler_fifth_seeds[] = {
        0x263748596a7b8c9dULL,
        0x3748596a7b8c9daeULL,
        0x48596a7b8c9daebfULL,
        0x596a7b8c9daebfd0ULL,
    };
    const uint64_t filler_sixth_seeds[] = {
        0x6a7b8c9daebfd0e1ULL,
        0x7b8c9daebfd0e1f2ULL,
        0x8c9daebfd0e1f304ULL,
        0x9daebfd0e1f30415ULL,
    };
    bool test_status = true;

    goto_priv(PRIV_HS);
    hspt_init();
    sfence_vma();

    for (unsigned round = 0; round < 4U; ++round) {
        const uint64_t before_seed =
            ai_micro_overlap_turnover_pattern(0x13579bdf24680000ULL, round, 0U);
        const uint64_t low_seed =
            ai_micro_overlap_turnover_pattern(0x2468ace135790000ULL, round, 1U);
        const uint64_t high_seed =
            ai_micro_overlap_turnover_pattern(0x3579bdf2468a0000ULL, round, 2U);
        const uint64_t after_seed =
            ai_micro_overlap_turnover_pattern(0x468ace13579b0000ULL, round, 3U);
        const uint64_t older_store_value =
            ai_micro_overlap_turnover_pattern(0x579bdf2468ac0000ULL, round, 4U);
        const uint32_t middle_store_value =
            (uint32_t)ai_micro_overlap_turnover_pattern(0x68ace13579bd0000ULL,
                round, 5U);
        const uint16_t younger_store_value =
            (uint16_t)ai_micro_overlap_turnover_pattern(0x79bdf2468ace0000ULL,
                round, 6U);
        uint64_t expected_low = low_seed;
        uint64_t expected_high = high_seed;
        bool initial_ok;
        bool guards_ok;
        bool backing_ok;

        ai_micro_overlap_apply_split_store_bytes(&expected_low, &expected_high,
            1U, 8U, older_store_value);
        ai_micro_overlap_apply_split_store_bytes(&expected_low, &expected_high,
            5U, 4U, (uint64_t)middle_store_value);
        ai_micro_overlap_apply_split_store_bytes(&expected_low, &expected_high,
            7U, 2U, (uint64_t)younger_store_value);

        goto_priv(PRIV_M);
        write64(before_paddr, before_seed);
        write64(low_paddr, low_seed);
        write64(high_paddr, high_seed);
        write64(after_paddr, after_seed);
        write64(filler_sample0_paddr, 0ULL);
        write64(filler_sample31_paddr, 0ULL);
        write64(filler_sample63_paddr, 0ULL);

        goto_priv(PRIV_HS);
        TEST_SETUP_EXCEPT();
        initial_ok =
            ld(before_vaddr) == before_seed &&
            ld(low_vaddr) == low_seed &&
            ld(high_vaddr) == high_seed &&
            ld(after_vaddr) == after_seed &&
            ld(filler_page_vaddr + 0U * 64U) == 0ULL &&
            ld(filler_page_vaddr + 31U * 64U) == 0ULL &&
            ld(filler_page_vaddr + 63U * 64U) == 0ULL;
        TEST_ASSERT("before the same-boundary three-shape cross16 refresh probe starts, the split boundary words, guards, and representative filler lines all read back their seeded values",
            excpt.triggered == false && initial_ok
        );

        TEST_SETUP_EXCEPT();
        sd(older_store_vaddr, older_store_value);
        ai_micro_overlap_dense_store_pressure_window(filler_page_vaddr, 0U, 64U,
            filler_first_seeds[round], filler_second_seeds[round]);
        sw(middle_store_vaddr, middle_store_value);
        ai_micro_overlap_dense_store_pressure_window(filler_page_vaddr, 0U, 64U,
            filler_third_seeds[round], filler_fourth_seeds[round]);
        sh(younger_store_vaddr, younger_store_value);
        ai_micro_overlap_dense_store_pressure_window(filler_page_vaddr, 0U, 64U,
            filler_fifth_seeds[round], filler_sixth_seeds[round]);

        uint64_t observed_low = ld(low_vaddr);
        uint64_t observed_high = ld(high_vaddr);
        fence_iorw_iorw();
        TEST_ASSERT("after three same-boundary cross16 stores with alternating split shapes, the final boundary keeps the exact three-generation byte-age image and filler pressure converges to the latest image",
            excpt.triggered == false &&
            observed_low == expected_low &&
            observed_high == expected_high &&
            ld(filler_page_vaddr + 0U * 64U) ==
                ai_micro_overlap_dense_store_pattern(filler_sixth_seeds[round], 0U) &&
            ld(filler_page_vaddr + 31U * 64U) ==
                ai_micro_overlap_dense_store_pattern(filler_sixth_seeds[round], 31U) &&
            ld(filler_page_vaddr + 63U * 64U) ==
                ai_micro_overlap_dense_store_pattern(filler_sixth_seeds[round], 63U)
        );

        if (check_adjacent) {
            TEST_SETUP_EXCEPT();
            guards_ok =
                ld(before_vaddr) == before_seed &&
                ld(after_vaddr) == after_seed;
            TEST_ASSERT("the same-boundary three-shape cross16 refresh probe leaves every surrounding guard word unchanged",
                excpt.triggered == false && guards_ok
            );
        }

        goto_priv(PRIV_M);
        backing_ok =
            read64(before_paddr) == before_seed &&
            read64(low_paddr) == expected_low &&
            read64(high_paddr) == expected_high &&
            read64(after_paddr) == after_seed &&
            read64(filler_sample0_paddr) ==
                ai_micro_overlap_dense_store_pattern(filler_sixth_seeds[round], 0U) &&
            read64(filler_sample31_paddr) ==
                ai_micro_overlap_dense_store_pattern(filler_sixth_seeds[round], 31U) &&
            read64(filler_sample63_paddr) ==
                ai_micro_overlap_dense_store_pattern(filler_sixth_seeds[round], 63U);
        TEST_ASSERT("the same-boundary three-shape cross16 refresh probe leaves backing memory with the exact final three-generation split image, unchanged guards, and the latest filler image",
            backing_ok
        );
    }

    goto_priv(PRIV_HS);
    return test_status;
}

static bool ai_micro_overlap_aligned_pair_then_cross16_turnover_window(
    bool check_adjacent)
{
    const enum test_page split_page = VSRWX_GRWX;
    const enum test_page aligned_page = SWITCH1;
    const enum test_page filler_page = VSRW_GRW;
    const uintptr_t split_page_vaddr = hs_page_base(split_page);
    const uintptr_t aligned_page_vaddr = hs_page_base(aligned_page);
    const uintptr_t filler_page_vaddr = hs_page_base(filler_page);
    const uintptr_t split_page_paddr = phys_page_base(split_page);
    const uintptr_t aligned_page_paddr = phys_page_base(aligned_page);
    const uintptr_t filler_page_paddr = phys_page_base(filler_page);
    const uintptr_t split_before_vaddr = split_page_vaddr + 0x100U;
    const uintptr_t split_low_vaddr = split_page_vaddr + 0x108U;
    const uintptr_t split_store_vaddr = split_page_vaddr + 0x10dU;
    const uintptr_t split_high_vaddr = split_page_vaddr + 0x110U;
    const uintptr_t split_after_vaddr = split_page_vaddr + 0x118U;
    const uintptr_t split_before_paddr = split_page_paddr + 0x100U;
    const uintptr_t split_low_paddr = split_page_paddr + 0x108U;
    const uintptr_t split_high_paddr = split_page_paddr + 0x110U;
    const uintptr_t split_after_paddr = split_page_paddr + 0x118U;
    const uintptr_t aligned0_prev_vaddr = aligned_page_vaddr + 0x0f8U;
    const uintptr_t aligned0_vaddr = aligned_page_vaddr + 0x100U;
    const uintptr_t aligned0_next_vaddr = aligned_page_vaddr + 0x108U;
    const uintptr_t aligned1_prev_vaddr = aligned_page_vaddr + 0x138U;
    const uintptr_t aligned1_vaddr = aligned_page_vaddr + 0x140U;
    const uintptr_t aligned1_next_vaddr = aligned_page_vaddr + 0x148U;
    const uintptr_t aligned0_prev_paddr = aligned_page_paddr + 0x0f8U;
    const uintptr_t aligned0_paddr = aligned_page_paddr + 0x100U;
    const uintptr_t aligned0_next_paddr = aligned_page_paddr + 0x108U;
    const uintptr_t aligned1_prev_paddr = aligned_page_paddr + 0x138U;
    const uintptr_t aligned1_paddr = aligned_page_paddr + 0x140U;
    const uintptr_t aligned1_next_paddr = aligned_page_paddr + 0x148U;
    const uintptr_t filler_sample0_paddr = filler_page_paddr + 0U * 64U;
    const uintptr_t filler_sample31_paddr = filler_page_paddr + 31U * 64U;
    const uintptr_t filler_sample63_paddr = filler_page_paddr + 63U * 64U;
    const uint64_t filler_first_seeds[] = {
        0x31425364758697a8ULL,
        0x425364758697a8b9ULL,
        0x5364758697a8b9caULL,
        0x64758697a8b9cadbULL,
    };
    const uint64_t filler_second_seeds[] = {
        0x758697a8b9cadbecULL,
        0x8697a8b9cadbecfdULL,
        0x97a8b9cadbecfd0eULL,
        0xa8b9cadbecfd0e1fULL,
    };
    const uint64_t filler_third_seeds[] = {
        0xb9cadbecfd0e1f20ULL,
        0xcadbecfd0e1f2031ULL,
        0xdbecfd0e1f203142ULL,
        0xecfd0e1f20314253ULL,
    };
    const uint64_t filler_fourth_seeds[] = {
        0xfd0e1f2031425364ULL,
        0x0e1f203142536475ULL,
        0x1f20314253647586ULL,
        0x2031425364758697ULL,
    };
    bool test_status = true;

    goto_priv(PRIV_HS);
    hspt_init();
    sfence_vma();

    for (unsigned round = 0; round < 4U; ++round) {
        const uint64_t split_before_seed =
            ai_micro_overlap_turnover_pattern(0x4152637485960000ULL, round, 0U);
        const uint64_t split_low_seed =
            ai_micro_overlap_turnover_pattern(0x5263748596a70000ULL, round, 1U);
        const uint64_t split_high_seed =
            ai_micro_overlap_turnover_pattern(0x63748596a7b80000ULL, round, 2U);
        const uint64_t split_after_seed =
            ai_micro_overlap_turnover_pattern(0x748596a7b8c90000ULL, round, 3U);
        const uint64_t aligned0_prev_seed =
            ai_micro_overlap_turnover_pattern(0x8596a7b8c9da0000ULL, round, 4U);
        const uint64_t aligned0_seed =
            ai_micro_overlap_turnover_pattern(0x96a7b8c9daeb0000ULL, round, 5U);
        const uint64_t aligned0_next_seed =
            ai_micro_overlap_turnover_pattern(0xa7b8c9daebfc0000ULL, round, 6U);
        const uint64_t aligned1_prev_seed =
            ai_micro_overlap_turnover_pattern(0xb8c9daebfc0d0000ULL, round, 7U);
        const uint64_t aligned1_seed =
            ai_micro_overlap_turnover_pattern(0xc9daebfc0d1e0000ULL, round, 8U);
        const uint64_t aligned1_next_seed =
            ai_micro_overlap_turnover_pattern(0xdaebfc0d1e2f0000ULL, round, 9U);
        const uint64_t aligned0_store_value =
            ai_micro_overlap_turnover_pattern(0xebfc0d1e2f300000ULL, round, 10U);
        const uint64_t aligned1_store_value =
            ai_micro_overlap_turnover_pattern(0xfc0d1e2f30410000ULL, round, 11U);
        const uint64_t split_store_value =
            ai_micro_overlap_turnover_pattern(0x0d1e2f3041520000ULL, round, 12U);
        uint64_t expected_split_low = split_low_seed;
        uint64_t expected_split_high = split_high_seed;
        bool initial_ok;
        bool guards_ok;
        bool backing_ok;

        ai_micro_overlap_apply_split_store_bytes(&expected_split_low,
            &expected_split_high, 5U, 8U, split_store_value);

        goto_priv(PRIV_M);
        write64(split_before_paddr, split_before_seed);
        write64(split_low_paddr, split_low_seed);
        write64(split_high_paddr, split_high_seed);
        write64(split_after_paddr, split_after_seed);
        write64(aligned0_prev_paddr, aligned0_prev_seed);
        write64(aligned0_paddr, aligned0_seed);
        write64(aligned0_next_paddr, aligned0_next_seed);
        write64(aligned1_prev_paddr, aligned1_prev_seed);
        write64(aligned1_paddr, aligned1_seed);
        write64(aligned1_next_paddr, aligned1_next_seed);
        write64(filler_sample0_paddr, 0ULL);
        write64(filler_sample31_paddr, 0ULL);
        write64(filler_sample63_paddr, 0ULL);

        goto_priv(PRIV_HS);
        TEST_SETUP_EXCEPT();
        initial_ok =
            ld(split_before_vaddr) == split_before_seed &&
            ld(split_low_vaddr) == split_low_seed &&
            ld(split_high_vaddr) == split_high_seed &&
            ld(split_after_vaddr) == split_after_seed &&
            ld(aligned0_prev_vaddr) == aligned0_prev_seed &&
            ld(aligned0_vaddr) == aligned0_seed &&
            ld(aligned0_next_vaddr) == aligned0_next_seed &&
            ld(aligned1_prev_vaddr) == aligned1_prev_seed &&
            ld(aligned1_vaddr) == aligned1_seed &&
            ld(aligned1_next_vaddr) == aligned1_next_seed &&
            ld(filler_page_vaddr + 0U * 64U) == 0ULL &&
            ld(filler_page_vaddr + 31U * 64U) == 0ULL &&
            ld(filler_page_vaddr + 63U * 64U) == 0ULL;
        TEST_ASSERT("before the aligned-pair then cross16 turnover probe starts, the aligned hot words, split boundary words, guards, and representative filler lines all read back their seeded values",
            excpt.triggered == false && initial_ok
        );

        TEST_SETUP_EXCEPT();
        sd(aligned0_vaddr, aligned0_store_value);
        sd(aligned1_vaddr, aligned1_store_value);
        ai_micro_overlap_dense_store_pressure_window(filler_page_vaddr, 0U, 64U,
            filler_first_seeds[round], filler_second_seeds[round]);
        sd(split_store_vaddr, split_store_value);
        ai_micro_overlap_dense_store_pressure_window(filler_page_vaddr, 0U, 64U,
            filler_third_seeds[round], filler_fourth_seeds[round]);

        uint64_t observed_aligned0 = ld(aligned0_vaddr);
        uint64_t observed_aligned1 = ld(aligned1_vaddr);
        uint64_t observed_split_low = ld(split_low_vaddr);
        uint64_t observed_split_high = ld(split_high_vaddr);
        fence_iorw_iorw();
        TEST_ASSERT("after an older aligned two-store batch is followed by a younger cross16 split-store, the aligned batch and the split boundary all keep only their own latest payloads",
            excpt.triggered == false &&
            observed_aligned0 == aligned0_store_value &&
            observed_aligned1 == aligned1_store_value &&
            observed_split_low == expected_split_low &&
            observed_split_high == expected_split_high &&
            ld(filler_page_vaddr + 0U * 64U) ==
                ai_micro_overlap_dense_store_pattern(filler_fourth_seeds[round], 0U) &&
            ld(filler_page_vaddr + 31U * 64U) ==
                ai_micro_overlap_dense_store_pattern(filler_fourth_seeds[round], 31U) &&
            ld(filler_page_vaddr + 63U * 64U) ==
                ai_micro_overlap_dense_store_pattern(filler_fourth_seeds[round], 63U)
        );

        if (check_adjacent) {
            TEST_SETUP_EXCEPT();
            guards_ok =
                ld(split_before_vaddr) == split_before_seed &&
                ld(split_after_vaddr) == split_after_seed &&
                ld(aligned0_prev_vaddr) == aligned0_prev_seed &&
                ld(aligned0_next_vaddr) == aligned0_next_seed &&
                ld(aligned1_prev_vaddr) == aligned1_prev_seed &&
                ld(aligned1_next_vaddr) == aligned1_next_seed;
            TEST_ASSERT("the aligned-pair then cross16 turnover probe leaves every surrounding guard word unchanged",
                excpt.triggered == false && guards_ok
            );
        }

        goto_priv(PRIV_M);
        backing_ok =
            read64(split_before_paddr) == split_before_seed &&
            read64(split_low_paddr) == expected_split_low &&
            read64(split_high_paddr) == expected_split_high &&
            read64(split_after_paddr) == split_after_seed &&
            read64(aligned0_prev_paddr) == aligned0_prev_seed &&
            read64(aligned0_paddr) == aligned0_store_value &&
            read64(aligned0_next_paddr) == aligned0_next_seed &&
            read64(aligned1_prev_paddr) == aligned1_prev_seed &&
            read64(aligned1_paddr) == aligned1_store_value &&
            read64(aligned1_next_paddr) == aligned1_next_seed &&
            read64(filler_sample0_paddr) ==
                ai_micro_overlap_dense_store_pattern(filler_fourth_seeds[round], 0U) &&
            read64(filler_sample31_paddr) ==
                ai_micro_overlap_dense_store_pattern(filler_fourth_seeds[round], 31U) &&
            read64(filler_sample63_paddr) ==
                ai_micro_overlap_dense_store_pattern(filler_fourth_seeds[round], 63U);
        TEST_ASSERT("the aligned-pair then cross16 turnover probe leaves backing memory with the aligned batch intact, the split boundary intact, unchanged guards, and the latest filler image",
            backing_ok
        );
    }

    goto_priv(PRIV_HS);
    return test_status;
}

static bool ai_micro_overlap_same_boundary_aligned_pair_then_cross16_window(
    bool check_adjacent)
{
    const enum test_page target_page = VSRWX_GRWX;
    const enum test_page filler_page = VSRW_GRW;
    const uintptr_t target_page_vaddr = hs_page_base(target_page);
    const uintptr_t filler_page_vaddr = hs_page_base(filler_page);
    const uintptr_t target_page_paddr = phys_page_base(target_page);
    const uintptr_t filler_page_paddr = phys_page_base(filler_page);
    const uintptr_t before_vaddr = target_page_vaddr + 0x100U;
    const uintptr_t low_vaddr = target_page_vaddr + 0x108U;
    const uintptr_t split_store_vaddr = target_page_vaddr + 0x10dU;
    const uintptr_t high_vaddr = target_page_vaddr + 0x110U;
    const uintptr_t after_vaddr = target_page_vaddr + 0x118U;
    const uintptr_t before_paddr = target_page_paddr + 0x100U;
    const uintptr_t low_paddr = target_page_paddr + 0x108U;
    const uintptr_t high_paddr = target_page_paddr + 0x110U;
    const uintptr_t after_paddr = target_page_paddr + 0x118U;
    const uintptr_t filler_sample0_paddr = filler_page_paddr + 0U * 64U;
    const uintptr_t filler_sample31_paddr = filler_page_paddr + 31U * 64U;
    const uintptr_t filler_sample63_paddr = filler_page_paddr + 63U * 64U;
    const uint64_t filler_first_seeds[] = {
        0x415263748596a7b8ULL,
        0x5263748596a7b8c9ULL,
        0x63748596a7b8c9daULL,
        0x748596a7b8c9daebULL,
    };
    const uint64_t filler_second_seeds[] = {
        0x8596a7b8c9daebfcULL,
        0x96a7b8c9daebfc0dULL,
        0xa7b8c9daebfc0d1eULL,
        0xb8c9daebfc0d1e2fULL,
    };
    const uint64_t filler_third_seeds[] = {
        0xc9daebfc0d1e2f30ULL,
        0xdaebfc0d1e2f3041ULL,
        0xebfc0d1e2f304152ULL,
        0xfc0d1e2f30415263ULL,
    };
    const uint64_t filler_fourth_seeds[] = {
        0x0d1e2f3041526374ULL,
        0x1e2f304152637485ULL,
        0x2f30415263748596ULL,
        0x30415263748596a7ULL,
    };
    bool test_status = true;

    goto_priv(PRIV_HS);
    hspt_init();
    sfence_vma();

    for (unsigned round = 0; round < 4U; ++round) {
        const uint64_t before_seed =
            ai_micro_overlap_turnover_pattern(0x15463748596a0000ULL, round, 0U);
        const uint64_t low_seed =
            ai_micro_overlap_turnover_pattern(0x265748596a7b0000ULL, round, 1U);
        const uint64_t high_seed =
            ai_micro_overlap_turnover_pattern(0x3768596a7b8c0000ULL, round, 2U);
        const uint64_t after_seed =
            ai_micro_overlap_turnover_pattern(0x48796a7b8c9d0000ULL, round, 3U);
        const uint64_t older_low_store_value =
            ai_micro_overlap_turnover_pattern(0x598a7b8c9dae0000ULL, round, 4U);
        const uint64_t older_high_store_value =
            ai_micro_overlap_turnover_pattern(0x6a9b8c9daebf0000ULL, round, 5U);
        const uint64_t split_store_value =
            ai_micro_overlap_turnover_pattern(0x7bac9daebfc00000ULL, round, 6U);
        uint64_t expected_low = older_low_store_value;
        uint64_t expected_high = older_high_store_value;
        bool initial_ok;
        bool guards_ok;
        bool backing_ok;

        ai_micro_overlap_apply_split_store_bytes(&expected_low, &expected_high,
            5U, 8U, split_store_value);

        goto_priv(PRIV_M);
        write64(before_paddr, before_seed);
        write64(low_paddr, low_seed);
        write64(high_paddr, high_seed);
        write64(after_paddr, after_seed);
        write64(filler_sample0_paddr, 0ULL);
        write64(filler_sample31_paddr, 0ULL);
        write64(filler_sample63_paddr, 0ULL);

        goto_priv(PRIV_HS);
        TEST_SETUP_EXCEPT();
        initial_ok =
            ld(before_vaddr) == before_seed &&
            ld(low_vaddr) == low_seed &&
            ld(high_vaddr) == high_seed &&
            ld(after_vaddr) == after_seed &&
            ld(filler_page_vaddr + 0U * 64U) == 0ULL &&
            ld(filler_page_vaddr + 31U * 64U) == 0ULL &&
            ld(filler_page_vaddr + 63U * 64U) == 0ULL;
        TEST_ASSERT("before the same-boundary aligned-pair then cross16 probe starts, the boundary words, guards, and representative filler lines all read back their seeded values",
            excpt.triggered == false && initial_ok
        );

        TEST_SETUP_EXCEPT();
        sd(low_vaddr, older_low_store_value);
        sd(high_vaddr, older_high_store_value);
        ai_micro_overlap_dense_store_pressure_window(filler_page_vaddr, 0U, 64U,
            filler_first_seeds[round], filler_second_seeds[round]);
        sd(split_store_vaddr, split_store_value);
        ai_micro_overlap_dense_store_pressure_window(filler_page_vaddr, 0U, 64U,
            filler_third_seeds[round], filler_fourth_seeds[round]);

        uint64_t observed_low = ld(low_vaddr);
        uint64_t observed_high = ld(high_vaddr);
        fence_iorw_iorw();
        TEST_ASSERT("after an older aligned low/high batch is followed by a younger same-boundary cross16 split-store, the boundary keeps the exact byte-age merge while filler pressure converges to the latest image",
            excpt.triggered == false &&
            observed_low == expected_low &&
            observed_high == expected_high &&
            ld(filler_page_vaddr + 0U * 64U) ==
                ai_micro_overlap_dense_store_pattern(filler_fourth_seeds[round], 0U) &&
            ld(filler_page_vaddr + 31U * 64U) ==
                ai_micro_overlap_dense_store_pattern(filler_fourth_seeds[round], 31U) &&
            ld(filler_page_vaddr + 63U * 64U) ==
                ai_micro_overlap_dense_store_pattern(filler_fourth_seeds[round], 63U)
        );

        if (check_adjacent) {
            TEST_SETUP_EXCEPT();
            guards_ok =
                ld(before_vaddr) == before_seed &&
                ld(after_vaddr) == after_seed;
            TEST_ASSERT("the same-boundary aligned-pair then cross16 probe leaves the outer guard words unchanged",
                excpt.triggered == false && guards_ok
            );
        }

        goto_priv(PRIV_M);
        backing_ok =
            read64(before_paddr) == before_seed &&
            read64(low_paddr) == expected_low &&
            read64(high_paddr) == expected_high &&
            read64(after_paddr) == after_seed &&
            read64(filler_sample0_paddr) ==
                ai_micro_overlap_dense_store_pattern(filler_fourth_seeds[round], 0U) &&
            read64(filler_sample31_paddr) ==
                ai_micro_overlap_dense_store_pattern(filler_fourth_seeds[round], 31U) &&
            read64(filler_sample63_paddr) ==
                ai_micro_overlap_dense_store_pattern(filler_fourth_seeds[round], 63U);
        TEST_ASSERT("the same-boundary aligned-pair then cross16 probe leaves backing memory with the exact byte-age merge, unchanged guards, and the latest filler image",
            backing_ok
        );
    }

    goto_priv(PRIV_HS);
    return test_status;
}

static bool ai_micro_overlap_same_boundary_cross16_then_aligned_pair_window(
    bool check_adjacent)
{
    const enum test_page target_page = VSRWX_GRWX;
    const enum test_page filler_page = VSRW_GRW;
    const uintptr_t target_page_vaddr = hs_page_base(target_page);
    const uintptr_t filler_page_vaddr = hs_page_base(filler_page);
    const uintptr_t target_page_paddr = phys_page_base(target_page);
    const uintptr_t filler_page_paddr = phys_page_base(filler_page);
    const uintptr_t before_vaddr = target_page_vaddr + 0x100U;
    const uintptr_t low_vaddr = target_page_vaddr + 0x108U;
    const uintptr_t split_store_vaddr = target_page_vaddr + 0x10dU;
    const uintptr_t high_vaddr = target_page_vaddr + 0x110U;
    const uintptr_t after_vaddr = target_page_vaddr + 0x118U;
    const uintptr_t before_paddr = target_page_paddr + 0x100U;
    const uintptr_t low_paddr = target_page_paddr + 0x108U;
    const uintptr_t high_paddr = target_page_paddr + 0x110U;
    const uintptr_t after_paddr = target_page_paddr + 0x118U;
    const uintptr_t filler_sample0_paddr = filler_page_paddr + 0U * 64U;
    const uintptr_t filler_sample31_paddr = filler_page_paddr + 31U * 64U;
    const uintptr_t filler_sample63_paddr = filler_page_paddr + 63U * 64U;
    const uint64_t filler_first_seeds[] = {
        0x1029384756a1b2c3ULL,
        0x213a495867b2c3d4ULL,
        0x324b5a6978c3d4e5ULL,
        0x435c6b7a89d4e5f6ULL,
    };
    const uint64_t filler_second_seeds[] = {
        0x546d7c8b9ae5f607ULL,
        0x657e8d9cabf60718ULL,
        0x768f9eadbc071829ULL,
        0x8790afbecd18293aULL,
    };
    const uint64_t filler_third_seeds[] = {
        0x98a1b0cfde293a4bULL,
        0xa9b2c1e0ef3a4b5cULL,
        0xbac3d2f1004b5c6dULL,
        0xcbd4e302115c6d7eULL,
    };
    const uint64_t filler_fourth_seeds[] = {
        0xdce5f413226d7e8fULL,
        0xedf60424337e8fa0ULL,
        0xfe071535448fa0b1ULL,
        0x0f1826465590b1c2ULL,
    };
    bool test_status = true;

    goto_priv(PRIV_HS);
    hspt_init();
    sfence_vma();

    for (unsigned round = 0; round < 4U; ++round) {
        const uint64_t before_seed =
            ai_micro_overlap_turnover_pattern(0x18394a5b6c7d0000ULL, round, 0U);
        const uint64_t low_seed =
            ai_micro_overlap_turnover_pattern(0x294a5b6c7d8e0000ULL, round, 1U);
        const uint64_t high_seed =
            ai_micro_overlap_turnover_pattern(0x3a5b6c7d8e9f0000ULL, round, 2U);
        const uint64_t after_seed =
            ai_micro_overlap_turnover_pattern(0x4b6c7d8e9fb00000ULL, round, 3U);
        const uint64_t split_store_value =
            ai_micro_overlap_turnover_pattern(0x5c7d8e9fb0c10000ULL, round, 4U);
        const uint64_t younger_low_store_value =
            ai_micro_overlap_turnover_pattern(0x6d8e9fb0c1d20000ULL, round, 5U);
        const uint64_t younger_high_store_value =
            ai_micro_overlap_turnover_pattern(0x7e9fb0c1d2e30000ULL, round, 6U);
        bool initial_ok;
        bool guards_ok;
        bool backing_ok;

        goto_priv(PRIV_M);
        write64(before_paddr, before_seed);
        write64(low_paddr, low_seed);
        write64(high_paddr, high_seed);
        write64(after_paddr, after_seed);
        write64(filler_sample0_paddr, 0ULL);
        write64(filler_sample31_paddr, 0ULL);
        write64(filler_sample63_paddr, 0ULL);

        goto_priv(PRIV_HS);
        TEST_SETUP_EXCEPT();
        initial_ok =
            ld(before_vaddr) == before_seed &&
            ld(low_vaddr) == low_seed &&
            ld(high_vaddr) == high_seed &&
            ld(after_vaddr) == after_seed &&
            ld(filler_page_vaddr + 0U * 64U) == 0ULL &&
            ld(filler_page_vaddr + 31U * 64U) == 0ULL &&
            ld(filler_page_vaddr + 63U * 64U) == 0ULL;
        TEST_ASSERT("before the same-boundary cross16 then aligned-pair probe starts, the boundary words, guards, and representative filler lines all read back their seeded values",
            excpt.triggered == false && initial_ok
        );

        TEST_SETUP_EXCEPT();
        sd(split_store_vaddr, split_store_value);
        ai_micro_overlap_dense_store_pressure_window(filler_page_vaddr, 0U, 64U,
            filler_first_seeds[round], filler_second_seeds[round]);
        sd(low_vaddr, younger_low_store_value);
        sd(high_vaddr, younger_high_store_value);
        ai_micro_overlap_dense_store_pressure_window(filler_page_vaddr, 0U, 64U,
            filler_third_seeds[round], filler_fourth_seeds[round]);

        uint64_t observed_low = ld(low_vaddr);
        uint64_t observed_high = ld(high_vaddr);
        fence_iorw_iorw();
        TEST_ASSERT("after an older same-boundary cross16 split-store is followed by a younger aligned low/high pair on that boundary, the two final words keep only the younger aligned payloads while filler pressure converges to the latest image",
            excpt.triggered == false &&
            observed_low == younger_low_store_value &&
            observed_high == younger_high_store_value &&
            ld(filler_page_vaddr + 0U * 64U) ==
                ai_micro_overlap_dense_store_pattern(filler_fourth_seeds[round], 0U) &&
            ld(filler_page_vaddr + 31U * 64U) ==
                ai_micro_overlap_dense_store_pattern(filler_fourth_seeds[round], 31U) &&
            ld(filler_page_vaddr + 63U * 64U) ==
                ai_micro_overlap_dense_store_pattern(filler_fourth_seeds[round], 63U)
        );

        if (check_adjacent) {
            TEST_SETUP_EXCEPT();
            guards_ok =
                ld(before_vaddr) == before_seed &&
                ld(after_vaddr) == after_seed;
            TEST_ASSERT("the same-boundary cross16 then aligned-pair probe leaves the outer guard words unchanged",
                excpt.triggered == false && guards_ok
            );
        }

        goto_priv(PRIV_M);
        backing_ok =
            read64(before_paddr) == before_seed &&
            read64(low_paddr) == younger_low_store_value &&
            read64(high_paddr) == younger_high_store_value &&
            read64(after_paddr) == after_seed &&
            read64(filler_sample0_paddr) ==
                ai_micro_overlap_dense_store_pattern(filler_fourth_seeds[round], 0U) &&
            read64(filler_sample31_paddr) ==
                ai_micro_overlap_dense_store_pattern(filler_fourth_seeds[round], 31U) &&
            read64(filler_sample63_paddr) ==
                ai_micro_overlap_dense_store_pattern(filler_fourth_seeds[round], 63U);
        TEST_ASSERT("the same-boundary cross16 then aligned-pair probe leaves backing memory with only the younger aligned pair image, unchanged guards, and the latest filler image",
            backing_ok
        );
    }

    goto_priv(PRIV_HS);
    return test_status;
}

static bool ai_micro_overlap_same_boundary_split_aligned_split_window(
    bool check_adjacent)
{
    const enum test_page target_page = VSRWX_GRWX;
    const enum test_page filler_page = VSRW_GRW;
    const uintptr_t target_page_vaddr = hs_page_base(target_page);
    const uintptr_t filler_page_vaddr = hs_page_base(filler_page);
    const uintptr_t target_page_paddr = phys_page_base(target_page);
    const uintptr_t filler_page_paddr = phys_page_base(filler_page);
    const uintptr_t before_vaddr = target_page_vaddr + 0x100U;
    const uintptr_t low_vaddr = target_page_vaddr + 0x108U;
    const uintptr_t older_split_store_vaddr = target_page_vaddr + 0x10dU;
    const uintptr_t younger_split_store_vaddr = target_page_vaddr + 0x10eU;
    const uintptr_t high_vaddr = target_page_vaddr + 0x110U;
    const uintptr_t after_vaddr = target_page_vaddr + 0x118U;
    const uintptr_t before_paddr = target_page_paddr + 0x100U;
    const uintptr_t low_paddr = target_page_paddr + 0x108U;
    const uintptr_t high_paddr = target_page_paddr + 0x110U;
    const uintptr_t after_paddr = target_page_paddr + 0x118U;
    const uintptr_t filler_sample0_paddr = filler_page_paddr + 0U * 64U;
    const uintptr_t filler_sample31_paddr = filler_page_paddr + 31U * 64U;
    const uintptr_t filler_sample63_paddr = filler_page_paddr + 63U * 64U;
    const uint64_t filler_first_seeds[] = {
        0x1133557799bbddf0ULL,
        0x22446688aaccee01ULL,
        0x33557799bbddff12ULL,
        0x446688aaccee1023ULL,
    };
    const uint64_t filler_second_seeds[] = {
        0x557799bbddff2134ULL,
        0x6688aaccee103245ULL,
        0x7799bbddff214356ULL,
        0x88aaccee10325467ULL,
    };
    const uint64_t filler_third_seeds[] = {
        0x99bbddff21436578ULL,
        0xaaccee1032547689ULL,
        0xbbddff214365879aULL,
        0xccee1032547698abULL,
    };
    const uint64_t filler_fourth_seeds[] = {
        0xddff21436587a9bcULL,
        0xee1032547698bacdULL,
        0xff21436587a9cbdeULL,
        0x1032547698badcefULL,
    };
    bool test_status = true;

    goto_priv(PRIV_HS);
    hspt_init();
    sfence_vma();

    for (unsigned round = 0; round < 4U; ++round) {
        const uint64_t before_seed =
            ai_micro_overlap_turnover_pattern(0x21436587a9cb0000ULL, round, 0U);
        const uint64_t low_seed =
            ai_micro_overlap_turnover_pattern(0x32547698badc0000ULL, round, 1U);
        const uint64_t high_seed =
            ai_micro_overlap_turnover_pattern(0x436587a9cbed0000ULL, round, 2U);
        const uint64_t after_seed =
            ai_micro_overlap_turnover_pattern(0x547698badcfe0000ULL, round, 3U);
        const uint64_t older_split_store_value =
            ai_micro_overlap_turnover_pattern(0x6587a9cbed0f0000ULL, round, 4U);
        const uint64_t middle_low_store_value =
            ai_micro_overlap_turnover_pattern(0x7698badcfe100000ULL, round, 5U);
        const uint64_t middle_high_store_value =
            ai_micro_overlap_turnover_pattern(0x87a9cbed0f210000ULL, round, 6U);
        const uint32_t younger_split_store_value =
            (uint32_t)ai_micro_overlap_turnover_pattern(0x98badcfe10320000ULL,
                round, 7U);
        uint64_t expected_low = middle_low_store_value;
        uint64_t expected_high = middle_high_store_value;
        bool initial_ok;
        bool guards_ok;
        bool backing_ok;

        ai_micro_overlap_apply_split_store_bytes(&expected_low, &expected_high,
            6U, 4U, younger_split_store_value);

        goto_priv(PRIV_M);
        write64(before_paddr, before_seed);
        write64(low_paddr, low_seed);
        write64(high_paddr, high_seed);
        write64(after_paddr, after_seed);
        write64(filler_sample0_paddr, 0ULL);
        write64(filler_sample31_paddr, 0ULL);
        write64(filler_sample63_paddr, 0ULL);

        goto_priv(PRIV_HS);
        TEST_SETUP_EXCEPT();
        initial_ok =
            ld(before_vaddr) == before_seed &&
            ld(low_vaddr) == low_seed &&
            ld(high_vaddr) == high_seed &&
            ld(after_vaddr) == after_seed &&
            ld(filler_page_vaddr + 0U * 64U) == 0ULL &&
            ld(filler_page_vaddr + 31U * 64U) == 0ULL &&
            ld(filler_page_vaddr + 63U * 64U) == 0ULL;
        TEST_ASSERT("before the same-boundary split-aligned-split probe starts, the boundary words, guards, and representative filler lines all read back their seeded values",
            excpt.triggered == false && initial_ok
        );

        TEST_SETUP_EXCEPT();
        sd(older_split_store_vaddr, older_split_store_value);
        ai_micro_overlap_dense_store_pressure_window(filler_page_vaddr, 0U, 64U,
            filler_first_seeds[round], filler_second_seeds[round]);
        sd(low_vaddr, middle_low_store_value);
        sd(high_vaddr, middle_high_store_value);
        sw(younger_split_store_vaddr, younger_split_store_value);
        ai_micro_overlap_dense_store_pressure_window(filler_page_vaddr, 0U, 64U,
            filler_third_seeds[round], filler_fourth_seeds[round]);

        uint64_t observed_low = ld(low_vaddr);
        uint64_t observed_high = ld(high_vaddr);
        fence_iorw_iorw();
        TEST_ASSERT("after an older same-boundary split-store, a middle aligned low/high refresh, and a younger narrower split-store on that boundary, the final words keep only the middle-plus-younger byte-age image while filler pressure converges to the latest image",
            excpt.triggered == false &&
            observed_low == expected_low &&
            observed_high == expected_high &&
            ld(filler_page_vaddr + 0U * 64U) ==
                ai_micro_overlap_dense_store_pattern(filler_fourth_seeds[round], 0U) &&
            ld(filler_page_vaddr + 31U * 64U) ==
                ai_micro_overlap_dense_store_pattern(filler_fourth_seeds[round], 31U) &&
            ld(filler_page_vaddr + 63U * 64U) ==
                ai_micro_overlap_dense_store_pattern(filler_fourth_seeds[round], 63U)
        );

        if (check_adjacent) {
            TEST_SETUP_EXCEPT();
            guards_ok =
                ld(before_vaddr) == before_seed &&
                ld(after_vaddr) == after_seed;
            TEST_ASSERT("the same-boundary split-aligned-split probe leaves the outer guard words unchanged",
                excpt.triggered == false && guards_ok
            );
        }

        goto_priv(PRIV_M);
        backing_ok =
            read64(before_paddr) == before_seed &&
            read64(low_paddr) == expected_low &&
            read64(high_paddr) == expected_high &&
            read64(after_paddr) == after_seed &&
            read64(filler_sample0_paddr) ==
                ai_micro_overlap_dense_store_pattern(filler_fourth_seeds[round], 0U) &&
            read64(filler_sample31_paddr) ==
                ai_micro_overlap_dense_store_pattern(filler_fourth_seeds[round], 31U) &&
            read64(filler_sample63_paddr) ==
                ai_micro_overlap_dense_store_pattern(filler_fourth_seeds[round], 63U);
        TEST_ASSERT("the same-boundary split-aligned-split probe leaves backing memory with only the expected middle-plus-younger byte-age image, unchanged guards, and the latest filler image",
            backing_ok
        );
    }

    goto_priv(PRIV_HS);
    return test_status;
}

static bool ai_micro_overlap_same_boundary_aligned_split_aligned_window(
    bool check_adjacent)
{
    const enum test_page target_page = VSRWX_GRWX;
    const enum test_page filler_page = VSRW_GRW;
    const uintptr_t target_page_vaddr = hs_page_base(target_page);
    const uintptr_t filler_page_vaddr = hs_page_base(filler_page);
    const uintptr_t target_page_paddr = phys_page_base(target_page);
    const uintptr_t filler_page_paddr = phys_page_base(filler_page);
    const uintptr_t before_vaddr = target_page_vaddr + 0x100U;
    const uintptr_t low_vaddr = target_page_vaddr + 0x108U;
    const uintptr_t split_store_vaddr = target_page_vaddr + 0x10dU;
    const uintptr_t high_vaddr = target_page_vaddr + 0x110U;
    const uintptr_t after_vaddr = target_page_vaddr + 0x118U;
    const uintptr_t before_paddr = target_page_paddr + 0x100U;
    const uintptr_t low_paddr = target_page_paddr + 0x108U;
    const uintptr_t high_paddr = target_page_paddr + 0x110U;
    const uintptr_t after_paddr = target_page_paddr + 0x118U;
    const uintptr_t filler_sample0_paddr = filler_page_paddr + 0U * 64U;
    const uintptr_t filler_sample31_paddr = filler_page_paddr + 31U * 64U;
    const uintptr_t filler_sample63_paddr = filler_page_paddr + 63U * 64U;
    const uint64_t filler_first_seeds[] = {
        0x0123456789abcdf0ULL,
        0x123456789abcdef1ULL,
        0x23456789abcdef02ULL,
        0x3456789abcdef013ULL,
    };
    const uint64_t filler_second_seeds[] = {
        0x456789abcdef1245ULL,
        0x56789abcdef12356ULL,
        0x6789abcdef234567ULL,
        0x789abcdef3456789ULL,
    };
    const uint64_t filler_third_seeds[] = {
        0x89abcdef456789abULL,
        0x9abcdef56789abcdULL,
        0xabcdef6789abcde0ULL,
        0xbcdef0789abcde01ULL,
    };
    const uint64_t filler_fourth_seeds[] = {
        0xcdef189abcde0123ULL,
        0xdef29abcde012345ULL,
        0xef03bcde01234567ULL,
        0xf014cdef12345678ULL,
    };
    bool test_status = true;

    goto_priv(PRIV_HS);
    hspt_init();
    sfence_vma();

    for (unsigned round = 0; round < 4U; ++round) {
        const uint64_t before_seed =
            ai_micro_overlap_turnover_pattern(0x1021324354650000ULL, round, 0U);
        const uint64_t low_seed =
            ai_micro_overlap_turnover_pattern(0x2132435465760000ULL, round, 1U);
        const uint64_t high_seed =
            ai_micro_overlap_turnover_pattern(0x3243546576870000ULL, round, 2U);
        const uint64_t after_seed =
            ai_micro_overlap_turnover_pattern(0x4354657687980000ULL, round, 3U);
        const uint64_t older_low_store_value =
            ai_micro_overlap_turnover_pattern(0x5465768798a90000ULL, round, 4U);
        const uint64_t older_high_store_value =
            ai_micro_overlap_turnover_pattern(0x65768798a9ba0000ULL, round, 5U);
        const uint64_t middle_split_store_value =
            ai_micro_overlap_turnover_pattern(0x768798a9bacb0000ULL, round, 6U);
        const uint64_t younger_low_store_value =
            ai_micro_overlap_turnover_pattern(0x8798a9bacbdc0000ULL, round, 7U);
        const uint64_t younger_high_store_value =
            ai_micro_overlap_turnover_pattern(0x98a9bacbdced0000ULL, round, 8U);
        bool initial_ok;
        bool guards_ok;
        bool backing_ok;

        goto_priv(PRIV_M);
        write64(before_paddr, before_seed);
        write64(low_paddr, low_seed);
        write64(high_paddr, high_seed);
        write64(after_paddr, after_seed);
        write64(filler_sample0_paddr, 0ULL);
        write64(filler_sample31_paddr, 0ULL);
        write64(filler_sample63_paddr, 0ULL);

        goto_priv(PRIV_HS);
        TEST_SETUP_EXCEPT();
        initial_ok =
            ld(before_vaddr) == before_seed &&
            ld(low_vaddr) == low_seed &&
            ld(high_vaddr) == high_seed &&
            ld(after_vaddr) == after_seed &&
            ld(filler_page_vaddr + 0U * 64U) == 0ULL &&
            ld(filler_page_vaddr + 31U * 64U) == 0ULL &&
            ld(filler_page_vaddr + 63U * 64U) == 0ULL;
        TEST_ASSERT("before the same-boundary aligned-split-aligned probe starts, the boundary words, guards, and representative filler lines all read back their seeded values",
            excpt.triggered == false && initial_ok
        );

        TEST_SETUP_EXCEPT();
        sd(low_vaddr, older_low_store_value);
        sd(high_vaddr, older_high_store_value);
        ai_micro_overlap_dense_store_pressure_window(filler_page_vaddr, 0U, 64U,
            filler_first_seeds[round], filler_second_seeds[round]);
        sd(split_store_vaddr, middle_split_store_value);
        sd(low_vaddr, younger_low_store_value);
        sd(high_vaddr, younger_high_store_value);
        ai_micro_overlap_dense_store_pressure_window(filler_page_vaddr, 0U, 64U,
            filler_third_seeds[round], filler_fourth_seeds[round]);

        uint64_t observed_low = ld(low_vaddr);
        uint64_t observed_high = ld(high_vaddr);
        fence_iorw_iorw();
        TEST_ASSERT("after an older same-boundary aligned pair, a middle split-store, and a younger aligned pair on that boundary, the final words keep only the younger aligned payloads while filler pressure converges to the latest image",
            excpt.triggered == false &&
            observed_low == younger_low_store_value &&
            observed_high == younger_high_store_value &&
            ld(filler_page_vaddr + 0U * 64U) ==
                ai_micro_overlap_dense_store_pattern(filler_fourth_seeds[round], 0U) &&
            ld(filler_page_vaddr + 31U * 64U) ==
                ai_micro_overlap_dense_store_pattern(filler_fourth_seeds[round], 31U) &&
            ld(filler_page_vaddr + 63U * 64U) ==
                ai_micro_overlap_dense_store_pattern(filler_fourth_seeds[round], 63U)
        );

        if (check_adjacent) {
            TEST_SETUP_EXCEPT();
            guards_ok =
                ld(before_vaddr) == before_seed &&
                ld(after_vaddr) == after_seed;
            TEST_ASSERT("the same-boundary aligned-split-aligned probe leaves the outer guard words unchanged",
                excpt.triggered == false && guards_ok
            );
        }

        goto_priv(PRIV_M);
        backing_ok =
            read64(before_paddr) == before_seed &&
            read64(low_paddr) == younger_low_store_value &&
            read64(high_paddr) == younger_high_store_value &&
            read64(after_paddr) == after_seed &&
            read64(filler_sample0_paddr) ==
                ai_micro_overlap_dense_store_pattern(filler_fourth_seeds[round], 0U) &&
            read64(filler_sample31_paddr) ==
                ai_micro_overlap_dense_store_pattern(filler_fourth_seeds[round], 31U) &&
            read64(filler_sample63_paddr) ==
                ai_micro_overlap_dense_store_pattern(filler_fourth_seeds[round], 63U);
        TEST_ASSERT("the same-boundary aligned-split-aligned probe leaves backing memory with only the younger aligned pair image, unchanged guards, and the latest filler image",
            backing_ok
        );
    }

    goto_priv(PRIV_HS);
    return test_status;
}

#endif /* AI_MICRO_OVERLAP_COMMON_H */
