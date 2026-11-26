#include "ai_micro_memblock_lrsc_trigger_common.h"

bool ai_micro_memblock_amomin_w_lower_word_load_trigger_breakpoint_suppresses_signed_min_lane_then_clear_succeeds_corner()
{
    TEST_START();

    const enum test_page page = VSRWX_GRWX;
    const uintptr_t base_vaddr = hs_page_base(page) + 0x900U;
    const uintptr_t base_paddr = phys_page_base(page) + 0x900U;
    const uintptr_t guard_vaddr = base_vaddr;
    const uintptr_t target_vaddr = base_vaddr + sizeof(uint64_t);
    const uintptr_t lower_word_vaddr = target_vaddr;
    const uintptr_t adjacent_vaddr = base_vaddr + 2U * sizeof(uint64_t);
    const uintptr_t marker_vaddr = base_vaddr + 3U * sizeof(uint64_t);
    const uintptr_t guard_paddr = base_paddr;
    const uintptr_t target_paddr = base_paddr + sizeof(uint64_t);
    const uintptr_t lower_word_paddr = target_paddr;
    const uintptr_t adjacent_paddr = base_paddr + 2U * sizeof(uint64_t);
    const uintptr_t marker_paddr = base_paddr + 3U * sizeof(uint64_t);
    const uint64_t guard_seed = 0x6c6f776d696e6770ULL;
    const uint32_t lower_initial = 0x80000010U;
    const uint32_t upper_initial = 0x2468ace0U;
    const uint64_t initial_value =
        ((uint64_t)upper_initial << 32) | lower_initial;
    const uint64_t adjacent_seed = 0x5566778899aabbccULL;
    const uint64_t marker_seed = 0xabcdef0123456789ULL;
    const uint64_t marker_final = 0x9876543210fedcbaULL;
    const uint32_t blocked_operand = 0x80000004U;
    const uint32_t final_operand = 0x80000000U;
    const uint32_t expected_lower = final_operand;
    const uint64_t expected_final =
        ((uint64_t)upper_initial << 32) | expected_lower;

    goto_priv(PRIV_M);
    write64(guard_paddr, guard_seed);
    write64(target_paddr, initial_value);
    write64(adjacent_paddr, adjacent_seed);
    write64(marker_paddr, marker_seed);

    goto_priv(PRIV_HS);
    hspt_init();
    sfence_vma();

    ai_memblock_lrsc_trigger_clear_mcontrol6_0();
    ai_memblock_lrsc_trigger_arm_mcontrol6_0(lower_word_vaddr,
        AI_MEMBLOCK_LRSC_TRIGGER_LOAD);
    uint64_t saved_mstatus =
        ai_memblock_lrsc_trigger_enable_breakpoint_delivery();

    goto_priv(PRIV_HS);
    TEST_SETUP_EXCEPT();
    (void)amomin_w(lower_word_vaddr, blocked_operand);
    TEST_ASSERT("a load-only mcontrol6 address trigger armed on the lower word reports breakpoint on amomin.w before the signed-min lane RMW can commit",
        excpt.triggered == true &&
        excpt.cause == CAUSE_BKP
    );

    ai_memblock_lrsc_trigger_restore_machine_status(saved_mstatus);
    ai_memblock_lrsc_trigger_clear_mcontrol6_0();

    goto_priv(PRIV_M);
    bool trigger_preserved_words =
        read64(guard_paddr) == guard_seed &&
        read64(target_paddr) == initial_value &&
        read32(lower_word_paddr) == lower_initial &&
        read32(target_paddr + sizeof(uint32_t)) == upper_initial &&
        read64(adjacent_paddr) == adjacent_seed &&
        read64(marker_paddr) == marker_seed;
    TEST_ASSERT("the load-triggered lower-word amomin.w breakpoint leaves the full doubleword, guard, adjacent, and marker words unchanged",
        trigger_preserved_words
    );

    goto_priv(PRIV_HS);
    TEST_SETUP_EXCEPT();
    uint32_t old_lower = amomin_w(lower_word_vaddr, final_operand);
    TEST_ASSERT("after clearing the load-only trigger, amomin.w returns the old lower word and signed-min updates only that 32-bit lane",
        excpt.triggered == false &&
        old_lower == lower_initial &&
        read64(target_vaddr) == expected_final &&
        read32(lower_word_vaddr) == expected_lower &&
        read32(lower_word_vaddr + sizeof(uint32_t)) == upper_initial
    );

    write64(marker_vaddr, marker_final);
    TEST_ASSERT("post-clear marker progress and non-target sentinels prove the amomin.w lower-word load-trigger breakpoint path did not poison following memory operations",
        read64(guard_vaddr) == guard_seed &&
        read64(adjacent_vaddr) == adjacent_seed &&
        read64(marker_vaddr) == marker_final
    );

    goto_priv(PRIV_M);
    TEST_ASSERT("backing memory shows only the post-clear lower-word amomin.w signed-min lane update plus marker progress, with upper lane, guard, and adjacent unchanged",
        read64(guard_paddr) == guard_seed &&
        read64(target_paddr) == expected_final &&
        read32(lower_word_paddr) == expected_lower &&
        read32(lower_word_paddr + sizeof(uint32_t)) == upper_initial &&
        read64(adjacent_paddr) == adjacent_seed &&
        read64(marker_paddr) == marker_final
    );

    TEST_END();
}

bool ai_micro_memblock_amomin_w_lower_word_store_trigger_breakpoint_suppresses_signed_min_lane_then_clear_succeeds_corner()
{
    TEST_START();

    const enum test_page page = VSRWX_GRWX;
    const uintptr_t base_vaddr = hs_page_base(page) + 0x940U;
    const uintptr_t base_paddr = phys_page_base(page) + 0x940U;
    const uintptr_t guard_vaddr = base_vaddr;
    const uintptr_t target_vaddr = base_vaddr + sizeof(uint64_t);
    const uintptr_t lower_word_vaddr = target_vaddr;
    const uintptr_t adjacent_vaddr = base_vaddr + 2U * sizeof(uint64_t);
    const uintptr_t marker_vaddr = base_vaddr + 3U * sizeof(uint64_t);
    const uintptr_t guard_paddr = base_paddr;
    const uintptr_t target_paddr = base_paddr + sizeof(uint64_t);
    const uintptr_t lower_word_paddr = target_paddr;
    const uintptr_t adjacent_paddr = base_paddr + 2U * sizeof(uint64_t);
    const uintptr_t marker_paddr = base_paddr + 3U * sizeof(uint64_t);
    const uint64_t guard_seed = 0x73746d696e6c6f77ULL;
    const uint32_t lower_initial = 0x80000018U;
    const uint32_t upper_initial = 0x0badcafeU;
    const uint64_t initial_value =
        ((uint64_t)upper_initial << 32) | lower_initial;
    const uint64_t adjacent_seed = 0x0123456789abcdefULL;
    const uint64_t marker_seed = 0xfedcba9876543210ULL;
    const uint64_t marker_final = 0x1020304050607080ULL;
    const uint32_t blocked_operand = 0x80000008U;
    const uint32_t final_operand = 0x80000000U;
    const uint32_t expected_lower = final_operand;
    const uint64_t expected_final =
        ((uint64_t)upper_initial << 32) | expected_lower;

    goto_priv(PRIV_M);
    write64(guard_paddr, guard_seed);
    write64(target_paddr, initial_value);
    write64(adjacent_paddr, adjacent_seed);
    write64(marker_paddr, marker_seed);

    goto_priv(PRIV_HS);
    hspt_init();
    sfence_vma();

    ai_memblock_lrsc_trigger_clear_mcontrol6_0();
    ai_memblock_lrsc_trigger_arm_mcontrol6_0(lower_word_vaddr,
        AI_MEMBLOCK_LRSC_TRIGGER_STORE);
    uint64_t saved_mstatus =
        ai_memblock_lrsc_trigger_enable_breakpoint_delivery();

    goto_priv(PRIV_HS);
    TEST_SETUP_EXCEPT();
    (void)amomin_w(lower_word_vaddr, blocked_operand);
    TEST_ASSERT("a store-only mcontrol6 address trigger armed on the lower word reports breakpoint on amomin.w before the signed-min lane write can commit",
        excpt.triggered == true &&
        excpt.cause == CAUSE_BKP
    );

    ai_memblock_lrsc_trigger_restore_machine_status(saved_mstatus);
    ai_memblock_lrsc_trigger_clear_mcontrol6_0();

    goto_priv(PRIV_M);
    bool trigger_preserved_words =
        read64(guard_paddr) == guard_seed &&
        read64(target_paddr) == initial_value &&
        read32(lower_word_paddr) == lower_initial &&
        read32(lower_word_paddr + sizeof(uint32_t)) == upper_initial &&
        read64(adjacent_paddr) == adjacent_seed &&
        read64(marker_paddr) == marker_seed;
    TEST_ASSERT("the store-triggered lower-word amomin.w breakpoint leaves the full doubleword, guard, adjacent, and marker words unchanged",
        trigger_preserved_words
    );

    goto_priv(PRIV_HS);
    TEST_SETUP_EXCEPT();
    uint32_t old_lower = amomin_w(lower_word_vaddr, final_operand);
    TEST_ASSERT("after clearing the store-only trigger, amomin.w returns the old lower word and signed-min updates only that 32-bit lane",
        excpt.triggered == false &&
        old_lower == lower_initial &&
        read64(target_vaddr) == expected_final &&
        read32(lower_word_vaddr) == expected_lower &&
        read32(lower_word_vaddr + sizeof(uint32_t)) == upper_initial
    );

    write64(marker_vaddr, marker_final);
    TEST_ASSERT("post-clear marker progress and non-target sentinels prove the amomin.w lower-word store-trigger breakpoint path did not poison following memory operations",
        read64(guard_vaddr) == guard_seed &&
        read64(adjacent_vaddr) == adjacent_seed &&
        read64(marker_vaddr) == marker_final
    );

    goto_priv(PRIV_M);
    TEST_ASSERT("backing memory shows only the post-clear lower-word amomin.w signed-min lane update plus marker progress, with upper lane, guard, and adjacent unchanged",
        read64(guard_paddr) == guard_seed &&
        read64(target_paddr) == expected_final &&
        read32(lower_word_paddr) == expected_lower &&
        read32(lower_word_paddr + sizeof(uint32_t)) == upper_initial &&
        read64(adjacent_paddr) == adjacent_seed &&
        read64(marker_paddr) == marker_final
    );

    TEST_END();
}

bool ai_micro_memblock_amominu_w_upper_word_store_trigger_breakpoint_suppresses_unsigned_min_lane_then_clear_succeeds_corner()
{
    TEST_START();

    const enum test_page page = VSRWX_GRWX;
    const uintptr_t base_vaddr = hs_page_base(page) + 0x980U;
    const uintptr_t base_paddr = phys_page_base(page) + 0x980U;
    const uintptr_t guard_vaddr = base_vaddr;
    const uintptr_t target_vaddr = base_vaddr + sizeof(uint64_t);
    const uintptr_t upper_word_vaddr = target_vaddr + sizeof(uint32_t);
    const uintptr_t adjacent_vaddr = base_vaddr + 2U * sizeof(uint64_t);
    const uintptr_t marker_vaddr = base_vaddr + 3U * sizeof(uint64_t);
    const uintptr_t guard_paddr = base_paddr;
    const uintptr_t target_paddr = base_paddr + sizeof(uint64_t);
    const uintptr_t upper_word_paddr = target_paddr + sizeof(uint32_t);
    const uintptr_t adjacent_paddr = base_paddr + 2U * sizeof(uint64_t);
    const uintptr_t marker_paddr = base_paddr + 3U * sizeof(uint64_t);
    const uint64_t guard_seed = 0x756d696e75707072ULL;
    const uint32_t lower_initial = 0x55aa33ccU;
    const uint32_t upper_initial = 0xffffffffU;
    const uint64_t initial_value =
        ((uint64_t)upper_initial << 32) | lower_initial;
    const uint64_t adjacent_seed = 0x1122334455667788ULL;
    const uint64_t marker_seed = 0x8877665544332211ULL;
    const uint64_t marker_final = 0x0f1e2d3c4b5a6978ULL;
    const uint32_t blocked_operand = 0x00000010U;
    const uint32_t final_operand = 0x00000000U;
    const uint32_t expected_upper = final_operand;
    const uint64_t expected_final =
        ((uint64_t)expected_upper << 32) | lower_initial;

    goto_priv(PRIV_M);
    write64(guard_paddr, guard_seed);
    write64(target_paddr, initial_value);
    write64(adjacent_paddr, adjacent_seed);
    write64(marker_paddr, marker_seed);

    goto_priv(PRIV_HS);
    hspt_init();
    sfence_vma();

    ai_memblock_lrsc_trigger_clear_mcontrol6_0();
    ai_memblock_lrsc_trigger_arm_mcontrol6_0(upper_word_vaddr,
        AI_MEMBLOCK_LRSC_TRIGGER_STORE);
    uint64_t saved_mstatus =
        ai_memblock_lrsc_trigger_enable_breakpoint_delivery();

    goto_priv(PRIV_HS);
    TEST_SETUP_EXCEPT();
    (void)amominu_w(upper_word_vaddr, blocked_operand);
    TEST_ASSERT("a store-only mcontrol6 address trigger armed on the upper word reports breakpoint on amominu.w before the unsigned-min lane write can commit",
        excpt.triggered == true &&
        excpt.cause == CAUSE_BKP
    );

    ai_memblock_lrsc_trigger_restore_machine_status(saved_mstatus);
    ai_memblock_lrsc_trigger_clear_mcontrol6_0();

    goto_priv(PRIV_M);
    bool trigger_preserved_words =
        read64(guard_paddr) == guard_seed &&
        read64(target_paddr) == initial_value &&
        read32(target_paddr) == lower_initial &&
        read32(upper_word_paddr) == upper_initial &&
        read64(adjacent_paddr) == adjacent_seed &&
        read64(marker_paddr) == marker_seed;
    TEST_ASSERT("the store-triggered upper-word amominu.w breakpoint leaves the full doubleword, guard, adjacent, and marker words unchanged",
        trigger_preserved_words
    );

    goto_priv(PRIV_HS);
    TEST_SETUP_EXCEPT();
    uint32_t old_upper = amominu_w(upper_word_vaddr, final_operand);
    TEST_ASSERT("after clearing the store-only trigger, amominu.w returns the old upper word and unsigned-min updates only that 32-bit lane",
        excpt.triggered == false &&
        old_upper == upper_initial &&
        read64(target_vaddr) == expected_final &&
        read32(target_vaddr) == lower_initial &&
        read32(upper_word_vaddr) == expected_upper
    );

    write64(marker_vaddr, marker_final);
    TEST_ASSERT("post-clear marker progress and non-target sentinels prove the amominu.w store-trigger breakpoint path did not poison following memory operations",
        read64(guard_vaddr) == guard_seed &&
        read64(adjacent_vaddr) == adjacent_seed &&
        read64(marker_vaddr) == marker_final
    );

    goto_priv(PRIV_M);
    TEST_ASSERT("backing memory shows only the post-clear upper-word amominu.w unsigned-min lane update plus marker progress, with lower lane, guard, and adjacent unchanged",
        read64(guard_paddr) == guard_seed &&
        read64(target_paddr) == expected_final &&
        read32(target_paddr) == lower_initial &&
        read32(upper_word_paddr) == expected_upper &&
        read64(adjacent_paddr) == adjacent_seed &&
        read64(marker_paddr) == marker_final
    );

    TEST_END();
}

bool ai_micro_memblock_amominu_w_upper_word_load_trigger_breakpoint_suppresses_unsigned_min_lane_then_clear_succeeds_corner()
{
    TEST_START();

    const enum test_page page = VSRWX_GRWX;
    const uintptr_t base_vaddr = hs_page_base(page) + 0x9c0U;
    const uintptr_t base_paddr = phys_page_base(page) + 0x9c0U;
    const uintptr_t guard_vaddr = base_vaddr;
    const uintptr_t target_vaddr = base_vaddr + sizeof(uint64_t);
    const uintptr_t upper_word_vaddr = target_vaddr + sizeof(uint32_t);
    const uintptr_t adjacent_vaddr = base_vaddr + 2U * sizeof(uint64_t);
    const uintptr_t marker_vaddr = base_vaddr + 3U * sizeof(uint64_t);
    const uintptr_t guard_paddr = base_paddr;
    const uintptr_t target_paddr = base_paddr + sizeof(uint64_t);
    const uintptr_t upper_word_paddr = target_paddr + sizeof(uint32_t);
    const uintptr_t adjacent_paddr = base_paddr + 2U * sizeof(uint64_t);
    const uintptr_t marker_paddr = base_paddr + 3U * sizeof(uint64_t);
    const uint64_t guard_seed = 0x756d696e6c647570ULL;
    const uint32_t lower_initial = 0xcc33aa55U;
    const uint32_t upper_initial = 0xffffffffU;
    const uint64_t initial_value =
        ((uint64_t)upper_initial << 32) | lower_initial;
    const uint64_t adjacent_seed = 0x8877665544332211ULL;
    const uint64_t marker_seed = 0x1122334455667788ULL;
    const uint64_t marker_final = 0x78695a4b3c2d1e0fULL;
    const uint32_t blocked_operand = 0x00000020U;
    const uint32_t final_operand = 0x00000000U;
    const uint32_t expected_upper = final_operand;
    const uint64_t expected_final =
        ((uint64_t)expected_upper << 32) | lower_initial;

    goto_priv(PRIV_M);
    write64(guard_paddr, guard_seed);
    write64(target_paddr, initial_value);
    write64(adjacent_paddr, adjacent_seed);
    write64(marker_paddr, marker_seed);

    goto_priv(PRIV_HS);
    hspt_init();
    sfence_vma();

    ai_memblock_lrsc_trigger_clear_mcontrol6_0();
    ai_memblock_lrsc_trigger_arm_mcontrol6_0(upper_word_vaddr,
        AI_MEMBLOCK_LRSC_TRIGGER_LOAD);
    uint64_t saved_mstatus =
        ai_memblock_lrsc_trigger_enable_breakpoint_delivery();

    goto_priv(PRIV_HS);
    TEST_SETUP_EXCEPT();
    (void)amominu_w(upper_word_vaddr, blocked_operand);
    TEST_ASSERT("a load-only mcontrol6 address trigger armed on the upper word reports breakpoint on amominu.w before the unsigned-min lane RMW can commit",
        excpt.triggered == true &&
        excpt.cause == CAUSE_BKP
    );

    ai_memblock_lrsc_trigger_restore_machine_status(saved_mstatus);
    ai_memblock_lrsc_trigger_clear_mcontrol6_0();

    goto_priv(PRIV_M);
    bool trigger_preserved_words =
        read64(guard_paddr) == guard_seed &&
        read64(target_paddr) == initial_value &&
        read32(target_paddr) == lower_initial &&
        read32(upper_word_paddr) == upper_initial &&
        read64(adjacent_paddr) == adjacent_seed &&
        read64(marker_paddr) == marker_seed;
    TEST_ASSERT("the load-triggered upper-word amominu.w breakpoint leaves the full doubleword, guard, adjacent, and marker words unchanged",
        trigger_preserved_words
    );

    goto_priv(PRIV_HS);
    TEST_SETUP_EXCEPT();
    uint32_t old_upper = amominu_w(upper_word_vaddr, final_operand);
    TEST_ASSERT("after clearing the load-only trigger, amominu.w returns the old upper word and unsigned-min updates only that 32-bit lane",
        excpt.triggered == false &&
        old_upper == upper_initial &&
        read64(target_vaddr) == expected_final &&
        read32(target_vaddr) == lower_initial &&
        read32(upper_word_vaddr) == expected_upper
    );

    write64(marker_vaddr, marker_final);
    TEST_ASSERT("post-clear marker progress and non-target sentinels prove the amominu.w load-trigger breakpoint path did not poison following memory operations",
        read64(guard_vaddr) == guard_seed &&
        read64(adjacent_vaddr) == adjacent_seed &&
        read64(marker_vaddr) == marker_final
    );

    goto_priv(PRIV_M);
    TEST_ASSERT("backing memory shows only the post-clear upper-word amominu.w unsigned-min lane update plus marker progress, with lower lane, guard, and adjacent unchanged",
        read64(guard_paddr) == guard_seed &&
        read64(target_paddr) == expected_final &&
        read32(target_paddr) == lower_initial &&
        read32(upper_word_paddr) == expected_upper &&
        read64(adjacent_paddr) == adjacent_seed &&
        read64(marker_paddr) == marker_final
    );

    TEST_END();
}

bool ai_micro_memblock_amominu_w_lower_word_load_trigger_breakpoint_suppresses_unsigned_min_lane_then_clear_succeeds_corner()
{
    TEST_START();

    const enum test_page page = VSRWX_GRWX;
    const uintptr_t base_vaddr = hs_page_base(page) + 0xa00U;
    const uintptr_t base_paddr = phys_page_base(page) + 0xa00U;
    const uintptr_t guard_vaddr = base_vaddr;
    const uintptr_t target_vaddr = base_vaddr + sizeof(uint64_t);
    const uintptr_t lower_word_vaddr = target_vaddr;
    const uintptr_t upper_word_vaddr = target_vaddr + sizeof(uint32_t);
    const uintptr_t adjacent_vaddr = base_vaddr + 2U * sizeof(uint64_t);
    const uintptr_t marker_vaddr = base_vaddr + 3U * sizeof(uint64_t);
    const uintptr_t guard_paddr = base_paddr;
    const uintptr_t target_paddr = base_paddr + sizeof(uint64_t);
    const uintptr_t lower_word_paddr = target_paddr;
    const uintptr_t upper_word_paddr = target_paddr + sizeof(uint32_t);
    const uintptr_t adjacent_paddr = base_paddr + 2U * sizeof(uint64_t);
    const uintptr_t marker_paddr = base_paddr + 3U * sizeof(uint64_t);
    const uint64_t guard_seed = 0x756d696e6c6f776cULL;
    const uint32_t lower_initial = 0xffffffffU;
    const uint32_t upper_initial = 0x33cc55aaU;
    const uint64_t initial_value =
        ((uint64_t)upper_initial << 32) | lower_initial;
    const uint64_t adjacent_seed = 0x123456789abcdef0ULL;
    const uint64_t marker_seed = 0x0f1e2d3c4b5a6978ULL;
    const uint64_t marker_final = 0x87695a4b3c2d1e0fULL;
    const uint32_t blocked_operand = 0x00000040U;
    const uint32_t final_operand = 0x00000000U;
    const uint32_t expected_lower = final_operand;
    const uint64_t expected_final =
        ((uint64_t)upper_initial << 32) | expected_lower;

    goto_priv(PRIV_M);
    write64(guard_paddr, guard_seed);
    write64(target_paddr, initial_value);
    write64(adjacent_paddr, adjacent_seed);
    write64(marker_paddr, marker_seed);

    goto_priv(PRIV_HS);
    hspt_init();
    sfence_vma();

    ai_memblock_lrsc_trigger_clear_mcontrol6_0();
    ai_memblock_lrsc_trigger_arm_mcontrol6_0(lower_word_vaddr,
        AI_MEMBLOCK_LRSC_TRIGGER_LOAD);
    uint64_t saved_mstatus =
        ai_memblock_lrsc_trigger_enable_breakpoint_delivery();

    goto_priv(PRIV_HS);
    TEST_SETUP_EXCEPT();
    (void)amominu_w(lower_word_vaddr, blocked_operand);
    TEST_ASSERT("a load-only mcontrol6 address trigger armed on the lower word reports breakpoint on amominu.w before the unsigned-min lane RMW can commit",
        excpt.triggered == true &&
        excpt.cause == CAUSE_BKP
    );

    ai_memblock_lrsc_trigger_restore_machine_status(saved_mstatus);
    ai_memblock_lrsc_trigger_clear_mcontrol6_0();

    goto_priv(PRIV_M);
    bool trigger_preserved_words =
        read64(guard_paddr) == guard_seed &&
        read64(target_paddr) == initial_value &&
        read32(lower_word_paddr) == lower_initial &&
        read32(upper_word_paddr) == upper_initial &&
        read64(adjacent_paddr) == adjacent_seed &&
        read64(marker_paddr) == marker_seed;
    TEST_ASSERT("the load-triggered lower-word amominu.w breakpoint leaves the full doubleword, guard, adjacent, and marker words unchanged",
        trigger_preserved_words
    );

    goto_priv(PRIV_HS);
    TEST_SETUP_EXCEPT();
    uint32_t old_lower = amominu_w(lower_word_vaddr, final_operand);
    TEST_ASSERT("after clearing the load-only trigger, amominu.w returns the old lower word and unsigned-min updates only that 32-bit lane",
        excpt.triggered == false &&
        old_lower == lower_initial &&
        read64(target_vaddr) == expected_final &&
        read32(lower_word_vaddr) == expected_lower &&
        read32(upper_word_vaddr) == upper_initial
    );

    write64(marker_vaddr, marker_final);
    TEST_ASSERT("post-clear marker progress and non-target sentinels prove the amominu.w lower-word load-trigger breakpoint path did not poison following memory operations",
        read64(guard_vaddr) == guard_seed &&
        read64(adjacent_vaddr) == adjacent_seed &&
        read64(marker_vaddr) == marker_final
    );

    goto_priv(PRIV_M);
    TEST_ASSERT("backing memory shows only the post-clear lower-word amominu.w unsigned-min lane update plus marker progress, with upper lane, guard, and adjacent unchanged",
        read64(guard_paddr) == guard_seed &&
        read64(target_paddr) == expected_final &&
        read32(lower_word_paddr) == expected_lower &&
        read32(upper_word_paddr) == upper_initial &&
        read64(adjacent_paddr) == adjacent_seed &&
        read64(marker_paddr) == marker_final
    );

    TEST_END();
}

bool ai_micro_memblock_amominu_w_lower_word_store_trigger_breakpoint_suppresses_unsigned_min_lane_then_clear_succeeds_corner()
{
    TEST_START();

    const enum test_page page = VSRWX_GRWX;
    const uintptr_t base_vaddr = hs_page_base(page) + 0xa40U;
    const uintptr_t base_paddr = phys_page_base(page) + 0xa40U;
    const uintptr_t guard_vaddr = base_vaddr;
    const uintptr_t target_vaddr = base_vaddr + sizeof(uint64_t);
    const uintptr_t lower_word_vaddr = target_vaddr;
    const uintptr_t upper_word_vaddr = target_vaddr + sizeof(uint32_t);
    const uintptr_t adjacent_vaddr = base_vaddr + 2U * sizeof(uint64_t);
    const uintptr_t marker_vaddr = base_vaddr + 3U * sizeof(uint64_t);
    const uintptr_t guard_paddr = base_paddr;
    const uintptr_t target_paddr = base_paddr + sizeof(uint64_t);
    const uintptr_t lower_word_paddr = target_paddr;
    const uintptr_t upper_word_paddr = target_paddr + sizeof(uint32_t);
    const uintptr_t adjacent_paddr = base_paddr + 2U * sizeof(uint64_t);
    const uintptr_t marker_paddr = base_paddr + 3U * sizeof(uint64_t);
    const uint64_t guard_seed = 0x756d696e736c6f77ULL;
    const uint32_t lower_initial = 0xffffffffU;
    const uint32_t upper_initial = 0xaa5533ccU;
    const uint64_t initial_value =
        ((uint64_t)upper_initial << 32) | lower_initial;
    const uint64_t adjacent_seed = 0xfedcba9876543210ULL;
    const uint64_t marker_seed = 0x1020304050607080ULL;
    const uint64_t marker_final = 0x0807060504030201ULL;
    const uint32_t blocked_operand = 0x00000080U;
    const uint32_t final_operand = 0x00000000U;
    const uint32_t expected_lower = final_operand;
    const uint64_t expected_final =
        ((uint64_t)upper_initial << 32) | expected_lower;

    goto_priv(PRIV_M);
    write64(guard_paddr, guard_seed);
    write64(target_paddr, initial_value);
    write64(adjacent_paddr, adjacent_seed);
    write64(marker_paddr, marker_seed);

    goto_priv(PRIV_HS);
    hspt_init();
    sfence_vma();

    ai_memblock_lrsc_trigger_clear_mcontrol6_0();
    ai_memblock_lrsc_trigger_arm_mcontrol6_0(lower_word_vaddr,
        AI_MEMBLOCK_LRSC_TRIGGER_STORE);
    uint64_t saved_mstatus =
        ai_memblock_lrsc_trigger_enable_breakpoint_delivery();

    goto_priv(PRIV_HS);
    TEST_SETUP_EXCEPT();
    (void)amominu_w(lower_word_vaddr, blocked_operand);
    TEST_ASSERT("a store-only mcontrol6 address trigger armed on the lower word reports breakpoint on amominu.w before the unsigned-min lane write can commit",
        excpt.triggered == true &&
        excpt.cause == CAUSE_BKP
    );

    ai_memblock_lrsc_trigger_restore_machine_status(saved_mstatus);
    ai_memblock_lrsc_trigger_clear_mcontrol6_0();

    goto_priv(PRIV_M);
    bool trigger_preserved_words =
        read64(guard_paddr) == guard_seed &&
        read64(target_paddr) == initial_value &&
        read32(lower_word_paddr) == lower_initial &&
        read32(upper_word_paddr) == upper_initial &&
        read64(adjacent_paddr) == adjacent_seed &&
        read64(marker_paddr) == marker_seed;
    TEST_ASSERT("the store-triggered lower-word amominu.w breakpoint leaves the full doubleword, guard, adjacent, and marker words unchanged",
        trigger_preserved_words
    );

    goto_priv(PRIV_HS);
    TEST_SETUP_EXCEPT();
    uint32_t old_lower = amominu_w(lower_word_vaddr, final_operand);
    TEST_ASSERT("after clearing the store-only trigger, amominu.w returns the old lower word and unsigned-min updates only that 32-bit lane",
        excpt.triggered == false &&
        old_lower == lower_initial &&
        read64(target_vaddr) == expected_final &&
        read32(lower_word_vaddr) == expected_lower &&
        read32(upper_word_vaddr) == upper_initial
    );

    write64(marker_vaddr, marker_final);
    TEST_ASSERT("post-clear marker progress and non-target sentinels prove the amominu.w lower-word store-trigger breakpoint path did not poison following memory operations",
        read64(guard_vaddr) == guard_seed &&
        read64(adjacent_vaddr) == adjacent_seed &&
        read64(marker_vaddr) == marker_final
    );

    goto_priv(PRIV_M);
    TEST_ASSERT("backing memory shows only the post-clear lower-word amominu.w unsigned-min lane update plus marker progress, with upper lane, guard, and adjacent unchanged",
        read64(guard_paddr) == guard_seed &&
        read64(target_paddr) == expected_final &&
        read32(lower_word_paddr) == expected_lower &&
        read32(upper_word_paddr) == upper_initial &&
        read64(adjacent_paddr) == adjacent_seed &&
        read64(marker_paddr) == marker_final
    );

    TEST_END();
}

bool ai_micro_memblock_amomaxu_w_upper_word_load_trigger_breakpoint_suppresses_unsigned_max_lane_then_clear_succeeds_corner()
{
    TEST_START();

    const enum test_page page = VSRWX_GRWX;
    const uintptr_t base_vaddr = hs_page_base(page) + 0xa80U;
    const uintptr_t base_paddr = phys_page_base(page) + 0xa80U;
    const uintptr_t guard_vaddr = base_vaddr;
    const uintptr_t target_vaddr = base_vaddr + sizeof(uint64_t);
    const uintptr_t upper_word_vaddr = target_vaddr + sizeof(uint32_t);
    const uintptr_t adjacent_vaddr = base_vaddr + 2U * sizeof(uint64_t);
    const uintptr_t marker_vaddr = base_vaddr + 3U * sizeof(uint64_t);
    const uintptr_t guard_paddr = base_paddr;
    const uintptr_t target_paddr = base_paddr + sizeof(uint64_t);
    const uintptr_t upper_word_paddr = target_paddr + sizeof(uint32_t);
    const uintptr_t adjacent_paddr = base_paddr + 2U * sizeof(uint64_t);
    const uintptr_t marker_paddr = base_paddr + 3U * sizeof(uint64_t);
    const uint64_t guard_seed = 0x756d61787570706cULL;
    const uint32_t lower_initial = 0x2468ace0U;
    const uint32_t upper_initial = 0x7fffffffU;
    const uint64_t initial_value =
        ((uint64_t)upper_initial << 32) | lower_initial;
    const uint64_t adjacent_seed = 0x0f1e2d3c4b5a6978ULL;
    const uint64_t marker_seed = 0x8877665544332211ULL;
    const uint64_t marker_final = 0x1122334455667788ULL;
    const uint32_t blocked_operand = 0x80000010U;
    const uint32_t final_operand = 0x80000000U;
    const uint32_t expected_upper = final_operand;
    const uint64_t expected_final =
        ((uint64_t)expected_upper << 32) | lower_initial;

    goto_priv(PRIV_M);
    write64(guard_paddr, guard_seed);
    write64(target_paddr, initial_value);
    write64(adjacent_paddr, adjacent_seed);
    write64(marker_paddr, marker_seed);

    goto_priv(PRIV_HS);
    hspt_init();
    sfence_vma();

    ai_memblock_lrsc_trigger_clear_mcontrol6_0();
    ai_memblock_lrsc_trigger_arm_mcontrol6_0(upper_word_vaddr,
        AI_MEMBLOCK_LRSC_TRIGGER_LOAD);
    uint64_t saved_mstatus =
        ai_memblock_lrsc_trigger_enable_breakpoint_delivery();

    goto_priv(PRIV_HS);
    TEST_SETUP_EXCEPT();
    (void)amomaxu_w(upper_word_vaddr, blocked_operand);
    TEST_ASSERT("a load-only mcontrol6 address trigger armed on the upper word reports breakpoint on amomaxu.w before the unsigned-max lane RMW can commit",
        excpt.triggered == true &&
        excpt.cause == CAUSE_BKP
    );

    ai_memblock_lrsc_trigger_restore_machine_status(saved_mstatus);
    ai_memblock_lrsc_trigger_clear_mcontrol6_0();

    goto_priv(PRIV_M);
    bool trigger_preserved_words =
        read64(guard_paddr) == guard_seed &&
        read64(target_paddr) == initial_value &&
        read32(target_paddr) == lower_initial &&
        read32(upper_word_paddr) == upper_initial &&
        read64(adjacent_paddr) == adjacent_seed &&
        read64(marker_paddr) == marker_seed;
    TEST_ASSERT("the load-triggered upper-word amomaxu.w breakpoint leaves the full doubleword, guard, adjacent, and marker words unchanged",
        trigger_preserved_words
    );

    goto_priv(PRIV_HS);
    TEST_SETUP_EXCEPT();
    uint32_t old_upper = amomaxu_w(upper_word_vaddr, final_operand);
    TEST_ASSERT("after clearing the load-only trigger, amomaxu.w returns the old upper word and unsigned-max updates only that 32-bit lane",
        excpt.triggered == false &&
        old_upper == upper_initial &&
        read64(target_vaddr) == expected_final &&
        read32(target_vaddr) == lower_initial &&
        read32(upper_word_vaddr) == expected_upper
    );

    write64(marker_vaddr, marker_final);
    TEST_ASSERT("post-clear marker progress and non-target sentinels prove the amomaxu.w load-trigger breakpoint path did not poison following memory operations",
        read64(guard_vaddr) == guard_seed &&
        read64(adjacent_vaddr) == adjacent_seed &&
        read64(marker_vaddr) == marker_final
    );

    goto_priv(PRIV_M);
    TEST_ASSERT("backing memory shows only the post-clear upper-word amomaxu.w unsigned-max lane update plus marker progress, with lower lane, guard, and adjacent unchanged",
        read64(guard_paddr) == guard_seed &&
        read64(target_paddr) == expected_final &&
        read32(target_paddr) == lower_initial &&
        read32(upper_word_paddr) == expected_upper &&
        read64(adjacent_paddr) == adjacent_seed &&
        read64(marker_paddr) == marker_final
    );

    TEST_END();
}

bool ai_micro_memblock_amomaxu_w_upper_word_store_trigger_breakpoint_suppresses_unsigned_max_lane_then_clear_succeeds_corner()
{
    TEST_START();

    const enum test_page page = VSRWX_GRWX;
    const uintptr_t base_vaddr = hs_page_base(page) + 0xac0U;
    const uintptr_t base_paddr = phys_page_base(page) + 0xac0U;
    const uintptr_t guard_vaddr = base_vaddr;
    const uintptr_t target_vaddr = base_vaddr + sizeof(uint64_t);
    const uintptr_t upper_word_vaddr = target_vaddr + sizeof(uint32_t);
    const uintptr_t adjacent_vaddr = base_vaddr + 2U * sizeof(uint64_t);
    const uintptr_t marker_vaddr = base_vaddr + 3U * sizeof(uint64_t);
    const uintptr_t guard_paddr = base_paddr;
    const uintptr_t target_paddr = base_paddr + sizeof(uint64_t);
    const uintptr_t upper_word_paddr = target_paddr + sizeof(uint32_t);
    const uintptr_t adjacent_paddr = base_paddr + 2U * sizeof(uint64_t);
    const uintptr_t marker_paddr = base_paddr + 3U * sizeof(uint64_t);
    const uint64_t guard_seed = 0x756d617875737470ULL;
    const uint32_t lower_initial = 0x13579bdfU;
    const uint32_t upper_initial = 0x7ffffffeU;
    const uint64_t initial_value =
        ((uint64_t)upper_initial << 32) | lower_initial;
    const uint64_t adjacent_seed = 0xf0e1d2c3b4a59687ULL;
    const uint64_t marker_seed = 0x0123456789abcdefULL;
    const uint64_t marker_final = 0xfedcba9876543210ULL;
    const uint32_t blocked_operand = 0x80000020U;
    const uint32_t final_operand = 0x80000001U;
    const uint32_t expected_upper = final_operand;
    const uint64_t expected_final =
        ((uint64_t)expected_upper << 32) | lower_initial;

    goto_priv(PRIV_M);
    write64(guard_paddr, guard_seed);
    write64(target_paddr, initial_value);
    write64(adjacent_paddr, adjacent_seed);
    write64(marker_paddr, marker_seed);

    goto_priv(PRIV_HS);
    hspt_init();
    sfence_vma();

    ai_memblock_lrsc_trigger_clear_mcontrol6_0();
    ai_memblock_lrsc_trigger_arm_mcontrol6_0(upper_word_vaddr,
        AI_MEMBLOCK_LRSC_TRIGGER_STORE);
    uint64_t saved_mstatus =
        ai_memblock_lrsc_trigger_enable_breakpoint_delivery();

    goto_priv(PRIV_HS);
    TEST_SETUP_EXCEPT();
    (void)amomaxu_w(upper_word_vaddr, blocked_operand);
    TEST_ASSERT("a store-only mcontrol6 address trigger armed on the upper word reports breakpoint on amomaxu.w before the unsigned-max lane write can commit",
        excpt.triggered == true &&
        excpt.cause == CAUSE_BKP
    );

    ai_memblock_lrsc_trigger_restore_machine_status(saved_mstatus);
    ai_memblock_lrsc_trigger_clear_mcontrol6_0();

    goto_priv(PRIV_M);
    bool trigger_preserved_words =
        read64(guard_paddr) == guard_seed &&
        read64(target_paddr) == initial_value &&
        read32(target_paddr) == lower_initial &&
        read32(upper_word_paddr) == upper_initial &&
        read64(adjacent_paddr) == adjacent_seed &&
        read64(marker_paddr) == marker_seed;
    TEST_ASSERT("the store-triggered upper-word amomaxu.w breakpoint leaves the full doubleword, guard, adjacent, and marker words unchanged",
        trigger_preserved_words
    );

    goto_priv(PRIV_HS);
    TEST_SETUP_EXCEPT();
    uint32_t old_upper = amomaxu_w(upper_word_vaddr, final_operand);
    TEST_ASSERT("after clearing the store-only trigger, amomaxu.w returns the old upper word and unsigned-max updates only that 32-bit lane",
        excpt.triggered == false &&
        old_upper == upper_initial &&
        read64(target_vaddr) == expected_final &&
        read32(target_vaddr) == lower_initial &&
        read32(upper_word_vaddr) == expected_upper
    );

    write64(marker_vaddr, marker_final);
    TEST_ASSERT("post-clear marker progress and non-target sentinels prove the amomaxu.w store-trigger breakpoint path did not poison following memory operations",
        read64(guard_vaddr) == guard_seed &&
        read64(adjacent_vaddr) == adjacent_seed &&
        read64(marker_vaddr) == marker_final
    );

    goto_priv(PRIV_M);
    TEST_ASSERT("backing memory shows only the post-clear upper-word amomaxu.w unsigned-max lane update plus marker progress, with lower lane, guard, and adjacent unchanged",
        read64(guard_paddr) == guard_seed &&
        read64(target_paddr) == expected_final &&
        read32(target_paddr) == lower_initial &&
        read32(upper_word_paddr) == expected_upper &&
        read64(adjacent_paddr) == adjacent_seed &&
        read64(marker_paddr) == marker_final
    );

    TEST_END();
}

bool ai_micro_memblock_amomaxu_w_lower_word_load_trigger_breakpoint_suppresses_unsigned_max_lane_then_clear_succeeds_corner()
{
    TEST_START();

    const enum test_page page = VSRWX_GRWX;
    const uintptr_t base_vaddr = hs_page_base(page) + 0xb00U;
    const uintptr_t base_paddr = phys_page_base(page) + 0xb00U;
    const uintptr_t guard_vaddr = base_vaddr;
    const uintptr_t target_vaddr = base_vaddr + sizeof(uint64_t);
    const uintptr_t lower_word_vaddr = target_vaddr;
    const uintptr_t upper_word_vaddr = target_vaddr + sizeof(uint32_t);
    const uintptr_t adjacent_vaddr = base_vaddr + 2U * sizeof(uint64_t);
    const uintptr_t marker_vaddr = base_vaddr + 3U * sizeof(uint64_t);
    const uintptr_t guard_paddr = base_paddr;
    const uintptr_t target_paddr = base_paddr + sizeof(uint64_t);
    const uintptr_t lower_word_paddr = target_paddr;
    const uintptr_t upper_word_paddr = target_paddr + sizeof(uint32_t);
    const uintptr_t adjacent_paddr = base_paddr + 2U * sizeof(uint64_t);
    const uintptr_t marker_paddr = base_paddr + 3U * sizeof(uint64_t);
    const uint64_t guard_seed = 0x756d61786c64726cULL;
    const uint32_t lower_initial = 0x7fffffffU;
    const uint32_t upper_initial = 0x2468ace0U;
    const uint64_t initial_value =
        ((uint64_t)upper_initial << 32) | lower_initial;
    const uint64_t adjacent_seed = 0x55aa6699cc33dd44ULL;
    const uint64_t marker_seed = 0x1029384756abcdefULL;
    const uint64_t marker_final = 0xfedcba6547382910ULL;
    const uint32_t blocked_operand = 0x80000040U;
    const uint32_t final_operand = 0x80000000U;
    const uint32_t expected_lower = final_operand;
    const uint64_t expected_final =
        ((uint64_t)upper_initial << 32) | expected_lower;

    goto_priv(PRIV_M);
    write64(guard_paddr, guard_seed);
    write64(target_paddr, initial_value);
    write64(adjacent_paddr, adjacent_seed);
    write64(marker_paddr, marker_seed);

    goto_priv(PRIV_HS);
    hspt_init();
    sfence_vma();

    ai_memblock_lrsc_trigger_clear_mcontrol6_0();
    ai_memblock_lrsc_trigger_arm_mcontrol6_0(lower_word_vaddr,
        AI_MEMBLOCK_LRSC_TRIGGER_LOAD);
    uint64_t saved_mstatus =
        ai_memblock_lrsc_trigger_enable_breakpoint_delivery();

    goto_priv(PRIV_HS);
    TEST_SETUP_EXCEPT();
    (void)amomaxu_w(lower_word_vaddr, blocked_operand);
    TEST_ASSERT("a load-only mcontrol6 address trigger armed on the lower word reports breakpoint on amomaxu.w before the unsigned-max lane RMW can commit",
        excpt.triggered == true &&
        excpt.cause == CAUSE_BKP
    );

    ai_memblock_lrsc_trigger_restore_machine_status(saved_mstatus);
    ai_memblock_lrsc_trigger_clear_mcontrol6_0();

    goto_priv(PRIV_M);
    bool trigger_preserved_words =
        read64(guard_paddr) == guard_seed &&
        read64(target_paddr) == initial_value &&
        read32(lower_word_paddr) == lower_initial &&
        read32(upper_word_paddr) == upper_initial &&
        read64(adjacent_paddr) == adjacent_seed &&
        read64(marker_paddr) == marker_seed;
    TEST_ASSERT("the load-triggered lower-word amomaxu.w breakpoint leaves the full doubleword, guard, adjacent, and marker words unchanged",
        trigger_preserved_words
    );

    goto_priv(PRIV_HS);
    TEST_SETUP_EXCEPT();
    uint32_t old_lower = amomaxu_w(lower_word_vaddr, final_operand);
    TEST_ASSERT("after clearing the load-only trigger, amomaxu.w returns the old lower word and unsigned-max updates only that 32-bit lane",
        excpt.triggered == false &&
        old_lower == lower_initial &&
        read64(target_vaddr) == expected_final &&
        read32(lower_word_vaddr) == expected_lower &&
        read32(upper_word_vaddr) == upper_initial
    );

    write64(marker_vaddr, marker_final);
    TEST_ASSERT("post-clear marker progress and non-target sentinels prove the amomaxu.w lower-word load-trigger breakpoint path did not poison following memory operations",
        read64(guard_vaddr) == guard_seed &&
        read64(adjacent_vaddr) == adjacent_seed &&
        read64(marker_vaddr) == marker_final
    );

    goto_priv(PRIV_M);
    TEST_ASSERT("backing memory shows only the post-clear lower-word amomaxu.w unsigned-max lane update plus marker progress, with upper lane, guard, and adjacent unchanged",
        read64(guard_paddr) == guard_seed &&
        read64(target_paddr) == expected_final &&
        read32(lower_word_paddr) == expected_lower &&
        read32(upper_word_paddr) == upper_initial &&
        read64(adjacent_paddr) == adjacent_seed &&
        read64(marker_paddr) == marker_final
    );

    TEST_END();
}

bool ai_micro_memblock_amomaxu_w_lower_word_store_trigger_breakpoint_suppresses_unsigned_max_lane_then_clear_succeeds_corner()
{
    TEST_START();

    const enum test_page page = VSRWX_GRWX;
    const uintptr_t base_vaddr = hs_page_base(page) + 0xb40U;
    const uintptr_t base_paddr = phys_page_base(page) + 0xb40U;
    const uintptr_t guard_vaddr = base_vaddr;
    const uintptr_t target_vaddr = base_vaddr + sizeof(uint64_t);
    const uintptr_t lower_word_vaddr = target_vaddr;
    const uintptr_t upper_word_vaddr = target_vaddr + sizeof(uint32_t);
    const uintptr_t adjacent_vaddr = base_vaddr + 2U * sizeof(uint64_t);
    const uintptr_t marker_vaddr = base_vaddr + 3U * sizeof(uint64_t);
    const uintptr_t guard_paddr = base_paddr;
    const uintptr_t target_paddr = base_paddr + sizeof(uint64_t);
    const uintptr_t lower_word_paddr = target_paddr;
    const uintptr_t upper_word_paddr = target_paddr + sizeof(uint32_t);
    const uintptr_t adjacent_paddr = base_paddr + 2U * sizeof(uint64_t);
    const uintptr_t marker_paddr = base_paddr + 3U * sizeof(uint64_t);
    const uint64_t guard_seed = 0x756d61786c737472ULL;
    const uint32_t lower_initial = 0x7ffffffeU;
    const uint32_t upper_initial = 0x3579bdf1U;
    const uint64_t initial_value =
        ((uint64_t)upper_initial << 32) | lower_initial;
    const uint64_t adjacent_seed = 0xa55a33cc996655aaULL;
    const uint64_t marker_seed = 0x0101020203030404ULL;
    const uint64_t marker_final = 0x4040030302020101ULL;
    const uint32_t blocked_operand = 0x80000030U;
    const uint32_t final_operand = 0x80000002U;
    const uint32_t expected_lower = final_operand;
    const uint64_t expected_final =
        ((uint64_t)upper_initial << 32) | expected_lower;

    goto_priv(PRIV_M);
    write64(guard_paddr, guard_seed);
    write64(target_paddr, initial_value);
    write64(adjacent_paddr, adjacent_seed);
    write64(marker_paddr, marker_seed);

    goto_priv(PRIV_HS);
    hspt_init();
    sfence_vma();

    ai_memblock_lrsc_trigger_clear_mcontrol6_0();
    ai_memblock_lrsc_trigger_arm_mcontrol6_0(lower_word_vaddr,
        AI_MEMBLOCK_LRSC_TRIGGER_STORE);
    uint64_t saved_mstatus =
        ai_memblock_lrsc_trigger_enable_breakpoint_delivery();

    goto_priv(PRIV_HS);
    TEST_SETUP_EXCEPT();
    (void)amomaxu_w(lower_word_vaddr, blocked_operand);
    TEST_ASSERT("a store-only mcontrol6 address trigger armed on the lower word reports breakpoint on amomaxu.w before the unsigned-max lane write can commit",
        excpt.triggered == true &&
        excpt.cause == CAUSE_BKP
    );

    ai_memblock_lrsc_trigger_restore_machine_status(saved_mstatus);
    ai_memblock_lrsc_trigger_clear_mcontrol6_0();

    goto_priv(PRIV_M);
    bool trigger_preserved_words =
        read64(guard_paddr) == guard_seed &&
        read64(target_paddr) == initial_value &&
        read32(lower_word_paddr) == lower_initial &&
        read32(upper_word_paddr) == upper_initial &&
        read64(adjacent_paddr) == adjacent_seed &&
        read64(marker_paddr) == marker_seed;
    TEST_ASSERT("the store-triggered lower-word amomaxu.w breakpoint leaves the full doubleword, guard, adjacent, and marker words unchanged",
        trigger_preserved_words
    );

    goto_priv(PRIV_HS);
    TEST_SETUP_EXCEPT();
    uint32_t old_lower = amomaxu_w(lower_word_vaddr, final_operand);
    TEST_ASSERT("after clearing the store-only trigger, amomaxu.w returns the old lower word and unsigned-max updates only that 32-bit lane",
        excpt.triggered == false &&
        old_lower == lower_initial &&
        read64(target_vaddr) == expected_final &&
        read32(lower_word_vaddr) == expected_lower &&
        read32(upper_word_vaddr) == upper_initial
    );

    write64(marker_vaddr, marker_final);
    TEST_ASSERT("post-clear marker progress and non-target sentinels prove the amomaxu.w lower-word store-trigger breakpoint path did not poison following memory operations",
        read64(guard_vaddr) == guard_seed &&
        read64(adjacent_vaddr) == adjacent_seed &&
        read64(marker_vaddr) == marker_final
    );

    goto_priv(PRIV_M);
    TEST_ASSERT("backing memory shows only the post-clear lower-word amomaxu.w unsigned-max lane update plus marker progress, with upper lane, guard, and adjacent unchanged",
        read64(guard_paddr) == guard_seed &&
        read64(target_paddr) == expected_final &&
        read32(lower_word_paddr) == expected_lower &&
        read32(upper_word_paddr) == upper_initial &&
        read64(adjacent_paddr) == adjacent_seed &&
        read64(marker_paddr) == marker_final
    );

    TEST_END();
}

bool ai_micro_memblock_amomax_w_upper_word_load_trigger_breakpoint_suppresses_signed_max_lane_then_clear_succeeds_corner()
{
    TEST_START();

    const enum test_page page = VSRWX_GRWX;
    const uintptr_t base_vaddr = hs_page_base(page) + 0xb80U;
    const uintptr_t base_paddr = phys_page_base(page) + 0xb80U;
    const uintptr_t guard_vaddr = base_vaddr;
    const uintptr_t target_vaddr = base_vaddr + sizeof(uint64_t);
    const uintptr_t upper_word_vaddr = target_vaddr + sizeof(uint32_t);
    const uintptr_t adjacent_vaddr = base_vaddr + 2U * sizeof(uint64_t);
    const uintptr_t marker_vaddr = base_vaddr + 3U * sizeof(uint64_t);
    const uintptr_t guard_paddr = base_paddr;
    const uintptr_t target_paddr = base_paddr + sizeof(uint64_t);
    const uintptr_t upper_word_paddr = target_paddr + sizeof(uint32_t);
    const uintptr_t adjacent_paddr = base_paddr + 2U * sizeof(uint64_t);
    const uintptr_t marker_paddr = base_paddr + 3U * sizeof(uint64_t);
    const uint64_t guard_seed = 0x6d61786c64757070ULL;
    const uint32_t lower_initial = 0x13579bdfU;
    const uint32_t upper_initial = 0x80000000U;
    const uint64_t initial_value =
        ((uint64_t)upper_initial << 32) | lower_initial;
    const uint64_t adjacent_seed = 0x0badcafe55aa33ccULL;
    const uint64_t marker_seed = 0x1122334455667788ULL;
    const uint64_t marker_final = 0x8877665544332211ULL;
    const uint32_t blocked_operand = 0x7ffffff0U;
    const uint32_t final_operand = 0x7fffffffU;
    const uint32_t expected_upper = final_operand;
    const uint64_t expected_final =
        ((uint64_t)expected_upper << 32) | lower_initial;

    goto_priv(PRIV_M);
    write64(guard_paddr, guard_seed);
    write64(target_paddr, initial_value);
    write64(adjacent_paddr, adjacent_seed);
    write64(marker_paddr, marker_seed);

    goto_priv(PRIV_HS);
    hspt_init();
    sfence_vma();

    ai_memblock_lrsc_trigger_clear_mcontrol6_0();
    ai_memblock_lrsc_trigger_arm_mcontrol6_0(upper_word_vaddr,
        AI_MEMBLOCK_LRSC_TRIGGER_LOAD);
    uint64_t saved_mstatus =
        ai_memblock_lrsc_trigger_enable_breakpoint_delivery();

    goto_priv(PRIV_HS);
    TEST_SETUP_EXCEPT();
    (void)amomax_w(upper_word_vaddr, blocked_operand);
    TEST_ASSERT("a load-only mcontrol6 address trigger armed on the upper word reports breakpoint on amomax.w before the signed-max lane RMW can commit",
        excpt.triggered == true &&
        excpt.cause == CAUSE_BKP
    );

    ai_memblock_lrsc_trigger_restore_machine_status(saved_mstatus);
    ai_memblock_lrsc_trigger_clear_mcontrol6_0();

    goto_priv(PRIV_M);
    bool trigger_preserved_words =
        read64(guard_paddr) == guard_seed &&
        read64(target_paddr) == initial_value &&
        read32(target_paddr) == lower_initial &&
        read32(upper_word_paddr) == upper_initial &&
        read64(adjacent_paddr) == adjacent_seed &&
        read64(marker_paddr) == marker_seed;
    TEST_ASSERT("the load-triggered upper-word amomax.w breakpoint leaves the full doubleword, guard, adjacent, and marker words unchanged",
        trigger_preserved_words
    );

    goto_priv(PRIV_HS);
    TEST_SETUP_EXCEPT();
    uint32_t old_upper = amomax_w(upper_word_vaddr, final_operand);
    TEST_ASSERT("after clearing the load-only trigger, amomax.w returns the old upper word and signed-max updates only that 32-bit lane",
        excpt.triggered == false &&
        old_upper == upper_initial &&
        read64(target_vaddr) == expected_final &&
        read32(target_vaddr) == lower_initial &&
        read32(upper_word_vaddr) == expected_upper
    );

    write64(marker_vaddr, marker_final);
    TEST_ASSERT("post-clear marker progress and non-target sentinels prove the amomax.w upper-word load-trigger breakpoint path did not poison following memory operations",
        read64(guard_vaddr) == guard_seed &&
        read64(adjacent_vaddr) == adjacent_seed &&
        read64(marker_vaddr) == marker_final
    );

    goto_priv(PRIV_M);
    TEST_ASSERT("backing memory shows only the post-clear upper-word amomax.w signed-max lane update plus marker progress, with lower lane, guard, and adjacent unchanged",
        read64(guard_paddr) == guard_seed &&
        read64(target_paddr) == expected_final &&
        read32(target_paddr) == lower_initial &&
        read32(upper_word_paddr) == expected_upper &&
        read64(adjacent_paddr) == adjacent_seed &&
        read64(marker_paddr) == marker_final
    );

    TEST_END();
}

bool ai_micro_memblock_amomax_w_upper_word_store_trigger_breakpoint_suppresses_signed_max_lane_then_clear_succeeds_corner()
{
    TEST_START();

    const enum test_page page = VSRWX_GRWX;
    const uintptr_t base_vaddr = hs_page_base(page) + 0xbc0U;
    const uintptr_t base_paddr = phys_page_base(page) + 0xbc0U;
    const uintptr_t guard_vaddr = base_vaddr;
    const uintptr_t target_vaddr = base_vaddr + sizeof(uint64_t);
    const uintptr_t upper_word_vaddr = target_vaddr + sizeof(uint32_t);
    const uintptr_t adjacent_vaddr = base_vaddr + 2U * sizeof(uint64_t);
    const uintptr_t marker_vaddr = base_vaddr + 3U * sizeof(uint64_t);
    const uintptr_t guard_paddr = base_paddr;
    const uintptr_t target_paddr = base_paddr + sizeof(uint64_t);
    const uintptr_t upper_word_paddr = target_paddr + sizeof(uint32_t);
    const uintptr_t adjacent_paddr = base_paddr + 2U * sizeof(uint64_t);
    const uintptr_t marker_paddr = base_paddr + 3U * sizeof(uint64_t);
    const uint64_t guard_seed = 0x6d61787374757070ULL;
    const uint32_t lower_initial = 0xcafebabeU;
    const uint32_t upper_initial = 0x80000000U;
    const uint64_t initial_value =
        ((uint64_t)upper_initial << 32) | lower_initial;
    const uint64_t adjacent_seed = 0x7766554433221100ULL;
    const uint64_t marker_seed = 0x0f0e0d0c0b0a0908ULL;
    const uint64_t marker_final = 0x8090a0b0c0d0e0f0ULL;
    const uint32_t blocked_operand = 0x7ffffff8U;
    const uint32_t final_operand = 0x7fffffffU;
    const uint32_t expected_upper = final_operand;
    const uint64_t expected_final =
        ((uint64_t)expected_upper << 32) | lower_initial;

    goto_priv(PRIV_M);
    write64(guard_paddr, guard_seed);
    write64(target_paddr, initial_value);
    write64(adjacent_paddr, adjacent_seed);
    write64(marker_paddr, marker_seed);

    goto_priv(PRIV_HS);
    hspt_init();
    sfence_vma();

    ai_memblock_lrsc_trigger_clear_mcontrol6_0();
    ai_memblock_lrsc_trigger_arm_mcontrol6_0(upper_word_vaddr,
        AI_MEMBLOCK_LRSC_TRIGGER_STORE);
    uint64_t saved_mstatus =
        ai_memblock_lrsc_trigger_enable_breakpoint_delivery();

    goto_priv(PRIV_HS);
    TEST_SETUP_EXCEPT();
    (void)amomax_w(upper_word_vaddr, blocked_operand);
    TEST_ASSERT("a store-only mcontrol6 address trigger armed on the upper word reports breakpoint on amomax.w before the signed-max lane write can commit",
        excpt.triggered == true &&
        excpt.cause == CAUSE_BKP
    );

    ai_memblock_lrsc_trigger_restore_machine_status(saved_mstatus);
    ai_memblock_lrsc_trigger_clear_mcontrol6_0();

    goto_priv(PRIV_M);
    bool trigger_preserved_words =
        read64(guard_paddr) == guard_seed &&
        read64(target_paddr) == initial_value &&
        read32(target_paddr) == lower_initial &&
        read32(upper_word_paddr) == upper_initial &&
        read64(adjacent_paddr) == adjacent_seed &&
        read64(marker_paddr) == marker_seed;
    TEST_ASSERT("the store-triggered upper-word amomax.w breakpoint leaves the full doubleword, guard, adjacent, and marker words unchanged",
        trigger_preserved_words
    );

    goto_priv(PRIV_HS);
    TEST_SETUP_EXCEPT();
    uint32_t old_upper = amomax_w(upper_word_vaddr, final_operand);
    TEST_ASSERT("after clearing the store-only trigger, amomax.w returns the old upper word and signed-max updates only that 32-bit lane",
        excpt.triggered == false &&
        old_upper == upper_initial &&
        read64(target_vaddr) == expected_final &&
        read32(target_vaddr) == lower_initial &&
        read32(upper_word_vaddr) == expected_upper
    );

    write64(marker_vaddr, marker_final);
    TEST_ASSERT("post-clear marker progress and non-target sentinels prove the amomax.w upper-word store-trigger breakpoint path did not poison following memory operations",
        read64(guard_vaddr) == guard_seed &&
        read64(adjacent_vaddr) == adjacent_seed &&
        read64(marker_vaddr) == marker_final
    );

    goto_priv(PRIV_M);
    TEST_ASSERT("backing memory shows only the post-clear upper-word amomax.w signed-max lane update plus marker progress, with lower lane, guard, and adjacent unchanged",
        read64(guard_paddr) == guard_seed &&
        read64(target_paddr) == expected_final &&
        read32(target_paddr) == lower_initial &&
        read32(upper_word_paddr) == expected_upper &&
        read64(adjacent_paddr) == adjacent_seed &&
        read64(marker_paddr) == marker_final
    );

    TEST_END();
}

bool ai_micro_memblock_amomax_w_lower_word_load_trigger_breakpoint_suppresses_signed_max_lane_then_clear_succeeds_corner()
{
    TEST_START();

    const enum test_page page = VSRWX_GRWX;
    const uintptr_t base_vaddr = hs_page_base(page) + 0xc00U;
    const uintptr_t base_paddr = phys_page_base(page) + 0xc00U;
    const uintptr_t guard_vaddr = base_vaddr;
    const uintptr_t target_vaddr = base_vaddr + sizeof(uint64_t);
    const uintptr_t lower_word_vaddr = target_vaddr;
    const uintptr_t upper_word_vaddr = target_vaddr + sizeof(uint32_t);
    const uintptr_t adjacent_vaddr = base_vaddr + 2U * sizeof(uint64_t);
    const uintptr_t marker_vaddr = base_vaddr + 3U * sizeof(uint64_t);
    const uintptr_t guard_paddr = base_paddr;
    const uintptr_t target_paddr = base_paddr + sizeof(uint64_t);
    const uintptr_t lower_word_paddr = target_paddr;
    const uintptr_t upper_word_paddr = target_paddr + sizeof(uint32_t);
    const uintptr_t adjacent_paddr = base_paddr + 2U * sizeof(uint64_t);
    const uintptr_t marker_paddr = base_paddr + 3U * sizeof(uint64_t);
    const uint64_t guard_seed = 0x6d61786c6c64726cULL;
    const uint32_t lower_initial = 0x80000000U;
    const uint32_t upper_initial = 0x2468ace0U;
    const uint64_t initial_value =
        ((uint64_t)upper_initial << 32) | lower_initial;
    const uint64_t adjacent_seed = 0x55aa33cc77889900ULL;
    const uint64_t marker_seed = 0x0123456789abcdefULL;
    const uint64_t marker_final = 0xfedcba9876543210ULL;
    const uint32_t blocked_operand = 0x7ffffff0U;
    const uint32_t final_operand = 0x7fffffffU;
    const uint32_t expected_lower = final_operand;
    const uint64_t expected_final =
        ((uint64_t)upper_initial << 32) | expected_lower;

    goto_priv(PRIV_M);
    write64(guard_paddr, guard_seed);
    write64(target_paddr, initial_value);
    write64(adjacent_paddr, adjacent_seed);
    write64(marker_paddr, marker_seed);

    goto_priv(PRIV_HS);
    hspt_init();
    sfence_vma();

    ai_memblock_lrsc_trigger_clear_mcontrol6_0();
    ai_memblock_lrsc_trigger_arm_mcontrol6_0(lower_word_vaddr,
        AI_MEMBLOCK_LRSC_TRIGGER_LOAD);
    uint64_t saved_mstatus =
        ai_memblock_lrsc_trigger_enable_breakpoint_delivery();

    goto_priv(PRIV_HS);
    TEST_SETUP_EXCEPT();
    (void)amomax_w(lower_word_vaddr, blocked_operand);
    TEST_ASSERT("a load-only mcontrol6 address trigger armed on the lower word reports breakpoint on amomax.w before the signed-max lane RMW can commit",
        excpt.triggered == true &&
        excpt.cause == CAUSE_BKP
    );

    ai_memblock_lrsc_trigger_restore_machine_status(saved_mstatus);
    ai_memblock_lrsc_trigger_clear_mcontrol6_0();

    goto_priv(PRIV_M);
    bool trigger_preserved_words =
        read64(guard_paddr) == guard_seed &&
        read64(target_paddr) == initial_value &&
        read32(lower_word_paddr) == lower_initial &&
        read32(upper_word_paddr) == upper_initial &&
        read64(adjacent_paddr) == adjacent_seed &&
        read64(marker_paddr) == marker_seed;
    TEST_ASSERT("the load-triggered lower-word amomax.w breakpoint leaves the full doubleword, guard, adjacent, and marker words unchanged",
        trigger_preserved_words
    );

    goto_priv(PRIV_HS);
    TEST_SETUP_EXCEPT();
    uint32_t old_lower = amomax_w(lower_word_vaddr, final_operand);
    TEST_ASSERT("after clearing the load-only trigger, amomax.w returns the old lower word and signed-max updates only that 32-bit lane",
        excpt.triggered == false &&
        old_lower == lower_initial &&
        read64(target_vaddr) == expected_final &&
        read32(lower_word_vaddr) == expected_lower &&
        read32(upper_word_vaddr) == upper_initial
    );

    write64(marker_vaddr, marker_final);
    TEST_ASSERT("post-clear marker progress and non-target sentinels prove the amomax.w lower-word load-trigger breakpoint path did not poison following memory operations",
        read64(guard_vaddr) == guard_seed &&
        read64(adjacent_vaddr) == adjacent_seed &&
        read64(marker_vaddr) == marker_final
    );

    goto_priv(PRIV_M);
    TEST_ASSERT("backing memory shows only the post-clear lower-word amomax.w signed-max lane update plus marker progress, with upper lane, guard, and adjacent unchanged",
        read64(guard_paddr) == guard_seed &&
        read64(target_paddr) == expected_final &&
        read32(lower_word_paddr) == expected_lower &&
        read32(upper_word_paddr) == upper_initial &&
        read64(adjacent_paddr) == adjacent_seed &&
        read64(marker_paddr) == marker_final
    );

    TEST_END();
}

bool ai_micro_memblock_amomax_w_lower_word_store_trigger_breakpoint_suppresses_signed_max_lane_then_clear_succeeds_corner()
{
    TEST_START();

    const enum test_page page = VSRWX_GRWX;
    const uintptr_t base_vaddr = hs_page_base(page) + 0xc40U;
    const uintptr_t base_paddr = phys_page_base(page) + 0xc40U;
    const uintptr_t guard_vaddr = base_vaddr;
    const uintptr_t target_vaddr = base_vaddr + sizeof(uint64_t);
    const uintptr_t lower_word_vaddr = target_vaddr;
    const uintptr_t upper_word_vaddr = target_vaddr + sizeof(uint32_t);
    const uintptr_t adjacent_vaddr = base_vaddr + 2U * sizeof(uint64_t);
    const uintptr_t marker_vaddr = base_vaddr + 3U * sizeof(uint64_t);
    const uintptr_t guard_paddr = base_paddr;
    const uintptr_t target_paddr = base_paddr + sizeof(uint64_t);
    const uintptr_t lower_word_paddr = target_paddr;
    const uintptr_t upper_word_paddr = target_paddr + sizeof(uint32_t);
    const uintptr_t adjacent_paddr = base_paddr + 2U * sizeof(uint64_t);
    const uintptr_t marker_paddr = base_paddr + 3U * sizeof(uint64_t);
    const uint64_t guard_seed = 0x6d61786c7374726cULL;
    const uint32_t lower_initial = 0x80000008U;
    const uint32_t upper_initial = 0x3579bdf1U;
    const uint64_t initial_value =
        ((uint64_t)upper_initial << 32) | lower_initial;
    const uint64_t adjacent_seed = 0xa55a66cc33dd9900ULL;
    const uint64_t marker_seed = 0x0011223344556677ULL;
    const uint64_t marker_final = 0x7766554433221100ULL;
    const uint32_t blocked_operand = 0x7ffffff8U;
    const uint32_t final_operand = 0x7fffffffU;
    const uint32_t expected_lower = final_operand;
    const uint64_t expected_final =
        ((uint64_t)upper_initial << 32) | expected_lower;

    goto_priv(PRIV_M);
    write64(guard_paddr, guard_seed);
    write64(target_paddr, initial_value);
    write64(adjacent_paddr, adjacent_seed);
    write64(marker_paddr, marker_seed);

    goto_priv(PRIV_HS);
    hspt_init();
    sfence_vma();

    ai_memblock_lrsc_trigger_clear_mcontrol6_0();
    ai_memblock_lrsc_trigger_arm_mcontrol6_0(lower_word_vaddr,
        AI_MEMBLOCK_LRSC_TRIGGER_STORE);
    uint64_t saved_mstatus =
        ai_memblock_lrsc_trigger_enable_breakpoint_delivery();

    goto_priv(PRIV_HS);
    TEST_SETUP_EXCEPT();
    (void)amomax_w(lower_word_vaddr, blocked_operand);
    TEST_ASSERT("a store-only mcontrol6 address trigger armed on the lower word reports breakpoint on amomax.w before the signed-max lane write can commit",
        excpt.triggered == true &&
        excpt.cause == CAUSE_BKP
    );

    ai_memblock_lrsc_trigger_restore_machine_status(saved_mstatus);
    ai_memblock_lrsc_trigger_clear_mcontrol6_0();

    goto_priv(PRIV_M);
    bool trigger_preserved_words =
        read64(guard_paddr) == guard_seed &&
        read64(target_paddr) == initial_value &&
        read32(lower_word_paddr) == lower_initial &&
        read32(upper_word_paddr) == upper_initial &&
        read64(adjacent_paddr) == adjacent_seed &&
        read64(marker_paddr) == marker_seed;
    TEST_ASSERT("the store-triggered lower-word amomax.w breakpoint leaves the full doubleword, guard, adjacent, and marker words unchanged",
        trigger_preserved_words
    );

    goto_priv(PRIV_HS);
    TEST_SETUP_EXCEPT();
    uint32_t old_lower = amomax_w(lower_word_vaddr, final_operand);
    TEST_ASSERT("after clearing the store-only trigger, amomax.w returns the old lower word and signed-max updates only that 32-bit lane",
        excpt.triggered == false &&
        old_lower == lower_initial &&
        read64(target_vaddr) == expected_final &&
        read32(lower_word_vaddr) == expected_lower &&
        read32(upper_word_vaddr) == upper_initial
    );

    write64(marker_vaddr, marker_final);
    TEST_ASSERT("post-clear marker progress and non-target sentinels prove the amomax.w lower-word store-trigger breakpoint path did not poison following memory operations",
        read64(guard_vaddr) == guard_seed &&
        read64(adjacent_vaddr) == adjacent_seed &&
        read64(marker_vaddr) == marker_final
    );

    goto_priv(PRIV_M);
    TEST_ASSERT("backing memory shows only the post-clear lower-word amomax.w signed-max lane update plus marker progress, with upper lane, guard, and adjacent unchanged",
        read64(guard_paddr) == guard_seed &&
        read64(target_paddr) == expected_final &&
        read32(lower_word_paddr) == expected_lower &&
        read32(upper_word_paddr) == upper_initial &&
        read64(adjacent_paddr) == adjacent_seed &&
        read64(marker_paddr) == marker_final
    );

    TEST_END();
}

bool ai_micro_memblock_amoxor_w_lower_word_load_trigger_breakpoint_suppresses_logic_xor_lane_then_clear_succeeds_corner()
{
    TEST_START();

    const enum test_page page = VSRWX_GRWX;
    const uintptr_t base_vaddr = hs_page_base(page) + 0xc80U;
    const uintptr_t base_paddr = phys_page_base(page) + 0xc80U;
    const uintptr_t guard_vaddr = base_vaddr;
    const uintptr_t target_vaddr = base_vaddr + sizeof(uint64_t);
    const uintptr_t lower_word_vaddr = target_vaddr;
    const uintptr_t upper_word_vaddr = target_vaddr + sizeof(uint32_t);
    const uintptr_t adjacent_vaddr = base_vaddr + 2U * sizeof(uint64_t);
    const uintptr_t marker_vaddr = base_vaddr + 3U * sizeof(uint64_t);
    const uintptr_t guard_paddr = base_paddr;
    const uintptr_t target_paddr = base_paddr + sizeof(uint64_t);
    const uintptr_t lower_word_paddr = target_paddr;
    const uintptr_t upper_word_paddr = target_paddr + sizeof(uint32_t);
    const uintptr_t adjacent_paddr = base_paddr + 2U * sizeof(uint64_t);
    const uintptr_t marker_paddr = base_paddr + 3U * sizeof(uint64_t);
    const uint64_t guard_seed = 0x786f726c64726c64ULL;
    const uint32_t lower_initial = 0xaa55cc33U;
    const uint32_t upper_initial = 0xdeadbeefU;
    const uint64_t initial_value =
        ((uint64_t)upper_initial << 32) | lower_initial;
    const uint64_t adjacent_seed = 0x13579bdf2468ace0ULL;
    const uint64_t marker_seed = 0x0f1e2d3c4b5a6978ULL;
    const uint64_t marker_final = 0x87695a4b3c2d1e0fULL;
    const uint32_t blocked_operand = 0x55aa33ccU;
    const uint32_t final_operand = 0x0f0f00ffU;
    const uint32_t expected_lower = lower_initial ^ final_operand;
    const uint64_t expected_final =
        ((uint64_t)upper_initial << 32) | expected_lower;

    goto_priv(PRIV_M);
    write64(guard_paddr, guard_seed);
    write64(target_paddr, initial_value);
    write64(adjacent_paddr, adjacent_seed);
    write64(marker_paddr, marker_seed);

    goto_priv(PRIV_HS);
    hspt_init();
    sfence_vma();

    ai_memblock_lrsc_trigger_clear_mcontrol6_0();
    ai_memblock_lrsc_trigger_arm_mcontrol6_0(lower_word_vaddr,
        AI_MEMBLOCK_LRSC_TRIGGER_LOAD);
    uint64_t saved_mstatus =
        ai_memblock_lrsc_trigger_enable_breakpoint_delivery();

    goto_priv(PRIV_HS);
    TEST_SETUP_EXCEPT();
    (void)amoxor_w(lower_word_vaddr, blocked_operand);
    TEST_ASSERT("a load-only mcontrol6 address trigger armed on the lower word reports breakpoint on amoxor.w before the logic-xor lane RMW can commit",
        excpt.triggered == true &&
        excpt.cause == CAUSE_BKP
    );

    ai_memblock_lrsc_trigger_restore_machine_status(saved_mstatus);
    ai_memblock_lrsc_trigger_clear_mcontrol6_0();

    goto_priv(PRIV_M);
    bool trigger_preserved_words =
        read64(guard_paddr) == guard_seed &&
        read64(target_paddr) == initial_value &&
        read32(lower_word_paddr) == lower_initial &&
        read32(upper_word_paddr) == upper_initial &&
        read64(adjacent_paddr) == adjacent_seed &&
        read64(marker_paddr) == marker_seed;
    TEST_ASSERT("the load-triggered lower-word amoxor.w breakpoint leaves the full doubleword, guard, adjacent, and marker words unchanged",
        trigger_preserved_words
    );

    goto_priv(PRIV_HS);
    TEST_SETUP_EXCEPT();
    uint32_t old_lower = amoxor_w(lower_word_vaddr, final_operand);
    TEST_ASSERT("after clearing the load-only trigger, amoxor.w returns the old lower word and XOR updates only that 32-bit lane",
        excpt.triggered == false &&
        old_lower == lower_initial &&
        read64(target_vaddr) == expected_final &&
        read32(lower_word_vaddr) == expected_lower &&
        read32(upper_word_vaddr) == upper_initial
    );

    write64(marker_vaddr, marker_final);
    TEST_ASSERT("post-clear marker progress and non-target sentinels prove the amoxor.w lower-word load-trigger breakpoint path did not poison following memory operations",
        read64(guard_vaddr) == guard_seed &&
        read64(adjacent_vaddr) == adjacent_seed &&
        read64(marker_vaddr) == marker_final
    );

    goto_priv(PRIV_M);
    TEST_ASSERT("backing memory shows only the post-clear lower-word amoxor.w XOR lane update plus marker progress, with upper lane, guard, and adjacent unchanged",
        read64(guard_paddr) == guard_seed &&
        read64(target_paddr) == expected_final &&
        read32(lower_word_paddr) == expected_lower &&
        read32(upper_word_paddr) == upper_initial &&
        read64(adjacent_paddr) == adjacent_seed &&
        read64(marker_paddr) == marker_final
    );

    TEST_END();
}

bool ai_micro_memblock_amoxor_w_lower_word_store_trigger_breakpoint_suppresses_logic_xor_lane_then_clear_succeeds_corner()
{
    TEST_START();

    const enum test_page page = VSRWX_GRWX;
    const uintptr_t base_vaddr = hs_page_base(page) + 0xcc0U;
    const uintptr_t base_paddr = phys_page_base(page) + 0xcc0U;
    const uintptr_t guard_vaddr = base_vaddr;
    const uintptr_t target_vaddr = base_vaddr + sizeof(uint64_t);
    const uintptr_t lower_word_vaddr = target_vaddr;
    const uintptr_t upper_word_vaddr = target_vaddr + sizeof(uint32_t);
    const uintptr_t adjacent_vaddr = base_vaddr + 2U * sizeof(uint64_t);
    const uintptr_t marker_vaddr = base_vaddr + 3U * sizeof(uint64_t);
    const uintptr_t guard_paddr = base_paddr;
    const uintptr_t target_paddr = base_paddr + sizeof(uint64_t);
    const uintptr_t lower_word_paddr = target_paddr;
    const uintptr_t upper_word_paddr = target_paddr + sizeof(uint32_t);
    const uintptr_t adjacent_paddr = base_paddr + 2U * sizeof(uint64_t);
    const uintptr_t marker_paddr = base_paddr + 3U * sizeof(uint64_t);
    const uint64_t guard_seed = 0x786f726c73747264ULL;
    const uint32_t lower_initial = 0x55aa33ccU;
    const uint32_t upper_initial = 0xcafebabeU;
    const uint64_t initial_value =
        ((uint64_t)upper_initial << 32) | lower_initial;
    const uint64_t adjacent_seed = 0xf0e1d2c3b4a59687ULL;
    const uint64_t marker_seed = 0x1020304050607080ULL;
    const uint64_t marker_final = 0x8070605040302010ULL;
    const uint32_t blocked_operand = 0xaa55cc33U;
    const uint32_t final_operand = 0x00ff0f0fU;
    const uint32_t expected_lower = lower_initial ^ final_operand;
    const uint64_t expected_final =
        ((uint64_t)upper_initial << 32) | expected_lower;

    goto_priv(PRIV_M);
    write64(guard_paddr, guard_seed);
    write64(target_paddr, initial_value);
    write64(adjacent_paddr, adjacent_seed);
    write64(marker_paddr, marker_seed);

    goto_priv(PRIV_HS);
    hspt_init();
    sfence_vma();

    ai_memblock_lrsc_trigger_clear_mcontrol6_0();
    ai_memblock_lrsc_trigger_arm_mcontrol6_0(lower_word_vaddr,
        AI_MEMBLOCK_LRSC_TRIGGER_STORE);
    uint64_t saved_mstatus =
        ai_memblock_lrsc_trigger_enable_breakpoint_delivery();

    goto_priv(PRIV_HS);
    TEST_SETUP_EXCEPT();
    (void)amoxor_w(lower_word_vaddr, blocked_operand);
    TEST_ASSERT("a store-only mcontrol6 address trigger armed on the lower word reports breakpoint on amoxor.w before the logic-xor lane write can commit",
        excpt.triggered == true &&
        excpt.cause == CAUSE_BKP
    );

    ai_memblock_lrsc_trigger_restore_machine_status(saved_mstatus);
    ai_memblock_lrsc_trigger_clear_mcontrol6_0();

    goto_priv(PRIV_M);
    bool trigger_preserved_words =
        read64(guard_paddr) == guard_seed &&
        read64(target_paddr) == initial_value &&
        read32(lower_word_paddr) == lower_initial &&
        read32(upper_word_paddr) == upper_initial &&
        read64(adjacent_paddr) == adjacent_seed &&
        read64(marker_paddr) == marker_seed;
    TEST_ASSERT("the store-triggered lower-word amoxor.w breakpoint leaves the full doubleword, guard, adjacent, and marker words unchanged",
        trigger_preserved_words
    );

    goto_priv(PRIV_HS);
    TEST_SETUP_EXCEPT();
    uint32_t old_lower = amoxor_w(lower_word_vaddr, final_operand);
    TEST_ASSERT("after clearing the store-only trigger, amoxor.w returns the old lower word and XOR updates only that 32-bit lane",
        excpt.triggered == false &&
        old_lower == lower_initial &&
        read64(target_vaddr) == expected_final &&
        read32(lower_word_vaddr) == expected_lower &&
        read32(upper_word_vaddr) == upper_initial
    );

    write64(marker_vaddr, marker_final);
    TEST_ASSERT("post-clear marker progress and non-target sentinels prove the amoxor.w lower-word store-trigger breakpoint path did not poison following memory operations",
        read64(guard_vaddr) == guard_seed &&
        read64(adjacent_vaddr) == adjacent_seed &&
        read64(marker_vaddr) == marker_final
    );

    goto_priv(PRIV_M);
    TEST_ASSERT("backing memory shows only the post-clear lower-word amoxor.w XOR lane update plus marker progress, with upper lane, guard, and adjacent unchanged",
        read64(guard_paddr) == guard_seed &&
        read64(target_paddr) == expected_final &&
        read32(lower_word_paddr) == expected_lower &&
        read32(upper_word_paddr) == upper_initial &&
        read64(adjacent_paddr) == adjacent_seed &&
        read64(marker_paddr) == marker_final
    );

    TEST_END();
}

bool ai_micro_memblock_amoxor_w_upper_word_load_trigger_breakpoint_suppresses_logic_xor_lane_then_clear_succeeds_corner()
{
    TEST_START();

    const enum test_page page = VSRWX_GRWX;
    const uintptr_t base_vaddr = hs_page_base(page) + 0xd00U;
    const uintptr_t base_paddr = phys_page_base(page) + 0xd00U;
    const uintptr_t guard_vaddr = base_vaddr;
    const uintptr_t target_vaddr = base_vaddr + sizeof(uint64_t);
    const uintptr_t lower_word_vaddr = target_vaddr;
    const uintptr_t upper_word_vaddr = target_vaddr + sizeof(uint32_t);
    const uintptr_t adjacent_vaddr = base_vaddr + 2U * sizeof(uint64_t);
    const uintptr_t marker_vaddr = base_vaddr + 3U * sizeof(uint64_t);
    const uintptr_t guard_paddr = base_paddr;
    const uintptr_t target_paddr = base_paddr + sizeof(uint64_t);
    const uintptr_t lower_word_paddr = target_paddr;
    const uintptr_t upper_word_paddr = target_paddr + sizeof(uint32_t);
    const uintptr_t adjacent_paddr = base_paddr + 2U * sizeof(uint64_t);
    const uintptr_t marker_paddr = base_paddr + 3U * sizeof(uint64_t);
    const uint64_t guard_seed = 0x786f7275706c6472ULL;
    const uint32_t lower_initial = 0x2468ace0U;
    const uint32_t upper_initial = 0xaa55cc33U;
    const uint64_t initial_value =
        ((uint64_t)upper_initial << 32) | lower_initial;
    const uint64_t adjacent_seed = 0x55aa33cc13579bdfULL;
    const uint64_t marker_seed = 0x1a2b3c4d5e6f7081ULL;
    const uint64_t marker_final = 0x81706f5e4d3c2b1aULL;
    const uint32_t blocked_operand = 0x55aa33ccU;
    const uint32_t final_operand = 0x0f0f00ffU;
    const uint32_t expected_upper = upper_initial ^ final_operand;
    const uint64_t expected_final =
        ((uint64_t)expected_upper << 32) | lower_initial;

    goto_priv(PRIV_M);
    write64(guard_paddr, guard_seed);
    write64(target_paddr, initial_value);
    write64(adjacent_paddr, adjacent_seed);
    write64(marker_paddr, marker_seed);

    goto_priv(PRIV_HS);
    hspt_init();
    sfence_vma();

    ai_memblock_lrsc_trigger_clear_mcontrol6_0();
    ai_memblock_lrsc_trigger_arm_mcontrol6_0(upper_word_vaddr,
        AI_MEMBLOCK_LRSC_TRIGGER_LOAD);
    uint64_t saved_mstatus =
        ai_memblock_lrsc_trigger_enable_breakpoint_delivery();

    goto_priv(PRIV_HS);
    TEST_SETUP_EXCEPT();
    (void)amoxor_w(upper_word_vaddr, blocked_operand);
    TEST_ASSERT("a load-only mcontrol6 address trigger armed on the upper word reports breakpoint on amoxor.w before the logic-xor lane RMW can commit",
        excpt.triggered == true &&
        excpt.cause == CAUSE_BKP
    );

    ai_memblock_lrsc_trigger_restore_machine_status(saved_mstatus);
    ai_memblock_lrsc_trigger_clear_mcontrol6_0();

    goto_priv(PRIV_M);
    bool trigger_preserved_words =
        read64(guard_paddr) == guard_seed &&
        read64(target_paddr) == initial_value &&
        read32(lower_word_paddr) == lower_initial &&
        read32(upper_word_paddr) == upper_initial &&
        read64(adjacent_paddr) == adjacent_seed &&
        read64(marker_paddr) == marker_seed;
    TEST_ASSERT("the load-triggered upper-word amoxor.w breakpoint leaves the full doubleword, guard, adjacent, and marker words unchanged",
        trigger_preserved_words
    );

    goto_priv(PRIV_HS);
    TEST_SETUP_EXCEPT();
    uint32_t old_upper = amoxor_w(upper_word_vaddr, final_operand);
    TEST_ASSERT("after clearing the load-only trigger, amoxor.w returns the old upper word and XOR updates only that 32-bit lane",
        excpt.triggered == false &&
        old_upper == upper_initial &&
        read64(target_vaddr) == expected_final &&
        read32(lower_word_vaddr) == lower_initial &&
        read32(upper_word_vaddr) == expected_upper
    );

    write64(marker_vaddr, marker_final);
    TEST_ASSERT("post-clear marker progress and non-target sentinels prove the amoxor.w upper-word load-trigger breakpoint path did not poison following memory operations",
        read64(guard_vaddr) == guard_seed &&
        read64(adjacent_vaddr) == adjacent_seed &&
        read64(marker_vaddr) == marker_final
    );

    goto_priv(PRIV_M);
    TEST_ASSERT("backing memory shows only the post-clear upper-word amoxor.w XOR lane update plus marker progress, with lower lane, guard, and adjacent unchanged",
        read64(guard_paddr) == guard_seed &&
        read64(target_paddr) == expected_final &&
        read32(lower_word_paddr) == lower_initial &&
        read32(upper_word_paddr) == expected_upper &&
        read64(adjacent_paddr) == adjacent_seed &&
        read64(marker_paddr) == marker_final
    );

    TEST_END();
}

bool ai_micro_memblock_amoxor_w_upper_word_store_trigger_breakpoint_suppresses_logic_xor_lane_then_clear_succeeds_corner()
{
    TEST_START();

    const enum test_page page = VSRWX_GRWX;
    const uintptr_t base_vaddr = hs_page_base(page) + 0xd40U;
    const uintptr_t base_paddr = phys_page_base(page) + 0xd40U;
    const uintptr_t guard_vaddr = base_vaddr;
    const uintptr_t target_vaddr = base_vaddr + sizeof(uint64_t);
    const uintptr_t lower_word_vaddr = target_vaddr;
    const uintptr_t upper_word_vaddr = target_vaddr + sizeof(uint32_t);
    const uintptr_t adjacent_vaddr = base_vaddr + 2U * sizeof(uint64_t);
    const uintptr_t marker_vaddr = base_vaddr + 3U * sizeof(uint64_t);
    const uintptr_t guard_paddr = base_paddr;
    const uintptr_t target_paddr = base_paddr + sizeof(uint64_t);
    const uintptr_t lower_word_paddr = target_paddr;
    const uintptr_t upper_word_paddr = target_paddr + sizeof(uint32_t);
    const uintptr_t adjacent_paddr = base_paddr + 2U * sizeof(uint64_t);
    const uintptr_t marker_paddr = base_paddr + 3U * sizeof(uint64_t);
    const uint64_t guard_seed = 0x786f727570737472ULL;
    const uint32_t lower_initial = 0x13579bdfU;
    const uint32_t upper_initial = 0x55aa33ccU;
    const uint64_t initial_value =
        ((uint64_t)upper_initial << 32) | lower_initial;
    const uint64_t adjacent_seed = 0x2468ace0f1e2d3c4ULL;
    const uint64_t marker_seed = 0x010f203e4d5c6b7aULL;
    const uint64_t marker_final = 0x7a6b5c4d3e200f01ULL;
    const uint32_t blocked_operand = 0xaa55cc33U;
    const uint32_t final_operand = 0x00ff0f0fU;
    const uint32_t expected_upper = upper_initial ^ final_operand;
    const uint64_t expected_final =
        ((uint64_t)expected_upper << 32) | lower_initial;

    goto_priv(PRIV_M);
    write64(guard_paddr, guard_seed);
    write64(target_paddr, initial_value);
    write64(adjacent_paddr, adjacent_seed);
    write64(marker_paddr, marker_seed);

    goto_priv(PRIV_HS);
    hspt_init();
    sfence_vma();

    ai_memblock_lrsc_trigger_clear_mcontrol6_0();
    ai_memblock_lrsc_trigger_arm_mcontrol6_0(upper_word_vaddr,
        AI_MEMBLOCK_LRSC_TRIGGER_STORE);
    uint64_t saved_mstatus =
        ai_memblock_lrsc_trigger_enable_breakpoint_delivery();

    goto_priv(PRIV_HS);
    TEST_SETUP_EXCEPT();
    (void)amoxor_w(upper_word_vaddr, blocked_operand);
    TEST_ASSERT("a store-only mcontrol6 address trigger armed on the upper word reports breakpoint on amoxor.w before the logic-xor lane write can commit",
        excpt.triggered == true &&
        excpt.cause == CAUSE_BKP
    );

    ai_memblock_lrsc_trigger_restore_machine_status(saved_mstatus);
    ai_memblock_lrsc_trigger_clear_mcontrol6_0();

    goto_priv(PRIV_M);
    bool trigger_preserved_words =
        read64(guard_paddr) == guard_seed &&
        read64(target_paddr) == initial_value &&
        read32(lower_word_paddr) == lower_initial &&
        read32(upper_word_paddr) == upper_initial &&
        read64(adjacent_paddr) == adjacent_seed &&
        read64(marker_paddr) == marker_seed;
    TEST_ASSERT("the store-triggered upper-word amoxor.w breakpoint leaves the full doubleword, guard, adjacent, and marker words unchanged",
        trigger_preserved_words
    );

    goto_priv(PRIV_HS);
    TEST_SETUP_EXCEPT();
    uint32_t old_upper = amoxor_w(upper_word_vaddr, final_operand);
    TEST_ASSERT("after clearing the store-only trigger, amoxor.w returns the old upper word and XOR updates only that 32-bit lane",
        excpt.triggered == false &&
        old_upper == upper_initial &&
        read64(target_vaddr) == expected_final &&
        read32(lower_word_vaddr) == lower_initial &&
        read32(upper_word_vaddr) == expected_upper
    );

    write64(marker_vaddr, marker_final);
    TEST_ASSERT("post-clear marker progress and non-target sentinels prove the amoxor.w upper-word store-trigger breakpoint path did not poison following memory operations",
        read64(guard_vaddr) == guard_seed &&
        read64(adjacent_vaddr) == adjacent_seed &&
        read64(marker_vaddr) == marker_final
    );

    goto_priv(PRIV_M);
    TEST_ASSERT("backing memory shows only the post-clear upper-word amoxor.w XOR lane update plus marker progress, with lower lane, guard, and adjacent unchanged",
        read64(guard_paddr) == guard_seed &&
        read64(target_paddr) == expected_final &&
        read32(lower_word_paddr) == lower_initial &&
        read32(upper_word_paddr) == expected_upper &&
        read64(adjacent_paddr) == adjacent_seed &&
        read64(marker_paddr) == marker_final
    );

    TEST_END();
}

bool ai_micro_memblock_amoor_w_lower_word_load_trigger_breakpoint_suppresses_logic_or_lane_then_clear_succeeds_corner()
{
    TEST_START();

    const enum test_page page = VSRWX_GRWX;
    const uintptr_t base_vaddr = hs_page_base(page) + 0xd80U;
    const uintptr_t base_paddr = phys_page_base(page) + 0xd80U;
    const uintptr_t guard_vaddr = base_vaddr;
    const uintptr_t target_vaddr = base_vaddr + sizeof(uint64_t);
    const uintptr_t lower_word_vaddr = target_vaddr;
    const uintptr_t upper_word_vaddr = target_vaddr + sizeof(uint32_t);
    const uintptr_t adjacent_vaddr = base_vaddr + 2U * sizeof(uint64_t);
    const uintptr_t marker_vaddr = base_vaddr + 3U * sizeof(uint64_t);
    const uintptr_t guard_paddr = base_paddr;
    const uintptr_t target_paddr = base_paddr + sizeof(uint64_t);
    const uintptr_t lower_word_paddr = target_paddr;
    const uintptr_t upper_word_paddr = target_paddr + sizeof(uint32_t);
    const uintptr_t adjacent_paddr = base_paddr + 2U * sizeof(uint64_t);
    const uintptr_t marker_paddr = base_paddr + 3U * sizeof(uint64_t);
    const uint64_t guard_seed = 0x6f726c64726c6472ULL;
    const uint32_t lower_initial = 0x12003400U;
    const uint32_t upper_initial = 0xcafebabeU;
    const uint64_t initial_value =
        ((uint64_t)upper_initial << 32) | lower_initial;
    const uint64_t adjacent_seed = 0x0f0e0d0c0b0a0908ULL;
    const uint64_t marker_seed = 0x1122448855aa33ccULL;
    const uint64_t marker_final = 0xcc33aa5588442211ULL;
    const uint32_t blocked_operand = 0x00f0004fU;
    const uint32_t final_operand = 0x0000c0ffU;
    const uint32_t expected_lower = lower_initial | final_operand;
    const uint64_t expected_final =
        ((uint64_t)upper_initial << 32) | expected_lower;

    goto_priv(PRIV_M);
    write64(guard_paddr, guard_seed);
    write64(target_paddr, initial_value);
    write64(adjacent_paddr, adjacent_seed);
    write64(marker_paddr, marker_seed);

    goto_priv(PRIV_HS);
    hspt_init();
    sfence_vma();

    ai_memblock_lrsc_trigger_clear_mcontrol6_0();
    ai_memblock_lrsc_trigger_arm_mcontrol6_0(lower_word_vaddr,
        AI_MEMBLOCK_LRSC_TRIGGER_LOAD);
    uint64_t saved_mstatus =
        ai_memblock_lrsc_trigger_enable_breakpoint_delivery();

    goto_priv(PRIV_HS);
    TEST_SETUP_EXCEPT();
    (void)amoor_w(lower_word_vaddr, blocked_operand);
    TEST_ASSERT("a load-only mcontrol6 address trigger armed on the lower word reports breakpoint on amoor.w before the logic-or lane RMW can commit",
        excpt.triggered == true &&
        excpt.cause == CAUSE_BKP
    );

    ai_memblock_lrsc_trigger_restore_machine_status(saved_mstatus);
    ai_memblock_lrsc_trigger_clear_mcontrol6_0();

    goto_priv(PRIV_M);
    bool trigger_preserved_words =
        read64(guard_paddr) == guard_seed &&
        read64(target_paddr) == initial_value &&
        read32(lower_word_paddr) == lower_initial &&
        read32(upper_word_paddr) == upper_initial &&
        read64(adjacent_paddr) == adjacent_seed &&
        read64(marker_paddr) == marker_seed;
    TEST_ASSERT("the load-triggered lower-word amoor.w breakpoint leaves the full doubleword, guard, adjacent, and marker words unchanged",
        trigger_preserved_words
    );

    goto_priv(PRIV_HS);
    TEST_SETUP_EXCEPT();
    uint32_t old_lower = amoor_w(lower_word_vaddr, final_operand);
    TEST_ASSERT("after clearing the load-only trigger, amoor.w returns the old lower word and OR updates only that 32-bit lane",
        excpt.triggered == false &&
        old_lower == lower_initial &&
        read64(target_vaddr) == expected_final &&
        read32(lower_word_vaddr) == expected_lower &&
        read32(upper_word_vaddr) == upper_initial
    );

    write64(marker_vaddr, marker_final);
    TEST_ASSERT("post-clear marker progress and non-target sentinels prove the amoor.w lower-word load-trigger breakpoint path did not poison following memory operations",
        read64(guard_vaddr) == guard_seed &&
        read64(adjacent_vaddr) == adjacent_seed &&
        read64(marker_vaddr) == marker_final
    );

    goto_priv(PRIV_M);
    TEST_ASSERT("backing memory shows only the post-clear lower-word amoor.w OR lane update plus marker progress, with upper lane, guard, and adjacent unchanged",
        read64(guard_paddr) == guard_seed &&
        read64(target_paddr) == expected_final &&
        read32(lower_word_paddr) == expected_lower &&
        read32(upper_word_paddr) == upper_initial &&
        read64(adjacent_paddr) == adjacent_seed &&
        read64(marker_paddr) == marker_final
    );

    TEST_END();
}

bool ai_micro_memblock_amoor_w_lower_word_store_trigger_breakpoint_suppresses_logic_or_lane_then_clear_succeeds_corner()
{
    TEST_START();

    const enum test_page page = VSRWX_GRWX;
    const uintptr_t base_vaddr = hs_page_base(page) + 0xdc0U;
    const uintptr_t base_paddr = phys_page_base(page) + 0xdc0U;
    const uintptr_t guard_vaddr = base_vaddr;
    const uintptr_t target_vaddr = base_vaddr + sizeof(uint64_t);
    const uintptr_t lower_word_vaddr = target_vaddr;
    const uintptr_t upper_word_vaddr = target_vaddr + sizeof(uint32_t);
    const uintptr_t adjacent_vaddr = base_vaddr + 2U * sizeof(uint64_t);
    const uintptr_t marker_vaddr = base_vaddr + 3U * sizeof(uint64_t);
    const uintptr_t guard_paddr = base_paddr;
    const uintptr_t target_paddr = base_paddr + sizeof(uint64_t);
    const uintptr_t lower_word_paddr = target_paddr;
    const uintptr_t upper_word_paddr = target_paddr + sizeof(uint32_t);
    const uintptr_t adjacent_paddr = base_paddr + 2U * sizeof(uint64_t);
    const uintptr_t marker_paddr = base_paddr + 3U * sizeof(uint64_t);
    const uint64_t guard_seed = 0x6f727374726c6472ULL;
    const uint32_t lower_initial = 0x00123040U;
    const uint32_t upper_initial = 0x55aa33ccU;
    const uint64_t initial_value =
        ((uint64_t)upper_initial << 32) | lower_initial;
    const uint64_t adjacent_seed = 0xfedcba9876543210ULL;
    const uint64_t marker_seed = 0x0123456789abcdefULL;
    const uint64_t marker_final = 0xfedcba9876543210ULL;
    const uint32_t blocked_operand = 0x0f0f00ffU;
    const uint32_t final_operand = 0x00f0c00fU;
    const uint32_t expected_lower = lower_initial | final_operand;
    const uint64_t expected_final =
        ((uint64_t)upper_initial << 32) | expected_lower;

    goto_priv(PRIV_M);
    write64(guard_paddr, guard_seed);
    write64(target_paddr, initial_value);
    write64(adjacent_paddr, adjacent_seed);
    write64(marker_paddr, marker_seed);

    goto_priv(PRIV_HS);
    hspt_init();
    sfence_vma();

    ai_memblock_lrsc_trigger_clear_mcontrol6_0();
    ai_memblock_lrsc_trigger_arm_mcontrol6_0(lower_word_vaddr,
        AI_MEMBLOCK_LRSC_TRIGGER_STORE);
    uint64_t saved_mstatus =
        ai_memblock_lrsc_trigger_enable_breakpoint_delivery();

    goto_priv(PRIV_HS);
    TEST_SETUP_EXCEPT();
    (void)amoor_w(lower_word_vaddr, blocked_operand);
    TEST_ASSERT("a store-only mcontrol6 address trigger armed on the lower word reports breakpoint on amoor.w before the logic-or lane write can commit",
        excpt.triggered == true &&
        excpt.cause == CAUSE_BKP
    );

    ai_memblock_lrsc_trigger_restore_machine_status(saved_mstatus);
    ai_memblock_lrsc_trigger_clear_mcontrol6_0();

    goto_priv(PRIV_M);
    bool trigger_preserved_words =
        read64(guard_paddr) == guard_seed &&
        read64(target_paddr) == initial_value &&
        read32(lower_word_paddr) == lower_initial &&
        read32(upper_word_paddr) == upper_initial &&
        read64(adjacent_paddr) == adjacent_seed &&
        read64(marker_paddr) == marker_seed;
    TEST_ASSERT("the store-triggered lower-word amoor.w breakpoint leaves the full doubleword, guard, adjacent, and marker words unchanged",
        trigger_preserved_words
    );

    goto_priv(PRIV_HS);
    TEST_SETUP_EXCEPT();
    uint32_t old_lower = amoor_w(lower_word_vaddr, final_operand);
    TEST_ASSERT("after clearing the store-only trigger, amoor.w returns the old lower word and OR updates only that 32-bit lane",
        excpt.triggered == false &&
        old_lower == lower_initial &&
        read64(target_vaddr) == expected_final &&
        read32(lower_word_vaddr) == expected_lower &&
        read32(upper_word_vaddr) == upper_initial
    );

    write64(marker_vaddr, marker_final);
    TEST_ASSERT("post-clear marker progress and non-target sentinels prove the amoor.w lower-word store-trigger breakpoint path did not poison following memory operations",
        read64(guard_vaddr) == guard_seed &&
        read64(adjacent_vaddr) == adjacent_seed &&
        read64(marker_vaddr) == marker_final
    );

    goto_priv(PRIV_M);
    TEST_ASSERT("backing memory shows only the post-clear lower-word amoor.w OR lane update plus marker progress, with upper lane, guard, and adjacent unchanged",
        read64(guard_paddr) == guard_seed &&
        read64(target_paddr) == expected_final &&
        read32(lower_word_paddr) == expected_lower &&
        read32(upper_word_paddr) == upper_initial &&
        read64(adjacent_paddr) == adjacent_seed &&
        read64(marker_paddr) == marker_final
    );

    TEST_END();
}

bool ai_micro_memblock_amoor_w_upper_word_load_trigger_breakpoint_suppresses_logic_or_lane_then_clear_succeeds_corner()
{
    TEST_START();

    const enum test_page page = VSRWX_GRWX;
    const uintptr_t base_vaddr = hs_page_base(page) + 0xe00U;
    const uintptr_t base_paddr = phys_page_base(page) + 0xe00U;
    const uintptr_t guard_vaddr = base_vaddr;
    const uintptr_t target_vaddr = base_vaddr + sizeof(uint64_t);
    const uintptr_t lower_word_vaddr = target_vaddr;
    const uintptr_t upper_word_vaddr = target_vaddr + sizeof(uint32_t);
    const uintptr_t adjacent_vaddr = base_vaddr + 2U * sizeof(uint64_t);
    const uintptr_t marker_vaddr = base_vaddr + 3U * sizeof(uint64_t);
    const uintptr_t guard_paddr = base_paddr;
    const uintptr_t target_paddr = base_paddr + sizeof(uint64_t);
    const uintptr_t lower_word_paddr = target_paddr;
    const uintptr_t upper_word_paddr = target_paddr + sizeof(uint32_t);
    const uintptr_t adjacent_paddr = base_paddr + 2U * sizeof(uint64_t);
    const uintptr_t marker_paddr = base_paddr + 3U * sizeof(uint64_t);
    const uint64_t guard_seed = 0x6f726c6472757070ULL;
    const uint32_t lower_initial = 0x2468ace0U;
    const uint32_t upper_initial = 0x12003400U;
    const uint64_t initial_value =
        ((uint64_t)upper_initial << 32) | lower_initial;
    const uint64_t adjacent_seed = 0x13579bdf2468ace0ULL;
    const uint64_t marker_seed = 0x0ddc0ffe55aa7788ULL;
    const uint64_t marker_final = 0x8877aa55fe0fcdd0ULL;
    const uint32_t blocked_operand = 0x00f0004fU;
    const uint32_t final_operand = 0x0000c0ffU;
    const uint32_t expected_upper = upper_initial | final_operand;
    const uint64_t expected_final =
        ((uint64_t)expected_upper << 32) | lower_initial;

    goto_priv(PRIV_M);
    write64(guard_paddr, guard_seed);
    write64(target_paddr, initial_value);
    write64(adjacent_paddr, adjacent_seed);
    write64(marker_paddr, marker_seed);

    goto_priv(PRIV_HS);
    hspt_init();
    sfence_vma();

    ai_memblock_lrsc_trigger_clear_mcontrol6_0();
    ai_memblock_lrsc_trigger_arm_mcontrol6_0(upper_word_vaddr,
        AI_MEMBLOCK_LRSC_TRIGGER_LOAD);
    uint64_t saved_mstatus =
        ai_memblock_lrsc_trigger_enable_breakpoint_delivery();

    goto_priv(PRIV_HS);
    TEST_SETUP_EXCEPT();
    (void)amoor_w(upper_word_vaddr, blocked_operand);
    TEST_ASSERT("a load-only mcontrol6 address trigger armed on the upper word reports breakpoint on amoor.w before the logic-or lane RMW can commit",
        excpt.triggered == true &&
        excpt.cause == CAUSE_BKP
    );

    ai_memblock_lrsc_trigger_restore_machine_status(saved_mstatus);
    ai_memblock_lrsc_trigger_clear_mcontrol6_0();

    goto_priv(PRIV_M);
    bool trigger_preserved_words =
        read64(guard_paddr) == guard_seed &&
        read64(target_paddr) == initial_value &&
        read32(lower_word_paddr) == lower_initial &&
        read32(upper_word_paddr) == upper_initial &&
        read64(adjacent_paddr) == adjacent_seed &&
        read64(marker_paddr) == marker_seed;
    TEST_ASSERT("the load-triggered upper-word amoor.w breakpoint leaves the full doubleword, guard, adjacent, and marker words unchanged",
        trigger_preserved_words
    );

    goto_priv(PRIV_HS);
    TEST_SETUP_EXCEPT();
    uint32_t old_upper = amoor_w(upper_word_vaddr, final_operand);
    TEST_ASSERT("after clearing the load-only trigger, amoor.w returns the old upper word and OR updates only that 32-bit lane",
        excpt.triggered == false &&
        old_upper == upper_initial &&
        read64(target_vaddr) == expected_final &&
        read32(lower_word_vaddr) == lower_initial &&
        read32(upper_word_vaddr) == expected_upper
    );

    write64(marker_vaddr, marker_final);
    TEST_ASSERT("post-clear marker progress and non-target sentinels prove the amoor.w upper-word load-trigger breakpoint path did not poison following memory operations",
        read64(guard_vaddr) == guard_seed &&
        read64(adjacent_vaddr) == adjacent_seed &&
        read64(marker_vaddr) == marker_final
    );

    goto_priv(PRIV_M);
    TEST_ASSERT("backing memory shows only the post-clear upper-word amoor.w OR lane update plus marker progress, with lower lane, guard, and adjacent unchanged",
        read64(guard_paddr) == guard_seed &&
        read64(target_paddr) == expected_final &&
        read32(lower_word_paddr) == lower_initial &&
        read32(upper_word_paddr) == expected_upper &&
        read64(adjacent_paddr) == adjacent_seed &&
        read64(marker_paddr) == marker_final
    );

    TEST_END();
}

bool ai_micro_memblock_amoand_w_upper_word_store_trigger_breakpoint_suppresses_logic_and_lane_then_clear_succeeds_corner()
{
    TEST_START();

    const enum test_page page = VSRWX_GRWX;
    const uintptr_t base_vaddr = hs_page_base(page) + 0xe40U;
    const uintptr_t base_paddr = phys_page_base(page) + 0xe40U;
    const uintptr_t guard_vaddr = base_vaddr;
    const uintptr_t target_vaddr = base_vaddr + sizeof(uint64_t);
    const uintptr_t lower_word_vaddr = target_vaddr;
    const uintptr_t upper_word_vaddr = target_vaddr + sizeof(uint32_t);
    const uintptr_t adjacent_vaddr = base_vaddr + 2U * sizeof(uint64_t);
    const uintptr_t marker_vaddr = base_vaddr + 3U * sizeof(uint64_t);
    const uintptr_t guard_paddr = base_paddr;
    const uintptr_t target_paddr = base_paddr + sizeof(uint64_t);
    const uintptr_t lower_word_paddr = target_paddr;
    const uintptr_t upper_word_paddr = target_paddr + sizeof(uint32_t);
    const uintptr_t adjacent_paddr = base_paddr + 2U * sizeof(uint64_t);
    const uintptr_t marker_paddr = base_paddr + 3U * sizeof(uint64_t);
    const uint64_t guard_seed = 0x616e647573747270ULL;
    const uint32_t lower_initial = 0x89abcdefU;
    const uint32_t upper_initial = 0xff0f5aa5U;
    const uint64_t initial_value =
        ((uint64_t)upper_initial << 32) | lower_initial;
    const uint64_t adjacent_seed = 0xf0e1d2c3b4a59687ULL;
    const uint64_t marker_seed = 0x1020304050607080ULL;
    const uint64_t marker_final = 0x8070605040302010ULL;
    const uint32_t blocked_operand = 0x00ff00ffU;
    const uint32_t final_operand = 0x0ff0ff00U;
    const uint32_t expected_upper = upper_initial & final_operand;
    const uint64_t expected_final =
        ((uint64_t)expected_upper << 32) | lower_initial;

    goto_priv(PRIV_M);
    write64(guard_paddr, guard_seed);
    write64(target_paddr, initial_value);
    write64(adjacent_paddr, adjacent_seed);
    write64(marker_paddr, marker_seed);

    goto_priv(PRIV_HS);
    hspt_init();
    sfence_vma();

    ai_memblock_lrsc_trigger_clear_mcontrol6_0();
    ai_memblock_lrsc_trigger_arm_mcontrol6_0(upper_word_vaddr,
        AI_MEMBLOCK_LRSC_TRIGGER_STORE);
    uint64_t saved_mstatus =
        ai_memblock_lrsc_trigger_enable_breakpoint_delivery();

    goto_priv(PRIV_HS);
    TEST_SETUP_EXCEPT();
    (void)amoand_w(upper_word_vaddr, blocked_operand);
    TEST_ASSERT("a store-only mcontrol6 address trigger armed on the upper word reports breakpoint on amoand.w before the logic-and lane write can commit",
        excpt.triggered == true &&
        excpt.cause == CAUSE_BKP
    );

    ai_memblock_lrsc_trigger_restore_machine_status(saved_mstatus);
    ai_memblock_lrsc_trigger_clear_mcontrol6_0();

    goto_priv(PRIV_M);
    bool trigger_preserved_words =
        read64(guard_paddr) == guard_seed &&
        read64(target_paddr) == initial_value &&
        read32(lower_word_paddr) == lower_initial &&
        read32(upper_word_paddr) == upper_initial &&
        read64(adjacent_paddr) == adjacent_seed &&
        read64(marker_paddr) == marker_seed;
    TEST_ASSERT("the store-triggered upper-word amoand.w breakpoint leaves the full doubleword, guard, adjacent, and marker words unchanged",
        trigger_preserved_words
    );

    goto_priv(PRIV_HS);
    TEST_SETUP_EXCEPT();
    uint32_t old_upper = amoand_w(upper_word_vaddr, final_operand);
    TEST_ASSERT("after clearing the store-only trigger, amoand.w returns the old upper word and AND updates only that 32-bit lane",
        excpt.triggered == false &&
        old_upper == upper_initial &&
        read64(target_vaddr) == expected_final &&
        read32(lower_word_vaddr) == lower_initial &&
        read32(upper_word_vaddr) == expected_upper
    );

    write64(marker_vaddr, marker_final);
    TEST_ASSERT("post-clear marker progress and non-target sentinels prove the amoand.w upper-word store-trigger breakpoint path did not poison following memory operations",
        read64(guard_vaddr) == guard_seed &&
        read64(adjacent_vaddr) == adjacent_seed &&
        read64(marker_vaddr) == marker_final
    );

    goto_priv(PRIV_M);
    TEST_ASSERT("backing memory shows only the post-clear upper-word amoand.w AND lane update plus marker progress, with lower lane, guard, and adjacent unchanged",
        read64(guard_paddr) == guard_seed &&
        read64(target_paddr) == expected_final &&
        read32(lower_word_paddr) == lower_initial &&
        read32(upper_word_paddr) == expected_upper &&
        read64(adjacent_paddr) == adjacent_seed &&
        read64(marker_paddr) == marker_final
    );

    TEST_END();
}

bool ai_micro_memblock_amoor_w_upper_word_store_trigger_breakpoint_suppresses_logic_or_lane_then_clear_succeeds_corner()
{
    TEST_START();

    const enum test_page page = VSRWX_GRWX;
    const uintptr_t base_vaddr = hs_page_base(page) + 0xe80U;
    const uintptr_t base_paddr = phys_page_base(page) + 0xe80U;
    const uintptr_t guard_vaddr = base_vaddr;
    const uintptr_t target_vaddr = base_vaddr + sizeof(uint64_t);
    const uintptr_t lower_word_vaddr = target_vaddr;
    const uintptr_t upper_word_vaddr = target_vaddr + sizeof(uint32_t);
    const uintptr_t adjacent_vaddr = base_vaddr + 2U * sizeof(uint64_t);
    const uintptr_t marker_vaddr = base_vaddr + 3U * sizeof(uint64_t);
    const uintptr_t guard_paddr = base_paddr;
    const uintptr_t target_paddr = base_paddr + sizeof(uint64_t);
    const uintptr_t lower_word_paddr = target_paddr;
    const uintptr_t upper_word_paddr = target_paddr + sizeof(uint32_t);
    const uintptr_t adjacent_paddr = base_paddr + 2U * sizeof(uint64_t);
    const uintptr_t marker_paddr = base_paddr + 3U * sizeof(uint64_t);
    const uint64_t guard_seed = 0x6f72737475707072ULL;
    const uint32_t lower_initial = 0x10203040U;
    const uint32_t upper_initial = 0x12003400U;
    const uint64_t initial_value =
        ((uint64_t)upper_initial << 32) | lower_initial;
    const uint64_t adjacent_seed = 0x2436485a6c7e8092ULL;
    const uint64_t marker_seed = 0x0f1e2d3c4b5a6978ULL;
    const uint64_t marker_final = 0x78695a4b3c2d1e0fULL;
    const uint32_t blocked_operand = 0x00f0004fU;
    const uint32_t final_operand = 0x0000c0ffU;
    const uint32_t expected_upper = upper_initial | final_operand;
    const uint64_t expected_final =
        ((uint64_t)expected_upper << 32) | lower_initial;

    goto_priv(PRIV_M);
    write64(guard_paddr, guard_seed);
    write64(target_paddr, initial_value);
    write64(adjacent_paddr, adjacent_seed);
    write64(marker_paddr, marker_seed);

    goto_priv(PRIV_HS);
    hspt_init();
    sfence_vma();

    ai_memblock_lrsc_trigger_clear_mcontrol6_0();
    ai_memblock_lrsc_trigger_arm_mcontrol6_0(upper_word_vaddr,
        AI_MEMBLOCK_LRSC_TRIGGER_STORE);
    uint64_t saved_mstatus =
        ai_memblock_lrsc_trigger_enable_breakpoint_delivery();

    goto_priv(PRIV_HS);
    TEST_SETUP_EXCEPT();
    (void)amoor_w(upper_word_vaddr, blocked_operand);
    TEST_ASSERT("a store-only mcontrol6 address trigger armed on the upper word reports breakpoint on amoor.w before the logic-or lane write can commit",
        excpt.triggered == true &&
        excpt.cause == CAUSE_BKP
    );

    ai_memblock_lrsc_trigger_restore_machine_status(saved_mstatus);
    ai_memblock_lrsc_trigger_clear_mcontrol6_0();

    goto_priv(PRIV_M);
    bool trigger_preserved_words =
        read64(guard_paddr) == guard_seed &&
        read64(target_paddr) == initial_value &&
        read32(lower_word_paddr) == lower_initial &&
        read32(upper_word_paddr) == upper_initial &&
        read64(adjacent_paddr) == adjacent_seed &&
        read64(marker_paddr) == marker_seed;
    TEST_ASSERT("the store-triggered upper-word amoor.w breakpoint leaves the full doubleword, guard, adjacent, and marker words unchanged",
        trigger_preserved_words
    );

    goto_priv(PRIV_HS);
    TEST_SETUP_EXCEPT();
    uint32_t old_upper = amoor_w(upper_word_vaddr, final_operand);
    TEST_ASSERT("after clearing the store-only trigger, amoor.w returns the old upper word and OR updates only that 32-bit lane",
        excpt.triggered == false &&
        old_upper == upper_initial &&
        read64(target_vaddr) == expected_final &&
        read32(lower_word_vaddr) == lower_initial &&
        read32(upper_word_vaddr) == expected_upper
    );

    write64(marker_vaddr, marker_final);
    TEST_ASSERT("post-clear marker progress and non-target sentinels prove the amoor.w upper-word store-trigger breakpoint path did not poison following memory operations",
        read64(guard_vaddr) == guard_seed &&
        read64(adjacent_vaddr) == adjacent_seed &&
        read64(marker_vaddr) == marker_final
    );

    goto_priv(PRIV_M);
    TEST_ASSERT("backing memory shows only the post-clear upper-word amoor.w OR lane update plus marker progress, with lower lane, guard, and adjacent unchanged",
        read64(guard_paddr) == guard_seed &&
        read64(target_paddr) == expected_final &&
        read32(lower_word_paddr) == lower_initial &&
        read32(upper_word_paddr) == expected_upper &&
        read64(adjacent_paddr) == adjacent_seed &&
        read64(marker_paddr) == marker_final
    );

    TEST_END();
}

bool ai_micro_memblock_amoand_w_upper_word_load_trigger_breakpoint_suppresses_logic_and_lane_then_clear_succeeds_corner()
{
    TEST_START();

    const enum test_page page = VSRWX_GRWX;
    const uintptr_t base_vaddr = hs_page_base(page) + 0xec0U;
    const uintptr_t base_paddr = phys_page_base(page) + 0xec0U;
    const uintptr_t guard_vaddr = base_vaddr;
    const uintptr_t target_vaddr = base_vaddr + sizeof(uint64_t);
    const uintptr_t lower_word_vaddr = target_vaddr;
    const uintptr_t upper_word_vaddr = target_vaddr + sizeof(uint32_t);
    const uintptr_t adjacent_vaddr = base_vaddr + 2U * sizeof(uint64_t);
    const uintptr_t marker_vaddr = base_vaddr + 3U * sizeof(uint64_t);
    const uintptr_t guard_paddr = base_paddr;
    const uintptr_t target_paddr = base_paddr + sizeof(uint64_t);
    const uintptr_t lower_word_paddr = target_paddr;
    const uintptr_t upper_word_paddr = target_paddr + sizeof(uint32_t);
    const uintptr_t adjacent_paddr = base_paddr + 2U * sizeof(uint64_t);
    const uintptr_t marker_paddr = base_paddr + 3U * sizeof(uint64_t);
    const uint64_t guard_seed = 0x616e646c64757070ULL;
    const uint32_t lower_initial = 0x76543210U;
    const uint32_t upper_initial = 0xf0ff55aaU;
    const uint64_t initial_value =
        ((uint64_t)upper_initial << 32) | lower_initial;
    const uint64_t adjacent_seed = 0x9182736455463728ULL;
    const uint64_t marker_seed = 0x0123456789abcdefULL;
    const uint64_t marker_final = 0xfedcba9876543210ULL;
    const uint32_t blocked_operand = 0x00ff00ffU;
    const uint32_t final_operand = 0x0ff0ff00U;
    const uint32_t expected_upper = upper_initial & final_operand;
    const uint64_t expected_final =
        ((uint64_t)expected_upper << 32) | lower_initial;

    goto_priv(PRIV_M);
    write64(guard_paddr, guard_seed);
    write64(target_paddr, initial_value);
    write64(adjacent_paddr, adjacent_seed);
    write64(marker_paddr, marker_seed);

    goto_priv(PRIV_HS);
    hspt_init();
    sfence_vma();

    ai_memblock_lrsc_trigger_clear_mcontrol6_0();
    ai_memblock_lrsc_trigger_arm_mcontrol6_0(upper_word_vaddr,
        AI_MEMBLOCK_LRSC_TRIGGER_LOAD);
    uint64_t saved_mstatus =
        ai_memblock_lrsc_trigger_enable_breakpoint_delivery();

    goto_priv(PRIV_HS);
    TEST_SETUP_EXCEPT();
    (void)amoand_w(upper_word_vaddr, blocked_operand);
    TEST_ASSERT("a load-only mcontrol6 address trigger armed on the upper word reports breakpoint on amoand.w before the logic-and lane RMW can commit",
        excpt.triggered == true &&
        excpt.cause == CAUSE_BKP
    );

    ai_memblock_lrsc_trigger_restore_machine_status(saved_mstatus);
    ai_memblock_lrsc_trigger_clear_mcontrol6_0();

    goto_priv(PRIV_M);
    bool trigger_preserved_words =
        read64(guard_paddr) == guard_seed &&
        read64(target_paddr) == initial_value &&
        read32(lower_word_paddr) == lower_initial &&
        read32(upper_word_paddr) == upper_initial &&
        read64(adjacent_paddr) == adjacent_seed &&
        read64(marker_paddr) == marker_seed;
    TEST_ASSERT("the load-triggered upper-word amoand.w breakpoint leaves the full doubleword, guard, adjacent, and marker words unchanged",
        trigger_preserved_words
    );

    goto_priv(PRIV_HS);
    TEST_SETUP_EXCEPT();
    uint32_t old_upper = amoand_w(upper_word_vaddr, final_operand);
    TEST_ASSERT("after clearing the load-only trigger, amoand.w returns the old upper word and AND updates only that 32-bit lane",
        excpt.triggered == false &&
        old_upper == upper_initial &&
        read64(target_vaddr) == expected_final &&
        read32(lower_word_vaddr) == lower_initial &&
        read32(upper_word_vaddr) == expected_upper
    );

    write64(marker_vaddr, marker_final);
    TEST_ASSERT("post-clear marker progress and non-target sentinels prove the amoand.w upper-word load-trigger breakpoint path did not poison following memory operations",
        read64(guard_vaddr) == guard_seed &&
        read64(adjacent_vaddr) == adjacent_seed &&
        read64(marker_vaddr) == marker_final
    );

    goto_priv(PRIV_M);
    TEST_ASSERT("backing memory shows only the post-clear upper-word amoand.w AND lane update plus marker progress, with lower lane, guard, and adjacent unchanged",
        read64(guard_paddr) == guard_seed &&
        read64(target_paddr) == expected_final &&
        read32(lower_word_paddr) == lower_initial &&
        read32(upper_word_paddr) == expected_upper &&
        read64(adjacent_paddr) == adjacent_seed &&
        read64(marker_paddr) == marker_final
    );

    TEST_END();
}

bool ai_micro_memblock_amoand_w_lower_word_load_trigger_breakpoint_suppresses_logic_and_lane_then_clear_succeeds_corner()
{
    TEST_START();

    const enum test_page page = VSRWX_GRWX;
    const uintptr_t base_vaddr = hs_page_base(page) + 0xf00U;
    const uintptr_t base_paddr = phys_page_base(page) + 0xf00U;
    const uintptr_t guard_vaddr = base_vaddr;
    const uintptr_t target_vaddr = base_vaddr + sizeof(uint64_t);
    const uintptr_t lower_word_vaddr = target_vaddr;
    const uintptr_t upper_word_vaddr = target_vaddr + sizeof(uint32_t);
    const uintptr_t adjacent_vaddr = base_vaddr + 2U * sizeof(uint64_t);
    const uintptr_t marker_vaddr = base_vaddr + 3U * sizeof(uint64_t);
    const uintptr_t guard_paddr = base_paddr;
    const uintptr_t target_paddr = base_paddr + sizeof(uint64_t);
    const uintptr_t lower_word_paddr = target_paddr;
    const uintptr_t upper_word_paddr = target_paddr + sizeof(uint32_t);
    const uintptr_t adjacent_paddr = base_paddr + 2U * sizeof(uint64_t);
    const uintptr_t marker_paddr = base_paddr + 3U * sizeof(uint64_t);
    const uint64_t guard_seed = 0x616e646c64726c64ULL;
    const uint32_t lower_initial = 0xf0ff55aaU;
    const uint32_t upper_initial = 0x76543210U;
    const uint64_t initial_value =
        ((uint64_t)upper_initial << 32) | lower_initial;
    const uint64_t adjacent_seed = 0xa5b4c3d2e1f08796ULL;
    const uint64_t marker_seed = 0x1029384756657483ULL;
    const uint64_t marker_final = 0x8374655647382910ULL;
    const uint32_t blocked_operand = 0x00ff00ffU;
    const uint32_t final_operand = 0x0ff0ff00U;
    const uint32_t expected_lower = lower_initial & final_operand;
    const uint64_t expected_final =
        ((uint64_t)upper_initial << 32) | expected_lower;

    goto_priv(PRIV_M);
    write64(guard_paddr, guard_seed);
    write64(target_paddr, initial_value);
    write64(adjacent_paddr, adjacent_seed);
    write64(marker_paddr, marker_seed);

    goto_priv(PRIV_HS);
    hspt_init();
    sfence_vma();

    ai_memblock_lrsc_trigger_clear_mcontrol6_0();
    ai_memblock_lrsc_trigger_arm_mcontrol6_0(lower_word_vaddr,
        AI_MEMBLOCK_LRSC_TRIGGER_LOAD);
    uint64_t saved_mstatus =
        ai_memblock_lrsc_trigger_enable_breakpoint_delivery();

    goto_priv(PRIV_HS);
    TEST_SETUP_EXCEPT();
    (void)amoand_w(lower_word_vaddr, blocked_operand);
    TEST_ASSERT("a load-only mcontrol6 address trigger armed on the lower word reports breakpoint on amoand.w before the logic-and lane RMW can commit",
        excpt.triggered == true &&
        excpt.cause == CAUSE_BKP
    );

    ai_memblock_lrsc_trigger_restore_machine_status(saved_mstatus);
    ai_memblock_lrsc_trigger_clear_mcontrol6_0();

    goto_priv(PRIV_M);
    bool trigger_preserved_words =
        read64(guard_paddr) == guard_seed &&
        read64(target_paddr) == initial_value &&
        read32(lower_word_paddr) == lower_initial &&
        read32(upper_word_paddr) == upper_initial &&
        read64(adjacent_paddr) == adjacent_seed &&
        read64(marker_paddr) == marker_seed;
    TEST_ASSERT("the load-triggered lower-word amoand.w breakpoint leaves the full doubleword, guard, adjacent, and marker words unchanged",
        trigger_preserved_words
    );

    goto_priv(PRIV_HS);
    TEST_SETUP_EXCEPT();
    uint32_t old_lower = amoand_w(lower_word_vaddr, final_operand);
    TEST_ASSERT("after clearing the load-only trigger, amoand.w returns the old lower word and AND updates only that 32-bit lane",
        excpt.triggered == false &&
        old_lower == lower_initial &&
        read64(target_vaddr) == expected_final &&
        read32(lower_word_vaddr) == expected_lower &&
        read32(upper_word_vaddr) == upper_initial
    );

    write64(marker_vaddr, marker_final);
    TEST_ASSERT("post-clear marker progress and non-target sentinels prove the amoand.w lower-word load-trigger breakpoint path did not poison following memory operations",
        read64(guard_vaddr) == guard_seed &&
        read64(adjacent_vaddr) == adjacent_seed &&
        read64(marker_vaddr) == marker_final
    );

    goto_priv(PRIV_M);
    TEST_ASSERT("backing memory shows only the post-clear lower-word amoand.w AND lane update plus marker progress, with upper lane, guard, and adjacent unchanged",
        read64(guard_paddr) == guard_seed &&
        read64(target_paddr) == expected_final &&
        read32(lower_word_paddr) == expected_lower &&
        read32(upper_word_paddr) == upper_initial &&
        read64(adjacent_paddr) == adjacent_seed &&
        read64(marker_paddr) == marker_final
    );

    TEST_END();
}
