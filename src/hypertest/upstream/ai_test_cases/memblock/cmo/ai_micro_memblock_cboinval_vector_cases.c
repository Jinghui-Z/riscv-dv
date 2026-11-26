#include <rvh_test.h>
#include <page_tables.h>
#include <instructions.h>
#include <stdint.h>

#define AI_ASSERT_NO_EXCEPT_AND(message, cond) \
    TEST_ASSERT(message, excpt.triggered == false && (cond))
#define AI_MEMBLOCK_CBOINVAL_VECTOR_ARRAY_SIZE(array) \
    (sizeof(array) / sizeof((array)[0]))
#define AI_MEMBLOCK_CBOINVAL_VECTOR_BLOCK_WORDS 8U
#define AI_MEMBLOCK_CBOINVAL_VECTOR_BYTES 16U

static inline void ai_memblock_cboinval_vector_prepare_hs_env(void)
{
    goto_priv(PRIV_M);
    CSRS(CSR_MENVCFG, MENVCFG_CBIE01);
    hspt_init();
    sfence_vma();
}

static inline void ai_memblock_cboinval_vector_reset_hs_env(void)
{
    goto_priv(PRIV_M);
    CSRC(CSR_MENVCFG, MENVCFG_CBIE01);
    hspt_init();
    sfence_vma();
}

static inline void ai_memblock_cboinval_vector_seed_block(uintptr_t paddr,
    uint64_t seed)
{
    for (size_t i = 0; i < AI_MEMBLOCK_CBOINVAL_VECTOR_BLOCK_WORDS; ++i) {
        write64(paddr + i * sizeof(uint64_t),
            seed + i * 0x1111111111111111ULL);
    }
}

static inline bool ai_memblock_cboinval_vector_block_matches_two_updates(
    uintptr_t paddr,
    uint64_t seed,
    size_t first_word,
    uint64_t first_value,
    size_t second_word,
    uint64_t second_value)
{
    for (size_t i = 0; i < AI_MEMBLOCK_CBOINVAL_VECTOR_BLOCK_WORDS; ++i) {
        uint64_t expected = seed + i * 0x1111111111111111ULL;

        if (i == first_word) {
            expected = first_value;
        } else if (i == second_word) {
            expected = second_value;
        }

        if (read64(paddr + i * sizeof(uint64_t)) != expected) {
            return false;
        }
    }

    return true;
}

static inline void ai_memblock_cboinval_then_normal_vector_copy_marker(
    uintptr_t inval_vaddr,
    uintptr_t src_vaddr,
    uintptr_t dst_vaddr,
    uintptr_t marker_vaddr,
    uint64_t marker_update)
{
    const int vtype = VTYPE(VSEW_FROM_BITS(8), 0);

    asm volatile(
        ".option push\n\t"
        ".option norvc\n\t"
        ".insn i 0x0f, 0x2, x0, %[inval_addr], 0\n\t"
        "vsetvl t0, %[vl], %[vtype]\n\t"
        "vle8.v v8, (%[src_addr])\n\t"
        "vse8.v v8, (%[dst_addr])\n\t"
        "sd %[marker_update], 0(%[marker_addr])\n\t"
        "fence rw, rw\n\t"
        ".option pop\n\t"
        :
        : [inval_addr] "r"(inval_vaddr),
          [src_addr] "r"(src_vaddr),
          [dst_addr] "r"(dst_vaddr),
          [marker_addr] "r"(marker_vaddr),
          [marker_update] "r"(marker_update),
          [vl] "r"((int)AI_MEMBLOCK_CBOINVAL_VECTOR_BYTES),
          [vtype] "r"(vtype)
        : "t0", "memory");
}

static inline void ai_memblock_normal_vector_copy_then_cboinval_marker(
    uintptr_t src_vaddr,
    uintptr_t dst_vaddr,
    uintptr_t inval_vaddr,
    uintptr_t marker_vaddr,
    uint64_t marker_update)
{
    const int vtype = VTYPE(VSEW_FROM_BITS(8), 0);

    asm volatile(
        ".option push\n\t"
        ".option norvc\n\t"
        "vsetvl t0, %[vl], %[vtype]\n\t"
        "vle8.v v8, (%[src_addr])\n\t"
        "vse8.v v8, (%[dst_addr])\n\t"
        ".insn i 0x0f, 0x2, x0, %[inval_addr], 0\n\t"
        "sd %[marker_update], 0(%[marker_addr])\n\t"
        "fence rw, rw\n\t"
        ".option pop\n\t"
        :
        : [src_addr] "r"(src_vaddr),
          [dst_addr] "r"(dst_vaddr),
          [inval_addr] "r"(inval_vaddr),
          [marker_addr] "r"(marker_vaddr),
          [marker_update] "r"(marker_update),
          [vl] "r"((int)AI_MEMBLOCK_CBOINVAL_VECTOR_BYTES),
          [vtype] "r"(vtype)
        : "t0", "memory");
}

bool ai_micro_hs_cboinval_then_normal_vector_no_stale_inval_owner_or_payload_corner()
{
    TEST_START();

    const enum test_page src_page = SWITCH1;
    const enum test_page dst_page = SWITCH2;
    const enum test_page marker_page = VSRWX_GRWX;
    const uintptr_t src_base_vaddr = hs_page_base(src_page) + 0xac0U;
    const uintptr_t src_base_paddr = phys_page_base(src_page) + 0xac0U;
    const uintptr_t inval_vaddr =
        src_base_vaddr + 4U * sizeof(uint64_t);
    const uintptr_t src_tail_vaddr =
        src_base_vaddr +
        (AI_MEMBLOCK_CBOINVAL_VECTOR_BLOCK_WORDS - 1U) * sizeof(uint64_t);
    const uintptr_t src_before_vaddr =
        src_base_vaddr - sizeof(uint64_t);
    const uintptr_t src_after_vaddr =
        src_base_vaddr + AI_MEMBLOCK_CBOINVAL_VECTOR_BLOCK_WORDS *
        sizeof(uint64_t);
    const uintptr_t src_before_paddr =
        src_base_paddr - sizeof(uint64_t);
    const uintptr_t src_after_paddr =
        src_base_paddr + AI_MEMBLOCK_CBOINVAL_VECTOR_BLOCK_WORDS *
        sizeof(uint64_t);
    const uintptr_t dst_vaddr = hs_page_base(dst_page) + 0xac0U;
    const uintptr_t dst_paddr = phys_page_base(dst_page) + 0xac0U;
    const uintptr_t dst_before_vaddr = dst_vaddr - sizeof(uint64_t);
    const uintptr_t dst_after_vaddr =
        dst_vaddr + AI_MEMBLOCK_CBOINVAL_VECTOR_BYTES;
    const uintptr_t dst_before_paddr = dst_paddr - sizeof(uint64_t);
    const uintptr_t dst_after_paddr =
        dst_paddr + AI_MEMBLOCK_CBOINVAL_VECTOR_BYTES;
    const uintptr_t marker_vaddr = hs_page_base(marker_page) + 0xb40U;
    const uintptr_t marker_paddr = phys_page_base(marker_page) + 0xb40U;
    const uintptr_t marker_guard_vaddr = marker_vaddr + sizeof(uint64_t);
    const uintptr_t marker_guard_paddr = marker_paddr + sizeof(uint64_t);
    const uint64_t src_seeds[] = {
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
    const uint64_t src_before_guards[] = {
        0xfeedfacecafebeefULL,
        0xdeadc0def00dbabeULL,
        0xfaceb00c13579bdfULL,
    };
    const uint64_t src_after_guards[] = {
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

    ai_memblock_cboinval_vector_prepare_hs_env();

    for (size_t round = 0;
         round < AI_MEMBLOCK_CBOINVAL_VECTOR_ARRAY_SIZE(src_seeds);
         ++round) {
        const uint64_t expected_tail =
            src_seeds[round] +
            (AI_MEMBLOCK_CBOINVAL_VECTOR_BLOCK_WORDS - 1U) *
            0x1111111111111111ULL;

        goto_priv(PRIV_M);
        ai_memblock_cboinval_vector_seed_block(src_base_paddr,
            src_seeds[round]);
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
        const uint64_t warmed_src0 = ld(src_base_vaddr);
        const uint64_t warmed_src1 =
            ld(src_base_vaddr + sizeof(uint64_t));
        const uint64_t warmed_tail = ld(src_tail_vaddr);
        const uint64_t warmed_marker = ld(marker_vaddr);
        AI_ASSERT_NO_EXCEPT_AND("before the CBO.INVAL plus normal-vector owner-window starts, source block and marker are readable with old seeded values",
            warmed_src0 == src_seeds[round] &&
            warmed_src1 ==
                src_seeds[round] + 0x1111111111111111ULL &&
            warmed_tail == expected_tail &&
            warmed_marker == marker_seeds[round]
        );

        goto_priv(PRIV_M);
        write64(src_base_paddr, src_word0_updates[round]);
        write64(src_base_paddr + sizeof(uint64_t),
            src_word1_updates[round]);

        goto_priv(PRIV_HS);
        TEST_SETUP_EXCEPT();
        ai_memblock_cboinval_then_normal_vector_copy_marker(inval_vaddr,
            src_base_vaddr, dst_vaddr, marker_vaddr,
            marker_updates[round]);
        const uint64_t observed_src0 = ld(src_base_vaddr);
        const uint64_t observed_src1 =
            ld(src_base_vaddr + sizeof(uint64_t));
        const uint64_t observed_tail = ld(src_tail_vaddr);
        const uint64_t observed_dst0 = ld(dst_vaddr);
        const uint64_t observed_dst1 = ld(dst_vaddr + sizeof(uint64_t));
        const uint64_t observed_marker = ld(marker_vaddr);
        AI_ASSERT_NO_EXCEPT_AND("CBO.INVAL immediately followed by normal vector load/store refills the updated source payload and releases inval ownership before marker progress",
            observed_src0 == src_word0_updates[round] &&
            observed_src1 == src_word1_updates[round] &&
            observed_tail == expected_tail &&
            observed_dst0 == src_word0_updates[round] &&
            observed_dst1 == src_word1_updates[round] &&
            observed_marker == marker_updates[round]
        );

        TEST_SETUP_EXCEPT();
        AI_ASSERT_NO_EXCEPT_AND("the CBO.INVAL plus normal-vector owner-window does not corrupt source, destination, or marker guards",
            ld(src_before_vaddr) == src_before_guards[round] &&
            ld(src_after_vaddr) == src_after_guards[round] &&
            ld(dst_before_vaddr) == dst_before_guards[round] &&
            ld(dst_after_vaddr) == dst_after_guards[round] &&
            ld(marker_guard_vaddr) == marker_guards[round]
        );

        goto_priv(PRIV_M);
        TEST_ASSERT("backing memory after the CBO.INVAL plus normal-vector owner-window contains the updated source words, copied vector destination, marker update, and unchanged guards",
            ai_memblock_cboinval_vector_block_matches_two_updates(
                src_base_paddr,
                src_seeds[round],
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

    ai_memblock_cboinval_vector_reset_hs_env();

    TEST_END();
}

bool ai_micro_hs_normal_vector_then_cboinval_no_stale_vector_writeback_or_inval_drop_corner()
{
    TEST_START();

    const enum test_page src_page = SWITCH1;
    const enum test_page dst_page = SWITCH2;
    const enum test_page marker_page = VSRWX_GRWX;
    const uintptr_t src_vaddr = hs_page_base(src_page) + 0xbc0U;
    const uintptr_t src_paddr = phys_page_base(src_page) + 0xbc0U;
    const uintptr_t src_guard_vaddr =
        src_vaddr + AI_MEMBLOCK_CBOINVAL_VECTOR_BYTES;
    const uintptr_t src_guard_paddr =
        src_paddr + AI_MEMBLOCK_CBOINVAL_VECTOR_BYTES;
    const uintptr_t dst_base_vaddr = hs_page_base(dst_page) + 0xbc0U;
    const uintptr_t dst_base_paddr = phys_page_base(dst_page) + 0xbc0U;
    const uintptr_t inval_vaddr =
        dst_base_vaddr + 4U * sizeof(uint64_t);
    const uintptr_t dst_tail_vaddr =
        dst_base_vaddr +
        (AI_MEMBLOCK_CBOINVAL_VECTOR_BLOCK_WORDS - 1U) * sizeof(uint64_t);
    const uintptr_t dst_before_vaddr =
        dst_base_vaddr - sizeof(uint64_t);
    const uintptr_t dst_after_vaddr =
        dst_base_vaddr + AI_MEMBLOCK_CBOINVAL_VECTOR_BLOCK_WORDS *
        sizeof(uint64_t);
    const uintptr_t dst_before_paddr =
        dst_base_paddr - sizeof(uint64_t);
    const uintptr_t dst_after_paddr =
        dst_base_paddr + AI_MEMBLOCK_CBOINVAL_VECTOR_BLOCK_WORDS *
        sizeof(uint64_t);
    const uintptr_t marker_vaddr = hs_page_base(marker_page) + 0xc40U;
    const uintptr_t marker_paddr = phys_page_base(marker_page) + 0xc40U;
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

    ai_memblock_cboinval_vector_prepare_hs_env();

    for (size_t round = 0;
         round < AI_MEMBLOCK_CBOINVAL_VECTOR_ARRAY_SIZE(dst_seeds);
         ++round) {
        const uint64_t expected_tail =
            dst_seeds[round] +
            (AI_MEMBLOCK_CBOINVAL_VECTOR_BLOCK_WORDS - 1U) *
            0x1111111111111111ULL;

        goto_priv(PRIV_M);
        write64(src_paddr, src_word0_values[round]);
        write64(src_paddr + sizeof(uint64_t), src_word1_values[round]);
        write64(src_guard_paddr, src_guards[round]);
        ai_memblock_cboinval_vector_seed_block(dst_base_paddr,
            dst_seeds[round]);
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
        AI_ASSERT_NO_EXCEPT_AND("before the normal-vector plus CBO.INVAL owner-window starts, source, destination tail, and marker are readable with seeded values",
            warmed_src0 == src_word0_values[round] &&
            warmed_src1 == src_word1_values[round] &&
            warmed_dst_tail == expected_tail &&
            warmed_marker == marker_seeds[round]
        );

        TEST_SETUP_EXCEPT();
        ai_memblock_normal_vector_copy_then_cboinval_marker(src_vaddr,
            dst_base_vaddr, inval_vaddr, marker_vaddr,
            marker_updates[round]);
        const uint64_t observed_dst0 = ld(dst_base_vaddr);
        const uint64_t observed_dst1 =
            ld(dst_base_vaddr + sizeof(uint64_t));
        const uint64_t observed_dst_tail = ld(dst_tail_vaddr);
        const uint64_t observed_marker = ld(marker_vaddr);
        AI_ASSERT_NO_EXCEPT_AND("normal vector load/store immediately followed by CBO.INVAL preserves vector payload before inval ownership releases to marker progress",
            observed_dst0 == src_word0_values[round] &&
            observed_dst1 == src_word1_values[round] &&
            observed_dst_tail == expected_tail &&
            observed_marker == marker_updates[round]
        );

        TEST_SETUP_EXCEPT();
        AI_ASSERT_NO_EXCEPT_AND("the normal-vector plus CBO.INVAL owner-window does not corrupt source, destination, or marker guards",
            ld(src_guard_vaddr) == src_guards[round] &&
            ld(dst_before_vaddr) == dst_before_guards[round] &&
            ld(dst_after_vaddr) == dst_after_guards[round] &&
            ld(marker_guard_vaddr) == marker_guards[round]
        );

        goto_priv(PRIV_M);
        TEST_ASSERT("backing memory after the normal-vector plus CBO.INVAL owner-window contains the copied vector payload, marker update, and unchanged guards",
            read64(src_paddr) == src_word0_values[round] &&
            read64(src_paddr + sizeof(uint64_t)) ==
                src_word1_values[round] &&
            read64(src_guard_paddr) == src_guards[round] &&
            ai_memblock_cboinval_vector_block_matches_two_updates(
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

    ai_memblock_cboinval_vector_reset_hs_env();

    TEST_END();
}
