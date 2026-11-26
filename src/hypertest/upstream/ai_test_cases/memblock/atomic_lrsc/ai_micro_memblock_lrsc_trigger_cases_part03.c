#include "ai_micro_memblock_lrsc_trigger_common.h"

bool ai_micro_memblock_amoand_w_lower_word_store_trigger_breakpoint_suppresses_logic_and_lane_then_clear_succeeds_corner()
{
    TEST_START();

    const enum test_page page = VSRWX_GRWX;
    const uintptr_t base_vaddr = hs_page_base(page) + 0xf40U;
    const uintptr_t base_paddr = phys_page_base(page) + 0xf40U;
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
    const uint64_t guard_seed = 0x616e646c73747264ULL;
    const uint32_t lower_initial = 0x55aa33ccU;
    const uint32_t upper_initial = 0xc3d2e1f0U;
    const uint64_t initial_value =
        ((uint64_t)upper_initial << 32) | lower_initial;
    const uint64_t adjacent_seed = 0x0f1e2d3c4b5a6978ULL;
    const uint64_t marker_seed = 0x8877665544332211ULL;
    const uint64_t marker_final = 0x1122334455667788ULL;
    const uint32_t blocked_operand = 0x33cc55aaU;
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
        AI_MEMBLOCK_LRSC_TRIGGER_STORE);
    uint64_t saved_mstatus =
        ai_memblock_lrsc_trigger_enable_breakpoint_delivery();

    goto_priv(PRIV_HS);
    TEST_SETUP_EXCEPT();
    (void)amoand_w(lower_word_vaddr, blocked_operand);
    TEST_ASSERT("a store-only mcontrol6 address trigger armed on the lower word reports breakpoint on amoand.w before the logic-and lane write can commit",
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
    TEST_ASSERT("the store-triggered lower-word amoand.w breakpoint leaves the full doubleword, guard, adjacent, and marker words unchanged",
        trigger_preserved_words
    );

    goto_priv(PRIV_HS);
    TEST_SETUP_EXCEPT();
    uint32_t old_lower = amoand_w(lower_word_vaddr, final_operand);
    TEST_ASSERT("after clearing the store-only trigger, amoand.w returns the old lower word and AND updates only that 32-bit lane",
        excpt.triggered == false &&
        old_lower == lower_initial &&
        read64(target_vaddr) == expected_final &&
        read32(lower_word_vaddr) == expected_lower &&
        read32(upper_word_vaddr) == upper_initial
    );

    write64(marker_vaddr, marker_final);
    TEST_ASSERT("post-clear marker progress and non-target sentinels prove the amoand.w lower-word store-trigger breakpoint path did not poison following memory operations",
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

bool ai_micro_memblock_amoand_d_load_trigger_breakpoint_suppresses_logic_and_doubleword_then_clear_succeeds_corner()
{
    TEST_START();

    const enum test_page page = VSRWX_GRWX;
    const uintptr_t base_vaddr = hs_page_base(page) + 0xf80U;
    const uintptr_t base_paddr = phys_page_base(page) + 0xf80U;
    const uintptr_t guard_vaddr = base_vaddr;
    const uintptr_t target_vaddr = base_vaddr + sizeof(uint64_t);
    const uintptr_t adjacent_vaddr = base_vaddr + 2U * sizeof(uint64_t);
    const uintptr_t marker_vaddr = base_vaddr + 3U * sizeof(uint64_t);
    const uintptr_t guard_paddr = base_paddr;
    const uintptr_t target_paddr = base_paddr + sizeof(uint64_t);
    const uintptr_t adjacent_paddr = base_paddr + 2U * sizeof(uint64_t);
    const uintptr_t marker_paddr = base_paddr + 3U * sizeof(uint64_t);
    const uint64_t guard_seed = 0x616e64646c647270ULL;
    const uint64_t initial_value = 0xf0ff55aac3d2e1f0ULL;
    const uint64_t adjacent_seed = 0x0f1e2d3c4b5a6978ULL;
    const uint64_t marker_seed = 0x8877665544332211ULL;
    const uint64_t marker_final = 0x1122334455667788ULL;
    const uint64_t blocked_operand = 0x00ff00ff00ff00ffULL;
    const uint64_t final_operand = 0x0ff0ff000ff0ff00ULL;
    const uint64_t expected_final = initial_value & final_operand;

    goto_priv(PRIV_M);
    write64(guard_paddr, guard_seed);
    write64(target_paddr, initial_value);
    write64(adjacent_paddr, adjacent_seed);
    write64(marker_paddr, marker_seed);

    goto_priv(PRIV_HS);
    hspt_init();
    sfence_vma();

    ai_memblock_lrsc_trigger_clear_mcontrol6_0();
    ai_memblock_lrsc_trigger_arm_mcontrol6_0(target_vaddr,
        AI_MEMBLOCK_LRSC_TRIGGER_LOAD);
    uint64_t saved_mstatus =
        ai_memblock_lrsc_trigger_enable_breakpoint_delivery();

    goto_priv(PRIV_HS);
    TEST_SETUP_EXCEPT();
    (void)amoand_d(target_vaddr, blocked_operand);
    TEST_ASSERT("a load-only mcontrol6 address trigger armed on the doubleword reports breakpoint on amoand.d before the logic-and RMW can commit",
        excpt.triggered == true &&
        excpt.cause == CAUSE_BKP
    );

    ai_memblock_lrsc_trigger_restore_machine_status(saved_mstatus);
    ai_memblock_lrsc_trigger_clear_mcontrol6_0();

    goto_priv(PRIV_M);
    bool trigger_preserved_words =
        read64(guard_paddr) == guard_seed &&
        read64(target_paddr) == initial_value &&
        read64(adjacent_paddr) == adjacent_seed &&
        read64(marker_paddr) == marker_seed;
    TEST_ASSERT("the load-triggered amoand.d breakpoint leaves target, guard, adjacent, and marker doublewords unchanged",
        trigger_preserved_words
    );

    goto_priv(PRIV_HS);
    TEST_SETUP_EXCEPT();
    uint64_t old_value = amoand_d(target_vaddr, final_operand);
    TEST_ASSERT("after clearing the load-only trigger, amoand.d returns the old doubleword and AND updates the full 64-bit target",
        excpt.triggered == false &&
        old_value == initial_value &&
        read64(target_vaddr) == expected_final
    );

    write64(marker_vaddr, marker_final);
    TEST_ASSERT("post-clear marker progress and non-target sentinels prove the amoand.d load-trigger breakpoint path did not poison following memory operations",
        read64(guard_vaddr) == guard_seed &&
        read64(adjacent_vaddr) == adjacent_seed &&
        read64(marker_vaddr) == marker_final
    );

    goto_priv(PRIV_M);
    TEST_ASSERT("backing memory shows only the post-clear amoand.d 64-bit AND update plus marker progress, with guard and adjacent unchanged",
        read64(guard_paddr) == guard_seed &&
        read64(target_paddr) == expected_final &&
        read64(adjacent_paddr) == adjacent_seed &&
        read64(marker_paddr) == marker_final
    );

    TEST_END();
}

bool ai_micro_memblock_amoand_d_store_trigger_breakpoint_suppresses_logic_and_doubleword_then_clear_succeeds_corner()
{
    TEST_START();

    const enum test_page page = VSRWX_GRWX;
    const uintptr_t base_vaddr = hs_page_base(page) + 0xfc0U;
    const uintptr_t base_paddr = phys_page_base(page) + 0xfc0U;
    const uintptr_t guard_vaddr = base_vaddr;
    const uintptr_t target_vaddr = base_vaddr + sizeof(uint64_t);
    const uintptr_t adjacent_vaddr = base_vaddr + 2U * sizeof(uint64_t);
    const uintptr_t marker_vaddr = base_vaddr + 3U * sizeof(uint64_t);
    const uintptr_t guard_paddr = base_paddr;
    const uintptr_t target_paddr = base_paddr + sizeof(uint64_t);
    const uintptr_t adjacent_paddr = base_paddr + 2U * sizeof(uint64_t);
    const uintptr_t marker_paddr = base_paddr + 3U * sizeof(uint64_t);
    const uint64_t guard_seed = 0x616e646473747270ULL;
    const uint64_t initial_value = 0x55aa33ccc3d2e1f0ULL;
    const uint64_t adjacent_seed = 0x9182736455463728ULL;
    const uint64_t marker_seed = 0x0123456789abcdefULL;
    const uint64_t marker_final = 0xfedcba9876543210ULL;
    const uint64_t blocked_operand = 0x33cc55aa00ff00ffULL;
    const uint64_t final_operand = 0x0ff0ff000ff0ff00ULL;
    const uint64_t expected_final = initial_value & final_operand;

    goto_priv(PRIV_M);
    write64(guard_paddr, guard_seed);
    write64(target_paddr, initial_value);
    write64(adjacent_paddr, adjacent_seed);
    write64(marker_paddr, marker_seed);

    goto_priv(PRIV_HS);
    hspt_init();
    sfence_vma();

    ai_memblock_lrsc_trigger_clear_mcontrol6_0();
    ai_memblock_lrsc_trigger_arm_mcontrol6_0(target_vaddr,
        AI_MEMBLOCK_LRSC_TRIGGER_STORE);
    uint64_t saved_mstatus =
        ai_memblock_lrsc_trigger_enable_breakpoint_delivery();

    goto_priv(PRIV_HS);
    TEST_SETUP_EXCEPT();
    (void)amoand_d(target_vaddr, blocked_operand);
    TEST_ASSERT("a store-only mcontrol6 address trigger armed on the doubleword reports breakpoint on amoand.d before the logic-and write can commit",
        excpt.triggered == true &&
        excpt.cause == CAUSE_BKP
    );

    ai_memblock_lrsc_trigger_restore_machine_status(saved_mstatus);
    ai_memblock_lrsc_trigger_clear_mcontrol6_0();

    goto_priv(PRIV_M);
    bool trigger_preserved_words =
        read64(guard_paddr) == guard_seed &&
        read64(target_paddr) == initial_value &&
        read64(adjacent_paddr) == adjacent_seed &&
        read64(marker_paddr) == marker_seed;
    TEST_ASSERT("the store-triggered amoand.d breakpoint leaves target, guard, adjacent, and marker doublewords unchanged",
        trigger_preserved_words
    );

    goto_priv(PRIV_HS);
    TEST_SETUP_EXCEPT();
    uint64_t old_value = amoand_d(target_vaddr, final_operand);
    TEST_ASSERT("after clearing the store-only trigger, amoand.d returns the old doubleword and AND updates the full 64-bit target",
        excpt.triggered == false &&
        old_value == initial_value &&
        read64(target_vaddr) == expected_final
    );

    write64(marker_vaddr, marker_final);
    TEST_ASSERT("post-clear marker progress and non-target sentinels prove the amoand.d store-trigger breakpoint path did not poison following memory operations",
        read64(guard_vaddr) == guard_seed &&
        read64(adjacent_vaddr) == adjacent_seed &&
        read64(marker_vaddr) == marker_final
    );

    goto_priv(PRIV_M);
    TEST_ASSERT("backing memory shows only the post-clear amoand.d 64-bit AND update plus marker progress, with guard and adjacent unchanged",
        read64(guard_paddr) == guard_seed &&
        read64(target_paddr) == expected_final &&
        read64(adjacent_paddr) == adjacent_seed &&
        read64(marker_paddr) == marker_final
    );

    TEST_END();
}

bool ai_micro_memblock_amoor_d_load_trigger_breakpoint_suppresses_logic_or_doubleword_then_clear_succeeds_corner()
{
    TEST_START();

    const enum test_page page = VSRWX_GRWX;
    const uintptr_t base_vaddr = hs_page_base(page) + 0x100U;
    const uintptr_t base_paddr = phys_page_base(page) + 0x100U;
    const uintptr_t guard_vaddr = base_vaddr;
    const uintptr_t target_vaddr = base_vaddr + sizeof(uint64_t);
    const uintptr_t adjacent_vaddr = base_vaddr + 2U * sizeof(uint64_t);
    const uintptr_t marker_vaddr = base_vaddr + 3U * sizeof(uint64_t);
    const uintptr_t guard_paddr = base_paddr;
    const uintptr_t target_paddr = base_paddr + sizeof(uint64_t);
    const uintptr_t adjacent_paddr = base_paddr + 2U * sizeof(uint64_t);
    const uintptr_t marker_paddr = base_paddr + 3U * sizeof(uint64_t);
    const uint64_t guard_seed = 0x6f7264646c647270ULL;
    const uint64_t initial_value = 0x12003400c3d2e1f0ULL;
    const uint64_t adjacent_seed = 0x2436485a6c7e8092ULL;
    const uint64_t marker_seed = 0x0f1e2d3c4b5a6978ULL;
    const uint64_t marker_final = 0x78695a4b3c2d1e0fULL;
    const uint64_t blocked_operand = 0x00f0004f0000004fULL;
    const uint64_t final_operand = 0x0000c0ff0000c0ffULL;
    const uint64_t expected_final = initial_value | final_operand;

    goto_priv(PRIV_M);
    write64(guard_paddr, guard_seed);
    write64(target_paddr, initial_value);
    write64(adjacent_paddr, adjacent_seed);
    write64(marker_paddr, marker_seed);

    goto_priv(PRIV_HS);
    hspt_init();
    sfence_vma();

    ai_memblock_lrsc_trigger_clear_mcontrol6_0();
    ai_memblock_lrsc_trigger_arm_mcontrol6_0(target_vaddr,
        AI_MEMBLOCK_LRSC_TRIGGER_LOAD);
    uint64_t saved_mstatus =
        ai_memblock_lrsc_trigger_enable_breakpoint_delivery();

    goto_priv(PRIV_HS);
    TEST_SETUP_EXCEPT();
    (void)amoor_d(target_vaddr, blocked_operand);
    TEST_ASSERT("a load-only mcontrol6 address trigger armed on the doubleword reports breakpoint on amoor.d before the logic-or RMW can commit",
        excpt.triggered == true &&
        excpt.cause == CAUSE_BKP
    );

    ai_memblock_lrsc_trigger_restore_machine_status(saved_mstatus);
    ai_memblock_lrsc_trigger_clear_mcontrol6_0();

    goto_priv(PRIV_M);
    bool trigger_preserved_words =
        read64(guard_paddr) == guard_seed &&
        read64(target_paddr) == initial_value &&
        read64(adjacent_paddr) == adjacent_seed &&
        read64(marker_paddr) == marker_seed;
    TEST_ASSERT("the load-triggered amoor.d breakpoint leaves target, guard, adjacent, and marker doublewords unchanged",
        trigger_preserved_words
    );

    goto_priv(PRIV_HS);
    TEST_SETUP_EXCEPT();
    uint64_t old_value = amoor_d(target_vaddr, final_operand);
    TEST_ASSERT("after clearing the load-only trigger, amoor.d returns the old doubleword and OR updates the full 64-bit target",
        excpt.triggered == false &&
        old_value == initial_value &&
        read64(target_vaddr) == expected_final
    );

    write64(marker_vaddr, marker_final);
    TEST_ASSERT("post-clear marker progress and non-target sentinels prove the amoor.d load-trigger breakpoint path did not poison following memory operations",
        read64(guard_vaddr) == guard_seed &&
        read64(adjacent_vaddr) == adjacent_seed &&
        read64(marker_vaddr) == marker_final
    );

    goto_priv(PRIV_M);
    TEST_ASSERT("backing memory shows only the post-clear amoor.d 64-bit OR update plus marker progress, with guard and adjacent unchanged",
        read64(guard_paddr) == guard_seed &&
        read64(target_paddr) == expected_final &&
        read64(adjacent_paddr) == adjacent_seed &&
        read64(marker_paddr) == marker_final
    );

    TEST_END();
}

bool ai_micro_memblock_amoor_d_store_trigger_breakpoint_suppresses_logic_or_doubleword_then_clear_succeeds_corner()
{
    TEST_START();

    const enum test_page page = VSRWX_GRWX;
    const uintptr_t base_vaddr = hs_page_base(page) + 0x140U;
    const uintptr_t base_paddr = phys_page_base(page) + 0x140U;
    const uintptr_t guard_vaddr = base_vaddr;
    const uintptr_t target_vaddr = base_vaddr + sizeof(uint64_t);
    const uintptr_t adjacent_vaddr = base_vaddr + 2U * sizeof(uint64_t);
    const uintptr_t marker_vaddr = base_vaddr + 3U * sizeof(uint64_t);
    const uintptr_t guard_paddr = base_paddr;
    const uintptr_t target_paddr = base_paddr + sizeof(uint64_t);
    const uintptr_t adjacent_paddr = base_paddr + 2U * sizeof(uint64_t);
    const uintptr_t marker_paddr = base_paddr + 3U * sizeof(uint64_t);
    const uint64_t guard_seed = 0x6f72646473747270ULL;
    const uint64_t initial_value = 0x21003400a5c3e1d0ULL;
    const uint64_t adjacent_seed = 0x6a5b4c3d2e1f8071ULL;
    const uint64_t marker_seed = 0x13579bdf2468ace0ULL;
    const uint64_t marker_final = 0x0eca8642fdb97531ULL;
    const uint64_t blocked_operand = 0x0f0000f0000f000fULL;
    const uint64_t final_operand = 0x00ff0f0000ff0f00ULL;
    const uint64_t expected_final = initial_value | final_operand;

    goto_priv(PRIV_M);
    write64(guard_paddr, guard_seed);
    write64(target_paddr, initial_value);
    write64(adjacent_paddr, adjacent_seed);
    write64(marker_paddr, marker_seed);

    goto_priv(PRIV_HS);
    hspt_init();
    sfence_vma();

    ai_memblock_lrsc_trigger_clear_mcontrol6_0();
    ai_memblock_lrsc_trigger_arm_mcontrol6_0(target_vaddr,
        AI_MEMBLOCK_LRSC_TRIGGER_STORE);
    uint64_t saved_mstatus =
        ai_memblock_lrsc_trigger_enable_breakpoint_delivery();

    goto_priv(PRIV_HS);
    TEST_SETUP_EXCEPT();
    (void)amoor_d(target_vaddr, blocked_operand);
    TEST_ASSERT("a store-only mcontrol6 address trigger armed on the doubleword reports breakpoint on amoor.d before the logic-or write can commit",
        excpt.triggered == true &&
        excpt.cause == CAUSE_BKP
    );

    ai_memblock_lrsc_trigger_restore_machine_status(saved_mstatus);
    ai_memblock_lrsc_trigger_clear_mcontrol6_0();

    goto_priv(PRIV_M);
    bool trigger_preserved_words =
        read64(guard_paddr) == guard_seed &&
        read64(target_paddr) == initial_value &&
        read64(adjacent_paddr) == adjacent_seed &&
        read64(marker_paddr) == marker_seed;
    TEST_ASSERT("the store-triggered amoor.d breakpoint leaves target, guard, adjacent, and marker doublewords unchanged",
        trigger_preserved_words
    );

    goto_priv(PRIV_HS);
    TEST_SETUP_EXCEPT();
    uint64_t old_value = amoor_d(target_vaddr, final_operand);
    TEST_ASSERT("after clearing the store-only trigger, amoor.d returns the old doubleword and OR updates the full 64-bit target",
        excpt.triggered == false &&
        old_value == initial_value &&
        read64(target_vaddr) == expected_final
    );

    write64(marker_vaddr, marker_final);
    TEST_ASSERT("post-clear marker progress and non-target sentinels prove the amoor.d store-trigger breakpoint path did not poison following memory operations",
        read64(guard_vaddr) == guard_seed &&
        read64(adjacent_vaddr) == adjacent_seed &&
        read64(marker_vaddr) == marker_final
    );

    goto_priv(PRIV_M);
    TEST_ASSERT("backing memory shows only the post-clear amoor.d 64-bit OR update plus marker progress, with guard and adjacent unchanged",
        read64(guard_paddr) == guard_seed &&
        read64(target_paddr) == expected_final &&
        read64(adjacent_paddr) == adjacent_seed &&
        read64(marker_paddr) == marker_final
    );

    TEST_END();
}

bool ai_micro_memblock_amoxor_d_load_trigger_breakpoint_suppresses_logic_xor_doubleword_then_clear_succeeds_corner()
{
    TEST_START();

    const enum test_page page = VSRWX_GRWX;
    const uintptr_t base_vaddr = hs_page_base(page) + 0x180U;
    const uintptr_t base_paddr = phys_page_base(page) + 0x180U;
    const uintptr_t guard_vaddr = base_vaddr;
    const uintptr_t target_vaddr = base_vaddr + sizeof(uint64_t);
    const uintptr_t adjacent_vaddr = base_vaddr + 2U * sizeof(uint64_t);
    const uintptr_t marker_vaddr = base_vaddr + 3U * sizeof(uint64_t);
    const uintptr_t guard_paddr = base_paddr;
    const uintptr_t target_paddr = base_paddr + sizeof(uint64_t);
    const uintptr_t adjacent_paddr = base_paddr + 2U * sizeof(uint64_t);
    const uintptr_t marker_paddr = base_paddr + 3U * sizeof(uint64_t);
    const uint64_t guard_seed = 0x786f72646c647270ULL;
    const uint64_t initial_value = 0x5a0f33c3a5c3e1d0ULL;
    const uint64_t adjacent_seed = 0x1029384756abcdefULL;
    const uint64_t marker_seed = 0xf0e1d2c3b4a59687ULL;
    const uint64_t marker_final = 0x7896a5b4c3d2e1f0ULL;
    const uint64_t blocked_operand = 0x00ff0f0f00ff0f0fULL;
    const uint64_t final_operand = 0x0f00f0ff00ff0f00ULL;
    const uint64_t expected_final = initial_value ^ final_operand;

    goto_priv(PRIV_M);
    write64(guard_paddr, guard_seed);
    write64(target_paddr, initial_value);
    write64(adjacent_paddr, adjacent_seed);
    write64(marker_paddr, marker_seed);

    goto_priv(PRIV_HS);
    hspt_init();
    sfence_vma();

    ai_memblock_lrsc_trigger_clear_mcontrol6_0();
    ai_memblock_lrsc_trigger_arm_mcontrol6_0(target_vaddr,
        AI_MEMBLOCK_LRSC_TRIGGER_LOAD);
    uint64_t saved_mstatus =
        ai_memblock_lrsc_trigger_enable_breakpoint_delivery();

    goto_priv(PRIV_HS);
    TEST_SETUP_EXCEPT();
    (void)amoxor_d(target_vaddr, blocked_operand);
    TEST_ASSERT("a load-only mcontrol6 address trigger armed on the doubleword reports breakpoint on amoxor.d before the logic-xor RMW can commit",
        excpt.triggered == true &&
        excpt.cause == CAUSE_BKP
    );

    ai_memblock_lrsc_trigger_restore_machine_status(saved_mstatus);
    ai_memblock_lrsc_trigger_clear_mcontrol6_0();

    goto_priv(PRIV_M);
    bool trigger_preserved_words =
        read64(guard_paddr) == guard_seed &&
        read64(target_paddr) == initial_value &&
        read64(adjacent_paddr) == adjacent_seed &&
        read64(marker_paddr) == marker_seed;
    TEST_ASSERT("the load-triggered amoxor.d breakpoint leaves target, guard, adjacent, and marker doublewords unchanged",
        trigger_preserved_words
    );

    goto_priv(PRIV_HS);
    TEST_SETUP_EXCEPT();
    uint64_t old_value = amoxor_d(target_vaddr, final_operand);
    TEST_ASSERT("after clearing the load-only trigger, amoxor.d returns the old doubleword and XOR updates the full 64-bit target",
        excpt.triggered == false &&
        old_value == initial_value &&
        read64(target_vaddr) == expected_final
    );

    write64(marker_vaddr, marker_final);
    TEST_ASSERT("post-clear marker progress and non-target sentinels prove the amoxor.d load-trigger breakpoint path did not poison following memory operations",
        read64(guard_vaddr) == guard_seed &&
        read64(adjacent_vaddr) == adjacent_seed &&
        read64(marker_vaddr) == marker_final
    );

    goto_priv(PRIV_M);
    TEST_ASSERT("backing memory shows only the post-clear amoxor.d 64-bit XOR update plus marker progress, with guard and adjacent unchanged",
        read64(guard_paddr) == guard_seed &&
        read64(target_paddr) == expected_final &&
        read64(adjacent_paddr) == adjacent_seed &&
        read64(marker_paddr) == marker_final
    );

    TEST_END();
}

bool ai_micro_memblock_amoxor_d_store_trigger_breakpoint_suppresses_logic_xor_doubleword_then_clear_succeeds_corner()
{
    TEST_START();

    const enum test_page page = VSRWX_GRWX;
    const uintptr_t base_vaddr = hs_page_base(page) + 0x1c0U;
    const uintptr_t base_paddr = phys_page_base(page) + 0x1c0U;
    const uintptr_t guard_vaddr = base_vaddr;
    const uintptr_t target_vaddr = base_vaddr + sizeof(uint64_t);
    const uintptr_t adjacent_vaddr = base_vaddr + 2U * sizeof(uint64_t);
    const uintptr_t marker_vaddr = base_vaddr + 3U * sizeof(uint64_t);
    const uintptr_t guard_paddr = base_paddr;
    const uintptr_t target_paddr = base_paddr + sizeof(uint64_t);
    const uintptr_t adjacent_paddr = base_paddr + 2U * sizeof(uint64_t);
    const uintptr_t marker_paddr = base_paddr + 3U * sizeof(uint64_t);
    const uint64_t guard_seed = 0x786f726473747270ULL;
    const uint64_t initial_value = 0x69f033c3a5c3e1d0ULL;
    const uint64_t adjacent_seed = 0x0badc0ffee123456ULL;
    const uint64_t marker_seed = 0x76543210fedcba98ULL;
    const uint64_t marker_final = 0x89abcdef01234567ULL;
    const uint64_t blocked_operand = 0x0ff00ff00ff00ff0ULL;
    const uint64_t final_operand = 0x00f0ff0f0f00ff0fULL;
    const uint64_t expected_final = initial_value ^ final_operand;

    goto_priv(PRIV_M);
    write64(guard_paddr, guard_seed);
    write64(target_paddr, initial_value);
    write64(adjacent_paddr, adjacent_seed);
    write64(marker_paddr, marker_seed);

    goto_priv(PRIV_HS);
    hspt_init();
    sfence_vma();

    ai_memblock_lrsc_trigger_clear_mcontrol6_0();
    ai_memblock_lrsc_trigger_arm_mcontrol6_0(target_vaddr,
        AI_MEMBLOCK_LRSC_TRIGGER_STORE);
    uint64_t saved_mstatus =
        ai_memblock_lrsc_trigger_enable_breakpoint_delivery();

    goto_priv(PRIV_HS);
    TEST_SETUP_EXCEPT();
    (void)amoxor_d(target_vaddr, blocked_operand);
    TEST_ASSERT("a store-only mcontrol6 address trigger armed on the doubleword reports breakpoint on amoxor.d before the logic-xor write can commit",
        excpt.triggered == true &&
        excpt.cause == CAUSE_BKP
    );

    ai_memblock_lrsc_trigger_restore_machine_status(saved_mstatus);
    ai_memblock_lrsc_trigger_clear_mcontrol6_0();

    goto_priv(PRIV_M);
    bool trigger_preserved_words =
        read64(guard_paddr) == guard_seed &&
        read64(target_paddr) == initial_value &&
        read64(adjacent_paddr) == adjacent_seed &&
        read64(marker_paddr) == marker_seed;
    TEST_ASSERT("the store-triggered amoxor.d breakpoint leaves target, guard, adjacent, and marker doublewords unchanged",
        trigger_preserved_words
    );

    goto_priv(PRIV_HS);
    TEST_SETUP_EXCEPT();
    uint64_t old_value = amoxor_d(target_vaddr, final_operand);
    TEST_ASSERT("after clearing the store-only trigger, amoxor.d returns the old doubleword and XOR updates the full 64-bit target",
        excpt.triggered == false &&
        old_value == initial_value &&
        read64(target_vaddr) == expected_final
    );

    write64(marker_vaddr, marker_final);
    TEST_ASSERT("post-clear marker progress and non-target sentinels prove the amoxor.d store-trigger breakpoint path did not poison following memory operations",
        read64(guard_vaddr) == guard_seed &&
        read64(adjacent_vaddr) == adjacent_seed &&
        read64(marker_vaddr) == marker_final
    );

    goto_priv(PRIV_M);
    TEST_ASSERT("backing memory shows only the post-clear amoxor.d 64-bit XOR update plus marker progress, with guard and adjacent unchanged",
        read64(guard_paddr) == guard_seed &&
        read64(target_paddr) == expected_final &&
        read64(adjacent_paddr) == adjacent_seed &&
        read64(marker_paddr) == marker_final
    );

    TEST_END();
}

bool ai_micro_memblock_amomin_d_load_trigger_breakpoint_suppresses_signed_min_doubleword_then_clear_succeeds_corner()
{
    TEST_START();

    const enum test_page page = VSRWX_GRWX;
    const uintptr_t base_vaddr = hs_page_base(page) + 0x200U;
    const uintptr_t base_paddr = phys_page_base(page) + 0x200U;
    const uintptr_t guard_vaddr = base_vaddr;
    const uintptr_t target_vaddr = base_vaddr + sizeof(uint64_t);
    const uintptr_t adjacent_vaddr = base_vaddr + 2U * sizeof(uint64_t);
    const uintptr_t marker_vaddr = base_vaddr + 3U * sizeof(uint64_t);
    const uintptr_t guard_paddr = base_paddr;
    const uintptr_t target_paddr = base_paddr + sizeof(uint64_t);
    const uintptr_t adjacent_paddr = base_paddr + 2U * sizeof(uint64_t);
    const uintptr_t marker_paddr = base_paddr + 3U * sizeof(uint64_t);
    const uint64_t guard_seed = 0x6d696e646c647270ULL;
    const uint64_t initial_value = 0x0000000000007120ULL;
    const uint64_t adjacent_seed = 0x1122aa55cc7733eeULL;
    const uint64_t marker_seed = 0x0ddc0ffeebadf00dULL;
    const uint64_t marker_final = 0xd00dfadeefc0dd00ULL;
    const uint64_t blocked_operand = 0x8000000000001001ULL;
    const uint64_t final_operand = 0xffffffffffff8f00ULL;
    const uint64_t expected_final = final_operand;

    goto_priv(PRIV_M);
    write64(guard_paddr, guard_seed);
    write64(target_paddr, initial_value);
    write64(adjacent_paddr, adjacent_seed);
    write64(marker_paddr, marker_seed);

    goto_priv(PRIV_HS);
    hspt_init();
    sfence_vma();

    ai_memblock_lrsc_trigger_clear_mcontrol6_0();
    ai_memblock_lrsc_trigger_arm_mcontrol6_0(target_vaddr,
        AI_MEMBLOCK_LRSC_TRIGGER_LOAD);
    uint64_t saved_mstatus =
        ai_memblock_lrsc_trigger_enable_breakpoint_delivery();

    goto_priv(PRIV_HS);
    TEST_SETUP_EXCEPT();
    (void)amomin_d(target_vaddr, blocked_operand);
    TEST_ASSERT("a load-only mcontrol6 address trigger armed on the doubleword reports breakpoint on amomin.d before the signed-min RMW can commit",
        excpt.triggered == true &&
        excpt.cause == CAUSE_BKP
    );

    ai_memblock_lrsc_trigger_restore_machine_status(saved_mstatus);
    ai_memblock_lrsc_trigger_clear_mcontrol6_0();

    goto_priv(PRIV_M);
    bool trigger_preserved_words =
        read64(guard_paddr) == guard_seed &&
        read64(target_paddr) == initial_value &&
        read64(adjacent_paddr) == adjacent_seed &&
        read64(marker_paddr) == marker_seed;
    TEST_ASSERT("the load-triggered amomin.d breakpoint leaves target, guard, adjacent, and marker doublewords unchanged",
        trigger_preserved_words
    );

    goto_priv(PRIV_HS);
    TEST_SETUP_EXCEPT();
    uint64_t old_value = amomin_d(target_vaddr, final_operand);
    TEST_ASSERT("after clearing the load-only trigger, amomin.d returns the old doubleword and signed-min updates the full 64-bit target",
        excpt.triggered == false &&
        old_value == initial_value &&
        read64(target_vaddr) == expected_final
    );

    write64(marker_vaddr, marker_final);
    TEST_ASSERT("post-clear marker progress and non-target sentinels prove the amomin.d load-trigger breakpoint path did not poison following memory operations",
        read64(guard_vaddr) == guard_seed &&
        read64(adjacent_vaddr) == adjacent_seed &&
        read64(marker_vaddr) == marker_final
    );

    goto_priv(PRIV_M);
    TEST_ASSERT("backing memory shows only the post-clear amomin.d 64-bit signed-min update plus marker progress, with guard and adjacent unchanged",
        read64(guard_paddr) == guard_seed &&
        read64(target_paddr) == expected_final &&
        read64(adjacent_paddr) == adjacent_seed &&
        read64(marker_paddr) == marker_final
    );

    TEST_END();
}

bool ai_micro_memblock_amomin_d_store_trigger_breakpoint_suppresses_signed_min_doubleword_then_clear_succeeds_corner()
{
    TEST_START();

    const enum test_page page = VSRWX_GRWX;
    const uintptr_t base_vaddr = hs_page_base(page) + 0x240U;
    const uintptr_t base_paddr = phys_page_base(page) + 0x240U;
    const uintptr_t guard_vaddr = base_vaddr;
    const uintptr_t target_vaddr = base_vaddr + sizeof(uint64_t);
    const uintptr_t adjacent_vaddr = base_vaddr + 2U * sizeof(uint64_t);
    const uintptr_t marker_vaddr = base_vaddr + 3U * sizeof(uint64_t);
    const uintptr_t guard_paddr = base_paddr;
    const uintptr_t target_paddr = base_paddr + sizeof(uint64_t);
    const uintptr_t adjacent_paddr = base_paddr + 2U * sizeof(uint64_t);
    const uintptr_t marker_paddr = base_paddr + 3U * sizeof(uint64_t);
    const uint64_t guard_seed = 0x6d696e6473747270ULL;
    const uint64_t initial_value = 0x0000000000006220ULL;
    const uint64_t adjacent_seed = 0x2233bb66dd8844ffULL;
    const uint64_t marker_seed = 0x1234fedc5678cafeULL;
    const uint64_t marker_final = 0xefcdab8967452301ULL;
    const uint64_t blocked_operand = 0x8000000000002001ULL;
    const uint64_t final_operand = 0xffffffffffff9e00ULL;
    const uint64_t expected_final = final_operand;

    goto_priv(PRIV_M);
    write64(guard_paddr, guard_seed);
    write64(target_paddr, initial_value);
    write64(adjacent_paddr, adjacent_seed);
    write64(marker_paddr, marker_seed);

    goto_priv(PRIV_HS);
    hspt_init();
    sfence_vma();

    ai_memblock_lrsc_trigger_clear_mcontrol6_0();
    ai_memblock_lrsc_trigger_arm_mcontrol6_0(target_vaddr,
        AI_MEMBLOCK_LRSC_TRIGGER_STORE);
    uint64_t saved_mstatus =
        ai_memblock_lrsc_trigger_enable_breakpoint_delivery();

    goto_priv(PRIV_HS);
    TEST_SETUP_EXCEPT();
    (void)amomin_d(target_vaddr, blocked_operand);
    TEST_ASSERT("a store-only mcontrol6 address trigger armed on the doubleword reports breakpoint on amomin.d before the signed-min write can commit",
        excpt.triggered == true &&
        excpt.cause == CAUSE_BKP
    );

    ai_memblock_lrsc_trigger_restore_machine_status(saved_mstatus);
    ai_memblock_lrsc_trigger_clear_mcontrol6_0();

    goto_priv(PRIV_M);
    bool trigger_preserved_words =
        read64(guard_paddr) == guard_seed &&
        read64(target_paddr) == initial_value &&
        read64(adjacent_paddr) == adjacent_seed &&
        read64(marker_paddr) == marker_seed;
    TEST_ASSERT("the store-triggered amomin.d breakpoint leaves target, guard, adjacent, and marker doublewords unchanged",
        trigger_preserved_words
    );

    goto_priv(PRIV_HS);
    TEST_SETUP_EXCEPT();
    uint64_t old_value = amomin_d(target_vaddr, final_operand);
    TEST_ASSERT("after clearing the store-only trigger, amomin.d returns the old doubleword and signed-min updates the full 64-bit target",
        excpt.triggered == false &&
        old_value == initial_value &&
        read64(target_vaddr) == expected_final
    );

    write64(marker_vaddr, marker_final);
    TEST_ASSERT("post-clear marker progress and non-target sentinels prove the amomin.d store-trigger breakpoint path did not poison following memory operations",
        read64(guard_vaddr) == guard_seed &&
        read64(adjacent_vaddr) == adjacent_seed &&
        read64(marker_vaddr) == marker_final
    );

    goto_priv(PRIV_M);
    TEST_ASSERT("backing memory shows only the post-clear amomin.d 64-bit signed-min update plus marker progress, with guard and adjacent unchanged",
        read64(guard_paddr) == guard_seed &&
        read64(target_paddr) == expected_final &&
        read64(adjacent_paddr) == adjacent_seed &&
        read64(marker_paddr) == marker_final
    );

    TEST_END();
}

bool ai_micro_memblock_amominu_d_load_trigger_breakpoint_suppresses_unsigned_min_doubleword_then_clear_succeeds_corner()
{
    TEST_START();

    const enum test_page page = VSRWX_GRWX;
    const uintptr_t base_vaddr = hs_page_base(page) + 0x280U;
    const uintptr_t base_paddr = phys_page_base(page) + 0x280U;
    const uintptr_t guard_vaddr = base_vaddr;
    const uintptr_t target_vaddr = base_vaddr + sizeof(uint64_t);
    const uintptr_t adjacent_vaddr = base_vaddr + 2U * sizeof(uint64_t);
    const uintptr_t marker_vaddr = base_vaddr + 3U * sizeof(uint64_t);
    const uintptr_t guard_paddr = base_paddr;
    const uintptr_t target_paddr = base_paddr + sizeof(uint64_t);
    const uintptr_t adjacent_paddr = base_paddr + 2U * sizeof(uint64_t);
    const uintptr_t marker_paddr = base_paddr + 3U * sizeof(uint64_t);
    const uint64_t guard_seed = 0x756d696e646c6470ULL;
    const uint64_t initial_value = 0xfedc000000007210ULL;
    const uint64_t adjacent_seed = 0x3344cc77ee995500ULL;
    const uint64_t marker_seed = 0x21436587a9cbed0fULL;
    const uint64_t marker_final = 0xf0debc9a78563412ULL;
    const uint64_t blocked_operand = 0x0000000000001111ULL;
    const uint64_t final_operand = 0x0000000000003210ULL;
    const uint64_t expected_final = final_operand;

    goto_priv(PRIV_M);
    write64(guard_paddr, guard_seed);
    write64(target_paddr, initial_value);
    write64(adjacent_paddr, adjacent_seed);
    write64(marker_paddr, marker_seed);

    goto_priv(PRIV_HS);
    hspt_init();
    sfence_vma();

    ai_memblock_lrsc_trigger_clear_mcontrol6_0();
    ai_memblock_lrsc_trigger_arm_mcontrol6_0(target_vaddr,
        AI_MEMBLOCK_LRSC_TRIGGER_LOAD);
    uint64_t saved_mstatus =
        ai_memblock_lrsc_trigger_enable_breakpoint_delivery();

    goto_priv(PRIV_HS);
    TEST_SETUP_EXCEPT();
    (void)amominu_d(target_vaddr, blocked_operand);
    TEST_ASSERT("a load-only mcontrol6 address trigger armed on the doubleword reports breakpoint on amominu.d before the unsigned-min RMW can commit",
        excpt.triggered == true &&
        excpt.cause == CAUSE_BKP
    );

    ai_memblock_lrsc_trigger_restore_machine_status(saved_mstatus);
    ai_memblock_lrsc_trigger_clear_mcontrol6_0();

    goto_priv(PRIV_M);
    bool trigger_preserved_words =
        read64(guard_paddr) == guard_seed &&
        read64(target_paddr) == initial_value &&
        read64(adjacent_paddr) == adjacent_seed &&
        read64(marker_paddr) == marker_seed;
    TEST_ASSERT("the load-triggered amominu.d breakpoint leaves target, guard, adjacent, and marker doublewords unchanged",
        trigger_preserved_words
    );

    goto_priv(PRIV_HS);
    TEST_SETUP_EXCEPT();
    uint64_t old_value = amominu_d(target_vaddr, final_operand);
    TEST_ASSERT("after clearing the load-only trigger, amominu.d returns the old doubleword and unsigned-min updates the full 64-bit target",
        excpt.triggered == false &&
        old_value == initial_value &&
        read64(target_vaddr) == expected_final
    );

    write64(marker_vaddr, marker_final);
    TEST_ASSERT("post-clear marker progress and non-target sentinels prove the amominu.d load-trigger breakpoint path did not poison following memory operations",
        read64(guard_vaddr) == guard_seed &&
        read64(adjacent_vaddr) == adjacent_seed &&
        read64(marker_vaddr) == marker_final
    );

    goto_priv(PRIV_M);
    TEST_ASSERT("backing memory shows only the post-clear amominu.d 64-bit unsigned-min update plus marker progress, with guard and adjacent unchanged",
        read64(guard_paddr) == guard_seed &&
        read64(target_paddr) == expected_final &&
        read64(adjacent_paddr) == adjacent_seed &&
        read64(marker_paddr) == marker_final
    );

    TEST_END();
}

bool ai_micro_memblock_amominu_d_store_trigger_breakpoint_suppresses_unsigned_min_doubleword_then_clear_succeeds_corner()
{
    TEST_START();

    const enum test_page page = VSRWX_GRWX;
    const uintptr_t base_vaddr = hs_page_base(page) + 0x2c0U;
    const uintptr_t base_paddr = phys_page_base(page) + 0x2c0U;
    const uintptr_t guard_vaddr = base_vaddr;
    const uintptr_t target_vaddr = base_vaddr + sizeof(uint64_t);
    const uintptr_t adjacent_vaddr = base_vaddr + 2U * sizeof(uint64_t);
    const uintptr_t marker_vaddr = base_vaddr + 3U * sizeof(uint64_t);
    const uintptr_t guard_paddr = base_paddr;
    const uintptr_t target_paddr = base_paddr + sizeof(uint64_t);
    const uintptr_t adjacent_paddr = base_paddr + 2U * sizeof(uint64_t);
    const uintptr_t marker_paddr = base_paddr + 3U * sizeof(uint64_t);
    const uint64_t guard_seed = 0x756d696e64737470ULL;
    const uint64_t initial_value = 0xfedc100000006210ULL;
    const uint64_t adjacent_seed = 0x4455dd88ffaa6600ULL;
    const uint64_t marker_seed = 0x3456789abcdef012ULL;
    const uint64_t marker_final = 0x210fedcba9876543ULL;
    const uint64_t blocked_operand = 0x0000000000002222ULL;
    const uint64_t final_operand = 0x0000000000002210ULL;
    const uint64_t expected_final = final_operand;

    goto_priv(PRIV_M);
    write64(guard_paddr, guard_seed);
    write64(target_paddr, initial_value);
    write64(adjacent_paddr, adjacent_seed);
    write64(marker_paddr, marker_seed);

    goto_priv(PRIV_HS);
    hspt_init();
    sfence_vma();

    ai_memblock_lrsc_trigger_clear_mcontrol6_0();
    ai_memblock_lrsc_trigger_arm_mcontrol6_0(target_vaddr,
        AI_MEMBLOCK_LRSC_TRIGGER_STORE);
    uint64_t saved_mstatus =
        ai_memblock_lrsc_trigger_enable_breakpoint_delivery();

    goto_priv(PRIV_HS);
    TEST_SETUP_EXCEPT();
    (void)amominu_d(target_vaddr, blocked_operand);
    TEST_ASSERT("a store-only mcontrol6 address trigger armed on the doubleword reports breakpoint on amominu.d before the unsigned-min write can commit",
        excpt.triggered == true &&
        excpt.cause == CAUSE_BKP
    );

    ai_memblock_lrsc_trigger_restore_machine_status(saved_mstatus);
    ai_memblock_lrsc_trigger_clear_mcontrol6_0();

    goto_priv(PRIV_M);
    bool trigger_preserved_words =
        read64(guard_paddr) == guard_seed &&
        read64(target_paddr) == initial_value &&
        read64(adjacent_paddr) == adjacent_seed &&
        read64(marker_paddr) == marker_seed;
    TEST_ASSERT("the store-triggered amominu.d breakpoint leaves target, guard, adjacent, and marker doublewords unchanged",
        trigger_preserved_words
    );

    goto_priv(PRIV_HS);
    TEST_SETUP_EXCEPT();
    uint64_t old_value = amominu_d(target_vaddr, final_operand);
    TEST_ASSERT("after clearing the store-only trigger, amominu.d returns the old doubleword and unsigned-min updates the full 64-bit target",
        excpt.triggered == false &&
        old_value == initial_value &&
        read64(target_vaddr) == expected_final
    );

    write64(marker_vaddr, marker_final);
    TEST_ASSERT("post-clear marker progress and non-target sentinels prove the amominu.d store-trigger breakpoint path did not poison following memory operations",
        read64(guard_vaddr) == guard_seed &&
        read64(adjacent_vaddr) == adjacent_seed &&
        read64(marker_vaddr) == marker_final
    );

    goto_priv(PRIV_M);
    TEST_ASSERT("backing memory shows only the post-clear amominu.d 64-bit unsigned-min update plus marker progress, with guard and adjacent unchanged",
        read64(guard_paddr) == guard_seed &&
        read64(target_paddr) == expected_final &&
        read64(adjacent_paddr) == adjacent_seed &&
        read64(marker_paddr) == marker_final
    );

    TEST_END();
}

bool ai_micro_memblock_amomax_d_load_trigger_breakpoint_suppresses_signed_max_doubleword_then_clear_succeeds_corner()
{
    TEST_START();

    const enum test_page page = VSRWX_GRWX;
    const uintptr_t base_vaddr = hs_page_base(page) + 0x300U;
    const uintptr_t base_paddr = phys_page_base(page) + 0x300U;
    const uintptr_t guard_vaddr = base_vaddr;
    const uintptr_t target_vaddr = base_vaddr + sizeof(uint64_t);
    const uintptr_t adjacent_vaddr = base_vaddr + 2U * sizeof(uint64_t);
    const uintptr_t marker_vaddr = base_vaddr + 3U * sizeof(uint64_t);
    const uintptr_t guard_paddr = base_paddr;
    const uintptr_t target_paddr = base_paddr + sizeof(uint64_t);
    const uintptr_t adjacent_paddr = base_paddr + 2U * sizeof(uint64_t);
    const uintptr_t marker_paddr = base_paddr + 3U * sizeof(uint64_t);
    const uint64_t guard_seed = 0x6d6178646c647270ULL;
    const uint64_t initial_value = 0xfffffffffff08000ULL;
    const uint64_t adjacent_seed = 0x5566ee99aabb7700ULL;
    const uint64_t marker_seed = 0x1029384756abcdefULL;
    const uint64_t marker_final = 0xfedcba6547382910ULL;
    const uint64_t blocked_operand = 0x0000000000006100ULL;
    const uint64_t final_operand = 0x0000000000007fffULL;
    const uint64_t expected_final = final_operand;

    goto_priv(PRIV_M);
    write64(guard_paddr, guard_seed);
    write64(target_paddr, initial_value);
    write64(adjacent_paddr, adjacent_seed);
    write64(marker_paddr, marker_seed);

    goto_priv(PRIV_HS);
    hspt_init();
    sfence_vma();

    ai_memblock_lrsc_trigger_clear_mcontrol6_0();
    ai_memblock_lrsc_trigger_arm_mcontrol6_0(target_vaddr,
        AI_MEMBLOCK_LRSC_TRIGGER_LOAD);
    uint64_t saved_mstatus =
        ai_memblock_lrsc_trigger_enable_breakpoint_delivery();

    goto_priv(PRIV_HS);
    TEST_SETUP_EXCEPT();
    (void)amomax_d(target_vaddr, blocked_operand);
    TEST_ASSERT("a load-only mcontrol6 address trigger armed on the doubleword reports breakpoint on amomax.d before the signed-max RMW can commit",
        excpt.triggered == true &&
        excpt.cause == CAUSE_BKP
    );

    ai_memblock_lrsc_trigger_restore_machine_status(saved_mstatus);
    ai_memblock_lrsc_trigger_clear_mcontrol6_0();

    goto_priv(PRIV_M);
    bool trigger_preserved_words =
        read64(guard_paddr) == guard_seed &&
        read64(target_paddr) == initial_value &&
        read64(adjacent_paddr) == adjacent_seed &&
        read64(marker_paddr) == marker_seed;
    TEST_ASSERT("the load-triggered amomax.d breakpoint leaves target, guard, adjacent, and marker doublewords unchanged",
        trigger_preserved_words
    );

    goto_priv(PRIV_HS);
    TEST_SETUP_EXCEPT();
    uint64_t old_value = amomax_d(target_vaddr, final_operand);
    TEST_ASSERT("after clearing the load-only trigger, amomax.d returns the old doubleword and signed-max updates the full 64-bit target",
        excpt.triggered == false &&
        old_value == initial_value &&
        read64(target_vaddr) == expected_final
    );

    write64(marker_vaddr, marker_final);
    TEST_ASSERT("post-clear marker progress and non-target sentinels prove the amomax.d load-trigger breakpoint path did not poison following memory operations",
        read64(guard_vaddr) == guard_seed &&
        read64(adjacent_vaddr) == adjacent_seed &&
        read64(marker_vaddr) == marker_final
    );

    goto_priv(PRIV_M);
    TEST_ASSERT("backing memory shows only the post-clear amomax.d 64-bit signed-max update plus marker progress, with guard and adjacent unchanged",
        read64(guard_paddr) == guard_seed &&
        read64(target_paddr) == expected_final &&
        read64(adjacent_paddr) == adjacent_seed &&
        read64(marker_paddr) == marker_final
    );

    TEST_END();
}

bool ai_micro_memblock_amomax_d_store_trigger_breakpoint_suppresses_signed_max_doubleword_then_clear_succeeds_corner()
{
    TEST_START();

    const enum test_page page = VSRWX_GRWX;
    const uintptr_t base_vaddr = hs_page_base(page) + 0x340U;
    const uintptr_t base_paddr = phys_page_base(page) + 0x340U;
    const uintptr_t guard_vaddr = base_vaddr;
    const uintptr_t target_vaddr = base_vaddr + sizeof(uint64_t);
    const uintptr_t adjacent_vaddr = base_vaddr + 2U * sizeof(uint64_t);
    const uintptr_t marker_vaddr = base_vaddr + 3U * sizeof(uint64_t);
    const uintptr_t guard_paddr = base_paddr;
    const uintptr_t target_paddr = base_paddr + sizeof(uint64_t);
    const uintptr_t adjacent_paddr = base_paddr + 2U * sizeof(uint64_t);
    const uintptr_t marker_paddr = base_paddr + 3U * sizeof(uint64_t);
    const uint64_t guard_seed = 0x6d61786473747270ULL;
    const uint64_t initial_value = 0xfffffffffff07000ULL;
    const uint64_t adjacent_seed = 0x6677ffaa88bb5500ULL;
    const uint64_t marker_seed = 0x23456789abcdef01ULL;
    const uint64_t marker_final = 0x10fedcba98765432ULL;
    const uint64_t blocked_operand = 0x0000000000006200ULL;
    const uint64_t final_operand = 0x0000000000006fffULL;
    const uint64_t expected_final = final_operand;

    goto_priv(PRIV_M);
    write64(guard_paddr, guard_seed);
    write64(target_paddr, initial_value);
    write64(adjacent_paddr, adjacent_seed);
    write64(marker_paddr, marker_seed);

    goto_priv(PRIV_HS);
    hspt_init();
    sfence_vma();

    ai_memblock_lrsc_trigger_clear_mcontrol6_0();
    ai_memblock_lrsc_trigger_arm_mcontrol6_0(target_vaddr,
        AI_MEMBLOCK_LRSC_TRIGGER_STORE);
    uint64_t saved_mstatus =
        ai_memblock_lrsc_trigger_enable_breakpoint_delivery();

    goto_priv(PRIV_HS);
    TEST_SETUP_EXCEPT();
    (void)amomax_d(target_vaddr, blocked_operand);
    TEST_ASSERT("a store-only mcontrol6 address trigger armed on the doubleword reports breakpoint on amomax.d before the signed-max write can commit",
        excpt.triggered == true &&
        excpt.cause == CAUSE_BKP
    );

    ai_memblock_lrsc_trigger_restore_machine_status(saved_mstatus);
    ai_memblock_lrsc_trigger_clear_mcontrol6_0();

    goto_priv(PRIV_M);
    bool trigger_preserved_words =
        read64(guard_paddr) == guard_seed &&
        read64(target_paddr) == initial_value &&
        read64(adjacent_paddr) == adjacent_seed &&
        read64(marker_paddr) == marker_seed;
    TEST_ASSERT("the store-triggered amomax.d breakpoint leaves target, guard, adjacent, and marker doublewords unchanged",
        trigger_preserved_words
    );

    goto_priv(PRIV_HS);
    TEST_SETUP_EXCEPT();
    uint64_t old_value = amomax_d(target_vaddr, final_operand);
    TEST_ASSERT("after clearing the store-only trigger, amomax.d returns the old doubleword and signed-max updates the full 64-bit target",
        excpt.triggered == false &&
        old_value == initial_value &&
        read64(target_vaddr) == expected_final
    );

    write64(marker_vaddr, marker_final);
    TEST_ASSERT("post-clear marker progress and non-target sentinels prove the amomax.d store-trigger breakpoint path did not poison following memory operations",
        read64(guard_vaddr) == guard_seed &&
        read64(adjacent_vaddr) == adjacent_seed &&
        read64(marker_vaddr) == marker_final
    );

    goto_priv(PRIV_M);
    TEST_ASSERT("backing memory shows only the post-clear amomax.d 64-bit signed-max update plus marker progress, with guard and adjacent unchanged",
        read64(guard_paddr) == guard_seed &&
        read64(target_paddr) == expected_final &&
        read64(adjacent_paddr) == adjacent_seed &&
        read64(marker_paddr) == marker_final
    );

    TEST_END();
}

bool ai_micro_memblock_amomaxu_d_load_trigger_breakpoint_suppresses_unsigned_max_doubleword_then_clear_succeeds_corner()
{
    TEST_START();

    const enum test_page page = VSRWX_GRWX;
    const uintptr_t base_vaddr = hs_page_base(page) + 0x380U;
    const uintptr_t base_paddr = phys_page_base(page) + 0x380U;
    const uintptr_t guard_vaddr = base_vaddr;
    const uintptr_t target_vaddr = base_vaddr + sizeof(uint64_t);
    const uintptr_t adjacent_vaddr = base_vaddr + 2U * sizeof(uint64_t);
    const uintptr_t marker_vaddr = base_vaddr + 3U * sizeof(uint64_t);
    const uintptr_t guard_paddr = base_paddr;
    const uintptr_t target_paddr = base_paddr + sizeof(uint64_t);
    const uintptr_t adjacent_paddr = base_paddr + 2U * sizeof(uint64_t);
    const uintptr_t marker_paddr = base_paddr + 3U * sizeof(uint64_t);
    const uint64_t guard_seed = 0x6d6178756c647270ULL;
    const uint64_t initial_value = 0x0000000000004f00ULL;
    const uint64_t adjacent_seed = 0x7788aa55cc33ee11ULL;
    const uint64_t marker_seed = 0x0123fedc4567ba98ULL;
    const uint64_t marker_final = 0x89abcdef76543210ULL;
    const uint64_t blocked_operand = 0x8000000000005100ULL;
    const uint64_t final_operand = 0x8000000000005000ULL;
    const uint64_t expected_final = final_operand;

    goto_priv(PRIV_M);
    write64(guard_paddr, guard_seed);
    write64(target_paddr, initial_value);
    write64(adjacent_paddr, adjacent_seed);
    write64(marker_paddr, marker_seed);

    goto_priv(PRIV_HS);
    hspt_init();
    sfence_vma();

    ai_memblock_lrsc_trigger_clear_mcontrol6_0();
    ai_memblock_lrsc_trigger_arm_mcontrol6_0(target_vaddr,
        AI_MEMBLOCK_LRSC_TRIGGER_LOAD);
    uint64_t saved_mstatus =
        ai_memblock_lrsc_trigger_enable_breakpoint_delivery();

    goto_priv(PRIV_HS);
    TEST_SETUP_EXCEPT();
    (void)amomaxu_d(target_vaddr, blocked_operand);
    TEST_ASSERT("a load-only mcontrol6 address trigger armed on the doubleword reports breakpoint on amomaxu.d before the unsigned-max RMW can commit",
        excpt.triggered == true &&
        excpt.cause == CAUSE_BKP
    );

    ai_memblock_lrsc_trigger_restore_machine_status(saved_mstatus);
    ai_memblock_lrsc_trigger_clear_mcontrol6_0();

    goto_priv(PRIV_M);
    bool trigger_preserved_words =
        read64(guard_paddr) == guard_seed &&
        read64(target_paddr) == initial_value &&
        read64(adjacent_paddr) == adjacent_seed &&
        read64(marker_paddr) == marker_seed;
    TEST_ASSERT("the load-triggered amomaxu.d breakpoint leaves target, guard, adjacent, and marker doublewords unchanged",
        trigger_preserved_words
    );

    goto_priv(PRIV_HS);
    TEST_SETUP_EXCEPT();
    uint64_t old_value = amomaxu_d(target_vaddr, final_operand);
    TEST_ASSERT("after clearing the load-only trigger, amomaxu.d returns the old doubleword and unsigned-max updates the full 64-bit target",
        excpt.triggered == false &&
        old_value == initial_value &&
        read64(target_vaddr) == expected_final
    );

    write64(marker_vaddr, marker_final);
    TEST_ASSERT("post-clear marker progress and non-target sentinels prove the amomaxu.d load-trigger breakpoint path did not poison following memory operations",
        read64(guard_vaddr) == guard_seed &&
        read64(adjacent_vaddr) == adjacent_seed &&
        read64(marker_vaddr) == marker_final
    );

    goto_priv(PRIV_M);
    TEST_ASSERT("backing memory shows only the post-clear amomaxu.d 64-bit unsigned-max update plus marker progress, with guard and adjacent unchanged",
        read64(guard_paddr) == guard_seed &&
        read64(target_paddr) == expected_final &&
        read64(adjacent_paddr) == adjacent_seed &&
        read64(marker_paddr) == marker_final
    );

    TEST_END();
}

bool ai_micro_memblock_amomaxu_d_store_trigger_breakpoint_suppresses_unsigned_max_doubleword_then_clear_succeeds_corner()
{
    TEST_START();

    const enum test_page page = VSRWX_GRWX;
    const uintptr_t base_vaddr = hs_page_base(page) + 0x3c0U;
    const uintptr_t base_paddr = phys_page_base(page) + 0x3c0U;
    const uintptr_t guard_vaddr = base_vaddr;
    const uintptr_t target_vaddr = base_vaddr + sizeof(uint64_t);
    const uintptr_t adjacent_vaddr = base_vaddr + 2U * sizeof(uint64_t);
    const uintptr_t marker_vaddr = base_vaddr + 3U * sizeof(uint64_t);
    const uintptr_t guard_paddr = base_paddr;
    const uintptr_t target_paddr = base_paddr + sizeof(uint64_t);
    const uintptr_t adjacent_paddr = base_paddr + 2U * sizeof(uint64_t);
    const uintptr_t marker_paddr = base_paddr + 3U * sizeof(uint64_t);
    const uint64_t guard_seed = 0x6d61787573747270ULL;
    const uint64_t initial_value = 0x0000000000005300ULL;
    const uint64_t adjacent_seed = 0x88aa55cc33ee1177ULL;
    const uint64_t marker_seed = 0xfedc0123ba984567ULL;
    const uint64_t marker_final = 0x7654321089abcdefULL;
    const uint64_t blocked_operand = 0x8000000000005500ULL;
    const uint64_t final_operand = 0x8000000000005400ULL;
    const uint64_t expected_final = final_operand;

    goto_priv(PRIV_M);
    write64(guard_paddr, guard_seed);
    write64(target_paddr, initial_value);
    write64(adjacent_paddr, adjacent_seed);
    write64(marker_paddr, marker_seed);

    goto_priv(PRIV_HS);
    hspt_init();
    sfence_vma();

    ai_memblock_lrsc_trigger_clear_mcontrol6_0();
    ai_memblock_lrsc_trigger_arm_mcontrol6_0(target_vaddr,
        AI_MEMBLOCK_LRSC_TRIGGER_STORE);
    uint64_t saved_mstatus =
        ai_memblock_lrsc_trigger_enable_breakpoint_delivery();

    goto_priv(PRIV_HS);
    TEST_SETUP_EXCEPT();
    (void)amomaxu_d(target_vaddr, blocked_operand);
    TEST_ASSERT("a store-only mcontrol6 address trigger armed on the doubleword reports breakpoint on amomaxu.d before the unsigned-max write can commit",
        excpt.triggered == true &&
        excpt.cause == CAUSE_BKP
    );

    ai_memblock_lrsc_trigger_restore_machine_status(saved_mstatus);
    ai_memblock_lrsc_trigger_clear_mcontrol6_0();

    goto_priv(PRIV_M);
    bool trigger_preserved_words =
        read64(guard_paddr) == guard_seed &&
        read64(target_paddr) == initial_value &&
        read64(adjacent_paddr) == adjacent_seed &&
        read64(marker_paddr) == marker_seed;
    TEST_ASSERT("the store-triggered amomaxu.d breakpoint leaves target, guard, adjacent, and marker doublewords unchanged",
        trigger_preserved_words
    );

    goto_priv(PRIV_HS);
    TEST_SETUP_EXCEPT();
    uint64_t old_value = amomaxu_d(target_vaddr, final_operand);
    TEST_ASSERT("after clearing the store-only trigger, amomaxu.d returns the old doubleword and unsigned-max updates the full 64-bit target",
        excpt.triggered == false &&
        old_value == initial_value &&
        read64(target_vaddr) == expected_final
    );

    write64(marker_vaddr, marker_final);
    TEST_ASSERT("post-clear marker progress and non-target sentinels prove the amomaxu.d store-trigger breakpoint path did not poison following memory operations",
        read64(guard_vaddr) == guard_seed &&
        read64(adjacent_vaddr) == adjacent_seed &&
        read64(marker_vaddr) == marker_final
    );

    goto_priv(PRIV_M);
    TEST_ASSERT("backing memory shows only the post-clear amomaxu.d 64-bit unsigned-max update plus marker progress, with guard and adjacent unchanged",
        read64(guard_paddr) == guard_seed &&
        read64(target_paddr) == expected_final &&
        read64(adjacent_paddr) == adjacent_seed &&
        read64(marker_paddr) == marker_final
    );

    TEST_END();
}

bool ai_micro_memblock_amoswap_d_load_trigger_breakpoint_suppresses_swap_doubleword_then_clear_succeeds_corner()
{
    TEST_START();

    const enum test_page page = VSRWX_GRWX;
    const uintptr_t base_vaddr = hs_page_base(page) + 0x400U;
    const uintptr_t base_paddr = phys_page_base(page) + 0x400U;
    const uintptr_t guard_vaddr = base_vaddr;
    const uintptr_t target_vaddr = base_vaddr + sizeof(uint64_t);
    const uintptr_t adjacent_vaddr = base_vaddr + 2U * sizeof(uint64_t);
    const uintptr_t marker_vaddr = base_vaddr + 3U * sizeof(uint64_t);
    const uintptr_t guard_paddr = base_paddr;
    const uintptr_t target_paddr = base_paddr + sizeof(uint64_t);
    const uintptr_t adjacent_paddr = base_paddr + 2U * sizeof(uint64_t);
    const uintptr_t marker_paddr = base_paddr + 3U * sizeof(uint64_t);
    const uint64_t guard_seed = 0x7377646c64727030ULL;
    const uint64_t initial_value = 0x0123456789abcdefULL;
    const uint64_t adjacent_seed = 0xfedcba9876543210ULL;
    const uint64_t marker_seed = 0x0badcafe11223344ULL;
    const uint64_t marker_final = 0x44332211efcdab89ULL;
    const uint64_t blocked_swap = 0xa5a55a5adeadbeefULL;
    const uint64_t final_swap = 0x5eedc0de0ddc0ffeULL;
    const uint64_t expected_final = final_swap;

    goto_priv(PRIV_M);
    write64(guard_paddr, guard_seed);
    write64(target_paddr, initial_value);
    write64(adjacent_paddr, adjacent_seed);
    write64(marker_paddr, marker_seed);

    goto_priv(PRIV_HS);
    hspt_init();
    sfence_vma();

    ai_memblock_lrsc_trigger_clear_mcontrol6_0();
    ai_memblock_lrsc_trigger_arm_mcontrol6_0(target_vaddr,
        AI_MEMBLOCK_LRSC_TRIGGER_LOAD);
    uint64_t saved_mstatus =
        ai_memblock_lrsc_trigger_enable_breakpoint_delivery();

    goto_priv(PRIV_HS);
    TEST_SETUP_EXCEPT();
    (void)amoswap_d(target_vaddr, blocked_swap);
    TEST_ASSERT("a load-only mcontrol6 address trigger armed on the doubleword reports breakpoint on amoswap.d before the 64-bit swap RMW can commit",
        excpt.triggered == true &&
        excpt.cause == CAUSE_BKP
    );

    ai_memblock_lrsc_trigger_restore_machine_status(saved_mstatus);
    ai_memblock_lrsc_trigger_clear_mcontrol6_0();

    goto_priv(PRIV_M);
    bool trigger_preserved_words =
        read64(guard_paddr) == guard_seed &&
        read64(target_paddr) == initial_value &&
        read64(adjacent_paddr) == adjacent_seed &&
        read64(marker_paddr) == marker_seed;
    TEST_ASSERT("the load-triggered amoswap.d breakpoint leaves target, guard, adjacent, and marker doublewords unchanged",
        trigger_preserved_words
    );

    goto_priv(PRIV_HS);
    TEST_SETUP_EXCEPT();
    uint64_t old_value = amoswap_d(target_vaddr, final_swap);
    TEST_ASSERT("after clearing the load-only trigger, amoswap.d returns the old doubleword and replaces the full 64-bit target",
        excpt.triggered == false &&
        old_value == initial_value &&
        read64(target_vaddr) == expected_final
    );

    write64(marker_vaddr, marker_final);
    TEST_ASSERT("post-clear marker progress and non-target sentinels prove the amoswap.d load-trigger breakpoint path did not poison following memory operations",
        read64(guard_vaddr) == guard_seed &&
        read64(adjacent_vaddr) == adjacent_seed &&
        read64(marker_vaddr) == marker_final
    );

    goto_priv(PRIV_M);
    TEST_ASSERT("backing memory shows only the post-clear amoswap.d 64-bit replacement plus marker progress, with guard and adjacent unchanged",
        read64(guard_paddr) == guard_seed &&
        read64(target_paddr) == expected_final &&
        read64(adjacent_paddr) == adjacent_seed &&
        read64(marker_paddr) == marker_final
    );

    TEST_END();
}

bool ai_micro_memblock_amoswap_d_store_trigger_breakpoint_suppresses_swap_doubleword_then_clear_succeeds_corner()
{
    TEST_START();

    const enum test_page page = VSRWX_GRWX;
    const uintptr_t base_vaddr = hs_page_base(page) + 0x440U;
    const uintptr_t base_paddr = phys_page_base(page) + 0x440U;
    const uintptr_t guard_vaddr = base_vaddr;
    const uintptr_t target_vaddr = base_vaddr + sizeof(uint64_t);
    const uintptr_t adjacent_vaddr = base_vaddr + 2U * sizeof(uint64_t);
    const uintptr_t marker_vaddr = base_vaddr + 3U * sizeof(uint64_t);
    const uintptr_t guard_paddr = base_paddr;
    const uintptr_t target_paddr = base_paddr + sizeof(uint64_t);
    const uintptr_t adjacent_paddr = base_paddr + 2U * sizeof(uint64_t);
    const uintptr_t marker_paddr = base_paddr + 3U * sizeof(uint64_t);
    const uint64_t guard_seed = 0x7377647374727030ULL;
    const uint64_t initial_value = 0xfedcba9876543210ULL;
    const uint64_t adjacent_seed = 0x0123456789abcdefULL;
    const uint64_t marker_seed = 0x112233440badcafeULL;
    const uint64_t marker_final = 0xefcdab8944332211ULL;
    const uint64_t blocked_swap = 0x5a5aa5a5beefdeadULL;
    const uint64_t final_swap = 0x0ddc0ffe5eedc0deULL;
    const uint64_t expected_final = final_swap;

    goto_priv(PRIV_M);
    write64(guard_paddr, guard_seed);
    write64(target_paddr, initial_value);
    write64(adjacent_paddr, adjacent_seed);
    write64(marker_paddr, marker_seed);

    goto_priv(PRIV_HS);
    hspt_init();
    sfence_vma();

    ai_memblock_lrsc_trigger_clear_mcontrol6_0();
    ai_memblock_lrsc_trigger_arm_mcontrol6_0(target_vaddr,
        AI_MEMBLOCK_LRSC_TRIGGER_STORE);
    uint64_t saved_mstatus =
        ai_memblock_lrsc_trigger_enable_breakpoint_delivery();

    goto_priv(PRIV_HS);
    TEST_SETUP_EXCEPT();
    (void)amoswap_d(target_vaddr, blocked_swap);
    TEST_ASSERT("a store-only mcontrol6 address trigger armed on the doubleword reports breakpoint on amoswap.d before the 64-bit replacement write can commit",
        excpt.triggered == true &&
        excpt.cause == CAUSE_BKP
    );

    ai_memblock_lrsc_trigger_restore_machine_status(saved_mstatus);
    ai_memblock_lrsc_trigger_clear_mcontrol6_0();

    goto_priv(PRIV_M);
    bool trigger_preserved_words =
        read64(guard_paddr) == guard_seed &&
        read64(target_paddr) == initial_value &&
        read64(adjacent_paddr) == adjacent_seed &&
        read64(marker_paddr) == marker_seed;
    TEST_ASSERT("the store-triggered amoswap.d breakpoint leaves target, guard, adjacent, and marker doublewords unchanged",
        trigger_preserved_words
    );

    goto_priv(PRIV_HS);
    TEST_SETUP_EXCEPT();
    uint64_t old_value = amoswap_d(target_vaddr, final_swap);
    TEST_ASSERT("after clearing the store-only trigger, amoswap.d returns the old doubleword and replaces the full 64-bit target",
        excpt.triggered == false &&
        old_value == initial_value &&
        read64(target_vaddr) == expected_final
    );

    write64(marker_vaddr, marker_final);
    TEST_ASSERT("post-clear marker progress and non-target sentinels prove the amoswap.d store-trigger breakpoint path did not poison following memory operations",
        read64(guard_vaddr) == guard_seed &&
        read64(adjacent_vaddr) == adjacent_seed &&
        read64(marker_vaddr) == marker_final
    );

    goto_priv(PRIV_M);
    TEST_ASSERT("backing memory shows only the post-clear amoswap.d 64-bit replacement plus marker progress, with guard and adjacent unchanged",
        read64(guard_paddr) == guard_seed &&
        read64(target_paddr) == expected_final &&
        read64(adjacent_paddr) == adjacent_seed &&
        read64(marker_paddr) == marker_final
    );

    TEST_END();
}

bool ai_micro_memblock_amoswap_d_load_store_trigger_breakpoint_suppresses_swap_doubleword_then_clear_succeeds_corner()
{
    TEST_START();

    const enum test_page page = VSRWX_GRWX;
    const uintptr_t base_vaddr = hs_page_base(page) + 0x500U;
    const uintptr_t base_paddr = phys_page_base(page) + 0x500U;
    const uintptr_t guard_vaddr = base_vaddr;
    const uintptr_t target_vaddr = base_vaddr + sizeof(uint64_t);
    const uintptr_t adjacent_vaddr = base_vaddr + 2U * sizeof(uint64_t);
    const uintptr_t marker_vaddr = base_vaddr + 3U * sizeof(uint64_t);
    const uintptr_t guard_paddr = base_paddr;
    const uintptr_t target_paddr = base_paddr + sizeof(uint64_t);
    const uintptr_t adjacent_paddr = base_paddr + 2U * sizeof(uint64_t);
    const uintptr_t marker_paddr = base_paddr + 3U * sizeof(uint64_t);
    const uint64_t guard_seed = 0x7377646c73747230ULL;
    const uint64_t initial_value = 0x89abcdef01234567ULL;
    const uint64_t adjacent_seed = 0xfedc001122334455ULL;
    const uint64_t marker_seed = 0xcafebabedeadbeefULL;
    const uint64_t marker_final = 0x1234567890abcdefULL;
    const uint64_t blocked_swap = 0x13579bdf2468ace0ULL;
    const uint64_t final_swap = 0xf00dfacecafefeedULL;
    const uint64_t expected_final = final_swap;

    goto_priv(PRIV_M);
    write64(guard_paddr, guard_seed);
    write64(target_paddr, initial_value);
    write64(adjacent_paddr, adjacent_seed);
    write64(marker_paddr, marker_seed);

    goto_priv(PRIV_HS);
    hspt_init();
    sfence_vma();

    ai_memblock_lrsc_trigger_clear_mcontrol6_0();
    ai_memblock_lrsc_trigger_arm_mcontrol6_0(target_vaddr,
        AI_MEMBLOCK_LRSC_TRIGGER_LOAD | AI_MEMBLOCK_LRSC_TRIGGER_STORE);
    uint64_t saved_mstatus =
        ai_memblock_lrsc_trigger_enable_breakpoint_delivery();

    goto_priv(PRIV_HS);
    TEST_SETUP_EXCEPT();
    (void)amoswap_d(target_vaddr, blocked_swap);
    TEST_ASSERT("a single mcontrol6 address trigger with both load and store access bits reports breakpoint on amoswap.d before the 64-bit replacement can commit",
        excpt.triggered == true &&
        excpt.cause == CAUSE_BKP
    );

    ai_memblock_lrsc_trigger_restore_machine_status(saved_mstatus);
    ai_memblock_lrsc_trigger_clear_mcontrol6_0();

    goto_priv(PRIV_M);
    bool trigger_preserved_words =
        read64(guard_paddr) == guard_seed &&
        read64(target_paddr) == initial_value &&
        read64(adjacent_paddr) == adjacent_seed &&
        read64(marker_paddr) == marker_seed;
    TEST_ASSERT("the combined load-store amoswap.d breakpoint leaves target, guard, adjacent, and marker doublewords unchanged",
        trigger_preserved_words
    );

    goto_priv(PRIV_HS);
    TEST_SETUP_EXCEPT();
    uint64_t old_value = amoswap_d(target_vaddr, final_swap);
    TEST_ASSERT("after clearing the combined load-store trigger, amoswap.d returns the old doubleword and replaces the full 64-bit target",
        excpt.triggered == false &&
        old_value == initial_value &&
        read64(target_vaddr) == expected_final
    );

    write64(marker_vaddr, marker_final);
    TEST_ASSERT("post-clear marker progress and non-target sentinels prove the combined access-bit amoswap.d breakpoint path did not poison following memory operations",
        read64(guard_vaddr) == guard_seed &&
        read64(adjacent_vaddr) == adjacent_seed &&
        read64(marker_vaddr) == marker_final
    );

    goto_priv(PRIV_M);
    TEST_ASSERT("backing memory shows only the post-clear amoswap.d 64-bit replacement plus marker progress, with guard and adjacent unchanged",
        read64(guard_paddr) == guard_seed &&
        read64(target_paddr) == expected_final &&
        read64(adjacent_paddr) == adjacent_seed &&
        read64(marker_paddr) == marker_final
    );

    TEST_END();
}

bool ai_micro_memblock_amomaxu_d_load_store_trigger_breakpoint_suppresses_unsigned_max_doubleword_then_clear_succeeds_corner()
{
    TEST_START();

    const enum test_page page = VSRWX_GRWX;
    const uintptr_t base_vaddr = hs_page_base(page) + 0x540U;
    const uintptr_t base_paddr = phys_page_base(page) + 0x540U;
    const uintptr_t guard_vaddr = base_vaddr;
    const uintptr_t target_vaddr = base_vaddr + sizeof(uint64_t);
    const uintptr_t adjacent_vaddr = base_vaddr + 2U * sizeof(uint64_t);
    const uintptr_t marker_vaddr = base_vaddr + 3U * sizeof(uint64_t);
    const uintptr_t guard_paddr = base_paddr;
    const uintptr_t target_paddr = base_paddr + sizeof(uint64_t);
    const uintptr_t adjacent_paddr = base_paddr + 2U * sizeof(uint64_t);
    const uintptr_t marker_paddr = base_paddr + 3U * sizeof(uint64_t);
    const uint64_t guard_seed = 0x6d6178756c737430ULL;
    const uint64_t initial_value = 0x0000000000006100ULL;
    const uint64_t adjacent_seed = 0xa5a55a5acc33ee11ULL;
    const uint64_t marker_seed = 0x55aa33cc77889900ULL;
    const uint64_t marker_final = 0x0f1e2d3c4b5a6978ULL;
    const uint64_t blocked_operand = 0x8000000000006300ULL;
    const uint64_t final_operand = 0x8000000000006200ULL;
    const uint64_t expected_final = final_operand;

    goto_priv(PRIV_M);
    write64(guard_paddr, guard_seed);
    write64(target_paddr, initial_value);
    write64(adjacent_paddr, adjacent_seed);
    write64(marker_paddr, marker_seed);

    goto_priv(PRIV_HS);
    hspt_init();
    sfence_vma();

    ai_memblock_lrsc_trigger_clear_mcontrol6_0();
    ai_memblock_lrsc_trigger_arm_mcontrol6_0(target_vaddr,
        AI_MEMBLOCK_LRSC_TRIGGER_LOAD | AI_MEMBLOCK_LRSC_TRIGGER_STORE);
    uint64_t saved_mstatus =
        ai_memblock_lrsc_trigger_enable_breakpoint_delivery();

    goto_priv(PRIV_HS);
    TEST_SETUP_EXCEPT();
    (void)amomaxu_d(target_vaddr, blocked_operand);
    TEST_ASSERT("a single mcontrol6 address trigger with both load and store access bits reports breakpoint on amomaxu.d before the unsigned-max update can commit",
        excpt.triggered == true &&
        excpt.cause == CAUSE_BKP
    );

    ai_memblock_lrsc_trigger_restore_machine_status(saved_mstatus);
    ai_memblock_lrsc_trigger_clear_mcontrol6_0();

    goto_priv(PRIV_M);
    bool trigger_preserved_words =
        read64(guard_paddr) == guard_seed &&
        read64(target_paddr) == initial_value &&
        read64(adjacent_paddr) == adjacent_seed &&
        read64(marker_paddr) == marker_seed;
    TEST_ASSERT("the combined load-store amomaxu.d breakpoint leaves target, guard, adjacent, and marker doublewords unchanged",
        trigger_preserved_words
    );

    goto_priv(PRIV_HS);
    TEST_SETUP_EXCEPT();
    uint64_t old_value = amomaxu_d(target_vaddr, final_operand);
    TEST_ASSERT("after clearing the combined load-store trigger, amomaxu.d returns the old doubleword and unsigned-max updates the full 64-bit target",
        excpt.triggered == false &&
        old_value == initial_value &&
        read64(target_vaddr) == expected_final
    );

    write64(marker_vaddr, marker_final);
    TEST_ASSERT("post-clear marker progress and non-target sentinels prove the combined access-bit amomaxu.d breakpoint path did not poison following memory operations",
        read64(guard_vaddr) == guard_seed &&
        read64(adjacent_vaddr) == adjacent_seed &&
        read64(marker_vaddr) == marker_final
    );

    goto_priv(PRIV_M);
    TEST_ASSERT("backing memory shows only the post-clear amomaxu.d 64-bit unsigned-max update plus marker progress, with guard and adjacent unchanged",
        read64(guard_paddr) == guard_seed &&
        read64(target_paddr) == expected_final &&
        read64(adjacent_paddr) == adjacent_seed &&
        read64(marker_paddr) == marker_final
    );

    TEST_END();
}

bool ai_micro_memblock_amomin_d_load_store_trigger_breakpoint_suppresses_signed_min_doubleword_then_clear_succeeds_corner()
{
    TEST_START();

    const enum test_page page = VSRWX_GRWX;
    const uintptr_t base_vaddr = hs_page_base(page) + 0x580U;
    const uintptr_t base_paddr = phys_page_base(page) + 0x580U;
    const uintptr_t guard_vaddr = base_vaddr;
    const uintptr_t target_vaddr = base_vaddr + sizeof(uint64_t);
    const uintptr_t adjacent_vaddr = base_vaddr + 2U * sizeof(uint64_t);
    const uintptr_t marker_vaddr = base_vaddr + 3U * sizeof(uint64_t);
    const uintptr_t guard_paddr = base_paddr;
    const uintptr_t target_paddr = base_paddr + sizeof(uint64_t);
    const uintptr_t adjacent_paddr = base_paddr + 2U * sizeof(uint64_t);
    const uintptr_t marker_paddr = base_paddr + 3U * sizeof(uint64_t);
    const uint64_t guard_seed = 0x6d696e6c73743030ULL;
    const uint64_t initial_value = 0x0000000000007400ULL;
    const uint64_t adjacent_seed = 0x33445566aa55cc77ULL;
    const uint64_t marker_seed = 0x1020304050607080ULL;
    const uint64_t marker_final = 0x8070605040302010ULL;
    const uint64_t blocked_operand = 0xffffffffffff7001ULL;
    const uint64_t final_operand = 0xffffffffffff8e00ULL;
    const uint64_t expected_final = final_operand;

    goto_priv(PRIV_M);
    write64(guard_paddr, guard_seed);
    write64(target_paddr, initial_value);
    write64(adjacent_paddr, adjacent_seed);
    write64(marker_paddr, marker_seed);

    goto_priv(PRIV_HS);
    hspt_init();
    sfence_vma();

    ai_memblock_lrsc_trigger_clear_mcontrol6_0();
    ai_memblock_lrsc_trigger_arm_mcontrol6_0(target_vaddr,
        AI_MEMBLOCK_LRSC_TRIGGER_LOAD | AI_MEMBLOCK_LRSC_TRIGGER_STORE);
    uint64_t saved_mstatus =
        ai_memblock_lrsc_trigger_enable_breakpoint_delivery();

    goto_priv(PRIV_HS);
    TEST_SETUP_EXCEPT();
    (void)amomin_d(target_vaddr, blocked_operand);
    TEST_ASSERT("a single mcontrol6 address trigger with both load and store access bits reports breakpoint on amomin.d before the signed-min update can commit",
        excpt.triggered == true &&
        excpt.cause == CAUSE_BKP
    );

    ai_memblock_lrsc_trigger_restore_machine_status(saved_mstatus);
    ai_memblock_lrsc_trigger_clear_mcontrol6_0();

    goto_priv(PRIV_M);
    bool trigger_preserved_words =
        read64(guard_paddr) == guard_seed &&
        read64(target_paddr) == initial_value &&
        read64(adjacent_paddr) == adjacent_seed &&
        read64(marker_paddr) == marker_seed;
    TEST_ASSERT("the combined load-store amomin.d breakpoint leaves target, guard, adjacent, and marker doublewords unchanged",
        trigger_preserved_words
    );

    goto_priv(PRIV_HS);
    TEST_SETUP_EXCEPT();
    uint64_t old_value = amomin_d(target_vaddr, final_operand);
    TEST_ASSERT("after clearing the combined load-store trigger, amomin.d returns the old doubleword and signed-min updates the full 64-bit target",
        excpt.triggered == false &&
        old_value == initial_value &&
        read64(target_vaddr) == expected_final
    );

    write64(marker_vaddr, marker_final);
    TEST_ASSERT("post-clear marker progress and non-target sentinels prove the combined access-bit amomin.d breakpoint path did not poison following memory operations",
        read64(guard_vaddr) == guard_seed &&
        read64(adjacent_vaddr) == adjacent_seed &&
        read64(marker_vaddr) == marker_final
    );

    goto_priv(PRIV_M);
    TEST_ASSERT("backing memory shows only the post-clear amomin.d 64-bit signed-min update plus marker progress, with guard and adjacent unchanged",
        read64(guard_paddr) == guard_seed &&
        read64(target_paddr) == expected_final &&
        read64(adjacent_paddr) == adjacent_seed &&
        read64(marker_paddr) == marker_final
    );

    TEST_END();
}

bool ai_micro_memblock_amominu_d_load_store_trigger_breakpoint_suppresses_unsigned_min_doubleword_then_clear_succeeds_corner()
{
    TEST_START();

    const enum test_page page = VSRWX_GRWX;
    const uintptr_t base_vaddr = hs_page_base(page) + 0x5c0U;
    const uintptr_t base_paddr = phys_page_base(page) + 0x5c0U;
    const uintptr_t guard_vaddr = base_vaddr;
    const uintptr_t target_vaddr = base_vaddr + sizeof(uint64_t);
    const uintptr_t adjacent_vaddr = base_vaddr + 2U * sizeof(uint64_t);
    const uintptr_t marker_vaddr = base_vaddr + 3U * sizeof(uint64_t);
    const uintptr_t guard_paddr = base_paddr;
    const uintptr_t target_paddr = base_paddr + sizeof(uint64_t);
    const uintptr_t adjacent_paddr = base_paddr + 2U * sizeof(uint64_t);
    const uintptr_t marker_paddr = base_paddr + 3U * sizeof(uint64_t);
    const uint64_t guard_seed = 0x756d696e6c737430ULL;
    const uint64_t initial_value = 0xfedc200000007400ULL;
    const uint64_t adjacent_seed = 0x66778899aabbccddULL;
    const uint64_t marker_seed = 0x0123456789abcdefULL;
    const uint64_t marker_final = 0xfedcba9876543210ULL;
    const uint64_t blocked_operand = 0x0000000000001201ULL;
    const uint64_t final_operand = 0x0000000000004210ULL;
    const uint64_t expected_final = final_operand;

    goto_priv(PRIV_M);
    write64(guard_paddr, guard_seed);
    write64(target_paddr, initial_value);
    write64(adjacent_paddr, adjacent_seed);
    write64(marker_paddr, marker_seed);

    goto_priv(PRIV_HS);
    hspt_init();
    sfence_vma();

    ai_memblock_lrsc_trigger_clear_mcontrol6_0();
    ai_memblock_lrsc_trigger_arm_mcontrol6_0(target_vaddr,
        AI_MEMBLOCK_LRSC_TRIGGER_LOAD | AI_MEMBLOCK_LRSC_TRIGGER_STORE);
    uint64_t saved_mstatus =
        ai_memblock_lrsc_trigger_enable_breakpoint_delivery();

    goto_priv(PRIV_HS);
    TEST_SETUP_EXCEPT();
    (void)amominu_d(target_vaddr, blocked_operand);
    TEST_ASSERT("a single mcontrol6 address trigger with both load and store access bits reports breakpoint on amominu.d before the unsigned-min update can commit",
        excpt.triggered == true &&
        excpt.cause == CAUSE_BKP
    );

    ai_memblock_lrsc_trigger_restore_machine_status(saved_mstatus);
    ai_memblock_lrsc_trigger_clear_mcontrol6_0();

    goto_priv(PRIV_M);
    bool trigger_preserved_words =
        read64(guard_paddr) == guard_seed &&
        read64(target_paddr) == initial_value &&
        read64(adjacent_paddr) == adjacent_seed &&
        read64(marker_paddr) == marker_seed;
    TEST_ASSERT("the combined load-store amominu.d breakpoint leaves target, guard, adjacent, and marker doublewords unchanged",
        trigger_preserved_words
    );

    goto_priv(PRIV_HS);
    TEST_SETUP_EXCEPT();
    uint64_t old_value = amominu_d(target_vaddr, final_operand);
    TEST_ASSERT("after clearing the combined load-store trigger, amominu.d returns the old doubleword and unsigned-min updates the full 64-bit target",
        excpt.triggered == false &&
        old_value == initial_value &&
        read64(target_vaddr) == expected_final
    );

    write64(marker_vaddr, marker_final);
    TEST_ASSERT("post-clear marker progress and non-target sentinels prove the combined access-bit amominu.d breakpoint path did not poison following memory operations",
        read64(guard_vaddr) == guard_seed &&
        read64(adjacent_vaddr) == adjacent_seed &&
        read64(marker_vaddr) == marker_final
    );

    goto_priv(PRIV_M);
    TEST_ASSERT("backing memory shows only the post-clear amominu.d 64-bit unsigned-min update plus marker progress, with guard and adjacent unchanged",
        read64(guard_paddr) == guard_seed &&
        read64(target_paddr) == expected_final &&
        read64(adjacent_paddr) == adjacent_seed &&
        read64(marker_paddr) == marker_final
    );

    TEST_END();
}

bool ai_micro_memblock_amomax_d_load_store_trigger_breakpoint_suppresses_signed_max_doubleword_then_clear_succeeds_corner()
{
    TEST_START();

    const enum test_page page = VSRWX_GRWX;
    const uintptr_t base_vaddr = hs_page_base(page) + 0x600U;
    const uintptr_t base_paddr = phys_page_base(page) + 0x600U;
    const uintptr_t guard_vaddr = base_vaddr;
    const uintptr_t target_vaddr = base_vaddr + sizeof(uint64_t);
    const uintptr_t adjacent_vaddr = base_vaddr + 2U * sizeof(uint64_t);
    const uintptr_t marker_vaddr = base_vaddr + 3U * sizeof(uint64_t);
    const uintptr_t guard_paddr = base_paddr;
    const uintptr_t target_paddr = base_paddr + sizeof(uint64_t);
    const uintptr_t adjacent_paddr = base_paddr + 2U * sizeof(uint64_t);
    const uintptr_t marker_paddr = base_paddr + 3U * sizeof(uint64_t);
    const uint64_t guard_seed = 0x6d61786c73743030ULL;
    const uint64_t initial_value = 0xfffffffffff06000ULL;
    const uint64_t adjacent_seed = 0x8899aabbccddeeffULL;
    const uint64_t marker_seed = 0x76543210fedcba98ULL;
    const uint64_t marker_final = 0x89abcdef01234567ULL;
    const uint64_t blocked_operand = 0x0000000000007300ULL;
    const uint64_t final_operand = 0x0000000000006a00ULL;
    const uint64_t expected_final = final_operand;

    goto_priv(PRIV_M);
    write64(guard_paddr, guard_seed);
    write64(target_paddr, initial_value);
    write64(adjacent_paddr, adjacent_seed);
    write64(marker_paddr, marker_seed);

    goto_priv(PRIV_HS);
    hspt_init();
    sfence_vma();

    ai_memblock_lrsc_trigger_clear_mcontrol6_0();
    ai_memblock_lrsc_trigger_arm_mcontrol6_0(target_vaddr,
        AI_MEMBLOCK_LRSC_TRIGGER_LOAD | AI_MEMBLOCK_LRSC_TRIGGER_STORE);
    uint64_t saved_mstatus =
        ai_memblock_lrsc_trigger_enable_breakpoint_delivery();

    goto_priv(PRIV_HS);
    TEST_SETUP_EXCEPT();
    (void)amomax_d(target_vaddr, blocked_operand);
    TEST_ASSERT("a single mcontrol6 address trigger with both load and store access bits reports breakpoint on amomax.d before the signed-max update can commit",
        excpt.triggered == true &&
        excpt.cause == CAUSE_BKP
    );

    ai_memblock_lrsc_trigger_restore_machine_status(saved_mstatus);
    ai_memblock_lrsc_trigger_clear_mcontrol6_0();

    goto_priv(PRIV_M);
    bool trigger_preserved_words =
        read64(guard_paddr) == guard_seed &&
        read64(target_paddr) == initial_value &&
        read64(adjacent_paddr) == adjacent_seed &&
        read64(marker_paddr) == marker_seed;
    TEST_ASSERT("the combined load-store amomax.d breakpoint leaves target, guard, adjacent, and marker doublewords unchanged",
        trigger_preserved_words
    );

    goto_priv(PRIV_HS);
    TEST_SETUP_EXCEPT();
    uint64_t old_value = amomax_d(target_vaddr, final_operand);
    TEST_ASSERT("after clearing the combined load-store trigger, amomax.d returns the old doubleword and signed-max updates the full 64-bit target",
        excpt.triggered == false &&
        old_value == initial_value &&
        read64(target_vaddr) == expected_final
    );

    write64(marker_vaddr, marker_final);
    TEST_ASSERT("post-clear marker progress and non-target sentinels prove the combined access-bit amomax.d breakpoint path did not poison following memory operations",
        read64(guard_vaddr) == guard_seed &&
        read64(adjacent_vaddr) == adjacent_seed &&
        read64(marker_vaddr) == marker_final
    );

    goto_priv(PRIV_M);
    TEST_ASSERT("backing memory shows only the post-clear amomax.d 64-bit signed-max update plus marker progress, with guard and adjacent unchanged",
        read64(guard_paddr) == guard_seed &&
        read64(target_paddr) == expected_final &&
        read64(adjacent_paddr) == adjacent_seed &&
        read64(marker_paddr) == marker_final
    );

    TEST_END();
}

bool ai_micro_memblock_amoand_d_load_store_trigger_breakpoint_suppresses_logic_and_doubleword_then_clear_succeeds_corner()
{
    TEST_START();

    const enum test_page page = VSRWX_GRWX;
    const uintptr_t base_vaddr = hs_page_base(page) + 0x640U;
    const uintptr_t base_paddr = phys_page_base(page) + 0x640U;
    const uintptr_t guard_vaddr = base_vaddr;
    const uintptr_t target_vaddr = base_vaddr + sizeof(uint64_t);
    const uintptr_t adjacent_vaddr = base_vaddr + 2U * sizeof(uint64_t);
    const uintptr_t marker_vaddr = base_vaddr + 3U * sizeof(uint64_t);
    const uintptr_t guard_paddr = base_paddr;
    const uintptr_t target_paddr = base_paddr + sizeof(uint64_t);
    const uintptr_t adjacent_paddr = base_paddr + 2U * sizeof(uint64_t);
    const uintptr_t marker_paddr = base_paddr + 3U * sizeof(uint64_t);
    const uint64_t guard_seed = 0x616e646c73743030ULL;
    const uint64_t initial_value = 0xf0ff55aac3d2e1f0ULL;
    const uint64_t adjacent_seed = 0x7e6d5c4b3a291807ULL;
    const uint64_t marker_seed = 0x0123fedc4567ba98ULL;
    const uint64_t marker_final = 0x98ba7654dcfe3210ULL;
    const uint64_t blocked_operand = 0x00ff00ff00ff00ffULL;
    const uint64_t final_operand = 0x0ff0ff000ff0ff00ULL;
    const uint64_t expected_final = initial_value & final_operand;

    goto_priv(PRIV_M);
    write64(guard_paddr, guard_seed);
    write64(target_paddr, initial_value);
    write64(adjacent_paddr, adjacent_seed);
    write64(marker_paddr, marker_seed);

    goto_priv(PRIV_HS);
    hspt_init();
    sfence_vma();

    ai_memblock_lrsc_trigger_clear_mcontrol6_0();
    ai_memblock_lrsc_trigger_arm_mcontrol6_0(target_vaddr,
        AI_MEMBLOCK_LRSC_TRIGGER_LOAD | AI_MEMBLOCK_LRSC_TRIGGER_STORE);
    uint64_t saved_mstatus =
        ai_memblock_lrsc_trigger_enable_breakpoint_delivery();

    goto_priv(PRIV_HS);
    TEST_SETUP_EXCEPT();
    (void)amoand_d(target_vaddr, blocked_operand);
    TEST_ASSERT("a single mcontrol6 address trigger with both load and store access bits reports breakpoint on amoand.d before the logic-and update can commit",
        excpt.triggered == true &&
        excpt.cause == CAUSE_BKP
    );

    ai_memblock_lrsc_trigger_restore_machine_status(saved_mstatus);
    ai_memblock_lrsc_trigger_clear_mcontrol6_0();

    goto_priv(PRIV_M);
    bool trigger_preserved_words =
        read64(guard_paddr) == guard_seed &&
        read64(target_paddr) == initial_value &&
        read64(adjacent_paddr) == adjacent_seed &&
        read64(marker_paddr) == marker_seed;
    TEST_ASSERT("the combined load-store amoand.d breakpoint leaves target, guard, adjacent, and marker doublewords unchanged",
        trigger_preserved_words
    );

    goto_priv(PRIV_HS);
    TEST_SETUP_EXCEPT();
    uint64_t old_value = amoand_d(target_vaddr, final_operand);
    TEST_ASSERT("after clearing the combined load-store trigger, amoand.d returns the old doubleword and AND updates the full 64-bit target",
        excpt.triggered == false &&
        old_value == initial_value &&
        read64(target_vaddr) == expected_final
    );

    write64(marker_vaddr, marker_final);
    TEST_ASSERT("post-clear marker progress and non-target sentinels prove the combined access-bit amoand.d breakpoint path did not poison following memory operations",
        read64(guard_vaddr) == guard_seed &&
        read64(adjacent_vaddr) == adjacent_seed &&
        read64(marker_vaddr) == marker_final
    );

    goto_priv(PRIV_M);
    TEST_ASSERT("backing memory shows only the post-clear amoand.d 64-bit AND update plus marker progress, with guard and adjacent unchanged",
        read64(guard_paddr) == guard_seed &&
        read64(target_paddr) == expected_final &&
        read64(adjacent_paddr) == adjacent_seed &&
        read64(marker_paddr) == marker_final
    );

    TEST_END();
}

bool ai_micro_memblock_amoor_d_load_store_trigger_breakpoint_suppresses_logic_or_doubleword_then_clear_succeeds_corner()
{
    TEST_START();

    const enum test_page page = VSRWX_GRWX;
    const uintptr_t base_vaddr = hs_page_base(page) + 0x680U;
    const uintptr_t base_paddr = phys_page_base(page) + 0x680U;
    const uintptr_t guard_vaddr = base_vaddr;
    const uintptr_t target_vaddr = base_vaddr + sizeof(uint64_t);
    const uintptr_t adjacent_vaddr = base_vaddr + 2U * sizeof(uint64_t);
    const uintptr_t marker_vaddr = base_vaddr + 3U * sizeof(uint64_t);
    const uintptr_t guard_paddr = base_paddr;
    const uintptr_t target_paddr = base_paddr + sizeof(uint64_t);
    const uintptr_t adjacent_paddr = base_paddr + 2U * sizeof(uint64_t);
    const uintptr_t marker_paddr = base_paddr + 3U * sizeof(uint64_t);
    const uint64_t guard_seed = 0x6f72646c73743030ULL;
    const uint64_t initial_value = 0x12003400c3d2e1f0ULL;
    const uint64_t adjacent_seed = 0x31425364758697a8ULL;
    const uint64_t marker_seed = 0x0badf00d5eedfaceULL;
    const uint64_t marker_final = 0xecafdeed0df0dab0ULL;
    const uint64_t blocked_operand = 0x00f0004f0000004fULL;
    const uint64_t final_operand = 0x0000c0ff0000c0ffULL;
    const uint64_t expected_final = initial_value | final_operand;

    goto_priv(PRIV_M);
    write64(guard_paddr, guard_seed);
    write64(target_paddr, initial_value);
    write64(adjacent_paddr, adjacent_seed);
    write64(marker_paddr, marker_seed);

    goto_priv(PRIV_HS);
    hspt_init();
    sfence_vma();

    ai_memblock_lrsc_trigger_clear_mcontrol6_0();
    ai_memblock_lrsc_trigger_arm_mcontrol6_0(target_vaddr,
        AI_MEMBLOCK_LRSC_TRIGGER_LOAD | AI_MEMBLOCK_LRSC_TRIGGER_STORE);
    uint64_t saved_mstatus =
        ai_memblock_lrsc_trigger_enable_breakpoint_delivery();

    goto_priv(PRIV_HS);
    TEST_SETUP_EXCEPT();
    (void)amoor_d(target_vaddr, blocked_operand);
    TEST_ASSERT("a single mcontrol6 address trigger with both load and store access bits reports breakpoint on amoor.d before the logic-or update can commit",
        excpt.triggered == true &&
        excpt.cause == CAUSE_BKP
    );

    ai_memblock_lrsc_trigger_restore_machine_status(saved_mstatus);
    ai_memblock_lrsc_trigger_clear_mcontrol6_0();

    goto_priv(PRIV_M);
    bool trigger_preserved_words =
        read64(guard_paddr) == guard_seed &&
        read64(target_paddr) == initial_value &&
        read64(adjacent_paddr) == adjacent_seed &&
        read64(marker_paddr) == marker_seed;
    TEST_ASSERT("the combined load-store amoor.d breakpoint leaves target, guard, adjacent, and marker doublewords unchanged",
        trigger_preserved_words
    );

    goto_priv(PRIV_HS);
    TEST_SETUP_EXCEPT();
    uint64_t old_value = amoor_d(target_vaddr, final_operand);
    TEST_ASSERT("after clearing the combined load-store trigger, amoor.d returns the old doubleword and OR updates the full 64-bit target",
        excpt.triggered == false &&
        old_value == initial_value &&
        read64(target_vaddr) == expected_final
    );

    write64(marker_vaddr, marker_final);
    TEST_ASSERT("post-clear marker progress and non-target sentinels prove the combined access-bit amoor.d breakpoint path did not poison following memory operations",
        read64(guard_vaddr) == guard_seed &&
        read64(adjacent_vaddr) == adjacent_seed &&
        read64(marker_vaddr) == marker_final
    );

    goto_priv(PRIV_M);
    TEST_ASSERT("backing memory shows only the post-clear amoor.d 64-bit OR update plus marker progress, with guard and adjacent unchanged",
        read64(guard_paddr) == guard_seed &&
        read64(target_paddr) == expected_final &&
        read64(adjacent_paddr) == adjacent_seed &&
        read64(marker_paddr) == marker_final
    );

    TEST_END();
}

bool ai_micro_memblock_amoxor_d_load_store_trigger_breakpoint_suppresses_logic_xor_doubleword_then_clear_succeeds_corner()
{
    TEST_START();

    const enum test_page page = VSRWX_GRWX;
    const uintptr_t base_vaddr = hs_page_base(page) + 0x6c0U;
    const uintptr_t base_paddr = phys_page_base(page) + 0x6c0U;
    const uintptr_t guard_vaddr = base_vaddr;
    const uintptr_t target_vaddr = base_vaddr + sizeof(uint64_t);
    const uintptr_t adjacent_vaddr = base_vaddr + 2U * sizeof(uint64_t);
    const uintptr_t marker_vaddr = base_vaddr + 3U * sizeof(uint64_t);
    const uintptr_t guard_paddr = base_paddr;
    const uintptr_t target_paddr = base_paddr + sizeof(uint64_t);
    const uintptr_t adjacent_paddr = base_paddr + 2U * sizeof(uint64_t);
    const uintptr_t marker_paddr = base_paddr + 3U * sizeof(uint64_t);
    const uint64_t guard_seed = 0x786f726c73743030ULL;
    const uint64_t initial_value = 0x5a0f33c3a5c3e1d0ULL;
    const uint64_t adjacent_seed = 0x4c3b2a1908f7e6d5ULL;
    const uint64_t marker_seed = 0xface5eed0badf00dULL;
    const uint64_t marker_final = 0x0df0dab0ecafdeedULL;
    const uint64_t blocked_operand = 0x00ff0f0f00ff0f0fULL;
    const uint64_t final_operand = 0x0f00f0ff00ff0f00ULL;
    const uint64_t expected_final = initial_value ^ final_operand;

    goto_priv(PRIV_M);
    write64(guard_paddr, guard_seed);
    write64(target_paddr, initial_value);
    write64(adjacent_paddr, adjacent_seed);
    write64(marker_paddr, marker_seed);

    goto_priv(PRIV_HS);
    hspt_init();
    sfence_vma();

    ai_memblock_lrsc_trigger_clear_mcontrol6_0();
    ai_memblock_lrsc_trigger_arm_mcontrol6_0(target_vaddr,
        AI_MEMBLOCK_LRSC_TRIGGER_LOAD | AI_MEMBLOCK_LRSC_TRIGGER_STORE);
    uint64_t saved_mstatus =
        ai_memblock_lrsc_trigger_enable_breakpoint_delivery();

    goto_priv(PRIV_HS);
    TEST_SETUP_EXCEPT();
    (void)amoxor_d(target_vaddr, blocked_operand);
    TEST_ASSERT("a single mcontrol6 address trigger with both load and store access bits reports breakpoint on amoxor.d before the logic-xor update can commit",
        excpt.triggered == true &&
        excpt.cause == CAUSE_BKP
    );

    ai_memblock_lrsc_trigger_restore_machine_status(saved_mstatus);
    ai_memblock_lrsc_trigger_clear_mcontrol6_0();

    goto_priv(PRIV_M);
    bool trigger_preserved_words =
        read64(guard_paddr) == guard_seed &&
        read64(target_paddr) == initial_value &&
        read64(adjacent_paddr) == adjacent_seed &&
        read64(marker_paddr) == marker_seed;
    TEST_ASSERT("the combined load-store amoxor.d breakpoint leaves target, guard, adjacent, and marker doublewords unchanged",
        trigger_preserved_words
    );

    goto_priv(PRIV_HS);
    TEST_SETUP_EXCEPT();
    uint64_t old_value = amoxor_d(target_vaddr, final_operand);
    TEST_ASSERT("after clearing the combined load-store trigger, amoxor.d returns the old doubleword and XOR updates the full 64-bit target",
        excpt.triggered == false &&
        old_value == initial_value &&
        read64(target_vaddr) == expected_final
    );

    write64(marker_vaddr, marker_final);
    TEST_ASSERT("post-clear marker progress and non-target sentinels prove the combined access-bit amoxor.d breakpoint path did not poison following memory operations",
        read64(guard_vaddr) == guard_seed &&
        read64(adjacent_vaddr) == adjacent_seed &&
        read64(marker_vaddr) == marker_final
    );

    goto_priv(PRIV_M);
    TEST_ASSERT("backing memory shows only the post-clear amoxor.d 64-bit XOR update plus marker progress, with guard and adjacent unchanged",
        read64(guard_paddr) == guard_seed &&
        read64(target_paddr) == expected_final &&
        read64(adjacent_paddr) == adjacent_seed &&
        read64(marker_paddr) == marker_final
    );

    TEST_END();
}

bool ai_micro_memblock_amoadd_d_load_store_trigger_breakpoint_suppresses_add_doubleword_then_clear_succeeds_corner()
{
    TEST_START();

    const enum test_page page = VSRWX_GRWX;
    const uintptr_t base_vaddr = hs_page_base(page) + 0x700U;
    const uintptr_t base_paddr = phys_page_base(page) + 0x700U;
    const uintptr_t guard_vaddr = base_vaddr;
    const uintptr_t target_vaddr = base_vaddr + sizeof(uint64_t);
    const uintptr_t adjacent_vaddr = base_vaddr + 2U * sizeof(uint64_t);
    const uintptr_t marker_vaddr = base_vaddr + 3U * sizeof(uint64_t);
    const uintptr_t guard_paddr = base_paddr;
    const uintptr_t target_paddr = base_paddr + sizeof(uint64_t);
    const uintptr_t adjacent_paddr = base_paddr + 2U * sizeof(uint64_t);
    const uintptr_t marker_paddr = base_paddr + 3U * sizeof(uint64_t);
    const uint64_t guard_seed = 0x6164646c73743030ULL;
    const uint64_t initial_value = 0x00ff00ff00ff00ffULL;
    const uint64_t adjacent_seed = 0x5a5aa5a5cc33ee11ULL;
    const uint64_t marker_seed = 0x1029384756abcdefULL;
    const uint64_t marker_final = 0xfedcba7654320198ULL;
    const uint64_t blocked_addend = 0x1111111111111111ULL;
    const uint64_t final_addend = 0x0001000100010001ULL;
    const uint64_t expected_final = initial_value + final_addend;

    goto_priv(PRIV_M);
    write64(guard_paddr, guard_seed);
    write64(target_paddr, initial_value);
    write64(adjacent_paddr, adjacent_seed);
    write64(marker_paddr, marker_seed);

    goto_priv(PRIV_HS);
    hspt_init();
    sfence_vma();

    ai_memblock_lrsc_trigger_clear_mcontrol6_0();
    ai_memblock_lrsc_trigger_arm_mcontrol6_0(target_vaddr,
        AI_MEMBLOCK_LRSC_TRIGGER_LOAD | AI_MEMBLOCK_LRSC_TRIGGER_STORE);
    uint64_t saved_mstatus =
        ai_memblock_lrsc_trigger_enable_breakpoint_delivery();

    goto_priv(PRIV_HS);
    TEST_SETUP_EXCEPT();
    (void)amoadd_d(target_vaddr, blocked_addend);
    TEST_ASSERT("a single mcontrol6 address trigger with both load and store access bits reports breakpoint on amoadd.d before the 64-bit add update can commit",
        excpt.triggered == true &&
        excpt.cause == CAUSE_BKP
    );

    ai_memblock_lrsc_trigger_restore_machine_status(saved_mstatus);
    ai_memblock_lrsc_trigger_clear_mcontrol6_0();

    goto_priv(PRIV_M);
    bool trigger_preserved_words =
        read64(guard_paddr) == guard_seed &&
        read64(target_paddr) == initial_value &&
        read64(adjacent_paddr) == adjacent_seed &&
        read64(marker_paddr) == marker_seed;
    TEST_ASSERT("the combined load-store amoadd.d breakpoint leaves target, guard, adjacent, and marker doublewords unchanged",
        trigger_preserved_words
    );

    goto_priv(PRIV_HS);
    TEST_SETUP_EXCEPT();
    uint64_t old_value = amoadd_d(target_vaddr, final_addend);
    TEST_ASSERT("after clearing the combined load-store trigger, amoadd.d returns the old doubleword and ADD updates the full 64-bit target",
        excpt.triggered == false &&
        old_value == initial_value &&
        read64(target_vaddr) == expected_final
    );

    write64(marker_vaddr, marker_final);
    TEST_ASSERT("post-clear marker progress and non-target sentinels prove the combined access-bit amoadd.d breakpoint path did not poison following memory operations",
        read64(guard_vaddr) == guard_seed &&
        read64(adjacent_vaddr) == adjacent_seed &&
        read64(marker_vaddr) == marker_final
    );

    goto_priv(PRIV_M);
    TEST_ASSERT("backing memory shows only the post-clear amoadd.d 64-bit add update plus marker progress, with guard and adjacent unchanged",
        read64(guard_paddr) == guard_seed &&
        read64(target_paddr) == expected_final &&
        read64(adjacent_paddr) == adjacent_seed &&
        read64(marker_paddr) == marker_final
    );

    TEST_END();
}

bool ai_micro_memblock_amoadd_w_lower_word_load_store_trigger_breakpoint_suppresses_add_lane_then_clear_succeeds_corner()
{
    TEST_START();

    const enum test_page page = VSRWX_GRWX;
    const uintptr_t base_vaddr = hs_page_base(page) + 0x740U;
    const uintptr_t base_paddr = phys_page_base(page) + 0x740U;
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
    const uint64_t guard_seed = 0x6164646c77303030ULL;
    const uint32_t lower_initial = 0x7ffffff0U;
    const uint32_t upper_initial = 0x2468ace0U;
    const uint64_t initial_value =
        ((uint64_t)upper_initial << 32) | lower_initial;
    const uint64_t adjacent_seed = 0x0f1e2d3c4b5a6978ULL;
    const uint64_t marker_seed = 0x55aa33cc77889900ULL;
    const uint64_t marker_final = 0x00998877cc33aa55ULL;
    const uint32_t blocked_addend = 0x01020304U;
    const uint32_t final_addend = 0x20U;
    const uint32_t expected_lower = lower_initial + final_addend;
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
        AI_MEMBLOCK_LRSC_TRIGGER_LOAD | AI_MEMBLOCK_LRSC_TRIGGER_STORE);
    uint64_t saved_mstatus =
        ai_memblock_lrsc_trigger_enable_breakpoint_delivery();

    goto_priv(PRIV_HS);
    TEST_SETUP_EXCEPT();
    (void)amoadd_w(lower_word_vaddr, blocked_addend);
    TEST_ASSERT("a single mcontrol6 address trigger with both load and store access bits reports breakpoint on lower-word amoadd.w before the lane add can commit",
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
    TEST_ASSERT("the combined load-store lower-word amoadd.w breakpoint leaves the full doubleword, guard, adjacent, and marker words unchanged",
        trigger_preserved_words
    );

    goto_priv(PRIV_HS);
    TEST_SETUP_EXCEPT();
    uint32_t old_lower = amoadd_w(lower_word_vaddr, final_addend);
    TEST_ASSERT("after clearing the combined load-store trigger, amoadd.w returns the old lower word and ADD updates only that 32-bit lane",
        excpt.triggered == false &&
        old_lower == lower_initial &&
        read64(target_vaddr) == expected_final &&
        read32(lower_word_vaddr) == expected_lower &&
        read32(upper_word_vaddr) == upper_initial
    );

    write64(marker_vaddr, marker_final);
    TEST_ASSERT("post-clear marker progress and non-target sentinels prove the combined access-bit lower-word amoadd.w breakpoint path did not poison following memory operations",
        read64(guard_vaddr) == guard_seed &&
        read64(adjacent_vaddr) == adjacent_seed &&
        read64(marker_vaddr) == marker_final
    );

    goto_priv(PRIV_M);
    TEST_ASSERT("backing memory shows only the post-clear lower-word amoadd.w lane update plus marker progress, with upper lane, guard, and adjacent unchanged",
        read64(guard_paddr) == guard_seed &&
        read64(target_paddr) == expected_final &&
        read32(lower_word_paddr) == expected_lower &&
        read32(upper_word_paddr) == upper_initial &&
        read64(adjacent_paddr) == adjacent_seed &&
        read64(marker_paddr) == marker_final
    );

    TEST_END();
}

bool ai_micro_memblock_amoadd_d_load_store_trigger_repeated_breakpoint_suppresses_add_then_clear_succeeds_corner()
{
    TEST_START();

    const enum test_page page = VSRWX_GRWX;
    const uintptr_t base_vaddr = hs_page_base(page) + 0x900U;
    const uintptr_t base_paddr = phys_page_base(page) + 0x900U;
    const uintptr_t guard_vaddr = base_vaddr;
    const uintptr_t target_vaddr = base_vaddr + sizeof(uint64_t);
    const uintptr_t adjacent_vaddr = base_vaddr + 2U * sizeof(uint64_t);
    const uintptr_t marker_vaddr = base_vaddr + 3U * sizeof(uint64_t);
    const uintptr_t guard_paddr = base_paddr;
    const uintptr_t target_paddr = base_paddr + sizeof(uint64_t);
    const uintptr_t adjacent_paddr = base_paddr + 2U * sizeof(uint64_t);
    const uintptr_t marker_paddr = base_paddr + 3U * sizeof(uint64_t);
    const uint64_t guard_seed = 0x6164647270303030ULL;
    const uint64_t initial_value = 0x00fe00fd00fc00fbULL;
    const uint64_t adjacent_seed = 0x2468ace013579bdfULL;
    const uint64_t marker_seed = 0xfedcba9876543210ULL;
    const uint64_t marker_final = 0x1029384756abcdefULL;
    const uint64_t first_blocked_addend = 0x1111111111111111ULL;
    const uint64_t second_blocked_addend = 0x2222222222222222ULL;
    const uint64_t final_addend = 0x0001000100010001ULL;
    const uint64_t followup_addend = 0x0000000000000020ULL;
    const uint64_t expected_final = initial_value + final_addend;
    const uint64_t expected_followup = expected_final + followup_addend;

    goto_priv(PRIV_M);
    write64(guard_paddr, guard_seed);
    write64(target_paddr, initial_value);
    write64(adjacent_paddr, adjacent_seed);
    write64(marker_paddr, marker_seed);

    goto_priv(PRIV_HS);
    hspt_init();
    sfence_vma();

    ai_memblock_lrsc_trigger_clear_mcontrol6_0();
    ai_memblock_lrsc_trigger_arm_mcontrol6_0(target_vaddr,
        AI_MEMBLOCK_LRSC_TRIGGER_LOAD | AI_MEMBLOCK_LRSC_TRIGGER_STORE);
    uint64_t saved_mstatus =
        ai_memblock_lrsc_trigger_enable_breakpoint_delivery();

    goto_priv(PRIV_HS);
    TEST_SETUP_EXCEPT();
    (void)amoadd_d(target_vaddr, first_blocked_addend);
    TEST_ASSERT("the first same-slot load-store mcontrol6 hit on amoadd.d reports breakpoint before the add RMW can commit",
        excpt.triggered == true &&
        excpt.cause == CAUSE_BKP
    );

    ai_memblock_lrsc_trigger_reenable_breakpoint_delivery();

    goto_priv(PRIV_HS);
    TEST_SETUP_EXCEPT();
    (void)amoadd_d(target_vaddr, second_blocked_addend);
    TEST_ASSERT("a second consecutive same-slot load-store mcontrol6 hit on amoadd.d still reports breakpoint and does not leak a completed add",
        excpt.triggered == true &&
        excpt.cause == CAUSE_BKP
    );

    ai_memblock_lrsc_trigger_restore_machine_status(saved_mstatus);
    ai_memblock_lrsc_trigger_clear_mcontrol6_0();

    goto_priv(PRIV_M);
    bool repeated_breakpoints_preserved_words =
        read64(guard_paddr) == guard_seed &&
        read64(target_paddr) == initial_value &&
        read64(adjacent_paddr) == adjacent_seed &&
        read64(marker_paddr) == marker_seed;
    TEST_ASSERT("two repeated combined-trigger amoadd.d breakpoint hits leave target, guard, adjacent, and marker doublewords unchanged",
        repeated_breakpoints_preserved_words
    );

    goto_priv(PRIV_HS);
    TEST_SETUP_EXCEPT();
    uint64_t old_value = amoadd_d(target_vaddr, final_addend);
    TEST_ASSERT("after clearing the repeated combined trigger, amoadd.d returns the old doubleword and ADD updates the full 64-bit target exactly once",
        excpt.triggered == false &&
        old_value == initial_value &&
        read64(target_vaddr) == expected_final
    );

    TEST_SETUP_EXCEPT();
    uint64_t followup_old = amoadd_d(target_vaddr, followup_addend);
    TEST_ASSERT("a follow-up amoadd.d after the recovered update observes the committed value and proves the add pipeline is not poisoned by repeated breakpoints",
        excpt.triggered == false &&
        followup_old == expected_final &&
        read64(target_vaddr) == expected_followup
    );

    write64(marker_vaddr, marker_final);
    TEST_ASSERT("post-clear marker progress and non-target sentinels prove repeated combined-trigger amoadd.d traps did not poison following memory operations",
        read64(guard_vaddr) == guard_seed &&
        read64(adjacent_vaddr) == adjacent_seed &&
        read64(marker_vaddr) == marker_final
    );

    goto_priv(PRIV_M);
    TEST_ASSERT("backing memory shows only the post-clear amoadd.d updates plus marker progress after repeated same-slot breakpoints",
        read64(guard_paddr) == guard_seed &&
        read64(target_paddr) == expected_followup &&
        read64(adjacent_paddr) == adjacent_seed &&
        read64(marker_paddr) == marker_final
    );

    TEST_END();
}
