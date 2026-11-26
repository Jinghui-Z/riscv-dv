#include "ai_micro_difftest_cboinvalid_sync_common.h"

bool ai_micro_hs_cbo_inval_prefetch_refill_then_prior_store_lrsc_no_stale_forward_or_reservation_loss_corner()
{
    TEST_START();

    const enum test_page target_page = SWITCH1;
    const enum test_page marker_page = SWITCH2;
    const uintptr_t target_base_vaddr = hs_page_base(target_page) + 0x880U;
    const uintptr_t target_base_paddr = phys_page_base(target_page) + 0x880U;
    const uintptr_t store_vaddr = target_base_vaddr;
    const uintptr_t lrsc_vaddr = target_base_vaddr;
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
    const uintptr_t before_guard_paddr = target_base_paddr - sizeof(uint64_t);
    const uintptr_t after_guard_paddr =
        target_base_paddr + 2U * AI_MICRO_DIFFSYNC_BLOCK_BYTES;
    const uintptr_t adjacent_base_vaddr =
        target_base_vaddr + AI_MICRO_DIFFSYNC_BLOCK_BYTES;
    const uintptr_t adjacent_base_paddr =
        target_base_paddr + AI_MICRO_DIFFSYNC_BLOCK_BYTES;
    const uintptr_t marker_vaddr = hs_page_base(marker_page) + 0x880U;
    const uintptr_t marker_paddr = phys_page_base(marker_page) + 0x880U;
    const uintptr_t marker_guard_vaddr = marker_vaddr + sizeof(uint64_t);
    const uintptr_t marker_guard_paddr = marker_paddr + sizeof(uint64_t);
    const uint64_t block_seeds[] = {
        0x66778899aabbccddULL,
        0x89abcdef01234567ULL,
        0x1021324354657687ULL,
    };
    const uint64_t adjacent_seeds[] = {
        0x0123456789abcdefULL,
        0xfedcba9876543210ULL,
        0x55aa7733cc11ee99ULL,
    };
    const uint64_t prior_stores[] = {
        0x1111222233334444ULL,
        0x5555666677778888ULL,
        0x9999aaaabbbbccccULL,
    };
    const uint64_t sc_payloads[] = {
        0x0102030405060708ULL,
        0x1827364554637281ULL,
        0xfdb97531eca86420ULL,
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
        0x13579bdf2468ace0ULL,
        0x02468ace13579bdfULL,
        0xfdb97531eca86420ULL,
    };
    const uint64_t marker_guards[] = {
        0x3141592653589793ULL,
        0x2718281828459045ULL,
        0x1123581321345589ULL,
    };

    ai_micro_diffsync_prepare_hs_env(MENVCFG_CBIE01);

    for (size_t round = 0; round < sizeof(block_seeds) / sizeof(block_seeds[0]);
         ++round) {
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
        AI_ASSERT_NO_EXCEPT_AND("before the cbo.inval + prefetch_r + prior-store + LR/SC window starts, target block, adjacent block, and marker are readable with seeded values",
            ld(lrsc_vaddr) == block_seeds[round] &&
            ld(tail_vaddr) == expected_tail &&
            ld(adjacent_base_vaddr) == adjacent_seeds[round] &&
            ld(marker_vaddr) == marker_seeds[round]
        );

        TEST_SETUP_EXCEPT();
        ai_micro_diffsync_cbo_inval_prefetch_store_lrsc_window(
            inval_vaddr, prefetch_vaddr, store_vaddr, lrsc_vaddr,
            neighbor_vaddr, marker_vaddr, prior_stores[round],
            sc_payloads[round], neighbor_updates[round],
            marker_updates[round], &lr_observed, &sc_status,
            &marker_observed);
        AI_ASSERT_NO_EXCEPT_AND("cbo.inval plus same-block prefetch_r must not let a refill image bypass the immediately younger prior store observed by LR/SC",
            lr_observed == prior_stores[round] &&
            sc_status == 0ULL &&
            marker_observed == marker_updates[round] &&
            ld(lrsc_vaddr) == sc_payloads[round] &&
            ld(neighbor_vaddr) == neighbor_updates[round] &&
            ld(tail_vaddr) == expected_tail &&
            ld(marker_vaddr) == marker_updates[round]
        );

        TEST_SETUP_EXCEPT();
        AI_ASSERT_NO_EXCEPT_AND("the cbo.inval + prefetch_r + prior-store + LR/SC window preserves target guards, marker guard, and the adjacent cache block",
            ld(before_guard_vaddr) == before_guards[round] &&
            ld(after_guard_vaddr) == after_guards[round] &&
            ld(marker_guard_vaddr) == marker_guards[round] &&
            ld(adjacent_base_vaddr) == adjacent_seeds[round]
        );

        goto_priv(PRIV_M);
        TEST_ASSERT("backing memory after the cbo.inval + prefetch_r + prior-store + LR/SC window contains only the successful SC payload, neighbor store, marker update, and unchanged guards",
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

    ai_micro_diffsync_reset_env(MENVCFG_CBIE01);

    TEST_END();
}

bool ai_micro_hs_cbo_inval_prefetch_same_paddr_alias_refill_then_prior_store_lrsc_no_stale_forward_or_reservation_loss_corner()
{
    TEST_START();

    const enum test_page base_page = SWITCH1;
    const enum test_page alias_page = SWITCH2;
    const uintptr_t target_base_vaddr = hs_page_base(base_page) + 0x900U;
    const uintptr_t target_base_paddr = phys_page_base(base_page) + 0x900U;
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
        0x6a7b8c9daeafb0c1ULL,
        0x123456789abcdef0ULL,
        0x0f1e2d3c4b5a6978ULL,
    };
    const uint64_t adjacent_seeds[] = {
        0x13579bdf2468ace0ULL,
        0xeca86420fdb97531ULL,
        0x55aa00ff7733cc11ULL,
    };
    const uint64_t prior_stores[] = {
        0x1011121314151617ULL,
        0x2122232425262728ULL,
        0x3132333435363738ULL,
    };
    const uint64_t sc_payloads[] = {
        0x8877665544332211ULL,
        0x7766554433221100ULL,
        0x66554433221100ffULL,
    };
    const uint64_t neighbor_updates[] = {
        0xa5a55a5af0f00f0fULL,
        0x0102030405060708ULL,
        0xfdb97531eca86420ULL,
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
        0x0123456789abcdefULL,
        0xfedcba9876543210ULL,
        0x55aa7733cc11ee99ULL,
    };
    uintptr_t alias_page_base_vaddr = 0;
    uintptr_t target_alias_vaddr = 0;
    uintptr_t inval_alias_vaddr = 0;
    uintptr_t neighbor_alias_vaddr = 0;
    uintptr_t tail_alias_vaddr = 0;
    uintptr_t adjacent_alias_vaddr = 0;
    uintptr_t marker_alias_vaddr = 0;
    uintptr_t marker_guard_alias_vaddr = 0;

    ai_micro_diffsync_prepare_hs_env(MENVCFG_CBIE01);
    alias_page_base_vaddr =
        ai_micro_diffsync_prepare_same_paddr_alias(alias_page, base_page);
    target_alias_vaddr = alias_page_base_vaddr + 0x900U;
    inval_alias_vaddr = target_alias_vaddr + 4U * sizeof(uint64_t);
    neighbor_alias_vaddr = target_alias_vaddr + 5U * sizeof(uint64_t);
    tail_alias_vaddr =
        target_alias_vaddr + (AI_MICRO_DIFFSYNC_BLOCK_WORDS - 1U) *
        sizeof(uint64_t);
    adjacent_alias_vaddr =
        target_alias_vaddr + AI_MICRO_DIFFSYNC_BLOCK_BYTES;
    marker_alias_vaddr =
        target_alias_vaddr + 3U * AI_MICRO_DIFFSYNC_BLOCK_BYTES;
    marker_guard_alias_vaddr =
        marker_alias_vaddr + sizeof(uint64_t);

    for (size_t round = 0; round < sizeof(block_seeds) / sizeof(block_seeds[0]);
         ++round) {
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
        AI_ASSERT_NO_EXCEPT_AND("before the same-paddr-alias cbo.inval + base-view prefetch_r + alias-view prior-store + LR/SC window starts, both target views, adjacent block, and marker are readable with seeded values",
            ld(target_base_vaddr) == block_seeds[round] &&
            ld(target_alias_vaddr) == block_seeds[round] &&
            ld(tail_base_vaddr) == expected_tail &&
            ld(tail_alias_vaddr) == expected_tail &&
            ld(adjacent_alias_vaddr) == adjacent_seeds[round] &&
            ld(marker_base_vaddr) == marker_seeds[round]
        );

        TEST_SETUP_EXCEPT();
        ai_micro_diffsync_cbo_inval_prefetch_store_lrsc_window(
            inval_alias_vaddr, prefetch_base_vaddr, target_alias_vaddr,
            target_alias_vaddr, neighbor_base_vaddr, marker_base_vaddr,
            prior_stores[round], sc_payloads[round],
            neighbor_updates[round], marker_updates[round], &lr_observed,
            &sc_status, &marker_observed);
        AI_ASSERT_NO_EXCEPT_AND("cbo.inval through a same-paddr alias followed by base-view prefetch_r must not let a refill image bypass the alias-view prior store observed by LR/SC",
            lr_observed == prior_stores[round] &&
            sc_status == 0ULL &&
            marker_observed == marker_updates[round] &&
            ld(target_base_vaddr) == sc_payloads[round] &&
            ld(target_alias_vaddr) == sc_payloads[round] &&
            ld(neighbor_base_vaddr) == neighbor_updates[round] &&
            ld(neighbor_alias_vaddr) == neighbor_updates[round] &&
            ld(tail_base_vaddr) == expected_tail &&
            ld(tail_alias_vaddr) == expected_tail &&
            ld(marker_alias_vaddr) == marker_updates[round]
        );

        TEST_SETUP_EXCEPT();
        AI_ASSERT_NO_EXCEPT_AND("the same-paddr-alias refill plus prior-store LR/SC reservation window preserves target guards, marker guard, and the adjacent cache block through both views",
            ld(before_guard_base_vaddr) == before_guards[round] &&
            ld(after_guard_base_vaddr) == after_guards[round] &&
            ld(marker_guard_base_vaddr) == marker_guards[round] &&
            ld(marker_guard_alias_vaddr) == marker_guards[round] &&
            ld(adjacent_alias_vaddr) == adjacent_seeds[round]
        );

        goto_priv(PRIV_M);
        TEST_ASSERT("backing memory after the same-paddr-alias cbo.inval + prefetch_r + prior-store + LR/SC window contains only the successful SC payload, neighbor store, marker update, and unchanged guards",
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

    ai_micro_diffsync_reset_env(MENVCFG_CBIE01);

    TEST_END();
}

bool ai_micro_hs_cbo_inval_prefetch_same_paddr_alias_refill_then_prior_store_amo_no_stale_old_value_or_lost_update_corner()
{
    TEST_START();

    const enum test_page base_page = SWITCH1;
    const enum test_page alias_page = SWITCH2;
    const uintptr_t target_base_vaddr = hs_page_base(base_page) + 0x980U;
    const uintptr_t target_base_paddr = phys_page_base(base_page) + 0x980U;
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
        0x1029384756abcdefULL,
        0x21436587a9cbed0fULL,
        0x89abcdef76543210ULL,
    };
    const uint64_t adjacent_seeds[] = {
        0x0123456789abcdefULL,
        0xfedcba9876543210ULL,
        0x55aa7733cc11ee99ULL,
    };
    const uint64_t prior_stores[] = {
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
        0xa5a55a5af0f00f0fULL,
        0x0102030405060708ULL,
        0xfdb97531eca86420ULL,
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
    uintptr_t neighbor_alias_vaddr = 0;
    uintptr_t tail_alias_vaddr = 0;
    uintptr_t adjacent_alias_vaddr = 0;
    uintptr_t marker_alias_vaddr = 0;
    uintptr_t marker_guard_alias_vaddr = 0;

    ai_micro_diffsync_prepare_hs_env(MENVCFG_CBIE01);
    alias_page_base_vaddr =
        ai_micro_diffsync_prepare_same_paddr_alias(alias_page, base_page);
    target_alias_vaddr = alias_page_base_vaddr + 0x980U;
    inval_alias_vaddr = target_alias_vaddr + 4U * sizeof(uint64_t);
    neighbor_alias_vaddr = target_alias_vaddr + 5U * sizeof(uint64_t);
    tail_alias_vaddr =
        target_alias_vaddr + (AI_MICRO_DIFFSYNC_BLOCK_WORDS - 1U) *
        sizeof(uint64_t);
    adjacent_alias_vaddr =
        target_alias_vaddr + AI_MICRO_DIFFSYNC_BLOCK_BYTES;
    marker_alias_vaddr =
        target_alias_vaddr + 3U * AI_MICRO_DIFFSYNC_BLOCK_BYTES;
    marker_guard_alias_vaddr =
        marker_alias_vaddr + sizeof(uint64_t);

    for (size_t round = 0; round < sizeof(block_seeds) / sizeof(block_seeds[0]);
         ++round) {
        const uint64_t after_first_amo =
            prior_stores[round] + first_addends[round];
        const uint64_t final_target =
            after_first_amo + second_addends[round];
        const uint64_t expected_tail =
            ai_micro_diffsync_seed_word(block_seeds[round],
                AI_MICRO_DIFFSYNC_BLOCK_WORDS - 1U);
        uint64_t first_amo_old = 0ULL;
        uint64_t second_amo_old = 0ULL;
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
        AI_ASSERT_NO_EXCEPT_AND("before the same-paddr-alias cbo.inval + base-view prefetch_r + alias-view prior-store + AMO window starts, both target views, adjacent block, and marker are readable with seeded values",
            ld(target_base_vaddr) == block_seeds[round] &&
            ld(target_alias_vaddr) == block_seeds[round] &&
            ld(tail_base_vaddr) == expected_tail &&
            ld(tail_alias_vaddr) == expected_tail &&
            ld(adjacent_alias_vaddr) == adjacent_seeds[round] &&
            ld(marker_base_vaddr) == marker_seeds[round]
        );

        TEST_SETUP_EXCEPT();
        ai_micro_diffsync_cbo_inval_prefetch_store_amo_window(
            inval_alias_vaddr, prefetch_base_vaddr, target_alias_vaddr,
            target_alias_vaddr, neighbor_base_vaddr, marker_base_vaddr,
            prior_stores[round], first_addends[round],
            second_addends[round], neighbor_updates[round],
            marker_updates[round], &first_amo_old, &second_amo_old,
            &marker_observed);
        AI_ASSERT_NO_EXCEPT_AND("cbo.inval through a same-paddr alias followed by base-view prefetch_r must not let a refill image bypass the alias-view prior store observed by AMO old values",
            first_amo_old == prior_stores[round] &&
            second_amo_old == after_first_amo &&
            marker_observed == marker_updates[round] &&
            ld(target_base_vaddr) == final_target &&
            ld(target_alias_vaddr) == final_target &&
            ld(neighbor_base_vaddr) == neighbor_updates[round] &&
            ld(neighbor_alias_vaddr) == neighbor_updates[round] &&
            ld(tail_base_vaddr) == expected_tail &&
            ld(tail_alias_vaddr) == expected_tail &&
            ld(marker_alias_vaddr) == marker_updates[round]
        );

        TEST_SETUP_EXCEPT();
        AI_ASSERT_NO_EXCEPT_AND("the same-paddr-alias refill plus prior-store AMO window preserves target guards, marker guard, and the adjacent cache block through both views",
            ld(before_guard_base_vaddr) == before_guards[round] &&
            ld(after_guard_base_vaddr) == after_guards[round] &&
            ld(marker_guard_base_vaddr) == marker_guards[round] &&
            ld(marker_guard_alias_vaddr) == marker_guards[round] &&
            ld(adjacent_alias_vaddr) == adjacent_seeds[round]
        );

        goto_priv(PRIV_M);
        TEST_ASSERT("backing memory after the same-paddr-alias cbo.inval + prefetch_r + prior-store + AMO window contains only the AMO result, neighbor store, marker update, and unchanged guards",
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

    ai_micro_diffsync_reset_env(MENVCFG_CBIE01);

    TEST_END();
}

bool ai_micro_hs_cbo_inval_prefetch_refill_then_prior_store_amo_no_stale_old_value_or_lost_update_corner()
{
    TEST_START();

    const enum test_page target_page = SWITCH1;
    const enum test_page marker_page = SWITCH2;
    const uintptr_t target_base_vaddr = hs_page_base(target_page) + 0xa00U;
    const uintptr_t target_base_paddr = phys_page_base(target_page) + 0xa00U;
    const uintptr_t store_vaddr = target_base_vaddr;
    const uintptr_t amo_vaddr = target_base_vaddr;
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
    const uintptr_t before_guard_paddr = target_base_paddr - sizeof(uint64_t);
    const uintptr_t after_guard_paddr =
        target_base_paddr + 2U * AI_MICRO_DIFFSYNC_BLOCK_BYTES;
    const uintptr_t adjacent_base_vaddr =
        target_base_vaddr + AI_MICRO_DIFFSYNC_BLOCK_BYTES;
    const uintptr_t adjacent_base_paddr =
        target_base_paddr + AI_MICRO_DIFFSYNC_BLOCK_BYTES;
    const uintptr_t marker_vaddr = hs_page_base(marker_page) + 0xa00U;
    const uintptr_t marker_paddr = phys_page_base(marker_page) + 0xa00U;
    const uintptr_t marker_guard_vaddr = marker_vaddr + sizeof(uint64_t);
    const uintptr_t marker_guard_paddr = marker_paddr + sizeof(uint64_t);
    const uint64_t block_seeds[] = {
        0x7766554433221100ULL,
        0x123456789abcdef0ULL,
        0x0f1e2d3c4b5a6978ULL,
    };
    const uint64_t adjacent_seeds[] = {
        0xfedcba9876543210ULL,
        0x55aa7733cc11ee99ULL,
        0x0123456789abcdefULL,
    };
    const uint64_t prior_stores[] = {
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
        0xa5a55a5af0f00f0fULL,
        0x0102030405060708ULL,
        0xfdb97531eca86420ULL,
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
        0x13579bdf2468ace0ULL,
        0x02468ace13579bdfULL,
        0xfdb97531eca86420ULL,
    };
    const uint64_t marker_guards[] = {
        0x3141592653589793ULL,
        0x2718281828459045ULL,
        0x1123581321345589ULL,
    };

    ai_micro_diffsync_prepare_hs_env(MENVCFG_CBIE01);

    for (size_t round = 0; round < sizeof(block_seeds) / sizeof(block_seeds[0]);
         ++round) {
        const uint64_t after_first_amo =
            prior_stores[round] + first_addends[round];
        const uint64_t final_target =
            after_first_amo + second_addends[round];
        const uint64_t expected_tail =
            ai_micro_diffsync_seed_word(block_seeds[round],
                AI_MICRO_DIFFSYNC_BLOCK_WORDS - 1U);
        uint64_t first_amo_old = 0ULL;
        uint64_t second_amo_old = 0ULL;
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
        AI_ASSERT_NO_EXCEPT_AND("before the cbo.inval + prefetch_r + prior-store + AMO window starts, target block, adjacent block, and marker are readable with seeded values",
            ld(amo_vaddr) == block_seeds[round] &&
            ld(tail_vaddr) == expected_tail &&
            ld(adjacent_base_vaddr) == adjacent_seeds[round] &&
            ld(marker_vaddr) == marker_seeds[round]
        );

        TEST_SETUP_EXCEPT();
        ai_micro_diffsync_cbo_inval_prefetch_store_amo_window(
            inval_vaddr, prefetch_vaddr, store_vaddr, amo_vaddr,
            neighbor_vaddr, marker_vaddr, prior_stores[round],
            first_addends[round], second_addends[round],
            neighbor_updates[round], marker_updates[round], &first_amo_old,
            &second_amo_old, &marker_observed);
        AI_ASSERT_NO_EXCEPT_AND("cbo.inval plus same-block prefetch_r must not let a refill image bypass the immediately younger prior store observed by AMO old values",
            first_amo_old == prior_stores[round] &&
            second_amo_old == after_first_amo &&
            marker_observed == marker_updates[round] &&
            ld(amo_vaddr) == final_target &&
            ld(neighbor_vaddr) == neighbor_updates[round] &&
            ld(tail_vaddr) == expected_tail &&
            ld(marker_vaddr) == marker_updates[round]
        );

        TEST_SETUP_EXCEPT();
        AI_ASSERT_NO_EXCEPT_AND("the cbo.inval + prefetch_r + prior-store + AMO window preserves target guards, marker guard, and the adjacent cache block",
            ld(before_guard_vaddr) == before_guards[round] &&
            ld(after_guard_vaddr) == after_guards[round] &&
            ld(marker_guard_vaddr) == marker_guards[round] &&
            ld(adjacent_base_vaddr) == adjacent_seeds[round]
        );

        goto_priv(PRIV_M);
        TEST_ASSERT("backing memory after the cbo.inval + prefetch_r + prior-store + AMO window contains only the final AMO result, neighbor store, marker update, and unchanged guards",
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

    ai_micro_diffsync_reset_env(MENVCFG_CBIE01);

    TEST_END();
}

bool ai_micro_hs_cbo_inval_prefetch_refill_then_prior_store_partial_forward_merge_no_stale_bytes_corner()
{
    TEST_START();

    const enum test_page target_page = SWITCH1;
    const enum test_page marker_page = SWITCH2;
    const uintptr_t target_base_vaddr = hs_page_base(target_page) + 0xa80U;
    const uintptr_t target_base_paddr = phys_page_base(target_page) + 0xa80U;
    const uintptr_t word_vaddr = target_base_vaddr;
    const uintptr_t byte_store_vaddr = target_base_vaddr + 1U;
    const uintptr_t half_store_vaddr = target_base_vaddr + 4U;
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
    const uintptr_t before_guard_paddr = target_base_paddr - sizeof(uint64_t);
    const uintptr_t after_guard_paddr =
        target_base_paddr + 2U * AI_MICRO_DIFFSYNC_BLOCK_BYTES;
    const uintptr_t adjacent_base_vaddr =
        target_base_vaddr + AI_MICRO_DIFFSYNC_BLOCK_BYTES;
    const uintptr_t adjacent_base_paddr =
        target_base_paddr + AI_MICRO_DIFFSYNC_BLOCK_BYTES;
    const uintptr_t marker_vaddr = hs_page_base(marker_page) + 0xa80U;
    const uintptr_t marker_paddr = phys_page_base(marker_page) + 0xa80U;
    const uintptr_t marker_guard_vaddr = marker_vaddr + sizeof(uint64_t);
    const uintptr_t marker_guard_paddr = marker_paddr + sizeof(uint64_t);
    const uint64_t block_seeds[] = {
        0x76543210fedcba98ULL,
        0x89abcdef01234567ULL,
        0x5a5aa5a55a5aa5a5ULL,
    };
    const uint64_t adjacent_seeds[] = {
        0xfedcba9876543210ULL,
        0x0123456789abcdefULL,
        0x55aa7733cc11ee99ULL,
    };
    const uint64_t prior_stores[] = {
        0x1111222233334444ULL,
        0x5555666677778888ULL,
        0x9999aaaabbbbccccULL,
    };
    const uint8_t byte_updates[] = {
        0x6aU,
        0xc4U,
        0x2eU,
    };
    const uint16_t halfword_updates[] = {
        0x7b8cU,
        0xd5e6U,
        0x9b0cU,
    };
    const uint64_t neighbor_updates[] = {
        0xa5a55a5af0f00f0fULL,
        0x0102030405060708ULL,
        0xfdb97531eca86420ULL,
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
        0x13579bdf2468ace0ULL,
        0x02468ace13579bdfULL,
        0xfdb97531eca86420ULL,
    };
    const uint64_t marker_guards[] = {
        0x3141592653589793ULL,
        0x2718281828459045ULL,
        0x1123581321345589ULL,
    };

    ai_micro_diffsync_prepare_hs_env(MENVCFG_CBIE01);

    for (size_t round = 0; round < sizeof(block_seeds) / sizeof(block_seeds[0]);
         ++round) {
        const uint64_t expected_word =
            ai_micro_diffsync_merge_byte_halfword_into_word(
                prior_stores[round],
                byte_updates[round],
                halfword_updates[round]);
        const uint64_t expected_tail =
            ai_micro_diffsync_seed_word(block_seeds[round],
                AI_MICRO_DIFFSYNC_BLOCK_WORDS - 1U);
        uint64_t word_observed = 0ULL;
        uint64_t byte_observed = 0ULL;
        uint64_t half_observed = 0ULL;
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
        AI_ASSERT_NO_EXCEPT_AND("before the cbo.inval + prefetch_r + prior-store + partial-store merge window starts, target block, adjacent block, and marker are readable with seeded values",
            ld(word_vaddr) == block_seeds[round] &&
            ld(tail_vaddr) == expected_tail &&
            ld(adjacent_base_vaddr) == adjacent_seeds[round] &&
            ld(marker_vaddr) == marker_seeds[round]
        );

        TEST_SETUP_EXCEPT();
        ai_micro_diffsync_cbo_inval_prefetch_prior_store_partial_window(
            inval_vaddr, prefetch_vaddr, word_vaddr, byte_store_vaddr,
            half_store_vaddr, neighbor_vaddr, marker_vaddr,
            prior_stores[round], byte_updates[round],
            halfword_updates[round], neighbor_updates[round],
            marker_updates[round], &word_observed, &byte_observed,
            &half_observed, &marker_observed);
        AI_ASSERT_NO_EXCEPT_AND("cbo.inval plus same-block prefetch_r must not let a refill image bypass the prior full store or younger byte/halfword stores observed by immediate mixed loads",
            word_observed == expected_word &&
            byte_observed == byte_updates[round] &&
            half_observed == halfword_updates[round] &&
            marker_observed == marker_updates[round] &&
            ld(word_vaddr) == expected_word &&
            ld(neighbor_vaddr) == neighbor_updates[round] &&
            ld(tail_vaddr) == expected_tail &&
            ld(marker_vaddr) == marker_updates[round]
        );

        TEST_SETUP_EXCEPT();
        AI_ASSERT_NO_EXCEPT_AND("the cbo.inval + prefetch_r + prior-store + partial-store merge window preserves target guards, marker guard, and the adjacent cache block",
            ld(before_guard_vaddr) == before_guards[round] &&
            ld(after_guard_vaddr) == after_guards[round] &&
            ld(marker_guard_vaddr) == marker_guards[round] &&
            ld(adjacent_base_vaddr) == adjacent_seeds[round]
        );

        goto_priv(PRIV_M);
        TEST_ASSERT("backing memory after the cbo.inval + prefetch_r + prior-store + partial-store merge window contains only the layered byte-age merged word, neighbor store, marker update, and unchanged guards",
            ai_micro_diffsync_block_matches_seed_except_two_words(
                target_base_paddr,
                block_seeds[round],
                0U,
                expected_word,
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

bool ai_micro_hs_cbo_inval_prefetch_same_paddr_alias_refill_then_prior_store_partial_forward_merge_no_stale_bytes_corner()
{
    TEST_START();

    const enum test_page base_page = SWITCH1;
    const enum test_page alias_page = SWITCH2;
    const uintptr_t target_base_vaddr = hs_page_base(base_page) + 0xb00U;
    const uintptr_t target_base_paddr = phys_page_base(base_page) + 0xb00U;
    const uintptr_t word_base_vaddr = target_base_vaddr;
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
        0x0d1c2b3a49586776ULL,
        0x89abcdef01234567ULL,
        0x5a5aa5a55a5aa5a5ULL,
    };
    const uint64_t adjacent_seeds[] = {
        0xfedcba9876543210ULL,
        0x0123456789abcdefULL,
        0x55aa7733cc11ee99ULL,
    };
    const uint64_t prior_stores[] = {
        0x1011223344556677ULL,
        0x8899aabbccddeeffULL,
        0x0123456789abcdefULL,
    };
    const uint8_t byte_updates[] = {
        0x71U,
        0xb2U,
        0x3dU,
    };
    const uint16_t halfword_updates[] = {
        0x4567U,
        0x89abU,
        0xcdefU,
    };
    const uint64_t neighbor_updates[] = {
        0xa5a55a5af0f00f0fULL,
        0x0102030405060708ULL,
        0xfdb97531eca86420ULL,
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
        0x0123456789abcdefULL,
        0xfedcba9876543210ULL,
        0x55aa7733cc11ee99ULL,
    };
    uintptr_t alias_page_base_vaddr = 0;
    uintptr_t target_alias_vaddr = 0;
    uintptr_t byte_store_alias_vaddr = 0;
    uintptr_t half_store_alias_vaddr = 0;
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
    target_alias_vaddr = alias_page_base_vaddr + 0xb00U;
    byte_store_alias_vaddr = target_alias_vaddr + 1U;
    half_store_alias_vaddr = target_alias_vaddr + 4U;
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
        const uint64_t expected_word =
            ai_micro_diffsync_merge_byte_halfword_into_word(
                prior_stores[round],
                byte_updates[round],
                halfword_updates[round]);
        const uint64_t expected_tail =
            ai_micro_diffsync_seed_word(block_seeds[round],
                AI_MICRO_DIFFSYNC_BLOCK_WORDS - 1U);
        uint64_t word_observed = 0ULL;
        uint64_t byte_observed = 0ULL;
        uint64_t half_observed = 0ULL;
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
        AI_ASSERT_NO_EXCEPT_AND("before the same-paddr-alias cbo.inval + base-view prefetch_r + alias-view prior-store + partial-store merge window starts, both target views, adjacent block, and marker are readable with seeded values",
            ld(word_base_vaddr) == block_seeds[round] &&
            ld(target_alias_vaddr) == block_seeds[round] &&
            ld(tail_base_vaddr) == expected_tail &&
            ld(tail_alias_vaddr) == expected_tail &&
            ld(adjacent_base_vaddr) == adjacent_seeds[round] &&
            ld(adjacent_alias_vaddr) == adjacent_seeds[round] &&
            ld(marker_base_vaddr) == marker_seeds[round] &&
            ld(marker_alias_vaddr) == marker_seeds[round]
        );

        TEST_SETUP_EXCEPT();
        ai_micro_diffsync_cbo_inval_prefetch_prior_store_partial_window(
            inval_alias_vaddr, prefetch_base_vaddr, target_alias_vaddr,
            byte_store_alias_vaddr, half_store_alias_vaddr,
            neighbor_base_vaddr, marker_base_vaddr, prior_stores[round],
            byte_updates[round], halfword_updates[round],
            neighbor_updates[round], marker_updates[round],
            &word_observed, &byte_observed, &half_observed,
            &marker_observed);
        AI_ASSERT_NO_EXCEPT_AND("cbo.inval through a same-paddr alias followed by base-view prefetch_r must not let a refill image bypass the alias-view prior full store or younger byte/halfword stores observed by mixed loads",
            word_observed == expected_word &&
            byte_observed == byte_updates[round] &&
            half_observed == halfword_updates[round] &&
            marker_observed == marker_updates[round] &&
            ld(word_base_vaddr) == expected_word &&
            ld(target_alias_vaddr) == expected_word &&
            ld(neighbor_base_vaddr) == neighbor_updates[round] &&
            ld(neighbor_alias_vaddr) == neighbor_updates[round] &&
            ld(tail_base_vaddr) == expected_tail &&
            ld(tail_alias_vaddr) == expected_tail &&
            ld(marker_base_vaddr) == marker_updates[round] &&
            ld(marker_alias_vaddr) == marker_updates[round]
        );

        TEST_SETUP_EXCEPT();
        AI_ASSERT_NO_EXCEPT_AND("the same-paddr-alias refill plus prior-store partial-merge window preserves guards and the adjacent cache block through both views",
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
        TEST_ASSERT("backing memory after the same-paddr-alias cbo.inval + prefetch_r + prior-store + partial-store merge window contains only the layered byte-age merged word, neighbor store, marker update, and unchanged guards",
            ai_micro_diffsync_block_matches_seed_except_two_words(
                target_base_paddr,
                block_seeds[round],
                0U,
                expected_word,
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

bool ai_micro_hs_cbo_inval_prefetch_refill_then_prior_store_partial_amo_no_stale_merged_old_value_or_lost_update_corner()
{
    TEST_START();

    const enum test_page target_page = SWITCH1;
    const enum test_page marker_page = SWITCH2;
    const uintptr_t target_base_vaddr = hs_page_base(target_page) + 0xb80U;
    const uintptr_t target_base_paddr = phys_page_base(target_page) + 0xb80U;
    const uintptr_t word_vaddr = target_base_vaddr;
    const uintptr_t byte_store_vaddr = target_base_vaddr + 1U;
    const uintptr_t half_store_vaddr = target_base_vaddr + 4U;
    const uintptr_t amo_vaddr = target_base_vaddr;
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
    const uintptr_t before_guard_paddr = target_base_paddr - sizeof(uint64_t);
    const uintptr_t after_guard_paddr =
        target_base_paddr + 2U * AI_MICRO_DIFFSYNC_BLOCK_BYTES;
    const uintptr_t adjacent_base_vaddr =
        target_base_vaddr + AI_MICRO_DIFFSYNC_BLOCK_BYTES;
    const uintptr_t adjacent_base_paddr =
        target_base_paddr + AI_MICRO_DIFFSYNC_BLOCK_BYTES;
    const uintptr_t marker_vaddr = hs_page_base(marker_page) + 0xb80U;
    const uintptr_t marker_paddr = phys_page_base(marker_page) + 0xb80U;
    const uintptr_t marker_guard_vaddr = marker_vaddr + sizeof(uint64_t);
    const uintptr_t marker_guard_paddr = marker_paddr + sizeof(uint64_t);
    const uint64_t block_seeds[] = {
        0x33221100ffeeddccULL,
        0x89abcdef01234567ULL,
        0x5a5aa5a55a5aa5a5ULL,
    };
    const uint64_t adjacent_seeds[] = {
        0xfedcba9876543210ULL,
        0x0123456789abcdefULL,
        0x55aa7733cc11ee99ULL,
    };
    const uint64_t prior_stores[] = {
        0x1111222233334444ULL,
        0x5555666677778888ULL,
        0x9999aaaabbbbccccULL,
    };
    const uint8_t byte_updates[] = {
        0x68U,
        0xb7U,
        0x4cU,
    };
    const uint16_t halfword_updates[] = {
        0x1357U,
        0x2468U,
        0x9bdfU,
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
        0xa5a55a5af0f00f0fULL,
        0x0102030405060708ULL,
        0xfdb97531eca86420ULL,
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
        0x0123456789abcdefULL,
        0xfedcba9876543210ULL,
        0x55aa7733cc11ee99ULL,
    };

    ai_micro_diffsync_prepare_hs_env(MENVCFG_CBIE01);

    for (size_t round = 0; round < sizeof(block_seeds) / sizeof(block_seeds[0]);
         ++round) {
        const uint64_t expected_merged =
            ai_micro_diffsync_merge_byte_halfword_into_word(
                prior_stores[round],
                byte_updates[round],
                halfword_updates[round]);
        const uint64_t after_first_amo =
            expected_merged + first_addends[round];
        const uint64_t final_target =
            after_first_amo + second_addends[round];
        const uint64_t expected_tail =
            ai_micro_diffsync_seed_word(block_seeds[round],
                AI_MICRO_DIFFSYNC_BLOCK_WORDS - 1U);
        uint64_t first_amo_old = 0ULL;
        uint64_t second_amo_old = 0ULL;
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
        AI_ASSERT_NO_EXCEPT_AND("before the cbo.inval + prefetch_r + prior-store + partial-store + AMO window starts, target block, adjacent block, and marker are readable with seeded values",
            ld(amo_vaddr) == block_seeds[round] &&
            ld(tail_vaddr) == expected_tail &&
            ld(adjacent_base_vaddr) == adjacent_seeds[round] &&
            ld(marker_vaddr) == marker_seeds[round]
        );

        TEST_SETUP_EXCEPT();
        ai_micro_diffsync_cbo_inval_prefetch_prior_store_partial_amo_window(
            inval_vaddr, prefetch_vaddr, word_vaddr, byte_store_vaddr,
            half_store_vaddr, amo_vaddr, neighbor_vaddr, marker_vaddr,
            prior_stores[round], byte_updates[round],
            halfword_updates[round], first_addends[round],
            second_addends[round], neighbor_updates[round],
            marker_updates[round], &first_amo_old, &second_amo_old,
            &marker_observed);
        AI_ASSERT_NO_EXCEPT_AND("cbo.inval plus same-block prefetch_r must not let a refill image bypass the prior full store plus younger byte/halfword stores consumed by AMO old values",
            first_amo_old == expected_merged &&
            second_amo_old == after_first_amo &&
            marker_observed == marker_updates[round] &&
            ld(amo_vaddr) == final_target &&
            ld(neighbor_vaddr) == neighbor_updates[round] &&
            ld(tail_vaddr) == expected_tail &&
            ld(marker_vaddr) == marker_updates[round]
        );

        TEST_SETUP_EXCEPT();
        AI_ASSERT_NO_EXCEPT_AND("the cbo.inval + prefetch_r + prior-store + partial-store + AMO window preserves target guards, marker guard, and the adjacent cache block",
            ld(before_guard_vaddr) == before_guards[round] &&
            ld(after_guard_vaddr) == after_guards[round] &&
            ld(marker_guard_vaddr) == marker_guards[round] &&
            ld(adjacent_base_vaddr) == adjacent_seeds[round]
        );

        goto_priv(PRIV_M);
        TEST_ASSERT("backing memory after the cbo.inval + prefetch_r + prior-store + partial-store + AMO window contains only the final AMO result, neighbor store, marker update, and unchanged guards",
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

    ai_micro_diffsync_reset_env(MENVCFG_CBIE01);

    TEST_END();
}

bool ai_micro_hs_cbo_inval_prefetch_refill_then_prior_store_partial_lrsc_no_stale_merged_old_value_or_reservation_loss_corner()
{
    TEST_START();

    const enum test_page target_page = SWITCH1;
    const enum test_page marker_page = SWITCH2;
    const uintptr_t target_base_vaddr = hs_page_base(target_page) + 0xd80U;
    const uintptr_t target_base_paddr = phys_page_base(target_page) + 0xd80U;
    const uintptr_t word_vaddr = target_base_vaddr;
    const uintptr_t byte_store_vaddr = target_base_vaddr + 1U;
    const uintptr_t half_store_vaddr = target_base_vaddr + 4U;
    const uintptr_t lrsc_vaddr = target_base_vaddr;
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
    const uintptr_t before_guard_paddr = target_base_paddr - sizeof(uint64_t);
    const uintptr_t after_guard_paddr =
        target_base_paddr + 2U * AI_MICRO_DIFFSYNC_BLOCK_BYTES;
    const uintptr_t adjacent_base_vaddr =
        target_base_vaddr + AI_MICRO_DIFFSYNC_BLOCK_BYTES;
    const uintptr_t adjacent_base_paddr =
        target_base_paddr + AI_MICRO_DIFFSYNC_BLOCK_BYTES;
    const uintptr_t marker_vaddr = hs_page_base(marker_page) + 0xd80U;
    const uintptr_t marker_paddr = phys_page_base(marker_page) + 0xd80U;
    const uintptr_t marker_guard_vaddr = marker_vaddr + sizeof(uint64_t);
    const uintptr_t marker_guard_paddr = marker_paddr + sizeof(uint64_t);
    const uint64_t block_seeds[] = {
        0x21436587a9cbed0fULL,
        0x89abcdef01234567ULL,
        0x5a5aa5a55a5aa5a5ULL,
    };
    const uint64_t adjacent_seeds[] = {
        0xfedcba9876543210ULL,
        0x0123456789abcdefULL,
        0x55aa7733cc11ee99ULL,
    };
    const uint64_t prior_stores[] = {
        0x1111222233334444ULL,
        0x5555666677778888ULL,
        0x9999aaaabbbbccccULL,
    };
    const uint8_t byte_updates[] = {
        0x52U,
        0xa9U,
        0x3eU,
    };
    const uint16_t halfword_updates[] = {
        0x2468U,
        0xace0U,
        0x1357U,
    };
    const uint64_t sc_payloads[] = {
        0x0102030405060708ULL,
        0x1827364554637281ULL,
        0xfdb97531eca86420ULL,
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
        0x13579bdf2468ace0ULL,
        0x02468ace13579bdfULL,
        0xfdb97531eca86420ULL,
    };
    const uint64_t marker_guards[] = {
        0x3141592653589793ULL,
        0x2718281828459045ULL,
        0x1123581321345589ULL,
    };

    ai_micro_diffsync_prepare_hs_env(MENVCFG_CBIE01);

    for (size_t round = 0; round < sizeof(block_seeds) / sizeof(block_seeds[0]);
         ++round) {
        const uint64_t expected_merged =
            ai_micro_diffsync_merge_byte_halfword_into_word(
                prior_stores[round],
                byte_updates[round],
                halfword_updates[round]);
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
        AI_ASSERT_NO_EXCEPT_AND("before the cbo.inval + prefetch_r + prior-store + partial-store + LR/SC window starts, target block, adjacent block, and marker are readable with seeded values",
            ld(lrsc_vaddr) == block_seeds[round] &&
            ld(tail_vaddr) == expected_tail &&
            ld(adjacent_base_vaddr) == adjacent_seeds[round] &&
            ld(marker_vaddr) == marker_seeds[round]
        );

        TEST_SETUP_EXCEPT();
        ai_micro_diffsync_cbo_inval_prefetch_prior_store_partial_lrsc_window(
            inval_vaddr, prefetch_vaddr, word_vaddr, byte_store_vaddr,
            half_store_vaddr, lrsc_vaddr, neighbor_vaddr, marker_vaddr,
            prior_stores[round], byte_updates[round],
            halfword_updates[round], sc_payloads[round],
            neighbor_updates[round], marker_updates[round],
            &lr_observed, &sc_status, &marker_observed);
        AI_ASSERT_NO_EXCEPT_AND("cbo.inval plus same-block prefetch_r must not let a refill image bypass the prior full store plus younger byte/halfword stores consumed by LR/SC",
            lr_observed == expected_merged &&
            sc_status == 0ULL &&
            marker_observed == marker_updates[round] &&
            ld(lrsc_vaddr) == sc_payloads[round] &&
            ld(neighbor_vaddr) == neighbor_updates[round] &&
            ld(tail_vaddr) == expected_tail &&
            ld(marker_vaddr) == marker_updates[round]
        );

        TEST_SETUP_EXCEPT();
        AI_ASSERT_NO_EXCEPT_AND("the cbo.inval + prefetch_r + prior-store + partial-store + LR/SC window preserves target guards, marker guard, and the adjacent cache block",
            ld(before_guard_vaddr) == before_guards[round] &&
            ld(after_guard_vaddr) == after_guards[round] &&
            ld(marker_guard_vaddr) == marker_guards[round] &&
            ld(adjacent_base_vaddr) == adjacent_seeds[round]
        );

        goto_priv(PRIV_M);
        TEST_ASSERT("backing memory after the cbo.inval + prefetch_r + prior-store + partial-store + LR/SC window contains only the successful SC payload, neighbor store, marker update, and unchanged guards",
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

    ai_micro_diffsync_reset_env(MENVCFG_CBIE01);

    TEST_END();
}

bool ai_micro_hs_cbo_inval_prefetch_same_paddr_alias_refill_then_prior_store_partial_amo_no_stale_merged_old_value_or_lost_update_corner()
{
    TEST_START();

    const enum test_page base_page = SWITCH1;
    const enum test_page alias_page = SWITCH2;
    const uintptr_t target_base_vaddr = hs_page_base(base_page) + 0xc00U;
    const uintptr_t target_base_paddr = phys_page_base(base_page) + 0xc00U;
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
        0x4433221100ffeeddULL,
        0x89abcdef01234567ULL,
        0x5a5aa5a55a5aa5a5ULL,
    };
    const uint64_t adjacent_seeds[] = {
        0xfedcba9876543210ULL,
        0x0123456789abcdefULL,
        0x55aa7733cc11ee99ULL,
    };
    const uint64_t prior_stores[] = {
        0x1111222233334444ULL,
        0x5555666677778888ULL,
        0x9999aaaabbbbccccULL,
    };
    const uint8_t byte_updates[] = {
        0x7dU,
        0xa6U,
        0x59U,
    };
    const uint16_t halfword_updates[] = {
        0x2244U,
        0x6688U,
        0xaaccU,
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
        0xa5a55a5af0f00f0fULL,
        0x0102030405060708ULL,
        0xfdb97531eca86420ULL,
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
        0x0123456789abcdefULL,
        0xfedcba9876543210ULL,
        0x55aa7733cc11ee99ULL,
    };
    uintptr_t alias_page_base_vaddr = 0;
    uintptr_t target_alias_vaddr = 0;
    uintptr_t byte_store_alias_vaddr = 0;
    uintptr_t half_store_alias_vaddr = 0;
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
    target_alias_vaddr = alias_page_base_vaddr + 0xc00U;
    byte_store_alias_vaddr = target_alias_vaddr + 1U;
    half_store_alias_vaddr = target_alias_vaddr + 4U;
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
        const uint64_t expected_merged =
            ai_micro_diffsync_merge_byte_halfword_into_word(
                prior_stores[round],
                byte_updates[round],
                halfword_updates[round]);
        const uint64_t after_first_amo =
            expected_merged + first_addends[round];
        const uint64_t final_target =
            after_first_amo + second_addends[round];
        const uint64_t expected_tail =
            ai_micro_diffsync_seed_word(block_seeds[round],
                AI_MICRO_DIFFSYNC_BLOCK_WORDS - 1U);
        uint64_t first_amo_old = 0ULL;
        uint64_t second_amo_old = 0ULL;
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
        AI_ASSERT_NO_EXCEPT_AND("before the same-paddr-alias cbo.inval + base-view prefetch_r + alias-view prior-store + partial-store + AMO window starts, both target views, adjacent block, and marker are readable with seeded values",
            ld(target_base_vaddr) == block_seeds[round] &&
            ld(target_alias_vaddr) == block_seeds[round] &&
            ld(tail_base_vaddr) == expected_tail &&
            ld(tail_alias_vaddr) == expected_tail &&
            ld(adjacent_base_vaddr) == adjacent_seeds[round] &&
            ld(adjacent_alias_vaddr) == adjacent_seeds[round] &&
            ld(marker_base_vaddr) == marker_seeds[round] &&
            ld(marker_alias_vaddr) == marker_seeds[round]
        );

        TEST_SETUP_EXCEPT();
        ai_micro_diffsync_cbo_inval_prefetch_prior_store_partial_amo_window(
            inval_alias_vaddr, prefetch_base_vaddr, target_alias_vaddr,
            byte_store_alias_vaddr, half_store_alias_vaddr,
            target_alias_vaddr, neighbor_base_vaddr, marker_base_vaddr,
            prior_stores[round], byte_updates[round],
            halfword_updates[round], first_addends[round],
            second_addends[round], neighbor_updates[round],
            marker_updates[round], &first_amo_old, &second_amo_old,
            &marker_observed);
        AI_ASSERT_NO_EXCEPT_AND("cbo.inval through a same-paddr alias followed by base-view prefetch_r must not let a refill image bypass the alias-view prior full store plus younger byte/halfword stores consumed by AMO old values",
            first_amo_old == expected_merged &&
            second_amo_old == after_first_amo &&
            marker_observed == marker_updates[round] &&
            ld(target_base_vaddr) == final_target &&
            ld(target_alias_vaddr) == final_target &&
            ld(neighbor_base_vaddr) == neighbor_updates[round] &&
            ld(neighbor_alias_vaddr) == neighbor_updates[round] &&
            ld(tail_base_vaddr) == expected_tail &&
            ld(tail_alias_vaddr) == expected_tail &&
            ld(marker_base_vaddr) == marker_updates[round] &&
            ld(marker_alias_vaddr) == marker_updates[round]
        );

        TEST_SETUP_EXCEPT();
        AI_ASSERT_NO_EXCEPT_AND("the same-paddr-alias refill plus prior-store partial-merge AMO window preserves guards and the adjacent cache block through both views",
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
        TEST_ASSERT("backing memory after the same-paddr-alias cbo.inval + prefetch_r + prior-store + partial-store + AMO window contains only the final AMO result, neighbor store, marker update, and unchanged guards",
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

    ai_micro_diffsync_reset_env(MENVCFG_CBIE01);

    TEST_END();
}

bool ai_micro_hs_cbo_inval_prefetch_same_paddr_alias_refill_then_prior_store_partial_lrsc_no_stale_merged_old_value_or_reservation_loss_corner()
{
    TEST_START();

    const enum test_page base_page = SWITCH1;
    const enum test_page alias_page = SWITCH2;
    const uintptr_t target_base_vaddr = hs_page_base(base_page) + 0xe00U;
    const uintptr_t target_base_paddr = phys_page_base(base_page) + 0xe00U;
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
        0x7766554433221100ULL,
        0x89abcdef01234567ULL,
        0x5a5aa5a55a5aa5a5ULL,
    };
    const uint64_t adjacent_seeds[] = {
        0xfedcba9876543210ULL,
        0x0123456789abcdefULL,
        0x55aa7733cc11ee99ULL,
    };
    const uint64_t prior_stores[] = {
        0x1111222233334444ULL,
        0x5555666677778888ULL,
        0x9999aaaabbbbccccULL,
    };
    const uint8_t byte_updates[] = {
        0x6dU,
        0xb4U,
        0x29U,
    };
    const uint16_t halfword_updates[] = {
        0x3141U,
        0x5926U,
        0x5358U,
    };
    const uint64_t sc_payloads[] = {
        0x0102030405060708ULL,
        0x1827364554637281ULL,
        0xfdb97531eca86420ULL,
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
        0x0123456789abcdefULL,
        0xfedcba9876543210ULL,
        0x55aa7733cc11ee99ULL,
    };
    uintptr_t alias_page_base_vaddr = 0;
    uintptr_t target_alias_vaddr = 0;
    uintptr_t byte_store_alias_vaddr = 0;
    uintptr_t half_store_alias_vaddr = 0;
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
    target_alias_vaddr = alias_page_base_vaddr + 0xe00U;
    byte_store_alias_vaddr = target_alias_vaddr + 1U;
    half_store_alias_vaddr = target_alias_vaddr + 4U;
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
        const uint64_t expected_merged =
            ai_micro_diffsync_merge_byte_halfword_into_word(
                prior_stores[round],
                byte_updates[round],
                halfword_updates[round]);
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
        AI_ASSERT_NO_EXCEPT_AND("before the same-paddr-alias cbo.inval + base-view prefetch_r + alias-view prior-store + partial-store + LR/SC window starts, both target views, adjacent block, and marker are readable with seeded values",
            ld(target_base_vaddr) == block_seeds[round] &&
            ld(target_alias_vaddr) == block_seeds[round] &&
            ld(tail_base_vaddr) == expected_tail &&
            ld(tail_alias_vaddr) == expected_tail &&
            ld(adjacent_base_vaddr) == adjacent_seeds[round] &&
            ld(adjacent_alias_vaddr) == adjacent_seeds[round] &&
            ld(marker_base_vaddr) == marker_seeds[round] &&
            ld(marker_alias_vaddr) == marker_seeds[round]
        );

        TEST_SETUP_EXCEPT();
        ai_micro_diffsync_cbo_inval_prefetch_prior_store_partial_lrsc_window(
            inval_alias_vaddr, prefetch_base_vaddr, target_alias_vaddr,
            byte_store_alias_vaddr, half_store_alias_vaddr,
            target_alias_vaddr, neighbor_base_vaddr, marker_base_vaddr,
            prior_stores[round], byte_updates[round],
            halfword_updates[round], sc_payloads[round],
            neighbor_updates[round], marker_updates[round],
            &lr_observed, &sc_status, &marker_observed);
        AI_ASSERT_NO_EXCEPT_AND("cbo.inval through a same-paddr alias followed by base-view prefetch_r must not let a refill image bypass the alias-view prior full store plus younger byte/halfword stores consumed by LR/SC",
            lr_observed == expected_merged &&
            sc_status == 0ULL &&
            marker_observed == marker_updates[round] &&
            ld(target_base_vaddr) == sc_payloads[round] &&
            ld(target_alias_vaddr) == sc_payloads[round] &&
            ld(neighbor_base_vaddr) == neighbor_updates[round] &&
            ld(neighbor_alias_vaddr) == neighbor_updates[round] &&
            ld(tail_base_vaddr) == expected_tail &&
            ld(tail_alias_vaddr) == expected_tail &&
            ld(marker_base_vaddr) == marker_updates[round] &&
            ld(marker_alias_vaddr) == marker_updates[round]
        );

        TEST_SETUP_EXCEPT();
        AI_ASSERT_NO_EXCEPT_AND("the same-paddr-alias refill plus prior-store partial-merge LR/SC window preserves guards and the adjacent cache block through both views",
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
        TEST_ASSERT("backing memory after the same-paddr-alias cbo.inval + prefetch_r + prior-store + partial-store + LR/SC window contains only the successful SC payload, neighbor store, marker update, and unchanged guards",
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

    ai_micro_diffsync_reset_env(MENVCFG_CBIE01);

    TEST_END();
}

bool ai_micro_hs_cbo_inval_prefetch_refill_then_prior_store_partial_fence_post_store_no_stale_merged_prior_or_marker_drop_corner()
{
    TEST_START();

    const enum test_page target_page = SWITCH1;
    const enum test_page marker_page = SWITCH2;
    const uintptr_t target_base_vaddr = hs_page_base(target_page) + 0xe80U;
    const uintptr_t target_base_paddr = phys_page_base(target_page) + 0xe80U;
    const uintptr_t word_vaddr = target_base_vaddr;
    const uintptr_t byte_store_vaddr = target_base_vaddr + 1U;
    const uintptr_t half_store_vaddr = target_base_vaddr + 4U;
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
    const uintptr_t marker_vaddr = hs_page_base(marker_page) + 0xe80U;
    const uintptr_t marker_paddr = phys_page_base(marker_page) + 0xe80U;
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
        0x1111222233334444ULL,
        0x5555666677778888ULL,
        0x9999aaaabbbbccccULL,
    };
    const uint8_t byte_updates[] = {
        0x42U,
        0x99U,
        0x5dU,
    };
    const uint16_t halfword_updates[] = {
        0x1357U,
        0x2468U,
        0xace0U,
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
        const uint64_t expected_merged =
            ai_micro_diffsync_merge_byte_halfword_into_word(
                prior_stores[round],
                byte_updates[round],
                halfword_updates[round]);
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
        AI_ASSERT_NO_EXCEPT_AND("before the cbo.inval + prefetch_r + prior-store + partial-store + fence handoff starts, target block, neighbor word, adjacent block, and marker are readable with seeded values",
            ld(word_vaddr) == block_seeds[round] &&
            ld(neighbor_vaddr) == expected_neighbor_seed &&
            ld(tail_vaddr) == expected_tail &&
            ld(adjacent_base_vaddr) == adjacent_seeds[round] &&
            ld(marker_vaddr) == marker_seeds[round]
        );

        TEST_SETUP_EXCEPT();
        ai_micro_diffsync_cbo_inval_prefetch_prior_store_partial_fence_post_store_window(
            inval_vaddr, prefetch_vaddr, word_vaddr, byte_store_vaddr,
            half_store_vaddr, neighbor_vaddr, marker_vaddr,
            prior_stores[round], byte_updates[round],
            halfword_updates[round], neighbor_updates[round],
            marker_updates[round], &target_after_fence, &neighbor_observed,
            &marker_observed);
        AI_ASSERT_NO_EXCEPT_AND("cbo.inval plus same-block prefetch_r must not let a refill image erase the merged prior full store plus younger partial stores before fence hands ownership to post-fence stores",
            target_after_fence == expected_merged &&
            neighbor_observed == neighbor_updates[round] &&
            marker_observed == marker_updates[round] &&
            ld(word_vaddr) == expected_merged &&
            ld(neighbor_vaddr) == neighbor_updates[round] &&
            ld(tail_vaddr) == expected_tail &&
            ld(marker_vaddr) == marker_updates[round]
        );

        TEST_SETUP_EXCEPT();
        AI_ASSERT_NO_EXCEPT_AND("the cbo.inval + prefetch_r + prior-store partial-merge fence handoff preserves target guards, marker guard, and the adjacent cache block",
            ld(before_guard_vaddr) == before_guards[round] &&
            ld(after_guard_vaddr) == after_guards[round] &&
            ld(marker_guard_vaddr) == marker_guards[round] &&
            ld(adjacent_base_vaddr) == adjacent_seeds[round]
        );

        goto_priv(PRIV_M);
        TEST_ASSERT("backing memory after the cbo.inval + prefetch_r + prior-store partial-merge fence handoff contains only the merged target word, post-fence neighbor store, marker update, and unchanged guards",
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

    ai_micro_diffsync_reset_env(MENVCFG_CBIE01);

    TEST_END();
}

bool ai_micro_hs_cbo_inval_prefetch_same_paddr_alias_refill_then_prior_store_partial_fence_post_store_no_stale_merged_prior_or_marker_drop_corner()
{
    TEST_START();

    const enum test_page base_page = SWITCH1;
    const enum test_page alias_page = SWITCH2;
    const uintptr_t target_base_vaddr = hs_page_base(base_page) + 0xf00U;
    const uintptr_t target_base_paddr = phys_page_base(base_page) + 0xf00U;
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
        0x1111222233334444ULL,
        0x5555666677778888ULL,
        0x9999aaaabbbbccccULL,
    };
    const uint8_t byte_updates[] = {
        0x53U,
        0xa7U,
        0x6cU,
    };
    const uint16_t halfword_updates[] = {
        0x2468U,
        0x1357U,
        0xbdf0U,
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
    target_alias_vaddr = alias_page_base_vaddr + 0xf00U;
    byte_store_alias_vaddr = target_alias_vaddr + 1U;
    half_store_alias_vaddr = target_alias_vaddr + 4U;
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
        const uint64_t expected_merged =
            ai_micro_diffsync_merge_byte_halfword_into_word(
                prior_stores[round],
                byte_updates[round],
                halfword_updates[round]);
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
        AI_ASSERT_NO_EXCEPT_AND("before the same-paddr-alias cbo.inval + base-view prefetch_r + alias-view prior-store + partial-store + fence handoff starts, both target views, neighbor word, adjacent block, and marker are readable with seeded values",
            ld(target_base_vaddr) == block_seeds[round] &&
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
        ai_micro_diffsync_cbo_inval_prefetch_prior_store_partial_fence_post_store_window(
            inval_alias_vaddr, prefetch_base_vaddr, target_alias_vaddr,
            byte_store_alias_vaddr, half_store_alias_vaddr,
            neighbor_base_vaddr, marker_base_vaddr,
            prior_stores[round], byte_updates[round],
            halfword_updates[round], neighbor_updates[round],
            marker_updates[round], &target_after_fence, &neighbor_observed,
            &marker_observed);
        AI_ASSERT_NO_EXCEPT_AND("cbo.inval through a same-paddr alias followed by base-view prefetch_r must not let a refill image erase the alias-view merged prior full store plus younger partial stores before fence hands ownership to post-fence stores",
            target_after_fence == expected_merged &&
            neighbor_observed == neighbor_updates[round] &&
            marker_observed == marker_updates[round] &&
            ld(target_base_vaddr) == expected_merged &&
            ld(target_alias_vaddr) == expected_merged &&
            ld(neighbor_base_vaddr) == neighbor_updates[round] &&
            ld(neighbor_alias_vaddr) == neighbor_updates[round] &&
            ld(tail_base_vaddr) == expected_tail &&
            ld(tail_alias_vaddr) == expected_tail &&
            ld(marker_base_vaddr) == marker_updates[round] &&
            ld(marker_alias_vaddr) == marker_updates[round]
        );

        TEST_SETUP_EXCEPT();
        AI_ASSERT_NO_EXCEPT_AND("the same-paddr-alias refill plus prior-store partial-merge fence handoff preserves guards and the adjacent cache block through both views",
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
        TEST_ASSERT("backing memory after the same-paddr-alias cbo.inval + prefetch_r + prior-store partial-merge fence handoff contains only the merged target word, post-fence neighbor store, marker update, and unchanged guards",
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

    ai_micro_diffsync_reset_env(MENVCFG_CBIE01);

    TEST_END();
}

bool ai_micro_hs_cbo_inval_prefetch_refill_then_prior_store_partial_cbom_no_stale_merged_payload_or_marker_drop_corner()
{
    TEST_START();

    const enum test_page target_page = SWITCH1;
    const enum test_page marker_page = SWITCH2;
    const uintptr_t target_base_vaddr = hs_page_base(target_page) + 0xd80U;
    const uintptr_t target_base_paddr = phys_page_base(target_page) + 0xd80U;
    const uintptr_t target_word_vaddr = target_base_vaddr;
    const uintptr_t byte_store_vaddr = target_base_vaddr + 1U;
    const uintptr_t half_store_vaddr = target_base_vaddr + 4U;
    const uintptr_t prefetch_vaddr = target_base_vaddr + sizeof(uint64_t);
    const uintptr_t clean_vaddr = target_base_vaddr + 2U * sizeof(uint64_t);
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
    const uintptr_t marker_vaddr = hs_page_base(marker_page) + 0xd80U;
    const uintptr_t marker_paddr = phys_page_base(marker_page) + 0xd80U;
    const uintptr_t marker_guard_vaddr = marker_vaddr + sizeof(uint64_t);
    const uintptr_t marker_guard_paddr = marker_paddr + sizeof(uint64_t);
    const uint64_t block_seeds[] = {
        0x31425364758697a8ULL,
        0x425364758697a8b9ULL,
        0x5364758697a8b9caULL,
    };
    const uint64_t adjacent_seeds[] = {
        0xfedcba9876543210ULL,
        0x0123456789abcdefULL,
        0x55aa7733cc11ee99ULL,
    };
    const uint64_t prior_stores[] = {
        0x1111222233334444ULL,
        0x5555666677778888ULL,
        0x9999aaaabbbbccccULL,
    };
    const uint8_t byte_updates[] = {
        0x7aU,
        0xc3U,
        0x4dU,
    };
    const uint16_t halfword_updates[] = {
        0x1357U,
        0x2468U,
        0xbdf0U,
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
        const uint64_t expected_merged =
            ai_micro_diffsync_merge_byte_halfword_into_word(
                prior_stores[round],
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
        AI_ASSERT_NO_EXCEPT_AND("before the same-block cbo.inval + prefetch_r + prior-store + partial-store + CBOM handoff starts, target block, clean/flush probe words, neighbor word, adjacent block, and marker are readable with seeded values",
            ld(target_word_vaddr) == block_seeds[round] &&
            ld(clean_vaddr) == expected_clean_seed &&
            ld(neighbor_vaddr) == expected_neighbor_seed &&
            ld(flush_vaddr) == expected_flush_seed &&
            ld(adjacent_base_vaddr) == adjacent_seeds[round] &&
            ld(marker_vaddr) == marker_seeds[round]
        );

        TEST_SETUP_EXCEPT();
        ai_micro_diffsync_cbo_inval_prefetch_prior_store_partial_cbom_window(
            inval_vaddr, prefetch_vaddr, target_word_vaddr,
            byte_store_vaddr, half_store_vaddr, clean_vaddr, neighbor_vaddr,
            flush_vaddr, marker_vaddr, prior_stores[round],
            byte_updates[round], halfword_updates[round],
            neighbor_updates[round], marker_updates[round], &target_observed,
            &neighbor_observed, &marker_observed);
        AI_ASSERT_NO_EXCEPT_AND("cbo.inval followed by same-block prefetch_r must not let CBOM clean/flush ownership write back a stale refill image over the prior full store plus younger byte/halfword stores",
            target_observed == expected_merged &&
            neighbor_observed == neighbor_updates[round] &&
            marker_observed == marker_updates[round] &&
            ld(target_word_vaddr) == expected_merged &&
            ld(clean_vaddr) == expected_clean_seed &&
            ld(neighbor_vaddr) == neighbor_updates[round] &&
            ld(flush_vaddr) == expected_flush_seed &&
            ld(marker_vaddr) == marker_updates[round]
        );

        TEST_SETUP_EXCEPT();
        AI_ASSERT_NO_EXCEPT_AND("the same-block refill plus prior-store partial-merge CBOM owner handoff preserves target guards, marker guard, and the adjacent cache block",
            ld(before_guard_vaddr) == before_guards[round] &&
            ld(after_guard_vaddr) == after_guards[round] &&
            ld(marker_guard_vaddr) == marker_guards[round] &&
            ld(adjacent_base_vaddr) == adjacent_seeds[round]
        );

        goto_priv(PRIV_M);
        TEST_ASSERT("backing memory after the same-block cbo.inval + prefetch_r + prior-store partial-merge CBOM handoff contains only the merged target word, post-CBOM neighbor store, marker update, and unchanged guards",
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

bool ai_micro_hs_cbo_inval_prefetch_same_paddr_alias_refill_then_prior_store_partial_cbom_no_stale_merged_payload_or_marker_drop_corner()
{
    TEST_START();

    const enum test_page base_page = SWITCH1;
    const enum test_page alias_page = SWITCH2;
    const uintptr_t target_base_vaddr = hs_page_base(base_page) + 0xec0U;
    const uintptr_t target_base_paddr = phys_page_base(base_page) + 0xec0U;
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
        0x718293a4b5c6d7e8ULL,
        0x8293a4b5c6d7e8f9ULL,
        0x93a4b5c6d7e8f901ULL,
    };
    const uint64_t adjacent_seeds[] = {
        0xfedcba9876543210ULL,
        0x0123456789abcdefULL,
        0x55aa7733cc11ee99ULL,
    };
    const uint64_t prior_stores[] = {
        0x1011222233334444ULL,
        0x5051666677778888ULL,
        0x9091aaaabbbbccccULL,
    };
    const uint8_t byte_updates[] = {
        0x68U,
        0x31U,
        0xf2U,
    };
    const uint16_t halfword_updates[] = {
        0x9753U,
        0x8642U,
        0x7531U,
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
    target_alias_vaddr = alias_page_base_vaddr + 0xec0U;
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
                prior_stores[round],
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
        AI_ASSERT_NO_EXCEPT_AND("before the same-paddr-alias cbo.inval + base-view prefetch_r + alias-view prior-store + partial-store + CBOM handoff starts, both target views, clean/flush probe words, neighbor word, adjacent block, and marker are readable with seeded values",
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
        ai_micro_diffsync_cbo_inval_prefetch_prior_store_partial_cbom_window(
            inval_alias_vaddr, prefetch_base_vaddr, target_alias_vaddr,
            byte_store_alias_vaddr, half_store_alias_vaddr,
            clean_base_vaddr, neighbor_base_vaddr, flush_alias_vaddr,
            marker_base_vaddr, prior_stores[round], byte_updates[round],
            halfword_updates[round], neighbor_updates[round],
            marker_updates[round], &target_observed, &neighbor_observed,
            &marker_observed);
        AI_ASSERT_NO_EXCEPT_AND("cbo.inval through a same-paddr alias followed by base-view prefetch_r must not let base/alias CBOM ownership write back a stale refill image over the alias-view prior full store plus younger byte/halfword stores",
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
        AI_ASSERT_NO_EXCEPT_AND("the same-paddr-alias refill plus prior-store partial-merge CBOM owner handoff preserves guards and the adjacent cache block through both views",
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
        TEST_ASSERT("backing memory after the same-paddr-alias cbo.inval + prefetch_r + prior-store partial-merge CBOM handoff contains only the merged target word, post-CBOM neighbor store, marker update, and unchanged guards",
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

bool ai_micro_hs_cbo_inval_prefetch_refill_then_cbom_post_partial_store_no_stale_owner_or_byte_mask_drop_corner()
{
    TEST_START();

    const enum test_page target_page = SWITCH1;
    const enum test_page marker_page = SWITCH2;
    const uintptr_t target_base_vaddr = hs_page_base(target_page) + 0xb80U;
    const uintptr_t target_base_paddr = phys_page_base(target_page) + 0xb80U;
    const uintptr_t target_word_vaddr = target_base_vaddr;
    const uintptr_t byte_store_vaddr = target_base_vaddr + 1U;
    const uintptr_t half_store_vaddr = target_base_vaddr + 4U;
    const uintptr_t prefetch_vaddr = target_base_vaddr + sizeof(uint64_t);
    const uintptr_t clean_vaddr = target_base_vaddr + 2U * sizeof(uint64_t);
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
    const uintptr_t marker_vaddr = hs_page_base(marker_page) + 0xb80U;
    const uintptr_t marker_paddr = phys_page_base(marker_page) + 0xb80U;
    const uintptr_t marker_guard_vaddr = marker_vaddr + sizeof(uint64_t);
    const uintptr_t marker_guard_paddr = marker_paddr + sizeof(uint64_t);
    const uint64_t block_seeds[] = {
        0xc1d2e3f405162738ULL,
        0xd2e3f40516273849ULL,
        0xe3f405162738495aULL,
    };
    const uint64_t adjacent_seeds[] = {
        0xfedcba9876543210ULL,
        0x0123456789abcdefULL,
        0x55aa7733cc11ee99ULL,
    };
    const uint8_t byte_updates[] = {
        0x2dU,
        0xe4U,
        0x6bU,
    };
    const uint16_t halfword_updates[] = {
        0x4dd4U,
        0x9aa9U,
        0xc33cU,
    };
    const uint64_t neighbor_updates[] = {
        0x1029384756abcdefULL,
        0xefcdab7658473920ULL,
        0x7766554433221100ULL,
    };
    const uint64_t before_guards[] = {
        0x0101010102020202ULL,
        0x0303030304040404ULL,
        0x0505050506060606ULL,
    };
    const uint64_t after_guards[] = {
        0x7071727374757677ULL,
        0x8786858483828180ULL,
        0x123456789abcdef0ULL,
    };
    const uint64_t marker_seeds[] = {
        0x0a0b0c0d0e0f1011ULL,
        0x11100f0e0d0c0b0aULL,
        0x2468135724681357ULL,
    };
    const uint64_t marker_updates[] = {
        0x0f1e2d3c4b5a6978ULL,
        0x8877665544332211ULL,
        0x1029384756102938ULL,
    };
    const uint64_t marker_guards[] = {
        0xfaceb00cfaceb00cULL,
        0xdec0deeddec0deedULL,
        0xfeed1234feed1234ULL,
    };

    ai_micro_diffsync_prepare_hs_env(MENVCFG_CBIE01 | MENVCFG_CBZFE);

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
        AI_ASSERT_NO_EXCEPT_AND("before the same-block cbo.inval + prefetch_r + CBOM + post-CBOM partial-store handoff starts, target block, clean/flush probe words, neighbor word, adjacent block, and marker are readable with seeded values",
            ld(target_word_vaddr) == block_seeds[round] &&
            ld(clean_vaddr) == expected_clean_seed &&
            ld(neighbor_vaddr) == expected_neighbor_seed &&
            ld(flush_vaddr) == expected_flush_seed &&
            ld(adjacent_base_vaddr) == adjacent_seeds[round] &&
            ld(marker_vaddr) == marker_seeds[round]
        );

        TEST_SETUP_EXCEPT();
        ai_micro_diffsync_cbo_inval_prefetch_cbom_post_partial_store_window(
            inval_vaddr, prefetch_vaddr, clean_vaddr, flush_vaddr,
            target_word_vaddr, byte_store_vaddr, half_store_vaddr,
            neighbor_vaddr, marker_vaddr, byte_updates[round],
            halfword_updates[round], neighbor_updates[round],
            marker_updates[round], &target_observed, &neighbor_observed,
            &marker_observed);
        AI_ASSERT_NO_EXCEPT_AND("cbo.inval followed by same-block prefetch_r and CBOM clean/flush must release CMO ownership before post-CBOM byte/halfword stores so narrow masks, neighbor store, and marker survive",
            target_observed == expected_merged &&
            neighbor_observed == neighbor_updates[round] &&
            marker_observed == marker_updates[round] &&
            ld(target_word_vaddr) == expected_merged &&
            ld(clean_vaddr) == expected_clean_seed &&
            ld(neighbor_vaddr) == neighbor_updates[round] &&
            ld(flush_vaddr) == expected_flush_seed &&
            ld(marker_vaddr) == marker_updates[round]
        );

        TEST_SETUP_EXCEPT();
        AI_ASSERT_NO_EXCEPT_AND("the same-block refill plus CBOM/post-partial-store owner handoff preserves target guards, marker guard, and the adjacent cache block",
            ld(before_guard_vaddr) == before_guards[round] &&
            ld(after_guard_vaddr) == after_guards[round] &&
            ld(marker_guard_vaddr) == marker_guards[round] &&
            ld(adjacent_base_vaddr) == adjacent_seeds[round]
        );

        goto_priv(PRIV_M);
        TEST_ASSERT("backing memory after the same-block cbo.inval + prefetch_r + CBOM/post-partial-store handoff contains only the narrow merged target word, post-CBOM neighbor store, marker update, and unchanged guards",
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
