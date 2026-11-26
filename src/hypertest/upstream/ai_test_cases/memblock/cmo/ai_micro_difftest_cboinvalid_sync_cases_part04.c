#include "ai_micro_difftest_cboinvalid_sync_common.h"

bool ai_micro_hs_cbo_inval_prefetch_same_paddr_alias_refill_then_cbom_post_partial_store_no_stale_owner_or_byte_mask_drop_corner()
{
    TEST_START();

    const enum test_page base_page = SWITCH1;
    const enum test_page alias_page = SWITCH2;
    const uintptr_t target_base_vaddr = hs_page_base(base_page) + 0xb40U;
    const uintptr_t target_base_paddr = phys_page_base(base_page) + 0xb40U;
    const uintptr_t target_word_base_vaddr = target_base_vaddr;
    const uintptr_t prefetch_base_vaddr =
        target_base_vaddr + sizeof(uint64_t);
    const uintptr_t clean_base_vaddr =
        target_base_vaddr + 2U * sizeof(uint64_t);
    const uintptr_t neighbor_base_vaddr =
        target_base_vaddr + 5U * sizeof(uint64_t);
    const uintptr_t flush_base_vaddr =
        target_base_vaddr + (AI_MICRO_DIFFSYNC_BLOCK_WORDS - 1U) *
        sizeof(uint64_t);
    const uintptr_t before_guard_base_vaddr =
        target_base_vaddr - sizeof(uint64_t);
    const uintptr_t after_guard_base_vaddr =
        target_base_vaddr + 2U * AI_MICRO_DIFFSYNC_BLOCK_BYTES;
    const uintptr_t adjacent_base_vaddr =
        target_base_vaddr + AI_MICRO_DIFFSYNC_BLOCK_BYTES;
    const uintptr_t marker_base_vaddr =
        target_base_vaddr + 3U * AI_MICRO_DIFFSYNC_BLOCK_BYTES;
    const uintptr_t marker_guard_base_vaddr =
        marker_base_vaddr + sizeof(uint64_t);
    const uintptr_t before_guard_paddr =
        target_base_paddr - sizeof(uint64_t);
    const uintptr_t after_guard_paddr =
        target_base_paddr + 2U * AI_MICRO_DIFFSYNC_BLOCK_BYTES;
    const uintptr_t adjacent_base_paddr =
        target_base_paddr + AI_MICRO_DIFFSYNC_BLOCK_BYTES;
    const uintptr_t marker_paddr =
        target_base_paddr + 3U * AI_MICRO_DIFFSYNC_BLOCK_BYTES;
    const uintptr_t marker_guard_paddr =
        marker_paddr + sizeof(uint64_t);
    const uint64_t block_seeds[] = {
        0x61728394a5b6c7d8ULL,
        0x728394a5b6c7d8e9ULL,
        0x8394a5b6c7d8e9faULL,
    };
    const uint64_t adjacent_seeds[] = {
        0xfedcba9876543210ULL,
        0x0123456789abcdefULL,
        0x55aa7733cc11ee99ULL,
    };
    const uint8_t byte_updates[] = {
        0x91U,
        0x2cU,
        0xd7U,
    };
    const uint16_t halfword_updates[] = {
        0x6aa6U,
        0x5bb5U,
        0x4cc4U,
    };
    const uint64_t neighbor_updates[] = {
        0xa5a55a5af0f00f0fULL,
        0x0102030405060708ULL,
        0x8877665544332211ULL,
    };
    const uint64_t before_guards[] = {
        0xfeedfacecafebeefULL,
        0xdeadc0def00dbabeULL,
        0xfaceb00c13579bdfULL,
    };
    const uint64_t after_guards[] = {
        0x0badf00d5a5aa5a5ULL,
        0x1020304050607080ULL,
        0x1122334455667788ULL,
    };
    const uint64_t marker_seeds[] = {
        0xa0a1a2a3a4a5a6a7ULL,
        0xb0b1b2b3b4b5b6b7ULL,
        0xc0c1c2c3c4c5c6c7ULL,
    };
    const uint64_t marker_updates[] = {
        0x3141592653589793ULL,
        0x2718281828459045ULL,
        0x1123581321345589ULL,
    };
    const uint64_t marker_guards[] = {
        0xfdb97531eca86420ULL,
        0x13579bdf2468ace0ULL,
        0x02468ace13579bdfULL,
    };
    uintptr_t alias_page_base_vaddr = 0;
    uintptr_t target_alias_vaddr = 0;
    uintptr_t byte_store_alias_vaddr = 0;
    uintptr_t half_store_alias_vaddr = 0;
    uintptr_t inval_alias_vaddr = 0;
    uintptr_t clean_alias_vaddr = 0;
    uintptr_t neighbor_alias_vaddr = 0;
    uintptr_t flush_alias_vaddr = 0;
    uintptr_t before_guard_alias_vaddr = 0;
    uintptr_t after_guard_alias_vaddr = 0;
    uintptr_t adjacent_alias_vaddr = 0;
    uintptr_t marker_alias_vaddr = 0;
    uintptr_t marker_guard_alias_vaddr = 0;

    ai_micro_diffsync_prepare_hs_env(MENVCFG_CBIE01 | MENVCFG_CBZFE);
    alias_page_base_vaddr =
        ai_micro_diffsync_prepare_same_paddr_alias(alias_page, base_page);
    target_alias_vaddr = alias_page_base_vaddr + 0xb40U;
    byte_store_alias_vaddr = target_alias_vaddr + 1U;
    half_store_alias_vaddr = target_alias_vaddr + 4U;
    inval_alias_vaddr = target_alias_vaddr + 4U * sizeof(uint64_t);
    clean_alias_vaddr = target_alias_vaddr + 2U * sizeof(uint64_t);
    neighbor_alias_vaddr = target_alias_vaddr + 5U * sizeof(uint64_t);
    flush_alias_vaddr =
        target_alias_vaddr + (AI_MICRO_DIFFSYNC_BLOCK_WORDS - 1U) *
        sizeof(uint64_t);
    before_guard_alias_vaddr = target_alias_vaddr - sizeof(uint64_t);
    after_guard_alias_vaddr =
        target_alias_vaddr + 2U * AI_MICRO_DIFFSYNC_BLOCK_BYTES;
    adjacent_alias_vaddr =
        target_alias_vaddr + AI_MICRO_DIFFSYNC_BLOCK_BYTES;
    marker_alias_vaddr =
        target_alias_vaddr + 3U * AI_MICRO_DIFFSYNC_BLOCK_BYTES;
    marker_guard_alias_vaddr =
        marker_alias_vaddr + sizeof(uint64_t);

    for (size_t round = 0; round < sizeof(block_seeds) / sizeof(block_seeds[0]);
         ++round) {
        const uint64_t expected_merged =
            ai_micro_diffsync_merge_byte_halfword_into_word(
                block_seeds[round],
                byte_updates[round],
                halfword_updates[round]);
        const uint64_t expected_clean_seed =
            ai_micro_diffsync_seed_word(block_seeds[round], 2U);
        const uint64_t expected_neighbor_seed =
            ai_micro_diffsync_seed_word(block_seeds[round], 5U);
        const uint64_t expected_flush_seed =
            ai_micro_diffsync_seed_word(block_seeds[round],
                AI_MICRO_DIFFSYNC_BLOCK_WORDS - 1U);
        uint64_t target_observed = 0ULL;
        uint64_t neighbor_observed = 0ULL;
        uint64_t marker_observed = 0ULL;

        goto_priv(PRIV_M);
        ai_micro_diffsync_seed_block(target_base_paddr, block_seeds[round]);
        ai_micro_diffsync_seed_block(adjacent_base_paddr, adjacent_seeds[round]);
        write64(before_guard_paddr, before_guards[round]);
        write64(after_guard_paddr, after_guards[round]);
        write64(marker_paddr, marker_seeds[round]);
        write64(marker_guard_paddr, marker_guards[round]);

        goto_priv(PRIV_HS);
        TEST_SETUP_EXCEPT();
        AI_ASSERT_NO_EXCEPT_AND("before the same-paddr-alias cbo.inval + base-view prefetch_r + base/alias CBOM + alias-view post-CBOM partial-store handoff starts, both target views, clean/flush probe words, neighbor word, adjacent block, and marker are readable with seeded values",
            ld(target_word_base_vaddr) == block_seeds[round] &&
            ld(target_alias_vaddr) == block_seeds[round] &&
            ld(clean_base_vaddr) == expected_clean_seed &&
            ld(clean_alias_vaddr) == expected_clean_seed &&
            ld(neighbor_base_vaddr) == expected_neighbor_seed &&
            ld(neighbor_alias_vaddr) == expected_neighbor_seed &&
            ld(flush_base_vaddr) == expected_flush_seed &&
            ld(flush_alias_vaddr) == expected_flush_seed &&
            ld(adjacent_base_vaddr) == adjacent_seeds[round] &&
            ld(adjacent_alias_vaddr) == adjacent_seeds[round] &&
            ld(marker_base_vaddr) == marker_seeds[round] &&
            ld(marker_alias_vaddr) == marker_seeds[round]
        );

        TEST_SETUP_EXCEPT();
        ai_micro_diffsync_cbo_inval_prefetch_cbom_post_partial_store_window(
            inval_alias_vaddr, prefetch_base_vaddr, clean_base_vaddr,
            flush_alias_vaddr, target_alias_vaddr, byte_store_alias_vaddr,
            half_store_alias_vaddr, neighbor_base_vaddr, marker_base_vaddr,
            byte_updates[round], halfword_updates[round],
            neighbor_updates[round], marker_updates[round],
            &target_observed, &neighbor_observed, &marker_observed);
        AI_ASSERT_NO_EXCEPT_AND("cbo.inval through a same-paddr alias followed by base-view prefetch_r and base/alias CBOM must release ownership before alias-view post-CBOM byte/halfword stores so narrow masks, neighbor store, and marker survive",
            target_observed == expected_merged &&
            neighbor_observed == neighbor_updates[round] &&
            marker_observed == marker_updates[round] &&
            ld(target_word_base_vaddr) == expected_merged &&
            ld(target_alias_vaddr) == expected_merged &&
            ld(clean_base_vaddr) == expected_clean_seed &&
            ld(clean_alias_vaddr) == expected_clean_seed &&
            ld(neighbor_base_vaddr) == neighbor_updates[round] &&
            ld(neighbor_alias_vaddr) == neighbor_updates[round] &&
            ld(flush_base_vaddr) == expected_flush_seed &&
            ld(flush_alias_vaddr) == expected_flush_seed &&
            ld(marker_base_vaddr) == marker_updates[round] &&
            ld(marker_alias_vaddr) == marker_updates[round]
        );

        TEST_SETUP_EXCEPT();
        AI_ASSERT_NO_EXCEPT_AND("the same-paddr-alias refill plus CBOM/post-partial-store owner handoff preserves guards and the adjacent cache block through both views",
            ld(before_guard_base_vaddr) == before_guards[round] &&
            ld(before_guard_alias_vaddr) == before_guards[round] &&
            ld(after_guard_base_vaddr) == after_guards[round] &&
            ld(after_guard_alias_vaddr) == after_guards[round] &&
            ld(marker_guard_base_vaddr) == marker_guards[round] &&
            ld(marker_guard_alias_vaddr) == marker_guards[round] &&
            ld(adjacent_base_vaddr) == adjacent_seeds[round] &&
            ld(adjacent_alias_vaddr) == adjacent_seeds[round]
        );

        goto_priv(PRIV_M);
        TEST_ASSERT("backing memory after the same-paddr-alias cbo.inval + prefetch_r + CBOM/post-partial-store handoff contains only the narrow merged target word, post-CBOM neighbor store, marker update, and unchanged guards",
            ai_micro_diffsync_block_matches_seed_except_two_words(
                target_base_paddr,
                block_seeds[round],
                0U,
                expected_merged,
                5U,
                neighbor_updates[round]) &&
            ai_micro_diffsync_block_matches_seed(adjacent_base_paddr,
                adjacent_seeds[round]) &&
            read64(before_guard_paddr) == before_guards[round] &&
            read64(after_guard_paddr) == after_guards[round] &&
            read64(marker_paddr) == marker_updates[round] &&
            read64(marker_guard_paddr) == marker_guards[round]
        );
    }

    ai_micro_diffsync_reset_env(MENVCFG_CBIE01 | MENVCFG_CBZFE);

    TEST_END();
}

bool ai_micro_hs_cbo_inval_prefetch_refill_then_cbo_zero_cross16_store_no_stale_zero_or_split_payload_drop_corner()
{
    TEST_START();

    const enum test_page target_page = SWITCH1;
    const enum test_page marker_page = SWITCH2;
    const uintptr_t target_base_vaddr = hs_page_base(target_page) + 0x640U;
    const uintptr_t target_base_paddr = phys_page_base(target_page) + 0x640U;
    const uintptr_t cross16_store_vaddr = target_base_vaddr + 15U;
    const uintptr_t low_word_vaddr = target_base_vaddr + sizeof(uint64_t);
    const uintptr_t high_word_vaddr =
        target_base_vaddr + 2U * sizeof(uint64_t);
    const uintptr_t prefetch_vaddr = target_base_vaddr + sizeof(uint64_t);
    const uintptr_t zero_vaddr = target_base_vaddr + 4U * sizeof(uint64_t);
    const uintptr_t inval_vaddr = target_base_vaddr + 6U * sizeof(uint64_t);
    const uintptr_t neighbor_vaddr =
        target_base_vaddr + 5U * sizeof(uint64_t);
    const uintptr_t tail_vaddr =
        target_base_vaddr + (AI_MICRO_DIFFSYNC_BLOCK_WORDS - 1U) *
        sizeof(uint64_t);
    const uintptr_t before_guard_vaddr = target_base_vaddr - sizeof(uint64_t);
    const uintptr_t after_guard_vaddr =
        target_base_vaddr + 2U * AI_MICRO_DIFFSYNC_BLOCK_BYTES;
    const uintptr_t adjacent_base_vaddr =
        target_base_vaddr + AI_MICRO_DIFFSYNC_BLOCK_BYTES;
    const uintptr_t adjacent_base_paddr =
        target_base_paddr + AI_MICRO_DIFFSYNC_BLOCK_BYTES;
    const uintptr_t before_guard_paddr =
        target_base_paddr - sizeof(uint64_t);
    const uintptr_t after_guard_paddr =
        target_base_paddr + 2U * AI_MICRO_DIFFSYNC_BLOCK_BYTES;
    const uintptr_t marker_vaddr = hs_page_base(marker_page) + 0x640U;
    const uintptr_t marker_paddr = phys_page_base(marker_page) + 0x640U;
    const uintptr_t marker_guard_vaddr = marker_vaddr + sizeof(uint64_t);
    const uintptr_t marker_guard_paddr = marker_paddr + sizeof(uint64_t);
    const uint64_t block_seeds[] = {
        0x6a5b4c3d2e1f9081ULL,
        0x78695a4b3c2d1e0fULL,
        0x8877665544332211ULL,
    };
    const uint64_t adjacent_seeds[] = {
        0xfedcba9876543210ULL,
        0x0123456789abcdefULL,
        0x55aa7733cc11ee99ULL,
    };
    const uint64_t cross16_payloads[] = {
        0x1122334455667788ULL,
        0x99aabbccddeeff10ULL,
        0x0f1e2d3c4b5a6978ULL,
    };
    const uint64_t neighbor_updates[] = {
        0x8877665544332211ULL,
        0x7766554433221100ULL,
        0x66554433221100ffULL,
    };
    const uint64_t before_guards[] = {
        0xfeedfacecafebeefULL,
        0xdeadc0def00dbabeULL,
        0xfaceb00c13579bdfULL,
    };
    const uint64_t after_guards[] = {
        0x0badf00d5a5aa5a5ULL,
        0x1020304050607080ULL,
        0x1122334455667788ULL,
    };
    const uint64_t marker_seeds[] = {
        0xa0a1a2a3a4a5a6a7ULL,
        0xb0b1b2b3b4b5b6b7ULL,
        0xc0c1c2c3c4c5c6c7ULL,
    };
    const uint64_t marker_updates[] = {
        0x3141592653589793ULL,
        0x2718281828459045ULL,
        0x1123581321345589ULL,
    };
    const uint64_t marker_guards[] = {
        0xfdb97531eca86420ULL,
        0x13579bdf2468ace0ULL,
        0x02468ace13579bdfULL,
    };

    ai_micro_diffsync_prepare_hs_env(MENVCFG_CBIE01 | MENVCFG_CBZE);

    for (size_t round = 0; round < sizeof(block_seeds) / sizeof(block_seeds[0]);
         ++round) {
        const uint64_t expected_word1 =
            ai_micro_diffsync_cross16_low_word_from_seed(0ULL,
                cross16_payloads[round]);
        const uint64_t expected_word2 =
            ai_micro_diffsync_cross16_high_word_from_seed(0ULL,
                cross16_payloads[round]);
        const uint64_t seed_word1 =
            ai_micro_diffsync_seed_word(block_seeds[round], 1U);
        const uint64_t seed_word2 =
            ai_micro_diffsync_seed_word(block_seeds[round], 2U);
        const uint64_t expected_neighbor_seed =
            ai_micro_diffsync_seed_word(block_seeds[round], 5U);
        const uint64_t expected_tail =
            ai_micro_diffsync_seed_word(block_seeds[round],
                AI_MICRO_DIFFSYNC_BLOCK_WORDS - 1U);
        uint64_t low_observed = 0ULL;
        uint64_t high_observed = 0ULL;
        uint64_t neighbor_observed = 0ULL;
        uint64_t marker_observed = 0ULL;

        goto_priv(PRIV_M);
        ai_micro_diffsync_seed_block(target_base_paddr, block_seeds[round]);
        ai_micro_diffsync_seed_block(adjacent_base_paddr, adjacent_seeds[round]);
        write64(before_guard_paddr, before_guards[round]);
        write64(after_guard_paddr, after_guards[round]);
        write64(marker_paddr, marker_seeds[round]);
        write64(marker_guard_paddr, marker_guards[round]);

        goto_priv(PRIV_HS);
        TEST_SETUP_EXCEPT();
        AI_ASSERT_NO_EXCEPT_AND("before the same-block cbo.inval + prefetch_r + cbo.zero + cross-16B store handoff starts, target split words, zero probe word, neighbor word, adjacent block, and marker are readable with seeded values",
            ld(low_word_vaddr) == seed_word1 &&
            ld(high_word_vaddr) == seed_word2 &&
            ld(zero_vaddr) == ai_micro_diffsync_seed_word(block_seeds[round],
                4U) &&
            ld(neighbor_vaddr) == expected_neighbor_seed &&
            ld(tail_vaddr) == expected_tail &&
            ld(adjacent_base_vaddr) == adjacent_seeds[round] &&
            ld(marker_vaddr) == marker_seeds[round]
        );

        TEST_SETUP_EXCEPT();
        ai_micro_diffsync_cbo_inval_prefetch_zero_cross16_store_window(
            inval_vaddr, prefetch_vaddr, zero_vaddr, cross16_store_vaddr,
            low_word_vaddr, high_word_vaddr, neighbor_vaddr, marker_vaddr,
            cross16_payloads[round], neighbor_updates[round],
            marker_updates[round], &low_observed, &high_observed,
            &neighbor_observed, &marker_observed);
        AI_ASSERT_NO_EXCEPT_AND("cbo.inval plus prefetch_r must release refill ownership before cbo.zero hands a zeroed block to the cross-16B split store, so both split halves, neighbor store, and marker survive",
            low_observed == expected_word1 &&
            high_observed == expected_word2 &&
            neighbor_observed == neighbor_updates[round] &&
            marker_observed == marker_updates[round] &&
            ld(low_word_vaddr) == expected_word1 &&
            ld(high_word_vaddr) == expected_word2 &&
            ld(neighbor_vaddr) == neighbor_updates[round] &&
            ld(tail_vaddr) == 0ULL &&
            ld(marker_vaddr) == marker_updates[round]
        );

        TEST_SETUP_EXCEPT();
        AI_ASSERT_NO_EXCEPT_AND("the refill-to-zero-to-cross16-store owner handoff preserves target guards, marker guard, and the adjacent cache block",
            ld(before_guard_vaddr) == before_guards[round] &&
            ld(after_guard_vaddr) == after_guards[round] &&
            ld(marker_guard_vaddr) == marker_guards[round] &&
            ld(adjacent_base_vaddr) == adjacent_seeds[round]
        );

        goto_priv(PRIV_M);
        TEST_ASSERT("backing memory after the same-block cbo.inval + prefetch_r + cbo.zero + cross-16B store handoff contains only the split-store overlay, post-zero neighbor store, marker update, and unchanged guards",
            ai_micro_diffsync_block_all_zero_except_three_words(
                target_base_paddr,
                1U,
                expected_word1,
                2U,
                expected_word2,
                5U,
                neighbor_updates[round]) &&
            ai_micro_diffsync_block_matches_seed(adjacent_base_paddr,
                adjacent_seeds[round]) &&
            read64(before_guard_paddr) == before_guards[round] &&
            read64(after_guard_paddr) == after_guards[round] &&
            read64(marker_paddr) == marker_updates[round] &&
            read64(marker_guard_paddr) == marker_guards[round]
        );
    }

    ai_micro_diffsync_reset_env(MENVCFG_CBIE01 | MENVCFG_CBZE);

    TEST_END();
}

bool ai_micro_hs_cbo_inval_prefetch_refill_then_cross16_store_cbo_zero_no_stale_split_or_zero_drop_corner()
{
    TEST_START();

    const enum test_page target_page = SWITCH1;
    const enum test_page marker_page = SWITCH2;
    const uintptr_t target_base_vaddr = hs_page_base(target_page) + 0x6c0U;
    const uintptr_t target_base_paddr = phys_page_base(target_page) + 0x6c0U;
    const uintptr_t cross16_store_vaddr = target_base_vaddr + 15U;
    const uintptr_t low_word_vaddr = target_base_vaddr + sizeof(uint64_t);
    const uintptr_t high_word_vaddr =
        target_base_vaddr + 2U * sizeof(uint64_t);
    const uintptr_t prefetch_vaddr = target_base_vaddr + sizeof(uint64_t);
    const uintptr_t zero_vaddr = target_base_vaddr + 5U * sizeof(uint64_t);
    const uintptr_t inval_vaddr = target_base_vaddr + 6U * sizeof(uint64_t);
    const uintptr_t tail_vaddr =
        target_base_vaddr + (AI_MICRO_DIFFSYNC_BLOCK_WORDS - 1U) *
        sizeof(uint64_t);
    const uintptr_t before_guard_vaddr = target_base_vaddr - sizeof(uint64_t);
    const uintptr_t after_guard_vaddr =
        target_base_vaddr + 2U * AI_MICRO_DIFFSYNC_BLOCK_BYTES;
    const uintptr_t adjacent_base_vaddr =
        target_base_vaddr + AI_MICRO_DIFFSYNC_BLOCK_BYTES;
    const uintptr_t adjacent_base_paddr =
        target_base_paddr + AI_MICRO_DIFFSYNC_BLOCK_BYTES;
    const uintptr_t before_guard_paddr =
        target_base_paddr - sizeof(uint64_t);
    const uintptr_t after_guard_paddr =
        target_base_paddr + 2U * AI_MICRO_DIFFSYNC_BLOCK_BYTES;
    const uintptr_t marker_vaddr = hs_page_base(marker_page) + 0x6c0U;
    const uintptr_t marker_paddr = phys_page_base(marker_page) + 0x6c0U;
    const uintptr_t marker_guard_vaddr = marker_vaddr + sizeof(uint64_t);
    const uintptr_t marker_guard_paddr = marker_paddr + sizeof(uint64_t);
    const uint64_t block_seeds[] = {
        0x1f2e3d4c5b6a7988ULL,
        0x8071625344352617ULL,
        0x99aabbccddeeff00ULL,
    };
    const uint64_t adjacent_seeds[] = {
        0x0f1e2d3c4b5a6978ULL,
        0xfedcba9876543210ULL,
        0x55aa7733cc11ee99ULL,
    };
    const uint64_t cross16_payloads[] = {
        0x13579bdf2468ace0ULL,
        0x02468ace13579bdfULL,
        0xfdb97531eca86420ULL,
    };
    const uint64_t before_guards[] = {
        0xfeedfacecafebeefULL,
        0xdeadc0def00dbabeULL,
        0xfaceb00c13579bdfULL,
    };
    const uint64_t after_guards[] = {
        0xabcdef0123456789ULL,
        0x0123456789abcdefULL,
        0xf0e1d2c3b4a59687ULL,
    };
    const uint64_t marker_seeds[] = {
        0xa4a5a6a7a8a9aaabULL,
        0xb4b5b6b7b8b9babbULL,
        0xc4c5c6c7c8c9cacbULL,
    };
    const uint64_t marker_updates[] = {
        0x1122334455667788ULL,
        0x8877665544332211ULL,
        0x2468ace013579bdfULL,
    };
    const uint64_t marker_guards[] = {
        0x2222333344445555ULL,
        0x6666777788889999ULL,
        0xaaaabbbbccccddddULL,
    };

    ai_micro_diffsync_prepare_hs_env(MENVCFG_CBIE01 | MENVCFG_CBZE);

    for (size_t round = 0; round < sizeof(block_seeds) / sizeof(block_seeds[0]);
         ++round) {
        const uint64_t seed_word1 =
            ai_micro_diffsync_seed_word(block_seeds[round], 1U);
        const uint64_t seed_word2 =
            ai_micro_diffsync_seed_word(block_seeds[round], 2U);
        const uint64_t expected_word1 =
            ai_micro_diffsync_cross16_low_word_from_seed(seed_word1,
                cross16_payloads[round]);
        const uint64_t expected_word2 =
            ai_micro_diffsync_cross16_high_word_from_seed(seed_word2,
                cross16_payloads[round]);
        const uint64_t expected_tail =
            ai_micro_diffsync_seed_word(block_seeds[round],
                AI_MICRO_DIFFSYNC_BLOCK_WORDS - 1U);
        uint64_t low_before_zero = 0ULL;
        uint64_t high_before_zero = 0ULL;
        uint64_t marker_observed = 0ULL;

        goto_priv(PRIV_M);
        ai_micro_diffsync_seed_block(target_base_paddr, block_seeds[round]);
        ai_micro_diffsync_seed_block(adjacent_base_paddr, adjacent_seeds[round]);
        write64(before_guard_paddr, before_guards[round]);
        write64(after_guard_paddr, after_guards[round]);
        write64(marker_paddr, marker_seeds[round]);
        write64(marker_guard_paddr, marker_guards[round]);

        goto_priv(PRIV_HS);
        TEST_SETUP_EXCEPT();
        AI_ASSERT_NO_EXCEPT_AND("before the same-block cbo.inval + prefetch_r + cross-16B store + cbo.zero handoff starts, target split words, zero probe word, adjacent block, and marker are readable with seeded values",
            ld(low_word_vaddr) == seed_word1 &&
            ld(high_word_vaddr) == seed_word2 &&
            ld(zero_vaddr) == ai_micro_diffsync_seed_word(block_seeds[round],
                5U) &&
            ld(tail_vaddr) == expected_tail &&
            ld(adjacent_base_vaddr) == adjacent_seeds[round] &&
            ld(marker_vaddr) == marker_seeds[round]
        );

        TEST_SETUP_EXCEPT();
        ai_micro_diffsync_cbo_inval_prefetch_cross16_store_zero_window(
            inval_vaddr, prefetch_vaddr, cross16_store_vaddr,
            low_word_vaddr, high_word_vaddr, zero_vaddr, marker_vaddr,
            cross16_payloads[round], marker_updates[round],
            &low_before_zero, &high_before_zero, &marker_observed);
        AI_ASSERT_NO_EXCEPT_AND("cbo.inval plus prefetch_r must release refill ownership before the cross-16B split store, and the following cbo.zero must clear both split halves while allowing marker progress",
            low_before_zero == expected_word1 &&
            high_before_zero == expected_word2 &&
            marker_observed == marker_updates[round] &&
            ld(low_word_vaddr) == 0ULL &&
            ld(high_word_vaddr) == 0ULL &&
            ld(tail_vaddr) == 0ULL &&
            ld(marker_vaddr) == marker_updates[round]
        );

        TEST_SETUP_EXCEPT();
        AI_ASSERT_NO_EXCEPT_AND("the refill-to-cross16-store-to-zero owner handoff preserves target guards, marker guard, and the adjacent cache block",
            ld(before_guard_vaddr) == before_guards[round] &&
            ld(after_guard_vaddr) == after_guards[round] &&
            ld(marker_guard_vaddr) == marker_guards[round] &&
            ld(adjacent_base_vaddr) == adjacent_seeds[round]
        );

        goto_priv(PRIV_M);
        TEST_ASSERT("backing memory after the same-block cbo.inval + prefetch_r + cross-16B store + cbo.zero handoff contains a fully zeroed target block, marker update, and unchanged guards",
            ai_micro_diffsync_block_all_zero(target_base_paddr) &&
            ai_micro_diffsync_block_matches_seed(adjacent_base_paddr,
                adjacent_seeds[round]) &&
            read64(before_guard_paddr) == before_guards[round] &&
            read64(after_guard_paddr) == after_guards[round] &&
            read64(marker_paddr) == marker_updates[round] &&
            read64(marker_guard_paddr) == marker_guards[round]
        );
    }

    ai_micro_diffsync_reset_env(MENVCFG_CBIE01 | MENVCFG_CBZE);

    TEST_END();
}

bool ai_micro_hs_cbo_inval_prefetch_refill_then_cbo_zero_cross16_load_no_stale_zero_or_merge_payload_corner()
{
    TEST_START();

    const enum test_page target_page = SWITCH1;
    const enum test_page marker_page = SWITCH2;
    const uintptr_t target_base_vaddr = hs_page_base(target_page) + 0x740U;
    const uintptr_t target_base_paddr = phys_page_base(target_page) + 0x740U;
    const uintptr_t cross16_load_vaddr = target_base_vaddr + 15U;
    const uintptr_t low_word_vaddr = target_base_vaddr + sizeof(uint64_t);
    const uintptr_t high_word_vaddr =
        target_base_vaddr + 2U * sizeof(uint64_t);
    const uintptr_t prefetch_vaddr = target_base_vaddr + sizeof(uint64_t);
    const uintptr_t zero_vaddr = target_base_vaddr + 4U * sizeof(uint64_t);
    const uintptr_t inval_vaddr = target_base_vaddr + 6U * sizeof(uint64_t);
    const uintptr_t tail_vaddr =
        target_base_vaddr + (AI_MICRO_DIFFSYNC_BLOCK_WORDS - 1U) *
        sizeof(uint64_t);
    const uintptr_t before_guard_vaddr = target_base_vaddr - sizeof(uint64_t);
    const uintptr_t after_guard_vaddr =
        target_base_vaddr + 2U * AI_MICRO_DIFFSYNC_BLOCK_BYTES;
    const uintptr_t adjacent_base_vaddr =
        target_base_vaddr + AI_MICRO_DIFFSYNC_BLOCK_BYTES;
    const uintptr_t adjacent_base_paddr =
        target_base_paddr + AI_MICRO_DIFFSYNC_BLOCK_BYTES;
    const uintptr_t before_guard_paddr =
        target_base_paddr - sizeof(uint64_t);
    const uintptr_t after_guard_paddr =
        target_base_paddr + 2U * AI_MICRO_DIFFSYNC_BLOCK_BYTES;
    const uintptr_t marker_vaddr = hs_page_base(marker_page) + 0x740U;
    const uintptr_t marker_paddr = phys_page_base(marker_page) + 0x740U;
    const uintptr_t marker_guard_vaddr = marker_vaddr + sizeof(uint64_t);
    const uintptr_t marker_guard_paddr = marker_paddr + sizeof(uint64_t);
    const uint64_t block_seeds[] = {
        0x31425364758697a8ULL,
        0x425364758697a8b9ULL,
        0x5364758697a8b9caULL,
    };
    const uint64_t adjacent_seeds[] = {
        0xaabbccddeeff0011ULL,
        0x2233445566778899ULL,
        0x99aabbccddeeff00ULL,
    };
    const uint64_t before_guards[] = {
        0x0badc0de0badc0deULL,
        0x1234432112344321ULL,
        0xfedcba9876543210ULL,
    };
    const uint64_t after_guards[] = {
        0x0102030405060708ULL,
        0x8070605040302010ULL,
        0x1123581321345589ULL,
    };
    const uint64_t marker_seeds[] = {
        0xaaaabbbbccccddddULL,
        0xddddccccbbbbaaaaULL,
        0x55aa55aa33cc33ccULL,
    };
    const uint64_t marker_updates[] = {
        0xc0ffee00c0ffee00ULL,
        0x0ddc0ffe0ddc0ffeULL,
        0xfeedfacefeedfaceULL,
    };
    const uint64_t marker_guards[] = {
        0x1357246813572468ULL,
        0x2468135724681357ULL,
        0xabcdef0123456789ULL,
    };

    ai_micro_diffsync_prepare_hs_env(MENVCFG_CBIE01 | MENVCFG_CBZE);

    for (size_t round = 0; round < sizeof(block_seeds) / sizeof(block_seeds[0]);
         ++round) {
        const uint64_t seed_word1 =
            ai_micro_diffsync_seed_word(block_seeds[round], 1U);
        const uint64_t seed_word2 =
            ai_micro_diffsync_seed_word(block_seeds[round], 2U);
        const uint64_t expected_tail =
            ai_micro_diffsync_seed_word(block_seeds[round],
                AI_MICRO_DIFFSYNC_BLOCK_WORDS - 1U);
        uint64_t load_observed = 0xffffffffffffffffULL;
        uint64_t marker_observed = 0ULL;

        goto_priv(PRIV_M);
        ai_micro_diffsync_seed_block(target_base_paddr, block_seeds[round]);
        ai_micro_diffsync_seed_block(adjacent_base_paddr, adjacent_seeds[round]);
        write64(before_guard_paddr, before_guards[round]);
        write64(after_guard_paddr, after_guards[round]);
        write64(marker_paddr, marker_seeds[round]);
        write64(marker_guard_paddr, marker_guards[round]);

        goto_priv(PRIV_HS);
        TEST_SETUP_EXCEPT();
        AI_ASSERT_NO_EXCEPT_AND("before the same-block cbo.inval + prefetch_r + cbo.zero + cross-16B load handoff starts, target split words, zero probe word, adjacent block, and marker are readable with seeded values",
            ld(low_word_vaddr) == seed_word1 &&
            ld(high_word_vaddr) == seed_word2 &&
            ld(zero_vaddr) == ai_micro_diffsync_seed_word(block_seeds[round],
                4U) &&
            ld(tail_vaddr) == expected_tail &&
            ld(adjacent_base_vaddr) == adjacent_seeds[round] &&
            ld(marker_vaddr) == marker_seeds[round]
        );

        TEST_SETUP_EXCEPT();
        ai_micro_diffsync_cbo_inval_prefetch_zero_cross16_load_window(
            inval_vaddr, prefetch_vaddr, zero_vaddr, cross16_load_vaddr,
            marker_vaddr, marker_updates[round], &load_observed,
            &marker_observed);
        AI_ASSERT_NO_EXCEPT_AND("cbo.inval plus prefetch_r must release refill ownership before cbo.zero, and the following cross-16B load must merge only zeroed halves while marker progresses",
            load_observed == 0ULL &&
            marker_observed == marker_updates[round] &&
            ld(low_word_vaddr) == 0ULL &&
            ld(high_word_vaddr) == 0ULL &&
            ld(tail_vaddr) == 0ULL &&
            ld(marker_vaddr) == marker_updates[round]
        );

        TEST_SETUP_EXCEPT();
        AI_ASSERT_NO_EXCEPT_AND("the refill-to-zero-to-cross16-load owner handoff preserves target guards, marker guard, and the adjacent cache block",
            ld(before_guard_vaddr) == before_guards[round] &&
            ld(after_guard_vaddr) == after_guards[round] &&
            ld(marker_guard_vaddr) == marker_guards[round] &&
            ld(adjacent_base_vaddr) == adjacent_seeds[round]
        );

        goto_priv(PRIV_M);
        TEST_ASSERT("backing memory after the same-block cbo.inval + prefetch_r + cbo.zero + cross-16B load handoff contains a fully zeroed target block, marker update, and unchanged guards",
            ai_micro_diffsync_block_all_zero(target_base_paddr) &&
            ai_micro_diffsync_block_matches_seed(adjacent_base_paddr,
                adjacent_seeds[round]) &&
            read64(before_guard_paddr) == before_guards[round] &&
            read64(after_guard_paddr) == after_guards[round] &&
            read64(marker_paddr) == marker_updates[round] &&
            read64(marker_guard_paddr) == marker_guards[round]
        );
    }

    ai_micro_diffsync_reset_env(MENVCFG_CBIE01 | MENVCFG_CBZE);

    TEST_END();
}

bool ai_micro_hs_cbo_inval_prefetch_refill_then_cross16_load_cbo_zero_no_stale_merge_or_zero_drop_corner()
{
    TEST_START();

    const enum test_page target_page = SWITCH1;
    const enum test_page marker_page = SWITCH2;
    const uintptr_t target_base_vaddr = hs_page_base(target_page) + 0x7c0U;
    const uintptr_t target_base_paddr = phys_page_base(target_page) + 0x7c0U;
    const uintptr_t cross16_load_vaddr = target_base_vaddr + 15U;
    const uintptr_t low_word_vaddr = target_base_vaddr + sizeof(uint64_t);
    const uintptr_t high_word_vaddr =
        target_base_vaddr + 2U * sizeof(uint64_t);
    const uintptr_t prefetch_vaddr = target_base_vaddr + sizeof(uint64_t);
    const uintptr_t zero_vaddr = target_base_vaddr + 5U * sizeof(uint64_t);
    const uintptr_t inval_vaddr = target_base_vaddr + 6U * sizeof(uint64_t);
    const uintptr_t tail_vaddr =
        target_base_vaddr + (AI_MICRO_DIFFSYNC_BLOCK_WORDS - 1U) *
        sizeof(uint64_t);
    const uintptr_t before_guard_vaddr = target_base_vaddr - sizeof(uint64_t);
    const uintptr_t after_guard_vaddr =
        target_base_vaddr + 2U * AI_MICRO_DIFFSYNC_BLOCK_BYTES;
    const uintptr_t adjacent_base_vaddr =
        target_base_vaddr + AI_MICRO_DIFFSYNC_BLOCK_BYTES;
    const uintptr_t adjacent_base_paddr =
        target_base_paddr + AI_MICRO_DIFFSYNC_BLOCK_BYTES;
    const uintptr_t before_guard_paddr =
        target_base_paddr - sizeof(uint64_t);
    const uintptr_t after_guard_paddr =
        target_base_paddr + 2U * AI_MICRO_DIFFSYNC_BLOCK_BYTES;
    const uintptr_t marker_vaddr = hs_page_base(marker_page) + 0x7c0U;
    const uintptr_t marker_paddr = phys_page_base(marker_page) + 0x7c0U;
    const uintptr_t marker_guard_vaddr = marker_vaddr + sizeof(uint64_t);
    const uintptr_t marker_guard_paddr = marker_paddr + sizeof(uint64_t);
    const uint64_t block_seeds[] = {
        0x61728394a5b6c7d8ULL,
        0x89abcdef01234567ULL,
        0xf1e2d3c4b5a69788ULL,
    };
    const uint64_t adjacent_seeds[] = {
        0x0011223344556677ULL,
        0x8899aabbccddeeffULL,
        0x76543210fedcba98ULL,
    };
    const uint64_t before_guards[] = {
        0x55aa00ff55aa00ffULL,
        0xaa5500ffaa5500ffULL,
        0x0f0e0d0c0b0a0908ULL,
    };
    const uint64_t after_guards[] = {
        0x1021324354657687ULL,
        0x98badcfe10325476ULL,
        0x7766554433221100ULL,
    };
    const uint64_t marker_seeds[] = {
        0x1111222233334444ULL,
        0x5555666677778888ULL,
        0x9999aaaabbbbccccULL,
    };
    const uint64_t marker_updates[] = {
        0xabcdef0123456789ULL,
        0x0123456789abcdefULL,
        0x13579bdf2468ace0ULL,
    };
    const uint64_t marker_guards[] = {
        0x5a5aa5a55a5aa5a5ULL,
        0xc3c33c3cc3c33c3cULL,
        0xf0f00f0ff0f00f0fULL,
    };

    ai_micro_diffsync_prepare_hs_env(MENVCFG_CBIE01 | MENVCFG_CBZE);

    for (size_t round = 0; round < sizeof(block_seeds) / sizeof(block_seeds[0]);
         ++round) {
        const uint64_t seed_word1 =
            ai_micro_diffsync_seed_word(block_seeds[round], 1U);
        const uint64_t seed_word2 =
            ai_micro_diffsync_seed_word(block_seeds[round], 2U);
        const uint64_t expected_cross16 =
            ai_micro_diffsync_cross16_load_from_seed(seed_word1, seed_word2);
        const uint64_t expected_tail =
            ai_micro_diffsync_seed_word(block_seeds[round],
                AI_MICRO_DIFFSYNC_BLOCK_WORDS - 1U);
        uint64_t load_observed = 0ULL;
        uint64_t marker_observed = 0ULL;

        goto_priv(PRIV_M);
        ai_micro_diffsync_seed_block(target_base_paddr, block_seeds[round]);
        ai_micro_diffsync_seed_block(adjacent_base_paddr, adjacent_seeds[round]);
        write64(before_guard_paddr, before_guards[round]);
        write64(after_guard_paddr, after_guards[round]);
        write64(marker_paddr, marker_seeds[round]);
        write64(marker_guard_paddr, marker_guards[round]);

        goto_priv(PRIV_HS);
        TEST_SETUP_EXCEPT();
        AI_ASSERT_NO_EXCEPT_AND("before the same-block cbo.inval + prefetch_r + cross-16B load + cbo.zero handoff starts, target split words, zero probe word, adjacent block, and marker are readable with seeded values",
            ld(low_word_vaddr) == seed_word1 &&
            ld(high_word_vaddr) == seed_word2 &&
            ld(zero_vaddr) == ai_micro_diffsync_seed_word(block_seeds[round],
                5U) &&
            ld(tail_vaddr) == expected_tail &&
            ld(adjacent_base_vaddr) == adjacent_seeds[round] &&
            ld(marker_vaddr) == marker_seeds[round]
        );

        TEST_SETUP_EXCEPT();
        ai_micro_diffsync_cbo_inval_prefetch_cross16_load_zero_window(
            inval_vaddr, prefetch_vaddr, cross16_load_vaddr, zero_vaddr,
            marker_vaddr, marker_updates[round], &load_observed,
            &marker_observed);
        AI_ASSERT_NO_EXCEPT_AND("cbo.inval plus prefetch_r must release refill ownership before the cross-16B split load, and the following cbo.zero must clear both split halves while allowing marker progress",
            load_observed == expected_cross16 &&
            marker_observed == marker_updates[round] &&
            ld(low_word_vaddr) == 0ULL &&
            ld(high_word_vaddr) == 0ULL &&
            ld(tail_vaddr) == 0ULL &&
            ld(marker_vaddr) == marker_updates[round]
        );

        TEST_SETUP_EXCEPT();
        AI_ASSERT_NO_EXCEPT_AND("the refill-to-cross16-load-to-zero owner handoff preserves target guards, marker guard, and the adjacent cache block",
            ld(before_guard_vaddr) == before_guards[round] &&
            ld(after_guard_vaddr) == after_guards[round] &&
            ld(marker_guard_vaddr) == marker_guards[round] &&
            ld(adjacent_base_vaddr) == adjacent_seeds[round]
        );

        goto_priv(PRIV_M);
        TEST_ASSERT("backing memory after the same-block cbo.inval + prefetch_r + cross-16B load + cbo.zero handoff contains a fully zeroed target block, marker update, and unchanged guards",
            ai_micro_diffsync_block_all_zero(target_base_paddr) &&
            ai_micro_diffsync_block_matches_seed(adjacent_base_paddr,
                adjacent_seeds[round]) &&
            read64(before_guard_paddr) == before_guards[round] &&
            read64(after_guard_paddr) == after_guards[round] &&
            read64(marker_paddr) == marker_updates[round] &&
            read64(marker_guard_paddr) == marker_guards[round]
        );
    }

    ai_micro_diffsync_reset_env(MENVCFG_CBIE01 | MENVCFG_CBZE);

    TEST_END();
}

bool ai_micro_hs_cbo_inval_prefetch_refill_then_cbom_post_cross16_load_no_stale_owner_or_merge_payload_corner()
{
    TEST_START();

    const enum test_page target_page = SWITCH1;
    const enum test_page marker_page = SWITCH2;
    const uintptr_t target_base_vaddr = hs_page_base(target_page) + 0x840U;
    const uintptr_t target_base_paddr = phys_page_base(target_page) + 0x840U;
    const uintptr_t cross16_load_vaddr = target_base_vaddr + 15U;
    const uintptr_t word1_vaddr = target_base_vaddr + sizeof(uint64_t);
    const uintptr_t word2_vaddr =
        target_base_vaddr + 2U * sizeof(uint64_t);
    const uintptr_t prefetch_vaddr = target_base_vaddr + sizeof(uint64_t);
    const uintptr_t clean_vaddr = target_base_vaddr + 3U * sizeof(uint64_t);
    const uintptr_t inval_vaddr = target_base_vaddr + 4U * sizeof(uint64_t);
    const uintptr_t neighbor_vaddr =
        target_base_vaddr + 5U * sizeof(uint64_t);
    const uintptr_t flush_vaddr =
        target_base_vaddr + (AI_MICRO_DIFFSYNC_BLOCK_WORDS - 1U) *
        sizeof(uint64_t);
    const uintptr_t before_guard_vaddr = target_base_vaddr - sizeof(uint64_t);
    const uintptr_t after_guard_vaddr =
        target_base_vaddr + 2U * AI_MICRO_DIFFSYNC_BLOCK_BYTES;
    const uintptr_t adjacent_base_vaddr =
        target_base_vaddr + AI_MICRO_DIFFSYNC_BLOCK_BYTES;
    const uintptr_t adjacent_base_paddr =
        target_base_paddr + AI_MICRO_DIFFSYNC_BLOCK_BYTES;
    const uintptr_t before_guard_paddr =
        target_base_paddr - sizeof(uint64_t);
    const uintptr_t after_guard_paddr =
        target_base_paddr + 2U * AI_MICRO_DIFFSYNC_BLOCK_BYTES;
    const uintptr_t marker_vaddr = hs_page_base(marker_page) + 0x840U;
    const uintptr_t marker_paddr = phys_page_base(marker_page) + 0x840U;
    const uintptr_t marker_guard_vaddr = marker_vaddr + sizeof(uint64_t);
    const uintptr_t marker_guard_paddr = marker_paddr + sizeof(uint64_t);
    const uint64_t block_seeds[] = {
        0x708192a3b4c5d6e7ULL,
        0x8192a3b4c5d6e7f8ULL,
        0x92a3b4c5d6e7f809ULL,
    };
    const uint64_t adjacent_seeds[] = {
        0x0123456789abcdefULL,
        0xfedcba9876543210ULL,
        0x55aa7733cc11ee99ULL,
    };
    const uint64_t word1_updates[] = {
        0x8877665544332211ULL,
        0x0123456789abcdefULL,
        0x76543210fedcba98ULL,
    };
    const uint64_t word2_updates[] = {
        0x1021324354657687ULL,
        0xfdb97531eca86420ULL,
        0x99aabbccddeeff00ULL,
    };
    const uint64_t neighbor_updates[] = {
        0x2468ace013579bdfULL,
        0x13579bdf2468ace0ULL,
        0x0f1e2d3c4b5a6978ULL,
    };
    const uint64_t before_guards[] = {
        0xfeedfacecafebeefULL,
        0xdeadc0def00dbabeULL,
        0xfaceb00c13579bdfULL,
    };
    const uint64_t after_guards[] = {
        0x0badf00d5a5aa5a5ULL,
        0x1020304050607080ULL,
        0x1122334455667788ULL,
    };
    const uint64_t marker_seeds[] = {
        0xa0a1a2a3a4a5a6a7ULL,
        0xb0b1b2b3b4b5b6b7ULL,
        0xc0c1c2c3c4c5c6c7ULL,
    };
    const uint64_t marker_updates[] = {
        0x3141592653589793ULL,
        0x2718281828459045ULL,
        0x1123581321345589ULL,
    };
    const uint64_t marker_guards[] = {
        0xfdb97531eca86420ULL,
        0x13579bdf2468ace0ULL,
        0x02468ace13579bdfULL,
    };

    ai_micro_diffsync_prepare_hs_env(MENVCFG_CBIE01 | MENVCFG_CBZFE);

    for (size_t round = 0; round < sizeof(block_seeds) / sizeof(block_seeds[0]);
         ++round) {
        const uint64_t seed_word1 =
            ai_micro_diffsync_seed_word(block_seeds[round], 1U);
        const uint64_t seed_word2 =
            ai_micro_diffsync_seed_word(block_seeds[round], 2U);
        const uint64_t expected_clean_seed =
            ai_micro_diffsync_seed_word(block_seeds[round], 3U);
        const uint64_t expected_neighbor_seed =
            ai_micro_diffsync_seed_word(block_seeds[round], 5U);
        const uint64_t expected_flush_seed =
            ai_micro_diffsync_seed_word(block_seeds[round],
                AI_MICRO_DIFFSYNC_BLOCK_WORDS - 1U);
        const uint64_t expected_cross16 =
            ai_micro_diffsync_cross16_load_from_seed(word1_updates[round],
                word2_updates[round]);
        uint64_t load_observed = 0ULL;
        uint64_t word1_observed = 0ULL;
        uint64_t word2_observed = 0ULL;
        uint64_t neighbor_observed = 0ULL;
        uint64_t marker_observed = 0ULL;

        goto_priv(PRIV_M);
        ai_micro_diffsync_seed_block(target_base_paddr, block_seeds[round]);
        ai_micro_diffsync_seed_block(adjacent_base_paddr, adjacent_seeds[round]);
        write64(before_guard_paddr, before_guards[round]);
        write64(after_guard_paddr, after_guards[round]);
        write64(marker_paddr, marker_seeds[round]);
        write64(marker_guard_paddr, marker_guards[round]);

        goto_priv(PRIV_HS);
        TEST_SETUP_EXCEPT();
        AI_ASSERT_NO_EXCEPT_AND("before the same-block cbo.inval + prefetch_r + dirty split words + CBOM + post-CBOM cross-16B load handoff starts, target split words, clean/flush probes, neighbor word, adjacent block, and marker are readable with seeded values",
            ld(word1_vaddr) == seed_word1 &&
            ld(word2_vaddr) == seed_word2 &&
            ld(clean_vaddr) == expected_clean_seed &&
            ld(neighbor_vaddr) == expected_neighbor_seed &&
            ld(flush_vaddr) == expected_flush_seed &&
            ld(adjacent_base_vaddr) == adjacent_seeds[round] &&
            ld(marker_vaddr) == marker_seeds[round]
        );

        TEST_SETUP_EXCEPT();
        ai_micro_diffsync_cbo_inval_prefetch_cbom_post_cross16_load_window(
            inval_vaddr, prefetch_vaddr, word1_vaddr, word2_vaddr,
            clean_vaddr, flush_vaddr, cross16_load_vaddr, neighbor_vaddr,
            marker_vaddr, word1_updates[round], word2_updates[round],
            neighbor_updates[round], marker_updates[round], &load_observed,
            &word1_observed, &word2_observed, &neighbor_observed,
            &marker_observed);
        AI_ASSERT_NO_EXCEPT_AND("cbo.inval plus same-block prefetch_r must not let CBOM clean/flush drop dirty split words before the post-CBOM cross-16B load merges them and marker progresses",
            load_observed == expected_cross16 &&
            word1_observed == word1_updates[round] &&
            word2_observed == word2_updates[round] &&
            neighbor_observed == neighbor_updates[round] &&
            marker_observed == marker_updates[round] &&
            ld(word1_vaddr) == word1_updates[round] &&
            ld(word2_vaddr) == word2_updates[round] &&
            ld(clean_vaddr) == expected_clean_seed &&
            ld(neighbor_vaddr) == neighbor_updates[round] &&
            ld(flush_vaddr) == expected_flush_seed &&
            ld(marker_vaddr) == marker_updates[round]
        );

        TEST_SETUP_EXCEPT();
        AI_ASSERT_NO_EXCEPT_AND("the same-block refill plus CBOM/post-cross16-load owner handoff preserves target guards, marker guard, and the adjacent cache block",
            ld(before_guard_vaddr) == before_guards[round] &&
            ld(after_guard_vaddr) == after_guards[round] &&
            ld(marker_guard_vaddr) == marker_guards[round] &&
            ld(adjacent_base_vaddr) == adjacent_seeds[round]
        );

        goto_priv(PRIV_M);
        TEST_ASSERT("backing memory after the same-block cbo.inval + prefetch_r + CBOM/post-cross16-load handoff contains the dirty split words, post-CBOM neighbor store, marker update, and unchanged guards",
            ai_micro_diffsync_block_matches_seed_except_three_words(
                target_base_paddr,
                block_seeds[round],
                1U,
                word1_updates[round],
                2U,
                word2_updates[round],
                5U,
                neighbor_updates[round]) &&
            ai_micro_diffsync_block_matches_seed(adjacent_base_paddr,
                adjacent_seeds[round]) &&
            read64(before_guard_paddr) == before_guards[round] &&
            read64(after_guard_paddr) == after_guards[round] &&
            read64(marker_paddr) == marker_updates[round] &&
            read64(marker_guard_paddr) == marker_guards[round]
        );
    }

    ai_micro_diffsync_reset_env(MENVCFG_CBIE01 | MENVCFG_CBZFE);

    TEST_END();
}

bool ai_micro_hs_cbo_inval_prefetch_refill_then_cross16_load_cbom_no_stale_merge_or_cmo_owner_corner()
{
    TEST_START();

    const enum test_page target_page = SWITCH1;
    const enum test_page marker_page = SWITCH2;
    const uintptr_t target_base_vaddr = hs_page_base(target_page) + 0x8c0U;
    const uintptr_t target_base_paddr = phys_page_base(target_page) + 0x8c0U;
    const uintptr_t cross16_load_vaddr = target_base_vaddr + 15U;
    const uintptr_t word1_vaddr = target_base_vaddr + sizeof(uint64_t);
    const uintptr_t word2_vaddr =
        target_base_vaddr + 2U * sizeof(uint64_t);
    const uintptr_t prefetch_vaddr = target_base_vaddr + sizeof(uint64_t);
    const uintptr_t clean_vaddr = target_base_vaddr + 3U * sizeof(uint64_t);
    const uintptr_t inval_vaddr = target_base_vaddr + 4U * sizeof(uint64_t);
    const uintptr_t neighbor_vaddr =
        target_base_vaddr + 5U * sizeof(uint64_t);
    const uintptr_t flush_vaddr =
        target_base_vaddr + (AI_MICRO_DIFFSYNC_BLOCK_WORDS - 1U) *
        sizeof(uint64_t);
    const uintptr_t before_guard_vaddr = target_base_vaddr - sizeof(uint64_t);
    const uintptr_t after_guard_vaddr =
        target_base_vaddr + 2U * AI_MICRO_DIFFSYNC_BLOCK_BYTES;
    const uintptr_t adjacent_base_vaddr =
        target_base_vaddr + AI_MICRO_DIFFSYNC_BLOCK_BYTES;
    const uintptr_t adjacent_base_paddr =
        target_base_paddr + AI_MICRO_DIFFSYNC_BLOCK_BYTES;
    const uintptr_t before_guard_paddr =
        target_base_paddr - sizeof(uint64_t);
    const uintptr_t after_guard_paddr =
        target_base_paddr + 2U * AI_MICRO_DIFFSYNC_BLOCK_BYTES;
    const uintptr_t marker_vaddr = hs_page_base(marker_page) + 0x8c0U;
    const uintptr_t marker_paddr = phys_page_base(marker_page) + 0x8c0U;
    const uintptr_t marker_guard_vaddr = marker_vaddr + sizeof(uint64_t);
    const uintptr_t marker_guard_paddr = marker_paddr + sizeof(uint64_t);
    const uint64_t block_seeds[] = {
        0xa3b4c5d6e7f8091aULL,
        0xb4c5d6e7f8091a2bULL,
        0xc5d6e7f8091a2b3cULL,
    };
    const uint64_t adjacent_seeds[] = {
        0xfedcba9876543210ULL,
        0x0123456789abcdefULL,
        0x55aa7733cc11ee99ULL,
    };
    const uint64_t word1_updates[] = {
        0x0f1e2d3c4b5a6978ULL,
        0xf0e1d2c3b4a59687ULL,
        0x1122334455667788ULL,
    };
    const uint64_t word2_updates[] = {
        0x8877665544332211ULL,
        0x1029384756abcdefULL,
        0x99aabbccddeeff00ULL,
    };
    const uint64_t neighbor_updates[] = {
        0x3141592653589793ULL,
        0x2718281828459045ULL,
        0x1123581321345589ULL,
    };
    const uint64_t before_guards[] = {
        0xfeedfacecafebeefULL,
        0xdeadc0def00dbabeULL,
        0xfaceb00c13579bdfULL,
    };
    const uint64_t after_guards[] = {
        0x0badf00d5a5aa5a5ULL,
        0x1020304050607080ULL,
        0x1122334455667788ULL,
    };
    const uint64_t marker_seeds[] = {
        0xa0a1a2a3a4a5a6a7ULL,
        0xb0b1b2b3b4b5b6b7ULL,
        0xc0c1c2c3c4c5c6c7ULL,
    };
    const uint64_t marker_updates[] = {
        0x5a5aa5a5c3c33c3cULL,
        0x69966996d2d22d2dULL,
        0x78877887e1e11e1eULL,
    };
    const uint64_t marker_guards[] = {
        0xfdb97531eca86420ULL,
        0x13579bdf2468ace0ULL,
        0x02468ace13579bdfULL,
    };

    ai_micro_diffsync_prepare_hs_env(MENVCFG_CBIE01 | MENVCFG_CBZFE);

    for (size_t round = 0; round < sizeof(block_seeds) / sizeof(block_seeds[0]);
         ++round) {
        const uint64_t seed_word1 =
            ai_micro_diffsync_seed_word(block_seeds[round], 1U);
        const uint64_t seed_word2 =
            ai_micro_diffsync_seed_word(block_seeds[round], 2U);
        const uint64_t expected_clean_seed =
            ai_micro_diffsync_seed_word(block_seeds[round], 3U);
        const uint64_t expected_neighbor_seed =
            ai_micro_diffsync_seed_word(block_seeds[round], 5U);
        const uint64_t expected_flush_seed =
            ai_micro_diffsync_seed_word(block_seeds[round],
                AI_MICRO_DIFFSYNC_BLOCK_WORDS - 1U);
        const uint64_t expected_cross16 =
            ai_micro_diffsync_cross16_load_from_seed(word1_updates[round],
                word2_updates[round]);
        uint64_t load_observed = 0ULL;
        uint64_t word1_observed = 0ULL;
        uint64_t word2_observed = 0ULL;
        uint64_t neighbor_observed = 0ULL;
        uint64_t marker_observed = 0ULL;

        goto_priv(PRIV_M);
        ai_micro_diffsync_seed_block(target_base_paddr, block_seeds[round]);
        ai_micro_diffsync_seed_block(adjacent_base_paddr, adjacent_seeds[round]);
        write64(before_guard_paddr, before_guards[round]);
        write64(after_guard_paddr, after_guards[round]);
        write64(marker_paddr, marker_seeds[round]);
        write64(marker_guard_paddr, marker_guards[round]);

        goto_priv(PRIV_HS);
        TEST_SETUP_EXCEPT();
        AI_ASSERT_NO_EXCEPT_AND("before the same-block cbo.inval + prefetch_r + dirty split words + cross-16B load + CBOM handoff starts, target split words, clean/flush probes, neighbor word, adjacent block, and marker are readable with seeded values",
            ld(word1_vaddr) == seed_word1 &&
            ld(word2_vaddr) == seed_word2 &&
            ld(clean_vaddr) == expected_clean_seed &&
            ld(neighbor_vaddr) == expected_neighbor_seed &&
            ld(flush_vaddr) == expected_flush_seed &&
            ld(adjacent_base_vaddr) == adjacent_seeds[round] &&
            ld(marker_vaddr) == marker_seeds[round]
        );

        TEST_SETUP_EXCEPT();
        ai_micro_diffsync_cbo_inval_prefetch_cross16_load_cbom_window(
            inval_vaddr, prefetch_vaddr, word1_vaddr, word2_vaddr,
            cross16_load_vaddr, clean_vaddr, flush_vaddr, neighbor_vaddr,
            marker_vaddr, word1_updates[round], word2_updates[round],
            neighbor_updates[round], marker_updates[round], &load_observed,
            &word1_observed, &word2_observed, &neighbor_observed,
            &marker_observed);
        AI_ASSERT_NO_EXCEPT_AND("cbo.inval plus same-block prefetch_r must not let the cross-16B load merge stale refill halves before CBOM clean/flush preserves the dirty split words and marker progresses",
            load_observed == expected_cross16 &&
            word1_observed == word1_updates[round] &&
            word2_observed == word2_updates[round] &&
            neighbor_observed == neighbor_updates[round] &&
            marker_observed == marker_updates[round] &&
            ld(word1_vaddr) == word1_updates[round] &&
            ld(word2_vaddr) == word2_updates[round] &&
            ld(clean_vaddr) == expected_clean_seed &&
            ld(neighbor_vaddr) == neighbor_updates[round] &&
            ld(flush_vaddr) == expected_flush_seed &&
            ld(marker_vaddr) == marker_updates[round]
        );

        TEST_SETUP_EXCEPT();
        AI_ASSERT_NO_EXCEPT_AND("the same-block refill plus cross16-load/CBOM owner handoff preserves target guards, marker guard, and the adjacent cache block",
            ld(before_guard_vaddr) == before_guards[round] &&
            ld(after_guard_vaddr) == after_guards[round] &&
            ld(marker_guard_vaddr) == marker_guards[round] &&
            ld(adjacent_base_vaddr) == adjacent_seeds[round]
        );

        goto_priv(PRIV_M);
        TEST_ASSERT("backing memory after the same-block cbo.inval + prefetch_r + cross16-load/CBOM handoff contains the dirty split words, post-CBOM neighbor store, marker update, and unchanged guards",
            ai_micro_diffsync_block_matches_seed_except_three_words(
                target_base_paddr,
                block_seeds[round],
                1U,
                word1_updates[round],
                2U,
                word2_updates[round],
                5U,
                neighbor_updates[round]) &&
            ai_micro_diffsync_block_matches_seed(adjacent_base_paddr,
                adjacent_seeds[round]) &&
            read64(before_guard_paddr) == before_guards[round] &&
            read64(after_guard_paddr) == after_guards[round] &&
            read64(marker_paddr) == marker_updates[round] &&
            read64(marker_guard_paddr) == marker_guards[round]
        );
    }

    ai_micro_diffsync_reset_env(MENVCFG_CBIE01 | MENVCFG_CBZFE);

    TEST_END();
}

bool ai_micro_hs_cbo_inval_prefetch_refill_then_cbom_lrsc_no_stale_owner_or_reservation_loss_corner()
{
    TEST_START();

    const enum test_page target_page = SWITCH1;
    const enum test_page marker_page = SWITCH2;
    const uintptr_t target_base_vaddr = hs_page_base(target_page) + 0x940U;
    const uintptr_t target_base_paddr = phys_page_base(target_page) + 0x940U;
    const uintptr_t lrsc_vaddr = target_base_vaddr;
    const uintptr_t prefetch_vaddr = target_base_vaddr + sizeof(uint64_t);
    const uintptr_t clean_vaddr = target_base_vaddr + 3U * sizeof(uint64_t);
    const uintptr_t inval_vaddr = target_base_vaddr + 4U * sizeof(uint64_t);
    const uintptr_t neighbor_vaddr =
        target_base_vaddr + 5U * sizeof(uint64_t);
    const uintptr_t flush_vaddr =
        target_base_vaddr + (AI_MICRO_DIFFSYNC_BLOCK_WORDS - 1U) *
        sizeof(uint64_t);
    const uintptr_t before_guard_vaddr = target_base_vaddr - sizeof(uint64_t);
    const uintptr_t after_guard_vaddr =
        target_base_vaddr + 2U * AI_MICRO_DIFFSYNC_BLOCK_BYTES;
    const uintptr_t adjacent_base_vaddr =
        target_base_vaddr + AI_MICRO_DIFFSYNC_BLOCK_BYTES;
    const uintptr_t before_guard_paddr =
        target_base_paddr - sizeof(uint64_t);
    const uintptr_t after_guard_paddr =
        target_base_paddr + 2U * AI_MICRO_DIFFSYNC_BLOCK_BYTES;
    const uintptr_t adjacent_base_paddr =
        target_base_paddr + AI_MICRO_DIFFSYNC_BLOCK_BYTES;
    const uintptr_t marker_vaddr = hs_page_base(marker_page) + 0x940U;
    const uintptr_t marker_paddr = phys_page_base(marker_page) + 0x940U;
    const uintptr_t marker_guard_vaddr = marker_vaddr + sizeof(uint64_t);
    const uintptr_t marker_guard_paddr = marker_paddr + sizeof(uint64_t);
    const uint64_t block_seeds[] = {
        0x5a6b7c8d9eafb0c1ULL,
        0x6b7c8d9eafb0c1d2ULL,
        0x7c8d9eafb0c1d2e3ULL,
    };
    const uint64_t adjacent_seeds[] = {
        0xfedcba9876543210ULL,
        0x0123456789abcdefULL,
        0x55aa7733cc11ee99ULL,
    };
    const uint64_t pre_lrsc_updates[] = {
        0x1111222233334444ULL,
        0x5555666677778888ULL,
        0x9999aaaabbbbccccULL,
    };
    const uint64_t sc_payloads[] = {
        0x0102030405060708ULL,
        0x1827364554637281ULL,
        0xfedcba9876543210ULL,
    };
    const uint64_t neighbor_updates[] = {
        0x8877665544332211ULL,
        0x7766554433221100ULL,
        0x66554433221100ffULL,
    };
    const uint64_t before_guards[] = {
        0xfeedfacecafebeefULL,
        0xdeadc0def00dbabeULL,
        0xfaceb00c13579bdfULL,
    };
    const uint64_t after_guards[] = {
        0x0badf00d5a5aa5a5ULL,
        0x1020304050607080ULL,
        0x1122334455667788ULL,
    };
    const uint64_t marker_seeds[] = {
        0xa0a1a2a3a4a5a6a7ULL,
        0xb0b1b2b3b4b5b6b7ULL,
        0xc0c1c2c3c4c5c6c7ULL,
    };
    const uint64_t marker_updates[] = {
        0x3141592653589793ULL,
        0x2718281828459045ULL,
        0x1123581321345589ULL,
    };
    const uint64_t marker_guards[] = {
        0xfdb97531eca86420ULL,
        0x13579bdf2468ace0ULL,
        0x02468ace13579bdfULL,
    };

    ai_micro_diffsync_prepare_hs_env(MENVCFG_CBIE01 | MENVCFG_CBZFE);

    for (size_t round = 0; round < sizeof(block_seeds) / sizeof(block_seeds[0]);
         ++round) {
        const uint64_t expected_clean_seed =
            ai_micro_diffsync_seed_word(block_seeds[round], 3U);
        const uint64_t expected_neighbor_seed =
            ai_micro_diffsync_seed_word(block_seeds[round], 5U);
        const uint64_t expected_flush_seed =
            ai_micro_diffsync_seed_word(block_seeds[round],
                AI_MICRO_DIFFSYNC_BLOCK_WORDS - 1U);
        uint64_t lr_observed = 0xffffffffffffffffULL;
        uint64_t sc_status = 1ULL;
        uint64_t neighbor_observed = 0ULL;
        uint64_t marker_observed = 0ULL;

        goto_priv(PRIV_M);
        ai_micro_diffsync_seed_block(target_base_paddr, block_seeds[round]);
        ai_micro_diffsync_seed_block(adjacent_base_paddr, adjacent_seeds[round]);
        write64(before_guard_paddr, before_guards[round]);
        write64(after_guard_paddr, after_guards[round]);
        write64(marker_paddr, marker_seeds[round]);
        write64(marker_guard_paddr, marker_guards[round]);

        goto_priv(PRIV_HS);
        TEST_SETUP_EXCEPT();
        AI_ASSERT_NO_EXCEPT_AND("before the same-block cbo.inval + prefetch_r + dirty store + CBOM + LR/SC handoff starts, target word, clean/flush probes, neighbor word, adjacent block, and marker are readable with seeded values",
            ld(lrsc_vaddr) == block_seeds[round] &&
            ld(clean_vaddr) == expected_clean_seed &&
            ld(neighbor_vaddr) == expected_neighbor_seed &&
            ld(flush_vaddr) == expected_flush_seed &&
            ld(adjacent_base_vaddr) == adjacent_seeds[round] &&
            ld(marker_vaddr) == marker_seeds[round]
        );

        TEST_SETUP_EXCEPT();
        ai_micro_diffsync_cbo_inval_prefetch_cbom_lrsc_window(
            inval_vaddr, prefetch_vaddr, clean_vaddr, flush_vaddr,
            lrsc_vaddr, neighbor_vaddr, marker_vaddr,
            pre_lrsc_updates[round], sc_payloads[round],
            neighbor_updates[round], marker_updates[round], &lr_observed,
            &sc_status, &neighbor_observed, &marker_observed);
        AI_ASSERT_NO_EXCEPT_AND("cbo.inval plus same-block prefetch_r must not let CBOM clean/flush replay a stale refill image or keep ownership when LR/SC follows",
            lr_observed == pre_lrsc_updates[round] &&
            sc_status == 0ULL &&
            neighbor_observed == neighbor_updates[round] &&
            marker_observed == marker_updates[round] &&
            ld(lrsc_vaddr) == sc_payloads[round] &&
            ld(clean_vaddr) == expected_clean_seed &&
            ld(neighbor_vaddr) == neighbor_updates[round] &&
            ld(flush_vaddr) == expected_flush_seed &&
            ld(marker_vaddr) == marker_updates[round]
        );

        TEST_SETUP_EXCEPT();
        AI_ASSERT_NO_EXCEPT_AND("the same-block refill plus CBOM-to-LR/SC owner handoff preserves target guards, marker guard, and the adjacent cache block",
            ld(before_guard_vaddr) == before_guards[round] &&
            ld(after_guard_vaddr) == after_guards[round] &&
            ld(marker_guard_vaddr) == marker_guards[round] &&
            ld(adjacent_base_vaddr) == adjacent_seeds[round]
        );

        goto_priv(PRIV_M);
        TEST_ASSERT("backing memory after the same-block cbo.inval + prefetch_r + CBOM + LR/SC handoff contains only the successful SC payload, same-block neighbor store, marker update, and unchanged guards",
            ai_micro_diffsync_block_matches_seed_except_two_words(
                target_base_paddr,
                block_seeds[round],
                0U,
                sc_payloads[round],
                5U,
                neighbor_updates[round]) &&
            ai_micro_diffsync_block_matches_seed(adjacent_base_paddr,
                adjacent_seeds[round]) &&
            read64(before_guard_paddr) == before_guards[round] &&
            read64(after_guard_paddr) == after_guards[round] &&
            read64(marker_paddr) == marker_updates[round] &&
            read64(marker_guard_paddr) == marker_guards[round]
        );
    }

    ai_micro_diffsync_reset_env(MENVCFG_CBIE01 | MENVCFG_CBZFE);

    TEST_END();
}

bool ai_micro_hs_cbo_inval_prefetch_refill_then_lrsc_cbom_no_stale_sc_or_cmo_owner_drop_corner()
{
    TEST_START();

    const enum test_page target_page = SWITCH1;
    const enum test_page marker_page = SWITCH2;
    const uintptr_t target_base_vaddr = hs_page_base(target_page) + 0x9c0U;
    const uintptr_t target_base_paddr = phys_page_base(target_page) + 0x9c0U;
    const uintptr_t lrsc_vaddr = target_base_vaddr;
    const uintptr_t prefetch_vaddr = target_base_vaddr + sizeof(uint64_t);
    const uintptr_t clean_vaddr = target_base_vaddr + 3U * sizeof(uint64_t);
    const uintptr_t inval_vaddr = target_base_vaddr + 4U * sizeof(uint64_t);
    const uintptr_t neighbor_vaddr =
        target_base_vaddr + 5U * sizeof(uint64_t);
    const uintptr_t flush_vaddr =
        target_base_vaddr + (AI_MICRO_DIFFSYNC_BLOCK_WORDS - 1U) *
        sizeof(uint64_t);
    const uintptr_t before_guard_vaddr = target_base_vaddr - sizeof(uint64_t);
    const uintptr_t after_guard_vaddr =
        target_base_vaddr + 2U * AI_MICRO_DIFFSYNC_BLOCK_BYTES;
    const uintptr_t adjacent_base_vaddr =
        target_base_vaddr + AI_MICRO_DIFFSYNC_BLOCK_BYTES;
    const uintptr_t before_guard_paddr =
        target_base_paddr - sizeof(uint64_t);
    const uintptr_t after_guard_paddr =
        target_base_paddr + 2U * AI_MICRO_DIFFSYNC_BLOCK_BYTES;
    const uintptr_t adjacent_base_paddr =
        target_base_paddr + AI_MICRO_DIFFSYNC_BLOCK_BYTES;
    const uintptr_t marker_vaddr = hs_page_base(marker_page) + 0x9c0U;
    const uintptr_t marker_paddr = phys_page_base(marker_page) + 0x9c0U;
    const uintptr_t marker_guard_vaddr = marker_vaddr + sizeof(uint64_t);
    const uintptr_t marker_guard_paddr = marker_paddr + sizeof(uint64_t);
    const uint64_t block_seeds[] = {
        0x8d9eafb0c1d2e3f4ULL,
        0x9eafb0c1d2e3f405ULL,
        0xafb0c1d2e3f40516ULL,
    };
    const uint64_t adjacent_seeds[] = {
        0x0123456789abcdefULL,
        0xfedcba9876543210ULL,
        0x55aa7733cc11ee99ULL,
    };
    const uint64_t sc_payloads[] = {
        0x13579bdf2468ace0ULL,
        0x02468ace13579bdfULL,
        0xfdb97531eca86420ULL,
    };
    const uint64_t neighbor_updates[] = {
        0x3141592653589793ULL,
        0x2718281828459045ULL,
        0x1123581321345589ULL,
    };
    const uint64_t before_guards[] = {
        0xfeedfacecafebeefULL,
        0xdeadc0def00dbabeULL,
        0xfaceb00c13579bdfULL,
    };
    const uint64_t after_guards[] = {
        0x0badf00d5a5aa5a5ULL,
        0x1020304050607080ULL,
        0x1122334455667788ULL,
    };
    const uint64_t marker_seeds[] = {
        0xa0a1a2a3a4a5a6a7ULL,
        0xb0b1b2b3b4b5b6b7ULL,
        0xc0c1c2c3c4c5c6c7ULL,
    };
    const uint64_t marker_updates[] = {
        0x0102030405060708ULL,
        0x1827364554637281ULL,
        0xfedcba9876543210ULL,
    };
    const uint64_t marker_guards[] = {
        0xfdb97531eca86420ULL,
        0x13579bdf2468ace0ULL,
        0x02468ace13579bdfULL,
    };

    ai_micro_diffsync_prepare_hs_env(MENVCFG_CBIE01 | MENVCFG_CBZFE);

    for (size_t round = 0; round < sizeof(block_seeds) / sizeof(block_seeds[0]);
         ++round) {
        const uint64_t expected_clean_seed =
            ai_micro_diffsync_seed_word(block_seeds[round], 3U);
        const uint64_t expected_neighbor_seed =
            ai_micro_diffsync_seed_word(block_seeds[round], 5U);
        const uint64_t expected_flush_seed =
            ai_micro_diffsync_seed_word(block_seeds[round],
                AI_MICRO_DIFFSYNC_BLOCK_WORDS - 1U);
        uint64_t lr_observed = 0xffffffffffffffffULL;
        uint64_t sc_status = 1ULL;
        uint64_t target_observed = 0ULL;
        uint64_t neighbor_observed = 0ULL;
        uint64_t marker_observed = 0ULL;

        goto_priv(PRIV_M);
        ai_micro_diffsync_seed_block(target_base_paddr, block_seeds[round]);
        ai_micro_diffsync_seed_block(adjacent_base_paddr, adjacent_seeds[round]);
        write64(before_guard_paddr, before_guards[round]);
        write64(after_guard_paddr, after_guards[round]);
        write64(marker_paddr, marker_seeds[round]);
        write64(marker_guard_paddr, marker_guards[round]);

        goto_priv(PRIV_HS);
        TEST_SETUP_EXCEPT();
        AI_ASSERT_NO_EXCEPT_AND("before the same-block cbo.inval + prefetch_r + LR/SC + CBOM handoff starts, target word, clean/flush probes, neighbor word, adjacent block, and marker are readable with seeded values",
            ld(lrsc_vaddr) == block_seeds[round] &&
            ld(clean_vaddr) == expected_clean_seed &&
            ld(neighbor_vaddr) == expected_neighbor_seed &&
            ld(flush_vaddr) == expected_flush_seed &&
            ld(adjacent_base_vaddr) == adjacent_seeds[round] &&
            ld(marker_vaddr) == marker_seeds[round]
        );

        TEST_SETUP_EXCEPT();
        ai_micro_diffsync_cbo_inval_prefetch_lrsc_cbom_window(
            inval_vaddr, prefetch_vaddr, lrsc_vaddr, clean_vaddr,
            flush_vaddr, neighbor_vaddr, marker_vaddr, sc_payloads[round],
            neighbor_updates[round], marker_updates[round], &lr_observed,
            &sc_status, &target_observed, &neighbor_observed,
            &marker_observed);
        AI_ASSERT_NO_EXCEPT_AND("cbo.inval plus same-block prefetch_r must not let LR/SC observe stale refill data or let following CBOM clean/flush drop the successful SC payload",
            lr_observed == block_seeds[round] &&
            sc_status == 0ULL &&
            target_observed == sc_payloads[round] &&
            neighbor_observed == neighbor_updates[round] &&
            marker_observed == marker_updates[round] &&
            ld(lrsc_vaddr) == sc_payloads[round] &&
            ld(clean_vaddr) == expected_clean_seed &&
            ld(neighbor_vaddr) == neighbor_updates[round] &&
            ld(flush_vaddr) == expected_flush_seed &&
            ld(marker_vaddr) == marker_updates[round]
        );

        TEST_SETUP_EXCEPT();
        AI_ASSERT_NO_EXCEPT_AND("the same-block refill plus LR/SC-to-CBOM owner handoff preserves target guards, marker guard, and the adjacent cache block",
            ld(before_guard_vaddr) == before_guards[round] &&
            ld(after_guard_vaddr) == after_guards[round] &&
            ld(marker_guard_vaddr) == marker_guards[round] &&
            ld(adjacent_base_vaddr) == adjacent_seeds[round]
        );

        goto_priv(PRIV_M);
        TEST_ASSERT("backing memory after the same-block cbo.inval + prefetch_r + LR/SC + CBOM handoff contains the successful SC payload, post-CBOM neighbor store, marker update, and unchanged guards",
            ai_micro_diffsync_block_matches_seed_except_two_words(
                target_base_paddr,
                block_seeds[round],
                0U,
                sc_payloads[round],
                5U,
                neighbor_updates[round]) &&
            ai_micro_diffsync_block_matches_seed(adjacent_base_paddr,
                adjacent_seeds[round]) &&
            read64(before_guard_paddr) == before_guards[round] &&
            read64(after_guard_paddr) == after_guards[round] &&
            read64(marker_paddr) == marker_updates[round] &&
            read64(marker_guard_paddr) == marker_guards[round]
        );
    }

    ai_micro_diffsync_reset_env(MENVCFG_CBIE01 | MENVCFG_CBZFE);

    TEST_END();
}

bool ai_micro_hs_cbo_inval_prefetch_refill_then_cbom_amo_no_stale_owner_or_lost_update_corner()
{
    TEST_START();

    const enum test_page target_page = SWITCH1;
    const enum test_page marker_page = SWITCH2;
    const uintptr_t target_base_vaddr = hs_page_base(target_page) + 0xa40U;
    const uintptr_t target_base_paddr = phys_page_base(target_page) + 0xa40U;
    const uintptr_t target_word_vaddr = target_base_vaddr;
    const uintptr_t prefetch_vaddr = target_base_vaddr + sizeof(uint64_t);
    const uintptr_t clean_vaddr = target_base_vaddr + 3U * sizeof(uint64_t);
    const uintptr_t inval_vaddr = target_base_vaddr + 4U * sizeof(uint64_t);
    const uintptr_t neighbor_vaddr =
        target_base_vaddr + 5U * sizeof(uint64_t);
    const uintptr_t flush_vaddr =
        target_base_vaddr + (AI_MICRO_DIFFSYNC_BLOCK_WORDS - 1U) *
        sizeof(uint64_t);
    const uintptr_t before_guard_vaddr = target_base_vaddr - sizeof(uint64_t);
    const uintptr_t after_guard_vaddr =
        target_base_vaddr + 2U * AI_MICRO_DIFFSYNC_BLOCK_BYTES;
    const uintptr_t adjacent_base_vaddr =
        target_base_vaddr + AI_MICRO_DIFFSYNC_BLOCK_BYTES;
    const uintptr_t before_guard_paddr =
        target_base_paddr - sizeof(uint64_t);
    const uintptr_t after_guard_paddr =
        target_base_paddr + 2U * AI_MICRO_DIFFSYNC_BLOCK_BYTES;
    const uintptr_t adjacent_base_paddr =
        target_base_paddr + AI_MICRO_DIFFSYNC_BLOCK_BYTES;
    const uintptr_t marker_vaddr = hs_page_base(marker_page) + 0xa40U;
    const uintptr_t marker_paddr = phys_page_base(marker_page) + 0xa40U;
    const uintptr_t marker_guard_vaddr = marker_vaddr + sizeof(uint64_t);
    const uintptr_t marker_guard_paddr = marker_paddr + sizeof(uint64_t);
    const uint64_t block_seeds[] = {
        0xb0c1d2e3f4051627ULL,
        0xc1d2e3f405162738ULL,
        0xd2e3f40516273849ULL,
    };
    const uint64_t adjacent_seeds[] = {
        0xfedcba9876543210ULL,
        0x0123456789abcdefULL,
        0x55aa7733cc11ee99ULL,
    };
    const uint64_t pre_amo_updates[] = {
        0x1111222233334444ULL,
        0x5555666677778888ULL,
        0x9999aaaabbbbccccULL,
    };
    const uint64_t first_addends[] = {
        0x0000000000000101ULL,
        0x0000000000000202ULL,
        0x0000000000000303ULL,
    };
    const uint64_t second_addends[] = {
        0x0001000100010001ULL,
        0x0002000200020002ULL,
        0x0003000300030003ULL,
    };
    const uint64_t neighbor_updates[] = {
        0x8877665544332211ULL,
        0x7766554433221100ULL,
        0x66554433221100ffULL,
    };
    const uint64_t before_guards[] = {
        0xfeedfacecafebeefULL,
        0xdeadc0def00dbabeULL,
        0xfaceb00c13579bdfULL,
    };
    const uint64_t after_guards[] = {
        0x0badf00d5a5aa5a5ULL,
        0x1020304050607080ULL,
        0x1122334455667788ULL,
    };
    const uint64_t marker_seeds[] = {
        0xa0a1a2a3a4a5a6a7ULL,
        0xb0b1b2b3b4b5b6b7ULL,
        0xc0c1c2c3c4c5c6c7ULL,
    };
    const uint64_t marker_updates[] = {
        0x3141592653589793ULL,
        0x2718281828459045ULL,
        0x1123581321345589ULL,
    };
    const uint64_t marker_guards[] = {
        0xfdb97531eca86420ULL,
        0x13579bdf2468ace0ULL,
        0x02468ace13579bdfULL,
    };

    ai_micro_diffsync_prepare_hs_env(MENVCFG_CBIE01 | MENVCFG_CBZFE);

    for (size_t round = 0; round < sizeof(block_seeds) / sizeof(block_seeds[0]);
         ++round) {
        const uint64_t expected_clean_seed =
            ai_micro_diffsync_seed_word(block_seeds[round], 3U);
        const uint64_t expected_neighbor_seed =
            ai_micro_diffsync_seed_word(block_seeds[round], 5U);
        const uint64_t expected_flush_seed =
            ai_micro_diffsync_seed_word(block_seeds[round],
                AI_MICRO_DIFFSYNC_BLOCK_WORDS - 1U);
        const uint64_t after_first_amo =
            pre_amo_updates[round] + first_addends[round];
        const uint64_t final_target =
            after_first_amo + second_addends[round];
        uint64_t first_amo_old = 0xffffffffffffffffULL;
        uint64_t second_amo_old = 0xffffffffffffffffULL;
        uint64_t target_observed = 0ULL;
        uint64_t neighbor_observed = 0ULL;
        uint64_t marker_observed = 0ULL;

        goto_priv(PRIV_M);
        ai_micro_diffsync_seed_block(target_base_paddr, block_seeds[round]);
        ai_micro_diffsync_seed_block(adjacent_base_paddr, adjacent_seeds[round]);
        write64(before_guard_paddr, before_guards[round]);
        write64(after_guard_paddr, after_guards[round]);
        write64(marker_paddr, marker_seeds[round]);
        write64(marker_guard_paddr, marker_guards[round]);

        goto_priv(PRIV_HS);
        TEST_SETUP_EXCEPT();
        AI_ASSERT_NO_EXCEPT_AND("before the same-block cbo.inval + prefetch_r + dirty store + CBOM + AMO handoff starts, target word, clean/flush probes, neighbor word, adjacent block, and marker are readable with seeded values",
            ld(target_word_vaddr) == block_seeds[round] &&
            ld(clean_vaddr) == expected_clean_seed &&
            ld(neighbor_vaddr) == expected_neighbor_seed &&
            ld(flush_vaddr) == expected_flush_seed &&
            ld(adjacent_base_vaddr) == adjacent_seeds[round] &&
            ld(marker_vaddr) == marker_seeds[round]
        );

        TEST_SETUP_EXCEPT();
        ai_micro_diffsync_cbo_inval_prefetch_cbom_amo_window(
            inval_vaddr, prefetch_vaddr, target_word_vaddr, clean_vaddr,
            flush_vaddr, neighbor_vaddr, marker_vaddr,
            pre_amo_updates[round], first_addends[round],
            second_addends[round], neighbor_updates[round],
            marker_updates[round], &first_amo_old, &second_amo_old,
            &target_observed, &neighbor_observed, &marker_observed);
        AI_ASSERT_NO_EXCEPT_AND("cbo.inval plus same-block prefetch_r must not let CBOM clean/flush replay a stale refill image before AMO consumes and updates the current dirty target word",
            first_amo_old == pre_amo_updates[round] &&
            second_amo_old == after_first_amo &&
            target_observed == final_target &&
            neighbor_observed == neighbor_updates[round] &&
            marker_observed == marker_updates[round] &&
            ld(target_word_vaddr) == final_target &&
            ld(clean_vaddr) == expected_clean_seed &&
            ld(neighbor_vaddr) == neighbor_updates[round] &&
            ld(flush_vaddr) == expected_flush_seed &&
            ld(marker_vaddr) == marker_updates[round]
        );

        TEST_SETUP_EXCEPT();
        AI_ASSERT_NO_EXCEPT_AND("the same-block refill plus CBOM-to-AMO owner handoff preserves target guards, marker guard, and the adjacent cache block",
            ld(before_guard_vaddr) == before_guards[round] &&
            ld(after_guard_vaddr) == after_guards[round] &&
            ld(marker_guard_vaddr) == marker_guards[round] &&
            ld(adjacent_base_vaddr) == adjacent_seeds[round]
        );

        goto_priv(PRIV_M);
        TEST_ASSERT("backing memory after the same-block cbo.inval + prefetch_r + CBOM + AMO handoff contains only the final AMO target value, same-block neighbor store, marker update, and unchanged guards",
            ai_micro_diffsync_block_matches_seed_except_two_words(
                target_base_paddr,
                block_seeds[round],
                0U,
                final_target,
                5U,
                neighbor_updates[round]) &&
            ai_micro_diffsync_block_matches_seed(adjacent_base_paddr,
                adjacent_seeds[round]) &&
            read64(before_guard_paddr) == before_guards[round] &&
            read64(after_guard_paddr) == after_guards[round] &&
            read64(marker_paddr) == marker_updates[round] &&
            read64(marker_guard_paddr) == marker_guards[round]
        );
    }

    ai_micro_diffsync_reset_env(MENVCFG_CBIE01 | MENVCFG_CBZFE);

    TEST_END();
}

bool ai_micro_hs_cbo_inval_prefetch_refill_then_amo_cbom_no_stale_amo_or_cmo_owner_drop_corner()
{
    TEST_START();

    const enum test_page target_page = SWITCH1;
    const enum test_page marker_page = SWITCH2;
    const uintptr_t target_base_vaddr = hs_page_base(target_page) + 0xac0U;
    const uintptr_t target_base_paddr = phys_page_base(target_page) + 0xac0U;
    const uintptr_t target_word_vaddr = target_base_vaddr;
    const uintptr_t prefetch_vaddr = target_base_vaddr + sizeof(uint64_t);
    const uintptr_t clean_vaddr = target_base_vaddr + 3U * sizeof(uint64_t);
    const uintptr_t inval_vaddr = target_base_vaddr + 4U * sizeof(uint64_t);
    const uintptr_t neighbor_vaddr =
        target_base_vaddr + 5U * sizeof(uint64_t);
    const uintptr_t flush_vaddr =
        target_base_vaddr + (AI_MICRO_DIFFSYNC_BLOCK_WORDS - 1U) *
        sizeof(uint64_t);
    const uintptr_t before_guard_vaddr = target_base_vaddr - sizeof(uint64_t);
    const uintptr_t after_guard_vaddr =
        target_base_vaddr + 2U * AI_MICRO_DIFFSYNC_BLOCK_BYTES;
    const uintptr_t adjacent_base_vaddr =
        target_base_vaddr + AI_MICRO_DIFFSYNC_BLOCK_BYTES;
    const uintptr_t before_guard_paddr =
        target_base_paddr - sizeof(uint64_t);
    const uintptr_t after_guard_paddr =
        target_base_paddr + 2U * AI_MICRO_DIFFSYNC_BLOCK_BYTES;
    const uintptr_t adjacent_base_paddr =
        target_base_paddr + AI_MICRO_DIFFSYNC_BLOCK_BYTES;
    const uintptr_t marker_vaddr = hs_page_base(marker_page) + 0xac0U;
    const uintptr_t marker_paddr = phys_page_base(marker_page) + 0xac0U;
    const uintptr_t marker_guard_vaddr = marker_vaddr + sizeof(uint64_t);
    const uintptr_t marker_guard_paddr = marker_paddr + sizeof(uint64_t);
    const uint64_t block_seeds[] = {
        0xe3f405162738495aULL,
        0xf405162738495a6bULL,
        0x05162738495a6b7cULL,
    };
    const uint64_t adjacent_seeds[] = {
        0x0123456789abcdefULL,
        0xfedcba9876543210ULL,
        0x55aa7733cc11ee99ULL,
    };
    const uint64_t first_addends[] = {
        0x0000000000000404ULL,
        0x0000000000000505ULL,
        0x0000000000000606ULL,
    };
    const uint64_t second_addends[] = {
        0x0004000400040004ULL,
        0x0005000500050005ULL,
        0x0006000600060006ULL,
    };
    const uint64_t neighbor_updates[] = {
        0x3141592653589793ULL,
        0x2718281828459045ULL,
        0x1123581321345589ULL,
    };
    const uint64_t before_guards[] = {
        0xfeedfacecafebeefULL,
        0xdeadc0def00dbabeULL,
        0xfaceb00c13579bdfULL,
    };
    const uint64_t after_guards[] = {
        0x0badf00d5a5aa5a5ULL,
        0x1020304050607080ULL,
        0x1122334455667788ULL,
    };
    const uint64_t marker_seeds[] = {
        0xa0a1a2a3a4a5a6a7ULL,
        0xb0b1b2b3b4b5b6b7ULL,
        0xc0c1c2c3c4c5c6c7ULL,
    };
    const uint64_t marker_updates[] = {
        0x0102030405060708ULL,
        0x1827364554637281ULL,
        0xfedcba9876543210ULL,
    };
    const uint64_t marker_guards[] = {
        0xfdb97531eca86420ULL,
        0x13579bdf2468ace0ULL,
        0x02468ace13579bdfULL,
    };

    ai_micro_diffsync_prepare_hs_env(MENVCFG_CBIE01 | MENVCFG_CBZFE);

    for (size_t round = 0; round < sizeof(block_seeds) / sizeof(block_seeds[0]);
         ++round) {
        const uint64_t expected_clean_seed =
            ai_micro_diffsync_seed_word(block_seeds[round], 3U);
        const uint64_t expected_neighbor_seed =
            ai_micro_diffsync_seed_word(block_seeds[round], 5U);
        const uint64_t expected_flush_seed =
            ai_micro_diffsync_seed_word(block_seeds[round],
                AI_MICRO_DIFFSYNC_BLOCK_WORDS - 1U);
        const uint64_t after_first_amo =
            block_seeds[round] + first_addends[round];
        const uint64_t final_target =
            after_first_amo + second_addends[round];
        uint64_t first_amo_old = 0xffffffffffffffffULL;
        uint64_t second_amo_old = 0xffffffffffffffffULL;
        uint64_t target_observed = 0ULL;
        uint64_t neighbor_observed = 0ULL;
        uint64_t marker_observed = 0ULL;

        goto_priv(PRIV_M);
        ai_micro_diffsync_seed_block(target_base_paddr, block_seeds[round]);
        ai_micro_diffsync_seed_block(adjacent_base_paddr, adjacent_seeds[round]);
        write64(before_guard_paddr, before_guards[round]);
        write64(after_guard_paddr, after_guards[round]);
        write64(marker_paddr, marker_seeds[round]);
        write64(marker_guard_paddr, marker_guards[round]);

        goto_priv(PRIV_HS);
        TEST_SETUP_EXCEPT();
        AI_ASSERT_NO_EXCEPT_AND("before the same-block cbo.inval + prefetch_r + AMO + CBOM handoff starts, target word, clean/flush probes, neighbor word, adjacent block, and marker are readable with seeded values",
            ld(target_word_vaddr) == block_seeds[round] &&
            ld(clean_vaddr) == expected_clean_seed &&
            ld(neighbor_vaddr) == expected_neighbor_seed &&
            ld(flush_vaddr) == expected_flush_seed &&
            ld(adjacent_base_vaddr) == adjacent_seeds[round] &&
            ld(marker_vaddr) == marker_seeds[round]
        );

        TEST_SETUP_EXCEPT();
        ai_micro_diffsync_cbo_inval_prefetch_amo_cbom_window(
            inval_vaddr, prefetch_vaddr, target_word_vaddr, clean_vaddr,
            flush_vaddr, neighbor_vaddr, marker_vaddr, first_addends[round],
            second_addends[round], neighbor_updates[round],
            marker_updates[round], &first_amo_old, &second_amo_old,
            &target_observed, &neighbor_observed, &marker_observed);
        AI_ASSERT_NO_EXCEPT_AND("cbo.inval plus same-block prefetch_r must not let AMO consume stale refill data or let following CBOM clean/flush drop the final AMO target value",
            first_amo_old == block_seeds[round] &&
            second_amo_old == after_first_amo &&
            target_observed == final_target &&
            neighbor_observed == neighbor_updates[round] &&
            marker_observed == marker_updates[round] &&
            ld(target_word_vaddr) == final_target &&
            ld(clean_vaddr) == expected_clean_seed &&
            ld(neighbor_vaddr) == neighbor_updates[round] &&
            ld(flush_vaddr) == expected_flush_seed &&
            ld(marker_vaddr) == marker_updates[round]
        );

        TEST_SETUP_EXCEPT();
        AI_ASSERT_NO_EXCEPT_AND("the same-block refill plus AMO-to-CBOM owner handoff preserves target guards, marker guard, and the adjacent cache block",
            ld(before_guard_vaddr) == before_guards[round] &&
            ld(after_guard_vaddr) == after_guards[round] &&
            ld(marker_guard_vaddr) == marker_guards[round] &&
            ld(adjacent_base_vaddr) == adjacent_seeds[round]
        );

        goto_priv(PRIV_M);
        TEST_ASSERT("backing memory after the same-block cbo.inval + prefetch_r + AMO + CBOM handoff contains the final AMO target value, post-CBOM neighbor store, marker update, and unchanged guards",
            ai_micro_diffsync_block_matches_seed_except_two_words(
                target_base_paddr,
                block_seeds[round],
                0U,
                final_target,
                5U,
                neighbor_updates[round]) &&
            ai_micro_diffsync_block_matches_seed(adjacent_base_paddr,
                adjacent_seeds[round]) &&
            read64(before_guard_paddr) == before_guards[round] &&
            read64(after_guard_paddr) == after_guards[round] &&
            read64(marker_paddr) == marker_updates[round] &&
            read64(marker_guard_paddr) == marker_guards[round]
        );
    }

    ai_micro_diffsync_reset_env(MENVCFG_CBIE01 | MENVCFG_CBZFE);

    TEST_END();
}

bool ai_micro_hs_cbo_inval_prefetch_refill_then_cbom_post_cross16_store_no_stale_owner_or_split_payload_drop_corner()
{
    TEST_START();

    const enum test_page target_page = SWITCH1;
    const enum test_page marker_page = SWITCH2;
    const uintptr_t target_base_vaddr = hs_page_base(target_page) + 0xa80U;
    const uintptr_t target_base_paddr = phys_page_base(target_page) + 0xa80U;
    const uintptr_t cross16_store_vaddr = target_base_vaddr + 15U;
    const uintptr_t low_word_vaddr = target_base_vaddr + sizeof(uint64_t);
    const uintptr_t high_word_vaddr =
        target_base_vaddr + 2U * sizeof(uint64_t);
    const uintptr_t prefetch_vaddr = target_base_vaddr + sizeof(uint64_t);
    const uintptr_t clean_vaddr = target_base_vaddr + 3U * sizeof(uint64_t);
    const uintptr_t inval_vaddr = target_base_vaddr + 4U * sizeof(uint64_t);
    const uintptr_t neighbor_vaddr =
        target_base_vaddr + 5U * sizeof(uint64_t);
    const uintptr_t flush_vaddr =
        target_base_vaddr + (AI_MICRO_DIFFSYNC_BLOCK_WORDS - 1U) *
        sizeof(uint64_t);
    const uintptr_t before_guard_vaddr = target_base_vaddr - sizeof(uint64_t);
    const uintptr_t after_guard_vaddr =
        target_base_vaddr + 2U * AI_MICRO_DIFFSYNC_BLOCK_BYTES;
    const uintptr_t adjacent_base_vaddr =
        target_base_vaddr + AI_MICRO_DIFFSYNC_BLOCK_BYTES;
    const uintptr_t adjacent_base_paddr =
        target_base_paddr + AI_MICRO_DIFFSYNC_BLOCK_BYTES;
    const uintptr_t before_guard_paddr =
        target_base_paddr - sizeof(uint64_t);
    const uintptr_t after_guard_paddr =
        target_base_paddr + 2U * AI_MICRO_DIFFSYNC_BLOCK_BYTES;
    const uintptr_t marker_vaddr = hs_page_base(marker_page) + 0xa80U;
    const uintptr_t marker_paddr = phys_page_base(marker_page) + 0xa80U;
    const uintptr_t marker_guard_vaddr = marker_vaddr + sizeof(uint64_t);
    const uintptr_t marker_guard_paddr = marker_paddr + sizeof(uint64_t);
    const uint64_t block_seeds[] = {
        0x1021324354657687ULL,
        0x2132435465768798ULL,
        0x31425364758697a8ULL,
    };
    const uint64_t adjacent_seeds[] = {
        0xfedcba9876543210ULL,
        0x0123456789abcdefULL,
        0x55aa7733cc11ee99ULL,
    };
    const uint64_t cross16_payloads[] = {
        0x1122334455667788ULL,
        0x99aabbccddeeff10ULL,
        0x0f1e2d3c4b5a6978ULL,
    };
    const uint64_t neighbor_updates[] = {
        0x8877665544332211ULL,
        0x7766554433221100ULL,
        0x66554433221100ffULL,
    };
    const uint64_t before_guards[] = {
        0xfeedfacecafebeefULL,
        0xdeadc0def00dbabeULL,
        0xfaceb00c13579bdfULL,
    };
    const uint64_t after_guards[] = {
        0x0badf00d5a5aa5a5ULL,
        0x1020304050607080ULL,
        0x1122334455667788ULL,
    };
    const uint64_t marker_seeds[] = {
        0xa0a1a2a3a4a5a6a7ULL,
        0xb0b1b2b3b4b5b6b7ULL,
        0xc0c1c2c3c4c5c6c7ULL,
    };
    const uint64_t marker_updates[] = {
        0x3141592653589793ULL,
        0x2718281828459045ULL,
        0x1123581321345589ULL,
    };
    const uint64_t marker_guards[] = {
        0xfdb97531eca86420ULL,
        0x13579bdf2468ace0ULL,
        0x02468ace13579bdfULL,
    };

    ai_micro_diffsync_prepare_hs_env(MENVCFG_CBIE01 | MENVCFG_CBZFE);

    for (size_t round = 0; round < sizeof(block_seeds) / sizeof(block_seeds[0]);
         ++round) {
        const uint64_t seed_word1 =
            ai_micro_diffsync_seed_word(block_seeds[round], 1U);
        const uint64_t seed_word2 =
            ai_micro_diffsync_seed_word(block_seeds[round], 2U);
        const uint64_t expected_word1 =
            ai_micro_diffsync_cross16_low_word_from_seed(seed_word1,
                cross16_payloads[round]);
        const uint64_t expected_word2 =
            ai_micro_diffsync_cross16_high_word_from_seed(seed_word2,
                cross16_payloads[round]);
        const uint64_t expected_clean_seed =
            ai_micro_diffsync_seed_word(block_seeds[round], 3U);
        const uint64_t expected_neighbor_seed =
            ai_micro_diffsync_seed_word(block_seeds[round], 5U);
        const uint64_t expected_flush_seed =
            ai_micro_diffsync_seed_word(block_seeds[round],
                AI_MICRO_DIFFSYNC_BLOCK_WORDS - 1U);
        uint64_t low_observed = 0ULL;
        uint64_t high_observed = 0ULL;
        uint64_t neighbor_observed = 0ULL;
        uint64_t marker_observed = 0ULL;

        goto_priv(PRIV_M);
        ai_micro_diffsync_seed_block(target_base_paddr, block_seeds[round]);
        ai_micro_diffsync_seed_block(adjacent_base_paddr, adjacent_seeds[round]);
        write64(before_guard_paddr, before_guards[round]);
        write64(after_guard_paddr, after_guards[round]);
        write64(marker_paddr, marker_seeds[round]);
        write64(marker_guard_paddr, marker_guards[round]);

        goto_priv(PRIV_HS);
        TEST_SETUP_EXCEPT();
        AI_ASSERT_NO_EXCEPT_AND("before the same-block cbo.inval + prefetch_r + CBOM + post-CBOM cross-16B store handoff starts, target split words, clean/flush probe words, neighbor word, adjacent block, and marker are readable with seeded values",
            ld(low_word_vaddr) == seed_word1 &&
            ld(high_word_vaddr) == seed_word2 &&
            ld(clean_vaddr) == expected_clean_seed &&
            ld(neighbor_vaddr) == expected_neighbor_seed &&
            ld(flush_vaddr) == expected_flush_seed &&
            ld(adjacent_base_vaddr) == adjacent_seeds[round] &&
            ld(marker_vaddr) == marker_seeds[round]
        );

        TEST_SETUP_EXCEPT();
        ai_micro_diffsync_cbo_inval_prefetch_cbom_post_cross16_store_window(
            inval_vaddr, prefetch_vaddr, clean_vaddr, flush_vaddr,
            cross16_store_vaddr, low_word_vaddr, high_word_vaddr,
            neighbor_vaddr, marker_vaddr, cross16_payloads[round],
            neighbor_updates[round], marker_updates[round],
            &low_observed, &high_observed, &neighbor_observed,
            &marker_observed);
        AI_ASSERT_NO_EXCEPT_AND("cbo.inval followed by same-block prefetch_r and CBOM clean/flush must release CMO ownership before the post-CBOM cross-16B split store so both split halves, neighbor store, and marker survive",
            low_observed == expected_word1 &&
            high_observed == expected_word2 &&
            neighbor_observed == neighbor_updates[round] &&
            marker_observed == marker_updates[round] &&
            ld(low_word_vaddr) == expected_word1 &&
            ld(high_word_vaddr) == expected_word2 &&
            ld(clean_vaddr) == expected_clean_seed &&
            ld(neighbor_vaddr) == neighbor_updates[round] &&
            ld(flush_vaddr) == expected_flush_seed &&
            ld(marker_vaddr) == marker_updates[round]
        );

        TEST_SETUP_EXCEPT();
        AI_ASSERT_NO_EXCEPT_AND("the same-block refill plus CBOM/post-cross16-store owner handoff preserves target guards, marker guard, and the adjacent cache block",
            ld(before_guard_vaddr) == before_guards[round] &&
            ld(after_guard_vaddr) == after_guards[round] &&
            ld(marker_guard_vaddr) == marker_guards[round] &&
            ld(adjacent_base_vaddr) == adjacent_seeds[round]
        );

        goto_priv(PRIV_M);
        TEST_ASSERT("backing memory after the same-block cbo.inval + prefetch_r + CBOM/post-cross16-store handoff contains only the precise split-store overlay, post-CBOM neighbor store, marker update, and unchanged guards",
            ai_micro_diffsync_block_matches_seed_except_three_words(
                target_base_paddr,
                block_seeds[round],
                1U,
                expected_word1,
                2U,
                expected_word2,
                5U,
                neighbor_updates[round]) &&
            ai_micro_diffsync_block_matches_seed(adjacent_base_paddr,
                adjacent_seeds[round]) &&
            read64(before_guard_paddr) == before_guards[round] &&
            read64(after_guard_paddr) == after_guards[round] &&
            read64(marker_paddr) == marker_updates[round] &&
            read64(marker_guard_paddr) == marker_guards[round]
        );
    }

    ai_micro_diffsync_reset_env(MENVCFG_CBIE01 | MENVCFG_CBZFE);

    TEST_END();
}

bool ai_micro_hs_cbo_inval_prefetch_same_paddr_alias_refill_then_cbom_post_cross16_store_no_stale_owner_or_split_payload_drop_corner()
{
    TEST_START();

    const enum test_page base_page = SWITCH1;
    const enum test_page alias_page = SWITCH2;
    const uintptr_t target_base_vaddr = hs_page_base(base_page) + 0xac0U;
    const uintptr_t target_base_paddr = phys_page_base(base_page) + 0xac0U;
    const uintptr_t low_word_base_vaddr =
        target_base_vaddr + sizeof(uint64_t);
    const uintptr_t high_word_base_vaddr =
        target_base_vaddr + 2U * sizeof(uint64_t);
    const uintptr_t prefetch_base_vaddr =
        target_base_vaddr + sizeof(uint64_t);
    const uintptr_t clean_base_vaddr =
        target_base_vaddr + 3U * sizeof(uint64_t);
    const uintptr_t neighbor_base_vaddr =
        target_base_vaddr + 5U * sizeof(uint64_t);
    const uintptr_t flush_base_vaddr =
        target_base_vaddr + (AI_MICRO_DIFFSYNC_BLOCK_WORDS - 1U) *
        sizeof(uint64_t);
    const uintptr_t before_guard_base_vaddr =
        target_base_vaddr - sizeof(uint64_t);
    const uintptr_t after_guard_base_vaddr =
        target_base_vaddr + 2U * AI_MICRO_DIFFSYNC_BLOCK_BYTES;
    const uintptr_t adjacent_base_vaddr =
        target_base_vaddr + AI_MICRO_DIFFSYNC_BLOCK_BYTES;
    const uintptr_t marker_base_vaddr =
        target_base_vaddr + 3U * AI_MICRO_DIFFSYNC_BLOCK_BYTES;
    const uintptr_t marker_guard_base_vaddr =
        marker_base_vaddr + sizeof(uint64_t);
    const uintptr_t before_guard_paddr =
        target_base_paddr - sizeof(uint64_t);
    const uintptr_t after_guard_paddr =
        target_base_paddr + 2U * AI_MICRO_DIFFSYNC_BLOCK_BYTES;
    const uintptr_t adjacent_base_paddr =
        target_base_paddr + AI_MICRO_DIFFSYNC_BLOCK_BYTES;
    const uintptr_t marker_paddr =
        target_base_paddr + 3U * AI_MICRO_DIFFSYNC_BLOCK_BYTES;
    const uintptr_t marker_guard_paddr =
        marker_paddr + sizeof(uint64_t);
    const uint64_t block_seeds[] = {
        0x4c5d6e7f8091a2b3ULL,
        0x5d6e7f8091a2b3c4ULL,
        0x6e7f8091a2b3c4d5ULL,
    };
    const uint64_t adjacent_seeds[] = {
        0xfedcba9876543210ULL,
        0x0123456789abcdefULL,
        0x55aa7733cc11ee99ULL,
    };
    const uint64_t cross16_payloads[] = {
        0x8877665544332211ULL,
        0x10ffeeddccbbaa99ULL,
        0x78695a4b3c2d1e0fULL,
    };
    const uint64_t neighbor_updates[] = {
        0xa5a55a5af0f00f0fULL,
        0x0102030405060708ULL,
        0x8877665544332211ULL,
    };
    const uint64_t before_guards[] = {
        0xfeedfacecafebeefULL,
        0xdeadc0def00dbabeULL,
        0xfaceb00c13579bdfULL,
    };
    const uint64_t after_guards[] = {
        0x0badf00d5a5aa5a5ULL,
        0x1020304050607080ULL,
        0x1122334455667788ULL,
    };
    const uint64_t marker_seeds[] = {
        0xa0a1a2a3a4a5a6a7ULL,
        0xb0b1b2b3b4b5b6b7ULL,
        0xc0c1c2c3c4c5c6c7ULL,
    };
    const uint64_t marker_updates[] = {
        0x3141592653589793ULL,
        0x2718281828459045ULL,
        0x1123581321345589ULL,
    };
    const uint64_t marker_guards[] = {
        0xfdb97531eca86420ULL,
        0x13579bdf2468ace0ULL,
        0x02468ace13579bdfULL,
    };
    uintptr_t alias_page_base_vaddr = 0;
    uintptr_t target_alias_vaddr = 0;
    uintptr_t cross16_store_alias_vaddr = 0;
    uintptr_t low_word_alias_vaddr = 0;
    uintptr_t high_word_alias_vaddr = 0;
    uintptr_t inval_alias_vaddr = 0;
    uintptr_t clean_alias_vaddr = 0;
    uintptr_t neighbor_alias_vaddr = 0;
    uintptr_t flush_alias_vaddr = 0;
    uintptr_t before_guard_alias_vaddr = 0;
    uintptr_t after_guard_alias_vaddr = 0;
    uintptr_t adjacent_alias_vaddr = 0;
    uintptr_t marker_alias_vaddr = 0;
    uintptr_t marker_guard_alias_vaddr = 0;

    ai_micro_diffsync_prepare_hs_env(MENVCFG_CBIE01 | MENVCFG_CBZFE);
    alias_page_base_vaddr =
        ai_micro_diffsync_prepare_same_paddr_alias(alias_page, base_page);
    target_alias_vaddr = alias_page_base_vaddr + 0xac0U;
    cross16_store_alias_vaddr = target_alias_vaddr + 15U;
    low_word_alias_vaddr = target_alias_vaddr + sizeof(uint64_t);
    high_word_alias_vaddr = target_alias_vaddr + 2U * sizeof(uint64_t);
    inval_alias_vaddr = target_alias_vaddr + 4U * sizeof(uint64_t);
    clean_alias_vaddr = target_alias_vaddr + 3U * sizeof(uint64_t);
    neighbor_alias_vaddr = target_alias_vaddr + 5U * sizeof(uint64_t);
    flush_alias_vaddr =
        target_alias_vaddr + (AI_MICRO_DIFFSYNC_BLOCK_WORDS - 1U) *
        sizeof(uint64_t);
    before_guard_alias_vaddr = target_alias_vaddr - sizeof(uint64_t);
    after_guard_alias_vaddr =
        target_alias_vaddr + 2U * AI_MICRO_DIFFSYNC_BLOCK_BYTES;
    adjacent_alias_vaddr =
        target_alias_vaddr + AI_MICRO_DIFFSYNC_BLOCK_BYTES;
    marker_alias_vaddr =
        target_alias_vaddr + 3U * AI_MICRO_DIFFSYNC_BLOCK_BYTES;
    marker_guard_alias_vaddr =
        marker_alias_vaddr + sizeof(uint64_t);

    for (size_t round = 0; round < sizeof(block_seeds) / sizeof(block_seeds[0]);
         ++round) {
        const uint64_t seed_word1 =
            ai_micro_diffsync_seed_word(block_seeds[round], 1U);
        const uint64_t seed_word2 =
            ai_micro_diffsync_seed_word(block_seeds[round], 2U);
        const uint64_t expected_word1 =
            ai_micro_diffsync_cross16_low_word_from_seed(seed_word1,
                cross16_payloads[round]);
        const uint64_t expected_word2 =
            ai_micro_diffsync_cross16_high_word_from_seed(seed_word2,
                cross16_payloads[round]);
        const uint64_t expected_clean_seed =
            ai_micro_diffsync_seed_word(block_seeds[round], 3U);
        const uint64_t expected_neighbor_seed =
            ai_micro_diffsync_seed_word(block_seeds[round], 5U);
        const uint64_t expected_flush_seed =
            ai_micro_diffsync_seed_word(block_seeds[round],
                AI_MICRO_DIFFSYNC_BLOCK_WORDS - 1U);
        uint64_t low_observed = 0ULL;
        uint64_t high_observed = 0ULL;
        uint64_t neighbor_observed = 0ULL;
        uint64_t marker_observed = 0ULL;

        goto_priv(PRIV_M);
        ai_micro_diffsync_seed_block(target_base_paddr, block_seeds[round]);
        ai_micro_diffsync_seed_block(adjacent_base_paddr, adjacent_seeds[round]);
        write64(before_guard_paddr, before_guards[round]);
        write64(after_guard_paddr, after_guards[round]);
        write64(marker_paddr, marker_seeds[round]);
        write64(marker_guard_paddr, marker_guards[round]);

        goto_priv(PRIV_HS);
        TEST_SETUP_EXCEPT();
        AI_ASSERT_NO_EXCEPT_AND("before the same-paddr-alias cbo.inval + base-view prefetch_r + base/alias CBOM + alias-view post-CBOM cross-16B store handoff starts, both target split-word views, clean/flush probes, neighbor, adjacent block, and marker are seeded",
            ld(low_word_base_vaddr) == seed_word1 &&
            ld(low_word_alias_vaddr) == seed_word1 &&
            ld(high_word_base_vaddr) == seed_word2 &&
            ld(high_word_alias_vaddr) == seed_word2 &&
            ld(clean_base_vaddr) == expected_clean_seed &&
            ld(clean_alias_vaddr) == expected_clean_seed &&
            ld(neighbor_base_vaddr) == expected_neighbor_seed &&
            ld(neighbor_alias_vaddr) == expected_neighbor_seed &&
            ld(flush_base_vaddr) == expected_flush_seed &&
            ld(flush_alias_vaddr) == expected_flush_seed &&
            ld(adjacent_base_vaddr) == adjacent_seeds[round] &&
            ld(adjacent_alias_vaddr) == adjacent_seeds[round] &&
            ld(marker_base_vaddr) == marker_seeds[round] &&
            ld(marker_alias_vaddr) == marker_seeds[round]
        );

        TEST_SETUP_EXCEPT();
        ai_micro_diffsync_cbo_inval_prefetch_cbom_post_cross16_store_window(
            inval_alias_vaddr, prefetch_base_vaddr, clean_base_vaddr,
            flush_alias_vaddr, cross16_store_alias_vaddr,
            low_word_alias_vaddr, high_word_alias_vaddr, neighbor_base_vaddr,
            marker_base_vaddr, cross16_payloads[round],
            neighbor_updates[round], marker_updates[round],
            &low_observed, &high_observed, &neighbor_observed,
            &marker_observed);
        AI_ASSERT_NO_EXCEPT_AND("cbo.inval through a same-paddr alias followed by base-view prefetch_r and base/alias CBOM must release ownership before alias-view post-CBOM cross-16B split store so both halves, neighbor, and marker survive",
            low_observed == expected_word1 &&
            high_observed == expected_word2 &&
            neighbor_observed == neighbor_updates[round] &&
            marker_observed == marker_updates[round] &&
            ld(low_word_base_vaddr) == expected_word1 &&
            ld(low_word_alias_vaddr) == expected_word1 &&
            ld(high_word_base_vaddr) == expected_word2 &&
            ld(high_word_alias_vaddr) == expected_word2 &&
            ld(clean_base_vaddr) == expected_clean_seed &&
            ld(clean_alias_vaddr) == expected_clean_seed &&
            ld(neighbor_base_vaddr) == neighbor_updates[round] &&
            ld(neighbor_alias_vaddr) == neighbor_updates[round] &&
            ld(flush_base_vaddr) == expected_flush_seed &&
            ld(flush_alias_vaddr) == expected_flush_seed &&
            ld(marker_base_vaddr) == marker_updates[round] &&
            ld(marker_alias_vaddr) == marker_updates[round]
        );

        TEST_SETUP_EXCEPT();
        AI_ASSERT_NO_EXCEPT_AND("the same-paddr-alias refill plus CBOM/post-cross16-store owner handoff preserves guards and the adjacent cache block through both views",
            ld(before_guard_base_vaddr) == before_guards[round] &&
            ld(before_guard_alias_vaddr) == before_guards[round] &&
            ld(after_guard_base_vaddr) == after_guards[round] &&
            ld(after_guard_alias_vaddr) == after_guards[round] &&
            ld(marker_guard_base_vaddr) == marker_guards[round] &&
            ld(marker_guard_alias_vaddr) == marker_guards[round] &&
            ld(adjacent_base_vaddr) == adjacent_seeds[round] &&
            ld(adjacent_alias_vaddr) == adjacent_seeds[round]
        );

        goto_priv(PRIV_M);
        TEST_ASSERT("backing memory after the same-paddr-alias cbo.inval + prefetch_r + CBOM/post-cross16-store handoff contains only the precise split-store overlay, post-CBOM neighbor store, marker update, and unchanged guards",
            ai_micro_diffsync_block_matches_seed_except_three_words(
                target_base_paddr,
                block_seeds[round],
                1U,
                expected_word1,
                2U,
                expected_word2,
                5U,
                neighbor_updates[round]) &&
            ai_micro_diffsync_block_matches_seed(adjacent_base_paddr,
                adjacent_seeds[round]) &&
            read64(before_guard_paddr) == before_guards[round] &&
            read64(after_guard_paddr) == after_guards[round] &&
            read64(marker_paddr) == marker_updates[round] &&
            read64(marker_guard_paddr) == marker_guards[round]
        );
    }

    ai_micro_diffsync_reset_env(MENVCFG_CBIE01 | MENVCFG_CBZFE);

    TEST_END();
}

bool ai_micro_hs_cbo_inval_prefetch_refill_then_prior_store_fence_post_store_no_stale_prior_or_marker_drop_corner()
{
    TEST_START();

    const enum test_page target_page = SWITCH1;
    const enum test_page marker_page = SWITCH2;
    const uintptr_t target_base_vaddr = hs_page_base(target_page) + 0xc80U;
    const uintptr_t target_base_paddr = phys_page_base(target_page) + 0xc80U;
    const uintptr_t target_word_vaddr = target_base_vaddr;
    const uintptr_t prefetch_vaddr = target_base_vaddr + sizeof(uint64_t);
    const uintptr_t inval_vaddr = target_base_vaddr + 4U * sizeof(uint64_t);
    const uintptr_t neighbor_vaddr =
        target_base_vaddr + 5U * sizeof(uint64_t);
    const uintptr_t tail_vaddr =
        target_base_vaddr + (AI_MICRO_DIFFSYNC_BLOCK_WORDS - 1U) *
        sizeof(uint64_t);
    const uintptr_t before_guard_vaddr = target_base_vaddr - sizeof(uint64_t);
    const uintptr_t after_guard_vaddr =
        target_base_vaddr + 2U * AI_MICRO_DIFFSYNC_BLOCK_BYTES;
    const uintptr_t adjacent_base_vaddr =
        target_base_vaddr + AI_MICRO_DIFFSYNC_BLOCK_BYTES;
    const uintptr_t adjacent_base_paddr =
        target_base_paddr + AI_MICRO_DIFFSYNC_BLOCK_BYTES;
    const uintptr_t before_guard_paddr =
        target_base_paddr - sizeof(uint64_t);
    const uintptr_t after_guard_paddr =
        target_base_paddr + 2U * AI_MICRO_DIFFSYNC_BLOCK_BYTES;
    const uintptr_t marker_vaddr = hs_page_base(marker_page) + 0xc80U;
    const uintptr_t marker_paddr = phys_page_base(marker_page) + 0xc80U;
    const uintptr_t marker_guard_vaddr = marker_vaddr + sizeof(uint64_t);
    const uintptr_t marker_guard_paddr = marker_paddr + sizeof(uint64_t);
    const uint64_t block_seeds[] = {
        0x1021324354657687ULL,
        0x2132435465768798ULL,
        0x32435465768798a9ULL,
    };
    const uint64_t adjacent_seeds[] = {
        0xfedcba9876543210ULL,
        0x0123456789abcdefULL,
        0x55aa7733cc11ee99ULL,
    };
    const uint64_t prior_stores[] = {
        0x0d0c0b0a09080706ULL,
        0x1d1c1b1a19181716ULL,
        0x2d2c2b2a29282726ULL,
    };
    const uint64_t neighbor_updates[] = {
        0x8877665544332211ULL,
        0x7766554433221100ULL,
        0x66554433221100ffULL,
    };
    const uint64_t before_guards[] = {
        0xfeedfacecafebeefULL,
        0xdeadc0def00dbabeULL,
        0xfaceb00c13579bdfULL,
    };
    const uint64_t after_guards[] = {
        0x0badf00d5a5aa5a5ULL,
        0x1020304050607080ULL,
        0x1122334455667788ULL,
    };
    const uint64_t marker_seeds[] = {
        0xa0a1a2a3a4a5a6a7ULL,
        0xb0b1b2b3b4b5b6b7ULL,
        0xc0c1c2c3c4c5c6c7ULL,
    };
    const uint64_t marker_updates[] = {
        0x3141592653589793ULL,
        0x2718281828459045ULL,
        0x1123581321345589ULL,
    };
    const uint64_t marker_guards[] = {
        0xfdb97531eca86420ULL,
        0x13579bdf2468ace0ULL,
        0x02468ace13579bdfULL,
    };

    ai_micro_diffsync_prepare_hs_env(MENVCFG_CBIE01);

    for (size_t round = 0; round < sizeof(block_seeds) / sizeof(block_seeds[0]);
         ++round) {
        const uint64_t expected_neighbor_seed =
            ai_micro_diffsync_seed_word(block_seeds[round], 5U);
        const uint64_t expected_tail =
            ai_micro_diffsync_seed_word(block_seeds[round],
                AI_MICRO_DIFFSYNC_BLOCK_WORDS - 1U);
        uint64_t target_after_fence = 0ULL;
        uint64_t neighbor_observed = 0ULL;
        uint64_t marker_observed = 0ULL;

        goto_priv(PRIV_M);
        ai_micro_diffsync_seed_block(target_base_paddr, block_seeds[round]);
        ai_micro_diffsync_seed_block(adjacent_base_paddr, adjacent_seeds[round]);
        write64(before_guard_paddr, before_guards[round]);
        write64(after_guard_paddr, after_guards[round]);
        write64(marker_paddr, marker_seeds[round]);
        write64(marker_guard_paddr, marker_guards[round]);

        goto_priv(PRIV_HS);
        TEST_SETUP_EXCEPT();
        AI_ASSERT_NO_EXCEPT_AND("before the cbo.inval + prefetch_r + prior-store + fence handoff starts, target block, neighbor word, adjacent block, and marker are readable with seeded values",
            ld(target_word_vaddr) == block_seeds[round] &&
            ld(neighbor_vaddr) == expected_neighbor_seed &&
            ld(tail_vaddr) == expected_tail &&
            ld(adjacent_base_vaddr) == adjacent_seeds[round] &&
            ld(marker_vaddr) == marker_seeds[round]
        );

        TEST_SETUP_EXCEPT();
        ai_micro_diffsync_cbo_inval_prefetch_prior_store_fence_post_store_window(
            inval_vaddr, prefetch_vaddr, target_word_vaddr, neighbor_vaddr,
            marker_vaddr, prior_stores[round], neighbor_updates[round],
            marker_updates[round], &target_after_fence, &neighbor_observed,
            &marker_observed);
        AI_ASSERT_NO_EXCEPT_AND("cbo.inval plus same-block prefetch_r must not let a refill image erase the prior store before the fence hands ownership to post-fence neighbor and marker stores",
            target_after_fence == prior_stores[round] &&
            neighbor_observed == neighbor_updates[round] &&
            marker_observed == marker_updates[round] &&
            ld(target_word_vaddr) == prior_stores[round] &&
            ld(neighbor_vaddr) == neighbor_updates[round] &&
            ld(tail_vaddr) == expected_tail &&
            ld(marker_vaddr) == marker_updates[round]
        );

        TEST_SETUP_EXCEPT();
        AI_ASSERT_NO_EXCEPT_AND("the cbo.inval + prefetch_r + prior-store + fence post-store owner handoff preserves target guards, marker guard, and the adjacent cache block",
            ld(before_guard_vaddr) == before_guards[round] &&
            ld(after_guard_vaddr) == after_guards[round] &&
            ld(marker_guard_vaddr) == marker_guards[round] &&
            ld(adjacent_base_vaddr) == adjacent_seeds[round]
        );

        goto_priv(PRIV_M);
        TEST_ASSERT("backing memory after the cbo.inval + prefetch_r + prior-store + fence post-store handoff contains only the prior target store, post-fence neighbor store, marker update, and unchanged guards",
            ai_micro_diffsync_block_matches_seed_except_two_words(
                target_base_paddr,
                block_seeds[round],
                0U,
                prior_stores[round],
                5U,
                neighbor_updates[round]) &&
            ai_micro_diffsync_block_matches_seed(adjacent_base_paddr,
                adjacent_seeds[round]) &&
            read64(before_guard_paddr) == before_guards[round] &&
            read64(after_guard_paddr) == after_guards[round] &&
            read64(marker_paddr) == marker_updates[round] &&
            read64(marker_guard_paddr) == marker_guards[round]
        );
    }

    ai_micro_diffsync_reset_env(MENVCFG_CBIE01);

    TEST_END();
}

bool ai_micro_hs_cbo_inval_prefetch_same_paddr_alias_refill_then_prior_store_fence_post_store_no_stale_prior_or_marker_drop_corner()
{
    TEST_START();

    const enum test_page base_page = SWITCH1;
    const enum test_page alias_page = SWITCH2;
    const uintptr_t target_base_vaddr = hs_page_base(base_page) + 0xd00U;
    const uintptr_t target_base_paddr = phys_page_base(base_page) + 0xd00U;
    const uintptr_t target_word_base_vaddr = target_base_vaddr;
    const uintptr_t prefetch_base_vaddr =
        target_base_vaddr + sizeof(uint64_t);
    const uintptr_t neighbor_base_vaddr =
        target_base_vaddr + 5U * sizeof(uint64_t);
    const uintptr_t tail_base_vaddr =
        target_base_vaddr + (AI_MICRO_DIFFSYNC_BLOCK_WORDS - 1U) *
        sizeof(uint64_t);
    const uintptr_t before_guard_base_vaddr =
        target_base_vaddr - sizeof(uint64_t);
    const uintptr_t after_guard_base_vaddr =
        target_base_vaddr + 2U * AI_MICRO_DIFFSYNC_BLOCK_BYTES;
    const uintptr_t adjacent_base_vaddr =
        target_base_vaddr + AI_MICRO_DIFFSYNC_BLOCK_BYTES;
    const uintptr_t marker_base_vaddr =
        target_base_vaddr + 3U * AI_MICRO_DIFFSYNC_BLOCK_BYTES;
    const uintptr_t marker_guard_base_vaddr =
        marker_base_vaddr + sizeof(uint64_t);
    const uintptr_t before_guard_paddr =
        target_base_paddr - sizeof(uint64_t);
    const uintptr_t after_guard_paddr =
        target_base_paddr + 2U * AI_MICRO_DIFFSYNC_BLOCK_BYTES;
    const uintptr_t adjacent_base_paddr =
        target_base_paddr + AI_MICRO_DIFFSYNC_BLOCK_BYTES;
    const uintptr_t marker_paddr =
        target_base_paddr + 3U * AI_MICRO_DIFFSYNC_BLOCK_BYTES;
    const uintptr_t marker_guard_paddr =
        marker_paddr + sizeof(uint64_t);
    const uint64_t block_seeds[] = {
        0x405162738495a6b7ULL,
        0x5162738495a6b7c8ULL,
        0x62738495a6b7c8d9ULL,
    };
    const uint64_t adjacent_seeds[] = {
        0xfedcba9876543210ULL,
        0x0123456789abcdefULL,
        0x55aa7733cc11ee99ULL,
    };
    const uint64_t prior_stores[] = {
        0x0708090a0b0c0d0eULL,
        0x1718191a1b1c1d1eULL,
        0x2728292a2b2c2d2eULL,
    };
    const uint64_t neighbor_updates[] = {
        0x8877665544332211ULL,
        0x7766554433221100ULL,
        0x66554433221100ffULL,
    };
    const uint64_t before_guards[] = {
        0xfeedfacecafebeefULL,
        0xdeadc0def00dbabeULL,
        0xfaceb00c13579bdfULL,
    };
    const uint64_t after_guards[] = {
        0x0badf00d5a5aa5a5ULL,
        0x1020304050607080ULL,
        0x1122334455667788ULL,
    };
    const uint64_t marker_seeds[] = {
        0xa0a1a2a3a4a5a6a7ULL,
        0xb0b1b2b3b4b5b6b7ULL,
        0xc0c1c2c3c4c5c6c7ULL,
    };
    const uint64_t marker_updates[] = {
        0x3141592653589793ULL,
        0x2718281828459045ULL,
        0x1123581321345589ULL,
    };
    const uint64_t marker_guards[] = {
        0xfdb97531eca86420ULL,
        0x13579bdf2468ace0ULL,
        0x02468ace13579bdfULL,
    };
    uintptr_t alias_page_base_vaddr = 0;
    uintptr_t target_alias_vaddr = 0;
    uintptr_t inval_alias_vaddr = 0;
    uintptr_t neighbor_alias_vaddr = 0;
    uintptr_t tail_alias_vaddr = 0;
    uintptr_t before_guard_alias_vaddr = 0;
    uintptr_t after_guard_alias_vaddr = 0;
    uintptr_t adjacent_alias_vaddr = 0;
    uintptr_t marker_alias_vaddr = 0;
    uintptr_t marker_guard_alias_vaddr = 0;

    ai_micro_diffsync_prepare_hs_env(MENVCFG_CBIE01);
    alias_page_base_vaddr =
        ai_micro_diffsync_prepare_same_paddr_alias(alias_page, base_page);
    target_alias_vaddr = alias_page_base_vaddr + 0xd00U;
    inval_alias_vaddr = target_alias_vaddr + 4U * sizeof(uint64_t);
    neighbor_alias_vaddr = target_alias_vaddr + 5U * sizeof(uint64_t);
    tail_alias_vaddr =
        target_alias_vaddr + (AI_MICRO_DIFFSYNC_BLOCK_WORDS - 1U) *
        sizeof(uint64_t);
    before_guard_alias_vaddr = target_alias_vaddr - sizeof(uint64_t);
    after_guard_alias_vaddr =
        target_alias_vaddr + 2U * AI_MICRO_DIFFSYNC_BLOCK_BYTES;
    adjacent_alias_vaddr =
        target_alias_vaddr + AI_MICRO_DIFFSYNC_BLOCK_BYTES;
    marker_alias_vaddr =
        target_alias_vaddr + 3U * AI_MICRO_DIFFSYNC_BLOCK_BYTES;
    marker_guard_alias_vaddr =
        marker_alias_vaddr + sizeof(uint64_t);

    for (size_t round = 0; round < sizeof(block_seeds) / sizeof(block_seeds[0]);
         ++round) {
        const uint64_t expected_neighbor_seed =
            ai_micro_diffsync_seed_word(block_seeds[round], 5U);
        const uint64_t expected_tail =
            ai_micro_diffsync_seed_word(block_seeds[round],
                AI_MICRO_DIFFSYNC_BLOCK_WORDS - 1U);
        uint64_t target_after_fence = 0ULL;
        uint64_t neighbor_observed = 0ULL;
        uint64_t marker_observed = 0ULL;

        goto_priv(PRIV_M);
        ai_micro_diffsync_seed_block(target_base_paddr, block_seeds[round]);
        ai_micro_diffsync_seed_block(adjacent_base_paddr, adjacent_seeds[round]);
        write64(before_guard_paddr, before_guards[round]);
        write64(after_guard_paddr, after_guards[round]);
        write64(marker_paddr, marker_seeds[round]);
        write64(marker_guard_paddr, marker_guards[round]);

        goto_priv(PRIV_HS);
        TEST_SETUP_EXCEPT();
        AI_ASSERT_NO_EXCEPT_AND("before the same-paddr-alias cbo.inval + base-view prefetch_r + alias-view prior-store + fence handoff starts, both target views, neighbor word, adjacent block, and marker are readable with seeded values",
            ld(target_word_base_vaddr) == block_seeds[round] &&
            ld(target_alias_vaddr) == block_seeds[round] &&
            ld(neighbor_base_vaddr) == expected_neighbor_seed &&
            ld(neighbor_alias_vaddr) == expected_neighbor_seed &&
            ld(tail_base_vaddr) == expected_tail &&
            ld(tail_alias_vaddr) == expected_tail &&
            ld(adjacent_base_vaddr) == adjacent_seeds[round] &&
            ld(adjacent_alias_vaddr) == adjacent_seeds[round] &&
            ld(marker_base_vaddr) == marker_seeds[round] &&
            ld(marker_alias_vaddr) == marker_seeds[round]
        );

        TEST_SETUP_EXCEPT();
        ai_micro_diffsync_cbo_inval_prefetch_prior_store_fence_post_store_window(
            inval_alias_vaddr, prefetch_base_vaddr, target_alias_vaddr,
            neighbor_base_vaddr, marker_base_vaddr, prior_stores[round],
            neighbor_updates[round], marker_updates[round],
            &target_after_fence, &neighbor_observed, &marker_observed);
        AI_ASSERT_NO_EXCEPT_AND("cbo.inval through a same-paddr alias followed by base-view prefetch_r must not let a refill image erase the alias-view prior store before fence ownership hands off to post-fence stores",
            target_after_fence == prior_stores[round] &&
            neighbor_observed == neighbor_updates[round] &&
            marker_observed == marker_updates[round] &&
            ld(target_word_base_vaddr) == prior_stores[round] &&
            ld(target_alias_vaddr) == prior_stores[round] &&
            ld(neighbor_base_vaddr) == neighbor_updates[round] &&
            ld(neighbor_alias_vaddr) == neighbor_updates[round] &&
            ld(tail_base_vaddr) == expected_tail &&
            ld(tail_alias_vaddr) == expected_tail &&
            ld(marker_base_vaddr) == marker_updates[round] &&
            ld(marker_alias_vaddr) == marker_updates[round]
        );

        TEST_SETUP_EXCEPT();
        AI_ASSERT_NO_EXCEPT_AND("the same-paddr-alias refill plus prior-store fence post-store owner handoff preserves guards and the adjacent cache block through both views",
            ld(before_guard_base_vaddr) == before_guards[round] &&
            ld(before_guard_alias_vaddr) == before_guards[round] &&
            ld(after_guard_base_vaddr) == after_guards[round] &&
            ld(after_guard_alias_vaddr) == after_guards[round] &&
            ld(marker_guard_base_vaddr) == marker_guards[round] &&
            ld(marker_guard_alias_vaddr) == marker_guards[round] &&
            ld(adjacent_base_vaddr) == adjacent_seeds[round] &&
            ld(adjacent_alias_vaddr) == adjacent_seeds[round]
        );

        goto_priv(PRIV_M);
        TEST_ASSERT("backing memory after the same-paddr-alias cbo.inval + prefetch_r + prior-store + fence post-store handoff contains only the prior target store, post-fence neighbor store, marker update, and unchanged guards",
            ai_micro_diffsync_block_matches_seed_except_two_words(
                target_base_paddr,
                block_seeds[round],
                0U,
                prior_stores[round],
                5U,
                neighbor_updates[round]) &&
            ai_micro_diffsync_block_matches_seed(adjacent_base_paddr,
                adjacent_seeds[round]) &&
            read64(before_guard_paddr) == before_guards[round] &&
            read64(after_guard_paddr) == after_guards[round] &&
            read64(marker_paddr) == marker_updates[round] &&
            read64(marker_guard_paddr) == marker_guards[round]
        );
    }

    ai_micro_diffsync_reset_env(MENVCFG_CBIE01);

    TEST_END();
}
