#include "ai_micro_memblock_cmo_triple_common.h"

bool ai_micro_hs_cboinval_then_cbom_then_cbozero_no_stale_triple_cmo_owner_corner()
{
    TEST_START();

    const enum test_page source_page = SWITCH1;
    const enum test_page marker_page = SWITCH2;
    const size_t load_word = 3U;
    const uintptr_t source_vaddr = hs_page_base(source_page) + 0x1200U;
    const uintptr_t source_paddr = phys_page_base(source_page) + 0x1200U;
    const uintptr_t inval_vaddr = source_vaddr + 4U * sizeof(uint64_t);
    const uintptr_t clean_vaddr = source_vaddr + 1U * sizeof(uint64_t);
    const uintptr_t flush_vaddr = source_vaddr +
        (AI_MEMBLOCK_CMO_TRIPLE_BLOCK_WORDS - 1U) * sizeof(uint64_t);
    const uintptr_t zero_vaddr = source_vaddr + 6U * sizeof(uint64_t);
    const uintptr_t load_vaddr = source_vaddr + load_word * sizeof(uint64_t);
    const uintptr_t source_tail_vaddr = flush_vaddr;
    const uintptr_t before_guard_vaddr = source_vaddr - sizeof(uint64_t);
    const uintptr_t after_guard_vaddr = source_vaddr +
        AI_MEMBLOCK_CMO_TRIPLE_BLOCK_WORDS * sizeof(uint64_t);
    const uintptr_t before_guard_paddr = source_paddr - sizeof(uint64_t);
    const uintptr_t after_guard_paddr = source_paddr +
        AI_MEMBLOCK_CMO_TRIPLE_BLOCK_WORDS * sizeof(uint64_t);
    const uintptr_t marker_vaddr = hs_page_base(marker_page) + 0x1200U;
    const uintptr_t marker_paddr = phys_page_base(marker_page) + 0x1200U;
    const uintptr_t marker_guard_vaddr = marker_vaddr + sizeof(uint64_t);
    const uintptr_t marker_guard_paddr = marker_paddr + sizeof(uint64_t);
    const uint64_t block_seeds[AI_MEMBLOCK_CMO_TRIPLE_ROUNDS] = {
        0x1029384756abcdefULL,
        0x213a495867bcdef0ULL,
        0x324b5a6978cdef01ULL,
    };
    const uint64_t before_guards[AI_MEMBLOCK_CMO_TRIPLE_ROUNDS] = {
        0xf00dd00dcafebeefULL,
        0xe11ee11ebabef00dULL,
        0xd22ff22fc001d00dULL,
    };
    const uint64_t after_guards[AI_MEMBLOCK_CMO_TRIPLE_ROUNDS] = {
        0x76543210fedcba98ULL,
        0x6543210fedcba987ULL,
        0x543210fedcba9876ULL,
    };
    const uint64_t marker_seeds[AI_MEMBLOCK_CMO_TRIPLE_ROUNDS] = {
        0x1111222233334444ULL,
        0x2222333344445555ULL,
        0x3333444455556666ULL,
    };
    const uint64_t marker_updates[AI_MEMBLOCK_CMO_TRIPLE_ROUNDS] = {
        0xabcdef0123456789ULL,
        0xbcdef0123456789aULL,
        0xcdef0123456789abULL,
    };
    const uint64_t marker_guards[AI_MEMBLOCK_CMO_TRIPLE_ROUNDS] = {
        0x55aa55aa33cc33ccULL,
        0x66bb66bb44dd44ddULL,
        0x77cc77cc55ee55eeULL,
    };

    ai_memblock_cmo_triple_prepare_hs_env();

    for (size_t round = 0; round < AI_MEMBLOCK_CMO_TRIPLE_ROUNDS; ++round) {
        uint64_t load_observed = 0ULL;
        uint64_t marker_observed = 0ULL;
        const uint64_t expected_load_before = block_seeds[round] +
            load_word * 0x1111111111111111ULL;
        const uint64_t expected_tail_before = block_seeds[round] +
            (AI_MEMBLOCK_CMO_TRIPLE_BLOCK_WORDS - 1U) *
            0x1111111111111111ULL;

        goto_priv(PRIV_M);
        ai_memblock_cmo_triple_seed_block(source_paddr, block_seeds[round]);
        write64(before_guard_paddr, before_guards[round]);
        write64(after_guard_paddr, after_guards[round]);
        write64(marker_paddr, marker_seeds[round]);
        write64(marker_guard_paddr, marker_guards[round]);

        goto_priv(PRIV_HS);
        TEST_SETUP_EXCEPT();
        AI_ASSERT_NO_EXCEPT_AND("before the CBO.INVAL -> CBOM -> CBO.ZERO triple owner chain starts, source block and marker are readable",
            ld(source_vaddr) == block_seeds[round] &&
            ld(load_vaddr) == expected_load_before &&
            ld(source_tail_vaddr) == expected_tail_before &&
            ld(marker_vaddr) == marker_seeds[round]
        );

        TEST_SETUP_EXCEPT();
        ai_memblock_cmo_triple_inval_cbom_zero_window(inval_vaddr,
            clean_vaddr, flush_vaddr, zero_vaddr, load_vaddr, marker_vaddr,
            marker_updates[round], &load_observed, &marker_observed);
        AI_ASSERT_NO_EXCEPT_AND("CBO.INVAL -> CBOM clean/flush -> CBO.ZERO releases each CMO owner in order so the final zero image and marker survive",
            load_observed == 0ULL &&
            marker_observed == marker_updates[round]
        );

        TEST_SETUP_EXCEPT();
        AI_ASSERT_NO_EXCEPT_AND("the CBO.INVAL -> CBOM -> CBO.ZERO triple owner chain leaves source block zero and all guards intact",
            ld(source_vaddr) == 0ULL &&
            ld(load_vaddr) == 0ULL &&
            ld(source_tail_vaddr) == 0ULL &&
            ld(before_guard_vaddr) == before_guards[round] &&
            ld(after_guard_vaddr) == after_guards[round] &&
            ld(marker_guard_vaddr) == marker_guards[round]
        );

        goto_priv(PRIV_M);
        TEST_ASSERT("backing memory after the CBO.INVAL -> CBOM -> CBO.ZERO triple owner chain contains a fully zeroed source block, marker update, and unchanged guards",
            ai_memblock_cmo_triple_block_all_zero(source_paddr) &&
            read64(before_guard_paddr) == before_guards[round] &&
            read64(after_guard_paddr) == after_guards[round] &&
            read64(marker_paddr) == marker_updates[round] &&
            read64(marker_guard_paddr) == marker_guards[round]
        );
    }

    ai_memblock_cmo_triple_reset_hs_env();

    TEST_END();
}

bool ai_micro_hs_interleaved_three_block_store_cbom_store_zero_store_zero_store_zero_four_generation_final_zero_corner()
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
        AI_ASSERT_NO_EXCEPT_AND("before the four-generation clean-zero-zero-zero CMO probe starts, all source blocks, guards, and marker are readable",
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
        ai_memblock_cmo_triple_three_block_store_cbom_store_zero_store_zero_store_zero_window(
            block0_vaddr, block1_vaddr, block2_vaddr, marker_vaddr,
            block0_older_store_values[round],
            block0_second_store_values[round],
            block0_third_store_values[round],
            block0_fourth_store_values[round], marker_updates[round],
            &block0_older_observed, &block0_second_observed,
            &block0_third_observed, &block0_fourth_observed,
            &block0_zero_observed, &block1_observed, &block2_observed,
            &marker_observed);
        AI_ASSERT_NO_EXCEPT_AND("under three-block CMO pressure, CBOM followed by triple CBO.ZERO clears all same-block store generations",
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
        AI_ASSERT_NO_EXCEPT_AND("after the four-generation clean-zero-zero-zero CMO probe, block0 and block1 are zero, block2 preserves seed, and guards remain intact",
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
        TEST_ASSERT("backing memory after the four-generation clean-zero-zero-zero CMO probe keeps block0 and block1 fully zero, block2 seed payload, marker update, and all guards intact",
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

bool ai_micro_hs_interleaved_three_block_store_cbom_store_zero_store_zero_store_inval_four_generation_final_post_zero_corner()
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
        AI_ASSERT_NO_EXCEPT_AND("before the four-generation clean-zero-zero-inval CMO probe starts, all source blocks, guards, and marker are readable",
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
        ai_memblock_cmo_triple_three_block_store_cbom_store_zero_store_zero_store_inval_window(
            block0_vaddr, block1_vaddr, block2_vaddr, marker_vaddr,
            block0_older_store_values[round],
            block0_second_store_values[round],
            block0_third_store_values[round],
            block0_fourth_store_values[round], marker_updates[round],
            &block0_older_observed, &block0_second_observed,
            &block0_third_observed, &block0_fourth_observed,
            &block0_zero_observed, &block1_observed, &block2_observed,
            &marker_observed);
        AI_ASSERT_NO_EXCEPT_AND("under three-block CMO pressure, CBOM/CBO.ZERO/CBO.ZERO/CBO.INVAL keeps only the fourth post-zero store visible",
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
        AI_ASSERT_NO_EXCEPT_AND("after the four-generation clean-zero-zero-inval CMO probe, block0 keeps only the fourth post-zero store, block1 is zero, block2 preserves seed, and guards remain intact",
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
        TEST_ASSERT("backing memory after the four-generation clean-zero-zero-inval CMO probe keeps block0 zero except the fourth post-zero store, block1 zero, block2 seed payload, marker update, and all guards intact",
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

bool ai_micro_hs_interleaved_three_block_store_cbom_store_zero_store_zero_store_cbom_four_generation_final_post_zero_corner()
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
        AI_ASSERT_NO_EXCEPT_AND("before the four-generation clean-zero-zero-clean CMO probe starts, all source blocks, guards, and marker are readable",
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
        ai_memblock_cmo_triple_three_block_store_cbom_store_zero_store_zero_store_cbom_window(
            block0_vaddr, block1_vaddr, block2_vaddr, marker_vaddr,
            block0_older_store_values[round],
            block0_second_store_values[round],
            block0_third_store_values[round],
            block0_fourth_store_values[round], marker_updates[round],
            &block0_older_observed, &block0_second_observed,
            &block0_third_observed, &block0_fourth_observed,
            &block0_zero_observed, &block1_observed, &block2_observed,
            &marker_observed);
        AI_ASSERT_NO_EXCEPT_AND("under three-block CMO pressure, CBOM/CBO.ZERO/CBO.ZERO/CBOM keeps only the fourth post-zero store visible through final clean",
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
        AI_ASSERT_NO_EXCEPT_AND("after the four-generation clean-zero-zero-clean CMO probe, block0 keeps only the fourth post-zero store, block1 is zero, block2 preserves seed, and guards remain intact",
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
        TEST_ASSERT("backing memory after the four-generation clean-zero-zero-clean CMO probe keeps block0 zero except the fourth post-zero store, block1 zero, block2 seed payload, marker update, and all guards intact",
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

bool ai_micro_hs_interleaved_three_block_store_cbom_store_zero_store_inval_store_inval_four_generation_post_zero_corner()
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
        AI_ASSERT_NO_EXCEPT_AND("before the four-generation clean-zero-inval-inval CMO probe starts, all source blocks, guards, and marker are readable",
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
        ai_memblock_cmo_triple_three_block_store_cbom_store_zero_store_inval_store_inval_window(
            block0_vaddr, block1_vaddr, block2_vaddr, marker_vaddr,
            block0_older_store_values[round],
            block0_second_store_values[round],
            block0_third_store_values[round],
            block0_fourth_store_values[round], marker_updates[round],
            &block0_older_observed, &block0_second_observed,
            &block0_third_observed, &block0_fourth_observed,
            &block0_zero_observed, &block1_observed, &block2_observed,
            &marker_observed);
        AI_ASSERT_NO_EXCEPT_AND("under three-block CMO pressure, CBOM/CBO.ZERO/CBO.INVAL/CBO.INVAL keeps both post-zero stores visible",
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
        AI_ASSERT_NO_EXCEPT_AND("after the four-generation clean-zero-inval-inval CMO probe, block0 keeps only post-zero stores, block1 is zero, block2 preserves seed, and guards remain intact",
            ld(block0_fourth_store_vaddr) ==
                block0_fourth_store_values[round] &&
            ld(block0_older_store_vaddr) == 0ULL &&
            ld(block0_zero_load_vaddr) == 0ULL &&
            ld(block0_second_store_vaddr) == 0ULL &&
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
        TEST_ASSERT("backing memory after the four-generation clean-zero-inval-inval CMO probe keeps block0 zero except the two post-zero stores, block1 zero, block2 seed payload, marker update, and all guards intact",
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

bool ai_micro_hs_interleaved_three_block_store_cbom_store_zero_store_cbom_store_cbom_four_generation_post_zero_corner()
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
        AI_ASSERT_NO_EXCEPT_AND("before the four-generation clean-zero-clean-clean CMO probe starts, all source blocks, guards, and marker are readable",
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
        ai_memblock_cmo_triple_three_block_store_cbom_store_zero_store_cbom_store_cbom_window(
            block0_vaddr, block1_vaddr, block2_vaddr, marker_vaddr,
            block0_older_store_values[round],
            block0_second_store_values[round],
            block0_third_store_values[round],
            block0_fourth_store_values[round], marker_updates[round],
            &block0_older_observed, &block0_second_observed,
            &block0_third_observed, &block0_fourth_observed,
            &block0_zero_observed, &block1_observed, &block2_observed,
            &marker_observed);
        AI_ASSERT_NO_EXCEPT_AND("under three-block CMO pressure, CBOM/CBO.ZERO/CBOM/CBOM keeps both post-zero stores visible through grouped clean owners",
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
        AI_ASSERT_NO_EXCEPT_AND("after the four-generation clean-zero-clean-clean CMO probe, block0 keeps only post-zero stores, block1 is zero, block2 preserves seed, and guards remain intact",
            ld(block0_fourth_store_vaddr) ==
                block0_fourth_store_values[round] &&
            ld(block0_older_store_vaddr) == 0ULL &&
            ld(block0_zero_load_vaddr) == 0ULL &&
            ld(block0_second_store_vaddr) == 0ULL &&
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
        TEST_ASSERT("backing memory after the four-generation clean-zero-clean-clean CMO probe keeps block0 zero except the two post-zero stores, block1 zero, block2 seed payload, marker update, and all guards intact",
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

bool ai_micro_hs_interleaved_three_block_store_cbom_store_inval_store_zero_store_zero_four_generation_final_zero_corner()
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
        AI_ASSERT_NO_EXCEPT_AND("before the four-generation clean-inval-zero-zero CMO probe starts, all source blocks, guards, and marker are readable",
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
        ai_memblock_cmo_triple_three_block_store_cbom_store_inval_store_zero_store_zero_window(
            block0_vaddr, block1_vaddr, block2_vaddr, marker_vaddr,
            block0_older_store_values[round],
            block0_second_store_values[round],
            block0_third_store_values[round],
            block0_fourth_store_values[round], marker_updates[round],
            &block0_older_observed, &block0_second_observed,
            &block0_third_observed, &block0_fourth_observed,
            &block0_zero_observed, &block1_observed, &block2_observed,
            &marker_observed);
        AI_ASSERT_NO_EXCEPT_AND("under three-block CMO pressure, final CBO.ZERO clears all generations after prior CBOM, CBO.INVAL, and earlier CBO.ZERO",
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
        AI_ASSERT_NO_EXCEPT_AND("after the four-generation clean-inval-zero-zero CMO probe, block0 is fully zero, block1 preserves seed, block2 is zero, and guards remain intact",
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
        TEST_ASSERT("backing memory after the four-generation clean-inval-zero-zero CMO probe keeps block0 and block2 fully zero, block1 seed payload, marker update, and all guards intact",
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

bool ai_micro_hs_interleaved_three_block_store_zero_store_inval_store_cbom_store_zero_four_generation_final_zero_corner()
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
    const uintptr_t block0_vaddr = hs_page_base(source_page) + 0x2680U;
    const uintptr_t block1_vaddr = hs_page_base(source_page) + 0x2880U;
    const uintptr_t block2_vaddr = hs_page_base(source_page) + 0x2a80U;
    const uintptr_t block0_paddr = phys_page_base(source_page) + 0x2680U;
    const uintptr_t block1_paddr = phys_page_base(source_page) + 0x2880U;
    const uintptr_t block2_paddr = phys_page_base(source_page) + 0x2a80U;
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
    const uintptr_t marker_vaddr = hs_page_base(marker_page) + 0x2680U;
    const uintptr_t marker_paddr = phys_page_base(marker_page) + 0x2680U;
    const uintptr_t marker_guard_vaddr = marker_vaddr + sizeof(uint64_t);
    const uintptr_t marker_guard_paddr = marker_paddr + sizeof(uint64_t);
    const uint64_t block0_seeds[AI_MEMBLOCK_CMO_TRIPLE_ROUNDS] = {
        0x4a5b6c7d8e9fa0b1ULL,
        0x5b6c7d8e9fa0b1c2ULL,
        0x6c7d8e9fa0b1c2d3ULL,
    };
    const uint64_t block1_seeds[AI_MEMBLOCK_CMO_TRIPLE_ROUNDS] = {
        0xb1a09f8e7d6c5b4aULL,
        0xc2b1a09f8e7d6c5bULL,
        0xd3c2b1a09f8e7d6cULL,
    };
    const uint64_t block2_seeds[AI_MEMBLOCK_CMO_TRIPLE_ROUNDS] = {
        0x0011223344556677ULL,
        0x1122334455667788ULL,
        0x2233445566778899ULL,
    };
    const uint64_t block0_older_store_values[AI_MEMBLOCK_CMO_TRIPLE_ROUNDS] = {
        0xa55aa55a12345678ULL,
        0xb66bb66b23456789ULL,
        0xc77cc77c3456789aULL,
    };
    const uint64_t block0_second_store_values[AI_MEMBLOCK_CMO_TRIPLE_ROUNDS] = {
        0x87654321a55aa55aULL,
        0x98765432b66bb66bULL,
        0xa9876543c77cc77cULL,
    };
    const uint64_t block0_third_store_values[AI_MEMBLOCK_CMO_TRIPLE_ROUNDS] = {
        0x0a1b2c3d4e5f6071ULL,
        0x1b2c3d4e5f607182ULL,
        0x2c3d4e5f60718293ULL,
    };
    const uint64_t block0_fourth_store_values[AI_MEMBLOCK_CMO_TRIPLE_ROUNDS] = {
        0x71605f4e3d2c1b0aULL,
        0x8271605f4e3d2c1bULL,
        0x938271605f4e3d2cULL,
    };
    const uint64_t before_guards[AI_MEMBLOCK_CMO_TRIPLE_ROUNDS] = {
        0x13579bdf2468ace0ULL,
        0x2468ace13579bdf1ULL,
        0x3579bdf2468ace02ULL,
    };
    const uint64_t guard01_values[AI_MEMBLOCK_CMO_TRIPLE_ROUNDS] = {
        0x0eca8642fdb97531ULL,
        0x1fdb97530eca8642ULL,
        0x20eca8641fdb9753ULL,
    };
    const uint64_t guard12_values[AI_MEMBLOCK_CMO_TRIPLE_ROUNDS] = {
        0xabcdef9876543210ULL,
        0xbcdef09876543211ULL,
        0xcdef019876543212ULL,
    };
    const uint64_t after_guards[AI_MEMBLOCK_CMO_TRIPLE_ROUNDS] = {
        0x0123456789fedcbaULL,
        0x123456789afedcbbULL,
        0x23456789abfedcbcULL,
    };
    const uint64_t marker_seeds[AI_MEMBLOCK_CMO_TRIPLE_ROUNDS] = {
        0xc001cafe55aa1234ULL,
        0xd112db0f66bb2345ULL,
        0xe223ec2077cc3456ULL,
    };
    const uint64_t marker_updates[AI_MEMBLOCK_CMO_TRIPLE_ROUNDS] = {
        0x3412aa55fecac001ULL,
        0x4523bb660fdb12d1ULL,
        0x5634cc7720ec23e2ULL,
    };
    const uint64_t marker_guards[AI_MEMBLOCK_CMO_TRIPLE_ROUNDS] = {
        0x1020304050607080ULL,
        0x2131415161718191ULL,
        0x32425262728292a2ULL,
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
        AI_ASSERT_NO_EXCEPT_AND("before the four-generation zero-inval-clean-zero CMO probe starts, all source blocks, guards, and marker are readable",
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
        ai_memblock_cmo_triple_three_block_store_zero_store_inval_store_cbom_store_zero_window(
            block0_vaddr, block1_vaddr, block2_vaddr, marker_vaddr,
            block0_older_store_values[round],
            block0_second_store_values[round],
            block0_third_store_values[round],
            block0_fourth_store_values[round], marker_updates[round],
            &block0_older_observed, &block0_second_observed,
            &block0_third_observed, &block0_fourth_observed,
            &block0_zero_observed, &block1_observed, &block2_observed,
            &marker_observed);
        AI_ASSERT_NO_EXCEPT_AND("under three-block CMO pressure, final CBO.ZERO clears all generations after early CBO.ZERO, CBO.INVAL, and CBOM activity",
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
        AI_ASSERT_NO_EXCEPT_AND("after the four-generation zero-inval-clean-zero CMO probe, block0 and block2 are zero, block1 preserves seed, and guards remain intact",
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
        TEST_ASSERT("backing memory after the four-generation zero-inval-clean-zero CMO probe keeps block0 zero, block1 seed payload, block2 zero, marker update, and all guards intact",
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

bool ai_micro_hs_interleaved_three_block_store_inval_store_zero_store_cbom_store_zero_four_generation_final_zero_corner()
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
    const uintptr_t block0_vaddr = hs_page_base(source_page) + 0x2080U;
    const uintptr_t block1_vaddr = hs_page_base(source_page) + 0x2280U;
    const uintptr_t block2_vaddr = hs_page_base(source_page) + 0x2480U;
    const uintptr_t block0_paddr = phys_page_base(source_page) + 0x2080U;
    const uintptr_t block1_paddr = phys_page_base(source_page) + 0x2280U;
    const uintptr_t block2_paddr = phys_page_base(source_page) + 0x2480U;
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
    const uintptr_t marker_vaddr = hs_page_base(marker_page) + 0x2080U;
    const uintptr_t marker_paddr = phys_page_base(marker_page) + 0x2080U;
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
        0x0f1e2d3c4b5a6978ULL,
        0x102f3e4d5c6b7a89ULL,
        0x21403f5e6d7c8b9aULL,
    };
    const uint64_t block0_older_store_values[AI_MEMBLOCK_CMO_TRIPLE_ROUNDS] = {
        0x89abcdef76543210ULL,
        0x9abcdef087654321ULL,
        0xabcdef0198765432ULL,
    };
    const uint64_t block0_second_store_values[AI_MEMBLOCK_CMO_TRIPLE_ROUNDS] = {
        0x01234567fedcba98ULL,
        0x12345678edcba987ULL,
        0x23456789dcba9876ULL,
    };
    const uint64_t block0_third_store_values[AI_MEMBLOCK_CMO_TRIPLE_ROUNDS] = {
        0xf0e1d2c3b4a59687ULL,
        0xe1d2c3b4a5968776ULL,
        0xd2c3b4a596877665ULL,
    };
    const uint64_t block0_fourth_store_values[AI_MEMBLOCK_CMO_TRIPLE_ROUNDS] = {
        0x78695a4b3c2d1e0fULL,
        0x897a6b5c4d3e2f10ULL,
        0x9a8b7c6d5e4f3021ULL,
    };
    const uint64_t before_guards[AI_MEMBLOCK_CMO_TRIPLE_ROUNDS] = {
        0x010f1e2d3c4b5a69ULL,
        0x12202f3e4d5c6b7aULL,
        0x2331404f5e6d7c8bULL,
    };
    const uint64_t guard01_values[AI_MEMBLOCK_CMO_TRIPLE_ROUNDS] = {
        0x695a4b3c2d1e0f01ULL,
        0x7a6b5c4d3e2f2012ULL,
        0x8b7c6d5e4f403123ULL,
    };
    const uint64_t guard12_values[AI_MEMBLOCK_CMO_TRIPLE_ROUNDS] = {
        0xcafe1234babe5678ULL,
        0xdb0f2345cbcf6789ULL,
        0xec203456dce0789aULL,
    };
    const uint64_t after_guards[AI_MEMBLOCK_CMO_TRIPLE_ROUNDS] = {
        0x8765ebab4321efacULL,
        0x9876fcbc5432f0bdULL,
        0xa9870dcd654301ceULL,
    };
    const uint64_t marker_seeds[AI_MEMBLOCK_CMO_TRIPLE_ROUNDS] = {
        0x55aa1234ccdd5678ULL,
        0x66bb2345ddee6789ULL,
        0x77cc3456eeff789aULL,
    };
    const uint64_t marker_updates[AI_MEMBLOCK_CMO_TRIPLE_ROUNDS] = {
        0x7856ddcc3412aa55ULL,
        0x8968eedd4523bb66ULL,
        0x9a78ffee5634cc77ULL,
    };
    const uint64_t marker_guards[AI_MEMBLOCK_CMO_TRIPLE_ROUNDS] = {
        0x1029384756abcdefULL,
        0x213a495867bcdef0ULL,
        0x324b5a6978cdef01ULL,
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
        AI_ASSERT_NO_EXCEPT_AND("before the four-generation inval-zero-clean-zero CMO probe starts, all source blocks, guards, and marker are readable",
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
        ai_memblock_cmo_triple_three_block_store_inval_store_zero_store_cbom_store_zero_window(
            block0_vaddr, block1_vaddr, block2_vaddr, marker_vaddr,
            block0_older_store_values[round],
            block0_second_store_values[round],
            block0_third_store_values[round],
            block0_fourth_store_values[round], marker_updates[round],
            &block0_older_observed, &block0_second_observed,
            &block0_third_observed, &block0_fourth_observed,
            &block0_zero_observed, &block1_observed, &block2_observed,
            &marker_observed);
        AI_ASSERT_NO_EXCEPT_AND("under three-block CMO pressure, final CBO.ZERO clears all four generations after CBO.INVAL, middle CBO.ZERO, and CBOM activity",
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
        AI_ASSERT_NO_EXCEPT_AND("after the four-generation inval-zero-clean-zero CMO probe, block0 and block1 are zero, block2 preserves seed, and guards remain intact",
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
        TEST_ASSERT("backing memory after the four-generation inval-zero-clean-zero CMO probe keeps block0 zero, block1 zero, block2 seed payload, marker update, and all guards intact",
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

bool ai_micro_hs_interleaved_three_block_store_cbom_store_inval_store_cbom_store_zero_four_generation_final_zero_corner()
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
    const uintptr_t block0_vaddr = hs_page_base(source_page) + 0x3680U;
    const uintptr_t block1_vaddr = hs_page_base(source_page) + 0x3700U;
    const uintptr_t block2_vaddr = hs_page_base(source_page) + 0x3780U;
    const uintptr_t block0_paddr = phys_page_base(source_page) + 0x3680U;
    const uintptr_t block1_paddr = phys_page_base(source_page) + 0x3700U;
    const uintptr_t block2_paddr = phys_page_base(source_page) + 0x3780U;
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
    const uintptr_t marker_vaddr = hs_page_base(marker_page) + 0x3680U;
    const uintptr_t marker_paddr = phys_page_base(marker_page) + 0x3680U;
    const uintptr_t marker_guard_vaddr = marker_vaddr + sizeof(uint64_t);
    const uintptr_t marker_guard_paddr = marker_paddr + sizeof(uint64_t);
    const uint64_t block0_seeds[AI_MEMBLOCK_CMO_TRIPLE_ROUNDS] = {
        0x6c7d8e9fab0c1d2eULL,
        0x7d8e9fab0c1d2e3fULL,
        0x8e9fab0c1d2e3f40ULL,
    };
    const uint64_t block1_seeds[AI_MEMBLOCK_CMO_TRIPLE_ROUNDS] = {
        0x2e1d0cab9f8e7d6cULL,
        0x3f2e1d0cab9f8e7dULL,
        0x403f2e1d0cab9f8eULL,
    };
    const uint64_t block2_seeds[AI_MEMBLOCK_CMO_TRIPLE_ROUNDS] = {
        0x02468ace13579bdfULL,
        0x13579bdf2468ace0ULL,
        0x2468ace03579bdf1ULL,
    };
    const uint64_t block0_older_store_values[AI_MEMBLOCK_CMO_TRIPLE_ROUNDS] = {
        0x1234fedc5678ba90ULL,
        0x23450fed6789cba1ULL,
        0x345610fe789adcb2ULL,
    };
    const uint64_t block0_second_store_values[AI_MEMBLOCK_CMO_TRIPLE_ROUNDS] = {
        0x90ab7856dcfe3412ULL,
        0xa1bc8967ed0f4523ULL,
        0xb2cd9a78fe105634ULL,
    };
    const uint64_t block0_third_store_values[AI_MEMBLOCK_CMO_TRIPLE_ROUNDS] = {
        0x0f1e2d3c4b5a6978ULL,
        0x102f3e4d5c6b7a89ULL,
        0x21403f5e6d7c8b9aULL,
    };
    const uint64_t block0_fourth_store_values[AI_MEMBLOCK_CMO_TRIPLE_ROUNDS] = {
        0xfedcba9012345678ULL,
        0x0fedcba123456789ULL,
        0x10fedcb23456789aULL,
    };
    const uint64_t before_guards[AI_MEMBLOCK_CMO_TRIPLE_ROUNDS] = {
        0x13572468ace0bdf9ULL,
        0x24683579bdf1cef0ULL,
        0x3579468ace02df01ULL,
    };
    const uint64_t guard01_values[AI_MEMBLOCK_CMO_TRIPLE_ROUNDS] = {
        0x9fdb0eca86427531ULL,
        0xa0ec1fdb97538642ULL,
        0xb1fd20eca8649753ULL,
    };
    const uint64_t guard12_values[AI_MEMBLOCK_CMO_TRIPLE_ROUNDS] = {
        0x0102030405060708ULL,
        0x1112131415161718ULL,
        0x2122232425262728ULL,
    };
    const uint64_t after_guards[AI_MEMBLOCK_CMO_TRIPLE_ROUNDS] = {
        0x8877665544332211ULL,
        0x9988776655443322ULL,
        0xaa99887766554433ULL,
    };
    const uint64_t marker_seeds[AI_MEMBLOCK_CMO_TRIPLE_ROUNDS] = {
        0xca11ab1e0ddc0ffeULL,
        0xdb22bc2f1eed100fULL,
        0xec33cd302ffe2110ULL,
    };
    const uint64_t marker_updates[AI_MEMBLOCK_CMO_TRIPLE_ROUNDS] = {
        0xeff0cdd0e1ba11acULL,
        0xf001dee1f2cb22bdULL,
        0x0112eff203dc33ceULL,
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
        AI_ASSERT_NO_EXCEPT_AND("before the four-generation clean-inval-clean-zero CMO probe starts, all source blocks, guards, and marker are readable",
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
        ai_memblock_cmo_triple_three_block_store_cbom_store_inval_store_cbom_store_zero_window(
            block0_vaddr, block1_vaddr, block2_vaddr, marker_vaddr,
            block0_older_store_values[round],
            block0_second_store_values[round],
            block0_third_store_values[round],
            block0_fourth_store_values[round], marker_updates[round],
            &block0_older_observed, &block0_second_observed,
            &block0_third_observed, &block0_fourth_observed,
            &block0_zero_observed, &block1_observed, &block2_observed,
            &marker_observed);
        AI_ASSERT_NO_EXCEPT_AND("under three-block CMO pressure, final CBO.ZERO clears all four same-block store generations after CBOM, CBO.INVAL, and CBOM activity",
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
        AI_ASSERT_NO_EXCEPT_AND("after the four-generation clean-inval-clean-zero CMO probe, block0 is fully zero, block1 preserves seed, block2 is zero, and guards remain intact",
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
        TEST_ASSERT("backing memory after the four-generation clean-inval-clean-zero CMO probe keeps block0 zero, block1 seed payload, block2 zero, marker update, and all guards intact",
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

bool ai_micro_hs_cboinval_then_cbozero_then_cbom_no_stale_triple_cmo_owner_corner()
{
    TEST_START();

    const enum test_page source_page = SWITCH1;
    const enum test_page marker_page = SWITCH2;
    const size_t load_word = 4U;
    const uintptr_t source_vaddr = hs_page_base(source_page) + 0x1300U;
    const uintptr_t source_paddr = phys_page_base(source_page) + 0x1300U;
    const uintptr_t inval_vaddr = source_vaddr + 2U * sizeof(uint64_t);
    const uintptr_t zero_vaddr = source_vaddr + 6U * sizeof(uint64_t);
    const uintptr_t clean_vaddr = source_vaddr + 1U * sizeof(uint64_t);
    const uintptr_t flush_vaddr = source_vaddr +
        (AI_MEMBLOCK_CMO_TRIPLE_BLOCK_WORDS - 1U) * sizeof(uint64_t);
    const uintptr_t load_vaddr = source_vaddr + load_word * sizeof(uint64_t);
    const uintptr_t source_tail_vaddr = flush_vaddr;
    const uintptr_t before_guard_vaddr = source_vaddr - sizeof(uint64_t);
    const uintptr_t after_guard_vaddr = source_vaddr +
        AI_MEMBLOCK_CMO_TRIPLE_BLOCK_WORDS * sizeof(uint64_t);
    const uintptr_t before_guard_paddr = source_paddr - sizeof(uint64_t);
    const uintptr_t after_guard_paddr = source_paddr +
        AI_MEMBLOCK_CMO_TRIPLE_BLOCK_WORDS * sizeof(uint64_t);
    const uintptr_t marker_vaddr = hs_page_base(marker_page) + 0x1300U;
    const uintptr_t marker_paddr = phys_page_base(marker_page) + 0x1300U;
    const uintptr_t marker_guard_vaddr = marker_vaddr + sizeof(uint64_t);
    const uintptr_t marker_guard_paddr = marker_paddr + sizeof(uint64_t);
    const uint64_t block_seeds[AI_MEMBLOCK_CMO_TRIPLE_ROUNDS] = {
        0x456789abcdef0123ULL,
        0x56789abcdef01234ULL,
        0x6789abcdef012345ULL,
    };
    const uint64_t before_guards[AI_MEMBLOCK_CMO_TRIPLE_ROUNDS] = {
        0x13579bdf2468ace0ULL,
        0x2468ace03579bdf1ULL,
        0x3579bdf1468ace02ULL,
    };
    const uint64_t after_guards[AI_MEMBLOCK_CMO_TRIPLE_ROUNDS] = {
        0x0f1e2d3c4b5a6978ULL,
        0x1e2d3c4b5a697887ULL,
        0x2d3c4b5a69788796ULL,
    };
    const uint64_t marker_seeds[AI_MEMBLOCK_CMO_TRIPLE_ROUNDS] = {
        0x1021324354657687ULL,
        0x2132435465768798ULL,
        0x32435465768798a9ULL,
    };
    const uint64_t marker_updates[AI_MEMBLOCK_CMO_TRIPLE_ROUNDS] = {
        0x89abcdef01234567ULL,
        0x9abcdef012345678ULL,
        0xabcdef0123456789ULL,
    };
    const uint64_t marker_guards[AI_MEMBLOCK_CMO_TRIPLE_ROUNDS] = {
        0xf0e1d2c3b4a59687ULL,
        0xe1d2c3b4a5968778ULL,
        0xd2c3b4a596877869ULL,
    };

    ai_memblock_cmo_triple_prepare_hs_env();

    for (size_t round = 0; round < AI_MEMBLOCK_CMO_TRIPLE_ROUNDS; ++round) {
        uint64_t load_observed = 0ULL;
        uint64_t marker_observed = 0ULL;
        const uint64_t expected_load_before = block_seeds[round] +
            load_word * 0x1111111111111111ULL;
        const uint64_t expected_tail_before = block_seeds[round] +
            (AI_MEMBLOCK_CMO_TRIPLE_BLOCK_WORDS - 1U) *
            0x1111111111111111ULL;

        goto_priv(PRIV_M);
        ai_memblock_cmo_triple_seed_block(source_paddr, block_seeds[round]);
        write64(before_guard_paddr, before_guards[round]);
        write64(after_guard_paddr, after_guards[round]);
        write64(marker_paddr, marker_seeds[round]);
        write64(marker_guard_paddr, marker_guards[round]);

        goto_priv(PRIV_HS);
        TEST_SETUP_EXCEPT();
        AI_ASSERT_NO_EXCEPT_AND("before the CBO.INVAL -> CBO.ZERO -> CBOM triple owner chain starts, source block and marker are readable",
            ld(source_vaddr) == block_seeds[round] &&
            ld(load_vaddr) == expected_load_before &&
            ld(source_tail_vaddr) == expected_tail_before &&
            ld(marker_vaddr) == marker_seeds[round]
        );

        TEST_SETUP_EXCEPT();
        ai_memblock_cmo_triple_inval_zero_cbom_window(inval_vaddr,
            zero_vaddr, clean_vaddr, flush_vaddr, load_vaddr, marker_vaddr,
            marker_updates[round], &load_observed, &marker_observed);
        AI_ASSERT_NO_EXCEPT_AND("CBO.INVAL -> CBO.ZERO -> CBOM clean/flush keeps the middle zero owner committed before the final CMO and marker progress",
            load_observed == 0ULL &&
            marker_observed == marker_updates[round]
        );

        TEST_SETUP_EXCEPT();
        AI_ASSERT_NO_EXCEPT_AND("the CBO.INVAL -> CBO.ZERO -> CBOM triple owner chain leaves source block zero and all guards intact",
            ld(source_vaddr) == 0ULL &&
            ld(load_vaddr) == 0ULL &&
            ld(source_tail_vaddr) == 0ULL &&
            ld(before_guard_vaddr) == before_guards[round] &&
            ld(after_guard_vaddr) == after_guards[round] &&
            ld(marker_guard_vaddr) == marker_guards[round]
        );

        goto_priv(PRIV_M);
        TEST_ASSERT("backing memory after the CBO.INVAL -> CBO.ZERO -> CBOM triple owner chain contains a fully zeroed source block, marker update, and unchanged guards",
            ai_memblock_cmo_triple_block_all_zero(source_paddr) &&
            read64(before_guard_paddr) == before_guards[round] &&
            read64(after_guard_paddr) == after_guards[round] &&
            read64(marker_paddr) == marker_updates[round] &&
            read64(marker_guard_paddr) == marker_guards[round]
        );
    }

    ai_memblock_cmo_triple_reset_hs_env();

    TEST_END();
}

bool ai_micro_hs_cbom_then_cbozero_then_cboinval_no_stale_triple_cmo_owner_corner()
{
    TEST_START();

    const enum test_page source_page = SWITCH1;
    const enum test_page marker_page = SWITCH2;
    const size_t load_word = 5U;
    const uintptr_t source_vaddr = hs_page_base(source_page) + 0x1380U;
    const uintptr_t source_paddr = phys_page_base(source_page) + 0x1380U;
    const uintptr_t clean_vaddr = source_vaddr + 1U * sizeof(uint64_t);
    const uintptr_t flush_vaddr = source_vaddr +
        (AI_MEMBLOCK_CMO_TRIPLE_BLOCK_WORDS - 1U) * sizeof(uint64_t);
    const uintptr_t zero_vaddr = source_vaddr + 4U * sizeof(uint64_t);
    const uintptr_t inval_vaddr = source_vaddr + 6U * sizeof(uint64_t);
    const uintptr_t load_vaddr = source_vaddr + load_word * sizeof(uint64_t);
    const uintptr_t source_tail_vaddr = flush_vaddr;
    const uintptr_t before_guard_vaddr = source_vaddr - sizeof(uint64_t);
    const uintptr_t after_guard_vaddr = source_vaddr +
        AI_MEMBLOCK_CMO_TRIPLE_BLOCK_WORDS * sizeof(uint64_t);
    const uintptr_t before_guard_paddr = source_paddr - sizeof(uint64_t);
    const uintptr_t after_guard_paddr = source_paddr +
        AI_MEMBLOCK_CMO_TRIPLE_BLOCK_WORDS * sizeof(uint64_t);
    const uintptr_t marker_vaddr = hs_page_base(marker_page) + 0x1380U;
    const uintptr_t marker_paddr = phys_page_base(marker_page) + 0x1380U;
    const uintptr_t marker_guard_vaddr = marker_vaddr + sizeof(uint64_t);
    const uintptr_t marker_guard_paddr = marker_paddr + sizeof(uint64_t);
    const uint64_t block_seeds[AI_MEMBLOCK_CMO_TRIPLE_ROUNDS] = {
        0xbadc0ffee0ddf00dULL,
        0xcbed100ff1eef11eULL,
        0xdcfe211002ff022fULL,
    };
    const uint64_t before_guards[AI_MEMBLOCK_CMO_TRIPLE_ROUNDS] = {
        0x55aa33cc77ee11ffULL,
        0x66bb44dd88ff2200ULL,
        0x77cc55ee99003311ULL,
    };
    const uint64_t after_guards[AI_MEMBLOCK_CMO_TRIPLE_ROUNDS] = {
        0x1122aabb3344ccddULL,
        0x2233bbcc4455ddeeULL,
        0x3344ccdd5566eeffULL,
    };
    const uint64_t marker_seeds[AI_MEMBLOCK_CMO_TRIPLE_ROUNDS] = {
        0x0badf00d55aa7711ULL,
        0x1cafe11e66bb8822ULL,
        0x2db0f22f77cc9933ULL,
    };
    const uint64_t marker_updates[AI_MEMBLOCK_CMO_TRIPLE_ROUNDS] = {
        0x0102030405060708ULL,
        0x1112131415161718ULL,
        0x2122232425262728ULL,
    };
    const uint64_t marker_guards[AI_MEMBLOCK_CMO_TRIPLE_ROUNDS] = {
        0x8877665544332211ULL,
        0x7766554433221100ULL,
        0x66554433221100ffULL,
    };

    ai_memblock_cmo_triple_prepare_hs_env();

    for (size_t round = 0; round < AI_MEMBLOCK_CMO_TRIPLE_ROUNDS; ++round) {
        uint64_t load_observed = 0ULL;
        uint64_t marker_observed = 0ULL;
        const uint64_t expected_load_before = block_seeds[round] +
            load_word * 0x1111111111111111ULL;
        const uint64_t expected_tail_before = block_seeds[round] +
            (AI_MEMBLOCK_CMO_TRIPLE_BLOCK_WORDS - 1U) *
            0x1111111111111111ULL;

        goto_priv(PRIV_M);
        ai_memblock_cmo_triple_seed_block(source_paddr, block_seeds[round]);
        write64(before_guard_paddr, before_guards[round]);
        write64(after_guard_paddr, after_guards[round]);
        write64(marker_paddr, marker_seeds[round]);
        write64(marker_guard_paddr, marker_guards[round]);

        goto_priv(PRIV_HS);
        TEST_SETUP_EXCEPT();
        AI_ASSERT_NO_EXCEPT_AND("before the CBOM -> CBO.ZERO -> CBO.INVAL triple owner chain starts, source block and marker are readable",
            ld(source_vaddr) == block_seeds[round] &&
            ld(load_vaddr) == expected_load_before &&
            ld(source_tail_vaddr) == expected_tail_before &&
            ld(marker_vaddr) == marker_seeds[round]
        );

        TEST_SETUP_EXCEPT();
        ai_memblock_cmo_triple_cbom_zero_inval_window(clean_vaddr,
            flush_vaddr, zero_vaddr, inval_vaddr, load_vaddr, marker_vaddr,
            marker_updates[round], &load_observed, &marker_observed);
        AI_ASSERT_NO_EXCEPT_AND("CBOM clean/flush -> CBO.ZERO -> CBO.INVAL keeps the middle zero owner committed and prevents the final inval from restoring stale data",
            load_observed == 0ULL &&
            marker_observed == marker_updates[round]
        );

        TEST_SETUP_EXCEPT();
        AI_ASSERT_NO_EXCEPT_AND("the CBOM -> CBO.ZERO -> CBO.INVAL triple owner chain leaves source block zero and all guards intact",
            ld(source_vaddr) == 0ULL &&
            ld(load_vaddr) == 0ULL &&
            ld(source_tail_vaddr) == 0ULL &&
            ld(before_guard_vaddr) == before_guards[round] &&
            ld(after_guard_vaddr) == after_guards[round] &&
            ld(marker_guard_vaddr) == marker_guards[round]
        );

        goto_priv(PRIV_M);
        TEST_ASSERT("backing memory after the CBOM -> CBO.ZERO -> CBO.INVAL triple owner chain contains a fully zeroed source block, marker update, and unchanged guards",
            ai_memblock_cmo_triple_block_all_zero(source_paddr) &&
            read64(before_guard_paddr) == before_guards[round] &&
            read64(after_guard_paddr) == after_guards[round] &&
            read64(marker_paddr) == marker_updates[round] &&
            read64(marker_guard_paddr) == marker_guards[round]
        );
    }

    ai_memblock_cmo_triple_reset_hs_env();

    TEST_END();
}

bool ai_micro_hs_cbozero_then_cbom_then_cboinval_no_stale_triple_cmo_owner_corner()
{
    TEST_START();

    const enum test_page source_page = SWITCH1;
    const enum test_page marker_page = SWITCH2;
    const size_t load_word = 2U;
    const uintptr_t source_vaddr = hs_page_base(source_page) + 0x1280U;
    const uintptr_t source_paddr = phys_page_base(source_page) + 0x1280U;
    const uintptr_t zero_vaddr = source_vaddr + 1U * sizeof(uint64_t);
    const uintptr_t clean_vaddr = source_vaddr + 2U * sizeof(uint64_t);
    const uintptr_t flush_vaddr = source_vaddr +
        (AI_MEMBLOCK_CMO_TRIPLE_BLOCK_WORDS - 1U) * sizeof(uint64_t);
    const uintptr_t inval_vaddr = source_vaddr + 5U * sizeof(uint64_t);
    const uintptr_t load_vaddr = source_vaddr + load_word * sizeof(uint64_t);
    const uintptr_t source_tail_vaddr = flush_vaddr;
    const uintptr_t before_guard_vaddr = source_vaddr - sizeof(uint64_t);
    const uintptr_t after_guard_vaddr = source_vaddr +
        AI_MEMBLOCK_CMO_TRIPLE_BLOCK_WORDS * sizeof(uint64_t);
    const uintptr_t before_guard_paddr = source_paddr - sizeof(uint64_t);
    const uintptr_t after_guard_paddr = source_paddr +
        AI_MEMBLOCK_CMO_TRIPLE_BLOCK_WORDS * sizeof(uint64_t);
    const uintptr_t marker_vaddr = hs_page_base(marker_page) + 0x1280U;
    const uintptr_t marker_paddr = phys_page_base(marker_page) + 0x1280U;
    const uintptr_t marker_guard_vaddr = marker_vaddr + sizeof(uint64_t);
    const uintptr_t marker_guard_paddr = marker_paddr + sizeof(uint64_t);
    const uint64_t block_seeds[AI_MEMBLOCK_CMO_TRIPLE_ROUNDS] = {
        0x89abcdef01234567ULL,
        0x9abcdef012345678ULL,
        0xabcdef0123456789ULL,
    };
    const uint64_t before_guards[AI_MEMBLOCK_CMO_TRIPLE_ROUNDS] = {
        0x0ace2468bdf13579ULL,
        0x1bdf3579ce02468aULL,
        0x2ce0468adf13579bULL,
    };
    const uint64_t after_guards[AI_MEMBLOCK_CMO_TRIPLE_ROUNDS] = {
        0xffeeddccbbaa9988ULL,
        0xeeddccbbaa998877ULL,
        0xddccbbaa99887766ULL,
    };
    const uint64_t marker_seeds[AI_MEMBLOCK_CMO_TRIPLE_ROUNDS] = {
        0x0123456789abcdefULL,
        0x123456789abcdef0ULL,
        0x23456789abcdef01ULL,
    };
    const uint64_t marker_updates[AI_MEMBLOCK_CMO_TRIPLE_ROUNDS] = {
        0xfedcba9876543210ULL,
        0xedcba9876543210fULL,
        0xdcba9876543210feULL,
    };
    const uint64_t marker_guards[AI_MEMBLOCK_CMO_TRIPLE_ROUNDS] = {
        0xa5a55a5ac3c33c3cULL,
        0xb6b66b6bd4d44d4dULL,
        0xc7c77c7ce5e55e5eULL,
    };

    ai_memblock_cmo_triple_prepare_hs_env();

    for (size_t round = 0; round < AI_MEMBLOCK_CMO_TRIPLE_ROUNDS; ++round) {
        uint64_t load_observed = 0ULL;
        uint64_t marker_observed = 0ULL;
        const uint64_t expected_load_before = block_seeds[round] +
            load_word * 0x1111111111111111ULL;
        const uint64_t expected_tail_before = block_seeds[round] +
            (AI_MEMBLOCK_CMO_TRIPLE_BLOCK_WORDS - 1U) *
            0x1111111111111111ULL;

        goto_priv(PRIV_M);
        ai_memblock_cmo_triple_seed_block(source_paddr, block_seeds[round]);
        write64(before_guard_paddr, before_guards[round]);
        write64(after_guard_paddr, after_guards[round]);
        write64(marker_paddr, marker_seeds[round]);
        write64(marker_guard_paddr, marker_guards[round]);

        goto_priv(PRIV_HS);
        TEST_SETUP_EXCEPT();
        AI_ASSERT_NO_EXCEPT_AND("before the CBO.ZERO -> CBOM -> CBO.INVAL triple owner chain starts, source block and marker are readable",
            ld(source_vaddr) == block_seeds[round] &&
            ld(load_vaddr) == expected_load_before &&
            ld(source_tail_vaddr) == expected_tail_before &&
            ld(marker_vaddr) == marker_seeds[round]
        );

        TEST_SETUP_EXCEPT();
        ai_memblock_cmo_triple_zero_cbom_inval_window(zero_vaddr,
            clean_vaddr, flush_vaddr, inval_vaddr, load_vaddr, marker_vaddr,
            marker_updates[round], &load_observed, &marker_observed);
        AI_ASSERT_NO_EXCEPT_AND("CBO.ZERO -> CBOM clean/flush -> CBO.INVAL releases each CMO owner in order so the zero image is not restored and marker survives",
            load_observed == 0ULL &&
            marker_observed == marker_updates[round]
        );

        TEST_SETUP_EXCEPT();
        AI_ASSERT_NO_EXCEPT_AND("the CBO.ZERO -> CBOM -> CBO.INVAL triple owner chain leaves source block zero and all guards intact",
            ld(source_vaddr) == 0ULL &&
            ld(load_vaddr) == 0ULL &&
            ld(source_tail_vaddr) == 0ULL &&
            ld(before_guard_vaddr) == before_guards[round] &&
            ld(after_guard_vaddr) == after_guards[round] &&
            ld(marker_guard_vaddr) == marker_guards[round]
        );

        goto_priv(PRIV_M);
        TEST_ASSERT("backing memory after the CBO.ZERO -> CBOM -> CBO.INVAL triple owner chain contains a fully zeroed source block, marker update, and unchanged guards",
            ai_memblock_cmo_triple_block_all_zero(source_paddr) &&
            read64(before_guard_paddr) == before_guards[round] &&
            read64(after_guard_paddr) == after_guards[round] &&
            read64(marker_paddr) == marker_updates[round] &&
            read64(marker_guard_paddr) == marker_guards[round]
        );
    }

    ai_memblock_cmo_triple_reset_hs_env();

    TEST_END();
}

bool ai_micro_hs_cbozero_then_cboinval_then_cbom_no_stale_triple_cmo_owner_corner()
{
    TEST_START();

    const enum test_page source_page = SWITCH1;
    const enum test_page marker_page = SWITCH2;
    const size_t load_word = 4U;
    const uintptr_t source_vaddr = hs_page_base(source_page) + 0x1400U;
    const uintptr_t source_paddr = phys_page_base(source_page) + 0x1400U;
    const uintptr_t zero_vaddr = source_vaddr + 3U * sizeof(uint64_t);
    const uintptr_t inval_vaddr = source_vaddr + 6U * sizeof(uint64_t);
    const uintptr_t clean_vaddr = source_vaddr + 1U * sizeof(uint64_t);
    const uintptr_t flush_vaddr = source_vaddr +
        (AI_MEMBLOCK_CMO_TRIPLE_BLOCK_WORDS - 1U) * sizeof(uint64_t);
    const uintptr_t load_vaddr = source_vaddr + load_word * sizeof(uint64_t);
    const uintptr_t source_tail_vaddr = flush_vaddr;
    const uintptr_t before_guard_vaddr = source_vaddr - sizeof(uint64_t);
    const uintptr_t after_guard_vaddr = source_vaddr +
        AI_MEMBLOCK_CMO_TRIPLE_BLOCK_WORDS * sizeof(uint64_t);
    const uintptr_t before_guard_paddr = source_paddr - sizeof(uint64_t);
    const uintptr_t after_guard_paddr = source_paddr +
        AI_MEMBLOCK_CMO_TRIPLE_BLOCK_WORDS * sizeof(uint64_t);
    const uintptr_t marker_vaddr = hs_page_base(marker_page) + 0x1400U;
    const uintptr_t marker_paddr = phys_page_base(marker_page) + 0x1400U;
    const uintptr_t marker_guard_vaddr = marker_vaddr + sizeof(uint64_t);
    const uintptr_t marker_guard_paddr = marker_paddr + sizeof(uint64_t);
    const uint64_t block_seeds[AI_MEMBLOCK_CMO_TRIPLE_ROUNDS] = {
        0x0a1b2c3d4e5f6071ULL,
        0x1b2c3d4e5f607182ULL,
        0x2c3d4e5f60718293ULL,
    };
    const uint64_t before_guards[AI_MEMBLOCK_CMO_TRIPLE_ROUNDS] = {
        0x1122334455667788ULL,
        0x2233445566778899ULL,
        0x33445566778899aaULL,
    };
    const uint64_t after_guards[AI_MEMBLOCK_CMO_TRIPLE_ROUNDS] = {
        0x8877665544332211ULL,
        0x9988776655443322ULL,
        0xaa99887766554433ULL,
    };
    const uint64_t marker_seeds[AI_MEMBLOCK_CMO_TRIPLE_ROUNDS] = {
        0x13572468ace0bdf1ULL,
        0x24683579bdf1ce02ULL,
        0x3579468ace02df13ULL,
    };
    const uint64_t marker_updates[AI_MEMBLOCK_CMO_TRIPLE_ROUNDS] = {
        0xf1e2d3c4b5a69788ULL,
        0xe2d3c4b5a6978879ULL,
        0xd3c4b5a69788796aULL,
    };
    const uint64_t marker_guards[AI_MEMBLOCK_CMO_TRIPLE_ROUNDS] = {
        0x55aa771199bb33ddULL,
        0x66bb8822aacc44eeULL,
        0x77cc9933bbdd55ffULL,
    };

    ai_memblock_cmo_triple_prepare_hs_env();

    for (size_t round = 0; round < AI_MEMBLOCK_CMO_TRIPLE_ROUNDS; ++round) {
        uint64_t load_observed = 0ULL;
        uint64_t marker_observed = 0ULL;
        const uint64_t expected_load_before = block_seeds[round] +
            load_word * 0x1111111111111111ULL;
        const uint64_t expected_tail_before = block_seeds[round] +
            (AI_MEMBLOCK_CMO_TRIPLE_BLOCK_WORDS - 1U) *
            0x1111111111111111ULL;

        goto_priv(PRIV_M);
        ai_memblock_cmo_triple_seed_block(source_paddr, block_seeds[round]);
        write64(before_guard_paddr, before_guards[round]);
        write64(after_guard_paddr, after_guards[round]);
        write64(marker_paddr, marker_seeds[round]);
        write64(marker_guard_paddr, marker_guards[round]);

        goto_priv(PRIV_HS);
        TEST_SETUP_EXCEPT();
        AI_ASSERT_NO_EXCEPT_AND("before the CBO.ZERO -> CBO.INVAL -> CBOM triple owner chain starts, source block and marker are readable",
            ld(source_vaddr) == block_seeds[round] &&
            ld(load_vaddr) == expected_load_before &&
            ld(source_tail_vaddr) == expected_tail_before &&
            ld(marker_vaddr) == marker_seeds[round]
        );

        TEST_SETUP_EXCEPT();
        ai_memblock_cmo_triple_zero_inval_cbom_window(zero_vaddr,
            inval_vaddr, clean_vaddr, flush_vaddr, load_vaddr, marker_vaddr,
            marker_updates[round], &load_observed, &marker_observed);
        AI_ASSERT_NO_EXCEPT_AND("CBO.ZERO -> CBO.INVAL -> CBOM clean/flush keeps the zero image visible after inval and final CMO owner retirement",
            load_observed == 0ULL &&
            marker_observed == marker_updates[round]
        );

        TEST_SETUP_EXCEPT();
        AI_ASSERT_NO_EXCEPT_AND("the CBO.ZERO -> CBO.INVAL -> CBOM triple owner chain leaves source block zero and all guards intact",
            ld(source_vaddr) == 0ULL &&
            ld(load_vaddr) == 0ULL &&
            ld(source_tail_vaddr) == 0ULL &&
            ld(before_guard_vaddr) == before_guards[round] &&
            ld(after_guard_vaddr) == after_guards[round] &&
            ld(marker_guard_vaddr) == marker_guards[round]
        );

        goto_priv(PRIV_M);
        TEST_ASSERT("backing memory after the CBO.ZERO -> CBO.INVAL -> CBOM triple owner chain contains a fully zeroed source block, marker update, and unchanged guards",
            ai_memblock_cmo_triple_block_all_zero(source_paddr) &&
            read64(before_guard_paddr) == before_guards[round] &&
            read64(after_guard_paddr) == after_guards[round] &&
            read64(marker_paddr) == marker_updates[round] &&
            read64(marker_guard_paddr) == marker_guards[round]
        );
    }

    ai_memblock_cmo_triple_reset_hs_env();

    TEST_END();
}

bool ai_micro_hs_cbom_then_cboinval_then_cbozero_no_stale_triple_cmo_owner_corner()
{
    TEST_START();

    const enum test_page source_page = SWITCH1;
    const enum test_page marker_page = SWITCH2;
    const size_t load_word = 5U;
    const uintptr_t source_vaddr = hs_page_base(source_page) + 0x1480U;
    const uintptr_t source_paddr = phys_page_base(source_page) + 0x1480U;
    const uintptr_t clean_vaddr = source_vaddr + 1U * sizeof(uint64_t);
    const uintptr_t flush_vaddr = source_vaddr +
        (AI_MEMBLOCK_CMO_TRIPLE_BLOCK_WORDS - 1U) * sizeof(uint64_t);
    const uintptr_t inval_vaddr = source_vaddr + 2U * sizeof(uint64_t);
    const uintptr_t zero_vaddr = source_vaddr + 6U * sizeof(uint64_t);
    const uintptr_t load_vaddr = source_vaddr + load_word * sizeof(uint64_t);
    const uintptr_t source_tail_vaddr = flush_vaddr;
    const uintptr_t before_guard_vaddr = source_vaddr - sizeof(uint64_t);
    const uintptr_t after_guard_vaddr = source_vaddr +
        AI_MEMBLOCK_CMO_TRIPLE_BLOCK_WORDS * sizeof(uint64_t);
    const uintptr_t before_guard_paddr = source_paddr - sizeof(uint64_t);
    const uintptr_t after_guard_paddr = source_paddr +
        AI_MEMBLOCK_CMO_TRIPLE_BLOCK_WORDS * sizeof(uint64_t);
    const uintptr_t marker_vaddr = hs_page_base(marker_page) + 0x1480U;
    const uintptr_t marker_paddr = phys_page_base(marker_page) + 0x1480U;
    const uintptr_t marker_guard_vaddr = marker_vaddr + sizeof(uint64_t);
    const uintptr_t marker_guard_paddr = marker_paddr + sizeof(uint64_t);
    const uint64_t block_seeds[AI_MEMBLOCK_CMO_TRIPLE_ROUNDS] = {
        0x4567fedcba890123ULL,
        0x56780fedcb9a1234ULL,
        0x67891fedcba2345ULL,
    };
    const uint64_t before_guards[AI_MEMBLOCK_CMO_TRIPLE_ROUNDS] = {
        0x0badc0ffee0ddf00ULL,
        0x1cafe11eff1ee011ULL,
        0x2db0f22f002ff122ULL,
    };
    const uint64_t after_guards[AI_MEMBLOCK_CMO_TRIPLE_ROUNDS] = {
        0xc001d00dfeedfaceULL,
        0xd112e11eff00dbdfULL,
        0xe223f22f0011ece0ULL,
    };
    const uint64_t marker_seeds[AI_MEMBLOCK_CMO_TRIPLE_ROUNDS] = {
        0x0101010123232323ULL,
        0x1212121234343434ULL,
        0x2323232345454545ULL,
    };
    const uint64_t marker_updates[AI_MEMBLOCK_CMO_TRIPLE_ROUNDS] = {
        0xa1b2c3d4e5f60718ULL,
        0xb2c3d4e5f6071829ULL,
        0xc3d4e5f60718293aULL,
    };
    const uint64_t marker_guards[AI_MEMBLOCK_CMO_TRIPLE_ROUNDS] = {
        0xabcdefabcdef1357ULL,
        0xbcdefabcdef2468ULL,
        0xcdefabcdef3579ULL,
    };

    ai_memblock_cmo_triple_prepare_hs_env();

    for (size_t round = 0; round < AI_MEMBLOCK_CMO_TRIPLE_ROUNDS; ++round) {
        uint64_t load_observed = 0ULL;
        uint64_t marker_observed = 0ULL;
        const uint64_t expected_load_before = block_seeds[round] +
            load_word * 0x1111111111111111ULL;
        const uint64_t expected_tail_before = block_seeds[round] +
            (AI_MEMBLOCK_CMO_TRIPLE_BLOCK_WORDS - 1U) *
            0x1111111111111111ULL;

        goto_priv(PRIV_M);
        ai_memblock_cmo_triple_seed_block(source_paddr, block_seeds[round]);
        write64(before_guard_paddr, before_guards[round]);
        write64(after_guard_paddr, after_guards[round]);
        write64(marker_paddr, marker_seeds[round]);
        write64(marker_guard_paddr, marker_guards[round]);

        goto_priv(PRIV_HS);
        TEST_SETUP_EXCEPT();
        AI_ASSERT_NO_EXCEPT_AND("before the CBOM -> CBO.INVAL -> CBO.ZERO triple owner chain starts, source block and marker are readable",
            ld(source_vaddr) == block_seeds[round] &&
            ld(load_vaddr) == expected_load_before &&
            ld(source_tail_vaddr) == expected_tail_before &&
            ld(marker_vaddr) == marker_seeds[round]
        );

        TEST_SETUP_EXCEPT();
        ai_memblock_cmo_triple_cbom_inval_zero_window(clean_vaddr,
            flush_vaddr, inval_vaddr, zero_vaddr, load_vaddr, marker_vaddr,
            marker_updates[round], &load_observed, &marker_observed);
        AI_ASSERT_NO_EXCEPT_AND("CBOM clean/flush -> CBO.INVAL -> CBO.ZERO releases CMO and inval owners before the final zero owner commits",
            load_observed == 0ULL &&
            marker_observed == marker_updates[round]
        );

        TEST_SETUP_EXCEPT();
        AI_ASSERT_NO_EXCEPT_AND("the CBOM -> CBO.INVAL -> CBO.ZERO triple owner chain leaves source block zero and all guards intact",
            ld(source_vaddr) == 0ULL &&
            ld(load_vaddr) == 0ULL &&
            ld(source_tail_vaddr) == 0ULL &&
            ld(before_guard_vaddr) == before_guards[round] &&
            ld(after_guard_vaddr) == after_guards[round] &&
            ld(marker_guard_vaddr) == marker_guards[round]
        );

        goto_priv(PRIV_M);
        TEST_ASSERT("backing memory after the CBOM -> CBO.INVAL -> CBO.ZERO triple owner chain contains a fully zeroed source block, marker update, and unchanged guards",
            ai_memblock_cmo_triple_block_all_zero(source_paddr) &&
            read64(before_guard_paddr) == before_guards[round] &&
            read64(after_guard_paddr) == after_guards[round] &&
            read64(marker_paddr) == marker_updates[round] &&
            read64(marker_guard_paddr) == marker_guards[round]
        );
    }

    ai_memblock_cmo_triple_reset_hs_env();

    TEST_END();
}
