#include <rvh_test.h>
#include <page_tables.h>
#include <stdint.h>

#define AI_ASSERT_NO_EXCEPT_AND(message, cond) \
    TEST_ASSERT(message, excpt.triggered == false && (cond))

#define AI_MICRO_HS_TLB_HIT_PAGE VSRWX_GRWX
#define AI_MICRO_HS_TLB_HIT_LOAD_OFF  0x18U
#define AI_MICRO_HS_TLB_HIT_STORE_OFF 0x20U
#define AI_MICRO_HS_TLB_MISS_LOAD_OFF 0x28U
#define AI_MICRO_HS_TLB_MISS_STORE_OFF 0x30U
#define AI_MICRO_HS_TLB_RELOAD_LOAD_OFF 0x38U
#define AI_MICRO_HS_TLB_REPEAT_LOAD_OFF 0x40U
#define AI_MICRO_HS_TLB_REPEAT_STORE_OFF 0x48U
#define AI_MICRO_HS_TLB_STORE_RELOAD_OFF 0x58U

static inline void ai_micro_hs_tlb_reinit_hs_translation(void)
{
    goto_priv(PRIV_HS);
    hspt_init();
    sfence_vma();
}

bool ai_micro_hs_load_tlb_hit_data_visible()
{
    TEST_START();

    const uintptr_t warm_vaddr = hs_page_base(AI_MICRO_HS_TLB_HIT_PAGE);
    const uintptr_t warm_paddr = phys_page_base(AI_MICRO_HS_TLB_HIT_PAGE);
    const uintptr_t vaddr = hs_page_base(AI_MICRO_HS_TLB_HIT_PAGE) +
        AI_MICRO_HS_TLB_HIT_LOAD_OFF;
    const uintptr_t paddr = phys_page_base(AI_MICRO_HS_TLB_HIT_PAGE) +
        AI_MICRO_HS_TLB_HIT_LOAD_OFF;
    const uint64_t warm_expected = 0xdeadbeefcafef00dULL;
    const uint64_t expected = 0x1122334455667788ULL;

    goto_priv(PRIV_M);
    write64(warm_paddr, warm_expected);
    write64(paddr, expected);

    ai_micro_hs_tlb_reinit_hs_translation();

    TEST_SETUP_EXCEPT();
    uint64_t warm = ld(warm_vaddr);
    AI_ASSERT_NO_EXCEPT_AND("an aligned HS cacheable load first warms the page translation before the TLB-hit probe",
        warm == warm_expected
    );

    TEST_SETUP_EXCEPT();
    uint64_t observed = ld(vaddr);
    AI_ASSERT_NO_EXCEPT_AND("an aligned HS cacheable load on a translated TLB-hit path completes without surfacing a vaddr-misaligned exception",
        observed == expected
    );

    TEST_END();
}

bool ai_micro_hs_store_tlb_hit_data_visible()
{
    TEST_START();

    const uintptr_t warm_vaddr = hs_page_base(AI_MICRO_HS_TLB_HIT_PAGE);
    const uintptr_t warm_paddr = phys_page_base(AI_MICRO_HS_TLB_HIT_PAGE);
    const uintptr_t vaddr = hs_page_base(AI_MICRO_HS_TLB_HIT_PAGE) +
        AI_MICRO_HS_TLB_HIT_STORE_OFF;
    const uintptr_t paddr = phys_page_base(AI_MICRO_HS_TLB_HIT_PAGE) +
        AI_MICRO_HS_TLB_HIT_STORE_OFF;
    const uint64_t warm_expected = 0x13579bdf2468ace0ULL;
    const uint64_t initial_value = 0x0102030405060708ULL;
    const uint64_t stored_value = 0x8877665544332211ULL;

    goto_priv(PRIV_M);
    write64(warm_paddr, warm_expected);
    write64(paddr, initial_value);

    ai_micro_hs_tlb_reinit_hs_translation();

    TEST_SETUP_EXCEPT();
    uint64_t warm = ld(warm_vaddr);
    AI_ASSERT_NO_EXCEPT_AND("an aligned HS cacheable store first warms the page translation before the TLB-hit probe",
        warm == warm_expected
    );

    TEST_SETUP_EXCEPT();
    sd(vaddr, stored_value);
    AI_ASSERT_NO_EXCEPT_AND("an aligned HS cacheable store on a translated TLB-hit path completes without surfacing a vaddr-misaligned exception",
        true
    );

    TEST_SETUP_EXCEPT();
    uint64_t observed = ld(vaddr);
    AI_ASSERT_NO_EXCEPT_AND("the same HS cacheable alias reads back the payload written by the TLB-hit store path",
        observed == stored_value
    );

    goto_priv(PRIV_M);
    TEST_ASSERT("the HS cacheable TLB-hit store path updates backing memory with the expected payload",
        read64(paddr) == stored_value
    );

    TEST_END();
}

bool ai_micro_hs_load_tlb_miss_replay_data_visible()
{
    TEST_START();

    const uintptr_t vaddr = hs_page_base(AI_MICRO_HS_TLB_HIT_PAGE) +
        AI_MICRO_HS_TLB_MISS_LOAD_OFF;
    const uintptr_t paddr = phys_page_base(AI_MICRO_HS_TLB_HIT_PAGE) +
        AI_MICRO_HS_TLB_MISS_LOAD_OFF;
    const uint64_t expected = 0xa1b2c3d4e5f60718ULL;

    goto_priv(PRIV_M);
    write64(paddr, expected);

    ai_micro_hs_tlb_reinit_hs_translation();

    TEST_SETUP_EXCEPT();
    uint64_t observed = ld(vaddr);
    AI_ASSERT_NO_EXCEPT_AND("a first aligned HS cacheable load on a cold translation path still returns the expected payload after the TLB-miss replay path completes",
        observed == expected
    );

    TEST_END();
}

bool ai_micro_hs_load_tlb_miss_then_hit_stable_corner()
{
    TEST_START();

    const uintptr_t vaddr = hs_page_base(AI_MICRO_HS_TLB_HIT_PAGE) +
        AI_MICRO_HS_TLB_MISS_LOAD_OFF;
    const uintptr_t paddr = phys_page_base(AI_MICRO_HS_TLB_HIT_PAGE) +
        AI_MICRO_HS_TLB_MISS_LOAD_OFF;
    const uint64_t expected = 0x3141592653589793ULL;

    goto_priv(PRIV_M);
    write64(paddr, expected);

    ai_micro_hs_tlb_reinit_hs_translation();

    TEST_SETUP_EXCEPT();
    uint64_t first = ld(vaddr);
    AI_ASSERT_NO_EXCEPT_AND("the first HS load to a cold translation path completes with the expected payload before the immediate re-access corner",
        first == expected
    );

    TEST_SETUP_EXCEPT();
    uint64_t second = ld(vaddr);
    AI_ASSERT_NO_EXCEPT_AND("re-accessing the same HS alias immediately after the TLB-miss replay path remains stable and returns the same payload",
        second == expected
    );

    TEST_END();
}

bool ai_micro_hs_load_tlb_reinit_then_cold_miss_stable_corner()
{
    TEST_START();

    const uintptr_t vaddr = hs_page_base(AI_MICRO_HS_TLB_HIT_PAGE) +
        AI_MICRO_HS_TLB_RELOAD_LOAD_OFF;
    const uintptr_t paddr = phys_page_base(AI_MICRO_HS_TLB_HIT_PAGE) +
        AI_MICRO_HS_TLB_RELOAD_LOAD_OFF;
    const uint64_t expected = 0x2718281828459045ULL;

    goto_priv(PRIV_M);
    write64(paddr, expected);

    ai_micro_hs_tlb_reinit_hs_translation();

    TEST_SETUP_EXCEPT();
    uint64_t first = ld(vaddr);
    AI_ASSERT_NO_EXCEPT_AND("the first HS load after rebuilding the translation context still completes through the cold TLB-miss replay path and returns the expected payload",
        first == expected
    );

    ai_micro_hs_tlb_reinit_hs_translation();

    TEST_SETUP_EXCEPT();
    uint64_t second = ld(vaddr);
    AI_ASSERT_NO_EXCEPT_AND("rebuilding the HS translation context again before reloading the same address still leaves the repeated cold-load replay result stable",
        second == expected
    );

    goto_priv(PRIV_M);
    TEST_ASSERT("repeated cold-load replay across HS translation rebuilds leaves backing memory unchanged",
        read64(paddr) == expected
    );

    TEST_END();
}

bool ai_micro_hs_load_repeated_tlb_miss_no_side_effects_corner()
{
    TEST_START();

    const uintptr_t vaddr = hs_page_base(AI_MICRO_HS_TLB_HIT_PAGE) +
        AI_MICRO_HS_TLB_REPEAT_LOAD_OFF;
    const uintptr_t paddr = phys_page_base(AI_MICRO_HS_TLB_HIT_PAGE) +
        AI_MICRO_HS_TLB_REPEAT_LOAD_OFF;
    const uintptr_t adjacent_paddr = paddr + sizeof(uint64_t);
    const uint64_t expected = 0x3141592653589793ULL;
    const uint64_t adjacent = 0x13579bdf2468ace0ULL;

    goto_priv(PRIV_M);
    write64(paddr, expected);
    write64(adjacent_paddr, adjacent);

    ai_micro_hs_tlb_reinit_hs_translation();

    TEST_SETUP_EXCEPT();
    uint64_t first = ld(vaddr);
    AI_ASSERT_NO_EXCEPT_AND("a first HS cold-load replay on the target line returns the seeded payload before the no-side-effect check",
        first == expected
    );

    ai_micro_hs_tlb_reinit_hs_translation();

    TEST_SETUP_EXCEPT();
    uint64_t second = ld(vaddr);
    AI_ASSERT_NO_EXCEPT_AND("repeating the HS cold-load replay after rebuilding translation still returns the same target payload",
        second == expected
    );

    goto_priv(PRIV_M);
    TEST_ASSERT("repeated HS cold-load replay does not modify either the target word or its adjacent backing-memory word",
        read64(paddr) == expected &&
        read64(adjacent_paddr) == adjacent
    );

    TEST_END();
}

bool ai_micro_hs_store_tlb_miss_replay_data_visible()
{
    TEST_START();

    const uintptr_t vaddr = hs_page_base(AI_MICRO_HS_TLB_HIT_PAGE) +
        AI_MICRO_HS_TLB_MISS_STORE_OFF;
    const uintptr_t paddr = phys_page_base(AI_MICRO_HS_TLB_HIT_PAGE) +
        AI_MICRO_HS_TLB_MISS_STORE_OFF;
    const uint64_t initial_value = 0x0f1e2d3c4b5a6978ULL;
    const uint64_t stored_value = 0x8877665544332210ULL;

    goto_priv(PRIV_M);
    write64(paddr, initial_value);

    ai_micro_hs_tlb_reinit_hs_translation();

    TEST_SETUP_EXCEPT();
    sd(vaddr, stored_value);
    AI_ASSERT_NO_EXCEPT_AND("a first aligned HS cacheable store on a cold translation path completes after the TLB-miss replay path and does not surface a vaddr-misaligned exception",
        true
    );

    TEST_SETUP_EXCEPT();
    uint64_t observed = ld(vaddr);
    AI_ASSERT_NO_EXCEPT_AND("the HS alias reads back the payload written by the cold-translation store after the TLB-miss replay path completes",
        observed == stored_value
    );

    goto_priv(PRIV_M);
    TEST_ASSERT("the cold-translation HS store also updates backing memory with the expected payload",
        read64(paddr) == stored_value
    );

    TEST_END();
}

bool ai_micro_hs_store_tlb_miss_adjacent_word_preserved_corner()
{
    TEST_START();

    const uintptr_t vaddr = hs_page_base(AI_MICRO_HS_TLB_HIT_PAGE) +
        AI_MICRO_HS_TLB_REPEAT_STORE_OFF;
    const uintptr_t paddr = phys_page_base(AI_MICRO_HS_TLB_HIT_PAGE) +
        AI_MICRO_HS_TLB_REPEAT_STORE_OFF;
    const uintptr_t adjacent_paddr = paddr + sizeof(uint64_t);
    const uint64_t initial_value = 0x0f1e2d3c4b5a6978ULL;
    const uint64_t stored_value = 0x8877665544332211ULL;
    const uint64_t adjacent = 0x123456789abcdef0ULL;

    goto_priv(PRIV_M);
    write64(paddr, initial_value);
    write64(adjacent_paddr, adjacent);

    ai_micro_hs_tlb_reinit_hs_translation();

    TEST_SETUP_EXCEPT();
    sd(vaddr, stored_value);
    AI_ASSERT_NO_EXCEPT_AND("a first HS cold-translation store updates the target line without surfacing an exception before the adjacent-word check",
        true
    );

    TEST_SETUP_EXCEPT();
    uint64_t observed = ld(vaddr);
    AI_ASSERT_NO_EXCEPT_AND("reloading the same HS alias after the cold-translation store returns the stored payload",
        observed == stored_value
    );

    goto_priv(PRIV_M);
    TEST_ASSERT("the HS cold-translation store updates only the target word while preserving the adjacent backing-memory word",
        read64(paddr) == stored_value &&
        read64(adjacent_paddr) == adjacent
    );

    TEST_END();
}

bool ai_micro_hs_store_tlb_miss_then_hit_stable_corner()
{
    TEST_START();

    const uintptr_t vaddr = hs_page_base(AI_MICRO_HS_TLB_HIT_PAGE) +
        AI_MICRO_HS_TLB_MISS_STORE_OFF;
    const uintptr_t paddr = phys_page_base(AI_MICRO_HS_TLB_HIT_PAGE) +
        AI_MICRO_HS_TLB_MISS_STORE_OFF;
    const uint64_t first_store = 0x123456789abcdef0ULL;
    const uint64_t second_store = 0x0fedcba987654321ULL;

    goto_priv(PRIV_M);
    write64(paddr, 0ULL);

    ai_micro_hs_tlb_reinit_hs_translation();

    TEST_SETUP_EXCEPT();
    sd(vaddr, first_store);
    AI_ASSERT_NO_EXCEPT_AND("the first HS store on a cold translation path completes successfully before the immediate re-store corner",
        true
    );

    TEST_SETUP_EXCEPT();
    sd(vaddr, second_store);
    AI_ASSERT_NO_EXCEPT_AND("re-storing to the same HS alias immediately after the cold-translation store remains stable and completes successfully",
        true
    );

    TEST_SETUP_EXCEPT();
    uint64_t observed = ld(vaddr);
    AI_ASSERT_NO_EXCEPT_AND("after a cold-translation store followed by an immediate re-store, the HS alias returns the latest payload",
        observed == second_store
    );

    goto_priv(PRIV_M);
    TEST_ASSERT("after the cold-translation store and immediate re-store sequence, backing memory also keeps the latest payload",
        read64(paddr) == second_store
    );

    TEST_END();
}

bool ai_micro_hs_store_tlb_miss_then_reinit_load_latest_corner()
{
    TEST_START();

    const uintptr_t vaddr = hs_page_base(AI_MICRO_HS_TLB_HIT_PAGE) +
        AI_MICRO_HS_TLB_STORE_RELOAD_OFF;
    const uintptr_t paddr = phys_page_base(AI_MICRO_HS_TLB_HIT_PAGE) +
        AI_MICRO_HS_TLB_STORE_RELOAD_OFF;
    const uint64_t initial_value = 0x0102030405060708ULL;
    const uint64_t stored_value = 0xfedcba9876543210ULL;

    goto_priv(PRIV_M);
    write64(paddr, initial_value);

    ai_micro_hs_tlb_reinit_hs_translation();

    TEST_SETUP_EXCEPT();
    sd(vaddr, stored_value);
    AI_ASSERT_NO_EXCEPT_AND("the first HS cold-translation store completes successfully before rebuilding translation for the follow-up load",
        true
    );

    ai_micro_hs_tlb_reinit_hs_translation();

    TEST_SETUP_EXCEPT();
    uint64_t observed = ld(vaddr);
    AI_ASSERT_NO_EXCEPT_AND("after rebuilding the HS translation context, a cold reload of the same address still returns the latest payload written by the earlier cold store",
        observed == stored_value
    );

    goto_priv(PRIV_M);
    TEST_ASSERT("the HS cold-store then rebuilt-cold-load sequence leaves backing memory updated with the stored payload",
        read64(paddr) == stored_value
    );

    TEST_END();
}
