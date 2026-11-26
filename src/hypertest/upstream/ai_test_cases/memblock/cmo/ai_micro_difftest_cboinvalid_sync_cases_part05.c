#include "ai_micro_difftest_cboinvalid_sync_common.h"

bool ai_micro_hs_cbo_inval_prefetch_refill_then_cbom_fence_post_store_no_stale_cmo_fence_owner_or_marker_drop_corner()
{
    TEST_START();

    const enum test_page target_page = SWITCH1;
    const enum test_page marker_page = SWITCH2;
    const uintptr_t target_base_vaddr = hs_page_base(target_page) + 0xd40U;
    const uintptr_t target_base_paddr = phys_page_base(target_page) + 0xd40U;
    const uintptr_t target_word_vaddr = target_base_vaddr;
    const uintptr_t prefetch_vaddr = target_base_vaddr + sizeof(uint64_t);
    const uintptr_t clean_vaddr = target_base_vaddr + 3U * sizeof(uint64_t);
    const uintptr_t inval_vaddr = target_base_vaddr + 4U * sizeof(uint64_t);
    const uintptr_t post_store_vaddr =
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
    const uintptr_t marker_vaddr = hs_page_base(marker_page) + 0xd40U;
    const uintptr_t marker_paddr = phys_page_base(marker_page) + 0xd40U;
    const uintptr_t marker_guard_vaddr = marker_vaddr + sizeof(uint64_t);
    const uintptr_t marker_guard_paddr = marker_paddr + sizeof(uint64_t);
    const uint64_t block_seeds[] = {
        0x4b5c6d7e8f9012a3ULL,
        0x5c6d7e8f9012a3b4ULL,
        0x6d7e8f9012a3b4c5ULL,
    };
    const uint64_t adjacent_seeds[] = {
        0xfedcba9876543210ULL,
        0x0123456789abcdefULL,
        0x55aa7733cc11ee99ULL,
    };
    const uint64_t dirty_target_updates[] = {
        0x1029384756aabbccULL,
        0x21436587a9cbed0fULL,
        0xfdb97531eca86420ULL,
    };
    const uint64_t post_store_updates[] = {
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
        const uint64_t expected_post_store_seed =
            ai_micro_diffsync_seed_word(block_seeds[round], 5U);
        const uint64_t expected_flush_seed =
            ai_micro_diffsync_seed_word(block_seeds[round],
                AI_MICRO_DIFFSYNC_BLOCK_WORDS - 1U);
        uint64_t target_observed = 0ULL;
        uint64_t post_store_observed = 0ULL;
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
        AI_ASSERT_NO_EXCEPT_AND("before the same-block cbo.inval + prefetch_r + dirty store + CBOM + fence handoff starts, target word, clean/flush probes, post-fence word, adjacent block, and marker are readable with seeded values",
            ld(target_word_vaddr) == block_seeds[round] &&
            ld(clean_vaddr) == expected_clean_seed &&
            ld(post_store_vaddr) == expected_post_store_seed &&
            ld(flush_vaddr) == expected_flush_seed &&
            ld(adjacent_base_vaddr) == adjacent_seeds[round] &&
            ld(marker_vaddr) == marker_seeds[round]
        );

        TEST_SETUP_EXCEPT();
        ai_micro_diffsync_cbo_inval_prefetch_cbom_fence_post_store_window(
            inval_vaddr, prefetch_vaddr, target_word_vaddr, clean_vaddr,
            flush_vaddr, post_store_vaddr, marker_vaddr,
            dirty_target_updates[round], post_store_updates[round],
            marker_updates[round], &target_observed, &post_store_observed,
            &marker_observed);
        AI_ASSERT_NO_EXCEPT_AND("cbo.inval plus same-block prefetch_r must not let CBOM clean/flush or the following fence replay stale refill data or block post-fence stores",
            target_observed == dirty_target_updates[round] &&
            post_store_observed == post_store_updates[round] &&
            marker_observed == marker_updates[round] &&
            ld(target_word_vaddr) == dirty_target_updates[round] &&
            ld(clean_vaddr) == expected_clean_seed &&
            ld(post_store_vaddr) == post_store_updates[round] &&
            ld(flush_vaddr) == expected_flush_seed &&
            ld(marker_vaddr) == marker_updates[round]
        );

        TEST_SETUP_EXCEPT();
        AI_ASSERT_NO_EXCEPT_AND("the refill plus dirty-store CBOM fence post-store handoff preserves target guards, marker guard, and the adjacent cache block",
            ld(before_guard_vaddr) == before_guards[round] &&
            ld(after_guard_vaddr) == after_guards[round] &&
            ld(marker_guard_vaddr) == marker_guards[round] &&
            ld(adjacent_base_vaddr) == adjacent_seeds[round]
        );

        goto_priv(PRIV_M);
        TEST_ASSERT("backing memory after the cbo.inval + prefetch_r + dirty-store CBOM fence post-store handoff contains the drained dirty target, post-fence store, marker update, and unchanged guards",
            ai_micro_diffsync_block_matches_seed_except_two_words(
                target_base_paddr,
                block_seeds[round],
                0U,
                dirty_target_updates[round],
                5U,
                post_store_updates[round]) &&
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

bool ai_micro_hs_cbo_inval_prefetch_refill_then_prior_store_fence_cbom_post_store_no_stale_fence_cmo_owner_or_marker_drop_corner()
{
    TEST_START();

    const enum test_page target_page = SWITCH1;
    const enum test_page marker_page = SWITCH2;
    const uintptr_t target_base_vaddr = hs_page_base(target_page) + 0xdc0U;
    const uintptr_t target_base_paddr = phys_page_base(target_page) + 0xdc0U;
    const uintptr_t target_word_vaddr = target_base_vaddr;
    const uintptr_t prefetch_vaddr = target_base_vaddr + sizeof(uint64_t);
    const uintptr_t clean_vaddr = target_base_vaddr + 3U * sizeof(uint64_t);
    const uintptr_t inval_vaddr = target_base_vaddr + 4U * sizeof(uint64_t);
    const uintptr_t post_store_vaddr =
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
    const uintptr_t marker_vaddr = hs_page_base(marker_page) + 0xdc0U;
    const uintptr_t marker_paddr = phys_page_base(marker_page) + 0xdc0U;
    const uintptr_t marker_guard_vaddr = marker_vaddr + sizeof(uint64_t);
    const uintptr_t marker_guard_paddr = marker_paddr + sizeof(uint64_t);
    const uint64_t block_seeds[] = {
        0x789abcde01234567ULL,
        0x89abcde012345678ULL,
        0x9abcde0123456789ULL,
    };
    const uint64_t adjacent_seeds[] = {
        0xfedcba9876543210ULL,
        0x0123456789abcdefULL,
        0x55aa7733cc11ee99ULL,
    };
    const uint64_t prior_store_updates[] = {
        0x0f1e2d3c4b5a6978ULL,
        0x1021324354657687ULL,
        0x2132435465768798ULL,
    };
    const uint64_t post_cbom_updates[] = {
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
        const uint64_t expected_post_store_seed =
            ai_micro_diffsync_seed_word(block_seeds[round], 5U);
        const uint64_t expected_flush_seed =
            ai_micro_diffsync_seed_word(block_seeds[round],
                AI_MICRO_DIFFSYNC_BLOCK_WORDS - 1U);
        uint64_t prior_store_observed = 0ULL;
        uint64_t post_store_observed = 0ULL;
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
        AI_ASSERT_NO_EXCEPT_AND("before the same-block cbo.inval + prefetch_r + prior store + fence + CBOM handoff starts, target word, clean/flush probes, post-CBOM word, adjacent block, and marker are readable with seeded values",
            ld(target_word_vaddr) == block_seeds[round] &&
            ld(clean_vaddr) == expected_clean_seed &&
            ld(post_store_vaddr) == expected_post_store_seed &&
            ld(flush_vaddr) == expected_flush_seed &&
            ld(adjacent_base_vaddr) == adjacent_seeds[round] &&
            ld(marker_vaddr) == marker_seeds[round]
        );

        TEST_SETUP_EXCEPT();
        ai_micro_diffsync_cbo_inval_prefetch_store_fence_cbom_post_store_window(
            inval_vaddr, prefetch_vaddr, target_word_vaddr, clean_vaddr,
            flush_vaddr, post_store_vaddr, marker_vaddr,
            prior_store_updates[round], post_cbom_updates[round],
            marker_updates[round], &prior_store_observed,
            &post_store_observed, &marker_observed);
        AI_ASSERT_NO_EXCEPT_AND("cbo.inval plus same-block prefetch_r must not let the fence before CBOM reuse stale refill ownership or let following CBOM drop post-CBOM stores",
            prior_store_observed == prior_store_updates[round] &&
            post_store_observed == post_cbom_updates[round] &&
            marker_observed == marker_updates[round] &&
            ld(target_word_vaddr) == prior_store_updates[round] &&
            ld(clean_vaddr) == expected_clean_seed &&
            ld(post_store_vaddr) == post_cbom_updates[round] &&
            ld(flush_vaddr) == expected_flush_seed &&
            ld(marker_vaddr) == marker_updates[round]
        );

        TEST_SETUP_EXCEPT();
        AI_ASSERT_NO_EXCEPT_AND("the refill plus prior-store fence CBOM post-store handoff preserves target guards, marker guard, and the adjacent cache block",
            ld(before_guard_vaddr) == before_guards[round] &&
            ld(after_guard_vaddr) == after_guards[round] &&
            ld(marker_guard_vaddr) == marker_guards[round] &&
            ld(adjacent_base_vaddr) == adjacent_seeds[round]
        );

        goto_priv(PRIV_M);
        TEST_ASSERT("backing memory after the cbo.inval + prefetch_r + prior-store fence CBOM post-store handoff contains the fence-preserved prior store, post-CBOM store, marker update, and unchanged guards",
            ai_micro_diffsync_block_matches_seed_except_two_words(
                target_base_paddr,
                block_seeds[round],
                0U,
                prior_store_updates[round],
                5U,
                post_cbom_updates[round]) &&
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

bool ai_micro_hs_cbo_inval_prefetch_same_paddr_alias_refill_then_cbom_fence_post_store_no_stale_cmo_fence_owner_or_marker_drop_corner()
{
    TEST_START();

    const enum test_page base_page = SWITCH1;
    const enum test_page alias_page = SWITCH2;
    const uintptr_t target_base_vaddr = hs_page_base(base_page) + 0xe40U;
    const uintptr_t target_base_paddr = phys_page_base(base_page) + 0xe40U;
    const uintptr_t target_word_base_vaddr = target_base_vaddr;
    const uintptr_t prefetch_base_vaddr =
        target_base_vaddr + sizeof(uint64_t);
    const uintptr_t clean_base_vaddr =
        target_base_vaddr + 3U * sizeof(uint64_t);
    const uintptr_t post_store_base_vaddr =
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
        0x8a9bacbdcedff001ULL,
        0x9bacbdcedff00112ULL,
        0xacbdcedff0011223ULL,
    };
    const uint64_t adjacent_seeds[] = {
        0xfedcba9876543210ULL,
        0x0123456789abcdefULL,
        0x55aa7733cc11ee99ULL,
    };
    const uint64_t dirty_target_updates[] = {
        0x132435465768798aULL,
        0x2435465768798a9bULL,
        0x35465768798a9bacULL,
    };
    const uint64_t post_store_updates[] = {
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
    uintptr_t clean_alias_vaddr = 0;
    uintptr_t post_store_alias_vaddr = 0;
    uintptr_t flush_alias_vaddr = 0;
    uintptr_t before_guard_alias_vaddr = 0;
    uintptr_t after_guard_alias_vaddr = 0;
    uintptr_t adjacent_alias_vaddr = 0;
    uintptr_t marker_alias_vaddr = 0;
    uintptr_t marker_guard_alias_vaddr = 0;

    ai_micro_diffsync_prepare_hs_env(MENVCFG_CBIE01 | MENVCFG_CBZFE);
    alias_page_base_vaddr =
        ai_micro_diffsync_prepare_same_paddr_alias(alias_page, base_page);
    target_alias_vaddr = alias_page_base_vaddr + 0xe40U;
    inval_alias_vaddr = target_alias_vaddr + 4U * sizeof(uint64_t);
    clean_alias_vaddr = target_alias_vaddr + 3U * sizeof(uint64_t);
    post_store_alias_vaddr = target_alias_vaddr + 5U * sizeof(uint64_t);
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
        const uint64_t expected_clean_seed =
            ai_micro_diffsync_seed_word(block_seeds[round], 3U);
        const uint64_t expected_post_store_seed =
            ai_micro_diffsync_seed_word(block_seeds[round], 5U);
        const uint64_t expected_flush_seed =
            ai_micro_diffsync_seed_word(block_seeds[round],
                AI_MICRO_DIFFSYNC_BLOCK_WORDS - 1U);
        uint64_t target_observed = 0ULL;
        uint64_t post_store_observed = 0ULL;
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
        AI_ASSERT_NO_EXCEPT_AND("before the same-paddr-alias cbo.inval + prefetch_r + dirty store + CBOM + fence handoff starts, both target views, clean/flush probes, post-fence word, adjacent block, and marker are readable with seeded values",
            ld(target_word_base_vaddr) == block_seeds[round] &&
            ld(target_alias_vaddr) == block_seeds[round] &&
            ld(clean_base_vaddr) == expected_clean_seed &&
            ld(clean_alias_vaddr) == expected_clean_seed &&
            ld(post_store_base_vaddr) == expected_post_store_seed &&
            ld(post_store_alias_vaddr) == expected_post_store_seed &&
            ld(flush_base_vaddr) == expected_flush_seed &&
            ld(flush_alias_vaddr) == expected_flush_seed &&
            ld(adjacent_base_vaddr) == adjacent_seeds[round] &&
            ld(adjacent_alias_vaddr) == adjacent_seeds[round] &&
            ld(marker_base_vaddr) == marker_seeds[round] &&
            ld(marker_alias_vaddr) == marker_seeds[round]
        );

        TEST_SETUP_EXCEPT();
        ai_micro_diffsync_cbo_inval_prefetch_cbom_fence_post_store_window(
            inval_alias_vaddr, prefetch_base_vaddr, target_alias_vaddr,
            clean_base_vaddr, flush_alias_vaddr, post_store_base_vaddr,
            marker_alias_vaddr, dirty_target_updates[round],
            post_store_updates[round], marker_updates[round],
            &target_observed, &post_store_observed, &marker_observed);
        AI_ASSERT_NO_EXCEPT_AND("cbo.inval through a same-paddr alias followed by base-view prefetch_r must not let alias/base CBOM plus fence replay stale refill data or block post-fence stores",
            target_observed == dirty_target_updates[round] &&
            post_store_observed == post_store_updates[round] &&
            marker_observed == marker_updates[round] &&
            ld(target_word_base_vaddr) == dirty_target_updates[round] &&
            ld(target_alias_vaddr) == dirty_target_updates[round] &&
            ld(clean_base_vaddr) == expected_clean_seed &&
            ld(clean_alias_vaddr) == expected_clean_seed &&
            ld(post_store_base_vaddr) == post_store_updates[round] &&
            ld(post_store_alias_vaddr) == post_store_updates[round] &&
            ld(flush_base_vaddr) == expected_flush_seed &&
            ld(flush_alias_vaddr) == expected_flush_seed &&
            ld(marker_base_vaddr) == marker_updates[round] &&
            ld(marker_alias_vaddr) == marker_updates[round]
        );

        TEST_SETUP_EXCEPT();
        AI_ASSERT_NO_EXCEPT_AND("the same-paddr-alias refill plus dirty-store CBOM fence post-store handoff preserves guards and the adjacent cache block through both views",
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
        TEST_ASSERT("backing memory after the same-paddr-alias cbo.inval + prefetch_r + dirty-store CBOM fence post-store handoff contains the drained dirty target, post-fence store, marker update, and unchanged guards",
            ai_micro_diffsync_block_matches_seed_except_two_words(
                target_base_paddr,
                block_seeds[round],
                0U,
                dirty_target_updates[round],
                5U,
                post_store_updates[round]) &&
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

bool ai_micro_hs_cbo_inval_prefetch_same_paddr_alias_refill_then_prior_store_fence_cbom_post_store_no_stale_fence_cmo_owner_or_marker_drop_corner()
{
    TEST_START();

    const enum test_page base_page = SWITCH1;
    const enum test_page alias_page = SWITCH2;
    const uintptr_t target_base_vaddr = hs_page_base(base_page) + 0xe80U;
    const uintptr_t target_base_paddr = phys_page_base(base_page) + 0xe80U;
    const uintptr_t target_word_base_vaddr = target_base_vaddr;
    const uintptr_t prefetch_base_vaddr =
        target_base_vaddr + sizeof(uint64_t);
    const uintptr_t clean_base_vaddr =
        target_base_vaddr + 3U * sizeof(uint64_t);
    const uintptr_t post_store_base_vaddr =
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
        0xb4c5d6e7f8091a2bULL,
        0xc5d6e7f8091a2b3cULL,
        0xd6e7f8091a2b3c4dULL,
    };
    const uint64_t adjacent_seeds[] = {
        0xfedcba9876543210ULL,
        0x0123456789abcdefULL,
        0x55aa7733cc11ee99ULL,
    };
    const uint64_t prior_store_updates[] = {
        0x465768798a9bacbdULL,
        0x5768798a9bacbdceULL,
        0x68798a9bacbdcedfULL,
    };
    const uint64_t post_cbom_updates[] = {
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
    uintptr_t clean_alias_vaddr = 0;
    uintptr_t post_store_alias_vaddr = 0;
    uintptr_t flush_alias_vaddr = 0;
    uintptr_t before_guard_alias_vaddr = 0;
    uintptr_t after_guard_alias_vaddr = 0;
    uintptr_t adjacent_alias_vaddr = 0;
    uintptr_t marker_alias_vaddr = 0;
    uintptr_t marker_guard_alias_vaddr = 0;

    ai_micro_diffsync_prepare_hs_env(MENVCFG_CBIE01 | MENVCFG_CBZFE);
    alias_page_base_vaddr =
        ai_micro_diffsync_prepare_same_paddr_alias(alias_page, base_page);
    target_alias_vaddr = alias_page_base_vaddr + 0xe80U;
    inval_alias_vaddr = target_alias_vaddr + 4U * sizeof(uint64_t);
    clean_alias_vaddr = target_alias_vaddr + 3U * sizeof(uint64_t);
    post_store_alias_vaddr = target_alias_vaddr + 5U * sizeof(uint64_t);
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
        const uint64_t expected_clean_seed =
            ai_micro_diffsync_seed_word(block_seeds[round], 3U);
        const uint64_t expected_post_store_seed =
            ai_micro_diffsync_seed_word(block_seeds[round], 5U);
        const uint64_t expected_flush_seed =
            ai_micro_diffsync_seed_word(block_seeds[round],
                AI_MICRO_DIFFSYNC_BLOCK_WORDS - 1U);
        uint64_t prior_store_observed = 0ULL;
        uint64_t post_store_observed = 0ULL;
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
        AI_ASSERT_NO_EXCEPT_AND("before the same-paddr-alias cbo.inval + prefetch_r + prior store + fence + CBOM handoff starts, both target views, clean/flush probes, post-CBOM word, adjacent block, and marker are readable with seeded values",
            ld(target_word_base_vaddr) == block_seeds[round] &&
            ld(target_alias_vaddr) == block_seeds[round] &&
            ld(clean_base_vaddr) == expected_clean_seed &&
            ld(clean_alias_vaddr) == expected_clean_seed &&
            ld(post_store_base_vaddr) == expected_post_store_seed &&
            ld(post_store_alias_vaddr) == expected_post_store_seed &&
            ld(flush_base_vaddr) == expected_flush_seed &&
            ld(flush_alias_vaddr) == expected_flush_seed &&
            ld(adjacent_base_vaddr) == adjacent_seeds[round] &&
            ld(adjacent_alias_vaddr) == adjacent_seeds[round] &&
            ld(marker_base_vaddr) == marker_seeds[round] &&
            ld(marker_alias_vaddr) == marker_seeds[round]
        );

        TEST_SETUP_EXCEPT();
        ai_micro_diffsync_cbo_inval_prefetch_store_fence_cbom_post_store_window(
            inval_alias_vaddr, prefetch_base_vaddr, target_alias_vaddr,
            clean_base_vaddr, flush_alias_vaddr, post_store_base_vaddr,
            marker_alias_vaddr, prior_store_updates[round],
            post_cbom_updates[round], marker_updates[round],
            &prior_store_observed, &post_store_observed, &marker_observed);
        AI_ASSERT_NO_EXCEPT_AND("cbo.inval through a same-paddr alias followed by base-view prefetch_r must not let the fence before alias/base CBOM reuse stale refill ownership or drop post-CBOM stores",
            prior_store_observed == prior_store_updates[round] &&
            post_store_observed == post_cbom_updates[round] &&
            marker_observed == marker_updates[round] &&
            ld(target_word_base_vaddr) == prior_store_updates[round] &&
            ld(target_alias_vaddr) == prior_store_updates[round] &&
            ld(clean_base_vaddr) == expected_clean_seed &&
            ld(clean_alias_vaddr) == expected_clean_seed &&
            ld(post_store_base_vaddr) == post_cbom_updates[round] &&
            ld(post_store_alias_vaddr) == post_cbom_updates[round] &&
            ld(flush_base_vaddr) == expected_flush_seed &&
            ld(flush_alias_vaddr) == expected_flush_seed &&
            ld(marker_base_vaddr) == marker_updates[round] &&
            ld(marker_alias_vaddr) == marker_updates[round]
        );

        TEST_SETUP_EXCEPT();
        AI_ASSERT_NO_EXCEPT_AND("the same-paddr-alias refill plus prior-store fence CBOM post-store handoff preserves guards and the adjacent cache block through both views",
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
        TEST_ASSERT("backing memory after the same-paddr-alias cbo.inval + prefetch_r + prior-store fence CBOM post-store handoff contains the fence-preserved prior store, post-CBOM store, marker update, and unchanged guards",
            ai_micro_diffsync_block_matches_seed_except_two_words(
                target_base_paddr,
                block_seeds[round],
                0U,
                prior_store_updates[round],
                5U,
                post_cbom_updates[round]) &&
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

bool ai_micro_hs_cbo_inval_prefetch_same_paddr_alias_refill_then_cbom_lrsc_no_stale_owner_or_reservation_loss_corner()
{
    TEST_START();

    const enum test_page base_page = SWITCH1;
    const enum test_page alias_page = SWITCH2;
    const uintptr_t target_base_vaddr = hs_page_base(base_page) + 0xbc0U;
    const uintptr_t target_base_paddr = phys_page_base(base_page) + 0xbc0U;
    const uintptr_t lrsc_base_vaddr = target_base_vaddr;
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
        0xe7f8091a2b3c4d5eULL,
        0xf8091a2b3c4d5e6fULL,
        0x091a2b3c4d5e6f70ULL,
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
    uintptr_t alias_page_base_vaddr = 0;
    uintptr_t lrsc_alias_vaddr = 0;
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
    lrsc_alias_vaddr = alias_page_base_vaddr + 0xbc0U;
    inval_alias_vaddr = lrsc_alias_vaddr + 4U * sizeof(uint64_t);
    clean_alias_vaddr = lrsc_alias_vaddr + 3U * sizeof(uint64_t);
    neighbor_alias_vaddr = lrsc_alias_vaddr + 5U * sizeof(uint64_t);
    flush_alias_vaddr =
        lrsc_alias_vaddr + (AI_MICRO_DIFFSYNC_BLOCK_WORDS - 1U) *
        sizeof(uint64_t);
    before_guard_alias_vaddr = lrsc_alias_vaddr - sizeof(uint64_t);
    after_guard_alias_vaddr =
        lrsc_alias_vaddr + 2U * AI_MICRO_DIFFSYNC_BLOCK_BYTES;
    adjacent_alias_vaddr =
        lrsc_alias_vaddr + AI_MICRO_DIFFSYNC_BLOCK_BYTES;
    marker_alias_vaddr =
        lrsc_alias_vaddr + 3U * AI_MICRO_DIFFSYNC_BLOCK_BYTES;
    marker_guard_alias_vaddr =
        marker_alias_vaddr + sizeof(uint64_t);

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
        AI_ASSERT_NO_EXCEPT_AND("before the same-paddr-alias cbo.inval + prefetch_r + dirty store + CBOM + LR/SC handoff starts, both target views, clean/flush probes, neighbor word, adjacent block, and marker are readable with seeded values",
            ld(lrsc_base_vaddr) == block_seeds[round] &&
            ld(lrsc_alias_vaddr) == block_seeds[round] &&
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
        ai_micro_diffsync_cbo_inval_prefetch_cbom_lrsc_window(
            inval_alias_vaddr, prefetch_base_vaddr, clean_base_vaddr,
            flush_alias_vaddr, lrsc_alias_vaddr, neighbor_base_vaddr,
            marker_alias_vaddr, pre_lrsc_updates[round], sc_payloads[round],
            neighbor_updates[round], marker_updates[round], &lr_observed,
            &sc_status, &neighbor_observed, &marker_observed);
        AI_ASSERT_NO_EXCEPT_AND("cbo.inval through a same-paddr alias followed by base-view prefetch_r must not let alias/base CBOM replay stale refill data or keep ownership when alias-view LR/SC follows",
            lr_observed == pre_lrsc_updates[round] &&
            sc_status == 0ULL &&
            neighbor_observed == neighbor_updates[round] &&
            marker_observed == marker_updates[round] &&
            ld(lrsc_base_vaddr) == sc_payloads[round] &&
            ld(lrsc_alias_vaddr) == sc_payloads[round] &&
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
        AI_ASSERT_NO_EXCEPT_AND("the same-paddr-alias refill plus CBOM-to-LR/SC owner handoff preserves guards and the adjacent cache block through both views",
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
        TEST_ASSERT("backing memory after the same-paddr-alias cbo.inval + prefetch_r + CBOM + LR/SC handoff contains only the successful SC payload, same-block neighbor store, marker update, and unchanged guards",
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

bool ai_micro_hs_cbo_inval_prefetch_same_paddr_alias_refill_then_lrsc_cbom_no_stale_sc_or_cmo_owner_drop_corner()
{
    TEST_START();

    const enum test_page base_page = SWITCH1;
    const enum test_page alias_page = SWITCH2;
    const uintptr_t target_base_vaddr = hs_page_base(base_page) + 0xc40U;
    const uintptr_t target_base_paddr = phys_page_base(base_page) + 0xc40U;
    const uintptr_t lrsc_base_vaddr = target_base_vaddr;
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
        0x1a2b3c4d5e6f7081ULL,
        0x2b3c4d5e6f708192ULL,
        0x3c4d5e6f708192a3ULL,
    };
    const uint64_t adjacent_seeds[] = {
        0x55aa7733cc11ee99ULL,
        0xfedcba9876543210ULL,
        0x0123456789abcdefULL,
    };
    const uint64_t sc_payloads[] = {
        0x89abcdef01234567ULL,
        0x76543210fedcba98ULL,
        0x0f1e2d3c4b5a6978ULL,
    };
    const uint64_t neighbor_updates[] = {
        0x1029384756657483ULL,
        0x9182736455463728ULL,
        0xa1b2c3d4e5f60718ULL,
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
        0xc0c1c2c3c4c5c6c7ULL,
        0xd0d1d2d3d4d5d6d7ULL,
        0xe0e1e2e3e4e5e6e7ULL,
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
    uintptr_t lrsc_alias_vaddr = 0;
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
    lrsc_alias_vaddr = alias_page_base_vaddr + 0xc40U;
    inval_alias_vaddr = lrsc_alias_vaddr + 4U * sizeof(uint64_t);
    clean_alias_vaddr = lrsc_alias_vaddr + 3U * sizeof(uint64_t);
    neighbor_alias_vaddr = lrsc_alias_vaddr + 5U * sizeof(uint64_t);
    flush_alias_vaddr =
        lrsc_alias_vaddr + (AI_MICRO_DIFFSYNC_BLOCK_WORDS - 1U) *
        sizeof(uint64_t);
    before_guard_alias_vaddr = lrsc_alias_vaddr - sizeof(uint64_t);
    after_guard_alias_vaddr =
        lrsc_alias_vaddr + 2U * AI_MICRO_DIFFSYNC_BLOCK_BYTES;
    adjacent_alias_vaddr =
        lrsc_alias_vaddr + AI_MICRO_DIFFSYNC_BLOCK_BYTES;
    marker_alias_vaddr =
        lrsc_alias_vaddr + 3U * AI_MICRO_DIFFSYNC_BLOCK_BYTES;
    marker_guard_alias_vaddr =
        marker_alias_vaddr + sizeof(uint64_t);

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
        AI_ASSERT_NO_EXCEPT_AND("before the same-paddr-alias cbo.inval + prefetch_r + LR/SC + CBOM handoff starts, both target views, clean/flush probes, neighbor word, adjacent block, and marker are readable with seeded values",
            ld(lrsc_base_vaddr) == block_seeds[round] &&
            ld(lrsc_alias_vaddr) == block_seeds[round] &&
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
        ai_micro_diffsync_cbo_inval_prefetch_lrsc_cbom_window(
            inval_alias_vaddr, prefetch_base_vaddr, lrsc_alias_vaddr,
            clean_base_vaddr, flush_alias_vaddr, neighbor_base_vaddr,
            marker_alias_vaddr, sc_payloads[round], neighbor_updates[round],
            marker_updates[round], &lr_observed, &sc_status,
            &target_observed, &neighbor_observed, &marker_observed);
        AI_ASSERT_NO_EXCEPT_AND("cbo.inval through a same-paddr alias followed by base-view prefetch_r must not let alias-view LR/SC lose its successful store when alias/base CBOM follows",
            lr_observed == block_seeds[round] &&
            sc_status == 0ULL &&
            target_observed == sc_payloads[round] &&
            neighbor_observed == neighbor_updates[round] &&
            marker_observed == marker_updates[round] &&
            ld(lrsc_base_vaddr) == sc_payloads[round] &&
            ld(lrsc_alias_vaddr) == sc_payloads[round] &&
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
        AI_ASSERT_NO_EXCEPT_AND("the same-paddr-alias refill plus LR/SC-to-CBOM owner handoff preserves guards and the adjacent cache block through both views",
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
        TEST_ASSERT("backing memory after the same-paddr-alias cbo.inval + prefetch_r + LR/SC + CBOM handoff contains the successful SC payload, post-CBOM neighbor store, marker update, and unchanged guards",
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

bool ai_micro_hs_cbo_inval_prefetch_same_paddr_alias_refill_then_cbom_amo_no_stale_owner_or_lost_update_corner()
{
    TEST_START();

    const enum test_page base_page = SWITCH1;
    const enum test_page alias_page = SWITCH2;
    const uintptr_t target_base_vaddr = hs_page_base(base_page) + 0xcc0U;
    const uintptr_t target_base_paddr = phys_page_base(base_page) + 0xcc0U;
    const uintptr_t target_word_base_vaddr = target_base_vaddr;
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
        0x405162738495a6b7ULL,
        0x5162738495a6b7c8ULL,
        0x62738495a6b7c8d9ULL,
    };
    const uint64_t adjacent_seeds[] = {
        0x0123456789abcdefULL,
        0x55aa7733cc11ee99ULL,
        0xfedcba9876543210ULL,
    };
    const uint64_t pre_amo_updates[] = {
        0x2222333344445555ULL,
        0x6666777788889999ULL,
        0xaaaabbbbccccddddULL,
    };
    const uint64_t first_addends[] = {
        0x0000000000000707ULL,
        0x0000000000000808ULL,
        0x0000000000000909ULL,
    };
    const uint64_t second_addends[] = {
        0x0007000700070007ULL,
        0x0008000800080008ULL,
        0x0009000900090009ULL,
    };
    const uint64_t neighbor_updates[] = {
        0x8899aabbccddeeffULL,
        0x7766554433221100ULL,
        0x1029384756657483ULL,
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
        0xd0d1d2d3d4d5d6d7ULL,
        0xe0e1e2e3e4e5e6e7ULL,
        0xf0f1f2f3f4f5f6f7ULL,
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
    uintptr_t alias_page_base_vaddr = 0;
    uintptr_t target_word_alias_vaddr = 0;
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
    target_word_alias_vaddr = alias_page_base_vaddr + 0xcc0U;
    inval_alias_vaddr = target_word_alias_vaddr + 4U * sizeof(uint64_t);
    clean_alias_vaddr = target_word_alias_vaddr + 3U * sizeof(uint64_t);
    neighbor_alias_vaddr = target_word_alias_vaddr + 5U * sizeof(uint64_t);
    flush_alias_vaddr =
        target_word_alias_vaddr + (AI_MICRO_DIFFSYNC_BLOCK_WORDS - 1U) *
        sizeof(uint64_t);
    before_guard_alias_vaddr =
        target_word_alias_vaddr - sizeof(uint64_t);
    after_guard_alias_vaddr =
        target_word_alias_vaddr + 2U * AI_MICRO_DIFFSYNC_BLOCK_BYTES;
    adjacent_alias_vaddr =
        target_word_alias_vaddr + AI_MICRO_DIFFSYNC_BLOCK_BYTES;
    marker_alias_vaddr =
        target_word_alias_vaddr + 3U * AI_MICRO_DIFFSYNC_BLOCK_BYTES;
    marker_guard_alias_vaddr =
        marker_alias_vaddr + sizeof(uint64_t);

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
        AI_ASSERT_NO_EXCEPT_AND("before the same-paddr-alias cbo.inval + prefetch_r + dirty store + CBOM + AMO handoff starts, both target views, clean/flush probes, neighbor word, adjacent block, and marker are readable with seeded values",
            ld(target_word_base_vaddr) == block_seeds[round] &&
            ld(target_word_alias_vaddr) == block_seeds[round] &&
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
        ai_micro_diffsync_cbo_inval_prefetch_cbom_amo_window(
            inval_alias_vaddr, prefetch_base_vaddr, target_word_alias_vaddr,
            clean_base_vaddr, flush_alias_vaddr, neighbor_base_vaddr,
            marker_alias_vaddr, pre_amo_updates[round], first_addends[round],
            second_addends[round], neighbor_updates[round],
            marker_updates[round], &first_amo_old, &second_amo_old,
            &target_observed, &neighbor_observed, &marker_observed);
        AI_ASSERT_NO_EXCEPT_AND("cbo.inval through a same-paddr alias followed by base-view prefetch_r must not let alias/base CBOM replay stale refill data before alias-view AMO consumes the current target word",
            first_amo_old == pre_amo_updates[round] &&
            second_amo_old == after_first_amo &&
            target_observed == final_target &&
            neighbor_observed == neighbor_updates[round] &&
            marker_observed == marker_updates[round] &&
            ld(target_word_base_vaddr) == final_target &&
            ld(target_word_alias_vaddr) == final_target &&
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
        AI_ASSERT_NO_EXCEPT_AND("the same-paddr-alias refill plus CBOM-to-AMO owner handoff preserves guards and the adjacent cache block through both views",
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
        TEST_ASSERT("backing memory after the same-paddr-alias cbo.inval + prefetch_r + CBOM + AMO handoff contains the final AMO target value, same-block neighbor store, marker update, and unchanged guards",
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

bool ai_micro_hs_cbo_inval_prefetch_same_paddr_alias_refill_then_amo_cbom_no_stale_amo_or_cmo_owner_drop_corner()
{
    TEST_START();

    const enum test_page base_page = SWITCH1;
    const enum test_page alias_page = SWITCH2;
    const uintptr_t target_base_vaddr = hs_page_base(base_page) + 0xd20U;
    const uintptr_t target_base_paddr = phys_page_base(base_page) + 0xd20U;
    const uintptr_t target_word_base_vaddr = target_base_vaddr;
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
        0x738495a6b7c8d9e0ULL,
        0x8495a6b7c8d9e0f1ULL,
        0x95a6b7c8d9e0f102ULL,
    };
    const uint64_t adjacent_seeds[] = {
        0xfedcba9876543210ULL,
        0x0123456789abcdefULL,
        0x55aa7733cc11ee99ULL,
    };
    const uint64_t first_addends[] = {
        0x0000000000000a0aULL,
        0x0000000000000b0bULL,
        0x0000000000000c0cULL,
    };
    const uint64_t second_addends[] = {
        0x000a000a000a000aULL,
        0x000b000b000b000bULL,
        0x000c000c000c000cULL,
    };
    const uint64_t neighbor_updates[] = {
        0x2233445566778899ULL,
        0x9988776655443322ULL,
        0x13579bdf2468ace0ULL,
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
    uintptr_t target_word_alias_vaddr = 0;
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
    target_word_alias_vaddr = alias_page_base_vaddr + 0xd20U;
    inval_alias_vaddr = target_word_alias_vaddr + 4U * sizeof(uint64_t);
    clean_alias_vaddr = target_word_alias_vaddr + 3U * sizeof(uint64_t);
    neighbor_alias_vaddr = target_word_alias_vaddr + 5U * sizeof(uint64_t);
    flush_alias_vaddr =
        target_word_alias_vaddr + (AI_MICRO_DIFFSYNC_BLOCK_WORDS - 1U) *
        sizeof(uint64_t);
    before_guard_alias_vaddr =
        target_word_alias_vaddr - sizeof(uint64_t);
    after_guard_alias_vaddr =
        target_word_alias_vaddr + 2U * AI_MICRO_DIFFSYNC_BLOCK_BYTES;
    adjacent_alias_vaddr =
        target_word_alias_vaddr + AI_MICRO_DIFFSYNC_BLOCK_BYTES;
    marker_alias_vaddr =
        target_word_alias_vaddr + 3U * AI_MICRO_DIFFSYNC_BLOCK_BYTES;
    marker_guard_alias_vaddr =
        marker_alias_vaddr + sizeof(uint64_t);

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
        AI_ASSERT_NO_EXCEPT_AND("before the same-paddr-alias cbo.inval + prefetch_r + AMO + CBOM handoff starts, both target views, clean/flush probes, neighbor word, adjacent block, and marker are readable with seeded values",
            ld(target_word_base_vaddr) == block_seeds[round] &&
            ld(target_word_alias_vaddr) == block_seeds[round] &&
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
        ai_micro_diffsync_cbo_inval_prefetch_amo_cbom_window(
            inval_alias_vaddr, prefetch_base_vaddr, target_word_alias_vaddr,
            clean_base_vaddr, flush_alias_vaddr, neighbor_base_vaddr,
            marker_alias_vaddr, first_addends[round], second_addends[round],
            neighbor_updates[round], marker_updates[round], &first_amo_old,
            &second_amo_old, &target_observed, &neighbor_observed,
            &marker_observed);
        AI_ASSERT_NO_EXCEPT_AND("cbo.inval through a same-paddr alias followed by base-view prefetch_r must not let alias-view AMO consume stale refill data or let following alias/base CBOM drop the final AMO target value",
            first_amo_old == block_seeds[round] &&
            second_amo_old == after_first_amo &&
            target_observed == final_target &&
            neighbor_observed == neighbor_updates[round] &&
            marker_observed == marker_updates[round] &&
            ld(target_word_base_vaddr) == final_target &&
            ld(target_word_alias_vaddr) == final_target &&
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
        AI_ASSERT_NO_EXCEPT_AND("the same-paddr-alias refill plus AMO-to-CBOM owner handoff preserves guards and the adjacent cache block through both views",
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
        TEST_ASSERT("backing memory after the same-paddr-alias cbo.inval + prefetch_r + AMO + CBOM handoff contains the final AMO target value, post-CBOM neighbor store, marker update, and unchanged guards",
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

bool ai_micro_hs_cbo_inval_prefetch_same_paddr_alias_refill_then_cross16_load_cbom_no_stale_merge_or_cmo_owner_corner()
{
    TEST_START();

    const enum test_page base_page = SWITCH1;
    const enum test_page alias_page = SWITCH2;
    const uintptr_t target_base_vaddr = hs_page_base(base_page) + 0xd60U;
    const uintptr_t target_base_paddr = phys_page_base(base_page) + 0xd60U;
    const uintptr_t word1_base_vaddr =
        target_base_vaddr + sizeof(uint64_t);
    const uintptr_t word2_base_vaddr =
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
        0xa5b6c7d8e9fa0b1cULL,
        0xb6c7d8e9fa0b1c2dULL,
        0xc7d8e9fa0b1c2d3eULL,
    };
    const uint64_t adjacent_seeds[] = {
        0x0123456789abcdefULL,
        0xfedcba9876543210ULL,
        0x55aa7733cc11ee99ULL,
    };
    const uint64_t word1_updates[] = {
        0x2143658709badcfeULL,
        0x89abcdef01234567ULL,
        0x76543210fedcba98ULL,
    };
    const uint64_t word2_updates[] = {
        0xfedcba9876543210ULL,
        0x1021324354657687ULL,
        0x99aabbccddeeff00ULL,
    };
    const uint64_t neighbor_updates[] = {
        0x0f1e2d3c4b5a6978ULL,
        0x8877665544332211ULL,
        0x2468ace013579bdfULL,
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
        0xd1d2d3d4d5d6d7d8ULL,
        0xe1e2e3e4e5e6e7e8ULL,
        0xf1f2f3f4f5f6f7f8ULL,
    };
    const uint64_t marker_updates[] = {
        0x13579bdf2468ace0ULL,
        0x02468ace13579bdfULL,
        0xfdb97531eca86420ULL,
    };
    const uint64_t marker_guards[] = {
        0xf0e1d2c3b4a59687ULL,
        0x0f1e2d3c4b5a6978ULL,
        0x1123581321345589ULL,
    };
    uintptr_t alias_page_base_vaddr = 0;
    uintptr_t word1_alias_vaddr = 0;
    uintptr_t word2_alias_vaddr = 0;
    uintptr_t cross16_load_alias_vaddr = 0;
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
    word1_alias_vaddr =
        alias_page_base_vaddr + 0xd60U + sizeof(uint64_t);
    word2_alias_vaddr =
        alias_page_base_vaddr + 0xd60U + 2U * sizeof(uint64_t);
    cross16_load_alias_vaddr = alias_page_base_vaddr + 0xd60U + 15U;
    inval_alias_vaddr =
        alias_page_base_vaddr + 0xd60U + 4U * sizeof(uint64_t);
    clean_alias_vaddr =
        alias_page_base_vaddr + 0xd60U + 3U * sizeof(uint64_t);
    neighbor_alias_vaddr =
        alias_page_base_vaddr + 0xd60U + 5U * sizeof(uint64_t);
    flush_alias_vaddr =
        alias_page_base_vaddr + 0xd60U +
        (AI_MICRO_DIFFSYNC_BLOCK_WORDS - 1U) * sizeof(uint64_t);
    before_guard_alias_vaddr =
        alias_page_base_vaddr + 0xd60U - sizeof(uint64_t);
    after_guard_alias_vaddr =
        alias_page_base_vaddr + 0xd60U +
        2U * AI_MICRO_DIFFSYNC_BLOCK_BYTES;
    adjacent_alias_vaddr =
        alias_page_base_vaddr + 0xd60U + AI_MICRO_DIFFSYNC_BLOCK_BYTES;
    marker_alias_vaddr =
        alias_page_base_vaddr + 0xd60U +
        3U * AI_MICRO_DIFFSYNC_BLOCK_BYTES;
    marker_guard_alias_vaddr =
        marker_alias_vaddr + sizeof(uint64_t);

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
        AI_ASSERT_NO_EXCEPT_AND("before the same-paddr-alias cbo.inval + prefetch_r + dirty split words + cross-16B load + CBOM handoff starts, both split-word views, clean/flush probes, neighbor word, adjacent block, and marker are readable with seeded values",
            ld(word1_base_vaddr) == seed_word1 &&
            ld(word1_alias_vaddr) == seed_word1 &&
            ld(word2_base_vaddr) == seed_word2 &&
            ld(word2_alias_vaddr) == seed_word2 &&
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
        ai_micro_diffsync_cbo_inval_prefetch_cross16_load_cbom_window(
            inval_alias_vaddr, prefetch_base_vaddr, word1_alias_vaddr,
            word2_base_vaddr, cross16_load_alias_vaddr, clean_base_vaddr,
            flush_alias_vaddr, neighbor_base_vaddr, marker_alias_vaddr,
            word1_updates[round], word2_updates[round],
            neighbor_updates[round], marker_updates[round], &load_observed,
            &word1_observed, &word2_observed, &neighbor_observed,
            &marker_observed);
        AI_ASSERT_NO_EXCEPT_AND("cbo.inval through a same-paddr alias followed by base-view prefetch_r must not let alias-view cross-16B load merge stale halves or let following alias/base CBOM drop dirty split words",
            load_observed == expected_cross16 &&
            word1_observed == word1_updates[round] &&
            word2_observed == word2_updates[round] &&
            neighbor_observed == neighbor_updates[round] &&
            marker_observed == marker_updates[round] &&
            ld(word1_base_vaddr) == word1_updates[round] &&
            ld(word1_alias_vaddr) == word1_updates[round] &&
            ld(word2_base_vaddr) == word2_updates[round] &&
            ld(word2_alias_vaddr) == word2_updates[round] &&
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
        AI_ASSERT_NO_EXCEPT_AND("the same-paddr-alias refill plus cross16-load-to-CBOM owner handoff preserves guards and the adjacent cache block through both views",
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
        TEST_ASSERT("backing memory after the same-paddr-alias cbo.inval + prefetch_r + cross16-load + CBOM handoff contains the dirty split words, post-CBOM neighbor store, marker update, and unchanged guards",
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

bool ai_micro_hs_cbo_inval_prefetch_same_paddr_alias_refill_then_cbom_post_cross16_load_no_stale_owner_or_merge_payload_corner()
{
    TEST_START();

    const enum test_page base_page = SWITCH1;
    const enum test_page alias_page = SWITCH2;
    const uintptr_t target_base_vaddr = hs_page_base(base_page) + 0xda0U;
    const uintptr_t target_base_paddr = phys_page_base(base_page) + 0xda0U;
    const uintptr_t word1_base_vaddr =
        target_base_vaddr + sizeof(uint64_t);
    const uintptr_t word2_base_vaddr =
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
        0xd8e9fa0b1c2d3e4fULL,
        0xe9fa0b1c2d3e4f50ULL,
        0xfa0b1c2d3e4f5061ULL,
    };
    const uint64_t adjacent_seeds[] = {
        0xfedcba9876543210ULL,
        0x55aa7733cc11ee99ULL,
        0x0123456789abcdefULL,
    };
    const uint64_t word1_updates[] = {
        0x0123456789abcdefULL,
        0xf0e1d2c3b4a59687ULL,
        0x2468ace013579bdfULL,
    };
    const uint64_t word2_updates[] = {
        0xfdb97531eca86420ULL,
        0x8877665544332211ULL,
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
        0x0d0e0f1011121314ULL,
        0x1d1e1f2021222324ULL,
        0x2d2e2f3031323334ULL,
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
    uintptr_t alias_page_base_vaddr = 0;
    uintptr_t word1_alias_vaddr = 0;
    uintptr_t word2_alias_vaddr = 0;
    uintptr_t cross16_load_alias_vaddr = 0;
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
    word1_alias_vaddr =
        alias_page_base_vaddr + 0xda0U + sizeof(uint64_t);
    word2_alias_vaddr =
        alias_page_base_vaddr + 0xda0U + 2U * sizeof(uint64_t);
    cross16_load_alias_vaddr = alias_page_base_vaddr + 0xda0U + 15U;
    inval_alias_vaddr =
        alias_page_base_vaddr + 0xda0U + 4U * sizeof(uint64_t);
    clean_alias_vaddr =
        alias_page_base_vaddr + 0xda0U + 3U * sizeof(uint64_t);
    neighbor_alias_vaddr =
        alias_page_base_vaddr + 0xda0U + 5U * sizeof(uint64_t);
    flush_alias_vaddr =
        alias_page_base_vaddr + 0xda0U +
        (AI_MICRO_DIFFSYNC_BLOCK_WORDS - 1U) * sizeof(uint64_t);
    before_guard_alias_vaddr =
        alias_page_base_vaddr + 0xda0U - sizeof(uint64_t);
    after_guard_alias_vaddr =
        alias_page_base_vaddr + 0xda0U +
        2U * AI_MICRO_DIFFSYNC_BLOCK_BYTES;
    adjacent_alias_vaddr =
        alias_page_base_vaddr + 0xda0U + AI_MICRO_DIFFSYNC_BLOCK_BYTES;
    marker_alias_vaddr =
        alias_page_base_vaddr + 0xda0U +
        3U * AI_MICRO_DIFFSYNC_BLOCK_BYTES;
    marker_guard_alias_vaddr =
        marker_alias_vaddr + sizeof(uint64_t);

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
        AI_ASSERT_NO_EXCEPT_AND("before the same-paddr-alias cbo.inval + prefetch_r + dirty split words + CBOM + post-CBOM cross-16B load handoff starts, both split-word views, clean/flush probes, neighbor word, adjacent block, and marker are readable with seeded values",
            ld(word1_base_vaddr) == seed_word1 &&
            ld(word1_alias_vaddr) == seed_word1 &&
            ld(word2_base_vaddr) == seed_word2 &&
            ld(word2_alias_vaddr) == seed_word2 &&
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
        ai_micro_diffsync_cbo_inval_prefetch_cbom_post_cross16_load_window(
            inval_alias_vaddr, prefetch_base_vaddr, word1_alias_vaddr,
            word2_base_vaddr, clean_base_vaddr, flush_alias_vaddr,
            cross16_load_alias_vaddr, neighbor_base_vaddr, marker_alias_vaddr,
            word1_updates[round], word2_updates[round],
            neighbor_updates[round], marker_updates[round], &load_observed,
            &word1_observed, &word2_observed, &neighbor_observed,
            &marker_observed);
        AI_ASSERT_NO_EXCEPT_AND("cbo.inval through a same-paddr alias followed by base-view prefetch_r must not let alias/base CBOM drop dirty split words before the alias-view post-CBOM cross-16B load merges them",
            load_observed == expected_cross16 &&
            word1_observed == word1_updates[round] &&
            word2_observed == word2_updates[round] &&
            neighbor_observed == neighbor_updates[round] &&
            marker_observed == marker_updates[round] &&
            ld(word1_base_vaddr) == word1_updates[round] &&
            ld(word1_alias_vaddr) == word1_updates[round] &&
            ld(word2_base_vaddr) == word2_updates[round] &&
            ld(word2_alias_vaddr) == word2_updates[round] &&
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
        AI_ASSERT_NO_EXCEPT_AND("the same-paddr-alias refill plus CBOM-to-post-cross16-load owner handoff preserves guards and the adjacent cache block through both views",
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
        TEST_ASSERT("backing memory after the same-paddr-alias cbo.inval + prefetch_r + CBOM/post-cross16-load handoff contains the dirty split words, post-CBOM neighbor store, marker update, and unchanged guards",
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

bool ai_micro_hs_cbo_inval_prefetch_same_paddr_alias_refill_then_cbo_zero_cross16_load_no_stale_zero_or_merge_payload_corner()
{
    TEST_START();

    const enum test_page base_page = SWITCH1;
    const enum test_page alias_page = SWITCH2;
    const uintptr_t target_offset = 0xf00U;
    const uintptr_t target_base_vaddr =
        hs_page_base(base_page) + target_offset;
    const uintptr_t target_base_paddr =
        phys_page_base(base_page) + target_offset;
    const uintptr_t word1_base_vaddr =
        target_base_vaddr + sizeof(uint64_t);
    const uintptr_t word2_base_vaddr =
        target_base_vaddr + 2U * sizeof(uint64_t);
    const uintptr_t prefetch_base_vaddr =
        target_base_vaddr + sizeof(uint64_t);
    const uintptr_t zero_base_vaddr =
        target_base_vaddr + 4U * sizeof(uint64_t);
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
        0x6a7b8c9dabcd1234ULL,
        0x7b8c9dabcd123456ULL,
        0x8c9dabcd12345678ULL,
    };
    const uint64_t adjacent_seeds[] = {
        0x1021324354657687ULL,
        0x98badcfe10325476ULL,
        0x7766554433221100ULL,
    };
    const uint64_t before_guards[] = {
        0x1357246813572468ULL,
        0x2468135724681357ULL,
        0xabcdef0123456789ULL,
    };
    const uint64_t after_guards[] = {
        0xf0e1d2c3b4a59687ULL,
        0x0f1e2d3c4b5a6978ULL,
        0x1123581321345589ULL,
    };
    const uint64_t marker_seeds[] = {
        0xa1a2a3a4a5a6a7a8ULL,
        0xb1b2b3b4b5b6b7b8ULL,
        0xc1c2c3c4c5c6c7c8ULL,
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
    uintptr_t alias_page_base_vaddr = 0;
    uintptr_t target_alias_vaddr = 0;
    uintptr_t word1_alias_vaddr = 0;
    uintptr_t word2_alias_vaddr = 0;
    uintptr_t cross16_load_alias_vaddr = 0;
    uintptr_t inval_alias_vaddr = 0;
    uintptr_t zero_alias_vaddr = 0;
    uintptr_t tail_alias_vaddr = 0;
    uintptr_t before_guard_alias_vaddr = 0;
    uintptr_t after_guard_alias_vaddr = 0;
    uintptr_t adjacent_alias_vaddr = 0;
    uintptr_t marker_alias_vaddr = 0;
    uintptr_t marker_guard_alias_vaddr = 0;

    ai_micro_diffsync_prepare_hs_env(MENVCFG_CBIE01 | MENVCFG_CBZE);
    alias_page_base_vaddr =
        ai_micro_diffsync_prepare_same_paddr_alias(alias_page, base_page);
    target_alias_vaddr = alias_page_base_vaddr + target_offset;
    word1_alias_vaddr =
        alias_page_base_vaddr + target_offset + sizeof(uint64_t);
    word2_alias_vaddr =
        alias_page_base_vaddr + target_offset + 2U * sizeof(uint64_t);
    cross16_load_alias_vaddr =
        alias_page_base_vaddr + target_offset + 15U;
    inval_alias_vaddr =
        alias_page_base_vaddr + target_offset + 4U * sizeof(uint64_t);
    zero_alias_vaddr =
        alias_page_base_vaddr + target_offset + 4U * sizeof(uint64_t);
    tail_alias_vaddr =
        alias_page_base_vaddr + target_offset +
        (AI_MICRO_DIFFSYNC_BLOCK_WORDS - 1U) * sizeof(uint64_t);
    before_guard_alias_vaddr =
        alias_page_base_vaddr + target_offset - sizeof(uint64_t);
    after_guard_alias_vaddr =
        alias_page_base_vaddr + target_offset +
        2U * AI_MICRO_DIFFSYNC_BLOCK_BYTES;
    adjacent_alias_vaddr =
        alias_page_base_vaddr + target_offset +
        AI_MICRO_DIFFSYNC_BLOCK_BYTES;
    marker_alias_vaddr =
        alias_page_base_vaddr + target_offset +
        3U * AI_MICRO_DIFFSYNC_BLOCK_BYTES;
    marker_guard_alias_vaddr =
        marker_alias_vaddr + sizeof(uint64_t);

    for (size_t round = 0; round < sizeof(block_seeds) / sizeof(block_seeds[0]);
         ++round) {
        const uint64_t seed_word1 =
            ai_micro_diffsync_seed_word(block_seeds[round], 1U);
        const uint64_t seed_word2 =
            ai_micro_diffsync_seed_word(block_seeds[round], 2U);
        const uint64_t expected_zero_seed =
            ai_micro_diffsync_seed_word(block_seeds[round], 4U);
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
        AI_ASSERT_NO_EXCEPT_AND("before the same-paddr-alias cbo.inval + prefetch_r + cbo.zero + cross-16B load handoff starts, base and alias views expose the seeded split words, zero probe word, adjacent block, and marker",
            ld(target_base_vaddr) == block_seeds[round] &&
            ld(target_alias_vaddr) == block_seeds[round] &&
            ld(word1_base_vaddr) == seed_word1 &&
            ld(word1_alias_vaddr) == seed_word1 &&
            ld(word2_base_vaddr) == seed_word2 &&
            ld(word2_alias_vaddr) == seed_word2 &&
            ld(zero_base_vaddr) == expected_zero_seed &&
            ld(zero_alias_vaddr) == expected_zero_seed &&
            ld(tail_base_vaddr) == expected_tail &&
            ld(tail_alias_vaddr) == expected_tail &&
            ld(adjacent_base_vaddr) == adjacent_seeds[round] &&
            ld(adjacent_alias_vaddr) == adjacent_seeds[round] &&
            ld(marker_base_vaddr) == marker_seeds[round] &&
            ld(marker_alias_vaddr) == marker_seeds[round]
        );

        TEST_SETUP_EXCEPT();
        ai_micro_diffsync_cbo_inval_prefetch_zero_cross16_load_window(
            inval_alias_vaddr, prefetch_base_vaddr, zero_alias_vaddr,
            cross16_load_alias_vaddr, marker_alias_vaddr,
            marker_updates[round], &load_observed, &marker_observed);
        AI_ASSERT_NO_EXCEPT_AND("cbo.inval through a same-paddr alias followed by base-view prefetch_r must let alias-view cbo.zero fully zero the block before the alias-view cross-16B load observes any stale split half",
            load_observed == 0ULL &&
            marker_observed == marker_updates[round] &&
            ld(target_base_vaddr) == 0ULL &&
            ld(target_alias_vaddr) == 0ULL &&
            ld(word1_base_vaddr) == 0ULL &&
            ld(word1_alias_vaddr) == 0ULL &&
            ld(word2_base_vaddr) == 0ULL &&
            ld(word2_alias_vaddr) == 0ULL &&
            ld(zero_base_vaddr) == 0ULL &&
            ld(zero_alias_vaddr) == 0ULL &&
            ld(tail_base_vaddr) == 0ULL &&
            ld(tail_alias_vaddr) == 0ULL &&
            ld(marker_base_vaddr) == marker_updates[round] &&
            ld(marker_alias_vaddr) == marker_updates[round]
        );

        TEST_SETUP_EXCEPT();
        AI_ASSERT_NO_EXCEPT_AND("the same-paddr-alias refill plus zero-to-cross16-load owner handoff preserves guards and the adjacent cache block through both views",
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
        TEST_ASSERT("backing memory after the same-paddr-alias cbo.inval + prefetch_r + cbo.zero + cross-16B load handoff contains a fully zeroed target block, marker update, and unchanged guards",
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

bool ai_micro_hs_cbo_inval_prefetch_same_paddr_alias_refill_then_cross16_load_cbo_zero_no_stale_merge_or_zero_drop_corner()
{
    TEST_START();

    const enum test_page base_page = SWITCH1;
    const enum test_page alias_page = SWITCH2;
    const uintptr_t target_offset = 0x580U;
    const uintptr_t target_base_vaddr =
        hs_page_base(base_page) + target_offset;
    const uintptr_t target_base_paddr =
        phys_page_base(base_page) + target_offset;
    const uintptr_t word1_base_vaddr =
        target_base_vaddr + sizeof(uint64_t);
    const uintptr_t word2_base_vaddr =
        target_base_vaddr + 2U * sizeof(uint64_t);
    const uintptr_t prefetch_base_vaddr =
        target_base_vaddr + sizeof(uint64_t);
    const uintptr_t zero_base_vaddr =
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
        0x1728394a5b6c7d8eULL,
        0x28394a5b6c7d8e9fULL,
        0x394a5b6c7d8e9fa0ULL,
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
    uintptr_t alias_page_base_vaddr = 0;
    uintptr_t target_alias_vaddr = 0;
    uintptr_t word1_alias_vaddr = 0;
    uintptr_t word2_alias_vaddr = 0;
    uintptr_t cross16_load_alias_vaddr = 0;
    uintptr_t inval_alias_vaddr = 0;
    uintptr_t zero_alias_vaddr = 0;
    uintptr_t tail_alias_vaddr = 0;
    uintptr_t before_guard_alias_vaddr = 0;
    uintptr_t after_guard_alias_vaddr = 0;
    uintptr_t adjacent_alias_vaddr = 0;
    uintptr_t marker_alias_vaddr = 0;
    uintptr_t marker_guard_alias_vaddr = 0;

    ai_micro_diffsync_prepare_hs_env(MENVCFG_CBIE01 | MENVCFG_CBZE);
    alias_page_base_vaddr =
        ai_micro_diffsync_prepare_same_paddr_alias(alias_page, base_page);
    target_alias_vaddr = alias_page_base_vaddr + target_offset;
    word1_alias_vaddr =
        alias_page_base_vaddr + target_offset + sizeof(uint64_t);
    word2_alias_vaddr =
        alias_page_base_vaddr + target_offset + 2U * sizeof(uint64_t);
    cross16_load_alias_vaddr =
        alias_page_base_vaddr + target_offset + 15U;
    inval_alias_vaddr =
        alias_page_base_vaddr + target_offset + 4U * sizeof(uint64_t);
    zero_alias_vaddr =
        alias_page_base_vaddr + target_offset + 5U * sizeof(uint64_t);
    tail_alias_vaddr =
        alias_page_base_vaddr + target_offset +
        (AI_MICRO_DIFFSYNC_BLOCK_WORDS - 1U) * sizeof(uint64_t);
    before_guard_alias_vaddr =
        alias_page_base_vaddr + target_offset - sizeof(uint64_t);
    after_guard_alias_vaddr =
        alias_page_base_vaddr + target_offset +
        2U * AI_MICRO_DIFFSYNC_BLOCK_BYTES;
    adjacent_alias_vaddr =
        alias_page_base_vaddr + target_offset +
        AI_MICRO_DIFFSYNC_BLOCK_BYTES;
    marker_alias_vaddr =
        alias_page_base_vaddr + target_offset +
        3U * AI_MICRO_DIFFSYNC_BLOCK_BYTES;
    marker_guard_alias_vaddr =
        marker_alias_vaddr + sizeof(uint64_t);

    for (size_t round = 0; round < sizeof(block_seeds) / sizeof(block_seeds[0]);
         ++round) {
        const uint64_t seed_word1 =
            ai_micro_diffsync_seed_word(block_seeds[round], 1U);
        const uint64_t seed_word2 =
            ai_micro_diffsync_seed_word(block_seeds[round], 2U);
        const uint64_t expected_zero_seed =
            ai_micro_diffsync_seed_word(block_seeds[round], 5U);
        const uint64_t expected_tail =
            ai_micro_diffsync_seed_word(block_seeds[round],
                AI_MICRO_DIFFSYNC_BLOCK_WORDS - 1U);
        const uint64_t expected_cross16 =
            ai_micro_diffsync_cross16_load_from_seed(seed_word1, seed_word2);
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
        AI_ASSERT_NO_EXCEPT_AND("before the same-paddr-alias cbo.inval + prefetch_r + cross-16B load + cbo.zero handoff starts, base and alias views expose the seeded split words, zero probe word, adjacent block, and marker",
            ld(target_base_vaddr) == block_seeds[round] &&
            ld(target_alias_vaddr) == block_seeds[round] &&
            ld(word1_base_vaddr) == seed_word1 &&
            ld(word1_alias_vaddr) == seed_word1 &&
            ld(word2_base_vaddr) == seed_word2 &&
            ld(word2_alias_vaddr) == seed_word2 &&
            ld(zero_base_vaddr) == expected_zero_seed &&
            ld(zero_alias_vaddr) == expected_zero_seed &&
            ld(tail_base_vaddr) == expected_tail &&
            ld(tail_alias_vaddr) == expected_tail &&
            ld(adjacent_base_vaddr) == adjacent_seeds[round] &&
            ld(adjacent_alias_vaddr) == adjacent_seeds[round] &&
            ld(marker_base_vaddr) == marker_seeds[round] &&
            ld(marker_alias_vaddr) == marker_seeds[round]
        );

        TEST_SETUP_EXCEPT();
        ai_micro_diffsync_cbo_inval_prefetch_cross16_load_zero_window(
            inval_alias_vaddr, prefetch_base_vaddr,
            cross16_load_alias_vaddr, zero_alias_vaddr, marker_alias_vaddr,
            marker_updates[round], &load_observed, &marker_observed);
        AI_ASSERT_NO_EXCEPT_AND("cbo.inval through a same-paddr alias followed by base-view prefetch_r must let the alias-view cross-16B load merge the seeded split words before alias-view cbo.zero clears the whole block",
            load_observed == expected_cross16 &&
            marker_observed == marker_updates[round] &&
            ld(target_base_vaddr) == 0ULL &&
            ld(target_alias_vaddr) == 0ULL &&
            ld(word1_base_vaddr) == 0ULL &&
            ld(word1_alias_vaddr) == 0ULL &&
            ld(word2_base_vaddr) == 0ULL &&
            ld(word2_alias_vaddr) == 0ULL &&
            ld(zero_base_vaddr) == 0ULL &&
            ld(zero_alias_vaddr) == 0ULL &&
            ld(tail_base_vaddr) == 0ULL &&
            ld(tail_alias_vaddr) == 0ULL &&
            ld(marker_base_vaddr) == marker_updates[round] &&
            ld(marker_alias_vaddr) == marker_updates[round]
        );

        TEST_SETUP_EXCEPT();
        AI_ASSERT_NO_EXCEPT_AND("the same-paddr-alias refill plus cross16-load-to-zero owner handoff preserves guards and the adjacent cache block through both views",
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
        TEST_ASSERT("backing memory after the same-paddr-alias cbo.inval + prefetch_r + cross-16B load + cbo.zero handoff contains a fully zeroed target block, marker update, and unchanged guards",
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

bool ai_micro_hs_cbo_inval_prefetch_same_paddr_alias_refill_then_cbo_zero_lrsc_no_stale_zero_or_reservation_loss_corner()
{
    TEST_START();

    const enum test_page base_page = SWITCH1;
    const enum test_page alias_page = SWITCH2;
    const uintptr_t target_offset = 0x480U;
    const uintptr_t target_base_vaddr =
        hs_page_base(base_page) + target_offset;
    const uintptr_t target_base_paddr =
        phys_page_base(base_page) + target_offset;
    const uintptr_t prefetch_base_vaddr =
        target_base_vaddr + sizeof(uint64_t);
    const uintptr_t zero_base_vaddr =
        target_base_vaddr + 4U * sizeof(uint64_t);
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
        0xd0c1b2a394857667ULL,
        0x8899aabbccddeeffULL,
        0x0123456789abcdefULL,
    };
    const uint64_t sc_payloads[] = {
        0x5a5a0000c3c3ffffULL,
        0x6b6b1111d2d2eeeeULL,
        0x7c7c2222e1e1ddddULL,
    };
    const uint64_t before_guards[] = {
        0x0f1e2d3c4b5a6978ULL,
        0x1021324354657687ULL,
        0x98badcfe10325476ULL,
    };
    const uint64_t after_guards[] = {
        0xfedcba9876543210ULL,
        0x55aa7733cc11ee99ULL,
        0x1123581321345589ULL,
    };
    const uint64_t marker_seeds[] = {
        0x1111222233334444ULL,
        0x5555666677778888ULL,
        0x9999aaaabbbbccccULL,
    };
    const uint64_t marker_updates[] = {
        0x13579bdf2468ace0ULL,
        0x02468ace13579bdfULL,
        0xfdb97531eca86420ULL,
    };
    const uint64_t marker_guards[] = {
        0x3141592653589793ULL,
        0x2718281828459045ULL,
        0x1123581321345589ULL,
    };
    uintptr_t alias_page_base_vaddr = 0;
    uintptr_t target_alias_vaddr = 0;
    uintptr_t inval_alias_vaddr = 0;
    uintptr_t zero_alias_vaddr = 0;
    uintptr_t tail_alias_vaddr = 0;
    uintptr_t before_guard_alias_vaddr = 0;
    uintptr_t after_guard_alias_vaddr = 0;
    uintptr_t adjacent_alias_vaddr = 0;
    uintptr_t marker_alias_vaddr = 0;
    uintptr_t marker_guard_alias_vaddr = 0;

    ai_micro_diffsync_prepare_hs_env(MENVCFG_CBIE01 | MENVCFG_CBZE);
    alias_page_base_vaddr =
        ai_micro_diffsync_prepare_same_paddr_alias(alias_page, base_page);
    target_alias_vaddr = alias_page_base_vaddr + target_offset;
    inval_alias_vaddr =
        alias_page_base_vaddr + target_offset + 4U * sizeof(uint64_t);
    zero_alias_vaddr =
        alias_page_base_vaddr + target_offset + 4U * sizeof(uint64_t);
    tail_alias_vaddr =
        alias_page_base_vaddr + target_offset +
        (AI_MICRO_DIFFSYNC_BLOCK_WORDS - 1U) * sizeof(uint64_t);
    before_guard_alias_vaddr =
        alias_page_base_vaddr + target_offset - sizeof(uint64_t);
    after_guard_alias_vaddr =
        alias_page_base_vaddr + target_offset +
        2U * AI_MICRO_DIFFSYNC_BLOCK_BYTES;
    adjacent_alias_vaddr =
        alias_page_base_vaddr + target_offset +
        AI_MICRO_DIFFSYNC_BLOCK_BYTES;
    marker_alias_vaddr =
        alias_page_base_vaddr + target_offset +
        3U * AI_MICRO_DIFFSYNC_BLOCK_BYTES;
    marker_guard_alias_vaddr =
        marker_alias_vaddr + sizeof(uint64_t);

    for (size_t round = 0; round < sizeof(block_seeds) / sizeof(block_seeds[0]);
         ++round) {
        const uint64_t expected_zero_seed =
            ai_micro_diffsync_seed_word(block_seeds[round], 4U);
        const uint64_t expected_tail =
            ai_micro_diffsync_seed_word(block_seeds[round],
                AI_MICRO_DIFFSYNC_BLOCK_WORDS - 1U);
        uint64_t lr_observed = 0xffffffffffffffffULL;
        uint64_t sc_status = 1ULL;
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
        AI_ASSERT_NO_EXCEPT_AND("before the same-paddr-alias cbo.inval + prefetch_r + cbo.zero + LR/SC handoff starts, base and alias views expose the seeded target word, zero probe, tail, adjacent block, and marker",
            ld(target_base_vaddr) == block_seeds[round] &&
            ld(target_alias_vaddr) == block_seeds[round] &&
            ld(zero_base_vaddr) == expected_zero_seed &&
            ld(zero_alias_vaddr) == expected_zero_seed &&
            ld(tail_base_vaddr) == expected_tail &&
            ld(tail_alias_vaddr) == expected_tail &&
            ld(adjacent_base_vaddr) == adjacent_seeds[round] &&
            ld(adjacent_alias_vaddr) == adjacent_seeds[round] &&
            ld(marker_base_vaddr) == marker_seeds[round] &&
            ld(marker_alias_vaddr) == marker_seeds[round]
        );

        TEST_SETUP_EXCEPT();
        ai_micro_diffsync_cbo_inval_prefetch_zero_lrsc_window(
            inval_alias_vaddr, prefetch_base_vaddr, zero_alias_vaddr,
            target_alias_vaddr, marker_alias_vaddr, sc_payloads[round],
            marker_updates[round], &lr_observed, &sc_status,
            &marker_observed);
        AI_ASSERT_NO_EXCEPT_AND("cbo.inval through a same-paddr alias followed by base-view prefetch_r must let alias-view cbo.zero fully zero the block before alias-view LR/SC observes zero and keeps its reservation",
            lr_observed == 0ULL &&
            sc_status == 0ULL &&
            marker_observed == marker_updates[round] &&
            ld(target_base_vaddr) == sc_payloads[round] &&
            ld(target_alias_vaddr) == sc_payloads[round] &&
            ld(zero_base_vaddr) == 0ULL &&
            ld(zero_alias_vaddr) == 0ULL &&
            ld(tail_base_vaddr) == 0ULL &&
            ld(tail_alias_vaddr) == 0ULL &&
            ld(marker_base_vaddr) == marker_updates[round] &&
            ld(marker_alias_vaddr) == marker_updates[round]
        );

        TEST_SETUP_EXCEPT();
        AI_ASSERT_NO_EXCEPT_AND("the same-paddr-alias zero-to-LR/SC owner handoff preserves guards and the adjacent cache block through both views",
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
        TEST_ASSERT("backing memory after the same-paddr-alias cbo.inval + prefetch_r + cbo.zero + LR/SC handoff contains one successful SC payload over an otherwise zeroed target block, marker update, and unchanged guards",
            ai_micro_diffsync_block_all_zero_except_word(target_base_paddr,
                0U, sc_payloads[round]) &&
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

bool ai_micro_hs_cbo_inval_prefetch_same_paddr_alias_refill_then_lrsc_cbo_zero_no_stale_sc_or_zero_drop_corner()
{
    TEST_START();

    const enum test_page base_page = SWITCH1;
    const enum test_page alias_page = SWITCH2;
    const uintptr_t target_offset = 0x500U;
    const uintptr_t target_base_vaddr =
        hs_page_base(base_page) + target_offset;
    const uintptr_t target_base_paddr =
        phys_page_base(base_page) + target_offset;
    const uintptr_t prefetch_base_vaddr =
        target_base_vaddr + sizeof(uint64_t);
    const uintptr_t zero_base_vaddr =
        target_base_vaddr + 6U * sizeof(uint64_t);
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
        0x18293a4b5c6d7e8fULL,
        0x293a4b5c6d7e8f90ULL,
        0x3a4b5c6d7e8f9011ULL,
    };
    const uint64_t adjacent_seeds[] = {
        0x0f1e2d3c4b5a6978ULL,
        0x1021324354657687ULL,
        0x98badcfe10325476ULL,
    };
    const uint64_t sc_payloads[] = {
        0xa55a5aa5f00f0ff0ULL,
        0xb66b6bb6e11e1ee1ULL,
        0xc77c7cc7d22d2dd2ULL,
    };
    const uint64_t before_guards[] = {
        0x55aa00ff55aa00ffULL,
        0xaa5500ffaa5500ffULL,
        0x0f0e0d0c0b0a0908ULL,
    };
    const uint64_t after_guards[] = {
        0x0123456789abcdefULL,
        0xfedcba9876543210ULL,
        0x1357246813572468ULL,
    };
    const uint64_t marker_seeds[] = {
        0x2222333344445555ULL,
        0x6666777788889999ULL,
        0xaaaabbbbccccddddULL,
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
    uintptr_t alias_page_base_vaddr = 0;
    uintptr_t target_alias_vaddr = 0;
    uintptr_t inval_alias_vaddr = 0;
    uintptr_t zero_alias_vaddr = 0;
    uintptr_t tail_alias_vaddr = 0;
    uintptr_t before_guard_alias_vaddr = 0;
    uintptr_t after_guard_alias_vaddr = 0;
    uintptr_t adjacent_alias_vaddr = 0;
    uintptr_t marker_alias_vaddr = 0;
    uintptr_t marker_guard_alias_vaddr = 0;

    ai_micro_diffsync_prepare_hs_env(MENVCFG_CBIE01 | MENVCFG_CBZE);
    alias_page_base_vaddr =
        ai_micro_diffsync_prepare_same_paddr_alias(alias_page, base_page);
    target_alias_vaddr = alias_page_base_vaddr + target_offset;
    inval_alias_vaddr =
        alias_page_base_vaddr + target_offset + 3U * sizeof(uint64_t);
    zero_alias_vaddr =
        alias_page_base_vaddr + target_offset + 6U * sizeof(uint64_t);
    tail_alias_vaddr =
        alias_page_base_vaddr + target_offset +
        (AI_MICRO_DIFFSYNC_BLOCK_WORDS - 1U) * sizeof(uint64_t);
    before_guard_alias_vaddr =
        alias_page_base_vaddr + target_offset - sizeof(uint64_t);
    after_guard_alias_vaddr =
        alias_page_base_vaddr + target_offset +
        2U * AI_MICRO_DIFFSYNC_BLOCK_BYTES;
    adjacent_alias_vaddr =
        alias_page_base_vaddr + target_offset +
        AI_MICRO_DIFFSYNC_BLOCK_BYTES;
    marker_alias_vaddr =
        alias_page_base_vaddr + target_offset +
        3U * AI_MICRO_DIFFSYNC_BLOCK_BYTES;
    marker_guard_alias_vaddr =
        marker_alias_vaddr + sizeof(uint64_t);

    for (size_t round = 0; round < sizeof(block_seeds) / sizeof(block_seeds[0]);
         ++round) {
        const uint64_t expected_zero_seed =
            ai_micro_diffsync_seed_word(block_seeds[round], 6U);
        const uint64_t expected_tail =
            ai_micro_diffsync_seed_word(block_seeds[round],
                AI_MICRO_DIFFSYNC_BLOCK_WORDS - 1U);
        uint64_t lr_observed = 0xffffffffffffffffULL;
        uint64_t sc_status = 1ULL;
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
        AI_ASSERT_NO_EXCEPT_AND("before the same-paddr-alias cbo.inval + prefetch_r + LR/SC + cbo.zero handoff starts, base and alias views expose the seeded target word, zero probe, tail, adjacent block, and marker",
            ld(target_base_vaddr) == block_seeds[round] &&
            ld(target_alias_vaddr) == block_seeds[round] &&
            ld(zero_base_vaddr) == expected_zero_seed &&
            ld(zero_alias_vaddr) == expected_zero_seed &&
            ld(tail_base_vaddr) == expected_tail &&
            ld(tail_alias_vaddr) == expected_tail &&
            ld(adjacent_base_vaddr) == adjacent_seeds[round] &&
            ld(adjacent_alias_vaddr) == adjacent_seeds[round] &&
            ld(marker_base_vaddr) == marker_seeds[round] &&
            ld(marker_alias_vaddr) == marker_seeds[round]
        );

        TEST_SETUP_EXCEPT();
        ai_micro_diffsync_cbo_inval_prefetch_lrsc_zero_window(
            inval_alias_vaddr, prefetch_base_vaddr, target_alias_vaddr,
            zero_alias_vaddr, marker_alias_vaddr, sc_payloads[round],
            marker_updates[round], &lr_observed, &sc_status,
            &marker_observed);
        AI_ASSERT_NO_EXCEPT_AND("cbo.inval through a same-paddr alias followed by base-view prefetch_r must let alias-view LR/SC consume the current seed before alias-view cbo.zero clears the whole block",
            lr_observed == block_seeds[round] &&
            sc_status == 0ULL &&
            marker_observed == marker_updates[round] &&
            ld(target_base_vaddr) == 0ULL &&
            ld(target_alias_vaddr) == 0ULL &&
            ld(zero_base_vaddr) == 0ULL &&
            ld(zero_alias_vaddr) == 0ULL &&
            ld(tail_base_vaddr) == 0ULL &&
            ld(tail_alias_vaddr) == 0ULL &&
            ld(marker_base_vaddr) == marker_updates[round] &&
            ld(marker_alias_vaddr) == marker_updates[round]
        );

        TEST_SETUP_EXCEPT();
        AI_ASSERT_NO_EXCEPT_AND("the same-paddr-alias LR/SC-to-zero owner handoff preserves guards and the adjacent cache block through both views",
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
        TEST_ASSERT("backing memory after the same-paddr-alias cbo.inval + prefetch_r + LR/SC + cbo.zero handoff contains a fully zeroed target block, marker update, and unchanged guards",
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

bool ai_micro_hs_cbo_inval_prefetch_same_paddr_alias_refill_then_cbo_zero_amo_no_stale_zero_or_lost_update_corner()
{
    TEST_START();

    const enum test_page base_page = SWITCH1;
    const enum test_page alias_page = SWITCH2;
    const uintptr_t target_offset = 0x600U;
    const uintptr_t target_base_vaddr =
        hs_page_base(base_page) + target_offset;
    const uintptr_t target_base_paddr =
        phys_page_base(base_page) + target_offset;
    const uintptr_t prefetch_base_vaddr =
        target_base_vaddr + sizeof(uint64_t);
    const uintptr_t zero_base_vaddr =
        target_base_vaddr + 4U * sizeof(uint64_t);
    const uintptr_t mid_base_vaddr =
        target_base_vaddr + 3U * sizeof(uint64_t);
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
        0x708192a3b4c5d6e7ULL,
        0x8192a3b4c5d6e7f0ULL,
        0x92a3b4c5d6e7f001ULL,
    };
    const uint64_t adjacent_seeds[] = {
        0x0a1b2c3d4e5f6071ULL,
        0x1122334455667788ULL,
        0x8877665544332211ULL,
    };
    const uint64_t first_addends[] = {
        0x0000000000000123ULL,
        0x0000000000000234ULL,
        0x0000000000000345ULL,
    };
    const uint64_t second_addends[] = {
        0x0001000100010001ULL,
        0x0002000200020002ULL,
        0x0003000300030003ULL,
    };
    const uint64_t before_guards[] = {
        0x55aa55aa00ff00ffULL,
        0xaa55aa55ff00ff00ULL,
        0x0f0e0d0c0b0a0908ULL,
    };
    const uint64_t after_guards[] = {
        0x0123456789abcdefULL,
        0xfedcba9876543210ULL,
        0x1357246813572468ULL,
    };
    const uint64_t marker_seeds[] = {
        0x3333444455556666ULL,
        0x777788889999aaaaULL,
        0xbbbbccccddddeeeeULL,
    };
    const uint64_t marker_updates[] = {
        0x2468ace013579bdfULL,
        0x13579bdf2468ace0ULL,
        0x0f1e2d3c4b5a6978ULL,
    };
    const uint64_t marker_guards[] = {
        0x3141592653589793ULL,
        0x2718281828459045ULL,
        0x1123581321345589ULL,
    };
    uintptr_t alias_page_base_vaddr = 0;
    uintptr_t target_alias_vaddr = 0;
    uintptr_t inval_alias_vaddr = 0;
    uintptr_t zero_alias_vaddr = 0;
    uintptr_t mid_alias_vaddr = 0;
    uintptr_t tail_alias_vaddr = 0;
    uintptr_t before_guard_alias_vaddr = 0;
    uintptr_t after_guard_alias_vaddr = 0;
    uintptr_t adjacent_alias_vaddr = 0;
    uintptr_t marker_alias_vaddr = 0;
    uintptr_t marker_guard_alias_vaddr = 0;

    ai_micro_diffsync_prepare_hs_env(MENVCFG_CBIE01 | MENVCFG_CBZE);
    alias_page_base_vaddr =
        ai_micro_diffsync_prepare_same_paddr_alias(alias_page, base_page);
    target_alias_vaddr = alias_page_base_vaddr + target_offset;
    inval_alias_vaddr =
        alias_page_base_vaddr + target_offset + 6U * sizeof(uint64_t);
    zero_alias_vaddr =
        alias_page_base_vaddr + target_offset + 4U * sizeof(uint64_t);
    mid_alias_vaddr =
        alias_page_base_vaddr + target_offset + 3U * sizeof(uint64_t);
    tail_alias_vaddr =
        alias_page_base_vaddr + target_offset +
        (AI_MICRO_DIFFSYNC_BLOCK_WORDS - 1U) * sizeof(uint64_t);
    before_guard_alias_vaddr =
        alias_page_base_vaddr + target_offset - sizeof(uint64_t);
    after_guard_alias_vaddr =
        alias_page_base_vaddr + target_offset +
        2U * AI_MICRO_DIFFSYNC_BLOCK_BYTES;
    adjacent_alias_vaddr =
        alias_page_base_vaddr + target_offset +
        AI_MICRO_DIFFSYNC_BLOCK_BYTES;
    marker_alias_vaddr =
        alias_page_base_vaddr + target_offset +
        3U * AI_MICRO_DIFFSYNC_BLOCK_BYTES;
    marker_guard_alias_vaddr =
        marker_alias_vaddr + sizeof(uint64_t);

    for (size_t round = 0; round < sizeof(block_seeds) / sizeof(block_seeds[0]);
         ++round) {
        const uint64_t expected_zero_seed =
            ai_micro_diffsync_seed_word(block_seeds[round], 4U);
        const uint64_t expected_mid_seed =
            ai_micro_diffsync_seed_word(block_seeds[round], 3U);
        const uint64_t expected_tail =
            ai_micro_diffsync_seed_word(block_seeds[round],
                AI_MICRO_DIFFSYNC_BLOCK_WORDS - 1U);
        const uint64_t final_target =
            first_addends[round] + second_addends[round];
        uint64_t first_amo_old = 0xffffffffffffffffULL;
        uint64_t second_amo_old = 0xffffffffffffffffULL;
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
        AI_ASSERT_NO_EXCEPT_AND("before the same-paddr-alias cbo.inval + prefetch_r + cbo.zero + AMO handoff starts, base and alias views expose the seeded target word, zero probe, mid/tail words, adjacent block, and marker",
            ld(target_base_vaddr) == block_seeds[round] &&
            ld(target_alias_vaddr) == block_seeds[round] &&
            ld(zero_base_vaddr) == expected_zero_seed &&
            ld(zero_alias_vaddr) == expected_zero_seed &&
            ld(mid_base_vaddr) == expected_mid_seed &&
            ld(mid_alias_vaddr) == expected_mid_seed &&
            ld(tail_base_vaddr) == expected_tail &&
            ld(tail_alias_vaddr) == expected_tail &&
            ld(adjacent_base_vaddr) == adjacent_seeds[round] &&
            ld(adjacent_alias_vaddr) == adjacent_seeds[round] &&
            ld(marker_base_vaddr) == marker_seeds[round] &&
            ld(marker_alias_vaddr) == marker_seeds[round]
        );

        TEST_SETUP_EXCEPT();
        ai_micro_diffsync_cbo_inval_prefetch_zero_amo_window(
            inval_alias_vaddr, prefetch_base_vaddr, zero_alias_vaddr,
            target_alias_vaddr, marker_alias_vaddr, first_addends[round],
            second_addends[round], marker_updates[round], &first_amo_old,
            &second_amo_old, &marker_observed);
        AI_ASSERT_NO_EXCEPT_AND("cbo.inval through a same-paddr alias followed by base-view prefetch_r must let alias-view cbo.zero fully zero the block before alias-view AMO observes zero and commits both updates",
            first_amo_old == 0ULL &&
            second_amo_old == first_addends[round] &&
            marker_observed == marker_updates[round] &&
            ld(target_base_vaddr) == final_target &&
            ld(target_alias_vaddr) == final_target &&
            ld(zero_base_vaddr) == 0ULL &&
            ld(zero_alias_vaddr) == 0ULL &&
            ld(mid_base_vaddr) == 0ULL &&
            ld(mid_alias_vaddr) == 0ULL &&
            ld(tail_base_vaddr) == 0ULL &&
            ld(tail_alias_vaddr) == 0ULL &&
            ld(marker_base_vaddr) == marker_updates[round] &&
            ld(marker_alias_vaddr) == marker_updates[round]
        );

        TEST_SETUP_EXCEPT();
        AI_ASSERT_NO_EXCEPT_AND("the same-paddr-alias zero-to-AMO owner handoff preserves guards and the adjacent cache block through both views",
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
        TEST_ASSERT("backing memory after the same-paddr-alias cbo.inval + prefetch_r + cbo.zero + AMO handoff contains both AMO updates over an otherwise zeroed target block, marker update, and unchanged guards",
            ai_micro_diffsync_block_all_zero_except_word(target_base_paddr,
                0U, final_target) &&
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
