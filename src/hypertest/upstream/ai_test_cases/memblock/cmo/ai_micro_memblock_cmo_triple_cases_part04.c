#include "ai_micro_memblock_cmo_triple_common.h"

bool ai_micro_hs_interleaved_three_block_store_zero_store_cbom_store_zero_store_inval_four_generation_late_zero_corner()
{
    TEST_START();

    const enum test_page source_page = VSRWX_GRWX;
    const enum test_page marker_page = SWITCH2;
    const size_t block0_fourth_store_word = 0U;
    const size_t block0_older_store_word = 1U;
    const size_t block0_zero_load_word = 2U;
    const size_t block0_second_store_word = 4U;
    const size_t block0_third_store_word = 6U;
    const size_t block1_load_word = 3U;
    const size_t block2_load_word = 5U;
    const uintptr_t block0_vaddr = hs_page_base(source_page) + 0xd80U;
    const uintptr_t block1_vaddr = hs_page_base(source_page) + 0xe00U;
    const uintptr_t block2_vaddr = hs_page_base(source_page) + 0xe80U;
    const uintptr_t block0_paddr = phys_page_base(source_page) + 0xd80U;
    const uintptr_t block1_paddr = phys_page_base(source_page) + 0xe00U;
    const uintptr_t block2_paddr = phys_page_base(source_page) + 0xe80U;
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
    const uintptr_t marker_vaddr = hs_page_base(marker_page) + 0xd80U;
    const uintptr_t marker_paddr = phys_page_base(marker_page) + 0xd80U;
    const uintptr_t marker_guard_vaddr = marker_vaddr + sizeof(uint64_t);
    const uintptr_t marker_guard_paddr = marker_paddr + sizeof(uint64_t);
    const uint64_t block0_seeds[AI_MEMBLOCK_CMO_TRIPLE_ROUNDS] = {
        0x415263748596a7b8ULL,
        0x5263748596a7b8c9ULL,
        0x63748596a7b8c9daULL,
    };
    const uint64_t block1_seeds[AI_MEMBLOCK_CMO_TRIPLE_ROUNDS] = {
        0xb8a7968574635241ULL,
        0xc9b8a79685746352ULL,
        0xdac9b8a796857463ULL,
    };
    const uint64_t block2_seeds[AI_MEMBLOCK_CMO_TRIPLE_ROUNDS] = {
        0x0ace13579bdf2468ULL,
        0x1bdf2468ace03579ULL,
        0x2ce03579bdf1468aULL,
    };
    const uint64_t block0_older_store_values[AI_MEMBLOCK_CMO_TRIPLE_ROUNDS] = {
        0x1122334455667788ULL,
        0x2233445566778899ULL,
        0x33445566778899aaULL,
    };
    const uint64_t block0_second_store_values[AI_MEMBLOCK_CMO_TRIPLE_ROUNDS] = {
        0x8877665544332211ULL,
        0x9988776655443322ULL,
        0xaa99887766554433ULL,
    };
    const uint64_t block0_third_store_values[AI_MEMBLOCK_CMO_TRIPLE_ROUNDS] = {
        0x0f1e2d3c4b5a6978ULL,
        0x102f3e4d5c6b7a89ULL,
        0x21403f5e6d7c8b9aULL,
    };
    const uint64_t block0_fourth_store_values[AI_MEMBLOCK_CMO_TRIPLE_ROUNDS] = {
        0x78695a4b3c2d1e0fULL,
        0x897a6b5c4d3e2f10ULL,
        0x9a8b7c6d5e3f4021ULL,
    };
    const uint64_t before_guards[AI_MEMBLOCK_CMO_TRIPLE_ROUNDS] = {
        0xdeadc0de01020304ULL,
        0xefbed1ef12131415ULL,
        0xf0cfe2f023242526ULL,
    };
    const uint64_t guard01_values[AI_MEMBLOCK_CMO_TRIPLE_ROUNDS] = {
        0x04030201dec0addeULL,
        0x15141312efd1beefULL,
        0x26252423f0e2cff0ULL,
    };
    const uint64_t guard12_values[AI_MEMBLOCK_CMO_TRIPLE_ROUNDS] = {
        0x3141592653589793ULL,
        0x2718281828459045ULL,
        0x1618033988749894ULL,
    };
    const uint64_t after_guards[AI_MEMBLOCK_CMO_TRIPLE_ROUNDS] = {
        0x9397585326594131ULL,
        0x4590452818281827ULL,
        0x9498879839038161ULL,
    };
    const uint64_t marker_seeds[AI_MEMBLOCK_CMO_TRIPLE_ROUNDS] = {
        0xabcdef0123456789ULL,
        0xbcdef0123456789aULL,
        0xcdef0123456789abULL,
    };
    const uint64_t marker_updates[AI_MEMBLOCK_CMO_TRIPLE_ROUNDS] = {
        0x9876543210fedcbaULL,
        0xa9876543210fedcbULL,
        0xba9876543210fedcULL,
    };
    const uint64_t marker_guards[AI_MEMBLOCK_CMO_TRIPLE_ROUNDS] = {
        0x55aa00ff55aa00ffULL,
        0x66bb11ee66bb11eeULL,
        0x77cc22dd77cc22ddULL,
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
        AI_ASSERT_NO_EXCEPT_AND("before the four-generation zero-clean-zero-inval CMO probe starts, all source blocks, guards, and marker are readable",
            ld(block0_vaddr) == expected_block0_fourth_before &&
            ld(block0_older_store_vaddr) == expected_block0_older_before &&
            ld(block0_second_store_vaddr) == expected_block0_second_before &&
            ld(block0_third_store_vaddr) == expected_block0_third_before &&
            ld(block1_vaddr) == block1_seeds[round] &&
            ld(block1_load_vaddr) == expected_block1_load &&
            ld(block2_vaddr) == block2_seeds[round] &&
            ld(block2_load_vaddr) == expected_block2_load &&
            ld(marker_vaddr) == marker_seeds[round]
        );

        TEST_SETUP_EXCEPT();
        ai_memblock_cmo_triple_three_block_store_zero_store_cbom_store_zero_store_inval_window(
            block0_vaddr, block1_vaddr, block2_vaddr, marker_vaddr,
            block0_older_store_values[round],
            block0_second_store_values[round],
            block0_third_store_values[round],
            block0_fourth_store_values[round], marker_updates[round],
            &block0_older_observed, &block0_second_observed,
            &block0_third_observed, &block0_fourth_observed,
            &block0_zero_observed, &block1_observed, &block2_observed,
            &marker_observed);
        AI_ASSERT_NO_EXCEPT_AND("under three-block CMO pressure, the second CBO.ZERO clears older three generations while the fourth store survives final CBO.INVAL",
            block0_older_observed == 0ULL &&
            block0_second_observed == 0ULL &&
            block0_third_observed == 0ULL &&
            block0_fourth_observed == block0_fourth_store_values[round] &&
            block0_zero_observed == 0ULL &&
            block1_observed == expected_block1_load &&
            block2_observed == 0ULL &&
            marker_observed == marker_updates[round]
        );

        TEST_SETUP_EXCEPT();
        AI_ASSERT_NO_EXCEPT_AND("after the four-generation zero-clean-zero-inval CMO probe, block0 keeps only the fourth post-second-zero store, block1 preserves seed, block2 is zero, and guards remain intact",
            ld(block0_fourth_store_vaddr) ==
                block0_fourth_store_values[round] &&
            ld(block0_older_store_vaddr) == 0ULL &&
            ld(block0_second_store_vaddr) == 0ULL &&
            ld(block0_third_store_vaddr) == 0ULL &&
            ld(block0_zero_load_vaddr) == 0ULL &&
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
        TEST_ASSERT("backing memory after the four-generation zero-clean-zero-inval CMO probe keeps block0 zero except the fourth post-second-zero store, block1 seed payload, block2 zero, marker update, and all guards intact",
            ai_memblock_cmo_triple_block_zero_except_word(
                block0_paddr, block0_fourth_store_word,
                block0_fourth_store_values[round]) &&
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

bool ai_micro_hs_interleaved_three_block_store_inval_store_cbom_store_zero_store_inval_four_generation_late_zero_corner()
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
    const uintptr_t block0_vaddr = hs_page_base(source_page) + 0x3500U;
    const uintptr_t block1_vaddr = hs_page_base(source_page) + 0x3580U;
    const uintptr_t block2_vaddr = hs_page_base(source_page) + 0x3600U;
    const uintptr_t block0_paddr = phys_page_base(source_page) + 0x3500U;
    const uintptr_t block1_paddr = phys_page_base(source_page) + 0x3580U;
    const uintptr_t block2_paddr = phys_page_base(source_page) + 0x3600U;
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
    const uintptr_t marker_vaddr = hs_page_base(marker_page) + 0x3500U;
    const uintptr_t marker_paddr = phys_page_base(marker_page) + 0x3500U;
    const uintptr_t marker_guard_vaddr = marker_vaddr + sizeof(uint64_t);
    const uintptr_t marker_guard_paddr = marker_paddr + sizeof(uint64_t);
    const uint64_t block0_seeds[AI_MEMBLOCK_CMO_TRIPLE_ROUNDS] = {
        0x5a6b7c8d9eaf0011ULL,
        0x6b7c8d9eaf001122ULL,
        0x7c8d9eaf00112233ULL,
    };
    const uint64_t block1_seeds[AI_MEMBLOCK_CMO_TRIPLE_ROUNDS] = {
        0x1100af9e8d7c6b5aULL,
        0x221100af9e8d7c6bULL,
        0x33221100af9e8d7cULL,
    };
    const uint64_t block2_seeds[AI_MEMBLOCK_CMO_TRIPLE_ROUNDS] = {
        0x13579bdf02468aceULL,
        0x2468ace113579bdfULL,
        0x3579bdf22468ace1ULL,
    };
    const uint64_t block0_older_store_values[AI_MEMBLOCK_CMO_TRIPLE_ROUNDS] = {
        0x0badc0de12345678ULL,
        0x1cbed1ef23456789ULL,
        0x2dcfe2f03456789aULL,
    };
    const uint64_t block0_second_store_values[AI_MEMBLOCK_CMO_TRIPLE_ROUNDS] = {
        0x87654321ed0cdab0ULL,
        0x98765432fe1debc1ULL,
        0xa98765430f2efcd2ULL,
    };
    const uint64_t block0_third_store_values[AI_MEMBLOCK_CMO_TRIPLE_ROUNDS] = {
        0x0123456789abcdeeULL,
        0x123456789abcdef0ULL,
        0x23456789abcdef01ULL,
    };
    const uint64_t block0_fourth_store_values[AI_MEMBLOCK_CMO_TRIPLE_ROUNDS] = {
        0xeedcba9876543210ULL,
        0xffedcba987654321ULL,
        0x10fedcba98765432ULL,
    };
    const uint64_t before_guards[AI_MEMBLOCK_CMO_TRIPLE_ROUNDS] = {
        0xc001d00d11112222ULL,
        0xd112e11e22223333ULL,
        0xe223f22f33334444ULL,
    };
    const uint64_t guard01_values[AI_MEMBLOCK_CMO_TRIPLE_ROUNDS] = {
        0x22221111d00dc001ULL,
        0x33332222e11ed112ULL,
        0x44443333f22fe223ULL,
    };
    const uint64_t guard12_values[AI_MEMBLOCK_CMO_TRIPLE_ROUNDS] = {
        0x1020304050607080ULL,
        0x2131415161718191ULL,
        0x32425262728292a2ULL,
    };
    const uint64_t after_guards[AI_MEMBLOCK_CMO_TRIPLE_ROUNDS] = {
        0x8070605040302010ULL,
        0x9181716151413121ULL,
        0xa292827262524232ULL,
    };
    const uint64_t marker_seeds[AI_MEMBLOCK_CMO_TRIPLE_ROUNDS] = {
        0x0f0e0d0c0b0a0908ULL,
        0x1f1e1d1c1b1a1918ULL,
        0x2f2e2d2c2b2a2928ULL,
    };
    const uint64_t marker_updates[AI_MEMBLOCK_CMO_TRIPLE_ROUNDS] = {
        0x08090a0b0c0d0e0fULL,
        0x18191a1b1c1d1e1fULL,
        0x28292a2b2c2d2e2fULL,
    };
    const uint64_t marker_guards[AI_MEMBLOCK_CMO_TRIPLE_ROUNDS] = {
        0xaa5500ffaa5500ffULL,
        0xbb6611eebb6611eeULL,
        0xcc7722ddcc7722ddULL,
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
        AI_ASSERT_NO_EXCEPT_AND("before the four-generation inval-clean-zero-inval CMO probe starts, all source blocks, guards, and marker are readable",
            ld(block0_vaddr) == expected_block0_fourth_before &&
            ld(block0_older_store_vaddr) == expected_block0_older_before &&
            ld(block0_second_store_vaddr) == expected_block0_second_before &&
            ld(block0_third_store_vaddr) == expected_block0_third_before &&
            ld(block1_vaddr) == block1_seeds[round] &&
            ld(block2_vaddr) == block2_seeds[round] &&
            ld(block2_load_vaddr) == expected_block2_load &&
            ld(marker_vaddr) == marker_seeds[round]
        );

        TEST_SETUP_EXCEPT();
        ai_memblock_cmo_triple_three_block_store_inval_store_cbom_store_zero_store_inval_window(
            block0_vaddr, block1_vaddr, block2_vaddr, marker_vaddr,
            block0_older_store_values[round],
            block0_second_store_values[round],
            block0_third_store_values[round],
            block0_fourth_store_values[round], marker_updates[round],
            &block0_older_observed, &block0_second_observed,
            &block0_third_observed, &block0_fourth_observed,
            &block0_zero_observed, &block1_observed, &block2_observed,
            &marker_observed);
        AI_ASSERT_NO_EXCEPT_AND("under three-block CMO pressure, late CBO.ZERO clears older three generations while the fourth store survives final CBO.INVAL",
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
        AI_ASSERT_NO_EXCEPT_AND("after the four-generation inval-clean-zero-inval CMO probe, block0 keeps only the fourth post-zero store, block1 is zero, block2 preserves seed, and guards remain intact",
            ld(block0_fourth_store_vaddr) ==
                block0_fourth_store_values[round] &&
            ld(block0_older_store_vaddr) == 0ULL &&
            ld(block0_second_store_vaddr) == 0ULL &&
            ld(block0_third_store_vaddr) == 0ULL &&
            ld(block0_zero_load_vaddr) == 0ULL &&
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
        TEST_ASSERT("backing memory after the four-generation inval-clean-zero-inval CMO probe keeps block0 zero except the fourth post-zero store, block1 zero, block2 seed payload, marker update, and all guards intact",
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

bool ai_micro_hs_interleaved_three_block_store_zero_store_cbom_store_inval_store_cbom_four_generation_post_zero_corner()
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
    const uintptr_t block0_vaddr = hs_page_base(source_page) + 0x3c80U;
    const uintptr_t block1_vaddr = hs_page_base(source_page) + 0x3d00U;
    const uintptr_t block2_vaddr = hs_page_base(source_page) + 0x3d80U;
    const uintptr_t block0_paddr = phys_page_base(source_page) + 0x3c80U;
    const uintptr_t block1_paddr = phys_page_base(source_page) + 0x3d00U;
    const uintptr_t block2_paddr = phys_page_base(source_page) + 0x3d80U;
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
    const uintptr_t marker_vaddr = hs_page_base(marker_page) + 0x3c80U;
    const uintptr_t marker_paddr = phys_page_base(marker_page) + 0x3c80U;
    const uintptr_t marker_guard_vaddr = marker_vaddr + sizeof(uint64_t);
    const uintptr_t marker_guard_paddr = marker_paddr + sizeof(uint64_t);
    const uint64_t block0_seeds[AI_MEMBLOCK_CMO_TRIPLE_ROUNDS] = {
        0x13572468ace0bdf1ULL,
        0x24683579bdf1ce02ULL,
        0x3579468ace02df13ULL,
    };
    const uint64_t block1_seeds[AI_MEMBLOCK_CMO_TRIPLE_ROUNDS] = {
        0xf1bde0ac68245713ULL,
        0x02cef1bd79356824ULL,
        0x13df02ce8a467935ULL,
    };
    const uint64_t block2_seeds[AI_MEMBLOCK_CMO_TRIPLE_ROUNDS] = {
        0x0badc0ffee123456ULL,
        0x1cbed100ff234567ULL,
        0x2dcfe21100345678ULL,
    };
    const uint64_t block0_older_store_values[AI_MEMBLOCK_CMO_TRIPLE_ROUNDS] = {
        0xa1a2a3a4a5a6a7a8ULL,
        0xb1b2b3b4b5b6b7b8ULL,
        0xc1c2c3c4c5c6c7c8ULL,
    };
    const uint64_t block0_second_store_values[AI_MEMBLOCK_CMO_TRIPLE_ROUNDS] = {
        0x1827364554637281ULL,
        0x2938475665748392ULL,
        0x3a495867768594a3ULL,
    };
    const uint64_t block0_third_store_values[AI_MEMBLOCK_CMO_TRIPLE_ROUNDS] = {
        0x81f0e1d2c3b4a596ULL,
        0x92f1e2d3c4b5a6a7ULL,
        0xa3f2e3d4c5b6a7b8ULL,
    };
    const uint64_t block0_fourth_store_values[AI_MEMBLOCK_CMO_TRIPLE_ROUNDS] = {
        0x76540123fedcba98ULL,
        0x87651234efcdab89ULL,
        0x98762345debc9a7aULL,
    };
    const uint64_t before_guards[AI_MEMBLOCK_CMO_TRIPLE_ROUNDS] = {
        0x1029384756abcdefULL,
        0x213a495867bcdef0ULL,
        0x324b5a6978cdef01ULL,
    };
    const uint64_t guard01_values[AI_MEMBLOCK_CMO_TRIPLE_ROUNDS] = {
        0xefcdab5647382910ULL,
        0xf0debc6758493a21ULL,
        0x01efcd78695a4b32ULL,
    };
    const uint64_t guard12_values[AI_MEMBLOCK_CMO_TRIPLE_ROUNDS] = {
        0x55aa55aa10203040ULL,
        0x66bb66bb21314151ULL,
        0x77cc77cc32425262ULL,
    };
    const uint64_t after_guards[AI_MEMBLOCK_CMO_TRIPLE_ROUNDS] = {
        0x40302010aa55aa55ULL,
        0x51413121bb66bb66ULL,
        0x62524232cc77cc77ULL,
    };
    const uint64_t marker_seeds[AI_MEMBLOCK_CMO_TRIPLE_ROUNDS] = {
        0x0ff1ce5eed0dd001ULL,
        0x1002df6ffe1ee112ULL,
        0x2113e0800f2ff223ULL,
    };
    const uint64_t marker_updates[AI_MEMBLOCK_CMO_TRIPLE_ROUNDS] = {
        0x01d00ded5ecef10fULL,
        0x12e11efe6fdf0210ULL,
        0x23f22f0f800e1321ULL,
    };
    const uint64_t marker_guards[AI_MEMBLOCK_CMO_TRIPLE_ROUNDS] = {
        0xcafef00d13579bdfULL,
        0xdbff011e2468ace0ULL,
        0xed00122f3579bdf1ULL,
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
        AI_ASSERT_NO_EXCEPT_AND("before the four-generation zero-clean-inval-clean CMO probe starts, all source blocks, guards, and marker are readable",
            ld(block0_vaddr) == expected_block0_fourth_before &&
            ld(block0_older_store_vaddr) == expected_block0_older_before &&
            ld(block0_second_store_vaddr) == expected_block0_second_before &&
            ld(block0_third_store_vaddr) == expected_block0_third_before &&
            ld(block1_vaddr) == block1_seeds[round] &&
            ld(block1_load_vaddr) == expected_block1_load &&
            ld(block2_vaddr) == block2_seeds[round] &&
            ld(block2_load_vaddr) == expected_block2_load &&
            ld(marker_vaddr) == marker_seeds[round]
        );

        TEST_SETUP_EXCEPT();
        ai_memblock_cmo_triple_three_block_store_zero_store_cbom_store_inval_store_cbom_window(
            block0_vaddr, block1_vaddr, block2_vaddr, marker_vaddr,
            block0_older_store_values[round],
            block0_second_store_values[round],
            block0_third_store_values[round],
            block0_fourth_store_values[round], marker_updates[round],
            &block0_older_observed, &block0_second_observed,
            &block0_third_observed, &block0_fourth_observed,
            &block0_zero_observed, &block1_observed, &block2_observed,
            &marker_observed);
        AI_ASSERT_NO_EXCEPT_AND("under three-block CMO pressure, first CBO.ZERO clears only the older generation while three post-zero stores survive CBOM, CBO.INVAL, and final CBOM",
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
        AI_ASSERT_NO_EXCEPT_AND("after the four-generation zero-clean-inval-clean CMO probe, block0 keeps only post-zero stores, block1 preserves seed, block2 is zero, and guards remain intact",
            ld(block0_fourth_store_vaddr) ==
                block0_fourth_store_values[round] &&
            ld(block0_older_store_vaddr) == 0ULL &&
            ld(block0_second_store_vaddr) ==
                block0_second_store_values[round] &&
            ld(block0_third_store_vaddr) ==
                block0_third_store_values[round] &&
            ld(block0_zero_load_vaddr) == 0ULL &&
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
        TEST_ASSERT("backing memory after the four-generation zero-clean-inval-clean CMO probe keeps block0 zero except three post-zero stores, block1 seed payload, block2 zero, marker update, and all guards intact",
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

bool ai_micro_hs_interleaved_three_block_store_inval_store_cbom_store_zero_store_cbom_four_generation_late_zero_corner()
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
    const uintptr_t block0_vaddr = hs_page_base(source_page) + 0x3e00U;
    const uintptr_t block1_vaddr = hs_page_base(source_page) + 0x3e80U;
    const uintptr_t block2_vaddr = hs_page_base(source_page) + 0x3f00U;
    const uintptr_t block0_paddr = phys_page_base(source_page) + 0x3e00U;
    const uintptr_t block1_paddr = phys_page_base(source_page) + 0x3e80U;
    const uintptr_t block2_paddr = phys_page_base(source_page) + 0x3f00U;
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
    const uintptr_t marker_vaddr = hs_page_base(marker_page) + 0x3e00U;
    const uintptr_t marker_paddr = phys_page_base(marker_page) + 0x3e00U;
    const uintptr_t marker_guard_vaddr = marker_vaddr + sizeof(uint64_t);
    const uintptr_t marker_guard_paddr = marker_paddr + sizeof(uint64_t);
    const uint64_t block0_seeds[AI_MEMBLOCK_CMO_TRIPLE_ROUNDS] = {
        0x5a6b7c8d9eaf1021ULL,
        0x6b7c8d9eaf102132ULL,
        0x7c8d9eaf10213243ULL,
    };
    const uint64_t block1_seeds[AI_MEMBLOCK_CMO_TRIPLE_ROUNDS] = {
        0x2110af9e8d7c6b5aULL,
        0x322110af9e8d7c6bULL,
        0x43322110af9e8d7cULL,
    };
    const uint64_t block2_seeds[AI_MEMBLOCK_CMO_TRIPLE_ROUNDS] = {
        0x0a1c2e4052647688ULL,
        0x1b2d3f5163758799ULL,
        0x2c3e5062748698aaULL,
    };
    const uint64_t block0_older_store_values[AI_MEMBLOCK_CMO_TRIPLE_ROUNDS] = {
        0x01030507090b0d0fULL,
        0x11131517191b1d1fULL,
        0x21232527292b2d2fULL,
    };
    const uint64_t block0_second_store_values[AI_MEMBLOCK_CMO_TRIPLE_ROUNDS] = {
        0xf0e0d0c0b0a09080ULL,
        0xe1d1c1b1a1918171ULL,
        0xd2c2b2a292827262ULL,
    };
    const uint64_t block0_third_store_values[AI_MEMBLOCK_CMO_TRIPLE_ROUNDS] = {
        0xa0b1c2d3e4f50617ULL,
        0xb1c2d3e4f5061728ULL,
        0xc2d3e4f506172839ULL,
    };
    const uint64_t block0_fourth_store_values[AI_MEMBLOCK_CMO_TRIPLE_ROUNDS] = {
        0x71605f4e3d2c1b0aULL,
        0x8271605f4e3d2c1bULL,
        0x938271605f4e3d2cULL,
    };
    const uint64_t before_guards[AI_MEMBLOCK_CMO_TRIPLE_ROUNDS] = {
        0x0123456789abcdefULL,
        0x123456789abcdef0ULL,
        0x23456789abcdef01ULL,
    };
    const uint64_t guard01_values[AI_MEMBLOCK_CMO_TRIPLE_ROUNDS] = {
        0xfedcba9876543210ULL,
        0xefcdab8967452301ULL,
        0xdebc9a7856341202ULL,
    };
    const uint64_t guard12_values[AI_MEMBLOCK_CMO_TRIPLE_ROUNDS] = {
        0x3141592653589793ULL,
        0x2718281828459045ULL,
        0x1618033988749894ULL,
    };
    const uint64_t after_guards[AI_MEMBLOCK_CMO_TRIPLE_ROUNDS] = {
        0x9397585326594131ULL,
        0x4590452818281827ULL,
        0x9498879839038161ULL,
    };
    const uint64_t marker_seeds[AI_MEMBLOCK_CMO_TRIPLE_ROUNDS] = {
        0xabcd1234ef005566ULL,
        0xbcde2345f0116677ULL,
        0xcdef345601227788ULL,
    };
    const uint64_t marker_updates[AI_MEMBLOCK_CMO_TRIPLE_ROUNDS] = {
        0x665500ef3412cdabULL,
        0x776611f04523debcULL,
        0x887722016534efcdULL,
    };
    const uint64_t marker_guards[AI_MEMBLOCK_CMO_TRIPLE_ROUNDS] = {
        0x0f0e0d0c0b0a0908ULL,
        0x1f1e1d1c1b1a1918ULL,
        0x2f2e2d2c2b2a2928ULL,
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
        AI_ASSERT_NO_EXCEPT_AND("before the four-generation inval-clean-zero-clean CMO probe starts, all source blocks, guards, and marker are readable",
            ld(block0_vaddr) == expected_block0_fourth_before &&
            ld(block0_older_store_vaddr) == expected_block0_older_before &&
            ld(block0_second_store_vaddr) == expected_block0_second_before &&
            ld(block0_third_store_vaddr) == expected_block0_third_before &&
            ld(block1_vaddr) == block1_seeds[round] &&
            ld(block2_vaddr) == block2_seeds[round] &&
            ld(block2_load_vaddr) == expected_block2_load &&
            ld(marker_vaddr) == marker_seeds[round]
        );

        TEST_SETUP_EXCEPT();
        ai_memblock_cmo_triple_three_block_store_inval_store_cbom_store_zero_store_cbom_window(
            block0_vaddr, block1_vaddr, block2_vaddr, marker_vaddr,
            block0_older_store_values[round],
            block0_second_store_values[round],
            block0_third_store_values[round],
            block0_fourth_store_values[round], marker_updates[round],
            &block0_older_observed, &block0_second_observed,
            &block0_third_observed, &block0_fourth_observed,
            &block0_zero_observed, &block1_observed, &block2_observed,
            &marker_observed);
        AI_ASSERT_NO_EXCEPT_AND("under three-block CMO pressure, late CBO.ZERO clears older three generations while the fourth store survives final CBOM after CBO.INVAL and CBOM",
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
        AI_ASSERT_NO_EXCEPT_AND("after the four-generation inval-clean-zero-clean CMO probe, block0 keeps only the fourth post-zero store, block1 is zero, block2 preserves seed, and guards remain intact",
            ld(block0_fourth_store_vaddr) ==
                block0_fourth_store_values[round] &&
            ld(block0_older_store_vaddr) == 0ULL &&
            ld(block0_second_store_vaddr) == 0ULL &&
            ld(block0_third_store_vaddr) == 0ULL &&
            ld(block0_zero_load_vaddr) == 0ULL &&
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
        TEST_ASSERT("backing memory after the four-generation inval-clean-zero-clean CMO probe keeps block0 zero except the fourth post-zero store, block1 zero, block2 seed payload, marker update, and all guards intact",
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

bool ai_micro_hs_interleaved_three_block_store_cbom_store_zero_store_inval_store_zero_four_generation_final_zero_corner()
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
    const uintptr_t block0_vaddr = hs_page_base(source_page) + 0x3e00U;
    const uintptr_t block1_vaddr = hs_page_base(source_page) + 0x3e80U;
    const uintptr_t block2_vaddr = hs_page_base(source_page) + 0x3f00U;
    const uintptr_t block0_paddr = phys_page_base(source_page) + 0x3e00U;
    const uintptr_t block1_paddr = phys_page_base(source_page) + 0x3e80U;
    const uintptr_t block2_paddr = phys_page_base(source_page) + 0x3f00U;
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
    const uintptr_t marker_vaddr = hs_page_base(marker_page) + 0x3e00U;
    const uintptr_t marker_paddr = phys_page_base(marker_page) + 0x3e00U;
    const uintptr_t marker_guard_vaddr = marker_vaddr + sizeof(uint64_t);
    const uintptr_t marker_guard_paddr = marker_paddr + sizeof(uint64_t);
    const uint64_t block0_seeds[AI_MEMBLOCK_CMO_TRIPLE_ROUNDS] = {
        0x1928374655aabbccULL,
        0x2a39485766bbccddULL,
        0x3b4a596877ccddeeULL,
    };
    const uint64_t block1_seeds[AI_MEMBLOCK_CMO_TRIPLE_ROUNDS] = {
        0xccbbaa5546372819ULL,
        0xddccbb665748392aULL,
        0xeeddcc7768594a3bULL,
    };
    const uint64_t block2_seeds[AI_MEMBLOCK_CMO_TRIPLE_ROUNDS] = {
        0x0badc0de10293847ULL,
        0x1cbed1ef213a4958ULL,
        0x2dcfe200324b5a69ULL,
    };
    const uint64_t block0_older_store_values[AI_MEMBLOCK_CMO_TRIPLE_ROUNDS] = {
        0x13579bdf2468ace0ULL,
        0x2468ace03579bdf1ULL,
        0x3579bdf1468ace02ULL,
    };
    const uint64_t block0_second_store_values[AI_MEMBLOCK_CMO_TRIPLE_ROUNDS] = {
        0xfdb97531eca86420ULL,
        0x0eca8642fdb97531ULL,
        0x1fdb97530eca8642ULL,
    };
    const uint64_t block0_third_store_values[AI_MEMBLOCK_CMO_TRIPLE_ROUNDS] = {
        0x55aa33cc11ee77ddULL,
        0x66bb44dd22ff88eeULL,
        0x77cc55ee330099ffULL,
    };
    const uint64_t block0_fourth_store_values[AI_MEMBLOCK_CMO_TRIPLE_ROUNDS] = {
        0xdd77ee11cc33aa55ULL,
        0xee88ff22dd44bb66ULL,
        0xff990033ee55cc77ULL,
    };
    const uint64_t before_guards[AI_MEMBLOCK_CMO_TRIPLE_ROUNDS] = {
        0xa1b2c3d4e5f60718ULL,
        0xb2c3d4e5f6071829ULL,
        0xc3d4e5f60718293aULL,
    };
    const uint64_t guard01_values[AI_MEMBLOCK_CMO_TRIPLE_ROUNDS] = {
        0x8170605f4e3d2c1bULL,
        0x928171605f4e3d2cULL,
        0xa3928171605f4e3dULL,
    };
    const uint64_t guard12_values[AI_MEMBLOCK_CMO_TRIPLE_ROUNDS] = {
        0x1123581321345589ULL,
        0x223469243245669aULL,
        0x33457a35335677abULL,
    };
    const uint64_t after_guards[AI_MEMBLOCK_CMO_TRIPLE_ROUNDS] = {
        0x8955342113582311ULL,
        0x9a66453224693422ULL,
        0xab775643357a4533ULL,
    };
    const uint64_t marker_seeds[AI_MEMBLOCK_CMO_TRIPLE_ROUNDS] = {
        0x0ace0ace12345678ULL,
        0x1bdf1bdf23456789ULL,
        0x2cf02cf03456789aULL,
    };
    const uint64_t marker_updates[AI_MEMBLOCK_CMO_TRIPLE_ROUNDS] = {
        0x78563412ce0ace0aULL,
        0x89674523df1bdf1bULL,
        0x9a785634f02cf02cULL,
    };
    const uint64_t marker_guards[AI_MEMBLOCK_CMO_TRIPLE_ROUNDS] = {
        0xf0e1d2c3b4a59687ULL,
        0xe1d2c3b4a5968778ULL,
        0xd2c3b4a596877869ULL,
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
        const uint64_t expected_block0_second_before =
            block0_seeds[round] +
            block0_second_store_word * 0x1111111111111111ULL;
        const uint64_t expected_block0_third_before =
            block0_seeds[round] +
            block0_third_store_word * 0x1111111111111111ULL;
        const uint64_t expected_block1_load =
            block1_seeds[round] +
            block1_load_word * 0x1111111111111111ULL;

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
        AI_ASSERT_NO_EXCEPT_AND("before the four-generation clean-zero-inval-zero CMO probe starts, all source blocks, guards, and marker are readable",
            ld(block0_vaddr) == expected_block0_fourth_before &&
            ld(block0_older_store_vaddr) == expected_block0_older_before &&
            ld(block0_second_store_vaddr) == expected_block0_second_before &&
            ld(block0_third_store_vaddr) == expected_block0_third_before &&
            ld(block1_vaddr) == block1_seeds[round] &&
            ld(block1_load_vaddr) == expected_block1_load &&
            ld(block2_vaddr) == block2_seeds[round] &&
            ld(marker_vaddr) == marker_seeds[round]
        );

        TEST_SETUP_EXCEPT();
        ai_memblock_cmo_triple_three_block_store_cbom_store_zero_store_inval_store_zero_window(
            block0_vaddr, block1_vaddr, block2_vaddr, marker_vaddr,
            block0_older_store_values[round],
            block0_second_store_values[round],
            block0_third_store_values[round],
            block0_fourth_store_values[round], marker_updates[round],
            &block0_older_observed, &block0_second_observed,
            &block0_third_observed, &block0_fourth_observed,
            &block0_zero_observed, &block1_observed, &block2_observed,
            &marker_observed);
        AI_ASSERT_NO_EXCEPT_AND("under three-block CMO pressure, final CBO.ZERO clears all generations after prior CBOM, middle CBO.ZERO, and CBO.INVAL",
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
        AI_ASSERT_NO_EXCEPT_AND("after the four-generation clean-zero-inval-zero CMO probe, block0 is fully zero, block1 preserves seed, block2 is zero, and guards remain intact",
            ld(block0_fourth_store_vaddr) == 0ULL &&
            ld(block0_older_store_vaddr) == 0ULL &&
            ld(block0_second_store_vaddr) == 0ULL &&
            ld(block0_third_store_vaddr) == 0ULL &&
            ld(block0_zero_load_vaddr) == 0ULL &&
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
        TEST_ASSERT("backing memory after the four-generation clean-zero-inval-zero CMO probe keeps block0 and block2 fully zero, block1 seed payload, marker update, and all guards intact",
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

bool ai_micro_hs_interleaved_three_block_store_cbom_store_inval_store_zero_store_inval_four_generation_late_zero_corner()
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
    const uintptr_t block0_vaddr = hs_page_base(source_page) + 0x3e00U;
    const uintptr_t block1_vaddr = hs_page_base(source_page) + 0x3e80U;
    const uintptr_t block2_vaddr = hs_page_base(source_page) + 0x3f00U;
    const uintptr_t block0_paddr = phys_page_base(source_page) + 0x3e00U;
    const uintptr_t block1_paddr = phys_page_base(source_page) + 0x3e80U;
    const uintptr_t block2_paddr = phys_page_base(source_page) + 0x3f00U;
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
    const uintptr_t marker_vaddr = hs_page_base(marker_page) + 0x3e00U;
    const uintptr_t marker_paddr = phys_page_base(marker_page) + 0x3e00U;
    const uintptr_t marker_guard_vaddr = marker_vaddr + sizeof(uint64_t);
    const uintptr_t marker_guard_paddr = marker_paddr + sizeof(uint64_t);
    const uint64_t block0_seeds[AI_MEMBLOCK_CMO_TRIPLE_ROUNDS] = {
        0x456789abcdef0123ULL,
        0x56789abcdef01234ULL,
        0x6789abcdef012345ULL,
    };
    const uint64_t block1_seeds[AI_MEMBLOCK_CMO_TRIPLE_ROUNDS] = {
        0x2301efcdab896745ULL,
        0x3412f0debc9a7856ULL,
        0x452301efcdab8967ULL,
    };
    const uint64_t block2_seeds[AI_MEMBLOCK_CMO_TRIPLE_ROUNDS] = {
        0x1021324354657687ULL,
        0x2132435465768798ULL,
        0x32435465768798a9ULL,
    };
    const uint64_t block0_older_store_values[AI_MEMBLOCK_CMO_TRIPLE_ROUNDS] = {
        0xa5a4a3a2a1a0b9b8ULL,
        0xb6b5b4b3b2b1cac9ULL,
        0xc7c6c5c4c3c2dbdaULL,
    };
    const uint64_t block0_second_store_values[AI_MEMBLOCK_CMO_TRIPLE_ROUNDS] = {
        0x18291a2b3c4d5e6fULL,
        0x293a2b3c4d5e6f70ULL,
        0x3a4b3c4d5e6f7081ULL,
    };
    const uint64_t block0_third_store_values[AI_MEMBLOCK_CMO_TRIPLE_ROUNDS] = {
        0xf0e1d2c3b4a59687ULL,
        0xe1d2c3b4a5968778ULL,
        0xd2c3b4a596877869ULL,
    };
    const uint64_t block0_fourth_store_values[AI_MEMBLOCK_CMO_TRIPLE_ROUNDS] = {
        0x789abcde00112233ULL,
        0x89abcdef11223344ULL,
        0x9abcdef022334455ULL,
    };
    const uint64_t before_guards[AI_MEMBLOCK_CMO_TRIPLE_ROUNDS] = {
        0x0a0b0c0d0e0f1011ULL,
        0x1b1c1d1e1f202122ULL,
        0x2c2d2e2f30313233ULL,
    };
    const uint64_t guard01_values[AI_MEMBLOCK_CMO_TRIPLE_ROUNDS] = {
        0x11100f0e0d0c0b0aULL,
        0x2221201f1e1d1c1bULL,
        0x333231302f2e2d2cULL,
    };
    const uint64_t guard12_values[AI_MEMBLOCK_CMO_TRIPLE_ROUNDS] = {
        0xabcdef9876543210ULL,
        0xbcdef0a987654321ULL,
        0xcdef01ba98765432ULL,
    };
    const uint64_t after_guards[AI_MEMBLOCK_CMO_TRIPLE_ROUNDS] = {
        0x1032547698efcdabULL,
        0x21436587a9f0debcULL,
        0x32547698ba01efcdULL,
    };
    const uint64_t marker_seeds[AI_MEMBLOCK_CMO_TRIPLE_ROUNDS] = {
        0x1122ffee3344ddccULL,
        0x223300ff4455eeddULL,
        0x334411005566ffeeULL,
    };
    const uint64_t marker_updates[AI_MEMBLOCK_CMO_TRIPLE_ROUNDS] = {
        0xccdd4433eeff2211ULL,
        0xddee5544ff003322ULL,
        0xeeff665500114433ULL,
    };
    const uint64_t marker_guards[AI_MEMBLOCK_CMO_TRIPLE_ROUNDS] = {
        0x55aa66bb77cc88ddULL,
        0x66bb77cc88dd99eeULL,
        0x77cc88dd99eeaaffULL,
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
        const uint64_t expected_block0_second_before =
            block0_seeds[round] +
            block0_second_store_word * 0x1111111111111111ULL;
        const uint64_t expected_block0_third_before =
            block0_seeds[round] +
            block0_third_store_word * 0x1111111111111111ULL;
        const uint64_t expected_block1_load =
            block1_seeds[round] +
            block1_load_word * 0x1111111111111111ULL;

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
        AI_ASSERT_NO_EXCEPT_AND("before the four-generation clean-inval-zero-inval CMO probe starts, all source blocks, guards, and marker are readable",
            ld(block0_vaddr) == expected_block0_fourth_before &&
            ld(block0_older_store_vaddr) == expected_block0_older_before &&
            ld(block0_second_store_vaddr) == expected_block0_second_before &&
            ld(block0_third_store_vaddr) == expected_block0_third_before &&
            ld(block1_vaddr) == block1_seeds[round] &&
            ld(block1_load_vaddr) == expected_block1_load &&
            ld(block2_vaddr) == block2_seeds[round] &&
            ld(marker_vaddr) == marker_seeds[round]
        );

        TEST_SETUP_EXCEPT();
        ai_memblock_cmo_triple_three_block_store_cbom_store_inval_store_zero_store_inval_window(
            block0_vaddr, block1_vaddr, block2_vaddr, marker_vaddr,
            block0_older_store_values[round],
            block0_second_store_values[round],
            block0_third_store_values[round],
            block0_fourth_store_values[round], marker_updates[round],
            &block0_older_observed, &block0_second_observed,
            &block0_third_observed, &block0_fourth_observed,
            &block0_zero_observed, &block1_observed, &block2_observed,
            &marker_observed);
        AI_ASSERT_NO_EXCEPT_AND("under three-block CMO pressure, late CBO.ZERO clears older three generations while the fourth store survives final CBO.INVAL",
            block0_older_observed == 0ULL &&
            block0_second_observed == 0ULL &&
            block0_third_observed == 0ULL &&
            block0_fourth_observed == block0_fourth_store_values[round] &&
            block0_zero_observed == 0ULL &&
            block1_observed == expected_block1_load &&
            block2_observed == 0ULL &&
            marker_observed == marker_updates[round]
        );

        TEST_SETUP_EXCEPT();
        AI_ASSERT_NO_EXCEPT_AND("after the four-generation clean-inval-zero-inval CMO probe, block0 keeps only the fourth post-zero store, block1 preserves seed, block2 is zero, and guards remain intact",
            ld(block0_fourth_store_vaddr) ==
                block0_fourth_store_values[round] &&
            ld(block0_older_store_vaddr) == 0ULL &&
            ld(block0_second_store_vaddr) == 0ULL &&
            ld(block0_third_store_vaddr) == 0ULL &&
            ld(block0_zero_load_vaddr) == 0ULL &&
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
        TEST_ASSERT("backing memory after the four-generation clean-inval-zero-inval CMO probe keeps block0 zero except the fourth post-zero store, block1 seed payload, block2 zero, marker update, and all guards intact",
            ai_memblock_cmo_triple_block_zero_except_word(
                block0_paddr, block0_fourth_store_word,
                block0_fourth_store_values[round]) &&
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

bool ai_micro_hs_interleaved_three_block_store_zero_store_inval_store_zero_store_inval_four_generation_late_zero_corner()
{
    TEST_START();

    const enum test_page source_page = VSRWX_GRWX;
    const enum test_page marker_page = SWITCH2;
    const size_t block0_fourth_store_word = 0U;
    const size_t block0_older_store_word = 1U;
    const size_t block0_zero_load_word = 2U;
    const size_t block0_second_store_word = 4U;
    const size_t block0_third_store_word = 6U;
    const size_t block1_load_word = 3U;
    const size_t block2_load_word = 5U;
    const uintptr_t block0_vaddr = hs_page_base(source_page) + 0xe80U;
    const uintptr_t block1_vaddr = hs_page_base(source_page) + 0xf00U;
    const uintptr_t block2_vaddr = hs_page_base(source_page) + 0xf80U;
    const uintptr_t block0_paddr = phys_page_base(source_page) + 0xe80U;
    const uintptr_t block1_paddr = phys_page_base(source_page) + 0xf00U;
    const uintptr_t block2_paddr = phys_page_base(source_page) + 0xf80U;
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
    const uintptr_t marker_vaddr = hs_page_base(marker_page) + 0xe80U;
    const uintptr_t marker_paddr = phys_page_base(marker_page) + 0xe80U;
    const uintptr_t marker_guard_vaddr = marker_vaddr + sizeof(uint64_t);
    const uintptr_t marker_guard_paddr = marker_paddr + sizeof(uint64_t);
    const uint64_t block0_seeds[AI_MEMBLOCK_CMO_TRIPLE_ROUNDS] = {
        0x1021324354657687ULL,
        0x2132435465768798ULL,
        0x32435465768798a9ULL,
    };
    const uint64_t block1_seeds[AI_MEMBLOCK_CMO_TRIPLE_ROUNDS] = {
        0x8798a9bacbdcedfeULL,
        0x98a9bacbdcedfe0fULL,
        0xa9bacbdcedfe0f10ULL,
    };
    const uint64_t block2_seeds[AI_MEMBLOCK_CMO_TRIPLE_ROUNDS] = {
        0x13579bdf2468ace0ULL,
        0x2468ace03579bdf1ULL,
        0x3579bdf1468ace02ULL,
    };
    const uint64_t block0_older_store_values[AI_MEMBLOCK_CMO_TRIPLE_ROUNDS] = {
        0x5a5b5c5d5e5f6061ULL,
        0x6b6c6d6e6f707172ULL,
        0x7c7d7e7f80818283ULL,
    };
    const uint64_t block0_second_store_values[AI_MEMBLOCK_CMO_TRIPLE_ROUNDS] = {
        0x61605f5e5d5c5b5aULL,
        0x7271706f6e6d6c6bULL,
        0x838281807f7e7d7cULL,
    };
    const uint64_t block0_third_store_values[AI_MEMBLOCK_CMO_TRIPLE_ROUNDS] = {
        0x0a1b2c3d4e5f6071ULL,
        0x1b2c3d4e5f607182ULL,
        0x2c3d4e5f60718293ULL,
    };
    const uint64_t block0_fourth_store_values[AI_MEMBLOCK_CMO_TRIPLE_ROUNDS] = {
        0xf1e2d3c4b5a69788ULL,
        0x02f3e4d5c6b7a899ULL,
        0x1304f5e6d7c8b9aaULL,
    };
    const uint64_t before_guards[AI_MEMBLOCK_CMO_TRIPLE_ROUNDS] = {
        0x0c0ffee0aabbccddULL,
        0x1d100ff1bbccddeeULL,
        0x2e211002ccddeeffULL,
    };
    const uint64_t guard01_values[AI_MEMBLOCK_CMO_TRIPLE_ROUNDS] = {
        0xddccbbaaeeff0c0fULL,
        0xeeddccbbff001d10ULL,
        0xffeeddcc00112e21ULL,
    };
    const uint64_t guard12_values[AI_MEMBLOCK_CMO_TRIPLE_ROUNDS] = {
        0x0f1e2d3c4b5a6978ULL,
        0x102f3e4d5c6b7a89ULL,
        0x21403f5e6d7c8b9aULL,
    };
    const uint64_t after_guards[AI_MEMBLOCK_CMO_TRIPLE_ROUNDS] = {
        0x78695a4b3c2d1e0fULL,
        0x897a6b5c4d3e2f10ULL,
        0x9a8b7c6d5e3f4021ULL,
    };
    const uint64_t marker_seeds[AI_MEMBLOCK_CMO_TRIPLE_ROUNDS] = {
        0x5566778899aabbccULL,
        0x66778899aabbccddULL,
        0x778899aabbccddeeULL,
    };
    const uint64_t marker_updates[AI_MEMBLOCK_CMO_TRIPLE_ROUNDS] = {
        0xccbbaa9988776655ULL,
        0xddccbbaa99887766ULL,
        0xeeddccbbaa998877ULL,
    };
    const uint64_t marker_guards[AI_MEMBLOCK_CMO_TRIPLE_ROUNDS] = {
        0xa5a55a5af0f00f0fULL,
        0xb6b64b4be1e11e1eULL,
        0xc7c73c3cd2d22d2dULL,
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
        AI_ASSERT_NO_EXCEPT_AND("before the four-generation zero-inval-zero-inval CMO probe starts, all source blocks, guards, and marker are readable",
            ld(block0_vaddr) == expected_block0_fourth_before &&
            ld(block0_older_store_vaddr) == expected_block0_older_before &&
            ld(block0_second_store_vaddr) == expected_block0_second_before &&
            ld(block0_third_store_vaddr) == expected_block0_third_before &&
            ld(block1_vaddr) == block1_seeds[round] &&
            ld(block1_load_vaddr) == expected_block1_load &&
            ld(block2_vaddr) == block2_seeds[round] &&
            ld(block2_load_vaddr) == expected_block2_load &&
            ld(marker_vaddr) == marker_seeds[round]
        );

        TEST_SETUP_EXCEPT();
        ai_memblock_cmo_triple_three_block_store_zero_store_inval_store_zero_store_inval_window(
            block0_vaddr, block1_vaddr, block2_vaddr, marker_vaddr,
            block0_older_store_values[round],
            block0_second_store_values[round],
            block0_third_store_values[round],
            block0_fourth_store_values[round], marker_updates[round],
            &block0_older_observed, &block0_second_observed,
            &block0_third_observed, &block0_fourth_observed,
            &block0_zero_observed, &block1_observed, &block2_observed,
            &marker_observed);
        AI_ASSERT_NO_EXCEPT_AND("under three-block CMO pressure, the second CBO.ZERO clears older three generations while the fourth store survives final CBO.INVAL after two inval owners",
            block0_older_observed == 0ULL &&
            block0_second_observed == 0ULL &&
            block0_third_observed == 0ULL &&
            block0_fourth_observed == block0_fourth_store_values[round] &&
            block0_zero_observed == 0ULL &&
            block1_observed == expected_block1_load &&
            block2_observed == 0ULL &&
            marker_observed == marker_updates[round]
        );

        TEST_SETUP_EXCEPT();
        AI_ASSERT_NO_EXCEPT_AND("after the four-generation zero-inval-zero-inval CMO probe, block0 keeps only the fourth post-second-zero store, block1 preserves seed, block2 is zero, and guards remain intact",
            ld(block0_fourth_store_vaddr) ==
                block0_fourth_store_values[round] &&
            ld(block0_older_store_vaddr) == 0ULL &&
            ld(block0_second_store_vaddr) == 0ULL &&
            ld(block0_third_store_vaddr) == 0ULL &&
            ld(block0_zero_load_vaddr) == 0ULL &&
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
        TEST_ASSERT("backing memory after the four-generation zero-inval-zero-inval CMO probe keeps block0 zero except the fourth post-second-zero store, block1 seed payload, block2 zero, marker update, and all guards intact",
            ai_memblock_cmo_triple_block_zero_except_word(
                block0_paddr, block0_fourth_store_word,
                block0_fourth_store_values[round]) &&
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

bool ai_micro_hs_interleaved_three_block_store_inval_store_zero_store_inval_store_zero_four_generation_final_zero_corner()
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
    const uintptr_t block0_vaddr = hs_page_base(source_page) + 0x2c80U;
    const uintptr_t block1_vaddr = hs_page_base(source_page) + 0x2e80U;
    const uintptr_t block2_vaddr = hs_page_base(source_page) + 0x3080U;
    const uintptr_t block0_paddr = phys_page_base(source_page) + 0x2c80U;
    const uintptr_t block1_paddr = phys_page_base(source_page) + 0x2e80U;
    const uintptr_t block2_paddr = phys_page_base(source_page) + 0x3080U;
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
    const uintptr_t marker_vaddr = hs_page_base(marker_page) + 0x2c80U;
    const uintptr_t marker_paddr = phys_page_base(marker_page) + 0x2c80U;
    const uintptr_t marker_guard_vaddr = marker_vaddr + sizeof(uint64_t);
    const uintptr_t marker_guard_paddr = marker_paddr + sizeof(uint64_t);
    const uint64_t block0_seeds[AI_MEMBLOCK_CMO_TRIPLE_ROUNDS] = {
        0x5061728394a5b6c7ULL,
        0x61728394a5b6c7d8ULL,
        0x728394a5b6c7d8e9ULL,
    };
    const uint64_t block1_seeds[AI_MEMBLOCK_CMO_TRIPLE_ROUNDS] = {
        0xc7b6a59483726150ULL,
        0xd8c7b6a594837261ULL,
        0xe9d8c7b6a5948372ULL,
    };
    const uint64_t block2_seeds[AI_MEMBLOCK_CMO_TRIPLE_ROUNDS] = {
        0x13572468ace0bdf1ULL,
        0x24683579bdf1ce02ULL,
        0x3579468ace02df13ULL,
    };
    const uint64_t block0_older_store_values[AI_MEMBLOCK_CMO_TRIPLE_ROUNDS] = {
        0x0a0b0c0d0e0f1011ULL,
        0x1b1c1d1e1f202122ULL,
        0x2c2d2e2f30313233ULL,
    };
    const uint64_t block0_second_store_values[AI_MEMBLOCK_CMO_TRIPLE_ROUNDS] = {
        0x11100f0e0d0c0b0aULL,
        0x2221201f1e1d1c1bULL,
        0x333231302f2e2d2cULL,
    };
    const uint64_t block0_third_store_values[AI_MEMBLOCK_CMO_TRIPLE_ROUNDS] = {
        0x0123456789abcdeeULL,
        0x123456789abcdef0ULL,
        0x23456789abcdef01ULL,
    };
    const uint64_t block0_fourth_store_values[AI_MEMBLOCK_CMO_TRIPLE_ROUNDS] = {
        0xeedcba9876543210ULL,
        0xffedcba987654321ULL,
        0x10fedcba98765432ULL,
    };
    const uint64_t before_guards[AI_MEMBLOCK_CMO_TRIPLE_ROUNDS] = {
        0xc001d00d13572468ULL,
        0xd112e11e24683579ULL,
        0xe223f22f3579468aULL,
    };
    const uint64_t guard01_values[AI_MEMBLOCK_CMO_TRIPLE_ROUNDS] = {
        0x682457130dd001c0ULL,
        0x793568241ee112d1ULL,
        0x8a4679352ff223e2ULL,
    };
    const uint64_t guard12_values[AI_MEMBLOCK_CMO_TRIPLE_ROUNDS] = {
        0x3141592653589793ULL,
        0x2718281828459045ULL,
        0x1618033988749894ULL,
    };
    const uint64_t after_guards[AI_MEMBLOCK_CMO_TRIPLE_ROUNDS] = {
        0x9397585326594131ULL,
        0x4590452818281827ULL,
        0x9498879839038161ULL,
    };
    const uint64_t marker_seeds[AI_MEMBLOCK_CMO_TRIPLE_ROUNDS] = {
        0xabcd1234ef005566ULL,
        0xbcde2345f0116677ULL,
        0xcdef345601227788ULL,
    };
    const uint64_t marker_updates[AI_MEMBLOCK_CMO_TRIPLE_ROUNDS] = {
        0x665500ef3412cdabULL,
        0x776611f04523debcULL,
        0x887722016534efcdULL,
    };
    const uint64_t marker_guards[AI_MEMBLOCK_CMO_TRIPLE_ROUNDS] = {
        0x0f0e0d0c0b0a0908ULL,
        0x1f1e1d1c1b1a1918ULL,
        0x2f2e2d2c2b2a2928ULL,
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
        AI_ASSERT_NO_EXCEPT_AND("before the four-generation inval-zero-inval-zero CMO probe starts, all source blocks, guards, and marker are readable",
            ld(block0_vaddr) == expected_block0_fourth_before &&
            ld(block0_older_store_vaddr) == expected_block0_older_before &&
            ld(block0_second_store_vaddr) == expected_block0_second_before &&
            ld(block0_third_store_vaddr) == expected_block0_third_before &&
            ld(block1_vaddr) == block1_seeds[round] &&
            ld(block2_vaddr) == block2_seeds[round] &&
            ld(block2_load_vaddr) == expected_block2_load &&
            ld(marker_vaddr) == marker_seeds[round]
        );

        TEST_SETUP_EXCEPT();
        ai_memblock_cmo_triple_three_block_store_inval_store_zero_store_inval_store_zero_window(
            block0_vaddr, block1_vaddr, block2_vaddr, marker_vaddr,
            block0_older_store_values[round],
            block0_second_store_values[round],
            block0_third_store_values[round],
            block0_fourth_store_values[round], marker_updates[round],
            &block0_older_observed, &block0_second_observed,
            &block0_third_observed, &block0_fourth_observed,
            &block0_zero_observed, &block1_observed, &block2_observed,
            &marker_observed);
        AI_ASSERT_NO_EXCEPT_AND("under three-block CMO pressure, final CBO.ZERO clears all generations after an earlier CBO.INVAL, middle CBO.ZERO, and second CBO.INVAL",
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
        AI_ASSERT_NO_EXCEPT_AND("after the four-generation inval-zero-inval-zero CMO probe, block0 and block1 are zero, block2 preserves seed, and guards remain intact",
            ld(block0_fourth_store_vaddr) == 0ULL &&
            ld(block0_older_store_vaddr) == 0ULL &&
            ld(block0_second_store_vaddr) == 0ULL &&
            ld(block0_third_store_vaddr) == 0ULL &&
            ld(block0_zero_load_vaddr) == 0ULL &&
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
        TEST_ASSERT("backing memory after the four-generation inval-zero-inval-zero CMO probe keeps block0 and block1 zero, block2 seed payload, marker update, and all guards intact",
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

bool ai_micro_hs_interleaved_three_block_store_zero_store_cbom_store_zero_store_cbom_four_generation_late_zero_corner()
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
    const uintptr_t marker_vaddr = hs_page_base(marker_page) + 0x3200U;
    const uintptr_t marker_paddr = phys_page_base(marker_page) + 0x3200U;
    const uintptr_t marker_guard_vaddr = marker_vaddr + sizeof(uint64_t);
    const uintptr_t marker_guard_paddr = marker_paddr + sizeof(uint64_t);
    const uint64_t block0_seeds[AI_MEMBLOCK_CMO_TRIPLE_ROUNDS] = {
        0x61728394a5b6c7d8ULL,
        0x728394a5b6c7d8e9ULL,
        0x8394a5b6c7d8e9faULL,
    };
    const uint64_t block1_seeds[AI_MEMBLOCK_CMO_TRIPLE_ROUNDS] = {
        0xd8c7b6a594837261ULL,
        0xe9d8c7b6a5948372ULL,
        0xfae9d8c7b6a59483ULL,
    };
    const uint64_t block2_seeds[AI_MEMBLOCK_CMO_TRIPLE_ROUNDS] = {
        0x02468ace13579bdfULL,
        0x13579bdf2468ace0ULL,
        0x2468ace03579bdf1ULL,
    };
    const uint64_t block0_older_store_values[AI_MEMBLOCK_CMO_TRIPLE_ROUNDS] = {
        0x1021324354657687ULL,
        0x2132435465768798ULL,
        0x32435465768798a9ULL,
    };
    const uint64_t block0_second_store_values[AI_MEMBLOCK_CMO_TRIPLE_ROUNDS] = {
        0x8776655443322110ULL,
        0x9887766554433221ULL,
        0xa998877665544332ULL,
    };
    const uint64_t block0_third_store_values[AI_MEMBLOCK_CMO_TRIPLE_ROUNDS] = {
        0x0f1e2d3c4b5a6978ULL,
        0x102f3e4d5c6b7a89ULL,
        0x21403f5e6d7c8b9aULL,
    };
    const uint64_t block0_fourth_store_values[AI_MEMBLOCK_CMO_TRIPLE_ROUNDS] = {
        0x78695a4b3c2d1e0fULL,
        0x897a6b5c4d3e2f10ULL,
        0x9a8b7c6d5e3f4021ULL,
    };
    const uint64_t before_guards[AI_MEMBLOCK_CMO_TRIPLE_ROUNDS] = {
        0xc001d00d2468ace0ULL,
        0xd112e11e3579bdf1ULL,
        0xe223f22f468ace02ULL,
    };
    const uint64_t guard01_values[AI_MEMBLOCK_CMO_TRIPLE_ROUNDS] = {
        0x0eca68240dd001c0ULL,
        0x1fdb79351ee112d1ULL,
        0x20ec8a462ff223e2ULL,
    };
    const uint64_t guard12_values[AI_MEMBLOCK_CMO_TRIPLE_ROUNDS] = {
        0x3141592653589793ULL,
        0x2718281828459045ULL,
        0x1618033988749894ULL,
    };
    const uint64_t after_guards[AI_MEMBLOCK_CMO_TRIPLE_ROUNDS] = {
        0x9397585326594131ULL,
        0x4590452818281827ULL,
        0x9498879839038161ULL,
    };
    const uint64_t marker_seeds[AI_MEMBLOCK_CMO_TRIPLE_ROUNDS] = {
        0xabcd1234ef005566ULL,
        0xbcde2345f0116677ULL,
        0xcdef345601227788ULL,
    };
    const uint64_t marker_updates[AI_MEMBLOCK_CMO_TRIPLE_ROUNDS] = {
        0x665500ef3412cdabULL,
        0x776611f04523debcULL,
        0x887722016534efcdULL,
    };
    const uint64_t marker_guards[AI_MEMBLOCK_CMO_TRIPLE_ROUNDS] = {
        0x55aa33cc55aa33ccULL,
        0x66bb44dd66bb44ddULL,
        0x77cc55ee77cc55eeULL,
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
        AI_ASSERT_NO_EXCEPT_AND("before the four-generation zero-clean-zero-clean CMO probe starts, all source blocks, guards, and marker are readable",
            ld(block0_vaddr) == expected_block0_fourth_before &&
            ld(block0_older_store_vaddr) == expected_block0_older_before &&
            ld(block0_second_store_vaddr) == expected_block0_second_before &&
            ld(block0_third_store_vaddr) == expected_block0_third_before &&
            ld(block1_vaddr) == block1_seeds[round] &&
            ld(block1_load_vaddr) == expected_block1_load &&
            ld(block2_vaddr) == block2_seeds[round] &&
            ld(block2_load_vaddr) == expected_block2_load &&
            ld(marker_vaddr) == marker_seeds[round]
        );

        TEST_SETUP_EXCEPT();
        ai_memblock_cmo_triple_three_block_store_zero_store_cbom_store_zero_store_cbom_window(
            block0_vaddr, block1_vaddr, block2_vaddr, marker_vaddr,
            block0_older_store_values[round],
            block0_second_store_values[round],
            block0_third_store_values[round],
            block0_fourth_store_values[round], marker_updates[round],
            &block0_older_observed, &block0_second_observed,
            &block0_third_observed, &block0_fourth_observed,
            &block0_zero_observed, &block1_observed, &block2_observed,
            &marker_observed);
        AI_ASSERT_NO_EXCEPT_AND("under three-block CMO pressure, the second CBO.ZERO clears older three generations while the fourth store survives final CBOM clean/flush",
            block0_older_observed == 0ULL &&
            block0_second_observed == 0ULL &&
            block0_third_observed == 0ULL &&
            block0_fourth_observed == block0_fourth_store_values[round] &&
            block0_zero_observed == 0ULL &&
            block1_observed == expected_block1_load &&
            block2_observed == 0ULL &&
            marker_observed == marker_updates[round]
        );

        TEST_SETUP_EXCEPT();
        AI_ASSERT_NO_EXCEPT_AND("after the four-generation zero-clean-zero-clean CMO probe, block0 keeps only the fourth post-second-zero store, block1 preserves seed, block2 is zero, and guards remain intact",
            ld(block0_fourth_store_vaddr) ==
                block0_fourth_store_values[round] &&
            ld(block0_older_store_vaddr) == 0ULL &&
            ld(block0_second_store_vaddr) == 0ULL &&
            ld(block0_third_store_vaddr) == 0ULL &&
            ld(block0_zero_load_vaddr) == 0ULL &&
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
        TEST_ASSERT("backing memory after the four-generation zero-clean-zero-clean CMO probe keeps block0 zero except the fourth post-second-zero store, block1 seed payload, block2 zero, marker update, and all guards intact",
            ai_memblock_cmo_triple_block_zero_except_word(
                block0_paddr, block0_fourth_store_word,
                block0_fourth_store_values[round]) &&
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

bool ai_micro_hs_interleaved_three_block_store_cbom_store_zero_store_cbom_store_zero_four_generation_final_zero_corner()
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
        0x1827364554637281ULL,
        0x2938475665748392ULL,
        0x3a495867768594a3ULL,
    };
    const uint64_t block1_seeds[AI_MEMBLOCK_CMO_TRIPLE_ROUNDS] = {
        0x8172635445362718ULL,
        0x9283746556473829ULL,
        0xa39485766658493aULL,
    };
    const uint64_t block2_seeds[AI_MEMBLOCK_CMO_TRIPLE_ROUNDS] = {
        0x0ace13572468bdf0ULL,
        0x1bdf24683579ce01ULL,
        0x2ce03579468adf12ULL,
    };
    const uint64_t block0_older_store_values[AI_MEMBLOCK_CMO_TRIPLE_ROUNDS] = {
        0xabcdef0123456789ULL,
        0xbcdef0123456789aULL,
        0xcdef0123456789abULL,
    };
    const uint64_t block0_second_store_values[AI_MEMBLOCK_CMO_TRIPLE_ROUNDS] = {
        0x9876543210fedcbaULL,
        0xa9876543210fedcbULL,
        0xba9876543210fedcULL,
    };
    const uint64_t block0_third_store_values[AI_MEMBLOCK_CMO_TRIPLE_ROUNDS] = {
        0x1122aabb3344ccddULL,
        0x2233bbcc4455ddeeULL,
        0x3344ccdd5566eeffULL,
    };
    const uint64_t block0_fourth_store_values[AI_MEMBLOCK_CMO_TRIPLE_ROUNDS] = {
        0xddcc4433bbaa2211ULL,
        0xeedd5544ccbb3322ULL,
        0xffee6655ddcc4433ULL,
    };
    const uint64_t before_guards[AI_MEMBLOCK_CMO_TRIPLE_ROUNDS] = {
        0xc001d00d89abcdefULL,
        0xd112e11e9abcdef0ULL,
        0xe223f22fabcdef01ULL,
    };
    const uint64_t guard01_values[AI_MEMBLOCK_CMO_TRIPLE_ROUNDS] = {
        0xfedcba980dd001c0ULL,
        0x0fedcba91ee112d1ULL,
        0x10fedcba2ff223e2ULL,
    };
    const uint64_t guard12_values[AI_MEMBLOCK_CMO_TRIPLE_ROUNDS] = {
        0x3141592653589793ULL,
        0x2718281828459045ULL,
        0x1618033988749894ULL,
    };
    const uint64_t after_guards[AI_MEMBLOCK_CMO_TRIPLE_ROUNDS] = {
        0x9397585326594131ULL,
        0x4590452818281827ULL,
        0x9498879839038161ULL,
    };
    const uint64_t marker_seeds[AI_MEMBLOCK_CMO_TRIPLE_ROUNDS] = {
        0x02468ace13579bdfULL,
        0x13579bdf2468ace0ULL,
        0x2468ace03579bdf1ULL,
    };
    const uint64_t marker_updates[AI_MEMBLOCK_CMO_TRIPLE_ROUNDS] = {
        0xfdb97531eca86420ULL,
        0x0eca8642fdb97531ULL,
        0x1fdb97530eca8642ULL,
    };
    const uint64_t marker_guards[AI_MEMBLOCK_CMO_TRIPLE_ROUNDS] = {
        0x0f0e0d0c0b0a0908ULL,
        0x1f1e1d1c1b1a1918ULL,
        0x2f2e2d2c2b2a2928ULL,
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
        const uint64_t expected_block0_second_before =
            block0_seeds[round] +
            block0_second_store_word * 0x1111111111111111ULL;
        const uint64_t expected_block0_third_before =
            block0_seeds[round] +
            block0_third_store_word * 0x1111111111111111ULL;
        const uint64_t expected_block1_load =
            block1_seeds[round] +
            block1_load_word * 0x1111111111111111ULL;

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
        AI_ASSERT_NO_EXCEPT_AND("before the four-generation clean-zero-clean-zero CMO probe starts, all source blocks, guards, and marker are readable",
            ld(block0_vaddr) == expected_block0_fourth_before &&
            ld(block0_older_store_vaddr) == expected_block0_older_before &&
            ld(block0_second_store_vaddr) == expected_block0_second_before &&
            ld(block0_third_store_vaddr) == expected_block0_third_before &&
            ld(block1_vaddr) == block1_seeds[round] &&
            ld(block1_load_vaddr) == expected_block1_load &&
            ld(block2_vaddr) == block2_seeds[round] &&
            ld(marker_vaddr) == marker_seeds[round]
        );

        TEST_SETUP_EXCEPT();
        ai_memblock_cmo_triple_three_block_store_cbom_store_zero_store_cbom_store_zero_window(
            block0_vaddr, block1_vaddr, block2_vaddr, marker_vaddr,
            block0_older_store_values[round],
            block0_second_store_values[round],
            block0_third_store_values[round],
            block0_fourth_store_values[round], marker_updates[round],
            &block0_older_observed, &block0_second_observed,
            &block0_third_observed, &block0_fourth_observed,
            &block0_zero_observed, &block1_observed, &block2_observed,
            &marker_observed);
        AI_ASSERT_NO_EXCEPT_AND("under three-block CMO pressure, final CBO.ZERO clears all generations after prior CBOM, middle CBO.ZERO, and second CBOM",
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
        AI_ASSERT_NO_EXCEPT_AND("after the four-generation clean-zero-clean-zero CMO probe, block0 is fully zero, block1 preserves seed, block2 is zero, and guards remain intact",
            ld(block0_fourth_store_vaddr) == 0ULL &&
            ld(block0_older_store_vaddr) == 0ULL &&
            ld(block0_second_store_vaddr) == 0ULL &&
            ld(block0_third_store_vaddr) == 0ULL &&
            ld(block0_zero_load_vaddr) == 0ULL &&
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
        TEST_ASSERT("backing memory after the four-generation clean-zero-clean-zero CMO probe keeps block0 and block2 fully zero, block1 seed payload, marker update, and all guards intact",
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

bool ai_micro_hs_interleaved_three_block_store_inval_store_cbom_store_inval_store_cbom_four_generation_no_zero_corner()
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
    const uintptr_t block0_vaddr = hs_page_base(source_page) + 0x3600U;
    const uintptr_t block1_vaddr = hs_page_base(source_page) + 0x3680U;
    const uintptr_t block2_vaddr = hs_page_base(source_page) + 0x3700U;
    const uintptr_t block0_paddr = phys_page_base(source_page) + 0x3600U;
    const uintptr_t block1_paddr = phys_page_base(source_page) + 0x3680U;
    const uintptr_t block2_paddr = phys_page_base(source_page) + 0x3700U;
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
    const uintptr_t marker_vaddr = hs_page_base(marker_page) + 0x3600U;
    const uintptr_t marker_paddr = phys_page_base(marker_page) + 0x3600U;
    const uintptr_t marker_guard_vaddr = marker_vaddr + sizeof(uint64_t);
    const uintptr_t marker_guard_paddr = marker_paddr + sizeof(uint64_t);
    const uint64_t block0_seeds[AI_MEMBLOCK_CMO_TRIPLE_ROUNDS] = {
        0x13579bdf02468aceULL,
        0x2468ace013579bdfULL,
        0x3579bdf12468ace0ULL,
    };
    const uint64_t block1_seeds[AI_MEMBLOCK_CMO_TRIPLE_ROUNDS] = {
        0xfdb975310eca8642ULL,
        0xeca86420fdb97531ULL,
        0xdb97531eeca86420ULL,
    };
    const uint64_t block2_seeds[AI_MEMBLOCK_CMO_TRIPLE_ROUNDS] = {
        0x1029384756aabbccULL,
        0x213a495867bbccddULL,
        0x324b5a6978ccddeeULL,
    };
    const uint64_t block0_older_store_values[AI_MEMBLOCK_CMO_TRIPLE_ROUNDS] = {
        0x0a1b2c3d4e5f6071ULL,
        0x1b2c3d4e5f607182ULL,
        0x2c3d4e5f60718293ULL,
    };
    const uint64_t block0_second_store_values[AI_MEMBLOCK_CMO_TRIPLE_ROUNDS] = {
        0x71605f4e3d2c1b0aULL,
        0x8271605f4e3d2c1bULL,
        0x938271605f4e3d2cULL,
    };
    const uint64_t block0_third_store_values[AI_MEMBLOCK_CMO_TRIPLE_ROUNDS] = {
        0x1123581321345589ULL,
        0x223469243245669aULL,
        0x33457a35435677abULL,
    };
    const uint64_t block0_fourth_store_values[AI_MEMBLOCK_CMO_TRIPLE_ROUNDS] = {
        0x8955342113582311ULL,
        0x9a66453224693422ULL,
        0xab775643357a4533ULL,
    };
    const uint64_t before_guards[AI_MEMBLOCK_CMO_TRIPLE_ROUNDS] = {
        0xc0013600d00d3600ULL,
        0xd1124711e11e4711ULL,
        0xe2235822f22f5822ULL,
    };
    const uint64_t guard01_values[AI_MEMBLOCK_CMO_TRIPLE_ROUNDS] = {
        0x0063d00d0063c001ULL,
        0x1174e11e1174d112ULL,
        0x2285f22f2285e223ULL,
    };
    const uint64_t guard12_values[AI_MEMBLOCK_CMO_TRIPLE_ROUNDS] = {
        0x5a5aa5a536003680ULL,
        0x6b6bb6b647113791ULL,
        0x7c7cc7c7582248a2ULL,
    };
    const uint64_t after_guards[AI_MEMBLOCK_CMO_TRIPLE_ROUNDS] = {
        0xa5a55a5a37003680ULL,
        0xb6b66b6b48114791ULL,
        0xc7c77c7c592258a2ULL,
    };
    const uint64_t marker_seeds[AI_MEMBLOCK_CMO_TRIPLE_ROUNDS] = {
        0x0102030405060708ULL,
        0x1112131415161718ULL,
        0x2122232425262728ULL,
    };
    const uint64_t marker_updates[AI_MEMBLOCK_CMO_TRIPLE_ROUNDS] = {
        0x8070605040302010ULL,
        0x9181716151413121ULL,
        0xa292827262524232ULL,
    };
    const uint64_t marker_guards[AI_MEMBLOCK_CMO_TRIPLE_ROUNDS] = {
        0x0f1e2d3c4b5a6978ULL,
        0x1f2e3d4c5b6a7988ULL,
        0x2f3e4d5c6b7a8998ULL,
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
        AI_ASSERT_NO_EXCEPT_AND("before the four-generation inval-clean-inval-clean CMO probe starts, all source blocks, guards, and marker are readable",
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
        ai_memblock_cmo_triple_three_block_store_inval_store_cbom_store_inval_store_cbom_window(
            block0_vaddr, block1_vaddr, block2_vaddr, marker_vaddr,
            block0_older_store_values[round],
            block0_second_store_values[round],
            block0_third_store_values[round],
            block0_fourth_store_values[round], marker_updates[round],
            &block0_older_observed, &block0_second_observed,
            &block0_third_observed, &block0_fourth_observed,
            &block0_seed_observed, &block1_observed, &block2_observed,
            &marker_observed);
        AI_ASSERT_NO_EXCEPT_AND("under three-block CMO pressure, alternating CBO.INVAL and CBOM keeps all four same-block store generations visible",
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
        AI_ASSERT_NO_EXCEPT_AND("after the four-generation inval-clean-inval-clean CMO probe, block0 preserves all stores, block1 is zero, block2 preserves seed, and guards remain intact",
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
        TEST_ASSERT("backing memory after the four-generation inval-clean-inval-clean CMO probe keeps block0 seed except all four store generations, block1 zero, block2 seed payload, marker update, and all guards intact",
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

bool ai_micro_hs_interleaved_three_block_store_cbom_store_inval_store_cbom_store_inval_four_generation_no_zero_corner()
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
    const uintptr_t block0_vaddr = hs_page_base(source_page) + 0x3800U;
    const uintptr_t block1_vaddr = hs_page_base(source_page) + 0x3880U;
    const uintptr_t block2_vaddr = hs_page_base(source_page) + 0x3900U;
    const uintptr_t block0_paddr = phys_page_base(source_page) + 0x3800U;
    const uintptr_t block1_paddr = phys_page_base(source_page) + 0x3880U;
    const uintptr_t block2_paddr = phys_page_base(source_page) + 0x3900U;
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
    const uintptr_t marker_vaddr = hs_page_base(marker_page) + 0x3800U;
    const uintptr_t marker_paddr = phys_page_base(marker_page) + 0x3800U;
    const uintptr_t marker_guard_vaddr = marker_vaddr + sizeof(uint64_t);
    const uintptr_t marker_guard_paddr = marker_paddr + sizeof(uint64_t);
    const uint64_t block0_seeds[AI_MEMBLOCK_CMO_TRIPLE_ROUNDS] = {
        0x55aa102030405060ULL,
        0x66bb213141516171ULL,
        0x77cc324252627282ULL,
    };
    const uint64_t block1_seeds[AI_MEMBLOCK_CMO_TRIPLE_ROUNDS] = {
        0x605040302010aa55ULL,
        0x716151413121bb66ULL,
        0x827262524232cc77ULL,
    };
    const uint64_t block2_seeds[AI_MEMBLOCK_CMO_TRIPLE_ROUNDS] = {
        0x0f1e2d3c4b5a6978ULL,
        0x1f2e3d4c5b6a7988ULL,
        0x2f3e4d5c6b7a8998ULL,
    };
    const uint64_t block0_older_store_values[AI_MEMBLOCK_CMO_TRIPLE_ROUNDS] = {
        0xa0b1c2d3e4f50617ULL,
        0xb1c2d3e4f5061728ULL,
        0xc2d3e4f506172839ULL,
    };
    const uint64_t block0_second_store_values[AI_MEMBLOCK_CMO_TRIPLE_ROUNDS] = {
        0x1706f5e4d3c2b1a0ULL,
        0x281706f5e4d3c2b1ULL,
        0x39281706f5e4d3c2ULL,
    };
    const uint64_t block0_third_store_values[AI_MEMBLOCK_CMO_TRIPLE_ROUNDS] = {
        0xabcdef9876543210ULL,
        0xbcdefa9876543211ULL,
        0xcdefab9876543212ULL,
    };
    const uint64_t block0_fourth_store_values[AI_MEMBLOCK_CMO_TRIPLE_ROUNDS] = {
        0x0123456789abcdefULL,
        0x123456789abcdef0ULL,
        0x23456789abcdef01ULL,
    };
    const uint64_t before_guards[AI_MEMBLOCK_CMO_TRIPLE_ROUNDS] = {
        0xc0014000d00d4000ULL,
        0xd1125111e11e5111ULL,
        0xe2236222f22f6222ULL,
    };
    const uint64_t guard01_values[AI_MEMBLOCK_CMO_TRIPLE_ROUNDS] = {
        0x0040d00d0040c001ULL,
        0x1151e11e1151d112ULL,
        0x2262f22f2262e223ULL,
    };
    const uint64_t guard12_values[AI_MEMBLOCK_CMO_TRIPLE_ROUNDS] = {
        0x3141592640004080ULL,
        0x2718281851115191ULL,
        0x16180339622262a2ULL,
    };
    const uint64_t after_guards[AI_MEMBLOCK_CMO_TRIPLE_ROUNDS] = {
        0x0804004096269513ULL,
        0x1915115181828272ULL,
        0x2a26626293038161ULL,
    };
    const uint64_t marker_seeds[AI_MEMBLOCK_CMO_TRIPLE_ROUNDS] = {
        0x13572468ace0bdf1ULL,
        0x24683579bdf1cef2ULL,
        0x3579468ace02df03ULL,
    };
    const uint64_t marker_updates[AI_MEMBLOCK_CMO_TRIPLE_ROUNDS] = {
        0xf1bde0ac68245713ULL,
        0xf2cef1bd79356824ULL,
        0xf3df02ce8a467935ULL,
    };
    const uint64_t marker_guards[AI_MEMBLOCK_CMO_TRIPLE_ROUNDS] = {
        0x55aa55aa40004000ULL,
        0x66bb66bb51115111ULL,
        0x77cc77cc62226222ULL,
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
        AI_ASSERT_NO_EXCEPT_AND("before the four-generation clean-inval-clean-inval CMO probe starts, all source blocks, guards, and marker are readable",
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
        ai_memblock_cmo_triple_three_block_store_cbom_store_inval_store_cbom_store_inval_window(
            block0_vaddr, block1_vaddr, block2_vaddr, marker_vaddr,
            block0_older_store_values[round],
            block0_second_store_values[round],
            block0_third_store_values[round],
            block0_fourth_store_values[round], marker_updates[round],
            &block0_older_observed, &block0_second_observed,
            &block0_third_observed, &block0_fourth_observed,
            &block0_seed_observed, &block1_observed, &block2_observed,
            &marker_observed);
        AI_ASSERT_NO_EXCEPT_AND("under three-block CMO pressure, alternating CBOM and CBO.INVAL keeps all four same-block store generations visible",
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
        AI_ASSERT_NO_EXCEPT_AND("after the four-generation clean-inval-clean-inval CMO probe, block0 preserves all stores, block1 is zero, block2 preserves seed, and guards remain intact",
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
        TEST_ASSERT("backing memory after the four-generation clean-inval-clean-inval CMO probe keeps block0 seed except all four store generations, block1 zero, block2 seed payload, marker update, and all guards intact",
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

bool ai_micro_hs_interleaved_three_block_store_cbom_store_cbom_store_cbom_store_cbom_four_generation_no_zero_corner()
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
    const uintptr_t marker_vaddr = hs_page_base(marker_page) + 0x3a00U;
    const uintptr_t marker_paddr = phys_page_base(marker_page) + 0x3a00U;
    const uintptr_t marker_guard_vaddr = marker_vaddr + sizeof(uint64_t);
    const uintptr_t marker_guard_paddr = marker_paddr + sizeof(uint64_t);
    const uint64_t block0_seeds[AI_MEMBLOCK_CMO_TRIPLE_ROUNDS] = {
        0x0ace13579bdf2468ULL,
        0x1bdf2468ace03579ULL,
        0x2ce03579bdf1468aULL,
    };
    const uint64_t block1_seeds[AI_MEMBLOCK_CMO_TRIPLE_ROUNDS] = {
        0x8642fdb97531eca0ULL,
        0x97530eca8642fdb1ULL,
        0xa8641fdb97530ec2ULL,
    };
    const uint64_t block2_seeds[AI_MEMBLOCK_CMO_TRIPLE_ROUNDS] = {
        0x2468ace013579bdfULL,
        0x3579bdf12468ace0ULL,
        0x468ace023579bdf1ULL,
    };
    const uint64_t block0_older_store_values[AI_MEMBLOCK_CMO_TRIPLE_ROUNDS] = {
        0xd00d100020003000ULL,
        0xe11e211131114111ULL,
        0xf22f322242225222ULL,
    };
    const uint64_t block0_second_store_values[AI_MEMBLOCK_CMO_TRIPLE_ROUNDS] = {
        0x000300020001d00dULL,
        0x111411131112e11eULL,
        0x222522242223f22fULL,
    };
    const uint64_t block0_third_store_values[AI_MEMBLOCK_CMO_TRIPLE_ROUNDS] = {
        0x5a5a0badc0ffee00ULL,
        0x6b6b1cbad1ffee11ULL,
        0x7c7c2dcb02ffee22ULL,
    };
    const uint64_t block0_fourth_store_values[AI_MEMBLOCK_CMO_TRIPLE_ROUNDS] = {
        0x00eeffc0ad0b5a5aULL,
        0x11eeffd1ad1c6b6bULL,
        0x22eeff02cb2d7c7cULL,
    };
    const uint64_t before_guards[AI_MEMBLOCK_CMO_TRIPLE_ROUNDS] = {
        0xc0013a00d00d3a00ULL,
        0xd1124b11e11e4b11ULL,
        0xe2235c22f22f5c22ULL,
    };
    const uint64_t guard01_values[AI_MEMBLOCK_CMO_TRIPLE_ROUNDS] = {
        0x00a3d00d00a3c001ULL,
        0x11b4e11e11b4d112ULL,
        0x22c5f22f22c5e223ULL,
    };
    const uint64_t guard12_values[AI_MEMBLOCK_CMO_TRIPLE_ROUNDS] = {
        0x314159263a003a80ULL,
        0x271828184b114b91ULL,
        0x161803395c225ca2ULL,
    };
    const uint64_t after_guards[AI_MEMBLOCK_CMO_TRIPLE_ROUNDS] = {
        0x08a3003a96269513ULL,
        0x19b4114b81828272ULL,
        0x2ac5225c93038161ULL,
    };
    const uint64_t marker_seeds[AI_MEMBLOCK_CMO_TRIPLE_ROUNDS] = {
        0xabcdef0123456789ULL,
        0xbcdef0123456789aULL,
        0xcdef0123456789abULL,
    };
    const uint64_t marker_updates[AI_MEMBLOCK_CMO_TRIPLE_ROUNDS] = {
        0x9876543210fedcbaULL,
        0xa9876543210fedcbULL,
        0xba9876543210fedcULL,
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
        AI_ASSERT_NO_EXCEPT_AND("before the four-generation clean-clean-clean-clean CMO probe starts, all source blocks, guards, and marker are readable",
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
        ai_memblock_cmo_triple_three_block_store_cbom_store_cbom_store_cbom_store_cbom_window(
            block0_vaddr, block1_vaddr, block2_vaddr, marker_vaddr,
            block0_older_store_values[round],
            block0_second_store_values[round],
            block0_third_store_values[round],
            block0_fourth_store_values[round], marker_updates[round],
            &block0_older_observed, &block0_second_observed,
            &block0_third_observed, &block0_fourth_observed,
            &block0_seed_observed, &block1_observed, &block2_observed,
            &marker_observed);
        AI_ASSERT_NO_EXCEPT_AND("under three-block CMO pressure, repeated same-block CBOM clean/flush keeps all four store generations visible",
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
        AI_ASSERT_NO_EXCEPT_AND("after the four-generation clean-clean-clean-clean CMO probe, block0 preserves all stores, block1 is zero, block2 preserves seed, and guards remain intact",
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
        TEST_ASSERT("backing memory after the four-generation clean-clean-clean-clean CMO probe keeps block0 seed except all four store generations, block1 zero, block2 seed payload, marker update, and all guards intact",
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

bool ai_micro_hs_interleaved_three_block_store_inval_store_inval_store_inval_store_inval_four_generation_no_zero_corner()
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
    const uintptr_t block0_vaddr = hs_page_base(source_page) + 0x3c00U;
    const uintptr_t block1_vaddr = hs_page_base(source_page) + 0x3c80U;
    const uintptr_t block2_vaddr = hs_page_base(source_page) + 0x3d00U;
    const uintptr_t block0_paddr = phys_page_base(source_page) + 0x3c00U;
    const uintptr_t block1_paddr = phys_page_base(source_page) + 0x3c80U;
    const uintptr_t block2_paddr = phys_page_base(source_page) + 0x3d00U;
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
    const uintptr_t marker_vaddr = hs_page_base(marker_page) + 0x3c00U;
    const uintptr_t marker_paddr = phys_page_base(marker_page) + 0x3c00U;
    const uintptr_t marker_guard_vaddr = marker_vaddr + sizeof(uint64_t);
    const uintptr_t marker_guard_paddr = marker_paddr + sizeof(uint64_t);
    const uint64_t block0_seeds[AI_MEMBLOCK_CMO_TRIPLE_ROUNDS] = {
        0x1a2b3c4d5e6f7081ULL,
        0x2b3c4d5e6f708192ULL,
        0x3c4d5e6f708192a3ULL,
    };
    const uint64_t block1_seeds[AI_MEMBLOCK_CMO_TRIPLE_ROUNDS] = {
        0x9180f7e6d5c4b3a2ULL,
        0xa29108f7e6d5c4b3ULL,
        0xb3a21908f7e6d5c4ULL,
    };
    const uint64_t block2_seeds[AI_MEMBLOCK_CMO_TRIPLE_ROUNDS] = {
        0x0123456789abcdefULL,
        0x123456789abcdef0ULL,
        0x23456789abcdef01ULL,
    };
    const uint64_t block0_older_store_values[AI_MEMBLOCK_CMO_TRIPLE_ROUNDS] = {
        0xcafe100020003000ULL,
        0xdb0f211131114111ULL,
        0xec20322242225222ULL,
    };
    const uint64_t block0_second_store_values[AI_MEMBLOCK_CMO_TRIPLE_ROUNDS] = {
        0x000300020001efacULL,
        0x1114111311120fbdULL,
        0x22252224222320ceULL,
    };
    const uint64_t block0_third_store_values[AI_MEMBLOCK_CMO_TRIPLE_ROUNDS] = {
        0x135724680badc0deULL,
        0x246835791cbad1efULL,
        0x3579468a2dcb0200ULL,
    };
    const uint64_t block0_fourth_store_values[AI_MEMBLOCK_CMO_TRIPLE_ROUNDS] = {
        0xed0cda0b86427531ULL,
        0xfe1deb1c97538642ULL,
        0x0f2efc2da8649753ULL,
    };
    const uint64_t before_guards[AI_MEMBLOCK_CMO_TRIPLE_ROUNDS] = {
        0xc0013c00d00d3c00ULL,
        0xd1124d11e11e4d11ULL,
        0xe2235e22f22f5e22ULL,
    };
    const uint64_t guard01_values[AI_MEMBLOCK_CMO_TRIPLE_ROUNDS] = {
        0x00c3d00d00c3c001ULL,
        0x11d4e11e11d4d112ULL,
        0x22e5f22f22e5e223ULL,
    };
    const uint64_t guard12_values[AI_MEMBLOCK_CMO_TRIPLE_ROUNDS] = {
        0x314159263c003c80ULL,
        0x271828184d114d91ULL,
        0x161803395e225ea2ULL,
    };
    const uint64_t after_guards[AI_MEMBLOCK_CMO_TRIPLE_ROUNDS] = {
        0x08c3003c96269513ULL,
        0x19d4114d81828272ULL,
        0x2ae5225e93038161ULL,
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
        0x55aa55aa3c003c00ULL,
        0x66bb66bb4d114d11ULL,
        0x77cc77cc5e225e22ULL,
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
        AI_ASSERT_NO_EXCEPT_AND("before the four-generation inval-inval-inval-inval CMO probe starts, all source blocks, guards, and marker are readable",
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
        ai_memblock_cmo_triple_three_block_store_inval_store_inval_store_inval_store_inval_window(
            block0_vaddr, block1_vaddr, block2_vaddr, marker_vaddr,
            block0_older_store_values[round],
            block0_second_store_values[round],
            block0_third_store_values[round],
            block0_fourth_store_values[round], marker_updates[round],
            &block0_older_observed, &block0_second_observed,
            &block0_third_observed, &block0_fourth_observed,
            &block0_seed_observed, &block1_observed, &block2_observed,
            &marker_observed);
        AI_ASSERT_NO_EXCEPT_AND("under three-block CMO pressure, repeated same-block CBO.INVAL keeps all four store generations visible",
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
        AI_ASSERT_NO_EXCEPT_AND("after the four-generation inval-inval-inval-inval CMO probe, block0 preserves all stores, block1 is zero, block2 preserves seed, and guards remain intact",
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
        TEST_ASSERT("backing memory after the four-generation inval-inval-inval-inval CMO probe keeps block0 seed except all four store generations, block1 zero, block2 seed payload, marker update, and all guards intact",
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

bool ai_micro_hs_interleaved_three_block_store_zero_store_zero_store_zero_store_zero_four_generation_final_zero_corner()
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
    const uintptr_t block0_vaddr = hs_page_base(source_page) + 0x3e00U;
    const uintptr_t block1_vaddr = hs_page_base(source_page) + 0x3e80U;
    const uintptr_t block2_vaddr = hs_page_base(source_page) + 0x3f00U;
    const uintptr_t block0_paddr = phys_page_base(source_page) + 0x3e00U;
    const uintptr_t block1_paddr = phys_page_base(source_page) + 0x3e80U;
    const uintptr_t block2_paddr = phys_page_base(source_page) + 0x3f00U;
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
    const uintptr_t marker_vaddr = hs_page_base(marker_page) + 0x3e00U;
    const uintptr_t marker_paddr = phys_page_base(marker_page) + 0x3e00U;
    const uintptr_t marker_guard_vaddr = marker_vaddr + sizeof(uint64_t);
    const uintptr_t marker_guard_paddr = marker_paddr + sizeof(uint64_t);
    const uint64_t block0_seeds[AI_MEMBLOCK_CMO_TRIPLE_ROUNDS] = {
        0x08192a3b4c5d6e7fULL,
        0x192a3b4c5d6e7f80ULL,
        0x2a3b4c5d6e7f8091ULL,
    };
    const uint64_t block1_seeds[AI_MEMBLOCK_CMO_TRIPLE_ROUNDS] = {
        0xf7e6d5c4b3a29180ULL,
        0xe6d5c4b3a291807fULL,
        0xd5c4b3a291807e6eULL,
    };
    const uint64_t block2_seeds[AI_MEMBLOCK_CMO_TRIPLE_ROUNDS] = {
        0x0f1e2d3c4b5a6978ULL,
        0x1e2d3c4b5a697887ULL,
        0x2d3c4b5a69788796ULL,
    };
    const uint64_t block0_older_store_values[AI_MEMBLOCK_CMO_TRIPLE_ROUNDS] = {
        0x5a5a100020003000ULL,
        0x6b6b211131114111ULL,
        0x7c7c322242225222ULL,
    };
    const uint64_t block0_second_store_values[AI_MEMBLOCK_CMO_TRIPLE_ROUNDS] = {
        0x000300020001a5a5ULL,
        0x111411131112b6b6ULL,
        0x222522242223c7c7ULL,
    };
    const uint64_t block0_third_store_values[AI_MEMBLOCK_CMO_TRIPLE_ROUNDS] = {
        0x42420000badc0de0ULL,
        0x53531111cbad1ef1ULL,
        0x64642222dcbe2002ULL,
    };
    const uint64_t block0_fourth_store_values[AI_MEMBLOCK_CMO_TRIPLE_ROUNDS] = {
        0x0edc0dab00002424ULL,
        0x1fed1ebc11113535ULL,
        0x20fe2fcd22224646ULL,
    };
    const uint64_t before_guards[AI_MEMBLOCK_CMO_TRIPLE_ROUNDS] = {
        0xc0013e00d00d3e00ULL,
        0xd1124f11e11e4f11ULL,
        0xe2236022f22f6022ULL,
    };
    const uint64_t guard01_values[AI_MEMBLOCK_CMO_TRIPLE_ROUNDS] = {
        0x00e3d00d00e3c001ULL,
        0x11f4e11e11f4d112ULL,
        0x2205f22f2205e223ULL,
    };
    const uint64_t guard12_values[AI_MEMBLOCK_CMO_TRIPLE_ROUNDS] = {
        0x314159263e003e80ULL,
        0x271828184f114f91ULL,
        0x16180339602260a2ULL,
    };
    const uint64_t after_guards[AI_MEMBLOCK_CMO_TRIPLE_ROUNDS] = {
        0x08e3003e96269513ULL,
        0x19f4114f81828272ULL,
        0x2a05226093038161ULL,
    };
    const uint64_t marker_seeds[AI_MEMBLOCK_CMO_TRIPLE_ROUNDS] = {
        0x6789abcdef012345ULL,
        0x789abcdef0123456ULL,
        0x89abcdef01234567ULL,
    };
    const uint64_t marker_updates[AI_MEMBLOCK_CMO_TRIPLE_ROUNDS] = {
        0x543210fedcba9876ULL,
        0x6543210fedcba987ULL,
        0x76543210fedcba98ULL,
    };
    const uint64_t marker_guards[AI_MEMBLOCK_CMO_TRIPLE_ROUNDS] = {
        0x55aa55aa3e003e00ULL,
        0x66bb66bb4f114f11ULL,
        0x77cc77cc60226022ULL,
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
        AI_ASSERT_NO_EXCEPT_AND("before the four-generation zero-zero-zero-zero CMO probe starts, all source blocks, guards, and marker are readable",
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
        ai_memblock_cmo_triple_three_block_store_zero_store_zero_store_zero_store_zero_window(
            block0_vaddr, block1_vaddr, block2_vaddr, marker_vaddr,
            block0_older_store_values[round],
            block0_second_store_values[round],
            block0_third_store_values[round],
            block0_fourth_store_values[round], marker_updates[round],
            &block0_older_observed, &block0_second_observed,
            &block0_third_observed, &block0_fourth_observed,
            &block0_seed_observed, &block1_observed, &block2_observed,
            &marker_observed);
        AI_ASSERT_NO_EXCEPT_AND("under three-block CMO pressure, repeated same-block CBO.ZERO clears all four store generations",
            block0_older_observed == 0ULL &&
            block0_second_observed == 0ULL &&
            block0_third_observed == 0ULL &&
            block0_fourth_observed == 0ULL &&
            block0_seed_observed == 0ULL &&
            block1_observed == 0ULL &&
            block2_observed == expected_block2_load &&
            marker_observed == marker_updates[round]
        );

        TEST_SETUP_EXCEPT();
        AI_ASSERT_NO_EXCEPT_AND("after the four-generation zero-zero-zero-zero CMO probe, block0 and block1 are zero, block2 preserves seed, and guards remain intact",
            ld(block0_fourth_store_vaddr) == 0ULL &&
            ld(block0_older_store_vaddr) == 0ULL &&
            ld(block0_seed_load_vaddr) == 0ULL &&
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
        TEST_ASSERT("backing memory after the four-generation zero-zero-zero-zero CMO probe keeps block0 and block1 zero, block2 seed payload, marker update, and all guards intact",
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
