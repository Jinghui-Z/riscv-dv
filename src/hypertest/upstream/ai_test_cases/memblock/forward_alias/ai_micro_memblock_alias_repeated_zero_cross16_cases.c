#include <rvh_test.h>
#include <page_tables.h>
#include <stddef.h>
#include <stdint.h>

#define AI_P13AX_ASSERT_NO_EXCEPT_AND(message, cond) \
    TEST_ASSERT(message, excpt.triggered == false && (cond))

#define AI_P13AX_BLOCK_BYTES 64U
#define AI_P13AX_BLOCK_WORDS 8U

static inline pte_t *ai_p13ax_hs_leaf_entry(enum test_page page)
{
#ifdef sv48
    return &hspt[4][page];
#else
    return &hspt[2][page];
#endif
}

static inline void ai_p13ax_prepare_hs_env(uint64_t menvcfg_bits)
{
    goto_priv(PRIV_M);
    CSRS(CSR_MENVCFG, menvcfg_bits);
    hspt_init();
    sfence_vma();
}

static inline uintptr_t ai_p13ax_prepare_same_paddr_alias(
    enum test_page alias_page,
    enum test_page base_page)
{
    goto_priv(PRIV_M);
    *ai_p13ax_hs_leaf_entry(alias_page) =
        (phys_page_base(base_page) >> 2) | PTE_V | PTE_AD | PTE_RWX;
    sfence_vma();

    return hs_page_base(alias_page);
}

static inline void ai_p13ax_reset_hs_env(uint64_t menvcfg_bits)
{
    goto_priv(PRIV_M);
    CSRC(CSR_MENVCFG, menvcfg_bits);
    hspt_init();
    sfence_vma();
}

static inline uint64_t ai_p13ax_seed_word(uint64_t seed, uintptr_t word_idx)
{
    return seed + (word_idx * 0x1111111111111111ULL);
}

static inline void ai_p13ax_seed_block(uintptr_t paddr, uint64_t seed)
{
    for (uintptr_t i = 0; i < AI_P13AX_BLOCK_WORDS; ++i) {
        write64(paddr + (i * sizeof(uint64_t)),
            ai_p13ax_seed_word(seed, i));
    }
}

static inline bool ai_p13ax_block_matches_seed(uintptr_t paddr, uint64_t seed)
{
    for (uintptr_t i = 0; i < AI_P13AX_BLOCK_WORDS; ++i) {
        if (read64(paddr + (i * sizeof(uint64_t))) !=
            ai_p13ax_seed_word(seed, i)) {
            return false;
        }
    }

    return true;
}

static inline uint64_t ai_p13ax_cross16_low_word(uint64_t payload)
{
    return (payload & 0xffULL) << 56;
}

static inline uint64_t ai_p13ax_cross16_high_word(uint64_t payload)
{
    return payload >> 8;
}

static inline bool ai_p13ax_zero_block_matches_cross16_store(
    uintptr_t paddr,
    uint64_t payload)
{
    for (uintptr_t i = 0; i < AI_P13AX_BLOCK_WORDS; ++i) {
        uint64_t expected = 0ULL;

        if (i == 1U) {
            expected = ai_p13ax_cross16_low_word(payload);
        } else if (i == 2U) {
            expected = ai_p13ax_cross16_high_word(payload);
        }

        if (read64(paddr + (i * sizeof(uint64_t))) != expected) {
            return false;
        }
    }

    return true;
}

static inline void ai_p13ax_cbo_inval_prefetch_zero_zero_cross16_window(
    uintptr_t inval_vaddr,
    uintptr_t prefetch_vaddr,
    uintptr_t first_zero_vaddr,
    uintptr_t second_zero_vaddr,
    uintptr_t cross16_store_vaddr,
    uintptr_t low_word_vaddr,
    uintptr_t high_word_vaddr,
    uintptr_t marker_vaddr,
    uint64_t cross16_payload,
    uint64_t marker_value,
    uint64_t *low_word_observed,
    uint64_t *high_word_observed,
    uint64_t *marker_observed)
{
    uint64_t low_readback = 0ULL;
    uint64_t high_readback = 0ULL;
    uint64_t marker_readback = 0ULL;

    asm volatile(
        ".option push\n\t"
        ".option norvc\n\t"
        ".insn i 0x0f, 0x2, x0, %[inval_addr], 0\n\t"
        ".insn i 0x13, 0x6, x0, %[prefetch_addr], 1\n\t"
        ".insn i 0x0f, 0x2, x0, %[first_zero_addr], 4\n\t"
        ".insn i 0x0f, 0x2, x0, %[second_zero_addr], 4\n\t"
        "sd %[cross16_payload], 0(%[cross16_addr])\n\t"
        "ld %[low_readback], 0(%[low_word_addr])\n\t"
        "ld %[high_readback], 0(%[high_word_addr])\n\t"
        "sd %[marker_value], 0(%[marker_addr])\n\t"
        "fence rw, rw\n\t"
        "ld %[marker_readback], 0(%[marker_addr])\n\t"
        ".option pop\n\t"
        : [low_readback] "=&r"(low_readback),
          [high_readback] "=&r"(high_readback),
          [marker_readback] "=&r"(marker_readback)
        : [inval_addr] "r"(inval_vaddr),
          [prefetch_addr] "r"(prefetch_vaddr),
          [first_zero_addr] "r"(first_zero_vaddr),
          [second_zero_addr] "r"(second_zero_vaddr),
          [cross16_addr] "r"(cross16_store_vaddr),
          [low_word_addr] "r"(low_word_vaddr),
          [high_word_addr] "r"(high_word_vaddr),
          [marker_addr] "r"(marker_vaddr),
          [cross16_payload] "r"(cross16_payload),
          [marker_value] "r"(marker_value)
        : "memory");

    *low_word_observed = low_readback;
    *high_word_observed = high_readback;
    *marker_observed = marker_readback;
}

bool ai_micro_hs_cbo_inval_prefetch_same_paddr_alias_refill_then_cbo_zero_cbo_zero_cross16_store_no_stale_repeated_zero_split_payload_or_marker_drop_corner()
{
    TEST_START();

    const enum test_page base_page = SWITCH1;
    const enum test_page alias_page = SWITCH2;
    const uintptr_t target_offset = 0xbc0U;
    const uintptr_t target_base_vaddr =
        hs_page_base(base_page) + target_offset;
    const uintptr_t target_base_paddr =
        phys_page_base(base_page) + target_offset;
    const uintptr_t low_word_base_vaddr =
        target_base_vaddr + sizeof(uint64_t);
    const uintptr_t high_word_base_vaddr =
        target_base_vaddr + 2U * sizeof(uint64_t);
    const uintptr_t prefetch_base_vaddr =
        target_base_vaddr + sizeof(uint64_t);
    const uintptr_t first_zero_base_vaddr =
        target_base_vaddr + 4U * sizeof(uint64_t);
    const uintptr_t second_zero_base_vaddr =
        target_base_vaddr + (AI_P13AX_BLOCK_WORDS - 1U) *
        sizeof(uint64_t);
    const uintptr_t before_guard_base_vaddr =
        target_base_vaddr - sizeof(uint64_t);
    const uintptr_t after_guard_base_vaddr =
        target_base_vaddr + 2U * AI_P13AX_BLOCK_BYTES;
    const uintptr_t adjacent_base_vaddr =
        target_base_vaddr + AI_P13AX_BLOCK_BYTES;
    const uintptr_t marker_base_vaddr =
        target_base_vaddr + 3U * AI_P13AX_BLOCK_BYTES;
    const uintptr_t marker_guard_base_vaddr =
        marker_base_vaddr + sizeof(uint64_t);
    const uintptr_t before_guard_paddr =
        target_base_paddr - sizeof(uint64_t);
    const uintptr_t after_guard_paddr =
        target_base_paddr + 2U * AI_P13AX_BLOCK_BYTES;
    const uintptr_t adjacent_base_paddr =
        target_base_paddr + AI_P13AX_BLOCK_BYTES;
    const uintptr_t marker_paddr =
        target_base_paddr + 3U * AI_P13AX_BLOCK_BYTES;
    const uintptr_t marker_guard_paddr =
        marker_paddr + sizeof(uint64_t);
    const uint64_t block_seeds[] = {
        0x31425364758697a8ULL,
        0x425364758697a8b9ULL,
        0x5364758697a8b9caULL,
    };
    const uint64_t adjacent_seeds[] = {
        0x0123456789abcdefULL,
        0xfedcba9876543210ULL,
        0x55aa7733cc11ee99ULL,
    };
    const uint64_t cross16_payloads[] = {
        0x1122334455667788ULL,
        0x99aabbccddeeff10ULL,
        0x0f1e2d3c4b5a6978ULL,
    };
    const uint64_t before_guards[] = {
        0xfdb97531eca86420ULL,
        0x13579bdf2468ace0ULL,
        0x02468ace13579bdfULL,
    };
    const uint64_t after_guards[] = {
        0x1020304050607080ULL,
        0x0badf00d5a5aa5a5ULL,
        0x1123581321345589ULL,
    };
    const uint64_t marker_seeds[] = {
        0xa0a1a2a3a4a5a6a7ULL,
        0xb0b1b2b3b4b5b6b7ULL,
        0xc0c1c2c3c4c5c6c7ULL,
    };
    const uint64_t marker_updates[] = {
        0x456789abcdef0123ULL,
        0x3210fedcba987654ULL,
        0x5a5aa5a50badf00dULL,
    };
    const uint64_t marker_guards[] = {
        0x2718281828459045ULL,
        0x3141592653589793ULL,
        0x1123581321345589ULL,
    };
    uintptr_t alias_page_base_vaddr = 0;
    uintptr_t target_alias_vaddr = 0;
    uintptr_t cross16_store_alias_vaddr = 0;
    uintptr_t low_word_alias_vaddr = 0;
    uintptr_t high_word_alias_vaddr = 0;
    uintptr_t inval_alias_vaddr = 0;
    uintptr_t first_zero_alias_vaddr = 0;
    uintptr_t second_zero_alias_vaddr = 0;
    uintptr_t before_guard_alias_vaddr = 0;
    uintptr_t after_guard_alias_vaddr = 0;
    uintptr_t adjacent_alias_vaddr = 0;
    uintptr_t marker_alias_vaddr = 0;
    uintptr_t marker_guard_alias_vaddr = 0;

    ai_p13ax_prepare_hs_env(MENVCFG_CBIE01 | MENVCFG_CBZE);
    alias_page_base_vaddr =
        ai_p13ax_prepare_same_paddr_alias(alias_page, base_page);
    target_alias_vaddr = alias_page_base_vaddr + target_offset;
    cross16_store_alias_vaddr = target_alias_vaddr + 15U;
    low_word_alias_vaddr = target_alias_vaddr + sizeof(uint64_t);
    high_word_alias_vaddr = target_alias_vaddr + 2U * sizeof(uint64_t);
    inval_alias_vaddr =
        target_alias_vaddr + 6U * sizeof(uint64_t);
    first_zero_alias_vaddr =
        target_alias_vaddr + 4U * sizeof(uint64_t);
    second_zero_alias_vaddr =
        target_alias_vaddr + (AI_P13AX_BLOCK_WORDS - 1U) *
        sizeof(uint64_t);
    before_guard_alias_vaddr =
        target_alias_vaddr - sizeof(uint64_t);
    after_guard_alias_vaddr =
        target_alias_vaddr + 2U * AI_P13AX_BLOCK_BYTES;
    adjacent_alias_vaddr =
        target_alias_vaddr + AI_P13AX_BLOCK_BYTES;
    marker_alias_vaddr =
        target_alias_vaddr + 3U * AI_P13AX_BLOCK_BYTES;
    marker_guard_alias_vaddr =
        marker_alias_vaddr + sizeof(uint64_t);

    for (size_t round = 0; round < sizeof(block_seeds) / sizeof(block_seeds[0]);
         ++round) {
        const uint64_t seed_word1 =
            ai_p13ax_seed_word(block_seeds[round], 1U);
        const uint64_t seed_word2 =
            ai_p13ax_seed_word(block_seeds[round], 2U);
        const uint64_t expected_first_zero_seed =
            ai_p13ax_seed_word(block_seeds[round], 4U);
        const uint64_t expected_second_zero_seed =
            ai_p13ax_seed_word(block_seeds[round], AI_P13AX_BLOCK_WORDS - 1U);
        const uint64_t expected_low_word =
            ai_p13ax_cross16_low_word(cross16_payloads[round]);
        const uint64_t expected_high_word =
            ai_p13ax_cross16_high_word(cross16_payloads[round]);
        uint64_t low_word_observed = 0ULL;
        uint64_t high_word_observed = 0ULL;
        uint64_t marker_observed = 0ULL;

        goto_priv(PRIV_M);
        ai_p13ax_seed_block(target_base_paddr, block_seeds[round]);
        ai_p13ax_seed_block(adjacent_base_paddr, adjacent_seeds[round]);
        write64(before_guard_paddr, before_guards[round]);
        write64(after_guard_paddr, after_guards[round]);
        write64(marker_paddr, marker_seeds[round]);
        write64(marker_guard_paddr, marker_guards[round]);

        goto_priv(PRIV_HS);
        TEST_SETUP_EXCEPT();
        AI_P13AX_ASSERT_NO_EXCEPT_AND("before the same-paddr-alias inval prefetch repeated-zero cross-16B store handoff starts, base and alias views expose seeded split words, zero probes, adjacent block, marker, and guards",
            ld(target_base_vaddr) == block_seeds[round] &&
            ld(target_alias_vaddr) == block_seeds[round] &&
            ld(low_word_base_vaddr) == seed_word1 &&
            ld(low_word_alias_vaddr) == seed_word1 &&
            ld(high_word_base_vaddr) == seed_word2 &&
            ld(high_word_alias_vaddr) == seed_word2 &&
            ld(first_zero_base_vaddr) == expected_first_zero_seed &&
            ld(first_zero_alias_vaddr) == expected_first_zero_seed &&
            ld(second_zero_base_vaddr) == expected_second_zero_seed &&
            ld(second_zero_alias_vaddr) == expected_second_zero_seed &&
            ld(adjacent_base_vaddr) == adjacent_seeds[round] &&
            ld(adjacent_alias_vaddr) == adjacent_seeds[round] &&
            ld(marker_base_vaddr) == marker_seeds[round] &&
            ld(marker_alias_vaddr) == marker_seeds[round]
        );

        TEST_SETUP_EXCEPT();
        ai_p13ax_cbo_inval_prefetch_zero_zero_cross16_window(
            inval_alias_vaddr, prefetch_base_vaddr,
            first_zero_alias_vaddr, second_zero_alias_vaddr,
            cross16_store_alias_vaddr, low_word_alias_vaddr,
            high_word_alias_vaddr, marker_alias_vaddr,
            cross16_payloads[round], marker_updates[round],
            &low_word_observed, &high_word_observed, &marker_observed);
        AI_P13AX_ASSERT_NO_EXCEPT_AND("same-paddr alias repeated CBO.ZERO must leave a zero image for the following alias-view cross-16B store, preserving both split halves and marker progress",
            low_word_observed == expected_low_word &&
            high_word_observed == expected_high_word &&
            marker_observed == marker_updates[round] &&
            ld(low_word_base_vaddr) == expected_low_word &&
            ld(low_word_alias_vaddr) == expected_low_word &&
            ld(high_word_base_vaddr) == expected_high_word &&
            ld(high_word_alias_vaddr) == expected_high_word &&
            ld(first_zero_base_vaddr) == 0ULL &&
            ld(first_zero_alias_vaddr) == 0ULL &&
            ld(second_zero_base_vaddr) == 0ULL &&
            ld(second_zero_alias_vaddr) == 0ULL &&
            ld(marker_base_vaddr) == marker_updates[round] &&
            ld(marker_alias_vaddr) == marker_updates[round]
        );

        TEST_SETUP_EXCEPT();
        AI_P13AX_ASSERT_NO_EXCEPT_AND("the same-paddr-alias repeated-zero to cross-16B store handoff preserves guards and the adjacent block through both views",
            ld(before_guard_base_vaddr) == before_guards[round] &&
            ld(before_guard_alias_vaddr) == before_guards[round] &&
            ld(after_guard_base_vaddr) == after_guards[round] &&
            ld(after_guard_alias_vaddr) == after_guards[round] &&
            ld(marker_guard_base_vaddr) == marker_guards[round] &&
            ld(marker_guard_alias_vaddr) == marker_guards[round] &&
            ld(adjacent_base_vaddr) == adjacent_seeds[round] &&
            ld(adjacent_alias_vaddr) == adjacent_seeds[round]
        );

        goto_priv(PRIV_M);
        TEST_ASSERT("backing memory after same-paddr-alias inval prefetch repeated-zero cross-16B store handoff contains only the split-store overlay over an otherwise zeroed block, marker update, and unchanged guards",
            ai_p13ax_zero_block_matches_cross16_store(target_base_paddr,
                cross16_payloads[round]) &&
            ai_p13ax_block_matches_seed(adjacent_base_paddr,
                adjacent_seeds[round]) &&
            read64(before_guard_paddr) == before_guards[round] &&
            read64(after_guard_paddr) == after_guards[round] &&
            read64(marker_paddr) == marker_updates[round] &&
            read64(marker_guard_paddr) == marker_guards[round]
        );
    }

    ai_p13ax_reset_hs_env(MENVCFG_CBIE01 | MENVCFG_CBZE);

    TEST_END();
}

static inline uint64_t ai_p13ay_cross16_load_from_seed(
    uint64_t low_word,
    uint64_t high_word)
{
    return (low_word >> 56) | ((high_word & 0x00ffffffffffffffULL) << 8);
}

static inline void ai_p13ay_cbo_inval_prefetch_zero_zero_cross16_load_window(
    uintptr_t inval_vaddr,
    uintptr_t prefetch_vaddr,
    uintptr_t first_zero_vaddr,
    uintptr_t second_zero_vaddr,
    uintptr_t cross16_load_vaddr,
    uintptr_t marker_vaddr,
    uint64_t marker_value,
    uint64_t *load_observed,
    uint64_t *marker_observed)
{
    uint64_t load_readback = 0xffffffffffffffffULL;
    uint64_t marker_readback = 0ULL;

    asm volatile(
        ".option push\n\t"
        ".option norvc\n\t"
        ".insn i 0x0f, 0x2, x0, %[inval_addr], 0\n\t"
        ".insn i 0x13, 0x6, x0, %[prefetch_addr], 1\n\t"
        ".insn i 0x0f, 0x2, x0, %[first_zero_addr], 4\n\t"
        ".insn i 0x0f, 0x2, x0, %[second_zero_addr], 4\n\t"
        "ld %[load_readback], 0(%[load_addr])\n\t"
        "sd %[marker_value], 0(%[marker_addr])\n\t"
        "fence rw, rw\n\t"
        "ld %[marker_readback], 0(%[marker_addr])\n\t"
        ".option pop\n\t"
        : [load_readback] "=&r"(load_readback),
          [marker_readback] "=&r"(marker_readback)
        : [inval_addr] "r"(inval_vaddr),
          [prefetch_addr] "r"(prefetch_vaddr),
          [first_zero_addr] "r"(first_zero_vaddr),
          [second_zero_addr] "r"(second_zero_vaddr),
          [load_addr] "r"(cross16_load_vaddr),
          [marker_addr] "r"(marker_vaddr),
          [marker_value] "r"(marker_value)
        : "memory");

    *load_observed = load_readback;
    *marker_observed = marker_readback;
}

bool ai_micro_hs_cbo_inval_prefetch_same_paddr_alias_refill_then_cbo_zero_cbo_zero_cross16_load_no_stale_repeated_zero_split_merge_or_marker_drop_corner()
{
    TEST_START();

    const enum test_page base_page = SWITCH1;
    const enum test_page alias_page = SWITCH2;
    const uintptr_t target_offset = 0xc00U;
    const uintptr_t target_base_vaddr =
        hs_page_base(base_page) + target_offset;
    const uintptr_t target_base_paddr =
        phys_page_base(base_page) + target_offset;
    const uintptr_t low_word_base_vaddr =
        target_base_vaddr + sizeof(uint64_t);
    const uintptr_t high_word_base_vaddr =
        target_base_vaddr + 2U * sizeof(uint64_t);
    const uintptr_t prefetch_base_vaddr =
        target_base_vaddr + sizeof(uint64_t);
    const uintptr_t first_zero_base_vaddr =
        target_base_vaddr + 4U * sizeof(uint64_t);
    const uintptr_t second_zero_base_vaddr =
        target_base_vaddr + (AI_P13AX_BLOCK_WORDS - 1U) *
        sizeof(uint64_t);
    const uintptr_t before_guard_base_vaddr =
        target_base_vaddr - sizeof(uint64_t);
    const uintptr_t after_guard_base_vaddr =
        target_base_vaddr + 2U * AI_P13AX_BLOCK_BYTES;
    const uintptr_t adjacent_base_vaddr =
        target_base_vaddr + AI_P13AX_BLOCK_BYTES;
    const uintptr_t marker_base_vaddr =
        target_base_vaddr + 3U * AI_P13AX_BLOCK_BYTES;
    const uintptr_t marker_guard_base_vaddr =
        marker_base_vaddr + sizeof(uint64_t);
    const uintptr_t before_guard_paddr =
        target_base_paddr - sizeof(uint64_t);
    const uintptr_t after_guard_paddr =
        target_base_paddr + 2U * AI_P13AX_BLOCK_BYTES;
    const uintptr_t adjacent_base_paddr =
        target_base_paddr + AI_P13AX_BLOCK_BYTES;
    const uintptr_t marker_paddr =
        target_base_paddr + 3U * AI_P13AX_BLOCK_BYTES;
    const uintptr_t marker_guard_paddr =
        marker_paddr + sizeof(uint64_t);
    const uint64_t block_seeds[] = {
        0x64758697a8b9cadbULL,
        0x758697a8b9cadbecULL,
        0x8697a8b9cadbecfdULL,
    };
    const uint64_t adjacent_seeds[] = {
        0x89abcdef01234567ULL,
        0x76543210fedcba98ULL,
        0x55aa7733cc11ee99ULL,
    };
    const uint64_t before_guards[] = {
        0x3141592653589793ULL,
        0x2718281828459045ULL,
        0x02468ace13579bdfULL,
    };
    const uint64_t after_guards[] = {
        0x0badf00d5a5aa5a5ULL,
        0x1020304050607080ULL,
        0x1123581321345589ULL,
    };
    const uint64_t marker_seeds[] = {
        0x1111222233334444ULL,
        0x5555666677778888ULL,
        0x9999aaaabbbbccccULL,
    };
    const uint64_t marker_updates[] = {
        0xabcdef0123456789ULL,
        0x0123456789abcdefULL,
        0x13579bdf2468ace0ULL,
    };
    const uint64_t marker_guards[] = {
        0x5a5aa5a55a5aa5a5ULL,
        0xc3c33c3cc3c33c3cULL,
        0xf0f00f0ff0f00f0fULL,
    };
    uintptr_t alias_page_base_vaddr = 0;
    uintptr_t target_alias_vaddr = 0;
    uintptr_t cross16_load_alias_vaddr = 0;
    uintptr_t low_word_alias_vaddr = 0;
    uintptr_t high_word_alias_vaddr = 0;
    uintptr_t inval_alias_vaddr = 0;
    uintptr_t first_zero_alias_vaddr = 0;
    uintptr_t second_zero_alias_vaddr = 0;
    uintptr_t before_guard_alias_vaddr = 0;
    uintptr_t after_guard_alias_vaddr = 0;
    uintptr_t adjacent_alias_vaddr = 0;
    uintptr_t marker_alias_vaddr = 0;
    uintptr_t marker_guard_alias_vaddr = 0;

    ai_p13ax_prepare_hs_env(MENVCFG_CBIE01 | MENVCFG_CBZE);
    alias_page_base_vaddr =
        ai_p13ax_prepare_same_paddr_alias(alias_page, base_page);
    target_alias_vaddr = alias_page_base_vaddr + target_offset;
    cross16_load_alias_vaddr = target_alias_vaddr + 15U;
    low_word_alias_vaddr = target_alias_vaddr + sizeof(uint64_t);
    high_word_alias_vaddr = target_alias_vaddr + 2U * sizeof(uint64_t);
    inval_alias_vaddr =
        target_alias_vaddr + 6U * sizeof(uint64_t);
    first_zero_alias_vaddr =
        target_alias_vaddr + 4U * sizeof(uint64_t);
    second_zero_alias_vaddr =
        target_alias_vaddr + (AI_P13AX_BLOCK_WORDS - 1U) *
        sizeof(uint64_t);
    before_guard_alias_vaddr =
        target_alias_vaddr - sizeof(uint64_t);
    after_guard_alias_vaddr =
        target_alias_vaddr + 2U * AI_P13AX_BLOCK_BYTES;
    adjacent_alias_vaddr =
        target_alias_vaddr + AI_P13AX_BLOCK_BYTES;
    marker_alias_vaddr =
        target_alias_vaddr + 3U * AI_P13AX_BLOCK_BYTES;
    marker_guard_alias_vaddr =
        marker_alias_vaddr + sizeof(uint64_t);

    for (size_t round = 0; round < sizeof(block_seeds) / sizeof(block_seeds[0]);
         ++round) {
        const uint64_t seed_word1 =
            ai_p13ax_seed_word(block_seeds[round], 1U);
        const uint64_t seed_word2 =
            ai_p13ax_seed_word(block_seeds[round], 2U);
        const uint64_t expected_seed_merge =
            ai_p13ay_cross16_load_from_seed(seed_word1, seed_word2);
        const uint64_t expected_first_zero_seed =
            ai_p13ax_seed_word(block_seeds[round], 4U);
        const uint64_t expected_second_zero_seed =
            ai_p13ax_seed_word(block_seeds[round], AI_P13AX_BLOCK_WORDS - 1U);
        uint64_t load_observed = 0xffffffffffffffffULL;
        uint64_t marker_observed = 0ULL;

        goto_priv(PRIV_M);
        ai_p13ax_seed_block(target_base_paddr, block_seeds[round]);
        ai_p13ax_seed_block(adjacent_base_paddr, adjacent_seeds[round]);
        write64(before_guard_paddr, before_guards[round]);
        write64(after_guard_paddr, after_guards[round]);
        write64(marker_paddr, marker_seeds[round]);
        write64(marker_guard_paddr, marker_guards[round]);

        goto_priv(PRIV_HS);
        TEST_SETUP_EXCEPT();
        AI_P13AX_ASSERT_NO_EXCEPT_AND("before the same-paddr-alias inval prefetch repeated-zero cross-16B load handoff starts, base and alias views expose seeded split words, zero probes, adjacent block, marker, and guards",
            ld(target_base_vaddr) == block_seeds[round] &&
            ld(target_alias_vaddr) == block_seeds[round] &&
            ld(low_word_base_vaddr) == seed_word1 &&
            ld(low_word_alias_vaddr) == seed_word1 &&
            ld(high_word_base_vaddr) == seed_word2 &&
            ld(high_word_alias_vaddr) == seed_word2 &&
            ld(first_zero_base_vaddr) == expected_first_zero_seed &&
            ld(first_zero_alias_vaddr) == expected_first_zero_seed &&
            ld(second_zero_base_vaddr) == expected_second_zero_seed &&
            ld(second_zero_alias_vaddr) == expected_second_zero_seed &&
            ld(adjacent_base_vaddr) == adjacent_seeds[round] &&
            ld(adjacent_alias_vaddr) == adjacent_seeds[round] &&
            ld(marker_base_vaddr) == marker_seeds[round] &&
            ld(marker_alias_vaddr) == marker_seeds[round]
        );

        TEST_SETUP_EXCEPT();
        ai_p13ay_cbo_inval_prefetch_zero_zero_cross16_load_window(
            inval_alias_vaddr, prefetch_base_vaddr,
            first_zero_alias_vaddr, second_zero_alias_vaddr,
            cross16_load_alias_vaddr, marker_alias_vaddr,
            marker_updates[round], &load_observed, &marker_observed);
        AI_P13AX_ASSERT_NO_EXCEPT_AND("same-paddr alias repeated CBO.ZERO must leave a zero image for the following alias-view cross-16B load merge and still allow marker progress",
            load_observed == 0ULL &&
            load_observed != expected_seed_merge &&
            marker_observed == marker_updates[round] &&
            ld(low_word_base_vaddr) == 0ULL &&
            ld(low_word_alias_vaddr) == 0ULL &&
            ld(high_word_base_vaddr) == 0ULL &&
            ld(high_word_alias_vaddr) == 0ULL &&
            ld(first_zero_base_vaddr) == 0ULL &&
            ld(first_zero_alias_vaddr) == 0ULL &&
            ld(second_zero_base_vaddr) == 0ULL &&
            ld(second_zero_alias_vaddr) == 0ULL &&
            ld(marker_base_vaddr) == marker_updates[round] &&
            ld(marker_alias_vaddr) == marker_updates[round]
        );

        TEST_SETUP_EXCEPT();
        AI_P13AX_ASSERT_NO_EXCEPT_AND("the same-paddr-alias repeated-zero to cross-16B load handoff preserves guards and the adjacent block through both views",
            ld(before_guard_base_vaddr) == before_guards[round] &&
            ld(before_guard_alias_vaddr) == before_guards[round] &&
            ld(after_guard_base_vaddr) == after_guards[round] &&
            ld(after_guard_alias_vaddr) == after_guards[round] &&
            ld(marker_guard_base_vaddr) == marker_guards[round] &&
            ld(marker_guard_alias_vaddr) == marker_guards[round] &&
            ld(adjacent_base_vaddr) == adjacent_seeds[round] &&
            ld(adjacent_alias_vaddr) == adjacent_seeds[round]
        );

        goto_priv(PRIV_M);
        TEST_ASSERT("backing memory after same-paddr-alias inval prefetch repeated-zero cross-16B load handoff contains a fully zeroed target block, marker update, and unchanged guards",
            ai_p13ax_block_matches_seed(adjacent_base_paddr,
                adjacent_seeds[round]) &&
            ai_p13ax_zero_block_matches_cross16_store(target_base_paddr,
                0ULL) &&
            read64(before_guard_paddr) == before_guards[round] &&
            read64(after_guard_paddr) == after_guards[round] &&
            read64(marker_paddr) == marker_updates[round] &&
            read64(marker_guard_paddr) == marker_guards[round]
        );
    }

    ai_p13ax_reset_hs_env(MENVCFG_CBIE01 | MENVCFG_CBZE);

    TEST_END();
}

static inline bool ai_p13az_block_all_zero_except_word(
    uintptr_t paddr,
    uintptr_t changed_word_idx,
    uint64_t changed_value)
{
    for (uintptr_t i = 0; i < AI_P13AX_BLOCK_WORDS; ++i) {
        const uint64_t expected = (i == changed_word_idx) ? changed_value : 0ULL;

        if (read64(paddr + (i * sizeof(uint64_t))) != expected) {
            return false;
        }
    }

    return true;
}

static inline void ai_p13az_cbo_inval_prefetch_zero_zero_cross16_load_amo_window(
    uintptr_t inval_vaddr,
    uintptr_t prefetch_vaddr,
    uintptr_t first_zero_vaddr,
    uintptr_t second_zero_vaddr,
    uintptr_t cross16_load_vaddr,
    uintptr_t amo_vaddr,
    uintptr_t marker_vaddr,
    uint64_t first_addend,
    uint64_t second_addend,
    uint64_t marker_value,
    uint64_t *load_observed,
    uint64_t *first_amo_old,
    uint64_t *second_amo_old,
    uint64_t *marker_observed)
{
    uint64_t load_readback = 0xffffffffffffffffULL;
    uint64_t first_old = 0xffffffffffffffffULL;
    uint64_t second_old = 0xffffffffffffffffULL;
    uint64_t marker_readback = 0ULL;

    asm volatile(
        ".option push\n\t"
        ".option norvc\n\t"
        ".insn i 0x0f, 0x2, x0, %[inval_addr], 0\n\t"
        ".insn i 0x13, 0x6, x0, %[prefetch_addr], 1\n\t"
        ".insn i 0x0f, 0x2, x0, %[first_zero_addr], 4\n\t"
        ".insn i 0x0f, 0x2, x0, %[second_zero_addr], 4\n\t"
        "ld %[load_readback], 0(%[load_addr])\n\t"
        "amoadd.d %[first_old], %[first_addend], (%[amo_addr])\n\t"
        "amoadd.d %[second_old], %[second_addend], (%[amo_addr])\n\t"
        "sd %[marker_value], 0(%[marker_addr])\n\t"
        "fence rw, rw\n\t"
        "ld %[marker_readback], 0(%[marker_addr])\n\t"
        ".option pop\n\t"
        : [load_readback] "=&r"(load_readback),
          [first_old] "=&r"(first_old),
          [second_old] "=&r"(second_old),
          [marker_readback] "=&r"(marker_readback)
        : [inval_addr] "r"(inval_vaddr),
          [prefetch_addr] "r"(prefetch_vaddr),
          [first_zero_addr] "r"(first_zero_vaddr),
          [second_zero_addr] "r"(second_zero_vaddr),
          [load_addr] "r"(cross16_load_vaddr),
          [amo_addr] "r"(amo_vaddr),
          [first_addend] "r"(first_addend),
          [second_addend] "r"(second_addend),
          [marker_addr] "r"(marker_vaddr),
          [marker_value] "r"(marker_value)
        : "memory");

    *load_observed = load_readback;
    *first_amo_old = first_old;
    *second_amo_old = second_old;
    *marker_observed = marker_readback;
}

bool ai_micro_hs_cbo_inval_prefetch_same_paddr_alias_refill_then_cbo_zero_cbo_zero_cross16_load_amo_no_stale_repeated_zero_merge_or_atomic_owner_corner()
{
    TEST_START();

    const enum test_page base_page = SWITCH1;
    const enum test_page alias_page = SWITCH2;
    const uintptr_t target_offset = 0xc40U;
    const uintptr_t target_base_vaddr =
        hs_page_base(base_page) + target_offset;
    const uintptr_t target_base_paddr =
        phys_page_base(base_page) + target_offset;
    const uintptr_t low_word_base_vaddr =
        target_base_vaddr + sizeof(uint64_t);
    const uintptr_t high_word_base_vaddr =
        target_base_vaddr + 2U * sizeof(uint64_t);
    const uintptr_t amo_base_vaddr =
        target_base_vaddr + 3U * sizeof(uint64_t);
    const uintptr_t prefetch_base_vaddr =
        target_base_vaddr + sizeof(uint64_t);
    const uintptr_t first_zero_base_vaddr =
        target_base_vaddr + 4U * sizeof(uint64_t);
    const uintptr_t second_zero_base_vaddr =
        target_base_vaddr + (AI_P13AX_BLOCK_WORDS - 1U) *
        sizeof(uint64_t);
    const uintptr_t before_guard_base_vaddr =
        target_base_vaddr - sizeof(uint64_t);
    const uintptr_t after_guard_base_vaddr =
        target_base_vaddr + 2U * AI_P13AX_BLOCK_BYTES;
    const uintptr_t adjacent_base_vaddr =
        target_base_vaddr + AI_P13AX_BLOCK_BYTES;
    const uintptr_t marker_base_vaddr =
        target_base_vaddr + 3U * AI_P13AX_BLOCK_BYTES;
    const uintptr_t marker_guard_base_vaddr =
        marker_base_vaddr + sizeof(uint64_t);
    const uintptr_t before_guard_paddr =
        target_base_paddr - sizeof(uint64_t);
    const uintptr_t after_guard_paddr =
        target_base_paddr + 2U * AI_P13AX_BLOCK_BYTES;
    const uintptr_t adjacent_base_paddr =
        target_base_paddr + AI_P13AX_BLOCK_BYTES;
    const uintptr_t marker_paddr =
        target_base_paddr + 3U * AI_P13AX_BLOCK_BYTES;
    const uintptr_t marker_guard_paddr =
        marker_paddr + sizeof(uint64_t);
    const uint64_t block_seeds[] = {
        0x97a8b9cadbecfd0eULL,
        0xa8b9cadbecfd0e1fULL,
        0xb9cadbecfd0e1f20ULL,
    };
    const uint64_t adjacent_seeds[] = {
        0x0123456789abcdefULL,
        0xfedcba9876543210ULL,
        0x55aa7733cc11ee99ULL,
    };
    const uint64_t first_addends[] = {
        0x0000000000004444ULL,
        0x0000000000005555ULL,
        0x0000000000006666ULL,
    };
    const uint64_t second_addends[] = {
        0x0004000400040004ULL,
        0x0005000500050005ULL,
        0x0006000600060006ULL,
    };
    const uint64_t before_guards[] = {
        0x2718281828459045ULL,
        0x3141592653589793ULL,
        0x02468ace13579bdfULL,
    };
    const uint64_t after_guards[] = {
        0x1020304050607080ULL,
        0x0badf00d5a5aa5a5ULL,
        0x1123581321345589ULL,
    };
    const uint64_t marker_seeds[] = {
        0xaaaabbbbccccddddULL,
        0xbbbbccccddddeeeeULL,
        0xccccddddeeeeffffULL,
    };
    const uint64_t marker_updates[] = {
        0x456789abcdef0123ULL,
        0x3210fedcba987654ULL,
        0x5a5aa5a50badf00dULL,
    };
    const uint64_t marker_guards[] = {
        0xf0f00f0ff0f00f0fULL,
        0xc3c33c3cc3c33c3cULL,
        0x5a5aa5a55a5aa5a5ULL,
    };
    uintptr_t alias_page_base_vaddr = 0;
    uintptr_t target_alias_vaddr = 0;
    uintptr_t cross16_load_alias_vaddr = 0;
    uintptr_t low_word_alias_vaddr = 0;
    uintptr_t high_word_alias_vaddr = 0;
    uintptr_t amo_alias_vaddr = 0;
    uintptr_t inval_alias_vaddr = 0;
    uintptr_t first_zero_alias_vaddr = 0;
    uintptr_t second_zero_alias_vaddr = 0;
    uintptr_t before_guard_alias_vaddr = 0;
    uintptr_t after_guard_alias_vaddr = 0;
    uintptr_t adjacent_alias_vaddr = 0;
    uintptr_t marker_alias_vaddr = 0;
    uintptr_t marker_guard_alias_vaddr = 0;

    ai_p13ax_prepare_hs_env(MENVCFG_CBIE01 | MENVCFG_CBZE);
    alias_page_base_vaddr =
        ai_p13ax_prepare_same_paddr_alias(alias_page, base_page);
    target_alias_vaddr = alias_page_base_vaddr + target_offset;
    cross16_load_alias_vaddr = target_alias_vaddr + 15U;
    low_word_alias_vaddr = target_alias_vaddr + sizeof(uint64_t);
    high_word_alias_vaddr = target_alias_vaddr + 2U * sizeof(uint64_t);
    amo_alias_vaddr = target_alias_vaddr + 3U * sizeof(uint64_t);
    inval_alias_vaddr =
        target_alias_vaddr + 6U * sizeof(uint64_t);
    first_zero_alias_vaddr =
        target_alias_vaddr + 4U * sizeof(uint64_t);
    second_zero_alias_vaddr =
        target_alias_vaddr + (AI_P13AX_BLOCK_WORDS - 1U) *
        sizeof(uint64_t);
    before_guard_alias_vaddr =
        target_alias_vaddr - sizeof(uint64_t);
    after_guard_alias_vaddr =
        target_alias_vaddr + 2U * AI_P13AX_BLOCK_BYTES;
    adjacent_alias_vaddr =
        target_alias_vaddr + AI_P13AX_BLOCK_BYTES;
    marker_alias_vaddr =
        target_alias_vaddr + 3U * AI_P13AX_BLOCK_BYTES;
    marker_guard_alias_vaddr =
        marker_alias_vaddr + sizeof(uint64_t);

    for (size_t round = 0; round < sizeof(block_seeds) / sizeof(block_seeds[0]);
         ++round) {
        const uint64_t seed_word1 =
            ai_p13ax_seed_word(block_seeds[round], 1U);
        const uint64_t seed_word2 =
            ai_p13ax_seed_word(block_seeds[round], 2U);
        const uint64_t seed_word3 =
            ai_p13ax_seed_word(block_seeds[round], 3U);
        const uint64_t expected_seed_merge =
            ai_p13ay_cross16_load_from_seed(seed_word1, seed_word2);
        const uint64_t expected_first_zero_seed =
            ai_p13ax_seed_word(block_seeds[round], 4U);
        const uint64_t expected_second_zero_seed =
            ai_p13ax_seed_word(block_seeds[round], AI_P13AX_BLOCK_WORDS - 1U);
        const uint64_t after_first_amo = first_addends[round];
        const uint64_t final_amo =
            first_addends[round] + second_addends[round];
        uint64_t load_observed = 0xffffffffffffffffULL;
        uint64_t first_amo_old = 0xffffffffffffffffULL;
        uint64_t second_amo_old = 0xffffffffffffffffULL;
        uint64_t marker_observed = 0ULL;

        goto_priv(PRIV_M);
        ai_p13ax_seed_block(target_base_paddr, block_seeds[round]);
        ai_p13ax_seed_block(adjacent_base_paddr, adjacent_seeds[round]);
        write64(before_guard_paddr, before_guards[round]);
        write64(after_guard_paddr, after_guards[round]);
        write64(marker_paddr, marker_seeds[round]);
        write64(marker_guard_paddr, marker_guards[round]);

        goto_priv(PRIV_HS);
        TEST_SETUP_EXCEPT();
        AI_P13AX_ASSERT_NO_EXCEPT_AND("before the same-paddr-alias inval prefetch repeated-zero cross-16B load plus AMO handoff starts, base and alias views expose seeded split words, AMO word, zero probes, adjacent block, marker, and guards",
            ld(target_base_vaddr) == block_seeds[round] &&
            ld(target_alias_vaddr) == block_seeds[round] &&
            ld(low_word_base_vaddr) == seed_word1 &&
            ld(low_word_alias_vaddr) == seed_word1 &&
            ld(high_word_base_vaddr) == seed_word2 &&
            ld(high_word_alias_vaddr) == seed_word2 &&
            ld(amo_base_vaddr) == seed_word3 &&
            ld(amo_alias_vaddr) == seed_word3 &&
            ld(first_zero_base_vaddr) == expected_first_zero_seed &&
            ld(first_zero_alias_vaddr) == expected_first_zero_seed &&
            ld(second_zero_base_vaddr) == expected_second_zero_seed &&
            ld(second_zero_alias_vaddr) == expected_second_zero_seed &&
            ld(adjacent_base_vaddr) == adjacent_seeds[round] &&
            ld(adjacent_alias_vaddr) == adjacent_seeds[round] &&
            ld(marker_base_vaddr) == marker_seeds[round] &&
            ld(marker_alias_vaddr) == marker_seeds[round]
        );

        TEST_SETUP_EXCEPT();
        ai_p13az_cbo_inval_prefetch_zero_zero_cross16_load_amo_window(
            inval_alias_vaddr, prefetch_base_vaddr,
            first_zero_alias_vaddr, second_zero_alias_vaddr,
            cross16_load_alias_vaddr, amo_alias_vaddr, marker_alias_vaddr,
            first_addends[round], second_addends[round],
            marker_updates[round], &load_observed, &first_amo_old,
            &second_amo_old, &marker_observed);
        AI_P13AX_ASSERT_NO_EXCEPT_AND("same-paddr alias repeated CBO.ZERO must feed a zero split-load merge before the following alias-view AMOs see and update only the zeroed AMO word",
            load_observed == 0ULL &&
            load_observed != expected_seed_merge &&
            first_amo_old == 0ULL &&
            first_amo_old != seed_word3 &&
            second_amo_old == after_first_amo &&
            marker_observed == marker_updates[round] &&
            ld(low_word_base_vaddr) == 0ULL &&
            ld(low_word_alias_vaddr) == 0ULL &&
            ld(high_word_base_vaddr) == 0ULL &&
            ld(high_word_alias_vaddr) == 0ULL &&
            ld(amo_base_vaddr) == final_amo &&
            ld(amo_alias_vaddr) == final_amo &&
            ld(first_zero_base_vaddr) == 0ULL &&
            ld(first_zero_alias_vaddr) == 0ULL &&
            ld(second_zero_base_vaddr) == 0ULL &&
            ld(second_zero_alias_vaddr) == 0ULL &&
            ld(marker_base_vaddr) == marker_updates[round] &&
            ld(marker_alias_vaddr) == marker_updates[round]
        );

        TEST_SETUP_EXCEPT();
        AI_P13AX_ASSERT_NO_EXCEPT_AND("the same-paddr-alias repeated-zero to split-load plus AMO handoff preserves guards and the adjacent block through both views",
            ld(before_guard_base_vaddr) == before_guards[round] &&
            ld(before_guard_alias_vaddr) == before_guards[round] &&
            ld(after_guard_base_vaddr) == after_guards[round] &&
            ld(after_guard_alias_vaddr) == after_guards[round] &&
            ld(marker_guard_base_vaddr) == marker_guards[round] &&
            ld(marker_guard_alias_vaddr) == marker_guards[round] &&
            ld(adjacent_base_vaddr) == adjacent_seeds[round] &&
            ld(adjacent_alias_vaddr) == adjacent_seeds[round]
        );

        goto_priv(PRIV_M);
        TEST_ASSERT("backing memory after same-paddr-alias inval prefetch repeated-zero cross-16B load plus AMO handoff contains only the final AMO result over an otherwise zeroed block, marker update, and unchanged guards",
            ai_p13az_block_all_zero_except_word(target_base_paddr,
                3U, final_amo) &&
            ai_p13ax_block_matches_seed(adjacent_base_paddr,
                adjacent_seeds[round]) &&
            read64(before_guard_paddr) == before_guards[round] &&
            read64(after_guard_paddr) == after_guards[round] &&
            read64(marker_paddr) == marker_updates[round] &&
            read64(marker_guard_paddr) == marker_guards[round]
        );
    }

    ai_p13ax_reset_hs_env(MENVCFG_CBIE01 | MENVCFG_CBZE);

    TEST_END();
}

static inline void ai_p13ba_cbo_inval_prefetch_zero_zero_cross16_load_lrsc_window(
    uintptr_t inval_vaddr,
    uintptr_t prefetch_vaddr,
    uintptr_t first_zero_vaddr,
    uintptr_t second_zero_vaddr,
    uintptr_t cross16_load_vaddr,
    uintptr_t lrsc_vaddr,
    uintptr_t marker_vaddr,
    uint64_t sc_value,
    uint64_t marker_value,
    uint64_t *load_observed,
    uint64_t *lr_observed,
    uint64_t *sc_status,
    uint64_t *marker_observed)
{
    uint64_t load_readback = 0xffffffffffffffffULL;
    uint64_t lr_readback = 0xffffffffffffffffULL;
    uint64_t status = 1ULL;
    uint64_t marker_readback = 0ULL;

    asm volatile(
        ".option push\n\t"
        ".option norvc\n\t"
        ".insn i 0x0f, 0x2, x0, %[inval_addr], 0\n\t"
        ".insn i 0x13, 0x6, x0, %[prefetch_addr], 1\n\t"
        ".insn i 0x0f, 0x2, x0, %[first_zero_addr], 4\n\t"
        ".insn i 0x0f, 0x2, x0, %[second_zero_addr], 4\n\t"
        "ld %[load_readback], 0(%[load_addr])\n\t"
        "lr.d %[lr_readback], 0(%[lrsc_addr])\n\t"
        "sc.d %[status], %[sc_value], 0(%[lrsc_addr])\n\t"
        "sd %[marker_value], 0(%[marker_addr])\n\t"
        "fence rw, rw\n\t"
        "ld %[marker_readback], 0(%[marker_addr])\n\t"
        ".option pop\n\t"
        : [load_readback] "=&r"(load_readback),
          [lr_readback] "=&r"(lr_readback),
          [status] "=&r"(status),
          [marker_readback] "=&r"(marker_readback)
        : [inval_addr] "r"(inval_vaddr),
          [prefetch_addr] "r"(prefetch_vaddr),
          [first_zero_addr] "r"(first_zero_vaddr),
          [second_zero_addr] "r"(second_zero_vaddr),
          [load_addr] "r"(cross16_load_vaddr),
          [lrsc_addr] "r"(lrsc_vaddr),
          [sc_value] "r"(sc_value),
          [marker_addr] "r"(marker_vaddr),
          [marker_value] "r"(marker_value)
        : "memory");

    *load_observed = load_readback;
    *lr_observed = lr_readback;
    *sc_status = status;
    *marker_observed = marker_readback;
}

bool ai_micro_hs_cbo_inval_prefetch_same_paddr_alias_refill_then_cbo_zero_cbo_zero_cross16_load_lrsc_no_stale_repeated_zero_merge_or_reservation_loss_corner()
{
    TEST_START();

    const enum test_page base_page = SWITCH1;
    const enum test_page alias_page = SWITCH2;
    const uintptr_t target_offset = 0xc80U;
    const uintptr_t target_base_vaddr =
        hs_page_base(base_page) + target_offset;
    const uintptr_t target_base_paddr =
        phys_page_base(base_page) + target_offset;
    const uintptr_t low_word_base_vaddr =
        target_base_vaddr + sizeof(uint64_t);
    const uintptr_t high_word_base_vaddr =
        target_base_vaddr + 2U * sizeof(uint64_t);
    const uintptr_t lrsc_base_vaddr =
        target_base_vaddr + 3U * sizeof(uint64_t);
    const uintptr_t prefetch_base_vaddr =
        target_base_vaddr + sizeof(uint64_t);
    const uintptr_t first_zero_base_vaddr =
        target_base_vaddr + 4U * sizeof(uint64_t);
    const uintptr_t second_zero_base_vaddr =
        target_base_vaddr + (AI_P13AX_BLOCK_WORDS - 1U) *
        sizeof(uint64_t);
    const uintptr_t before_guard_base_vaddr =
        target_base_vaddr - sizeof(uint64_t);
    const uintptr_t after_guard_base_vaddr =
        target_base_vaddr + 2U * AI_P13AX_BLOCK_BYTES;
    const uintptr_t adjacent_base_vaddr =
        target_base_vaddr + AI_P13AX_BLOCK_BYTES;
    const uintptr_t marker_base_vaddr =
        target_base_vaddr + 3U * AI_P13AX_BLOCK_BYTES;
    const uintptr_t marker_guard_base_vaddr =
        marker_base_vaddr + sizeof(uint64_t);
    const uintptr_t before_guard_paddr =
        target_base_paddr - sizeof(uint64_t);
    const uintptr_t after_guard_paddr =
        target_base_paddr + 2U * AI_P13AX_BLOCK_BYTES;
    const uintptr_t adjacent_base_paddr =
        target_base_paddr + AI_P13AX_BLOCK_BYTES;
    const uintptr_t marker_paddr =
        target_base_paddr + 3U * AI_P13AX_BLOCK_BYTES;
    const uintptr_t marker_guard_paddr =
        marker_paddr + sizeof(uint64_t);
    const uint64_t block_seeds[] = {
        0xcadbecfd0e1f2031ULL,
        0xdbecfd0e1f203142ULL,
        0xecfd0e1f20314253ULL,
    };
    const uint64_t adjacent_seeds[] = {
        0x67452301efcdab89ULL,
        0x89abcdef01234567ULL,
        0x76543210fedcba98ULL,
    };
    const uint64_t sc_payloads[] = {
        0x11220000aabbccddULL,
        0x22331111bbccddeeULL,
        0x33442222ccddeeffULL,
    };
    const uint64_t before_guards[] = {
        0x13579bdf2468ace0ULL,
        0x02468ace13579bdfULL,
        0xfdb97531eca86420ULL,
    };
    const uint64_t after_guards[] = {
        0x0badf00d5a5aa5a5ULL,
        0x1020304050607080ULL,
        0x1122334455667788ULL,
    };
    const uint64_t marker_seeds[] = {
        0x1111222233334444ULL,
        0x5555666677778888ULL,
        0x9999aaaabbbbccccULL,
    };
    const uint64_t marker_updates[] = {
        0xabcdef0123456789ULL,
        0x0123456789abcdefULL,
        0x13579bdf2468ace0ULL,
    };
    const uint64_t marker_guards[] = {
        0x5a5aa5a55a5aa5a5ULL,
        0xc3c33c3cc3c33c3cULL,
        0xf0f00f0ff0f00f0fULL,
    };
    uintptr_t alias_page_base_vaddr = 0;
    uintptr_t target_alias_vaddr = 0;
    uintptr_t cross16_load_alias_vaddr = 0;
    uintptr_t low_word_alias_vaddr = 0;
    uintptr_t high_word_alias_vaddr = 0;
    uintptr_t lrsc_alias_vaddr = 0;
    uintptr_t inval_alias_vaddr = 0;
    uintptr_t first_zero_alias_vaddr = 0;
    uintptr_t second_zero_alias_vaddr = 0;
    uintptr_t before_guard_alias_vaddr = 0;
    uintptr_t after_guard_alias_vaddr = 0;
    uintptr_t adjacent_alias_vaddr = 0;
    uintptr_t marker_alias_vaddr = 0;
    uintptr_t marker_guard_alias_vaddr = 0;

    ai_p13ax_prepare_hs_env(MENVCFG_CBIE01 | MENVCFG_CBZE);
    alias_page_base_vaddr =
        ai_p13ax_prepare_same_paddr_alias(alias_page, base_page);
    target_alias_vaddr = alias_page_base_vaddr + target_offset;
    cross16_load_alias_vaddr = target_alias_vaddr + 15U;
    low_word_alias_vaddr = target_alias_vaddr + sizeof(uint64_t);
    high_word_alias_vaddr = target_alias_vaddr + 2U * sizeof(uint64_t);
    lrsc_alias_vaddr = target_alias_vaddr + 3U * sizeof(uint64_t);
    inval_alias_vaddr =
        target_alias_vaddr + 6U * sizeof(uint64_t);
    first_zero_alias_vaddr =
        target_alias_vaddr + 4U * sizeof(uint64_t);
    second_zero_alias_vaddr =
        target_alias_vaddr + (AI_P13AX_BLOCK_WORDS - 1U) *
        sizeof(uint64_t);
    before_guard_alias_vaddr =
        target_alias_vaddr - sizeof(uint64_t);
    after_guard_alias_vaddr =
        target_alias_vaddr + 2U * AI_P13AX_BLOCK_BYTES;
    adjacent_alias_vaddr =
        target_alias_vaddr + AI_P13AX_BLOCK_BYTES;
    marker_alias_vaddr =
        target_alias_vaddr + 3U * AI_P13AX_BLOCK_BYTES;
    marker_guard_alias_vaddr =
        marker_alias_vaddr + sizeof(uint64_t);

    for (size_t round = 0; round < sizeof(block_seeds) / sizeof(block_seeds[0]);
         ++round) {
        const uint64_t seed_word1 =
            ai_p13ax_seed_word(block_seeds[round], 1U);
        const uint64_t seed_word2 =
            ai_p13ax_seed_word(block_seeds[round], 2U);
        const uint64_t seed_word3 =
            ai_p13ax_seed_word(block_seeds[round], 3U);
        const uint64_t expected_seed_merge =
            ai_p13ay_cross16_load_from_seed(seed_word1, seed_word2);
        const uint64_t expected_first_zero_seed =
            ai_p13ax_seed_word(block_seeds[round], 4U);
        const uint64_t expected_second_zero_seed =
            ai_p13ax_seed_word(block_seeds[round], AI_P13AX_BLOCK_WORDS - 1U);
        uint64_t load_observed = 0xffffffffffffffffULL;
        uint64_t lr_observed = 0xffffffffffffffffULL;
        uint64_t sc_status = 1ULL;
        uint64_t marker_observed = 0ULL;

        goto_priv(PRIV_M);
        ai_p13ax_seed_block(target_base_paddr, block_seeds[round]);
        ai_p13ax_seed_block(adjacent_base_paddr, adjacent_seeds[round]);
        write64(before_guard_paddr, before_guards[round]);
        write64(after_guard_paddr, after_guards[round]);
        write64(marker_paddr, marker_seeds[round]);
        write64(marker_guard_paddr, marker_guards[round]);

        goto_priv(PRIV_HS);
        TEST_SETUP_EXCEPT();
        AI_P13AX_ASSERT_NO_EXCEPT_AND("before the same-paddr-alias inval prefetch repeated-zero cross-16B load plus LR/SC handoff starts, base and alias views expose seeded split words, LR/SC word, zero probes, adjacent block, marker, and guards",
            ld(target_base_vaddr) == block_seeds[round] &&
            ld(target_alias_vaddr) == block_seeds[round] &&
            ld(low_word_base_vaddr) == seed_word1 &&
            ld(low_word_alias_vaddr) == seed_word1 &&
            ld(high_word_base_vaddr) == seed_word2 &&
            ld(high_word_alias_vaddr) == seed_word2 &&
            ld(lrsc_base_vaddr) == seed_word3 &&
            ld(lrsc_alias_vaddr) == seed_word3 &&
            ld(first_zero_base_vaddr) == expected_first_zero_seed &&
            ld(first_zero_alias_vaddr) == expected_first_zero_seed &&
            ld(second_zero_base_vaddr) == expected_second_zero_seed &&
            ld(second_zero_alias_vaddr) == expected_second_zero_seed &&
            ld(adjacent_base_vaddr) == adjacent_seeds[round] &&
            ld(adjacent_alias_vaddr) == adjacent_seeds[round] &&
            ld(marker_base_vaddr) == marker_seeds[round] &&
            ld(marker_alias_vaddr) == marker_seeds[round]
        );

        TEST_SETUP_EXCEPT();
        ai_p13ba_cbo_inval_prefetch_zero_zero_cross16_load_lrsc_window(
            inval_alias_vaddr, prefetch_base_vaddr,
            first_zero_alias_vaddr, second_zero_alias_vaddr,
            cross16_load_alias_vaddr, lrsc_alias_vaddr, marker_alias_vaddr,
            sc_payloads[round], marker_updates[round], &load_observed,
            &lr_observed, &sc_status, &marker_observed);
        AI_P13AX_ASSERT_NO_EXCEPT_AND("same-paddr alias repeated CBO.ZERO must feed a zero split-load merge before the following alias-view LR/SC sees zero and preserves the SC reservation",
            load_observed == 0ULL &&
            load_observed != expected_seed_merge &&
            lr_observed == 0ULL &&
            lr_observed != seed_word3 &&
            sc_status == 0ULL &&
            marker_observed == marker_updates[round] &&
            ld(low_word_base_vaddr) == 0ULL &&
            ld(low_word_alias_vaddr) == 0ULL &&
            ld(high_word_base_vaddr) == 0ULL &&
            ld(high_word_alias_vaddr) == 0ULL &&
            ld(lrsc_base_vaddr) == sc_payloads[round] &&
            ld(lrsc_alias_vaddr) == sc_payloads[round] &&
            ld(first_zero_base_vaddr) == 0ULL &&
            ld(first_zero_alias_vaddr) == 0ULL &&
            ld(second_zero_base_vaddr) == 0ULL &&
            ld(second_zero_alias_vaddr) == 0ULL &&
            ld(marker_base_vaddr) == marker_updates[round] &&
            ld(marker_alias_vaddr) == marker_updates[round]
        );

        TEST_SETUP_EXCEPT();
        AI_P13AX_ASSERT_NO_EXCEPT_AND("the same-paddr-alias repeated-zero to split-load plus LR/SC handoff preserves guards and the adjacent block through both views",
            ld(before_guard_base_vaddr) == before_guards[round] &&
            ld(before_guard_alias_vaddr) == before_guards[round] &&
            ld(after_guard_base_vaddr) == after_guards[round] &&
            ld(after_guard_alias_vaddr) == after_guards[round] &&
            ld(marker_guard_base_vaddr) == marker_guards[round] &&
            ld(marker_guard_alias_vaddr) == marker_guards[round] &&
            ld(adjacent_base_vaddr) == adjacent_seeds[round] &&
            ld(adjacent_alias_vaddr) == adjacent_seeds[round]
        );

        goto_priv(PRIV_M);
        TEST_ASSERT("backing memory after same-paddr-alias inval prefetch repeated-zero cross-16B load plus LR/SC handoff contains only the successful SC payload over an otherwise zeroed block, marker update, and unchanged guards",
            ai_p13az_block_all_zero_except_word(target_base_paddr,
                3U, sc_payloads[round]) &&
            ai_p13ax_block_matches_seed(adjacent_base_paddr,
                adjacent_seeds[round]) &&
            read64(before_guard_paddr) == before_guards[round] &&
            read64(after_guard_paddr) == after_guards[round] &&
            read64(marker_paddr) == marker_updates[round] &&
            read64(marker_guard_paddr) == marker_guards[round]
        );
    }

    ai_p13ax_reset_hs_env(MENVCFG_CBIE01 | MENVCFG_CBZE);

    TEST_END();
}

static inline bool ai_p13bb_zero_block_matches_cross16_store_and_word(
    uintptr_t paddr,
    uint64_t payload,
    uintptr_t word_idx,
    uint64_t word_value)
{
    for (uintptr_t i = 0; i < AI_P13AX_BLOCK_WORDS; ++i) {
        uint64_t expected = 0ULL;

        if (i == 1U) {
            expected = ai_p13ax_cross16_low_word(payload);
        } else if (i == 2U) {
            expected = ai_p13ax_cross16_high_word(payload);
        } else if (i == word_idx) {
            expected = word_value;
        }

        if (read64(paddr + (i * sizeof(uint64_t))) != expected) {
            return false;
        }
    }

    return true;
}

static inline void ai_p13bb_cbo_inval_prefetch_zero_zero_cross16_store_lrsc_window(
    uintptr_t inval_vaddr,
    uintptr_t prefetch_vaddr,
    uintptr_t first_zero_vaddr,
    uintptr_t second_zero_vaddr,
    uintptr_t cross16_store_vaddr,
    uintptr_t lrsc_vaddr,
    uintptr_t marker_vaddr,
    uint64_t cross16_payload,
    uint64_t sc_value,
    uint64_t marker_value,
    uint64_t *lr_observed,
    uint64_t *sc_status,
    uint64_t *marker_observed)
{
    uint64_t lr_readback = 0xffffffffffffffffULL;
    uint64_t status = 1ULL;
    uint64_t marker_readback = 0ULL;

    asm volatile(
        ".option push\n\t"
        ".option norvc\n\t"
        ".insn i 0x0f, 0x2, x0, %[inval_addr], 0\n\t"
        ".insn i 0x13, 0x6, x0, %[prefetch_addr], 1\n\t"
        ".insn i 0x0f, 0x2, x0, %[first_zero_addr], 4\n\t"
        ".insn i 0x0f, 0x2, x0, %[second_zero_addr], 4\n\t"
        "sd %[cross16_payload], 0(%[cross16_addr])\n\t"
        "lr.d %[lr_readback], 0(%[lrsc_addr])\n\t"
        "sc.d %[status], %[sc_value], 0(%[lrsc_addr])\n\t"
        "sd %[marker_value], 0(%[marker_addr])\n\t"
        "fence rw, rw\n\t"
        "ld %[marker_readback], 0(%[marker_addr])\n\t"
        ".option pop\n\t"
        : [lr_readback] "=&r"(lr_readback),
          [status] "=&r"(status),
          [marker_readback] "=&r"(marker_readback)
        : [inval_addr] "r"(inval_vaddr),
          [prefetch_addr] "r"(prefetch_vaddr),
          [first_zero_addr] "r"(first_zero_vaddr),
          [second_zero_addr] "r"(second_zero_vaddr),
          [cross16_addr] "r"(cross16_store_vaddr),
          [cross16_payload] "r"(cross16_payload),
          [lrsc_addr] "r"(lrsc_vaddr),
          [sc_value] "r"(sc_value),
          [marker_addr] "r"(marker_vaddr),
          [marker_value] "r"(marker_value)
        : "memory");

    *lr_observed = lr_readback;
    *sc_status = status;
    *marker_observed = marker_readback;
}

bool ai_micro_hs_cbo_inval_prefetch_same_paddr_alias_refill_then_cbo_zero_cbo_zero_cross16_store_lrsc_no_stale_repeated_zero_split_payload_or_reservation_loss_corner()
{
    TEST_START();

    const enum test_page base_page = SWITCH1;
    const enum test_page alias_page = SWITCH2;
    const uintptr_t target_offset = 0xcc0U;
    const uintptr_t target_base_vaddr =
        hs_page_base(base_page) + target_offset;
    const uintptr_t target_base_paddr =
        phys_page_base(base_page) + target_offset;
    const uintptr_t low_word_base_vaddr =
        target_base_vaddr + sizeof(uint64_t);
    const uintptr_t high_word_base_vaddr =
        target_base_vaddr + 2U * sizeof(uint64_t);
    const uintptr_t lrsc_base_vaddr =
        target_base_vaddr + 3U * sizeof(uint64_t);
    const uintptr_t prefetch_base_vaddr =
        target_base_vaddr + sizeof(uint64_t);
    const uintptr_t first_zero_base_vaddr =
        target_base_vaddr + 4U * sizeof(uint64_t);
    const uintptr_t second_zero_base_vaddr =
        target_base_vaddr + (AI_P13AX_BLOCK_WORDS - 1U) *
        sizeof(uint64_t);
    const uintptr_t before_guard_base_vaddr =
        target_base_vaddr - sizeof(uint64_t);
    const uintptr_t after_guard_base_vaddr =
        target_base_vaddr + 2U * AI_P13AX_BLOCK_BYTES;
    const uintptr_t adjacent_base_vaddr =
        target_base_vaddr + AI_P13AX_BLOCK_BYTES;
    const uintptr_t marker_base_vaddr =
        target_base_vaddr + 3U * AI_P13AX_BLOCK_BYTES;
    const uintptr_t marker_guard_base_vaddr =
        marker_base_vaddr + sizeof(uint64_t);
    const uintptr_t before_guard_paddr =
        target_base_paddr - sizeof(uint64_t);
    const uintptr_t after_guard_paddr =
        target_base_paddr + 2U * AI_P13AX_BLOCK_BYTES;
    const uintptr_t adjacent_base_paddr =
        target_base_paddr + AI_P13AX_BLOCK_BYTES;
    const uintptr_t marker_paddr =
        target_base_paddr + 3U * AI_P13AX_BLOCK_BYTES;
    const uintptr_t marker_guard_paddr =
        marker_paddr + sizeof(uint64_t);
    const uint64_t block_seeds[] = {
        0x9ace02468bdf1357ULL,
        0xabdf13579ce02468ULL,
        0xbce02468adf13579ULL,
    };
    const uint64_t adjacent_seeds[] = {
        0x1029384756abcdefULL,
        0xf0e1d2c3b4a59687ULL,
        0x8877665544332211ULL,
    };
    const uint64_t cross16_payloads[] = {
        0x0123456789abcdefULL,
        0xfedcba9876543210ULL,
        0x8899aabbccddeeffULL,
    };
    const uint64_t sc_payloads[] = {
        0x0a0b0c0d0e0f1011ULL,
        0x1a1b1c1d1e1f2021ULL,
        0x2a2b2c2d2e2f3031ULL,
    };
    const uint64_t before_guards[] = {
        0x2468ace013579bdfULL,
        0xfdb97531eca86420ULL,
        0x13579bdf2468ace0ULL,
    };
    const uint64_t after_guards[] = {
        0x1123581321345589ULL,
        0x3141592653589793ULL,
        0x2718281828459045ULL,
    };
    const uint64_t marker_seeds[] = {
        0x0102030405060708ULL,
        0x1112131415161718ULL,
        0x2122232425262728ULL,
    };
    const uint64_t marker_updates[] = {
        0x89abcdef01234567ULL,
        0x76543210fedcba98ULL,
        0x55aa55aa33cc33ccULL,
    };
    const uint64_t marker_guards[] = {
        0xa5a55a5aa5a55a5aULL,
        0xc3c33c3cc3c33c3cULL,
        0xf0f00f0ff0f00f0fULL,
    };
    uintptr_t alias_page_base_vaddr = 0;
    uintptr_t target_alias_vaddr = 0;
    uintptr_t cross16_store_alias_vaddr = 0;
    uintptr_t low_word_alias_vaddr = 0;
    uintptr_t high_word_alias_vaddr = 0;
    uintptr_t lrsc_alias_vaddr = 0;
    uintptr_t inval_alias_vaddr = 0;
    uintptr_t first_zero_alias_vaddr = 0;
    uintptr_t second_zero_alias_vaddr = 0;
    uintptr_t before_guard_alias_vaddr = 0;
    uintptr_t after_guard_alias_vaddr = 0;
    uintptr_t adjacent_alias_vaddr = 0;
    uintptr_t marker_alias_vaddr = 0;
    uintptr_t marker_guard_alias_vaddr = 0;

    ai_p13ax_prepare_hs_env(MENVCFG_CBIE01 | MENVCFG_CBZE);
    alias_page_base_vaddr =
        ai_p13ax_prepare_same_paddr_alias(alias_page, base_page);
    target_alias_vaddr = alias_page_base_vaddr + target_offset;
    cross16_store_alias_vaddr = target_alias_vaddr + 15U;
    low_word_alias_vaddr = target_alias_vaddr + sizeof(uint64_t);
    high_word_alias_vaddr = target_alias_vaddr + 2U * sizeof(uint64_t);
    lrsc_alias_vaddr = target_alias_vaddr + 3U * sizeof(uint64_t);
    inval_alias_vaddr =
        target_alias_vaddr + 6U * sizeof(uint64_t);
    first_zero_alias_vaddr =
        target_alias_vaddr + 4U * sizeof(uint64_t);
    second_zero_alias_vaddr =
        target_alias_vaddr + (AI_P13AX_BLOCK_WORDS - 1U) *
        sizeof(uint64_t);
    before_guard_alias_vaddr =
        target_alias_vaddr - sizeof(uint64_t);
    after_guard_alias_vaddr =
        target_alias_vaddr + 2U * AI_P13AX_BLOCK_BYTES;
    adjacent_alias_vaddr =
        target_alias_vaddr + AI_P13AX_BLOCK_BYTES;
    marker_alias_vaddr =
        target_alias_vaddr + 3U * AI_P13AX_BLOCK_BYTES;
    marker_guard_alias_vaddr =
        marker_alias_vaddr + sizeof(uint64_t);

    for (size_t round = 0; round < sizeof(block_seeds) / sizeof(block_seeds[0]);
         ++round) {
        const uint64_t seed_word1 =
            ai_p13ax_seed_word(block_seeds[round], 1U);
        const uint64_t seed_word2 =
            ai_p13ax_seed_word(block_seeds[round], 2U);
        const uint64_t seed_word3 =
            ai_p13ax_seed_word(block_seeds[round], 3U);
        const uint64_t expected_first_zero_seed =
            ai_p13ax_seed_word(block_seeds[round], 4U);
        const uint64_t expected_second_zero_seed =
            ai_p13ax_seed_word(block_seeds[round], AI_P13AX_BLOCK_WORDS - 1U);
        const uint64_t expected_low_word =
            ai_p13ax_cross16_low_word(cross16_payloads[round]);
        const uint64_t expected_high_word =
            ai_p13ax_cross16_high_word(cross16_payloads[round]);
        uint64_t lr_observed = 0xffffffffffffffffULL;
        uint64_t sc_status = 1ULL;
        uint64_t marker_observed = 0ULL;

        goto_priv(PRIV_M);
        ai_p13ax_seed_block(target_base_paddr, block_seeds[round]);
        ai_p13ax_seed_block(adjacent_base_paddr, adjacent_seeds[round]);
        write64(before_guard_paddr, before_guards[round]);
        write64(after_guard_paddr, after_guards[round]);
        write64(marker_paddr, marker_seeds[round]);
        write64(marker_guard_paddr, marker_guards[round]);

        goto_priv(PRIV_HS);
        TEST_SETUP_EXCEPT();
        AI_P13AX_ASSERT_NO_EXCEPT_AND("before the same-paddr-alias inval prefetch repeated-zero cross-16B store plus LR/SC handoff starts, base and alias views expose seeded split words, LR/SC word, zero probes, adjacent block, marker, and guards",
            ld(target_base_vaddr) == block_seeds[round] &&
            ld(target_alias_vaddr) == block_seeds[round] &&
            ld(low_word_base_vaddr) == seed_word1 &&
            ld(low_word_alias_vaddr) == seed_word1 &&
            ld(high_word_base_vaddr) == seed_word2 &&
            ld(high_word_alias_vaddr) == seed_word2 &&
            ld(lrsc_base_vaddr) == seed_word3 &&
            ld(lrsc_alias_vaddr) == seed_word3 &&
            ld(first_zero_base_vaddr) == expected_first_zero_seed &&
            ld(first_zero_alias_vaddr) == expected_first_zero_seed &&
            ld(second_zero_base_vaddr) == expected_second_zero_seed &&
            ld(second_zero_alias_vaddr) == expected_second_zero_seed &&
            ld(adjacent_base_vaddr) == adjacent_seeds[round] &&
            ld(adjacent_alias_vaddr) == adjacent_seeds[round] &&
            ld(marker_base_vaddr) == marker_seeds[round] &&
            ld(marker_alias_vaddr) == marker_seeds[round]
        );

        TEST_SETUP_EXCEPT();
        ai_p13bb_cbo_inval_prefetch_zero_zero_cross16_store_lrsc_window(
            inval_alias_vaddr, prefetch_base_vaddr,
            first_zero_alias_vaddr, second_zero_alias_vaddr,
            cross16_store_alias_vaddr, lrsc_alias_vaddr, marker_alias_vaddr,
            cross16_payloads[round], sc_payloads[round],
            marker_updates[round], &lr_observed, &sc_status,
            &marker_observed);
        AI_P13AX_ASSERT_NO_EXCEPT_AND("same-paddr alias repeated CBO.ZERO must preserve the following split-store overlay before alias-view LR/SC sees zero and keeps the reservation",
            lr_observed == 0ULL &&
            lr_observed != seed_word3 &&
            sc_status == 0ULL &&
            marker_observed == marker_updates[round] &&
            ld(low_word_base_vaddr) == expected_low_word &&
            ld(low_word_alias_vaddr) == expected_low_word &&
            ld(high_word_base_vaddr) == expected_high_word &&
            ld(high_word_alias_vaddr) == expected_high_word &&
            ld(lrsc_base_vaddr) == sc_payloads[round] &&
            ld(lrsc_alias_vaddr) == sc_payloads[round] &&
            ld(first_zero_base_vaddr) == 0ULL &&
            ld(first_zero_alias_vaddr) == 0ULL &&
            ld(second_zero_base_vaddr) == 0ULL &&
            ld(second_zero_alias_vaddr) == 0ULL &&
            ld(marker_base_vaddr) == marker_updates[round] &&
            ld(marker_alias_vaddr) == marker_updates[round]
        );

        TEST_SETUP_EXCEPT();
        AI_P13AX_ASSERT_NO_EXCEPT_AND("the same-paddr-alias repeated-zero to split-store plus LR/SC handoff preserves guards and the adjacent block through both views",
            ld(before_guard_base_vaddr) == before_guards[round] &&
            ld(before_guard_alias_vaddr) == before_guards[round] &&
            ld(after_guard_base_vaddr) == after_guards[round] &&
            ld(after_guard_alias_vaddr) == after_guards[round] &&
            ld(marker_guard_base_vaddr) == marker_guards[round] &&
            ld(marker_guard_alias_vaddr) == marker_guards[round] &&
            ld(adjacent_base_vaddr) == adjacent_seeds[round] &&
            ld(adjacent_alias_vaddr) == adjacent_seeds[round]
        );

        goto_priv(PRIV_M);
        TEST_ASSERT("backing memory after same-paddr-alias inval prefetch repeated-zero cross-16B store plus LR/SC handoff contains only the split-store overlay and successful SC payload over an otherwise zeroed block, marker update, and unchanged guards",
            ai_p13bb_zero_block_matches_cross16_store_and_word(
                target_base_paddr, cross16_payloads[round],
                3U, sc_payloads[round]) &&
            ai_p13ax_block_matches_seed(adjacent_base_paddr,
                adjacent_seeds[round]) &&
            read64(before_guard_paddr) == before_guards[round] &&
            read64(after_guard_paddr) == after_guards[round] &&
            read64(marker_paddr) == marker_updates[round] &&
            read64(marker_guard_paddr) == marker_guards[round]
        );
    }

    ai_p13ax_reset_hs_env(MENVCFG_CBIE01 | MENVCFG_CBZE);

    TEST_END();
}

static inline void ai_p13bc_cbo_inval_prefetch_zero_zero_cross16_store_amo_window(
    uintptr_t inval_vaddr,
    uintptr_t prefetch_vaddr,
    uintptr_t first_zero_vaddr,
    uintptr_t second_zero_vaddr,
    uintptr_t cross16_store_vaddr,
    uintptr_t amo_vaddr,
    uintptr_t marker_vaddr,
    uint64_t cross16_payload,
    uint64_t first_addend,
    uint64_t second_addend,
    uint64_t marker_value,
    uint64_t *first_amo_old,
    uint64_t *second_amo_old,
    uint64_t *marker_observed)
{
    uint64_t first_old = 0xffffffffffffffffULL;
    uint64_t second_old = 0xffffffffffffffffULL;
    uint64_t marker_readback = 0ULL;

    asm volatile(
        ".option push\n\t"
        ".option norvc\n\t"
        ".insn i 0x0f, 0x2, x0, %[inval_addr], 0\n\t"
        ".insn i 0x13, 0x6, x0, %[prefetch_addr], 1\n\t"
        ".insn i 0x0f, 0x2, x0, %[first_zero_addr], 4\n\t"
        ".insn i 0x0f, 0x2, x0, %[second_zero_addr], 4\n\t"
        "sd %[cross16_payload], 0(%[cross16_addr])\n\t"
        "amoadd.d %[first_old], %[first_addend], (%[amo_addr])\n\t"
        "amoadd.d %[second_old], %[second_addend], (%[amo_addr])\n\t"
        "sd %[marker_value], 0(%[marker_addr])\n\t"
        "fence rw, rw\n\t"
        "ld %[marker_readback], 0(%[marker_addr])\n\t"
        ".option pop\n\t"
        : [first_old] "=&r"(first_old),
          [second_old] "=&r"(second_old),
          [marker_readback] "=&r"(marker_readback)
        : [inval_addr] "r"(inval_vaddr),
          [prefetch_addr] "r"(prefetch_vaddr),
          [first_zero_addr] "r"(first_zero_vaddr),
          [second_zero_addr] "r"(second_zero_vaddr),
          [cross16_addr] "r"(cross16_store_vaddr),
          [cross16_payload] "r"(cross16_payload),
          [amo_addr] "r"(amo_vaddr),
          [first_addend] "r"(first_addend),
          [second_addend] "r"(second_addend),
          [marker_addr] "r"(marker_vaddr),
          [marker_value] "r"(marker_value)
        : "memory");

    *first_amo_old = first_old;
    *second_amo_old = second_old;
    *marker_observed = marker_readback;
}

bool ai_micro_hs_cbo_inval_prefetch_same_paddr_alias_refill_then_cbo_zero_cbo_zero_cross16_store_amo_no_stale_repeated_zero_split_payload_or_atomic_owner_corner()
{
    TEST_START();

    const enum test_page base_page = SWITCH1;
    const enum test_page alias_page = SWITCH2;
    const uintptr_t target_offset = 0xd00U;
    const uintptr_t target_base_vaddr =
        hs_page_base(base_page) + target_offset;
    const uintptr_t target_base_paddr =
        phys_page_base(base_page) + target_offset;
    const uintptr_t low_word_base_vaddr =
        target_base_vaddr + sizeof(uint64_t);
    const uintptr_t high_word_base_vaddr =
        target_base_vaddr + 2U * sizeof(uint64_t);
    const uintptr_t amo_base_vaddr =
        target_base_vaddr + 3U * sizeof(uint64_t);
    const uintptr_t prefetch_base_vaddr =
        target_base_vaddr + sizeof(uint64_t);
    const uintptr_t first_zero_base_vaddr =
        target_base_vaddr + 4U * sizeof(uint64_t);
    const uintptr_t second_zero_base_vaddr =
        target_base_vaddr + (AI_P13AX_BLOCK_WORDS - 1U) *
        sizeof(uint64_t);
    const uintptr_t before_guard_base_vaddr =
        target_base_vaddr - sizeof(uint64_t);
    const uintptr_t after_guard_base_vaddr =
        target_base_vaddr + 2U * AI_P13AX_BLOCK_BYTES;
    const uintptr_t adjacent_base_vaddr =
        target_base_vaddr + AI_P13AX_BLOCK_BYTES;
    const uintptr_t marker_base_vaddr =
        target_base_vaddr + 3U * AI_P13AX_BLOCK_BYTES;
    const uintptr_t marker_guard_base_vaddr =
        marker_base_vaddr + sizeof(uint64_t);
    const uintptr_t before_guard_paddr =
        target_base_paddr - sizeof(uint64_t);
    const uintptr_t after_guard_paddr =
        target_base_paddr + 2U * AI_P13AX_BLOCK_BYTES;
    const uintptr_t adjacent_base_paddr =
        target_base_paddr + AI_P13AX_BLOCK_BYTES;
    const uintptr_t marker_paddr =
        target_base_paddr + 3U * AI_P13AX_BLOCK_BYTES;
    const uintptr_t marker_guard_paddr =
        marker_paddr + sizeof(uint64_t);
    const uint64_t block_seeds[] = {
        0xcdef0123456789abULL,
        0xdef0123456789abcULL,
        0xef0123456789abcdULL,
    };
    const uint64_t adjacent_seeds[] = {
        0x31425364758697a8ULL,
        0x425364758697a8b9ULL,
        0x5364758697a8b9caULL,
    };
    const uint64_t cross16_payloads[] = {
        0x1122334455667788ULL,
        0x99aabbccddeeff10ULL,
        0x0f1e2d3c4b5a6978ULL,
    };
    const uint64_t first_addends[] = {
        0x0001000100010001ULL,
        0x0002000200020002ULL,
        0x0003000300030003ULL,
    };
    const uint64_t second_addends[] = {
        0x0010001000100010ULL,
        0x0020002000200020ULL,
        0x0030003000300030ULL,
    };
    const uint64_t before_guards[] = {
        0xdeadbeefcafef00dULL,
        0x0badc0de13579bdfULL,
        0xfdb97531eca86420ULL,
    };
    const uint64_t after_guards[] = {
        0x1020304050607080ULL,
        0x1123581321345589ULL,
        0x3141592653589793ULL,
    };
    const uint64_t marker_seeds[] = {
        0xa0a1a2a3a4a5a6a7ULL,
        0xb0b1b2b3b4b5b6b7ULL,
        0xc0c1c2c3c4c5c6c7ULL,
    };
    const uint64_t marker_updates[] = {
        0x456789abcdef0123ULL,
        0x3210fedcba987654ULL,
        0x5a5aa5a50badf00dULL,
    };
    const uint64_t marker_guards[] = {
        0x2718281828459045ULL,
        0xc3c33c3cc3c33c3cULL,
        0xf0f00f0ff0f00f0fULL,
    };
    uintptr_t alias_page_base_vaddr = 0;
    uintptr_t target_alias_vaddr = 0;
    uintptr_t cross16_store_alias_vaddr = 0;
    uintptr_t low_word_alias_vaddr = 0;
    uintptr_t high_word_alias_vaddr = 0;
    uintptr_t amo_alias_vaddr = 0;
    uintptr_t inval_alias_vaddr = 0;
    uintptr_t first_zero_alias_vaddr = 0;
    uintptr_t second_zero_alias_vaddr = 0;
    uintptr_t before_guard_alias_vaddr = 0;
    uintptr_t after_guard_alias_vaddr = 0;
    uintptr_t adjacent_alias_vaddr = 0;
    uintptr_t marker_alias_vaddr = 0;
    uintptr_t marker_guard_alias_vaddr = 0;

    ai_p13ax_prepare_hs_env(MENVCFG_CBIE01 | MENVCFG_CBZE);
    alias_page_base_vaddr =
        ai_p13ax_prepare_same_paddr_alias(alias_page, base_page);
    target_alias_vaddr = alias_page_base_vaddr + target_offset;
    cross16_store_alias_vaddr = target_alias_vaddr + 15U;
    low_word_alias_vaddr = target_alias_vaddr + sizeof(uint64_t);
    high_word_alias_vaddr = target_alias_vaddr + 2U * sizeof(uint64_t);
    amo_alias_vaddr = target_alias_vaddr + 3U * sizeof(uint64_t);
    inval_alias_vaddr =
        target_alias_vaddr + 6U * sizeof(uint64_t);
    first_zero_alias_vaddr =
        target_alias_vaddr + 4U * sizeof(uint64_t);
    second_zero_alias_vaddr =
        target_alias_vaddr + (AI_P13AX_BLOCK_WORDS - 1U) *
        sizeof(uint64_t);
    before_guard_alias_vaddr =
        target_alias_vaddr - sizeof(uint64_t);
    after_guard_alias_vaddr =
        target_alias_vaddr + 2U * AI_P13AX_BLOCK_BYTES;
    adjacent_alias_vaddr =
        target_alias_vaddr + AI_P13AX_BLOCK_BYTES;
    marker_alias_vaddr =
        target_alias_vaddr + 3U * AI_P13AX_BLOCK_BYTES;
    marker_guard_alias_vaddr =
        marker_alias_vaddr + sizeof(uint64_t);

    for (size_t round = 0; round < sizeof(block_seeds) / sizeof(block_seeds[0]);
         ++round) {
        const uint64_t seed_word1 =
            ai_p13ax_seed_word(block_seeds[round], 1U);
        const uint64_t seed_word2 =
            ai_p13ax_seed_word(block_seeds[round], 2U);
        const uint64_t seed_word3 =
            ai_p13ax_seed_word(block_seeds[round], 3U);
        const uint64_t expected_first_zero_seed =
            ai_p13ax_seed_word(block_seeds[round], 4U);
        const uint64_t expected_second_zero_seed =
            ai_p13ax_seed_word(block_seeds[round], AI_P13AX_BLOCK_WORDS - 1U);
        const uint64_t expected_low_word =
            ai_p13ax_cross16_low_word(cross16_payloads[round]);
        const uint64_t expected_high_word =
            ai_p13ax_cross16_high_word(cross16_payloads[round]);
        const uint64_t after_first_amo = first_addends[round];
        const uint64_t final_amo =
            first_addends[round] + second_addends[round];
        uint64_t first_amo_old = 0xffffffffffffffffULL;
        uint64_t second_amo_old = 0xffffffffffffffffULL;
        uint64_t marker_observed = 0ULL;

        goto_priv(PRIV_M);
        ai_p13ax_seed_block(target_base_paddr, block_seeds[round]);
        ai_p13ax_seed_block(adjacent_base_paddr, adjacent_seeds[round]);
        write64(before_guard_paddr, before_guards[round]);
        write64(after_guard_paddr, after_guards[round]);
        write64(marker_paddr, marker_seeds[round]);
        write64(marker_guard_paddr, marker_guards[round]);

        goto_priv(PRIV_HS);
        TEST_SETUP_EXCEPT();
        AI_P13AX_ASSERT_NO_EXCEPT_AND("before the same-paddr-alias inval prefetch repeated-zero cross-16B store plus AMO handoff starts, base and alias views expose seeded split words, AMO word, zero probes, adjacent block, marker, and guards",
            ld(target_base_vaddr) == block_seeds[round] &&
            ld(target_alias_vaddr) == block_seeds[round] &&
            ld(low_word_base_vaddr) == seed_word1 &&
            ld(low_word_alias_vaddr) == seed_word1 &&
            ld(high_word_base_vaddr) == seed_word2 &&
            ld(high_word_alias_vaddr) == seed_word2 &&
            ld(amo_base_vaddr) == seed_word3 &&
            ld(amo_alias_vaddr) == seed_word3 &&
            ld(first_zero_base_vaddr) == expected_first_zero_seed &&
            ld(first_zero_alias_vaddr) == expected_first_zero_seed &&
            ld(second_zero_base_vaddr) == expected_second_zero_seed &&
            ld(second_zero_alias_vaddr) == expected_second_zero_seed &&
            ld(adjacent_base_vaddr) == adjacent_seeds[round] &&
            ld(adjacent_alias_vaddr) == adjacent_seeds[round] &&
            ld(marker_base_vaddr) == marker_seeds[round] &&
            ld(marker_alias_vaddr) == marker_seeds[round]
        );

        TEST_SETUP_EXCEPT();
        ai_p13bc_cbo_inval_prefetch_zero_zero_cross16_store_amo_window(
            inval_alias_vaddr, prefetch_base_vaddr,
            first_zero_alias_vaddr, second_zero_alias_vaddr,
            cross16_store_alias_vaddr, amo_alias_vaddr, marker_alias_vaddr,
            cross16_payloads[round], first_addends[round],
            second_addends[round], marker_updates[round],
            &first_amo_old, &second_amo_old, &marker_observed);
        AI_P13AX_ASSERT_NO_EXCEPT_AND("same-paddr alias repeated CBO.ZERO must preserve the following split-store overlay before alias-view AMOs see and update only the zeroed AMO word",
            first_amo_old == 0ULL &&
            first_amo_old != seed_word3 &&
            second_amo_old == after_first_amo &&
            marker_observed == marker_updates[round] &&
            ld(low_word_base_vaddr) == expected_low_word &&
            ld(low_word_alias_vaddr) == expected_low_word &&
            ld(high_word_base_vaddr) == expected_high_word &&
            ld(high_word_alias_vaddr) == expected_high_word &&
            ld(amo_base_vaddr) == final_amo &&
            ld(amo_alias_vaddr) == final_amo &&
            ld(first_zero_base_vaddr) == 0ULL &&
            ld(first_zero_alias_vaddr) == 0ULL &&
            ld(second_zero_base_vaddr) == 0ULL &&
            ld(second_zero_alias_vaddr) == 0ULL &&
            ld(marker_base_vaddr) == marker_updates[round] &&
            ld(marker_alias_vaddr) == marker_updates[round]
        );

        TEST_SETUP_EXCEPT();
        AI_P13AX_ASSERT_NO_EXCEPT_AND("the same-paddr-alias repeated-zero to split-store plus AMO handoff preserves guards and the adjacent block through both views",
            ld(before_guard_base_vaddr) == before_guards[round] &&
            ld(before_guard_alias_vaddr) == before_guards[round] &&
            ld(after_guard_base_vaddr) == after_guards[round] &&
            ld(after_guard_alias_vaddr) == after_guards[round] &&
            ld(marker_guard_base_vaddr) == marker_guards[round] &&
            ld(marker_guard_alias_vaddr) == marker_guards[round] &&
            ld(adjacent_base_vaddr) == adjacent_seeds[round] &&
            ld(adjacent_alias_vaddr) == adjacent_seeds[round]
        );

        goto_priv(PRIV_M);
        TEST_ASSERT("backing memory after same-paddr-alias inval prefetch repeated-zero cross-16B store plus AMO handoff contains only the split-store overlay and final AMO result over an otherwise zeroed block, marker update, and unchanged guards",
            ai_p13bb_zero_block_matches_cross16_store_and_word(
                target_base_paddr, cross16_payloads[round],
                3U, final_amo) &&
            ai_p13ax_block_matches_seed(adjacent_base_paddr,
                adjacent_seeds[round]) &&
            read64(before_guard_paddr) == before_guards[round] &&
            read64(after_guard_paddr) == after_guards[round] &&
            read64(marker_paddr) == marker_updates[round] &&
            read64(marker_guard_paddr) == marker_guards[round]
        );
    }

    ai_p13ax_reset_hs_env(MENVCFG_CBIE01 | MENVCFG_CBZE);

    TEST_END();
}

static inline void ai_p13bd_cbo_inval_prefetch_zero_zero_cross16_store_fence_post_store_window(
    uintptr_t inval_vaddr,
    uintptr_t prefetch_vaddr,
    uintptr_t first_zero_vaddr,
    uintptr_t second_zero_vaddr,
    uintptr_t cross16_store_vaddr,
    uintptr_t low_word_vaddr,
    uintptr_t high_word_vaddr,
    uintptr_t post_store_vaddr,
    uintptr_t marker_vaddr,
    uint64_t cross16_payload,
    uint64_t post_store_value,
    uint64_t marker_value,
    uint64_t *low_word_observed,
    uint64_t *high_word_observed,
    uint64_t *post_store_observed,
    uint64_t *marker_observed)
{
    uint64_t low_readback = 0ULL;
    uint64_t high_readback = 0ULL;
    uint64_t post_store_readback = 0ULL;
    uint64_t marker_readback = 0ULL;

    asm volatile(
        ".option push\n\t"
        ".option norvc\n\t"
        ".insn i 0x0f, 0x2, x0, %[inval_addr], 0\n\t"
        ".insn i 0x13, 0x6, x0, %[prefetch_addr], 1\n\t"
        ".insn i 0x0f, 0x2, x0, %[first_zero_addr], 4\n\t"
        ".insn i 0x0f, 0x2, x0, %[second_zero_addr], 4\n\t"
        "sd %[cross16_payload], 0(%[cross16_addr])\n\t"
        "ld %[low_readback], 0(%[low_word_addr])\n\t"
        "ld %[high_readback], 0(%[high_word_addr])\n\t"
        "fence rw, rw\n\t"
        "sd %[post_store_value], 0(%[post_store_addr])\n\t"
        "ld %[post_store_readback], 0(%[post_store_addr])\n\t"
        "sd %[marker_value], 0(%[marker_addr])\n\t"
        "fence rw, rw\n\t"
        "ld %[marker_readback], 0(%[marker_addr])\n\t"
        ".option pop\n\t"
        : [low_readback] "=&r"(low_readback),
          [high_readback] "=&r"(high_readback),
          [post_store_readback] "=&r"(post_store_readback),
          [marker_readback] "=&r"(marker_readback)
        : [inval_addr] "r"(inval_vaddr),
          [prefetch_addr] "r"(prefetch_vaddr),
          [first_zero_addr] "r"(first_zero_vaddr),
          [second_zero_addr] "r"(second_zero_vaddr),
          [cross16_addr] "r"(cross16_store_vaddr),
          [low_word_addr] "r"(low_word_vaddr),
          [high_word_addr] "r"(high_word_vaddr),
          [post_store_addr] "r"(post_store_vaddr),
          [marker_addr] "r"(marker_vaddr),
          [cross16_payload] "r"(cross16_payload),
          [post_store_value] "r"(post_store_value),
          [marker_value] "r"(marker_value)
        : "memory");

    *low_word_observed = low_readback;
    *high_word_observed = high_readback;
    *post_store_observed = post_store_readback;
    *marker_observed = marker_readback;
}

bool ai_micro_hs_cbo_inval_prefetch_same_paddr_alias_refill_then_cbo_zero_cbo_zero_cross16_store_fence_post_store_no_stale_repeated_zero_split_fence_owner_or_marker_drop_corner()
{
    TEST_START();

    const enum test_page base_page = SWITCH1;
    const enum test_page alias_page = SWITCH2;
    const uintptr_t target_offset = 0xd40U;
    const uintptr_t target_base_vaddr =
        hs_page_base(base_page) + target_offset;
    const uintptr_t target_base_paddr =
        phys_page_base(base_page) + target_offset;
    const uintptr_t low_word_base_vaddr =
        target_base_vaddr + sizeof(uint64_t);
    const uintptr_t high_word_base_vaddr =
        target_base_vaddr + 2U * sizeof(uint64_t);
    const uintptr_t post_store_base_vaddr =
        target_base_vaddr + 3U * sizeof(uint64_t);
    const uintptr_t prefetch_base_vaddr =
        target_base_vaddr + sizeof(uint64_t);
    const uintptr_t first_zero_base_vaddr =
        target_base_vaddr + 4U * sizeof(uint64_t);
    const uintptr_t second_zero_base_vaddr =
        target_base_vaddr + (AI_P13AX_BLOCK_WORDS - 1U) *
        sizeof(uint64_t);
    const uintptr_t before_guard_base_vaddr =
        target_base_vaddr - sizeof(uint64_t);
    const uintptr_t after_guard_base_vaddr =
        target_base_vaddr + 2U * AI_P13AX_BLOCK_BYTES;
    const uintptr_t adjacent_base_vaddr =
        target_base_vaddr + AI_P13AX_BLOCK_BYTES;
    const uintptr_t marker_base_vaddr =
        target_base_vaddr + 3U * AI_P13AX_BLOCK_BYTES;
    const uintptr_t marker_guard_base_vaddr =
        marker_base_vaddr + sizeof(uint64_t);
    const uintptr_t before_guard_paddr =
        target_base_paddr - sizeof(uint64_t);
    const uintptr_t after_guard_paddr =
        target_base_paddr + 2U * AI_P13AX_BLOCK_BYTES;
    const uintptr_t adjacent_base_paddr =
        target_base_paddr + AI_P13AX_BLOCK_BYTES;
    const uintptr_t marker_paddr =
        target_base_paddr + 3U * AI_P13AX_BLOCK_BYTES;
    const uintptr_t marker_guard_paddr =
        marker_paddr + sizeof(uint64_t);
    const uint64_t block_seeds[] = {
        0xd02468ace13579bfULL,
        0xe13579bfd02468acULL,
        0xf2468ace13579bd0ULL,
    };
    const uint64_t adjacent_seeds[] = {
        0xabcdef0123456789ULL,
        0x0123456789abcdefULL,
        0x76543210fedcba98ULL,
    };
    const uint64_t cross16_payloads[] = {
        0x0a1b2c3d4e5f6071ULL,
        0x1021324354657687ULL,
        0x8899aabbccddeeffULL,
    };
    const uint64_t post_fence_updates[] = {
        0x31425364758697a8ULL,
        0x425364758697a8b9ULL,
        0x5364758697a8b9caULL,
    };
    const uint64_t before_guards[] = {
        0xfdb97531eca86420ULL,
        0x13579bdf2468ace0ULL,
        0x02468ace13579bdfULL,
    };
    const uint64_t after_guards[] = {
        0x1020304050607080ULL,
        0x0badf00d5a5aa5a5ULL,
        0x1123581321345589ULL,
    };
    const uint64_t marker_seeds[] = {
        0xa1a2a3a4a5a6a7a8ULL,
        0xb1b2b3b4b5b6b7b8ULL,
        0xc1c2c3c4c5c6c7c8ULL,
    };
    const uint64_t marker_updates[] = {
        0x456789abcdef0123ULL,
        0x3210fedcba987654ULL,
        0x5a5aa5a50badf00dULL,
    };
    const uint64_t marker_guards[] = {
        0x2718281828459045ULL,
        0x3141592653589793ULL,
        0xf0f00f0ff0f00f0fULL,
    };
    uintptr_t alias_page_base_vaddr = 0;
    uintptr_t target_alias_vaddr = 0;
    uintptr_t cross16_store_alias_vaddr = 0;
    uintptr_t low_word_alias_vaddr = 0;
    uintptr_t high_word_alias_vaddr = 0;
    uintptr_t post_store_alias_vaddr = 0;
    uintptr_t inval_alias_vaddr = 0;
    uintptr_t first_zero_alias_vaddr = 0;
    uintptr_t second_zero_alias_vaddr = 0;
    uintptr_t before_guard_alias_vaddr = 0;
    uintptr_t after_guard_alias_vaddr = 0;
    uintptr_t adjacent_alias_vaddr = 0;
    uintptr_t marker_alias_vaddr = 0;
    uintptr_t marker_guard_alias_vaddr = 0;

    ai_p13ax_prepare_hs_env(MENVCFG_CBIE01 | MENVCFG_CBZE);
    alias_page_base_vaddr =
        ai_p13ax_prepare_same_paddr_alias(alias_page, base_page);
    target_alias_vaddr = alias_page_base_vaddr + target_offset;
    cross16_store_alias_vaddr = target_alias_vaddr + 15U;
    low_word_alias_vaddr = target_alias_vaddr + sizeof(uint64_t);
    high_word_alias_vaddr = target_alias_vaddr + 2U * sizeof(uint64_t);
    post_store_alias_vaddr = target_alias_vaddr + 3U * sizeof(uint64_t);
    inval_alias_vaddr =
        target_alias_vaddr + 6U * sizeof(uint64_t);
    first_zero_alias_vaddr =
        target_alias_vaddr + 4U * sizeof(uint64_t);
    second_zero_alias_vaddr =
        target_alias_vaddr + (AI_P13AX_BLOCK_WORDS - 1U) *
        sizeof(uint64_t);
    before_guard_alias_vaddr =
        target_alias_vaddr - sizeof(uint64_t);
    after_guard_alias_vaddr =
        target_alias_vaddr + 2U * AI_P13AX_BLOCK_BYTES;
    adjacent_alias_vaddr =
        target_alias_vaddr + AI_P13AX_BLOCK_BYTES;
    marker_alias_vaddr =
        target_alias_vaddr + 3U * AI_P13AX_BLOCK_BYTES;
    marker_guard_alias_vaddr =
        marker_alias_vaddr + sizeof(uint64_t);

    for (size_t round = 0; round < sizeof(block_seeds) / sizeof(block_seeds[0]);
         ++round) {
        const uint64_t seed_word1 =
            ai_p13ax_seed_word(block_seeds[round], 1U);
        const uint64_t seed_word2 =
            ai_p13ax_seed_word(block_seeds[round], 2U);
        const uint64_t seed_word3 =
            ai_p13ax_seed_word(block_seeds[round], 3U);
        const uint64_t expected_first_zero_seed =
            ai_p13ax_seed_word(block_seeds[round], 4U);
        const uint64_t expected_second_zero_seed =
            ai_p13ax_seed_word(block_seeds[round], AI_P13AX_BLOCK_WORDS - 1U);
        const uint64_t expected_low_word =
            ai_p13ax_cross16_low_word(cross16_payloads[round]);
        const uint64_t expected_high_word =
            ai_p13ax_cross16_high_word(cross16_payloads[round]);
        uint64_t low_word_observed = 0ULL;
        uint64_t high_word_observed = 0ULL;
        uint64_t post_store_observed = 0ULL;
        uint64_t marker_observed = 0ULL;

        goto_priv(PRIV_M);
        ai_p13ax_seed_block(target_base_paddr, block_seeds[round]);
        ai_p13ax_seed_block(adjacent_base_paddr, adjacent_seeds[round]);
        write64(before_guard_paddr, before_guards[round]);
        write64(after_guard_paddr, after_guards[round]);
        write64(marker_paddr, marker_seeds[round]);
        write64(marker_guard_paddr, marker_guards[round]);

        goto_priv(PRIV_HS);
        TEST_SETUP_EXCEPT();
        AI_P13AX_ASSERT_NO_EXCEPT_AND("before the same-paddr-alias inval prefetch repeated-zero cross-16B store fence post-store handoff starts, base and alias views expose seeded split words, post-store word, zero probes, adjacent block, marker, and guards",
            ld(target_base_vaddr) == block_seeds[round] &&
            ld(target_alias_vaddr) == block_seeds[round] &&
            ld(low_word_base_vaddr) == seed_word1 &&
            ld(low_word_alias_vaddr) == seed_word1 &&
            ld(high_word_base_vaddr) == seed_word2 &&
            ld(high_word_alias_vaddr) == seed_word2 &&
            ld(post_store_base_vaddr) == seed_word3 &&
            ld(post_store_alias_vaddr) == seed_word3 &&
            ld(first_zero_base_vaddr) == expected_first_zero_seed &&
            ld(first_zero_alias_vaddr) == expected_first_zero_seed &&
            ld(second_zero_base_vaddr) == expected_second_zero_seed &&
            ld(second_zero_alias_vaddr) == expected_second_zero_seed &&
            ld(adjacent_base_vaddr) == adjacent_seeds[round] &&
            ld(adjacent_alias_vaddr) == adjacent_seeds[round] &&
            ld(marker_base_vaddr) == marker_seeds[round] &&
            ld(marker_alias_vaddr) == marker_seeds[round]
        );

        TEST_SETUP_EXCEPT();
        ai_p13bd_cbo_inval_prefetch_zero_zero_cross16_store_fence_post_store_window(
            inval_alias_vaddr, prefetch_base_vaddr,
            first_zero_alias_vaddr, second_zero_alias_vaddr,
            cross16_store_alias_vaddr, low_word_alias_vaddr,
            high_word_alias_vaddr, post_store_alias_vaddr,
            marker_alias_vaddr, cross16_payloads[round],
            post_fence_updates[round], marker_updates[round],
            &low_word_observed, &high_word_observed,
            &post_store_observed, &marker_observed);
        AI_P13AX_ASSERT_NO_EXCEPT_AND("same-paddr alias repeated CBO.ZERO must preserve the split-store overlay across fence before the post-fence store and marker make progress",
            low_word_observed == expected_low_word &&
            high_word_observed == expected_high_word &&
            post_store_observed == post_fence_updates[round] &&
            marker_observed == marker_updates[round] &&
            ld(target_base_vaddr) == 0ULL &&
            ld(target_alias_vaddr) == 0ULL &&
            ld(low_word_base_vaddr) == expected_low_word &&
            ld(low_word_alias_vaddr) == expected_low_word &&
            ld(high_word_base_vaddr) == expected_high_word &&
            ld(high_word_alias_vaddr) == expected_high_word &&
            ld(post_store_base_vaddr) == post_fence_updates[round] &&
            ld(post_store_alias_vaddr) == post_fence_updates[round] &&
            ld(first_zero_base_vaddr) == 0ULL &&
            ld(first_zero_alias_vaddr) == 0ULL &&
            ld(second_zero_base_vaddr) == 0ULL &&
            ld(second_zero_alias_vaddr) == 0ULL &&
            ld(marker_base_vaddr) == marker_updates[round] &&
            ld(marker_alias_vaddr) == marker_updates[round]
        );

        TEST_SETUP_EXCEPT();
        AI_P13AX_ASSERT_NO_EXCEPT_AND("the same-paddr-alias repeated-zero split-store to fence post-store handoff preserves guards and the adjacent block through both views",
            ld(before_guard_base_vaddr) == before_guards[round] &&
            ld(before_guard_alias_vaddr) == before_guards[round] &&
            ld(after_guard_base_vaddr) == after_guards[round] &&
            ld(after_guard_alias_vaddr) == after_guards[round] &&
            ld(marker_guard_base_vaddr) == marker_guards[round] &&
            ld(marker_guard_alias_vaddr) == marker_guards[round] &&
            ld(adjacent_base_vaddr) == adjacent_seeds[round] &&
            ld(adjacent_alias_vaddr) == adjacent_seeds[round]
        );

        goto_priv(PRIV_M);
        TEST_ASSERT("backing memory after same-paddr-alias inval prefetch repeated-zero cross-16B store fence post-store handoff contains only the split-store overlay and post-fence word over an otherwise zeroed block, marker update, and unchanged guards",
            ai_p13bb_zero_block_matches_cross16_store_and_word(
                target_base_paddr, cross16_payloads[round],
                3U, post_fence_updates[round]) &&
            ai_p13ax_block_matches_seed(adjacent_base_paddr,
                adjacent_seeds[round]) &&
            read64(before_guard_paddr) == before_guards[round] &&
            read64(after_guard_paddr) == after_guards[round] &&
            read64(marker_paddr) == marker_updates[round] &&
            read64(marker_guard_paddr) == marker_guards[round]
        );
    }

    ai_p13ax_reset_hs_env(MENVCFG_CBIE01 | MENVCFG_CBZE);

    TEST_END();
}

static inline void ai_p13be_cbo_inval_prefetch_zero_zero_cross16_load_fence_post_store_window(
    uintptr_t inval_vaddr,
    uintptr_t prefetch_vaddr,
    uintptr_t first_zero_vaddr,
    uintptr_t second_zero_vaddr,
    uintptr_t cross16_load_vaddr,
    uintptr_t post_store_vaddr,
    uintptr_t marker_vaddr,
    uint64_t post_store_value,
    uint64_t marker_value,
    uint64_t *load_observed,
    uint64_t *post_store_observed,
    uint64_t *marker_observed)
{
    uint64_t load_readback = 0xffffffffffffffffULL;
    uint64_t post_store_readback = 0ULL;
    uint64_t marker_readback = 0ULL;

    asm volatile(
        ".option push\n\t"
        ".option norvc\n\t"
        ".insn i 0x0f, 0x2, x0, %[inval_addr], 0\n\t"
        ".insn i 0x13, 0x6, x0, %[prefetch_addr], 1\n\t"
        ".insn i 0x0f, 0x2, x0, %[first_zero_addr], 4\n\t"
        ".insn i 0x0f, 0x2, x0, %[second_zero_addr], 4\n\t"
        "ld %[load_readback], 0(%[load_addr])\n\t"
        "fence rw, rw\n\t"
        "sd %[post_store_value], 0(%[post_store_addr])\n\t"
        "ld %[post_store_readback], 0(%[post_store_addr])\n\t"
        "sd %[marker_value], 0(%[marker_addr])\n\t"
        "fence rw, rw\n\t"
        "ld %[marker_readback], 0(%[marker_addr])\n\t"
        ".option pop\n\t"
        : [load_readback] "=&r"(load_readback),
          [post_store_readback] "=&r"(post_store_readback),
          [marker_readback] "=&r"(marker_readback)
        : [inval_addr] "r"(inval_vaddr),
          [prefetch_addr] "r"(prefetch_vaddr),
          [first_zero_addr] "r"(first_zero_vaddr),
          [second_zero_addr] "r"(second_zero_vaddr),
          [load_addr] "r"(cross16_load_vaddr),
          [post_store_addr] "r"(post_store_vaddr),
          [marker_addr] "r"(marker_vaddr),
          [post_store_value] "r"(post_store_value),
          [marker_value] "r"(marker_value)
        : "memory");

    *load_observed = load_readback;
    *post_store_observed = post_store_readback;
    *marker_observed = marker_readback;
}

bool ai_micro_hs_cbo_inval_prefetch_same_paddr_alias_refill_then_cbo_zero_cbo_zero_cross16_load_fence_post_store_no_stale_repeated_zero_split_fence_owner_or_marker_drop_corner()
{
    TEST_START();

    const enum test_page base_page = SWITCH1;
    const enum test_page alias_page = SWITCH2;
    const uintptr_t target_offset = 0xd80U;
    const uintptr_t target_base_vaddr =
        hs_page_base(base_page) + target_offset;
    const uintptr_t target_base_paddr =
        phys_page_base(base_page) + target_offset;
    const uintptr_t low_word_base_vaddr =
        target_base_vaddr + sizeof(uint64_t);
    const uintptr_t high_word_base_vaddr =
        target_base_vaddr + 2U * sizeof(uint64_t);
    const uintptr_t post_store_base_vaddr =
        target_base_vaddr + 3U * sizeof(uint64_t);
    const uintptr_t prefetch_base_vaddr =
        target_base_vaddr + sizeof(uint64_t);
    const uintptr_t first_zero_base_vaddr =
        target_base_vaddr + 4U * sizeof(uint64_t);
    const uintptr_t second_zero_base_vaddr =
        target_base_vaddr + (AI_P13AX_BLOCK_WORDS - 1U) *
        sizeof(uint64_t);
    const uintptr_t before_guard_base_vaddr =
        target_base_vaddr - sizeof(uint64_t);
    const uintptr_t after_guard_base_vaddr =
        target_base_vaddr + 2U * AI_P13AX_BLOCK_BYTES;
    const uintptr_t adjacent_base_vaddr =
        target_base_vaddr + AI_P13AX_BLOCK_BYTES;
    const uintptr_t marker_base_vaddr =
        target_base_vaddr + 3U * AI_P13AX_BLOCK_BYTES;
    const uintptr_t marker_guard_base_vaddr =
        marker_base_vaddr + sizeof(uint64_t);
    const uintptr_t before_guard_paddr =
        target_base_paddr - sizeof(uint64_t);
    const uintptr_t after_guard_paddr =
        target_base_paddr + 2U * AI_P13AX_BLOCK_BYTES;
    const uintptr_t adjacent_base_paddr =
        target_base_paddr + AI_P13AX_BLOCK_BYTES;
    const uintptr_t marker_paddr =
        target_base_paddr + 3U * AI_P13AX_BLOCK_BYTES;
    const uintptr_t marker_guard_paddr =
        marker_paddr + sizeof(uint64_t);
    const uint64_t block_seeds[] = {
        0x02468ace13579bdfULL,
        0x13579bdf02468aceULL,
        0x2468ace13579bdf0ULL,
    };
    const uint64_t adjacent_seeds[] = {
        0x89abcdef01234567ULL,
        0x76543210fedcba98ULL,
        0x55aa7733cc11ee99ULL,
    };
    const uint64_t post_fence_updates[] = {
        0x60718293a4b5c6d7ULL,
        0x718293a4b5c6d7e8ULL,
        0x8293a4b5c6d7e8f9ULL,
    };
    const uint64_t before_guards[] = {
        0xfeedfacecafebeefULL,
        0xdeadc0def00dbabeULL,
        0xfaceb00c13579bdfULL,
    };
    const uint64_t after_guards[] = {
        0x0badf00d5a5aa5a5ULL,
        0x1020304050607080ULL,
        0x1123581321345589ULL,
    };
    const uint64_t marker_seeds[] = {
        0x0102030405060708ULL,
        0x1112131415161718ULL,
        0x2122232425262728ULL,
    };
    const uint64_t marker_updates[] = {
        0x3141592653589793ULL,
        0x2718281828459045ULL,
        0x1123581321345589ULL,
    };
    const uint64_t marker_guards[] = {
        0xfdb97531eca86420ULL,
        0x13579bdf2468ace0ULL,
        0xf0f00f0ff0f00f0fULL,
    };
    uintptr_t alias_page_base_vaddr = 0;
    uintptr_t target_alias_vaddr = 0;
    uintptr_t cross16_load_alias_vaddr = 0;
    uintptr_t low_word_alias_vaddr = 0;
    uintptr_t high_word_alias_vaddr = 0;
    uintptr_t post_store_alias_vaddr = 0;
    uintptr_t inval_alias_vaddr = 0;
    uintptr_t first_zero_alias_vaddr = 0;
    uintptr_t second_zero_alias_vaddr = 0;
    uintptr_t before_guard_alias_vaddr = 0;
    uintptr_t after_guard_alias_vaddr = 0;
    uintptr_t adjacent_alias_vaddr = 0;
    uintptr_t marker_alias_vaddr = 0;
    uintptr_t marker_guard_alias_vaddr = 0;

    ai_p13ax_prepare_hs_env(MENVCFG_CBIE01 | MENVCFG_CBZE);
    alias_page_base_vaddr =
        ai_p13ax_prepare_same_paddr_alias(alias_page, base_page);
    target_alias_vaddr = alias_page_base_vaddr + target_offset;
    cross16_load_alias_vaddr = target_alias_vaddr + 15U;
    low_word_alias_vaddr = target_alias_vaddr + sizeof(uint64_t);
    high_word_alias_vaddr = target_alias_vaddr + 2U * sizeof(uint64_t);
    post_store_alias_vaddr = target_alias_vaddr + 3U * sizeof(uint64_t);
    inval_alias_vaddr =
        target_alias_vaddr + 6U * sizeof(uint64_t);
    first_zero_alias_vaddr =
        target_alias_vaddr + 4U * sizeof(uint64_t);
    second_zero_alias_vaddr =
        target_alias_vaddr + (AI_P13AX_BLOCK_WORDS - 1U) *
        sizeof(uint64_t);
    before_guard_alias_vaddr =
        target_alias_vaddr - sizeof(uint64_t);
    after_guard_alias_vaddr =
        target_alias_vaddr + 2U * AI_P13AX_BLOCK_BYTES;
    adjacent_alias_vaddr =
        target_alias_vaddr + AI_P13AX_BLOCK_BYTES;
    marker_alias_vaddr =
        target_alias_vaddr + 3U * AI_P13AX_BLOCK_BYTES;
    marker_guard_alias_vaddr =
        marker_alias_vaddr + sizeof(uint64_t);

    for (size_t round = 0; round < sizeof(block_seeds) / sizeof(block_seeds[0]);
         ++round) {
        const uint64_t seed_word1 =
            ai_p13ax_seed_word(block_seeds[round], 1U);
        const uint64_t seed_word2 =
            ai_p13ax_seed_word(block_seeds[round], 2U);
        const uint64_t seed_word3 =
            ai_p13ax_seed_word(block_seeds[round], 3U);
        const uint64_t expected_seed_merge =
            ai_p13ay_cross16_load_from_seed(seed_word1, seed_word2);
        const uint64_t expected_first_zero_seed =
            ai_p13ax_seed_word(block_seeds[round], 4U);
        const uint64_t expected_second_zero_seed =
            ai_p13ax_seed_word(block_seeds[round], AI_P13AX_BLOCK_WORDS - 1U);
        uint64_t load_observed = 0xffffffffffffffffULL;
        uint64_t post_store_observed = 0ULL;
        uint64_t marker_observed = 0ULL;

        goto_priv(PRIV_M);
        ai_p13ax_seed_block(target_base_paddr, block_seeds[round]);
        ai_p13ax_seed_block(adjacent_base_paddr, adjacent_seeds[round]);
        write64(before_guard_paddr, before_guards[round]);
        write64(after_guard_paddr, after_guards[round]);
        write64(marker_paddr, marker_seeds[round]);
        write64(marker_guard_paddr, marker_guards[round]);

        goto_priv(PRIV_HS);
        TEST_SETUP_EXCEPT();
        AI_P13AX_ASSERT_NO_EXCEPT_AND("before the same-paddr-alias inval prefetch repeated-zero cross-16B load fence post-store handoff starts, base and alias views expose seeded split words, post-store word, zero probes, adjacent block, marker, and guards",
            ld(target_base_vaddr) == block_seeds[round] &&
            ld(target_alias_vaddr) == block_seeds[round] &&
            ld(low_word_base_vaddr) == seed_word1 &&
            ld(low_word_alias_vaddr) == seed_word1 &&
            ld(high_word_base_vaddr) == seed_word2 &&
            ld(high_word_alias_vaddr) == seed_word2 &&
            ld(post_store_base_vaddr) == seed_word3 &&
            ld(post_store_alias_vaddr) == seed_word3 &&
            ld(first_zero_base_vaddr) == expected_first_zero_seed &&
            ld(first_zero_alias_vaddr) == expected_first_zero_seed &&
            ld(second_zero_base_vaddr) == expected_second_zero_seed &&
            ld(second_zero_alias_vaddr) == expected_second_zero_seed &&
            ld(adjacent_base_vaddr) == adjacent_seeds[round] &&
            ld(adjacent_alias_vaddr) == adjacent_seeds[round] &&
            ld(marker_base_vaddr) == marker_seeds[round] &&
            ld(marker_alias_vaddr) == marker_seeds[round]
        );

        TEST_SETUP_EXCEPT();
        ai_p13be_cbo_inval_prefetch_zero_zero_cross16_load_fence_post_store_window(
            inval_alias_vaddr, prefetch_base_vaddr,
            first_zero_alias_vaddr, second_zero_alias_vaddr,
            cross16_load_alias_vaddr, post_store_alias_vaddr,
            marker_alias_vaddr, post_fence_updates[round],
            marker_updates[round], &load_observed, &post_store_observed,
            &marker_observed);
        AI_P13AX_ASSERT_NO_EXCEPT_AND("same-paddr alias repeated CBO.ZERO must feed a zero split-load merge across fence before the post-fence store and marker make progress",
            load_observed == 0ULL &&
            load_observed != expected_seed_merge &&
            post_store_observed == post_fence_updates[round] &&
            marker_observed == marker_updates[round] &&
            ld(target_base_vaddr) == 0ULL &&
            ld(target_alias_vaddr) == 0ULL &&
            ld(low_word_base_vaddr) == 0ULL &&
            ld(low_word_alias_vaddr) == 0ULL &&
            ld(high_word_base_vaddr) == 0ULL &&
            ld(high_word_alias_vaddr) == 0ULL &&
            ld(post_store_base_vaddr) == post_fence_updates[round] &&
            ld(post_store_alias_vaddr) == post_fence_updates[round] &&
            ld(first_zero_base_vaddr) == 0ULL &&
            ld(first_zero_alias_vaddr) == 0ULL &&
            ld(second_zero_base_vaddr) == 0ULL &&
            ld(second_zero_alias_vaddr) == 0ULL &&
            ld(marker_base_vaddr) == marker_updates[round] &&
            ld(marker_alias_vaddr) == marker_updates[round]
        );

        TEST_SETUP_EXCEPT();
        AI_P13AX_ASSERT_NO_EXCEPT_AND("the same-paddr-alias repeated-zero split-load to fence post-store handoff preserves guards and the adjacent block through both views",
            ld(before_guard_base_vaddr) == before_guards[round] &&
            ld(before_guard_alias_vaddr) == before_guards[round] &&
            ld(after_guard_base_vaddr) == after_guards[round] &&
            ld(after_guard_alias_vaddr) == after_guards[round] &&
            ld(marker_guard_base_vaddr) == marker_guards[round] &&
            ld(marker_guard_alias_vaddr) == marker_guards[round] &&
            ld(adjacent_base_vaddr) == adjacent_seeds[round] &&
            ld(adjacent_alias_vaddr) == adjacent_seeds[round]
        );

        goto_priv(PRIV_M);
        TEST_ASSERT("backing memory after same-paddr-alias inval prefetch repeated-zero cross-16B load fence post-store handoff contains only the post-fence word over an otherwise zeroed block, marker update, and unchanged guards",
            ai_p13az_block_all_zero_except_word(target_base_paddr,
                3U, post_fence_updates[round]) &&
            ai_p13ax_block_matches_seed(adjacent_base_paddr,
                adjacent_seeds[round]) &&
            read64(before_guard_paddr) == before_guards[round] &&
            read64(after_guard_paddr) == after_guards[round] &&
            read64(marker_paddr) == marker_updates[round] &&
            read64(marker_guard_paddr) == marker_guards[round]
        );
    }

    ai_p13ax_reset_hs_env(MENVCFG_CBIE01 | MENVCFG_CBZE);

    TEST_END();
}

static inline void ai_p13bf_cbo_inval_prefetch_zero_zero_cross16_store_cbom_post_store_window(
    uintptr_t inval_vaddr,
    uintptr_t prefetch_vaddr,
    uintptr_t first_zero_vaddr,
    uintptr_t second_zero_vaddr,
    uintptr_t cross16_store_vaddr,
    uintptr_t low_word_vaddr,
    uintptr_t high_word_vaddr,
    uintptr_t clean_vaddr,
    uintptr_t flush_vaddr,
    uintptr_t post_store_vaddr,
    uintptr_t marker_vaddr,
    uint64_t cross16_payload,
    uint64_t post_store_value,
    uint64_t marker_value,
    uint64_t *low_word_observed,
    uint64_t *high_word_observed,
    uint64_t *post_store_observed,
    uint64_t *marker_observed)
{
    uint64_t low_readback = 0ULL;
    uint64_t high_readback = 0ULL;
    uint64_t post_store_readback = 0ULL;
    uint64_t marker_readback = 0ULL;

    asm volatile(
        ".option push\n\t"
        ".option norvc\n\t"
        ".insn i 0x0f, 0x2, x0, %[inval_addr], 0\n\t"
        ".insn i 0x13, 0x6, x0, %[prefetch_addr], 1\n\t"
        ".insn i 0x0f, 0x2, x0, %[first_zero_addr], 4\n\t"
        ".insn i 0x0f, 0x2, x0, %[second_zero_addr], 4\n\t"
        "sd %[cross16_payload], 0(%[cross16_addr])\n\t"
        "ld %[low_readback], 0(%[low_word_addr])\n\t"
        "ld %[high_readback], 0(%[high_word_addr])\n\t"
        ".insn i 0x0f, 0x2, x0, %[clean_addr], 1\n\t"
        ".insn i 0x0f, 0x2, x0, %[flush_addr], 2\n\t"
        "sd %[post_store_value], 0(%[post_store_addr])\n\t"
        "ld %[post_store_readback], 0(%[post_store_addr])\n\t"
        "sd %[marker_value], 0(%[marker_addr])\n\t"
        "fence rw, rw\n\t"
        "ld %[marker_readback], 0(%[marker_addr])\n\t"
        ".option pop\n\t"
        : [low_readback] "=&r"(low_readback),
          [high_readback] "=&r"(high_readback),
          [post_store_readback] "=&r"(post_store_readback),
          [marker_readback] "=&r"(marker_readback)
        : [inval_addr] "r"(inval_vaddr),
          [prefetch_addr] "r"(prefetch_vaddr),
          [first_zero_addr] "r"(first_zero_vaddr),
          [second_zero_addr] "r"(second_zero_vaddr),
          [cross16_addr] "r"(cross16_store_vaddr),
          [low_word_addr] "r"(low_word_vaddr),
          [high_word_addr] "r"(high_word_vaddr),
          [clean_addr] "r"(clean_vaddr),
          [flush_addr] "r"(flush_vaddr),
          [post_store_addr] "r"(post_store_vaddr),
          [marker_addr] "r"(marker_vaddr),
          [cross16_payload] "r"(cross16_payload),
          [post_store_value] "r"(post_store_value),
          [marker_value] "r"(marker_value)
        : "memory");

    *low_word_observed = low_readback;
    *high_word_observed = high_readback;
    *post_store_observed = post_store_readback;
    *marker_observed = marker_readback;
}

bool ai_micro_hs_cbo_inval_prefetch_same_paddr_alias_refill_then_cbo_zero_cbo_zero_cross16_store_cbom_post_store_no_stale_repeated_zero_split_cmo_owner_or_marker_drop_corner()
{
    TEST_START();

    const enum test_page base_page = SWITCH1;
    const enum test_page alias_page = SWITCH2;
    const uintptr_t target_offset = 0xdc0U;
    const uintptr_t target_base_vaddr =
        hs_page_base(base_page) + target_offset;
    const uintptr_t target_base_paddr =
        phys_page_base(base_page) + target_offset;
    const uintptr_t low_word_base_vaddr =
        target_base_vaddr + sizeof(uint64_t);
    const uintptr_t high_word_base_vaddr =
        target_base_vaddr + 2U * sizeof(uint64_t);
    const uintptr_t post_store_base_vaddr =
        target_base_vaddr + 3U * sizeof(uint64_t);
    const uintptr_t prefetch_base_vaddr =
        target_base_vaddr + sizeof(uint64_t);
    const uintptr_t first_zero_base_vaddr =
        target_base_vaddr + 4U * sizeof(uint64_t);
    const uintptr_t second_zero_base_vaddr =
        target_base_vaddr + (AI_P13AX_BLOCK_WORDS - 1U) *
        sizeof(uint64_t);
    const uintptr_t before_guard_base_vaddr =
        target_base_vaddr - sizeof(uint64_t);
    const uintptr_t after_guard_base_vaddr =
        target_base_vaddr + 2U * AI_P13AX_BLOCK_BYTES;
    const uintptr_t adjacent_base_vaddr =
        target_base_vaddr + AI_P13AX_BLOCK_BYTES;
    const uintptr_t marker_base_vaddr =
        target_base_vaddr + 3U * AI_P13AX_BLOCK_BYTES;
    const uintptr_t marker_guard_base_vaddr =
        marker_base_vaddr + sizeof(uint64_t);
    const uintptr_t before_guard_paddr =
        target_base_paddr - sizeof(uint64_t);
    const uintptr_t after_guard_paddr =
        target_base_paddr + 2U * AI_P13AX_BLOCK_BYTES;
    const uintptr_t adjacent_base_paddr =
        target_base_paddr + AI_P13AX_BLOCK_BYTES;
    const uintptr_t marker_paddr =
        target_base_paddr + 3U * AI_P13AX_BLOCK_BYTES;
    const uintptr_t marker_guard_paddr =
        marker_paddr + sizeof(uint64_t);
    const uint64_t block_seeds[] = {
        0x3579bdf02468ace1ULL,
        0x468ace13579bdf02ULL,
        0x579bdf02468ace13ULL,
    };
    const uint64_t adjacent_seeds[] = {
        0x1029384756abcdefULL,
        0xf0e1d2c3b4a59687ULL,
        0x8877665544332211ULL,
    };
    const uint64_t cross16_payloads[] = {
        0x0123456789abcdefULL,
        0xfedcba9876543210ULL,
        0x8899aabbccddeeffULL,
    };
    const uint64_t post_cbom_updates[] = {
        0xa5a55a5af0f00f0fULL,
        0x0102030405060708ULL,
        0x8877665544332211ULL,
    };
    const uint64_t before_guards[] = {
        0x2468ace013579bdfULL,
        0xfdb97531eca86420ULL,
        0x13579bdf2468ace0ULL,
    };
    const uint64_t after_guards[] = {
        0x1123581321345589ULL,
        0x3141592653589793ULL,
        0x2718281828459045ULL,
    };
    const uint64_t marker_seeds[] = {
        0x9091929394959697ULL,
        0xa0a1a2a3a4a5a6a7ULL,
        0xb0b1b2b3b4b5b6b7ULL,
    };
    const uint64_t marker_updates[] = {
        0x456789abcdef0123ULL,
        0x3210fedcba987654ULL,
        0x5a5aa5a50badf00dULL,
    };
    const uint64_t marker_guards[] = {
        0x2718281828459045ULL,
        0xc3c33c3cc3c33c3cULL,
        0xf0f00f0ff0f00f0fULL,
    };
    uintptr_t alias_page_base_vaddr = 0;
    uintptr_t target_alias_vaddr = 0;
    uintptr_t cross16_store_alias_vaddr = 0;
    uintptr_t low_word_alias_vaddr = 0;
    uintptr_t high_word_alias_vaddr = 0;
    uintptr_t post_store_alias_vaddr = 0;
    uintptr_t inval_alias_vaddr = 0;
    uintptr_t clean_alias_vaddr = 0;
    uintptr_t flush_alias_vaddr = 0;
    uintptr_t first_zero_alias_vaddr = 0;
    uintptr_t second_zero_alias_vaddr = 0;
    uintptr_t before_guard_alias_vaddr = 0;
    uintptr_t after_guard_alias_vaddr = 0;
    uintptr_t adjacent_alias_vaddr = 0;
    uintptr_t marker_alias_vaddr = 0;
    uintptr_t marker_guard_alias_vaddr = 0;

    ai_p13ax_prepare_hs_env(MENVCFG_CBIE01 | MENVCFG_CBZE | MENVCFG_CBZFE);
    alias_page_base_vaddr =
        ai_p13ax_prepare_same_paddr_alias(alias_page, base_page);
    target_alias_vaddr = alias_page_base_vaddr + target_offset;
    cross16_store_alias_vaddr = target_alias_vaddr + 15U;
    low_word_alias_vaddr = target_alias_vaddr + sizeof(uint64_t);
    high_word_alias_vaddr = target_alias_vaddr + 2U * sizeof(uint64_t);
    post_store_alias_vaddr = target_alias_vaddr + 3U * sizeof(uint64_t);
    inval_alias_vaddr =
        target_alias_vaddr + 6U * sizeof(uint64_t);
    clean_alias_vaddr =
        target_alias_vaddr + 3U * sizeof(uint64_t);
    flush_alias_vaddr =
        target_alias_vaddr + (AI_P13AX_BLOCK_WORDS - 1U) *
        sizeof(uint64_t);
    first_zero_alias_vaddr =
        target_alias_vaddr + 4U * sizeof(uint64_t);
    second_zero_alias_vaddr =
        target_alias_vaddr + (AI_P13AX_BLOCK_WORDS - 1U) *
        sizeof(uint64_t);
    before_guard_alias_vaddr =
        target_alias_vaddr - sizeof(uint64_t);
    after_guard_alias_vaddr =
        target_alias_vaddr + 2U * AI_P13AX_BLOCK_BYTES;
    adjacent_alias_vaddr =
        target_alias_vaddr + AI_P13AX_BLOCK_BYTES;
    marker_alias_vaddr =
        target_alias_vaddr + 3U * AI_P13AX_BLOCK_BYTES;
    marker_guard_alias_vaddr =
        marker_alias_vaddr + sizeof(uint64_t);

    for (size_t round = 0; round < sizeof(block_seeds) / sizeof(block_seeds[0]);
         ++round) {
        const uint64_t seed_word1 =
            ai_p13ax_seed_word(block_seeds[round], 1U);
        const uint64_t seed_word2 =
            ai_p13ax_seed_word(block_seeds[round], 2U);
        const uint64_t seed_word3 =
            ai_p13ax_seed_word(block_seeds[round], 3U);
        const uint64_t expected_first_zero_seed =
            ai_p13ax_seed_word(block_seeds[round], 4U);
        const uint64_t expected_second_zero_seed =
            ai_p13ax_seed_word(block_seeds[round], AI_P13AX_BLOCK_WORDS - 1U);
        const uint64_t expected_low_word =
            ai_p13ax_cross16_low_word(cross16_payloads[round]);
        const uint64_t expected_high_word =
            ai_p13ax_cross16_high_word(cross16_payloads[round]);
        uint64_t low_word_observed = 0ULL;
        uint64_t high_word_observed = 0ULL;
        uint64_t post_store_observed = 0ULL;
        uint64_t marker_observed = 0ULL;

        goto_priv(PRIV_M);
        ai_p13ax_seed_block(target_base_paddr, block_seeds[round]);
        ai_p13ax_seed_block(adjacent_base_paddr, adjacent_seeds[round]);
        write64(before_guard_paddr, before_guards[round]);
        write64(after_guard_paddr, after_guards[round]);
        write64(marker_paddr, marker_seeds[round]);
        write64(marker_guard_paddr, marker_guards[round]);

        goto_priv(PRIV_HS);
        TEST_SETUP_EXCEPT();
        AI_P13AX_ASSERT_NO_EXCEPT_AND("before the same-paddr-alias inval prefetch repeated-zero cross-16B store CBOM post-store handoff starts, base and alias views expose seeded split words, post-store word, zero probes, adjacent block, marker, and guards",
            ld(target_base_vaddr) == block_seeds[round] &&
            ld(target_alias_vaddr) == block_seeds[round] &&
            ld(low_word_base_vaddr) == seed_word1 &&
            ld(low_word_alias_vaddr) == seed_word1 &&
            ld(high_word_base_vaddr) == seed_word2 &&
            ld(high_word_alias_vaddr) == seed_word2 &&
            ld(post_store_base_vaddr) == seed_word3 &&
            ld(post_store_alias_vaddr) == seed_word3 &&
            ld(first_zero_base_vaddr) == expected_first_zero_seed &&
            ld(first_zero_alias_vaddr) == expected_first_zero_seed &&
            ld(second_zero_base_vaddr) == expected_second_zero_seed &&
            ld(second_zero_alias_vaddr) == expected_second_zero_seed &&
            ld(adjacent_base_vaddr) == adjacent_seeds[round] &&
            ld(adjacent_alias_vaddr) == adjacent_seeds[round] &&
            ld(marker_base_vaddr) == marker_seeds[round] &&
            ld(marker_alias_vaddr) == marker_seeds[round]
        );

        TEST_SETUP_EXCEPT();
        ai_p13bf_cbo_inval_prefetch_zero_zero_cross16_store_cbom_post_store_window(
            inval_alias_vaddr, prefetch_base_vaddr,
            first_zero_alias_vaddr, second_zero_alias_vaddr,
            cross16_store_alias_vaddr, low_word_alias_vaddr,
            high_word_alias_vaddr, clean_alias_vaddr, flush_alias_vaddr,
            post_store_alias_vaddr, marker_alias_vaddr,
            cross16_payloads[round], post_cbom_updates[round],
            marker_updates[round], &low_word_observed, &high_word_observed,
            &post_store_observed, &marker_observed);
        AI_P13AX_ASSERT_NO_EXCEPT_AND("same-paddr alias repeated CBO.ZERO must preserve the split-store overlay across CBOM clean/flush before the post-CBOM store and marker make progress",
            low_word_observed == expected_low_word &&
            high_word_observed == expected_high_word &&
            post_store_observed == post_cbom_updates[round] &&
            marker_observed == marker_updates[round] &&
            ld(target_base_vaddr) == 0ULL &&
            ld(target_alias_vaddr) == 0ULL &&
            ld(low_word_base_vaddr) == expected_low_word &&
            ld(low_word_alias_vaddr) == expected_low_word &&
            ld(high_word_base_vaddr) == expected_high_word &&
            ld(high_word_alias_vaddr) == expected_high_word &&
            ld(post_store_base_vaddr) == post_cbom_updates[round] &&
            ld(post_store_alias_vaddr) == post_cbom_updates[round] &&
            ld(first_zero_base_vaddr) == 0ULL &&
            ld(first_zero_alias_vaddr) == 0ULL &&
            ld(second_zero_base_vaddr) == 0ULL &&
            ld(second_zero_alias_vaddr) == 0ULL &&
            ld(marker_base_vaddr) == marker_updates[round] &&
            ld(marker_alias_vaddr) == marker_updates[round]
        );

        TEST_SETUP_EXCEPT();
        AI_P13AX_ASSERT_NO_EXCEPT_AND("the same-paddr-alias repeated-zero split-store to CBOM post-store handoff preserves guards and the adjacent block through both views",
            ld(before_guard_base_vaddr) == before_guards[round] &&
            ld(before_guard_alias_vaddr) == before_guards[round] &&
            ld(after_guard_base_vaddr) == after_guards[round] &&
            ld(after_guard_alias_vaddr) == after_guards[round] &&
            ld(marker_guard_base_vaddr) == marker_guards[round] &&
            ld(marker_guard_alias_vaddr) == marker_guards[round] &&
            ld(adjacent_base_vaddr) == adjacent_seeds[round] &&
            ld(adjacent_alias_vaddr) == adjacent_seeds[round]
        );

        goto_priv(PRIV_M);
        TEST_ASSERT("backing memory after same-paddr-alias inval prefetch repeated-zero cross-16B store CBOM post-store handoff contains only the split-store overlay and post-CBOM word over an otherwise zeroed block, marker update, and unchanged guards",
            ai_p13bb_zero_block_matches_cross16_store_and_word(
                target_base_paddr, cross16_payloads[round],
                3U, post_cbom_updates[round]) &&
            ai_p13ax_block_matches_seed(adjacent_base_paddr,
                adjacent_seeds[round]) &&
            read64(before_guard_paddr) == before_guards[round] &&
            read64(after_guard_paddr) == after_guards[round] &&
            read64(marker_paddr) == marker_updates[round] &&
            read64(marker_guard_paddr) == marker_guards[round]
        );
    }

    ai_p13ax_reset_hs_env(MENVCFG_CBIE01 | MENVCFG_CBZE | MENVCFG_CBZFE);

    TEST_END();
}
