#include <rvh_test.h>
#include <page_tables.h>
#include <instructions.h>
#include <stdint.h>

#define AI_ASSERT_NO_EXCEPT_AND(message, cond) \
    TEST_ASSERT(message, excpt.triggered == false && (cond))
#define AI_MEMBLOCK_CBOZERO_VFOF_ARRAY_SIZE(array) \
    (sizeof(array) / sizeof((array)[0]))
#define AI_MEMBLOCK_CBOZERO_VFOF_BLOCK_WORDS 8U
#define AI_MEMBLOCK_CBOZERO_VFOF_BYTES 16U
#define AI_MEMBLOCK_CBOZERO_VFOF_PARTIAL_BYTES 8U

static inline pte_t *ai_memblock_cbozero_vfof_hs_leaf_entry(
    enum test_page page)
{
#ifdef sv48
    return &hspt[4][page];
#else
    return &hspt[2][page];
#endif
}

static inline void ai_memblock_cbozero_vfof_set_hs_leaf(
    enum test_page page,
    uint64_t flags)
{
    *ai_memblock_cbozero_vfof_hs_leaf_entry(page) =
        (phys_page_base(page) >> 2) | flags;
}

static inline void ai_memblock_cbozero_vfof_prepare_hs_env(void)
{
    goto_priv(PRIV_M);
    CSRS(CSR_MENVCFG, MENVCFG_CBZE);
    hspt_init();
    sfence_vma();
}

static inline void ai_memblock_cbozero_vfof_reset_hs_env(void)
{
    goto_priv(PRIV_M);
    CSRC(CSR_MENVCFG, MENVCFG_CBZE);
    hspt_init();
    sfence_vma();
}

static inline void ai_memblock_cbozero_vfof_seed_block(uintptr_t paddr,
    uint64_t seed)
{
    for (size_t i = 0; i < AI_MEMBLOCK_CBOZERO_VFOF_BLOCK_WORDS; ++i) {
        write64(paddr + i * sizeof(uint64_t),
            seed + i * 0x1111111111111111ULL);
    }
}

static inline bool ai_memblock_cbozero_vfof_block_all_zero(uintptr_t paddr)
{
    for (size_t i = 0; i < AI_MEMBLOCK_CBOZERO_VFOF_BLOCK_WORDS; ++i) {
        if (read64(paddr + i * sizeof(uint64_t)) != 0ULL) {
            return false;
        }
    }

    return true;
}

static inline bool ai_memblock_cbozero_vfof_range_all_zero(uintptr_t paddr,
    unsigned len)
{
    for (unsigned i = 0; i < len; ++i) {
        if (read8(paddr + i) != 0U) {
            return false;
        }
    }

    return true;
}

static inline void ai_memblock_cbozero_vfof_write_bytes(uintptr_t paddr,
    const uint8_t *bytes,
    unsigned len)
{
    for (unsigned i = 0; i < len; ++i) {
        write8(paddr + i, bytes[i]);
    }
}

static inline bool ai_memblock_cbozero_vfof_bytes_match(uintptr_t paddr,
    const uint8_t *bytes,
    unsigned len)
{
    for (unsigned i = 0; i < len; ++i) {
        if (read8(paddr + i) != bytes[i]) {
            return false;
        }
    }

    return true;
}

static inline void ai_memblock_cbozero_then_vfof_copy_marker(
    uintptr_t zero_vaddr,
    uintptr_t src_vaddr,
    uintptr_t dst_vaddr,
    uintptr_t marker_vaddr,
    uint64_t marker_update,
    unsigned *observed_vl)
{
    const int vtype = VTYPE(VSEW_FROM_BITS(8), 0);
    unsigned captured_vl = 0U;

    asm volatile(
        ".option push\n\t"
        ".option norvc\n\t"
        ".insn i 0x0f, 0x2, x0, %[zero_addr], 4\n\t"
        "vsetvl t0, %[vl], %[vtype]\n\t"
        "vle8ff.v v8, (%[src_addr])\n\t"
        "csrr %[out_vl], vl\n\t"
        "vse8.v v8, (%[dst_addr])\n\t"
        "sd %[marker_update], 0(%[marker_addr])\n\t"
        "fence rw, rw\n\t"
        ".option pop\n\t"
        : [out_vl] "=&r"(captured_vl)
        : [zero_addr] "r"(zero_vaddr),
          [src_addr] "r"(src_vaddr),
          [dst_addr] "r"(dst_vaddr),
          [marker_addr] "r"(marker_vaddr),
          [marker_update] "r"(marker_update),
          [vl] "r"((int)AI_MEMBLOCK_CBOZERO_VFOF_BYTES),
          [vtype] "r"(vtype)
        : "t0", "memory");

    *observed_vl = captured_vl;
}

bool ai_micro_hs_cbozero_then_vfof_no_stale_zero_flush_owner_or_fof_payload_corner()
{
    TEST_START();

    const enum test_page src_page = SWITCH1;
    const enum test_page dst_page = SWITCH2;
    const enum test_page marker_page = VSRWX_GRWX;
    const uintptr_t src_base_vaddr = hs_page_base(src_page) + 0xf00U;
    const uintptr_t src_base_paddr = phys_page_base(src_page) + 0xf00U;
    const uintptr_t zero_subblock_vaddr =
        src_base_vaddr + 4U * sizeof(uint64_t);
    const uintptr_t src_tail_vaddr =
        src_base_vaddr +
        (AI_MEMBLOCK_CBOZERO_VFOF_BLOCK_WORDS - 1U) * sizeof(uint64_t);
    const uintptr_t src_before_vaddr =
        src_base_vaddr - sizeof(uint64_t);
    const uintptr_t src_after_vaddr =
        src_base_vaddr + AI_MEMBLOCK_CBOZERO_VFOF_BLOCK_WORDS *
        sizeof(uint64_t);
    const uintptr_t src_before_paddr =
        src_base_paddr - sizeof(uint64_t);
    const uintptr_t src_after_paddr =
        src_base_paddr + AI_MEMBLOCK_CBOZERO_VFOF_BLOCK_WORDS *
        sizeof(uint64_t);
    const uintptr_t dst_vaddr = hs_page_base(dst_page) + 0xf00U;
    const uintptr_t dst_paddr = phys_page_base(dst_page) + 0xf00U;
    const uintptr_t dst_before_vaddr = dst_vaddr - sizeof(uint64_t);
    const uintptr_t dst_after_vaddr =
        dst_vaddr + AI_MEMBLOCK_CBOZERO_VFOF_BYTES;
    const uintptr_t dst_before_paddr = dst_paddr - sizeof(uint64_t);
    const uintptr_t dst_after_paddr =
        dst_paddr + AI_MEMBLOCK_CBOZERO_VFOF_BYTES;
    const uintptr_t marker_vaddr = hs_page_base(marker_page) + 0xf00U;
    const uintptr_t marker_paddr = phys_page_base(marker_page) + 0xf00U;
    const uintptr_t marker_guard_vaddr = marker_vaddr + sizeof(uint64_t);
    const uintptr_t marker_guard_paddr = marker_paddr + sizeof(uint64_t);
    const uint64_t src_seeds[] = {
        0x1021324354657687ULL,
        0x89abcdef01234567ULL,
        0xf0e1d2c3b4a59687ULL,
    };
    const uint64_t dst_fills[] = {
        0xc3c3c3c3c3c3c3c3ULL,
        0x3c3c3c3c3c3c3c3cULL,
        0xa55aa55ac33cc33cULL,
    };
    const uint64_t src_before_guards[] = {
        0x13579bdf2468ace0ULL,
        0x02468ace13579bdfULL,
        0xfdb97531eca86420ULL,
    };
    const uint64_t src_after_guards[] = {
        0x1123581321345589ULL,
        0x3141592653589793ULL,
        0x2718281828459045ULL,
    };
    const uint64_t dst_before_guards[] = {
        0xfeedfacecafebeefULL,
        0xdeadc0def00dbabeULL,
        0xfaceb00c13579bdfULL,
    };
    const uint64_t dst_after_guards[] = {
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
        0x8877665544332211ULL,
        0x7766554433221100ULL,
        0x66554433221100ffULL,
    };
    const uint64_t marker_guards[] = {
        0x55aa33cc7799bbddULL,
        0x1234fedc5678ba90ULL,
        0x0badc0de55aa33ccULL,
    };

    ai_memblock_cbozero_vfof_prepare_hs_env();

    for (size_t round = 0;
         round < AI_MEMBLOCK_CBOZERO_VFOF_ARRAY_SIZE(src_seeds);
         ++round) {
        const uint64_t expected_src_tail =
            src_seeds[round] +
            (AI_MEMBLOCK_CBOZERO_VFOF_BLOCK_WORDS - 1U) *
            0x1111111111111111ULL;
        unsigned observed_vl = 0U;

        goto_priv(PRIV_M);
        ai_memblock_cbozero_vfof_seed_block(src_base_paddr,
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
        const uint64_t warmed_src_tail = ld(src_tail_vaddr);
        const uint64_t warmed_marker = ld(marker_vaddr);
        AI_ASSERT_NO_EXCEPT_AND("before the CBO.ZERO plus VFOF owner-window starts, source block and marker are readable with old seeded values",
            warmed_src0 == src_seeds[round] &&
            warmed_src1 ==
                src_seeds[round] + 0x1111111111111111ULL &&
            warmed_src_tail == expected_src_tail &&
            warmed_marker == marker_seeds[round]
        );

        TEST_SETUP_EXCEPT();
        ai_memblock_cbozero_then_vfof_copy_marker(zero_subblock_vaddr,
            src_base_vaddr, dst_vaddr, marker_vaddr,
            marker_updates[round], &observed_vl);
        const uint64_t observed_src0 = ld(src_base_vaddr);
        const uint64_t observed_src1 =
            ld(src_base_vaddr + sizeof(uint64_t));
        const uint64_t observed_src_tail = ld(src_tail_vaddr);
        const uint64_t observed_dst0 = ld(dst_vaddr);
        const uint64_t observed_dst1 = ld(dst_vaddr + sizeof(uint64_t));
        const uint64_t observed_marker = ld(marker_vaddr);
        AI_ASSERT_NO_EXCEPT_AND("CBO.ZERO immediately followed by VFOF keeps full vl, exposes only zeroed source payload, and releases marker progress",
            observed_vl == AI_MEMBLOCK_CBOZERO_VFOF_BYTES &&
            observed_src0 == 0ULL &&
            observed_src1 == 0ULL &&
            observed_src_tail == 0ULL &&
            observed_dst0 == 0ULL &&
            observed_dst1 == 0ULL &&
            observed_marker == marker_updates[round]
        );

        TEST_SETUP_EXCEPT();
        AI_ASSERT_NO_EXCEPT_AND("the CBO.ZERO plus VFOF owner-window does not corrupt source, destination, or marker guards",
            ld(src_before_vaddr) == src_before_guards[round] &&
            ld(src_after_vaddr) == src_after_guards[round] &&
            ld(dst_before_vaddr) == dst_before_guards[round] &&
            ld(dst_after_vaddr) == dst_after_guards[round] &&
            ld(marker_guard_vaddr) == marker_guards[round]
        );

        goto_priv(PRIV_M);
        TEST_ASSERT("backing memory after the CBO.ZERO plus VFOF owner-window is fully zeroed at the source block, captures only zero payload, updates marker, and preserves guards",
            ai_memblock_cbozero_vfof_block_all_zero(src_base_paddr) &&
            read64(src_before_paddr) == src_before_guards[round] &&
            read64(src_after_paddr) == src_after_guards[round] &&
            read64(dst_paddr) == 0ULL &&
            read64(dst_paddr + sizeof(uint64_t)) == 0ULL &&
            read64(dst_before_paddr) == dst_before_guards[round] &&
            read64(dst_after_paddr) == dst_after_guards[round] &&
            read64(marker_paddr) == marker_updates[round] &&
            read64(marker_guard_paddr) == marker_guards[round]
        );
    }

    ai_memblock_cbozero_vfof_reset_hs_env();

    TEST_END();
}

static inline void ai_memblock_vfof_copy_then_cbozero_marker(
    uintptr_t src_vaddr,
    uintptr_t dst_vaddr,
    uintptr_t capture_vaddr,
    uintptr_t zero_vaddr,
    uintptr_t marker_vaddr,
    uint64_t marker_update,
    unsigned *observed_vl)
{
    const int vtype = VTYPE(VSEW_FROM_BITS(8), 0);
    unsigned captured_vl = 0U;

    asm volatile(
        ".option push\n\t"
        ".option norvc\n\t"
        "vsetvl t0, %[vl], %[vtype]\n\t"
        "vle8ff.v v8, (%[src_addr])\n\t"
        "csrr %[out_vl], vl\n\t"
        "vse8.v v8, (%[dst_addr])\n\t"
        "vse8.v v8, (%[capture_addr])\n\t"
        ".insn i 0x0f, 0x2, x0, %[zero_addr], 4\n\t"
        "sd %[marker_update], 0(%[marker_addr])\n\t"
        "fence rw, rw\n\t"
        ".option pop\n\t"
        : [out_vl] "=&r"(captured_vl)
        : [src_addr] "r"(src_vaddr),
          [dst_addr] "r"(dst_vaddr),
          [capture_addr] "r"(capture_vaddr),
          [zero_addr] "r"(zero_vaddr),
          [marker_addr] "r"(marker_vaddr),
          [marker_update] "r"(marker_update),
          [vl] "r"((int)AI_MEMBLOCK_CBOZERO_VFOF_BYTES),
          [vtype] "r"(vtype)
        : "t0", "memory");

    *observed_vl = captured_vl;
}

bool ai_micro_hs_vfof_then_cbozero_no_stale_fof_writeback_or_zero_flush_owner_corner()
{
    TEST_START();

    const enum test_page src_page = SWITCH1;
    const enum test_page dst_page = SWITCH2;
    const enum test_page aux_page = VSRWX_GRWX;
    const uintptr_t src_vaddr = hs_page_base(src_page) + 0xf40U;
    const uintptr_t src_paddr = phys_page_base(src_page) + 0xf40U;
    const uintptr_t src_guard_vaddr =
        src_vaddr + AI_MEMBLOCK_CBOZERO_VFOF_BYTES;
    const uintptr_t src_guard_paddr =
        src_paddr + AI_MEMBLOCK_CBOZERO_VFOF_BYTES;
    const uintptr_t dst_base_vaddr = hs_page_base(dst_page) + 0xf40U;
    const uintptr_t dst_base_paddr = phys_page_base(dst_page) + 0xf40U;
    const uintptr_t zero_subblock_vaddr =
        dst_base_vaddr + 4U * sizeof(uint64_t);
    const uintptr_t dst_tail_vaddr =
        dst_base_vaddr +
        (AI_MEMBLOCK_CBOZERO_VFOF_BLOCK_WORDS - 1U) * sizeof(uint64_t);
    const uintptr_t dst_before_vaddr =
        dst_base_vaddr - sizeof(uint64_t);
    const uintptr_t dst_after_vaddr =
        dst_base_vaddr + AI_MEMBLOCK_CBOZERO_VFOF_BLOCK_WORDS *
        sizeof(uint64_t);
    const uintptr_t dst_before_paddr =
        dst_base_paddr - sizeof(uint64_t);
    const uintptr_t dst_after_paddr =
        dst_base_paddr + AI_MEMBLOCK_CBOZERO_VFOF_BLOCK_WORDS *
        sizeof(uint64_t);
    const uintptr_t capture_vaddr = hs_page_base(aux_page) + 0xf20U;
    const uintptr_t capture_paddr = phys_page_base(aux_page) + 0xf20U;
    const uintptr_t capture_before_vaddr =
        capture_vaddr - sizeof(uint64_t);
    const uintptr_t capture_after_vaddr =
        capture_vaddr + AI_MEMBLOCK_CBOZERO_VFOF_BYTES;
    const uintptr_t capture_before_paddr =
        capture_paddr - sizeof(uint64_t);
    const uintptr_t capture_after_paddr =
        capture_paddr + AI_MEMBLOCK_CBOZERO_VFOF_BYTES;
    const uintptr_t marker_vaddr = hs_page_base(aux_page) + 0xf80U;
    const uintptr_t marker_paddr = phys_page_base(aux_page) + 0xf80U;
    const uintptr_t marker_guard_vaddr = marker_vaddr + sizeof(uint64_t);
    const uintptr_t marker_guard_paddr = marker_paddr + sizeof(uint64_t);
    const uint64_t src_word0_values[] = {
        0x0011223344556677ULL,
        0x8899aabbccddeeffULL,
        0x13579bdf2468ace0ULL,
    };
    const uint64_t src_word1_values[] = {
        0x7766554433221100ULL,
        0xffeeddccbbaa9988ULL,
        0x0eca8642fdb97531ULL,
    };
    const uint64_t dst_seeds[] = {
        0x1021324354657687ULL,
        0x89abcdef01234567ULL,
        0xf0e1d2c3b4a59687ULL,
    };
    const uint64_t capture_fills[] = {
        0xc3c3c3c3c3c3c3c3ULL,
        0x3c3c3c3c3c3c3c3cULL,
        0xa55aa55ac33cc33cULL,
    };
    const uint64_t src_guards[] = {
        0x55aa33cc7799bbddULL,
        0x1234fedc5678ba90ULL,
        0x0badc0de55aa33ccULL,
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
    const uint64_t capture_before_guards[] = {
        0xfeedfacecafebeefULL,
        0xdeadc0def00dbabeULL,
        0xfaceb00c13579bdfULL,
    };
    const uint64_t capture_after_guards[] = {
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
        0x8877665544332211ULL,
        0x7766554433221100ULL,
        0x66554433221100ffULL,
    };
    const uint64_t marker_guards[] = {
        0xa1b2c3d4e5f60718ULL,
        0x1029384756a5b4c3ULL,
        0x0f1e2d3c4b5a6978ULL,
    };

    ai_memblock_cbozero_vfof_prepare_hs_env();

    for (size_t round = 0;
         round < AI_MEMBLOCK_CBOZERO_VFOF_ARRAY_SIZE(dst_seeds);
         ++round) {
        const uint64_t expected_dst_tail =
            dst_seeds[round] +
            (AI_MEMBLOCK_CBOZERO_VFOF_BLOCK_WORDS - 1U) *
            0x1111111111111111ULL;
        unsigned observed_vl = 0U;

        goto_priv(PRIV_M);
        write64(src_paddr, src_word0_values[round]);
        write64(src_paddr + sizeof(uint64_t), src_word1_values[round]);
        write64(src_guard_paddr, src_guards[round]);
        ai_memblock_cbozero_vfof_seed_block(dst_base_paddr,
            dst_seeds[round]);
        write64(dst_before_paddr, dst_before_guards[round]);
        write64(dst_after_paddr, dst_after_guards[round]);
        write64(capture_paddr, capture_fills[round]);
        write64(capture_paddr + sizeof(uint64_t), ~capture_fills[round]);
        write64(capture_before_paddr, capture_before_guards[round]);
        write64(capture_after_paddr, capture_after_guards[round]);
        write64(marker_paddr, marker_seeds[round]);
        write64(marker_guard_paddr, marker_guards[round]);

        goto_priv(PRIV_HS);
        TEST_SETUP_EXCEPT();
        const uint64_t warmed_src0 = ld(src_vaddr);
        const uint64_t warmed_src1 = ld(src_vaddr + sizeof(uint64_t));
        const uint64_t warmed_dst_tail = ld(dst_tail_vaddr);
        const uint64_t warmed_marker = ld(marker_vaddr);
        AI_ASSERT_NO_EXCEPT_AND("before the VFOF plus CBO.ZERO owner-window starts, source payload, destination tail, and marker are readable with seeded values",
            warmed_src0 == src_word0_values[round] &&
            warmed_src1 == src_word1_values[round] &&
            warmed_dst_tail == expected_dst_tail &&
            warmed_marker == marker_seeds[round]
        );

        TEST_SETUP_EXCEPT();
        ai_memblock_vfof_copy_then_cbozero_marker(src_vaddr,
            dst_base_vaddr, capture_vaddr, zero_subblock_vaddr,
            marker_vaddr, marker_updates[round], &observed_vl);
        const uint64_t observed_dst0 = ld(dst_base_vaddr);
        const uint64_t observed_dst1 =
            ld(dst_base_vaddr + sizeof(uint64_t));
        const uint64_t observed_dst_tail = ld(dst_tail_vaddr);
        const uint64_t observed_capture0 = ld(capture_vaddr);
        const uint64_t observed_capture1 =
            ld(capture_vaddr + sizeof(uint64_t));
        const uint64_t observed_marker = ld(marker_vaddr);
        AI_ASSERT_NO_EXCEPT_AND("VFOF store-out immediately followed by CBO.ZERO preserves full-vl FOF payload in capture, zeroes the target block, and releases marker progress",
            observed_vl == AI_MEMBLOCK_CBOZERO_VFOF_BYTES &&
            observed_dst0 == 0ULL &&
            observed_dst1 == 0ULL &&
            observed_dst_tail == 0ULL &&
            observed_capture0 == src_word0_values[round] &&
            observed_capture1 == src_word1_values[round] &&
            observed_marker == marker_updates[round]
        );

        TEST_SETUP_EXCEPT();
        AI_ASSERT_NO_EXCEPT_AND("the VFOF plus CBO.ZERO owner-window does not corrupt source, destination, capture, or marker guards",
            ld(src_guard_vaddr) == src_guards[round] &&
            ld(dst_before_vaddr) == dst_before_guards[round] &&
            ld(dst_after_vaddr) == dst_after_guards[round] &&
            ld(capture_before_vaddr) == capture_before_guards[round] &&
            ld(capture_after_vaddr) == capture_after_guards[round] &&
            ld(marker_guard_vaddr) == marker_guards[round]
        );

        goto_priv(PRIV_M);
        TEST_ASSERT("backing memory after the VFOF plus CBO.ZERO owner-window keeps the FOF source and capture payload, fully zeroes destination, updates marker, and preserves guards",
            read64(src_paddr) == src_word0_values[round] &&
            read64(src_paddr + sizeof(uint64_t)) ==
                src_word1_values[round] &&
            read64(src_guard_paddr) == src_guards[round] &&
            ai_memblock_cbozero_vfof_block_all_zero(dst_base_paddr) &&
            read64(dst_before_paddr) == dst_before_guards[round] &&
            read64(dst_after_paddr) == dst_after_guards[round] &&
            read64(capture_paddr) == src_word0_values[round] &&
            read64(capture_paddr + sizeof(uint64_t)) ==
                src_word1_values[round] &&
            read64(capture_before_paddr) == capture_before_guards[round] &&
            read64(capture_after_paddr) == capture_after_guards[round] &&
            read64(marker_paddr) == marker_updates[round] &&
            read64(marker_guard_paddr) == marker_guards[round]
        );
    }

    ai_memblock_cbozero_vfof_reset_hs_env();

    TEST_END();
}

static inline void ai_memblock_cbozero_then_partial_vfof_capture_marker(
    uintptr_t zero_vaddr,
    uintptr_t fof_src_vaddr,
    uintptr_t fof_dst_vaddr,
    uintptr_t marker_vaddr,
    uint64_t marker_update,
    unsigned *observed_vl)
{
    const int vtype = VTYPE(VSEW_FROM_BITS(8), 0);
    unsigned captured_vl = 0U;

    asm volatile(
        ".option push\n\t"
        ".option norvc\n\t"
        ".insn i 0x0f, 0x2, x0, %[zero_addr], 4\n\t"
        "vsetvl t0, %[vl], %[vtype]\n\t"
        "vle8ff.v v8, (%[fof_src_addr])\n\t"
        "csrr %[out_vl], vl\n\t"
        "vse8.v v8, (%[fof_dst_addr])\n\t"
        "sd %[marker_update], 0(%[marker_addr])\n\t"
        "fence rw, rw\n\t"
        ".option pop\n\t"
        : [out_vl] "=&r"(captured_vl)
        : [zero_addr] "r"(zero_vaddr),
          [fof_src_addr] "r"(fof_src_vaddr),
          [fof_dst_addr] "r"(fof_dst_vaddr),
          [marker_addr] "r"(marker_vaddr),
          [marker_update] "r"(marker_update),
          [vl] "r"((int)AI_MEMBLOCK_CBOZERO_VFOF_BYTES),
          [vtype] "r"(vtype)
        : "t0", "memory");

    *observed_vl = captured_vl;
}

bool ai_micro_hs_cbozero_then_vfof_partial_stop_no_stale_zero_owner_or_vl_payload_corner()
{
    TEST_START();

    const enum test_page low_page = SWITCH1;
    const enum test_page high_page = SWITCH2;
    const enum test_page aux_page = VSRWX_GRWX;
    const uintptr_t low_tail_vaddr =
        hs_page_base(low_page) + PAGE_SIZE -
        AI_MEMBLOCK_CBOZERO_VFOF_PARTIAL_BYTES;
    const uintptr_t zero_vaddr = low_tail_vaddr;
    const uintptr_t low_tail_paddr =
        phys_page_base(low_page) + PAGE_SIZE -
        AI_MEMBLOCK_CBOZERO_VFOF_PARTIAL_BYTES;
    const uintptr_t zero_block_paddr =
        phys_page_base(low_page) + PAGE_SIZE -
        AI_MEMBLOCK_CBOZERO_VFOF_BLOCK_WORDS * sizeof(uint64_t);
    const uintptr_t zero_block_before_vaddr =
        hs_page_base(low_page) + PAGE_SIZE -
        AI_MEMBLOCK_CBOZERO_VFOF_BLOCK_WORDS * sizeof(uint64_t) -
        sizeof(uint64_t);
    const uintptr_t zero_block_before_paddr =
        phys_page_base(low_page) + PAGE_SIZE -
        AI_MEMBLOCK_CBOZERO_VFOF_BLOCK_WORDS * sizeof(uint64_t) -
        sizeof(uint64_t);
    const uintptr_t high_head_paddr = phys_page_base(high_page);
    const uintptr_t fof_dst_vaddr = hs_page_base(aux_page) + 0xfa0U;
    const uintptr_t fof_dst_paddr = phys_page_base(aux_page) + 0xfa0U;
    const uintptr_t fof_dst_before_vaddr =
        fof_dst_vaddr - sizeof(uint64_t);
    const uintptr_t fof_dst_after_vaddr =
        fof_dst_vaddr + AI_MEMBLOCK_CBOZERO_VFOF_BYTES;
    const uintptr_t fof_dst_before_paddr =
        fof_dst_paddr - sizeof(uint64_t);
    const uintptr_t fof_dst_after_paddr =
        fof_dst_paddr + AI_MEMBLOCK_CBOZERO_VFOF_BYTES;
    const uintptr_t marker_vaddr = hs_page_base(aux_page) + 0xfe0U;
    const uintptr_t marker_paddr = phys_page_base(aux_page) + 0xfe0U;
    const uintptr_t marker_guard_vaddr = marker_vaddr + sizeof(uint64_t);
    const uintptr_t marker_guard_paddr = marker_paddr + sizeof(uint64_t);
    static const uint8_t low_tail_patterns[][AI_MEMBLOCK_CBOZERO_VFOF_PARTIAL_BYTES] = {
        { 0x10, 0x21, 0x32, 0x43, 0x54, 0x65, 0x76, 0x87 },
        { 0xa0, 0xb1, 0xc2, 0xd3, 0xe4, 0xf5, 0x16, 0x27 },
        { 0x5a, 0x4b, 0x3c, 0x2d, 0x1e, 0x0f, 0x7d, 0x6c },
    };
    static const uint8_t high_head_patterns[][AI_MEMBLOCK_CBOZERO_VFOF_PARTIAL_BYTES] = {
        { 0x88, 0x99, 0xaa, 0xbb, 0xcc, 0xdd, 0xee, 0xff },
        { 0x17, 0x28, 0x39, 0x4a, 0x5b, 0x6c, 0x7d, 0x8e },
        { 0xf0, 0xe1, 0xd2, 0xc3, 0xb4, 0xa5, 0x96, 0x87 },
    };
    const uint64_t low_tail_words[] = {
        0x8776655443322110ULL,
        0x2716f5e4d3c2b1a0ULL,
        0x6c7d0f1e2d3c4b5aULL,
    };
    const uint64_t zero_block_seeds[] = {
        0x1021324354657687ULL,
        0x89abcdef01234567ULL,
        0xf0e1d2c3b4a59687ULL,
    };
    const uint64_t zero_block_before_guards[] = {
        0x13579bdf2468ace0ULL,
        0x02468ace13579bdfULL,
        0xfdb97531eca86420ULL,
    };
    const uint64_t fof_dst_fills[] = {
        0xc3c3c3c3c3c3c3c3ULL,
        0x3c3c3c3c3c3c3c3cULL,
        0xa55aa55ac33cc33cULL,
    };
    const uint64_t fof_dst_before_guards[] = {
        0xfeedfacecafebeefULL,
        0xdeadc0def00dbabeULL,
        0xfaceb00c13579bdfULL,
    };
    const uint64_t fof_dst_after_guards[] = {
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
        0x8877665544332211ULL,
        0x7766554433221100ULL,
        0x66554433221100ffULL,
    };
    const uint64_t marker_guards[] = {
        0xa1b2c3d4e5f60718ULL,
        0x1029384756a5b4c3ULL,
        0x0f1e2d3c4b5a6978ULL,
    };

    ai_memblock_cbozero_vfof_prepare_hs_env();

    for (size_t round = 0;
         round < AI_MEMBLOCK_CBOZERO_VFOF_ARRAY_SIZE(zero_block_seeds);
         ++round) {
        unsigned observed_vl = 0U;

        goto_priv(PRIV_M);
        ai_memblock_cbozero_vfof_seed_block(zero_block_paddr,
            zero_block_seeds[round]);
        ai_memblock_cbozero_vfof_write_bytes(low_tail_paddr,
            low_tail_patterns[round],
            AI_MEMBLOCK_CBOZERO_VFOF_PARTIAL_BYTES);
        ai_memblock_cbozero_vfof_write_bytes(high_head_paddr,
            high_head_patterns[round],
            AI_MEMBLOCK_CBOZERO_VFOF_PARTIAL_BYTES);
        write64(zero_block_before_paddr, zero_block_before_guards[round]);
        write64(fof_dst_paddr, fof_dst_fills[round]);
        write64(fof_dst_paddr + sizeof(uint64_t),
            ~fof_dst_fills[round]);
        write64(fof_dst_before_paddr, fof_dst_before_guards[round]);
        write64(fof_dst_after_paddr, fof_dst_after_guards[round]);
        write64(marker_paddr, marker_seeds[round]);
        write64(marker_guard_paddr, marker_guards[round]);

        goto_priv(PRIV_HS);
        hspt_init();
        ai_memblock_cbozero_vfof_set_hs_leaf(low_page,
            PTE_V | PTE_AD | PTE_RWX);
        ai_memblock_cbozero_vfof_set_hs_leaf(high_page,
            PTE_V | PTE_AD | PTE_X);
        ai_memblock_cbozero_vfof_set_hs_leaf(aux_page,
            PTE_V | PTE_AD | PTE_RWX);
        sfence_vma();

        TEST_SETUP_EXCEPT();
        const uint64_t warmed_low_tail = ld(low_tail_vaddr);
        const uint64_t warmed_block_before = ld(zero_block_before_vaddr);
        const uint64_t warmed_marker = ld(marker_vaddr);
        AI_ASSERT_NO_EXCEPT_AND("before the CBO.ZERO plus partial-stop VFOF window starts, the low-page tail and marker are readable while the next page is execute-only",
            warmed_low_tail == low_tail_words[round] &&
            warmed_block_before == zero_block_before_guards[round] &&
            warmed_marker == marker_seeds[round]
        );

        TEST_SETUP_EXCEPT();
        ai_memblock_cbozero_then_partial_vfof_capture_marker(zero_vaddr,
            low_tail_vaddr, fof_dst_vaddr, marker_vaddr,
            marker_updates[round], &observed_vl);
        const uint64_t observed_low_tail = ld(low_tail_vaddr);
        const uint64_t observed_fof_dst0 = ld(fof_dst_vaddr);
        const uint64_t observed_fof_dst1 =
            ld(fof_dst_vaddr + sizeof(uint64_t));
        const uint64_t observed_marker = ld(marker_vaddr);
        AI_ASSERT_NO_EXCEPT_AND("CBO.ZERO immediately followed by cross-page partial-stop VFOF shrinks vl to the zeroed low-page bytes, stores only zero payload, and releases marker progress",
            observed_vl == AI_MEMBLOCK_CBOZERO_VFOF_PARTIAL_BYTES &&
            observed_low_tail == 0ULL &&
            observed_fof_dst0 == 0ULL &&
            observed_fof_dst1 == ~fof_dst_fills[round] &&
            observed_marker == marker_updates[round]
        );

        TEST_SETUP_EXCEPT();
        AI_ASSERT_NO_EXCEPT_AND("the CBO.ZERO plus partial-stop VFOF window preserves adjacent low-page, destination, and marker guards",
            ld(zero_block_before_vaddr) == zero_block_before_guards[round] &&
            ld(fof_dst_before_vaddr) == fof_dst_before_guards[round] &&
            ld(fof_dst_after_vaddr) == fof_dst_after_guards[round] &&
            ld(marker_guard_vaddr) == marker_guards[round]
        );

        goto_priv(PRIV_M);
        TEST_ASSERT("backing memory after the CBO.ZERO plus partial-stop VFOF window zeroes only the low-page block, leaves the execute-only high page untouched, stores only the shortened zero payload, updates marker, and preserves guards",
            ai_memblock_cbozero_vfof_range_all_zero(zero_block_paddr,
                AI_MEMBLOCK_CBOZERO_VFOF_BLOCK_WORDS * sizeof(uint64_t)) &&
            ai_memblock_cbozero_vfof_bytes_match(high_head_paddr,
                high_head_patterns[round],
                AI_MEMBLOCK_CBOZERO_VFOF_PARTIAL_BYTES) &&
            read64(zero_block_before_paddr) ==
                zero_block_before_guards[round] &&
            read64(fof_dst_paddr) == 0ULL &&
            read64(fof_dst_paddr + sizeof(uint64_t)) ==
                ~fof_dst_fills[round] &&
            read64(fof_dst_before_paddr) == fof_dst_before_guards[round] &&
            read64(fof_dst_after_paddr) == fof_dst_after_guards[round] &&
            read64(marker_paddr) == marker_updates[round] &&
            read64(marker_guard_paddr) == marker_guards[round]
        );
    }

    ai_memblock_cbozero_vfof_reset_hs_env();

    TEST_END();
}

static inline void ai_memblock_partial_vfof_capture_then_cbozero_marker(
    uintptr_t fof_src_vaddr,
    uintptr_t fof_dst_vaddr,
    uintptr_t zero_vaddr,
    uintptr_t marker_vaddr,
    uint64_t marker_update,
    unsigned *observed_vl)
{
    const int vtype = VTYPE(VSEW_FROM_BITS(8), 0);
    unsigned captured_vl = 0U;

    asm volatile(
        ".option push\n\t"
        ".option norvc\n\t"
        "vsetvl t0, %[vl], %[vtype]\n\t"
        "vle8ff.v v8, (%[fof_src_addr])\n\t"
        "csrr %[out_vl], vl\n\t"
        "vse8.v v8, (%[fof_dst_addr])\n\t"
        ".insn i 0x0f, 0x2, x0, %[zero_addr], 4\n\t"
        "sd %[marker_update], 0(%[marker_addr])\n\t"
        "fence rw, rw\n\t"
        ".option pop\n\t"
        : [out_vl] "=&r"(captured_vl)
        : [fof_src_addr] "r"(fof_src_vaddr),
          [fof_dst_addr] "r"(fof_dst_vaddr),
          [zero_addr] "r"(zero_vaddr),
          [marker_addr] "r"(marker_vaddr),
          [marker_update] "r"(marker_update),
          [vl] "r"((int)AI_MEMBLOCK_CBOZERO_VFOF_BYTES),
          [vtype] "r"(vtype)
        : "t0", "memory");

    *observed_vl = captured_vl;
}

bool ai_micro_hs_vfof_partial_stop_then_cbozero_no_stale_vl_payload_or_zero_owner_corner()
{
    TEST_START();

    const enum test_page low_page = SWITCH1;
    const enum test_page high_page = SWITCH2;
    const enum test_page aux_page = VSRWX_GRWX;
    const uintptr_t low_tail_vaddr =
        hs_page_base(low_page) + PAGE_SIZE -
        AI_MEMBLOCK_CBOZERO_VFOF_PARTIAL_BYTES;
    const uintptr_t zero_vaddr = low_tail_vaddr;
    const uintptr_t low_tail_paddr =
        phys_page_base(low_page) + PAGE_SIZE -
        AI_MEMBLOCK_CBOZERO_VFOF_PARTIAL_BYTES;
    const uintptr_t zero_block_paddr =
        phys_page_base(low_page) + PAGE_SIZE -
        AI_MEMBLOCK_CBOZERO_VFOF_BLOCK_WORDS * sizeof(uint64_t);
    const uintptr_t zero_block_before_vaddr =
        hs_page_base(low_page) + PAGE_SIZE -
        AI_MEMBLOCK_CBOZERO_VFOF_BLOCK_WORDS * sizeof(uint64_t) -
        sizeof(uint64_t);
    const uintptr_t zero_block_before_paddr =
        phys_page_base(low_page) + PAGE_SIZE -
        AI_MEMBLOCK_CBOZERO_VFOF_BLOCK_WORDS * sizeof(uint64_t) -
        sizeof(uint64_t);
    const uintptr_t high_head_paddr = phys_page_base(high_page);
    const uintptr_t fof_dst_vaddr = hs_page_base(aux_page) + 0xf40U;
    const uintptr_t fof_dst_paddr = phys_page_base(aux_page) + 0xf40U;
    const uintptr_t fof_dst_before_vaddr =
        fof_dst_vaddr - sizeof(uint64_t);
    const uintptr_t fof_dst_after_vaddr =
        fof_dst_vaddr + AI_MEMBLOCK_CBOZERO_VFOF_BYTES;
    const uintptr_t fof_dst_before_paddr =
        fof_dst_paddr - sizeof(uint64_t);
    const uintptr_t fof_dst_after_paddr =
        fof_dst_paddr + AI_MEMBLOCK_CBOZERO_VFOF_BYTES;
    const uintptr_t marker_vaddr = hs_page_base(aux_page) + 0xf80U;
    const uintptr_t marker_paddr = phys_page_base(aux_page) + 0xf80U;
    const uintptr_t marker_guard_vaddr = marker_vaddr + sizeof(uint64_t);
    const uintptr_t marker_guard_paddr = marker_paddr + sizeof(uint64_t);
    static const uint8_t low_tail_patterns[][AI_MEMBLOCK_CBOZERO_VFOF_PARTIAL_BYTES] = {
        { 0x11, 0x22, 0x33, 0x44, 0x55, 0x66, 0x77, 0x88 },
        { 0xa1, 0xb2, 0xc3, 0xd4, 0xe5, 0xf6, 0x17, 0x28 },
        { 0x5b, 0x4c, 0x3d, 0x2e, 0x1f, 0x0a, 0x7e, 0x6d },
    };
    static const uint8_t high_head_patterns[][AI_MEMBLOCK_CBOZERO_VFOF_PARTIAL_BYTES] = {
        { 0x89, 0x9a, 0xab, 0xbc, 0xcd, 0xde, 0xef, 0xf1 },
        { 0x18, 0x29, 0x3a, 0x4b, 0x5c, 0x6d, 0x7e, 0x8f },
        { 0xf1, 0xe2, 0xd3, 0xc4, 0xb5, 0xa6, 0x97, 0x88 },
    };
    const uint64_t low_tail_words[] = {
        0x8877665544332211ULL,
        0x2817f6e5d4c3b2a1ULL,
        0x6d7e0a1f2e3d4c5bULL,
    };
    const uint64_t zero_block_seeds[] = {
        0x1122334455667788ULL,
        0x99aabbccddeeff00ULL,
        0x13579bdf2468ace0ULL,
    };
    const uint64_t zero_block_before_guards[] = {
        0x3141592653589793ULL,
        0x2718281828459045ULL,
        0x1123581321345589ULL,
    };
    const uint64_t fof_dst_fills[] = {
        0xd3d3d3d3d3d3d3d3ULL,
        0x2c2c2c2c2c2c2c2cULL,
        0xb44bb44bc22dc22dULL,
    };
    const uint64_t fof_dst_before_guards[] = {
        0x55aa33cc7799bbddULL,
        0x1234fedc5678ba90ULL,
        0x0badc0de55aa33ccULL,
    };
    const uint64_t fof_dst_after_guards[] = {
        0xa1b2c3d4e5f60718ULL,
        0x1029384756a5b4c3ULL,
        0x0f1e2d3c4b5a6978ULL,
    };
    const uint64_t marker_seeds[] = {
        0xa7a6a5a4a3a2a1a0ULL,
        0xb7b6b5b4b3b2b1b0ULL,
        0xc7c6c5c4c3c2c1c0ULL,
    };
    const uint64_t marker_updates[] = {
        0x0123456789abcdefULL,
        0xfedcba9876543210ULL,
        0x2468ace013579bdfULL,
    };
    const uint64_t marker_guards[] = {
        0xfeedfacecafebeefULL,
        0xdeadc0def00dbabeULL,
        0xfaceb00c13579bdfULL,
    };

    ai_memblock_cbozero_vfof_prepare_hs_env();

    for (size_t round = 0;
         round < AI_MEMBLOCK_CBOZERO_VFOF_ARRAY_SIZE(zero_block_seeds);
         ++round) {
        unsigned observed_vl = 0U;

        goto_priv(PRIV_M);
        ai_memblock_cbozero_vfof_seed_block(zero_block_paddr,
            zero_block_seeds[round]);
        ai_memblock_cbozero_vfof_write_bytes(low_tail_paddr,
            low_tail_patterns[round],
            AI_MEMBLOCK_CBOZERO_VFOF_PARTIAL_BYTES);
        ai_memblock_cbozero_vfof_write_bytes(high_head_paddr,
            high_head_patterns[round],
            AI_MEMBLOCK_CBOZERO_VFOF_PARTIAL_BYTES);
        write64(zero_block_before_paddr, zero_block_before_guards[round]);
        write64(fof_dst_paddr, fof_dst_fills[round]);
        write64(fof_dst_paddr + sizeof(uint64_t),
            ~fof_dst_fills[round]);
        write64(fof_dst_before_paddr, fof_dst_before_guards[round]);
        write64(fof_dst_after_paddr, fof_dst_after_guards[round]);
        write64(marker_paddr, marker_seeds[round]);
        write64(marker_guard_paddr, marker_guards[round]);

        goto_priv(PRIV_HS);
        hspt_init();
        ai_memblock_cbozero_vfof_set_hs_leaf(low_page,
            PTE_V | PTE_AD | PTE_RWX);
        ai_memblock_cbozero_vfof_set_hs_leaf(high_page,
            PTE_V | PTE_AD | PTE_X);
        ai_memblock_cbozero_vfof_set_hs_leaf(aux_page,
            PTE_V | PTE_AD | PTE_RWX);
        sfence_vma();

        TEST_SETUP_EXCEPT();
        const uint64_t warmed_low_tail = ld(low_tail_vaddr);
        const uint64_t warmed_block_before = ld(zero_block_before_vaddr);
        const uint64_t warmed_marker = ld(marker_vaddr);
        AI_ASSERT_NO_EXCEPT_AND("before the partial-stop VFOF plus CBO.ZERO window starts, the low-page tail and marker are readable while the next page is execute-only",
            warmed_low_tail == low_tail_words[round] &&
            warmed_block_before == zero_block_before_guards[round] &&
            warmed_marker == marker_seeds[round]
        );

        TEST_SETUP_EXCEPT();
        ai_memblock_partial_vfof_capture_then_cbozero_marker(
            low_tail_vaddr, fof_dst_vaddr, zero_vaddr, marker_vaddr,
            marker_updates[round], &observed_vl);
        const uint64_t observed_low_tail = ld(low_tail_vaddr);
        const uint64_t observed_fof_dst0 = ld(fof_dst_vaddr);
        const uint64_t observed_fof_dst1 =
            ld(fof_dst_vaddr + sizeof(uint64_t));
        const uint64_t observed_marker = ld(marker_vaddr);
        AI_ASSERT_NO_EXCEPT_AND("cross-page partial-stop VFOF immediately followed by CBO.ZERO keeps the shortened payload visible in destination, zeroes the low-page block, and releases marker progress",
            observed_vl == AI_MEMBLOCK_CBOZERO_VFOF_PARTIAL_BYTES &&
            observed_low_tail == 0ULL &&
            observed_fof_dst0 == low_tail_words[round] &&
            observed_fof_dst1 == ~fof_dst_fills[round] &&
            observed_marker == marker_updates[round]
        );

        TEST_SETUP_EXCEPT();
        AI_ASSERT_NO_EXCEPT_AND("the partial-stop VFOF plus CBO.ZERO window preserves adjacent low-page, destination, and marker guards",
            ld(zero_block_before_vaddr) == zero_block_before_guards[round] &&
            ld(fof_dst_before_vaddr) == fof_dst_before_guards[round] &&
            ld(fof_dst_after_vaddr) == fof_dst_after_guards[round] &&
            ld(marker_guard_vaddr) == marker_guards[round]
        );

        goto_priv(PRIV_M);
        TEST_ASSERT("backing memory after the partial-stop VFOF plus CBO.ZERO window keeps only the shortened low-page payload in destination, zeroes the low-page block, leaves high page untouched, updates marker, and preserves guards",
            ai_memblock_cbozero_vfof_range_all_zero(zero_block_paddr,
                AI_MEMBLOCK_CBOZERO_VFOF_BLOCK_WORDS * sizeof(uint64_t)) &&
            ai_memblock_cbozero_vfof_bytes_match(high_head_paddr,
                high_head_patterns[round],
                AI_MEMBLOCK_CBOZERO_VFOF_PARTIAL_BYTES) &&
            read64(zero_block_before_paddr) ==
                zero_block_before_guards[round] &&
            ai_memblock_cbozero_vfof_bytes_match(fof_dst_paddr,
                low_tail_patterns[round],
                AI_MEMBLOCK_CBOZERO_VFOF_PARTIAL_BYTES) &&
            read64(fof_dst_paddr + sizeof(uint64_t)) ==
                ~fof_dst_fills[round] &&
            read64(fof_dst_before_paddr) == fof_dst_before_guards[round] &&
            read64(fof_dst_after_paddr) == fof_dst_after_guards[round] &&
            read64(marker_paddr) == marker_updates[round] &&
            read64(marker_guard_paddr) == marker_guards[round]
        );
    }

    ai_memblock_cbozero_vfof_reset_hs_env();

    TEST_END();
}

static inline void ai_memblock_cbozero_then_dual_vfof_capture_marker(
    uintptr_t zero_vaddr,
    uintptr_t old_src_vaddr,
    uintptr_t young_src_vaddr,
    uintptr_t old_dst_vaddr,
    uintptr_t young_dst_vaddr,
    uintptr_t marker_vaddr,
    uint64_t marker_update,
    unsigned *old_vl,
    unsigned *young_vl)
{
    const int vtype = VTYPE(VSEW_FROM_BITS(8), 0);
    unsigned captured_old_vl = 0U;
    unsigned captured_young_vl = 0U;

    asm volatile(
        ".option push\n\t"
        ".option norvc\n\t"
        ".insn i 0x0f, 0x2, x0, %[zero_addr], 4\n\t"
        "vsetvl t0, %[vl], %[vtype]\n\t"
        "vle8ff.v v8, (%[old_src_addr])\n\t"
        "csrr %[old_vl_out], vl\n\t"
        "vsetvl t0, %[vl], %[vtype]\n\t"
        "vle8ff.v v16, (%[young_src_addr])\n\t"
        "csrr %[young_vl_out], vl\n\t"
        "vse8.v v8, (%[old_dst_addr])\n\t"
        "vse8.v v16, (%[young_dst_addr])\n\t"
        "sd %[marker_update], 0(%[marker_addr])\n\t"
        "fence rw, rw\n\t"
        ".option pop\n\t"
        : [old_vl_out] "=&r"(captured_old_vl),
          [young_vl_out] "=&r"(captured_young_vl)
        : [zero_addr] "r"(zero_vaddr),
          [old_src_addr] "r"(old_src_vaddr),
          [young_src_addr] "r"(young_src_vaddr),
          [old_dst_addr] "r"(old_dst_vaddr),
          [young_dst_addr] "r"(young_dst_vaddr),
          [marker_addr] "r"(marker_vaddr),
          [marker_update] "r"(marker_update),
          [vl] "r"((int)AI_MEMBLOCK_CBOZERO_VFOF_BYTES),
          [vtype] "r"(vtype)
        : "t0", "memory");

    *old_vl = captured_old_vl;
    *young_vl = captured_young_vl;
}

bool ai_micro_hs_cbozero_then_back_to_back_vfof_no_stale_zero_or_younger_payload_corner()
{
    TEST_START();

    const enum test_page old_page = SWITCH1;
    const enum test_page young_page = SWITCH2;
    const enum test_page aux_page = VSRWX_GRWX;
    const uintptr_t old_src_base_vaddr = hs_page_base(old_page) + 0xe00U;
    const uintptr_t old_src_base_paddr = phys_page_base(old_page) + 0xe00U;
    const uintptr_t zero_subblock_vaddr =
        old_src_base_vaddr + 4U * sizeof(uint64_t);
    const uintptr_t old_src_tail_vaddr =
        old_src_base_vaddr +
        (AI_MEMBLOCK_CBOZERO_VFOF_BLOCK_WORDS - 1U) * sizeof(uint64_t);
    const uintptr_t old_src_before_vaddr =
        old_src_base_vaddr - sizeof(uint64_t);
    const uintptr_t old_src_after_vaddr =
        old_src_base_vaddr + AI_MEMBLOCK_CBOZERO_VFOF_BLOCK_WORDS *
        sizeof(uint64_t);
    const uintptr_t old_src_before_paddr =
        old_src_base_paddr - sizeof(uint64_t);
    const uintptr_t old_src_after_paddr =
        old_src_base_paddr + AI_MEMBLOCK_CBOZERO_VFOF_BLOCK_WORDS *
        sizeof(uint64_t);
    const uintptr_t young_src_vaddr = hs_page_base(young_page) + 0xe00U;
    const uintptr_t young_src_paddr = phys_page_base(young_page) + 0xe00U;
    const uintptr_t young_src_guard_vaddr =
        young_src_vaddr + AI_MEMBLOCK_CBOZERO_VFOF_BYTES;
    const uintptr_t young_src_guard_paddr =
        young_src_paddr + AI_MEMBLOCK_CBOZERO_VFOF_BYTES;
    const uintptr_t old_dst_vaddr = hs_page_base(aux_page) + 0xe00U;
    const uintptr_t old_dst_paddr = phys_page_base(aux_page) + 0xe00U;
    const uintptr_t old_dst_before_vaddr =
        old_dst_vaddr - sizeof(uint64_t);
    const uintptr_t old_dst_after_vaddr =
        old_dst_vaddr + AI_MEMBLOCK_CBOZERO_VFOF_BYTES;
    const uintptr_t old_dst_before_paddr =
        old_dst_paddr - sizeof(uint64_t);
    const uintptr_t old_dst_after_paddr =
        old_dst_paddr + AI_MEMBLOCK_CBOZERO_VFOF_BYTES;
    const uintptr_t young_dst_vaddr = hs_page_base(aux_page) + 0xe40U;
    const uintptr_t young_dst_paddr = phys_page_base(aux_page) + 0xe40U;
    const uintptr_t young_dst_before_vaddr =
        young_dst_vaddr - sizeof(uint64_t);
    const uintptr_t young_dst_after_vaddr =
        young_dst_vaddr + AI_MEMBLOCK_CBOZERO_VFOF_BYTES;
    const uintptr_t young_dst_before_paddr =
        young_dst_paddr - sizeof(uint64_t);
    const uintptr_t young_dst_after_paddr =
        young_dst_paddr + AI_MEMBLOCK_CBOZERO_VFOF_BYTES;
    const uintptr_t marker_vaddr = hs_page_base(aux_page) + 0xe80U;
    const uintptr_t marker_paddr = phys_page_base(aux_page) + 0xe80U;
    const uintptr_t marker_guard_vaddr = marker_vaddr + sizeof(uint64_t);
    const uintptr_t marker_guard_paddr = marker_paddr + sizeof(uint64_t);
    const uint64_t old_src_seeds[] = {
        0x1021324354657687ULL,
        0x89abcdef01234567ULL,
        0xf0e1d2c3b4a59687ULL,
    };
    const uint64_t young_word0_values[] = {
        0x0011223344556677ULL,
        0x8899aabbccddeeffULL,
        0x13579bdf2468ace0ULL,
    };
    const uint64_t young_word1_values[] = {
        0x7766554433221100ULL,
        0xffeeddccbbaa9988ULL,
        0x0eca8642fdb97531ULL,
    };
    const uint64_t old_src_before_guards[] = {
        0x3141592653589793ULL,
        0x2718281828459045ULL,
        0x1123581321345589ULL,
    };
    const uint64_t old_src_after_guards[] = {
        0xfeedfacecafebeefULL,
        0xdeadc0def00dbabeULL,
        0xfaceb00c13579bdfULL,
    };
    const uint64_t young_src_guards[] = {
        0x55aa33cc7799bbddULL,
        0x1234fedc5678ba90ULL,
        0x0badc0de55aa33ccULL,
    };
    const uint64_t old_dst_fills[] = {
        0xd3d3d3d3d3d3d3d3ULL,
        0x2c2c2c2c2c2c2c2cULL,
        0xb44bb44bc22dc22dULL,
    };
    const uint64_t young_dst_fills[] = {
        0xc3c3c3c3c3c3c3c3ULL,
        0x3c3c3c3c3c3c3c3cULL,
        0xa55aa55ac33cc33cULL,
    };
    const uint64_t old_dst_before_guards[] = {
        0xa1b2c3d4e5f60718ULL,
        0x1029384756a5b4c3ULL,
        0x0f1e2d3c4b5a6978ULL,
    };
    const uint64_t old_dst_after_guards[] = {
        0x13579bdf2468ace0ULL,
        0x02468ace13579bdfULL,
        0xfdb97531eca86420ULL,
    };
    const uint64_t young_dst_before_guards[] = {
        0x1123581321345589ULL,
        0x3141592653589793ULL,
        0x2718281828459045ULL,
    };
    const uint64_t young_dst_after_guards[] = {
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
        0x0123456789abcdefULL,
        0xfedcba9876543210ULL,
        0x2468ace013579bdfULL,
    };
    const uint64_t marker_guards[] = {
        0x8877665544332211ULL,
        0x7766554433221100ULL,
        0x66554433221100ffULL,
    };

    ai_memblock_cbozero_vfof_prepare_hs_env();

    for (size_t round = 0;
         round < AI_MEMBLOCK_CBOZERO_VFOF_ARRAY_SIZE(old_src_seeds);
         ++round) {
        const uint64_t expected_old_tail =
            old_src_seeds[round] +
            (AI_MEMBLOCK_CBOZERO_VFOF_BLOCK_WORDS - 1U) *
            0x1111111111111111ULL;
        unsigned observed_old_vl = 0U;
        unsigned observed_young_vl = 0U;

        goto_priv(PRIV_M);
        ai_memblock_cbozero_vfof_seed_block(old_src_base_paddr,
            old_src_seeds[round]);
        write64(old_src_before_paddr, old_src_before_guards[round]);
        write64(old_src_after_paddr, old_src_after_guards[round]);
        write64(young_src_paddr, young_word0_values[round]);
        write64(young_src_paddr + sizeof(uint64_t),
            young_word1_values[round]);
        write64(young_src_guard_paddr, young_src_guards[round]);
        write64(old_dst_paddr, old_dst_fills[round]);
        write64(old_dst_paddr + sizeof(uint64_t),
            ~old_dst_fills[round]);
        write64(old_dst_before_paddr, old_dst_before_guards[round]);
        write64(old_dst_after_paddr, old_dst_after_guards[round]);
        write64(young_dst_paddr, young_dst_fills[round]);
        write64(young_dst_paddr + sizeof(uint64_t),
            ~young_dst_fills[round]);
        write64(young_dst_before_paddr, young_dst_before_guards[round]);
        write64(young_dst_after_paddr, young_dst_after_guards[round]);
        write64(marker_paddr, marker_seeds[round]);
        write64(marker_guard_paddr, marker_guards[round]);

        goto_priv(PRIV_HS);
        TEST_SETUP_EXCEPT();
        const uint64_t warmed_old0 = ld(old_src_base_vaddr);
        const uint64_t warmed_old_tail = ld(old_src_tail_vaddr);
        const uint64_t warmed_young0 = ld(young_src_vaddr);
        const uint64_t warmed_young1 =
            ld(young_src_vaddr + sizeof(uint64_t));
        const uint64_t warmed_marker = ld(marker_vaddr);
        AI_ASSERT_NO_EXCEPT_AND("before the CBO.ZERO plus back-to-back VFOF window starts, the older zero target, younger source, and marker are readable with seeded values",
            warmed_old0 == old_src_seeds[round] &&
            warmed_old_tail == expected_old_tail &&
            warmed_young0 == young_word0_values[round] &&
            warmed_young1 == young_word1_values[round] &&
            warmed_marker == marker_seeds[round]
        );

        TEST_SETUP_EXCEPT();
        ai_memblock_cbozero_then_dual_vfof_capture_marker(
            zero_subblock_vaddr, old_src_base_vaddr, young_src_vaddr,
            old_dst_vaddr, young_dst_vaddr, marker_vaddr,
            marker_updates[round], &observed_old_vl, &observed_young_vl);
        const uint64_t observed_old_src0 = ld(old_src_base_vaddr);
        const uint64_t observed_old_tail = ld(old_src_tail_vaddr);
        const uint64_t observed_old_dst0 = ld(old_dst_vaddr);
        const uint64_t observed_old_dst1 =
            ld(old_dst_vaddr + sizeof(uint64_t));
        const uint64_t observed_young_dst0 = ld(young_dst_vaddr);
        const uint64_t observed_young_dst1 =
            ld(young_dst_vaddr + sizeof(uint64_t));
        const uint64_t observed_marker = ld(marker_vaddr);
        AI_ASSERT_NO_EXCEPT_AND("CBO.ZERO immediately followed by two VFOF instructions keeps older zero payload and younger source payload separate while both vl values stay full-width",
            observed_old_vl == AI_MEMBLOCK_CBOZERO_VFOF_BYTES &&
            observed_young_vl == AI_MEMBLOCK_CBOZERO_VFOF_BYTES &&
            observed_old_src0 == 0ULL &&
            observed_old_tail == 0ULL &&
            observed_old_dst0 == 0ULL &&
            observed_old_dst1 == 0ULL &&
            observed_young_dst0 == young_word0_values[round] &&
            observed_young_dst1 == young_word1_values[round] &&
            observed_marker == marker_updates[round]
        );

        TEST_SETUP_EXCEPT();
        AI_ASSERT_NO_EXCEPT_AND("the CBO.ZERO plus back-to-back VFOF window preserves source, destination, and marker guards",
            ld(old_src_before_vaddr) == old_src_before_guards[round] &&
            ld(old_src_after_vaddr) == old_src_after_guards[round] &&
            ld(young_src_guard_vaddr) == young_src_guards[round] &&
            ld(old_dst_before_vaddr) == old_dst_before_guards[round] &&
            ld(old_dst_after_vaddr) == old_dst_after_guards[round] &&
            ld(young_dst_before_vaddr) == young_dst_before_guards[round] &&
            ld(young_dst_after_vaddr) == young_dst_after_guards[round] &&
            ld(marker_guard_vaddr) == marker_guards[round]
        );

        goto_priv(PRIV_M);
        TEST_ASSERT("backing memory after the CBO.ZERO plus back-to-back VFOF window zeroes only the older source block, keeps younger source and destination payloads distinct, updates marker, and preserves guards",
            ai_memblock_cbozero_vfof_block_all_zero(old_src_base_paddr) &&
            read64(old_src_before_paddr) == old_src_before_guards[round] &&
            read64(old_src_after_paddr) == old_src_after_guards[round] &&
            read64(young_src_paddr) == young_word0_values[round] &&
            read64(young_src_paddr + sizeof(uint64_t)) ==
                young_word1_values[round] &&
            read64(young_src_guard_paddr) == young_src_guards[round] &&
            read64(old_dst_paddr) == 0ULL &&
            read64(old_dst_paddr + sizeof(uint64_t)) == 0ULL &&
            read64(old_dst_before_paddr) == old_dst_before_guards[round] &&
            read64(old_dst_after_paddr) == old_dst_after_guards[round] &&
            read64(young_dst_paddr) == young_word0_values[round] &&
            read64(young_dst_paddr + sizeof(uint64_t)) ==
                young_word1_values[round] &&
            read64(young_dst_before_paddr) ==
                young_dst_before_guards[round] &&
            read64(young_dst_after_paddr) == young_dst_after_guards[round] &&
            read64(marker_paddr) == marker_updates[round] &&
            read64(marker_guard_paddr) == marker_guards[round]
        );
    }

    ai_memblock_cbozero_vfof_reset_hs_env();

    TEST_END();
}
