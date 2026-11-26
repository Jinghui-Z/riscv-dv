#include "ai_micro_memblock_cmo_triple_common.h"

bool ai_micro_hs_interleaved_three_block_store_cbom_store_cbom_store_cbom_store_inval_four_generation_no_zero_corner()
{
    TEST_START();

    const enum test_page source_page = SWITCH1;
    const enum test_page marker_page = SWITCH2;
    const size_t block0_fourth_store_word = 0U;
    const size_t block0_older_store_word = 1U;
    const size_t block0_seed_load_word = 2U;
    const size_t block0_second_store_word = 4U;
    const size_t block0_third_store_word = 6U;
    const size_t block1_load_word = 3U;
    const size_t block2_load_word = 5U;
    const uintptr_t block0_vaddr = hs_page_base(source_page) + 0x2e00U;
    const uintptr_t block1_vaddr = hs_page_base(source_page) + 0x2e80U;
    const uintptr_t block2_vaddr = hs_page_base(source_page) + 0x2f00U;
    const uintptr_t block0_paddr = phys_page_base(source_page) + 0x2e00U;
    const uintptr_t block1_paddr = phys_page_base(source_page) + 0x2e80U;
    const uintptr_t block2_paddr = phys_page_base(source_page) + 0x2f00U;
    const uintptr_t block0_fourth_store_vaddr =
        block0_vaddr + block0_fourth_store_word * sizeof(uint64_t);
    const uintptr_t block0_older_store_vaddr =
        block0_vaddr + block0_older_store_word * sizeof(uint64_t);
    const uintptr_t block0_seed_load_vaddr =
        block0_vaddr + block0_seed_load_word * sizeof(uint64_t);
    const uintptr_t block0_second_store_vaddr =
        block0_vaddr + block0_second_store_word * sizeof(uint64_t);
    const uintptr_t block0_third_store_vaddr =
        block0_vaddr + block0_third_store_word * sizeof(uint64_t);
    const uintptr_t block1_load_vaddr =
        block1_vaddr + block1_load_word * sizeof(uint64_t);
    const uintptr_t block2_load_vaddr =
        block2_vaddr + block2_load_word * sizeof(uint64_t);
    const uintptr_t before_guard_vaddr = block0_vaddr - sizeof(uint64_t);
    const uintptr_t guard01_vaddr =
        block0_vaddr +
        AI_MEMBLOCK_CMO_TRIPLE_BLOCK_WORDS * sizeof(uint64_t);
    const uintptr_t guard12_vaddr =
        block1_vaddr +
        AI_MEMBLOCK_CMO_TRIPLE_BLOCK_WORDS * sizeof(uint64_t);
    const uintptr_t after_guard_vaddr =
        block2_vaddr +
        AI_MEMBLOCK_CMO_TRIPLE_BLOCK_WORDS * sizeof(uint64_t);
    const uintptr_t before_guard_paddr = block0_paddr - sizeof(uint64_t);
    const uintptr_t guard01_paddr =
        block0_paddr +
        AI_MEMBLOCK_CMO_TRIPLE_BLOCK_WORDS * sizeof(uint64_t);
    const uintptr_t guard12_paddr =
        block1_paddr +
        AI_MEMBLOCK_CMO_TRIPLE_BLOCK_WORDS * sizeof(uint64_t);
    const uintptr_t after_guard_paddr =
        block2_paddr +
        AI_MEMBLOCK_CMO_TRIPLE_BLOCK_WORDS * sizeof(uint64_t);
    const uintptr_t marker_vaddr = hs_page_base(marker_page) + 0x2e00U;
    const uintptr_t marker_paddr = phys_page_base(marker_page) + 0x2e00U;
    const uintptr_t marker_guard_vaddr = marker_vaddr + sizeof(uint64_t);
    const uintptr_t marker_guard_paddr = marker_paddr + sizeof(uint64_t);
    const uint64_t block0_seeds[AI_MEMBLOCK_CMO_TRIPLE_ROUNDS] = {
        0x2e003e004e005e00ULL,
        0x3f114f115f116f11ULL,
        0x5022602270228022ULL,
    };
    const uint64_t block1_seeds[AI_MEMBLOCK_CMO_TRIPLE_ROUNDS] = {
        0x807060502e002e80ULL,
        0x918171613f113f91ULL,
        0xa2928272502250a2ULL,
    };
    const uint64_t block2_seeds[AI_MEMBLOCK_CMO_TRIPLE_ROUNDS] = {
        0x2468ace02e002f00ULL,
        0x3579bdf13f114011ULL,
        0x468ace0250225122ULL,
    };
    const uint64_t block0_older_store_values[AI_MEMBLOCK_CMO_TRIPLE_ROUNDS] = {
        0x11002e0022003300ULL,
        0x22113f1133114411ULL,
        0x3322502244225522ULL,
    };
    const uint64_t block0_second_store_values[AI_MEMBLOCK_CMO_TRIPLE_ROUNDS] = {
        0x00330022002e0011ULL,
        0x11441133113f1122ULL,
        0x2255224422502233ULL,
    };
    const uint64_t block0_third_store_values[AI_MEMBLOCK_CMO_TRIPLE_ROUNDS] = {
        0xcafe2e00feed2e00ULL,
        0xdb0f3f110ffe3f11ULL,
        0xec205022200f5022ULL,
    };
    const uint64_t block0_fourth_store_values[AI_MEMBLOCK_CMO_TRIPLE_ROUNDS] = {
        0xc0012e00deadbeefULL,
        0xd1123f11efbedf00ULL,
        0xe2235022f0cfe011ULL,
    };
    const uint64_t before_guards[AI_MEMBLOCK_CMO_TRIPLE_ROUNDS] = {
        0xc0012e00d00d2e00ULL,
        0xd1123f11e11e3f11ULL,
        0xe2235022f22f5022ULL,
    };
    const uint64_t guard01_values[AI_MEMBLOCK_CMO_TRIPLE_ROUNDS] = {
        0x002ed00d002ec001ULL,
        0x113fe11e113fd112ULL,
        0x2250f22f2250e223ULL,
    };
    const uint64_t guard12_values[AI_MEMBLOCK_CMO_TRIPLE_ROUNDS] = {
        0x314159262e002e80ULL,
        0x271828183f113f91ULL,
        0x16180339502250a2ULL,
    };
    const uint64_t after_guards[AI_MEMBLOCK_CMO_TRIPLE_ROUNDS] = {
        0x082e002e96269513ULL,
        0x193f113f81828272ULL,
        0x2a50225093038161ULL,
    };
    const uint64_t marker_seeds[AI_MEMBLOCK_CMO_TRIPLE_ROUNDS] = {
        0x0123456789abcdefULL,
        0x123456789abcdef0ULL,
        0x23456789abcdef01ULL,
    };
    const uint64_t marker_updates[AI_MEMBLOCK_CMO_TRIPLE_ROUNDS] = {
        0xfedcba9876543210ULL,
        0xedcba98765432101ULL,
        0xdcba987654321012ULL,
    };
    const uint64_t marker_guards[AI_MEMBLOCK_CMO_TRIPLE_ROUNDS] = {
        0x55aa55aa2e002e00ULL,
        0x66bb66bb3f113f11ULL,
        0x77cc77cc50225022ULL,
    };

    ai_memblock_cmo_triple_prepare_hs_env();

    for (size_t round = 0; round < AI_MEMBLOCK_CMO_TRIPLE_ROUNDS; ++round) {
        uint64_t block0_older_observed = 0ULL;
        uint64_t block0_second_observed = 0ULL;
        uint64_t block0_third_observed = 0ULL;
        uint64_t block0_fourth_observed = 0ULL;
        uint64_t block0_seed_observed = 0ULL;
        uint64_t block1_observed = 0ULL;
        uint64_t block2_observed = 0ULL;
        uint64_t marker_observed = 0ULL;
        const uint64_t expected_block0_fourth_before =
            block0_seeds[round] +
            block0_fourth_store_word * 0x1111111111111111ULL;
        const uint64_t expected_block0_older_before =
            block0_seeds[round] +
            block0_older_store_word * 0x1111111111111111ULL;
        const uint64_t expected_block0_seed_load =
            block0_seeds[round] +
            block0_seed_load_word * 0x1111111111111111ULL;
        const uint64_t expected_block0_second_before =
            block0_seeds[round] +
            block0_second_store_word * 0x1111111111111111ULL;
        const uint64_t expected_block0_third_before =
            block0_seeds[round] +
            block0_third_store_word * 0x1111111111111111ULL;
        const uint64_t expected_block2_load =
            block2_seeds[round] +
            block2_load_word * 0x1111111111111111ULL;

        goto_priv(PRIV_M);
        ai_memblock_cmo_triple_seed_block(block0_paddr,
            block0_seeds[round]);
        ai_memblock_cmo_triple_seed_block(block1_paddr,
            block1_seeds[round]);
        ai_memblock_cmo_triple_seed_block(block2_paddr,
            block2_seeds[round]);
        write64(before_guard_paddr, before_guards[round]);
        write64(guard01_paddr, guard01_values[round]);
        write64(guard12_paddr, guard12_values[round]);
        write64(after_guard_paddr, after_guards[round]);
        write64(marker_paddr, marker_seeds[round]);
        write64(marker_guard_paddr, marker_guards[round]);

        goto_priv(PRIV_HS);
        TEST_SETUP_EXCEPT();
        AI_ASSERT_NO_EXCEPT_AND("before the four-generation clean-clean-clean-inval CMO probe starts, all source blocks, guards, and marker are readable",
            ld(block0_vaddr) == expected_block0_fourth_before &&
            ld(block0_older_store_vaddr) == expected_block0_older_before &&
            ld(block0_seed_load_vaddr) == expected_block0_seed_load &&
            ld(block0_second_store_vaddr) == expected_block0_second_before &&
            ld(block0_third_store_vaddr) == expected_block0_third_before &&
            ld(block1_vaddr) == block1_seeds[round] &&
            ld(block2_vaddr) == block2_seeds[round] &&
            ld(block2_load_vaddr) == expected_block2_load &&
            ld(marker_vaddr) == marker_seeds[round]
        );

        TEST_SETUP_EXCEPT();
        ai_memblock_cmo_triple_three_block_store_cbom_store_cbom_store_cbom_store_inval_window(
            block0_vaddr, block1_vaddr, block2_vaddr, marker_vaddr,
            block0_older_store_values[round],
            block0_second_store_values[round],
            block0_third_store_values[round],
            block0_fourth_store_values[round], marker_updates[round],
            &block0_older_observed, &block0_second_observed,
            &block0_third_observed, &block0_fourth_observed,
            &block0_seed_observed, &block1_observed, &block2_observed,
            &marker_observed);
        AI_ASSERT_NO_EXCEPT_AND("under three-block CMO pressure, triple CBOM then CBO.INVAL keeps all four same-block store generations visible",
            block0_older_observed == block0_older_store_values[round] &&
            block0_second_observed == block0_second_store_values[round] &&
            block0_third_observed == block0_third_store_values[round] &&
            block0_fourth_observed == block0_fourth_store_values[round] &&
            block0_seed_observed == expected_block0_seed_load &&
            block1_observed == 0ULL &&
            block2_observed == expected_block2_load &&
            marker_observed == marker_updates[round]
        );

        TEST_SETUP_EXCEPT();
        AI_ASSERT_NO_EXCEPT_AND("after the four-generation clean-clean-clean-inval CMO probe, block0 preserves all stores, block1 is zero, block2 preserves seed, and guards remain intact",
            ld(block0_fourth_store_vaddr) ==
                block0_fourth_store_values[round] &&
            ld(block0_older_store_vaddr) ==
                block0_older_store_values[round] &&
            ld(block0_seed_load_vaddr) == expected_block0_seed_load &&
            ld(block0_second_store_vaddr) ==
                block0_second_store_values[round] &&
            ld(block0_third_store_vaddr) ==
                block0_third_store_values[round] &&
            ld(block1_vaddr) == 0ULL &&
            ld(block1_load_vaddr) == 0ULL &&
            ld(block2_vaddr) == block2_seeds[round] &&
            ld(block2_load_vaddr) == expected_block2_load &&
            ld(before_guard_vaddr) == before_guards[round] &&
            ld(guard01_vaddr) == guard01_values[round] &&
            ld(guard12_vaddr) == guard12_values[round] &&
            ld(after_guard_vaddr) == after_guards[round] &&
            ld(marker_guard_vaddr) == marker_guards[round]
        );

        goto_priv(PRIV_M);
        TEST_ASSERT("backing memory after the four-generation clean-clean-clean-inval CMO probe keeps block0 seed except all four store generations, block1 zero, block2 seed payload, marker update, and all guards intact",
            ai_memblock_cmo_triple_block_matches_seed_except_four_words(
                block0_paddr, block0_seeds[round],
                block0_fourth_store_word,
                block0_fourth_store_values[round],
                block0_older_store_word,
                block0_older_store_values[round],
                block0_second_store_word,
                block0_second_store_values[round],
                block0_third_store_word,
                block0_third_store_values[round]) &&
            ai_memblock_cmo_triple_block_all_zero(block1_paddr) &&
            ai_memblock_cmo_triple_block_matches_seed(block2_paddr,
                block2_seeds[round]) &&
            read64(before_guard_paddr) == before_guards[round] &&
            read64(guard01_paddr) == guard01_values[round] &&
            read64(guard12_paddr) == guard12_values[round] &&
            read64(after_guard_paddr) == after_guards[round] &&
            read64(marker_paddr) == marker_updates[round] &&
            read64(marker_guard_paddr) == marker_guards[round]
        );
    }

    ai_memblock_cmo_triple_reset_hs_env();

    TEST_END();
}

bool ai_micro_hs_interleaved_three_block_store_cbom_store_cbom_store_cbom_store_zero_four_generation_final_zero_corner()
{
    TEST_START();

    const enum test_page source_page = SWITCH1;
    const enum test_page marker_page = SWITCH2;
    const size_t block0_fourth_store_word = 0U;
    const size_t block0_older_store_word = 1U;
    const size_t block0_zero_load_word = 2U;
    const size_t block0_second_store_word = 4U;
    const size_t block0_third_store_word = 6U;
    const size_t block1_load_word = 3U;
    const size_t block2_load_word = 5U;
    const uintptr_t block0_vaddr = hs_page_base(source_page) + 0x3000U;
    const uintptr_t block1_vaddr = hs_page_base(source_page) + 0x3080U;
    const uintptr_t block2_vaddr = hs_page_base(source_page) + 0x3100U;
    const uintptr_t block0_paddr = phys_page_base(source_page) + 0x3000U;
    const uintptr_t block1_paddr = phys_page_base(source_page) + 0x3080U;
    const uintptr_t block2_paddr = phys_page_base(source_page) + 0x3100U;
    const uintptr_t block0_fourth_store_vaddr =
        block0_vaddr + block0_fourth_store_word * sizeof(uint64_t);
    const uintptr_t block0_older_store_vaddr =
        block0_vaddr + block0_older_store_word * sizeof(uint64_t);
    const uintptr_t block0_zero_load_vaddr =
        block0_vaddr + block0_zero_load_word * sizeof(uint64_t);
    const uintptr_t block0_second_store_vaddr =
        block0_vaddr + block0_second_store_word * sizeof(uint64_t);
    const uintptr_t block0_third_store_vaddr =
        block0_vaddr + block0_third_store_word * sizeof(uint64_t);
    const uintptr_t block1_load_vaddr =
        block1_vaddr + block1_load_word * sizeof(uint64_t);
    const uintptr_t block2_load_vaddr =
        block2_vaddr + block2_load_word * sizeof(uint64_t);
    const uintptr_t before_guard_vaddr = block0_vaddr - sizeof(uint64_t);
    const uintptr_t guard01_vaddr =
        block0_vaddr +
        AI_MEMBLOCK_CMO_TRIPLE_BLOCK_WORDS * sizeof(uint64_t);
    const uintptr_t guard12_vaddr =
        block1_vaddr +
        AI_MEMBLOCK_CMO_TRIPLE_BLOCK_WORDS * sizeof(uint64_t);
    const uintptr_t after_guard_vaddr =
        block2_vaddr +
        AI_MEMBLOCK_CMO_TRIPLE_BLOCK_WORDS * sizeof(uint64_t);
    const uintptr_t before_guard_paddr = block0_paddr - sizeof(uint64_t);
    const uintptr_t guard01_paddr =
        block0_paddr +
        AI_MEMBLOCK_CMO_TRIPLE_BLOCK_WORDS * sizeof(uint64_t);
    const uintptr_t guard12_paddr =
        block1_paddr +
        AI_MEMBLOCK_CMO_TRIPLE_BLOCK_WORDS * sizeof(uint64_t);
    const uintptr_t after_guard_paddr =
        block2_paddr +
        AI_MEMBLOCK_CMO_TRIPLE_BLOCK_WORDS * sizeof(uint64_t);
    const uintptr_t marker_vaddr = hs_page_base(marker_page) + 0x3000U;
    const uintptr_t marker_paddr = phys_page_base(marker_page) + 0x3000U;
    const uintptr_t marker_guard_vaddr = marker_vaddr + sizeof(uint64_t);
    const uintptr_t marker_guard_paddr = marker_paddr + sizeof(uint64_t);
    const uint64_t block0_seeds[AI_MEMBLOCK_CMO_TRIPLE_ROUNDS] = {
        0x3000400050006000ULL,
        0x4111511161117111ULL,
        0x5222622272228222ULL,
    };
    const uint64_t block1_seeds[AI_MEMBLOCK_CMO_TRIPLE_ROUNDS] = {
        0x8070605030003080ULL,
        0x9181716141114191ULL,
        0xa2928272522252a2ULL,
    };
    const uint64_t block2_seeds[AI_MEMBLOCK_CMO_TRIPLE_ROUNDS] = {
        0x2468ace030003100ULL,
        0x3579bdf141114211ULL,
        0x468ace0252225322ULL,
    };
    const uint64_t block0_older_store_values[AI_MEMBLOCK_CMO_TRIPLE_ROUNDS] = {
        0x1100300022003300ULL,
        0x2211411133114411ULL,
        0x3322522244225522ULL,
    };
    const uint64_t block0_second_store_values[AI_MEMBLOCK_CMO_TRIPLE_ROUNDS] = {
        0x0033002200300011ULL,
        0x1144113311411122ULL,
        0x2255224422522233ULL,
    };
    const uint64_t block0_third_store_values[AI_MEMBLOCK_CMO_TRIPLE_ROUNDS] = {
        0xcafe3000feed3000ULL,
        0xdb0f41110ffe4111ULL,
        0xec205222200f5222ULL,
    };
    const uint64_t block0_fourth_store_values[AI_MEMBLOCK_CMO_TRIPLE_ROUNDS] = {
        0xc0013000deadbeefULL,
        0xd1124111efbedf00ULL,
        0xe2235222f0cfe011ULL,
    };
    const uint64_t before_guards[AI_MEMBLOCK_CMO_TRIPLE_ROUNDS] = {
        0xc0013000d00d3000ULL,
        0xd1124111e11e4111ULL,
        0xe2235222f22f5222ULL,
    };
    const uint64_t guard01_values[AI_MEMBLOCK_CMO_TRIPLE_ROUNDS] = {
        0x0030d00d0030c001ULL,
        0x1141e11e1141d112ULL,
        0x2252f22f2252e223ULL,
    };
    const uint64_t guard12_values[AI_MEMBLOCK_CMO_TRIPLE_ROUNDS] = {
        0x3141592630003080ULL,
        0x2718281841114191ULL,
        0x16180339522252a2ULL,
    };
    const uint64_t after_guards[AI_MEMBLOCK_CMO_TRIPLE_ROUNDS] = {
        0x0830003096269513ULL,
        0x1941114181828272ULL,
        0x2a52225293038161ULL,
    };
    const uint64_t marker_seeds[AI_MEMBLOCK_CMO_TRIPLE_ROUNDS] = {
        0x1023456789abcdefULL,
        0x213456789abcdef0ULL,
        0x32456789abcdef01ULL,
    };
    const uint64_t marker_updates[AI_MEMBLOCK_CMO_TRIPLE_ROUNDS] = {
        0xefdcba9876543210ULL,
        0xdfcba98765432101ULL,
        0xcfba987654321012ULL,
    };
    const uint64_t marker_guards[AI_MEMBLOCK_CMO_TRIPLE_ROUNDS] = {
        0x55aa55aa30003000ULL,
        0x66bb66bb41114111ULL,
        0x77cc77cc52225222ULL,
    };

    ai_memblock_cmo_triple_prepare_hs_env();

    for (size_t round = 0; round < AI_MEMBLOCK_CMO_TRIPLE_ROUNDS; ++round) {
        uint64_t block0_older_observed = 0ULL;
        uint64_t block0_second_observed = 0ULL;
        uint64_t block0_third_observed = 0ULL;
        uint64_t block0_fourth_observed = 0ULL;
        uint64_t block0_zero_observed = 0ULL;
        uint64_t block1_observed = 0ULL;
        uint64_t block2_observed = 0ULL;
        uint64_t marker_observed = 0ULL;
        const uint64_t expected_block0_fourth_before =
            block0_seeds[round] +
            block0_fourth_store_word * 0x1111111111111111ULL;
        const uint64_t expected_block0_older_before =
            block0_seeds[round] +
            block0_older_store_word * 0x1111111111111111ULL;
        const uint64_t expected_block0_zero_before =
            block0_seeds[round] +
            block0_zero_load_word * 0x1111111111111111ULL;
        const uint64_t expected_block0_second_before =
            block0_seeds[round] +
            block0_second_store_word * 0x1111111111111111ULL;
        const uint64_t expected_block0_third_before =
            block0_seeds[round] +
            block0_third_store_word * 0x1111111111111111ULL;
        const uint64_t expected_block2_load =
            block2_seeds[round] +
            block2_load_word * 0x1111111111111111ULL;

        goto_priv(PRIV_M);
        ai_memblock_cmo_triple_seed_block(block0_paddr,
            block0_seeds[round]);
        ai_memblock_cmo_triple_seed_block(block1_paddr,
            block1_seeds[round]);
        ai_memblock_cmo_triple_seed_block(block2_paddr,
            block2_seeds[round]);
        write64(before_guard_paddr, before_guards[round]);
        write64(guard01_paddr, guard01_values[round]);
        write64(guard12_paddr, guard12_values[round]);
        write64(after_guard_paddr, after_guards[round]);
        write64(marker_paddr, marker_seeds[round]);
        write64(marker_guard_paddr, marker_guards[round]);

        goto_priv(PRIV_HS);
        TEST_SETUP_EXCEPT();
        AI_ASSERT_NO_EXCEPT_AND("before the four-generation clean-clean-clean-zero CMO probe starts, all source blocks, guards, and marker are readable",
            ld(block0_vaddr) == expected_block0_fourth_before &&
            ld(block0_older_store_vaddr) == expected_block0_older_before &&
            ld(block0_zero_load_vaddr) == expected_block0_zero_before &&
            ld(block0_second_store_vaddr) == expected_block0_second_before &&
            ld(block0_third_store_vaddr) == expected_block0_third_before &&
            ld(block1_vaddr) == block1_seeds[round] &&
            ld(block2_vaddr) == block2_seeds[round] &&
            ld(block2_load_vaddr) == expected_block2_load &&
            ld(marker_vaddr) == marker_seeds[round]
        );

        TEST_SETUP_EXCEPT();
        ai_memblock_cmo_triple_three_block_store_cbom_store_cbom_store_cbom_store_zero_window(
            block0_vaddr, block1_vaddr, block2_vaddr, marker_vaddr,
            block0_older_store_values[round],
            block0_second_store_values[round],
            block0_third_store_values[round],
            block0_fourth_store_values[round], marker_updates[round],
            &block0_older_observed, &block0_second_observed,
            &block0_third_observed, &block0_fourth_observed,
            &block0_zero_observed, &block1_observed, &block2_observed,
            &marker_observed);
        AI_ASSERT_NO_EXCEPT_AND("under three-block CMO pressure, triple CBOM then CBO.ZERO leaves block0 fully zeroed",
            block0_older_observed == 0ULL &&
            block0_second_observed == 0ULL &&
            block0_third_observed == 0ULL &&
            block0_fourth_observed == 0ULL &&
            block0_zero_observed == 0ULL &&
            block1_observed == 0ULL &&
            block2_observed == expected_block2_load &&
            marker_observed == marker_updates[round]
        );

        TEST_SETUP_EXCEPT();
        AI_ASSERT_NO_EXCEPT_AND("after the four-generation clean-clean-clean-zero CMO probe, final CBO.ZERO clears block0, block1 is zero, block2 preserves seed, and guards remain intact",
            ld(block0_fourth_store_vaddr) == 0ULL &&
            ld(block0_older_store_vaddr) == 0ULL &&
            ld(block0_zero_load_vaddr) == 0ULL &&
            ld(block0_second_store_vaddr) == 0ULL &&
            ld(block0_third_store_vaddr) == 0ULL &&
            ld(block1_vaddr) == 0ULL &&
            ld(block1_load_vaddr) == 0ULL &&
            ld(block2_vaddr) == block2_seeds[round] &&
            ld(block2_load_vaddr) == expected_block2_load &&
            ld(before_guard_vaddr) == before_guards[round] &&
            ld(guard01_vaddr) == guard01_values[round] &&
            ld(guard12_vaddr) == guard12_values[round] &&
            ld(after_guard_vaddr) == after_guards[round] &&
            ld(marker_guard_vaddr) == marker_guards[round]
        );

        goto_priv(PRIV_M);
        TEST_ASSERT("backing memory after the four-generation clean-clean-clean-zero CMO probe keeps block0 zero, block1 zero, block2 seed payload, marker update, and all guards intact",
            ai_memblock_cmo_triple_block_all_zero(block0_paddr) &&
            ai_memblock_cmo_triple_block_all_zero(block1_paddr) &&
            ai_memblock_cmo_triple_block_matches_seed(block2_paddr,
                block2_seeds[round]) &&
            read64(before_guard_paddr) == before_guards[round] &&
            read64(guard01_paddr) == guard01_values[round] &&
            read64(guard12_paddr) == guard12_values[round] &&
            read64(after_guard_paddr) == after_guards[round] &&
            read64(marker_paddr) == marker_updates[round] &&
            read64(marker_guard_paddr) == marker_guards[round]
        );
    }

    ai_memblock_cmo_triple_reset_hs_env();

    TEST_END();
}

bool ai_micro_hs_interleaved_three_block_store_inval_store_inval_store_inval_store_cbom_four_generation_no_zero_corner()
{
    TEST_START();

    const enum test_page source_page = SWITCH1;
    const enum test_page marker_page = SWITCH2;
    const size_t block0_fourth_store_word = 0U;
    const size_t block0_older_store_word = 1U;
    const size_t block0_seed_load_word = 2U;
    const size_t block0_second_store_word = 4U;
    const size_t block0_third_store_word = 6U;
    const size_t block1_load_word = 3U;
    const size_t block2_load_word = 5U;
    const uintptr_t block0_vaddr = hs_page_base(source_page) + 0x3200U;
    const uintptr_t block1_vaddr = hs_page_base(source_page) + 0x3280U;
    const uintptr_t block2_vaddr = hs_page_base(source_page) + 0x3300U;
    const uintptr_t block0_paddr = phys_page_base(source_page) + 0x3200U;
    const uintptr_t block1_paddr = phys_page_base(source_page) + 0x3280U;
    const uintptr_t block2_paddr = phys_page_base(source_page) + 0x3300U;
    const uintptr_t block0_fourth_store_vaddr =
        block0_vaddr + block0_fourth_store_word * sizeof(uint64_t);
    const uintptr_t block0_older_store_vaddr =
        block0_vaddr + block0_older_store_word * sizeof(uint64_t);
    const uintptr_t block0_seed_load_vaddr =
        block0_vaddr + block0_seed_load_word * sizeof(uint64_t);
    const uintptr_t block0_second_store_vaddr =
        block0_vaddr + block0_second_store_word * sizeof(uint64_t);
    const uintptr_t block0_third_store_vaddr =
        block0_vaddr + block0_third_store_word * sizeof(uint64_t);
    const uintptr_t block1_load_vaddr =
        block1_vaddr + block1_load_word * sizeof(uint64_t);
    const uintptr_t block2_load_vaddr =
        block2_vaddr + block2_load_word * sizeof(uint64_t);
    const uintptr_t before_guard_vaddr = block0_vaddr - sizeof(uint64_t);
    const uintptr_t guard01_vaddr =
        block0_vaddr +
        AI_MEMBLOCK_CMO_TRIPLE_BLOCK_WORDS * sizeof(uint64_t);
    const uintptr_t guard12_vaddr =
        block1_vaddr +
        AI_MEMBLOCK_CMO_TRIPLE_BLOCK_WORDS * sizeof(uint64_t);
    const uintptr_t after_guard_vaddr =
        block2_vaddr +
        AI_MEMBLOCK_CMO_TRIPLE_BLOCK_WORDS * sizeof(uint64_t);
    const uintptr_t before_guard_paddr = block0_paddr - sizeof(uint64_t);
    const uintptr_t guard01_paddr =
        block0_paddr +
        AI_MEMBLOCK_CMO_TRIPLE_BLOCK_WORDS * sizeof(uint64_t);
    const uintptr_t guard12_paddr =
        block1_paddr +
        AI_MEMBLOCK_CMO_TRIPLE_BLOCK_WORDS * sizeof(uint64_t);
    const uintptr_t after_guard_paddr =
        block2_paddr +
        AI_MEMBLOCK_CMO_TRIPLE_BLOCK_WORDS * sizeof(uint64_t);
    const uintptr_t marker_vaddr = hs_page_base(marker_page) + 0x3200U;
    const uintptr_t marker_paddr = phys_page_base(marker_page) + 0x3200U;
    const uintptr_t marker_guard_vaddr = marker_vaddr + sizeof(uint64_t);
    const uintptr_t marker_guard_paddr = marker_paddr + sizeof(uint64_t);
    const uint64_t block0_seeds[AI_MEMBLOCK_CMO_TRIPLE_ROUNDS] = {
        0x3200420052006200ULL,
        0x4311531163117311ULL,
        0x5422642274228422ULL,
    };
    const uint64_t block1_seeds[AI_MEMBLOCK_CMO_TRIPLE_ROUNDS] = {
        0x8070605032003280ULL,
        0x9181716143114391ULL,
        0xa2928272542254a2ULL,
    };
    const uint64_t block2_seeds[AI_MEMBLOCK_CMO_TRIPLE_ROUNDS] = {
        0x2468ace032003300ULL,
        0x3579bdf143114411ULL,
        0x468ace0254225522ULL,
    };
    const uint64_t block0_older_store_values[AI_MEMBLOCK_CMO_TRIPLE_ROUNDS] = {
        0x1100320022003300ULL,
        0x2211431133114411ULL,
        0x3322542244225522ULL,
    };
    const uint64_t block0_second_store_values[AI_MEMBLOCK_CMO_TRIPLE_ROUNDS] = {
        0x0033002200320011ULL,
        0x1144113311431122ULL,
        0x2255224422542233ULL,
    };
    const uint64_t block0_third_store_values[AI_MEMBLOCK_CMO_TRIPLE_ROUNDS] = {
        0xcafe3200feed3200ULL,
        0xdb0f43110ffe4311ULL,
        0xec205422200f5422ULL,
    };
    const uint64_t block0_fourth_store_values[AI_MEMBLOCK_CMO_TRIPLE_ROUNDS] = {
        0xc0013200deadbeefULL,
        0xd1124311efbedf00ULL,
        0xe2235422f0cfe011ULL,
    };
    const uint64_t before_guards[AI_MEMBLOCK_CMO_TRIPLE_ROUNDS] = {
        0xc0013200d00d3200ULL,
        0xd1124311e11e4311ULL,
        0xe2235422f22f5422ULL,
    };
    const uint64_t guard01_values[AI_MEMBLOCK_CMO_TRIPLE_ROUNDS] = {
        0x0032d00d0032c001ULL,
        0x1143e11e1143d112ULL,
        0x2254f22f2254e223ULL,
    };
    const uint64_t guard12_values[AI_MEMBLOCK_CMO_TRIPLE_ROUNDS] = {
        0x3141592632003280ULL,
        0x2718281843114391ULL,
        0x16180339542254a2ULL,
    };
    const uint64_t after_guards[AI_MEMBLOCK_CMO_TRIPLE_ROUNDS] = {
        0x0832003296269513ULL,
        0x1943114381828272ULL,
        0x2a54225493038161ULL,
    };
    const uint64_t marker_seeds[AI_MEMBLOCK_CMO_TRIPLE_ROUNDS] = {
        0x89abcdef01234567ULL,
        0x9abcdef012345678ULL,
        0xabcdef0123456789ULL,
    };
    const uint64_t marker_updates[AI_MEMBLOCK_CMO_TRIPLE_ROUNDS] = {
        0x76543210fedcba98ULL,
        0x876543210fedcba9ULL,
        0x9876543210fedcbaULL,
    };
    const uint64_t marker_guards[AI_MEMBLOCK_CMO_TRIPLE_ROUNDS] = {
        0x55aa55aa32003200ULL,
        0x66bb66bb43114311ULL,
        0x77cc77cc54225422ULL,
    };

    ai_memblock_cmo_triple_prepare_hs_env();

    for (size_t round = 0; round < AI_MEMBLOCK_CMO_TRIPLE_ROUNDS; ++round) {
        uint64_t block0_older_observed = 0ULL;
        uint64_t block0_second_observed = 0ULL;
        uint64_t block0_third_observed = 0ULL;
        uint64_t block0_fourth_observed = 0ULL;
        uint64_t block0_seed_observed = 0ULL;
        uint64_t block1_observed = 0ULL;
        uint64_t block2_observed = 0ULL;
        uint64_t marker_observed = 0ULL;
        const uint64_t expected_block0_fourth_before =
            block0_seeds[round] +
            block0_fourth_store_word * 0x1111111111111111ULL;
        const uint64_t expected_block0_older_before =
            block0_seeds[round] +
            block0_older_store_word * 0x1111111111111111ULL;
        const uint64_t expected_block0_seed_load =
            block0_seeds[round] +
            block0_seed_load_word * 0x1111111111111111ULL;
        const uint64_t expected_block0_second_before =
            block0_seeds[round] +
            block0_second_store_word * 0x1111111111111111ULL;
        const uint64_t expected_block0_third_before =
            block0_seeds[round] +
            block0_third_store_word * 0x1111111111111111ULL;
        const uint64_t expected_block2_load =
            block2_seeds[round] +
            block2_load_word * 0x1111111111111111ULL;

        goto_priv(PRIV_M);
        ai_memblock_cmo_triple_seed_block(block0_paddr,
            block0_seeds[round]);
        ai_memblock_cmo_triple_seed_block(block1_paddr,
            block1_seeds[round]);
        ai_memblock_cmo_triple_seed_block(block2_paddr,
            block2_seeds[round]);
        write64(before_guard_paddr, before_guards[round]);
        write64(guard01_paddr, guard01_values[round]);
        write64(guard12_paddr, guard12_values[round]);
        write64(after_guard_paddr, after_guards[round]);
        write64(marker_paddr, marker_seeds[round]);
        write64(marker_guard_paddr, marker_guards[round]);

        goto_priv(PRIV_HS);
        TEST_SETUP_EXCEPT();
        AI_ASSERT_NO_EXCEPT_AND("before the four-generation inval-inval-inval-clean CMO probe starts, all source blocks, guards, and marker are readable",
            ld(block0_vaddr) == expected_block0_fourth_before &&
            ld(block0_older_store_vaddr) == expected_block0_older_before &&
            ld(block0_seed_load_vaddr) == expected_block0_seed_load &&
            ld(block0_second_store_vaddr) == expected_block0_second_before &&
            ld(block0_third_store_vaddr) == expected_block0_third_before &&
            ld(block1_vaddr) == block1_seeds[round] &&
            ld(block2_vaddr) == block2_seeds[round] &&
            ld(block2_load_vaddr) == expected_block2_load &&
            ld(marker_vaddr) == marker_seeds[round]
        );

        TEST_SETUP_EXCEPT();
        ai_memblock_cmo_triple_three_block_store_inval_store_inval_store_inval_store_cbom_window(
            block0_vaddr, block1_vaddr, block2_vaddr, marker_vaddr,
            block0_older_store_values[round],
            block0_second_store_values[round],
            block0_third_store_values[round],
            block0_fourth_store_values[round], marker_updates[round],
            &block0_older_observed, &block0_second_observed,
            &block0_third_observed, &block0_fourth_observed,
            &block0_seed_observed, &block1_observed, &block2_observed,
            &marker_observed);
        AI_ASSERT_NO_EXCEPT_AND("under three-block CMO pressure, triple CBO.INVAL then CBOM keeps all four same-block store generations visible",
            block0_older_observed == block0_older_store_values[round] &&
            block0_second_observed == block0_second_store_values[round] &&
            block0_third_observed == block0_third_store_values[round] &&
            block0_fourth_observed == block0_fourth_store_values[round] &&
            block0_seed_observed == expected_block0_seed_load &&
            block1_observed == 0ULL &&
            block2_observed == expected_block2_load &&
            marker_observed == marker_updates[round]
        );

        TEST_SETUP_EXCEPT();
        AI_ASSERT_NO_EXCEPT_AND("after the four-generation inval-inval-inval-clean CMO probe, block0 preserves all stores, block1 is zero, block2 preserves seed, and guards remain intact",
            ld(block0_fourth_store_vaddr) ==
                block0_fourth_store_values[round] &&
            ld(block0_older_store_vaddr) ==
                block0_older_store_values[round] &&
            ld(block0_seed_load_vaddr) == expected_block0_seed_load &&
            ld(block0_second_store_vaddr) ==
                block0_second_store_values[round] &&
            ld(block0_third_store_vaddr) ==
                block0_third_store_values[round] &&
            ld(block1_vaddr) == 0ULL &&
            ld(block1_load_vaddr) == 0ULL &&
            ld(block2_vaddr) == block2_seeds[round] &&
            ld(block2_load_vaddr) == expected_block2_load &&
            ld(before_guard_vaddr) == before_guards[round] &&
            ld(guard01_vaddr) == guard01_values[round] &&
            ld(guard12_vaddr) == guard12_values[round] &&
            ld(after_guard_vaddr) == after_guards[round] &&
            ld(marker_guard_vaddr) == marker_guards[round]
        );

        goto_priv(PRIV_M);
        TEST_ASSERT("backing memory after the four-generation inval-inval-inval-clean CMO probe keeps block0 seed except all four store generations, block1 zero, block2 seed payload, marker update, and all guards intact",
            ai_memblock_cmo_triple_block_matches_seed_except_four_words(
                block0_paddr, block0_seeds[round],
                block0_fourth_store_word,
                block0_fourth_store_values[round],
                block0_older_store_word,
                block0_older_store_values[round],
                block0_second_store_word,
                block0_second_store_values[round],
                block0_third_store_word,
                block0_third_store_values[round]) &&
            ai_memblock_cmo_triple_block_all_zero(block1_paddr) &&
            ai_memblock_cmo_triple_block_matches_seed(block2_paddr,
                block2_seeds[round]) &&
            read64(before_guard_paddr) == before_guards[round] &&
            read64(guard01_paddr) == guard01_values[round] &&
            read64(guard12_paddr) == guard12_values[round] &&
            read64(after_guard_paddr) == after_guards[round] &&
            read64(marker_paddr) == marker_updates[round] &&
            read64(marker_guard_paddr) == marker_guards[round]
        );
    }

    ai_memblock_cmo_triple_reset_hs_env();

    TEST_END();
}

bool ai_micro_hs_interleaved_three_block_store_inval_store_inval_store_inval_store_zero_four_generation_final_zero_corner()
{
    TEST_START();

    const enum test_page source_page = SWITCH1;
    const enum test_page marker_page = SWITCH2;
    const size_t block0_fourth_store_word = 0U;
    const size_t block0_older_store_word = 1U;
    const size_t block0_zero_load_word = 2U;
    const size_t block0_second_store_word = 4U;
    const size_t block0_third_store_word = 6U;
    const size_t block1_load_word = 3U;
    const size_t block2_load_word = 5U;
    const uintptr_t block0_vaddr = hs_page_base(source_page) + 0x3400U;
    const uintptr_t block1_vaddr = hs_page_base(source_page) + 0x3480U;
    const uintptr_t block2_vaddr = hs_page_base(source_page) + 0x3500U;
    const uintptr_t block0_paddr = phys_page_base(source_page) + 0x3400U;
    const uintptr_t block1_paddr = phys_page_base(source_page) + 0x3480U;
    const uintptr_t block2_paddr = phys_page_base(source_page) + 0x3500U;
    const uintptr_t block0_fourth_store_vaddr =
        block0_vaddr + block0_fourth_store_word * sizeof(uint64_t);
    const uintptr_t block0_older_store_vaddr =
        block0_vaddr + block0_older_store_word * sizeof(uint64_t);
    const uintptr_t block0_zero_load_vaddr =
        block0_vaddr + block0_zero_load_word * sizeof(uint64_t);
    const uintptr_t block0_second_store_vaddr =
        block0_vaddr + block0_second_store_word * sizeof(uint64_t);
    const uintptr_t block0_third_store_vaddr =
        block0_vaddr + block0_third_store_word * sizeof(uint64_t);
    const uintptr_t block1_load_vaddr =
        block1_vaddr + block1_load_word * sizeof(uint64_t);
    const uintptr_t block2_load_vaddr =
        block2_vaddr + block2_load_word * sizeof(uint64_t);
    const uintptr_t before_guard_vaddr = block0_vaddr - sizeof(uint64_t);
    const uintptr_t guard01_vaddr =
        block0_vaddr +
        AI_MEMBLOCK_CMO_TRIPLE_BLOCK_WORDS * sizeof(uint64_t);
    const uintptr_t guard12_vaddr =
        block1_vaddr +
        AI_MEMBLOCK_CMO_TRIPLE_BLOCK_WORDS * sizeof(uint64_t);
    const uintptr_t after_guard_vaddr =
        block2_vaddr +
        AI_MEMBLOCK_CMO_TRIPLE_BLOCK_WORDS * sizeof(uint64_t);
    const uintptr_t before_guard_paddr = block0_paddr - sizeof(uint64_t);
    const uintptr_t guard01_paddr =
        block0_paddr +
        AI_MEMBLOCK_CMO_TRIPLE_BLOCK_WORDS * sizeof(uint64_t);
    const uintptr_t guard12_paddr =
        block1_paddr +
        AI_MEMBLOCK_CMO_TRIPLE_BLOCK_WORDS * sizeof(uint64_t);
    const uintptr_t after_guard_paddr =
        block2_paddr +
        AI_MEMBLOCK_CMO_TRIPLE_BLOCK_WORDS * sizeof(uint64_t);
    const uintptr_t marker_vaddr = hs_page_base(marker_page) + 0x3400U;
    const uintptr_t marker_paddr = phys_page_base(marker_page) + 0x3400U;
    const uintptr_t marker_guard_vaddr = marker_vaddr + sizeof(uint64_t);
    const uintptr_t marker_guard_paddr = marker_paddr + sizeof(uint64_t);
    const uint64_t block0_seeds[AI_MEMBLOCK_CMO_TRIPLE_ROUNDS] = {
        0x3400440054006400ULL,
        0x4511551165117511ULL,
        0x5622662276228622ULL,
    };
    const uint64_t block1_seeds[AI_MEMBLOCK_CMO_TRIPLE_ROUNDS] = {
        0x8070605034003480ULL,
        0x9181716145114591ULL,
        0xa2928272562256a2ULL,
    };
    const uint64_t block2_seeds[AI_MEMBLOCK_CMO_TRIPLE_ROUNDS] = {
        0x2468ace034003500ULL,
        0x3579bdf145114611ULL,
        0x468ace0256225722ULL,
    };
    const uint64_t block0_older_store_values[AI_MEMBLOCK_CMO_TRIPLE_ROUNDS] = {
        0x1100340022003300ULL,
        0x2211451133114411ULL,
        0x3322562244225522ULL,
    };
    const uint64_t block0_second_store_values[AI_MEMBLOCK_CMO_TRIPLE_ROUNDS] = {
        0x0033002200340011ULL,
        0x1144113311451122ULL,
        0x2255224422562233ULL,
    };
    const uint64_t block0_third_store_values[AI_MEMBLOCK_CMO_TRIPLE_ROUNDS] = {
        0xcafe3400feed3400ULL,
        0xdb0f45110ffe4511ULL,
        0xec205622200f5622ULL,
    };
    const uint64_t block0_fourth_store_values[AI_MEMBLOCK_CMO_TRIPLE_ROUNDS] = {
        0xc0013400deadbeefULL,
        0xd1124511efbedf00ULL,
        0xe2235622f0cfe011ULL,
    };
    const uint64_t before_guards[AI_MEMBLOCK_CMO_TRIPLE_ROUNDS] = {
        0xc0013400d00d3400ULL,
        0xd1124511e11e4511ULL,
        0xe2235622f22f5622ULL,
    };
    const uint64_t guard01_values[AI_MEMBLOCK_CMO_TRIPLE_ROUNDS] = {
        0x0034d00d0034c001ULL,
        0x1145e11e1145d112ULL,
        0x2256f22f2256e223ULL,
    };
    const uint64_t guard12_values[AI_MEMBLOCK_CMO_TRIPLE_ROUNDS] = {
        0x3141592634003480ULL,
        0x2718281845114591ULL,
        0x16180339562256a2ULL,
    };
    const uint64_t after_guards[AI_MEMBLOCK_CMO_TRIPLE_ROUNDS] = {
        0x0834003496269513ULL,
        0x1945114581828272ULL,
        0x2a56225693038161ULL,
    };
    const uint64_t marker_seeds[AI_MEMBLOCK_CMO_TRIPLE_ROUNDS] = {
        0x13579bdf2468ace0ULL,
        0x2468ace03579bdf1ULL,
        0x3579bdf1468ace02ULL,
    };
    const uint64_t marker_updates[AI_MEMBLOCK_CMO_TRIPLE_ROUNDS] = {
        0x0eca8642fdb97531ULL,
        0x1fdb97530eca8642ULL,
        0x20eca8641fdb9753ULL,
    };
    const uint64_t marker_guards[AI_MEMBLOCK_CMO_TRIPLE_ROUNDS] = {
        0x55aa55aa34003400ULL,
        0x66bb66bb45114511ULL,
        0x77cc77cc56225622ULL,
    };

    ai_memblock_cmo_triple_prepare_hs_env();

    for (size_t round = 0; round < AI_MEMBLOCK_CMO_TRIPLE_ROUNDS; ++round) {
        uint64_t block0_older_observed = 0ULL;
        uint64_t block0_second_observed = 0ULL;
        uint64_t block0_third_observed = 0ULL;
        uint64_t block0_fourth_observed = 0ULL;
        uint64_t block0_zero_observed = 0ULL;
        uint64_t block1_observed = 0ULL;
        uint64_t block2_observed = 0ULL;
        uint64_t marker_observed = 0ULL;
        const uint64_t expected_block0_fourth_before =
            block0_seeds[round] +
            block0_fourth_store_word * 0x1111111111111111ULL;
        const uint64_t expected_block0_older_before =
            block0_seeds[round] +
            block0_older_store_word * 0x1111111111111111ULL;
        const uint64_t expected_block0_zero_before =
            block0_seeds[round] +
            block0_zero_load_word * 0x1111111111111111ULL;
        const uint64_t expected_block0_second_before =
            block0_seeds[round] +
            block0_second_store_word * 0x1111111111111111ULL;
        const uint64_t expected_block0_third_before =
            block0_seeds[round] +
            block0_third_store_word * 0x1111111111111111ULL;
        const uint64_t expected_block2_load =
            block2_seeds[round] +
            block2_load_word * 0x1111111111111111ULL;

        goto_priv(PRIV_M);
        ai_memblock_cmo_triple_seed_block(block0_paddr,
            block0_seeds[round]);
        ai_memblock_cmo_triple_seed_block(block1_paddr,
            block1_seeds[round]);
        ai_memblock_cmo_triple_seed_block(block2_paddr,
            block2_seeds[round]);
        write64(before_guard_paddr, before_guards[round]);
        write64(guard01_paddr, guard01_values[round]);
        write64(guard12_paddr, guard12_values[round]);
        write64(after_guard_paddr, after_guards[round]);
        write64(marker_paddr, marker_seeds[round]);
        write64(marker_guard_paddr, marker_guards[round]);

        goto_priv(PRIV_HS);
        TEST_SETUP_EXCEPT();
        AI_ASSERT_NO_EXCEPT_AND("before the four-generation inval-inval-inval-zero CMO probe starts, all source blocks, guards, and marker are readable",
            ld(block0_vaddr) == expected_block0_fourth_before &&
            ld(block0_older_store_vaddr) == expected_block0_older_before &&
            ld(block0_zero_load_vaddr) == expected_block0_zero_before &&
            ld(block0_second_store_vaddr) == expected_block0_second_before &&
            ld(block0_third_store_vaddr) == expected_block0_third_before &&
            ld(block1_vaddr) == block1_seeds[round] &&
            ld(block2_vaddr) == block2_seeds[round] &&
            ld(block2_load_vaddr) == expected_block2_load &&
            ld(marker_vaddr) == marker_seeds[round]
        );

        TEST_SETUP_EXCEPT();
        ai_memblock_cmo_triple_three_block_store_inval_store_inval_store_inval_store_zero_window(
            block0_vaddr, block1_vaddr, block2_vaddr, marker_vaddr,
            block0_older_store_values[round],
            block0_second_store_values[round],
            block0_third_store_values[round],
            block0_fourth_store_values[round], marker_updates[round],
            &block0_older_observed, &block0_second_observed,
            &block0_third_observed, &block0_fourth_observed,
            &block0_zero_observed, &block1_observed, &block2_observed,
            &marker_observed);
        AI_ASSERT_NO_EXCEPT_AND("under three-block CMO pressure, triple CBO.INVAL then CBO.ZERO leaves block0 fully zeroed",
            block0_older_observed == 0ULL &&
            block0_second_observed == 0ULL &&
            block0_third_observed == 0ULL &&
            block0_fourth_observed == 0ULL &&
            block0_zero_observed == 0ULL &&
            block1_observed == 0ULL &&
            block2_observed == expected_block2_load &&
            marker_observed == marker_updates[round]
        );

        TEST_SETUP_EXCEPT();
        AI_ASSERT_NO_EXCEPT_AND("after the four-generation inval-inval-inval-zero CMO probe, final CBO.ZERO clears block0, block1 is zero, block2 preserves seed, and guards remain intact",
            ld(block0_fourth_store_vaddr) == 0ULL &&
            ld(block0_older_store_vaddr) == 0ULL &&
            ld(block0_zero_load_vaddr) == 0ULL &&
            ld(block0_second_store_vaddr) == 0ULL &&
            ld(block0_third_store_vaddr) == 0ULL &&
            ld(block1_vaddr) == 0ULL &&
            ld(block1_load_vaddr) == 0ULL &&
            ld(block2_vaddr) == block2_seeds[round] &&
            ld(block2_load_vaddr) == expected_block2_load &&
            ld(before_guard_vaddr) == before_guards[round] &&
            ld(guard01_vaddr) == guard01_values[round] &&
            ld(guard12_vaddr) == guard12_values[round] &&
            ld(after_guard_vaddr) == after_guards[round] &&
            ld(marker_guard_vaddr) == marker_guards[round]
        );

        goto_priv(PRIV_M);
        TEST_ASSERT("backing memory after the four-generation inval-inval-inval-zero CMO probe keeps block0 zero, block1 zero, block2 seed payload, marker update, and all guards intact",
            ai_memblock_cmo_triple_block_all_zero(block0_paddr) &&
            ai_memblock_cmo_triple_block_all_zero(block1_paddr) &&
            ai_memblock_cmo_triple_block_matches_seed(block2_paddr,
                block2_seeds[round]) &&
            read64(before_guard_paddr) == before_guards[round] &&
            read64(guard01_paddr) == guard01_values[round] &&
            read64(guard12_paddr) == guard12_values[round] &&
            read64(after_guard_paddr) == after_guards[round] &&
            read64(marker_paddr) == marker_updates[round] &&
            read64(marker_guard_paddr) == marker_guards[round]
        );
    }

    ai_memblock_cmo_triple_reset_hs_env();

    TEST_END();
}

bool ai_micro_hs_interleaved_three_block_store_zero_store_zero_store_zero_store_inval_four_generation_post_third_zero_corner()
{
    TEST_START();

    const enum test_page source_page = SWITCH1;
    const enum test_page marker_page = SWITCH2;
    const size_t block0_fourth_store_word = 0U;
    const size_t block0_older_store_word = 1U;
    const size_t block0_zero_load_word = 2U;
    const size_t block0_second_store_word = 4U;
    const size_t block0_third_store_word = 6U;
    const size_t block1_load_word = 3U;
    const size_t block2_load_word = 5U;
    const uintptr_t block0_vaddr = hs_page_base(source_page) + 0x3a00U;
    const uintptr_t block1_vaddr = hs_page_base(source_page) + 0x3a80U;
    const uintptr_t block2_vaddr = hs_page_base(source_page) + 0x3b00U;
    const uintptr_t block0_paddr = phys_page_base(source_page) + 0x3a00U;
    const uintptr_t block1_paddr = phys_page_base(source_page) + 0x3a80U;
    const uintptr_t block2_paddr = phys_page_base(source_page) + 0x3b00U;
    const uintptr_t block0_fourth_store_vaddr =
        block0_vaddr + block0_fourth_store_word * sizeof(uint64_t);
    const uintptr_t block0_older_store_vaddr =
        block0_vaddr + block0_older_store_word * sizeof(uint64_t);
    const uintptr_t block0_zero_load_vaddr =
        block0_vaddr + block0_zero_load_word * sizeof(uint64_t);
    const uintptr_t block0_second_store_vaddr =
        block0_vaddr + block0_second_store_word * sizeof(uint64_t);
    const uintptr_t block0_third_store_vaddr =
        block0_vaddr + block0_third_store_word * sizeof(uint64_t);
    const uintptr_t block1_load_vaddr =
        block1_vaddr + block1_load_word * sizeof(uint64_t);
    const uintptr_t block2_load_vaddr =
        block2_vaddr + block2_load_word * sizeof(uint64_t);
    const uintptr_t before_guard_vaddr = block0_vaddr - sizeof(uint64_t);
    const uintptr_t guard01_vaddr =
        block0_vaddr +
        AI_MEMBLOCK_CMO_TRIPLE_BLOCK_WORDS * sizeof(uint64_t);
    const uintptr_t guard12_vaddr =
        block1_vaddr +
        AI_MEMBLOCK_CMO_TRIPLE_BLOCK_WORDS * sizeof(uint64_t);
    const uintptr_t after_guard_vaddr =
        block2_vaddr +
        AI_MEMBLOCK_CMO_TRIPLE_BLOCK_WORDS * sizeof(uint64_t);
    const uintptr_t before_guard_paddr = block0_paddr - sizeof(uint64_t);
    const uintptr_t guard01_paddr =
        block0_paddr +
        AI_MEMBLOCK_CMO_TRIPLE_BLOCK_WORDS * sizeof(uint64_t);
    const uintptr_t guard12_paddr =
        block1_paddr +
        AI_MEMBLOCK_CMO_TRIPLE_BLOCK_WORDS * sizeof(uint64_t);
    const uintptr_t after_guard_paddr =
        block2_paddr +
        AI_MEMBLOCK_CMO_TRIPLE_BLOCK_WORDS * sizeof(uint64_t);
    const uintptr_t marker_vaddr = hs_page_base(marker_page) + 0x3a00U;
    const uintptr_t marker_paddr = phys_page_base(marker_page) + 0x3a00U;
    const uintptr_t marker_guard_vaddr = marker_vaddr + sizeof(uint64_t);
    const uintptr_t marker_guard_paddr = marker_paddr + sizeof(uint64_t);
    const uint64_t block0_seeds[AI_MEMBLOCK_CMO_TRIPLE_ROUNDS] = {
        0x3a004a005a006a00ULL,
        0x4b115b116b117b11ULL,
        0x5c226c227c228c22ULL,
    };
    const uint64_t block1_seeds[AI_MEMBLOCK_CMO_TRIPLE_ROUNDS] = {
        0x807060503a003a80ULL,
        0x918171614b114b91ULL,
        0xa29282725c225ca2ULL,
    };
    const uint64_t block2_seeds[AI_MEMBLOCK_CMO_TRIPLE_ROUNDS] = {
        0x2468ace03a003b00ULL,
        0x3579bdf14b114c11ULL,
        0x468ace025c225d22ULL,
    };
    const uint64_t block0_older_store_values[AI_MEMBLOCK_CMO_TRIPLE_ROUNDS] = {
        0x11003a0022003300ULL,
        0x22114b1133114411ULL,
        0x33225c2244225522ULL,
    };
    const uint64_t block0_second_store_values[AI_MEMBLOCK_CMO_TRIPLE_ROUNDS] = {
        0x00330022003a0011ULL,
        0x11441133114b1122ULL,
        0x22552244225c2233ULL,
    };
    const uint64_t block0_third_store_values[AI_MEMBLOCK_CMO_TRIPLE_ROUNDS] = {
        0xcafe3a00feed3a00ULL,
        0xdb0f4b110ffe4b11ULL,
        0xec205c22200f5c22ULL,
    };
    const uint64_t block0_fourth_store_values[AI_MEMBLOCK_CMO_TRIPLE_ROUNDS] = {
        0xc0013a00deadbeefULL,
        0xd1124b11efbedf00ULL,
        0xe2235c22f0cfe011ULL,
    };
    const uint64_t before_guards[AI_MEMBLOCK_CMO_TRIPLE_ROUNDS] = {
        0xc0013a00d00d3a00ULL,
        0xd1124b11e11e4b11ULL,
        0xe2235c22f22f5c22ULL,
    };
    const uint64_t guard01_values[AI_MEMBLOCK_CMO_TRIPLE_ROUNDS] = {
        0x003ad00d003ac001ULL,
        0x114be11e114bd112ULL,
        0x225cf22f225ce223ULL,
    };
    const uint64_t guard12_values[AI_MEMBLOCK_CMO_TRIPLE_ROUNDS] = {
        0x314159263a003a80ULL,
        0x271828184b114b91ULL,
        0x161803395c225ca2ULL,
    };
    const uint64_t after_guards[AI_MEMBLOCK_CMO_TRIPLE_ROUNDS] = {
        0x083a003a96269513ULL,
        0x194b114b81828272ULL,
        0x2a5c225c93038161ULL,
    };
    const uint64_t marker_seeds[AI_MEMBLOCK_CMO_TRIPLE_ROUNDS] = {
        0x89abcdef01234567ULL,
        0x9abcdef012345678ULL,
        0xabcdef0123456789ULL,
    };
    const uint64_t marker_updates[AI_MEMBLOCK_CMO_TRIPLE_ROUNDS] = {
        0x76543210fedcba98ULL,
        0x876543210fedcba9ULL,
        0x9876543210fedcbaULL,
    };
    const uint64_t marker_guards[AI_MEMBLOCK_CMO_TRIPLE_ROUNDS] = {
        0x55aa55aa3a003a00ULL,
        0x66bb66bb4b114b11ULL,
        0x77cc77cc5c225c22ULL,
    };

    ai_memblock_cmo_triple_prepare_hs_env();

    for (size_t round = 0; round < AI_MEMBLOCK_CMO_TRIPLE_ROUNDS; ++round) {
        uint64_t block0_older_observed = 0ULL;
        uint64_t block0_second_observed = 0ULL;
        uint64_t block0_third_observed = 0ULL;
        uint64_t block0_fourth_observed = 0ULL;
        uint64_t block0_zero_observed = 0ULL;
        uint64_t block1_observed = 0ULL;
        uint64_t block2_observed = 0ULL;
        uint64_t marker_observed = 0ULL;
        const uint64_t expected_block0_fourth_before =
            block0_seeds[round] +
            block0_fourth_store_word * 0x1111111111111111ULL;
        const uint64_t expected_block0_older_before =
            block0_seeds[round] +
            block0_older_store_word * 0x1111111111111111ULL;
        const uint64_t expected_block0_zero_before =
            block0_seeds[round] +
            block0_zero_load_word * 0x1111111111111111ULL;
        const uint64_t expected_block0_second_before =
            block0_seeds[round] +
            block0_second_store_word * 0x1111111111111111ULL;
        const uint64_t expected_block0_third_before =
            block0_seeds[round] +
            block0_third_store_word * 0x1111111111111111ULL;
        const uint64_t expected_block2_load =
            block2_seeds[round] +
            block2_load_word * 0x1111111111111111ULL;

        goto_priv(PRIV_M);
        ai_memblock_cmo_triple_seed_block(block0_paddr,
            block0_seeds[round]);
        ai_memblock_cmo_triple_seed_block(block1_paddr,
            block1_seeds[round]);
        ai_memblock_cmo_triple_seed_block(block2_paddr,
            block2_seeds[round]);
        write64(before_guard_paddr, before_guards[round]);
        write64(guard01_paddr, guard01_values[round]);
        write64(guard12_paddr, guard12_values[round]);
        write64(after_guard_paddr, after_guards[round]);
        write64(marker_paddr, marker_seeds[round]);
        write64(marker_guard_paddr, marker_guards[round]);

        goto_priv(PRIV_HS);
        TEST_SETUP_EXCEPT();
        AI_ASSERT_NO_EXCEPT_AND("before the four-generation zero-zero-zero-inval CMO probe starts, all source blocks, guards, and marker are readable",
            ld(block0_vaddr) == expected_block0_fourth_before &&
            ld(block0_older_store_vaddr) == expected_block0_older_before &&
            ld(block0_zero_load_vaddr) == expected_block0_zero_before &&
            ld(block0_second_store_vaddr) == expected_block0_second_before &&
            ld(block0_third_store_vaddr) == expected_block0_third_before &&
            ld(block1_vaddr) == block1_seeds[round] &&
            ld(block2_vaddr) == block2_seeds[round] &&
            ld(block2_load_vaddr) == expected_block2_load &&
            ld(marker_vaddr) == marker_seeds[round]
        );

        TEST_SETUP_EXCEPT();
        ai_memblock_cmo_triple_three_block_store_zero_store_zero_store_zero_store_inval_window(
            block0_vaddr, block1_vaddr, block2_vaddr, marker_vaddr,
            block0_older_store_values[round],
            block0_second_store_values[round],
            block0_third_store_values[round],
            block0_fourth_store_values[round], marker_updates[round],
            &block0_older_observed, &block0_second_observed,
            &block0_third_observed, &block0_fourth_observed,
            &block0_zero_observed, &block1_observed, &block2_observed,
            &marker_observed);
        AI_ASSERT_NO_EXCEPT_AND("under three-block CMO pressure, triple CBO.ZERO then CBO.INVAL keeps only the final post-zero store visible",
            block0_older_observed == 0ULL &&
            block0_second_observed == 0ULL &&
            block0_third_observed == 0ULL &&
            block0_fourth_observed == block0_fourth_store_values[round] &&
            block0_zero_observed == 0ULL &&
            block1_observed == 0ULL &&
            block2_observed == expected_block2_load &&
            marker_observed == marker_updates[round]
        );

        TEST_SETUP_EXCEPT();
        AI_ASSERT_NO_EXCEPT_AND("after the four-generation zero-zero-zero-inval CMO probe, block0 keeps only the final post-zero store, block1 is zero, block2 preserves seed, and guards remain intact",
            ld(block0_fourth_store_vaddr) ==
                block0_fourth_store_values[round] &&
            ld(block0_older_store_vaddr) == 0ULL &&
            ld(block0_zero_load_vaddr) == 0ULL &&
            ld(block0_second_store_vaddr) == 0ULL &&
            ld(block0_third_store_vaddr) == 0ULL &&
            ld(block1_vaddr) == 0ULL &&
            ld(block1_load_vaddr) == 0ULL &&
            ld(block2_vaddr) == block2_seeds[round] &&
            ld(block2_load_vaddr) == expected_block2_load &&
            ld(before_guard_vaddr) == before_guards[round] &&
            ld(guard01_vaddr) == guard01_values[round] &&
            ld(guard12_vaddr) == guard12_values[round] &&
            ld(after_guard_vaddr) == after_guards[round] &&
            ld(marker_guard_vaddr) == marker_guards[round]
        );

        goto_priv(PRIV_M);
        TEST_ASSERT("backing memory after the four-generation zero-zero-zero-inval CMO probe keeps block0 zero except the final post-zero store, block1 zero, block2 seed payload, marker update, and all guards intact",
            ai_memblock_cmo_triple_block_zero_except_word(
                block0_paddr, block0_fourth_store_word,
                block0_fourth_store_values[round]) &&
            ai_memblock_cmo_triple_block_all_zero(block1_paddr) &&
            ai_memblock_cmo_triple_block_matches_seed(block2_paddr,
                block2_seeds[round]) &&
            read64(before_guard_paddr) == before_guards[round] &&
            read64(guard01_paddr) == guard01_values[round] &&
            read64(guard12_paddr) == guard12_values[round] &&
            read64(after_guard_paddr) == after_guards[round] &&
            read64(marker_paddr) == marker_updates[round] &&
            read64(marker_guard_paddr) == marker_guards[round]
        );
    }

    ai_memblock_cmo_triple_reset_hs_env();

    TEST_END();
}

bool ai_micro_hs_interleaved_three_block_store_zero_store_zero_store_zero_store_cbom_four_generation_post_third_zero_corner()
{
    TEST_START();

    const enum test_page source_page = SWITCH1;
    const enum test_page marker_page = SWITCH2;
    const size_t block0_fourth_store_word = 0U;
    const size_t block0_older_store_word = 1U;
    const size_t block0_zero_load_word = 2U;
    const size_t block0_second_store_word = 4U;
    const size_t block0_third_store_word = 6U;
    const size_t block1_load_word = 3U;
    const size_t block2_load_word = 5U;
    const uintptr_t block0_vaddr = hs_page_base(source_page) + 0x3a00U;
    const uintptr_t block1_vaddr = hs_page_base(source_page) + 0x3a80U;
    const uintptr_t block2_vaddr = hs_page_base(source_page) + 0x3b00U;
    const uintptr_t block0_paddr = phys_page_base(source_page) + 0x3a00U;
    const uintptr_t block1_paddr = phys_page_base(source_page) + 0x3a80U;
    const uintptr_t block2_paddr = phys_page_base(source_page) + 0x3b00U;
    const uintptr_t block0_fourth_store_vaddr =
        block0_vaddr + block0_fourth_store_word * sizeof(uint64_t);
    const uintptr_t block0_older_store_vaddr =
        block0_vaddr + block0_older_store_word * sizeof(uint64_t);
    const uintptr_t block0_zero_load_vaddr =
        block0_vaddr + block0_zero_load_word * sizeof(uint64_t);
    const uintptr_t block0_second_store_vaddr =
        block0_vaddr + block0_second_store_word * sizeof(uint64_t);
    const uintptr_t block0_third_store_vaddr =
        block0_vaddr + block0_third_store_word * sizeof(uint64_t);
    const uintptr_t block1_load_vaddr =
        block1_vaddr + block1_load_word * sizeof(uint64_t);
    const uintptr_t block2_load_vaddr =
        block2_vaddr + block2_load_word * sizeof(uint64_t);
    const uintptr_t before_guard_vaddr = block0_vaddr - sizeof(uint64_t);
    const uintptr_t guard01_vaddr =
        block0_vaddr +
        AI_MEMBLOCK_CMO_TRIPLE_BLOCK_WORDS * sizeof(uint64_t);
    const uintptr_t guard12_vaddr =
        block1_vaddr +
        AI_MEMBLOCK_CMO_TRIPLE_BLOCK_WORDS * sizeof(uint64_t);
    const uintptr_t after_guard_vaddr =
        block2_vaddr +
        AI_MEMBLOCK_CMO_TRIPLE_BLOCK_WORDS * sizeof(uint64_t);
    const uintptr_t before_guard_paddr = block0_paddr - sizeof(uint64_t);
    const uintptr_t guard01_paddr =
        block0_paddr +
        AI_MEMBLOCK_CMO_TRIPLE_BLOCK_WORDS * sizeof(uint64_t);
    const uintptr_t guard12_paddr =
        block1_paddr +
        AI_MEMBLOCK_CMO_TRIPLE_BLOCK_WORDS * sizeof(uint64_t);
    const uintptr_t after_guard_paddr =
        block2_paddr +
        AI_MEMBLOCK_CMO_TRIPLE_BLOCK_WORDS * sizeof(uint64_t);
    const uintptr_t marker_vaddr = hs_page_base(marker_page) + 0x3a00U;
    const uintptr_t marker_paddr = phys_page_base(marker_page) + 0x3a00U;
    const uintptr_t marker_guard_vaddr = marker_vaddr + sizeof(uint64_t);
    const uintptr_t marker_guard_paddr = marker_paddr + sizeof(uint64_t);
    const uint64_t block0_seeds[AI_MEMBLOCK_CMO_TRIPLE_ROUNDS] = {
        0x3a004a005a006a00ULL,
        0x4b115b116b117b11ULL,
        0x5c226c227c228c22ULL,
    };
    const uint64_t block1_seeds[AI_MEMBLOCK_CMO_TRIPLE_ROUNDS] = {
        0x807060503a003a80ULL,
        0x918171614b114b91ULL,
        0xa29282725c225ca2ULL,
    };
    const uint64_t block2_seeds[AI_MEMBLOCK_CMO_TRIPLE_ROUNDS] = {
        0x2468ace03a003b00ULL,
        0x3579bdf14b114c11ULL,
        0x468ace025c225d22ULL,
    };
    const uint64_t block0_older_store_values[AI_MEMBLOCK_CMO_TRIPLE_ROUNDS] = {
        0x11003a0022003300ULL,
        0x22114b1133114411ULL,
        0x33225c2244225522ULL,
    };
    const uint64_t block0_second_store_values[AI_MEMBLOCK_CMO_TRIPLE_ROUNDS] = {
        0x00330022003a0011ULL,
        0x11441133114b1122ULL,
        0x22552244225c2233ULL,
    };
    const uint64_t block0_third_store_values[AI_MEMBLOCK_CMO_TRIPLE_ROUNDS] = {
        0xcafe3a00feed3a00ULL,
        0xdb0f4b110ffe4b11ULL,
        0xec205c22200f5c22ULL,
    };
    const uint64_t block0_fourth_store_values[AI_MEMBLOCK_CMO_TRIPLE_ROUNDS] = {
        0xc0013a00deadbeefULL,
        0xd1124b11efbedf00ULL,
        0xe2235c22f0cfe011ULL,
    };
    const uint64_t before_guards[AI_MEMBLOCK_CMO_TRIPLE_ROUNDS] = {
        0xc0013a00d00d3a00ULL,
        0xd1124b11e11e4b11ULL,
        0xe2235c22f22f5c22ULL,
    };
    const uint64_t guard01_values[AI_MEMBLOCK_CMO_TRIPLE_ROUNDS] = {
        0x003ad00d003ac001ULL,
        0x114be11e114bd112ULL,
        0x225cf22f225ce223ULL,
    };
    const uint64_t guard12_values[AI_MEMBLOCK_CMO_TRIPLE_ROUNDS] = {
        0x314159263a003a80ULL,
        0x271828184b114b91ULL,
        0x161803395c225ca2ULL,
    };
    const uint64_t after_guards[AI_MEMBLOCK_CMO_TRIPLE_ROUNDS] = {
        0x083a003a96269513ULL,
        0x194b114b81828272ULL,
        0x2a5c225c93038161ULL,
    };
    const uint64_t marker_seeds[AI_MEMBLOCK_CMO_TRIPLE_ROUNDS] = {
        0x89abcdef01234567ULL,
        0x9abcdef012345678ULL,
        0xabcdef0123456789ULL,
    };
    const uint64_t marker_updates[AI_MEMBLOCK_CMO_TRIPLE_ROUNDS] = {
        0x76543210fedcba98ULL,
        0x876543210fedcba9ULL,
        0x9876543210fedcbaULL,
    };
    const uint64_t marker_guards[AI_MEMBLOCK_CMO_TRIPLE_ROUNDS] = {
        0x55aa55aa3a003a00ULL,
        0x66bb66bb4b114b11ULL,
        0x77cc77cc5c225c22ULL,
    };

    ai_memblock_cmo_triple_prepare_hs_env();

    for (size_t round = 0; round < AI_MEMBLOCK_CMO_TRIPLE_ROUNDS; ++round) {
        uint64_t block0_older_observed = 0ULL;
        uint64_t block0_second_observed = 0ULL;
        uint64_t block0_third_observed = 0ULL;
        uint64_t block0_fourth_observed = 0ULL;
        uint64_t block0_zero_observed = 0ULL;
        uint64_t block1_observed = 0ULL;
        uint64_t block2_observed = 0ULL;
        uint64_t marker_observed = 0ULL;
        const uint64_t expected_block0_fourth_before =
            block0_seeds[round] +
            block0_fourth_store_word * 0x1111111111111111ULL;
        const uint64_t expected_block0_older_before =
            block0_seeds[round] +
            block0_older_store_word * 0x1111111111111111ULL;
        const uint64_t expected_block0_zero_before =
            block0_seeds[round] +
            block0_zero_load_word * 0x1111111111111111ULL;
        const uint64_t expected_block0_second_before =
            block0_seeds[round] +
            block0_second_store_word * 0x1111111111111111ULL;
        const uint64_t expected_block0_third_before =
            block0_seeds[round] +
            block0_third_store_word * 0x1111111111111111ULL;
        const uint64_t expected_block2_load =
            block2_seeds[round] +
            block2_load_word * 0x1111111111111111ULL;

        goto_priv(PRIV_M);
        ai_memblock_cmo_triple_seed_block(block0_paddr,
            block0_seeds[round]);
        ai_memblock_cmo_triple_seed_block(block1_paddr,
            block1_seeds[round]);
        ai_memblock_cmo_triple_seed_block(block2_paddr,
            block2_seeds[round]);
        write64(before_guard_paddr, before_guards[round]);
        write64(guard01_paddr, guard01_values[round]);
        write64(guard12_paddr, guard12_values[round]);
        write64(after_guard_paddr, after_guards[round]);
        write64(marker_paddr, marker_seeds[round]);
        write64(marker_guard_paddr, marker_guards[round]);

        goto_priv(PRIV_HS);
        TEST_SETUP_EXCEPT();
        AI_ASSERT_NO_EXCEPT_AND("before the four-generation zero-zero-zero-clean CMO probe starts, all source blocks, guards, and marker are readable",
            ld(block0_vaddr) == expected_block0_fourth_before &&
            ld(block0_older_store_vaddr) == expected_block0_older_before &&
            ld(block0_zero_load_vaddr) == expected_block0_zero_before &&
            ld(block0_second_store_vaddr) == expected_block0_second_before &&
            ld(block0_third_store_vaddr) == expected_block0_third_before &&
            ld(block1_vaddr) == block1_seeds[round] &&
            ld(block2_vaddr) == block2_seeds[round] &&
            ld(block2_load_vaddr) == expected_block2_load &&
            ld(marker_vaddr) == marker_seeds[round]
        );

        TEST_SETUP_EXCEPT();
        ai_memblock_cmo_triple_three_block_store_zero_store_zero_store_zero_store_cbom_window(
            block0_vaddr, block1_vaddr, block2_vaddr, marker_vaddr,
            block0_older_store_values[round],
            block0_second_store_values[round],
            block0_third_store_values[round],
            block0_fourth_store_values[round], marker_updates[round],
            &block0_older_observed, &block0_second_observed,
            &block0_third_observed, &block0_fourth_observed,
            &block0_zero_observed, &block1_observed, &block2_observed,
            &marker_observed);
        AI_ASSERT_NO_EXCEPT_AND("under three-block CMO pressure, triple CBO.ZERO then CBOM keeps only the final post-zero store visible",
            block0_older_observed == 0ULL &&
            block0_second_observed == 0ULL &&
            block0_third_observed == 0ULL &&
            block0_fourth_observed == block0_fourth_store_values[round] &&
            block0_zero_observed == 0ULL &&
            block1_observed == 0ULL &&
            block2_observed == expected_block2_load &&
            marker_observed == marker_updates[round]
        );

        TEST_SETUP_EXCEPT();
        AI_ASSERT_NO_EXCEPT_AND("after the four-generation zero-zero-zero-clean CMO probe, block0 keeps only the final post-zero store, block1 is zero, block2 preserves seed, and guards remain intact",
            ld(block0_fourth_store_vaddr) ==
                block0_fourth_store_values[round] &&
            ld(block0_older_store_vaddr) == 0ULL &&
            ld(block0_zero_load_vaddr) == 0ULL &&
            ld(block0_second_store_vaddr) == 0ULL &&
            ld(block0_third_store_vaddr) == 0ULL &&
            ld(block1_vaddr) == 0ULL &&
            ld(block1_load_vaddr) == 0ULL &&
            ld(block2_vaddr) == block2_seeds[round] &&
            ld(block2_load_vaddr) == expected_block2_load &&
            ld(before_guard_vaddr) == before_guards[round] &&
            ld(guard01_vaddr) == guard01_values[round] &&
            ld(guard12_vaddr) == guard12_values[round] &&
            ld(after_guard_vaddr) == after_guards[round] &&
            ld(marker_guard_vaddr) == marker_guards[round]
        );

        goto_priv(PRIV_M);
        TEST_ASSERT("backing memory after the four-generation zero-zero-zero-clean CMO probe keeps block0 zero except the final post-zero store, block1 zero, block2 seed payload, marker update, and all guards intact",
            ai_memblock_cmo_triple_block_zero_except_word(
                block0_paddr, block0_fourth_store_word,
                block0_fourth_store_values[round]) &&
            ai_memblock_cmo_triple_block_all_zero(block1_paddr) &&
            ai_memblock_cmo_triple_block_matches_seed(block2_paddr,
                block2_seeds[round]) &&
            read64(before_guard_paddr) == before_guards[round] &&
            read64(guard01_paddr) == guard01_values[round] &&
            read64(guard12_paddr) == guard12_values[round] &&
            read64(after_guard_paddr) == after_guards[round] &&
            read64(marker_paddr) == marker_updates[round] &&
            read64(marker_guard_paddr) == marker_guards[round]
        );
    }

    ai_memblock_cmo_triple_reset_hs_env();

    TEST_END();
}

bool ai_micro_hs_interleaved_three_block_store_zero_store_zero_store_inval_store_zero_four_generation_final_zero_corner()
{
    TEST_START();

    const enum test_page source_page = SWITCH1;
    const enum test_page marker_page = SWITCH2;
    const size_t block0_fourth_store_word = 0U;
    const size_t block0_older_store_word = 1U;
    const size_t block0_zero_load_word = 2U;
    const size_t block0_second_store_word = 4U;
    const size_t block0_third_store_word = 6U;
    const size_t block1_load_word = 3U;
    const size_t block2_load_word = 5U;
    const uintptr_t block0_vaddr = hs_page_base(source_page) + 0x3a00U;
    const uintptr_t block1_vaddr = hs_page_base(source_page) + 0x3a80U;
    const uintptr_t block2_vaddr = hs_page_base(source_page) + 0x3b00U;
    const uintptr_t block0_paddr = phys_page_base(source_page) + 0x3a00U;
    const uintptr_t block1_paddr = phys_page_base(source_page) + 0x3a80U;
    const uintptr_t block2_paddr = phys_page_base(source_page) + 0x3b00U;
    const uintptr_t block0_fourth_store_vaddr =
        block0_vaddr + block0_fourth_store_word * sizeof(uint64_t);
    const uintptr_t block0_older_store_vaddr =
        block0_vaddr + block0_older_store_word * sizeof(uint64_t);
    const uintptr_t block0_zero_load_vaddr =
        block0_vaddr + block0_zero_load_word * sizeof(uint64_t);
    const uintptr_t block0_second_store_vaddr =
        block0_vaddr + block0_second_store_word * sizeof(uint64_t);
    const uintptr_t block0_third_store_vaddr =
        block0_vaddr + block0_third_store_word * sizeof(uint64_t);
    const uintptr_t block1_load_vaddr =
        block1_vaddr + block1_load_word * sizeof(uint64_t);
    const uintptr_t block2_load_vaddr =
        block2_vaddr + block2_load_word * sizeof(uint64_t);
    const uintptr_t before_guard_vaddr = block0_vaddr - sizeof(uint64_t);
    const uintptr_t guard01_vaddr =
        block0_vaddr +
        AI_MEMBLOCK_CMO_TRIPLE_BLOCK_WORDS * sizeof(uint64_t);
    const uintptr_t guard12_vaddr =
        block1_vaddr +
        AI_MEMBLOCK_CMO_TRIPLE_BLOCK_WORDS * sizeof(uint64_t);
    const uintptr_t after_guard_vaddr =
        block2_vaddr +
        AI_MEMBLOCK_CMO_TRIPLE_BLOCK_WORDS * sizeof(uint64_t);
    const uintptr_t before_guard_paddr = block0_paddr - sizeof(uint64_t);
    const uintptr_t guard01_paddr =
        block0_paddr +
        AI_MEMBLOCK_CMO_TRIPLE_BLOCK_WORDS * sizeof(uint64_t);
    const uintptr_t guard12_paddr =
        block1_paddr +
        AI_MEMBLOCK_CMO_TRIPLE_BLOCK_WORDS * sizeof(uint64_t);
    const uintptr_t after_guard_paddr =
        block2_paddr +
        AI_MEMBLOCK_CMO_TRIPLE_BLOCK_WORDS * sizeof(uint64_t);
    const uintptr_t marker_vaddr = hs_page_base(marker_page) + 0x3a00U;
    const uintptr_t marker_paddr = phys_page_base(marker_page) + 0x3a00U;
    const uintptr_t marker_guard_vaddr = marker_vaddr + sizeof(uint64_t);
    const uintptr_t marker_guard_paddr = marker_paddr + sizeof(uint64_t);
    const uint64_t block0_seeds[AI_MEMBLOCK_CMO_TRIPLE_ROUNDS] = {
        0x3a004a005a006a00ULL,
        0x4b115b116b117b11ULL,
        0x5c226c227c228c22ULL,
    };
    const uint64_t block1_seeds[AI_MEMBLOCK_CMO_TRIPLE_ROUNDS] = {
        0x807060503a003a80ULL,
        0x918171614b114b91ULL,
        0xa29282725c225ca2ULL,
    };
    const uint64_t block2_seeds[AI_MEMBLOCK_CMO_TRIPLE_ROUNDS] = {
        0x2468ace03a003b00ULL,
        0x3579bdf14b114c11ULL,
        0x468ace025c225d22ULL,
    };
    const uint64_t block0_older_store_values[AI_MEMBLOCK_CMO_TRIPLE_ROUNDS] = {
        0x11003a0022003300ULL,
        0x22114b1133114411ULL,
        0x33225c2244225522ULL,
    };
    const uint64_t block0_second_store_values[AI_MEMBLOCK_CMO_TRIPLE_ROUNDS] = {
        0x00330022003a0011ULL,
        0x11441133114b1122ULL,
        0x22552244225c2233ULL,
    };
    const uint64_t block0_third_store_values[AI_MEMBLOCK_CMO_TRIPLE_ROUNDS] = {
        0xcafe3a00feed3a00ULL,
        0xdb0f4b110ffe4b11ULL,
        0xec205c22200f5c22ULL,
    };
    const uint64_t block0_fourth_store_values[AI_MEMBLOCK_CMO_TRIPLE_ROUNDS] = {
        0xc0013a00deadbeefULL,
        0xd1124b11efbedf00ULL,
        0xe2235c22f0cfe011ULL,
    };
    const uint64_t before_guards[AI_MEMBLOCK_CMO_TRIPLE_ROUNDS] = {
        0xc0013a00d00d3a00ULL,
        0xd1124b11e11e4b11ULL,
        0xe2235c22f22f5c22ULL,
    };
    const uint64_t guard01_values[AI_MEMBLOCK_CMO_TRIPLE_ROUNDS] = {
        0x003ad00d003ac001ULL,
        0x114be11e114bd112ULL,
        0x225cf22f225ce223ULL,
    };
    const uint64_t guard12_values[AI_MEMBLOCK_CMO_TRIPLE_ROUNDS] = {
        0x314159263a003a80ULL,
        0x271828184b114b91ULL,
        0x161803395c225ca2ULL,
    };
    const uint64_t after_guards[AI_MEMBLOCK_CMO_TRIPLE_ROUNDS] = {
        0x083a003a96269513ULL,
        0x194b114b81828272ULL,
        0x2a5c225c93038161ULL,
    };
    const uint64_t marker_seeds[AI_MEMBLOCK_CMO_TRIPLE_ROUNDS] = {
        0x89abcdef01234567ULL,
        0x9abcdef012345678ULL,
        0xabcdef0123456789ULL,
    };
    const uint64_t marker_updates[AI_MEMBLOCK_CMO_TRIPLE_ROUNDS] = {
        0x76543210fedcba98ULL,
        0x876543210fedcba9ULL,
        0x9876543210fedcbaULL,
    };
    const uint64_t marker_guards[AI_MEMBLOCK_CMO_TRIPLE_ROUNDS] = {
        0x55aa55aa3a003a00ULL,
        0x66bb66bb4b114b11ULL,
        0x77cc77cc5c225c22ULL,
    };

    ai_memblock_cmo_triple_prepare_hs_env();

    for (size_t round = 0; round < AI_MEMBLOCK_CMO_TRIPLE_ROUNDS; ++round) {
        uint64_t block0_older_observed = 0ULL;
        uint64_t block0_second_observed = 0ULL;
        uint64_t block0_third_observed = 0ULL;
        uint64_t block0_fourth_observed = 0ULL;
        uint64_t block0_zero_observed = 0ULL;
        uint64_t block1_observed = 0ULL;
        uint64_t block2_observed = 0ULL;
        uint64_t marker_observed = 0ULL;
        const uint64_t expected_block0_fourth_before =
            block0_seeds[round] +
            block0_fourth_store_word * 0x1111111111111111ULL;
        const uint64_t expected_block0_older_before =
            block0_seeds[round] +
            block0_older_store_word * 0x1111111111111111ULL;
        const uint64_t expected_block0_zero_before =
            block0_seeds[round] +
            block0_zero_load_word * 0x1111111111111111ULL;
        const uint64_t expected_block0_second_before =
            block0_seeds[round] +
            block0_second_store_word * 0x1111111111111111ULL;
        const uint64_t expected_block0_third_before =
            block0_seeds[round] +
            block0_third_store_word * 0x1111111111111111ULL;
        const uint64_t expected_block2_load =
            block2_seeds[round] +
            block2_load_word * 0x1111111111111111ULL;

        goto_priv(PRIV_M);
        ai_memblock_cmo_triple_seed_block(block0_paddr,
            block0_seeds[round]);
        ai_memblock_cmo_triple_seed_block(block1_paddr,
            block1_seeds[round]);
        ai_memblock_cmo_triple_seed_block(block2_paddr,
            block2_seeds[round]);
        write64(before_guard_paddr, before_guards[round]);
        write64(guard01_paddr, guard01_values[round]);
        write64(guard12_paddr, guard12_values[round]);
        write64(after_guard_paddr, after_guards[round]);
        write64(marker_paddr, marker_seeds[round]);
        write64(marker_guard_paddr, marker_guards[round]);

        goto_priv(PRIV_HS);
        TEST_SETUP_EXCEPT();
        AI_ASSERT_NO_EXCEPT_AND("before the four-generation zero-zero-inval-zero CMO probe starts, all source blocks, guards, and marker are readable",
            ld(block0_vaddr) == expected_block0_fourth_before &&
            ld(block0_older_store_vaddr) == expected_block0_older_before &&
            ld(block0_zero_load_vaddr) == expected_block0_zero_before &&
            ld(block0_second_store_vaddr) == expected_block0_second_before &&
            ld(block0_third_store_vaddr) == expected_block0_third_before &&
            ld(block1_vaddr) == block1_seeds[round] &&
            ld(block2_vaddr) == block2_seeds[round] &&
            ld(block2_load_vaddr) == expected_block2_load &&
            ld(marker_vaddr) == marker_seeds[round]
        );

        TEST_SETUP_EXCEPT();
        ai_memblock_cmo_triple_three_block_store_zero_store_zero_store_inval_store_zero_window(
            block0_vaddr, block1_vaddr, block2_vaddr, marker_vaddr,
            block0_older_store_values[round],
            block0_second_store_values[round],
            block0_third_store_values[round],
            block0_fourth_store_values[round], marker_updates[round],
            &block0_older_observed, &block0_second_observed,
            &block0_third_observed, &block0_fourth_observed,
            &block0_zero_observed, &block1_observed, &block2_observed,
            &marker_observed);
        AI_ASSERT_NO_EXCEPT_AND("under three-block CMO pressure, double CBO.ZERO then CBO.INVAL/CBO.ZERO leaves block0 fully zeroed",
            block0_older_observed == 0ULL &&
            block0_second_observed == 0ULL &&
            block0_third_observed == 0ULL &&
            block0_fourth_observed == 0ULL &&
            block0_zero_observed == 0ULL &&
            block1_observed == 0ULL &&
            block2_observed == expected_block2_load &&
            marker_observed == marker_updates[round]
        );

        TEST_SETUP_EXCEPT();
        AI_ASSERT_NO_EXCEPT_AND("after the four-generation zero-zero-inval-zero CMO probe, final CBO.ZERO clears block0, block1 is zero, block2 preserves seed, and guards remain intact",
            ld(block0_fourth_store_vaddr) == 0ULL &&
            ld(block0_older_store_vaddr) == 0ULL &&
            ld(block0_zero_load_vaddr) == 0ULL &&
            ld(block0_second_store_vaddr) == 0ULL &&
            ld(block0_third_store_vaddr) == 0ULL &&
            ld(block1_vaddr) == 0ULL &&
            ld(block1_load_vaddr) == 0ULL &&
            ld(block2_vaddr) == block2_seeds[round] &&
            ld(block2_load_vaddr) == expected_block2_load &&
            ld(before_guard_vaddr) == before_guards[round] &&
            ld(guard01_vaddr) == guard01_values[round] &&
            ld(guard12_vaddr) == guard12_values[round] &&
            ld(after_guard_vaddr) == after_guards[round] &&
            ld(marker_guard_vaddr) == marker_guards[round]
        );

        goto_priv(PRIV_M);
        TEST_ASSERT("backing memory after the four-generation zero-zero-inval-zero CMO probe keeps block0 and block1 zero, block2 seed payload, marker update, and all guards intact",
            ai_memblock_cmo_triple_block_all_zero(block0_paddr) &&
            ai_memblock_cmo_triple_block_all_zero(block1_paddr) &&
            ai_memblock_cmo_triple_block_matches_seed(block2_paddr,
                block2_seeds[round]) &&
            read64(before_guard_paddr) == before_guards[round] &&
            read64(guard01_paddr) == guard01_values[round] &&
            read64(guard12_paddr) == guard12_values[round] &&
            read64(after_guard_paddr) == after_guards[round] &&
            read64(marker_paddr) == marker_updates[round] &&
            read64(marker_guard_paddr) == marker_guards[round]
        );
    }

    ai_memblock_cmo_triple_reset_hs_env();

    TEST_END();
}

bool ai_micro_hs_interleaved_three_block_store_zero_store_zero_store_cbom_store_zero_four_generation_final_zero_corner()
{
    TEST_START();

    const enum test_page source_page = SWITCH1;
    const enum test_page marker_page = SWITCH2;
    const size_t block0_fourth_store_word = 0U;
    const size_t block0_older_store_word = 1U;
    const size_t block0_zero_load_word = 2U;
    const size_t block0_second_store_word = 4U;
    const size_t block0_third_store_word = 6U;
    const size_t block1_load_word = 3U;
    const size_t block2_load_word = 5U;
    const uintptr_t block0_vaddr = hs_page_base(source_page) + 0x3a00U;
    const uintptr_t block1_vaddr = hs_page_base(source_page) + 0x3a80U;
    const uintptr_t block2_vaddr = hs_page_base(source_page) + 0x3b00U;
    const uintptr_t block0_paddr = phys_page_base(source_page) + 0x3a00U;
    const uintptr_t block1_paddr = phys_page_base(source_page) + 0x3a80U;
    const uintptr_t block2_paddr = phys_page_base(source_page) + 0x3b00U;
    const uintptr_t block0_fourth_store_vaddr =
        block0_vaddr + block0_fourth_store_word * sizeof(uint64_t);
    const uintptr_t block0_older_store_vaddr =
        block0_vaddr + block0_older_store_word * sizeof(uint64_t);
    const uintptr_t block0_zero_load_vaddr =
        block0_vaddr + block0_zero_load_word * sizeof(uint64_t);
    const uintptr_t block0_second_store_vaddr =
        block0_vaddr + block0_second_store_word * sizeof(uint64_t);
    const uintptr_t block0_third_store_vaddr =
        block0_vaddr + block0_third_store_word * sizeof(uint64_t);
    const uintptr_t block1_load_vaddr =
        block1_vaddr + block1_load_word * sizeof(uint64_t);
    const uintptr_t block2_load_vaddr =
        block2_vaddr + block2_load_word * sizeof(uint64_t);
    const uintptr_t before_guard_vaddr = block0_vaddr - sizeof(uint64_t);
    const uintptr_t guard01_vaddr =
        block0_vaddr +
        AI_MEMBLOCK_CMO_TRIPLE_BLOCK_WORDS * sizeof(uint64_t);
    const uintptr_t guard12_vaddr =
        block1_vaddr +
        AI_MEMBLOCK_CMO_TRIPLE_BLOCK_WORDS * sizeof(uint64_t);
    const uintptr_t after_guard_vaddr =
        block2_vaddr +
        AI_MEMBLOCK_CMO_TRIPLE_BLOCK_WORDS * sizeof(uint64_t);
    const uintptr_t before_guard_paddr = block0_paddr - sizeof(uint64_t);
    const uintptr_t guard01_paddr =
        block0_paddr +
        AI_MEMBLOCK_CMO_TRIPLE_BLOCK_WORDS * sizeof(uint64_t);
    const uintptr_t guard12_paddr =
        block1_paddr +
        AI_MEMBLOCK_CMO_TRIPLE_BLOCK_WORDS * sizeof(uint64_t);
    const uintptr_t after_guard_paddr =
        block2_paddr +
        AI_MEMBLOCK_CMO_TRIPLE_BLOCK_WORDS * sizeof(uint64_t);
    const uintptr_t marker_vaddr = hs_page_base(marker_page) + 0x3a00U;
    const uintptr_t marker_paddr = phys_page_base(marker_page) + 0x3a00U;
    const uintptr_t marker_guard_vaddr = marker_vaddr + sizeof(uint64_t);
    const uintptr_t marker_guard_paddr = marker_paddr + sizeof(uint64_t);
    const uint64_t block0_seeds[AI_MEMBLOCK_CMO_TRIPLE_ROUNDS] = {
        0x3a004a005a006a00ULL,
        0x4b115b116b117b11ULL,
        0x5c226c227c228c22ULL,
    };
    const uint64_t block1_seeds[AI_MEMBLOCK_CMO_TRIPLE_ROUNDS] = {
        0x807060503a003a80ULL,
        0x918171614b114b91ULL,
        0xa29282725c225ca2ULL,
    };
    const uint64_t block2_seeds[AI_MEMBLOCK_CMO_TRIPLE_ROUNDS] = {
        0x2468ace03a003b00ULL,
        0x3579bdf14b114c11ULL,
        0x468ace025c225d22ULL,
    };
    const uint64_t block0_older_store_values[AI_MEMBLOCK_CMO_TRIPLE_ROUNDS] = {
        0x11003a0022003300ULL,
        0x22114b1133114411ULL,
        0x33225c2244225522ULL,
    };
    const uint64_t block0_second_store_values[AI_MEMBLOCK_CMO_TRIPLE_ROUNDS] = {
        0x00330022003a0011ULL,
        0x11441133114b1122ULL,
        0x22552244225c2233ULL,
    };
    const uint64_t block0_third_store_values[AI_MEMBLOCK_CMO_TRIPLE_ROUNDS] = {
        0xcafe3a00feed3a00ULL,
        0xdb0f4b110ffe4b11ULL,
        0xec205c22200f5c22ULL,
    };
    const uint64_t block0_fourth_store_values[AI_MEMBLOCK_CMO_TRIPLE_ROUNDS] = {
        0xc0013a00deadbeefULL,
        0xd1124b11efbedf00ULL,
        0xe2235c22f0cfe011ULL,
    };
    const uint64_t before_guards[AI_MEMBLOCK_CMO_TRIPLE_ROUNDS] = {
        0xc0013a00d00d3a00ULL,
        0xd1124b11e11e4b11ULL,
        0xe2235c22f22f5c22ULL,
    };
    const uint64_t guard01_values[AI_MEMBLOCK_CMO_TRIPLE_ROUNDS] = {
        0x003ad00d003ac001ULL,
        0x114be11e114bd112ULL,
        0x225cf22f225ce223ULL,
    };
    const uint64_t guard12_values[AI_MEMBLOCK_CMO_TRIPLE_ROUNDS] = {
        0x314159263a003a80ULL,
        0x271828184b114b91ULL,
        0x161803395c225ca2ULL,
    };
    const uint64_t after_guards[AI_MEMBLOCK_CMO_TRIPLE_ROUNDS] = {
        0x083a003a96269513ULL,
        0x194b114b81828272ULL,
        0x2a5c225c93038161ULL,
    };
    const uint64_t marker_seeds[AI_MEMBLOCK_CMO_TRIPLE_ROUNDS] = {
        0x89abcdef01234567ULL,
        0x9abcdef012345678ULL,
        0xabcdef0123456789ULL,
    };
    const uint64_t marker_updates[AI_MEMBLOCK_CMO_TRIPLE_ROUNDS] = {
        0x76543210fedcba98ULL,
        0x876543210fedcba9ULL,
        0x9876543210fedcbaULL,
    };
    const uint64_t marker_guards[AI_MEMBLOCK_CMO_TRIPLE_ROUNDS] = {
        0x55aa55aa3a003a00ULL,
        0x66bb66bb4b114b11ULL,
        0x77cc77cc5c225c22ULL,
    };

    ai_memblock_cmo_triple_prepare_hs_env();

    for (size_t round = 0; round < AI_MEMBLOCK_CMO_TRIPLE_ROUNDS; ++round) {
        uint64_t block0_older_observed = 0ULL;
        uint64_t block0_second_observed = 0ULL;
        uint64_t block0_third_observed = 0ULL;
        uint64_t block0_fourth_observed = 0ULL;
        uint64_t block0_zero_observed = 0ULL;
        uint64_t block1_observed = 0ULL;
        uint64_t block2_observed = 0ULL;
        uint64_t marker_observed = 0ULL;
        const uint64_t expected_block0_fourth_before =
            block0_seeds[round] +
            block0_fourth_store_word * 0x1111111111111111ULL;
        const uint64_t expected_block0_older_before =
            block0_seeds[round] +
            block0_older_store_word * 0x1111111111111111ULL;
        const uint64_t expected_block0_zero_before =
            block0_seeds[round] +
            block0_zero_load_word * 0x1111111111111111ULL;
        const uint64_t expected_block0_second_before =
            block0_seeds[round] +
            block0_second_store_word * 0x1111111111111111ULL;
        const uint64_t expected_block0_third_before =
            block0_seeds[round] +
            block0_third_store_word * 0x1111111111111111ULL;
        const uint64_t expected_block2_load =
            block2_seeds[round] +
            block2_load_word * 0x1111111111111111ULL;

        goto_priv(PRIV_M);
        ai_memblock_cmo_triple_seed_block(block0_paddr,
            block0_seeds[round]);
        ai_memblock_cmo_triple_seed_block(block1_paddr,
            block1_seeds[round]);
        ai_memblock_cmo_triple_seed_block(block2_paddr,
            block2_seeds[round]);
        write64(before_guard_paddr, before_guards[round]);
        write64(guard01_paddr, guard01_values[round]);
        write64(guard12_paddr, guard12_values[round]);
        write64(after_guard_paddr, after_guards[round]);
        write64(marker_paddr, marker_seeds[round]);
        write64(marker_guard_paddr, marker_guards[round]);

        goto_priv(PRIV_HS);
        TEST_SETUP_EXCEPT();
        AI_ASSERT_NO_EXCEPT_AND("before the four-generation zero-zero-cbom-zero CMO probe starts, all source blocks, guards, and marker are readable",
            ld(block0_vaddr) == expected_block0_fourth_before &&
            ld(block0_older_store_vaddr) == expected_block0_older_before &&
            ld(block0_zero_load_vaddr) == expected_block0_zero_before &&
            ld(block0_second_store_vaddr) == expected_block0_second_before &&
            ld(block0_third_store_vaddr) == expected_block0_third_before &&
            ld(block1_vaddr) == block1_seeds[round] &&
            ld(block2_vaddr) == block2_seeds[round] &&
            ld(block2_load_vaddr) == expected_block2_load &&
            ld(marker_vaddr) == marker_seeds[round]
        );

        TEST_SETUP_EXCEPT();
        ai_memblock_cmo_triple_three_block_store_zero_store_zero_store_cbom_store_zero_window(
            block0_vaddr, block1_vaddr, block2_vaddr, marker_vaddr,
            block0_older_store_values[round],
            block0_second_store_values[round],
            block0_third_store_values[round],
            block0_fourth_store_values[round], marker_updates[round],
            &block0_older_observed, &block0_second_observed,
            &block0_third_observed, &block0_fourth_observed,
            &block0_zero_observed, &block1_observed, &block2_observed,
            &marker_observed);
        AI_ASSERT_NO_EXCEPT_AND("under three-block CMO pressure, double CBO.ZERO then CBOM/CBO.ZERO leaves block0 fully zeroed",
            block0_older_observed == 0ULL &&
            block0_second_observed == 0ULL &&
            block0_third_observed == 0ULL &&
            block0_fourth_observed == 0ULL &&
            block0_zero_observed == 0ULL &&
            block1_observed == 0ULL &&
            block2_observed == expected_block2_load &&
            marker_observed == marker_updates[round]
        );

        TEST_SETUP_EXCEPT();
        AI_ASSERT_NO_EXCEPT_AND("after the four-generation zero-zero-cbom-zero CMO probe, final CBO.ZERO clears block0, block1 is zero, block2 preserves seed, and guards remain intact",
            ld(block0_fourth_store_vaddr) == 0ULL &&
            ld(block0_older_store_vaddr) == 0ULL &&
            ld(block0_zero_load_vaddr) == 0ULL &&
            ld(block0_second_store_vaddr) == 0ULL &&
            ld(block0_third_store_vaddr) == 0ULL &&
            ld(block1_vaddr) == 0ULL &&
            ld(block1_load_vaddr) == 0ULL &&
            ld(block2_vaddr) == block2_seeds[round] &&
            ld(block2_load_vaddr) == expected_block2_load &&
            ld(before_guard_vaddr) == before_guards[round] &&
            ld(guard01_vaddr) == guard01_values[round] &&
            ld(guard12_vaddr) == guard12_values[round] &&
            ld(after_guard_vaddr) == after_guards[round] &&
            ld(marker_guard_vaddr) == marker_guards[round]
        );

        goto_priv(PRIV_M);
        TEST_ASSERT("backing memory after the four-generation zero-zero-cbom-zero CMO probe keeps block0 and block1 zero, block2 seed payload, marker update, and all guards intact",
            ai_memblock_cmo_triple_block_all_zero(block0_paddr) &&
            ai_memblock_cmo_triple_block_all_zero(block1_paddr) &&
            ai_memblock_cmo_triple_block_matches_seed(block2_paddr,
                block2_seeds[round]) &&
            read64(before_guard_paddr) == before_guards[round] &&
            read64(guard01_paddr) == guard01_values[round] &&
            read64(guard12_paddr) == guard12_values[round] &&
            read64(after_guard_paddr) == after_guards[round] &&
            read64(marker_paddr) == marker_updates[round] &&
            read64(marker_guard_paddr) == marker_guards[round]
        );
    }

    ai_memblock_cmo_triple_reset_hs_env();

    TEST_END();
}

bool ai_micro_hs_interleaved_three_block_store_zero_store_inval_store_zero_store_zero_four_generation_final_zero_corner()
{
    TEST_START();

    const enum test_page source_page = SWITCH1;
    const enum test_page marker_page = SWITCH2;
    const size_t block0_fourth_store_word = 0U;
    const size_t block0_older_store_word = 1U;
    const size_t block0_zero_load_word = 2U;
    const size_t block0_second_store_word = 4U;
    const size_t block0_third_store_word = 6U;
    const size_t block1_load_word = 3U;
    const size_t block2_load_word = 5U;
    const uintptr_t block0_vaddr = hs_page_base(source_page) + 0x3b80U;
    const uintptr_t block1_vaddr = hs_page_base(source_page) + 0x3c00U;
    const uintptr_t block2_vaddr = hs_page_base(source_page) + 0x3c80U;
    const uintptr_t block0_paddr = phys_page_base(source_page) + 0x3b80U;
    const uintptr_t block1_paddr = phys_page_base(source_page) + 0x3c00U;
    const uintptr_t block2_paddr = phys_page_base(source_page) + 0x3c80U;
    const uintptr_t block0_fourth_store_vaddr =
        block0_vaddr + block0_fourth_store_word * sizeof(uint64_t);
    const uintptr_t block0_older_store_vaddr =
        block0_vaddr + block0_older_store_word * sizeof(uint64_t);
    const uintptr_t block0_zero_load_vaddr =
        block0_vaddr + block0_zero_load_word * sizeof(uint64_t);
    const uintptr_t block0_second_store_vaddr =
        block0_vaddr + block0_second_store_word * sizeof(uint64_t);
    const uintptr_t block0_third_store_vaddr =
        block0_vaddr + block0_third_store_word * sizeof(uint64_t);
    const uintptr_t block1_load_vaddr =
        block1_vaddr + block1_load_word * sizeof(uint64_t);
    const uintptr_t block2_load_vaddr =
        block2_vaddr + block2_load_word * sizeof(uint64_t);
    const uintptr_t before_guard_vaddr = block0_vaddr - sizeof(uint64_t);
    const uintptr_t guard01_vaddr =
        block0_vaddr +
        AI_MEMBLOCK_CMO_TRIPLE_BLOCK_WORDS * sizeof(uint64_t);
    const uintptr_t guard12_vaddr =
        block1_vaddr +
        AI_MEMBLOCK_CMO_TRIPLE_BLOCK_WORDS * sizeof(uint64_t);
    const uintptr_t after_guard_vaddr =
        block2_vaddr +
        AI_MEMBLOCK_CMO_TRIPLE_BLOCK_WORDS * sizeof(uint64_t);
    const uintptr_t before_guard_paddr = block0_paddr - sizeof(uint64_t);
    const uintptr_t guard01_paddr =
        block0_paddr +
        AI_MEMBLOCK_CMO_TRIPLE_BLOCK_WORDS * sizeof(uint64_t);
    const uintptr_t guard12_paddr =
        block1_paddr +
        AI_MEMBLOCK_CMO_TRIPLE_BLOCK_WORDS * sizeof(uint64_t);
    const uintptr_t after_guard_paddr =
        block2_paddr +
        AI_MEMBLOCK_CMO_TRIPLE_BLOCK_WORDS * sizeof(uint64_t);
    const uintptr_t marker_vaddr = hs_page_base(marker_page) + 0x3b80U;
    const uintptr_t marker_paddr = phys_page_base(marker_page) + 0x3b80U;
    const uintptr_t marker_guard_vaddr = marker_vaddr + sizeof(uint64_t);
    const uintptr_t marker_guard_paddr = marker_paddr + sizeof(uint64_t);
    const uint64_t block0_seeds[AI_MEMBLOCK_CMO_TRIPLE_ROUNDS] = {
        0x3b804b805b806b80ULL,
        0x4c916c917c918c91ULL,
        0x5da27da28da29da2ULL,
    };
    const uint64_t block1_seeds[AI_MEMBLOCK_CMO_TRIPLE_ROUNDS] = {
        0x807060503b803c00ULL,
        0x918171614c914d11ULL,
        0xa29282725da25e22ULL,
    };
    const uint64_t block2_seeds[AI_MEMBLOCK_CMO_TRIPLE_ROUNDS] = {
        0x2468ace03b803c80ULL,
        0x3579bdf14c914d91ULL,
        0x468ace025da25ea2ULL,
    };
    const uint64_t block0_older_store_values[AI_MEMBLOCK_CMO_TRIPLE_ROUNDS] = {
        0x11003b8022003380ULL,
        0x22114c9133114491ULL,
        0x33225da2442255a2ULL,
    };
    const uint64_t block0_second_store_values[AI_MEMBLOCK_CMO_TRIPLE_ROUNDS] = {
        0x00330022003b8011ULL,
        0x11441133114c9122ULL,
        0x22552244225da233ULL,
    };
    const uint64_t block0_third_store_values[AI_MEMBLOCK_CMO_TRIPLE_ROUNDS] = {
        0xcafe3b80feed3b80ULL,
        0xdb0f4c910ffe4c91ULL,
        0xec205da2200f5da2ULL,
    };
    const uint64_t block0_fourth_store_values[AI_MEMBLOCK_CMO_TRIPLE_ROUNDS] = {
        0xc0013b80deadbeefULL,
        0xd1124c91efbedf00ULL,
        0xe2235da2f0cfe011ULL,
    };
    const uint64_t before_guards[AI_MEMBLOCK_CMO_TRIPLE_ROUNDS] = {
        0xc0013b80d00d3b80ULL,
        0xd1124c91e11e4c91ULL,
        0xe2235da2f22f5da2ULL,
    };
    const uint64_t guard01_values[AI_MEMBLOCK_CMO_TRIPLE_ROUNDS] = {
        0x003bd00d003bc001ULL,
        0x114ce11e114cd112ULL,
        0x225df22f225de223ULL,
    };
    const uint64_t guard12_values[AI_MEMBLOCK_CMO_TRIPLE_ROUNDS] = {
        0x314159263b803c00ULL,
        0x271828184c914d11ULL,
        0x161803395da25e22ULL,
    };
    const uint64_t after_guards[AI_MEMBLOCK_CMO_TRIPLE_ROUNDS] = {
        0x083b803c96269513ULL,
        0x194c914d81828272ULL,
        0x2a5da25e93038161ULL,
    };
    const uint64_t marker_seeds[AI_MEMBLOCK_CMO_TRIPLE_ROUNDS] = {
        0x89abcdef01234567ULL,
        0x9abcdef012345678ULL,
        0xabcdef0123456789ULL,
    };
    const uint64_t marker_updates[AI_MEMBLOCK_CMO_TRIPLE_ROUNDS] = {
        0x76543210fedcba98ULL,
        0x876543210fedcba9ULL,
        0x9876543210fedcbaULL,
    };
    const uint64_t marker_guards[AI_MEMBLOCK_CMO_TRIPLE_ROUNDS] = {
        0x55aa55aa3b803b80ULL,
        0x66bb66bb4c914c91ULL,
        0x77cc77cc5da25da2ULL,
    };

    ai_memblock_cmo_triple_prepare_hs_env();

    for (size_t round = 0; round < AI_MEMBLOCK_CMO_TRIPLE_ROUNDS; ++round) {
        uint64_t block0_older_observed = 0ULL;
        uint64_t block0_second_observed = 0ULL;
        uint64_t block0_third_observed = 0ULL;
        uint64_t block0_fourth_observed = 0ULL;
        uint64_t block0_zero_observed = 0ULL;
        uint64_t block1_observed = 0ULL;
        uint64_t block2_observed = 0ULL;
        uint64_t marker_observed = 0ULL;
        const uint64_t expected_block0_fourth_before =
            block0_seeds[round] +
            block0_fourth_store_word * 0x1111111111111111ULL;
        const uint64_t expected_block0_older_before =
            block0_seeds[round] +
            block0_older_store_word * 0x1111111111111111ULL;
        const uint64_t expected_block0_zero_before =
            block0_seeds[round] +
            block0_zero_load_word * 0x1111111111111111ULL;
        const uint64_t expected_block0_second_before =
            block0_seeds[round] +
            block0_second_store_word * 0x1111111111111111ULL;
        const uint64_t expected_block0_third_before =
            block0_seeds[round] +
            block0_third_store_word * 0x1111111111111111ULL;
        const uint64_t expected_block1_load =
            block1_seeds[round] +
            block1_load_word * 0x1111111111111111ULL;
        const uint64_t expected_block2_load =
            block2_seeds[round] +
            block2_load_word * 0x1111111111111111ULL;

        goto_priv(PRIV_M);
        ai_memblock_cmo_triple_seed_block(block0_paddr,
            block0_seeds[round]);
        ai_memblock_cmo_triple_seed_block(block1_paddr,
            block1_seeds[round]);
        ai_memblock_cmo_triple_seed_block(block2_paddr,
            block2_seeds[round]);
        write64(before_guard_paddr, before_guards[round]);
        write64(guard01_paddr, guard01_values[round]);
        write64(guard12_paddr, guard12_values[round]);
        write64(after_guard_paddr, after_guards[round]);
        write64(marker_paddr, marker_seeds[round]);
        write64(marker_guard_paddr, marker_guards[round]);

        goto_priv(PRIV_HS);
        TEST_SETUP_EXCEPT();
        AI_ASSERT_NO_EXCEPT_AND("before the four-generation zero-inval-zero-zero CMO probe starts, all source blocks, guards, and marker are readable",
            ld(block0_vaddr) == expected_block0_fourth_before &&
            ld(block0_older_store_vaddr) == expected_block0_older_before &&
            ld(block0_zero_load_vaddr) == expected_block0_zero_before &&
            ld(block0_second_store_vaddr) == expected_block0_second_before &&
            ld(block0_third_store_vaddr) == expected_block0_third_before &&
            ld(block1_vaddr) == block1_seeds[round] &&
            ld(block1_load_vaddr) == expected_block1_load &&
            ld(block2_vaddr) == block2_seeds[round] &&
            ld(block2_load_vaddr) == expected_block2_load &&
            ld(marker_vaddr) == marker_seeds[round]
        );

        TEST_SETUP_EXCEPT();
        ai_memblock_cmo_triple_three_block_store_zero_store_inval_store_zero_store_zero_window(
            block0_vaddr, block1_vaddr, block2_vaddr, marker_vaddr,
            block0_older_store_values[round],
            block0_second_store_values[round],
            block0_third_store_values[round],
            block0_fourth_store_values[round], marker_updates[round],
            &block0_older_observed, &block0_second_observed,
            &block0_third_observed, &block0_fourth_observed,
            &block0_zero_observed, &block1_observed, &block2_observed,
            &marker_observed);
        AI_ASSERT_NO_EXCEPT_AND("under three-block CMO pressure, CBO.ZERO/CBO.INVAL/CBO.ZERO/CBO.ZERO leaves block0 fully zeroed",
            block0_older_observed == 0ULL &&
            block0_second_observed == 0ULL &&
            block0_third_observed == 0ULL &&
            block0_fourth_observed == 0ULL &&
            block0_zero_observed == 0ULL &&
            block1_observed == expected_block1_load &&
            block2_observed == 0ULL &&
            marker_observed == marker_updates[round]
        );

        TEST_SETUP_EXCEPT();
        AI_ASSERT_NO_EXCEPT_AND("after the four-generation zero-inval-zero-zero CMO probe, final CBO.ZERO clears block0, block1 preserves seed, block2 is zero, and guards remain intact",
            ld(block0_fourth_store_vaddr) == 0ULL &&
            ld(block0_older_store_vaddr) == 0ULL &&
            ld(block0_zero_load_vaddr) == 0ULL &&
            ld(block0_second_store_vaddr) == 0ULL &&
            ld(block0_third_store_vaddr) == 0ULL &&
            ld(block1_vaddr) == block1_seeds[round] &&
            ld(block1_load_vaddr) == expected_block1_load &&
            ld(block2_vaddr) == 0ULL &&
            ld(block2_load_vaddr) == 0ULL &&
            ld(before_guard_vaddr) == before_guards[round] &&
            ld(guard01_vaddr) == guard01_values[round] &&
            ld(guard12_vaddr) == guard12_values[round] &&
            ld(after_guard_vaddr) == after_guards[round] &&
            ld(marker_guard_vaddr) == marker_guards[round]
        );

        goto_priv(PRIV_M);
        TEST_ASSERT("backing memory after the four-generation zero-inval-zero-zero CMO probe keeps block0 zero, block1 seed payload, block2 zero, marker update, and all guards intact",
            ai_memblock_cmo_triple_block_all_zero(block0_paddr) &&
            ai_memblock_cmo_triple_block_matches_seed(block1_paddr,
                block1_seeds[round]) &&
            ai_memblock_cmo_triple_block_all_zero(block2_paddr) &&
            read64(before_guard_paddr) == before_guards[round] &&
            read64(guard01_paddr) == guard01_values[round] &&
            read64(guard12_paddr) == guard12_values[round] &&
            read64(after_guard_paddr) == after_guards[round] &&
            read64(marker_paddr) == marker_updates[round] &&
            read64(marker_guard_paddr) == marker_guards[round]
        );
    }

    ai_memblock_cmo_triple_reset_hs_env();

    TEST_END();
}

bool ai_micro_hs_interleaved_three_block_store_zero_store_inval_store_inval_store_zero_four_generation_final_zero_corner()
{
    TEST_START();

    const enum test_page source_page = SWITCH1;
    const enum test_page marker_page = SWITCH2;
    const size_t block0_fourth_store_word = 0U;
    const size_t block0_older_store_word = 1U;
    const size_t block0_zero_load_word = 2U;
    const size_t block0_second_store_word = 4U;
    const size_t block0_third_store_word = 6U;
    const size_t block1_load_word = 3U;
    const size_t block2_load_word = 5U;
    const uintptr_t block0_vaddr = hs_page_base(source_page) + 0x3d00U;
    const uintptr_t block1_vaddr = hs_page_base(source_page) + 0x3d80U;
    const uintptr_t block2_vaddr = hs_page_base(source_page) + 0x3e00U;
    const uintptr_t block0_paddr = phys_page_base(source_page) + 0x3d00U;
    const uintptr_t block1_paddr = phys_page_base(source_page) + 0x3d80U;
    const uintptr_t block2_paddr = phys_page_base(source_page) + 0x3e00U;
    const uintptr_t block0_fourth_store_vaddr =
        block0_vaddr + block0_fourth_store_word * sizeof(uint64_t);
    const uintptr_t block0_older_store_vaddr =
        block0_vaddr + block0_older_store_word * sizeof(uint64_t);
    const uintptr_t block0_zero_load_vaddr =
        block0_vaddr + block0_zero_load_word * sizeof(uint64_t);
    const uintptr_t block0_second_store_vaddr =
        block0_vaddr + block0_second_store_word * sizeof(uint64_t);
    const uintptr_t block0_third_store_vaddr =
        block0_vaddr + block0_third_store_word * sizeof(uint64_t);
    const uintptr_t block1_load_vaddr =
        block1_vaddr + block1_load_word * sizeof(uint64_t);
    const uintptr_t block2_load_vaddr =
        block2_vaddr + block2_load_word * sizeof(uint64_t);
    const uintptr_t before_guard_vaddr = block0_vaddr - sizeof(uint64_t);
    const uintptr_t guard01_vaddr =
        block0_vaddr +
        AI_MEMBLOCK_CMO_TRIPLE_BLOCK_WORDS * sizeof(uint64_t);
    const uintptr_t guard12_vaddr =
        block1_vaddr +
        AI_MEMBLOCK_CMO_TRIPLE_BLOCK_WORDS * sizeof(uint64_t);
    const uintptr_t after_guard_vaddr =
        block2_vaddr +
        AI_MEMBLOCK_CMO_TRIPLE_BLOCK_WORDS * sizeof(uint64_t);
    const uintptr_t before_guard_paddr = block0_paddr - sizeof(uint64_t);
    const uintptr_t guard01_paddr =
        block0_paddr +
        AI_MEMBLOCK_CMO_TRIPLE_BLOCK_WORDS * sizeof(uint64_t);
    const uintptr_t guard12_paddr =
        block1_paddr +
        AI_MEMBLOCK_CMO_TRIPLE_BLOCK_WORDS * sizeof(uint64_t);
    const uintptr_t after_guard_paddr =
        block2_paddr +
        AI_MEMBLOCK_CMO_TRIPLE_BLOCK_WORDS * sizeof(uint64_t);
    const uintptr_t marker_vaddr = hs_page_base(marker_page) + 0x3d00U;
    const uintptr_t marker_paddr = phys_page_base(marker_page) + 0x3d00U;
    const uintptr_t marker_guard_vaddr = marker_vaddr + sizeof(uint64_t);
    const uintptr_t marker_guard_paddr = marker_paddr + sizeof(uint64_t);
    const uint64_t block0_seeds[AI_MEMBLOCK_CMO_TRIPLE_ROUNDS] = {
        0x3d004d005d006d00ULL,
        0x4e116e117e118e11ULL,
        0x5f227f228f229f22ULL,
    };
    const uint64_t block1_seeds[AI_MEMBLOCK_CMO_TRIPLE_ROUNDS] = {
        0x807060503d003d80ULL,
        0x918171614e114e91ULL,
        0xa29282725f225fa2ULL,
    };
    const uint64_t block2_seeds[AI_MEMBLOCK_CMO_TRIPLE_ROUNDS] = {
        0x2468ace03d003e00ULL,
        0x3579bdf14e114f11ULL,
        0x468ace025f226022ULL,
    };
    const uint64_t block0_older_store_values[AI_MEMBLOCK_CMO_TRIPLE_ROUNDS] = {
        0x11003d0022003300ULL,
        0x22114e1133114411ULL,
        0x33225f2244225522ULL,
    };
    const uint64_t block0_second_store_values[AI_MEMBLOCK_CMO_TRIPLE_ROUNDS] = {
        0x00330022003d0011ULL,
        0x11441133114e1122ULL,
        0x22552244225f2233ULL,
    };
    const uint64_t block0_third_store_values[AI_MEMBLOCK_CMO_TRIPLE_ROUNDS] = {
        0xcafe3d00feed3d00ULL,
        0xdb0f4e110ffe4e11ULL,
        0xec205f22200f5f22ULL,
    };
    const uint64_t block0_fourth_store_values[AI_MEMBLOCK_CMO_TRIPLE_ROUNDS] = {
        0xc0013d00deadbeefULL,
        0xd1124e11efbedf00ULL,
        0xe2235f22f0cfe011ULL,
    };
    const uint64_t before_guards[AI_MEMBLOCK_CMO_TRIPLE_ROUNDS] = {
        0xc0013d00d00d3d00ULL,
        0xd1124e11e11e4e11ULL,
        0xe2235f22f22f5f22ULL,
    };
    const uint64_t guard01_values[AI_MEMBLOCK_CMO_TRIPLE_ROUNDS] = {
        0x003dd00d003dc001ULL,
        0x114ee11e114ed112ULL,
        0x225ff22f225fe223ULL,
    };
    const uint64_t guard12_values[AI_MEMBLOCK_CMO_TRIPLE_ROUNDS] = {
        0x314159263d003d80ULL,
        0x271828184e114e91ULL,
        0x161803395f225fa2ULL,
    };
    const uint64_t after_guards[AI_MEMBLOCK_CMO_TRIPLE_ROUNDS] = {
        0x083d003e96269513ULL,
        0x194e114f81828272ULL,
        0x2a5f226093038161ULL,
    };
    const uint64_t marker_seeds[AI_MEMBLOCK_CMO_TRIPLE_ROUNDS] = {
        0x89abcdef01234567ULL,
        0x9abcdef012345678ULL,
        0xabcdef0123456789ULL,
    };
    const uint64_t marker_updates[AI_MEMBLOCK_CMO_TRIPLE_ROUNDS] = {
        0x76543210fedcba98ULL,
        0x876543210fedcba9ULL,
        0x9876543210fedcbaULL,
    };
    const uint64_t marker_guards[AI_MEMBLOCK_CMO_TRIPLE_ROUNDS] = {
        0x55aa55aa3d003d00ULL,
        0x66bb66bb4e114e11ULL,
        0x77cc77cc5f225f22ULL,
    };

    ai_memblock_cmo_triple_prepare_hs_env();

    for (size_t round = 0; round < AI_MEMBLOCK_CMO_TRIPLE_ROUNDS; ++round) {
        uint64_t block0_older_observed = 0ULL;
        uint64_t block0_second_observed = 0ULL;
        uint64_t block0_third_observed = 0ULL;
        uint64_t block0_fourth_observed = 0ULL;
        uint64_t block0_zero_observed = 0ULL;
        uint64_t block1_observed = 0ULL;
        uint64_t block2_observed = 0ULL;
        uint64_t marker_observed = 0ULL;
        const uint64_t expected_block0_fourth_before =
            block0_seeds[round] +
            block0_fourth_store_word * 0x1111111111111111ULL;
        const uint64_t expected_block0_older_before =
            block0_seeds[round] +
            block0_older_store_word * 0x1111111111111111ULL;
        const uint64_t expected_block0_zero_before =
            block0_seeds[round] +
            block0_zero_load_word * 0x1111111111111111ULL;
        const uint64_t expected_block0_second_before =
            block0_seeds[round] +
            block0_second_store_word * 0x1111111111111111ULL;
        const uint64_t expected_block0_third_before =
            block0_seeds[round] +
            block0_third_store_word * 0x1111111111111111ULL;
        const uint64_t expected_block1_load =
            block1_seeds[round] +
            block1_load_word * 0x1111111111111111ULL;
        const uint64_t expected_block2_load =
            block2_seeds[round] +
            block2_load_word * 0x1111111111111111ULL;

        goto_priv(PRIV_M);
        ai_memblock_cmo_triple_seed_block(block0_paddr,
            block0_seeds[round]);
        ai_memblock_cmo_triple_seed_block(block1_paddr,
            block1_seeds[round]);
        ai_memblock_cmo_triple_seed_block(block2_paddr,
            block2_seeds[round]);
        write64(before_guard_paddr, before_guards[round]);
        write64(guard01_paddr, guard01_values[round]);
        write64(guard12_paddr, guard12_values[round]);
        write64(after_guard_paddr, after_guards[round]);
        write64(marker_paddr, marker_seeds[round]);
        write64(marker_guard_paddr, marker_guards[round]);

        goto_priv(PRIV_HS);
        TEST_SETUP_EXCEPT();
        AI_ASSERT_NO_EXCEPT_AND("before the four-generation zero-inval-inval-zero CMO probe starts, all source blocks, guards, and marker are readable",
            ld(block0_vaddr) == expected_block0_fourth_before &&
            ld(block0_older_store_vaddr) == expected_block0_older_before &&
            ld(block0_zero_load_vaddr) == expected_block0_zero_before &&
            ld(block0_second_store_vaddr) == expected_block0_second_before &&
            ld(block0_third_store_vaddr) == expected_block0_third_before &&
            ld(block1_vaddr) == block1_seeds[round] &&
            ld(block1_load_vaddr) == expected_block1_load &&
            ld(block2_vaddr) == block2_seeds[round] &&
            ld(block2_load_vaddr) == expected_block2_load &&
            ld(marker_vaddr) == marker_seeds[round]
        );

        TEST_SETUP_EXCEPT();
        ai_memblock_cmo_triple_three_block_store_zero_store_inval_store_inval_store_zero_window(
            block0_vaddr, block1_vaddr, block2_vaddr, marker_vaddr,
            block0_older_store_values[round],
            block0_second_store_values[round],
            block0_third_store_values[round],
            block0_fourth_store_values[round], marker_updates[round],
            &block0_older_observed, &block0_second_observed,
            &block0_third_observed, &block0_fourth_observed,
            &block0_zero_observed, &block1_observed, &block2_observed,
            &marker_observed);
        AI_ASSERT_NO_EXCEPT_AND("under three-block CMO pressure, CBO.ZERO/CBO.INVAL/CBO.INVAL/CBO.ZERO leaves block0 fully zeroed",
            block0_older_observed == 0ULL &&
            block0_second_observed == 0ULL &&
            block0_third_observed == 0ULL &&
            block0_fourth_observed == 0ULL &&
            block0_zero_observed == 0ULL &&
            block1_observed == expected_block1_load &&
            block2_observed == 0ULL &&
            marker_observed == marker_updates[round]
        );

        TEST_SETUP_EXCEPT();
        AI_ASSERT_NO_EXCEPT_AND("after the four-generation zero-inval-inval-zero CMO probe, final CBO.ZERO clears block0, block1 preserves seed, block2 is zero, and guards remain intact",
            ld(block0_fourth_store_vaddr) == 0ULL &&
            ld(block0_older_store_vaddr) == 0ULL &&
            ld(block0_zero_load_vaddr) == 0ULL &&
            ld(block0_second_store_vaddr) == 0ULL &&
            ld(block0_third_store_vaddr) == 0ULL &&
            ld(block1_vaddr) == block1_seeds[round] &&
            ld(block1_load_vaddr) == expected_block1_load &&
            ld(block2_vaddr) == 0ULL &&
            ld(block2_load_vaddr) == 0ULL &&
            ld(before_guard_vaddr) == before_guards[round] &&
            ld(guard01_vaddr) == guard01_values[round] &&
            ld(guard12_vaddr) == guard12_values[round] &&
            ld(after_guard_vaddr) == after_guards[round] &&
            ld(marker_guard_vaddr) == marker_guards[round]
        );

        goto_priv(PRIV_M);
        TEST_ASSERT("backing memory after the four-generation zero-inval-inval-zero CMO probe keeps block0 zero, block1 seed payload, block2 zero, marker update, and all guards intact",
            ai_memblock_cmo_triple_block_all_zero(block0_paddr) &&
            ai_memblock_cmo_triple_block_matches_seed(block1_paddr,
                block1_seeds[round]) &&
            ai_memblock_cmo_triple_block_all_zero(block2_paddr) &&
            read64(before_guard_paddr) == before_guards[round] &&
            read64(guard01_paddr) == guard01_values[round] &&
            read64(guard12_paddr) == guard12_values[round] &&
            read64(after_guard_paddr) == after_guards[round] &&
            read64(marker_paddr) == marker_updates[round] &&
            read64(marker_guard_paddr) == marker_guards[round]
        );
    }

    ai_memblock_cmo_triple_reset_hs_env();

    TEST_END();
}

bool ai_micro_hs_interleaved_three_block_store_zero_store_inval_store_inval_store_inval_four_generation_post_first_zero_corner()
{
    TEST_START();

    const enum test_page source_page = SWITCH1;
    const enum test_page marker_page = SWITCH2;
    const size_t block0_fourth_store_word = 0U;
    const size_t block0_older_store_word = 1U;
    const size_t block0_zero_load_word = 2U;
    const size_t block0_second_store_word = 4U;
    const size_t block0_third_store_word = 6U;
    const size_t block1_load_word = 3U;
    const size_t block2_load_word = 5U;
    const uintptr_t block0_vaddr = hs_page_base(source_page) + 0x3e80U;
    const uintptr_t block1_vaddr = hs_page_base(source_page) + 0x3f00U;
    const uintptr_t block2_vaddr = hs_page_base(source_page) + 0x3f80U;
    const uintptr_t block0_paddr = phys_page_base(source_page) + 0x3e80U;
    const uintptr_t block1_paddr = phys_page_base(source_page) + 0x3f00U;
    const uintptr_t block2_paddr = phys_page_base(source_page) + 0x3f80U;
    const uintptr_t block0_fourth_store_vaddr =
        block0_vaddr + block0_fourth_store_word * sizeof(uint64_t);
    const uintptr_t block0_older_store_vaddr =
        block0_vaddr + block0_older_store_word * sizeof(uint64_t);
    const uintptr_t block0_zero_load_vaddr =
        block0_vaddr + block0_zero_load_word * sizeof(uint64_t);
    const uintptr_t block0_second_store_vaddr =
        block0_vaddr + block0_second_store_word * sizeof(uint64_t);
    const uintptr_t block0_third_store_vaddr =
        block0_vaddr + block0_third_store_word * sizeof(uint64_t);
    const uintptr_t block1_load_vaddr =
        block1_vaddr + block1_load_word * sizeof(uint64_t);
    const uintptr_t block2_load_vaddr =
        block2_vaddr + block2_load_word * sizeof(uint64_t);
    const uintptr_t before_guard_vaddr = block0_vaddr - sizeof(uint64_t);
    const uintptr_t guard01_vaddr =
        block0_vaddr +
        AI_MEMBLOCK_CMO_TRIPLE_BLOCK_WORDS * sizeof(uint64_t);
    const uintptr_t guard12_vaddr =
        block1_vaddr +
        AI_MEMBLOCK_CMO_TRIPLE_BLOCK_WORDS * sizeof(uint64_t);
    const uintptr_t after_guard_vaddr =
        block2_vaddr +
        AI_MEMBLOCK_CMO_TRIPLE_BLOCK_WORDS * sizeof(uint64_t);
    const uintptr_t before_guard_paddr = block0_paddr - sizeof(uint64_t);
    const uintptr_t guard01_paddr =
        block0_paddr +
        AI_MEMBLOCK_CMO_TRIPLE_BLOCK_WORDS * sizeof(uint64_t);
    const uintptr_t guard12_paddr =
        block1_paddr +
        AI_MEMBLOCK_CMO_TRIPLE_BLOCK_WORDS * sizeof(uint64_t);
    const uintptr_t after_guard_paddr =
        block2_paddr +
        AI_MEMBLOCK_CMO_TRIPLE_BLOCK_WORDS * sizeof(uint64_t);
    const uintptr_t marker_vaddr = hs_page_base(marker_page) + 0x3e80U;
    const uintptr_t marker_paddr = phys_page_base(marker_page) + 0x3e80U;
    const uintptr_t marker_guard_vaddr = marker_vaddr + sizeof(uint64_t);
    const uintptr_t marker_guard_paddr = marker_paddr + sizeof(uint64_t);
    const uint64_t block0_seeds[AI_MEMBLOCK_CMO_TRIPLE_ROUNDS] = {
        0x3e804e805e806e80ULL,
        0x4f915f916f917f91ULL,
        0x60a270a280a290a2ULL,
    };
    const uint64_t block1_seeds[AI_MEMBLOCK_CMO_TRIPLE_ROUNDS] = {
        0x807060503e803f00ULL,
        0x918171614f915011ULL,
        0xa292827260a26122ULL,
    };
    const uint64_t block2_seeds[AI_MEMBLOCK_CMO_TRIPLE_ROUNDS] = {
        0x2468ace03e803f80ULL,
        0x3579bdf14f915091ULL,
        0x468ace0260a261a2ULL,
    };
    const uint64_t block0_older_store_values[AI_MEMBLOCK_CMO_TRIPLE_ROUNDS] = {
        0x11003e8022003300ULL,
        0x22114f9133114411ULL,
        0x332260a244225522ULL,
    };
    const uint64_t block0_second_store_values[AI_MEMBLOCK_CMO_TRIPLE_ROUNDS] = {
        0x00330022003e8011ULL,
        0x11441133114f9122ULL,
        0x225522442260a233ULL,
    };
    const uint64_t block0_third_store_values[AI_MEMBLOCK_CMO_TRIPLE_ROUNDS] = {
        0xcafe3e80feed3e80ULL,
        0xdb0f4f910ffe4f91ULL,
        0xec2060a2200f60a2ULL,
    };
    const uint64_t block0_fourth_store_values[AI_MEMBLOCK_CMO_TRIPLE_ROUNDS] = {
        0xc0013e80deadbeefULL,
        0xd1124f91efbedf00ULL,
        0xe22360a2f0cfe011ULL,
    };
    const uint64_t before_guards[AI_MEMBLOCK_CMO_TRIPLE_ROUNDS] = {
        0xc0013e80d00d3e80ULL,
        0xd1124f91e11e4f91ULL,
        0xe22360a2f22f60a2ULL,
    };
    const uint64_t guard01_values[AI_MEMBLOCK_CMO_TRIPLE_ROUNDS] = {
        0x3e80d00d3e80c001ULL,
        0x4f91e11e4f91d112ULL,
        0x60a2f22f60a2e223ULL,
    };
    const uint64_t guard12_values[AI_MEMBLOCK_CMO_TRIPLE_ROUNDS] = {
        0x314159263e803f00ULL,
        0x271828184f915011ULL,
        0x1618033960a26122ULL,
    };
    const uint64_t after_guards[AI_MEMBLOCK_CMO_TRIPLE_ROUNDS] = {
        0x083e803f80926951ULL,
        0x194f915091818282ULL,
        0x2a60a261a2930381ULL,
    };
    const uint64_t marker_seeds[AI_MEMBLOCK_CMO_TRIPLE_ROUNDS] = {
        0x89abcdef01234567ULL,
        0x9abcdef012345678ULL,
        0xabcdef0123456789ULL,
    };
    const uint64_t marker_updates[AI_MEMBLOCK_CMO_TRIPLE_ROUNDS] = {
        0x76543210fedcba98ULL,
        0x876543210fedcba9ULL,
        0x9876543210fedcbaULL,
    };
    const uint64_t marker_guards[AI_MEMBLOCK_CMO_TRIPLE_ROUNDS] = {
        0x55aa55aa3e803e80ULL,
        0x66bb66bb4f914f91ULL,
        0x77cc77cc60a260a2ULL,
    };

    ai_memblock_cmo_triple_prepare_hs_env();

    for (size_t round = 0; round < AI_MEMBLOCK_CMO_TRIPLE_ROUNDS; ++round) {
        uint64_t block0_older_observed = 0ULL;
        uint64_t block0_second_observed = 0ULL;
        uint64_t block0_third_observed = 0ULL;
        uint64_t block0_fourth_observed = 0ULL;
        uint64_t block0_zero_observed = 0ULL;
        uint64_t block1_observed = 0ULL;
        uint64_t block2_observed = 0ULL;
        uint64_t marker_observed = 0ULL;
        const uint64_t expected_block0_fourth_before =
            block0_seeds[round] +
            block0_fourth_store_word * 0x1111111111111111ULL;
        const uint64_t expected_block0_older_before =
            block0_seeds[round] +
            block0_older_store_word * 0x1111111111111111ULL;
        const uint64_t expected_block0_zero_before =
            block0_seeds[round] +
            block0_zero_load_word * 0x1111111111111111ULL;
        const uint64_t expected_block0_second_before =
            block0_seeds[round] +
            block0_second_store_word * 0x1111111111111111ULL;
        const uint64_t expected_block0_third_before =
            block0_seeds[round] +
            block0_third_store_word * 0x1111111111111111ULL;
        const uint64_t expected_block1_load =
            block1_seeds[round] +
            block1_load_word * 0x1111111111111111ULL;
        const uint64_t expected_block2_load =
            block2_seeds[round] +
            block2_load_word * 0x1111111111111111ULL;

        goto_priv(PRIV_M);
        ai_memblock_cmo_triple_seed_block(block0_paddr,
            block0_seeds[round]);
        ai_memblock_cmo_triple_seed_block(block1_paddr,
            block1_seeds[round]);
        ai_memblock_cmo_triple_seed_block(block2_paddr,
            block2_seeds[round]);
        write64(before_guard_paddr, before_guards[round]);
        write64(guard01_paddr, guard01_values[round]);
        write64(guard12_paddr, guard12_values[round]);
        write64(after_guard_paddr, after_guards[round]);
        write64(marker_paddr, marker_seeds[round]);
        write64(marker_guard_paddr, marker_guards[round]);

        goto_priv(PRIV_HS);
        TEST_SETUP_EXCEPT();
        AI_ASSERT_NO_EXCEPT_AND("before the four-generation zero-inval-inval-inval CMO probe starts, all source blocks, guards, and marker are readable",
            ld(block0_vaddr) == expected_block0_fourth_before &&
            ld(block0_older_store_vaddr) == expected_block0_older_before &&
            ld(block0_zero_load_vaddr) == expected_block0_zero_before &&
            ld(block0_second_store_vaddr) == expected_block0_second_before &&
            ld(block0_third_store_vaddr) == expected_block0_third_before &&
            ld(block1_vaddr) == block1_seeds[round] &&
            ld(block1_load_vaddr) == expected_block1_load &&
            ld(block2_vaddr) == block2_seeds[round] &&
            ld(block2_load_vaddr) == expected_block2_load &&
            ld(marker_vaddr) == marker_seeds[round]
        );

        TEST_SETUP_EXCEPT();
        ai_memblock_cmo_triple_three_block_store_zero_store_inval_store_inval_store_inval_window(
            block0_vaddr, block1_vaddr, block2_vaddr, marker_vaddr,
            block0_older_store_values[round],
            block0_second_store_values[round],
            block0_third_store_values[round],
            block0_fourth_store_values[round], marker_updates[round],
            &block0_older_observed, &block0_second_observed,
            &block0_third_observed, &block0_fourth_observed,
            &block0_zero_observed, &block1_observed, &block2_observed,
            &marker_observed);
        AI_ASSERT_NO_EXCEPT_AND("under three-block CMO pressure, CBO.ZERO/CBO.INVAL/CBO.INVAL/CBO.INVAL keeps all post-zero store generations visible",
            block0_older_observed == 0ULL &&
            block0_second_observed == block0_second_store_values[round] &&
            block0_third_observed == block0_third_store_values[round] &&
            block0_fourth_observed == block0_fourth_store_values[round] &&
            block0_zero_observed == 0ULL &&
            block1_observed == expected_block1_load &&
            block2_observed == 0ULL &&
            marker_observed == marker_updates[round]
        );

        TEST_SETUP_EXCEPT();
        AI_ASSERT_NO_EXCEPT_AND("after the four-generation zero-inval-inval-inval CMO probe, block0 keeps only post-first-zero stores, block1 preserves seed, block2 is zero, and guards remain intact",
            ld(block0_fourth_store_vaddr) ==
                block0_fourth_store_values[round] &&
            ld(block0_older_store_vaddr) == 0ULL &&
            ld(block0_zero_load_vaddr) == 0ULL &&
            ld(block0_second_store_vaddr) ==
                block0_second_store_values[round] &&
            ld(block0_third_store_vaddr) ==
                block0_third_store_values[round] &&
            ld(block1_vaddr) == block1_seeds[round] &&
            ld(block1_load_vaddr) == expected_block1_load &&
            ld(block2_vaddr) == 0ULL &&
            ld(block2_load_vaddr) == 0ULL &&
            ld(before_guard_vaddr) == before_guards[round] &&
            ld(guard01_vaddr) == guard01_values[round] &&
            ld(guard12_vaddr) == guard12_values[round] &&
            ld(after_guard_vaddr) == after_guards[round] &&
            ld(marker_guard_vaddr) == marker_guards[round]
        );

        goto_priv(PRIV_M);
        TEST_ASSERT("backing memory after the four-generation zero-inval-inval-inval CMO probe keeps block0 zero except the second, third, and fourth post-first-zero stores, block1 seed payload, block2 zero, marker update, and all guards intact",
            ai_memblock_cmo_triple_block_zero_except_three_words(
                block0_paddr, block0_fourth_store_word,
                block0_fourth_store_values[round],
                block0_second_store_word,
                block0_second_store_values[round],
                block0_third_store_word,
                block0_third_store_values[round]) &&
            ai_memblock_cmo_triple_block_matches_seed(block1_paddr,
                block1_seeds[round]) &&
            ai_memblock_cmo_triple_block_all_zero(block2_paddr) &&
            read64(before_guard_paddr) == before_guards[round] &&
            read64(guard01_paddr) == guard01_values[round] &&
            read64(guard12_paddr) == guard12_values[round] &&
            read64(after_guard_paddr) == after_guards[round] &&
            read64(marker_paddr) == marker_updates[round] &&
            read64(marker_guard_paddr) == marker_guards[round]
        );
    }

    ai_memblock_cmo_triple_reset_hs_env();

    TEST_END();
}

bool ai_micro_hs_interleaved_three_block_store_zero_store_inval_store_inval_store_cbom_four_generation_post_first_zero_corner()
{
    TEST_START();

    const enum test_page source_page = SWITCH1;
    const enum test_page marker_page = SWITCH2;
    const size_t block0_fourth_store_word = 0U;
    const size_t block0_older_store_word = 1U;
    const size_t block0_zero_load_word = 2U;
    const size_t block0_second_store_word = 4U;
    const size_t block0_third_store_word = 6U;
    const size_t block1_load_word = 3U;
    const size_t block2_load_word = 5U;
    const uintptr_t block0_vaddr = hs_page_base(source_page) + 0x3c40U;
    const uintptr_t block1_vaddr = hs_page_base(source_page) + 0x3cc0U;
    const uintptr_t block2_vaddr = hs_page_base(source_page) + 0x3d40U;
    const uintptr_t block0_paddr = phys_page_base(source_page) + 0x3c40U;
    const uintptr_t block1_paddr = phys_page_base(source_page) + 0x3cc0U;
    const uintptr_t block2_paddr = phys_page_base(source_page) + 0x3d40U;
    const uintptr_t block0_fourth_store_vaddr =
        block0_vaddr + block0_fourth_store_word * sizeof(uint64_t);
    const uintptr_t block0_older_store_vaddr =
        block0_vaddr + block0_older_store_word * sizeof(uint64_t);
    const uintptr_t block0_zero_load_vaddr =
        block0_vaddr + block0_zero_load_word * sizeof(uint64_t);
    const uintptr_t block0_second_store_vaddr =
        block0_vaddr + block0_second_store_word * sizeof(uint64_t);
    const uintptr_t block0_third_store_vaddr =
        block0_vaddr + block0_third_store_word * sizeof(uint64_t);
    const uintptr_t block1_load_vaddr =
        block1_vaddr + block1_load_word * sizeof(uint64_t);
    const uintptr_t block2_load_vaddr =
        block2_vaddr + block2_load_word * sizeof(uint64_t);
    const uintptr_t before_guard_vaddr = block0_vaddr - sizeof(uint64_t);
    const uintptr_t guard01_vaddr =
        block0_vaddr +
        AI_MEMBLOCK_CMO_TRIPLE_BLOCK_WORDS * sizeof(uint64_t);
    const uintptr_t guard12_vaddr =
        block1_vaddr +
        AI_MEMBLOCK_CMO_TRIPLE_BLOCK_WORDS * sizeof(uint64_t);
    const uintptr_t after_guard_vaddr =
        block2_vaddr +
        AI_MEMBLOCK_CMO_TRIPLE_BLOCK_WORDS * sizeof(uint64_t);
    const uintptr_t before_guard_paddr = block0_paddr - sizeof(uint64_t);
    const uintptr_t guard01_paddr =
        block0_paddr +
        AI_MEMBLOCK_CMO_TRIPLE_BLOCK_WORDS * sizeof(uint64_t);
    const uintptr_t guard12_paddr =
        block1_paddr +
        AI_MEMBLOCK_CMO_TRIPLE_BLOCK_WORDS * sizeof(uint64_t);
    const uintptr_t after_guard_paddr =
        block2_paddr +
        AI_MEMBLOCK_CMO_TRIPLE_BLOCK_WORDS * sizeof(uint64_t);
    const uintptr_t marker_vaddr = hs_page_base(marker_page) + 0x3c40U;
    const uintptr_t marker_paddr = phys_page_base(marker_page) + 0x3c40U;
    const uintptr_t marker_guard_vaddr = marker_vaddr + sizeof(uint64_t);
    const uintptr_t marker_guard_paddr = marker_paddr + sizeof(uint64_t);
    const uint64_t block0_seeds[AI_MEMBLOCK_CMO_TRIPLE_ROUNDS] = {
        0x3c40500060007000ULL,
        0x5111611171118111ULL,
        0x6222722282229222ULL,
    };
    const uint64_t block1_seeds[AI_MEMBLOCK_CMO_TRIPLE_ROUNDS] = {
        0x807060503c403cc0ULL,
        0x9181716151115191ULL,
        0xa2928272622262a2ULL,
    };
    const uint64_t block2_seeds[AI_MEMBLOCK_CMO_TRIPLE_ROUNDS] = {
        0x2468ace03c403d40ULL,
        0x3579bdf151115211ULL,
        0x468ace0262226322ULL,
    };
    const uint64_t block0_older_store_values[AI_MEMBLOCK_CMO_TRIPLE_ROUNDS] = {
        0x11003c4022003300ULL,
        0x2211511133114411ULL,
        0x3322622244225522ULL,
    };
    const uint64_t block0_second_store_values[AI_MEMBLOCK_CMO_TRIPLE_ROUNDS] = {
        0x00330022003c4011ULL,
        0x1144113311511122ULL,
        0x2255224422622233ULL,
    };
    const uint64_t block0_third_store_values[AI_MEMBLOCK_CMO_TRIPLE_ROUNDS] = {
        0xcafe3c40feed3c40ULL,
        0xdb0f51110ffe5111ULL,
        0xec206222200f6222ULL,
    };
    const uint64_t block0_fourth_store_values[AI_MEMBLOCK_CMO_TRIPLE_ROUNDS] = {
        0xc0013c40deadbeefULL,
        0xd1125111efbedf00ULL,
        0xe2236222f0cfe011ULL,
    };
    const uint64_t before_guards[AI_MEMBLOCK_CMO_TRIPLE_ROUNDS] = {
        0xc0013c40d00d3c40ULL,
        0xd1125111e11e5111ULL,
        0xe2236222f22f6222ULL,
    };
    const uint64_t guard01_values[AI_MEMBLOCK_CMO_TRIPLE_ROUNDS] = {
        0x3c40d00d3c40c001ULL,
        0x5111e11e5111d112ULL,
        0x6222f22f6222e223ULL,
    };
    const uint64_t guard12_values[AI_MEMBLOCK_CMO_TRIPLE_ROUNDS] = {
        0x314159263c403cc0ULL,
        0x2718281851115191ULL,
        0x16180339622262a2ULL,
    };
    const uint64_t after_guards[AI_MEMBLOCK_CMO_TRIPLE_ROUNDS] = {
        0x083c403d40962695ULL,
        0x1951115211818282ULL,
        0x2a62226322930381ULL,
    };
    const uint64_t marker_seeds[AI_MEMBLOCK_CMO_TRIPLE_ROUNDS] = {
        0x89abcdef01234567ULL,
        0x9abcdef012345678ULL,
        0xabcdef0123456789ULL,
    };
    const uint64_t marker_updates[AI_MEMBLOCK_CMO_TRIPLE_ROUNDS] = {
        0x76543210fedcba98ULL,
        0x876543210fedcba9ULL,
        0x9876543210fedcbaULL,
    };
    const uint64_t marker_guards[AI_MEMBLOCK_CMO_TRIPLE_ROUNDS] = {
        0x55aa55aa3c403c40ULL,
        0x66bb66bb51115111ULL,
        0x77cc77cc62226222ULL,
    };

    ai_memblock_cmo_triple_prepare_hs_env();

    for (size_t round = 0; round < AI_MEMBLOCK_CMO_TRIPLE_ROUNDS; ++round) {
        uint64_t block0_older_observed = 0ULL;
        uint64_t block0_second_observed = 0ULL;
        uint64_t block0_third_observed = 0ULL;
        uint64_t block0_fourth_observed = 0ULL;
        uint64_t block0_zero_observed = 0ULL;
        uint64_t block1_observed = 0ULL;
        uint64_t block2_observed = 0ULL;
        uint64_t marker_observed = 0ULL;
        const uint64_t expected_block0_fourth_before =
            block0_seeds[round] +
            block0_fourth_store_word * 0x1111111111111111ULL;
        const uint64_t expected_block0_older_before =
            block0_seeds[round] +
            block0_older_store_word * 0x1111111111111111ULL;
        const uint64_t expected_block0_zero_before =
            block0_seeds[round] +
            block0_zero_load_word * 0x1111111111111111ULL;
        const uint64_t expected_block0_second_before =
            block0_seeds[round] +
            block0_second_store_word * 0x1111111111111111ULL;
        const uint64_t expected_block0_third_before =
            block0_seeds[round] +
            block0_third_store_word * 0x1111111111111111ULL;
        const uint64_t expected_block1_load =
            block1_seeds[round] +
            block1_load_word * 0x1111111111111111ULL;
        const uint64_t expected_block2_load =
            block2_seeds[round] +
            block2_load_word * 0x1111111111111111ULL;

        goto_priv(PRIV_M);
        ai_memblock_cmo_triple_seed_block(block0_paddr,
            block0_seeds[round]);
        ai_memblock_cmo_triple_seed_block(block1_paddr,
            block1_seeds[round]);
        ai_memblock_cmo_triple_seed_block(block2_paddr,
            block2_seeds[round]);
        write64(before_guard_paddr, before_guards[round]);
        write64(guard01_paddr, guard01_values[round]);
        write64(guard12_paddr, guard12_values[round]);
        write64(after_guard_paddr, after_guards[round]);
        write64(marker_paddr, marker_seeds[round]);
        write64(marker_guard_paddr, marker_guards[round]);

        goto_priv(PRIV_HS);
        TEST_SETUP_EXCEPT();
        AI_ASSERT_NO_EXCEPT_AND("before the four-generation zero-inval-inval-clean CMO probe starts, all source blocks, guards, and marker are readable",
            ld(block0_vaddr) == expected_block0_fourth_before &&
            ld(block0_older_store_vaddr) == expected_block0_older_before &&
            ld(block0_zero_load_vaddr) == expected_block0_zero_before &&
            ld(block0_second_store_vaddr) == expected_block0_second_before &&
            ld(block0_third_store_vaddr) == expected_block0_third_before &&
            ld(block1_vaddr) == block1_seeds[round] &&
            ld(block1_load_vaddr) == expected_block1_load &&
            ld(block2_vaddr) == block2_seeds[round] &&
            ld(block2_load_vaddr) == expected_block2_load &&
            ld(marker_vaddr) == marker_seeds[round]
        );

        TEST_SETUP_EXCEPT();
        ai_memblock_cmo_triple_three_block_store_zero_store_inval_store_inval_store_cbom_window(
            block0_vaddr, block1_vaddr, block2_vaddr, marker_vaddr,
            block0_older_store_values[round],
            block0_second_store_values[round],
            block0_third_store_values[round],
            block0_fourth_store_values[round], marker_updates[round],
            &block0_older_observed, &block0_second_observed,
            &block0_third_observed, &block0_fourth_observed,
            &block0_zero_observed, &block1_observed, &block2_observed,
            &marker_observed);
        AI_ASSERT_NO_EXCEPT_AND("under three-block CMO pressure, CBO.ZERO/CBO.INVAL/CBO.INVAL/CBOM keeps all post-zero store generations visible",
            block0_older_observed == 0ULL &&
            block0_second_observed == block0_second_store_values[round] &&
            block0_third_observed == block0_third_store_values[round] &&
            block0_fourth_observed == block0_fourth_store_values[round] &&
            block0_zero_observed == 0ULL &&
            block1_observed == expected_block1_load &&
            block2_observed == 0ULL &&
            marker_observed == marker_updates[round]
        );

        TEST_SETUP_EXCEPT();
        AI_ASSERT_NO_EXCEPT_AND("after the four-generation zero-inval-inval-clean CMO probe, block0 keeps only post-first-zero stores, block1 preserves seed, block2 is zero, and guards remain intact",
            ld(block0_fourth_store_vaddr) ==
                block0_fourth_store_values[round] &&
            ld(block0_older_store_vaddr) == 0ULL &&
            ld(block0_zero_load_vaddr) == 0ULL &&
            ld(block0_second_store_vaddr) ==
                block0_second_store_values[round] &&
            ld(block0_third_store_vaddr) ==
                block0_third_store_values[round] &&
            ld(block1_vaddr) == block1_seeds[round] &&
            ld(block1_load_vaddr) == expected_block1_load &&
            ld(block2_vaddr) == 0ULL &&
            ld(block2_load_vaddr) == 0ULL &&
            ld(before_guard_vaddr) == before_guards[round] &&
            ld(guard01_vaddr) == guard01_values[round] &&
            ld(guard12_vaddr) == guard12_values[round] &&
            ld(after_guard_vaddr) == after_guards[round] &&
            ld(marker_guard_vaddr) == marker_guards[round]
        );

        goto_priv(PRIV_M);
        TEST_ASSERT("backing memory after the four-generation zero-inval-inval-clean CMO probe keeps block0 zero except the second, third, and fourth post-first-zero stores, block1 seed payload, block2 zero, marker update, and all guards intact",
            ai_memblock_cmo_triple_block_zero_except_three_words(
                block0_paddr, block0_fourth_store_word,
                block0_fourth_store_values[round],
                block0_second_store_word,
                block0_second_store_values[round],
                block0_third_store_word,
                block0_third_store_values[round]) &&
            ai_memblock_cmo_triple_block_matches_seed(block1_paddr,
                block1_seeds[round]) &&
            ai_memblock_cmo_triple_block_all_zero(block2_paddr) &&
            read64(before_guard_paddr) == before_guards[round] &&
            read64(guard01_paddr) == guard01_values[round] &&
            read64(guard12_paddr) == guard12_values[round] &&
            read64(after_guard_paddr) == after_guards[round] &&
            read64(marker_paddr) == marker_updates[round] &&
            read64(marker_guard_paddr) == marker_guards[round]
        );
    }

    ai_memblock_cmo_triple_reset_hs_env();

    TEST_END();
}

bool ai_micro_hs_interleaved_three_block_store_zero_store_inval_store_cbom_store_cbom_four_generation_post_first_zero_corner()
{
    TEST_START();

    const enum test_page source_page = SWITCH1;
    const enum test_page marker_page = SWITCH2;
    const size_t block0_fourth_store_word = 0U;
    const size_t block0_older_store_word = 1U;
    const size_t block0_zero_load_word = 2U;
    const size_t block0_second_store_word = 4U;
    const size_t block0_third_store_word = 6U;
    const size_t block1_load_word = 3U;
    const size_t block2_load_word = 5U;
    const uintptr_t block0_vaddr = hs_page_base(source_page) + 0x3b40U;
    const uintptr_t block1_vaddr = hs_page_base(source_page) + 0x3bc0U;
    const uintptr_t block2_vaddr = hs_page_base(source_page) + 0x3c40U;
    const uintptr_t block0_paddr = phys_page_base(source_page) + 0x3b40U;
    const uintptr_t block1_paddr = phys_page_base(source_page) + 0x3bc0U;
    const uintptr_t block2_paddr = phys_page_base(source_page) + 0x3c40U;
    const uintptr_t block0_fourth_store_vaddr =
        block0_vaddr + block0_fourth_store_word * sizeof(uint64_t);
    const uintptr_t block0_older_store_vaddr =
        block0_vaddr + block0_older_store_word * sizeof(uint64_t);
    const uintptr_t block0_zero_load_vaddr =
        block0_vaddr + block0_zero_load_word * sizeof(uint64_t);
    const uintptr_t block0_second_store_vaddr =
        block0_vaddr + block0_second_store_word * sizeof(uint64_t);
    const uintptr_t block0_third_store_vaddr =
        block0_vaddr + block0_third_store_word * sizeof(uint64_t);
    const uintptr_t block1_load_vaddr =
        block1_vaddr + block1_load_word * sizeof(uint64_t);
    const uintptr_t block2_load_vaddr =
        block2_vaddr + block2_load_word * sizeof(uint64_t);
    const uintptr_t before_guard_vaddr = block0_vaddr - sizeof(uint64_t);
    const uintptr_t guard01_vaddr =
        block0_vaddr +
        AI_MEMBLOCK_CMO_TRIPLE_BLOCK_WORDS * sizeof(uint64_t);
    const uintptr_t guard12_vaddr =
        block1_vaddr +
        AI_MEMBLOCK_CMO_TRIPLE_BLOCK_WORDS * sizeof(uint64_t);
    const uintptr_t after_guard_vaddr =
        block2_vaddr +
        AI_MEMBLOCK_CMO_TRIPLE_BLOCK_WORDS * sizeof(uint64_t);
    const uintptr_t before_guard_paddr = block0_paddr - sizeof(uint64_t);
    const uintptr_t guard01_paddr =
        block0_paddr +
        AI_MEMBLOCK_CMO_TRIPLE_BLOCK_WORDS * sizeof(uint64_t);
    const uintptr_t guard12_paddr =
        block1_paddr +
        AI_MEMBLOCK_CMO_TRIPLE_BLOCK_WORDS * sizeof(uint64_t);
    const uintptr_t after_guard_paddr =
        block2_paddr +
        AI_MEMBLOCK_CMO_TRIPLE_BLOCK_WORDS * sizeof(uint64_t);
    const uintptr_t marker_vaddr = hs_page_base(marker_page) + 0x3b40U;
    const uintptr_t marker_paddr = phys_page_base(marker_page) + 0x3b40U;
    const uintptr_t marker_guard_vaddr = marker_vaddr + sizeof(uint64_t);
    const uintptr_t marker_guard_paddr = marker_paddr + sizeof(uint64_t);
    const uint64_t block0_seeds[AI_MEMBLOCK_CMO_TRIPLE_ROUNDS] = {
        0x3b40500060007000ULL,
        0x4c51611171118111ULL,
        0x5d62722282229222ULL,
    };
    const uint64_t block1_seeds[AI_MEMBLOCK_CMO_TRIPLE_ROUNDS] = {
        0x807060503b403bc0ULL,
        0x918171614c514c91ULL,
        0xa29282725d625da2ULL,
    };
    const uint64_t block2_seeds[AI_MEMBLOCK_CMO_TRIPLE_ROUNDS] = {
        0x2468ace03b403c40ULL,
        0x3579bdf14c514d11ULL,
        0x468ace025d625e22ULL,
    };
    const uint64_t block0_older_store_values[AI_MEMBLOCK_CMO_TRIPLE_ROUNDS] = {
        0x11003b4022003300ULL,
        0x22114c5133114411ULL,
        0x33225d6244225522ULL,
    };
    const uint64_t block0_second_store_values[AI_MEMBLOCK_CMO_TRIPLE_ROUNDS] = {
        0x00330022003b4011ULL,
        0x11441133114c5122ULL,
        0x22552244225d6233ULL,
    };
    const uint64_t block0_third_store_values[AI_MEMBLOCK_CMO_TRIPLE_ROUNDS] = {
        0xcafe3b40feed3b40ULL,
        0xdb0f4c510ffe4c51ULL,
        0xec205d62200f5d62ULL,
    };
    const uint64_t block0_fourth_store_values[AI_MEMBLOCK_CMO_TRIPLE_ROUNDS] = {
        0xc0013b40deadbeefULL,
        0xd1124c51efbedf00ULL,
        0xe2235d62f0cfe011ULL,
    };
    const uint64_t before_guards[AI_MEMBLOCK_CMO_TRIPLE_ROUNDS] = {
        0xc0013b40d00d3b40ULL,
        0xd1124c51e11e4c51ULL,
        0xe2235d62f22f5d62ULL,
    };
    const uint64_t guard01_values[AI_MEMBLOCK_CMO_TRIPLE_ROUNDS] = {
        0x3b40d00d3b40c001ULL,
        0x4c51e11e4c51d112ULL,
        0x5d62f22f5d62e223ULL,
    };
    const uint64_t guard12_values[AI_MEMBLOCK_CMO_TRIPLE_ROUNDS] = {
        0x314159263b403bc0ULL,
        0x271828184c514c91ULL,
        0x161803395d625da2ULL,
    };
    const uint64_t after_guards[AI_MEMBLOCK_CMO_TRIPLE_ROUNDS] = {
        0x083b403c40962695ULL,
        0x194c514d11818282ULL,
        0x2a5d625e22930381ULL,
    };
    const uint64_t marker_seeds[AI_MEMBLOCK_CMO_TRIPLE_ROUNDS] = {
        0x89abcdef01234567ULL,
        0x9abcdef012345678ULL,
        0xabcdef0123456789ULL,
    };
    const uint64_t marker_updates[AI_MEMBLOCK_CMO_TRIPLE_ROUNDS] = {
        0x76543210fedcba98ULL,
        0x876543210fedcba9ULL,
        0x9876543210fedcbaULL,
    };
    const uint64_t marker_guards[AI_MEMBLOCK_CMO_TRIPLE_ROUNDS] = {
        0x55aa55aa3b403b40ULL,
        0x66bb66bb4c514c51ULL,
        0x77cc77cc5d625d62ULL,
    };

    ai_memblock_cmo_triple_prepare_hs_env();

    for (size_t round = 0; round < AI_MEMBLOCK_CMO_TRIPLE_ROUNDS; ++round) {
        uint64_t block0_older_observed = 0ULL;
        uint64_t block0_second_observed = 0ULL;
        uint64_t block0_third_observed = 0ULL;
        uint64_t block0_fourth_observed = 0ULL;
        uint64_t block0_zero_observed = 0ULL;
        uint64_t block1_observed = 0ULL;
        uint64_t block2_observed = 0ULL;
        uint64_t marker_observed = 0ULL;
        const uint64_t expected_block0_fourth_before =
            block0_seeds[round] +
            block0_fourth_store_word * 0x1111111111111111ULL;
        const uint64_t expected_block0_older_before =
            block0_seeds[round] +
            block0_older_store_word * 0x1111111111111111ULL;
        const uint64_t expected_block0_zero_before =
            block0_seeds[round] +
            block0_zero_load_word * 0x1111111111111111ULL;
        const uint64_t expected_block0_second_before =
            block0_seeds[round] +
            block0_second_store_word * 0x1111111111111111ULL;
        const uint64_t expected_block0_third_before =
            block0_seeds[round] +
            block0_third_store_word * 0x1111111111111111ULL;
        const uint64_t expected_block1_load =
            block1_seeds[round] +
            block1_load_word * 0x1111111111111111ULL;
        const uint64_t expected_block2_load =
            block2_seeds[round] +
            block2_load_word * 0x1111111111111111ULL;

        goto_priv(PRIV_M);
        ai_memblock_cmo_triple_seed_block(block0_paddr,
            block0_seeds[round]);
        ai_memblock_cmo_triple_seed_block(block1_paddr,
            block1_seeds[round]);
        ai_memblock_cmo_triple_seed_block(block2_paddr,
            block2_seeds[round]);
        write64(before_guard_paddr, before_guards[round]);
        write64(guard01_paddr, guard01_values[round]);
        write64(guard12_paddr, guard12_values[round]);
        write64(after_guard_paddr, after_guards[round]);
        write64(marker_paddr, marker_seeds[round]);
        write64(marker_guard_paddr, marker_guards[round]);

        goto_priv(PRIV_HS);
        TEST_SETUP_EXCEPT();
        AI_ASSERT_NO_EXCEPT_AND("before the four-generation zero-inval-clean-clean CMO probe starts, all source blocks, guards, and marker are readable",
            ld(block0_vaddr) == expected_block0_fourth_before &&
            ld(block0_older_store_vaddr) == expected_block0_older_before &&
            ld(block0_zero_load_vaddr) == expected_block0_zero_before &&
            ld(block0_second_store_vaddr) == expected_block0_second_before &&
            ld(block0_third_store_vaddr) == expected_block0_third_before &&
            ld(block1_vaddr) == block1_seeds[round] &&
            ld(block1_load_vaddr) == expected_block1_load &&
            ld(block2_vaddr) == block2_seeds[round] &&
            ld(block2_load_vaddr) == expected_block2_load &&
            ld(marker_vaddr) == marker_seeds[round]
        );

        TEST_SETUP_EXCEPT();
        ai_memblock_cmo_triple_three_block_store_zero_store_inval_store_cbom_store_cbom_window(
            block0_vaddr, block1_vaddr, block2_vaddr, marker_vaddr,
            block0_older_store_values[round],
            block0_second_store_values[round],
            block0_third_store_values[round],
            block0_fourth_store_values[round], marker_updates[round],
            &block0_older_observed, &block0_second_observed,
            &block0_third_observed, &block0_fourth_observed,
            &block0_zero_observed, &block1_observed, &block2_observed,
            &marker_observed);
        AI_ASSERT_NO_EXCEPT_AND("under three-block CMO pressure, CBO.ZERO/CBO.INVAL/CBOM/CBOM keeps all post-zero store generations visible",
            block0_older_observed == 0ULL &&
            block0_second_observed == block0_second_store_values[round] &&
            block0_third_observed == block0_third_store_values[round] &&
            block0_fourth_observed == block0_fourth_store_values[round] &&
            block0_zero_observed == 0ULL &&
            block1_observed == expected_block1_load &&
            block2_observed == 0ULL &&
            marker_observed == marker_updates[round]
        );

        TEST_SETUP_EXCEPT();
        AI_ASSERT_NO_EXCEPT_AND("after the four-generation zero-inval-clean-clean CMO probe, block0 keeps only post-first-zero stores, block1 preserves seed, block2 is zero, and guards remain intact",
            ld(block0_fourth_store_vaddr) ==
                block0_fourth_store_values[round] &&
            ld(block0_older_store_vaddr) == 0ULL &&
            ld(block0_zero_load_vaddr) == 0ULL &&
            ld(block0_second_store_vaddr) ==
                block0_second_store_values[round] &&
            ld(block0_third_store_vaddr) ==
                block0_third_store_values[round] &&
            ld(block1_vaddr) == block1_seeds[round] &&
            ld(block1_load_vaddr) == expected_block1_load &&
            ld(block2_vaddr) == 0ULL &&
            ld(block2_load_vaddr) == 0ULL &&
            ld(before_guard_vaddr) == before_guards[round] &&
            ld(guard01_vaddr) == guard01_values[round] &&
            ld(guard12_vaddr) == guard12_values[round] &&
            ld(after_guard_vaddr) == after_guards[round] &&
            ld(marker_guard_vaddr) == marker_guards[round]
        );

        goto_priv(PRIV_M);
        TEST_ASSERT("backing memory after the four-generation zero-inval-clean-clean CMO probe keeps block0 zero except the second, third, and fourth post-first-zero stores, block1 seed payload, block2 zero, marker update, and all guards intact",
            ai_memblock_cmo_triple_block_zero_except_three_words(
                block0_paddr, block0_fourth_store_word,
                block0_fourth_store_values[round],
                block0_second_store_word,
                block0_second_store_values[round],
                block0_third_store_word,
                block0_third_store_values[round]) &&
            ai_memblock_cmo_triple_block_matches_seed(block1_paddr,
                block1_seeds[round]) &&
            ai_memblock_cmo_triple_block_all_zero(block2_paddr) &&
            read64(before_guard_paddr) == before_guards[round] &&
            read64(guard01_paddr) == guard01_values[round] &&
            read64(guard12_paddr) == guard12_values[round] &&
            read64(after_guard_paddr) == after_guards[round] &&
            read64(marker_paddr) == marker_updates[round] &&
            read64(marker_guard_paddr) == marker_guards[round]
        );
    }

    ai_memblock_cmo_triple_reset_hs_env();

    TEST_END();
}

bool ai_micro_hs_interleaved_three_block_store_zero_store_cbom_store_zero_store_zero_four_generation_final_zero_corner()
{
    TEST_START();

    const enum test_page source_page = SWITCH1;
    const enum test_page marker_page = SWITCH2;
    const size_t block0_fourth_store_word = 0U;
    const size_t block0_older_store_word = 1U;
    const size_t block0_zero_load_word = 2U;
    const size_t block0_second_store_word = 4U;
    const size_t block0_third_store_word = 6U;
    const size_t block1_load_word = 3U;
    const size_t block2_load_word = 5U;
    const uintptr_t block0_vaddr = hs_page_base(source_page) + 0x3a40U;
    const uintptr_t block1_vaddr = hs_page_base(source_page) + 0x3ac0U;
    const uintptr_t block2_vaddr = hs_page_base(source_page) + 0x3b40U;
    const uintptr_t block0_paddr = phys_page_base(source_page) + 0x3a40U;
    const uintptr_t block1_paddr = phys_page_base(source_page) + 0x3ac0U;
    const uintptr_t block2_paddr = phys_page_base(source_page) + 0x3b40U;
    const uintptr_t block0_fourth_store_vaddr =
        block0_vaddr + block0_fourth_store_word * sizeof(uint64_t);
    const uintptr_t block0_older_store_vaddr =
        block0_vaddr + block0_older_store_word * sizeof(uint64_t);
    const uintptr_t block0_zero_load_vaddr =
        block0_vaddr + block0_zero_load_word * sizeof(uint64_t);
    const uintptr_t block0_second_store_vaddr =
        block0_vaddr + block0_second_store_word * sizeof(uint64_t);
    const uintptr_t block0_third_store_vaddr =
        block0_vaddr + block0_third_store_word * sizeof(uint64_t);
    const uintptr_t block1_load_vaddr =
        block1_vaddr + block1_load_word * sizeof(uint64_t);
    const uintptr_t block2_load_vaddr =
        block2_vaddr + block2_load_word * sizeof(uint64_t);
    const uintptr_t before_guard_vaddr = block0_vaddr - sizeof(uint64_t);
    const uintptr_t guard01_vaddr =
        block0_vaddr +
        AI_MEMBLOCK_CMO_TRIPLE_BLOCK_WORDS * sizeof(uint64_t);
    const uintptr_t guard12_vaddr =
        block1_vaddr +
        AI_MEMBLOCK_CMO_TRIPLE_BLOCK_WORDS * sizeof(uint64_t);
    const uintptr_t after_guard_vaddr =
        block2_vaddr +
        AI_MEMBLOCK_CMO_TRIPLE_BLOCK_WORDS * sizeof(uint64_t);
    const uintptr_t before_guard_paddr = block0_paddr - sizeof(uint64_t);
    const uintptr_t guard01_paddr =
        block0_paddr +
        AI_MEMBLOCK_CMO_TRIPLE_BLOCK_WORDS * sizeof(uint64_t);
    const uintptr_t guard12_paddr =
        block1_paddr +
        AI_MEMBLOCK_CMO_TRIPLE_BLOCK_WORDS * sizeof(uint64_t);
    const uintptr_t after_guard_paddr =
        block2_paddr +
        AI_MEMBLOCK_CMO_TRIPLE_BLOCK_WORDS * sizeof(uint64_t);
    const uintptr_t marker_vaddr = hs_page_base(marker_page) + 0x3a40U;
    const uintptr_t marker_paddr = phys_page_base(marker_page) + 0x3a40U;
    const uintptr_t marker_guard_vaddr = marker_vaddr + sizeof(uint64_t);
    const uintptr_t marker_guard_paddr = marker_paddr + sizeof(uint64_t);
    const uint64_t block0_seeds[AI_MEMBLOCK_CMO_TRIPLE_ROUNDS] = {
        0x3a40500060007000ULL,
        0x4b51611171118111ULL,
        0x5c62722282229222ULL,
    };
    const uint64_t block1_seeds[AI_MEMBLOCK_CMO_TRIPLE_ROUNDS] = {
        0x807060503a403ac0ULL,
        0x918171614b514b91ULL,
        0xa29282725c625ca2ULL,
    };
    const uint64_t block2_seeds[AI_MEMBLOCK_CMO_TRIPLE_ROUNDS] = {
        0x2468ace03a403b40ULL,
        0x3579bdf14b514c11ULL,
        0x468ace025c625d22ULL,
    };
    const uint64_t block0_older_store_values[AI_MEMBLOCK_CMO_TRIPLE_ROUNDS] = {
        0x11003a4022003300ULL,
        0x22114b5133114411ULL,
        0x33225c6244225522ULL,
    };
    const uint64_t block0_second_store_values[AI_MEMBLOCK_CMO_TRIPLE_ROUNDS] = {
        0x00330022003a4011ULL,
        0x11441133114b5122ULL,
        0x22552244225c6233ULL,
    };
    const uint64_t block0_third_store_values[AI_MEMBLOCK_CMO_TRIPLE_ROUNDS] = {
        0xcafe3a40feed3a40ULL,
        0xdb0f4b510ffe4b51ULL,
        0xec205c62200f5c62ULL,
    };
    const uint64_t block0_fourth_store_values[AI_MEMBLOCK_CMO_TRIPLE_ROUNDS] = {
        0xc0013a40deadbeefULL,
        0xd1124b51efbedf00ULL,
        0xe2235c62f0cfe011ULL,
    };
    const uint64_t before_guards[AI_MEMBLOCK_CMO_TRIPLE_ROUNDS] = {
        0xc0013a40d00d3a40ULL,
        0xd1124b51e11e4b51ULL,
        0xe2235c62f22f5c62ULL,
    };
    const uint64_t guard01_values[AI_MEMBLOCK_CMO_TRIPLE_ROUNDS] = {
        0x3a40d00d3a40c001ULL,
        0x4b51e11e4b51d112ULL,
        0x5c62f22f5c62e223ULL,
    };
    const uint64_t guard12_values[AI_MEMBLOCK_CMO_TRIPLE_ROUNDS] = {
        0x314159263a403ac0ULL,
        0x271828184b514b91ULL,
        0x161803395c625ca2ULL,
    };
    const uint64_t after_guards[AI_MEMBLOCK_CMO_TRIPLE_ROUNDS] = {
        0x083a403b40962695ULL,
        0x194b514c11818282ULL,
        0x2a5c625d22930381ULL,
    };
    const uint64_t marker_seeds[AI_MEMBLOCK_CMO_TRIPLE_ROUNDS] = {
        0x89abcdef01234567ULL,
        0x9abcdef012345678ULL,
        0xabcdef0123456789ULL,
    };
    const uint64_t marker_updates[AI_MEMBLOCK_CMO_TRIPLE_ROUNDS] = {
        0x76543210fedcba98ULL,
        0x876543210fedcba9ULL,
        0x9876543210fedcbaULL,
    };
    const uint64_t marker_guards[AI_MEMBLOCK_CMO_TRIPLE_ROUNDS] = {
        0x55aa55aa3a403a40ULL,
        0x66bb66bb4b514b51ULL,
        0x77cc77cc5c625c62ULL,
    };

    ai_memblock_cmo_triple_prepare_hs_env();

    for (size_t round = 0; round < AI_MEMBLOCK_CMO_TRIPLE_ROUNDS; ++round) {
        uint64_t block0_older_observed = 0ULL;
        uint64_t block0_second_observed = 0ULL;
        uint64_t block0_third_observed = 0ULL;
        uint64_t block0_fourth_observed = 0ULL;
        uint64_t block0_zero_observed = 0ULL;
        uint64_t block1_observed = 0ULL;
        uint64_t block2_observed = 0ULL;
        uint64_t marker_observed = 0ULL;
        const uint64_t expected_block0_fourth_before =
            block0_seeds[round] +
            block0_fourth_store_word * 0x1111111111111111ULL;
        const uint64_t expected_block0_older_before =
            block0_seeds[round] +
            block0_older_store_word * 0x1111111111111111ULL;
        const uint64_t expected_block0_zero_before =
            block0_seeds[round] +
            block0_zero_load_word * 0x1111111111111111ULL;
        const uint64_t expected_block0_second_before =
            block0_seeds[round] +
            block0_second_store_word * 0x1111111111111111ULL;
        const uint64_t expected_block0_third_before =
            block0_seeds[round] +
            block0_third_store_word * 0x1111111111111111ULL;
        const uint64_t expected_block1_load =
            block1_seeds[round] +
            block1_load_word * 0x1111111111111111ULL;
        const uint64_t expected_block2_load =
            block2_seeds[round] +
            block2_load_word * 0x1111111111111111ULL;

        goto_priv(PRIV_M);
        ai_memblock_cmo_triple_seed_block(block0_paddr,
            block0_seeds[round]);
        ai_memblock_cmo_triple_seed_block(block1_paddr,
            block1_seeds[round]);
        ai_memblock_cmo_triple_seed_block(block2_paddr,
            block2_seeds[round]);
        write64(before_guard_paddr, before_guards[round]);
        write64(guard01_paddr, guard01_values[round]);
        write64(guard12_paddr, guard12_values[round]);
        write64(after_guard_paddr, after_guards[round]);
        write64(marker_paddr, marker_seeds[round]);
        write64(marker_guard_paddr, marker_guards[round]);

        goto_priv(PRIV_HS);
        TEST_SETUP_EXCEPT();
        AI_ASSERT_NO_EXCEPT_AND("before the four-generation zero-clean-zero-zero CMO probe starts, all source blocks, guards, and marker are readable",
            ld(block0_vaddr) == expected_block0_fourth_before &&
            ld(block0_older_store_vaddr) == expected_block0_older_before &&
            ld(block0_zero_load_vaddr) == expected_block0_zero_before &&
            ld(block0_second_store_vaddr) == expected_block0_second_before &&
            ld(block0_third_store_vaddr) == expected_block0_third_before &&
            ld(block1_vaddr) == block1_seeds[round] &&
            ld(block1_load_vaddr) == expected_block1_load &&
            ld(block2_vaddr) == block2_seeds[round] &&
            ld(block2_load_vaddr) == expected_block2_load &&
            ld(marker_vaddr) == marker_seeds[round]
        );

        TEST_SETUP_EXCEPT();
        ai_memblock_cmo_triple_three_block_store_zero_store_cbom_store_zero_store_zero_window(
            block0_vaddr, block1_vaddr, block2_vaddr, marker_vaddr,
            block0_older_store_values[round],
            block0_second_store_values[round],
            block0_third_store_values[round],
            block0_fourth_store_values[round], marker_updates[round],
            &block0_older_observed, &block0_second_observed,
            &block0_third_observed, &block0_fourth_observed,
            &block0_zero_observed, &block1_observed, &block2_observed,
            &marker_observed);
        AI_ASSERT_NO_EXCEPT_AND("under three-block CMO pressure, CBO.ZERO/CBOM/CBO.ZERO/CBO.ZERO clears all store generations",
            block0_older_observed == 0ULL &&
            block0_second_observed == 0ULL &&
            block0_third_observed == 0ULL &&
            block0_fourth_observed == 0ULL &&
            block0_zero_observed == 0ULL &&
            block1_observed == expected_block1_load &&
            block2_observed == 0ULL &&
            marker_observed == marker_updates[round]
        );

        TEST_SETUP_EXCEPT();
        AI_ASSERT_NO_EXCEPT_AND("after the four-generation zero-clean-zero-zero CMO probe, block0 is fully zero, block1 preserves seed, block2 is zero, and guards remain intact",
            ld(block0_fourth_store_vaddr) == 0ULL &&
            ld(block0_older_store_vaddr) == 0ULL &&
            ld(block0_zero_load_vaddr) == 0ULL &&
            ld(block0_second_store_vaddr) == 0ULL &&
            ld(block0_third_store_vaddr) == 0ULL &&
            ld(block1_vaddr) == block1_seeds[round] &&
            ld(block1_load_vaddr) == expected_block1_load &&
            ld(block2_vaddr) == 0ULL &&
            ld(block2_load_vaddr) == 0ULL &&
            ld(before_guard_vaddr) == before_guards[round] &&
            ld(guard01_vaddr) == guard01_values[round] &&
            ld(guard12_vaddr) == guard12_values[round] &&
            ld(after_guard_vaddr) == after_guards[round] &&
            ld(marker_guard_vaddr) == marker_guards[round]
        );

        goto_priv(PRIV_M);
        TEST_ASSERT("backing memory after the four-generation zero-clean-zero-zero CMO probe keeps block0 and block2 zero, block1 seed payload, marker update, and all guards intact",
            ai_memblock_cmo_triple_block_all_zero(block0_paddr) &&
            ai_memblock_cmo_triple_block_matches_seed(block1_paddr,
                block1_seeds[round]) &&
            ai_memblock_cmo_triple_block_all_zero(block2_paddr) &&
            read64(before_guard_paddr) == before_guards[round] &&
            read64(guard01_paddr) == guard01_values[round] &&
            read64(guard12_paddr) == guard12_values[round] &&
            read64(after_guard_paddr) == after_guards[round] &&
            read64(marker_paddr) == marker_updates[round] &&
            read64(marker_guard_paddr) == marker_guards[round]
        );
    }

    ai_memblock_cmo_triple_reset_hs_env();

    TEST_END();
}

bool ai_micro_hs_interleaved_three_block_store_zero_store_cbom_store_inval_store_inval_four_generation_post_first_zero_corner()
{
    TEST_START();

    const enum test_page source_page = SWITCH1;
    const enum test_page marker_page = SWITCH2;
    const size_t block0_fourth_store_word = 0U;
    const size_t block0_older_store_word = 1U;
    const size_t block0_zero_load_word = 2U;
    const size_t block0_second_store_word = 4U;
    const size_t block0_third_store_word = 6U;
    const size_t block1_load_word = 3U;
    const size_t block2_load_word = 5U;
    const uintptr_t block0_vaddr = hs_page_base(source_page) + 0x3940U;
    const uintptr_t block1_vaddr = hs_page_base(source_page) + 0x39c0U;
    const uintptr_t block2_vaddr = hs_page_base(source_page) + 0x3a40U;
    const uintptr_t block0_paddr = phys_page_base(source_page) + 0x3940U;
    const uintptr_t block1_paddr = phys_page_base(source_page) + 0x39c0U;
    const uintptr_t block2_paddr = phys_page_base(source_page) + 0x3a40U;
    const uintptr_t block0_fourth_store_vaddr =
        block0_vaddr + block0_fourth_store_word * sizeof(uint64_t);
    const uintptr_t block0_older_store_vaddr =
        block0_vaddr + block0_older_store_word * sizeof(uint64_t);
    const uintptr_t block0_zero_load_vaddr =
        block0_vaddr + block0_zero_load_word * sizeof(uint64_t);
    const uintptr_t block0_second_store_vaddr =
        block0_vaddr + block0_second_store_word * sizeof(uint64_t);
    const uintptr_t block0_third_store_vaddr =
        block0_vaddr + block0_third_store_word * sizeof(uint64_t);
    const uintptr_t block1_load_vaddr =
        block1_vaddr + block1_load_word * sizeof(uint64_t);
    const uintptr_t block2_load_vaddr =
        block2_vaddr + block2_load_word * sizeof(uint64_t);
    const uintptr_t before_guard_vaddr = block0_vaddr - sizeof(uint64_t);
    const uintptr_t guard01_vaddr =
        block0_vaddr +
        AI_MEMBLOCK_CMO_TRIPLE_BLOCK_WORDS * sizeof(uint64_t);
    const uintptr_t guard12_vaddr =
        block1_vaddr +
        AI_MEMBLOCK_CMO_TRIPLE_BLOCK_WORDS * sizeof(uint64_t);
    const uintptr_t after_guard_vaddr =
        block2_vaddr +
        AI_MEMBLOCK_CMO_TRIPLE_BLOCK_WORDS * sizeof(uint64_t);
    const uintptr_t before_guard_paddr = block0_paddr - sizeof(uint64_t);
    const uintptr_t guard01_paddr =
        block0_paddr +
        AI_MEMBLOCK_CMO_TRIPLE_BLOCK_WORDS * sizeof(uint64_t);
    const uintptr_t guard12_paddr =
        block1_paddr +
        AI_MEMBLOCK_CMO_TRIPLE_BLOCK_WORDS * sizeof(uint64_t);
    const uintptr_t after_guard_paddr =
        block2_paddr +
        AI_MEMBLOCK_CMO_TRIPLE_BLOCK_WORDS * sizeof(uint64_t);
    const uintptr_t marker_vaddr = hs_page_base(marker_page) + 0x3940U;
    const uintptr_t marker_paddr = phys_page_base(marker_page) + 0x3940U;
    const uintptr_t marker_guard_vaddr = marker_vaddr + sizeof(uint64_t);
    const uintptr_t marker_guard_paddr = marker_paddr + sizeof(uint64_t);
    const uint64_t block0_seeds[AI_MEMBLOCK_CMO_TRIPLE_ROUNDS] = {
        0x3940500060007000ULL,
        0x4a51611171118111ULL,
        0x5b62722282229222ULL,
    };
    const uint64_t block1_seeds[AI_MEMBLOCK_CMO_TRIPLE_ROUNDS] = {
        0x80706050394039c0ULL,
        0x918171614a514a91ULL,
        0xa29282725b625ba2ULL,
    };
    const uint64_t block2_seeds[AI_MEMBLOCK_CMO_TRIPLE_ROUNDS] = {
        0x2468ace039403a40ULL,
        0x3579bdf14a514b11ULL,
        0x468ace025b625c22ULL,
    };
    const uint64_t block0_older_store_values[AI_MEMBLOCK_CMO_TRIPLE_ROUNDS] = {
        0x1100394022003300ULL,
        0x22114a5133114411ULL,
        0x33225b6244225522ULL,
    };
    const uint64_t block0_second_store_values[AI_MEMBLOCK_CMO_TRIPLE_ROUNDS] = {
        0x0033002200394011ULL,
        0x11441133114a5122ULL,
        0x22552244225b6233ULL,
    };
    const uint64_t block0_third_store_values[AI_MEMBLOCK_CMO_TRIPLE_ROUNDS] = {
        0xcafe3940feed3940ULL,
        0xdb0f4a510ffe4a51ULL,
        0xec205b62200f5b62ULL,
    };
    const uint64_t block0_fourth_store_values[AI_MEMBLOCK_CMO_TRIPLE_ROUNDS] = {
        0xc0013940deadbeefULL,
        0xd1124a51efbedf00ULL,
        0xe2235b62f0cfe011ULL,
    };
    const uint64_t before_guards[AI_MEMBLOCK_CMO_TRIPLE_ROUNDS] = {
        0xc0013940d00d3940ULL,
        0xd1124a51e11e4a51ULL,
        0xe2235b62f22f5b62ULL,
    };
    const uint64_t guard01_values[AI_MEMBLOCK_CMO_TRIPLE_ROUNDS] = {
        0x3940d00d3940c001ULL,
        0x4a51e11e4a51d112ULL,
        0x5b62f22f5b62e223ULL,
    };
    const uint64_t guard12_values[AI_MEMBLOCK_CMO_TRIPLE_ROUNDS] = {
        0x31415926394039c0ULL,
        0x271828184a514a91ULL,
        0x161803395b625ba2ULL,
    };
    const uint64_t after_guards[AI_MEMBLOCK_CMO_TRIPLE_ROUNDS] = {
        0x0839403a40962695ULL,
        0x194a514b11818282ULL,
        0x2a5b625c22930381ULL,
    };
    const uint64_t marker_seeds[AI_MEMBLOCK_CMO_TRIPLE_ROUNDS] = {
        0x89abcdef01234567ULL,
        0x9abcdef012345678ULL,
        0xabcdef0123456789ULL,
    };
    const uint64_t marker_updates[AI_MEMBLOCK_CMO_TRIPLE_ROUNDS] = {
        0x76543210fedcba98ULL,
        0x876543210fedcba9ULL,
        0x9876543210fedcbaULL,
    };
    const uint64_t marker_guards[AI_MEMBLOCK_CMO_TRIPLE_ROUNDS] = {
        0x55aa55aa39403940ULL,
        0x66bb66bb4a514a51ULL,
        0x77cc77cc5b625b62ULL,
    };

    ai_memblock_cmo_triple_prepare_hs_env();

    for (size_t round = 0; round < AI_MEMBLOCK_CMO_TRIPLE_ROUNDS; ++round) {
        uint64_t block0_older_observed = 0ULL;
        uint64_t block0_second_observed = 0ULL;
        uint64_t block0_third_observed = 0ULL;
        uint64_t block0_fourth_observed = 0ULL;
        uint64_t block0_zero_observed = 0ULL;
        uint64_t block1_observed = 0ULL;
        uint64_t block2_observed = 0ULL;
        uint64_t marker_observed = 0ULL;
        const uint64_t expected_block0_fourth_before =
            block0_seeds[round] +
            block0_fourth_store_word * 0x1111111111111111ULL;
        const uint64_t expected_block0_older_before =
            block0_seeds[round] +
            block0_older_store_word * 0x1111111111111111ULL;
        const uint64_t expected_block0_zero_before =
            block0_seeds[round] +
            block0_zero_load_word * 0x1111111111111111ULL;
        const uint64_t expected_block0_second_before =
            block0_seeds[round] +
            block0_second_store_word * 0x1111111111111111ULL;
        const uint64_t expected_block0_third_before =
            block0_seeds[round] +
            block0_third_store_word * 0x1111111111111111ULL;
        const uint64_t expected_block1_load =
            block1_seeds[round] +
            block1_load_word * 0x1111111111111111ULL;
        const uint64_t expected_block2_load =
            block2_seeds[round] +
            block2_load_word * 0x1111111111111111ULL;

        goto_priv(PRIV_M);
        ai_memblock_cmo_triple_seed_block(block0_paddr,
            block0_seeds[round]);
        ai_memblock_cmo_triple_seed_block(block1_paddr,
            block1_seeds[round]);
        ai_memblock_cmo_triple_seed_block(block2_paddr,
            block2_seeds[round]);
        write64(before_guard_paddr, before_guards[round]);
        write64(guard01_paddr, guard01_values[round]);
        write64(guard12_paddr, guard12_values[round]);
        write64(after_guard_paddr, after_guards[round]);
        write64(marker_paddr, marker_seeds[round]);
        write64(marker_guard_paddr, marker_guards[round]);

        goto_priv(PRIV_HS);
        TEST_SETUP_EXCEPT();
        AI_ASSERT_NO_EXCEPT_AND("before the four-generation zero-clean-inval-inval CMO probe starts, all source blocks, guards, and marker are readable",
            ld(block0_vaddr) == expected_block0_fourth_before &&
            ld(block0_older_store_vaddr) == expected_block0_older_before &&
            ld(block0_zero_load_vaddr) == expected_block0_zero_before &&
            ld(block0_second_store_vaddr) == expected_block0_second_before &&
            ld(block0_third_store_vaddr) == expected_block0_third_before &&
            ld(block1_vaddr) == block1_seeds[round] &&
            ld(block1_load_vaddr) == expected_block1_load &&
            ld(block2_vaddr) == block2_seeds[round] &&
            ld(block2_load_vaddr) == expected_block2_load &&
            ld(marker_vaddr) == marker_seeds[round]
        );

        TEST_SETUP_EXCEPT();
        ai_memblock_cmo_triple_three_block_store_zero_store_cbom_store_inval_store_inval_window(
            block0_vaddr, block1_vaddr, block2_vaddr, marker_vaddr,
            block0_older_store_values[round],
            block0_second_store_values[round],
            block0_third_store_values[round],
            block0_fourth_store_values[round], marker_updates[round],
            &block0_older_observed, &block0_second_observed,
            &block0_third_observed, &block0_fourth_observed,
            &block0_zero_observed, &block1_observed, &block2_observed,
            &marker_observed);
        AI_ASSERT_NO_EXCEPT_AND("under three-block CMO pressure, CBO.ZERO/CBOM/CBO.INVAL/CBO.INVAL keeps all post-zero store generations visible",
            block0_older_observed == 0ULL &&
            block0_second_observed == block0_second_store_values[round] &&
            block0_third_observed == block0_third_store_values[round] &&
            block0_fourth_observed == block0_fourth_store_values[round] &&
            block0_zero_observed == 0ULL &&
            block1_observed == expected_block1_load &&
            block2_observed == 0ULL &&
            marker_observed == marker_updates[round]
        );

        TEST_SETUP_EXCEPT();
        AI_ASSERT_NO_EXCEPT_AND("after the four-generation zero-clean-inval-inval CMO probe, block0 keeps only post-first-zero stores, block1 preserves seed, block2 is zero, and guards remain intact",
            ld(block0_fourth_store_vaddr) ==
                block0_fourth_store_values[round] &&
            ld(block0_older_store_vaddr) == 0ULL &&
            ld(block0_zero_load_vaddr) == 0ULL &&
            ld(block0_second_store_vaddr) ==
                block0_second_store_values[round] &&
            ld(block0_third_store_vaddr) ==
                block0_third_store_values[round] &&
            ld(block1_vaddr) == block1_seeds[round] &&
            ld(block1_load_vaddr) == expected_block1_load &&
            ld(block2_vaddr) == 0ULL &&
            ld(block2_load_vaddr) == 0ULL &&
            ld(before_guard_vaddr) == before_guards[round] &&
            ld(guard01_vaddr) == guard01_values[round] &&
            ld(guard12_vaddr) == guard12_values[round] &&
            ld(after_guard_vaddr) == after_guards[round] &&
            ld(marker_guard_vaddr) == marker_guards[round]
        );

        goto_priv(PRIV_M);
        TEST_ASSERT("backing memory after the four-generation zero-clean-inval-inval CMO probe keeps block0 zero except the second, third, and fourth post-first-zero stores, block1 seed payload, block2 zero, marker update, and all guards intact",
            ai_memblock_cmo_triple_block_zero_except_three_words(
                block0_paddr, block0_fourth_store_word,
                block0_fourth_store_values[round],
                block0_second_store_word,
                block0_second_store_values[round],
                block0_third_store_word,
                block0_third_store_values[round]) &&
            ai_memblock_cmo_triple_block_matches_seed(block1_paddr,
                block1_seeds[round]) &&
            ai_memblock_cmo_triple_block_all_zero(block2_paddr) &&
            read64(before_guard_paddr) == before_guards[round] &&
            read64(guard01_paddr) == guard01_values[round] &&
            read64(guard12_paddr) == guard12_values[round] &&
            read64(after_guard_paddr) == after_guards[round] &&
            read64(marker_paddr) == marker_updates[round] &&
            read64(marker_guard_paddr) == marker_guards[round]
        );
    }

    ai_memblock_cmo_triple_reset_hs_env();

    TEST_END();
}
