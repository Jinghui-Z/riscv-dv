#include <rvh_test.h>
#include <page_tables.h>
#include <instructions.h>
#include <stdint.h>

#define AI_ASSERT_NO_EXCEPT_AND(message, cond) \
    TEST_ASSERT(message, excpt.triggered == false && (cond))
#define AI_ARRAY_SIZE(array) (sizeof(array) / sizeof((array)[0]))

static inline uint64_t ai_micro_forward_dense_store_pattern(uint64_t seed,
    uintptr_t line_index)
{
    return seed ^ (0x9e3779b97f4a7c15ULL * (line_index + 3U)) ^
        (line_index << 11);
}

static inline void ai_micro_forward_dense_store_pressure(
    uintptr_t page_base_vaddr, uintptr_t line_count, uint64_t first_seed,
    uint64_t second_seed)
{
    for (uintptr_t i = 0; i < line_count; ++i) {
        *(volatile uint64_t *)(page_base_vaddr + i * 64U) =
            ai_micro_forward_dense_store_pattern(first_seed, i);
    }

    for (uintptr_t i = 0; i < line_count; ++i) {
        *(volatile uint64_t *)(page_base_vaddr + i * 64U) =
            ai_micro_forward_dense_store_pattern(second_seed, i);
    }
}

static inline uint64_t ai_micro_forward_merge_byte_halfword(uint64_t base,
    uint8_t byte_value, uint16_t halfword_value)
{
    const uint64_t byte_mask = 0xffULL << 8;
    const uint64_t halfword_mask = 0xffffULL << 32;

    return (base & ~byte_mask & ~halfword_mask) |
        ((uint64_t)byte_value << 8) |
        ((uint64_t)halfword_value << 32);
}

bool ai_micro_hs_load_forward_lsq_partial_over_sbuffer_full_store_byte_age_merge_corner()
{
    TEST_START();

    const enum test_page target_page = VSRWX_GRWX;
    const enum test_page filler_page = VSRW_GRW;
    const uintptr_t target_page_vaddr = hs_page_base(target_page);
    const uintptr_t filler_page_vaddr = hs_page_base(filler_page);
    const uintptr_t target_page_paddr = phys_page_base(target_page);
    const uintptr_t filler_page_paddr = phys_page_base(filler_page);
    const uintptr_t target_vaddr = target_page_vaddr + 0x180U;
    const uintptr_t guard_before_vaddr = target_vaddr - sizeof(uint64_t);
    const uintptr_t guard_after_vaddr = target_vaddr + sizeof(uint64_t);
    const uintptr_t target_paddr = target_page_paddr + 0x180U;
    const uintptr_t guard_before_paddr = target_paddr - sizeof(uint64_t);
    const uintptr_t guard_after_paddr = target_paddr + sizeof(uint64_t);
    const uintptr_t filler_sample0_paddr = filler_page_paddr + 0U * 64U;
    const uintptr_t filler_sample31_paddr = filler_page_paddr + 31U * 64U;
    const uint64_t seed_values[] = {
        0x1021324354657687ULL,
        0x89abcdef01234567ULL,
        0x55aa33cc77ee11ddULL
    };
    const uint64_t older_full_values[] = {
        0xf0e1d2c3b4a59687ULL,
        0x0123456789abcdefULL,
        0xa5a55a5ac3c33c3cULL
    };
    const uint8_t younger_byte_values[] = {
        0x5aU,
        0xc3U,
        0x2dU
    };
    const uint16_t younger_halfword_values[] = {
        0x6b7cU,
        0xd4e5U,
        0x9a0bU
    };
    const uint64_t guard_before_values[] = {
        0x0f1e2d3c4b5a6978ULL,
        0x7766554433221100ULL,
        0x3141592653589793ULL
    };
    const uint64_t guard_after_values[] = {
        0x8877aabbccddeeffULL,
        0xdeadbeefcafebabeULL,
        0x1234fedc5678ba98ULL
    };
    const uint64_t filler_first_seeds[] = {
        0x11c6f8ab93d04e27ULL,
        0x228be91f47a18d5eULL,
        0x3372da8bfb720c95ULL
    };
    const uint64_t filler_second_seeds[] = {
        0x55efbc737314da03ULL,
        0x66b6ade727e5193aULL,
        0x777d9e5bdbb65871ULL
    };

    goto_priv(PRIV_HS);
    hspt_init();
    sfence_vma();

    for (unsigned round = 0; round < AI_ARRAY_SIZE(seed_values); ++round) {
        const uint64_t expected =
            ai_micro_forward_merge_byte_halfword(older_full_values[round],
                younger_byte_values[round], younger_halfword_values[round]);

        goto_priv(PRIV_M);
        write64(guard_before_paddr, guard_before_values[round]);
        write64(target_paddr, seed_values[round]);
        write64(guard_after_paddr, guard_after_values[round]);
        write64(filler_sample0_paddr, 0ULL);
        write64(filler_sample31_paddr, 0ULL);

        goto_priv(PRIV_HS);
        TEST_SETUP_EXCEPT();
        AI_ASSERT_NO_EXCEPT_AND("before the LSQ-over-sbuffer byte-age merge probe starts, target, guards, and pressure lines are readable with their seeded values",
            ld(guard_before_vaddr) == guard_before_values[round] &&
            ld(target_vaddr) == seed_values[round] &&
            ld(guard_after_vaddr) == guard_after_values[round] &&
            ld(filler_page_vaddr + 0U * 64U) == 0ULL &&
            ld(filler_page_vaddr + 31U * 64U) == 0ULL
        );

        TEST_SETUP_EXCEPT();
        *(volatile uint64_t *)target_vaddr = older_full_values[round];
        ai_micro_forward_dense_store_pressure(filler_page_vaddr, 32U,
            filler_first_seeds[round], filler_second_seeds[round]);
        *(volatile uint8_t *)(target_vaddr + 1U) = younger_byte_values[round];
        *(volatile uint16_t *)(target_vaddr + 4U) = younger_halfword_values[round];
        uint64_t observed = ld(target_vaddr);
        fence_iorw_iorw();
        AI_ASSERT_NO_EXCEPT_AND("an immediate full-width load after older full-store pressure plus younger byte/halfword stores observes the byte-age merge with younger LSQ bytes overriding older sbuffer/cache bytes",
            observed == expected &&
            ld(filler_page_vaddr + 0U * 64U) ==
                ai_micro_forward_dense_store_pattern(filler_second_seeds[round], 0U) &&
            ld(filler_page_vaddr + 31U * 64U) ==
                ai_micro_forward_dense_store_pattern(filler_second_seeds[round], 31U)
        );

        TEST_SETUP_EXCEPT();
        AI_ASSERT_NO_EXCEPT_AND("the lower and upper guard words remain unchanged after the mixed LSQ/sbuffer forward-priority window",
            ld(guard_before_vaddr) == guard_before_values[round] &&
            ld(guard_after_vaddr) == guard_after_values[round]
        );

        goto_priv(PRIV_M);
        TEST_ASSERT("backing memory keeps the merged target image, unchanged guards, and latest pressure-line payloads after the LSQ-over-sbuffer byte-age merge probe",
            read64(guard_before_paddr) == guard_before_values[round] &&
            read64(target_paddr) == expected &&
            read64(guard_after_paddr) == guard_after_values[round] &&
            read64(filler_sample0_paddr) ==
                ai_micro_forward_dense_store_pattern(filler_second_seeds[round], 0U) &&
            read64(filler_sample31_paddr) ==
                ai_micro_forward_dense_store_pattern(filler_second_seeds[round], 31U)
        );
    }

    TEST_END();
}

bool ai_micro_hs_load_forward_lsq_full_over_sbuffer_partial_store_no_stale_byte_corner()
{
    TEST_START();

    const enum test_page target_page = VSRWX_GRWX;
    const enum test_page filler_page = VSRW_GRW;
    const uintptr_t target_page_vaddr = hs_page_base(target_page);
    const uintptr_t filler_page_vaddr = hs_page_base(filler_page);
    const uintptr_t target_page_paddr = phys_page_base(target_page);
    const uintptr_t filler_page_paddr = phys_page_base(filler_page);
    const uintptr_t target_vaddr = target_page_vaddr + 0x280U;
    const uintptr_t guard_before_vaddr = target_vaddr - sizeof(uint64_t);
    const uintptr_t guard_after_vaddr = target_vaddr + sizeof(uint64_t);
    const uintptr_t target_paddr = target_page_paddr + 0x280U;
    const uintptr_t guard_before_paddr = target_paddr - sizeof(uint64_t);
    const uintptr_t guard_after_paddr = target_paddr + sizeof(uint64_t);
    const uintptr_t filler_sample7_paddr = filler_page_paddr + 7U * 64U;
    const uintptr_t filler_sample23_paddr = filler_page_paddr + 23U * 64U;
    const uint64_t seed_values[] = {
        0x0123456789abcdefULL,
        0xfedcba9876543210ULL,
        0x13579bdf2468ace0ULL
    };
    const uint16_t older_halfword_values[] = {
        0x3141U,
        0xa5a5U,
        0x0badU
    };
    const uint8_t older_byte_values[] = {
        0x59U,
        0xc6U,
        0x72U
    };
    const uint64_t younger_full_values[] = {
        0x8877665544332211ULL,
        0x0f1e2d3c4b5a6978ULL,
        0xc001d00dcafef00dULL
    };
    const uint64_t guard_before_values[] = {
        0xfeedfacecafebeefULL,
        0x1021324354657687ULL,
        0x55aa55aa33cc33ccULL
    };
    const uint64_t guard_after_values[] = {
        0xdeadc0def00dbabeULL,
        0x7766554433221100ULL,
        0x3141592653589793ULL
    };
    const uint64_t filler_first_seeds[] = {
        0x123456789abcdef0ULL,
        0x23456789abcdef01ULL,
        0x3456789abcdef012ULL
    };
    const uint64_t filler_second_seeds[] = {
        0x456789abcdef0123ULL,
        0x56789abcdef01234ULL,
        0x6789abcdef012345ULL
    };

    goto_priv(PRIV_HS);
    hspt_init();
    sfence_vma();

    for (unsigned round = 0; round < AI_ARRAY_SIZE(seed_values); ++round) {
        goto_priv(PRIV_M);
        write64(guard_before_paddr, guard_before_values[round]);
        write64(target_paddr, seed_values[round]);
        write64(guard_after_paddr, guard_after_values[round]);
        write64(filler_sample7_paddr, 0ULL);
        write64(filler_sample23_paddr, 0ULL);

        goto_priv(PRIV_HS);
        TEST_SETUP_EXCEPT();
        AI_ASSERT_NO_EXCEPT_AND("before the LSQ-full-over-sbuffer-partial probe starts, target, guards, and pressure lines are readable with their seeded values",
            ld(guard_before_vaddr) == guard_before_values[round] &&
            ld(target_vaddr) == seed_values[round] &&
            ld(guard_after_vaddr) == guard_after_values[round] &&
            ld(filler_page_vaddr + 7U * 64U) == 0ULL &&
            ld(filler_page_vaddr + 23U * 64U) == 0ULL
        );

        TEST_SETUP_EXCEPT();
        *(volatile uint16_t *)(target_vaddr + 2U) = older_halfword_values[round];
        *(volatile uint8_t *)(target_vaddr + 6U) = older_byte_values[round];
        ai_micro_forward_dense_store_pressure(filler_page_vaddr, 32U,
            filler_first_seeds[round], filler_second_seeds[round]);
        *(volatile uint64_t *)target_vaddr = younger_full_values[round];
        uint64_t observed = ld(target_vaddr);
        fence_iorw_iorw();
        AI_ASSERT_NO_EXCEPT_AND("an immediate full-width load after older partial-store pressure plus a younger full store observes only the younger full-store payload with no stale partial byte leakage",
            observed == younger_full_values[round] &&
            ld(filler_page_vaddr + 7U * 64U) ==
                ai_micro_forward_dense_store_pattern(filler_second_seeds[round], 7U) &&
            ld(filler_page_vaddr + 23U * 64U) ==
                ai_micro_forward_dense_store_pattern(filler_second_seeds[round], 23U)
        );

        TEST_SETUP_EXCEPT();
        AI_ASSERT_NO_EXCEPT_AND("the guard words remain unchanged after the younger-full-over-older-partial forward-priority window",
            ld(guard_before_vaddr) == guard_before_values[round] &&
            ld(guard_after_vaddr) == guard_after_values[round]
        );

        goto_priv(PRIV_M);
        TEST_ASSERT("backing memory keeps only the final younger full-store image, unchanged guards, and latest pressure-line payloads after the LSQ-full-over-sbuffer-partial probe",
            read64(guard_before_paddr) == guard_before_values[round] &&
            read64(target_paddr) == younger_full_values[round] &&
            read64(guard_after_paddr) == guard_after_values[round] &&
            read64(filler_sample7_paddr) ==
                ai_micro_forward_dense_store_pattern(filler_second_seeds[round], 7U) &&
            read64(filler_sample23_paddr) ==
                ai_micro_forward_dense_store_pattern(filler_second_seeds[round], 23U)
        );
    }

    TEST_END();
}
