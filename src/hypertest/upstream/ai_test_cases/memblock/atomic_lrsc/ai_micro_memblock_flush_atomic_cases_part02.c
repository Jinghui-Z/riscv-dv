#include "ai_micro_memblock_flush_atomic_common.h"

bool ai_micro_hs_cboinval_then_cross16_store_no_stale_inval_owner_or_partial_payload_corner()
{
    TEST_START();

    const enum test_page src_page = SWITCH1;
    const enum test_page marker_page = SWITCH2;
    const uintptr_t src_base_vaddr = hs_page_base(src_page) + 0xb40U;
    const uintptr_t src_base_paddr = phys_page_base(src_page) + 0xb40U;
    const uintptr_t src_word1_vaddr = src_base_vaddr + sizeof(uint64_t);
    const uintptr_t src_word2_vaddr = src_base_vaddr + 2U * sizeof(uint64_t);
    const uintptr_t inval_vaddr = src_base_vaddr + 5U * sizeof(uint64_t);
    const uintptr_t cross16_store_vaddr = src_base_vaddr + 15U;
    const uintptr_t src_tail_vaddr =
        src_base_vaddr +
        (AI_MEMBLOCK_CMO_AMO_BLOCK_WORDS - 1U) * sizeof(uint64_t);
    const uintptr_t before_guard_vaddr = src_base_vaddr - sizeof(uint64_t);
    const uintptr_t after_guard_vaddr =
        src_base_vaddr + AI_MEMBLOCK_CMO_AMO_BLOCK_WORDS *
        sizeof(uint64_t);
    const uintptr_t before_guard_paddr =
        src_base_paddr - sizeof(uint64_t);
    const uintptr_t after_guard_paddr =
        src_base_paddr + AI_MEMBLOCK_CMO_AMO_BLOCK_WORDS *
        sizeof(uint64_t);
    const uintptr_t marker_vaddr = hs_page_base(marker_page) + 0xb40U;
    const uintptr_t marker_paddr = phys_page_base(marker_page) + 0xb40U;
    const uintptr_t marker_guard_vaddr = marker_vaddr + sizeof(uint64_t);
    const uintptr_t marker_guard_paddr = marker_paddr + sizeof(uint64_t);
    const uint64_t block_seeds[] = {
        0x708192a3b4c5d6e7ULL,
        0x8192a3b4c5d6e7f8ULL,
        0x92a3b4c5d6e7f809ULL,
    };
    const uint64_t store_payloads[] = {
        0xf0e1d2c3b4a59687ULL,
        0x1029384756abcdefULL,
        0x8877665544332211ULL,
    };
    const uint64_t before_guards[] = {
        0x0f1e2d3c4b5a6978ULL,
        0x8877665544332211ULL,
        0xabcdef0123456789ULL,
    };
    const uint64_t after_guards[] = {
        0x9876543210fedcbaULL,
        0x1122334455667788ULL,
        0x8899aabbccddeeffULL,
    };
    const uint64_t marker_seeds[] = {
        0x0101010102020202ULL,
        0x0303030304040404ULL,
        0x0505050506060606ULL,
    };
    const uint64_t marker_updates[] = {
        0x2468ace013579bdfULL,
        0xfdb97531eca86420ULL,
        0x7766554433221100ULL,
    };
    const uint64_t marker_guards[] = {
        0x3141592653589793ULL,
        0x2718281828459045ULL,
        0x1123581321345589ULL,
    };

    ai_memblock_cmo_inval_prepare_hs_env();

    for (size_t round = 0; round < AI_MEMBLOCK_FLUSH_ARRAY_SIZE(block_seeds);
         ++round) {
        const uint64_t seed_word1 =
            block_seeds[round] + 0x1111111111111111ULL;
        const uint64_t seed_word2 =
            block_seeds[round] + 2U * 0x1111111111111111ULL;
        const uint64_t expected_word1 =
            ai_memblock_cmo_cross16_store_low_word_from_seed(seed_word1,
                store_payloads[round]);
        const uint64_t expected_word2 =
            ai_memblock_cmo_cross16_store_high_word_from_seed(seed_word2,
                store_payloads[round]);
        const uint64_t expected_tail =
            block_seeds[round] +
            (AI_MEMBLOCK_CMO_AMO_BLOCK_WORDS - 1U) *
            0x1111111111111111ULL;

        goto_priv(PRIV_M);
        ai_memblock_cmo_amo_seed_block(src_base_paddr, block_seeds[round]);
        write64(before_guard_paddr, before_guards[round]);
        write64(after_guard_paddr, after_guards[round]);
        write64(marker_paddr, marker_seeds[round]);
        write64(marker_guard_paddr, marker_guards[round]);

        goto_priv(PRIV_HS);
        TEST_SETUP_EXCEPT();
        const uint64_t warmed_src = ld(src_base_vaddr);
        const uint64_t warmed_word1 = ld(src_word1_vaddr);
        const uint64_t warmed_word2 = ld(src_word2_vaddr);
        const uint64_t warmed_tail = ld(src_tail_vaddr);
        const uint64_t warmed_marker = ld(marker_vaddr);
        AI_ASSERT_NO_EXCEPT_AND("before the CBO.INVAL plus cross-16B store probe starts, source block and marker are readable with seeded values",
            warmed_src == block_seeds[round] &&
            warmed_word1 == seed_word1 &&
            warmed_word2 == seed_word2 &&
            warmed_tail == expected_tail &&
            warmed_marker == marker_seeds[round]
        );

        TEST_SETUP_EXCEPT();
        ai_memblock_cmo_cboinval_cross16_store_then_marker(inval_vaddr,
            cross16_store_vaddr, store_payloads[round], marker_vaddr,
            marker_updates[round]);
        const uint64_t observed_src = ld(src_base_vaddr);
        const uint64_t observed_word1 = ld(src_word1_vaddr);
        const uint64_t observed_word2 = ld(src_word2_vaddr);
        const uint64_t observed_tail = ld(src_tail_vaddr);
        const uint64_t observed_marker = ld(marker_vaddr);
        AI_ASSERT_NO_EXCEPT_AND("CBO.INVAL immediately followed by a same-block cross-16B store releases inval ownership so the split-store overlay and marker survive",
            observed_src == block_seeds[round] &&
            observed_word1 == expected_word1 &&
            observed_word2 == expected_word2 &&
            observed_tail == expected_tail &&
            observed_marker == marker_updates[round]
        );

        TEST_SETUP_EXCEPT();
        AI_ASSERT_NO_EXCEPT_AND("the CBO.INVAL plus cross-16B store owner-window does not corrupt source or marker guards",
            ld(before_guard_vaddr) == before_guards[round] &&
            ld(after_guard_vaddr) == after_guards[round] &&
            ld(marker_guard_vaddr) == marker_guards[round]
        );

        goto_priv(PRIV_M);
        TEST_ASSERT("backing memory after the CBO.INVAL plus cross-16B store owner-window contains the split-store overlay, marker update, and unchanged guards",
            ai_memblock_cmo_block_matches_cross16_store(src_base_paddr,
                block_seeds[round], store_payloads[round]) &&
            read64(before_guard_paddr) == before_guards[round] &&
            read64(after_guard_paddr) == after_guards[round] &&
            read64(marker_paddr) == marker_updates[round] &&
            read64(marker_guard_paddr) == marker_guards[round]
        );
    }

    ai_memblock_cmo_inval_reset_hs_env();

    TEST_END();
}

bool ai_micro_hs_cross16_store_then_cboinval_no_stale_split_store_or_inval_owner_corner()
{
    TEST_START();

    const enum test_page src_page = SWITCH1;
    const enum test_page marker_page = SWITCH2;
    const uintptr_t src_base_vaddr = hs_page_base(src_page) + 0xd40U;
    const uintptr_t src_base_paddr = phys_page_base(src_page) + 0xd40U;
    const uintptr_t src_word1_vaddr = src_base_vaddr + sizeof(uint64_t);
    const uintptr_t src_word2_vaddr =
        src_base_vaddr + 2U * sizeof(uint64_t);
    const uintptr_t cross16_store_vaddr = src_base_vaddr + 15U;
    const uintptr_t inval_vaddr = src_base_vaddr + 5U * sizeof(uint64_t);
    const uintptr_t src_tail_vaddr =
        src_base_vaddr +
        (AI_MEMBLOCK_CMO_AMO_BLOCK_WORDS - 1U) * sizeof(uint64_t);
    const uintptr_t before_guard_vaddr = src_base_vaddr - sizeof(uint64_t);
    const uintptr_t after_guard_vaddr =
        src_base_vaddr + AI_MEMBLOCK_CMO_AMO_BLOCK_WORDS *
        sizeof(uint64_t);
    const uintptr_t before_guard_paddr =
        src_base_paddr - sizeof(uint64_t);
    const uintptr_t after_guard_paddr =
        src_base_paddr + AI_MEMBLOCK_CMO_AMO_BLOCK_WORDS *
        sizeof(uint64_t);
    const uintptr_t marker_vaddr = hs_page_base(marker_page) + 0xd40U;
    const uintptr_t marker_paddr = phys_page_base(marker_page) + 0xd40U;
    const uintptr_t marker_guard_vaddr = marker_vaddr + sizeof(uint64_t);
    const uintptr_t marker_guard_paddr = marker_paddr + sizeof(uint64_t);
    const uint64_t block_seeds[] = {
        0x5364758697a8b9caULL,
        0x64758697a8b9cadbULL,
        0x758697a8b9cadbecULL,
    };
    const uint64_t store_payloads[] = {
        0x0123456789abcdefULL,
        0xfedcba9876543210ULL,
        0x8877665544332211ULL,
    };
    const uint64_t before_guards[] = {
        0x0badf00d5a5aa5a5ULL,
        0x1020304050607080ULL,
        0x1122334455667788ULL,
    };
    const uint64_t after_guards[] = {
        0xfeedfacecafebeefULL,
        0xdeadc0def00dbabeULL,
        0xfaceb00c13579bdfULL,
    };
    const uint64_t marker_seeds[] = {
        0xa0a1a2a3a4a5a6a7ULL,
        0xb0b1b2b3b4b5b6b7ULL,
        0xc0c1c2c3c4c5c6c7ULL,
    };
    const uint64_t marker_updates[] = {
        0x2468ace013579bdfULL,
        0xfdb97531eca86420ULL,
        0x7766554433221100ULL,
    };
    const uint64_t marker_guards[] = {
        0x3141592653589793ULL,
        0x2718281828459045ULL,
        0x1123581321345589ULL,
    };

    ai_memblock_cmo_inval_prepare_hs_env();

    for (size_t round = 0; round < AI_MEMBLOCK_FLUSH_ARRAY_SIZE(block_seeds);
         ++round) {
        const uint64_t seed_word1 =
            block_seeds[round] + 0x1111111111111111ULL;
        const uint64_t seed_word2 =
            block_seeds[round] + 2U * 0x1111111111111111ULL;
        const uint64_t expected_word1 =
            ai_memblock_cmo_cross16_store_low_word_from_seed(seed_word1,
                store_payloads[round]);
        const uint64_t expected_word2 =
            ai_memblock_cmo_cross16_store_high_word_from_seed(seed_word2,
                store_payloads[round]);
        const uint64_t expected_tail =
            block_seeds[round] +
            (AI_MEMBLOCK_CMO_AMO_BLOCK_WORDS - 1U) *
            0x1111111111111111ULL;

        goto_priv(PRIV_M);
        ai_memblock_cmo_amo_seed_block(src_base_paddr, block_seeds[round]);
        write64(before_guard_paddr, before_guards[round]);
        write64(after_guard_paddr, after_guards[round]);
        write64(marker_paddr, marker_seeds[round]);
        write64(marker_guard_paddr, marker_guards[round]);

        goto_priv(PRIV_HS);
        TEST_SETUP_EXCEPT();
        const uint64_t warmed_src = ld(src_base_vaddr);
        const uint64_t warmed_word1 = ld(src_word1_vaddr);
        const uint64_t warmed_word2 = ld(src_word2_vaddr);
        const uint64_t warmed_tail = ld(src_tail_vaddr);
        const uint64_t warmed_marker = ld(marker_vaddr);
        AI_ASSERT_NO_EXCEPT_AND("before the cross-16B store plus CBO.INVAL probe starts, source block and marker are readable with seeded values",
            warmed_src == block_seeds[round] &&
            warmed_word1 == seed_word1 &&
            warmed_word2 == seed_word2 &&
            warmed_tail == expected_tail &&
            warmed_marker == marker_seeds[round]
        );

        TEST_SETUP_EXCEPT();
        ai_memblock_cmo_cross16_store_cboinval_then_marker(
            cross16_store_vaddr,
            store_payloads[round],
            inval_vaddr,
            marker_vaddr,
            marker_updates[round]);
        const uint64_t observed_src = ld(src_base_vaddr);
        const uint64_t observed_word1 = ld(src_word1_vaddr);
        const uint64_t observed_word2 = ld(src_word2_vaddr);
        const uint64_t observed_tail = ld(src_tail_vaddr);
        const uint64_t observed_marker = ld(marker_vaddr);
        AI_ASSERT_NO_EXCEPT_AND("same-block cross-16B store immediately followed by CBO.INVAL keeps split-store and inval ownership ordered so the partial overlay and marker survive",
            observed_src == block_seeds[round] &&
            observed_word1 == expected_word1 &&
            observed_word2 == expected_word2 &&
            observed_tail == expected_tail &&
            observed_marker == marker_updates[round]
        );

        TEST_SETUP_EXCEPT();
        AI_ASSERT_NO_EXCEPT_AND("the cross-16B store plus CBO.INVAL owner-window does not corrupt source or marker guards",
            ld(before_guard_vaddr) == before_guards[round] &&
            ld(after_guard_vaddr) == after_guards[round] &&
            ld(marker_guard_vaddr) == marker_guards[round]
        );

        goto_priv(PRIV_M);
        TEST_ASSERT("backing memory after the cross-16B store plus CBO.INVAL owner-window contains the precise split-store overlay, marker update, and unchanged guards",
            ai_memblock_cmo_block_matches_cross16_store(src_base_paddr,
                block_seeds[round], store_payloads[round]) &&
            read64(before_guard_paddr) == before_guards[round] &&
            read64(after_guard_paddr) == after_guards[round] &&
            read64(marker_paddr) == marker_updates[round] &&
            read64(marker_guard_paddr) == marker_guards[round]
        );
    }

    ai_memblock_cmo_inval_reset_hs_env();

    TEST_END();
}

bool ai_micro_hs_cross16_store_then_cbom_no_stale_split_store_or_cmo_owner_corner()
{
    TEST_START();

    const enum test_page src_page = SWITCH1;
    const enum test_page marker_page = SWITCH2;
    const uintptr_t src_base_vaddr = hs_page_base(src_page) + 0x300U;
    const uintptr_t src_base_paddr = phys_page_base(src_page) + 0x300U;
    const uintptr_t src_word1_vaddr = src_base_vaddr + sizeof(uint64_t);
    const uintptr_t src_word2_vaddr = src_base_vaddr + 2U * sizeof(uint64_t);
    const uintptr_t cross16_store_vaddr = src_base_vaddr + 15U;
    const uintptr_t src_tail_vaddr =
        src_base_vaddr +
        (AI_MEMBLOCK_CMO_AMO_BLOCK_WORDS - 1U) * sizeof(uint64_t);
    const uintptr_t before_guard_vaddr = src_base_vaddr - sizeof(uint64_t);
    const uintptr_t after_guard_vaddr =
        src_base_vaddr + AI_MEMBLOCK_CMO_AMO_BLOCK_WORDS *
        sizeof(uint64_t);
    const uintptr_t before_guard_paddr =
        src_base_paddr - sizeof(uint64_t);
    const uintptr_t after_guard_paddr =
        src_base_paddr + AI_MEMBLOCK_CMO_AMO_BLOCK_WORDS *
        sizeof(uint64_t);
    const uintptr_t marker_vaddr = hs_page_base(marker_page) + 0x300U;
    const uintptr_t marker_paddr = phys_page_base(marker_page) + 0x300U;
    const uintptr_t marker_guard_vaddr = marker_vaddr + sizeof(uint64_t);
    const uintptr_t marker_guard_paddr = marker_paddr + sizeof(uint64_t);
    const uint64_t block_seeds[] = {
        0x405162738495a6b7ULL,
        0x5162738495a6b7c8ULL,
        0x62738495a6b7c8d9ULL,
    };
    const uint64_t store_payloads[] = {
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
        0x0102030405060708ULL,
        0x1827364554637281ULL,
        0xfedcba9876543210ULL,
    };
    const uint64_t marker_updates[] = {
        0x3141592653589793ULL,
        0x2718281828459045ULL,
        0x1123581321345589ULL,
    };
    const uint64_t marker_guards[] = {
        0x8877665544332211ULL,
        0x7766554433221100ULL,
        0x66554433221100ffULL,
    };

    ai_memblock_cmo_amo_prepare_hs_env();

    for (size_t round = 0; round < AI_MEMBLOCK_FLUSH_ARRAY_SIZE(block_seeds);
         ++round) {
        const uint64_t seed_word1 =
            block_seeds[round] + 0x1111111111111111ULL;
        const uint64_t seed_word2 =
            block_seeds[round] + 2U * 0x1111111111111111ULL;
        const uint64_t expected_word1 =
            ai_memblock_cmo_cross16_store_low_word_from_seed(seed_word1,
                store_payloads[round]);
        const uint64_t expected_word2 =
            ai_memblock_cmo_cross16_store_high_word_from_seed(seed_word2,
                store_payloads[round]);

        goto_priv(PRIV_M);
        ai_memblock_cmo_amo_seed_block(src_base_paddr, block_seeds[round]);
        write64(before_guard_paddr, before_guards[round]);
        write64(after_guard_paddr, after_guards[round]);
        write64(marker_paddr, marker_seeds[round]);
        write64(marker_guard_paddr, marker_guards[round]);

        goto_priv(PRIV_HS);
        TEST_SETUP_EXCEPT();
        const uint64_t warmed_src = ld(src_base_vaddr);
        const uint64_t warmed_word1 = ld(src_word1_vaddr);
        const uint64_t warmed_word2 = ld(src_word2_vaddr);
        const uint64_t warmed_tail = ld(src_tail_vaddr);
        const uint64_t warmed_marker = ld(marker_vaddr);
        AI_ASSERT_NO_EXCEPT_AND("before the cross-16B store plus CBOM probe starts, source block and marker are readable with seeded values",
            warmed_src == block_seeds[round] &&
            warmed_word1 == seed_word1 &&
            warmed_word2 == seed_word2 &&
            warmed_tail == block_seeds[round] +
                (AI_MEMBLOCK_CMO_AMO_BLOCK_WORDS - 1U) *
                0x1111111111111111ULL &&
            warmed_marker == marker_seeds[round]
        );

        TEST_SETUP_EXCEPT();
        ai_memblock_cmo_cross16_store_only(cross16_store_vaddr,
            store_payloads[round]);
        cbo_clean(src_base_vaddr);
        cbo_flush(src_tail_vaddr);
        sd(marker_vaddr, marker_updates[round]);
        fence_iorw_iorw();
        const uint64_t observed_src = ld(src_base_vaddr);
        const uint64_t observed_word1 = ld(src_word1_vaddr);
        const uint64_t observed_word2 = ld(src_word2_vaddr);
        const uint64_t observed_tail = ld(src_tail_vaddr);
        const uint64_t observed_marker = ld(marker_vaddr);
        AI_ASSERT_NO_EXCEPT_AND("same-block cross-16B store immediately followed by CBOM clean/flush keeps split-store and CMO ownership ordered so the partial overlay and marker survive",
            observed_src == block_seeds[round] &&
            observed_word1 == expected_word1 &&
            observed_word2 == expected_word2 &&
            observed_tail == block_seeds[round] +
                (AI_MEMBLOCK_CMO_AMO_BLOCK_WORDS - 1U) *
                0x1111111111111111ULL &&
            observed_marker == marker_updates[round]
        );

        TEST_SETUP_EXCEPT();
        AI_ASSERT_NO_EXCEPT_AND("the cross-16B store plus CBOM owner-window does not corrupt source or marker guards",
            ld(before_guard_vaddr) == before_guards[round] &&
            ld(after_guard_vaddr) == after_guards[round] &&
            ld(marker_guard_vaddr) == marker_guards[round]
        );

        goto_priv(PRIV_M);
        TEST_ASSERT("backing memory after the cross-16B store plus CBOM owner-window contains the precise split-store byte overlay, marker update, and unchanged guards",
            ai_memblock_cmo_block_matches_head_and_cross16_store(
                src_base_paddr,
                block_seeds[round],
                block_seeds[round],
                store_payloads[round]) &&
            read64(before_guard_paddr) == before_guards[round] &&
            read64(after_guard_paddr) == after_guards[round] &&
            read64(marker_paddr) == marker_updates[round] &&
            read64(marker_guard_paddr) == marker_guards[round]
        );
    }

    ai_memblock_cmo_amo_reset_hs_env();

    TEST_END();
}

bool ai_micro_hs_cross16_store_then_cbozero_no_stale_split_store_or_zero_owner_corner()
{
    TEST_START();

    const enum test_page src_page = SWITCH1;
    const enum test_page marker_page = SWITCH2;
    const uintptr_t src_base_vaddr = hs_page_base(src_page) + 0x400U;
    const uintptr_t src_base_paddr = phys_page_base(src_page) + 0x400U;
    const uintptr_t src_word1_vaddr = src_base_vaddr + sizeof(uint64_t);
    const uintptr_t src_word2_vaddr = src_base_vaddr + 2U * sizeof(uint64_t);
    const uintptr_t cross16_store_vaddr = src_base_vaddr + 15U;
    const uintptr_t zero_subblock_vaddr =
        src_base_vaddr + 3U * sizeof(uint64_t);
    const uintptr_t src_tail_vaddr =
        src_base_vaddr +
        (AI_MEMBLOCK_CMO_AMO_BLOCK_WORDS - 1U) * sizeof(uint64_t);
    const uintptr_t before_guard_vaddr = src_base_vaddr - sizeof(uint64_t);
    const uintptr_t after_guard_vaddr =
        src_base_vaddr + AI_MEMBLOCK_CMO_AMO_BLOCK_WORDS *
        sizeof(uint64_t);
    const uintptr_t before_guard_paddr =
        src_base_paddr - sizeof(uint64_t);
    const uintptr_t after_guard_paddr =
        src_base_paddr + AI_MEMBLOCK_CMO_AMO_BLOCK_WORDS *
        sizeof(uint64_t);
    const uintptr_t marker_vaddr = hs_page_base(marker_page) + 0x400U;
    const uintptr_t marker_paddr = phys_page_base(marker_page) + 0x400U;
    const uintptr_t marker_guard_vaddr = marker_vaddr + sizeof(uint64_t);
    const uintptr_t marker_guard_paddr = marker_paddr + sizeof(uint64_t);
    const uint64_t block_seeds[] = {
        0x123456789abcdef0ULL,
        0x21436587a9cbed0fULL,
        0x3141592653589793ULL,
    };
    const uint64_t store_payloads[] = {
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
        0x0102030405060708ULL,
        0x1827364554637281ULL,
        0xfedcba9876543210ULL,
    };
    const uint64_t marker_updates[] = {
        0x2468ace013579bdfULL,
        0xfdb97531eca86420ULL,
        0x7766554433221100ULL,
    };
    const uint64_t marker_guards[] = {
        0x8877665544332211ULL,
        0x7766554433221100ULL,
        0x66554433221100ffULL,
    };

    goto_priv(PRIV_M);
    CSRS(CSR_MENVCFG, MENVCFG_CBZE);
    hspt_init();
    sfence_vma();

    for (size_t round = 0; round < AI_MEMBLOCK_FLUSH_ARRAY_SIZE(block_seeds);
         ++round) {
        const uint64_t seed_word1 =
            block_seeds[round] + 0x1111111111111111ULL;
        const uint64_t seed_word2 =
            block_seeds[round] + 2U * 0x1111111111111111ULL;

        goto_priv(PRIV_M);
        ai_memblock_cmo_amo_seed_block(src_base_paddr, block_seeds[round]);
        write64(before_guard_paddr, before_guards[round]);
        write64(after_guard_paddr, after_guards[round]);
        write64(marker_paddr, marker_seeds[round]);
        write64(marker_guard_paddr, marker_guards[round]);

        goto_priv(PRIV_HS);
        TEST_SETUP_EXCEPT();
        const uint64_t warmed_src = ld(src_base_vaddr);
        const uint64_t warmed_word1 = ld(src_word1_vaddr);
        const uint64_t warmed_word2 = ld(src_word2_vaddr);
        const uint64_t warmed_tail = ld(src_tail_vaddr);
        const uint64_t warmed_marker = ld(marker_vaddr);
        AI_ASSERT_NO_EXCEPT_AND("before the cross-16B store plus CBO.ZERO probe starts, source block and marker are readable with seeded values",
            warmed_src == block_seeds[round] &&
            warmed_word1 == seed_word1 &&
            warmed_word2 == seed_word2 &&
            warmed_tail == block_seeds[round] +
                (AI_MEMBLOCK_CMO_AMO_BLOCK_WORDS - 1U) *
                0x1111111111111111ULL &&
            warmed_marker == marker_seeds[round]
        );

        TEST_SETUP_EXCEPT();
        ai_memblock_cmo_cross16_store_only(cross16_store_vaddr,
            store_payloads[round]);
        cbo_zero(zero_subblock_vaddr);
        sd(marker_vaddr, marker_updates[round]);
        fence_iorw_iorw();
        const uint64_t observed_src = ld(src_base_vaddr);
        const uint64_t observed_word1 = ld(src_word1_vaddr);
        const uint64_t observed_word2 = ld(src_word2_vaddr);
        const uint64_t observed_tail = ld(src_tail_vaddr);
        const uint64_t observed_marker = ld(marker_vaddr);
        AI_ASSERT_NO_EXCEPT_AND("same-block cross-16B store immediately followed by CBO.ZERO keeps split-store and zero flush ownership ordered so the final block is fully zero and marker still progresses",
            observed_src == 0ULL &&
            observed_word1 == 0ULL &&
            observed_word2 == 0ULL &&
            observed_tail == 0ULL &&
            observed_marker == marker_updates[round]
        );

        TEST_SETUP_EXCEPT();
        AI_ASSERT_NO_EXCEPT_AND("the cross-16B store plus CBO.ZERO owner-window does not corrupt source or marker guards",
            ld(before_guard_vaddr) == before_guards[round] &&
            ld(after_guard_vaddr) == after_guards[round] &&
            ld(marker_guard_vaddr) == marker_guards[round]
        );

        goto_priv(PRIV_M);
        TEST_ASSERT("backing memory after the cross-16B store plus CBO.ZERO owner-window contains a fully zeroed source block, marker update, and unchanged guards",
            ai_memblock_cmo_amo_block_all_zero(src_base_paddr) &&
            read64(before_guard_paddr) == before_guards[round] &&
            read64(after_guard_paddr) == after_guards[round] &&
            read64(marker_paddr) == marker_updates[round] &&
            read64(marker_guard_paddr) == marker_guards[round]
        );
    }

    goto_priv(PRIV_M);
    CSRC(CSR_MENVCFG, MENVCFG_CBZE);
    hspt_init();
    sfence_vma();

    TEST_END();
}

bool ai_micro_hs_cross16_store_then_amo_no_stale_split_store_or_atomic_owner_corner()
{
    TEST_START();

    const enum test_page src_page = SWITCH1;
    const enum test_page marker_page = SWITCH2;
    const uintptr_t src_base_vaddr = hs_page_base(src_page) + 0x900U;
    const uintptr_t src_base_paddr = phys_page_base(src_page) + 0x900U;
    const uintptr_t src_word1_vaddr = src_base_vaddr + sizeof(uint64_t);
    const uintptr_t src_word2_vaddr = src_base_vaddr + 2U * sizeof(uint64_t);
    const uintptr_t amo_target_vaddr =
        src_base_vaddr + 3U * sizeof(uint64_t);
    const uintptr_t cross16_store_vaddr = src_base_vaddr + 15U;
    const uintptr_t src_tail_vaddr =
        src_base_vaddr +
        (AI_MEMBLOCK_CMO_AMO_BLOCK_WORDS - 1U) * sizeof(uint64_t);
    const uintptr_t before_guard_vaddr = src_base_vaddr - sizeof(uint64_t);
    const uintptr_t after_guard_vaddr =
        src_base_vaddr + AI_MEMBLOCK_CMO_AMO_BLOCK_WORDS *
        sizeof(uint64_t);
    const uintptr_t before_guard_paddr =
        src_base_paddr - sizeof(uint64_t);
    const uintptr_t after_guard_paddr =
        src_base_paddr + AI_MEMBLOCK_CMO_AMO_BLOCK_WORDS *
        sizeof(uint64_t);
    const uintptr_t marker_vaddr = hs_page_base(marker_page) + 0x900U;
    const uintptr_t marker_paddr = phys_page_base(marker_page) + 0x900U;
    const uintptr_t marker_guard_vaddr = marker_vaddr + sizeof(uint64_t);
    const uintptr_t marker_guard_paddr = marker_paddr + sizeof(uint64_t);
    const uint64_t block_seeds[] = {
        0x123456789abcdef0ULL,
        0x21436587a9cbed0fULL,
        0x3141592653589793ULL,
    };
    const uint64_t store_payloads[] = {
        0x1122334455667788ULL,
        0x99aabbccddeeff10ULL,
        0x0f1e2d3c4b5a6978ULL,
    };
    const uint64_t amo_addends[] = {
        0x0001000100010001ULL,
        0x0002000200020002ULL,
        0x0003000300030003ULL,
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
        0x0102030405060708ULL,
        0x1827364554637281ULL,
        0xfedcba9876543210ULL,
    };
    const uint64_t marker_updates[] = {
        0x89abcdef01234567ULL,
        0x76543210fedcba98ULL,
        0x55aa55aa33cc33ccULL,
    };
    const uint64_t marker_guards[] = {
        0x3141592653589793ULL,
        0x2718281828459045ULL,
        0x1123581321345589ULL,
    };

    goto_priv(PRIV_HS);
    hspt_init();
    sfence_vma();

    for (size_t round = 0; round < AI_MEMBLOCK_FLUSH_ARRAY_SIZE(block_seeds);
         ++round) {
        const uint64_t seed_word1 =
            block_seeds[round] + 0x1111111111111111ULL;
        const uint64_t seed_word2 =
            block_seeds[round] + 2U * 0x1111111111111111ULL;
        const uint64_t seed_amo_target =
            block_seeds[round] + 3U * 0x1111111111111111ULL;
        const uint64_t expected_word1 =
            ai_memblock_cmo_cross16_store_low_word_from_seed(seed_word1,
                store_payloads[round]);
        const uint64_t expected_word2 =
            ai_memblock_cmo_cross16_store_high_word_from_seed(seed_word2,
                store_payloads[round]);
        const uint64_t expected_amo_target =
            seed_amo_target + amo_addends[round];

        goto_priv(PRIV_M);
        ai_memblock_cmo_amo_seed_block(src_base_paddr, block_seeds[round]);
        write64(before_guard_paddr, before_guards[round]);
        write64(after_guard_paddr, after_guards[round]);
        write64(marker_paddr, marker_seeds[round]);
        write64(marker_guard_paddr, marker_guards[round]);

        goto_priv(PRIV_HS);
        TEST_SETUP_EXCEPT();
        const uint64_t warmed_src = ld(src_base_vaddr);
        const uint64_t warmed_word1 = ld(src_word1_vaddr);
        const uint64_t warmed_word2 = ld(src_word2_vaddr);
        const uint64_t warmed_amo_target = ld(amo_target_vaddr);
        const uint64_t warmed_tail = ld(src_tail_vaddr);
        const uint64_t warmed_marker = ld(marker_vaddr);
        AI_ASSERT_NO_EXCEPT_AND("before the cross-16B store plus AMO probe starts, source block and marker are readable with seeded values",
            warmed_src == block_seeds[round] &&
            warmed_word1 == seed_word1 &&
            warmed_word2 == seed_word2 &&
            warmed_amo_target == seed_amo_target &&
            warmed_tail == block_seeds[round] +
                (AI_MEMBLOCK_CMO_AMO_BLOCK_WORDS - 1U) *
                0x1111111111111111ULL &&
            warmed_marker == marker_seeds[round]
        );

        TEST_SETUP_EXCEPT();
        uint64_t amo_old_observed = 0ULL;
        ai_memblock_cmo_cross16_store_amo_then_marker(
            cross16_store_vaddr,
            store_payloads[round],
            amo_target_vaddr,
            amo_addends[round],
            marker_vaddr,
            marker_updates[round],
            &amo_old_observed);
        const uint64_t observed_src = ld(src_base_vaddr);
        const uint64_t observed_word1 = ld(src_word1_vaddr);
        const uint64_t observed_word2 = ld(src_word2_vaddr);
        const uint64_t observed_amo_target = ld(amo_target_vaddr);
        const uint64_t observed_tail = ld(src_tail_vaddr);
        const uint64_t observed_marker = ld(marker_vaddr);
        AI_ASSERT_NO_EXCEPT_AND("same-block cross-16B store immediately followed by AMO keeps split-store and atomic flush ownership ordered so the split overlay survives and AMO sees current target",
            amo_old_observed == seed_amo_target &&
            observed_src == block_seeds[round] &&
            observed_word1 == expected_word1 &&
            observed_word2 == expected_word2 &&
            observed_amo_target == expected_amo_target &&
            observed_tail == block_seeds[round] +
                (AI_MEMBLOCK_CMO_AMO_BLOCK_WORDS - 1U) *
                0x1111111111111111ULL &&
            observed_marker == marker_updates[round]
        );

        TEST_SETUP_EXCEPT();
        AI_ASSERT_NO_EXCEPT_AND("the cross-16B store plus AMO owner-window does not corrupt source or marker guards",
            ld(before_guard_vaddr) == before_guards[round] &&
            ld(after_guard_vaddr) == after_guards[round] &&
            ld(marker_guard_vaddr) == marker_guards[round]
        );

        goto_priv(PRIV_M);
        TEST_ASSERT("backing memory after the cross-16B store plus AMO owner-window contains the split-store overlay, AMO update, marker update, and unchanged guards",
            ai_memblock_cmo_amo_block_matches_three_updates(
                src_base_paddr,
                block_seeds[round],
                1U,
                expected_word1,
                2U,
                expected_word2,
                3U,
                expected_amo_target) &&
            read64(before_guard_paddr) == before_guards[round] &&
            read64(after_guard_paddr) == after_guards[round] &&
            read64(marker_paddr) == marker_updates[round] &&
            read64(marker_guard_paddr) == marker_guards[round]
        );
    }

    TEST_END();
}

bool ai_micro_hs_cross16_store_then_lrsc_no_stale_split_store_or_reservation_owner_corner()
{
    TEST_START();

    const enum test_page src_page = SWITCH1;
    const enum test_page marker_page = SWITCH2;
    const uintptr_t src_base_vaddr = hs_page_base(src_page) + 0xa00U;
    const uintptr_t src_base_paddr = phys_page_base(src_page) + 0xa00U;
    const uintptr_t src_word1_vaddr = src_base_vaddr + sizeof(uint64_t);
    const uintptr_t src_word2_vaddr = src_base_vaddr + 2U * sizeof(uint64_t);
    const uintptr_t lrsc_target_vaddr =
        src_base_vaddr + 3U * sizeof(uint64_t);
    const uintptr_t cross16_store_vaddr = src_base_vaddr + 15U;
    const uintptr_t src_tail_vaddr =
        src_base_vaddr +
        (AI_MEMBLOCK_CMO_AMO_BLOCK_WORDS - 1U) * sizeof(uint64_t);
    const uintptr_t before_guard_vaddr = src_base_vaddr - sizeof(uint64_t);
    const uintptr_t after_guard_vaddr =
        src_base_vaddr + AI_MEMBLOCK_CMO_AMO_BLOCK_WORDS *
        sizeof(uint64_t);
    const uintptr_t before_guard_paddr =
        src_base_paddr - sizeof(uint64_t);
    const uintptr_t after_guard_paddr =
        src_base_paddr + AI_MEMBLOCK_CMO_AMO_BLOCK_WORDS *
        sizeof(uint64_t);
    const uintptr_t marker_vaddr = hs_page_base(marker_page) + 0xa00U;
    const uintptr_t marker_paddr = phys_page_base(marker_page) + 0xa00U;
    const uintptr_t marker_guard_vaddr = marker_vaddr + sizeof(uint64_t);
    const uintptr_t marker_guard_paddr = marker_paddr + sizeof(uint64_t);
    const uint64_t block_seeds[] = {
        0x1122334455667788ULL,
        0x2233445566778899ULL,
        0x33445566778899aaULL,
    };
    const uint64_t store_payloads[] = {
        0x8877665544332211ULL,
        0x7766554433221100ULL,
        0x66554433221100ffULL,
    };
    const uint64_t sc_payloads[] = {
        0x0102030405060708ULL,
        0x1827364554637281ULL,
        0xfedcba9876543210ULL,
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

    goto_priv(PRIV_HS);
    hspt_init();
    sfence_vma();

    for (size_t round = 0; round < AI_MEMBLOCK_FLUSH_ARRAY_SIZE(block_seeds);
         ++round) {
        const uint64_t seed_word1 =
            block_seeds[round] + 0x1111111111111111ULL;
        const uint64_t seed_word2 =
            block_seeds[round] + 2U * 0x1111111111111111ULL;
        const uint64_t seed_lrsc_target =
            block_seeds[round] + 3U * 0x1111111111111111ULL;
        const uint64_t expected_word1 =
            ai_memblock_cmo_cross16_store_low_word_from_seed(seed_word1,
                store_payloads[round]);
        const uint64_t expected_word2 =
            ai_memblock_cmo_cross16_store_high_word_from_seed(seed_word2,
                store_payloads[round]);

        goto_priv(PRIV_M);
        ai_memblock_cmo_amo_seed_block(src_base_paddr, block_seeds[round]);
        write64(before_guard_paddr, before_guards[round]);
        write64(after_guard_paddr, after_guards[round]);
        write64(marker_paddr, marker_seeds[round]);
        write64(marker_guard_paddr, marker_guards[round]);

        goto_priv(PRIV_HS);
        TEST_SETUP_EXCEPT();
        const uint64_t warmed_src = ld(src_base_vaddr);
        const uint64_t warmed_word1 = ld(src_word1_vaddr);
        const uint64_t warmed_word2 = ld(src_word2_vaddr);
        const uint64_t warmed_lrsc_target = ld(lrsc_target_vaddr);
        const uint64_t warmed_tail = ld(src_tail_vaddr);
        const uint64_t warmed_marker = ld(marker_vaddr);
        AI_ASSERT_NO_EXCEPT_AND("before the cross-16B store plus LR/SC probe starts, source block and marker are readable with seeded values",
            warmed_src == block_seeds[round] &&
            warmed_word1 == seed_word1 &&
            warmed_word2 == seed_word2 &&
            warmed_lrsc_target == seed_lrsc_target &&
            warmed_tail == block_seeds[round] +
                (AI_MEMBLOCK_CMO_AMO_BLOCK_WORDS - 1U) *
                0x1111111111111111ULL &&
            warmed_marker == marker_seeds[round]
        );

        TEST_SETUP_EXCEPT();
        uint64_t lr_observed = 0ULL;
        uint64_t sc_status = 1ULL;
        ai_memblock_cmo_cross16_store_lrsc_then_marker(
            cross16_store_vaddr,
            store_payloads[round],
            lrsc_target_vaddr,
            sc_payloads[round],
            marker_vaddr,
            marker_updates[round],
            &lr_observed,
            &sc_status);
        const uint64_t observed_src = ld(src_base_vaddr);
        const uint64_t observed_word1 = ld(src_word1_vaddr);
        const uint64_t observed_word2 = ld(src_word2_vaddr);
        const uint64_t observed_lrsc_target = ld(lrsc_target_vaddr);
        const uint64_t observed_tail = ld(src_tail_vaddr);
        const uint64_t observed_marker = ld(marker_vaddr);
        AI_ASSERT_NO_EXCEPT_AND("same-block cross-16B store immediately followed by LR/SC keeps split-store and reservation ownership ordered so the split overlay survives and SC succeeds",
            lr_observed == seed_lrsc_target &&
            sc_status == 0ULL &&
            observed_src == block_seeds[round] &&
            observed_word1 == expected_word1 &&
            observed_word2 == expected_word2 &&
            observed_lrsc_target == sc_payloads[round] &&
            observed_tail == block_seeds[round] +
                (AI_MEMBLOCK_CMO_AMO_BLOCK_WORDS - 1U) *
                0x1111111111111111ULL &&
            observed_marker == marker_updates[round]
        );

        TEST_SETUP_EXCEPT();
        AI_ASSERT_NO_EXCEPT_AND("the cross-16B store plus LR/SC owner-window does not corrupt source or marker guards",
            ld(before_guard_vaddr) == before_guards[round] &&
            ld(after_guard_vaddr) == after_guards[round] &&
            ld(marker_guard_vaddr) == marker_guards[round]
        );

        goto_priv(PRIV_M);
        TEST_ASSERT("backing memory after the cross-16B store plus LR/SC owner-window contains the split-store overlay, successful SC payload, marker update, and unchanged guards",
            ai_memblock_cmo_amo_block_matches_three_updates(
                src_base_paddr,
                block_seeds[round],
                1U,
                expected_word1,
                2U,
                expected_word2,
                3U,
                sc_payloads[round]) &&
            read64(before_guard_paddr) == before_guards[round] &&
            read64(after_guard_paddr) == after_guards[round] &&
            read64(marker_paddr) == marker_updates[round] &&
            read64(marker_guard_paddr) == marker_guards[round]
        );
    }

    TEST_END();
}

bool ai_micro_hs_amo_then_cross16_load_no_stale_atomic_owner_or_merge_payload_corner()
{
    TEST_START();

    const enum test_page src_page = SWITCH1;
    const enum test_page marker_page = SWITCH2;
    const uintptr_t src_base_vaddr = hs_page_base(src_page) + 0xb00U;
    const uintptr_t src_base_paddr = phys_page_base(src_page) + 0xb00U;
    const uintptr_t src_word1_vaddr = src_base_vaddr + sizeof(uint64_t);
    const uintptr_t src_word2_vaddr = src_base_vaddr + 2U * sizeof(uint64_t);
    const uintptr_t cross16_load_vaddr = src_base_vaddr + 15U;
    const uintptr_t src_tail_vaddr =
        src_base_vaddr +
        (AI_MEMBLOCK_CMO_AMO_BLOCK_WORDS - 1U) * sizeof(uint64_t);
    const uintptr_t before_guard_vaddr = src_base_vaddr - sizeof(uint64_t);
    const uintptr_t after_guard_vaddr =
        src_base_vaddr + AI_MEMBLOCK_CMO_AMO_BLOCK_WORDS *
        sizeof(uint64_t);
    const uintptr_t before_guard_paddr =
        src_base_paddr - sizeof(uint64_t);
    const uintptr_t after_guard_paddr =
        src_base_paddr + AI_MEMBLOCK_CMO_AMO_BLOCK_WORDS *
        sizeof(uint64_t);
    const uintptr_t marker_vaddr = hs_page_base(marker_page) + 0xb00U;
    const uintptr_t marker_paddr = phys_page_base(marker_page) + 0xb00U;
    const uintptr_t marker_guard_vaddr = marker_vaddr + sizeof(uint64_t);
    const uintptr_t marker_guard_paddr = marker_paddr + sizeof(uint64_t);
    const uint64_t block_seeds[] = {
        0x0a1b2c3d4e5f6071ULL,
        0x1b2c3d4e5f607182ULL,
        0x2c3d4e5f60718293ULL,
    };
    const uint64_t amo_addends[] = {
        0x0001000100010001ULL,
        0x0002000200020002ULL,
        0x0003000300030003ULL,
    };
    const uint64_t word2_updates[] = {
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
        0x0badf00d5a5aa5a5ULL,
        0x1020304050607080ULL,
        0x1122334455667788ULL,
    };
    const uint64_t marker_seeds[] = {
        0x0102030405060708ULL,
        0x1827364554637281ULL,
        0xfedcba9876543210ULL,
    };
    const uint64_t marker_updates[] = {
        0x89abcdef01234567ULL,
        0x76543210fedcba98ULL,
        0x55aa55aa33cc33ccULL,
    };
    const uint64_t marker_guards[] = {
        0x3141592653589793ULL,
        0x2718281828459045ULL,
        0x1123581321345589ULL,
    };

    goto_priv(PRIV_HS);
    hspt_init();
    sfence_vma();

    for (size_t round = 0; round < AI_MEMBLOCK_FLUSH_ARRAY_SIZE(block_seeds);
         ++round) {
        const uint64_t seed_word1 =
            block_seeds[round] + 0x1111111111111111ULL;
        const uint64_t expected_word1 =
            seed_word1 + amo_addends[round];
        const uint64_t expected_cross16 =
            ai_memblock_cmo_cross16_load_expected(expected_word1,
                word2_updates[round]);

        goto_priv(PRIV_M);
        ai_memblock_cmo_amo_seed_block(src_base_paddr, block_seeds[round]);
        write64(before_guard_paddr, before_guards[round]);
        write64(after_guard_paddr, after_guards[round]);
        write64(marker_paddr, marker_seeds[round]);
        write64(marker_guard_paddr, marker_guards[round]);

        goto_priv(PRIV_HS);
        TEST_SETUP_EXCEPT();
        const uint64_t warmed_src = ld(src_base_vaddr);
        const uint64_t warmed_word1 = ld(src_word1_vaddr);
        const uint64_t warmed_word2 = ld(src_word2_vaddr);
        const uint64_t warmed_tail = ld(src_tail_vaddr);
        const uint64_t warmed_marker = ld(marker_vaddr);
        AI_ASSERT_NO_EXCEPT_AND("before the AMO plus cross-16B load probe starts, source block and marker are readable with seeded values",
            warmed_src == block_seeds[round] &&
            warmed_word1 == seed_word1 &&
            warmed_word2 == block_seeds[round] + 2U *
                0x1111111111111111ULL &&
            warmed_tail == block_seeds[round] +
                (AI_MEMBLOCK_CMO_AMO_BLOCK_WORDS - 1U) *
                0x1111111111111111ULL &&
            warmed_marker == marker_seeds[round]
        );

        TEST_SETUP_EXCEPT();
        sd(src_word2_vaddr, word2_updates[round]);
        uint64_t amo_old_observed = 0ULL;
        uint64_t cross16_observed = 0ULL;
        ai_memblock_cmo_amo_cross16_load_then_marker(
            src_word1_vaddr,
            amo_addends[round],
            cross16_load_vaddr,
            marker_vaddr,
            marker_updates[round],
            &amo_old_observed,
            &cross16_observed);
        const uint64_t observed_src = ld(src_base_vaddr);
        const uint64_t observed_word1 = ld(src_word1_vaddr);
        const uint64_t observed_word2 = ld(src_word2_vaddr);
        const uint64_t observed_tail = ld(src_tail_vaddr);
        const uint64_t observed_marker = ld(marker_vaddr);
        AI_ASSERT_NO_EXCEPT_AND("same-block AMO immediately followed by cross-16B load keeps atomic flush ownership and split-load merge ordered so the load sees the AMO-updated high byte and current neighbor payload",
            amo_old_observed == seed_word1 &&
            cross16_observed == expected_cross16 &&
            observed_src == block_seeds[round] &&
            observed_word1 == expected_word1 &&
            observed_word2 == word2_updates[round] &&
            observed_tail == block_seeds[round] +
                (AI_MEMBLOCK_CMO_AMO_BLOCK_WORDS - 1U) *
                0x1111111111111111ULL &&
            observed_marker == marker_updates[round]
        );

        TEST_SETUP_EXCEPT();
        AI_ASSERT_NO_EXCEPT_AND("the AMO plus cross-16B load owner-window does not corrupt source or marker guards",
            ld(before_guard_vaddr) == before_guards[round] &&
            ld(after_guard_vaddr) == after_guards[round] &&
            ld(marker_guard_vaddr) == marker_guards[round]
        );

        goto_priv(PRIV_M);
        TEST_ASSERT("backing memory after the AMO plus cross-16B load owner-window contains the AMO update, neighbor payload, marker update, and unchanged guards",
            ai_memblock_cmo_amo_block_matches_two_updates(
                src_base_paddr,
                block_seeds[round],
                1U,
                expected_word1,
                2U,
                word2_updates[round]) &&
            read64(before_guard_paddr) == before_guards[round] &&
            read64(after_guard_paddr) == after_guards[round] &&
            read64(marker_paddr) == marker_updates[round] &&
            read64(marker_guard_paddr) == marker_guards[round]
        );
    }

    TEST_END();
}

bool ai_micro_hs_lrsc_then_cross16_load_no_stale_reservation_owner_or_merge_payload_corner()
{
    TEST_START();

    const enum test_page src_page = SWITCH1;
    const enum test_page marker_page = SWITCH2;
    const uintptr_t src_base_vaddr = hs_page_base(src_page) + 0xc00U;
    const uintptr_t src_base_paddr = phys_page_base(src_page) + 0xc00U;
    const uintptr_t src_word1_vaddr = src_base_vaddr + sizeof(uint64_t);
    const uintptr_t src_word2_vaddr = src_base_vaddr + 2U * sizeof(uint64_t);
    const uintptr_t cross16_load_vaddr = src_base_vaddr + 15U;
    const uintptr_t src_tail_vaddr =
        src_base_vaddr +
        (AI_MEMBLOCK_CMO_AMO_BLOCK_WORDS - 1U) * sizeof(uint64_t);
    const uintptr_t before_guard_vaddr = src_base_vaddr - sizeof(uint64_t);
    const uintptr_t after_guard_vaddr =
        src_base_vaddr + AI_MEMBLOCK_CMO_AMO_BLOCK_WORDS *
        sizeof(uint64_t);
    const uintptr_t before_guard_paddr =
        src_base_paddr - sizeof(uint64_t);
    const uintptr_t after_guard_paddr =
        src_base_paddr + AI_MEMBLOCK_CMO_AMO_BLOCK_WORDS *
        sizeof(uint64_t);
    const uintptr_t marker_vaddr = hs_page_base(marker_page) + 0xc00U;
    const uintptr_t marker_paddr = phys_page_base(marker_page) + 0xc00U;
    const uintptr_t marker_guard_vaddr = marker_vaddr + sizeof(uint64_t);
    const uintptr_t marker_guard_paddr = marker_paddr + sizeof(uint64_t);
    const uint64_t block_seeds[] = {
        0x0b1c2d3e4f607182ULL,
        0x1c2d3e4f60718293ULL,
        0x2d3e4f60718293a4ULL,
    };
    const uint64_t sc_payloads[] = {
        0x8877665544332211ULL,
        0x7766554433221100ULL,
        0x66554433221100ffULL,
    };
    const uint64_t word2_updates[] = {
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
        0x0badf00d5a5aa5a5ULL,
        0x1020304050607080ULL,
        0x1122334455667788ULL,
    };
    const uint64_t marker_seeds[] = {
        0x0102030405060708ULL,
        0x1827364554637281ULL,
        0xfedcba9876543210ULL,
    };
    const uint64_t marker_updates[] = {
        0xa5a55a5ac3c33c3cULL,
        0xb6b66969d2d22d2dULL,
        0xc7c77878e1e11e1eULL,
    };
    const uint64_t marker_guards[] = {
        0x3141592653589793ULL,
        0x2718281828459045ULL,
        0x1123581321345589ULL,
    };

    goto_priv(PRIV_HS);
    hspt_init();
    sfence_vma();

    for (size_t round = 0; round < AI_MEMBLOCK_FLUSH_ARRAY_SIZE(block_seeds);
         ++round) {
        const uint64_t seed_word1 =
            block_seeds[round] + 0x1111111111111111ULL;
        const uint64_t expected_cross16 =
            ai_memblock_cmo_cross16_load_expected(sc_payloads[round],
                word2_updates[round]);

        goto_priv(PRIV_M);
        ai_memblock_cmo_amo_seed_block(src_base_paddr, block_seeds[round]);
        write64(before_guard_paddr, before_guards[round]);
        write64(after_guard_paddr, after_guards[round]);
        write64(marker_paddr, marker_seeds[round]);
        write64(marker_guard_paddr, marker_guards[round]);

        goto_priv(PRIV_HS);
        TEST_SETUP_EXCEPT();
        const uint64_t warmed_src = ld(src_base_vaddr);
        const uint64_t warmed_word1 = ld(src_word1_vaddr);
        const uint64_t warmed_word2 = ld(src_word2_vaddr);
        const uint64_t warmed_tail = ld(src_tail_vaddr);
        const uint64_t warmed_marker = ld(marker_vaddr);
        AI_ASSERT_NO_EXCEPT_AND("before the LR/SC plus cross-16B load probe starts, source block and marker are readable with seeded values",
            warmed_src == block_seeds[round] &&
            warmed_word1 == seed_word1 &&
            warmed_word2 == block_seeds[round] + 2U *
                0x1111111111111111ULL &&
            warmed_tail == block_seeds[round] +
                (AI_MEMBLOCK_CMO_AMO_BLOCK_WORDS - 1U) *
                0x1111111111111111ULL &&
            warmed_marker == marker_seeds[round]
        );

        TEST_SETUP_EXCEPT();
        sd(src_word2_vaddr, word2_updates[round]);
        uint64_t lr_observed = 0ULL;
        uint64_t sc_status = 1ULL;
        uint64_t cross16_observed = 0ULL;
        ai_memblock_cmo_lrsc_cross16_load_then_marker(
            src_word1_vaddr,
            sc_payloads[round],
            cross16_load_vaddr,
            marker_vaddr,
            marker_updates[round],
            &lr_observed,
            &sc_status,
            &cross16_observed);
        const uint64_t observed_src = ld(src_base_vaddr);
        const uint64_t observed_word1 = ld(src_word1_vaddr);
        const uint64_t observed_word2 = ld(src_word2_vaddr);
        const uint64_t observed_tail = ld(src_tail_vaddr);
        const uint64_t observed_marker = ld(marker_vaddr);
        AI_ASSERT_NO_EXCEPT_AND("same-block LR/SC immediately followed by cross-16B load keeps reservation ownership and split-load merge ordered so the load sees the SC-updated high byte and current neighbor payload",
            lr_observed == seed_word1 &&
            sc_status == 0ULL &&
            cross16_observed == expected_cross16 &&
            observed_src == block_seeds[round] &&
            observed_word1 == sc_payloads[round] &&
            observed_word2 == word2_updates[round] &&
            observed_tail == block_seeds[round] +
                (AI_MEMBLOCK_CMO_AMO_BLOCK_WORDS - 1U) *
                0x1111111111111111ULL &&
            observed_marker == marker_updates[round]
        );

        TEST_SETUP_EXCEPT();
        AI_ASSERT_NO_EXCEPT_AND("the LR/SC plus cross-16B load owner-window does not corrupt source or marker guards",
            ld(before_guard_vaddr) == before_guards[round] &&
            ld(after_guard_vaddr) == after_guards[round] &&
            ld(marker_guard_vaddr) == marker_guards[round]
        );

        goto_priv(PRIV_M);
        TEST_ASSERT("backing memory after the LR/SC plus cross-16B load owner-window contains the SC payload, neighbor payload, marker update, and unchanged guards",
            ai_memblock_cmo_amo_block_matches_two_updates(
                src_base_paddr,
                block_seeds[round],
                1U,
                sc_payloads[round],
                2U,
                word2_updates[round]) &&
            read64(before_guard_paddr) == before_guards[round] &&
            read64(after_guard_paddr) == after_guards[round] &&
            read64(marker_paddr) == marker_updates[round] &&
            read64(marker_guard_paddr) == marker_guards[round]
        );
    }

    TEST_END();
}

bool ai_micro_hs_amo_then_cross16_store_no_stale_atomic_owner_or_partial_payload_corner()
{
    TEST_START();

    const enum test_page src_page = SWITCH1;
    const enum test_page marker_page = SWITCH2;
    const uintptr_t src_base_vaddr = hs_page_base(src_page) + 0xd00U;
    const uintptr_t src_base_paddr = phys_page_base(src_page) + 0xd00U;
    const uintptr_t src_word1_vaddr = src_base_vaddr + sizeof(uint64_t);
    const uintptr_t src_word2_vaddr = src_base_vaddr + 2U * sizeof(uint64_t);
    const uintptr_t cross16_store_vaddr = src_base_vaddr + 15U;
    const uintptr_t src_tail_vaddr =
        src_base_vaddr +
        (AI_MEMBLOCK_CMO_AMO_BLOCK_WORDS - 1U) * sizeof(uint64_t);
    const uintptr_t before_guard_vaddr = src_base_vaddr - sizeof(uint64_t);
    const uintptr_t after_guard_vaddr =
        src_base_vaddr + AI_MEMBLOCK_CMO_AMO_BLOCK_WORDS *
        sizeof(uint64_t);
    const uintptr_t before_guard_paddr =
        src_base_paddr - sizeof(uint64_t);
    const uintptr_t after_guard_paddr =
        src_base_paddr + AI_MEMBLOCK_CMO_AMO_BLOCK_WORDS *
        sizeof(uint64_t);
    const uintptr_t marker_vaddr = hs_page_base(marker_page) + 0xd00U;
    const uintptr_t marker_paddr = phys_page_base(marker_page) + 0xd00U;
    const uintptr_t marker_guard_vaddr = marker_vaddr + sizeof(uint64_t);
    const uintptr_t marker_guard_paddr = marker_paddr + sizeof(uint64_t);
    const uint64_t block_seeds[] = {
        0x0c1d2e3f40516273ULL,
        0x1d2e3f4051627384ULL,
        0x2e3f405162738495ULL,
    };
    const uint64_t amo_addends[] = {
        0x0001000100010001ULL,
        0x0002000200020002ULL,
        0x0003000300030003ULL,
    };
    const uint64_t store_payloads[] = {
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
        0x0102030405060708ULL,
        0x1827364554637281ULL,
        0xfedcba9876543210ULL,
    };
    const uint64_t marker_updates[] = {
        0x89abcdef01234567ULL,
        0x76543210fedcba98ULL,
        0x55aa55aa33cc33ccULL,
    };
    const uint64_t marker_guards[] = {
        0x3141592653589793ULL,
        0x2718281828459045ULL,
        0x1123581321345589ULL,
    };

    goto_priv(PRIV_HS);
    hspt_init();
    sfence_vma();

    for (size_t round = 0; round < AI_MEMBLOCK_FLUSH_ARRAY_SIZE(block_seeds);
         ++round) {
        const uint64_t seed_word1 =
            block_seeds[round] + 0x1111111111111111ULL;
        const uint64_t seed_word2 =
            block_seeds[round] + 2U * 0x1111111111111111ULL;
        const uint64_t amo_updated_word1 =
            seed_word1 + amo_addends[round];
        const uint64_t expected_word1 =
            ai_memblock_cmo_cross16_store_low_word_from_seed(
                amo_updated_word1, store_payloads[round]);
        const uint64_t expected_word2 =
            ai_memblock_cmo_cross16_store_high_word_from_seed(seed_word2,
                store_payloads[round]);

        goto_priv(PRIV_M);
        ai_memblock_cmo_amo_seed_block(src_base_paddr, block_seeds[round]);
        write64(before_guard_paddr, before_guards[round]);
        write64(after_guard_paddr, after_guards[round]);
        write64(marker_paddr, marker_seeds[round]);
        write64(marker_guard_paddr, marker_guards[round]);

        goto_priv(PRIV_HS);
        TEST_SETUP_EXCEPT();
        const uint64_t warmed_src = ld(src_base_vaddr);
        const uint64_t warmed_word1 = ld(src_word1_vaddr);
        const uint64_t warmed_word2 = ld(src_word2_vaddr);
        const uint64_t warmed_tail = ld(src_tail_vaddr);
        const uint64_t warmed_marker = ld(marker_vaddr);
        AI_ASSERT_NO_EXCEPT_AND("before the AMO plus cross-16B store probe starts, source block and marker are readable with seeded values",
            warmed_src == block_seeds[round] &&
            warmed_word1 == seed_word1 &&
            warmed_word2 == seed_word2 &&
            warmed_tail == block_seeds[round] +
                (AI_MEMBLOCK_CMO_AMO_BLOCK_WORDS - 1U) *
                0x1111111111111111ULL &&
            warmed_marker == marker_seeds[round]
        );

        TEST_SETUP_EXCEPT();
        uint64_t amo_old_observed = 0ULL;
        ai_memblock_cmo_amo_cross16_store_then_marker(
            src_word1_vaddr,
            amo_addends[round],
            cross16_store_vaddr,
            store_payloads[round],
            marker_vaddr,
            marker_updates[round],
            &amo_old_observed);
        const uint64_t observed_src = ld(src_base_vaddr);
        const uint64_t observed_word1 = ld(src_word1_vaddr);
        const uint64_t observed_word2 = ld(src_word2_vaddr);
        const uint64_t observed_tail = ld(src_tail_vaddr);
        const uint64_t observed_marker = ld(marker_vaddr);
        AI_ASSERT_NO_EXCEPT_AND("same-block AMO immediately followed by cross-16B store keeps atomic ownership and split-store payload ordered so the store overlays the AMO-updated word without losing either half",
            amo_old_observed == seed_word1 &&
            observed_src == block_seeds[round] &&
            observed_word1 == expected_word1 &&
            observed_word2 == expected_word2 &&
            observed_tail == block_seeds[round] +
                (AI_MEMBLOCK_CMO_AMO_BLOCK_WORDS - 1U) *
                0x1111111111111111ULL &&
            observed_marker == marker_updates[round]
        );

        TEST_SETUP_EXCEPT();
        AI_ASSERT_NO_EXCEPT_AND("the AMO plus cross-16B store owner-window does not corrupt source or marker guards",
            ld(before_guard_vaddr) == before_guards[round] &&
            ld(after_guard_vaddr) == after_guards[round] &&
            ld(marker_guard_vaddr) == marker_guards[round]
        );

        goto_priv(PRIV_M);
        TEST_ASSERT("backing memory after the AMO plus cross-16B store owner-window contains the AMO-influenced split-store overlay, marker update, and unchanged guards",
            ai_memblock_cmo_amo_block_matches_two_updates(
                src_base_paddr,
                block_seeds[round],
                1U,
                expected_word1,
                2U,
                expected_word2) &&
            read64(before_guard_paddr) == before_guards[round] &&
            read64(after_guard_paddr) == after_guards[round] &&
            read64(marker_paddr) == marker_updates[round] &&
            read64(marker_guard_paddr) == marker_guards[round]
        );
    }

    TEST_END();
}

bool ai_micro_hs_lrsc_then_cross16_store_no_stale_reservation_owner_or_partial_payload_corner()
{
    TEST_START();

    const enum test_page src_page = SWITCH1;
    const enum test_page marker_page = SWITCH2;
    const uintptr_t src_base_vaddr = hs_page_base(src_page) + 0xe00U;
    const uintptr_t src_base_paddr = phys_page_base(src_page) + 0xe00U;
    const uintptr_t src_word1_vaddr = src_base_vaddr + sizeof(uint64_t);
    const uintptr_t src_word2_vaddr = src_base_vaddr + 2U * sizeof(uint64_t);
    const uintptr_t cross16_store_vaddr = src_base_vaddr + 15U;
    const uintptr_t src_tail_vaddr =
        src_base_vaddr +
        (AI_MEMBLOCK_CMO_AMO_BLOCK_WORDS - 1U) * sizeof(uint64_t);
    const uintptr_t before_guard_vaddr = src_base_vaddr - sizeof(uint64_t);
    const uintptr_t after_guard_vaddr =
        src_base_vaddr + AI_MEMBLOCK_CMO_AMO_BLOCK_WORDS *
        sizeof(uint64_t);
    const uintptr_t before_guard_paddr =
        src_base_paddr - sizeof(uint64_t);
    const uintptr_t after_guard_paddr =
        src_base_paddr + AI_MEMBLOCK_CMO_AMO_BLOCK_WORDS *
        sizeof(uint64_t);
    const uintptr_t marker_vaddr = hs_page_base(marker_page) + 0xe00U;
    const uintptr_t marker_paddr = phys_page_base(marker_page) + 0xe00U;
    const uintptr_t marker_guard_vaddr = marker_vaddr + sizeof(uint64_t);
    const uintptr_t marker_guard_paddr = marker_paddr + sizeof(uint64_t);
    const uint64_t block_seeds[] = {
        0x0d1e2f4051627384ULL,
        0x1e2f405162738495ULL,
        0x2f405162738495a6ULL,
    };
    const uint64_t sc_payloads[] = {
        0x8877665544332211ULL,
        0x7766554433221100ULL,
        0x66554433221100ffULL,
    };
    const uint64_t store_payloads[] = {
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
        0x0102030405060708ULL,
        0x1827364554637281ULL,
        0xfedcba9876543210ULL,
    };
    const uint64_t marker_updates[] = {
        0xa5a55a5ac3c33c3cULL,
        0xb6b66969d2d22d2dULL,
        0xc7c77878e1e11e1eULL,
    };
    const uint64_t marker_guards[] = {
        0x3141592653589793ULL,
        0x2718281828459045ULL,
        0x1123581321345589ULL,
    };

    goto_priv(PRIV_HS);
    hspt_init();
    sfence_vma();

    for (size_t round = 0; round < AI_MEMBLOCK_FLUSH_ARRAY_SIZE(block_seeds);
         ++round) {
        const uint64_t seed_word1 =
            block_seeds[round] + 0x1111111111111111ULL;
        const uint64_t seed_word2 =
            block_seeds[round] + 2U * 0x1111111111111111ULL;
        const uint64_t expected_word1 =
            ai_memblock_cmo_cross16_store_low_word_from_seed(
                sc_payloads[round], store_payloads[round]);
        const uint64_t expected_word2 =
            ai_memblock_cmo_cross16_store_high_word_from_seed(seed_word2,
                store_payloads[round]);

        goto_priv(PRIV_M);
        ai_memblock_cmo_amo_seed_block(src_base_paddr, block_seeds[round]);
        write64(before_guard_paddr, before_guards[round]);
        write64(after_guard_paddr, after_guards[round]);
        write64(marker_paddr, marker_seeds[round]);
        write64(marker_guard_paddr, marker_guards[round]);

        goto_priv(PRIV_HS);
        TEST_SETUP_EXCEPT();
        const uint64_t warmed_src = ld(src_base_vaddr);
        const uint64_t warmed_word1 = ld(src_word1_vaddr);
        const uint64_t warmed_word2 = ld(src_word2_vaddr);
        const uint64_t warmed_tail = ld(src_tail_vaddr);
        const uint64_t warmed_marker = ld(marker_vaddr);
        AI_ASSERT_NO_EXCEPT_AND("before the LR/SC plus cross-16B store probe starts, source block and marker are readable with seeded values",
            warmed_src == block_seeds[round] &&
            warmed_word1 == seed_word1 &&
            warmed_word2 == seed_word2 &&
            warmed_tail == block_seeds[round] +
                (AI_MEMBLOCK_CMO_AMO_BLOCK_WORDS - 1U) *
                0x1111111111111111ULL &&
            warmed_marker == marker_seeds[round]
        );

        TEST_SETUP_EXCEPT();
        uint64_t lr_observed = 0ULL;
        uint64_t sc_status = 1ULL;
        ai_memblock_cmo_lrsc_cross16_store_then_marker(
            src_word1_vaddr,
            sc_payloads[round],
            cross16_store_vaddr,
            store_payloads[round],
            marker_vaddr,
            marker_updates[round],
            &lr_observed,
            &sc_status);
        const uint64_t observed_src = ld(src_base_vaddr);
        const uint64_t observed_word1 = ld(src_word1_vaddr);
        const uint64_t observed_word2 = ld(src_word2_vaddr);
        const uint64_t observed_tail = ld(src_tail_vaddr);
        const uint64_t observed_marker = ld(marker_vaddr);
        AI_ASSERT_NO_EXCEPT_AND("same-block LR/SC immediately followed by cross-16B store keeps reservation ownership and split-store payload ordered so the store overlays the SC-updated word without losing either half",
            lr_observed == seed_word1 &&
            sc_status == 0ULL &&
            observed_src == block_seeds[round] &&
            observed_word1 == expected_word1 &&
            observed_word2 == expected_word2 &&
            observed_tail == block_seeds[round] +
                (AI_MEMBLOCK_CMO_AMO_BLOCK_WORDS - 1U) *
                0x1111111111111111ULL &&
            observed_marker == marker_updates[round]
        );

        TEST_SETUP_EXCEPT();
        AI_ASSERT_NO_EXCEPT_AND("the LR/SC plus cross-16B store owner-window does not corrupt source or marker guards",
            ld(before_guard_vaddr) == before_guards[round] &&
            ld(after_guard_vaddr) == after_guards[round] &&
            ld(marker_guard_vaddr) == marker_guards[round]
        );

        goto_priv(PRIV_M);
        TEST_ASSERT("backing memory after the LR/SC plus cross-16B store owner-window contains the SC-influenced split-store overlay, marker update, and unchanged guards",
            ai_memblock_cmo_amo_block_matches_two_updates(
                src_base_paddr,
                block_seeds[round],
                1U,
                expected_word1,
                2U,
                expected_word2) &&
            read64(before_guard_paddr) == before_guards[round] &&
            read64(after_guard_paddr) == after_guards[round] &&
            read64(marker_paddr) == marker_updates[round] &&
            read64(marker_guard_paddr) == marker_guards[round]
        );
    }

    TEST_END();
}

bool ai_micro_hs_cross16_store_then_fence_no_stale_split_store_or_sbisempty_corner()
{
    TEST_START();

    const enum test_page src_page = SWITCH1;
    const enum test_page marker_page = SWITCH2;
    const uintptr_t src_base_vaddr = hs_page_base(src_page) + 0xf00U;
    const uintptr_t src_base_paddr = phys_page_base(src_page) + 0xf00U;
    const uintptr_t src_word1_vaddr = src_base_vaddr + sizeof(uint64_t);
    const uintptr_t src_word2_vaddr = src_base_vaddr + 2U * sizeof(uint64_t);
    const uintptr_t cross16_store_vaddr = src_base_vaddr + 15U;
    const uintptr_t src_tail_vaddr =
        src_base_vaddr +
        (AI_MEMBLOCK_CMO_AMO_BLOCK_WORDS - 1U) * sizeof(uint64_t);
    const uintptr_t before_guard_vaddr = src_base_vaddr - sizeof(uint64_t);
    const uintptr_t after_guard_vaddr =
        src_base_vaddr + AI_MEMBLOCK_CMO_AMO_BLOCK_WORDS *
        sizeof(uint64_t);
    const uintptr_t before_guard_paddr =
        src_base_paddr - sizeof(uint64_t);
    const uintptr_t after_guard_paddr =
        src_base_paddr + AI_MEMBLOCK_CMO_AMO_BLOCK_WORDS *
        sizeof(uint64_t);
    const uintptr_t marker_vaddr = hs_page_base(marker_page) + 0xf00U;
    const uintptr_t marker_paddr = phys_page_base(marker_page) + 0xf00U;
    const uintptr_t marker_guard_vaddr = marker_vaddr + sizeof(uint64_t);
    const uintptr_t marker_guard_paddr = marker_paddr + sizeof(uint64_t);
    const uint64_t block_seeds[] = {
        0x0e1f304152637485ULL,
        0x1f30415263748596ULL,
        0x30415263748596a7ULL,
    };
    const uint64_t store_payloads[] = {
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
        0x0102030405060708ULL,
        0x1827364554637281ULL,
        0xfedcba9876543210ULL,
    };
    const uint64_t marker_updates[] = {
        0x89abcdef01234567ULL,
        0x76543210fedcba98ULL,
        0x55aa55aa33cc33ccULL,
    };
    const uint64_t marker_guards[] = {
        0x3141592653589793ULL,
        0x2718281828459045ULL,
        0x1123581321345589ULL,
    };

    goto_priv(PRIV_HS);
    hspt_init();
    sfence_vma();

    for (size_t round = 0; round < AI_MEMBLOCK_FLUSH_ARRAY_SIZE(block_seeds);
         ++round) {
        const uint64_t seed_word1 =
            block_seeds[round] + 0x1111111111111111ULL;
        const uint64_t seed_word2 =
            block_seeds[round] + 2U * 0x1111111111111111ULL;
        const uint64_t expected_word1 =
            ai_memblock_cmo_cross16_store_low_word_from_seed(seed_word1,
                store_payloads[round]);
        const uint64_t expected_word2 =
            ai_memblock_cmo_cross16_store_high_word_from_seed(seed_word2,
                store_payloads[round]);

        goto_priv(PRIV_M);
        ai_memblock_cmo_amo_seed_block(src_base_paddr, block_seeds[round]);
        write64(before_guard_paddr, before_guards[round]);
        write64(after_guard_paddr, after_guards[round]);
        write64(marker_paddr, marker_seeds[round]);
        write64(marker_guard_paddr, marker_guards[round]);

        goto_priv(PRIV_HS);
        TEST_SETUP_EXCEPT();
        const uint64_t warmed_src = ld(src_base_vaddr);
        const uint64_t warmed_word1 = ld(src_word1_vaddr);
        const uint64_t warmed_word2 = ld(src_word2_vaddr);
        const uint64_t warmed_tail = ld(src_tail_vaddr);
        const uint64_t warmed_marker = ld(marker_vaddr);
        AI_ASSERT_NO_EXCEPT_AND("before the cross-16B store plus fence probe starts, source block and marker are readable with seeded values",
            warmed_src == block_seeds[round] &&
            warmed_word1 == seed_word1 &&
            warmed_word2 == seed_word2 &&
            warmed_tail == block_seeds[round] +
                (AI_MEMBLOCK_CMO_AMO_BLOCK_WORDS - 1U) *
                0x1111111111111111ULL &&
            warmed_marker == marker_seeds[round]
        );

        TEST_SETUP_EXCEPT();
        ai_memblock_cmo_cross16_store_fence_then_marker(
            cross16_store_vaddr,
            store_payloads[round],
            marker_vaddr,
            marker_updates[round]);
        const uint64_t observed_src = ld(src_base_vaddr);
        const uint64_t observed_word1 = ld(src_word1_vaddr);
        const uint64_t observed_word2 = ld(src_word2_vaddr);
        const uint64_t observed_tail = ld(src_tail_vaddr);
        const uint64_t observed_marker = ld(marker_vaddr);
        AI_ASSERT_NO_EXCEPT_AND("same-block cross-16B store immediately followed by fence keeps split-store writeback and sbIsEmpty ownership ordered so both halves are visible before the marker store",
            observed_src == block_seeds[round] &&
            observed_word1 == expected_word1 &&
            observed_word2 == expected_word2 &&
            observed_tail == block_seeds[round] +
                (AI_MEMBLOCK_CMO_AMO_BLOCK_WORDS - 1U) *
                0x1111111111111111ULL &&
            observed_marker == marker_updates[round]
        );

        TEST_SETUP_EXCEPT();
        AI_ASSERT_NO_EXCEPT_AND("the cross-16B store plus fence owner-window does not corrupt source or marker guards",
            ld(before_guard_vaddr) == before_guards[round] &&
            ld(after_guard_vaddr) == after_guards[round] &&
            ld(marker_guard_vaddr) == marker_guards[round]
        );

        goto_priv(PRIV_M);
        TEST_ASSERT("backing memory after the cross-16B store plus fence owner-window contains the split-store overlay, marker update, and unchanged guards",
            ai_memblock_cmo_amo_block_matches_two_updates(
                src_base_paddr,
                block_seeds[round],
                1U,
                expected_word1,
                2U,
                expected_word2) &&
            read64(before_guard_paddr) == before_guards[round] &&
            read64(after_guard_paddr) == after_guards[round] &&
            read64(marker_paddr) == marker_updates[round] &&
            read64(marker_guard_paddr) == marker_guards[round]
        );
    }

    TEST_END();
}

bool ai_micro_hs_cross16_load_then_fence_no_stale_merge_or_sbisempty_corner()
{
    TEST_START();

    const enum test_page src_page = SWITCH1;
    const enum test_page marker_page = SWITCH2;
    const uintptr_t src_base_vaddr = hs_page_base(src_page) + 0x100U;
    const uintptr_t src_base_paddr = phys_page_base(src_page) + 0x100U;
    const uintptr_t src_word1_vaddr = src_base_vaddr + sizeof(uint64_t);
    const uintptr_t src_word2_vaddr = src_base_vaddr + 2U * sizeof(uint64_t);
    const uintptr_t cross16_load_vaddr = src_base_vaddr + 15U;
    const uintptr_t src_tail_vaddr =
        src_base_vaddr +
        (AI_MEMBLOCK_CMO_AMO_BLOCK_WORDS - 1U) * sizeof(uint64_t);
    const uintptr_t before_guard_vaddr = src_base_vaddr - sizeof(uint64_t);
    const uintptr_t after_guard_vaddr =
        src_base_vaddr + AI_MEMBLOCK_CMO_AMO_BLOCK_WORDS *
        sizeof(uint64_t);
    const uintptr_t before_guard_paddr =
        src_base_paddr - sizeof(uint64_t);
    const uintptr_t after_guard_paddr =
        src_base_paddr + AI_MEMBLOCK_CMO_AMO_BLOCK_WORDS *
        sizeof(uint64_t);
    const uintptr_t marker_vaddr = hs_page_base(marker_page) + 0x100U;
    const uintptr_t marker_paddr = phys_page_base(marker_page) + 0x100U;
    const uintptr_t marker_guard_vaddr = marker_vaddr + sizeof(uint64_t);
    const uintptr_t marker_guard_paddr = marker_paddr + sizeof(uint64_t);
    const uint64_t block_seeds[] = {
        0x0f20314253647586ULL,
        0x2031425364758697ULL,
        0x31425364758697a8ULL,
    };
    const uint64_t word1_updates[] = {
        0xf0e1d2c3b4a59687ULL,
        0x1029384756abcdefULL,
        0x8877665544332211ULL,
    };
    const uint64_t word2_updates[] = {
        0x0f1e2d3c4b5a6978ULL,
        0xfedcba9876543210ULL,
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
        0x0102030405060708ULL,
        0x1827364554637281ULL,
        0xfedcba9876543210ULL,
    };
    const uint64_t marker_updates[] = {
        0xa5a55a5ac3c33c3cULL,
        0xb6b66969d2d22d2dULL,
        0xc7c77878e1e11e1eULL,
    };
    const uint64_t marker_guards[] = {
        0x3141592653589793ULL,
        0x2718281828459045ULL,
        0x1123581321345589ULL,
    };

    goto_priv(PRIV_HS);
    hspt_init();
    sfence_vma();

    for (size_t round = 0; round < AI_MEMBLOCK_FLUSH_ARRAY_SIZE(block_seeds);
         ++round) {
        const uint64_t expected_cross16 =
            ai_memblock_cmo_cross16_load_expected(word1_updates[round],
                word2_updates[round]);

        goto_priv(PRIV_M);
        ai_memblock_cmo_amo_seed_block(src_base_paddr, block_seeds[round]);
        write64(before_guard_paddr, before_guards[round]);
        write64(after_guard_paddr, after_guards[round]);
        write64(marker_paddr, marker_seeds[round]);
        write64(marker_guard_paddr, marker_guards[round]);

        goto_priv(PRIV_HS);
        TEST_SETUP_EXCEPT();
        const uint64_t warmed_src = ld(src_base_vaddr);
        const uint64_t warmed_word1 = ld(src_word1_vaddr);
        const uint64_t warmed_word2 = ld(src_word2_vaddr);
        const uint64_t warmed_tail = ld(src_tail_vaddr);
        const uint64_t warmed_marker = ld(marker_vaddr);
        AI_ASSERT_NO_EXCEPT_AND("before the cross-16B load plus fence probe starts, source block and marker are readable with seeded values",
            warmed_src == block_seeds[round] &&
            warmed_word1 == block_seeds[round] + 0x1111111111111111ULL &&
            warmed_word2 == block_seeds[round] + 2U *
                0x1111111111111111ULL &&
            warmed_tail == block_seeds[round] +
                (AI_MEMBLOCK_CMO_AMO_BLOCK_WORDS - 1U) *
                0x1111111111111111ULL &&
            warmed_marker == marker_seeds[round]
        );

        TEST_SETUP_EXCEPT();
        sd(src_word1_vaddr, word1_updates[round]);
        sd(src_word2_vaddr, word2_updates[round]);
        const uint64_t cross16_observed =
            ai_memblock_cmo_cross16_load_fence_then_marker(
                cross16_load_vaddr,
                marker_vaddr,
                marker_updates[round]);
        const uint64_t observed_word1 = ld(src_word1_vaddr);
        const uint64_t observed_word2 = ld(src_word2_vaddr);
        const uint64_t observed_tail = ld(src_tail_vaddr);
        const uint64_t observed_marker = ld(marker_vaddr);
        AI_ASSERT_NO_EXCEPT_AND("same-block cross-16B load immediately followed by fence keeps split-load merge and sbIsEmpty ownership ordered so the merged payload and later marker store both remain current",
            cross16_observed == expected_cross16 &&
            observed_word1 == word1_updates[round] &&
            observed_word2 == word2_updates[round] &&
            observed_tail == block_seeds[round] +
                (AI_MEMBLOCK_CMO_AMO_BLOCK_WORDS - 1U) *
                0x1111111111111111ULL &&
            observed_marker == marker_updates[round]
        );

        TEST_SETUP_EXCEPT();
        AI_ASSERT_NO_EXCEPT_AND("the cross-16B load plus fence owner-window does not corrupt source or marker guards",
            ld(before_guard_vaddr) == before_guards[round] &&
            ld(after_guard_vaddr) == after_guards[round] &&
            ld(marker_guard_vaddr) == marker_guards[round]
        );

        goto_priv(PRIV_M);
        TEST_ASSERT("backing memory after the cross-16B load plus fence owner-window contains the two source updates, marker update, and unchanged guards",
            ai_memblock_cmo_amo_block_matches_two_updates(
                src_base_paddr,
                block_seeds[round],
                1U,
                word1_updates[round],
                2U,
                word2_updates[round]) &&
            read64(before_guard_paddr) == before_guards[round] &&
            read64(after_guard_paddr) == after_guards[round] &&
            read64(marker_paddr) == marker_updates[round] &&
            read64(marker_guard_paddr) == marker_guards[round]
        );
    }

    TEST_END();
}

bool ai_micro_hs_fence_then_cross16_store_no_stale_fence_owner_or_partial_payload_corner()
{
    TEST_START();

    const enum test_page src_page = SWITCH1;
    const enum test_page marker_page = SWITCH2;
    const uintptr_t src_base_vaddr = hs_page_base(src_page) + 0x180U;
    const uintptr_t src_base_paddr = phys_page_base(src_page) + 0x180U;
    const uintptr_t src_word1_vaddr = src_base_vaddr + sizeof(uint64_t);
    const uintptr_t src_word2_vaddr = src_base_vaddr + 2U * sizeof(uint64_t);
    const uintptr_t cross16_store_vaddr = src_base_vaddr + 15U;
    const uintptr_t src_tail_vaddr =
        src_base_vaddr +
        (AI_MEMBLOCK_CMO_AMO_BLOCK_WORDS - 1U) * sizeof(uint64_t);
    const uintptr_t before_guard_vaddr = src_base_vaddr - sizeof(uint64_t);
    const uintptr_t after_guard_vaddr =
        src_base_vaddr + AI_MEMBLOCK_CMO_AMO_BLOCK_WORDS *
        sizeof(uint64_t);
    const uintptr_t before_guard_paddr =
        src_base_paddr - sizeof(uint64_t);
    const uintptr_t after_guard_paddr =
        src_base_paddr + AI_MEMBLOCK_CMO_AMO_BLOCK_WORDS *
        sizeof(uint64_t);
    const uintptr_t marker_vaddr = hs_page_base(marker_page) + 0x180U;
    const uintptr_t marker_paddr = phys_page_base(marker_page) + 0x180U;
    const uintptr_t marker_guard_vaddr = marker_vaddr + sizeof(uint64_t);
    const uintptr_t marker_guard_paddr = marker_paddr + sizeof(uint64_t);
    const uint64_t block_seeds[] = {
        0x1021324354657687ULL,
        0x2132435465768798ULL,
        0x32435465768798a9ULL,
    };
    const uint64_t head_payloads[] = {
        0x0102030405060708ULL,
        0x1827364554637281ULL,
        0xfedcba9876543210ULL,
    };
    const uint64_t store_payloads[] = {
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

    goto_priv(PRIV_HS);
    hspt_init();
    sfence_vma();

    for (size_t round = 0; round < AI_MEMBLOCK_FLUSH_ARRAY_SIZE(block_seeds);
         ++round) {
        const uint64_t seed_word1 =
            block_seeds[round] + 0x1111111111111111ULL;
        const uint64_t seed_word2 =
            block_seeds[round] + 2U * 0x1111111111111111ULL;
        const uint64_t expected_word1 =
            ai_memblock_cmo_cross16_store_low_word_from_seed(seed_word1,
                store_payloads[round]);
        const uint64_t expected_word2 =
            ai_memblock_cmo_cross16_store_high_word_from_seed(seed_word2,
                store_payloads[round]);

        goto_priv(PRIV_M);
        ai_memblock_cmo_amo_seed_block(src_base_paddr, block_seeds[round]);
        write64(before_guard_paddr, before_guards[round]);
        write64(after_guard_paddr, after_guards[round]);
        write64(marker_paddr, marker_seeds[round]);
        write64(marker_guard_paddr, marker_guards[round]);

        goto_priv(PRIV_HS);
        TEST_SETUP_EXCEPT();
        const uint64_t warmed_src = ld(src_base_vaddr);
        const uint64_t warmed_word1 = ld(src_word1_vaddr);
        const uint64_t warmed_word2 = ld(src_word2_vaddr);
        const uint64_t warmed_tail = ld(src_tail_vaddr);
        const uint64_t warmed_marker = ld(marker_vaddr);
        AI_ASSERT_NO_EXCEPT_AND("before the fence plus cross-16B store probe starts, source block and marker are readable with seeded values",
            warmed_src == block_seeds[round] &&
            warmed_word1 == seed_word1 &&
            warmed_word2 == seed_word2 &&
            warmed_tail == block_seeds[round] +
                (AI_MEMBLOCK_CMO_AMO_BLOCK_WORDS - 1U) *
                0x1111111111111111ULL &&
            warmed_marker == marker_seeds[round]
        );

        TEST_SETUP_EXCEPT();
        ai_memblock_cmo_fence_cross16_store_then_marker(
            src_base_vaddr,
            head_payloads[round],
            cross16_store_vaddr,
            store_payloads[round],
            marker_vaddr,
            marker_updates[round]);
        const uint64_t observed_src = ld(src_base_vaddr);
        const uint64_t observed_word1 = ld(src_word1_vaddr);
        const uint64_t observed_word2 = ld(src_word2_vaddr);
        const uint64_t observed_tail = ld(src_tail_vaddr);
        const uint64_t observed_marker = ld(marker_vaddr);
        AI_ASSERT_NO_EXCEPT_AND("a fence-drained head store immediately followed by same-block cross-16B store releases fence ownership before split-store issue so the head store, split overlay, and marker all remain current",
            observed_src == head_payloads[round] &&
            observed_word1 == expected_word1 &&
            observed_word2 == expected_word2 &&
            observed_tail == block_seeds[round] +
                (AI_MEMBLOCK_CMO_AMO_BLOCK_WORDS - 1U) *
                0x1111111111111111ULL &&
            observed_marker == marker_updates[round]
        );

        TEST_SETUP_EXCEPT();
        AI_ASSERT_NO_EXCEPT_AND("the fence plus cross-16B store owner-window does not corrupt source or marker guards",
            ld(before_guard_vaddr) == before_guards[round] &&
            ld(after_guard_vaddr) == after_guards[round] &&
            ld(marker_guard_vaddr) == marker_guards[round]
        );

        goto_priv(PRIV_M);
        TEST_ASSERT("backing memory after the fence plus cross-16B store owner-window contains the fence-drained head store, split-store overlay, marker update, and unchanged guards",
            ai_memblock_cmo_block_matches_head_and_cross16_store(
                src_base_paddr,
                block_seeds[round],
                head_payloads[round],
                store_payloads[round]) &&
            read64(before_guard_paddr) == before_guards[round] &&
            read64(after_guard_paddr) == after_guards[round] &&
            read64(marker_paddr) == marker_updates[round] &&
            read64(marker_guard_paddr) == marker_guards[round]
        );
    }

    TEST_END();
}

bool ai_micro_hs_fence_then_cross16_load_no_stale_fence_owner_or_merge_payload_corner()
{
    TEST_START();

    const enum test_page src_page = SWITCH1;
    const enum test_page marker_page = SWITCH2;
    const uintptr_t src_base_vaddr = hs_page_base(src_page) + 0x280U;
    const uintptr_t src_base_paddr = phys_page_base(src_page) + 0x280U;
    const uintptr_t src_word1_vaddr = src_base_vaddr + sizeof(uint64_t);
    const uintptr_t src_word2_vaddr = src_base_vaddr + 2U * sizeof(uint64_t);
    const uintptr_t cross16_load_vaddr = src_base_vaddr + 15U;
    const uintptr_t src_tail_vaddr =
        src_base_vaddr +
        (AI_MEMBLOCK_CMO_AMO_BLOCK_WORDS - 1U) * sizeof(uint64_t);
    const uintptr_t before_guard_vaddr = src_base_vaddr - sizeof(uint64_t);
    const uintptr_t after_guard_vaddr =
        src_base_vaddr + AI_MEMBLOCK_CMO_AMO_BLOCK_WORDS *
        sizeof(uint64_t);
    const uintptr_t before_guard_paddr =
        src_base_paddr - sizeof(uint64_t);
    const uintptr_t after_guard_paddr =
        src_base_paddr + AI_MEMBLOCK_CMO_AMO_BLOCK_WORDS *
        sizeof(uint64_t);
    const uintptr_t marker_vaddr = hs_page_base(marker_page) + 0x280U;
    const uintptr_t marker_paddr = phys_page_base(marker_page) + 0x280U;
    const uintptr_t marker_guard_vaddr = marker_vaddr + sizeof(uint64_t);
    const uintptr_t marker_guard_paddr = marker_paddr + sizeof(uint64_t);
    const uint64_t block_seeds[] = {
        0x0a1b2c3d4e5f6071ULL,
        0x1b2c3d4e5f607182ULL,
        0x2c3d4e5f60718293ULL,
    };
    const uint64_t head_payloads[] = {
        0x1122334455667788ULL,
        0x8877665544332211ULL,
        0x0102030405060708ULL,
    };
    const uint64_t word1_updates[] = {
        0xf1e2d3c4b5a69788ULL,
        0x123456789abcdef0ULL,
        0x55aa55aa33cc33ccULL,
    };
    const uint64_t word2_updates[] = {
        0x0f1e2d3c4b5a6978ULL,
        0xfedcba9876543210ULL,
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
        0x2468ace013579bdfULL,
        0xfdb97531eca86420ULL,
        0x7766554433221100ULL,
    };
    const uint64_t marker_guards[] = {
        0x3141592653589793ULL,
        0x2718281828459045ULL,
        0x1123581321345589ULL,
    };

    goto_priv(PRIV_HS);
    hspt_init();
    sfence_vma();

    for (size_t round = 0; round < AI_MEMBLOCK_FLUSH_ARRAY_SIZE(block_seeds);
         ++round) {
        const uint64_t seed_word1 =
            block_seeds[round] + 0x1111111111111111ULL;
        const uint64_t seed_word2 =
            block_seeds[round] + 2U * 0x1111111111111111ULL;
        const uint64_t expected_cross16 =
            ai_memblock_cmo_cross16_load_expected(word1_updates[round],
                word2_updates[round]);
        uint64_t cross16_observed = 0ULL;

        goto_priv(PRIV_M);
        ai_memblock_cmo_amo_seed_block(src_base_paddr, block_seeds[round]);
        write64(before_guard_paddr, before_guards[round]);
        write64(after_guard_paddr, after_guards[round]);
        write64(marker_paddr, marker_seeds[round]);
        write64(marker_guard_paddr, marker_guards[round]);

        goto_priv(PRIV_HS);
        TEST_SETUP_EXCEPT();
        const uint64_t warmed_src = ld(src_base_vaddr);
        const uint64_t warmed_word1 = ld(src_word1_vaddr);
        const uint64_t warmed_word2 = ld(src_word2_vaddr);
        const uint64_t warmed_tail = ld(src_tail_vaddr);
        const uint64_t warmed_marker = ld(marker_vaddr);
        AI_ASSERT_NO_EXCEPT_AND("before the fence plus cross-16B load probe starts, source block and marker are readable with seeded values",
            warmed_src == block_seeds[round] &&
            warmed_word1 == seed_word1 &&
            warmed_word2 == seed_word2 &&
            warmed_tail == block_seeds[round] +
                (AI_MEMBLOCK_CMO_AMO_BLOCK_WORDS - 1U) *
                0x1111111111111111ULL &&
            warmed_marker == marker_seeds[round]
        );

        TEST_SETUP_EXCEPT();
        ai_memblock_cmo_fence_cross16_load_then_marker(
            src_base_vaddr,
            head_payloads[round],
            src_word1_vaddr,
            word1_updates[round],
            src_word2_vaddr,
            word2_updates[round],
            cross16_load_vaddr,
            marker_vaddr,
            marker_updates[round],
            &cross16_observed);
        const uint64_t observed_src = ld(src_base_vaddr);
        const uint64_t observed_word1 = ld(src_word1_vaddr);
        const uint64_t observed_word2 = ld(src_word2_vaddr);
        const uint64_t observed_tail = ld(src_tail_vaddr);
        const uint64_t observed_marker = ld(marker_vaddr);
        AI_ASSERT_NO_EXCEPT_AND("a fence-drained store group immediately followed by same-block cross-16B load releases fence ownership before split-load merge so the load result and marker remain current",
            cross16_observed == expected_cross16 &&
            observed_src == head_payloads[round] &&
            observed_word1 == word1_updates[round] &&
            observed_word2 == word2_updates[round] &&
            observed_tail == block_seeds[round] +
                (AI_MEMBLOCK_CMO_AMO_BLOCK_WORDS - 1U) *
                0x1111111111111111ULL &&
            observed_marker == marker_updates[round]
        );

        TEST_SETUP_EXCEPT();
        AI_ASSERT_NO_EXCEPT_AND("the fence plus cross-16B load owner-window does not corrupt source or marker guards",
            ld(before_guard_vaddr) == before_guards[round] &&
            ld(after_guard_vaddr) == after_guards[round] &&
            ld(marker_guard_vaddr) == marker_guards[round]
        );

        goto_priv(PRIV_M);
        TEST_ASSERT("backing memory after the fence plus cross-16B load owner-window contains the fence-drained source stores, marker update, and unchanged guards",
            ai_memblock_cmo_amo_block_matches_three_updates(
                src_base_paddr,
                block_seeds[round],
                0U,
                head_payloads[round],
                1U,
                word1_updates[round],
                2U,
                word2_updates[round]) &&
            read64(before_guard_paddr) == before_guards[round] &&
            read64(after_guard_paddr) == after_guards[round] &&
            read64(marker_paddr) == marker_updates[round] &&
            read64(marker_guard_paddr) == marker_guards[round]
        );
    }

    TEST_END();
}

bool ai_micro_hs_cross16_load_then_cbozero_no_stale_merge_or_zero_owner_corner()
{
    TEST_START();

    const enum test_page src_page = SWITCH1;
    const enum test_page marker_page = SWITCH2;
    const uintptr_t src_base_vaddr = hs_page_base(src_page) + 0x500U;
    const uintptr_t src_base_paddr = phys_page_base(src_page) + 0x500U;
    const uintptr_t src_word1_vaddr = src_base_vaddr + sizeof(uint64_t);
    const uintptr_t src_word2_vaddr = src_base_vaddr + 2U * sizeof(uint64_t);
    const uintptr_t cross16_load_vaddr = src_base_vaddr + 15U;
    const uintptr_t zero_subblock_vaddr =
        src_base_vaddr + 3U * sizeof(uint64_t);
    const uintptr_t src_tail_vaddr =
        src_base_vaddr +
        (AI_MEMBLOCK_CMO_AMO_BLOCK_WORDS - 1U) * sizeof(uint64_t);
    const uintptr_t before_guard_vaddr = src_base_vaddr - sizeof(uint64_t);
    const uintptr_t after_guard_vaddr =
        src_base_vaddr + AI_MEMBLOCK_CMO_AMO_BLOCK_WORDS *
        sizeof(uint64_t);
    const uintptr_t before_guard_paddr =
        src_base_paddr - sizeof(uint64_t);
    const uintptr_t after_guard_paddr =
        src_base_paddr + AI_MEMBLOCK_CMO_AMO_BLOCK_WORDS *
        sizeof(uint64_t);
    const uintptr_t marker_vaddr = hs_page_base(marker_page) + 0x500U;
    const uintptr_t marker_paddr = phys_page_base(marker_page) + 0x500U;
    const uintptr_t marker_guard_vaddr = marker_vaddr + sizeof(uint64_t);
    const uintptr_t marker_guard_paddr = marker_paddr + sizeof(uint64_t);
    const uint64_t block_seeds[] = {
        0x15263748596a7b8cULL,
        0x263748596a7b8c9dULL,
        0x3748596a7b8c9daeULL,
    };
    const uint64_t word1_updates[] = {
        0x0123456789abcdefULL,
        0xfedcba9876543210ULL,
        0x8877665544332211ULL,
    };
    const uint64_t word2_updates[] = {
        0x13579bdf2468ace0ULL,
        0x02468ace13579bdfULL,
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
        0x0102030405060708ULL,
        0x1827364554637281ULL,
        0xfedcba9876543210ULL,
    };
    const uint64_t marker_updates[] = {
        0x89abcdef01234567ULL,
        0x76543210fedcba98ULL,
        0x55aa55aa33cc33ccULL,
    };
    const uint64_t marker_guards[] = {
        0x3141592653589793ULL,
        0x2718281828459045ULL,
        0x1123581321345589ULL,
    };

    goto_priv(PRIV_M);
    CSRS(CSR_MENVCFG, MENVCFG_CBZE);
    hspt_init();
    sfence_vma();

    for (size_t round = 0; round < AI_MEMBLOCK_FLUSH_ARRAY_SIZE(block_seeds);
         ++round) {
        const uint64_t expected_cross16 =
            ai_memblock_cmo_cross16_load_expected(word1_updates[round],
                word2_updates[round]);

        goto_priv(PRIV_M);
        ai_memblock_cmo_amo_seed_block(src_base_paddr, block_seeds[round]);
        write64(before_guard_paddr, before_guards[round]);
        write64(after_guard_paddr, after_guards[round]);
        write64(marker_paddr, marker_seeds[round]);
        write64(marker_guard_paddr, marker_guards[round]);

        goto_priv(PRIV_HS);
        TEST_SETUP_EXCEPT();
        const uint64_t warmed_src = ld(src_base_vaddr);
        const uint64_t warmed_word1 = ld(src_word1_vaddr);
        const uint64_t warmed_word2 = ld(src_word2_vaddr);
        const uint64_t warmed_tail = ld(src_tail_vaddr);
        const uint64_t warmed_marker = ld(marker_vaddr);
        AI_ASSERT_NO_EXCEPT_AND("before the cross-16B load plus CBO.ZERO probe starts, source block and marker are readable with seeded values",
            warmed_src == block_seeds[round] &&
            warmed_word1 == block_seeds[round] + 0x1111111111111111ULL &&
            warmed_word2 == block_seeds[round] + 2U *
                0x1111111111111111ULL &&
            warmed_tail == block_seeds[round] +
                (AI_MEMBLOCK_CMO_AMO_BLOCK_WORDS - 1U) *
                0x1111111111111111ULL &&
            warmed_marker == marker_seeds[round]
        );

        TEST_SETUP_EXCEPT();
        sd(src_word1_vaddr, word1_updates[round]);
        sd(src_word2_vaddr, word2_updates[round]);
        const uint64_t cross16_observed =
            ai_memblock_cmo_cross16_load_cbozero_then_marker(
                cross16_load_vaddr,
                zero_subblock_vaddr,
                marker_vaddr,
                marker_updates[round]);
        const uint64_t observed_src = ld(src_base_vaddr);
        const uint64_t observed_word1 = ld(src_word1_vaddr);
        const uint64_t observed_word2 = ld(src_word2_vaddr);
        const uint64_t observed_tail = ld(src_tail_vaddr);
        const uint64_t observed_marker = ld(marker_vaddr);
        AI_ASSERT_NO_EXCEPT_AND("same-block cross-16B load immediately followed by CBO.ZERO keeps split-load merge and zero flush ownership ordered so the load sees current payload and final block is zero",
            cross16_observed == expected_cross16 &&
            observed_src == 0ULL &&
            observed_word1 == 0ULL &&
            observed_word2 == 0ULL &&
            observed_tail == 0ULL &&
            observed_marker == marker_updates[round]
        );

        TEST_SETUP_EXCEPT();
        AI_ASSERT_NO_EXCEPT_AND("the cross-16B load plus CBO.ZERO owner-window does not corrupt source or marker guards",
            ld(before_guard_vaddr) == before_guards[round] &&
            ld(after_guard_vaddr) == after_guards[round] &&
            ld(marker_guard_vaddr) == marker_guards[round]
        );

        goto_priv(PRIV_M);
        TEST_ASSERT("backing memory after the cross-16B load plus CBO.ZERO owner-window contains a fully zeroed source block, marker update, and unchanged guards",
            ai_memblock_cmo_amo_block_all_zero(src_base_paddr) &&
            read64(before_guard_paddr) == before_guards[round] &&
            read64(after_guard_paddr) == after_guards[round] &&
            read64(marker_paddr) == marker_updates[round] &&
            read64(marker_guard_paddr) == marker_guards[round]
        );
    }

    goto_priv(PRIV_M);
    CSRC(CSR_MENVCFG, MENVCFG_CBZE);
    hspt_init();
    sfence_vma();

    TEST_END();
}

bool ai_micro_hs_cross16_load_then_cbom_no_stale_merge_or_cmo_owner_corner()
{
    TEST_START();

    const enum test_page src_page = SWITCH1;
    const enum test_page marker_page = SWITCH2;
    const uintptr_t src_base_vaddr = hs_page_base(src_page) + 0x600U;
    const uintptr_t src_base_paddr = phys_page_base(src_page) + 0x600U;
    const uintptr_t src_word1_vaddr = src_base_vaddr + sizeof(uint64_t);
    const uintptr_t src_word2_vaddr = src_base_vaddr + 2U * sizeof(uint64_t);
    const uintptr_t cross16_load_vaddr = src_base_vaddr + 15U;
    const uintptr_t src_tail_vaddr =
        src_base_vaddr +
        (AI_MEMBLOCK_CMO_AMO_BLOCK_WORDS - 1U) * sizeof(uint64_t);
    const uintptr_t before_guard_vaddr = src_base_vaddr - sizeof(uint64_t);
    const uintptr_t after_guard_vaddr =
        src_base_vaddr + AI_MEMBLOCK_CMO_AMO_BLOCK_WORDS *
        sizeof(uint64_t);
    const uintptr_t before_guard_paddr =
        src_base_paddr - sizeof(uint64_t);
    const uintptr_t after_guard_paddr =
        src_base_paddr + AI_MEMBLOCK_CMO_AMO_BLOCK_WORDS *
        sizeof(uint64_t);
    const uintptr_t marker_vaddr = hs_page_base(marker_page) + 0x600U;
    const uintptr_t marker_paddr = phys_page_base(marker_page) + 0x600U;
    const uintptr_t marker_guard_vaddr = marker_vaddr + sizeof(uint64_t);
    const uintptr_t marker_guard_paddr = marker_paddr + sizeof(uint64_t);
    const uint64_t block_seeds[] = {
        0x1a2b3c4d5e6f7081ULL,
        0x2b3c4d5e6f708192ULL,
        0x3c4d5e6f708192a3ULL,
    };
    const uint64_t word1_updates[] = {
        0xf0e1d2c3b4a59687ULL,
        0x1029384756abcdefULL,
        0x8877665544332211ULL,
    };
    const uint64_t word2_updates[] = {
        0x0f1e2d3c4b5a6978ULL,
        0xfedcba9876543210ULL,
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
        0x0102030405060708ULL,
        0x1827364554637281ULL,
        0xfedcba9876543210ULL,
    };
    const uint64_t marker_updates[] = {
        0x5a5aa5a5c3c33c3cULL,
        0x69966996d2d22d2dULL,
        0x78877887e1e11e1eULL,
    };
    const uint64_t marker_guards[] = {
        0x3141592653589793ULL,
        0x2718281828459045ULL,
        0x1123581321345589ULL,
    };

    ai_memblock_cmo_amo_prepare_hs_env();

    for (size_t round = 0; round < AI_MEMBLOCK_FLUSH_ARRAY_SIZE(block_seeds);
         ++round) {
        const uint64_t expected_cross16 =
            ai_memblock_cmo_cross16_load_expected(word1_updates[round],
                word2_updates[round]);

        goto_priv(PRIV_M);
        ai_memblock_cmo_amo_seed_block(src_base_paddr, block_seeds[round]);
        write64(before_guard_paddr, before_guards[round]);
        write64(after_guard_paddr, after_guards[round]);
        write64(marker_paddr, marker_seeds[round]);
        write64(marker_guard_paddr, marker_guards[round]);

        goto_priv(PRIV_HS);
        TEST_SETUP_EXCEPT();
        const uint64_t warmed_src = ld(src_base_vaddr);
        const uint64_t warmed_word1 = ld(src_word1_vaddr);
        const uint64_t warmed_word2 = ld(src_word2_vaddr);
        const uint64_t warmed_tail = ld(src_tail_vaddr);
        const uint64_t warmed_marker = ld(marker_vaddr);
        AI_ASSERT_NO_EXCEPT_AND("before the cross-16B load plus CBOM probe starts, source block and marker are readable with seeded values",
            warmed_src == block_seeds[round] &&
            warmed_word1 == block_seeds[round] + 0x1111111111111111ULL &&
            warmed_word2 == block_seeds[round] + 2U *
                0x1111111111111111ULL &&
            warmed_tail == block_seeds[round] +
                (AI_MEMBLOCK_CMO_AMO_BLOCK_WORDS - 1U) *
                0x1111111111111111ULL &&
            warmed_marker == marker_seeds[round]
        );

        TEST_SETUP_EXCEPT();
        sd(src_word1_vaddr, word1_updates[round]);
        sd(src_word2_vaddr, word2_updates[round]);
        const uint64_t cross16_observed =
            ai_memblock_cmo_cross16_load_cbom_then_marker(
                cross16_load_vaddr,
                src_base_vaddr,
                src_tail_vaddr,
                marker_vaddr,
                marker_updates[round]);
        const uint64_t observed_word1 = ld(src_word1_vaddr);
        const uint64_t observed_word2 = ld(src_word2_vaddr);
        const uint64_t observed_tail = ld(src_tail_vaddr);
        const uint64_t observed_marker = ld(marker_vaddr);
        AI_ASSERT_NO_EXCEPT_AND("same-block cross-16B load immediately followed by CBOM clean/flush keeps split-load merge and CMO flush ownership ordered so the load sees current payload and the block is preserved",
            cross16_observed == expected_cross16 &&
            observed_word1 == word1_updates[round] &&
            observed_word2 == word2_updates[round] &&
            observed_tail == block_seeds[round] +
                (AI_MEMBLOCK_CMO_AMO_BLOCK_WORDS - 1U) *
                0x1111111111111111ULL &&
            observed_marker == marker_updates[round]
        );

        TEST_SETUP_EXCEPT();
        AI_ASSERT_NO_EXCEPT_AND("the cross-16B load plus CBOM owner-window does not corrupt source or marker guards",
            ld(before_guard_vaddr) == before_guards[round] &&
            ld(after_guard_vaddr) == after_guards[round] &&
            ld(marker_guard_vaddr) == marker_guards[round]
        );

        goto_priv(PRIV_M);
        TEST_ASSERT("backing memory after the cross-16B load plus CBOM owner-window contains the two source stores, marker update, and unchanged guards",
            ai_memblock_cmo_amo_block_matches_two_updates(
                src_base_paddr,
                block_seeds[round],
                1U,
                word1_updates[round],
                2U,
                word2_updates[round]) &&
            read64(before_guard_paddr) == before_guards[round] &&
            read64(after_guard_paddr) == after_guards[round] &&
            read64(marker_paddr) == marker_updates[round] &&
            read64(marker_guard_paddr) == marker_guards[round]
        );
    }

    ai_memblock_cmo_amo_reset_hs_env();

    TEST_END();
}

bool ai_micro_hs_cross16_load_then_cboinval_no_stale_merge_or_inval_owner_corner()
{
    TEST_START();

    const enum test_page src_page = SWITCH1;
    const enum test_page marker_page = SWITCH2;
    const uintptr_t src_base_vaddr = hs_page_base(src_page) + 0xa40U;
    const uintptr_t src_base_paddr = phys_page_base(src_page) + 0xa40U;
    const uintptr_t src_word1_vaddr = src_base_vaddr + sizeof(uint64_t);
    const uintptr_t src_word2_vaddr =
        src_base_vaddr + 2U * sizeof(uint64_t);
    const uintptr_t cross16_load_vaddr = src_base_vaddr + 15U;
    const uintptr_t inval_vaddr = src_base_vaddr + 5U * sizeof(uint64_t);
    const uintptr_t src_tail_vaddr =
        src_base_vaddr +
        (AI_MEMBLOCK_CMO_AMO_BLOCK_WORDS - 1U) * sizeof(uint64_t);
    const uintptr_t before_guard_vaddr = src_base_vaddr - sizeof(uint64_t);
    const uintptr_t after_guard_vaddr =
        src_base_vaddr + AI_MEMBLOCK_CMO_AMO_BLOCK_WORDS *
        sizeof(uint64_t);
    const uintptr_t before_guard_paddr =
        src_base_paddr - sizeof(uint64_t);
    const uintptr_t after_guard_paddr =
        src_base_paddr + AI_MEMBLOCK_CMO_AMO_BLOCK_WORDS *
        sizeof(uint64_t);
    const uintptr_t marker_vaddr = hs_page_base(marker_page) + 0xa40U;
    const uintptr_t marker_paddr = phys_page_base(marker_page) + 0xa40U;
    const uintptr_t marker_guard_vaddr = marker_vaddr + sizeof(uint64_t);
    const uintptr_t marker_guard_paddr = marker_paddr + sizeof(uint64_t);
    const uint64_t block_seeds[] = {
        0x2435465768798a9bULL,
        0x35465768798a9bacULL,
        0x465768798a9bacbdULL,
    };
    const uint64_t word1_updates[] = {
        0x0f1e2d3c4b5a6978ULL,
        0xfedcba9876543210ULL,
        0x8877665544332211ULL,
    };
    const uint64_t word2_updates[] = {
        0xf0e1d2c3b4a59687ULL,
        0x1029384756abcdefULL,
        0x1123581321345589ULL,
    };
    const uint64_t before_guards[] = {
        0x123456789abcdef0ULL,
        0x21436587a9cbed0fULL,
        0x3141592653589793ULL,
    };
    const uint64_t after_guards[] = {
        0xfeedfacecafebeefULL,
        0xdeadc0def00dbabeULL,
        0xfaceb00c13579bdfULL,
    };
    const uint64_t marker_seeds[] = {
        0x0102030405060708ULL,
        0x1827364554637281ULL,
        0xfedcba9876543210ULL,
    };
    const uint64_t marker_updates[] = {
        0x89abcdef01234567ULL,
        0x76543210fedcba98ULL,
        0x55aa55aa33cc33ccULL,
    };
    const uint64_t marker_guards[] = {
        0x3141592653589793ULL,
        0x2718281828459045ULL,
        0x1123581321345589ULL,
    };

    ai_memblock_cmo_inval_prepare_hs_env();

    for (size_t round = 0; round < AI_MEMBLOCK_FLUSH_ARRAY_SIZE(block_seeds);
         ++round) {
        const uint64_t seed_word1 =
            block_seeds[round] + 0x1111111111111111ULL;
        const uint64_t seed_word2 =
            block_seeds[round] + 2U * 0x1111111111111111ULL;
        const uint64_t expected_tail =
            block_seeds[round] +
            (AI_MEMBLOCK_CMO_AMO_BLOCK_WORDS - 1U) *
            0x1111111111111111ULL;
        const uint64_t expected_cross16 =
            ai_memblock_cmo_cross16_load_expected(word1_updates[round],
                word2_updates[round]);

        goto_priv(PRIV_M);
        ai_memblock_cmo_amo_seed_block(src_base_paddr, block_seeds[round]);
        write64(before_guard_paddr, before_guards[round]);
        write64(after_guard_paddr, after_guards[round]);
        write64(marker_paddr, marker_seeds[round]);
        write64(marker_guard_paddr, marker_guards[round]);

        goto_priv(PRIV_HS);
        TEST_SETUP_EXCEPT();
        const uint64_t warmed_src = ld(src_base_vaddr);
        const uint64_t warmed_word1 = ld(src_word1_vaddr);
        const uint64_t warmed_word2 = ld(src_word2_vaddr);
        const uint64_t warmed_tail = ld(src_tail_vaddr);
        const uint64_t warmed_marker = ld(marker_vaddr);
        AI_ASSERT_NO_EXCEPT_AND("before the cross-16B load plus CBO.INVAL probe starts, source block and marker are readable with seeded values",
            warmed_src == block_seeds[round] &&
            warmed_word1 == seed_word1 &&
            warmed_word2 == seed_word2 &&
            warmed_tail == expected_tail &&
            warmed_marker == marker_seeds[round]
        );

        TEST_SETUP_EXCEPT();
        sd(src_word1_vaddr, word1_updates[round]);
        sd(src_word2_vaddr, word2_updates[round]);
        const uint64_t cross16_observed =
            ai_memblock_cmo_cross16_load_cboinval_then_marker(
                cross16_load_vaddr,
                inval_vaddr,
                marker_vaddr,
                marker_updates[round]);
        const uint64_t observed_src = ld(src_base_vaddr);
        const uint64_t observed_word1 = ld(src_word1_vaddr);
        const uint64_t observed_word2 = ld(src_word2_vaddr);
        const uint64_t observed_tail = ld(src_tail_vaddr);
        const uint64_t observed_marker = ld(marker_vaddr);
        AI_ASSERT_NO_EXCEPT_AND("same-block cross-16B load immediately followed by CBO.INVAL keeps split-load merge and inval ownership ordered so the load sees current payload and marker progresses",
            cross16_observed == expected_cross16 &&
            observed_src == block_seeds[round] &&
            observed_word1 == word1_updates[round] &&
            observed_word2 == word2_updates[round] &&
            observed_tail == expected_tail &&
            observed_marker == marker_updates[round]
        );

        TEST_SETUP_EXCEPT();
        AI_ASSERT_NO_EXCEPT_AND("the cross-16B load plus CBO.INVAL owner-window does not corrupt source or marker guards",
            ld(before_guard_vaddr) == before_guards[round] &&
            ld(after_guard_vaddr) == after_guards[round] &&
            ld(marker_guard_vaddr) == marker_guards[round]
        );

        goto_priv(PRIV_M);
        TEST_ASSERT("backing memory after the cross-16B load plus CBO.INVAL owner-window contains the two source stores, marker update, and unchanged guards",
            ai_memblock_cmo_amo_block_matches_two_updates(
                src_base_paddr,
                block_seeds[round],
                1U,
                word1_updates[round],
                2U,
                word2_updates[round]) &&
            read64(before_guard_paddr) == before_guards[round] &&
            read64(after_guard_paddr) == after_guards[round] &&
            read64(marker_paddr) == marker_updates[round] &&
            read64(marker_guard_paddr) == marker_guards[round]
        );
    }

    ai_memblock_cmo_inval_reset_hs_env();

    TEST_END();
}
