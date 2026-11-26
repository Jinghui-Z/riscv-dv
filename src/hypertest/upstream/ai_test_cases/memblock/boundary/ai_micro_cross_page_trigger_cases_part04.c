#include "ai_micro_cross_page_trigger_common.h"

bool ai_micro_hs_store_cross_page_high_half_execute_and_address_trigger_page_fault_repeated_breakpoint_then_fault_no_side_effect_adjacent_preserved_corner()
{
    TEST_START();

    enum test_page low_page = VSRW_GX;
    enum test_page high_page = VSRX_GRWX;
    uintptr_t vaddr = ai_micro_cross_page_trigger_addr(low_page, 4U);
    uintptr_t high_half_vaddr = hs_page_base(high_page);
    uintptr_t low_adjacent_phys = phys_page_base(low_page) + PAGE_SIZE - (2 * sizeof(uint64_t));
    uintptr_t low_phys = ai_micro_cross_page_trigger_page_end_phys(low_page);
    uintptr_t high_phys = phys_page_base(high_page);
    uintptr_t high_adjacent_phys = high_phys + sizeof(uint64_t);
    uintptr_t trigger_pc = (uintptr_t)ai_micro_cross_page_execute_store_once;
    const uint64_t low_adjacent = 0x3141592653589793ULL;
    const uint64_t low_pattern = 0x1122334455667788ULL;
    const uint64_t high_pattern = 0x13579bdf2468ace0ULL;
    const uint64_t high_adjacent = 0x2718281828459045ULL;
    const uint64_t expected_low_after_fault = low_pattern;
    bool trap_triggered;
    uint64_t trap_cause;
    uint64_t trap_tval;
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
    ai_micro_cross_page_arm_mcontrol6_slot(1, vaddr, AI_MICRO_CROSS_PAGE_MCONTROL6_STORE);
    saved_mstatus = ai_micro_cross_page_enable_machine_breakpoint_delivery();

    goto_priv(PRIV_HS);
    TEST_SETUP_EXCEPT();
    ai_micro_cross_page_execute_store_once(vaddr, 0xfedcba9876543210ULL);
    TEST_ASSERT("with both execute-PC and address trigger slots armed on the same cross-page high-half page-fault split store, the first access still reports breakpoint in the adjacent-preservation corner",
        excpt.triggered == true &&
        excpt.cause == CAUSE_BKP
    );

    goto_priv(PRIV_M);
    TEST_ASSERT("the first trapped cross-page high-half page-fault split store still leaves both page fragments and their adjacent neighbors unchanged in the adjacent-preservation corner",
        read64(low_adjacent_phys) == low_adjacent &&
        read64(low_phys) == low_pattern &&
        read64(high_phys) == high_pattern &&
        read64(high_adjacent_phys) == high_adjacent
    );
    ai_micro_cross_page_restore_machine_status(saved_mstatus);

    goto_priv(PRIV_HS);
    hspt_init();
    sfence_vma();

    saved_mstatus = ai_micro_cross_page_enable_machine_breakpoint_delivery();
    goto_priv(PRIV_HS);
    TEST_SETUP_EXCEPT();
    ai_micro_cross_page_execute_store_once(vaddr, 0xfedcba9876543210ULL);
    TEST_ASSERT("with both execute-PC and address trigger slots still armed on that same high-half page-fault split store, the second access still reports breakpoint in the adjacent-preservation corner",
        excpt.triggered == true &&
        excpt.cause == CAUSE_BKP
    );

    goto_priv(PRIV_M);
    TEST_ASSERT("the second trapped cross-page high-half page-fault split store still leaves both page fragments and their adjacent neighbors unchanged in the adjacent-preservation corner",
        read64(low_adjacent_phys) == low_adjacent &&
        read64(low_phys) == low_pattern &&
        read64(high_phys) == high_pattern &&
        read64(high_adjacent_phys) == high_adjacent
    );
    ai_micro_cross_page_restore_machine_status(saved_mstatus);

    goto_priv(PRIV_HS);
    hspt_init();
    sfence_vma();

    ai_micro_cross_page_clear_mcontrol6_slot(0);
    saved_mstatus = ai_micro_cross_page_enable_machine_breakpoint_delivery();
    goto_priv(PRIV_HS);
    TEST_SETUP_EXCEPT();
    ai_micro_cross_page_execute_store_once(vaddr, 0xfedcba9876543210ULL);
    TEST_ASSERT("after two prior dual-slot breakpoint hits, clearing only the execute-PC slot still leaves the cross-page store address trigger active on the high-half page-fault path in the adjacent-preservation corner",
        excpt.triggered == true &&
        excpt.cause == CAUSE_BKP
    );

    goto_priv(PRIV_M);
    TEST_ASSERT("with only the address trigger slot armed, the trapped cross-page high-half page-fault split store still leaves both page fragments and their adjacent neighbors unchanged in the adjacent-preservation corner",
        read64(low_adjacent_phys) == low_adjacent &&
        read64(low_phys) == low_pattern &&
        read64(high_phys) == high_pattern &&
        read64(high_adjacent_phys) == high_adjacent
    );
    ai_micro_cross_page_restore_machine_status(saved_mstatus);

    goto_priv(PRIV_HS);
    hspt_init();
    sfence_vma();

    ai_micro_cross_page_clear_mcontrol6_slot(1);
    goto_priv(PRIV_HS);
    TEST_SETUP_EXCEPT();
    ai_micro_cross_page_execute_store_once(vaddr, 0xfedcba9876543210ULL);
    trap_triggered = excpt.triggered;
    trap_cause = excpt.cause;
    trap_tval = excpt.tval;

    goto_priv(PRIV_M);
    TEST_ASSERT("after clearing both trigger slots, the same cross-page store falls back to the high-half store page-fault corner with no store side effect while adjacent neighbors still stay untouched",
        trap_triggered == true &&
        trap_cause == CAUSE_SPF &&
        trap_tval == high_half_vaddr &&
        read64(low_adjacent_phys) == low_adjacent &&
        read64(low_phys) == expected_low_after_fault &&
        read64(high_phys) == high_pattern &&
        read64(high_adjacent_phys) == high_adjacent
    );

    TEST_END();
}

bool ai_micro_hs_store_cross_page_high_half_execute_and_address_trigger_page_fault_repeated_breakpoint_then_fix_high_half_recovers_corner()
{
    TEST_START();

    enum test_page low_page = VSRW_GX;
    enum test_page high_page = VSRX_GRWX;
    uintptr_t vaddr = ai_micro_cross_page_trigger_addr(low_page, 4U);
    uintptr_t high_half_vaddr = hs_page_base(high_page);
    uintptr_t low_phys = ai_micro_cross_page_trigger_page_end_phys(low_page);
    uintptr_t high_phys = phys_page_base(high_page);
    uintptr_t trigger_pc = (uintptr_t)ai_micro_cross_page_execute_store_once;
    const uint64_t low_pattern = 0x1122334455667788ULL;
    const uint64_t high_pattern = 0x13579bdf2468ace0ULL;
    const uint64_t fault_store = 0xfedcba9876543210ULL;
    const uint64_t expected_low_after_fault = low_pattern;
    const uint64_t recovery_store = 0x0123456789abcdefULL;
    const uint64_t expected_low_after_store =
        (low_pattern & 0xffffffffULL) | ((recovery_store & 0xffffffffULL) << 32);
    const uint64_t expected_high_after_store =
        (high_pattern & 0xffffffff00000000ULL) | (recovery_store >> 32);
    bool trap_triggered;
    uint64_t trap_cause;
    uint64_t trap_tval;
    uint64_t saved_mstatus;

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
    ai_micro_cross_page_arm_mcontrol6_slot(1, vaddr, AI_MICRO_CROSS_PAGE_MCONTROL6_STORE);
    saved_mstatus = ai_micro_cross_page_enable_machine_breakpoint_delivery();

    goto_priv(PRIV_HS);
    TEST_SETUP_EXCEPT();
    ai_micro_cross_page_execute_store_once(vaddr, fault_store);
    TEST_ASSERT("with both execute-PC and address trigger slots armed on the same cross-page high-half page-fault split store, the first access still reports breakpoint before any later repair",
        excpt.triggered == true &&
        excpt.cause == CAUSE_BKP
    );

    goto_priv(PRIV_M);
    TEST_ASSERT("the first trapped cross-page high-half page-fault split store still leaves both page fragments unchanged before any later repair",
        read64(low_phys) == low_pattern &&
        read64(high_phys) == high_pattern
    );
    ai_micro_cross_page_restore_machine_status(saved_mstatus);

    goto_priv(PRIV_HS);
    hspt_init();
    sfence_vma();

    saved_mstatus = ai_micro_cross_page_enable_machine_breakpoint_delivery();
    goto_priv(PRIV_HS);
    TEST_SETUP_EXCEPT();
    ai_micro_cross_page_execute_store_once(vaddr, fault_store);
    TEST_ASSERT("with both execute-PC and address trigger slots still armed on that same high-half page-fault split store, the second access still reports breakpoint before any later repair",
        excpt.triggered == true &&
        excpt.cause == CAUSE_BKP
    );

    goto_priv(PRIV_M);
    TEST_ASSERT("the second trapped cross-page high-half page-fault split store still leaves both page fragments unchanged before any later repair",
        read64(low_phys) == low_pattern &&
        read64(high_phys) == high_pattern
    );
    ai_micro_cross_page_restore_machine_status(saved_mstatus);

    goto_priv(PRIV_HS);
    hspt_init();
    sfence_vma();

    ai_micro_cross_page_clear_mcontrol6_slot(0);
    saved_mstatus = ai_micro_cross_page_enable_machine_breakpoint_delivery();
    goto_priv(PRIV_HS);
    TEST_SETUP_EXCEPT();
    ai_micro_cross_page_execute_store_once(vaddr, fault_store);
    TEST_ASSERT("after two prior dual-slot breakpoint hits, clearing only the execute-PC slot still leaves the cross-page store address trigger active on the high-half page-fault path before repair",
        excpt.triggered == true &&
        excpt.cause == CAUSE_BKP
    );

    goto_priv(PRIV_M);
    TEST_ASSERT("with only the address trigger slot armed, the trapped cross-page high-half page-fault split store still leaves both page fragments unchanged before repair",
        read64(low_phys) == low_pattern &&
        read64(high_phys) == high_pattern
    );
    ai_micro_cross_page_restore_machine_status(saved_mstatus);

    goto_priv(PRIV_HS);
    hspt_init();
    sfence_vma();

    ai_micro_cross_page_clear_mcontrol6_slot(1);
    goto_priv(PRIV_HS);
    TEST_SETUP_EXCEPT();
    ai_micro_cross_page_execute_store_once(vaddr, fault_store);
    trap_triggered = excpt.triggered;
    trap_cause = excpt.cause;
    trap_tval = excpt.tval;

    goto_priv(PRIV_M);
    TEST_ASSERT("after clearing both trigger slots, the same cross-page store first falls back to the high-half store page-fault corner with no store side effect before repair",
        trap_triggered == true &&
        trap_cause == CAUSE_SPF &&
        trap_tval == high_half_vaddr &&
        read64(low_phys) == expected_low_after_fault &&
        read64(high_phys) == high_pattern
    );
    ai_micro_cross_page_set_hs_leaf(high_page, phys_page_base(high_page),
        PTE_V | PTE_AD | PTE_RWX);
    sfence_vma();
    write64(low_phys, low_pattern);
    write64(high_phys, high_pattern);

    goto_priv(PRIV_HS);
    TEST_SETUP_EXCEPT();
    ai_micro_cross_page_execute_store_once(vaddr, recovery_store);
    TEST_ASSERT("after repeated dual-slot breakpoint hits on the high-half page-fault split store, fixing the upper leaf lets the same split store recover and retire normally",
        excpt.triggered == false
    );

    goto_priv(PRIV_M);
    TEST_ASSERT("after the recovered split-store path, the low boundary fragment matches the lower half of the committed payload while preserving the untouched low-page bytes",
        read64(low_phys) == expected_low_after_store
    );
    TEST_ASSERT("after the recovered split-store path, the high boundary fragment matches the upper half of the committed payload while preserving the untouched high-page bytes",
        read64(high_phys) == expected_high_after_store
    );

    goto_priv(PRIV_HS);
    hspt_init();
    sfence_vma();

    TEST_END();
}

bool ai_micro_hs_store_cross_page_high_half_execute_and_address_trigger_page_fault_repeated_breakpoint_then_fix_high_half_adjacent_preserved_corner()
{
    TEST_START();

    enum test_page low_page = VSRW_GX;
    enum test_page high_page = VSRX_GRWX;
    uintptr_t vaddr = ai_micro_cross_page_trigger_addr(low_page, 4U);
    uintptr_t high_half_vaddr = hs_page_base(high_page);
    uintptr_t low_adjacent_phys = phys_page_base(low_page) + PAGE_SIZE - (2 * sizeof(uint64_t));
    uintptr_t low_phys = ai_micro_cross_page_trigger_page_end_phys(low_page);
    uintptr_t high_phys = phys_page_base(high_page);
    uintptr_t high_adjacent_phys = high_phys + sizeof(uint64_t);
    uintptr_t trigger_pc = (uintptr_t)ai_micro_cross_page_execute_store_once;
    const uint64_t low_adjacent = 0x3141592653589793ULL;
    const uint64_t low_pattern = 0x1122334455667788ULL;
    const uint64_t high_pattern = 0x13579bdf2468ace0ULL;
    const uint64_t high_adjacent = 0x2718281828459045ULL;
    const uint64_t fault_store = 0xfedcba9876543210ULL;
    const uint64_t expected_low_after_fault = low_pattern;
    const uint64_t recovery_store = 0x0123456789abcdefULL;
    const uint64_t expected_low_after_store =
        (low_pattern & 0xffffffffULL) | ((recovery_store & 0xffffffffULL) << 32);
    const uint64_t expected_high_after_store =
        (high_pattern & 0xffffffff00000000ULL) | (recovery_store >> 32);
    bool trap_triggered;
    uint64_t trap_cause;
    uint64_t trap_tval;
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
    ai_micro_cross_page_arm_mcontrol6_slot(1, vaddr, AI_MICRO_CROSS_PAGE_MCONTROL6_STORE);
    saved_mstatus = ai_micro_cross_page_enable_machine_breakpoint_delivery();

    goto_priv(PRIV_HS);
    TEST_SETUP_EXCEPT();
    ai_micro_cross_page_execute_store_once(vaddr, fault_store);
    TEST_ASSERT("with both execute-PC and address trigger slots armed on the same cross-page high-half page-fault split store, the first access still reports breakpoint before any later repair in the adjacent-preservation corner",
        excpt.triggered == true &&
        excpt.cause == CAUSE_BKP
    );

    goto_priv(PRIV_M);
    TEST_ASSERT("the first trapped cross-page high-half page-fault split store still leaves both page fragments and their adjacent neighbors unchanged before any later repair",
        read64(low_adjacent_phys) == low_adjacent &&
        read64(low_phys) == low_pattern &&
        read64(high_phys) == high_pattern &&
        read64(high_adjacent_phys) == high_adjacent
    );
    ai_micro_cross_page_restore_machine_status(saved_mstatus);

    goto_priv(PRIV_HS);
    hspt_init();
    sfence_vma();

    saved_mstatus = ai_micro_cross_page_enable_machine_breakpoint_delivery();
    goto_priv(PRIV_HS);
    TEST_SETUP_EXCEPT();
    ai_micro_cross_page_execute_store_once(vaddr, fault_store);
    TEST_ASSERT("with both execute-PC and address trigger slots still armed on that same high-half page-fault split store, the second access still reports breakpoint before any later repair in the adjacent-preservation corner",
        excpt.triggered == true &&
        excpt.cause == CAUSE_BKP
    );

    goto_priv(PRIV_M);
    TEST_ASSERT("the second trapped cross-page high-half page-fault split store still leaves both page fragments and their adjacent neighbors unchanged before any later repair",
        read64(low_adjacent_phys) == low_adjacent &&
        read64(low_phys) == low_pattern &&
        read64(high_phys) == high_pattern &&
        read64(high_adjacent_phys) == high_adjacent
    );
    ai_micro_cross_page_restore_machine_status(saved_mstatus);

    goto_priv(PRIV_HS);
    hspt_init();
    sfence_vma();

    ai_micro_cross_page_clear_mcontrol6_slot(0);
    saved_mstatus = ai_micro_cross_page_enable_machine_breakpoint_delivery();
    goto_priv(PRIV_HS);
    TEST_SETUP_EXCEPT();
    ai_micro_cross_page_execute_store_once(vaddr, fault_store);
    TEST_ASSERT("after two prior dual-slot breakpoint hits, clearing only the execute-PC slot still leaves the cross-page store address trigger active on the high-half page-fault path before repair in the adjacent-preservation corner",
        excpt.triggered == true &&
        excpt.cause == CAUSE_BKP
    );

    goto_priv(PRIV_M);
    TEST_ASSERT("with only the address trigger slot armed, the trapped cross-page high-half page-fault split store still leaves both page fragments and their adjacent neighbors unchanged before repair",
        read64(low_adjacent_phys) == low_adjacent &&
        read64(low_phys) == low_pattern &&
        read64(high_phys) == high_pattern &&
        read64(high_adjacent_phys) == high_adjacent
    );
    ai_micro_cross_page_restore_machine_status(saved_mstatus);

    goto_priv(PRIV_HS);
    hspt_init();
    sfence_vma();

    ai_micro_cross_page_clear_mcontrol6_slot(1);
    goto_priv(PRIV_HS);
    TEST_SETUP_EXCEPT();
    ai_micro_cross_page_execute_store_once(vaddr, fault_store);
    trap_triggered = excpt.triggered;
    trap_cause = excpt.cause;
    trap_tval = excpt.tval;

    goto_priv(PRIV_M);
    TEST_ASSERT("after clearing both trigger slots, the same cross-page store first falls back to the high-half store page-fault corner with no store side effect before repair in the adjacent-preservation corner",
        trap_triggered == true &&
        trap_cause == CAUSE_SPF &&
        trap_tval == high_half_vaddr &&
        read64(low_adjacent_phys) == low_adjacent &&
        read64(low_phys) == expected_low_after_fault &&
        read64(high_phys) == high_pattern &&
        read64(high_adjacent_phys) == high_adjacent
    );
    ai_micro_cross_page_set_hs_leaf(high_page, phys_page_base(high_page),
        PTE_V | PTE_AD | PTE_RWX);
    sfence_vma();
    write64(low_adjacent_phys, low_adjacent);
    write64(low_phys, low_pattern);
    write64(high_phys, high_pattern);
    write64(high_adjacent_phys, high_adjacent);

    goto_priv(PRIV_HS);
    TEST_SETUP_EXCEPT();
    ai_micro_cross_page_execute_store_once(vaddr, recovery_store);
    TEST_ASSERT("after repeated dual-slot breakpoint hits on the high-half page-fault split store, fixing the upper leaf lets the same split store recover and retire normally while keeping adjacent words untouched",
        excpt.triggered == false
    );

    goto_priv(PRIV_M);
    TEST_ASSERT("after the recovered split-store path, the low boundary fragment matches the lower half of the committed payload while preserving the adjacent low-page word",
        read64(low_adjacent_phys) == low_adjacent &&
        read64(low_phys) == expected_low_after_store
    );
    TEST_ASSERT("after the recovered split-store path, the high boundary fragment matches the upper half of the committed payload while preserving the adjacent high-page word",
        read64(high_phys) == expected_high_after_store &&
        read64(high_adjacent_phys) == high_adjacent
    );

    goto_priv(PRIV_HS);
    hspt_init();
    sfence_vma();

    TEST_END();
}

bool ai_micro_hs_store_cross_page_high_half_execute_and_address_trigger_access_fault_repeated_breakpoint_then_fault_no_side_effect_corner()
{
    TEST_START();

    uintptr_t vaddr = ai_micro_cross_page_trigger_addr(VSRW_GRW, 4U);
    uintptr_t high_half_vaddr = hs_page_base(VSRW_GRX);
    uintptr_t low_phys = ai_micro_cross_page_trigger_page_end_phys(VSRW_GRW);
    uintptr_t deny_start = phys_page_base(VSRW_GRX);
    uintptr_t deny_end = deny_start + PAGE_SIZE;
    uintptr_t trigger_pc = (uintptr_t)ai_micro_cross_page_execute_store_once;
    const uint64_t low_pattern = 0x1122334455667788ULL;
    const uint64_t high_pattern = 0x13579bdf2468ace0ULL;
    const uint64_t expected_low_after_fault = low_pattern;
    bool trap_triggered;
    uint64_t trap_cause;
    uint64_t trap_tval;
    uint64_t saved_mstatus;

    goto_priv(PRIV_M);
    write64(low_phys, low_pattern);
    write64(deny_start, high_pattern);

    ai_micro_cross_page_configure_pmp_window(deny_start, deny_end, AI_MICRO_PMP_TOR_NO_W);

    goto_priv(PRIV_HS);
    hspt_init();
    sfence_vma();

    ai_micro_cross_page_clear_mcontrol6_slot(0);
    ai_micro_cross_page_clear_mcontrol6_slot(1);
    ai_micro_cross_page_arm_mcontrol6_slot(0, trigger_pc,
        AI_MICRO_CROSS_PAGE_MCONTROL6_EXECUTE);
    ai_micro_cross_page_arm_mcontrol6_slot(1, vaddr, AI_MICRO_CROSS_PAGE_MCONTROL6_STORE);
    saved_mstatus = ai_micro_cross_page_enable_machine_breakpoint_delivery();

    goto_priv(PRIV_HS);
    TEST_SETUP_EXCEPT();
    ai_micro_cross_page_execute_store_once(vaddr, 0xfedcba9876543210ULL);
    TEST_ASSERT("with both execute-PC and address trigger slots armed on the same cross-page high-half access-fault split store, the first access still reports breakpoint",
        excpt.triggered == true &&
        excpt.cause == CAUSE_BKP
    );

    goto_priv(PRIV_M);
    TEST_ASSERT("the first trapped cross-page high-half access-fault split store still leaves both page fragments unchanged",
        read64(low_phys) == low_pattern &&
        read64(deny_start) == high_pattern
    );
    ai_micro_cross_page_restore_machine_status(saved_mstatus);

    goto_priv(PRIV_HS);
    hspt_init();
    sfence_vma();

    saved_mstatus = ai_micro_cross_page_enable_machine_breakpoint_delivery();
    goto_priv(PRIV_HS);
    TEST_SETUP_EXCEPT();
    ai_micro_cross_page_execute_store_once(vaddr, 0xfedcba9876543210ULL);
    TEST_ASSERT("with both execute-PC and address trigger slots still armed on that same high-half access-fault split store, the second access still reports breakpoint",
        excpt.triggered == true &&
        excpt.cause == CAUSE_BKP
    );

    goto_priv(PRIV_M);
    TEST_ASSERT("the second trapped cross-page high-half access-fault split store still leaves both page fragments unchanged",
        read64(low_phys) == low_pattern &&
        read64(deny_start) == high_pattern
    );
    ai_micro_cross_page_restore_machine_status(saved_mstatus);

    goto_priv(PRIV_HS);
    hspt_init();
    sfence_vma();

    ai_micro_cross_page_clear_mcontrol6_slot(0);
    saved_mstatus = ai_micro_cross_page_enable_machine_breakpoint_delivery();
    goto_priv(PRIV_HS);
    TEST_SETUP_EXCEPT();
    ai_micro_cross_page_execute_store_once(vaddr, 0xfedcba9876543210ULL);
    TEST_ASSERT("after two prior dual-slot breakpoint hits, clearing only the execute-PC slot still leaves the cross-page store address trigger active on the high-half access-fault path",
        excpt.triggered == true &&
        excpt.cause == CAUSE_BKP
    );

    goto_priv(PRIV_M);
    TEST_ASSERT("with only the address trigger slot armed, the trapped cross-page high-half access-fault split store still leaves both page fragments unchanged",
        read64(low_phys) == low_pattern &&
        read64(deny_start) == high_pattern
    );
    ai_micro_cross_page_restore_machine_status(saved_mstatus);

    goto_priv(PRIV_HS);
    hspt_init();
    sfence_vma();

    ai_micro_cross_page_clear_mcontrol6_slot(1);
    goto_priv(PRIV_HS);
    TEST_SETUP_EXCEPT();
    ai_micro_cross_page_execute_store_once(vaddr, 0xfedcba9876543210ULL);
    trap_triggered = excpt.triggered;
    trap_cause = excpt.cause;
    trap_tval = excpt.tval;

    goto_priv(PRIV_M);
    TEST_ASSERT("after clearing both trigger slots, the same cross-page store falls back to the high-half store access-fault corner with no store side effect",
        trap_triggered == true &&
        trap_cause == CAUSE_SAF &&
        trap_tval == high_half_vaddr &&
        read64(low_phys) == expected_low_after_fault &&
        read64(deny_start) == high_pattern
    );

    ai_micro_cross_page_clear_pmp_window();

    TEST_END();
}

bool ai_micro_hs_store_cross_page_high_half_execute_and_address_trigger_access_fault_repeated_breakpoint_then_fault_no_side_effect_adjacent_preserved_corner()
{
    TEST_START();

    enum test_page low_page = VSRW_GRW;
    enum test_page high_page = VSRW_GRX;
    uintptr_t vaddr = ai_micro_cross_page_trigger_addr(low_page, 4U);
    uintptr_t high_half_vaddr = hs_page_base(high_page);
    uintptr_t low_adjacent_phys = phys_page_base(low_page) + PAGE_SIZE - (2 * sizeof(uint64_t));
    uintptr_t low_phys = ai_micro_cross_page_trigger_page_end_phys(low_page);
    uintptr_t high_phys = phys_page_base(high_page);
    uintptr_t high_adjacent_phys = high_phys + sizeof(uint64_t);
    uintptr_t deny_start = high_phys;
    uintptr_t deny_end = deny_start + PAGE_SIZE;
    uintptr_t trigger_pc = (uintptr_t)ai_micro_cross_page_execute_store_once;
    const uint64_t low_adjacent = 0x0badc0de55aa1234ULL;
    const uint64_t low_pattern = 0x1122334455667788ULL;
    const uint64_t high_pattern = 0x13579bdf2468ace0ULL;
    const uint64_t high_adjacent = 0x1234fedc5678ba90ULL;
    const uint64_t expected_low_after_fault = low_pattern;
    bool trap_triggered;
    uint64_t trap_cause;
    uint64_t trap_tval;
    uint64_t saved_mstatus;

    goto_priv(PRIV_M);
    write64(low_adjacent_phys, low_adjacent);
    write64(low_phys, low_pattern);
    write64(high_phys, high_pattern);
    write64(high_adjacent_phys, high_adjacent);

    ai_micro_cross_page_configure_pmp_window(deny_start, deny_end, AI_MICRO_PMP_TOR_NO_W);

    goto_priv(PRIV_HS);
    hspt_init();
    sfence_vma();

    ai_micro_cross_page_clear_mcontrol6_slot(0);
    ai_micro_cross_page_clear_mcontrol6_slot(1);
    ai_micro_cross_page_arm_mcontrol6_slot(0, trigger_pc,
        AI_MICRO_CROSS_PAGE_MCONTROL6_EXECUTE);
    ai_micro_cross_page_arm_mcontrol6_slot(1, vaddr, AI_MICRO_CROSS_PAGE_MCONTROL6_STORE);
    saved_mstatus = ai_micro_cross_page_enable_machine_breakpoint_delivery();

    goto_priv(PRIV_HS);
    TEST_SETUP_EXCEPT();
    ai_micro_cross_page_execute_store_once(vaddr, 0xfedcba9876543210ULL);
    TEST_ASSERT("with both execute-PC and address trigger slots armed on the same cross-page high-half access-fault split store, the first access still reports breakpoint in the adjacent-preservation corner",
        excpt.triggered == true &&
        excpt.cause == CAUSE_BKP
    );

    goto_priv(PRIV_M);
    TEST_ASSERT("the first trapped cross-page high-half access-fault split store still leaves both page fragments and their adjacent neighbors unchanged in the adjacent-preservation corner",
        read64(low_adjacent_phys) == low_adjacent &&
        read64(low_phys) == low_pattern &&
        read64(high_phys) == high_pattern &&
        read64(high_adjacent_phys) == high_adjacent
    );
    ai_micro_cross_page_restore_machine_status(saved_mstatus);

    goto_priv(PRIV_HS);
    hspt_init();
    sfence_vma();

    saved_mstatus = ai_micro_cross_page_enable_machine_breakpoint_delivery();
    goto_priv(PRIV_HS);
    TEST_SETUP_EXCEPT();
    ai_micro_cross_page_execute_store_once(vaddr, 0xfedcba9876543210ULL);
    TEST_ASSERT("with both execute-PC and address trigger slots still armed on that same high-half access-fault split store, the second access still reports breakpoint in the adjacent-preservation corner",
        excpt.triggered == true &&
        excpt.cause == CAUSE_BKP
    );

    goto_priv(PRIV_M);
    TEST_ASSERT("the second trapped cross-page high-half access-fault split store still leaves both page fragments and their adjacent neighbors unchanged in the adjacent-preservation corner",
        read64(low_adjacent_phys) == low_adjacent &&
        read64(low_phys) == low_pattern &&
        read64(high_phys) == high_pattern &&
        read64(high_adjacent_phys) == high_adjacent
    );
    ai_micro_cross_page_restore_machine_status(saved_mstatus);

    goto_priv(PRIV_HS);
    hspt_init();
    sfence_vma();

    ai_micro_cross_page_clear_mcontrol6_slot(0);
    saved_mstatus = ai_micro_cross_page_enable_machine_breakpoint_delivery();
    goto_priv(PRIV_HS);
    TEST_SETUP_EXCEPT();
    ai_micro_cross_page_execute_store_once(vaddr, 0xfedcba9876543210ULL);
    TEST_ASSERT("after two prior dual-slot breakpoint hits, clearing only the execute-PC slot still leaves the cross-page store address trigger active on the high-half access-fault path in the adjacent-preservation corner",
        excpt.triggered == true &&
        excpt.cause == CAUSE_BKP
    );

    goto_priv(PRIV_M);
    TEST_ASSERT("with only the address trigger slot armed, the trapped cross-page high-half access-fault split store still leaves both page fragments and their adjacent neighbors unchanged in the adjacent-preservation corner",
        read64(low_adjacent_phys) == low_adjacent &&
        read64(low_phys) == low_pattern &&
        read64(high_phys) == high_pattern &&
        read64(high_adjacent_phys) == high_adjacent
    );
    ai_micro_cross_page_restore_machine_status(saved_mstatus);

    goto_priv(PRIV_HS);
    hspt_init();
    sfence_vma();

    ai_micro_cross_page_clear_mcontrol6_slot(1);
    goto_priv(PRIV_HS);
    TEST_SETUP_EXCEPT();
    ai_micro_cross_page_execute_store_once(vaddr, 0xfedcba9876543210ULL);
    trap_triggered = excpt.triggered;
    trap_cause = excpt.cause;
    trap_tval = excpt.tval;

    goto_priv(PRIV_M);
    TEST_ASSERT("after clearing both trigger slots, the same cross-page store falls back to the high-half store access-fault corner with no store side effect while adjacent neighbors still stay untouched",
        trap_triggered == true &&
        trap_cause == CAUSE_SAF &&
        trap_tval == high_half_vaddr &&
        read64(low_adjacent_phys) == low_adjacent &&
        read64(low_phys) == expected_low_after_fault &&
        read64(high_phys) == high_pattern &&
        read64(high_adjacent_phys) == high_adjacent
    );

    ai_micro_cross_page_clear_pmp_window();

    TEST_END();
}

bool ai_micro_hs_store_cross_page_high_half_execute_and_address_trigger_access_fault_repeated_breakpoint_then_restore_pmp_recovers_corner()
{
    TEST_START();

    enum test_page low_page = VSRW_GRW;
    enum test_page high_page = VSRW_GRX;
    uintptr_t vaddr = ai_micro_cross_page_trigger_addr(low_page, 4U);
    uintptr_t high_half_vaddr = hs_page_base(high_page);
    uintptr_t low_phys = ai_micro_cross_page_trigger_page_end_phys(low_page);
    uintptr_t high_phys = phys_page_base(high_page);
    uintptr_t deny_start = high_phys;
    uintptr_t deny_end = deny_start + PAGE_SIZE;
    uintptr_t trigger_pc = (uintptr_t)ai_micro_cross_page_execute_store_once;
    const uint64_t low_pattern = 0x2233445566778899ULL;
    const uint64_t high_pattern = 0x2468ace013579bdfULL;
    const uint64_t fault_store = 0xfedcba9876543210ULL;
    const uint64_t expected_low_after_fault = low_pattern;
    const uint64_t recovery_store = 0x89abcdef01234567ULL;
    const uint64_t expected_low_after_store =
        (low_pattern & 0xffffffffULL) | ((recovery_store & 0xffffffffULL) << 32);
    const uint64_t expected_high_after_store =
        (high_pattern & 0xffffffff00000000ULL) | (recovery_store >> 32);
    bool trap_triggered;
    uint64_t trap_cause;
    uint64_t trap_tval;
    uint64_t saved_mstatus;

    goto_priv(PRIV_M);
    write64(low_phys, low_pattern);
    write64(high_phys, high_pattern);

    ai_micro_cross_page_configure_pmp_window(deny_start, deny_end, AI_MICRO_PMP_TOR_NO_W);

    goto_priv(PRIV_HS);
    hspt_init();
    sfence_vma();

    ai_micro_cross_page_clear_mcontrol6_slot(0);
    ai_micro_cross_page_clear_mcontrol6_slot(1);
    ai_micro_cross_page_arm_mcontrol6_slot(0, trigger_pc,
        AI_MICRO_CROSS_PAGE_MCONTROL6_EXECUTE);
    ai_micro_cross_page_arm_mcontrol6_slot(1, vaddr, AI_MICRO_CROSS_PAGE_MCONTROL6_STORE);
    saved_mstatus = ai_micro_cross_page_enable_machine_breakpoint_delivery();

    goto_priv(PRIV_HS);
    TEST_SETUP_EXCEPT();
    ai_micro_cross_page_execute_store_once(vaddr, fault_store);
    TEST_ASSERT("with both execute-PC and address trigger slots armed on the same cross-page high-half access-fault split store, the first access still reports breakpoint before PMP permission is restored",
        excpt.triggered == true &&
        excpt.cause == CAUSE_BKP
    );

    goto_priv(PRIV_M);
    TEST_ASSERT("the first trapped cross-page high-half access-fault split store still leaves both page fragments unchanged before PMP permission is restored",
        read64(low_phys) == low_pattern &&
        read64(high_phys) == high_pattern
    );
    ai_micro_cross_page_restore_machine_status(saved_mstatus);

    goto_priv(PRIV_HS);
    hspt_init();
    sfence_vma();

    saved_mstatus = ai_micro_cross_page_enable_machine_breakpoint_delivery();
    goto_priv(PRIV_HS);
    TEST_SETUP_EXCEPT();
    ai_micro_cross_page_execute_store_once(vaddr, fault_store);
    TEST_ASSERT("with both execute-PC and address trigger slots still armed on that same high-half access-fault split store, the second access still reports breakpoint before PMP permission is restored",
        excpt.triggered == true &&
        excpt.cause == CAUSE_BKP
    );

    goto_priv(PRIV_M);
    TEST_ASSERT("the second trapped cross-page high-half access-fault split store still leaves both page fragments unchanged before PMP permission is restored",
        read64(low_phys) == low_pattern &&
        read64(high_phys) == high_pattern
    );
    ai_micro_cross_page_restore_machine_status(saved_mstatus);

    goto_priv(PRIV_HS);
    hspt_init();
    sfence_vma();

    ai_micro_cross_page_clear_mcontrol6_slot(0);
    saved_mstatus = ai_micro_cross_page_enable_machine_breakpoint_delivery();
    goto_priv(PRIV_HS);
    TEST_SETUP_EXCEPT();
    ai_micro_cross_page_execute_store_once(vaddr, fault_store);
    TEST_ASSERT("after two prior dual-slot breakpoint hits, clearing only the execute-PC slot still leaves the cross-page store address trigger active on the high-half access-fault path before PMP permission is restored",
        excpt.triggered == true &&
        excpt.cause == CAUSE_BKP
    );

    goto_priv(PRIV_M);
    TEST_ASSERT("with only the address trigger slot armed, the trapped cross-page high-half access-fault split store still leaves both page fragments unchanged before PMP permission is restored",
        read64(low_phys) == low_pattern &&
        read64(high_phys) == high_pattern
    );
    ai_micro_cross_page_restore_machine_status(saved_mstatus);

    goto_priv(PRIV_HS);
    hspt_init();
    sfence_vma();

    ai_micro_cross_page_clear_mcontrol6_slot(1);
    goto_priv(PRIV_HS);
    TEST_SETUP_EXCEPT();
    ai_micro_cross_page_execute_store_once(vaddr, fault_store);
    trap_triggered = excpt.triggered;
    trap_cause = excpt.cause;
    trap_tval = excpt.tval;

    goto_priv(PRIV_M);
    TEST_ASSERT("after clearing both trigger slots, the same cross-page store first falls back to the high-half store access-fault corner with no store side effect before PMP permission is restored",
        trap_triggered == true &&
        trap_cause == CAUSE_SAF &&
        trap_tval == high_half_vaddr &&
        read64(low_phys) == expected_low_after_fault &&
        read64(high_phys) == high_pattern
    );
    ai_micro_cross_page_clear_pmp_window();
    write64(low_phys, low_pattern);
    write64(high_phys, high_pattern);

    goto_priv(PRIV_HS);
    hspt_init();
    sfence_vma();

    TEST_SETUP_EXCEPT();
    ai_micro_cross_page_execute_store_once(vaddr, recovery_store);
    TEST_ASSERT("after repeated dual-slot breakpoint hits on the high-half access-fault split store, restoring PMP write permission lets the same split store recover and retire normally",
        excpt.triggered == false
    );

    goto_priv(PRIV_M);
    TEST_ASSERT("after the recovered split-store path, the low boundary fragment matches the lower half of the committed payload while preserving the untouched low-page bytes",
        read64(low_phys) == expected_low_after_store
    );
    TEST_ASSERT("after the recovered split-store path, the high boundary fragment matches the upper half of the committed payload while preserving the untouched high-page bytes",
        read64(high_phys) == expected_high_after_store
    );

    TEST_END();
}

bool ai_micro_hs_store_cross_page_high_half_execute_and_address_trigger_access_fault_repeated_breakpoint_then_restore_pmp_adjacent_preserved_corner()
{
    TEST_START();

    enum test_page low_page = VSRW_GRW;
    enum test_page high_page = VSRW_GRX;
    uintptr_t vaddr = ai_micro_cross_page_trigger_addr(low_page, 4U);
    uintptr_t high_half_vaddr = hs_page_base(high_page);
    uintptr_t low_adjacent_phys = phys_page_base(low_page) + PAGE_SIZE - (2 * sizeof(uint64_t));
    uintptr_t low_phys = ai_micro_cross_page_trigger_page_end_phys(low_page);
    uintptr_t high_phys = phys_page_base(high_page);
    uintptr_t high_adjacent_phys = high_phys + sizeof(uint64_t);
    uintptr_t deny_start = high_phys;
    uintptr_t deny_end = deny_start + PAGE_SIZE;
    uintptr_t trigger_pc = (uintptr_t)ai_micro_cross_page_execute_store_once;
    const uint64_t low_adjacent = 0x0badc0de55aa1234ULL;
    const uint64_t low_pattern = 0x2233445566778899ULL;
    const uint64_t high_pattern = 0x2468ace013579bdfULL;
    const uint64_t high_adjacent = 0x1234fedc5678ba90ULL;
    const uint64_t fault_store = 0xfedcba9876543210ULL;
    const uint64_t expected_low_after_fault = low_pattern;
    const uint64_t recovery_store = 0x89abcdef01234567ULL;
    const uint64_t expected_low_after_store =
        (low_pattern & 0xffffffffULL) | ((recovery_store & 0xffffffffULL) << 32);
    const uint64_t expected_high_after_store =
        (high_pattern & 0xffffffff00000000ULL) | (recovery_store >> 32);
    bool trap_triggered;
    uint64_t trap_cause;
    uint64_t trap_tval;
    uint64_t saved_mstatus;

    goto_priv(PRIV_M);
    write64(low_adjacent_phys, low_adjacent);
    write64(low_phys, low_pattern);
    write64(high_phys, high_pattern);
    write64(high_adjacent_phys, high_adjacent);

    ai_micro_cross_page_configure_pmp_window(deny_start, deny_end, AI_MICRO_PMP_TOR_NO_W);

    goto_priv(PRIV_HS);
    hspt_init();
    sfence_vma();

    ai_micro_cross_page_clear_mcontrol6_slot(0);
    ai_micro_cross_page_clear_mcontrol6_slot(1);
    ai_micro_cross_page_arm_mcontrol6_slot(0, trigger_pc,
        AI_MICRO_CROSS_PAGE_MCONTROL6_EXECUTE);
    ai_micro_cross_page_arm_mcontrol6_slot(1, vaddr, AI_MICRO_CROSS_PAGE_MCONTROL6_STORE);
    saved_mstatus = ai_micro_cross_page_enable_machine_breakpoint_delivery();

    goto_priv(PRIV_HS);
    TEST_SETUP_EXCEPT();
    ai_micro_cross_page_execute_store_once(vaddr, fault_store);
    TEST_ASSERT("with both execute-PC and address trigger slots armed on the same cross-page high-half access-fault split store, the first access still reports breakpoint before PMP permission is restored in the adjacent-preservation corner",
        excpt.triggered == true &&
        excpt.cause == CAUSE_BKP
    );

    goto_priv(PRIV_M);
    TEST_ASSERT("the first trapped cross-page high-half access-fault split store still leaves both page fragments and their adjacent neighbors unchanged before PMP permission is restored",
        read64(low_adjacent_phys) == low_adjacent &&
        read64(low_phys) == low_pattern &&
        read64(high_phys) == high_pattern &&
        read64(high_adjacent_phys) == high_adjacent
    );
    ai_micro_cross_page_restore_machine_status(saved_mstatus);

    goto_priv(PRIV_HS);
    hspt_init();
    sfence_vma();

    saved_mstatus = ai_micro_cross_page_enable_machine_breakpoint_delivery();
    goto_priv(PRIV_HS);
    TEST_SETUP_EXCEPT();
    ai_micro_cross_page_execute_store_once(vaddr, fault_store);
    TEST_ASSERT("with both execute-PC and address trigger slots still armed on that same high-half access-fault split store, the second access still reports breakpoint before PMP permission is restored in the adjacent-preservation corner",
        excpt.triggered == true &&
        excpt.cause == CAUSE_BKP
    );

    goto_priv(PRIV_M);
    TEST_ASSERT("the second trapped cross-page high-half access-fault split store still leaves both page fragments and their adjacent neighbors unchanged before PMP permission is restored",
        read64(low_adjacent_phys) == low_adjacent &&
        read64(low_phys) == low_pattern &&
        read64(high_phys) == high_pattern &&
        read64(high_adjacent_phys) == high_adjacent
    );
    ai_micro_cross_page_restore_machine_status(saved_mstatus);

    goto_priv(PRIV_HS);
    hspt_init();
    sfence_vma();

    ai_micro_cross_page_clear_mcontrol6_slot(0);
    saved_mstatus = ai_micro_cross_page_enable_machine_breakpoint_delivery();
    goto_priv(PRIV_HS);
    TEST_SETUP_EXCEPT();
    ai_micro_cross_page_execute_store_once(vaddr, fault_store);
    TEST_ASSERT("after two prior dual-slot breakpoint hits, clearing only the execute-PC slot still leaves the cross-page store address trigger active on the high-half access-fault path before PMP permission is restored in the adjacent-preservation corner",
        excpt.triggered == true &&
        excpt.cause == CAUSE_BKP
    );

    goto_priv(PRIV_M);
    TEST_ASSERT("with only the address trigger slot armed, the trapped cross-page high-half access-fault split store still leaves both page fragments and their adjacent neighbors unchanged before PMP permission is restored",
        read64(low_adjacent_phys) == low_adjacent &&
        read64(low_phys) == low_pattern &&
        read64(high_phys) == high_pattern &&
        read64(high_adjacent_phys) == high_adjacent
    );
    ai_micro_cross_page_restore_machine_status(saved_mstatus);

    goto_priv(PRIV_HS);
    hspt_init();
    sfence_vma();

    ai_micro_cross_page_clear_mcontrol6_slot(1);
    goto_priv(PRIV_HS);
    TEST_SETUP_EXCEPT();
    ai_micro_cross_page_execute_store_once(vaddr, fault_store);
    trap_triggered = excpt.triggered;
    trap_cause = excpt.cause;
    trap_tval = excpt.tval;

    goto_priv(PRIV_M);
    TEST_ASSERT("after clearing both trigger slots, the same cross-page store first falls back to the high-half store access-fault corner with no store side effect before PMP permission is restored in the adjacent-preservation corner",
        trap_triggered == true &&
        trap_cause == CAUSE_SAF &&
        trap_tval == high_half_vaddr &&
        read64(low_adjacent_phys) == low_adjacent &&
        read64(low_phys) == expected_low_after_fault &&
        read64(high_phys) == high_pattern &&
        read64(high_adjacent_phys) == high_adjacent
    );
    ai_micro_cross_page_clear_pmp_window();
    write64(low_adjacent_phys, low_adjacent);
    write64(low_phys, low_pattern);
    write64(high_phys, high_pattern);
    write64(high_adjacent_phys, high_adjacent);

    goto_priv(PRIV_HS);
    hspt_init();
    sfence_vma();

    TEST_SETUP_EXCEPT();
    ai_micro_cross_page_execute_store_once(vaddr, recovery_store);
    TEST_ASSERT("after repeated dual-slot breakpoint hits on the high-half access-fault split store, restoring PMP write permission lets the same split store recover and retire normally while keeping adjacent words untouched",
        excpt.triggered == false
    );

    goto_priv(PRIV_M);
    TEST_ASSERT("after the recovered split-store path, the low boundary fragment matches the lower half of the committed payload while preserving the adjacent low-page word after PMP restore",
        read64(low_adjacent_phys) == low_adjacent &&
        read64(low_phys) == expected_low_after_store
    );
    TEST_ASSERT("after the recovered split-store path, the high boundary fragment matches the upper half of the committed payload while preserving the adjacent high-page word after PMP restore",
        read64(high_phys) == expected_high_after_store &&
        read64(high_adjacent_phys) == high_adjacent
    );

    TEST_END();
}

bool ai_micro_hs_load_cross_page_low_half_dual_address_trigger_page_fault_priority_spike_corner()
{
    TEST_START();

    uintptr_t vaddr = ai_micro_cross_page_trigger_addr(VSX_GX, 4U);
    uintptr_t second_half_vaddr = vaddr + 4U;

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
    TEST_ASSERT("with both the original cross-page misaligned load address slot and the true second-half-start slot armed, Spike still reports breakpoint on the low-half page-fault path because only the original address matches",
        excpt.triggered == true &&
        excpt.cause == CAUSE_BKP
    );
    ai_micro_cross_page_restore_machine_status(saved_mstatus);

    goto_priv(PRIV_HS);
    hspt_init();
    sfence_vma();

    ai_micro_cross_page_clear_mcontrol6_slot(0);
    saved_mstatus = ai_micro_cross_page_enable_machine_breakpoint_delivery();
    goto_priv(PRIV_HS);
    TEST_SETUP_EXCEPT();
    ai_micro_cross_page_execute_load_once(vaddr);
    TEST_ASSERT("after clearing the original misaligned-address slot, the remaining second-half-start slot stays ignored on Spike and the cross-page load falls straight back to the underlying low-half load page fault",
        excpt.triggered == true &&
        excpt.cause == CAUSE_LPF &&
        excpt.tval == vaddr
    );
    ai_micro_cross_page_restore_machine_status(saved_mstatus);

    ai_micro_cross_page_clear_mcontrol6_slot(1);
    TEST_END();
}

bool ai_micro_hs_load_cross_page_low_half_dual_address_trigger_access_fault_priority_spike_corner()
{
    TEST_START();

    uintptr_t vaddr = ai_micro_cross_page_trigger_addr(VSRWX_GRX, 4U);
    uintptr_t second_half_vaddr = vaddr + 4U;
    uintptr_t deny_start = phys_page_base(VSRWX_GRX);
    uintptr_t deny_end = deny_start + PAGE_SIZE;

    ai_micro_cross_page_configure_pmp_window(deny_start, deny_end, AI_MICRO_PMP_TOR_NO_R);

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
    TEST_ASSERT("with both the original cross-page misaligned load address slot and the true second-half-start slot armed, Spike still reports breakpoint on the low-half access-fault path because only the original address matches",
        excpt.triggered == true &&
        excpt.cause == CAUSE_BKP
    );
    ai_micro_cross_page_restore_machine_status(saved_mstatus);

    goto_priv(PRIV_HS);
    hspt_init();
    sfence_vma();

    ai_micro_cross_page_clear_mcontrol6_slot(0);
    saved_mstatus = ai_micro_cross_page_enable_machine_breakpoint_delivery();
    goto_priv(PRIV_HS);
    TEST_SETUP_EXCEPT();
    ai_micro_cross_page_execute_load_once(vaddr);
    TEST_ASSERT("after clearing the original misaligned-address slot, the remaining second-half-start slot stays ignored on Spike and the cross-page load falls straight back to the underlying low-half load access fault",
        excpt.triggered == true &&
        excpt.cause == CAUSE_LAF &&
        excpt.tval == vaddr
    );
    ai_micro_cross_page_restore_machine_status(saved_mstatus);

    ai_micro_cross_page_clear_mcontrol6_slot(1);
    ai_micro_cross_page_clear_pmp_window();

    TEST_END();
}

bool ai_micro_hs_load_cross_page_high_half_dual_address_trigger_page_fault_priority_spike_corner()
{
    TEST_START();

    uintptr_t vaddr = ai_micro_cross_page_trigger_addr(VSR_GX, 4U);
    uintptr_t second_half_vaddr = vaddr + 4U;

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
    TEST_ASSERT("with both the original cross-page misaligned load address slot and the true second-half-start slot armed, Spike still reports breakpoint on the upper-half page-fault path because only the original address matches",
        excpt.triggered == true &&
        excpt.cause == CAUSE_BKP
    );
    ai_micro_cross_page_restore_machine_status(saved_mstatus);

    goto_priv(PRIV_HS);
    hspt_init();
    sfence_vma();

    ai_micro_cross_page_clear_mcontrol6_slot(0);
    saved_mstatus = ai_micro_cross_page_enable_machine_breakpoint_delivery();
    goto_priv(PRIV_HS);
    TEST_SETUP_EXCEPT();
    ai_micro_cross_page_execute_load_once(vaddr);
    TEST_ASSERT("after clearing the original misaligned-address slot, the remaining second-half-start slot stays ignored on Spike and the cross-page load falls straight back to the underlying upper-half load page fault",
        excpt.triggered == true &&
        excpt.cause == CAUSE_LPF &&
        excpt.tval == second_half_vaddr
    );
    ai_micro_cross_page_restore_machine_status(saved_mstatus);

    ai_micro_cross_page_clear_mcontrol6_slot(1);
    TEST_END();
}

bool ai_micro_hs_load_cross_page_high_half_dual_address_trigger_access_fault_priority_spike_corner()
{
    TEST_START();

    uintptr_t vaddr = ai_micro_cross_page_trigger_addr(VSRWX_GRX, 4U);
    uintptr_t second_half_vaddr = vaddr + 4U;
    uintptr_t deny_start = phys_page_base(VSRWX_GR);
    uintptr_t deny_end = deny_start + PAGE_SIZE;

    ai_micro_cross_page_configure_pmp_window(deny_start, deny_end, AI_MICRO_PMP_TOR_NO_R);

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
    TEST_ASSERT("with both the original cross-page misaligned load address slot and the true second-half-start slot armed, Spike still reports breakpoint on the upper-half access-fault path because only the original address matches",
        excpt.triggered == true &&
        excpt.cause == CAUSE_BKP
    );
    ai_micro_cross_page_restore_machine_status(saved_mstatus);

    goto_priv(PRIV_HS);
    hspt_init();
    sfence_vma();

    ai_micro_cross_page_clear_mcontrol6_slot(0);
    saved_mstatus = ai_micro_cross_page_enable_machine_breakpoint_delivery();
    goto_priv(PRIV_HS);
    TEST_SETUP_EXCEPT();
    ai_micro_cross_page_execute_load_once(vaddr);
    TEST_ASSERT("after clearing the original misaligned-address slot, the remaining second-half-start slot stays ignored on Spike and the cross-page load falls straight back to the underlying upper-half load access fault",
        excpt.triggered == true &&
        excpt.cause == CAUSE_LAF &&
        excpt.tval == second_half_vaddr
    );
    ai_micro_cross_page_restore_machine_status(saved_mstatus);

    ai_micro_cross_page_clear_mcontrol6_slot(1);
    ai_micro_cross_page_clear_pmp_window();

    TEST_END();
}

bool ai_micro_hs_load_cross_page_high_half_dual_address_trigger_page_fault_repeated_breakpoint_then_fix_high_half_recovers_spike_corner()
{
    TEST_START();

    enum test_page low_page = VSR_GX;
    enum test_page high_page = VSW_GI;
    uintptr_t vaddr = ai_micro_cross_page_trigger_addr(low_page, 4U);
    uintptr_t second_half_vaddr = vaddr + 4U;
    uintptr_t low_phys = ai_micro_cross_page_trigger_page_end_phys(low_page);
    uintptr_t high_phys = phys_page_base(high_page);
    const uint64_t low_pattern = 0x8798a9bacbdcedfeULL;
    const uint64_t high_pattern = 0x1032547698badcfeULL;
    const uint64_t recovery_expected =
        ((high_pattern & 0xffffffffULL) << 32) | (low_pattern >> 32);
    uint64_t saved_mstatus;

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
    saved_mstatus = ai_micro_cross_page_enable_machine_breakpoint_delivery();

    goto_priv(PRIV_HS);
    TEST_SETUP_EXCEPT();
    ai_micro_cross_page_execute_load_once(vaddr);
    TEST_ASSERT("with both the original cross-page misaligned load address slot and the true second-half-start slot armed, the first upper-half page-fault split-load attempt still reports breakpoint on Spike because only the original address matches",
        excpt.triggered == true &&
        excpt.cause == CAUSE_BKP
    );
    ai_micro_cross_page_restore_machine_status(saved_mstatus);

    goto_priv(PRIV_HS);
    hspt_init();
    sfence_vma();

    saved_mstatus = ai_micro_cross_page_enable_machine_breakpoint_delivery();
    goto_priv(PRIV_HS);
    TEST_SETUP_EXCEPT();
    ai_micro_cross_page_execute_load_once(vaddr);
    TEST_ASSERT("with the same original misaligned-load slot and second-half-start slot still armed, the second upper-half page-fault split-load attempt still reports breakpoint on Spike",
        excpt.triggered == true &&
        excpt.cause == CAUSE_BKP
    );
    ai_micro_cross_page_restore_machine_status(saved_mstatus);

    goto_priv(PRIV_HS);
    hspt_init();
    sfence_vma();

    ai_micro_cross_page_clear_mcontrol6_slot(0);
    saved_mstatus = ai_micro_cross_page_enable_machine_breakpoint_delivery();
    goto_priv(PRIV_HS);
    TEST_SETUP_EXCEPT();
    ai_micro_cross_page_execute_load_once(vaddr);
    TEST_ASSERT("after two prior breakpoint hits and then clearing only the original misaligned-load slot, the remaining second-half-start slot still stays ignored on Spike, so the split load first falls back to the upper-half load page fault before repair",
        excpt.triggered == true &&
        excpt.cause == CAUSE_LPF &&
        excpt.tval == second_half_vaddr,
        "triggered=%d cause=0x%llx tval=0x%llx expected_tval=0x%llx",
        excpt.triggered, excpt.cause, excpt.tval, second_half_vaddr
    );
    ai_micro_cross_page_restore_machine_status(saved_mstatus);

    goto_priv(PRIV_M);
    TEST_ASSERT("before repairing the upper leaf, repeated dual-address-trigger hits and the fallback upper-half load page fault leave both boundary backing doublewords unchanged",
        read64(low_phys) == low_pattern &&
        read64(high_phys) == high_pattern
    );
    ai_micro_cross_page_set_hs_leaf(high_page, phys_page_base(high_page),
        PTE_V | PTE_AD | PTE_RWX);
    sfence_vma();

    goto_priv(PRIV_HS);
    TEST_SETUP_EXCEPT();
    uint64_t observed = ai_micro_cross_page_execute_load_once(vaddr);
    TEST_ASSERT("after repeated dual-address-trigger breakpoint hits on the upper-half page-fault split load, fixing the upper leaf lets the same split load recover while the second-half-start slot still stays ignored on Spike",
        excpt.triggered == false &&
        observed == recovery_expected,
        "triggered=%d cause=0x%llx tval=0x%llx observed=0x%llx expected=0x%llx",
        excpt.triggered, excpt.cause, excpt.tval, observed, recovery_expected
    );

    ai_micro_cross_page_clear_mcontrol6_slot(1);
    TEST_END();
}

bool ai_micro_hs_load_cross_page_high_half_dual_address_trigger_access_fault_repeated_breakpoint_then_restore_pmp_recovers_spike_corner()
{
    TEST_START();

    enum test_page low_page = VSRWX_GRX;
    enum test_page high_page = VSRWX_GR;
    uintptr_t vaddr = ai_micro_cross_page_trigger_addr(low_page, 4U);
    uintptr_t second_half_vaddr = vaddr + 4U;
    uintptr_t low_phys = ai_micro_cross_page_trigger_page_end_phys(low_page);
    uintptr_t high_phys = phys_page_base(high_page);
    uintptr_t deny_start = high_phys;
    uintptr_t deny_end = deny_start + PAGE_SIZE;
    const uint64_t low_pattern = 0x98a9bacbdcedfe10ULL;
    const uint64_t high_pattern = 0x21436587a9cbed0fULL;
    const uint64_t recovery_expected =
        ((high_pattern & 0xffffffffULL) << 32) | (low_pattern >> 32);
    uint64_t saved_mstatus;

    goto_priv(PRIV_M);
    write64(low_phys, low_pattern);
    write64(high_phys, high_pattern);

    ai_micro_cross_page_configure_pmp_window(deny_start, deny_end, AI_MICRO_PMP_TOR_NO_R);

    goto_priv(PRIV_HS);
    hspt_init();
    sfence_vma();

    ai_micro_cross_page_clear_mcontrol6_slot(0);
    ai_micro_cross_page_clear_mcontrol6_slot(1);
    ai_micro_cross_page_arm_mcontrol6_slot(0, vaddr,
        AI_MICRO_CROSS_PAGE_MCONTROL6_LOAD);
    ai_micro_cross_page_arm_mcontrol6_slot(1, second_half_vaddr,
        AI_MICRO_CROSS_PAGE_MCONTROL6_LOAD);
    saved_mstatus = ai_micro_cross_page_enable_machine_breakpoint_delivery();

    goto_priv(PRIV_HS);
    TEST_SETUP_EXCEPT();
    ai_micro_cross_page_execute_load_once(vaddr);
    TEST_ASSERT("with both the original cross-page misaligned load address slot and the true second-half-start slot armed, the first upper-half access-fault split-load attempt still reports breakpoint on Spike because only the original address matches",
        excpt.triggered == true &&
        excpt.cause == CAUSE_BKP
    );
    ai_micro_cross_page_restore_machine_status(saved_mstatus);

    goto_priv(PRIV_HS);
    hspt_init();
    sfence_vma();

    saved_mstatus = ai_micro_cross_page_enable_machine_breakpoint_delivery();
    goto_priv(PRIV_HS);
    TEST_SETUP_EXCEPT();
    ai_micro_cross_page_execute_load_once(vaddr);
    TEST_ASSERT("with the same original misaligned-load slot and second-half-start slot still armed, the second upper-half access-fault split-load attempt still reports breakpoint on Spike",
        excpt.triggered == true &&
        excpt.cause == CAUSE_BKP
    );
    ai_micro_cross_page_restore_machine_status(saved_mstatus);

    goto_priv(PRIV_HS);
    hspt_init();
    sfence_vma();

    ai_micro_cross_page_clear_mcontrol6_slot(0);
    saved_mstatus = ai_micro_cross_page_enable_machine_breakpoint_delivery();
    goto_priv(PRIV_HS);
    TEST_SETUP_EXCEPT();
    ai_micro_cross_page_execute_load_once(vaddr);
    TEST_ASSERT("after two prior breakpoint hits and then clearing only the original misaligned-load slot, the remaining second-half-start slot still stays ignored on Spike, so the split load first falls back to the upper-half load access fault before PMP permission is restored",
        excpt.triggered == true &&
        excpt.cause == CAUSE_LAF &&
        excpt.tval == second_half_vaddr
    );
    ai_micro_cross_page_restore_machine_status(saved_mstatus);

    goto_priv(PRIV_M);
    TEST_ASSERT("before restoring PMP read permission, repeated dual-address-trigger hits and the fallback upper-half load access fault leave both boundary backing doublewords unchanged",
        read64(low_phys) == low_pattern &&
        read64(high_phys) == high_pattern
    );
    ai_micro_cross_page_clear_pmp_window();

    goto_priv(PRIV_HS);
    hspt_init();
    sfence_vma();

    TEST_SETUP_EXCEPT();
    uint64_t observed = ai_micro_cross_page_execute_load_once(vaddr);
    TEST_ASSERT("after repeated dual-address-trigger breakpoint hits on the upper-half access-fault split load, restoring PMP read permission lets the same split load recover while the second-half-start slot still stays ignored on Spike",
        excpt.triggered == false &&
        observed == recovery_expected
    );

    ai_micro_cross_page_clear_mcontrol6_slot(1);
    TEST_END();
}

bool ai_micro_hs_store_cross_page_low_half_dual_address_trigger_page_fault_no_side_effect_spike_corner()
{
    TEST_START();

    uintptr_t vaddr = ai_micro_cross_page_trigger_addr(VSR_GX, 4U);
    uintptr_t second_half_vaddr = vaddr + 4U;
    uintptr_t low_phys = ai_micro_cross_page_trigger_page_end_phys(VSR_GX);
    uintptr_t high_phys = phys_page_base(VSX_GRWX);
    const uint64_t low_pattern = 0x1122334455667788ULL;
    const uint64_t high_pattern = 0x99aabbccddeeff00ULL;
    bool trap_triggered;
    uint64_t trap_cause;
    uint64_t trap_tval;

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
    TEST_ASSERT("with both the original cross-page misaligned store address slot and the true second-half-start slot armed, Spike still reports breakpoint on the low-half page-fault path because only the original address matches",
        excpt.triggered == true &&
        excpt.cause == CAUSE_BKP
    );

    goto_priv(PRIV_M);
    TEST_ASSERT("while the original misaligned-address slot is still armed, the trapped low-half page-fault split store leaves both page fragments unchanged",
        read64(low_phys) == low_pattern &&
        read64(high_phys) == high_pattern
    );
    ai_micro_cross_page_restore_machine_status(saved_mstatus);

    goto_priv(PRIV_HS);
    hspt_init();
    sfence_vma();

    ai_micro_cross_page_clear_mcontrol6_slot(0);
    saved_mstatus = ai_micro_cross_page_enable_machine_breakpoint_delivery();
    goto_priv(PRIV_HS);
    TEST_SETUP_EXCEPT();
    ai_micro_cross_page_execute_store_once(vaddr, 0x0123456789abcdefULL);
    trap_triggered = excpt.triggered;
    trap_cause = excpt.cause;
    trap_tval = excpt.tval;
    ai_micro_cross_page_restore_machine_status(saved_mstatus);

    goto_priv(PRIV_M);
    TEST_ASSERT("after clearing the original misaligned-address slot, the remaining second-half-start slot stays ignored on Spike and the cross-page store falls straight back to the underlying low-half store page fault with both page fragments unchanged",
        trap_triggered == true &&
        trap_cause == CAUSE_SPF &&
        trap_tval == vaddr &&
        read64(low_phys) == low_pattern &&
        read64(high_phys) == high_pattern
    );

    ai_micro_cross_page_clear_mcontrol6_slot(1);
    TEST_END();
}

bool ai_micro_hs_store_cross_page_low_half_dual_address_trigger_access_fault_no_side_effect_spike_corner()
{
    TEST_START();

    uintptr_t vaddr = ai_micro_cross_page_trigger_addr(VSRW_GRW, 4U);
    uintptr_t second_half_vaddr = vaddr + 4U;
    uintptr_t low_phys = ai_micro_cross_page_trigger_page_end_phys(VSRW_GRW);
    uintptr_t high_phys = phys_page_base(VSRW_GRX);
    uintptr_t deny_start = phys_page_base(VSRW_GRW);
    uintptr_t deny_end = deny_start + PAGE_SIZE;
    const uint64_t low_pattern = 0x1122334455667788ULL;
    const uint64_t high_pattern = 0x99aabbccddeeff00ULL;
    bool trap_triggered;
    uint64_t trap_cause;
    uint64_t trap_tval;

    goto_priv(PRIV_M);
    write64(low_phys, low_pattern);
    write64(high_phys, high_pattern);

    ai_micro_cross_page_configure_pmp_window(deny_start, deny_end, AI_MICRO_PMP_TOR_NO_W);

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
    TEST_ASSERT("with both the original cross-page misaligned store address slot and the true second-half-start slot armed, Spike still reports breakpoint on the low-half access-fault path because only the original address matches",
        excpt.triggered == true &&
        excpt.cause == CAUSE_BKP
    );

    goto_priv(PRIV_M);
    TEST_ASSERT("while the original misaligned-address slot is still armed, the trapped low-half access-fault split store leaves both page fragments unchanged",
        read64(low_phys) == low_pattern &&
        read64(high_phys) == high_pattern
    );
    ai_micro_cross_page_restore_machine_status(saved_mstatus);

    goto_priv(PRIV_HS);
    hspt_init();
    sfence_vma();

    ai_micro_cross_page_clear_mcontrol6_slot(0);
    saved_mstatus = ai_micro_cross_page_enable_machine_breakpoint_delivery();
    goto_priv(PRIV_HS);
    TEST_SETUP_EXCEPT();
    ai_micro_cross_page_execute_store_once(vaddr, 0x0123456789abcdefULL);
    trap_triggered = excpt.triggered;
    trap_cause = excpt.cause;
    trap_tval = excpt.tval;
    ai_micro_cross_page_restore_machine_status(saved_mstatus);

    goto_priv(PRIV_M);
    TEST_ASSERT("after clearing the original misaligned-address slot, the remaining second-half-start slot stays ignored on Spike and the cross-page store falls straight back to the underlying low-half store access fault with both page fragments unchanged",
        trap_triggered == true &&
        trap_cause == CAUSE_SAF &&
        trap_tval == vaddr &&
        read64(low_phys) == low_pattern &&
        read64(high_phys) == high_pattern
    );

    ai_micro_cross_page_clear_mcontrol6_slot(1);
    ai_micro_cross_page_clear_pmp_window();

    TEST_END();
}

bool ai_micro_hs_store_cross_page_high_half_dual_address_trigger_page_fault_no_side_effect_spike_corner()
{
    TEST_START();

    uintptr_t vaddr = ai_micro_cross_page_trigger_addr(VSRW_GX, 4U);
    uintptr_t second_half_vaddr = vaddr + 4U;
    uintptr_t low_phys = ai_micro_cross_page_trigger_page_end_phys(VSRW_GX);
    uintptr_t high_phys = phys_page_base(VSRX_GRWX);
    const uint64_t low_pattern = 0x1122334455667788ULL;
    const uint64_t high_pattern = 0x13579bdf2468ace0ULL;
    const uint64_t expected_low_after_fault = low_pattern;
    bool trap_triggered;
    uint64_t trap_cause;
    uint64_t trap_tval;

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
    ai_micro_cross_page_execute_store_once(vaddr, 0xfedcba9876543210ULL);
    TEST_ASSERT("with both the original cross-page misaligned store address slot and the true second-half-start slot armed, Spike still reports breakpoint on the upper-half page-fault path because only the original address matches",
        excpt.triggered == true &&
        excpt.cause == CAUSE_BKP
    );

    goto_priv(PRIV_M);
    TEST_ASSERT("while the original misaligned-address slot is still armed, the trapped upper-half page-fault split store leaves both page fragments unchanged",
        read64(low_phys) == low_pattern &&
        read64(high_phys) == high_pattern
    );
    ai_micro_cross_page_restore_machine_status(saved_mstatus);

    goto_priv(PRIV_HS);
    hspt_init();
    sfence_vma();

    ai_micro_cross_page_clear_mcontrol6_slot(0);
    saved_mstatus = ai_micro_cross_page_enable_machine_breakpoint_delivery();
    goto_priv(PRIV_HS);
    TEST_SETUP_EXCEPT();
    ai_micro_cross_page_execute_store_once(vaddr, 0xfedcba9876543210ULL);
    trap_triggered = excpt.triggered;
    trap_cause = excpt.cause;
    trap_tval = excpt.tval;
    ai_micro_cross_page_restore_machine_status(saved_mstatus);

    goto_priv(PRIV_M);
    TEST_ASSERT("after clearing the original misaligned-address slot, the remaining second-half-start slot stays ignored on Spike and the cross-page store falls straight back to the upper-half store page-fault corner with no store side effect",
        trap_triggered == true &&
        trap_cause == CAUSE_SPF &&
        trap_tval == second_half_vaddr &&
        read64(low_phys) == expected_low_after_fault &&
        read64(high_phys) == high_pattern
    );

    ai_micro_cross_page_clear_mcontrol6_slot(1);
    TEST_END();
}

bool ai_micro_hs_store_cross_page_high_half_dual_address_trigger_access_fault_no_side_effect_spike_corner()
{
    TEST_START();

    uintptr_t vaddr = ai_micro_cross_page_trigger_addr(VSRW_GRW, 4U);
    uintptr_t second_half_vaddr = vaddr + 4U;
    uintptr_t low_phys = ai_micro_cross_page_trigger_page_end_phys(VSRW_GRW);
    uintptr_t deny_start = phys_page_base(VSRW_GRX);
    uintptr_t deny_end = deny_start + PAGE_SIZE;
    const uint64_t low_pattern = 0x1122334455667788ULL;
    const uint64_t high_pattern = 0x13579bdf2468ace0ULL;
    const uint64_t expected_low_after_fault = low_pattern;
    bool trap_triggered;
    uint64_t trap_cause;
    uint64_t trap_tval;

    goto_priv(PRIV_M);
    write64(low_phys, low_pattern);
    write64(deny_start, high_pattern);

    ai_micro_cross_page_configure_pmp_window(deny_start, deny_end, AI_MICRO_PMP_TOR_NO_W);

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
    ai_micro_cross_page_execute_store_once(vaddr, 0xfedcba9876543210ULL);
    TEST_ASSERT("with both the original cross-page misaligned store address slot and the true second-half-start slot armed, Spike still reports breakpoint on the upper-half access-fault path because only the original address matches",
        excpt.triggered == true &&
        excpt.cause == CAUSE_BKP
    );

    goto_priv(PRIV_M);
    TEST_ASSERT("while the original misaligned-address slot is still armed, the trapped upper-half access-fault split store leaves both page fragments unchanged",
        read64(low_phys) == low_pattern &&
        read64(deny_start) == high_pattern
    );
    ai_micro_cross_page_restore_machine_status(saved_mstatus);

    goto_priv(PRIV_HS);
    hspt_init();
    sfence_vma();

    ai_micro_cross_page_clear_mcontrol6_slot(0);
    saved_mstatus = ai_micro_cross_page_enable_machine_breakpoint_delivery();
    goto_priv(PRIV_HS);
    TEST_SETUP_EXCEPT();
    ai_micro_cross_page_execute_store_once(vaddr, 0xfedcba9876543210ULL);
    trap_triggered = excpt.triggered;
    trap_cause = excpt.cause;
    trap_tval = excpt.tval;
    ai_micro_cross_page_restore_machine_status(saved_mstatus);

    goto_priv(PRIV_M);
    TEST_ASSERT("after clearing the original misaligned-address slot, the remaining second-half-start slot stays ignored on Spike and the cross-page store falls straight back to the upper-half store access-fault corner with no store side effect",
        trap_triggered == true &&
        trap_cause == CAUSE_SAF &&
        trap_tval == second_half_vaddr &&
        read64(low_phys) == expected_low_after_fault &&
        read64(deny_start) == high_pattern
    );

    ai_micro_cross_page_clear_mcontrol6_slot(1);
    ai_micro_cross_page_clear_pmp_window();

    TEST_END();
}

bool ai_micro_hs_store_cross_page_high_half_dual_address_trigger_page_fault_repeated_breakpoint_then_fix_high_half_recovers_spike_corner()
{
    TEST_START();

    enum test_page low_page = VSRW_GX;
    enum test_page high_page = VSRX_GRWX;
    uintptr_t vaddr = ai_micro_cross_page_trigger_addr(low_page, 4U);
    uintptr_t second_half_vaddr = vaddr + 4U;
    uintptr_t low_phys = ai_micro_cross_page_trigger_page_end_phys(low_page);
    uintptr_t high_phys = phys_page_base(high_page);
    const uint64_t low_pattern = 0x1122334455667788ULL;
    const uint64_t high_pattern = 0x13579bdf2468ace0ULL;
    const uint64_t fault_store = 0xfedcba9876543210ULL;
    const uint64_t expected_low_after_fault = low_pattern;
    const uint64_t recovery_store = 0x0123456789abcdefULL;
    const uint64_t expected_low_after_store =
        (low_pattern & 0xffffffffULL) | ((recovery_store & 0xffffffffULL) << 32);
    const uint64_t expected_high_after_store =
        (high_pattern & 0xffffffff00000000ULL) | (recovery_store >> 32);
    uint64_t saved_mstatus;

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
    saved_mstatus = ai_micro_cross_page_enable_machine_breakpoint_delivery();

    goto_priv(PRIV_HS);
    TEST_SETUP_EXCEPT();
    ai_micro_cross_page_execute_store_once(vaddr, fault_store);
    TEST_ASSERT("with both the original cross-page misaligned store address slot and the true second-half-start slot armed, the first upper-half page-fault split-store attempt still reports breakpoint on Spike because only the original address matches",
        excpt.triggered == true &&
        excpt.cause == CAUSE_BKP
    );

    goto_priv(PRIV_M);
    TEST_ASSERT("the first trapped upper-half page-fault split-store attempt still leaves both page fragments unchanged before any later repair",
        read64(low_phys) == low_pattern &&
        read64(high_phys) == high_pattern
    );
    ai_micro_cross_page_restore_machine_status(saved_mstatus);

    goto_priv(PRIV_HS);
    hspt_init();
    sfence_vma();

    saved_mstatus = ai_micro_cross_page_enable_machine_breakpoint_delivery();
    goto_priv(PRIV_HS);
    TEST_SETUP_EXCEPT();
    ai_micro_cross_page_execute_store_once(vaddr, fault_store);
    TEST_ASSERT("with the same original misaligned-store slot and second-half-start slot still armed, the second upper-half page-fault split-store attempt still reports breakpoint on Spike",
        excpt.triggered == true &&
        excpt.cause == CAUSE_BKP
    );

    goto_priv(PRIV_M);
    TEST_ASSERT("the second trapped upper-half page-fault split-store attempt still leaves both page fragments unchanged before any later repair",
        read64(low_phys) == low_pattern &&
        read64(high_phys) == high_pattern
    );
    ai_micro_cross_page_restore_machine_status(saved_mstatus);

    goto_priv(PRIV_HS);
    hspt_init();
    sfence_vma();

    ai_micro_cross_page_clear_mcontrol6_slot(0);
    saved_mstatus = ai_micro_cross_page_enable_machine_breakpoint_delivery();
    goto_priv(PRIV_HS);
    TEST_SETUP_EXCEPT();
    ai_micro_cross_page_execute_store_once(vaddr, fault_store);
    TEST_ASSERT("after two prior breakpoint hits and then clearing only the original misaligned-store slot, the remaining second-half-start slot still stays ignored on Spike, so the split store first falls back to the upper-half store page-fault corner before repair",
        excpt.triggered == true &&
        excpt.cause == CAUSE_SPF &&
        excpt.tval == second_half_vaddr
    );
    ai_micro_cross_page_restore_machine_status(saved_mstatus);

    goto_priv(PRIV_M);
    TEST_ASSERT("before repairing the upper leaf, repeated dual-address-trigger hits and the fallback upper-half store page fault leave both page fragments unchanged",
        read64(low_phys) == expected_low_after_fault &&
        read64(high_phys) == high_pattern
    );
    ai_micro_cross_page_set_hs_leaf(high_page, phys_page_base(high_page),
        PTE_V | PTE_AD | PTE_RWX);
    sfence_vma();
    write64(low_phys, low_pattern);
    write64(high_phys, high_pattern);

    goto_priv(PRIV_HS);
    TEST_SETUP_EXCEPT();
    ai_micro_cross_page_execute_store_once(vaddr, recovery_store);
    TEST_ASSERT("after repeated dual-address-trigger breakpoint hits on the upper-half page-fault split store, fixing the upper leaf lets the same split store recover while the second-half-start slot still stays ignored on Spike",
        excpt.triggered == false
    );

    goto_priv(PRIV_M);
    TEST_ASSERT("after the recovered split-store path, the low boundary fragment matches the lower half of the committed payload while preserving the untouched low-page bytes",
        read64(low_phys) == expected_low_after_store
    );
    TEST_ASSERT("after the recovered split-store path, the high boundary fragment matches the upper half of the committed payload while preserving the untouched high-page bytes",
        read64(high_phys) == expected_high_after_store
    );

    ai_micro_cross_page_clear_mcontrol6_slot(1);
    TEST_END();
}

bool ai_micro_hs_store_cross_page_high_half_dual_address_trigger_access_fault_repeated_breakpoint_then_restore_pmp_recovers_spike_corner()
{
    TEST_START();

    enum test_page low_page = VSRW_GRW;
    enum test_page high_page = VSRW_GRX;
    uintptr_t vaddr = ai_micro_cross_page_trigger_addr(low_page, 4U);
    uintptr_t second_half_vaddr = vaddr + 4U;
    uintptr_t low_phys = ai_micro_cross_page_trigger_page_end_phys(low_page);
    uintptr_t high_phys = phys_page_base(high_page);
    uintptr_t deny_start = high_phys;
    uintptr_t deny_end = deny_start + PAGE_SIZE;
    const uint64_t low_pattern = 0x1122334455667788ULL;
    const uint64_t high_pattern = 0x13579bdf2468ace0ULL;
    const uint64_t fault_store = 0xfedcba9876543210ULL;
    const uint64_t expected_low_after_fault = low_pattern;
    const uint64_t recovery_store = 0x0123456789abcdefULL;
    const uint64_t expected_low_after_store =
        (low_pattern & 0xffffffffULL) | ((recovery_store & 0xffffffffULL) << 32);
    const uint64_t expected_high_after_store =
        (high_pattern & 0xffffffff00000000ULL) | (recovery_store >> 32);
    uint64_t saved_mstatus;

    goto_priv(PRIV_M);
    write64(low_phys, low_pattern);
    write64(high_phys, high_pattern);

    ai_micro_cross_page_configure_pmp_window(deny_start, deny_end, AI_MICRO_PMP_TOR_NO_W);

    goto_priv(PRIV_HS);
    hspt_init();
    sfence_vma();

    ai_micro_cross_page_clear_mcontrol6_slot(0);
    ai_micro_cross_page_clear_mcontrol6_slot(1);
    ai_micro_cross_page_arm_mcontrol6_slot(0, vaddr,
        AI_MICRO_CROSS_PAGE_MCONTROL6_STORE);
    ai_micro_cross_page_arm_mcontrol6_slot(1, second_half_vaddr,
        AI_MICRO_CROSS_PAGE_MCONTROL6_STORE);
    saved_mstatus = ai_micro_cross_page_enable_machine_breakpoint_delivery();

    goto_priv(PRIV_HS);
    TEST_SETUP_EXCEPT();
    ai_micro_cross_page_execute_store_once(vaddr, fault_store);
    TEST_ASSERT("with both the original cross-page misaligned store address slot and the true second-half-start slot armed, the first upper-half access-fault split-store attempt still reports breakpoint on Spike because only the original address matches",
        excpt.triggered == true &&
        excpt.cause == CAUSE_BKP
    );

    goto_priv(PRIV_M);
    TEST_ASSERT("the first trapped upper-half access-fault split-store attempt still leaves both page fragments unchanged before PMP permission is restored",
        read64(low_phys) == low_pattern &&
        read64(high_phys) == high_pattern
    );
    ai_micro_cross_page_restore_machine_status(saved_mstatus);

    goto_priv(PRIV_HS);
    hspt_init();
    sfence_vma();

    saved_mstatus = ai_micro_cross_page_enable_machine_breakpoint_delivery();
    goto_priv(PRIV_HS);
    TEST_SETUP_EXCEPT();
    ai_micro_cross_page_execute_store_once(vaddr, fault_store);
    TEST_ASSERT("with the same original misaligned-store slot and second-half-start slot still armed, the second upper-half access-fault split-store attempt still reports breakpoint on Spike",
        excpt.triggered == true &&
        excpt.cause == CAUSE_BKP
    );

    goto_priv(PRIV_M);
    TEST_ASSERT("the second trapped upper-half access-fault split-store attempt still leaves both page fragments unchanged before PMP permission is restored",
        read64(low_phys) == low_pattern &&
        read64(high_phys) == high_pattern
    );
    ai_micro_cross_page_restore_machine_status(saved_mstatus);

    goto_priv(PRIV_HS);
    hspt_init();
    sfence_vma();

    ai_micro_cross_page_clear_mcontrol6_slot(0);
    saved_mstatus = ai_micro_cross_page_enable_machine_breakpoint_delivery();
    goto_priv(PRIV_HS);
    TEST_SETUP_EXCEPT();
    ai_micro_cross_page_execute_store_once(vaddr, fault_store);
    TEST_ASSERT("after two prior breakpoint hits and then clearing only the original misaligned-store slot, the remaining second-half-start slot still stays ignored on Spike, so the split store first falls back to the upper-half store access-fault corner before PMP permission is restored",
        excpt.triggered == true &&
        excpt.cause == CAUSE_SAF &&
        excpt.tval == second_half_vaddr
    );
    ai_micro_cross_page_restore_machine_status(saved_mstatus);

    goto_priv(PRIV_M);
    TEST_ASSERT("before restoring PMP write permission, repeated dual-address-trigger hits and the fallback upper-half store access fault leave both page fragments unchanged",
        read64(low_phys) == expected_low_after_fault &&
        read64(high_phys) == high_pattern
    );
    ai_micro_cross_page_clear_pmp_window();
    write64(low_phys, low_pattern);
    write64(high_phys, high_pattern);

    goto_priv(PRIV_HS);
    hspt_init();
    sfence_vma();

    TEST_SETUP_EXCEPT();
    ai_micro_cross_page_execute_store_once(vaddr, recovery_store);
    TEST_ASSERT("after repeated dual-address-trigger breakpoint hits on the upper-half access-fault split store, restoring PMP write permission lets the same split store recover while the second-half-start slot still stays ignored on Spike",
        excpt.triggered == false
    );

    goto_priv(PRIV_M);
    TEST_ASSERT("after the recovered split-store path, the low boundary fragment matches the lower half of the committed payload while preserving the untouched low-page bytes",
        read64(low_phys) == expected_low_after_store
    );
    TEST_ASSERT("after the recovered split-store path, the high boundary fragment matches the upper half of the committed payload while preserving the untouched high-page bytes",
        read64(high_phys) == expected_high_after_store
    );

    ai_micro_cross_page_clear_mcontrol6_slot(1);
    TEST_END();
}

bool ai_micro_hs_load_cross_page_high_half_dual_address_trigger_page_fault_repeated_breakpoint_then_fix_high_half_adjacent_preserved_spike_corner()
{
    TEST_START();

    enum test_page low_page = VSR_GX;
    enum test_page high_page = VSW_GI;
    uintptr_t vaddr = ai_micro_cross_page_trigger_addr(low_page, 4U);
    uintptr_t second_half_vaddr = vaddr + 4U;
    uintptr_t low_adjacent_phys = phys_page_base(low_page) + PAGE_SIZE - (2 * sizeof(uint64_t));
    uintptr_t low_phys = ai_micro_cross_page_trigger_page_end_phys(low_page);
    uintptr_t high_phys = phys_page_base(high_page);
    uintptr_t high_adjacent_phys = high_phys + sizeof(uint64_t);
    const uint64_t low_adjacent = 0x55aa55aa33cc33ccULL;
    const uint64_t low_pattern = 0x8798a9bacbdcedfeULL;
    const uint64_t high_pattern = 0x1032547698badcfeULL;
    const uint64_t high_adjacent = 0xcc33cc3355aa55aaULL;
    const uint64_t recovery_expected =
        ((high_pattern & 0xffffffffULL) << 32) | (low_pattern >> 32);
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
    ai_micro_cross_page_arm_mcontrol6_slot(0, vaddr,
        AI_MICRO_CROSS_PAGE_MCONTROL6_LOAD);
    ai_micro_cross_page_arm_mcontrol6_slot(1, second_half_vaddr,
        AI_MICRO_CROSS_PAGE_MCONTROL6_LOAD);
    saved_mstatus = ai_micro_cross_page_enable_machine_breakpoint_delivery();

    goto_priv(PRIV_HS);
    TEST_SETUP_EXCEPT();
    ai_micro_cross_page_execute_load_once(vaddr);
    TEST_ASSERT("with both the original cross-page misaligned load address slot and the true second-half-start slot armed, the first upper-half page-fault split-load attempt still reports breakpoint on Spike because only the original address matches in the adjacent-preservation corner",
        excpt.triggered == true &&
        excpt.cause == CAUSE_BKP
    );
    ai_micro_cross_page_restore_machine_status(saved_mstatus);

    goto_priv(PRIV_M);
    TEST_ASSERT("the first trapped upper-half page-fault split-load attempt still leaves both boundary doublewords and their adjacent neighbors unchanged before any later repair",
        read64(low_adjacent_phys) == low_adjacent &&
        read64(low_phys) == low_pattern &&
        read64(high_phys) == high_pattern &&
        read64(high_adjacent_phys) == high_adjacent
    );

    goto_priv(PRIV_HS);
    hspt_init();
    sfence_vma();

    saved_mstatus = ai_micro_cross_page_enable_machine_breakpoint_delivery();
    goto_priv(PRIV_HS);
    TEST_SETUP_EXCEPT();
    ai_micro_cross_page_execute_load_once(vaddr);
    TEST_ASSERT("with the same original misaligned-load slot and second-half-start slot still armed, the second upper-half page-fault split-load attempt still reports breakpoint on Spike in the adjacent-preservation corner",
        excpt.triggered == true &&
        excpt.cause == CAUSE_BKP
    );
    ai_micro_cross_page_restore_machine_status(saved_mstatus);

    goto_priv(PRIV_M);
    TEST_ASSERT("the second trapped upper-half page-fault split-load attempt still leaves both boundary doublewords and their adjacent neighbors unchanged before any later repair",
        read64(low_adjacent_phys) == low_adjacent &&
        read64(low_phys) == low_pattern &&
        read64(high_phys) == high_pattern &&
        read64(high_adjacent_phys) == high_adjacent
    );

    goto_priv(PRIV_HS);
    hspt_init();
    sfence_vma();

    ai_micro_cross_page_clear_mcontrol6_slot(0);
    saved_mstatus = ai_micro_cross_page_enable_machine_breakpoint_delivery();
    goto_priv(PRIV_HS);
    TEST_SETUP_EXCEPT();
    ai_micro_cross_page_execute_load_once(vaddr);
    TEST_ASSERT("after two prior breakpoint hits and then clearing only the original misaligned-load slot, the remaining second-half-start slot still stays ignored on Spike, so the split load first falls back to the upper-half load page fault before repair in the adjacent-preservation corner",
        excpt.triggered == true &&
        excpt.cause == CAUSE_LPF &&
        excpt.tval == second_half_vaddr
    );
    ai_micro_cross_page_restore_machine_status(saved_mstatus);

    goto_priv(PRIV_M);
    TEST_ASSERT("before repairing the upper leaf, repeated dual-address-trigger hits and the fallback upper-half load page fault leave both boundary backing doublewords and their adjacent neighbors unchanged",
        read64(low_adjacent_phys) == low_adjacent &&
        read64(low_phys) == low_pattern &&
        read64(high_phys) == high_pattern &&
        read64(high_adjacent_phys) == high_adjacent
    );
    ai_micro_cross_page_set_hs_leaf(high_page, phys_page_base(high_page),
        PTE_V | PTE_AD | PTE_RWX);
    sfence_vma();

    goto_priv(PRIV_HS);
    TEST_SETUP_EXCEPT();
    uint64_t observed = ai_micro_cross_page_execute_load_once(vaddr);
    TEST_ASSERT("after repeated dual-address-trigger breakpoint hits on the upper-half page-fault split load, fixing the upper leaf lets the same split load recover while the second-half-start slot still stays ignored on Spike and adjacent words stay untouched",
        excpt.triggered == false &&
        observed == recovery_expected
    );

    goto_priv(PRIV_M);
    TEST_ASSERT("the recovered split-load path reads the merged payload without dirtying either adjacent boundary word in the adjacent-preservation corner",
        read64(low_adjacent_phys) == low_adjacent &&
        read64(low_phys) == low_pattern &&
        read64(high_phys) == high_pattern &&
        read64(high_adjacent_phys) == high_adjacent
    );

    ai_micro_cross_page_clear_mcontrol6_slot(1);
    TEST_END();
}

bool ai_micro_hs_load_cross_page_high_half_dual_address_trigger_access_fault_repeated_breakpoint_then_restore_pmp_adjacent_preserved_spike_corner()
{
    TEST_START();

    enum test_page low_page = VSRWX_GRX;
    enum test_page high_page = VSRWX_GR;
    uintptr_t vaddr = ai_micro_cross_page_trigger_addr(low_page, 4U);
    uintptr_t second_half_vaddr = vaddr + 4U;
    uintptr_t low_adjacent_phys = phys_page_base(low_page) + PAGE_SIZE - (2 * sizeof(uint64_t));
    uintptr_t low_phys = ai_micro_cross_page_trigger_page_end_phys(low_page);
    uintptr_t high_phys = phys_page_base(high_page);
    uintptr_t high_adjacent_phys = high_phys + sizeof(uint64_t);
    uintptr_t deny_start = high_phys;
    uintptr_t deny_end = deny_start + PAGE_SIZE;
    const uint64_t low_adjacent = 0x1234432155aa55aaULL;
    const uint64_t low_pattern = 0x98a9bacbdcedfe10ULL;
    const uint64_t high_pattern = 0x21436587a9cbed0fULL;
    const uint64_t high_adjacent = 0xaa55aa5512344321ULL;
    const uint64_t recovery_expected =
        ((high_pattern & 0xffffffffULL) << 32) | (low_pattern >> 32);
    uint64_t saved_mstatus;

    goto_priv(PRIV_M);
    write64(low_adjacent_phys, low_adjacent);
    write64(low_phys, low_pattern);
    write64(high_phys, high_pattern);
    write64(high_adjacent_phys, high_adjacent);

    ai_micro_cross_page_configure_pmp_window(deny_start, deny_end, AI_MICRO_PMP_TOR_NO_R);

    goto_priv(PRIV_HS);
    hspt_init();
    sfence_vma();

    ai_micro_cross_page_clear_mcontrol6_slot(0);
    ai_micro_cross_page_clear_mcontrol6_slot(1);
    ai_micro_cross_page_arm_mcontrol6_slot(0, vaddr,
        AI_MICRO_CROSS_PAGE_MCONTROL6_LOAD);
    ai_micro_cross_page_arm_mcontrol6_slot(1, second_half_vaddr,
        AI_MICRO_CROSS_PAGE_MCONTROL6_LOAD);
    saved_mstatus = ai_micro_cross_page_enable_machine_breakpoint_delivery();

    goto_priv(PRIV_HS);
    TEST_SETUP_EXCEPT();
    ai_micro_cross_page_execute_load_once(vaddr);
    TEST_ASSERT("with both the original cross-page misaligned load address slot and the true second-half-start slot armed, the first upper-half access-fault split-load attempt still reports breakpoint on Spike because only the original address matches in the adjacent-preservation corner",
        excpt.triggered == true &&
        excpt.cause == CAUSE_BKP
    );
    ai_micro_cross_page_restore_machine_status(saved_mstatus);

    goto_priv(PRIV_M);
    TEST_ASSERT("the first trapped upper-half access-fault split-load attempt still leaves both boundary doublewords and their adjacent neighbors unchanged before PMP permission is restored",
        read64(low_adjacent_phys) == low_adjacent &&
        read64(low_phys) == low_pattern &&
        read64(high_phys) == high_pattern &&
        read64(high_adjacent_phys) == high_adjacent
    );

    goto_priv(PRIV_HS);
    hspt_init();
    sfence_vma();

    saved_mstatus = ai_micro_cross_page_enable_machine_breakpoint_delivery();
    goto_priv(PRIV_HS);
    TEST_SETUP_EXCEPT();
    ai_micro_cross_page_execute_load_once(vaddr);
    TEST_ASSERT("with the same original misaligned-load slot and second-half-start slot still armed, the second upper-half access-fault split-load attempt still reports breakpoint on Spike in the adjacent-preservation corner",
        excpt.triggered == true &&
        excpt.cause == CAUSE_BKP
    );
    ai_micro_cross_page_restore_machine_status(saved_mstatus);

    goto_priv(PRIV_M);
    TEST_ASSERT("the second trapped upper-half access-fault split-load attempt still leaves both boundary doublewords and their adjacent neighbors unchanged before PMP permission is restored",
        read64(low_adjacent_phys) == low_adjacent &&
        read64(low_phys) == low_pattern &&
        read64(high_phys) == high_pattern &&
        read64(high_adjacent_phys) == high_adjacent
    );

    goto_priv(PRIV_HS);
    hspt_init();
    sfence_vma();

    ai_micro_cross_page_clear_mcontrol6_slot(0);
    saved_mstatus = ai_micro_cross_page_enable_machine_breakpoint_delivery();
    goto_priv(PRIV_HS);
    TEST_SETUP_EXCEPT();
    ai_micro_cross_page_execute_load_once(vaddr);
    TEST_ASSERT("after two prior breakpoint hits and then clearing only the original misaligned-load slot, the remaining second-half-start slot still stays ignored on Spike, so the split load first falls back to the upper-half load access fault before PMP permission is restored in the adjacent-preservation corner",
        excpt.triggered == true &&
        excpt.cause == CAUSE_LAF &&
        excpt.tval == second_half_vaddr
    );
    ai_micro_cross_page_restore_machine_status(saved_mstatus);

    goto_priv(PRIV_M);
    TEST_ASSERT("before restoring PMP read permission, repeated dual-address-trigger hits and the fallback upper-half load access fault leave both boundary backing doublewords and their adjacent neighbors unchanged",
        read64(low_adjacent_phys) == low_adjacent &&
        read64(low_phys) == low_pattern &&
        read64(high_phys) == high_pattern &&
        read64(high_adjacent_phys) == high_adjacent
    );
    ai_micro_cross_page_clear_pmp_window();

    goto_priv(PRIV_HS);
    hspt_init();
    sfence_vma();

    TEST_SETUP_EXCEPT();
    uint64_t observed = ai_micro_cross_page_execute_load_once(vaddr);
    TEST_ASSERT("after repeated dual-address-trigger breakpoint hits on the upper-half access-fault split load, restoring PMP read permission lets the same split load recover while the second-half-start slot still stays ignored on Spike and adjacent words stay untouched",
        excpt.triggered == false &&
        observed == recovery_expected
    );

    goto_priv(PRIV_M);
    TEST_ASSERT("the recovered split-load path reads the merged payload without dirtying either adjacent boundary word after the access-fault recovery sequence",
        read64(low_adjacent_phys) == low_adjacent &&
        read64(low_phys) == low_pattern &&
        read64(high_phys) == high_pattern &&
        read64(high_adjacent_phys) == high_adjacent
    );

    ai_micro_cross_page_clear_mcontrol6_slot(1);
    TEST_END();
}

bool ai_micro_hs_store_cross_page_high_half_dual_address_trigger_page_fault_repeated_breakpoint_then_fix_high_half_adjacent_preserved_spike_corner()
{
    TEST_START();

    enum test_page low_page = VSRW_GX;
    enum test_page high_page = VSRX_GRWX;
    uintptr_t vaddr = ai_micro_cross_page_trigger_addr(low_page, 4U);
    uintptr_t second_half_vaddr = vaddr + 4U;
    uintptr_t low_adjacent_phys = phys_page_base(low_page) + PAGE_SIZE - (2 * sizeof(uint64_t));
    uintptr_t low_phys = ai_micro_cross_page_trigger_page_end_phys(low_page);
    uintptr_t high_phys = phys_page_base(high_page);
    uintptr_t high_adjacent_phys = high_phys + sizeof(uint64_t);
    const uint64_t low_adjacent = 0x3141592653589793ULL;
    const uint64_t low_pattern = 0x1122334455667788ULL;
    const uint64_t high_pattern = 0x13579bdf2468ace0ULL;
    const uint64_t high_adjacent = 0x2718281828459045ULL;
    const uint64_t fault_store = 0xfedcba9876543210ULL;
    const uint64_t expected_low_after_fault = low_pattern;
    const uint64_t recovery_store = 0x0123456789abcdefULL;
    const uint64_t expected_low_after_store =
        (low_pattern & 0xffffffffULL) | ((recovery_store & 0xffffffffULL) << 32);
    const uint64_t expected_high_after_store =
        (high_pattern & 0xffffffff00000000ULL) | (recovery_store >> 32);
    bool trap_triggered;
    uint64_t trap_cause;
    uint64_t trap_tval;
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
    ai_micro_cross_page_arm_mcontrol6_slot(0, vaddr,
        AI_MICRO_CROSS_PAGE_MCONTROL6_STORE);
    ai_micro_cross_page_arm_mcontrol6_slot(1, second_half_vaddr,
        AI_MICRO_CROSS_PAGE_MCONTROL6_STORE);
    saved_mstatus = ai_micro_cross_page_enable_machine_breakpoint_delivery();

    goto_priv(PRIV_HS);
    TEST_SETUP_EXCEPT();
    ai_micro_cross_page_execute_store_once(vaddr, fault_store);
    TEST_ASSERT("with both the original cross-page misaligned store address slot and the true second-half-start slot armed, the first upper-half page-fault split-store attempt still reports breakpoint on Spike because only the original address matches in the adjacent-preservation corner",
        excpt.triggered == true &&
        excpt.cause == CAUSE_BKP
    );

    goto_priv(PRIV_M);
    TEST_ASSERT("the first trapped upper-half page-fault split-store attempt still leaves both page fragments and their adjacent neighbors unchanged before any later repair",
        read64(low_adjacent_phys) == low_adjacent &&
        read64(low_phys) == low_pattern &&
        read64(high_phys) == high_pattern &&
        read64(high_adjacent_phys) == high_adjacent
    );
    ai_micro_cross_page_restore_machine_status(saved_mstatus);

    goto_priv(PRIV_HS);
    hspt_init();
    sfence_vma();

    saved_mstatus = ai_micro_cross_page_enable_machine_breakpoint_delivery();
    goto_priv(PRIV_HS);
    TEST_SETUP_EXCEPT();
    ai_micro_cross_page_execute_store_once(vaddr, fault_store);
    TEST_ASSERT("with the same original misaligned-store slot and second-half-start slot still armed, the second upper-half page-fault split-store attempt still reports breakpoint on Spike in the adjacent-preservation corner",
        excpt.triggered == true &&
        excpt.cause == CAUSE_BKP
    );

    goto_priv(PRIV_M);
    TEST_ASSERT("the second trapped upper-half page-fault split-store attempt still leaves both page fragments and their adjacent neighbors unchanged before any later repair",
        read64(low_adjacent_phys) == low_adjacent &&
        read64(low_phys) == low_pattern &&
        read64(high_phys) == high_pattern &&
        read64(high_adjacent_phys) == high_adjacent
    );
    ai_micro_cross_page_restore_machine_status(saved_mstatus);

    goto_priv(PRIV_HS);
    hspt_init();
    sfence_vma();

    ai_micro_cross_page_clear_mcontrol6_slot(0);
    saved_mstatus = ai_micro_cross_page_enable_machine_breakpoint_delivery();
    goto_priv(PRIV_HS);
    TEST_SETUP_EXCEPT();
    ai_micro_cross_page_execute_store_once(vaddr, fault_store);
    trap_triggered = excpt.triggered;
    trap_cause = excpt.cause;
    trap_tval = excpt.tval;
    ai_micro_cross_page_restore_machine_status(saved_mstatus);

    goto_priv(PRIV_M);
    TEST_ASSERT("after two prior breakpoint hits and then clearing only the original misaligned-store slot, the remaining second-half-start slot still stays ignored on Spike, so the split store first falls back to the upper-half store page-fault corner before repair while adjacent words stay untouched",
        trap_triggered == true &&
        trap_cause == CAUSE_SPF &&
        trap_tval == second_half_vaddr &&
        read64(low_adjacent_phys) == low_adjacent &&
        read64(low_phys) == expected_low_after_fault &&
        read64(high_phys) == high_pattern &&
        read64(high_adjacent_phys) == high_adjacent
    );
    ai_micro_cross_page_set_hs_leaf(high_page, phys_page_base(high_page),
        PTE_V | PTE_AD | PTE_RWX);
    sfence_vma();
    write64(low_adjacent_phys, low_adjacent);
    write64(low_phys, low_pattern);
    write64(high_phys, high_pattern);
    write64(high_adjacent_phys, high_adjacent);

    goto_priv(PRIV_HS);
    TEST_SETUP_EXCEPT();
    ai_micro_cross_page_execute_store_once(vaddr, recovery_store);
    TEST_ASSERT("after repeated dual-address-trigger breakpoint hits on the upper-half page-fault split store, fixing the upper leaf lets the same split store recover while the second-half-start slot still stays ignored on Spike and adjacent words stay untouched",
        excpt.triggered == false
    );

    goto_priv(PRIV_M);
    TEST_ASSERT("after the recovered split-store path, the low boundary fragment matches the lower half of the committed payload while preserving the adjacent low-page word",
        read64(low_adjacent_phys) == low_adjacent &&
        read64(low_phys) == expected_low_after_store
    );
    TEST_ASSERT("after the recovered split-store path, the high boundary fragment matches the upper half of the committed payload while preserving the adjacent high-page word",
        read64(high_phys) == expected_high_after_store &&
        read64(high_adjacent_phys) == high_adjacent
    );

    ai_micro_cross_page_clear_mcontrol6_slot(1);
    TEST_END();
}

bool ai_micro_hs_store_cross_page_high_half_dual_address_trigger_access_fault_repeated_breakpoint_then_restore_pmp_adjacent_preserved_spike_corner()
{
    TEST_START();

    enum test_page low_page = VSRW_GRW;
    enum test_page high_page = VSRW_GRX;
    uintptr_t vaddr = ai_micro_cross_page_trigger_addr(low_page, 4U);
    uintptr_t second_half_vaddr = vaddr + 4U;
    uintptr_t low_adjacent_phys = phys_page_base(low_page) + PAGE_SIZE - (2 * sizeof(uint64_t));
    uintptr_t low_phys = ai_micro_cross_page_trigger_page_end_phys(low_page);
    uintptr_t high_phys = phys_page_base(high_page);
    uintptr_t high_adjacent_phys = high_phys + sizeof(uint64_t);
    uintptr_t deny_start = high_phys;
    uintptr_t deny_end = deny_start + PAGE_SIZE;
    const uint64_t low_adjacent = 0x0badc0de55aa1234ULL;
    const uint64_t low_pattern = 0x1122334455667788ULL;
    const uint64_t high_pattern = 0x13579bdf2468ace0ULL;
    const uint64_t high_adjacent = 0x1234fedc5678ba90ULL;
    const uint64_t fault_store = 0xfedcba9876543210ULL;
    const uint64_t expected_low_after_fault = low_pattern;
    const uint64_t recovery_store = 0x0123456789abcdefULL;
    const uint64_t expected_low_after_store =
        (low_pattern & 0xffffffffULL) | ((recovery_store & 0xffffffffULL) << 32);
    const uint64_t expected_high_after_store =
        (high_pattern & 0xffffffff00000000ULL) | (recovery_store >> 32);
    bool trap_triggered;
    uint64_t trap_cause;
    uint64_t trap_tval;
    uint64_t saved_mstatus;

    goto_priv(PRIV_M);
    write64(low_adjacent_phys, low_adjacent);
    write64(low_phys, low_pattern);
    write64(high_phys, high_pattern);
    write64(high_adjacent_phys, high_adjacent);

    ai_micro_cross_page_configure_pmp_window(deny_start, deny_end, AI_MICRO_PMP_TOR_NO_W);

    goto_priv(PRIV_HS);
    hspt_init();
    sfence_vma();

    ai_micro_cross_page_clear_mcontrol6_slot(0);
    ai_micro_cross_page_clear_mcontrol6_slot(1);
    ai_micro_cross_page_arm_mcontrol6_slot(0, vaddr,
        AI_MICRO_CROSS_PAGE_MCONTROL6_STORE);
    ai_micro_cross_page_arm_mcontrol6_slot(1, second_half_vaddr,
        AI_MICRO_CROSS_PAGE_MCONTROL6_STORE);
    saved_mstatus = ai_micro_cross_page_enable_machine_breakpoint_delivery();

    goto_priv(PRIV_HS);
    TEST_SETUP_EXCEPT();
    ai_micro_cross_page_execute_store_once(vaddr, fault_store);
    TEST_ASSERT("with both the original cross-page misaligned store address slot and the true second-half-start slot armed, the first upper-half access-fault split-store attempt still reports breakpoint on Spike because only the original address matches in the adjacent-preservation corner",
        excpt.triggered == true &&
        excpt.cause == CAUSE_BKP
    );

    goto_priv(PRIV_M);
    TEST_ASSERT("the first trapped upper-half access-fault split-store attempt still leaves both page fragments and their adjacent neighbors unchanged before PMP permission is restored",
        read64(low_adjacent_phys) == low_adjacent &&
        read64(low_phys) == low_pattern &&
        read64(high_phys) == high_pattern &&
        read64(high_adjacent_phys) == high_adjacent
    );
    ai_micro_cross_page_restore_machine_status(saved_mstatus);

    goto_priv(PRIV_HS);
    hspt_init();
    sfence_vma();

    saved_mstatus = ai_micro_cross_page_enable_machine_breakpoint_delivery();
    goto_priv(PRIV_HS);
    TEST_SETUP_EXCEPT();
    ai_micro_cross_page_execute_store_once(vaddr, fault_store);
    TEST_ASSERT("with the same original misaligned-store slot and second-half-start slot still armed, the second upper-half access-fault split-store attempt still reports breakpoint on Spike in the adjacent-preservation corner",
        excpt.triggered == true &&
        excpt.cause == CAUSE_BKP
    );

    goto_priv(PRIV_M);
    TEST_ASSERT("the second trapped upper-half access-fault split-store attempt still leaves both page fragments and their adjacent neighbors unchanged before PMP permission is restored",
        read64(low_adjacent_phys) == low_adjacent &&
        read64(low_phys) == low_pattern &&
        read64(high_phys) == high_pattern &&
        read64(high_adjacent_phys) == high_adjacent
    );
    ai_micro_cross_page_restore_machine_status(saved_mstatus);

    goto_priv(PRIV_HS);
    hspt_init();
    sfence_vma();

    ai_micro_cross_page_clear_mcontrol6_slot(0);
    saved_mstatus = ai_micro_cross_page_enable_machine_breakpoint_delivery();
    goto_priv(PRIV_HS);
    TEST_SETUP_EXCEPT();
    ai_micro_cross_page_execute_store_once(vaddr, fault_store);
    trap_triggered = excpt.triggered;
    trap_cause = excpt.cause;
    trap_tval = excpt.tval;
    ai_micro_cross_page_restore_machine_status(saved_mstatus);

    goto_priv(PRIV_M);
    TEST_ASSERT("after two prior breakpoint hits and then clearing only the original misaligned-store slot, the remaining second-half-start slot still stays ignored on Spike, so the split store first falls back to the upper-half store access-fault corner before PMP permission is restored while adjacent words stay untouched",
        trap_triggered == true &&
        trap_cause == CAUSE_SAF &&
        trap_tval == second_half_vaddr &&
        read64(low_adjacent_phys) == low_adjacent &&
        read64(low_phys) == expected_low_after_fault &&
        read64(high_phys) == high_pattern &&
        read64(high_adjacent_phys) == high_adjacent
    );
    ai_micro_cross_page_clear_pmp_window();
    write64(low_adjacent_phys, low_adjacent);
    write64(low_phys, low_pattern);
    write64(high_phys, high_pattern);
    write64(high_adjacent_phys, high_adjacent);

    goto_priv(PRIV_HS);
    hspt_init();
    sfence_vma();

    TEST_SETUP_EXCEPT();
    ai_micro_cross_page_execute_store_once(vaddr, recovery_store);
    TEST_ASSERT("after repeated dual-address-trigger breakpoint hits on the upper-half access-fault split store, restoring PMP write permission lets the same split store recover while the second-half-start slot still stays ignored on Spike and adjacent words stay untouched",
        excpt.triggered == false
    );

    goto_priv(PRIV_M);
    TEST_ASSERT("after the recovered split-store path, the low boundary fragment matches the lower half of the committed payload while preserving the adjacent low-page word after the access-fault recovery sequence",
        read64(low_adjacent_phys) == low_adjacent &&
        read64(low_phys) == expected_low_after_store
    );
    TEST_ASSERT("after the recovered split-store path, the high boundary fragment matches the upper half of the committed payload while preserving the adjacent high-page word after the access-fault recovery sequence",
        read64(high_phys) == expected_high_after_store &&
        read64(high_adjacent_phys) == high_adjacent
    );

    ai_micro_cross_page_clear_mcontrol6_slot(1);
    TEST_END();
}
