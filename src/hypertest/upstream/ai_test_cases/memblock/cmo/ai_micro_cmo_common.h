#ifndef AI_MICRO_CMO_COMMON_H
#define AI_MICRO_CMO_COMMON_H

#include <rvh_test.h>
#include <page_tables.h>
#include <stdint.h>

/* Public case prototypes. Definitions live in the partNN sources. */
bool ai_micro_hs_cbom_aligned_no_exception();
bool ai_micro_hs_cmo_subblock_offset_no_exception();
bool ai_micro_hs_cbom_tail_subblock_offset_no_exception_corner();
bool ai_micro_hs_cbom_midblock_tail_offset_both_neighbors_preserved_corner();
bool ai_micro_hs_cbozero_data_zeroed();
bool ai_micro_hs_whole_line_store_and_partial_store_same_line_forward_by_byte_age_correctly_corner();
bool ai_micro_hs_cbo_zero_lineflag_wildcard_no_wrong_forward_to_other_word_in_line_corner();
bool ai_micro_hs_cbozero_storequeue_head_adjacent_block_preserved_corner();
bool ai_micro_hs_cbo_zero_partial_store_small_load_mixed_source_no_select_mixup_corner();
bool ai_micro_hs_cbo_zero_partial_store_small_load_mixed_source_adjacent_preserved_corner();
bool ai_micro_hs_sbuffer_same_block_inflight_valid_mixed_load_forward_by_byte_age_corner();
bool ai_micro_hs_sbuffer_same_block_inflight_valid_mixed_load_forward_by_byte_age_adjacent_preserved_corner();
bool ai_micro_hs_sbuffer_same_block_diff_word_inflight_valid_no_cross_word_forward_corner();
bool ai_micro_hs_sbuffer_same_block_diff_word_inflight_valid_no_cross_word_forward_adjacent_preserved_corner();
bool ai_micro_hs_sbuffer_same_mixed_word_back_to_back_loads_no_stale_query_select_corner();
bool ai_micro_hs_sbuffer_same_mixed_word_back_to_back_loads_no_stale_query_select_adjacent_preserved_corner();
bool ai_micro_hs_sbuffer_same_block_waiting_valid_second_merge_preserves_byte_age_corner();
bool ai_micro_hs_sbuffer_same_block_waiting_valid_second_merge_preserves_byte_age_adjacent_preserved_corner();
bool ai_micro_hs_sbuffer_same_block_waiting_valid_multiword_merge_keeps_all_words_corner();
bool ai_micro_hs_sbuffer_same_block_waiting_valid_multiword_merge_keeps_all_words_adjacent_preserved_corner();
bool ai_micro_hs_sbuffer_reinflight_handoff_then_younger_same_word_merge_preserves_generation_corner();
bool ai_micro_hs_sbuffer_reinflight_handoff_then_younger_same_word_merge_adjacent_preserved_corner();
bool ai_micro_hs_sbuffer_tag_mismatch_decision_stays_on_original_query_epoch_corner();
bool ai_micro_hs_alias_matchinvalid_not_created_or_lost_by_one_cycle_entry_retag_corner();
bool ai_micro_hs_storequeue_cbozero_sbuffer_adjacent_cycle_collision_corner();
bool ai_micro_hs_cbozero_tail_subblock_offset_adjacent_block_preserved_corner();
bool ai_micro_hs_cbozero_midblock_tail_offset_both_neighbors_preserved_corner();
bool ai_micro_hs_cbom_repeated_same_block_idempotent_corner();
bool ai_micro_hs_cbozero_repeated_same_block_idempotent_corner();
bool ai_micro_hs_cbozero_then_cbom_same_block_zero_persists_corner();
bool ai_micro_hs_cbom_pmp_read_access_fault();
bool ai_micro_hs_cbom_pmp_read_access_fault_then_valid_probe_adjacent_block_preserved_corner();
bool ai_micro_hs_cbom_repeated_pmp_read_access_fault_stable_corner();
bool ai_micro_hs_cbom_mmio_access_fault();
bool ai_micro_hs_cbom_mmio_access_fault_then_valid_probe_adjacent_block_preserved_corner();
bool ai_micro_hs_cbom_repeated_mmio_access_fault_stable_corner();
bool ai_micro_hs_cbom_page_fault();
bool ai_micro_hs_cbom_page_fault_then_valid_probe_adjacent_block_preserved_corner();
bool ai_micro_hs_cbom_repeated_page_fault_stable_corner();
bool ai_micro_hs_cbom_tlb_access_fault();
bool ai_micro_hs_cbom_tlb_access_fault_then_valid_probe_adjacent_block_preserved_corner();
bool ai_micro_hs_cbom_repeated_tlb_access_fault_stable_corner();
bool ai_micro_hs_cbozero_page_fault();
bool ai_micro_hs_cbozero_page_fault_then_valid_zero_adjacent_block_preserved_corner();
bool ai_micro_hs_cbozero_repeated_page_fault_stable_corner();
bool ai_micro_hs_multiple_cbo_zero_same_window_no_merged_or_lost_flush_corner();
bool ai_micro_hs_back_to_back_cbo_zero_busy_sbuffer_no_stuck_or_wrong_line_zero_corner();
bool ai_micro_hs_cbo_zero_delayed_sbuffer_fire_not_rebound_to_new_sq_head_corner();
bool ai_micro_hs_sq_head_advance_after_cbo_zero_fire_no_false_or_missed_flush_trigger_corner();
bool ai_micro_hs_cbozero_tlb_access_fault();
bool ai_micro_vs_gstage_cbozero_high_ppn_tval_rs1_manual_corner();
bool ai_micro_hs_cbozero_tlb_access_fault_then_valid_zero_adjacent_block_preserved_corner();
bool ai_micro_hs_cbozero_repeated_tlb_access_fault_stable_corner();
bool ai_micro_hs_cbozero_pmp_write_access_fault();
bool ai_micro_hs_cbozero_pmp_write_access_fault_then_restore_adjacent_block_preserved_corner();
bool ai_micro_hs_cbozero_repeated_pmp_write_access_fault_stable_corner();
bool ai_micro_hs_cbozero_mmio_access_fault();
bool ai_micro_hs_cbozero_mmio_access_fault_then_valid_zero_adjacent_block_preserved_corner();
bool ai_micro_hs_cbozero_repeated_mmio_access_fault_stable_corner();
bool ai_micro_hs_cbom_storequeue_head_data_visible();
bool ai_micro_hs_cbom_storequeue_head_adjacent_block_preserved_corner();
bool ai_micro_hs_storequeue_cbom_sqdeq_not_corrupted_by_sbuffer_overlap_corner();
bool ai_micro_hs_sbuffer_same_block_inflight_then_new_store_ordered_correctly_corner();
bool ai_micro_hs_sbuffer_same_block_inflight_mask_clears_without_stuck_corner();
bool ai_micro_hs_sbuffer_sameblock_inflight_replacement_no_hotline_starvation_corner();
bool ai_micro_hs_sbuffer_sameblock_pressure_no_wrong_victim_or_evict_oscillation_corner();
bool ai_micro_hs_sbuffer_evict_waits_for_same_entry_refill_write_completion_corner();
bool ai_micro_hs_sbuffer_refill_and_evict_same_entry_no_stale_mask_or_data_to_dcache_corner();
bool ai_micro_hs_sbuffer_refill_to_mp_resp_uses_coherent_line_snapshot_corner();
bool ai_micro_hs_sbuffer_same_entry_update_during_refill_to_mp_no_mixed_epoch_data_corner();
bool ai_micro_hs_sbuffer_partial_store_miss_full_overwrite_complement_line_image_corner();
bool ai_micro_hs_sbuffer_evict_valid_payload_stable_under_ready_backpressure_corner();
bool ai_micro_hs_sbuffer_same_entry_update_after_evict_valid_no_mutated_dcache_payload_corner();
bool ai_micro_hs_cmo_last_instruction_page_fault_recovers();
bool ai_micro_hs_cmo_first_instruction_page_fault_recovers();
bool ai_micro_hs_cmo_last_instruction_access_fault_recovers_corner();
bool ai_micro_hs_cmo_first_instruction_access_fault_recovers_corner();
bool ai_micro_hs_cmo_last_instruction_mmio_access_fault_recovers_corner();
bool ai_micro_hs_cmo_first_instruction_mmio_access_fault_recovers_corner();
bool ai_micro_hs_cmo_last_instruction_page_fault_adjacent_block_preserved_corner();
bool ai_micro_hs_cmo_first_instruction_page_fault_adjacent_block_preserved_corner();
bool ai_micro_hs_cmo_last_instruction_access_fault_adjacent_block_preserved_corner();
bool ai_micro_hs_cmo_last_instruction_mmio_access_fault_adjacent_block_preserved_corner();
bool ai_micro_hs_cmo_first_instruction_access_fault_adjacent_block_preserved_corner();
bool ai_micro_hs_cmo_first_instruction_mmio_access_fault_adjacent_block_preserved_corner();
bool ai_micro_hs_sbuffer_hitresp_delayed_mask_clear_reused_entry_no_later_store_drop_corner();
bool ai_micro_hs_sbuffer_hitresp_delayed_mask_clear_reused_entry_no_cross_line_mask_loss_corner();

#define AI_ASSERT_NO_EXCEPT_AND(message, cond) \
    TEST_ASSERT(message, excpt.triggered == false && (cond))

#define AI_MICRO_CMO_CBOM_PAGE    SWITCH1
#define AI_MICRO_CMO_CBOZERO_PAGE SWITCH2
#define AI_MICRO_CMO_ALIAS_PAGE   VSRWX_GRWX
#define AI_MICRO_CMO_MMIO_ALIAS_PAGE   VSRWX_GRWX
#define AI_MICRO_CMO_MMIO_BACKING_PAGE SCRATCHPAD
#define AI_MICRO_CMO_MMIO_MEM_BASE     0x30000000000ULL
#define AI_MICRO_CMO_BLOCK_OFFSET 64U
#define AI_MICRO_CMO_BLOCK_WORDS  8U
#define AI_MICRO_CMO_PMP_TOR_RWX  0x0fULL
#define AI_MICRO_CMO_PMP_TOR_NO_R 0x0eULL
#define AI_MICRO_CMO_PMP_TOR_NO_W 0x0dULL

static inline pte_t *ai_micro_cmo_hs_leaf_entry(enum test_page page)
{
#ifdef sv48
    return &hspt[4][page];
#else
    return &hspt[2][page];
#endif
}

static inline void ai_micro_cmo_map_alias_to_base(enum test_page alias_page,
    enum test_page base_page)
{
    *ai_micro_cmo_hs_leaf_entry(alias_page) =
        (phys_page_base(base_page) >> 2) | PTE_V | PTE_AD | PTE_RWX;
}

static inline void ai_micro_cmo_prepare_hs_env(uint64_t menvcfg_bits)
{
    goto_priv(PRIV_M);
    CSRS(CSR_MENVCFG, menvcfg_bits);
    hspt_init();
    sfence_vma();
}

static inline void ai_micro_cmo_restore_pmp_full_access(void)
{
    goto_priv(PRIV_M);
    CSRW(CSR_PMPCFG0, AI_MICRO_CMO_PMP_TOR_RWX);
    CSRW(CSR_PMPADDR0, (uint64_t)-1);
    CSRW(CSR_PMPADDR1, 0ULL);
    CSRW(CSR_PMPADDR2, 0ULL);
}

static inline void ai_micro_cmo_configure_pmp_window(uintptr_t deny_start,
    uintptr_t deny_end, uint64_t deny_cfg)
{
    goto_priv(PRIV_M);
    CSRW(CSR_PMPCFG0, 0ULL);
    CSRW(CSR_PMPADDR0, deny_start >> 2);
    CSRW(CSR_PMPADDR1, deny_end >> 2);
    CSRW(CSR_PMPADDR2, (uint64_t)-1);
    CSRW(CSR_PMPCFG0, AI_MICRO_CMO_PMP_TOR_RWX | (deny_cfg << 8) |
        (AI_MICRO_CMO_PMP_TOR_RWX << 16));
}

static inline uintptr_t ai_micro_cmo_alias_anchor(enum test_page page,
    uintptr_t target_page_start)
{
    return target_page_start - ((uintptr_t)page * PAGE_SIZE);
}

static inline uintptr_t ai_micro_cmo_mmio_backing_paddr(void)
{
    return phys_page_base_offset(AI_MICRO_CMO_MMIO_BACKING_PAGE,
        AI_MICRO_CMO_MMIO_MEM_BASE);
}

static inline void ai_micro_cmo_reset_mmio_env(uint64_t menvcfg_bits)
{
    goto_priv(PRIV_M);
    CSRC(CSR_MENVCFG, MENVCFG_PBMTE | menvcfg_bits);
    CSRW(CSR_PMAADDR0, 0xFFFFFFFFFFF);
    CSRW(CSR_PMACFG0, 0x6F);

    CSRW(CSR_PMACFG2, 0xF006F0F00000000);
    CSRW(CSR_PMAADDR12, 0x20000000);
    CSRW(CSR_PMAADDR13, 0x120000000);
    CSRW(CSR_PMAADDR14, 0xC000000000);
    CSRW(CSR_PMAADDR15, 0x10000000000);

    CSRW(CSR_PMACFG0, 0x0);
    ai_micro_cmo_restore_pmp_full_access();
    hspt_init();
    sfence_vma();
}

static inline uintptr_t ai_micro_cmo_prepare_mmio_alias(uint64_t menvcfg_bits)
{
    uintptr_t backing_paddr = ai_micro_cmo_mmio_backing_paddr();

    ai_micro_cmo_reset_mmio_env(menvcfg_bits);

    CSRW(CSR_PMAADDR0, backing_paddr >> 2);
    CSRW(CSR_PMAADDR1, (backing_paddr + PAGE_SIZE) >> 2);
    CSRS(CSR_PMACFG0, 1ULL << 8);
    CSRS(CSR_PMACFG0, 1ULL << 9);
    CSRS(CSR_PMACFG0, 1ULL << 10);
    CSRC(CSR_PMACFG0, 1ULL << 14);
    CSRS(CSR_PMACFG0, 1ULL << 11);
    CSRS(CSR_MENVCFG, menvcfg_bits);

    hspt_init();
    hspt_leaf_change_base_paddr(
        AI_MICRO_CMO_MMIO_ALIAS_PAGE,
        ai_micro_cmo_alias_anchor(AI_MICRO_CMO_MMIO_ALIAS_PAGE, backing_paddr));
    sfence_vma();

    return hs_page_base(AI_MICRO_CMO_MMIO_ALIAS_PAGE) + AI_MICRO_CMO_BLOCK_OFFSET;
}

static inline void ai_micro_cmo_reset_env(uint64_t menvcfg_bits)
{
    goto_priv(PRIV_M);
    CSRC(CSR_MENVCFG, menvcfg_bits);
    hspt_init();
    sfence_vma();
}

static inline void ai_micro_cmo_seed_block(uintptr_t paddr, uint64_t seed)
{
    for (size_t i = 0; i < AI_MICRO_CMO_BLOCK_WORDS; ++i) {
        write64(paddr + (i * sizeof(uint64_t)),
            seed + (i * 0x1111111111111111ULL));
    }
}

static inline bool ai_micro_cmo_block_all_zero(uintptr_t paddr)
{
    for (size_t i = 0; i < AI_MICRO_CMO_BLOCK_WORDS; ++i) {
        if (read64(paddr + (i * sizeof(uint64_t))) != 0ULL) {
            return false;
        }
    }

    return true;
}

static inline bool ai_micro_cmo_block_matches_seed(uintptr_t paddr, uint64_t seed)
{
    for (size_t i = 0; i < AI_MICRO_CMO_BLOCK_WORDS; ++i) {
        if (read64(paddr + (i * sizeof(uint64_t))) !=
            (seed + (i * 0x1111111111111111ULL))) {
            return false;
        }
    }

    return true;
}

static inline bool ai_micro_cmo_block_matches_seed_except_word(uintptr_t paddr,
    uint64_t seed, size_t word_idx, uint64_t expected_word)
{
    for (size_t i = 0; i < AI_MICRO_CMO_BLOCK_WORDS; ++i) {
        uint64_t expected = (i == word_idx) ? expected_word :
            (seed + (i * 0x1111111111111111ULL));
        if (read64(paddr + (i * sizeof(uint64_t))) != expected) {
            return false;
        }
    }

    return true;
}

static inline bool ai_micro_cmo_block_matches_seed_except_two_words(
    uintptr_t paddr,
    uint64_t seed,
    size_t first_word_idx,
    uint64_t first_expected_word,
    size_t second_word_idx,
    uint64_t second_expected_word)
{
    for (size_t i = 0; i < AI_MICRO_CMO_BLOCK_WORDS; ++i) {
        uint64_t expected = seed + (i * 0x1111111111111111ULL);

        if (i == first_word_idx) {
            expected = first_expected_word;
        } else if (i == second_word_idx) {
            expected = second_expected_word;
        }

        if (read64(paddr + (i * sizeof(uint64_t))) != expected) {
            return false;
        }
    }

    return true;
}

static inline bool ai_micro_cmo_block_matches_seed_except_three_words(
    uintptr_t paddr,
    uint64_t seed,
    size_t first_word_idx,
    uint64_t first_expected_word,
    size_t second_word_idx,
    uint64_t second_expected_word,
    size_t third_word_idx,
    uint64_t third_expected_word)
{
    for (size_t i = 0; i < AI_MICRO_CMO_BLOCK_WORDS; ++i) {
        uint64_t expected = seed + (i * 0x1111111111111111ULL);

        if (i == first_word_idx) {
            expected = first_expected_word;
        } else if (i == second_word_idx) {
            expected = second_expected_word;
        } else if (i == third_word_idx) {
            expected = third_expected_word;
        }

        if (read64(paddr + (i * sizeof(uint64_t))) != expected) {
            return false;
        }
    }

    return true;
}

static inline uint64_t ai_micro_cmo_masked_store_value(unsigned size,
    uint64_t value)
{
    switch (size) {
    case 1U:
        return value & 0xffULL;
    case 2U:
        return value & 0xffffULL;
    case 4U:
        return value & 0xffffffffULL;
    default:
        return value;
    }
}

static inline uint64_t ai_micro_cmo_expected_word_after_partial_zero_store(
    unsigned start_byte,
    unsigned size,
    uint64_t value)
{
    return ai_micro_cmo_masked_store_value(size, value) << (start_byte * 8U);
}

static inline uint64_t ai_micro_cmo_merge_partial_store_into_word(
    uint64_t base_word,
    unsigned start_byte,
    unsigned size,
    uint64_t value)
{
    uint64_t store_mask = 0ULL;

    switch (size) {
    case 1U:
        store_mask = 0xffULL;
        break;
    case 2U:
        store_mask = 0xffffULL;
        break;
    case 4U:
        store_mask = 0xffffffffULL;
        break;
    default:
        return value;
    }

    store_mask <<= (start_byte * 8U);

    return (base_word & ~store_mask) |
        (ai_micro_cmo_masked_store_value(size, value) << (start_byte * 8U));
}

static inline uint64_t ai_micro_cmo_expected_unsigned_window_from_word(
    uint64_t word,
    unsigned start_byte,
    unsigned size)
{
    uint64_t shifted = word >> (start_byte * 8U);

    switch (size) {
    case 1U:
        return shifted & 0xffULL;
    case 2U:
        return shifted & 0xffffULL;
    case 4U:
        return shifted & 0xffffffffULL;
    default:
        return shifted;
    }
}

static inline uint64_t ai_micro_cmo_expected_signed_window_from_word(
    uint64_t word,
    unsigned start_byte,
    unsigned size)
{
    uint64_t value = ai_micro_cmo_expected_unsigned_window_from_word(
        word, start_byte, size);

    switch (size) {
    case 1U:
        return (uint64_t)(int64_t)(int8_t)(uint8_t)value;
    case 2U:
        return (uint64_t)(int64_t)(int16_t)(uint16_t)value;
    case 4U:
        return (uint64_t)(int64_t)(int32_t)(uint32_t)value;
    default:
        return value;
    }
}

static inline uint64_t ai_micro_cmo_signed_lw_value(uintptr_t addr)
{
    int64_t value = 0;

    asm volatile(
        ".option push\n\t"
        ".option norvc\n\t"
        "lw %0, 0(%1)\n\t"
        ".option pop\n\t"
        : "=r"(value) : "r"(addr) : "memory");

    return (uint64_t)value;
}

static bool ai_micro_cmo_cbo_zero_partial_store_small_load_mixed_source_window(
    bool check_adjacent)
{
    enum {
        AI_MICRO_CMO_MIXED_SMALL_LOAD_CASES = 3,
        AI_MICRO_CMO_MIXED_SMALL_LOAD_ROUNDS = 3
    };
    const uintptr_t block_offsets[AI_MICRO_CMO_MIXED_SMALL_LOAD_CASES] = {
        AI_MICRO_CMO_BLOCK_OFFSET * 5U,
        AI_MICRO_CMO_BLOCK_OFFSET * 7U,
        AI_MICRO_CMO_BLOCK_OFFSET * 9U,
    };
    const unsigned store_offsets[AI_MICRO_CMO_MIXED_SMALL_LOAD_CASES] = {
        6U,
        6U,
        3U,
    };
    const unsigned store_sizes[AI_MICRO_CMO_MIXED_SMALL_LOAD_CASES] = {
        2U,
        2U,
        1U,
    };
    const unsigned load_offsets[AI_MICRO_CMO_MIXED_SMALL_LOAD_CASES] = {
        4U,
        4U,
        2U,
    };
    const unsigned load_sizes[AI_MICRO_CMO_MIXED_SMALL_LOAD_CASES] = {
        4U,
        4U,
        2U,
    };
    const bool load_signed[AI_MICRO_CMO_MIXED_SMALL_LOAD_CASES] = {
        false,
        true,
        false,
    };
    const uint64_t seeds[AI_MICRO_CMO_MIXED_SMALL_LOAD_CASES][AI_MICRO_CMO_MIXED_SMALL_LOAD_ROUNDS] = {
        { 0x1021324354657687ULL, 0x89abcdef01234567ULL, 0x5566778899aabbccULL },
        { 0x0fedcba987654321ULL, 0x1122334455667788ULL, 0xaabbccddeeff0011ULL },
        { 0x3141592653589793ULL, 0x2718281828459045ULL, 0x13579bdf2468ace0ULL },
    };
    const uint64_t store_values[AI_MICRO_CMO_MIXED_SMALL_LOAD_CASES][AI_MICRO_CMO_MIXED_SMALL_LOAD_ROUNDS] = {
        { 0xbeefULL, 0x7a5cULL, 0xd012ULL },
        { 0x80ffULL, 0x9234ULL, 0xf155ULL },
        { 0xa5ULL, 0x7cULL, 0xe1ULL },
    };
    uintptr_t base_vaddr[AI_MICRO_CMO_MIXED_SMALL_LOAD_CASES];
    uintptr_t base_paddr[AI_MICRO_CMO_MIXED_SMALL_LOAD_CASES];
    uintptr_t zero_vaddr[AI_MICRO_CMO_MIXED_SMALL_LOAD_CASES];
    uintptr_t store_vaddr[AI_MICRO_CMO_MIXED_SMALL_LOAD_CASES];
    uintptr_t load_vaddr[AI_MICRO_CMO_MIXED_SMALL_LOAD_CASES];
    uintptr_t leading_paddr[AI_MICRO_CMO_MIXED_SMALL_LOAD_CASES];
    uintptr_t trailing_paddr[AI_MICRO_CMO_MIXED_SMALL_LOAD_CASES];
    bool test_status = true;

    for (unsigned i = 0; i < AI_MICRO_CMO_MIXED_SMALL_LOAD_CASES; ++i) {
        base_vaddr[i] = hs_page_base(AI_MICRO_CMO_CBOZERO_PAGE) + block_offsets[i];
        base_paddr[i] = phys_page_base(AI_MICRO_CMO_CBOZERO_PAGE) + block_offsets[i];
        zero_vaddr[i] = base_vaddr[i] + 24U;
        store_vaddr[i] = base_vaddr[i] + store_offsets[i];
        load_vaddr[i] = base_vaddr[i] + load_offsets[i];
        leading_paddr[i] = base_paddr[i] - sizeof(uint64_t);
        trailing_paddr[i] = base_paddr[i] + (AI_MICRO_CMO_BLOCK_WORDS * sizeof(uint64_t));
    }

    goto_priv(PRIV_M);
    ai_micro_cmo_prepare_hs_env(MENVCFG_CBZE);

    for (unsigned round = 0; round < AI_MICRO_CMO_MIXED_SMALL_LOAD_ROUNDS; ++round) {
        uint64_t warmed[AI_MICRO_CMO_MIXED_SMALL_LOAD_CASES];
        uint64_t observed[AI_MICRO_CMO_MIXED_SMALL_LOAD_CASES];
        uint64_t expected_word0[AI_MICRO_CMO_MIXED_SMALL_LOAD_CASES];
        uint64_t expected_load[AI_MICRO_CMO_MIXED_SMALL_LOAD_CASES];
        uint64_t leading_guard[AI_MICRO_CMO_MIXED_SMALL_LOAD_CASES];
        uint64_t trailing_guard[AI_MICRO_CMO_MIXED_SMALL_LOAD_CASES];

        goto_priv(PRIV_M);
        for (unsigned i = 0; i < AI_MICRO_CMO_MIXED_SMALL_LOAD_CASES; ++i) {
            ai_micro_cmo_seed_block(base_paddr[i], seeds[i][round]);
            leading_guard[i] = 0x9000000000000000ULL |
                ((uint64_t)round << 16) | ((uint64_t)i << 8) | 0x5aULL;
            trailing_guard[i] = 0xa000000000000000ULL |
                ((uint64_t)round << 16) | ((uint64_t)i << 8) | 0xc3ULL;
            write64(leading_paddr[i], leading_guard[i]);
            write64(trailing_paddr[i], trailing_guard[i]);
        }

        goto_priv(PRIV_HS);
        hspt_init();
        sfence_vma();

        TEST_SETUP_EXCEPT();
        for (unsigned i = 0; i < AI_MICRO_CMO_MIXED_SMALL_LOAD_CASES; ++i) {
            warmed[i] = ld(base_vaddr[i]);
        }
        TEST_ASSERT("before the cbo.zero plus younger partial-store mixed-source small-load probe, all target HS cache blocks still warm with their seeded first words",
            excpt.triggered == false &&
            warmed[0] == seeds[0][round] &&
            warmed[1] == seeds[1][round] &&
            warmed[2] == seeds[2][round]
        );

        TEST_SETUP_EXCEPT();
        for (unsigned i = 0; i < AI_MICRO_CMO_MIXED_SMALL_LOAD_CASES; ++i) {
            cbo_zero(zero_vaddr[i]);
            switch (store_sizes[i]) {
            case 1U:
                sb(store_vaddr[i], (uint8_t)store_values[i][round]);
                break;
            case 2U:
                sh(store_vaddr[i], (uint16_t)store_values[i][round]);
                break;
            case 4U:
                sw(store_vaddr[i], (uint32_t)store_values[i][round]);
                break;
            default:
                break;
            }
            if (load_sizes[i] == 2U) {
                observed[i] = lhu(load_vaddr[i]);
            } else if (load_signed[i]) {
                observed[i] = ai_micro_cmo_signed_lw_value(load_vaddr[i]);
            } else {
                observed[i] = lwu(load_vaddr[i]);
            }
        }
        TEST_ASSERT("after an older whole-line cbo.zero and a younger partial store on the same cache line, the immediate small-load probes all complete without exception",
            excpt.triggered == false
        );

        goto_priv(PRIV_M);
        for (unsigned i = 0; i < AI_MICRO_CMO_MIXED_SMALL_LOAD_CASES; ++i) {
            expected_word0[i] = ai_micro_cmo_expected_word_after_partial_zero_store(
                store_offsets[i], store_sizes[i], store_values[i][round]);
            expected_load[i] = load_signed[i] ?
                ai_micro_cmo_expected_signed_window_from_word(
                    expected_word0[i], load_offsets[i], load_sizes[i]) :
                ai_micro_cmo_expected_unsigned_window_from_word(
                    expected_word0[i], load_offsets[i], load_sizes[i]);
        }
        TEST_ASSERT("the immediate small loads observe only the younger partial-store bytes over the older whole-line zero source, with zero-filled non-overlap bytes and correct final sign or zero extension",
            observed[0] == expected_load[0] &&
            observed[1] == expected_load[1] &&
            observed[2] == expected_load[2]
        );

        goto_priv(PRIV_HS);
        TEST_SETUP_EXCEPT();
        fence_iorw_iorw();
        TEST_ASSERT("the ordering fence after the cbo.zero plus younger partial-store mixed-source small-load probe retires without exception",
            excpt.triggered == false
        );

        goto_priv(PRIV_M);
        {
            bool backing_ok = true;
            for (unsigned i = 0; i < AI_MICRO_CMO_MIXED_SMALL_LOAD_CASES; ++i) {
                backing_ok = backing_ok &&
                    read64(base_paddr[i]) == expected_word0[i] &&
                    read64(base_paddr[i] + sizeof(uint64_t)) == 0ULL &&
                    read64(base_paddr[i] + (2U * sizeof(uint64_t))) == 0ULL &&
                    read64(base_paddr[i] + (3U * sizeof(uint64_t))) == 0ULL &&
                    read64(base_paddr[i] + (4U * sizeof(uint64_t))) == 0ULL &&
                    read64(base_paddr[i] + (5U * sizeof(uint64_t))) == 0ULL &&
                    read64(base_paddr[i] + (6U * sizeof(uint64_t))) == 0ULL &&
                    read64(base_paddr[i] + (7U * sizeof(uint64_t))) == 0ULL &&
                    (!check_adjacent ||
                        (read64(leading_paddr[i]) == leading_guard[i] &&
                         read64(trailing_paddr[i]) == trailing_guard[i]));
            }
            TEST_ASSERT("after the drain point, backing memory keeps only the younger partial-store bytes inside an otherwise zeroed cache block, and adjacent guards stay unchanged when requested",
                backing_ok
            );
        }
    }

    goto_priv(PRIV_M);
    ai_micro_cmo_reset_env(MENVCFG_CBZE);

    return test_status;
}

static bool ai_micro_cmo_sbuffer_same_block_inflight_valid_mixed_load_window(
    bool check_adjacent)
{
    enum {
        AI_MICRO_CMO_INFLIGHT_VALID_CASES = 3,
        AI_MICRO_CMO_INFLIGHT_VALID_ROUNDS = 3
    };
    const uintptr_t block_offsets[AI_MICRO_CMO_INFLIGHT_VALID_CASES] = {
        AI_MICRO_CMO_BLOCK_OFFSET * 11U,
        AI_MICRO_CMO_BLOCK_OFFSET * 13U,
        AI_MICRO_CMO_BLOCK_OFFSET * 15U,
    };
    const unsigned word_offsets[AI_MICRO_CMO_INFLIGHT_VALID_CASES] = {
        0U,
        1U,
        3U,
    };
    const unsigned flush_offsets[AI_MICRO_CMO_INFLIGHT_VALID_CASES] = {
        1U,
        15U,
        31U,
    };
    const unsigned store_offsets[AI_MICRO_CMO_INFLIGHT_VALID_CASES] = {
        2U,
        6U,
        6U,
    };
    const unsigned store_sizes[AI_MICRO_CMO_INFLIGHT_VALID_CASES] = {
        2U,
        2U,
        1U,
    };
    const unsigned load_offsets[AI_MICRO_CMO_INFLIGHT_VALID_CASES] = {
        0U,
        4U,
        6U,
    };
    const unsigned load_sizes[AI_MICRO_CMO_INFLIGHT_VALID_CASES] = {
        8U,
        4U,
        2U,
    };
    const bool load_signed[AI_MICRO_CMO_INFLIGHT_VALID_CASES] = {
        false,
        true,
        false,
    };
    const uint64_t seeds[AI_MICRO_CMO_INFLIGHT_VALID_CASES][AI_MICRO_CMO_INFLIGHT_VALID_ROUNDS] = {
        { 0x1021324354657687ULL, 0x5566778899aabbccULL, 0x89abcdef01234567ULL },
        { 0x0fedcba987654321ULL, 0xa1b2c3d4e5f60718ULL, 0x1122334455667788ULL },
        { 0x3141592653589793ULL, 0x2718281828459045ULL, 0x13579bdf2468ace0ULL },
    };
    const uint64_t older_store_values[AI_MICRO_CMO_INFLIGHT_VALID_CASES][AI_MICRO_CMO_INFLIGHT_VALID_ROUNDS] = {
        { 0x8877665544332211ULL, 0x0123456789abcdefULL, 0xfedcba9876543210ULL },
        { 0x1122334455667788ULL, 0x99aabbccddeeff00ULL, 0x7766554433221100ULL },
        { 0xa1b2c3d4e5f60718ULL, 0x0f1e2d3c4b5a6978ULL, 0x8899aabbccddeeffULL },
    };
    const uint64_t younger_store_values[AI_MICRO_CMO_INFLIGHT_VALID_CASES][AI_MICRO_CMO_INFLIGHT_VALID_ROUNDS] = {
        { 0xbeefULL, 0x7a5cULL, 0xd012ULL },
        { 0x80ffULL, 0x9234ULL, 0xf155ULL },
        { 0xa5ULL, 0x7cULL, 0xe1ULL },
    };
    uintptr_t base_vaddr[AI_MICRO_CMO_INFLIGHT_VALID_CASES];
    uintptr_t base_paddr[AI_MICRO_CMO_INFLIGHT_VALID_CASES];
    uintptr_t target_word_vaddr[AI_MICRO_CMO_INFLIGHT_VALID_CASES];
    uintptr_t target_word_paddr[AI_MICRO_CMO_INFLIGHT_VALID_CASES];
    uintptr_t flush_vaddr[AI_MICRO_CMO_INFLIGHT_VALID_CASES];
    uintptr_t store_vaddr[AI_MICRO_CMO_INFLIGHT_VALID_CASES];
    uintptr_t load_vaddr[AI_MICRO_CMO_INFLIGHT_VALID_CASES];
    uintptr_t leading_paddr[AI_MICRO_CMO_INFLIGHT_VALID_CASES];
    uintptr_t trailing_paddr[AI_MICRO_CMO_INFLIGHT_VALID_CASES];
    bool test_status = true;

    for (unsigned i = 0; i < AI_MICRO_CMO_INFLIGHT_VALID_CASES; ++i) {
        uintptr_t word_byte_offset = word_offsets[i] * sizeof(uint64_t);

        base_vaddr[i] = hs_page_base(AI_MICRO_CMO_CBOM_PAGE) + block_offsets[i];
        base_paddr[i] = phys_page_base(AI_MICRO_CMO_CBOM_PAGE) + block_offsets[i];
        target_word_vaddr[i] = base_vaddr[i] + word_byte_offset;
        target_word_paddr[i] = base_paddr[i] + word_byte_offset;
        flush_vaddr[i] = base_vaddr[i] + flush_offsets[i];
        store_vaddr[i] = target_word_vaddr[i] + store_offsets[i];
        load_vaddr[i] = target_word_vaddr[i] + load_offsets[i];
        leading_paddr[i] = base_paddr[i] - sizeof(uint64_t);
        trailing_paddr[i] = base_paddr[i] + (AI_MICRO_CMO_BLOCK_WORDS * sizeof(uint64_t));
    }

    goto_priv(PRIV_M);
    ai_micro_cmo_prepare_hs_env(MENVCFG_CBZFE);

    for (unsigned round = 0; round < AI_MICRO_CMO_INFLIGHT_VALID_ROUNDS; ++round) {
        uint64_t warmed[AI_MICRO_CMO_INFLIGHT_VALID_CASES];
        uint64_t observed[AI_MICRO_CMO_INFLIGHT_VALID_CASES];
        uint64_t expected_word[AI_MICRO_CMO_INFLIGHT_VALID_CASES];
        uint64_t expected_load[AI_MICRO_CMO_INFLIGHT_VALID_CASES];
        uint64_t leading_guard[AI_MICRO_CMO_INFLIGHT_VALID_CASES];
        uint64_t trailing_guard[AI_MICRO_CMO_INFLIGHT_VALID_CASES];

        goto_priv(PRIV_M);
        for (unsigned i = 0; i < AI_MICRO_CMO_INFLIGHT_VALID_CASES; ++i) {
            ai_micro_cmo_seed_block(base_paddr[i], seeds[i][round]);
            leading_guard[i] = 0xb100000000000000ULL |
                ((uint64_t)round << 16) | ((uint64_t)i << 8) | 0x4dULL;
            trailing_guard[i] = 0xc200000000000000ULL |
                ((uint64_t)round << 16) | ((uint64_t)i << 8) | 0x7eULL;
            write64(leading_paddr[i], leading_guard[i]);
            write64(trailing_paddr[i], trailing_guard[i]);
        }

        goto_priv(PRIV_HS);
        hspt_init();
        sfence_vma();

        TEST_SETUP_EXCEPT();
        for (unsigned i = 0; i < AI_MICRO_CMO_INFLIGHT_VALID_CASES; ++i) {
            warmed[i] = ld(target_word_vaddr[i]);
        }
        TEST_ASSERT("before the same-block inflight-plus-valid mixed-forward probe, all target words are readable with their seeded payloads",
            excpt.triggered == false &&
            warmed[0] == (seeds[0][round] +
                ((uint64_t)word_offsets[0] * 0x1111111111111111ULL)) &&
            warmed[1] == (seeds[1][round] +
                ((uint64_t)word_offsets[1] * 0x1111111111111111ULL)) &&
            warmed[2] == (seeds[2][round] +
                ((uint64_t)word_offsets[2] * 0x1111111111111111ULL))
        );

        TEST_SETUP_EXCEPT();
        for (unsigned i = 0; i < AI_MICRO_CMO_INFLIGHT_VALID_CASES; ++i) {
            sd(target_word_vaddr[i], older_store_values[i][round]);
            cbo_flush(flush_vaddr[i]);
            switch (store_sizes[i]) {
            case 1U:
                sb(store_vaddr[i], (uint8_t)younger_store_values[i][round]);
                break;
            case 2U:
                sh(store_vaddr[i], (uint16_t)younger_store_values[i][round]);
                break;
            case 4U:
                sw(store_vaddr[i], (uint32_t)younger_store_values[i][round]);
                break;
            default:
                break;
            }
            if (load_sizes[i] == 8U) {
                observed[i] = ld(load_vaddr[i]);
            } else if (load_sizes[i] == 2U) {
                observed[i] = lhu(load_vaddr[i]);
            } else if (load_signed[i]) {
                observed[i] = ai_micro_cmo_signed_lw_value(load_vaddr[i]);
            } else {
                observed[i] = lwu(load_vaddr[i]);
            }
        }
        TEST_ASSERT("the older full store, same-block cbo.flush, younger partial store, and immediate mixed-window load all retire without exception",
            excpt.triggered == false
        );

        goto_priv(PRIV_M);
        for (unsigned i = 0; i < AI_MICRO_CMO_INFLIGHT_VALID_CASES; ++i) {
            expected_word[i] = ai_micro_cmo_merge_partial_store_into_word(
                older_store_values[i][round],
                store_offsets[i],
                store_sizes[i],
                younger_store_values[i][round]);
            expected_load[i] = (load_sizes[i] == 8U) ? expected_word[i] :
                (load_signed[i] ?
                    ai_micro_cmo_expected_signed_window_from_word(
                        expected_word[i], load_offsets[i], load_sizes[i]) :
                    ai_micro_cmo_expected_unsigned_window_from_word(
                        expected_word[i], load_offsets[i], load_sizes[i]));
        }
        TEST_ASSERT("each immediate mixed-window load returns the byte-age merge of the older same-block inflight source and the younger valid source only, with correct final load shaping",
            observed[0] == expected_load[0] &&
            observed[1] == expected_load[1] &&
            observed[2] == expected_load[2]
        );

        goto_priv(PRIV_HS);
        TEST_SETUP_EXCEPT();
        fence_iorw_iorw();
        TEST_ASSERT("the ordering fence after the same-block inflight-plus-valid mixed-forward probe retires without exception",
            excpt.triggered == false
        );

        goto_priv(PRIV_M);
        {
            bool backing_ok = true;
            for (unsigned i = 0; i < AI_MICRO_CMO_INFLIGHT_VALID_CASES; ++i) {
                backing_ok = backing_ok &&
                    ai_micro_cmo_block_matches_seed_except_word(
                        base_paddr[i],
                        seeds[i][round],
                        word_offsets[i],
                        expected_word[i]) &&
                    (!check_adjacent ||
                        (read64(leading_paddr[i]) == leading_guard[i] &&
                         read64(trailing_paddr[i]) == trailing_guard[i]));
            }
            TEST_ASSERT("after drain, backing memory keeps the merged target word only, all untouched words remain seeded, and adjacent guards stay unchanged when requested",
                backing_ok
            );
        }
    }

    goto_priv(PRIV_M);
    ai_micro_cmo_reset_env(MENVCFG_CBZFE);

    return test_status;
}

static bool ai_micro_cmo_sbuffer_same_block_diff_word_inflight_valid_window(
    bool check_adjacent)
{
    enum {
        AI_MICRO_CMO_DIFF_WORD_CASES = 3,
        AI_MICRO_CMO_DIFF_WORD_ROUNDS = 3
    };
    const uintptr_t block_offsets[AI_MICRO_CMO_DIFF_WORD_CASES] = {
        AI_MICRO_CMO_BLOCK_OFFSET * 17U,
        AI_MICRO_CMO_BLOCK_OFFSET * 19U,
        AI_MICRO_CMO_BLOCK_OFFSET * 21U,
    };
    const unsigned older_word_offsets[AI_MICRO_CMO_DIFF_WORD_CASES] = {
        0U,
        1U,
        3U,
    };
    const unsigned younger_word_offsets[AI_MICRO_CMO_DIFF_WORD_CASES] = {
        1U,
        0U,
        2U,
    };
    const unsigned untouched_word_offsets[AI_MICRO_CMO_DIFF_WORD_CASES] = {
        4U,
        5U,
        6U,
    };
    const unsigned flush_offsets[AI_MICRO_CMO_DIFF_WORD_CASES] = {
        7U,
        15U,
        31U,
    };
    const uint64_t seeds[AI_MICRO_CMO_DIFF_WORD_CASES][AI_MICRO_CMO_DIFF_WORD_ROUNDS] = {
        { 0x1021324354657687ULL, 0x5566778899aabbccULL, 0x89abcdef01234567ULL },
        { 0x0fedcba987654321ULL, 0xa1b2c3d4e5f60718ULL, 0x1122334455667788ULL },
        { 0x3141592653589793ULL, 0x2718281828459045ULL, 0x13579bdf2468ace0ULL },
    };
    const uint64_t older_store_values[AI_MICRO_CMO_DIFF_WORD_CASES][AI_MICRO_CMO_DIFF_WORD_ROUNDS] = {
        { 0x8877665544332211ULL, 0x0123456789abcdefULL, 0xfedcba9876543210ULL },
        { 0x1122334455667788ULL, 0x99aabbccddeeff00ULL, 0x7766554433221100ULL },
        { 0xa1b2c3d4e5f60718ULL, 0x0f1e2d3c4b5a6978ULL, 0x8899aabbccddeeffULL },
    };
    const uint64_t younger_store_values[AI_MICRO_CMO_DIFF_WORD_CASES][AI_MICRO_CMO_DIFF_WORD_ROUNDS] = {
        { 0x0102030405060708ULL, 0x1817161514131211ULL, 0x2233445566778899ULL },
        { 0xdeadc0decafef00dULL, 0x0badf00d12345678ULL, 0x5566aa55cc33ee77ULL },
        { 0x55aa33cc77ee11ffULL, 0x1234fedc5678ba98ULL, 0x76543210fedcba98ULL },
    };
    uintptr_t base_vaddr[AI_MICRO_CMO_DIFF_WORD_CASES];
    uintptr_t base_paddr[AI_MICRO_CMO_DIFF_WORD_CASES];
    uintptr_t older_word_vaddr[AI_MICRO_CMO_DIFF_WORD_CASES];
    uintptr_t younger_word_vaddr[AI_MICRO_CMO_DIFF_WORD_CASES];
    uintptr_t untouched_word_vaddr[AI_MICRO_CMO_DIFF_WORD_CASES];
    uintptr_t flush_vaddr[AI_MICRO_CMO_DIFF_WORD_CASES];
    uintptr_t leading_paddr[AI_MICRO_CMO_DIFF_WORD_CASES];
    uintptr_t trailing_paddr[AI_MICRO_CMO_DIFF_WORD_CASES];
    bool test_status = true;

    for (unsigned i = 0; i < AI_MICRO_CMO_DIFF_WORD_CASES; ++i) {
        base_vaddr[i] = hs_page_base(AI_MICRO_CMO_CBOM_PAGE) + block_offsets[i];
        base_paddr[i] = phys_page_base(AI_MICRO_CMO_CBOM_PAGE) + block_offsets[i];
        older_word_vaddr[i] = base_vaddr[i] + (older_word_offsets[i] * sizeof(uint64_t));
        younger_word_vaddr[i] = base_vaddr[i] + (younger_word_offsets[i] * sizeof(uint64_t));
        untouched_word_vaddr[i] = base_vaddr[i] + (untouched_word_offsets[i] * sizeof(uint64_t));
        flush_vaddr[i] = base_vaddr[i] + flush_offsets[i];
        leading_paddr[i] = base_paddr[i] - sizeof(uint64_t);
        trailing_paddr[i] = base_paddr[i] + (AI_MICRO_CMO_BLOCK_WORDS * sizeof(uint64_t));
    }

    goto_priv(PRIV_M);
    ai_micro_cmo_prepare_hs_env(MENVCFG_CBZFE);

    for (unsigned round = 0; round < AI_MICRO_CMO_DIFF_WORD_ROUNDS; ++round) {
        uint64_t warmed_older[AI_MICRO_CMO_DIFF_WORD_CASES];
        uint64_t warmed_younger[AI_MICRO_CMO_DIFF_WORD_CASES];
        uint64_t warmed_untouched[AI_MICRO_CMO_DIFF_WORD_CASES];
        uint64_t observed_older[AI_MICRO_CMO_DIFF_WORD_CASES];
        uint64_t observed_younger[AI_MICRO_CMO_DIFF_WORD_CASES];
        uint64_t observed_untouched[AI_MICRO_CMO_DIFF_WORD_CASES];
        uint64_t expected_untouched[AI_MICRO_CMO_DIFF_WORD_CASES];
        uint64_t leading_guard[AI_MICRO_CMO_DIFF_WORD_CASES];
        uint64_t trailing_guard[AI_MICRO_CMO_DIFF_WORD_CASES];

        goto_priv(PRIV_M);
        for (unsigned i = 0; i < AI_MICRO_CMO_DIFF_WORD_CASES; ++i) {
            ai_micro_cmo_seed_block(base_paddr[i], seeds[i][round]);
            leading_guard[i] = 0xd300000000000000ULL |
                ((uint64_t)round << 16) | ((uint64_t)i << 8) | 0x29ULL;
            trailing_guard[i] = 0xe400000000000000ULL |
                ((uint64_t)round << 16) | ((uint64_t)i << 8) | 0x6bULL;
            write64(leading_paddr[i], leading_guard[i]);
            write64(trailing_paddr[i], trailing_guard[i]);
            expected_untouched[i] = seeds[i][round] +
                ((uint64_t)untouched_word_offsets[i] * 0x1111111111111111ULL);
        }

        goto_priv(PRIV_HS);
        hspt_init();
        sfence_vma();

        TEST_SETUP_EXCEPT();
        for (unsigned i = 0; i < AI_MICRO_CMO_DIFF_WORD_CASES; ++i) {
            warmed_older[i] = ld(older_word_vaddr[i]);
            warmed_younger[i] = ld(younger_word_vaddr[i]);
            warmed_untouched[i] = ld(untouched_word_vaddr[i]);
        }
        TEST_ASSERT("before the same-block different-word inflight-plus-valid probe, older, younger, and untouched target words are all readable with seeded payloads",
            excpt.triggered == false &&
            warmed_older[0] == (seeds[0][round] +
                ((uint64_t)older_word_offsets[0] * 0x1111111111111111ULL)) &&
            warmed_younger[0] == (seeds[0][round] +
                ((uint64_t)younger_word_offsets[0] * 0x1111111111111111ULL)) &&
            warmed_untouched[0] == expected_untouched[0] &&
            warmed_older[1] == (seeds[1][round] +
                ((uint64_t)older_word_offsets[1] * 0x1111111111111111ULL)) &&
            warmed_younger[1] == (seeds[1][round] +
                ((uint64_t)younger_word_offsets[1] * 0x1111111111111111ULL)) &&
            warmed_untouched[1] == expected_untouched[1] &&
            warmed_older[2] == (seeds[2][round] +
                ((uint64_t)older_word_offsets[2] * 0x1111111111111111ULL)) &&
            warmed_younger[2] == (seeds[2][round] +
                ((uint64_t)younger_word_offsets[2] * 0x1111111111111111ULL)) &&
            warmed_untouched[2] == expected_untouched[2]
        );

        TEST_SETUP_EXCEPT();
        for (unsigned i = 0; i < AI_MICRO_CMO_DIFF_WORD_CASES; ++i) {
            sd(older_word_vaddr[i], older_store_values[i][round]);
            cbo_flush(flush_vaddr[i]);
            sd(younger_word_vaddr[i], younger_store_values[i][round]);
            observed_older[i] = ld(older_word_vaddr[i]);
            observed_younger[i] = ld(younger_word_vaddr[i]);
            observed_untouched[i] = ld(untouched_word_vaddr[i]);
        }
        TEST_ASSERT("the older-word store, same-block cbo.flush, younger different-word store, and immediate dual-word loads all retire without exception",
            excpt.triggered == false
        );

        TEST_ASSERT("the immediate same-block dual-word loads keep word selection bound to the queried word only, with no cross-word bleed from the younger valid source into the older inflight word or an untouched word",
            observed_older[0] == older_store_values[0][round] &&
            observed_younger[0] == younger_store_values[0][round] &&
            observed_untouched[0] == expected_untouched[0] &&
            observed_older[1] == older_store_values[1][round] &&
            observed_younger[1] == younger_store_values[1][round] &&
            observed_untouched[1] == expected_untouched[1] &&
            observed_older[2] == older_store_values[2][round] &&
            observed_younger[2] == younger_store_values[2][round] &&
            observed_untouched[2] == expected_untouched[2]
        );

        goto_priv(PRIV_HS);
        TEST_SETUP_EXCEPT();
        fence_iorw_iorw();
        TEST_ASSERT("the ordering fence after the same-block different-word inflight-plus-valid probe retires without exception",
            excpt.triggered == false
        );

        goto_priv(PRIV_M);
        {
            bool backing_ok = true;
            for (unsigned i = 0; i < AI_MICRO_CMO_DIFF_WORD_CASES; ++i) {
                backing_ok = backing_ok &&
                    ai_micro_cmo_block_matches_seed_except_two_words(
                        base_paddr[i],
                        seeds[i][round],
                        older_word_offsets[i],
                        older_store_values[i][round],
                        younger_word_offsets[i],
                        younger_store_values[i][round]) &&
                    (!check_adjacent ||
                        (read64(leading_paddr[i]) == leading_guard[i] &&
                         read64(trailing_paddr[i]) == trailing_guard[i]));
            }
            TEST_ASSERT("after drain, both modified words keep their correct per-word payloads, untouched words stay seeded, and adjacent guards remain unchanged when requested",
                backing_ok
            );
        }
    }

    goto_priv(PRIV_M);
    ai_micro_cmo_reset_env(MENVCFG_CBZFE);

    return test_status;
}

static bool ai_micro_cmo_sbuffer_same_block_mixed_word_back_to_back_load_window(
    bool check_adjacent)
{
    enum {
        AI_MICRO_CMO_MIXED_B2B_CASES = 3,
        AI_MICRO_CMO_MIXED_B2B_ROUNDS = 3
    };
    const uintptr_t block_offsets[AI_MICRO_CMO_MIXED_B2B_CASES] = {
        AI_MICRO_CMO_BLOCK_OFFSET * 23U,
        AI_MICRO_CMO_BLOCK_OFFSET * 25U,
        AI_MICRO_CMO_BLOCK_OFFSET * 27U,
    };
    const unsigned store_offsets[AI_MICRO_CMO_MIXED_B2B_CASES] = {
        2U,
        6U,
        3U,
    };
    const unsigned store_sizes[AI_MICRO_CMO_MIXED_B2B_CASES] = {
        2U,
        2U,
        1U,
    };
    const unsigned load_a_offsets[AI_MICRO_CMO_MIXED_B2B_CASES] = {
        0U,
        4U,
        2U,
    };
    const unsigned load_a_sizes[AI_MICRO_CMO_MIXED_B2B_CASES] = {
        4U,
        4U,
        2U,
    };
    const bool load_a_signed[AI_MICRO_CMO_MIXED_B2B_CASES] = {
        false,
        true,
        false,
    };
    const unsigned load_b_offsets[AI_MICRO_CMO_MIXED_B2B_CASES] = {
        2U,
        6U,
        3U,
    };
    const unsigned load_b_sizes[AI_MICRO_CMO_MIXED_B2B_CASES] = {
        2U,
        2U,
        1U,
    };
    const bool load_b_signed[AI_MICRO_CMO_MIXED_B2B_CASES] = {
        false,
        false,
        false,
    };
    const uint64_t seeds[AI_MICRO_CMO_MIXED_B2B_CASES][AI_MICRO_CMO_MIXED_B2B_ROUNDS] = {
        { 0x1021324354657687ULL, 0x5566778899aabbccULL, 0x89abcdef01234567ULL },
        { 0x0fedcba987654321ULL, 0xa1b2c3d4e5f60718ULL, 0x1122334455667788ULL },
        { 0x3141592653589793ULL, 0x2718281828459045ULL, 0x13579bdf2468ace0ULL },
    };
    const uint64_t older_store_values[AI_MICRO_CMO_MIXED_B2B_CASES][AI_MICRO_CMO_MIXED_B2B_ROUNDS] = {
        { 0x8877665544332211ULL, 0x0123456789abcdefULL, 0xfedcba9876543210ULL },
        { 0x1122334455667788ULL, 0x99aabbccddeeff00ULL, 0x7766554433221100ULL },
        { 0xa1b2c3d4e5f60718ULL, 0x0f1e2d3c4b5a6978ULL, 0x8899aabbccddeeffULL },
    };
    const uint64_t younger_store_values[AI_MICRO_CMO_MIXED_B2B_CASES][AI_MICRO_CMO_MIXED_B2B_ROUNDS] = {
        { 0xbeefULL, 0x7a5cULL, 0xd012ULL },
        { 0x80ffULL, 0x9234ULL, 0xf155ULL },
        { 0xa5ULL, 0x7cULL, 0xe1ULL },
    };
    uintptr_t base_vaddr[AI_MICRO_CMO_MIXED_B2B_CASES];
    uintptr_t base_paddr[AI_MICRO_CMO_MIXED_B2B_CASES];
    uintptr_t target_word_vaddr[AI_MICRO_CMO_MIXED_B2B_CASES];
    uintptr_t flush_vaddr[AI_MICRO_CMO_MIXED_B2B_CASES];
    uintptr_t store_vaddr[AI_MICRO_CMO_MIXED_B2B_CASES];
    uintptr_t leading_paddr[AI_MICRO_CMO_MIXED_B2B_CASES];
    uintptr_t trailing_paddr[AI_MICRO_CMO_MIXED_B2B_CASES];
    bool test_status = true;

    for (unsigned i = 0; i < AI_MICRO_CMO_MIXED_B2B_CASES; ++i) {
        base_vaddr[i] = hs_page_base(AI_MICRO_CMO_CBOM_PAGE) + block_offsets[i];
        base_paddr[i] = phys_page_base(AI_MICRO_CMO_CBOM_PAGE) + block_offsets[i];
        target_word_vaddr[i] = base_vaddr[i];
        flush_vaddr[i] = base_vaddr[i] + ((i == 0U) ? 1U : ((i == 1U) ? 15U : 31U));
        store_vaddr[i] = target_word_vaddr[i] + store_offsets[i];
        leading_paddr[i] = base_paddr[i] - sizeof(uint64_t);
        trailing_paddr[i] = base_paddr[i] + (AI_MICRO_CMO_BLOCK_WORDS * sizeof(uint64_t));
    }

    goto_priv(PRIV_M);
    ai_micro_cmo_prepare_hs_env(MENVCFG_CBZFE);

    for (unsigned round = 0; round < AI_MICRO_CMO_MIXED_B2B_ROUNDS; ++round) {
        uint64_t warmed[AI_MICRO_CMO_MIXED_B2B_CASES];
        uint64_t observed_first[AI_MICRO_CMO_MIXED_B2B_CASES];
        uint64_t observed_second[AI_MICRO_CMO_MIXED_B2B_CASES];
        uint64_t expected_word[AI_MICRO_CMO_MIXED_B2B_CASES];
        uint64_t expected_a[AI_MICRO_CMO_MIXED_B2B_CASES];
        uint64_t expected_b[AI_MICRO_CMO_MIXED_B2B_CASES];
        uint64_t expected_first[AI_MICRO_CMO_MIXED_B2B_CASES];
        uint64_t expected_second[AI_MICRO_CMO_MIXED_B2B_CASES];
        uint64_t leading_guard[AI_MICRO_CMO_MIXED_B2B_CASES];
        uint64_t trailing_guard[AI_MICRO_CMO_MIXED_B2B_CASES];
        bool reverse_order = (round & 1U) != 0U;

        goto_priv(PRIV_M);
        for (unsigned i = 0; i < AI_MICRO_CMO_MIXED_B2B_CASES; ++i) {
            ai_micro_cmo_seed_block(base_paddr[i], seeds[i][round]);
            leading_guard[i] = 0xf500000000000000ULL |
                ((uint64_t)round << 16) | ((uint64_t)i << 8) | 0x19ULL;
            trailing_guard[i] = 0x9600000000000000ULL |
                ((uint64_t)round << 16) | ((uint64_t)i << 8) | 0x8cULL;
            write64(leading_paddr[i], leading_guard[i]);
            write64(trailing_paddr[i], trailing_guard[i]);
        }

        goto_priv(PRIV_HS);
        hspt_init();
        sfence_vma();

        TEST_SETUP_EXCEPT();
        for (unsigned i = 0; i < AI_MICRO_CMO_MIXED_B2B_CASES; ++i) {
            warmed[i] = ld(target_word_vaddr[i]);
        }
        TEST_ASSERT("before the same mixed-source back-to-back load probe, all target words are readable with their seeded payloads",
            excpt.triggered == false &&
            warmed[0] == seeds[0][round] &&
            warmed[1] == seeds[1][round] &&
            warmed[2] == seeds[2][round]
        );

        TEST_SETUP_EXCEPT();
        for (unsigned i = 0; i < AI_MICRO_CMO_MIXED_B2B_CASES; ++i) {
            sd(target_word_vaddr[i], older_store_values[i][round]);
            cbo_flush(flush_vaddr[i]);
            switch (store_sizes[i]) {
            case 1U:
                sb(store_vaddr[i], (uint8_t)younger_store_values[i][round]);
                break;
            case 2U:
                sh(store_vaddr[i], (uint16_t)younger_store_values[i][round]);
                break;
            case 4U:
                sw(store_vaddr[i], (uint32_t)younger_store_values[i][round]);
                break;
            default:
                break;
            }

            if (!reverse_order) {
                if (load_a_sizes[i] == 2U) {
                    observed_first[i] = lhu(target_word_vaddr[i] + load_a_offsets[i]);
                } else if (load_a_signed[i]) {
                    observed_first[i] = ai_micro_cmo_signed_lw_value(target_word_vaddr[i] + load_a_offsets[i]);
                } else {
                    observed_first[i] = lwu(target_word_vaddr[i] + load_a_offsets[i]);
                }

                if (load_b_sizes[i] == 1U) {
                    observed_second[i] = lbu(target_word_vaddr[i] + load_b_offsets[i]);
                } else {
                    observed_second[i] = lhu(target_word_vaddr[i] + load_b_offsets[i]);
                }
            } else {
                if (load_b_sizes[i] == 1U) {
                    observed_first[i] = lbu(target_word_vaddr[i] + load_b_offsets[i]);
                } else {
                    observed_first[i] = lhu(target_word_vaddr[i] + load_b_offsets[i]);
                }

                if (load_a_sizes[i] == 2U) {
                    observed_second[i] = lhu(target_word_vaddr[i] + load_a_offsets[i]);
                } else if (load_a_signed[i]) {
                    observed_second[i] = ai_micro_cmo_signed_lw_value(target_word_vaddr[i] + load_a_offsets[i]);
                } else {
                    observed_second[i] = lwu(target_word_vaddr[i] + load_a_offsets[i]);
                }
            }
        }
        TEST_ASSERT("the older full store, same-block cbo.flush, younger partial store, and immediate back-to-back mixed-window loads all retire without exception",
            excpt.triggered == false
        );

        goto_priv(PRIV_M);
        for (unsigned i = 0; i < AI_MICRO_CMO_MIXED_B2B_CASES; ++i) {
            expected_word[i] = ai_micro_cmo_merge_partial_store_into_word(
                older_store_values[i][round],
                store_offsets[i],
                store_sizes[i],
                younger_store_values[i][round]);
            expected_a[i] = load_a_signed[i] ?
                ai_micro_cmo_expected_signed_window_from_word(
                    expected_word[i], load_a_offsets[i], load_a_sizes[i]) :
                ai_micro_cmo_expected_unsigned_window_from_word(
                    expected_word[i], load_a_offsets[i], load_a_sizes[i]);
            expected_b[i] = load_b_signed[i] ?
                ai_micro_cmo_expected_signed_window_from_word(
                    expected_word[i], load_b_offsets[i], load_b_sizes[i]) :
                ai_micro_cmo_expected_unsigned_window_from_word(
                    expected_word[i], load_b_offsets[i], load_b_sizes[i]);
            expected_first[i] = reverse_order ? expected_b[i] : expected_a[i];
            expected_second[i] = reverse_order ? expected_a[i] : expected_b[i];
        }
        TEST_ASSERT("back-to-back immediate loads over the same inflight-plus-valid merged word keep each query bound to its own byte slice and final shaping, with no stale previous-query select leaking into the next load",
            observed_first[0] == expected_first[0] &&
            observed_second[0] == expected_second[0] &&
            observed_first[1] == expected_first[1] &&
            observed_second[1] == expected_second[1] &&
            observed_first[2] == expected_first[2] &&
            observed_second[2] == expected_second[2]
        );

        goto_priv(PRIV_HS);
        TEST_SETUP_EXCEPT();
        fence_iorw_iorw();
        TEST_ASSERT("the ordering fence after the same mixed-source back-to-back load probe retires without exception",
            excpt.triggered == false
        );

        goto_priv(PRIV_M);
        {
            bool backing_ok = true;
            for (unsigned i = 0; i < AI_MICRO_CMO_MIXED_B2B_CASES; ++i) {
                backing_ok = backing_ok &&
                    ai_micro_cmo_block_matches_seed_except_word(
                        base_paddr[i],
                        seeds[i][round],
                        0U,
                        expected_word[i]) &&
                    (!check_adjacent ||
                        (read64(leading_paddr[i]) == leading_guard[i] &&
                         read64(trailing_paddr[i]) == trailing_guard[i]));
            }
            TEST_ASSERT("after drain, the merged target word persists, untouched words stay seeded, and adjacent guards remain unchanged when requested",
                backing_ok
            );
        }
    }

    goto_priv(PRIV_M);
    ai_micro_cmo_reset_env(MENVCFG_CBZFE);

    return test_status;
}

static bool ai_micro_cmo_sbuffer_same_block_waiting_valid_two_merges_window(
    bool check_adjacent)
{
    enum {
        AI_MICRO_CMO_WAITING_VALID_MERGE_CASES = 3,
        AI_MICRO_CMO_WAITING_VALID_MERGE_ROUNDS = 3
    };
    const uintptr_t block_offsets[AI_MICRO_CMO_WAITING_VALID_MERGE_CASES] = {
        AI_MICRO_CMO_BLOCK_OFFSET * 29U,
        AI_MICRO_CMO_BLOCK_OFFSET * 31U,
        AI_MICRO_CMO_BLOCK_OFFSET * 33U,
    };
    const unsigned word_offsets[AI_MICRO_CMO_WAITING_VALID_MERGE_CASES] = {
        0U,
        1U,
        3U,
    };
    const unsigned flush_offsets[AI_MICRO_CMO_WAITING_VALID_MERGE_CASES] = {
        1U,
        15U,
        31U,
    };
    const unsigned store1_offsets[AI_MICRO_CMO_WAITING_VALID_MERGE_CASES] = {
        2U,
        5U,
        3U,
    };
    const unsigned store1_sizes[AI_MICRO_CMO_WAITING_VALID_MERGE_CASES] = {
        2U,
        2U,
        1U,
    };
    const unsigned store2_offsets[AI_MICRO_CMO_WAITING_VALID_MERGE_CASES] = {
        6U,
        6U,
        4U,
    };
    const unsigned store2_sizes[AI_MICRO_CMO_WAITING_VALID_MERGE_CASES] = {
        1U,
        1U,
        2U,
    };
    const unsigned load_offsets[AI_MICRO_CMO_WAITING_VALID_MERGE_CASES] = {
        0U,
        4U,
        3U,
    };
    const unsigned load_sizes[AI_MICRO_CMO_WAITING_VALID_MERGE_CASES] = {
        8U,
        4U,
        2U,
    };
    const bool load_signed[AI_MICRO_CMO_WAITING_VALID_MERGE_CASES] = {
        false,
        true,
        false,
    };
    const uint64_t seeds[AI_MICRO_CMO_WAITING_VALID_MERGE_CASES][AI_MICRO_CMO_WAITING_VALID_MERGE_ROUNDS] = {
        { 0x1021324354657687ULL, 0x5566778899aabbccULL, 0x89abcdef01234567ULL },
        { 0x0fedcba987654321ULL, 0xa1b2c3d4e5f60718ULL, 0x1122334455667788ULL },
        { 0x3141592653589793ULL, 0x2718281828459045ULL, 0x13579bdf2468ace0ULL },
    };
    const uint64_t older_store_values[AI_MICRO_CMO_WAITING_VALID_MERGE_CASES][AI_MICRO_CMO_WAITING_VALID_MERGE_ROUNDS] = {
        { 0x8877665544332211ULL, 0x0123456789abcdefULL, 0xfedcba9876543210ULL },
        { 0x1122334455667788ULL, 0x99aabbccddeeff00ULL, 0x7766554433221100ULL },
        { 0xa1b2c3d4e5f60718ULL, 0x0f1e2d3c4b5a6978ULL, 0x8899aabbccddeeffULL },
    };
    const uint64_t store1_values[AI_MICRO_CMO_WAITING_VALID_MERGE_CASES][AI_MICRO_CMO_WAITING_VALID_MERGE_ROUNDS] = {
        { 0xbeefULL, 0x7a5cULL, 0xd012ULL },
        { 0x80ffULL, 0x9234ULL, 0xf155ULL },
        { 0xa5ULL, 0x7cULL, 0xe1ULL },
    };
    const uint64_t store2_values[AI_MICRO_CMO_WAITING_VALID_MERGE_CASES][AI_MICRO_CMO_WAITING_VALID_MERGE_ROUNDS] = {
        { 0xaaULL, 0x6cULL, 0xf1ULL },
        { 0x12ULL, 0xf0ULL, 0x7bULL },
        { 0x3456ULL, 0x80ffULL, 0x12aaULL },
    };
    uintptr_t base_vaddr[AI_MICRO_CMO_WAITING_VALID_MERGE_CASES];
    uintptr_t base_paddr[AI_MICRO_CMO_WAITING_VALID_MERGE_CASES];
    uintptr_t target_word_vaddr[AI_MICRO_CMO_WAITING_VALID_MERGE_CASES];
    uintptr_t base_word_paddr[AI_MICRO_CMO_WAITING_VALID_MERGE_CASES];
    uintptr_t flush_vaddr[AI_MICRO_CMO_WAITING_VALID_MERGE_CASES];
    uintptr_t store1_vaddr[AI_MICRO_CMO_WAITING_VALID_MERGE_CASES];
    uintptr_t store2_vaddr[AI_MICRO_CMO_WAITING_VALID_MERGE_CASES];
    uintptr_t load_vaddr[AI_MICRO_CMO_WAITING_VALID_MERGE_CASES];
    uintptr_t leading_paddr[AI_MICRO_CMO_WAITING_VALID_MERGE_CASES];
    uintptr_t trailing_paddr[AI_MICRO_CMO_WAITING_VALID_MERGE_CASES];
    bool test_status = true;

    for (unsigned i = 0; i < AI_MICRO_CMO_WAITING_VALID_MERGE_CASES; ++i) {
        uintptr_t word_byte_offset = word_offsets[i] * sizeof(uint64_t);

        base_vaddr[i] = hs_page_base(AI_MICRO_CMO_CBOM_PAGE) + block_offsets[i];
        base_paddr[i] = phys_page_base(AI_MICRO_CMO_CBOM_PAGE) + block_offsets[i];
        target_word_vaddr[i] = base_vaddr[i] + word_byte_offset;
        base_word_paddr[i] = base_paddr[i] + word_byte_offset;
        flush_vaddr[i] = base_vaddr[i] + flush_offsets[i];
        store1_vaddr[i] = target_word_vaddr[i] + store1_offsets[i];
        store2_vaddr[i] = target_word_vaddr[i] + store2_offsets[i];
        load_vaddr[i] = target_word_vaddr[i] + load_offsets[i];
        leading_paddr[i] = base_paddr[i] - sizeof(uint64_t);
        trailing_paddr[i] = base_paddr[i] + (AI_MICRO_CMO_BLOCK_WORDS * sizeof(uint64_t));
    }

    goto_priv(PRIV_M);
    ai_micro_cmo_prepare_hs_env(MENVCFG_CBZFE);

    for (unsigned round = 0; round < AI_MICRO_CMO_WAITING_VALID_MERGE_ROUNDS; ++round) {
        uint64_t warmed[AI_MICRO_CMO_WAITING_VALID_MERGE_CASES];
        uint64_t observed[AI_MICRO_CMO_WAITING_VALID_MERGE_CASES];
        uint64_t expected_word[AI_MICRO_CMO_WAITING_VALID_MERGE_CASES];
        uint64_t expected_load[AI_MICRO_CMO_WAITING_VALID_MERGE_CASES];
        uint64_t leading_guard[AI_MICRO_CMO_WAITING_VALID_MERGE_CASES];
        uint64_t trailing_guard[AI_MICRO_CMO_WAITING_VALID_MERGE_CASES];
        bool reverse_order = (round & 1U) != 0U;

        goto_priv(PRIV_M);
        for (unsigned i = 0; i < AI_MICRO_CMO_WAITING_VALID_MERGE_CASES; ++i) {
            ai_micro_cmo_seed_block(base_paddr[i], seeds[i][round]);
            leading_guard[i] = 0xa700000000000000ULL |
                ((uint64_t)round << 16) | ((uint64_t)i << 8) | 0x3dULL;
            trailing_guard[i] = 0xb800000000000000ULL |
                ((uint64_t)round << 16) | ((uint64_t)i << 8) | 0x91ULL;
            write64(leading_paddr[i], leading_guard[i]);
            write64(trailing_paddr[i], trailing_guard[i]);
        }

        goto_priv(PRIV_HS);
        hspt_init();
        sfence_vma();

        TEST_SETUP_EXCEPT();
        for (unsigned i = 0; i < AI_MICRO_CMO_WAITING_VALID_MERGE_CASES; ++i) {
            warmed[i] = ld(target_word_vaddr[i]);
        }
        TEST_ASSERT("before the same-block waiting-valid second-merge probe, all target words are readable with their seeded payloads",
            excpt.triggered == false &&
            warmed[0] == (seeds[0][round] +
                ((uint64_t)word_offsets[0] * 0x1111111111111111ULL)) &&
            warmed[1] == (seeds[1][round] +
                ((uint64_t)word_offsets[1] * 0x1111111111111111ULL)) &&
            warmed[2] == (seeds[2][round] +
                ((uint64_t)word_offsets[2] * 0x1111111111111111ULL))
        );

        TEST_SETUP_EXCEPT();
        for (unsigned i = 0; i < AI_MICRO_CMO_WAITING_VALID_MERGE_CASES; ++i) {
            sd(target_word_vaddr[i], older_store_values[i][round]);
            cbo_flush(flush_vaddr[i]);

            if (!reverse_order) {
                switch (store1_sizes[i]) {
                case 1U:
                    sb(store1_vaddr[i], (uint8_t)store1_values[i][round]);
                    break;
                case 2U:
                    sh(store1_vaddr[i], (uint16_t)store1_values[i][round]);
                    break;
                case 4U:
                    sw(store1_vaddr[i], (uint32_t)store1_values[i][round]);
                    break;
                default:
                    break;
                }

                switch (store2_sizes[i]) {
                case 1U:
                    sb(store2_vaddr[i], (uint8_t)store2_values[i][round]);
                    break;
                case 2U:
                    sh(store2_vaddr[i], (uint16_t)store2_values[i][round]);
                    break;
                case 4U:
                    sw(store2_vaddr[i], (uint32_t)store2_values[i][round]);
                    break;
                default:
                    break;
                }
            } else {
                switch (store2_sizes[i]) {
                case 1U:
                    sb(store2_vaddr[i], (uint8_t)store2_values[i][round]);
                    break;
                case 2U:
                    sh(store2_vaddr[i], (uint16_t)store2_values[i][round]);
                    break;
                case 4U:
                    sw(store2_vaddr[i], (uint32_t)store2_values[i][round]);
                    break;
                default:
                    break;
                }

                switch (store1_sizes[i]) {
                case 1U:
                    sb(store1_vaddr[i], (uint8_t)store1_values[i][round]);
                    break;
                case 2U:
                    sh(store1_vaddr[i], (uint16_t)store1_values[i][round]);
                    break;
                case 4U:
                    sw(store1_vaddr[i], (uint32_t)store1_values[i][round]);
                    break;
                default:
                    break;
                }
            }

            if (load_sizes[i] == 8U) {
                observed[i] = ld(load_vaddr[i]);
            } else if (load_sizes[i] == 2U) {
                observed[i] = lhu(load_vaddr[i]);
            } else if (load_signed[i]) {
                observed[i] = ai_micro_cmo_signed_lw_value(load_vaddr[i]);
            } else {
                observed[i] = lwu(load_vaddr[i]);
            }
        }
        TEST_ASSERT("the older full store, same-block cbo.flush, two younger partial stores, and immediate merged-window load all retire without exception",
            excpt.triggered == false
        );

        goto_priv(PRIV_M);
        for (unsigned i = 0; i < AI_MICRO_CMO_WAITING_VALID_MERGE_CASES; ++i) {
            uint64_t merged_once = older_store_values[i][round];

            if (!reverse_order) {
                merged_once = ai_micro_cmo_merge_partial_store_into_word(
                    merged_once,
                    store1_offsets[i],
                    store1_sizes[i],
                    store1_values[i][round]);
                expected_word[i] = ai_micro_cmo_merge_partial_store_into_word(
                    merged_once,
                    store2_offsets[i],
                    store2_sizes[i],
                    store2_values[i][round]);
            } else {
                merged_once = ai_micro_cmo_merge_partial_store_into_word(
                    merged_once,
                    store2_offsets[i],
                    store2_sizes[i],
                    store2_values[i][round]);
                expected_word[i] = ai_micro_cmo_merge_partial_store_into_word(
                    merged_once,
                    store1_offsets[i],
                    store1_sizes[i],
                    store1_values[i][round]);
            }

            expected_load[i] = load_signed[i] ?
                ai_micro_cmo_expected_signed_window_from_word(
                    expected_word[i], load_offsets[i], load_sizes[i]) :
                ai_micro_cmo_expected_unsigned_window_from_word(
                    expected_word[i], load_offsets[i], load_sizes[i]);
        }
        TEST_ASSERT("the immediate load observes the full byte-age merge of the older inflight source plus both younger stores merged into the waiting valid entry, with no lost second merge or stale allocation target",
            observed[0] == expected_load[0] &&
            observed[1] == expected_load[1] &&
            observed[2] == expected_load[2]
        );

        goto_priv(PRIV_HS);
        TEST_SETUP_EXCEPT();
        fence_iorw_iorw();
        TEST_ASSERT("the ordering fence after the same-block waiting-valid second-merge probe retires without exception",
            excpt.triggered == false
        );

        goto_priv(PRIV_M);
        {
            bool backing_ok = true;
            for (unsigned i = 0; i < AI_MICRO_CMO_WAITING_VALID_MERGE_CASES; ++i) {
                backing_ok = backing_ok &&
                    ai_micro_cmo_block_matches_seed_except_word(
                        base_paddr[i],
                        seeds[i][round],
                        word_offsets[i],
                        expected_word[i]) &&
                    (!check_adjacent ||
                        (read64(leading_paddr[i]) == leading_guard[i] &&
                         read64(trailing_paddr[i]) == trailing_guard[i]));
            }
            TEST_ASSERT("after drain, the cumulatively merged target word persists, untouched words stay seeded, and adjacent guards remain unchanged when requested",
                backing_ok
            );
        }
    }

    goto_priv(PRIV_M);
    ai_micro_cmo_reset_env(MENVCFG_CBZFE);

    return test_status;
}

static bool ai_micro_cmo_sbuffer_same_block_waiting_valid_multiword_window(
    bool check_adjacent)
{
    enum {
        AI_MICRO_CMO_WAITING_VALID_MULTIWORD_CASES = 3,
        AI_MICRO_CMO_WAITING_VALID_MULTIWORD_ROUNDS = 3
    };
    const uintptr_t block_offsets[AI_MICRO_CMO_WAITING_VALID_MULTIWORD_CASES] = {
        AI_MICRO_CMO_BLOCK_OFFSET * 35U,
        AI_MICRO_CMO_BLOCK_OFFSET * 37U,
        AI_MICRO_CMO_BLOCK_OFFSET * 39U,
    };
    const unsigned older_word_offsets[AI_MICRO_CMO_WAITING_VALID_MULTIWORD_CASES] = {
        0U,
        2U,
        1U,
    };
    const unsigned younger0_word_offsets[AI_MICRO_CMO_WAITING_VALID_MULTIWORD_CASES] = {
        1U,
        0U,
        3U,
    };
    const unsigned younger1_word_offsets[AI_MICRO_CMO_WAITING_VALID_MULTIWORD_CASES] = {
        3U,
        4U,
        5U,
    };
    const unsigned untouched_word_offsets[AI_MICRO_CMO_WAITING_VALID_MULTIWORD_CASES] = {
        6U,
        5U,
        7U,
    };
    const unsigned flush_offsets[AI_MICRO_CMO_WAITING_VALID_MULTIWORD_CASES] = {
        1U,
        15U,
        31U,
    };
    const uint64_t seeds[AI_MICRO_CMO_WAITING_VALID_MULTIWORD_CASES][AI_MICRO_CMO_WAITING_VALID_MULTIWORD_ROUNDS] = {
        { 0x1021324354657687ULL, 0x5566778899aabbccULL, 0x89abcdef01234567ULL },
        { 0x0fedcba987654321ULL, 0xa1b2c3d4e5f60718ULL, 0x1122334455667788ULL },
        { 0x3141592653589793ULL, 0x2718281828459045ULL, 0x13579bdf2468ace0ULL },
    };
    const uint64_t older_store_values[AI_MICRO_CMO_WAITING_VALID_MULTIWORD_CASES][AI_MICRO_CMO_WAITING_VALID_MULTIWORD_ROUNDS] = {
        { 0x8877665544332211ULL, 0x0123456789abcdefULL, 0xfedcba9876543210ULL },
        { 0x1122334455667788ULL, 0x99aabbccddeeff00ULL, 0x7766554433221100ULL },
        { 0xa1b2c3d4e5f60718ULL, 0x0f1e2d3c4b5a6978ULL, 0x8899aabbccddeeffULL },
    };
    const uint64_t younger0_values[AI_MICRO_CMO_WAITING_VALID_MULTIWORD_CASES][AI_MICRO_CMO_WAITING_VALID_MULTIWORD_ROUNDS] = {
        { 0x0102030405060708ULL, 0x1817161514131211ULL, 0x2233445566778899ULL },
        { 0xdeadc0decafef00dULL, 0x0badf00d12345678ULL, 0x5566aa55cc33ee77ULL },
        { 0x55aa33cc77ee11ffULL, 0x1234fedc5678ba98ULL, 0x76543210fedcba98ULL },
    };
    const uint64_t younger1_values[AI_MICRO_CMO_WAITING_VALID_MULTIWORD_CASES][AI_MICRO_CMO_WAITING_VALID_MULTIWORD_ROUNDS] = {
        { 0xa1b2c3d4e5f60718ULL, 0xb1c2d3e4f5060719ULL, 0xc1d2e3f40516081aULL },
        { 0x3141592653589793ULL, 0x2718281828459045ULL, 0x13579bdf2468ace0ULL },
        { 0x8899aabbccddeeffULL, 0x1122446688aacceeULL, 0xf0e1d2c3b4a59687ULL },
    };
    uintptr_t base_vaddr[AI_MICRO_CMO_WAITING_VALID_MULTIWORD_CASES];
    uintptr_t base_paddr[AI_MICRO_CMO_WAITING_VALID_MULTIWORD_CASES];
    uintptr_t older_word_vaddr[AI_MICRO_CMO_WAITING_VALID_MULTIWORD_CASES];
    uintptr_t younger0_word_vaddr[AI_MICRO_CMO_WAITING_VALID_MULTIWORD_CASES];
    uintptr_t younger1_word_vaddr[AI_MICRO_CMO_WAITING_VALID_MULTIWORD_CASES];
    uintptr_t untouched_word_vaddr[AI_MICRO_CMO_WAITING_VALID_MULTIWORD_CASES];
    uintptr_t flush_vaddr[AI_MICRO_CMO_WAITING_VALID_MULTIWORD_CASES];
    uintptr_t leading_paddr[AI_MICRO_CMO_WAITING_VALID_MULTIWORD_CASES];
    uintptr_t trailing_paddr[AI_MICRO_CMO_WAITING_VALID_MULTIWORD_CASES];
    bool test_status = true;

    for (unsigned i = 0; i < AI_MICRO_CMO_WAITING_VALID_MULTIWORD_CASES; ++i) {
        base_vaddr[i] = hs_page_base(AI_MICRO_CMO_CBOM_PAGE) + block_offsets[i];
        base_paddr[i] = phys_page_base(AI_MICRO_CMO_CBOM_PAGE) + block_offsets[i];
        older_word_vaddr[i] = base_vaddr[i] + (older_word_offsets[i] * sizeof(uint64_t));
        younger0_word_vaddr[i] = base_vaddr[i] + (younger0_word_offsets[i] * sizeof(uint64_t));
        younger1_word_vaddr[i] = base_vaddr[i] + (younger1_word_offsets[i] * sizeof(uint64_t));
        untouched_word_vaddr[i] = base_vaddr[i] + (untouched_word_offsets[i] * sizeof(uint64_t));
        flush_vaddr[i] = base_vaddr[i] + flush_offsets[i];
        leading_paddr[i] = base_paddr[i] - sizeof(uint64_t);
        trailing_paddr[i] = base_paddr[i] + (AI_MICRO_CMO_BLOCK_WORDS * sizeof(uint64_t));
    }

    goto_priv(PRIV_M);
    ai_micro_cmo_prepare_hs_env(MENVCFG_CBZFE);

    for (unsigned round = 0; round < AI_MICRO_CMO_WAITING_VALID_MULTIWORD_ROUNDS; ++round) {
        uint64_t warmed_older[AI_MICRO_CMO_WAITING_VALID_MULTIWORD_CASES];
        uint64_t warmed_younger0[AI_MICRO_CMO_WAITING_VALID_MULTIWORD_CASES];
        uint64_t warmed_younger1[AI_MICRO_CMO_WAITING_VALID_MULTIWORD_CASES];
        uint64_t warmed_untouched[AI_MICRO_CMO_WAITING_VALID_MULTIWORD_CASES];
        uint64_t observed_older[AI_MICRO_CMO_WAITING_VALID_MULTIWORD_CASES];
        uint64_t observed_younger0[AI_MICRO_CMO_WAITING_VALID_MULTIWORD_CASES];
        uint64_t observed_younger1[AI_MICRO_CMO_WAITING_VALID_MULTIWORD_CASES];
        uint64_t observed_untouched[AI_MICRO_CMO_WAITING_VALID_MULTIWORD_CASES];
        uint64_t expected_untouched[AI_MICRO_CMO_WAITING_VALID_MULTIWORD_CASES];
        uint64_t leading_guard[AI_MICRO_CMO_WAITING_VALID_MULTIWORD_CASES];
        uint64_t trailing_guard[AI_MICRO_CMO_WAITING_VALID_MULTIWORD_CASES];

        goto_priv(PRIV_M);
        for (unsigned i = 0; i < AI_MICRO_CMO_WAITING_VALID_MULTIWORD_CASES; ++i) {
            ai_micro_cmo_seed_block(base_paddr[i], seeds[i][round]);
            leading_guard[i] = 0xc900000000000000ULL |
                ((uint64_t)round << 16) | ((uint64_t)i << 8) | 0x27ULL;
            trailing_guard[i] = 0xda00000000000000ULL |
                ((uint64_t)round << 16) | ((uint64_t)i << 8) | 0x6eULL;
            write64(leading_paddr[i], leading_guard[i]);
            write64(trailing_paddr[i], trailing_guard[i]);
            expected_untouched[i] = seeds[i][round] +
                ((uint64_t)untouched_word_offsets[i] * 0x1111111111111111ULL);
        }

        goto_priv(PRIV_HS);
        hspt_init();
        sfence_vma();

        TEST_SETUP_EXCEPT();
        for (unsigned i = 0; i < AI_MICRO_CMO_WAITING_VALID_MULTIWORD_CASES; ++i) {
            warmed_older[i] = ld(older_word_vaddr[i]);
            warmed_younger0[i] = ld(younger0_word_vaddr[i]);
            warmed_younger1[i] = ld(younger1_word_vaddr[i]);
            warmed_untouched[i] = ld(untouched_word_vaddr[i]);
        }
        TEST_ASSERT("before the same-block waiting-valid multiword merge probe, all queried words are readable with their seeded payloads",
            excpt.triggered == false &&
            warmed_older[0] == (seeds[0][round] +
                ((uint64_t)older_word_offsets[0] * 0x1111111111111111ULL)) &&
            warmed_younger0[0] == (seeds[0][round] +
                ((uint64_t)younger0_word_offsets[0] * 0x1111111111111111ULL)) &&
            warmed_younger1[0] == (seeds[0][round] +
                ((uint64_t)younger1_word_offsets[0] * 0x1111111111111111ULL)) &&
            warmed_untouched[0] == expected_untouched[0] &&
            warmed_older[1] == (seeds[1][round] +
                ((uint64_t)older_word_offsets[1] * 0x1111111111111111ULL)) &&
            warmed_younger0[1] == (seeds[1][round] +
                ((uint64_t)younger0_word_offsets[1] * 0x1111111111111111ULL)) &&
            warmed_younger1[1] == (seeds[1][round] +
                ((uint64_t)younger1_word_offsets[1] * 0x1111111111111111ULL)) &&
            warmed_untouched[1] == expected_untouched[1] &&
            warmed_older[2] == (seeds[2][round] +
                ((uint64_t)older_word_offsets[2] * 0x1111111111111111ULL)) &&
            warmed_younger0[2] == (seeds[2][round] +
                ((uint64_t)younger0_word_offsets[2] * 0x1111111111111111ULL)) &&
            warmed_younger1[2] == (seeds[2][round] +
                ((uint64_t)younger1_word_offsets[2] * 0x1111111111111111ULL)) &&
            warmed_untouched[2] == expected_untouched[2]
        );

        TEST_SETUP_EXCEPT();
        for (unsigned i = 0; i < AI_MICRO_CMO_WAITING_VALID_MULTIWORD_CASES; ++i) {
            sd(older_word_vaddr[i], older_store_values[i][round]);
            cbo_flush(flush_vaddr[i]);
            sd(younger0_word_vaddr[i], younger0_values[i][round]);
            sd(younger1_word_vaddr[i], younger1_values[i][round]);
            observed_older[i] = ld(older_word_vaddr[i]);
            observed_younger0[i] = ld(younger0_word_vaddr[i]);
            observed_younger1[i] = ld(younger1_word_vaddr[i]);
            observed_untouched[i] = ld(untouched_word_vaddr[i]);
        }
        TEST_ASSERT("the older-word store, same-block cbo.flush, and two younger different-word stores all retire before the immediate per-word load checks without exception",
            excpt.triggered == false
        );

        TEST_ASSERT("one waiting valid entry can accumulate later stores to multiple words in the same block: the older inflight word, both younger words, and an untouched word all read back their own correct payloads with no duplicate-line bleed or dropped word",
            observed_older[0] == older_store_values[0][round] &&
            observed_younger0[0] == younger0_values[0][round] &&
            observed_younger1[0] == younger1_values[0][round] &&
            observed_untouched[0] == expected_untouched[0] &&
            observed_older[1] == older_store_values[1][round] &&
            observed_younger0[1] == younger0_values[1][round] &&
            observed_younger1[1] == younger1_values[1][round] &&
            observed_untouched[1] == expected_untouched[1] &&
            observed_older[2] == older_store_values[2][round] &&
            observed_younger0[2] == younger0_values[2][round] &&
            observed_younger1[2] == younger1_values[2][round] &&
            observed_untouched[2] == expected_untouched[2]
        );

        goto_priv(PRIV_HS);
        TEST_SETUP_EXCEPT();
        fence_iorw_iorw();
        TEST_ASSERT("the ordering fence after the same-block waiting-valid multiword merge probe retires without exception",
            excpt.triggered == false
        );

        goto_priv(PRIV_M);
        {
            bool backing_ok = true;
            for (unsigned i = 0; i < AI_MICRO_CMO_WAITING_VALID_MULTIWORD_CASES; ++i) {
                backing_ok = backing_ok &&
                    ai_micro_cmo_block_matches_seed_except_three_words(
                        base_paddr[i],
                        seeds[i][round],
                        older_word_offsets[i],
                        older_store_values[i][round],
                        younger0_word_offsets[i],
                        younger0_values[i][round],
                        younger1_word_offsets[i],
                        younger1_values[i][round]) &&
                    (!check_adjacent ||
                        (read64(leading_paddr[i]) == leading_guard[i] &&
                         read64(trailing_paddr[i]) == trailing_guard[i]));
            }
            TEST_ASSERT("after drain, the older word and both younger merged words persist together, untouched words stay seeded, and adjacent guards remain unchanged when requested",
                backing_ok
            );
        }
    }

    goto_priv(PRIV_M);
    ai_micro_cmo_reset_env(MENVCFG_CBZFE);

    return test_status;
}

static bool ai_micro_cmo_sbuffer_reinflight_handoff_same_word_window(
    bool check_adjacent)
{
    enum {
        AI_MICRO_CMO_REINFLIGHT_HANDOFF_CASES = 3,
        AI_MICRO_CMO_REINFLIGHT_HANDOFF_ROUNDS = 3
    };
    const uintptr_t block_offsets[AI_MICRO_CMO_REINFLIGHT_HANDOFF_CASES] = {
        AI_MICRO_CMO_BLOCK_OFFSET * 41U,
        AI_MICRO_CMO_BLOCK_OFFSET * 43U,
        AI_MICRO_CMO_BLOCK_OFFSET * 45U,
    };
    const unsigned older_word_offsets[AI_MICRO_CMO_REINFLIGHT_HANDOFF_CASES] = {
        0U,
        2U,
        5U,
    };
    const unsigned target_word_offsets[AI_MICRO_CMO_REINFLIGHT_HANDOFF_CASES] = {
        1U,
        4U,
        6U,
    };
    const unsigned untouched_word_offsets[AI_MICRO_CMO_REINFLIGHT_HANDOFF_CASES] = {
        6U,
        7U,
        1U,
    };
    const unsigned flush1_offsets[AI_MICRO_CMO_REINFLIGHT_HANDOFF_CASES] = {
        8U,
        31U,
        47U,
    };
    const unsigned flush2_offsets[AI_MICRO_CMO_REINFLIGHT_HANDOFF_CASES] = {
        40U,
        5U,
        60U,
    };
    const unsigned store_offsets[AI_MICRO_CMO_REINFLIGHT_HANDOFF_CASES] = {
        2U,
        1U,
        4U,
    };
    const unsigned store_sizes[AI_MICRO_CMO_REINFLIGHT_HANDOFF_CASES] = {
        2U,
        1U,
        4U,
    };
    const unsigned load_offsets[AI_MICRO_CMO_REINFLIGHT_HANDOFF_CASES] = {
        0U,
        0U,
        4U,
    };
    const unsigned load_sizes[AI_MICRO_CMO_REINFLIGHT_HANDOFF_CASES] = {
        8U,
        4U,
        4U,
    };
    const bool load_signed[AI_MICRO_CMO_REINFLIGHT_HANDOFF_CASES] = {
        false,
        true,
        false,
    };
    const uint64_t seeds[AI_MICRO_CMO_REINFLIGHT_HANDOFF_CASES][AI_MICRO_CMO_REINFLIGHT_HANDOFF_ROUNDS] = {
        { 0x1021324354657687ULL, 0x5566778899aabbccULL, 0x89abcdef01234567ULL },
        { 0x0fedcba987654321ULL, 0xa1b2c3d4e5f60718ULL, 0x1122334455667788ULL },
        { 0x3141592653589793ULL, 0x2718281828459045ULL, 0x13579bdf2468ace0ULL },
    };
    const uint64_t older_store_values[AI_MICRO_CMO_REINFLIGHT_HANDOFF_CASES][AI_MICRO_CMO_REINFLIGHT_HANDOFF_ROUNDS] = {
        { 0x8877665544332211ULL, 0x0123456789abcdefULL, 0xfedcba9876543210ULL },
        { 0x1122334455667788ULL, 0x99aabbccddeeff00ULL, 0x7766554433221100ULL },
        { 0xa1b2c3d4e5f60718ULL, 0x0f1e2d3c4b5a6978ULL, 0x8899aabbccddeeffULL },
    };
    const uint64_t target_store_values[AI_MICRO_CMO_REINFLIGHT_HANDOFF_CASES][AI_MICRO_CMO_REINFLIGHT_HANDOFF_ROUNDS] = {
        { 0x0102030405060708ULL, 0x1817161514131211ULL, 0x2233445566778899ULL },
        { 0xdeadc0decafef00dULL, 0x0badf00d12345678ULL, 0x5566aa55cc33ee77ULL },
        { 0x55aa33cc77ee11ffULL, 0x1234fedc5678ba98ULL, 0x76543210fedcba98ULL },
    };
    const uint64_t late_store_values[AI_MICRO_CMO_REINFLIGHT_HANDOFF_CASES][AI_MICRO_CMO_REINFLIGHT_HANDOFF_ROUNDS] = {
        { 0x000000000000beefULL, 0x000000000000cafeULL, 0x0000000000001357ULL },
        { 0x00000000000000a5ULL, 0x000000000000004cULL, 0x00000000000000d2ULL },
        { 0x89abcdefULL, 0x76543210ULL, 0x13579bdfULL },
    };
    uintptr_t base_vaddr[AI_MICRO_CMO_REINFLIGHT_HANDOFF_CASES];
    uintptr_t base_paddr[AI_MICRO_CMO_REINFLIGHT_HANDOFF_CASES];
    uintptr_t older_word_vaddr[AI_MICRO_CMO_REINFLIGHT_HANDOFF_CASES];
    uintptr_t target_word_vaddr[AI_MICRO_CMO_REINFLIGHT_HANDOFF_CASES];
    uintptr_t untouched_word_vaddr[AI_MICRO_CMO_REINFLIGHT_HANDOFF_CASES];
    uintptr_t flush1_vaddr[AI_MICRO_CMO_REINFLIGHT_HANDOFF_CASES];
    uintptr_t flush2_vaddr[AI_MICRO_CMO_REINFLIGHT_HANDOFF_CASES];
    uintptr_t late_store_vaddr[AI_MICRO_CMO_REINFLIGHT_HANDOFF_CASES];
    uintptr_t load_vaddr[AI_MICRO_CMO_REINFLIGHT_HANDOFF_CASES];
    uintptr_t leading_paddr[AI_MICRO_CMO_REINFLIGHT_HANDOFF_CASES];
    uintptr_t trailing_paddr[AI_MICRO_CMO_REINFLIGHT_HANDOFF_CASES];
    bool test_status = true;

    for (unsigned i = 0; i < AI_MICRO_CMO_REINFLIGHT_HANDOFF_CASES; ++i) {
        base_vaddr[i] = hs_page_base(AI_MICRO_CMO_CBOM_PAGE) + block_offsets[i];
        base_paddr[i] = phys_page_base(AI_MICRO_CMO_CBOM_PAGE) + block_offsets[i];
        older_word_vaddr[i] = base_vaddr[i] + (older_word_offsets[i] * sizeof(uint64_t));
        target_word_vaddr[i] = base_vaddr[i] + (target_word_offsets[i] * sizeof(uint64_t));
        untouched_word_vaddr[i] = base_vaddr[i] + (untouched_word_offsets[i] * sizeof(uint64_t));
        flush1_vaddr[i] = base_vaddr[i] + flush1_offsets[i];
        flush2_vaddr[i] = base_vaddr[i] + flush2_offsets[i];
        late_store_vaddr[i] = target_word_vaddr[i] + store_offsets[i];
        load_vaddr[i] = target_word_vaddr[i] + load_offsets[i];
        leading_paddr[i] = base_paddr[i] - sizeof(uint64_t);
        trailing_paddr[i] = base_paddr[i] + (AI_MICRO_CMO_BLOCK_WORDS * sizeof(uint64_t));
    }

    goto_priv(PRIV_M);
    ai_micro_cmo_prepare_hs_env(MENVCFG_CBZFE);

    for (unsigned round = 0; round < AI_MICRO_CMO_REINFLIGHT_HANDOFF_ROUNDS; ++round) {
        uint64_t warmed_older[AI_MICRO_CMO_REINFLIGHT_HANDOFF_CASES];
        uint64_t warmed_target[AI_MICRO_CMO_REINFLIGHT_HANDOFF_CASES];
        uint64_t warmed_untouched[AI_MICRO_CMO_REINFLIGHT_HANDOFF_CASES];
        uint64_t observed_older[AI_MICRO_CMO_REINFLIGHT_HANDOFF_CASES];
        uint64_t observed_target[AI_MICRO_CMO_REINFLIGHT_HANDOFF_CASES];
        uint64_t observed_untouched[AI_MICRO_CMO_REINFLIGHT_HANDOFF_CASES];
        uint64_t expected_target_word[AI_MICRO_CMO_REINFLIGHT_HANDOFF_CASES];
        uint64_t expected_target_load[AI_MICRO_CMO_REINFLIGHT_HANDOFF_CASES];
        uint64_t expected_untouched[AI_MICRO_CMO_REINFLIGHT_HANDOFF_CASES];
        uint64_t leading_guard[AI_MICRO_CMO_REINFLIGHT_HANDOFF_CASES];
        uint64_t trailing_guard[AI_MICRO_CMO_REINFLIGHT_HANDOFF_CASES];

        goto_priv(PRIV_M);
        for (unsigned i = 0; i < AI_MICRO_CMO_REINFLIGHT_HANDOFF_CASES; ++i) {
            ai_micro_cmo_seed_block(base_paddr[i], seeds[i][round]);
            leading_guard[i] = 0xeb00000000000000ULL |
                ((uint64_t)round << 16) | ((uint64_t)i << 8) | 0x28ULL;
            trailing_guard[i] = 0xfc00000000000000ULL |
                ((uint64_t)round << 16) | ((uint64_t)i << 8) | 0x71ULL;
            write64(leading_paddr[i], leading_guard[i]);
            write64(trailing_paddr[i], trailing_guard[i]);
            expected_untouched[i] = seeds[i][round] +
                ((uint64_t)untouched_word_offsets[i] * 0x1111111111111111ULL);
        }

        goto_priv(PRIV_HS);
        hspt_init();
        sfence_vma();

        TEST_SETUP_EXCEPT();
        for (unsigned i = 0; i < AI_MICRO_CMO_REINFLIGHT_HANDOFF_CASES; ++i) {
            warmed_older[i] = ld(older_word_vaddr[i]);
            warmed_target[i] = ld(target_word_vaddr[i]);
            warmed_untouched[i] = ld(untouched_word_vaddr[i]);
        }
        TEST_ASSERT("before the repeated same-block reinflight handoff probe, the older word, target word, and untouched word are all readable with their seeded payloads",
            excpt.triggered == false &&
            warmed_older[0] == (seeds[0][round] +
                ((uint64_t)older_word_offsets[0] * 0x1111111111111111ULL)) &&
            warmed_target[0] == (seeds[0][round] +
                ((uint64_t)target_word_offsets[0] * 0x1111111111111111ULL)) &&
            warmed_untouched[0] == expected_untouched[0] &&
            warmed_older[1] == (seeds[1][round] +
                ((uint64_t)older_word_offsets[1] * 0x1111111111111111ULL)) &&
            warmed_target[1] == (seeds[1][round] +
                ((uint64_t)target_word_offsets[1] * 0x1111111111111111ULL)) &&
            warmed_untouched[1] == expected_untouched[1] &&
            warmed_older[2] == (seeds[2][round] +
                ((uint64_t)older_word_offsets[2] * 0x1111111111111111ULL)) &&
            warmed_target[2] == (seeds[2][round] +
                ((uint64_t)target_word_offsets[2] * 0x1111111111111111ULL)) &&
            warmed_untouched[2] == expected_untouched[2]
        );

        TEST_SETUP_EXCEPT();
        for (unsigned i = 0; i < AI_MICRO_CMO_REINFLIGHT_HANDOFF_CASES; ++i) {
            sd(older_word_vaddr[i], older_store_values[i][round]);
            cbo_flush(flush1_vaddr[i]);
            sd(target_word_vaddr[i], target_store_values[i][round]);
            cbo_flush(flush2_vaddr[i]);

            switch (store_sizes[i]) {
            case 1U:
                sb(late_store_vaddr[i], (uint8_t)late_store_values[i][round]);
                break;
            case 2U:
                sh(late_store_vaddr[i], (uint16_t)late_store_values[i][round]);
                break;
            case 4U:
                sw(late_store_vaddr[i], (uint32_t)late_store_values[i][round]);
                break;
            default:
                break;
            }

            observed_older[i] = ld(older_word_vaddr[i]);
            if (load_sizes[i] == 8U) {
                observed_target[i] = ld(load_vaddr[i]);
            } else if (load_signed[i]) {
                observed_target[i] = ai_micro_cmo_signed_lw_value(load_vaddr[i]);
            } else if (load_sizes[i] == 4U) {
                observed_target[i] = lwu(load_vaddr[i]);
            } else {
                observed_target[i] = lhu(load_vaddr[i]);
            }
            observed_untouched[i] = ld(untouched_word_vaddr[i]);
        }
        TEST_ASSERT("the older store, two same-block cbo.flush probes, younger full store, and final same-word partial store all retire before the reinflight handoff load checks without exception",
            excpt.triggered == false
        );

        goto_priv(PRIV_M);
        for (unsigned i = 0; i < AI_MICRO_CMO_REINFLIGHT_HANDOFF_CASES; ++i) {
            expected_target_word[i] = ai_micro_cmo_merge_partial_store_into_word(
                target_store_values[i][round],
                store_offsets[i],
                store_sizes[i],
                late_store_values[i][round]);
            expected_target_load[i] = load_signed[i] ?
                ai_micro_cmo_expected_signed_window_from_word(
                    expected_target_word[i], load_offsets[i], load_sizes[i]) :
                ai_micro_cmo_expected_unsigned_window_from_word(
                    expected_target_word[i], load_offsets[i], load_sizes[i]);
        }

        TEST_ASSERT("after the repeated same-block reinflight handoff, the original older word stays bound to its own payload, the target word keeps only the newer generation plus final partial merge, and an untouched word remains seeded",
            observed_older[0] == older_store_values[0][round] &&
            observed_target[0] == expected_target_load[0] &&
            observed_untouched[0] == expected_untouched[0] &&
            observed_older[1] == older_store_values[1][round] &&
            observed_target[1] == expected_target_load[1] &&
            observed_untouched[1] == expected_untouched[1] &&
            observed_older[2] == older_store_values[2][round] &&
            observed_target[2] == expected_target_load[2] &&
            observed_untouched[2] == expected_untouched[2]
        );

        goto_priv(PRIV_HS);
        TEST_SETUP_EXCEPT();
        fence_iorw_iorw();
        TEST_ASSERT("the ordering fence after the repeated same-block reinflight handoff probe retires without exception",
            excpt.triggered == false
        );

        goto_priv(PRIV_M);
        {
            bool backing_ok = true;
            for (unsigned i = 0; i < AI_MICRO_CMO_REINFLIGHT_HANDOFF_CASES; ++i) {
                backing_ok = backing_ok &&
                    ai_micro_cmo_block_matches_seed_except_two_words(
                        base_paddr[i],
                        seeds[i][round],
                        older_word_offsets[i],
                        older_store_values[i][round],
                        target_word_offsets[i],
                        expected_target_word[i]) &&
                    (!check_adjacent ||
                        (read64(leading_paddr[i]) == leading_guard[i] &&
                         read64(trailing_paddr[i]) == trailing_guard[i]));
            }
            TEST_ASSERT("after drain, the older word and reinflight target word persist with their final images, untouched words stay seeded, and adjacent guards remain unchanged when requested",
                backing_ok
            );
        }
    }

    goto_priv(PRIV_M);
    ai_micro_cmo_reset_env(MENVCFG_CBZFE);

    return test_status;
}

static bool ai_micro_sbuffer_hitresp_delayed_mask_clear_window(bool dense_pressure)
{
    const uintptr_t target_base_vaddr = hs_page_base(AI_MICRO_CMO_CBOM_PAGE) +
        (AI_MICRO_CMO_BLOCK_OFFSET * 56U);
    const uintptr_t target_base_paddr = phys_page_base(AI_MICRO_CMO_CBOM_PAGE) +
        (AI_MICRO_CMO_BLOCK_OFFSET * 56U);
    const uintptr_t later_base_vaddrs[] = {
        hs_page_base(AI_MICRO_CMO_CBOZERO_PAGE) + (AI_MICRO_CMO_BLOCK_OFFSET * 8U),
        hs_page_base(AI_MICRO_CMO_CBOZERO_PAGE) + (AI_MICRO_CMO_BLOCK_OFFSET * 9U),
        hs_page_base(AI_MICRO_CMO_CBOZERO_PAGE) + (AI_MICRO_CMO_BLOCK_OFFSET * 10U),
    };
    const uintptr_t later_base_paddrs[] = {
        phys_page_base(AI_MICRO_CMO_CBOZERO_PAGE) + (AI_MICRO_CMO_BLOCK_OFFSET * 8U),
        phys_page_base(AI_MICRO_CMO_CBOZERO_PAGE) + (AI_MICRO_CMO_BLOCK_OFFSET * 9U),
        phys_page_base(AI_MICRO_CMO_CBOZERO_PAGE) + (AI_MICRO_CMO_BLOCK_OFFSET * 10U),
    };
    const size_t later_word_offsets[] = { 0U, 2U, 5U };
    const uint64_t target_seed = 0x6162636465666700ULL;
    const uint64_t later_seeds[] = {
        0x7172737475767700ULL,
        0x8182838485868700ULL,
        0x9192939495969700ULL,
    };
    const uint64_t later_update_bases[] = {
        0xa1b2c3d4e5f60000ULL,
        0xb1c2d3e4f5060000ULL,
        0xc1d2e3f405160000ULL,
    };
    const size_t later_count = dense_pressure ? 3U : 1U;
    const size_t rounds = dense_pressure ? 6U : 8U;
    uint64_t target_expected = target_seed;
    uint64_t later_expected[] = {
        later_seeds[0],
        later_seeds[1] + (2U * 0x1111111111111111ULL),
        later_seeds[2] + (5U * 0x1111111111111111ULL),
    };
    bool test_status = true;

    goto_priv(PRIV_M);
    ai_micro_cmo_seed_block(target_base_paddr, target_seed);
    for (size_t i = 0; i < 3U; ++i) {
        ai_micro_cmo_seed_block(later_base_paddrs[i], later_seeds[i]);
    }
    ai_micro_cmo_prepare_hs_env(MENVCFG_CBZFE);

    goto_priv(PRIV_HS);
    TEST_SETUP_EXCEPT();
    uint64_t warmed_target = ld(target_base_vaddr);
    AI_ASSERT_NO_EXCEPT_AND("the target cache line is readable before the delayed hit-response mask-clear reuse window begins",
        warmed_target == target_seed
    );

    for (size_t i = 0; i < later_count; ++i) {
        TEST_SETUP_EXCEPT();
        uint64_t warmed_later = ld(later_base_vaddrs[i] +
            (later_word_offsets[i] * sizeof(uint64_t)));
        AI_ASSERT_NO_EXCEPT_AND("the later cacheable store targets are readable before the delayed mask-clear reuse window begins",
            warmed_later == later_expected[i]
        );
    }

    for (size_t round = 0; round < rounds; ++round) {
        target_expected = 0xd1e2f30415260000ULL + ((uint64_t)round << 8) + 0x5aULL;

        TEST_SETUP_EXCEPT();
        sd(target_base_vaddr, target_expected);
        AI_ASSERT_NO_EXCEPT_AND("the older target store retires before the delayed mask-clear reuse probe",
            true
        );

        TEST_SETUP_EXCEPT();
        cbo_flush(target_base_vaddr + ((round & 1U) ? 8U : 40U));
        AI_ASSERT_NO_EXCEPT_AND("the same-line cbo.flush retires while the older store enters the hit-response cleanup window",
            true
        );

        for (size_t i = 0; i < later_count; ++i) {
            later_expected[i] = later_update_bases[i] + ((uint64_t)round << 8) +
                (uint64_t)(0x31U + i);

            TEST_SETUP_EXCEPT();
            sd(later_base_vaddrs[i] + (later_word_offsets[i] * sizeof(uint64_t)),
                later_expected[i]);
            AI_ASSERT_NO_EXCEPT_AND("the immediately following later cacheable store retires without exception in the delayed mask-clear reuse window",
                true
            );
        }
    }

    TEST_SETUP_EXCEPT();
    fence_iorw_iorw();
    AI_ASSERT_NO_EXCEPT_AND("the ordering fence after the delayed hit-response mask-clear reuse window retires without exception",
        true
    );

    TEST_SETUP_EXCEPT();
    uint64_t observed_target = ld(target_base_vaddr);
    AI_ASSERT_NO_EXCEPT_AND("after the delayed hit-response mask-clear reuse window, the older target line keeps only its latest payload",
        observed_target == target_expected
    );

    for (size_t i = 0; i < later_count; ++i) {
        TEST_SETUP_EXCEPT();
        uint64_t observed_later = ld(later_base_vaddrs[i] +
            (later_word_offsets[i] * sizeof(uint64_t)));
        AI_ASSERT_NO_EXCEPT_AND("after the delayed hit-response mask-clear reuse window, each later store target keeps only its own latest payload",
            observed_later == later_expected[i]
        );
    }

    goto_priv(PRIV_M);
    bool later_backing_ok = true;
    for (size_t i = 0; i < later_count; ++i) {
        later_backing_ok = later_backing_ok &&
            ai_micro_cmo_block_matches_seed_except_word(later_base_paddrs[i],
                later_seeds[i], later_word_offsets[i], later_expected[i]);
    }
    TEST_ASSERT("the delayed hit-response mask-clear reuse approximation preserves the latest target payload and all later independent store payloads without cross-line mask loss",
        ai_micro_cmo_block_matches_seed_except_word(target_base_paddr,
            target_seed, 0U, target_expected) &&
        later_backing_ok
    );
    ai_micro_cmo_reset_env(MENVCFG_CBZFE);

    goto_priv(PRIV_HS);
    hspt_init();
    sfence_vma();

    return test_status;
}

#endif /* AI_MICRO_CMO_COMMON_H */
