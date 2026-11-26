#include <rvh_test.h>
#include <page_tables.h>
#include <stddef.h>
#include <stdint.h>

#define AI_P13AU_ASSERT_NO_EXCEPT_AND(message, cond) \
    TEST_ASSERT(message, excpt.triggered == false && (cond))

#define AI_P13AU_BLOCK_BYTES 64U
#define AI_P13AU_BLOCK_WORDS 8U

static inline pte_t *ai_p13au_hs_leaf_entry(enum test_page page)
{
#ifdef sv48
    return &hspt[4][page];
#else
    return &hspt[2][page];
#endif
}

static inline void ai_p13au_prepare_hs_env(uint64_t menvcfg_bits)
{
    goto_priv(PRIV_M);
    CSRS(CSR_MENVCFG, menvcfg_bits);
    hspt_init();
    sfence_vma();
}

static inline uintptr_t ai_p13au_prepare_same_paddr_alias(
    enum test_page alias_page,
    enum test_page base_page)
{
    goto_priv(PRIV_M);
    *ai_p13au_hs_leaf_entry(alias_page) =
        (phys_page_base(base_page) >> 2) | PTE_V | PTE_AD | PTE_RWX;
    sfence_vma();

    return hs_page_base(alias_page);
}

static inline void ai_p13au_reset_hs_env(uint64_t menvcfg_bits)
{
    goto_priv(PRIV_M);
    CSRC(CSR_MENVCFG, menvcfg_bits);
    hspt_init();
    sfence_vma();
}

static inline uint64_t ai_p13au_seed_word(uint64_t seed, uintptr_t word_idx)
{
    return seed + (word_idx * 0x1111111111111111ULL);
}

static inline void ai_p13au_seed_block(uintptr_t paddr, uint64_t seed)
{
    for (uintptr_t i = 0; i < AI_P13AU_BLOCK_WORDS; ++i) {
        write64(paddr + (i * sizeof(uint64_t)),
            ai_p13au_seed_word(seed, i));
    }
}

static inline bool ai_p13au_block_matches_seed(uintptr_t paddr, uint64_t seed)
{
    for (uintptr_t i = 0; i < AI_P13AU_BLOCK_WORDS; ++i) {
        if (read64(paddr + (i * sizeof(uint64_t))) !=
            ai_p13au_seed_word(seed, i)) {
            return false;
        }
    }

    return true;
}

static inline bool ai_p13au_block_all_zero_except_word(
    uintptr_t paddr,
    uintptr_t changed_word_idx,
    uint64_t changed_value)
{
    for (uintptr_t i = 0; i < AI_P13AU_BLOCK_WORDS; ++i) {
        const uint64_t expected = (i == changed_word_idx) ? changed_value : 0ULL;

        if (read64(paddr + (i * sizeof(uint64_t))) != expected) {
            return false;
        }
    }

    return true;
}

static inline void ai_p13au_cbo_inval_prefetch_zero_zero_lrsc_window(
    uintptr_t inval_vaddr,
    uintptr_t prefetch_vaddr,
    uintptr_t first_zero_vaddr,
    uintptr_t second_zero_vaddr,
    uintptr_t lrsc_vaddr,
    uintptr_t marker_vaddr,
    uint64_t sc_value,
    uint64_t marker_value,
    uint64_t *lr_observed,
    uint64_t *sc_status,
    uint64_t *marker_observed)
{
    uint64_t lr_readback = 0ULL;
    uint64_t status = 1ULL;
    uint64_t marker_readback = 0ULL;

    asm volatile(
        ".option push\n\t"
        ".option norvc\n\t"
        ".insn i 0x0f, 0x2, x0, %[inval_addr], 0\n\t"
        ".insn i 0x13, 0x6, x0, %[prefetch_addr], 1\n\t"
        ".insn i 0x0f, 0x2, x0, %[first_zero_addr], 4\n\t"
        ".insn i 0x0f, 0x2, x0, %[second_zero_addr], 4\n\t"
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
          [lrsc_addr] "r"(lrsc_vaddr),
          [sc_value] "r"(sc_value),
          [marker_addr] "r"(marker_vaddr),
          [marker_value] "r"(marker_value)
        : "memory");

    *lr_observed = lr_readback;
    *sc_status = status;
    *marker_observed = marker_readback;
}

bool ai_micro_hs_cbo_inval_prefetch_same_paddr_alias_refill_then_cbo_zero_cbo_zero_lrsc_no_stale_repeated_zero_reservation_or_marker_drop_corner()
{
    TEST_START();

    const enum test_page base_page = SWITCH1;
    const enum test_page alias_page = SWITCH2;
    const uintptr_t target_offset = 0xb00U;
    const uintptr_t target_base_vaddr =
        hs_page_base(base_page) + target_offset;
    const uintptr_t target_base_paddr =
        phys_page_base(base_page) + target_offset;
    const uintptr_t prefetch_base_vaddr =
        target_base_vaddr + sizeof(uint64_t);
    const uintptr_t first_zero_base_vaddr =
        target_base_vaddr + 4U * sizeof(uint64_t);
    const uintptr_t lrsc_base_vaddr = target_base_vaddr;
    const uintptr_t middle_probe_base_vaddr =
        target_base_vaddr + 5U * sizeof(uint64_t);
    const uintptr_t second_zero_base_vaddr =
        target_base_vaddr + (AI_P13AU_BLOCK_WORDS - 1U) * sizeof(uint64_t);
    const uintptr_t before_guard_base_vaddr =
        target_base_vaddr - sizeof(uint64_t);
    const uintptr_t after_guard_base_vaddr =
        target_base_vaddr + 2U * AI_P13AU_BLOCK_BYTES;
    const uintptr_t adjacent_base_vaddr =
        target_base_vaddr + AI_P13AU_BLOCK_BYTES;
    const uintptr_t marker_base_vaddr =
        target_base_vaddr + 3U * AI_P13AU_BLOCK_BYTES;
    const uintptr_t marker_guard_base_vaddr =
        marker_base_vaddr + sizeof(uint64_t);
    const uintptr_t before_guard_paddr =
        target_base_paddr - sizeof(uint64_t);
    const uintptr_t after_guard_paddr =
        target_base_paddr + 2U * AI_P13AU_BLOCK_BYTES;
    const uintptr_t adjacent_base_paddr =
        target_base_paddr + AI_P13AU_BLOCK_BYTES;
    const uintptr_t marker_paddr =
        target_base_paddr + 3U * AI_P13AU_BLOCK_BYTES;
    const uintptr_t marker_guard_paddr =
        marker_paddr + sizeof(uint64_t);
    const uint64_t block_seeds[] = {
        0x93a4b5c6d7e8f901ULL,
        0xa4b5c6d7e8f90112ULL,
        0xb5c6d7e8f9011223ULL,
    };
    const uint64_t adjacent_seeds[] = {
        0xd0c1b2a394857667ULL,
        0x8899aabbccddeeffULL,
        0x0123456789abcdefULL,
    };
    const uint64_t sc_payloads[] = {
        0x5a5a0000c3c3ffffULL,
        0x6b6b1111d2d2eeeeULL,
        0x7c7c2222e1e1ddddULL,
    };
    const uint64_t before_guards[] = {
        0x0f1e2d3c4b5a6978ULL,
        0x1021324354657687ULL,
        0x98badcfe10325476ULL,
    };
    const uint64_t after_guards[] = {
        0xfedcba9876543210ULL,
        0x55aa7733cc11ee99ULL,
        0x1123581321345589ULL,
    };
    const uint64_t marker_seeds[] = {
        0x3333444455556666ULL,
        0x777788889999aaaaULL,
        0xbbbbccccddddeeeeULL,
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
    uintptr_t alias_page_base_vaddr = 0;
    uintptr_t target_alias_vaddr = 0;
    uintptr_t inval_alias_vaddr = 0;
    uintptr_t first_zero_alias_vaddr = 0;
    uintptr_t lrsc_alias_vaddr = 0;
    uintptr_t middle_probe_alias_vaddr = 0;
    uintptr_t second_zero_alias_vaddr = 0;
    uintptr_t before_guard_alias_vaddr = 0;
    uintptr_t after_guard_alias_vaddr = 0;
    uintptr_t adjacent_alias_vaddr = 0;
    uintptr_t marker_alias_vaddr = 0;
    uintptr_t marker_guard_alias_vaddr = 0;

    ai_p13au_prepare_hs_env(MENVCFG_CBIE01 | MENVCFG_CBZE);
    alias_page_base_vaddr =
        ai_p13au_prepare_same_paddr_alias(alias_page, base_page);
    target_alias_vaddr = alias_page_base_vaddr + target_offset;
    inval_alias_vaddr =
        target_alias_vaddr + 6U * sizeof(uint64_t);
    first_zero_alias_vaddr =
        target_alias_vaddr + 4U * sizeof(uint64_t);
    lrsc_alias_vaddr = target_alias_vaddr;
    middle_probe_alias_vaddr =
        target_alias_vaddr + 5U * sizeof(uint64_t);
    second_zero_alias_vaddr =
        target_alias_vaddr + (AI_P13AU_BLOCK_WORDS - 1U) * sizeof(uint64_t);
    before_guard_alias_vaddr =
        target_alias_vaddr - sizeof(uint64_t);
    after_guard_alias_vaddr =
        target_alias_vaddr + 2U * AI_P13AU_BLOCK_BYTES;
    adjacent_alias_vaddr =
        target_alias_vaddr + AI_P13AU_BLOCK_BYTES;
    marker_alias_vaddr =
        target_alias_vaddr + 3U * AI_P13AU_BLOCK_BYTES;
    marker_guard_alias_vaddr =
        marker_alias_vaddr + sizeof(uint64_t);

    for (size_t round = 0; round < sizeof(block_seeds) / sizeof(block_seeds[0]);
         ++round) {
        const uint64_t expected_first_zero_seed =
            ai_p13au_seed_word(block_seeds[round], 4U);
        const uint64_t expected_middle_seed =
            ai_p13au_seed_word(block_seeds[round], 5U);
        const uint64_t expected_second_zero_seed =
            ai_p13au_seed_word(block_seeds[round], AI_P13AU_BLOCK_WORDS - 1U);
        uint64_t lr_observed = 0xffffffffffffffffULL;
        uint64_t sc_status = 1ULL;
        uint64_t marker_observed = 0ULL;

        goto_priv(PRIV_M);
        ai_p13au_seed_block(target_base_paddr, block_seeds[round]);
        ai_p13au_seed_block(adjacent_base_paddr, adjacent_seeds[round]);
        write64(before_guard_paddr, before_guards[round]);
        write64(after_guard_paddr, after_guards[round]);
        write64(marker_paddr, marker_seeds[round]);
        write64(marker_guard_paddr, marker_guards[round]);

        goto_priv(PRIV_HS);
        TEST_SETUP_EXCEPT();
        AI_P13AU_ASSERT_NO_EXCEPT_AND("before the same-paddr-alias inval prefetch repeated-zero LR/SC handoff starts, base and alias views expose seeded target probes, adjacent block, marker, and guards",
            ld(target_base_vaddr) == block_seeds[round] &&
            ld(target_alias_vaddr) == block_seeds[round] &&
            ld(first_zero_base_vaddr) == expected_first_zero_seed &&
            ld(first_zero_alias_vaddr) == expected_first_zero_seed &&
            ld(middle_probe_base_vaddr) == expected_middle_seed &&
            ld(middle_probe_alias_vaddr) == expected_middle_seed &&
            ld(second_zero_base_vaddr) == expected_second_zero_seed &&
            ld(second_zero_alias_vaddr) == expected_second_zero_seed &&
            ld(adjacent_base_vaddr) == adjacent_seeds[round] &&
            ld(adjacent_alias_vaddr) == adjacent_seeds[round] &&
            ld(marker_base_vaddr) == marker_seeds[round] &&
            ld(marker_alias_vaddr) == marker_seeds[round]
        );

        TEST_SETUP_EXCEPT();
        ai_p13au_cbo_inval_prefetch_zero_zero_lrsc_window(
            inval_alias_vaddr, prefetch_base_vaddr,
            first_zero_alias_vaddr, second_zero_alias_vaddr,
            lrsc_alias_vaddr, marker_alias_vaddr,
            sc_payloads[round], marker_updates[round],
            &lr_observed, &sc_status, &marker_observed);
        AI_P13AU_ASSERT_NO_EXCEPT_AND("same-paddr alias repeated CBO.ZERO must leave a zero image for alias-view LR/SC and preserve the reservation through SC",
            lr_observed == 0ULL &&
            sc_status == 0ULL &&
            marker_observed == marker_updates[round] &&
            ld(lrsc_base_vaddr) == sc_payloads[round] &&
            ld(lrsc_alias_vaddr) == sc_payloads[round] &&
            ld(first_zero_base_vaddr) == 0ULL &&
            ld(first_zero_alias_vaddr) == 0ULL &&
            ld(middle_probe_base_vaddr) == 0ULL &&
            ld(middle_probe_alias_vaddr) == 0ULL &&
            ld(second_zero_base_vaddr) == 0ULL &&
            ld(second_zero_alias_vaddr) == 0ULL &&
            ld(marker_base_vaddr) == marker_updates[round] &&
            ld(marker_alias_vaddr) == marker_updates[round]
        );

        TEST_SETUP_EXCEPT();
        AI_P13AU_ASSERT_NO_EXCEPT_AND("the same-paddr-alias repeated-zero to LR/SC handoff preserves guards and the adjacent block through both views",
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
        TEST_ASSERT("backing memory after same-paddr-alias inval prefetch repeated-zero LR/SC handoff contains only the successful SC payload over an otherwise zeroed block, marker update, and unchanged guards",
            ai_p13au_block_all_zero_except_word(target_base_paddr,
                0U, sc_payloads[round]) &&
            ai_p13au_block_matches_seed(adjacent_base_paddr,
                adjacent_seeds[round]) &&
            read64(before_guard_paddr) == before_guards[round] &&
            read64(after_guard_paddr) == after_guards[round] &&
            read64(marker_paddr) == marker_updates[round] &&
            read64(marker_guard_paddr) == marker_guards[round]
        );
    }

    ai_p13au_reset_hs_env(MENVCFG_CBIE01 | MENVCFG_CBZE);

    TEST_END();
}
