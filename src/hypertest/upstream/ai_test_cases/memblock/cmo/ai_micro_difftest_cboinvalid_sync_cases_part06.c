#include "ai_micro_difftest_cboinvalid_sync_common.h"

bool ai_micro_hs_cbo_inval_prefetch_same_paddr_alias_refill_then_amo_cbo_zero_no_stale_amo_or_zero_drop_corner()
{
    TEST_START();

    const enum test_page base_page = SWITCH1;
    const enum test_page alias_page = SWITCH2;
    const uintptr_t target_offset = 0x680U;
    const uintptr_t target_base_vaddr =
        hs_page_base(base_page) + target_offset;
    const uintptr_t target_base_paddr =
        phys_page_base(base_page) + target_offset;
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
        0xa3b4c5d6e7f00112ULL,
        0xb4c5d6e7f0011223ULL,
        0xc5d6e7f001122334ULL,
    };
    const uint64_t adjacent_seeds[] = {
        0x1021324354657687ULL,
        0x89abcdef76543210ULL,
        0x55aa7733cc11ee99ULL,
    };
    const uint64_t first_addends[] = {
        0x0000000000000456ULL,
        0x0000000000000567ULL,
        0x0000000000000678ULL,
    };
    const uint64_t second_addends[] = {
        0x0004000400040004ULL,
        0x0005000500050005ULL,
        0x0006000600060006ULL,
    };
    const uint64_t before_guards[] = {
        0xfdb97531eca86420ULL,
        0x13579bdf2468ace0ULL,
        0x02468ace13579bdfULL,
    };
    const uint64_t after_guards[] = {
        0x2233445566778899ULL,
        0x9988776655443322ULL,
        0x0102030405060708ULL,
    };
    const uint64_t marker_seeds[] = {
        0x4444555566667777ULL,
        0x88889999aaaabbbbULL,
        0xccccddddeeeeffffULL,
    };
    const uint64_t marker_updates[] = {
        0x89abcdef01234567ULL,
        0x76543210fedcba98ULL,
        0x0ff00ff00aa55aa5ULL,
    };
    const uint64_t marker_guards[] = {
        0x0101010102020202ULL,
        0x0303030304040404ULL,
        0x0505050506060606ULL,
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
        const uint64_t expected_zero_seed =
            ai_micro_diffsync_seed_word(block_seeds[round], 5U);
        const uint64_t expected_tail =
            ai_micro_diffsync_seed_word(block_seeds[round],
                AI_MICRO_DIFFSYNC_BLOCK_WORDS - 1U);
        const uint64_t after_first_amo =
            block_seeds[round] + first_addends[round];
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
        AI_ASSERT_NO_EXCEPT_AND("before the same-paddr-alias cbo.inval + prefetch_r + AMO + cbo.zero handoff starts, base and alias views expose the seeded target word, zero probe, tail, adjacent block, and marker",
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
        ai_micro_diffsync_cbo_inval_prefetch_amo_zero_window(
            inval_alias_vaddr, prefetch_base_vaddr, target_alias_vaddr,
            zero_alias_vaddr, marker_alias_vaddr, first_addends[round],
            second_addends[round], marker_updates[round], &first_amo_old,
            &second_amo_old, &marker_observed);
        AI_ASSERT_NO_EXCEPT_AND("cbo.inval through a same-paddr alias followed by base-view prefetch_r must let alias-view AMO consume the current seed before alias-view cbo.zero clears the whole block",
            first_amo_old == block_seeds[round] &&
            second_amo_old == after_first_amo &&
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
        AI_ASSERT_NO_EXCEPT_AND("the same-paddr-alias AMO-to-zero owner handoff preserves guards and the adjacent cache block through both views",
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
        TEST_ASSERT("backing memory after the same-paddr-alias cbo.inval + prefetch_r + AMO + cbo.zero handoff contains a fully zeroed target block, marker update, and unchanged guards",
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

bool ai_micro_hs_cbo_inval_prefetch_same_paddr_alias_refill_then_cbo_zero_partial_store_no_stale_zero_or_byte_mask_drop_corner()
{
    TEST_START();

    const enum test_page base_page = SWITCH1;
    const enum test_page alias_page = SWITCH2;
    const uintptr_t target_offset = 0x700U;
    const uintptr_t target_base_vaddr =
        hs_page_base(base_page) + target_offset;
    const uintptr_t target_base_paddr =
        phys_page_base(base_page) + target_offset;
    const uintptr_t word_base_vaddr = target_base_vaddr;
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
        0xd6e7f00112233445ULL,
        0xe7f0011223344556ULL,
        0xf001122334455667ULL,
    };
    const uint64_t adjacent_seeds[] = {
        0xccddeeff00112233ULL,
        0x32435465768798a9ULL,
        0xbcdef7654321089aULL,
    };
    const uint8_t byte_updates[] = {
        0x8eU,
        0xd7U,
        0x41U,
    };
    const uint16_t halfword_updates[] = {
        0x6996U,
        0xd44dU,
        0x8778U,
    };
    const uint64_t before_guards[] = {
        0x0102030405060708ULL,
        0x8070605040302010ULL,
        0x13579bdf2468ace0ULL,
    };
    const uint64_t after_guards[] = {
        0x1122334455667788ULL,
        0x8877665544332211ULL,
        0x2468ace013579bdfULL,
    };
    const uint64_t marker_seeds[] = {
        0x5555666677778888ULL,
        0x9999aaaabbbbccccULL,
        0xddddeeeeffff0000ULL,
    };
    const uint64_t marker_updates[] = {
        0x456789abcdef0123ULL,
        0x3210fedcba987654ULL,
        0x5a5aa5a50badf00dULL,
    };
    const uint64_t marker_guards[] = {
        0x123456789abcdef0ULL,
        0x0badf00d5a5aa5a5ULL,
        0x7766554433221100ULL,
    };
    uintptr_t alias_page_base_vaddr = 0;
    uintptr_t target_alias_vaddr = 0;
    uintptr_t word_alias_vaddr = 0;
    uintptr_t byte_store_alias_vaddr = 0;
    uintptr_t half_store_alias_vaddr = 0;
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
    word_alias_vaddr = target_alias_vaddr;
    byte_store_alias_vaddr = target_alias_vaddr + 1U;
    half_store_alias_vaddr = target_alias_vaddr + 4U;
    inval_alias_vaddr =
        alias_page_base_vaddr + target_offset + 6U * sizeof(uint64_t);
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
        const uint64_t expected_merged =
            ai_micro_diffsync_merge_byte_halfword_into_word(
                0ULL,
                byte_updates[round],
                halfword_updates[round]);
        uint64_t word_observed = 0xffffffffffffffffULL;
        uint64_t byte_observed = 0xffffffffffffffffULL;
        uint64_t half_observed = 0xffffffffffffffffULL;
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
        AI_ASSERT_NO_EXCEPT_AND("before the same-paddr-alias cbo.inval + prefetch_r + cbo.zero + partial-store handoff starts, base and alias views expose the seeded target word, zero probe, tail, adjacent block, and marker",
            ld(word_base_vaddr) == block_seeds[round] &&
            ld(word_alias_vaddr) == block_seeds[round] &&
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
        ai_micro_diffsync_cbo_inval_prefetch_zero_partial_store_window(
            inval_alias_vaddr, prefetch_base_vaddr, zero_alias_vaddr,
            word_alias_vaddr, byte_store_alias_vaddr,
            half_store_alias_vaddr, marker_alias_vaddr,
            byte_updates[round], halfword_updates[round],
            marker_updates[round], &word_observed, &byte_observed,
            &half_observed, &marker_observed);
        AI_ASSERT_NO_EXCEPT_AND("cbo.inval through a same-paddr alias followed by base-view prefetch_r must let alias-view cbo.zero clear the block before alias-view byte/halfword stores merge only their masks onto zero",
            word_observed == expected_merged &&
            byte_observed == byte_updates[round] &&
            half_observed == halfword_updates[round] &&
            marker_observed == marker_updates[round] &&
            ld(word_base_vaddr) == expected_merged &&
            ld(word_alias_vaddr) == expected_merged &&
            ld(zero_base_vaddr) == 0ULL &&
            ld(zero_alias_vaddr) == 0ULL &&
            ld(tail_base_vaddr) == 0ULL &&
            ld(tail_alias_vaddr) == 0ULL &&
            ld(marker_base_vaddr) == marker_updates[round] &&
            ld(marker_alias_vaddr) == marker_updates[round]
        );

        TEST_SETUP_EXCEPT();
        AI_ASSERT_NO_EXCEPT_AND("the same-paddr-alias zero-to-partial-store owner handoff preserves guards and the adjacent cache block through both views",
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
        TEST_ASSERT("backing memory after the same-paddr-alias cbo.inval + prefetch_r + cbo.zero + partial-store handoff contains only the narrow merged word over an otherwise zeroed block, marker update, and unchanged guards",
            ai_micro_diffsync_block_all_zero_except_word(target_base_paddr,
                0U, expected_merged) &&
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

bool ai_micro_hs_cbo_inval_prefetch_same_paddr_alias_refill_then_partial_store_cbo_zero_no_stale_mask_or_zero_drop_corner()
{
    TEST_START();

    const enum test_page base_page = SWITCH1;
    const enum test_page alias_page = SWITCH2;
    const uintptr_t target_offset = 0x780U;
    const uintptr_t target_base_vaddr =
        hs_page_base(base_page) + target_offset;
    const uintptr_t target_base_paddr =
        phys_page_base(base_page) + target_offset;
    const uintptr_t word_base_vaddr = target_base_vaddr;
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
        0x0f1e2d3c4b5a6978ULL,
        0x89abcdef76543210ULL,
        0x3141592653589793ULL,
    };
    const uint64_t adjacent_seeds[] = {
        0xfedcba9876543210ULL,
        0x1020304050607080ULL,
        0x55aa7733cc11ee99ULL,
    };
    const uint8_t byte_updates[] = {
        0x9bU,
        0x26U,
        0xd4U,
    };
    const uint16_t halfword_updates[] = {
        0x5aa5U,
        0xc33cU,
        0x9669U,
    };
    const uint64_t before_guards[] = {
        0x0badf00d5a5aa5a5ULL,
        0x1020304050607080ULL,
        0x1122334455667788ULL,
    };
    const uint64_t after_guards[] = {
        0x8877665544332211ULL,
        0x7766554433221100ULL,
        0x66554433221100ffULL,
    };
    const uint64_t marker_seeds[] = {
        0xaa55aa55aa55aa55ULL,
        0x55aa55aa55aa55aaULL,
        0xcafed00d5a5aa5a5ULL,
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
    uintptr_t word_alias_vaddr = 0;
    uintptr_t byte_store_alias_vaddr = 0;
    uintptr_t half_store_alias_vaddr = 0;
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
    word_alias_vaddr = target_alias_vaddr;
    byte_store_alias_vaddr = target_alias_vaddr + 1U;
    half_store_alias_vaddr = target_alias_vaddr + 4U;
    inval_alias_vaddr =
        alias_page_base_vaddr + target_offset + 6U * sizeof(uint64_t);
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
        const uint64_t expected_merged =
            ai_micro_diffsync_merge_byte_halfword_into_word(
                block_seeds[round],
                byte_updates[round],
                halfword_updates[round]);
        const uint64_t expected_zero_seed =
            ai_micro_diffsync_seed_word(block_seeds[round], 5U);
        const uint64_t expected_tail =
            ai_micro_diffsync_seed_word(block_seeds[round],
                AI_MICRO_DIFFSYNC_BLOCK_WORDS - 1U);
        uint64_t merged_before_zero = 0xffffffffffffffffULL;
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
        AI_ASSERT_NO_EXCEPT_AND("before the same-paddr-alias cbo.inval + prefetch_r + partial-store + cbo.zero handoff starts, base and alias views expose the seeded target word, zero probe, tail, adjacent block, and marker",
            ld(word_base_vaddr) == block_seeds[round] &&
            ld(word_alias_vaddr) == block_seeds[round] &&
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
        ai_micro_diffsync_cbo_inval_prefetch_partial_store_zero_window(
            inval_alias_vaddr, prefetch_base_vaddr, word_alias_vaddr,
            byte_store_alias_vaddr, half_store_alias_vaddr,
            zero_alias_vaddr, marker_alias_vaddr, byte_updates[round],
            halfword_updates[round], marker_updates[round],
            &merged_before_zero, &marker_observed);
        AI_ASSERT_NO_EXCEPT_AND("cbo.inval through a same-paddr alias followed by base-view prefetch_r must let alias-view byte/halfword stores merge into the refill image before alias-view cbo.zero clears the block",
            merged_before_zero == expected_merged &&
            marker_observed == marker_updates[round] &&
            ld(word_base_vaddr) == 0ULL &&
            ld(word_alias_vaddr) == 0ULL &&
            ld(zero_base_vaddr) == 0ULL &&
            ld(zero_alias_vaddr) == 0ULL &&
            ld(tail_base_vaddr) == 0ULL &&
            ld(tail_alias_vaddr) == 0ULL &&
            ld(marker_base_vaddr) == marker_updates[round] &&
            ld(marker_alias_vaddr) == marker_updates[round]
        );

        TEST_SETUP_EXCEPT();
        AI_ASSERT_NO_EXCEPT_AND("the same-paddr-alias partial-store-to-zero owner handoff preserves guards and the adjacent cache block through both views",
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
        TEST_ASSERT("backing memory after the same-paddr-alias cbo.inval + prefetch_r + partial-store + cbo.zero handoff contains a fully zeroed target block, marker update, and unchanged guards",
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

bool ai_micro_hs_cbo_inval_prefetch_same_paddr_alias_refill_then_cross16_store_cbo_zero_no_stale_split_or_zero_drop_corner()
{
    TEST_START();

    const enum test_page base_page = SWITCH1;
    const enum test_page alias_page = SWITCH2;
    const uintptr_t target_offset = 0x800U;
    const uintptr_t target_base_vaddr =
        hs_page_base(base_page) + target_offset;
    const uintptr_t target_base_paddr =
        phys_page_base(base_page) + target_offset;
    const uintptr_t low_word_base_vaddr =
        target_base_vaddr + sizeof(uint64_t);
    const uintptr_t high_word_base_vaddr =
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
        0x4d5e6f708192a3b4ULL,
        0x5e6f708192a3b4c5ULL,
        0x6f708192a3b4c5d6ULL,
    };
    const uint64_t adjacent_seeds[] = {
        0x1021324354657687ULL,
        0x98badcfe10325476ULL,
        0x7766554433221100ULL,
    };
    const uint64_t cross16_payloads[] = {
        0x8877665544332211ULL,
        0x10ffeeddccbbaa99ULL,
        0x78695a4b3c2d1e0fULL,
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
        0xa2a3a4a5a6a7a8a9ULL,
        0xb2b3b4b5b6b7b8b9ULL,
        0xc2c3c4c5c6c7c8c9ULL,
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
    uintptr_t cross16_store_alias_vaddr = 0;
    uintptr_t low_word_alias_vaddr = 0;
    uintptr_t high_word_alias_vaddr = 0;
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
    cross16_store_alias_vaddr = target_alias_vaddr + 15U;
    low_word_alias_vaddr = target_alias_vaddr + sizeof(uint64_t);
    high_word_alias_vaddr = target_alias_vaddr + 2U * sizeof(uint64_t);
    inval_alias_vaddr =
        target_alias_vaddr + 6U * sizeof(uint64_t);
    zero_alias_vaddr =
        target_alias_vaddr + 5U * sizeof(uint64_t);
    tail_alias_vaddr =
        target_alias_vaddr + (AI_MICRO_DIFFSYNC_BLOCK_WORDS - 1U) *
        sizeof(uint64_t);
    before_guard_alias_vaddr =
        target_alias_vaddr - sizeof(uint64_t);
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
        const uint64_t expected_zero_seed =
            ai_micro_diffsync_seed_word(block_seeds[round], 5U);
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
        AI_ASSERT_NO_EXCEPT_AND("before the same-paddr-alias cbo.inval + prefetch_r + cross-16B store + cbo.zero handoff starts, base and alias views expose seeded split words, zero probe, tail, adjacent block, and marker",
            ld(target_base_vaddr) == block_seeds[round] &&
            ld(target_alias_vaddr) == block_seeds[round] &&
            ld(low_word_base_vaddr) == seed_word1 &&
            ld(low_word_alias_vaddr) == seed_word1 &&
            ld(high_word_base_vaddr) == seed_word2 &&
            ld(high_word_alias_vaddr) == seed_word2 &&
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
        ai_micro_diffsync_cbo_inval_prefetch_cross16_store_zero_window(
            inval_alias_vaddr, prefetch_base_vaddr,
            cross16_store_alias_vaddr, low_word_alias_vaddr,
            high_word_alias_vaddr, zero_alias_vaddr, marker_alias_vaddr,
            cross16_payloads[round], marker_updates[round],
            &low_before_zero, &high_before_zero, &marker_observed);
        AI_ASSERT_NO_EXCEPT_AND("cbo.inval through a same-paddr alias followed by base-view prefetch_r must let the alias-view cross-16B split store become visible before alias-view cbo.zero clears the full block",
            low_before_zero == expected_word1 &&
            high_before_zero == expected_word2 &&
            marker_observed == marker_updates[round] &&
            ld(target_base_vaddr) == 0ULL &&
            ld(target_alias_vaddr) == 0ULL &&
            ld(low_word_base_vaddr) == 0ULL &&
            ld(low_word_alias_vaddr) == 0ULL &&
            ld(high_word_base_vaddr) == 0ULL &&
            ld(high_word_alias_vaddr) == 0ULL &&
            ld(zero_base_vaddr) == 0ULL &&
            ld(zero_alias_vaddr) == 0ULL &&
            ld(tail_base_vaddr) == 0ULL &&
            ld(tail_alias_vaddr) == 0ULL &&
            ld(marker_base_vaddr) == marker_updates[round] &&
            ld(marker_alias_vaddr) == marker_updates[round]
        );

        TEST_SETUP_EXCEPT();
        AI_ASSERT_NO_EXCEPT_AND("the same-paddr-alias cross16-store-to-zero owner handoff preserves guards and the adjacent cache block through both views",
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
        TEST_ASSERT("backing memory after the same-paddr-alias cbo.inval + prefetch_r + cross-16B store + cbo.zero handoff contains a fully zeroed target block, marker update, and unchanged guards",
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

bool ai_micro_hs_cbo_inval_prefetch_same_paddr_alias_refill_then_cross16_store_cbom_no_stale_split_or_cmo_owner_drop_corner()
{
    TEST_START();

    const enum test_page base_page = SWITCH1;
    const enum test_page alias_page = SWITCH2;
    const uintptr_t target_offset = 0x840U;
    const uintptr_t target_base_vaddr =
        hs_page_base(base_page) + target_offset;
    const uintptr_t target_base_paddr =
        phys_page_base(base_page) + target_offset;
    const uintptr_t low_word_base_vaddr =
        target_base_vaddr + sizeof(uint64_t);
    const uintptr_t high_word_base_vaddr =
        target_base_vaddr + 2U * sizeof(uint64_t);
    const uintptr_t prefetch_base_vaddr =
        target_base_vaddr + sizeof(uint64_t);
    const uintptr_t clean_base_vaddr =
        target_base_vaddr + 3U * sizeof(uint64_t);
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
        0x728394a5b6c7d8e9ULL,
        0x8394a5b6c7d8e9faULL,
        0x94a5b6c7d8e9fa0bULL,
    };
    const uint64_t adjacent_seeds[] = {
        0x0123456789abcdefULL,
        0xfedcba9876543210ULL,
        0x55aa7733cc11ee99ULL,
    };
    const uint64_t cross16_payloads[] = {
        0x1122334455667788ULL,
        0x99aabbccddeeff10ULL,
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
        0xd0d1d2d3d4d5d6d7ULL,
        0xe0e1e2e3e4e5e6e7ULL,
        0xf0f1f2f3f4f5f6f7ULL,
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
    uintptr_t flush_alias_vaddr = 0;
    uintptr_t before_guard_alias_vaddr = 0;
    uintptr_t after_guard_alias_vaddr = 0;
    uintptr_t adjacent_alias_vaddr = 0;
    uintptr_t marker_alias_vaddr = 0;
    uintptr_t marker_guard_alias_vaddr = 0;

    ai_micro_diffsync_prepare_hs_env(MENVCFG_CBIE01 | MENVCFG_CBZFE);
    alias_page_base_vaddr =
        ai_micro_diffsync_prepare_same_paddr_alias(alias_page, base_page);
    target_alias_vaddr = alias_page_base_vaddr + target_offset;
    cross16_store_alias_vaddr = target_alias_vaddr + 15U;
    low_word_alias_vaddr = target_alias_vaddr + sizeof(uint64_t);
    high_word_alias_vaddr = target_alias_vaddr + 2U * sizeof(uint64_t);
    inval_alias_vaddr =
        target_alias_vaddr + 4U * sizeof(uint64_t);
    clean_alias_vaddr =
        target_alias_vaddr + 3U * sizeof(uint64_t);
    flush_alias_vaddr =
        target_alias_vaddr + (AI_MICRO_DIFFSYNC_BLOCK_WORDS - 1U) *
        sizeof(uint64_t);
    before_guard_alias_vaddr =
        target_alias_vaddr - sizeof(uint64_t);
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
        const uint64_t expected_flush_seed =
            ai_micro_diffsync_seed_word(block_seeds[round],
                AI_MICRO_DIFFSYNC_BLOCK_WORDS - 1U);
        uint64_t low_observed = 0ULL;
        uint64_t high_observed = 0ULL;
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
        AI_ASSERT_NO_EXCEPT_AND("before the same-paddr-alias cbo.inval + prefetch_r + cross-16B store + CBOM handoff starts, base and alias views expose seeded split words, clean/flush probes, adjacent block, and marker",
            ld(target_base_vaddr) == block_seeds[round] &&
            ld(target_alias_vaddr) == block_seeds[round] &&
            ld(low_word_base_vaddr) == seed_word1 &&
            ld(low_word_alias_vaddr) == seed_word1 &&
            ld(high_word_base_vaddr) == seed_word2 &&
            ld(high_word_alias_vaddr) == seed_word2 &&
            ld(clean_base_vaddr) == expected_clean_seed &&
            ld(clean_alias_vaddr) == expected_clean_seed &&
            ld(flush_base_vaddr) == expected_flush_seed &&
            ld(flush_alias_vaddr) == expected_flush_seed &&
            ld(adjacent_base_vaddr) == adjacent_seeds[round] &&
            ld(adjacent_alias_vaddr) == adjacent_seeds[round] &&
            ld(marker_base_vaddr) == marker_seeds[round] &&
            ld(marker_alias_vaddr) == marker_seeds[round]
        );

        TEST_SETUP_EXCEPT();
        ai_micro_diffsync_cbo_inval_prefetch_cross16_store_cbom_window(
            inval_alias_vaddr, prefetch_base_vaddr,
            cross16_store_alias_vaddr, low_word_alias_vaddr,
            high_word_alias_vaddr, clean_base_vaddr, flush_alias_vaddr,
            marker_alias_vaddr, cross16_payloads[round],
            marker_updates[round], &low_observed, &high_observed,
            &marker_observed);
        AI_ASSERT_NO_EXCEPT_AND("cbo.inval through a same-paddr alias followed by base-view prefetch_r must let alias-view cross-16B split store become visible before base/alias CBOM drains the dirty block",
            low_observed == expected_word1 &&
            high_observed == expected_word2 &&
            marker_observed == marker_updates[round] &&
            ld(low_word_base_vaddr) == expected_word1 &&
            ld(low_word_alias_vaddr) == expected_word1 &&
            ld(high_word_base_vaddr) == expected_word2 &&
            ld(high_word_alias_vaddr) == expected_word2 &&
            ld(clean_base_vaddr) == expected_clean_seed &&
            ld(clean_alias_vaddr) == expected_clean_seed &&
            ld(flush_base_vaddr) == expected_flush_seed &&
            ld(flush_alias_vaddr) == expected_flush_seed &&
            ld(marker_base_vaddr) == marker_updates[round] &&
            ld(marker_alias_vaddr) == marker_updates[round]
        );

        TEST_SETUP_EXCEPT();
        AI_ASSERT_NO_EXCEPT_AND("the same-paddr-alias cross16-store-to-CBOM owner handoff preserves guards and the adjacent cache block through both views",
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
        TEST_ASSERT("backing memory after the same-paddr-alias cbo.inval + prefetch_r + cross-16B store + CBOM handoff contains the precise split-store overlay, marker update, and unchanged guards",
            ai_micro_diffsync_block_matches_seed_except_two_words(
                target_base_paddr,
                block_seeds[round],
                1U,
                expected_word1,
                2U,
                expected_word2) &&
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

bool ai_micro_hs_cbo_inval_prefetch_same_paddr_alias_refill_then_cbo_zero_fence_post_store_no_stale_zero_fence_owner_or_marker_drop_corner()
{
    TEST_START();

    const enum test_page base_page = SWITCH1;
    const enum test_page alias_page = SWITCH2;
    const uintptr_t target_offset = 0x880U;
    const uintptr_t target_base_vaddr =
        hs_page_base(base_page) + target_offset;
    const uintptr_t target_base_paddr =
        phys_page_base(base_page) + target_offset;
    const uintptr_t post_store_base_vaddr = target_base_vaddr;
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
        0xa5b6c7d8e9fa0b1cULL,
        0xb6c7d8e9fa0b1c2dULL,
        0xc7d8e9fa0b1c2d3eULL,
    };
    const uint64_t adjacent_seeds[] = {
        0x0123456789abcdefULL,
        0xfedcba9876543210ULL,
        0x55aa7733cc11ee99ULL,
    };
    const uint64_t post_store_updates[] = {
        0x13579bdf2468ace0ULL,
        0x2468ace013579bdfULL,
        0x3579bdf02468ace1ULL,
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
    uintptr_t post_store_alias_vaddr = 0;
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
    post_store_alias_vaddr = target_alias_vaddr;
    inval_alias_vaddr =
        target_alias_vaddr + 6U * sizeof(uint64_t);
    zero_alias_vaddr =
        target_alias_vaddr + 4U * sizeof(uint64_t);
    tail_alias_vaddr =
        target_alias_vaddr + (AI_MICRO_DIFFSYNC_BLOCK_WORDS - 1U) *
        sizeof(uint64_t);
    before_guard_alias_vaddr =
        target_alias_vaddr - sizeof(uint64_t);
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
        const uint64_t expected_zero_seed =
            ai_micro_diffsync_seed_word(block_seeds[round], 4U);
        const uint64_t expected_tail =
            ai_micro_diffsync_seed_word(block_seeds[round],
                AI_MICRO_DIFFSYNC_BLOCK_WORDS - 1U);
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
        AI_ASSERT_NO_EXCEPT_AND("before the same-paddr-alias cbo.inval + prefetch_r + cbo.zero + fence + post-store handoff starts, base and alias views expose seeded target, zero probe, tail, adjacent block, and marker",
            ld(post_store_base_vaddr) == block_seeds[round] &&
            ld(post_store_alias_vaddr) == block_seeds[round] &&
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
        ai_micro_diffsync_cbo_inval_prefetch_zero_fence_post_store_window(
            inval_alias_vaddr, prefetch_base_vaddr, zero_alias_vaddr,
            post_store_alias_vaddr, marker_alias_vaddr,
            post_store_updates[round], marker_updates[round],
            &post_store_observed, &marker_observed);
        AI_ASSERT_NO_EXCEPT_AND("cbo.inval through a same-paddr alias followed by base-view prefetch_r must let alias-view cbo.zero drain through fence before the post-fence alias store and marker become visible",
            post_store_observed == post_store_updates[round] &&
            marker_observed == marker_updates[round] &&
            ld(post_store_base_vaddr) == post_store_updates[round] &&
            ld(post_store_alias_vaddr) == post_store_updates[round] &&
            ld(zero_base_vaddr) == 0ULL &&
            ld(zero_alias_vaddr) == 0ULL &&
            ld(tail_base_vaddr) == 0ULL &&
            ld(tail_alias_vaddr) == 0ULL &&
            ld(marker_base_vaddr) == marker_updates[round] &&
            ld(marker_alias_vaddr) == marker_updates[round]
        );

        TEST_SETUP_EXCEPT();
        AI_ASSERT_NO_EXCEPT_AND("the same-paddr-alias zero-fence post-store owner handoff preserves guards and the adjacent cache block through both views",
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
        TEST_ASSERT("backing memory after the same-paddr-alias cbo.inval + prefetch_r + cbo.zero + fence + post-store handoff contains only the post-fence store over an otherwise zeroed block, marker update, and unchanged guards",
            ai_micro_diffsync_block_all_zero_except_word(target_base_paddr,
                0U, post_store_updates[round]) &&
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

bool ai_micro_hs_cbo_inval_prefetch_same_paddr_alias_refill_then_prior_store_fence_cbo_zero_post_store_no_stale_prior_zero_owner_or_marker_drop_corner()
{
    TEST_START();

    const enum test_page base_page = SWITCH1;
    const enum test_page alias_page = SWITCH2;
    const uintptr_t target_offset = 0x8c0U;
    const uintptr_t target_base_vaddr =
        hs_page_base(base_page) + target_offset;
    const uintptr_t target_base_paddr =
        phys_page_base(base_page) + target_offset;
    const uintptr_t prior_store_base_vaddr = target_base_vaddr;
    const uintptr_t prefetch_base_vaddr =
        target_base_vaddr + sizeof(uint64_t);
    const uintptr_t zero_base_vaddr =
        target_base_vaddr + 4U * sizeof(uint64_t);
    const uintptr_t post_store_base_vaddr =
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
        0xd8e9fa0b1c2d3e4fULL,
        0xe9fa0b1c2d3e4f50ULL,
        0xfa0b1c2d3e4f5061ULL,
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
    const uint64_t post_zero_updates[] = {
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
        0xa1a2a3a4a5a6a7a8ULL,
        0xb1b2b3b4b5b6b7b8ULL,
        0xc1c2c3c4c5c6c7c8ULL,
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
    uintptr_t prior_store_alias_vaddr = 0;
    uintptr_t inval_alias_vaddr = 0;
    uintptr_t zero_alias_vaddr = 0;
    uintptr_t post_store_alias_vaddr = 0;
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
    prior_store_alias_vaddr = target_alias_vaddr;
    inval_alias_vaddr =
        target_alias_vaddr + 6U * sizeof(uint64_t);
    zero_alias_vaddr =
        target_alias_vaddr + 4U * sizeof(uint64_t);
    post_store_alias_vaddr =
        target_alias_vaddr + 5U * sizeof(uint64_t);
    tail_alias_vaddr =
        target_alias_vaddr + (AI_MICRO_DIFFSYNC_BLOCK_WORDS - 1U) *
        sizeof(uint64_t);
    before_guard_alias_vaddr =
        target_alias_vaddr - sizeof(uint64_t);
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
        const uint64_t expected_zero_seed =
            ai_micro_diffsync_seed_word(block_seeds[round], 4U);
        const uint64_t expected_post_store_seed =
            ai_micro_diffsync_seed_word(block_seeds[round], 5U);
        const uint64_t expected_tail =
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
        AI_ASSERT_NO_EXCEPT_AND("before the same-paddr-alias cbo.inval + prefetch_r + prior-store + fence + cbo.zero handoff starts, base and alias views expose seeded target, zero probe, post-zero word, tail, adjacent block, and marker",
            ld(prior_store_base_vaddr) == block_seeds[round] &&
            ld(prior_store_alias_vaddr) == block_seeds[round] &&
            ld(zero_base_vaddr) == expected_zero_seed &&
            ld(zero_alias_vaddr) == expected_zero_seed &&
            ld(post_store_base_vaddr) == expected_post_store_seed &&
            ld(post_store_alias_vaddr) == expected_post_store_seed &&
            ld(tail_base_vaddr) == expected_tail &&
            ld(tail_alias_vaddr) == expected_tail &&
            ld(adjacent_base_vaddr) == adjacent_seeds[round] &&
            ld(adjacent_alias_vaddr) == adjacent_seeds[round] &&
            ld(marker_base_vaddr) == marker_seeds[round] &&
            ld(marker_alias_vaddr) == marker_seeds[round]
        );

        TEST_SETUP_EXCEPT();
        ai_micro_diffsync_cbo_inval_prefetch_prior_store_fence_zero_post_store_window(
            inval_alias_vaddr, prefetch_base_vaddr, prior_store_alias_vaddr,
            zero_alias_vaddr, post_store_alias_vaddr, marker_alias_vaddr,
            prior_store_updates[round], post_zero_updates[round],
            marker_updates[round], &prior_store_observed,
            &post_store_observed, &marker_observed);
        AI_ASSERT_NO_EXCEPT_AND("cbo.inval through a same-paddr alias followed by base-view prefetch_r must let the prior alias store survive fence, then let alias-view cbo.zero clear it before the post-zero store and marker progress",
            prior_store_observed == prior_store_updates[round] &&
            post_store_observed == post_zero_updates[round] &&
            marker_observed == marker_updates[round] &&
            ld(prior_store_base_vaddr) == 0ULL &&
            ld(prior_store_alias_vaddr) == 0ULL &&
            ld(zero_base_vaddr) == 0ULL &&
            ld(zero_alias_vaddr) == 0ULL &&
            ld(post_store_base_vaddr) == post_zero_updates[round] &&
            ld(post_store_alias_vaddr) == post_zero_updates[round] &&
            ld(tail_base_vaddr) == 0ULL &&
            ld(tail_alias_vaddr) == 0ULL &&
            ld(marker_base_vaddr) == marker_updates[round] &&
            ld(marker_alias_vaddr) == marker_updates[round]
        );

        TEST_SETUP_EXCEPT();
        AI_ASSERT_NO_EXCEPT_AND("the same-paddr-alias prior-store fence zero post-store owner handoff preserves guards and the adjacent cache block through both views",
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
        TEST_ASSERT("backing memory after the same-paddr-alias cbo.inval + prefetch_r + prior-store + fence + cbo.zero + post-store handoff contains only the post-zero store over an otherwise zeroed block, marker update, and unchanged guards",
            ai_micro_diffsync_block_all_zero_except_word(target_base_paddr,
                5U, post_zero_updates[round]) &&
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

bool ai_micro_hs_cbo_inval_prefetch_same_paddr_alias_refill_then_cbo_zero_cbom_post_store_no_stale_zero_cmo_owner_or_marker_drop_corner()
{
    TEST_START();

    const enum test_page base_page = SWITCH1;
    const enum test_page alias_page = SWITCH2;
    const uintptr_t target_offset = 0x900U;
    const uintptr_t target_base_vaddr =
        hs_page_base(base_page) + target_offset;
    const uintptr_t target_base_paddr =
        phys_page_base(base_page) + target_offset;
    const uintptr_t prefetch_base_vaddr =
        target_base_vaddr + sizeof(uint64_t);
    const uintptr_t clean_base_vaddr =
        target_base_vaddr + 3U * sizeof(uint64_t);
    const uintptr_t zero_base_vaddr =
        target_base_vaddr + 4U * sizeof(uint64_t);
    const uintptr_t post_store_base_vaddr =
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
        0x0b1c2d3e4f506172ULL,
        0x1c2d3e4f50617283ULL,
        0x2d3e4f5061728394ULL,
    };
    const uint64_t adjacent_seeds[] = {
        0x0123456789abcdefULL,
        0xfedcba9876543210ULL,
        0x55aa7733cc11ee99ULL,
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
        0xa2a3a4a5a6a7a8a9ULL,
        0xb2b3b4b5b6b7b8b9ULL,
        0xc2c3c4c5c6c7c8c9ULL,
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
    uintptr_t zero_alias_vaddr = 0;
    uintptr_t post_store_alias_vaddr = 0;
    uintptr_t tail_alias_vaddr = 0;
    uintptr_t before_guard_alias_vaddr = 0;
    uintptr_t after_guard_alias_vaddr = 0;
    uintptr_t adjacent_alias_vaddr = 0;
    uintptr_t marker_alias_vaddr = 0;
    uintptr_t marker_guard_alias_vaddr = 0;

    ai_micro_diffsync_prepare_hs_env(
        MENVCFG_CBIE01 | MENVCFG_CBZE | MENVCFG_CBZFE);
    alias_page_base_vaddr =
        ai_micro_diffsync_prepare_same_paddr_alias(alias_page, base_page);
    target_alias_vaddr = alias_page_base_vaddr + target_offset;
    inval_alias_vaddr =
        target_alias_vaddr + 6U * sizeof(uint64_t);
    clean_alias_vaddr =
        target_alias_vaddr + 3U * sizeof(uint64_t);
    zero_alias_vaddr =
        target_alias_vaddr + 4U * sizeof(uint64_t);
    post_store_alias_vaddr =
        target_alias_vaddr + 5U * sizeof(uint64_t);
    tail_alias_vaddr =
        target_alias_vaddr + (AI_MICRO_DIFFSYNC_BLOCK_WORDS - 1U) *
        sizeof(uint64_t);
    before_guard_alias_vaddr =
        target_alias_vaddr - sizeof(uint64_t);
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
        const uint64_t expected_zero_seed =
            ai_micro_diffsync_seed_word(block_seeds[round], 4U);
        const uint64_t expected_post_store_seed =
            ai_micro_diffsync_seed_word(block_seeds[round], 5U);
        const uint64_t expected_tail =
            ai_micro_diffsync_seed_word(block_seeds[round],
                AI_MICRO_DIFFSYNC_BLOCK_WORDS - 1U);
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
        AI_ASSERT_NO_EXCEPT_AND("before the same-paddr-alias cbo.inval + prefetch_r + cbo.zero + CBOM post-store handoff starts, base and alias views expose seeded target, clean/zero/post-store probes, tail, adjacent block, and marker",
            ld(target_base_vaddr) == block_seeds[round] &&
            ld(target_alias_vaddr) == block_seeds[round] &&
            ld(clean_base_vaddr) == expected_clean_seed &&
            ld(clean_alias_vaddr) == expected_clean_seed &&
            ld(zero_base_vaddr) == expected_zero_seed &&
            ld(zero_alias_vaddr) == expected_zero_seed &&
            ld(post_store_base_vaddr) == expected_post_store_seed &&
            ld(post_store_alias_vaddr) == expected_post_store_seed &&
            ld(tail_base_vaddr) == expected_tail &&
            ld(tail_alias_vaddr) == expected_tail &&
            ld(adjacent_base_vaddr) == adjacent_seeds[round] &&
            ld(adjacent_alias_vaddr) == adjacent_seeds[round] &&
            ld(marker_base_vaddr) == marker_seeds[round] &&
            ld(marker_alias_vaddr) == marker_seeds[round]
        );

        TEST_SETUP_EXCEPT();
        ai_micro_diffsync_cbo_inval_prefetch_zero_cbom_post_store_window(
            inval_alias_vaddr, prefetch_base_vaddr, zero_alias_vaddr,
            clean_base_vaddr, tail_alias_vaddr, post_store_alias_vaddr,
            marker_alias_vaddr, post_cbom_updates[round],
            marker_updates[round], &post_store_observed, &marker_observed);
        AI_ASSERT_NO_EXCEPT_AND("cbo.inval through a same-paddr alias followed by base-view prefetch_r must let alias-view cbo.zero commit before base/alias CBOM and the post-CBOM alias store progress",
            post_store_observed == post_cbom_updates[round] &&
            marker_observed == marker_updates[round] &&
            ld(target_base_vaddr) == 0ULL &&
            ld(target_alias_vaddr) == 0ULL &&
            ld(clean_base_vaddr) == 0ULL &&
            ld(clean_alias_vaddr) == 0ULL &&
            ld(zero_base_vaddr) == 0ULL &&
            ld(zero_alias_vaddr) == 0ULL &&
            ld(post_store_base_vaddr) == post_cbom_updates[round] &&
            ld(post_store_alias_vaddr) == post_cbom_updates[round] &&
            ld(tail_base_vaddr) == 0ULL &&
            ld(tail_alias_vaddr) == 0ULL &&
            ld(marker_base_vaddr) == marker_updates[round] &&
            ld(marker_alias_vaddr) == marker_updates[round]
        );

        TEST_SETUP_EXCEPT();
        AI_ASSERT_NO_EXCEPT_AND("the same-paddr-alias zero-to-CBOM post-store owner handoff preserves guards and the adjacent cache block through both views",
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
        TEST_ASSERT("backing memory after the same-paddr-alias cbo.inval + prefetch_r + cbo.zero + CBOM + post-store handoff contains only the post-CBOM store over an otherwise zeroed block, marker update, and unchanged guards",
            ai_micro_diffsync_block_all_zero_except_word(target_base_paddr,
                5U, post_cbom_updates[round]) &&
            ai_micro_diffsync_block_matches_seed(adjacent_base_paddr,
                adjacent_seeds[round]) &&
            read64(before_guard_paddr) == before_guards[round] &&
            read64(after_guard_paddr) == after_guards[round] &&
            read64(marker_paddr) == marker_updates[round] &&
            read64(marker_guard_paddr) == marker_guards[round]
        );
    }

    ai_micro_diffsync_reset_env(
        MENVCFG_CBIE01 | MENVCFG_CBZE | MENVCFG_CBZFE);

    TEST_END();
}

bool ai_micro_hs_cbo_inval_prefetch_same_paddr_alias_refill_then_cbom_cbo_zero_post_store_no_stale_cmo_zero_owner_or_marker_drop_corner()
{
    TEST_START();

    const enum test_page base_page = SWITCH1;
    const enum test_page alias_page = SWITCH2;
    const uintptr_t target_offset = 0x940U;
    const uintptr_t target_base_vaddr =
        hs_page_base(base_page) + target_offset;
    const uintptr_t target_base_paddr =
        phys_page_base(base_page) + target_offset;
    const uintptr_t prefetch_base_vaddr =
        target_base_vaddr + sizeof(uint64_t);
    const uintptr_t clean_base_vaddr =
        target_base_vaddr + 3U * sizeof(uint64_t);
    const uintptr_t zero_base_vaddr =
        target_base_vaddr + 4U * sizeof(uint64_t);
    const uintptr_t post_store_base_vaddr =
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
        0x3e4f5061728394a5ULL,
        0x4f5061728394a5b6ULL,
        0x5061728394a5b6c7ULL,
    };
    const uint64_t adjacent_seeds[] = {
        0xfedcba9876543210ULL,
        0x0123456789abcdefULL,
        0x55aa7733cc11ee99ULL,
    };
    const uint64_t post_zero_updates[] = {
        0x13579bdf2468ace0ULL,
        0x2468ace013579bdfULL,
        0x3579bdf02468ace1ULL,
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
        0xd2d3d4d5d6d7d8d9ULL,
        0xe2e3e4e5e6e7e8e9ULL,
        0xf2f3f4f5f6f7f8f9ULL,
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
    uintptr_t zero_alias_vaddr = 0;
    uintptr_t post_store_alias_vaddr = 0;
    uintptr_t tail_alias_vaddr = 0;
    uintptr_t before_guard_alias_vaddr = 0;
    uintptr_t after_guard_alias_vaddr = 0;
    uintptr_t adjacent_alias_vaddr = 0;
    uintptr_t marker_alias_vaddr = 0;
    uintptr_t marker_guard_alias_vaddr = 0;

    ai_micro_diffsync_prepare_hs_env(
        MENVCFG_CBIE01 | MENVCFG_CBZE | MENVCFG_CBZFE);
    alias_page_base_vaddr =
        ai_micro_diffsync_prepare_same_paddr_alias(alias_page, base_page);
    target_alias_vaddr = alias_page_base_vaddr + target_offset;
    inval_alias_vaddr =
        target_alias_vaddr + 6U * sizeof(uint64_t);
    clean_alias_vaddr =
        target_alias_vaddr + 3U * sizeof(uint64_t);
    zero_alias_vaddr =
        target_alias_vaddr + 4U * sizeof(uint64_t);
    post_store_alias_vaddr =
        target_alias_vaddr + 5U * sizeof(uint64_t);
    tail_alias_vaddr =
        target_alias_vaddr + (AI_MICRO_DIFFSYNC_BLOCK_WORDS - 1U) *
        sizeof(uint64_t);
    before_guard_alias_vaddr =
        target_alias_vaddr - sizeof(uint64_t);
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
        const uint64_t expected_zero_seed =
            ai_micro_diffsync_seed_word(block_seeds[round], 4U);
        const uint64_t expected_post_store_seed =
            ai_micro_diffsync_seed_word(block_seeds[round], 5U);
        const uint64_t expected_tail =
            ai_micro_diffsync_seed_word(block_seeds[round],
                AI_MICRO_DIFFSYNC_BLOCK_WORDS - 1U);
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
        AI_ASSERT_NO_EXCEPT_AND("before the same-paddr-alias cbo.inval + prefetch_r + CBOM + cbo.zero post-store handoff starts, base and alias views expose seeded target, clean/zero/post-store probes, tail, adjacent block, and marker",
            ld(target_base_vaddr) == block_seeds[round] &&
            ld(target_alias_vaddr) == block_seeds[round] &&
            ld(clean_base_vaddr) == expected_clean_seed &&
            ld(clean_alias_vaddr) == expected_clean_seed &&
            ld(zero_base_vaddr) == expected_zero_seed &&
            ld(zero_alias_vaddr) == expected_zero_seed &&
            ld(post_store_base_vaddr) == expected_post_store_seed &&
            ld(post_store_alias_vaddr) == expected_post_store_seed &&
            ld(tail_base_vaddr) == expected_tail &&
            ld(tail_alias_vaddr) == expected_tail &&
            ld(adjacent_base_vaddr) == adjacent_seeds[round] &&
            ld(adjacent_alias_vaddr) == adjacent_seeds[round] &&
            ld(marker_base_vaddr) == marker_seeds[round] &&
            ld(marker_alias_vaddr) == marker_seeds[round]
        );

        TEST_SETUP_EXCEPT();
        ai_micro_diffsync_cbo_inval_prefetch_cbom_zero_post_store_window(
            inval_alias_vaddr, prefetch_base_vaddr, clean_base_vaddr,
            tail_alias_vaddr, zero_alias_vaddr, post_store_alias_vaddr,
            marker_alias_vaddr, post_zero_updates[round],
            marker_updates[round], &post_store_observed, &marker_observed);
        AI_ASSERT_NO_EXCEPT_AND("cbo.inval through a same-paddr alias followed by base-view prefetch_r must let base/alias CBOM finish before alias-view cbo.zero clears the block and the post-zero alias store progresses",
            post_store_observed == post_zero_updates[round] &&
            marker_observed == marker_updates[round] &&
            ld(target_base_vaddr) == 0ULL &&
            ld(target_alias_vaddr) == 0ULL &&
            ld(clean_base_vaddr) == 0ULL &&
            ld(clean_alias_vaddr) == 0ULL &&
            ld(zero_base_vaddr) == 0ULL &&
            ld(zero_alias_vaddr) == 0ULL &&
            ld(post_store_base_vaddr) == post_zero_updates[round] &&
            ld(post_store_alias_vaddr) == post_zero_updates[round] &&
            ld(tail_base_vaddr) == 0ULL &&
            ld(tail_alias_vaddr) == 0ULL &&
            ld(marker_base_vaddr) == marker_updates[round] &&
            ld(marker_alias_vaddr) == marker_updates[round]
        );

        TEST_SETUP_EXCEPT();
        AI_ASSERT_NO_EXCEPT_AND("the same-paddr-alias CBOM-to-zero post-store owner handoff preserves guards and the adjacent cache block through both views",
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
        TEST_ASSERT("backing memory after the same-paddr-alias cbo.inval + prefetch_r + CBOM + cbo.zero + post-store handoff contains only the post-zero store over an otherwise zeroed block, marker update, and unchanged guards",
            ai_micro_diffsync_block_all_zero_except_word(target_base_paddr,
                5U, post_zero_updates[round]) &&
            ai_micro_diffsync_block_matches_seed(adjacent_base_paddr,
                adjacent_seeds[round]) &&
            read64(before_guard_paddr) == before_guards[round] &&
            read64(after_guard_paddr) == after_guards[round] &&
            read64(marker_paddr) == marker_updates[round] &&
            read64(marker_guard_paddr) == marker_guards[round]
        );
    }

    ai_micro_diffsync_reset_env(
        MENVCFG_CBIE01 | MENVCFG_CBZE | MENVCFG_CBZFE);

    TEST_END();
}

bool ai_micro_hs_cbo_inval_prefetch_same_paddr_alias_refill_then_cbo_zero_cbo_inval_post_store_no_stale_zero_inval_owner_or_marker_drop_corner()
{
    TEST_START();

    const enum test_page base_page = SWITCH1;
    const enum test_page alias_page = SWITCH2;
    const uintptr_t target_offset = 0x980U;
    const uintptr_t target_base_vaddr =
        hs_page_base(base_page) + target_offset;
    const uintptr_t target_base_paddr =
        phys_page_base(base_page) + target_offset;
    const uintptr_t prefetch_base_vaddr =
        target_base_vaddr + sizeof(uint64_t);
    const uintptr_t zero_base_vaddr =
        target_base_vaddr + 4U * sizeof(uint64_t);
    const uintptr_t post_store_base_vaddr =
        target_base_vaddr + 5U * sizeof(uint64_t);
    const uintptr_t second_inval_base_vaddr =
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
        0x1021324354657687ULL,
        0x98badcfe10325476ULL,
        0x7766554433221100ULL,
    };
    const uint64_t post_inval_updates[] = {
        0x88442211ddccbbaaULL,
        0x77553311ccaaffeeULL,
        0x66442200bb99ddffULL,
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
        0x0a0b0c0d0e0f1011ULL,
        0x1a1b1c1d1e1f2021ULL,
        0x2a2b2c2d2e2f3031ULL,
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
    uintptr_t zero_alias_vaddr = 0;
    uintptr_t post_store_alias_vaddr = 0;
    uintptr_t second_inval_alias_vaddr = 0;
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
        target_alias_vaddr + 6U * sizeof(uint64_t);
    zero_alias_vaddr =
        target_alias_vaddr + 4U * sizeof(uint64_t);
    post_store_alias_vaddr =
        target_alias_vaddr + 5U * sizeof(uint64_t);
    second_inval_alias_vaddr =
        target_alias_vaddr + (AI_MICRO_DIFFSYNC_BLOCK_WORDS - 1U) *
        sizeof(uint64_t);
    before_guard_alias_vaddr =
        target_alias_vaddr - sizeof(uint64_t);
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
        const uint64_t expected_zero_seed =
            ai_micro_diffsync_seed_word(block_seeds[round], 4U);
        const uint64_t expected_post_store_seed =
            ai_micro_diffsync_seed_word(block_seeds[round], 5U);
        const uint64_t expected_second_inval_seed =
            ai_micro_diffsync_seed_word(block_seeds[round],
                AI_MICRO_DIFFSYNC_BLOCK_WORDS - 1U);
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
        AI_ASSERT_NO_EXCEPT_AND("before the same-paddr-alias cbo.inval + prefetch_r + cbo.zero + second cbo.inval post-store handoff starts, base and alias views expose seeded target, zero/post-store/inval probes, adjacent block, and marker",
            ld(target_base_vaddr) == block_seeds[round] &&
            ld(target_alias_vaddr) == block_seeds[round] &&
            ld(zero_base_vaddr) == expected_zero_seed &&
            ld(zero_alias_vaddr) == expected_zero_seed &&
            ld(post_store_base_vaddr) == expected_post_store_seed &&
            ld(post_store_alias_vaddr) == expected_post_store_seed &&
            ld(second_inval_base_vaddr) == expected_second_inval_seed &&
            ld(second_inval_alias_vaddr) == expected_second_inval_seed &&
            ld(adjacent_base_vaddr) == adjacent_seeds[round] &&
            ld(adjacent_alias_vaddr) == adjacent_seeds[round] &&
            ld(marker_base_vaddr) == marker_seeds[round] &&
            ld(marker_alias_vaddr) == marker_seeds[round]
        );

        TEST_SETUP_EXCEPT();
        ai_micro_diffsync_cbo_inval_prefetch_zero_inval_post_store_window(
            inval_alias_vaddr, prefetch_base_vaddr, zero_alias_vaddr,
            second_inval_alias_vaddr, post_store_alias_vaddr,
            marker_alias_vaddr, post_inval_updates[round],
            marker_updates[round], &post_store_observed, &marker_observed);
        AI_ASSERT_NO_EXCEPT_AND("cbo.inval through a same-paddr alias followed by base-view prefetch_r must let alias-view cbo.zero commit before the second alias-view cbo.inval releases the post-inval store and marker",
            post_store_observed == post_inval_updates[round] &&
            marker_observed == marker_updates[round] &&
            ld(target_base_vaddr) == 0ULL &&
            ld(target_alias_vaddr) == 0ULL &&
            ld(zero_base_vaddr) == 0ULL &&
            ld(zero_alias_vaddr) == 0ULL &&
            ld(post_store_base_vaddr) == post_inval_updates[round] &&
            ld(post_store_alias_vaddr) == post_inval_updates[round] &&
            ld(second_inval_base_vaddr) == 0ULL &&
            ld(second_inval_alias_vaddr) == 0ULL &&
            ld(marker_base_vaddr) == marker_updates[round] &&
            ld(marker_alias_vaddr) == marker_updates[round]
        );

        TEST_SETUP_EXCEPT();
        AI_ASSERT_NO_EXCEPT_AND("the same-paddr-alias zero-to-second-inval post-store owner handoff preserves guards and the adjacent cache block through both views",
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
        TEST_ASSERT("backing memory after the same-paddr-alias cbo.inval + prefetch_r + cbo.zero + second cbo.inval + post-store handoff contains only the post-inval store over an otherwise zeroed block, marker update, and unchanged guards",
            ai_micro_diffsync_block_all_zero_except_word(target_base_paddr,
                5U, post_inval_updates[round]) &&
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

bool ai_micro_hs_cbo_inval_prefetch_same_paddr_alias_refill_then_cbo_inval_cbo_zero_post_store_no_stale_inval_zero_owner_or_marker_drop_corner()
{
    TEST_START();

    const enum test_page base_page = SWITCH1;
    const enum test_page alias_page = SWITCH2;
    const uintptr_t target_offset = 0x9c0U;
    const uintptr_t target_base_vaddr =
        hs_page_base(base_page) + target_offset;
    const uintptr_t target_base_paddr =
        phys_page_base(base_page) + target_offset;
    const uintptr_t prefetch_base_vaddr =
        target_base_vaddr + sizeof(uint64_t);
    const uintptr_t zero_base_vaddr =
        target_base_vaddr + 4U * sizeof(uint64_t);
    const uintptr_t post_store_base_vaddr =
        target_base_vaddr + 5U * sizeof(uint64_t);
    const uintptr_t second_inval_base_vaddr =
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
        0x94a5b6c7d8e9fa0bULL,
        0xa5b6c7d8e9fa0b1cULL,
        0xb6c7d8e9fa0b1c2dULL,
    };
    const uint64_t adjacent_seeds[] = {
        0x8899aabbccddeeffULL,
        0x7766554433221100ULL,
        0x1029384756657483ULL,
    };
    const uint64_t post_zero_updates[] = {
        0x4d3c2b1a88776655ULL,
        0x5e4d3c2b77665544ULL,
        0x6f5e4d3c66554433ULL,
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
        0x3a3b3c3d3e3f4041ULL,
        0x4a4b4c4d4e4f5051ULL,
        0x5a5b5c5d5e5f6061ULL,
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
    uintptr_t zero_alias_vaddr = 0;
    uintptr_t post_store_alias_vaddr = 0;
    uintptr_t second_inval_alias_vaddr = 0;
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
        target_alias_vaddr + 6U * sizeof(uint64_t);
    second_inval_alias_vaddr =
        target_alias_vaddr + (AI_MICRO_DIFFSYNC_BLOCK_WORDS - 1U) *
        sizeof(uint64_t);
    zero_alias_vaddr =
        target_alias_vaddr + 4U * sizeof(uint64_t);
    post_store_alias_vaddr =
        target_alias_vaddr + 5U * sizeof(uint64_t);
    before_guard_alias_vaddr =
        target_alias_vaddr - sizeof(uint64_t);
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
        const uint64_t expected_zero_seed =
            ai_micro_diffsync_seed_word(block_seeds[round], 4U);
        const uint64_t expected_post_store_seed =
            ai_micro_diffsync_seed_word(block_seeds[round], 5U);
        const uint64_t expected_second_inval_seed =
            ai_micro_diffsync_seed_word(block_seeds[round],
                AI_MICRO_DIFFSYNC_BLOCK_WORDS - 1U);
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
        AI_ASSERT_NO_EXCEPT_AND("before the same-paddr-alias cbo.inval + prefetch_r + second cbo.inval + cbo.zero post-store handoff starts, base and alias views expose seeded target, zero/post-store/inval probes, adjacent block, and marker",
            ld(target_base_vaddr) == block_seeds[round] &&
            ld(target_alias_vaddr) == block_seeds[round] &&
            ld(zero_base_vaddr) == expected_zero_seed &&
            ld(zero_alias_vaddr) == expected_zero_seed &&
            ld(post_store_base_vaddr) == expected_post_store_seed &&
            ld(post_store_alias_vaddr) == expected_post_store_seed &&
            ld(second_inval_base_vaddr) == expected_second_inval_seed &&
            ld(second_inval_alias_vaddr) == expected_second_inval_seed &&
            ld(adjacent_base_vaddr) == adjacent_seeds[round] &&
            ld(adjacent_alias_vaddr) == adjacent_seeds[round] &&
            ld(marker_base_vaddr) == marker_seeds[round] &&
            ld(marker_alias_vaddr) == marker_seeds[round]
        );

        TEST_SETUP_EXCEPT();
        ai_micro_diffsync_cbo_inval_prefetch_inval_zero_post_store_window(
            inval_alias_vaddr, prefetch_base_vaddr,
            second_inval_alias_vaddr, zero_alias_vaddr,
            post_store_alias_vaddr, marker_alias_vaddr,
            post_zero_updates[round], marker_updates[round],
            &post_store_observed, &marker_observed);
        AI_ASSERT_NO_EXCEPT_AND("cbo.inval through a same-paddr alias followed by base-view prefetch_r and a second alias-view cbo.inval must still let alias-view cbo.zero clear the block before the post-zero store and marker progress",
            post_store_observed == post_zero_updates[round] &&
            marker_observed == marker_updates[round] &&
            ld(target_base_vaddr) == 0ULL &&
            ld(target_alias_vaddr) == 0ULL &&
            ld(zero_base_vaddr) == 0ULL &&
            ld(zero_alias_vaddr) == 0ULL &&
            ld(post_store_base_vaddr) == post_zero_updates[round] &&
            ld(post_store_alias_vaddr) == post_zero_updates[round] &&
            ld(second_inval_base_vaddr) == 0ULL &&
            ld(second_inval_alias_vaddr) == 0ULL &&
            ld(marker_base_vaddr) == marker_updates[round] &&
            ld(marker_alias_vaddr) == marker_updates[round]
        );

        TEST_SETUP_EXCEPT();
        AI_ASSERT_NO_EXCEPT_AND("the same-paddr-alias repeated-inval-to-zero post-store owner handoff preserves guards and the adjacent cache block through both views",
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
        TEST_ASSERT("backing memory after the same-paddr-alias cbo.inval + prefetch_r + second cbo.inval + cbo.zero + post-store handoff contains only the post-zero store over an otherwise zeroed block, marker update, and unchanged guards",
            ai_micro_diffsync_block_all_zero_except_word(target_base_paddr,
                5U, post_zero_updates[round]) &&
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

bool ai_micro_hs_cbo_inval_prefetch_same_paddr_alias_refill_then_cbo_zero_cbo_zero_post_store_no_stale_repeated_zero_owner_or_marker_drop_corner()
{
    TEST_START();

    const enum test_page base_page = SWITCH1;
    const enum test_page alias_page = SWITCH2;
    const uintptr_t target_offset = 0xa00U;
    const uintptr_t target_base_vaddr =
        hs_page_base(base_page) + target_offset;
    const uintptr_t target_base_paddr =
        phys_page_base(base_page) + target_offset;
    const uintptr_t prefetch_base_vaddr =
        target_base_vaddr + sizeof(uint64_t);
    const uintptr_t first_zero_base_vaddr =
        target_base_vaddr + 4U * sizeof(uint64_t);
    const uintptr_t post_store_base_vaddr =
        target_base_vaddr + 5U * sizeof(uint64_t);
    const uintptr_t second_zero_base_vaddr =
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
        0xc7d8e9fa0b1c2d3eULL,
        0xd8e9fa0b1c2d3e4fULL,
        0xe9fa0b1c2d3e4f50ULL,
    };
    const uint64_t adjacent_seeds[] = {
        0x0123456789abcdefULL,
        0xfedcba9876543210ULL,
        0x13579bdf2468ace0ULL,
    };
    const uint64_t post_second_zero_updates[] = {
        0x778899aabbccddeeULL,
        0x66778899aabbccddULL,
        0x5566778899aabbccULL,
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
        0x6a6b6c6d6e6f7071ULL,
        0x7a7b7c7d7e7f8081ULL,
        0x8a8b8c8d8e8f9091ULL,
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
    uintptr_t first_zero_alias_vaddr = 0;
    uintptr_t post_store_alias_vaddr = 0;
    uintptr_t second_zero_alias_vaddr = 0;
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
        target_alias_vaddr + 6U * sizeof(uint64_t);
    first_zero_alias_vaddr =
        target_alias_vaddr + 4U * sizeof(uint64_t);
    post_store_alias_vaddr =
        target_alias_vaddr + 5U * sizeof(uint64_t);
    second_zero_alias_vaddr =
        target_alias_vaddr + (AI_MICRO_DIFFSYNC_BLOCK_WORDS - 1U) *
        sizeof(uint64_t);
    before_guard_alias_vaddr =
        target_alias_vaddr - sizeof(uint64_t);
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
        const uint64_t expected_first_zero_seed =
            ai_micro_diffsync_seed_word(block_seeds[round], 4U);
        const uint64_t expected_post_store_seed =
            ai_micro_diffsync_seed_word(block_seeds[round], 5U);
        const uint64_t expected_second_zero_seed =
            ai_micro_diffsync_seed_word(block_seeds[round],
                AI_MICRO_DIFFSYNC_BLOCK_WORDS - 1U);
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
        AI_ASSERT_NO_EXCEPT_AND("before the same-paddr-alias cbo.inval + prefetch_r + repeated cbo.zero post-store handoff starts, base and alias views expose seeded target, zero probes, post-store word, adjacent block, and marker",
            ld(target_base_vaddr) == block_seeds[round] &&
            ld(target_alias_vaddr) == block_seeds[round] &&
            ld(first_zero_base_vaddr) == expected_first_zero_seed &&
            ld(first_zero_alias_vaddr) == expected_first_zero_seed &&
            ld(post_store_base_vaddr) == expected_post_store_seed &&
            ld(post_store_alias_vaddr) == expected_post_store_seed &&
            ld(second_zero_base_vaddr) == expected_second_zero_seed &&
            ld(second_zero_alias_vaddr) == expected_second_zero_seed &&
            ld(adjacent_base_vaddr) == adjacent_seeds[round] &&
            ld(adjacent_alias_vaddr) == adjacent_seeds[round] &&
            ld(marker_base_vaddr) == marker_seeds[round] &&
            ld(marker_alias_vaddr) == marker_seeds[round]
        );

        TEST_SETUP_EXCEPT();
        ai_micro_diffsync_cbo_inval_prefetch_zero_zero_post_store_window(
            inval_alias_vaddr, prefetch_base_vaddr,
            first_zero_alias_vaddr, second_zero_alias_vaddr,
            post_store_alias_vaddr, marker_alias_vaddr,
            post_second_zero_updates[round], marker_updates[round],
            &post_store_observed, &marker_observed);
        AI_ASSERT_NO_EXCEPT_AND("cbo.inval through a same-paddr alias followed by base-view prefetch_r and two alias-view cbo.zero operations must leave the block zeroed before the post-second-zero store and marker progress",
            post_store_observed == post_second_zero_updates[round] &&
            marker_observed == marker_updates[round] &&
            ld(target_base_vaddr) == 0ULL &&
            ld(target_alias_vaddr) == 0ULL &&
            ld(first_zero_base_vaddr) == 0ULL &&
            ld(first_zero_alias_vaddr) == 0ULL &&
            ld(post_store_base_vaddr) == post_second_zero_updates[round] &&
            ld(post_store_alias_vaddr) == post_second_zero_updates[round] &&
            ld(second_zero_base_vaddr) == 0ULL &&
            ld(second_zero_alias_vaddr) == 0ULL &&
            ld(marker_base_vaddr) == marker_updates[round] &&
            ld(marker_alias_vaddr) == marker_updates[round]
        );

        TEST_SETUP_EXCEPT();
        AI_ASSERT_NO_EXCEPT_AND("the same-paddr-alias repeated-zero post-store owner handoff preserves guards and the adjacent cache block through both views",
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
        TEST_ASSERT("backing memory after the same-paddr-alias cbo.inval + prefetch_r + repeated cbo.zero + post-second-zero store handoff contains only the post store over an otherwise zeroed block, marker update, and unchanged guards",
            ai_micro_diffsync_block_all_zero_except_word(target_base_paddr,
                5U, post_second_zero_updates[round]) &&
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
