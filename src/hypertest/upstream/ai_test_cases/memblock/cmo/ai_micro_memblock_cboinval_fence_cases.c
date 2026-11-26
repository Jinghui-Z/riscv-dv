#include <rvh_test.h>
#include <page_tables.h>
#include <instructions.h>
#include <stdint.h>

#define AI_ASSERT_NO_EXCEPT_AND(message, cond) \
    TEST_ASSERT(message, excpt.triggered == false && (cond))
#define AI_MEMBLOCK_CBOINVAL_FENCE_ARRAY_SIZE(array) \
    (sizeof(array) / sizeof((array)[0]))
#define AI_MEMBLOCK_CBOINVAL_FENCE_BLOCK_WORDS 8U

static inline void ai_memblock_cboinval_fence_prepare_hs_env(void)
{
    goto_priv(PRIV_M);
    CSRS(CSR_MENVCFG, MENVCFG_CBIE01);
    hspt_init();
    sfence_vma();
}

static inline void ai_memblock_cboinval_fence_reset_hs_env(void)
{
    goto_priv(PRIV_M);
    CSRC(CSR_MENVCFG, MENVCFG_CBIE01);
    hspt_init();
    sfence_vma();
}

static inline void ai_memblock_cboinval_fence_seed_block(uintptr_t paddr,
    uint64_t seed)
{
    for (size_t i = 0; i < AI_MEMBLOCK_CBOINVAL_FENCE_BLOCK_WORDS; ++i) {
        write64(paddr + i * sizeof(uint64_t),
            seed + i * 0x1111111111111111ULL);
    }
}

static inline bool ai_memblock_cboinval_fence_block_matches_two_updates(
    uintptr_t paddr,
    uint64_t seed,
    size_t first_word,
    uint64_t first_value,
    size_t second_word,
    uint64_t second_value)
{
    for (size_t i = 0; i < AI_MEMBLOCK_CBOINVAL_FENCE_BLOCK_WORDS; ++i) {
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

static inline void ai_memblock_cboinval_fence_then_post_stores(
    uintptr_t inval_vaddr,
    uintptr_t target_vaddr,
    uint64_t target_update,
    uintptr_t neighbor_vaddr,
    uint64_t neighbor_update,
    uintptr_t marker_vaddr,
    uint64_t marker_update)
{
    asm volatile(
        ".option push\n\t"
        ".option norvc\n\t"
        ".insn i 0x0f, 0x2, x0, %[inval_addr], 0\n\t"
        "fence rw, rw\n\t"
        "sd %[target_update], 0(%[target_addr])\n\t"
        "sd %[neighbor_update], 0(%[neighbor_addr])\n\t"
        "sd %[marker_update], 0(%[marker_addr])\n\t"
        "fence rw, rw\n\t"
        ".option pop\n\t"
        :
        : [inval_addr] "r"(inval_vaddr),
          [target_addr] "r"(target_vaddr),
          [target_update] "r"(target_update),
          [neighbor_addr] "r"(neighbor_vaddr),
          [neighbor_update] "r"(neighbor_update),
          [marker_addr] "r"(marker_vaddr),
          [marker_update] "r"(marker_update)
        : "memory");
}

bool ai_micro_hs_cboinval_then_fence_no_stale_inval_owner_or_marker_drop_corner()
{
    TEST_START();

    const enum test_page target_page = SWITCH1;
    const enum test_page marker_page = SWITCH2;
    const uintptr_t target_base_vaddr = hs_page_base(target_page) + 0x4c0U;
    const uintptr_t target_base_paddr = phys_page_base(target_page) + 0x4c0U;
    const uintptr_t target_word_vaddr = target_base_vaddr;
    const uintptr_t neighbor_word_vaddr =
        target_base_vaddr + 3U * sizeof(uint64_t);
    const uintptr_t inval_vaddr =
        target_base_vaddr + 4U * sizeof(uint64_t);
    const uintptr_t target_tail_vaddr =
        target_base_vaddr +
        (AI_MEMBLOCK_CBOINVAL_FENCE_BLOCK_WORDS - 1U) * sizeof(uint64_t);
    const uintptr_t before_guard_vaddr =
        target_base_vaddr - sizeof(uint64_t);
    const uintptr_t after_guard_vaddr =
        target_base_vaddr + AI_MEMBLOCK_CBOINVAL_FENCE_BLOCK_WORDS *
        sizeof(uint64_t);
    const uintptr_t before_guard_paddr =
        target_base_paddr - sizeof(uint64_t);
    const uintptr_t after_guard_paddr =
        target_base_paddr + AI_MEMBLOCK_CBOINVAL_FENCE_BLOCK_WORDS *
        sizeof(uint64_t);
    const uintptr_t marker_vaddr = hs_page_base(marker_page) + 0x4c0U;
    const uintptr_t marker_paddr = phys_page_base(marker_page) + 0x4c0U;
    const uintptr_t marker_guard_vaddr = marker_vaddr + sizeof(uint64_t);
    const uintptr_t marker_guard_paddr = marker_paddr + sizeof(uint64_t);
    const uint64_t block_seeds[] = {
        0x13572468abcdef01ULL,
        0x24681357bcdef012ULL,
        0x35792468cdef0123ULL,
    };
    const uint64_t target_updates[] = {
        0x0a0b0c0d0e0f1011ULL,
        0x1a1b1c1d1e1f2021ULL,
        0x2a2b2c2d2e2f3031ULL,
    };
    const uint64_t neighbor_updates[] = {
        0x8877665544332211ULL,
        0x7766554433221100ULL,
        0x66554433221100ffULL,
    };
    const uint64_t before_guards[] = {
        0xfedcba9876543210ULL,
        0x0123456789abcdefULL,
        0x55aa55aa33cc33ccULL,
    };
    const uint64_t after_guards[] = {
        0x1020304050607080ULL,
        0x1122334455667788ULL,
        0x99aabbccddeeff00ULL,
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

    ai_memblock_cboinval_fence_prepare_hs_env();

    for (size_t round = 0;
         round < AI_MEMBLOCK_CBOINVAL_FENCE_ARRAY_SIZE(block_seeds);
         ++round) {
        const uint64_t expected_neighbor_seed =
            block_seeds[round] + 3U * 0x1111111111111111ULL;
        const uint64_t expected_tail =
            block_seeds[round] +
            (AI_MEMBLOCK_CBOINVAL_FENCE_BLOCK_WORDS - 1U) *
            0x1111111111111111ULL;

        goto_priv(PRIV_M);
        ai_memblock_cboinval_fence_seed_block(target_base_paddr,
            block_seeds[round]);
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
        AI_ASSERT_NO_EXCEPT_AND("before the CBO.INVAL then fence owner-window starts, target block and marker are readable with seeded values",
            warmed_target == block_seeds[round] &&
            warmed_neighbor == expected_neighbor_seed &&
            warmed_tail == expected_tail &&
            warmed_marker == marker_seeds[round]
        );

        TEST_SETUP_EXCEPT();
        ai_memblock_cboinval_fence_then_post_stores(inval_vaddr,
            target_word_vaddr, target_updates[round], neighbor_word_vaddr,
            neighbor_updates[round], marker_vaddr, marker_updates[round]);
        const uint64_t observed_target = ld(target_word_vaddr);
        const uint64_t observed_neighbor = ld(neighbor_word_vaddr);
        const uint64_t observed_tail = ld(target_tail_vaddr);
        const uint64_t observed_marker = ld(marker_vaddr);
        AI_ASSERT_NO_EXCEPT_AND("CBO.INVAL immediately followed by fence releases inval ownership before post-fence stores and marker progress",
            observed_target == target_updates[round] &&
            observed_neighbor == neighbor_updates[round] &&
            observed_tail == expected_tail &&
            observed_marker == marker_updates[round]
        );

        TEST_SETUP_EXCEPT();
        AI_ASSERT_NO_EXCEPT_AND("the CBO.INVAL then fence owner-window does not corrupt target or marker guards",
            ld(before_guard_vaddr) == before_guards[round] &&
            ld(after_guard_vaddr) == after_guards[round] &&
            ld(marker_guard_vaddr) == marker_guards[round]
        );

        goto_priv(PRIV_M);
        TEST_ASSERT("backing memory after the CBO.INVAL then fence owner-window contains only the post-fence stores, marker update, and unchanged guards",
            ai_memblock_cboinval_fence_block_matches_two_updates(
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

    ai_memblock_cboinval_fence_reset_hs_env();

    TEST_END();
}
