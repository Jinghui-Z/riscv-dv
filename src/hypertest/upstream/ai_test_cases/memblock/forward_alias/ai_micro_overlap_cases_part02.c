#include "ai_micro_overlap_common.h"

bool ai_micro_hs_stld_violation_store_killed_mid_select_does_not_flush_younger_load_corner()
{
    TEST_START();

    const enum test_page base_page = VSRWX_GRWX;
    const enum test_page fault_store_page = SWITCH1;
    const enum test_page load0_page = SWITCH2;
    const enum test_page load1_page = SCRATCHPAD;
    const uintptr_t first_base_vaddr = hs_page_base(base_page) + 0xc00U;
    const uintptr_t first_fault_store_vaddr = hs_page_base(fault_store_page) + 0xc00U;
    const uintptr_t first_load0_vaddr = hs_page_base(load0_page) + 0xc00U;
    const uintptr_t first_load1_vaddr = hs_page_base(load1_page) + 0xc00U;
    const uintptr_t first_adjacent_vaddr = first_base_vaddr + sizeof(uint64_t);
    const uintptr_t second_base_vaddr = hs_page_base(base_page) + 0xc80U;
    const uintptr_t second_load0_vaddr = hs_page_base(load0_page) + 0xc80U;
    const uintptr_t second_load1_vaddr = hs_page_base(load1_page) + 0xc80U;
    const uintptr_t second_adjacent_vaddr = second_base_vaddr + sizeof(uint64_t);
    const uintptr_t marker_vaddr = hs_page_base(base_page) + 0xce0U;
    const uintptr_t first_paddr = phys_page_base(base_page) + 0xc00U;
    const uintptr_t first_adjacent_paddr = first_paddr + sizeof(uint64_t);
    const uintptr_t second_paddr = phys_page_base(base_page) + 0xc80U;
    const uintptr_t second_adjacent_paddr = second_paddr + sizeof(uint64_t);
    const uintptr_t marker_paddr = phys_page_base(base_page) + 0xce0U;
    const uint64_t first_initial = 0x1111222233334444ULL;
    const uint64_t second_initial = 0x5566778899aabbccULL;
    const uint64_t killed_store_value = 0xddeeff0011223344ULL;
    const uint64_t second_store_value = 0xa1b2c3d4e5f60718ULL;
    const uint64_t first_adjacent = 0x8899aabbccddeeffULL;
    const uint64_t second_adjacent = 0x13579bdf2468ace0ULL;
    uint64_t first_load0 = 0ULL;
    uint64_t first_load1 = 0ULL;
    uint64_t second_load0 = 0ULL;
    uint64_t second_load1 = 0ULL;

    goto_priv(PRIV_M);
    write64(first_paddr, first_initial);
    write64(first_adjacent_paddr, first_adjacent);
    write64(second_paddr, second_initial);
    write64(second_adjacent_paddr, second_adjacent);
    write64(marker_paddr, 0ULL);

    goto_priv(PRIV_HS);
    hspt_init();

    goto_priv(PRIV_M);
    ai_micro_overlap_map_alias_to_base(fault_store_page, base_page);
    ai_micro_overlap_map_alias_to_base(load0_page, base_page);
    ai_micro_overlap_map_alias_to_base(load1_page, base_page);
    ai_micro_overlap_make_leaf_access_fault(fault_store_page);
    sfence_vma();

    goto_priv(PRIV_HS);
    ai_micro_overlap_enable_hs_cbo_inval();
    TEST_SETUP_EXCEPT();
    uint64_t warmed_first = ld(first_base_vaddr);
    AI_ASSERT_NO_EXCEPT_AND("the first RAW target line is readable before the store-killed-mid-select stale-rollback probe",
        warmed_first == first_initial
    );

    TEST_SETUP_EXCEPT();
    uint64_t warmed_second = ld(second_base_vaddr);
    AI_ASSERT_NO_EXCEPT_AND("the second RAW target line is also readable before the store-killed-mid-select stale-rollback probe",
        warmed_second == second_initial
    );

    TEST_SETUP_EXCEPT();
    ai_micro_overlap_delayed_faulting_store_then_two_loads_capture(
        first_fault_store_vaddr, first_load0_vaddr, first_load1_vaddr,
        marker_vaddr, killed_store_value, &first_load0, &first_load1);
    TEST_ASSERT("the first delayed store episode is killed by a leaf access fault, yet both younger loads on that line still retire with the original payload instead of taking a stale store-load rollback",
        excpt.triggered == true &&
        excpt.cause == CAUSE_SAF &&
        excpt.tval == first_fault_store_vaddr &&
        first_load0 == first_initial &&
        first_load1 == first_initial
    );

    TEST_SETUP_EXCEPT();
    cbo_inval(second_base_vaddr);
    AI_ASSERT_NO_EXCEPT_AND("an HS cbo.inval on the second target line retires without exception immediately after the killed-store episode to create the next replay-pressure window",
        true
    );

    TEST_SETUP_EXCEPT();
    fence_iorw_iorw();
    AI_ASSERT_NO_EXCEPT_AND("the ordering fence after invalidating the second target line retires without exception before the next delayed-store replay episode",
        true
    );

    TEST_SETUP_EXCEPT();
    ai_micro_overlap_delayed_store_then_two_loads_capture(
        second_base_vaddr, second_load0_vaddr, second_load1_vaddr,
        second_store_value, &second_load0, &second_load1);
    TEST_ASSERT("after the first store is killed mid-select, the very next delayed-store replay episode on another line still completes and both younger loads retire with the new store payload, so no stale rollback from the killed store survives into the next window",
        excpt.triggered == false &&
        second_load0 == second_store_value &&
        second_load1 == second_store_value
    );

    TEST_SETUP_EXCEPT();
    uint64_t observed_first = ld(first_load0_vaddr);
    AI_ASSERT_NO_EXCEPT_AND("reloading the first line through a younger alias after the follow-up replay episode still returns the original payload because the killed store never committed",
        observed_first == first_initial
    );

    TEST_SETUP_EXCEPT();
    uint64_t observed_second = ld(second_base_vaddr);
    AI_ASSERT_NO_EXCEPT_AND("reloading the second line through the base HS alias after the follow-up replay episode returns the second delayed store payload",
        observed_second == second_store_value
    );

    TEST_SETUP_EXCEPT();
    uint64_t observed_first_adjacent = ld(first_adjacent_vaddr);
    AI_ASSERT_NO_EXCEPT_AND("the adjacent word next to the killed-store line remains unchanged across both episodes",
        observed_first_adjacent == first_adjacent
    );

    TEST_SETUP_EXCEPT();
    uint64_t observed_second_adjacent = ld(second_adjacent_vaddr);
    AI_ASSERT_NO_EXCEPT_AND("the adjacent word next to the follow-up replay line also remains unchanged across both episodes",
        observed_second_adjacent == second_adjacent
    );

    goto_priv(PRIV_M);
    TEST_ASSERT("the store-killed-mid-select stale-rollback corner leaves the killed-store line untouched, the next replay line updated only by its own delayed store, both adjacent words preserved, and the marker confirms the younger loads outlived the killed store",
        read64(first_paddr) == first_initial &&
        read64(first_adjacent_paddr) == first_adjacent &&
        read64(second_paddr) == second_store_value &&
        read64(second_adjacent_paddr) == second_adjacent &&
        read64(marker_paddr) == 1ULL
    );
    ai_micro_overlap_disable_hs_cbo_inval();

    TEST_END();
}

bool ai_micro_hs_loadqueue_raw_same_rob_two_loads_oldest_uop_selected_corner()
{
    TEST_START();

    const enum test_page base_page = VSRWX_GRWX;
    const enum test_page store_page = SWITCH1;
    const enum test_page older_load_page = SWITCH2;
    const enum test_page younger_load_page = SCRATCHPAD;
    const uintptr_t base_vaddr = hs_page_base(base_page) + 0xd00U;
    const uintptr_t store_vaddr = hs_page_base(store_page) + 0xd00U;
    const uintptr_t older_load_vaddr = hs_page_base(older_load_page) + 0xd00U;
    const uintptr_t younger_load_vaddr = hs_page_base(younger_load_page) + 0xd00U;
    const uintptr_t adjacent_base_vaddr = base_vaddr + sizeof(uint64_t);
    const uintptr_t older_marker_vaddr = hs_page_base(base_page) + 0xd80U;
    const uintptr_t younger_marker_vaddr = hs_page_base(base_page) + 0xdc0U;
    const uintptr_t paddr = phys_page_base(base_page) + 0xd00U;
    const uintptr_t adjacent_paddr = paddr + sizeof(uint64_t);
    const uintptr_t older_marker_paddr = phys_page_base(base_page) + 0xd80U;
    const uintptr_t younger_marker_paddr = phys_page_base(base_page) + 0xdc0U;
    const uint64_t initial_value = 0x0123456789abcdefULL;
    const uint64_t delayed_store_value = 0xa1b2c3d4e5f60718ULL;
    const uint64_t adjacent_value = 0x55aa33cc77ee11ffULL;
    const uint64_t older_marker_seed = 0x1111222233334444ULL;
    const uint64_t younger_marker_seed = 0x88889999aaaabbbbULL;

    goto_priv(PRIV_M);
    write64(paddr, initial_value);
    write64(adjacent_paddr, adjacent_value);
    write64(older_marker_paddr, older_marker_seed);
    write64(younger_marker_paddr, younger_marker_seed);

    goto_priv(PRIV_HS);
    hspt_init();

    goto_priv(PRIV_M);
    ai_micro_overlap_map_alias_to_base(store_page, base_page);
    ai_micro_overlap_map_alias_to_base(older_load_page, base_page);
    ai_micro_overlap_map_alias_to_base(younger_load_page, base_page);
    sfence_vma();

    goto_priv(PRIV_HS);
    TEST_SETUP_EXCEPT();
    uint64_t warmed_base = ld(base_vaddr);
    AI_ASSERT_NO_EXCEPT_AND("the RAW oldest-uop approximation first warms the target line through the base HS alias",
        warmed_base == initial_value
    );

    TEST_SETUP_EXCEPT();
    uint64_t warmed_adjacent = ld(adjacent_base_vaddr);
    AI_ASSERT_NO_EXCEPT_AND("the adjacent HS word is also readable before the dual-load RAW oldest-uop approximation",
        warmed_adjacent == adjacent_value
    );

    TEST_SETUP_EXCEPT();
    ai_micro_overlap_delayed_store_then_marked_two_loads(store_vaddr,
        older_load_vaddr, younger_load_vaddr, older_marker_vaddr,
        younger_marker_vaddr, delayed_store_value);
    AI_ASSERT_NO_EXCEPT_AND("a delayed older store followed by two back-to-back same-PA younger loads and two value-dependent marker stores retires without architectural exception in the RAW oldest-uop approximation",
        true
    );

    TEST_SETUP_EXCEPT();
    fence_iorw_iorw();
    AI_ASSERT_NO_EXCEPT_AND("the ordering fence after the dual-load RAW oldest-uop approximation retires without exception before marker checks",
        true
    );

    TEST_SETUP_EXCEPT();
    uint64_t observed_older_marker = ld(older_marker_vaddr);
    AI_ASSERT_NO_EXCEPT_AND("the marker store placed between the two younger loads observes the delayed store payload, approximating that rollback would target the architecturally older load rather than letting the between-load side effect escape with stale data",
        observed_older_marker == delayed_store_value
    );

    TEST_SETUP_EXCEPT();
    uint64_t observed_younger_marker = ld(younger_marker_vaddr);
    AI_ASSERT_NO_EXCEPT_AND("the marker store after the second younger load also observes the delayed store payload in the RAW oldest-uop approximation",
        observed_younger_marker == delayed_store_value
    );

    TEST_SETUP_EXCEPT();
    uint64_t observed_older_alias = ld(older_load_vaddr);
    AI_ASSERT_NO_EXCEPT_AND("reloading through the architecturally older younger-load alias after the RAW oldest-uop approximation returns the delayed store payload",
        observed_older_alias == delayed_store_value
    );

    TEST_SETUP_EXCEPT();
    uint64_t observed_younger_alias = ld(younger_load_vaddr);
    AI_ASSERT_NO_EXCEPT_AND("reloading through the architecturally later younger-load alias after the RAW oldest-uop approximation also returns the delayed store payload",
        observed_younger_alias == delayed_store_value
    );

    TEST_SETUP_EXCEPT();
    uint64_t observed_adjacent = ld(adjacent_base_vaddr);
    AI_ASSERT_NO_EXCEPT_AND("the adjacent word next to the RAW oldest-uop target remains unchanged across the approximation probe",
        observed_adjacent == adjacent_value
    );

    goto_priv(PRIV_M);
    TEST_ASSERT("the RAW oldest-uop approximation leaves backing memory with only the delayed store payload on the target, preserves the adjacent word, and records the delayed payload in both markers",
        read64(paddr) == delayed_store_value &&
        read64(adjacent_paddr) == adjacent_value &&
        read64(older_marker_paddr) == delayed_store_value &&
        read64(younger_marker_paddr) == delayed_store_value
    );

    TEST_END();
}

bool ai_micro_hs_stld_violation_same_rob_diff_uopidx_redirect_target_correct_corner()
{
    TEST_START();

    const enum test_page base_page = VSRWX_GRWX;
    const enum test_page store_page = SWITCH1;
    const enum test_page load_a_page = SWITCH2;
    const enum test_page load_b_page = SCRATCHPAD;
    const uintptr_t base_vaddr = hs_page_base(base_page) + 0xe00U;
    const uintptr_t store_vaddr = hs_page_base(store_page) + 0xe00U;
    const uintptr_t load_a_vaddr = hs_page_base(load_a_page) + 0xe00U;
    const uintptr_t load_b_vaddr = hs_page_base(load_b_page) + 0xe00U;
    const uintptr_t adjacent_base_vaddr = base_vaddr + sizeof(uint64_t);
    const uintptr_t older_marker_vaddr = hs_page_base(base_page) + 0xe80U;
    const uintptr_t younger_marker_vaddr = hs_page_base(base_page) + 0xec0U;
    const uintptr_t paddr = phys_page_base(base_page) + 0xe00U;
    const uintptr_t adjacent_paddr = paddr + sizeof(uint64_t);
    const uintptr_t older_marker_paddr = phys_page_base(base_page) + 0xe80U;
    const uintptr_t younger_marker_paddr = phys_page_base(base_page) + 0xec0U;
    const uint64_t first_initial = 0x1021324354657687ULL;
    const uint64_t first_store_value = 0x8899aabbccddeeffULL;
    const uint64_t second_store_value = 0xfedcba9876543210ULL;
    const uint64_t adjacent_value = 0x13579bdf2468ace0ULL;
    const uint64_t first_older_seed = 0x0101010101010101ULL;
    const uint64_t first_younger_seed = 0x0202020202020202ULL;
    const uint64_t second_older_seed = 0x0303030303030303ULL;
    const uint64_t second_younger_seed = 0x0404040404040404ULL;

    goto_priv(PRIV_M);
    write64(paddr, first_initial);
    write64(adjacent_paddr, adjacent_value);
    write64(older_marker_paddr, first_older_seed);
    write64(younger_marker_paddr, first_younger_seed);

    goto_priv(PRIV_HS);
    hspt_init();

    goto_priv(PRIV_M);
    ai_micro_overlap_map_alias_to_base(store_page, base_page);
    ai_micro_overlap_map_alias_to_base(load_a_page, base_page);
    ai_micro_overlap_map_alias_to_base(load_b_page, base_page);
    sfence_vma();

    goto_priv(PRIV_HS);
    TEST_SETUP_EXCEPT();
    uint64_t warmed_base = ld(base_vaddr);
    AI_ASSERT_NO_EXCEPT_AND("the repeated RAW redirect-target approximation first warms the target line through the base HS alias",
        warmed_base == first_initial
    );

    TEST_SETUP_EXCEPT();
    ai_micro_overlap_delayed_store_then_marked_two_loads(store_vaddr,
        load_a_vaddr, load_b_vaddr, older_marker_vaddr, younger_marker_vaddr,
        first_store_value);
    AI_ASSERT_NO_EXCEPT_AND("the first delayed-store RAW redirect-target approximation round retires without architectural exception",
        true
    );

    TEST_SETUP_EXCEPT();
    fence_iorw_iorw();
    AI_ASSERT_NO_EXCEPT_AND("the ordering fence after the first delayed-store RAW redirect-target approximation round retires without exception",
        true
    );

    TEST_SETUP_EXCEPT();
    uint64_t first_older_marker = ld(older_marker_vaddr);
    AI_ASSERT_NO_EXCEPT_AND("in the first RAW redirect-target approximation round, the between-load marker records the delayed store payload instead of a stale pre-store value",
        first_older_marker == first_store_value
    );

    TEST_SETUP_EXCEPT();
    uint64_t first_younger_marker = ld(younger_marker_vaddr);
    AI_ASSERT_NO_EXCEPT_AND("in the first RAW redirect-target approximation round, the later marker also records the delayed store payload",
        first_younger_marker == first_store_value
    );

    goto_priv(PRIV_M);
    write64(older_marker_paddr, second_older_seed);
    write64(younger_marker_paddr, second_younger_seed);

    goto_priv(PRIV_HS);
    TEST_SETUP_EXCEPT();
    ai_micro_overlap_delayed_store_then_marked_two_loads(store_vaddr,
        load_b_vaddr, load_a_vaddr, older_marker_vaddr, younger_marker_vaddr,
        second_store_value);
    AI_ASSERT_NO_EXCEPT_AND("the repeated RAW redirect-target approximation also retires normally when the two same-PA sibling loads swap virtual-alias order",
        true
    );

    TEST_SETUP_EXCEPT();
    fence_iorw_iorw();
    AI_ASSERT_NO_EXCEPT_AND("the ordering fence after the swapped-alias RAW redirect-target approximation round retires without exception",
        true
    );

    TEST_SETUP_EXCEPT();
    uint64_t second_older_marker = ld(older_marker_vaddr);
    AI_ASSERT_NO_EXCEPT_AND("after swapping which sibling alias is the earlier load in the repeated RAW redirect-target approximation, the between-load marker still records only the latest delayed store payload",
        second_older_marker == second_store_value
    );

    TEST_SETUP_EXCEPT();
    uint64_t second_younger_marker = ld(younger_marker_vaddr);
    AI_ASSERT_NO_EXCEPT_AND("after the swapped-alias repeated RAW redirect-target approximation, the later marker also records only the latest delayed store payload",
        second_younger_marker == second_store_value
    );

    TEST_SETUP_EXCEPT();
    uint64_t observed_base = ld(base_vaddr);
    AI_ASSERT_NO_EXCEPT_AND("reloading through the base HS alias after the repeated RAW redirect-target approximation returns only the second delayed store payload",
        observed_base == second_store_value
    );

    TEST_SETUP_EXCEPT();
    uint64_t observed_alias_a = ld(load_a_vaddr);
    AI_ASSERT_NO_EXCEPT_AND("reloading through the first same-PA sibling alias after the repeated RAW redirect-target approximation returns only the second delayed store payload",
        observed_alias_a == second_store_value
    );

    TEST_SETUP_EXCEPT();
    uint64_t observed_alias_b = ld(load_b_vaddr);
    AI_ASSERT_NO_EXCEPT_AND("reloading through the second same-PA sibling alias after the repeated RAW redirect-target approximation also returns only the second delayed store payload",
        observed_alias_b == second_store_value
    );

    TEST_SETUP_EXCEPT();
    uint64_t observed_adjacent = ld(adjacent_base_vaddr);
    AI_ASSERT_NO_EXCEPT_AND("the adjacent word next to the repeated RAW redirect-target target remains unchanged across both rounds",
        observed_adjacent == adjacent_value
    );

    goto_priv(PRIV_M);
    TEST_ASSERT("the repeated RAW redirect-target approximation leaves backing memory with only the second delayed store payload on the target, preserves the adjacent word, and refreshes both markers to the latest payload even after the sibling aliases swap order",
        read64(paddr) == second_store_value &&
        read64(adjacent_paddr) == adjacent_value &&
        read64(older_marker_paddr) == second_store_value &&
        read64(younger_marker_paddr) == second_store_value
    );

    TEST_END();
}

bool ai_micro_hs_load_vpmatchfail_and_ldld_same_cycle_rollback_priority_correct_corner()
{
    TEST_START();

    const enum test_page base_page = VSRWX_GRWX;
    const enum test_page store_page = SWITCH1;
    const enum test_page older_load_page = SWITCH2;
    const enum test_page younger_load_page = SCRATCHPAD;
    const uintptr_t base_vaddr = hs_page_base(base_page) + 0xe40U;
    const uintptr_t store_vaddr = hs_page_base(store_page) + 0xe40U;
    const uintptr_t older_load_vaddr = hs_page_base(older_load_page) + 0xe40U;
    const uintptr_t younger_load_vaddr = hs_page_base(younger_load_page) + 0xe40U;
    const uintptr_t adjacent_base_vaddr = base_vaddr + sizeof(uint64_t);
    const uintptr_t older_marker_vaddr = hs_page_base(base_page) + 0xf00U;
    const uintptr_t younger_marker_vaddr = hs_page_base(base_page) + 0xf40U;
    const uintptr_t paddr = phys_page_base(base_page) + 0xe40U;
    const uintptr_t adjacent_paddr = paddr + sizeof(uint64_t);
    const uintptr_t older_marker_paddr = phys_page_base(base_page) + 0xf00U;
    const uintptr_t younger_marker_paddr = phys_page_base(base_page) + 0xf40U;
    const uint64_t initial_value = 0x1021324354657687ULL;
    const uint64_t delayed_store_value = 0x8899aabbccddeeffULL;
    const uint64_t adjacent_value = 0x13579bdf2468ace0ULL;
    const uint64_t older_marker_seed = 0x0101010101010101ULL;
    const uint64_t younger_marker_seed = 0x0202020202020202ULL;

    goto_priv(PRIV_M);
    write64(paddr, initial_value);
    write64(adjacent_paddr, adjacent_value);
    write64(older_marker_paddr, older_marker_seed);
    write64(younger_marker_paddr, younger_marker_seed);

    goto_priv(PRIV_HS);
    hspt_init();

    goto_priv(PRIV_M);
    ai_micro_overlap_map_alias_to_base(store_page, base_page);
    ai_micro_overlap_map_alias_to_base(older_load_page, base_page);
    ai_micro_overlap_map_alias_to_base(younger_load_page, base_page);
    sfence_vma();

    goto_priv(PRIV_HS);
    TEST_SETUP_EXCEPT();
    uint64_t warmed_base = ld(base_vaddr);
    AI_ASSERT_NO_EXCEPT_AND("the combined alias-load rollback-priority probe first warms the target line through the base HS alias",
        warmed_base == initial_value
    );

    TEST_SETUP_EXCEPT();
    uint64_t warmed_adjacent = ld(adjacent_base_vaddr);
    AI_ASSERT_NO_EXCEPT_AND("the adjacent HS word is also readable before the combined alias-load rollback-priority probe",
        warmed_adjacent == adjacent_value
    );

    TEST_SETUP_EXCEPT();
    ai_micro_overlap_delayed_store_then_marked_two_loads(store_vaddr,
        older_load_vaddr, younger_load_vaddr, older_marker_vaddr,
        younger_marker_vaddr, delayed_store_value);
    AI_ASSERT_NO_EXCEPT_AND("a delayed same-PA alias store followed by two same-PA sibling loads retires without architectural exception in the combined vp_match_fail plus ld-ld rollback-priority approximation",
        true
    );

    TEST_SETUP_EXCEPT();
    fence_iorw_iorw();
    AI_ASSERT_NO_EXCEPT_AND("the ordering fence after the combined alias-load rollback-priority approximation retires without exception before marker checks",
        true
    );

    TEST_SETUP_EXCEPT();
    uint64_t observed_older_marker = ld(older_marker_vaddr);
    AI_ASSERT_NO_EXCEPT_AND("the between-load marker records only the delayed alias-store payload, approximating that a combined vp_match_fail plus ld-ld rollback still preserves the architecturally earlier boundary instead of losing or over-flushing that side effect",
        observed_older_marker == delayed_store_value
    );

    TEST_SETUP_EXCEPT();
    uint64_t observed_younger_marker = ld(younger_marker_vaddr);
    AI_ASSERT_NO_EXCEPT_AND("the later marker also records only the delayed alias-store payload after the combined alias-load rollback-priority approximation",
        observed_younger_marker == delayed_store_value
    );

    TEST_SETUP_EXCEPT();
    uint64_t observed_older_alias = ld(older_load_vaddr);
    AI_ASSERT_NO_EXCEPT_AND("reloading through the architecturally older same-PA alias after the combined alias-load rollback-priority approximation returns only the delayed store payload",
        observed_older_alias == delayed_store_value
    );

    TEST_SETUP_EXCEPT();
    uint64_t observed_younger_alias = ld(younger_load_vaddr);
    AI_ASSERT_NO_EXCEPT_AND("reloading through the architecturally younger same-PA alias after the combined alias-load rollback-priority approximation also returns only the delayed store payload",
        observed_younger_alias == delayed_store_value
    );

    TEST_SETUP_EXCEPT();
    uint64_t observed_base = ld(base_vaddr);
    AI_ASSERT_NO_EXCEPT_AND("the base HS alias also reads back only the delayed store payload after the combined alias-load rollback-priority approximation",
        observed_base == delayed_store_value
    );

    TEST_SETUP_EXCEPT();
    uint64_t observed_adjacent = ld(adjacent_base_vaddr);
    AI_ASSERT_NO_EXCEPT_AND("the adjacent word next to the combined alias-load rollback-priority target remains unchanged",
        observed_adjacent == adjacent_value
    );

    goto_priv(PRIV_M);
    TEST_ASSERT("the combined vp_match_fail plus ld-ld rollback-priority approximation leaves backing memory with only the delayed alias-store payload on the target, preserves the adjacent word, and records that payload in both marker stores exactly once",
        read64(paddr) == delayed_store_value &&
        read64(adjacent_paddr) == adjacent_value &&
        read64(older_marker_paddr) == delayed_store_value &&
        read64(younger_marker_paddr) == delayed_store_value
    );

    TEST_END();
}

bool ai_micro_hs_load_alias_and_ldld_combined_event_no_wrong_flush_scope_corner()
{
    TEST_START();

    const enum test_page base_page = VSRWX_GRWX;
    const enum test_page store_page = SWITCH1;
    const enum test_page load_a_page = SWITCH2;
    const enum test_page load_b_page = SCRATCHPAD;
    const uintptr_t base_vaddr = hs_page_base(base_page) + 0xf80U;
    const uintptr_t store_vaddr = hs_page_base(store_page) + 0xf80U;
    const uintptr_t load_a_vaddr = hs_page_base(load_a_page) + 0xf80U;
    const uintptr_t load_b_vaddr = hs_page_base(load_b_page) + 0xf80U;
    const uintptr_t adjacent_base_vaddr = base_vaddr + sizeof(uint64_t);
    const uintptr_t older_marker_vaddr = hs_page_base(base_page) + 0x1040U;
    const uintptr_t younger_marker_vaddr = hs_page_base(base_page) + 0x1080U;
    const uintptr_t paddr = phys_page_base(base_page) + 0xf80U;
    const uintptr_t adjacent_paddr = paddr + sizeof(uint64_t);
    const uintptr_t older_marker_paddr = phys_page_base(base_page) + 0x1040U;
    const uintptr_t younger_marker_paddr = phys_page_base(base_page) + 0x1080U;
    const uint64_t first_initial = 0x5566778899aabbccULL;
    const uint64_t first_store_value = 0x0123456789abcdefULL;
    const uint64_t second_store_value = 0xfedcba9876543210ULL;
    const uint64_t adjacent_value = 0xa1b2c3d4e5f60718ULL;
    const uint64_t first_older_seed = 0x1111222233334444ULL;
    const uint64_t first_younger_seed = 0x5555666677778888ULL;
    const uint64_t second_older_seed = 0x9999aaaabbbbccccULL;
    const uint64_t second_younger_seed = 0xddddeeeeffff0000ULL;

    goto_priv(PRIV_M);
    write64(paddr, first_initial);
    write64(adjacent_paddr, adjacent_value);
    write64(older_marker_paddr, first_older_seed);
    write64(younger_marker_paddr, first_younger_seed);

    goto_priv(PRIV_HS);
    hspt_init();

    goto_priv(PRIV_M);
    ai_micro_overlap_map_alias_to_base(store_page, base_page);
    ai_micro_overlap_map_alias_to_base(load_a_page, base_page);
    ai_micro_overlap_map_alias_to_base(load_b_page, base_page);
    sfence_vma();

    goto_priv(PRIV_HS);
    TEST_SETUP_EXCEPT();
    uint64_t warmed_base = ld(base_vaddr);
    AI_ASSERT_NO_EXCEPT_AND("the repeated combined alias-load flush-scope probe first warms the target line through the base HS alias",
        warmed_base == first_initial
    );

    TEST_SETUP_EXCEPT();
    uint64_t warmed_adjacent = ld(adjacent_base_vaddr);
    AI_ASSERT_NO_EXCEPT_AND("the adjacent HS word is also readable before the repeated combined alias-load flush-scope probe",
        warmed_adjacent == adjacent_value
    );

    TEST_SETUP_EXCEPT();
    ai_micro_overlap_delayed_store_then_marked_two_loads(store_vaddr,
        load_a_vaddr, load_b_vaddr, older_marker_vaddr, younger_marker_vaddr,
        first_store_value);
    AI_ASSERT_NO_EXCEPT_AND("the first combined alias-load flush-scope approximation round retires without architectural exception",
        true
    );

    TEST_SETUP_EXCEPT();
    fence_iorw_iorw();
    AI_ASSERT_NO_EXCEPT_AND("the ordering fence after the first combined alias-load flush-scope approximation round retires without exception",
        true
    );

    TEST_SETUP_EXCEPT();
    uint64_t first_older_marker = ld(older_marker_vaddr);
    AI_ASSERT_NO_EXCEPT_AND("in the first combined alias-load flush-scope approximation round, the between-load marker records only the first delayed alias-store payload",
        first_older_marker == first_store_value
    );

    TEST_SETUP_EXCEPT();
    uint64_t first_younger_marker = ld(younger_marker_vaddr);
    AI_ASSERT_NO_EXCEPT_AND("in the first combined alias-load flush-scope approximation round, the later marker also records only the first delayed alias-store payload",
        first_younger_marker == first_store_value
    );

    goto_priv(PRIV_M);
    write64(older_marker_paddr, second_older_seed);
    write64(younger_marker_paddr, second_younger_seed);

    goto_priv(PRIV_HS);
    TEST_SETUP_EXCEPT();
    ai_micro_overlap_delayed_store_then_marked_two_loads(store_vaddr,
        load_b_vaddr, load_a_vaddr, older_marker_vaddr, younger_marker_vaddr,
        second_store_value);
    AI_ASSERT_NO_EXCEPT_AND("the repeated combined alias-load flush-scope approximation also retires normally when the two same-PA sibling loads swap virtual-alias order",
        true
    );

    TEST_SETUP_EXCEPT();
    fence_iorw_iorw();
    AI_ASSERT_NO_EXCEPT_AND("the ordering fence after the swapped-alias combined alias-load flush-scope approximation round retires without exception",
        true
    );

    TEST_SETUP_EXCEPT();
    uint64_t second_older_marker = ld(older_marker_vaddr);
    AI_ASSERT_NO_EXCEPT_AND("after swapping which sibling alias is the earlier load in the repeated combined alias-load flush-scope approximation, the between-load marker still records only the latest delayed alias-store payload",
        second_older_marker == second_store_value
    );

    TEST_SETUP_EXCEPT();
    uint64_t second_younger_marker = ld(younger_marker_vaddr);
    AI_ASSERT_NO_EXCEPT_AND("after the swapped-alias repeated combined alias-load flush-scope approximation, the later marker also records only the latest delayed alias-store payload",
        second_younger_marker == second_store_value
    );

    TEST_SETUP_EXCEPT();
    uint64_t observed_base = ld(base_vaddr);
    AI_ASSERT_NO_EXCEPT_AND("reloading through the base HS alias after the repeated combined alias-load flush-scope approximation returns only the second delayed alias-store payload",
        observed_base == second_store_value
    );

    TEST_SETUP_EXCEPT();
    uint64_t observed_alias_a = ld(load_a_vaddr);
    AI_ASSERT_NO_EXCEPT_AND("reloading through the first same-PA sibling alias after the repeated combined alias-load flush-scope approximation returns only the second delayed alias-store payload",
        observed_alias_a == second_store_value
    );

    TEST_SETUP_EXCEPT();
    uint64_t observed_alias_b = ld(load_b_vaddr);
    AI_ASSERT_NO_EXCEPT_AND("reloading through the second same-PA sibling alias after the repeated combined alias-load flush-scope approximation also returns only the second delayed alias-store payload",
        observed_alias_b == second_store_value
    );

    TEST_SETUP_EXCEPT();
    uint64_t observed_adjacent = ld(adjacent_base_vaddr);
    AI_ASSERT_NO_EXCEPT_AND("the adjacent word next to the repeated combined alias-load flush-scope target remains unchanged across both rounds",
        observed_adjacent == adjacent_value
    );

    goto_priv(PRIV_M);
    TEST_ASSERT("the repeated combined vp_match_fail plus ld-ld approximation leaves backing memory with only the second delayed alias-store payload on the target, preserves the adjacent word, and refreshes both markers to the latest payload after the sibling aliases swap order",
        read64(paddr) == second_store_value &&
        read64(adjacent_paddr) == adjacent_value &&
        read64(older_marker_paddr) == second_store_value &&
        read64(younger_marker_paddr) == second_store_value
    );

    TEST_END();
}

bool ai_micro_hs_raw_tracker_slot_reuse_old_revoke_must_not_kill_new_load_corner()
{
    TEST_START();

    const enum test_page base_page = VSRWX_GRWX;
    const enum test_page store_page = SWITCH1;
    const enum test_page load0_page = SWITCH2;
    const enum test_page load1_page = SCRATCHPAD;
    const uintptr_t base_vaddr = hs_page_base(base_page) + 0x7c0U;
    const uintptr_t store_vaddr = hs_page_base(store_page) + 0x7c0U;
    const uintptr_t load0_vaddr = hs_page_base(load0_page) + 0x7c0U;
    const uintptr_t load1_vaddr = hs_page_base(load1_page) + 0x7c0U;
    const uintptr_t adjacent_base_vaddr = base_vaddr + sizeof(uint64_t);
    const uintptr_t recovery_vaddr = hs_page_base(base_page) + 0x840U;
    const uintptr_t paddr = phys_page_base(base_page) + 0x7c0U;
    const uintptr_t adjacent_paddr = paddr + sizeof(uint64_t);
    const uintptr_t recovery_paddr = phys_page_base(base_page) + 0x840U;
    const uint64_t initial_value = 0x1111222233334444ULL;
    const uint64_t first_store_value = 0x5566778899aabbccULL;
    const uint64_t second_store_value = 0xddeeff0011223344ULL;
    const uint64_t adjacent = 0x8899aabbccddeeffULL;
    const uint64_t recovery = 0x13579bdf2468ace0ULL;
    uint64_t first_load0 = 0ULL;
    uint64_t first_load1 = 0ULL;
    uint64_t second_load0 = 0ULL;
    uint64_t second_load1 = 0ULL;

    goto_priv(PRIV_M);
    write64(paddr, initial_value);
    write64(adjacent_paddr, adjacent);
    write64(recovery_paddr, recovery);

    goto_priv(PRIV_HS);
    hspt_init();

    goto_priv(PRIV_M);
    ai_micro_overlap_map_alias_to_base(store_page, base_page);
    ai_micro_overlap_map_alias_to_base(load0_page, base_page);
    ai_micro_overlap_map_alias_to_base(load1_page, base_page);
    sfence_vma();

    goto_priv(PRIV_HS);
    ai_micro_overlap_enable_hs_cbo_inval();
    TEST_SETUP_EXCEPT();
    cbo_inval(base_vaddr);
    AI_ASSERT_NO_EXCEPT_AND("an HS cbo.inval on the target line retires without exception before the first RAW tracker slot-reuse probe",
        true
    );

    TEST_SETUP_EXCEPT();
    fence_iorw_iorw();
    AI_ASSERT_NO_EXCEPT_AND("the ordering fence after the first target-line invalidation retires without exception before the RAW tracker slot-reuse probe",
        true
    );

    TEST_SETUP_EXCEPT();
    ai_micro_overlap_delayed_store_then_two_loads_capture(store_vaddr,
        load0_vaddr, load1_vaddr, first_store_value,
        &first_load0, &first_load1);
    TEST_ASSERT("in the first delayed-store episode, both younger alias loads still retire with the first delayed store payload so the initial revoke cannot corrupt a later reused RAW slot",
        excpt.triggered == false &&
        first_load0 == first_store_value &&
        first_load1 == first_store_value
    );

    TEST_SETUP_EXCEPT();
    cbo_inval(base_vaddr);
    AI_ASSERT_NO_EXCEPT_AND("a second HS cbo.inval on the same target line retires without exception before the recycled-slot RAW probe",
        true
    );

    TEST_SETUP_EXCEPT();
    fence_iorw_iorw();
    AI_ASSERT_NO_EXCEPT_AND("the ordering fence after the second target-line invalidation retires without exception before the recycled-slot RAW probe",
        true
    );

    TEST_SETUP_EXCEPT();
    ai_micro_overlap_delayed_store_then_two_loads_capture(store_vaddr,
        load0_vaddr, load1_vaddr, second_store_value,
        &second_load0, &second_load1);
    TEST_ASSERT("after immediately reusing the same target line for another delayed-store episode, both younger alias loads still retire with the second delayed store payload so an old revoke cannot kill the new RAW-tracked load",
        excpt.triggered == false &&
        second_load0 == second_store_value &&
        second_load1 == second_store_value
    );

    TEST_SETUP_EXCEPT();
    uint64_t observed_base = ld(base_vaddr);
    AI_ASSERT_NO_EXCEPT_AND("reloading through the base HS alias after the repeated RAW tracker slot-reuse probe returns the second delayed store payload",
        observed_base == second_store_value
    );

    TEST_SETUP_EXCEPT();
    uint64_t observed_load0 = ld(load0_vaddr);
    AI_ASSERT_NO_EXCEPT_AND("reloading through the first younger-load alias after the repeated RAW tracker slot-reuse probe also returns the second delayed store payload",
        observed_load0 == second_store_value
    );

    TEST_SETUP_EXCEPT();
    uint64_t observed_adjacent = ld(adjacent_base_vaddr);
    AI_ASSERT_NO_EXCEPT_AND("the adjacent HS word remains unchanged across the repeated RAW tracker slot-reuse probe",
        observed_adjacent == adjacent
    );

    TEST_SETUP_EXCEPT();
    uint64_t observed_recovery = ld(recovery_vaddr);
    AI_ASSERT_NO_EXCEPT_AND("an unrelated HS recovery word remains unchanged across the repeated RAW tracker slot-reuse probe",
        observed_recovery == recovery
    );

    goto_priv(PRIV_M);
    TEST_ASSERT("the repeated RAW tracker slot-reuse corner leaves backing memory with only the second delayed store update while preserving the adjacent and unrelated words",
        read64(paddr) == second_store_value &&
        read64(adjacent_paddr) == adjacent &&
        read64(recovery_paddr) == recovery
    );
    ai_micro_overlap_disable_hs_cbo_inval();

    TEST_END();
}

bool ai_micro_hs_loadqueue_raw_dual_store_violation_two_targets_no_cross_flush_corner()
{
    TEST_START();

    const enum test_page base0_page = VSRWX_GRWX;
    const enum test_page base1_page = VSRW_GRWX;
    const enum test_page store0_page = SWITCH1;
    const enum test_page load0_page = SWITCH2;
    const enum test_page store1_page = SCRATCHPAD;
    const enum test_page load1_page = VSURWX_GRWX;
    const uintptr_t base0_vaddr = hs_page_base(base0_page) + 0xc00U;
    const uintptr_t base1_vaddr = hs_page_base(base1_page) + 0xc40U;
    const uintptr_t store0_vaddr = hs_page_base(store0_page) + 0xc00U;
    const uintptr_t load0_vaddr = hs_page_base(load0_page) + 0xc00U;
    const uintptr_t store1_vaddr = hs_page_base(store1_page) + 0xc40U;
    const uintptr_t load1_vaddr = hs_page_base(load1_page) + 0xc40U;
    const uintptr_t adjacent0_vaddr = base0_vaddr + sizeof(uint64_t);
    const uintptr_t adjacent1_vaddr = base1_vaddr + sizeof(uint64_t);
    const uintptr_t paddr0 = phys_page_base(base0_page) + 0xc00U;
    const uintptr_t paddr1 = phys_page_base(base1_page) + 0xc40U;
    const uintptr_t adjacent0_paddr = paddr0 + sizeof(uint64_t);
    const uintptr_t adjacent1_paddr = paddr1 + sizeof(uint64_t);
    const uint64_t initial0 = 0x0123456789abcdefULL;
    const uint64_t initial1 = 0xfedcba9876543210ULL;
    const uint64_t store0_value = 0xa1b2c3d4e5f60718ULL;
    const uint64_t store1_value = 0x8877665544332211ULL;
    const uint64_t adjacent0 = 0x55aa33cc77ee11ffULL;
    const uint64_t adjacent1 = 0x0f1e2d3c4b5a6978ULL;
    uint64_t observed0 = 0ULL;
    uint64_t observed1 = 0ULL;

    goto_priv(PRIV_M);
    write64(paddr0, initial0);
    write64(paddr1, initial1);
    write64(adjacent0_paddr, adjacent0);
    write64(adjacent1_paddr, adjacent1);

    goto_priv(PRIV_HS);
    hspt_init();

    goto_priv(PRIV_M);
    ai_micro_overlap_map_alias_to_base(store0_page, base0_page);
    ai_micro_overlap_map_alias_to_base(load0_page, base0_page);
    ai_micro_overlap_map_alias_to_base(store1_page, base1_page);
    ai_micro_overlap_map_alias_to_base(load1_page, base1_page);
    sfence_vma();

    goto_priv(PRIV_HS);
    ai_micro_overlap_enable_hs_cbo_inval();

    TEST_SETUP_EXCEPT();
    uint64_t warmed0 = ld(base0_vaddr);
    AI_ASSERT_NO_EXCEPT_AND("the first RAW target line is readable before the dual-store two-target rollback probe",
        warmed0 == initial0
    );

    TEST_SETUP_EXCEPT();
    uint64_t warmed1 = ld(base1_vaddr);
    AI_ASSERT_NO_EXCEPT_AND("the second RAW target line is readable before the dual-store two-target rollback probe",
        warmed1 == initial1
    );

    TEST_SETUP_EXCEPT();
    cbo_inval(base0_vaddr);
    AI_ASSERT_NO_EXCEPT_AND("an HS cbo.inval on the first RAW target line retires before the dual-store two-target rollback probe",
        true
    );

    TEST_SETUP_EXCEPT();
    cbo_inval(base1_vaddr);
    AI_ASSERT_NO_EXCEPT_AND("an HS cbo.inval on the second RAW target line retires before the dual-store two-target rollback probe",
        true
    );

    TEST_SETUP_EXCEPT();
    fence_iorw_iorw();
    AI_ASSERT_NO_EXCEPT_AND("the ordering fence after both target-line invalidations retires before the dual-store two-target rollback probe",
        true
    );

    TEST_SETUP_EXCEPT();
    ai_micro_overlap_delayed_two_stores_then_two_loads_capture(
        store0_vaddr, store1_vaddr, load0_vaddr, load1_vaddr,
        store0_value, store1_value, &observed0, &observed1);
    TEST_ASSERT("two independent delayed-store RAW windows in one contiguous probe both replay to their own latest payloads without cross-target flush or stale old data",
        excpt.triggered == false &&
        observed0 == store0_value &&
        observed1 == store1_value
    );

    TEST_SETUP_EXCEPT();
    uint64_t reloaded0 = ld(base0_vaddr);
    AI_ASSERT_NO_EXCEPT_AND("reloading the first base alias after the dual-store RAW probe returns only the first store payload",
        reloaded0 == store0_value
    );

    TEST_SETUP_EXCEPT();
    uint64_t reloaded1 = ld(base1_vaddr);
    AI_ASSERT_NO_EXCEPT_AND("reloading the second base alias after the dual-store RAW probe returns only the second store payload",
        reloaded1 == store1_value
    );

    TEST_SETUP_EXCEPT();
    uint64_t adjacent0_seen = ld(adjacent0_vaddr);
    AI_ASSERT_NO_EXCEPT_AND("the adjacent word next to the first RAW target remains unchanged across the dual-store rollback probe",
        adjacent0_seen == adjacent0
    );

    TEST_SETUP_EXCEPT();
    uint64_t adjacent1_seen = ld(adjacent1_vaddr);
    AI_ASSERT_NO_EXCEPT_AND("the adjacent word next to the second RAW target remains unchanged across the dual-store rollback probe",
        adjacent1_seen == adjacent1
    );

    goto_priv(PRIV_M);
    TEST_ASSERT("the dual-store two-target RAW probe leaves each backing line with only its own delayed-store payload and preserves both adjacent words",
        read64(paddr0) == store0_value &&
        read64(paddr1) == store1_value &&
        read64(adjacent0_paddr) == adjacent0 &&
        read64(adjacent1_paddr) == adjacent1
    );
    ai_micro_overlap_disable_hs_cbo_inval();

    TEST_END();
}

bool ai_micro_hs_load_forward_valid_and_invalid_cause_same_epoch_no_cross_cycle_mixup_corner()
{
    TEST_START();

    const enum test_page base_page = VSRWX_GRWX;
    const enum test_page alias_page = SWITCH1;
    const enum test_page direct_page = SWITCH2;
    const uintptr_t base_vaddr = hs_page_base(base_page) + 0x880U;
    const uintptr_t alias_vaddr = hs_page_base(alias_page) + 0x880U;
    const uintptr_t direct_vaddr = hs_page_base(direct_page) + 0x180U;
    const uintptr_t adjacent_base_vaddr = base_vaddr + sizeof(uint64_t);
    const uintptr_t adjacent_direct_vaddr = direct_vaddr + sizeof(uint64_t);
    const uintptr_t paddr = phys_page_base(base_page) + 0x880U;
    const uintptr_t direct_paddr = phys_page_base(direct_page) + 0x180U;
    const uintptr_t adjacent_paddr = paddr + sizeof(uint64_t);
    const uintptr_t adjacent_direct_paddr = direct_paddr + sizeof(uint64_t);
    const uint64_t initial_value = 0x0123456789abcdefULL;
    const uint64_t delayed_store_value = 0xa1b2c3d4e5f60718ULL;
    const uint64_t direct_initial = 0x1021324354657687ULL;
    const uint64_t direct_store_value = 0x8877665544332211ULL;
    const uint64_t adjacent = 0x55aa33cc77ee11ffULL;
    const uint64_t adjacent_direct = 0xdeadbeefcafed00dULL;
    uint64_t observed_alias = 0ULL;
    uint64_t observed_direct = 0ULL;

    goto_priv(PRIV_M);
    write64(paddr, initial_value);
    write64(adjacent_paddr, adjacent);
    write64(direct_paddr, direct_initial);
    write64(adjacent_direct_paddr, adjacent_direct);

    goto_priv(PRIV_HS);
    hspt_init();

    goto_priv(PRIV_M);
    ai_micro_overlap_map_alias_to_base(alias_page, base_page);
    sfence_vma();

    goto_priv(PRIV_HS);
    TEST_SETUP_EXCEPT();
    uint64_t warmed_alias = ld(alias_vaddr);
    AI_ASSERT_NO_EXCEPT_AND("the alias HS load first observes the shared seeded payload before the forward-valid plus invalid-cause epoch-mix probe",
        warmed_alias == initial_value
    );

    TEST_SETUP_EXCEPT();
    uint64_t warmed_direct = ld(direct_vaddr);
    AI_ASSERT_NO_EXCEPT_AND("the independent HS load first observes its own seeded payload before the forward-valid plus invalid-cause epoch-mix probe",
        warmed_direct == direct_initial
    );

    TEST_SETUP_EXCEPT();
    ai_micro_overlap_delayed_data_store_then_store_then_two_loads_capture(
        base_vaddr, direct_vaddr, alias_vaddr, direct_vaddr,
        delayed_store_value, direct_store_value,
        &observed_alias, &observed_direct);
    TEST_ASSERT("with an older delayed-data store followed by a next-cycle independent direct store, each younger load stays bound to its own forward-query epoch instead of mixing valid with a neighbor cycle's invalid cause",
        excpt.triggered == false &&
        observed_alias == delayed_store_value &&
        observed_direct == direct_store_value
    );

    TEST_SETUP_EXCEPT();
    uint64_t observed_base = ld(base_vaddr);
    AI_ASSERT_NO_EXCEPT_AND("reloading through the base HS alias after the forward-valid plus invalid-cause epoch-mix probe returns the delayed store payload",
        observed_base == delayed_store_value
    );

    TEST_SETUP_EXCEPT();
    uint64_t observed_direct_again = ld(direct_vaddr);
    AI_ASSERT_NO_EXCEPT_AND("reloading through the independent HS address after the forward-valid plus invalid-cause epoch-mix probe returns the direct store payload",
        observed_direct_again == direct_store_value
    );

    TEST_SETUP_EXCEPT();
    uint64_t observed_adjacent = ld(adjacent_base_vaddr);
    AI_ASSERT_NO_EXCEPT_AND("the adjacent word next to the delayed-data store target remains unchanged across the forward-valid plus invalid-cause epoch-mix probe",
        observed_adjacent == adjacent
    );

    TEST_SETUP_EXCEPT();
    uint64_t observed_adjacent_direct = ld(adjacent_direct_vaddr);
    AI_ASSERT_NO_EXCEPT_AND("the adjacent word next to the independent direct-store target also remains unchanged across the forward-valid plus invalid-cause epoch-mix probe",
        observed_adjacent_direct == adjacent_direct
    );

    goto_priv(PRIV_M);
    TEST_ASSERT("the forward-valid plus invalid-cause epoch-mix corner updates each backing word exactly once with its own store result while preserving both adjacent words",
        read64(paddr) == delayed_store_value &&
        read64(direct_paddr) == direct_store_value &&
        read64(adjacent_paddr) == adjacent &&
        read64(adjacent_direct_paddr) == adjacent_direct
    );

    TEST_END();
}

bool ai_micro_hs_load_replay_reports_real_blocking_sqidx_not_next_cycle_stale_index_corner()
{
    TEST_START();

    const enum test_page base_page = VSRWX_GRWX;
    const enum test_page alias_page = SWITCH1;
    const enum test_page direct_page = SWITCH2;
    const uintptr_t base_vaddr = hs_page_base(base_page) + 0x8c0U;
    const uintptr_t alias_vaddr = hs_page_base(alias_page) + 0x8c0U;
    const uintptr_t direct_vaddr = hs_page_base(direct_page) + 0x1c0U;
    const uintptr_t adjacent_base_vaddr = base_vaddr + sizeof(uint64_t);
    const uintptr_t adjacent_direct_vaddr = direct_vaddr + sizeof(uint64_t);
    const uintptr_t paddr = phys_page_base(base_page) + 0x8c0U;
    const uintptr_t direct_paddr = phys_page_base(direct_page) + 0x1c0U;
    const uintptr_t adjacent_paddr = paddr + sizeof(uint64_t);
    const uintptr_t adjacent_direct_paddr = direct_paddr + sizeof(uint64_t);
    const uint64_t initial_value = 0x1111222233334444ULL;
    const uint64_t delayed_store_value = 0x5566778899aabbccULL;
    const uint64_t direct_initial = 0x0fedcba987654321ULL;
    const uint64_t direct_store_value = 0xddeeff0011223344ULL;
    const uint64_t adjacent = 0x8899aabbccddeeffULL;
    const uint64_t adjacent_direct = 0x13579bdf2468ace0ULL;
    uint64_t observed_base = 0ULL;
    uint64_t observed_direct = 0ULL;

    goto_priv(PRIV_M);
    write64(paddr, initial_value);
    write64(adjacent_paddr, adjacent);
    write64(direct_paddr, direct_initial);
    write64(adjacent_direct_paddr, adjacent_direct);

    goto_priv(PRIV_HS);
    hspt_init();

    goto_priv(PRIV_M);
    ai_micro_overlap_map_alias_to_base(alias_page, base_page);
    sfence_vma();

    goto_priv(PRIV_HS);
    TEST_SETUP_EXCEPT();
    uint64_t warmed_base = ld(base_vaddr);
    AI_ASSERT_NO_EXCEPT_AND("the base HS load first observes the shared seeded payload before the blocking-sqidx epoch-binding probe",
        warmed_base == initial_value
    );

    TEST_SETUP_EXCEPT();
    uint64_t warmed_direct = ld(direct_vaddr);
    AI_ASSERT_NO_EXCEPT_AND("the independent HS load first observes its own seeded payload before the blocking-sqidx epoch-binding probe",
        warmed_direct == direct_initial
    );

    TEST_SETUP_EXCEPT();
    ai_micro_overlap_delayed_addr_store_then_store_then_two_loads_capture(
        alias_vaddr, direct_vaddr, base_vaddr, direct_vaddr,
        delayed_store_value, direct_store_value,
        &observed_base, &observed_direct);
    TEST_ASSERT("with an older delayed-address alias store followed by a next-cycle independent direct store, each younger load replays only for its real blocking store instead of inheriting a stale next-cycle SQ index",
        excpt.triggered == false &&
        observed_base == delayed_store_value &&
        observed_direct == direct_store_value
    );

    TEST_SETUP_EXCEPT();
    uint64_t observed_alias = ld(alias_vaddr);
    AI_ASSERT_NO_EXCEPT_AND("reloading through the alias of the delayed-address store target after the blocking-sqidx epoch-binding probe returns the delayed store payload",
        observed_alias == delayed_store_value
    );

    TEST_SETUP_EXCEPT();
    uint64_t observed_direct_again = ld(direct_vaddr);
    AI_ASSERT_NO_EXCEPT_AND("reloading through the independent direct-store target after the blocking-sqidx epoch-binding probe returns the direct store payload",
        observed_direct_again == direct_store_value
    );

    TEST_SETUP_EXCEPT();
    uint64_t observed_adjacent = ld(adjacent_base_vaddr);
    AI_ASSERT_NO_EXCEPT_AND("the adjacent word next to the delayed-address store target remains unchanged across the blocking-sqidx epoch-binding probe",
        observed_adjacent == adjacent
    );

    TEST_SETUP_EXCEPT();
    uint64_t observed_adjacent_direct = ld(adjacent_direct_vaddr);
    AI_ASSERT_NO_EXCEPT_AND("the adjacent word next to the independent direct-store target also remains unchanged across the blocking-sqidx epoch-binding probe",
        observed_adjacent_direct == adjacent_direct
    );

    goto_priv(PRIV_M);
    TEST_ASSERT("the blocking-sqidx epoch-binding corner updates each backing word with only its own store result while preserving both adjacent words",
        read64(paddr) == delayed_store_value &&
        read64(direct_paddr) == direct_store_value &&
        read64(adjacent_paddr) == adjacent &&
        read64(adjacent_direct_paddr) == adjacent_direct
    );

    TEST_END();
}

bool ai_micro_hs_freelist_prealloc_free_then_realloc_ready_and_slot_stay_consistent_corner()
{
    TEST_START();

    const enum test_page base_page = VSRWX_GRWX;
    const enum test_page store_page = SWITCH1;
    const enum test_page load0_page = SWITCH2;
    const enum test_page load1_page = SCRATCHPAD;
    const uintptr_t first_base_vaddr = hs_page_base(base_page) + 0x900U;
    const uintptr_t second_base_vaddr = hs_page_base(base_page) + 0x980U;
    const uintptr_t first_store_vaddr = hs_page_base(store_page) + 0x900U;
    const uintptr_t second_store_vaddr = hs_page_base(store_page) + 0x980U;
    const uintptr_t first_load0_vaddr = hs_page_base(load0_page) + 0x900U;
    const uintptr_t second_load0_vaddr = hs_page_base(load0_page) + 0x980U;
    const uintptr_t first_load1_vaddr = hs_page_base(load1_page) + 0x900U;
    const uintptr_t second_load1_vaddr = hs_page_base(load1_page) + 0x980U;
    const uintptr_t first_adjacent_vaddr = first_base_vaddr + sizeof(uint64_t);
    const uintptr_t second_adjacent_vaddr = second_base_vaddr + sizeof(uint64_t);
    const uintptr_t first_paddr = phys_page_base(base_page) + 0x900U;
    const uintptr_t second_paddr = phys_page_base(base_page) + 0x980U;
    const uintptr_t first_adjacent_paddr = first_paddr + sizeof(uint64_t);
    const uintptr_t second_adjacent_paddr = second_paddr + sizeof(uint64_t);
    const uint64_t first_initial = 0x0123456789abcdefULL;
    const uint64_t second_initial = 0x1111222233334444ULL;
    const uint64_t first_episode_store = 0xa1b2c3d4e5f60718ULL;
    const uint64_t second_episode_store = 0x5566778899aabbccULL;
    const uint64_t third_episode_store = 0xddeeff0011223344ULL;
    const uint64_t first_adjacent = 0x55aa33cc77ee11ffULL;
    const uint64_t second_adjacent = 0x8899aabbccddeeffULL;
    uint64_t first_episode_load0 = 0ULL;
    uint64_t first_episode_load1 = 0ULL;
    uint64_t second_episode_load0 = 0ULL;
    uint64_t second_episode_load1 = 0ULL;
    uint64_t third_episode_load0 = 0ULL;
    uint64_t third_episode_load1 = 0ULL;

    goto_priv(PRIV_M);
    write64(first_paddr, first_initial);
    write64(second_paddr, second_initial);
    write64(first_adjacent_paddr, first_adjacent);
    write64(second_adjacent_paddr, second_adjacent);

    goto_priv(PRIV_HS);
    hspt_init();

    goto_priv(PRIV_M);
    ai_micro_overlap_map_alias_to_base(store_page, base_page);
    ai_micro_overlap_map_alias_to_base(load0_page, base_page);
    ai_micro_overlap_map_alias_to_base(load1_page, base_page);
    sfence_vma();

    goto_priv(PRIV_HS);
    ai_micro_overlap_enable_hs_cbo_inval();
    TEST_SETUP_EXCEPT();
    uint64_t warmed_first = ld(first_base_vaddr);
    AI_ASSERT_NO_EXCEPT_AND("the first replay-pressure target line is readable before the freelist prealloc free-then-realloc corner",
        warmed_first == first_initial
    );

    TEST_SETUP_EXCEPT();
    uint64_t warmed_second = ld(second_base_vaddr);
    AI_ASSERT_NO_EXCEPT_AND("the second replay-pressure target line is also readable before the freelist prealloc free-then-realloc corner",
        warmed_second == second_initial
    );

    TEST_SETUP_EXCEPT();
    cbo_inval(first_base_vaddr);
    AI_ASSERT_NO_EXCEPT_AND("an HS cbo.inval on the first target line retires without exception before the first cold-load replay episode in the freelist prealloc free-then-realloc corner",
        true
    );

    TEST_SETUP_EXCEPT();
    fence_iorw_iorw();
    AI_ASSERT_NO_EXCEPT_AND("the ordering fence after invalidating the first target line retires without exception before the first freelist prealloc replay episode",
        true
    );

    TEST_SETUP_EXCEPT();
    ai_micro_overlap_delayed_store_then_two_loads_capture(first_store_vaddr,
        first_load0_vaddr, first_load1_vaddr, first_episode_store,
        &first_episode_load0, &first_episode_load1);
    TEST_ASSERT("in the first cold-load replay episode, both younger alias loads still retire with the delayed store payload so the later free can hand the slot back cleanly",
        excpt.triggered == false &&
        first_episode_load0 == first_episode_store &&
        first_episode_load1 == first_episode_store
    );

    TEST_SETUP_EXCEPT();
    cbo_inval(second_base_vaddr);
    AI_ASSERT_NO_EXCEPT_AND("an HS cbo.inval on the second target line retires without exception immediately after the first episode in the freelist prealloc free-then-realloc corner",
        true
    );

    TEST_SETUP_EXCEPT();
    fence_iorw_iorw();
    AI_ASSERT_NO_EXCEPT_AND("the ordering fence after invalidating the second target line retires without exception before the immediate reallocation replay episode",
        true
    );

    TEST_SETUP_EXCEPT();
    ai_micro_overlap_delayed_store_then_two_loads_capture(second_store_vaddr,
        second_load0_vaddr, second_load1_vaddr, second_episode_store,
        &second_episode_load0, &second_episode_load1);
    TEST_ASSERT("after the first replay-pressure episode frees its tracking state, the very next cold-load replay episode on a different line still allocates cleanly and both younger alias loads retire with the new delayed store payload",
        excpt.triggered == false &&
        second_episode_load0 == second_episode_store &&
        second_episode_load1 == second_episode_store
    );

    TEST_SETUP_EXCEPT();
    cbo_inval(first_base_vaddr);
    AI_ASSERT_NO_EXCEPT_AND("a second HS cbo.inval on the first target line retires without exception before reusing the earlier replay slot again in the freelist prealloc corner",
        true
    );

    TEST_SETUP_EXCEPT();
    fence_iorw_iorw();
    AI_ASSERT_NO_EXCEPT_AND("the ordering fence after re-invalidating the first target line retires without exception before the reused-slot replay episode",
        true
    );

    TEST_SETUP_EXCEPT();
    ai_micro_overlap_delayed_store_then_two_loads_capture(first_store_vaddr,
        first_load0_vaddr, first_load1_vaddr, third_episode_store,
        &third_episode_load0, &third_episode_load1);
    TEST_ASSERT("reusing the earlier cold-load replay target immediately after the second episode still returns the third delayed store payload on both younger alias loads, so free-then-reallocate bookkeeping stays consistent across consecutive episodes",
        excpt.triggered == false &&
        third_episode_load0 == third_episode_store &&
        third_episode_load1 == third_episode_store
    );

    TEST_SETUP_EXCEPT();
    uint64_t observed_first_base = ld(first_base_vaddr);
    AI_ASSERT_NO_EXCEPT_AND("reloading the first target through the base HS alias after the freelist prealloc free-then-realloc corner returns the third delayed store payload",
        observed_first_base == third_episode_store
    );

    TEST_SETUP_EXCEPT();
    uint64_t observed_second_base = ld(second_base_vaddr);
    AI_ASSERT_NO_EXCEPT_AND("reloading the second target through the base HS alias after the freelist prealloc free-then-realloc corner returns the second delayed store payload",
        observed_second_base == second_episode_store
    );

    TEST_SETUP_EXCEPT();
    uint64_t observed_first_adjacent = ld(first_adjacent_vaddr);
    AI_ASSERT_NO_EXCEPT_AND("the adjacent word next to the first target remains unchanged across the freelist prealloc free-then-realloc corner",
        observed_first_adjacent == first_adjacent
    );

    TEST_SETUP_EXCEPT();
    uint64_t observed_second_adjacent = ld(second_adjacent_vaddr);
    AI_ASSERT_NO_EXCEPT_AND("the adjacent word next to the second target also remains unchanged across the freelist prealloc free-then-realloc corner",
        observed_second_adjacent == second_adjacent
    );

    goto_priv(PRIV_M);
    TEST_ASSERT("the freelist prealloc free-then-realloc corner leaves each backing word with only its latest delayed-store payload while preserving both adjacent words, showing that immediate reallocation after a free neither loses progress nor reuses a stale slot prediction",
        read64(first_paddr) == third_episode_store &&
        read64(second_paddr) == second_episode_store &&
        read64(first_adjacent_paddr) == first_adjacent &&
        read64(second_adjacent_paddr) == second_adjacent
    );
    ai_micro_overlap_disable_hs_cbo_inval();

    TEST_END();
}

bool ai_micro_hs_raw_replay_queue_same_cycle_free_next_cycle_alloc_no_ghost_full_or_dup_slot_corner()
{
    TEST_START();

    const enum test_page base_page = VSRWX_GRWX;
    const enum test_page store_page = SWITCH1;
    const enum test_page load0_page = SWITCH2;
    const enum test_page load1_page = SCRATCHPAD;
    const uintptr_t first_base_vaddr = hs_page_base(base_page) + 0xa00U;
    const uintptr_t second_base_vaddr = hs_page_base(base_page) + 0xa80U;
    const uintptr_t third_base_vaddr = hs_page_base(base_page) + 0xb00U;
    const uintptr_t first_store_vaddr = hs_page_base(store_page) + 0xa00U;
    const uintptr_t second_store_vaddr = hs_page_base(store_page) + 0xa80U;
    const uintptr_t third_store_vaddr = hs_page_base(store_page) + 0xb00U;
    const uintptr_t first_load0_vaddr = hs_page_base(load0_page) + 0xa00U;
    const uintptr_t second_load0_vaddr = hs_page_base(load0_page) + 0xa80U;
    const uintptr_t third_load0_vaddr = hs_page_base(load0_page) + 0xb00U;
    const uintptr_t first_load1_vaddr = hs_page_base(load1_page) + 0xa00U;
    const uintptr_t second_load1_vaddr = hs_page_base(load1_page) + 0xa80U;
    const uintptr_t third_load1_vaddr = hs_page_base(load1_page) + 0xb00U;
    const uintptr_t first_adjacent_vaddr = first_base_vaddr + sizeof(uint64_t);
    const uintptr_t second_adjacent_vaddr = second_base_vaddr + sizeof(uint64_t);
    const uintptr_t third_adjacent_vaddr = third_base_vaddr + sizeof(uint64_t);
    const uintptr_t recovery_vaddr = hs_page_base(base_page) + 0xb80U;
    const uintptr_t first_paddr = phys_page_base(base_page) + 0xa00U;
    const uintptr_t second_paddr = phys_page_base(base_page) + 0xa80U;
    const uintptr_t third_paddr = phys_page_base(base_page) + 0xb00U;
    const uintptr_t first_adjacent_paddr = first_paddr + sizeof(uint64_t);
    const uintptr_t second_adjacent_paddr = second_paddr + sizeof(uint64_t);
    const uintptr_t third_adjacent_paddr = third_paddr + sizeof(uint64_t);
    const uintptr_t recovery_paddr = phys_page_base(base_page) + 0xb80U;
    const uint64_t first_initial = 0x1021324354657687ULL;
    const uint64_t second_initial = 0x2233445566778899ULL;
    const uint64_t third_initial = 0x33445566778899aaULL;
    const uint64_t first_store_value = 0x0102030405060708ULL;
    const uint64_t second_store_value = 0x1122334455667788ULL;
    const uint64_t third_store_value = 0x99aabbccddeeff00ULL;
    const uint64_t first_adjacent = 0x55aa33cc77ee11ffULL;
    const uint64_t second_adjacent = 0x8899aabbccddeeffULL;
    const uint64_t third_adjacent = 0x0fedcba987654321ULL;
    const uint64_t recovery = 0x13579bdf2468ace0ULL;
    uint64_t first_load0 = 0ULL;
    uint64_t first_load1 = 0ULL;
    uint64_t second_load0 = 0ULL;
    uint64_t second_load1 = 0ULL;
    uint64_t third_load0 = 0ULL;
    uint64_t third_load1 = 0ULL;

    goto_priv(PRIV_M);
    write64(first_paddr, first_initial);
    write64(second_paddr, second_initial);
    write64(third_paddr, third_initial);
    write64(first_adjacent_paddr, first_adjacent);
    write64(second_adjacent_paddr, second_adjacent);
    write64(third_adjacent_paddr, third_adjacent);
    write64(recovery_paddr, recovery);

    goto_priv(PRIV_HS);
    hspt_init();

    goto_priv(PRIV_M);
    ai_micro_overlap_map_alias_to_base(store_page, base_page);
    ai_micro_overlap_map_alias_to_base(load0_page, base_page);
    ai_micro_overlap_map_alias_to_base(load1_page, base_page);
    sfence_vma();

    goto_priv(PRIV_HS);
    ai_micro_overlap_enable_hs_cbo_inval();
    TEST_SETUP_EXCEPT();
    uint64_t warmed_first = ld(first_base_vaddr);
    AI_ASSERT_NO_EXCEPT_AND("the first replay-pressure target line is readable before the raw-replay freelist ghost-full corner",
        warmed_first == first_initial
    );

    TEST_SETUP_EXCEPT();
    uint64_t warmed_second = ld(second_base_vaddr);
    AI_ASSERT_NO_EXCEPT_AND("the second replay-pressure target line is readable before the raw-replay freelist ghost-full corner",
        warmed_second == second_initial
    );

    TEST_SETUP_EXCEPT();
    uint64_t warmed_third = ld(third_base_vaddr);
    AI_ASSERT_NO_EXCEPT_AND("the third replay-pressure target line is readable before the raw-replay freelist ghost-full corner",
        warmed_third == third_initial
    );

    TEST_SETUP_EXCEPT();
    cbo_inval(first_base_vaddr);
    AI_ASSERT_NO_EXCEPT_AND("an HS cbo.inval on the first target line retires without exception before the first replay episode in the raw-replay freelist ghost-full corner",
        true
    );

    TEST_SETUP_EXCEPT();
    fence_iorw_iorw();
    AI_ASSERT_NO_EXCEPT_AND("the ordering fence after invalidating the first target line retires without exception before the first raw-replay freelist episode",
        true
    );

    TEST_SETUP_EXCEPT();
    ai_micro_overlap_delayed_store_then_two_loads_capture(first_store_vaddr,
        first_load0_vaddr, first_load1_vaddr, first_store_value,
        &first_load0, &first_load1);
    TEST_ASSERT("the first cold-load replay episode still makes forward progress and both younger alias loads retire with the first delayed store payload",
        excpt.triggered == false &&
        first_load0 == first_store_value &&
        first_load1 == first_store_value
    );

    TEST_SETUP_EXCEPT();
    cbo_inval(second_base_vaddr);
    AI_ASSERT_NO_EXCEPT_AND("an HS cbo.inval on the second target line retires without exception immediately before the second raw-replay freelist episode",
        true
    );

    TEST_SETUP_EXCEPT();
    fence_iorw_iorw();
    AI_ASSERT_NO_EXCEPT_AND("the ordering fence after invalidating the second target line retires without exception before the second raw-replay freelist episode",
        true
    );

    TEST_SETUP_EXCEPT();
    ai_micro_overlap_delayed_store_then_two_loads_capture(second_store_vaddr,
        second_load0_vaddr, second_load1_vaddr, second_store_value,
        &second_load0, &second_load1);
    TEST_ASSERT("the second immediate replay-pressure episode on a different line still allocates and both younger alias loads retire with the second delayed store payload, so there is no ghost-full backpressure from the freed earlier slot",
        excpt.triggered == false &&
        second_load0 == second_store_value &&
        second_load1 == second_store_value
    );

    TEST_SETUP_EXCEPT();
    cbo_inval(third_base_vaddr);
    AI_ASSERT_NO_EXCEPT_AND("an HS cbo.inval on the third target line retires without exception immediately before the third raw-replay freelist episode",
        true
    );

    TEST_SETUP_EXCEPT();
    fence_iorw_iorw();
    AI_ASSERT_NO_EXCEPT_AND("the ordering fence after invalidating the third target line retires without exception before the third raw-replay freelist episode",
        true
    );

    TEST_SETUP_EXCEPT();
    ai_micro_overlap_delayed_store_then_two_loads_capture(third_store_vaddr,
        third_load0_vaddr, third_load1_vaddr, third_store_value,
        &third_load0, &third_load1);
    TEST_ASSERT("the third consecutive replay-pressure episode also allocates cleanly and both younger alias loads retire with the third delayed store payload, so immediate free-then-next-cycle-alloc does not duplicate a stale slot id",
        excpt.triggered == false &&
        third_load0 == third_store_value &&
        third_load1 == third_store_value
    );

    TEST_SETUP_EXCEPT();
    uint64_t observed_first = ld(first_base_vaddr);
    AI_ASSERT_NO_EXCEPT_AND("reloading the first target after the consecutive raw-replay freelist episodes returns only the first delayed store payload",
        observed_first == first_store_value
    );

    TEST_SETUP_EXCEPT();
    uint64_t observed_second = ld(second_base_vaddr);
    AI_ASSERT_NO_EXCEPT_AND("reloading the second target after the consecutive raw-replay freelist episodes returns only the second delayed store payload",
        observed_second == second_store_value
    );

    TEST_SETUP_EXCEPT();
    uint64_t observed_third = ld(third_base_vaddr);
    AI_ASSERT_NO_EXCEPT_AND("reloading the third target after the consecutive raw-replay freelist episodes returns only the third delayed store payload",
        observed_third == third_store_value
    );

    TEST_SETUP_EXCEPT();
    uint64_t observed_first_adjacent = ld(first_adjacent_vaddr);
    AI_ASSERT_NO_EXCEPT_AND("the adjacent word next to the first target remains unchanged across the consecutive raw-replay freelist episodes",
        observed_first_adjacent == first_adjacent
    );

    TEST_SETUP_EXCEPT();
    uint64_t observed_second_adjacent = ld(second_adjacent_vaddr);
    AI_ASSERT_NO_EXCEPT_AND("the adjacent word next to the second target remains unchanged across the consecutive raw-replay freelist episodes",
        observed_second_adjacent == second_adjacent
    );

    TEST_SETUP_EXCEPT();
    uint64_t observed_third_adjacent = ld(third_adjacent_vaddr);
    AI_ASSERT_NO_EXCEPT_AND("the adjacent word next to the third target remains unchanged across the consecutive raw-replay freelist episodes",
        observed_third_adjacent == third_adjacent
    );

    TEST_SETUP_EXCEPT();
    uint64_t observed_recovery = ld(recovery_vaddr);
    AI_ASSERT_NO_EXCEPT_AND("the unrelated recovery word remains unchanged across the consecutive raw-replay freelist episodes",
        observed_recovery == recovery
    );

    goto_priv(PRIV_M);
    TEST_ASSERT("the consecutive raw-replay freelist corner leaves each backing word with only its own delayed-store payload while preserving all adjacent and unrelated words, indicating no ghost-full refusal and no duplicate stale slot reuse across immediate free-then-alloc boundaries",
        read64(first_paddr) == first_store_value &&
        read64(second_paddr) == second_store_value &&
        read64(third_paddr) == third_store_value &&
        read64(first_adjacent_paddr) == first_adjacent &&
        read64(second_adjacent_paddr) == second_adjacent &&
        read64(third_adjacent_paddr) == third_adjacent &&
        read64(recovery_paddr) == recovery
    );
    ai_micro_overlap_disable_hs_cbo_inval();

    TEST_END();
}

bool ai_micro_hs_load_replay_sq_ready_ptr_wrap_no_false_release_corner()
{
    TEST_START();

    const enum test_page base_page = VSRWX_GRWX;
    const enum test_page alias_page = SWITCH1;
    const enum test_page direct_page = SWITCH2;
    const enum test_page filler_page = SCRATCHPAD;
    const uintptr_t base_vaddr = hs_page_base(base_page) + 0x140U;
    const uintptr_t alias_vaddr = hs_page_base(alias_page) + 0x140U;
    const uintptr_t direct_vaddr = hs_page_base(direct_page) + 0x240U;
    const uintptr_t adjacent_base_vaddr = base_vaddr + sizeof(uint64_t);
    const uintptr_t filler_base_vaddr = hs_page_base(filler_page);
    const uintptr_t paddr = phys_page_base(base_page) + 0x140U;
    const uintptr_t direct_paddr = phys_page_base(direct_page) + 0x240U;
    const uintptr_t adjacent_paddr = paddr + sizeof(uint64_t);
    const uintptr_t filler_sample0_paddr = phys_page_base(filler_page);
    const uintptr_t filler_sample31_paddr = phys_page_base(filler_page) + 31U * 64U;
    const uintptr_t filler_sample63_paddr = phys_page_base(filler_page) + 63U * 64U;
    const uint64_t initial_value = 0x1111222233334444ULL;
    const uint64_t delayed_store_value0 = 0x5566778899aabbccULL;
    const uint64_t delayed_store_value1 = 0xa1b2c3d4e5f60718ULL;
    const uint64_t direct_initial = 0x0fedcba987654321ULL;
    const uint64_t direct_store_value0 = 0xddeeff0011223344ULL;
    const uint64_t direct_store_value1 = 0x13579bdf2468ace0ULL;
    const uint64_t adjacent = 0x8899aabbccddeeffULL;
    const uint64_t filler_seed0 = 0x0123456789abcdefULL;
    const uint64_t filler_seed1 = 0xfedcba9876543210ULL;
    const uint64_t filler_seed2 = 0x0f1e2d3c4b5a6978ULL;
    const uint64_t filler_seed3 = 0x89abcdef01234567ULL;
    uint64_t observed_base0 = 0ULL;
    uint64_t observed_direct0 = 0ULL;
    uint64_t observed_base1 = 0ULL;
    uint64_t observed_direct1 = 0ULL;

    goto_priv(PRIV_M);
    write64(paddr, initial_value);
    write64(adjacent_paddr, adjacent);
    write64(direct_paddr, direct_initial);
    write64(filler_sample0_paddr, 0ULL);
    write64(filler_sample31_paddr, 0ULL);
    write64(filler_sample63_paddr, 0ULL);

    goto_priv(PRIV_HS);
    hspt_init();

    goto_priv(PRIV_M);
    ai_micro_overlap_map_alias_to_base(alias_page, base_page);
    sfence_vma();

    goto_priv(PRIV_HS);
    TEST_SETUP_EXCEPT();
    uint64_t warmed_base = ld(base_vaddr);
    AI_ASSERT_NO_EXCEPT_AND("the replay-release wrap probe first warms the target line through the base HS alias",
        warmed_base == initial_value
    );

    TEST_SETUP_EXCEPT();
    uint64_t warmed_direct = ld(direct_vaddr);
    AI_ASSERT_NO_EXCEPT_AND("the replay-release wrap probe also warms the independent direct-store line before dense store pressure starts",
        warmed_direct == direct_initial
    );

    TEST_SETUP_EXCEPT();
    ai_micro_overlap_dense_store_pressure_window(filler_base_vaddr, 0U, 64U,
        filler_seed0, filler_seed1);
    AI_ASSERT_NO_EXCEPT_AND("a full-page dense HS store-pressure pass completes before the first replay-release boundary probe and leaves representative filler lines at the second-pass payload",
        ld(filler_base_vaddr + 0U * 64U) ==
            ai_micro_overlap_dense_store_pattern(filler_seed1, 0U) &&
        ld(filler_base_vaddr + 31U * 64U) ==
            ai_micro_overlap_dense_store_pattern(filler_seed1, 31U) &&
        ld(filler_base_vaddr + 63U * 64U) ==
            ai_micro_overlap_dense_store_pattern(filler_seed1, 63U)
    );

    TEST_SETUP_EXCEPT();
    ai_micro_overlap_delayed_addr_store_then_store_then_two_loads_capture(
        alias_vaddr, direct_vaddr, base_vaddr, direct_vaddr,
        delayed_store_value0, direct_store_value0,
        &observed_base0, &observed_direct0);
    TEST_ASSERT("after dense store pressure has moved store-ready boundaries across a full-page window, the younger alias load still waits for the delayed older store and the independent younger load binds only to its own direct store",
        excpt.triggered == false &&
        observed_base0 == delayed_store_value0 &&
        observed_direct0 == direct_store_value0
    );

    TEST_SETUP_EXCEPT();
    ai_micro_overlap_dense_store_pressure_window(filler_base_vaddr, 0U, 64U,
        filler_seed2, filler_seed3);
    AI_ASSERT_NO_EXCEPT_AND("a second full-page dense HS store-pressure pass also completes before the wrap-and-reuse replay-release probe and refreshes the filler lines to the new second-pass payload",
        ld(filler_base_vaddr + 0U * 64U) ==
            ai_micro_overlap_dense_store_pattern(filler_seed3, 0U) &&
        ld(filler_base_vaddr + 31U * 64U) ==
            ai_micro_overlap_dense_store_pattern(filler_seed3, 31U) &&
        ld(filler_base_vaddr + 63U * 64U) ==
            ai_micro_overlap_dense_store_pattern(filler_seed3, 63U)
    );

    TEST_SETUP_EXCEPT();
    ai_micro_overlap_delayed_addr_store_then_store_then_two_loads_capture(
        alias_vaddr, direct_vaddr, base_vaddr, direct_vaddr,
        delayed_store_value1, direct_store_value1,
        &observed_base1, &observed_direct1);
    TEST_ASSERT("after repeating the same replay-release boundary probe across another dense-store window, the target load still observes only the new delayed store payload and the independent load still observes only the new direct-store payload",
        excpt.triggered == false &&
        observed_base1 == delayed_store_value1 &&
        observed_direct1 == direct_store_value1
    );

    TEST_SETUP_EXCEPT();
    uint64_t observed_alias = ld(alias_vaddr);
    AI_ASSERT_NO_EXCEPT_AND("reloading through the alias after the repeated replay-release wrap probe returns only the second delayed store payload",
        observed_alias == delayed_store_value1
    );

    TEST_SETUP_EXCEPT();
    uint64_t observed_direct = ld(direct_vaddr);
    AI_ASSERT_NO_EXCEPT_AND("reloading the independent direct-store line after the repeated replay-release wrap probe returns only the second direct-store payload",
        observed_direct == direct_store_value1
    );

    TEST_SETUP_EXCEPT();
    uint64_t observed_adjacent = ld(adjacent_base_vaddr);
    AI_ASSERT_NO_EXCEPT_AND("the adjacent word next to the replay-release wrap target remains unchanged across both dense-store windows",
        observed_adjacent == adjacent
    );

    goto_priv(PRIV_M);
    TEST_ASSERT("the replay-release wrap corner leaves backing memory with only the second delayed store on the shared target, only the second direct store on the independent line, preserves the adjacent word, and leaves the representative filler lines at the latest dense-store payload",
        read64(paddr) == delayed_store_value1 &&
        read64(direct_paddr) == direct_store_value1 &&
        read64(adjacent_paddr) == adjacent &&
        read64(filler_sample0_paddr) ==
            ai_micro_overlap_dense_store_pattern(filler_seed3, 0U) &&
        read64(filler_sample31_paddr) ==
            ai_micro_overlap_dense_store_pattern(filler_seed3, 31U) &&
        read64(filler_sample63_paddr) ==
            ai_micro_overlap_dense_store_pattern(filler_seed3, 63U)
    );

    TEST_END();
}

bool ai_micro_hs_load_replay_dense_store_boundary_no_stuck_corner()
{
    TEST_START();

    const enum test_page base_page = VSRWX_GRWX;
    const enum test_page store_page = SWITCH1;
    const enum test_page load0_page = SWITCH2;
    const enum test_page load1_page = SCRATCHPAD;
    const uintptr_t base_vaddr = hs_page_base(base_page) + 0x180U;
    const uintptr_t store_vaddr = hs_page_base(store_page) + 0x180U;
    const uintptr_t load0_vaddr = hs_page_base(load0_page) + 0x180U;
    const uintptr_t load1_vaddr = hs_page_base(load1_page) + 0x180U;
    const uintptr_t adjacent_base_vaddr = base_vaddr + sizeof(uint64_t);
    const uintptr_t filler_page_base_vaddr = hs_page_base(base_page);
    const uintptr_t paddr = phys_page_base(base_page) + 0x180U;
    const uintptr_t adjacent_paddr = paddr + sizeof(uint64_t);
    const uintptr_t filler_sample32_paddr = phys_page_base(base_page) + 32U * 64U;
    const uintptr_t filler_sample43_paddr = phys_page_base(base_page) + 43U * 64U;
    const uintptr_t filler_sample55_paddr = phys_page_base(base_page) + 55U * 64U;
    const uint64_t initial_value = 0x0123456789abcdefULL;
    const uint64_t first_store_value = 0x5566778899aabbccULL;
    const uint64_t second_store_value = 0xa1b2c3d4e5f60718ULL;
    const uint64_t adjacent = 0x99aabbccddeeff00ULL;
    const uint64_t filler_seed0 = 0x13579bdf2468ace0ULL;
    const uint64_t filler_seed1 = 0x2468ace013579bdfULL;
    const uint64_t filler_seed2 = 0x0badc0ffee0ddf00ULL;
    const uint64_t filler_seed3 = 0x1122334455667788ULL;
    uint64_t first_load0 = 0ULL;
    uint64_t first_load1 = 0ULL;
    uint64_t second_load0 = 0ULL;
    uint64_t second_load1 = 0ULL;

    goto_priv(PRIV_M);
    write64(paddr, initial_value);
    write64(adjacent_paddr, adjacent);
    write64(filler_sample32_paddr, 0ULL);
    write64(filler_sample43_paddr, 0ULL);
    write64(filler_sample55_paddr, 0ULL);

    goto_priv(PRIV_HS);
    hspt_init();

    goto_priv(PRIV_M);
    ai_micro_overlap_map_alias_to_base(store_page, base_page);
    ai_micro_overlap_map_alias_to_base(load0_page, base_page);
    ai_micro_overlap_map_alias_to_base(load1_page, base_page);
    sfence_vma();

    goto_priv(PRIV_HS);
    ai_micro_overlap_enable_hs_cbo_inval();
    TEST_SETUP_EXCEPT();
    uint64_t warmed_base = ld(base_vaddr);
    AI_ASSERT_NO_EXCEPT_AND("the dense-store replay-boundary no-stuck probe first warms the target line through the base HS alias",
        warmed_base == initial_value
    );

    TEST_SETUP_EXCEPT();
    cbo_inval(base_vaddr);
    AI_ASSERT_NO_EXCEPT_AND("an HS cbo.inval on the replay-boundary target line retires without exception before the first dense-store replay probe",
        true
    );

    TEST_SETUP_EXCEPT();
    fence_iorw_iorw();
    AI_ASSERT_NO_EXCEPT_AND("the ordering fence after invalidating the replay-boundary target line retires without exception before the first dense-store replay probe",
        true
    );

    TEST_SETUP_EXCEPT();
    ai_micro_overlap_dense_store_pressure_window(filler_page_base_vaddr, 32U,
        24U, filler_seed0, filler_seed1);
    AI_ASSERT_NO_EXCEPT_AND("dense store pressure on a distant window of the same base page completes before the first replay-boundary probe and leaves representative pressure lines at the second-pass payload",
        ld(filler_page_base_vaddr + 32U * 64U) ==
            ai_micro_overlap_dense_store_pattern(filler_seed1, 32U) &&
        ld(filler_page_base_vaddr + 43U * 64U) ==
            ai_micro_overlap_dense_store_pattern(filler_seed1, 43U) &&
        ld(filler_page_base_vaddr + 55U * 64U) ==
            ai_micro_overlap_dense_store_pattern(filler_seed1, 55U)
    );

    TEST_SETUP_EXCEPT();
    ai_micro_overlap_delayed_store_then_two_loads_capture(store_vaddr,
        load0_vaddr, load1_vaddr, first_store_value,
        &first_load0, &first_load1);
    TEST_ASSERT("with replay pressure already created on the target line, both younger alias loads still retire with the first delayed store payload after the nearby dense store window, so the replay release does not falsely unblock or hang",
        excpt.triggered == false &&
        first_load0 == first_store_value &&
        first_load1 == first_store_value
    );

    TEST_SETUP_EXCEPT();
    cbo_inval(base_vaddr);
    AI_ASSERT_NO_EXCEPT_AND("a second HS cbo.inval on the replay-boundary target line retires without exception before the repeated dense-store replay probe",
        true
    );

    TEST_SETUP_EXCEPT();
    fence_iorw_iorw();
    AI_ASSERT_NO_EXCEPT_AND("the ordering fence after the second invalidation retires without exception before the repeated dense-store replay probe",
        true
    );

    TEST_SETUP_EXCEPT();
    ai_micro_overlap_dense_store_pressure_window(filler_page_base_vaddr, 32U,
        24U, filler_seed2, filler_seed3);
    AI_ASSERT_NO_EXCEPT_AND("a second dense store window on the same distant base-page lines also completes before the repeated replay-boundary probe and refreshes the representative pressure lines to the new second-pass payload",
        ld(filler_page_base_vaddr + 32U * 64U) ==
            ai_micro_overlap_dense_store_pattern(filler_seed3, 32U) &&
        ld(filler_page_base_vaddr + 43U * 64U) ==
            ai_micro_overlap_dense_store_pattern(filler_seed3, 43U) &&
        ld(filler_page_base_vaddr + 55U * 64U) ==
            ai_micro_overlap_dense_store_pattern(filler_seed3, 55U)
    );

    TEST_SETUP_EXCEPT();
    ai_micro_overlap_delayed_store_then_two_loads_capture(store_vaddr,
        load0_vaddr, load1_vaddr, second_store_value,
        &second_load0, &second_load1);
    TEST_ASSERT("after repeating the replay-boundary probe across a fresh dense-store window, both younger alias loads still retire with the second delayed store payload so the replay path makes forward progress instead of getting stuck on stale store-ready boundaries",
        excpt.triggered == false &&
        second_load0 == second_store_value &&
        second_load1 == second_store_value
    );

    TEST_SETUP_EXCEPT();
    uint64_t observed_base = ld(base_vaddr);
    AI_ASSERT_NO_EXCEPT_AND("reloading the target through the base HS alias after the repeated dense-store replay probe returns only the second delayed store payload",
        observed_base == second_store_value
    );

    TEST_SETUP_EXCEPT();
    uint64_t observed_load0 = ld(load0_vaddr);
    AI_ASSERT_NO_EXCEPT_AND("reloading through the first younger-load alias after the repeated dense-store replay probe also returns only the second delayed store payload",
        observed_load0 == second_store_value
    );

    TEST_SETUP_EXCEPT();
    uint64_t observed_load1 = ld(load1_vaddr);
    AI_ASSERT_NO_EXCEPT_AND("reloading through the second younger-load alias after the repeated dense-store replay probe also returns only the second delayed store payload",
        observed_load1 == second_store_value
    );

    TEST_SETUP_EXCEPT();
    uint64_t observed_adjacent = ld(adjacent_base_vaddr);
    AI_ASSERT_NO_EXCEPT_AND("the adjacent word next to the replay-boundary target remains unchanged across both dense-store replay probes",
        observed_adjacent == adjacent
    );

    goto_priv(PRIV_M);
    TEST_ASSERT("the dense-store replay-boundary no-stuck corner leaves backing memory with only the second delayed store on the target, preserves the adjacent word, and leaves the representative pressure lines at the latest second-pass payload",
        read64(paddr) == second_store_value &&
        read64(adjacent_paddr) == adjacent &&
        read64(filler_sample32_paddr) ==
            ai_micro_overlap_dense_store_pattern(filler_seed3, 32U) &&
        read64(filler_sample43_paddr) ==
            ai_micro_overlap_dense_store_pattern(filler_seed3, 43U) &&
        read64(filler_sample55_paddr) ==
            ai_micro_overlap_dense_store_pattern(filler_seed3, 55U)
    );
    ai_micro_overlap_disable_hs_cbo_inval();

    TEST_END();
}

bool ai_micro_hs_load_replay_escape_zero_threshold_reversed_alias_pair_no_stale_corner()
{
    TEST_START();

    const enum test_page base_page = VSRWX_GRWX;
    const enum test_page store_page = SWITCH1;
    const enum test_page first_load_page = SWITCH2;
    const enum test_page second_load_page = SCRATCHPAD;
    const uintptr_t base_vaddr = hs_page_base(base_page) + 0x280U;
    const uintptr_t store_vaddr = hs_page_base(store_page) + 0x280U;
    const uintptr_t first_load_vaddr = hs_page_base(first_load_page) + 0x280U;
    const uintptr_t second_load_vaddr = hs_page_base(second_load_page) + 0x280U;
    const uintptr_t adjacent_base_vaddr = base_vaddr + sizeof(uint64_t);
    const uintptr_t filler_page_base_vaddr = hs_page_base(base_page);
    const uintptr_t paddr = phys_page_base(base_page) + 0x280U;
    const uintptr_t adjacent_paddr = paddr + sizeof(uint64_t);
    const uintptr_t filler_sample40_paddr = phys_page_base(base_page) + 40U * 64U;
    const uintptr_t filler_sample47_paddr = phys_page_base(base_page) + 47U * 64U;
    const uintptr_t filler_sample55_paddr = phys_page_base(base_page) + 55U * 64U;
    const uint64_t initial_value = 0x0f1e2d3c4b5a6978ULL;
    const uint64_t first_store_value = 0x123456789abcdef0ULL;
    const uint64_t second_store_value = 0xfedcba9876543210ULL;
    const uint64_t adjacent = 0x8877665544332211ULL;
    const uint64_t filler_seed0 = 0x1021324354657687ULL;
    const uint64_t filler_seed1 = 0x9182736455463728ULL;
    const uint64_t filler_seed2 = 0x0ace13579bdf2468ULL;
    const uint64_t filler_seed3 = 0x2468ace013579bdfULL;
    uint64_t original_smblockctl = 0ULL;
    uint64_t first_observed_first_alias = 0ULL;
    uint64_t first_observed_second_alias = 0ULL;
    uint64_t second_observed_first_alias = 0ULL;
    uint64_t second_observed_second_alias = 0ULL;

    goto_priv(PRIV_M);
    write64(paddr, initial_value);
    write64(adjacent_paddr, adjacent);
    write64(filler_sample40_paddr, 0ULL);
    write64(filler_sample47_paddr, 0ULL);
    write64(filler_sample55_paddr, 0ULL);

    TEST_SETUP_EXCEPT();
    original_smblockctl = ai_micro_overlap_read_smblockctl();
    TEST_ASSERT("smblockctl is readable before forcing zero-threshold replay_escape for the reversed same-paddr alias replay probe",
        excpt.triggered == false
    );

    TEST_SETUP_EXCEPT();
    ai_micro_overlap_write_smblockctl(
        (original_smblockctl & ~AI_MICRO_OVERLAP_REPLAY_ESCAPE_TIME_MASK) |
        AI_MICRO_OVERLAP_REPLAY_ESCAPE_ENABLE
    );
    TEST_ASSERT("programming replay_escape enable with threshold zero completes before the reversed alias replay probe",
        excpt.triggered == false &&
        ((ai_micro_overlap_read_smblockctl() & (AI_MICRO_OVERLAP_REPLAY_ESCAPE_ENABLE |
            AI_MICRO_OVERLAP_REPLAY_ESCAPE_TIME_MASK)) ==
            AI_MICRO_OVERLAP_REPLAY_ESCAPE_ENABLE)
    );

    goto_priv(PRIV_HS);
    hspt_init();

    goto_priv(PRIV_M);
    ai_micro_overlap_map_alias_to_base(store_page, base_page);
    ai_micro_overlap_map_alias_to_base(first_load_page, base_page);
    ai_micro_overlap_map_alias_to_base(second_load_page, base_page);
    sfence_vma();

    goto_priv(PRIV_HS);
    ai_micro_overlap_enable_hs_cbo_inval();

    TEST_SETUP_EXCEPT();
    uint64_t warmed_base = ld(base_vaddr);
    AI_ASSERT_NO_EXCEPT_AND("the zero-threshold reversed-alias replay probe first warms the target line through the base HS alias",
        warmed_base == initial_value
    );

    TEST_SETUP_EXCEPT();
    cbo_inval(base_vaddr);
    AI_ASSERT_NO_EXCEPT_AND("the first HS cbo.inval retires before the zero-threshold reversed-alias replay probe",
        true
    );

    TEST_SETUP_EXCEPT();
    fence_iorw_iorw();
    AI_ASSERT_NO_EXCEPT_AND("the fence after the first invalidation retires before the zero-threshold reversed-alias replay probe",
        true
    );

    TEST_SETUP_EXCEPT();
    ai_micro_overlap_dense_store_pressure_window(filler_page_base_vaddr, 40U,
        16U, filler_seed0, filler_seed1);
    AI_ASSERT_NO_EXCEPT_AND("dense store pressure completes before the first reversed-alias replay probe and refreshes representative filler lines",
        ld(filler_page_base_vaddr + 40U * 64U) ==
            ai_micro_overlap_dense_store_pattern(filler_seed1, 40U) &&
        ld(filler_page_base_vaddr + 47U * 64U) ==
            ai_micro_overlap_dense_store_pattern(filler_seed1, 47U) &&
        ld(filler_page_base_vaddr + 55U * 64U) ==
            ai_micro_overlap_dense_store_pattern(filler_seed1, 55U)
    );

    TEST_SETUP_EXCEPT();
    ai_micro_overlap_delayed_store_then_two_loads_capture(store_vaddr,
        second_load_vaddr, first_load_vaddr, first_store_value,
        &first_observed_second_alias, &first_observed_first_alias);
    TEST_ASSERT("with replay_escape forced immediately, the first reversed same-paddr alias load pair both observe the delayed store payload and neither alias sees a stale cache image",
        excpt.triggered == false &&
        first_observed_first_alias == first_store_value &&
        first_observed_second_alias == first_store_value
    );

    TEST_SETUP_EXCEPT();
    cbo_inval(base_vaddr);
    AI_ASSERT_NO_EXCEPT_AND("the second HS cbo.inval retires before the repeated zero-threshold reversed-alias replay probe",
        true
    );

    TEST_SETUP_EXCEPT();
    fence_iorw_iorw();
    AI_ASSERT_NO_EXCEPT_AND("the fence after the second invalidation retires before the repeated zero-threshold reversed-alias replay probe",
        true
    );

    TEST_SETUP_EXCEPT();
    ai_micro_overlap_dense_store_pressure_window(filler_page_base_vaddr, 40U,
        16U, filler_seed2, filler_seed3);
    AI_ASSERT_NO_EXCEPT_AND("the second dense store pressure window completes before the repeated reversed-alias replay probe and refreshes representative filler lines",
        ld(filler_page_base_vaddr + 40U * 64U) ==
            ai_micro_overlap_dense_store_pattern(filler_seed3, 40U) &&
        ld(filler_page_base_vaddr + 47U * 64U) ==
            ai_micro_overlap_dense_store_pattern(filler_seed3, 47U) &&
        ld(filler_page_base_vaddr + 55U * 64U) ==
            ai_micro_overlap_dense_store_pattern(filler_seed3, 55U)
    );

    TEST_SETUP_EXCEPT();
    ai_micro_overlap_delayed_store_then_two_loads_capture(store_vaddr,
        second_load_vaddr, first_load_vaddr, second_store_value,
        &second_observed_second_alias, &second_observed_first_alias);
    TEST_ASSERT("after repeating the zero-threshold replay_escape probe with the two younger aliases issued in reverse order, both aliases still observe the second delayed store payload",
        excpt.triggered == false &&
        second_observed_first_alias == second_store_value &&
        second_observed_second_alias == second_store_value
    );

    TEST_SETUP_EXCEPT();
    uint64_t observed_base = ld(base_vaddr);
    uint64_t observed_first_alias = ld(first_load_vaddr);
    uint64_t observed_second_alias = ld(second_load_vaddr);
    uint64_t observed_adjacent = ld(adjacent_base_vaddr);
    AI_ASSERT_NO_EXCEPT_AND("all same-paddr aliases reload the final delayed-store payload after the zero-threshold reversed-alias replay probe and the adjacent word remains unchanged",
        observed_base == second_store_value &&
        observed_first_alias == second_store_value &&
        observed_second_alias == second_store_value &&
        observed_adjacent == adjacent
    );

    ai_micro_overlap_disable_hs_cbo_inval();

    goto_priv(PRIV_M);
    TEST_SETUP_EXCEPT();
    ai_micro_overlap_write_smblockctl(original_smblockctl);
    TEST_ASSERT("restoring smblockctl after the zero-threshold reversed-alias replay probe completes without exception",
        excpt.triggered == false
    );

    TEST_ASSERT("the zero-threshold reversed-alias replay probe leaves backing memory with only the second delayed store on the shared target, preserves the adjacent word, and leaves filler lines at the latest dense-store payload",
        read64(paddr) == second_store_value &&
        read64(adjacent_paddr) == adjacent &&
        read64(filler_sample40_paddr) ==
            ai_micro_overlap_dense_store_pattern(filler_seed3, 40U) &&
        read64(filler_sample47_paddr) ==
            ai_micro_overlap_dense_store_pattern(filler_seed3, 47U) &&
        read64(filler_sample55_paddr) ==
            ai_micro_overlap_dense_store_pattern(filler_seed3, 55U)
    );

    TEST_END();
}

bool ai_micro_hs_replay_like_load_with_nearby_cross16_mab_wb_no_lost_completion_corner()
{
    TEST_START();
    test_status = test_status &&
        ai_micro_overlap_replay_like_with_nearby_cross16_mab_window(false);
    TEST_END();
}

bool ai_micro_hs_replay_like_load_with_nearby_cross16_mab_wb_adjacent_preserved_corner()
{
    TEST_START();
    test_status = test_status &&
        ai_micro_overlap_replay_like_with_nearby_cross16_mab_window(true);
    TEST_END();
}

bool ai_micro_hs_storequeue_older_sbuffer_handoff_then_younger_rewrite_not_dropped_corner()
{
    TEST_START();
    bool ok = ai_micro_overlap_storequeue_older_sbuffer_handoff_rewrite_window(false);
    TEST_ASSERT("older SBuffer handoff delayed clear does not drop the younger hot-word rewrite",
        ok
    );
    TEST_END();
}

bool ai_micro_hs_storequeue_older_sbuffer_handoff_then_younger_rewrite_adjacent_preserved_corner()
{
    TEST_START();
    bool ok = ai_micro_overlap_storequeue_older_sbuffer_handoff_rewrite_window(true);
    TEST_ASSERT("older SBuffer handoff plus younger rewrite preserves companion words and adjacent sentinels",
        ok
    );
    TEST_END();
}

bool ai_micro_hs_databuffer_two_pair_turnover_no_swap_or_drop_corner()
{
    TEST_START();
    bool ok = ai_micro_overlap_databuffer_two_pair_turnover_window(false);
    TEST_ASSERT("dense two-pair DataBuffer turnover keeps younger store pairs ordered without swap or drop",
        ok
    );
    TEST_END();
}

bool ai_micro_hs_databuffer_two_pair_turnover_adjacent_preserved_corner()
{
    TEST_START();
    bool ok = ai_micro_overlap_databuffer_two_pair_turnover_window(true);
    TEST_ASSERT("DataBuffer two-pair turnover preserves younger pairs and adjacent guard words",
        ok
    );
    TEST_END();
}

bool ai_micro_hs_storequeue_cross16_pseudo_second_half_then_younger_aligned_batch_no_drop_corner()
{
    TEST_START();
    bool ok = ai_micro_overlap_cross16_pseudo_second_half_aligned_batch_window(false);
    TEST_ASSERT("cross16 pseudo high-half handoff does not rebind or drop the following aligned store batch",
        ok
    );
    TEST_END();
}

bool ai_micro_hs_storequeue_cross16_pseudo_second_half_then_younger_aligned_batch_adjacent_preserved_corner()
{
    TEST_START();
    bool ok = ai_micro_overlap_cross16_pseudo_second_half_aligned_batch_window(true);
    TEST_ASSERT("cross16 pseudo high-half plus younger aligned batch preserves split fragments and adjacent sentinels",
        ok
    );
    TEST_END();
}

bool ai_micro_hs_storequeue_cross16_high_half_then_younger_same_word_rewrite_keeps_youngest_data_corner()
{
    TEST_START();
    bool ok = ai_micro_overlap_cross16_high_half_younger_rewrite_window(false);
    TEST_ASSERT("cross16 high-half pseudo beat cannot overwrite a younger same-word rewrite",
        ok
    );
    TEST_END();
}

bool ai_micro_hs_storequeue_cross16_high_half_then_younger_same_word_rewrite_adjacent_preserved_corner()
{
    TEST_START();
    bool ok = ai_micro_overlap_cross16_high_half_younger_rewrite_window(true);
    TEST_ASSERT("cross16 high-half younger rewrite keeps youngest data and adjacent guard words",
        ok
    );
    TEST_END();
}

bool ai_micro_hs_storequeue_cross16_scalar_vector_wb_not_false_split_completion_corner()
{
    TEST_START();
    bool ok = ai_micro_overlap_cross16_scalar_vector_wb_window(false);
    TEST_ASSERT("unrelated vector store writeback cannot falsely complete an older scalar cross16 split before a younger high-word rewrite",
        ok
    );
    TEST_END();
}

bool ai_micro_hs_storequeue_cross16_scalar_vector_wb_not_false_split_completion_adjacent_preserved_corner()
{
    TEST_START();
    bool ok = ai_micro_overlap_cross16_scalar_vector_wb_window(true);
    TEST_ASSERT("scalar cross16 plus unrelated vector writeback preserves scalar byte-age image, vector payload, and adjacent sentinels",
        ok
    );
    TEST_END();
}

bool ai_micro_hs_storequeue_cross16_scalar_vector_wb_same_boundary_next_split_keeps_latest_image_corner()
{
    TEST_START();
    bool ok = ai_micro_overlap_cross16_vector_wb_same_boundary_refresh_window(false);
    TEST_ASSERT("unrelated vector writeback cannot free scalar cross16 delayed completion before the next same-boundary split refresh",
        ok
    );
    TEST_END();
}

bool ai_micro_hs_storequeue_cross16_scalar_vector_wb_same_boundary_next_split_adjacent_preserved_corner()
{
    TEST_START();
    bool ok = ai_micro_overlap_cross16_vector_wb_same_boundary_refresh_window(true);
    TEST_ASSERT("same-boundary scalar cross16 refresh after unrelated vector writeback preserves latest split image and sentinels",
        ok
    );
    TEST_END();
}

bool ai_micro_hs_storequeue_cross16_scalar_vector_wb_next_boundary_no_cross_bind_corner()
{
    TEST_START();
    bool ok = ai_micro_overlap_cross16_vector_wb_cross_boundary_window(false);
    TEST_ASSERT("unrelated vector writeback cannot rebind scalar cross16 delayed completion into the next boundary",
        ok
    );
    TEST_END();
}

bool ai_micro_hs_storequeue_cross16_scalar_vector_wb_next_boundary_adjacent_preserved_corner()
{
    TEST_START();
    bool ok = ai_micro_overlap_cross16_vector_wb_cross_boundary_window(true);
    TEST_ASSERT("cross-boundary scalar cross16 after unrelated vector writeback preserves both boundary images and guards",
        ok
    );
    TEST_END();
}

bool ai_micro_hs_storequeue_cross16_scalar_vector_wb_next_boundary_width_switch_no_stale_template_corner()
{
    TEST_START();
    bool ok = ai_micro_overlap_cross16_vector_wb_cross_boundary_width_switch_window(false);
    TEST_ASSERT("unrelated vector writeback cannot carry an older scalar sw split template into a younger sd boundary",
        ok
    );
    TEST_END();
}

bool ai_micro_hs_storequeue_cross16_scalar_vector_wb_next_boundary_width_switch_adjacent_preserved_corner()
{
    TEST_START();
    bool ok = ai_micro_overlap_cross16_vector_wb_cross_boundary_width_switch_window(true);
    TEST_ASSERT("scalar cross16 sw-to-sd width switch after vector writeback preserves mixed-width split images and guards",
        ok
    );
    TEST_END();
}

bool ai_micro_hs_storequeue_cross16_scalar_vector_wb_next_boundary_narrower_switch_merges_by_byte_age_corner()
{
    TEST_START();
    bool ok = ai_micro_overlap_cross16_vector_wb_cross_boundary_narrower_switch_window(false);
    TEST_ASSERT("unrelated vector writeback cannot carry an older scalar sd split template into a younger sw boundary",
        ok
    );
    TEST_END();
}

bool ai_micro_hs_storequeue_cross16_scalar_vector_wb_next_boundary_narrower_switch_adjacent_preserved_corner()
{
    TEST_START();
    bool ok = ai_micro_overlap_cross16_vector_wb_cross_boundary_narrower_switch_window(true);
    TEST_ASSERT("scalar cross16 sd-to-sw narrower switch after vector writeback preserves byte-age image and guards",
        ok
    );
    TEST_END();
}

bool ai_micro_hs_storequeue_cross16_scalar_vector_wb_same_boundary_three_shape_refresh_no_stale_older_generation_corner()
{
    TEST_START();
    bool ok = ai_micro_overlap_cross16_vector_wb_same_boundary_three_shape_window(false);
    TEST_ASSERT("unrelated vector writeback cannot revive older scalar split generations across same-boundary sd-sw-sh refresh",
        ok
    );
    TEST_END();
}

bool ai_micro_hs_storequeue_cross16_scalar_vector_wb_same_boundary_three_shape_adjacent_preserved_corner()
{
    TEST_START();
    bool ok = ai_micro_overlap_cross16_vector_wb_same_boundary_three_shape_window(true);
    TEST_ASSERT("same-boundary scalar sd-sw-sh refresh after vector writeback preserves final byte-age image and sentinels",
        ok
    );
    TEST_END();
}

bool ai_micro_hs_storequeue_cross16_low_half_then_younger_same_word_rewrite_keeps_youngest_data_corner()
{
    TEST_START();
    bool ok = ai_micro_overlap_cross16_low_half_younger_rewrite_window(false);
    TEST_ASSERT("cross16 low-half owner cannot survive a younger same-word rewrite",
        ok
    );
    TEST_END();
}

bool ai_micro_hs_storequeue_cross16_low_half_then_younger_same_word_rewrite_adjacent_preserved_corner()
{
    TEST_START();
    bool ok = ai_micro_overlap_cross16_low_half_younger_rewrite_window(true);
    TEST_ASSERT("cross16 low-half younger rewrite keeps youngest data and adjacent guard words",
        ok
    );
    TEST_END();
}

bool ai_micro_hs_storequeue_consecutive_cross16_boundaries_no_cross_bind_or_drop_corner()
{
    TEST_START();
    test_status = test_status &&
        ai_micro_overlap_consecutive_cross16_boundary_window(false);
    TEST_END();
}

bool ai_micro_hs_storequeue_consecutive_cross16_boundaries_adjacent_preserved_corner()
{
    TEST_START();
    test_status = test_status &&
        ai_micro_overlap_consecutive_cross16_boundary_window(true);
    TEST_END();
}

bool ai_micro_hs_storequeue_consecutive_cross16_same_boundary_keeps_latest_split_image_corner()
{
    TEST_START();
    test_status = test_status &&
        ai_micro_overlap_consecutive_cross16_same_boundary_window(false);
    TEST_END();
}

bool ai_micro_hs_storequeue_consecutive_cross16_same_boundary_adjacent_preserved_corner()
{
    TEST_START();
    test_status = test_status &&
        ai_micro_overlap_consecutive_cross16_same_boundary_window(true);
    TEST_END();
}

bool ai_micro_hs_storequeue_consecutive_cross16_same_boundary_width_switch_no_stale_template_corner()
{
    TEST_START();
    test_status = test_status &&
        ai_micro_overlap_cross16_same_boundary_width_switch_window(false);
    TEST_END();
}

bool ai_micro_hs_storequeue_consecutive_cross16_same_boundary_width_switch_adjacent_preserved_corner()
{
    TEST_START();
    test_status = test_status &&
        ai_micro_overlap_cross16_same_boundary_width_switch_window(true);
    TEST_END();
}

bool ai_micro_hs_storequeue_consecutive_cross16_same_boundary_narrower_switch_merges_by_byte_age_corner()
{
    TEST_START();
    test_status = test_status &&
        ai_micro_overlap_cross16_same_boundary_narrower_switch_window(false);
    TEST_END();
}

bool ai_micro_hs_storequeue_consecutive_cross16_same_boundary_narrower_switch_adjacent_preserved_corner()
{
    TEST_START();
    test_status = test_status &&
        ai_micro_overlap_cross16_same_boundary_narrower_switch_window(true);
    TEST_END();
}

bool ai_micro_hs_storequeue_consecutive_cross16_same_boundary_three_shape_refresh_no_stale_older_generation_corner()
{
    TEST_START();
    test_status = test_status &&
        ai_micro_overlap_cross16_same_boundary_three_shape_window(false);
    TEST_END();
}

bool ai_micro_hs_storequeue_consecutive_cross16_same_boundary_three_shape_adjacent_preserved_corner()
{
    TEST_START();
    test_status = test_status &&
        ai_micro_overlap_cross16_same_boundary_three_shape_window(true);
    TEST_END();
}

bool ai_micro_hs_databuffer_aligned_pair_then_cross16_split_no_cross_bind_or_drop_corner()
{
    TEST_START();
    bool ok = ai_micro_overlap_aligned_pair_then_cross16_turnover_window(false);
    TEST_ASSERT("aligned pair followed by a cross16 split keeps independent aligned stores and split boundary bytes in their own generations",
        ok
    );
    TEST_END();
}

bool ai_micro_hs_databuffer_aligned_pair_then_cross16_split_adjacent_preserved_corner()
{
    TEST_START();
    bool ok = ai_micro_overlap_aligned_pair_then_cross16_turnover_window(true);
    TEST_ASSERT("aligned pair followed by a cross16 split preserves aligned targets, split boundary bytes, and adjacent guards",
        ok
    );
    TEST_END();
}

bool ai_micro_hs_databuffer_aligned_pair_then_same_boundary_cross16_merges_by_byte_age_corner()
{
    TEST_START();
    bool ok = ai_micro_overlap_same_boundary_aligned_pair_then_cross16_window(false);
    TEST_ASSERT("same-boundary aligned low/high pair plus younger cross16 split merges strictly by byte age",
        ok
    );
    TEST_END();
}

bool ai_micro_hs_databuffer_aligned_pair_then_same_boundary_cross16_adjacent_preserved_corner()
{
    TEST_START();
    bool ok = ai_micro_overlap_same_boundary_aligned_pair_then_cross16_window(true);
    TEST_ASSERT("same-boundary aligned pair plus younger cross16 split preserves byte-age image and outer guards",
        ok
    );
    TEST_END();
}

bool ai_micro_hs_databuffer_same_boundary_cross16_then_aligned_pair_fully_refreshes_boundary_corner()
{
    TEST_START();
    bool ok = ai_micro_overlap_same_boundary_cross16_then_aligned_pair_window(false);
    TEST_ASSERT("younger aligned pair fully refreshes both boundary words after an older same-boundary cross16 split",
        ok
    );
    TEST_END();
}

bool ai_micro_hs_databuffer_same_boundary_cross16_then_aligned_pair_adjacent_preserved_corner()
{
    TEST_START();
    bool ok = ai_micro_overlap_same_boundary_cross16_then_aligned_pair_window(true);
    TEST_ASSERT("same-boundary cross16 then younger aligned pair preserves final aligned image and outer guards",
        ok
    );
    TEST_END();
}

bool ai_micro_hs_databuffer_same_boundary_split_aligned_split_no_oldest_leak_corner()
{
    TEST_START();
    bool ok = ai_micro_overlap_same_boundary_split_aligned_split_window(false);
    TEST_ASSERT("middle aligned refresh prevents the oldest split template from leaking into a younger same-boundary split image",
        ok
    );
    TEST_END();
}

bool ai_micro_hs_databuffer_same_boundary_split_aligned_split_adjacent_preserved_corner()
{
    TEST_START();
    bool ok = ai_micro_overlap_same_boundary_split_aligned_split_window(true);
    TEST_ASSERT("split-aligned-split same-boundary generation order preserves the byte-age image and outer guards",
        ok
    );
    TEST_END();
}

bool ai_micro_hs_databuffer_same_boundary_aligned_split_aligned_no_middle_leak_corner()
{
    TEST_START();
    bool ok = ai_micro_overlap_same_boundary_aligned_split_aligned_window(false);
    TEST_ASSERT("final aligned pair suppresses the middle split-store image on the same boundary",
        ok
    );
    TEST_END();
}

bool ai_micro_hs_databuffer_same_boundary_aligned_split_aligned_adjacent_preserved_corner()
{
    TEST_START();
    bool ok = ai_micro_overlap_same_boundary_aligned_split_aligned_window(true);
    TEST_ASSERT("aligned-split-aligned same-boundary generation order preserves final aligned image and outer guards",
        ok
    );
    TEST_END();
}
