#include "ai_micro_memblock_lrsc_trigger_common.h"

bool ai_micro_memblock_amoadd_w_upper_word_load_store_trigger_breakpoint_suppresses_add_lane_then_clear_succeeds_corner()
{
    TEST_START();

    const enum test_page page = VSRWX_GRWX;
    const uintptr_t base_vaddr = hs_page_base(page) + 0x780U;
    const uintptr_t base_paddr = phys_page_base(page) + 0x780U;
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
    const uint64_t guard_seed = 0x6164647577303030ULL;
    const uint32_t lower_initial = 0x13579bdfU;
    const uint32_t upper_initial = 0x6ffffff0U;
    const uint64_t initial_value =
        ((uint64_t)upper_initial << 32) | lower_initial;
    const uint64_t adjacent_seed = 0x2468ace013579bdfULL;
    const uint64_t marker_seed = 0x1029384756abcdefULL;
    const uint64_t marker_final = 0xfedcba6547382910ULL;
    const uint32_t blocked_addend = 0x01010101U;
    const uint32_t final_addend = 0x10U;
    const uint32_t expected_upper = upper_initial + final_addend;
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
        AI_MEMBLOCK_LRSC_TRIGGER_LOAD | AI_MEMBLOCK_LRSC_TRIGGER_STORE);
    uint64_t saved_mstatus =
        ai_memblock_lrsc_trigger_enable_breakpoint_delivery();

    goto_priv(PRIV_HS);
    TEST_SETUP_EXCEPT();
    (void)amoadd_w(upper_word_vaddr, blocked_addend);
    TEST_ASSERT("a single mcontrol6 address trigger with both load and store access bits reports breakpoint on upper-word amoadd.w before the lane add can commit",
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
    TEST_ASSERT("the combined load-store upper-word amoadd.w breakpoint leaves the full doubleword, guard, adjacent, and marker words unchanged",
        trigger_preserved_words
    );

    goto_priv(PRIV_HS);
    TEST_SETUP_EXCEPT();
    uint32_t old_upper = amoadd_w(upper_word_vaddr, final_addend);
    TEST_ASSERT("after clearing the combined load-store trigger, amoadd.w returns the old upper word and ADD updates only that 32-bit lane",
        excpt.triggered == false &&
        old_upper == upper_initial &&
        read64(target_vaddr) == expected_final &&
        read32(lower_word_vaddr) == lower_initial &&
        read32(upper_word_vaddr) == expected_upper
    );

    write64(marker_vaddr, marker_final);
    TEST_ASSERT("post-clear marker progress and non-target sentinels prove the combined access-bit upper-word amoadd.w breakpoint path did not poison following memory operations",
        read64(guard_vaddr) == guard_seed &&
        read64(adjacent_vaddr) == adjacent_seed &&
        read64(marker_vaddr) == marker_final
    );

    goto_priv(PRIV_M);
    TEST_ASSERT("backing memory shows only the post-clear upper-word amoadd.w lane update plus marker progress, with lower lane, guard, and adjacent unchanged",
        read64(guard_paddr) == guard_seed &&
        read64(target_paddr) == expected_final &&
        read32(lower_word_paddr) == lower_initial &&
        read32(upper_word_paddr) == expected_upper &&
        read64(adjacent_paddr) == adjacent_seed &&
        read64(marker_paddr) == marker_final
    );

    TEST_END();
}

bool ai_micro_memblock_amoswap_w_lower_word_load_store_trigger_breakpoint_suppresses_replacement_lane_then_clear_succeeds_corner()
{
    TEST_START();

    const enum test_page page = VSRWX_GRWX;
    const uintptr_t base_vaddr = hs_page_base(page) + 0x7c0U;
    const uintptr_t base_paddr = phys_page_base(page) + 0x7c0U;
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
    const uint64_t guard_seed = 0x7377706c73303030ULL;
    const uint32_t lower_initial = 0x31415926U;
    const uint32_t upper_initial = 0xabcdef12U;
    const uint64_t initial_value =
        ((uint64_t)upper_initial << 32) | lower_initial;
    const uint64_t adjacent_seed = 0x0123456789abcdefULL;
    const uint64_t marker_seed = 0xf0e1d2c3b4a59687ULL;
    const uint64_t marker_final = 0x78695a4b3c2d1e0fULL;
    const uint32_t blocked_lower_value = 0xdeadbeefU;
    const uint32_t final_lower_value = 0xc001d00dU;
    const uint64_t expected_final =
        ((uint64_t)upper_initial << 32) | final_lower_value;

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
    (void)amoswap_w(lower_word_vaddr, blocked_lower_value);
    TEST_ASSERT("a single mcontrol6 address trigger with both load and store access bits reports breakpoint on lower-word amoswap.w before the replacement lane can commit",
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
    TEST_ASSERT("the combined load-store lower-word amoswap.w breakpoint leaves the full doubleword, guard, adjacent, and marker words unchanged",
        trigger_preserved_words
    );

    goto_priv(PRIV_HS);
    TEST_SETUP_EXCEPT();
    uint32_t old_lower = amoswap_w(lower_word_vaddr, final_lower_value);
    TEST_ASSERT("after clearing the combined load-store trigger, amoswap.w returns the old lower word and replaces only that 32-bit lane",
        excpt.triggered == false &&
        old_lower == lower_initial &&
        read64(target_vaddr) == expected_final &&
        read32(lower_word_vaddr) == final_lower_value &&
        read32(upper_word_vaddr) == upper_initial
    );

    write64(marker_vaddr, marker_final);
    TEST_ASSERT("post-clear marker progress and non-target sentinels prove the combined access-bit lower-word amoswap.w breakpoint path did not poison following memory operations",
        read64(guard_vaddr) == guard_seed &&
        read64(adjacent_vaddr) == adjacent_seed &&
        read64(marker_vaddr) == marker_final
    );

    goto_priv(PRIV_M);
    TEST_ASSERT("backing memory shows only the post-clear lower-word amoswap.w lane replacement plus marker progress, with upper lane, guard, and adjacent unchanged",
        read64(guard_paddr) == guard_seed &&
        read64(target_paddr) == expected_final &&
        read32(lower_word_paddr) == final_lower_value &&
        read32(upper_word_paddr) == upper_initial &&
        read64(adjacent_paddr) == adjacent_seed &&
        read64(marker_paddr) == marker_final
    );

    TEST_END();
}

bool ai_micro_memblock_amoswap_w_upper_word_load_store_trigger_breakpoint_suppresses_replacement_lane_then_clear_succeeds_corner()
{
    TEST_START();

    const enum test_page page = VSRWX_GRWX;
    const uintptr_t base_vaddr = hs_page_base(page) + 0x800U;
    const uintptr_t base_paddr = phys_page_base(page) + 0x800U;
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
    const uint64_t guard_seed = 0x7377707577333030ULL;
    const uint32_t lower_initial = 0x76543210U;
    const uint32_t upper_initial = 0x1234abcdU;
    const uint64_t initial_value =
        ((uint64_t)upper_initial << 32) | lower_initial;
    const uint64_t adjacent_seed = 0x8877665544332211ULL;
    const uint64_t marker_seed = 0x0102030405060708ULL;
    const uint64_t marker_final = 0x8070605040302010ULL;
    const uint32_t blocked_upper_value = 0xfaceb00cU;
    const uint32_t final_upper_value = 0x5eedc0deU;
    const uint64_t expected_final =
        ((uint64_t)final_upper_value << 32) | lower_initial;

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
        AI_MEMBLOCK_LRSC_TRIGGER_LOAD | AI_MEMBLOCK_LRSC_TRIGGER_STORE);
    uint64_t saved_mstatus =
        ai_memblock_lrsc_trigger_enable_breakpoint_delivery();

    goto_priv(PRIV_HS);
    TEST_SETUP_EXCEPT();
    (void)amoswap_w(upper_word_vaddr, blocked_upper_value);
    TEST_ASSERT("a single mcontrol6 address trigger with both load and store access bits reports breakpoint on upper-word amoswap.w before the replacement lane can commit",
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
    TEST_ASSERT("the combined load-store upper-word amoswap.w breakpoint leaves the full doubleword, guard, adjacent, and marker words unchanged",
        trigger_preserved_words
    );

    goto_priv(PRIV_HS);
    TEST_SETUP_EXCEPT();
    uint32_t old_upper = amoswap_w(upper_word_vaddr, final_upper_value);
    TEST_ASSERT("after clearing the combined load-store trigger, amoswap.w returns the old upper word and replaces only that 32-bit lane",
        excpt.triggered == false &&
        old_upper == upper_initial &&
        read64(target_vaddr) == expected_final &&
        read32(lower_word_vaddr) == lower_initial &&
        read32(upper_word_vaddr) == final_upper_value
    );

    write64(marker_vaddr, marker_final);
    TEST_ASSERT("post-clear marker progress and non-target sentinels prove the combined access-bit upper-word amoswap.w breakpoint path did not poison following memory operations",
        read64(guard_vaddr) == guard_seed &&
        read64(adjacent_vaddr) == adjacent_seed &&
        read64(marker_vaddr) == marker_final
    );

    goto_priv(PRIV_M);
    TEST_ASSERT("backing memory shows only the post-clear upper-word amoswap.w lane replacement plus marker progress, with lower lane, guard, and adjacent unchanged",
        read64(guard_paddr) == guard_seed &&
        read64(target_paddr) == expected_final &&
        read32(lower_word_paddr) == lower_initial &&
        read32(upper_word_paddr) == final_upper_value &&
        read64(adjacent_paddr) == adjacent_seed &&
        read64(marker_paddr) == marker_final
    );

    TEST_END();
}

bool ai_micro_memblock_amomin_w_lower_word_load_store_trigger_breakpoint_suppresses_signed_min_lane_then_clear_succeeds_corner()
{
    TEST_START();

    const enum test_page page = VSRWX_GRWX;
    const uintptr_t base_vaddr = hs_page_base(page) + 0x840U;
    const uintptr_t base_paddr = phys_page_base(page) + 0x840U;
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
    const uint64_t guard_seed = 0x6d696e6c73303030ULL;
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
        AI_MEMBLOCK_LRSC_TRIGGER_LOAD | AI_MEMBLOCK_LRSC_TRIGGER_STORE);
    uint64_t saved_mstatus =
        ai_memblock_lrsc_trigger_enable_breakpoint_delivery();

    goto_priv(PRIV_HS);
    TEST_SETUP_EXCEPT();
    (void)amomin_w(lower_word_vaddr, blocked_operand);
    TEST_ASSERT("a single mcontrol6 address trigger with both load and store access bits reports breakpoint on lower-word amomin.w before the signed-min lane can commit",
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
    TEST_ASSERT("the combined load-store lower-word amomin.w breakpoint leaves the full doubleword, guard, adjacent, and marker words unchanged",
        trigger_preserved_words
    );

    goto_priv(PRIV_HS);
    TEST_SETUP_EXCEPT();
    uint32_t old_lower = amomin_w(lower_word_vaddr, final_operand);
    TEST_ASSERT("after clearing the combined load-store trigger, amomin.w returns the old lower word and signed-min updates only that 32-bit lane",
        excpt.triggered == false &&
        old_lower == lower_initial &&
        read64(target_vaddr) == expected_final &&
        read32(lower_word_vaddr) == expected_lower &&
        read32(upper_word_vaddr) == upper_initial
    );

    write64(marker_vaddr, marker_final);
    TEST_ASSERT("post-clear marker progress and non-target sentinels prove the combined access-bit lower-word amomin.w breakpoint path did not poison following memory operations",
        read64(guard_vaddr) == guard_seed &&
        read64(adjacent_vaddr) == adjacent_seed &&
        read64(marker_vaddr) == marker_final
    );

    goto_priv(PRIV_M);
    TEST_ASSERT("backing memory shows only the post-clear lower-word amomin.w signed-min lane update plus marker progress, with upper lane, guard, and adjacent unchanged",
        read64(guard_paddr) == guard_seed &&
        read64(target_paddr) == expected_final &&
        read32(lower_word_paddr) == expected_lower &&
        read32(upper_word_paddr) == upper_initial &&
        read64(adjacent_paddr) == adjacent_seed &&
        read64(marker_paddr) == marker_final
    );

    TEST_END();
}

bool ai_micro_memblock_amomin_w_upper_word_load_store_trigger_breakpoint_suppresses_signed_min_lane_then_clear_succeeds_corner()
{
    TEST_START();

    const enum test_page page = VSRWX_GRWX;
    const uintptr_t base_vaddr = hs_page_base(page) + 0x880U;
    const uintptr_t base_paddr = phys_page_base(page) + 0x880U;
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
    const uint64_t guard_seed = 0x6d696e7577333030ULL;
    const uint32_t lower_initial = 0x2468ace0U;
    const uint32_t upper_initial = 0x80000010U;
    const uint64_t initial_value =
        ((uint64_t)upper_initial << 32) | lower_initial;
    const uint64_t adjacent_seed = 0x1029384756abcdefULL;
    const uint64_t marker_seed = 0x0123456789abcdefULL;
    const uint64_t marker_final = 0xfedcba9876543210ULL;
    const uint32_t blocked_operand = 0x80000004U;
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
        AI_MEMBLOCK_LRSC_TRIGGER_LOAD | AI_MEMBLOCK_LRSC_TRIGGER_STORE);
    uint64_t saved_mstatus =
        ai_memblock_lrsc_trigger_enable_breakpoint_delivery();

    goto_priv(PRIV_HS);
    TEST_SETUP_EXCEPT();
    (void)amomin_w(upper_word_vaddr, blocked_operand);
    TEST_ASSERT("a single mcontrol6 address trigger with both load and store access bits reports breakpoint on upper-word amomin.w before the signed-min lane can commit",
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
    TEST_ASSERT("the combined load-store upper-word amomin.w breakpoint leaves the full doubleword, guard, adjacent, and marker words unchanged",
        trigger_preserved_words
    );

    goto_priv(PRIV_HS);
    TEST_SETUP_EXCEPT();
    uint32_t old_upper = amomin_w(upper_word_vaddr, final_operand);
    TEST_ASSERT("after clearing the combined load-store trigger, amomin.w returns the old upper word and signed-min updates only that 32-bit lane",
        excpt.triggered == false &&
        old_upper == upper_initial &&
        read64(target_vaddr) == expected_final &&
        read32(lower_word_vaddr) == lower_initial &&
        read32(upper_word_vaddr) == expected_upper
    );

    write64(marker_vaddr, marker_final);
    TEST_ASSERT("post-clear marker progress and non-target sentinels prove the combined access-bit upper-word amomin.w breakpoint path did not poison following memory operations",
        read64(guard_vaddr) == guard_seed &&
        read64(adjacent_vaddr) == adjacent_seed &&
        read64(marker_vaddr) == marker_final
    );

    goto_priv(PRIV_M);
    TEST_ASSERT("backing memory shows only the post-clear upper-word amomin.w signed-min lane update plus marker progress, with lower lane, guard, and adjacent unchanged",
        read64(guard_paddr) == guard_seed &&
        read64(target_paddr) == expected_final &&
        read32(lower_word_paddr) == lower_initial &&
        read32(upper_word_paddr) == expected_upper &&
        read64(adjacent_paddr) == adjacent_seed &&
        read64(marker_paddr) == marker_final
    );

    TEST_END();
}
