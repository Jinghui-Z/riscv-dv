#include "ai_micro_cross_16b_cacheable_common.h"

bool ai_micro_hs_store_cross_16b_execute_trigger_with_second_half_address_slot_page_fault_repeated_breakpoint_then_fault_no_side_effect_spike_corner()
{
    TEST_START();

    uintptr_t paddr = phys_page_base(VSI_GI);
    uintptr_t vaddr = ai_micro_cross_16b_cacheable_addr(VSI_GI);
    uintptr_t second_half_vaddr = ai_micro_cross_16b_second_half_addr(VSI_GI);
    uintptr_t trigger_pc = (uintptr_t)ai_micro_cross_16b_execute_store_once;
    const uint64_t low_pattern = AI_MICRO_CROSS_16B_LOW_PATTERN;
    const uint64_t high_pattern = AI_MICRO_CROSS_16B_HIGH_PATTERN;
    bool trap_triggered;
    uint64_t trap_cause;
    uint64_t trap_tval;

    goto_priv(PRIV_M);
    write64(paddr + 8U, low_pattern);
    write64(paddr + 16U, high_pattern);

    goto_priv(PRIV_HS);
    hspt_init();
    sfence_vma();

    ai_micro_cross_16b_clear_mcontrol6_slot(0);
    ai_micro_cross_16b_clear_mcontrol6_slot(1);
    ai_micro_cross_16b_arm_mcontrol6_slot(0, trigger_pc, AI_MICRO_CROSS_16B_MCONTROL6_EXECUTE);
    ai_micro_cross_16b_arm_mcontrol6_slot(1, second_half_vaddr, AI_MICRO_CROSS_16B_MCONTROL6_STORE);
    uint64_t saved_mstatus = ai_micro_cross_16b_enable_machine_breakpoint_delivery();

    goto_priv(PRIV_HS);
    TEST_SETUP_EXCEPT();
    ai_micro_cross_16b_execute_store_once(vaddr, AI_MICRO_CROSS_16B_STORE_PATTERN);
    TEST_ASSERT("with execute-PC trigger armed and the address slot programmed to the true second-half start address, the first cross-16B store page-fault attempt still reports breakpoint on Spike",
        excpt.triggered == true &&
        excpt.cause == CAUSE_BKP
    );

    goto_priv(PRIV_M);
    TEST_ASSERT("the first trapped cross-16B store page-fault attempt still leaves both aligned doublewords unchanged while only the execute-PC slot is effective on Spike",
        read64(paddr + 8U) == low_pattern &&
        read64(paddr + 16U) == high_pattern
    );
    ai_micro_cross_16b_restore_machine_status(saved_mstatus);

    goto_priv(PRIV_HS);
    hspt_init();
    sfence_vma();

    saved_mstatus = ai_micro_cross_16b_enable_machine_breakpoint_delivery();
    goto_priv(PRIV_HS);
    TEST_SETUP_EXCEPT();
    ai_micro_cross_16b_execute_store_once(vaddr, AI_MICRO_CROSS_16B_STORE_PATTERN);
    TEST_ASSERT("with the same execute-PC and second-half-start slots still armed, the second cross-16B store page-fault attempt still reports breakpoint on Spike",
        excpt.triggered == true &&
        excpt.cause == CAUSE_BKP
    );

    goto_priv(PRIV_M);
    TEST_ASSERT("the second trapped cross-16B store page-fault attempt still leaves both aligned doublewords unchanged while only the execute-PC slot is effective on Spike",
        read64(paddr + 8U) == low_pattern &&
        read64(paddr + 16U) == high_pattern
    );
    ai_micro_cross_16b_restore_machine_status(saved_mstatus);

    goto_priv(PRIV_HS);
    hspt_init();
    sfence_vma();

    ai_micro_cross_16b_clear_mcontrol6_slot(0);
    saved_mstatus = ai_micro_cross_16b_enable_machine_breakpoint_delivery();
    goto_priv(PRIV_HS);
    TEST_SETUP_EXCEPT();
    ai_micro_cross_16b_execute_store_once(vaddr, AI_MICRO_CROSS_16B_STORE_PATTERN);
    trap_triggered = excpt.triggered;
    trap_cause = excpt.cause;
    trap_tval = excpt.tval;
    ai_micro_cross_16b_restore_machine_status(saved_mstatus);

    goto_priv(PRIV_M);
    TEST_ASSERT("after two prior execute-trigger breakpoint hits, clearing only the execute-PC slot still leaves the second-half-start address slot ignored on Spike, so the cross-16B store falls back to store page fault with no side effects",
        trap_triggered == true &&
        trap_cause == CAUSE_SPF &&
        trap_tval == vaddr &&
        read64(paddr + 8U) == low_pattern &&
        read64(paddr + 16U) == high_pattern
    );

    ai_micro_cross_16b_clear_mcontrol6_slot(1);
    TEST_END();
}

bool ai_micro_hs_store_cross_16b_execute_trigger_with_second_half_address_slot_page_fault_repeated_breakpoint_then_fault_no_side_effect_adjacent_preserved_spike_corner()
{
    TEST_START();

    uintptr_t paddr = phys_page_base(VSI_GI);
    uintptr_t vaddr = ai_micro_cross_16b_cacheable_addr(VSI_GI);
    uintptr_t second_half_vaddr = ai_micro_cross_16b_second_half_addr(VSI_GI);
    uintptr_t leading_phys = paddr;
    uintptr_t low_phys = paddr + 8U;
    uintptr_t high_phys = paddr + 16U;
    uintptr_t trailing_phys = paddr + 24U;
    uintptr_t trigger_pc = (uintptr_t)ai_micro_cross_16b_execute_store_once;
    const uint64_t leading_pattern = 0x1234567890abcdefULL;
    const uint64_t low_pattern = AI_MICRO_CROSS_16B_LOW_PATTERN;
    const uint64_t high_pattern = AI_MICRO_CROSS_16B_HIGH_PATTERN;
    const uint64_t trailing_pattern = 0x0fedcba987654321ULL;
    bool trap_triggered;
    uint64_t trap_cause;
    uint64_t trap_tval;
    uint64_t saved_mstatus;

    goto_priv(PRIV_M);
    write64(leading_phys, leading_pattern);
    write64(low_phys, low_pattern);
    write64(high_phys, high_pattern);
    write64(trailing_phys, trailing_pattern);

    goto_priv(PRIV_HS);
    hspt_init();
    sfence_vma();

    ai_micro_cross_16b_clear_mcontrol6_slot(0);
    ai_micro_cross_16b_clear_mcontrol6_slot(1);
    ai_micro_cross_16b_arm_mcontrol6_slot(0, trigger_pc, AI_MICRO_CROSS_16B_MCONTROL6_EXECUTE);
    ai_micro_cross_16b_arm_mcontrol6_slot(1, second_half_vaddr, AI_MICRO_CROSS_16B_MCONTROL6_STORE);
    saved_mstatus = ai_micro_cross_16b_enable_machine_breakpoint_delivery();

    goto_priv(PRIV_HS);
    TEST_SETUP_EXCEPT();
    ai_micro_cross_16b_execute_store_once(vaddr, AI_MICRO_CROSS_16B_STORE_PATTERN);
    TEST_ASSERT("with execute-PC trigger armed and the address slot programmed to the true second-half start address, the first cross-16B store page-fault attempt still reports breakpoint on Spike in the adjacent-preservation corner",
        excpt.triggered == true &&
        excpt.cause == CAUSE_BKP
    );

    goto_priv(PRIV_M);
    TEST_ASSERT("the first trapped cross-16B store page-fault attempt still leaves both aligned doublewords and their immediate neighbors unchanged while only the execute-PC slot is effective on Spike",
        read64(leading_phys) == leading_pattern &&
        read64(low_phys) == low_pattern &&
        read64(high_phys) == high_pattern &&
        read64(trailing_phys) == trailing_pattern
    );
    ai_micro_cross_16b_restore_machine_status(saved_mstatus);

    goto_priv(PRIV_HS);
    hspt_init();
    sfence_vma();

    saved_mstatus = ai_micro_cross_16b_enable_machine_breakpoint_delivery();
    goto_priv(PRIV_HS);
    TEST_SETUP_EXCEPT();
    ai_micro_cross_16b_execute_store_once(vaddr, AI_MICRO_CROSS_16B_STORE_PATTERN);
    TEST_ASSERT("with the same execute-PC and second-half-start slots still armed, the second cross-16B store page-fault attempt still reports breakpoint on Spike in the adjacent-preservation corner",
        excpt.triggered == true &&
        excpt.cause == CAUSE_BKP
    );

    goto_priv(PRIV_M);
    TEST_ASSERT("the second trapped cross-16B store page-fault attempt still leaves both aligned doublewords and their immediate neighbors unchanged while only the execute-PC slot is effective on Spike",
        read64(leading_phys) == leading_pattern &&
        read64(low_phys) == low_pattern &&
        read64(high_phys) == high_pattern &&
        read64(trailing_phys) == trailing_pattern
    );
    ai_micro_cross_16b_restore_machine_status(saved_mstatus);

    goto_priv(PRIV_HS);
    hspt_init();
    sfence_vma();

    ai_micro_cross_16b_clear_mcontrol6_slot(0);
    saved_mstatus = ai_micro_cross_16b_enable_machine_breakpoint_delivery();
    goto_priv(PRIV_HS);
    TEST_SETUP_EXCEPT();
    ai_micro_cross_16b_execute_store_once(vaddr, AI_MICRO_CROSS_16B_STORE_PATTERN);
    trap_triggered = excpt.triggered;
    trap_cause = excpt.cause;
    trap_tval = excpt.tval;
    ai_micro_cross_16b_restore_machine_status(saved_mstatus);

    goto_priv(PRIV_M);
    TEST_ASSERT("after two prior execute-trigger breakpoint hits, clearing only the execute-PC slot still leaves the second-half-start address slot ignored on Spike, so the cross-16B store falls back to store page fault with no side effects in the adjacent-preservation corner",
        trap_triggered == true &&
        trap_cause == CAUSE_SPF &&
        trap_tval == vaddr &&
        read64(leading_phys) == leading_pattern &&
        read64(low_phys) == low_pattern &&
        read64(high_phys) == high_pattern &&
        read64(trailing_phys) == trailing_pattern
    );

    ai_micro_cross_16b_clear_mcontrol6_slot(1);
    TEST_END();
}

bool ai_micro_hs_store_cross_16b_execute_and_address_trigger_both_breakpoint_corner()
{
    TEST_START();

    uintptr_t paddr = phys_page_base(VSRWX_GRWX);
    uintptr_t vaddr = ai_micro_cross_16b_cacheable_addr(VSRWX_GRWX);
    uintptr_t trigger_pc = (uintptr_t)ai_micro_cross_16b_execute_store_once;

    goto_priv(PRIV_M);
    write64(paddr + 8U, AI_MICRO_CROSS_16B_LOW_PATTERN);
    write64(paddr + 16U, AI_MICRO_CROSS_16B_HIGH_PATTERN);

    goto_priv(PRIV_HS);
    hspt_init();
    sfence_vma();

    ai_micro_cross_16b_clear_mcontrol6_slot(0);
    ai_micro_cross_16b_clear_mcontrol6_slot(1);
    ai_micro_cross_16b_arm_mcontrol6_slot(0, trigger_pc, AI_MICRO_CROSS_16B_MCONTROL6_EXECUTE);
    ai_micro_cross_16b_arm_mcontrol6_slot(1, vaddr, AI_MICRO_CROSS_16B_MCONTROL6_STORE);
    uint64_t saved_mstatus = ai_micro_cross_16b_enable_machine_breakpoint_delivery();

    goto_priv(PRIV_HS);
    TEST_SETUP_EXCEPT();
    ai_micro_cross_16b_execute_store_once(vaddr, AI_MICRO_CROSS_16B_STORE_PATTERN);
    TEST_ASSERT("an HS cross-16B cacheable store still reports breakpoint when both execute-PC and address trigger slots match the same split store",
        excpt.triggered == true &&
        excpt.cause == CAUSE_BKP
    );

    goto_priv(PRIV_M);
    TEST_ASSERT("with both trigger slots armed, the trapped HS cross-16B store still leaves the lower aligned doubleword unchanged",
        read64(paddr + 8U) == AI_MICRO_CROSS_16B_LOW_PATTERN
    );
    TEST_ASSERT("with both trigger slots armed, the trapped HS cross-16B store still leaves the upper aligned doubleword unchanged",
        read64(paddr + 16U) == AI_MICRO_CROSS_16B_HIGH_PATTERN
    );

    ai_micro_cross_16b_restore_machine_status(saved_mstatus);

    ai_micro_cross_16b_clear_mcontrol6_slot(0);
    saved_mstatus = ai_micro_cross_16b_enable_machine_breakpoint_delivery();
    goto_priv(PRIV_HS);
    TEST_SETUP_EXCEPT();
    ai_micro_cross_16b_execute_store_once(vaddr, AI_MICRO_CROSS_16B_STORE_PATTERN);
    TEST_ASSERT("after clearing only the execute-PC slot, the remaining cross-16B store address trigger still reports breakpoint",
        excpt.triggered == true &&
        excpt.cause == CAUSE_BKP
    );

    goto_priv(PRIV_M);
    TEST_ASSERT("with only the address trigger slot armed, the trapped HS cross-16B store still leaves the lower aligned doubleword unchanged",
        read64(paddr + 8U) == AI_MICRO_CROSS_16B_LOW_PATTERN
    );
    TEST_ASSERT("with only the address trigger slot armed, the trapped HS cross-16B store still leaves the upper aligned doubleword unchanged",
        read64(paddr + 16U) == AI_MICRO_CROSS_16B_HIGH_PATTERN
    );

    ai_micro_cross_16b_restore_machine_status(saved_mstatus);

    ai_micro_cross_16b_clear_mcontrol6_slot(1);
    goto_priv(PRIV_HS);
    TEST_SETUP_EXCEPT();
    ai_micro_cross_16b_execute_store_once(vaddr, AI_MICRO_CROSS_16B_STORE_PATTERN);
    TEST_ASSERT("after clearing both trigger slots, the same HS cross-16B cacheable store completes normally",
        excpt.triggered == false
    );

    goto_priv(PRIV_M);
    TEST_ASSERT("after clearing both trigger slots, the lower aligned doubleword reflects the split store bytes",
        read64(paddr + 8U) == 0xef66554433221100ULL
    );
    TEST_ASSERT("after clearing both trigger slots, the upper aligned doubleword reflects the split store bytes",
        read64(paddr + 16U) == 0xff0123456789abcdULL
    );

    TEST_END();
}

bool ai_micro_hs_store_cross_16b_execute_and_address_trigger_repeated_breakpoint_then_completion_corner()
{
    TEST_START();

    uintptr_t paddr = phys_page_base(VSRWX_GRWX);
    uintptr_t vaddr = ai_micro_cross_16b_cacheable_addr(VSRWX_GRWX);
    uintptr_t trigger_pc = (uintptr_t)ai_micro_cross_16b_execute_store_once;

    goto_priv(PRIV_M);
    write64(paddr + 8U, AI_MICRO_CROSS_16B_LOW_PATTERN);
    write64(paddr + 16U, AI_MICRO_CROSS_16B_HIGH_PATTERN);

    goto_priv(PRIV_HS);
    hspt_init();
    sfence_vma();

    ai_micro_cross_16b_clear_mcontrol6_slot(0);
    ai_micro_cross_16b_clear_mcontrol6_slot(1);
    ai_micro_cross_16b_arm_mcontrol6_slot(0, trigger_pc, AI_MICRO_CROSS_16B_MCONTROL6_EXECUTE);
    ai_micro_cross_16b_arm_mcontrol6_slot(1, vaddr, AI_MICRO_CROSS_16B_MCONTROL6_STORE);
    uint64_t saved_mstatus = ai_micro_cross_16b_enable_machine_breakpoint_delivery();

    goto_priv(PRIV_HS);
    TEST_SETUP_EXCEPT();
    ai_micro_cross_16b_execute_store_once(vaddr, AI_MICRO_CROSS_16B_STORE_PATTERN);
    TEST_ASSERT("with both execute-PC and address trigger slots armed on the same cross-16B cacheable store, the first access reports breakpoint",
        excpt.triggered == true &&
        excpt.cause == CAUSE_BKP
    );

    goto_priv(PRIV_M);
    TEST_ASSERT("the first trapped cross-16B cacheable store still leaves both aligned doublewords unchanged",
        read64(paddr + 8U) == AI_MICRO_CROSS_16B_LOW_PATTERN &&
        read64(paddr + 16U) == AI_MICRO_CROSS_16B_HIGH_PATTERN
    );
    ai_micro_cross_16b_restore_machine_status(saved_mstatus);

    saved_mstatus = ai_micro_cross_16b_enable_machine_breakpoint_delivery();
    goto_priv(PRIV_HS);
    TEST_SETUP_EXCEPT();
    ai_micro_cross_16b_execute_store_once(vaddr, AI_MICRO_CROSS_16B_STORE_PATTERN);
    TEST_ASSERT("with both execute-PC and address trigger slots still armed on that same cross-16B cacheable store, the second access still reports breakpoint",
        excpt.triggered == true &&
        excpt.cause == CAUSE_BKP
    );

    goto_priv(PRIV_M);
    TEST_ASSERT("the second trapped cross-16B cacheable store still leaves both aligned doublewords unchanged",
        read64(paddr + 8U) == AI_MICRO_CROSS_16B_LOW_PATTERN &&
        read64(paddr + 16U) == AI_MICRO_CROSS_16B_HIGH_PATTERN
    );
    ai_micro_cross_16b_restore_machine_status(saved_mstatus);

    ai_micro_cross_16b_clear_mcontrol6_slot(0);
    saved_mstatus = ai_micro_cross_16b_enable_machine_breakpoint_delivery();
    goto_priv(PRIV_HS);
    TEST_SETUP_EXCEPT();
    ai_micro_cross_16b_execute_store_once(vaddr, AI_MICRO_CROSS_16B_STORE_PATTERN);
    TEST_ASSERT("after two prior dual-slot breakpoint hits, clearing only the execute-PC slot still leaves the cross-16B store address trigger active",
        excpt.triggered == true &&
        excpt.cause == CAUSE_BKP
    );

    goto_priv(PRIV_M);
    TEST_ASSERT("with only the address trigger slot armed, the trapped cross-16B cacheable store still leaves both aligned doublewords unchanged",
        read64(paddr + 8U) == AI_MICRO_CROSS_16B_LOW_PATTERN &&
        read64(paddr + 16U) == AI_MICRO_CROSS_16B_HIGH_PATTERN
    );
    ai_micro_cross_16b_restore_machine_status(saved_mstatus);

    ai_micro_cross_16b_clear_mcontrol6_slot(1);
    goto_priv(PRIV_HS);
    TEST_SETUP_EXCEPT();
    ai_micro_cross_16b_execute_store_once(vaddr, AI_MICRO_CROSS_16B_STORE_PATTERN);
    TEST_ASSERT("after clearing both trigger slots, the same cross-16B cacheable store completes normally",
        excpt.triggered == false
    );

    goto_priv(PRIV_M);
    TEST_ASSERT("after clearing both trigger slots, the lower aligned doubleword reflects the split store bytes",
        read64(paddr + 8U) == 0xef66554433221100ULL
    );
    TEST_ASSERT("after clearing both trigger slots, the upper aligned doubleword reflects the split store bytes",
        read64(paddr + 16U) == 0xff0123456789abcdULL
    );

    TEST_END();
}

bool ai_micro_hs_store_cross_16b_execute_and_address_trigger_repeated_breakpoint_then_completion_adjacent_preserved_corner()
{
    TEST_START();

    uintptr_t paddr = phys_page_base(VSRWX_GRWX);
    uintptr_t vaddr = ai_micro_cross_16b_cacheable_addr(VSRWX_GRWX);
    uintptr_t leading_phys = paddr;
    uintptr_t low_phys = paddr + 8U;
    uintptr_t high_phys = paddr + 16U;
    uintptr_t trailing_phys = paddr + 24U;
    uintptr_t trigger_pc = (uintptr_t)ai_micro_cross_16b_execute_store_once;
    const uint64_t leading_pattern = 0x1234567890abcdefULL;
    const uint64_t low_pattern = AI_MICRO_CROSS_16B_LOW_PATTERN;
    const uint64_t high_pattern = AI_MICRO_CROSS_16B_HIGH_PATTERN;
    const uint64_t trailing_pattern = 0x0fedcba987654321ULL;
    uint64_t saved_mstatus;

    goto_priv(PRIV_M);
    write64(leading_phys, leading_pattern);
    write64(low_phys, low_pattern);
    write64(high_phys, high_pattern);
    write64(trailing_phys, trailing_pattern);

    goto_priv(PRIV_HS);
    hspt_init();
    sfence_vma();

    ai_micro_cross_16b_clear_mcontrol6_slot(0);
    ai_micro_cross_16b_clear_mcontrol6_slot(1);
    ai_micro_cross_16b_arm_mcontrol6_slot(0, trigger_pc, AI_MICRO_CROSS_16B_MCONTROL6_EXECUTE);
    ai_micro_cross_16b_arm_mcontrol6_slot(1, vaddr, AI_MICRO_CROSS_16B_MCONTROL6_STORE);
    saved_mstatus = ai_micro_cross_16b_enable_machine_breakpoint_delivery();

    goto_priv(PRIV_HS);
    TEST_SETUP_EXCEPT();
    ai_micro_cross_16b_execute_store_once(vaddr, AI_MICRO_CROSS_16B_STORE_PATTERN);
    TEST_ASSERT("with both execute-PC and address trigger slots armed on the same cross-16B cacheable store, the first access still reports breakpoint in the adjacent-preservation corner",
        excpt.triggered == true &&
        excpt.cause == CAUSE_BKP
    );

    goto_priv(PRIV_M);
    TEST_ASSERT("the first trapped cross-16B cacheable store still leaves the two touched aligned doublewords and their immediate neighbors unchanged",
        read64(leading_phys) == leading_pattern &&
        read64(low_phys) == low_pattern &&
        read64(high_phys) == high_pattern &&
        read64(trailing_phys) == trailing_pattern
    );
    ai_micro_cross_16b_restore_machine_status(saved_mstatus);

    saved_mstatus = ai_micro_cross_16b_enable_machine_breakpoint_delivery();
    goto_priv(PRIV_HS);
    TEST_SETUP_EXCEPT();
    ai_micro_cross_16b_execute_store_once(vaddr, AI_MICRO_CROSS_16B_STORE_PATTERN);
    TEST_ASSERT("with both execute-PC and address trigger slots still armed on that same cross-16B cacheable store, the second access still reports breakpoint in the adjacent-preservation corner",
        excpt.triggered == true &&
        excpt.cause == CAUSE_BKP
    );

    goto_priv(PRIV_M);
    TEST_ASSERT("the second trapped cross-16B cacheable store still leaves the two touched aligned doublewords and their immediate neighbors unchanged",
        read64(leading_phys) == leading_pattern &&
        read64(low_phys) == low_pattern &&
        read64(high_phys) == high_pattern &&
        read64(trailing_phys) == trailing_pattern
    );
    ai_micro_cross_16b_restore_machine_status(saved_mstatus);

    ai_micro_cross_16b_clear_mcontrol6_slot(0);
    saved_mstatus = ai_micro_cross_16b_enable_machine_breakpoint_delivery();
    goto_priv(PRIV_HS);
    TEST_SETUP_EXCEPT();
    ai_micro_cross_16b_execute_store_once(vaddr, AI_MICRO_CROSS_16B_STORE_PATTERN);
    TEST_ASSERT("after two prior dual-slot breakpoint hits, clearing only the execute-PC slot still leaves the cross-16B store address trigger active in the adjacent-preservation corner",
        excpt.triggered == true &&
        excpt.cause == CAUSE_BKP
    );

    goto_priv(PRIV_M);
    TEST_ASSERT("with only the address trigger slot armed, the trapped cross-16B cacheable store still leaves the two touched aligned doublewords and their immediate neighbors unchanged",
        read64(leading_phys) == leading_pattern &&
        read64(low_phys) == low_pattern &&
        read64(high_phys) == high_pattern &&
        read64(trailing_phys) == trailing_pattern
    );
    ai_micro_cross_16b_restore_machine_status(saved_mstatus);

    ai_micro_cross_16b_clear_mcontrol6_slot(1);
    goto_priv(PRIV_HS);
    TEST_SETUP_EXCEPT();
    ai_micro_cross_16b_execute_store_once(vaddr, AI_MICRO_CROSS_16B_STORE_PATTERN);
    TEST_ASSERT("after clearing both trigger slots, the same cross-16B cacheable store completes normally in the adjacent-preservation corner",
        excpt.triggered == false
    );

    goto_priv(PRIV_M);
    TEST_ASSERT("after the recovered cross-16B cacheable store path, only the two touched aligned doublewords change while their immediate neighbors remain unchanged",
        read64(leading_phys) == leading_pattern &&
        read64(low_phys) == 0xef66554433221100ULL &&
        read64(high_phys) == 0xff0123456789abcdULL &&
        read64(trailing_phys) == trailing_pattern
    );

    TEST_END();
}

bool ai_micro_hs_store_cross_16b_execute_trigger_with_second_half_address_slot_access_fault_spike_corner()
{
    TEST_START();

    uintptr_t paddr = phys_page_base(VSRWX_GRWX);
    uintptr_t vaddr = ai_micro_cross_16b_cacheable_addr(VSRWX_GRWX);
    uintptr_t second_half_vaddr = ai_micro_cross_16b_second_half_addr(VSRWX_GRWX);
    uintptr_t trigger_pc = (uintptr_t)ai_micro_cross_16b_execute_store_once;
    const uint64_t low_pattern = AI_MICRO_CROSS_16B_LOW_PATTERN;
    const uint64_t high_pattern = AI_MICRO_CROSS_16B_HIGH_PATTERN;
    bool trap_triggered;
    uint64_t trap_cause;
    uint64_t trap_tval;

    goto_priv(PRIV_M);
    write64(paddr + 8U, low_pattern);
    write64(paddr + 16U, high_pattern);

    goto_priv(PRIV_HS);
    hspt_init();

    goto_priv(PRIV_M);
    hspt_llptw_ppn_high_bit_overflow_setup();
    sfence_vma();

    ai_micro_cross_16b_clear_mcontrol6_slot(0);
    ai_micro_cross_16b_clear_mcontrol6_slot(1);
    ai_micro_cross_16b_arm_mcontrol6_slot(0, trigger_pc, AI_MICRO_CROSS_16B_MCONTROL6_EXECUTE);
    ai_micro_cross_16b_arm_mcontrol6_slot(1, second_half_vaddr, AI_MICRO_CROSS_16B_MCONTROL6_STORE);
    uint64_t saved_mstatus = ai_micro_cross_16b_enable_machine_breakpoint_delivery();

    goto_priv(PRIV_HS);
    TEST_SETUP_EXCEPT();
    ai_micro_cross_16b_execute_store_once(vaddr, AI_MICRO_CROSS_16B_STORE_PATTERN);
    TEST_ASSERT("with execute-PC trigger armed and the address slot programmed to the true second-half start address, Spike still reports breakpoint first on the cross-16B store access-fault path",
        excpt.triggered == true &&
        excpt.cause == CAUSE_BKP
    );

    goto_priv(PRIV_M);
    TEST_ASSERT("while the execute-PC slot is still armed, the misprogrammed second-half address slot does not matter and the trapped cross-16B store access-fault path leaves both aligned doublewords unchanged",
        read64(paddr + 8U) == low_pattern &&
        read64(paddr + 16U) == high_pattern
    );
    ai_micro_cross_16b_restore_machine_status(saved_mstatus);

    goto_priv(PRIV_HS);
    hspt_init();

    goto_priv(PRIV_M);
    hspt_llptw_ppn_high_bit_overflow_setup();
    sfence_vma();

    ai_micro_cross_16b_clear_mcontrol6_slot(0);
    saved_mstatus = ai_micro_cross_16b_enable_machine_breakpoint_delivery();
    goto_priv(PRIV_HS);
    TEST_SETUP_EXCEPT();
    ai_micro_cross_16b_execute_store_once(vaddr, AI_MICRO_CROSS_16B_STORE_PATTERN);
    trap_triggered = excpt.triggered;
    trap_cause = excpt.cause;
    trap_tval = excpt.tval;
    ai_micro_cross_16b_restore_machine_status(saved_mstatus);

    goto_priv(PRIV_M);
    TEST_ASSERT("after clearing the execute-PC slot, the second-half-start address slot does not keep matching on Spike; the cross-16B store falls straight back to the underlying store access fault with no split-store side effects",
        trap_triggered == true &&
        trap_cause == CAUSE_SAF &&
        trap_tval == vaddr &&
        read64(paddr + 8U) == low_pattern &&
        read64(paddr + 16U) == high_pattern
    );

    ai_micro_cross_16b_clear_mcontrol6_slot(1);

    goto_priv(PRIV_HS);
    hspt_init();
    sfence_vma();

    TEST_END();
}

bool ai_micro_hs_store_cross_16b_execute_trigger_with_second_half_address_slot_access_fault_repeated_breakpoint_then_fault_no_side_effect_spike_corner()
{
    TEST_START();

    uintptr_t paddr = phys_page_base(VSRWX_GRWX);
    uintptr_t vaddr = ai_micro_cross_16b_cacheable_addr(VSRWX_GRWX);
    uintptr_t second_half_vaddr = ai_micro_cross_16b_second_half_addr(VSRWX_GRWX);
    uintptr_t trigger_pc = (uintptr_t)ai_micro_cross_16b_execute_store_once;
    const uint64_t low_pattern = AI_MICRO_CROSS_16B_LOW_PATTERN;
    const uint64_t high_pattern = AI_MICRO_CROSS_16B_HIGH_PATTERN;
    bool trap_triggered;
    uint64_t trap_cause;
    uint64_t trap_tval;

    goto_priv(PRIV_M);
    write64(paddr + 8U, low_pattern);
    write64(paddr + 16U, high_pattern);

    goto_priv(PRIV_HS);
    hspt_init();

    goto_priv(PRIV_M);
    hspt_llptw_ppn_high_bit_overflow_setup();
    sfence_vma();

    ai_micro_cross_16b_clear_mcontrol6_slot(0);
    ai_micro_cross_16b_clear_mcontrol6_slot(1);
    ai_micro_cross_16b_arm_mcontrol6_slot(0, trigger_pc, AI_MICRO_CROSS_16B_MCONTROL6_EXECUTE);
    ai_micro_cross_16b_arm_mcontrol6_slot(1, second_half_vaddr, AI_MICRO_CROSS_16B_MCONTROL6_STORE);
    uint64_t saved_mstatus = ai_micro_cross_16b_enable_machine_breakpoint_delivery();

    goto_priv(PRIV_HS);
    TEST_SETUP_EXCEPT();
    ai_micro_cross_16b_execute_store_once(vaddr, AI_MICRO_CROSS_16B_STORE_PATTERN);
    TEST_ASSERT("with execute-PC trigger armed and the address slot programmed to the true second-half start address, the first cross-16B store access-fault attempt still reports breakpoint on Spike",
        excpt.triggered == true &&
        excpt.cause == CAUSE_BKP
    );

    goto_priv(PRIV_M);
    TEST_ASSERT("the first trapped cross-16B store access-fault attempt still leaves both aligned doublewords unchanged while only the execute-PC slot is effective on Spike",
        read64(paddr + 8U) == low_pattern &&
        read64(paddr + 16U) == high_pattern
    );
    ai_micro_cross_16b_restore_machine_status(saved_mstatus);

    goto_priv(PRIV_HS);
    hspt_init();

    goto_priv(PRIV_M);
    hspt_llptw_ppn_high_bit_overflow_setup();
    sfence_vma();

    saved_mstatus = ai_micro_cross_16b_enable_machine_breakpoint_delivery();
    goto_priv(PRIV_HS);
    TEST_SETUP_EXCEPT();
    ai_micro_cross_16b_execute_store_once(vaddr, AI_MICRO_CROSS_16B_STORE_PATTERN);
    TEST_ASSERT("with the same execute-PC and second-half-start slots still armed, the second cross-16B store access-fault attempt still reports breakpoint on Spike",
        excpt.triggered == true &&
        excpt.cause == CAUSE_BKP
    );

    goto_priv(PRIV_M);
    TEST_ASSERT("the second trapped cross-16B store access-fault attempt still leaves both aligned doublewords unchanged while only the execute-PC slot is effective on Spike",
        read64(paddr + 8U) == low_pattern &&
        read64(paddr + 16U) == high_pattern
    );
    ai_micro_cross_16b_restore_machine_status(saved_mstatus);

    goto_priv(PRIV_HS);
    hspt_init();

    goto_priv(PRIV_M);
    hspt_llptw_ppn_high_bit_overflow_setup();
    sfence_vma();

    ai_micro_cross_16b_clear_mcontrol6_slot(0);
    saved_mstatus = ai_micro_cross_16b_enable_machine_breakpoint_delivery();
    goto_priv(PRIV_HS);
    TEST_SETUP_EXCEPT();
    ai_micro_cross_16b_execute_store_once(vaddr, AI_MICRO_CROSS_16B_STORE_PATTERN);
    trap_triggered = excpt.triggered;
    trap_cause = excpt.cause;
    trap_tval = excpt.tval;
    ai_micro_cross_16b_restore_machine_status(saved_mstatus);

    goto_priv(PRIV_M);
    TEST_ASSERT("after two prior execute-trigger breakpoint hits, clearing only the execute-PC slot still leaves the second-half-start address slot ignored on Spike, so the cross-16B store falls back to store access fault with no side effects",
        trap_triggered == true &&
        trap_cause == CAUSE_SAF &&
        trap_tval == vaddr &&
        read64(paddr + 8U) == low_pattern &&
        read64(paddr + 16U) == high_pattern
    );

    ai_micro_cross_16b_clear_mcontrol6_slot(1);

    goto_priv(PRIV_HS);
    hspt_init();
    sfence_vma();

    TEST_END();
}

bool ai_micro_hs_store_cross_16b_execute_trigger_with_second_half_address_slot_access_fault_repeated_breakpoint_then_fault_no_side_effect_adjacent_preserved_spike_corner()
{
    TEST_START();

    uintptr_t paddr = phys_page_base(VSRWX_GRWX);
    uintptr_t vaddr = ai_micro_cross_16b_cacheable_addr(VSRWX_GRWX);
    uintptr_t second_half_vaddr = ai_micro_cross_16b_second_half_addr(VSRWX_GRWX);
    uintptr_t leading_phys = paddr;
    uintptr_t low_phys = paddr + 8U;
    uintptr_t high_phys = paddr + 16U;
    uintptr_t trailing_phys = paddr + 24U;
    uintptr_t trigger_pc = (uintptr_t)ai_micro_cross_16b_execute_store_once;
    const uint64_t leading_pattern = 0x1234567890abcdefULL;
    const uint64_t low_pattern = AI_MICRO_CROSS_16B_LOW_PATTERN;
    const uint64_t high_pattern = AI_MICRO_CROSS_16B_HIGH_PATTERN;
    const uint64_t trailing_pattern = 0x0fedcba987654321ULL;
    bool trap_triggered;
    uint64_t trap_cause;
    uint64_t trap_tval;
    uint64_t saved_mstatus;

    goto_priv(PRIV_M);
    write64(leading_phys, leading_pattern);
    write64(low_phys, low_pattern);
    write64(high_phys, high_pattern);
    write64(trailing_phys, trailing_pattern);

    goto_priv(PRIV_HS);
    hspt_init();

    goto_priv(PRIV_M);
    hspt_llptw_ppn_high_bit_overflow_setup();
    sfence_vma();

    ai_micro_cross_16b_clear_mcontrol6_slot(0);
    ai_micro_cross_16b_clear_mcontrol6_slot(1);
    ai_micro_cross_16b_arm_mcontrol6_slot(0, trigger_pc, AI_MICRO_CROSS_16B_MCONTROL6_EXECUTE);
    ai_micro_cross_16b_arm_mcontrol6_slot(1, second_half_vaddr, AI_MICRO_CROSS_16B_MCONTROL6_STORE);
    saved_mstatus = ai_micro_cross_16b_enable_machine_breakpoint_delivery();

    goto_priv(PRIV_HS);
    TEST_SETUP_EXCEPT();
    ai_micro_cross_16b_execute_store_once(vaddr, AI_MICRO_CROSS_16B_STORE_PATTERN);
    TEST_ASSERT("with execute-PC trigger armed and the address slot programmed to the true second-half start address, the first cross-16B store access-fault attempt still reports breakpoint on Spike in the adjacent-preservation corner",
        excpt.triggered == true &&
        excpt.cause == CAUSE_BKP
    );

    goto_priv(PRIV_M);
    TEST_ASSERT("the first trapped cross-16B store access-fault attempt still leaves both aligned doublewords and their immediate neighbors unchanged while only the execute-PC slot is effective on Spike",
        read64(leading_phys) == leading_pattern &&
        read64(low_phys) == low_pattern &&
        read64(high_phys) == high_pattern &&
        read64(trailing_phys) == trailing_pattern
    );
    ai_micro_cross_16b_restore_machine_status(saved_mstatus);

    goto_priv(PRIV_HS);
    hspt_init();

    goto_priv(PRIV_M);
    hspt_llptw_ppn_high_bit_overflow_setup();
    sfence_vma();

    saved_mstatus = ai_micro_cross_16b_enable_machine_breakpoint_delivery();
    goto_priv(PRIV_HS);
    TEST_SETUP_EXCEPT();
    ai_micro_cross_16b_execute_store_once(vaddr, AI_MICRO_CROSS_16B_STORE_PATTERN);
    TEST_ASSERT("with the same execute-PC and second-half-start slots still armed, the second cross-16B store access-fault attempt still reports breakpoint on Spike in the adjacent-preservation corner",
        excpt.triggered == true &&
        excpt.cause == CAUSE_BKP
    );

    goto_priv(PRIV_M);
    TEST_ASSERT("the second trapped cross-16B store access-fault attempt still leaves both aligned doublewords and their immediate neighbors unchanged while only the execute-PC slot is effective on Spike",
        read64(leading_phys) == leading_pattern &&
        read64(low_phys) == low_pattern &&
        read64(high_phys) == high_pattern &&
        read64(trailing_phys) == trailing_pattern
    );
    ai_micro_cross_16b_restore_machine_status(saved_mstatus);

    goto_priv(PRIV_HS);
    hspt_init();

    goto_priv(PRIV_M);
    hspt_llptw_ppn_high_bit_overflow_setup();
    sfence_vma();

    ai_micro_cross_16b_clear_mcontrol6_slot(0);
    saved_mstatus = ai_micro_cross_16b_enable_machine_breakpoint_delivery();
    goto_priv(PRIV_HS);
    TEST_SETUP_EXCEPT();
    ai_micro_cross_16b_execute_store_once(vaddr, AI_MICRO_CROSS_16B_STORE_PATTERN);
    trap_triggered = excpt.triggered;
    trap_cause = excpt.cause;
    trap_tval = excpt.tval;
    ai_micro_cross_16b_restore_machine_status(saved_mstatus);

    goto_priv(PRIV_M);
    TEST_ASSERT("after two prior execute-trigger breakpoint hits, clearing only the execute-PC slot still leaves the second-half-start address slot ignored on Spike, so the cross-16B store falls back to store access fault with no side effects in the adjacent-preservation corner",
        trap_triggered == true &&
        trap_cause == CAUSE_SAF &&
        trap_tval == vaddr &&
        read64(leading_phys) == leading_pattern &&
        read64(low_phys) == low_pattern &&
        read64(high_phys) == high_pattern &&
        read64(trailing_phys) == trailing_pattern
    );

    ai_micro_cross_16b_clear_mcontrol6_slot(1);

    goto_priv(PRIV_HS);
    hspt_init();
    sfence_vma();

    TEST_END();
}

bool ai_micro_hs_store_cross_16b_execute_and_address_trigger_page_fault_priority_corner()
{
    TEST_START();

    uintptr_t vaddr = ai_micro_cross_16b_cacheable_addr(VSI_GI);
    uintptr_t trigger_pc = (uintptr_t)ai_micro_cross_16b_execute_store_once;

    goto_priv(PRIV_HS);
    hspt_init();
    sfence_vma();

    ai_micro_cross_16b_clear_mcontrol6_slot(0);
    ai_micro_cross_16b_clear_mcontrol6_slot(1);
    ai_micro_cross_16b_arm_mcontrol6_slot(0, trigger_pc, AI_MICRO_CROSS_16B_MCONTROL6_EXECUTE);
    ai_micro_cross_16b_arm_mcontrol6_slot(1, vaddr, AI_MICRO_CROSS_16B_MCONTROL6_STORE);
    uint64_t saved_mstatus = ai_micro_cross_16b_enable_machine_breakpoint_delivery();

    goto_priv(PRIV_HS);
    TEST_SETUP_EXCEPT();
    ai_micro_cross_16b_execute_store_once(vaddr, AI_MICRO_CROSS_16B_STORE_PATTERN);
    TEST_ASSERT("an HS cross-16B store still reports breakpoint when both execute-PC and address trigger slots match a split store that would otherwise raise page fault",
        excpt.triggered == true &&
        excpt.cause == CAUSE_BKP
    );
    ai_micro_cross_16b_restore_machine_status(saved_mstatus);

    goto_priv(PRIV_HS);
    hspt_init();
    sfence_vma();

    ai_micro_cross_16b_clear_mcontrol6_slot(0);
    saved_mstatus = ai_micro_cross_16b_enable_machine_breakpoint_delivery();
    goto_priv(PRIV_HS);
    TEST_SETUP_EXCEPT();
    ai_micro_cross_16b_execute_store_once(vaddr, AI_MICRO_CROSS_16B_STORE_PATTERN);
    TEST_ASSERT("after clearing only the execute-PC slot, the remaining cross-16B store address trigger still reports breakpoint on the same page-fault path",
        excpt.triggered == true &&
        excpt.cause == CAUSE_BKP
    );
    ai_micro_cross_16b_restore_machine_status(saved_mstatus);

    goto_priv(PRIV_HS);
    hspt_init();
    sfence_vma();

    ai_micro_cross_16b_clear_mcontrol6_slot(1);
    goto_priv(PRIV_HS);
    TEST_SETUP_EXCEPT();
    ai_micro_cross_16b_execute_store_once(vaddr, AI_MICRO_CROSS_16B_STORE_PATTERN);
    TEST_ASSERT("after clearing both trigger slots, the same HS cross-16B store reports store page fault with the original misaligned VA",
        excpt.triggered == true &&
        excpt.cause == CAUSE_SPF &&
        excpt.tval == vaddr
    );

    TEST_END();
}

bool ai_micro_hs_store_cross_16b_execute_and_address_trigger_page_fault_repeated_breakpoint_then_fault_corner()
{
    TEST_START();

    uintptr_t paddr = phys_page_base(VSI_GI);
    uintptr_t vaddr = ai_micro_cross_16b_cacheable_addr(VSI_GI);
    uintptr_t trigger_pc = (uintptr_t)ai_micro_cross_16b_execute_store_once;
    const uint64_t low_pattern = AI_MICRO_CROSS_16B_LOW_PATTERN;
    const uint64_t high_pattern = AI_MICRO_CROSS_16B_HIGH_PATTERN;

    goto_priv(PRIV_M);
    write64(paddr + 8U, low_pattern);
    write64(paddr + 16U, high_pattern);

    goto_priv(PRIV_HS);
    hspt_init();
    sfence_vma();

    ai_micro_cross_16b_clear_mcontrol6_slot(0);
    ai_micro_cross_16b_clear_mcontrol6_slot(1);
    ai_micro_cross_16b_arm_mcontrol6_slot(0, trigger_pc, AI_MICRO_CROSS_16B_MCONTROL6_EXECUTE);
    ai_micro_cross_16b_arm_mcontrol6_slot(1, vaddr, AI_MICRO_CROSS_16B_MCONTROL6_STORE);
    uint64_t saved_mstatus = ai_micro_cross_16b_enable_machine_breakpoint_delivery();

    goto_priv(PRIV_HS);
    TEST_SETUP_EXCEPT();
    ai_micro_cross_16b_execute_store_once(vaddr, AI_MICRO_CROSS_16B_STORE_PATTERN);
    TEST_ASSERT("with both execute-PC and address trigger slots armed on the cross-16B store page-fault path, the first access reports breakpoint",
        excpt.triggered == true &&
        excpt.cause == CAUSE_BKP
    );

    goto_priv(PRIV_M);
    TEST_ASSERT("the first trapped cross-16B store page-fault probe leaves both aligned doublewords unchanged",
        read64(paddr + 8U) == low_pattern &&
        read64(paddr + 16U) == high_pattern
    );
    ai_micro_cross_16b_restore_machine_status(saved_mstatus);

    goto_priv(PRIV_HS);
    hspt_init();
    sfence_vma();

    saved_mstatus = ai_micro_cross_16b_enable_machine_breakpoint_delivery();
    goto_priv(PRIV_HS);
    TEST_SETUP_EXCEPT();
    ai_micro_cross_16b_execute_store_once(vaddr, AI_MICRO_CROSS_16B_STORE_PATTERN);
    TEST_ASSERT("with both execute-PC and address trigger slots still armed on the same cross-16B store page-fault path, the second access still reports breakpoint",
        excpt.triggered == true &&
        excpt.cause == CAUSE_BKP
    );

    goto_priv(PRIV_M);
    TEST_ASSERT("the second trapped cross-16B store page-fault probe still leaves both aligned doublewords unchanged",
        read64(paddr + 8U) == low_pattern &&
        read64(paddr + 16U) == high_pattern
    );
    ai_micro_cross_16b_restore_machine_status(saved_mstatus);

    goto_priv(PRIV_HS);
    hspt_init();
    sfence_vma();

    ai_micro_cross_16b_clear_mcontrol6_slot(0);
    saved_mstatus = ai_micro_cross_16b_enable_machine_breakpoint_delivery();
    goto_priv(PRIV_HS);
    TEST_SETUP_EXCEPT();
    ai_micro_cross_16b_execute_store_once(vaddr, AI_MICRO_CROSS_16B_STORE_PATTERN);
    TEST_ASSERT("after two prior dual-slot breakpoint hits, clearing only the execute-PC slot still leaves the cross-16B store address trigger active on the same page-fault path",
        excpt.triggered == true &&
        excpt.cause == CAUSE_BKP
    );

    goto_priv(PRIV_M);
    TEST_ASSERT("with only the address trigger slot armed, the trapped cross-16B store page-fault probe still leaves both aligned doublewords unchanged",
        read64(paddr + 8U) == low_pattern &&
        read64(paddr + 16U) == high_pattern
    );
    ai_micro_cross_16b_restore_machine_status(saved_mstatus);

    goto_priv(PRIV_HS);
    hspt_init();
    sfence_vma();

    ai_micro_cross_16b_clear_mcontrol6_slot(1);
    goto_priv(PRIV_HS);
    TEST_SETUP_EXCEPT();
    ai_micro_cross_16b_execute_store_once(vaddr, AI_MICRO_CROSS_16B_STORE_PATTERN);
    TEST_ASSERT("after clearing both trigger slots, the same cross-16B store falls back to store page fault with the original misaligned VA and no side effects",
        excpt.triggered == true &&
        excpt.cause == CAUSE_SPF &&
        excpt.tval == vaddr
    );

    goto_priv(PRIV_M);
    TEST_ASSERT("after clearing both trigger slots on the cross-16B store page-fault path, both aligned doublewords remain unchanged",
        read64(paddr + 8U) == low_pattern &&
        read64(paddr + 16U) == high_pattern
    );

    TEST_END();
}

bool ai_micro_hs_store_cross_16b_execute_and_address_trigger_page_fault_repeated_breakpoint_then_fault_adjacent_preserved_corner()
{
    TEST_START();

    uintptr_t paddr = phys_page_base(VSI_GI);
    uintptr_t vaddr = ai_micro_cross_16b_cacheable_addr(VSI_GI);
    uintptr_t leading_phys = paddr;
    uintptr_t low_phys = paddr + 8U;
    uintptr_t high_phys = paddr + 16U;
    uintptr_t trailing_phys = paddr + 24U;
    uintptr_t trigger_pc = (uintptr_t)ai_micro_cross_16b_execute_store_once;
    const uint64_t leading_pattern = 0x1234567890abcdefULL;
    const uint64_t low_pattern = AI_MICRO_CROSS_16B_LOW_PATTERN;
    const uint64_t high_pattern = AI_MICRO_CROSS_16B_HIGH_PATTERN;
    const uint64_t trailing_pattern = 0x0fedcba987654321ULL;
    bool trap_triggered;
    uint64_t trap_cause;
    uint64_t trap_tval;
    uint64_t saved_mstatus;

    goto_priv(PRIV_M);
    write64(leading_phys, leading_pattern);
    write64(low_phys, low_pattern);
    write64(high_phys, high_pattern);
    write64(trailing_phys, trailing_pattern);

    goto_priv(PRIV_HS);
    hspt_init();
    sfence_vma();

    ai_micro_cross_16b_clear_mcontrol6_slot(0);
    ai_micro_cross_16b_clear_mcontrol6_slot(1);
    ai_micro_cross_16b_arm_mcontrol6_slot(0, trigger_pc, AI_MICRO_CROSS_16B_MCONTROL6_EXECUTE);
    ai_micro_cross_16b_arm_mcontrol6_slot(1, vaddr, AI_MICRO_CROSS_16B_MCONTROL6_STORE);
    saved_mstatus = ai_micro_cross_16b_enable_machine_breakpoint_delivery();

    goto_priv(PRIV_HS);
    TEST_SETUP_EXCEPT();
    ai_micro_cross_16b_execute_store_once(vaddr, AI_MICRO_CROSS_16B_STORE_PATTERN);
    TEST_ASSERT("with both execute-PC and address trigger slots armed on the cross-16B store page-fault path, the first access still reports breakpoint in the adjacent-preservation corner",
        excpt.triggered == true &&
        excpt.cause == CAUSE_BKP
    );

    goto_priv(PRIV_M);
    TEST_ASSERT("the first trapped cross-16B store page-fault probe still leaves the two touched aligned doublewords and their immediate neighbors unchanged",
        read64(leading_phys) == leading_pattern &&
        read64(low_phys) == low_pattern &&
        read64(high_phys) == high_pattern &&
        read64(trailing_phys) == trailing_pattern
    );
    ai_micro_cross_16b_restore_machine_status(saved_mstatus);

    goto_priv(PRIV_HS);
    hspt_init();
    sfence_vma();

    saved_mstatus = ai_micro_cross_16b_enable_machine_breakpoint_delivery();
    goto_priv(PRIV_HS);
    TEST_SETUP_EXCEPT();
    ai_micro_cross_16b_execute_store_once(vaddr, AI_MICRO_CROSS_16B_STORE_PATTERN);
    TEST_ASSERT("with both execute-PC and address trigger slots still armed on the same cross-16B store page-fault path, the second access still reports breakpoint in the adjacent-preservation corner",
        excpt.triggered == true &&
        excpt.cause == CAUSE_BKP
    );

    goto_priv(PRIV_M);
    TEST_ASSERT("the second trapped cross-16B store page-fault probe still leaves the two touched aligned doublewords and their immediate neighbors unchanged",
        read64(leading_phys) == leading_pattern &&
        read64(low_phys) == low_pattern &&
        read64(high_phys) == high_pattern &&
        read64(trailing_phys) == trailing_pattern
    );
    ai_micro_cross_16b_restore_machine_status(saved_mstatus);

    goto_priv(PRIV_HS);
    hspt_init();
    sfence_vma();

    ai_micro_cross_16b_clear_mcontrol6_slot(0);
    saved_mstatus = ai_micro_cross_16b_enable_machine_breakpoint_delivery();
    goto_priv(PRIV_HS);
    TEST_SETUP_EXCEPT();
    ai_micro_cross_16b_execute_store_once(vaddr, AI_MICRO_CROSS_16B_STORE_PATTERN);
    TEST_ASSERT("after two prior dual-slot breakpoint hits, clearing only the execute-PC slot still leaves the cross-16B store address trigger active on the same page-fault path in the adjacent-preservation corner",
        excpt.triggered == true &&
        excpt.cause == CAUSE_BKP
    );

    goto_priv(PRIV_M);
    TEST_ASSERT("with only the address trigger slot armed, the trapped cross-16B store page-fault probe still leaves the two touched aligned doublewords and their immediate neighbors unchanged",
        read64(leading_phys) == leading_pattern &&
        read64(low_phys) == low_pattern &&
        read64(high_phys) == high_pattern &&
        read64(trailing_phys) == trailing_pattern
    );
    ai_micro_cross_16b_restore_machine_status(saved_mstatus);

    goto_priv(PRIV_HS);
    hspt_init();
    sfence_vma();

    ai_micro_cross_16b_clear_mcontrol6_slot(1);
    goto_priv(PRIV_HS);
    TEST_SETUP_EXCEPT();
    ai_micro_cross_16b_execute_store_once(vaddr, AI_MICRO_CROSS_16B_STORE_PATTERN);
    trap_triggered = excpt.triggered;
    trap_cause = excpt.cause;
    trap_tval = excpt.tval;

    goto_priv(PRIV_M);
    TEST_ASSERT("after clearing both trigger slots, the same cross-16B store falls back to store page fault with the original misaligned VA and no side effects in the adjacent-preservation corner",
        trap_triggered == true &&
        trap_cause == CAUSE_SPF &&
        trap_tval == vaddr &&
        read64(leading_phys) == leading_pattern &&
        read64(low_phys) == low_pattern &&
        read64(high_phys) == high_pattern &&
        read64(trailing_phys) == trailing_pattern
    );

    TEST_END();
}

bool ai_micro_hs_store_cross_16b_execute_and_address_trigger_page_fault_repeated_breakpoint_then_valid_cross_16b_store_recovers_corner()
{
    TEST_START();

    uintptr_t fault_vaddr = ai_micro_cross_16b_cacheable_addr(VSI_GI);
    uintptr_t ok_vaddr = ai_micro_cross_16b_cacheable_addr(VSRWX_GRWX);
    uintptr_t ok_phys = phys_page_base(VSRWX_GRWX);
    uintptr_t trigger_pc = (uintptr_t)ai_micro_cross_16b_execute_store_once;
    const uint64_t leading_pattern = 0x1122334455667788ULL;
    const uint64_t low_pattern = AI_MICRO_CROSS_16B_LOW_PATTERN;
    const uint64_t high_pattern = AI_MICRO_CROSS_16B_HIGH_PATTERN;
    const uint64_t trailing_pattern = 0x8877665544332211ULL;
    uint64_t saved_mstatus;

    goto_priv(PRIV_M);
    write64(ok_phys, leading_pattern);
    write64(ok_phys + 8U, low_pattern);
    write64(ok_phys + 16U, high_pattern);
    write64(ok_phys + 24U, trailing_pattern);

    goto_priv(PRIV_HS);
    hspt_init();
    sfence_vma();

    ai_micro_cross_16b_clear_mcontrol6_slot(0);
    ai_micro_cross_16b_clear_mcontrol6_slot(1);
    ai_micro_cross_16b_arm_mcontrol6_slot(0, trigger_pc, AI_MICRO_CROSS_16B_MCONTROL6_EXECUTE);
    ai_micro_cross_16b_arm_mcontrol6_slot(1, fault_vaddr, AI_MICRO_CROSS_16B_MCONTROL6_STORE);
    saved_mstatus = ai_micro_cross_16b_enable_machine_breakpoint_delivery();

    goto_priv(PRIV_HS);
    TEST_SETUP_EXCEPT();
    ai_micro_cross_16b_execute_store_once(fault_vaddr, AI_MICRO_CROSS_16B_STORE_PATTERN);
    TEST_ASSERT("with both execute-PC and address trigger slots armed on the cross-16B store page-fault path, the first access still reports breakpoint before recovery",
        excpt.triggered == true &&
        excpt.cause == CAUSE_BKP
    );
    ai_micro_cross_16b_restore_machine_status(saved_mstatus);

    goto_priv(PRIV_HS);
    hspt_init();
    sfence_vma();

    saved_mstatus = ai_micro_cross_16b_enable_machine_breakpoint_delivery();
    goto_priv(PRIV_HS);
    TEST_SETUP_EXCEPT();
    ai_micro_cross_16b_execute_store_once(fault_vaddr, AI_MICRO_CROSS_16B_STORE_PATTERN);
    TEST_ASSERT("with both trigger slots still armed on the same cross-16B store page-fault path, the second access still reports breakpoint before recovery",
        excpt.triggered == true &&
        excpt.cause == CAUSE_BKP
    );
    ai_micro_cross_16b_restore_machine_status(saved_mstatus);

    goto_priv(PRIV_HS);
    hspt_init();
    sfence_vma();

    ai_micro_cross_16b_clear_mcontrol6_slot(0);
    saved_mstatus = ai_micro_cross_16b_enable_machine_breakpoint_delivery();
    goto_priv(PRIV_HS);
    TEST_SETUP_EXCEPT();
    ai_micro_cross_16b_execute_store_once(fault_vaddr, AI_MICRO_CROSS_16B_STORE_PATTERN);
    TEST_ASSERT("after two prior dual-slot breakpoint hits, clearing only the execute-PC slot still leaves the cross-16B store address trigger active on the same page-fault path before recovery",
        excpt.triggered == true &&
        excpt.cause == CAUSE_BKP
    );
    ai_micro_cross_16b_restore_machine_status(saved_mstatus);

    goto_priv(PRIV_HS);
    hspt_init();
    sfence_vma();

    ai_micro_cross_16b_clear_mcontrol6_slot(1);
    goto_priv(PRIV_HS);
    TEST_SETUP_EXCEPT();
    ai_micro_cross_16b_execute_store_once(fault_vaddr, AI_MICRO_CROSS_16B_STORE_PATTERN);
    TEST_ASSERT("after clearing both trigger slots, the same cross-16B store first falls back to store page fault with the original misaligned VA before recovery",
        excpt.triggered == true &&
        excpt.cause == CAUSE_SPF &&
        excpt.tval == fault_vaddr
    );

    TEST_SETUP_EXCEPT();
    sd(ok_vaddr, AI_MICRO_CROSS_16B_STORE_PATTERN);
    TEST_ASSERT("after repeated trigger hits and the fallback page fault, a following valid cacheable cross-16B store still commits normally",
        excpt.triggered == false
    );

    goto_priv(PRIV_M);
    TEST_ASSERT("the recovered cross-16B store updates only the two participating doublewords while the adjacent doublewords stay unchanged after the trigger-plus-page-fault sequence",
        read64(ok_phys) == leading_pattern &&
        read64(ok_phys + 8U) == 0xef66554433221100ULL &&
        read64(ok_phys + 16U) == 0xff0123456789abcdULL &&
        read64(ok_phys + 24U) == trailing_pattern
    );

    TEST_END();
}

bool ai_micro_hs_store_cross_16b_execute_and_address_trigger_page_fault_repeated_breakpoint_then_valid_cross_16b_store_adjacent_preserved_corner()
{
    TEST_START();

    uintptr_t fault_vaddr = ai_micro_cross_16b_cacheable_addr(VSI_GI);
    uintptr_t ok_vaddr = ai_micro_cross_16b_cacheable_addr(VSRWX_GRWX);
    uintptr_t ok_phys = phys_page_base(VSRWX_GRWX);
    uintptr_t leading_phys = ok_phys;
    uintptr_t low_phys = ok_phys + 8U;
    uintptr_t high_phys = ok_phys + 16U;
    uintptr_t trailing_phys = ok_phys + 24U;
    uintptr_t trigger_pc = (uintptr_t)ai_micro_cross_16b_execute_store_once;
    const uint64_t leading_pattern = 0x1122334455667788ULL;
    const uint64_t low_pattern = AI_MICRO_CROSS_16B_LOW_PATTERN;
    const uint64_t high_pattern = AI_MICRO_CROSS_16B_HIGH_PATTERN;
    const uint64_t trailing_pattern = 0x8877665544332211ULL;
    uint64_t saved_mstatus;

    goto_priv(PRIV_M);
    write64(leading_phys, leading_pattern);
    write64(low_phys, low_pattern);
    write64(high_phys, high_pattern);
    write64(trailing_phys, trailing_pattern);

    goto_priv(PRIV_HS);
    hspt_init();
    sfence_vma();

    ai_micro_cross_16b_clear_mcontrol6_slot(0);
    ai_micro_cross_16b_clear_mcontrol6_slot(1);
    ai_micro_cross_16b_arm_mcontrol6_slot(0, trigger_pc, AI_MICRO_CROSS_16B_MCONTROL6_EXECUTE);
    ai_micro_cross_16b_arm_mcontrol6_slot(1, fault_vaddr, AI_MICRO_CROSS_16B_MCONTROL6_STORE);
    saved_mstatus = ai_micro_cross_16b_enable_machine_breakpoint_delivery();

    goto_priv(PRIV_HS);
    TEST_SETUP_EXCEPT();
    ai_micro_cross_16b_execute_store_once(fault_vaddr, AI_MICRO_CROSS_16B_STORE_PATTERN);
    TEST_ASSERT("with both execute-PC and address trigger slots armed on the cross-16B store page-fault path, the first access still reports breakpoint before recovery in the adjacent-preservation corner",
        excpt.triggered == true &&
        excpt.cause == CAUSE_BKP
    );

    goto_priv(PRIV_M);
    TEST_ASSERT("the first trapped trigger-plus-page-fault store probe still leaves the future recovery target doublewords and their adjacent doublewords unchanged",
        read64(leading_phys) == leading_pattern &&
        read64(low_phys) == low_pattern &&
        read64(high_phys) == high_pattern &&
        read64(trailing_phys) == trailing_pattern
    );
    ai_micro_cross_16b_restore_machine_status(saved_mstatus);

    goto_priv(PRIV_HS);
    hspt_init();
    sfence_vma();

    saved_mstatus = ai_micro_cross_16b_enable_machine_breakpoint_delivery();
    goto_priv(PRIV_HS);
    TEST_SETUP_EXCEPT();
    ai_micro_cross_16b_execute_store_once(fault_vaddr, AI_MICRO_CROSS_16B_STORE_PATTERN);
    TEST_ASSERT("with both trigger slots still armed on the same cross-16B store page-fault path, the second access still reports breakpoint before recovery in the adjacent-preservation corner",
        excpt.triggered == true &&
        excpt.cause == CAUSE_BKP
    );

    goto_priv(PRIV_M);
    TEST_ASSERT("the second trapped trigger-plus-page-fault store probe still leaves the future recovery target doublewords and their adjacent doublewords unchanged",
        read64(leading_phys) == leading_pattern &&
        read64(low_phys) == low_pattern &&
        read64(high_phys) == high_pattern &&
        read64(trailing_phys) == trailing_pattern
    );
    ai_micro_cross_16b_restore_machine_status(saved_mstatus);

    goto_priv(PRIV_HS);
    hspt_init();
    sfence_vma();

    ai_micro_cross_16b_clear_mcontrol6_slot(0);
    saved_mstatus = ai_micro_cross_16b_enable_machine_breakpoint_delivery();
    goto_priv(PRIV_HS);
    TEST_SETUP_EXCEPT();
    ai_micro_cross_16b_execute_store_once(fault_vaddr, AI_MICRO_CROSS_16B_STORE_PATTERN);
    TEST_ASSERT("after two prior dual-slot breakpoint hits, clearing only the execute-PC slot still leaves the cross-16B store address trigger active on the same page-fault path before recovery in the adjacent-preservation corner",
        excpt.triggered == true &&
        excpt.cause == CAUSE_BKP
    );

    goto_priv(PRIV_M);
    TEST_ASSERT("with only the address trigger slot armed, the trapped trigger-plus-page-fault store probe still leaves the future recovery target doublewords and their adjacent doublewords unchanged",
        read64(leading_phys) == leading_pattern &&
        read64(low_phys) == low_pattern &&
        read64(high_phys) == high_pattern &&
        read64(trailing_phys) == trailing_pattern
    );
    ai_micro_cross_16b_restore_machine_status(saved_mstatus);

    goto_priv(PRIV_HS);
    hspt_init();
    sfence_vma();

    ai_micro_cross_16b_clear_mcontrol6_slot(1);
    goto_priv(PRIV_HS);
    TEST_SETUP_EXCEPT();
    ai_micro_cross_16b_execute_store_once(fault_vaddr, AI_MICRO_CROSS_16B_STORE_PATTERN);
    TEST_ASSERT("after clearing both trigger slots, the same cross-16B store first falls back to store page fault with the original misaligned VA before recovery in the adjacent-preservation corner",
        excpt.triggered == true &&
        excpt.cause == CAUSE_SPF &&
        excpt.tval == fault_vaddr
    );

    goto_priv(PRIV_M);
    TEST_ASSERT("the fallback cross-16B store page-fault path still leaves the future recovery target doublewords and their adjacent doublewords unchanged",
        read64(leading_phys) == leading_pattern &&
        read64(low_phys) == low_pattern &&
        read64(high_phys) == high_pattern &&
        read64(trailing_phys) == trailing_pattern
    );

    goto_priv(PRIV_HS);
    TEST_SETUP_EXCEPT();
    sd(ok_vaddr, AI_MICRO_CROSS_16B_STORE_PATTERN);
    TEST_ASSERT("after repeated trigger hits and the fallback page fault, a following valid cacheable cross-16B store still commits normally in the adjacent-preservation corner",
        excpt.triggered == false
    );

    goto_priv(PRIV_M);
    TEST_ASSERT("the recovered cross-16B store updates only the two participating doublewords while the adjacent doublewords stay unchanged in the adjacent-preservation corner",
        read64(leading_phys) == leading_pattern &&
        read64(low_phys) == 0xef66554433221100ULL &&
        read64(high_phys) == 0xff0123456789abcdULL &&
        read64(trailing_phys) == trailing_pattern
    );

    TEST_END();
}

bool ai_micro_hs_store_cross_16b_execute_and_address_trigger_access_fault_priority_corner()
{
    TEST_START();

    uintptr_t vaddr = ai_micro_cross_16b_cacheable_addr(VSRWX_GRWX);
    uintptr_t trigger_pc = (uintptr_t)ai_micro_cross_16b_execute_store_once;

    goto_priv(PRIV_HS);
    hspt_init();

    goto_priv(PRIV_M);
    hspt_llptw_ppn_high_bit_overflow_setup();
    sfence_vma();

    ai_micro_cross_16b_clear_mcontrol6_slot(0);
    ai_micro_cross_16b_clear_mcontrol6_slot(1);
    ai_micro_cross_16b_arm_mcontrol6_slot(0, trigger_pc, AI_MICRO_CROSS_16B_MCONTROL6_EXECUTE);
    ai_micro_cross_16b_arm_mcontrol6_slot(1, vaddr, AI_MICRO_CROSS_16B_MCONTROL6_STORE);
    uint64_t saved_mstatus = ai_micro_cross_16b_enable_machine_breakpoint_delivery();

    goto_priv(PRIV_HS);
    TEST_SETUP_EXCEPT();
    ai_micro_cross_16b_execute_store_once(vaddr, AI_MICRO_CROSS_16B_STORE_PATTERN);
    TEST_ASSERT("an HS cross-16B store still reports breakpoint when both execute-PC and address trigger slots match a split store that would otherwise raise access fault",
        excpt.triggered == true &&
        excpt.cause == CAUSE_BKP
    );
    ai_micro_cross_16b_restore_machine_status(saved_mstatus);

    goto_priv(PRIV_HS);
    hspt_init();

    goto_priv(PRIV_M);
    hspt_llptw_ppn_high_bit_overflow_setup();
    sfence_vma();

    ai_micro_cross_16b_clear_mcontrol6_slot(0);
    saved_mstatus = ai_micro_cross_16b_enable_machine_breakpoint_delivery();
    goto_priv(PRIV_HS);
    TEST_SETUP_EXCEPT();
    ai_micro_cross_16b_execute_store_once(vaddr, AI_MICRO_CROSS_16B_STORE_PATTERN);
    TEST_ASSERT("after clearing only the execute-PC slot, the remaining cross-16B store address trigger still reports breakpoint on the same access-fault path",
        excpt.triggered == true &&
        excpt.cause == CAUSE_BKP
    );
    ai_micro_cross_16b_restore_machine_status(saved_mstatus);

    goto_priv(PRIV_HS);
    hspt_init();

    goto_priv(PRIV_M);
    hspt_llptw_ppn_high_bit_overflow_setup();
    sfence_vma();

    ai_micro_cross_16b_clear_mcontrol6_slot(1);
    goto_priv(PRIV_HS);
    TEST_SETUP_EXCEPT();
    ai_micro_cross_16b_execute_store_once(vaddr, AI_MICRO_CROSS_16B_STORE_PATTERN);
    TEST_ASSERT("after clearing both trigger slots, the same HS cross-16B store reports store access fault with the original misaligned VA",
        excpt.triggered == true &&
        excpt.cause == CAUSE_SAF &&
        excpt.tval == vaddr
    );

    goto_priv(PRIV_HS);
    hspt_init();
    sfence_vma();

    TEST_END();
}

bool ai_micro_hs_store_cross_16b_execute_and_address_trigger_access_fault_repeated_breakpoint_then_fault_corner()
{
    TEST_START();

    uintptr_t paddr = phys_page_base(VSRWX_GRWX);
    uintptr_t vaddr = ai_micro_cross_16b_cacheable_addr(VSRWX_GRWX);
    uintptr_t trigger_pc = (uintptr_t)ai_micro_cross_16b_execute_store_once;
    const uint64_t low_pattern = AI_MICRO_CROSS_16B_LOW_PATTERN;
    const uint64_t high_pattern = AI_MICRO_CROSS_16B_HIGH_PATTERN;

    goto_priv(PRIV_M);
    write64(paddr + 8U, low_pattern);
    write64(paddr + 16U, high_pattern);

    goto_priv(PRIV_HS);
    hspt_init();

    goto_priv(PRIV_M);
    hspt_llptw_ppn_high_bit_overflow_setup();
    sfence_vma();

    ai_micro_cross_16b_clear_mcontrol6_slot(0);
    ai_micro_cross_16b_clear_mcontrol6_slot(1);
    ai_micro_cross_16b_arm_mcontrol6_slot(0, trigger_pc, AI_MICRO_CROSS_16B_MCONTROL6_EXECUTE);
    ai_micro_cross_16b_arm_mcontrol6_slot(1, vaddr, AI_MICRO_CROSS_16B_MCONTROL6_STORE);
    uint64_t saved_mstatus = ai_micro_cross_16b_enable_machine_breakpoint_delivery();

    goto_priv(PRIV_HS);
    TEST_SETUP_EXCEPT();
    ai_micro_cross_16b_execute_store_once(vaddr, AI_MICRO_CROSS_16B_STORE_PATTERN);
    TEST_ASSERT("with both execute-PC and address trigger slots armed on the cross-16B store access-fault path, the first access reports breakpoint",
        excpt.triggered == true &&
        excpt.cause == CAUSE_BKP
    );

    goto_priv(PRIV_M);
    TEST_ASSERT("the first trapped cross-16B store access-fault probe leaves both aligned doublewords unchanged",
        read64(paddr + 8U) == low_pattern &&
        read64(paddr + 16U) == high_pattern
    );
    ai_micro_cross_16b_restore_machine_status(saved_mstatus);

    goto_priv(PRIV_HS);
    hspt_init();

    goto_priv(PRIV_M);
    hspt_llptw_ppn_high_bit_overflow_setup();
    sfence_vma();

    saved_mstatus = ai_micro_cross_16b_enable_machine_breakpoint_delivery();
    goto_priv(PRIV_HS);
    TEST_SETUP_EXCEPT();
    ai_micro_cross_16b_execute_store_once(vaddr, AI_MICRO_CROSS_16B_STORE_PATTERN);
    TEST_ASSERT("with both execute-PC and address trigger slots still armed on the same cross-16B store access-fault path, the second access still reports breakpoint",
        excpt.triggered == true &&
        excpt.cause == CAUSE_BKP
    );

    goto_priv(PRIV_M);
    TEST_ASSERT("the second trapped cross-16B store access-fault probe still leaves both aligned doublewords unchanged",
        read64(paddr + 8U) == low_pattern &&
        read64(paddr + 16U) == high_pattern
    );
    ai_micro_cross_16b_restore_machine_status(saved_mstatus);

    goto_priv(PRIV_HS);
    hspt_init();

    goto_priv(PRIV_M);
    hspt_llptw_ppn_high_bit_overflow_setup();
    sfence_vma();

    ai_micro_cross_16b_clear_mcontrol6_slot(0);
    saved_mstatus = ai_micro_cross_16b_enable_machine_breakpoint_delivery();
    goto_priv(PRIV_HS);
    TEST_SETUP_EXCEPT();
    ai_micro_cross_16b_execute_store_once(vaddr, AI_MICRO_CROSS_16B_STORE_PATTERN);
    TEST_ASSERT("after two prior dual-slot breakpoint hits, clearing only the execute-PC slot still leaves the cross-16B store address trigger active on the same access-fault path",
        excpt.triggered == true &&
        excpt.cause == CAUSE_BKP
    );

    goto_priv(PRIV_M);
    TEST_ASSERT("with only the address trigger slot armed, the trapped cross-16B store access-fault probe still leaves both aligned doublewords unchanged",
        read64(paddr + 8U) == low_pattern &&
        read64(paddr + 16U) == high_pattern
    );
    ai_micro_cross_16b_restore_machine_status(saved_mstatus);

    goto_priv(PRIV_HS);
    hspt_init();

    goto_priv(PRIV_M);
    hspt_llptw_ppn_high_bit_overflow_setup();
    sfence_vma();

    ai_micro_cross_16b_clear_mcontrol6_slot(1);
    goto_priv(PRIV_HS);
    TEST_SETUP_EXCEPT();
    ai_micro_cross_16b_execute_store_once(vaddr, AI_MICRO_CROSS_16B_STORE_PATTERN);
    TEST_ASSERT("after clearing both trigger slots, the same cross-16B store falls back to store access fault with the original misaligned VA and no side effects",
        excpt.triggered == true &&
        excpt.cause == CAUSE_SAF &&
        excpt.tval == vaddr
    );

    goto_priv(PRIV_M);
    TEST_ASSERT("after clearing both trigger slots on the cross-16B store access-fault path, both aligned doublewords remain unchanged",
        read64(paddr + 8U) == low_pattern &&
        read64(paddr + 16U) == high_pattern
    );

    goto_priv(PRIV_HS);
    hspt_init();
    sfence_vma();

    TEST_END();
}

bool ai_micro_hs_store_cross_16b_execute_and_address_trigger_access_fault_repeated_breakpoint_then_fault_adjacent_preserved_corner()
{
    TEST_START();

    uintptr_t paddr = phys_page_base(VSRWX_GRWX);
    uintptr_t vaddr = ai_micro_cross_16b_cacheable_addr(VSRWX_GRWX);
    uintptr_t leading_phys = paddr;
    uintptr_t low_phys = paddr + 8U;
    uintptr_t high_phys = paddr + 16U;
    uintptr_t trailing_phys = paddr + 24U;
    uintptr_t trigger_pc = (uintptr_t)ai_micro_cross_16b_execute_store_once;
    const uint64_t leading_pattern = 0x1122aa55cc337788ULL;
    const uint64_t low_pattern = AI_MICRO_CROSS_16B_LOW_PATTERN;
    const uint64_t high_pattern = AI_MICRO_CROSS_16B_HIGH_PATTERN;
    const uint64_t trailing_pattern = 0x887733cc55aa2211ULL;
    bool trap_triggered;
    uint64_t trap_cause;
    uint64_t trap_tval;
    uint64_t saved_mstatus;

    goto_priv(PRIV_M);
    write64(leading_phys, leading_pattern);
    write64(low_phys, low_pattern);
    write64(high_phys, high_pattern);
    write64(trailing_phys, trailing_pattern);

    goto_priv(PRIV_HS);
    hspt_init();

    goto_priv(PRIV_M);
    hspt_llptw_ppn_high_bit_overflow_setup();
    sfence_vma();

    ai_micro_cross_16b_clear_mcontrol6_slot(0);
    ai_micro_cross_16b_clear_mcontrol6_slot(1);
    ai_micro_cross_16b_arm_mcontrol6_slot(0, trigger_pc, AI_MICRO_CROSS_16B_MCONTROL6_EXECUTE);
    ai_micro_cross_16b_arm_mcontrol6_slot(1, vaddr, AI_MICRO_CROSS_16B_MCONTROL6_STORE);
    saved_mstatus = ai_micro_cross_16b_enable_machine_breakpoint_delivery();

    goto_priv(PRIV_HS);
    TEST_SETUP_EXCEPT();
    ai_micro_cross_16b_execute_store_once(vaddr, AI_MICRO_CROSS_16B_STORE_PATTERN);
    TEST_ASSERT("with both execute-PC and address trigger slots armed on the cross-16B store access-fault path, the first access still reports breakpoint in the adjacent-preservation corner",
        excpt.triggered == true &&
        excpt.cause == CAUSE_BKP
    );

    goto_priv(PRIV_M);
    TEST_ASSERT("the first trapped cross-16B store access-fault probe still leaves the two touched aligned doublewords and their immediate neighbors unchanged",
        read64(leading_phys) == leading_pattern &&
        read64(low_phys) == low_pattern &&
        read64(high_phys) == high_pattern &&
        read64(trailing_phys) == trailing_pattern
    );
    ai_micro_cross_16b_restore_machine_status(saved_mstatus);

    goto_priv(PRIV_HS);
    hspt_init();

    goto_priv(PRIV_M);
    hspt_llptw_ppn_high_bit_overflow_setup();
    sfence_vma();

    saved_mstatus = ai_micro_cross_16b_enable_machine_breakpoint_delivery();
    goto_priv(PRIV_HS);
    TEST_SETUP_EXCEPT();
    ai_micro_cross_16b_execute_store_once(vaddr, AI_MICRO_CROSS_16B_STORE_PATTERN);
    TEST_ASSERT("with both execute-PC and address trigger slots still armed on the same cross-16B store access-fault path, the second access still reports breakpoint in the adjacent-preservation corner",
        excpt.triggered == true &&
        excpt.cause == CAUSE_BKP
    );

    goto_priv(PRIV_M);
    TEST_ASSERT("the second trapped cross-16B store access-fault probe still leaves the two touched aligned doublewords and their immediate neighbors unchanged",
        read64(leading_phys) == leading_pattern &&
        read64(low_phys) == low_pattern &&
        read64(high_phys) == high_pattern &&
        read64(trailing_phys) == trailing_pattern
    );
    ai_micro_cross_16b_restore_machine_status(saved_mstatus);

    goto_priv(PRIV_HS);
    hspt_init();

    goto_priv(PRIV_M);
    hspt_llptw_ppn_high_bit_overflow_setup();
    sfence_vma();

    ai_micro_cross_16b_clear_mcontrol6_slot(0);
    saved_mstatus = ai_micro_cross_16b_enable_machine_breakpoint_delivery();
    goto_priv(PRIV_HS);
    TEST_SETUP_EXCEPT();
    ai_micro_cross_16b_execute_store_once(vaddr, AI_MICRO_CROSS_16B_STORE_PATTERN);
    TEST_ASSERT("after two prior dual-slot breakpoint hits, clearing only the execute-PC slot still leaves the cross-16B store address trigger active on the same access-fault path in the adjacent-preservation corner",
        excpt.triggered == true &&
        excpt.cause == CAUSE_BKP
    );

    goto_priv(PRIV_M);
    TEST_ASSERT("with only the address trigger slot armed, the trapped cross-16B store access-fault probe still leaves the two touched aligned doublewords and their immediate neighbors unchanged",
        read64(leading_phys) == leading_pattern &&
        read64(low_phys) == low_pattern &&
        read64(high_phys) == high_pattern &&
        read64(trailing_phys) == trailing_pattern
    );
    ai_micro_cross_16b_restore_machine_status(saved_mstatus);

    goto_priv(PRIV_HS);
    hspt_init();

    goto_priv(PRIV_M);
    hspt_llptw_ppn_high_bit_overflow_setup();
    sfence_vma();

    ai_micro_cross_16b_clear_mcontrol6_slot(1);
    goto_priv(PRIV_HS);
    TEST_SETUP_EXCEPT();
    ai_micro_cross_16b_execute_store_once(vaddr, AI_MICRO_CROSS_16B_STORE_PATTERN);
    trap_triggered = excpt.triggered;
    trap_cause = excpt.cause;
    trap_tval = excpt.tval;

    goto_priv(PRIV_M);
    TEST_ASSERT("after clearing both trigger slots, the same cross-16B store falls back to store access fault with the original misaligned VA and no side effects in the adjacent-preservation corner",
        trap_triggered == true &&
        trap_cause == CAUSE_SAF &&
        trap_tval == vaddr &&
        read64(leading_phys) == leading_pattern &&
        read64(low_phys) == low_pattern &&
        read64(high_phys) == high_pattern &&
        read64(trailing_phys) == trailing_pattern
    );

    goto_priv(PRIV_HS);
    hspt_init();
    sfence_vma();

    TEST_END();
}

bool ai_micro_hs_store_cross_16b_execute_and_address_trigger_access_fault_repeated_breakpoint_then_valid_cross_16b_store_recovers_corner()
{
    TEST_START();

    uintptr_t vaddr = ai_micro_cross_16b_cacheable_addr(VSRWX_GRWX);
    uintptr_t paddr = phys_page_base(VSRWX_GRWX);
    uintptr_t trigger_pc = (uintptr_t)ai_micro_cross_16b_execute_store_once;
    const uint64_t leading_pattern = 0x1122334455667788ULL;
    const uint64_t low_pattern = AI_MICRO_CROSS_16B_LOW_PATTERN;
    const uint64_t high_pattern = AI_MICRO_CROSS_16B_HIGH_PATTERN;
    const uint64_t trailing_pattern = 0x8877665544332211ULL;
    uint64_t saved_mstatus;

    goto_priv(PRIV_M);
    write64(paddr, leading_pattern);
    write64(paddr + 8U, low_pattern);
    write64(paddr + 16U, high_pattern);
    write64(paddr + 24U, trailing_pattern);

    goto_priv(PRIV_HS);
    hspt_init();

    goto_priv(PRIV_M);
    hspt_llptw_ppn_high_bit_overflow_setup();
    sfence_vma();

    ai_micro_cross_16b_clear_mcontrol6_slot(0);
    ai_micro_cross_16b_clear_mcontrol6_slot(1);
    ai_micro_cross_16b_arm_mcontrol6_slot(0, trigger_pc, AI_MICRO_CROSS_16B_MCONTROL6_EXECUTE);
    ai_micro_cross_16b_arm_mcontrol6_slot(1, vaddr, AI_MICRO_CROSS_16B_MCONTROL6_STORE);
    saved_mstatus = ai_micro_cross_16b_enable_machine_breakpoint_delivery();

    goto_priv(PRIV_HS);
    TEST_SETUP_EXCEPT();
    ai_micro_cross_16b_execute_store_once(vaddr, AI_MICRO_CROSS_16B_STORE_PATTERN);
    TEST_ASSERT("with both execute-PC and address trigger slots armed on the cross-16B store access-fault path, the first access still reports breakpoint before recovery",
        excpt.triggered == true &&
        excpt.cause == CAUSE_BKP
    );
    ai_micro_cross_16b_restore_machine_status(saved_mstatus);

    goto_priv(PRIV_HS);
    hspt_init();

    goto_priv(PRIV_M);
    hspt_llptw_ppn_high_bit_overflow_setup();
    sfence_vma();

    saved_mstatus = ai_micro_cross_16b_enable_machine_breakpoint_delivery();
    goto_priv(PRIV_HS);
    TEST_SETUP_EXCEPT();
    ai_micro_cross_16b_execute_store_once(vaddr, AI_MICRO_CROSS_16B_STORE_PATTERN);
    TEST_ASSERT("with both trigger slots still armed on the same cross-16B store access-fault path, the second access still reports breakpoint before recovery",
        excpt.triggered == true &&
        excpt.cause == CAUSE_BKP
    );
    ai_micro_cross_16b_restore_machine_status(saved_mstatus);

    goto_priv(PRIV_HS);
    hspt_init();

    goto_priv(PRIV_M);
    hspt_llptw_ppn_high_bit_overflow_setup();
    sfence_vma();

    ai_micro_cross_16b_clear_mcontrol6_slot(0);
    saved_mstatus = ai_micro_cross_16b_enable_machine_breakpoint_delivery();
    goto_priv(PRIV_HS);
    TEST_SETUP_EXCEPT();
    ai_micro_cross_16b_execute_store_once(vaddr, AI_MICRO_CROSS_16B_STORE_PATTERN);
    TEST_ASSERT("after two prior dual-slot breakpoint hits, clearing only the execute-PC slot still leaves the cross-16B store address trigger active on the same access-fault path before recovery",
        excpt.triggered == true &&
        excpt.cause == CAUSE_BKP
    );
    ai_micro_cross_16b_restore_machine_status(saved_mstatus);

    goto_priv(PRIV_HS);
    hspt_init();

    goto_priv(PRIV_M);
    hspt_llptw_ppn_high_bit_overflow_setup();
    sfence_vma();

    ai_micro_cross_16b_clear_mcontrol6_slot(1);
    goto_priv(PRIV_HS);
    TEST_SETUP_EXCEPT();
    ai_micro_cross_16b_execute_store_once(vaddr, AI_MICRO_CROSS_16B_STORE_PATTERN);
    TEST_ASSERT("after clearing both trigger slots, the same cross-16B store first falls back to store access fault with the original misaligned VA before recovery",
        excpt.triggered == true &&
        excpt.cause == CAUSE_SAF &&
        excpt.tval == vaddr
    );

    goto_priv(PRIV_M);
    TEST_ASSERT("the fallback cross-16B store access-fault path still leaves both participating doublewords and their adjacent sentinels unchanged before recovery",
        read64(paddr) == leading_pattern &&
        read64(paddr + 8U) == low_pattern &&
        read64(paddr + 16U) == high_pattern &&
        read64(paddr + 24U) == trailing_pattern
    );

    goto_priv(PRIV_HS);
    hspt_init();
    sfence_vma();

    TEST_SETUP_EXCEPT();
    sd(vaddr, AI_MICRO_CROSS_16B_STORE_PATTERN);
    TEST_ASSERT("after repeated trigger hits and the fallback access fault, restoring translation lets the same cacheable cross-16B store succeed normally",
        excpt.triggered == false
    );

    goto_priv(PRIV_M);
    TEST_ASSERT("the recovered cross-16B store updates only the two participating doublewords while the adjacent doublewords stay unchanged after the trigger-plus-access-fault sequence",
        read64(paddr) == leading_pattern &&
        read64(paddr + 8U) == 0xef66554433221100ULL &&
        read64(paddr + 16U) == 0xff0123456789abcdULL &&
        read64(paddr + 24U) == trailing_pattern
    );

    TEST_END();
}

bool ai_micro_hs_store_cross_16b_execute_and_address_trigger_access_fault_repeated_breakpoint_then_valid_cross_16b_store_adjacent_preserved_corner()
{
    TEST_START();

    uintptr_t vaddr = ai_micro_cross_16b_cacheable_addr(VSRWX_GRWX);
    uintptr_t paddr = phys_page_base(VSRWX_GRWX);
    uintptr_t leading_phys = paddr;
    uintptr_t low_phys = paddr + 8U;
    uintptr_t high_phys = paddr + 16U;
    uintptr_t trailing_phys = paddr + 24U;
    uintptr_t trigger_pc = (uintptr_t)ai_micro_cross_16b_execute_store_once;
    const uint64_t leading_pattern = 0x1122334455667788ULL;
    const uint64_t low_pattern = AI_MICRO_CROSS_16B_LOW_PATTERN;
    const uint64_t high_pattern = AI_MICRO_CROSS_16B_HIGH_PATTERN;
    const uint64_t trailing_pattern = 0x8877665544332211ULL;
    uint64_t saved_mstatus;

    goto_priv(PRIV_M);
    write64(leading_phys, leading_pattern);
    write64(low_phys, low_pattern);
    write64(high_phys, high_pattern);
    write64(trailing_phys, trailing_pattern);

    goto_priv(PRIV_HS);
    hspt_init();

    goto_priv(PRIV_M);
    hspt_llptw_ppn_high_bit_overflow_setup();
    sfence_vma();

    ai_micro_cross_16b_clear_mcontrol6_slot(0);
    ai_micro_cross_16b_clear_mcontrol6_slot(1);
    ai_micro_cross_16b_arm_mcontrol6_slot(0, trigger_pc, AI_MICRO_CROSS_16B_MCONTROL6_EXECUTE);
    ai_micro_cross_16b_arm_mcontrol6_slot(1, vaddr, AI_MICRO_CROSS_16B_MCONTROL6_STORE);
    saved_mstatus = ai_micro_cross_16b_enable_machine_breakpoint_delivery();

    goto_priv(PRIV_HS);
    TEST_SETUP_EXCEPT();
    ai_micro_cross_16b_execute_store_once(vaddr, AI_MICRO_CROSS_16B_STORE_PATTERN);
    TEST_ASSERT("with both execute-PC and address trigger slots armed on the cross-16B store access-fault path, the first access still reports breakpoint before recovery in the adjacent-preservation corner",
        excpt.triggered == true &&
        excpt.cause == CAUSE_BKP
    );

    goto_priv(PRIV_M);
    TEST_ASSERT("the first trapped trigger-plus-access-fault store probe still leaves the future recovery target doublewords and their adjacent doublewords unchanged",
        read64(leading_phys) == leading_pattern &&
        read64(low_phys) == low_pattern &&
        read64(high_phys) == high_pattern &&
        read64(trailing_phys) == trailing_pattern
    );
    ai_micro_cross_16b_restore_machine_status(saved_mstatus);

    goto_priv(PRIV_HS);
    hspt_init();

    goto_priv(PRIV_M);
    hspt_llptw_ppn_high_bit_overflow_setup();
    sfence_vma();

    saved_mstatus = ai_micro_cross_16b_enable_machine_breakpoint_delivery();
    goto_priv(PRIV_HS);
    TEST_SETUP_EXCEPT();
    ai_micro_cross_16b_execute_store_once(vaddr, AI_MICRO_CROSS_16B_STORE_PATTERN);
    TEST_ASSERT("with both trigger slots still armed on the same cross-16B store access-fault path, the second access still reports breakpoint before recovery in the adjacent-preservation corner",
        excpt.triggered == true &&
        excpt.cause == CAUSE_BKP
    );

    goto_priv(PRIV_M);
    TEST_ASSERT("the second trapped trigger-plus-access-fault store probe still leaves the future recovery target doublewords and their adjacent doublewords unchanged",
        read64(leading_phys) == leading_pattern &&
        read64(low_phys) == low_pattern &&
        read64(high_phys) == high_pattern &&
        read64(trailing_phys) == trailing_pattern
    );
    ai_micro_cross_16b_restore_machine_status(saved_mstatus);

    goto_priv(PRIV_HS);
    hspt_init();

    goto_priv(PRIV_M);
    hspt_llptw_ppn_high_bit_overflow_setup();
    sfence_vma();

    ai_micro_cross_16b_clear_mcontrol6_slot(0);
    saved_mstatus = ai_micro_cross_16b_enable_machine_breakpoint_delivery();
    goto_priv(PRIV_HS);
    TEST_SETUP_EXCEPT();
    ai_micro_cross_16b_execute_store_once(vaddr, AI_MICRO_CROSS_16B_STORE_PATTERN);
    TEST_ASSERT("after two prior dual-slot breakpoint hits, clearing only the execute-PC slot still leaves the cross-16B store address trigger active on the same access-fault path before recovery in the adjacent-preservation corner",
        excpt.triggered == true &&
        excpt.cause == CAUSE_BKP
    );

    goto_priv(PRIV_M);
    TEST_ASSERT("with only the address trigger slot armed, the trapped trigger-plus-access-fault store probe still leaves the future recovery target doublewords and their adjacent doublewords unchanged",
        read64(leading_phys) == leading_pattern &&
        read64(low_phys) == low_pattern &&
        read64(high_phys) == high_pattern &&
        read64(trailing_phys) == trailing_pattern
    );
    ai_micro_cross_16b_restore_machine_status(saved_mstatus);

    goto_priv(PRIV_HS);
    hspt_init();

    goto_priv(PRIV_M);
    hspt_llptw_ppn_high_bit_overflow_setup();
    sfence_vma();

    ai_micro_cross_16b_clear_mcontrol6_slot(1);
    goto_priv(PRIV_HS);
    TEST_SETUP_EXCEPT();
    ai_micro_cross_16b_execute_store_once(vaddr, AI_MICRO_CROSS_16B_STORE_PATTERN);
    TEST_ASSERT("after clearing both trigger slots, the same cross-16B store first falls back to store access fault with the original misaligned VA before recovery in the adjacent-preservation corner",
        excpt.triggered == true &&
        excpt.cause == CAUSE_SAF &&
        excpt.tval == vaddr
    );

    goto_priv(PRIV_M);
    TEST_ASSERT("the fallback cross-16B store access-fault path still leaves the future recovery target doublewords and their adjacent doublewords unchanged",
        read64(leading_phys) == leading_pattern &&
        read64(low_phys) == low_pattern &&
        read64(high_phys) == high_pattern &&
        read64(trailing_phys) == trailing_pattern
    );

    goto_priv(PRIV_HS);
    hspt_init();
    sfence_vma();

    TEST_SETUP_EXCEPT();
    sd(vaddr, AI_MICRO_CROSS_16B_STORE_PATTERN);
    TEST_ASSERT("after repeated trigger hits and the fallback access fault, restoring translation lets the same cacheable cross-16B store succeed normally in the adjacent-preservation corner",
        excpt.triggered == false
    );

    goto_priv(PRIV_M);
    TEST_ASSERT("the recovered cross-16B store updates only the two participating doublewords while the adjacent doublewords stay unchanged in the adjacent-preservation corner",
        read64(leading_phys) == leading_pattern &&
        read64(low_phys) == 0xef66554433221100ULL &&
        read64(high_phys) == 0xff0123456789abcdULL &&
        read64(trailing_phys) == trailing_pattern
    );

    TEST_END();
}

bool ai_micro_hs_load_cross_16b_page_fault()
{
    TEST_START();

    uintptr_t vaddr = ai_micro_cross_16b_cacheable_addr(VSI_GI);

    goto_priv(PRIV_HS);
    hspt_init();
    sfence_vma();

    TEST_SETUP_EXCEPT();
    ld(vaddr);
    TEST_ASSERT("an HS load that is misaligned across 16B reports load page fault when translation resolves to an invalid leaf",
        excpt.triggered == true &&
        excpt.cause == CAUSE_LPF &&
        excpt.tval == vaddr
    );

    TEST_END();
}

bool ai_micro_hs_load_cross_16b_page_fault_then_valid_cross_16b_load_recovers_corner()
{
    TEST_START();

    uintptr_t fault_vaddr = ai_micro_cross_16b_cacheable_addr(VSI_GI);
    uintptr_t ok_vaddr = ai_micro_cross_16b_cacheable_addr(VSRWX_GRWX);
    uintptr_t ok_phys = phys_page_base(VSRWX_GRWX);
    uint64_t value = 0;

    goto_priv(PRIV_M);
    write64(ok_phys + 8U, AI_MICRO_CROSS_16B_LOW_PATTERN);
    write64(ok_phys + 16U, AI_MICRO_CROSS_16B_HIGH_PATTERN);

    goto_priv(PRIV_HS);
    hspt_init();
    sfence_vma();

    TEST_SETUP_EXCEPT();
    ld(fault_vaddr);
    TEST_ASSERT("a cross-16B HS load first reports load page fault when the misaligned access lands on an invalid leaf",
        excpt.triggered == true &&
        excpt.cause == CAUSE_LPF &&
        excpt.tval == fault_vaddr
    );

    TEST_SETUP_EXCEPT();
    value = ld(ok_vaddr);
    TEST_ASSERT("after the cross-16B load page fault, a following cacheable cross-16B load on a valid page still succeeds immediately",
        excpt.triggered == false &&
        value == 0xeeddccbbaa998877ULL
    );

    TEST_END();
}

bool ai_micro_hs_load_cross_16b_page_fault_then_valid_cross_16b_load_adjacent_preserved_corner()
{
    TEST_START();

    uintptr_t fault_vaddr = ai_micro_cross_16b_cacheable_addr(VSI_GI);
    uintptr_t ok_vaddr = ai_micro_cross_16b_cacheable_addr(VSRWX_GRWX);
    uintptr_t ok_phys = phys_page_base(VSRWX_GRWX);
    uintptr_t leading_phys = ok_phys;
    uintptr_t low_phys = ok_phys + 8U;
    uintptr_t high_phys = ok_phys + 16U;
    uintptr_t trailing_phys = ok_phys + 24U;
    const uint64_t leading_pattern = 0x1122334455667788ULL;
    const uint64_t low_pattern = AI_MICRO_CROSS_16B_LOW_PATTERN;
    const uint64_t high_pattern = AI_MICRO_CROSS_16B_HIGH_PATTERN;
    const uint64_t trailing_pattern = 0x8877665544332211ULL;
    uint64_t value = 0;

    goto_priv(PRIV_M);
    write64(leading_phys, leading_pattern);
    write64(low_phys, low_pattern);
    write64(high_phys, high_pattern);
    write64(trailing_phys, trailing_pattern);

    goto_priv(PRIV_HS);
    hspt_init();
    sfence_vma();

    TEST_SETUP_EXCEPT();
    ld(fault_vaddr);
    TEST_ASSERT("a cross-16B HS load first reports load page fault when the misaligned access lands on an invalid leaf in the adjacent-preservation corner",
        excpt.triggered == true &&
        excpt.cause == CAUSE_LPF &&
        excpt.tval == fault_vaddr
    );

    goto_priv(PRIV_M);
    TEST_ASSERT("the cross-16B load page fault leaves both participating doublewords and their adjacent doublewords unchanged before recovery",
        read64(leading_phys) == leading_pattern &&
        read64(low_phys) == low_pattern &&
        read64(high_phys) == high_pattern &&
        read64(trailing_phys) == trailing_pattern
    );

    goto_priv(PRIV_HS);
    TEST_SETUP_EXCEPT();
    value = ld(ok_vaddr);
    TEST_ASSERT("after the cross-16B load page fault, a following cacheable cross-16B load on a valid page still succeeds immediately in the adjacent-preservation corner",
        excpt.triggered == false &&
        value == 0xeeddccbbaa998877ULL
    );

    goto_priv(PRIV_M);
    TEST_ASSERT("after the recovery load succeeds, both participating doublewords and their adjacent doublewords remain unchanged",
        read64(leading_phys) == leading_pattern &&
        read64(low_phys) == low_pattern &&
        read64(high_phys) == high_pattern &&
        read64(trailing_phys) == trailing_pattern
    );

    TEST_END();
}

bool ai_micro_hs_replayq_delayed_misalign_request_not_rebound_to_reused_entry_corner()
{
    TEST_START();
    test_status = test_status &&
        ai_micro_cross_16b_page_fault_then_other_load_no_rebind(false);
    TEST_END();
}

bool ai_micro_hs_replayq_delayed_misalign_request_not_rebound_to_reused_entry_adjacent_preserved_corner()
{
    TEST_START();
    test_status = test_status &&
        ai_micro_cross_16b_page_fault_then_other_load_no_rebind(true);
    TEST_END();
}

bool ai_micro_hs_older_replayq_misalign_not_starved_by_repeated_younger_direct_requests_corner()
{
    TEST_START();
    test_status = test_status &&
        ai_micro_cross_16b_page_fault_then_repeated_younger_load_pressure(false);
    TEST_END();
}

bool ai_micro_hs_older_replayq_misalign_not_starved_by_repeated_younger_direct_requests_adjacent_preserved_corner()
{
    TEST_START();
    test_status = test_status &&
        ai_micro_cross_16b_page_fault_then_repeated_younger_load_pressure(true);
    TEST_END();
}

bool ai_micro_hs_replayq_head_misalign_not_freed_when_direct_exu_misalign_wins_misalignbuffer_corner()
{
    TEST_START();
    bool ok = ai_micro_cross_16b_page_fault_then_repeated_younger_load_pressure(false);
    TEST_ASSERT("replayQ-head cross-16B load still completes with its own repaired payload under repeated younger direct misalign pressure",
        ok
    );
    TEST_END();
}

bool ai_micro_hs_shared_misalignbuffer_misalign_mux_no_false_schedule_or_free_of_replay_head_corner()
{
    TEST_START();
    bool ok = ai_micro_cross_16b_page_fault_then_repeated_younger_load_pressure(true);
    TEST_ASSERT("shared MAB arbitration pressure preserves the replay-head owner and adjacent cacheable boundary sentinels",
        ok
    );
    TEST_END();
}

bool ai_micro_hs_robhead_two_misalign_replay_entries_same_row_follow_uopidx_order_corner()
{
    TEST_START();
    test_status = test_status &&
        ai_micro_cross_16b_dual_fault_then_staged_recovery(false);
    TEST_END();
}

bool ai_micro_hs_same_rob_row_dual_misalign_load_replay_not_selected_by_physical_index_corner()
{
    TEST_START();
    test_status = test_status &&
        ai_micro_cross_16b_dual_fault_then_staged_recovery(true);
    TEST_END();
}

bool ai_micro_hs_replay_port_cold_counter_not_inherited_by_next_misalign_replay_owner_corner()
{
    TEST_START();
    bool ok = ai_micro_cross_16b_sequential_replay_owners_under_direct_pressure(false);
    TEST_ASSERT("sequential replay owners on the misalign replay path make progress without inheriting stale port cooldown",
        ok
    );
    TEST_END();
}

bool ai_micro_hs_replay_port_cold_counter_switch_owner_adjacent_preserved_corner()
{
    TEST_START();
    bool ok = ai_micro_cross_16b_sequential_replay_owners_under_direct_pressure(true);
    TEST_ASSERT("replay-port owner switch under direct misalign pressure preserves repaired payloads and adjacent guard words",
        ok
    );
    TEST_END();
}

bool ai_micro_hs_load_cross_16b_access_fault()
{
    TEST_START();

    uintptr_t vaddr = ai_micro_cross_16b_cacheable_addr(VSRWX_GRWX);

    goto_priv(PRIV_HS);
    hspt_init();

    goto_priv(PRIV_M);
    hspt_llptw_ppn_high_bit_overflow_setup();
    sfence_vma();

    goto_priv(PRIV_HS);
    TEST_SETUP_EXCEPT();
    ld(vaddr);
    TEST_ASSERT("an HS load that is misaligned across 16B reports load access fault when the leaf PPN overflows",
        excpt.triggered == true &&
        excpt.cause == CAUSE_LAF &&
        excpt.tval == vaddr
    );

    goto_priv(PRIV_HS);
    hspt_init();
    sfence_vma();

    TEST_END();
}

bool ai_micro_hs_load_cross_16b_access_fault_then_valid_cross_16b_load_recovers_corner()
{
    TEST_START();

    uintptr_t vaddr = ai_micro_cross_16b_cacheable_addr(VSRWX_GRWX);
    uintptr_t paddr = phys_page_base(VSRWX_GRWX);
    uint64_t value = 0;

    goto_priv(PRIV_M);
    write64(paddr + 8U, AI_MICRO_CROSS_16B_LOW_PATTERN);
    write64(paddr + 16U, AI_MICRO_CROSS_16B_HIGH_PATTERN);

    goto_priv(PRIV_HS);
    hspt_init();

    goto_priv(PRIV_M);
    hspt_llptw_ppn_high_bit_overflow_setup();
    sfence_vma();

    goto_priv(PRIV_HS);
    TEST_SETUP_EXCEPT();
    ld(vaddr);
    TEST_ASSERT("a cross-16B HS load first reports load access fault when the leaf PPN overflows",
        excpt.triggered == true &&
        excpt.cause == CAUSE_LAF &&
        excpt.tval == vaddr
    );

    goto_priv(PRIV_HS);
    hspt_init();
    sfence_vma();

    TEST_SETUP_EXCEPT();
    value = ld(vaddr);
    TEST_ASSERT("after restoring the translation, the same cacheable cross-16B load succeeds and returns the expected merged payload",
        excpt.triggered == false &&
        value == 0xeeddccbbaa998877ULL
    );

    TEST_END();
}

bool ai_micro_hs_load_cross_16b_access_fault_then_valid_cross_16b_load_adjacent_preserved_corner()
{
    TEST_START();

    uintptr_t vaddr = ai_micro_cross_16b_cacheable_addr(VSRWX_GRWX);
    uintptr_t paddr = phys_page_base(VSRWX_GRWX);
    uintptr_t leading_phys = paddr;
    uintptr_t low_phys = paddr + 8U;
    uintptr_t high_phys = paddr + 16U;
    uintptr_t trailing_phys = paddr + 24U;
    const uint64_t leading_pattern = 0x1122334455667788ULL;
    const uint64_t low_pattern = AI_MICRO_CROSS_16B_LOW_PATTERN;
    const uint64_t high_pattern = AI_MICRO_CROSS_16B_HIGH_PATTERN;
    const uint64_t trailing_pattern = 0x8877665544332211ULL;
    uint64_t value = 0;

    goto_priv(PRIV_M);
    write64(leading_phys, leading_pattern);
    write64(low_phys, low_pattern);
    write64(high_phys, high_pattern);
    write64(trailing_phys, trailing_pattern);

    goto_priv(PRIV_HS);
    hspt_init();

    goto_priv(PRIV_M);
    hspt_llptw_ppn_high_bit_overflow_setup();
    sfence_vma();

    goto_priv(PRIV_HS);
    TEST_SETUP_EXCEPT();
    ld(vaddr);
    TEST_ASSERT("a cross-16B HS load first reports load access fault when the leaf PPN overflows in the adjacent-preservation corner",
        excpt.triggered == true &&
        excpt.cause == CAUSE_LAF &&
        excpt.tval == vaddr
    );

    goto_priv(PRIV_M);
    TEST_ASSERT("the cross-16B load access fault leaves both participating doublewords and their adjacent doublewords unchanged before recovery",
        read64(leading_phys) == leading_pattern &&
        read64(low_phys) == low_pattern &&
        read64(high_phys) == high_pattern &&
        read64(trailing_phys) == trailing_pattern
    );

    goto_priv(PRIV_HS);
    hspt_init();
    sfence_vma();

    TEST_SETUP_EXCEPT();
    value = ld(vaddr);
    TEST_ASSERT("after restoring the translation, the same cacheable cross-16B load succeeds and returns the expected merged payload in the adjacent-preservation corner",
        excpt.triggered == false &&
        value == 0xeeddccbbaa998877ULL
    );

    goto_priv(PRIV_M);
    TEST_ASSERT("after the recovery load succeeds, both participating doublewords and their adjacent doublewords remain unchanged",
        read64(leading_phys) == leading_pattern &&
        read64(low_phys) == low_pattern &&
        read64(high_phys) == high_pattern &&
        read64(trailing_phys) == trailing_pattern
    );

    TEST_END();
}

bool ai_micro_hs_load_cross_16b_warm_merge_then_fault_then_signedness_switch_recovery_no_stale_state_corner()
{
    TEST_START();
    test_status = test_status &&
        ai_micro_cross_16b_warm_merge_then_fault_then_signedness_switch_recovery(false);
    TEST_END();
}

bool ai_micro_hs_load_cross_16b_warm_merge_then_fault_then_signedness_switch_recovery_adjacent_preserved_corner()
{
    TEST_START();
    test_status = test_status &&
        ai_micro_cross_16b_warm_merge_then_fault_then_signedness_switch_recovery(true);
    TEST_END();
}

bool ai_micro_hs_load_cross_16b_fault_then_different_shape_recovery_no_stale_merge_template_corner()
{
    TEST_START();

    enum {
        AI_MICRO_CROSS_16B_FAULT_RECOVER_LOAD_CASES = 2
    };
    const bool use_access_fault[AI_MICRO_CROSS_16B_FAULT_RECOVER_LOAD_CASES] = {
        false,
        true,
    };
    const enum test_page fault_pages[AI_MICRO_CROSS_16B_FAULT_RECOVER_LOAD_CASES] = {
        VSI_GI,
        VSRWX_GRWX,
    };
    const enum test_page ok_pages[AI_MICRO_CROSS_16B_FAULT_RECOVER_LOAD_CASES] = {
        VSRW_GRW,
        SWITCH1,
    };
    const uintptr_t ok_offsets[AI_MICRO_CROSS_16B_FAULT_RECOVER_LOAD_CASES] = {
        AI_MICRO_CROSS_16B_3B1B_OFFSET,
        12U,
    };
    const unsigned ok_sizes[AI_MICRO_CROSS_16B_FAULT_RECOVER_LOAD_CASES] = {
        4U,
        8U,
    };
    const bool ok_sign_extend[AI_MICRO_CROSS_16B_FAULT_RECOVER_LOAD_CASES] = {
        true,
        false,
    };
    const uint64_t fault_leading_patterns[AI_MICRO_CROSS_16B_FAULT_RECOVER_LOAD_CASES] = {
        0x3141592653589793ULL,
        0x0123456789abcdefULL,
    };
    const uint64_t fault_low_patterns[AI_MICRO_CROSS_16B_FAULT_RECOVER_LOAD_CASES] = {
        0x8070605040302010ULL,
        0x1021324354657687ULL,
    };
    const uint64_t fault_high_patterns[AI_MICRO_CROSS_16B_FAULT_RECOVER_LOAD_CASES] = {
        0x1122334455667799ULL,
        0x98badcfe10325476ULL,
    };
    const uint64_t fault_trailing_patterns[AI_MICRO_CROSS_16B_FAULT_RECOVER_LOAD_CASES] = {
        0x2718281828459045ULL,
        0xfedcba9876543210ULL,
    };
    const uint64_t ok_leading_patterns[AI_MICRO_CROSS_16B_FAULT_RECOVER_LOAD_CASES] = {
        0x1111222233334444ULL,
        0x5566778899aabbccULL,
    };
    const uint64_t ok_low_patterns[AI_MICRO_CROSS_16B_FAULT_RECOVER_LOAD_CASES] = {
        0x7766554433221100ULL,
        0xf0e1d2c3b4a59687ULL,
    };
    const uint64_t ok_high_patterns[AI_MICRO_CROSS_16B_FAULT_RECOVER_LOAD_CASES] = {
        0x99aabbccddeeff80ULL,
        0x1021324354657687ULL,
    };
    const uint64_t ok_trailing_patterns[AI_MICRO_CROSS_16B_FAULT_RECOVER_LOAD_CASES] = {
        0x8877665544332211ULL,
        0x0badf00dfacefeedULL,
    };

    for (unsigned i = 0; i < AI_MICRO_CROSS_16B_FAULT_RECOVER_LOAD_CASES; ++i) {
        const uintptr_t fault_page_phys = phys_page_base(fault_pages[i]);
        const uintptr_t ok_page_phys = phys_page_base(ok_pages[i]);
        const uintptr_t fault_vaddr = ai_micro_cross_16b_cacheable_addr(fault_pages[i]);
        const uintptr_t ok_vaddr =
            ai_micro_cross_16b_cacheable_addr_with_offset(ok_pages[i], ok_offsets[i]);
        const uintptr_t fault_leading_phys = fault_page_phys;
        const uintptr_t fault_low_phys = fault_page_phys + 8U;
        const uintptr_t fault_high_phys = fault_page_phys + 16U;
        const uintptr_t fault_trailing_phys = fault_page_phys + 24U;
        const uintptr_t ok_leading_phys = ok_page_phys;
        const uintptr_t ok_low_phys = ok_page_phys + 8U;
        const uintptr_t ok_high_phys = ok_page_phys + 16U;
        const uintptr_t ok_trailing_phys = ok_page_phys + 24U;
        const uint64_t expected_ok_value = ai_micro_cross_16b_expected_value(
            ok_low_patterns[i],
            ok_high_patterns[i],
            (unsigned)(ok_offsets[i] - 8U),
            ok_sizes[i],
            ok_sign_extend[i]);
        uint64_t observed = 0ULL;

        goto_priv(PRIV_M);
        write64(fault_leading_phys, fault_leading_patterns[i]);
        write64(fault_low_phys, fault_low_patterns[i]);
        write64(fault_high_phys, fault_high_patterns[i]);
        write64(fault_trailing_phys, fault_trailing_patterns[i]);
        write64(ok_leading_phys, ok_leading_patterns[i]);
        write64(ok_low_phys, ok_low_patterns[i]);
        write64(ok_high_phys, ok_high_patterns[i]);
        write64(ok_trailing_phys, ok_trailing_patterns[i]);

        goto_priv(PRIV_HS);
        hspt_init();
        if (use_access_fault[i]) {
            goto_priv(PRIV_M);
            hspt_llptw_ppn_high_bit_overflow_setup();
        } else {
            goto_priv(PRIV_M);
        }
        sfence_vma();

        goto_priv(PRIV_HS);
        TEST_SETUP_EXCEPT();
        ld(fault_vaddr);
        TEST_ASSERT((use_access_fault[i] ?
            "a faulting cross-16B ld first reports load access fault before the later different-shape recovery load is attempted" :
            "a faulting cross-16B ld first reports load page fault before the later different-shape recovery load is attempted"),
            excpt.triggered == true &&
            excpt.cause == (use_access_fault[i] ? CAUSE_LAF : CAUSE_LPF) &&
            excpt.tval == fault_vaddr
        );

        goto_priv(PRIV_M);
        TEST_ASSERT((use_access_fault[i] ?
            "the cross-16B load access-fault episode leaves both the faulting boundary and the later different-shape recovery boundary plus their adjacent doublewords unchanged" :
            "the cross-16B load page-fault episode leaves both the faulting boundary and the later different-shape recovery boundary plus their adjacent doublewords unchanged"),
            read64(fault_leading_phys) == fault_leading_patterns[i] &&
            read64(fault_low_phys) == fault_low_patterns[i] &&
            read64(fault_high_phys) == fault_high_patterns[i] &&
            read64(fault_trailing_phys) == fault_trailing_patterns[i] &&
            read64(ok_leading_phys) == ok_leading_patterns[i] &&
            read64(ok_low_phys) == ok_low_patterns[i] &&
            read64(ok_high_phys) == ok_high_patterns[i] &&
            read64(ok_trailing_phys) == ok_trailing_patterns[i]
        );

        goto_priv(PRIV_HS);
        hspt_init();
        sfence_vma();

        TEST_SETUP_EXCEPT();
        observed = ai_micro_cross_16b_observed_value(
            ok_vaddr, ok_sizes[i], ok_sign_extend[i]);
        TEST_ASSERT((use_access_fault[i] ?
            "after the faulting cross-16B ld access-fault episode is cleared, a later different-shape recovery load returns only its own fresh merged payload with no stale merge-template carryover" :
            "after the faulting cross-16B ld page-fault episode is cleared, a later different-shape recovery load returns only its own fresh merged payload with no stale merge-template carryover"),
            excpt.triggered == false &&
            observed == expected_ok_value
        );

        goto_priv(PRIV_M);
        TEST_ASSERT((use_access_fault[i] ?
            "after the access-fault then different-shape load recovery sequence, the faulting boundary stays untouched and the recovery boundary keeps only its original load-source bytes" :
            "after the page-fault then different-shape load recovery sequence, the faulting boundary stays untouched and the recovery boundary keeps only its original load-source bytes"),
            read64(fault_leading_phys) == fault_leading_patterns[i] &&
            read64(fault_low_phys) == fault_low_patterns[i] &&
            read64(fault_high_phys) == fault_high_patterns[i] &&
            read64(fault_trailing_phys) == fault_trailing_patterns[i] &&
            read64(ok_leading_phys) == ok_leading_patterns[i] &&
            read64(ok_low_phys) == ok_low_patterns[i] &&
            read64(ok_high_phys) == ok_high_patterns[i] &&
            read64(ok_trailing_phys) == ok_trailing_patterns[i]
        );
    }

    goto_priv(PRIV_HS);
    hspt_init();
    sfence_vma();

    TEST_END();
}

bool ai_micro_hs_store_cross_16b_page_fault()
{
    TEST_START();

    uintptr_t vaddr = ai_micro_cross_16b_cacheable_addr(VSI_GI);

    goto_priv(PRIV_HS);
    hspt_init();
    sfence_vma();

    TEST_SETUP_EXCEPT();
    sd(vaddr, AI_MICRO_CROSS_16B_STORE_PATTERN);
    TEST_ASSERT("an HS store that is misaligned across 16B reports store page fault when translation resolves to an invalid leaf",
        excpt.triggered == true &&
        excpt.cause == CAUSE_SPF &&
        excpt.tval == vaddr
    );

    TEST_END();
}

bool ai_micro_hs_store_cross_16b_fault_then_different_shape_recovery_no_stale_split_template_corner()
{
    TEST_START();

    enum {
        AI_MICRO_CROSS_16B_FAULT_RECOVER_STORE_CASES = 2
    };
    const bool use_access_fault[AI_MICRO_CROSS_16B_FAULT_RECOVER_STORE_CASES] = {
        false,
        true,
    };
    const enum test_page fault_pages[AI_MICRO_CROSS_16B_FAULT_RECOVER_STORE_CASES] = {
        VSI_GI,
        VSRWX_GRWX,
    };
    const enum test_page ok_pages[AI_MICRO_CROSS_16B_FAULT_RECOVER_STORE_CASES] = {
        VSRW_GRW,
        SWITCH1,
    };
    const uintptr_t ok_offsets[AI_MICRO_CROSS_16B_FAULT_RECOVER_STORE_CASES] = {
        AI_MICRO_CROSS_16B_3B1B_OFFSET,
        12U,
    };
    const unsigned ok_sizes[AI_MICRO_CROSS_16B_FAULT_RECOVER_STORE_CASES] = {
        4U,
        8U,
    };
    const uint64_t store_values[AI_MICRO_CROSS_16B_FAULT_RECOVER_STORE_CASES] = {
        0x89abcdef01234567ULL,
        0xfedcba9876543210ULL,
    };
    const uint64_t fault_leading_patterns[AI_MICRO_CROSS_16B_FAULT_RECOVER_STORE_CASES] = {
        0x1111222233334444ULL,
        0x5555666677778888ULL,
    };
    const uint64_t fault_low_patterns[AI_MICRO_CROSS_16B_FAULT_RECOVER_STORE_CASES] = {
        0x7766554433221100ULL,
        0x1021324354657687ULL,
    };
    const uint64_t fault_high_patterns[AI_MICRO_CROSS_16B_FAULT_RECOVER_STORE_CASES] = {
        0xffeeddccbbaa9988ULL,
        0x98badcfe10325476ULL,
    };
    const uint64_t fault_trailing_patterns[AI_MICRO_CROSS_16B_FAULT_RECOVER_STORE_CASES] = {
        0x8877665544332211ULL,
        0xfedcba9876543210ULL,
    };
    const uint64_t ok_leading_patterns[AI_MICRO_CROSS_16B_FAULT_RECOVER_STORE_CASES] = {
        0x0123456789abcdefULL,
        0x3141592653589793ULL,
    };
    const uint64_t ok_low_patterns[AI_MICRO_CROSS_16B_FAULT_RECOVER_STORE_CASES] = {
        0xdeadbeefcafef00dULL,
        0xf0e1d2c3b4a59687ULL,
    };
    const uint64_t ok_high_patterns[AI_MICRO_CROSS_16B_FAULT_RECOVER_STORE_CASES] = {
        0x0badf00dfacefeedULL,
        0x1021324354657687ULL,
    };
    const uint64_t ok_trailing_patterns[AI_MICRO_CROSS_16B_FAULT_RECOVER_STORE_CASES] = {
        0x99aabbccddeeff00ULL,
        0x2718281828459045ULL,
    };

    for (unsigned i = 0; i < AI_MICRO_CROSS_16B_FAULT_RECOVER_STORE_CASES; ++i) {
        const uintptr_t fault_page_phys = phys_page_base(fault_pages[i]);
        const uintptr_t ok_page_phys = phys_page_base(ok_pages[i]);
        const uintptr_t fault_vaddr = ai_micro_cross_16b_cacheable_addr(fault_pages[i]);
        const uintptr_t ok_vaddr =
            ai_micro_cross_16b_cacheable_addr_with_offset(ok_pages[i], ok_offsets[i]);
        const uintptr_t fault_leading_phys = fault_page_phys;
        const uintptr_t fault_low_phys = fault_page_phys + 8U;
        const uintptr_t fault_high_phys = fault_page_phys + 16U;
        const uintptr_t fault_trailing_phys = fault_page_phys + 24U;
        const uintptr_t ok_leading_phys = ok_page_phys;
        const uintptr_t ok_low_phys = ok_page_phys + 8U;
        const uintptr_t ok_high_phys = ok_page_phys + 16U;
        const uintptr_t ok_trailing_phys = ok_page_phys + 24U;
        uint64_t expected_ok_low = 0ULL;
        uint64_t expected_ok_high = 0ULL;

        goto_priv(PRIV_M);
        write64(fault_leading_phys, fault_leading_patterns[i]);
        write64(fault_low_phys, fault_low_patterns[i]);
        write64(fault_high_phys, fault_high_patterns[i]);
        write64(fault_trailing_phys, fault_trailing_patterns[i]);
        write64(ok_leading_phys, ok_leading_patterns[i]);
        write64(ok_low_phys, ok_low_patterns[i]);
        write64(ok_high_phys, ok_high_patterns[i]);
        write64(ok_trailing_phys, ok_trailing_patterns[i]);

        goto_priv(PRIV_HS);
        hspt_init();
        if (use_access_fault[i]) {
            goto_priv(PRIV_M);
            hspt_llptw_ppn_high_bit_overflow_setup();
        } else {
            goto_priv(PRIV_M);
        }
        sfence_vma();

        goto_priv(PRIV_HS);
        TEST_SETUP_EXCEPT();
        sd(fault_vaddr, AI_MICRO_CROSS_16B_STORE_PATTERN);
        TEST_ASSERT((use_access_fault[i] ?
            "a faulting cross-16B sd first reports store access fault before the later different-shape recovery store is attempted" :
            "a faulting cross-16B sd first reports store page fault before the later different-shape recovery store is attempted"),
            excpt.triggered == true &&
            excpt.cause == (use_access_fault[i] ? CAUSE_SAF : CAUSE_SPF) &&
            excpt.tval == fault_vaddr
        );

        goto_priv(PRIV_M);
        TEST_ASSERT((use_access_fault[i] ?
            "the cross-16B store access-fault episode leaves both the faulting boundary and the later different-shape recovery boundary plus their adjacent doublewords unchanged" :
            "the cross-16B store page-fault episode leaves both the faulting boundary and the later different-shape recovery boundary plus their adjacent doublewords unchanged"),
            read64(fault_leading_phys) == fault_leading_patterns[i] &&
            read64(fault_low_phys) == fault_low_patterns[i] &&
            read64(fault_high_phys) == fault_high_patterns[i] &&
            read64(fault_trailing_phys) == fault_trailing_patterns[i] &&
            read64(ok_leading_phys) == ok_leading_patterns[i] &&
            read64(ok_low_phys) == ok_low_patterns[i] &&
            read64(ok_high_phys) == ok_high_patterns[i] &&
            read64(ok_trailing_phys) == ok_trailing_patterns[i]
        );

        goto_priv(PRIV_HS);
        hspt_init();
        sfence_vma();

        TEST_SETUP_EXCEPT();
        if (ok_sizes[i] == 4U) {
            sw(ok_vaddr, (uint32_t)store_values[i]);
        } else {
            sd(ok_vaddr, store_values[i]);
        }
        TEST_ASSERT((use_access_fault[i] ?
            "after the faulting cross-16B sd access-fault episode is cleared, a later different-shape recovery store commits without stale split-template carryover" :
            "after the faulting cross-16B sd page-fault episode is cleared, a later different-shape recovery store commits without stale split-template carryover"),
            excpt.triggered == false
        );

        ai_micro_cross_16b_expected_words_after_store(
            ok_low_patterns[i],
            ok_high_patterns[i],
            (unsigned)(ok_offsets[i] - 8U),
            ok_sizes[i],
            store_values[i],
            &expected_ok_low,
            &expected_ok_high);

        goto_priv(PRIV_M);
        TEST_ASSERT((use_access_fault[i] ?
            "after the access-fault then different-shape store recovery sequence, the faulting boundary stays untouched and the recovery boundary reflects only its own exact byte image" :
            "after the page-fault then different-shape store recovery sequence, the faulting boundary stays untouched and the recovery boundary reflects only its own exact byte image"),
            read64(fault_leading_phys) == fault_leading_patterns[i] &&
            read64(fault_low_phys) == fault_low_patterns[i] &&
            read64(fault_high_phys) == fault_high_patterns[i] &&
            read64(fault_trailing_phys) == fault_trailing_patterns[i] &&
            read64(ok_leading_phys) == ok_leading_patterns[i] &&
            read64(ok_low_phys) == expected_ok_low &&
            read64(ok_high_phys) == expected_ok_high &&
            read64(ok_trailing_phys) == ok_trailing_patterns[i]
        );
    }

    goto_priv(PRIV_HS);
    hspt_init();
    sfence_vma();

    TEST_END();
}

bool ai_micro_hs_load_cross_16b_fault_then_5b3b_6b2b_recovery_no_stale_merge_template_corner()
{
    TEST_START();

    enum {
        AI_MICRO_CROSS_16B_FAULT_RECOVER_HIGH_MID_LOAD_CASES = 4
    };
    const bool use_access_fault[AI_MICRO_CROSS_16B_FAULT_RECOVER_HIGH_MID_LOAD_CASES] = {
        false,
        false,
        true,
        true,
    };
    const enum test_page fault_pages[AI_MICRO_CROSS_16B_FAULT_RECOVER_HIGH_MID_LOAD_CASES] = {
        VSI_GI,
        VSI_GI,
        VSRWX_GRWX,
        VSRWX_GRWX,
    };
    const enum test_page ok_pages[AI_MICRO_CROSS_16B_FAULT_RECOVER_HIGH_MID_LOAD_CASES] = {
        VSRW_GRW,
        SWITCH1,
        VSRW_GRW,
        SWITCH1,
    };
    const uintptr_t ok_offsets[AI_MICRO_CROSS_16B_FAULT_RECOVER_HIGH_MID_LOAD_CASES] = {
        11U,
        10U,
        11U,
        10U,
    };
    const uint64_t fault_leading_patterns[AI_MICRO_CROSS_16B_FAULT_RECOVER_HIGH_MID_LOAD_CASES] = {
        0x3141592653589793ULL,
        0x0123456789abcdefULL,
        0x5566778899aabbccULL,
        0xfedcba9876543210ULL,
    };
    const uint64_t fault_low_patterns[AI_MICRO_CROSS_16B_FAULT_RECOVER_HIGH_MID_LOAD_CASES] = {
        0x8070605040302010ULL,
        0x1021324354657687ULL,
        0x89abcdef01234567ULL,
        0x0badf00dfacefeedULL,
    };
    const uint64_t fault_high_patterns[AI_MICRO_CROSS_16B_FAULT_RECOVER_HIGH_MID_LOAD_CASES] = {
        0x1122334455667799ULL,
        0x98badcfe10325476ULL,
        0x66778899aabbccddULL,
        0x8877665544332211ULL,
    };
    const uint64_t fault_trailing_patterns[AI_MICRO_CROSS_16B_FAULT_RECOVER_HIGH_MID_LOAD_CASES] = {
        0x2718281828459045ULL,
        0xdeadbeefcafef00dULL,
        0x123456789abcdef0ULL,
        0x5a5aa5a5cc33dd44ULL,
    };
    const uint64_t ok_leading_patterns[AI_MICRO_CROSS_16B_FAULT_RECOVER_HIGH_MID_LOAD_CASES] = {
        0x1111222233334444ULL,
        0x5566778899aabbccULL,
        0x4444333322221111ULL,
        0x9999aaaabbbbccccULL,
    };
    const uint64_t ok_low_patterns[AI_MICRO_CROSS_16B_FAULT_RECOVER_HIGH_MID_LOAD_CASES] = {
        0x7766554433221100ULL,
        0xf0e1d2c3b4a59687ULL,
        0x1021324354657687ULL,
        0xdeadbeefcafef00dULL,
    };
    const uint64_t ok_high_patterns[AI_MICRO_CROSS_16B_FAULT_RECOVER_HIGH_MID_LOAD_CASES] = {
        0xffeeddccbbaa9988ULL,
        0x1021324354657687ULL,
        0x3141592653589793ULL,
        0x0badf00dfacefeedULL,
    };
    const uint64_t ok_trailing_patterns[AI_MICRO_CROSS_16B_FAULT_RECOVER_HIGH_MID_LOAD_CASES] = {
        0x8877665544332211ULL,
        0x2718281828459045ULL,
        0xddddccccbbbbaaaaULL,
        0x13579bdf2468ace0ULL,
    };

    for (unsigned i = 0; i < AI_MICRO_CROSS_16B_FAULT_RECOVER_HIGH_MID_LOAD_CASES; ++i) {
        const uintptr_t fault_page_phys = phys_page_base(fault_pages[i]);
        const uintptr_t ok_page_phys = phys_page_base(ok_pages[i]);
        const uintptr_t fault_vaddr = ai_micro_cross_16b_cacheable_addr(fault_pages[i]);
        const uintptr_t ok_vaddr =
            ai_micro_cross_16b_cacheable_addr_with_offset(ok_pages[i], ok_offsets[i]);
        const uintptr_t fault_leading_phys = fault_page_phys;
        const uintptr_t fault_low_phys = fault_page_phys + 8U;
        const uintptr_t fault_high_phys = fault_page_phys + 16U;
        const uintptr_t fault_trailing_phys = fault_page_phys + 24U;
        const uintptr_t ok_leading_phys = ok_page_phys;
        const uintptr_t ok_low_phys = ok_page_phys + 8U;
        const uintptr_t ok_high_phys = ok_page_phys + 16U;
        const uintptr_t ok_trailing_phys = ok_page_phys + 24U;
        const uint64_t expected_ok_value = ai_micro_cross_16b_expected_value(
            ok_low_patterns[i],
            ok_high_patterns[i],
            (unsigned)(ok_offsets[i] - 8U),
            8U,
            false);
        uint64_t observed = 0ULL;

        goto_priv(PRIV_M);
        write64(fault_leading_phys, fault_leading_patterns[i]);
        write64(fault_low_phys, fault_low_patterns[i]);
        write64(fault_high_phys, fault_high_patterns[i]);
        write64(fault_trailing_phys, fault_trailing_patterns[i]);
        write64(ok_leading_phys, ok_leading_patterns[i]);
        write64(ok_low_phys, ok_low_patterns[i]);
        write64(ok_high_phys, ok_high_patterns[i]);
        write64(ok_trailing_phys, ok_trailing_patterns[i]);

        goto_priv(PRIV_HS);
        hspt_init();
        if (use_access_fault[i]) {
            goto_priv(PRIV_M);
            hspt_llptw_ppn_high_bit_overflow_setup();
        } else {
            goto_priv(PRIV_M);
        }
        sfence_vma();

        goto_priv(PRIV_HS);
        TEST_SETUP_EXCEPT();
        ld(fault_vaddr);
        TEST_ASSERT((use_access_fault[i] ?
            "a faulting cross-16B ld first reports load access fault before the later mirrored 5B+3B or 6B+2B recovery load is attempted" :
            "a faulting cross-16B ld first reports load page fault before the later mirrored 5B+3B or 6B+2B recovery load is attempted"),
            excpt.triggered == true &&
            excpt.cause == (use_access_fault[i] ? CAUSE_LAF : CAUSE_LPF) &&
            excpt.tval == fault_vaddr
        );

        goto_priv(PRIV_M);
        TEST_ASSERT((use_access_fault[i] ?
            "the cross-16B load access-fault episode leaves both the faulting boundary and the later mirrored middle-ratio recovery boundary plus their adjacent doublewords unchanged" :
            "the cross-16B load page-fault episode leaves both the faulting boundary and the later mirrored middle-ratio recovery boundary plus their adjacent doublewords unchanged"),
            read64(fault_leading_phys) == fault_leading_patterns[i] &&
            read64(fault_low_phys) == fault_low_patterns[i] &&
            read64(fault_high_phys) == fault_high_patterns[i] &&
            read64(fault_trailing_phys) == fault_trailing_patterns[i] &&
            read64(ok_leading_phys) == ok_leading_patterns[i] &&
            read64(ok_low_phys) == ok_low_patterns[i] &&
            read64(ok_high_phys) == ok_high_patterns[i] &&
            read64(ok_trailing_phys) == ok_trailing_patterns[i]
        );

        goto_priv(PRIV_HS);
        hspt_init();
        sfence_vma();

        TEST_SETUP_EXCEPT();
        observed = ld(ok_vaddr);
        TEST_ASSERT((use_access_fault[i] ?
            "after the faulting cross-16B ld access-fault episode is cleared, a later mirrored 5B+3B or 6B+2B recovery load returns only its own fresh merged payload with no stale merge-template carryover" :
            "after the faulting cross-16B ld page-fault episode is cleared, a later mirrored 5B+3B or 6B+2B recovery load returns only its own fresh merged payload with no stale merge-template carryover"),
            excpt.triggered == false &&
            observed == expected_ok_value
        );

        goto_priv(PRIV_M);
        TEST_ASSERT((use_access_fault[i] ?
            "after the access-fault then mirrored middle-ratio load recovery sequence, the faulting boundary stays untouched and the recovery boundary keeps only its original load-source bytes" :
            "after the page-fault then mirrored middle-ratio load recovery sequence, the faulting boundary stays untouched and the recovery boundary keeps only its original load-source bytes"),
            read64(fault_leading_phys) == fault_leading_patterns[i] &&
            read64(fault_low_phys) == fault_low_patterns[i] &&
            read64(fault_high_phys) == fault_high_patterns[i] &&
            read64(fault_trailing_phys) == fault_trailing_patterns[i] &&
            read64(ok_leading_phys) == ok_leading_patterns[i] &&
            read64(ok_low_phys) == ok_low_patterns[i] &&
            read64(ok_high_phys) == ok_high_patterns[i] &&
            read64(ok_trailing_phys) == ok_trailing_patterns[i]
        );
    }

    goto_priv(PRIV_HS);
    hspt_init();
    sfence_vma();

    TEST_END();
}
