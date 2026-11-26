#include "ai_micro_memblock_cmo_triple_common.h"

bool ai_micro_hs_interleaved_three_block_store_zero_store_cbom_store_cbom_store_zero_four_generation_final_zero_corner()
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
    const uintptr_t block0_vaddr = hs_page_base(source_page) + 0x3840U;
    const uintptr_t block1_vaddr = hs_page_base(source_page) + 0x38c0U;
    const uintptr_t block2_vaddr = hs_page_base(source_page) + 0x3940U;
    const uintptr_t block0_paddr = phys_page_base(source_page) + 0x3840U;
    const uintptr_t block1_paddr = phys_page_base(source_page) + 0x38c0U;
    const uintptr_t block2_paddr = phys_page_base(source_page) + 0x3940U;
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
    const uintptr_t marker_vaddr = hs_page_base(marker_page) + 0x3840U;
    const uintptr_t marker_paddr = phys_page_base(marker_page) + 0x3840U;
    const uintptr_t marker_guard_vaddr = marker_vaddr + sizeof(uint64_t);
    const uintptr_t marker_guard_paddr = marker_paddr + sizeof(uint64_t);
    const uint64_t block0_seeds[AI_MEMBLOCK_CMO_TRIPLE_ROUNDS] = {
        0x3840500060007000ULL,
        0x4951611171118111ULL,
        0x5a62722282229222ULL,
    };
    const uint64_t block1_seeds[AI_MEMBLOCK_CMO_TRIPLE_ROUNDS] = {
        0x80706050384038c0ULL,
        0x9181716149514991ULL,
        0xa29282725a625aa2ULL,
    };
    const uint64_t block2_seeds[AI_MEMBLOCK_CMO_TRIPLE_ROUNDS] = {
        0x2468ace038403940ULL,
        0x3579bdf149514a11ULL,
        0x468ace025a625b22ULL,
    };
    const uint64_t block0_older_store_values[AI_MEMBLOCK_CMO_TRIPLE_ROUNDS] = {
        0x1100384022003300ULL,
        0x2211495133114411ULL,
        0x33225a6244225522ULL,
    };
    const uint64_t block0_second_store_values[AI_MEMBLOCK_CMO_TRIPLE_ROUNDS] = {
        0x0033002200384011ULL,
        0x1144113311495122ULL,
        0x22552244225a6233ULL,
    };
    const uint64_t block0_third_store_values[AI_MEMBLOCK_CMO_TRIPLE_ROUNDS] = {
        0xcafe3840feed3840ULL,
        0xdb0f49510ffe4951ULL,
        0xec205a62200f5a62ULL,
    };
    const uint64_t block0_fourth_store_values[AI_MEMBLOCK_CMO_TRIPLE_ROUNDS] = {
        0xc0013840deadbeefULL,
        0xd1124951efbedf00ULL,
        0xe2235a62f0cfe011ULL,
    };
    const uint64_t before_guards[AI_MEMBLOCK_CMO_TRIPLE_ROUNDS] = {
        0xc0013840d00d3840ULL,
        0xd1124951e11e4951ULL,
        0xe2235a62f22f5a62ULL,
    };
    const uint64_t guard01_values[AI_MEMBLOCK_CMO_TRIPLE_ROUNDS] = {
        0x3840d00d3840c001ULL,
        0x4951e11e4951d112ULL,
        0x5a62f22f5a62e223ULL,
    };
    const uint64_t guard12_values[AI_MEMBLOCK_CMO_TRIPLE_ROUNDS] = {
        0x31415926384038c0ULL,
        0x2718281849514991ULL,
        0x161803395a625aa2ULL,
    };
    const uint64_t after_guards[AI_MEMBLOCK_CMO_TRIPLE_ROUNDS] = {
        0x0838403940962695ULL,
        0x1949514a11818282ULL,
        0x2a5a625b22930381ULL,
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
        0x55aa55aa38403840ULL,
        0x66bb66bb49514951ULL,
        0x77cc77cc5a625a62ULL,
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
        AI_ASSERT_NO_EXCEPT_AND("before the four-generation zero-clean-clean-zero CMO probe starts, all source blocks, guards, and marker are readable",
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
        ai_memblock_cmo_triple_three_block_store_zero_store_cbom_store_cbom_store_zero_window(
            block0_vaddr, block1_vaddr, block2_vaddr, marker_vaddr,
            block0_older_store_values[round],
            block0_second_store_values[round],
            block0_third_store_values[round],
            block0_fourth_store_values[round], marker_updates[round],
            &block0_older_observed, &block0_second_observed,
            &block0_third_observed, &block0_fourth_observed,
            &block0_zero_observed, &block1_observed, &block2_observed,
            &marker_observed);
        AI_ASSERT_NO_EXCEPT_AND("under three-block CMO pressure, CBO.ZERO/CBOM/CBOM/CBO.ZERO clears all store generations",
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
        AI_ASSERT_NO_EXCEPT_AND("after the four-generation zero-clean-clean-zero CMO probe, block0 is fully zero, block1 preserves seed, block2 is zero, and guards remain intact",
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
        TEST_ASSERT("backing memory after the four-generation zero-clean-clean-zero CMO probe keeps block0 and block2 zero, block1 seed payload, marker update, and all guards intact",
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

bool ai_micro_hs_interleaved_three_block_store_zero_store_cbom_store_cbom_store_inval_four_generation_post_first_zero_corner()
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
    const uintptr_t block0_vaddr = hs_page_base(source_page) + 0x3740U;
    const uintptr_t block1_vaddr = hs_page_base(source_page) + 0x37c0U;
    const uintptr_t block2_vaddr = hs_page_base(source_page) + 0x3840U;
    const uintptr_t block0_paddr = phys_page_base(source_page) + 0x3740U;
    const uintptr_t block1_paddr = phys_page_base(source_page) + 0x37c0U;
    const uintptr_t block2_paddr = phys_page_base(source_page) + 0x3840U;
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
    const uintptr_t marker_vaddr = hs_page_base(marker_page) + 0x3740U;
    const uintptr_t marker_paddr = phys_page_base(marker_page) + 0x3740U;
    const uintptr_t marker_guard_vaddr = marker_vaddr + sizeof(uint64_t);
    const uintptr_t marker_guard_paddr = marker_paddr + sizeof(uint64_t);
    const uint64_t block0_seeds[AI_MEMBLOCK_CMO_TRIPLE_ROUNDS] = {
        0x3740500060007000ULL,
        0x4851611171118111ULL,
        0x5962722282229222ULL,
    };
    const uint64_t block1_seeds[AI_MEMBLOCK_CMO_TRIPLE_ROUNDS] = {
        0x80706050374037c0ULL,
        0x9181716148514891ULL,
        0xa2928272596259a2ULL,
    };
    const uint64_t block2_seeds[AI_MEMBLOCK_CMO_TRIPLE_ROUNDS] = {
        0x2468ace037403840ULL,
        0x3579bdf148514911ULL,
        0x468ace0259625a22ULL,
    };
    const uint64_t block0_older_store_values[AI_MEMBLOCK_CMO_TRIPLE_ROUNDS] = {
        0x1100374022003300ULL,
        0x2211485133114411ULL,
        0x3322596244225522ULL,
    };
    const uint64_t block0_second_store_values[AI_MEMBLOCK_CMO_TRIPLE_ROUNDS] = {
        0x0033002200374011ULL,
        0x1144113311485122ULL,
        0x2255224422596233ULL,
    };
    const uint64_t block0_third_store_values[AI_MEMBLOCK_CMO_TRIPLE_ROUNDS] = {
        0xcafe3740feed3740ULL,
        0xdb0f48510ffe4851ULL,
        0xec205962200f5962ULL,
    };
    const uint64_t block0_fourth_store_values[AI_MEMBLOCK_CMO_TRIPLE_ROUNDS] = {
        0xc0013740deadbeefULL,
        0xd1124851efbedf00ULL,
        0xe2235962f0cfe011ULL,
    };
    const uint64_t before_guards[AI_MEMBLOCK_CMO_TRIPLE_ROUNDS] = {
        0xc0013740d00d3740ULL,
        0xd1124851e11e4851ULL,
        0xe2235962f22f5962ULL,
    };
    const uint64_t guard01_values[AI_MEMBLOCK_CMO_TRIPLE_ROUNDS] = {
        0x3740d00d3740c001ULL,
        0x4851e11e4851d112ULL,
        0x5962f22f5962e223ULL,
    };
    const uint64_t guard12_values[AI_MEMBLOCK_CMO_TRIPLE_ROUNDS] = {
        0x31415926374037c0ULL,
        0x2718281848514891ULL,
        0x16180339596259a2ULL,
    };
    const uint64_t after_guards[AI_MEMBLOCK_CMO_TRIPLE_ROUNDS] = {
        0x0837403840962695ULL,
        0x1948514911818282ULL,
        0x2a59625a22930381ULL,
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
        0x55aa55aa37403740ULL,
        0x66bb66bb48514851ULL,
        0x77cc77cc59625962ULL,
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
        AI_ASSERT_NO_EXCEPT_AND("before the four-generation zero-clean-clean-inval CMO probe starts, all source blocks, guards, and marker are readable",
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
        ai_memblock_cmo_triple_three_block_store_zero_store_cbom_store_cbom_store_inval_window(
            block0_vaddr, block1_vaddr, block2_vaddr, marker_vaddr,
            block0_older_store_values[round],
            block0_second_store_values[round],
            block0_third_store_values[round],
            block0_fourth_store_values[round], marker_updates[round],
            &block0_older_observed, &block0_second_observed,
            &block0_third_observed, &block0_fourth_observed,
            &block0_zero_observed, &block1_observed, &block2_observed,
            &marker_observed);
        AI_ASSERT_NO_EXCEPT_AND("under three-block CMO pressure, CBO.ZERO/CBOM/CBOM/CBO.INVAL keeps all post-zero store generations visible",
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
        AI_ASSERT_NO_EXCEPT_AND("after the four-generation zero-clean-clean-inval CMO probe, block0 keeps only post-first-zero stores, block1 preserves seed, block2 is zero, and guards remain intact",
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
        TEST_ASSERT("backing memory after the four-generation zero-clean-clean-inval CMO probe keeps block0 zero except the second, third, and fourth post-first-zero stores, block1 seed payload, block2 zero, marker update, and all guards intact",
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

bool ai_micro_hs_interleaved_three_block_store_zero_store_cbom_store_cbom_store_cbom_four_generation_post_first_zero_corner()
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
    const uintptr_t block0_vaddr = hs_page_base(source_page) + 0x3640U;
    const uintptr_t block1_vaddr = hs_page_base(source_page) + 0x36c0U;
    const uintptr_t block2_vaddr = hs_page_base(source_page) + 0x3740U;
    const uintptr_t block0_paddr = phys_page_base(source_page) + 0x3640U;
    const uintptr_t block1_paddr = phys_page_base(source_page) + 0x36c0U;
    const uintptr_t block2_paddr = phys_page_base(source_page) + 0x3740U;
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
    const uintptr_t marker_vaddr = hs_page_base(marker_page) + 0x3640U;
    const uintptr_t marker_paddr = phys_page_base(marker_page) + 0x3640U;
    const uintptr_t marker_guard_vaddr = marker_vaddr + sizeof(uint64_t);
    const uintptr_t marker_guard_paddr = marker_paddr + sizeof(uint64_t);
    const uint64_t block0_seeds[AI_MEMBLOCK_CMO_TRIPLE_ROUNDS] = {
        0x3640500060007000ULL,
        0x4751611171118111ULL,
        0x5862722282229222ULL,
    };
    const uint64_t block1_seeds[AI_MEMBLOCK_CMO_TRIPLE_ROUNDS] = {
        0x80706050364036c0ULL,
        0x9181716147514791ULL,
        0xa2928272586258a2ULL,
    };
    const uint64_t block2_seeds[AI_MEMBLOCK_CMO_TRIPLE_ROUNDS] = {
        0x2468ace036403740ULL,
        0x3579bdf147514811ULL,
        0x468ace0258625922ULL,
    };
    const uint64_t block0_older_store_values[AI_MEMBLOCK_CMO_TRIPLE_ROUNDS] = {
        0x1100364022003300ULL,
        0x2211475133114411ULL,
        0x3322586244225522ULL,
    };
    const uint64_t block0_second_store_values[AI_MEMBLOCK_CMO_TRIPLE_ROUNDS] = {
        0x0033002200364011ULL,
        0x1144113311475122ULL,
        0x2255224422586233ULL,
    };
    const uint64_t block0_third_store_values[AI_MEMBLOCK_CMO_TRIPLE_ROUNDS] = {
        0xcafe3640feed3640ULL,
        0xdb0f47510ffe4751ULL,
        0xec205862200f5862ULL,
    };
    const uint64_t block0_fourth_store_values[AI_MEMBLOCK_CMO_TRIPLE_ROUNDS] = {
        0xc0013640deadbeefULL,
        0xd1124751efbedf00ULL,
        0xe2235862f0cfe011ULL,
    };
    const uint64_t before_guards[AI_MEMBLOCK_CMO_TRIPLE_ROUNDS] = {
        0xc0013640d00d3640ULL,
        0xd1124751e11e4751ULL,
        0xe2235862f22f5862ULL,
    };
    const uint64_t guard01_values[AI_MEMBLOCK_CMO_TRIPLE_ROUNDS] = {
        0x3640d00d3640c001ULL,
        0x4751e11e4751d112ULL,
        0x5862f22f5862e223ULL,
    };
    const uint64_t guard12_values[AI_MEMBLOCK_CMO_TRIPLE_ROUNDS] = {
        0x31415926364036c0ULL,
        0x2718281847514791ULL,
        0x16180339586258a2ULL,
    };
    const uint64_t after_guards[AI_MEMBLOCK_CMO_TRIPLE_ROUNDS] = {
        0x0836403740962695ULL,
        0x1947514811818282ULL,
        0x2a58625922930381ULL,
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
        0x55aa55aa36403640ULL,
        0x66bb66bb47514751ULL,
        0x77cc77cc58625862ULL,
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
        AI_ASSERT_NO_EXCEPT_AND("before the four-generation zero-clean-clean-clean CMO probe starts, all source blocks, guards, and marker are readable",
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
        ai_memblock_cmo_triple_three_block_store_zero_store_cbom_store_cbom_store_cbom_window(
            block0_vaddr, block1_vaddr, block2_vaddr, marker_vaddr,
            block0_older_store_values[round],
            block0_second_store_values[round],
            block0_third_store_values[round],
            block0_fourth_store_values[round], marker_updates[round],
            &block0_older_observed, &block0_second_observed,
            &block0_third_observed, &block0_fourth_observed,
            &block0_zero_observed, &block1_observed, &block2_observed,
            &marker_observed);
        AI_ASSERT_NO_EXCEPT_AND("under three-block CMO pressure, CBO.ZERO/CBOM/CBOM/CBOM keeps all post-zero store generations visible",
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
        AI_ASSERT_NO_EXCEPT_AND("after the four-generation zero-clean-clean-clean CMO probe, block0 keeps only post-first-zero stores, block1 preserves seed, block2 is zero, and guards remain intact",
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
        TEST_ASSERT("backing memory after the four-generation zero-clean-clean-clean CMO probe keeps block0 zero except the second, third, and fourth post-first-zero stores, block1 seed payload, block2 zero, marker update, and all guards intact",
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

bool ai_micro_hs_interleaved_three_block_store_inval_store_zero_store_zero_store_zero_four_generation_final_zero_corner()
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
    const uintptr_t block0_vaddr = hs_page_base(source_page) + 0x3540U;
    const uintptr_t block1_vaddr = hs_page_base(source_page) + 0x35c0U;
    const uintptr_t block2_vaddr = hs_page_base(source_page) + 0x3640U;
    const uintptr_t block0_paddr = phys_page_base(source_page) + 0x3540U;
    const uintptr_t block1_paddr = phys_page_base(source_page) + 0x35c0U;
    const uintptr_t block2_paddr = phys_page_base(source_page) + 0x3640U;
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
    const uintptr_t marker_vaddr = hs_page_base(marker_page) + 0x3540U;
    const uintptr_t marker_paddr = phys_page_base(marker_page) + 0x3540U;
    const uintptr_t marker_guard_vaddr = marker_vaddr + sizeof(uint64_t);
    const uintptr_t marker_guard_paddr = marker_paddr + sizeof(uint64_t);
    const uint64_t block0_seeds[AI_MEMBLOCK_CMO_TRIPLE_ROUNDS] = {
        0x3540500060007000ULL,
        0x4651611171118111ULL,
        0x5762722282229222ULL,
    };
    const uint64_t block1_seeds[AI_MEMBLOCK_CMO_TRIPLE_ROUNDS] = {
        0x80706050354035c0ULL,
        0x9181716146514691ULL,
        0xa2928272576257a2ULL,
    };
    const uint64_t block2_seeds[AI_MEMBLOCK_CMO_TRIPLE_ROUNDS] = {
        0x2468ace035403640ULL,
        0x3579bdf146514711ULL,
        0x468ace0257625822ULL,
    };
    const uint64_t block0_older_store_values[AI_MEMBLOCK_CMO_TRIPLE_ROUNDS] = {
        0x1100354022003300ULL,
        0x2211465133114411ULL,
        0x3322576244225522ULL,
    };
    const uint64_t block0_second_store_values[AI_MEMBLOCK_CMO_TRIPLE_ROUNDS] = {
        0x0033002200354011ULL,
        0x1144113311465122ULL,
        0x2255224422576233ULL,
    };
    const uint64_t block0_third_store_values[AI_MEMBLOCK_CMO_TRIPLE_ROUNDS] = {
        0xcafe3540feed3540ULL,
        0xdb0f46510ffe4651ULL,
        0xec205762200f5762ULL,
    };
    const uint64_t block0_fourth_store_values[AI_MEMBLOCK_CMO_TRIPLE_ROUNDS] = {
        0xc0013540deadbeefULL,
        0xd1124651efbedf00ULL,
        0xe2235762f0cfe011ULL,
    };
    const uint64_t before_guards[AI_MEMBLOCK_CMO_TRIPLE_ROUNDS] = {
        0xc0013540d00d3540ULL,
        0xd1124651e11e4651ULL,
        0xe2235762f22f5762ULL,
    };
    const uint64_t guard01_values[AI_MEMBLOCK_CMO_TRIPLE_ROUNDS] = {
        0x3540d00d3540c001ULL,
        0x4651e11e4651d112ULL,
        0x5762f22f5762e223ULL,
    };
    const uint64_t guard12_values[AI_MEMBLOCK_CMO_TRIPLE_ROUNDS] = {
        0x31415926354035c0ULL,
        0x2718281846514691ULL,
        0x16180339576257a2ULL,
    };
    const uint64_t after_guards[AI_MEMBLOCK_CMO_TRIPLE_ROUNDS] = {
        0x0835403640962695ULL,
        0x1946514711818282ULL,
        0x2a57625822930381ULL,
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
        0x55aa55aa35403540ULL,
        0x66bb66bb46514651ULL,
        0x77cc77cc57625762ULL,
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
        AI_ASSERT_NO_EXCEPT_AND("before the four-generation inval-zero-zero-zero CMO probe starts, all source blocks, guards, and marker are readable",
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
        ai_memblock_cmo_triple_three_block_store_inval_store_zero_store_zero_store_zero_window(
            block0_vaddr, block1_vaddr, block2_vaddr, marker_vaddr,
            block0_older_store_values[round],
            block0_second_store_values[round],
            block0_third_store_values[round],
            block0_fourth_store_values[round], marker_updates[round],
            &block0_older_observed, &block0_second_observed,
            &block0_third_observed, &block0_fourth_observed,
            &block0_zero_observed, &block1_observed, &block2_observed,
            &marker_observed);
        AI_ASSERT_NO_EXCEPT_AND("under three-block CMO pressure, CBO.INVAL/CBO.ZERO/CBO.ZERO/CBO.ZERO leaves block0 fully zeroed",
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
        AI_ASSERT_NO_EXCEPT_AND("after the four-generation inval-zero-zero-zero CMO probe, block0 is fully zero, block1 is zero, block2 preserves seed, and guards remain intact",
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
        TEST_ASSERT("backing memory after the four-generation inval-zero-zero-zero CMO probe keeps block0 zero, block1 zero, block2 seed payload, marker update, and all guards intact",
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

bool ai_micro_hs_interleaved_three_block_store_inval_store_zero_store_zero_store_inval_four_generation_post_second_zero_corner()
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
    const uintptr_t block0_vaddr = hs_page_base(source_page) + 0x3440U;
    const uintptr_t block1_vaddr = hs_page_base(source_page) + 0x34c0U;
    const uintptr_t block2_vaddr = hs_page_base(source_page) + 0x3540U;
    const uintptr_t block0_paddr = phys_page_base(source_page) + 0x3440U;
    const uintptr_t block1_paddr = phys_page_base(source_page) + 0x34c0U;
    const uintptr_t block2_paddr = phys_page_base(source_page) + 0x3540U;
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
    const uintptr_t marker_vaddr = hs_page_base(marker_page) + 0x3440U;
    const uintptr_t marker_paddr = phys_page_base(marker_page) + 0x3440U;
    const uintptr_t marker_guard_vaddr = marker_vaddr + sizeof(uint64_t);
    const uintptr_t marker_guard_paddr = marker_paddr + sizeof(uint64_t);
    const uint64_t block0_seeds[AI_MEMBLOCK_CMO_TRIPLE_ROUNDS] = {
        0x3440500060007000ULL,
        0x4551611171118111ULL,
        0x5662722282229222ULL,
    };
    const uint64_t block1_seeds[AI_MEMBLOCK_CMO_TRIPLE_ROUNDS] = {
        0x80706050344034c0ULL,
        0x9181716145514591ULL,
        0xa2928272566256a2ULL,
    };
    const uint64_t block2_seeds[AI_MEMBLOCK_CMO_TRIPLE_ROUNDS] = {
        0x2468ace034403540ULL,
        0x3579bdf145514611ULL,
        0x468ace0256625722ULL,
    };
    const uint64_t block0_older_store_values[AI_MEMBLOCK_CMO_TRIPLE_ROUNDS] = {
        0x1100344022003300ULL,
        0x2211455133114411ULL,
        0x3322566244225522ULL,
    };
    const uint64_t block0_second_store_values[AI_MEMBLOCK_CMO_TRIPLE_ROUNDS] = {
        0x0033002200344011ULL,
        0x1144113311455122ULL,
        0x2255224422566233ULL,
    };
    const uint64_t block0_third_store_values[AI_MEMBLOCK_CMO_TRIPLE_ROUNDS] = {
        0xcafe3440feed3440ULL,
        0xdb0f45510ffe4551ULL,
        0xec205662200f5662ULL,
    };
    const uint64_t block0_fourth_store_values[AI_MEMBLOCK_CMO_TRIPLE_ROUNDS] = {
        0xc0013440deadbeefULL,
        0xd1124551efbedf00ULL,
        0xe2235662f0cfe011ULL,
    };
    const uint64_t before_guards[AI_MEMBLOCK_CMO_TRIPLE_ROUNDS] = {
        0xc0013440d00d3440ULL,
        0xd1124551e11e4551ULL,
        0xe2235662f22f5662ULL,
    };
    const uint64_t guard01_values[AI_MEMBLOCK_CMO_TRIPLE_ROUNDS] = {
        0x3440d00d3440c001ULL,
        0x4551e11e4551d112ULL,
        0x5662f22f5662e223ULL,
    };
    const uint64_t guard12_values[AI_MEMBLOCK_CMO_TRIPLE_ROUNDS] = {
        0x31415926344034c0ULL,
        0x2718281845514591ULL,
        0x16180339566256a2ULL,
    };
    const uint64_t after_guards[AI_MEMBLOCK_CMO_TRIPLE_ROUNDS] = {
        0x0834403540962695ULL,
        0x1945514611818282ULL,
        0x2a56625722930381ULL,
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
        0x55aa55aa34403440ULL,
        0x66bb66bb45514551ULL,
        0x77cc77cc56625662ULL,
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
        AI_ASSERT_NO_EXCEPT_AND("before the four-generation inval-zero-zero-inval CMO probe starts, all source blocks, guards, and marker are readable",
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
        ai_memblock_cmo_triple_three_block_store_inval_store_zero_store_zero_store_inval_window(
            block0_vaddr, block1_vaddr, block2_vaddr, marker_vaddr,
            block0_older_store_values[round],
            block0_second_store_values[round],
            block0_third_store_values[round],
            block0_fourth_store_values[round], marker_updates[round],
            &block0_older_observed, &block0_second_observed,
            &block0_third_observed, &block0_fourth_observed,
            &block0_zero_observed, &block1_observed, &block2_observed,
            &marker_observed);
        AI_ASSERT_NO_EXCEPT_AND("under three-block CMO pressure, CBO.INVAL/CBO.ZERO/CBO.ZERO/CBO.INVAL keeps only the fourth post-second-zero store visible",
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
        AI_ASSERT_NO_EXCEPT_AND("after the four-generation inval-zero-zero-inval CMO probe, block0 keeps only the fourth store, block1 is zero, block2 preserves seed, and guards remain intact",
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
        TEST_ASSERT("backing memory after the four-generation inval-zero-zero-inval CMO probe keeps block0 zero except the fourth post-second-zero store, block1 zero, block2 seed payload, marker update, and all guards intact",
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

bool ai_micro_hs_interleaved_three_block_store_inval_store_zero_store_zero_store_cbom_four_generation_post_second_zero_corner()
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
    const uintptr_t block0_vaddr = hs_page_base(source_page) + 0x3340U;
    const uintptr_t block1_vaddr = hs_page_base(source_page) + 0x33c0U;
    const uintptr_t block2_vaddr = hs_page_base(source_page) + 0x3440U;
    const uintptr_t block0_paddr = phys_page_base(source_page) + 0x3340U;
    const uintptr_t block1_paddr = phys_page_base(source_page) + 0x33c0U;
    const uintptr_t block2_paddr = phys_page_base(source_page) + 0x3440U;
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
    const uintptr_t marker_vaddr = hs_page_base(marker_page) + 0x3340U;
    const uintptr_t marker_paddr = phys_page_base(marker_page) + 0x3340U;
    const uintptr_t marker_guard_vaddr = marker_vaddr + sizeof(uint64_t);
    const uintptr_t marker_guard_paddr = marker_paddr + sizeof(uint64_t);
    const uint64_t block0_seeds[AI_MEMBLOCK_CMO_TRIPLE_ROUNDS] = {
        0x3340500060007000ULL,
        0x4451611171118111ULL,
        0x5562722282229222ULL,
    };
    const uint64_t block1_seeds[AI_MEMBLOCK_CMO_TRIPLE_ROUNDS] = {
        0x80706050334033c0ULL,
        0x9181716144514491ULL,
        0xa2928272556255a2ULL,
    };
    const uint64_t block2_seeds[AI_MEMBLOCK_CMO_TRIPLE_ROUNDS] = {
        0x2468ace033403440ULL,
        0x3579bdf144514511ULL,
        0x468ace0255625622ULL,
    };
    const uint64_t block0_older_store_values[AI_MEMBLOCK_CMO_TRIPLE_ROUNDS] = {
        0x1100334022003300ULL,
        0x2211445133114411ULL,
        0x3322556244225522ULL,
    };
    const uint64_t block0_second_store_values[AI_MEMBLOCK_CMO_TRIPLE_ROUNDS] = {
        0x0033002200334011ULL,
        0x1144113311445122ULL,
        0x2255224422556233ULL,
    };
    const uint64_t block0_third_store_values[AI_MEMBLOCK_CMO_TRIPLE_ROUNDS] = {
        0xcafe3340feed3340ULL,
        0xdb0f44510ffe4451ULL,
        0xec205562200f5562ULL,
    };
    const uint64_t block0_fourth_store_values[AI_MEMBLOCK_CMO_TRIPLE_ROUNDS] = {
        0xc0013340deadbeefULL,
        0xd1124451efbedf00ULL,
        0xe2235562f0cfe011ULL,
    };
    const uint64_t before_guards[AI_MEMBLOCK_CMO_TRIPLE_ROUNDS] = {
        0xc0013340d00d3340ULL,
        0xd1124451e11e4451ULL,
        0xe2235562f22f5562ULL,
    };
    const uint64_t guard01_values[AI_MEMBLOCK_CMO_TRIPLE_ROUNDS] = {
        0x3340d00d3340c001ULL,
        0x4451e11e4451d112ULL,
        0x5562f22f5562e223ULL,
    };
    const uint64_t guard12_values[AI_MEMBLOCK_CMO_TRIPLE_ROUNDS] = {
        0x31415926334033c0ULL,
        0x2718281844514491ULL,
        0x16180339556255a2ULL,
    };
    const uint64_t after_guards[AI_MEMBLOCK_CMO_TRIPLE_ROUNDS] = {
        0x0833403440962695ULL,
        0x1944514511818282ULL,
        0x2a55625622930381ULL,
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
        0x55aa55aa33403340ULL,
        0x66bb66bb44514451ULL,
        0x77cc77cc55625562ULL,
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
        AI_ASSERT_NO_EXCEPT_AND("before the four-generation inval-zero-zero-cbom CMO probe starts, all source blocks, guards, and marker are readable",
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
        ai_memblock_cmo_triple_three_block_store_inval_store_zero_store_zero_store_cbom_window(
            block0_vaddr, block1_vaddr, block2_vaddr, marker_vaddr,
            block0_older_store_values[round],
            block0_second_store_values[round],
            block0_third_store_values[round],
            block0_fourth_store_values[round], marker_updates[round],
            &block0_older_observed, &block0_second_observed,
            &block0_third_observed, &block0_fourth_observed,
            &block0_zero_observed, &block1_observed, &block2_observed,
            &marker_observed);
        AI_ASSERT_NO_EXCEPT_AND("under three-block CMO pressure, CBO.INVAL/CBO.ZERO/CBO.ZERO/CBOM keeps only the fourth post-second-zero store visible through final CBOM",
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
        AI_ASSERT_NO_EXCEPT_AND("after the four-generation inval-zero-zero-cbom CMO probe, block0 keeps only the fourth store, block1 is zero, block2 preserves seed, and guards remain intact",
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
        TEST_ASSERT("backing memory after the four-generation inval-zero-zero-cbom CMO probe keeps block0 zero except the fourth post-second-zero store, block1 zero, block2 seed payload, marker update, and all guards intact",
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

bool ai_micro_hs_interleaved_three_block_store_inval_store_zero_store_inval_store_inval_four_generation_mid_zero_corner()
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
    const uintptr_t block0_vaddr = hs_page_base(source_page) + 0x3240U;
    const uintptr_t block1_vaddr = hs_page_base(source_page) + 0x32c0U;
    const uintptr_t block2_vaddr = hs_page_base(source_page) + 0x3340U;
    const uintptr_t block0_paddr = phys_page_base(source_page) + 0x3240U;
    const uintptr_t block1_paddr = phys_page_base(source_page) + 0x32c0U;
    const uintptr_t block2_paddr = phys_page_base(source_page) + 0x3340U;
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
    const uintptr_t marker_vaddr = hs_page_base(marker_page) + 0x3240U;
    const uintptr_t marker_paddr = phys_page_base(marker_page) + 0x3240U;
    const uintptr_t marker_guard_vaddr = marker_vaddr + sizeof(uint64_t);
    const uintptr_t marker_guard_paddr = marker_paddr + sizeof(uint64_t);
    const uint64_t block0_seeds[AI_MEMBLOCK_CMO_TRIPLE_ROUNDS] = {
        0x3240500060007000ULL,
        0x4351611171118111ULL,
        0x5462722282229222ULL,
    };
    const uint64_t block1_seeds[AI_MEMBLOCK_CMO_TRIPLE_ROUNDS] = {
        0x80706050324032c0ULL,
        0x9181716143514391ULL,
        0xa2928272546254a2ULL,
    };
    const uint64_t block2_seeds[AI_MEMBLOCK_CMO_TRIPLE_ROUNDS] = {
        0x2468ace032403340ULL,
        0x3579bdf143514411ULL,
        0x468ace0254625522ULL,
    };
    const uint64_t block0_older_store_values[AI_MEMBLOCK_CMO_TRIPLE_ROUNDS] = {
        0x1100324022003300ULL,
        0x2211435133114411ULL,
        0x3322546244225522ULL,
    };
    const uint64_t block0_second_store_values[AI_MEMBLOCK_CMO_TRIPLE_ROUNDS] = {
        0x0033002200324011ULL,
        0x1144113311435122ULL,
        0x2255224422546233ULL,
    };
    const uint64_t block0_third_store_values[AI_MEMBLOCK_CMO_TRIPLE_ROUNDS] = {
        0xcafe3240feed3240ULL,
        0xdb0f43510ffe4351ULL,
        0xec205462200f5462ULL,
    };
    const uint64_t block0_fourth_store_values[AI_MEMBLOCK_CMO_TRIPLE_ROUNDS] = {
        0xc0013240deadbeefULL,
        0xd1124351efbedf00ULL,
        0xe2235462f0cfe011ULL,
    };
    const uint64_t before_guards[AI_MEMBLOCK_CMO_TRIPLE_ROUNDS] = {
        0xc0013240d00d3240ULL,
        0xd1124351e11e4351ULL,
        0xe2235462f22f5462ULL,
    };
    const uint64_t guard01_values[AI_MEMBLOCK_CMO_TRIPLE_ROUNDS] = {
        0x3240d00d3240c001ULL,
        0x4351e11e4351d112ULL,
        0x5462f22f5462e223ULL,
    };
    const uint64_t guard12_values[AI_MEMBLOCK_CMO_TRIPLE_ROUNDS] = {
        0x31415926324032c0ULL,
        0x2718281843514391ULL,
        0x16180339546254a2ULL,
    };
    const uint64_t after_guards[AI_MEMBLOCK_CMO_TRIPLE_ROUNDS] = {
        0x0832403340962695ULL,
        0x1943514411818282ULL,
        0x2a54625522930381ULL,
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
        0x55aa55aa32403240ULL,
        0x66bb66bb43514351ULL,
        0x77cc77cc54625462ULL,
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
        AI_ASSERT_NO_EXCEPT_AND("before the four-generation inval-zero-inval-inval CMO probe starts, all source blocks, guards, and marker are readable",
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
        ai_memblock_cmo_triple_three_block_store_inval_store_zero_store_inval_store_inval_window(
            block0_vaddr, block1_vaddr, block2_vaddr, marker_vaddr,
            block0_older_store_values[round],
            block0_second_store_values[round],
            block0_third_store_values[round],
            block0_fourth_store_values[round], marker_updates[round],
            &block0_older_observed, &block0_second_observed,
            &block0_third_observed, &block0_fourth_observed,
            &block0_zero_observed, &block1_observed, &block2_observed,
            &marker_observed);
        AI_ASSERT_NO_EXCEPT_AND("under three-block CMO pressure, middle CBO.ZERO clears pre-zero generations while third and fourth stores survive two later CBO.INVALs",
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
        AI_ASSERT_NO_EXCEPT_AND("after the four-generation inval-zero-inval-inval CMO probe, block0 keeps only post-zero stores, block1 is zero, block2 preserves seed, and guards remain intact",
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
        TEST_ASSERT("backing memory after the four-generation inval-zero-inval-inval CMO probe keeps block0 zero except the two post-zero stores, block1 zero, block2 seed payload, marker update, and all guards intact",
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

bool ai_micro_hs_interleaved_three_block_store_inval_store_zero_store_cbom_store_cbom_four_generation_mid_zero_corner()
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
    const uintptr_t block0_vaddr = hs_page_base(source_page) + 0x3140U;
    const uintptr_t block1_vaddr = hs_page_base(source_page) + 0x31c0U;
    const uintptr_t block2_vaddr = hs_page_base(source_page) + 0x3240U;
    const uintptr_t block0_paddr = phys_page_base(source_page) + 0x3140U;
    const uintptr_t block1_paddr = phys_page_base(source_page) + 0x31c0U;
    const uintptr_t block2_paddr = phys_page_base(source_page) + 0x3240U;
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
    const uintptr_t marker_vaddr = hs_page_base(marker_page) + 0x3140U;
    const uintptr_t marker_paddr = phys_page_base(marker_page) + 0x3140U;
    const uintptr_t marker_guard_vaddr = marker_vaddr + sizeof(uint64_t);
    const uintptr_t marker_guard_paddr = marker_paddr + sizeof(uint64_t);
    const uint64_t block0_seeds[AI_MEMBLOCK_CMO_TRIPLE_ROUNDS] = {
        0x3140500060007000ULL,
        0x4251611171118111ULL,
        0x5362722282229222ULL,
    };
    const uint64_t block1_seeds[AI_MEMBLOCK_CMO_TRIPLE_ROUNDS] = {
        0x80706050314031c0ULL,
        0x9181716142514291ULL,
        0xa2928272536253a2ULL,
    };
    const uint64_t block2_seeds[AI_MEMBLOCK_CMO_TRIPLE_ROUNDS] = {
        0x2468ace031403240ULL,
        0x3579bdf142514311ULL,
        0x468ace0253625422ULL,
    };
    const uint64_t block0_older_store_values[AI_MEMBLOCK_CMO_TRIPLE_ROUNDS] = {
        0x1100314022003300ULL,
        0x2211425133114411ULL,
        0x3322536244225522ULL,
    };
    const uint64_t block0_second_store_values[AI_MEMBLOCK_CMO_TRIPLE_ROUNDS] = {
        0x0033002200314011ULL,
        0x1144113311425122ULL,
        0x2255224422536233ULL,
    };
    const uint64_t block0_third_store_values[AI_MEMBLOCK_CMO_TRIPLE_ROUNDS] = {
        0xcafe3140feed3140ULL,
        0xdb0f42510ffe4251ULL,
        0xec205362200f5362ULL,
    };
    const uint64_t block0_fourth_store_values[AI_MEMBLOCK_CMO_TRIPLE_ROUNDS] = {
        0xc0013140deadbeefULL,
        0xd1124251efbedf00ULL,
        0xe2235362f0cfe011ULL,
    };
    const uint64_t before_guards[AI_MEMBLOCK_CMO_TRIPLE_ROUNDS] = {
        0xc0013140d00d3140ULL,
        0xd1124251e11e4251ULL,
        0xe2235362f22f5362ULL,
    };
    const uint64_t guard01_values[AI_MEMBLOCK_CMO_TRIPLE_ROUNDS] = {
        0x3140d00d3140c001ULL,
        0x4251e11e4251d112ULL,
        0x5362f22f5362e223ULL,
    };
    const uint64_t guard12_values[AI_MEMBLOCK_CMO_TRIPLE_ROUNDS] = {
        0x31415926314031c0ULL,
        0x2718281842514291ULL,
        0x16180339536253a2ULL,
    };
    const uint64_t after_guards[AI_MEMBLOCK_CMO_TRIPLE_ROUNDS] = {
        0x0831403240962695ULL,
        0x1942514311818282ULL,
        0x2a53625422930381ULL,
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
        0x55aa55aa31403140ULL,
        0x66bb66bb42514251ULL,
        0x77cc77cc53625362ULL,
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
        AI_ASSERT_NO_EXCEPT_AND("before the four-generation inval-zero-clean-clean CMO probe starts, all source blocks, guards, and marker are readable",
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
        ai_memblock_cmo_triple_three_block_store_inval_store_zero_store_cbom_store_cbom_window(
            block0_vaddr, block1_vaddr, block2_vaddr, marker_vaddr,
            block0_older_store_values[round],
            block0_second_store_values[round],
            block0_third_store_values[round],
            block0_fourth_store_values[round], marker_updates[round],
            &block0_older_observed, &block0_second_observed,
            &block0_third_observed, &block0_fourth_observed,
            &block0_zero_observed, &block1_observed, &block2_observed,
            &marker_observed);
        AI_ASSERT_NO_EXCEPT_AND("under three-block CMO pressure, middle CBO.ZERO clears pre-zero generations while third and fourth stores survive two later CBOMs",
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
        AI_ASSERT_NO_EXCEPT_AND("after the four-generation inval-zero-clean-clean CMO probe, block0 keeps only post-zero stores, block1 is zero, block2 preserves seed, and guards remain intact",
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
        TEST_ASSERT("backing memory after the four-generation inval-zero-clean-clean CMO probe keeps block0 zero except the two post-zero stores, block1 zero, block2 seed payload, marker update, and all guards intact",
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

bool ai_micro_hs_interleaved_three_block_store_inval_store_inval_store_cbom_store_inval_four_generation_no_zero_corner()
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
    const uintptr_t block0_vaddr = hs_page_base(source_page) + 0x3340U;
    const uintptr_t block1_vaddr = hs_page_base(source_page) + 0x33c0U;
    const uintptr_t block2_vaddr = hs_page_base(source_page) + 0x3440U;
    const uintptr_t block0_paddr = phys_page_base(source_page) + 0x3340U;
    const uintptr_t block1_paddr = phys_page_base(source_page) + 0x33c0U;
    const uintptr_t block2_paddr = phys_page_base(source_page) + 0x3440U;
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
    const uintptr_t marker_vaddr = hs_page_base(marker_page) + 0x3340U;
    const uintptr_t marker_paddr = phys_page_base(marker_page) + 0x3340U;
    const uintptr_t marker_guard_vaddr = marker_vaddr + sizeof(uint64_t);
    const uintptr_t marker_guard_paddr = marker_paddr + sizeof(uint64_t);
    const uint64_t block0_seeds[AI_MEMBLOCK_CMO_TRIPLE_ROUNDS] = {
        0x3340434053406340ULL,
        0x4451545164517451ULL,
        0x5562656275628562ULL,
    };
    const uint64_t block1_seeds[AI_MEMBLOCK_CMO_TRIPLE_ROUNDS] = {
        0x80706050334033c0ULL,
        0x91817161445144d1ULL,
        0xa2928272556255e2ULL,
    };
    const uint64_t block2_seeds[AI_MEMBLOCK_CMO_TRIPLE_ROUNDS] = {
        0x2468ace033403440ULL,
        0x3579bdf144514551ULL,
        0x468ace0255625662ULL,
    };
    const uint64_t block0_older_store_values[AI_MEMBLOCK_CMO_TRIPLE_ROUNDS] = {
        0x1100334022003300ULL,
        0x2211445133114411ULL,
        0x3322556244225522ULL,
    };
    const uint64_t block0_second_store_values[AI_MEMBLOCK_CMO_TRIPLE_ROUNDS] = {
        0x0033002200334011ULL,
        0x1144113311445122ULL,
        0x2255224422556233ULL,
    };
    const uint64_t block0_third_store_values[AI_MEMBLOCK_CMO_TRIPLE_ROUNDS] = {
        0xcafe3340feed3340ULL,
        0xdb0f44510ffe4451ULL,
        0xec205562200f5562ULL,
    };
    const uint64_t block0_fourth_store_values[AI_MEMBLOCK_CMO_TRIPLE_ROUNDS] = {
        0xc0013340deadbeefULL,
        0xd1124451efbedf00ULL,
        0xe2235562f0cfe011ULL,
    };
    const uint64_t before_guards[AI_MEMBLOCK_CMO_TRIPLE_ROUNDS] = {
        0xc0013340d00d3340ULL,
        0xd1124451e11e4451ULL,
        0xe2235562f22f5562ULL,
    };
    const uint64_t guard01_values[AI_MEMBLOCK_CMO_TRIPLE_ROUNDS] = {
        0x3340d00d3340c001ULL,
        0x4451e11e4451d112ULL,
        0x5562f22f5562e223ULL,
    };
    const uint64_t guard12_values[AI_MEMBLOCK_CMO_TRIPLE_ROUNDS] = {
        0x31415926334033c0ULL,
        0x27182818445144d1ULL,
        0x16180339556255e2ULL,
    };
    const uint64_t after_guards[AI_MEMBLOCK_CMO_TRIPLE_ROUNDS] = {
        0x0833403440962695ULL,
        0x1944514511818282ULL,
        0x2a55625622930381ULL,
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
        0x55aa55aa33403340ULL,
        0x66bb66bb44514451ULL,
        0x77cc77cc55625562ULL,
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
        AI_ASSERT_NO_EXCEPT_AND("before the four-generation inval-inval-clean-inval CMO probe starts, all source blocks, guards, and marker are readable",
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
        ai_memblock_cmo_triple_three_block_store_inval_store_inval_store_cbom_store_inval_window(
            block0_vaddr, block1_vaddr, block2_vaddr, marker_vaddr,
            block0_older_store_values[round],
            block0_second_store_values[round],
            block0_third_store_values[round],
            block0_fourth_store_values[round], marker_updates[round],
            &block0_older_observed, &block0_second_observed,
            &block0_third_observed, &block0_fourth_observed,
            &block0_seed_observed, &block1_observed, &block2_observed,
            &marker_observed);
        AI_ASSERT_NO_EXCEPT_AND("under three-block CMO pressure, double CBO.INVAL then CBOM and final CBO.INVAL keep all four same-block store generations visible",
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
        AI_ASSERT_NO_EXCEPT_AND("after the four-generation inval-inval-clean-inval CMO probe, block0 preserves all stores, block1 is zero, block2 preserves seed, and guards remain intact",
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
        TEST_ASSERT("backing memory after the four-generation inval-inval-clean-inval CMO probe keeps block0 seed except all four store generations, block1 zero, block2 seed payload, marker update, and all guards intact",
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

bool ai_micro_hs_interleaved_three_block_store_inval_store_cbom_store_inval_store_inval_four_generation_no_zero_corner()
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
    const uintptr_t block0_vaddr = hs_page_base(source_page) + 0x3540U;
    const uintptr_t block1_vaddr = hs_page_base(source_page) + 0x35c0U;
    const uintptr_t block2_vaddr = hs_page_base(source_page) + 0x3640U;
    const uintptr_t block0_paddr = phys_page_base(source_page) + 0x3540U;
    const uintptr_t block1_paddr = phys_page_base(source_page) + 0x35c0U;
    const uintptr_t block2_paddr = phys_page_base(source_page) + 0x3640U;
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
    const uintptr_t marker_vaddr = hs_page_base(marker_page) + 0x3540U;
    const uintptr_t marker_paddr = phys_page_base(marker_page) + 0x3540U;
    const uintptr_t marker_guard_vaddr = marker_vaddr + sizeof(uint64_t);
    const uintptr_t marker_guard_paddr = marker_paddr + sizeof(uint64_t);
    const uint64_t block0_seeds[AI_MEMBLOCK_CMO_TRIPLE_ROUNDS] = {
        0x3540454055406540ULL,
        0x4651565166517651ULL,
        0x5762676277628762ULL,
    };
    const uint64_t block1_seeds[AI_MEMBLOCK_CMO_TRIPLE_ROUNDS] = {
        0x80706050354035c0ULL,
        0x91817161465146d1ULL,
        0xa2928272576257e2ULL,
    };
    const uint64_t block2_seeds[AI_MEMBLOCK_CMO_TRIPLE_ROUNDS] = {
        0x2468ace035403640ULL,
        0x3579bdf146514751ULL,
        0x468ace0257625862ULL,
    };
    const uint64_t block0_older_store_values[AI_MEMBLOCK_CMO_TRIPLE_ROUNDS] = {
        0x1100354022003300ULL,
        0x2211465133114411ULL,
        0x3322576244225522ULL,
    };
    const uint64_t block0_second_store_values[AI_MEMBLOCK_CMO_TRIPLE_ROUNDS] = {
        0x0033002200354011ULL,
        0x1144113311465122ULL,
        0x2255224422576233ULL,
    };
    const uint64_t block0_third_store_values[AI_MEMBLOCK_CMO_TRIPLE_ROUNDS] = {
        0xcafe3540feed3540ULL,
        0xdb0f46510ffe4651ULL,
        0xec205762200f5762ULL,
    };
    const uint64_t block0_fourth_store_values[AI_MEMBLOCK_CMO_TRIPLE_ROUNDS] = {
        0xc0013540deadbeefULL,
        0xd1124651efbedf00ULL,
        0xe2235762f0cfe011ULL,
    };
    const uint64_t before_guards[AI_MEMBLOCK_CMO_TRIPLE_ROUNDS] = {
        0xc0013540d00d3540ULL,
        0xd1124651e11e4651ULL,
        0xe2235762f22f5762ULL,
    };
    const uint64_t guard01_values[AI_MEMBLOCK_CMO_TRIPLE_ROUNDS] = {
        0x3540d00d3540c001ULL,
        0x4651e11e4651d112ULL,
        0x5762f22f5762e223ULL,
    };
    const uint64_t guard12_values[AI_MEMBLOCK_CMO_TRIPLE_ROUNDS] = {
        0x31415926354035c0ULL,
        0x27182818465146d1ULL,
        0x16180339576257e2ULL,
    };
    const uint64_t after_guards[AI_MEMBLOCK_CMO_TRIPLE_ROUNDS] = {
        0x0835403640962695ULL,
        0x1946514711818282ULL,
        0x2a57625822930381ULL,
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
        0x55aa55aa35403540ULL,
        0x66bb66bb46514651ULL,
        0x77cc77cc57625762ULL,
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
        AI_ASSERT_NO_EXCEPT_AND("before the four-generation inval-clean-inval-inval CMO probe starts, all source blocks, guards, and marker are readable",
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
        ai_memblock_cmo_triple_three_block_store_inval_store_cbom_store_inval_store_inval_window(
            block0_vaddr, block1_vaddr, block2_vaddr, marker_vaddr,
            block0_older_store_values[round],
            block0_second_store_values[round],
            block0_third_store_values[round],
            block0_fourth_store_values[round], marker_updates[round],
            &block0_older_observed, &block0_second_observed,
            &block0_third_observed, &block0_fourth_observed,
            &block0_seed_observed, &block1_observed, &block2_observed,
            &marker_observed);
        AI_ASSERT_NO_EXCEPT_AND("under three-block CMO pressure, CBO.INVAL, CBOM, and double CBO.INVAL keep all four same-block store generations visible",
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
        AI_ASSERT_NO_EXCEPT_AND("after the four-generation inval-clean-inval-inval CMO probe, block0 preserves all stores, block1 is zero, block2 preserves seed, and guards remain intact",
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
        TEST_ASSERT("backing memory after the four-generation inval-clean-inval-inval CMO probe keeps block0 seed except all four store generations, block1 zero, block2 seed payload, marker update, and all guards intact",
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

bool ai_micro_hs_interleaved_three_block_store_inval_store_cbom_store_zero_store_zero_four_generation_final_zero_corner()
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
    const uintptr_t block0_vaddr = hs_page_base(source_page) + 0x3740U;
    const uintptr_t block1_vaddr = hs_page_base(source_page) + 0x37c0U;
    const uintptr_t block2_vaddr = hs_page_base(source_page) + 0x3840U;
    const uintptr_t block0_paddr = phys_page_base(source_page) + 0x3740U;
    const uintptr_t block1_paddr = phys_page_base(source_page) + 0x37c0U;
    const uintptr_t block2_paddr = phys_page_base(source_page) + 0x3840U;
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
    const uintptr_t marker_vaddr = hs_page_base(marker_page) + 0x3740U;
    const uintptr_t marker_paddr = phys_page_base(marker_page) + 0x3740U;
    const uintptr_t marker_guard_vaddr = marker_vaddr + sizeof(uint64_t);
    const uintptr_t marker_guard_paddr = marker_paddr + sizeof(uint64_t);
    const uint64_t block0_seeds[AI_MEMBLOCK_CMO_TRIPLE_ROUNDS] = {
        0x3740474057406740ULL,
        0x4851585168517851ULL,
        0x5962696279628962ULL,
    };
    const uint64_t block1_seeds[AI_MEMBLOCK_CMO_TRIPLE_ROUNDS] = {
        0x80706050374037c0ULL,
        0x91817161485148d1ULL,
        0xa2928272596259e2ULL,
    };
    const uint64_t block2_seeds[AI_MEMBLOCK_CMO_TRIPLE_ROUNDS] = {
        0x2468ace037403840ULL,
        0x3579bdf148514951ULL,
        0x468ace0259625a62ULL,
    };
    const uint64_t block0_older_store_values[AI_MEMBLOCK_CMO_TRIPLE_ROUNDS] = {
        0x1100374022003300ULL,
        0x2211485133114411ULL,
        0x3322596244225522ULL,
    };
    const uint64_t block0_second_store_values[AI_MEMBLOCK_CMO_TRIPLE_ROUNDS] = {
        0x0033002200374011ULL,
        0x1144113311485122ULL,
        0x2255224422596233ULL,
    };
    const uint64_t block0_third_store_values[AI_MEMBLOCK_CMO_TRIPLE_ROUNDS] = {
        0xcafe3740feed3740ULL,
        0xdb0f48510ffe4851ULL,
        0xec205962200f5962ULL,
    };
    const uint64_t block0_fourth_store_values[AI_MEMBLOCK_CMO_TRIPLE_ROUNDS] = {
        0xc0013740deadbeefULL,
        0xd1124851efbedf00ULL,
        0xe2235962f0cfe011ULL,
    };
    const uint64_t before_guards[AI_MEMBLOCK_CMO_TRIPLE_ROUNDS] = {
        0xc0013740d00d3740ULL,
        0xd1124851e11e4851ULL,
        0xe2235962f22f5962ULL,
    };
    const uint64_t guard01_values[AI_MEMBLOCK_CMO_TRIPLE_ROUNDS] = {
        0x3740d00d3740c001ULL,
        0x4851e11e4851d112ULL,
        0x5962f22f5962e223ULL,
    };
    const uint64_t guard12_values[AI_MEMBLOCK_CMO_TRIPLE_ROUNDS] = {
        0x31415926374037c0ULL,
        0x27182818485148d1ULL,
        0x16180339596259e2ULL,
    };
    const uint64_t after_guards[AI_MEMBLOCK_CMO_TRIPLE_ROUNDS] = {
        0x0837403840962695ULL,
        0x1948514911818282ULL,
        0x2a59625a22930381ULL,
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
        0x55aa55aa37403740ULL,
        0x66bb66bb48514851ULL,
        0x77cc77cc59625962ULL,
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
        AI_ASSERT_NO_EXCEPT_AND("before the four-generation inval-clean-zero-zero CMO probe starts, all source blocks, guards, and marker are readable",
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
        ai_memblock_cmo_triple_three_block_store_inval_store_cbom_store_zero_store_zero_window(
            block0_vaddr, block1_vaddr, block2_vaddr, marker_vaddr,
            block0_older_store_values[round],
            block0_second_store_values[round],
            block0_third_store_values[round],
            block0_fourth_store_values[round], marker_updates[round],
            &block0_older_observed, &block0_second_observed,
            &block0_third_observed, &block0_fourth_observed,
            &block0_zero_observed, &block1_observed, &block2_observed,
            &marker_observed);
        AI_ASSERT_NO_EXCEPT_AND("under three-block CMO pressure, final double CBO.ZERO clears all same-block store generations after CBO.INVAL and CBOM activity",
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
        AI_ASSERT_NO_EXCEPT_AND("after the four-generation inval-clean-zero-zero CMO probe, block0 and block1 are zero, block2 preserves seed, and guards remain intact",
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
        TEST_ASSERT("backing memory after the four-generation inval-clean-zero-zero CMO probe keeps block0 and block1 fully zero, block2 seed payload, marker update, and all guards intact",
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

bool ai_micro_hs_interleaved_three_block_store_inval_store_cbom_store_cbom_store_inval_four_generation_no_zero_corner()
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
    const uintptr_t marker_vaddr = hs_page_base(marker_page) + 0x3940U;
    const uintptr_t marker_paddr = phys_page_base(marker_page) + 0x3940U;
    const uintptr_t marker_guard_vaddr = marker_vaddr + sizeof(uint64_t);
    const uintptr_t marker_guard_paddr = marker_paddr + sizeof(uint64_t);
    const uint64_t block0_seeds[AI_MEMBLOCK_CMO_TRIPLE_ROUNDS] = {
        0x3940494059406940ULL,
        0x4a515a516a517a51ULL,
        0x5b626b627b628b62ULL,
    };
    const uint64_t block1_seeds[AI_MEMBLOCK_CMO_TRIPLE_ROUNDS] = {
        0x80706050394039c0ULL,
        0x918171614a514ad1ULL,
        0xa29282725b625be2ULL,
    };
    const uint64_t block2_seeds[AI_MEMBLOCK_CMO_TRIPLE_ROUNDS] = {
        0x2468ace039403a40ULL,
        0x3579bdf14a514b51ULL,
        0x468ace025b625c62ULL,
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
        0x271828184a514ad1ULL,
        0x161803395b625be2ULL,
    };
    const uint64_t after_guards[AI_MEMBLOCK_CMO_TRIPLE_ROUNDS] = {
        0x0839403a40962695ULL,
        0x194a514b511818282ULL,
        0x2a5b625c622930381ULL,
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
        AI_ASSERT_NO_EXCEPT_AND("before the four-generation inval-clean-clean-inval CMO probe starts, all source blocks, guards, and marker are readable",
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
        ai_memblock_cmo_triple_three_block_store_inval_store_cbom_store_cbom_store_inval_window(
            block0_vaddr, block1_vaddr, block2_vaddr, marker_vaddr,
            block0_older_store_values[round],
            block0_second_store_values[round],
            block0_third_store_values[round],
            block0_fourth_store_values[round], marker_updates[round],
            &block0_older_observed, &block0_second_observed,
            &block0_third_observed, &block0_fourth_observed,
            &block0_seed_observed, &block1_observed, &block2_observed,
            &marker_observed);
        AI_ASSERT_NO_EXCEPT_AND("under three-block CMO pressure, CBO.INVAL followed by double CBOM and final CBO.INVAL keeps all four same-block store generations visible",
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
        AI_ASSERT_NO_EXCEPT_AND("after the four-generation inval-clean-clean-inval CMO probe, block0 preserves all stores, block1 is zero, block2 preserves seed, and guards remain intact",
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
        TEST_ASSERT("backing memory after the four-generation inval-clean-clean-inval CMO probe keeps block0 seed except all four store generations, block1 zero, block2 seed payload, marker update, and all guards intact",
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

bool ai_micro_hs_interleaved_three_block_store_inval_store_cbom_store_cbom_store_zero_four_generation_final_zero_corner()
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
        0x3b404b405b406b40ULL,
        0x4c515c516c517c51ULL,
        0x5d626d627d628d62ULL,
    };
    const uint64_t block1_seeds[AI_MEMBLOCK_CMO_TRIPLE_ROUNDS] = {
        0x807060503b403bc0ULL,
        0x918171614c514cd1ULL,
        0xa29282725d625de2ULL,
    };
    const uint64_t block2_seeds[AI_MEMBLOCK_CMO_TRIPLE_ROUNDS] = {
        0x2468ace03b403c40ULL,
        0x3579bdf14c514d51ULL,
        0x468ace025d625e62ULL,
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
        0x271828184c514cd1ULL,
        0x161803395d625de2ULL,
    };
    const uint64_t after_guards[AI_MEMBLOCK_CMO_TRIPLE_ROUNDS] = {
        0x083b403c40962695ULL,
        0x194c514d511818282ULL,
        0x2a5d625e622930381ULL,
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
        AI_ASSERT_NO_EXCEPT_AND("before the four-generation inval-clean-clean-zero CMO probe starts, all source blocks, guards, and marker are readable",
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
        ai_memblock_cmo_triple_three_block_store_inval_store_cbom_store_cbom_store_zero_window(
            block0_vaddr, block1_vaddr, block2_vaddr, marker_vaddr,
            block0_older_store_values[round],
            block0_second_store_values[round],
            block0_third_store_values[round],
            block0_fourth_store_values[round], marker_updates[round],
            &block0_older_observed, &block0_second_observed,
            &block0_third_observed, &block0_fourth_observed,
            &block0_zero_observed, &block1_observed, &block2_observed,
            &marker_observed);
        AI_ASSERT_NO_EXCEPT_AND("under three-block CMO pressure, final CBO.ZERO clears all same-block store generations after CBO.INVAL and double CBOM activity",
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
        AI_ASSERT_NO_EXCEPT_AND("after the four-generation inval-clean-clean-zero CMO probe, block0 and block1 are zero, block2 preserves seed, and guards remain intact",
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
        TEST_ASSERT("backing memory after the four-generation inval-clean-clean-zero CMO probe keeps block0 and block1 fully zero, block2 seed payload, marker update, and all guards intact",
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

bool ai_micro_hs_interleaved_three_block_store_inval_store_cbom_store_cbom_store_cbom_four_generation_no_zero_corner()
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
    const uintptr_t block0_vaddr = hs_page_base(source_page) + 0x3d40U;
    const uintptr_t block1_vaddr = hs_page_base(source_page) + 0x3dc0U;
    const uintptr_t block2_vaddr = hs_page_base(source_page) + 0x3e40U;
    const uintptr_t block0_paddr = phys_page_base(source_page) + 0x3d40U;
    const uintptr_t block1_paddr = phys_page_base(source_page) + 0x3dc0U;
    const uintptr_t block2_paddr = phys_page_base(source_page) + 0x3e40U;
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
    const uintptr_t marker_vaddr = hs_page_base(marker_page) + 0x3d40U;
    const uintptr_t marker_paddr = phys_page_base(marker_page) + 0x3d40U;
    const uintptr_t marker_guard_vaddr = marker_vaddr + sizeof(uint64_t);
    const uintptr_t marker_guard_paddr = marker_paddr + sizeof(uint64_t);
    const uint64_t block0_seeds[AI_MEMBLOCK_CMO_TRIPLE_ROUNDS] = {
        0x3d404d405d406d40ULL,
        0x4e515e516e517e51ULL,
        0x5f626f627f628f62ULL,
    };
    const uint64_t block1_seeds[AI_MEMBLOCK_CMO_TRIPLE_ROUNDS] = {
        0x807060503d403dc0ULL,
        0x918171614e514ed1ULL,
        0xa29282725f625fe2ULL,
    };
    const uint64_t block2_seeds[AI_MEMBLOCK_CMO_TRIPLE_ROUNDS] = {
        0x2468ace03d403e40ULL,
        0x3579bdf14e514f51ULL,
        0x468ace025f626062ULL,
    };
    const uint64_t block0_older_store_values[AI_MEMBLOCK_CMO_TRIPLE_ROUNDS] = {
        0x11003d4022003300ULL,
        0x22114e5133114411ULL,
        0x33225f6244225522ULL,
    };
    const uint64_t block0_second_store_values[AI_MEMBLOCK_CMO_TRIPLE_ROUNDS] = {
        0x00330022003d4011ULL,
        0x11441133114e5122ULL,
        0x22552244225f6233ULL,
    };
    const uint64_t block0_third_store_values[AI_MEMBLOCK_CMO_TRIPLE_ROUNDS] = {
        0xcafe3d40feed3d40ULL,
        0xdb0f4e510ffe4e51ULL,
        0xec205f62200f5f62ULL,
    };
    const uint64_t block0_fourth_store_values[AI_MEMBLOCK_CMO_TRIPLE_ROUNDS] = {
        0xc0013d40deadbeefULL,
        0xd1124e51efbedf00ULL,
        0xe2235f62f0cfe011ULL,
    };
    const uint64_t before_guards[AI_MEMBLOCK_CMO_TRIPLE_ROUNDS] = {
        0xc0013d40d00d3d40ULL,
        0xd1124e51e11e4e51ULL,
        0xe2235f62f22f5f62ULL,
    };
    const uint64_t guard01_values[AI_MEMBLOCK_CMO_TRIPLE_ROUNDS] = {
        0x3d40d00d3d40c001ULL,
        0x4e51e11e4e51d112ULL,
        0x5f62f22f5f62e223ULL,
    };
    const uint64_t guard12_values[AI_MEMBLOCK_CMO_TRIPLE_ROUNDS] = {
        0x314159263d403dc0ULL,
        0x271828184e514ed1ULL,
        0x161803395f625fe2ULL,
    };
    const uint64_t after_guards[AI_MEMBLOCK_CMO_TRIPLE_ROUNDS] = {
        0x083d403e40962695ULL,
        0x194e514f511818282ULL,
        0x2a5f6260622930381ULL,
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
        0x55aa55aa3d403d40ULL,
        0x66bb66bb4e514e51ULL,
        0x77cc77cc5f625f62ULL,
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
        AI_ASSERT_NO_EXCEPT_AND("before the four-generation inval-clean-clean-clean CMO probe starts, all source blocks, guards, and marker are readable",
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
        ai_memblock_cmo_triple_three_block_store_inval_store_cbom_store_cbom_store_cbom_window(
            block0_vaddr, block1_vaddr, block2_vaddr, marker_vaddr,
            block0_older_store_values[round],
            block0_second_store_values[round],
            block0_third_store_values[round],
            block0_fourth_store_values[round], marker_updates[round],
            &block0_older_observed, &block0_second_observed,
            &block0_third_observed, &block0_fourth_observed,
            &block0_seed_observed, &block1_observed, &block2_observed,
            &marker_observed);
        AI_ASSERT_NO_EXCEPT_AND("under three-block CMO pressure, CBO.INVAL followed by triple CBOM keeps all four same-block store generations visible",
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
        AI_ASSERT_NO_EXCEPT_AND("after the four-generation inval-clean-clean-clean CMO probe, block0 preserves all stores, block1 is zero, block2 preserves seed, and guards remain intact",
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
        TEST_ASSERT("backing memory after the four-generation inval-clean-clean-clean CMO probe keeps block0 seed except all four store generations, block1 zero, block2 seed payload, marker update, and all guards intact",
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
