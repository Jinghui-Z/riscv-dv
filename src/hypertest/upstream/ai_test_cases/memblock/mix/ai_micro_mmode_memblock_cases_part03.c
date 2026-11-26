#include "ai_micro_mmode_memblock_common.h"

bool ai_micro_m_samepage_cross_16b_cross_producer_same_width_template_fault_switch_store_then_load_keeps_latest_cause_and_tval_corner()
{
    TEST_START();

    static const uintptr_t store_offsets[2] = {
        AI_MICRO_MEMBLOCK_CROSS_16B_BOUNDARY0_SD_4B4B_OFFSET,
        AI_MICRO_MEMBLOCK_CROSS_16B_BOUNDARY1_SD_OFFSET,
    };
    static const uintptr_t load_offsets[2] = {
        AI_MICRO_MEMBLOCK_CROSS_16B_BOUNDARY1_SD_OFFSET,
        AI_MICRO_MEMBLOCK_CROSS_16B_BOUNDARY0_SD_4B4B_OFFSET,
    };
    static const uint64_t store_values[2][2] = {
        { 0x0123456789abcdefULL, 0x13579bdf2468ace0ULL },
        { 0xf0e1d2c3b4a59687ULL, 0x89abcdef01234567ULL },
    };
    static const uint8_t init_patterns[2][2][16] = {
        {
            { 0xe2, 0xd3, 0xc4, 0xb5, 0xa6, 0x97, 0x88, 0x79,
              0x6a, 0x5b, 0x4c, 0x3d, 0x2e, 0x1f, 0x10, 0x01 },
            { 0x24, 0x35, 0x46, 0x57, 0x68, 0x79, 0x8a, 0x9b,
              0xac, 0xbd, 0xce, 0xdf, 0xe0, 0xf1, 0x02, 0x13 },
        },
        {
            { 0x5c, 0x4d, 0x3e, 0x2f, 0x10, 0x01, 0xf2, 0xe3,
              0xd4, 0xc5, 0xb6, 0xa7, 0x98, 0x89, 0x7a, 0x6b },
            { 0xb3, 0xa4, 0x95, 0x86, 0x77, 0x68, 0x59, 0x4a,
              0x3b, 0x2c, 0x1d, 0x0e, 0xff, 0xe1, 0xd2, 0xc3 },
        },
    };
    static const uint8_t guard_before_patterns[2][8] = {
        { 0x16, 0x28, 0x3a, 0x4c, 0x5e, 0x60, 0x72, 0x84 },
        { 0x8e, 0x7c, 0x6a, 0x58, 0x46, 0x34, 0x22, 0x10 },
    };
    static const uint8_t guard_middle_patterns[2][16] = {
        { 0xb0, 0xc2, 0xd4, 0xe6, 0xf8, 0x0a, 0x1c, 0x2e,
          0x3f, 0x50, 0x61, 0x72, 0x83, 0x94, 0xa5, 0xb6 },
        { 0x19, 0x2b, 0x3d, 0x4f, 0x51, 0x63, 0x75, 0x87,
          0x99, 0xab, 0xbd, 0xcf, 0xd1, 0xe3, 0xf5, 0x07 },
    };
    static const uint8_t guard_after_patterns[2][8] = {
        { 0x27, 0x49, 0x6b, 0x8d, 0xaf, 0xc1, 0xd3, 0xe5 },
        { 0x4d, 0x5f, 0x71, 0x83, 0x95, 0xa7, 0xb9, 0xcb },
    };
    const enum test_page page = SWITCH1;
    const uintptr_t boundary_phys[2] = {
        ai_micro_m_memblock_cross_16b_boundary_phys(0U),
        ai_micro_m_memblock_cross_16b_boundary_phys(1U),
    };
    const uintptr_t guard_before_phys =
        ai_micro_m_memblock_cross_16b_guard_before_phys();
    const uintptr_t guard_middle_phys =
        ai_micro_m_memblock_cross_16b_guard_middle_phys();
    const uintptr_t guard_after_phys =
        ai_micro_m_memblock_cross_16b_guard_after_phys();
    uint64_t observed_value = 0ULL;

    for (unsigned round = 0; round < 2U; ++round) {
        const unsigned first = round;
        const unsigned second = first ^ 1U;
        const uintptr_t store_vaddr =
            ai_micro_m_memblock_cross_16b_addr_with_offset(page,
                store_offsets[round]);
        const uintptr_t load_vaddr =
            ai_micro_m_memblock_cross_16b_addr_with_offset(page,
                load_offsets[round]);

        goto_priv(PRIV_M);
        ai_micro_m_memblock_reset_env();
        ai_micro_m_memblock_write_bytes(boundary_phys[0],
            init_patterns[0][round], 16U);
        ai_micro_m_memblock_write_bytes(boundary_phys[1],
            init_patterns[1][round], 16U);
        ai_micro_m_memblock_write_bytes(guard_before_phys,
            guard_before_patterns[round], 8U);
        ai_micro_m_memblock_write_bytes(guard_middle_phys,
            guard_middle_patterns[round], 16U);
        ai_micro_m_memblock_write_bytes(guard_after_phys,
            guard_after_patterns[round], 8U);
        ai_micro_m_memblock_configure_cross_16b_pmp_window(
            boundary_phys[first] + 8U, boundary_phys[first] + 16U,
            AI_MICRO_MEMBLOCK_PMP_TOR_NO_W);

        ai_micro_m_memblock_prepare_supervisor_translation();
        ai_micro_m_memblock_set_cross_16b_leaves(PTE_V | PTE_AD | PTE_RWX);
        sfence_vma();

        TEST_SETUP_EXCEPT();
        ai_micro_m_memblock_translated_sd(store_vaddr, store_values[first][round]);
        TEST_ASSERT("the first no-H same-page cross-16B cross-producer same-width different-template store fault reports a store access fault with tval bound to the original misaligned address",
            excpt.triggered == true &&
            excpt.cause == CAUSE_SAF &&
            excpt.tval == ai_micro_m_memblock_cross_16b_second_half_vaddr(store_vaddr)
        );

        goto_priv(PRIV_M);
        TEST_ASSERT("after the first no-H same-page cross-16B cross-producer same-width different-template store fault, both boundary windows and all guard regions remain unchanged before the follow-up load fault",
            ai_micro_m_memblock_bytes_match(boundary_phys[0],
                init_patterns[0][round], 16U) &&
            ai_micro_m_memblock_bytes_match(boundary_phys[1],
                init_patterns[1][round], 16U) &&
            ai_micro_m_memblock_bytes_match(guard_before_phys,
                guard_before_patterns[round], 8U) &&
            ai_micro_m_memblock_bytes_match(guard_middle_phys,
                guard_middle_patterns[round], 16U) &&
            ai_micro_m_memblock_bytes_match(guard_after_phys,
                guard_after_patterns[round], 8U)
        );

        ai_micro_m_memblock_configure_cross_16b_pmp_window(
            boundary_phys[second] + 8U, boundary_phys[second] + 16U,
            AI_MICRO_MEMBLOCK_PMP_TOR_NO_R);

        TEST_SETUP_EXCEPT();
        observed_value = ai_micro_m_memblock_translated_scalar_load(
            false, sizeof(uint64_t), load_vaddr);
        TEST_ASSERT("after the earlier same-page cross-16B cross-producer same-width different-template store fault has already occupied the shared exception path, the follow-up load fault must refresh cause and tval to the newer template-specific address",
            excpt.triggered == true &&
            excpt.cause == CAUSE_LAF &&
            excpt.tval == ai_micro_m_memblock_cross_16b_second_half_vaddr(load_vaddr)
        );

        goto_priv(PRIV_M);
        TEST_ASSERT("the same-page cross-16B cross-producer same-width different-template store->load fault-switch sequence leaves both boundary windows and all guard regions unchanged because both faults are side-effect free",
            ai_micro_m_memblock_bytes_match(boundary_phys[0],
                init_patterns[0][round], 16U) &&
            ai_micro_m_memblock_bytes_match(boundary_phys[1],
                init_patterns[1][round], 16U) &&
            ai_micro_m_memblock_bytes_match(guard_before_phys,
                guard_before_patterns[round], 8U) &&
            ai_micro_m_memblock_bytes_match(guard_middle_phys,
                guard_middle_patterns[round], 16U) &&
            ai_micro_m_memblock_bytes_match(guard_after_phys,
                guard_after_patterns[round], 8U)
        );
    }

    (void)observed_value;
    ai_micro_m_memblock_reset_env();

    TEST_END();
}

bool ai_micro_m_scalar_samepage_cross_16b_store_fake_crosspage_state_not_rebound_to_next_boundary_corner()
{
    TEST_START();

    test_status = test_status &&
        ai_micro_m_memblock_samepage_cross_16b_store_fake_crosspage_window(
            false);

    TEST_END();
}

bool ai_micro_m_scalar_samepage_cross_16b_store_fake_crosspage_state_adjacent_preserved_corner()
{
    TEST_START();

    test_status = test_status &&
        ai_micro_m_memblock_samepage_cross_16b_store_fake_crosspage_window(
            true);

    TEST_END();
}

bool ai_micro_m_scalar_samepage_cross_16b_store_width_switch_no_fake_crosspage_rebind_corner()
{
    TEST_START();

    test_status = test_status &&
        ai_micro_m_memblock_samepage_cross_16b_store_width_switch_window(
            false);

    TEST_END();
}

bool ai_micro_m_scalar_samepage_cross_16b_store_width_switch_adjacent_preserved_corner()
{
    TEST_START();

    test_status = test_status &&
        ai_micro_m_memblock_samepage_cross_16b_store_width_switch_window(
            true);

    TEST_END();
}

bool ai_micro_m_scalar_samepage_cross_16b_store_same_width_template_switch_no_stale_template_corner()
{
    TEST_START();

    test_status = test_status &&
        ai_micro_m_memblock_samepage_cross_16b_store_same_width_template_switch_window(
            false);

    TEST_END();
}

bool ai_micro_m_scalar_samepage_cross_16b_store_same_width_template_switch_adjacent_preserved_corner()
{
    TEST_START();

    test_status = test_status &&
        ai_micro_m_memblock_samepage_cross_16b_store_same_width_template_switch_window(
            true);

    TEST_END();
}

bool ai_micro_m_scalar_samepage_cross_16b_store_fault_path_same_width_template_switch_no_stale_template_corner()
{
    TEST_START();

    test_status = test_status &&
        ai_micro_m_memblock_samepage_cross_16b_store_fault_same_width_template_switch_window(
            false);

    TEST_END();
}

bool ai_micro_m_scalar_samepage_cross_16b_store_fault_path_same_width_template_switch_adjacent_preserved_corner()
{
    TEST_START();

    test_status = test_status &&
        ai_micro_m_memblock_samepage_cross_16b_store_fault_same_width_template_switch_window(
            true);

    TEST_END();
}

bool ai_micro_m_scalar_samepage_cross_16b_store_fault_path_state_not_rebound_to_next_boundary_corner()
{
    TEST_START();

    test_status = test_status &&
        ai_micro_m_memblock_samepage_cross_16b_store_fault_reuse_window(
            false);

    TEST_END();
}

bool ai_micro_m_scalar_samepage_cross_16b_store_fault_path_adjacent_preserved_corner()
{
    TEST_START();

    test_status = test_status &&
        ai_micro_m_memblock_samepage_cross_16b_store_fault_reuse_window(
            true);

    TEST_END();
}

bool ai_micro_m_scalar_samepage_cross_16b_store_fault_path_width_switch_no_fake_crosspage_rebind_corner()
{
    TEST_START();

    test_status = test_status &&
        ai_micro_m_memblock_samepage_cross_16b_store_fault_width_switch_window(
            false);

    TEST_END();
}

bool ai_micro_m_scalar_samepage_cross_16b_store_fault_path_width_switch_adjacent_preserved_corner()
{
    TEST_START();

    test_status = test_status &&
        ai_micro_m_memblock_samepage_cross_16b_store_fault_width_switch_window(
            true);

    TEST_END();
}

bool ai_micro_m_scalar_samepage_cross_16b_store_fault_path_same_address_width_switch_refreshes_current_template_corner()
{
    TEST_START();

    test_status = test_status &&
        ai_micro_m_memblock_samepage_cross_16b_store_fault_same_address_width_switch_window(
            false);

    TEST_END();
}

bool ai_micro_m_scalar_samepage_cross_16b_store_fault_path_same_address_width_switch_adjacent_preserved_corner()
{
    TEST_START();

    test_status = test_status &&
        ai_micro_m_memblock_samepage_cross_16b_store_fault_same_address_width_switch_window(
            true);

    TEST_END();
}

bool ai_micro_m_scalar_samepage_cross_16b_store_fault_path_repair_then_valid_next_boundary_store_then_new_fault_refreshes_tval_corner()
{
    TEST_START();

    test_status = test_status &&
        ai_micro_m_memblock_samepage_cross_16b_store_fault_repair_then_new_fault_refresh_window(
            false);

    TEST_END();
}

bool ai_micro_m_scalar_samepage_cross_16b_store_fault_path_repair_then_valid_next_boundary_store_then_original_boundary_new_fault_refreshes_back_tval_corner()
{
    TEST_START();

    test_status = test_status &&
        ai_micro_m_memblock_samepage_cross_16b_store_fault_repair_then_return_fault_refresh_window(
            false);

    TEST_END();
}

bool ai_micro_m_scalar_samepage_cross_16b_store_fault_path_repair_then_valid_next_boundary_store_then_original_boundary_new_fault_adjacent_preserved_corner()
{
    TEST_START();

    test_status = test_status &&
        ai_micro_m_memblock_samepage_cross_16b_store_fault_repair_then_return_fault_refresh_window(
            true);

    TEST_END();
}

bool ai_micro_m_scalar_samepage_cross_16b_store_fault_path_repair_then_valid_next_boundary_same_width_template_store_then_original_boundary_new_fault_refreshes_back_template_tval_corner()
{
    TEST_START();

    test_status = test_status &&
        ai_micro_m_memblock_samepage_cross_16b_store_fault_repair_then_return_fault_same_width_template_window(
            false);

    TEST_END();
}

bool ai_micro_m_scalar_samepage_cross_16b_store_fault_path_repair_then_valid_next_boundary_same_width_template_store_then_same_boundary_new_fault_refreshes_current_template_tval_corner()
{
    TEST_START();

    test_status = test_status &&
        ai_micro_m_memblock_samepage_cross_16b_store_fault_repair_then_next_boundary_template_fault_window(
            false);

    TEST_END();
}

bool ai_micro_m_pbmt_nc_replay_head_narrow_scalar_signedness_matches_current_uop_corner()
{
    TEST_START();

    const enum test_page alias_page = AI_MICRO_MEMBLOCK_PBMT_NC_ALIAS_PAGE;
    const enum test_page redirect_fault_page = VSI_GI;
    const enum test_page survivor_first_page = SWITCH1;
    const enum test_page survivor_second_page = VSRW_GRW;
    static const uintptr_t target_offsets[] = {
        0x180U,
        0x1c0U,
        0x200U,
    };
    static const unsigned size_bytes[] = {
        sizeof(uint8_t),
        sizeof(uint16_t),
        sizeof(uint32_t),
    };
    static const unsigned load_offsets[] = {
        7U,
        6U,
        4U,
    };
    static const uint64_t target_values[] = {
        0x81aa223344556677ULL,
        0x9234f67889abcdeeULL,
        0x89abcdef10203040ULL,
    };
    static const uint64_t survivor_first_values[] = {
        0x1021324354657687ULL,
        0x2031425364758697ULL,
        0x30415263748596a7ULL,
    };
    static const uint64_t survivor_second_values[] = {
        0x8899aabbccddeeffULL,
        0x99aabbccddeeff00ULL,
        0xaabbccddeeff0011ULL,
    };

    for (unsigned round = 0; round < 3U; ++round) {
        const uintptr_t target_paddr = phys_page_base(X) + target_offsets[round];
        const uintptr_t redirect_fault_vaddr =
            hs_page_base(redirect_fault_page) + target_offsets[round];
        const uintptr_t survivor_first_vaddr =
            hs_page_base(survivor_first_page) + target_offsets[round];
        const uintptr_t survivor_second_vaddr =
            hs_page_base(survivor_second_page) + target_offsets[round];
        const uintptr_t survivor_first_paddr =
            phys_page_base(survivor_first_page) + target_offsets[round];
        const uintptr_t survivor_second_paddr =
            phys_page_base(survivor_second_page) + target_offsets[round];
        const unsigned width = size_bytes[round];
        const unsigned offset = load_offsets[round];
        const bool fault_signed = (round & 1U) != 0U;
        const bool recovery_signed = !fault_signed;
        const uint64_t expected_scalar =
            ai_micro_m_memblock_extract_scalar(target_values[round], offset,
                width, recovery_signed);
        const uintptr_t alias_vaddr =
            hs_page_base(alias_page) + target_offsets[round];

        goto_priv(PRIV_M);
        ai_micro_m_memblock_reset_env();
        ai_micro_m_memblock_reset_pbmt_env();
        write64(survivor_first_paddr, survivor_first_values[round]);
        write64(survivor_second_paddr, survivor_second_values[round]);

        ai_micro_m_memblock_prepare_pbmt_translation();
        TEST_ASSERT("PBMT=NC replay-head signedness setup seeds the target doubleword through the same NC alias that the recovery load uses, avoiding cacheable-to-uncache visibility assumptions",
            ai_micro_m_memblock_seed_pbmt_alias_doubleword(
                alias_page, target_paddr, target_values[round])
        );
        (void)ai_micro_m_memblock_configure_pbmt_alias(alias_page, target_paddr);
        ai_micro_m_memblock_set_leaf(redirect_fault_page, 0ULL);
        sfence_vma();

        TEST_SETUP_EXCEPT();
        (void)ai_micro_m_memblock_translated_scalar_load(fault_signed,
            width, redirect_fault_vaddr + offset);
        TEST_ASSERT("before the no-H PBMT=NC replay-head signedness refresh probe, the redirect-driving invalid-leaf narrow load still reports a precise page fault on its own faulting address",
            excpt.triggered == true &&
            excpt.cause == CAUSE_LPF &&
            excpt.tval == redirect_fault_vaddr + offset
        );

        TEST_SETUP_EXCEPT();
        uint64_t observed_survivor_first =
            ai_micro_m_memblock_translated_ld(survivor_first_vaddr);
        TEST_ASSERT("in the no-H PBMT=NC replay-head signedness refresh probe, the first surviving cacheable load still returns only its own payload after the redirect-driving fault",
            excpt.triggered == false &&
            observed_survivor_first == survivor_first_values[round]
        );

        TEST_SETUP_EXCEPT();
        uint64_t observed_scalar =
            ai_micro_m_memblock_translated_scalar_load(recovery_signed, width,
                alias_vaddr + offset);
        TEST_ASSERT("after the redirect-driving narrow fault, the no-H PBMT=NC replay-head scalar load must use its current width and signedness instead of inheriting stale sign/zero-extension state",
            excpt.triggered == false &&
            observed_scalar == expected_scalar
        );

        TEST_SETUP_EXCEPT();
        uint64_t observed_survivor_second =
            ai_micro_m_memblock_translated_ld(survivor_second_vaddr);
        TEST_ASSERT("in the no-H PBMT=NC replay-head signedness refresh probe, the second surviving cacheable load also keeps only its own payload after the uncache load retires",
            excpt.triggered == false &&
            observed_survivor_second == survivor_second_values[round]
        );

        goto_priv(PRIV_M);
        TEST_ASSERT("the no-H PBMT=NC replay-head signedness refresh probe does not perturb either surviving cacheable target or the backing line that feeds the uncache alias",
            read64(target_paddr) == target_values[round] &&
            read64(survivor_first_paddr) == survivor_first_values[round] &&
            read64(survivor_second_paddr) == survivor_second_values[round]
        );
    }

    ai_micro_m_memblock_reset_pbmt_env();
    ai_micro_m_memblock_reset_env();

    TEST_END();
}

bool ai_micro_m_store_pbmt_nc_storequeue_head_narrow_width_switch_exact_byte_age_merge_corner()
{
    TEST_START();

    const enum test_page alias_page = AI_MICRO_MEMBLOCK_PBMT_NC_ALIAS_PAGE;
    static const uint64_t initial_values[] = {
        0x1817161514131211ULL,
        0x2827262524232221ULL,
    };
    static const uint64_t adjacent_values[] = {
        0x8877665544332211ULL,
        0x99aabbccddeeff00ULL,
    };
    static const unsigned store_sizes[] = {
        sizeof(uint8_t),
        sizeof(uint16_t),
        sizeof(uint32_t),
    };
    static const unsigned store_offsets[] = {
        7U,
        2U,
        4U,
    };
    static const uint64_t store_values[] = {
        0x00000000000000a5ULL,
        0x000000000000b4c3ULL,
        0x00000000d3e4f506ULL,
    };
    const uintptr_t target_offset = 0x80U;

    for (unsigned round = 0; round < 2U; ++round) {
        const bool reverse_order = round != 0U;
        const uintptr_t target_paddr = phys_page_base(X) + target_offset;
        const uintptr_t adjacent_paddr = target_paddr + sizeof(uint64_t);
        const uintptr_t alias_line_vaddr = hs_page_base(alias_page) + target_offset;
        const uintptr_t adjacent_alias_vaddr =
            alias_line_vaddr + sizeof(uint64_t);
        uint64_t expected_final_value = initial_values[round];
        uint64_t observed_alias = 0ULL;
        uint64_t observed_adjacent_alias = 0ULL;
        bool observed_alias_ok = false;
        bool observed_adjacent_alias_ok = false;

        goto_priv(PRIV_M);
        ai_micro_m_memblock_reset_env();
        ai_micro_m_memblock_reset_pbmt_env();

        ai_micro_m_memblock_prepare_pbmt_translation();
        TEST_ASSERT("PBMT=NC StoreQueue-head setup seeds the target line through the same alias that the narrow stores use, avoiding cacheable-to-uncache visibility assumptions",
            ai_micro_m_memblock_seed_pbmt_alias_doubleword(
                alias_page, target_paddr, initial_values[round])
        );
        TEST_ASSERT("PBMT=NC StoreQueue-head setup seeds the adjacent guard doubleword through the same alias before checking that narrow stores leave it untouched",
            ai_micro_m_memblock_seed_pbmt_alias_doubleword(
                alias_page, adjacent_paddr, adjacent_values[round])
        );
        (void)ai_micro_m_memblock_configure_pbmt_alias(alias_page,
            target_paddr);

        for (unsigned step = 0; step < 3U; ++step) {
            const unsigned index = reverse_order ? (2U - step) : step;
            expected_final_value = ai_micro_m_memblock_merge_unsigned(
                expected_final_value, store_values[index], store_offsets[index],
                store_sizes[index]);

            TEST_SETUP_EXCEPT();
            ai_micro_m_memblock_translated_store_by_size(
                alias_line_vaddr + store_offsets[index],
                store_sizes[index],
                store_values[index]);
            TEST_ASSERT("each no-H PBMT=NC StoreQueue-head narrow store in the width-switch sequence must retire without surfacing an architectural exception",
                excpt.triggered == false
            );
        }

        TEST_SETUP_EXCEPT();
        observed_alias = ai_micro_m_memblock_translated_ld(alias_line_vaddr);
        observed_alias_ok = !excpt.triggered;
        TEST_ASSERT("after the no-H PBMT=NC StoreQueue-head narrow width-switch sequence, line readback through the same alias must show the exact final byte-age merge",
            observed_alias_ok &&
            observed_alias == expected_final_value
        );

        TEST_SETUP_EXCEPT();
        observed_adjacent_alias =
            ai_micro_m_memblock_translated_ld(adjacent_alias_vaddr);
        observed_adjacent_alias_ok = !excpt.triggered;
        TEST_ASSERT("the no-H PBMT=NC StoreQueue-head narrow width-switch sequence updates only the intended byte lanes and leaves the adjacent doubleword untouched when checked through the same NC alias",
            observed_alias_ok &&
            observed_adjacent_alias_ok &&
            observed_alias == expected_final_value &&
            observed_adjacent_alias == adjacent_values[round]
        );
    }

    ai_micro_m_memblock_reset_pbmt_env();
    ai_micro_m_memblock_reset_env();

    TEST_END();
}

bool ai_micro_m_store_pbmt_io_bus_error_then_same_alias_good_pbmt_io_store_clears_stale_exception_corner()
{
    TEST_START();

    const enum test_page alias_page = AI_MICRO_MEMBLOCK_PBMT_IO_ALIAS_PAGE;
    const uintptr_t bad_vaddr =
        hs_page_base(alias_page) +
        (AI_MICRO_MEMBLOCK_PBMT_BAD_PADDR & (PAGE_SIZE - 1U));
    const uintptr_t good_paddr =
        ai_micro_m_memblock_good_mmio64_paddr(0x240U);
    const uint64_t initial_good = 0x2030405060708090ULL;
    const uint64_t first_good = 0x7766554433221100ULL;
    const uint64_t second_good = 0x1f2e3d4c5b6a7988ULL;
    uintptr_t good_vaddr = 0U;

    goto_priv(PRIV_M);
    ai_micro_m_memblock_reset_env();
    ai_micro_m_memblock_reset_pbmt_env();
    write64(good_paddr, initial_good);

    ai_micro_m_memblock_prepare_pbmt_translation();
    (void)ai_micro_m_memblock_configure_pbmt_alias(alias_page,
        AI_MICRO_MEMBLOCK_PBMT_BAD_PADDR);

    TEST_SETUP_EXCEPT();
    ai_micro_m_memblock_translated_sd(bad_vaddr, 0xa55aa55aa55aa55aULL);
    TEST_ASSERT("the first no-H PBMT=IO store on the bad alias should report a precise store access fault before the same-alias stale-exception clear probe",
        excpt.triggered == true &&
        excpt.cause == CAUSE_SAF &&
        excpt.tval == bad_vaddr
    );

    good_vaddr = ai_micro_m_memblock_configure_pbmt_alias(alias_page, good_paddr);

    TEST_SETUP_EXCEPT();
    ai_micro_m_memblock_translated_sd(good_vaddr, first_good);
    TEST_ASSERT("after a no-H PBMT=IO bad-alias store fault, retargeting the same alias to a good page should let the next good store complete without inheriting stale exception state",
        excpt.triggered == false
    );

    TEST_SETUP_EXCEPT();
    ai_micro_m_memblock_translated_sd(good_vaddr, second_good);
    TEST_ASSERT("after the first recovered good no-H PBMT=IO store already succeeds on the retargeted alias, a second good store on the same alias must also complete without a false repeated fault",
        excpt.triggered == false
    );

    TEST_SETUP_EXCEPT();
    uint64_t observed = ai_micro_m_memblock_translated_ld(good_vaddr);
    TEST_ASSERT("after the no-H PBMT=IO bad-then-good-then-good store sequence on the retargeted alias, the target doubleword keeps only the youngest good payload",
        excpt.triggered == false &&
        observed == second_good
    );

    goto_priv(PRIV_M);
    TEST_ASSERT("after the no-H PBMT=IO stale-exception clear probe, backing memory matches only the final good payload",
        read64(good_paddr) == second_good
    );

    ai_micro_m_memblock_reset_pbmt_env();
    ai_micro_m_memblock_reset_env();

    TEST_END();
}

bool ai_micro_m_load_pbmt_narrow_bus_error_then_same_width_signedness_switch_recovery_corner()
{
    TEST_START();

    static const uintptr_t scratch_offsets[] = {
        0x180U,
        0x1c0U,
        0x200U,
    };
    static const unsigned size_bytes[] = {
        sizeof(uint8_t),
        sizeof(uint16_t),
        sizeof(uint32_t),
    };
    static const unsigned load_offsets[] = {
        7U,
        6U,
        4U,
    };
    static const uint64_t initial_goods[] = {
        0x8155aa1122334455ULL,
        0x92345566778899aaULL,
        0x89abcdef10203040ULL,
    };
    static const uint64_t updated_goods[] = {
        0xfe66778899aabbccULL,
        0x8abc01234567def0ULL,
        0xf1234567a0b0c0d0ULL,
    };

    /*
     * Keep this matrix on PBMT=NC.  PBMT=IO refresh/stale-exception coverage is
     * handled by the dedicated IO cases below; mixing it into the narrow decode
     * matrix pushes Spike into unsupported PMA CSR paths and can leave RTL
     * regressions waiting on IO/device response rather than testing decode
     * refresh.
     */
    for (unsigned kind = 0; kind < 1U; ++kind) {
        const enum test_page alias_page = (kind == 0U) ?
            AI_MICRO_MEMBLOCK_PBMT_NC_ALIAS_PAGE :
            AI_MICRO_MEMBLOCK_PBMT_IO_ALIAS_PAGE;
        const uintptr_t bad_paddr = (kind == 0U) ?
            AI_MICRO_MEMBLOCK_PBMT_BAD_PADDR_ALT :
            AI_MICRO_MEMBLOCK_PBMT_BAD_PADDR;

        for (unsigned round = 0; round < 3U; ++round) {
            const unsigned width = size_bytes[round];
            const unsigned offset = load_offsets[round];
            const bool fault_signed = (round & 1U) == 0U;
            const bool recovery_signed = !fault_signed;
            const uintptr_t good_paddr =
                ai_micro_m_memblock_good_paddr_for_alias(
                    alias_page, scratch_offsets[round]);
            const uintptr_t bad_vaddr = hs_page_base(alias_page) +
                ((bad_paddr + offset) & (PAGE_SIZE - 1U));
            const uint64_t expected_recovery =
                ai_micro_m_memblock_extract_scalar(updated_goods[round], offset,
                    width, recovery_signed);
            uintptr_t good_vaddr = 0U;

            goto_priv(PRIV_M);
            ai_micro_m_memblock_reset_env();
            ai_micro_m_memblock_reset_pbmt_env();

            ai_micro_m_memblock_prepare_pbmt_translation();
            TEST_ASSERT("PBMT setup store seeds the later good-load backing doubleword through the same uncache route instead of relying on cacheable-to-uncache visibility",
                ai_micro_m_memblock_seed_pbmt_alias_doubleword(
                    alias_page, good_paddr, updated_goods[round])
            );
            (void)ai_micro_m_memblock_configure_pbmt_alias(alias_page, bad_paddr);

            ai_micro_m_memblock_enable_mprv_supervisor_translation();
            TEST_SETUP_EXCEPT();
            (void)ai_micro_m_memblock_scalar_load_current_decode(
                fault_signed, width, bad_vaddr);
            ai_micro_m_memblock_disable_mprv_supervisor_translation();
            TEST_ASSERT("the first no-H PBMT narrow load on the bad alias should report a precise load access fault before the same-width signedness-switch recovery probe",
                excpt.triggered == true &&
                excpt.cause == CAUSE_LAF &&
                excpt.tval == bad_vaddr
            );

            good_vaddr = ai_micro_m_memblock_configure_pbmt_alias(alias_page, good_paddr);

            TEST_SETUP_EXCEPT();
            uint64_t observed =
                ai_micro_m_memblock_translated_scalar_load(recovery_signed,
                    width, good_vaddr + offset);
            TEST_ASSERT("after the no-H PBMT narrow load fault episode, retargeting the same alias to a good page must let the next same-width opposite-signedness load return the value selected by its current decode",
                excpt.triggered == false &&
                observed == expected_recovery
            );

            goto_priv(PRIV_M);
            TEST_ASSERT("the no-H PBMT narrow load signedness-switch recovery probe leaves the static recovery doubleword unchanged across the bad-alias episode",
                read64(good_paddr) == updated_goods[round]
            );
        }
    }

    ai_micro_m_memblock_reset_pbmt_env();
    ai_micro_m_memblock_reset_env();

    TEST_END();
}

bool ai_micro_m_load_pbmt_narrow_bad_good_next_bad_fault_refreshes_to_new_alias_corner()
{
    TEST_START();

    static const uintptr_t scratch_offsets[] = {
        0x180U,
        0x1c0U,
        0x200U,
    };
    static const unsigned size_bytes[] = {
        sizeof(uint8_t),
        sizeof(uint16_t),
        sizeof(uint32_t),
    };
    static const unsigned load_offsets[] = {
        7U,
        6U,
        4U,
    };
    static const uint64_t middle_goods[] = {
        0xfe66778899aabbccULL,
        0x8abc01234567def0ULL,
        0xf1234567a0b0c0d0ULL,
    };

    /*
     * Keep this matrix on PBMT=NC.  PBMT=IO refresh/stale-exception coverage is
     * handled by the dedicated IO cases below; mixing it into the narrow decode
     * matrix pushes Spike into unsupported PMA CSR paths and can leave RTL
     * regressions waiting on IO/device response rather than testing decode
     * refresh.
     */
    for (unsigned kind = 0; kind < 1U; ++kind) {
        const enum test_page alias_page = (kind == 0U) ?
            AI_MICRO_MEMBLOCK_PBMT_NC_ALIAS_PAGE :
            AI_MICRO_MEMBLOCK_PBMT_IO_ALIAS_PAGE;
        const uintptr_t first_bad_paddr = (kind == 0U) ?
            AI_MICRO_MEMBLOCK_PBMT_BAD_PADDR_ALT :
            AI_MICRO_MEMBLOCK_PBMT_BAD_PADDR;
        const uintptr_t second_bad_paddr = (kind == 0U) ?
            AI_MICRO_MEMBLOCK_PBMT_BAD_PADDR :
            AI_MICRO_MEMBLOCK_PBMT_BAD_PADDR_ALT;

        for (unsigned round = 0; round < 3U; ++round) {
            const unsigned width = size_bytes[round];
            const unsigned offset = load_offsets[round];
            const bool first_signed = (round & 1U) == 0U;
            const bool middle_signed = !first_signed;
            const bool final_signed = first_signed;
            const uintptr_t good_paddr =
                ai_micro_m_memblock_good_paddr_for_alias(
                    alias_page, scratch_offsets[round]);
            const uintptr_t first_bad_vaddr = hs_page_base(alias_page) +
                ((first_bad_paddr + offset) & (PAGE_SIZE - 1U));
            const uintptr_t second_bad_vaddr = hs_page_base(alias_page) +
                ((second_bad_paddr + offset) & (PAGE_SIZE - 1U));
            const uint64_t expected_middle =
                ai_micro_m_memblock_extract_scalar(middle_goods[round], offset,
                    width, middle_signed);
            uintptr_t good_vaddr = 0U;

            goto_priv(PRIV_M);
            ai_micro_m_memblock_reset_env();
            ai_micro_m_memblock_reset_pbmt_env();

            ai_micro_m_memblock_prepare_pbmt_translation();
            TEST_ASSERT("PBMT setup store seeds the later good-load backing doubleword through the same uncache route instead of relying on cacheable-to-uncache visibility",
                ai_micro_m_memblock_seed_pbmt_alias_doubleword(
                    alias_page, good_paddr, middle_goods[round])
            );
            (void)ai_micro_m_memblock_configure_pbmt_alias(alias_page,
                first_bad_paddr);

            ai_micro_m_memblock_enable_mprv_supervisor_translation();
            TEST_SETUP_EXCEPT();
            (void)ai_micro_m_memblock_scalar_load_current_decode(
                first_signed, width, first_bad_vaddr);
            ai_micro_m_memblock_disable_mprv_supervisor_translation();
            TEST_ASSERT("the first no-H PBMT narrow load in the bad-good-next-bad refresh probe should report a precise load access fault at the first bad alias",
                excpt.triggered == true &&
                excpt.cause == CAUSE_LAF &&
                excpt.tval == first_bad_vaddr
            );

            good_vaddr = ai_micro_m_memblock_configure_pbmt_alias(alias_page,
                good_paddr);

            TEST_SETUP_EXCEPT();
            uint64_t observed_middle =
                ai_micro_m_memblock_translated_scalar_load(middle_signed,
                    width, good_vaddr + offset);
            TEST_ASSERT("after the first no-H PBMT narrow bad-alias load fault, retargeting the same alias to a good page should let the middle good load complete with its current width and signedness decode",
                excpt.triggered == false &&
                observed_middle == expected_middle
            );

            (void)ai_micro_m_memblock_configure_pbmt_alias(alias_page,
                second_bad_paddr);

            ai_micro_m_memblock_enable_mprv_supervisor_translation();
            TEST_SETUP_EXCEPT();
            (void)ai_micro_m_memblock_scalar_load_current_decode(
                final_signed, width, second_bad_vaddr);
            ai_micro_m_memblock_disable_mprv_supervisor_translation();
            TEST_ASSERT("after the middle good no-H PBMT narrow load has already completed on the retargeted alias, the next bad-alias load must refresh tval to the newer bad alias instead of reusing stale exception metadata from the first bad episode",
                excpt.triggered == true &&
                excpt.cause == CAUSE_LAF &&
                excpt.tval == second_bad_vaddr
            );

            goto_priv(PRIV_M);
            TEST_ASSERT("the no-H PBMT narrow bad-good-next-bad refresh probe leaves the good backing doubleword unchanged across both bad-alias episodes",
                read64(good_paddr) == middle_goods[round]
            );
        }
    }

    ai_micro_m_memblock_reset_pbmt_env();
    ai_micro_m_memblock_reset_env();

    TEST_END();
}

bool ai_micro_m_load_pbmt_narrow_bad_good_next_bad_width_switch_refreshes_new_alias_corner()
{
    TEST_START();

    static const unsigned first_sizes[] = {
        sizeof(uint8_t),
        sizeof(uint16_t),
        sizeof(uint32_t),
    };
    static const unsigned first_offsets[] = { 7U, 6U, 4U };
    static const bool first_signed[] = { true, false, true };
    static const unsigned middle_sizes[] = {
        sizeof(uint16_t),
        sizeof(uint32_t),
        sizeof(uint8_t),
    };
    static const unsigned middle_offsets[] = { 6U, 4U, 7U };
    static const bool middle_signed[] = { false, true, false };
    static const uintptr_t scratch_offsets[] = { 0x180U, 0x1c0U, 0x200U };
    static const uint64_t good_values[] = {
        0x8abc01234567def0ULL,
        0xf1234567a0b0c0d0ULL,
        0xfe66778899aabbccULL,
    };

    /*
     * Keep this matrix on PBMT=NC.  PBMT=IO refresh/stale-exception coverage is
     * handled by the dedicated IO cases below; mixing it into the narrow decode
     * matrix pushes Spike into unsupported PMA CSR paths and can leave RTL
     * regressions waiting on IO/device response rather than testing decode
     * refresh.
     */
    for (unsigned kind = 0; kind < 1U; ++kind) {
        const enum test_page alias_page = (kind == 0U) ?
            AI_MICRO_MEMBLOCK_PBMT_NC_ALIAS_PAGE :
            AI_MICRO_MEMBLOCK_PBMT_IO_ALIAS_PAGE;
        const uintptr_t first_bad_paddr = (kind == 0U) ?
            AI_MICRO_MEMBLOCK_PBMT_BAD_PADDR_ALT :
            AI_MICRO_MEMBLOCK_PBMT_BAD_PADDR;
        const uintptr_t second_bad_paddr = (kind == 0U) ?
            AI_MICRO_MEMBLOCK_PBMT_BAD_PADDR :
            AI_MICRO_MEMBLOCK_PBMT_BAD_PADDR_ALT;

        for (unsigned round = 0; round < 3U; ++round) {
            const uintptr_t good_paddr =
                ai_micro_m_memblock_good_paddr_for_alias(
                    alias_page, scratch_offsets[round]);
            const uintptr_t first_bad_vaddr = hs_page_base(alias_page) +
                ((first_bad_paddr + first_offsets[round]) & (PAGE_SIZE - 1U));
            const uintptr_t second_bad_vaddr = hs_page_base(alias_page) +
                ((second_bad_paddr + middle_offsets[round]) & (PAGE_SIZE - 1U));
            const uint64_t expected_middle =
                ai_micro_m_memblock_extract_scalar(good_values[round],
                    middle_offsets[round], middle_sizes[round],
                    middle_signed[round]);
            uintptr_t good_vaddr = 0U;

            goto_priv(PRIV_M);
            ai_micro_m_memblock_reset_env();
            ai_micro_m_memblock_reset_pbmt_env();

            ai_micro_m_memblock_prepare_pbmt_translation();
            TEST_ASSERT("PBMT setup store seeds the later good-load backing doubleword through the same uncache route instead of relying on cacheable-to-uncache visibility",
                ai_micro_m_memblock_seed_pbmt_alias_doubleword(
                    alias_page, good_paddr, good_values[round])
            );
            (void)ai_micro_m_memblock_configure_pbmt_alias(alias_page,
                first_bad_paddr);

            ai_micro_m_memblock_enable_mprv_supervisor_translation();
            TEST_SETUP_EXCEPT();
            (void)ai_micro_m_memblock_scalar_load_current_decode(
                first_signed[round], first_sizes[round], first_bad_vaddr);
            ai_micro_m_memblock_disable_mprv_supervisor_translation();
            TEST_ASSERT("the first no-H PBMT narrow load in the width-switch bad-good-next-bad refresh probe should report a precise load access fault at the first bad alias",
                excpt.triggered == true &&
                excpt.cause == CAUSE_LAF &&
                excpt.tval == first_bad_vaddr
            );

            good_vaddr = ai_micro_m_memblock_configure_pbmt_alias(alias_page,
                good_paddr);

            TEST_SETUP_EXCEPT();
            uint64_t observed_middle =
                ai_micro_m_memblock_translated_scalar_load(
                    middle_signed[round], middle_sizes[round],
                    good_vaddr + middle_offsets[round]);
            TEST_ASSERT("after the first no-H PBMT narrow bad-alias fault, retargeting the same alias to a good page and switching width/decode should let the middle good load return the value selected by its current decode",
                excpt.triggered == false &&
                observed_middle == expected_middle
            );

            (void)ai_micro_m_memblock_configure_pbmt_alias(alias_page,
                second_bad_paddr);

            ai_micro_m_memblock_enable_mprv_supervisor_translation();
            TEST_SETUP_EXCEPT();
            (void)ai_micro_m_memblock_scalar_load_current_decode(
                middle_signed[round], middle_sizes[round], second_bad_vaddr);
            ai_micro_m_memblock_disable_mprv_supervisor_translation();
            TEST_ASSERT("after the middle good no-H PBMT narrow load has already completed with the switched width/decode, the next bad-alias load must refresh tval to the newer bad alias instead of reusing stale metadata from the first bad episode",
                excpt.triggered == true &&
                excpt.cause == CAUSE_LAF &&
                excpt.tval == second_bad_vaddr
            );

            goto_priv(PRIV_M);
            TEST_ASSERT("the no-H PBMT narrow width-switch bad-good-next-bad refresh probe leaves the good backing doubleword unchanged across both bad-alias episodes",
                read64(good_paddr) == good_values[round]
            );
        }
    }

    ai_micro_m_memblock_reset_pbmt_env();
    ai_micro_m_memblock_reset_env();

    TEST_END();
}

bool ai_micro_m_load_pbmt_bad_good_store_next_bad_fault_refreshes_to_new_alias_corner()
{
    TEST_START();

    static const uintptr_t target_offsets[] = {
        0x180U,
        0x1c0U,
        0x200U,
    };
    static const unsigned size_bytes[] = {
        sizeof(uint8_t),
        sizeof(uint16_t),
        sizeof(uint32_t),
    };
    static const unsigned load_offsets[] = {
        7U,
        6U,
        4U,
    };
    static const uint64_t middle_store_values[] = {
        0x1122334455667788ULL,
        0x89abcdef01234567ULL,
        0xf0e1d2c3b4a59687ULL,
    };

    /*
     * Keep this matrix on PBMT=NC.  PBMT=IO refresh/stale-exception coverage is
     * handled by the dedicated IO cases below; mixing it into the narrow decode
     * matrix pushes Spike into unsupported PMA CSR paths and can leave RTL
     * regressions waiting on IO/device response rather than testing decode
     * refresh.
     */
    for (unsigned kind = 0; kind < 1U; ++kind) {
        const enum test_page alias_page = (kind == 0U) ?
            AI_MICRO_MEMBLOCK_PBMT_NC_ALIAS_PAGE :
            AI_MICRO_MEMBLOCK_PBMT_IO_ALIAS_PAGE;
        const uintptr_t first_bad_paddr = (kind == 0U) ?
            AI_MICRO_MEMBLOCK_PBMT_BAD_PADDR_ALT :
            AI_MICRO_MEMBLOCK_PBMT_BAD_PADDR;
        const uintptr_t second_bad_paddr = (kind == 0U) ?
            AI_MICRO_MEMBLOCK_PBMT_BAD_PADDR :
            AI_MICRO_MEMBLOCK_PBMT_BAD_PADDR_ALT;

        for (unsigned round = 0; round < 3U; ++round) {
            const unsigned width = size_bytes[round];
            const unsigned offset = load_offsets[round];
            const bool fault_signed = (round & 1U) == 0U;
            const uintptr_t good_paddr =
                ai_micro_m_memblock_good_paddr_for_alias(
                    alias_page, target_offsets[round]);
            const uintptr_t first_bad_vaddr = hs_page_base(alias_page) +
                ((first_bad_paddr + offset) & (PAGE_SIZE - 1U));
            const uintptr_t second_bad_vaddr = hs_page_base(alias_page) +
                ((second_bad_paddr + offset) & (PAGE_SIZE - 1U));
            const uint64_t middle_store = middle_store_values[round];
            uintptr_t good_vaddr = 0U;

            goto_priv(PRIV_M);
            ai_micro_m_memblock_reset_env();
            ai_micro_m_memblock_reset_pbmt_env();
            write64(good_paddr, 0ULL);

            ai_micro_m_memblock_prepare_pbmt_translation();
            (void)ai_micro_m_memblock_configure_pbmt_alias(alias_page,
                first_bad_paddr);

            ai_micro_m_memblock_enable_mprv_supervisor_translation();
            TEST_SETUP_EXCEPT();
            (void)ai_micro_m_memblock_scalar_load_current_decode(
                fault_signed, width, first_bad_vaddr);
            ai_micro_m_memblock_disable_mprv_supervisor_translation();
            TEST_ASSERT("the first no-H PBMT bad-alias load in the load-good-store-next-bad refresh probe should report a precise load access fault at the first bad alias",
                excpt.triggered == true &&
                excpt.cause == CAUSE_LAF &&
                excpt.tval == first_bad_vaddr
            );

            good_vaddr = ai_micro_m_memblock_configure_pbmt_alias(alias_page,
                good_paddr);

            TEST_SETUP_EXCEPT();
            ai_micro_m_memblock_translated_sd(good_vaddr, middle_store);
            TEST_ASSERT("after the first no-H PBMT bad-alias load fault, retargeting the same alias to a good page should let the middle good store complete without inheriting stale exception state",
                excpt.triggered == false
            );

            (void)ai_micro_m_memblock_configure_pbmt_alias(alias_page,
                second_bad_paddr);

            ai_micro_m_memblock_enable_mprv_supervisor_translation();
            TEST_SETUP_EXCEPT();
            (void)ai_micro_m_memblock_scalar_load_current_decode(
                fault_signed, width, second_bad_vaddr);
            ai_micro_m_memblock_disable_mprv_supervisor_translation();
            TEST_ASSERT("after the middle good no-H PBMT store has already completed on the retargeted alias, the next bad-alias load must refresh cause+tval to the newer load-side bad alias instead of reusing stale metadata from the first bad load episode",
                excpt.triggered == true &&
                excpt.cause == CAUSE_LAF &&
                excpt.tval == second_bad_vaddr
            );

            good_vaddr = ai_micro_m_memblock_configure_pbmt_alias(alias_page,
                good_paddr);

            TEST_SETUP_EXCEPT();
            uint64_t observed_good =
                ai_micro_m_memblock_translated_scalar_load(false,
                    sizeof(uint64_t), good_vaddr);
            TEST_ASSERT("the no-H PBMT load-good-store-next-bad-load refresh probe reads back the successful middle-store payload through the retargeted PBMT alias",
                excpt.triggered == false &&
                observed_good == middle_store
            );
        }
    }

    ai_micro_m_memblock_reset_pbmt_env();
    ai_micro_m_memblock_reset_env();

    TEST_END();
}

bool ai_micro_m_store_pbmt_bad_good_load_next_bad_fault_refreshes_to_new_alias_corner()
{
    TEST_START();

    static const uintptr_t scratch_offsets[] = {
        0x180U,
        0x1c0U,
        0x200U,
    };
    static const unsigned size_bytes[] = {
        sizeof(uint8_t),
        sizeof(uint16_t),
        sizeof(uint32_t),
    };
    static const unsigned load_offsets[] = {
        7U,
        6U,
        4U,
    };
    static const uint64_t middle_goods[] = {
        0xfe66778899aabbccULL,
        0x8abc01234567def0ULL,
        0xf1234567a0b0c0d0ULL,
    };

    /*
     * Keep this matrix on PBMT=NC.  PBMT=IO refresh/stale-exception coverage is
     * handled by the dedicated IO cases below; mixing it into the narrow decode
     * matrix pushes Spike into unsupported PMA CSR paths and can leave RTL
     * regressions waiting on IO/device response rather than testing decode
     * refresh.
     */
    for (unsigned kind = 0; kind < 1U; ++kind) {
        const enum test_page alias_page = (kind == 0U) ?
            AI_MICRO_MEMBLOCK_PBMT_NC_ALIAS_PAGE :
            AI_MICRO_MEMBLOCK_PBMT_IO_ALIAS_PAGE;
        const uintptr_t first_bad_paddr = (kind == 0U) ?
            AI_MICRO_MEMBLOCK_PBMT_BAD_PADDR_ALT :
            AI_MICRO_MEMBLOCK_PBMT_BAD_PADDR;
        const uintptr_t second_bad_paddr = (kind == 0U) ?
            AI_MICRO_MEMBLOCK_PBMT_BAD_PADDR :
            AI_MICRO_MEMBLOCK_PBMT_BAD_PADDR_ALT;

        for (unsigned round = 0; round < 3U; ++round) {
            const unsigned width = size_bytes[round];
            const unsigned offset = load_offsets[round];
            const bool middle_signed = (round & 1U) != 0U;
            const uintptr_t good_paddr =
                ai_micro_m_memblock_good_paddr_for_alias(
                    alias_page, scratch_offsets[round]);
            const uintptr_t first_bad_vaddr = hs_page_base(alias_page) +
                ((first_bad_paddr + offset) & (PAGE_SIZE - 1U));
            const uintptr_t second_bad_vaddr = hs_page_base(alias_page) +
                ((second_bad_paddr + offset) & (PAGE_SIZE - 1U));
            const uint64_t middle_good = middle_goods[round];
            const uint64_t expected_middle =
                ai_micro_m_memblock_extract_scalar(middle_good, offset, width,
                    middle_signed);
            uintptr_t good_vaddr = 0U;

            goto_priv(PRIV_M);
            ai_micro_m_memblock_reset_env();
            ai_micro_m_memblock_reset_pbmt_env();

            ai_micro_m_memblock_prepare_pbmt_translation();
            TEST_ASSERT("PBMT setup store seeds the later good-load backing doubleword through the same uncache route instead of relying on cacheable-to-uncache visibility",
                ai_micro_m_memblock_seed_pbmt_alias_doubleword(
                    alias_page, good_paddr, middle_good)
            );
            (void)ai_micro_m_memblock_configure_pbmt_alias(alias_page,
                first_bad_paddr);

            TEST_SETUP_EXCEPT();
            ai_micro_m_memblock_translated_store_by_size(first_bad_vaddr,
                width, 0xa55aa55aa55aa55aULL);
            TEST_ASSERT("the first no-H PBMT bad-alias store in the store-good-load-next-bad refresh probe should report a precise store access fault at the first bad alias",
                excpt.triggered == true &&
                excpt.cause == CAUSE_SAF &&
                excpt.tval == first_bad_vaddr
            );

            good_vaddr = ai_micro_m_memblock_configure_pbmt_alias(alias_page,
                good_paddr);

            TEST_SETUP_EXCEPT();
            uint64_t observed_middle =
                ai_micro_m_memblock_translated_scalar_load(middle_signed,
                    width, good_vaddr + offset);
            TEST_ASSERT("after the first no-H PBMT bad-alias store fault, retargeting the same alias to a good page should let the middle good load complete with its current width and signedness decode",
                excpt.triggered == false &&
                observed_middle == expected_middle
            );

            (void)ai_micro_m_memblock_configure_pbmt_alias(alias_page,
                second_bad_paddr);

            TEST_SETUP_EXCEPT();
            ai_micro_m_memblock_translated_store_by_size(second_bad_vaddr,
                width, 0x5aa55aa55aa55aa5ULL);
            TEST_ASSERT("after the middle good no-H PBMT load has already completed on the retargeted alias, the next bad-alias store must refresh cause+tval to the newer store-side bad alias instead of reusing stale metadata from the first bad store episode",
                excpt.triggered == true &&
                excpt.cause == CAUSE_SAF &&
                excpt.tval == second_bad_vaddr
            );

            goto_priv(PRIV_M);
            TEST_ASSERT("the no-H PBMT store-good-load-next-bad-store refresh probe leaves the good backing doubleword unchanged across both bad-alias episodes",
                read64(good_paddr) == middle_good
            );
        }
    }

    ai_micro_m_memblock_reset_pbmt_env();
    ai_micro_m_memblock_reset_env();

    TEST_END();
}

bool ai_micro_m_store_pbmt_io_bad_good_next_bad_fault_refreshes_to_new_alias_corner()
{
    TEST_START();

    const enum test_page alias_page = AI_MICRO_MEMBLOCK_PBMT_IO_ALIAS_PAGE;
    const uintptr_t first_bad_paddr = AI_MICRO_MEMBLOCK_PBMT_BAD_PADDR;
    const uintptr_t second_bad_paddr = AI_MICRO_MEMBLOCK_PBMT_BAD_PADDR_ALT;
    const uintptr_t first_bad_vaddr =
        hs_page_base(alias_page) + (first_bad_paddr & (PAGE_SIZE - 1U));
    const uintptr_t second_bad_vaddr =
        hs_page_base(alias_page) + (second_bad_paddr & (PAGE_SIZE - 1U));
    const uintptr_t good_paddr =
        ai_micro_m_memblock_good_mmio64_paddr(0x2c0U);
    const uint64_t initial_good = 0x1020304050607080ULL;
    const uint64_t good_store = 0x7766554433221100ULL;
    uintptr_t good_vaddr = 0U;

    goto_priv(PRIV_M);
    ai_micro_m_memblock_reset_env();
    ai_micro_m_memblock_reset_pbmt_env();
    write64(good_paddr, initial_good);

    ai_micro_m_memblock_prepare_pbmt_translation();
    (void)ai_micro_m_memblock_configure_pbmt_alias(alias_page, first_bad_paddr);

    TEST_SETUP_EXCEPT();
    ai_micro_m_memblock_translated_sd(first_bad_vaddr, 0xa55aa55aa55aa55aULL);
    TEST_ASSERT("the first no-H PBMT=IO bad-alias store in the bad-good-next-bad refresh probe should report a precise store access fault at the first bad alias",
        excpt.triggered == true &&
        excpt.cause == CAUSE_SAF &&
        excpt.tval == first_bad_vaddr
    );

    good_vaddr = ai_micro_m_memblock_configure_pbmt_alias(alias_page, good_paddr);

    TEST_SETUP_EXCEPT();
    ai_micro_m_memblock_translated_sd(good_vaddr, good_store);
    TEST_ASSERT("after the first no-H PBMT=IO bad-alias store fault, retargeting the same alias to a good page should let the next good store complete without inheriting stale exception state",
        excpt.triggered == false
    );

    (void)ai_micro_m_memblock_configure_pbmt_alias(alias_page, second_bad_paddr);

    TEST_SETUP_EXCEPT();
    ai_micro_m_memblock_translated_sd(second_bad_vaddr, 0x5aa55aa55aa55aa5ULL);
    TEST_ASSERT("after the good no-H PBMT=IO store has already completed on the retargeted alias, the next bad-alias store must refresh tval to the newer bad alias instead of reusing stale exception metadata from the first bad episode",
        excpt.triggered == true &&
        excpt.cause == CAUSE_SAF &&
        excpt.tval == second_bad_vaddr
    );

    goto_priv(PRIV_M);
    TEST_ASSERT("the no-H PBMT=IO bad-good-next-bad store refresh probe leaves the good backing word with only the successful middle-store payload",
        read64(good_paddr) == good_store
    );

    ai_micro_m_memblock_reset_pbmt_env();
    ai_micro_m_memblock_reset_env();

    TEST_END();
}

bool ai_micro_m_store_pbmt_nc_bad_good_next_bad_fault_refreshes_to_new_alias_corner()
{
    TEST_START();

    const enum test_page alias_page = AI_MICRO_MEMBLOCK_PBMT_NC_ALIAS_PAGE;
    const uintptr_t first_bad_paddr = AI_MICRO_MEMBLOCK_PBMT_BAD_PADDR_ALT;
    const uintptr_t second_bad_paddr = AI_MICRO_MEMBLOCK_PBMT_BAD_PADDR;
    const uintptr_t first_bad_vaddr =
        hs_page_base(alias_page) + (first_bad_paddr & (PAGE_SIZE - 1U));
    const uintptr_t second_bad_vaddr =
        hs_page_base(alias_page) + (second_bad_paddr & (PAGE_SIZE - 1U));
    const uintptr_t good_paddr = phys_page_base(SWITCH2) + 0x440U;
    const uint64_t initial_good = 0x2030405060708090ULL;
    const uint64_t good_store = 0x7766554433221100ULL;
    uintptr_t good_vaddr = 0U;

    goto_priv(PRIV_M);
    ai_micro_m_memblock_reset_env();
    ai_micro_m_memblock_reset_pbmt_env();
    write64(good_paddr, initial_good);

    ai_micro_m_memblock_prepare_pbmt_translation();
    (void)ai_micro_m_memblock_configure_pbmt_alias(alias_page, first_bad_paddr);

    TEST_SETUP_EXCEPT();
    ai_micro_m_memblock_translated_sd(first_bad_vaddr, 0xa55aa55aa55aa55aULL);
    TEST_ASSERT("the first no-H PBMT=NC bad-alias store in the bad-good-next-bad refresh probe should report a precise store access fault at the first bad alias",
        excpt.triggered == true &&
        excpt.cause == CAUSE_SAF &&
        excpt.tval == first_bad_vaddr
    );

    good_vaddr = ai_micro_m_memblock_configure_pbmt_alias(alias_page, good_paddr);

    TEST_SETUP_EXCEPT();
    ai_micro_m_memblock_translated_sd(good_vaddr, good_store);
    TEST_ASSERT("after the first no-H PBMT=NC bad-alias store fault, retargeting the same alias to a good page should let the next good store complete without inheriting stale exception state",
        excpt.triggered == false
    );

    (void)ai_micro_m_memblock_configure_pbmt_alias(alias_page, second_bad_paddr);

    TEST_SETUP_EXCEPT();
    ai_micro_m_memblock_translated_sd(second_bad_vaddr, 0x5aa55aa55aa55aa5ULL);
    TEST_ASSERT("after the good no-H PBMT=NC store has already completed on the retargeted alias, the next bad-alias store must refresh tval to the newer bad alias instead of reusing stale exception metadata from the first bad episode",
        excpt.triggered == true &&
        excpt.cause == CAUSE_SAF &&
        excpt.tval == second_bad_vaddr
    );

    good_vaddr = ai_micro_m_memblock_configure_pbmt_alias(alias_page, good_paddr);

    TEST_SETUP_EXCEPT();
    uint64_t observed_good =
        ai_micro_m_memblock_translated_scalar_load(false, sizeof(uint64_t),
            good_vaddr);
    TEST_ASSERT("the no-H PBMT=NC bad-good-next-bad store refresh probe reads back only the successful middle-store payload through the retargeted PBMT alias",
        excpt.triggered == false &&
        observed_good == good_store
    );

    ai_micro_m_memblock_reset_pbmt_env();
    ai_micro_m_memblock_reset_env();

    TEST_END();
}

bool ai_micro_m_store_pbmt_io_bad_good_next_bad_width_switch_refreshes_new_alias_corner()
{
    TEST_START();

    static const unsigned first_sizes[] = {
        sizeof(uint8_t),
        sizeof(uint16_t),
        sizeof(uint32_t),
    };
    static const unsigned first_offsets[] = { 7U, 2U, 4U };
    static const unsigned middle_sizes[] = {
        sizeof(uint16_t),
        sizeof(uint32_t),
        sizeof(uint8_t),
    };
    static const unsigned middle_offsets[] = { 2U, 4U, 7U };
    static const uintptr_t good_offsets[] = { 0x380U, 0x3c0U, 0x400U };
    static const uint64_t initial_goods[] = {
        0x1817161514131211ULL,
        0x2827262524232221ULL,
        0x3837363534333231ULL,
    };
    static const uint64_t middle_store_values[] = {
        0x000000000000b4c3ULL,
        0x00000000d3e4f506ULL,
        0x00000000000000a5ULL,
    };

    const enum test_page alias_page = AI_MICRO_MEMBLOCK_PBMT_IO_ALIAS_PAGE;
    const uintptr_t first_bad_paddr = AI_MICRO_MEMBLOCK_PBMT_BAD_PADDR;
    const uintptr_t second_bad_paddr = AI_MICRO_MEMBLOCK_PBMT_BAD_PADDR_ALT;

    for (unsigned round = 0; round < 3U; ++round) {
        const uintptr_t good_paddr =
            ai_micro_m_memblock_good_paddr_for_alias(
                alias_page, good_offsets[round]);
        const uintptr_t first_bad_vaddr =
            hs_page_base(alias_page) +
            ((first_bad_paddr + first_offsets[round]) & (PAGE_SIZE - 1U));
        const uintptr_t second_bad_vaddr =
            hs_page_base(alias_page) +
            ((second_bad_paddr + middle_offsets[round]) & (PAGE_SIZE - 1U));
        const uint64_t expected_middle = ai_micro_m_memblock_merge_unsigned(
            initial_goods[round], middle_store_values[round],
            middle_offsets[round], middle_sizes[round]);
        uintptr_t good_vaddr = 0U;

        goto_priv(PRIV_M);
        ai_micro_m_memblock_reset_env();
        ai_micro_m_memblock_reset_pbmt_env();
        write64(good_paddr, initial_goods[round]);

        ai_micro_m_memblock_prepare_pbmt_translation();
        (void)ai_micro_m_memblock_configure_pbmt_alias(alias_page,
            first_bad_paddr);

        TEST_SETUP_EXCEPT();
        ai_micro_m_memblock_translated_store_by_size(first_bad_vaddr,
            first_sizes[round], middle_store_values[round]);
        TEST_ASSERT("the first no-H PBMT bad-alias store in the width-switch bad-good-next-bad refresh probe should report a precise store access fault at the first bad alias",
            excpt.triggered == true &&
            excpt.cause == CAUSE_SAF &&
            excpt.tval == first_bad_vaddr
        );

        good_vaddr = ai_micro_m_memblock_configure_pbmt_alias(alias_page,
            good_paddr);

        TEST_SETUP_EXCEPT();
        ai_micro_m_memblock_translated_store_by_size(good_vaddr +
            middle_offsets[round], middle_sizes[round], middle_store_values[round]);
        TEST_ASSERT("after the first no-H PBMT bad-alias store fault, retargeting the same alias to a good page and switching width should let the middle good store complete without inheriting stale exception state",
            excpt.triggered == false
        );

        (void)ai_micro_m_memblock_configure_pbmt_alias(alias_page,
            second_bad_paddr);

        TEST_SETUP_EXCEPT();
        ai_micro_m_memblock_translated_store_by_size(second_bad_vaddr,
            middle_sizes[round], middle_store_values[round]);
        TEST_ASSERT("after the middle good no-H PBMT store has already completed with the switched width, the next bad-alias store must refresh tval to the newer bad alias instead of reusing stale metadata from the first bad episode",
            excpt.triggered == true &&
            excpt.cause == CAUSE_SAF &&
            excpt.tval == second_bad_vaddr
        );

        goto_priv(PRIV_M);
        TEST_ASSERT("the no-H PBMT store width-switch bad-good-next-bad refresh probe leaves the good backing doubleword with only the successful middle-store payload",
            read64(good_paddr) == expected_middle
        );
    }

    ai_micro_m_memblock_reset_pbmt_env();
    ai_micro_m_memblock_reset_env();

    TEST_END();
}

bool ai_micro_m_load_pbmt_io_bad_good_next_bad_fault_refreshes_to_new_alias_corner()
{
    TEST_START();

    const enum test_page alias_page = AI_MICRO_MEMBLOCK_PBMT_IO_ALIAS_PAGE;
    const uintptr_t first_bad_paddr = AI_MICRO_MEMBLOCK_PBMT_BAD_PADDR;
    const uintptr_t second_bad_paddr = AI_MICRO_MEMBLOCK_PBMT_BAD_PADDR_ALT;
    const unsigned width = sizeof(uint32_t);
    const unsigned offset = 4U;
    const uintptr_t first_bad_vaddr = hs_page_base(alias_page) +
        ((first_bad_paddr + offset) & (PAGE_SIZE - 1U));
    const uintptr_t second_bad_vaddr = hs_page_base(alias_page) +
        ((second_bad_paddr + offset) & (PAGE_SIZE - 1U));
    const uintptr_t good_paddr =
        ai_micro_m_memblock_good_mmio32_paddr_for_offset(offset);
    const uint64_t good_value = 0x89abcdef10203040ULL;
    const uint64_t expected_good =
        ai_micro_m_memblock_extract_scalar(good_value, offset, width, false);
    uintptr_t good_vaddr = 0U;

    goto_priv(PRIV_M);
    ai_micro_m_memblock_reset_env();
    ai_micro_m_memblock_reset_pbmt_env();
    write32(good_paddr + offset, (uint32_t)expected_good);

    ai_micro_m_memblock_prepare_pbmt_translation();
    (void)ai_micro_m_memblock_configure_pbmt_alias(alias_page, first_bad_paddr);

    ai_micro_m_memblock_enable_mprv_supervisor_translation();
    TEST_SETUP_EXCEPT();
    (void)ai_micro_m_memblock_scalar_load_current_decode(
        false, width, first_bad_vaddr);
    ai_micro_m_memblock_disable_mprv_supervisor_translation();
    TEST_ASSERT("the first no-H PBMT=IO bad-alias load in the bad-good-next-bad refresh probe should report a precise load access fault at the first bad alias",
        excpt.triggered == true &&
        excpt.cause == CAUSE_LAF &&
        excpt.tval == first_bad_vaddr
    );

    good_vaddr = ai_micro_m_memblock_configure_pbmt_alias(alias_page, good_paddr);

    TEST_SETUP_EXCEPT();
    uint64_t observed_good =
        ai_micro_m_memblock_translated_scalar_load(false, width,
            good_vaddr + offset);
    TEST_ASSERT("after the first no-H PBMT=IO bad-alias load fault, retargeting the same alias to a good page should let the next good load complete without inheriting stale exception state",
        excpt.triggered == false &&
        observed_good == expected_good
    );

    (void)ai_micro_m_memblock_configure_pbmt_alias(alias_page, second_bad_paddr);

    ai_micro_m_memblock_enable_mprv_supervisor_translation();
    TEST_SETUP_EXCEPT();
    (void)ai_micro_m_memblock_scalar_load_current_decode(
        false, width, second_bad_vaddr);
    ai_micro_m_memblock_disable_mprv_supervisor_translation();
    TEST_ASSERT("after the good no-H PBMT=IO load has already completed on the retargeted alias, the next bad-alias load must refresh tval to the newer bad alias instead of reusing stale exception metadata from the first bad episode",
        excpt.triggered == true &&
        excpt.cause == CAUSE_LAF &&
        excpt.tval == second_bad_vaddr
    );

    goto_priv(PRIV_M);
    TEST_ASSERT("the no-H PBMT=IO bad-good-next-bad load refresh probe leaves the good MMIO32 backing register unchanged across both bad-alias episodes",
        read32(good_paddr + offset) == (uint32_t)expected_good
    );

    ai_micro_m_memblock_reset_pbmt_env();
    ai_micro_m_memblock_reset_env();

    TEST_END();
}

bool ai_micro_m_load_pbmt_io_bad_good_next_bad_store_fault_refreshes_to_new_alias_corner()
{
    TEST_START();

    const enum test_page alias_page = AI_MICRO_MEMBLOCK_PBMT_IO_ALIAS_PAGE;
    const uintptr_t first_bad_paddr = AI_MICRO_MEMBLOCK_PBMT_BAD_PADDR;
    const uintptr_t second_bad_paddr = AI_MICRO_MEMBLOCK_PBMT_BAD_PADDR_ALT;
    const uintptr_t first_bad_vaddr =
        hs_page_base(alias_page) + (first_bad_paddr & (PAGE_SIZE - 1U));
    const uintptr_t second_bad_vaddr =
        hs_page_base(alias_page) + (second_bad_paddr & (PAGE_SIZE - 1U));
    const uintptr_t good_paddr =
        ai_micro_m_memblock_good_mmio64_paddr(0x340U);
    const uint64_t initial_good = 0x1122334455667788ULL;
    const uint64_t good_store = 0xa1b2c3d4e5f60718ULL;
    uintptr_t good_vaddr = 0U;

    goto_priv(PRIV_M);
    ai_micro_m_memblock_reset_env();
    ai_micro_m_memblock_reset_pbmt_env();
    write64(good_paddr, initial_good);

    ai_micro_m_memblock_prepare_pbmt_translation();
    (void)ai_micro_m_memblock_configure_pbmt_alias(alias_page, first_bad_paddr);

    ai_micro_m_memblock_enable_mprv_supervisor_translation();
    TEST_SETUP_EXCEPT();
    (void)ai_micro_m_memblock_scalar_load_current_decode(
        false, sizeof(uint64_t), first_bad_vaddr);
    ai_micro_m_memblock_disable_mprv_supervisor_translation();
    TEST_ASSERT("the first no-H PBMT=IO bad-alias load in the cross-producer bad-good-next-bad refresh probe should report a precise load access fault at the first bad alias",
        excpt.triggered == true &&
        excpt.cause == CAUSE_LAF &&
        excpt.tval == first_bad_vaddr
    );

    good_vaddr = ai_micro_m_memblock_configure_pbmt_alias(alias_page, good_paddr);

    TEST_SETUP_EXCEPT();
    ai_micro_m_memblock_translated_sd(good_vaddr, good_store);
    TEST_ASSERT("after the first no-H PBMT=IO bad-alias load fault, retargeting the same alias to a good page should let the middle good store complete without inheriting stale exception state",
        excpt.triggered == false
    );

    (void)ai_micro_m_memblock_configure_pbmt_alias(alias_page, second_bad_paddr);

    TEST_SETUP_EXCEPT();
    ai_micro_m_memblock_translated_sd(second_bad_vaddr, 0x5aa55aa55aa55aa5ULL);
    TEST_ASSERT("after the middle good no-H PBMT=IO store has already completed on the retargeted alias, the next bad-alias store must refresh cause+tval to the newer store-side bad alias instead of reusing stale metadata from the first bad load episode",
        excpt.triggered == true &&
        excpt.cause == CAUSE_SAF &&
        excpt.tval == second_bad_vaddr
    );

    goto_priv(PRIV_M);
    TEST_ASSERT("the no-H PBMT=IO load-good-store-next-bad-store refresh probe leaves the good backing word with only the successful middle-store payload",
        read64(good_paddr) == good_store
    );

    ai_micro_m_memblock_reset_pbmt_env();
    ai_micro_m_memblock_reset_env();

    TEST_END();
}

bool ai_micro_m_store_pbmt_io_bad_good_next_bad_load_fault_refreshes_to_new_alias_corner()
{
    TEST_START();

    const enum test_page alias_page = AI_MICRO_MEMBLOCK_PBMT_IO_ALIAS_PAGE;
    const uintptr_t first_bad_paddr = AI_MICRO_MEMBLOCK_PBMT_BAD_PADDR;
    const uintptr_t second_bad_paddr = AI_MICRO_MEMBLOCK_PBMT_BAD_PADDR_ALT;
    const unsigned width = sizeof(uint64_t);
    const uintptr_t first_bad_vaddr =
        hs_page_base(alias_page) + (first_bad_paddr & (PAGE_SIZE - 1U));
    const uintptr_t second_bad_vaddr =
        hs_page_base(alias_page) + (second_bad_paddr & (PAGE_SIZE - 1U));
    const uintptr_t good_paddr =
        ai_micro_m_memblock_good_mmio64_paddr(0x380U);
    const uint64_t good_value = 0x0f1e2d3c4b5a6978ULL;
    uintptr_t good_vaddr = 0U;

    goto_priv(PRIV_M);
    ai_micro_m_memblock_reset_env();
    ai_micro_m_memblock_reset_pbmt_env();
    write64(good_paddr, good_value);

    ai_micro_m_memblock_prepare_pbmt_translation();
    (void)ai_micro_m_memblock_configure_pbmt_alias(alias_page, first_bad_paddr);

    TEST_SETUP_EXCEPT();
    ai_micro_m_memblock_translated_sd(first_bad_vaddr, 0xa55aa55aa55aa55aULL);
    TEST_ASSERT("the first no-H PBMT=IO bad-alias store in the cross-producer bad-good-next-bad refresh probe should report a precise store access fault at the first bad alias",
        excpt.triggered == true &&
        excpt.cause == CAUSE_SAF &&
        excpt.tval == first_bad_vaddr
    );

    good_vaddr = ai_micro_m_memblock_configure_pbmt_alias(alias_page, good_paddr);

    TEST_SETUP_EXCEPT();
    uint64_t observed_good =
        ai_micro_m_memblock_translated_scalar_load(false, width, good_vaddr);
    TEST_ASSERT("after the first no-H PBMT=IO bad-alias store fault, retargeting the same alias to a good page should let the middle good load complete without inheriting stale exception state",
        excpt.triggered == false &&
        observed_good == good_value
    );

    (void)ai_micro_m_memblock_configure_pbmt_alias(alias_page, second_bad_paddr);

    ai_micro_m_memblock_enable_mprv_supervisor_translation();
    TEST_SETUP_EXCEPT();
    (void)ai_micro_m_memblock_scalar_load_current_decode(
        false, width, second_bad_vaddr);
    ai_micro_m_memblock_disable_mprv_supervisor_translation();
    TEST_ASSERT("after the middle good no-H PBMT=IO load has already completed on the retargeted alias, the next bad-alias load must refresh cause+tval to the newer load-side bad alias instead of reusing stale metadata from the first bad store episode",
        excpt.triggered == true &&
        excpt.cause == CAUSE_LAF &&
        excpt.tval == second_bad_vaddr
    );

    goto_priv(PRIV_M);
    TEST_ASSERT("the no-H PBMT=IO store-good-load-next-bad-load refresh probe leaves the good backing word unchanged across both bad-alias episodes",
        read64(good_paddr) == good_value
    );

    ai_micro_m_memblock_reset_pbmt_env();
    ai_micro_m_memblock_reset_env();

    TEST_END();
}

bool ai_micro_m_pbmt_io_load_store_oldest_wins_diagnostic_corner()
{
    TEST_START();

    const enum test_page alias_page = AI_MICRO_MEMBLOCK_PBMT_IO_ALIAS_PAGE;
    const uintptr_t bad_paddr_load = AI_MICRO_MEMBLOCK_PBMT_BAD_PADDR;
    const uintptr_t bad_paddr_store = AI_MICRO_MEMBLOCK_PBMT_BAD_PADDR_ALT;
    const uintptr_t load_vaddr =
        hs_page_base(alias_page) + (bad_paddr_load & (PAGE_SIZE - 1U));
    const uintptr_t store_vaddr =
        hs_page_base(alias_page) + (bad_paddr_store & (PAGE_SIZE - 1U));
    const uintptr_t marker_paddr = phys_page_base(SCRATCHPAD);
    struct ai_micro_m_memblock_exception_snapshot first = {0};

    goto_priv(PRIV_M);
    ai_micro_m_memblock_reset_env();
    ai_micro_m_memblock_reset_pbmt_env();
    write64(marker_paddr, 0ULL);

    ai_micro_m_memblock_prepare_pbmt_translation();
    (void)ai_micro_m_memblock_configure_pbmt_alias(alias_page, bad_paddr_load);

    TEST_SETUP_EXCEPT();
    ai_micro_m_memblock_translated_load_store_fault_pair_capture(
        load_vaddr, store_vaddr, marker_paddr, 0x0102030405060708ULL, &first);

    TEST_ASSERT("in the no-H PBMT=IO load-plus-store diagnostic pair with older bad-alias load and younger bad-alias store, the first observed precise trap remains the older load access fault",
        first.triggered == true &&
        first.cause == CAUSE_LAF &&
        first.tval == load_vaddr
    );
    TEST_ASSERT("after the in-sequence rearm step, the younger bad-alias store in that no-H PBMT=IO diagnostic pair still reports the expected store access fault",
        excpt.triggered == true &&
        excpt.cause == CAUSE_SAF &&
        excpt.tval == store_vaddr
    );
    goto_priv(PRIV_M);
    TEST_ASSERT("the trailing marker store in the older-load-first no-H PBMT=IO diagnostic pair executes only after both precise traps have been observed",
        read64(marker_paddr) == 1ULL
    );

    first = (struct ai_micro_m_memblock_exception_snapshot){0};

    goto_priv(PRIV_M);
    ai_micro_m_memblock_reset_env();
    ai_micro_m_memblock_reset_pbmt_env();
    write64(marker_paddr, 0ULL);

    ai_micro_m_memblock_prepare_pbmt_translation();
    (void)ai_micro_m_memblock_configure_pbmt_alias(alias_page, bad_paddr_load);

    TEST_SETUP_EXCEPT();
    ai_micro_m_memblock_translated_store_load_fault_pair_capture(
        store_vaddr, load_vaddr, marker_paddr, 0x1111111122222222ULL, &first);

    TEST_ASSERT("in the reverse no-H PBMT=IO diagnostic pair with older bad-alias store and younger bad-alias load, the first observed precise trap remains the older store access fault",
        first.triggered == true &&
        first.cause == CAUSE_SAF &&
        first.tval == store_vaddr
    );
    TEST_ASSERT("after the in-sequence rearm step, the younger bad-alias load in the reverse no-H PBMT=IO diagnostic pair still reports the expected load access fault",
        excpt.triggered == true &&
        excpt.cause == CAUSE_LAF &&
        excpt.tval == load_vaddr
    );
    goto_priv(PRIV_M);
    TEST_ASSERT("the trailing marker store in the reverse no-H PBMT=IO diagnostic pair executes only after both precise traps have been observed",
        read64(marker_paddr) == 1ULL
    );

    ai_micro_m_memblock_reset_pbmt_env();
    ai_micro_m_memblock_reset_env();

    TEST_END();
}

bool ai_micro_m_pbmt_mixed_nc_load_io_store_oldest_wins_diagnostic_corner()
{
    TEST_START();

    const enum test_page load_alias_page = AI_MICRO_MEMBLOCK_PBMT_NC_ALIAS_PAGE;
    const enum test_page store_alias_page = AI_MICRO_MEMBLOCK_PBMT_IO_ALIAS_PAGE;
    const uintptr_t bad_paddr_load = AI_MICRO_MEMBLOCK_PBMT_BAD_PADDR_ALT;
    const uintptr_t bad_paddr_store = AI_MICRO_MEMBLOCK_PBMT_BAD_PADDR;
    const uintptr_t marker_paddr = phys_page_base(SCRATCHPAD);
    uintptr_t load_vaddr = 0U;
    uintptr_t store_vaddr = 0U;
    struct ai_micro_m_memblock_exception_snapshot first = {0};

    goto_priv(PRIV_M);
    ai_micro_m_memblock_reset_env();
    ai_micro_m_memblock_reset_pbmt_env();
    write64(marker_paddr, 0ULL);

    ai_micro_m_memblock_prepare_pbmt_translation();
    load_vaddr = ai_micro_m_memblock_configure_pbmt_alias(load_alias_page,
        bad_paddr_load);
    store_vaddr = ai_micro_m_memblock_configure_pbmt_alias(store_alias_page,
        bad_paddr_store);

    TEST_SETUP_EXCEPT();
    ai_micro_m_memblock_translated_load_store_fault_pair_capture(
        load_vaddr, store_vaddr, marker_paddr, 0x0102030405060708ULL, &first);

    TEST_ASSERT("in the no-H mixed PBMT diagnostic pair with older PBMT=NC bad-alias load and younger PBMT=IO bad-alias store, the first observed precise trap remains the older load access fault",
        first.triggered == true &&
        first.cause == CAUSE_LAF &&
        first.tval == load_vaddr
    );
    TEST_ASSERT("after the in-sequence rearm step, the younger PBMT=IO bad-alias store in that no-H mixed PBMT diagnostic pair still reports the expected store access fault",
        excpt.triggered == true &&
        excpt.cause == CAUSE_SAF &&
        excpt.tval == store_vaddr
    );
    goto_priv(PRIV_M);
    TEST_ASSERT("the trailing marker store in the older-load-first no-H mixed PBMT diagnostic pair executes only after both precise traps have been observed",
        read64(marker_paddr) == 1ULL
    );

    first = (struct ai_micro_m_memblock_exception_snapshot){0};

    goto_priv(PRIV_M);
    ai_micro_m_memblock_reset_env();
    ai_micro_m_memblock_reset_pbmt_env();
    write64(marker_paddr, 0ULL);

    ai_micro_m_memblock_prepare_pbmt_translation();
    load_vaddr = ai_micro_m_memblock_configure_pbmt_alias(load_alias_page,
        bad_paddr_load);
    store_vaddr = ai_micro_m_memblock_configure_pbmt_alias(store_alias_page,
        bad_paddr_store);

    TEST_SETUP_EXCEPT();
    ai_micro_m_memblock_translated_store_load_fault_pair_capture(
        store_vaddr, load_vaddr, marker_paddr, 0x1111111122222222ULL, &first);

    TEST_ASSERT("in the reverse no-H mixed PBMT diagnostic pair with older PBMT=IO bad-alias store and younger PBMT=NC bad-alias load, the first observed precise trap remains the older store access fault",
        first.triggered == true &&
        first.cause == CAUSE_SAF &&
        first.tval == store_vaddr
    );
    TEST_ASSERT("after the in-sequence rearm step, the younger PBMT=NC bad-alias load in the reverse no-H mixed PBMT diagnostic pair still reports the expected load access fault",
        excpt.triggered == true &&
        excpt.cause == CAUSE_LAF &&
        excpt.tval == load_vaddr
    );
    goto_priv(PRIV_M);
    TEST_ASSERT("the trailing marker store in the reverse no-H mixed PBMT diagnostic pair executes only after both precise traps have been observed",
        read64(marker_paddr) == 1ULL
    );

    ai_micro_m_memblock_reset_pbmt_env();
    ai_micro_m_memblock_reset_env();

    TEST_END();
}

bool ai_micro_m_load_pbmt_nc_bad_good_store_pbmt_io_next_bad_fault_refreshes_to_new_alias_corner()
{
    TEST_START();

    const enum test_page load_alias_page = AI_MICRO_MEMBLOCK_PBMT_NC_ALIAS_PAGE;
    const enum test_page store_alias_page = AI_MICRO_MEMBLOCK_PBMT_IO_ALIAS_PAGE;
    const uintptr_t first_bad_paddr_load = AI_MICRO_MEMBLOCK_PBMT_BAD_PADDR_ALT;
    const uintptr_t second_bad_paddr_store = AI_MICRO_MEMBLOCK_PBMT_BAD_PADDR;
    const uintptr_t first_bad_vaddr =
        hs_page_base(load_alias_page) +
        (first_bad_paddr_load & (PAGE_SIZE - 1U));
    const uintptr_t second_bad_vaddr =
        hs_page_base(store_alias_page) +
        (second_bad_paddr_store & (PAGE_SIZE - 1U));
    const uintptr_t good_paddr =
        ai_micro_m_memblock_good_mmio64_paddr(0x500U);
    const uint64_t initial_good = 0x1020304050607080ULL;
    const uint64_t middle_store = 0x7766554433221100ULL;
    uintptr_t good_vaddr = 0U;

    goto_priv(PRIV_M);
    ai_micro_m_memblock_reset_env();
    ai_micro_m_memblock_reset_pbmt_env();
    write64(good_paddr, initial_good);

    ai_micro_m_memblock_prepare_pbmt_translation();
    (void)ai_micro_m_memblock_configure_pbmt_alias(load_alias_page,
        first_bad_paddr_load);

    TEST_SETUP_EXCEPT();
    (void)ai_micro_m_memblock_translated_ld(first_bad_vaddr);
    TEST_ASSERT("the first no-H mixed PBMT bad-alias load in the NC-load-good-IO-store-next-bad refresh probe should report a precise load access fault at the first NC bad alias",
        excpt.triggered == true &&
        excpt.cause == CAUSE_LAF &&
        excpt.tval == first_bad_vaddr
    );

    good_vaddr = ai_micro_m_memblock_configure_pbmt_alias(store_alias_page,
        good_paddr);

    TEST_SETUP_EXCEPT();
    ai_micro_m_memblock_translated_sd(good_vaddr, middle_store);
    TEST_ASSERT("after the first no-H mixed PBMT NC bad-alias load fault, retargeting the IO alias page to a good address should let the middle good store complete without inheriting stale exception state",
        excpt.triggered == false
    );

    (void)ai_micro_m_memblock_configure_pbmt_alias(store_alias_page,
        second_bad_paddr_store);

    TEST_SETUP_EXCEPT();
    ai_micro_m_memblock_translated_sd(second_bad_vaddr, 0x5aa55aa55aa55aa5ULL);
    TEST_ASSERT("after the middle good no-H mixed PBMT IO store has already completed, the next IO bad-alias store must refresh cause+tval to the newer store-side bad alias instead of reusing stale metadata from the earlier NC bad-alias load episode",
        excpt.triggered == true &&
        excpt.cause == CAUSE_SAF &&
        excpt.tval == second_bad_vaddr
    );

    goto_priv(PRIV_M);
    TEST_ASSERT("the no-H mixed PBMT NC-load-good-IO-store-next-bad refresh probe leaves the good backing word with only the successful middle-store payload",
        read64(good_paddr) == middle_store
    );

    ai_micro_m_memblock_reset_pbmt_env();
    ai_micro_m_memblock_reset_env();

    TEST_END();
}

bool ai_micro_m_store_pbmt_io_bad_good_load_pbmt_nc_next_bad_fault_refreshes_to_new_alias_corner()
{
    TEST_START();

    const enum test_page store_alias_page = AI_MICRO_MEMBLOCK_PBMT_IO_ALIAS_PAGE;
    const enum test_page load_alias_page = AI_MICRO_MEMBLOCK_PBMT_NC_ALIAS_PAGE;
    const uintptr_t first_bad_paddr_store = AI_MICRO_MEMBLOCK_PBMT_BAD_PADDR;
    const uintptr_t second_bad_paddr_load = AI_MICRO_MEMBLOCK_PBMT_BAD_PADDR_ALT;
    const uintptr_t first_bad_vaddr =
        hs_page_base(store_alias_page) +
        (first_bad_paddr_store & (PAGE_SIZE - 1U));
    const uintptr_t second_bad_vaddr =
        hs_page_base(load_alias_page) +
        (second_bad_paddr_load & (PAGE_SIZE - 1U));
    static const uint64_t middle_good = 0xfe66778899aabbccULL;
    const uintptr_t scratch_paddr = phys_page_base(SWITCH2) + 0x540U;
    const uintptr_t good_paddr = scratch_paddr;
    uintptr_t good_vaddr = 0U;

    goto_priv(PRIV_M);
    ai_micro_m_memblock_reset_env();
    ai_micro_m_memblock_reset_pbmt_env();
    ai_micro_m_memblock_prepare_pbmt_translation();
    TEST_ASSERT("PBMT setup store seeds the later good-load backing doubleword through the same uncache route instead of relying on cacheable-to-uncache visibility",
        ai_micro_m_memblock_seed_pbmt_alias_doubleword(
            load_alias_page, good_paddr, middle_good)
    );
    (void)ai_micro_m_memblock_configure_pbmt_alias(store_alias_page,
        first_bad_paddr_store);

    TEST_SETUP_EXCEPT();
    ai_micro_m_memblock_translated_sd(first_bad_vaddr, 0xa55aa55aa55aa55aULL);
    TEST_ASSERT("the first no-H mixed PBMT bad-alias store in the IO-store-good-NC-load-next-bad refresh probe should report a precise store access fault at the first IO bad alias",
        excpt.triggered == true &&
        excpt.cause == CAUSE_SAF &&
        excpt.tval == first_bad_vaddr
    );

    good_vaddr = ai_micro_m_memblock_configure_pbmt_alias(load_alias_page,
        good_paddr);

    TEST_SETUP_EXCEPT();
    uint64_t observed_middle = ai_micro_m_memblock_translated_ld(good_vaddr);
    TEST_ASSERT("after the first no-H mixed PBMT IO bad-alias store fault, retargeting the NC alias page to a good address should let the middle good load complete without inheriting stale exception state",
        excpt.triggered == false &&
        observed_middle == middle_good
    );

    (void)ai_micro_m_memblock_configure_pbmt_alias(load_alias_page,
        second_bad_paddr_load);

    TEST_SETUP_EXCEPT();
    (void)ai_micro_m_memblock_translated_ld(second_bad_vaddr);
    TEST_ASSERT("after the middle good no-H mixed PBMT NC load has already completed, the next NC bad-alias load must refresh cause+tval to the newer load-side bad alias instead of reusing stale metadata from the earlier IO bad-alias store episode",
        excpt.triggered == true &&
        excpt.cause == CAUSE_LAF &&
        excpt.tval == second_bad_vaddr
    );

    goto_priv(PRIV_M);
    TEST_ASSERT("the no-H mixed PBMT IO-store-good-NC-load-next-bad refresh probe leaves the good backing word unchanged across both bad-alias episodes",
        read64(good_paddr) == middle_good
    );

    ai_micro_m_memblock_reset_pbmt_env();
    ai_micro_m_memblock_reset_env();

    TEST_END();
}

bool ai_micro_m_load_pbmt_nc_bad_good_store_pbmt_io_next_bad_width_switch_refreshes_new_alias_corner()
{
    TEST_START();

    static const unsigned load_sizes[] = {
        sizeof(uint8_t),
        sizeof(uint16_t),
        sizeof(uint32_t),
    };
    static const unsigned load_offsets[] = { 7U, 6U, 4U };
    static const bool load_signed[] = { true, false, true };
    static const unsigned store_sizes[] = {
        sizeof(uint16_t),
        sizeof(uint32_t),
        sizeof(uint8_t),
    };
    static const unsigned store_offsets[] = { 2U, 4U, 7U };
    static const uintptr_t good_offsets[] = { 0x580U, 0x5c0U, 0x600U };
    static const uint64_t initial_goods[] = {
        0x1817161514131211ULL,
        0x2827262524232221ULL,
        0x3837363534333231ULL,
    };
    static const uint64_t middle_store_values[] = {
        0x000000000000b4c3ULL,
        0x00000000d3e4f506ULL,
        0x00000000000000a5ULL,
    };

    const enum test_page load_alias_page = AI_MICRO_MEMBLOCK_PBMT_NC_ALIAS_PAGE;
    const enum test_page store_alias_page = AI_MICRO_MEMBLOCK_PBMT_IO_ALIAS_PAGE;
    const uintptr_t first_bad_paddr_load = AI_MICRO_MEMBLOCK_PBMT_BAD_PADDR_ALT;
    const uintptr_t second_bad_paddr_store = AI_MICRO_MEMBLOCK_PBMT_BAD_PADDR;

    for (unsigned round = 0; round < 3U; ++round) {
        const uintptr_t first_bad_vaddr =
            hs_page_base(load_alias_page) +
            ((first_bad_paddr_load + load_offsets[round]) &
                (PAGE_SIZE - 1U));
        const uintptr_t second_bad_vaddr =
            hs_page_base(store_alias_page) +
            ((second_bad_paddr_store + store_offsets[round]) &
                (PAGE_SIZE - 1U));
        const uintptr_t good_paddr =
            ai_micro_m_memblock_good_paddr_for_alias(
                store_alias_page, good_offsets[round]);
        const uint64_t expected_middle = ai_micro_m_memblock_merge_unsigned(
            initial_goods[round], middle_store_values[round],
            store_offsets[round], store_sizes[round]);
        uintptr_t good_vaddr = 0U;

        goto_priv(PRIV_M);
        ai_micro_m_memblock_reset_env();
        ai_micro_m_memblock_reset_pbmt_env();
        write64(good_paddr, initial_goods[round]);

        ai_micro_m_memblock_prepare_pbmt_translation();
        (void)ai_micro_m_memblock_configure_pbmt_alias(load_alias_page,
            first_bad_paddr_load);

        ai_micro_m_memblock_enable_mprv_supervisor_translation();
        TEST_SETUP_EXCEPT();
        (void)ai_micro_m_memblock_scalar_load_current_decode(
            load_signed[round], load_sizes[round], first_bad_vaddr);
        ai_micro_m_memblock_disable_mprv_supervisor_translation();
        TEST_ASSERT("the first no-H mixed PBMT width-switch probe should report a precise NC bad-alias load fault at the first narrow load alias",
            excpt.triggered == true &&
            excpt.cause == CAUSE_LAF &&
            excpt.tval == first_bad_vaddr
        );

        good_vaddr = ai_micro_m_memblock_configure_pbmt_alias(store_alias_page,
            good_paddr);

        TEST_SETUP_EXCEPT();
        ai_micro_m_memblock_translated_store_by_size(
            good_vaddr + store_offsets[round], store_sizes[round],
            middle_store_values[round]);
        TEST_ASSERT("after the first no-H mixed PBMT NC bad-alias narrow load fault, retargeting the IO alias page to a good address and switching width should let the middle good store complete without inheriting stale exception state",
            excpt.triggered == false
        );

        (void)ai_micro_m_memblock_configure_pbmt_alias(store_alias_page,
            second_bad_paddr_store);

        TEST_SETUP_EXCEPT();
        ai_micro_m_memblock_translated_store_by_size(
            second_bad_vaddr, store_sizes[round], middle_store_values[round]);
        TEST_ASSERT("after the middle good no-H mixed PBMT IO store has already completed with the switched width, the next IO bad-alias store must refresh cause+tval to the newer bad alias instead of reusing stale metadata from the earlier NC narrow load episode",
            excpt.triggered == true &&
            excpt.cause == CAUSE_SAF &&
            excpt.tval == second_bad_vaddr
        );

        goto_priv(PRIV_M);
        TEST_ASSERT("the no-H mixed PBMT NC-load-good-IO-store width-switch next-bad refresh probe leaves the good backing doubleword with only the successful middle-store payload",
            read64(good_paddr) == expected_middle
        );
    }

    ai_micro_m_memblock_reset_pbmt_env();
    ai_micro_m_memblock_reset_env();

    TEST_END();
}

bool ai_micro_m_store_pbmt_io_bad_good_load_pbmt_nc_next_bad_width_switch_refreshes_new_alias_corner()
{
    TEST_START();

    static const unsigned store_sizes[] = {
        sizeof(uint8_t),
        sizeof(uint16_t),
        sizeof(uint32_t),
    };
    static const unsigned store_offsets[] = { 7U, 2U, 4U };
    static const unsigned load_sizes[] = {
        sizeof(uint16_t),
        sizeof(uint32_t),
        sizeof(uint8_t),
    };
    static const unsigned load_offsets[] = { 6U, 4U, 7U };
    static const bool load_signed[] = { false, true, false };
    static const uintptr_t scratch_offsets[] = { 0x640U, 0x680U, 0x6c0U };
    static const uint64_t good_values[] = {
        0x8abc01234567def0ULL,
        0xf1234567a0b0c0d0ULL,
        0xfe66778899aabbccULL,
    };

    const enum test_page store_alias_page = AI_MICRO_MEMBLOCK_PBMT_IO_ALIAS_PAGE;
    const enum test_page load_alias_page = AI_MICRO_MEMBLOCK_PBMT_NC_ALIAS_PAGE;
    const uintptr_t first_bad_paddr_store = AI_MICRO_MEMBLOCK_PBMT_BAD_PADDR;
    const uintptr_t second_bad_paddr_load = AI_MICRO_MEMBLOCK_PBMT_BAD_PADDR_ALT;

    for (unsigned round = 0; round < 3U; ++round) {
        const uintptr_t first_bad_vaddr =
            hs_page_base(store_alias_page) +
            ((first_bad_paddr_store + store_offsets[round]) &
                (PAGE_SIZE - 1U));
        const uintptr_t second_bad_vaddr =
            hs_page_base(load_alias_page) +
            ((second_bad_paddr_load + load_offsets[round]) &
                (PAGE_SIZE - 1U));
        const uintptr_t scratch_paddr =
            phys_page_base(SWITCH2) + scratch_offsets[round];
        const uintptr_t good_paddr = scratch_paddr;
        const uint64_t expected_middle =
            ai_micro_m_memblock_extract_scalar(good_values[round],
                load_offsets[round], load_sizes[round], load_signed[round]);
        uintptr_t good_vaddr = 0U;

        goto_priv(PRIV_M);
        ai_micro_m_memblock_reset_env();
        ai_micro_m_memblock_reset_pbmt_env();

        ai_micro_m_memblock_prepare_pbmt_translation();
        TEST_ASSERT("PBMT setup store seeds the later good-load backing doubleword through the same uncache route instead of relying on cacheable-to-uncache visibility",
            ai_micro_m_memblock_seed_pbmt_alias_doubleword(
                load_alias_page, good_paddr, good_values[round])
        );
        (void)ai_micro_m_memblock_configure_pbmt_alias(store_alias_page,
            first_bad_paddr_store);

        TEST_SETUP_EXCEPT();
        ai_micro_m_memblock_translated_store_by_size(
            first_bad_vaddr, store_sizes[round], 0xa55aa55aa55aa55aULL);
        TEST_ASSERT("the first no-H mixed PBMT width-switch probe should report a precise IO bad-alias store fault at the first width-switched store alias",
            excpt.triggered == true &&
            excpt.cause == CAUSE_SAF &&
            excpt.tval == first_bad_vaddr
        );

        good_vaddr = ai_micro_m_memblock_configure_pbmt_alias(load_alias_page,
            good_paddr);

        TEST_SETUP_EXCEPT();
        uint64_t observed_middle = ai_micro_m_memblock_translated_scalar_load(
            load_signed[round], load_sizes[round],
            good_vaddr + load_offsets[round]);
        TEST_ASSERT("after the first no-H mixed PBMT IO bad-alias width-switched store fault, retargeting the NC alias page to a good address and switching decode should let the middle good load return the value selected by its current decode",
            excpt.triggered == false &&
            observed_middle == expected_middle
        );

        (void)ai_micro_m_memblock_configure_pbmt_alias(load_alias_page,
            second_bad_paddr_load);

        ai_micro_m_memblock_enable_mprv_supervisor_translation();
        TEST_SETUP_EXCEPT();
        (void)ai_micro_m_memblock_scalar_load_current_decode(
            load_signed[round], load_sizes[round], second_bad_vaddr);
        ai_micro_m_memblock_disable_mprv_supervisor_translation();
        TEST_ASSERT("after the middle good no-H mixed PBMT NC load has already completed with the switched decode, the next NC bad-alias load must refresh cause+tval to the newer bad alias instead of reusing stale metadata from the earlier IO width-switched store episode",
            excpt.triggered == true &&
            excpt.cause == CAUSE_LAF &&
            excpt.tval == second_bad_vaddr
        );

        goto_priv(PRIV_M);
        TEST_ASSERT("the no-H mixed PBMT IO-store-good-NC-load width-switch next-bad refresh probe leaves the good backing doubleword unchanged across both bad-alias episodes",
            read64(good_paddr) == good_values[round]
        );
    }

    ai_micro_m_memblock_reset_pbmt_env();
    ai_micro_m_memblock_reset_env();

    TEST_END();
}

bool ai_micro_m_load_pbmt_nc_bad_good_store_pbmt_io_next_bad_load_width_switch_refreshes_new_alias_corner()
{
    TEST_START();

    static const unsigned first_load_sizes[] = {
        sizeof(uint8_t),
        sizeof(uint16_t),
        sizeof(uint32_t),
    };
    static const unsigned first_load_offsets[] = { 7U, 6U, 4U };
    static const bool first_load_signed[] = { true, false, true };
    static const unsigned store_sizes[] = {
        sizeof(uint16_t),
        sizeof(uint32_t),
        sizeof(uint8_t),
    };
    static const unsigned store_offsets[] = { 2U, 4U, 7U };
    static const unsigned second_load_sizes[] = {
        sizeof(uint16_t),
        sizeof(uint32_t),
        sizeof(uint8_t),
    };
    static const unsigned second_load_offsets[] = { 6U, 4U, 7U };
    static const bool second_load_signed[] = { false, true, false };
    static const uintptr_t good_offsets[] = { 0x700U, 0x740U, 0x780U };
    static const uint64_t initial_goods[] = {
        0x1112131415161718ULL,
        0x2122232425262728ULL,
        0x3132333435363738ULL,
    };
    static const uint64_t middle_store_values[] = {
        0x000000000000d2c1ULL,
        0x00000000b4a59687ULL,
        0x000000000000004eULL,
    };

    const enum test_page load_alias_page = AI_MICRO_MEMBLOCK_PBMT_NC_ALIAS_PAGE;
    const enum test_page store_alias_page = AI_MICRO_MEMBLOCK_PBMT_IO_ALIAS_PAGE;
    const uintptr_t first_bad_paddr_load = AI_MICRO_MEMBLOCK_PBMT_BAD_PADDR_ALT;
    const uintptr_t second_bad_paddr_load = AI_MICRO_MEMBLOCK_PBMT_BAD_PADDR;

    for (unsigned round = 0; round < 3U; ++round) {
        const uintptr_t first_bad_vaddr =
            hs_page_base(load_alias_page) +
            ((first_bad_paddr_load + first_load_offsets[round]) &
                (PAGE_SIZE - 1U));
        const uintptr_t second_bad_vaddr =
            hs_page_base(load_alias_page) +
            ((second_bad_paddr_load + second_load_offsets[round]) &
                (PAGE_SIZE - 1U));
        const uintptr_t good_paddr =
            ai_micro_m_memblock_good_paddr_for_alias(
                store_alias_page, good_offsets[round]);
        const uint64_t expected_middle = ai_micro_m_memblock_merge_unsigned(
            initial_goods[round], middle_store_values[round],
            store_offsets[round], store_sizes[round]);
        uintptr_t good_vaddr = 0U;

        goto_priv(PRIV_M);
        ai_micro_m_memblock_reset_env();
        ai_micro_m_memblock_reset_pbmt_env();
        write64(good_paddr, initial_goods[round]);

        ai_micro_m_memblock_prepare_pbmt_translation();
        (void)ai_micro_m_memblock_configure_pbmt_alias(load_alias_page,
            first_bad_paddr_load);

        ai_micro_m_memblock_enable_mprv_supervisor_translation();
        TEST_SETUP_EXCEPT();
        (void)ai_micro_m_memblock_scalar_load_current_decode(
            first_load_signed[round], first_load_sizes[round], first_bad_vaddr);
        ai_micro_m_memblock_disable_mprv_supervisor_translation();
        TEST_ASSERT("the first no-H mixed same-producer width-switch probe should report a precise NC bad-alias load fault at the first narrow load alias",
            excpt.triggered == true &&
            excpt.cause == CAUSE_LAF &&
            excpt.tval == first_bad_vaddr
        );

        good_vaddr = ai_micro_m_memblock_configure_pbmt_alias(store_alias_page,
            good_paddr);

        TEST_SETUP_EXCEPT();
        ai_micro_m_memblock_translated_store_by_size(
            good_vaddr + store_offsets[round], store_sizes[round],
            middle_store_values[round]);
        TEST_ASSERT("after the first no-H mixed NC bad-alias narrow load fault, retargeting the IO alias page to a good address and switching width should let the middle good store complete without inheriting stale exception state",
            excpt.triggered == false
        );

        goto_priv(PRIV_M);
        TEST_ASSERT("the no-H mixed same-producer width-switch probe leaves the good backing doubleword with only the successful middle-store payload before the younger same-producer load fault is injected",
            read64(good_paddr) == expected_middle
        );

        (void)ai_micro_m_memblock_configure_pbmt_alias(load_alias_page,
            second_bad_paddr_load);

        ai_micro_m_memblock_enable_mprv_supervisor_translation();
        TEST_SETUP_EXCEPT();
        (void)ai_micro_m_memblock_scalar_load_current_decode(
            second_load_signed[round], second_load_sizes[round],
            second_bad_vaddr);
        ai_micro_m_memblock_disable_mprv_supervisor_translation();
        TEST_ASSERT("after the middle good no-H mixed IO width-switched store has already completed, the next NC bad-alias load with the switched width/decode must refresh tval to the newer bad alias instead of reusing stale metadata from the first NC narrow load episode",
            excpt.triggered == true &&
            excpt.cause == CAUSE_LAF &&
            excpt.tval == second_bad_vaddr
        );

        goto_priv(PRIV_M);
        TEST_ASSERT("the no-H mixed NC-load-good-IO-store-next-bad-load width-switch refresh probe keeps the good backing doubleword equal to the successful middle-store payload",
            read64(good_paddr) == expected_middle
        );
    }

    ai_micro_m_memblock_reset_pbmt_env();
    ai_micro_m_memblock_reset_env();

    TEST_END();
}

bool ai_micro_m_store_pbmt_io_bad_good_load_pbmt_nc_next_bad_store_width_switch_refreshes_new_alias_corner()
{
    TEST_START();

    static const unsigned first_store_sizes[] = {
        sizeof(uint8_t),
        sizeof(uint16_t),
        sizeof(uint32_t),
    };
    static const unsigned first_store_offsets[] = { 7U, 2U, 4U };
    static const unsigned load_sizes[] = {
        sizeof(uint16_t),
        sizeof(uint32_t),
        sizeof(uint8_t),
    };
    static const unsigned load_offsets[] = { 6U, 4U, 7U };
    static const bool load_signed[] = { false, true, false };
    static const unsigned second_store_sizes[] = {
        sizeof(uint16_t),
        sizeof(uint32_t),
        sizeof(uint8_t),
    };
    static const unsigned second_store_offsets[] = { 2U, 4U, 7U };
    static const uintptr_t scratch_offsets[] = { 0x7c0U, 0x800U, 0x840U };
    static const uint64_t good_values[] = {
        0x8abc01234567def0ULL,
        0xf1234567a0b0c0d0ULL,
        0xfe66778899aabbccULL,
    };

    const enum test_page store_alias_page = AI_MICRO_MEMBLOCK_PBMT_IO_ALIAS_PAGE;
    const enum test_page load_alias_page = AI_MICRO_MEMBLOCK_PBMT_NC_ALIAS_PAGE;
    const uintptr_t first_bad_paddr_store = AI_MICRO_MEMBLOCK_PBMT_BAD_PADDR;
    const uintptr_t second_bad_paddr_store = AI_MICRO_MEMBLOCK_PBMT_BAD_PADDR_ALT;

    for (unsigned round = 0; round < 3U; ++round) {
        const uintptr_t first_bad_vaddr =
            hs_page_base(store_alias_page) +
            ((first_bad_paddr_store + first_store_offsets[round]) &
                (PAGE_SIZE - 1U));
        const uintptr_t second_bad_vaddr =
            hs_page_base(store_alias_page) +
            ((second_bad_paddr_store + second_store_offsets[round]) &
                (PAGE_SIZE - 1U));
        const uintptr_t scratch_paddr =
            phys_page_base(SWITCH2) + scratch_offsets[round];
        const uintptr_t good_paddr = scratch_paddr;
        uintptr_t good_vaddr = 0U;

        goto_priv(PRIV_M);
        ai_micro_m_memblock_reset_env();
        ai_micro_m_memblock_reset_pbmt_env();

        ai_micro_m_memblock_prepare_pbmt_translation();
        TEST_ASSERT("PBMT setup store seeds the later good-load backing doubleword through the same uncache route instead of relying on cacheable-to-uncache visibility",
            ai_micro_m_memblock_seed_pbmt_alias_doubleword(
                load_alias_page, good_paddr, good_values[round])
        );
        (void)ai_micro_m_memblock_configure_pbmt_alias(store_alias_page,
            first_bad_paddr_store);

        TEST_SETUP_EXCEPT();
        ai_micro_m_memblock_translated_store_by_size(
            first_bad_vaddr, first_store_sizes[round], 0xa55aa55aa55aa55aULL);
        TEST_ASSERT("the first no-H mixed same-producer width-switch probe should report a precise IO bad-alias store fault at the first width-switched store alias",
            excpt.triggered == true &&
            excpt.cause == CAUSE_SAF &&
            excpt.tval == first_bad_vaddr
        );

        good_vaddr = ai_micro_m_memblock_configure_pbmt_alias(load_alias_page,
            good_paddr);

        TEST_SETUP_EXCEPT();
        uint64_t observed_middle = ai_micro_m_memblock_translated_scalar_load(
            load_signed[round], load_sizes[round],
            good_vaddr + load_offsets[round]);
        TEST_ASSERT("after the first no-H mixed IO bad-alias width-switched store fault, retargeting the NC alias page to a good address and switching decode should let the middle good load return the value selected by its current decode",
            excpt.triggered == false &&
            observed_middle ==
                ai_micro_m_memblock_extract_scalar(good_values[round],
                    load_offsets[round], load_sizes[round], load_signed[round])
        );

        goto_priv(PRIV_M);
        TEST_ASSERT("the no-H mixed same-producer width-switch probe leaves the good backing doubleword unchanged before the younger same-producer store fault is injected",
            read64(good_paddr) == good_values[round]
        );

        (void)ai_micro_m_memblock_configure_pbmt_alias(store_alias_page,
            second_bad_paddr_store);

        TEST_SETUP_EXCEPT();
        ai_micro_m_memblock_translated_store_by_size(
            second_bad_vaddr, second_store_sizes[round],
            0x5aa55aa55aa55aa5ULL);
        TEST_ASSERT("after the middle good no-H mixed NC load with switched decode has already completed, the next IO bad-alias store with the switched width must refresh tval to the newer bad alias instead of reusing stale metadata from the first IO width-switched store episode",
            excpt.triggered == true &&
            excpt.cause == CAUSE_SAF &&
            excpt.tval == second_bad_vaddr
        );

        goto_priv(PRIV_M);
        TEST_ASSERT("the no-H mixed IO-store-good-NC-load-next-bad-store width-switch refresh probe leaves the good backing doubleword unchanged across both bad-alias episodes",
            read64(good_paddr) == good_values[round]
        );
    }

    ai_micro_m_memblock_reset_pbmt_env();
    ai_micro_m_memblock_reset_env();

    TEST_END();
}

bool ai_micro_m_load_pbmt_nc_bad_good_store_pbmt_io_next_bad_store_width_switch_refreshes_current_alias_corner()
{
    TEST_START();

    static const unsigned first_load_sizes[] = {
        sizeof(uint8_t),
        sizeof(uint16_t),
        sizeof(uint32_t),
    };
    static const unsigned first_load_offsets[] = { 7U, 6U, 4U };
    static const bool first_load_signed[] = { true, false, true };
    static const unsigned middle_store_sizes[] = {
        sizeof(uint16_t),
        sizeof(uint32_t),
        sizeof(uint8_t),
    };
    static const unsigned middle_store_offsets[] = { 2U, 4U, 7U };
    static const uint64_t middle_store_values[] = {
        0x000000000000d2c1ULL,
        0x00000000b4a59687ULL,
        0x000000000000004eULL,
    };
    static const unsigned final_store_sizes[] = {
        sizeof(uint32_t),
        sizeof(uint8_t),
        sizeof(uint16_t),
    };
    static const unsigned final_store_offsets[] = { 4U, 7U, 2U };
    static const uint64_t final_store_values[] = {
        0x0000000011223344ULL,
        0x000000000000005aULL,
        0x000000000000bcadULL,
    };
    static const uintptr_t good_offsets[] = { 0x880U, 0x8c0U, 0x900U };
    static const uint64_t initial_goods[] = {
        0x5152535455565758ULL,
        0x6162636465666768ULL,
        0x7172737475767778ULL,
    };

    const enum test_page load_alias_page = AI_MICRO_MEMBLOCK_PBMT_NC_ALIAS_PAGE;
    const enum test_page store_alias_page = AI_MICRO_MEMBLOCK_PBMT_IO_ALIAS_PAGE;
    const uintptr_t first_bad_paddr_load = AI_MICRO_MEMBLOCK_PBMT_BAD_PADDR_ALT;
    const uintptr_t second_bad_paddr_store = AI_MICRO_MEMBLOCK_PBMT_BAD_PADDR;

    for (unsigned round = 0; round < 3U; ++round) {
        const uintptr_t first_bad_vaddr =
            hs_page_base(load_alias_page) +
            ((first_bad_paddr_load + first_load_offsets[round]) &
                (PAGE_SIZE - 1U));
        const uintptr_t second_bad_vaddr =
            hs_page_base(store_alias_page) +
            ((second_bad_paddr_store + final_store_offsets[round]) &
                (PAGE_SIZE - 1U));
        const uintptr_t good_paddr =
            ai_micro_m_memblock_good_paddr_for_alias(
                store_alias_page, good_offsets[round]);
        const uint64_t expected_middle = ai_micro_m_memblock_merge_unsigned(
            initial_goods[round], middle_store_values[round],
            middle_store_offsets[round], middle_store_sizes[round]);
        uintptr_t good_vaddr = 0U;

        goto_priv(PRIV_M);
        ai_micro_m_memblock_reset_env();
        ai_micro_m_memblock_reset_pbmt_env();
        write64(good_paddr, initial_goods[round]);

        ai_micro_m_memblock_prepare_pbmt_translation();
        (void)ai_micro_m_memblock_configure_pbmt_alias(load_alias_page,
            first_bad_paddr_load);

        ai_micro_m_memblock_enable_mprv_supervisor_translation();
        TEST_SETUP_EXCEPT();
        (void)ai_micro_m_memblock_scalar_load_current_decode(
            first_load_signed[round], first_load_sizes[round], first_bad_vaddr);
        ai_micro_m_memblock_disable_mprv_supervisor_translation();
        TEST_ASSERT("the first no-H mixed tighter width-switch probe should report a precise NC bad-alias load fault at the first narrow load alias",
            excpt.triggered == true &&
            excpt.cause == CAUSE_LAF &&
            excpt.tval == first_bad_vaddr
        );

        good_vaddr = ai_micro_m_memblock_configure_pbmt_alias(store_alias_page,
            good_paddr);

        TEST_SETUP_EXCEPT();
        ai_micro_m_memblock_translated_store_by_size(
            good_vaddr + middle_store_offsets[round],
            middle_store_sizes[round], middle_store_values[round]);
        TEST_ASSERT("after the first no-H mixed NC bad-alias load fault, retargeting the IO alias page to a good address and using the middle switched-width store shape should let the good store complete without inheriting stale exception state",
            excpt.triggered == false
        );

        goto_priv(PRIV_M);
        TEST_ASSERT("the no-H mixed tighter width-switch probe leaves the good backing doubleword with only the successful middle-store payload before the younger switched-shape store fault is injected",
            read64(good_paddr) == expected_middle
        );

        (void)ai_micro_m_memblock_configure_pbmt_alias(store_alias_page,
            second_bad_paddr_store);

        TEST_SETUP_EXCEPT();
        ai_micro_m_memblock_translated_store_by_size(
            second_bad_vaddr, final_store_sizes[round],
            final_store_values[round]);
        TEST_ASSERT("after the middle good no-H mixed IO store has already completed, the next IO bad-alias store with a different switched shape must refresh cause+tval to its current bad alias instead of reusing stale metadata from either the first NC load or the middle IO store shape",
            excpt.triggered == true &&
            excpt.cause == CAUSE_SAF &&
            excpt.tval == second_bad_vaddr
        );

        goto_priv(PRIV_M);
        TEST_ASSERT("the no-H mixed NC-load-good-IO-store-next-bad-store tighter width-switch refresh probe keeps the good backing doubleword equal to the successful middle-store payload",
            read64(good_paddr) == expected_middle
        );
    }

    ai_micro_m_memblock_reset_pbmt_env();
    ai_micro_m_memblock_reset_env();

    TEST_END();
}

bool ai_micro_m_store_pbmt_io_bad_good_load_pbmt_nc_next_bad_load_width_switch_refreshes_current_alias_corner()
{
    TEST_START();

    static const unsigned first_store_sizes[] = {
        sizeof(uint8_t),
        sizeof(uint16_t),
        sizeof(uint32_t),
    };
    static const unsigned first_store_offsets[] = { 7U, 2U, 4U };
    static const unsigned middle_load_sizes[] = {
        sizeof(uint16_t),
        sizeof(uint32_t),
        sizeof(uint8_t),
    };
    static const unsigned middle_load_offsets[] = { 6U, 4U, 7U };
    static const bool middle_load_signed[] = { false, true, false };
    static const unsigned final_load_sizes[] = {
        sizeof(uint32_t),
        sizeof(uint8_t),
        sizeof(uint16_t),
    };
    static const unsigned final_load_offsets[] = { 4U, 7U, 6U };
    static const bool final_load_signed[] = { true, false, true };
    static const uintptr_t scratch_offsets[] = { 0x940U, 0x980U, 0x9c0U };
    static const uint64_t good_values[] = {
        0x8abc01234567def0ULL,
        0xf1234567a0b0c0d0ULL,
        0xfe66778899aabbccULL,
    };

    const enum test_page store_alias_page = AI_MICRO_MEMBLOCK_PBMT_IO_ALIAS_PAGE;
    const enum test_page load_alias_page = AI_MICRO_MEMBLOCK_PBMT_NC_ALIAS_PAGE;
    const uintptr_t first_bad_paddr_store = AI_MICRO_MEMBLOCK_PBMT_BAD_PADDR;
    const uintptr_t second_bad_paddr_load = AI_MICRO_MEMBLOCK_PBMT_BAD_PADDR_ALT;

    for (unsigned round = 0; round < 3U; ++round) {
        const uintptr_t first_bad_vaddr =
            hs_page_base(store_alias_page) +
            ((first_bad_paddr_store + first_store_offsets[round]) &
                (PAGE_SIZE - 1U));
        const uintptr_t second_bad_vaddr =
            hs_page_base(load_alias_page) +
            ((second_bad_paddr_load + final_load_offsets[round]) &
                (PAGE_SIZE - 1U));
        const uintptr_t scratch_paddr =
            phys_page_base(SWITCH2) + scratch_offsets[round];
        const uintptr_t good_paddr = scratch_paddr;
        const uint64_t expected_middle =
            ai_micro_m_memblock_extract_scalar(good_values[round],
                middle_load_offsets[round], middle_load_sizes[round],
                middle_load_signed[round]);
        uintptr_t good_vaddr = 0U;

        goto_priv(PRIV_M);
        ai_micro_m_memblock_reset_env();
        ai_micro_m_memblock_reset_pbmt_env();

        ai_micro_m_memblock_prepare_pbmt_translation();
        TEST_ASSERT("PBMT setup store seeds the later good-load backing doubleword through the same uncache route instead of relying on cacheable-to-uncache visibility",
            ai_micro_m_memblock_seed_pbmt_alias_doubleword(
                load_alias_page, good_paddr, good_values[round])
        );
        (void)ai_micro_m_memblock_configure_pbmt_alias(store_alias_page,
            first_bad_paddr_store);

        TEST_SETUP_EXCEPT();
        ai_micro_m_memblock_translated_store_by_size(
            first_bad_vaddr, first_store_sizes[round], 0xa55aa55aa55aa55aULL);
        TEST_ASSERT("the first no-H mixed tighter width-switch probe should report a precise IO bad-alias store fault at the first width-switched store alias",
            excpt.triggered == true &&
            excpt.cause == CAUSE_SAF &&
            excpt.tval == first_bad_vaddr
        );

        good_vaddr = ai_micro_m_memblock_configure_pbmt_alias(load_alias_page,
            good_paddr);

        TEST_SETUP_EXCEPT();
        uint64_t observed_middle = ai_micro_m_memblock_translated_scalar_load(
            middle_load_signed[round], middle_load_sizes[round],
            good_vaddr + middle_load_offsets[round]);
        TEST_ASSERT("after the first no-H mixed IO bad-alias store fault, retargeting the NC alias page to a good address and using the middle switched decode should let the good load return the value selected by its current decode",
            excpt.triggered == false &&
            observed_middle == expected_middle
        );

        goto_priv(PRIV_M);
        TEST_ASSERT("the no-H mixed tighter width-switch probe leaves the good backing doubleword unchanged before the younger switched-shape load fault is injected",
            read64(good_paddr) == good_values[round]
        );

        (void)ai_micro_m_memblock_configure_pbmt_alias(load_alias_page,
            second_bad_paddr_load);

        ai_micro_m_memblock_enable_mprv_supervisor_translation();
        TEST_SETUP_EXCEPT();
        (void)ai_micro_m_memblock_scalar_load_current_decode(
            final_load_signed[round], final_load_sizes[round],
            second_bad_vaddr);
        ai_micro_m_memblock_disable_mprv_supervisor_translation();
        TEST_ASSERT("after the middle good no-H mixed NC load has already completed, the next NC bad-alias load with a different switched shape must refresh cause+tval to its current bad alias instead of reusing stale metadata from either the first IO store or the middle NC load shape",
            excpt.triggered == true &&
            excpt.cause == CAUSE_LAF &&
            excpt.tval == second_bad_vaddr
        );

        goto_priv(PRIV_M);
        TEST_ASSERT("the no-H mixed IO-store-good-NC-load-next-bad-load tighter width-switch refresh probe leaves the good backing doubleword unchanged across both bad-alias episodes",
            read64(good_paddr) == good_values[round]
        );
    }

    ai_micro_m_memblock_reset_pbmt_env();
    ai_micro_m_memblock_reset_env();

    TEST_END();
}

bool ai_micro_m_load_pbmt_nc_same_alias_bad_good_store_pbmt_io_then_same_alias_bad_load_width_switch_refreshes_current_alias_corner()
{
    TEST_START();

    static const unsigned first_load_sizes[] = {
        sizeof(uint8_t),
        sizeof(uint16_t),
        sizeof(uint32_t),
    };
    static const unsigned first_load_offsets[] = { 7U, 6U, 4U };
    static const bool first_load_signed[] = { true, false, true };
    static const unsigned middle_store_sizes[] = {
        sizeof(uint16_t),
        sizeof(uint32_t),
        sizeof(uint8_t),
    };
    static const unsigned middle_store_offsets[] = { 2U, 4U, 7U };
    static const uint64_t middle_store_values[] = {
        0x000000000000c3d2ULL,
        0x00000000a5968778ULL,
        0x000000000000005eULL,
    };
    static const unsigned final_load_sizes[] = {
        sizeof(uint32_t),
        sizeof(uint8_t),
        sizeof(uint16_t),
    };
    static const unsigned final_load_offsets[] = { 4U, 7U, 6U };
    static const bool final_load_signed[] = { false, true, false };
    static const uintptr_t good_offsets[] = { 0xa00U, 0xa40U, 0xa80U };
    static const uint64_t initial_goods[] = {
        0x4142434445464748ULL,
        0x5152535455565758ULL,
        0x6162636465666768ULL,
    };

    const enum test_page load_alias_page = AI_MICRO_MEMBLOCK_PBMT_NC_ALIAS_PAGE;
    const enum test_page store_alias_page = AI_MICRO_MEMBLOCK_PBMT_IO_ALIAS_PAGE;
    const uintptr_t bad_paddr_load = AI_MICRO_MEMBLOCK_PBMT_BAD_PADDR_ALT;

    for (unsigned round = 0; round < 3U; ++round) {
        const uintptr_t first_bad_vaddr =
            hs_page_base(load_alias_page) +
            ((bad_paddr_load + first_load_offsets[round]) &
                (PAGE_SIZE - 1U));
        const uintptr_t final_bad_vaddr =
            hs_page_base(load_alias_page) +
            ((bad_paddr_load + final_load_offsets[round]) &
                (PAGE_SIZE - 1U));
        const uintptr_t good_paddr =
            ai_micro_m_memblock_good_paddr_for_alias(
                store_alias_page, good_offsets[round]);
        const uint64_t expected_middle = ai_micro_m_memblock_merge_unsigned(
            initial_goods[round], middle_store_values[round],
            middle_store_offsets[round], middle_store_sizes[round]);
        uintptr_t good_vaddr = 0U;

        goto_priv(PRIV_M);
        ai_micro_m_memblock_reset_env();
        ai_micro_m_memblock_reset_pbmt_env();
        write64(good_paddr, initial_goods[round]);

        ai_micro_m_memblock_prepare_pbmt_translation();
        (void)ai_micro_m_memblock_configure_pbmt_alias(load_alias_page,
            bad_paddr_load);

        ai_micro_m_memblock_enable_mprv_supervisor_translation();
        TEST_SETUP_EXCEPT();
        (void)ai_micro_m_memblock_scalar_load_current_decode(
            first_load_signed[round], first_load_sizes[round], first_bad_vaddr);
        ai_micro_m_memblock_disable_mprv_supervisor_translation();
        TEST_ASSERT("the first no-H mixed same-alias probe should report a precise NC bad-alias load fault at the first narrow load shape",
            excpt.triggered == true &&
            excpt.cause == CAUSE_LAF &&
            excpt.tval == first_bad_vaddr
        );

        good_vaddr = ai_micro_m_memblock_configure_pbmt_alias(store_alias_page,
            good_paddr);

        TEST_SETUP_EXCEPT();
        ai_micro_m_memblock_translated_store_by_size(
            good_vaddr + middle_store_offsets[round],
            middle_store_sizes[round], middle_store_values[round]);
        TEST_ASSERT("after the first no-H mixed same-alias NC load fault, routing through an IO good store with a different width should complete without inheriting stale exception state",
            excpt.triggered == false
        );

        goto_priv(PRIV_M);
        TEST_ASSERT("the no-H mixed same-alias probe leaves the good backing doubleword with only the successful middle-store payload before the final same-alias bad load is injected",
            read64(good_paddr) == expected_middle
        );

        (void)ai_micro_m_memblock_configure_pbmt_alias(load_alias_page,
            bad_paddr_load);

        ai_micro_m_memblock_enable_mprv_supervisor_translation();
        TEST_SETUP_EXCEPT();
        (void)ai_micro_m_memblock_scalar_load_current_decode(
            final_load_signed[round], final_load_sizes[round], final_bad_vaddr);
        ai_micro_m_memblock_disable_mprv_supervisor_translation();
        TEST_ASSERT("after the middle good IO width-switched store has already completed, returning to the same NC bad alias with a different load shape must refresh tval to the current bad alias instead of reusing stale metadata from the first NC bad episode",
            excpt.triggered == true &&
            excpt.cause == CAUSE_LAF &&
            excpt.tval == final_bad_vaddr
        );

        goto_priv(PRIV_M);
        TEST_ASSERT("the no-H mixed same-alias NC-load-good-IO-store-then-same-alias-bad-load probe keeps the good backing doubleword equal to the successful middle-store payload",
            read64(good_paddr) == expected_middle
        );
    }

    ai_micro_m_memblock_reset_pbmt_env();
    ai_micro_m_memblock_reset_env();

    TEST_END();
}

bool ai_micro_m_store_pbmt_io_same_alias_bad_good_load_pbmt_nc_then_same_alias_bad_store_width_switch_refreshes_current_alias_corner()
{
    TEST_START();

    static const unsigned first_store_sizes[] = {
        sizeof(uint8_t),
        sizeof(uint16_t),
        sizeof(uint32_t),
    };
    static const unsigned first_store_offsets[] = { 7U, 2U, 4U };
    static const unsigned middle_load_sizes[] = {
        sizeof(uint16_t),
        sizeof(uint32_t),
        sizeof(uint8_t),
    };
    static const unsigned middle_load_offsets[] = { 6U, 4U, 7U };
    static const bool middle_load_signed[] = { false, true, false };
    static const unsigned final_store_sizes[] = {
        sizeof(uint32_t),
        sizeof(uint8_t),
        sizeof(uint16_t),
    };
    static const unsigned final_store_offsets[] = { 4U, 7U, 2U };
    static const uintptr_t scratch_offsets[] = { 0xac0U, 0xb00U, 0xb40U };
    static const uint64_t good_values[] = {
        0x8abc01234567def0ULL,
        0xf1234567a0b0c0d0ULL,
        0xfe66778899aabbccULL,
    };

    const enum test_page store_alias_page = AI_MICRO_MEMBLOCK_PBMT_IO_ALIAS_PAGE;
    const enum test_page load_alias_page = AI_MICRO_MEMBLOCK_PBMT_NC_ALIAS_PAGE;
    const uintptr_t bad_paddr_store = AI_MICRO_MEMBLOCK_PBMT_BAD_PADDR;

    for (unsigned round = 0; round < 3U; ++round) {
        const uintptr_t first_bad_vaddr =
            hs_page_base(store_alias_page) +
            ((bad_paddr_store + first_store_offsets[round]) &
                (PAGE_SIZE - 1U));
        const uintptr_t final_bad_vaddr =
            hs_page_base(store_alias_page) +
            ((bad_paddr_store + final_store_offsets[round]) &
                (PAGE_SIZE - 1U));
        const uintptr_t scratch_paddr =
            phys_page_base(SWITCH2) + scratch_offsets[round];
        const uintptr_t good_paddr = scratch_paddr;
        const uint64_t expected_middle =
            ai_micro_m_memblock_extract_scalar(good_values[round],
                middle_load_offsets[round], middle_load_sizes[round],
                middle_load_signed[round]);
        uintptr_t good_vaddr = 0U;

        goto_priv(PRIV_M);
        ai_micro_m_memblock_reset_env();
        ai_micro_m_memblock_reset_pbmt_env();

        ai_micro_m_memblock_prepare_pbmt_translation();
        TEST_ASSERT("PBMT setup store seeds the later good-load backing doubleword through the same uncache route instead of relying on cacheable-to-uncache visibility",
            ai_micro_m_memblock_seed_pbmt_alias_doubleword(
                load_alias_page, good_paddr, good_values[round])
        );
        (void)ai_micro_m_memblock_configure_pbmt_alias(store_alias_page,
            bad_paddr_store);

        TEST_SETUP_EXCEPT();
        ai_micro_m_memblock_translated_store_by_size(
            first_bad_vaddr, first_store_sizes[round], 0xa55aa55aa55aa55aULL);
        TEST_ASSERT("the first no-H mixed same-alias probe should report a precise IO bad-alias store fault at the first width-switched store shape",
            excpt.triggered == true &&
            excpt.cause == CAUSE_SAF &&
            excpt.tval == first_bad_vaddr
        );

        good_vaddr = ai_micro_m_memblock_configure_pbmt_alias(load_alias_page,
            good_paddr);

        TEST_SETUP_EXCEPT();
        uint64_t observed_middle = ai_micro_m_memblock_translated_scalar_load(
            middle_load_signed[round], middle_load_sizes[round],
            good_vaddr + middle_load_offsets[round]);
        TEST_ASSERT("after the first no-H mixed same-alias IO store fault, routing through an NC good load with a different decode should return the value selected by its current decode",
            excpt.triggered == false &&
            observed_middle == expected_middle
        );

        goto_priv(PRIV_M);
        TEST_ASSERT("the no-H mixed same-alias probe leaves the good backing doubleword unchanged before the final same-alias bad store is injected",
            read64(good_paddr) == good_values[round]
        );

        (void)ai_micro_m_memblock_configure_pbmt_alias(store_alias_page,
            bad_paddr_store);

        TEST_SETUP_EXCEPT();
        ai_micro_m_memblock_translated_store_by_size(
            final_bad_vaddr, final_store_sizes[round], 0x5aa55aa55aa55aa5ULL);
        TEST_ASSERT("after the middle good NC load with switched decode has already completed, returning to the same IO bad alias with a different store shape must refresh tval to the current bad alias instead of reusing stale metadata from the first IO bad episode",
            excpt.triggered == true &&
            excpt.cause == CAUSE_SAF &&
            excpt.tval == final_bad_vaddr
        );

        goto_priv(PRIV_M);
        TEST_ASSERT("the no-H mixed same-alias IO-store-good-NC-load-then-same-alias-bad-store probe leaves the good backing doubleword unchanged across both bad-alias episodes",
            read64(good_paddr) == good_values[round]
        );
    }

    ai_micro_m_memblock_reset_pbmt_env();
    ai_micro_m_memblock_reset_env();

    TEST_END();
}

bool ai_micro_m_load_pbmt_nc_same_alias_bad_good_store_pbmt_io_then_same_alias_bad_store_width_switch_refreshes_current_alias_corner()
{
    TEST_START();

    static const unsigned first_load_sizes[] = {
        sizeof(uint8_t),
        sizeof(uint16_t),
        sizeof(uint32_t),
    };
    static const unsigned first_load_offsets[] = { 7U, 6U, 4U };
    static const bool first_load_signed[] = { true, false, true };
    static const unsigned middle_store_sizes[] = {
        sizeof(uint16_t),
        sizeof(uint32_t),
        sizeof(uint8_t),
    };
    static const unsigned middle_store_offsets[] = { 2U, 4U, 7U };
    static const uint64_t middle_store_values[] = {
        0x000000000000e4d3ULL,
        0x00000000c5b4a697ULL,
        0x000000000000006fULL,
    };
    static const unsigned final_store_sizes[] = {
        sizeof(uint32_t),
        sizeof(uint8_t),
        sizeof(uint16_t),
    };
    static const unsigned final_store_offsets[] = { 4U, 7U, 2U };
    static const uint64_t final_store_values[] = {
        0x0000000011223344ULL,
        0x000000000000005aULL,
        0x000000000000bcadULL,
    };
    static const uintptr_t good_offsets[] = { 0xb80U, 0xbc0U, 0xc00U };
    static const uint64_t initial_goods[] = {
        0x8182838485868788ULL,
        0x9192939495969798ULL,
        0xa1a2a3a4a5a6a7a8ULL,
    };

    const enum test_page load_alias_page = AI_MICRO_MEMBLOCK_PBMT_NC_ALIAS_PAGE;
    const enum test_page store_alias_page = AI_MICRO_MEMBLOCK_PBMT_IO_ALIAS_PAGE;
    const uintptr_t bad_paddr = AI_MICRO_MEMBLOCK_PBMT_BAD_PADDR_ALT;

    for (unsigned round = 0; round < 3U; ++round) {
        const uintptr_t first_bad_vaddr =
            hs_page_base(load_alias_page) +
            ((bad_paddr + first_load_offsets[round]) & (PAGE_SIZE - 1U));
        const uintptr_t final_bad_vaddr =
            hs_page_base(store_alias_page) +
            ((bad_paddr + final_store_offsets[round]) & (PAGE_SIZE - 1U));
        const uintptr_t good_paddr =
            ai_micro_m_memblock_good_paddr_for_alias(
                store_alias_page, good_offsets[round]);
        const uint64_t expected_middle = ai_micro_m_memblock_merge_unsigned(
            initial_goods[round], middle_store_values[round],
            middle_store_offsets[round], middle_store_sizes[round]);
        uintptr_t good_vaddr = 0U;

        goto_priv(PRIV_M);
        ai_micro_m_memblock_reset_env();
        ai_micro_m_memblock_reset_pbmt_env();
        write64(good_paddr, initial_goods[round]);

        ai_micro_m_memblock_prepare_pbmt_translation();
        (void)ai_micro_m_memblock_configure_pbmt_alias(load_alias_page,
            bad_paddr);

        ai_micro_m_memblock_enable_mprv_supervisor_translation();
        TEST_SETUP_EXCEPT();
        (void)ai_micro_m_memblock_scalar_load_current_decode(
            first_load_signed[round], first_load_sizes[round], first_bad_vaddr);
        ai_micro_m_memblock_disable_mprv_supervisor_translation();
        TEST_ASSERT("the first no-H mixed producer-flip probe should report a precise NC bad-alias load fault at the first narrow load shape",
            excpt.triggered == true &&
            excpt.cause == CAUSE_LAF &&
            excpt.tval == first_bad_vaddr
        );

        good_vaddr = ai_micro_m_memblock_configure_pbmt_alias(store_alias_page,
            good_paddr);

        TEST_SETUP_EXCEPT();
        ai_micro_m_memblock_translated_store_by_size(
            good_vaddr + middle_store_offsets[round], middle_store_sizes[round],
            middle_store_values[round]);
        TEST_ASSERT("after the first no-H mixed same-alias NC load fault, routing through an IO good store with a different width should complete without inheriting stale exception state",
            excpt.triggered == false
        );

        goto_priv(PRIV_M);
        TEST_ASSERT("the no-H mixed producer-flip probe leaves the good backing doubleword with only the successful middle-store payload before the final same-alias bad store is injected",
            read64(good_paddr) == expected_middle
        );

        (void)ai_micro_m_memblock_configure_pbmt_alias(store_alias_page,
            bad_paddr);

        TEST_SETUP_EXCEPT();
        ai_micro_m_memblock_translated_store_by_size(
            final_bad_vaddr, final_store_sizes[round], final_store_values[round]);
        TEST_ASSERT("after the middle good IO store has already completed, returning to the same bad alias on the opposite producer side with a different switched store shape must refresh cause+tval to the current store-side bad alias instead of reusing stale metadata from the first NC load episode",
            excpt.triggered == true &&
            excpt.cause == CAUSE_SAF &&
            excpt.tval == final_bad_vaddr
        );

        goto_priv(PRIV_M);
        TEST_ASSERT("the no-H mixed NC-load-good-IO-store-then-same-alias-bad-store producer-flip probe keeps the good backing doubleword equal to the successful middle-store payload",
            read64(good_paddr) == expected_middle
        );
    }

    ai_micro_m_memblock_reset_pbmt_env();
    ai_micro_m_memblock_reset_env();

    TEST_END();
}

bool ai_micro_m_load_pbmt_nc_bad_good_store_pbmt_io_then_same_alias_bad_store_width_switch_refreshes_current_alias_corner()
{
    TEST_START();

    static const unsigned first_load_sizes[] = {
        sizeof(uint8_t),
        sizeof(uint16_t),
        sizeof(uint32_t),
    };
    static const unsigned first_load_offsets[] = { 7U, 6U, 4U };
    static const bool first_load_signed[] = { true, false, true };
    static const unsigned middle_store_sizes[] = {
        sizeof(uint16_t),
        sizeof(uint32_t),
        sizeof(uint8_t),
    };
    static const unsigned middle_store_offsets[] = { 2U, 4U, 7U };
    static const uint64_t middle_store_values[] = {
        0x000000000000ab98ULL,
        0x00000000cdef1357ULL,
        0x0000000000000076ULL,
    };
    static const unsigned final_store_sizes[] = {
        sizeof(uint32_t),
        sizeof(uint8_t),
        sizeof(uint16_t),
    };
    static const unsigned final_store_offsets[] = { 4U, 7U, 2U };
    static const uint64_t final_store_values[] = {
        0x0000000011223344ULL,
        0x000000000000005aULL,
        0x000000000000bcadULL,
    };
    static const uintptr_t good_offsets[] = { 0xd00U, 0xd40U, 0xd80U };
    static const uint64_t initial_goods[] = {
        0xc1c2c3c4c5c6c7c8ULL,
        0xd1d2d3d4d5d6d7d8ULL,
        0xe1e2e3e4e5e6e7e8ULL,
    };

    const enum test_page load_alias_page = AI_MICRO_MEMBLOCK_PBMT_NC_ALIAS_PAGE;
    const enum test_page store_alias_page = AI_MICRO_MEMBLOCK_PBMT_IO_ALIAS_PAGE;
    const uintptr_t bad_paddr = AI_MICRO_MEMBLOCK_PBMT_BAD_PADDR_ALT;

    for (unsigned round = 0; round < 3U; ++round) {
        const uintptr_t first_bad_vaddr =
            hs_page_base(load_alias_page) +
            ((bad_paddr + first_load_offsets[round]) &
                (PAGE_SIZE - 1U));
        const uintptr_t final_bad_vaddr =
            hs_page_base(store_alias_page) +
            ((bad_paddr + final_store_offsets[round]) &
                (PAGE_SIZE - 1U));
        const uintptr_t good_paddr =
            ai_micro_m_memblock_good_paddr_for_alias(
                store_alias_page, good_offsets[round]);
        const uint64_t expected_middle = ai_micro_m_memblock_merge_unsigned(
            initial_goods[round], middle_store_values[round],
            middle_store_offsets[round], middle_store_sizes[round]);
        uintptr_t good_vaddr = 0U;

        goto_priv(PRIV_M);
        ai_micro_m_memblock_reset_env();
        ai_micro_m_memblock_reset_pbmt_env();
        write64(good_paddr, initial_goods[round]);

        ai_micro_m_memblock_prepare_pbmt_translation();
        (void)ai_micro_m_memblock_configure_pbmt_alias(load_alias_page,
            bad_paddr);

        ai_micro_m_memblock_enable_mprv_supervisor_translation();
        TEST_SETUP_EXCEPT();
        (void)ai_micro_m_memblock_scalar_load_current_decode(
            first_load_signed[round], first_load_sizes[round], first_bad_vaddr);
        ai_micro_m_memblock_disable_mprv_supervisor_translation();
        TEST_ASSERT("the first no-H mixed producer-flip probe should report a precise NC bad-alias load fault at the first narrow load shape",
            excpt.triggered == true &&
            excpt.cause == CAUSE_LAF &&
            excpt.tval == first_bad_vaddr
        );

        good_vaddr = ai_micro_m_memblock_configure_pbmt_alias(store_alias_page,
            good_paddr);

        TEST_SETUP_EXCEPT();
        ai_micro_m_memblock_translated_store_by_size(
            good_vaddr + middle_store_offsets[round], middle_store_sizes[round],
            middle_store_values[round]);
        TEST_ASSERT("after the first no-H mixed NC bad-alias load fault, routing through an IO good store with a different width should complete without inheriting stale exception state",
            excpt.triggered == false
        );

        goto_priv(PRIV_M);
        TEST_ASSERT("the no-H mixed producer-flip probe leaves the good backing doubleword with only the successful middle-store payload before the final same-alias bad store is injected",
            read64(good_paddr) == expected_middle
        );

        (void)ai_micro_m_memblock_configure_pbmt_alias(store_alias_page,
            bad_paddr);

        TEST_SETUP_EXCEPT();
        ai_micro_m_memblock_translated_store_by_size(
            final_bad_vaddr, final_store_sizes[round], final_store_values[round]);
        TEST_ASSERT("after the middle good IO store has already completed, returning to the same bad alias on the opposite producer side with a different switched store shape must refresh cause+tval to the current store-side bad alias instead of reusing stale metadata from the first NC load episode",
            excpt.triggered == true &&
            excpt.cause == CAUSE_SAF &&
            excpt.tval == final_bad_vaddr
        );

        goto_priv(PRIV_M);
        TEST_ASSERT("the no-H mixed NC-load-good-IO-store-then-same-alias-bad-store producer-flip probe keeps the good backing doubleword equal to the successful middle-store payload",
            read64(good_paddr) == expected_middle
        );
    }

    ai_micro_m_memblock_reset_pbmt_env();
    ai_micro_m_memblock_reset_env();

    TEST_END();
}

bool ai_micro_m_store_pbmt_io_same_alias_bad_good_load_pbmt_nc_then_same_alias_bad_load_width_switch_refreshes_current_alias_corner()
{
    TEST_START();

    static const unsigned first_store_sizes[] = {
        sizeof(uint8_t),
        sizeof(uint16_t),
        sizeof(uint32_t),
    };
    static const unsigned first_store_offsets[] = { 7U, 2U, 4U };
    static const unsigned middle_load_sizes[] = {
        sizeof(uint16_t),
        sizeof(uint32_t),
        sizeof(uint8_t),
    };
    static const unsigned middle_load_offsets[] = { 6U, 4U, 7U };
    static const bool middle_load_signed[] = { false, true, false };
    static const unsigned final_load_sizes[] = {
        sizeof(uint32_t),
        sizeof(uint8_t),
        sizeof(uint16_t),
    };
    static const unsigned final_load_offsets[] = { 4U, 7U, 6U };
    static const bool final_load_signed[] = { true, false, true };
    static const uintptr_t scratch_offsets[] = { 0xdc0U, 0xe00U, 0xe40U };
    static const uint64_t good_values[] = {
        0x8abc01234567def0ULL,
        0xf1234567a0b0c0d0ULL,
        0xfe66778899aabbccULL,
    };

    const enum test_page store_alias_page = AI_MICRO_MEMBLOCK_PBMT_IO_ALIAS_PAGE;
    const enum test_page load_alias_page = AI_MICRO_MEMBLOCK_PBMT_NC_ALIAS_PAGE;
    const uintptr_t bad_paddr = AI_MICRO_MEMBLOCK_PBMT_BAD_PADDR;

    for (unsigned round = 0; round < 3U; ++round) {
        const uintptr_t first_bad_vaddr =
            hs_page_base(store_alias_page) +
            ((bad_paddr + first_store_offsets[round]) &
                (PAGE_SIZE - 1U));
        const uintptr_t final_bad_vaddr =
            hs_page_base(load_alias_page) +
            ((bad_paddr + final_load_offsets[round]) &
                (PAGE_SIZE - 1U));
        const uintptr_t scratch_paddr =
            phys_page_base(SWITCH2) + scratch_offsets[round];
        const uintptr_t good_paddr = scratch_paddr;
        const uint64_t expected_middle =
            ai_micro_m_memblock_extract_scalar(good_values[round],
                middle_load_offsets[round], middle_load_sizes[round],
                middle_load_signed[round]);
        uintptr_t good_vaddr = 0U;

        goto_priv(PRIV_M);
        ai_micro_m_memblock_reset_env();
        ai_micro_m_memblock_reset_pbmt_env();

        ai_micro_m_memblock_prepare_pbmt_translation();
        TEST_ASSERT("PBMT setup store seeds the later good-load backing doubleword through the same uncache route instead of relying on cacheable-to-uncache visibility",
            ai_micro_m_memblock_seed_pbmt_alias_doubleword(
                load_alias_page, good_paddr, good_values[round])
        );
        (void)ai_micro_m_memblock_configure_pbmt_alias(store_alias_page,
            bad_paddr);

        TEST_SETUP_EXCEPT();
        ai_micro_m_memblock_translated_store_by_size(
            first_bad_vaddr, first_store_sizes[round], 0xa55aa55aa55aa55aULL);
        TEST_ASSERT("the first no-H mixed producer-flip probe should report a precise IO bad-alias store fault at the first width-switched store shape",
            excpt.triggered == true &&
            excpt.cause == CAUSE_SAF &&
            excpt.tval == first_bad_vaddr
        );

        good_vaddr = ai_micro_m_memblock_configure_pbmt_alias(load_alias_page,
            good_paddr);

        TEST_SETUP_EXCEPT();
        uint64_t observed_middle = ai_micro_m_memblock_translated_scalar_load(
            middle_load_signed[round], middle_load_sizes[round],
            good_vaddr + middle_load_offsets[round]);
        TEST_ASSERT("after the first no-H mixed IO bad-alias store fault, routing through an NC good load with a different decode should return the value selected by its current decode",
            excpt.triggered == false &&
            observed_middle == expected_middle
        );

        goto_priv(PRIV_M);
        TEST_ASSERT("the no-H mixed producer-flip probe leaves the good backing doubleword unchanged before the final same-alias bad load is injected",
            read64(good_paddr) == good_values[round]
        );

        (void)ai_micro_m_memblock_configure_pbmt_alias(load_alias_page,
            bad_paddr);

        ai_micro_m_memblock_enable_mprv_supervisor_translation();
        TEST_SETUP_EXCEPT();
        (void)ai_micro_m_memblock_scalar_load_current_decode(
            final_load_signed[round], final_load_sizes[round], final_bad_vaddr);
        ai_micro_m_memblock_disable_mprv_supervisor_translation();
        TEST_ASSERT("after the middle good NC load has already completed, returning to the same bad alias on the opposite producer side with a different switched load shape must refresh cause+tval to the current load-side bad alias instead of reusing stale metadata from the first IO store episode",
            excpt.triggered == true &&
            excpt.cause == CAUSE_LAF &&
            excpt.tval == final_bad_vaddr
        );

        goto_priv(PRIV_M);
        TEST_ASSERT("the no-H mixed IO-store-good-NC-load-then-same-alias-bad-load producer-flip probe leaves the good backing doubleword unchanged across both bad-alias episodes",
            read64(good_paddr) == good_values[round]
        );
    }

    ai_micro_m_memblock_reset_pbmt_env();
    ai_micro_m_memblock_reset_env();

    TEST_END();
}
