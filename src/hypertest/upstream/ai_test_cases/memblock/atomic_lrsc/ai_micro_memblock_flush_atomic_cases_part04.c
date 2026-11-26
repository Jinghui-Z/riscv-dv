#include "ai_micro_memblock_flush_atomic_common.h"

bool ai_micro_hs_normal_vector_store_then_lrsc_no_stale_vector_writeback_or_reservation_loss_corner()
{
    TEST_START();

    const enum test_page src_page = SWITCH1;
    const enum test_page dst_page = SWITCH2;
    const enum test_page lrsc_page = VSRWX_GRWX;
    const uintptr_t src_vaddr = hs_page_base(src_page) + 0x180U;
    const uintptr_t src_paddr = phys_page_base(src_page) + 0x180U;
    const uintptr_t src_guard_vaddr = src_vaddr + AI_MEMBLOCK_CMO_VECTOR_BYTES;
    const uintptr_t src_guard_paddr = src_paddr + AI_MEMBLOCK_CMO_VECTOR_BYTES;
    const uintptr_t dst_base_vaddr = hs_page_base(dst_page) + 0x180U;
    const uintptr_t dst_base_paddr = phys_page_base(dst_page) + 0x180U;
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
    const uintptr_t lrsc_vaddr = hs_page_base(lrsc_page) + 0xd00U;
    const uintptr_t lrsc_paddr = phys_page_base(lrsc_page) + 0xd00U;
    const uintptr_t lrsc_before_vaddr = lrsc_vaddr - sizeof(uint64_t);
    const uintptr_t lrsc_after_vaddr = lrsc_vaddr + sizeof(uint64_t);
    const uintptr_t lrsc_before_paddr = lrsc_paddr - sizeof(uint64_t);
    const uintptr_t lrsc_after_paddr = lrsc_paddr + sizeof(uint64_t);
    const uintptr_t marker_vaddr = hs_page_base(lrsc_page) + 0xd80U;
    const uintptr_t marker_paddr = phys_page_base(lrsc_page) + 0xd80U;
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
    const uint64_t lrsc_seeds[] = {
        0x1011223344556677ULL,
        0x2021334455667788ULL,
        0x3031445566778899ULL,
    };
    const uint64_t sc_payloads[] = {
        0x0102030405060708ULL,
        0x1827364554637281ULL,
        0xfedcba9876543210ULL,
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
        AI_ASSERT_NO_EXCEPT_AND("before the normal-vector store then LR/SC probe starts, source, destination tail, LR/SC target, and marker are readable with seeded values",
            warmed_src0 == src_word0_values[round] &&
            warmed_src1 == src_word1_values[round] &&
            warmed_dst_tail == dst_seeds[round] +
                (AI_MEMBLOCK_CMO_AMO_BLOCK_WORDS - 1U) *
                0x1111111111111111ULL &&
            warmed_lrsc == lrsc_seeds[round] &&
            warmed_marker == marker_seeds[round]
        );

        TEST_SETUP_EXCEPT();
        ai_memblock_cmo_normal_vector_store_then_lrsc_marker(src_vaddr,
            dst_base_vaddr, lrsc_vaddr, sc_payloads[round], marker_vaddr,
            marker_updates[round], &lr_observed, &sc_status,
            &lrsc_reloaded, &marker_reloaded);
        const uint64_t observed_dst0 = ld(dst_base_vaddr);
        const uint64_t observed_dst1 =
            ld(dst_base_vaddr + sizeof(uint64_t));
        const uint64_t observed_dst_tail = ld(dst_tail_vaddr);
        const uint64_t observed_marker = ld(marker_vaddr);
        AI_ASSERT_NO_EXCEPT_AND("normal vector store immediately followed by LR/SC releases vector writeback before reservation handling, preserving copied payload, successful SC result, and marker progress",
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
        AI_ASSERT_NO_EXCEPT_AND("the normal-vector store then LR/SC boundary does not corrupt source, destination, LR/SC, or marker guards",
            ld(src_guard_vaddr) == src_guards[round] &&
            ld(dst_before_vaddr) == dst_before_guards[round] &&
            ld(dst_after_vaddr) == dst_after_guards[round] &&
            ld(lrsc_before_vaddr) == lrsc_before_guards[round] &&
            ld(lrsc_after_vaddr) == lrsc_after_guards[round] &&
            ld(marker_guard_vaddr) == marker_guards[round]
        );

        goto_priv(PRIV_M);
        TEST_ASSERT("backing memory after the normal-vector store then LR/SC window contains the copied vector payload, SC update, marker update, and unchanged guards",
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

bool ai_micro_hs_cbom_then_vsegment_no_stale_cmo_flush_owner_or_segment_payload_corner()
{
    TEST_START();

    const enum test_page src_page = SWITCH1;
    const enum test_page dst_page = SWITCH2;
    const enum test_page marker_page = VSRWX_GRWX;
    const uintptr_t src_base_vaddr = hs_page_base(src_page) + 0xb00U;
    const uintptr_t src_base_paddr = phys_page_base(src_page) + 0xb00U;
    const uintptr_t src_word1_vaddr = src_base_vaddr + sizeof(uint64_t);
    const uintptr_t src_tail_vaddr =
        src_base_vaddr +
        (AI_MEMBLOCK_CMO_AMO_BLOCK_WORDS - 1U) * sizeof(uint64_t);
    const uintptr_t src_before_vaddr = src_base_vaddr - sizeof(uint64_t);
    const uintptr_t src_after_vaddr =
        src_base_vaddr + AI_MEMBLOCK_CMO_AMO_BLOCK_WORDS *
        sizeof(uint64_t);
    const uintptr_t src_before_paddr = src_base_paddr - sizeof(uint64_t);
    const uintptr_t src_after_paddr =
        src_base_paddr + AI_MEMBLOCK_CMO_AMO_BLOCK_WORDS *
        sizeof(uint64_t);
    const uintptr_t dst_vaddr = hs_page_base(dst_page) + 0xb00U;
    const uintptr_t dst_paddr = phys_page_base(dst_page) + 0xb00U;
    const uintptr_t dst_before_vaddr = dst_vaddr - sizeof(uint64_t);
    const uintptr_t dst_after_vaddr = dst_vaddr + AI_MEMBLOCK_CMO_VECTOR_BYTES;
    const uintptr_t dst_before_paddr = dst_paddr - sizeof(uint64_t);
    const uintptr_t dst_after_paddr = dst_paddr + AI_MEMBLOCK_CMO_VECTOR_BYTES;
    const uintptr_t marker_vaddr = hs_page_base(marker_page) + 0xb80U;
    const uintptr_t marker_paddr = phys_page_base(marker_page) + 0xb80U;
    const uintptr_t marker_guard_vaddr = marker_vaddr + sizeof(uint64_t);
    const uintptr_t marker_guard_paddr = marker_paddr + sizeof(uint64_t);
    const uint64_t block_seeds[] = {
        0x21436587a9cbed0fULL,
        0x1029384756abcedfULL,
        0x0f0e0d0c0b0a0908ULL,
    };
    const uint64_t src_word0_updates[] = {
        0x0123456789abcdefULL,
        0xfedcba9876543210ULL,
        0x8877665544332211ULL,
    };
    const uint64_t src_word1_updates[] = {
        0x13579bdf2468ace0ULL,
        0x02468ace13579bdfULL,
        0x1123581321345589ULL,
    };
    const uint64_t dst_fills[] = {
        0xa5a55a5ac3c33c3cULL,
        0x5a5aa5a53c3cc3c3ULL,
        0xdeadbeefcafebabeULL,
    };
    const uint64_t src_before_guards[] = {
        0x0badf00d5a5aa5a5ULL,
        0x1020304050607080ULL,
        0x1122334455667788ULL,
    };
    const uint64_t src_after_guards[] = {
        0xfeedfacecafebeefULL,
        0xdeadc0def00dbabeULL,
        0xfaceb00c13579bdfULL,
    };
    const uint64_t dst_before_guards[] = {
        0x3141592653589793ULL,
        0x2718281828459045ULL,
        0x1123581321345589ULL,
    };
    const uint64_t dst_after_guards[] = {
        0xfdb97531eca86420ULL,
        0x13579bdf2468ace0ULL,
        0x02468ace13579bdfULL,
    };
    const uint64_t marker_seeds[] = {
        0xc001d00dcafef00dULL,
        0xd15ea5edfeedfaceULL,
        0xabcdef0123456789ULL,
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

    ai_memblock_cmo_amo_prepare_hs_env();

    for (size_t round = 0; round < AI_MEMBLOCK_FLUSH_ARRAY_SIZE(block_seeds);
         ++round) {
        goto_priv(PRIV_M);
        ai_memblock_cmo_amo_seed_block(src_base_paddr, block_seeds[round]);
        write64(src_before_paddr, src_before_guards[round]);
        write64(src_after_paddr, src_after_guards[round]);
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
        AI_ASSERT_NO_EXCEPT_AND("before the CBOM plus VSegment owner-window probe starts, source block and marker are readable with seeded values",
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
        ai_memblock_cmo_vsegment_copy_then_marker(src_base_vaddr,
            dst_vaddr, marker_vaddr, marker_updates[round]);
        const uint64_t observed_src0 = ld(src_base_vaddr);
        const uint64_t observed_src1 = ld(src_word1_vaddr);
        const uint64_t observed_tail = ld(src_tail_vaddr);
        const uint64_t observed_dst0 = ld(dst_vaddr);
        const uint64_t observed_dst1 = ld(dst_vaddr + sizeof(uint64_t));
        const uint64_t observed_marker = ld(marker_vaddr);
        AI_ASSERT_NO_EXCEPT_AND("CBOM clean/flush immediately followed by VSegment load/store keeps CMO flush ownership separate so segment payload sees the drained source stores",
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
        AI_ASSERT_NO_EXCEPT_AND("the CBOM plus VSegment owner-window does not corrupt source, destination, or marker guards",
            ld(src_before_vaddr) == src_before_guards[round] &&
            ld(src_after_vaddr) == src_after_guards[round] &&
            ld(dst_before_vaddr) == dst_before_guards[round] &&
            ld(dst_after_vaddr) == dst_after_guards[round] &&
            ld(marker_guard_vaddr) == marker_guards[round]
        );

        goto_priv(PRIV_M);
        TEST_ASSERT("backing memory after the CBOM plus VSegment owner-window contains the drained source stores, copied segment destination, marker update, and unchanged guards",
            ai_memblock_cmo_amo_block_matches_two_updates(
                src_base_paddr,
                block_seeds[round],
                0U,
                src_word0_updates[round],
                1U,
                src_word1_updates[round]) &&
            read64(src_before_paddr) == src_before_guards[round] &&
            read64(src_after_paddr) == src_after_guards[round] &&
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

bool ai_micro_hs_vsegment_then_cbom_no_stale_segment_store_or_cmo_owner_corner()
{
    TEST_START();

    const enum test_page src_page = SWITCH1;
    const enum test_page dst_page = SWITCH2;
    const enum test_page marker_page = VSRWX_GRWX;
    const uintptr_t src_vaddr = hs_page_base(src_page) + 0xc00U;
    const uintptr_t src_paddr = phys_page_base(src_page) + 0xc00U;
    const uintptr_t src_guard_vaddr = src_vaddr + AI_MEMBLOCK_CMO_VECTOR_BYTES;
    const uintptr_t src_guard_paddr = src_paddr + AI_MEMBLOCK_CMO_VECTOR_BYTES;
    const uintptr_t dst_base_vaddr = hs_page_base(dst_page) + 0xc00U;
    const uintptr_t dst_base_paddr = phys_page_base(dst_page) + 0xc00U;
    const uintptr_t dst_tail_vaddr =
        dst_base_vaddr +
        (AI_MEMBLOCK_CMO_AMO_BLOCK_WORDS - 1U) * sizeof(uint64_t);
    const uintptr_t dst_before_vaddr = dst_base_vaddr - sizeof(uint64_t);
    const uintptr_t dst_after_vaddr =
        dst_base_vaddr + AI_MEMBLOCK_CMO_AMO_BLOCK_WORDS *
        sizeof(uint64_t);
    const uintptr_t dst_before_paddr = dst_base_paddr - sizeof(uint64_t);
    const uintptr_t dst_after_paddr =
        dst_base_paddr + AI_MEMBLOCK_CMO_AMO_BLOCK_WORDS *
        sizeof(uint64_t);
    const uintptr_t marker_vaddr = hs_page_base(marker_page) + 0xc80U;
    const uintptr_t marker_paddr = phys_page_base(marker_page) + 0xc80U;
    const uintptr_t marker_guard_vaddr = marker_vaddr + sizeof(uint64_t);
    const uintptr_t marker_guard_paddr = marker_paddr + sizeof(uint64_t);
    const uint64_t src_word0_values[] = {
        0x3141592653589793ULL,
        0x2718281828459045ULL,
        0x1123581321345589ULL,
    };
    const uint64_t src_word1_values[] = {
        0x89abcdef01234567ULL,
        0x76543210fedcba98ULL,
        0x55aa55aa33cc33ccULL,
    };
    const uint64_t dst_seeds[] = {
        0x0a1b2c3d4e5f6071ULL,
        0x8192a3b4c5d6e7f8ULL,
        0xf0e1d2c3b4a59687ULL,
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
        0x0123456789abcdefULL,
        0xfedcba9876543210ULL,
        0x8877665544332211ULL,
    };
    const uint64_t marker_guards[] = {
        0x3141592653589793ULL,
        0x2718281828459045ULL,
        0x1123581321345589ULL,
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
        AI_ASSERT_NO_EXCEPT_AND("before the VSegment plus CBOM owner-window probe starts, source, destination tail, and marker are readable with seeded values",
            warmed_src0 == src_word0_values[round] &&
            warmed_src1 == src_word1_values[round] &&
            warmed_dst_tail == dst_seeds[round] +
                (AI_MEMBLOCK_CMO_AMO_BLOCK_WORDS - 1U) *
                0x1111111111111111ULL &&
            warmed_marker == marker_seeds[round]
        );

        TEST_SETUP_EXCEPT();
        ai_memblock_cmo_vsegment_copy_only(src_vaddr, dst_base_vaddr);
        cbo_clean(dst_base_vaddr);
        cbo_flush(dst_tail_vaddr);
        sd(marker_vaddr, marker_updates[round]);
        fence_iorw_iorw();
        const uint64_t observed_dst0 = ld(dst_base_vaddr);
        const uint64_t observed_dst1 =
            ld(dst_base_vaddr + sizeof(uint64_t));
        const uint64_t observed_dst_tail = ld(dst_tail_vaddr);
        const uint64_t observed_marker = ld(marker_vaddr);
        AI_ASSERT_NO_EXCEPT_AND("VSegment load/store immediately followed by CBOM clean/flush keeps segment store and CMO ownership ordered so the copied payload and marker both survive",
            observed_dst0 == src_word0_values[round] &&
            observed_dst1 == src_word1_values[round] &&
            observed_dst_tail == dst_seeds[round] +
                (AI_MEMBLOCK_CMO_AMO_BLOCK_WORDS - 1U) *
                0x1111111111111111ULL &&
            observed_marker == marker_updates[round]
        );

        TEST_SETUP_EXCEPT();
        AI_ASSERT_NO_EXCEPT_AND("the VSegment plus CBOM owner-window does not corrupt source, destination, or marker guards",
            ld(src_guard_vaddr) == src_guards[round] &&
            ld(dst_before_vaddr) == dst_before_guards[round] &&
            ld(dst_after_vaddr) == dst_after_guards[round] &&
            ld(marker_guard_vaddr) == marker_guards[round]
        );

        goto_priv(PRIV_M);
        TEST_ASSERT("backing memory after the VSegment plus CBOM owner-window contains the copied segment payload, marker update, and unchanged guards",
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

bool ai_micro_hs_cbom_then_fence_no_stale_cmo_flush_owner_or_marker_drop_corner()
{
    TEST_START();

    const enum test_page target_page = SWITCH2;
    const enum test_page marker_page = VSRWX_GRWX;
    const uintptr_t target_base_vaddr = hs_page_base(target_page) + 0x340U;
    const uintptr_t target_base_paddr = phys_page_base(target_page) + 0x340U;
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
    const uintptr_t marker_vaddr = hs_page_base(marker_page) + 0x340U;
    const uintptr_t marker_paddr = phys_page_base(marker_page) + 0x340U;
    const uintptr_t marker_guard_vaddr = marker_vaddr + sizeof(uint64_t);
    const uintptr_t marker_guard_paddr = marker_paddr + sizeof(uint64_t);
    const uint64_t block_seeds[] = {
        0x1223344556677889ULL,
        0x233445566778899aULL,
        0x3445566778899aabULL,
    };
    const uint64_t target_updates[] = {
        0x0102030405060708ULL,
        0x1112131415161718ULL,
        0x2122232425262728ULL,
    };
    const uint64_t neighbor_updates[] = {
        0x8899aabbccddeeffULL,
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
        0x1234fedc5678ba90ULL,
        0x0badc0de55aa33ccULL,
        0x55aa33cc7799bbddULL,
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
        const uint64_t warmed_neighbor = ld(neighbor_word_vaddr);
        const uint64_t warmed_tail = ld(target_tail_vaddr);
        const uint64_t warmed_marker = ld(marker_vaddr);
        AI_ASSERT_NO_EXCEPT_AND("before the CBOM then fence owner-window starts, target block and marker are readable with seeded values",
            warmed_target == block_seeds[round] &&
            warmed_neighbor == block_seeds[round] +
                3U * 0x1111111111111111ULL &&
            warmed_tail == block_seeds[round] +
                (AI_MEMBLOCK_CMO_AMO_BLOCK_WORDS - 1U) *
                0x1111111111111111ULL &&
            warmed_marker == marker_seeds[round]
        );

        TEST_SETUP_EXCEPT();
        sd(target_word_vaddr, target_updates[round]);
        sd(neighbor_word_vaddr, neighbor_updates[round]);
        cbo_clean(target_word_vaddr);
        cbo_flush(target_tail_vaddr);
        fence_iorw_iorw();
        sd(marker_vaddr, marker_updates[round]);
        fence_iorw_iorw();
        const uint64_t observed_target = ld(target_word_vaddr);
        const uint64_t observed_neighbor = ld(neighbor_word_vaddr);
        const uint64_t observed_tail = ld(target_tail_vaddr);
        const uint64_t observed_marker = ld(marker_vaddr);
        AI_ASSERT_NO_EXCEPT_AND("CBOM clean/flush immediately followed by fence releases the CMO flush owner only after drained stores are visible and the post-fence marker still progresses",
            observed_target == target_updates[round] &&
            observed_neighbor == neighbor_updates[round] &&
            observed_tail == block_seeds[round] +
                (AI_MEMBLOCK_CMO_AMO_BLOCK_WORDS - 1U) *
                0x1111111111111111ULL &&
            observed_marker == marker_updates[round]
        );

        TEST_SETUP_EXCEPT();
        AI_ASSERT_NO_EXCEPT_AND("the CBOM then fence owner-window does not corrupt target or marker guards",
            ld(before_guard_vaddr) == before_guards[round] &&
            ld(after_guard_vaddr) == after_guards[round] &&
            ld(marker_guard_vaddr) == marker_guards[round]
        );

        goto_priv(PRIV_M);
        TEST_ASSERT("backing memory after the CBOM then fence owner-window contains only the drained stores, marker update, and unchanged guards",
            ai_memblock_cmo_amo_block_matches_two_updates(
                target_base_paddr,
                block_seeds[round],
                0U,
                target_updates[round],
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

bool ai_micro_hs_fence_then_cbom_no_stale_fence_flush_owner_or_cmo_drop_corner()
{
    TEST_START();

    const enum test_page target_page = SWITCH2;
    const enum test_page marker_page = VSRWX_GRWX;
    const uintptr_t target_base_vaddr = hs_page_base(target_page) + 0x440U;
    const uintptr_t target_base_paddr = phys_page_base(target_page) + 0x440U;
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
    const uintptr_t marker_vaddr = hs_page_base(marker_page) + 0x440U;
    const uintptr_t marker_paddr = phys_page_base(marker_page) + 0x440U;
    const uintptr_t marker_guard_vaddr = marker_vaddr + sizeof(uint64_t);
    const uintptr_t marker_guard_paddr = marker_paddr + sizeof(uint64_t);
    const uint64_t block_seeds[] = {
        0x45566778899aabbcULL,
        0x566778899aabbccdULL,
        0x6778899aabbccddeULL,
    };
    const uint64_t drained_updates[] = {
        0x8877665544332211ULL,
        0x99aabbccddeeff00ULL,
        0x0123456789abcdefULL,
    };
    const uint64_t neighbor_updates[] = {
        0x1029384756aabbccULL,
        0x21436587a9cbed0fULL,
        0xfdb97531eca86420ULL,
    };
    const uint64_t before_guards[] = {
        0xabcdabcd12341234ULL,
        0x56785678fedcfedcULL,
        0x0f0e0d0c0b0a0908ULL,
    };
    const uint64_t after_guards[] = {
        0xf0e0d0c0b0a09080ULL,
        0x13579bdf2468ace0ULL,
        0x02468ace13579bdfULL,
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
        0xfeedfacecafebeefULL,
        0xdeadc0def00dbabeULL,
        0xfaceb00c13579bdfULL,
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
        const uint64_t warmed_neighbor = ld(neighbor_word_vaddr);
        const uint64_t warmed_tail = ld(target_tail_vaddr);
        const uint64_t warmed_marker = ld(marker_vaddr);
        AI_ASSERT_NO_EXCEPT_AND("before the fence then CBOM owner-window starts, target block and marker are readable with seeded values",
            warmed_target == block_seeds[round] &&
            warmed_neighbor == block_seeds[round] +
                3U * 0x1111111111111111ULL &&
            warmed_tail == block_seeds[round] +
                (AI_MEMBLOCK_CMO_AMO_BLOCK_WORDS - 1U) *
                0x1111111111111111ULL &&
            warmed_marker == marker_seeds[round]
        );

        TEST_SETUP_EXCEPT();
        sd(target_word_vaddr, drained_updates[round]);
        sd(neighbor_word_vaddr, neighbor_updates[round]);
        fence_iorw_iorw();
        cbo_clean(target_word_vaddr);
        cbo_flush(target_tail_vaddr);
        sd(marker_vaddr, marker_updates[round]);
        fence_iorw_iorw();
        const uint64_t observed_target = ld(target_word_vaddr);
        const uint64_t observed_neighbor = ld(neighbor_word_vaddr);
        const uint64_t observed_tail = ld(target_tail_vaddr);
        const uint64_t observed_marker = ld(marker_vaddr);
        AI_ASSERT_NO_EXCEPT_AND("fence drain immediately followed by CBOM clean/flush releases fence ownership before CMO drain and preserves the post-CBOM marker",
            observed_target == drained_updates[round] &&
            observed_neighbor == neighbor_updates[round] &&
            observed_tail == block_seeds[round] +
                (AI_MEMBLOCK_CMO_AMO_BLOCK_WORDS - 1U) *
                0x1111111111111111ULL &&
            observed_marker == marker_updates[round]
        );

        TEST_SETUP_EXCEPT();
        AI_ASSERT_NO_EXCEPT_AND("the fence then CBOM owner-window does not corrupt target or marker guards",
            ld(before_guard_vaddr) == before_guards[round] &&
            ld(after_guard_vaddr) == after_guards[round] &&
            ld(marker_guard_vaddr) == marker_guards[round]
        );

        goto_priv(PRIV_M);
        TEST_ASSERT("backing memory after the fence then CBOM owner-window contains the drained stores, marker update, and unchanged guards",
            ai_memblock_cmo_amo_block_matches_two_updates(
                target_base_paddr,
                block_seeds[round],
                0U,
                drained_updates[round],
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

bool ai_micro_hs_fence_then_cbozero_no_stale_fence_flush_owner_or_zero_drop_corner()
{
    TEST_START();

    const enum test_page target_page = SWITCH1;
    const enum test_page marker_page = VSRWX_GRWX;
    const uintptr_t target_base_vaddr = hs_page_base(target_page) + 0x640U;
    const uintptr_t target_base_paddr = phys_page_base(target_page) + 0x640U;
    const uintptr_t zero_subblock_vaddr =
        target_base_vaddr + 4U * sizeof(uint64_t);
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
    const uintptr_t marker_vaddr = hs_page_base(marker_page) + 0x640U;
    const uintptr_t marker_paddr = phys_page_base(marker_page) + 0x640U;
    const uintptr_t marker_guard_vaddr = marker_vaddr + sizeof(uint64_t);
    const uintptr_t marker_guard_paddr = marker_paddr + sizeof(uint64_t);
    const uint64_t block_seeds[] = {
        0x798a9bacbdcedff0ULL,
        0x8a9bacbdcedff001ULL,
        0x9bacbdcedff00112ULL,
    };
    const uint64_t drained_updates[] = {
        0x0ddc0ffeec0ffee0ULL,
        0x1ee1deadc0de0001ULL,
        0x2ee2facefeed0002ULL,
    };
    const uint64_t neighbor_updates[] = {
        0x8877665544332211ULL,
        0x7766554433221100ULL,
        0x66554433221100ffULL,
    };
    const uint64_t before_guards[] = {
        0xa1b2c3d4e5f60718ULL,
        0x1029384756a5b4c3ULL,
        0x0f1e2d3c4b5a6978ULL,
    };
    const uint64_t after_guards[] = {
        0x8877aabbccddeeffULL,
        0x77668899aabbccddULL,
        0x6655778899aabbccULL,
    };
    const uint64_t marker_seeds[] = {
        0xf0e1d2c3b4a59687ULL,
        0xe1d2c3b4a5968778ULL,
        0xd2c3b4a596877869ULL,
    };
    const uint64_t marker_updates[] = {
        0x3141592653589793ULL,
        0x2718281828459045ULL,
        0x1123581321345589ULL,
    };
    const uint64_t marker_guards[] = {
        0x55aa33cc7799bbddULL,
        0x1234fedc5678ba90ULL,
        0x0badc0de55aa33ccULL,
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
        const uint64_t warmed_target = ld(target_word_vaddr);
        const uint64_t warmed_neighbor = ld(neighbor_word_vaddr);
        const uint64_t warmed_tail = ld(target_tail_vaddr);
        const uint64_t warmed_marker = ld(marker_vaddr);
        AI_ASSERT_NO_EXCEPT_AND("before the fence then CBO.ZERO owner-window starts, target block and marker are readable with seeded values",
            warmed_target == block_seeds[round] &&
            warmed_neighbor == block_seeds[round] +
                3U * 0x1111111111111111ULL &&
            warmed_tail == block_seeds[round] +
                (AI_MEMBLOCK_CMO_AMO_BLOCK_WORDS - 1U) *
                0x1111111111111111ULL &&
            warmed_marker == marker_seeds[round]
        );

        TEST_SETUP_EXCEPT();
        sd(target_word_vaddr, drained_updates[round]);
        sd(neighbor_word_vaddr, neighbor_updates[round]);
        fence_iorw_iorw();
        cbo_zero(zero_subblock_vaddr);
        sd(marker_vaddr, marker_updates[round]);
        fence_iorw_iorw();
        const uint64_t observed_target = ld(target_word_vaddr);
        const uint64_t observed_neighbor = ld(neighbor_word_vaddr);
        const uint64_t observed_tail = ld(target_tail_vaddr);
        const uint64_t observed_marker = ld(marker_vaddr);
        AI_ASSERT_NO_EXCEPT_AND("fence drain immediately followed by CBO.ZERO releases fence ownership before zero flush and preserves the post-zero marker",
            observed_target == 0ULL &&
            observed_neighbor == 0ULL &&
            observed_tail == 0ULL &&
            observed_marker == marker_updates[round]
        );

        TEST_SETUP_EXCEPT();
        AI_ASSERT_NO_EXCEPT_AND("the fence then CBO.ZERO owner-window does not corrupt target or marker guards",
            ld(before_guard_vaddr) == before_guards[round] &&
            ld(after_guard_vaddr) == after_guards[round] &&
            ld(marker_guard_vaddr) == marker_guards[round]
        );

        goto_priv(PRIV_M);
        TEST_ASSERT("backing memory after the fence then CBO.ZERO owner-window contains a fully zeroed target block, marker update, and unchanged guards",
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

bool ai_micro_hs_cbom_then_cbozero_no_stale_cmo_owner_or_zero_drop_corner()
{
    TEST_START();

    const enum test_page target_page = SWITCH2;
    const enum test_page marker_page = VSRWX_GRWX;
    const uintptr_t target_base_vaddr = hs_page_base(target_page) + 0x640U;
    const uintptr_t target_base_paddr = phys_page_base(target_page) + 0x640U;
    const uintptr_t zero_subblock_vaddr =
        target_base_vaddr + 4U * sizeof(uint64_t);
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
    const uintptr_t marker_vaddr = hs_page_base(marker_page) + 0x740U;
    const uintptr_t marker_paddr = phys_page_base(marker_page) + 0x740U;
    const uintptr_t marker_guard_vaddr = marker_vaddr + sizeof(uint64_t);
    const uintptr_t marker_guard_paddr = marker_paddr + sizeof(uint64_t);
    const uint64_t block_seeds[] = {
        0x2435465768798a9bULL,
        0x35465768798a9bacULL,
        0x465768798a9bacbdULL,
    };
    const uint64_t pre_cbom_updates[] = {
        0xa5a55a5ac3c33c3cULL,
        0xb6b66b6bd4d44d4dULL,
        0xc7c77c7ce5e55e5eULL,
    };
    const uint64_t neighbor_updates[] = {
        0x1021324354657687ULL,
        0x2132435465768798ULL,
        0x32435465768798a9ULL,
    };
    const uint64_t before_guards[] = {
        0x13579bdf2468ace0ULL,
        0x2468ace013579bdfULL,
        0x3579bdf12468ace0ULL,
    };
    const uint64_t after_guards[] = {
        0xfedcba9876543210ULL,
        0xedcba9876543210fULL,
        0xdcba9876543210feULL,
    };
    const uint64_t marker_seeds[] = {
        0x0a1b2c3d4e5f6071ULL,
        0x1b2c3d4e5f607182ULL,
        0x2c3d4e5f60718293ULL,
    };
    const uint64_t marker_updates[] = {
        0x1123581321345589ULL,
        0x3141592653589793ULL,
        0x2718281828459045ULL,
    };
    const uint64_t marker_guards[] = {
        0x8877665544332211ULL,
        0x7766554433221100ULL,
        0x66554433221100ffULL,
    };

    goto_priv(PRIV_M);
    CSRS(CSR_MENVCFG, MENVCFG_CBZFE | MENVCFG_CBZE);
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
        const uint64_t warmed_target = ld(target_word_vaddr);
        const uint64_t warmed_neighbor = ld(neighbor_word_vaddr);
        const uint64_t warmed_tail = ld(target_tail_vaddr);
        const uint64_t warmed_marker = ld(marker_vaddr);
        AI_ASSERT_NO_EXCEPT_AND("before the CBOM then CBO.ZERO owner-window starts, target block and marker are readable with seeded values",
            warmed_target == block_seeds[round] &&
            warmed_neighbor == block_seeds[round] +
                3U * 0x1111111111111111ULL &&
            warmed_tail == block_seeds[round] +
                (AI_MEMBLOCK_CMO_AMO_BLOCK_WORDS - 1U) *
                0x1111111111111111ULL &&
            warmed_marker == marker_seeds[round]
        );

        TEST_SETUP_EXCEPT();
        sd(target_word_vaddr, pre_cbom_updates[round]);
        sd(neighbor_word_vaddr, neighbor_updates[round]);
        cbo_clean(target_word_vaddr);
        cbo_flush(target_tail_vaddr);
        cbo_zero(zero_subblock_vaddr);
        sd(marker_vaddr, marker_updates[round]);
        fence_iorw_iorw();
        const uint64_t observed_target = ld(target_word_vaddr);
        const uint64_t observed_neighbor = ld(neighbor_word_vaddr);
        const uint64_t observed_tail = ld(target_tail_vaddr);
        const uint64_t observed_marker = ld(marker_vaddr);
        AI_ASSERT_NO_EXCEPT_AND("CBOM clean/flush immediately followed by CBO.ZERO releases CMO ownership before the zero flush and preserves marker progress",
            observed_target == 0ULL &&
            observed_neighbor == 0ULL &&
            observed_tail == 0ULL &&
            observed_marker == marker_updates[round]
        );

        TEST_SETUP_EXCEPT();
        AI_ASSERT_NO_EXCEPT_AND("the CBOM then CBO.ZERO owner-window does not corrupt target or marker guards",
            ld(before_guard_vaddr) == before_guards[round] &&
            ld(after_guard_vaddr) == after_guards[round] &&
            ld(marker_guard_vaddr) == marker_guards[round]
        );

        goto_priv(PRIV_M);
        TEST_ASSERT("backing memory after the CBOM then CBO.ZERO owner-window contains a fully zeroed target block, marker update, and unchanged guards",
            ai_memblock_cmo_amo_block_all_zero(target_base_paddr) &&
            read64(before_guard_paddr) == before_guards[round] &&
            read64(after_guard_paddr) == after_guards[round] &&
            read64(marker_paddr) == marker_updates[round] &&
            read64(marker_guard_paddr) == marker_guards[round]
        );
    }

    goto_priv(PRIV_M);
    CSRC(CSR_MENVCFG, MENVCFG_CBZFE | MENVCFG_CBZE);
    hspt_init();
    sfence_vma();

    TEST_END();
}

bool ai_micro_hs_cbozero_then_cbom_no_stale_zero_owner_or_cmo_marker_drop_corner()
{
    TEST_START();

    const enum test_page target_page = SWITCH1;
    const enum test_page marker_page = VSRWX_GRWX;
    const uintptr_t target_base_vaddr = hs_page_base(target_page) + 0x740U;
    const uintptr_t target_base_paddr = phys_page_base(target_page) + 0x740U;
    const uintptr_t zero_subblock_vaddr =
        target_base_vaddr + 4U * sizeof(uint64_t);
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
    const uintptr_t marker_vaddr = hs_page_base(marker_page) + 0x840U;
    const uintptr_t marker_paddr = phys_page_base(marker_page) + 0x840U;
    const uintptr_t marker_guard_vaddr = marker_vaddr + sizeof(uint64_t);
    const uintptr_t marker_guard_paddr = marker_paddr + sizeof(uint64_t);
    const uint64_t block_seeds[] = {
        0x597a9bbcdf002142ULL,
        0x6a8bacceef113253ULL,
        0x7b9cbdde00224364ULL,
    };
    const uint64_t before_guards[] = {
        0x1020304050607080ULL,
        0x2030405060708090ULL,
        0x30405060708090a0ULL,
    };
    const uint64_t after_guards[] = {
        0xa090807060504030ULL,
        0xb0a0908070605040ULL,
        0xc0b0a09080706050ULL,
    };
    const uint64_t marker_seeds[] = {
        0x55aa55aa33cc33ccULL,
        0x66bb66bb44dd44ddULL,
        0x77cc77cc55ee55eeULL,
    };
    const uint64_t marker_updates[] = {
        0xfdb97531eca86420ULL,
        0x02468ace13579bdfULL,
        0x13579bdf2468ace0ULL,
    };
    const uint64_t marker_guards[] = {
        0x1122334455667788ULL,
        0x2233445566778899ULL,
        0x33445566778899aaULL,
    };

    goto_priv(PRIV_M);
    CSRS(CSR_MENVCFG, MENVCFG_CBZFE | MENVCFG_CBZE);
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
        const uint64_t warmed_target = ld(target_word_vaddr);
        const uint64_t warmed_neighbor = ld(neighbor_word_vaddr);
        const uint64_t warmed_tail = ld(target_tail_vaddr);
        const uint64_t warmed_marker = ld(marker_vaddr);
        AI_ASSERT_NO_EXCEPT_AND("before the CBO.ZERO then CBOM owner-window starts, target block and marker are readable with seeded values",
            warmed_target == block_seeds[round] &&
            warmed_neighbor == block_seeds[round] +
                3U * 0x1111111111111111ULL &&
            warmed_tail == block_seeds[round] +
                (AI_MEMBLOCK_CMO_AMO_BLOCK_WORDS - 1U) *
                0x1111111111111111ULL &&
            warmed_marker == marker_seeds[round]
        );

        TEST_SETUP_EXCEPT();
        cbo_zero(zero_subblock_vaddr);
        cbo_clean(target_word_vaddr);
        cbo_flush(target_tail_vaddr);
        sd(marker_vaddr, marker_updates[round]);
        fence_iorw_iorw();
        const uint64_t observed_target = ld(target_word_vaddr);
        const uint64_t observed_neighbor = ld(neighbor_word_vaddr);
        const uint64_t observed_tail = ld(target_tail_vaddr);
        const uint64_t observed_marker = ld(marker_vaddr);
        AI_ASSERT_NO_EXCEPT_AND("CBO.ZERO immediately followed by CBOM clean/flush releases zero ownership before CMO finish and preserves marker progress",
            observed_target == 0ULL &&
            observed_neighbor == 0ULL &&
            observed_tail == 0ULL &&
            observed_marker == marker_updates[round]
        );

        TEST_SETUP_EXCEPT();
        AI_ASSERT_NO_EXCEPT_AND("the CBO.ZERO then CBOM owner-window does not corrupt target or marker guards",
            ld(before_guard_vaddr) == before_guards[round] &&
            ld(after_guard_vaddr) == after_guards[round] &&
            ld(marker_guard_vaddr) == marker_guards[round]
        );

        goto_priv(PRIV_M);
        TEST_ASSERT("backing memory after the CBO.ZERO then CBOM owner-window contains a fully zeroed target block, marker update, and unchanged guards",
            ai_memblock_cmo_amo_block_all_zero(target_base_paddr) &&
            read64(before_guard_paddr) == before_guards[round] &&
            read64(after_guard_paddr) == after_guards[round] &&
            read64(marker_paddr) == marker_updates[round] &&
            read64(marker_guard_paddr) == marker_guards[round]
        );
    }

    goto_priv(PRIV_M);
    CSRC(CSR_MENVCFG, MENVCFG_CBZFE | MENVCFG_CBZE);
    hspt_init();
    sfence_vma();

    TEST_END();
}

bool ai_micro_hs_vsegment_store_then_cbozero_no_stale_segment_flush_owner_or_zero_drop_corner()
{
    TEST_START();

    const enum test_page src_page = SWITCH1;
    const enum test_page seg_dst_page = SWITCH2;
    const enum test_page zero_page = VSRWX_GRWX;
    const uintptr_t src_vaddr = hs_page_base(src_page) + 0xe00U;
    const uintptr_t src_paddr = phys_page_base(src_page) + 0xe00U;
    const uintptr_t src_guard_vaddr =
        src_vaddr + AI_MEMBLOCK_CMO_VECTOR_BYTES;
    const uintptr_t src_guard_paddr =
        src_paddr + AI_MEMBLOCK_CMO_VECTOR_BYTES;
    const uintptr_t seg_dst_vaddr = hs_page_base(seg_dst_page) + 0xe00U;
    const uintptr_t seg_dst_paddr = phys_page_base(seg_dst_page) + 0xe00U;
    const uintptr_t seg_dst_before_vaddr =
        seg_dst_vaddr - sizeof(uint64_t);
    const uintptr_t seg_dst_after_vaddr =
        seg_dst_vaddr + AI_MEMBLOCK_CMO_VECTOR_BYTES;
    const uintptr_t seg_dst_before_paddr =
        seg_dst_paddr - sizeof(uint64_t);
    const uintptr_t seg_dst_after_paddr =
        seg_dst_paddr + AI_MEMBLOCK_CMO_VECTOR_BYTES;
    const uintptr_t zero_base_vaddr = hs_page_base(zero_page) + 0xe00U;
    const uintptr_t zero_base_paddr = phys_page_base(zero_page) + 0xe00U;
    const uintptr_t zero_subblock_vaddr =
        zero_base_vaddr + 4U * sizeof(uint64_t);
    const uintptr_t zero_tail_vaddr =
        zero_base_vaddr +
        (AI_MEMBLOCK_CMO_AMO_BLOCK_WORDS - 1U) * sizeof(uint64_t);
    const uintptr_t zero_before_vaddr =
        zero_base_vaddr - sizeof(uint64_t);
    const uintptr_t zero_after_vaddr =
        zero_base_vaddr + AI_MEMBLOCK_CMO_AMO_BLOCK_WORDS *
        sizeof(uint64_t);
    const uintptr_t zero_before_paddr =
        zero_base_paddr - sizeof(uint64_t);
    const uintptr_t zero_after_paddr =
        zero_base_paddr + AI_MEMBLOCK_CMO_AMO_BLOCK_WORDS *
        sizeof(uint64_t);
    const uintptr_t marker_vaddr = hs_page_base(zero_page) + 0xf00U;
    const uintptr_t marker_paddr = phys_page_base(zero_page) + 0xf00U;
    const uintptr_t marker_guard_vaddr = marker_vaddr + sizeof(uint64_t);
    const uintptr_t marker_guard_paddr = marker_paddr + sizeof(uint64_t);
    const uint64_t src_word0_values[] = {
        0x1021324354657687ULL,
        0x89abcdef01234567ULL,
        0x76543210fedcba98ULL,
    };
    const uint64_t src_word1_values[] = {
        0xfedcba9876543210ULL,
        0x13579bdf2468ace0ULL,
        0x02468ace13579bdfULL,
    };
    const uint64_t zero_seeds[] = {
        0x3141592653589793ULL,
        0x2718281828459045ULL,
        0x1123581321345589ULL,
    };
    const uint64_t src_guards[] = {
        0xa1b2c3d4e5f60718ULL,
        0x1029384756a5b4c3ULL,
        0x0f1e2d3c4b5a6978ULL,
    };
    const uint64_t seg_dst_fills[] = {
        0xa5a55a5ac3c33c3cULL,
        0x5a5aa5a53c3cc3c3ULL,
        0xdeadbeefcafebabeULL,
    };
    const uint64_t seg_dst_before_guards[] = {
        0x55aa33cc7799bbddULL,
        0x1234fedc5678ba90ULL,
        0x0badc0de55aa33ccULL,
    };
    const uint64_t seg_dst_after_guards[] = {
        0x8877aabbccddeeffULL,
        0x77668899aabbccddULL,
        0x6655778899aabbccULL,
    };
    const uint64_t zero_before_guards[] = {
        0xfdb97531eca86420ULL,
        0x13579bdf2468ace0ULL,
        0x02468ace13579bdfULL,
    };
    const uint64_t zero_after_guards[] = {
        0x0a1b2c3d4e5f6071ULL,
        0x1b2c3d4e5f607182ULL,
        0x2c3d4e5f60718293ULL,
    };
    const uint64_t marker_seeds[] = {
        0xc001d00dcafef00dULL,
        0xd15ea5edfeedfaceULL,
        0xabcdef0123456789ULL,
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

    for (size_t round = 0; round < AI_MEMBLOCK_FLUSH_ARRAY_SIZE(zero_seeds);
         ++round) {
        goto_priv(PRIV_M);
        write64(src_paddr, src_word0_values[round]);
        write64(src_paddr + sizeof(uint64_t), src_word1_values[round]);
        write64(src_guard_paddr, src_guards[round]);
        write64(seg_dst_paddr, seg_dst_fills[round]);
        write64(seg_dst_paddr + sizeof(uint64_t),
            ~seg_dst_fills[round]);
        write64(seg_dst_before_paddr, seg_dst_before_guards[round]);
        write64(seg_dst_after_paddr, seg_dst_after_guards[round]);
        ai_memblock_cmo_amo_seed_block(zero_base_paddr, zero_seeds[round]);
        write64(zero_before_paddr, zero_before_guards[round]);
        write64(zero_after_paddr, zero_after_guards[round]);
        write64(marker_paddr, marker_seeds[round]);
        write64(marker_guard_paddr, marker_guards[round]);

        goto_priv(PRIV_HS);
        TEST_SETUP_EXCEPT();
        const uint64_t warmed_src0 = ld(src_vaddr);
        const uint64_t warmed_src1 = ld(src_vaddr + sizeof(uint64_t));
        const uint64_t warmed_zero_tail = ld(zero_tail_vaddr);
        const uint64_t warmed_marker = ld(marker_vaddr);
        AI_ASSERT_NO_EXCEPT_AND("before the VSegment then CBO.ZERO owner-window starts, source, zero target tail, and marker are readable with seeded values",
            warmed_src0 == src_word0_values[round] &&
            warmed_src1 == src_word1_values[round] &&
            warmed_zero_tail == zero_seeds[round] +
                (AI_MEMBLOCK_CMO_AMO_BLOCK_WORDS - 1U) *
                0x1111111111111111ULL &&
            warmed_marker == marker_seeds[round]
        );

        TEST_SETUP_EXCEPT();
        ai_memblock_cmo_vsegment_copy_only(src_vaddr, seg_dst_vaddr);
        cbo_zero(zero_subblock_vaddr);
        sd(marker_vaddr, marker_updates[round]);
        fence_iorw_iorw();
        const uint64_t observed_seg_dst0 = ld(seg_dst_vaddr);
        const uint64_t observed_seg_dst1 =
            ld(seg_dst_vaddr + sizeof(uint64_t));
        const uint64_t observed_zero0 = ld(zero_base_vaddr);
        const uint64_t observed_zero_tail = ld(zero_tail_vaddr);
        const uint64_t observed_marker = ld(marker_vaddr);
        AI_ASSERT_NO_EXCEPT_AND("VSegment store immediately followed by CBO.ZERO releases segment flush ownership before zero flush and preserves marker progress",
            observed_seg_dst0 == src_word0_values[round] &&
            observed_seg_dst1 == src_word1_values[round] &&
            observed_zero0 == 0ULL &&
            observed_zero_tail == 0ULL &&
            observed_marker == marker_updates[round]
        );

        TEST_SETUP_EXCEPT();
        AI_ASSERT_NO_EXCEPT_AND("the VSegment then CBO.ZERO owner-window does not corrupt source, segment destination, zero-block, or marker guards",
            ld(src_guard_vaddr) == src_guards[round] &&
            ld(seg_dst_before_vaddr) == seg_dst_before_guards[round] &&
            ld(seg_dst_after_vaddr) == seg_dst_after_guards[round] &&
            ld(zero_before_vaddr) == zero_before_guards[round] &&
            ld(zero_after_vaddr) == zero_after_guards[round] &&
            ld(marker_guard_vaddr) == marker_guards[round]
        );

        goto_priv(PRIV_M);
        TEST_ASSERT("backing memory after the VSegment then CBO.ZERO owner-window contains the copied segment payload, fully zeroed target block, marker update, and unchanged guards",
            read64(src_paddr) == src_word0_values[round] &&
            read64(src_paddr + sizeof(uint64_t)) ==
                src_word1_values[round] &&
            read64(src_guard_paddr) == src_guards[round] &&
            read64(seg_dst_paddr) == src_word0_values[round] &&
            read64(seg_dst_paddr + sizeof(uint64_t)) ==
                src_word1_values[round] &&
            read64(seg_dst_before_paddr) ==
                seg_dst_before_guards[round] &&
            read64(seg_dst_after_paddr) == seg_dst_after_guards[round] &&
            ai_memblock_cmo_amo_block_all_zero(zero_base_paddr) &&
            read64(zero_before_paddr) == zero_before_guards[round] &&
            read64(zero_after_paddr) == zero_after_guards[round] &&
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

bool ai_micro_hs_normal_vector_store_then_cbozero_no_stale_vector_writeback_or_zero_drop_corner()
{
    TEST_START();

    const enum test_page src_page = SWITCH1;
    const enum test_page vec_dst_page = SWITCH2;
    const enum test_page zero_page = VSRWX_GRWX;
    const uintptr_t src_vaddr = hs_page_base(src_page) + 0xf00U;
    const uintptr_t src_paddr = phys_page_base(src_page) + 0xf00U;
    const uintptr_t src_guard_vaddr =
        src_vaddr + AI_MEMBLOCK_CMO_VECTOR_BYTES;
    const uintptr_t src_guard_paddr =
        src_paddr + AI_MEMBLOCK_CMO_VECTOR_BYTES;
    const uintptr_t vec_dst_vaddr = hs_page_base(vec_dst_page) + 0xf00U;
    const uintptr_t vec_dst_paddr = phys_page_base(vec_dst_page) + 0xf00U;
    const uintptr_t vec_dst_before_vaddr =
        vec_dst_vaddr - sizeof(uint64_t);
    const uintptr_t vec_dst_after_vaddr =
        vec_dst_vaddr + AI_MEMBLOCK_CMO_VECTOR_BYTES;
    const uintptr_t vec_dst_before_paddr =
        vec_dst_paddr - sizeof(uint64_t);
    const uintptr_t vec_dst_after_paddr =
        vec_dst_paddr + AI_MEMBLOCK_CMO_VECTOR_BYTES;
    const uintptr_t zero_base_vaddr = hs_page_base(zero_page) + 0x200U;
    const uintptr_t zero_base_paddr = phys_page_base(zero_page) + 0x200U;
    const uintptr_t zero_subblock_vaddr =
        zero_base_vaddr + 3U * sizeof(uint64_t);
    const uintptr_t zero_tail_vaddr =
        zero_base_vaddr +
        (AI_MEMBLOCK_CMO_AMO_BLOCK_WORDS - 1U) * sizeof(uint64_t);
    const uintptr_t zero_before_vaddr =
        zero_base_vaddr - sizeof(uint64_t);
    const uintptr_t zero_after_vaddr =
        zero_base_vaddr + AI_MEMBLOCK_CMO_AMO_BLOCK_WORDS *
        sizeof(uint64_t);
    const uintptr_t zero_before_paddr =
        zero_base_paddr - sizeof(uint64_t);
    const uintptr_t zero_after_paddr =
        zero_base_paddr + AI_MEMBLOCK_CMO_AMO_BLOCK_WORDS *
        sizeof(uint64_t);
    const uintptr_t marker_vaddr = hs_page_base(zero_page) + 0x2c0U;
    const uintptr_t marker_paddr = phys_page_base(zero_page) + 0x2c0U;
    const uintptr_t marker_guard_vaddr = marker_vaddr + sizeof(uint64_t);
    const uintptr_t marker_guard_paddr = marker_paddr + sizeof(uint64_t);
    const uint64_t src_word0_values[] = {
        0x3141592653589793ULL,
        0x2718281828459045ULL,
        0x1123581321345589ULL,
    };
    const uint64_t src_word1_values[] = {
        0x89abcdef01234567ULL,
        0x76543210fedcba98ULL,
        0x55aa55aa33cc33ccULL,
    };
    const uint64_t zero_seeds[] = {
        0x2435465768798a9bULL,
        0x35465768798a9bacULL,
        0x465768798a9bacbdULL,
    };
    const uint64_t src_guards[] = {
        0xfeedfacecafebeefULL,
        0xdeadc0def00dbabeULL,
        0xfaceb00c13579bdfULL,
    };
    const uint64_t vec_dst_fills[] = {
        0x0f1e2d3c4b5a6978ULL,
        0x8877665544332211ULL,
        0x13579bdf2468ace0ULL,
    };
    const uint64_t vec_dst_before_guards[] = {
        0x0badf00d5a5aa5a5ULL,
        0x1020304050607080ULL,
        0x1122334455667788ULL,
    };
    const uint64_t vec_dst_after_guards[] = {
        0xfdb97531eca86420ULL,
        0x13579bdf2468ace0ULL,
        0x02468ace13579bdfULL,
    };
    const uint64_t zero_before_guards[] = {
        0x55aa33cc7799bbddULL,
        0x1234fedc5678ba90ULL,
        0x0badc0de55aa33ccULL,
    };
    const uint64_t zero_after_guards[] = {
        0xa1b2c3d4e5f60718ULL,
        0x1029384756a5b4c3ULL,
        0x0f1e2d3c4b5a6978ULL,
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
        0x1123581321345589ULL,
        0x3141592653589793ULL,
        0x2718281828459045ULL,
    };

    goto_priv(PRIV_M);
    CSRS(CSR_MENVCFG, MENVCFG_CBZE);
    hspt_init();
    sfence_vma();

    for (size_t round = 0; round < AI_MEMBLOCK_FLUSH_ARRAY_SIZE(zero_seeds);
         ++round) {
        goto_priv(PRIV_M);
        write64(src_paddr, src_word0_values[round]);
        write64(src_paddr + sizeof(uint64_t), src_word1_values[round]);
        write64(src_guard_paddr, src_guards[round]);
        write64(vec_dst_paddr, vec_dst_fills[round]);
        write64(vec_dst_paddr + sizeof(uint64_t),
            ~vec_dst_fills[round]);
        write64(vec_dst_before_paddr, vec_dst_before_guards[round]);
        write64(vec_dst_after_paddr, vec_dst_after_guards[round]);
        ai_memblock_cmo_amo_seed_block(zero_base_paddr, zero_seeds[round]);
        write64(zero_before_paddr, zero_before_guards[round]);
        write64(zero_after_paddr, zero_after_guards[round]);
        write64(marker_paddr, marker_seeds[round]);
        write64(marker_guard_paddr, marker_guards[round]);

        goto_priv(PRIV_HS);
        TEST_SETUP_EXCEPT();
        const uint64_t warmed_src0 = ld(src_vaddr);
        const uint64_t warmed_src1 = ld(src_vaddr + sizeof(uint64_t));
        const uint64_t warmed_zero_tail = ld(zero_tail_vaddr);
        const uint64_t warmed_marker = ld(marker_vaddr);
        AI_ASSERT_NO_EXCEPT_AND("before the normal-vector store then CBO.ZERO owner-window starts, source, zero target tail, and marker are readable with seeded values",
            warmed_src0 == src_word0_values[round] &&
            warmed_src1 == src_word1_values[round] &&
            warmed_zero_tail == zero_seeds[round] +
                (AI_MEMBLOCK_CMO_AMO_BLOCK_WORDS - 1U) *
                0x1111111111111111ULL &&
            warmed_marker == marker_seeds[round]
        );

        TEST_SETUP_EXCEPT();
        ai_memblock_cmo_normal_vector_copy_only(src_vaddr, vec_dst_vaddr);
        cbo_zero(zero_subblock_vaddr);
        sd(marker_vaddr, marker_updates[round]);
        fence_iorw_iorw();
        const uint64_t observed_vec_dst0 = ld(vec_dst_vaddr);
        const uint64_t observed_vec_dst1 =
            ld(vec_dst_vaddr + sizeof(uint64_t));
        const uint64_t observed_zero0 = ld(zero_base_vaddr);
        const uint64_t observed_zero_tail = ld(zero_tail_vaddr);
        const uint64_t observed_marker = ld(marker_vaddr);
        AI_ASSERT_NO_EXCEPT_AND("normal vector store immediately followed by CBO.ZERO releases vector writeback before zero flush and preserves marker progress",
            observed_vec_dst0 == src_word0_values[round] &&
            observed_vec_dst1 == src_word1_values[round] &&
            observed_zero0 == 0ULL &&
            observed_zero_tail == 0ULL &&
            observed_marker == marker_updates[round]
        );

        TEST_SETUP_EXCEPT();
        AI_ASSERT_NO_EXCEPT_AND("the normal-vector store then CBO.ZERO owner-window does not corrupt source, vector destination, zero-block, or marker guards",
            ld(src_guard_vaddr) == src_guards[round] &&
            ld(vec_dst_before_vaddr) == vec_dst_before_guards[round] &&
            ld(vec_dst_after_vaddr) == vec_dst_after_guards[round] &&
            ld(zero_before_vaddr) == zero_before_guards[round] &&
            ld(zero_after_vaddr) == zero_after_guards[round] &&
            ld(marker_guard_vaddr) == marker_guards[round]
        );

        goto_priv(PRIV_M);
        TEST_ASSERT("backing memory after the normal-vector store then CBO.ZERO owner-window contains the copied vector payload, fully zeroed target block, marker update, and unchanged guards",
            read64(src_paddr) == src_word0_values[round] &&
            read64(src_paddr + sizeof(uint64_t)) ==
                src_word1_values[round] &&
            read64(src_guard_paddr) == src_guards[round] &&
            read64(vec_dst_paddr) == src_word0_values[round] &&
            read64(vec_dst_paddr + sizeof(uint64_t)) ==
                src_word1_values[round] &&
            read64(vec_dst_before_paddr) ==
                vec_dst_before_guards[round] &&
            read64(vec_dst_after_paddr) == vec_dst_after_guards[round] &&
            ai_memblock_cmo_amo_block_all_zero(zero_base_paddr) &&
            read64(zero_before_paddr) == zero_before_guards[round] &&
            read64(zero_after_paddr) == zero_after_guards[round] &&
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

bool ai_micro_hs_cbozero_then_fence_no_stale_zero_flush_owner_or_marker_drop_corner()
{
    TEST_START();

    const enum test_page target_page = SWITCH2;
    const enum test_page marker_page = VSRWX_GRWX;
    const uintptr_t target_base_vaddr = hs_page_base(target_page) + 0x240U;
    const uintptr_t target_base_paddr = phys_page_base(target_page) + 0x240U;
    const uintptr_t zero_subblock_vaddr =
        target_base_vaddr + 5U * sizeof(uint64_t);
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
    const uintptr_t marker_vaddr = hs_page_base(marker_page) + 0x240U;
    const uintptr_t marker_paddr = phys_page_base(marker_page) + 0x240U;
    const uintptr_t marker_guard_vaddr = marker_vaddr + sizeof(uint64_t);
    const uintptr_t marker_guard_paddr = marker_paddr + sizeof(uint64_t);
    const uint64_t block_seeds[] = {
        0x1029384756abcedfULL,
        0x213a4b5c6d7e8f90ULL,
        0x324b5c6d7e8f9011ULL,
    };
    const uint64_t before_guards[] = {
        0x8877aabbccddeeffULL,
        0x77668899aabbccddULL,
        0x6655778899aabbccULL,
    };
    const uint64_t after_guards[] = {
        0x55aa33cc7799bbddULL,
        0x1234fedc5678ba90ULL,
        0x0badc0de55aa33ccULL,
    };
    const uint64_t marker_seeds[] = {
        0xc001d00dcafef00dULL,
        0xd15ea5edfeedfaceULL,
        0xabcdef0123456789ULL,
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
        const uint64_t warmed_target = ld(target_word_vaddr);
        const uint64_t warmed_tail = ld(target_tail_vaddr);
        const uint64_t warmed_marker = ld(marker_vaddr);
        AI_ASSERT_NO_EXCEPT_AND("before the CBO.ZERO then fence owner-window starts, target block and marker are readable with seeded values",
            warmed_target == block_seeds[round] &&
            warmed_tail == block_seeds[round] +
                (AI_MEMBLOCK_CMO_AMO_BLOCK_WORDS - 1U) *
                0x1111111111111111ULL &&
            warmed_marker == marker_seeds[round]
        );

        TEST_SETUP_EXCEPT();
        cbo_zero(zero_subblock_vaddr);
        fence_iorw_iorw();
        sd(marker_vaddr, marker_updates[round]);
        fence_iorw_iorw();
        const uint64_t observed_target = ld(target_word_vaddr);
        const uint64_t observed_tail = ld(target_tail_vaddr);
        const uint64_t observed_marker = ld(marker_vaddr);
        AI_ASSERT_NO_EXCEPT_AND("CBO.ZERO immediately followed by fence releases zero ownership before fence completion and preserves the post-fence marker",
            observed_target == 0ULL &&
            observed_tail == 0ULL &&
            observed_marker == marker_updates[round]
        );

        TEST_SETUP_EXCEPT();
        AI_ASSERT_NO_EXCEPT_AND("the CBO.ZERO then fence owner-window does not corrupt target or marker guards",
            ld(before_guard_vaddr) == before_guards[round] &&
            ld(after_guard_vaddr) == after_guards[round] &&
            ld(marker_guard_vaddr) == marker_guards[round]
        );

        goto_priv(PRIV_M);
        TEST_ASSERT("backing memory after the CBO.ZERO then fence owner-window contains a fully zeroed target block, marker update, and unchanged guards",
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

bool ai_micro_hs_wrong_path_amo_redirect_no_stale_atomic_state_or_spurious_fault_corner()
{
    TEST_START();

    const enum test_page target_page = SWITCH1;
    const enum test_page control_page = SWITCH2;
    const enum test_page marker_page = VSRW_GRW;
    const uintptr_t target_vaddr = hs_page_base(target_page) + 0x280U;
    const uintptr_t target_paddr = phys_page_base(target_page) + 0x280U;
    const uintptr_t target_guard_vaddr = target_vaddr + sizeof(uint64_t);
    const uintptr_t target_guard_paddr = target_paddr + sizeof(uint64_t);
    const uintptr_t control_vaddr = hs_page_base(control_page) + 0x300U;
    const uintptr_t control_paddr = phys_page_base(control_page) + 0x300U;
    const uintptr_t marker_vaddr = hs_page_base(marker_page) + 0x380U;
    const uintptr_t marker_paddr = phys_page_base(marker_page) + 0x380U;
    const uintptr_t marker_guard_vaddr = marker_vaddr + sizeof(uint64_t);
    const uintptr_t marker_guard_paddr = marker_paddr + sizeof(uint64_t);
    const uint64_t target_seeds[] = {
        0x1111222233334444ULL,
        0x2222333344445555ULL,
    };
    const uint64_t train_addends[] = {
        0x0000000000000101ULL,
        0x0000000000000202ULL,
    };
    const uint64_t wrong_path_addends[] = {
        0x0000000000001001ULL,
        0x0000000000002002ULL,
    };
    const uint64_t recovery_addends[] = {
        0x0000000000000011ULL,
        0x0000000000000022ULL,
    };
    const uint64_t marker_seeds[] = {
        0x0102030405060708ULL,
        0x1020304050607080ULL,
    };
    const uint64_t marker_training[] = {
        0xa0a1a2a3a4a5a6a7ULL,
        0xb0b1b2b3b4b5b6b7ULL,
    };
    const uint64_t marker_taken[] = {
        0x8877665544332211ULL,
        0x7766554433221100ULL,
    };
    const uint64_t marker_recovery[] = {
        0x13579bdf2468ace0ULL,
        0x02468ace13579bdfULL,
    };
    const uint64_t target_guards[] = {
        0xfeedfacecafebeefULL,
        0xdeadc0def00dbabeULL,
    };
    const uint64_t marker_guards[] = {
        0x3141592653589793ULL,
        0x2718281828459045ULL,
    };

    for (size_t round = 0;
         round < AI_MEMBLOCK_FLUSH_ARRAY_SIZE(target_seeds);
         ++round) {
        const uint64_t trained_target =
            target_seeds[round] +
            AI_MEMBLOCK_ATOMIC_REDIRECT_TRAIN_ROUNDS *
            train_addends[round];
        const uint64_t recovered_target =
            target_seeds[round] + recovery_addends[round];
        const uint64_t probe_target_flags = (round == 0U) ?
            (PTE_V | PTE_AD | PTE_RWX) : (PTE_V | PTE_AD | PTE_R);

        goto_priv(PRIV_M);
        write64(target_paddr, target_seeds[round]);
        write64(target_guard_paddr, target_guards[round]);
        write64(control_paddr, 1ULL);
        write64(marker_paddr, marker_seeds[round]);
        write64(marker_guard_paddr, marker_guards[round]);

        goto_priv(PRIV_HS);
        hspt_init();
        ai_memblock_atomic_tlb_set_hs_leaf(target_page,
            PTE_V | PTE_AD | PTE_RWX);
        ai_memblock_atomic_tlb_set_hs_leaf(control_page,
            PTE_V | PTE_AD | PTE_RWX);
        ai_memblock_atomic_tlb_set_hs_leaf(marker_page,
            PTE_V | PTE_AD | PTE_RWX);
        sfence_vma();

        TEST_SETUP_EXCEPT();
        AI_ASSERT_NO_EXCEPT_AND("before wrong-path AMO redirect training starts, target/control/marker are readable with seeded values",
            ld(target_vaddr) == target_seeds[round] &&
            ld(control_vaddr) == 1ULL &&
            ld(marker_vaddr) == marker_seeds[round]
        );

        TEST_SETUP_EXCEPT();
        ai_memblock_atomic_wrong_path_branch_window(control_vaddr,
            target_vaddr, marker_vaddr, train_addends[round],
            marker_training[round],
            AI_MEMBLOCK_ATOMIC_REDIRECT_TRAIN_ROUNDS);
        AI_ASSERT_NO_EXCEPT_AND("fall-through AMO training retires cleanly and accumulates only the training addend",
            ld(target_vaddr) == trained_target &&
            ld(marker_vaddr) == marker_training[round]
        );

        goto_priv(PRIV_M);
        write64(target_paddr, target_seeds[round]);
        write64(control_paddr, 0ULL);
        write64(marker_paddr, marker_seeds[round]);

        goto_priv(PRIV_HS);
        ai_memblock_atomic_tlb_set_hs_leaf(target_page, probe_target_flags);
        sfence_vma();

        TEST_SETUP_EXCEPT();
        ai_memblock_atomic_wrong_path_branch_window(control_vaddr,
            target_vaddr, marker_vaddr, wrong_path_addends[round],
            marker_taken[round],
            AI_MEMBLOCK_ATOMIC_REDIRECT_PROBE_ROUNDS);
        AI_ASSERT_NO_EXCEPT_AND("taken branch must kill the wrong-path AMO before it updates the target or raises a spurious store page fault",
            ld(target_vaddr) == target_seeds[round] &&
            ld(marker_vaddr) == marker_taken[round]
        );

        ai_memblock_atomic_tlb_set_hs_leaf(target_page,
            PTE_V | PTE_AD | PTE_RWX);
        sfence_vma();

        TEST_SETUP_EXCEPT();
        const uint64_t recovery_old =
            amoadd_d(target_vaddr, recovery_addends[round]);
        sd(marker_vaddr, marker_recovery[round]);
        fence_iorw_iorw();
        AI_ASSERT_NO_EXCEPT_AND("after the skipped wrong-path AMO, the next valid AMO uses fresh atomic state and returns the original target value",
            recovery_old == target_seeds[round] &&
            ld(target_vaddr) == recovered_target &&
            ld(marker_vaddr) == marker_recovery[round] &&
            ld(target_guard_vaddr) == target_guards[round] &&
            ld(marker_guard_vaddr) == marker_guards[round]
        );

        goto_priv(PRIV_M);
        TEST_ASSERT("backing memory after the wrong-path AMO redirect probe contains only the recovery AMO, marker update, and unchanged guards",
            read64(target_paddr) == recovered_target &&
            read64(target_guard_paddr) == target_guards[round] &&
            read64(marker_paddr) == marker_recovery[round] &&
            read64(marker_guard_paddr) == marker_guards[round]
        );
    }

    goto_priv(PRIV_HS);
    hspt_init();
    sfence_vma();

    TEST_END();
}
