#include <rvh_test.h>
#include <page_tables.h>
#include <instructions.h>
#include <stdint.h>

#define AI_ASSERT_NO_EXCEPT_AND(message, cond) \
    TEST_ASSERT(message, excpt.triggered == false && (cond))
#define AI_MEMBLOCK_STD_STREAM_SLOTS 16U

static inline uint64_t ai_memblock_std_stream_pattern(uint64_t seed,
    unsigned round, unsigned slot, unsigned phase)
{
    return seed ^
        (0x9e3779b97f4a7c15ULL * (uint64_t)(slot + 1U)) ^
        ((uint64_t)(round + 3U) << 48) ^
        ((uint64_t)(phase + 5U) << 32) ^
        ((uint64_t)slot << 7);
}

static inline void ai_memblock_std_seed_slots(uintptr_t paddr, uint64_t seed)
{
    for (unsigned i = 0; i < AI_MEMBLOCK_STD_STREAM_SLOTS; ++i) {
        write64(paddr + i * sizeof(uint64_t), seed + i);
    }
}

static inline bool ai_memblock_std_slots_match(uintptr_t paddr,
    const uint64_t *expected)
{
    for (unsigned i = 0; i < AI_MEMBLOCK_STD_STREAM_SLOTS; ++i) {
        if (read64(paddr + i * sizeof(uint64_t)) != expected[i]) {
            return false;
        }
    }

    return true;
}

bool ai_micro_hs_storedata_writeback_stream_interleaved_addr_no_std_ready_drop_corner()
{
    TEST_START();

    const enum test_page stream_page = VSRWX_GRWX;
    const enum test_page marker_page = VSRW_GRW;
    const uintptr_t stream_vaddr = hs_page_base(stream_page) + 0x200U;
    const uintptr_t stream_paddr = phys_page_base(stream_page) + 0x200U;
    const uintptr_t guard_before_vaddr = stream_vaddr - sizeof(uint64_t);
    const uintptr_t guard_after_vaddr =
        stream_vaddr + AI_MEMBLOCK_STD_STREAM_SLOTS * sizeof(uint64_t);
    const uintptr_t guard_before_paddr = stream_paddr - sizeof(uint64_t);
    const uintptr_t guard_after_paddr =
        stream_paddr + AI_MEMBLOCK_STD_STREAM_SLOTS * sizeof(uint64_t);
    const uintptr_t marker_vaddr = hs_page_base(marker_page) + 0x300U;
    const uintptr_t marker_paddr = phys_page_base(marker_page) + 0x300U;
    const uintptr_t marker_guard_paddr = marker_paddr + sizeof(uint64_t);
    const uint64_t stream_seeds[] = {
        0x1021324354657687ULL,
        0x89abcdef01234567ULL,
        0x55aa33cc77ee11ddULL,
    };
    const uint64_t guard_before_values[] = {
        0xfeedfacecafebeefULL,
        0xdeadc0def00dbabeULL,
        0xfaceb00c13579bdfULL,
    };
    const uint64_t guard_after_values[] = {
        0x0badf00d5a5aa5a5ULL,
        0x13579bdf2468ace0ULL,
        0x02468ace13579bdfULL,
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
        0x3141592653589793ULL,
        0x2718281828459045ULL,
        0x1123581321345589ULL,
    };

    goto_priv(PRIV_HS);
    hspt_init();
    sfence_vma();

    for (unsigned round = 0; round < 3U; ++round) {
        uint64_t expected[AI_MEMBLOCK_STD_STREAM_SLOTS];
        bool observed_all_match = true;

        goto_priv(PRIV_M);
        ai_memblock_std_seed_slots(stream_paddr, stream_seeds[round]);
        write64(guard_before_paddr, guard_before_values[round]);
        write64(guard_after_paddr, guard_after_values[round]);
        write64(marker_paddr, marker_seeds[round]);
        write64(marker_guard_paddr, marker_guards[round]);

        goto_priv(PRIV_HS);
        TEST_SETUP_EXCEPT();
        AI_ASSERT_NO_EXCEPT_AND("before the MemBlock std writeback stream probe starts, stream guards and marker are readable with seeded values",
            ld(guard_before_vaddr) == guard_before_values[round] &&
            ld(guard_after_vaddr) == guard_after_values[round] &&
            ld(marker_vaddr) == marker_seeds[round]
        );

        TEST_SETUP_EXCEPT();
        for (unsigned i = 0; i < AI_MEMBLOCK_STD_STREAM_SLOTS; i += 2U) {
            const uint64_t value =
                ai_memblock_std_stream_pattern(0x1111222233334444ULL,
                    round, i, 0U);
            sd(stream_vaddr + i * sizeof(uint64_t), value);
            expected[i] = value;
        }
        for (unsigned i = 1; i < AI_MEMBLOCK_STD_STREAM_SLOTS; i += 2U) {
            const uint64_t value =
                ai_memblock_std_stream_pattern(0x2222333344445555ULL,
                    round, i, 1U);
            sd(stream_vaddr + i * sizeof(uint64_t), value);
            expected[i] = value;
        }
        for (int i = (int)AI_MEMBLOCK_STD_STREAM_SLOTS - 1; i >= 0; --i) {
            const uint64_t value =
                ai_memblock_std_stream_pattern(0x3333444455556666ULL,
                    round, (unsigned)i, 2U);
            sd(stream_vaddr + (uintptr_t)i * sizeof(uint64_t), value);
            expected[i] = value;
        }
        for (unsigned i = 0; i < AI_MEMBLOCK_STD_STREAM_SLOTS; i += 3U) {
            const uint64_t value =
                ai_memblock_std_stream_pattern(0x4444555566667777ULL,
                    round, i, 3U);
            sd(stream_vaddr + i * sizeof(uint64_t), value);
            expected[i] = value;
        }
        sd(marker_vaddr, marker_updates[round]);
        fence_iorw_iorw();

        for (unsigned i = 0; i < AI_MEMBLOCK_STD_STREAM_SLOTS; ++i) {
            observed_all_match = observed_all_match &&
                (ld(stream_vaddr + i * sizeof(uint64_t)) == expected[i]);
        }
        AI_ASSERT_NO_EXCEPT_AND("dense scalar store-data writeback stream with interleaved address order keeps every slot's latest payload and does not drop or duplicate a std completion",
            observed_all_match &&
            ld(marker_vaddr) == marker_updates[round]
        );

        TEST_SETUP_EXCEPT();
        AI_ASSERT_NO_EXCEPT_AND("the std writeback stream probe preserves adjacent guards after dense store-data pressure",
            ld(guard_before_vaddr) == guard_before_values[round] &&
            ld(guard_after_vaddr) == guard_after_values[round]
        );

        goto_priv(PRIV_M);
        TEST_ASSERT("backing memory after the std writeback stream probe contains each latest store payload, marker update, and unchanged guards",
            ai_memblock_std_slots_match(stream_paddr, expected) &&
            read64(guard_before_paddr) == guard_before_values[round] &&
            read64(guard_after_paddr) == guard_after_values[round] &&
            read64(marker_paddr) == marker_updates[round] &&
            read64(marker_guard_paddr) == marker_guards[round]
        );
    }

    TEST_END();
}
