#include "ai_micro_memblock_cmo_triple_common.h"

bool ai_micro_hs_interleaved_three_block_store_zero_store_inval_store_cbom_three_generation_corner()
{
    TEST_START();

    const enum test_page source_page = SWITCH1;
    const enum test_page marker_page = SWITCH2;
    const size_t block0_older_store_word = 1U;
    const size_t block0_middle_store_word = 4U;
    const size_t block0_younger_store_word = 6U;
    const size_t block0_zero_load_word = 2U;
    const size_t block1_load_word = 3U;
    const size_t block2_load_word = 5U;
    const uintptr_t block0_vaddr = hs_page_base(source_page) + 0x2f00U;
    const uintptr_t block1_vaddr = hs_page_base(source_page) + 0x2f80U;
    const uintptr_t block2_vaddr = hs_page_base(source_page) + 0x3000U;
    const uintptr_t block0_paddr = phys_page_base(source_page) + 0x2f00U;
    const uintptr_t block1_paddr = phys_page_base(source_page) + 0x2f80U;
    const uintptr_t block2_paddr = phys_page_base(source_page) + 0x3000U;
    const uintptr_t block0_older_store_vaddr =
        block0_vaddr + block0_older_store_word * sizeof(uint64_t);
    const uintptr_t block0_zero_vaddr =
        block0_vaddr + 3U * sizeof(uint64_t);
    const uintptr_t block1_inval_vaddr =
        block1_vaddr + 2U * sizeof(uint64_t);
    const uintptr_t block2_zero_vaddr =
        block2_vaddr + 1U * sizeof(uint64_t);
    const uintptr_t block0_middle_store_vaddr =
        block0_vaddr + block0_middle_store_word * sizeof(uint64_t);
    const uintptr_t block1_clean_vaddr =
        block1_vaddr + 5U * sizeof(uint64_t);
    const uintptr_t block2_clean_vaddr =
        block2_vaddr + 6U * sizeof(uint64_t);
    const uintptr_t block0_inval_vaddr =
        block0_vaddr + 5U * sizeof(uint64_t);
    const uintptr_t block0_younger_store_vaddr =
        block0_vaddr + block0_younger_store_word * sizeof(uint64_t);
    const uintptr_t block0_clean_vaddr =
        block0_vaddr + 7U * sizeof(uint64_t);
    const uintptr_t block0_flush_vaddr =
        block0_vaddr +
        (AI_MEMBLOCK_CMO_TRIPLE_BLOCK_WORDS - 1U) * sizeof(uint64_t);
    const uintptr_t block1_flush_vaddr =
        block1_vaddr +
        (AI_MEMBLOCK_CMO_TRIPLE_BLOCK_WORDS - 1U) * sizeof(uint64_t);
    const uintptr_t block2_flush_vaddr =
        block2_vaddr +
        (AI_MEMBLOCK_CMO_TRIPLE_BLOCK_WORDS - 1U) * sizeof(uint64_t);
    const uintptr_t block0_older_load_vaddr = block0_older_store_vaddr;
    const uintptr_t block0_middle_load_vaddr = block0_middle_store_vaddr;
    const uintptr_t block0_younger_load_vaddr = block0_younger_store_vaddr;
    const uintptr_t block0_zero_load_vaddr =
        block0_vaddr + block0_zero_load_word * sizeof(uint64_t);
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
    const uintptr_t marker_vaddr = hs_page_base(marker_page) + 0x2f00U;
    const uintptr_t marker_paddr = phys_page_base(marker_page) + 0x2f00U;
    const uintptr_t marker_guard_vaddr = marker_vaddr + sizeof(uint64_t);
    const uintptr_t marker_guard_paddr = marker_paddr + sizeof(uint64_t);
    const uint64_t block0_seeds[AI_MEMBLOCK_CMO_TRIPLE_ROUNDS] = {
        0x010f1e2d3c4b5a69ULL,
        0x122031405f6e7d8cULL,
        0x23314251607f8e9dULL,
    };
    const uint64_t block1_seeds[AI_MEMBLOCK_CMO_TRIPLE_ROUNDS] = {
        0x88796a5b4c3d2e1fULL,
        0x998a7b6c5d4e3f20ULL,
        0xaa9b8c7d6e5f4031ULL,
    };
    const uint64_t block2_seeds[AI_MEMBLOCK_CMO_TRIPLE_ROUNDS] = {
        0x1021324354657687ULL,
        0x2132435465768798ULL,
        0x32435465768798a9ULL,
    };
    const uint64_t block0_older_store_values[AI_MEMBLOCK_CMO_TRIPLE_ROUNDS] = {
        0xd1d2d3d4d5d6d7d8ULL,
        0xe1e2e3e4e5e6e7e8ULL,
        0xf1f2f3f4f5f6f7f8ULL,
    };
    const uint64_t block0_middle_store_values[AI_MEMBLOCK_CMO_TRIPLE_ROUNDS] = {
        0x0a0b0c0d0e0f1011ULL,
        0x1a1b1c1d1e1f2021ULL,
        0x2a2b2c2d2e2f3031ULL,
    };
    const uint64_t block0_younger_store_values[AI_MEMBLOCK_CMO_TRIPLE_ROUNDS] = {
        0x9081726354453627ULL,
        0xa092837465564738ULL,
        0xb0a3948576675849ULL,
    };
    const uint64_t before_guards[AI_MEMBLOCK_CMO_TRIPLE_ROUNDS] = {
        0x55aa010203040506ULL,
        0x66bb111213141516ULL,
        0x77cc212223242526ULL,
    };
    const uint64_t guard01_values[AI_MEMBLOCK_CMO_TRIPLE_ROUNDS] = {
        0x060504030201aa55ULL,
        0x161514131211bb66ULL,
        0x262524232221cc77ULL,
    };
    const uint64_t guard12_values[AI_MEMBLOCK_CMO_TRIPLE_ROUNDS] = {
        0x13572468abcdef01ULL,
        0x24683579bcdef012ULL,
        0x3579468acdef0123ULL,
    };
    const uint64_t after_guards[AI_MEMBLOCK_CMO_TRIPLE_ROUNDS] = {
        0x10fedcba86427531ULL,
        0x21fedcba97538642ULL,
        0x32fedcbaa8649753ULL,
    };
    const uint64_t marker_seeds[AI_MEMBLOCK_CMO_TRIPLE_ROUNDS] = {
        0xc001d00d11112222ULL,
        0xd112e11e22223333ULL,
        0xe223f22f33334444ULL,
    };
    const uint64_t marker_updates[AI_MEMBLOCK_CMO_TRIPLE_ROUNDS] = {
        0x22221111d00dc001ULL,
        0x33332222e11ed112ULL,
        0x44443333f22fe223ULL,
    };
    const uint64_t marker_guards[AI_MEMBLOCK_CMO_TRIPLE_ROUNDS] = {
        0xabcdef0123456789ULL,
        0xbcdef0123456789aULL,
        0xcdef0123456789abULL,
    };

    ai_memblock_cmo_triple_prepare_hs_env();

    for (size_t round = 0; round < AI_MEMBLOCK_CMO_TRIPLE_ROUNDS; ++round) {
        uint64_t block0_older_observed = 0ULL;
        uint64_t block0_middle_observed = 0ULL;
        uint64_t block0_younger_observed = 0ULL;
        uint64_t block0_zero_observed = 0ULL;
        uint64_t block1_observed = 0ULL;
        uint64_t block2_observed = 0ULL;
        uint64_t marker_observed = 0ULL;
        const uint64_t expected_block0_older_before =
            block0_seeds[round] +
            block0_older_store_word * 0x1111111111111111ULL;
        const uint64_t expected_block0_middle_before =
            block0_seeds[round] +
            block0_middle_store_word * 0x1111111111111111ULL;
        const uint64_t expected_block0_younger_before =
            block0_seeds[round] +
            block0_younger_store_word * 0x1111111111111111ULL;
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
        AI_ASSERT_NO_EXCEPT_AND("before the three-generation store CMO probe starts, all source blocks, guards, and marker are readable",
            ld(block0_vaddr) == block0_seeds[round] &&
            ld(block0_older_store_vaddr) == expected_block0_older_before &&
            ld(block0_middle_store_vaddr) == expected_block0_middle_before &&
            ld(block0_younger_store_vaddr) ==
                expected_block0_younger_before &&
            ld(block1_vaddr) == block1_seeds[round] &&
            ld(block1_load_vaddr) == expected_block1_load &&
            ld(block2_vaddr) == block2_seeds[round] &&
            ld(marker_vaddr) == marker_seeds[round]
        );

        TEST_SETUP_EXCEPT();
        ai_memblock_cmo_triple_three_block_store_zero_store_inval_store_cbom_window(
            block0_older_store_vaddr,
            block0_older_store_values[round], block0_zero_vaddr,
            block1_inval_vaddr, block2_zero_vaddr,
            block0_middle_store_vaddr,
            block0_middle_store_values[round], block1_clean_vaddr,
            block2_clean_vaddr, block0_inval_vaddr,
            block0_younger_store_vaddr,
            block0_younger_store_values[round], block0_clean_vaddr,
            block0_flush_vaddr, block1_flush_vaddr, block2_flush_vaddr,
            block0_older_load_vaddr, block0_middle_load_vaddr,
            block0_younger_load_vaddr, block0_zero_load_vaddr,
            block1_load_vaddr, block2_load_vaddr, marker_vaddr,
            marker_updates[round], &block0_older_observed,
            &block0_middle_observed, &block0_younger_observed,
            &block0_zero_observed, &block1_observed, &block2_observed,
            &marker_observed);
        AI_ASSERT_NO_EXCEPT_AND("under three-block CMO pressure, CBO.ZERO clears the older generation while middle and younger generations survive the following CBO.INVAL and CBOM",
            block0_older_observed == 0ULL &&
            block0_middle_observed == block0_middle_store_values[round] &&
            block0_younger_observed == block0_younger_store_values[round] &&
            block0_zero_observed == 0ULL &&
            block1_observed == expected_block1_load &&
            block2_observed == 0ULL &&
            marker_observed == marker_updates[round]
        );

        TEST_SETUP_EXCEPT();
        AI_ASSERT_NO_EXCEPT_AND("after the three-generation store CMO probe, block0 keeps only middle and younger stores, block1 preserves seed, block2 is zero, and guards remain intact",
            ld(block0_older_store_vaddr) == 0ULL &&
            ld(block0_middle_store_vaddr) ==
                block0_middle_store_values[round] &&
            ld(block0_younger_store_vaddr) ==
                block0_younger_store_values[round] &&
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
        TEST_ASSERT("backing memory after the three-generation store CMO probe keeps block0 zero except middle and younger stores, block1 seed payload, block2 zero, marker update, and all guards intact",
            ai_memblock_cmo_triple_block_zero_except_two_words(
                block0_paddr, block0_middle_store_word,
                block0_middle_store_values[round],
                block0_younger_store_word,
                block0_younger_store_values[round]) &&
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

bool ai_micro_hs_interleaved_three_block_store_inval_store_zero_store_cbom_post_zero_generation_corner()
{
    TEST_START();

    const enum test_page source_page = SWITCH1;
    const enum test_page marker_page = SWITCH2;
    const size_t block0_older_store_word = 1U;
    const size_t block0_middle_store_word = 4U;
    const size_t block0_younger_store_word = 6U;
    const size_t block0_zero_load_word = 2U;
    const size_t block1_load_word = 3U;
    const size_t block2_load_word = 5U;
    const uintptr_t block0_vaddr = hs_page_base(source_page) + 0x3100U;
    const uintptr_t block1_vaddr = hs_page_base(source_page) + 0x3180U;
    const uintptr_t block2_vaddr = hs_page_base(source_page) + 0x3200U;
    const uintptr_t block0_paddr = phys_page_base(source_page) + 0x3100U;
    const uintptr_t block1_paddr = phys_page_base(source_page) + 0x3180U;
    const uintptr_t block2_paddr = phys_page_base(source_page) + 0x3200U;
    const uintptr_t block0_older_store_vaddr =
        block0_vaddr + block0_older_store_word * sizeof(uint64_t);
    const uintptr_t block0_middle_store_vaddr =
        block0_vaddr + block0_middle_store_word * sizeof(uint64_t);
    const uintptr_t block0_younger_store_vaddr =
        block0_vaddr + block0_younger_store_word * sizeof(uint64_t);
    const uintptr_t block0_zero_load_vaddr =
        block0_vaddr + block0_zero_load_word * sizeof(uint64_t);
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
    const uintptr_t marker_vaddr = hs_page_base(marker_page) + 0x3100U;
    const uintptr_t marker_paddr = phys_page_base(marker_page) + 0x3100U;
    const uintptr_t marker_guard_vaddr = marker_vaddr + sizeof(uint64_t);
    const uintptr_t marker_guard_paddr = marker_paddr + sizeof(uint64_t);
    const uint64_t block0_seeds[AI_MEMBLOCK_CMO_TRIPLE_ROUNDS] = {
        0x0f1e2d3c4b5a6978ULL,
        0x102f3e4d5c6b7a89ULL,
        0x21403f5e6d7c8b9aULL,
    };
    const uint64_t block1_seeds[AI_MEMBLOCK_CMO_TRIPLE_ROUNDS] = {
        0x8796a5b4c3d2e1f0ULL,
        0x98a7b6c5d4e3f201ULL,
        0xa9b8c7d6e5f40312ULL,
    };
    const uint64_t block2_seeds[AI_MEMBLOCK_CMO_TRIPLE_ROUNDS] = {
        0x1827364554637281ULL,
        0x2938475665748392ULL,
        0x3a495867768594a3ULL,
    };
    const uint64_t block0_older_store_values[AI_MEMBLOCK_CMO_TRIPLE_ROUNDS] = {
        0xc1c2c3c4c5c6c7c8ULL,
        0xd1d2d3d4d5d6d7d8ULL,
        0xe1e2e3e4e5e6e7e8ULL,
    };
    const uint64_t block0_middle_store_values[AI_MEMBLOCK_CMO_TRIPLE_ROUNDS] = {
        0x191a1b1c1d1e1f20ULL,
        0x292a2b2c2d2e2f30ULL,
        0x393a3b3c3d3e3f40ULL,
    };
    const uint64_t block0_younger_store_values[AI_MEMBLOCK_CMO_TRIPLE_ROUNDS] = {
        0x8071625344352617ULL,
        0x9182736455463728ULL,
        0xa293847566574839ULL,
    };
    const uint64_t before_guards[AI_MEMBLOCK_CMO_TRIPLE_ROUNDS] = {
        0x5152535455565758ULL,
        0x6162636465666768ULL,
        0x7172737475767778ULL,
    };
    const uint64_t guard01_values[AI_MEMBLOCK_CMO_TRIPLE_ROUNDS] = {
        0x5857565554535251ULL,
        0x6867666564636261ULL,
        0x7877767574737271ULL,
    };
    const uint64_t guard12_values[AI_MEMBLOCK_CMO_TRIPLE_ROUNDS] = {
        0x1234567890abcdefULL,
        0x234567890abcdef1ULL,
        0x34567890abcdef12ULL,
    };
    const uint64_t after_guards[AI_MEMBLOCK_CMO_TRIPLE_ROUNDS] = {
        0xfedcba0987654321ULL,
        0xedcba09876543210ULL,
        0xdcba09876543210fULL,
    };
    const uint64_t marker_seeds[AI_MEMBLOCK_CMO_TRIPLE_ROUNDS] = {
        0xc0ffee0011223344ULL,
        0xd1ffee1122334455ULL,
        0xe2ffee2233445566ULL,
    };
    const uint64_t marker_updates[AI_MEMBLOCK_CMO_TRIPLE_ROUNDS] = {
        0x4433221100eeffc0ULL,
        0x5544332211eeffd1ULL,
        0x6655443322eeffe2ULL,
    };
    const uint64_t marker_guards[AI_MEMBLOCK_CMO_TRIPLE_ROUNDS] = {
        0x0badf00d12345678ULL,
        0x1badf00d23456789ULL,
        0x2badf00d3456789aULL,
    };

    ai_memblock_cmo_triple_prepare_hs_env();

    for (size_t round = 0; round < AI_MEMBLOCK_CMO_TRIPLE_ROUNDS; ++round) {
        uint64_t block0_older_observed = 0ULL;
        uint64_t block0_middle_observed = 0ULL;
        uint64_t block0_younger_observed = 0ULL;
        uint64_t block0_zero_observed = 0ULL;
        uint64_t block1_observed = 0ULL;
        uint64_t block2_observed = 0ULL;
        uint64_t marker_observed = 0ULL;
        const uint64_t expected_block0_older_before =
            block0_seeds[round] +
            block0_older_store_word * 0x1111111111111111ULL;
        const uint64_t expected_block0_middle_before =
            block0_seeds[round] +
            block0_middle_store_word * 0x1111111111111111ULL;
        const uint64_t expected_block0_younger_before =
            block0_seeds[round] +
            block0_younger_store_word * 0x1111111111111111ULL;
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
        AI_ASSERT_NO_EXCEPT_AND("before the post-zero generation probe starts, all source blocks, guards, and marker are readable",
            ld(block0_vaddr) == block0_seeds[round] &&
            ld(block0_older_store_vaddr) == expected_block0_older_before &&
            ld(block0_middle_store_vaddr) == expected_block0_middle_before &&
            ld(block0_younger_store_vaddr) ==
                expected_block0_younger_before &&
            ld(block1_vaddr) == block1_seeds[round] &&
            ld(block2_vaddr) == block2_seeds[round] &&
            ld(block2_load_vaddr) == expected_block2_load &&
            ld(marker_vaddr) == marker_seeds[round]
        );

        TEST_SETUP_EXCEPT();
        ai_memblock_cmo_triple_three_block_store_inval_store_zero_store_cbom_window(
            block0_vaddr, block1_vaddr, block2_vaddr, marker_vaddr,
            block0_older_store_values[round],
            block0_middle_store_values[round],
            block0_younger_store_values[round],
            marker_updates[round], &block0_older_observed,
            &block0_middle_observed, &block0_younger_observed,
            &block0_zero_observed, &block1_observed, &block2_observed,
            &marker_observed);
        AI_ASSERT_NO_EXCEPT_AND("under three-block CMO pressure, CBO.ZERO clears older and middle generations while the post-zero younger generation survives CBOM",
            block0_older_observed == 0ULL &&
            block0_middle_observed == 0ULL &&
            block0_younger_observed == block0_younger_store_values[round] &&
            block0_zero_observed == 0ULL &&
            block1_observed == 0ULL &&
            block2_observed == expected_block2_load &&
            marker_observed == marker_updates[round]
        );

        TEST_SETUP_EXCEPT();
        AI_ASSERT_NO_EXCEPT_AND("after the post-zero generation probe, block0 keeps only the younger store, block1 is zero, block2 preserves seed, and guards remain intact",
            ld(block0_older_store_vaddr) == 0ULL &&
            ld(block0_middle_store_vaddr) == 0ULL &&
            ld(block0_younger_store_vaddr) ==
                block0_younger_store_values[round] &&
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
        TEST_ASSERT("backing memory after the post-zero generation probe keeps block0 zero except the younger store, block1 zero, block2 seed payload, marker update, and all guards intact",
            ai_memblock_cmo_triple_block_zero_except_word(
                block0_paddr, block0_younger_store_word,
                block0_younger_store_values[round]) &&
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

bool ai_micro_hs_interleaved_three_block_store_cbom_store_zero_store_inval_post_clean_generation_corner()
{
    TEST_START();

    const enum test_page source_page = SWITCH1;
    const enum test_page marker_page = SWITCH2;
    const size_t block0_older_store_word = 1U;
    const size_t block0_middle_store_word = 4U;
    const size_t block0_younger_store_word = 6U;
    const size_t block0_zero_load_word = 2U;
    const size_t block1_load_word = 3U;
    const size_t block2_load_word = 5U;
    const uintptr_t block0_vaddr = hs_page_base(source_page) + 0x3300U;
    const uintptr_t block1_vaddr = hs_page_base(source_page) + 0x3380U;
    const uintptr_t block2_vaddr = hs_page_base(source_page) + 0x3400U;
    const uintptr_t block0_paddr = phys_page_base(source_page) + 0x3300U;
    const uintptr_t block1_paddr = phys_page_base(source_page) + 0x3380U;
    const uintptr_t block2_paddr = phys_page_base(source_page) + 0x3400U;
    const uintptr_t block0_older_store_vaddr =
        block0_vaddr + block0_older_store_word * sizeof(uint64_t);
    const uintptr_t block0_middle_store_vaddr =
        block0_vaddr + block0_middle_store_word * sizeof(uint64_t);
    const uintptr_t block0_younger_store_vaddr =
        block0_vaddr + block0_younger_store_word * sizeof(uint64_t);
    const uintptr_t block0_zero_load_vaddr =
        block0_vaddr + block0_zero_load_word * sizeof(uint64_t);
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
    const uintptr_t marker_vaddr = hs_page_base(marker_page) + 0x3300U;
    const uintptr_t marker_paddr = phys_page_base(marker_page) + 0x3300U;
    const uintptr_t marker_guard_vaddr = marker_vaddr + sizeof(uint64_t);
    const uintptr_t marker_guard_paddr = marker_paddr + sizeof(uint64_t);
    const uint64_t block0_seeds[AI_MEMBLOCK_CMO_TRIPLE_ROUNDS] = {
        0x31425364758697a8ULL,
        0x425364758697a8b9ULL,
        0x5364758697a8b9caULL,
    };
    const uint64_t block1_seeds[AI_MEMBLOCK_CMO_TRIPLE_ROUNDS] = {
        0xa897867564534231ULL,
        0xb9a8978675645342ULL,
        0xcab9a89786756453ULL,
    };
    const uint64_t block2_seeds[AI_MEMBLOCK_CMO_TRIPLE_ROUNDS] = {
        0x0c1d2e3f40516273ULL,
        0x1d2e3f4051627384ULL,
        0x2e3f405162738495ULL,
    };
    const uint64_t block0_older_store_values[AI_MEMBLOCK_CMO_TRIPLE_ROUNDS] = {
        0xcacbcccdcecfd0d1ULL,
        0xdbdcdddedfe0e1e2ULL,
        0xecedeeeff0f1f2f3ULL,
    };
    const uint64_t block0_middle_store_values[AI_MEMBLOCK_CMO_TRIPLE_ROUNDS] = {
        0x4142434445464748ULL,
        0x5152535455565758ULL,
        0x6162636465666768ULL,
    };
    const uint64_t block0_younger_store_values[AI_MEMBLOCK_CMO_TRIPLE_ROUNDS] = {
        0x88796a5b4c3d2e1fULL,
        0x998a7b6c5d4e3f20ULL,
        0xaa9b8c7d6e5f4031ULL,
    };
    const uint64_t before_guards[AI_MEMBLOCK_CMO_TRIPLE_ROUNDS] = {
        0x012389ab4567cdefULL,
        0x12349abc5678def0ULL,
        0x2345abcd6789ef01ULL,
    };
    const uint64_t guard01_values[AI_MEMBLOCK_CMO_TRIPLE_ROUNDS] = {
        0xfedc7654ba982310ULL,
        0x0fed8765cba93421ULL,
        0x10fe9876dcba4532ULL,
    };
    const uint64_t guard12_values[AI_MEMBLOCK_CMO_TRIPLE_ROUNDS] = {
        0xabcdef0123456789ULL,
        0xbcdef0123456789aULL,
        0xcdef0123456789abULL,
    };
    const uint64_t after_guards[AI_MEMBLOCK_CMO_TRIPLE_ROUNDS] = {
        0x9876543210fedcbaULL,
        0xa9876543210fedcbULL,
        0xba9876543210fedcULL,
    };
    const uint64_t marker_seeds[AI_MEMBLOCK_CMO_TRIPLE_ROUNDS] = {
        0xfaceb00c01020304ULL,
        0x0aceb00d11121314ULL,
        0x1aceb00e21222324ULL,
    };
    const uint64_t marker_updates[AI_MEMBLOCK_CMO_TRIPLE_ROUNDS] = {
        0x040302010cb0cefaULL,
        0x141312110db0ce0aULL,
        0x242322210eb0ce1aULL,
    };
    const uint64_t marker_guards[AI_MEMBLOCK_CMO_TRIPLE_ROUNDS] = {
        0x55aa55aa11223344ULL,
        0x66bb66bb22334455ULL,
        0x77cc77cc33445566ULL,
    };

    ai_memblock_cmo_triple_prepare_hs_env();

    for (size_t round = 0; round < AI_MEMBLOCK_CMO_TRIPLE_ROUNDS; ++round) {
        uint64_t block0_older_observed = 0ULL;
        uint64_t block0_middle_observed = 0ULL;
        uint64_t block0_younger_observed = 0ULL;
        uint64_t block0_zero_observed = 0ULL;
        uint64_t block1_observed = 0ULL;
        uint64_t block2_observed = 0ULL;
        uint64_t marker_observed = 0ULL;
        const uint64_t expected_block0_older_before =
            block0_seeds[round] +
            block0_older_store_word * 0x1111111111111111ULL;
        const uint64_t expected_block0_middle_before =
            block0_seeds[round] +
            block0_middle_store_word * 0x1111111111111111ULL;
        const uint64_t expected_block0_younger_before =
            block0_seeds[round] +
            block0_younger_store_word * 0x1111111111111111ULL;
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
        AI_ASSERT_NO_EXCEPT_AND("before the post-clean zero generation probe starts, all source blocks, guards, and marker are readable",
            ld(block0_vaddr) == block0_seeds[round] &&
            ld(block0_older_store_vaddr) == expected_block0_older_before &&
            ld(block0_middle_store_vaddr) == expected_block0_middle_before &&
            ld(block0_younger_store_vaddr) ==
                expected_block0_younger_before &&
            ld(block1_vaddr) == block1_seeds[round] &&
            ld(block1_load_vaddr) == expected_block1_load &&
            ld(block2_vaddr) == block2_seeds[round] &&
            ld(marker_vaddr) == marker_seeds[round]
        );

        TEST_SETUP_EXCEPT();
        ai_memblock_cmo_triple_three_block_store_cbom_store_zero_store_inval_window(
            block0_vaddr, block1_vaddr, block2_vaddr, marker_vaddr,
            block0_older_store_values[round],
            block0_middle_store_values[round],
            block0_younger_store_values[round],
            marker_updates[round], &block0_older_observed,
            &block0_middle_observed, &block0_younger_observed,
            &block0_zero_observed, &block1_observed, &block2_observed,
            &marker_observed);
        AI_ASSERT_NO_EXCEPT_AND("under three-block CMO pressure, CBO.ZERO clears generations before it while the post-zero younger generation survives final CBO.INVAL",
            block0_older_observed == 0ULL &&
            block0_middle_observed == 0ULL &&
            block0_younger_observed == block0_younger_store_values[round] &&
            block0_zero_observed == 0ULL &&
            block1_observed == expected_block1_load &&
            block2_observed == 0ULL &&
            marker_observed == marker_updates[round]
        );

        TEST_SETUP_EXCEPT();
        AI_ASSERT_NO_EXCEPT_AND("after the post-clean zero generation probe, block0 keeps only the younger store, block1 preserves seed, block2 is zero, and guards remain intact",
            ld(block0_older_store_vaddr) == 0ULL &&
            ld(block0_middle_store_vaddr) == 0ULL &&
            ld(block0_younger_store_vaddr) ==
                block0_younger_store_values[round] &&
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
        TEST_ASSERT("backing memory after the post-clean zero generation probe keeps block0 zero except the younger store, block1 seed payload, block2 zero, marker update, and all guards intact",
            ai_memblock_cmo_triple_block_zero_except_word(
                block0_paddr, block0_younger_store_word,
                block0_younger_store_values[round]) &&
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

bool ai_micro_hs_interleaved_three_block_store_zero_store_cbom_store_inval_mid_clean_generation_corner()
{
    TEST_START();

    const enum test_page source_page = SWITCH1;
    const enum test_page marker_page = SWITCH2;
    const size_t block0_older_store_word = 1U;
    const size_t block0_middle_store_word = 4U;
    const size_t block0_younger_store_word = 6U;
    const size_t block0_zero_load_word = 2U;
    const size_t block1_load_word = 3U;
    const size_t block2_load_word = 5U;
    const uintptr_t block0_vaddr = hs_page_base(source_page) + 0x3500U;
    const uintptr_t block1_vaddr = hs_page_base(source_page) + 0x3580U;
    const uintptr_t block2_vaddr = hs_page_base(source_page) + 0x3600U;
    const uintptr_t block0_paddr = phys_page_base(source_page) + 0x3500U;
    const uintptr_t block1_paddr = phys_page_base(source_page) + 0x3580U;
    const uintptr_t block2_paddr = phys_page_base(source_page) + 0x3600U;
    const uintptr_t block0_older_store_vaddr =
        block0_vaddr + block0_older_store_word * sizeof(uint64_t);
    const uintptr_t block0_middle_store_vaddr =
        block0_vaddr + block0_middle_store_word * sizeof(uint64_t);
    const uintptr_t block0_younger_store_vaddr =
        block0_vaddr + block0_younger_store_word * sizeof(uint64_t);
    const uintptr_t block0_zero_load_vaddr =
        block0_vaddr + block0_zero_load_word * sizeof(uint64_t);
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
        0x05162738495a6b7cULL,
        0x162738495a6b7c8dULL,
        0x2738495a6b7c8d9eULL,
    };
    const uint64_t block1_seeds[AI_MEMBLOCK_CMO_TRIPLE_ROUNDS] = {
        0x8c7b6a5948372615ULL,
        0x9d8c7b6a59483726ULL,
        0xae9d8c7b6a594837ULL,
    };
    const uint64_t block2_seeds[AI_MEMBLOCK_CMO_TRIPLE_ROUNDS] = {
        0x2031425364758697ULL,
        0x31425364758697a8ULL,
        0x425364758697a8b9ULL,
    };
    const uint64_t block0_older_store_values[AI_MEMBLOCK_CMO_TRIPLE_ROUNDS] = {
        0xb1b2b3b4b5b6b7b8ULL,
        0xc1c2c3c4c5c6c7c8ULL,
        0xd1d2d3d4d5d6d7d8ULL,
    };
    const uint64_t block0_middle_store_values[AI_MEMBLOCK_CMO_TRIPLE_ROUNDS] = {
        0x2122232425262728ULL,
        0x3132333435363738ULL,
        0x4142434445464748ULL,
    };
    const uint64_t block0_younger_store_values[AI_MEMBLOCK_CMO_TRIPLE_ROUNDS] = {
        0x7081625344352617ULL,
        0x8192736455463728ULL,
        0x92a3847566574839ULL,
    };
    const uint64_t before_guards[AI_MEMBLOCK_CMO_TRIPLE_ROUNDS] = {
        0x13579bdf2468ace0ULL,
        0x2468ace03579bdf1ULL,
        0x3579bdf1468ace02ULL,
    };
    const uint64_t guard01_values[AI_MEMBLOCK_CMO_TRIPLE_ROUNDS] = {
        0x0eca8642fdb97531ULL,
        0x1fdb97530eca8642ULL,
        0x20eca8641fdb9753ULL,
    };
    const uint64_t guard12_values[AI_MEMBLOCK_CMO_TRIPLE_ROUNDS] = {
        0xaa55aa5501020304ULL,
        0xbb66bb6611121314ULL,
        0xcc77cc7721222324ULL,
    };
    const uint64_t after_guards[AI_MEMBLOCK_CMO_TRIPLE_ROUNDS] = {
        0x4030201055aa55aaULL,
        0x5040302066bb66bbULL,
        0x6050403077cc77ccULL,
    };
    const uint64_t marker_seeds[AI_MEMBLOCK_CMO_TRIPLE_ROUNDS] = {
        0xabcddcba01010101ULL,
        0xbcdeedcb02020202ULL,
        0xcdeffedc03030303ULL,
    };
    const uint64_t marker_updates[AI_MEMBLOCK_CMO_TRIPLE_ROUNDS] = {
        0x1010101bacddcbaULL,
        0x2020202cdeedcbbULL,
        0x3030303dceffedcULL,
    };
    const uint64_t marker_guards[AI_MEMBLOCK_CMO_TRIPLE_ROUNDS] = {
        0x1234abcd5678ef90ULL,
        0x2345bcde6789f0a1ULL,
        0x3456cdef7890a1b2ULL,
    };

    ai_memblock_cmo_triple_prepare_hs_env();

    for (size_t round = 0; round < AI_MEMBLOCK_CMO_TRIPLE_ROUNDS; ++round) {
        uint64_t block0_older_observed = 0ULL;
        uint64_t block0_middle_observed = 0ULL;
        uint64_t block0_younger_observed = 0ULL;
        uint64_t block0_zero_observed = 0ULL;
        uint64_t block1_observed = 0ULL;
        uint64_t block2_observed = 0ULL;
        uint64_t marker_observed = 0ULL;
        const uint64_t expected_block0_older_before =
            block0_seeds[round] +
            block0_older_store_word * 0x1111111111111111ULL;
        const uint64_t expected_block0_middle_before =
            block0_seeds[round] +
            block0_middle_store_word * 0x1111111111111111ULL;
        const uint64_t expected_block0_younger_before =
            block0_seeds[round] +
            block0_younger_store_word * 0x1111111111111111ULL;
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
        AI_ASSERT_NO_EXCEPT_AND("before the mid-clean generation probe starts, all source blocks, guards, and marker are readable",
            ld(block0_vaddr) == block0_seeds[round] &&
            ld(block0_older_store_vaddr) == expected_block0_older_before &&
            ld(block0_middle_store_vaddr) == expected_block0_middle_before &&
            ld(block0_younger_store_vaddr) ==
                expected_block0_younger_before &&
            ld(block1_vaddr) == block1_seeds[round] &&
            ld(block1_load_vaddr) == expected_block1_load &&
            ld(block2_vaddr) == block2_seeds[round] &&
            ld(marker_vaddr) == marker_seeds[round]
        );

        TEST_SETUP_EXCEPT();
        ai_memblock_cmo_triple_three_block_store_zero_store_cbom_store_inval_window(
            block0_vaddr, block1_vaddr, block2_vaddr, marker_vaddr,
            block0_older_store_values[round],
            block0_middle_store_values[round],
            block0_younger_store_values[round],
            marker_updates[round], &block0_older_observed,
            &block0_middle_observed, &block0_younger_observed,
            &block0_zero_observed, &block1_observed, &block2_observed,
            &marker_observed);
        AI_ASSERT_NO_EXCEPT_AND("under three-block CMO pressure, CBO.ZERO clears the older generation while middle and younger generations survive CBOM and final CBO.INVAL",
            block0_older_observed == 0ULL &&
            block0_middle_observed == block0_middle_store_values[round] &&
            block0_younger_observed == block0_younger_store_values[round] &&
            block0_zero_observed == 0ULL &&
            block1_observed == expected_block1_load &&
            block2_observed == 0ULL &&
            marker_observed == marker_updates[round]
        );

        TEST_SETUP_EXCEPT();
        AI_ASSERT_NO_EXCEPT_AND("after the mid-clean generation probe, block0 keeps middle and younger stores, block1 preserves seed, block2 is zero, and guards remain intact",
            ld(block0_older_store_vaddr) == 0ULL &&
            ld(block0_middle_store_vaddr) ==
                block0_middle_store_values[round] &&
            ld(block0_younger_store_vaddr) ==
                block0_younger_store_values[round] &&
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
        TEST_ASSERT("backing memory after the mid-clean generation probe keeps block0 zero except middle and younger stores, block1 seed payload, block2 zero, marker update, and all guards intact",
            ai_memblock_cmo_triple_block_zero_except_two_words(
                block0_paddr, block0_middle_store_word,
                block0_middle_store_values[round],
                block0_younger_store_word,
                block0_younger_store_values[round]) &&
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

bool ai_micro_hs_interleaved_three_block_store_inval_store_cbom_store_zero_final_zero_generation_corner()
{
    TEST_START();

    const enum test_page source_page = SWITCH1;
    const enum test_page marker_page = SWITCH2;
    const size_t block0_older_store_word = 1U;
    const size_t block0_middle_store_word = 4U;
    const size_t block0_younger_store_word = 6U;
    const size_t block0_zero_load_word = 2U;
    const size_t block1_load_word = 3U;
    const size_t block2_load_word = 5U;
    const uintptr_t block0_vaddr = hs_page_base(source_page) + 0x3700U;
    const uintptr_t block1_vaddr = hs_page_base(source_page) + 0x3780U;
    const uintptr_t block2_vaddr = hs_page_base(source_page) + 0x3800U;
    const uintptr_t block0_paddr = phys_page_base(source_page) + 0x3700U;
    const uintptr_t block1_paddr = phys_page_base(source_page) + 0x3780U;
    const uintptr_t block2_paddr = phys_page_base(source_page) + 0x3800U;
    const uintptr_t block0_older_store_vaddr =
        block0_vaddr + block0_older_store_word * sizeof(uint64_t);
    const uintptr_t block0_middle_store_vaddr =
        block0_vaddr + block0_middle_store_word * sizeof(uint64_t);
    const uintptr_t block0_younger_store_vaddr =
        block0_vaddr + block0_younger_store_word * sizeof(uint64_t);
    const uintptr_t block0_zero_load_vaddr =
        block0_vaddr + block0_zero_load_word * sizeof(uint64_t);
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
    const uintptr_t marker_vaddr = hs_page_base(marker_page) + 0x3700U;
    const uintptr_t marker_paddr = phys_page_base(marker_page) + 0x3700U;
    const uintptr_t marker_guard_vaddr = marker_vaddr + sizeof(uint64_t);
    const uintptr_t marker_guard_paddr = marker_paddr + sizeof(uint64_t);
    const uint64_t block0_seeds[AI_MEMBLOCK_CMO_TRIPLE_ROUNDS] = {
        0x60718293a4b5c6d7ULL,
        0x718293a4b5c6d7e8ULL,
        0x8293a4b5c6d7e8f9ULL,
    };
    const uint64_t block1_seeds[AI_MEMBLOCK_CMO_TRIPLE_ROUNDS] = {
        0xd7c6b5a493827160ULL,
        0xe8d7c6b5a4938271ULL,
        0xf9e8d7c6b5a49382ULL,
    };
    const uint64_t block2_seeds[AI_MEMBLOCK_CMO_TRIPLE_ROUNDS] = {
        0x3141592653589793ULL,
        0x42616a375469a8a4ULL,
        0x53727b48657ab9b5ULL,
    };
    const uint64_t block0_older_store_values[AI_MEMBLOCK_CMO_TRIPLE_ROUNDS] = {
        0xababcdcdefef0101ULL,
        0xbcbcdededfdf1212ULL,
        0xcdcdefeff0f02323ULL,
    };
    const uint64_t block0_middle_store_values[AI_MEMBLOCK_CMO_TRIPLE_ROUNDS] = {
        0x3031323334353637ULL,
        0x4041424344454647ULL,
        0x5051525354555657ULL,
    };
    const uint64_t block0_younger_store_values[AI_MEMBLOCK_CMO_TRIPLE_ROUNDS] = {
        0x76540123fedcba98ULL,
        0x87651234efcdab89ULL,
        0x98762345dcba9876ULL,
    };
    const uint64_t before_guards[AI_MEMBLOCK_CMO_TRIPLE_ROUNDS] = {
        0x1111222233334444ULL,
        0x2222333344445555ULL,
        0x3333444455556666ULL,
    };
    const uint64_t guard01_values[AI_MEMBLOCK_CMO_TRIPLE_ROUNDS] = {
        0x4444333322221111ULL,
        0x5555444433332222ULL,
        0x6666555544443333ULL,
    };
    const uint64_t guard12_values[AI_MEMBLOCK_CMO_TRIPLE_ROUNDS] = {
        0x0123fedc4567ba98ULL,
        0x1234edcb5678ab89ULL,
        0x2345dcba67899a78ULL,
    };
    const uint64_t after_guards[AI_MEMBLOCK_CMO_TRIPLE_ROUNDS] = {
        0x89ab4567cdef0123ULL,
        0x9abc5678def01234ULL,
        0xabcd6789ef012345ULL,
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
        0xf0f1f2f3f4f5f6f7ULL,
        0xe0e1e2e3e4e5e6e7ULL,
        0xd0d1d2d3d4d5d6d7ULL,
    };

    ai_memblock_cmo_triple_prepare_hs_env();

    for (size_t round = 0; round < AI_MEMBLOCK_CMO_TRIPLE_ROUNDS; ++round) {
        uint64_t block0_older_observed = 0ULL;
        uint64_t block0_middle_observed = 0ULL;
        uint64_t block0_younger_observed = 0ULL;
        uint64_t block0_zero_observed = 0ULL;
        uint64_t block1_observed = 0ULL;
        uint64_t block2_observed = 0ULL;
        uint64_t marker_observed = 0ULL;
        const uint64_t expected_block0_older_before =
            block0_seeds[round] +
            block0_older_store_word * 0x1111111111111111ULL;
        const uint64_t expected_block0_middle_before =
            block0_seeds[round] +
            block0_middle_store_word * 0x1111111111111111ULL;
        const uint64_t expected_block0_younger_before =
            block0_seeds[round] +
            block0_younger_store_word * 0x1111111111111111ULL;
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
        AI_ASSERT_NO_EXCEPT_AND("before the final-zero generation probe starts, all source blocks, guards, and marker are readable",
            ld(block0_vaddr) == block0_seeds[round] &&
            ld(block0_older_store_vaddr) == expected_block0_older_before &&
            ld(block0_middle_store_vaddr) == expected_block0_middle_before &&
            ld(block0_younger_store_vaddr) ==
                expected_block0_younger_before &&
            ld(block1_vaddr) == block1_seeds[round] &&
            ld(block2_vaddr) == block2_seeds[round] &&
            ld(block2_load_vaddr) == expected_block2_load &&
            ld(marker_vaddr) == marker_seeds[round]
        );

        TEST_SETUP_EXCEPT();
        ai_memblock_cmo_triple_three_block_store_inval_store_cbom_store_zero_window(
            block0_vaddr, block1_vaddr, block2_vaddr, marker_vaddr,
            block0_older_store_values[round],
            block0_middle_store_values[round],
            block0_younger_store_values[round],
            marker_updates[round], &block0_older_observed,
            &block0_middle_observed, &block0_younger_observed,
            &block0_zero_observed, &block1_observed, &block2_observed,
            &marker_observed);
        AI_ASSERT_NO_EXCEPT_AND("under three-block CMO pressure, final CBO.ZERO clears older, middle, and younger generations after CBO.INVAL and CBOM activity",
            block0_older_observed == 0ULL &&
            block0_middle_observed == 0ULL &&
            block0_younger_observed == 0ULL &&
            block0_zero_observed == 0ULL &&
            block1_observed == 0ULL &&
            block2_observed == expected_block2_load &&
            marker_observed == marker_updates[round]
        );

        TEST_SETUP_EXCEPT();
        AI_ASSERT_NO_EXCEPT_AND("after the final-zero generation probe, block0 and block1 are zero, block2 preserves seed, and guards remain intact",
            ld(block0_older_store_vaddr) == 0ULL &&
            ld(block0_middle_store_vaddr) == 0ULL &&
            ld(block0_younger_store_vaddr) == 0ULL &&
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
        TEST_ASSERT("backing memory after the final-zero generation probe keeps block0 and block1 fully zero, block2 seed payload, marker update, and all guards intact",
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

bool ai_micro_hs_interleaved_three_block_store_cbom_store_inval_store_zero_final_zero_generation_corner()
{
    TEST_START();

    const enum test_page source_page = SWITCH1;
    const enum test_page marker_page = SWITCH2;
    const size_t block0_older_store_word = 1U;
    const size_t block0_middle_store_word = 4U;
    const size_t block0_younger_store_word = 6U;
    const size_t block0_zero_load_word = 2U;
    const size_t block1_load_word = 3U;
    const size_t block2_load_word = 5U;
    const uintptr_t block0_vaddr = hs_page_base(source_page) + 0x3900U;
    const uintptr_t block1_vaddr = hs_page_base(source_page) + 0x3980U;
    const uintptr_t block2_vaddr = hs_page_base(source_page) + 0x3a00U;
    const uintptr_t block0_paddr = phys_page_base(source_page) + 0x3900U;
    const uintptr_t block1_paddr = phys_page_base(source_page) + 0x3980U;
    const uintptr_t block2_paddr = phys_page_base(source_page) + 0x3a00U;
    const uintptr_t block0_older_store_vaddr =
        block0_vaddr + block0_older_store_word * sizeof(uint64_t);
    const uintptr_t block0_middle_store_vaddr =
        block0_vaddr + block0_middle_store_word * sizeof(uint64_t);
    const uintptr_t block0_younger_store_vaddr =
        block0_vaddr + block0_younger_store_word * sizeof(uint64_t);
    const uintptr_t block0_zero_load_vaddr =
        block0_vaddr + block0_zero_load_word * sizeof(uint64_t);
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
    const uintptr_t marker_vaddr = hs_page_base(marker_page) + 0x3900U;
    const uintptr_t marker_paddr = phys_page_base(marker_page) + 0x3900U;
    const uintptr_t marker_guard_vaddr = marker_vaddr + sizeof(uint64_t);
    const uintptr_t marker_guard_paddr = marker_paddr + sizeof(uint64_t);
    const uint64_t block0_seeds[AI_MEMBLOCK_CMO_TRIPLE_ROUNDS] = {
        0x6d7e8f9012345678ULL,
        0x7e8f90123456789aULL,
        0x8f90123456789abcULL,
    };
    const uint64_t block1_seeds[AI_MEMBLOCK_CMO_TRIPLE_ROUNDS] = {
        0x8765432109f8e7d6ULL,
        0x9876543210f9e8d7ULL,
        0xa9876543210af9e8ULL,
    };
    const uint64_t block2_seeds[AI_MEMBLOCK_CMO_TRIPLE_ROUNDS] = {
        0x2468ace013579bdfULL,
        0x3579bdf12468ace0ULL,
        0x468ace023579bdf1ULL,
    };
    const uint64_t block0_older_store_values[AI_MEMBLOCK_CMO_TRIPLE_ROUNDS] = {
        0xa1b2c3d4e5f60718ULL,
        0xb2c3d4e5f6071829ULL,
        0xc3d4e5f60718293aULL,
    };
    const uint64_t block0_middle_store_values[AI_MEMBLOCK_CMO_TRIPLE_ROUNDS] = {
        0x18293a4b5c6d7e8fULL,
        0x293a4b5c6d7e8f90ULL,
        0x3a4b5c6d7e8f9012ULL,
    };
    const uint64_t block0_younger_store_values[AI_MEMBLOCK_CMO_TRIPLE_ROUNDS] = {
        0xfedc012389ab4567ULL,
        0xedcb12349abc5678ULL,
        0xdcba2345abcd6789ULL,
    };
    const uint64_t before_guards[AI_MEMBLOCK_CMO_TRIPLE_ROUNDS] = {
        0x0102030405060708ULL,
        0x1112131415161718ULL,
        0x2122232425262728ULL,
    };
    const uint64_t guard01_values[AI_MEMBLOCK_CMO_TRIPLE_ROUNDS] = {
        0x0807060504030201ULL,
        0x1817161514131211ULL,
        0x2827262524232221ULL,
    };
    const uint64_t guard12_values[AI_MEMBLOCK_CMO_TRIPLE_ROUNDS] = {
        0x5a5b5c5d5e5f6061ULL,
        0x6a6b6c6d6e6f7071ULL,
        0x7a7b7c7d7e7f8081ULL,
    };
    const uint64_t after_guards[AI_MEMBLOCK_CMO_TRIPLE_ROUNDS] = {
        0x161514131211100fULL,
        0x2726252423222120ULL,
        0x3837363534333231ULL,
    };
    const uint64_t marker_seeds[AI_MEMBLOCK_CMO_TRIPLE_ROUNDS] = {
        0x900df00d01020304ULL,
        0xa00ef00e11121314ULL,
        0xb00ff00f21222324ULL,
    };
    const uint64_t marker_updates[AI_MEMBLOCK_CMO_TRIPLE_ROUNDS] = {
        0x040302010df00d90ULL,
        0x141312110ef00ea0ULL,
        0x242322210ff00fb0ULL,
    };
    const uint64_t marker_guards[AI_MEMBLOCK_CMO_TRIPLE_ROUNDS] = {
        0xcafefeed11223344ULL,
        0xdb0ffeee22334455ULL,
        0xec20ffef33445566ULL,
    };

    ai_memblock_cmo_triple_prepare_hs_env();

    for (size_t round = 0; round < AI_MEMBLOCK_CMO_TRIPLE_ROUNDS; ++round) {
        uint64_t block0_older_observed = 0ULL;
        uint64_t block0_middle_observed = 0ULL;
        uint64_t block0_younger_observed = 0ULL;
        uint64_t block0_zero_observed = 0ULL;
        uint64_t block1_observed = 0ULL;
        uint64_t block2_observed = 0ULL;
        uint64_t marker_observed = 0ULL;
        const uint64_t expected_block0_older_before =
            block0_seeds[round] +
            block0_older_store_word * 0x1111111111111111ULL;
        const uint64_t expected_block0_middle_before =
            block0_seeds[round] +
            block0_middle_store_word * 0x1111111111111111ULL;
        const uint64_t expected_block0_younger_before =
            block0_seeds[round] +
            block0_younger_store_word * 0x1111111111111111ULL;
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
        AI_ASSERT_NO_EXCEPT_AND("before the clean-inval final-zero generation probe starts, all source blocks, guards, and marker are readable",
            ld(block0_vaddr) == block0_seeds[round] &&
            ld(block0_older_store_vaddr) == expected_block0_older_before &&
            ld(block0_middle_store_vaddr) == expected_block0_middle_before &&
            ld(block0_younger_store_vaddr) ==
                expected_block0_younger_before &&
            ld(block1_vaddr) == block1_seeds[round] &&
            ld(block2_vaddr) == block2_seeds[round] &&
            ld(block2_load_vaddr) == expected_block2_load &&
            ld(marker_vaddr) == marker_seeds[round]
        );

        TEST_SETUP_EXCEPT();
        ai_memblock_cmo_triple_three_block_store_cbom_store_inval_store_zero_window(
            block0_vaddr, block1_vaddr, block2_vaddr, marker_vaddr,
            block0_older_store_values[round],
            block0_middle_store_values[round],
            block0_younger_store_values[round],
            marker_updates[round], &block0_older_observed,
            &block0_middle_observed, &block0_younger_observed,
            &block0_zero_observed, &block1_observed, &block2_observed,
            &marker_observed);
        AI_ASSERT_NO_EXCEPT_AND("under three-block CMO pressure, final CBO.ZERO clears three store generations after CBOM and CBO.INVAL activity",
            block0_older_observed == 0ULL &&
            block0_middle_observed == 0ULL &&
            block0_younger_observed == 0ULL &&
            block0_zero_observed == 0ULL &&
            block1_observed == 0ULL &&
            block2_observed == expected_block2_load &&
            marker_observed == marker_updates[round]
        );

        TEST_SETUP_EXCEPT();
        AI_ASSERT_NO_EXCEPT_AND("after the clean-inval final-zero generation probe, block0 and block1 are zero, block2 preserves seed, and guards remain intact",
            ld(block0_older_store_vaddr) == 0ULL &&
            ld(block0_middle_store_vaddr) == 0ULL &&
            ld(block0_younger_store_vaddr) == 0ULL &&
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
        TEST_ASSERT("backing memory after the clean-inval final-zero generation probe keeps block0 and block1 fully zero, block2 seed payload, marker update, and all guards intact",
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

bool ai_micro_hs_interleaved_three_block_store_zero_store_inval_store_cbom_store_inval_four_generation_corner()
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
    const uintptr_t block0_vaddr = hs_page_base(source_page) + 0x3b00U;
    const uintptr_t block1_vaddr = hs_page_base(source_page) + 0x3b80U;
    const uintptr_t block2_vaddr = hs_page_base(source_page) + 0x3c00U;
    const uintptr_t block0_paddr = phys_page_base(source_page) + 0x3b00U;
    const uintptr_t block1_paddr = phys_page_base(source_page) + 0x3b80U;
    const uintptr_t block2_paddr = phys_page_base(source_page) + 0x3c00U;
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
    const uintptr_t marker_vaddr = hs_page_base(marker_page) + 0x3b00U;
    const uintptr_t marker_paddr = phys_page_base(marker_page) + 0x3b00U;
    const uintptr_t marker_guard_vaddr = marker_vaddr + sizeof(uint64_t);
    const uintptr_t marker_guard_paddr = marker_paddr + sizeof(uint64_t);
    const uint64_t block0_seeds[AI_MEMBLOCK_CMO_TRIPLE_ROUNDS] = {
        0x1112131415161718ULL,
        0x2122232425262728ULL,
        0x3132333435363738ULL,
    };
    const uint64_t block1_seeds[AI_MEMBLOCK_CMO_TRIPLE_ROUNDS] = {
        0x8171615141312111ULL,
        0x9282726252423221ULL,
        0xa393837363534331ULL,
    };
    const uint64_t block2_seeds[AI_MEMBLOCK_CMO_TRIPLE_ROUNDS] = {
        0x0d1e2f3041526374ULL,
        0x1e2f304152637485ULL,
        0x2f30415263748596ULL,
    };
    const uint64_t block0_older_store_values[AI_MEMBLOCK_CMO_TRIPLE_ROUNDS] = {
        0xb0b1b2b3b4b5b6b7ULL,
        0xc0c1c2c3c4c5c6c7ULL,
        0xd0d1d2d3d4d5d6d7ULL,
    };
    const uint64_t block0_second_store_values[AI_MEMBLOCK_CMO_TRIPLE_ROUNDS] = {
        0x4041424344454647ULL,
        0x5051525354555657ULL,
        0x6061626364656667ULL,
    };
    const uint64_t block0_third_store_values[AI_MEMBLOCK_CMO_TRIPLE_ROUNDS] = {
        0x8877665544332211ULL,
        0x9988776655443322ULL,
        0xaa99887766554433ULL,
    };
    const uint64_t block0_fourth_store_values[AI_MEMBLOCK_CMO_TRIPLE_ROUNDS] = {
        0x192a3b4c5d6e7f80ULL,
        0x2a3b4c5d6e7f8091ULL,
        0x3b4c5d6e7f8091a2ULL,
    };
    const uint64_t before_guards[AI_MEMBLOCK_CMO_TRIPLE_ROUNDS] = {
        0xf1e2d3c4b5a69788ULL,
        0xe2d3c4b5a6978879ULL,
        0xd3c4b5a69788796aULL,
    };
    const uint64_t guard01_values[AI_MEMBLOCK_CMO_TRIPLE_ROUNDS] = {
        0x88796a5b4c3d2e1fULL,
        0x998a7b6c5d4e3f20ULL,
        0xaa9b8c7d6e5f4031ULL,
    };
    const uint64_t guard12_values[AI_MEMBLOCK_CMO_TRIPLE_ROUNDS] = {
        0x1234fedc5678ba90ULL,
        0x2345edcb6789ab01ULL,
        0x3456dcba789abc12ULL,
    };
    const uint64_t after_guards[AI_MEMBLOCK_CMO_TRIPLE_ROUNDS] = {
        0x90ba7856dcfe3412ULL,
        0x01ab8967cbed4523ULL,
        0x12bc9a78badc5634ULL,
    };
    const uint64_t marker_seeds[AI_MEMBLOCK_CMO_TRIPLE_ROUNDS] = {
        0x1122aabb3344ccddULL,
        0x2233bbcc4455ddeeULL,
        0x3344ccdd5566eeffULL,
    };
    const uint64_t marker_updates[AI_MEMBLOCK_CMO_TRIPLE_ROUNDS] = {
        0xddcc4433bbaa2211ULL,
        0xeedd5544ccbb3322ULL,
        0xffee6655ddcc4433ULL,
    };
    const uint64_t marker_guards[AI_MEMBLOCK_CMO_TRIPLE_ROUNDS] = {
        0x0f1e2d3c4b5a6978ULL,
        0x102f3e4d5c6b7a89ULL,
        0x21403f5e6d7c8b9aULL,
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
        AI_ASSERT_NO_EXCEPT_AND("before the four-generation CMO probe starts, all source blocks, guards, and marker are readable",
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
        ai_memblock_cmo_triple_three_block_store_zero_store_inval_store_cbom_store_inval_window(
            block0_vaddr, block1_vaddr, block2_vaddr, marker_vaddr,
            block0_older_store_values[round],
            block0_second_store_values[round],
            block0_third_store_values[round],
            block0_fourth_store_values[round], marker_updates[round],
            &block0_older_observed, &block0_second_observed,
            &block0_third_observed, &block0_fourth_observed,
            &block0_zero_observed, &block1_observed, &block2_observed,
            &marker_observed);
        AI_ASSERT_NO_EXCEPT_AND("under three-block CMO pressure, the first CBO.ZERO clears only the older generation while later three store generations survive CBO.INVAL, CBOM, and final CBO.INVAL",
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
        AI_ASSERT_NO_EXCEPT_AND("after the four-generation CMO probe, block0 keeps only post-zero stores, block1 preserves seed, block2 is zero, and guards remain intact",
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
        TEST_ASSERT("backing memory after the four-generation CMO probe keeps block0 zero except the three post-zero stores, block1 seed payload, block2 zero, marker update, and all guards intact",
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

bool ai_micro_hs_interleaved_three_block_store_inval_store_cbom_store_inval_store_zero_four_generation_final_zero_corner()
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
        0x13579bdf2468ace0ULL,
        0x2468ace03579bdf1ULL,
        0x3579bdf1468ace02ULL,
    };
    const uint64_t block0_older_store_values[AI_MEMBLOCK_CMO_TRIPLE_ROUNDS] = {
        0x0a1b2c3d4e5f6071ULL,
        0x1b2c3d4e5f607182ULL,
        0x2c3d4e5f60718293ULL,
    };
    const uint64_t block0_second_store_values[AI_MEMBLOCK_CMO_TRIPLE_ROUNDS] = {
        0x718293a4b5c6d7e8ULL,
        0x8293a4b5c6d7e8f9ULL,
        0x93a4b5c6d7e8f90aULL,
    };
    const uint64_t block0_third_store_values[AI_MEMBLOCK_CMO_TRIPLE_ROUNDS] = {
        0xf1e0d3c2b5a49786ULL,
        0xe2d1c4b3a6978875ULL,
        0xd3c2b5a497867764ULL,
    };
    const uint64_t block0_fourth_store_values[AI_MEMBLOCK_CMO_TRIPLE_ROUNDS] = {
        0x66778899aabbccddULL,
        0x778899aabbccddeeULL,
        0x8899aabbccddeeffULL,
    };
    const uint64_t before_guards[AI_MEMBLOCK_CMO_TRIPLE_ROUNDS] = {
        0x1021324354657687ULL,
        0x2132435465768798ULL,
        0x32435465768798a9ULL,
    };
    const uint64_t guard01_values[AI_MEMBLOCK_CMO_TRIPLE_ROUNDS] = {
        0x8776655443322110ULL,
        0x9887766554433221ULL,
        0xa998877665544332ULL,
    };
    const uint64_t guard12_values[AI_MEMBLOCK_CMO_TRIPLE_ROUNDS] = {
        0x5e4d3c2b1a091827ULL,
        0x6f5e4d3c2b1a2938ULL,
        0x706f5e4d3c2b3a49ULL,
    };
    const uint64_t after_guards[AI_MEMBLOCK_CMO_TRIPLE_ROUNDS] = {
        0x2718091a2b3c4d5eULL,
        0x38291a2b3c4d5e6fULL,
        0x493a2b3c4d5e6f70ULL,
    };
    const uint64_t marker_seeds[AI_MEMBLOCK_CMO_TRIPLE_ROUNDS] = {
        0x55aa11bb22cc33ddULL,
        0x66bb22cc33dd44eeULL,
        0x77cc33dd44ee55ffULL,
    };
    const uint64_t marker_updates[AI_MEMBLOCK_CMO_TRIPLE_ROUNDS] = {
        0xdd33cc22bb11aa55ULL,
        0xee44dd33cc22bb66ULL,
        0xff55ee44dd33cc77ULL,
    };
    const uint64_t marker_guards[AI_MEMBLOCK_CMO_TRIPLE_ROUNDS] = {
        0xabcdef0123456789ULL,
        0xbcdef0123456789aULL,
        0xcdef0123456789abULL,
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
        AI_ASSERT_NO_EXCEPT_AND("before the four-generation final-zero CMO probe starts, all source blocks, guards, and marker are readable",
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
        ai_memblock_cmo_triple_three_block_store_inval_store_cbom_store_inval_store_zero_window(
            block0_vaddr, block1_vaddr, block2_vaddr, marker_vaddr,
            block0_older_store_values[round],
            block0_second_store_values[round],
            block0_third_store_values[round],
            block0_fourth_store_values[round], marker_updates[round],
            &block0_older_observed, &block0_second_observed,
            &block0_third_observed, &block0_fourth_observed,
            &block0_zero_observed, &block1_observed, &block2_observed,
            &marker_observed);
        AI_ASSERT_NO_EXCEPT_AND("under three-block CMO pressure, final CBO.ZERO clears four same-block store generations after CBO.INVAL, CBOM, and CBO.INVAL activity",
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
        AI_ASSERT_NO_EXCEPT_AND("after the four-generation final-zero CMO probe, block0 and block1 are zero, block2 preserves seed, and guards remain intact",
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
        TEST_ASSERT("backing memory after the four-generation final-zero CMO probe keeps block0 and block1 fully zero, block2 seed payload, marker update, and all guards intact",
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

bool ai_micro_hs_interleaved_three_block_store_cbom_store_zero_store_inval_store_cbom_four_generation_mid_zero_corner()
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
        0x0f1e2d3c4b5a6978ULL,
        0x102f3e4d5c6b7a89ULL,
        0x21403f5e6d7c8b9aULL,
    };
    const uint64_t block1_seeds[AI_MEMBLOCK_CMO_TRIPLE_ROUNDS] = {
        0x78695a4b3c2d1e0fULL,
        0x897a6b5c4d3e2f10ULL,
        0x9a8b7c6d5e4f3021ULL,
    };
    const uint64_t block2_seeds[AI_MEMBLOCK_CMO_TRIPLE_ROUNDS] = {
        0x1123581321345589ULL,
        0x223469243245669aULL,
        0x33457a35435677abULL,
    };
    const uint64_t block0_older_store_values[AI_MEMBLOCK_CMO_TRIPLE_ROUNDS] = {
        0xa55a010203040506ULL,
        0xb66b111213141516ULL,
        0xc77c212223242526ULL,
    };
    const uint64_t block0_second_store_values[AI_MEMBLOCK_CMO_TRIPLE_ROUNDS] = {
        0x0605040302015aa5ULL,
        0x1615141312116bb6ULL,
        0x2625242322217cc7ULL,
    };
    const uint64_t block0_third_store_values[AI_MEMBLOCK_CMO_TRIPLE_ROUNDS] = {
        0xd1e2f30415263748ULL,
        0xe2f3041526374859ULL,
        0xf30415263748596aULL,
    };
    const uint64_t block0_fourth_store_values[AI_MEMBLOCK_CMO_TRIPLE_ROUNDS] = {
        0x4837261504f3e2d1ULL,
        0x594837261504f3e2ULL,
        0x6a594837261504f3ULL,
    };
    const uint64_t before_guards[AI_MEMBLOCK_CMO_TRIPLE_ROUNDS] = {
        0x7f6e5d4c3b2a1908ULL,
        0x8f7e6d5c4b3a2918ULL,
        0x9f8e7d6c5b4a3928ULL,
    };
    const uint64_t guard01_values[AI_MEMBLOCK_CMO_TRIPLE_ROUNDS] = {
        0x08192a3b4c5d6e7fULL,
        0x18293a4b5c6d7e8fULL,
        0x28394a5b6c7d8e9fULL,
    };
    const uint64_t guard12_values[AI_MEMBLOCK_CMO_TRIPLE_ROUNDS] = {
        0xca11ab22bc33cd44ULL,
        0xdb22bc33cd44de55ULL,
        0xec33cd44de55ef66ULL,
    };
    const uint64_t after_guards[AI_MEMBLOCK_CMO_TRIPLE_ROUNDS] = {
        0x44cd33bc22ab11caULL,
        0x55de44cd33bc22dbULL,
        0x66ef55de44cd33ecULL,
    };
    const uint64_t marker_seeds[AI_MEMBLOCK_CMO_TRIPLE_ROUNDS] = {
        0x13572468ace0bdf9ULL,
        0x24683579bdf1ce0aULL,
        0x3579468ace02df1bULL,
    };
    const uint64_t marker_updates[AI_MEMBLOCK_CMO_TRIPLE_ROUNDS] = {
        0xf9bde0ac68245713ULL,
        0x0acef1bd79356824ULL,
        0x1bdf02ce8a467935ULL,
    };
    const uint64_t marker_guards[AI_MEMBLOCK_CMO_TRIPLE_ROUNDS] = {
        0xfeedfacecafebeefULL,
        0x0ddc0ffeebadcafeULL,
        0x1234abcd5678ef90ULL,
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
        AI_ASSERT_NO_EXCEPT_AND("before the four-generation mid-zero CMO probe starts, all source blocks, guards, and marker are readable",
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
        ai_memblock_cmo_triple_three_block_store_cbom_store_zero_store_inval_store_cbom_window(
            block0_vaddr, block1_vaddr, block2_vaddr, marker_vaddr,
            block0_older_store_values[round],
            block0_second_store_values[round],
            block0_third_store_values[round],
            block0_fourth_store_values[round], marker_updates[round],
            &block0_older_observed, &block0_second_observed,
            &block0_third_observed, &block0_fourth_observed,
            &block0_zero_observed, &block1_observed, &block2_observed,
            &marker_observed);
        AI_ASSERT_NO_EXCEPT_AND("under three-block CMO pressure, middle CBO.ZERO clears pre-zero generations while later two store generations survive CBO.INVAL and final CBOM",
            block0_older_observed == 0ULL &&
            block0_second_observed == 0ULL &&
            block0_third_observed == block0_third_store_values[round] &&
            block0_fourth_observed == block0_fourth_store_values[round] &&
            block0_zero_observed == 0ULL &&
            block1_observed == expected_block1_load &&
            block2_observed == 0ULL &&
            marker_observed == marker_updates[round]
        );

        TEST_SETUP_EXCEPT();
        AI_ASSERT_NO_EXCEPT_AND("after the four-generation mid-zero CMO probe, block0 keeps only post-zero stores, block1 preserves seed, block2 is zero, and guards remain intact",
            ld(block0_fourth_store_vaddr) ==
                block0_fourth_store_values[round] &&
            ld(block0_older_store_vaddr) == 0ULL &&
            ld(block0_second_store_vaddr) == 0ULL &&
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
        TEST_ASSERT("backing memory after the four-generation mid-zero CMO probe keeps block0 zero except the two post-zero stores, block1 seed payload, block2 zero, marker update, and all guards intact",
            ai_memblock_cmo_triple_block_zero_except_two_words(
                block0_paddr, block0_fourth_store_word,
                block0_fourth_store_values[round],
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

bool ai_micro_hs_interleaved_three_block_store_zero_store_cbom_store_inval_store_zero_four_generation_final_zero_corner()
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
    const uintptr_t block0_vaddr = hs_page_base(source_page) + 0x480U;
    const uintptr_t block1_vaddr = hs_page_base(source_page) + 0x500U;
    const uintptr_t block2_vaddr = hs_page_base(source_page) + 0x580U;
    const uintptr_t block0_paddr = phys_page_base(source_page) + 0x480U;
    const uintptr_t block1_paddr = phys_page_base(source_page) + 0x500U;
    const uintptr_t block2_paddr = phys_page_base(source_page) + 0x580U;
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
    const uintptr_t marker_vaddr = hs_page_base(marker_page) + 0x480U;
    const uintptr_t marker_paddr = phys_page_base(marker_page) + 0x480U;
    const uintptr_t marker_guard_vaddr = marker_vaddr + sizeof(uint64_t);
    const uintptr_t marker_guard_paddr = marker_paddr + sizeof(uint64_t);
    const uint64_t block0_seeds[AI_MEMBLOCK_CMO_TRIPLE_ROUNDS] = {
        0x1020304050607080ULL,
        0x2131415161718191ULL,
        0x32425262728292a2ULL,
    };
    const uint64_t block1_seeds[AI_MEMBLOCK_CMO_TRIPLE_ROUNDS] = {
        0x8070605040302010ULL,
        0x9181716151413121ULL,
        0xa292827262524232ULL,
    };
    const uint64_t block2_seeds[AI_MEMBLOCK_CMO_TRIPLE_ROUNDS] = {
        0x5a6b7c8d9eafc0d1ULL,
        0x6b7c8d9eafc0d1e2ULL,
        0x7c8d9eafc0d1e2f3ULL,
    };
    const uint64_t block0_older_store_values[AI_MEMBLOCK_CMO_TRIPLE_ROUNDS] = {
        0x010203040506a5a5ULL,
        0x111213141516b6b6ULL,
        0x212223242526c7c7ULL,
    };
    const uint64_t block0_second_store_values[AI_MEMBLOCK_CMO_TRIPLE_ROUNDS] = {
        0xa5a5060504030201ULL,
        0xb6b6161514131211ULL,
        0xc7c7262524232221ULL,
    };
    const uint64_t block0_third_store_values[AI_MEMBLOCK_CMO_TRIPLE_ROUNDS] = {
        0xdad1d2d3d4d5d6d7ULL,
        0xebe2e3e4e5e6e7e8ULL,
        0xfcf3f4f5f6f7f8f9ULL,
    };
    const uint64_t block0_fourth_store_values[AI_MEMBLOCK_CMO_TRIPLE_ROUNDS] = {
        0x0badc0ffee123456ULL,
        0x1cadc0ffee234567ULL,
        0x2dadc0ffee345678ULL,
    };
    const uint64_t before_guards[AI_MEMBLOCK_CMO_TRIPLE_ROUNDS] = {
        0x76543210fedcba98ULL,
        0x87654321edcba987ULL,
        0x98765432dcba9876ULL,
    };
    const uint64_t guard01_values[AI_MEMBLOCK_CMO_TRIPLE_ROUNDS] = {
        0x89abcdef01234567ULL,
        0x9abcdef012345678ULL,
        0xabcdef0123456789ULL,
    };
    const uint64_t guard12_values[AI_MEMBLOCK_CMO_TRIPLE_ROUNDS] = {
        0x13579bdf2468ace0ULL,
        0x2468ace013579bdfULL,
        0x3579bdf02468ace1ULL,
    };
    const uint64_t after_guards[AI_MEMBLOCK_CMO_TRIPLE_ROUNDS] = {
        0x0eca8642fdb97531ULL,
        0x1fdb97530eca8642ULL,
        0x20eca8641fdb9753ULL,
    };
    const uint64_t marker_seeds[AI_MEMBLOCK_CMO_TRIPLE_ROUNDS] = {
        0x445566778899aabbULL,
        0x5566778899aabbccULL,
        0x66778899aabbccddULL,
    };
    const uint64_t marker_updates[AI_MEMBLOCK_CMO_TRIPLE_ROUNDS] = {
        0xbbaa998877665544ULL,
        0xccbbaa9988776655ULL,
        0xddccbbaa99887766ULL,
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
        AI_ASSERT_NO_EXCEPT_AND("before the four-generation double-zero CMO probe starts, all source blocks, guards, and marker are readable",
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
        ai_memblock_cmo_triple_three_block_store_zero_store_cbom_store_inval_store_zero_window(
            block0_vaddr, block1_vaddr, block2_vaddr, marker_vaddr,
            block0_older_store_values[round],
            block0_second_store_values[round],
            block0_third_store_values[round],
            block0_fourth_store_values[round], marker_updates[round],
            &block0_older_observed, &block0_second_observed,
            &block0_third_observed, &block0_fourth_observed,
            &block0_zero_observed, &block1_observed, &block2_observed,
            &marker_observed);
        AI_ASSERT_NO_EXCEPT_AND("under three-block CMO pressure, final CBO.ZERO clears all four same-block store generations after an earlier zero, CBOM, and CBO.INVAL activity",
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
        AI_ASSERT_NO_EXCEPT_AND("after the four-generation double-zero CMO probe, block0 and block2 are zero, block1 preserves seed, and guards remain intact",
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
        TEST_ASSERT("backing memory after the four-generation double-zero CMO probe keeps block0 and block2 fully zero, block1 seed payload, marker update, and all guards intact",
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

bool ai_micro_hs_interleaved_three_block_store_inval_store_zero_store_cbom_store_inval_four_generation_mid_zero_corner()
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
    const uintptr_t block0_vaddr = hs_page_base(source_page) + 0x600U;
    const uintptr_t block1_vaddr = hs_page_base(source_page) + 0x680U;
    const uintptr_t block2_vaddr = hs_page_base(source_page) + 0x700U;
    const uintptr_t block0_paddr = phys_page_base(source_page) + 0x600U;
    const uintptr_t block1_paddr = phys_page_base(source_page) + 0x680U;
    const uintptr_t block2_paddr = phys_page_base(source_page) + 0x700U;
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
    const uintptr_t marker_vaddr = hs_page_base(marker_page) + 0x600U;
    const uintptr_t marker_paddr = phys_page_base(marker_page) + 0x600U;
    const uintptr_t marker_guard_vaddr = marker_vaddr + sizeof(uint64_t);
    const uintptr_t marker_guard_paddr = marker_paddr + sizeof(uint64_t);
    const uint64_t block0_seeds[AI_MEMBLOCK_CMO_TRIPLE_ROUNDS] = {
        0x0c1d2e3f40516273ULL,
        0x1d2e3f4051627384ULL,
        0x2e3f405162738495ULL,
    };
    const uint64_t block1_seeds[AI_MEMBLOCK_CMO_TRIPLE_ROUNDS] = {
        0x736251403f2e1d0cULL,
        0x84736251403f2e1dULL,
        0x9584736251403f2eULL,
    };
    const uint64_t block2_seeds[AI_MEMBLOCK_CMO_TRIPLE_ROUNDS] = {
        0x0123456789abcdefULL,
        0x123456789abcdef0ULL,
        0x23456789abcdef01ULL,
    };
    const uint64_t block0_older_store_values[AI_MEMBLOCK_CMO_TRIPLE_ROUNDS] = {
        0xfedcba9876543210ULL,
        0xedcba9876543210fULL,
        0xdcba9876543210feULL,
    };
    const uint64_t block0_second_store_values[AI_MEMBLOCK_CMO_TRIPLE_ROUNDS] = {
        0x1023456789abcdefULL,
        0x213456789abcdef0ULL,
        0x32456789abcdef01ULL,
    };
    const uint64_t block0_third_store_values[AI_MEMBLOCK_CMO_TRIPLE_ROUNDS] = {
        0x89abcdef01234567ULL,
        0x9abcdef012345678ULL,
        0xabcdef0123456789ULL,
    };
    const uint64_t block0_fourth_store_values[AI_MEMBLOCK_CMO_TRIPLE_ROUNDS] = {
        0x76543210fedcba98ULL,
        0x87654321edcba987ULL,
        0x98765432dcba9876ULL,
    };
    const uint64_t before_guards[AI_MEMBLOCK_CMO_TRIPLE_ROUNDS] = {
        0x1122334455667788ULL,
        0x2233445566778899ULL,
        0x33445566778899aaULL,
    };
    const uint64_t guard01_values[AI_MEMBLOCK_CMO_TRIPLE_ROUNDS] = {
        0x8877665544332211ULL,
        0x9988776655443322ULL,
        0xaa99887766554433ULL,
    };
    const uint64_t guard12_values[AI_MEMBLOCK_CMO_TRIPLE_ROUNDS] = {
        0x0f2e4d6c8baa9876ULL,
        0x1f3e5d7c9bbb9876ULL,
        0x2f4e6d8cabcc9876ULL,
    };
    const uint64_t after_guards[AI_MEMBLOCK_CMO_TRIPLE_ROUNDS] = {
        0x6789aabc6c4d2e0fULL,
        0x789abbbd7d5e3f10ULL,
        0x89abccce8e6f4021ULL,
    };
    const uint64_t marker_seeds[AI_MEMBLOCK_CMO_TRIPLE_ROUNDS] = {
        0x13572468ace0bdf9ULL,
        0x24683579bdf1ce0aULL,
        0x3579468ace02df1bULL,
    };
    const uint64_t marker_updates[AI_MEMBLOCK_CMO_TRIPLE_ROUNDS] = {
        0xf9bde0ac68245713ULL,
        0x0acef1bd79356824ULL,
        0x1bdf02ce8a467935ULL,
    };
    const uint64_t marker_guards[AI_MEMBLOCK_CMO_TRIPLE_ROUNDS] = {
        0xc001d00dcafe0123ULL,
        0xd112e11edbfe1234ULL,
        0xe223f22fecef2345ULL,
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
        AI_ASSERT_NO_EXCEPT_AND("before the four-generation inval-zero CMO probe starts, all source blocks, guards, and marker are readable",
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
        ai_memblock_cmo_triple_three_block_store_inval_store_zero_store_cbom_store_inval_window(
            block0_vaddr, block1_vaddr, block2_vaddr, marker_vaddr,
            block0_older_store_values[round],
            block0_second_store_values[round],
            block0_third_store_values[round],
            block0_fourth_store_values[round], marker_updates[round],
            &block0_older_observed, &block0_second_observed,
            &block0_third_observed, &block0_fourth_observed,
            &block0_zero_observed, &block1_observed, &block2_observed,
            &marker_observed);
        AI_ASSERT_NO_EXCEPT_AND("under three-block CMO pressure, middle CBO.ZERO clears pre-zero generations while later two store generations survive CBOM and final CBO.INVAL",
            block0_older_observed == 0ULL &&
            block0_second_observed == 0ULL &&
            block0_third_observed == block0_third_store_values[round] &&
            block0_fourth_observed == block0_fourth_store_values[round] &&
            block0_zero_observed == 0ULL &&
            block1_observed == 0ULL &&
            block2_observed == expected_block2_load &&
            marker_observed == marker_updates[round]
        );

        TEST_SETUP_EXCEPT();
        AI_ASSERT_NO_EXCEPT_AND("after the four-generation inval-zero CMO probe, block0 keeps only post-zero stores, block1 is zero, block2 preserves seed, and guards remain intact",
            ld(block0_fourth_store_vaddr) ==
                block0_fourth_store_values[round] &&
            ld(block0_older_store_vaddr) == 0ULL &&
            ld(block0_second_store_vaddr) == 0ULL &&
            ld(block0_third_store_vaddr) ==
                block0_third_store_values[round] &&
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
        TEST_ASSERT("backing memory after the four-generation inval-zero CMO probe keeps block0 zero except the two post-zero stores, block1 zero, block2 seed payload, marker update, and all guards intact",
            ai_memblock_cmo_triple_block_zero_except_two_words(
                block0_paddr, block0_fourth_store_word,
                block0_fourth_store_values[round],
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

bool ai_micro_hs_interleaved_three_block_store_cbom_store_inval_store_zero_store_cbom_four_generation_late_zero_corner()
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
    const uintptr_t block0_vaddr = hs_page_base(source_page) + 0x780U;
    const uintptr_t block1_vaddr = hs_page_base(source_page) + 0x800U;
    const uintptr_t block2_vaddr = hs_page_base(source_page) + 0x880U;
    const uintptr_t block0_paddr = phys_page_base(source_page) + 0x780U;
    const uintptr_t block1_paddr = phys_page_base(source_page) + 0x800U;
    const uintptr_t block2_paddr = phys_page_base(source_page) + 0x880U;
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
    const uintptr_t marker_vaddr = hs_page_base(marker_page) + 0x780U;
    const uintptr_t marker_paddr = phys_page_base(marker_page) + 0x780U;
    const uintptr_t marker_guard_vaddr = marker_vaddr + sizeof(uint64_t);
    const uintptr_t marker_guard_paddr = marker_paddr + sizeof(uint64_t);
    const uint64_t block0_seeds[AI_MEMBLOCK_CMO_TRIPLE_ROUNDS] = {
        0x3141592653589793ULL,
        0x42526a376469a8a4ULL,
        0x53637b48757ab9b5ULL,
    };
    const uint64_t block1_seeds[AI_MEMBLOCK_CMO_TRIPLE_ROUNDS] = {
        0x9397585326594131ULL,
        0xa4a86964376a5242ULL,
        0xb5b97a75487b6353ULL,
    };
    const uint64_t block2_seeds[AI_MEMBLOCK_CMO_TRIPLE_ROUNDS] = {
        0x2718281828459045ULL,
        0x382939293956a156ULL,
        0x493a4a3a4a67b267ULL,
    };
    const uint64_t block0_older_store_values[AI_MEMBLOCK_CMO_TRIPLE_ROUNDS] = {
        0xa1b2c3d4e5f60718ULL,
        0xb2c3d4e5f6071829ULL,
        0xc3d4e5f60718293aULL,
    };
    const uint64_t block0_second_store_values[AI_MEMBLOCK_CMO_TRIPLE_ROUNDS] = {
        0x8170605f4e3d2c1bULL,
        0x928171706f4e3d2cULL,
        0xa3928271805f4e3dULL,
    };
    const uint64_t block0_third_store_values[AI_MEMBLOCK_CMO_TRIPLE_ROUNDS] = {
        0x0f0e0d0c0b0a0908ULL,
        0x1f1e1d1c1b1a1918ULL,
        0x2f2e2d2c2b2a2928ULL,
    };
    const uint64_t block0_fourth_store_values[AI_MEMBLOCK_CMO_TRIPLE_ROUNDS] = {
        0x8091a2b3c4d5e6f7ULL,
        0x91a2b3c4d5e6f708ULL,
        0xa2b3c4d5e6f70819ULL,
    };
    const uint64_t before_guards[AI_MEMBLOCK_CMO_TRIPLE_ROUNDS] = {
        0x55aa55aa11223344ULL,
        0x66bb66bb22334455ULL,
        0x77cc77cc33445566ULL,
    };
    const uint64_t guard01_values[AI_MEMBLOCK_CMO_TRIPLE_ROUNDS] = {
        0x44332211aa55aa55ULL,
        0x55443322bb66bb66ULL,
        0x66554433cc77cc77ULL,
    };
    const uint64_t guard12_values[AI_MEMBLOCK_CMO_TRIPLE_ROUNDS] = {
        0x0123cafe4567babeULL,
        0x1234dbef5678cbcfULL,
        0x2345ecf06789dce0ULL,
    };
    const uint64_t after_guards[AI_MEMBLOCK_CMO_TRIPLE_ROUNDS] = {
        0xbabe4567cafe0123ULL,
        0xcbcf5678dbef1234ULL,
        0xdce06789ecf02345ULL,
    };
    const uint64_t marker_seeds[AI_MEMBLOCK_CMO_TRIPLE_ROUNDS] = {
        0xdeadbeef00112233ULL,
        0xefbedead11223344ULL,
        0xf0cfefbe22334455ULL,
    };
    const uint64_t marker_updates[AI_MEMBLOCK_CMO_TRIPLE_ROUNDS] = {
        0x33221100efbeaddeULL,
        0x44332211addebeefULL,
        0x55443322beefcff0ULL,
    };
    const uint64_t marker_guards[AI_MEMBLOCK_CMO_TRIPLE_ROUNDS] = {
        0x13579bdf02468aceULL,
        0x2468ace113579bdfULL,
        0x3579bdf22468ace0ULL,
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
        AI_ASSERT_NO_EXCEPT_AND("before the four-generation late-zero CMO probe starts, all source blocks, guards, and marker are readable",
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
        ai_memblock_cmo_triple_three_block_store_cbom_store_inval_store_zero_store_cbom_window(
            block0_vaddr, block1_vaddr, block2_vaddr, marker_vaddr,
            block0_older_store_values[round],
            block0_second_store_values[round],
            block0_third_store_values[round],
            block0_fourth_store_values[round], marker_updates[round],
            &block0_older_observed, &block0_second_observed,
            &block0_third_observed, &block0_fourth_observed,
            &block0_zero_observed, &block1_observed, &block2_observed,
            &marker_observed);
        AI_ASSERT_NO_EXCEPT_AND("under three-block CMO pressure, late CBO.ZERO clears older three generations while the fourth store survives final CBOM",
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
        AI_ASSERT_NO_EXCEPT_AND("after the four-generation late-zero CMO probe, block0 keeps only the fourth post-zero store, block1 preserves seed, block2 is zero, and guards remain intact",
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
        TEST_ASSERT("backing memory after the four-generation late-zero CMO probe keeps block0 zero except the fourth post-zero store, block1 seed payload, block2 zero, marker update, and all guards intact",
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

bool ai_micro_hs_interleaved_three_block_store_zero_store_inval_store_zero_store_cbom_four_generation_late_zero_corner()
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
    const uintptr_t block0_vaddr = hs_page_base(source_page) + 0x900U;
    const uintptr_t block1_vaddr = hs_page_base(source_page) + 0x980U;
    const uintptr_t block2_vaddr = hs_page_base(source_page) + 0xa00U;
    const uintptr_t block0_paddr = phys_page_base(source_page) + 0x900U;
    const uintptr_t block1_paddr = phys_page_base(source_page) + 0x980U;
    const uintptr_t block2_paddr = phys_page_base(source_page) + 0xa00U;
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
    const uintptr_t marker_vaddr = hs_page_base(marker_page) + 0x900U;
    const uintptr_t marker_paddr = phys_page_base(marker_page) + 0x900U;
    const uintptr_t marker_guard_vaddr = marker_vaddr + sizeof(uint64_t);
    const uintptr_t marker_guard_paddr = marker_paddr + sizeof(uint64_t);
    const uint64_t block0_seeds[AI_MEMBLOCK_CMO_TRIPLE_ROUNDS] = {
        0x0a0b0c0d0e0f1011ULL,
        0x1b1c1d1e1f202122ULL,
        0x2c2d2e2f30313233ULL,
    };
    const uint64_t block1_seeds[AI_MEMBLOCK_CMO_TRIPLE_ROUNDS] = {
        0x11100f0e0d0c0b0aULL,
        0x2221201f1e1d1c1bULL,
        0x333231302f2e2d2cULL,
    };
    const uint64_t block2_seeds[AI_MEMBLOCK_CMO_TRIPLE_ROUNDS] = {
        0x456789abcdeffedcULL,
        0x56789abcdeffedcbULL,
        0x6789abcdeffedcbaULL,
    };
    const uint64_t block0_older_store_values[AI_MEMBLOCK_CMO_TRIPLE_ROUNDS] = {
        0x123456789abcdef0ULL,
        0x23456789abcdef01ULL,
        0x3456789abcdef012ULL,
    };
    const uint64_t block0_second_store_values[AI_MEMBLOCK_CMO_TRIPLE_ROUNDS] = {
        0xf0debc9a78563412ULL,
        0x01efcdab89674523ULL,
        0x120fedbc9a785634ULL,
    };
    const uint64_t block0_third_store_values[AI_MEMBLOCK_CMO_TRIPLE_ROUNDS] = {
        0x0c1d2e3f40516273ULL,
        0x1d2e3f4051627384ULL,
        0x2e3f405162738495ULL,
    };
    const uint64_t block0_fourth_store_values[AI_MEMBLOCK_CMO_TRIPLE_ROUNDS] = {
        0x736251403f2e1d0cULL,
        0x84736251403f2e1dULL,
        0x9584736251403f2eULL,
    };
    const uint64_t before_guards[AI_MEMBLOCK_CMO_TRIPLE_ROUNDS] = {
        0xabcdef9876543210ULL,
        0xbcdefa9876543211ULL,
        0xcdefab9876543212ULL,
    };
    const uint64_t guard01_values[AI_MEMBLOCK_CMO_TRIPLE_ROUNDS] = {
        0x1023456789fedcbaULL,
        0x213456789afedcbbULL,
        0x32456789abfedcbcULL,
    };
    const uint64_t guard12_values[AI_MEMBLOCK_CMO_TRIPLE_ROUNDS] = {
        0x5566778899aabbccULL,
        0x66778899aabbccddULL,
        0x778899aabbccddeeULL,
    };
    const uint64_t after_guards[AI_MEMBLOCK_CMO_TRIPLE_ROUNDS] = {
        0xccbbaa9988776655ULL,
        0xddccbbaa99887766ULL,
        0xeeddccbbaa998877ULL,
    };
    const uint64_t marker_seeds[AI_MEMBLOCK_CMO_TRIPLE_ROUNDS] = {
        0x0ddc0ffeebadf00dULL,
        0x1eed1fff0cbe011eULL,
        0x2ffe30001dcf122fULL,
    };
    const uint64_t marker_updates[AI_MEMBLOCK_CMO_TRIPLE_ROUNDS] = {
        0x0df0adbeef0cdd0dULL,
        0x1e01becff01dee1eULL,
        0x2f12cfd0012eff2fULL,
    };
    const uint64_t marker_guards[AI_MEMBLOCK_CMO_TRIPLE_ROUNDS] = {
        0xa0b1c2d3e4f50617ULL,
        0xb1c2d3e4f5061728ULL,
        0xc2d3e4f506172839ULL,
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
        AI_ASSERT_NO_EXCEPT_AND("before the four-generation double-zero late-clean CMO probe starts, all source blocks, guards, and marker are readable",
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
        ai_memblock_cmo_triple_three_block_store_zero_store_inval_store_zero_store_cbom_window(
            block0_vaddr, block1_vaddr, block2_vaddr, marker_vaddr,
            block0_older_store_values[round],
            block0_second_store_values[round],
            block0_third_store_values[round],
            block0_fourth_store_values[round], marker_updates[round],
            &block0_older_observed, &block0_second_observed,
            &block0_third_observed, &block0_fourth_observed,
            &block0_zero_observed, &block1_observed, &block2_observed,
            &marker_observed);
        AI_ASSERT_NO_EXCEPT_AND("under three-block CMO pressure, the second CBO.ZERO clears older three generations while the fourth store survives final CBOM",
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
        AI_ASSERT_NO_EXCEPT_AND("after the four-generation double-zero late-clean CMO probe, block0 keeps only the fourth post-second-zero store, block1 preserves seed, block2 is zero, and guards remain intact",
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
        TEST_ASSERT("backing memory after the four-generation double-zero late-clean CMO probe keeps block0 zero except the fourth post-second-zero store, block1 seed payload, block2 zero, marker update, and all guards intact",
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

bool ai_micro_hs_interleaved_three_block_store_inval_store_zero_store_inval_store_cbom_four_generation_mid_zero_corner()
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
    const uintptr_t block0_vaddr = hs_page_base(source_page) + 0xa80U;
    const uintptr_t block1_vaddr = hs_page_base(source_page) + 0xb00U;
    const uintptr_t block2_vaddr = hs_page_base(source_page) + 0xb80U;
    const uintptr_t block0_paddr = phys_page_base(source_page) + 0xa80U;
    const uintptr_t block1_paddr = phys_page_base(source_page) + 0xb00U;
    const uintptr_t block2_paddr = phys_page_base(source_page) + 0xb80U;
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
    const uintptr_t marker_vaddr = hs_page_base(marker_page) + 0xa80U;
    const uintptr_t marker_paddr = phys_page_base(marker_page) + 0xa80U;
    const uintptr_t marker_guard_vaddr = marker_vaddr + sizeof(uint64_t);
    const uintptr_t marker_guard_paddr = marker_paddr + sizeof(uint64_t);
    const uint64_t block0_seeds[AI_MEMBLOCK_CMO_TRIPLE_ROUNDS] = {
        0x0f1e2d3c4b5a6978ULL,
        0x102f3e4d5c6b7a89ULL,
        0x21403f5e6d7c8b9aULL,
    };
    const uint64_t block1_seeds[AI_MEMBLOCK_CMO_TRIPLE_ROUNDS] = {
        0x78695a4b3c2d1e0fULL,
        0x897a6b5c4d3e2f10ULL,
        0x9a8b7c6d5e3f4021ULL,
    };
    const uint64_t block2_seeds[AI_MEMBLOCK_CMO_TRIPLE_ROUNDS] = {
        0x2468ace013579bdfULL,
        0x3579bdf12468ace0ULL,
        0x468ace023579bdf1ULL,
    };
    const uint64_t block0_older_store_values[AI_MEMBLOCK_CMO_TRIPLE_ROUNDS] = {
        0xfedc0123456789abULL,
        0xedcb123456789abcULL,
        0xdcba23456789abcdULL,
    };
    const uint64_t block0_second_store_values[AI_MEMBLOCK_CMO_TRIPLE_ROUNDS] = {
        0xab8967452301dcfeULL,
        0xbc9a78563412edcfULL,
        0xcdab89674523f0daULL,
    };
    const uint64_t block0_third_store_values[AI_MEMBLOCK_CMO_TRIPLE_ROUNDS] = {
        0x31425364758697a8ULL,
        0x425364758697a8b9ULL,
        0x5364758697a8b9caULL,
    };
    const uint64_t block0_fourth_store_values[AI_MEMBLOCK_CMO_TRIPLE_ROUNDS] = {
        0xa897867564534231ULL,
        0xb9a8978675645342ULL,
        0xcab9a89786756453ULL,
    };
    const uint64_t before_guards[AI_MEMBLOCK_CMO_TRIPLE_ROUNDS] = {
        0x55aa0123fedc9876ULL,
        0x66bb1234edcb8765ULL,
        0x77cc2345dcba7654ULL,
    };
    const uint64_t guard01_values[AI_MEMBLOCK_CMO_TRIPLE_ROUNDS] = {
        0x6789fedc0123ba98ULL,
        0x789aefcd1234ab89ULL,
        0x89abdfbe23459a7aULL,
    };
    const uint64_t guard12_values[AI_MEMBLOCK_CMO_TRIPLE_ROUNDS] = {
        0x0badf00d13572468ULL,
        0x1cbe011e24683579ULL,
        0x2dcf122f3579468aULL,
    };
    const uint64_t after_guards[AI_MEMBLOCK_CMO_TRIPLE_ROUNDS] = {
        0x682457130df0adbeULL,
        0x793568241e01becfULL,
        0x8a4679352f12cfd0ULL,
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
        0x0123456789abcdefULL,
        0x123456789abcdef0ULL,
        0x23456789abcdef01ULL,
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
        AI_ASSERT_NO_EXCEPT_AND("before the four-generation inval-zero-inval-clean CMO probe starts, all source blocks, guards, and marker are readable",
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
        ai_memblock_cmo_triple_three_block_store_inval_store_zero_store_inval_store_cbom_window(
            block0_vaddr, block1_vaddr, block2_vaddr, marker_vaddr,
            block0_older_store_values[round],
            block0_second_store_values[round],
            block0_third_store_values[round],
            block0_fourth_store_values[round], marker_updates[round],
            &block0_older_observed, &block0_second_observed,
            &block0_third_observed, &block0_fourth_observed,
            &block0_zero_observed, &block1_observed, &block2_observed,
            &marker_observed);
        AI_ASSERT_NO_EXCEPT_AND("under three-block CMO pressure, middle CBO.ZERO clears pre-zero generations while post-zero stores survive a later CBO.INVAL and final CBOM",
            block0_older_observed == 0ULL &&
            block0_second_observed == 0ULL &&
            block0_third_observed == block0_third_store_values[round] &&
            block0_fourth_observed == block0_fourth_store_values[round] &&
            block0_zero_observed == 0ULL &&
            block1_observed == 0ULL &&
            block2_observed == expected_block2_load &&
            marker_observed == marker_updates[round]
        );

        TEST_SETUP_EXCEPT();
        AI_ASSERT_NO_EXCEPT_AND("after the four-generation inval-zero-inval-clean CMO probe, block0 keeps only post-zero stores, block1 is zero, block2 preserves seed, and guards remain intact",
            ld(block0_fourth_store_vaddr) ==
                block0_fourth_store_values[round] &&
            ld(block0_older_store_vaddr) == 0ULL &&
            ld(block0_second_store_vaddr) == 0ULL &&
            ld(block0_third_store_vaddr) ==
                block0_third_store_values[round] &&
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
        TEST_ASSERT("backing memory after the four-generation inval-zero-inval-clean CMO probe keeps block0 zero except the two post-zero stores, block1 zero, block2 seed payload, marker update, and all guards intact",
            ai_memblock_cmo_triple_block_zero_except_two_words(
                block0_paddr, block0_fourth_store_word,
                block0_fourth_store_values[round],
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

bool ai_micro_hs_interleaved_three_block_store_cbom_store_zero_store_cbom_store_inval_four_generation_mid_zero_corner()
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
    const uintptr_t block0_vaddr = hs_page_base(source_page) + 0xc00U;
    const uintptr_t block1_vaddr = hs_page_base(source_page) + 0xc80U;
    const uintptr_t block2_vaddr = hs_page_base(source_page) + 0xd00U;
    const uintptr_t block0_paddr = phys_page_base(source_page) + 0xc00U;
    const uintptr_t block1_paddr = phys_page_base(source_page) + 0xc80U;
    const uintptr_t block2_paddr = phys_page_base(source_page) + 0xd00U;
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
    const uintptr_t marker_vaddr = hs_page_base(marker_page) + 0xc00U;
    const uintptr_t marker_paddr = phys_page_base(marker_page) + 0xc00U;
    const uintptr_t marker_guard_vaddr = marker_vaddr + sizeof(uint64_t);
    const uintptr_t marker_guard_paddr = marker_paddr + sizeof(uint64_t);
    const uint64_t block0_seeds[AI_MEMBLOCK_CMO_TRIPLE_ROUNDS] = {
        0x1213141516171819ULL,
        0x232425262728292aULL,
        0x3435363738393a3bULL,
    };
    const uint64_t block1_seeds[AI_MEMBLOCK_CMO_TRIPLE_ROUNDS] = {
        0x1918171615141312ULL,
        0x2a29282726252423ULL,
        0x3b3a393837363534ULL,
    };
    const uint64_t block2_seeds[AI_MEMBLOCK_CMO_TRIPLE_ROUNDS] = {
        0x4c5d6e7f8091a2b3ULL,
        0x5d6e7f8091a2b3c4ULL,
        0x6e7f8091a2b3c4d5ULL,
    };
    const uint64_t block0_older_store_values[AI_MEMBLOCK_CMO_TRIPLE_ROUNDS] = {
        0xaaaabbbbccccddddULL,
        0xbbbbccccddddeeeeULL,
        0xccccddddeeeeffffULL,
    };
    const uint64_t block0_second_store_values[AI_MEMBLOCK_CMO_TRIPLE_ROUNDS] = {
        0xddddccccbbbbaaaaULL,
        0xeeeeddddccccbbbbULL,
        0xffffeeeeddddccccULL,
    };
    const uint64_t block0_third_store_values[AI_MEMBLOCK_CMO_TRIPLE_ROUNDS] = {
        0x02468ace13579bdfULL,
        0x13579bdf2468ace0ULL,
        0x2468ace03579bdf1ULL,
    };
    const uint64_t block0_fourth_store_values[AI_MEMBLOCK_CMO_TRIPLE_ROUNDS] = {
        0xfdb97531eca86420ULL,
        0x0eca8642fdb97531ULL,
        0x1fdb97530eca8642ULL,
    };
    const uint64_t before_guards[AI_MEMBLOCK_CMO_TRIPLE_ROUNDS] = {
        0x0123fedc4567ba98ULL,
        0x1234edcb5678ab89ULL,
        0x2345dcba67899a7aULL,
    };
    const uint64_t guard01_values[AI_MEMBLOCK_CMO_TRIPLE_ROUNDS] = {
        0x98ba6745dcfe2301ULL,
        0x89ab7856cbed3412ULL,
        0x7a9a8967badc4523ULL,
    };
    const uint64_t guard12_values[AI_MEMBLOCK_CMO_TRIPLE_ROUNDS] = {
        0xc0ffee0011223344ULL,
        0xd100ff1122334455ULL,
        0xe211002233445566ULL,
    };
    const uint64_t after_guards[AI_MEMBLOCK_CMO_TRIPLE_ROUNDS] = {
        0x4433221100eeffc0ULL,
        0x5544332211ff00d1ULL,
        0x66554433220011e2ULL,
    };
    const uint64_t marker_seeds[AI_MEMBLOCK_CMO_TRIPLE_ROUNDS] = {
        0x13572468ace0ffffULL,
        0x24683579bdf10000ULL,
        0x3579468ace021111ULL,
    };
    const uint64_t marker_updates[AI_MEMBLOCK_CMO_TRIPLE_ROUNDS] = {
        0xffffe0ac68245713ULL,
        0x0000f1bd79356824ULL,
        0x111102ce8a467935ULL,
    };
    const uint64_t marker_guards[AI_MEMBLOCK_CMO_TRIPLE_ROUNDS] = {
        0x55aa55aa55aa55aaULL,
        0x66bb66bb66bb66bbULL,
        0x77cc77cc77cc77ccULL,
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
        AI_ASSERT_NO_EXCEPT_AND("before the four-generation clean-zero-clean-inval CMO probe starts, all source blocks, guards, and marker are readable",
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
        ai_memblock_cmo_triple_three_block_store_cbom_store_zero_store_cbom_store_inval_window(
            block0_vaddr, block1_vaddr, block2_vaddr, marker_vaddr,
            block0_older_store_values[round],
            block0_second_store_values[round],
            block0_third_store_values[round],
            block0_fourth_store_values[round], marker_updates[round],
            &block0_older_observed, &block0_second_observed,
            &block0_third_observed, &block0_fourth_observed,
            &block0_zero_observed, &block1_observed, &block2_observed,
            &marker_observed);
        AI_ASSERT_NO_EXCEPT_AND("under three-block CMO pressure, middle CBO.ZERO clears pre-zero generations while post-zero stores survive final CBOM and final CBO.INVAL",
            block0_older_observed == 0ULL &&
            block0_second_observed == 0ULL &&
            block0_third_observed == block0_third_store_values[round] &&
            block0_fourth_observed == block0_fourth_store_values[round] &&
            block0_zero_observed == 0ULL &&
            block1_observed == expected_block1_load &&
            block2_observed == 0ULL &&
            marker_observed == marker_updates[round]
        );

        TEST_SETUP_EXCEPT();
        AI_ASSERT_NO_EXCEPT_AND("after the four-generation clean-zero-clean-inval CMO probe, block0 keeps only post-zero stores, block1 preserves seed, block2 is zero, and guards remain intact",
            ld(block0_fourth_store_vaddr) ==
                block0_fourth_store_values[round] &&
            ld(block0_older_store_vaddr) == 0ULL &&
            ld(block0_second_store_vaddr) == 0ULL &&
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
        TEST_ASSERT("backing memory after the four-generation clean-zero-clean-inval CMO probe keeps block0 zero except the two post-zero stores, block1 seed payload, block2 zero, marker update, and all guards intact",
            ai_memblock_cmo_triple_block_zero_except_two_words(
                block0_paddr, block0_fourth_store_word,
                block0_fourth_store_values[round],
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
