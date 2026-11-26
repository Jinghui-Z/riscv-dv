#include <rvh_test.h>
#include <page_tables.h>
#include <instructions.h>
#include <stdint.h>

#define AI_ASSERT_NO_EXCEPT_AND(message, cond) \
    TEST_ASSERT(message, excpt.triggered == false && (cond))
#define AI_MEMBLOCK_CBOZERO_MASKED_ARRAY_SIZE(array) \
    (sizeof(array) / sizeof((array)[0]))
#define AI_MEMBLOCK_CBOZERO_MASKED_BLOCK_WORDS 8U
#define AI_MEMBLOCK_CBOZERO_MASKED_VECTOR_BYTES 16U

static inline void ai_memblock_cbozero_masked_prepare_hs_env(void)
{
    goto_priv(PRIV_M);
    CSRS(CSR_MENVCFG, MENVCFG_CBZE);
    hspt_init();
    sfence_vma();
}

static inline void ai_memblock_cbozero_masked_reset_hs_env(void)
{
    goto_priv(PRIV_M);
    CSRC(CSR_MENVCFG, MENVCFG_CBZE);
    hspt_init();
    sfence_vma();
}

static inline void ai_memblock_cbozero_masked_seed_block(uintptr_t paddr,
    uint64_t seed)
{
    for (size_t i = 0; i < AI_MEMBLOCK_CBOZERO_MASKED_BLOCK_WORDS; ++i) {
        write64(paddr + i * sizeof(uint64_t),
            seed + i * 0x1111111111111111ULL);
    }
}

static inline bool ai_memblock_cbozero_masked_block_all_zero(uintptr_t paddr)
{
    for (size_t i = 0; i < AI_MEMBLOCK_CBOZERO_MASKED_BLOCK_WORDS; ++i) {
        if (read64(paddr + i * sizeof(uint64_t)) != 0ULL) {
            return false;
        }
    }

    return true;
}

static inline void ai_memblock_cbozero_masked_write_bytes(uintptr_t paddr,
    const uint8_t *bytes,
    unsigned len)
{
    for (unsigned i = 0; i < len; ++i) {
        write8(paddr + i, bytes[i]);
    }
}

static inline bool ai_memblock_cbozero_masked_bytes_match(uintptr_t paddr,
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

static inline bool ai_memblock_cbozero_masked_partial_store_matches(
    uintptr_t paddr,
    const uint8_t *active_source,
    const uint8_t *mask_bytes,
    unsigned len)
{
    for (unsigned i = 0; i < len; ++i) {
        const bool active = ((mask_bytes[i / 8U] >> (i % 8U)) & 1U) != 0U;
        const uint8_t expected = active ? active_source[i] : 0U;

        if (read8(paddr + i) != expected) {
            return false;
        }
    }

    return true;
}

static inline bool ai_memblock_cbozero_masked_partial_load_capture_matches(
    uintptr_t paddr,
    const uint8_t *inactive_seed,
    const uint8_t *mask_bytes,
    unsigned len)
{
    for (unsigned i = 0; i < len; ++i) {
        const bool active = ((mask_bytes[i / 8U] >> (i % 8U)) & 1U) != 0U;
        const uint8_t expected = active ? 0U : inactive_seed[i];

        if (read8(paddr + i) != expected) {
            return false;
        }
    }

    return true;
}

static inline bool ai_memblock_cbozero_masked_partial_load_source_matches(
    uintptr_t paddr,
    const uint8_t *active_source,
    const uint8_t *inactive_seed,
    const uint8_t *mask_bytes,
    unsigned len)
{
    for (unsigned i = 0; i < len; ++i) {
        const bool active = ((mask_bytes[i / 8U] >> (i % 8U)) & 1U) != 0U;
        const uint8_t expected = active ? active_source[i] : inactive_seed[i];

        if (read8(paddr + i) != expected) {
            return false;
        }
    }

    return true;
}

static inline bool ai_memblock_cbozero_masked_tail_all_zero(uintptr_t paddr,
    size_t first_tail_word)
{
    for (size_t i = first_tail_word;
         i < AI_MEMBLOCK_CBOZERO_MASKED_BLOCK_WORDS;
         ++i) {
        if (read64(paddr + i * sizeof(uint64_t)) != 0ULL) {
            return false;
        }
    }

    return true;
}

static inline void ai_memblock_cbozero_then_all_inactive_vstore_marker(
    uintptr_t zero_vaddr,
    uintptr_t src_vaddr,
    uintptr_t dst_vaddr,
    uintptr_t marker_vaddr,
    uint64_t marker_update)
{
    const int vtype = VTYPE(VSEW_FROM_BITS(8), 0);

    asm volatile(
        ".option push\n\t"
        ".option norvc\n\t"
        ".insn i 0x0f, 0x2, x0, %[zero_addr], 4\n\t"
        "vsetvl t0, %[vl], %[vtype]\n\t"
        "vle8.v v8, (%[src_addr])\n\t"
        "vmv.v.i v0, 0\n\t"
        "vse8.v v8, (%[dst_addr]), v0.t\n\t"
        "sd %[marker_update], 0(%[marker_addr])\n\t"
        "fence rw, rw\n\t"
        ".option pop\n\t"
        :
        : [zero_addr] "r"(zero_vaddr),
          [src_addr] "r"(src_vaddr),
          [dst_addr] "r"(dst_vaddr),
          [marker_addr] "r"(marker_vaddr),
          [marker_update] "r"(marker_update),
          [vl] "r"((int)AI_MEMBLOCK_CBOZERO_MASKED_VECTOR_BYTES),
          [vtype] "r"(vtype)
        : "t0", "memory");
}

bool ai_micro_hs_cbozero_then_masked_vector_store_all_inactive_no_side_effect_or_marker_drop_corner()
{
    TEST_START();

    const enum test_page src_page = SWITCH1;
    const enum test_page dst_page = SWITCH2;
    const enum test_page marker_page = VSRWX_GRWX;
    const uintptr_t src_vaddr = hs_page_base(src_page) + 0x700U;
    const uintptr_t src_paddr = phys_page_base(src_page) + 0x700U;
    const uintptr_t src_guard_vaddr =
        src_vaddr + AI_MEMBLOCK_CBOZERO_MASKED_VECTOR_BYTES;
    const uintptr_t src_guard_paddr =
        src_paddr + AI_MEMBLOCK_CBOZERO_MASKED_VECTOR_BYTES;
    const uintptr_t dst_base_vaddr = hs_page_base(dst_page) + 0x700U;
    const uintptr_t dst_base_paddr = phys_page_base(dst_page) + 0x700U;
    const uintptr_t zero_subblock_vaddr =
        dst_base_vaddr + 3U * sizeof(uint64_t);
    const uintptr_t dst_tail_vaddr =
        dst_base_vaddr +
        (AI_MEMBLOCK_CBOZERO_MASKED_BLOCK_WORDS - 1U) *
        sizeof(uint64_t);
    const uintptr_t dst_before_vaddr =
        dst_base_vaddr - sizeof(uint64_t);
    const uintptr_t dst_after_vaddr =
        dst_base_vaddr + AI_MEMBLOCK_CBOZERO_MASKED_BLOCK_WORDS *
        sizeof(uint64_t);
    const uintptr_t dst_before_paddr =
        dst_base_paddr - sizeof(uint64_t);
    const uintptr_t dst_after_paddr =
        dst_base_paddr + AI_MEMBLOCK_CBOZERO_MASKED_BLOCK_WORDS *
        sizeof(uint64_t);
    const uintptr_t marker_vaddr = hs_page_base(marker_page) + 0x700U;
    const uintptr_t marker_paddr = phys_page_base(marker_page) + 0x700U;
    const uintptr_t marker_guard_vaddr = marker_vaddr + sizeof(uint64_t);
    const uintptr_t marker_guard_paddr = marker_paddr + sizeof(uint64_t);
    const uint64_t src_word0_values[] = {
        0x1021324354657687ULL,
        0x89abcdef01234567ULL,
        0xf0e1d2c3b4a59687ULL,
    };
    const uint64_t src_word1_values[] = {
        0x8877665544332211ULL,
        0x7766554433221100ULL,
        0x66554433221100ffULL,
    };
    const uint64_t dst_seeds[] = {
        0x0102030405060708ULL,
        0x1827364554637281ULL,
        0xfedcba9876543210ULL,
    };
    const uint64_t src_guards[] = {
        0xa1b2c3d4e5f60718ULL,
        0xb2c3d4e5f60718a1ULL,
        0xc3d4e5f60718a1b2ULL,
    };
    const uint64_t dst_before_guards[] = {
        0x13579bdf2468ace0ULL,
        0x2468ace013579bdfULL,
        0x3579bdf02468ace1ULL,
    };
    const uint64_t dst_after_guards[] = {
        0x55aa55aa33cc33ccULL,
        0x66bb66bb44dd44ddULL,
        0x77cc77cc55ee55eeULL,
    };
    const uint64_t marker_seeds[] = {
        0x3141592653589793ULL,
        0x2718281828459045ULL,
        0x1618033988749894ULL,
    };
    const uint64_t marker_updates[] = {
        0xca11000000000001ULL,
        0xca11000000000002ULL,
        0xca11000000000003ULL,
    };
    const uint64_t marker_guards[] = {
        0x0ddc0ffeeca5e001ULL,
        0x0ddc0ffeeca5e002ULL,
        0x0ddc0ffeeca5e003ULL,
    };

    ai_memblock_cbozero_masked_prepare_hs_env();

    for (size_t round = 0;
         round < AI_MEMBLOCK_CBOZERO_MASKED_ARRAY_SIZE(dst_seeds);
         ++round) {
        const uint64_t expected_tail =
            dst_seeds[round] +
            (AI_MEMBLOCK_CBOZERO_MASKED_BLOCK_WORDS - 1U) *
            0x1111111111111111ULL;

        goto_priv(PRIV_M);
        write64(src_paddr, src_word0_values[round]);
        write64(src_paddr + sizeof(uint64_t), src_word1_values[round]);
        write64(src_guard_paddr, src_guards[round]);
        ai_memblock_cbozero_masked_seed_block(dst_base_paddr,
            dst_seeds[round]);
        write64(dst_before_paddr, dst_before_guards[round]);
        write64(dst_after_paddr, dst_after_guards[round]);
        write64(marker_paddr, marker_seeds[round]);
        write64(marker_guard_paddr, marker_guards[round]);

        goto_priv(PRIV_HS);
        TEST_SETUP_EXCEPT();
        const uint64_t warmed_src0 = ld(src_vaddr);
        const uint64_t warmed_src1 = ld(src_vaddr + sizeof(uint64_t));
        const uint64_t warmed_dst0 = ld(dst_base_vaddr);
        const uint64_t warmed_tail = ld(dst_tail_vaddr);
        const uint64_t warmed_marker = ld(marker_vaddr);
        AI_ASSERT_NO_EXCEPT_AND("before the CBO.ZERO plus all-inactive masked vector-store owner-window starts, source, destination, and marker are readable with seeded values",
            warmed_src0 == src_word0_values[round] &&
            warmed_src1 == src_word1_values[round] &&
            warmed_dst0 == dst_seeds[round] &&
            warmed_tail == expected_tail &&
            warmed_marker == marker_seeds[round]
        );

        TEST_SETUP_EXCEPT();
        ai_memblock_cbozero_then_all_inactive_vstore_marker(
            zero_subblock_vaddr, src_vaddr, dst_base_vaddr, marker_vaddr,
            marker_updates[round]);
        const uint64_t observed_dst0 = ld(dst_base_vaddr);
        const uint64_t observed_dst1 =
            ld(dst_base_vaddr + sizeof(uint64_t));
        const uint64_t observed_tail = ld(dst_tail_vaddr);
        const uint64_t observed_marker = ld(marker_vaddr);
        AI_ASSERT_NO_EXCEPT_AND("CBO.ZERO immediately followed by all-inactive masked unit-stride vector store keeps the whole destination block zero and releases marker progress",
            observed_dst0 == 0ULL &&
            observed_dst1 == 0ULL &&
            observed_tail == 0ULL &&
            observed_marker == marker_updates[round]
        );

        TEST_SETUP_EXCEPT();
        AI_ASSERT_NO_EXCEPT_AND("the CBO.ZERO plus all-inactive masked vector-store owner-window does not corrupt source, destination, or marker guards",
            ld(src_guard_vaddr) == src_guards[round] &&
            ld(dst_before_vaddr) == dst_before_guards[round] &&
            ld(dst_after_vaddr) == dst_after_guards[round] &&
            ld(marker_guard_vaddr) == marker_guards[round]
        );

        goto_priv(PRIV_M);
        TEST_ASSERT("backing memory after the CBO.ZERO plus all-inactive masked vector-store window contains a fully zeroed destination block, unchanged source, marker update, and intact guards",
            read64(src_paddr) == src_word0_values[round] &&
            read64(src_paddr + sizeof(uint64_t)) ==
                src_word1_values[round] &&
            read64(src_guard_paddr) == src_guards[round] &&
            ai_memblock_cbozero_masked_block_all_zero(dst_base_paddr) &&
            read64(dst_before_paddr) == dst_before_guards[round] &&
            read64(dst_after_paddr) == dst_after_guards[round] &&
            read64(marker_paddr) == marker_updates[round] &&
            read64(marker_guard_paddr) == marker_guards[round]
        );
    }

    ai_memblock_cbozero_masked_reset_hs_env();

    TEST_END();
}

static inline void ai_memblock_cbozero_then_partial_masked_vload_capture_marker(
    uintptr_t zero_vaddr,
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
        ".insn i 0x0f, 0x2, x0, %[zero_addr], 4\n\t"
        "vsetvl t0, %[vl], %[vtype]\n\t"
        "vle8.v v8, (%[seed_src_addr])\n\t"
        "vlm.v v0, (%[mask_addr])\n\t"
        "vle8.v v8, (%[masked_src_addr]), v0.t\n\t"
        "vse8.v v8, (%[capture_dst_addr])\n\t"
        "sd %[marker_update], 0(%[marker_addr])\n\t"
        "fence rw, rw\n\t"
        ".option pop\n\t"
        :
        : [zero_addr] "r"(zero_vaddr),
          [seed_src_addr] "r"(seed_src_vaddr),
          [masked_src_addr] "r"(masked_src_vaddr),
          [mask_addr] "r"(mask_vaddr),
          [capture_dst_addr] "r"(capture_dst_vaddr),
          [marker_addr] "r"(marker_vaddr),
          [marker_update] "r"(marker_update),
          [vl] "r"((int)AI_MEMBLOCK_CBOZERO_MASKED_VECTOR_BYTES),
          [vtype] "r"(vtype)
        : "t0", "memory");
}

bool ai_micro_hs_cbozero_then_masked_vector_load_partial_active_no_lane_mix_or_marker_drop_corner()
{
    TEST_START();

    const enum test_page seed_page = SWITCH1;
    const enum test_page masked_page = SWITCH2;
    const enum test_page aux_page = VSRWX_GRWX;
    const uintptr_t seed_src_vaddr = hs_page_base(seed_page) + 0x980U;
    const uintptr_t seed_src_paddr = phys_page_base(seed_page) + 0x980U;
    const uintptr_t seed_guard_vaddr =
        seed_src_vaddr + AI_MEMBLOCK_CBOZERO_MASKED_VECTOR_BYTES;
    const uintptr_t seed_guard_paddr =
        seed_src_paddr + AI_MEMBLOCK_CBOZERO_MASKED_VECTOR_BYTES;
    const uintptr_t masked_base_vaddr = hs_page_base(masked_page) + 0x980U;
    const uintptr_t masked_base_paddr = phys_page_base(masked_page) + 0x980U;
    const uintptr_t zero_subblock_vaddr =
        masked_base_vaddr + 3U * sizeof(uint64_t);
    const uintptr_t masked_tail_vaddr =
        masked_base_vaddr +
        (AI_MEMBLOCK_CBOZERO_MASKED_BLOCK_WORDS - 1U) *
        sizeof(uint64_t);
    const uintptr_t masked_before_vaddr =
        masked_base_vaddr - sizeof(uint64_t);
    const uintptr_t masked_after_vaddr =
        masked_base_vaddr + AI_MEMBLOCK_CBOZERO_MASKED_BLOCK_WORDS *
        sizeof(uint64_t);
    const uintptr_t masked_before_paddr =
        masked_base_paddr - sizeof(uint64_t);
    const uintptr_t masked_after_paddr =
        masked_base_paddr + AI_MEMBLOCK_CBOZERO_MASKED_BLOCK_WORDS *
        sizeof(uint64_t);
    const uintptr_t capture_dst_vaddr = hs_page_base(aux_page) + 0x980U;
    const uintptr_t capture_dst_paddr = phys_page_base(aux_page) + 0x980U;
    const uintptr_t capture_before_vaddr =
        capture_dst_vaddr - sizeof(uint64_t);
    const uintptr_t capture_after_vaddr =
        capture_dst_vaddr + AI_MEMBLOCK_CBOZERO_MASKED_VECTOR_BYTES;
    const uintptr_t capture_before_paddr =
        capture_dst_paddr - sizeof(uint64_t);
    const uintptr_t capture_after_paddr =
        capture_dst_paddr + AI_MEMBLOCK_CBOZERO_MASKED_VECTOR_BYTES;
    const uintptr_t mask_vaddr = hs_page_base(aux_page) + 0xa00U;
    const uintptr_t mask_paddr = phys_page_base(aux_page) + 0xa00U;
    const uintptr_t mask_guard_vaddr = mask_vaddr + sizeof(uint64_t);
    const uintptr_t mask_guard_paddr = mask_paddr + sizeof(uint64_t);
    const uintptr_t marker_vaddr = hs_page_base(aux_page) + 0xa80U;
    const uintptr_t marker_paddr = phys_page_base(aux_page) + 0xa80U;
    const uintptr_t marker_guard_vaddr = marker_vaddr + sizeof(uint64_t);
    const uintptr_t marker_guard_paddr = marker_paddr + sizeof(uint64_t);
    static const uint8_t seed_patterns[][AI_MEMBLOCK_CBOZERO_MASKED_VECTOR_BYTES] = {
        { 0x5a, 0x6b, 0x7c, 0x8d, 0x9e, 0xaf, 0xb0, 0xc1,
          0xd2, 0xe3, 0xf4, 0x05, 0x16, 0x27, 0x38, 0x49 },
        { 0xa5, 0x94, 0x83, 0x72, 0x61, 0x50, 0x4f, 0x3e,
          0x2d, 0x1c, 0x0b, 0xfa, 0xe9, 0xd8, 0xc7, 0xb6 },
        { 0x09, 0x18, 0x27, 0x36, 0x45, 0x54, 0x63, 0x72,
          0x81, 0x90, 0xaf, 0xbe, 0xcd, 0xdc, 0xeb, 0xf0 },
    };
    static const uint8_t mask_patterns[][2] = {
        { 0x55, 0xaa },
        { 0xaa, 0x55 },
        { 0xc3, 0x3c },
    };
    const uint64_t masked_seeds[] = {
        0x89abcdef01234567ULL,
        0x76543210fedcba98ULL,
        0x1021324354657687ULL,
    };
    const uint64_t seed_guards[] = {
        0x445566778899aabbULL,
        0x5566778899aabbccULL,
        0x66778899aabbccddULL,
    };
    const uint64_t masked_before_guards[] = {
        0x0aceface12345678ULL,
        0x1bdfaced23456789ULL,
        0x2cfebabe3456789aULL,
    };
    const uint64_t masked_after_guards[] = {
        0x778899aabbccddeeULL,
        0x8899aabbccddeeffULL,
        0x99aabbccddeeff00ULL,
    };
    const uint64_t capture_fills[] = {
        0x1111222233334444ULL,
        0x5555666677778888ULL,
        0x9999aaaabbbbccccULL,
    };
    const uint64_t capture_before_guards[] = {
        0x13572468ace0bdf9ULL,
        0x24681357bdf9ace0ULL,
        0x35792468ce0abdf1ULL,
    };
    const uint64_t capture_after_guards[] = {
        0xc001c0de55aa33ccULL,
        0xd112d1ef66bb44ddULL,
        0xe223e2f077cc55eeULL,
    };
    const uint64_t mask_guards[] = {
        0xabcdef9876543210ULL,
        0xbcdefa8765432101ULL,
        0xcdefab7654321012ULL,
    };
    const uint64_t marker_seeds[] = {
        0x0101010101010101ULL,
        0x0202020202020202ULL,
        0x0303030303030303ULL,
    };
    const uint64_t marker_updates[] = {
        0xca55000000000001ULL,
        0xca55000000000002ULL,
        0xca55000000000003ULL,
    };
    const uint64_t marker_guards[] = {
        0xfeeddadac0010001ULL,
        0xfeeddadac0010002ULL,
        0xfeeddadac0010003ULL,
    };

    ai_memblock_cbozero_masked_prepare_hs_env();

    for (size_t round = 0;
         round < AI_MEMBLOCK_CBOZERO_MASKED_ARRAY_SIZE(masked_seeds);
         ++round) {
        const uint64_t expected_tail =
            masked_seeds[round] +
            (AI_MEMBLOCK_CBOZERO_MASKED_BLOCK_WORDS - 1U) *
            0x1111111111111111ULL;

        goto_priv(PRIV_M);
        ai_memblock_cbozero_masked_write_bytes(seed_src_paddr,
            seed_patterns[round], AI_MEMBLOCK_CBOZERO_MASKED_VECTOR_BYTES);
        write64(seed_guard_paddr, seed_guards[round]);
        ai_memblock_cbozero_masked_seed_block(masked_base_paddr,
            masked_seeds[round]);
        write64(masked_before_paddr, masked_before_guards[round]);
        write64(masked_after_paddr, masked_after_guards[round]);
        write64(capture_dst_paddr, capture_fills[round]);
        write64(capture_dst_paddr + sizeof(uint64_t),
            ~capture_fills[round]);
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
        const uint64_t warmed_masked0 = ld(masked_base_vaddr);
        const uint64_t warmed_tail = ld(masked_tail_vaddr);
        const uint64_t warmed_marker = ld(marker_vaddr);
        AI_ASSERT_NO_EXCEPT_AND("before the CBO.ZERO plus partial-active masked vector-load owner-window starts, seed source, masked source, and marker are readable with seeded values",
            warmed_seed0 != 0ULL &&
            warmed_masked0 == masked_seeds[round] &&
            warmed_tail == expected_tail &&
            warmed_marker == marker_seeds[round]
        );

        TEST_SETUP_EXCEPT();
        ai_memblock_cbozero_then_partial_masked_vload_capture_marker(
            zero_subblock_vaddr, seed_src_vaddr, masked_base_vaddr,
            mask_vaddr, capture_dst_vaddr, marker_vaddr,
            marker_updates[round]);
        const uint64_t observed_masked0 = ld(masked_base_vaddr);
        const uint64_t observed_tail = ld(masked_tail_vaddr);
        const uint64_t observed_marker = ld(marker_vaddr);
        AI_ASSERT_NO_EXCEPT_AND("CBO.ZERO immediately followed by partial-active masked unit-stride vector load captures zero active lanes, keeps inactive seed lanes, zeroes the source block, and releases marker progress",
            ai_memblock_cbozero_masked_partial_load_capture_matches(
                capture_dst_vaddr,
                seed_patterns[round],
                mask_patterns[round],
                AI_MEMBLOCK_CBOZERO_MASKED_VECTOR_BYTES) &&
            observed_masked0 == 0ULL &&
            observed_tail == 0ULL &&
            observed_marker == marker_updates[round]
        );

        TEST_SETUP_EXCEPT();
        AI_ASSERT_NO_EXCEPT_AND("the CBO.ZERO plus partial-active masked vector-load owner-window does not corrupt seed source, masked source, mask, capture, or marker guards",
            ld(seed_guard_vaddr) == seed_guards[round] &&
            ld(masked_before_vaddr) == masked_before_guards[round] &&
            ld(masked_after_vaddr) == masked_after_guards[round] &&
            ld(mask_guard_vaddr) == mask_guards[round] &&
            ld(capture_before_vaddr) == capture_before_guards[round] &&
            ld(capture_after_vaddr) == capture_after_guards[round] &&
            ld(marker_guard_vaddr) == marker_guards[round]
        );

        goto_priv(PRIV_M);
        TEST_ASSERT("backing memory after the CBO.ZERO plus partial-active masked vector-load window captures zero active lanes, seed inactive lanes, fully zeroes masked source, updates marker, and preserves guards",
            ai_memblock_cbozero_masked_partial_load_capture_matches(
                capture_dst_paddr,
                seed_patterns[round],
                mask_patterns[round],
                AI_MEMBLOCK_CBOZERO_MASKED_VECTOR_BYTES) &&
            ai_memblock_cbozero_masked_block_all_zero(masked_base_paddr) &&
            read64(seed_guard_paddr) == seed_guards[round] &&
            read64(masked_before_paddr) == masked_before_guards[round] &&
            read64(masked_after_paddr) == masked_after_guards[round] &&
            read8(mask_paddr) == mask_patterns[round][0] &&
            read8(mask_paddr + 1U) == mask_patterns[round][1] &&
            read64(mask_guard_paddr) == mask_guards[round] &&
            read64(capture_before_paddr) == capture_before_guards[round] &&
            read64(capture_after_paddr) == capture_after_guards[round] &&
            read64(marker_paddr) == marker_updates[round] &&
            read64(marker_guard_paddr) == marker_guards[round]
        );
    }

    ai_memblock_cbozero_masked_reset_hs_env();

    TEST_END();
}

static inline void ai_memblock_cbozero_then_partial_masked_vstore_marker(
    uintptr_t zero_vaddr,
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
        ".insn i 0x0f, 0x2, x0, %[zero_addr], 4\n\t"
        "vsetvl t0, %[vl], %[vtype]\n\t"
        "vle8.v v8, (%[src_addr])\n\t"
        "vlm.v v0, (%[mask_addr])\n\t"
        "vse8.v v8, (%[dst_addr]), v0.t\n\t"
        "sd %[marker_update], 0(%[marker_addr])\n\t"
        "fence rw, rw\n\t"
        ".option pop\n\t"
        :
        : [zero_addr] "r"(zero_vaddr),
          [src_addr] "r"(src_vaddr),
          [dst_addr] "r"(dst_vaddr),
          [mask_addr] "r"(mask_vaddr),
          [marker_addr] "r"(marker_vaddr),
          [marker_update] "r"(marker_update),
          [vl] "r"((int)AI_MEMBLOCK_CBOZERO_MASKED_VECTOR_BYTES),
          [vtype] "r"(vtype)
        : "t0", "memory");
}

bool ai_micro_hs_cbozero_then_masked_vector_store_partial_active_no_lane_mix_or_marker_drop_corner()
{
    TEST_START();

    const enum test_page src_page = SWITCH1;
    const enum test_page dst_page = SWITCH2;
    const enum test_page aux_page = VSRWX_GRWX;
    const uintptr_t src_vaddr = hs_page_base(src_page) + 0x880U;
    const uintptr_t src_paddr = phys_page_base(src_page) + 0x880U;
    const uintptr_t src_guard_vaddr =
        src_vaddr + AI_MEMBLOCK_CBOZERO_MASKED_VECTOR_BYTES;
    const uintptr_t src_guard_paddr =
        src_paddr + AI_MEMBLOCK_CBOZERO_MASKED_VECTOR_BYTES;
    const uintptr_t dst_base_vaddr = hs_page_base(dst_page) + 0x880U;
    const uintptr_t dst_base_paddr = phys_page_base(dst_page) + 0x880U;
    const uintptr_t zero_subblock_vaddr =
        dst_base_vaddr + 3U * sizeof(uint64_t);
    const uintptr_t dst_tail_vaddr =
        dst_base_vaddr +
        (AI_MEMBLOCK_CBOZERO_MASKED_BLOCK_WORDS - 1U) *
        sizeof(uint64_t);
    const uintptr_t dst_before_vaddr =
        dst_base_vaddr - sizeof(uint64_t);
    const uintptr_t dst_after_vaddr =
        dst_base_vaddr + AI_MEMBLOCK_CBOZERO_MASKED_BLOCK_WORDS *
        sizeof(uint64_t);
    const uintptr_t dst_before_paddr =
        dst_base_paddr - sizeof(uint64_t);
    const uintptr_t dst_after_paddr =
        dst_base_paddr + AI_MEMBLOCK_CBOZERO_MASKED_BLOCK_WORDS *
        sizeof(uint64_t);
    const uintptr_t mask_vaddr = hs_page_base(aux_page) + 0x880U;
    const uintptr_t mask_paddr = phys_page_base(aux_page) + 0x880U;
    const uintptr_t mask_guard_vaddr = mask_vaddr + sizeof(uint64_t);
    const uintptr_t mask_guard_paddr = mask_paddr + sizeof(uint64_t);
    const uintptr_t marker_vaddr = hs_page_base(aux_page) + 0x900U;
    const uintptr_t marker_paddr = phys_page_base(aux_page) + 0x900U;
    const uintptr_t marker_guard_vaddr = marker_vaddr + sizeof(uint64_t);
    const uintptr_t marker_guard_paddr = marker_paddr + sizeof(uint64_t);
    static const uint8_t src_patterns[][AI_MEMBLOCK_CBOZERO_MASKED_VECTOR_BYTES] = {
        { 0xf0, 0xe1, 0xd2, 0xc3, 0xb4, 0xa5, 0x96, 0x87,
          0x78, 0x69, 0x5a, 0x4b, 0x3c, 0x2d, 0x1e, 0x0f },
        { 0x13, 0x24, 0x35, 0x46, 0x57, 0x68, 0x79, 0x8a,
          0x9b, 0xac, 0xbd, 0xce, 0xdf, 0xe0, 0xf1, 0x02 },
        { 0x91, 0x82, 0x73, 0x64, 0x55, 0x46, 0x37, 0x28,
          0x19, 0x0a, 0xfb, 0xec, 0xdd, 0xce, 0xbf, 0xa0 },
    };
    static const uint8_t mask_patterns[][2] = {
        { 0x55, 0x55 },
        { 0xaa, 0xaa },
        { 0x3c, 0xc3 },
    };
    const uint64_t dst_seeds[] = {
        0x456789abcdef0123ULL,
        0xba9876543210fedcULL,
        0x0f1e2d3c4b5a6978ULL,
    };
    const uint64_t src_guards[] = {
        0xaa55aa55aa55aa55ULL,
        0xbb66bb66bb66bb66ULL,
        0xcc77cc77cc77cc77ULL,
    };
    const uint64_t dst_before_guards[] = {
        0x123456789abcdef0ULL,
        0x23456789abcdef01ULL,
        0x3456789abcdef012ULL,
    };
    const uint64_t dst_after_guards[] = {
        0x0ddc0ffeeca5e101ULL,
        0x0ddc0ffeeca5e102ULL,
        0x0ddc0ffeeca5e103ULL,
    };
    const uint64_t mask_guards[] = {
        0x1122334455667788ULL,
        0x2233445566778899ULL,
        0x33445566778899aaULL,
    };
    const uint64_t marker_seeds[] = {
        0xa0a1a2a3a4a5a6a7ULL,
        0xb0b1b2b3b4b5b6b7ULL,
        0xc0c1c2c3c4c5c6c7ULL,
    };
    const uint64_t marker_updates[] = {
        0xfaceb00c00000001ULL,
        0xfaceb00c00000002ULL,
        0xfaceb00c00000003ULL,
    };
    const uint64_t marker_guards[] = {
        0x3141592653589793ULL,
        0x2718281828459045ULL,
        0x1618033988749894ULL,
    };

    ai_memblock_cbozero_masked_prepare_hs_env();

    for (size_t round = 0;
         round < AI_MEMBLOCK_CBOZERO_MASKED_ARRAY_SIZE(dst_seeds);
         ++round) {
        const uint64_t expected_tail =
            dst_seeds[round] +
            (AI_MEMBLOCK_CBOZERO_MASKED_BLOCK_WORDS - 1U) *
            0x1111111111111111ULL;

        goto_priv(PRIV_M);
        ai_memblock_cbozero_masked_write_bytes(src_paddr,
            src_patterns[round], AI_MEMBLOCK_CBOZERO_MASKED_VECTOR_BYTES);
        write64(src_guard_paddr, src_guards[round]);
        ai_memblock_cbozero_masked_seed_block(dst_base_paddr,
            dst_seeds[round]);
        write64(dst_before_paddr, dst_before_guards[round]);
        write64(dst_after_paddr, dst_after_guards[round]);
        write8(mask_paddr, mask_patterns[round][0]);
        write8(mask_paddr + 1U, mask_patterns[round][1]);
        write64(mask_guard_paddr, mask_guards[round]);
        write64(marker_paddr, marker_seeds[round]);
        write64(marker_guard_paddr, marker_guards[round]);

        goto_priv(PRIV_HS);
        TEST_SETUP_EXCEPT();
        const uint64_t warmed_dst0 = ld(dst_base_vaddr);
        const uint64_t warmed_tail = ld(dst_tail_vaddr);
        const uint64_t warmed_marker = ld(marker_vaddr);
        AI_ASSERT_NO_EXCEPT_AND("before the CBO.ZERO plus partial-active masked vector-store owner-window starts, destination and marker are readable with seeded values",
            warmed_dst0 == dst_seeds[round] &&
            warmed_tail == expected_tail &&
            warmed_marker == marker_seeds[round]
        );

        TEST_SETUP_EXCEPT();
        ai_memblock_cbozero_then_partial_masked_vstore_marker(
            zero_subblock_vaddr, src_vaddr, dst_base_vaddr, mask_vaddr,
            marker_vaddr, marker_updates[round]);
        const uint64_t observed_tail = ld(dst_tail_vaddr);
        const uint64_t observed_marker = ld(marker_vaddr);
        AI_ASSERT_NO_EXCEPT_AND("CBO.ZERO immediately followed by partial-active masked unit-stride vector store keeps active source lanes, inactive zero lanes, zero tail, and marker progress",
            ai_memblock_cbozero_masked_partial_store_matches(
                dst_base_vaddr,
                src_patterns[round],
                mask_patterns[round],
                AI_MEMBLOCK_CBOZERO_MASKED_VECTOR_BYTES) &&
            observed_tail == 0ULL &&
            observed_marker == marker_updates[round]
        );

        TEST_SETUP_EXCEPT();
        AI_ASSERT_NO_EXCEPT_AND("the CBO.ZERO plus partial-active masked vector-store owner-window does not corrupt source, destination, mask, or marker guards",
            ld(src_guard_vaddr) == src_guards[round] &&
            ld(dst_before_vaddr) == dst_before_guards[round] &&
            ld(dst_after_vaddr) == dst_after_guards[round] &&
            ld(mask_guard_vaddr) == mask_guards[round] &&
            ld(marker_guard_vaddr) == marker_guards[round]
        );

        goto_priv(PRIV_M);
        TEST_ASSERT("backing memory after the CBO.ZERO plus partial-active masked vector-store window contains active source bytes, inactive zero bytes, zero tail, marker update, and intact guards",
            ai_memblock_cbozero_masked_partial_store_matches(
                dst_base_paddr,
                src_patterns[round],
                mask_patterns[round],
                AI_MEMBLOCK_CBOZERO_MASKED_VECTOR_BYTES) &&
            ai_memblock_cbozero_masked_tail_all_zero(dst_base_paddr,
                AI_MEMBLOCK_CBOZERO_MASKED_VECTOR_BYTES / sizeof(uint64_t)) &&
            read64(src_guard_paddr) == src_guards[round] &&
            read64(dst_before_paddr) == dst_before_guards[round] &&
            read64(dst_after_paddr) == dst_after_guards[round] &&
            read8(mask_paddr) == mask_patterns[round][0] &&
            read8(mask_paddr + 1U) == mask_patterns[round][1] &&
            read64(mask_guard_paddr) == mask_guards[round] &&
            read64(marker_paddr) == marker_updates[round] &&
            read64(marker_guard_paddr) == marker_guards[round]
        );
    }

    ai_memblock_cbozero_masked_reset_hs_env();

    TEST_END();
}

static inline void ai_memblock_partial_masked_vstore_then_cbozero_marker(
    uintptr_t src_vaddr,
    uintptr_t dst_vaddr,
    uintptr_t mask_vaddr,
    uintptr_t zero_vaddr,
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
        ".insn i 0x0f, 0x2, x0, %[zero_addr], 4\n\t"
        "sd %[marker_update], 0(%[marker_addr])\n\t"
        "fence rw, rw\n\t"
        ".option pop\n\t"
        :
        : [src_addr] "r"(src_vaddr),
          [dst_addr] "r"(dst_vaddr),
          [mask_addr] "r"(mask_vaddr),
          [zero_addr] "r"(zero_vaddr),
          [marker_addr] "r"(marker_vaddr),
          [marker_update] "r"(marker_update),
          [vl] "r"((int)AI_MEMBLOCK_CBOZERO_MASKED_VECTOR_BYTES),
          [vtype] "r"(vtype)
        : "t0", "memory");
}

bool ai_micro_hs_masked_vector_store_partial_active_then_cbozero_no_lane_mix_or_zero_drop_corner()
{
    TEST_START();

    const enum test_page src_page = SWITCH1;
    const enum test_page dst_page = SWITCH2;
    const enum test_page aux_page = VSRWX_GRWX;
    const uintptr_t src_vaddr = hs_page_base(src_page) + 0xb00U;
    const uintptr_t src_paddr = phys_page_base(src_page) + 0xb00U;
    const uintptr_t src_guard_vaddr =
        src_vaddr + AI_MEMBLOCK_CBOZERO_MASKED_VECTOR_BYTES;
    const uintptr_t src_guard_paddr =
        src_paddr + AI_MEMBLOCK_CBOZERO_MASKED_VECTOR_BYTES;
    const uintptr_t dst_base_vaddr = hs_page_base(dst_page) + 0xb00U;
    const uintptr_t dst_base_paddr = phys_page_base(dst_page) + 0xb00U;
    const uintptr_t zero_subblock_vaddr =
        dst_base_vaddr + 3U * sizeof(uint64_t);
    const uintptr_t dst_tail_vaddr =
        dst_base_vaddr +
        (AI_MEMBLOCK_CBOZERO_MASKED_BLOCK_WORDS - 1U) *
        sizeof(uint64_t);
    const uintptr_t dst_before_vaddr =
        dst_base_vaddr - sizeof(uint64_t);
    const uintptr_t dst_after_vaddr =
        dst_base_vaddr + AI_MEMBLOCK_CBOZERO_MASKED_BLOCK_WORDS *
        sizeof(uint64_t);
    const uintptr_t dst_before_paddr =
        dst_base_paddr - sizeof(uint64_t);
    const uintptr_t dst_after_paddr =
        dst_base_paddr + AI_MEMBLOCK_CBOZERO_MASKED_BLOCK_WORDS *
        sizeof(uint64_t);
    const uintptr_t mask_vaddr = hs_page_base(aux_page) + 0xb00U;
    const uintptr_t mask_paddr = phys_page_base(aux_page) + 0xb00U;
    const uintptr_t mask_guard_vaddr = mask_vaddr + sizeof(uint64_t);
    const uintptr_t mask_guard_paddr = mask_paddr + sizeof(uint64_t);
    const uintptr_t marker_vaddr = hs_page_base(aux_page) + 0xb80U;
    const uintptr_t marker_paddr = phys_page_base(aux_page) + 0xb80U;
    const uintptr_t marker_guard_vaddr = marker_vaddr + sizeof(uint64_t);
    const uintptr_t marker_guard_paddr = marker_paddr + sizeof(uint64_t);
    static const uint8_t src_patterns[][AI_MEMBLOCK_CBOZERO_MASKED_VECTOR_BYTES] = {
        { 0x21, 0x32, 0x43, 0x54, 0x65, 0x76, 0x87, 0x98,
          0xa9, 0xba, 0xcb, 0xdc, 0xed, 0xfe, 0x0f, 0x10 },
        { 0x10, 0x0f, 0xfe, 0xed, 0xdc, 0xcb, 0xba, 0xa9,
          0x98, 0x87, 0x76, 0x65, 0x54, 0x43, 0x32, 0x21 },
        { 0x3d, 0x4c, 0x5b, 0x6a, 0x79, 0x88, 0x97, 0xa6,
          0xb5, 0xc4, 0xd3, 0xe2, 0xf1, 0x00, 0x1f, 0x2e },
    };
    static const uint8_t mask_patterns[][2] = {
        { 0x0f, 0xf0 },
        { 0xf0, 0x0f },
        { 0x66, 0x99 },
    };
    const uint64_t dst_seeds[] = {
        0xabcdef0123456789ULL,
        0x0123456789abcdefULL,
        0x55aa55aa33cc33ccULL,
    };
    const uint64_t src_guards[] = {
        0x7071727374757677ULL,
        0x8081828384858687ULL,
        0x9091929394959697ULL,
    };
    const uint64_t dst_before_guards[] = {
        0x1111333355557777ULL,
        0x2222444466668888ULL,
        0x3333555577779999ULL,
    };
    const uint64_t dst_after_guards[] = {
        0xaaaabbbbccccddddULL,
        0xbbbbccccddddeeeeULL,
        0xccccddddeeeeffffULL,
    };
    const uint64_t mask_guards[] = {
        0x1a2b3c4d5e6f7081ULL,
        0x2b3c4d5e6f708192ULL,
        0x3c4d5e6f708192a3ULL,
    };
    const uint64_t marker_seeds[] = {
        0x0bad000000000001ULL,
        0x0bad000000000002ULL,
        0x0bad000000000003ULL,
    };
    const uint64_t marker_updates[] = {
        0xc0de000000000001ULL,
        0xc0de000000000002ULL,
        0xc0de000000000003ULL,
    };
    const uint64_t marker_guards[] = {
        0xdad1000000000001ULL,
        0xdad1000000000002ULL,
        0xdad1000000000003ULL,
    };

    ai_memblock_cbozero_masked_prepare_hs_env();

    for (size_t round = 0;
         round < AI_MEMBLOCK_CBOZERO_MASKED_ARRAY_SIZE(dst_seeds);
         ++round) {
        const uint64_t expected_tail =
            dst_seeds[round] +
            (AI_MEMBLOCK_CBOZERO_MASKED_BLOCK_WORDS - 1U) *
            0x1111111111111111ULL;

        goto_priv(PRIV_M);
        ai_memblock_cbozero_masked_write_bytes(src_paddr,
            src_patterns[round], AI_MEMBLOCK_CBOZERO_MASKED_VECTOR_BYTES);
        write64(src_guard_paddr, src_guards[round]);
        ai_memblock_cbozero_masked_seed_block(dst_base_paddr,
            dst_seeds[round]);
        write64(dst_before_paddr, dst_before_guards[round]);
        write64(dst_after_paddr, dst_after_guards[round]);
        write8(mask_paddr, mask_patterns[round][0]);
        write8(mask_paddr + 1U, mask_patterns[round][1]);
        write64(mask_guard_paddr, mask_guards[round]);
        write64(marker_paddr, marker_seeds[round]);
        write64(marker_guard_paddr, marker_guards[round]);

        goto_priv(PRIV_HS);
        TEST_SETUP_EXCEPT();
        const uint64_t warmed_dst0 = ld(dst_base_vaddr);
        const uint64_t warmed_tail = ld(dst_tail_vaddr);
        const uint64_t warmed_marker = ld(marker_vaddr);
        AI_ASSERT_NO_EXCEPT_AND("before the partial-active masked vector-store plus CBO.ZERO owner-window starts, destination and marker are readable with seeded values",
            warmed_dst0 == dst_seeds[round] &&
            warmed_tail == expected_tail &&
            warmed_marker == marker_seeds[round]
        );

        TEST_SETUP_EXCEPT();
        ai_memblock_partial_masked_vstore_then_cbozero_marker(
            src_vaddr, dst_base_vaddr, mask_vaddr, zero_subblock_vaddr,
            marker_vaddr, marker_updates[round]);
        const uint64_t observed_dst0 = ld(dst_base_vaddr);
        const uint64_t observed_dst1 =
            ld(dst_base_vaddr + sizeof(uint64_t));
        const uint64_t observed_tail = ld(dst_tail_vaddr);
        const uint64_t observed_marker = ld(marker_vaddr);
        AI_ASSERT_NO_EXCEPT_AND("partial-active masked unit-stride vector store immediately followed by CBO.ZERO leaves the destination block fully zero and releases marker progress",
            observed_dst0 == 0ULL &&
            observed_dst1 == 0ULL &&
            observed_tail == 0ULL &&
            observed_marker == marker_updates[round]
        );

        TEST_SETUP_EXCEPT();
        AI_ASSERT_NO_EXCEPT_AND("the partial-active masked vector-store plus CBO.ZERO owner-window does not corrupt source, destination, mask, or marker guards",
            ld(src_guard_vaddr) == src_guards[round] &&
            ld(dst_before_vaddr) == dst_before_guards[round] &&
            ld(dst_after_vaddr) == dst_after_guards[round] &&
            ld(mask_guard_vaddr) == mask_guards[round] &&
            ld(marker_guard_vaddr) == marker_guards[round]
        );

        goto_priv(PRIV_M);
        TEST_ASSERT("backing memory after the partial-active masked vector-store plus CBO.ZERO window keeps source and mask intact, fully zeroes destination, updates marker, and preserves guards",
            ai_memblock_cbozero_masked_bytes_match(src_paddr,
                src_patterns[round],
                AI_MEMBLOCK_CBOZERO_MASKED_VECTOR_BYTES) &&
            ai_memblock_cbozero_masked_block_all_zero(dst_base_paddr) &&
            read64(src_guard_paddr) == src_guards[round] &&
            read64(dst_before_paddr) == dst_before_guards[round] &&
            read64(dst_after_paddr) == dst_after_guards[round] &&
            read8(mask_paddr) == mask_patterns[round][0] &&
            read8(mask_paddr + 1U) == mask_patterns[round][1] &&
            read64(mask_guard_paddr) == mask_guards[round] &&
            read64(marker_paddr) == marker_updates[round] &&
            read64(marker_guard_paddr) == marker_guards[round]
        );
    }

    ai_memblock_cbozero_masked_reset_hs_env();

    TEST_END();
}

static inline void ai_memblock_all_inactive_vstore_then_cbozero_marker(
    uintptr_t src_vaddr,
    uintptr_t dst_vaddr,
    uintptr_t zero_vaddr,
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
        ".insn i 0x0f, 0x2, x0, %[zero_addr], 4\n\t"
        "sd %[marker_update], 0(%[marker_addr])\n\t"
        "fence rw, rw\n\t"
        ".option pop\n\t"
        :
        : [src_addr] "r"(src_vaddr),
          [dst_addr] "r"(dst_vaddr),
          [zero_addr] "r"(zero_vaddr),
          [marker_addr] "r"(marker_vaddr),
          [marker_update] "r"(marker_update),
          [vl] "r"((int)AI_MEMBLOCK_CBOZERO_MASKED_VECTOR_BYTES),
          [vtype] "r"(vtype)
        : "t0", "memory");
}

bool ai_micro_hs_masked_vector_store_all_inactive_then_cbozero_no_side_effect_or_zero_drop_corner()
{
    TEST_START();

    const enum test_page src_page = SWITCH1;
    const enum test_page dst_page = SWITCH2;
    const enum test_page marker_page = VSRWX_GRWX;
    const uintptr_t src_vaddr = hs_page_base(src_page) + 0xd80U;
    const uintptr_t src_paddr = phys_page_base(src_page) + 0xd80U;
    const uintptr_t src_guard_vaddr =
        src_vaddr + AI_MEMBLOCK_CBOZERO_MASKED_VECTOR_BYTES;
    const uintptr_t src_guard_paddr =
        src_paddr + AI_MEMBLOCK_CBOZERO_MASKED_VECTOR_BYTES;
    const uintptr_t dst_base_vaddr = hs_page_base(dst_page) + 0xd80U;
    const uintptr_t dst_base_paddr = phys_page_base(dst_page) + 0xd80U;
    const uintptr_t zero_subblock_vaddr =
        dst_base_vaddr + 3U * sizeof(uint64_t);
    const uintptr_t dst_tail_vaddr =
        dst_base_vaddr +
        (AI_MEMBLOCK_CBOZERO_MASKED_BLOCK_WORDS - 1U) *
        sizeof(uint64_t);
    const uintptr_t dst_before_vaddr =
        dst_base_vaddr - sizeof(uint64_t);
    const uintptr_t dst_after_vaddr =
        dst_base_vaddr + AI_MEMBLOCK_CBOZERO_MASKED_BLOCK_WORDS *
        sizeof(uint64_t);
    const uintptr_t dst_before_paddr =
        dst_base_paddr - sizeof(uint64_t);
    const uintptr_t dst_after_paddr =
        dst_base_paddr + AI_MEMBLOCK_CBOZERO_MASKED_BLOCK_WORDS *
        sizeof(uint64_t);
    const uintptr_t marker_vaddr = hs_page_base(marker_page) + 0xd80U;
    const uintptr_t marker_paddr = phys_page_base(marker_page) + 0xd80U;
    const uintptr_t marker_guard_vaddr = marker_vaddr + sizeof(uint64_t);
    const uintptr_t marker_guard_paddr = marker_paddr + sizeof(uint64_t);
    static const uint8_t src_patterns[][AI_MEMBLOCK_CBOZERO_MASKED_VECTOR_BYTES] = {
        { 0x31, 0x42, 0x53, 0x64, 0x75, 0x86, 0x97, 0xa8,
          0xb9, 0xca, 0xdb, 0xec, 0xfd, 0x0e, 0x1f, 0x20 },
        { 0x20, 0x1f, 0x0e, 0xfd, 0xec, 0xdb, 0xca, 0xb9,
          0xa8, 0x97, 0x86, 0x75, 0x64, 0x53, 0x42, 0x31 },
        { 0x4a, 0x5b, 0x6c, 0x7d, 0x8e, 0x9f, 0xa0, 0xb1,
          0xc2, 0xd3, 0xe4, 0xf5, 0x06, 0x17, 0x28, 0x39 },
    };
    const uint64_t dst_seeds[] = {
        0x0011223344556677ULL,
        0x8899aabbccddeeffULL,
        0x13579bdf2468ace0ULL,
    };
    const uint64_t src_guards[] = {
        0x01030507090b0d0fULL,
        0x11131517191b1d1fULL,
        0x21232527292b2d2fULL,
    };
    const uint64_t dst_before_guards[] = {
        0xf001f002f003f004ULL,
        0xf101f102f103f104ULL,
        0xf201f202f203f204ULL,
    };
    const uint64_t dst_after_guards[] = {
        0x0a0b0c0d0e0f1011ULL,
        0x1a1b1c1d1e1f2021ULL,
        0x2a2b2c2d2e2f3031ULL,
    };
    const uint64_t marker_seeds[] = {
        0x5152535455565758ULL,
        0x6162636465666768ULL,
        0x7172737475767778ULL,
    };
    const uint64_t marker_updates[] = {
        0xdecafbad00000001ULL,
        0xdecafbad00000002ULL,
        0xdecafbad00000003ULL,
    };
    const uint64_t marker_guards[] = {
        0x8182838485868788ULL,
        0x9192939495969798ULL,
        0xa1a2a3a4a5a6a7a8ULL,
    };

    ai_memblock_cbozero_masked_prepare_hs_env();

    for (size_t round = 0;
         round < AI_MEMBLOCK_CBOZERO_MASKED_ARRAY_SIZE(dst_seeds);
         ++round) {
        const uint64_t expected_tail =
            dst_seeds[round] +
            (AI_MEMBLOCK_CBOZERO_MASKED_BLOCK_WORDS - 1U) *
            0x1111111111111111ULL;

        goto_priv(PRIV_M);
        ai_memblock_cbozero_masked_write_bytes(src_paddr,
            src_patterns[round], AI_MEMBLOCK_CBOZERO_MASKED_VECTOR_BYTES);
        write64(src_guard_paddr, src_guards[round]);
        ai_memblock_cbozero_masked_seed_block(dst_base_paddr,
            dst_seeds[round]);
        write64(dst_before_paddr, dst_before_guards[round]);
        write64(dst_after_paddr, dst_after_guards[round]);
        write64(marker_paddr, marker_seeds[round]);
        write64(marker_guard_paddr, marker_guards[round]);

        goto_priv(PRIV_HS);
        TEST_SETUP_EXCEPT();
        const uint64_t warmed_dst0 = ld(dst_base_vaddr);
        const uint64_t warmed_tail = ld(dst_tail_vaddr);
        const uint64_t warmed_marker = ld(marker_vaddr);
        AI_ASSERT_NO_EXCEPT_AND("before the all-inactive masked vector-store plus CBO.ZERO owner-window starts, destination and marker are readable with seeded values",
            warmed_dst0 == dst_seeds[round] &&
            warmed_tail == expected_tail &&
            warmed_marker == marker_seeds[round]
        );

        TEST_SETUP_EXCEPT();
        ai_memblock_all_inactive_vstore_then_cbozero_marker(
            src_vaddr, dst_base_vaddr, zero_subblock_vaddr, marker_vaddr,
            marker_updates[round]);
        const uint64_t observed_dst0 = ld(dst_base_vaddr);
        const uint64_t observed_dst1 =
            ld(dst_base_vaddr + sizeof(uint64_t));
        const uint64_t observed_tail = ld(dst_tail_vaddr);
        const uint64_t observed_marker = ld(marker_vaddr);
        AI_ASSERT_NO_EXCEPT_AND("all-inactive masked unit-stride vector store immediately followed by CBO.ZERO leaves the destination block fully zero and releases marker progress",
            observed_dst0 == 0ULL &&
            observed_dst1 == 0ULL &&
            observed_tail == 0ULL &&
            observed_marker == marker_updates[round]
        );

        TEST_SETUP_EXCEPT();
        AI_ASSERT_NO_EXCEPT_AND("the all-inactive masked vector-store plus CBO.ZERO owner-window does not corrupt source, destination, or marker guards",
            ld(src_guard_vaddr) == src_guards[round] &&
            ld(dst_before_vaddr) == dst_before_guards[round] &&
            ld(dst_after_vaddr) == dst_after_guards[round] &&
            ld(marker_guard_vaddr) == marker_guards[round]
        );

        goto_priv(PRIV_M);
        TEST_ASSERT("backing memory after the all-inactive masked vector-store plus CBO.ZERO window keeps source intact, fully zeroes destination, updates marker, and preserves guards",
            ai_memblock_cbozero_masked_bytes_match(src_paddr,
                src_patterns[round],
                AI_MEMBLOCK_CBOZERO_MASKED_VECTOR_BYTES) &&
            ai_memblock_cbozero_masked_block_all_zero(dst_base_paddr) &&
            read64(src_guard_paddr) == src_guards[round] &&
            read64(dst_before_paddr) == dst_before_guards[round] &&
            read64(dst_after_paddr) == dst_after_guards[round] &&
            read64(marker_paddr) == marker_updates[round] &&
            read64(marker_guard_paddr) == marker_guards[round]
        );
    }

    ai_memblock_cbozero_masked_reset_hs_env();

    TEST_END();
}

static inline void ai_memblock_partial_masked_vload_then_cbozero_marker(
    uintptr_t seed_src_vaddr,
    uintptr_t masked_src_vaddr,
    uintptr_t mask_vaddr,
    uintptr_t capture_dst_vaddr,
    uintptr_t zero_vaddr,
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
        "vse8.v v8, (%[capture_dst_addr])\n\t"
        ".insn i 0x0f, 0x2, x0, %[zero_addr], 4\n\t"
        "sd %[marker_update], 0(%[marker_addr])\n\t"
        "fence rw, rw\n\t"
        ".option pop\n\t"
        :
        : [seed_src_addr] "r"(seed_src_vaddr),
          [masked_src_addr] "r"(masked_src_vaddr),
          [mask_addr] "r"(mask_vaddr),
          [capture_dst_addr] "r"(capture_dst_vaddr),
          [zero_addr] "r"(zero_vaddr),
          [marker_addr] "r"(marker_vaddr),
          [marker_update] "r"(marker_update),
          [vl] "r"((int)AI_MEMBLOCK_CBOZERO_MASKED_VECTOR_BYTES),
          [vtype] "r"(vtype)
        : "t0", "memory");
}

bool ai_micro_hs_masked_vector_load_partial_active_then_cbozero_no_lane_mix_or_zero_drop_corner()
{
    TEST_START();

    const enum test_page seed_page = SWITCH1;
    const enum test_page masked_page = SWITCH2;
    const enum test_page aux_page = VSRWX_GRWX;
    const uintptr_t seed_src_vaddr = hs_page_base(seed_page) + 0xc00U;
    const uintptr_t seed_src_paddr = phys_page_base(seed_page) + 0xc00U;
    const uintptr_t seed_guard_vaddr =
        seed_src_vaddr + AI_MEMBLOCK_CBOZERO_MASKED_VECTOR_BYTES;
    const uintptr_t seed_guard_paddr =
        seed_src_paddr + AI_MEMBLOCK_CBOZERO_MASKED_VECTOR_BYTES;
    const uintptr_t masked_base_vaddr = hs_page_base(masked_page) + 0xc00U;
    const uintptr_t masked_base_paddr = phys_page_base(masked_page) + 0xc00U;
    const uintptr_t zero_subblock_vaddr =
        masked_base_vaddr + 3U * sizeof(uint64_t);
    const uintptr_t masked_tail_vaddr =
        masked_base_vaddr +
        (AI_MEMBLOCK_CBOZERO_MASKED_BLOCK_WORDS - 1U) *
        sizeof(uint64_t);
    const uintptr_t masked_before_vaddr =
        masked_base_vaddr - sizeof(uint64_t);
    const uintptr_t masked_after_vaddr =
        masked_base_vaddr + AI_MEMBLOCK_CBOZERO_MASKED_BLOCK_WORDS *
        sizeof(uint64_t);
    const uintptr_t masked_before_paddr =
        masked_base_paddr - sizeof(uint64_t);
    const uintptr_t masked_after_paddr =
        masked_base_paddr + AI_MEMBLOCK_CBOZERO_MASKED_BLOCK_WORDS *
        sizeof(uint64_t);
    const uintptr_t capture_dst_vaddr = hs_page_base(aux_page) + 0xc00U;
    const uintptr_t capture_dst_paddr = phys_page_base(aux_page) + 0xc00U;
    const uintptr_t capture_before_vaddr =
        capture_dst_vaddr - sizeof(uint64_t);
    const uintptr_t capture_after_vaddr =
        capture_dst_vaddr + AI_MEMBLOCK_CBOZERO_MASKED_VECTOR_BYTES;
    const uintptr_t capture_before_paddr =
        capture_dst_paddr - sizeof(uint64_t);
    const uintptr_t capture_after_paddr =
        capture_dst_paddr + AI_MEMBLOCK_CBOZERO_MASKED_VECTOR_BYTES;
    const uintptr_t mask_vaddr = hs_page_base(aux_page) + 0xc80U;
    const uintptr_t mask_paddr = phys_page_base(aux_page) + 0xc80U;
    const uintptr_t mask_guard_vaddr = mask_vaddr + sizeof(uint64_t);
    const uintptr_t mask_guard_paddr = mask_paddr + sizeof(uint64_t);
    const uintptr_t marker_vaddr = hs_page_base(aux_page) + 0xd00U;
    const uintptr_t marker_paddr = phys_page_base(aux_page) + 0xd00U;
    const uintptr_t marker_guard_vaddr = marker_vaddr + sizeof(uint64_t);
    const uintptr_t marker_guard_paddr = marker_paddr + sizeof(uint64_t);
    static const uint8_t seed_patterns[][AI_MEMBLOCK_CBOZERO_MASKED_VECTOR_BYTES] = {
        { 0xee, 0xdd, 0xcc, 0xbb, 0xaa, 0x99, 0x88, 0x77,
          0x66, 0x55, 0x44, 0x33, 0x22, 0x11, 0x00, 0xff },
        { 0x17, 0x26, 0x35, 0x44, 0x53, 0x62, 0x71, 0x80,
          0x8f, 0x9e, 0xad, 0xbc, 0xcb, 0xda, 0xe9, 0xf8 },
        { 0xf8, 0xe9, 0xda, 0xcb, 0xbc, 0xad, 0x9e, 0x8f,
          0x80, 0x71, 0x62, 0x53, 0x44, 0x35, 0x26, 0x17 },
    };
    static const uint8_t masked_patterns[][AI_MEMBLOCK_CBOZERO_MASKED_VECTOR_BYTES] = {
        { 0x01, 0x12, 0x23, 0x34, 0x45, 0x56, 0x67, 0x78,
          0x89, 0x9a, 0xab, 0xbc, 0xcd, 0xde, 0xef, 0xf0 },
        { 0xf0, 0xef, 0xde, 0xcd, 0xbc, 0xab, 0x9a, 0x89,
          0x78, 0x67, 0x56, 0x45, 0x34, 0x23, 0x12, 0x01 },
        { 0x42, 0x53, 0x64, 0x75, 0x86, 0x97, 0xa8, 0xb9,
          0xca, 0xdb, 0xec, 0xfd, 0x0e, 0x1f, 0x20, 0x31 },
    };
    static const uint8_t mask_patterns[][2] = {
        { 0x0f, 0x3c },
        { 0xf0, 0xc3 },
        { 0x99, 0x66 },
    };
    const uint64_t masked_seeds[] = {
        0x0102030405060708ULL,
        0x8877665544332211ULL,
        0xfedcba9876543210ULL,
    };
    const uint64_t seed_guards[] = {
        0xaabbccddeeff0011ULL,
        0xbbccddeeff001122ULL,
        0xccddeeff00112233ULL,
    };
    const uint64_t masked_before_guards[] = {
        0x1234432112344321ULL,
        0x2345543223455432ULL,
        0x3456654334566543ULL,
    };
    const uint64_t masked_after_guards[] = {
        0x4567765445677654ULL,
        0x5678876556788765ULL,
        0x6789987667899876ULL,
    };
    const uint64_t capture_fills[] = {
        0x0f0f0f0ff0f0f0f0ULL,
        0x1e1e1e1ee1e1e1e1ULL,
        0x2d2d2d2dd2d2d2d2ULL,
    };
    const uint64_t capture_before_guards[] = {
        0x55aa55aa00ff00ffULL,
        0x66bb66bb11ee11eeULL,
        0x77cc77cc22dd22ddULL,
    };
    const uint64_t capture_after_guards[] = {
        0x8899aabbccddeeffULL,
        0x99aabbccddeeff00ULL,
        0xaabbccddeeff0011ULL,
    };
    const uint64_t mask_guards[] = {
        0xface000000000001ULL,
        0xface000000000002ULL,
        0xface000000000003ULL,
    };
    const uint64_t marker_seeds[] = {
        0xbadd000000000001ULL,
        0xbadd000000000002ULL,
        0xbadd000000000003ULL,
    };
    const uint64_t marker_updates[] = {
        0x1ead000000000001ULL,
        0x1ead000000000002ULL,
        0x1ead000000000003ULL,
    };
    const uint64_t marker_guards[] = {
        0x600d000000000001ULL,
        0x600d000000000002ULL,
        0x600d000000000003ULL,
    };

    ai_memblock_cbozero_masked_prepare_hs_env();

    for (size_t round = 0;
         round < AI_MEMBLOCK_CBOZERO_MASKED_ARRAY_SIZE(masked_seeds);
         ++round) {
        const uint64_t expected_tail =
            masked_seeds[round] +
            (AI_MEMBLOCK_CBOZERO_MASKED_BLOCK_WORDS - 1U) *
            0x1111111111111111ULL;

        goto_priv(PRIV_M);
        ai_memblock_cbozero_masked_write_bytes(seed_src_paddr,
            seed_patterns[round], AI_MEMBLOCK_CBOZERO_MASKED_VECTOR_BYTES);
        write64(seed_guard_paddr, seed_guards[round]);
        ai_memblock_cbozero_masked_seed_block(masked_base_paddr,
            masked_seeds[round]);
        ai_memblock_cbozero_masked_write_bytes(masked_base_paddr,
            masked_patterns[round], AI_MEMBLOCK_CBOZERO_MASKED_VECTOR_BYTES);
        write64(masked_before_paddr, masked_before_guards[round]);
        write64(masked_after_paddr, masked_after_guards[round]);
        write64(capture_dst_paddr, capture_fills[round]);
        write64(capture_dst_paddr + sizeof(uint64_t),
            ~capture_fills[round]);
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
        const uint64_t warmed_masked0 = ld(masked_base_vaddr);
        const uint64_t warmed_tail = ld(masked_tail_vaddr);
        const uint64_t warmed_marker = ld(marker_vaddr);
        AI_ASSERT_NO_EXCEPT_AND("before the partial-active masked vector-load plus CBO.ZERO owner-window starts, seed source, masked source, and marker are readable with seeded values",
            warmed_seed0 != 0ULL &&
            warmed_masked0 != 0ULL &&
            warmed_tail == expected_tail &&
            warmed_marker == marker_seeds[round]
        );

        TEST_SETUP_EXCEPT();
        ai_memblock_partial_masked_vload_then_cbozero_marker(
            seed_src_vaddr, masked_base_vaddr, mask_vaddr,
            capture_dst_vaddr, zero_subblock_vaddr, marker_vaddr,
            marker_updates[round]);
        const uint64_t observed_masked0 = ld(masked_base_vaddr);
        const uint64_t observed_tail = ld(masked_tail_vaddr);
        const uint64_t observed_marker = ld(marker_vaddr);
        AI_ASSERT_NO_EXCEPT_AND("partial-active masked unit-stride vector load immediately followed by CBO.ZERO captures active source lanes, keeps inactive seed lanes, zeroes the source block, and releases marker progress",
            ai_memblock_cbozero_masked_partial_load_source_matches(
                capture_dst_vaddr,
                masked_patterns[round],
                seed_patterns[round],
                mask_patterns[round],
                AI_MEMBLOCK_CBOZERO_MASKED_VECTOR_BYTES) &&
            observed_masked0 == 0ULL &&
            observed_tail == 0ULL &&
            observed_marker == marker_updates[round]
        );

        TEST_SETUP_EXCEPT();
        AI_ASSERT_NO_EXCEPT_AND("the partial-active masked vector-load plus CBO.ZERO owner-window does not corrupt seed source, masked source, mask, capture, or marker guards",
            ld(seed_guard_vaddr) == seed_guards[round] &&
            ld(masked_before_vaddr) == masked_before_guards[round] &&
            ld(masked_after_vaddr) == masked_after_guards[round] &&
            ld(mask_guard_vaddr) == mask_guards[round] &&
            ld(capture_before_vaddr) == capture_before_guards[round] &&
            ld(capture_after_vaddr) == capture_after_guards[round] &&
            ld(marker_guard_vaddr) == marker_guards[round]
        );

        goto_priv(PRIV_M);
        TEST_ASSERT("backing memory after the partial-active masked vector-load plus CBO.ZERO window captures active source and inactive seed lanes, fully zeroes masked source, updates marker, and preserves guards",
            ai_memblock_cbozero_masked_bytes_match(seed_src_paddr,
                seed_patterns[round],
                AI_MEMBLOCK_CBOZERO_MASKED_VECTOR_BYTES) &&
            ai_memblock_cbozero_masked_partial_load_source_matches(
                capture_dst_paddr,
                masked_patterns[round],
                seed_patterns[round],
                mask_patterns[round],
                AI_MEMBLOCK_CBOZERO_MASKED_VECTOR_BYTES) &&
            ai_memblock_cbozero_masked_block_all_zero(masked_base_paddr) &&
            read64(seed_guard_paddr) == seed_guards[round] &&
            read64(masked_before_paddr) == masked_before_guards[round] &&
            read64(masked_after_paddr) == masked_after_guards[round] &&
            read8(mask_paddr) == mask_patterns[round][0] &&
            read8(mask_paddr + 1U) == mask_patterns[round][1] &&
            read64(mask_guard_paddr) == mask_guards[round] &&
            read64(capture_before_paddr) == capture_before_guards[round] &&
            read64(capture_after_paddr) == capture_after_guards[round] &&
            read64(marker_paddr) == marker_updates[round] &&
            read64(marker_guard_paddr) == marker_guards[round]
        );
    }

    ai_memblock_cbozero_masked_reset_hs_env();

    TEST_END();
}

static inline void ai_memblock_all_inactive_vload_then_cbozero_marker(
    uintptr_t seed_src_vaddr,
    uintptr_t masked_src_vaddr,
    uintptr_t capture_dst_vaddr,
    uintptr_t zero_vaddr,
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
        "vse8.v v8, (%[capture_dst_addr])\n\t"
        ".insn i 0x0f, 0x2, x0, %[zero_addr], 4\n\t"
        "sd %[marker_update], 0(%[marker_addr])\n\t"
        "fence rw, rw\n\t"
        ".option pop\n\t"
        :
        : [seed_src_addr] "r"(seed_src_vaddr),
          [masked_src_addr] "r"(masked_src_vaddr),
          [capture_dst_addr] "r"(capture_dst_vaddr),
          [zero_addr] "r"(zero_vaddr),
          [marker_addr] "r"(marker_vaddr),
          [marker_update] "r"(marker_update),
          [vl] "r"((int)AI_MEMBLOCK_CBOZERO_MASKED_VECTOR_BYTES),
          [vtype] "r"(vtype)
        : "t0", "memory");
}

bool ai_micro_hs_masked_vector_load_all_inactive_then_cbozero_no_dest_clobber_or_zero_drop_corner()
{
    TEST_START();

    const enum test_page seed_page = SWITCH1;
    const enum test_page masked_page = SWITCH2;
    const enum test_page aux_page = VSRWX_GRWX;
    const uintptr_t seed_src_vaddr = hs_page_base(seed_page) + 0xe00U;
    const uintptr_t seed_src_paddr = phys_page_base(seed_page) + 0xe00U;
    const uintptr_t seed_guard_vaddr =
        seed_src_vaddr + AI_MEMBLOCK_CBOZERO_MASKED_VECTOR_BYTES;
    const uintptr_t seed_guard_paddr =
        seed_src_paddr + AI_MEMBLOCK_CBOZERO_MASKED_VECTOR_BYTES;
    const uintptr_t masked_base_vaddr = hs_page_base(masked_page) + 0xe00U;
    const uintptr_t masked_base_paddr = phys_page_base(masked_page) + 0xe00U;
    const uintptr_t zero_subblock_vaddr =
        masked_base_vaddr + 3U * sizeof(uint64_t);
    const uintptr_t masked_tail_vaddr =
        masked_base_vaddr +
        (AI_MEMBLOCK_CBOZERO_MASKED_BLOCK_WORDS - 1U) *
        sizeof(uint64_t);
    const uintptr_t masked_before_vaddr =
        masked_base_vaddr - sizeof(uint64_t);
    const uintptr_t masked_after_vaddr =
        masked_base_vaddr + AI_MEMBLOCK_CBOZERO_MASKED_BLOCK_WORDS *
        sizeof(uint64_t);
    const uintptr_t masked_before_paddr =
        masked_base_paddr - sizeof(uint64_t);
    const uintptr_t masked_after_paddr =
        masked_base_paddr + AI_MEMBLOCK_CBOZERO_MASKED_BLOCK_WORDS *
        sizeof(uint64_t);
    const uintptr_t capture_dst_vaddr = hs_page_base(aux_page) + 0xe00U;
    const uintptr_t capture_dst_paddr = phys_page_base(aux_page) + 0xe00U;
    const uintptr_t capture_before_vaddr =
        capture_dst_vaddr - sizeof(uint64_t);
    const uintptr_t capture_after_vaddr =
        capture_dst_vaddr + AI_MEMBLOCK_CBOZERO_MASKED_VECTOR_BYTES;
    const uintptr_t capture_before_paddr =
        capture_dst_paddr - sizeof(uint64_t);
    const uintptr_t capture_after_paddr =
        capture_dst_paddr + AI_MEMBLOCK_CBOZERO_MASKED_VECTOR_BYTES;
    const uintptr_t marker_vaddr = hs_page_base(aux_page) + 0xe80U;
    const uintptr_t marker_paddr = phys_page_base(aux_page) + 0xe80U;
    const uintptr_t marker_guard_vaddr = marker_vaddr + sizeof(uint64_t);
    const uintptr_t marker_guard_paddr = marker_paddr + sizeof(uint64_t);
    static const uint8_t seed_patterns[][AI_MEMBLOCK_CBOZERO_MASKED_VECTOR_BYTES] = {
        { 0x7f, 0x6e, 0x5d, 0x4c, 0x3b, 0x2a, 0x19, 0x08,
          0xf7, 0xe6, 0xd5, 0xc4, 0xb3, 0xa2, 0x91, 0x80 },
        { 0x80, 0x91, 0xa2, 0xb3, 0xc4, 0xd5, 0xe6, 0xf7,
          0x08, 0x19, 0x2a, 0x3b, 0x4c, 0x5d, 0x6e, 0x7f },
        { 0x24, 0x68, 0xac, 0xe0, 0x13, 0x57, 0x9b, 0xdf,
          0x31, 0x75, 0xb9, 0xfd, 0x02, 0x46, 0x8a, 0xce },
    };
    const uint64_t masked_seeds[] = {
        0x2233445566778899ULL,
        0x99aabbccddeeff00ULL,
        0x1123581321345589ULL,
    };
    const uint64_t seed_guards[] = {
        0xbaadf00d00000001ULL,
        0xbaadf00d00000002ULL,
        0xbaadf00d00000003ULL,
    };
    const uint64_t masked_before_guards[] = {
        0xc001d00d00000001ULL,
        0xc001d00d00000002ULL,
        0xc001d00d00000003ULL,
    };
    const uint64_t masked_after_guards[] = {
        0xfeedc0de00000001ULL,
        0xfeedc0de00000002ULL,
        0xfeedc0de00000003ULL,
    };
    const uint64_t capture_fills[] = {
        0xaaaaaaaa55555555ULL,
        0xbbbbbbbb66666666ULL,
        0xcccccccc77777777ULL,
    };
    const uint64_t capture_before_guards[] = {
        0x1357135713571357ULL,
        0x2468246824682468ULL,
        0x3699369936993699ULL,
    };
    const uint64_t capture_after_guards[] = {
        0xabcdefabcdefabcdULL,
        0xbcdefabcdefabcdeULL,
        0xcdefabcdefabcdefULL,
    };
    const uint64_t marker_seeds[] = {
        0x0123456789abcdefULL,
        0x123456789abcdef0ULL,
        0x23456789abcdef01ULL,
    };
    const uint64_t marker_updates[] = {
        0x0eed000000000001ULL,
        0x0eed000000000002ULL,
        0x0eed000000000003ULL,
    };
    const uint64_t marker_guards[] = {
        0xfedcba9876543210ULL,
        0xedcba9876543210fULL,
        0xdcba9876543210feULL,
    };

    ai_memblock_cbozero_masked_prepare_hs_env();

    for (size_t round = 0;
         round < AI_MEMBLOCK_CBOZERO_MASKED_ARRAY_SIZE(masked_seeds);
         ++round) {
        const uint64_t expected_tail =
            masked_seeds[round] +
            (AI_MEMBLOCK_CBOZERO_MASKED_BLOCK_WORDS - 1U) *
            0x1111111111111111ULL;

        goto_priv(PRIV_M);
        ai_memblock_cbozero_masked_write_bytes(seed_src_paddr,
            seed_patterns[round], AI_MEMBLOCK_CBOZERO_MASKED_VECTOR_BYTES);
        write64(seed_guard_paddr, seed_guards[round]);
        ai_memblock_cbozero_masked_seed_block(masked_base_paddr,
            masked_seeds[round]);
        write64(masked_before_paddr, masked_before_guards[round]);
        write64(masked_after_paddr, masked_after_guards[round]);
        write64(capture_dst_paddr, capture_fills[round]);
        write64(capture_dst_paddr + sizeof(uint64_t),
            ~capture_fills[round]);
        write64(capture_before_paddr, capture_before_guards[round]);
        write64(capture_after_paddr, capture_after_guards[round]);
        write64(marker_paddr, marker_seeds[round]);
        write64(marker_guard_paddr, marker_guards[round]);

        goto_priv(PRIV_HS);
        TEST_SETUP_EXCEPT();
        const uint64_t warmed_seed0 = ld(seed_src_vaddr);
        const uint64_t warmed_masked0 = ld(masked_base_vaddr);
        const uint64_t warmed_tail = ld(masked_tail_vaddr);
        const uint64_t warmed_marker = ld(marker_vaddr);
        AI_ASSERT_NO_EXCEPT_AND("before the all-inactive masked vector-load plus CBO.ZERO owner-window starts, seed source, masked source, and marker are readable with seeded values",
            warmed_seed0 != 0ULL &&
            warmed_masked0 == masked_seeds[round] &&
            warmed_tail == expected_tail &&
            warmed_marker == marker_seeds[round]
        );

        TEST_SETUP_EXCEPT();
        ai_memblock_all_inactive_vload_then_cbozero_marker(
            seed_src_vaddr, masked_base_vaddr, capture_dst_vaddr,
            zero_subblock_vaddr, marker_vaddr, marker_updates[round]);
        const uint64_t observed_masked0 = ld(masked_base_vaddr);
        const uint64_t observed_tail = ld(masked_tail_vaddr);
        const uint64_t observed_marker = ld(marker_vaddr);
        AI_ASSERT_NO_EXCEPT_AND("all-inactive masked unit-stride vector load immediately followed by CBO.ZERO keeps the preloaded vector payload, zeroes the masked source block, and releases marker progress",
            ai_memblock_cbozero_masked_bytes_match(capture_dst_vaddr,
                seed_patterns[round],
                AI_MEMBLOCK_CBOZERO_MASKED_VECTOR_BYTES) &&
            observed_masked0 == 0ULL &&
            observed_tail == 0ULL &&
            observed_marker == marker_updates[round]
        );

        TEST_SETUP_EXCEPT();
        AI_ASSERT_NO_EXCEPT_AND("the all-inactive masked vector-load plus CBO.ZERO owner-window does not corrupt seed source, masked source, capture, or marker guards",
            ld(seed_guard_vaddr) == seed_guards[round] &&
            ld(masked_before_vaddr) == masked_before_guards[round] &&
            ld(masked_after_vaddr) == masked_after_guards[round] &&
            ld(capture_before_vaddr) == capture_before_guards[round] &&
            ld(capture_after_vaddr) == capture_after_guards[round] &&
            ld(marker_guard_vaddr) == marker_guards[round]
        );

        goto_priv(PRIV_M);
        TEST_ASSERT("backing memory after the all-inactive masked vector-load plus CBO.ZERO window keeps seed payload, captures only the preloaded vector payload, fully zeroes masked source, updates marker, and preserves guards",
            ai_memblock_cbozero_masked_bytes_match(seed_src_paddr,
                seed_patterns[round],
                AI_MEMBLOCK_CBOZERO_MASKED_VECTOR_BYTES) &&
            ai_memblock_cbozero_masked_bytes_match(capture_dst_paddr,
                seed_patterns[round],
                AI_MEMBLOCK_CBOZERO_MASKED_VECTOR_BYTES) &&
            ai_memblock_cbozero_masked_block_all_zero(masked_base_paddr) &&
            read64(seed_guard_paddr) == seed_guards[round] &&
            read64(masked_before_paddr) == masked_before_guards[round] &&
            read64(masked_after_paddr) == masked_after_guards[round] &&
            read64(capture_before_paddr) == capture_before_guards[round] &&
            read64(capture_after_paddr) == capture_after_guards[round] &&
            read64(marker_paddr) == marker_updates[round] &&
            read64(marker_guard_paddr) == marker_guards[round]
        );
    }

    ai_memblock_cbozero_masked_reset_hs_env();

    TEST_END();
}

static inline void ai_memblock_cbozero_then_all_inactive_vload_capture_marker(
    uintptr_t zero_vaddr,
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
        ".insn i 0x0f, 0x2, x0, %[zero_addr], 4\n\t"
        "vsetvl t0, %[vl], %[vtype]\n\t"
        "vle8.v v8, (%[seed_src_addr])\n\t"
        "vmv.v.i v0, 0\n\t"
        "vle8.v v8, (%[masked_src_addr]), v0.t\n\t"
        "vse8.v v8, (%[capture_dst_addr])\n\t"
        "sd %[marker_update], 0(%[marker_addr])\n\t"
        "fence rw, rw\n\t"
        ".option pop\n\t"
        :
        : [zero_addr] "r"(zero_vaddr),
          [seed_src_addr] "r"(seed_src_vaddr),
          [masked_src_addr] "r"(masked_src_vaddr),
          [capture_dst_addr] "r"(capture_dst_vaddr),
          [marker_addr] "r"(marker_vaddr),
          [marker_update] "r"(marker_update),
          [vl] "r"((int)AI_MEMBLOCK_CBOZERO_MASKED_VECTOR_BYTES),
          [vtype] "r"(vtype)
        : "t0", "memory");
}

bool ai_micro_hs_cbozero_then_masked_vector_load_all_inactive_no_dest_clobber_or_marker_drop_corner()
{
    TEST_START();

    const enum test_page seed_page = SWITCH1;
    const enum test_page masked_page = SWITCH2;
    const enum test_page aux_page = VSRWX_GRWX;
    const uintptr_t seed_src_vaddr = hs_page_base(seed_page) + 0x780U;
    const uintptr_t seed_src_paddr = phys_page_base(seed_page) + 0x780U;
    const uintptr_t seed_guard_vaddr =
        seed_src_vaddr + AI_MEMBLOCK_CBOZERO_MASKED_VECTOR_BYTES;
    const uintptr_t seed_guard_paddr =
        seed_src_paddr + AI_MEMBLOCK_CBOZERO_MASKED_VECTOR_BYTES;
    const uintptr_t masked_base_vaddr = hs_page_base(masked_page) + 0x780U;
    const uintptr_t masked_base_paddr = phys_page_base(masked_page) + 0x780U;
    const uintptr_t zero_subblock_vaddr =
        masked_base_vaddr + 3U * sizeof(uint64_t);
    const uintptr_t masked_tail_vaddr =
        masked_base_vaddr +
        (AI_MEMBLOCK_CBOZERO_MASKED_BLOCK_WORDS - 1U) *
        sizeof(uint64_t);
    const uintptr_t masked_before_vaddr =
        masked_base_vaddr - sizeof(uint64_t);
    const uintptr_t masked_after_vaddr =
        masked_base_vaddr + AI_MEMBLOCK_CBOZERO_MASKED_BLOCK_WORDS *
        sizeof(uint64_t);
    const uintptr_t masked_before_paddr =
        masked_base_paddr - sizeof(uint64_t);
    const uintptr_t masked_after_paddr =
        masked_base_paddr + AI_MEMBLOCK_CBOZERO_MASKED_BLOCK_WORDS *
        sizeof(uint64_t);
    const uintptr_t capture_dst_vaddr = hs_page_base(aux_page) + 0x780U;
    const uintptr_t capture_dst_paddr = phys_page_base(aux_page) + 0x780U;
    const uintptr_t capture_before_vaddr =
        capture_dst_vaddr - sizeof(uint64_t);
    const uintptr_t capture_after_vaddr =
        capture_dst_vaddr + AI_MEMBLOCK_CBOZERO_MASKED_VECTOR_BYTES;
    const uintptr_t capture_before_paddr =
        capture_dst_paddr - sizeof(uint64_t);
    const uintptr_t capture_after_paddr =
        capture_dst_paddr + AI_MEMBLOCK_CBOZERO_MASKED_VECTOR_BYTES;
    const uintptr_t marker_vaddr = hs_page_base(aux_page) + 0x800U;
    const uintptr_t marker_paddr = phys_page_base(aux_page) + 0x800U;
    const uintptr_t marker_guard_vaddr = marker_vaddr + sizeof(uint64_t);
    const uintptr_t marker_guard_paddr = marker_paddr + sizeof(uint64_t);
    const uint64_t seed_word0_values[] = {
        0x0011223344556677ULL,
        0x8899aabbccddeeffULL,
        0x13579bdf2468ace0ULL,
    };
    const uint64_t seed_word1_values[] = {
        0x7766554433221100ULL,
        0xffeeddccbbaa9988ULL,
        0x0eca8642fdb97531ULL,
    };
    const uint64_t masked_seeds[] = {
        0x0f1e2d3c4b5a6978ULL,
        0x8877665544332211ULL,
        0x2435465768798a9bULL,
    };
    const uint64_t seed_guards[] = {
        0x1020304050607080ULL,
        0x1122334455667788ULL,
        0x2233445566778899ULL,
    };
    const uint64_t masked_before_guards[] = {
        0x3141592653589793ULL,
        0x2718281828459045ULL,
        0x1123581321345589ULL,
    };
    const uint64_t masked_after_guards[] = {
        0xfeedfacecafebeefULL,
        0xdeadc0def00dbabeULL,
        0xfaceb00c13579bdfULL,
    };
    const uint64_t capture_fills[] = {
        0xc3c3c3c3c3c3c3c3ULL,
        0x3c3c3c3c3c3c3c3cULL,
        0xa55aa55ac33cc33cULL,
    };
    const uint64_t capture_before_guards[] = {
        0x55aa33cc7799bbddULL,
        0x1234fedc5678ba90ULL,
        0x0badc0de55aa33ccULL,
    };
    const uint64_t capture_after_guards[] = {
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
        0xabcdef0123456789ULL,
        0xbcdef0123456789aULL,
        0xcdef0123456789abULL,
    };

    ai_memblock_cbozero_masked_prepare_hs_env();

    for (size_t round = 0;
         round < AI_MEMBLOCK_CBOZERO_MASKED_ARRAY_SIZE(masked_seeds);
         ++round) {
        const uint64_t expected_tail =
            masked_seeds[round] +
            (AI_MEMBLOCK_CBOZERO_MASKED_BLOCK_WORDS - 1U) *
            0x1111111111111111ULL;

        goto_priv(PRIV_M);
        write64(seed_src_paddr, seed_word0_values[round]);
        write64(seed_src_paddr + sizeof(uint64_t),
            seed_word1_values[round]);
        write64(seed_guard_paddr, seed_guards[round]);
        ai_memblock_cbozero_masked_seed_block(masked_base_paddr,
            masked_seeds[round]);
        write64(masked_before_paddr, masked_before_guards[round]);
        write64(masked_after_paddr, masked_after_guards[round]);
        write64(capture_dst_paddr, capture_fills[round]);
        write64(capture_dst_paddr + sizeof(uint64_t),
            ~capture_fills[round]);
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
        const uint64_t warmed_tail = ld(masked_tail_vaddr);
        const uint64_t warmed_marker = ld(marker_vaddr);
        AI_ASSERT_NO_EXCEPT_AND("before the CBO.ZERO plus all-inactive masked vector-load owner-window starts, seed source, masked source, and marker are readable with seeded values",
            warmed_seed0 == seed_word0_values[round] &&
            warmed_seed1 == seed_word1_values[round] &&
            warmed_masked0 == masked_seeds[round] &&
            warmed_tail == expected_tail &&
            warmed_marker == marker_seeds[round]
        );

        TEST_SETUP_EXCEPT();
        ai_memblock_cbozero_then_all_inactive_vload_capture_marker(
            zero_subblock_vaddr, seed_src_vaddr, masked_base_vaddr,
            capture_dst_vaddr, marker_vaddr, marker_updates[round]);
        const uint64_t observed_capture0 = ld(capture_dst_vaddr);
        const uint64_t observed_capture1 =
            ld(capture_dst_vaddr + sizeof(uint64_t));
        const uint64_t observed_masked0 = ld(masked_base_vaddr);
        const uint64_t observed_tail = ld(masked_tail_vaddr);
        const uint64_t observed_marker = ld(marker_vaddr);
        AI_ASSERT_NO_EXCEPT_AND("CBO.ZERO immediately followed by all-inactive masked unit-stride vector load keeps the preloaded vector payload, zeroes the masked source block, and releases marker progress",
            observed_capture0 == seed_word0_values[round] &&
            observed_capture1 == seed_word1_values[round] &&
            observed_masked0 == 0ULL &&
            observed_tail == 0ULL &&
            observed_marker == marker_updates[round]
        );

        TEST_SETUP_EXCEPT();
        AI_ASSERT_NO_EXCEPT_AND("the CBO.ZERO plus all-inactive masked vector-load owner-window does not corrupt seed source, masked source, capture, or marker guards",
            ld(seed_guard_vaddr) == seed_guards[round] &&
            ld(masked_before_vaddr) == masked_before_guards[round] &&
            ld(masked_after_vaddr) == masked_after_guards[round] &&
            ld(capture_before_vaddr) == capture_before_guards[round] &&
            ld(capture_after_vaddr) == capture_after_guards[round] &&
            ld(marker_guard_vaddr) == marker_guards[round]
        );

        goto_priv(PRIV_M);
        TEST_ASSERT("backing memory after the CBO.ZERO plus all-inactive masked vector-load window keeps seed payload, fully zeroes masked source, captures only the preloaded vector payload, updates marker, and preserves guards",
            read64(seed_src_paddr) == seed_word0_values[round] &&
            read64(seed_src_paddr + sizeof(uint64_t)) ==
                seed_word1_values[round] &&
            read64(seed_guard_paddr) == seed_guards[round] &&
            ai_memblock_cbozero_masked_block_all_zero(masked_base_paddr) &&
            read64(masked_before_paddr) == masked_before_guards[round] &&
            read64(masked_after_paddr) == masked_after_guards[round] &&
            read64(capture_dst_paddr) == seed_word0_values[round] &&
            read64(capture_dst_paddr + sizeof(uint64_t)) ==
                seed_word1_values[round] &&
            read64(capture_before_paddr) == capture_before_guards[round] &&
            read64(capture_after_paddr) == capture_after_guards[round] &&
            read64(marker_paddr) == marker_updates[round] &&
            read64(marker_guard_paddr) == marker_guards[round]
        );
    }

    ai_memblock_cbozero_masked_reset_hs_env();

    TEST_END();
}
