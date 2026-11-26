#include <rvh_test.h>
#include <page_tables.h>
#include <instructions.h>
#include <stdint.h>

#define AI_ASSERT_NO_EXCEPT_AND(message, cond) \
    TEST_ASSERT(message, excpt.triggered == false && (cond))
#define AI_MEMBLOCK_FAST_WK_DST_SLOTS 8U
#define AI_MEMBLOCK_FAST_WK_PRESSURE_LINES 24U
#define AI_MEMBLOCK_FAST_WK_ARRAY_SIZE(array) (sizeof(array) / sizeof((array)[0]))

static inline uint64_t ai_memblock_fast_wk_pressure_pattern(uint64_t seed,
    unsigned line)
{
    return seed ^ (0x9e3779b97f4a7c15ULL * (uint64_t)(line + 1U)) ^
        ((uint64_t)line << 9);
}

static inline void ai_memblock_fast_wk_seed_dst_slots(uintptr_t paddr,
    const uint64_t *values)
{
    for (unsigned i = 0; i < AI_MEMBLOCK_FAST_WK_DST_SLOTS; ++i) {
        write64(paddr + i * sizeof(uint64_t), values[i]);
    }
}

static inline bool ai_memblock_fast_wk_dst_slots_match(uintptr_t paddr,
    const uint64_t *expected)
{
    for (unsigned i = 0; i < AI_MEMBLOCK_FAST_WK_DST_SLOTS; ++i) {
        if (read64(paddr + i * sizeof(uint64_t)) != expected[i]) {
            return false;
        }
    }

    return true;
}

static inline void ai_memblock_fast_wk_store_pressure(uintptr_t base_vaddr,
    uint64_t first_seed, uint64_t second_seed)
{
    for (unsigned i = 0; i < AI_MEMBLOCK_FAST_WK_PRESSURE_LINES; ++i) {
        sd(base_vaddr + i * 64U,
            ai_memblock_fast_wk_pressure_pattern(first_seed, i));
    }

    for (unsigned i = 0; i < AI_MEMBLOCK_FAST_WK_PRESSURE_LINES; ++i) {
        sd(base_vaddr + i * 64U,
            ai_memblock_fast_wk_pressure_pattern(second_seed, i));
    }
}

static inline uint64_t ai_memblock_fast_wk_dep_store_chain(
    uintptr_t selector_vaddr,
    uintptr_t payload_vaddr,
    uintptr_t dst_base_vaddr)
{
    uint64_t observed;

    __asm__ volatile(
        "ld t0, 0(%[selector])\n\t"
        "andi t0, t0, 7\n\t"
        "slli t1, t0, 3\n\t"
        "add t2, %[dst_base], t1\n\t"
        "ld t3, 0(%[payload])\n\t"
        "xor t3, t3, t0\n\t"
        "sd t3, 0(t2)\n\t"
        "ld %[observed], 0(t2)\n\t"
        : [observed] "=r" (observed)
        : [selector] "r" (selector_vaddr),
          [payload] "r" (payload_vaddr),
          [dst_base] "r" (dst_base_vaddr)
        : "t0", "t1", "t2", "t3", "memory"
    );

    return observed;
}

bool ai_micro_hs_load_fast_wakeup_dependent_store_addr_data_no_stale_source_corner()
{
    TEST_START();

    const enum test_page source_page = VSRW_GRW;
    const enum test_page dst_page = VSRWX_GRWX;
    const enum test_page pressure_page = SWITCH1;
    const uintptr_t source_vaddr = hs_page_base(source_page);
    const uintptr_t source_paddr = phys_page_base(source_page);
    const uintptr_t dst_vaddr = hs_page_base(dst_page) + 0x200U;
    const uintptr_t dst_paddr = phys_page_base(dst_page) + 0x200U;
    const uintptr_t pressure_vaddr = hs_page_base(pressure_page) + 0x400U;
    const uintptr_t pressure_paddr = phys_page_base(pressure_page) + 0x400U;
    const uintptr_t selector_vaddr = source_vaddr + 0x180U;
    const uintptr_t selector_paddr = source_paddr + 0x180U;
    const uintptr_t payload_vaddr = source_vaddr + 0x188U;
    const uintptr_t payload_paddr = source_paddr + 0x188U;
    const uintptr_t guard_before_vaddr = dst_vaddr - sizeof(uint64_t);
    const uintptr_t guard_after_vaddr =
        dst_vaddr + AI_MEMBLOCK_FAST_WK_DST_SLOTS * sizeof(uint64_t);
    const uintptr_t guard_before_paddr = dst_paddr - sizeof(uint64_t);
    const uintptr_t guard_after_paddr =
        dst_paddr + AI_MEMBLOCK_FAST_WK_DST_SLOTS * sizeof(uint64_t);
    const uint64_t selector_values[] = {
        0x0000000000000006ULL,
        0x0000000000000001ULL,
        0x0000000000000007ULL,
    };
    const uint64_t payload_values[] = {
        0x1122334455667788ULL,
        0xa5a55a5ac3c33c3cULL,
        0x0f1e2d3c4b5a6978ULL,
    };
    const uint64_t slot_seeds[] = {
        0x0101010101010101ULL,
        0x2020202020202020ULL,
        0x4040404040404040ULL,
    };
    const uint64_t guard_before_values[] = {
        0xfeedfacecafebeefULL,
        0xdeadc0def00dbabeULL,
        0x0123456789abcdefULL,
    };
    const uint64_t guard_after_values[] = {
        0x0badf00d5a5aa5a5ULL,
        0x13579bdf2468ace0ULL,
        0xfedcba9876543210ULL,
    };
    const uint64_t pressure_first_seeds[] = {
        0x1111222233334444ULL,
        0x2222333344445555ULL,
        0x3333444455556666ULL,
    };
    const uint64_t pressure_second_seeds[] = {
        0x777788889999aaaaULL,
        0x88889999aaaabbbbULL,
        0x9999aaaabbbbccccULL,
    };

    goto_priv(PRIV_HS);
    hspt_init();
    sfence_vma();

    for (unsigned round = 0;
         round < AI_MEMBLOCK_FAST_WK_ARRAY_SIZE(selector_values);
         ++round) {
        uint64_t expected_slots[AI_MEMBLOCK_FAST_WK_DST_SLOTS];
        uint64_t observed;
        const unsigned selected_slot =
            (unsigned)(selector_values[round] & 0x7ULL);
        const uint64_t expected_payload =
            payload_values[round] ^ (uint64_t)selected_slot;

        for (unsigned i = 0; i < AI_MEMBLOCK_FAST_WK_DST_SLOTS; ++i) {
            expected_slots[i] = slot_seeds[round] +
                (uint64_t)i * 0x1111111111111111ULL;
        }
        expected_slots[selected_slot] = expected_payload;

        goto_priv(PRIV_M);
        write64(selector_paddr, selector_values[round]);
        write64(payload_paddr, payload_values[round]);
        write64(guard_before_paddr, guard_before_values[round]);
        write64(guard_after_paddr, guard_after_values[round]);
        ai_memblock_fast_wk_seed_dst_slots(dst_paddr, expected_slots);
        write64(dst_paddr + selected_slot * sizeof(uint64_t),
            slot_seeds[round] + (uint64_t)selected_slot * 0x1111111111111111ULL);
        write64(pressure_paddr,
            ai_memblock_fast_wk_pressure_pattern(0ULL, 0U));
        write64(pressure_paddr + (AI_MEMBLOCK_FAST_WK_PRESSURE_LINES - 1U) * 64U,
            ai_memblock_fast_wk_pressure_pattern(0ULL,
                AI_MEMBLOCK_FAST_WK_PRESSURE_LINES - 1U));

        goto_priv(PRIV_HS);
        TEST_SETUP_EXCEPT();
        AI_ASSERT_NO_EXCEPT_AND("before the load-fast-wakeup dependent-store probe starts, selector, payload, destination guards, and pressure samples are readable",
            ld(selector_vaddr) == selector_values[round] &&
            ld(payload_vaddr) == payload_values[round] &&
            ld(guard_before_vaddr) == guard_before_values[round] &&
            ld(guard_after_vaddr) == guard_after_values[round]
        );

        TEST_SETUP_EXCEPT();
        ai_memblock_fast_wk_store_pressure(pressure_vaddr,
            pressure_first_seeds[round], pressure_second_seeds[round]);
        observed = ai_memblock_fast_wk_dep_store_chain(selector_vaddr,
            payload_vaddr, dst_vaddr);
        fence_iorw_iorw();
        AI_ASSERT_NO_EXCEPT_AND("load fast wakeup does not let the dependent store use stale selector or payload sources for address/data",
            observed == expected_payload &&
            ld(dst_vaddr + selected_slot * sizeof(uint64_t)) ==
                expected_payload &&
            ld(pressure_vaddr) ==
                ai_memblock_fast_wk_pressure_pattern(
                    pressure_second_seeds[round], 0U) &&
            ld(pressure_vaddr +
                (AI_MEMBLOCK_FAST_WK_PRESSURE_LINES - 1U) * 64U) ==
                ai_memblock_fast_wk_pressure_pattern(
                    pressure_second_seeds[round],
                    AI_MEMBLOCK_FAST_WK_PRESSURE_LINES - 1U)
        );

        TEST_SETUP_EXCEPT();
        bool all_slots_match = true;
        for (unsigned i = 0; i < AI_MEMBLOCK_FAST_WK_DST_SLOTS; ++i) {
            all_slots_match = all_slots_match &&
                (ld(dst_vaddr + i * sizeof(uint64_t)) == expected_slots[i]);
        }
        AI_ASSERT_NO_EXCEPT_AND("only the slot selected by the current load result is updated, proving the dependent store did not use an old fast-wakeup source",
            all_slots_match &&
            ld(guard_before_vaddr) == guard_before_values[round] &&
            ld(guard_after_vaddr) == guard_after_values[round]
        );

        goto_priv(PRIV_M);
        TEST_ASSERT("backing memory preserves the exact load-selected slot update, unchanged destination guards, and latest store-pressure samples",
            ai_memblock_fast_wk_dst_slots_match(dst_paddr, expected_slots) &&
            read64(guard_before_paddr) == guard_before_values[round] &&
            read64(guard_after_paddr) == guard_after_values[round] &&
            read64(pressure_paddr) ==
                ai_memblock_fast_wk_pressure_pattern(
                    pressure_second_seeds[round], 0U) &&
            read64(pressure_paddr +
                (AI_MEMBLOCK_FAST_WK_PRESSURE_LINES - 1U) * 64U) ==
                ai_memblock_fast_wk_pressure_pattern(
                    pressure_second_seeds[round],
                    AI_MEMBLOCK_FAST_WK_PRESSURE_LINES - 1U)
        );
    }

    TEST_END();
}
