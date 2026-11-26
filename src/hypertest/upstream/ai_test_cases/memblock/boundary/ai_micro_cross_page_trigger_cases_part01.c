#include "ai_micro_cross_page_trigger_common.h"

bool ai_micro_hs_vector_store_trigger_earlier_than_high_half_access_fault_repeated_breakpoint_corner()
{
    TEST_START();
    test_status = test_status &&
        ai_micro_cross_page_vector_store_trigger_vs_access_fault_priority(true);
    TEST_END();
}

bool ai_micro_hs_vector_store_low_half_access_fault_suppresses_later_high_half_trigger_corner()
{
    TEST_START();
    test_status = test_status &&
        ai_micro_cross_page_vector_store_trigger_vs_access_fault_priority(false);
    TEST_END();
}

bool ai_micro_hs_vector_store_same_elem_misalign_and_trigger_tie_breakpoint_corner()
{
    TEST_START();
    test_status = test_status &&
        ai_micro_cross_page_vector_store_trigger_vs_misalign_priority(true);
    TEST_END();
}

bool ai_micro_hs_vector_store_earlier_misalign_suppresses_later_elem_trigger_corner()
{
    TEST_START();
    test_status = test_status &&
        ai_micro_cross_page_vector_store_trigger_vs_misalign_priority(false);
    TEST_END();
}

bool ai_micro_hs_load_cross_page_second_half_address_trigger_ignored_spike_corner()
{
    TEST_START();

    uintptr_t vaddr = ai_micro_cross_page_trigger_addr(VSRWX_GRX, 4U);
    uintptr_t second_half_vaddr = vaddr + 4U;
    uintptr_t low_phys = ai_micro_cross_page_trigger_page_end_phys(VSRWX_GRX);
    uintptr_t high_phys = phys_page_base(VSRWX_GR);
    const uint64_t low_pattern = 0x1122334455667788ULL;
    const uint64_t high_pattern = 0xaabbccdd00112233ULL;

    goto_priv(PRIV_M);
    write64(low_phys, low_pattern);
    write64(high_phys, high_pattern);

    goto_priv(PRIV_HS);
    hspt_init();
    sfence_vma();

    ai_micro_cross_page_clear_mcontrol6_slot(0);
    ai_micro_cross_page_clear_mcontrol6_slot(1);
    ai_micro_cross_page_arm_mcontrol6_slot(0, second_half_vaddr,
        AI_MICRO_CROSS_PAGE_MCONTROL6_LOAD);
    uint64_t saved_mstatus = ai_micro_cross_page_enable_machine_breakpoint_delivery();

    goto_priv(PRIV_HS);
    TEST_SETUP_EXCEPT();
    uint64_t value = ai_micro_cross_page_execute_load_once(vaddr);
    TEST_ASSERT("on Spike, arming a split-load address trigger at the true second-half start address does not match; the cross-page load still completes and returns the merged payload",
        excpt.triggered == false &&
        value == 0x0011223311223344ULL
    );
    ai_micro_cross_page_restore_machine_status(saved_mstatus);

    ai_micro_cross_page_clear_mcontrol6_slot(0);
    TEST_END();
}

bool ai_micro_hs_fetch_after_scalar_cross_page_store_fence_i_execute_trigger_repeated_breakpoint_then_completion_adjacent_preserved_corner()
{
    TEST_START();

    enum test_page low_page = VSRWX_GRWX;
    enum test_page high_page = VSRWX_GRW;
    const uint8_t warm_pattern[8] = {
        0x13, 0x05, 0x30, 0x00,
        0x67, 0x80, 0x00, 0x00
    };
    const uint8_t patched_pattern[8] = {
        0x13, 0x05, 0x70, 0x00,
        0x67, 0x80, 0x00, 0x00
    };
    const uintptr_t vaddr = ai_micro_cross_page_trigger_addr(low_page, 4U);
    const uintptr_t low_phys_start = phys_page_base(low_page) + PAGE_SIZE - 4U;
    const uintptr_t sentinel_before_addr = low_phys_start - 1U;
    const uintptr_t sentinel_after_addr = phys_page_base(high_page) + 4U;
    const uint8_t sentinel_before = 0x5aU;
    const uint8_t sentinel_after = 0xa5U;
    uint64_t (*fetch_func)(void) = (uint64_t (*)(void))vaddr;
    uint64_t observed = 0;
    uint64_t saved_mstatus = 0;

    goto_priv(PRIV_M);
    ai_micro_cross_page_trigger_write_bytes(low_phys_start, warm_pattern, 8U);
    write8(sentinel_before_addr, sentinel_before);
    write8(sentinel_after_addr, sentinel_after);

    goto_priv(PRIV_HS);
    hspt_init();

    goto_priv(PRIV_M);
    ai_micro_cross_page_set_hs_leaf(low_page, phys_page_base(low_page),
        PTE_V | PTE_AD | PTE_RWX);
    ai_micro_cross_page_set_hs_leaf(high_page, phys_page_base(high_page),
        PTE_V | PTE_AD | PTE_RWX);
    sfence_vma();

    goto_priv(PRIV_HS);
    ai_micro_cross_page_fence_i();

    TEST_SETUP_EXCEPT();
    observed = fetch_func();
    TEST_ASSERT("before the scalar cross-page store rewrites the two-page executable HS block, the warmed fetch path still returns the original value in the execute-trigger recovery corner",
        excpt.triggered == false &&
        observed == 3ULL
    );

    TEST_SETUP_EXCEPT();
    sd(vaddr, 0x0000806700700513ULL);
    TEST_ASSERT("a scalar cross-page store can rewrite the two-page executable HS block without fault before the repeated execute-trigger fetch probe",
        excpt.triggered == false
    );

    goto_priv(PRIV_M);
    TEST_ASSERT("after the scalar cross-page store rewrites the executable HS block, the patched instruction bytes are visible in backing memory and the adjacent sentinels stay unchanged before fence.i in the execute-trigger recovery corner",
        ai_micro_cross_page_trigger_bytes_match(low_phys_start, patched_pattern, 8U) &&
        read8(sentinel_before_addr) == sentinel_before &&
        read8(sentinel_after_addr) == sentinel_after
    );

    goto_priv(PRIV_HS);
    ai_micro_cross_page_fence_i();
    ai_micro_cross_page_clear_mcontrol6_slot(0);
    ai_micro_cross_page_clear_mcontrol6_slot(1);
    ai_micro_cross_page_arm_mcontrol6_slot(0, vaddr, AI_MICRO_CROSS_PAGE_MCONTROL6_EXECUTE);
    saved_mstatus = ai_micro_cross_page_enable_machine_breakpoint_delivery();
    goto_priv(PRIV_HS);

    TEST_SETUP_EXCEPT();
    fetch_func();
    TEST_ASSERT("after the scalar cross-page store and fence.i, the first fetch to the rewritten executable HS block still reports breakpoint when the execute-PC trigger is armed",
        excpt.triggered == true &&
        excpt.cause == CAUSE_BKP
    );

    goto_priv(PRIV_M);
    TEST_ASSERT("the first trapped fetch to the scalar-rewritten two-page executable HS block still leaves the patched bytes and adjacent sentinels unchanged",
        ai_micro_cross_page_trigger_bytes_match(low_phys_start, patched_pattern, 8U) &&
        read8(sentinel_before_addr) == sentinel_before &&
        read8(sentinel_after_addr) == sentinel_after
    );
    ai_micro_cross_page_restore_machine_status(saved_mstatus);

    saved_mstatus = ai_micro_cross_page_enable_machine_breakpoint_delivery();
    goto_priv(PRIV_HS);
    TEST_SETUP_EXCEPT();
    fetch_func();
    TEST_ASSERT("after the scalar cross-page store and fence.i, the second fetch to the rewritten executable HS block still reports breakpoint when the execute-PC trigger remains armed",
        excpt.triggered == true &&
        excpt.cause == CAUSE_BKP
    );

    goto_priv(PRIV_M);
    TEST_ASSERT("the second trapped fetch to the scalar-rewritten two-page executable HS block still leaves the patched bytes and adjacent sentinels unchanged",
        ai_micro_cross_page_trigger_bytes_match(low_phys_start, patched_pattern, 8U) &&
        read8(sentinel_before_addr) == sentinel_before &&
        read8(sentinel_after_addr) == sentinel_after
    );
    ai_micro_cross_page_restore_machine_status(saved_mstatus);

    ai_micro_cross_page_clear_mcontrol6_slot(0);

    goto_priv(PRIV_HS);
    ai_micro_cross_page_fence_i();
    TEST_SETUP_EXCEPT();
    observed = fetch_func();
    TEST_ASSERT("after clearing the execute-PC trigger, fetching from the scalar-rewritten two-page executable HS block completes normally and returns the patched value",
        excpt.triggered == false &&
        observed == 7ULL
    );

    goto_priv(PRIV_M);
    TEST_ASSERT("after the scalar cross-page store plus repeated execute-trigger fetch sequence, the patched bytes remain intact and the adjacent sentinels stay unchanged",
        ai_micro_cross_page_trigger_bytes_match(low_phys_start, patched_pattern, 8U) &&
        read8(sentinel_before_addr) == sentinel_before &&
        read8(sentinel_after_addr) == sentinel_after
    );

    goto_priv(PRIV_HS);
    hspt_init();
    sfence_vma();

    TEST_END();
}

bool ai_micro_hs_fetch_after_scalar_cross_page_store_fence_i_execute_trigger_repeated_breakpoint_then_no_x_page_fault_adjacent_preserved_corner()
{
    TEST_START();

    enum test_page low_page = VSRWX_GRWX;
    enum test_page high_page = VSRWX_GRW;
    const uint8_t warm_pattern[8] = {
        0x13, 0x05, 0x30, 0x00,
        0x67, 0x80, 0x00, 0x00
    };
    const uint8_t patched_pattern[8] = {
        0x13, 0x05, 0x70, 0x00,
        0x67, 0x80, 0x00, 0x00
    };
    const uintptr_t vaddr = ai_micro_cross_page_trigger_addr(low_page, 4U);
    const uintptr_t low_phys_start = phys_page_base(low_page) + PAGE_SIZE - 4U;
    const uintptr_t sentinel_before_addr = low_phys_start - 1U;
    const uintptr_t sentinel_after_addr = phys_page_base(high_page) + 4U;
    const uint8_t sentinel_before = 0x5bU;
    const uint8_t sentinel_after = 0xb5U;
    uint64_t (*fetch_func)(void) = (uint64_t (*)(void))vaddr;
    uint64_t observed = 0;
    uint64_t saved_mstatus = 0;

    goto_priv(PRIV_M);
    ai_micro_cross_page_trigger_write_bytes(low_phys_start, warm_pattern, 8U);
    write8(sentinel_before_addr, sentinel_before);
    write8(sentinel_after_addr, sentinel_after);

    goto_priv(PRIV_HS);
    hspt_init();

    goto_priv(PRIV_M);
    ai_micro_cross_page_set_hs_leaf(low_page, phys_page_base(low_page),
        PTE_V | PTE_AD | PTE_RWX);
    ai_micro_cross_page_set_hs_leaf(high_page, phys_page_base(high_page),
        PTE_V | PTE_AD | PTE_RWX);
    sfence_vma();

    goto_priv(PRIV_HS);
    ai_micro_cross_page_fence_i();

    TEST_SETUP_EXCEPT();
    observed = fetch_func();
    TEST_ASSERT("before the scalar cross-page execute-trigger plus no-X probe rewrites the two-page executable HS block, the warmed fetch path still returns the original value",
        excpt.triggered == false &&
        observed == 3ULL
    );

    TEST_SETUP_EXCEPT();
    sd(vaddr, 0x0000806700700513ULL);
    TEST_ASSERT("a scalar cross-page store can rewrite the two-page executable HS block before the repeated execute-trigger plus no-X fetch probe",
        excpt.triggered == false
    );

    goto_priv(PRIV_M);
    TEST_ASSERT("after the scalar cross-page store rewrites the executable HS block, the patched instruction bytes are visible in backing memory and the adjacent sentinels stay unchanged before the repeated execute-trigger plus no-X fetch probe",
        ai_micro_cross_page_trigger_bytes_match(low_phys_start, patched_pattern, 8U) &&
        read8(sentinel_before_addr) == sentinel_before &&
        read8(sentinel_after_addr) == sentinel_after
    );

    goto_priv(PRIV_HS);
    ai_micro_cross_page_fence_i();
    ai_micro_cross_page_clear_mcontrol6_slot(0);
    ai_micro_cross_page_clear_mcontrol6_slot(1);
    ai_micro_cross_page_arm_mcontrol6_slot(0, vaddr, AI_MICRO_CROSS_PAGE_MCONTROL6_EXECUTE);
    saved_mstatus = ai_micro_cross_page_enable_machine_breakpoint_delivery();
    goto_priv(PRIV_HS);

    TEST_SETUP_EXCEPT();
    fetch_func();
    TEST_ASSERT("after the scalar cross-page store and fence.i, the first fetch to the rewritten executable HS block still reports breakpoint before the later no-X fetch page fault is exposed",
        excpt.triggered == true &&
        excpt.cause == CAUSE_BKP
    );

    goto_priv(PRIV_M);
    TEST_ASSERT("the first trapped fetch in the scalar cross-page execute-trigger plus no-X corner still leaves the patched bytes and adjacent sentinels unchanged",
        ai_micro_cross_page_trigger_bytes_match(low_phys_start, patched_pattern, 8U) &&
        read8(sentinel_before_addr) == sentinel_before &&
        read8(sentinel_after_addr) == sentinel_after
    );
    ai_micro_cross_page_restore_machine_status(saved_mstatus);

    saved_mstatus = ai_micro_cross_page_enable_machine_breakpoint_delivery();
    goto_priv(PRIV_HS);
    TEST_SETUP_EXCEPT();
    fetch_func();
    TEST_ASSERT("after the scalar cross-page store and fence.i, the second fetch to the rewritten executable HS block still reports breakpoint while the execute-PC trigger remains armed before the later no-X fetch page fault is exposed",
        excpt.triggered == true &&
        excpt.cause == CAUSE_BKP
    );

    goto_priv(PRIV_M);
    TEST_ASSERT("the second trapped fetch in the scalar cross-page execute-trigger plus no-X corner still leaves the patched bytes and adjacent sentinels unchanged",
        ai_micro_cross_page_trigger_bytes_match(low_phys_start, patched_pattern, 8U) &&
        read8(sentinel_before_addr) == sentinel_before &&
        read8(sentinel_after_addr) == sentinel_after
    );
    ai_micro_cross_page_restore_machine_status(saved_mstatus);
    ai_micro_cross_page_clear_mcontrol6_slot(0);
    ai_micro_cross_page_set_hs_leaf(low_page, phys_page_base(low_page),
        PTE_V | PTE_AD | PTE_RW);
    ai_micro_cross_page_set_hs_leaf(high_page, phys_page_base(high_page),
        PTE_V | PTE_AD | PTE_RW);
    sfence_vma();

    goto_priv(PRIV_HS);
    ai_micro_cross_page_fence_i();
    TEST_SETUP_EXCEPT();
    TEST_EXEC_EXCEPT(vaddr);
    TEST_ASSERT("after clearing the execute-PC trigger, the same scalar-rewritten two-page executable HS block falls back to instruction page fault when execute permission is removed",
        excpt.triggered == true &&
        excpt.cause == CAUSE_IPF &&
        excpt.tval == vaddr
    );

    goto_priv(PRIV_M);
    TEST_ASSERT("after repeated execute-trigger breakpoint hits and the later no-X fetch page fault, the patched bytes remain intact and the adjacent sentinels stay unchanged in the scalar cross-page corner",
        ai_micro_cross_page_trigger_bytes_match(low_phys_start, patched_pattern, 8U) &&
        read8(sentinel_before_addr) == sentinel_before &&
        read8(sentinel_after_addr) == sentinel_after
    );

    goto_priv(PRIV_HS);
    hspt_init();
    sfence_vma();

    TEST_END();
}

bool ai_micro_hs_fetch_after_scalar_cross_page_store_fence_i_execute_trigger_repeated_breakpoint_then_grant_x_executes_adjacent_preserved_corner()
{
    TEST_START();

    enum test_page low_page = VSRWX_GRWX;
    enum test_page high_page = VSRWX_GRW;
    const uint8_t warm_pattern[8] = {
        0x13, 0x05, 0x30, 0x00,
        0x67, 0x80, 0x00, 0x00
    };
    const uint8_t patched_pattern[8] = {
        0x13, 0x05, 0x70, 0x00,
        0x67, 0x80, 0x00, 0x00
    };
    const uintptr_t vaddr = ai_micro_cross_page_trigger_addr(low_page, 4U);
    const uintptr_t low_phys_start = phys_page_base(low_page) + PAGE_SIZE - 4U;
    const uintptr_t sentinel_before_addr = low_phys_start - 1U;
    const uintptr_t sentinel_after_addr = phys_page_base(high_page) + 4U;
    const uint8_t sentinel_before = 0x5cU;
    const uint8_t sentinel_after = 0xc5U;
    uint64_t (*fetch_func)(void) = (uint64_t (*)(void))vaddr;
    uint64_t observed = 0;
    uint64_t saved_mstatus = 0;

    goto_priv(PRIV_M);
    ai_micro_cross_page_trigger_write_bytes(low_phys_start, warm_pattern, 8U);
    write8(sentinel_before_addr, sentinel_before);
    write8(sentinel_after_addr, sentinel_after);

    goto_priv(PRIV_HS);
    hspt_init();

    goto_priv(PRIV_M);
    ai_micro_cross_page_set_hs_leaf(low_page, phys_page_base(low_page),
        PTE_V | PTE_AD | PTE_RWX);
    ai_micro_cross_page_set_hs_leaf(high_page, phys_page_base(high_page),
        PTE_V | PTE_AD | PTE_RWX);
    sfence_vma();

    goto_priv(PRIV_HS);
    ai_micro_cross_page_fence_i();

    TEST_SETUP_EXCEPT();
    observed = fetch_func();
    TEST_ASSERT("before the scalar cross-page execute-trigger plus grant-X-recovery probe rewrites the two-page executable HS block, the warmed fetch path still returns the original value",
        excpt.triggered == false &&
        observed == 3ULL
    );

    TEST_SETUP_EXCEPT();
    sd(vaddr, 0x0000806700700513ULL);
    TEST_ASSERT("a scalar cross-page store can rewrite the two-page executable HS block before the repeated execute-trigger plus grant-X-recovery fetch probe",
        excpt.triggered == false
    );

    goto_priv(PRIV_M);
    TEST_ASSERT("after the scalar cross-page store rewrites the executable HS block, the patched instruction bytes are visible in backing memory and the adjacent sentinels stay unchanged before the repeated execute-trigger plus grant-X-recovery fetch probe",
        ai_micro_cross_page_trigger_bytes_match(low_phys_start, patched_pattern, 8U) &&
        read8(sentinel_before_addr) == sentinel_before &&
        read8(sentinel_after_addr) == sentinel_after
    );

    goto_priv(PRIV_HS);
    ai_micro_cross_page_fence_i();
    ai_micro_cross_page_clear_mcontrol6_slot(0);
    ai_micro_cross_page_clear_mcontrol6_slot(1);
    ai_micro_cross_page_arm_mcontrol6_slot(0, vaddr, AI_MICRO_CROSS_PAGE_MCONTROL6_EXECUTE);
    saved_mstatus = ai_micro_cross_page_enable_machine_breakpoint_delivery();
    goto_priv(PRIV_HS);

    TEST_SETUP_EXCEPT();
    fetch_func();
    TEST_ASSERT("after the scalar cross-page store and fence.i, the first fetch to the rewritten executable HS block still reports breakpoint before the later grant-X-recovery sequence begins",
        excpt.triggered == true &&
        excpt.cause == CAUSE_BKP
    );

    goto_priv(PRIV_M);
    TEST_ASSERT("the first trapped fetch in the scalar cross-page execute-trigger plus grant-X-recovery corner still leaves the patched bytes and adjacent sentinels unchanged",
        ai_micro_cross_page_trigger_bytes_match(low_phys_start, patched_pattern, 8U) &&
        read8(sentinel_before_addr) == sentinel_before &&
        read8(sentinel_after_addr) == sentinel_after
    );
    ai_micro_cross_page_restore_machine_status(saved_mstatus);

    saved_mstatus = ai_micro_cross_page_enable_machine_breakpoint_delivery();
    goto_priv(PRIV_HS);
    TEST_SETUP_EXCEPT();
    fetch_func();
    TEST_ASSERT("after the scalar cross-page store and fence.i, the second fetch to the rewritten executable HS block still reports breakpoint while the execute-PC trigger remains armed before the later grant-X-recovery sequence begins",
        excpt.triggered == true &&
        excpt.cause == CAUSE_BKP
    );

    goto_priv(PRIV_M);
    TEST_ASSERT("the second trapped fetch in the scalar cross-page execute-trigger plus grant-X-recovery corner still leaves the patched bytes and adjacent sentinels unchanged",
        ai_micro_cross_page_trigger_bytes_match(low_phys_start, patched_pattern, 8U) &&
        read8(sentinel_before_addr) == sentinel_before &&
        read8(sentinel_after_addr) == sentinel_after
    );
    ai_micro_cross_page_restore_machine_status(saved_mstatus);
    ai_micro_cross_page_clear_mcontrol6_slot(0);
    ai_micro_cross_page_set_hs_leaf(low_page, phys_page_base(low_page),
        PTE_V | PTE_AD | PTE_RW);
    ai_micro_cross_page_set_hs_leaf(high_page, phys_page_base(high_page),
        PTE_V | PTE_AD | PTE_RW);
    sfence_vma();

    goto_priv(PRIV_HS);
    ai_micro_cross_page_fence_i();
    TEST_SETUP_EXCEPT();
    TEST_EXEC_EXCEPT(vaddr);
    TEST_ASSERT("after clearing the execute-PC trigger, the same scalar-rewritten two-page executable HS block first falls back to instruction page fault when execute permission is removed",
        excpt.triggered == true &&
        excpt.cause == CAUSE_IPF &&
        excpt.tval == vaddr
    );

    goto_priv(PRIV_M);
    TEST_ASSERT("the no-X fetch page fault before execute recovery still leaves the patched bytes and adjacent sentinels unchanged in the scalar cross-page execute-trigger corner",
        ai_micro_cross_page_trigger_bytes_match(low_phys_start, patched_pattern, 8U) &&
        read8(sentinel_before_addr) == sentinel_before &&
        read8(sentinel_after_addr) == sentinel_after
    );
    ai_micro_cross_page_set_hs_leaf(low_page, phys_page_base(low_page),
        PTE_V | PTE_AD | PTE_RWX);
    ai_micro_cross_page_set_hs_leaf(high_page, phys_page_base(high_page),
        PTE_V | PTE_AD | PTE_RWX);
    sfence_vma();

    goto_priv(PRIV_HS);
    ai_micro_cross_page_fence_i();
    TEST_SETUP_EXCEPT();
    observed = fetch_func();
    TEST_ASSERT("after restoring execute permission, the scalar-rewritten two-page executable HS block completes normally again and returns the patched value",
        excpt.triggered == false &&
        observed == 7ULL
    );

    goto_priv(PRIV_M);
    TEST_ASSERT("after repeated execute-trigger breakpoint hits, the later no-X fetch page fault, and final execute-permission repair, the patched bytes remain intact and the adjacent sentinels stay unchanged in the scalar cross-page corner",
        ai_micro_cross_page_trigger_bytes_match(low_phys_start, patched_pattern, 8U) &&
        read8(sentinel_before_addr) == sentinel_before &&
        read8(sentinel_after_addr) == sentinel_after
    );

    goto_priv(PRIV_HS);
    hspt_init();
    sfence_vma();

    TEST_END();
}

bool ai_micro_hs_fetch_after_scalar_cross_page_store_fence_i_execute_trigger_repeated_breakpoint_then_noncanonical_fetch_page_fault_adjacent_preserved_corner()
{
    TEST_START();
    test_status = test_status && ai_micro_cross_page_scalar_store_repeated_breakpoint_followup(
        AI_MICRO_CROSS_PAGE_FOLLOWUP_NONCANONICAL);
    TEST_END();
}

bool ai_micro_hs_fetch_after_scalar_cross_page_store_fence_i_execute_trigger_repeated_breakpoint_then_all_ones_fetch_page_fault_adjacent_preserved_corner()
{
    TEST_START();
    test_status = test_status && ai_micro_cross_page_scalar_store_repeated_breakpoint_followup(
        AI_MICRO_CROSS_PAGE_FOLLOWUP_ALL_ONES);
    TEST_END();
}

bool ai_micro_hs_fetch_after_scalar_cross_page_store_fence_i_execute_trigger_repeated_breakpoint_then_leaf_overflow_fetch_access_fault_adjacent_preserved_corner()
{
    TEST_START();
    test_status = test_status && ai_micro_cross_page_scalar_store_repeated_breakpoint_followup(
        AI_MICRO_CROSS_PAGE_FOLLOWUP_LEAF_OVERFLOW_FAULT);
    TEST_END();
}

bool ai_micro_hs_fetch_after_scalar_cross_page_store_fence_i_execute_trigger_repeated_breakpoint_then_leaf_overflow_fix_leaf_executes_adjacent_preserved_corner()
{
    TEST_START();
    test_status = test_status && ai_micro_cross_page_scalar_store_repeated_breakpoint_followup(
        AI_MICRO_CROSS_PAGE_FOLLOWUP_LEAF_OVERFLOW_FIX);
    TEST_END();
}

bool ai_micro_hs_fetch_after_vector_cross_page_store_fence_i_execute_trigger_repeated_breakpoint_then_completion_adjacent_preserved_corner()
{
    TEST_START();

    enum test_page low_page = VSRWX_GRWX;
    enum test_page high_page = VSRWX_GRW;
    const uint8_t warm_pattern[8] = {
        0x13, 0x05, 0x30, 0x00,
        0x67, 0x80, 0x00, 0x00
    };
    const uint8_t patched_pattern[8] = {
        0x13, 0x05, 0x70, 0x00,
        0x67, 0x80, 0x00, 0x00
    };
    const uintptr_t source_vaddr = hs_page_base(VSRW_GRWX) + 0x1c0U;
    const uintptr_t source_phys = phys_page_base(VSRW_GRWX) + 0x1c0U;
    const uintptr_t vaddr = ai_micro_cross_page_trigger_addr(low_page, 4U);
    const uintptr_t low_phys_start = phys_page_base(low_page) + PAGE_SIZE - 4U;
    const uintptr_t sentinel_before_addr = low_phys_start - 1U;
    const uintptr_t sentinel_after_addr = phys_page_base(high_page) + 4U;
    const uint8_t sentinel_before = 0x3cU;
    const uint8_t sentinel_after = 0xc3U;
    uint64_t (*fetch_func)(void) = (uint64_t (*)(void))vaddr;
    uint64_t observed = 0;
    uint64_t saved_mstatus = 0;

    goto_priv(PRIV_M);
    ai_micro_cross_page_trigger_write_bytes(low_phys_start, warm_pattern, 8U);
    ai_micro_cross_page_trigger_write_bytes(source_phys, patched_pattern, 8U);
    write8(sentinel_before_addr, sentinel_before);
    write8(sentinel_after_addr, sentinel_after);

    goto_priv(PRIV_HS);
    hspt_init();

    goto_priv(PRIV_M);
    ai_micro_cross_page_set_hs_leaf(low_page, phys_page_base(low_page),
        PTE_V | PTE_AD | PTE_RWX);
    ai_micro_cross_page_set_hs_leaf(high_page, phys_page_base(high_page),
        PTE_V | PTE_AD | PTE_RWX);
    sfence_vma();

    goto_priv(PRIV_HS);
    ai_micro_cross_page_fence_i();

    TEST_SETUP_EXCEPT();
    observed = fetch_func();
    TEST_ASSERT("before the vector cross-page store rewrites the two-page executable HS block, the warmed fetch path still returns the original value in the execute-trigger recovery corner",
        excpt.triggered == false &&
        observed == 3ULL
    );

    TEST_SETUP_EXCEPT();
    vle8_to_v6((const uint8_t *)source_vaddr, 8);
    TEST_ASSERT("a vector unit-stride load can stage the replacement instruction bytes before the vector cross-page store rewrites the executable HS block in the execute-trigger recovery corner",
        excpt.triggered == false
    );

    TEST_SETUP_EXCEPT();
    vse8_from_v6((uint8_t *)vaddr, 8);
    TEST_ASSERT("a vector unit-stride store can rewrite the two-page executable HS block across the page boundary before the repeated execute-trigger fetch probe",
        excpt.triggered == false
    );

    goto_priv(PRIV_M);
    TEST_ASSERT("after the vector cross-page store rewrites the executable HS block, the patched instruction bytes are visible in backing memory and the adjacent sentinels stay unchanged before fence.i in the execute-trigger recovery corner",
        ai_micro_cross_page_trigger_bytes_match(low_phys_start, patched_pattern, 8U) &&
        read8(sentinel_before_addr) == sentinel_before &&
        read8(sentinel_after_addr) == sentinel_after
    );

    goto_priv(PRIV_HS);
    ai_micro_cross_page_fence_i();
    ai_micro_cross_page_clear_mcontrol6_slot(0);
    ai_micro_cross_page_clear_mcontrol6_slot(1);
    ai_micro_cross_page_arm_mcontrol6_slot(0, vaddr, AI_MICRO_CROSS_PAGE_MCONTROL6_EXECUTE);
    saved_mstatus = ai_micro_cross_page_enable_machine_breakpoint_delivery();
    goto_priv(PRIV_HS);

    TEST_SETUP_EXCEPT();
    fetch_func();
    TEST_ASSERT("after the vector cross-page store and fence.i, the first fetch to the rewritten executable HS block still reports breakpoint when the execute-PC trigger is armed",
        excpt.triggered == true &&
        excpt.cause == CAUSE_BKP
    );

    goto_priv(PRIV_M);
    TEST_ASSERT("the first trapped fetch to the vector-rewritten two-page executable HS block still leaves the patched bytes and adjacent sentinels unchanged",
        ai_micro_cross_page_trigger_bytes_match(low_phys_start, patched_pattern, 8U) &&
        read8(sentinel_before_addr) == sentinel_before &&
        read8(sentinel_after_addr) == sentinel_after
    );
    ai_micro_cross_page_restore_machine_status(saved_mstatus);

    saved_mstatus = ai_micro_cross_page_enable_machine_breakpoint_delivery();
    goto_priv(PRIV_HS);
    TEST_SETUP_EXCEPT();
    fetch_func();
    TEST_ASSERT("after the vector cross-page store and fence.i, the second fetch to the rewritten executable HS block still reports breakpoint when the execute-PC trigger remains armed",
        excpt.triggered == true &&
        excpt.cause == CAUSE_BKP
    );

    goto_priv(PRIV_M);
    TEST_ASSERT("the second trapped fetch to the vector-rewritten two-page executable HS block still leaves the patched bytes and adjacent sentinels unchanged",
        ai_micro_cross_page_trigger_bytes_match(low_phys_start, patched_pattern, 8U) &&
        read8(sentinel_before_addr) == sentinel_before &&
        read8(sentinel_after_addr) == sentinel_after
    );
    ai_micro_cross_page_restore_machine_status(saved_mstatus);

    ai_micro_cross_page_clear_mcontrol6_slot(0);

    goto_priv(PRIV_HS);
    ai_micro_cross_page_fence_i();
    TEST_SETUP_EXCEPT();
    observed = fetch_func();
    TEST_ASSERT("after clearing the execute-PC trigger, fetching from the vector-rewritten two-page executable HS block completes normally and returns the patched value",
        excpt.triggered == false &&
        observed == 7ULL
    );

    goto_priv(PRIV_M);
    TEST_ASSERT("after the vector cross-page store plus repeated execute-trigger fetch sequence, the patched bytes remain intact and the adjacent sentinels stay unchanged",
        ai_micro_cross_page_trigger_bytes_match(low_phys_start, patched_pattern, 8U) &&
        read8(sentinel_before_addr) == sentinel_before &&
        read8(sentinel_after_addr) == sentinel_after
    );

    goto_priv(PRIV_HS);
    hspt_init();
    sfence_vma();

    TEST_END();
}

bool ai_micro_hs_fetch_after_vector_cross_page_store_fence_i_execute_trigger_repeated_breakpoint_then_no_x_page_fault_adjacent_preserved_corner()
{
    TEST_START();

    enum test_page low_page = VSRWX_GRWX;
    enum test_page high_page = VSRWX_GRW;
    const uint8_t warm_pattern[8] = {
        0x13, 0x05, 0x30, 0x00,
        0x67, 0x80, 0x00, 0x00
    };
    const uint8_t patched_pattern[8] = {
        0x13, 0x05, 0x70, 0x00,
        0x67, 0x80, 0x00, 0x00
    };
    const uintptr_t source_vaddr = hs_page_base(VSRW_GRWX) + 0x1d0U;
    const uintptr_t source_phys = phys_page_base(VSRW_GRWX) + 0x1d0U;
    const uintptr_t vaddr = ai_micro_cross_page_trigger_addr(low_page, 4U);
    const uintptr_t low_phys_start = phys_page_base(low_page) + PAGE_SIZE - 4U;
    const uintptr_t sentinel_before_addr = low_phys_start - 1U;
    const uintptr_t sentinel_after_addr = phys_page_base(high_page) + 4U;
    const uint8_t sentinel_before = 0x3dU;
    const uint8_t sentinel_after = 0xd3U;
    uint64_t (*fetch_func)(void) = (uint64_t (*)(void))vaddr;
    uint64_t observed = 0;
    uint64_t saved_mstatus = 0;

    goto_priv(PRIV_M);
    ai_micro_cross_page_trigger_write_bytes(low_phys_start, warm_pattern, 8U);
    ai_micro_cross_page_trigger_write_bytes(source_phys, patched_pattern, 8U);
    write8(sentinel_before_addr, sentinel_before);
    write8(sentinel_after_addr, sentinel_after);

    goto_priv(PRIV_HS);
    hspt_init();

    goto_priv(PRIV_M);
    ai_micro_cross_page_set_hs_leaf(low_page, phys_page_base(low_page),
        PTE_V | PTE_AD | PTE_RWX);
    ai_micro_cross_page_set_hs_leaf(high_page, phys_page_base(high_page),
        PTE_V | PTE_AD | PTE_RWX);
    sfence_vma();

    goto_priv(PRIV_HS);
    ai_micro_cross_page_fence_i();

    TEST_SETUP_EXCEPT();
    observed = fetch_func();
    TEST_ASSERT("before the vector cross-page execute-trigger plus no-X probe rewrites the two-page executable HS block, the warmed fetch path still returns the original value",
        excpt.triggered == false &&
        observed == 3ULL
    );

    TEST_SETUP_EXCEPT();
    vle8_to_v6((const uint8_t *)source_vaddr, 8);
    TEST_ASSERT("a vector unit-stride load can stage the replacement instruction bytes before the vector cross-page execute-trigger plus no-X fetch probe",
        excpt.triggered == false
    );

    TEST_SETUP_EXCEPT();
    vse8_from_v6((uint8_t *)vaddr, 8);
    TEST_ASSERT("a vector unit-stride store can rewrite the two-page executable HS block across the page boundary before the repeated execute-trigger plus no-X fetch probe",
        excpt.triggered == false
    );

    goto_priv(PRIV_M);
    TEST_ASSERT("after the vector cross-page store rewrites the executable HS block, the patched instruction bytes are visible in backing memory and the adjacent sentinels stay unchanged before the repeated execute-trigger plus no-X fetch probe",
        ai_micro_cross_page_trigger_bytes_match(low_phys_start, patched_pattern, 8U) &&
        read8(sentinel_before_addr) == sentinel_before &&
        read8(sentinel_after_addr) == sentinel_after
    );

    goto_priv(PRIV_HS);
    ai_micro_cross_page_fence_i();
    ai_micro_cross_page_clear_mcontrol6_slot(0);
    ai_micro_cross_page_clear_mcontrol6_slot(1);
    ai_micro_cross_page_arm_mcontrol6_slot(0, vaddr, AI_MICRO_CROSS_PAGE_MCONTROL6_EXECUTE);
    saved_mstatus = ai_micro_cross_page_enable_machine_breakpoint_delivery();
    goto_priv(PRIV_HS);

    TEST_SETUP_EXCEPT();
    fetch_func();
    TEST_ASSERT("after the vector cross-page store and fence.i, the first fetch to the rewritten executable HS block still reports breakpoint before the later no-X fetch page fault is exposed",
        excpt.triggered == true &&
        excpt.cause == CAUSE_BKP
    );

    goto_priv(PRIV_M);
    TEST_ASSERT("the first trapped fetch in the vector cross-page execute-trigger plus no-X corner still leaves the patched bytes and adjacent sentinels unchanged",
        ai_micro_cross_page_trigger_bytes_match(low_phys_start, patched_pattern, 8U) &&
        read8(sentinel_before_addr) == sentinel_before &&
        read8(sentinel_after_addr) == sentinel_after
    );
    ai_micro_cross_page_restore_machine_status(saved_mstatus);

    saved_mstatus = ai_micro_cross_page_enable_machine_breakpoint_delivery();
    goto_priv(PRIV_HS);
    TEST_SETUP_EXCEPT();
    fetch_func();
    TEST_ASSERT("after the vector cross-page store and fence.i, the second fetch to the rewritten executable HS block still reports breakpoint while the execute-PC trigger remains armed before the later no-X fetch page fault is exposed",
        excpt.triggered == true &&
        excpt.cause == CAUSE_BKP
    );

    goto_priv(PRIV_M);
    TEST_ASSERT("the second trapped fetch in the vector cross-page execute-trigger plus no-X corner still leaves the patched bytes and adjacent sentinels unchanged",
        ai_micro_cross_page_trigger_bytes_match(low_phys_start, patched_pattern, 8U) &&
        read8(sentinel_before_addr) == sentinel_before &&
        read8(sentinel_after_addr) == sentinel_after
    );
    ai_micro_cross_page_restore_machine_status(saved_mstatus);
    ai_micro_cross_page_clear_mcontrol6_slot(0);
    ai_micro_cross_page_set_hs_leaf(low_page, phys_page_base(low_page),
        PTE_V | PTE_AD | PTE_RW);
    ai_micro_cross_page_set_hs_leaf(high_page, phys_page_base(high_page),
        PTE_V | PTE_AD | PTE_RW);
    sfence_vma();

    goto_priv(PRIV_HS);
    ai_micro_cross_page_fence_i();
    TEST_SETUP_EXCEPT();
    TEST_EXEC_EXCEPT(vaddr);
    TEST_ASSERT("after clearing the execute-PC trigger, the same vector-rewritten two-page executable HS block falls back to instruction page fault when execute permission is removed",
        excpt.triggered == true &&
        excpt.cause == CAUSE_IPF &&
        excpt.tval == vaddr
    );

    goto_priv(PRIV_M);
    TEST_ASSERT("after repeated execute-trigger breakpoint hits and the later no-X fetch page fault, the patched bytes remain intact and the adjacent sentinels stay unchanged in the vector cross-page corner",
        ai_micro_cross_page_trigger_bytes_match(low_phys_start, patched_pattern, 8U) &&
        read8(sentinel_before_addr) == sentinel_before &&
        read8(sentinel_after_addr) == sentinel_after
    );

    goto_priv(PRIV_HS);
    hspt_init();
    sfence_vma();

    TEST_END();
}

bool ai_micro_hs_fetch_after_vector_cross_page_store_fence_i_execute_trigger_repeated_breakpoint_then_grant_x_executes_adjacent_preserved_corner()
{
    TEST_START();

    enum test_page low_page = VSRWX_GRWX;
    enum test_page high_page = VSRWX_GRW;
    const uint8_t warm_pattern[8] = {
        0x13, 0x05, 0x30, 0x00,
        0x67, 0x80, 0x00, 0x00
    };
    const uint8_t patched_pattern[8] = {
        0x13, 0x05, 0x70, 0x00,
        0x67, 0x80, 0x00, 0x00
    };
    const uintptr_t source_vaddr = hs_page_base(VSRW_GRWX) + 0x1e0U;
    const uintptr_t source_phys = phys_page_base(VSRW_GRWX) + 0x1e0U;
    const uintptr_t vaddr = ai_micro_cross_page_trigger_addr(low_page, 4U);
    const uintptr_t low_phys_start = phys_page_base(low_page) + PAGE_SIZE - 4U;
    const uintptr_t sentinel_before_addr = low_phys_start - 1U;
    const uintptr_t sentinel_after_addr = phys_page_base(high_page) + 4U;
    const uint8_t sentinel_before = 0x3eU;
    const uint8_t sentinel_after = 0xe3U;
    uint64_t (*fetch_func)(void) = (uint64_t (*)(void))vaddr;
    uint64_t observed = 0;
    uint64_t saved_mstatus = 0;

    goto_priv(PRIV_M);
    ai_micro_cross_page_trigger_write_bytes(low_phys_start, warm_pattern, 8U);
    ai_micro_cross_page_trigger_write_bytes(source_phys, patched_pattern, 8U);
    write8(sentinel_before_addr, sentinel_before);
    write8(sentinel_after_addr, sentinel_after);

    goto_priv(PRIV_HS);
    hspt_init();

    goto_priv(PRIV_M);
    ai_micro_cross_page_set_hs_leaf(low_page, phys_page_base(low_page),
        PTE_V | PTE_AD | PTE_RWX);
    ai_micro_cross_page_set_hs_leaf(high_page, phys_page_base(high_page),
        PTE_V | PTE_AD | PTE_RWX);
    sfence_vma();

    goto_priv(PRIV_HS);
    ai_micro_cross_page_fence_i();

    TEST_SETUP_EXCEPT();
    observed = fetch_func();
    TEST_ASSERT("before the vector cross-page execute-trigger plus grant-X-recovery probe rewrites the two-page executable HS block, the warmed fetch path still returns the original value",
        excpt.triggered == false &&
        observed == 3ULL
    );

    TEST_SETUP_EXCEPT();
    vle8_to_v6((const uint8_t *)source_vaddr, 8);
    TEST_ASSERT("a vector unit-stride load can stage the replacement instruction bytes before the vector cross-page execute-trigger plus grant-X-recovery fetch probe",
        excpt.triggered == false
    );

    TEST_SETUP_EXCEPT();
    vse8_from_v6((uint8_t *)vaddr, 8);
    TEST_ASSERT("a vector unit-stride store can rewrite the two-page executable HS block across the page boundary before the repeated execute-trigger plus grant-X-recovery fetch probe",
        excpt.triggered == false
    );

    goto_priv(PRIV_M);
    TEST_ASSERT("after the vector cross-page store rewrites the executable HS block, the patched instruction bytes are visible in backing memory and the adjacent sentinels stay unchanged before the repeated execute-trigger plus grant-X-recovery fetch probe",
        ai_micro_cross_page_trigger_bytes_match(low_phys_start, patched_pattern, 8U) &&
        read8(sentinel_before_addr) == sentinel_before &&
        read8(sentinel_after_addr) == sentinel_after
    );

    goto_priv(PRIV_HS);
    ai_micro_cross_page_fence_i();
    ai_micro_cross_page_clear_mcontrol6_slot(0);
    ai_micro_cross_page_clear_mcontrol6_slot(1);
    ai_micro_cross_page_arm_mcontrol6_slot(0, vaddr, AI_MICRO_CROSS_PAGE_MCONTROL6_EXECUTE);
    saved_mstatus = ai_micro_cross_page_enable_machine_breakpoint_delivery();
    goto_priv(PRIV_HS);

    TEST_SETUP_EXCEPT();
    fetch_func();
    TEST_ASSERT("after the vector cross-page store and fence.i, the first fetch to the rewritten executable HS block still reports breakpoint before the later grant-X-recovery sequence begins",
        excpt.triggered == true &&
        excpt.cause == CAUSE_BKP
    );

    goto_priv(PRIV_M);
    TEST_ASSERT("the first trapped fetch in the vector cross-page execute-trigger plus grant-X-recovery corner still leaves the patched bytes and adjacent sentinels unchanged",
        ai_micro_cross_page_trigger_bytes_match(low_phys_start, patched_pattern, 8U) &&
        read8(sentinel_before_addr) == sentinel_before &&
        read8(sentinel_after_addr) == sentinel_after
    );
    ai_micro_cross_page_restore_machine_status(saved_mstatus);

    saved_mstatus = ai_micro_cross_page_enable_machine_breakpoint_delivery();
    goto_priv(PRIV_HS);
    TEST_SETUP_EXCEPT();
    fetch_func();
    TEST_ASSERT("after the vector cross-page store and fence.i, the second fetch to the rewritten executable HS block still reports breakpoint while the execute-PC trigger remains armed before the later grant-X-recovery sequence begins",
        excpt.triggered == true &&
        excpt.cause == CAUSE_BKP
    );

    goto_priv(PRIV_M);
    TEST_ASSERT("the second trapped fetch in the vector cross-page execute-trigger plus grant-X-recovery corner still leaves the patched bytes and adjacent sentinels unchanged",
        ai_micro_cross_page_trigger_bytes_match(low_phys_start, patched_pattern, 8U) &&
        read8(sentinel_before_addr) == sentinel_before &&
        read8(sentinel_after_addr) == sentinel_after
    );
    ai_micro_cross_page_restore_machine_status(saved_mstatus);
    ai_micro_cross_page_clear_mcontrol6_slot(0);
    ai_micro_cross_page_set_hs_leaf(low_page, phys_page_base(low_page),
        PTE_V | PTE_AD | PTE_RW);
    ai_micro_cross_page_set_hs_leaf(high_page, phys_page_base(high_page),
        PTE_V | PTE_AD | PTE_RW);
    sfence_vma();

    goto_priv(PRIV_HS);
    ai_micro_cross_page_fence_i();
    TEST_SETUP_EXCEPT();
    TEST_EXEC_EXCEPT(vaddr);
    TEST_ASSERT("after clearing the execute-PC trigger, the same vector-rewritten two-page executable HS block first falls back to instruction page fault when execute permission is removed",
        excpt.triggered == true &&
        excpt.cause == CAUSE_IPF &&
        excpt.tval == vaddr
    );

    goto_priv(PRIV_M);
    TEST_ASSERT("the no-X fetch page fault before execute recovery still leaves the patched bytes and adjacent sentinels unchanged in the vector cross-page execute-trigger corner",
        ai_micro_cross_page_trigger_bytes_match(low_phys_start, patched_pattern, 8U) &&
        read8(sentinel_before_addr) == sentinel_before &&
        read8(sentinel_after_addr) == sentinel_after
    );
    ai_micro_cross_page_set_hs_leaf(low_page, phys_page_base(low_page),
        PTE_V | PTE_AD | PTE_RWX);
    ai_micro_cross_page_set_hs_leaf(high_page, phys_page_base(high_page),
        PTE_V | PTE_AD | PTE_RWX);
    sfence_vma();

    goto_priv(PRIV_HS);
    ai_micro_cross_page_fence_i();
    TEST_SETUP_EXCEPT();
    observed = fetch_func();
    TEST_ASSERT("after restoring execute permission, the vector-rewritten two-page executable HS block completes normally again and returns the patched value",
        excpt.triggered == false &&
        observed == 7ULL
    );

    goto_priv(PRIV_M);
    TEST_ASSERT("after repeated execute-trigger breakpoint hits, the later no-X fetch page fault, and final execute-permission repair, the patched bytes remain intact and the adjacent sentinels stay unchanged in the vector cross-page corner",
        ai_micro_cross_page_trigger_bytes_match(low_phys_start, patched_pattern, 8U) &&
        read8(sentinel_before_addr) == sentinel_before &&
        read8(sentinel_after_addr) == sentinel_after
    );

    goto_priv(PRIV_HS);
    hspt_init();
    sfence_vma();

    TEST_END();
}

bool ai_micro_hs_fetch_after_vector_cross_page_store_fence_i_execute_trigger_repeated_breakpoint_then_noncanonical_fetch_page_fault_adjacent_preserved_corner()
{
    TEST_START();
    test_status = test_status && ai_micro_cross_page_vector_store_repeated_breakpoint_followup(
        AI_MICRO_CROSS_PAGE_FOLLOWUP_NONCANONICAL);
    TEST_END();
}

bool ai_micro_hs_fetch_after_vector_cross_page_store_fence_i_execute_trigger_repeated_breakpoint_then_all_ones_fetch_page_fault_adjacent_preserved_corner()
{
    TEST_START();
    test_status = test_status && ai_micro_cross_page_vector_store_repeated_breakpoint_followup(
        AI_MICRO_CROSS_PAGE_FOLLOWUP_ALL_ONES);
    TEST_END();
}

bool ai_micro_hs_fetch_after_vector_cross_page_store_fence_i_execute_trigger_repeated_breakpoint_then_leaf_overflow_fetch_access_fault_adjacent_preserved_corner()
{
    TEST_START();
    test_status = test_status && ai_micro_cross_page_vector_store_repeated_breakpoint_followup(
        AI_MICRO_CROSS_PAGE_FOLLOWUP_LEAF_OVERFLOW_FAULT);
    TEST_END();
}

bool ai_micro_hs_fetch_after_vector_cross_page_store_fence_i_execute_trigger_repeated_breakpoint_then_leaf_overflow_fix_leaf_executes_adjacent_preserved_corner()
{
    TEST_START();
    test_status = test_status && ai_micro_cross_page_vector_store_repeated_breakpoint_followup(
        AI_MICRO_CROSS_PAGE_FOLLOWUP_LEAF_OVERFLOW_FIX);
    TEST_END();
}

bool ai_micro_hs_fetch_after_scalar_cross_page_store_after_vector_cross_page_store_fence_i_execute_trigger_repeated_breakpoint_then_completion_adjacent_preserved_corner()
{
    TEST_START();

    enum test_page low_page = VSRWX_GRWX;
    enum test_page high_page = VSRWX_GRW;
    const uint8_t warm_pattern[8] = {
        0x13, 0x05, 0x30, 0x00,
        0x67, 0x80, 0x00, 0x00
    };
    const uint8_t vector_pattern[16] = {
        0x30, 0x31, 0x32, 0x33,
        0x13, 0x05, 0x50, 0x00,
        0x67, 0x80, 0x00, 0x00,
        0x3c, 0x3d, 0x3e, 0x3f
    };
    const uint8_t expected_final[16] = {
        0x30, 0x31, 0x32, 0x33,
        0x13, 0x05, 0x90, 0x00,
        0x67, 0x80, 0x00, 0x00,
        0x3c, 0x3d, 0x3e, 0x3f
    };
    const uintptr_t source_vaddr = hs_page_base(VSRWX_GRWX) + 0x240U;
    const uintptr_t source_phys = phys_page_base(VSRWX_GRWX) + 0x240U;
    const uintptr_t boundary_vaddr = ai_micro_cross_page_trigger_addr(low_page, 8U);
    const uintptr_t code_vaddr = ai_micro_cross_page_trigger_addr(low_page, 4U);
    const uintptr_t low_phys_start = phys_page_base(low_page) + PAGE_SIZE - 8U;
    const uintptr_t code_phys_start = phys_page_base(low_page) + PAGE_SIZE - 4U;
    const uintptr_t high_phys_start = phys_page_base(high_page);
    const uintptr_t sentinel_before_addr = low_phys_start - 1U;
    const uintptr_t sentinel_after_addr = high_phys_start + 8U;
    const uint8_t sentinel_before = 0x6dU;
    const uint8_t sentinel_after = 0xd6U;
    uint64_t (*fetch_func)(void) = (uint64_t (*)(void))code_vaddr;
    uint64_t observed = 0;
    uint64_t saved_mstatus = 0;

    goto_priv(PRIV_M);
    ai_micro_cross_page_trigger_write_bytes(code_phys_start, warm_pattern, 8U);
    ai_micro_cross_page_trigger_write_bytes(source_phys, vector_pattern, 16U);
    write8(sentinel_before_addr, sentinel_before);
    write8(sentinel_after_addr, sentinel_after);

    goto_priv(PRIV_HS);
    hspt_init();

    goto_priv(PRIV_M);
    ai_micro_cross_page_set_hs_leaf(low_page, phys_page_base(low_page),
        PTE_V | PTE_AD | PTE_RWX);
    ai_micro_cross_page_set_hs_leaf(high_page, phys_page_base(high_page),
        PTE_V | PTE_AD | PTE_RWX);
    sfence_vma();

    goto_priv(PRIV_HS);
    ai_micro_cross_page_fence_i();

    TEST_SETUP_EXCEPT();
    observed = fetch_func();
    TEST_ASSERT("before the mixed vector-then-scalar cross-page code rewrite execute-trigger probe starts, the warmed fetch path still returns the original value",
        excpt.triggered == false &&
        observed == 3ULL
    );

    TEST_SETUP_EXCEPT();
    vle8_to_v6((const uint8_t *)source_vaddr, 16);
    TEST_ASSERT("a vector unit-stride load can stage the first cross-page instruction payload before the mixed execute-trigger code-rewrite probe",
        excpt.triggered == false
    );

    TEST_SETUP_EXCEPT();
    vse8_from_v6((uint8_t *)boundary_vaddr, 16);
    TEST_ASSERT("a vector unit-stride store can rewrite the shared two-page executable HS boundary before the overlapping scalar cross-page execute-trigger code rewrite",
        excpt.triggered == false
    );

    goto_priv(PRIV_M);
    TEST_ASSERT("after the first vector cross-page execute-trigger code rewrite step, the staged boundary bytes are visible across both backing pages and the outer sentinels stay unchanged before the scalar overwrite",
        ai_micro_cross_page_trigger_bytes_match(low_phys_start, vector_pattern, 8U) &&
        ai_micro_cross_page_trigger_bytes_match(high_phys_start, vector_pattern + 8U, 8U) &&
        read8(sentinel_before_addr) == sentinel_before &&
        read8(sentinel_after_addr) == sentinel_after
    );

    goto_priv(PRIV_HS);
    TEST_SETUP_EXCEPT();
    sd(code_vaddr, 0x0000806700900513ULL);
    TEST_ASSERT("after the vector rewrite, an overlapping scalar cross-page store can overwrite the executable HS block to the final instruction stream before the execute-trigger fetch probe",
        excpt.triggered == false
    );

    goto_priv(PRIV_M);
    TEST_ASSERT("after the mixed vector-then-scalar cross-page execute-trigger code rewrite sequence, the final boundary bytes match the scalar-overwritten instruction stream and the outer sentinels stay unchanged before fence.i",
        ai_micro_cross_page_trigger_bytes_match(low_phys_start, expected_final, 8U) &&
        ai_micro_cross_page_trigger_bytes_match(high_phys_start, expected_final + 8U, 8U) &&
        read8(sentinel_before_addr) == sentinel_before &&
        read8(sentinel_after_addr) == sentinel_after
    );

    goto_priv(PRIV_HS);
    ai_micro_cross_page_fence_i();
    ai_micro_cross_page_clear_mcontrol6_slot(0);
    ai_micro_cross_page_clear_mcontrol6_slot(1);
    ai_micro_cross_page_arm_mcontrol6_slot(0, code_vaddr,
        AI_MICRO_CROSS_PAGE_MCONTROL6_EXECUTE);
    saved_mstatus = ai_micro_cross_page_enable_machine_breakpoint_delivery();
    goto_priv(PRIV_HS);

    TEST_SETUP_EXCEPT();
    fetch_func();
    TEST_ASSERT("after the mixed vector-then-scalar cross-page rewrite and fence.i, the first fetch to the final executable HS block still reports breakpoint when the execute-PC trigger is armed",
        excpt.triggered == true &&
        excpt.cause == CAUSE_BKP
    );

    goto_priv(PRIV_M);
    TEST_ASSERT("the first trapped fetch to the mixed vector-then-scalar rewritten two-page executable HS block still leaves the final boundary bytes and the outer sentinels unchanged",
        ai_micro_cross_page_trigger_bytes_match(low_phys_start, expected_final, 8U) &&
        ai_micro_cross_page_trigger_bytes_match(high_phys_start, expected_final + 8U, 8U) &&
        read8(sentinel_before_addr) == sentinel_before &&
        read8(sentinel_after_addr) == sentinel_after
    );
    ai_micro_cross_page_restore_machine_status(saved_mstatus);

    saved_mstatus = ai_micro_cross_page_enable_machine_breakpoint_delivery();
    goto_priv(PRIV_HS);
    TEST_SETUP_EXCEPT();
    fetch_func();
    TEST_ASSERT("after the mixed vector-then-scalar cross-page rewrite and fence.i, the second fetch to the final executable HS block still reports breakpoint when the execute-PC trigger remains armed",
        excpt.triggered == true &&
        excpt.cause == CAUSE_BKP
    );

    goto_priv(PRIV_M);
    TEST_ASSERT("the second trapped fetch to the mixed vector-then-scalar rewritten two-page executable HS block still leaves the final boundary bytes and the outer sentinels unchanged",
        ai_micro_cross_page_trigger_bytes_match(low_phys_start, expected_final, 8U) &&
        ai_micro_cross_page_trigger_bytes_match(high_phys_start, expected_final + 8U, 8U) &&
        read8(sentinel_before_addr) == sentinel_before &&
        read8(sentinel_after_addr) == sentinel_after
    );
    ai_micro_cross_page_restore_machine_status(saved_mstatus);

    ai_micro_cross_page_clear_mcontrol6_slot(0);

    goto_priv(PRIV_HS);
    ai_micro_cross_page_fence_i();
    TEST_SETUP_EXCEPT();
    observed = fetch_func();
    TEST_ASSERT("after clearing the execute-PC trigger, the mixed vector-then-scalar rewritten two-page executable HS block completes normally and returns the final scalar-overwritten value",
        excpt.triggered == false &&
        observed == 9ULL
    );

    goto_priv(PRIV_M);
    TEST_ASSERT("after the mixed vector-then-scalar cross-page rewrite plus repeated execute-trigger fetch sequence, the final boundary bytes remain intact and the outer sentinels stay unchanged",
        ai_micro_cross_page_trigger_bytes_match(low_phys_start, expected_final, 8U) &&
        ai_micro_cross_page_trigger_bytes_match(high_phys_start, expected_final + 8U, 8U) &&
        read8(sentinel_before_addr) == sentinel_before &&
        read8(sentinel_after_addr) == sentinel_after
    );

    goto_priv(PRIV_HS);
    hspt_init();
    sfence_vma();

    TEST_END();
}

bool ai_micro_hs_fetch_after_scalar_cross_page_store_after_vector_cross_page_store_fence_i_execute_trigger_repeated_breakpoint_then_no_x_page_fault_adjacent_preserved_corner()
{
    TEST_START();

    enum test_page low_page = VSRWX_GRWX;
    enum test_page high_page = VSRWX_GRW;
    const uint8_t warm_pattern[8] = {
        0x13, 0x05, 0x30, 0x00,
        0x67, 0x80, 0x00, 0x00
    };
    const uint8_t vector_pattern[16] = {
        0x30, 0x31, 0x32, 0x33,
        0x13, 0x05, 0x50, 0x00,
        0x67, 0x80, 0x00, 0x00,
        0x3c, 0x3d, 0x3e, 0x3f
    };
    const uint8_t expected_final[16] = {
        0x30, 0x31, 0x32, 0x33,
        0x13, 0x05, 0x90, 0x00,
        0x67, 0x80, 0x00, 0x00,
        0x3c, 0x3d, 0x3e, 0x3f
    };
    const uintptr_t source_vaddr = hs_page_base(VSRWX_GRWX) + 0x2e0U;
    const uintptr_t source_phys = phys_page_base(VSRWX_GRWX) + 0x2e0U;
    const uintptr_t boundary_vaddr = ai_micro_cross_page_trigger_addr(low_page, 8U);
    const uintptr_t code_vaddr = ai_micro_cross_page_trigger_addr(low_page, 4U);
    const uintptr_t low_phys_start = phys_page_base(low_page) + PAGE_SIZE - 8U;
    const uintptr_t code_phys_start = phys_page_base(low_page) + PAGE_SIZE - 4U;
    const uintptr_t high_phys_start = phys_page_base(high_page);
    const uintptr_t sentinel_before_addr = low_phys_start - 1U;
    const uintptr_t sentinel_after_addr = high_phys_start + 8U;
    const uint8_t sentinel_before = 0x79U;
    const uint8_t sentinel_after = 0xe9U;
    uint64_t (*fetch_func)(void) = (uint64_t (*)(void))code_vaddr;
    uint64_t observed = 0;
    uint64_t saved_mstatus = 0;

    goto_priv(PRIV_M);
    ai_micro_cross_page_trigger_write_bytes(code_phys_start, warm_pattern, 8U);
    ai_micro_cross_page_trigger_write_bytes(source_phys, vector_pattern, 16U);
    write8(sentinel_before_addr, sentinel_before);
    write8(sentinel_after_addr, sentinel_after);

    goto_priv(PRIV_HS);
    hspt_init();

    goto_priv(PRIV_M);
    ai_micro_cross_page_set_hs_leaf(low_page, phys_page_base(low_page),
        PTE_V | PTE_AD | PTE_RWX);
    ai_micro_cross_page_set_hs_leaf(high_page, phys_page_base(high_page),
        PTE_V | PTE_AD | PTE_RWX);
    sfence_vma();

    goto_priv(PRIV_HS);
    ai_micro_cross_page_fence_i();

    TEST_SETUP_EXCEPT();
    observed = fetch_func();
    TEST_ASSERT("before the mixed vector-then-scalar cross-page execute-trigger plus no-X probe starts, the warmed fetch path still returns the original value",
        excpt.triggered == false &&
        observed == 3ULL
    );

    TEST_SETUP_EXCEPT();
    vle8_to_v6((const uint8_t *)source_vaddr, 16);
    TEST_ASSERT("a vector unit-stride load can stage the first cross-page instruction payload before the mixed execute-trigger plus no-X code-rewrite probe",
        excpt.triggered == false
    );

    TEST_SETUP_EXCEPT();
    vse8_from_v6((uint8_t *)boundary_vaddr, 16);
    TEST_ASSERT("a vector unit-stride store can rewrite the shared two-page executable HS boundary before the overlapping scalar cross-page execute-trigger plus no-X code rewrite",
        excpt.triggered == false
    );

    goto_priv(PRIV_M);
    TEST_ASSERT("after the first vector cross-page execute-trigger plus no-X code-rewrite step, the staged boundary bytes are visible across both backing pages and the outer sentinels stay unchanged before the scalar overwrite",
        ai_micro_cross_page_trigger_bytes_match(low_phys_start, vector_pattern, 8U) &&
        ai_micro_cross_page_trigger_bytes_match(high_phys_start, vector_pattern + 8U, 8U) &&
        read8(sentinel_before_addr) == sentinel_before &&
        read8(sentinel_after_addr) == sentinel_after
    );

    goto_priv(PRIV_HS);
    TEST_SETUP_EXCEPT();
    sd(code_vaddr, 0x0000806700900513ULL);
    TEST_ASSERT("after the vector rewrite, an overlapping scalar cross-page store can overwrite the executable HS block to the final instruction stream before the execute-trigger plus no-X fetch probe",
        excpt.triggered == false
    );

    goto_priv(PRIV_M);
    TEST_ASSERT("after the mixed vector-then-scalar cross-page execute-trigger plus no-X code rewrite sequence, the final boundary bytes match the scalar-overwritten instruction stream and the outer sentinels stay unchanged before fence.i",
        ai_micro_cross_page_trigger_bytes_match(low_phys_start, expected_final, 8U) &&
        ai_micro_cross_page_trigger_bytes_match(high_phys_start, expected_final + 8U, 8U) &&
        read8(sentinel_before_addr) == sentinel_before &&
        read8(sentinel_after_addr) == sentinel_after
    );

    goto_priv(PRIV_HS);
    ai_micro_cross_page_fence_i();
    ai_micro_cross_page_clear_mcontrol6_slot(0);
    ai_micro_cross_page_clear_mcontrol6_slot(1);
    ai_micro_cross_page_arm_mcontrol6_slot(0, code_vaddr,
        AI_MICRO_CROSS_PAGE_MCONTROL6_EXECUTE);
    saved_mstatus = ai_micro_cross_page_enable_machine_breakpoint_delivery();
    goto_priv(PRIV_HS);

    TEST_SETUP_EXCEPT();
    fetch_func();
    TEST_ASSERT("after the mixed vector-then-scalar cross-page rewrite and fence.i, the first fetch still reports breakpoint before the later no-X fetch page fault is exposed",
        excpt.triggered == true &&
        excpt.cause == CAUSE_BKP
    );

    goto_priv(PRIV_M);
    TEST_ASSERT("the first trapped fetch in the mixed vector-then-scalar cross-page execute-trigger plus no-X corner still leaves the final boundary bytes and the outer sentinels unchanged",
        ai_micro_cross_page_trigger_bytes_match(low_phys_start, expected_final, 8U) &&
        ai_micro_cross_page_trigger_bytes_match(high_phys_start, expected_final + 8U, 8U) &&
        read8(sentinel_before_addr) == sentinel_before &&
        read8(sentinel_after_addr) == sentinel_after
    );
    ai_micro_cross_page_restore_machine_status(saved_mstatus);

    saved_mstatus = ai_micro_cross_page_enable_machine_breakpoint_delivery();
    goto_priv(PRIV_HS);
    TEST_SETUP_EXCEPT();
    fetch_func();
    TEST_ASSERT("after the mixed vector-then-scalar cross-page rewrite and fence.i, the second fetch still reports breakpoint while the execute-PC trigger remains armed before the later no-X fetch page fault is exposed",
        excpt.triggered == true &&
        excpt.cause == CAUSE_BKP
    );

    goto_priv(PRIV_M);
    TEST_ASSERT("the second trapped fetch in the mixed vector-then-scalar cross-page execute-trigger plus no-X corner still leaves the final boundary bytes and the outer sentinels unchanged",
        ai_micro_cross_page_trigger_bytes_match(low_phys_start, expected_final, 8U) &&
        ai_micro_cross_page_trigger_bytes_match(high_phys_start, expected_final + 8U, 8U) &&
        read8(sentinel_before_addr) == sentinel_before &&
        read8(sentinel_after_addr) == sentinel_after
    );
    ai_micro_cross_page_restore_machine_status(saved_mstatus);
    ai_micro_cross_page_clear_mcontrol6_slot(0);
    ai_micro_cross_page_set_hs_leaf(low_page, phys_page_base(low_page),
        PTE_V | PTE_AD | PTE_RW);
    ai_micro_cross_page_set_hs_leaf(high_page, phys_page_base(high_page),
        PTE_V | PTE_AD | PTE_RW);
    sfence_vma();

    goto_priv(PRIV_HS);
    ai_micro_cross_page_fence_i();
    TEST_SETUP_EXCEPT();
    TEST_EXEC_EXCEPT(code_vaddr);
    TEST_ASSERT("after clearing the execute-PC trigger, the same mixed vector-then-scalar rewritten two-page executable HS block falls back to instruction page fault when execute permission is removed",
        excpt.triggered == true &&
        excpt.cause == CAUSE_IPF &&
        excpt.tval == code_vaddr
    );

    goto_priv(PRIV_M);
    TEST_ASSERT("after repeated execute-trigger breakpoint hits and the later no-X fetch page fault, the final boundary bytes remain intact and the outer sentinels stay unchanged",
        ai_micro_cross_page_trigger_bytes_match(low_phys_start, expected_final, 8U) &&
        ai_micro_cross_page_trigger_bytes_match(high_phys_start, expected_final + 8U, 8U) &&
        read8(sentinel_before_addr) == sentinel_before &&
        read8(sentinel_after_addr) == sentinel_after
    );

    goto_priv(PRIV_HS);
    hspt_init();
    sfence_vma();

    TEST_END();
}

bool ai_micro_hs_fetch_after_scalar_cross_page_store_after_vector_cross_page_store_fence_i_execute_trigger_repeated_breakpoint_then_grant_x_executes_adjacent_preserved_corner()
{
    TEST_START();

    enum test_page low_page = VSRWX_GRWX;
    enum test_page high_page = VSRWX_GRW;
    const uint8_t warm_pattern[8] = {
        0x13, 0x05, 0x30, 0x00,
        0x67, 0x80, 0x00, 0x00
    };
    const uint8_t vector_pattern[16] = {
        0x30, 0x31, 0x32, 0x33,
        0x13, 0x05, 0x50, 0x00,
        0x67, 0x80, 0x00, 0x00,
        0x3c, 0x3d, 0x3e, 0x3f
    };
    const uint8_t expected_final[16] = {
        0x30, 0x31, 0x32, 0x33,
        0x13, 0x05, 0x90, 0x00,
        0x67, 0x80, 0x00, 0x00,
        0x3c, 0x3d, 0x3e, 0x3f
    };
    const uintptr_t source_vaddr = hs_page_base(VSRWX_GRWX) + 0x300U;
    const uintptr_t source_phys = phys_page_base(VSRWX_GRWX) + 0x300U;
    const uintptr_t boundary_vaddr = ai_micro_cross_page_trigger_addr(low_page, 8U);
    const uintptr_t code_vaddr = ai_micro_cross_page_trigger_addr(low_page, 4U);
    const uintptr_t low_phys_start = phys_page_base(low_page) + PAGE_SIZE - 8U;
    const uintptr_t code_phys_start = phys_page_base(low_page) + PAGE_SIZE - 4U;
    const uintptr_t high_phys_start = phys_page_base(high_page);
    const uintptr_t sentinel_before_addr = low_phys_start - 1U;
    const uintptr_t sentinel_after_addr = high_phys_start + 8U;
    const uint8_t sentinel_before = 0x7aU;
    const uint8_t sentinel_after = 0xfaU;
    uint64_t (*fetch_func)(void) = (uint64_t (*)(void))code_vaddr;
    uint64_t observed = 0;
    uint64_t saved_mstatus = 0;

    goto_priv(PRIV_M);
    ai_micro_cross_page_trigger_write_bytes(code_phys_start, warm_pattern, 8U);
    ai_micro_cross_page_trigger_write_bytes(source_phys, vector_pattern, 16U);
    write8(sentinel_before_addr, sentinel_before);
    write8(sentinel_after_addr, sentinel_after);

    goto_priv(PRIV_HS);
    hspt_init();

    goto_priv(PRIV_M);
    ai_micro_cross_page_set_hs_leaf(low_page, phys_page_base(low_page),
        PTE_V | PTE_AD | PTE_RWX);
    ai_micro_cross_page_set_hs_leaf(high_page, phys_page_base(high_page),
        PTE_V | PTE_AD | PTE_RWX);
    sfence_vma();

    goto_priv(PRIV_HS);
    ai_micro_cross_page_fence_i();

    TEST_SETUP_EXCEPT();
    observed = fetch_func();
    TEST_ASSERT("before the mixed vector-then-scalar cross-page execute-trigger plus grant-X-recovery probe starts, the warmed fetch path still returns the original value",
        excpt.triggered == false &&
        observed == 3ULL
    );

    TEST_SETUP_EXCEPT();
    vle8_to_v6((const uint8_t *)source_vaddr, 16);
    TEST_ASSERT("a vector unit-stride load can stage the first cross-page instruction payload before the mixed execute-trigger plus grant-X-recovery code-rewrite probe",
        excpt.triggered == false
    );

    TEST_SETUP_EXCEPT();
    vse8_from_v6((uint8_t *)boundary_vaddr, 16);
    TEST_ASSERT("a vector unit-stride store can rewrite the shared two-page executable HS boundary before the overlapping scalar cross-page execute-trigger plus grant-X-recovery code rewrite",
        excpt.triggered == false
    );

    goto_priv(PRIV_M);
    TEST_ASSERT("after the first vector cross-page execute-trigger plus grant-X-recovery code-rewrite step, the staged boundary bytes are visible across both backing pages and the outer sentinels stay unchanged before the scalar overwrite",
        ai_micro_cross_page_trigger_bytes_match(low_phys_start, vector_pattern, 8U) &&
        ai_micro_cross_page_trigger_bytes_match(high_phys_start, vector_pattern + 8U, 8U) &&
        read8(sentinel_before_addr) == sentinel_before &&
        read8(sentinel_after_addr) == sentinel_after
    );

    goto_priv(PRIV_HS);
    TEST_SETUP_EXCEPT();
    sd(code_vaddr, 0x0000806700900513ULL);
    TEST_ASSERT("after the vector rewrite, an overlapping scalar cross-page store can overwrite the executable HS block to the final instruction stream before the execute-trigger plus grant-X-recovery fetch probe",
        excpt.triggered == false
    );

    goto_priv(PRIV_M);
    TEST_ASSERT("after the mixed vector-then-scalar cross-page execute-trigger plus grant-X-recovery code rewrite sequence, the final boundary bytes match the scalar-overwritten instruction stream and the outer sentinels stay unchanged before fence.i",
        ai_micro_cross_page_trigger_bytes_match(low_phys_start, expected_final, 8U) &&
        ai_micro_cross_page_trigger_bytes_match(high_phys_start, expected_final + 8U, 8U) &&
        read8(sentinel_before_addr) == sentinel_before &&
        read8(sentinel_after_addr) == sentinel_after
    );

    goto_priv(PRIV_HS);
    ai_micro_cross_page_fence_i();
    ai_micro_cross_page_clear_mcontrol6_slot(0);
    ai_micro_cross_page_clear_mcontrol6_slot(1);
    ai_micro_cross_page_arm_mcontrol6_slot(0, code_vaddr,
        AI_MICRO_CROSS_PAGE_MCONTROL6_EXECUTE);
    saved_mstatus = ai_micro_cross_page_enable_machine_breakpoint_delivery();
    goto_priv(PRIV_HS);

    TEST_SETUP_EXCEPT();
    fetch_func();
    TEST_ASSERT("after the mixed vector-then-scalar cross-page rewrite and fence.i, the first fetch still reports breakpoint before the later grant-X-recovery sequence begins",
        excpt.triggered == true &&
        excpt.cause == CAUSE_BKP
    );

    goto_priv(PRIV_M);
    TEST_ASSERT("the first trapped fetch in the mixed vector-then-scalar cross-page execute-trigger plus grant-X-recovery corner still leaves the final boundary bytes and the outer sentinels unchanged",
        ai_micro_cross_page_trigger_bytes_match(low_phys_start, expected_final, 8U) &&
        ai_micro_cross_page_trigger_bytes_match(high_phys_start, expected_final + 8U, 8U) &&
        read8(sentinel_before_addr) == sentinel_before &&
        read8(sentinel_after_addr) == sentinel_after
    );
    ai_micro_cross_page_restore_machine_status(saved_mstatus);

    saved_mstatus = ai_micro_cross_page_enable_machine_breakpoint_delivery();
    goto_priv(PRIV_HS);
    TEST_SETUP_EXCEPT();
    fetch_func();
    TEST_ASSERT("after the mixed vector-then-scalar cross-page rewrite and fence.i, the second fetch still reports breakpoint while the execute-PC trigger remains armed before the later grant-X-recovery sequence begins",
        excpt.triggered == true &&
        excpt.cause == CAUSE_BKP
    );

    goto_priv(PRIV_M);
    TEST_ASSERT("the second trapped fetch in the mixed vector-then-scalar cross-page execute-trigger plus grant-X-recovery corner still leaves the final boundary bytes and the outer sentinels unchanged",
        ai_micro_cross_page_trigger_bytes_match(low_phys_start, expected_final, 8U) &&
        ai_micro_cross_page_trigger_bytes_match(high_phys_start, expected_final + 8U, 8U) &&
        read8(sentinel_before_addr) == sentinel_before &&
        read8(sentinel_after_addr) == sentinel_after
    );
    ai_micro_cross_page_restore_machine_status(saved_mstatus);
    ai_micro_cross_page_clear_mcontrol6_slot(0);
    ai_micro_cross_page_set_hs_leaf(low_page, phys_page_base(low_page),
        PTE_V | PTE_AD | PTE_RW);
    ai_micro_cross_page_set_hs_leaf(high_page, phys_page_base(high_page),
        PTE_V | PTE_AD | PTE_RW);
    sfence_vma();

    goto_priv(PRIV_HS);
    ai_micro_cross_page_fence_i();
    TEST_SETUP_EXCEPT();
    TEST_EXEC_EXCEPT(code_vaddr);
    TEST_ASSERT("after clearing the execute-PC trigger, the same mixed vector-then-scalar rewritten two-page executable HS block first falls back to instruction page fault when execute permission is removed",
        excpt.triggered == true &&
        excpt.cause == CAUSE_IPF &&
        excpt.tval == code_vaddr
    );

    goto_priv(PRIV_M);
    TEST_ASSERT("the no-X fetch page fault before execute recovery still leaves the final boundary bytes and the outer sentinels unchanged in the mixed cross-page execute-trigger corner",
        ai_micro_cross_page_trigger_bytes_match(low_phys_start, expected_final, 8U) &&
        ai_micro_cross_page_trigger_bytes_match(high_phys_start, expected_final + 8U, 8U) &&
        read8(sentinel_before_addr) == sentinel_before &&
        read8(sentinel_after_addr) == sentinel_after
    );
    ai_micro_cross_page_set_hs_leaf(low_page, phys_page_base(low_page),
        PTE_V | PTE_AD | PTE_RWX);
    ai_micro_cross_page_set_hs_leaf(high_page, phys_page_base(high_page),
        PTE_V | PTE_AD | PTE_RWX);
    sfence_vma();

    goto_priv(PRIV_HS);
    ai_micro_cross_page_fence_i();
    TEST_SETUP_EXCEPT();
    observed = fetch_func();
    TEST_ASSERT("after restoring execute permission, the mixed vector-then-scalar rewritten two-page executable HS block completes normally again and returns the final scalar-overwritten value",
        excpt.triggered == false &&
        observed == 9ULL
    );

    goto_priv(PRIV_M);
    TEST_ASSERT("after repeated execute-trigger breakpoint hits, the later no-X fetch page fault, and final execute-permission repair, the final boundary bytes remain intact and the outer sentinels stay unchanged",
        ai_micro_cross_page_trigger_bytes_match(low_phys_start, expected_final, 8U) &&
        ai_micro_cross_page_trigger_bytes_match(high_phys_start, expected_final + 8U, 8U) &&
        read8(sentinel_before_addr) == sentinel_before &&
        read8(sentinel_after_addr) == sentinel_after
    );

    goto_priv(PRIV_HS);
    hspt_init();
    sfence_vma();

    TEST_END();
}

bool ai_micro_hs_fetch_after_scalar_cross_page_store_after_vector_cross_page_store_fence_i_execute_trigger_repeated_breakpoint_then_leaf_overflow_fetch_access_fault_adjacent_preserved_corner()
{
    TEST_START();

    enum test_page low_page = VSRWX_GRWX;
    enum test_page high_page = VSRWX_GRW;
    const uint8_t warm_pattern[8] = {
        0x13, 0x05, 0x30, 0x00,
        0x67, 0x80, 0x00, 0x00
    };
    const uint8_t vector_pattern[16] = {
        0x30, 0x31, 0x32, 0x33,
        0x13, 0x05, 0x50, 0x00,
        0x67, 0x80, 0x00, 0x00,
        0x3c, 0x3d, 0x3e, 0x3f
    };
    const uint8_t expected_final[16] = {
        0x30, 0x31, 0x32, 0x33,
        0x13, 0x05, 0x90, 0x00,
        0x67, 0x80, 0x00, 0x00,
        0x3c, 0x3d, 0x3e, 0x3f
    };
    const uintptr_t source_vaddr = hs_page_base(VSRWX_GRWX) + 0x260U;
    const uintptr_t source_phys = phys_page_base(VSRWX_GRWX) + 0x260U;
    const uintptr_t boundary_vaddr = ai_micro_cross_page_trigger_addr(low_page, 8U);
    const uintptr_t code_vaddr = ai_micro_cross_page_trigger_addr(low_page, 4U);
    const uintptr_t second_half_vaddr = code_vaddr + 4U;
    const uintptr_t low_phys_start = phys_page_base(low_page) + PAGE_SIZE - 8U;
    const uintptr_t code_phys_start = phys_page_base(low_page) + PAGE_SIZE - 4U;
    const uintptr_t high_phys_start = phys_page_base(high_page);
    const uintptr_t sentinel_before_addr = low_phys_start - 1U;
    const uintptr_t sentinel_after_addr = high_phys_start + 8U;
    const uint8_t sentinel_before = 0x73U;
    const uint8_t sentinel_after = 0xb7U;
    uint64_t (*fetch_func)(void) = (uint64_t (*)(void))code_vaddr;
    uint64_t observed = 0;
    uint64_t saved_mstatus = 0;

    goto_priv(PRIV_M);
    ai_micro_cross_page_trigger_write_bytes(code_phys_start, warm_pattern, 8U);
    ai_micro_cross_page_trigger_write_bytes(source_phys, vector_pattern, 16U);
    write8(sentinel_before_addr, sentinel_before);
    write8(sentinel_after_addr, sentinel_after);

    goto_priv(PRIV_HS);
    hspt_init();

    goto_priv(PRIV_M);
    ai_micro_cross_page_set_hs_leaf(low_page, phys_page_base(low_page),
        PTE_V | PTE_AD | PTE_RWX);
    ai_micro_cross_page_set_hs_leaf(high_page, phys_page_base(high_page),
        PTE_V | PTE_AD | PTE_RWX);
    sfence_vma();

    goto_priv(PRIV_HS);
    ai_micro_cross_page_fence_i();

    TEST_SETUP_EXCEPT();
    observed = fetch_func();
    TEST_ASSERT("before the mixed vector-then-scalar cross-page execute-trigger plus leaf-overflow probe starts, the warmed fetch path still returns the original value",
        excpt.triggered == false &&
        observed == 3ULL
    );

    TEST_SETUP_EXCEPT();
    vle8_to_v6((const uint8_t *)source_vaddr, 16);
    TEST_ASSERT("a vector unit-stride load can stage the first cross-page instruction payload before the mixed execute-trigger plus leaf-overflow code-rewrite probe",
        excpt.triggered == false
    );

    TEST_SETUP_EXCEPT();
    vse8_from_v6((uint8_t *)boundary_vaddr, 16);
    TEST_ASSERT("a vector unit-stride store can rewrite the shared two-page executable HS boundary before the overlapping scalar cross-page execute-trigger plus leaf-overflow code rewrite",
        excpt.triggered == false
    );

    goto_priv(PRIV_M);
    TEST_ASSERT("after the first vector cross-page execute-trigger plus leaf-overflow code-rewrite step, the staged boundary bytes are visible across both backing pages and the outer sentinels stay unchanged before the scalar overwrite",
        ai_micro_cross_page_trigger_bytes_match(low_phys_start, vector_pattern, 8U) &&
        ai_micro_cross_page_trigger_bytes_match(high_phys_start, vector_pattern + 8U, 8U) &&
        read8(sentinel_before_addr) == sentinel_before &&
        read8(sentinel_after_addr) == sentinel_after
    );

    goto_priv(PRIV_HS);
    TEST_SETUP_EXCEPT();
    sd(code_vaddr, 0x0000806700900513ULL);
    TEST_ASSERT("after the vector rewrite, an overlapping scalar cross-page store can overwrite the executable HS block to the final instruction stream before the execute-trigger plus leaf-overflow fetch probe",
        excpt.triggered == false
    );

    goto_priv(PRIV_M);
    TEST_ASSERT("after the mixed vector-then-scalar cross-page execute-trigger plus leaf-overflow code rewrite sequence, the final boundary bytes match the scalar-overwritten instruction stream and the outer sentinels stay unchanged before fence.i",
        ai_micro_cross_page_trigger_bytes_match(low_phys_start, expected_final, 8U) &&
        ai_micro_cross_page_trigger_bytes_match(high_phys_start, expected_final + 8U, 8U) &&
        read8(sentinel_before_addr) == sentinel_before &&
        read8(sentinel_after_addr) == sentinel_after
    );

    goto_priv(PRIV_HS);
    ai_micro_cross_page_fence_i();
    ai_micro_cross_page_clear_mcontrol6_slot(0);
    ai_micro_cross_page_clear_mcontrol6_slot(1);
    ai_micro_cross_page_arm_mcontrol6_slot(0, code_vaddr,
        AI_MICRO_CROSS_PAGE_MCONTROL6_EXECUTE);
    saved_mstatus = ai_micro_cross_page_enable_machine_breakpoint_delivery();
    goto_priv(PRIV_HS);

    TEST_SETUP_EXCEPT();
    fetch_func();
    TEST_ASSERT("after the mixed vector-then-scalar cross-page rewrite and fence.i, the first fetch still reports breakpoint before the later upper-page leaf-overflow fetch access fault is exposed",
        excpt.triggered == true &&
        excpt.cause == CAUSE_BKP
    );

    goto_priv(PRIV_M);
    TEST_ASSERT("the first trapped fetch in the mixed vector-then-scalar cross-page execute-trigger plus leaf-overflow corner still leaves the final boundary bytes and the outer sentinels unchanged",
        ai_micro_cross_page_trigger_bytes_match(low_phys_start, expected_final, 8U) &&
        ai_micro_cross_page_trigger_bytes_match(high_phys_start, expected_final + 8U, 8U) &&
        read8(sentinel_before_addr) == sentinel_before &&
        read8(sentinel_after_addr) == sentinel_after
    );
    ai_micro_cross_page_restore_machine_status(saved_mstatus);

    saved_mstatus = ai_micro_cross_page_enable_machine_breakpoint_delivery();
    goto_priv(PRIV_HS);
    TEST_SETUP_EXCEPT();
    fetch_func();
    TEST_ASSERT("after the mixed vector-then-scalar cross-page rewrite and fence.i, the second fetch still reports breakpoint while the execute-PC trigger remains armed before the later upper-page leaf-overflow fetch access fault is exposed",
        excpt.triggered == true &&
        excpt.cause == CAUSE_BKP
    );

    goto_priv(PRIV_M);
    TEST_ASSERT("the second trapped fetch in the mixed vector-then-scalar cross-page execute-trigger plus leaf-overflow corner still leaves the final boundary bytes and the outer sentinels unchanged",
        ai_micro_cross_page_trigger_bytes_match(low_phys_start, expected_final, 8U) &&
        ai_micro_cross_page_trigger_bytes_match(high_phys_start, expected_final + 8U, 8U) &&
        read8(sentinel_before_addr) == sentinel_before &&
        read8(sentinel_after_addr) == sentinel_after
    );
    ai_micro_cross_page_restore_machine_status(saved_mstatus);
    ai_micro_cross_page_clear_mcontrol6_slot(0);
    *ai_micro_cross_page_hs_leaf_entry(high_page) |= (1ULL << 46);
    sfence_vma();

    goto_priv(PRIV_HS);
    ai_micro_cross_page_fence_i();
    TEST_SETUP_EXCEPT();
    TEST_EXEC_EXCEPT(code_vaddr);
    TEST_ASSERT("after clearing the execute-PC trigger, the same mixed vector-then-scalar rewritten two-page executable HS block falls back to instruction access fault when the upper-page leaf PPN overflows",
        excpt.triggered == true &&
        excpt.cause == CAUSE_IAF &&
        excpt.tval == second_half_vaddr
    );

    goto_priv(PRIV_M);
    TEST_ASSERT("after repeated execute-trigger breakpoint hits and the later upper-page leaf-overflow fetch access fault, the final boundary bytes remain intact and the outer sentinels stay unchanged",
        ai_micro_cross_page_trigger_bytes_match(low_phys_start, expected_final, 8U) &&
        ai_micro_cross_page_trigger_bytes_match(high_phys_start, expected_final + 8U, 8U) &&
        read8(sentinel_before_addr) == sentinel_before &&
        read8(sentinel_after_addr) == sentinel_after
    );

    goto_priv(PRIV_HS);
    hspt_init();
    sfence_vma();

    TEST_END();
}

bool ai_micro_hs_fetch_after_scalar_cross_page_store_after_vector_cross_page_store_fence_i_execute_trigger_repeated_breakpoint_then_leaf_overflow_fix_leaf_executes_adjacent_preserved_corner()
{
    TEST_START();

    enum test_page low_page = VSRWX_GRWX;
    enum test_page high_page = VSRWX_GRW;
    const uint8_t warm_pattern[8] = {
        0x13, 0x05, 0x30, 0x00,
        0x67, 0x80, 0x00, 0x00
    };
    const uint8_t vector_pattern[16] = {
        0x30, 0x31, 0x32, 0x33,
        0x13, 0x05, 0x50, 0x00,
        0x67, 0x80, 0x00, 0x00,
        0x3c, 0x3d, 0x3e, 0x3f
    };
    const uint8_t expected_final[16] = {
        0x30, 0x31, 0x32, 0x33,
        0x13, 0x05, 0x90, 0x00,
        0x67, 0x80, 0x00, 0x00,
        0x3c, 0x3d, 0x3e, 0x3f
    };
    const uintptr_t source_vaddr = hs_page_base(VSRWX_GRWX) + 0x280U;
    const uintptr_t source_phys = phys_page_base(VSRWX_GRWX) + 0x280U;
    const uintptr_t boundary_vaddr = ai_micro_cross_page_trigger_addr(low_page, 8U);
    const uintptr_t code_vaddr = ai_micro_cross_page_trigger_addr(low_page, 4U);
    const uintptr_t second_half_vaddr = code_vaddr + 4U;
    const uintptr_t low_phys_start = phys_page_base(low_page) + PAGE_SIZE - 8U;
    const uintptr_t code_phys_start = phys_page_base(low_page) + PAGE_SIZE - 4U;
    const uintptr_t high_phys_start = phys_page_base(high_page);
    const uintptr_t sentinel_before_addr = low_phys_start - 1U;
    const uintptr_t sentinel_after_addr = high_phys_start + 8U;
    const uint8_t sentinel_before = 0x74U;
    const uint8_t sentinel_after = 0xc7U;
    uint64_t (*fetch_func)(void) = (uint64_t (*)(void))code_vaddr;
    uint64_t observed = 0;
    uint64_t saved_mstatus = 0;

    goto_priv(PRIV_M);
    ai_micro_cross_page_trigger_write_bytes(code_phys_start, warm_pattern, 8U);
    ai_micro_cross_page_trigger_write_bytes(source_phys, vector_pattern, 16U);
    write8(sentinel_before_addr, sentinel_before);
    write8(sentinel_after_addr, sentinel_after);

    goto_priv(PRIV_HS);
    hspt_init();

    goto_priv(PRIV_M);
    ai_micro_cross_page_set_hs_leaf(low_page, phys_page_base(low_page),
        PTE_V | PTE_AD | PTE_RWX);
    ai_micro_cross_page_set_hs_leaf(high_page, phys_page_base(high_page),
        PTE_V | PTE_AD | PTE_RWX);
    sfence_vma();

    goto_priv(PRIV_HS);
    ai_micro_cross_page_fence_i();

    TEST_SETUP_EXCEPT();
    observed = fetch_func();
    TEST_ASSERT("before the mixed vector-then-scalar cross-page execute-trigger plus leaf-repair probe starts, the warmed fetch path still returns the original value",
        excpt.triggered == false &&
        observed == 3ULL
    );

    TEST_SETUP_EXCEPT();
    vle8_to_v6((const uint8_t *)source_vaddr, 16);
    TEST_ASSERT("a vector unit-stride load can stage the first cross-page instruction payload before the mixed execute-trigger plus leaf-repair code-rewrite probe",
        excpt.triggered == false
    );

    TEST_SETUP_EXCEPT();
    vse8_from_v6((uint8_t *)boundary_vaddr, 16);
    TEST_ASSERT("a vector unit-stride store can rewrite the shared two-page executable HS boundary before the overlapping scalar cross-page execute-trigger plus leaf-repair code rewrite",
        excpt.triggered == false
    );

    goto_priv(PRIV_M);
    TEST_ASSERT("after the first vector cross-page execute-trigger plus leaf-repair code-rewrite step, the staged boundary bytes are visible across both backing pages and the outer sentinels stay unchanged before the scalar overwrite",
        ai_micro_cross_page_trigger_bytes_match(low_phys_start, vector_pattern, 8U) &&
        ai_micro_cross_page_trigger_bytes_match(high_phys_start, vector_pattern + 8U, 8U) &&
        read8(sentinel_before_addr) == sentinel_before &&
        read8(sentinel_after_addr) == sentinel_after
    );

    goto_priv(PRIV_HS);
    TEST_SETUP_EXCEPT();
    sd(code_vaddr, 0x0000806700900513ULL);
    TEST_ASSERT("after the vector rewrite, an overlapping scalar cross-page store can overwrite the executable HS block to the final instruction stream before the execute-trigger plus leaf-repair fetch probe",
        excpt.triggered == false
    );

    goto_priv(PRIV_M);
    TEST_ASSERT("after the mixed vector-then-scalar cross-page execute-trigger plus leaf-repair code rewrite sequence, the final boundary bytes match the scalar-overwritten instruction stream and the outer sentinels stay unchanged before fence.i",
        ai_micro_cross_page_trigger_bytes_match(low_phys_start, expected_final, 8U) &&
        ai_micro_cross_page_trigger_bytes_match(high_phys_start, expected_final + 8U, 8U) &&
        read8(sentinel_before_addr) == sentinel_before &&
        read8(sentinel_after_addr) == sentinel_after
    );

    goto_priv(PRIV_HS);
    ai_micro_cross_page_fence_i();
    ai_micro_cross_page_clear_mcontrol6_slot(0);
    ai_micro_cross_page_clear_mcontrol6_slot(1);
    ai_micro_cross_page_arm_mcontrol6_slot(0, code_vaddr,
        AI_MICRO_CROSS_PAGE_MCONTROL6_EXECUTE);
    saved_mstatus = ai_micro_cross_page_enable_machine_breakpoint_delivery();
    goto_priv(PRIV_HS);

    TEST_SETUP_EXCEPT();
    fetch_func();
    TEST_ASSERT("after the mixed vector-then-scalar cross-page rewrite and fence.i, the first fetch still reports breakpoint before the later upper-page leaf repair sequence begins",
        excpt.triggered == true &&
        excpt.cause == CAUSE_BKP
    );

    goto_priv(PRIV_M);
    TEST_ASSERT("the first trapped fetch in the mixed vector-then-scalar cross-page execute-trigger plus leaf-repair corner still leaves the final boundary bytes and the outer sentinels unchanged",
        ai_micro_cross_page_trigger_bytes_match(low_phys_start, expected_final, 8U) &&
        ai_micro_cross_page_trigger_bytes_match(high_phys_start, expected_final + 8U, 8U) &&
        read8(sentinel_before_addr) == sentinel_before &&
        read8(sentinel_after_addr) == sentinel_after
    );
    ai_micro_cross_page_restore_machine_status(saved_mstatus);

    saved_mstatus = ai_micro_cross_page_enable_machine_breakpoint_delivery();
    goto_priv(PRIV_HS);
    TEST_SETUP_EXCEPT();
    fetch_func();
    TEST_ASSERT("after the mixed vector-then-scalar cross-page rewrite and fence.i, the second fetch still reports breakpoint while the execute-PC trigger remains armed before the later upper-page leaf repair sequence begins",
        excpt.triggered == true &&
        excpt.cause == CAUSE_BKP
    );

    goto_priv(PRIV_M);
    TEST_ASSERT("the second trapped fetch in the mixed vector-then-scalar cross-page execute-trigger plus leaf-repair corner still leaves the final boundary bytes and the outer sentinels unchanged",
        ai_micro_cross_page_trigger_bytes_match(low_phys_start, expected_final, 8U) &&
        ai_micro_cross_page_trigger_bytes_match(high_phys_start, expected_final + 8U, 8U) &&
        read8(sentinel_before_addr) == sentinel_before &&
        read8(sentinel_after_addr) == sentinel_after
    );
    ai_micro_cross_page_restore_machine_status(saved_mstatus);
    ai_micro_cross_page_clear_mcontrol6_slot(0);
    *ai_micro_cross_page_hs_leaf_entry(high_page) |= (1ULL << 46);
    sfence_vma();

    goto_priv(PRIV_HS);
    ai_micro_cross_page_fence_i();
    TEST_SETUP_EXCEPT();
    TEST_EXEC_EXCEPT(code_vaddr);
    TEST_ASSERT("after clearing the execute-PC trigger, the same mixed vector-then-scalar rewritten two-page executable HS block first falls back to instruction access fault when the upper-page leaf PPN overflows",
        excpt.triggered == true &&
        excpt.cause == CAUSE_IAF &&
        excpt.tval == second_half_vaddr
    );

    goto_priv(PRIV_M);
    TEST_ASSERT("the upper-page leaf-overflow fetch access fault before repair still leaves the final boundary bytes and the outer sentinels unchanged in the mixed cross-page execute-trigger corner",
        ai_micro_cross_page_trigger_bytes_match(low_phys_start, expected_final, 8U) &&
        ai_micro_cross_page_trigger_bytes_match(high_phys_start, expected_final + 8U, 8U) &&
        read8(sentinel_before_addr) == sentinel_before &&
        read8(sentinel_after_addr) == sentinel_after
    );
    ai_micro_cross_page_set_hs_leaf(low_page, phys_page_base(low_page),
        PTE_V | PTE_AD | PTE_RWX);
    ai_micro_cross_page_set_hs_leaf(high_page, phys_page_base(high_page),
        PTE_V | PTE_AD | PTE_RWX);
    sfence_vma();

    goto_priv(PRIV_HS);
    ai_micro_cross_page_fence_i();
    TEST_SETUP_EXCEPT();
    observed = fetch_func();
    TEST_ASSERT("after repairing the upper-page leaf entry, the mixed vector-then-scalar rewritten two-page executable HS block completes normally again and returns the final scalar-overwritten value",
        excpt.triggered == false &&
        observed == 9ULL
    );

    goto_priv(PRIV_M);
    TEST_ASSERT("after repeated execute-trigger breakpoint hits, the later upper-page leaf-overflow fetch access fault, and final leaf repair, the final boundary bytes remain intact and the outer sentinels stay unchanged",
        ai_micro_cross_page_trigger_bytes_match(low_phys_start, expected_final, 8U) &&
        ai_micro_cross_page_trigger_bytes_match(high_phys_start, expected_final + 8U, 8U) &&
        read8(sentinel_before_addr) == sentinel_before &&
        read8(sentinel_after_addr) == sentinel_after
    );

    goto_priv(PRIV_HS);
    hspt_init();
    sfence_vma();

    TEST_END();
}

bool ai_micro_hs_fetch_after_scalar_cross_page_store_after_vector_cross_page_store_fence_i_execute_trigger_repeated_breakpoint_then_noncanonical_fetch_page_fault_adjacent_preserved_corner()
{
    TEST_START();

    enum test_page low_page = VSRWX_GRWX;
    enum test_page high_page = VSRWX_GRW;
    const uint8_t warm_pattern[8] = {
        0x13, 0x05, 0x30, 0x00,
        0x67, 0x80, 0x00, 0x00
    };
    const uint8_t vector_pattern[16] = {
        0x30, 0x31, 0x32, 0x33,
        0x13, 0x05, 0x50, 0x00,
        0x67, 0x80, 0x00, 0x00,
        0x3c, 0x3d, 0x3e, 0x3f
    };
    const uint8_t expected_final[16] = {
        0x30, 0x31, 0x32, 0x33,
        0x13, 0x05, 0x90, 0x00,
        0x67, 0x80, 0x00, 0x00,
        0x3c, 0x3d, 0x3e, 0x3f
    };
    const uintptr_t source_vaddr = hs_page_base(VSRWX_GRWX) + 0x2a0U;
    const uintptr_t source_phys = phys_page_base(VSRWX_GRWX) + 0x2a0U;
    const uintptr_t boundary_vaddr = ai_micro_cross_page_trigger_addr(low_page, 8U);
    const uintptr_t code_vaddr = ai_micro_cross_page_trigger_addr(low_page, 4U);
    const uintptr_t bad_vaddr = ai_micro_cross_page_trigger_make_noncanonical_vaddr(code_vaddr);
    const uintptr_t low_phys_start = phys_page_base(low_page) + PAGE_SIZE - 8U;
    const uintptr_t code_phys_start = phys_page_base(low_page) + PAGE_SIZE - 4U;
    const uintptr_t high_phys_start = phys_page_base(high_page);
    const uintptr_t sentinel_before_addr = low_phys_start - 1U;
    const uintptr_t sentinel_after_addr = high_phys_start + 8U;
    const uint8_t sentinel_before = 0x77U;
    const uint8_t sentinel_after = 0xf7U;
    uint64_t (*fetch_func)(void) = (uint64_t (*)(void))code_vaddr;
    uint64_t observed = 0;
    uint64_t saved_mstatus = 0;

    goto_priv(PRIV_M);
    ai_micro_cross_page_trigger_write_bytes(code_phys_start, warm_pattern, 8U);
    ai_micro_cross_page_trigger_write_bytes(source_phys, vector_pattern, 16U);
    write8(sentinel_before_addr, sentinel_before);
    write8(sentinel_after_addr, sentinel_after);

    goto_priv(PRIV_HS);
    hspt_init();

    goto_priv(PRIV_M);
    ai_micro_cross_page_set_hs_leaf(low_page, phys_page_base(low_page),
        PTE_V | PTE_AD | PTE_RWX);
    ai_micro_cross_page_set_hs_leaf(high_page, phys_page_base(high_page),
        PTE_V | PTE_AD | PTE_RWX);
    sfence_vma();

    goto_priv(PRIV_HS);
    ai_micro_cross_page_fence_i();

    TEST_SETUP_EXCEPT();
    observed = fetch_func();
    TEST_ASSERT("before the mixed vector-then-scalar cross-page execute-trigger plus non-canonical fetch probe starts, the warmed fetch path still returns the original value",
        excpt.triggered == false &&
        observed == 3ULL
    );

    TEST_SETUP_EXCEPT();
    vle8_to_v6((const uint8_t *)source_vaddr, 16);
    TEST_ASSERT("a vector unit-stride load can stage the first cross-page instruction payload before the mixed execute-trigger plus non-canonical code-rewrite probe",
        excpt.triggered == false
    );

    TEST_SETUP_EXCEPT();
    vse8_from_v6((uint8_t *)boundary_vaddr, 16);
    TEST_ASSERT("a vector unit-stride store can rewrite the shared two-page executable HS boundary before the overlapping scalar cross-page execute-trigger plus non-canonical code rewrite",
        excpt.triggered == false
    );

    goto_priv(PRIV_M);
    TEST_ASSERT("after the first vector cross-page execute-trigger plus non-canonical code-rewrite step, the staged boundary bytes are visible across both backing pages and the outer sentinels stay unchanged before the scalar overwrite",
        ai_micro_cross_page_trigger_bytes_match(low_phys_start, vector_pattern, 8U) &&
        ai_micro_cross_page_trigger_bytes_match(high_phys_start, vector_pattern + 8U, 8U) &&
        read8(sentinel_before_addr) == sentinel_before &&
        read8(sentinel_after_addr) == sentinel_after
    );

    goto_priv(PRIV_HS);
    TEST_SETUP_EXCEPT();
    sd(code_vaddr, 0x0000806700900513ULL);
    TEST_ASSERT("after the vector rewrite, an overlapping scalar cross-page store can overwrite the executable HS block to the final instruction stream before the execute-trigger plus non-canonical fetch probe",
        excpt.triggered == false
    );

    goto_priv(PRIV_M);
    TEST_ASSERT("after the mixed vector-then-scalar cross-page execute-trigger plus non-canonical code rewrite sequence, the final boundary bytes match the scalar-overwritten instruction stream and the outer sentinels stay unchanged before fence.i",
        ai_micro_cross_page_trigger_bytes_match(low_phys_start, expected_final, 8U) &&
        ai_micro_cross_page_trigger_bytes_match(high_phys_start, expected_final + 8U, 8U) &&
        read8(sentinel_before_addr) == sentinel_before &&
        read8(sentinel_after_addr) == sentinel_after
    );

    goto_priv(PRIV_HS);
    ai_micro_cross_page_fence_i();
    ai_micro_cross_page_clear_mcontrol6_slot(0);
    ai_micro_cross_page_clear_mcontrol6_slot(1);
    ai_micro_cross_page_arm_mcontrol6_slot(0, code_vaddr,
        AI_MICRO_CROSS_PAGE_MCONTROL6_EXECUTE);
    saved_mstatus = ai_micro_cross_page_enable_machine_breakpoint_delivery();
    goto_priv(PRIV_HS);

    TEST_SETUP_EXCEPT();
    fetch_func();
    TEST_ASSERT("after the mixed vector-then-scalar cross-page rewrite and fence.i, the first fetch still reports breakpoint before the later non-canonical fetch page fault is exposed",
        excpt.triggered == true &&
        excpt.cause == CAUSE_BKP
    );

    goto_priv(PRIV_M);
    TEST_ASSERT("the first trapped fetch in the mixed vector-then-scalar cross-page execute-trigger plus non-canonical corner still leaves the final boundary bytes and the outer sentinels unchanged",
        ai_micro_cross_page_trigger_bytes_match(low_phys_start, expected_final, 8U) &&
        ai_micro_cross_page_trigger_bytes_match(high_phys_start, expected_final + 8U, 8U) &&
        read8(sentinel_before_addr) == sentinel_before &&
        read8(sentinel_after_addr) == sentinel_after
    );
    ai_micro_cross_page_restore_machine_status(saved_mstatus);

    saved_mstatus = ai_micro_cross_page_enable_machine_breakpoint_delivery();
    goto_priv(PRIV_HS);
    TEST_SETUP_EXCEPT();
    fetch_func();
    TEST_ASSERT("after the mixed vector-then-scalar cross-page rewrite and fence.i, the second fetch still reports breakpoint while the execute-PC trigger remains armed before the later non-canonical fetch page fault is exposed",
        excpt.triggered == true &&
        excpt.cause == CAUSE_BKP
    );

    goto_priv(PRIV_M);
    TEST_ASSERT("the second trapped fetch in the mixed vector-then-scalar cross-page execute-trigger plus non-canonical corner still leaves the final boundary bytes and the outer sentinels unchanged",
        ai_micro_cross_page_trigger_bytes_match(low_phys_start, expected_final, 8U) &&
        ai_micro_cross_page_trigger_bytes_match(high_phys_start, expected_final + 8U, 8U) &&
        read8(sentinel_before_addr) == sentinel_before &&
        read8(sentinel_after_addr) == sentinel_after
    );
    ai_micro_cross_page_restore_machine_status(saved_mstatus);
    ai_micro_cross_page_clear_mcontrol6_slot(0);

    goto_priv(PRIV_HS);
    ai_micro_cross_page_fence_i();
    TEST_SETUP_EXCEPT();
    TEST_EXEC_EXCEPT(bad_vaddr);
    TEST_ASSERT("after clearing the execute-PC trigger, the same mixed vector-then-scalar rewritten two-page executable HS block falls back to instruction page fault for the non-canonical fetch target with the raw VA in tval",
        excpt.triggered == true &&
        excpt.cause == CAUSE_IPF &&
        excpt.tval == bad_vaddr
    );

    goto_priv(PRIV_M);
    TEST_ASSERT("after repeated execute-trigger breakpoint hits and the later non-canonical fetch page fault, the final boundary bytes remain intact and the outer sentinels stay unchanged",
        ai_micro_cross_page_trigger_bytes_match(low_phys_start, expected_final, 8U) &&
        ai_micro_cross_page_trigger_bytes_match(high_phys_start, expected_final + 8U, 8U) &&
        read8(sentinel_before_addr) == sentinel_before &&
        read8(sentinel_after_addr) == sentinel_after
    );

    goto_priv(PRIV_HS);
    TEST_SETUP_EXCEPT();
    observed = fetch_func();
    TEST_ASSERT("after the mixed vector-then-scalar cross-page execute-trigger plus non-canonical fetch fault sequence, a subsequent canonical fetch still executes the final scalar-overwritten instruction stream",
        excpt.triggered == false &&
        observed == 9ULL
    );

    goto_priv(PRIV_M);
    TEST_ASSERT("after canonical recovery from the mixed vector-then-scalar cross-page execute-trigger plus non-canonical fetch corner, the final boundary bytes remain intact and the outer sentinels stay unchanged",
        ai_micro_cross_page_trigger_bytes_match(low_phys_start, expected_final, 8U) &&
        ai_micro_cross_page_trigger_bytes_match(high_phys_start, expected_final + 8U, 8U) &&
        read8(sentinel_before_addr) == sentinel_before &&
        read8(sentinel_after_addr) == sentinel_after
    );

    goto_priv(PRIV_HS);
    hspt_init();
    sfence_vma();

    TEST_END();
}

bool ai_micro_hs_fetch_after_scalar_cross_page_store_after_vector_cross_page_store_fence_i_execute_trigger_repeated_breakpoint_then_all_ones_fetch_page_fault_adjacent_preserved_corner()
{
    TEST_START();

    enum test_page low_page = VSRWX_GRWX;
    enum test_page high_page = VSRWX_GRW;
    const uint8_t warm_pattern[8] = {
        0x13, 0x05, 0x30, 0x00,
        0x67, 0x80, 0x00, 0x00
    };
    const uint8_t vector_pattern[16] = {
        0x30, 0x31, 0x32, 0x33,
        0x13, 0x05, 0x50, 0x00,
        0x67, 0x80, 0x00, 0x00,
        0x3c, 0x3d, 0x3e, 0x3f
    };
    const uint8_t expected_final[16] = {
        0x30, 0x31, 0x32, 0x33,
        0x13, 0x05, 0x90, 0x00,
        0x67, 0x80, 0x00, 0x00,
        0x3c, 0x3d, 0x3e, 0x3f
    };
    const uintptr_t source_vaddr = hs_page_base(VSRWX_GRWX) + 0x2c0U;
    const uintptr_t source_phys = phys_page_base(VSRWX_GRWX) + 0x2c0U;
    const uintptr_t boundary_vaddr = ai_micro_cross_page_trigger_addr(low_page, 8U);
    const uintptr_t code_vaddr = ai_micro_cross_page_trigger_addr(low_page, 4U);
    const uintptr_t bad_vaddr = (uintptr_t)-1;
    const uintptr_t fetch_target = bad_vaddr & ~1ULL;
    const uintptr_t low_phys_start = phys_page_base(low_page) + PAGE_SIZE - 8U;
    const uintptr_t code_phys_start = phys_page_base(low_page) + PAGE_SIZE - 4U;
    const uintptr_t high_phys_start = phys_page_base(high_page);
    const uintptr_t sentinel_before_addr = low_phys_start - 1U;
    const uintptr_t sentinel_after_addr = high_phys_start + 8U;
    const uint8_t sentinel_before = 0x78U;
    const uint8_t sentinel_after = 0xa8U;
    uint64_t (*fetch_func)(void) = (uint64_t (*)(void))code_vaddr;
    uint64_t observed = 0;
    uint64_t saved_mstatus = 0;

    goto_priv(PRIV_M);
    ai_micro_cross_page_trigger_write_bytes(code_phys_start, warm_pattern, 8U);
    ai_micro_cross_page_trigger_write_bytes(source_phys, vector_pattern, 16U);
    write8(sentinel_before_addr, sentinel_before);
    write8(sentinel_after_addr, sentinel_after);

    goto_priv(PRIV_HS);
    hspt_init();

    goto_priv(PRIV_M);
    ai_micro_cross_page_set_hs_leaf(low_page, phys_page_base(low_page),
        PTE_V | PTE_AD | PTE_RWX);
    ai_micro_cross_page_set_hs_leaf(high_page, phys_page_base(high_page),
        PTE_V | PTE_AD | PTE_RWX);
    sfence_vma();

    goto_priv(PRIV_HS);
    ai_micro_cross_page_fence_i();

    TEST_SETUP_EXCEPT();
    observed = fetch_func();
    TEST_ASSERT("before the mixed vector-then-scalar cross-page execute-trigger plus all-ones fetch probe starts, the warmed fetch path still returns the original value",
        excpt.triggered == false &&
        observed == 3ULL
    );

    TEST_SETUP_EXCEPT();
    vle8_to_v6((const uint8_t *)source_vaddr, 16);
    TEST_ASSERT("a vector unit-stride load can stage the first cross-page instruction payload before the mixed execute-trigger plus all-ones code-rewrite probe",
        excpt.triggered == false
    );

    TEST_SETUP_EXCEPT();
    vse8_from_v6((uint8_t *)boundary_vaddr, 16);
    TEST_ASSERT("a vector unit-stride store can rewrite the shared two-page executable HS boundary before the overlapping scalar cross-page execute-trigger plus all-ones code rewrite",
        excpt.triggered == false
    );

    goto_priv(PRIV_M);
    TEST_ASSERT("after the first vector cross-page execute-trigger plus all-ones code-rewrite step, the staged boundary bytes are visible across both backing pages and the outer sentinels stay unchanged before the scalar overwrite",
        ai_micro_cross_page_trigger_bytes_match(low_phys_start, vector_pattern, 8U) &&
        ai_micro_cross_page_trigger_bytes_match(high_phys_start, vector_pattern + 8U, 8U) &&
        read8(sentinel_before_addr) == sentinel_before &&
        read8(sentinel_after_addr) == sentinel_after
    );

    goto_priv(PRIV_HS);
    TEST_SETUP_EXCEPT();
    sd(code_vaddr, 0x0000806700900513ULL);
    TEST_ASSERT("after the vector rewrite, an overlapping scalar cross-page store can overwrite the executable HS block to the final instruction stream before the execute-trigger plus all-ones fetch probe",
        excpt.triggered == false
    );

    goto_priv(PRIV_M);
    TEST_ASSERT("after the mixed vector-then-scalar cross-page execute-trigger plus all-ones code rewrite sequence, the final boundary bytes match the scalar-overwritten instruction stream and the outer sentinels stay unchanged before fence.i",
        ai_micro_cross_page_trigger_bytes_match(low_phys_start, expected_final, 8U) &&
        ai_micro_cross_page_trigger_bytes_match(high_phys_start, expected_final + 8U, 8U) &&
        read8(sentinel_before_addr) == sentinel_before &&
        read8(sentinel_after_addr) == sentinel_after
    );

    goto_priv(PRIV_HS);
    ai_micro_cross_page_fence_i();
    ai_micro_cross_page_clear_mcontrol6_slot(0);
    ai_micro_cross_page_clear_mcontrol6_slot(1);
    ai_micro_cross_page_arm_mcontrol6_slot(0, code_vaddr,
        AI_MICRO_CROSS_PAGE_MCONTROL6_EXECUTE);
    saved_mstatus = ai_micro_cross_page_enable_machine_breakpoint_delivery();
    goto_priv(PRIV_HS);

    TEST_SETUP_EXCEPT();
    fetch_func();
    TEST_ASSERT("after the mixed vector-then-scalar cross-page rewrite and fence.i, the first fetch still reports breakpoint before the later all-ones fetch page fault is exposed",
        excpt.triggered == true &&
        excpt.cause == CAUSE_BKP
    );

    goto_priv(PRIV_M);
    TEST_ASSERT("the first trapped fetch in the mixed vector-then-scalar cross-page execute-trigger plus all-ones corner still leaves the final boundary bytes and the outer sentinels unchanged",
        ai_micro_cross_page_trigger_bytes_match(low_phys_start, expected_final, 8U) &&
        ai_micro_cross_page_trigger_bytes_match(high_phys_start, expected_final + 8U, 8U) &&
        read8(sentinel_before_addr) == sentinel_before &&
        read8(sentinel_after_addr) == sentinel_after
    );
    ai_micro_cross_page_restore_machine_status(saved_mstatus);

    saved_mstatus = ai_micro_cross_page_enable_machine_breakpoint_delivery();
    goto_priv(PRIV_HS);
    TEST_SETUP_EXCEPT();
    fetch_func();
    TEST_ASSERT("after the mixed vector-then-scalar cross-page rewrite and fence.i, the second fetch still reports breakpoint while the execute-PC trigger remains armed before the later all-ones fetch page fault is exposed",
        excpt.triggered == true &&
        excpt.cause == CAUSE_BKP
    );

    goto_priv(PRIV_M);
    TEST_ASSERT("the second trapped fetch in the mixed vector-then-scalar cross-page execute-trigger plus all-ones corner still leaves the final boundary bytes and the outer sentinels unchanged",
        ai_micro_cross_page_trigger_bytes_match(low_phys_start, expected_final, 8U) &&
        ai_micro_cross_page_trigger_bytes_match(high_phys_start, expected_final + 8U, 8U) &&
        read8(sentinel_before_addr) == sentinel_before &&
        read8(sentinel_after_addr) == sentinel_after
    );
    ai_micro_cross_page_restore_machine_status(saved_mstatus);
    ai_micro_cross_page_clear_mcontrol6_slot(0);

    goto_priv(PRIV_HS);
    ai_micro_cross_page_fence_i();
    TEST_SETUP_EXCEPT();
    TEST_EXEC_EXCEPT(bad_vaddr);
    TEST_ASSERT("after clearing the execute-PC trigger, the same mixed vector-then-scalar rewritten two-page executable HS block falls back to instruction page fault for the all-ones fetch target with the JALR-cleared target in tval",
        excpt.triggered == true &&
        excpt.cause == CAUSE_IPF &&
        excpt.tval == fetch_target
    );

    goto_priv(PRIV_M);
    TEST_ASSERT("after repeated execute-trigger breakpoint hits and the later all-ones fetch page fault, the final boundary bytes remain intact and the outer sentinels stay unchanged",
        ai_micro_cross_page_trigger_bytes_match(low_phys_start, expected_final, 8U) &&
        ai_micro_cross_page_trigger_bytes_match(high_phys_start, expected_final + 8U, 8U) &&
        read8(sentinel_before_addr) == sentinel_before &&
        read8(sentinel_after_addr) == sentinel_after
    );

    goto_priv(PRIV_HS);
    TEST_SETUP_EXCEPT();
    observed = fetch_func();
    TEST_ASSERT("after the mixed vector-then-scalar cross-page execute-trigger plus all-ones fetch fault sequence, a subsequent canonical fetch still executes the final scalar-overwritten instruction stream",
        excpt.triggered == false &&
        observed == 9ULL
    );

    goto_priv(PRIV_M);
    TEST_ASSERT("after canonical recovery from the mixed vector-then-scalar cross-page execute-trigger plus all-ones fetch corner, the final boundary bytes remain intact and the outer sentinels stay unchanged",
        ai_micro_cross_page_trigger_bytes_match(low_phys_start, expected_final, 8U) &&
        ai_micro_cross_page_trigger_bytes_match(high_phys_start, expected_final + 8U, 8U) &&
        read8(sentinel_before_addr) == sentinel_before &&
        read8(sentinel_after_addr) == sentinel_after
    );

    goto_priv(PRIV_HS);
    hspt_init();
    sfence_vma();

    TEST_END();
}

bool ai_micro_hs_load_cross_page_dual_address_trigger_only_original_matches_spike_corner()
{
    TEST_START();

    uintptr_t vaddr = ai_micro_cross_page_trigger_addr(VSRWX_GRX, 4U);
    uintptr_t second_half_vaddr = vaddr + 4U;
    uintptr_t low_phys = ai_micro_cross_page_trigger_page_end_phys(VSRWX_GRX);
    uintptr_t high_phys = phys_page_base(VSRWX_GR);
    const uint64_t low_pattern = 0x1122334455667788ULL;
    const uint64_t high_pattern = 0xaabbccdd00112233ULL;

    goto_priv(PRIV_M);
    write64(low_phys, low_pattern);
    write64(high_phys, high_pattern);

    goto_priv(PRIV_HS);
    hspt_init();
    sfence_vma();

    ai_micro_cross_page_clear_mcontrol6_slot(0);
    ai_micro_cross_page_clear_mcontrol6_slot(1);
    ai_micro_cross_page_arm_mcontrol6_slot(0, vaddr,
        AI_MICRO_CROSS_PAGE_MCONTROL6_LOAD);
    ai_micro_cross_page_arm_mcontrol6_slot(1, second_half_vaddr,
        AI_MICRO_CROSS_PAGE_MCONTROL6_LOAD);
    uint64_t saved_mstatus = ai_micro_cross_page_enable_machine_breakpoint_delivery();

    goto_priv(PRIV_HS);
    TEST_SETUP_EXCEPT();
    ai_micro_cross_page_execute_load_once(vaddr);
    TEST_ASSERT("with both the original cross-page misaligned address slot and the true second-half-start slot armed, Spike still reports breakpoint because only the original split-load address matches",
        excpt.triggered == true &&
        excpt.cause == CAUSE_BKP
    );
    ai_micro_cross_page_restore_machine_status(saved_mstatus);

    ai_micro_cross_page_clear_mcontrol6_slot(0);
    saved_mstatus = ai_micro_cross_page_enable_machine_breakpoint_delivery();

    goto_priv(PRIV_HS);
    TEST_SETUP_EXCEPT();
    uint64_t value = ai_micro_cross_page_execute_load_once(vaddr);
    TEST_ASSERT("after clearing the original cross-page misaligned-address slot, the remaining second-half-start slot stays ignored on Spike and the split load completes normally",
        excpt.triggered == false &&
        value == 0x0011223311223344ULL
    );
    ai_micro_cross_page_restore_machine_status(saved_mstatus);

    ai_micro_cross_page_clear_mcontrol6_slot(1);
    TEST_END();
}

bool ai_micro_hs_load_cross_page_dual_address_trigger_repeated_breakpoint_then_completion_spike_corner()
{
    TEST_START();

    uintptr_t vaddr = ai_micro_cross_page_trigger_addr(VSRWX_GRX, 4U);
    uintptr_t second_half_vaddr = vaddr + 4U;
    uintptr_t low_phys = ai_micro_cross_page_trigger_page_end_phys(VSRWX_GRX);
    uintptr_t high_phys = phys_page_base(VSRWX_GR);
    const uint64_t low_pattern = 0x1122334455667788ULL;
    const uint64_t high_pattern = 0xaabbccdd00112233ULL;
    uint64_t value;

    goto_priv(PRIV_M);
    write64(low_phys, low_pattern);
    write64(high_phys, high_pattern);

    goto_priv(PRIV_HS);
    hspt_init();
    sfence_vma();

    ai_micro_cross_page_clear_mcontrol6_slot(0);
    ai_micro_cross_page_clear_mcontrol6_slot(1);
    ai_micro_cross_page_arm_mcontrol6_slot(0, vaddr,
        AI_MICRO_CROSS_PAGE_MCONTROL6_LOAD);
    ai_micro_cross_page_arm_mcontrol6_slot(1, second_half_vaddr,
        AI_MICRO_CROSS_PAGE_MCONTROL6_LOAD);
    uint64_t saved_mstatus = ai_micro_cross_page_enable_machine_breakpoint_delivery();

    goto_priv(PRIV_HS);
    TEST_SETUP_EXCEPT();
    ai_micro_cross_page_execute_load_once(vaddr);
    TEST_ASSERT("with both split-load address slots armed, the first cross-page load still reports breakpoint on Spike because only the original misaligned vaddr matches",
        excpt.triggered == true &&
        excpt.cause == CAUSE_BKP
    );
    ai_micro_cross_page_restore_machine_status(saved_mstatus);

    saved_mstatus = ai_micro_cross_page_enable_machine_breakpoint_delivery();
    goto_priv(PRIV_HS);
    TEST_SETUP_EXCEPT();
    ai_micro_cross_page_execute_load_once(vaddr);
    TEST_ASSERT("repeating the same cross-page load with both address slots still armed reports breakpoint again and does not let the ignored second-half slot start matching later",
        excpt.triggered == true &&
        excpt.cause == CAUSE_BKP
    );
    ai_micro_cross_page_restore_machine_status(saved_mstatus);

    ai_micro_cross_page_clear_mcontrol6_slot(0);
    saved_mstatus = ai_micro_cross_page_enable_machine_breakpoint_delivery();

    goto_priv(PRIV_HS);
    TEST_SETUP_EXCEPT();
    value = ai_micro_cross_page_execute_load_once(vaddr);
    TEST_ASSERT("after two prior breakpoint hits and then clearing only the original-address slot, the remaining second-half-start slot still stays ignored on Spike and the split load completes normally",
        excpt.triggered == false &&
        value == 0x0011223311223344ULL
    );
    ai_micro_cross_page_restore_machine_status(saved_mstatus);

    ai_micro_cross_page_clear_mcontrol6_slot(1);
    TEST_END();
}

bool ai_micro_hs_store_cross_page_second_half_address_trigger_ignored_spike_corner()
{
    TEST_START();

    uintptr_t vaddr = ai_micro_cross_page_trigger_addr(VSRW_GRW, 4U);
    uintptr_t second_half_vaddr = vaddr + 4U;
    uintptr_t low_phys = ai_micro_cross_page_trigger_page_end_phys(VSRW_GRW);
    uintptr_t high_phys = phys_page_base(VSRW_GRX);
    const uint64_t low_pattern = 0x1122334455667788ULL;
    const uint64_t high_pattern = 0xaabbccdd00112233ULL;
    bool trap_triggered;

    goto_priv(PRIV_M);
    write64(low_phys, low_pattern);
    write64(high_phys, high_pattern);

    goto_priv(PRIV_HS);
    hspt_init();
    sfence_vma();

    ai_micro_cross_page_clear_mcontrol6_slot(0);
    ai_micro_cross_page_clear_mcontrol6_slot(1);
    ai_micro_cross_page_arm_mcontrol6_slot(0, second_half_vaddr,
        AI_MICRO_CROSS_PAGE_MCONTROL6_STORE);
    uint64_t saved_mstatus = ai_micro_cross_page_enable_machine_breakpoint_delivery();

    goto_priv(PRIV_HS);
    TEST_SETUP_EXCEPT();
    ai_micro_cross_page_execute_store_once(vaddr, 0x0123456789abcdefULL);
    trap_triggered = excpt.triggered;
    ai_micro_cross_page_restore_machine_status(saved_mstatus);

    goto_priv(PRIV_M);
    TEST_ASSERT("on Spike, arming a split-store address trigger at the true second-half start address does not match; the cross-page store commits both fragments normally",
        trap_triggered == false &&
        read64(low_phys) == 0x89abcdef55667788ULL &&
        read64(high_phys) == 0xaabbccdd01234567ULL
    );

    ai_micro_cross_page_clear_mcontrol6_slot(0);
    TEST_END();
}

bool ai_micro_hs_store_cross_page_dual_address_trigger_only_original_matches_spike_corner()
{
    TEST_START();

    uintptr_t vaddr = ai_micro_cross_page_trigger_addr(VSRW_GRW, 4U);
    uintptr_t second_half_vaddr = vaddr + 4U;
    uintptr_t low_phys = ai_micro_cross_page_trigger_page_end_phys(VSRW_GRW);
    uintptr_t high_phys = phys_page_base(VSRW_GRX);
    const uint64_t low_pattern = 0x1122334455667788ULL;
    const uint64_t high_pattern = 0xaabbccdd00112233ULL;
    bool trap_triggered;

    goto_priv(PRIV_M);
    write64(low_phys, low_pattern);
    write64(high_phys, high_pattern);

    goto_priv(PRIV_HS);
    hspt_init();
    sfence_vma();

    ai_micro_cross_page_clear_mcontrol6_slot(0);
    ai_micro_cross_page_clear_mcontrol6_slot(1);
    ai_micro_cross_page_arm_mcontrol6_slot(0, vaddr,
        AI_MICRO_CROSS_PAGE_MCONTROL6_STORE);
    ai_micro_cross_page_arm_mcontrol6_slot(1, second_half_vaddr,
        AI_MICRO_CROSS_PAGE_MCONTROL6_STORE);
    uint64_t saved_mstatus = ai_micro_cross_page_enable_machine_breakpoint_delivery();

    goto_priv(PRIV_HS);
    TEST_SETUP_EXCEPT();
    ai_micro_cross_page_execute_store_once(vaddr, 0x0123456789abcdefULL);
    trap_triggered = excpt.triggered;
    ai_micro_cross_page_restore_machine_status(saved_mstatus);

    goto_priv(PRIV_M);
    TEST_ASSERT("with both the original cross-page misaligned store address slot and the true second-half-start slot armed, Spike still reports breakpoint before any split-store bytes commit because only the original address matches",
        trap_triggered == true &&
        read64(low_phys) == low_pattern &&
        read64(high_phys) == high_pattern
    );

    ai_micro_cross_page_clear_mcontrol6_slot(0);
    saved_mstatus = ai_micro_cross_page_enable_machine_breakpoint_delivery();

    goto_priv(PRIV_HS);
    TEST_SETUP_EXCEPT();
    ai_micro_cross_page_execute_store_once(vaddr, 0x0123456789abcdefULL);
    trap_triggered = excpt.triggered;
    ai_micro_cross_page_restore_machine_status(saved_mstatus);

    goto_priv(PRIV_M);
    TEST_ASSERT("after clearing the original cross-page misaligned-address slot, the remaining second-half-start store slot stays ignored on Spike and the split store commits normally",
        trap_triggered == false &&
        read64(low_phys) == 0x89abcdef55667788ULL &&
        read64(high_phys) == 0xaabbccdd01234567ULL
    );

    ai_micro_cross_page_clear_mcontrol6_slot(1);
    TEST_END();
}

bool ai_micro_hs_store_cross_page_dual_address_trigger_repeated_breakpoint_then_completion_spike_corner()
{
    TEST_START();

    uintptr_t vaddr = ai_micro_cross_page_trigger_addr(VSRW_GRW, 4U);
    uintptr_t second_half_vaddr = vaddr + 4U;
    uintptr_t low_phys = ai_micro_cross_page_trigger_page_end_phys(VSRW_GRW);
    uintptr_t high_phys = phys_page_base(VSRW_GRX);
    const uint64_t low_pattern = 0x1122334455667788ULL;
    const uint64_t high_pattern = 0xaabbccdd00112233ULL;
    bool trap_triggered;

    goto_priv(PRIV_M);
    write64(low_phys, low_pattern);
    write64(high_phys, high_pattern);

    goto_priv(PRIV_HS);
    hspt_init();
    sfence_vma();

    ai_micro_cross_page_clear_mcontrol6_slot(0);
    ai_micro_cross_page_clear_mcontrol6_slot(1);
    ai_micro_cross_page_arm_mcontrol6_slot(0, vaddr,
        AI_MICRO_CROSS_PAGE_MCONTROL6_STORE);
    ai_micro_cross_page_arm_mcontrol6_slot(1, second_half_vaddr,
        AI_MICRO_CROSS_PAGE_MCONTROL6_STORE);
    uint64_t saved_mstatus = ai_micro_cross_page_enable_machine_breakpoint_delivery();

    goto_priv(PRIV_HS);
    TEST_SETUP_EXCEPT();
    ai_micro_cross_page_execute_store_once(vaddr, 0x0123456789abcdefULL);
    trap_triggered = excpt.triggered;
    ai_micro_cross_page_restore_machine_status(saved_mstatus);

    goto_priv(PRIV_M);
    TEST_ASSERT("with both split-store address slots armed, the first cross-page store still reports breakpoint on Spike before either fragment commits because only the original misaligned vaddr matches",
        trap_triggered == true &&
        read64(low_phys) == low_pattern &&
        read64(high_phys) == high_pattern
    );

    saved_mstatus = ai_micro_cross_page_enable_machine_breakpoint_delivery();
    goto_priv(PRIV_HS);
    TEST_SETUP_EXCEPT();
    ai_micro_cross_page_execute_store_once(vaddr, 0x0123456789abcdefULL);
    trap_triggered = excpt.triggered;
    ai_micro_cross_page_restore_machine_status(saved_mstatus);

    goto_priv(PRIV_M);
    TEST_ASSERT("repeating the same cross-page store with both address slots still armed reports breakpoint again and still leaves both page fragments untouched",
        trap_triggered == true &&
        read64(low_phys) == low_pattern &&
        read64(high_phys) == high_pattern
    );

    ai_micro_cross_page_clear_mcontrol6_slot(0);
    saved_mstatus = ai_micro_cross_page_enable_machine_breakpoint_delivery();

    goto_priv(PRIV_HS);
    TEST_SETUP_EXCEPT();
    ai_micro_cross_page_execute_store_once(vaddr, 0x0123456789abcdefULL);
    trap_triggered = excpt.triggered;
    ai_micro_cross_page_restore_machine_status(saved_mstatus);

    goto_priv(PRIV_M);
    TEST_ASSERT("after two prior breakpoint hits and then clearing only the original-address slot, the remaining second-half-start store slot still stays ignored on Spike and the split store commits normally",
        trap_triggered == false &&
        read64(low_phys) == 0x89abcdef55667788ULL &&
        read64(high_phys) == 0xaabbccdd01234567ULL
    );

    ai_micro_cross_page_clear_mcontrol6_slot(1);
    TEST_END();
}

bool ai_micro_hs_load_cross_page_execute_and_address_trigger_both_breakpoint_corner()
{
    TEST_START();

    uintptr_t vaddr = ai_micro_cross_page_trigger_addr(VSRWX_GRX, 4U);
    uintptr_t low_phys = ai_micro_cross_page_trigger_page_end_phys(VSRWX_GRX);
    uintptr_t high_phys = phys_page_base(VSRWX_GR);
    uintptr_t trigger_pc = (uintptr_t)ai_micro_cross_page_execute_load_once;
    const uint64_t low_pattern = 0x1122334455667788ULL;
    const uint64_t high_pattern = 0xaabbccdd00112233ULL;

    goto_priv(PRIV_M);
    write64(low_phys, low_pattern);
    write64(high_phys, high_pattern);

    goto_priv(PRIV_HS);
    hspt_init();
    sfence_vma();

    ai_micro_cross_page_clear_mcontrol6_slot(0);
    ai_micro_cross_page_clear_mcontrol6_slot(1);
    ai_micro_cross_page_arm_mcontrol6_slot(0, trigger_pc,
        AI_MICRO_CROSS_PAGE_MCONTROL6_EXECUTE);
    ai_micro_cross_page_arm_mcontrol6_slot(1, vaddr,
        AI_MICRO_CROSS_PAGE_MCONTROL6_LOAD);
    uint64_t saved_mstatus = ai_micro_cross_page_enable_machine_breakpoint_delivery();

    goto_priv(PRIV_HS);
    TEST_SETUP_EXCEPT();
    ai_micro_cross_page_execute_load_once(vaddr);
    TEST_ASSERT("an HS cross-page load still reports breakpoint when both execute-PC and address trigger slots match the same no-fault split load",
        excpt.triggered == true &&
        excpt.cause == CAUSE_BKP
    );
    ai_micro_cross_page_restore_machine_status(saved_mstatus);

    ai_micro_cross_page_clear_mcontrol6_slot(0);
    saved_mstatus = ai_micro_cross_page_enable_machine_breakpoint_delivery();

    goto_priv(PRIV_HS);
    TEST_SETUP_EXCEPT();
    ai_micro_cross_page_execute_load_once(vaddr);
    TEST_ASSERT("after clearing only the execute-PC slot, the remaining cross-page load address trigger still reports breakpoint on the same no-fault split path",
        excpt.triggered == true &&
        excpt.cause == CAUSE_BKP
    );
    ai_micro_cross_page_restore_machine_status(saved_mstatus);

    ai_micro_cross_page_clear_mcontrol6_slot(1);
    goto_priv(PRIV_HS);
    TEST_SETUP_EXCEPT();
    uint64_t value = ai_micro_cross_page_execute_load_once(vaddr);
    TEST_ASSERT("after clearing both trigger slots, the same HS cross-page split load completes normally and still returns the merged payload",
        excpt.triggered == false &&
        value == 0x0011223311223344ULL
    );

    TEST_END();
}

bool ai_micro_hs_load_cross_page_execute_and_address_trigger_repeated_breakpoint_then_completion_corner()
{
    TEST_START();

    uintptr_t vaddr = ai_micro_cross_page_trigger_addr(VSRWX_GRX, 4U);
    uintptr_t low_phys = ai_micro_cross_page_trigger_page_end_phys(VSRWX_GRX);
    uintptr_t high_phys = phys_page_base(VSRWX_GR);
    uintptr_t trigger_pc = (uintptr_t)ai_micro_cross_page_execute_load_once;
    const uint64_t low_pattern = 0x1122334455667788ULL;
    const uint64_t high_pattern = 0xaabbccdd00112233ULL;

    goto_priv(PRIV_M);
    write64(low_phys, low_pattern);
    write64(high_phys, high_pattern);

    goto_priv(PRIV_HS);
    hspt_init();
    sfence_vma();

    ai_micro_cross_page_clear_mcontrol6_slot(0);
    ai_micro_cross_page_clear_mcontrol6_slot(1);
    ai_micro_cross_page_arm_mcontrol6_slot(0, trigger_pc,
        AI_MICRO_CROSS_PAGE_MCONTROL6_EXECUTE);
    ai_micro_cross_page_arm_mcontrol6_slot(1, vaddr,
        AI_MICRO_CROSS_PAGE_MCONTROL6_LOAD);
    uint64_t saved_mstatus = ai_micro_cross_page_enable_machine_breakpoint_delivery();

    goto_priv(PRIV_HS);
    TEST_SETUP_EXCEPT();
    ai_micro_cross_page_execute_load_once(vaddr);
    TEST_ASSERT("with both execute-PC and address trigger slots armed on the same cross-page no-fault split load, the first access reports breakpoint",
        excpt.triggered == true &&
        excpt.cause == CAUSE_BKP
    );

    goto_priv(PRIV_M);
    TEST_ASSERT("the first trapped cross-page no-fault split load still leaves both page fragments unchanged",
        read64(low_phys) == low_pattern &&
        read64(high_phys) == high_pattern
    );
    ai_micro_cross_page_restore_machine_status(saved_mstatus);

    saved_mstatus = ai_micro_cross_page_enable_machine_breakpoint_delivery();
    goto_priv(PRIV_HS);
    TEST_SETUP_EXCEPT();
    ai_micro_cross_page_execute_load_once(vaddr);
    TEST_ASSERT("with both execute-PC and address trigger slots still armed on that same cross-page no-fault split load, the second access still reports breakpoint",
        excpt.triggered == true &&
        excpt.cause == CAUSE_BKP
    );

    goto_priv(PRIV_M);
    TEST_ASSERT("the second trapped cross-page no-fault split load still leaves both page fragments unchanged",
        read64(low_phys) == low_pattern &&
        read64(high_phys) == high_pattern
    );
    ai_micro_cross_page_restore_machine_status(saved_mstatus);

    ai_micro_cross_page_clear_mcontrol6_slot(0);
    saved_mstatus = ai_micro_cross_page_enable_machine_breakpoint_delivery();
    goto_priv(PRIV_HS);
    TEST_SETUP_EXCEPT();
    ai_micro_cross_page_execute_load_once(vaddr);
    TEST_ASSERT("after two prior dual-slot breakpoint hits, clearing only the execute-PC slot still leaves the cross-page load address trigger active",
        excpt.triggered == true &&
        excpt.cause == CAUSE_BKP
    );

    goto_priv(PRIV_M);
    TEST_ASSERT("with only the address trigger slot armed, the trapped cross-page no-fault split load still leaves both page fragments unchanged",
        read64(low_phys) == low_pattern &&
        read64(high_phys) == high_pattern
    );
    ai_micro_cross_page_restore_machine_status(saved_mstatus);

    ai_micro_cross_page_clear_mcontrol6_slot(1);
    goto_priv(PRIV_HS);
    TEST_SETUP_EXCEPT();
    uint64_t value = ai_micro_cross_page_execute_load_once(vaddr);
    TEST_ASSERT("after clearing both trigger slots, the same cross-page split load completes normally and still returns the merged payload",
        excpt.triggered == false &&
        value == 0x0011223311223344ULL
    );

    TEST_END();
}

bool ai_micro_hs_load_cross_page_execute_and_address_trigger_repeated_breakpoint_then_completion_adjacent_preserved_corner()
{
    TEST_START();

    enum test_page low_page = VSRWX_GRX;
    enum test_page high_page = VSRWX_GR;
    uintptr_t vaddr = ai_micro_cross_page_trigger_addr(low_page, 4U);
    uintptr_t low_adjacent_phys = phys_page_base(low_page) + PAGE_SIZE - (2 * sizeof(uint64_t));
    uintptr_t low_phys = ai_micro_cross_page_trigger_page_end_phys(low_page);
    uintptr_t high_phys = phys_page_base(high_page);
    uintptr_t high_adjacent_phys = high_phys + sizeof(uint64_t);
    uintptr_t trigger_pc = (uintptr_t)ai_micro_cross_page_execute_load_once;
    const uint64_t low_adjacent = 0x55aa55aa33cc33ccULL;
    const uint64_t low_pattern = 0x1122334455667788ULL;
    const uint64_t high_pattern = 0xaabbccdd00112233ULL;
    const uint64_t high_adjacent = 0xcc33cc3355aa55aaULL;
    uint64_t saved_mstatus;

    goto_priv(PRIV_M);
    write64(low_adjacent_phys, low_adjacent);
    write64(low_phys, low_pattern);
    write64(high_phys, high_pattern);
    write64(high_adjacent_phys, high_adjacent);

    goto_priv(PRIV_HS);
    hspt_init();
    sfence_vma();

    ai_micro_cross_page_clear_mcontrol6_slot(0);
    ai_micro_cross_page_clear_mcontrol6_slot(1);
    ai_micro_cross_page_arm_mcontrol6_slot(0, trigger_pc,
        AI_MICRO_CROSS_PAGE_MCONTROL6_EXECUTE);
    ai_micro_cross_page_arm_mcontrol6_slot(1, vaddr,
        AI_MICRO_CROSS_PAGE_MCONTROL6_LOAD);
    saved_mstatus = ai_micro_cross_page_enable_machine_breakpoint_delivery();

    goto_priv(PRIV_HS);
    TEST_SETUP_EXCEPT();
    ai_micro_cross_page_execute_load_once(vaddr);
    TEST_ASSERT("with both execute-PC and address trigger slots armed on the same no-fault cross-page split load, the first access still reports breakpoint in the adjacent-preservation corner",
        excpt.triggered == true &&
        excpt.cause == CAUSE_BKP
    );

    goto_priv(PRIV_M);
    TEST_ASSERT("the first trapped no-fault cross-page split-load probe still leaves both boundary doublewords and their adjacent neighbors unchanged",
        read64(low_adjacent_phys) == low_adjacent &&
        read64(low_phys) == low_pattern &&
        read64(high_phys) == high_pattern &&
        read64(high_adjacent_phys) == high_adjacent
    );
    ai_micro_cross_page_restore_machine_status(saved_mstatus);

    saved_mstatus = ai_micro_cross_page_enable_machine_breakpoint_delivery();
    goto_priv(PRIV_HS);
    TEST_SETUP_EXCEPT();
    ai_micro_cross_page_execute_load_once(vaddr);
    TEST_ASSERT("with both execute-PC and address trigger slots still armed on that same no-fault cross-page split load, the second access still reports breakpoint in the adjacent-preservation corner",
        excpt.triggered == true &&
        excpt.cause == CAUSE_BKP
    );

    goto_priv(PRIV_M);
    TEST_ASSERT("the second trapped no-fault cross-page split-load probe still leaves both boundary doublewords and their adjacent neighbors unchanged",
        read64(low_adjacent_phys) == low_adjacent &&
        read64(low_phys) == low_pattern &&
        read64(high_phys) == high_pattern &&
        read64(high_adjacent_phys) == high_adjacent
    );
    ai_micro_cross_page_restore_machine_status(saved_mstatus);

    ai_micro_cross_page_clear_mcontrol6_slot(0);
    saved_mstatus = ai_micro_cross_page_enable_machine_breakpoint_delivery();
    goto_priv(PRIV_HS);
    TEST_SETUP_EXCEPT();
    ai_micro_cross_page_execute_load_once(vaddr);
    TEST_ASSERT("after two prior dual-slot breakpoint hits, clearing only the execute-PC slot still leaves the cross-page load address trigger active in the adjacent-preservation corner",
        excpt.triggered == true &&
        excpt.cause == CAUSE_BKP
    );

    goto_priv(PRIV_M);
    TEST_ASSERT("with only the address trigger slot armed, the trapped no-fault cross-page split-load probe still leaves both boundary doublewords and their adjacent neighbors unchanged",
        read64(low_adjacent_phys) == low_adjacent &&
        read64(low_phys) == low_pattern &&
        read64(high_phys) == high_pattern &&
        read64(high_adjacent_phys) == high_adjacent
    );
    ai_micro_cross_page_restore_machine_status(saved_mstatus);

    ai_micro_cross_page_clear_mcontrol6_slot(1);
    goto_priv(PRIV_HS);
    TEST_SETUP_EXCEPT();
    uint64_t value = ai_micro_cross_page_execute_load_once(vaddr);
    TEST_ASSERT("after clearing both trigger slots, the same no-fault cross-page split load completes normally and still returns the merged payload in the adjacent-preservation corner",
        excpt.triggered == false &&
        value == 0x0011223311223344ULL
    );

    goto_priv(PRIV_M);
    TEST_ASSERT("after the recovered no-fault split-load path with execute-PC and address triggers, both boundary doublewords and their adjacent neighbors remain unchanged",
        read64(low_adjacent_phys) == low_adjacent &&
        read64(low_phys) == low_pattern &&
        read64(high_phys) == high_pattern &&
        read64(high_adjacent_phys) == high_adjacent
    );

    TEST_END();
}

bool ai_micro_hs_scalar_cross_page_load_after_vector_cross_page_execute_and_address_trigger_repeated_breakpoint_then_completion_corner()
{
    TEST_START();

    const uint8_t pattern[16] = {
        0x80, 0x81, 0x82, 0x83,
        0x84, 0x85, 0x86, 0x87,
        0x88, 0x89, 0x8a, 0x8b,
        0x8c, 0x8d, 0x8e, 0x8f
    };
    uintptr_t vector_vaddr = ai_micro_cross_page_trigger_addr(VSRWX_GRX, 8U);
    uintptr_t scalar_vaddr = ai_micro_cross_page_trigger_addr(VSRWX_GRX, 4U);
    uintptr_t low_phys = ai_micro_cross_page_trigger_page_end_phys(VSRWX_GRX);
    uintptr_t high_phys = phys_page_base(VSRWX_GR);
    uintptr_t scratch_vaddr = hs_page_base(VSRWX_GRWX) + 0x40U;
    uintptr_t scratch_phys = phys_page_base(VSRWX_GRWX) + 0x40U;
    uintptr_t trigger_pc = (uintptr_t)ai_micro_cross_page_execute_load_once;
    const uint8_t scratch_before = 0x5aU;
    const uint8_t scratch_after = 0xa5U;
    uint64_t saved_mstatus;
    uint64_t value = 0;

    goto_priv(PRIV_M);
    ai_micro_cross_page_trigger_write_bytes(low_phys, pattern, 8U);
    ai_micro_cross_page_trigger_write_bytes(high_phys, pattern + 8U, 8U);
    write8(scratch_phys - 1U, scratch_before);
    write8(scratch_phys + 16U, scratch_after);

    goto_priv(PRIV_HS);
    hspt_init();
    sfence_vma();

    TEST_SETUP_EXCEPT();
    vle8_to_v6((const uint8_t *)vector_vaddr, 16);
    TEST_ASSERT("a vector unit-stride load can pull 16 bytes across the same HS page boundary before the mixed vector/scalar trigger probe",
        excpt.triggered == false
    );

    TEST_SETUP_EXCEPT();
    vse8_from_v6((uint8_t *)scratch_vaddr, 16);
    TEST_ASSERT("storing the vector cross-page payload into a scratch page preserves the original 16-byte boundary byte sequence before the scalar trigger probe",
        excpt.triggered == false
    );

    goto_priv(PRIV_M);
    TEST_ASSERT("the scratch copy after the vector cross-page load still matches the exact boundary payload and keeps adjacent sentinels unchanged before the scalar trigger probe",
        ai_micro_cross_page_trigger_bytes_match(scratch_phys, pattern, 16U) &&
        read8(scratch_phys - 1U) == scratch_before &&
        read8(scratch_phys + 16U) == scratch_after
    );

    goto_priv(PRIV_HS);
    hspt_init();
    sfence_vma();

    ai_micro_cross_page_clear_mcontrol6_slot(0);
    ai_micro_cross_page_clear_mcontrol6_slot(1);
    ai_micro_cross_page_arm_mcontrol6_slot(0, trigger_pc,
        AI_MICRO_CROSS_PAGE_MCONTROL6_EXECUTE);
    ai_micro_cross_page_arm_mcontrol6_slot(1, scalar_vaddr,
        AI_MICRO_CROSS_PAGE_MCONTROL6_LOAD);
    saved_mstatus = ai_micro_cross_page_enable_machine_breakpoint_delivery();

    goto_priv(PRIV_HS);
    TEST_SETUP_EXCEPT();
    ai_micro_cross_page_execute_load_once(scalar_vaddr);
    TEST_ASSERT("after a vector cross-page load touches the same page pair, the first scalar dual-trigger split-load probe still reports breakpoint",
        excpt.triggered == true &&
        excpt.cause == CAUSE_BKP
    );

    goto_priv(PRIV_M);
    TEST_ASSERT("the first trapped scalar cross-page trigger probe still leaves both boundary fragments unchanged after the prior vector load",
        ai_micro_cross_page_trigger_bytes_match(low_phys, pattern, 8U) &&
        ai_micro_cross_page_trigger_bytes_match(high_phys, pattern + 8U, 8U)
    );
    ai_micro_cross_page_restore_machine_status(saved_mstatus);

    saved_mstatus = ai_micro_cross_page_enable_machine_breakpoint_delivery();
    goto_priv(PRIV_HS);
    TEST_SETUP_EXCEPT();
    ai_micro_cross_page_execute_load_once(scalar_vaddr);
    TEST_ASSERT("after the earlier vector cross-page load, the second scalar dual-trigger split-load probe still reports breakpoint",
        excpt.triggered == true &&
        excpt.cause == CAUSE_BKP
    );

    goto_priv(PRIV_M);
    TEST_ASSERT("the second trapped scalar cross-page trigger probe still leaves both boundary fragments unchanged after the prior vector load",
        ai_micro_cross_page_trigger_bytes_match(low_phys, pattern, 8U) &&
        ai_micro_cross_page_trigger_bytes_match(high_phys, pattern + 8U, 8U)
    );
    ai_micro_cross_page_restore_machine_status(saved_mstatus);

    ai_micro_cross_page_clear_mcontrol6_slot(0);
    saved_mstatus = ai_micro_cross_page_enable_machine_breakpoint_delivery();
    goto_priv(PRIV_HS);
    TEST_SETUP_EXCEPT();
    ai_micro_cross_page_execute_load_once(scalar_vaddr);
    TEST_ASSERT("after two prior breakpoint hits and an earlier vector cross-page load, clearing only the execute-PC slot still leaves the scalar cross-page load address trigger active",
        excpt.triggered == true &&
        excpt.cause == CAUSE_BKP
    );

    goto_priv(PRIV_M);
    TEST_ASSERT("with only the address trigger slot armed, the scalar cross-page load probe still leaves both boundary fragments unchanged after the prior vector load",
        ai_micro_cross_page_trigger_bytes_match(low_phys, pattern, 8U) &&
        ai_micro_cross_page_trigger_bytes_match(high_phys, pattern + 8U, 8U)
    );
    ai_micro_cross_page_restore_machine_status(saved_mstatus);

    ai_micro_cross_page_clear_mcontrol6_slot(1);
    goto_priv(PRIV_HS);
    TEST_SETUP_EXCEPT();
    value = ai_micro_cross_page_execute_load_once(scalar_vaddr);
    TEST_ASSERT("after clearing both trigger slots, the scalar cross-page load still completes normally and returns the merged payload even though a vector cross-page load touched the shared path earlier",
        excpt.triggered == false &&
        value == 0x8b8a898887868584ULL
    );

    TEST_END();
}
