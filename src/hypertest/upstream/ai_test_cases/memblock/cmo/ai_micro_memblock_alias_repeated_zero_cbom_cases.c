#include <rvh_test.h>
#include <page_tables.h>
#include <stddef.h>
#include <stdint.h>

#define AI_P13AR_ASSERT_NO_EXCEPT_AND(message, cond) \
    TEST_ASSERT(message, excpt.triggered == false && (cond))

#define AI_P13AR_BLOCK_BYTES 64U
#define AI_P13AR_BLOCK_WORDS 8U

static inline pte_t *ai_p13ar_hs_leaf_entry(enum test_page page)
{
#ifdef sv48
    return &hspt[4][page];
#else
    return &hspt[2][page];
#endif
}

static inline void ai_p13ar_prepare_hs_env(uint64_t menvcfg_bits)
{
    goto_priv(PRIV_M);
    CSRS(CSR_MENVCFG, menvcfg_bits);
    hspt_init();
    sfence_vma();
}

static inline uintptr_t ai_p13ar_prepare_same_paddr_alias(
    enum test_page alias_page,
    enum test_page base_page)
{
    goto_priv(PRIV_M);
    *ai_p13ar_hs_leaf_entry(alias_page) =
        (phys_page_base(base_page) >> 2) | PTE_V | PTE_AD | PTE_RWX;
    sfence_vma();

    return hs_page_base(alias_page);
}

static inline void ai_p13ar_reset_hs_env(uint64_t menvcfg_bits)
{
    goto_priv(PRIV_M);
    CSRC(CSR_MENVCFG, menvcfg_bits);
    hspt_init();
    sfence_vma();
}

static inline uint64_t ai_p13ar_seed_word(uint64_t seed, uintptr_t word_idx)
{
    return seed + (word_idx * 0x1111111111111111ULL);
}

static inline void ai_p13ar_seed_block(uintptr_t paddr, uint64_t seed)
{
    for (uintptr_t i = 0; i < AI_P13AR_BLOCK_WORDS; ++i) {
        write64(paddr + (i * sizeof(uint64_t)),
            ai_p13ar_seed_word(seed, i));
    }
}

static inline bool ai_p13ar_block_matches_seed(uintptr_t paddr, uint64_t seed)
{
    for (uintptr_t i = 0; i < AI_P13AR_BLOCK_WORDS; ++i) {
        if (read64(paddr + (i * sizeof(uint64_t))) !=
            ai_p13ar_seed_word(seed, i)) {
            return false;
        }
    }

    return true;
}

static inline bool ai_p13ar_block_all_zero_except_word(
    uintptr_t paddr,
    uintptr_t changed_word_idx,
    uint64_t changed_value)
{
    for (uintptr_t i = 0; i < AI_P13AR_BLOCK_WORDS; ++i) {
        const uint64_t expected = (i == changed_word_idx) ? changed_value : 0ULL;

        if (read64(paddr + (i * sizeof(uint64_t))) != expected) {
            return false;
        }
    }

    return true;
}

static inline void ai_p13ar_cbo_inval_prefetch_zero_zero_cbom_post_store_window(
    uintptr_t inval_vaddr,
    uintptr_t prefetch_vaddr,
    uintptr_t first_zero_vaddr,
    uintptr_t second_zero_vaddr,
    uintptr_t clean_vaddr,
    uintptr_t flush_vaddr,
    uintptr_t post_store_vaddr,
    uintptr_t marker_vaddr,
    uint64_t post_store_value,
    uint64_t marker_value,
    uint64_t *post_store_observed,
    uint64_t *marker_observed)
{
    uint64_t post_store_readback = 0ULL;
    uint64_t marker_readback = 0ULL;

    asm volatile(
        ".option push\n\t"
        ".option norvc\n\t"
        ".insn i 0x0f, 0x2, x0, %[inval_addr], 0\n\t"
        ".insn i 0x13, 0x6, x0, %[prefetch_addr], 1\n\t"
        ".insn i 0x0f, 0x2, x0, %[first_zero_addr], 4\n\t"
        ".insn i 0x0f, 0x2, x0, %[second_zero_addr], 4\n\t"
        ".insn i 0x0f, 0x2, x0, %[clean_addr], 1\n\t"
        ".insn i 0x0f, 0x2, x0, %[flush_addr], 2\n\t"
        "sd %[post_store_value], 0(%[post_store_addr])\n\t"
        "sd %[marker_value], 0(%[marker_addr])\n\t"
        "fence rw, rw\n\t"
        "ld %[post_store_readback], 0(%[post_store_addr])\n\t"
        "ld %[marker_readback], 0(%[marker_addr])\n\t"
        ".option pop\n\t"
        : [post_store_readback] "=&r"(post_store_readback),
          [marker_readback] "=&r"(marker_readback)
        : [inval_addr] "r"(inval_vaddr),
          [prefetch_addr] "r"(prefetch_vaddr),
          [first_zero_addr] "r"(first_zero_vaddr),
          [second_zero_addr] "r"(second_zero_vaddr),
          [clean_addr] "r"(clean_vaddr),
          [flush_addr] "r"(flush_vaddr),
          [post_store_addr] "r"(post_store_vaddr),
          [marker_addr] "r"(marker_vaddr),
          [post_store_value] "r"(post_store_value),
          [marker_value] "r"(marker_value)
        : "memory");

    *post_store_observed = post_store_readback;
    *marker_observed = marker_readback;
}

bool ai_micro_hs_cbo_inval_prefetch_same_paddr_alias_refill_then_cbo_zero_cbo_zero_cbom_post_store_no_stale_repeated_zero_cmo_owner_or_marker_drop_corner()
{
    TEST_START();

    const enum test_page base_page = SWITCH1;
    const enum test_page alias_page = SWITCH2;
    const uintptr_t target_offset = 0xa40U;
    const uintptr_t target_base_vaddr =
        hs_page_base(base_page) + target_offset;
    const uintptr_t target_base_paddr =
        phys_page_base(base_page) + target_offset;
    const uintptr_t prefetch_base_vaddr =
        target_base_vaddr + sizeof(uint64_t);
    const uintptr_t clean_base_vaddr =
        target_base_vaddr + 3U * sizeof(uint64_t);
    const uintptr_t first_zero_base_vaddr =
        target_base_vaddr + 4U * sizeof(uint64_t);
    const uintptr_t post_store_base_vaddr =
        target_base_vaddr + 5U * sizeof(uint64_t);
    const uintptr_t second_zero_base_vaddr =
        target_base_vaddr + (AI_P13AR_BLOCK_WORDS - 1U) * sizeof(uint64_t);
    const uintptr_t before_guard_base_vaddr =
        target_base_vaddr - sizeof(uint64_t);
    const uintptr_t after_guard_base_vaddr =
        target_base_vaddr + 2U * AI_P13AR_BLOCK_BYTES;
    const uintptr_t adjacent_base_vaddr =
        target_base_vaddr + AI_P13AR_BLOCK_BYTES;
    const uintptr_t marker_base_vaddr =
        target_base_vaddr + 3U * AI_P13AR_BLOCK_BYTES;
    const uintptr_t marker_guard_base_vaddr =
        marker_base_vaddr + sizeof(uint64_t);
    const uintptr_t before_guard_paddr =
        target_base_paddr - sizeof(uint64_t);
    const uintptr_t after_guard_paddr =
        target_base_paddr + 2U * AI_P13AR_BLOCK_BYTES;
    const uintptr_t adjacent_base_paddr =
        target_base_paddr + AI_P13AR_BLOCK_BYTES;
    const uintptr_t marker_paddr =
        target_base_paddr + 3U * AI_P13AR_BLOCK_BYTES;
    const uintptr_t marker_guard_paddr =
        marker_paddr + sizeof(uint64_t);
    const uint64_t block_seeds[] = {
        0x0d1e2f3041526374ULL,
        0x1e2f304152637485ULL,
        0x2f30415263748596ULL,
    };
    const uint64_t adjacent_seeds[] = {
        0x89abcdef01234567ULL,
        0x76543210fedcba98ULL,
        0x2468ace013579bdfULL,
    };
    const uint64_t post_cbom_updates[] = {
        0x9a8b7c6d5e4f3021ULL,
        0x8b7c6d5e4f302112ULL,
        0x7c6d5e4f30211203ULL,
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
        0x9091929394959697ULL,
        0xa0a1a2a3a4a5a6a7ULL,
        0xb0b1b2b3b4b5b6b7ULL,
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
    uintptr_t alias_page_base_vaddr = 0;
    uintptr_t target_alias_vaddr = 0;
    uintptr_t inval_alias_vaddr = 0;
    uintptr_t clean_alias_vaddr = 0;
    uintptr_t first_zero_alias_vaddr = 0;
    uintptr_t post_store_alias_vaddr = 0;
    uintptr_t second_zero_alias_vaddr = 0;
    uintptr_t before_guard_alias_vaddr = 0;
    uintptr_t after_guard_alias_vaddr = 0;
    uintptr_t adjacent_alias_vaddr = 0;
    uintptr_t marker_alias_vaddr = 0;
    uintptr_t marker_guard_alias_vaddr = 0;

    ai_p13ar_prepare_hs_env(MENVCFG_CBIE01 | MENVCFG_CBZE | MENVCFG_CBZFE);
    alias_page_base_vaddr =
        ai_p13ar_prepare_same_paddr_alias(alias_page, base_page);
    target_alias_vaddr = alias_page_base_vaddr + target_offset;
    inval_alias_vaddr =
        target_alias_vaddr + 6U * sizeof(uint64_t);
    clean_alias_vaddr =
        target_alias_vaddr + 3U * sizeof(uint64_t);
    first_zero_alias_vaddr =
        target_alias_vaddr + 4U * sizeof(uint64_t);
    post_store_alias_vaddr =
        target_alias_vaddr + 5U * sizeof(uint64_t);
    second_zero_alias_vaddr =
        target_alias_vaddr + (AI_P13AR_BLOCK_WORDS - 1U) * sizeof(uint64_t);
    before_guard_alias_vaddr =
        target_alias_vaddr - sizeof(uint64_t);
    after_guard_alias_vaddr =
        target_alias_vaddr + 2U * AI_P13AR_BLOCK_BYTES;
    adjacent_alias_vaddr =
        target_alias_vaddr + AI_P13AR_BLOCK_BYTES;
    marker_alias_vaddr =
        target_alias_vaddr + 3U * AI_P13AR_BLOCK_BYTES;
    marker_guard_alias_vaddr =
        marker_alias_vaddr + sizeof(uint64_t);

    for (size_t round = 0; round < sizeof(block_seeds) / sizeof(block_seeds[0]);
         ++round) {
        const uint64_t expected_clean_seed =
            ai_p13ar_seed_word(block_seeds[round], 3U);
        const uint64_t expected_first_zero_seed =
            ai_p13ar_seed_word(block_seeds[round], 4U);
        const uint64_t expected_post_store_seed =
            ai_p13ar_seed_word(block_seeds[round], 5U);
        const uint64_t expected_second_zero_seed =
            ai_p13ar_seed_word(block_seeds[round], AI_P13AR_BLOCK_WORDS - 1U);
        uint64_t post_store_observed = 0ULL;
        uint64_t marker_observed = 0ULL;

        goto_priv(PRIV_M);
        ai_p13ar_seed_block(target_base_paddr, block_seeds[round]);
        ai_p13ar_seed_block(adjacent_base_paddr, adjacent_seeds[round]);
        write64(before_guard_paddr, before_guards[round]);
        write64(after_guard_paddr, after_guards[round]);
        write64(marker_paddr, marker_seeds[round]);
        write64(marker_guard_paddr, marker_guards[round]);

        goto_priv(PRIV_HS);
        TEST_SETUP_EXCEPT();
        AI_P13AR_ASSERT_NO_EXCEPT_AND("before the same-paddr-alias inval prefetch repeated-zero CBOM post-store handoff starts, base and alias views expose seeded target probes, adjacent block, marker, and guards",
            ld(target_base_vaddr) == block_seeds[round] &&
            ld(target_alias_vaddr) == block_seeds[round] &&
            ld(clean_base_vaddr) == expected_clean_seed &&
            ld(clean_alias_vaddr) == expected_clean_seed &&
            ld(first_zero_base_vaddr) == expected_first_zero_seed &&
            ld(first_zero_alias_vaddr) == expected_first_zero_seed &&
            ld(post_store_base_vaddr) == expected_post_store_seed &&
            ld(post_store_alias_vaddr) == expected_post_store_seed &&
            ld(second_zero_base_vaddr) == expected_second_zero_seed &&
            ld(second_zero_alias_vaddr) == expected_second_zero_seed &&
            ld(adjacent_base_vaddr) == adjacent_seeds[round] &&
            ld(adjacent_alias_vaddr) == adjacent_seeds[round] &&
            ld(marker_base_vaddr) == marker_seeds[round] &&
            ld(marker_alias_vaddr) == marker_seeds[round]
        );

        TEST_SETUP_EXCEPT();
        ai_p13ar_cbo_inval_prefetch_zero_zero_cbom_post_store_window(
            inval_alias_vaddr, prefetch_base_vaddr,
            first_zero_alias_vaddr, second_zero_alias_vaddr,
            clean_base_vaddr, second_zero_alias_vaddr,
            post_store_alias_vaddr, marker_alias_vaddr,
            post_cbom_updates[round], marker_updates[round],
            &post_store_observed, &marker_observed);
        AI_P13AR_ASSERT_NO_EXCEPT_AND("same-paddr alias repeated CBO.ZERO followed by base-alias CBOM must keep the block zeroed before the post-CBOM store and marker progress",
            post_store_observed == post_cbom_updates[round] &&
            marker_observed == marker_updates[round] &&
            ld(target_base_vaddr) == 0ULL &&
            ld(target_alias_vaddr) == 0ULL &&
            ld(clean_base_vaddr) == 0ULL &&
            ld(clean_alias_vaddr) == 0ULL &&
            ld(first_zero_base_vaddr) == 0ULL &&
            ld(first_zero_alias_vaddr) == 0ULL &&
            ld(post_store_base_vaddr) == post_cbom_updates[round] &&
            ld(post_store_alias_vaddr) == post_cbom_updates[round] &&
            ld(second_zero_base_vaddr) == 0ULL &&
            ld(second_zero_alias_vaddr) == 0ULL &&
            ld(marker_base_vaddr) == marker_updates[round] &&
            ld(marker_alias_vaddr) == marker_updates[round]
        );

        TEST_SETUP_EXCEPT();
        AI_P13AR_ASSERT_NO_EXCEPT_AND("the same-paddr-alias repeated-zero to CBOM post-store handoff preserves guards and the adjacent block through both views",
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
        TEST_ASSERT("backing memory after same-paddr-alias inval prefetch repeated-zero CBOM post-store handoff contains only the post-CBOM store over an otherwise zeroed block, marker update, and unchanged guards",
            ai_p13ar_block_all_zero_except_word(target_base_paddr,
                5U, post_cbom_updates[round]) &&
            ai_p13ar_block_matches_seed(adjacent_base_paddr,
                adjacent_seeds[round]) &&
            read64(before_guard_paddr) == before_guards[round] &&
            read64(after_guard_paddr) == after_guards[round] &&
            read64(marker_paddr) == marker_updates[round] &&
            read64(marker_guard_paddr) == marker_guards[round]
        );
    }

    ai_p13ar_reset_hs_env(MENVCFG_CBIE01 | MENVCFG_CBZE | MENVCFG_CBZFE);

    TEST_END();
}
