#include "ai_micro_memblock_cmo_triple_common.h"

bool ai_micro_hs_interleaved_two_block_cbozero_cboinval_cbom_no_cross_cmo_owner_corner()
{
    TEST_START();

    const enum test_page source_page = SWITCH1;
    const enum test_page marker_page = SWITCH2;
    const size_t block0_load_word = 4U;
    const size_t block1_load_word = 3U;
    const uintptr_t block0_vaddr = hs_page_base(source_page) + 0x1500U;
    const uintptr_t block1_vaddr = hs_page_base(source_page) + 0x1580U;
    const uintptr_t block0_paddr = phys_page_base(source_page) + 0x1500U;
    const uintptr_t block1_paddr = phys_page_base(source_page) + 0x1580U;
    const uintptr_t block0_zero_vaddr =
        block0_vaddr + 6U * sizeof(uint64_t);
    const uintptr_t block1_inval_vaddr =
        block1_vaddr + 2U * sizeof(uint64_t);
    const uintptr_t block0_clean_vaddr =
        block0_vaddr + 1U * sizeof(uint64_t);
    const uintptr_t block1_flush_vaddr =
        block1_vaddr +
        (AI_MEMBLOCK_CMO_TRIPLE_BLOCK_WORDS - 1U) * sizeof(uint64_t);
    const uintptr_t block0_load_vaddr =
        block0_vaddr + block0_load_word * sizeof(uint64_t);
    const uintptr_t block1_load_vaddr =
        block1_vaddr + block1_load_word * sizeof(uint64_t);
    const uintptr_t before_guard_vaddr = block0_vaddr - sizeof(uint64_t);
    const uintptr_t between_guard_vaddr =
        block0_vaddr +
        AI_MEMBLOCK_CMO_TRIPLE_BLOCK_WORDS * sizeof(uint64_t);
    const uintptr_t after_guard_vaddr =
        block1_vaddr +
        AI_MEMBLOCK_CMO_TRIPLE_BLOCK_WORDS * sizeof(uint64_t);
    const uintptr_t before_guard_paddr = block0_paddr - sizeof(uint64_t);
    const uintptr_t between_guard_paddr =
        block0_paddr +
        AI_MEMBLOCK_CMO_TRIPLE_BLOCK_WORDS * sizeof(uint64_t);
    const uintptr_t after_guard_paddr =
        block1_paddr +
        AI_MEMBLOCK_CMO_TRIPLE_BLOCK_WORDS * sizeof(uint64_t);
    const uintptr_t marker_vaddr = hs_page_base(marker_page) + 0x1500U;
    const uintptr_t marker_paddr = phys_page_base(marker_page) + 0x1500U;
    const uintptr_t marker_guard_vaddr = marker_vaddr + sizeof(uint64_t);
    const uintptr_t marker_guard_paddr = marker_paddr + sizeof(uint64_t);
    const uint64_t block0_seeds[AI_MEMBLOCK_CMO_TRIPLE_ROUNDS] = {
        0x0123456789abcdefULL,
        0x123456789abcdef0ULL,
        0x23456789abcdef01ULL,
    };
    const uint64_t block1_seeds[AI_MEMBLOCK_CMO_TRIPLE_ROUNDS] = {
        0xfedcba9876543210ULL,
        0xedcba9876543210fULL,
        0xdcba9876543210feULL,
    };
    const uint64_t before_guards[AI_MEMBLOCK_CMO_TRIPLE_ROUNDS] = {
        0x0f0e0d0c0b0a0908ULL,
        0x1f1e1d1c1b1a1918ULL,
        0x2f2e2d2c2b2a2928ULL,
    };
    const uint64_t between_guards[AI_MEMBLOCK_CMO_TRIPLE_ROUNDS] = {
        0xaabbccdd00112233ULL,
        0xbbccddee11223344ULL,
        0xccddeeff22334455ULL,
    };
    const uint64_t after_guards[AI_MEMBLOCK_CMO_TRIPLE_ROUNDS] = {
        0x8899aabbccddeeffULL,
        0x99aabbccddeeff00ULL,
        0xaabbccddeeff0011ULL,
    };
    const uint64_t marker_seeds[AI_MEMBLOCK_CMO_TRIPLE_ROUNDS] = {
        0x3141592653589793ULL,
        0x2718281828459045ULL,
        0x1618033988749894ULL,
    };
    const uint64_t marker_updates[AI_MEMBLOCK_CMO_TRIPLE_ROUNDS] = {
        0x4242424243434343ULL,
        0x5353535354545454ULL,
        0x6464646465656565ULL,
    };
    const uint64_t marker_guards[AI_MEMBLOCK_CMO_TRIPLE_ROUNDS] = {
        0x0102030405060708ULL,
        0x1112131415161718ULL,
        0x2122232425262728ULL,
    };

    ai_memblock_cmo_triple_prepare_hs_env();

    for (size_t round = 0; round < AI_MEMBLOCK_CMO_TRIPLE_ROUNDS; ++round) {
        uint64_t block0_observed = 0ULL;
        uint64_t block1_observed = 0ULL;
        uint64_t marker_observed = 0ULL;
        const uint64_t expected_block1_load = block1_seeds[round] +
            block1_load_word * 0x1111111111111111ULL;

        goto_priv(PRIV_M);
        ai_memblock_cmo_triple_seed_block(block0_paddr,
            block0_seeds[round]);
        ai_memblock_cmo_triple_seed_block(block1_paddr,
            block1_seeds[round]);
        write64(before_guard_paddr, before_guards[round]);
        write64(between_guard_paddr, between_guards[round]);
        write64(after_guard_paddr, after_guards[round]);
        write64(marker_paddr, marker_seeds[round]);
        write64(marker_guard_paddr, marker_guards[round]);

        goto_priv(PRIV_HS);
        TEST_SETUP_EXCEPT();
        AI_ASSERT_NO_EXCEPT_AND("before the interleaved two-block CMO owner probe starts, both source blocks, guards, and marker are readable",
            ld(block0_vaddr) == block0_seeds[round] &&
            ld(block0_load_vaddr) ==
                block0_seeds[round] +
                block0_load_word * 0x1111111111111111ULL &&
            ld(block1_vaddr) == block1_seeds[round] &&
            ld(block1_load_vaddr) == expected_block1_load &&
            ld(marker_vaddr) == marker_seeds[round]
        );

        TEST_SETUP_EXCEPT();
        ai_memblock_cmo_triple_two_block_mixed_window(block0_zero_vaddr,
            block1_inval_vaddr, block0_clean_vaddr, block1_flush_vaddr,
            block0_load_vaddr, block1_load_vaddr, marker_vaddr,
            marker_updates[round], &block0_observed, &block1_observed,
            &marker_observed);
        AI_ASSERT_NO_EXCEPT_AND("interleaved CBO.ZERO, CBO.INVAL, and CBOM on two cache blocks keep owner retirement block-local and let marker progress",
            block0_observed == 0ULL &&
            block1_observed == expected_block1_load &&
            marker_observed == marker_updates[round]
        );

        TEST_SETUP_EXCEPT();
        AI_ASSERT_NO_EXCEPT_AND("after the interleaved two-block CMO owner probe, block0 is zero, block1 keeps seed payload, and guards remain intact",
            ld(block0_vaddr) == 0ULL &&
            ld(block0_load_vaddr) == 0ULL &&
            ld(block1_vaddr) == block1_seeds[round] &&
            ld(block1_load_vaddr) == expected_block1_load &&
            ld(before_guard_vaddr) == before_guards[round] &&
            ld(between_guard_vaddr) == between_guards[round] &&
            ld(after_guard_vaddr) == after_guards[round] &&
            ld(marker_guard_vaddr) == marker_guards[round]
        );

        goto_priv(PRIV_M);
        TEST_ASSERT("backing memory after the interleaved two-block CMO owner probe keeps block0 zero, block1 seed payload, marker update, and all guards intact",
            ai_memblock_cmo_triple_block_all_zero(block0_paddr) &&
            ai_memblock_cmo_triple_block_matches_seed(block1_paddr,
                block1_seeds[round]) &&
            read64(before_guard_paddr) == before_guards[round] &&
            read64(between_guard_paddr) == between_guards[round] &&
            read64(after_guard_paddr) == after_guards[round] &&
            read64(marker_paddr) == marker_updates[round] &&
            read64(marker_guard_paddr) == marker_guards[round]
        );
    }

    ai_memblock_cmo_triple_reset_hs_env();

    TEST_END();
}

bool ai_micro_hs_interleaved_two_block_dual_cbozero_cmo_no_lost_zero_owner_corner()
{
    TEST_START();

    const enum test_page source_page = SWITCH1;
    const enum test_page marker_page = SWITCH2;
    const size_t block0_load_word = 3U;
    const size_t block1_load_word = 5U;
    const uintptr_t block0_vaddr = hs_page_base(source_page) + 0x1700U;
    const uintptr_t block1_vaddr = hs_page_base(source_page) + 0x1780U;
    const uintptr_t block0_paddr = phys_page_base(source_page) + 0x1700U;
    const uintptr_t block1_paddr = phys_page_base(source_page) + 0x1780U;
    const uintptr_t block0_zero_vaddr =
        block0_vaddr + 6U * sizeof(uint64_t);
    const uintptr_t block1_zero_vaddr =
        block1_vaddr + 1U * sizeof(uint64_t);
    const uintptr_t block0_inval_vaddr =
        block0_vaddr + 2U * sizeof(uint64_t);
    const uintptr_t block1_clean_vaddr =
        block1_vaddr + 3U * sizeof(uint64_t);
    const uintptr_t block0_flush_vaddr =
        block0_vaddr +
        (AI_MEMBLOCK_CMO_TRIPLE_BLOCK_WORDS - 1U) * sizeof(uint64_t);
    const uintptr_t block1_inval_vaddr =
        block1_vaddr + 5U * sizeof(uint64_t);
    const uintptr_t block0_load_vaddr =
        block0_vaddr + block0_load_word * sizeof(uint64_t);
    const uintptr_t block1_load_vaddr =
        block1_vaddr + block1_load_word * sizeof(uint64_t);
    const uintptr_t before_guard_vaddr = block0_vaddr - sizeof(uint64_t);
    const uintptr_t between_guard_vaddr =
        block0_vaddr +
        AI_MEMBLOCK_CMO_TRIPLE_BLOCK_WORDS * sizeof(uint64_t);
    const uintptr_t after_guard_vaddr =
        block1_vaddr +
        AI_MEMBLOCK_CMO_TRIPLE_BLOCK_WORDS * sizeof(uint64_t);
    const uintptr_t before_guard_paddr = block0_paddr - sizeof(uint64_t);
    const uintptr_t between_guard_paddr =
        block0_paddr +
        AI_MEMBLOCK_CMO_TRIPLE_BLOCK_WORDS * sizeof(uint64_t);
    const uintptr_t after_guard_paddr =
        block1_paddr +
        AI_MEMBLOCK_CMO_TRIPLE_BLOCK_WORDS * sizeof(uint64_t);
    const uintptr_t marker_vaddr = hs_page_base(marker_page) + 0x1700U;
    const uintptr_t marker_paddr = phys_page_base(marker_page) + 0x1700U;
    const uintptr_t marker_guard_vaddr = marker_vaddr + sizeof(uint64_t);
    const uintptr_t marker_guard_paddr = marker_paddr + sizeof(uint64_t);
    const uint64_t block0_seeds[AI_MEMBLOCK_CMO_TRIPLE_ROUNDS] = {
        0x1020304050607080ULL,
        0x2131415161718191ULL,
        0x32425262728292a2ULL,
    };
    const uint64_t block1_seeds[AI_MEMBLOCK_CMO_TRIPLE_ROUNDS] = {
        0x8877665544332211ULL,
        0x7766554433221100ULL,
        0x66554433221100ffULL,
    };
    const uint64_t before_guards[AI_MEMBLOCK_CMO_TRIPLE_ROUNDS] = {
        0xf00ff00f0ff00ff0ULL,
        0xe11ee11e1ee11ee1ULL,
        0xd22dd22d2dd22dd2ULL,
    };
    const uint64_t between_guards[AI_MEMBLOCK_CMO_TRIPLE_ROUNDS] = {
        0x1357135724682468ULL,
        0x2468246835793579ULL,
        0x35793579468a468aULL,
    };
    const uint64_t after_guards[AI_MEMBLOCK_CMO_TRIPLE_ROUNDS] = {
        0xabcdef0123456789ULL,
        0xbcdef0123456789aULL,
        0xcdef0123456789abULL,
    };
    const uint64_t marker_seeds[AI_MEMBLOCK_CMO_TRIPLE_ROUNDS] = {
        0x5a5aa5a55a5aa5a5ULL,
        0x6b6bb6b66b6bb6b6ULL,
        0x7c7cc7c77c7cc7c7ULL,
    };
    const uint64_t marker_updates[AI_MEMBLOCK_CMO_TRIPLE_ROUNDS] = {
        0x0101020203030404ULL,
        0x1111121213131414ULL,
        0x2121222223232424ULL,
    };
    const uint64_t marker_guards[AI_MEMBLOCK_CMO_TRIPLE_ROUNDS] = {
        0xdeadbeefcafebabeULL,
        0xcafebabedeadbeefULL,
        0xbeefdeadcafef00dULL,
    };

    ai_memblock_cmo_triple_prepare_hs_env();

    for (size_t round = 0; round < AI_MEMBLOCK_CMO_TRIPLE_ROUNDS; ++round) {
        uint64_t block0_observed = 0ULL;
        uint64_t block1_observed = 0ULL;
        uint64_t marker_observed = 0ULL;

        goto_priv(PRIV_M);
        ai_memblock_cmo_triple_seed_block(block0_paddr,
            block0_seeds[round]);
        ai_memblock_cmo_triple_seed_block(block1_paddr,
            block1_seeds[round]);
        write64(before_guard_paddr, before_guards[round]);
        write64(between_guard_paddr, between_guards[round]);
        write64(after_guard_paddr, after_guards[round]);
        write64(marker_paddr, marker_seeds[round]);
        write64(marker_guard_paddr, marker_guards[round]);

        goto_priv(PRIV_HS);
        TEST_SETUP_EXCEPT();
        AI_ASSERT_NO_EXCEPT_AND("before the dual-zero two-block CMO owner probe starts, both source blocks, guards, and marker are readable",
            ld(block0_vaddr) == block0_seeds[round] &&
            ld(block0_load_vaddr) ==
                block0_seeds[round] +
                block0_load_word * 0x1111111111111111ULL &&
            ld(block1_vaddr) == block1_seeds[round] &&
            ld(block1_load_vaddr) ==
                block1_seeds[round] +
                block1_load_word * 0x1111111111111111ULL &&
            ld(marker_vaddr) == marker_seeds[round]
        );

        TEST_SETUP_EXCEPT();
        ai_memblock_cmo_triple_two_block_dual_zero_window(
            block0_zero_vaddr, block1_zero_vaddr, block0_inval_vaddr,
            block1_clean_vaddr, block0_flush_vaddr, block1_inval_vaddr,
            block0_load_vaddr, block1_load_vaddr, marker_vaddr,
            marker_updates[round], &block0_observed, &block1_observed,
            &marker_observed);
        AI_ASSERT_NO_EXCEPT_AND("dual interleaved CBO.ZERO chains across two cache blocks do not drop either zero owner and let marker progress",
            block0_observed == 0ULL &&
            block1_observed == 0ULL &&
            marker_observed == marker_updates[round]
        );

        TEST_SETUP_EXCEPT();
        AI_ASSERT_NO_EXCEPT_AND("after the dual-zero two-block CMO owner probe, both blocks are zero and guards remain intact",
            ld(block0_vaddr) == 0ULL &&
            ld(block0_load_vaddr) == 0ULL &&
            ld(block1_vaddr) == 0ULL &&
            ld(block1_load_vaddr) == 0ULL &&
            ld(before_guard_vaddr) == before_guards[round] &&
            ld(between_guard_vaddr) == between_guards[round] &&
            ld(after_guard_vaddr) == after_guards[round] &&
            ld(marker_guard_vaddr) == marker_guards[round]
        );

        goto_priv(PRIV_M);
        TEST_ASSERT("backing memory after the dual-zero two-block CMO owner probe keeps both blocks zero, marker update, and all guards intact",
            ai_memblock_cmo_triple_block_all_zero(block0_paddr) &&
            ai_memblock_cmo_triple_block_all_zero(block1_paddr) &&
            read64(before_guard_paddr) == before_guards[round] &&
            read64(between_guard_paddr) == between_guards[round] &&
            read64(after_guard_paddr) == after_guards[round] &&
            read64(marker_paddr) == marker_updates[round] &&
            read64(marker_guard_paddr) == marker_guards[round]
        );
    }

    ai_memblock_cmo_triple_reset_hs_env();

    TEST_END();
}

bool ai_micro_hs_interleaved_two_block_cboinval_cbom_cbozero_no_cross_cmo_owner_corner()
{
    TEST_START();

    const enum test_page source_page = SWITCH1;
    const enum test_page marker_page = SWITCH2;
    const size_t block0_load_word = 2U;
    const size_t block1_load_word = 5U;
    const uintptr_t block0_vaddr = hs_page_base(source_page) + 0x1600U;
    const uintptr_t block1_vaddr = hs_page_base(source_page) + 0x1680U;
    const uintptr_t block0_paddr = phys_page_base(source_page) + 0x1600U;
    const uintptr_t block1_paddr = phys_page_base(source_page) + 0x1680U;
    const uintptr_t block0_inval_vaddr =
        block0_vaddr + 3U * sizeof(uint64_t);
    const uintptr_t block1_zero_vaddr =
        block1_vaddr + 6U * sizeof(uint64_t);
    const uintptr_t block0_clean_vaddr =
        block0_vaddr + 1U * sizeof(uint64_t);
    const uintptr_t block1_inval_vaddr =
        block1_vaddr + 2U * sizeof(uint64_t);
    const uintptr_t block0_flush_vaddr =
        block0_vaddr +
        (AI_MEMBLOCK_CMO_TRIPLE_BLOCK_WORDS - 1U) * sizeof(uint64_t);
    const uintptr_t block0_load_vaddr =
        block0_vaddr + block0_load_word * sizeof(uint64_t);
    const uintptr_t block1_load_vaddr =
        block1_vaddr + block1_load_word * sizeof(uint64_t);
    const uintptr_t before_guard_vaddr = block0_vaddr - sizeof(uint64_t);
    const uintptr_t between_guard_vaddr =
        block0_vaddr +
        AI_MEMBLOCK_CMO_TRIPLE_BLOCK_WORDS * sizeof(uint64_t);
    const uintptr_t after_guard_vaddr =
        block1_vaddr +
        AI_MEMBLOCK_CMO_TRIPLE_BLOCK_WORDS * sizeof(uint64_t);
    const uintptr_t before_guard_paddr = block0_paddr - sizeof(uint64_t);
    const uintptr_t between_guard_paddr =
        block0_paddr +
        AI_MEMBLOCK_CMO_TRIPLE_BLOCK_WORDS * sizeof(uint64_t);
    const uintptr_t after_guard_paddr =
        block1_paddr +
        AI_MEMBLOCK_CMO_TRIPLE_BLOCK_WORDS * sizeof(uint64_t);
    const uintptr_t marker_vaddr = hs_page_base(marker_page) + 0x1600U;
    const uintptr_t marker_paddr = phys_page_base(marker_page) + 0x1600U;
    const uintptr_t marker_guard_vaddr = marker_vaddr + sizeof(uint64_t);
    const uintptr_t marker_guard_paddr = marker_paddr + sizeof(uint64_t);
    const uint64_t block0_seeds[AI_MEMBLOCK_CMO_TRIPLE_ROUNDS] = {
        0x13579bdf2468ace0ULL,
        0x2468ace03579bdf1ULL,
        0x3579bdf1468ace02ULL,
    };
    const uint64_t block1_seeds[AI_MEMBLOCK_CMO_TRIPLE_ROUNDS] = {
        0xf0e1d2c3b4a59687ULL,
        0xe1d2c3b4a5968778ULL,
        0xd2c3b4a596877869ULL,
    };
    const uint64_t before_guards[AI_MEMBLOCK_CMO_TRIPLE_ROUNDS] = {
        0x55aa55aa00ff00ffULL,
        0x66bb66bb11ee11eeULL,
        0x77cc77cc22dd22ddULL,
    };
    const uint64_t between_guards[AI_MEMBLOCK_CMO_TRIPLE_ROUNDS] = {
        0x0123012345674567ULL,
        0x1234123456785678ULL,
        0x2345234567896789ULL,
    };
    const uint64_t after_guards[AI_MEMBLOCK_CMO_TRIPLE_ROUNDS] = {
        0xfedcfedcba98ba98ULL,
        0xedcbedcba987a987ULL,
        0xdcfadcba98769876ULL,
    };
    const uint64_t marker_seeds[AI_MEMBLOCK_CMO_TRIPLE_ROUNDS] = {
        0x0a0b0c0d0e0f0102ULL,
        0x1a1b1c1d1e1f1112ULL,
        0x2a2b2c2d2e2f2122ULL,
    };
    const uint64_t marker_updates[AI_MEMBLOCK_CMO_TRIPLE_ROUNDS] = {
        0xa5a5a5a55a5a5a5aULL,
        0xb6b6b6b66b6b6b6bULL,
        0xc7c7c7c77c7c7c7cULL,
    };
    const uint64_t marker_guards[AI_MEMBLOCK_CMO_TRIPLE_ROUNDS] = {
        0x9091929394959697ULL,
        0x8081828384858687ULL,
        0x7071727374757677ULL,
    };

    ai_memblock_cmo_triple_prepare_hs_env();

    for (size_t round = 0; round < AI_MEMBLOCK_CMO_TRIPLE_ROUNDS; ++round) {
        uint64_t block0_observed = 0ULL;
        uint64_t block1_observed = 0ULL;
        uint64_t marker_observed = 0ULL;
        const uint64_t expected_block0_load = block0_seeds[round] +
            block0_load_word * 0x1111111111111111ULL;

        goto_priv(PRIV_M);
        ai_memblock_cmo_triple_seed_block(block0_paddr,
            block0_seeds[round]);
        ai_memblock_cmo_triple_seed_block(block1_paddr,
            block1_seeds[round]);
        write64(before_guard_paddr, before_guards[round]);
        write64(between_guard_paddr, between_guards[round]);
        write64(after_guard_paddr, after_guards[round]);
        write64(marker_paddr, marker_seeds[round]);
        write64(marker_guard_paddr, marker_guards[round]);

        goto_priv(PRIV_HS);
        TEST_SETUP_EXCEPT();
        AI_ASSERT_NO_EXCEPT_AND("before the opposite two-block CMO owner probe starts, both source blocks, guards, and marker are readable",
            ld(block0_vaddr) == block0_seeds[round] &&
            ld(block0_load_vaddr) == expected_block0_load &&
            ld(block1_vaddr) == block1_seeds[round] &&
            ld(block1_load_vaddr) ==
                block1_seeds[round] +
                block1_load_word * 0x1111111111111111ULL &&
            ld(marker_vaddr) == marker_seeds[round]
        );

        TEST_SETUP_EXCEPT();
        ai_memblock_cmo_triple_two_block_opposite_window(
            block0_inval_vaddr, block1_zero_vaddr, block0_clean_vaddr,
            block1_inval_vaddr, block0_flush_vaddr, block0_load_vaddr,
            block1_load_vaddr, marker_vaddr, marker_updates[round],
            &block0_observed, &block1_observed, &marker_observed);
        AI_ASSERT_NO_EXCEPT_AND("opposite interleaved CBO.INVAL/CBOM and CBO.ZERO/CBO.INVAL chains keep each block's owner retirement independent and let marker progress",
            block0_observed == expected_block0_load &&
            block1_observed == 0ULL &&
            marker_observed == marker_updates[round]
        );

        TEST_SETUP_EXCEPT();
        AI_ASSERT_NO_EXCEPT_AND("after the opposite two-block CMO owner probe, block0 keeps seed payload, block1 is zero, and guards remain intact",
            ld(block0_vaddr) == block0_seeds[round] &&
            ld(block0_load_vaddr) == expected_block0_load &&
            ld(block1_vaddr) == 0ULL &&
            ld(block1_load_vaddr) == 0ULL &&
            ld(before_guard_vaddr) == before_guards[round] &&
            ld(between_guard_vaddr) == between_guards[round] &&
            ld(after_guard_vaddr) == after_guards[round] &&
            ld(marker_guard_vaddr) == marker_guards[round]
        );

        goto_priv(PRIV_M);
        TEST_ASSERT("backing memory after the opposite two-block CMO owner probe keeps block0 seed payload, block1 zero, marker update, and all guards intact",
            ai_memblock_cmo_triple_block_matches_seed(block0_paddr,
                block0_seeds[round]) &&
            ai_memblock_cmo_triple_block_all_zero(block1_paddr) &&
            read64(before_guard_paddr) == before_guards[round] &&
            read64(between_guard_paddr) == between_guards[round] &&
            read64(after_guard_paddr) == after_guards[round] &&
            read64(marker_paddr) == marker_updates[round] &&
            read64(marker_guard_paddr) == marker_guards[round]
        );
    }

    ai_memblock_cmo_triple_reset_hs_env();

    TEST_END();
}

bool ai_micro_hs_interleaved_two_block_cboinval_cbom_dual_preserve_no_cross_cmo_owner_corner()
{
    TEST_START();

    const enum test_page source_page = SWITCH1;
    const enum test_page marker_page = SWITCH2;
    const size_t block0_load_word = 2U;
    const size_t block1_load_word = 4U;
    const uintptr_t block0_vaddr = hs_page_base(source_page) + 0x1800U;
    const uintptr_t block1_vaddr = hs_page_base(source_page) + 0x1880U;
    const uintptr_t block0_paddr = phys_page_base(source_page) + 0x1800U;
    const uintptr_t block1_paddr = phys_page_base(source_page) + 0x1880U;
    const uintptr_t block0_inval_vaddr =
        block0_vaddr + 3U * sizeof(uint64_t);
    const uintptr_t block1_inval_vaddr =
        block1_vaddr + 5U * sizeof(uint64_t);
    const uintptr_t block0_clean_vaddr =
        block0_vaddr + 1U * sizeof(uint64_t);
    const uintptr_t block1_clean_vaddr =
        block1_vaddr + 2U * sizeof(uint64_t);
    const uintptr_t block0_flush_vaddr =
        block0_vaddr +
        (AI_MEMBLOCK_CMO_TRIPLE_BLOCK_WORDS - 1U) * sizeof(uint64_t);
    const uintptr_t block1_flush_vaddr =
        block1_vaddr +
        (AI_MEMBLOCK_CMO_TRIPLE_BLOCK_WORDS - 1U) * sizeof(uint64_t);
    const uintptr_t block0_load_vaddr =
        block0_vaddr + block0_load_word * sizeof(uint64_t);
    const uintptr_t block1_load_vaddr =
        block1_vaddr + block1_load_word * sizeof(uint64_t);
    const uintptr_t before_guard_vaddr = block0_vaddr - sizeof(uint64_t);
    const uintptr_t between_guard_vaddr =
        block0_vaddr +
        AI_MEMBLOCK_CMO_TRIPLE_BLOCK_WORDS * sizeof(uint64_t);
    const uintptr_t after_guard_vaddr =
        block1_vaddr +
        AI_MEMBLOCK_CMO_TRIPLE_BLOCK_WORDS * sizeof(uint64_t);
    const uintptr_t before_guard_paddr = block0_paddr - sizeof(uint64_t);
    const uintptr_t between_guard_paddr =
        block0_paddr +
        AI_MEMBLOCK_CMO_TRIPLE_BLOCK_WORDS * sizeof(uint64_t);
    const uintptr_t after_guard_paddr =
        block1_paddr +
        AI_MEMBLOCK_CMO_TRIPLE_BLOCK_WORDS * sizeof(uint64_t);
    const uintptr_t marker_vaddr = hs_page_base(marker_page) + 0x1800U;
    const uintptr_t marker_paddr = phys_page_base(marker_page) + 0x1800U;
    const uintptr_t marker_guard_vaddr = marker_vaddr + sizeof(uint64_t);
    const uintptr_t marker_guard_paddr = marker_paddr + sizeof(uint64_t);
    const uint64_t block0_seeds[AI_MEMBLOCK_CMO_TRIPLE_ROUNDS] = {
        0x0f1e2d3c4b5a6978ULL,
        0x1e2d3c4b5a697887ULL,
        0x2d3c4b5a69788796ULL,
    };
    const uint64_t block1_seeds[AI_MEMBLOCK_CMO_TRIPLE_ROUNDS] = {
        0x89abcdef01234567ULL,
        0x9abcdef012345678ULL,
        0xabcdef0123456789ULL,
    };
    const uint64_t before_guards[AI_MEMBLOCK_CMO_TRIPLE_ROUNDS] = {
        0x0101010102020202ULL,
        0x1111111112121212ULL,
        0x2121212122222222ULL,
    };
    const uint64_t between_guards[AI_MEMBLOCK_CMO_TRIPLE_ROUNDS] = {
        0x33445566778899aaULL,
        0x445566778899aabbULL,
        0x5566778899aabbccULL,
    };
    const uint64_t after_guards[AI_MEMBLOCK_CMO_TRIPLE_ROUNDS] = {
        0xccbbaa9988776655ULL,
        0xddccbbaa99887766ULL,
        0xeeddccbbaa998877ULL,
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
        0xa0a1a2a3a4a5a6a7ULL,
        0xb0b1b2b3b4b5b6b7ULL,
        0xc0c1c2c3c4c5c6c7ULL,
    };

    ai_memblock_cmo_triple_prepare_hs_env();

    for (size_t round = 0; round < AI_MEMBLOCK_CMO_TRIPLE_ROUNDS; ++round) {
        uint64_t block0_observed = 0ULL;
        uint64_t block1_observed = 0ULL;
        uint64_t marker_observed = 0ULL;
        const uint64_t expected_block0_load = block0_seeds[round] +
            block0_load_word * 0x1111111111111111ULL;
        const uint64_t expected_block1_load = block1_seeds[round] +
            block1_load_word * 0x1111111111111111ULL;

        goto_priv(PRIV_M);
        ai_memblock_cmo_triple_seed_block(block0_paddr,
            block0_seeds[round]);
        ai_memblock_cmo_triple_seed_block(block1_paddr,
            block1_seeds[round]);
        write64(before_guard_paddr, before_guards[round]);
        write64(between_guard_paddr, between_guards[round]);
        write64(after_guard_paddr, after_guards[round]);
        write64(marker_paddr, marker_seeds[round]);
        write64(marker_guard_paddr, marker_guards[round]);

        goto_priv(PRIV_HS);
        TEST_SETUP_EXCEPT();
        AI_ASSERT_NO_EXCEPT_AND("before the dual-preserve two-block CMO owner probe starts, both source blocks, guards, and marker are readable",
            ld(block0_vaddr) == block0_seeds[round] &&
            ld(block0_load_vaddr) == expected_block0_load &&
            ld(block1_vaddr) == block1_seeds[round] &&
            ld(block1_load_vaddr) == expected_block1_load &&
            ld(marker_vaddr) == marker_seeds[round]
        );

        TEST_SETUP_EXCEPT();
        ai_memblock_cmo_triple_two_block_dual_preserve_window(
            block0_inval_vaddr, block1_inval_vaddr, block0_clean_vaddr,
            block1_clean_vaddr, block0_flush_vaddr, block1_flush_vaddr,
            block0_load_vaddr, block1_load_vaddr, marker_vaddr,
            marker_updates[round], &block0_observed, &block1_observed,
            &marker_observed);
        AI_ASSERT_NO_EXCEPT_AND("dual interleaved CBO.INVAL and CBOM chains across two cache blocks preserve both block payloads and let marker progress",
            block0_observed == expected_block0_load &&
            block1_observed == expected_block1_load &&
            marker_observed == marker_updates[round]
        );

        TEST_SETUP_EXCEPT();
        AI_ASSERT_NO_EXCEPT_AND("after the dual-preserve two-block CMO owner probe, both blocks keep seed payloads and guards remain intact",
            ld(block0_vaddr) == block0_seeds[round] &&
            ld(block0_load_vaddr) == expected_block0_load &&
            ld(block1_vaddr) == block1_seeds[round] &&
            ld(block1_load_vaddr) == expected_block1_load &&
            ld(before_guard_vaddr) == before_guards[round] &&
            ld(between_guard_vaddr) == between_guards[round] &&
            ld(after_guard_vaddr) == after_guards[round] &&
            ld(marker_guard_vaddr) == marker_guards[round]
        );

        goto_priv(PRIV_M);
        TEST_ASSERT("backing memory after the dual-preserve two-block CMO owner probe keeps both block seed payloads, marker update, and all guards intact",
            ai_memblock_cmo_triple_block_matches_seed(block0_paddr,
                block0_seeds[round]) &&
            ai_memblock_cmo_triple_block_matches_seed(block1_paddr,
                block1_seeds[round]) &&
            read64(before_guard_paddr) == before_guards[round] &&
            read64(between_guard_paddr) == between_guards[round] &&
            read64(after_guard_paddr) == after_guards[round] &&
            read64(marker_paddr) == marker_updates[round] &&
            read64(marker_guard_paddr) == marker_guards[round]
        );
    }

    ai_memblock_cmo_triple_reset_hs_env();

    TEST_END();
}

bool ai_micro_hs_interleaved_three_block_round_robin_cmo_no_cross_owner_corner()
{
    TEST_START();

    const enum test_page source_page = SWITCH1;
    const enum test_page marker_page = SWITCH2;
    const size_t block0_load_word = 4U;
    const size_t block1_load_word = 3U;
    const size_t block2_load_word = 5U;
    const uintptr_t block0_vaddr = hs_page_base(source_page) + 0x1900U;
    const uintptr_t block1_vaddr = hs_page_base(source_page) + 0x1980U;
    const uintptr_t block2_vaddr = hs_page_base(source_page) + 0x1a00U;
    const uintptr_t block0_paddr = phys_page_base(source_page) + 0x1900U;
    const uintptr_t block1_paddr = phys_page_base(source_page) + 0x1980U;
    const uintptr_t block2_paddr = phys_page_base(source_page) + 0x1a00U;
    const uintptr_t block0_zero_vaddr =
        block0_vaddr + 6U * sizeof(uint64_t);
    const uintptr_t block1_inval_vaddr =
        block1_vaddr + 2U * sizeof(uint64_t);
    const uintptr_t block2_zero_vaddr =
        block2_vaddr + 1U * sizeof(uint64_t);
    const uintptr_t block0_inval_vaddr =
        block0_vaddr + 1U * sizeof(uint64_t);
    const uintptr_t block1_clean_vaddr =
        block1_vaddr + 4U * sizeof(uint64_t);
    const uintptr_t block2_flush_vaddr =
        block2_vaddr +
        (AI_MEMBLOCK_CMO_TRIPLE_BLOCK_WORDS - 1U) * sizeof(uint64_t);
    const uintptr_t block1_flush_vaddr =
        block1_vaddr +
        (AI_MEMBLOCK_CMO_TRIPLE_BLOCK_WORDS - 1U) * sizeof(uint64_t);
    const uintptr_t block0_load_vaddr =
        block0_vaddr + block0_load_word * sizeof(uint64_t);
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
    const uintptr_t marker_vaddr = hs_page_base(marker_page) + 0x1900U;
    const uintptr_t marker_paddr = phys_page_base(marker_page) + 0x1900U;
    const uintptr_t marker_guard_vaddr = marker_vaddr + sizeof(uint64_t);
    const uintptr_t marker_guard_paddr = marker_paddr + sizeof(uint64_t);
    const uint64_t block0_seeds[AI_MEMBLOCK_CMO_TRIPLE_ROUNDS] = {
        0x13572468ace0bdf9ULL,
        0x24683579bdf1ce0aULL,
        0x3579468ace02df1bULL,
    };
    const uint64_t block1_seeds[AI_MEMBLOCK_CMO_TRIPLE_ROUNDS] = {
        0xfdb97531eca86420ULL,
        0xeca86420db97531fULL,
        0xdb97531fca86420eULL,
    };
    const uint64_t block2_seeds[AI_MEMBLOCK_CMO_TRIPLE_ROUNDS] = {
        0x0c1d2e3f40516273ULL,
        0x1d2e3f4051627384ULL,
        0x2e3f405162738495ULL,
    };
    const uint64_t before_guards[AI_MEMBLOCK_CMO_TRIPLE_ROUNDS] = {
        0x1122334455667788ULL,
        0x2233445566778899ULL,
        0x33445566778899aaULL,
    };
    const uint64_t guard01_values[AI_MEMBLOCK_CMO_TRIPLE_ROUNDS] = {
        0xa5a501015a5afefeULL,
        0xb6b612126b6bededULL,
        0xc7c723237c7cdcdcULL,
    };
    const uint64_t guard12_values[AI_MEMBLOCK_CMO_TRIPLE_ROUNDS] = {
        0x0101a5a5fefe5a5aULL,
        0x1212b6b6eded6b6bULL,
        0x2323c7c7dcdc7c7cULL,
    };
    const uint64_t after_guards[AI_MEMBLOCK_CMO_TRIPLE_ROUNDS] = {
        0x8877665544332211ULL,
        0x9988776655443322ULL,
        0xaa99887766554433ULL,
    };
    const uint64_t marker_seeds[AI_MEMBLOCK_CMO_TRIPLE_ROUNDS] = {
        0x0a0b0c0d0e0f1011ULL,
        0x1a1b1c1d1e1f2021ULL,
        0x2a2b2c2d2e2f3031ULL,
    };
    const uint64_t marker_updates[AI_MEMBLOCK_CMO_TRIPLE_ROUNDS] = {
        0x7172737475767778ULL,
        0x8182838485868788ULL,
        0x9192939495969798ULL,
    };
    const uint64_t marker_guards[AI_MEMBLOCK_CMO_TRIPLE_ROUNDS] = {
        0xd0d1d2d3d4d5d6d7ULL,
        0xe0e1e2e3e4e5e6e7ULL,
        0xf0f1f2f3f4f5f6f7ULL,
    };

    ai_memblock_cmo_triple_prepare_hs_env();

    for (size_t round = 0; round < AI_MEMBLOCK_CMO_TRIPLE_ROUNDS; ++round) {
        uint64_t block0_observed = 0ULL;
        uint64_t block1_observed = 0ULL;
        uint64_t block2_observed = 0ULL;
        uint64_t marker_observed = 0ULL;
        const uint64_t expected_block1_load = block1_seeds[round] +
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
        AI_ASSERT_NO_EXCEPT_AND("before the three-block round-robin CMO owner probe starts, all source blocks, guards, and marker are readable",
            ld(block0_vaddr) == block0_seeds[round] &&
            ld(block0_load_vaddr) ==
                block0_seeds[round] +
                block0_load_word * 0x1111111111111111ULL &&
            ld(block1_vaddr) == block1_seeds[round] &&
            ld(block1_load_vaddr) == expected_block1_load &&
            ld(block2_vaddr) == block2_seeds[round] &&
            ld(block2_load_vaddr) ==
                block2_seeds[round] +
                block2_load_word * 0x1111111111111111ULL &&
            ld(marker_vaddr) == marker_seeds[round]
        );

        TEST_SETUP_EXCEPT();
        ai_memblock_cmo_triple_three_block_round_robin_window(
            block0_zero_vaddr, block1_inval_vaddr, block2_zero_vaddr,
            block0_inval_vaddr, block1_clean_vaddr, block2_flush_vaddr,
            block1_flush_vaddr, block0_load_vaddr, block1_load_vaddr,
            block2_load_vaddr, marker_vaddr, marker_updates[round],
            &block0_observed, &block1_observed, &block2_observed,
            &marker_observed);
        AI_ASSERT_NO_EXCEPT_AND("round-robin CBO.ZERO, CBO.INVAL, and CBOM across three cache blocks keep each owner block-local and let marker progress",
            block0_observed == 0ULL &&
            block1_observed == expected_block1_load &&
            block2_observed == 0ULL &&
            marker_observed == marker_updates[round]
        );

        TEST_SETUP_EXCEPT();
        AI_ASSERT_NO_EXCEPT_AND("after the three-block round-robin CMO owner probe, zero and preserve results stay block-local and guards remain intact",
            ld(block0_vaddr) == 0ULL &&
            ld(block0_load_vaddr) == 0ULL &&
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
        TEST_ASSERT("backing memory after the three-block round-robin CMO owner probe keeps block0 and block2 zero, block1 seed payload, marker update, and all guards intact",
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

bool ai_micro_hs_interleaved_three_block_cmo_younger_scalar_store_no_lost_payload_corner()
{
    TEST_START();

    const enum test_page source_page = SWITCH1;
    const enum test_page marker_page = SWITCH2;
    const size_t block0_store_word = 2U;
    const size_t block0_zero_load_word = 5U;
    const size_t block1_load_word = 3U;
    const size_t block2_load_word = 6U;
    const uintptr_t block0_vaddr = hs_page_base(source_page) + 0x1b00U;
    const uintptr_t block1_vaddr = hs_page_base(source_page) + 0x1b80U;
    const uintptr_t block2_vaddr = hs_page_base(source_page) + 0x1c00U;
    const uintptr_t block0_paddr = phys_page_base(source_page) + 0x1b00U;
    const uintptr_t block1_paddr = phys_page_base(source_page) + 0x1b80U;
    const uintptr_t block2_paddr = phys_page_base(source_page) + 0x1c00U;
    const uintptr_t block0_zero_vaddr =
        block0_vaddr + 6U * sizeof(uint64_t);
    const uintptr_t block1_inval_vaddr =
        block1_vaddr + 2U * sizeof(uint64_t);
    const uintptr_t block2_zero_vaddr =
        block2_vaddr + 1U * sizeof(uint64_t);
    const uintptr_t block0_store_vaddr =
        block0_vaddr + block0_store_word * sizeof(uint64_t);
    const uintptr_t block0_zero_load_vaddr =
        block0_vaddr + block0_zero_load_word * sizeof(uint64_t);
    const uintptr_t block1_clean_vaddr =
        block1_vaddr + 4U * sizeof(uint64_t);
    const uintptr_t block2_flush_vaddr =
        block2_vaddr +
        (AI_MEMBLOCK_CMO_TRIPLE_BLOCK_WORDS - 1U) * sizeof(uint64_t);
    const uintptr_t block1_flush_vaddr =
        block1_vaddr +
        (AI_MEMBLOCK_CMO_TRIPLE_BLOCK_WORDS - 1U) * sizeof(uint64_t);
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
    const uintptr_t marker_vaddr = hs_page_base(marker_page) + 0x1b00U;
    const uintptr_t marker_paddr = phys_page_base(marker_page) + 0x1b00U;
    const uintptr_t marker_guard_vaddr = marker_vaddr + sizeof(uint64_t);
    const uintptr_t marker_guard_paddr = marker_paddr + sizeof(uint64_t);
    const uint64_t block0_seeds[AI_MEMBLOCK_CMO_TRIPLE_ROUNDS] = {
        0x1021324354657687ULL,
        0x2132435465768798ULL,
        0x32435465768798a9ULL,
    };
    const uint64_t block1_seeds[AI_MEMBLOCK_CMO_TRIPLE_ROUNDS] = {
        0xf1e2d3c4b5a69788ULL,
        0xe2d3c4b5a6978879ULL,
        0xd3c4b5a69788796aULL,
    };
    const uint64_t block2_seeds[AI_MEMBLOCK_CMO_TRIPLE_ROUNDS] = {
        0x0f1e2d3c4b5a6978ULL,
        0x1e2d3c4b5a697887ULL,
        0x2d3c4b5a69788796ULL,
    };
    const uint64_t block0_store_values[AI_MEMBLOCK_CMO_TRIPLE_ROUNDS] = {
        0x1122aabb3344ccddULL,
        0x2233bbcc4455ddeeULL,
        0x3344ccdd5566eeffULL,
    };
    const uint64_t before_guards[AI_MEMBLOCK_CMO_TRIPLE_ROUNDS] = {
        0x0123456789abcdefULL,
        0x123456789abcdef0ULL,
        0x23456789abcdef01ULL,
    };
    const uint64_t guard01_values[AI_MEMBLOCK_CMO_TRIPLE_ROUNDS] = {
        0xfedcba9876543210ULL,
        0xedcba9876543210fULL,
        0xdcba9876543210feULL,
    };
    const uint64_t guard12_values[AI_MEMBLOCK_CMO_TRIPLE_ROUNDS] = {
        0x55aa55aa33cc33ccULL,
        0x66bb66bb44dd44ddULL,
        0x77cc77cc55ee55eeULL,
    };
    const uint64_t after_guards[AI_MEMBLOCK_CMO_TRIPLE_ROUNDS] = {
        0xaa55aa55cc33cc33ULL,
        0xbb66bb66dd44dd44ULL,
        0xcc77cc77ee55ee55ULL,
    };
    const uint64_t marker_seeds[AI_MEMBLOCK_CMO_TRIPLE_ROUNDS] = {
        0x1010101020202020ULL,
        0x3030303040404040ULL,
        0x5050505060606060ULL,
    };
    const uint64_t marker_updates[AI_MEMBLOCK_CMO_TRIPLE_ROUNDS] = {
        0xa0a0a0a0b0b0b0b0ULL,
        0xb1b1b1b1c1c1c1c1ULL,
        0xc2c2c2c2d2d2d2d2ULL,
    };
    const uint64_t marker_guards[AI_MEMBLOCK_CMO_TRIPLE_ROUNDS] = {
        0x0807060504030201ULL,
        0x1817161514131211ULL,
        0x2827262524232221ULL,
    };

    ai_memblock_cmo_triple_prepare_hs_env();

    for (size_t round = 0; round < AI_MEMBLOCK_CMO_TRIPLE_ROUNDS; ++round) {
        uint64_t block0_store_observed = 0ULL;
        uint64_t block0_zero_observed = 0ULL;
        uint64_t block1_observed = 0ULL;
        uint64_t block2_observed = 0ULL;
        const uint64_t expected_block1_load = block1_seeds[round] +
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
        AI_ASSERT_NO_EXCEPT_AND("before the three-block CMO plus younger scalar store probe starts, all source blocks, guards, and marker are readable",
            ld(block0_vaddr) == block0_seeds[round] &&
            ld(block0_store_vaddr) ==
                block0_seeds[round] +
                block0_store_word * 0x1111111111111111ULL &&
            ld(block1_vaddr) == block1_seeds[round] &&
            ld(block1_load_vaddr) == expected_block1_load &&
            ld(block2_vaddr) == block2_seeds[round] &&
            ld(block2_load_vaddr) ==
                block2_seeds[round] +
                block2_load_word * 0x1111111111111111ULL &&
            ld(marker_vaddr) == marker_seeds[round]
        );

        TEST_SETUP_EXCEPT();
        ai_memblock_cmo_triple_three_block_store_after_zero_window(
            block0_zero_vaddr, block1_inval_vaddr, block2_zero_vaddr,
            block0_store_vaddr, block0_store_values[round],
            block1_clean_vaddr, block2_flush_vaddr, block1_flush_vaddr,
            block0_zero_load_vaddr, block1_load_vaddr, block2_load_vaddr,
            marker_vaddr, marker_updates[round], &block0_store_observed,
            &block0_zero_observed, &block1_observed, &block2_observed);
        AI_ASSERT_NO_EXCEPT_AND("under interleaved three-block CMO pressure, a younger scalar store after CBO.ZERO remains visible and other owner results stay block-local",
            block0_store_observed == block0_store_values[round] &&
            block0_zero_observed == 0ULL &&
            block1_observed == expected_block1_load &&
            block2_observed == 0ULL
        );

        TEST_SETUP_EXCEPT();
        AI_ASSERT_NO_EXCEPT_AND("after the three-block CMO plus younger scalar store probe, block0 keeps the younger store, block1 preserves seed, block2 is zero, and guards remain intact",
            ld(block0_store_vaddr) == block0_store_values[round] &&
            ld(block0_zero_load_vaddr) == 0ULL &&
            ld(block1_vaddr) == block1_seeds[round] &&
            ld(block1_load_vaddr) == expected_block1_load &&
            ld(block2_vaddr) == 0ULL &&
            ld(block2_load_vaddr) == 0ULL &&
            ld(before_guard_vaddr) == before_guards[round] &&
            ld(guard01_vaddr) == guard01_values[round] &&
            ld(guard12_vaddr) == guard12_values[round] &&
            ld(after_guard_vaddr) == after_guards[round] &&
            ld(marker_vaddr) == marker_updates[round] &&
            ld(marker_guard_vaddr) == marker_guards[round]
        );

        goto_priv(PRIV_M);
        TEST_ASSERT("backing memory after the three-block CMO plus younger scalar store probe keeps block0 zero except the younger store, block1 seed payload, block2 zero, marker update, and all guards intact",
            ai_memblock_cmo_triple_block_zero_except_word(block0_paddr,
                block0_store_word, block0_store_values[round]) &&
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

bool ai_micro_hs_interleaved_three_block_cboinval_store_cbom_no_lost_payload_corner()
{
    TEST_START();

    const enum test_page source_page = SWITCH1;
    const enum test_page marker_page = SWITCH2;
    const size_t block0_store_word = 2U;
    const size_t block0_seed_load_word = 5U;
    const size_t block1_load_word = 4U;
    const size_t block2_load_word = 3U;
    const uintptr_t block0_vaddr = hs_page_base(source_page) + 0x1d00U;
    const uintptr_t block1_vaddr = hs_page_base(source_page) + 0x1d80U;
    const uintptr_t block2_vaddr = hs_page_base(source_page) + 0x1e00U;
    const uintptr_t block0_paddr = phys_page_base(source_page) + 0x1d00U;
    const uintptr_t block1_paddr = phys_page_base(source_page) + 0x1d80U;
    const uintptr_t block2_paddr = phys_page_base(source_page) + 0x1e00U;
    const uintptr_t block0_inval_vaddr =
        block0_vaddr + 1U * sizeof(uint64_t);
    const uintptr_t block1_zero_vaddr =
        block1_vaddr + 6U * sizeof(uint64_t);
    const uintptr_t block2_inval_vaddr =
        block2_vaddr + 2U * sizeof(uint64_t);
    const uintptr_t block0_store_vaddr =
        block0_vaddr + block0_store_word * sizeof(uint64_t);
    const uintptr_t block0_clean_vaddr =
        block0_vaddr + 4U * sizeof(uint64_t);
    const uintptr_t block1_flush_vaddr =
        block1_vaddr +
        (AI_MEMBLOCK_CMO_TRIPLE_BLOCK_WORDS - 1U) * sizeof(uint64_t);
    const uintptr_t block2_clean_vaddr =
        block2_vaddr + 5U * sizeof(uint64_t);
    const uintptr_t block0_flush_vaddr =
        block0_vaddr +
        (AI_MEMBLOCK_CMO_TRIPLE_BLOCK_WORDS - 1U) * sizeof(uint64_t);
    const uintptr_t block0_store_load_vaddr = block0_store_vaddr;
    const uintptr_t block0_seed_load_vaddr =
        block0_vaddr + block0_seed_load_word * sizeof(uint64_t);
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
    const uintptr_t marker_vaddr = hs_page_base(marker_page) + 0x1d00U;
    const uintptr_t marker_paddr = phys_page_base(marker_page) + 0x1d00U;
    const uintptr_t marker_guard_vaddr = marker_vaddr + sizeof(uint64_t);
    const uintptr_t marker_guard_paddr = marker_paddr + sizeof(uint64_t);
    const uint64_t block0_seeds[AI_MEMBLOCK_CMO_TRIPLE_ROUNDS] = {
        0x89abcdef01234567ULL,
        0x9abcdef012345678ULL,
        0xabcdef0123456789ULL,
    };
    const uint64_t block1_seeds[AI_MEMBLOCK_CMO_TRIPLE_ROUNDS] = {
        0x76543210fedcba98ULL,
        0x6543210fedcba987ULL,
        0x543210fedcba9876ULL,
    };
    const uint64_t block2_seeds[AI_MEMBLOCK_CMO_TRIPLE_ROUNDS] = {
        0x0badc0ffee123456ULL,
        0x1badc0ffee234567ULL,
        0x2badc0ffee345678ULL,
    };
    const uint64_t block0_store_values[AI_MEMBLOCK_CMO_TRIPLE_ROUNDS] = {
        0xf0e1d2c3b4a59687ULL,
        0xe1d2c3b4a5968778ULL,
        0xd2c3b4a596877869ULL,
    };
    const uint64_t before_guards[AI_MEMBLOCK_CMO_TRIPLE_ROUNDS] = {
        0x9091929394959697ULL,
        0xa0a1a2a3a4a5a6a7ULL,
        0xb0b1b2b3b4b5b6b7ULL,
    };
    const uint64_t guard01_values[AI_MEMBLOCK_CMO_TRIPLE_ROUNDS] = {
        0x0706050403020100ULL,
        0x1716151413121110ULL,
        0x2726252423222120ULL,
    };
    const uint64_t guard12_values[AI_MEMBLOCK_CMO_TRIPLE_ROUNDS] = {
        0x8081828384858687ULL,
        0x7071727374757677ULL,
        0x6061626364656667ULL,
    };
    const uint64_t after_guards[AI_MEMBLOCK_CMO_TRIPLE_ROUNDS] = {
        0xf7f6f5f4f3f2f1f0ULL,
        0xe7e6e5e4e3e2e1e0ULL,
        0xd7d6d5d4d3d2d1d0ULL,
    };
    const uint64_t marker_seeds[AI_MEMBLOCK_CMO_TRIPLE_ROUNDS] = {
        0x1111111122222222ULL,
        0x3333333344444444ULL,
        0x5555555566666666ULL,
    };
    const uint64_t marker_updates[AI_MEMBLOCK_CMO_TRIPLE_ROUNDS] = {
        0xaaaabbbbccccddddULL,
        0xbbbbccccddddeeeeULL,
        0xccccddddeeeeffffULL,
    };
    const uint64_t marker_guards[AI_MEMBLOCK_CMO_TRIPLE_ROUNDS] = {
        0x13579bdf2468ace0ULL,
        0x2468ace03579bdf1ULL,
        0x3579bdf1468ace02ULL,
    };

    ai_memblock_cmo_triple_prepare_hs_env();

    for (size_t round = 0; round < AI_MEMBLOCK_CMO_TRIPLE_ROUNDS; ++round) {
        uint64_t block0_store_observed = 0ULL;
        uint64_t block0_seed_observed = 0ULL;
        uint64_t block1_observed = 0ULL;
        uint64_t block2_observed = 0ULL;
        uint64_t marker_observed = 0ULL;
        const uint64_t expected_block0_seed_load = block0_seeds[round] +
            block0_seed_load_word * 0x1111111111111111ULL;
        const uint64_t expected_block2_load = block2_seeds[round] +
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
        AI_ASSERT_NO_EXCEPT_AND("before the three-block CBO.INVAL store CBOM probe starts, all source blocks, guards, and marker are readable",
            ld(block0_vaddr) == block0_seeds[round] &&
            ld(block0_store_vaddr) ==
                block0_seeds[round] +
                block0_store_word * 0x1111111111111111ULL &&
            ld(block0_seed_load_vaddr) == expected_block0_seed_load &&
            ld(block1_vaddr) == block1_seeds[round] &&
            ld(block2_vaddr) == block2_seeds[round] &&
            ld(block2_load_vaddr) == expected_block2_load &&
            ld(marker_vaddr) == marker_seeds[round]
        );

        TEST_SETUP_EXCEPT();
        ai_memblock_cmo_triple_three_block_store_after_inval_cbom_window(
            block0_inval_vaddr, block1_zero_vaddr, block2_inval_vaddr,
            block0_store_vaddr, block0_store_values[round],
            block0_clean_vaddr, block1_flush_vaddr, block2_clean_vaddr,
            block0_flush_vaddr, block0_store_load_vaddr,
            block0_seed_load_vaddr, block1_load_vaddr, block2_load_vaddr,
            marker_vaddr, marker_updates[round], &block0_store_observed,
            &block0_seed_observed, &block1_observed, &block2_observed,
            &marker_observed);
        AI_ASSERT_NO_EXCEPT_AND("under interleaved three-block CMO pressure, a younger scalar store after CBO.INVAL survives the following same-block CBOM clean/flush",
            block0_store_observed == block0_store_values[round] &&
            block0_seed_observed == expected_block0_seed_load &&
            block1_observed == 0ULL &&
            block2_observed == expected_block2_load &&
            marker_observed == marker_updates[round]
        );

        TEST_SETUP_EXCEPT();
        AI_ASSERT_NO_EXCEPT_AND("after the three-block CBO.INVAL store CBOM probe, block0 keeps the younger store with seeded neighbors, block1 is zero, block2 preserves seed, and guards remain intact",
            ld(block0_store_vaddr) == block0_store_values[round] &&
            ld(block0_seed_load_vaddr) == expected_block0_seed_load &&
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
        TEST_ASSERT("backing memory after the three-block CBO.INVAL store CBOM probe keeps block0 seed except the younger store, block1 zero, block2 seed payload, marker update, and all guards intact",
            ai_memblock_cmo_triple_block_matches_seed_except_word(block0_paddr,
                block0_seeds[round], block0_store_word,
                block0_store_values[round]) &&
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

bool ai_micro_hs_interleaved_three_block_store_inval_store_cbom_no_stale_middle_owner_corner()
{
    TEST_START();

    const enum test_page source_page = SWITCH1;
    const enum test_page marker_page = SWITCH2;
    const size_t block0_older_store_word = 1U;
    const size_t block0_younger_store_word = 5U;
    const size_t block0_seed_load_word = 6U;
    const size_t block1_load_word = 2U;
    const size_t block2_load_word = 3U;
    const uintptr_t block0_vaddr = hs_page_base(source_page) + 0x2300U;
    const uintptr_t block1_vaddr = hs_page_base(source_page) + 0x2380U;
    const uintptr_t block2_vaddr = hs_page_base(source_page) + 0x2400U;
    const uintptr_t block0_paddr = phys_page_base(source_page) + 0x2300U;
    const uintptr_t block1_paddr = phys_page_base(source_page) + 0x2380U;
    const uintptr_t block2_paddr = phys_page_base(source_page) + 0x2400U;
    const uintptr_t block0_older_store_vaddr =
        block0_vaddr + block0_older_store_word * sizeof(uint64_t);
    const uintptr_t block0_inval_vaddr =
        block0_vaddr + 3U * sizeof(uint64_t);
    const uintptr_t block1_zero_vaddr =
        block1_vaddr + 6U * sizeof(uint64_t);
    const uintptr_t block2_inval_vaddr =
        block2_vaddr + 2U * sizeof(uint64_t);
    const uintptr_t block0_younger_store_vaddr =
        block0_vaddr + block0_younger_store_word * sizeof(uint64_t);
    const uintptr_t block0_clean_vaddr =
        block0_vaddr + 2U * sizeof(uint64_t);
    const uintptr_t block1_flush_vaddr =
        block1_vaddr +
        (AI_MEMBLOCK_CMO_TRIPLE_BLOCK_WORDS - 1U) * sizeof(uint64_t);
    const uintptr_t block2_clean_vaddr =
        block2_vaddr + 5U * sizeof(uint64_t);
    const uintptr_t block0_flush_vaddr =
        block0_vaddr +
        (AI_MEMBLOCK_CMO_TRIPLE_BLOCK_WORDS - 1U) * sizeof(uint64_t);
    const uintptr_t block2_flush_vaddr =
        block2_vaddr +
        (AI_MEMBLOCK_CMO_TRIPLE_BLOCK_WORDS - 1U) * sizeof(uint64_t);
    const uintptr_t block0_older_load_vaddr = block0_older_store_vaddr;
    const uintptr_t block0_younger_load_vaddr = block0_younger_store_vaddr;
    const uintptr_t block0_seed_load_vaddr =
        block0_vaddr + block0_seed_load_word * sizeof(uint64_t);
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
    const uintptr_t marker_vaddr = hs_page_base(marker_page) + 0x2300U;
    const uintptr_t marker_paddr = phys_page_base(marker_page) + 0x2300U;
    const uintptr_t marker_guard_vaddr = marker_vaddr + sizeof(uint64_t);
    const uintptr_t marker_guard_paddr = marker_paddr + sizeof(uint64_t);
    const uint64_t block0_seeds[AI_MEMBLOCK_CMO_TRIPLE_ROUNDS] = {
        0x3141592653589793ULL,
        0x42526a376469a8a4ULL,
        0x53637b48757ab9b5ULL,
    };
    const uint64_t block1_seeds[AI_MEMBLOCK_CMO_TRIPLE_ROUNDS] = {
        0x2718281828459045ULL,
        0x382939293956a156ULL,
        0x493a4a4a4a67b267ULL,
    };
    const uint64_t block2_seeds[AI_MEMBLOCK_CMO_TRIPLE_ROUNDS] = {
        0x0ddc0ffeec0ffee0ULL,
        0x1eed100ffd10fff1ULL,
        0x2ffe21100e211002ULL,
    };
    const uint64_t block0_older_store_values[AI_MEMBLOCK_CMO_TRIPLE_ROUNDS] = {
        0xaaa111bbb222ccc3ULL,
        0xbbb222ccc333ddd4ULL,
        0xccc333ddd444eee5ULL,
    };
    const uint64_t block0_younger_store_values[AI_MEMBLOCK_CMO_TRIPLE_ROUNDS] = {
        0x111aaa222bbb333cULL,
        0x222bbb333ccc444dULL,
        0x333ccc444ddd555eULL,
    };
    const uint64_t before_guards[AI_MEMBLOCK_CMO_TRIPLE_ROUNDS] = {
        0x0100feed0200beefULL,
        0x1211feed1311beefULL,
        0x2322feed2422beefULL,
    };
    const uint64_t guard01_values[AI_MEMBLOCK_CMO_TRIPLE_ROUNDS] = {
        0xdead0101beef0202ULL,
        0xdead1212beef1313ULL,
        0xdead2323beef2424ULL,
    };
    const uint64_t guard12_values[AI_MEMBLOCK_CMO_TRIPLE_ROUNDS] = {
        0xc0010101d00d0202ULL,
        0xc0011212d00d1313ULL,
        0xc0012323d00d2424ULL,
    };
    const uint64_t after_guards[AI_MEMBLOCK_CMO_TRIPLE_ROUNDS] = {
        0xf0f00101e1e10202ULL,
        0xf0f01212e1e11313ULL,
        0xf0f02323e1e12424ULL,
    };
    const uint64_t marker_seeds[AI_MEMBLOCK_CMO_TRIPLE_ROUNDS] = {
        0x1020304050607080ULL,
        0x2131415161718191ULL,
        0x32425262728292a2ULL,
    };
    const uint64_t marker_updates[AI_MEMBLOCK_CMO_TRIPLE_ROUNDS] = {
        0x80f0e0d0c0b0a090ULL,
        0x91e1d1c1b1a19181ULL,
        0xa2d2c2b2a2928272ULL,
    };
    const uint64_t marker_guards[AI_MEMBLOCK_CMO_TRIPLE_ROUNDS] = {
        0x5a5a01015a5a0202ULL,
        0x6b6b12126b6b1313ULL,
        0x7c7c23237c7c2424ULL,
    };

    ai_memblock_cmo_triple_prepare_hs_env();

    for (size_t round = 0; round < AI_MEMBLOCK_CMO_TRIPLE_ROUNDS; ++round) {
        uint64_t block0_older_observed = 0ULL;
        uint64_t block0_younger_observed = 0ULL;
        uint64_t block0_seed_observed = 0ULL;
        uint64_t block1_observed = 0ULL;
        uint64_t block2_observed = 0ULL;
        uint64_t marker_observed = 0ULL;
        const uint64_t expected_block0_older_before =
            block0_seeds[round] +
            block0_older_store_word * 0x1111111111111111ULL;
        const uint64_t expected_block0_younger_before =
            block0_seeds[round] +
            block0_younger_store_word * 0x1111111111111111ULL;
        const uint64_t expected_block0_seed_load =
            block0_seeds[round] +
            block0_seed_load_word * 0x1111111111111111ULL;
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
        AI_ASSERT_NO_EXCEPT_AND("before the store CBO.INVAL store CBOM probe starts, all source blocks, guards, and marker are readable",
            ld(block0_vaddr) == block0_seeds[round] &&
            ld(block0_older_store_vaddr) == expected_block0_older_before &&
            ld(block0_younger_store_vaddr) == expected_block0_younger_before &&
            ld(block0_seed_load_vaddr) == expected_block0_seed_load &&
            ld(block1_vaddr) == block1_seeds[round] &&
            ld(block2_vaddr) == block2_seeds[round] &&
            ld(block2_load_vaddr) == expected_block2_load &&
            ld(marker_vaddr) == marker_seeds[round]
        );

        TEST_SETUP_EXCEPT();
        ai_memblock_cmo_triple_three_block_store_inval_store_cbom_window(
            block0_older_store_vaddr,
            block0_older_store_values[round], block0_inval_vaddr,
            block1_zero_vaddr, block2_inval_vaddr,
            block0_younger_store_vaddr,
            block0_younger_store_values[round], block0_clean_vaddr,
            block1_flush_vaddr, block2_clean_vaddr, block0_flush_vaddr,
            block2_flush_vaddr, block0_older_load_vaddr,
            block0_younger_load_vaddr, block0_seed_load_vaddr,
            block1_load_vaddr, block2_load_vaddr, marker_vaddr,
            marker_updates[round], &block0_older_observed,
            &block0_younger_observed, &block0_seed_observed,
            &block1_observed, &block2_observed, &marker_observed);
        AI_ASSERT_NO_EXCEPT_AND("under interleaved three-block CMO pressure, same-block older and younger scalar stores around CBO.INVAL stay visible through the following CBOM",
            block0_older_observed == block0_older_store_values[round] &&
            block0_younger_observed == block0_younger_store_values[round] &&
            block0_seed_observed == expected_block0_seed_load &&
            block1_observed == 0ULL &&
            block2_observed == expected_block2_load &&
            marker_observed == marker_updates[round]
        );

        TEST_SETUP_EXCEPT();
        AI_ASSERT_NO_EXCEPT_AND("after the store CBO.INVAL store CBOM probe, block0 keeps both store generations, block1 is zero, block2 preserves seed, and guards remain intact",
            ld(block0_older_store_vaddr) ==
                block0_older_store_values[round] &&
            ld(block0_younger_store_vaddr) ==
                block0_younger_store_values[round] &&
            ld(block0_seed_load_vaddr) == expected_block0_seed_load &&
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
        TEST_ASSERT("backing memory after the store CBO.INVAL store CBOM probe keeps block0 seed except both store generations, block1 zero, block2 seed payload, marker update, and all guards intact",
            ai_memblock_cmo_triple_block_matches_seed_except_two_words(
                block0_paddr, block0_seeds[round],
                block0_older_store_word,
                block0_older_store_values[round],
                block0_younger_store_word,
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

bool ai_micro_hs_interleaved_three_block_cmo_dual_younger_store_no_cross_payload_corner()
{
    TEST_START();

    const enum test_page source_page = SWITCH1;
    const enum test_page marker_page = SWITCH2;
    const size_t block0_store_word = 2U;
    const size_t block0_zero_load_word = 5U;
    const size_t block1_load_word = 3U;
    const size_t block2_store_word = 4U;
    const size_t block2_seed_load_word = 6U;
    const uintptr_t block0_vaddr = hs_page_base(source_page) + 0x1f00U;
    const uintptr_t block1_vaddr = hs_page_base(source_page) + 0x1f80U;
    const uintptr_t block2_vaddr = hs_page_base(source_page) + 0x2000U;
    const uintptr_t block0_paddr = phys_page_base(source_page) + 0x1f00U;
    const uintptr_t block1_paddr = phys_page_base(source_page) + 0x1f80U;
    const uintptr_t block2_paddr = phys_page_base(source_page) + 0x2000U;
    const uintptr_t block0_zero_vaddr =
        block0_vaddr + 6U * sizeof(uint64_t);
    const uintptr_t block1_inval_vaddr =
        block1_vaddr + 1U * sizeof(uint64_t);
    const uintptr_t block2_inval_vaddr =
        block2_vaddr + 2U * sizeof(uint64_t);
    const uintptr_t block0_store_vaddr =
        block0_vaddr + block0_store_word * sizeof(uint64_t);
    const uintptr_t block2_store_vaddr =
        block2_vaddr + block2_store_word * sizeof(uint64_t);
    const uintptr_t block1_clean_vaddr =
        block1_vaddr + 5U * sizeof(uint64_t);
    const uintptr_t block0_flush_vaddr =
        block0_vaddr +
        (AI_MEMBLOCK_CMO_TRIPLE_BLOCK_WORDS - 1U) * sizeof(uint64_t);
    const uintptr_t block2_clean_vaddr =
        block2_vaddr + 3U * sizeof(uint64_t);
    const uintptr_t block1_flush_vaddr =
        block1_vaddr +
        (AI_MEMBLOCK_CMO_TRIPLE_BLOCK_WORDS - 1U) * sizeof(uint64_t);
    const uintptr_t block0_store_load_vaddr = block0_store_vaddr;
    const uintptr_t block0_zero_load_vaddr =
        block0_vaddr + block0_zero_load_word * sizeof(uint64_t);
    const uintptr_t block1_load_vaddr =
        block1_vaddr + block1_load_word * sizeof(uint64_t);
    const uintptr_t block2_store_load_vaddr = block2_store_vaddr;
    const uintptr_t block2_seed_load_vaddr =
        block2_vaddr + block2_seed_load_word * sizeof(uint64_t);
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
    const uintptr_t marker_vaddr = hs_page_base(marker_page) + 0x1f00U;
    const uintptr_t marker_paddr = phys_page_base(marker_page) + 0x1f00U;
    const uintptr_t marker_guard_vaddr = marker_vaddr + sizeof(uint64_t);
    const uintptr_t marker_guard_paddr = marker_paddr + sizeof(uint64_t);
    const uint64_t block0_seeds[AI_MEMBLOCK_CMO_TRIPLE_ROUNDS] = {
        0x0102030405060708ULL,
        0x1112131415161718ULL,
        0x2122232425262728ULL,
    };
    const uint64_t block1_seeds[AI_MEMBLOCK_CMO_TRIPLE_ROUNDS] = {
        0x8877665544332211ULL,
        0x7766554433221100ULL,
        0x66554433221100ffULL,
    };
    const uint64_t block2_seeds[AI_MEMBLOCK_CMO_TRIPLE_ROUNDS] = {
        0x123456789abcdef0ULL,
        0x23456789abcdef01ULL,
        0x3456789abcdef012ULL,
    };
    const uint64_t block0_store_values[AI_MEMBLOCK_CMO_TRIPLE_ROUNDS] = {
        0xfedcba9876543210ULL,
        0xedcba9876543210fULL,
        0xdcba9876543210feULL,
    };
    const uint64_t block2_store_values[AI_MEMBLOCK_CMO_TRIPLE_ROUNDS] = {
        0x0f1e2d3c4b5a6978ULL,
        0x1e2d3c4b5a697887ULL,
        0x2d3c4b5a69788796ULL,
    };
    const uint64_t before_guards[AI_MEMBLOCK_CMO_TRIPLE_ROUNDS] = {
        0xaabbccdd00112233ULL,
        0xbbccddee11223344ULL,
        0xccddeeff22334455ULL,
    };
    const uint64_t guard01_values[AI_MEMBLOCK_CMO_TRIPLE_ROUNDS] = {
        0x3344112255667788ULL,
        0x4455223366778899ULL,
        0x55663344778899aaULL,
    };
    const uint64_t guard12_values[AI_MEMBLOCK_CMO_TRIPLE_ROUNDS] = {
        0x8877556633221100ULL,
        0x9988667744332211ULL,
        0xaa99778855443322ULL,
    };
    const uint64_t after_guards[AI_MEMBLOCK_CMO_TRIPLE_ROUNDS] = {
        0x00ffeeddccbbaa99ULL,
        0x11ffeeddccbbaa88ULL,
        0x22ffeeddccbbaa77ULL,
    };
    const uint64_t marker_seeds[AI_MEMBLOCK_CMO_TRIPLE_ROUNDS] = {
        0x0c0d0e0f01020304ULL,
        0x1c1d1e1f11121314ULL,
        0x2c2d2e2f21222324ULL,
    };
    const uint64_t marker_updates[AI_MEMBLOCK_CMO_TRIPLE_ROUNDS] = {
        0x5a5aa5a55a5aa5a5ULL,
        0x6b6bb6b66b6bb6b6ULL,
        0x7c7cc7c77c7cc7c7ULL,
    };
    const uint64_t marker_guards[AI_MEMBLOCK_CMO_TRIPLE_ROUNDS] = {
        0xcafebabedeadbeefULL,
        0xdeadbeefcafebabeULL,
        0xbeefdeadcafef00dULL,
    };

    ai_memblock_cmo_triple_prepare_hs_env();

    for (size_t round = 0; round < AI_MEMBLOCK_CMO_TRIPLE_ROUNDS; ++round) {
        uint64_t block0_store_observed = 0ULL;
        uint64_t block0_zero_observed = 0ULL;
        uint64_t block1_observed = 0ULL;
        uint64_t block2_store_observed = 0ULL;
        uint64_t block2_seed_observed = 0ULL;
        uint64_t marker_observed = 0ULL;
        const uint64_t expected_block1_load = block1_seeds[round] +
            block1_load_word * 0x1111111111111111ULL;
        const uint64_t expected_block2_seed_load = block2_seeds[round] +
            block2_seed_load_word * 0x1111111111111111ULL;

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
        AI_ASSERT_NO_EXCEPT_AND("before the three-block dual younger store CMO probe starts, all source blocks, guards, and marker are readable",
            ld(block0_vaddr) == block0_seeds[round] &&
            ld(block0_store_vaddr) ==
                block0_seeds[round] +
                block0_store_word * 0x1111111111111111ULL &&
            ld(block1_vaddr) == block1_seeds[round] &&
            ld(block1_load_vaddr) == expected_block1_load &&
            ld(block2_vaddr) == block2_seeds[round] &&
            ld(block2_store_vaddr) ==
                block2_seeds[round] +
                block2_store_word * 0x1111111111111111ULL &&
            ld(block2_seed_load_vaddr) == expected_block2_seed_load &&
            ld(marker_vaddr) == marker_seeds[round]
        );

        TEST_SETUP_EXCEPT();
        ai_memblock_cmo_triple_three_block_dual_store_window(
            block0_zero_vaddr, block1_inval_vaddr, block2_inval_vaddr,
            block0_store_vaddr, block0_store_values[round],
            block2_store_vaddr, block2_store_values[round],
            block1_clean_vaddr, block0_flush_vaddr, block2_clean_vaddr,
            block1_flush_vaddr, block0_store_load_vaddr,
            block0_zero_load_vaddr, block1_load_vaddr,
            block2_store_load_vaddr, block2_seed_load_vaddr, marker_vaddr,
            marker_updates[round], &block0_store_observed,
            &block0_zero_observed, &block1_observed,
            &block2_store_observed, &block2_seed_observed,
            &marker_observed);
        AI_ASSERT_NO_EXCEPT_AND("under interleaved three-block CMO pressure, the two younger scalar stores stay bound to their own cache blocks and marker progresses",
            block0_store_observed == block0_store_values[round] &&
            block0_zero_observed == 0ULL &&
            block1_observed == expected_block1_load &&
            block2_store_observed == block2_store_values[round] &&
            block2_seed_observed == expected_block2_seed_load &&
            marker_observed == marker_updates[round]
        );

        TEST_SETUP_EXCEPT();
        AI_ASSERT_NO_EXCEPT_AND("after the dual younger store CMO probe, block0 keeps zero plus store, block1 preserves seed, block2 keeps seed plus store, and guards remain intact",
            ld(block0_store_vaddr) == block0_store_values[round] &&
            ld(block0_zero_load_vaddr) == 0ULL &&
            ld(block1_vaddr) == block1_seeds[round] &&
            ld(block1_load_vaddr) == expected_block1_load &&
            ld(block2_store_vaddr) == block2_store_values[round] &&
            ld(block2_seed_load_vaddr) == expected_block2_seed_load &&
            ld(before_guard_vaddr) == before_guards[round] &&
            ld(guard01_vaddr) == guard01_values[round] &&
            ld(guard12_vaddr) == guard12_values[round] &&
            ld(after_guard_vaddr) == after_guards[round] &&
            ld(marker_guard_vaddr) == marker_guards[round]
        );

        goto_priv(PRIV_M);
        TEST_ASSERT("backing memory after the dual younger store CMO probe keeps block0 zero except its younger store, block1 seed, block2 seed except its younger store, marker update, and all guards intact",
            ai_memblock_cmo_triple_block_zero_except_word(block0_paddr,
                block0_store_word, block0_store_values[round]) &&
            ai_memblock_cmo_triple_block_matches_seed(block1_paddr,
                block1_seeds[round]) &&
            ai_memblock_cmo_triple_block_matches_seed_except_word(block2_paddr,
                block2_seeds[round], block2_store_word,
                block2_store_values[round]) &&
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

bool ai_micro_hs_interleaved_three_block_older_stores_then_cmo_no_cross_owner_corner()
{
    TEST_START();

    const enum test_page source_page = SWITCH1;
    const enum test_page marker_page = SWITCH2;
    const size_t block0_store_word = 2U;
    const size_t block0_zero_load_word = 5U;
    const size_t block1_store_word = 3U;
    const size_t block1_seed_load_word = 6U;
    const size_t block2_store_word = 4U;
    const size_t block2_seed_load_word = 1U;
    const uintptr_t block0_vaddr = hs_page_base(source_page) + 0x2100U;
    const uintptr_t block1_vaddr = hs_page_base(source_page) + 0x2180U;
    const uintptr_t block2_vaddr = hs_page_base(source_page) + 0x2200U;
    const uintptr_t block0_paddr = phys_page_base(source_page) + 0x2100U;
    const uintptr_t block1_paddr = phys_page_base(source_page) + 0x2180U;
    const uintptr_t block2_paddr = phys_page_base(source_page) + 0x2200U;
    const uintptr_t block0_store_vaddr =
        block0_vaddr + block0_store_word * sizeof(uint64_t);
    const uintptr_t block1_store_vaddr =
        block1_vaddr + block1_store_word * sizeof(uint64_t);
    const uintptr_t block2_store_vaddr =
        block2_vaddr + block2_store_word * sizeof(uint64_t);
    const uintptr_t block0_zero_vaddr =
        block0_vaddr + 6U * sizeof(uint64_t);
    const uintptr_t block1_inval_vaddr =
        block1_vaddr + 2U * sizeof(uint64_t);
    const uintptr_t block2_clean_vaddr =
        block2_vaddr + 3U * sizeof(uint64_t);
    const uintptr_t block0_flush_vaddr =
        block0_vaddr +
        (AI_MEMBLOCK_CMO_TRIPLE_BLOCK_WORDS - 1U) * sizeof(uint64_t);
    const uintptr_t block1_clean_vaddr =
        block1_vaddr + 5U * sizeof(uint64_t);
    const uintptr_t block2_flush_vaddr =
        block2_vaddr +
        (AI_MEMBLOCK_CMO_TRIPLE_BLOCK_WORDS - 1U) * sizeof(uint64_t);
    const uintptr_t block0_store_load_vaddr = block0_store_vaddr;
    const uintptr_t block0_zero_load_vaddr =
        block0_vaddr + block0_zero_load_word * sizeof(uint64_t);
    const uintptr_t block1_store_load_vaddr = block1_store_vaddr;
    const uintptr_t block1_seed_load_vaddr =
        block1_vaddr + block1_seed_load_word * sizeof(uint64_t);
    const uintptr_t block2_store_load_vaddr = block2_store_vaddr;
    const uintptr_t block2_seed_load_vaddr =
        block2_vaddr + block2_seed_load_word * sizeof(uint64_t);
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
    const uintptr_t marker_vaddr = hs_page_base(marker_page) + 0x2100U;
    const uintptr_t marker_paddr = phys_page_base(marker_page) + 0x2100U;
    const uintptr_t marker_guard_vaddr = marker_vaddr + sizeof(uint64_t);
    const uintptr_t marker_guard_paddr = marker_paddr + sizeof(uint64_t);
    const uint64_t block0_seeds[AI_MEMBLOCK_CMO_TRIPLE_ROUNDS] = {
        0x0a1b2c3d4e5f6071ULL,
        0x1b2c3d4e5f607182ULL,
        0x2c3d4e5f60718293ULL,
    };
    const uint64_t block1_seeds[AI_MEMBLOCK_CMO_TRIPLE_ROUNDS] = {
        0x8091a2b3c4d5e6f7ULL,
        0x91a2b3c4d5e6f708ULL,
        0xa2b3c4d5e6f70819ULL,
    };
    const uint64_t block2_seeds[AI_MEMBLOCK_CMO_TRIPLE_ROUNDS] = {
        0x1234432112344321ULL,
        0x2345543223455432ULL,
        0x3456654334566543ULL,
    };
    const uint64_t block0_store_values[AI_MEMBLOCK_CMO_TRIPLE_ROUNDS] = {
        0xff00ee11dd22cc33ULL,
        0xee11dd22cc33bb44ULL,
        0xdd22cc33bb44aa55ULL,
    };
    const uint64_t block1_store_values[AI_MEMBLOCK_CMO_TRIPLE_ROUNDS] = {
        0x1020304050607080ULL,
        0x2131415161718191ULL,
        0x32425262728292a2ULL,
    };
    const uint64_t block2_store_values[AI_MEMBLOCK_CMO_TRIPLE_ROUNDS] = {
        0xf0f1f2f3f4f5f6f7ULL,
        0xe0e1e2e3e4e5e6e7ULL,
        0xd0d1d2d3d4d5d6d7ULL,
    };
    const uint64_t before_guards[AI_MEMBLOCK_CMO_TRIPLE_ROUNDS] = {
        0x1122112233443344ULL,
        0x2233223344554455ULL,
        0x3344334455665566ULL,
    };
    const uint64_t guard01_values[AI_MEMBLOCK_CMO_TRIPLE_ROUNDS] = {
        0x7788778899aa99aaULL,
        0x88998899aabbaabbULL,
        0x99aa99aabbccbcccULL,
    };
    const uint64_t guard12_values[AI_MEMBLOCK_CMO_TRIPLE_ROUNDS] = {
        0xccbbccbb88778877ULL,
        0xddccddcc99889988ULL,
        0xeeddeeddaa99aa99ULL,
    };
    const uint64_t after_guards[AI_MEMBLOCK_CMO_TRIPLE_ROUNDS] = {
        0x01010101fefefefeULL,
        0x12121212ededededULL,
        0x23232323dcdcdcdcULL,
    };
    const uint64_t marker_seeds[AI_MEMBLOCK_CMO_TRIPLE_ROUNDS] = {
        0x13572468ace0bdf9ULL,
        0x24683579bdf1ce0aULL,
        0x3579468ace02df1bULL,
    };
    const uint64_t marker_updates[AI_MEMBLOCK_CMO_TRIPLE_ROUNDS] = {
        0x97531fdb86420ecaULL,
        0x86420eca7531fdb9ULL,
        0x7531fdb96420eca8ULL,
    };
    const uint64_t marker_guards[AI_MEMBLOCK_CMO_TRIPLE_ROUNDS] = {
        0xabcdef0123456789ULL,
        0xbcdef0123456789aULL,
        0xcdef0123456789abULL,
    };

    ai_memblock_cmo_triple_prepare_hs_env();

    for (size_t round = 0; round < AI_MEMBLOCK_CMO_TRIPLE_ROUNDS; ++round) {
        uint64_t block0_store_observed = 0ULL;
        uint64_t block0_zero_observed = 0ULL;
        uint64_t block1_store_observed = 0ULL;
        uint64_t block1_seed_observed = 0ULL;
        uint64_t block2_store_observed = 0ULL;
        uint64_t block2_seed_observed = 0ULL;
        uint64_t marker_observed = 0ULL;
        const uint64_t expected_block1_seed_load = block1_seeds[round] +
            block1_seed_load_word * 0x1111111111111111ULL;
        const uint64_t expected_block2_seed_load = block2_seeds[round] +
            block2_seed_load_word * 0x1111111111111111ULL;

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
        AI_ASSERT_NO_EXCEPT_AND("before the older-store then three-block CMO probe starts, all source blocks, guards, and marker are readable",
            ld(block0_vaddr) == block0_seeds[round] &&
            ld(block1_vaddr) == block1_seeds[round] &&
            ld(block1_seed_load_vaddr) == expected_block1_seed_load &&
            ld(block2_vaddr) == block2_seeds[round] &&
            ld(block2_seed_load_vaddr) == expected_block2_seed_load &&
            ld(marker_vaddr) == marker_seeds[round]
        );

        TEST_SETUP_EXCEPT();
        ai_memblock_cmo_triple_three_block_older_store_then_cmo_window(
            block0_store_vaddr, block0_store_values[round],
            block1_store_vaddr, block1_store_values[round],
            block2_store_vaddr, block2_store_values[round],
            block0_zero_vaddr, block1_inval_vaddr, block2_clean_vaddr,
            block0_flush_vaddr, block1_clean_vaddr, block2_flush_vaddr,
            block0_store_load_vaddr, block0_zero_load_vaddr,
            block1_store_load_vaddr, block1_seed_load_vaddr,
            block2_store_load_vaddr, block2_seed_load_vaddr, marker_vaddr,
            marker_updates[round], &block0_store_observed,
            &block0_zero_observed, &block1_store_observed,
            &block1_seed_observed, &block2_store_observed,
            &block2_seed_observed, &marker_observed);
        AI_ASSERT_NO_EXCEPT_AND("interleaved CMO after older stores zeroes only the CBO.ZERO block while preserving stores on CBO.INVAL and CBOM blocks",
            block0_store_observed == 0ULL &&
            block0_zero_observed == 0ULL &&
            block1_store_observed == block1_store_values[round] &&
            block1_seed_observed == expected_block1_seed_load &&
            block2_store_observed == block2_store_values[round] &&
            block2_seed_observed == expected_block2_seed_load &&
            marker_observed == marker_updates[round]
        );

        TEST_SETUP_EXCEPT();
        AI_ASSERT_NO_EXCEPT_AND("after older stores are followed by interleaved CMO, block0 is zero, block1 and block2 keep their stores and seed neighbors, and guards remain intact",
            ld(block0_store_vaddr) == 0ULL &&
            ld(block0_zero_load_vaddr) == 0ULL &&
            ld(block1_store_vaddr) == block1_store_values[round] &&
            ld(block1_seed_load_vaddr) == expected_block1_seed_load &&
            ld(block2_store_vaddr) == block2_store_values[round] &&
            ld(block2_seed_load_vaddr) == expected_block2_seed_load &&
            ld(before_guard_vaddr) == before_guards[round] &&
            ld(guard01_vaddr) == guard01_values[round] &&
            ld(guard12_vaddr) == guard12_values[round] &&
            ld(after_guard_vaddr) == after_guards[round] &&
            ld(marker_guard_vaddr) == marker_guards[round]
        );

        goto_priv(PRIV_M);
        TEST_ASSERT("backing memory after older stores are followed by interleaved CMO keeps block0 zero, block1 and block2 store updates, marker update, and all guards intact",
            ai_memblock_cmo_triple_block_all_zero(block0_paddr) &&
            ai_memblock_cmo_triple_block_matches_seed_except_word(block1_paddr,
                block1_seeds[round], block1_store_word,
                block1_store_values[round]) &&
            ai_memblock_cmo_triple_block_matches_seed_except_word(block2_paddr,
                block2_seeds[round], block2_store_word,
                block2_store_values[round]) &&
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

bool ai_micro_hs_interleaved_three_block_store_zero_store_cbom_no_stale_zero_owner_corner()
{
    TEST_START();

    const enum test_page source_page = SWITCH1;
    const enum test_page marker_page = SWITCH2;
    const size_t block0_older_store_word = 1U;
    const size_t block0_younger_store_word = 5U;
    const size_t block0_zero_load_word = 6U;
    const size_t block1_load_word = 3U;
    const size_t block2_load_word = 4U;
    const uintptr_t block0_vaddr = hs_page_base(source_page) + 0x2500U;
    const uintptr_t block1_vaddr = hs_page_base(source_page) + 0x2580U;
    const uintptr_t block2_vaddr = hs_page_base(source_page) + 0x2600U;
    const uintptr_t block0_paddr = phys_page_base(source_page) + 0x2500U;
    const uintptr_t block1_paddr = phys_page_base(source_page) + 0x2580U;
    const uintptr_t block2_paddr = phys_page_base(source_page) + 0x2600U;
    const uintptr_t block0_older_store_vaddr =
        block0_vaddr + block0_older_store_word * sizeof(uint64_t);
    const uintptr_t block0_zero_vaddr =
        block0_vaddr + 3U * sizeof(uint64_t);
    const uintptr_t block1_inval_vaddr =
        block1_vaddr + 2U * sizeof(uint64_t);
    const uintptr_t block2_inval_vaddr =
        block2_vaddr + 1U * sizeof(uint64_t);
    const uintptr_t block0_younger_store_vaddr =
        block0_vaddr + block0_younger_store_word * sizeof(uint64_t);
    const uintptr_t block0_clean_vaddr =
        block0_vaddr + 2U * sizeof(uint64_t);
    const uintptr_t block1_clean_vaddr =
        block1_vaddr + 5U * sizeof(uint64_t);
    const uintptr_t block2_clean_vaddr =
        block2_vaddr + 6U * sizeof(uint64_t);
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
    const uintptr_t marker_vaddr = hs_page_base(marker_page) + 0x2500U;
    const uintptr_t marker_paddr = phys_page_base(marker_page) + 0x2500U;
    const uintptr_t marker_guard_vaddr = marker_vaddr + sizeof(uint64_t);
    const uintptr_t marker_guard_paddr = marker_paddr + sizeof(uint64_t);
    const uint64_t block0_seeds[AI_MEMBLOCK_CMO_TRIPLE_ROUNDS] = {
        0x6180339887498948ULL,
        0x729144a9985a9a59ULL,
        0x83a255baa96bab6aULL,
    };
    const uint64_t block1_seeds[AI_MEMBLOCK_CMO_TRIPLE_ROUNDS] = {
        0x1414213562373095ULL,
        0x25253246734841a6ULL,
        0x36364357845952b7ULL,
    };
    const uint64_t block2_seeds[AI_MEMBLOCK_CMO_TRIPLE_ROUNDS] = {
        0x1732050807568877ULL,
        0x2843161918679988ULL,
        0x3954272a2978aa99ULL,
    };
    const uint64_t block0_older_store_values[AI_MEMBLOCK_CMO_TRIPLE_ROUNDS] = {
        0x0a0b0c0d0e0f1011ULL,
        0x1b1c1d1e1f202122ULL,
        0x2c2d2e2f30313233ULL,
    };
    const uint64_t block0_younger_store_values[AI_MEMBLOCK_CMO_TRIPLE_ROUNDS] = {
        0xf1e2d3c4b5a69788ULL,
        0xe2d3c4b5a6978879ULL,
        0xd3c4b5a69788796aULL,
    };
    const uint64_t before_guards[AI_MEMBLOCK_CMO_TRIPLE_ROUNDS] = {
        0x11110000aaaabbbbULL,
        0x22221111bbbbccccULL,
        0x33332222ccccddddULL,
    };
    const uint64_t guard01_values[AI_MEMBLOCK_CMO_TRIPLE_ROUNDS] = {
        0x44443333ddddeeeeULL,
        0x55554444eeeeffffULL,
        0x66665555ffff0000ULL,
    };
    const uint64_t guard12_values[AI_MEMBLOCK_CMO_TRIPLE_ROUNDS] = {
        0x7777666600001111ULL,
        0x8888777711112222ULL,
        0x9999888822223333ULL,
    };
    const uint64_t after_guards[AI_MEMBLOCK_CMO_TRIPLE_ROUNDS] = {
        0xaaa9999833334444ULL,
        0xbbbaaa9944445555ULL,
        0xcccbbbaa55556666ULL,
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
        0x55aa55aa11ee11eeULL,
        0x66bb66bb22ff22ffULL,
        0x77cc77cc33003300ULL,
    };

    ai_memblock_cmo_triple_prepare_hs_env();

    for (size_t round = 0; round < AI_MEMBLOCK_CMO_TRIPLE_ROUNDS; ++round) {
        uint64_t block0_older_observed = 0ULL;
        uint64_t block0_younger_observed = 0ULL;
        uint64_t block0_zero_observed = 0ULL;
        uint64_t block1_observed = 0ULL;
        uint64_t block2_observed = 0ULL;
        uint64_t marker_observed = 0ULL;
        const uint64_t expected_block0_older_before =
            block0_seeds[round] +
            block0_older_store_word * 0x1111111111111111ULL;
        const uint64_t expected_block0_younger_before =
            block0_seeds[round] +
            block0_younger_store_word * 0x1111111111111111ULL;
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
        AI_ASSERT_NO_EXCEPT_AND("before the store CBO.ZERO store CBOM probe starts, all source blocks, guards, and marker are readable",
            ld(block0_vaddr) == block0_seeds[round] &&
            ld(block0_older_store_vaddr) == expected_block0_older_before &&
            ld(block0_younger_store_vaddr) == expected_block0_younger_before &&
            ld(block1_vaddr) == block1_seeds[round] &&
            ld(block1_load_vaddr) == expected_block1_load &&
            ld(block2_vaddr) == block2_seeds[round] &&
            ld(block2_load_vaddr) == expected_block2_load &&
            ld(marker_vaddr) == marker_seeds[round]
        );

        TEST_SETUP_EXCEPT();
        ai_memblock_cmo_triple_three_block_store_zero_store_cbom_window(
            block0_older_store_vaddr,
            block0_older_store_values[round], block0_zero_vaddr,
            block1_inval_vaddr, block2_inval_vaddr,
            block0_younger_store_vaddr,
            block0_younger_store_values[round], block0_clean_vaddr,
            block1_clean_vaddr, block2_clean_vaddr, block0_flush_vaddr,
            block1_flush_vaddr, block2_flush_vaddr,
            block0_older_load_vaddr, block0_younger_load_vaddr,
            block0_zero_load_vaddr, block1_load_vaddr, block2_load_vaddr,
            marker_vaddr, marker_updates[round], &block0_older_observed,
            &block0_younger_observed, &block0_zero_observed,
            &block1_observed, &block2_observed, &marker_observed);
        AI_ASSERT_NO_EXCEPT_AND("under interleaved three-block CMO pressure, CBO.ZERO drops the same-block older store but preserves the younger store through the following CBOM",
            block0_older_observed == 0ULL &&
            block0_younger_observed == block0_younger_store_values[round] &&
            block0_zero_observed == 0ULL &&
            block1_observed == expected_block1_load &&
            block2_observed == expected_block2_load &&
            marker_observed == marker_updates[round]
        );

        TEST_SETUP_EXCEPT();
        AI_ASSERT_NO_EXCEPT_AND("after the store CBO.ZERO store CBOM probe, block0 is zero except the younger store, block1 and block2 preserve seed, and guards remain intact",
            ld(block0_older_store_vaddr) == 0ULL &&
            ld(block0_younger_store_vaddr) ==
                block0_younger_store_values[round] &&
            ld(block0_zero_load_vaddr) == 0ULL &&
            ld(block1_vaddr) == block1_seeds[round] &&
            ld(block1_load_vaddr) == expected_block1_load &&
            ld(block2_vaddr) == block2_seeds[round] &&
            ld(block2_load_vaddr) == expected_block2_load &&
            ld(before_guard_vaddr) == before_guards[round] &&
            ld(guard01_vaddr) == guard01_values[round] &&
            ld(guard12_vaddr) == guard12_values[round] &&
            ld(after_guard_vaddr) == after_guards[round] &&
            ld(marker_guard_vaddr) == marker_guards[round]
        );

        goto_priv(PRIV_M);
        TEST_ASSERT("backing memory after the store CBO.ZERO store CBOM probe keeps block0 zero except the younger store, block1 and block2 seed payload, marker update, and all guards intact",
            ai_memblock_cmo_triple_block_zero_except_word(block0_paddr,
                block0_younger_store_word,
                block0_younger_store_values[round]) &&
            ai_memblock_cmo_triple_block_matches_seed(block1_paddr,
                block1_seeds[round]) &&
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

bool ai_micro_hs_interleaved_three_block_store_cbom_store_zero_no_stale_finish_owner_corner()
{
    TEST_START();

    const enum test_page source_page = SWITCH1;
    const enum test_page marker_page = SWITCH2;
    const size_t block0_older_store_word = 1U;
    const size_t block0_younger_store_word = 5U;
    const size_t block0_zero_load_word = 6U;
    const size_t block1_load_word = 2U;
    const size_t block2_load_word = 4U;
    const uintptr_t block0_vaddr = hs_page_base(source_page) + 0x2700U;
    const uintptr_t block1_vaddr = hs_page_base(source_page) + 0x2780U;
    const uintptr_t block2_vaddr = hs_page_base(source_page) + 0x2800U;
    const uintptr_t block0_paddr = phys_page_base(source_page) + 0x2700U;
    const uintptr_t block1_paddr = phys_page_base(source_page) + 0x2780U;
    const uintptr_t block2_paddr = phys_page_base(source_page) + 0x2800U;
    const uintptr_t block0_older_store_vaddr =
        block0_vaddr + block0_older_store_word * sizeof(uint64_t);
    const uintptr_t block0_clean_vaddr =
        block0_vaddr + 2U * sizeof(uint64_t);
    const uintptr_t block1_inval_vaddr =
        block1_vaddr + 1U * sizeof(uint64_t);
    const uintptr_t block2_inval_vaddr =
        block2_vaddr + 2U * sizeof(uint64_t);
    const uintptr_t block0_younger_store_vaddr =
        block0_vaddr + block0_younger_store_word * sizeof(uint64_t);
    const uintptr_t block1_clean_vaddr =
        block1_vaddr + 5U * sizeof(uint64_t);
    const uintptr_t block2_clean_vaddr =
        block2_vaddr + 6U * sizeof(uint64_t);
    const uintptr_t block0_zero_vaddr =
        block0_vaddr + 3U * sizeof(uint64_t);
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
    const uintptr_t marker_vaddr = hs_page_base(marker_page) + 0x2700U;
    const uintptr_t marker_paddr = phys_page_base(marker_page) + 0x2700U;
    const uintptr_t marker_guard_vaddr = marker_vaddr + sizeof(uint64_t);
    const uintptr_t marker_guard_paddr = marker_paddr + sizeof(uint64_t);
    const uint64_t block0_seeds[AI_MEMBLOCK_CMO_TRIPLE_ROUNDS] = {
        0x9091929394959697ULL,
        0xa0a1a2a3a4a5a6a7ULL,
        0xb0b1b2b3b4b5b6b7ULL,
    };
    const uint64_t block1_seeds[AI_MEMBLOCK_CMO_TRIPLE_ROUNDS] = {
        0x0706050403020100ULL,
        0x1716151413121110ULL,
        0x2726252423222120ULL,
    };
    const uint64_t block2_seeds[AI_MEMBLOCK_CMO_TRIPLE_ROUNDS] = {
        0x8081828384858687ULL,
        0x7071727374757677ULL,
        0x6061626364656667ULL,
    };
    const uint64_t block0_older_store_values[AI_MEMBLOCK_CMO_TRIPLE_ROUNDS] = {
        0x13579bdf2468ace0ULL,
        0x2468ace03579bdf1ULL,
        0x3579bdf1468ace02ULL,
    };
    const uint64_t block0_younger_store_values[AI_MEMBLOCK_CMO_TRIPLE_ROUNDS] = {
        0x0f0e0d0c0b0a0908ULL,
        0x1f1e1d1c1b1a1918ULL,
        0x2f2e2d2c2b2a2928ULL,
    };
    const uint64_t before_guards[AI_MEMBLOCK_CMO_TRIPLE_ROUNDS] = {
        0xabc00123def04567ULL,
        0xbcd11234ef015678ULL,
        0xcde22345f0126789ULL,
    };
    const uint64_t guard01_values[AI_MEMBLOCK_CMO_TRIPLE_ROUNDS] = {
        0x4567abc00123def0ULL,
        0x5678bcd11234ef01ULL,
        0x6789cde22345f012ULL,
    };
    const uint64_t guard12_values[AI_MEMBLOCK_CMO_TRIPLE_ROUNDS] = {
        0x0fedcba987654321ULL,
        0x1edcba9876543210ULL,
        0x2dcba9876543210fULL,
    };
    const uint64_t after_guards[AI_MEMBLOCK_CMO_TRIPLE_ROUNDS] = {
        0x789abcdef0012345ULL,
        0x89abcdef00123456ULL,
        0x9abcdef001234567ULL,
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
        uint64_t block0_older_observed = 0ULL;
        uint64_t block0_younger_observed = 0ULL;
        uint64_t block0_zero_observed = 0ULL;
        uint64_t block1_observed = 0ULL;
        uint64_t block2_observed = 0ULL;
        uint64_t marker_observed = 0ULL;
        const uint64_t expected_block0_older_before =
            block0_seeds[round] +
            block0_older_store_word * 0x1111111111111111ULL;
        const uint64_t expected_block0_younger_before =
            block0_seeds[round] +
            block0_younger_store_word * 0x1111111111111111ULL;
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
        AI_ASSERT_NO_EXCEPT_AND("before the store CBOM store CBO.ZERO probe starts, all source blocks, guards, and marker are readable",
            ld(block0_vaddr) == block0_seeds[round] &&
            ld(block0_older_store_vaddr) == expected_block0_older_before &&
            ld(block0_younger_store_vaddr) == expected_block0_younger_before &&
            ld(block1_vaddr) == block1_seeds[round] &&
            ld(block1_load_vaddr) == expected_block1_load &&
            ld(block2_vaddr) == block2_seeds[round] &&
            ld(block2_load_vaddr) == expected_block2_load &&
            ld(marker_vaddr) == marker_seeds[round]
        );

        TEST_SETUP_EXCEPT();
        ai_memblock_cmo_triple_three_block_store_cbom_store_zero_window(
            block0_older_store_vaddr,
            block0_older_store_values[round], block0_clean_vaddr,
            block1_inval_vaddr, block2_inval_vaddr,
            block0_younger_store_vaddr,
            block0_younger_store_values[round], block1_clean_vaddr,
            block2_clean_vaddr, block0_zero_vaddr, block0_flush_vaddr,
            block1_flush_vaddr, block2_flush_vaddr,
            block0_older_load_vaddr, block0_younger_load_vaddr,
            block0_zero_load_vaddr, block1_load_vaddr, block2_load_vaddr,
            marker_vaddr, marker_updates[round], &block0_older_observed,
            &block0_younger_observed, &block0_zero_observed,
            &block1_observed, &block2_observed, &marker_observed);
        AI_ASSERT_NO_EXCEPT_AND("under interleaved three-block CMO pressure, final CBO.ZERO clears both same-block store generations after earlier CBOM and younger store activity",
            block0_older_observed == 0ULL &&
            block0_younger_observed == 0ULL &&
            block0_zero_observed == 0ULL &&
            block1_observed == expected_block1_load &&
            block2_observed == expected_block2_load &&
            marker_observed == marker_updates[round]
        );

        TEST_SETUP_EXCEPT();
        AI_ASSERT_NO_EXCEPT_AND("after the store CBOM store CBO.ZERO probe, final zero owns block0 while block1 and block2 preserve seed and guards remain intact",
            ld(block0_older_store_vaddr) == 0ULL &&
            ld(block0_younger_store_vaddr) == 0ULL &&
            ld(block0_zero_load_vaddr) == 0ULL &&
            ld(block1_vaddr) == block1_seeds[round] &&
            ld(block1_load_vaddr) == expected_block1_load &&
            ld(block2_vaddr) == block2_seeds[round] &&
            ld(block2_load_vaddr) == expected_block2_load &&
            ld(before_guard_vaddr) == before_guards[round] &&
            ld(guard01_vaddr) == guard01_values[round] &&
            ld(guard12_vaddr) == guard12_values[round] &&
            ld(after_guard_vaddr) == after_guards[round] &&
            ld(marker_guard_vaddr) == marker_guards[round]
        );

        goto_priv(PRIV_M);
        TEST_ASSERT("backing memory after the store CBOM store CBO.ZERO probe keeps block0 fully zero, block1 and block2 seed payload, marker update, and all guards intact",
            ai_memblock_cmo_triple_block_all_zero(block0_paddr) &&
            ai_memblock_cmo_triple_block_matches_seed(block1_paddr,
                block1_seeds[round]) &&
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

bool ai_micro_hs_interleaved_three_block_store_zero_store_inval_no_stale_zero_owner_corner()
{
    TEST_START();

    const enum test_page source_page = SWITCH1;
    const enum test_page marker_page = SWITCH2;
    const size_t block0_older_store_word = 1U;
    const size_t block0_younger_store_word = 5U;
    const size_t block0_zero_load_word = 6U;
    const size_t block1_load_word = 3U;
    const size_t block2_load_word = 4U;
    const uintptr_t block0_vaddr = hs_page_base(source_page) + 0x2900U;
    const uintptr_t block1_vaddr = hs_page_base(source_page) + 0x2980U;
    const uintptr_t block2_vaddr = hs_page_base(source_page) + 0x2a00U;
    const uintptr_t block0_paddr = phys_page_base(source_page) + 0x2900U;
    const uintptr_t block1_paddr = phys_page_base(source_page) + 0x2980U;
    const uintptr_t block2_paddr = phys_page_base(source_page) + 0x2a00U;
    const uintptr_t block0_older_store_vaddr =
        block0_vaddr + block0_older_store_word * sizeof(uint64_t);
    const uintptr_t block0_zero_vaddr =
        block0_vaddr + 3U * sizeof(uint64_t);
    const uintptr_t block1_inval_vaddr =
        block1_vaddr + 2U * sizeof(uint64_t);
    const uintptr_t block2_zero_vaddr =
        block2_vaddr + 1U * sizeof(uint64_t);
    const uintptr_t block0_younger_store_vaddr =
        block0_vaddr + block0_younger_store_word * sizeof(uint64_t);
    const uintptr_t block1_clean_vaddr =
        block1_vaddr + 5U * sizeof(uint64_t);
    const uintptr_t block2_clean_vaddr =
        block2_vaddr + 6U * sizeof(uint64_t);
    const uintptr_t block0_inval_vaddr =
        block0_vaddr + 2U * sizeof(uint64_t);
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
    const uintptr_t marker_vaddr = hs_page_base(marker_page) + 0x2900U;
    const uintptr_t marker_paddr = phys_page_base(marker_page) + 0x2900U;
    const uintptr_t marker_guard_vaddr = marker_vaddr + sizeof(uint64_t);
    const uintptr_t marker_guard_paddr = marker_paddr + sizeof(uint64_t);
    const uint64_t block0_seeds[AI_MEMBLOCK_CMO_TRIPLE_ROUNDS] = {
        0xabcdef0123456789ULL,
        0xbcdef0123456789aULL,
        0xcdef0123456789abULL,
    };
    const uint64_t block1_seeds[AI_MEMBLOCK_CMO_TRIPLE_ROUNDS] = {
        0x76543210fedcba98ULL,
        0x6543210fedcba987ULL,
        0x543210fedcba9876ULL,
    };
    const uint64_t block2_seeds[AI_MEMBLOCK_CMO_TRIPLE_ROUNDS] = {
        0x0badc0ffee123456ULL,
        0x1badc0ffee234567ULL,
        0x2badc0ffee345678ULL,
    };
    const uint64_t block0_older_store_values[AI_MEMBLOCK_CMO_TRIPLE_ROUNDS] = {
        0xf0e1d2c3b4a59687ULL,
        0xe1d2c3b4a5968778ULL,
        0xd2c3b4a596877869ULL,
    };
    const uint64_t block0_younger_store_values[AI_MEMBLOCK_CMO_TRIPLE_ROUNDS] = {
        0x1111111122222222ULL,
        0x3333333344444444ULL,
        0x5555555566666666ULL,
    };
    const uint64_t before_guards[AI_MEMBLOCK_CMO_TRIPLE_ROUNDS] = {
        0x0123456789abcdefULL,
        0x123456789abcdef0ULL,
        0x23456789abcdef01ULL,
    };
    const uint64_t guard01_values[AI_MEMBLOCK_CMO_TRIPLE_ROUNDS] = {
        0xfedcba9876543210ULL,
        0xedcba9876543210fULL,
        0xdcba9876543210feULL,
    };
    const uint64_t guard12_values[AI_MEMBLOCK_CMO_TRIPLE_ROUNDS] = {
        0x55aa55aa33cc33ccULL,
        0x66bb66bb44dd44ddULL,
        0x77cc77cc55ee55eeULL,
    };
    const uint64_t after_guards[AI_MEMBLOCK_CMO_TRIPLE_ROUNDS] = {
        0xaa55aa55cc33cc33ULL,
        0xbb66bb66dd44dd44ULL,
        0xcc77cc77ee55ee55ULL,
    };
    const uint64_t marker_seeds[AI_MEMBLOCK_CMO_TRIPLE_ROUNDS] = {
        0x1010101020202020ULL,
        0x3030303040404040ULL,
        0x5050505060606060ULL,
    };
    const uint64_t marker_updates[AI_MEMBLOCK_CMO_TRIPLE_ROUNDS] = {
        0xa0a0a0a0b0b0b0b0ULL,
        0xb1b1b1b1c1c1c1c1ULL,
        0xc2c2c2c2d2d2d2d2ULL,
    };
    const uint64_t marker_guards[AI_MEMBLOCK_CMO_TRIPLE_ROUNDS] = {
        0x0807060504030201ULL,
        0x1817161514131211ULL,
        0x2827262524232221ULL,
    };

    ai_memblock_cmo_triple_prepare_hs_env();

    for (size_t round = 0; round < AI_MEMBLOCK_CMO_TRIPLE_ROUNDS; ++round) {
        uint64_t block0_older_observed = 0ULL;
        uint64_t block0_younger_observed = 0ULL;
        uint64_t block0_zero_observed = 0ULL;
        uint64_t block1_observed = 0ULL;
        uint64_t block2_observed = 0ULL;
        uint64_t marker_observed = 0ULL;
        const uint64_t expected_block0_older_before =
            block0_seeds[round] +
            block0_older_store_word * 0x1111111111111111ULL;
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
        AI_ASSERT_NO_EXCEPT_AND("before the store CBO.ZERO store CBO.INVAL probe starts, all source blocks, guards, and marker are readable",
            ld(block0_vaddr) == block0_seeds[round] &&
            ld(block0_older_store_vaddr) == expected_block0_older_before &&
            ld(block0_younger_store_vaddr) == expected_block0_younger_before &&
            ld(block1_vaddr) == block1_seeds[round] &&
            ld(block1_load_vaddr) == expected_block1_load &&
            ld(block2_vaddr) == block2_seeds[round] &&
            ld(marker_vaddr) == marker_seeds[round]
        );

        TEST_SETUP_EXCEPT();
        ai_memblock_cmo_triple_three_block_store_zero_store_inval_window(
            block0_older_store_vaddr,
            block0_older_store_values[round], block0_zero_vaddr,
            block1_inval_vaddr, block2_zero_vaddr,
            block0_younger_store_vaddr,
            block0_younger_store_values[round], block1_clean_vaddr,
            block2_clean_vaddr, block0_inval_vaddr, block0_flush_vaddr,
            block1_flush_vaddr, block2_flush_vaddr,
            block0_older_load_vaddr, block0_younger_load_vaddr,
            block0_zero_load_vaddr, block1_load_vaddr, block2_load_vaddr,
            marker_vaddr, marker_updates[round], &block0_older_observed,
            &block0_younger_observed, &block0_zero_observed,
            &block1_observed, &block2_observed, &marker_observed);
        AI_ASSERT_NO_EXCEPT_AND("under interleaved three-block CMO pressure, final CBO.INVAL does not restore zero-before image over the younger store",
            block0_older_observed == 0ULL &&
            block0_younger_observed == block0_younger_store_values[round] &&
            block0_zero_observed == 0ULL &&
            block1_observed == expected_block1_load &&
            block2_observed == 0ULL &&
            marker_observed == marker_updates[round]
        );

        TEST_SETUP_EXCEPT();
        AI_ASSERT_NO_EXCEPT_AND("after the store CBO.ZERO store CBO.INVAL probe, block0 keeps zero plus younger store, block1 preserves seed, block2 is zero, and guards remain intact",
            ld(block0_older_store_vaddr) == 0ULL &&
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
        TEST_ASSERT("backing memory after the store CBO.ZERO store CBO.INVAL probe keeps block0 zero except the younger store, block1 seed payload, block2 zero, marker update, and all guards intact",
            ai_memblock_cmo_triple_block_zero_except_word(block0_paddr,
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

bool ai_micro_hs_interleaved_three_block_store_inval_store_zero_no_stale_inval_owner_corner()
{
    TEST_START();

    const enum test_page source_page = SWITCH1;
    const enum test_page marker_page = SWITCH2;
    const size_t block0_older_store_word = 1U;
    const size_t block0_younger_store_word = 5U;
    const size_t block0_zero_load_word = 6U;
    const size_t block1_load_word = 4U;
    const size_t block2_load_word = 3U;
    const uintptr_t block0_vaddr = hs_page_base(source_page) + 0x2b00U;
    const uintptr_t block1_vaddr = hs_page_base(source_page) + 0x2b80U;
    const uintptr_t block2_vaddr = hs_page_base(source_page) + 0x2c00U;
    const uintptr_t block0_paddr = phys_page_base(source_page) + 0x2b00U;
    const uintptr_t block1_paddr = phys_page_base(source_page) + 0x2b80U;
    const uintptr_t block2_paddr = phys_page_base(source_page) + 0x2c00U;
    const uintptr_t block0_older_store_vaddr =
        block0_vaddr + block0_older_store_word * sizeof(uint64_t);
    const uintptr_t block0_inval_vaddr =
        block0_vaddr + 3U * sizeof(uint64_t);
    const uintptr_t block1_zero_vaddr =
        block1_vaddr + 6U * sizeof(uint64_t);
    const uintptr_t block2_inval_vaddr =
        block2_vaddr + 2U * sizeof(uint64_t);
    const uintptr_t block0_younger_store_vaddr =
        block0_vaddr + block0_younger_store_word * sizeof(uint64_t);
    const uintptr_t block1_flush_vaddr =
        block1_vaddr +
        (AI_MEMBLOCK_CMO_TRIPLE_BLOCK_WORDS - 1U) * sizeof(uint64_t);
    const uintptr_t block2_clean_vaddr =
        block2_vaddr + 5U * sizeof(uint64_t);
    const uintptr_t block0_zero_vaddr =
        block0_vaddr + 2U * sizeof(uint64_t);
    const uintptr_t block0_flush_vaddr =
        block0_vaddr +
        (AI_MEMBLOCK_CMO_TRIPLE_BLOCK_WORDS - 1U) * sizeof(uint64_t);
    const uintptr_t block2_flush_vaddr =
        block2_vaddr +
        (AI_MEMBLOCK_CMO_TRIPLE_BLOCK_WORDS - 1U) * sizeof(uint64_t);
    const uintptr_t block0_older_load_vaddr = block0_older_store_vaddr;
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
    const uintptr_t marker_vaddr = hs_page_base(marker_page) + 0x2b00U;
    const uintptr_t marker_paddr = phys_page_base(marker_page) + 0x2b00U;
    const uintptr_t marker_guard_vaddr = marker_vaddr + sizeof(uint64_t);
    const uintptr_t marker_guard_paddr = marker_paddr + sizeof(uint64_t);
    const uint64_t block0_seeds[AI_MEMBLOCK_CMO_TRIPLE_ROUNDS] = {
        0x1a2b3c4d5e6f7081ULL,
        0x2b3c4d5e6f708192ULL,
        0x3c4d5e6f708192a3ULL,
    };
    const uint64_t block1_seeds[AI_MEMBLOCK_CMO_TRIPLE_ROUNDS] = {
        0x8f7e6d5c4b3a2918ULL,
        0x9f8e7d6c5b4a3928ULL,
        0xaf9e8d7c6b5a4938ULL,
    };
    const uint64_t block2_seeds[AI_MEMBLOCK_CMO_TRIPLE_ROUNDS] = {
        0x1029384756657483ULL,
        0x213a495867768594ULL,
        0x324b5a69788796a5ULL,
    };
    const uint64_t block0_older_store_values[AI_MEMBLOCK_CMO_TRIPLE_ROUNDS] = {
        0xd0c0b0a090807060ULL,
        0xe1d1c1b1a1918171ULL,
        0xf2e2d2c2b2a29282ULL,
    };
    const uint64_t block0_younger_store_values[AI_MEMBLOCK_CMO_TRIPLE_ROUNDS] = {
        0x060708090a0b0c0dULL,
        0x1718191a1b1c1d1eULL,
        0x28292a2b2c2d2e2fULL,
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
        0x0a1b2c3d4e5f6071ULL,
        0x1b2c3d4e5f607182ULL,
        0x2c3d4e5f60718293ULL,
    };
    const uint64_t after_guards[AI_MEMBLOCK_CMO_TRIPLE_ROUNDS] = {
        0x71605f4e3d2c1b0aULL,
        0x8271605f4e3d2c1bULL,
        0x938271605f4e3d2cULL,
    };
    const uint64_t marker_seeds[AI_MEMBLOCK_CMO_TRIPLE_ROUNDS] = {
        0xabcdef9876543210ULL,
        0xbcdefa9876543211ULL,
        0xcdefab9876543212ULL,
    };
    const uint64_t marker_updates[AI_MEMBLOCK_CMO_TRIPLE_ROUNDS] = {
        0x0123456789abcdefULL,
        0x123456789abcdef0ULL,
        0x23456789abcdef01ULL,
    };
    const uint64_t marker_guards[AI_MEMBLOCK_CMO_TRIPLE_ROUNDS] = {
        0xf1e2d3c4b5a69788ULL,
        0xe2d3c4b5a6978879ULL,
        0xd3c4b5a69788796aULL,
    };

    ai_memblock_cmo_triple_prepare_hs_env();

    for (size_t round = 0; round < AI_MEMBLOCK_CMO_TRIPLE_ROUNDS; ++round) {
        uint64_t block0_older_observed = 0ULL;
        uint64_t block0_younger_observed = 0ULL;
        uint64_t block0_zero_observed = 0ULL;
        uint64_t block1_observed = 0ULL;
        uint64_t block2_observed = 0ULL;
        uint64_t marker_observed = 0ULL;
        const uint64_t expected_block0_older_before =
            block0_seeds[round] +
            block0_older_store_word * 0x1111111111111111ULL;
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
        AI_ASSERT_NO_EXCEPT_AND("before the store CBO.INVAL store CBO.ZERO probe starts, all source blocks, guards, and marker are readable",
            ld(block0_vaddr) == block0_seeds[round] &&
            ld(block0_older_store_vaddr) == expected_block0_older_before &&
            ld(block0_younger_store_vaddr) == expected_block0_younger_before &&
            ld(block1_vaddr) == block1_seeds[round] &&
            ld(block2_vaddr) == block2_seeds[round] &&
            ld(block2_load_vaddr) == expected_block2_load &&
            ld(marker_vaddr) == marker_seeds[round]
        );

        TEST_SETUP_EXCEPT();
        ai_memblock_cmo_triple_three_block_store_inval_store_zero_window(
            block0_older_store_vaddr,
            block0_older_store_values[round], block0_inval_vaddr,
            block1_zero_vaddr, block2_inval_vaddr,
            block0_younger_store_vaddr,
            block0_younger_store_values[round], block1_flush_vaddr,
            block2_clean_vaddr, block0_zero_vaddr, block0_flush_vaddr,
            block2_flush_vaddr, block0_older_load_vaddr,
            block0_younger_load_vaddr, block0_zero_load_vaddr,
            block1_load_vaddr, block2_load_vaddr, marker_vaddr,
            marker_updates[round], &block0_older_observed,
            &block0_younger_observed, &block0_zero_observed,
            &block1_observed, &block2_observed, &marker_observed);
        AI_ASSERT_NO_EXCEPT_AND("under interleaved three-block CMO pressure, final CBO.ZERO clears both same-block store generations after earlier CBO.INVAL and younger store activity",
            block0_older_observed == 0ULL &&
            block0_younger_observed == 0ULL &&
            block0_zero_observed == 0ULL &&
            block1_observed == 0ULL &&
            block2_observed == expected_block2_load &&
            marker_observed == marker_updates[round]
        );

        TEST_SETUP_EXCEPT();
        AI_ASSERT_NO_EXCEPT_AND("after the store CBO.INVAL store CBO.ZERO probe, final zero owns block0, block1 is zero, block2 preserves seed, and guards remain intact",
            ld(block0_older_store_vaddr) == 0ULL &&
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
        TEST_ASSERT("backing memory after the store CBO.INVAL store CBO.ZERO probe keeps block0 and block1 zero, block2 seed payload, marker update, and all guards intact",
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

bool ai_micro_hs_interleaved_three_block_store_cbom_store_inval_no_stale_clean_owner_corner()
{
    TEST_START();

    const enum test_page source_page = SWITCH1;
    const enum test_page marker_page = SWITCH2;
    const size_t block0_older_store_word = 1U;
    const size_t block0_younger_store_word = 5U;
    const size_t block0_seed_load_word = 6U;
    const size_t block1_load_word = 4U;
    const size_t block2_load_word = 3U;
    const uintptr_t block0_vaddr = hs_page_base(source_page) + 0x2d00U;
    const uintptr_t block1_vaddr = hs_page_base(source_page) + 0x2d80U;
    const uintptr_t block2_vaddr = hs_page_base(source_page) + 0x2e00U;
    const uintptr_t block0_paddr = phys_page_base(source_page) + 0x2d00U;
    const uintptr_t block1_paddr = phys_page_base(source_page) + 0x2d80U;
    const uintptr_t block2_paddr = phys_page_base(source_page) + 0x2e00U;
    const uintptr_t block0_older_store_vaddr =
        block0_vaddr + block0_older_store_word * sizeof(uint64_t);
    const uintptr_t block0_clean_vaddr =
        block0_vaddr + 2U * sizeof(uint64_t);
    const uintptr_t block1_zero_vaddr =
        block1_vaddr + 6U * sizeof(uint64_t);
    const uintptr_t block2_inval_vaddr =
        block2_vaddr + 2U * sizeof(uint64_t);
    const uintptr_t block0_younger_store_vaddr =
        block0_vaddr + block0_younger_store_word * sizeof(uint64_t);
    const uintptr_t block1_flush_vaddr =
        block1_vaddr +
        (AI_MEMBLOCK_CMO_TRIPLE_BLOCK_WORDS - 1U) * sizeof(uint64_t);
    const uintptr_t block2_clean_vaddr =
        block2_vaddr + 5U * sizeof(uint64_t);
    const uintptr_t block0_inval_vaddr =
        block0_vaddr + 3U * sizeof(uint64_t);
    const uintptr_t block0_flush_vaddr =
        block0_vaddr +
        (AI_MEMBLOCK_CMO_TRIPLE_BLOCK_WORDS - 1U) * sizeof(uint64_t);
    const uintptr_t block2_flush_vaddr =
        block2_vaddr +
        (AI_MEMBLOCK_CMO_TRIPLE_BLOCK_WORDS - 1U) * sizeof(uint64_t);
    const uintptr_t block0_older_load_vaddr = block0_older_store_vaddr;
    const uintptr_t block0_younger_load_vaddr = block0_younger_store_vaddr;
    const uintptr_t block0_seed_load_vaddr =
        block0_vaddr + block0_seed_load_word * sizeof(uint64_t);
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
    const uintptr_t marker_vaddr = hs_page_base(marker_page) + 0x2d00U;
    const uintptr_t marker_paddr = phys_page_base(marker_page) + 0x2d00U;
    const uintptr_t marker_guard_vaddr = marker_vaddr + sizeof(uint64_t);
    const uintptr_t marker_guard_paddr = marker_paddr + sizeof(uint64_t);
    const uint64_t block0_seeds[AI_MEMBLOCK_CMO_TRIPLE_ROUNDS] = {
        0x4d3c2b1a09182736ULL,
        0x5e4d3c2b1a293847ULL,
        0x6f5e4d3c2b3a4958ULL,
    };
    const uint64_t block1_seeds[AI_MEMBLOCK_CMO_TRIPLE_ROUNDS] = {
        0x362718091a2b3c4dULL,
        0x4738291a2b3c4d5eULL,
        0x58493a2b3c4d5e6fULL,
    };
    const uint64_t block2_seeds[AI_MEMBLOCK_CMO_TRIPLE_ROUNDS] = {
        0x13572468ace0bdf1ULL,
        0x24683579bdf1cef2ULL,
        0x3579468ace02df03ULL,
    };
    const uint64_t block0_older_store_values[AI_MEMBLOCK_CMO_TRIPLE_ROUNDS] = {
        0xabcddcba10203040ULL,
        0xbcdeedcb21314151ULL,
        0xcdeffedc32425262ULL,
    };
    const uint64_t block0_younger_store_values[AI_MEMBLOCK_CMO_TRIPLE_ROUNDS] = {
        0x40302010abcddcbaULL,
        0x51413121bcdeedcbULL,
        0x62524232cdeffedcULL,
    };
    const uint64_t before_guards[AI_MEMBLOCK_CMO_TRIPLE_ROUNDS] = {
        0x0f1e2d3c4b5a6978ULL,
        0x1f2e3d4c5b6a7988ULL,
        0x2f3e4d5c6b7a8998ULL,
    };
    const uint64_t guard01_values[AI_MEMBLOCK_CMO_TRIPLE_ROUNDS] = {
        0x78695a4b3c2d1e0fULL,
        0x88796a5b4c3d2e1fULL,
        0x98897a6b5c4d3e2fULL,
    };
    const uint64_t guard12_values[AI_MEMBLOCK_CMO_TRIPLE_ROUNDS] = {
        0xa5a55a5acc33dd44ULL,
        0xb6b66b6bdd44ee55ULL,
        0xc7c77c7cee55ff66ULL,
    };
    const uint64_t after_guards[AI_MEMBLOCK_CMO_TRIPLE_ROUNDS] = {
        0x44dd33cc5a5aa5a5ULL,
        0x55ee44dd6b6bb6b6ULL,
        0x66ff55ee7c7cc7c7ULL,
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
        0xdad1c8bfb6ada49bULL,
        0xebe2d9d0c7beb5acULL,
        0xfcf3eae1d8cfc6bdULL,
    };

    ai_memblock_cmo_triple_prepare_hs_env();

    for (size_t round = 0; round < AI_MEMBLOCK_CMO_TRIPLE_ROUNDS; ++round) {
        uint64_t block0_older_observed = 0ULL;
        uint64_t block0_younger_observed = 0ULL;
        uint64_t block0_seed_observed = 0ULL;
        uint64_t block1_observed = 0ULL;
        uint64_t block2_observed = 0ULL;
        uint64_t marker_observed = 0ULL;
        const uint64_t expected_block0_older_before =
            block0_seeds[round] +
            block0_older_store_word * 0x1111111111111111ULL;
        const uint64_t expected_block0_younger_before =
            block0_seeds[round] +
            block0_younger_store_word * 0x1111111111111111ULL;
        const uint64_t expected_block0_seed_load =
            block0_seeds[round] +
            block0_seed_load_word * 0x1111111111111111ULL;
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
        AI_ASSERT_NO_EXCEPT_AND("before the store CBOM store CBO.INVAL probe starts, all source blocks, guards, and marker are readable",
            ld(block0_vaddr) == block0_seeds[round] &&
            ld(block0_older_store_vaddr) == expected_block0_older_before &&
            ld(block0_younger_store_vaddr) == expected_block0_younger_before &&
            ld(block0_seed_load_vaddr) == expected_block0_seed_load &&
            ld(block1_vaddr) == block1_seeds[round] &&
            ld(block2_vaddr) == block2_seeds[round] &&
            ld(block2_load_vaddr) == expected_block2_load &&
            ld(marker_vaddr) == marker_seeds[round]
        );

        TEST_SETUP_EXCEPT();
        ai_memblock_cmo_triple_three_block_store_cbom_store_inval_window(
            block0_older_store_vaddr,
            block0_older_store_values[round], block0_clean_vaddr,
            block1_zero_vaddr, block2_inval_vaddr,
            block0_younger_store_vaddr,
            block0_younger_store_values[round], block1_flush_vaddr,
            block2_clean_vaddr, block0_inval_vaddr, block0_flush_vaddr,
            block2_flush_vaddr, block0_older_load_vaddr,
            block0_younger_load_vaddr, block0_seed_load_vaddr,
            block1_load_vaddr, block2_load_vaddr, marker_vaddr,
            marker_updates[round], &block0_older_observed,
            &block0_younger_observed, &block0_seed_observed,
            &block1_observed, &block2_observed, &marker_observed);
        AI_ASSERT_NO_EXCEPT_AND("under interleaved three-block CMO pressure, final CBO.INVAL does not restore pre-younger clean image over either same-block store generation",
            block0_older_observed == block0_older_store_values[round] &&
            block0_younger_observed == block0_younger_store_values[round] &&
            block0_seed_observed == expected_block0_seed_load &&
            block1_observed == 0ULL &&
            block2_observed == expected_block2_load &&
            marker_observed == marker_updates[round]
        );

        TEST_SETUP_EXCEPT();
        AI_ASSERT_NO_EXCEPT_AND("after the store CBOM store CBO.INVAL probe, block0 keeps both store generations, block1 is zero, block2 preserves seed, and guards remain intact",
            ld(block0_older_store_vaddr) ==
                block0_older_store_values[round] &&
            ld(block0_younger_store_vaddr) ==
                block0_younger_store_values[round] &&
            ld(block0_seed_load_vaddr) == expected_block0_seed_load &&
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
        TEST_ASSERT("backing memory after the store CBOM store CBO.INVAL probe keeps block0 seed except both store generations, block1 zero, block2 seed payload, marker update, and all guards intact",
            ai_memblock_cmo_triple_block_matches_seed_except_two_words(
                block0_paddr, block0_seeds[round],
                block0_older_store_word,
                block0_older_store_values[round],
                block0_younger_store_word,
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
