#include <rvh_test.h>
#include <page_tables.h>
#include <instructions.h>
#include <stdint.h>

#define AI_ASSERT_NO_EXCEPT_AND(message, cond) \
    TEST_ASSERT(message, excpt.triggered == false && (cond))
#define AI_MEMBLOCK_CBOINVAL_INACTIVE_ARRAY_SIZE(array) \
    (sizeof(array) / sizeof((array)[0]))
#define AI_MEMBLOCK_CBOINVAL_INACTIVE_BLOCK_WORDS 8U
#define AI_MEMBLOCK_CBOINVAL_INACTIVE_BYTES 16U

static inline void ai_memblock_cboinval_inactive_prepare_hs_env(void)
{
    goto_priv(PRIV_M);
    CSRS(CSR_MENVCFG, MENVCFG_CBIE01);
    hspt_init();
    sfence_vma();
}

static inline void ai_memblock_cboinval_inactive_reset_hs_env(void)
{
    goto_priv(PRIV_M);
    CSRC(CSR_MENVCFG, MENVCFG_CBIE01);
    hspt_init();
    sfence_vma();
}

static inline void ai_memblock_cboinval_inactive_seed_block(uintptr_t paddr,
    uint64_t seed)
{
    for (size_t i = 0; i < AI_MEMBLOCK_CBOINVAL_INACTIVE_BLOCK_WORDS; ++i) {
        write64(paddr + i * sizeof(uint64_t),
            seed + i * 0x1111111111111111ULL);
    }
}

static inline void ai_memblock_cboinval_inactive_write_bytes(uintptr_t paddr,
    const uint8_t *bytes,
    unsigned len)
{
    for (unsigned i = 0; i < len; ++i) {
        write8(paddr + i, bytes[i]);
    }
}

static inline bool ai_memblock_cboinval_inactive_bytes_match(uintptr_t paddr,
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

static inline uint64_t ai_memblock_cboinval_inactive_bytes_to_u64(
    const uint8_t *bytes,
    unsigned first)
{
    uint64_t value = 0;

    for (unsigned i = 0; i < sizeof(uint64_t); ++i) {
        value |= ((uint64_t)bytes[first + i]) << (8U * i);
    }

    return value;
}

static inline bool ai_memblock_cboinval_inactive_block_tail_matches_seed(
    uintptr_t paddr,
    uint64_t seed,
    size_t first_tail_word)
{
    for (size_t i = first_tail_word;
         i < AI_MEMBLOCK_CBOINVAL_INACTIVE_BLOCK_WORDS;
         ++i) {
        const uint64_t expected = seed + i * 0x1111111111111111ULL;

        if (read64(paddr + i * sizeof(uint64_t)) != expected) {
            return false;
        }
    }

    return true;
}

static inline bool ai_memblock_cboinval_partial_masked_store_matches(
    uintptr_t paddr,
    const uint8_t *active_source,
    const uint8_t *inactive_source,
    const uint8_t *mask_bytes,
    unsigned len)
{
    for (unsigned i = 0; i < len; ++i) {
        const bool active = ((mask_bytes[i / 8U] >> (i % 8U)) & 1U) != 0U;
        const uint8_t expected = active ? active_source[i] : inactive_source[i];

        if (read8(paddr + i) != expected) {
            return false;
        }
    }

    return true;
}

static inline void ai_memblock_cboinval_then_masked_vstore_marker(
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
        "vmv.v.i v0, 0\n\t"
        "vse8.v v8, (%[dst_addr]), v0.t\n\t"
        "sd %[marker_update], 0(%[marker_addr])\n\t"
        "fence rw, rw\n\t"
        ".option pop\n\t"
        :
        : [inval_addr] "r"(inval_vaddr),
          [src_addr] "r"(src_vaddr),
          [dst_addr] "r"(dst_vaddr),
          [marker_addr] "r"(marker_vaddr),
          [marker_update] "r"(marker_update),
          [vl] "r"((int)AI_MEMBLOCK_CBOINVAL_INACTIVE_BYTES),
          [vtype] "r"(vtype)
        : "t0", "memory");
}

static inline void ai_memblock_cboinval_then_masked_vload_capture_marker(
    uintptr_t inval_vaddr,
    uintptr_t seed_src_vaddr,
    uintptr_t masked_src_vaddr,
    uintptr_t capture_dst_vaddr,
    uintptr_t marker_vaddr,
    uint64_t marker_update)
{
    const int vtype = VTYPE(VSEW_FROM_BITS(8), 0);

    asm volatile(
        ".option push\n\t"
        ".option norvc\n\t"
        "vsetvl t0, %[vl], %[vtype]\n\t"
        "vle8.v v8, (%[seed_src_addr])\n\t"
        ".insn i 0x0f, 0x2, x0, %[inval_addr], 0\n\t"
        "vmv.v.i v0, 0\n\t"
        "vle8.v v8, (%[masked_src_addr]), v0.t\n\t"
        "vse8.v v8, (%[capture_dst_addr])\n\t"
        "sd %[marker_update], 0(%[marker_addr])\n\t"
        "fence rw, rw\n\t"
        ".option pop\n\t"
        :
        : [inval_addr] "r"(inval_vaddr),
          [seed_src_addr] "r"(seed_src_vaddr),
          [masked_src_addr] "r"(masked_src_vaddr),
          [capture_dst_addr] "r"(capture_dst_vaddr),
          [marker_addr] "r"(marker_vaddr),
          [marker_update] "r"(marker_update),
          [vl] "r"((int)AI_MEMBLOCK_CBOINVAL_INACTIVE_BYTES),
          [vtype] "r"(vtype)
        : "t0", "memory");
}

bool ai_micro_hs_cboinval_then_masked_vector_store_all_inactive_no_side_effect_or_marker_drop_corner()
{
    TEST_START();

    const enum test_page src_page = SWITCH1;
    const enum test_page dst_page = SWITCH2;
    const enum test_page marker_page = VSRWX_GRWX;
    const uintptr_t src_vaddr = hs_page_base(src_page) + 0x880U;
    const uintptr_t src_paddr = phys_page_base(src_page) + 0x880U;
    const uintptr_t src_before_vaddr = src_vaddr - sizeof(uint64_t);
    const uintptr_t src_after_vaddr =
        src_vaddr + AI_MEMBLOCK_CBOINVAL_INACTIVE_BYTES;
    const uintptr_t src_before_paddr = src_paddr - sizeof(uint64_t);
    const uintptr_t src_after_paddr =
        src_paddr + AI_MEMBLOCK_CBOINVAL_INACTIVE_BYTES;
    const uintptr_t dst_base_vaddr = hs_page_base(dst_page) + 0x880U;
    const uintptr_t dst_base_paddr = phys_page_base(dst_page) + 0x880U;
    const uintptr_t inval_vaddr =
        dst_base_vaddr + 4U * sizeof(uint64_t);
    const uintptr_t dst_tail_vaddr =
        dst_base_vaddr +
        (AI_MEMBLOCK_CBOINVAL_INACTIVE_BLOCK_WORDS - 1U) *
        sizeof(uint64_t);
    const uintptr_t dst_before_vaddr =
        dst_base_vaddr - sizeof(uint64_t);
    const uintptr_t dst_after_vaddr =
        dst_base_vaddr + AI_MEMBLOCK_CBOINVAL_INACTIVE_BLOCK_WORDS *
        sizeof(uint64_t);
    const uintptr_t dst_before_paddr =
        dst_base_paddr - sizeof(uint64_t);
    const uintptr_t dst_after_paddr =
        dst_base_paddr + AI_MEMBLOCK_CBOINVAL_INACTIVE_BLOCK_WORDS *
        sizeof(uint64_t);
    const uintptr_t marker_vaddr = hs_page_base(marker_page) + 0x980U;
    const uintptr_t marker_paddr = phys_page_base(marker_page) + 0x980U;
    const uintptr_t marker_guard_vaddr = marker_vaddr + sizeof(uint64_t);
    const uintptr_t marker_guard_paddr = marker_paddr + sizeof(uint64_t);
    static const uint8_t src_patterns[][AI_MEMBLOCK_CBOINVAL_INACTIVE_BYTES] = {
        { 0xa0, 0xb1, 0xc2, 0xd3, 0xe4, 0xf5, 0x16, 0x27,
          0x38, 0x49, 0x5a, 0x6b, 0x7c, 0x8d, 0x9e, 0xaf },
        { 0x5f, 0x4e, 0x3d, 0x2c, 0x1b, 0x0a, 0xf9, 0xe8,
          0xd7, 0xc6, 0xb5, 0xa4, 0x93, 0x82, 0x71, 0x60 },
        { 0x12, 0x34, 0x56, 0x78, 0x9a, 0xbc, 0xde, 0xf0,
          0x0f, 0xed, 0xcb, 0xa9, 0x87, 0x65, 0x43, 0x21 },
    };
    static const uint8_t dst_post_inval_patterns[]
        [AI_MEMBLOCK_CBOINVAL_INACTIVE_BYTES] = {
        { 0x6c, 0x5b, 0x4a, 0x39, 0x28, 0x17, 0x06, 0xf5,
          0xe4, 0xd3, 0xc2, 0xb1, 0xa0, 0x9f, 0x8e, 0x7d },
        { 0x80, 0x71, 0x62, 0x53, 0x44, 0x35, 0x26, 0x17,
          0x08, 0xf9, 0xea, 0xdb, 0xcc, 0xbd, 0xae, 0x9f },
        { 0x03, 0x14, 0x25, 0x36, 0x47, 0x58, 0x69, 0x7a,
          0x8b, 0x9c, 0xad, 0xbe, 0xcf, 0xd0, 0xe1, 0xf2 },
    };
    const uint64_t dst_old_seeds[] = {
        0x0f1e2d3c4b5a6978ULL,
        0x8877665544332211ULL,
        0x13579bdf2468ace0ULL,
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
        0x44aa55bb66cc77ddULL,
        0x55bb66cc77dd88eeULL,
        0x66cc77dd88ee99ffULL,
    };

    ai_memblock_cboinval_inactive_prepare_hs_env();

    for (size_t round = 0;
         round < AI_MEMBLOCK_CBOINVAL_INACTIVE_ARRAY_SIZE(dst_old_seeds);
         ++round) {
        const uint64_t dst_old_word1 =
            dst_old_seeds[round] + 0x1111111111111111ULL;
        const uint64_t expected_tail =
            dst_old_seeds[round] +
            (AI_MEMBLOCK_CBOINVAL_INACTIVE_BLOCK_WORDS - 1U) *
            0x1111111111111111ULL;
        const uint64_t expected_dst0 =
            ai_memblock_cboinval_inactive_bytes_to_u64(
                dst_post_inval_patterns[round], 0U);
        const uint64_t expected_dst1 =
            ai_memblock_cboinval_inactive_bytes_to_u64(
                dst_post_inval_patterns[round], sizeof(uint64_t));

        goto_priv(PRIV_M);
        ai_memblock_cboinval_inactive_write_bytes(src_paddr,
            src_patterns[round], AI_MEMBLOCK_CBOINVAL_INACTIVE_BYTES);
        write64(src_before_paddr, src_before_guards[round]);
        write64(src_after_paddr, src_after_guards[round]);
        ai_memblock_cboinval_inactive_seed_block(dst_base_paddr,
            dst_old_seeds[round]);
        write64(dst_before_paddr, dst_before_guards[round]);
        write64(dst_after_paddr, dst_after_guards[round]);
        write64(marker_paddr, marker_seeds[round]);
        write64(marker_guard_paddr, marker_guards[round]);

        goto_priv(PRIV_HS);
        TEST_SETUP_EXCEPT();
        const uint64_t warmed_src0 = ld(src_vaddr);
        const uint64_t warmed_src1 = ld(src_vaddr + sizeof(uint64_t));
        const uint64_t warmed_dst0 = ld(dst_base_vaddr);
        const uint64_t warmed_dst1 =
            ld(dst_base_vaddr + sizeof(uint64_t));
        const uint64_t warmed_dst_tail = ld(dst_tail_vaddr);
        const uint64_t warmed_marker = ld(marker_vaddr);
        AI_ASSERT_NO_EXCEPT_AND("before the CBO.INVAL plus all-inactive masked vector-store owner-window starts, source, destination cacheline, and marker are readable with seeded values",
            warmed_src0 ==
                ai_memblock_cboinval_inactive_bytes_to_u64(
                    src_patterns[round], 0U) &&
            warmed_src1 ==
                ai_memblock_cboinval_inactive_bytes_to_u64(
                    src_patterns[round], sizeof(uint64_t)) &&
            warmed_dst0 == dst_old_seeds[round] &&
            warmed_dst1 == dst_old_word1 &&
            warmed_dst_tail == expected_tail &&
            warmed_marker == marker_seeds[round]
        );

        goto_priv(PRIV_M);
        ai_memblock_cboinval_inactive_write_bytes(dst_base_paddr,
            dst_post_inval_patterns[round],
            AI_MEMBLOCK_CBOINVAL_INACTIVE_BYTES);

        goto_priv(PRIV_HS);
        TEST_SETUP_EXCEPT();
        ai_memblock_cboinval_then_masked_vstore_marker(inval_vaddr,
            src_vaddr, dst_base_vaddr, marker_vaddr,
            marker_updates[round]);
        const uint64_t observed_dst0 = ld(dst_base_vaddr);
        const uint64_t observed_dst1 =
            ld(dst_base_vaddr + sizeof(uint64_t));
        const uint64_t observed_dst_tail = ld(dst_tail_vaddr);
        const uint64_t observed_marker = ld(marker_vaddr);
        AI_ASSERT_NO_EXCEPT_AND("CBO.INVAL immediately followed by all-inactive masked unit-stride vector store refills the post-inval destination image, makes no vector-store side effect, and releases marker progress",
            observed_dst0 == expected_dst0 &&
            observed_dst1 == expected_dst1 &&
            observed_dst_tail == expected_tail &&
            observed_marker == marker_updates[round]
        );

        TEST_SETUP_EXCEPT();
        AI_ASSERT_NO_EXCEPT_AND("the CBO.INVAL plus all-inactive masked vector-store owner-window does not corrupt source, destination, or marker guards",
            ld(src_before_vaddr) == src_before_guards[round] &&
            ld(src_after_vaddr) == src_after_guards[round] &&
            ld(dst_before_vaddr) == dst_before_guards[round] &&
            ld(dst_after_vaddr) == dst_after_guards[round] &&
            ld(marker_guard_vaddr) == marker_guards[round]
        );

        goto_priv(PRIV_M);
        TEST_ASSERT("backing memory after the CBO.INVAL plus all-inactive masked vector-store window keeps source payload, destination post-inval image, destination tail, marker, and guards intact",
            ai_memblock_cboinval_inactive_bytes_match(src_paddr,
                src_patterns[round],
                AI_MEMBLOCK_CBOINVAL_INACTIVE_BYTES) &&
            read64(src_before_paddr) == src_before_guards[round] &&
            read64(src_after_paddr) == src_after_guards[round] &&
            ai_memblock_cboinval_inactive_bytes_match(dst_base_paddr,
                dst_post_inval_patterns[round],
                AI_MEMBLOCK_CBOINVAL_INACTIVE_BYTES) &&
            ai_memblock_cboinval_inactive_block_tail_matches_seed(
                dst_base_paddr,
                dst_old_seeds[round],
                AI_MEMBLOCK_CBOINVAL_INACTIVE_BYTES / sizeof(uint64_t)) &&
            read64(dst_before_paddr) == dst_before_guards[round] &&
            read64(dst_after_paddr) == dst_after_guards[round] &&
            read64(marker_paddr) == marker_updates[round] &&
            read64(marker_guard_paddr) == marker_guards[round]
        );
    }

    ai_memblock_cboinval_inactive_reset_hs_env();

    TEST_END();
}

bool ai_micro_hs_cboinval_then_masked_vector_load_all_inactive_no_dest_clobber_or_marker_drop_corner()
{
    TEST_START();

    const enum test_page seed_page = SWITCH1;
    const enum test_page masked_page = SWITCH2;
    const enum test_page capture_page = VSRWX_GRWX;
    const enum test_page marker_page = VSRWX_GRWX;
    const uintptr_t seed_src_vaddr = hs_page_base(seed_page) + 0xa80U;
    const uintptr_t seed_src_paddr = phys_page_base(seed_page) + 0xa80U;
    const uintptr_t seed_guard_vaddr =
        seed_src_vaddr + AI_MEMBLOCK_CBOINVAL_INACTIVE_BYTES;
    const uintptr_t seed_guard_paddr =
        seed_src_paddr + AI_MEMBLOCK_CBOINVAL_INACTIVE_BYTES;
    const uintptr_t masked_base_vaddr = hs_page_base(masked_page) + 0xa80U;
    const uintptr_t masked_base_paddr = phys_page_base(masked_page) + 0xa80U;
    const uintptr_t inval_vaddr =
        masked_base_vaddr + 4U * sizeof(uint64_t);
    const uintptr_t masked_tail_vaddr =
        masked_base_vaddr +
        (AI_MEMBLOCK_CBOINVAL_INACTIVE_BLOCK_WORDS - 1U) *
        sizeof(uint64_t);
    const uintptr_t masked_before_vaddr =
        masked_base_vaddr - sizeof(uint64_t);
    const uintptr_t masked_after_vaddr =
        masked_base_vaddr + AI_MEMBLOCK_CBOINVAL_INACTIVE_BLOCK_WORDS *
        sizeof(uint64_t);
    const uintptr_t masked_before_paddr =
        masked_base_paddr - sizeof(uint64_t);
    const uintptr_t masked_after_paddr =
        masked_base_paddr + AI_MEMBLOCK_CBOINVAL_INACTIVE_BLOCK_WORDS *
        sizeof(uint64_t);
    const uintptr_t capture_dst_vaddr = hs_page_base(capture_page) + 0xa80U;
    const uintptr_t capture_dst_paddr = phys_page_base(capture_page) + 0xa80U;
    const uintptr_t capture_before_vaddr =
        capture_dst_vaddr - sizeof(uint64_t);
    const uintptr_t capture_after_vaddr =
        capture_dst_vaddr + AI_MEMBLOCK_CBOINVAL_INACTIVE_BYTES;
    const uintptr_t capture_before_paddr =
        capture_dst_paddr - sizeof(uint64_t);
    const uintptr_t capture_after_paddr =
        capture_dst_paddr + AI_MEMBLOCK_CBOINVAL_INACTIVE_BYTES;
    const uintptr_t marker_vaddr = hs_page_base(marker_page) + 0xb80U;
    const uintptr_t marker_paddr = phys_page_base(marker_page) + 0xb80U;
    const uintptr_t marker_guard_vaddr = marker_vaddr + sizeof(uint64_t);
    const uintptr_t marker_guard_paddr = marker_paddr + sizeof(uint64_t);
    static const uint8_t seed_patterns[][AI_MEMBLOCK_CBOINVAL_INACTIVE_BYTES] = {
        { 0x10, 0x21, 0x32, 0x43, 0x54, 0x65, 0x76, 0x87,
          0x98, 0xa9, 0xba, 0xcb, 0xdc, 0xed, 0xfe, 0x0f },
        { 0xf0, 0xe1, 0xd2, 0xc3, 0xb4, 0xa5, 0x96, 0x87,
          0x78, 0x69, 0x5a, 0x4b, 0x3c, 0x2d, 0x1e, 0x0f },
        { 0x55, 0xaa, 0x44, 0xbb, 0x33, 0xcc, 0x22, 0xdd,
          0x11, 0xee, 0x00, 0xff, 0x99, 0x66, 0x88, 0x77 },
    };
    static const uint8_t masked_post_inval_patterns[]
        [AI_MEMBLOCK_CBOINVAL_INACTIVE_BYTES] = {
        { 0xc1, 0xd2, 0xe3, 0xf4, 0x15, 0x26, 0x37, 0x48,
          0x59, 0x6a, 0x7b, 0x8c, 0x9d, 0xae, 0xbf, 0xc0 },
        { 0x1f, 0x2e, 0x3d, 0x4c, 0x5b, 0x6a, 0x79, 0x88,
          0x97, 0xa6, 0xb5, 0xc4, 0xd3, 0xe2, 0xf1, 0x00 },
        { 0x83, 0x72, 0x61, 0x50, 0x4f, 0x3e, 0x2d, 0x1c,
          0x0b, 0xfa, 0xe9, 0xd8, 0xc7, 0xb6, 0xa5, 0x94 },
    };
    static const uint8_t capture_old_patterns[]
        [AI_MEMBLOCK_CBOINVAL_INACTIVE_BYTES] = {
        { 0x9a, 0x89, 0x78, 0x67, 0x56, 0x45, 0x34, 0x23,
          0x12, 0x01, 0xfe, 0xed, 0xdc, 0xcb, 0xba, 0xa9 },
        { 0x31, 0x42, 0x53, 0x64, 0x75, 0x86, 0x97, 0xa8,
          0xb9, 0xca, 0xdb, 0xec, 0xfd, 0x0e, 0x1f, 0x20 },
        { 0xde, 0xad, 0xbe, 0xef, 0xca, 0xfe, 0xba, 0xbe,
          0xf0, 0x0d, 0xc0, 0xde, 0x13, 0x57, 0x9b, 0xdf },
    };
    const uint64_t masked_old_seeds[] = {
        0x22446688aaccee00ULL,
        0x1133557799bbddffULL,
        0xfedcba9876543210ULL,
    };
    const uint64_t seed_guards[] = {
        0x0101010101010101ULL,
        0x0202020202020202ULL,
        0x0303030303030303ULL,
    };
    const uint64_t masked_before_guards[] = {
        0x44aa55bb66cc77ddULL,
        0x55bb66cc77dd88eeULL,
        0x66cc77dd88ee99ffULL,
    };
    const uint64_t masked_after_guards[] = {
        0x0dd0cc0bb0aa0990ULL,
        0x1ee1dd1cc1bb1aa1ULL,
        0x2ff2ee2dd2cc2bb2ULL,
    };
    const uint64_t capture_before_guards[] = {
        0x1020304050607080ULL,
        0x2030405060708090ULL,
        0x30405060708090a0ULL,
    };
    const uint64_t capture_after_guards[] = {
        0x8877665544332211ULL,
        0x7766554433221100ULL,
        0x66554433221100ffULL,
    };
    const uint64_t marker_seeds[] = {
        0x13579bdf2468ace0ULL,
        0x2468ace013579bdfULL,
        0xfdb97531eca86420ULL,
    };
    const uint64_t marker_updates[] = {
        0xa5a55a5ac3c33c3cULL,
        0xb6b66b6bd4d44d4dULL,
        0xc7c77c7ce5e55e5eULL,
    };
    const uint64_t marker_guards[] = {
        0xf0f1f2f3f4f5f6f7ULL,
        0xe0e1e2e3e4e5e6e7ULL,
        0xd0d1d2d3d4d5d6d7ULL,
    };

    ai_memblock_cboinval_inactive_prepare_hs_env();

    for (size_t round = 0;
         round < AI_MEMBLOCK_CBOINVAL_INACTIVE_ARRAY_SIZE(masked_old_seeds);
         ++round) {
        const uint64_t masked_old_word1 =
            masked_old_seeds[round] + 0x1111111111111111ULL;
        const uint64_t expected_tail =
            masked_old_seeds[round] +
            (AI_MEMBLOCK_CBOINVAL_INACTIVE_BLOCK_WORDS - 1U) *
            0x1111111111111111ULL;
        const uint64_t expected_masked0 =
            ai_memblock_cboinval_inactive_bytes_to_u64(
                masked_post_inval_patterns[round], 0U);
        const uint64_t expected_masked1 =
            ai_memblock_cboinval_inactive_bytes_to_u64(
                masked_post_inval_patterns[round], sizeof(uint64_t));

        goto_priv(PRIV_M);
        ai_memblock_cboinval_inactive_write_bytes(seed_src_paddr,
            seed_patterns[round], AI_MEMBLOCK_CBOINVAL_INACTIVE_BYTES);
        write64(seed_guard_paddr, seed_guards[round]);
        ai_memblock_cboinval_inactive_seed_block(masked_base_paddr,
            masked_old_seeds[round]);
        write64(masked_before_paddr, masked_before_guards[round]);
        write64(masked_after_paddr, masked_after_guards[round]);
        ai_memblock_cboinval_inactive_write_bytes(capture_dst_paddr,
            capture_old_patterns[round],
            AI_MEMBLOCK_CBOINVAL_INACTIVE_BYTES);
        write64(capture_before_paddr, capture_before_guards[round]);
        write64(capture_after_paddr, capture_after_guards[round]);
        write64(marker_paddr, marker_seeds[round]);
        write64(marker_guard_paddr, marker_guards[round]);

        goto_priv(PRIV_HS);
        TEST_SETUP_EXCEPT();
        const uint64_t warmed_seed0 = ld(seed_src_vaddr);
        const uint64_t warmed_seed1 =
            ld(seed_src_vaddr + sizeof(uint64_t));
        const uint64_t warmed_masked0 = ld(masked_base_vaddr);
        const uint64_t warmed_masked1 =
            ld(masked_base_vaddr + sizeof(uint64_t));
        const uint64_t warmed_tail = ld(masked_tail_vaddr);
        const uint64_t warmed_marker = ld(marker_vaddr);
        AI_ASSERT_NO_EXCEPT_AND("before the CBO.INVAL plus all-inactive masked vector-load owner-window starts, seed source, masked source cacheline, and marker are readable with seeded values",
            warmed_seed0 ==
                ai_memblock_cboinval_inactive_bytes_to_u64(
                    seed_patterns[round], 0U) &&
            warmed_seed1 ==
                ai_memblock_cboinval_inactive_bytes_to_u64(
                    seed_patterns[round], sizeof(uint64_t)) &&
            warmed_masked0 == masked_old_seeds[round] &&
            warmed_masked1 == masked_old_word1 &&
            warmed_tail == expected_tail &&
            warmed_marker == marker_seeds[round]
        );

        goto_priv(PRIV_M);
        ai_memblock_cboinval_inactive_write_bytes(masked_base_paddr,
            masked_post_inval_patterns[round],
            AI_MEMBLOCK_CBOINVAL_INACTIVE_BYTES);

        goto_priv(PRIV_HS);
        TEST_SETUP_EXCEPT();
        ai_memblock_cboinval_then_masked_vload_capture_marker(inval_vaddr,
            seed_src_vaddr, masked_base_vaddr, capture_dst_vaddr,
            marker_vaddr, marker_updates[round]);
        const uint64_t observed_capture0 = ld(capture_dst_vaddr);
        const uint64_t observed_capture1 =
            ld(capture_dst_vaddr + sizeof(uint64_t));
        const uint64_t observed_masked0 = ld(masked_base_vaddr);
        const uint64_t observed_masked1 =
            ld(masked_base_vaddr + sizeof(uint64_t));
        const uint64_t observed_tail = ld(masked_tail_vaddr);
        const uint64_t observed_marker = ld(marker_vaddr);
        AI_ASSERT_NO_EXCEPT_AND("CBO.INVAL immediately followed by all-inactive masked unit-stride vector load keeps the preloaded vector payload, refills the masked source after inval, and releases marker progress",
            observed_capture0 ==
                ai_memblock_cboinval_inactive_bytes_to_u64(
                    seed_patterns[round], 0U) &&
            observed_capture1 ==
                ai_memblock_cboinval_inactive_bytes_to_u64(
                    seed_patterns[round], sizeof(uint64_t)) &&
            observed_masked0 == expected_masked0 &&
            observed_masked1 == expected_masked1 &&
            observed_tail == expected_tail &&
            observed_marker == marker_updates[round]
        );

        TEST_SETUP_EXCEPT();
        AI_ASSERT_NO_EXCEPT_AND("the CBO.INVAL plus all-inactive masked vector-load owner-window does not corrupt seed, masked-source, capture, or marker guards",
            ld(seed_guard_vaddr) == seed_guards[round] &&
            ld(masked_before_vaddr) == masked_before_guards[round] &&
            ld(masked_after_vaddr) == masked_after_guards[round] &&
            ld(capture_before_vaddr) == capture_before_guards[round] &&
            ld(capture_after_vaddr) == capture_after_guards[round] &&
            ld(marker_guard_vaddr) == marker_guards[round]
        );

        goto_priv(PRIV_M);
        TEST_ASSERT("backing memory after the CBO.INVAL plus all-inactive masked vector-load window keeps seed payload, captures only the preloaded vector payload, refills masked source, marker, and guards intact",
            ai_memblock_cboinval_inactive_bytes_match(seed_src_paddr,
                seed_patterns[round],
                AI_MEMBLOCK_CBOINVAL_INACTIVE_BYTES) &&
            read64(seed_guard_paddr) == seed_guards[round] &&
            ai_memblock_cboinval_inactive_bytes_match(masked_base_paddr,
                masked_post_inval_patterns[round],
                AI_MEMBLOCK_CBOINVAL_INACTIVE_BYTES) &&
            ai_memblock_cboinval_inactive_block_tail_matches_seed(
                masked_base_paddr,
                masked_old_seeds[round],
                AI_MEMBLOCK_CBOINVAL_INACTIVE_BYTES / sizeof(uint64_t)) &&
            read64(masked_before_paddr) == masked_before_guards[round] &&
            read64(masked_after_paddr) == masked_after_guards[round] &&
            ai_memblock_cboinval_inactive_bytes_match(capture_dst_paddr,
                seed_patterns[round],
                AI_MEMBLOCK_CBOINVAL_INACTIVE_BYTES) &&
            read64(capture_before_paddr) == capture_before_guards[round] &&
            read64(capture_after_paddr) == capture_after_guards[round] &&
            read64(marker_paddr) == marker_updates[round] &&
            read64(marker_guard_paddr) == marker_guards[round]
        );
    }

    ai_memblock_cboinval_inactive_reset_hs_env();

    TEST_END();
}

static inline void ai_memblock_masked_vstore_then_cboinval_marker(
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
        "vmv.v.i v0, 0\n\t"
        "vse8.v v8, (%[dst_addr]), v0.t\n\t"
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
          [vl] "r"((int)AI_MEMBLOCK_CBOINVAL_INACTIVE_BYTES),
          [vtype] "r"(vtype)
        : "t0", "memory");
}

bool ai_micro_hs_masked_vector_store_all_inactive_then_cboinval_no_side_effect_or_inval_drop_corner()
{
    TEST_START();

    const enum test_page src_page = SWITCH1;
    const enum test_page dst_page = SWITCH2;
    const enum test_page marker_page = VSRWX_GRWX;
    const uintptr_t src_vaddr = hs_page_base(src_page) + 0xc80U;
    const uintptr_t src_paddr = phys_page_base(src_page) + 0xc80U;
    const uintptr_t src_guard_vaddr =
        src_vaddr + AI_MEMBLOCK_CBOINVAL_INACTIVE_BYTES;
    const uintptr_t src_guard_paddr =
        src_paddr + AI_MEMBLOCK_CBOINVAL_INACTIVE_BYTES;
    const uintptr_t dst_base_vaddr = hs_page_base(dst_page) + 0xc80U;
    const uintptr_t dst_base_paddr = phys_page_base(dst_page) + 0xc80U;
    const uintptr_t inval_vaddr =
        dst_base_vaddr + 4U * sizeof(uint64_t);
    const uintptr_t dst_tail_vaddr =
        dst_base_vaddr +
        (AI_MEMBLOCK_CBOINVAL_INACTIVE_BLOCK_WORDS - 1U) *
        sizeof(uint64_t);
    const uintptr_t dst_before_vaddr =
        dst_base_vaddr - sizeof(uint64_t);
    const uintptr_t dst_after_vaddr =
        dst_base_vaddr + AI_MEMBLOCK_CBOINVAL_INACTIVE_BLOCK_WORDS *
        sizeof(uint64_t);
    const uintptr_t dst_before_paddr =
        dst_base_paddr - sizeof(uint64_t);
    const uintptr_t dst_after_paddr =
        dst_base_paddr + AI_MEMBLOCK_CBOINVAL_INACTIVE_BLOCK_WORDS *
        sizeof(uint64_t);
    const uintptr_t marker_vaddr = hs_page_base(marker_page) + 0xd80U;
    const uintptr_t marker_paddr = phys_page_base(marker_page) + 0xd80U;
    const uintptr_t marker_guard_vaddr = marker_vaddr + sizeof(uint64_t);
    const uintptr_t marker_guard_paddr = marker_paddr + sizeof(uint64_t);
    static const uint8_t src_patterns[][AI_MEMBLOCK_CBOINVAL_INACTIVE_BYTES] = {
        { 0xe1, 0xd2, 0xc3, 0xb4, 0xa5, 0x96, 0x87, 0x78,
          0x69, 0x5a, 0x4b, 0x3c, 0x2d, 0x1e, 0x0f, 0xf0 },
        { 0x18, 0x29, 0x3a, 0x4b, 0x5c, 0x6d, 0x7e, 0x8f,
          0x90, 0xa1, 0xb2, 0xc3, 0xd4, 0xe5, 0xf6, 0x07 },
        { 0x44, 0x11, 0x55, 0x22, 0x66, 0x33, 0x77, 0x88,
          0x99, 0xaa, 0xbb, 0xcc, 0xdd, 0xee, 0xff, 0x00 },
    };
    static const uint8_t dst_post_inval_patterns[]
        [AI_MEMBLOCK_CBOINVAL_INACTIVE_BYTES] = {
        { 0x0a, 0x1b, 0x2c, 0x3d, 0x4e, 0x5f, 0x60, 0x71,
          0x82, 0x93, 0xa4, 0xb5, 0xc6, 0xd7, 0xe8, 0xf9 },
        { 0xf9, 0xe8, 0xd7, 0xc6, 0xb5, 0xa4, 0x93, 0x82,
          0x71, 0x60, 0x5f, 0x4e, 0x3d, 0x2c, 0x1b, 0x0a },
        { 0x21, 0x43, 0x65, 0x87, 0xa9, 0xcb, 0xed, 0x0f,
          0x10, 0x32, 0x54, 0x76, 0x98, 0xba, 0xdc, 0xfe },
    };
    const uint64_t dst_old_seeds[] = {
        0x3141592653589793ULL,
        0x2718281828459045ULL,
        0x1618033988749894ULL,
    };
    const uint64_t src_guards[] = {
        0x0102030405060708ULL,
        0x1827364554637281ULL,
        0xfedcba9876543210ULL,
    };
    const uint64_t dst_before_guards[] = {
        0x55aa55aa55aa55aaULL,
        0x66bb66bb66bb66bbULL,
        0x77cc77cc77cc77ccULL,
    };
    const uint64_t dst_after_guards[] = {
        0xaa55aa55aa55aa55ULL,
        0xbb66bb66bb66bb66ULL,
        0xcc77cc77cc77cc77ULL,
    };
    const uint64_t marker_seeds[] = {
        0x1111222233334444ULL,
        0x5555666677778888ULL,
        0x9999aaaabbbbccccULL,
    };
    const uint64_t marker_updates[] = {
        0x0badf00d5a5aa5a5ULL,
        0x1020304050607080ULL,
        0x1122334455667788ULL,
    };
    const uint64_t marker_guards[] = {
        0x8877665544332211ULL,
        0x7766554433221100ULL,
        0x66554433221100ffULL,
    };

    ai_memblock_cboinval_inactive_prepare_hs_env();

    for (size_t round = 0;
         round < AI_MEMBLOCK_CBOINVAL_INACTIVE_ARRAY_SIZE(dst_old_seeds);
         ++round) {
        const uint64_t dst_old_word1 =
            dst_old_seeds[round] + 0x1111111111111111ULL;
        const uint64_t expected_tail =
            dst_old_seeds[round] +
            (AI_MEMBLOCK_CBOINVAL_INACTIVE_BLOCK_WORDS - 1U) *
            0x1111111111111111ULL;
        const uint64_t expected_dst0 =
            ai_memblock_cboinval_inactive_bytes_to_u64(
                dst_post_inval_patterns[round], 0U);
        const uint64_t expected_dst1 =
            ai_memblock_cboinval_inactive_bytes_to_u64(
                dst_post_inval_patterns[round], sizeof(uint64_t));

        goto_priv(PRIV_M);
        ai_memblock_cboinval_inactive_write_bytes(src_paddr,
            src_patterns[round], AI_MEMBLOCK_CBOINVAL_INACTIVE_BYTES);
        write64(src_guard_paddr, src_guards[round]);
        ai_memblock_cboinval_inactive_seed_block(dst_base_paddr,
            dst_old_seeds[round]);
        write64(dst_before_paddr, dst_before_guards[round]);
        write64(dst_after_paddr, dst_after_guards[round]);
        write64(marker_paddr, marker_seeds[round]);
        write64(marker_guard_paddr, marker_guards[round]);

        goto_priv(PRIV_HS);
        TEST_SETUP_EXCEPT();
        const uint64_t warmed_src0 = ld(src_vaddr);
        const uint64_t warmed_src1 = ld(src_vaddr + sizeof(uint64_t));
        const uint64_t warmed_dst0 = ld(dst_base_vaddr);
        const uint64_t warmed_dst1 =
            ld(dst_base_vaddr + sizeof(uint64_t));
        const uint64_t warmed_tail = ld(dst_tail_vaddr);
        const uint64_t warmed_marker = ld(marker_vaddr);
        AI_ASSERT_NO_EXCEPT_AND("before the all-inactive masked vector-store plus CBO.INVAL owner-window starts, source, destination cacheline, and marker are readable with seeded values",
            warmed_src0 ==
                ai_memblock_cboinval_inactive_bytes_to_u64(
                    src_patterns[round], 0U) &&
            warmed_src1 ==
                ai_memblock_cboinval_inactive_bytes_to_u64(
                    src_patterns[round], sizeof(uint64_t)) &&
            warmed_dst0 == dst_old_seeds[round] &&
            warmed_dst1 == dst_old_word1 &&
            warmed_tail == expected_tail &&
            warmed_marker == marker_seeds[round]
        );

        goto_priv(PRIV_M);
        ai_memblock_cboinval_inactive_write_bytes(dst_base_paddr,
            dst_post_inval_patterns[round],
            AI_MEMBLOCK_CBOINVAL_INACTIVE_BYTES);

        goto_priv(PRIV_HS);
        TEST_SETUP_EXCEPT();
        ai_memblock_masked_vstore_then_cboinval_marker(src_vaddr,
            dst_base_vaddr, inval_vaddr, marker_vaddr,
            marker_updates[round]);
        const uint64_t observed_dst0 = ld(dst_base_vaddr);
        const uint64_t observed_dst1 =
            ld(dst_base_vaddr + sizeof(uint64_t));
        const uint64_t observed_tail = ld(dst_tail_vaddr);
        const uint64_t observed_marker = ld(marker_vaddr);
        AI_ASSERT_NO_EXCEPT_AND("all-inactive masked unit-stride vector store immediately followed by CBO.INVAL makes no vector-store side effect, refills the post-inval destination image, and releases marker progress",
            observed_dst0 == expected_dst0 &&
            observed_dst1 == expected_dst1 &&
            observed_tail == expected_tail &&
            observed_marker == marker_updates[round]
        );

        TEST_SETUP_EXCEPT();
        AI_ASSERT_NO_EXCEPT_AND("the all-inactive masked vector-store plus CBO.INVAL owner-window does not corrupt source, destination, or marker guards",
            ld(src_guard_vaddr) == src_guards[round] &&
            ld(dst_before_vaddr) == dst_before_guards[round] &&
            ld(dst_after_vaddr) == dst_after_guards[round] &&
            ld(marker_guard_vaddr) == marker_guards[round]
        );

        goto_priv(PRIV_M);
        TEST_ASSERT("backing memory after the all-inactive masked vector-store plus CBO.INVAL window keeps source payload, destination post-inval image, destination tail, marker, and guards intact",
            ai_memblock_cboinval_inactive_bytes_match(src_paddr,
                src_patterns[round],
                AI_MEMBLOCK_CBOINVAL_INACTIVE_BYTES) &&
            read64(src_guard_paddr) == src_guards[round] &&
            ai_memblock_cboinval_inactive_bytes_match(dst_base_paddr,
                dst_post_inval_patterns[round],
                AI_MEMBLOCK_CBOINVAL_INACTIVE_BYTES) &&
            ai_memblock_cboinval_inactive_block_tail_matches_seed(
                dst_base_paddr,
                dst_old_seeds[round],
                AI_MEMBLOCK_CBOINVAL_INACTIVE_BYTES / sizeof(uint64_t)) &&
            read64(dst_before_paddr) == dst_before_guards[round] &&
            read64(dst_after_paddr) == dst_after_guards[round] &&
            read64(marker_paddr) == marker_updates[round] &&
            read64(marker_guard_paddr) == marker_guards[round]
        );
    }

    ai_memblock_cboinval_inactive_reset_hs_env();

    TEST_END();
}

static inline void ai_memblock_masked_vload_then_cboinval_capture_marker(
    uintptr_t seed_src_vaddr,
    uintptr_t masked_src_vaddr,
    uintptr_t inval_vaddr,
    uintptr_t capture_dst_vaddr,
    uintptr_t marker_vaddr,
    uint64_t marker_update)
{
    const int vtype = VTYPE(VSEW_FROM_BITS(8), 0);

    asm volatile(
        ".option push\n\t"
        ".option norvc\n\t"
        "vsetvl t0, %[vl], %[vtype]\n\t"
        "vle8.v v8, (%[seed_src_addr])\n\t"
        "vmv.v.i v0, 0\n\t"
        "vle8.v v8, (%[masked_src_addr]), v0.t\n\t"
        ".insn i 0x0f, 0x2, x0, %[inval_addr], 0\n\t"
        "vse8.v v8, (%[capture_dst_addr])\n\t"
        "sd %[marker_update], 0(%[marker_addr])\n\t"
        "fence rw, rw\n\t"
        ".option pop\n\t"
        :
        : [seed_src_addr] "r"(seed_src_vaddr),
          [masked_src_addr] "r"(masked_src_vaddr),
          [inval_addr] "r"(inval_vaddr),
          [capture_dst_addr] "r"(capture_dst_vaddr),
          [marker_addr] "r"(marker_vaddr),
          [marker_update] "r"(marker_update),
          [vl] "r"((int)AI_MEMBLOCK_CBOINVAL_INACTIVE_BYTES),
          [vtype] "r"(vtype)
        : "t0", "memory");
}

bool ai_micro_hs_masked_vector_load_all_inactive_then_cboinval_no_dest_clobber_or_inval_drop_corner()
{
    TEST_START();

    const enum test_page seed_page = SWITCH1;
    const enum test_page masked_page = SWITCH2;
    const enum test_page capture_page = VSRWX_GRWX;
    const enum test_page marker_page = VSRWX_GRWX;
    const uintptr_t seed_src_vaddr = hs_page_base(seed_page) + 0xe80U;
    const uintptr_t seed_src_paddr = phys_page_base(seed_page) + 0xe80U;
    const uintptr_t seed_guard_vaddr =
        seed_src_vaddr + AI_MEMBLOCK_CBOINVAL_INACTIVE_BYTES;
    const uintptr_t seed_guard_paddr =
        seed_src_paddr + AI_MEMBLOCK_CBOINVAL_INACTIVE_BYTES;
    const uintptr_t masked_base_vaddr = hs_page_base(masked_page) + 0xe80U;
    const uintptr_t masked_base_paddr = phys_page_base(masked_page) + 0xe80U;
    const uintptr_t inval_vaddr =
        masked_base_vaddr + 4U * sizeof(uint64_t);
    const uintptr_t masked_tail_vaddr =
        masked_base_vaddr +
        (AI_MEMBLOCK_CBOINVAL_INACTIVE_BLOCK_WORDS - 1U) *
        sizeof(uint64_t);
    const uintptr_t masked_before_vaddr =
        masked_base_vaddr - sizeof(uint64_t);
    const uintptr_t masked_after_vaddr =
        masked_base_vaddr + AI_MEMBLOCK_CBOINVAL_INACTIVE_BLOCK_WORDS *
        sizeof(uint64_t);
    const uintptr_t masked_before_paddr =
        masked_base_paddr - sizeof(uint64_t);
    const uintptr_t masked_after_paddr =
        masked_base_paddr + AI_MEMBLOCK_CBOINVAL_INACTIVE_BLOCK_WORDS *
        sizeof(uint64_t);
    const uintptr_t capture_dst_vaddr = hs_page_base(capture_page) + 0xe80U;
    const uintptr_t capture_dst_paddr = phys_page_base(capture_page) + 0xe80U;
    const uintptr_t capture_before_vaddr =
        capture_dst_vaddr - sizeof(uint64_t);
    const uintptr_t capture_after_vaddr =
        capture_dst_vaddr + AI_MEMBLOCK_CBOINVAL_INACTIVE_BYTES;
    const uintptr_t capture_before_paddr =
        capture_dst_paddr - sizeof(uint64_t);
    const uintptr_t capture_after_paddr =
        capture_dst_paddr + AI_MEMBLOCK_CBOINVAL_INACTIVE_BYTES;
    const uintptr_t marker_vaddr = hs_page_base(marker_page) + 0xf40U;
    const uintptr_t marker_paddr = phys_page_base(marker_page) + 0xf40U;
    const uintptr_t marker_guard_vaddr = marker_vaddr + sizeof(uint64_t);
    const uintptr_t marker_guard_paddr = marker_paddr + sizeof(uint64_t);
    static const uint8_t seed_patterns[][AI_MEMBLOCK_CBOINVAL_INACTIVE_BYTES] = {
        { 0x7a, 0x69, 0x58, 0x47, 0x36, 0x25, 0x14, 0x03,
          0xf2, 0xe1, 0xd0, 0xcf, 0xbe, 0xad, 0x9c, 0x8b },
        { 0x08, 0x19, 0x2a, 0x3b, 0x4c, 0x5d, 0x6e, 0x7f,
          0x80, 0x91, 0xa2, 0xb3, 0xc4, 0xd5, 0xe6, 0xf7 },
        { 0x5a, 0xa5, 0x69, 0x96, 0x78, 0x87, 0x4b, 0xb4,
          0x3c, 0xc3, 0x2d, 0xd2, 0x1e, 0xe1, 0x0f, 0xf0 },
    };
    static const uint8_t masked_post_inval_patterns[]
        [AI_MEMBLOCK_CBOINVAL_INACTIVE_BYTES] = {
        { 0xd0, 0xc1, 0xb2, 0xa3, 0x94, 0x85, 0x76, 0x67,
          0x58, 0x49, 0x3a, 0x2b, 0x1c, 0x0d, 0xfe, 0xef },
        { 0x2f, 0x3e, 0x4d, 0x5c, 0x6b, 0x7a, 0x89, 0x98,
          0xa7, 0xb6, 0xc5, 0xd4, 0xe3, 0xf2, 0x01, 0x10 },
        { 0x93, 0x82, 0x71, 0x60, 0x5f, 0x4e, 0x3d, 0x2c,
          0x1b, 0x0a, 0xf9, 0xe8, 0xd7, 0xc6, 0xb5, 0xa4 },
    };
    static const uint8_t capture_old_patterns[]
        [AI_MEMBLOCK_CBOINVAL_INACTIVE_BYTES] = {
        { 0x01, 0x23, 0x45, 0x67, 0x89, 0xab, 0xcd, 0xef,
          0xfe, 0xdc, 0xba, 0x98, 0x76, 0x54, 0x32, 0x10 },
        { 0xca, 0xfe, 0xba, 0xbe, 0x13, 0x57, 0x9b, 0xdf,
          0xf0, 0x0d, 0xc0, 0xde, 0x24, 0x68, 0xac, 0xe0 },
        { 0x6d, 0x5c, 0x4b, 0x3a, 0x29, 0x18, 0x07, 0xf6,
          0xe5, 0xd4, 0xc3, 0xb2, 0xa1, 0x90, 0x8f, 0x7e },
    };
    const uint64_t masked_old_seeds[] = {
        0x0f0e0d0c0b0a0908ULL,
        0x8070605040302010ULL,
        0x123456789abcdef0ULL,
    };
    const uint64_t seed_guards[] = {
        0xabcdef0123456789ULL,
        0xbcdef0123456789aULL,
        0xcdef0123456789abULL,
    };
    const uint64_t masked_before_guards[] = {
        0x0123456789abcdefULL,
        0x123456789abcdef0ULL,
        0x23456789abcdef01ULL,
    };
    const uint64_t masked_after_guards[] = {
        0xfedcba9876543210ULL,
        0xedcba9876543210fULL,
        0xdcba9876543210feULL,
    };
    const uint64_t capture_before_guards[] = {
        0x0a0b0c0d0e0f1011ULL,
        0x1a1b1c1d1e1f2021ULL,
        0x2a2b2c2d2e2f3031ULL,
    };
    const uint64_t capture_after_guards[] = {
        0x1122334455667788ULL,
        0x2233445566778899ULL,
        0x33445566778899aaULL,
    };
    const uint64_t marker_seeds[] = {
        0x5566778899aabbccULL,
        0x66778899aabbccddULL,
        0x778899aabbccddeeULL,
    };
    const uint64_t marker_updates[] = {
        0x0101010101010101ULL,
        0x0202020202020202ULL,
        0x0303030303030303ULL,
    };
    const uint64_t marker_guards[] = {
        0xdeadbeefcafebabeULL,
        0xfeedface0badf00dULL,
        0xfaceb00c13579bdfULL,
    };

    ai_memblock_cboinval_inactive_prepare_hs_env();

    for (size_t round = 0;
         round < AI_MEMBLOCK_CBOINVAL_INACTIVE_ARRAY_SIZE(masked_old_seeds);
         ++round) {
        const uint64_t masked_old_word1 =
            masked_old_seeds[round] + 0x1111111111111111ULL;
        const uint64_t expected_tail =
            masked_old_seeds[round] +
            (AI_MEMBLOCK_CBOINVAL_INACTIVE_BLOCK_WORDS - 1U) *
            0x1111111111111111ULL;
        const uint64_t expected_masked0 =
            ai_memblock_cboinval_inactive_bytes_to_u64(
                masked_post_inval_patterns[round], 0U);
        const uint64_t expected_masked1 =
            ai_memblock_cboinval_inactive_bytes_to_u64(
                masked_post_inval_patterns[round], sizeof(uint64_t));

        goto_priv(PRIV_M);
        ai_memblock_cboinval_inactive_write_bytes(seed_src_paddr,
            seed_patterns[round], AI_MEMBLOCK_CBOINVAL_INACTIVE_BYTES);
        write64(seed_guard_paddr, seed_guards[round]);
        ai_memblock_cboinval_inactive_seed_block(masked_base_paddr,
            masked_old_seeds[round]);
        write64(masked_before_paddr, masked_before_guards[round]);
        write64(masked_after_paddr, masked_after_guards[round]);
        ai_memblock_cboinval_inactive_write_bytes(capture_dst_paddr,
            capture_old_patterns[round],
            AI_MEMBLOCK_CBOINVAL_INACTIVE_BYTES);
        write64(capture_before_paddr, capture_before_guards[round]);
        write64(capture_after_paddr, capture_after_guards[round]);
        write64(marker_paddr, marker_seeds[round]);
        write64(marker_guard_paddr, marker_guards[round]);

        goto_priv(PRIV_HS);
        TEST_SETUP_EXCEPT();
        const uint64_t warmed_seed0 = ld(seed_src_vaddr);
        const uint64_t warmed_seed1 =
            ld(seed_src_vaddr + sizeof(uint64_t));
        const uint64_t warmed_masked0 = ld(masked_base_vaddr);
        const uint64_t warmed_masked1 =
            ld(masked_base_vaddr + sizeof(uint64_t));
        const uint64_t warmed_tail = ld(masked_tail_vaddr);
        const uint64_t warmed_marker = ld(marker_vaddr);
        AI_ASSERT_NO_EXCEPT_AND("before the all-inactive masked vector-load plus CBO.INVAL owner-window starts, seed source, masked source cacheline, and marker are readable with seeded values",
            warmed_seed0 ==
                ai_memblock_cboinval_inactive_bytes_to_u64(
                    seed_patterns[round], 0U) &&
            warmed_seed1 ==
                ai_memblock_cboinval_inactive_bytes_to_u64(
                    seed_patterns[round], sizeof(uint64_t)) &&
            warmed_masked0 == masked_old_seeds[round] &&
            warmed_masked1 == masked_old_word1 &&
            warmed_tail == expected_tail &&
            warmed_marker == marker_seeds[round]
        );

        goto_priv(PRIV_M);
        ai_memblock_cboinval_inactive_write_bytes(masked_base_paddr,
            masked_post_inval_patterns[round],
            AI_MEMBLOCK_CBOINVAL_INACTIVE_BYTES);

        goto_priv(PRIV_HS);
        TEST_SETUP_EXCEPT();
        ai_memblock_masked_vload_then_cboinval_capture_marker(seed_src_vaddr,
            masked_base_vaddr, inval_vaddr, capture_dst_vaddr,
            marker_vaddr, marker_updates[round]);
        const uint64_t observed_capture0 = ld(capture_dst_vaddr);
        const uint64_t observed_capture1 =
            ld(capture_dst_vaddr + sizeof(uint64_t));
        const uint64_t observed_masked0 = ld(masked_base_vaddr);
        const uint64_t observed_masked1 =
            ld(masked_base_vaddr + sizeof(uint64_t));
        const uint64_t observed_tail = ld(masked_tail_vaddr);
        const uint64_t observed_marker = ld(marker_vaddr);
        AI_ASSERT_NO_EXCEPT_AND("all-inactive masked unit-stride vector load immediately followed by CBO.INVAL keeps the preloaded vector payload, refills the masked source after inval, and releases marker progress",
            observed_capture0 ==
                ai_memblock_cboinval_inactive_bytes_to_u64(
                    seed_patterns[round], 0U) &&
            observed_capture1 ==
                ai_memblock_cboinval_inactive_bytes_to_u64(
                    seed_patterns[round], sizeof(uint64_t)) &&
            observed_masked0 == expected_masked0 &&
            observed_masked1 == expected_masked1 &&
            observed_tail == expected_tail &&
            observed_marker == marker_updates[round]
        );

        TEST_SETUP_EXCEPT();
        AI_ASSERT_NO_EXCEPT_AND("the all-inactive masked vector-load plus CBO.INVAL owner-window does not corrupt seed, masked-source, capture, or marker guards",
            ld(seed_guard_vaddr) == seed_guards[round] &&
            ld(masked_before_vaddr) == masked_before_guards[round] &&
            ld(masked_after_vaddr) == masked_after_guards[round] &&
            ld(capture_before_vaddr) == capture_before_guards[round] &&
            ld(capture_after_vaddr) == capture_after_guards[round] &&
            ld(marker_guard_vaddr) == marker_guards[round]
        );

        goto_priv(PRIV_M);
        TEST_ASSERT("backing memory after the all-inactive masked vector-load plus CBO.INVAL window keeps seed payload, captures only the preloaded vector payload, refills masked source, marker, and guards intact",
            ai_memblock_cboinval_inactive_bytes_match(seed_src_paddr,
                seed_patterns[round],
                AI_MEMBLOCK_CBOINVAL_INACTIVE_BYTES) &&
            read64(seed_guard_paddr) == seed_guards[round] &&
            ai_memblock_cboinval_inactive_bytes_match(masked_base_paddr,
                masked_post_inval_patterns[round],
                AI_MEMBLOCK_CBOINVAL_INACTIVE_BYTES) &&
            ai_memblock_cboinval_inactive_block_tail_matches_seed(
                masked_base_paddr,
                masked_old_seeds[round],
                AI_MEMBLOCK_CBOINVAL_INACTIVE_BYTES / sizeof(uint64_t)) &&
            read64(masked_before_paddr) == masked_before_guards[round] &&
            read64(masked_after_paddr) == masked_after_guards[round] &&
            ai_memblock_cboinval_inactive_bytes_match(capture_dst_paddr,
                seed_patterns[round],
                AI_MEMBLOCK_CBOINVAL_INACTIVE_BYTES) &&
            read64(capture_before_paddr) == capture_before_guards[round] &&
            read64(capture_after_paddr) == capture_after_guards[round] &&
            read64(marker_paddr) == marker_updates[round] &&
            read64(marker_guard_paddr) == marker_guards[round]
        );
    }

    ai_memblock_cboinval_inactive_reset_hs_env();

    TEST_END();
}

static inline void ai_memblock_cboinval_then_partial_masked_vstore_marker(
    uintptr_t inval_vaddr,
    uintptr_t src_vaddr,
    uintptr_t dst_vaddr,
    uintptr_t mask_vaddr,
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
        "vlm.v v0, (%[mask_addr])\n\t"
        "vse8.v v8, (%[dst_addr]), v0.t\n\t"
        "sd %[marker_update], 0(%[marker_addr])\n\t"
        "fence rw, rw\n\t"
        ".option pop\n\t"
        :
        : [inval_addr] "r"(inval_vaddr),
          [src_addr] "r"(src_vaddr),
          [dst_addr] "r"(dst_vaddr),
          [mask_addr] "r"(mask_vaddr),
          [marker_addr] "r"(marker_vaddr),
          [marker_update] "r"(marker_update),
          [vl] "r"((int)AI_MEMBLOCK_CBOINVAL_INACTIVE_BYTES),
          [vtype] "r"(vtype)
        : "t0", "memory");
}

bool ai_micro_hs_cboinval_then_masked_vector_store_partial_active_no_lane_mix_or_marker_drop_corner()
{
    TEST_START();

    const enum test_page src_page = SWITCH1;
    const enum test_page dst_page = SWITCH2;
    const enum test_page aux_page = VSRWX_GRWX;
    const uintptr_t src_vaddr = hs_page_base(src_page) + 0x240U;
    const uintptr_t src_paddr = phys_page_base(src_page) + 0x240U;
    const uintptr_t src_guard_vaddr =
        src_vaddr + AI_MEMBLOCK_CBOINVAL_INACTIVE_BYTES;
    const uintptr_t src_guard_paddr =
        src_paddr + AI_MEMBLOCK_CBOINVAL_INACTIVE_BYTES;
    const uintptr_t dst_base_vaddr = hs_page_base(dst_page) + 0x240U;
    const uintptr_t dst_base_paddr = phys_page_base(dst_page) + 0x240U;
    const uintptr_t inval_vaddr =
        dst_base_vaddr + 4U * sizeof(uint64_t);
    const uintptr_t dst_tail_vaddr =
        dst_base_vaddr +
        (AI_MEMBLOCK_CBOINVAL_INACTIVE_BLOCK_WORDS - 1U) *
        sizeof(uint64_t);
    const uintptr_t dst_before_vaddr =
        dst_base_vaddr - sizeof(uint64_t);
    const uintptr_t dst_after_vaddr =
        dst_base_vaddr + AI_MEMBLOCK_CBOINVAL_INACTIVE_BLOCK_WORDS *
        sizeof(uint64_t);
    const uintptr_t dst_before_paddr =
        dst_base_paddr - sizeof(uint64_t);
    const uintptr_t dst_after_paddr =
        dst_base_paddr + AI_MEMBLOCK_CBOINVAL_INACTIVE_BLOCK_WORDS *
        sizeof(uint64_t);
    const uintptr_t mask_vaddr = hs_page_base(aux_page) + 0x240U;
    const uintptr_t mask_paddr = phys_page_base(aux_page) + 0x240U;
    const uintptr_t mask_guard_paddr = mask_paddr + sizeof(uint64_t);
    const uintptr_t mask_guard_vaddr = mask_vaddr + sizeof(uint64_t);
    const uintptr_t marker_vaddr = hs_page_base(aux_page) + 0x2c0U;
    const uintptr_t marker_paddr = phys_page_base(aux_page) + 0x2c0U;
    const uintptr_t marker_guard_vaddr = marker_vaddr + sizeof(uint64_t);
    const uintptr_t marker_guard_paddr = marker_paddr + sizeof(uint64_t);
    static const uint8_t src_patterns[][AI_MEMBLOCK_CBOINVAL_INACTIVE_BYTES] = {
        { 0xf0, 0xe1, 0xd2, 0xc3, 0xb4, 0xa5, 0x96, 0x87,
          0x78, 0x69, 0x5a, 0x4b, 0x3c, 0x2d, 0x1e, 0x0f },
        { 0x13, 0x24, 0x35, 0x46, 0x57, 0x68, 0x79, 0x8a,
          0x9b, 0xac, 0xbd, 0xce, 0xdf, 0xe0, 0xf1, 0x02 },
        { 0x91, 0x82, 0x73, 0x64, 0x55, 0x46, 0x37, 0x28,
          0x19, 0x0a, 0xfb, 0xec, 0xdd, 0xce, 0xbf, 0xa0 },
    };
    static const uint8_t dst_post_inval_patterns[]
        [AI_MEMBLOCK_CBOINVAL_INACTIVE_BYTES] = {
        { 0x0f, 0x1e, 0x2d, 0x3c, 0x4b, 0x5a, 0x69, 0x78,
          0x87, 0x96, 0xa5, 0xb4, 0xc3, 0xd2, 0xe1, 0xf0 },
        { 0x7f, 0x6e, 0x5d, 0x4c, 0x3b, 0x2a, 0x19, 0x08,
          0x18, 0x29, 0x3a, 0x4b, 0x5c, 0x6d, 0x7e, 0x8f },
        { 0xa0, 0xbf, 0xce, 0xdd, 0xec, 0xfb, 0x0a, 0x19,
          0x28, 0x37, 0x46, 0x55, 0x64, 0x73, 0x82, 0x91 },
    };
    static const uint8_t mask_patterns[][2] = {
        { 0x55, 0x55 },
        { 0xaa, 0xaa },
        { 0x3c, 0xc3 },
    };
    const uint64_t dst_old_seeds[] = {
        0x0102030405060708ULL,
        0x1827364554637281ULL,
        0xfedcba9876543210ULL,
    };
    const uint64_t src_guards[] = {
        0x0f0e0d0c0b0a0908ULL,
        0x8070605040302010ULL,
        0x123456789abcdef0ULL,
    };
    const uint64_t dst_before_guards[] = {
        0x13579bdf2468ace0ULL,
        0x2468ace013579bdfULL,
        0xfdb97531eca86420ULL,
    };
    const uint64_t dst_after_guards[] = {
        0x55aa55aa55aa55aaULL,
        0x66bb66bb66bb66bbULL,
        0x77cc77cc77cc77ccULL,
    };
    const uint64_t mask_guards[] = {
        0xaa55aa55aa55aa55ULL,
        0xbb66bb66bb66bb66ULL,
        0xcc77cc77cc77cc77ULL,
    };
    const uint64_t marker_seeds[] = {
        0x3141592653589793ULL,
        0x2718281828459045ULL,
        0x1618033988749894ULL,
    };
    const uint64_t marker_updates[] = {
        0x8877665544332211ULL,
        0x7766554433221100ULL,
        0x66554433221100ffULL,
    };
    const uint64_t marker_guards[] = {
        0xabcdef0123456789ULL,
        0xbcdef0123456789aULL,
        0xcdef0123456789abULL,
    };

    ai_memblock_cboinval_inactive_prepare_hs_env();

    for (size_t round = 0;
         round < AI_MEMBLOCK_CBOINVAL_INACTIVE_ARRAY_SIZE(dst_old_seeds);
         ++round) {
        const uint64_t dst_old_word1 =
            dst_old_seeds[round] + 0x1111111111111111ULL;
        const uint64_t expected_tail =
            dst_old_seeds[round] +
            (AI_MEMBLOCK_CBOINVAL_INACTIVE_BLOCK_WORDS - 1U) *
            0x1111111111111111ULL;

        goto_priv(PRIV_M);
        ai_memblock_cboinval_inactive_write_bytes(src_paddr,
            src_patterns[round], AI_MEMBLOCK_CBOINVAL_INACTIVE_BYTES);
        write64(src_guard_paddr, src_guards[round]);
        ai_memblock_cboinval_inactive_seed_block(dst_base_paddr,
            dst_old_seeds[round]);
        write64(dst_before_paddr, dst_before_guards[round]);
        write64(dst_after_paddr, dst_after_guards[round]);
        write8(mask_paddr, mask_patterns[round][0]);
        write8(mask_paddr + 1U, mask_patterns[round][1]);
        write64(mask_guard_paddr, mask_guards[round]);
        write64(marker_paddr, marker_seeds[round]);
        write64(marker_guard_paddr, marker_guards[round]);

        goto_priv(PRIV_HS);
        TEST_SETUP_EXCEPT();
        const uint64_t warmed_src0 = ld(src_vaddr);
        const uint64_t warmed_src1 = ld(src_vaddr + sizeof(uint64_t));
        const uint64_t warmed_dst0 = ld(dst_base_vaddr);
        const uint64_t warmed_dst1 =
            ld(dst_base_vaddr + sizeof(uint64_t));
        const uint64_t warmed_tail = ld(dst_tail_vaddr);
        const uint64_t warmed_marker = ld(marker_vaddr);
        AI_ASSERT_NO_EXCEPT_AND("before the CBO.INVAL plus partial-active masked vector-store owner-window starts, source, destination cacheline, and marker are readable with seeded values",
            warmed_src0 ==
                ai_memblock_cboinval_inactive_bytes_to_u64(
                    src_patterns[round], 0U) &&
            warmed_src1 ==
                ai_memblock_cboinval_inactive_bytes_to_u64(
                    src_patterns[round], sizeof(uint64_t)) &&
            warmed_dst0 == dst_old_seeds[round] &&
            warmed_dst1 == dst_old_word1 &&
            warmed_tail == expected_tail &&
            warmed_marker == marker_seeds[round]
        );

        goto_priv(PRIV_M);
        ai_memblock_cboinval_inactive_write_bytes(dst_base_paddr,
            dst_post_inval_patterns[round],
            AI_MEMBLOCK_CBOINVAL_INACTIVE_BYTES);

        goto_priv(PRIV_HS);
        TEST_SETUP_EXCEPT();
        ai_memblock_cboinval_then_partial_masked_vstore_marker(inval_vaddr,
            src_vaddr, dst_base_vaddr, mask_vaddr, marker_vaddr,
            marker_updates[round]);
        const uint64_t observed_tail = ld(dst_tail_vaddr);
        const uint64_t observed_marker = ld(marker_vaddr);
        AI_ASSERT_NO_EXCEPT_AND("CBO.INVAL immediately followed by partial-active masked unit-stride vector store updates only active lanes, preserves inactive post-inval lanes, and releases marker progress",
            ai_memblock_cboinval_partial_masked_store_matches(
                dst_base_vaddr,
                src_patterns[round],
                dst_post_inval_patterns[round],
                mask_patterns[round],
                AI_MEMBLOCK_CBOINVAL_INACTIVE_BYTES) &&
            observed_tail == expected_tail &&
            observed_marker == marker_updates[round]
        );

        TEST_SETUP_EXCEPT();
        AI_ASSERT_NO_EXCEPT_AND("the CBO.INVAL plus partial-active masked vector-store owner-window does not corrupt source, destination, mask, or marker guards",
            ld(src_guard_vaddr) == src_guards[round] &&
            ld(dst_before_vaddr) == dst_before_guards[round] &&
            ld(dst_after_vaddr) == dst_after_guards[round] &&
            ld(mask_guard_vaddr) == mask_guards[round] &&
            ld(marker_guard_vaddr) == marker_guards[round]
        );

        goto_priv(PRIV_M);
        TEST_ASSERT("backing memory after the CBO.INVAL plus partial-active masked vector-store window contains active source bytes, inactive post-inval bytes, destination tail, marker, and guards intact",
            ai_memblock_cboinval_inactive_bytes_match(src_paddr,
                src_patterns[round],
                AI_MEMBLOCK_CBOINVAL_INACTIVE_BYTES) &&
            read64(src_guard_paddr) == src_guards[round] &&
            ai_memblock_cboinval_partial_masked_store_matches(
                dst_base_paddr,
                src_patterns[round],
                dst_post_inval_patterns[round],
                mask_patterns[round],
                AI_MEMBLOCK_CBOINVAL_INACTIVE_BYTES) &&
            ai_memblock_cboinval_inactive_block_tail_matches_seed(
                dst_base_paddr,
                dst_old_seeds[round],
                AI_MEMBLOCK_CBOINVAL_INACTIVE_BYTES / sizeof(uint64_t)) &&
            read64(dst_before_paddr) == dst_before_guards[round] &&
            read64(dst_after_paddr) == dst_after_guards[round] &&
            read8(mask_paddr) == mask_patterns[round][0] &&
            read8(mask_paddr + 1U) == mask_patterns[round][1] &&
            read64(mask_guard_paddr) == mask_guards[round] &&
            read64(marker_paddr) == marker_updates[round] &&
            read64(marker_guard_paddr) == marker_guards[round]
        );
    }

    ai_memblock_cboinval_inactive_reset_hs_env();

    TEST_END();
}

static inline void ai_memblock_cboinval_then_partial_masked_vload_capture_marker(
    uintptr_t inval_vaddr,
    uintptr_t seed_src_vaddr,
    uintptr_t masked_src_vaddr,
    uintptr_t mask_vaddr,
    uintptr_t capture_dst_vaddr,
    uintptr_t marker_vaddr,
    uint64_t marker_update)
{
    const int vtype = VTYPE(VSEW_FROM_BITS(8), 0);

    asm volatile(
        ".option push\n\t"
        ".option norvc\n\t"
        "vsetvl t0, %[vl], %[vtype]\n\t"
        "vle8.v v8, (%[seed_src_addr])\n\t"
        ".insn i 0x0f, 0x2, x0, %[inval_addr], 0\n\t"
        "vlm.v v0, (%[mask_addr])\n\t"
        "vle8.v v8, (%[masked_src_addr]), v0.t\n\t"
        "vse8.v v8, (%[capture_dst_addr])\n\t"
        "sd %[marker_update], 0(%[marker_addr])\n\t"
        "fence rw, rw\n\t"
        ".option pop\n\t"
        :
        : [inval_addr] "r"(inval_vaddr),
          [seed_src_addr] "r"(seed_src_vaddr),
          [masked_src_addr] "r"(masked_src_vaddr),
          [mask_addr] "r"(mask_vaddr),
          [capture_dst_addr] "r"(capture_dst_vaddr),
          [marker_addr] "r"(marker_vaddr),
          [marker_update] "r"(marker_update),
          [vl] "r"((int)AI_MEMBLOCK_CBOINVAL_INACTIVE_BYTES),
          [vtype] "r"(vtype)
        : "t0", "memory");
}

bool ai_micro_hs_cboinval_then_masked_vector_load_partial_active_no_lane_mix_or_marker_drop_corner()
{
    TEST_START();

    const enum test_page seed_page = SWITCH1;
    const enum test_page masked_page = SWITCH2;
    const enum test_page aux_page = VSRWX_GRWX;
    const uintptr_t seed_src_vaddr = hs_page_base(seed_page) + 0x340U;
    const uintptr_t seed_src_paddr = phys_page_base(seed_page) + 0x340U;
    const uintptr_t seed_guard_vaddr =
        seed_src_vaddr + AI_MEMBLOCK_CBOINVAL_INACTIVE_BYTES;
    const uintptr_t seed_guard_paddr =
        seed_src_paddr + AI_MEMBLOCK_CBOINVAL_INACTIVE_BYTES;
    const uintptr_t masked_base_vaddr = hs_page_base(masked_page) + 0x340U;
    const uintptr_t masked_base_paddr = phys_page_base(masked_page) + 0x340U;
    const uintptr_t inval_vaddr =
        masked_base_vaddr + 4U * sizeof(uint64_t);
    const uintptr_t masked_tail_vaddr =
        masked_base_vaddr +
        (AI_MEMBLOCK_CBOINVAL_INACTIVE_BLOCK_WORDS - 1U) *
        sizeof(uint64_t);
    const uintptr_t masked_before_vaddr =
        masked_base_vaddr - sizeof(uint64_t);
    const uintptr_t masked_after_vaddr =
        masked_base_vaddr + AI_MEMBLOCK_CBOINVAL_INACTIVE_BLOCK_WORDS *
        sizeof(uint64_t);
    const uintptr_t masked_before_paddr =
        masked_base_paddr - sizeof(uint64_t);
    const uintptr_t masked_after_paddr =
        masked_base_paddr + AI_MEMBLOCK_CBOINVAL_INACTIVE_BLOCK_WORDS *
        sizeof(uint64_t);
    const uintptr_t capture_dst_vaddr = hs_page_base(aux_page) + 0x340U;
    const uintptr_t capture_dst_paddr = phys_page_base(aux_page) + 0x340U;
    const uintptr_t capture_before_vaddr =
        capture_dst_vaddr - sizeof(uint64_t);
    const uintptr_t capture_after_vaddr =
        capture_dst_vaddr + AI_MEMBLOCK_CBOINVAL_INACTIVE_BYTES;
    const uintptr_t capture_before_paddr =
        capture_dst_paddr - sizeof(uint64_t);
    const uintptr_t capture_after_paddr =
        capture_dst_paddr + AI_MEMBLOCK_CBOINVAL_INACTIVE_BYTES;
    const uintptr_t mask_vaddr = hs_page_base(aux_page) + 0x3c0U;
    const uintptr_t mask_paddr = phys_page_base(aux_page) + 0x3c0U;
    const uintptr_t mask_guard_vaddr = mask_vaddr + sizeof(uint64_t);
    const uintptr_t mask_guard_paddr = mask_paddr + sizeof(uint64_t);
    const uintptr_t marker_vaddr = hs_page_base(aux_page) + 0x440U;
    const uintptr_t marker_paddr = phys_page_base(aux_page) + 0x440U;
    const uintptr_t marker_guard_vaddr = marker_vaddr + sizeof(uint64_t);
    const uintptr_t marker_guard_paddr = marker_paddr + sizeof(uint64_t);
    static const uint8_t seed_patterns[][AI_MEMBLOCK_CBOINVAL_INACTIVE_BYTES] = {
        { 0x21, 0x32, 0x43, 0x54, 0x65, 0x76, 0x87, 0x98,
          0xa9, 0xba, 0xcb, 0xdc, 0xed, 0xfe, 0x0f, 0x10 },
        { 0xef, 0xde, 0xcd, 0xbc, 0xab, 0x9a, 0x89, 0x78,
          0x67, 0x56, 0x45, 0x34, 0x23, 0x12, 0x01, 0xf0 },
        { 0x42, 0x24, 0x66, 0x33, 0x88, 0x44, 0xaa, 0x55,
          0xcc, 0x77, 0xee, 0x99, 0x00, 0xbb, 0x11, 0xdd },
    };
    static const uint8_t masked_post_inval_patterns[]
        [AI_MEMBLOCK_CBOINVAL_INACTIVE_BYTES] = {
        { 0xd1, 0xc2, 0xb3, 0xa4, 0x95, 0x86, 0x77, 0x68,
          0x59, 0x4a, 0x3b, 0x2c, 0x1d, 0x0e, 0xff, 0xe0 },
        { 0x0e, 0x1d, 0x2c, 0x3b, 0x4a, 0x59, 0x68, 0x77,
          0x86, 0x95, 0xa4, 0xb3, 0xc2, 0xd1, 0xe0, 0xff },
        { 0x93, 0x84, 0x75, 0x66, 0x57, 0x48, 0x39, 0x2a,
          0x1b, 0x0c, 0xfd, 0xee, 0xdf, 0xc0, 0xb1, 0xa2 },
    };
    static const uint8_t capture_old_patterns[]
        [AI_MEMBLOCK_CBOINVAL_INACTIVE_BYTES] = {
        { 0x5e, 0x4d, 0x3c, 0x2b, 0x1a, 0x09, 0xf8, 0xe7,
          0xd6, 0xc5, 0xb4, 0xa3, 0x92, 0x81, 0x70, 0x6f },
        { 0xa1, 0xb2, 0xc3, 0xd4, 0xe5, 0xf6, 0x07, 0x18,
          0x29, 0x3a, 0x4b, 0x5c, 0x6d, 0x7e, 0x8f, 0x90 },
        { 0xca, 0xfe, 0xba, 0xbe, 0xde, 0xad, 0xbe, 0xef,
          0xf0, 0x0d, 0xc0, 0xde, 0x13, 0x57, 0x9b, 0xdf },
    };
    static const uint8_t mask_patterns[][2] = {
        { 0x55, 0x55 },
        { 0xaa, 0xaa },
        { 0xc3, 0x3c },
    };
    const uint64_t masked_old_seeds[] = {
        0x0123456789abcdefULL,
        0xfedcba9876543210ULL,
        0x1021324354657687ULL,
    };
    const uint64_t seed_guards[] = {
        0x1122334455667788ULL,
        0x2233445566778899ULL,
        0x33445566778899aaULL,
    };
    const uint64_t masked_before_guards[] = {
        0x8877665544332211ULL,
        0x7766554433221100ULL,
        0x66554433221100ffULL,
    };
    const uint64_t masked_after_guards[] = {
        0x0102030405060708ULL,
        0x1827364554637281ULL,
        0xfedcba9876543210ULL,
    };
    const uint64_t capture_before_guards[] = {
        0xabcdef0123456789ULL,
        0xbcdef0123456789aULL,
        0xcdef0123456789abULL,
    };
    const uint64_t capture_after_guards[] = {
        0x0123456789abcdefULL,
        0x123456789abcdef0ULL,
        0x23456789abcdef01ULL,
    };
    const uint64_t mask_guards[] = {
        0xfedcba9876543210ULL,
        0xedcba9876543210fULL,
        0xdcba9876543210feULL,
    };
    const uint64_t marker_seeds[] = {
        0xa0a1a2a3a4a5a6a7ULL,
        0xb0b1b2b3b4b5b6b7ULL,
        0xc0c1c2c3c4c5c6c7ULL,
    };
    const uint64_t marker_updates[] = {
        0x3141592653589793ULL,
        0x2718281828459045ULL,
        0x1618033988749894ULL,
    };
    const uint64_t marker_guards[] = {
        0xdeadbeefcafebabeULL,
        0xfeedface0badf00dULL,
        0xfaceb00c13579bdfULL,
    };

    ai_memblock_cboinval_inactive_prepare_hs_env();

    for (size_t round = 0;
         round < AI_MEMBLOCK_CBOINVAL_INACTIVE_ARRAY_SIZE(masked_old_seeds);
         ++round) {
        const uint64_t masked_old_word1 =
            masked_old_seeds[round] + 0x1111111111111111ULL;
        const uint64_t expected_tail =
            masked_old_seeds[round] +
            (AI_MEMBLOCK_CBOINVAL_INACTIVE_BLOCK_WORDS - 1U) *
            0x1111111111111111ULL;
        const uint64_t expected_masked0 =
            ai_memblock_cboinval_inactive_bytes_to_u64(
                masked_post_inval_patterns[round], 0U);
        const uint64_t expected_masked1 =
            ai_memblock_cboinval_inactive_bytes_to_u64(
                masked_post_inval_patterns[round], sizeof(uint64_t));

        goto_priv(PRIV_M);
        ai_memblock_cboinval_inactive_write_bytes(seed_src_paddr,
            seed_patterns[round], AI_MEMBLOCK_CBOINVAL_INACTIVE_BYTES);
        write64(seed_guard_paddr, seed_guards[round]);
        ai_memblock_cboinval_inactive_seed_block(masked_base_paddr,
            masked_old_seeds[round]);
        write64(masked_before_paddr, masked_before_guards[round]);
        write64(masked_after_paddr, masked_after_guards[round]);
        ai_memblock_cboinval_inactive_write_bytes(capture_dst_paddr,
            capture_old_patterns[round],
            AI_MEMBLOCK_CBOINVAL_INACTIVE_BYTES);
        write64(capture_before_paddr, capture_before_guards[round]);
        write64(capture_after_paddr, capture_after_guards[round]);
        write8(mask_paddr, mask_patterns[round][0]);
        write8(mask_paddr + 1U, mask_patterns[round][1]);
        write64(mask_guard_paddr, mask_guards[round]);
        write64(marker_paddr, marker_seeds[round]);
        write64(marker_guard_paddr, marker_guards[round]);

        goto_priv(PRIV_HS);
        TEST_SETUP_EXCEPT();
        const uint64_t warmed_seed0 = ld(seed_src_vaddr);
        const uint64_t warmed_seed1 =
            ld(seed_src_vaddr + sizeof(uint64_t));
        const uint64_t warmed_masked0 = ld(masked_base_vaddr);
        const uint64_t warmed_masked1 =
            ld(masked_base_vaddr + sizeof(uint64_t));
        const uint64_t warmed_tail = ld(masked_tail_vaddr);
        const uint64_t warmed_marker = ld(marker_vaddr);
        AI_ASSERT_NO_EXCEPT_AND("before the CBO.INVAL plus partial-active masked vector-load owner-window starts, seed source, masked source cacheline, and marker are readable with seeded values",
            warmed_seed0 ==
                ai_memblock_cboinval_inactive_bytes_to_u64(
                    seed_patterns[round], 0U) &&
            warmed_seed1 ==
                ai_memblock_cboinval_inactive_bytes_to_u64(
                    seed_patterns[round], sizeof(uint64_t)) &&
            warmed_masked0 == masked_old_seeds[round] &&
            warmed_masked1 == masked_old_word1 &&
            warmed_tail == expected_tail &&
            warmed_marker == marker_seeds[round]
        );

        goto_priv(PRIV_M);
        ai_memblock_cboinval_inactive_write_bytes(masked_base_paddr,
            masked_post_inval_patterns[round],
            AI_MEMBLOCK_CBOINVAL_INACTIVE_BYTES);

        goto_priv(PRIV_HS);
        TEST_SETUP_EXCEPT();
        ai_memblock_cboinval_then_partial_masked_vload_capture_marker(
            inval_vaddr, seed_src_vaddr, masked_base_vaddr, mask_vaddr,
            capture_dst_vaddr, marker_vaddr, marker_updates[round]);
        const uint64_t observed_masked0 = ld(masked_base_vaddr);
        const uint64_t observed_masked1 =
            ld(masked_base_vaddr + sizeof(uint64_t));
        const uint64_t observed_tail = ld(masked_tail_vaddr);
        const uint64_t observed_marker = ld(marker_vaddr);
        AI_ASSERT_NO_EXCEPT_AND("CBO.INVAL immediately followed by partial-active masked unit-stride vector load updates only active lanes, preserves inactive seed lanes, refills masked source, and releases marker progress",
            ai_memblock_cboinval_partial_masked_store_matches(
                capture_dst_vaddr,
                masked_post_inval_patterns[round],
                seed_patterns[round],
                mask_patterns[round],
                AI_MEMBLOCK_CBOINVAL_INACTIVE_BYTES) &&
            observed_masked0 == expected_masked0 &&
            observed_masked1 == expected_masked1 &&
            observed_tail == expected_tail &&
            observed_marker == marker_updates[round]
        );

        TEST_SETUP_EXCEPT();
        AI_ASSERT_NO_EXCEPT_AND("the CBO.INVAL plus partial-active masked vector-load owner-window does not corrupt seed, masked-source, capture, mask, or marker guards",
            ld(seed_guard_vaddr) == seed_guards[round] &&
            ld(masked_before_vaddr) == masked_before_guards[round] &&
            ld(masked_after_vaddr) == masked_after_guards[round] &&
            ld(capture_before_vaddr) == capture_before_guards[round] &&
            ld(capture_after_vaddr) == capture_after_guards[round] &&
            ld(mask_guard_vaddr) == mask_guards[round] &&
            ld(marker_guard_vaddr) == marker_guards[round]
        );

        goto_priv(PRIV_M);
        TEST_ASSERT("backing memory after the CBO.INVAL plus partial-active masked vector-load window captures active post-inval bytes, inactive seed bytes, masked source, marker, and guards intact",
            ai_memblock_cboinval_inactive_bytes_match(seed_src_paddr,
                seed_patterns[round],
                AI_MEMBLOCK_CBOINVAL_INACTIVE_BYTES) &&
            read64(seed_guard_paddr) == seed_guards[round] &&
            ai_memblock_cboinval_inactive_bytes_match(masked_base_paddr,
                masked_post_inval_patterns[round],
                AI_MEMBLOCK_CBOINVAL_INACTIVE_BYTES) &&
            ai_memblock_cboinval_inactive_block_tail_matches_seed(
                masked_base_paddr,
                masked_old_seeds[round],
                AI_MEMBLOCK_CBOINVAL_INACTIVE_BYTES / sizeof(uint64_t)) &&
            read64(masked_before_paddr) == masked_before_guards[round] &&
            read64(masked_after_paddr) == masked_after_guards[round] &&
            ai_memblock_cboinval_partial_masked_store_matches(
                capture_dst_paddr,
                masked_post_inval_patterns[round],
                seed_patterns[round],
                mask_patterns[round],
                AI_MEMBLOCK_CBOINVAL_INACTIVE_BYTES) &&
            read64(capture_before_paddr) == capture_before_guards[round] &&
            read64(capture_after_paddr) == capture_after_guards[round] &&
            read8(mask_paddr) == mask_patterns[round][0] &&
            read8(mask_paddr + 1U) == mask_patterns[round][1] &&
            read64(mask_guard_paddr) == mask_guards[round] &&
            read64(marker_paddr) == marker_updates[round] &&
            read64(marker_guard_paddr) == marker_guards[round]
        );
    }

    ai_memblock_cboinval_inactive_reset_hs_env();

    TEST_END();
}

static inline void ai_memblock_partial_masked_vstore_then_cboinval_marker(
    uintptr_t src_vaddr,
    uintptr_t dst_vaddr,
    uintptr_t mask_vaddr,
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
        "vlm.v v0, (%[mask_addr])\n\t"
        "vse8.v v8, (%[dst_addr]), v0.t\n\t"
        ".insn i 0x0f, 0x2, x0, %[inval_addr], 0\n\t"
        "sd %[marker_update], 0(%[marker_addr])\n\t"
        "fence rw, rw\n\t"
        ".option pop\n\t"
        :
        : [src_addr] "r"(src_vaddr),
          [dst_addr] "r"(dst_vaddr),
          [mask_addr] "r"(mask_vaddr),
          [inval_addr] "r"(inval_vaddr),
          [marker_addr] "r"(marker_vaddr),
          [marker_update] "r"(marker_update),
          [vl] "r"((int)AI_MEMBLOCK_CBOINVAL_INACTIVE_BYTES),
          [vtype] "r"(vtype)
        : "t0", "memory");
}

bool ai_micro_hs_masked_vector_store_partial_active_then_cboinval_no_lane_mix_or_inval_drop_corner()
{
    TEST_START();

    const enum test_page src_page = SWITCH1;
    const enum test_page dst_page = SWITCH2;
    const enum test_page aux_page = VSRWX_GRWX;
    const uintptr_t src_vaddr = hs_page_base(src_page) + 0x480U;
    const uintptr_t src_paddr = phys_page_base(src_page) + 0x480U;
    const uintptr_t src_guard_vaddr =
        src_vaddr + AI_MEMBLOCK_CBOINVAL_INACTIVE_BYTES;
    const uintptr_t src_guard_paddr =
        src_paddr + AI_MEMBLOCK_CBOINVAL_INACTIVE_BYTES;
    const uintptr_t dst_base_vaddr = hs_page_base(dst_page) + 0x480U;
    const uintptr_t dst_base_paddr = phys_page_base(dst_page) + 0x480U;
    const uintptr_t inval_vaddr =
        dst_base_vaddr + 4U * sizeof(uint64_t);
    const uintptr_t dst_tail_vaddr =
        dst_base_vaddr +
        (AI_MEMBLOCK_CBOINVAL_INACTIVE_BLOCK_WORDS - 1U) *
        sizeof(uint64_t);
    const uintptr_t dst_before_vaddr =
        dst_base_vaddr - sizeof(uint64_t);
    const uintptr_t dst_after_vaddr =
        dst_base_vaddr + AI_MEMBLOCK_CBOINVAL_INACTIVE_BLOCK_WORDS *
        sizeof(uint64_t);
    const uintptr_t dst_before_paddr =
        dst_base_paddr - sizeof(uint64_t);
    const uintptr_t dst_after_paddr =
        dst_base_paddr + AI_MEMBLOCK_CBOINVAL_INACTIVE_BLOCK_WORDS *
        sizeof(uint64_t);
    const uintptr_t mask_vaddr = hs_page_base(aux_page) + 0x480U;
    const uintptr_t mask_paddr = phys_page_base(aux_page) + 0x480U;
    const uintptr_t mask_guard_vaddr = mask_vaddr + sizeof(uint64_t);
    const uintptr_t mask_guard_paddr = mask_paddr + sizeof(uint64_t);
    const uintptr_t marker_vaddr = hs_page_base(aux_page) + 0x500U;
    const uintptr_t marker_paddr = phys_page_base(aux_page) + 0x500U;
    const uintptr_t marker_guard_vaddr = marker_vaddr + sizeof(uint64_t);
    const uintptr_t marker_guard_paddr = marker_paddr + sizeof(uint64_t);
    static const uint8_t src_patterns[][AI_MEMBLOCK_CBOINVAL_INACTIVE_BYTES] = {
        { 0x86, 0x75, 0x64, 0x53, 0x42, 0x31, 0x20, 0x1f,
          0x2e, 0x3d, 0x4c, 0x5b, 0x6a, 0x79, 0x88, 0x97 },
        { 0x0b, 0x1c, 0x2d, 0x3e, 0x4f, 0x50, 0x61, 0x72,
          0x83, 0x94, 0xa5, 0xb6, 0xc7, 0xd8, 0xe9, 0xfa },
        { 0xfe, 0xdc, 0xba, 0x98, 0x76, 0x54, 0x32, 0x10,
          0x01, 0x23, 0x45, 0x67, 0x89, 0xab, 0xcd, 0xef },
    };
    static const uint8_t dst_post_inval_patterns[]
        [AI_MEMBLOCK_CBOINVAL_INACTIVE_BYTES] = {
        { 0x17, 0x28, 0x39, 0x4a, 0x5b, 0x6c, 0x7d, 0x8e,
          0x9f, 0xa0, 0xb1, 0xc2, 0xd3, 0xe4, 0xf5, 0x06 },
        { 0xf6, 0xe5, 0xd4, 0xc3, 0xb2, 0xa1, 0x90, 0x8f,
          0x7e, 0x6d, 0x5c, 0x4b, 0x3a, 0x29, 0x18, 0x07 },
        { 0x62, 0x73, 0x84, 0x95, 0xa6, 0xb7, 0xc8, 0xd9,
          0xea, 0xfb, 0x0c, 0x1d, 0x2e, 0x3f, 0x40, 0x51 },
    };
    static const uint8_t mask_patterns[][2] = {
        { 0xc3, 0x3c },
        { 0x5a, 0xa5 },
        { 0x99, 0x66 },
    };
    const uint64_t dst_old_seeds[] = {
        0x0a1b2c3d4e5f6071ULL,
        0x8192a3b4c5d6e7f8ULL,
        0x13572468ace0bdf9ULL,
    };
    const uint64_t src_guards[] = {
        0x1029384756abcdefULL,
        0x203948576abcedf1ULL,
        0x304958677abcde02ULL,
    };
    const uint64_t dst_before_guards[] = {
        0xf0e1d2c3b4a59687ULL,
        0xe1d2c3b4a5968778ULL,
        0xd2c3b4a596877869ULL,
    };
    const uint64_t dst_after_guards[] = {
        0x0123456789abcdefULL,
        0x123456789abcdef0ULL,
        0x23456789abcdef01ULL,
    };
    const uint64_t mask_guards[] = {
        0x55aa33cc77ee11ffULL,
        0x66bb44dd88ff2200ULL,
        0x77cc55ee99003311ULL,
    };
    const uint64_t marker_seeds[] = {
        0x0ddc0ffeeca5e001ULL,
        0x0ddc0ffeeca5e002ULL,
        0x0ddc0ffeeca5e003ULL,
    };
    const uint64_t marker_updates[] = {
        0x5eedfacecafeb001ULL,
        0x5eedfacecafeb002ULL,
        0x5eedfacecafeb003ULL,
    };
    const uint64_t marker_guards[] = {
        0xa5a500ffcc33ee11ULL,
        0xb6b61100dd44ff22ULL,
        0xc7c72211ee550033ULL,
    };

    ai_memblock_cboinval_inactive_prepare_hs_env();

    for (size_t round = 0;
         round < AI_MEMBLOCK_CBOINVAL_INACTIVE_ARRAY_SIZE(dst_old_seeds);
         ++round) {
        const uint64_t dst_old_word1 =
            dst_old_seeds[round] + 0x1111111111111111ULL;
        const uint64_t expected_tail =
            dst_old_seeds[round] +
            (AI_MEMBLOCK_CBOINVAL_INACTIVE_BLOCK_WORDS - 1U) *
            0x1111111111111111ULL;

        goto_priv(PRIV_M);
        ai_memblock_cboinval_inactive_write_bytes(src_paddr,
            src_patterns[round], AI_MEMBLOCK_CBOINVAL_INACTIVE_BYTES);
        write64(src_guard_paddr, src_guards[round]);
        ai_memblock_cboinval_inactive_seed_block(dst_base_paddr,
            dst_old_seeds[round]);
        write64(dst_before_paddr, dst_before_guards[round]);
        write64(dst_after_paddr, dst_after_guards[round]);
        write8(mask_paddr, mask_patterns[round][0]);
        write8(mask_paddr + 1U, mask_patterns[round][1]);
        write64(mask_guard_paddr, mask_guards[round]);
        write64(marker_paddr, marker_seeds[round]);
        write64(marker_guard_paddr, marker_guards[round]);

        goto_priv(PRIV_HS);
        TEST_SETUP_EXCEPT();
        const uint64_t warmed_src0 = ld(src_vaddr);
        const uint64_t warmed_src1 = ld(src_vaddr + sizeof(uint64_t));
        const uint64_t warmed_dst0 = ld(dst_base_vaddr);
        const uint64_t warmed_dst1 =
            ld(dst_base_vaddr + sizeof(uint64_t));
        const uint64_t warmed_tail = ld(dst_tail_vaddr);
        const uint64_t warmed_marker = ld(marker_vaddr);
        AI_ASSERT_NO_EXCEPT_AND("before the partial-active masked vector-store plus CBO.INVAL owner-window starts, source, destination cacheline, and marker are readable with seeded values",
            warmed_src0 ==
                ai_memblock_cboinval_inactive_bytes_to_u64(
                    src_patterns[round], 0U) &&
            warmed_src1 ==
                ai_memblock_cboinval_inactive_bytes_to_u64(
                    src_patterns[round], sizeof(uint64_t)) &&
            warmed_dst0 == dst_old_seeds[round] &&
            warmed_dst1 == dst_old_word1 &&
            warmed_tail == expected_tail &&
            warmed_marker == marker_seeds[round]
        );

        goto_priv(PRIV_M);
        ai_memblock_cboinval_inactive_write_bytes(dst_base_paddr,
            dst_post_inval_patterns[round],
            AI_MEMBLOCK_CBOINVAL_INACTIVE_BYTES);

        goto_priv(PRIV_HS);
        TEST_SETUP_EXCEPT();
        ai_memblock_partial_masked_vstore_then_cboinval_marker(src_vaddr,
            dst_base_vaddr, mask_vaddr, inval_vaddr, marker_vaddr,
            marker_updates[round]);
        const uint64_t observed_tail = ld(dst_tail_vaddr);
        const uint64_t observed_marker = ld(marker_vaddr);
        AI_ASSERT_NO_EXCEPT_AND("partial-active masked unit-stride vector store immediately followed by CBO.INVAL keeps active lane payload, inactive post-inval lanes, and marker progress",
            ai_memblock_cboinval_partial_masked_store_matches(
                dst_base_vaddr,
                src_patterns[round],
                dst_post_inval_patterns[round],
                mask_patterns[round],
                AI_MEMBLOCK_CBOINVAL_INACTIVE_BYTES) &&
            observed_tail == expected_tail &&
            observed_marker == marker_updates[round]
        );

        TEST_SETUP_EXCEPT();
        AI_ASSERT_NO_EXCEPT_AND("the partial-active masked vector-store plus CBO.INVAL owner-window does not corrupt source, destination, mask, or marker guards",
            ld(src_guard_vaddr) == src_guards[round] &&
            ld(dst_before_vaddr) == dst_before_guards[round] &&
            ld(dst_after_vaddr) == dst_after_guards[round] &&
            ld(mask_guard_vaddr) == mask_guards[round] &&
            ld(marker_guard_vaddr) == marker_guards[round]
        );

        goto_priv(PRIV_M);
        TEST_ASSERT("backing memory after the partial-active masked vector-store plus CBO.INVAL window contains active source bytes, inactive post-inval bytes, destination tail, marker, and guards intact",
            ai_memblock_cboinval_inactive_bytes_match(src_paddr,
                src_patterns[round],
                AI_MEMBLOCK_CBOINVAL_INACTIVE_BYTES) &&
            read64(src_guard_paddr) == src_guards[round] &&
            ai_memblock_cboinval_partial_masked_store_matches(
                dst_base_paddr,
                src_patterns[round],
                dst_post_inval_patterns[round],
                mask_patterns[round],
                AI_MEMBLOCK_CBOINVAL_INACTIVE_BYTES) &&
            ai_memblock_cboinval_inactive_block_tail_matches_seed(
                dst_base_paddr,
                dst_old_seeds[round],
                AI_MEMBLOCK_CBOINVAL_INACTIVE_BYTES / sizeof(uint64_t)) &&
            read64(dst_before_paddr) == dst_before_guards[round] &&
            read64(dst_after_paddr) == dst_after_guards[round] &&
            read8(mask_paddr) == mask_patterns[round][0] &&
            read8(mask_paddr + 1U) == mask_patterns[round][1] &&
            read64(mask_guard_paddr) == mask_guards[round] &&
            read64(marker_paddr) == marker_updates[round] &&
            read64(marker_guard_paddr) == marker_guards[round]
        );
    }

    ai_memblock_cboinval_inactive_reset_hs_env();

    TEST_END();
}

static inline void ai_memblock_partial_masked_vload_then_cboinval_capture_marker(
    uintptr_t seed_src_vaddr,
    uintptr_t masked_src_vaddr,
    uintptr_t mask_vaddr,
    uintptr_t inval_vaddr,
    uintptr_t capture_dst_vaddr,
    uintptr_t marker_vaddr,
    uint64_t marker_update)
{
    const int vtype = VTYPE(VSEW_FROM_BITS(8), 0);

    asm volatile(
        ".option push\n\t"
        ".option norvc\n\t"
        "vsetvl t0, %[vl], %[vtype]\n\t"
        "vle8.v v8, (%[seed_src_addr])\n\t"
        "vlm.v v0, (%[mask_addr])\n\t"
        "vle8.v v8, (%[masked_src_addr]), v0.t\n\t"
        ".insn i 0x0f, 0x2, x0, %[inval_addr], 0\n\t"
        "vse8.v v8, (%[capture_dst_addr])\n\t"
        "sd %[marker_update], 0(%[marker_addr])\n\t"
        "fence rw, rw\n\t"
        ".option pop\n\t"
        :
        : [seed_src_addr] "r"(seed_src_vaddr),
          [masked_src_addr] "r"(masked_src_vaddr),
          [mask_addr] "r"(mask_vaddr),
          [inval_addr] "r"(inval_vaddr),
          [capture_dst_addr] "r"(capture_dst_vaddr),
          [marker_addr] "r"(marker_vaddr),
          [marker_update] "r"(marker_update),
          [vl] "r"((int)AI_MEMBLOCK_CBOINVAL_INACTIVE_BYTES),
          [vtype] "r"(vtype)
        : "t0", "memory");
}

bool ai_micro_hs_masked_vector_load_partial_active_then_cboinval_no_lane_mix_or_inval_drop_corner()
{
    TEST_START();

    const enum test_page seed_page = SWITCH1;
    const enum test_page masked_page = SWITCH2;
    const enum test_page aux_page = VSRWX_GRWX;
    const uintptr_t seed_src_vaddr = hs_page_base(seed_page) + 0x580U;
    const uintptr_t seed_src_paddr = phys_page_base(seed_page) + 0x580U;
    const uintptr_t seed_guard_vaddr =
        seed_src_vaddr + AI_MEMBLOCK_CBOINVAL_INACTIVE_BYTES;
    const uintptr_t seed_guard_paddr =
        seed_src_paddr + AI_MEMBLOCK_CBOINVAL_INACTIVE_BYTES;
    const uintptr_t masked_base_vaddr = hs_page_base(masked_page) + 0x580U;
    const uintptr_t masked_base_paddr = phys_page_base(masked_page) + 0x580U;
    const uintptr_t inval_vaddr =
        masked_base_vaddr + 4U * sizeof(uint64_t);
    const uintptr_t masked_tail_vaddr =
        masked_base_vaddr +
        (AI_MEMBLOCK_CBOINVAL_INACTIVE_BLOCK_WORDS - 1U) *
        sizeof(uint64_t);
    const uintptr_t masked_before_vaddr =
        masked_base_vaddr - sizeof(uint64_t);
    const uintptr_t masked_after_vaddr =
        masked_base_vaddr + AI_MEMBLOCK_CBOINVAL_INACTIVE_BLOCK_WORDS *
        sizeof(uint64_t);
    const uintptr_t masked_before_paddr =
        masked_base_paddr - sizeof(uint64_t);
    const uintptr_t masked_after_paddr =
        masked_base_paddr + AI_MEMBLOCK_CBOINVAL_INACTIVE_BLOCK_WORDS *
        sizeof(uint64_t);
    const uintptr_t capture_dst_vaddr = hs_page_base(aux_page) + 0x580U;
    const uintptr_t capture_dst_paddr = phys_page_base(aux_page) + 0x580U;
    const uintptr_t capture_before_vaddr =
        capture_dst_vaddr - sizeof(uint64_t);
    const uintptr_t capture_after_vaddr =
        capture_dst_vaddr + AI_MEMBLOCK_CBOINVAL_INACTIVE_BYTES;
    const uintptr_t capture_before_paddr =
        capture_dst_paddr - sizeof(uint64_t);
    const uintptr_t capture_after_paddr =
        capture_dst_paddr + AI_MEMBLOCK_CBOINVAL_INACTIVE_BYTES;
    const uintptr_t mask_vaddr = hs_page_base(aux_page) + 0x600U;
    const uintptr_t mask_paddr = phys_page_base(aux_page) + 0x600U;
    const uintptr_t mask_guard_vaddr = mask_vaddr + sizeof(uint64_t);
    const uintptr_t mask_guard_paddr = mask_paddr + sizeof(uint64_t);
    const uintptr_t marker_vaddr = hs_page_base(aux_page) + 0x680U;
    const uintptr_t marker_paddr = phys_page_base(aux_page) + 0x680U;
    const uintptr_t marker_guard_vaddr = marker_vaddr + sizeof(uint64_t);
    const uintptr_t marker_guard_paddr = marker_paddr + sizeof(uint64_t);
    static const uint8_t seed_patterns[][AI_MEMBLOCK_CBOINVAL_INACTIVE_BYTES] = {
        { 0x14, 0x25, 0x36, 0x47, 0x58, 0x69, 0x7a, 0x8b,
          0x9c, 0xad, 0xbe, 0xcf, 0xd0, 0xe1, 0xf2, 0x03 },
        { 0xfa, 0xe9, 0xd8, 0xc7, 0xb6, 0xa5, 0x94, 0x83,
          0x72, 0x61, 0x50, 0x4f, 0x3e, 0x2d, 0x1c, 0x0b },
        { 0x31, 0x13, 0x57, 0x75, 0x9b, 0xb9, 0xdf, 0xfd,
          0xec, 0xce, 0xa8, 0x8a, 0x64, 0x46, 0x20, 0x02 },
    };
    static const uint8_t masked_source_patterns[]
        [AI_MEMBLOCK_CBOINVAL_INACTIVE_BYTES] = {
        { 0xe3, 0xd2, 0xc1, 0xb0, 0xaf, 0x9e, 0x8d, 0x7c,
          0x6b, 0x5a, 0x49, 0x38, 0x27, 0x16, 0x05, 0xf4 },
        { 0x04, 0x16, 0x28, 0x3a, 0x4c, 0x5e, 0x60, 0x72,
          0x84, 0x96, 0xa8, 0xba, 0xcc, 0xde, 0xe0, 0xf2 },
        { 0x8f, 0x70, 0x6e, 0x51, 0x4d, 0x3c, 0x2a, 0x19,
          0x07, 0xf6, 0xe4, 0xd3, 0xc1, 0xb0, 0xae, 0x9d },
    };
    static const uint8_t capture_old_patterns[]
        [AI_MEMBLOCK_CBOINVAL_INACTIVE_BYTES] = {
        { 0xa5, 0x5a, 0xb6, 0x6b, 0xc7, 0x7c, 0xd8, 0x8d,
          0xe9, 0x9e, 0xfa, 0xaf, 0x0b, 0xb0, 0x1c, 0xc1 },
        { 0xde, 0xad, 0xbe, 0xef, 0xca, 0xfe, 0xba, 0xbe,
          0x13, 0x57, 0x9b, 0xdf, 0x24, 0x68, 0xac, 0xe0 },
        { 0x6c, 0x5d, 0x4e, 0x3f, 0x20, 0x11, 0x02, 0xf3,
          0xe4, 0xd5, 0xc6, 0xb7, 0xa8, 0x99, 0x8a, 0x7b },
    };
    static const uint8_t mask_patterns[][2] = {
        { 0x3c, 0xc3 },
        { 0xa5, 0x5a },
        { 0x66, 0x99 },
    };
    const uint64_t masked_tail_seeds[] = {
        0x2233445566778899ULL,
        0x33445566778899aaULL,
        0x445566778899aabbULL,
    };
    const uint64_t seed_guards[] = {
        0x89abcdef01234567ULL,
        0x9abcdef012345678ULL,
        0xabcdef0123456789ULL,
    };
    const uint64_t masked_before_guards[] = {
        0x13579bdf2468ace0ULL,
        0x2468ace013579bdfULL,
        0x3579bdf02468ace1ULL,
    };
    const uint64_t masked_after_guards[] = {
        0xfedcba9876543210ULL,
        0xedcba9876543210fULL,
        0xdcba9876543210feULL,
    };
    const uint64_t capture_before_guards[] = {
        0x0f1e2d3c4b5a6978ULL,
        0x1e2d3c4b5a69780fULL,
        0x2d3c4b5a69780f1eULL,
    };
    const uint64_t capture_after_guards[] = {
        0x8877665544332211ULL,
        0x7766554433221100ULL,
        0x66554433221100ffULL,
    };
    const uint64_t mask_guards[] = {
        0xca11ab1edead0001ULL,
        0xca11ab1edead0002ULL,
        0xca11ab1edead0003ULL,
    };
    const uint64_t marker_seeds[] = {
        0x1123581321345589ULL,
        0x3141592653589793ULL,
        0x2718281828459045ULL,
    };
    const uint64_t marker_updates[] = {
        0x8081828384858687ULL,
        0x9091929394959697ULL,
        0xa0a1a2a3a4a5a6a7ULL,
    };
    const uint64_t marker_guards[] = {
        0x0102030405060708ULL,
        0x1827364554637281ULL,
        0xfedcba9876543210ULL,
    };

    ai_memblock_cboinval_inactive_prepare_hs_env();

    for (size_t round = 0;
         round < AI_MEMBLOCK_CBOINVAL_INACTIVE_ARRAY_SIZE(masked_tail_seeds);
         ++round) {
        const uint64_t expected_masked0 =
            ai_memblock_cboinval_inactive_bytes_to_u64(
                masked_source_patterns[round], 0U);
        const uint64_t expected_masked1 =
            ai_memblock_cboinval_inactive_bytes_to_u64(
                masked_source_patterns[round], sizeof(uint64_t));
        const uint64_t expected_tail =
            masked_tail_seeds[round] +
            (AI_MEMBLOCK_CBOINVAL_INACTIVE_BLOCK_WORDS - 1U) *
            0x1111111111111111ULL;

        goto_priv(PRIV_M);
        ai_memblock_cboinval_inactive_write_bytes(seed_src_paddr,
            seed_patterns[round], AI_MEMBLOCK_CBOINVAL_INACTIVE_BYTES);
        write64(seed_guard_paddr, seed_guards[round]);
        ai_memblock_cboinval_inactive_seed_block(masked_base_paddr,
            masked_tail_seeds[round]);
        ai_memblock_cboinval_inactive_write_bytes(masked_base_paddr,
            masked_source_patterns[round],
            AI_MEMBLOCK_CBOINVAL_INACTIVE_BYTES);
        write64(masked_before_paddr, masked_before_guards[round]);
        write64(masked_after_paddr, masked_after_guards[round]);
        ai_memblock_cboinval_inactive_write_bytes(capture_dst_paddr,
            capture_old_patterns[round],
            AI_MEMBLOCK_CBOINVAL_INACTIVE_BYTES);
        write64(capture_before_paddr, capture_before_guards[round]);
        write64(capture_after_paddr, capture_after_guards[round]);
        write8(mask_paddr, mask_patterns[round][0]);
        write8(mask_paddr + 1U, mask_patterns[round][1]);
        write64(mask_guard_paddr, mask_guards[round]);
        write64(marker_paddr, marker_seeds[round]);
        write64(marker_guard_paddr, marker_guards[round]);

        goto_priv(PRIV_HS);
        TEST_SETUP_EXCEPT();
        const uint64_t warmed_seed0 = ld(seed_src_vaddr);
        const uint64_t warmed_seed1 =
            ld(seed_src_vaddr + sizeof(uint64_t));
        const uint64_t warmed_masked0 = ld(masked_base_vaddr);
        const uint64_t warmed_masked1 =
            ld(masked_base_vaddr + sizeof(uint64_t));
        const uint64_t warmed_tail = ld(masked_tail_vaddr);
        const uint64_t warmed_marker = ld(marker_vaddr);
        AI_ASSERT_NO_EXCEPT_AND("before the partial-active masked vector-load plus CBO.INVAL owner-window starts, seed source, masked source cacheline, and marker are readable with seeded values",
            warmed_seed0 ==
                ai_memblock_cboinval_inactive_bytes_to_u64(
                    seed_patterns[round], 0U) &&
            warmed_seed1 ==
                ai_memblock_cboinval_inactive_bytes_to_u64(
                    seed_patterns[round], sizeof(uint64_t)) &&
            warmed_masked0 == expected_masked0 &&
            warmed_masked1 == expected_masked1 &&
            warmed_tail == expected_tail &&
            warmed_marker == marker_seeds[round]
        );

        TEST_SETUP_EXCEPT();
        ai_memblock_partial_masked_vload_then_cboinval_capture_marker(
            seed_src_vaddr, masked_base_vaddr, mask_vaddr, inval_vaddr,
            capture_dst_vaddr, marker_vaddr, marker_updates[round]);
        const uint64_t observed_masked0 = ld(masked_base_vaddr);
        const uint64_t observed_masked1 =
            ld(masked_base_vaddr + sizeof(uint64_t));
        const uint64_t observed_tail = ld(masked_tail_vaddr);
        const uint64_t observed_marker = ld(marker_vaddr);
        AI_ASSERT_NO_EXCEPT_AND("partial-active masked unit-stride vector load immediately followed by CBO.INVAL keeps active source lanes, inactive seed lanes, source image, and marker progress",
            ai_memblock_cboinval_partial_masked_store_matches(
                capture_dst_vaddr,
                masked_source_patterns[round],
                seed_patterns[round],
                mask_patterns[round],
                AI_MEMBLOCK_CBOINVAL_INACTIVE_BYTES) &&
            observed_masked0 == expected_masked0 &&
            observed_masked1 == expected_masked1 &&
            observed_tail == expected_tail &&
            observed_marker == marker_updates[round]
        );

        TEST_SETUP_EXCEPT();
        AI_ASSERT_NO_EXCEPT_AND("the partial-active masked vector-load plus CBO.INVAL owner-window does not corrupt seed, masked-source, capture, mask, or marker guards",
            ld(seed_guard_vaddr) == seed_guards[round] &&
            ld(masked_before_vaddr) == masked_before_guards[round] &&
            ld(masked_after_vaddr) == masked_after_guards[round] &&
            ld(capture_before_vaddr) == capture_before_guards[round] &&
            ld(capture_after_vaddr) == capture_after_guards[round] &&
            ld(mask_guard_vaddr) == mask_guards[round] &&
            ld(marker_guard_vaddr) == marker_guards[round]
        );

        goto_priv(PRIV_M);
        TEST_ASSERT("backing memory after the partial-active masked vector-load plus CBO.INVAL window captures active masked-source bytes, inactive seed bytes, marker, and guards intact",
            ai_memblock_cboinval_inactive_bytes_match(seed_src_paddr,
                seed_patterns[round],
                AI_MEMBLOCK_CBOINVAL_INACTIVE_BYTES) &&
            read64(seed_guard_paddr) == seed_guards[round] &&
            ai_memblock_cboinval_inactive_bytes_match(masked_base_paddr,
                masked_source_patterns[round],
                AI_MEMBLOCK_CBOINVAL_INACTIVE_BYTES) &&
            ai_memblock_cboinval_inactive_block_tail_matches_seed(
                masked_base_paddr,
                masked_tail_seeds[round],
                AI_MEMBLOCK_CBOINVAL_INACTIVE_BYTES / sizeof(uint64_t)) &&
            read64(masked_before_paddr) == masked_before_guards[round] &&
            read64(masked_after_paddr) == masked_after_guards[round] &&
            ai_memblock_cboinval_partial_masked_store_matches(
                capture_dst_paddr,
                masked_source_patterns[round],
                seed_patterns[round],
                mask_patterns[round],
                AI_MEMBLOCK_CBOINVAL_INACTIVE_BYTES) &&
            read64(capture_before_paddr) == capture_before_guards[round] &&
            read64(capture_after_paddr) == capture_after_guards[round] &&
            read8(mask_paddr) == mask_patterns[round][0] &&
            read8(mask_paddr + 1U) == mask_patterns[round][1] &&
            read64(mask_guard_paddr) == mask_guards[round] &&
            read64(marker_paddr) == marker_updates[round] &&
            read64(marker_guard_paddr) == marker_guards[round]
        );
    }

    ai_memblock_cboinval_inactive_reset_hs_env();

    TEST_END();
}
