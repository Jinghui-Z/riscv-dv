#include "ai_micro_memblock_flush_atomic_common.h"

bool ai_micro_hs_cross16_load_then_amo_no_stale_merge_or_atomic_owner_corner()
{
    TEST_START();

    const enum test_page src_page = SWITCH1;
    const enum test_page marker_page = SWITCH2;
    const uintptr_t src_base_vaddr = hs_page_base(src_page) + 0x700U;
    const uintptr_t src_base_paddr = phys_page_base(src_page) + 0x700U;
    const uintptr_t src_word1_vaddr = src_base_vaddr + sizeof(uint64_t);
    const uintptr_t src_word2_vaddr = src_base_vaddr + 2U * sizeof(uint64_t);
    const uintptr_t amo_target_vaddr =
        src_base_vaddr + 3U * sizeof(uint64_t);
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
    const uintptr_t marker_vaddr = hs_page_base(marker_page) + 0x700U;
    const uintptr_t marker_paddr = phys_page_base(marker_page) + 0x700U;
    const uintptr_t marker_guard_vaddr = marker_vaddr + sizeof(uint64_t);
    const uintptr_t marker_guard_paddr = marker_paddr + sizeof(uint64_t);
    const uint64_t block_seeds[] = {
        0x1020304050607080ULL,
        0x2131415161718191ULL,
        0x32425262728292a2ULL,
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
        const uint64_t seed_amo_target =
            block_seeds[round] + 3U * 0x1111111111111111ULL;
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
        AI_ASSERT_NO_EXCEPT_AND("before the cross-16B load plus AMO probe starts, source block and marker are readable with seeded values",
            warmed_src == block_seeds[round] &&
            warmed_word1 == block_seeds[round] + 0x1111111111111111ULL &&
            warmed_word2 == block_seeds[round] + 2U *
                0x1111111111111111ULL &&
            warmed_amo_target == seed_amo_target &&
            warmed_tail == block_seeds[round] +
                (AI_MEMBLOCK_CMO_AMO_BLOCK_WORDS - 1U) *
                0x1111111111111111ULL &&
            warmed_marker == marker_seeds[round]
        );

        TEST_SETUP_EXCEPT();
        sd(src_word1_vaddr, word1_updates[round]);
        sd(src_word2_vaddr, word2_updates[round]);
        uint64_t cross16_observed = 0ULL;
        uint64_t amo_old_observed = 0ULL;
        ai_memblock_cmo_cross16_load_amo_then_marker(
            cross16_load_vaddr,
            amo_target_vaddr,
            amo_addends[round],
            marker_vaddr,
            marker_updates[round],
            &cross16_observed,
            &amo_old_observed);
        const uint64_t observed_word1 = ld(src_word1_vaddr);
        const uint64_t observed_word2 = ld(src_word2_vaddr);
        const uint64_t observed_amo_target = ld(amo_target_vaddr);
        const uint64_t observed_tail = ld(src_tail_vaddr);
        const uint64_t observed_marker = ld(marker_vaddr);
        AI_ASSERT_NO_EXCEPT_AND("same-block cross-16B load immediately followed by AMO keeps split-load merge and atomic flush ownership ordered so the load sees current payload and AMO updates the current target",
            cross16_observed == expected_cross16 &&
            amo_old_observed == seed_amo_target &&
            observed_word1 == word1_updates[round] &&
            observed_word2 == word2_updates[round] &&
            observed_amo_target == expected_amo_target &&
            observed_tail == block_seeds[round] +
                (AI_MEMBLOCK_CMO_AMO_BLOCK_WORDS - 1U) *
                0x1111111111111111ULL &&
            observed_marker == marker_updates[round]
        );

        TEST_SETUP_EXCEPT();
        AI_ASSERT_NO_EXCEPT_AND("the cross-16B load plus AMO owner-window does not corrupt source or marker guards",
            ld(before_guard_vaddr) == before_guards[round] &&
            ld(after_guard_vaddr) == after_guards[round] &&
            ld(marker_guard_vaddr) == marker_guards[round]
        );

        goto_priv(PRIV_M);
        TEST_ASSERT("backing memory after the cross-16B load plus AMO owner-window contains the two source stores, AMO update, marker update, and unchanged guards",
            ai_memblock_cmo_amo_block_matches_three_updates(
                src_base_paddr,
                block_seeds[round],
                1U,
                word1_updates[round],
                2U,
                word2_updates[round],
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

bool ai_micro_hs_cross16_load_then_lrsc_no_stale_merge_or_reservation_owner_corner()
{
    TEST_START();

    const enum test_page src_page = SWITCH1;
    const enum test_page marker_page = SWITCH2;
    const uintptr_t src_base_vaddr = hs_page_base(src_page) + 0x800U;
    const uintptr_t src_base_paddr = phys_page_base(src_page) + 0x800U;
    const uintptr_t src_word1_vaddr = src_base_vaddr + sizeof(uint64_t);
    const uintptr_t src_word2_vaddr = src_base_vaddr + 2U * sizeof(uint64_t);
    const uintptr_t lrsc_target_vaddr =
        src_base_vaddr + 3U * sizeof(uint64_t);
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
    const uintptr_t marker_vaddr = hs_page_base(marker_page) + 0x800U;
    const uintptr_t marker_paddr = phys_page_base(marker_page) + 0x800U;
    const uintptr_t marker_guard_vaddr = marker_vaddr + sizeof(uint64_t);
    const uintptr_t marker_guard_paddr = marker_paddr + sizeof(uint64_t);
    const uint64_t block_seeds[] = {
        0x1122334455667788ULL,
        0x2233445566778899ULL,
        0x33445566778899aaULL,
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
        const uint64_t expected_cross16 =
            ai_memblock_cmo_cross16_load_expected(word1_updates[round],
                word2_updates[round]);
        const uint64_t seed_lrsc_target =
            block_seeds[round] + 3U * 0x1111111111111111ULL;

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
        AI_ASSERT_NO_EXCEPT_AND("before the cross-16B load plus LR/SC probe starts, source block and marker are readable with seeded values",
            warmed_src == block_seeds[round] &&
            warmed_word1 == block_seeds[round] + 0x1111111111111111ULL &&
            warmed_word2 == block_seeds[round] + 2U *
                0x1111111111111111ULL &&
            warmed_lrsc_target == seed_lrsc_target &&
            warmed_tail == block_seeds[round] +
                (AI_MEMBLOCK_CMO_AMO_BLOCK_WORDS - 1U) *
                0x1111111111111111ULL &&
            warmed_marker == marker_seeds[round]
        );

        TEST_SETUP_EXCEPT();
        sd(src_word1_vaddr, word1_updates[round]);
        sd(src_word2_vaddr, word2_updates[round]);
        uint64_t cross16_observed = 0ULL;
        uint64_t lr_observed = 0ULL;
        uint64_t sc_status = 1ULL;
        ai_memblock_cmo_cross16_load_lrsc_then_marker(
            cross16_load_vaddr,
            lrsc_target_vaddr,
            sc_payloads[round],
            marker_vaddr,
            marker_updates[round],
            &cross16_observed,
            &lr_observed,
            &sc_status);
        const uint64_t observed_word1 = ld(src_word1_vaddr);
        const uint64_t observed_word2 = ld(src_word2_vaddr);
        const uint64_t observed_lrsc_target = ld(lrsc_target_vaddr);
        const uint64_t observed_tail = ld(src_tail_vaddr);
        const uint64_t observed_marker = ld(marker_vaddr);
        AI_ASSERT_NO_EXCEPT_AND("same-block cross-16B load immediately followed by LR/SC keeps split-load merge and reservation ownership ordered so the load sees current payload and SC succeeds",
            cross16_observed == expected_cross16 &&
            lr_observed == seed_lrsc_target &&
            sc_status == 0ULL &&
            observed_word1 == word1_updates[round] &&
            observed_word2 == word2_updates[round] &&
            observed_lrsc_target == sc_payloads[round] &&
            observed_tail == block_seeds[round] +
                (AI_MEMBLOCK_CMO_AMO_BLOCK_WORDS - 1U) *
                0x1111111111111111ULL &&
            observed_marker == marker_updates[round]
        );

        TEST_SETUP_EXCEPT();
        AI_ASSERT_NO_EXCEPT_AND("the cross-16B load plus LR/SC owner-window does not corrupt source or marker guards",
            ld(before_guard_vaddr) == before_guards[round] &&
            ld(after_guard_vaddr) == after_guards[round] &&
            ld(marker_guard_vaddr) == marker_guards[round]
        );

        goto_priv(PRIV_M);
        TEST_ASSERT("backing memory after the cross-16B load plus LR/SC owner-window contains the two source stores, successful SC payload, marker update, and unchanged guards",
            ai_memblock_cmo_amo_block_matches_three_updates(
                src_base_paddr,
                block_seeds[round],
                1U,
                word1_updates[round],
                2U,
                word2_updates[round],
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

bool ai_micro_hs_cbozero_then_cross16_store_no_stale_zero_flush_owner_or_partial_payload_corner()
{
    TEST_START();

    const enum test_page src_page = SWITCH1;
    const enum test_page marker_page = SWITCH2;
    const uintptr_t src_base_vaddr = hs_page_base(src_page) + 0x200U;
    const uintptr_t src_base_paddr = phys_page_base(src_page) + 0x200U;
    const uintptr_t zero_subblock_vaddr =
        src_base_vaddr + 4U * sizeof(uint64_t);
    const uintptr_t cross16_store_vaddr = src_base_vaddr + 15U;
    const uintptr_t src_word1_vaddr = src_base_vaddr + sizeof(uint64_t);
    const uintptr_t src_word2_vaddr = src_base_vaddr + 2U * sizeof(uint64_t);
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
    const uintptr_t marker_vaddr = hs_page_base(marker_page) + 0x200U;
    const uintptr_t marker_paddr = phys_page_base(marker_page) + 0x200U;
    const uintptr_t marker_guard_vaddr = marker_vaddr + sizeof(uint64_t);
    const uintptr_t marker_guard_paddr = marker_paddr + sizeof(uint64_t);
    const uint64_t block_seeds[] = {
        0x1021324354657687ULL,
        0x2132435465768798ULL,
        0x31425364758697a8ULL,
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
        0x8877665544332211ULL,
        0x7766554433221100ULL,
        0x66554433221100ffULL,
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
        const uint64_t expected_word1 =
            ai_memblock_cmo_cross16_store_low_word(store_payloads[round]);
        const uint64_t expected_word2 =
            ai_memblock_cmo_cross16_store_high_word(store_payloads[round]);

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
        AI_ASSERT_NO_EXCEPT_AND("before the CBO.ZERO plus cross-16B store probe starts, source block and marker are readable with seeded values",
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
        cbo_zero(zero_subblock_vaddr);
        ai_memblock_cmo_cross16_store_then_marker(cross16_store_vaddr,
            store_payloads[round], marker_vaddr, marker_updates[round]);
        const uint64_t observed_src = ld(src_base_vaddr);
        const uint64_t observed_word1 = ld(src_word1_vaddr);
        const uint64_t observed_word2 = ld(src_word2_vaddr);
        const uint64_t observed_tail = ld(src_tail_vaddr);
        const uint64_t observed_marker = ld(marker_vaddr);
        AI_ASSERT_NO_EXCEPT_AND("CBO.ZERO immediately followed by a same-block cross-16B store keeps zero flush ownership separate so only the split-store byte lanes survive",
            observed_src == 0ULL &&
            observed_word1 == expected_word1 &&
            observed_word2 == expected_word2 &&
            observed_tail == 0ULL &&
            observed_marker == marker_updates[round]
        );

        TEST_SETUP_EXCEPT();
        AI_ASSERT_NO_EXCEPT_AND("the CBO.ZERO plus cross-16B store owner-window does not corrupt source or marker guards",
            ld(before_guard_vaddr) == before_guards[round] &&
            ld(after_guard_vaddr) == after_guards[round] &&
            ld(marker_guard_vaddr) == marker_guards[round]
        );

        goto_priv(PRIV_M);
        TEST_ASSERT("backing memory after the CBO.ZERO plus cross-16B store owner-window contains zero block with only the split-store byte overlay, marker update, and unchanged guards",
            ai_memblock_cmo_zero_block_matches_cross16_store(src_base_paddr,
                store_payloads[round]) &&
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

bool ai_micro_hs_cbozero_then_lrsc_no_stale_zero_flush_owner_or_reservation_loss_corner()
{
    TEST_START();

    const enum test_page target_page = SWITCH1;
    const enum test_page marker_page = SWITCH2;
    const uintptr_t target_base_vaddr = hs_page_base(target_page) + 0x300U;
    const uintptr_t target_base_paddr = phys_page_base(target_page) + 0x300U;
    const uintptr_t zero_subblock_vaddr =
        target_base_vaddr + 4U * sizeof(uint64_t);
    const uintptr_t lrsc_vaddr = target_base_vaddr;
    const uintptr_t target_tail_vaddr =
        target_base_vaddr +
        (AI_MEMBLOCK_CMO_AMO_BLOCK_WORDS - 1U) * sizeof(uint64_t);
    const uintptr_t before_guard_vaddr =
        target_base_vaddr - sizeof(uint64_t);
    const uintptr_t after_guard_vaddr =
        target_base_vaddr + AI_MEMBLOCK_CMO_AMO_BLOCK_WORDS *
        sizeof(uint64_t);
    const uintptr_t before_guard_paddr =
        target_base_paddr - sizeof(uint64_t);
    const uintptr_t after_guard_paddr =
        target_base_paddr + AI_MEMBLOCK_CMO_AMO_BLOCK_WORDS *
        sizeof(uint64_t);
    const uintptr_t marker_vaddr = hs_page_base(marker_page) + 0x300U;
    const uintptr_t marker_paddr = phys_page_base(marker_page) + 0x300U;
    const uintptr_t marker_guard_vaddr = marker_vaddr + sizeof(uint64_t);
    const uintptr_t marker_guard_paddr = marker_paddr + sizeof(uint64_t);
    const uint64_t block_seeds[] = {
        0x1020304050607080ULL,
        0x21436587a9cbed0fULL,
        0x3141592653589793ULL,
    };
    const uint64_t sc_payloads[] = {
        0x0123456789abcdefULL,
        0xfedcba9876543210ULL,
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
        0x13579bdf2468ace0ULL,
        0x02468ace13579bdfULL,
        0xfdb97531eca86420ULL,
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
        goto_priv(PRIV_M);
        ai_memblock_cmo_amo_seed_block(target_base_paddr, block_seeds[round]);
        write64(before_guard_paddr, before_guards[round]);
        write64(after_guard_paddr, after_guards[round]);
        write64(marker_paddr, marker_seeds[round]);
        write64(marker_guard_paddr, marker_guards[round]);

        goto_priv(PRIV_HS);
        TEST_SETUP_EXCEPT();
        const uint64_t warmed_head = ld(target_base_vaddr);
        const uint64_t warmed_tail = ld(target_tail_vaddr);
        const uint64_t warmed_marker = ld(marker_vaddr);
        AI_ASSERT_NO_EXCEPT_AND("before the CBO.ZERO plus LR/SC probe starts, target block and marker are readable with seeded values",
            warmed_head == block_seeds[round] &&
            warmed_tail == block_seeds[round] +
                (AI_MEMBLOCK_CMO_AMO_BLOCK_WORDS - 1U) *
                0x1111111111111111ULL &&
            warmed_marker == marker_seeds[round]
        );

        TEST_SETUP_EXCEPT();
        uint64_t lr_observed = 0xffffffffffffffffULL;
        uint64_t sc_status = 1ULL;
        cbo_zero(zero_subblock_vaddr);
        ai_memblock_cmo_lrsc_then_marker(lrsc_vaddr, sc_payloads[round],
            marker_vaddr, marker_updates[round], &lr_observed, &sc_status);
        const uint64_t observed_head = ld(target_base_vaddr);
        const uint64_t observed_tail = ld(target_tail_vaddr);
        const uint64_t observed_marker = ld(marker_vaddr);
        AI_ASSERT_NO_EXCEPT_AND("CBO.ZERO immediately followed by LR/SC keeps zero flush ownership separate so LR sees zero and SC keeps the reservation",
            lr_observed == 0ULL &&
            sc_status == 0ULL &&
            observed_head == sc_payloads[round] &&
            observed_tail == 0ULL &&
            observed_marker == marker_updates[round]
        );

        TEST_SETUP_EXCEPT();
        AI_ASSERT_NO_EXCEPT_AND("the CBO.ZERO plus LR/SC owner-window does not corrupt target or marker guards",
            ld(before_guard_vaddr) == before_guards[round] &&
            ld(after_guard_vaddr) == after_guards[round] &&
            ld(marker_guard_vaddr) == marker_guards[round]
        );

        goto_priv(PRIV_M);
        TEST_ASSERT("backing memory after the CBO.ZERO plus LR/SC owner-window contains one successful SC payload over an otherwise zeroed block, marker update, and unchanged guards",
            ai_memblock_cmo_amo_zero_block_matches_two_updates(
                target_base_paddr, 0U, sc_payloads[round],
                AI_MEMBLOCK_CMO_AMO_BLOCK_WORDS, 0ULL) &&
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

bool ai_micro_hs_amo_then_cbozero_no_stale_atomic_flush_owner_or_zero_drop_corner()
{
    TEST_START();

    const enum test_page target_page = SWITCH1;
    const enum test_page marker_page = SWITCH2;
    const uintptr_t target_base_vaddr = hs_page_base(target_page) + 0x400U;
    const uintptr_t target_base_paddr = phys_page_base(target_page) + 0x400U;
    const uintptr_t zero_subblock_vaddr =
        target_base_vaddr + 5U * sizeof(uint64_t);
    const uintptr_t target_tail_vaddr =
        target_base_vaddr +
        (AI_MEMBLOCK_CMO_AMO_BLOCK_WORDS - 1U) * sizeof(uint64_t);
    const uintptr_t before_guard_vaddr =
        target_base_vaddr - sizeof(uint64_t);
    const uintptr_t after_guard_vaddr =
        target_base_vaddr + AI_MEMBLOCK_CMO_AMO_BLOCK_WORDS *
        sizeof(uint64_t);
    const uintptr_t before_guard_paddr =
        target_base_paddr - sizeof(uint64_t);
    const uintptr_t after_guard_paddr =
        target_base_paddr + AI_MEMBLOCK_CMO_AMO_BLOCK_WORDS *
        sizeof(uint64_t);
    const uintptr_t marker_vaddr = hs_page_base(marker_page) + 0x400U;
    const uintptr_t marker_paddr = phys_page_base(marker_page) + 0x400U;
    const uintptr_t marker_guard_vaddr = marker_vaddr + sizeof(uint64_t);
    const uintptr_t marker_guard_paddr = marker_paddr + sizeof(uint64_t);
    const uint64_t block_seeds[] = {
        0x13579bdf2468ace0ULL,
        0x02468ace13579bdfULL,
        0xfdb97531eca86420ULL,
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
        0x8877665544332211ULL,
        0x7766554433221100ULL,
        0x66554433221100ffULL,
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
        goto_priv(PRIV_M);
        ai_memblock_cmo_amo_seed_block(target_base_paddr, block_seeds[round]);
        write64(before_guard_paddr, before_guards[round]);
        write64(after_guard_paddr, after_guards[round]);
        write64(marker_paddr, marker_seeds[round]);
        write64(marker_guard_paddr, marker_guards[round]);

        goto_priv(PRIV_HS);
        TEST_SETUP_EXCEPT();
        const uint64_t warmed_head = ld(target_base_vaddr);
        const uint64_t warmed_tail = ld(target_tail_vaddr);
        const uint64_t warmed_marker = ld(marker_vaddr);
        AI_ASSERT_NO_EXCEPT_AND("before the AMO plus CBO.ZERO flush-owner probe starts, target block and marker are readable with seeded values",
            warmed_head == block_seeds[round] &&
            warmed_tail == block_seeds[round] +
                (AI_MEMBLOCK_CMO_AMO_BLOCK_WORDS - 1U) *
                0x1111111111111111ULL &&
            warmed_marker == marker_seeds[round]
        );

        TEST_SETUP_EXCEPT();
        const uint64_t amo_old = amoadd_d(target_base_vaddr,
            amo_addends[round]);
        cbo_zero(zero_subblock_vaddr);
        sd(marker_vaddr, marker_updates[round]);
        fence_iorw_iorw();
        const uint64_t observed_head = ld(target_base_vaddr);
        const uint64_t observed_tail = ld(target_tail_vaddr);
        const uint64_t observed_marker = ld(marker_vaddr);
        AI_ASSERT_NO_EXCEPT_AND("AMO immediately followed by CBO.ZERO keeps flush ownership ordered so AMO reads the pre-zero seed and CBO.ZERO still clears the full block",
            amo_old == block_seeds[round] &&
            observed_head == 0ULL &&
            observed_tail == 0ULL &&
            observed_marker == marker_updates[round]
        );

        TEST_SETUP_EXCEPT();
        AI_ASSERT_NO_EXCEPT_AND("the AMO plus CBO.ZERO owner-window does not corrupt target or marker guards",
            ld(before_guard_vaddr) == before_guards[round] &&
            ld(after_guard_vaddr) == after_guards[round] &&
            ld(marker_guard_vaddr) == marker_guards[round]
        );

        goto_priv(PRIV_M);
        TEST_ASSERT("backing memory after the AMO plus CBO.ZERO owner-window contains a fully zeroed target block, marker update, and unchanged guards",
            ai_memblock_cmo_amo_block_all_zero(target_base_paddr) &&
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

bool ai_micro_hs_lrsc_then_cbozero_no_stale_reservation_or_zero_drop_corner()
{
    TEST_START();

    const enum test_page target_page = SWITCH1;
    const enum test_page marker_page = SWITCH2;
    const uintptr_t target_base_vaddr = hs_page_base(target_page) + 0x500U;
    const uintptr_t target_base_paddr = phys_page_base(target_page) + 0x500U;
    const uintptr_t zero_subblock_vaddr =
        target_base_vaddr + 6U * sizeof(uint64_t);
    const uintptr_t target_tail_vaddr =
        target_base_vaddr +
        (AI_MEMBLOCK_CMO_AMO_BLOCK_WORDS - 1U) * sizeof(uint64_t);
    const uintptr_t before_guard_vaddr =
        target_base_vaddr - sizeof(uint64_t);
    const uintptr_t after_guard_vaddr =
        target_base_vaddr + AI_MEMBLOCK_CMO_AMO_BLOCK_WORDS *
        sizeof(uint64_t);
    const uintptr_t before_guard_paddr =
        target_base_paddr - sizeof(uint64_t);
    const uintptr_t after_guard_paddr =
        target_base_paddr + AI_MEMBLOCK_CMO_AMO_BLOCK_WORDS *
        sizeof(uint64_t);
    const uintptr_t marker_vaddr = hs_page_base(marker_page) + 0x500U;
    const uintptr_t marker_paddr = phys_page_base(marker_page) + 0x500U;
    const uintptr_t marker_guard_vaddr = marker_vaddr + sizeof(uint64_t);
    const uintptr_t marker_guard_paddr = marker_paddr + sizeof(uint64_t);
    const uint64_t block_seeds[] = {
        0x0123456789abcdefULL,
        0xfedcba9876543210ULL,
        0x0f1e2d3c4b5a6978ULL,
    };
    const uint64_t sc_payloads[] = {
        0x1111222233334444ULL,
        0x5555666677778888ULL,
        0x9999aaaabbbbccccULL,
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
        0x13579bdf2468ace0ULL,
        0x02468ace13579bdfULL,
        0xfdb97531eca86420ULL,
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
        goto_priv(PRIV_M);
        ai_memblock_cmo_amo_seed_block(target_base_paddr, block_seeds[round]);
        write64(before_guard_paddr, before_guards[round]);
        write64(after_guard_paddr, after_guards[round]);
        write64(marker_paddr, marker_seeds[round]);
        write64(marker_guard_paddr, marker_guards[round]);

        goto_priv(PRIV_HS);
        TEST_SETUP_EXCEPT();
        const uint64_t warmed_head = ld(target_base_vaddr);
        const uint64_t warmed_tail = ld(target_tail_vaddr);
        const uint64_t warmed_marker = ld(marker_vaddr);
        AI_ASSERT_NO_EXCEPT_AND("before the LR/SC plus CBO.ZERO probe starts, target block and marker are readable with seeded values",
            warmed_head == block_seeds[round] &&
            warmed_tail == block_seeds[round] +
                (AI_MEMBLOCK_CMO_AMO_BLOCK_WORDS - 1U) *
                0x1111111111111111ULL &&
            warmed_marker == marker_seeds[round]
        );

        TEST_SETUP_EXCEPT();
        uint64_t lr_observed = 0xffffffffffffffffULL;
        uint64_t sc_status = 1ULL;
        ai_memblock_cmo_lrsc_only(target_base_vaddr, sc_payloads[round],
            &lr_observed, &sc_status);
        cbo_zero(zero_subblock_vaddr);
        sd(marker_vaddr, marker_updates[round]);
        fence_iorw_iorw();
        const uint64_t observed_head = ld(target_base_vaddr);
        const uint64_t observed_tail = ld(target_tail_vaddr);
        const uint64_t observed_marker = ld(marker_vaddr);
        AI_ASSERT_NO_EXCEPT_AND("LR/SC immediately followed by CBO.ZERO keeps the reservation result ordered before the zero flush clears the full block",
            lr_observed == block_seeds[round] &&
            sc_status == 0ULL &&
            observed_head == 0ULL &&
            observed_tail == 0ULL &&
            observed_marker == marker_updates[round]
        );

        TEST_SETUP_EXCEPT();
        AI_ASSERT_NO_EXCEPT_AND("the LR/SC plus CBO.ZERO owner-window does not corrupt target or marker guards",
            ld(before_guard_vaddr) == before_guards[round] &&
            ld(after_guard_vaddr) == after_guards[round] &&
            ld(marker_guard_vaddr) == marker_guards[round]
        );

        goto_priv(PRIV_M);
        TEST_ASSERT("backing memory after the LR/SC plus CBO.ZERO owner-window contains a fully zeroed target block, marker update, and unchanged guards",
            ai_memblock_cmo_amo_block_all_zero(target_base_paddr) &&
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

bool ai_micro_hs_cbom_then_lrsc_same_block_no_stale_cmo_flush_owner_or_sc_loss_corner()
{
    TEST_START();

    const enum test_page target_page = SWITCH1;
    const enum test_page marker_page = SWITCH2;
    const uintptr_t target_base_vaddr = hs_page_base(target_page) + 0x600U;
    const uintptr_t target_base_paddr = phys_page_base(target_page) + 0x600U;
    const uintptr_t target_word_vaddr = target_base_vaddr;
    const uintptr_t neighbor_word_vaddr =
        target_base_vaddr + 3U * sizeof(uint64_t);
    const uintptr_t target_tail_vaddr =
        target_base_vaddr +
        (AI_MEMBLOCK_CMO_AMO_BLOCK_WORDS - 1U) * sizeof(uint64_t);
    const uintptr_t before_guard_vaddr =
        target_base_vaddr - sizeof(uint64_t);
    const uintptr_t after_guard_vaddr =
        target_base_vaddr + AI_MEMBLOCK_CMO_AMO_BLOCK_WORDS *
        sizeof(uint64_t);
    const uintptr_t before_guard_paddr =
        target_base_paddr - sizeof(uint64_t);
    const uintptr_t after_guard_paddr =
        target_base_paddr + AI_MEMBLOCK_CMO_AMO_BLOCK_WORDS *
        sizeof(uint64_t);
    const uintptr_t marker_vaddr = hs_page_base(marker_page) + 0x600U;
    const uintptr_t marker_paddr = phys_page_base(marker_page) + 0x600U;
    const uintptr_t marker_guard_vaddr = marker_vaddr + sizeof(uint64_t);
    const uintptr_t marker_guard_paddr = marker_paddr + sizeof(uint64_t);
    const uint64_t block_seeds[] = {
        0x0123456789abcdefULL,
        0x1021324354657687ULL,
        0x3141592653589793ULL,
    };
    const uint64_t pre_lrsc_stores[] = {
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
        0x13579bdf2468ace0ULL,
        0x02468ace13579bdfULL,
        0xfdb97531eca86420ULL,
    };
    const uint64_t marker_guards[] = {
        0x3141592653589793ULL,
        0x2718281828459045ULL,
        0x1123581321345589ULL,
    };

    ai_memblock_cmo_amo_prepare_hs_env();

    for (size_t round = 0; round < AI_MEMBLOCK_FLUSH_ARRAY_SIZE(block_seeds);
         ++round) {
        goto_priv(PRIV_M);
        ai_memblock_cmo_amo_seed_block(target_base_paddr, block_seeds[round]);
        write64(before_guard_paddr, before_guards[round]);
        write64(after_guard_paddr, after_guards[round]);
        write64(marker_paddr, marker_seeds[round]);
        write64(marker_guard_paddr, marker_guards[round]);

        goto_priv(PRIV_HS);
        TEST_SETUP_EXCEPT();
        const uint64_t warmed_target = ld(target_word_vaddr);
        const uint64_t warmed_tail = ld(target_tail_vaddr);
        const uint64_t warmed_marker = ld(marker_vaddr);
        AI_ASSERT_NO_EXCEPT_AND("before the CBOM plus LR/SC owner-window probe starts, target block and marker are readable with seeded values",
            warmed_target == block_seeds[round] &&
            warmed_tail == block_seeds[round] +
                (AI_MEMBLOCK_CMO_AMO_BLOCK_WORDS - 1U) *
                0x1111111111111111ULL &&
            warmed_marker == marker_seeds[round]
        );

        TEST_SETUP_EXCEPT();
        sd(target_word_vaddr, pre_lrsc_stores[round]);
        sd(neighbor_word_vaddr, neighbor_updates[round]);
        cbo_clean(target_word_vaddr);
        cbo_flush(target_tail_vaddr);
        uint64_t lr_observed = 0xffffffffffffffffULL;
        uint64_t sc_status = 1ULL;
        ai_memblock_cmo_lrsc_then_marker(target_word_vaddr,
            sc_payloads[round], marker_vaddr, marker_updates[round],
            &lr_observed, &sc_status);
        const uint64_t observed_target = ld(target_word_vaddr);
        const uint64_t observed_neighbor = ld(neighbor_word_vaddr);
        const uint64_t observed_tail = ld(target_tail_vaddr);
        const uint64_t observed_marker = ld(marker_vaddr);
        AI_ASSERT_NO_EXCEPT_AND("CBOM clean/flush immediately followed by LR/SC keeps CMO and atomic flush ownership separate so LR sees the drained store and SC keeps its reservation",
            lr_observed == pre_lrsc_stores[round] &&
            sc_status == 0ULL &&
            observed_target == sc_payloads[round] &&
            observed_neighbor == neighbor_updates[round] &&
            observed_tail == block_seeds[round] +
                (AI_MEMBLOCK_CMO_AMO_BLOCK_WORDS - 1U) *
                0x1111111111111111ULL &&
            observed_marker == marker_updates[round]
        );

        TEST_SETUP_EXCEPT();
        AI_ASSERT_NO_EXCEPT_AND("the CBOM plus LR/SC owner-window does not corrupt target or marker guards",
            ld(before_guard_vaddr) == before_guards[round] &&
            ld(after_guard_vaddr) == after_guards[round] &&
            ld(marker_guard_vaddr) == marker_guards[round]
        );

        goto_priv(PRIV_M);
        TEST_ASSERT("backing memory after the CBOM plus LR/SC owner-window contains only the successful SC payload, same-block neighbor store, marker update, and unchanged guards",
            ai_memblock_cmo_amo_block_matches_two_updates(
                target_base_paddr,
                block_seeds[round],
                0U,
                sc_payloads[round],
                3U,
                neighbor_updates[round]) &&
            read64(before_guard_paddr) == before_guards[round] &&
            read64(after_guard_paddr) == after_guards[round] &&
            read64(marker_paddr) == marker_updates[round] &&
            read64(marker_guard_paddr) == marker_guards[round]
        );
    }

    ai_memblock_cmo_amo_reset_hs_env();

    TEST_END();
}

bool ai_micro_hs_lrsc_then_cbom_same_block_no_stale_atomic_flush_owner_or_clean_drop_corner()
{
    TEST_START();

    const enum test_page target_page = SWITCH1;
    const enum test_page marker_page = SWITCH2;
    const uintptr_t target_base_vaddr = hs_page_base(target_page) + 0x700U;
    const uintptr_t target_base_paddr = phys_page_base(target_page) + 0x700U;
    const uintptr_t target_word_vaddr = target_base_vaddr;
    const uintptr_t target_tail_vaddr =
        target_base_vaddr +
        (AI_MEMBLOCK_CMO_AMO_BLOCK_WORDS - 1U) * sizeof(uint64_t);
    const uintptr_t before_guard_vaddr =
        target_base_vaddr - sizeof(uint64_t);
    const uintptr_t after_guard_vaddr =
        target_base_vaddr + AI_MEMBLOCK_CMO_AMO_BLOCK_WORDS *
        sizeof(uint64_t);
    const uintptr_t before_guard_paddr =
        target_base_paddr - sizeof(uint64_t);
    const uintptr_t after_guard_paddr =
        target_base_paddr + AI_MEMBLOCK_CMO_AMO_BLOCK_WORDS *
        sizeof(uint64_t);
    const uintptr_t marker_vaddr = hs_page_base(marker_page) + 0x700U;
    const uintptr_t marker_paddr = phys_page_base(marker_page) + 0x700U;
    const uintptr_t marker_guard_vaddr = marker_vaddr + sizeof(uint64_t);
    const uintptr_t marker_guard_paddr = marker_paddr + sizeof(uint64_t);
    const uint64_t block_seeds[] = {
        0x1020304050607080ULL,
        0x21436587a9cbed0fULL,
        0x3141592653589793ULL,
    };
    const uint64_t sc_payloads[] = {
        0x1111222233334444ULL,
        0x5555666677778888ULL,
        0x9999aaaabbbbccccULL,
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

    ai_memblock_cmo_amo_prepare_hs_env();

    for (size_t round = 0; round < AI_MEMBLOCK_FLUSH_ARRAY_SIZE(block_seeds);
         ++round) {
        goto_priv(PRIV_M);
        ai_memblock_cmo_amo_seed_block(target_base_paddr, block_seeds[round]);
        write64(before_guard_paddr, before_guards[round]);
        write64(after_guard_paddr, after_guards[round]);
        write64(marker_paddr, marker_seeds[round]);
        write64(marker_guard_paddr, marker_guards[round]);

        goto_priv(PRIV_HS);
        TEST_SETUP_EXCEPT();
        const uint64_t warmed_target = ld(target_word_vaddr);
        const uint64_t warmed_tail = ld(target_tail_vaddr);
        const uint64_t warmed_marker = ld(marker_vaddr);
        AI_ASSERT_NO_EXCEPT_AND("before the LR/SC plus CBOM owner-window probe starts, target block and marker are readable with seeded values",
            warmed_target == block_seeds[round] &&
            warmed_tail == block_seeds[round] +
                (AI_MEMBLOCK_CMO_AMO_BLOCK_WORDS - 1U) *
                0x1111111111111111ULL &&
            warmed_marker == marker_seeds[round]
        );

        TEST_SETUP_EXCEPT();
        uint64_t lr_observed = 0xffffffffffffffffULL;
        uint64_t sc_status = 1ULL;
        ai_memblock_cmo_lrsc_only(target_word_vaddr, sc_payloads[round],
            &lr_observed, &sc_status);
        cbo_clean(target_word_vaddr);
        cbo_flush(target_tail_vaddr);
        sd(marker_vaddr, marker_updates[round]);
        fence_iorw_iorw();
        const uint64_t observed_target = ld(target_word_vaddr);
        const uint64_t observed_tail = ld(target_tail_vaddr);
        const uint64_t observed_marker = ld(marker_vaddr);
        AI_ASSERT_NO_EXCEPT_AND("LR/SC immediately followed by CBOM clean/flush keeps atomic and CMO flush ownership ordered so the successful SC is not dropped and marker still progresses",
            lr_observed == block_seeds[round] &&
            sc_status == 0ULL &&
            observed_target == sc_payloads[round] &&
            observed_tail == block_seeds[round] +
                (AI_MEMBLOCK_CMO_AMO_BLOCK_WORDS - 1U) *
                0x1111111111111111ULL &&
            observed_marker == marker_updates[round]
        );

        TEST_SETUP_EXCEPT();
        AI_ASSERT_NO_EXCEPT_AND("the LR/SC plus CBOM owner-window does not corrupt target or marker guards",
            ld(before_guard_vaddr) == before_guards[round] &&
            ld(after_guard_vaddr) == after_guards[round] &&
            ld(marker_guard_vaddr) == marker_guards[round]
        );

        goto_priv(PRIV_M);
        TEST_ASSERT("backing memory after the LR/SC plus CBOM owner-window contains the successful SC payload, marker update, and unchanged block tail and guards",
            ai_memblock_cmo_amo_block_matches_two_updates(
                target_base_paddr,
                block_seeds[round],
                0U,
                sc_payloads[round],
                AI_MEMBLOCK_CMO_AMO_BLOCK_WORDS,
                0ULL) &&
            read64(before_guard_paddr) == before_guards[round] &&
            read64(after_guard_paddr) == after_guards[round] &&
            read64(marker_paddr) == marker_updates[round] &&
            read64(marker_guard_paddr) == marker_guards[round]
        );
    }

    ai_memblock_cmo_amo_reset_hs_env();

    TEST_END();
}

bool ai_micro_hs_amo_then_cbom_same_block_no_stale_atomic_flush_owner_or_clean_drop_corner()
{
    TEST_START();

    const enum test_page target_page = SWITCH1;
    const enum test_page marker_page = SWITCH2;
    const uintptr_t target_base_vaddr = hs_page_base(target_page) + 0x800U;
    const uintptr_t target_base_paddr = phys_page_base(target_page) + 0x800U;
    const uintptr_t target_word_vaddr = target_base_vaddr;
    const uintptr_t neighbor_word_vaddr =
        target_base_vaddr + 3U * sizeof(uint64_t);
    const uintptr_t target_tail_vaddr =
        target_base_vaddr +
        (AI_MEMBLOCK_CMO_AMO_BLOCK_WORDS - 1U) * sizeof(uint64_t);
    const uintptr_t before_guard_vaddr =
        target_base_vaddr - sizeof(uint64_t);
    const uintptr_t after_guard_vaddr =
        target_base_vaddr + AI_MEMBLOCK_CMO_AMO_BLOCK_WORDS *
        sizeof(uint64_t);
    const uintptr_t before_guard_paddr =
        target_base_paddr - sizeof(uint64_t);
    const uintptr_t after_guard_paddr =
        target_base_paddr + AI_MEMBLOCK_CMO_AMO_BLOCK_WORDS *
        sizeof(uint64_t);
    const uintptr_t marker_vaddr = hs_page_base(marker_page) + 0x800U;
    const uintptr_t marker_paddr = phys_page_base(marker_page) + 0x800U;
    const uintptr_t marker_guard_vaddr = marker_vaddr + sizeof(uint64_t);
    const uintptr_t marker_guard_paddr = marker_paddr + sizeof(uint64_t);
    const uint64_t block_seeds[] = {
        0x0101010101010101ULL,
        0x2121212121212121ULL,
        0x4141414141414141ULL,
    };
    const uint64_t amo_addends[] = {
        0x0000000000000101ULL,
        0x0000000000000202ULL,
        0x0000000000000303ULL,
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

    ai_memblock_cmo_amo_prepare_hs_env();

    for (size_t round = 0; round < AI_MEMBLOCK_FLUSH_ARRAY_SIZE(block_seeds);
         ++round) {
        const uint64_t expected_target =
            block_seeds[round] + amo_addends[round];

        goto_priv(PRIV_M);
        ai_memblock_cmo_amo_seed_block(target_base_paddr, block_seeds[round]);
        write64(before_guard_paddr, before_guards[round]);
        write64(after_guard_paddr, after_guards[round]);
        write64(marker_paddr, marker_seeds[round]);
        write64(marker_guard_paddr, marker_guards[round]);

        goto_priv(PRIV_HS);
        TEST_SETUP_EXCEPT();
        const uint64_t warmed_target = ld(target_word_vaddr);
        const uint64_t warmed_tail = ld(target_tail_vaddr);
        const uint64_t warmed_marker = ld(marker_vaddr);
        AI_ASSERT_NO_EXCEPT_AND("before the AMO plus CBOM owner-window probe starts, target block and marker are readable with seeded values",
            warmed_target == block_seeds[round] &&
            warmed_tail == block_seeds[round] +
                (AI_MEMBLOCK_CMO_AMO_BLOCK_WORDS - 1U) *
                0x1111111111111111ULL &&
            warmed_marker == marker_seeds[round]
        );

        TEST_SETUP_EXCEPT();
        sd(neighbor_word_vaddr, neighbor_updates[round]);
        const uint64_t amo_old =
            amoadd_d(target_word_vaddr, amo_addends[round]);
        cbo_clean(target_word_vaddr);
        cbo_flush(target_tail_vaddr);
        sd(marker_vaddr, marker_updates[round]);
        fence_iorw_iorw();
        const uint64_t observed_target = ld(target_word_vaddr);
        const uint64_t observed_neighbor = ld(neighbor_word_vaddr);
        const uint64_t observed_tail = ld(target_tail_vaddr);
        const uint64_t observed_marker = ld(marker_vaddr);
        AI_ASSERT_NO_EXCEPT_AND("AMO immediately followed by CBOM clean/flush keeps atomic and CMO flush ownership ordered so the AMO update and same-block pressure store are not dropped",
            amo_old == block_seeds[round] &&
            observed_target == expected_target &&
            observed_neighbor == neighbor_updates[round] &&
            observed_tail == block_seeds[round] +
                (AI_MEMBLOCK_CMO_AMO_BLOCK_WORDS - 1U) *
                0x1111111111111111ULL &&
            observed_marker == marker_updates[round]
        );

        TEST_SETUP_EXCEPT();
        AI_ASSERT_NO_EXCEPT_AND("the AMO plus CBOM owner-window does not corrupt target or marker guards",
            ld(before_guard_vaddr) == before_guards[round] &&
            ld(after_guard_vaddr) == after_guards[round] &&
            ld(marker_guard_vaddr) == marker_guards[round]
        );

        goto_priv(PRIV_M);
        TEST_ASSERT("backing memory after the AMO plus CBOM owner-window contains the AMO update, same-block pressure store, marker update, and unchanged guards",
            ai_memblock_cmo_amo_block_matches_two_updates(
                target_base_paddr,
                block_seeds[round],
                0U,
                expected_target,
                3U,
                neighbor_updates[round]) &&
            read64(before_guard_paddr) == before_guards[round] &&
            read64(after_guard_paddr) == after_guards[round] &&
            read64(marker_paddr) == marker_updates[round] &&
            read64(marker_guard_paddr) == marker_guards[round]
        );
    }

    ai_memblock_cmo_amo_reset_hs_env();

    TEST_END();
}

bool ai_micro_hs_cbom_then_normal_vector_no_stale_cmo_flush_owner_or_payload_corner()
{
    TEST_START();

    const enum test_page src_page = SWITCH1;
    const enum test_page dst_page = SWITCH2;
    const enum test_page marker_page = VSRWX_GRWX;
    const uintptr_t src_base_vaddr = hs_page_base(src_page) + 0x900U;
    const uintptr_t src_base_paddr = phys_page_base(src_page) + 0x900U;
    const uintptr_t src_word1_vaddr = src_base_vaddr + sizeof(uint64_t);
    const uintptr_t src_tail_vaddr =
        src_base_vaddr +
        (AI_MEMBLOCK_CMO_AMO_BLOCK_WORDS - 1U) * sizeof(uint64_t);
    const uintptr_t before_guard_vaddr =
        src_base_vaddr - sizeof(uint64_t);
    const uintptr_t after_guard_vaddr =
        src_base_vaddr + AI_MEMBLOCK_CMO_AMO_BLOCK_WORDS *
        sizeof(uint64_t);
    const uintptr_t before_guard_paddr =
        src_base_paddr - sizeof(uint64_t);
    const uintptr_t after_guard_paddr =
        src_base_paddr + AI_MEMBLOCK_CMO_AMO_BLOCK_WORDS *
        sizeof(uint64_t);
    const uintptr_t dst_vaddr = hs_page_base(dst_page) + 0x900U;
    const uintptr_t dst_paddr = phys_page_base(dst_page) + 0x900U;
    const uintptr_t dst_before_vaddr = dst_vaddr - sizeof(uint64_t);
    const uintptr_t dst_after_vaddr = dst_vaddr + AI_MEMBLOCK_CMO_VECTOR_BYTES;
    const uintptr_t dst_before_paddr = dst_paddr - sizeof(uint64_t);
    const uintptr_t dst_after_paddr = dst_paddr + AI_MEMBLOCK_CMO_VECTOR_BYTES;
    const uintptr_t marker_vaddr = hs_page_base(marker_page) + 0x980U;
    const uintptr_t marker_paddr = phys_page_base(marker_page) + 0x980U;
    const uintptr_t marker_guard_vaddr = marker_vaddr + sizeof(uint64_t);
    const uintptr_t marker_guard_paddr = marker_paddr + sizeof(uint64_t);
    const uint64_t block_seeds[] = {
        0x0f1e2d3c4b5a6978ULL,
        0x8877665544332211ULL,
        0x13579bdf2468ace0ULL,
    };
    const uint64_t src_word0_updates[] = {
        0x1111222233334444ULL,
        0x5555666677778888ULL,
        0x9999aaaabbbbccccULL,
    };
    const uint64_t src_word1_updates[] = {
        0x0102030405060708ULL,
        0x1827364554637281ULL,
        0xfedcba9876543210ULL,
    };
    const uint64_t dst_fills[] = {
        0xc3c3c3c3c3c3c3c3ULL,
        0x3c3c3c3c3c3c3c3cULL,
        0xa55aa55ac33cc33cULL,
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
    const uint64_t dst_before_guards[] = {
        0x13579bdf2468ace0ULL,
        0x02468ace13579bdfULL,
        0xfdb97531eca86420ULL,
    };
    const uint64_t dst_after_guards[] = {
        0x1123581321345589ULL,
        0x3141592653589793ULL,
        0x2718281828459045ULL,
    };
    const uint64_t marker_seeds[] = {
        0xa0a1a2a3a4a5a6a7ULL,
        0xb0b1b2b3b4b5b6b7ULL,
        0xc0c1c2c3c4c5c6c7ULL,
    };
    const uint64_t marker_updates[] = {
        0x8877665544332211ULL,
        0x7766554433221100ULL,
        0x66554433221100ffULL,
    };
    const uint64_t marker_guards[] = {
        0x0badf00d5a5aa5a5ULL,
        0x1020304050607080ULL,
        0x1122334455667788ULL,
    };

    ai_memblock_cmo_amo_prepare_hs_env();

    for (size_t round = 0; round < AI_MEMBLOCK_FLUSH_ARRAY_SIZE(block_seeds);
         ++round) {
        goto_priv(PRIV_M);
        ai_memblock_cmo_amo_seed_block(src_base_paddr, block_seeds[round]);
        write64(before_guard_paddr, before_guards[round]);
        write64(after_guard_paddr, after_guards[round]);
        write64(dst_paddr, dst_fills[round]);
        write64(dst_paddr + sizeof(uint64_t), ~dst_fills[round]);
        write64(dst_before_paddr, dst_before_guards[round]);
        write64(dst_after_paddr, dst_after_guards[round]);
        write64(marker_paddr, marker_seeds[round]);
        write64(marker_guard_paddr, marker_guards[round]);

        goto_priv(PRIV_HS);
        TEST_SETUP_EXCEPT();
        const uint64_t warmed_src = ld(src_base_vaddr);
        const uint64_t warmed_tail = ld(src_tail_vaddr);
        const uint64_t warmed_marker = ld(marker_vaddr);
        AI_ASSERT_NO_EXCEPT_AND("before the CBOM plus normal-vector owner-window probe starts, source block and marker are readable with seeded values",
            warmed_src == block_seeds[round] &&
            warmed_tail == block_seeds[round] +
                (AI_MEMBLOCK_CMO_AMO_BLOCK_WORDS - 1U) *
                0x1111111111111111ULL &&
            warmed_marker == marker_seeds[round]
        );

        TEST_SETUP_EXCEPT();
        sd(src_base_vaddr, src_word0_updates[round]);
        sd(src_word1_vaddr, src_word1_updates[round]);
        cbo_clean(src_base_vaddr);
        cbo_flush(src_tail_vaddr);
        ai_memblock_cmo_normal_vector_copy_then_marker(src_base_vaddr,
            dst_vaddr, marker_vaddr, marker_updates[round]);
        const uint64_t observed_src0 = ld(src_base_vaddr);
        const uint64_t observed_src1 = ld(src_word1_vaddr);
        const uint64_t observed_tail = ld(src_tail_vaddr);
        const uint64_t observed_dst0 = ld(dst_vaddr);
        const uint64_t observed_dst1 = ld(dst_vaddr + sizeof(uint64_t));
        const uint64_t observed_marker = ld(marker_vaddr);
        AI_ASSERT_NO_EXCEPT_AND("CBOM clean/flush immediately followed by normal vector load/store keeps CMO and vector split/merge ownership separate so vector payload sees the drained source stores",
            observed_src0 == src_word0_updates[round] &&
            observed_src1 == src_word1_updates[round] &&
            observed_tail == block_seeds[round] +
                (AI_MEMBLOCK_CMO_AMO_BLOCK_WORDS - 1U) *
                0x1111111111111111ULL &&
            observed_dst0 == src_word0_updates[round] &&
            observed_dst1 == src_word1_updates[round] &&
            observed_marker == marker_updates[round]
        );

        TEST_SETUP_EXCEPT();
        AI_ASSERT_NO_EXCEPT_AND("the CBOM plus normal-vector owner-window does not corrupt source, destination, or marker guards",
            ld(before_guard_vaddr) == before_guards[round] &&
            ld(after_guard_vaddr) == after_guards[round] &&
            ld(dst_before_vaddr) == dst_before_guards[round] &&
            ld(dst_after_vaddr) == dst_after_guards[round] &&
            ld(marker_guard_vaddr) == marker_guards[round]
        );

        goto_priv(PRIV_M);
        TEST_ASSERT("backing memory after the CBOM plus normal-vector owner-window contains the two drained source stores, copied vector destination, marker update, and unchanged guards",
            ai_memblock_cmo_amo_block_matches_two_updates(
                src_base_paddr,
                block_seeds[round],
                0U,
                src_word0_updates[round],
                1U,
                src_word1_updates[round]) &&
            read64(before_guard_paddr) == before_guards[round] &&
            read64(after_guard_paddr) == after_guards[round] &&
            read64(dst_paddr) == src_word0_updates[round] &&
            read64(dst_paddr + sizeof(uint64_t)) ==
                src_word1_updates[round] &&
            read64(dst_before_paddr) == dst_before_guards[round] &&
            read64(dst_after_paddr) == dst_after_guards[round] &&
            read64(marker_paddr) == marker_updates[round] &&
            read64(marker_guard_paddr) == marker_guards[round]
        );
    }

    ai_memblock_cmo_amo_reset_hs_env();

    TEST_END();
}

bool ai_micro_hs_normal_vector_then_cbom_no_stale_vector_store_or_cmo_owner_corner()
{
    TEST_START();

    const enum test_page src_page = SWITCH1;
    const enum test_page dst_page = SWITCH2;
    const enum test_page marker_page = VSRWX_GRWX;
    const uintptr_t src_vaddr = hs_page_base(src_page) + 0xa00U;
    const uintptr_t src_paddr = phys_page_base(src_page) + 0xa00U;
    const uintptr_t src_guard_vaddr = src_vaddr + AI_MEMBLOCK_CMO_VECTOR_BYTES;
    const uintptr_t src_guard_paddr = src_paddr + AI_MEMBLOCK_CMO_VECTOR_BYTES;
    const uintptr_t dst_base_vaddr = hs_page_base(dst_page) + 0xa00U;
    const uintptr_t dst_base_paddr = phys_page_base(dst_page) + 0xa00U;
    const uintptr_t dst_tail_vaddr =
        dst_base_vaddr +
        (AI_MEMBLOCK_CMO_AMO_BLOCK_WORDS - 1U) * sizeof(uint64_t);
    const uintptr_t dst_before_vaddr =
        dst_base_vaddr - sizeof(uint64_t);
    const uintptr_t dst_after_vaddr =
        dst_base_vaddr + AI_MEMBLOCK_CMO_AMO_BLOCK_WORDS *
        sizeof(uint64_t);
    const uintptr_t dst_before_paddr =
        dst_base_paddr - sizeof(uint64_t);
    const uintptr_t dst_after_paddr =
        dst_base_paddr + AI_MEMBLOCK_CMO_AMO_BLOCK_WORDS *
        sizeof(uint64_t);
    const uintptr_t marker_vaddr = hs_page_base(marker_page) + 0xa80U;
    const uintptr_t marker_paddr = phys_page_base(marker_page) + 0xa80U;
    const uintptr_t marker_guard_vaddr = marker_vaddr + sizeof(uint64_t);
    const uintptr_t marker_guard_paddr = marker_paddr + sizeof(uint64_t);
    const uint64_t src_word0_values[] = {
        0x1111222233334444ULL,
        0x5555666677778888ULL,
        0x9999aaaabbbbccccULL,
    };
    const uint64_t src_word1_values[] = {
        0x0102030405060708ULL,
        0x1827364554637281ULL,
        0xfedcba9876543210ULL,
    };
    const uint64_t dst_seeds[] = {
        0x0f1e2d3c4b5a6978ULL,
        0x8877665544332211ULL,
        0x13579bdf2468ace0ULL,
    };
    const uint64_t src_guards[] = {
        0xfeedfacecafebeefULL,
        0xdeadc0def00dbabeULL,
        0xfaceb00c13579bdfULL,
    };
    const uint64_t dst_before_guards[] = {
        0x0badf00d5a5aa5a5ULL,
        0x1020304050607080ULL,
        0x1122334455667788ULL,
    };
    const uint64_t dst_after_guards[] = {
        0x13579bdf2468ace0ULL,
        0x02468ace13579bdfULL,
        0xfdb97531eca86420ULL,
    };
    const uint64_t marker_seeds[] = {
        0xa0a1a2a3a4a5a6a7ULL,
        0xb0b1b2b3b4b5b6b7ULL,
        0xc0c1c2c3c4c5c6c7ULL,
    };
    const uint64_t marker_updates[] = {
        0x8877665544332211ULL,
        0x7766554433221100ULL,
        0x66554433221100ffULL,
    };
    const uint64_t marker_guards[] = {
        0x1123581321345589ULL,
        0x3141592653589793ULL,
        0x2718281828459045ULL,
    };

    ai_memblock_cmo_amo_prepare_hs_env();

    for (size_t round = 0; round < AI_MEMBLOCK_FLUSH_ARRAY_SIZE(dst_seeds);
         ++round) {
        goto_priv(PRIV_M);
        write64(src_paddr, src_word0_values[round]);
        write64(src_paddr + sizeof(uint64_t), src_word1_values[round]);
        write64(src_guard_paddr, src_guards[round]);
        ai_memblock_cmo_amo_seed_block(dst_base_paddr, dst_seeds[round]);
        write64(dst_before_paddr, dst_before_guards[round]);
        write64(dst_after_paddr, dst_after_guards[round]);
        write64(marker_paddr, marker_seeds[round]);
        write64(marker_guard_paddr, marker_guards[round]);

        goto_priv(PRIV_HS);
        TEST_SETUP_EXCEPT();
        const uint64_t warmed_src0 = ld(src_vaddr);
        const uint64_t warmed_src1 = ld(src_vaddr + sizeof(uint64_t));
        const uint64_t warmed_dst_tail = ld(dst_tail_vaddr);
        const uint64_t warmed_marker = ld(marker_vaddr);
        AI_ASSERT_NO_EXCEPT_AND("before the normal-vector plus CBOM owner-window probe starts, source, destination tail, and marker are readable with seeded values",
            warmed_src0 == src_word0_values[round] &&
            warmed_src1 == src_word1_values[round] &&
            warmed_dst_tail == dst_seeds[round] +
                (AI_MEMBLOCK_CMO_AMO_BLOCK_WORDS - 1U) *
                0x1111111111111111ULL &&
            warmed_marker == marker_seeds[round]
        );

        TEST_SETUP_EXCEPT();
        ai_memblock_cmo_normal_vector_copy_only(src_vaddr, dst_base_vaddr);
        cbo_clean(dst_base_vaddr);
        cbo_flush(dst_tail_vaddr);
        sd(marker_vaddr, marker_updates[round]);
        fence_iorw_iorw();
        const uint64_t observed_dst0 = ld(dst_base_vaddr);
        const uint64_t observed_dst1 =
            ld(dst_base_vaddr + sizeof(uint64_t));
        const uint64_t observed_dst_tail = ld(dst_tail_vaddr);
        const uint64_t observed_marker = ld(marker_vaddr);
        AI_ASSERT_NO_EXCEPT_AND("normal vector load/store immediately followed by CBOM clean/flush keeps vector store and CMO ownership ordered so the copied payload and marker both survive",
            observed_dst0 == src_word0_values[round] &&
            observed_dst1 == src_word1_values[round] &&
            observed_dst_tail == dst_seeds[round] +
                (AI_MEMBLOCK_CMO_AMO_BLOCK_WORDS - 1U) *
                0x1111111111111111ULL &&
            observed_marker == marker_updates[round]
        );

        TEST_SETUP_EXCEPT();
        AI_ASSERT_NO_EXCEPT_AND("the normal-vector plus CBOM owner-window does not corrupt source, destination, or marker guards",
            ld(src_guard_vaddr) == src_guards[round] &&
            ld(dst_before_vaddr) == dst_before_guards[round] &&
            ld(dst_after_vaddr) == dst_after_guards[round] &&
            ld(marker_guard_vaddr) == marker_guards[round]
        );

        goto_priv(PRIV_M);
        TEST_ASSERT("backing memory after the normal-vector plus CBOM owner-window contains the copied vector payload, marker update, and unchanged guards",
            read64(src_paddr) == src_word0_values[round] &&
            read64(src_paddr + sizeof(uint64_t)) ==
                src_word1_values[round] &&
            read64(src_guard_paddr) == src_guards[round] &&
            ai_memblock_cmo_amo_block_matches_two_updates(
                dst_base_paddr,
                dst_seeds[round],
                0U,
                src_word0_values[round],
                1U,
                src_word1_values[round]) &&
            read64(dst_before_paddr) == dst_before_guards[round] &&
            read64(dst_after_paddr) == dst_after_guards[round] &&
            read64(marker_paddr) == marker_updates[round] &&
            read64(marker_guard_paddr) == marker_guards[round]
        );
    }

    ai_memblock_cmo_amo_reset_hs_env();

    TEST_END();
}

bool ai_micro_hs_normal_vector_store_then_fence_no_stale_vector_writeback_or_sbisempty_corner()
{
    TEST_START();

    const enum test_page src_page = SWITCH1;
    const enum test_page dst_page = SWITCH2;
    const enum test_page marker_page = VSRWX_GRWX;
    const uintptr_t src_vaddr = hs_page_base(src_page) + 0xd80U;
    const uintptr_t src_paddr = phys_page_base(src_page) + 0xd80U;
    const uintptr_t src_guard_vaddr = src_vaddr + AI_MEMBLOCK_CMO_VECTOR_BYTES;
    const uintptr_t src_guard_paddr = src_paddr + AI_MEMBLOCK_CMO_VECTOR_BYTES;
    const uintptr_t dst_base_vaddr = hs_page_base(dst_page) + 0xd80U;
    const uintptr_t dst_base_paddr = phys_page_base(dst_page) + 0xd80U;
    const uintptr_t dst_tail_vaddr =
        dst_base_vaddr +
        (AI_MEMBLOCK_CMO_AMO_BLOCK_WORDS - 1U) * sizeof(uint64_t);
    const uintptr_t dst_before_vaddr =
        dst_base_vaddr - sizeof(uint64_t);
    const uintptr_t dst_after_vaddr =
        dst_base_vaddr + AI_MEMBLOCK_CMO_AMO_BLOCK_WORDS *
        sizeof(uint64_t);
    const uintptr_t dst_before_paddr =
        dst_base_paddr - sizeof(uint64_t);
    const uintptr_t dst_after_paddr =
        dst_base_paddr + AI_MEMBLOCK_CMO_AMO_BLOCK_WORDS *
        sizeof(uint64_t);
    const uintptr_t marker_vaddr = hs_page_base(marker_page) + 0xd80U;
    const uintptr_t marker_paddr = phys_page_base(marker_page) + 0xd80U;
    const uintptr_t marker_guard_vaddr = marker_vaddr + sizeof(uint64_t);
    const uintptr_t marker_guard_paddr = marker_paddr + sizeof(uint64_t);
    const uint64_t src_word0_values[] = {
        0x0123456789abcdefULL,
        0xfedcba9876543210ULL,
        0x8899aabbccddeeffULL,
    };
    const uint64_t src_word1_values[] = {
        0x13579bdf2468ace0ULL,
        0x02468ace13579bdfULL,
        0xfdb97531eca86420ULL,
    };
    const uint64_t dst_seeds[] = {
        0x0f1e2d3c4b5a6978ULL,
        0x8877665544332211ULL,
        0x21436587a9cbed0fULL,
    };
    const uint64_t src_guards[] = {
        0xfeedfacecafebeefULL,
        0xdeadc0def00dbabeULL,
        0xfaceb00c13579bdfULL,
    };
    const uint64_t dst_before_guards[] = {
        0x0badf00d5a5aa5a5ULL,
        0x1020304050607080ULL,
        0x1122334455667788ULL,
    };
    const uint64_t dst_after_guards[] = {
        0xfdb97531eca86420ULL,
        0x13579bdf2468ace0ULL,
        0x02468ace13579bdfULL,
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
        0x1234fedc5678ba90ULL,
        0x0badc0de55aa33ccULL,
        0x55aa33cc7799bbddULL,
    };

    goto_priv(PRIV_HS);
    hspt_init();
    sfence_vma();

    for (size_t round = 0; round < AI_MEMBLOCK_FLUSH_ARRAY_SIZE(dst_seeds);
         ++round) {
        uint64_t marker_reloaded = 0ULL;

        goto_priv(PRIV_M);
        write64(src_paddr, src_word0_values[round]);
        write64(src_paddr + sizeof(uint64_t), src_word1_values[round]);
        write64(src_guard_paddr, src_guards[round]);
        ai_memblock_cmo_amo_seed_block(dst_base_paddr, dst_seeds[round]);
        write64(dst_before_paddr, dst_before_guards[round]);
        write64(dst_after_paddr, dst_after_guards[round]);
        write64(marker_paddr, marker_seeds[round]);
        write64(marker_guard_paddr, marker_guards[round]);

        goto_priv(PRIV_HS);
        TEST_SETUP_EXCEPT();
        const uint64_t warmed_src0 = ld(src_vaddr);
        const uint64_t warmed_src1 = ld(src_vaddr + sizeof(uint64_t));
        const uint64_t warmed_dst_tail = ld(dst_tail_vaddr);
        const uint64_t warmed_marker = ld(marker_vaddr);
        AI_ASSERT_NO_EXCEPT_AND("before the normal-vector store then fence probe starts, source, destination tail, and marker are readable with seeded values",
            warmed_src0 == src_word0_values[round] &&
            warmed_src1 == src_word1_values[round] &&
            warmed_dst_tail == dst_seeds[round] +
                (AI_MEMBLOCK_CMO_AMO_BLOCK_WORDS - 1U) *
                0x1111111111111111ULL &&
            warmed_marker == marker_seeds[round]
        );

        TEST_SETUP_EXCEPT();
        ai_memblock_cmo_normal_vector_store_fence_marker(src_vaddr,
            dst_base_vaddr, marker_vaddr, marker_updates[round],
            &marker_reloaded);
        const uint64_t observed_dst0 = ld(dst_base_vaddr);
        const uint64_t observed_dst1 =
            ld(dst_base_vaddr + sizeof(uint64_t));
        const uint64_t observed_dst_tail = ld(dst_tail_vaddr);
        const uint64_t observed_marker = ld(marker_vaddr);
        AI_ASSERT_NO_EXCEPT_AND("normal vector store immediately followed by fence waits for vector writeback instead of observing stale sbIsEmpty, and the post-fence marker still progresses",
            observed_dst0 == src_word0_values[round] &&
            observed_dst1 == src_word1_values[round] &&
            observed_dst_tail == dst_seeds[round] +
                (AI_MEMBLOCK_CMO_AMO_BLOCK_WORDS - 1U) *
                0x1111111111111111ULL &&
            observed_marker == marker_updates[round] &&
            marker_reloaded == marker_updates[round]
        );

        TEST_SETUP_EXCEPT();
        AI_ASSERT_NO_EXCEPT_AND("the normal-vector store then fence boundary does not corrupt source, destination, or marker guards",
            ld(src_guard_vaddr) == src_guards[round] &&
            ld(dst_before_vaddr) == dst_before_guards[round] &&
            ld(dst_after_vaddr) == dst_after_guards[round] &&
            ld(marker_guard_vaddr) == marker_guards[round]
        );

        goto_priv(PRIV_M);
        TEST_ASSERT("backing memory after the normal-vector store then fence window contains the copied vector payload, marker update, and unchanged guards",
            read64(src_paddr) == src_word0_values[round] &&
            read64(src_paddr + sizeof(uint64_t)) ==
                src_word1_values[round] &&
            read64(src_guard_paddr) == src_guards[round] &&
            ai_memblock_cmo_amo_block_matches_two_updates(
                dst_base_paddr,
                dst_seeds[round],
                0U,
                src_word0_values[round],
                1U,
                src_word1_values[round]) &&
            read64(dst_before_paddr) == dst_before_guards[round] &&
            read64(dst_after_paddr) == dst_after_guards[round] &&
            read64(marker_paddr) == marker_updates[round] &&
            read64(marker_guard_paddr) == marker_guards[round]
        );
    }

    goto_priv(PRIV_HS);
    hspt_init();
    sfence_vma();

    TEST_END();
}

bool ai_micro_hs_fence_then_normal_vector_store_no_stale_fence_owner_or_vector_payload_corner()
{
    TEST_START();

    const enum test_page src_page = SWITCH1;
    const enum test_page dst_page = SWITCH2;
    const enum test_page marker_page = VSRWX_GRWX;
    const uintptr_t src_vaddr = hs_page_base(src_page) + 0xe80U;
    const uintptr_t src_paddr = phys_page_base(src_page) + 0xe80U;
    const uintptr_t src_guard_vaddr = src_vaddr + AI_MEMBLOCK_CMO_VECTOR_BYTES;
    const uintptr_t src_guard_paddr = src_paddr + AI_MEMBLOCK_CMO_VECTOR_BYTES;
    const uintptr_t dst_base_vaddr = hs_page_base(dst_page) + 0xe80U;
    const uintptr_t dst_base_paddr = phys_page_base(dst_page) + 0xe80U;
    const uintptr_t dst_pressure_vaddr =
        dst_base_vaddr +
        (AI_MEMBLOCK_CMO_AMO_BLOCK_WORDS - 1U) * sizeof(uint64_t);
    const uintptr_t dst_before_vaddr =
        dst_base_vaddr - sizeof(uint64_t);
    const uintptr_t dst_after_vaddr =
        dst_base_vaddr + AI_MEMBLOCK_CMO_AMO_BLOCK_WORDS *
        sizeof(uint64_t);
    const uintptr_t dst_pressure_paddr =
        dst_base_paddr +
        (AI_MEMBLOCK_CMO_AMO_BLOCK_WORDS - 1U) * sizeof(uint64_t);
    const uintptr_t dst_before_paddr =
        dst_base_paddr - sizeof(uint64_t);
    const uintptr_t dst_after_paddr =
        dst_base_paddr + AI_MEMBLOCK_CMO_AMO_BLOCK_WORDS *
        sizeof(uint64_t);
    const uintptr_t marker_vaddr = hs_page_base(marker_page) + 0xe80U;
    const uintptr_t marker_paddr = phys_page_base(marker_page) + 0xe80U;
    const uintptr_t marker_guard_vaddr = marker_vaddr + sizeof(uint64_t);
    const uintptr_t marker_guard_paddr = marker_paddr + sizeof(uint64_t);
    const uint64_t src_word0_values[] = {
        0x1020304050607080ULL,
        0x8877665544332211ULL,
        0x13579bdf2468ace0ULL,
    };
    const uint64_t src_word1_values[] = {
        0x0102030405060708ULL,
        0xfedcba9876543210ULL,
        0x02468ace13579bdfULL,
    };
    const uint64_t dst_seeds[] = {
        0x21436587a9cbed0fULL,
        0x0f1e2d3c4b5a6978ULL,
        0x1122334455667788ULL,
    };
    const uint64_t pressure_updates[] = {
        0x8899aabbccddeeffULL,
        0x7766554433221100ULL,
        0x66554433221100ffULL,
    };
    const uint64_t src_guards[] = {
        0xfeedfacecafebeefULL,
        0xdeadc0def00dbabeULL,
        0xfaceb00c13579bdfULL,
    };
    const uint64_t dst_before_guards[] = {
        0x1234fedc5678ba90ULL,
        0x0badc0de55aa33ccULL,
        0x55aa33cc7799bbddULL,
    };
    const uint64_t dst_after_guards[] = {
        0xfdb97531eca86420ULL,
        0x13579bdf2468ace0ULL,
        0x02468ace13579bdfULL,
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
        0x0badf00d5a5aa5a5ULL,
        0x1020304050607080ULL,
        0x1122334455667788ULL,
    };

    goto_priv(PRIV_HS);
    hspt_init();
    sfence_vma();

    for (size_t round = 0; round < AI_MEMBLOCK_FLUSH_ARRAY_SIZE(dst_seeds);
         ++round) {
        uint64_t marker_reloaded = 0ULL;

        goto_priv(PRIV_M);
        write64(src_paddr, src_word0_values[round]);
        write64(src_paddr + sizeof(uint64_t), src_word1_values[round]);
        write64(src_guard_paddr, src_guards[round]);
        ai_memblock_cmo_amo_seed_block(dst_base_paddr, dst_seeds[round]);
        write64(dst_before_paddr, dst_before_guards[round]);
        write64(dst_after_paddr, dst_after_guards[round]);
        write64(marker_paddr, marker_seeds[round]);
        write64(marker_guard_paddr, marker_guards[round]);

        goto_priv(PRIV_HS);
        TEST_SETUP_EXCEPT();
        const uint64_t warmed_src0 = ld(src_vaddr);
        const uint64_t warmed_src1 = ld(src_vaddr + sizeof(uint64_t));
        const uint64_t warmed_pressure = ld(dst_pressure_vaddr);
        const uint64_t warmed_marker = ld(marker_vaddr);
        AI_ASSERT_NO_EXCEPT_AND("before the fence then normal-vector store probe starts, source, destination tail, and marker are readable with seeded values",
            warmed_src0 == src_word0_values[round] &&
            warmed_src1 == src_word1_values[round] &&
            warmed_pressure == dst_seeds[round] +
                (AI_MEMBLOCK_CMO_AMO_BLOCK_WORDS - 1U) *
                0x1111111111111111ULL &&
            warmed_marker == marker_seeds[round]
        );

        TEST_SETUP_EXCEPT();
        ai_memblock_cmo_fence_then_normal_vector_store_marker(
            dst_pressure_vaddr, pressure_updates[round], src_vaddr,
            dst_base_vaddr, marker_vaddr, marker_updates[round],
            &marker_reloaded);
        const uint64_t observed_dst0 = ld(dst_base_vaddr);
        const uint64_t observed_dst1 =
            ld(dst_base_vaddr + sizeof(uint64_t));
        const uint64_t observed_pressure = ld(dst_pressure_vaddr);
        const uint64_t observed_marker = ld(marker_vaddr);
        AI_ASSERT_NO_EXCEPT_AND("fence drain immediately followed by normal vector store releases fence ownership before vector writeback, preserving both the drained pressure store and vector payload",
            observed_dst0 == src_word0_values[round] &&
            observed_dst1 == src_word1_values[round] &&
            observed_pressure == pressure_updates[round] &&
            observed_marker == marker_updates[round] &&
            marker_reloaded == marker_updates[round]
        );

        TEST_SETUP_EXCEPT();
        AI_ASSERT_NO_EXCEPT_AND("the fence then normal-vector store boundary does not corrupt source, destination, or marker guards",
            ld(src_guard_vaddr) == src_guards[round] &&
            ld(dst_before_vaddr) == dst_before_guards[round] &&
            ld(dst_after_vaddr) == dst_after_guards[round] &&
            ld(marker_guard_vaddr) == marker_guards[round]
        );

        goto_priv(PRIV_M);
        TEST_ASSERT("backing memory after the fence then normal-vector store window contains the drained pressure store, copied vector payload, marker update, and unchanged guards",
            read64(src_paddr) == src_word0_values[round] &&
            read64(src_paddr + sizeof(uint64_t)) ==
                src_word1_values[round] &&
            read64(src_guard_paddr) == src_guards[round] &&
            ai_memblock_cmo_amo_block_matches_three_updates(
                dst_base_paddr,
                dst_seeds[round],
                0U,
                src_word0_values[round],
                1U,
                src_word1_values[round],
                AI_MEMBLOCK_CMO_AMO_BLOCK_WORDS - 1U,
                pressure_updates[round]) &&
            read64(dst_before_paddr) == dst_before_guards[round] &&
            read64(dst_after_paddr) == dst_after_guards[round] &&
            read64(marker_paddr) == marker_updates[round] &&
            read64(marker_guard_paddr) == marker_guards[round]
        );
    }

    goto_priv(PRIV_HS);
    hspt_init();
    sfence_vma();

    TEST_END();
}

bool ai_micro_hs_amo_then_normal_vector_store_no_stale_atomic_owner_or_vector_payload_corner()
{
    TEST_START();

    const enum test_page src_page = SWITCH1;
    const enum test_page dst_page = SWITCH2;
    const enum test_page amo_page = VSRWX_GRWX;
    const uintptr_t src_vaddr = hs_page_base(src_page) + 0xf00U;
    const uintptr_t src_paddr = phys_page_base(src_page) + 0xf00U;
    const uintptr_t src_guard_vaddr = src_vaddr + AI_MEMBLOCK_CMO_VECTOR_BYTES;
    const uintptr_t src_guard_paddr = src_paddr + AI_MEMBLOCK_CMO_VECTOR_BYTES;
    const uintptr_t dst_base_vaddr = hs_page_base(dst_page) + 0xf00U;
    const uintptr_t dst_base_paddr = phys_page_base(dst_page) + 0xf00U;
    const uintptr_t dst_tail_vaddr =
        dst_base_vaddr +
        (AI_MEMBLOCK_CMO_AMO_BLOCK_WORDS - 1U) * sizeof(uint64_t);
    const uintptr_t dst_before_vaddr =
        dst_base_vaddr - sizeof(uint64_t);
    const uintptr_t dst_after_vaddr =
        dst_base_vaddr + AI_MEMBLOCK_CMO_AMO_BLOCK_WORDS *
        sizeof(uint64_t);
    const uintptr_t dst_before_paddr =
        dst_base_paddr - sizeof(uint64_t);
    const uintptr_t dst_after_paddr =
        dst_base_paddr + AI_MEMBLOCK_CMO_AMO_BLOCK_WORDS *
        sizeof(uint64_t);
    const uintptr_t amo_vaddr = hs_page_base(amo_page) + 0xa00U;
    const uintptr_t amo_paddr = phys_page_base(amo_page) + 0xa00U;
    const uintptr_t amo_before_vaddr = amo_vaddr - sizeof(uint64_t);
    const uintptr_t amo_after_vaddr = amo_vaddr + sizeof(uint64_t);
    const uintptr_t amo_before_paddr = amo_paddr - sizeof(uint64_t);
    const uintptr_t amo_after_paddr = amo_paddr + sizeof(uint64_t);
    const uintptr_t marker_vaddr = hs_page_base(amo_page) + 0xa80U;
    const uintptr_t marker_paddr = phys_page_base(amo_page) + 0xa80U;
    const uintptr_t marker_guard_vaddr = marker_vaddr + sizeof(uint64_t);
    const uintptr_t marker_guard_paddr = marker_paddr + sizeof(uint64_t);
    const uint64_t src_word0_values[] = {
        0x0123456789abcdefULL,
        0xfedcba9876543210ULL,
        0x8899aabbccddeeffULL,
    };
    const uint64_t src_word1_values[] = {
        0x13579bdf2468ace0ULL,
        0x02468ace13579bdfULL,
        0xfdb97531eca86420ULL,
    };
    const uint64_t dst_seeds[] = {
        0x21436587a9cbed0fULL,
        0x0f1e2d3c4b5a6978ULL,
        0x1122334455667788ULL,
    };
    const uint64_t amo_seeds[] = {
        0x1011223344556677ULL,
        0x2021334455667788ULL,
        0x3031445566778899ULL,
    };
    const uint64_t amo_addends[] = {
        0x0001000100010001ULL,
        0x0002000200020002ULL,
        0x0003000300030003ULL,
    };
    const uint64_t src_guards[] = {
        0xfeedfacecafebeefULL,
        0xdeadc0def00dbabeULL,
        0xfaceb00c13579bdfULL,
    };
    const uint64_t dst_before_guards[] = {
        0x0badf00d5a5aa5a5ULL,
        0x1020304050607080ULL,
        0x1122334455667788ULL,
    };
    const uint64_t dst_after_guards[] = {
        0xfdb97531eca86420ULL,
        0x13579bdf2468ace0ULL,
        0x02468ace13579bdfULL,
    };
    const uint64_t amo_before_guards[] = {
        0x1234fedc5678ba90ULL,
        0x0badc0de55aa33ccULL,
        0x55aa33cc7799bbddULL,
    };
    const uint64_t amo_after_guards[] = {
        0x1123581321345589ULL,
        0x3141592653589793ULL,
        0x2718281828459045ULL,
    };
    const uint64_t marker_seeds[] = {
        0xa0a1a2a3a4a5a6a7ULL,
        0xb0b1b2b3b4b5b6b7ULL,
        0xc0c1c2c3c4c5c6c7ULL,
    };
    const uint64_t marker_updates[] = {
        0x7766554433221100ULL,
        0x66554433221100ffULL,
        0x554433221100ffeeULL,
    };
    const uint64_t marker_guards[] = {
        0x89abcdef01234567ULL,
        0x76543210fedcba98ULL,
        0x55aa55aa33cc33ccULL,
    };

    goto_priv(PRIV_HS);
    hspt_init();
    sfence_vma();

    for (size_t round = 0; round < AI_MEMBLOCK_FLUSH_ARRAY_SIZE(dst_seeds);
         ++round) {
        const uint64_t expected_amo =
            amo_seeds[round] + amo_addends[round];
        uint64_t amo_old = 0ULL;
        uint64_t amo_observed = 0ULL;
        uint64_t marker_reloaded = 0ULL;

        goto_priv(PRIV_M);
        write64(src_paddr, src_word0_values[round]);
        write64(src_paddr + sizeof(uint64_t), src_word1_values[round]);
        write64(src_guard_paddr, src_guards[round]);
        ai_memblock_cmo_amo_seed_block(dst_base_paddr, dst_seeds[round]);
        write64(dst_before_paddr, dst_before_guards[round]);
        write64(dst_after_paddr, dst_after_guards[round]);
        write64(amo_paddr, amo_seeds[round]);
        write64(amo_before_paddr, amo_before_guards[round]);
        write64(amo_after_paddr, amo_after_guards[round]);
        write64(marker_paddr, marker_seeds[round]);
        write64(marker_guard_paddr, marker_guards[round]);

        goto_priv(PRIV_HS);
        TEST_SETUP_EXCEPT();
        const uint64_t warmed_src0 = ld(src_vaddr);
        const uint64_t warmed_src1 = ld(src_vaddr + sizeof(uint64_t));
        const uint64_t warmed_dst_tail = ld(dst_tail_vaddr);
        const uint64_t warmed_amo = ld(amo_vaddr);
        const uint64_t warmed_marker = ld(marker_vaddr);
        AI_ASSERT_NO_EXCEPT_AND("before the AMO then normal-vector store probe starts, source, destination tail, AMO target, and marker are readable with seeded values",
            warmed_src0 == src_word0_values[round] &&
            warmed_src1 == src_word1_values[round] &&
            warmed_dst_tail == dst_seeds[round] +
                (AI_MEMBLOCK_CMO_AMO_BLOCK_WORDS - 1U) *
                0x1111111111111111ULL &&
            warmed_amo == amo_seeds[round] &&
            warmed_marker == marker_seeds[round]
        );

        TEST_SETUP_EXCEPT();
        ai_memblock_cmo_amo_then_normal_vector_store_marker(amo_vaddr,
            amo_addends[round], src_vaddr, dst_base_vaddr, marker_vaddr,
            marker_updates[round], &amo_old, &amo_observed,
            &marker_reloaded);
        const uint64_t observed_dst0 = ld(dst_base_vaddr);
        const uint64_t observed_dst1 =
            ld(dst_base_vaddr + sizeof(uint64_t));
        const uint64_t observed_dst_tail = ld(dst_tail_vaddr);
        const uint64_t observed_marker = ld(marker_vaddr);
        AI_ASSERT_NO_EXCEPT_AND("AMO immediately followed by normal vector store releases atomic ownership before vector writeback, preserving AMO result, vector payload, and marker progress",
            amo_old == amo_seeds[round] &&
            amo_observed == expected_amo &&
            observed_dst0 == src_word0_values[round] &&
            observed_dst1 == src_word1_values[round] &&
            observed_dst_tail == dst_seeds[round] +
                (AI_MEMBLOCK_CMO_AMO_BLOCK_WORDS - 1U) *
                0x1111111111111111ULL &&
            observed_marker == marker_updates[round] &&
            marker_reloaded == marker_updates[round]
        );

        TEST_SETUP_EXCEPT();
        AI_ASSERT_NO_EXCEPT_AND("the AMO then normal-vector store boundary does not corrupt source, destination, AMO, or marker guards",
            ld(src_guard_vaddr) == src_guards[round] &&
            ld(dst_before_vaddr) == dst_before_guards[round] &&
            ld(dst_after_vaddr) == dst_after_guards[round] &&
            ld(amo_before_vaddr) == amo_before_guards[round] &&
            ld(amo_after_vaddr) == amo_after_guards[round] &&
            ld(marker_guard_vaddr) == marker_guards[round]
        );

        goto_priv(PRIV_M);
        TEST_ASSERT("backing memory after the AMO then normal-vector store window contains the AMO update, copied vector payload, marker update, and unchanged guards",
            read64(src_paddr) == src_word0_values[round] &&
            read64(src_paddr + sizeof(uint64_t)) ==
                src_word1_values[round] &&
            read64(src_guard_paddr) == src_guards[round] &&
            ai_memblock_cmo_amo_block_matches_two_updates(
                dst_base_paddr,
                dst_seeds[round],
                0U,
                src_word0_values[round],
                1U,
                src_word1_values[round]) &&
            read64(dst_before_paddr) == dst_before_guards[round] &&
            read64(dst_after_paddr) == dst_after_guards[round] &&
            read64(amo_paddr) == expected_amo &&
            read64(amo_before_paddr) == amo_before_guards[round] &&
            read64(amo_after_paddr) == amo_after_guards[round] &&
            read64(marker_paddr) == marker_updates[round] &&
            read64(marker_guard_paddr) == marker_guards[round]
        );
    }

    goto_priv(PRIV_HS);
    hspt_init();
    sfence_vma();

    TEST_END();
}

bool ai_micro_hs_lrsc_then_normal_vector_store_no_stale_reservation_owner_or_vector_payload_corner()
{
    TEST_START();

    const enum test_page src_page = SWITCH1;
    const enum test_page dst_page = SWITCH2;
    const enum test_page lrsc_page = VSRWX_GRWX;
    const uintptr_t src_vaddr = hs_page_base(src_page) + 0xf80U;
    const uintptr_t src_paddr = phys_page_base(src_page) + 0xf80U;
    const uintptr_t src_guard_vaddr = src_vaddr + AI_MEMBLOCK_CMO_VECTOR_BYTES;
    const uintptr_t src_guard_paddr = src_paddr + AI_MEMBLOCK_CMO_VECTOR_BYTES;
    const uintptr_t dst_base_vaddr = hs_page_base(dst_page) + 0xf80U;
    const uintptr_t dst_base_paddr = phys_page_base(dst_page) + 0xf80U;
    const uintptr_t dst_tail_vaddr =
        dst_base_vaddr +
        (AI_MEMBLOCK_CMO_AMO_BLOCK_WORDS - 1U) * sizeof(uint64_t);
    const uintptr_t dst_before_vaddr =
        dst_base_vaddr - sizeof(uint64_t);
    const uintptr_t dst_after_vaddr =
        dst_base_vaddr + AI_MEMBLOCK_CMO_AMO_BLOCK_WORDS *
        sizeof(uint64_t);
    const uintptr_t dst_before_paddr =
        dst_base_paddr - sizeof(uint64_t);
    const uintptr_t dst_after_paddr =
        dst_base_paddr + AI_MEMBLOCK_CMO_AMO_BLOCK_WORDS *
        sizeof(uint64_t);
    const uintptr_t lrsc_vaddr = hs_page_base(lrsc_page) + 0xb00U;
    const uintptr_t lrsc_paddr = phys_page_base(lrsc_page) + 0xb00U;
    const uintptr_t lrsc_before_vaddr = lrsc_vaddr - sizeof(uint64_t);
    const uintptr_t lrsc_after_vaddr = lrsc_vaddr + sizeof(uint64_t);
    const uintptr_t lrsc_before_paddr = lrsc_paddr - sizeof(uint64_t);
    const uintptr_t lrsc_after_paddr = lrsc_paddr + sizeof(uint64_t);
    const uintptr_t marker_vaddr = hs_page_base(lrsc_page) + 0xb80U;
    const uintptr_t marker_paddr = phys_page_base(lrsc_page) + 0xb80U;
    const uintptr_t marker_guard_vaddr = marker_vaddr + sizeof(uint64_t);
    const uintptr_t marker_guard_paddr = marker_paddr + sizeof(uint64_t);
    const uint64_t src_word0_values[] = {
        0x1020304050607080ULL,
        0x8877665544332211ULL,
        0x13579bdf2468ace0ULL,
    };
    const uint64_t src_word1_values[] = {
        0x0102030405060708ULL,
        0xfedcba9876543210ULL,
        0x02468ace13579bdfULL,
    };
    const uint64_t dst_seeds[] = {
        0x3141592653589793ULL,
        0x2718281828459045ULL,
        0x1123581321345589ULL,
    };
    const uint64_t lrsc_seeds[] = {
        0x1011223344556677ULL,
        0x2021334455667788ULL,
        0x3031445566778899ULL,
    };
    const uint64_t sc_payloads[] = {
        0x1111222233334444ULL,
        0x5555666677778888ULL,
        0x9999aaaabbbbccccULL,
    };
    const uint64_t src_guards[] = {
        0xfeedfacecafebeefULL,
        0xdeadc0def00dbabeULL,
        0xfaceb00c13579bdfULL,
    };
    const uint64_t dst_before_guards[] = {
        0x0badf00d5a5aa5a5ULL,
        0x1020304050607080ULL,
        0x1122334455667788ULL,
    };
    const uint64_t dst_after_guards[] = {
        0xfdb97531eca86420ULL,
        0x13579bdf2468ace0ULL,
        0x02468ace13579bdfULL,
    };
    const uint64_t lrsc_before_guards[] = {
        0x1234fedc5678ba90ULL,
        0x0badc0de55aa33ccULL,
        0x55aa33cc7799bbddULL,
    };
    const uint64_t lrsc_after_guards[] = {
        0x1123581321345589ULL,
        0x3141592653589793ULL,
        0x2718281828459045ULL,
    };
    const uint64_t marker_seeds[] = {
        0xa0a1a2a3a4a5a6a7ULL,
        0xb0b1b2b3b4b5b6b7ULL,
        0xc0c1c2c3c4c5c6c7ULL,
    };
    const uint64_t marker_updates[] = {
        0x7766554433221100ULL,
        0x66554433221100ffULL,
        0x554433221100ffeeULL,
    };
    const uint64_t marker_guards[] = {
        0x89abcdef01234567ULL,
        0x76543210fedcba98ULL,
        0x55aa55aa33cc33ccULL,
    };

    goto_priv(PRIV_HS);
    hspt_init();
    sfence_vma();

    for (size_t round = 0; round < AI_MEMBLOCK_FLUSH_ARRAY_SIZE(dst_seeds);
         ++round) {
        uint64_t lr_observed = 0ULL;
        uint64_t sc_status = 1ULL;
        uint64_t lrsc_reloaded = 0ULL;
        uint64_t marker_reloaded = 0ULL;

        goto_priv(PRIV_M);
        write64(src_paddr, src_word0_values[round]);
        write64(src_paddr + sizeof(uint64_t), src_word1_values[round]);
        write64(src_guard_paddr, src_guards[round]);
        ai_memblock_cmo_amo_seed_block(dst_base_paddr, dst_seeds[round]);
        write64(dst_before_paddr, dst_before_guards[round]);
        write64(dst_after_paddr, dst_after_guards[round]);
        write64(lrsc_paddr, lrsc_seeds[round]);
        write64(lrsc_before_paddr, lrsc_before_guards[round]);
        write64(lrsc_after_paddr, lrsc_after_guards[round]);
        write64(marker_paddr, marker_seeds[round]);
        write64(marker_guard_paddr, marker_guards[round]);

        goto_priv(PRIV_HS);
        TEST_SETUP_EXCEPT();
        const uint64_t warmed_src0 = ld(src_vaddr);
        const uint64_t warmed_src1 = ld(src_vaddr + sizeof(uint64_t));
        const uint64_t warmed_dst_tail = ld(dst_tail_vaddr);
        const uint64_t warmed_lrsc = ld(lrsc_vaddr);
        const uint64_t warmed_marker = ld(marker_vaddr);
        AI_ASSERT_NO_EXCEPT_AND("before the LR/SC then normal-vector store probe starts, source, destination tail, LR/SC target, and marker are readable with seeded values",
            warmed_src0 == src_word0_values[round] &&
            warmed_src1 == src_word1_values[round] &&
            warmed_dst_tail == dst_seeds[round] +
                (AI_MEMBLOCK_CMO_AMO_BLOCK_WORDS - 1U) *
                0x1111111111111111ULL &&
            warmed_lrsc == lrsc_seeds[round] &&
            warmed_marker == marker_seeds[round]
        );

        TEST_SETUP_EXCEPT();
        ai_memblock_cmo_lrsc_then_normal_vector_store_marker(lrsc_vaddr,
            sc_payloads[round], src_vaddr, dst_base_vaddr, marker_vaddr,
            marker_updates[round], &lr_observed, &sc_status,
            &lrsc_reloaded, &marker_reloaded);
        const uint64_t observed_dst0 = ld(dst_base_vaddr);
        const uint64_t observed_dst1 =
            ld(dst_base_vaddr + sizeof(uint64_t));
        const uint64_t observed_dst_tail = ld(dst_tail_vaddr);
        const uint64_t observed_marker = ld(marker_vaddr);
        AI_ASSERT_NO_EXCEPT_AND("LR/SC immediately followed by normal vector store releases reservation ownership before vector writeback, preserving SC result, vector payload, and marker progress",
            lr_observed == lrsc_seeds[round] &&
            sc_status == 0ULL &&
            lrsc_reloaded == sc_payloads[round] &&
            observed_dst0 == src_word0_values[round] &&
            observed_dst1 == src_word1_values[round] &&
            observed_dst_tail == dst_seeds[round] +
                (AI_MEMBLOCK_CMO_AMO_BLOCK_WORDS - 1U) *
                0x1111111111111111ULL &&
            observed_marker == marker_updates[round] &&
            marker_reloaded == marker_updates[round]
        );

        TEST_SETUP_EXCEPT();
        AI_ASSERT_NO_EXCEPT_AND("the LR/SC then normal-vector store boundary does not corrupt source, destination, LR/SC, or marker guards",
            ld(src_guard_vaddr) == src_guards[round] &&
            ld(dst_before_vaddr) == dst_before_guards[round] &&
            ld(dst_after_vaddr) == dst_after_guards[round] &&
            ld(lrsc_before_vaddr) == lrsc_before_guards[round] &&
            ld(lrsc_after_vaddr) == lrsc_after_guards[round] &&
            ld(marker_guard_vaddr) == marker_guards[round]
        );

        goto_priv(PRIV_M);
        TEST_ASSERT("backing memory after the LR/SC then normal-vector store window contains the SC update, copied vector payload, marker update, and unchanged guards",
            read64(src_paddr) == src_word0_values[round] &&
            read64(src_paddr + sizeof(uint64_t)) ==
                src_word1_values[round] &&
            read64(src_guard_paddr) == src_guards[round] &&
            ai_memblock_cmo_amo_block_matches_two_updates(
                dst_base_paddr,
                dst_seeds[round],
                0U,
                src_word0_values[round],
                1U,
                src_word1_values[round]) &&
            read64(dst_before_paddr) == dst_before_guards[round] &&
            read64(dst_after_paddr) == dst_after_guards[round] &&
            read64(lrsc_paddr) == sc_payloads[round] &&
            read64(lrsc_before_paddr) == lrsc_before_guards[round] &&
            read64(lrsc_after_paddr) == lrsc_after_guards[round] &&
            read64(marker_paddr) == marker_updates[round] &&
            read64(marker_guard_paddr) == marker_guards[round]
        );
    }

    goto_priv(PRIV_HS);
    hspt_init();
    sfence_vma();

    TEST_END();
}

bool ai_micro_hs_normal_vector_store_then_amo_no_stale_vector_writeback_or_atomic_old_value_corner()
{
    TEST_START();

    const enum test_page src_page = SWITCH1;
    const enum test_page dst_page = SWITCH2;
    const enum test_page amo_page = VSRWX_GRWX;
    const uintptr_t src_vaddr = hs_page_base(src_page) + 0x100U;
    const uintptr_t src_paddr = phys_page_base(src_page) + 0x100U;
    const uintptr_t src_guard_vaddr = src_vaddr + AI_MEMBLOCK_CMO_VECTOR_BYTES;
    const uintptr_t src_guard_paddr = src_paddr + AI_MEMBLOCK_CMO_VECTOR_BYTES;
    const uintptr_t dst_base_vaddr = hs_page_base(dst_page) + 0x100U;
    const uintptr_t dst_base_paddr = phys_page_base(dst_page) + 0x100U;
    const uintptr_t dst_tail_vaddr =
        dst_base_vaddr +
        (AI_MEMBLOCK_CMO_AMO_BLOCK_WORDS - 1U) * sizeof(uint64_t);
    const uintptr_t dst_before_vaddr =
        dst_base_vaddr - sizeof(uint64_t);
    const uintptr_t dst_after_vaddr =
        dst_base_vaddr + AI_MEMBLOCK_CMO_AMO_BLOCK_WORDS *
        sizeof(uint64_t);
    const uintptr_t dst_before_paddr =
        dst_base_paddr - sizeof(uint64_t);
    const uintptr_t dst_after_paddr =
        dst_base_paddr + AI_MEMBLOCK_CMO_AMO_BLOCK_WORDS *
        sizeof(uint64_t);
    const uintptr_t amo_vaddr = hs_page_base(amo_page) + 0xc00U;
    const uintptr_t amo_paddr = phys_page_base(amo_page) + 0xc00U;
    const uintptr_t amo_before_vaddr = amo_vaddr - sizeof(uint64_t);
    const uintptr_t amo_after_vaddr = amo_vaddr + sizeof(uint64_t);
    const uintptr_t amo_before_paddr = amo_paddr - sizeof(uint64_t);
    const uintptr_t amo_after_paddr = amo_paddr + sizeof(uint64_t);
    const uintptr_t marker_vaddr = hs_page_base(amo_page) + 0xc80U;
    const uintptr_t marker_paddr = phys_page_base(amo_page) + 0xc80U;
    const uintptr_t marker_guard_vaddr = marker_vaddr + sizeof(uint64_t);
    const uintptr_t marker_guard_paddr = marker_paddr + sizeof(uint64_t);
    const uint64_t src_word0_values[] = {
        0x0123456789abcdefULL,
        0xfedcba9876543210ULL,
        0x8899aabbccddeeffULL,
    };
    const uint64_t src_word1_values[] = {
        0x13579bdf2468ace0ULL,
        0x02468ace13579bdfULL,
        0xfdb97531eca86420ULL,
    };
    const uint64_t dst_seeds[] = {
        0x21436587a9cbed0fULL,
        0x0f1e2d3c4b5a6978ULL,
        0x1122334455667788ULL,
    };
    const uint64_t amo_seeds[] = {
        0x0101010101010101ULL,
        0x2121212121212121ULL,
        0x4141414141414141ULL,
    };
    const uint64_t amo_addends[] = {
        0x0000000000000101ULL,
        0x0000000000000202ULL,
        0x0000000000000303ULL,
    };
    const uint64_t src_guards[] = {
        0xfeedfacecafebeefULL,
        0xdeadc0def00dbabeULL,
        0xfaceb00c13579bdfULL,
    };
    const uint64_t dst_before_guards[] = {
        0x0badf00d5a5aa5a5ULL,
        0x1020304050607080ULL,
        0x1122334455667788ULL,
    };
    const uint64_t dst_after_guards[] = {
        0xfdb97531eca86420ULL,
        0x13579bdf2468ace0ULL,
        0x02468ace13579bdfULL,
    };
    const uint64_t amo_before_guards[] = {
        0x1234fedc5678ba90ULL,
        0x0badc0de55aa33ccULL,
        0x55aa33cc7799bbddULL,
    };
    const uint64_t amo_after_guards[] = {
        0x1123581321345589ULL,
        0x3141592653589793ULL,
        0x2718281828459045ULL,
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
        0x89abcdef01234567ULL,
        0x76543210fedcba98ULL,
        0x55aa55aa33cc33ccULL,
    };

    goto_priv(PRIV_HS);
    hspt_init();
    sfence_vma();

    for (size_t round = 0; round < AI_MEMBLOCK_FLUSH_ARRAY_SIZE(dst_seeds);
         ++round) {
        const uint64_t expected_amo =
            amo_seeds[round] + amo_addends[round];
        uint64_t amo_old = 0ULL;
        uint64_t amo_observed = 0ULL;
        uint64_t marker_reloaded = 0ULL;

        goto_priv(PRIV_M);
        write64(src_paddr, src_word0_values[round]);
        write64(src_paddr + sizeof(uint64_t), src_word1_values[round]);
        write64(src_guard_paddr, src_guards[round]);
        ai_memblock_cmo_amo_seed_block(dst_base_paddr, dst_seeds[round]);
        write64(dst_before_paddr, dst_before_guards[round]);
        write64(dst_after_paddr, dst_after_guards[round]);
        write64(amo_paddr, amo_seeds[round]);
        write64(amo_before_paddr, amo_before_guards[round]);
        write64(amo_after_paddr, amo_after_guards[round]);
        write64(marker_paddr, marker_seeds[round]);
        write64(marker_guard_paddr, marker_guards[round]);

        goto_priv(PRIV_HS);
        TEST_SETUP_EXCEPT();
        const uint64_t warmed_src0 = ld(src_vaddr);
        const uint64_t warmed_src1 = ld(src_vaddr + sizeof(uint64_t));
        const uint64_t warmed_dst_tail = ld(dst_tail_vaddr);
        const uint64_t warmed_amo = ld(amo_vaddr);
        const uint64_t warmed_marker = ld(marker_vaddr);
        AI_ASSERT_NO_EXCEPT_AND("before the normal-vector store then AMO probe starts, source, destination tail, AMO target, and marker are readable with seeded values",
            warmed_src0 == src_word0_values[round] &&
            warmed_src1 == src_word1_values[round] &&
            warmed_dst_tail == dst_seeds[round] +
                (AI_MEMBLOCK_CMO_AMO_BLOCK_WORDS - 1U) *
                0x1111111111111111ULL &&
            warmed_amo == amo_seeds[round] &&
            warmed_marker == marker_seeds[round]
        );

        TEST_SETUP_EXCEPT();
        ai_memblock_cmo_normal_vector_store_then_amo_marker(src_vaddr,
            dst_base_vaddr, amo_vaddr, amo_addends[round], marker_vaddr,
            marker_updates[round], &amo_old, &amo_observed,
            &marker_reloaded);
        const uint64_t observed_dst0 = ld(dst_base_vaddr);
        const uint64_t observed_dst1 =
            ld(dst_base_vaddr + sizeof(uint64_t));
        const uint64_t observed_dst_tail = ld(dst_tail_vaddr);
        const uint64_t observed_marker = ld(marker_vaddr);
        AI_ASSERT_NO_EXCEPT_AND("normal vector store immediately followed by AMO releases vector writeback before atomic old-value capture, preserving copied payload, AMO result, and marker progress",
            amo_old == amo_seeds[round] &&
            amo_observed == expected_amo &&
            observed_dst0 == src_word0_values[round] &&
            observed_dst1 == src_word1_values[round] &&
            observed_dst_tail == dst_seeds[round] +
                (AI_MEMBLOCK_CMO_AMO_BLOCK_WORDS - 1U) *
                0x1111111111111111ULL &&
            observed_marker == marker_updates[round] &&
            marker_reloaded == marker_updates[round]
        );

        TEST_SETUP_EXCEPT();
        AI_ASSERT_NO_EXCEPT_AND("the normal-vector store then AMO boundary does not corrupt source, destination, AMO, or marker guards",
            ld(src_guard_vaddr) == src_guards[round] &&
            ld(dst_before_vaddr) == dst_before_guards[round] &&
            ld(dst_after_vaddr) == dst_after_guards[round] &&
            ld(amo_before_vaddr) == amo_before_guards[round] &&
            ld(amo_after_vaddr) == amo_after_guards[round] &&
            ld(marker_guard_vaddr) == marker_guards[round]
        );

        goto_priv(PRIV_M);
        TEST_ASSERT("backing memory after the normal-vector store then AMO window contains the copied vector payload, AMO update, marker update, and unchanged guards",
            read64(src_paddr) == src_word0_values[round] &&
            read64(src_paddr + sizeof(uint64_t)) ==
                src_word1_values[round] &&
            read64(src_guard_paddr) == src_guards[round] &&
            ai_memblock_cmo_amo_block_matches_two_updates(
                dst_base_paddr,
                dst_seeds[round],
                0U,
                src_word0_values[round],
                1U,
                src_word1_values[round]) &&
            read64(dst_before_paddr) == dst_before_guards[round] &&
            read64(dst_after_paddr) == dst_after_guards[round] &&
            read64(amo_paddr) == expected_amo &&
            read64(amo_before_paddr) == amo_before_guards[round] &&
            read64(amo_after_paddr) == amo_after_guards[round] &&
            read64(marker_paddr) == marker_updates[round] &&
            read64(marker_guard_paddr) == marker_guards[round]
        );
    }

    goto_priv(PRIV_HS);
    hspt_init();
    sfence_vma();

    TEST_END();
}
