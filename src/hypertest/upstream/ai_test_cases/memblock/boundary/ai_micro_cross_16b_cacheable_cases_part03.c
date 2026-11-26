#include "ai_micro_cross_16b_cacheable_common.h"

bool ai_micro_hs_load_cross_16b_dual_address_trigger_only_original_matches_spike_corner()
{
    TEST_START();

    uintptr_t paddr = phys_page_base(VSRWX_GRWX);
    uintptr_t vaddr = ai_micro_cross_16b_cacheable_addr(VSRWX_GRWX);
    uintptr_t second_half_vaddr = ai_micro_cross_16b_second_half_addr(VSRWX_GRWX);

    goto_priv(PRIV_M);
    write64(paddr + 8U, AI_MICRO_CROSS_16B_LOW_PATTERN);
    write64(paddr + 16U, AI_MICRO_CROSS_16B_HIGH_PATTERN);

    goto_priv(PRIV_HS);
    hspt_init();
    sfence_vma();

    ai_micro_cross_16b_clear_mcontrol6_slot(0);
    ai_micro_cross_16b_clear_mcontrol6_slot(1);
    ai_micro_cross_16b_arm_mcontrol6_slot(0, vaddr, AI_MICRO_CROSS_16B_MCONTROL6_LOAD);
    ai_micro_cross_16b_arm_mcontrol6_slot(1, second_half_vaddr, AI_MICRO_CROSS_16B_MCONTROL6_LOAD);
    uint64_t saved_mstatus = ai_micro_cross_16b_enable_machine_breakpoint_delivery();

    goto_priv(PRIV_HS);
    TEST_SETUP_EXCEPT();
    ai_micro_cross_16b_execute_load_once(vaddr);
    TEST_ASSERT("with both the original misaligned address slot and the true second-half-start slot armed, Spike still reports breakpoint on the cross-16B load because only the original split-load address matches",
        excpt.triggered == true &&
        excpt.cause == CAUSE_BKP
    );
    ai_micro_cross_16b_restore_machine_status(saved_mstatus);

    ai_micro_cross_16b_clear_mcontrol6_slot(0);
    saved_mstatus = ai_micro_cross_16b_enable_machine_breakpoint_delivery();

    goto_priv(PRIV_HS);
    TEST_SETUP_EXCEPT();
    uint64_t value = ai_micro_cross_16b_execute_load_once(vaddr);
    TEST_ASSERT("after clearing the original misaligned-address slot, the remaining second-half-start address slot stays ignored on Spike and the cross-16B load completes normally",
        excpt.triggered == false &&
        value == 0xeeddccbbaa998877ULL
    );
    ai_micro_cross_16b_restore_machine_status(saved_mstatus);

    ai_micro_cross_16b_clear_mcontrol6_slot(1);
    TEST_END();
}

bool ai_micro_hs_load_cross_16b_dual_address_trigger_repeated_breakpoint_then_completion_spike_corner()
{
    TEST_START();

    uintptr_t paddr = phys_page_base(VSRWX_GRWX);
    uintptr_t vaddr = ai_micro_cross_16b_cacheable_addr(VSRWX_GRWX);
    uintptr_t second_half_vaddr = ai_micro_cross_16b_second_half_addr(VSRWX_GRWX);
    uint64_t value;

    goto_priv(PRIV_M);
    write64(paddr + 8U, AI_MICRO_CROSS_16B_LOW_PATTERN);
    write64(paddr + 16U, AI_MICRO_CROSS_16B_HIGH_PATTERN);

    goto_priv(PRIV_HS);
    hspt_init();
    sfence_vma();

    ai_micro_cross_16b_clear_mcontrol6_slot(0);
    ai_micro_cross_16b_clear_mcontrol6_slot(1);
    ai_micro_cross_16b_arm_mcontrol6_slot(0, vaddr, AI_MICRO_CROSS_16B_MCONTROL6_LOAD);
    ai_micro_cross_16b_arm_mcontrol6_slot(1, second_half_vaddr, AI_MICRO_CROSS_16B_MCONTROL6_LOAD);
    uint64_t saved_mstatus = ai_micro_cross_16b_enable_machine_breakpoint_delivery();

    goto_priv(PRIV_HS);
    TEST_SETUP_EXCEPT();
    ai_micro_cross_16b_execute_load_once(vaddr);
    TEST_ASSERT("with both split-load address slots armed, the first cross-16B load still reports breakpoint on Spike because only the original misaligned vaddr matches",
        excpt.triggered == true &&
        excpt.cause == CAUSE_BKP
    );
    ai_micro_cross_16b_restore_machine_status(saved_mstatus);

    saved_mstatus = ai_micro_cross_16b_enable_machine_breakpoint_delivery();
    goto_priv(PRIV_HS);
    TEST_SETUP_EXCEPT();
    ai_micro_cross_16b_execute_load_once(vaddr);
    TEST_ASSERT("repeating the same cross-16B load with both address slots still armed reports breakpoint again and still keeps the second-half-start slot ignored on Spike",
        excpt.triggered == true &&
        excpt.cause == CAUSE_BKP
    );
    ai_micro_cross_16b_restore_machine_status(saved_mstatus);

    ai_micro_cross_16b_clear_mcontrol6_slot(0);
    saved_mstatus = ai_micro_cross_16b_enable_machine_breakpoint_delivery();

    goto_priv(PRIV_HS);
    TEST_SETUP_EXCEPT();
    value = ai_micro_cross_16b_execute_load_once(vaddr);
    TEST_ASSERT("after two prior breakpoint hits and then clearing only the original-address slot, the remaining second-half-start slot still stays ignored on Spike and the cross-16B load completes normally",
        excpt.triggered == false &&
        value == 0xeeddccbbaa998877ULL
    );
    ai_micro_cross_16b_restore_machine_status(saved_mstatus);

    ai_micro_cross_16b_clear_mcontrol6_slot(1);
    TEST_END();
}

bool ai_micro_hs_load_cross_16b_dual_address_trigger_page_fault_priority_spike_corner()
{
    TEST_START();

    uintptr_t vaddr = ai_micro_cross_16b_cacheable_addr(VSI_GI);
    uintptr_t second_half_vaddr = ai_micro_cross_16b_second_half_addr(VSI_GI);

    goto_priv(PRIV_HS);
    hspt_init();
    sfence_vma();

    ai_micro_cross_16b_clear_mcontrol6_slot(0);
    ai_micro_cross_16b_clear_mcontrol6_slot(1);
    ai_micro_cross_16b_arm_mcontrol6_slot(0, vaddr, AI_MICRO_CROSS_16B_MCONTROL6_LOAD);
    ai_micro_cross_16b_arm_mcontrol6_slot(1, second_half_vaddr, AI_MICRO_CROSS_16B_MCONTROL6_LOAD);
    uint64_t saved_mstatus = ai_micro_cross_16b_enable_machine_breakpoint_delivery();

    goto_priv(PRIV_HS);
    TEST_SETUP_EXCEPT();
    ai_micro_cross_16b_execute_load_once(vaddr);
    TEST_ASSERT("with both the original misaligned load address slot and the true second-half-start slot armed, Spike still reports breakpoint on the cross-16B load page-fault path because only the original address matches",
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
    ai_micro_cross_16b_execute_load_once(vaddr);
    TEST_ASSERT("after clearing the original misaligned-address slot, the remaining second-half-start slot stays ignored on Spike and the cross-16B load falls straight back to the underlying load page fault",
        excpt.triggered == true &&
        excpt.cause == CAUSE_LPF &&
        excpt.tval == vaddr
    );
    ai_micro_cross_16b_restore_machine_status(saved_mstatus);

    ai_micro_cross_16b_clear_mcontrol6_slot(1);
    TEST_END();
}

bool ai_micro_hs_load_cross_16b_dual_address_trigger_access_fault_priority_spike_corner()
{
    TEST_START();

    uintptr_t vaddr = ai_micro_cross_16b_cacheable_addr(VSRWX_GRWX);
    uintptr_t second_half_vaddr = ai_micro_cross_16b_second_half_addr(VSRWX_GRWX);

    goto_priv(PRIV_HS);
    hspt_init();

    goto_priv(PRIV_M);
    hspt_llptw_ppn_high_bit_overflow_setup();
    sfence_vma();

    ai_micro_cross_16b_clear_mcontrol6_slot(0);
    ai_micro_cross_16b_clear_mcontrol6_slot(1);
    ai_micro_cross_16b_arm_mcontrol6_slot(0, vaddr, AI_MICRO_CROSS_16B_MCONTROL6_LOAD);
    ai_micro_cross_16b_arm_mcontrol6_slot(1, second_half_vaddr, AI_MICRO_CROSS_16B_MCONTROL6_LOAD);
    uint64_t saved_mstatus = ai_micro_cross_16b_enable_machine_breakpoint_delivery();

    goto_priv(PRIV_HS);
    TEST_SETUP_EXCEPT();
    ai_micro_cross_16b_execute_load_once(vaddr);
    TEST_ASSERT("with both the original misaligned load address slot and the true second-half-start slot armed, Spike still reports breakpoint on the cross-16B load access-fault path because only the original address matches",
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
    ai_micro_cross_16b_execute_load_once(vaddr);
    TEST_ASSERT("after clearing the original misaligned-address slot, the remaining second-half-start slot stays ignored on Spike and the cross-16B load falls straight back to the underlying load access fault",
        excpt.triggered == true &&
        excpt.cause == CAUSE_LAF &&
        excpt.tval == vaddr
    );
    ai_micro_cross_16b_restore_machine_status(saved_mstatus);

    ai_micro_cross_16b_clear_mcontrol6_slot(1);

    goto_priv(PRIV_HS);
    hspt_init();
    sfence_vma();

    TEST_END();
}

bool ai_micro_hs_load_cross_16b_execute_and_address_trigger_both_breakpoint_corner()
{
    TEST_START();

    uintptr_t paddr = phys_page_base(VSRWX_GRWX);
    uintptr_t vaddr = ai_micro_cross_16b_cacheable_addr(VSRWX_GRWX);
    uintptr_t trigger_pc = (uintptr_t)ai_micro_cross_16b_execute_load_once;

    goto_priv(PRIV_M);
    write64(paddr + 8U, AI_MICRO_CROSS_16B_LOW_PATTERN);
    write64(paddr + 16U, AI_MICRO_CROSS_16B_HIGH_PATTERN);

    goto_priv(PRIV_HS);
    hspt_init();
    sfence_vma();

    ai_micro_cross_16b_clear_mcontrol6_slot(0);
    ai_micro_cross_16b_clear_mcontrol6_slot(1);
    ai_micro_cross_16b_arm_mcontrol6_slot(0, trigger_pc, AI_MICRO_CROSS_16B_MCONTROL6_EXECUTE);
    ai_micro_cross_16b_arm_mcontrol6_slot(1, vaddr, AI_MICRO_CROSS_16B_MCONTROL6_LOAD);
    uint64_t saved_mstatus = ai_micro_cross_16b_enable_machine_breakpoint_delivery();

    goto_priv(PRIV_HS);
    TEST_SETUP_EXCEPT();
    ai_micro_cross_16b_execute_load_once(vaddr);
    TEST_ASSERT("an HS cross-16B cacheable load still reports breakpoint when both execute-PC and address trigger slots match the same split load",
        excpt.triggered == true &&
        excpt.cause == CAUSE_BKP
    );
    ai_micro_cross_16b_restore_machine_status(saved_mstatus);

    ai_micro_cross_16b_clear_mcontrol6_slot(0);
    saved_mstatus = ai_micro_cross_16b_enable_machine_breakpoint_delivery();
    goto_priv(PRIV_HS);
    TEST_SETUP_EXCEPT();
    ai_micro_cross_16b_execute_load_once(vaddr);
    TEST_ASSERT("after clearing only the execute-PC slot, the remaining cross-16B load address trigger still reports breakpoint",
        excpt.triggered == true &&
        excpt.cause == CAUSE_BKP
    );
    ai_micro_cross_16b_restore_machine_status(saved_mstatus);

    ai_micro_cross_16b_clear_mcontrol6_slot(1);
    goto_priv(PRIV_HS);
    TEST_SETUP_EXCEPT();
    uint64_t value = ai_micro_cross_16b_execute_load_once(vaddr);
    TEST_ASSERT("after clearing both trigger slots, the same HS cross-16B cacheable load completes normally and still merges data correctly",
        excpt.triggered == false &&
        value == 0xeeddccbbaa998877ULL
    );

    TEST_END();
}

bool ai_micro_hs_load_cross_16b_execute_and_address_trigger_repeated_breakpoint_then_completion_corner()
{
    TEST_START();

    uintptr_t paddr = phys_page_base(VSRWX_GRWX);
    uintptr_t vaddr = ai_micro_cross_16b_cacheable_addr(VSRWX_GRWX);
    uintptr_t trigger_pc = (uintptr_t)ai_micro_cross_16b_execute_load_once;

    goto_priv(PRIV_M);
    write64(paddr + 8U, AI_MICRO_CROSS_16B_LOW_PATTERN);
    write64(paddr + 16U, AI_MICRO_CROSS_16B_HIGH_PATTERN);

    goto_priv(PRIV_HS);
    hspt_init();
    sfence_vma();

    ai_micro_cross_16b_clear_mcontrol6_slot(0);
    ai_micro_cross_16b_clear_mcontrol6_slot(1);
    ai_micro_cross_16b_arm_mcontrol6_slot(0, trigger_pc, AI_MICRO_CROSS_16B_MCONTROL6_EXECUTE);
    ai_micro_cross_16b_arm_mcontrol6_slot(1, vaddr, AI_MICRO_CROSS_16B_MCONTROL6_LOAD);
    uint64_t saved_mstatus = ai_micro_cross_16b_enable_machine_breakpoint_delivery();

    goto_priv(PRIV_HS);
    TEST_SETUP_EXCEPT();
    ai_micro_cross_16b_execute_load_once(vaddr);
    TEST_ASSERT("with both execute-PC and address trigger slots armed on the same cross-16B cacheable load, the first access reports breakpoint",
        excpt.triggered == true &&
        excpt.cause == CAUSE_BKP
    );
    ai_micro_cross_16b_restore_machine_status(saved_mstatus);

    saved_mstatus = ai_micro_cross_16b_enable_machine_breakpoint_delivery();
    goto_priv(PRIV_HS);
    TEST_SETUP_EXCEPT();
    ai_micro_cross_16b_execute_load_once(vaddr);
    TEST_ASSERT("with both execute-PC and address trigger slots still armed on that same cross-16B cacheable load, the second access still reports breakpoint",
        excpt.triggered == true &&
        excpt.cause == CAUSE_BKP
    );
    ai_micro_cross_16b_restore_machine_status(saved_mstatus);

    ai_micro_cross_16b_clear_mcontrol6_slot(0);
    saved_mstatus = ai_micro_cross_16b_enable_machine_breakpoint_delivery();
    goto_priv(PRIV_HS);
    TEST_SETUP_EXCEPT();
    ai_micro_cross_16b_execute_load_once(vaddr);
    TEST_ASSERT("after two prior dual-slot breakpoint hits, clearing only the execute-PC slot still leaves the cross-16B load address trigger active",
        excpt.triggered == true &&
        excpt.cause == CAUSE_BKP
    );
    ai_micro_cross_16b_restore_machine_status(saved_mstatus);

    ai_micro_cross_16b_clear_mcontrol6_slot(1);
    goto_priv(PRIV_HS);
    TEST_SETUP_EXCEPT();
    uint64_t value = ai_micro_cross_16b_execute_load_once(vaddr);
    TEST_ASSERT("after clearing both trigger slots, the same cross-16B cacheable load completes normally and still merges data correctly",
        excpt.triggered == false &&
        value == 0xeeddccbbaa998877ULL
    );

    TEST_END();
}

bool ai_micro_hs_load_cross_16b_execute_and_address_trigger_repeated_breakpoint_then_completion_adjacent_preserved_corner()
{
    TEST_START();

    uintptr_t paddr = phys_page_base(VSRWX_GRWX);
    uintptr_t vaddr = ai_micro_cross_16b_cacheable_addr(VSRWX_GRWX);
    uintptr_t leading_phys = paddr;
    uintptr_t low_phys = paddr + 8U;
    uintptr_t high_phys = paddr + 16U;
    uintptr_t trailing_phys = paddr + 24U;
    uintptr_t trigger_pc = (uintptr_t)ai_micro_cross_16b_execute_load_once;
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
    ai_micro_cross_16b_arm_mcontrol6_slot(1, vaddr, AI_MICRO_CROSS_16B_MCONTROL6_LOAD);
    saved_mstatus = ai_micro_cross_16b_enable_machine_breakpoint_delivery();

    goto_priv(PRIV_HS);
    TEST_SETUP_EXCEPT();
    ai_micro_cross_16b_execute_load_once(vaddr);
    TEST_ASSERT("with both execute-PC and address trigger slots armed on the same cross-16B cacheable load, the first access still reports breakpoint in the adjacent-preservation corner",
        excpt.triggered == true &&
        excpt.cause == CAUSE_BKP
    );

    goto_priv(PRIV_M);
    TEST_ASSERT("the first trapped cross-16B cacheable load still leaves the two touched aligned doublewords and their immediate neighbors unchanged",
        read64(leading_phys) == leading_pattern &&
        read64(low_phys) == low_pattern &&
        read64(high_phys) == high_pattern &&
        read64(trailing_phys) == trailing_pattern
    );
    ai_micro_cross_16b_restore_machine_status(saved_mstatus);

    saved_mstatus = ai_micro_cross_16b_enable_machine_breakpoint_delivery();
    goto_priv(PRIV_HS);
    TEST_SETUP_EXCEPT();
    ai_micro_cross_16b_execute_load_once(vaddr);
    TEST_ASSERT("with both execute-PC and address trigger slots still armed on that same cross-16B cacheable load, the second access still reports breakpoint in the adjacent-preservation corner",
        excpt.triggered == true &&
        excpt.cause == CAUSE_BKP
    );

    goto_priv(PRIV_M);
    TEST_ASSERT("the second trapped cross-16B cacheable load still leaves the two touched aligned doublewords and their immediate neighbors unchanged",
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
    ai_micro_cross_16b_execute_load_once(vaddr);
    TEST_ASSERT("after two prior dual-slot breakpoint hits, clearing only the execute-PC slot still leaves the cross-16B load address trigger active in the adjacent-preservation corner",
        excpt.triggered == true &&
        excpt.cause == CAUSE_BKP
    );

    goto_priv(PRIV_M);
    TEST_ASSERT("with only the address trigger slot armed, the trapped cross-16B cacheable load still leaves the two touched aligned doublewords and their immediate neighbors unchanged",
        read64(leading_phys) == leading_pattern &&
        read64(low_phys) == low_pattern &&
        read64(high_phys) == high_pattern &&
        read64(trailing_phys) == trailing_pattern
    );
    ai_micro_cross_16b_restore_machine_status(saved_mstatus);

    ai_micro_cross_16b_clear_mcontrol6_slot(1);
    goto_priv(PRIV_HS);
    TEST_SETUP_EXCEPT();
    uint64_t value = ai_micro_cross_16b_execute_load_once(vaddr);
    TEST_ASSERT("after clearing both trigger slots, the same cross-16B cacheable load completes normally and still merges data correctly in the adjacent-preservation corner",
        excpt.triggered == false &&
        value == 0xeeddccbbaa998877ULL
    );

    goto_priv(PRIV_M);
    TEST_ASSERT("after the recovered cross-16B cacheable load path, the two touched aligned doublewords and their immediate neighbors remain unchanged",
        read64(leading_phys) == leading_pattern &&
        read64(low_phys) == low_pattern &&
        read64(high_phys) == high_pattern &&
        read64(trailing_phys) == trailing_pattern
    );

    TEST_END();
}

bool ai_micro_hs_load_cross_16b_execute_trigger_with_second_half_address_slot_ignored_spike_corner()
{
    TEST_START();

    uintptr_t paddr = phys_page_base(VSRWX_GRWX);
    uintptr_t vaddr = ai_micro_cross_16b_cacheable_addr(VSRWX_GRWX);
    uintptr_t second_half_vaddr = ai_micro_cross_16b_second_half_addr(VSRWX_GRWX);
    uintptr_t trigger_pc = (uintptr_t)ai_micro_cross_16b_execute_load_once;

    goto_priv(PRIV_M);
    write64(paddr + 8U, AI_MICRO_CROSS_16B_LOW_PATTERN);
    write64(paddr + 16U, AI_MICRO_CROSS_16B_HIGH_PATTERN);

    goto_priv(PRIV_HS);
    hspt_init();
    sfence_vma();

    ai_micro_cross_16b_clear_mcontrol6_slot(0);
    ai_micro_cross_16b_clear_mcontrol6_slot(1);
    ai_micro_cross_16b_arm_mcontrol6_slot(0, trigger_pc, AI_MICRO_CROSS_16B_MCONTROL6_EXECUTE);
    ai_micro_cross_16b_arm_mcontrol6_slot(1, second_half_vaddr, AI_MICRO_CROSS_16B_MCONTROL6_LOAD);
    uint64_t saved_mstatus = ai_micro_cross_16b_enable_machine_breakpoint_delivery();

    goto_priv(PRIV_HS);
    TEST_SETUP_EXCEPT();
    ai_micro_cross_16b_execute_load_once(vaddr);
    TEST_ASSERT("with execute-PC trigger armed and the address slot programmed to the true second-half start address, Spike still reports breakpoint first on the cross-16B split load",
        excpt.triggered == true &&
        excpt.cause == CAUSE_BKP
    );
    ai_micro_cross_16b_restore_machine_status(saved_mstatus);

    ai_micro_cross_16b_clear_mcontrol6_slot(0);
    saved_mstatus = ai_micro_cross_16b_enable_machine_breakpoint_delivery();

    goto_priv(PRIV_HS);
    TEST_SETUP_EXCEPT();
    uint64_t value = ai_micro_cross_16b_execute_load_once(vaddr);
    TEST_ASSERT("after clearing the execute-PC slot, the second-half-start address slot does not keep matching on Spike; the cross-16B load completes normally",
        excpt.triggered == false &&
        value == 0xeeddccbbaa998877ULL
    );
    ai_micro_cross_16b_restore_machine_status(saved_mstatus);

    ai_micro_cross_16b_clear_mcontrol6_slot(1);
    TEST_END();
}

bool ai_micro_hs_load_cross_16b_execute_trigger_with_second_half_address_slot_repeated_breakpoint_then_completion_spike_corner()
{
    TEST_START();

    uintptr_t paddr = phys_page_base(VSRWX_GRWX);
    uintptr_t vaddr = ai_micro_cross_16b_cacheable_addr(VSRWX_GRWX);
    uintptr_t second_half_vaddr = ai_micro_cross_16b_second_half_addr(VSRWX_GRWX);
    uintptr_t trigger_pc = (uintptr_t)ai_micro_cross_16b_execute_load_once;

    goto_priv(PRIV_M);
    write64(paddr + 8U, AI_MICRO_CROSS_16B_LOW_PATTERN);
    write64(paddr + 16U, AI_MICRO_CROSS_16B_HIGH_PATTERN);

    goto_priv(PRIV_HS);
    hspt_init();
    sfence_vma();

    ai_micro_cross_16b_clear_mcontrol6_slot(0);
    ai_micro_cross_16b_clear_mcontrol6_slot(1);
    ai_micro_cross_16b_arm_mcontrol6_slot(0, trigger_pc, AI_MICRO_CROSS_16B_MCONTROL6_EXECUTE);
    ai_micro_cross_16b_arm_mcontrol6_slot(1, second_half_vaddr, AI_MICRO_CROSS_16B_MCONTROL6_LOAD);
    uint64_t saved_mstatus = ai_micro_cross_16b_enable_machine_breakpoint_delivery();

    goto_priv(PRIV_HS);
    TEST_SETUP_EXCEPT();
    ai_micro_cross_16b_execute_load_once(vaddr);
    TEST_ASSERT("with execute-PC trigger armed and the address slot programmed to the true second-half start address, the first cross-16B split-load attempt still reports breakpoint on Spike",
        excpt.triggered == true &&
        excpt.cause == CAUSE_BKP
    );
    ai_micro_cross_16b_restore_machine_status(saved_mstatus);

    saved_mstatus = ai_micro_cross_16b_enable_machine_breakpoint_delivery();
    goto_priv(PRIV_HS);
    TEST_SETUP_EXCEPT();
    ai_micro_cross_16b_execute_load_once(vaddr);
    TEST_ASSERT("with the same execute-PC and second-half-start slots still armed, the second cross-16B split-load attempt still reports breakpoint on Spike",
        excpt.triggered == true &&
        excpt.cause == CAUSE_BKP
    );
    ai_micro_cross_16b_restore_machine_status(saved_mstatus);

    ai_micro_cross_16b_clear_mcontrol6_slot(0);
    saved_mstatus = ai_micro_cross_16b_enable_machine_breakpoint_delivery();

    goto_priv(PRIV_HS);
    TEST_SETUP_EXCEPT();
    uint64_t value = ai_micro_cross_16b_execute_load_once(vaddr);
    TEST_ASSERT("after two prior execute-trigger breakpoint hits, clearing only the execute-PC slot still leaves the second-half-start address slot ignored on Spike, so the cross-16B split load completes normally",
        excpt.triggered == false &&
        value == 0xeeddccbbaa998877ULL
    );
    ai_micro_cross_16b_restore_machine_status(saved_mstatus);

    ai_micro_cross_16b_clear_mcontrol6_slot(1);
    TEST_END();
}

bool ai_micro_hs_load_cross_16b_execute_trigger_with_second_half_address_slot_repeated_breakpoint_then_completion_adjacent_preserved_spike_corner()
{
    TEST_START();

    uintptr_t paddr = phys_page_base(VSRWX_GRWX);
    uintptr_t vaddr = ai_micro_cross_16b_cacheable_addr(VSRWX_GRWX);
    uintptr_t second_half_vaddr = ai_micro_cross_16b_second_half_addr(VSRWX_GRWX);
    uintptr_t leading_phys = paddr;
    uintptr_t low_phys = paddr + 8U;
    uintptr_t high_phys = paddr + 16U;
    uintptr_t trailing_phys = paddr + 24U;
    uintptr_t trigger_pc = (uintptr_t)ai_micro_cross_16b_execute_load_once;
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
    ai_micro_cross_16b_arm_mcontrol6_slot(1, second_half_vaddr, AI_MICRO_CROSS_16B_MCONTROL6_LOAD);
    saved_mstatus = ai_micro_cross_16b_enable_machine_breakpoint_delivery();

    goto_priv(PRIV_HS);
    TEST_SETUP_EXCEPT();
    ai_micro_cross_16b_execute_load_once(vaddr);
    TEST_ASSERT("with execute-PC trigger armed and the address slot programmed to the true second-half start address, the first cross-16B split-load attempt still reports breakpoint on Spike in the adjacent-preservation corner",
        excpt.triggered == true &&
        excpt.cause == CAUSE_BKP
    );

    goto_priv(PRIV_M);
    TEST_ASSERT("the first trapped cross-16B split-load attempt still leaves the two touched aligned doublewords and their immediate neighbors unchanged while only the execute-PC slot is effective on Spike",
        read64(leading_phys) == leading_pattern &&
        read64(low_phys) == low_pattern &&
        read64(high_phys) == high_pattern &&
        read64(trailing_phys) == trailing_pattern
    );
    ai_micro_cross_16b_restore_machine_status(saved_mstatus);

    saved_mstatus = ai_micro_cross_16b_enable_machine_breakpoint_delivery();
    goto_priv(PRIV_HS);
    TEST_SETUP_EXCEPT();
    ai_micro_cross_16b_execute_load_once(vaddr);
    TEST_ASSERT("with the same execute-PC and second-half-start slots still armed, the second cross-16B split-load attempt still reports breakpoint on Spike in the adjacent-preservation corner",
        excpt.triggered == true &&
        excpt.cause == CAUSE_BKP
    );

    goto_priv(PRIV_M);
    TEST_ASSERT("the second trapped cross-16B split-load attempt still leaves the two touched aligned doublewords and their immediate neighbors unchanged while only the execute-PC slot is effective on Spike",
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
    uint64_t value = ai_micro_cross_16b_execute_load_once(vaddr);
    TEST_ASSERT("after two prior execute-trigger breakpoint hits, clearing only the execute-PC slot still leaves the second-half-start address slot ignored on Spike, so the cross-16B split load completes normally in the adjacent-preservation corner",
        excpt.triggered == false &&
        value == 0xeeddccbbaa998877ULL
    );
    ai_micro_cross_16b_restore_machine_status(saved_mstatus);

    goto_priv(PRIV_M);
    TEST_ASSERT("after the repeated execute-trigger probes, the recovered cross-16B split load still leaves the two touched aligned doublewords and their immediate neighbors unchanged",
        read64(leading_phys) == leading_pattern &&
        read64(low_phys) == low_pattern &&
        read64(high_phys) == high_pattern &&
        read64(trailing_phys) == trailing_pattern
    );

    ai_micro_cross_16b_clear_mcontrol6_slot(1);
    TEST_END();
}

bool ai_micro_hs_load_cross_16b_execute_trigger_with_second_half_address_slot_page_fault_spike_corner()
{
    TEST_START();

    uintptr_t vaddr = ai_micro_cross_16b_cacheable_addr(VSI_GI);
    uintptr_t second_half_vaddr = ai_micro_cross_16b_second_half_addr(VSI_GI);
    uintptr_t trigger_pc = (uintptr_t)ai_micro_cross_16b_execute_load_once;

    goto_priv(PRIV_HS);
    hspt_init();
    sfence_vma();

    ai_micro_cross_16b_clear_mcontrol6_slot(0);
    ai_micro_cross_16b_clear_mcontrol6_slot(1);
    ai_micro_cross_16b_arm_mcontrol6_slot(0, trigger_pc, AI_MICRO_CROSS_16B_MCONTROL6_EXECUTE);
    ai_micro_cross_16b_arm_mcontrol6_slot(1, second_half_vaddr, AI_MICRO_CROSS_16B_MCONTROL6_LOAD);
    uint64_t saved_mstatus = ai_micro_cross_16b_enable_machine_breakpoint_delivery();

    goto_priv(PRIV_HS);
    TEST_SETUP_EXCEPT();
    ai_micro_cross_16b_execute_load_once(vaddr);
    TEST_ASSERT("with execute-PC trigger armed and the address slot programmed to the true second-half start address, Spike still reports breakpoint first on the cross-16B load page-fault path",
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
    ai_micro_cross_16b_execute_load_once(vaddr);
    TEST_ASSERT("after clearing the execute-PC slot, the second-half-start address slot does not keep matching on Spike; the cross-16B load falls straight back to the underlying load page fault",
        excpt.triggered == true &&
        excpt.cause == CAUSE_LPF &&
        excpt.tval == vaddr
    );
    ai_micro_cross_16b_restore_machine_status(saved_mstatus);

    ai_micro_cross_16b_clear_mcontrol6_slot(1);
    TEST_END();
}

bool ai_micro_hs_load_cross_16b_execute_trigger_with_second_half_address_slot_page_fault_repeated_breakpoint_then_fault_spike_corner()
{
    TEST_START();

    uintptr_t vaddr = ai_micro_cross_16b_cacheable_addr(VSI_GI);
    uintptr_t second_half_vaddr = ai_micro_cross_16b_second_half_addr(VSI_GI);
    uintptr_t trigger_pc = (uintptr_t)ai_micro_cross_16b_execute_load_once;

    goto_priv(PRIV_HS);
    hspt_init();
    sfence_vma();

    ai_micro_cross_16b_clear_mcontrol6_slot(0);
    ai_micro_cross_16b_clear_mcontrol6_slot(1);
    ai_micro_cross_16b_arm_mcontrol6_slot(0, trigger_pc, AI_MICRO_CROSS_16B_MCONTROL6_EXECUTE);
    ai_micro_cross_16b_arm_mcontrol6_slot(1, second_half_vaddr, AI_MICRO_CROSS_16B_MCONTROL6_LOAD);
    uint64_t saved_mstatus = ai_micro_cross_16b_enable_machine_breakpoint_delivery();

    goto_priv(PRIV_HS);
    TEST_SETUP_EXCEPT();
    ai_micro_cross_16b_execute_load_once(vaddr);
    TEST_ASSERT("with execute-PC trigger armed and the address slot programmed to the true second-half start address, the first cross-16B load page-fault attempt still reports breakpoint on Spike",
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
    ai_micro_cross_16b_execute_load_once(vaddr);
    TEST_ASSERT("with the same execute-PC and second-half-start slots still armed, the second cross-16B load page-fault attempt still reports breakpoint on Spike",
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
    ai_micro_cross_16b_execute_load_once(vaddr);
    TEST_ASSERT("after two prior execute-trigger breakpoint hits, clearing only the execute-PC slot still leaves the second-half-start address slot ignored on Spike, so the cross-16B load falls back to load page fault",
        excpt.triggered == true &&
        excpt.cause == CAUSE_LPF &&
        excpt.tval == vaddr
    );
    ai_micro_cross_16b_restore_machine_status(saved_mstatus);

    ai_micro_cross_16b_clear_mcontrol6_slot(1);
    TEST_END();
}

bool ai_micro_hs_load_cross_16b_execute_trigger_with_second_half_address_slot_page_fault_repeated_breakpoint_then_fault_adjacent_preserved_spike_corner()
{
    TEST_START();

    uintptr_t paddr = phys_page_base(VSI_GI);
    uintptr_t vaddr = ai_micro_cross_16b_cacheable_addr(VSI_GI);
    uintptr_t second_half_vaddr = ai_micro_cross_16b_second_half_addr(VSI_GI);
    uintptr_t leading_phys = paddr;
    uintptr_t low_phys = paddr + 8U;
    uintptr_t high_phys = paddr + 16U;
    uintptr_t trailing_phys = paddr + 24U;
    uintptr_t trigger_pc = (uintptr_t)ai_micro_cross_16b_execute_load_once;
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
    ai_micro_cross_16b_arm_mcontrol6_slot(1, second_half_vaddr, AI_MICRO_CROSS_16B_MCONTROL6_LOAD);
    saved_mstatus = ai_micro_cross_16b_enable_machine_breakpoint_delivery();

    goto_priv(PRIV_HS);
    TEST_SETUP_EXCEPT();
    ai_micro_cross_16b_execute_load_once(vaddr);
    TEST_ASSERT("with execute-PC trigger armed and the address slot programmed to the true second-half start address, the first cross-16B load page-fault attempt still reports breakpoint on Spike in the adjacent-preservation corner",
        excpt.triggered == true &&
        excpt.cause == CAUSE_BKP
    );

    goto_priv(PRIV_M);
    TEST_ASSERT("the first trapped cross-16B load page-fault attempt still leaves the two touched aligned doublewords and their immediate neighbors unchanged while only the execute-PC slot is effective on Spike",
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
    ai_micro_cross_16b_execute_load_once(vaddr);
    TEST_ASSERT("with the same execute-PC and second-half-start slots still armed, the second cross-16B load page-fault attempt still reports breakpoint on Spike in the adjacent-preservation corner",
        excpt.triggered == true &&
        excpt.cause == CAUSE_BKP
    );

    goto_priv(PRIV_M);
    TEST_ASSERT("the second trapped cross-16B load page-fault attempt still leaves the two touched aligned doublewords and their immediate neighbors unchanged while only the execute-PC slot is effective on Spike",
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
    ai_micro_cross_16b_execute_load_once(vaddr);
    TEST_ASSERT("after two prior execute-trigger breakpoint hits, clearing only the execute-PC slot still leaves the second-half-start address slot ignored on Spike, so the cross-16B load falls back to load page fault in the adjacent-preservation corner",
        excpt.triggered == true &&
        excpt.cause == CAUSE_LPF &&
        excpt.tval == vaddr
    );

    goto_priv(PRIV_M);
    TEST_ASSERT("after the fallback load page fault, the same cross-16B load probe still leaves the two touched aligned doublewords and their immediate neighbors unchanged",
        read64(leading_phys) == leading_pattern &&
        read64(low_phys) == low_pattern &&
        read64(high_phys) == high_pattern &&
        read64(trailing_phys) == trailing_pattern
    );
    ai_micro_cross_16b_restore_machine_status(saved_mstatus);

    ai_micro_cross_16b_clear_mcontrol6_slot(1);
    TEST_END();
}

bool ai_micro_hs_load_cross_16b_execute_and_address_trigger_page_fault_priority_corner()
{
    TEST_START();

    uintptr_t vaddr = ai_micro_cross_16b_cacheable_addr(VSI_GI);
    uintptr_t trigger_pc = (uintptr_t)ai_micro_cross_16b_execute_load_once;

    goto_priv(PRIV_HS);
    hspt_init();
    sfence_vma();

    ai_micro_cross_16b_clear_mcontrol6_slot(0);
    ai_micro_cross_16b_clear_mcontrol6_slot(1);
    ai_micro_cross_16b_arm_mcontrol6_slot(0, trigger_pc, AI_MICRO_CROSS_16B_MCONTROL6_EXECUTE);
    ai_micro_cross_16b_arm_mcontrol6_slot(1, vaddr, AI_MICRO_CROSS_16B_MCONTROL6_LOAD);
    uint64_t saved_mstatus = ai_micro_cross_16b_enable_machine_breakpoint_delivery();

    goto_priv(PRIV_HS);
    TEST_SETUP_EXCEPT();
    ai_micro_cross_16b_execute_load_once(vaddr);
    TEST_ASSERT("an HS cross-16B load still reports breakpoint when both execute-PC and address trigger slots match a split load that would otherwise raise page fault",
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
    ai_micro_cross_16b_execute_load_once(vaddr);
    TEST_ASSERT("after clearing only the execute-PC slot, the remaining cross-16B load address trigger still reports breakpoint on the same page-fault path",
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
    ai_micro_cross_16b_execute_load_once(vaddr);
    TEST_ASSERT("after clearing both trigger slots, the same HS cross-16B load reports load page fault with the original misaligned VA",
        excpt.triggered == true &&
        excpt.cause == CAUSE_LPF &&
        excpt.tval == vaddr
    );

    TEST_END();
}

bool ai_micro_hs_load_cross_16b_execute_and_address_trigger_page_fault_repeated_breakpoint_then_fault_corner()
{
    TEST_START();

    uintptr_t vaddr = ai_micro_cross_16b_cacheable_addr(VSI_GI);
    uintptr_t trigger_pc = (uintptr_t)ai_micro_cross_16b_execute_load_once;

    goto_priv(PRIV_HS);
    hspt_init();
    sfence_vma();

    ai_micro_cross_16b_clear_mcontrol6_slot(0);
    ai_micro_cross_16b_clear_mcontrol6_slot(1);
    ai_micro_cross_16b_arm_mcontrol6_slot(0, trigger_pc, AI_MICRO_CROSS_16B_MCONTROL6_EXECUTE);
    ai_micro_cross_16b_arm_mcontrol6_slot(1, vaddr, AI_MICRO_CROSS_16B_MCONTROL6_LOAD);
    uint64_t saved_mstatus = ai_micro_cross_16b_enable_machine_breakpoint_delivery();

    goto_priv(PRIV_HS);
    TEST_SETUP_EXCEPT();
    ai_micro_cross_16b_execute_load_once(vaddr);
    TEST_ASSERT("with both execute-PC and address trigger slots armed on the cross-16B load page-fault path, the first access reports breakpoint",
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
    ai_micro_cross_16b_execute_load_once(vaddr);
    TEST_ASSERT("with both execute-PC and address trigger slots still armed on the same cross-16B load page-fault path, the second access still reports breakpoint",
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
    ai_micro_cross_16b_execute_load_once(vaddr);
    TEST_ASSERT("after two prior dual-slot breakpoint hits, clearing only the execute-PC slot still leaves the cross-16B load address trigger active on the same page-fault path",
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
    ai_micro_cross_16b_execute_load_once(vaddr);
    TEST_ASSERT("after clearing both trigger slots, the same cross-16B load falls back to load page fault with the original misaligned VA",
        excpt.triggered == true &&
        excpt.cause == CAUSE_LPF &&
        excpt.tval == vaddr
    );

    TEST_END();
}

bool ai_micro_hs_load_cross_16b_execute_and_address_trigger_page_fault_repeated_breakpoint_then_fault_adjacent_preserved_corner()
{
    TEST_START();

    uintptr_t paddr = phys_page_base(VSI_GI);
    uintptr_t vaddr = ai_micro_cross_16b_cacheable_addr(VSI_GI);
    uintptr_t leading_phys = paddr;
    uintptr_t low_phys = paddr + 8U;
    uintptr_t high_phys = paddr + 16U;
    uintptr_t trailing_phys = paddr + 24U;
    uintptr_t trigger_pc = (uintptr_t)ai_micro_cross_16b_execute_load_once;
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
    ai_micro_cross_16b_arm_mcontrol6_slot(1, vaddr, AI_MICRO_CROSS_16B_MCONTROL6_LOAD);
    saved_mstatus = ai_micro_cross_16b_enable_machine_breakpoint_delivery();

    goto_priv(PRIV_HS);
    TEST_SETUP_EXCEPT();
    ai_micro_cross_16b_execute_load_once(vaddr);
    TEST_ASSERT("with both execute-PC and address trigger slots armed on the cross-16B load page-fault path, the first access still reports breakpoint in the adjacent-preservation corner",
        excpt.triggered == true &&
        excpt.cause == CAUSE_BKP
    );
    ai_micro_cross_16b_restore_machine_status(saved_mstatus);

    goto_priv(PRIV_M);
    TEST_ASSERT("the first trapped cross-16B load page-fault probe still leaves the two touched aligned doublewords and their immediate neighbors unchanged",
        read64(leading_phys) == leading_pattern &&
        read64(low_phys) == low_pattern &&
        read64(high_phys) == high_pattern &&
        read64(trailing_phys) == trailing_pattern
    );

    goto_priv(PRIV_HS);
    hspt_init();
    sfence_vma();

    saved_mstatus = ai_micro_cross_16b_enable_machine_breakpoint_delivery();
    goto_priv(PRIV_HS);
    TEST_SETUP_EXCEPT();
    ai_micro_cross_16b_execute_load_once(vaddr);
    TEST_ASSERT("with both execute-PC and address trigger slots still armed on the same cross-16B load page-fault path, the second access still reports breakpoint in the adjacent-preservation corner",
        excpt.triggered == true &&
        excpt.cause == CAUSE_BKP
    );
    ai_micro_cross_16b_restore_machine_status(saved_mstatus);

    goto_priv(PRIV_M);
    TEST_ASSERT("the second trapped cross-16B load page-fault probe still leaves the two touched aligned doublewords and their immediate neighbors unchanged",
        read64(leading_phys) == leading_pattern &&
        read64(low_phys) == low_pattern &&
        read64(high_phys) == high_pattern &&
        read64(trailing_phys) == trailing_pattern
    );

    goto_priv(PRIV_HS);
    hspt_init();
    sfence_vma();

    ai_micro_cross_16b_clear_mcontrol6_slot(0);
    saved_mstatus = ai_micro_cross_16b_enable_machine_breakpoint_delivery();
    goto_priv(PRIV_HS);
    TEST_SETUP_EXCEPT();
    ai_micro_cross_16b_execute_load_once(vaddr);
    TEST_ASSERT("after two prior dual-slot breakpoint hits, clearing only the execute-PC slot still leaves the cross-16B load address trigger active on the same page-fault path in the adjacent-preservation corner",
        excpt.triggered == true &&
        excpt.cause == CAUSE_BKP
    );
    ai_micro_cross_16b_restore_machine_status(saved_mstatus);

    goto_priv(PRIV_M);
    TEST_ASSERT("with only the address trigger slot armed, the trapped cross-16B load page-fault probe still leaves the two touched aligned doublewords and their immediate neighbors unchanged",
        read64(leading_phys) == leading_pattern &&
        read64(low_phys) == low_pattern &&
        read64(high_phys) == high_pattern &&
        read64(trailing_phys) == trailing_pattern
    );

    goto_priv(PRIV_HS);
    hspt_init();
    sfence_vma();

    ai_micro_cross_16b_clear_mcontrol6_slot(1);
    goto_priv(PRIV_HS);
    TEST_SETUP_EXCEPT();
    ai_micro_cross_16b_execute_load_once(vaddr);
    TEST_ASSERT("after clearing both trigger slots, the same cross-16B load falls back to load page fault with the original misaligned VA in the adjacent-preservation corner",
        excpt.triggered == true &&
        excpt.cause == CAUSE_LPF &&
        excpt.tval == vaddr
    );

    goto_priv(PRIV_M);
    TEST_ASSERT("the fallback cross-16B load page-fault path still leaves the two touched aligned doublewords and their immediate neighbors unchanged",
        read64(leading_phys) == leading_pattern &&
        read64(low_phys) == low_pattern &&
        read64(high_phys) == high_pattern &&
        read64(trailing_phys) == trailing_pattern
    );

    TEST_END();
}

bool ai_micro_hs_load_cross_16b_execute_and_address_trigger_page_fault_repeated_breakpoint_then_valid_cross_16b_load_recovers_corner()
{
    TEST_START();

    uintptr_t fault_vaddr = ai_micro_cross_16b_cacheable_addr(VSI_GI);
    uintptr_t ok_vaddr = ai_micro_cross_16b_cacheable_addr(VSRWX_GRWX);
    uintptr_t ok_phys = phys_page_base(VSRWX_GRWX);
    uintptr_t trigger_pc = (uintptr_t)ai_micro_cross_16b_execute_load_once;
    uint64_t saved_mstatus;
    uint64_t value = 0;

    goto_priv(PRIV_M);
    write64(ok_phys + 8U, AI_MICRO_CROSS_16B_LOW_PATTERN);
    write64(ok_phys + 16U, AI_MICRO_CROSS_16B_HIGH_PATTERN);

    goto_priv(PRIV_HS);
    hspt_init();
    sfence_vma();

    ai_micro_cross_16b_clear_mcontrol6_slot(0);
    ai_micro_cross_16b_clear_mcontrol6_slot(1);
    ai_micro_cross_16b_arm_mcontrol6_slot(0, trigger_pc, AI_MICRO_CROSS_16B_MCONTROL6_EXECUTE);
    ai_micro_cross_16b_arm_mcontrol6_slot(1, fault_vaddr, AI_MICRO_CROSS_16B_MCONTROL6_LOAD);
    saved_mstatus = ai_micro_cross_16b_enable_machine_breakpoint_delivery();

    goto_priv(PRIV_HS);
    TEST_SETUP_EXCEPT();
    ai_micro_cross_16b_execute_load_once(fault_vaddr);
    TEST_ASSERT("with both execute-PC and address trigger slots armed on the cross-16B load page-fault path, the first access still reports breakpoint before recovery",
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
    ai_micro_cross_16b_execute_load_once(fault_vaddr);
    TEST_ASSERT("with both trigger slots still armed on the same cross-16B load page-fault path, the second access still reports breakpoint before recovery",
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
    ai_micro_cross_16b_execute_load_once(fault_vaddr);
    TEST_ASSERT("after two prior dual-slot breakpoint hits, clearing only the execute-PC slot still leaves the cross-16B load address trigger active on the same page-fault path before recovery",
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
    ai_micro_cross_16b_execute_load_once(fault_vaddr);
    TEST_ASSERT("after clearing both trigger slots, the same cross-16B load first falls back to load page fault with the original misaligned VA before recovery",
        excpt.triggered == true &&
        excpt.cause == CAUSE_LPF &&
        excpt.tval == fault_vaddr
    );

    TEST_SETUP_EXCEPT();
    value = ld(ok_vaddr);
    TEST_ASSERT("after repeated trigger hits and the fallback page fault, a following valid cacheable cross-16B load still succeeds immediately with the expected merged payload",
        excpt.triggered == false &&
        value == 0xeeddccbbaa998877ULL
    );

    TEST_END();
}

bool ai_micro_hs_load_cross_16b_execute_and_address_trigger_page_fault_repeated_breakpoint_then_valid_cross_16b_load_adjacent_preserved_corner()
{
    TEST_START();

    uintptr_t fault_vaddr = ai_micro_cross_16b_cacheable_addr(VSI_GI);
    uintptr_t ok_vaddr = ai_micro_cross_16b_cacheable_addr(VSRWX_GRWX);
    uintptr_t ok_phys = phys_page_base(VSRWX_GRWX);
    uintptr_t leading_phys = ok_phys;
    uintptr_t low_phys = ok_phys + 8U;
    uintptr_t high_phys = ok_phys + 16U;
    uintptr_t trailing_phys = ok_phys + 24U;
    uintptr_t trigger_pc = (uintptr_t)ai_micro_cross_16b_execute_load_once;
    const uint64_t leading_pattern = 0x1122334455667788ULL;
    const uint64_t low_pattern = AI_MICRO_CROSS_16B_LOW_PATTERN;
    const uint64_t high_pattern = AI_MICRO_CROSS_16B_HIGH_PATTERN;
    const uint64_t trailing_pattern = 0x8877665544332211ULL;
    uint64_t saved_mstatus;
    uint64_t value = 0;

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
    ai_micro_cross_16b_arm_mcontrol6_slot(1, fault_vaddr, AI_MICRO_CROSS_16B_MCONTROL6_LOAD);
    saved_mstatus = ai_micro_cross_16b_enable_machine_breakpoint_delivery();

    goto_priv(PRIV_HS);
    TEST_SETUP_EXCEPT();
    ai_micro_cross_16b_execute_load_once(fault_vaddr);
    TEST_ASSERT("with both execute-PC and address trigger slots armed on the cross-16B load page-fault path, the first access still reports breakpoint before recovery in the adjacent-preservation corner",
        excpt.triggered == true &&
        excpt.cause == CAUSE_BKP
    );

    goto_priv(PRIV_M);
    TEST_ASSERT("the first trapped trigger-plus-page-fault probe still leaves the future recovery target doublewords and their adjacent doublewords unchanged",
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
    ai_micro_cross_16b_execute_load_once(fault_vaddr);
    TEST_ASSERT("with both trigger slots still armed on the same cross-16B load page-fault path, the second access still reports breakpoint before recovery in the adjacent-preservation corner",
        excpt.triggered == true &&
        excpt.cause == CAUSE_BKP
    );

    goto_priv(PRIV_M);
    TEST_ASSERT("the second trapped trigger-plus-page-fault probe still leaves the future recovery target doublewords and their adjacent doublewords unchanged",
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
    ai_micro_cross_16b_execute_load_once(fault_vaddr);
    TEST_ASSERT("after two prior dual-slot breakpoint hits, clearing only the execute-PC slot still leaves the cross-16B load address trigger active on the same page-fault path before recovery in the adjacent-preservation corner",
        excpt.triggered == true &&
        excpt.cause == CAUSE_BKP
    );

    goto_priv(PRIV_M);
    TEST_ASSERT("with only the address trigger slot armed, the trapped trigger-plus-page-fault probe still leaves the future recovery target doublewords and their adjacent doublewords unchanged",
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
    ai_micro_cross_16b_execute_load_once(fault_vaddr);
    TEST_ASSERT("after clearing both trigger slots, the same cross-16B load first falls back to load page fault with the original misaligned VA before recovery in the adjacent-preservation corner",
        excpt.triggered == true &&
        excpt.cause == CAUSE_LPF &&
        excpt.tval == fault_vaddr
    );

    goto_priv(PRIV_M);
    TEST_ASSERT("the fallback cross-16B load page-fault path still leaves the future recovery target doublewords and their adjacent doublewords unchanged",
        read64(leading_phys) == leading_pattern &&
        read64(low_phys) == low_pattern &&
        read64(high_phys) == high_pattern &&
        read64(trailing_phys) == trailing_pattern
    );

    goto_priv(PRIV_HS);
    TEST_SETUP_EXCEPT();
    value = ld(ok_vaddr);
    TEST_ASSERT("after repeated trigger hits and the fallback page fault, a following valid cacheable cross-16B load still succeeds immediately with the expected merged payload in the adjacent-preservation corner",
        excpt.triggered == false &&
        value == 0xeeddccbbaa998877ULL
    );

    goto_priv(PRIV_M);
    TEST_ASSERT("after the recovery load succeeds, the future recovery target doublewords and their adjacent doublewords remain unchanged",
        read64(leading_phys) == leading_pattern &&
        read64(low_phys) == low_pattern &&
        read64(high_phys) == high_pattern &&
        read64(trailing_phys) == trailing_pattern
    );

    TEST_END();
}

bool ai_micro_hs_load_cross_16b_execute_trigger_with_second_half_address_slot_access_fault_spike_corner()
{
    TEST_START();

    uintptr_t vaddr = ai_micro_cross_16b_cacheable_addr(VSRWX_GRWX);
    uintptr_t second_half_vaddr = ai_micro_cross_16b_second_half_addr(VSRWX_GRWX);
    uintptr_t trigger_pc = (uintptr_t)ai_micro_cross_16b_execute_load_once;

    goto_priv(PRIV_HS);
    hspt_init();

    goto_priv(PRIV_M);
    hspt_llptw_ppn_high_bit_overflow_setup();
    sfence_vma();

    ai_micro_cross_16b_clear_mcontrol6_slot(0);
    ai_micro_cross_16b_clear_mcontrol6_slot(1);
    ai_micro_cross_16b_arm_mcontrol6_slot(0, trigger_pc, AI_MICRO_CROSS_16B_MCONTROL6_EXECUTE);
    ai_micro_cross_16b_arm_mcontrol6_slot(1, second_half_vaddr, AI_MICRO_CROSS_16B_MCONTROL6_LOAD);
    uint64_t saved_mstatus = ai_micro_cross_16b_enable_machine_breakpoint_delivery();

    goto_priv(PRIV_HS);
    TEST_SETUP_EXCEPT();
    ai_micro_cross_16b_execute_load_once(vaddr);
    TEST_ASSERT("with execute-PC trigger armed and the address slot programmed to the true second-half start address, Spike still reports breakpoint first on the cross-16B load access-fault path",
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
    ai_micro_cross_16b_execute_load_once(vaddr);
    TEST_ASSERT("after clearing the execute-PC slot, the second-half-start address slot does not keep matching on Spike; the cross-16B load falls straight back to the underlying load access fault",
        excpt.triggered == true &&
        excpt.cause == CAUSE_LAF &&
        excpt.tval == vaddr
    );
    ai_micro_cross_16b_restore_machine_status(saved_mstatus);

    ai_micro_cross_16b_clear_mcontrol6_slot(1);

    goto_priv(PRIV_HS);
    hspt_init();
    sfence_vma();

    TEST_END();
}

bool ai_micro_hs_load_cross_16b_execute_trigger_with_second_half_address_slot_access_fault_repeated_breakpoint_then_fault_spike_corner()
{
    TEST_START();

    uintptr_t vaddr = ai_micro_cross_16b_cacheable_addr(VSRWX_GRWX);
    uintptr_t second_half_vaddr = ai_micro_cross_16b_second_half_addr(VSRWX_GRWX);
    uintptr_t trigger_pc = (uintptr_t)ai_micro_cross_16b_execute_load_once;

    goto_priv(PRIV_HS);
    hspt_init();

    goto_priv(PRIV_M);
    hspt_llptw_ppn_high_bit_overflow_setup();
    sfence_vma();

    ai_micro_cross_16b_clear_mcontrol6_slot(0);
    ai_micro_cross_16b_clear_mcontrol6_slot(1);
    ai_micro_cross_16b_arm_mcontrol6_slot(0, trigger_pc, AI_MICRO_CROSS_16B_MCONTROL6_EXECUTE);
    ai_micro_cross_16b_arm_mcontrol6_slot(1, second_half_vaddr, AI_MICRO_CROSS_16B_MCONTROL6_LOAD);
    uint64_t saved_mstatus = ai_micro_cross_16b_enable_machine_breakpoint_delivery();

    goto_priv(PRIV_HS);
    TEST_SETUP_EXCEPT();
    ai_micro_cross_16b_execute_load_once(vaddr);
    TEST_ASSERT("with execute-PC trigger armed and the address slot programmed to the true second-half start address, the first cross-16B load access-fault attempt still reports breakpoint on Spike",
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
    ai_micro_cross_16b_execute_load_once(vaddr);
    TEST_ASSERT("with the same execute-PC and second-half-start slots still armed, the second cross-16B load access-fault attempt still reports breakpoint on Spike",
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
    ai_micro_cross_16b_execute_load_once(vaddr);
    TEST_ASSERT("after two prior execute-trigger breakpoint hits, clearing only the execute-PC slot still leaves the second-half-start address slot ignored on Spike, so the cross-16B load falls back to load access fault",
        excpt.triggered == true &&
        excpt.cause == CAUSE_LAF &&
        excpt.tval == vaddr
    );
    ai_micro_cross_16b_restore_machine_status(saved_mstatus);

    ai_micro_cross_16b_clear_mcontrol6_slot(1);

    goto_priv(PRIV_HS);
    hspt_init();
    sfence_vma();

    TEST_END();
}

bool ai_micro_hs_load_cross_16b_execute_trigger_with_second_half_address_slot_access_fault_repeated_breakpoint_then_fault_adjacent_preserved_spike_corner()
{
    TEST_START();

    uintptr_t paddr = phys_page_base(VSRWX_GRWX);
    uintptr_t vaddr = ai_micro_cross_16b_cacheable_addr(VSRWX_GRWX);
    uintptr_t second_half_vaddr = ai_micro_cross_16b_second_half_addr(VSRWX_GRWX);
    uintptr_t leading_phys = paddr;
    uintptr_t low_phys = paddr + 8U;
    uintptr_t high_phys = paddr + 16U;
    uintptr_t trailing_phys = paddr + 24U;
    uintptr_t trigger_pc = (uintptr_t)ai_micro_cross_16b_execute_load_once;
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

    goto_priv(PRIV_M);
    hspt_llptw_ppn_high_bit_overflow_setup();
    sfence_vma();

    ai_micro_cross_16b_clear_mcontrol6_slot(0);
    ai_micro_cross_16b_clear_mcontrol6_slot(1);
    ai_micro_cross_16b_arm_mcontrol6_slot(0, trigger_pc, AI_MICRO_CROSS_16B_MCONTROL6_EXECUTE);
    ai_micro_cross_16b_arm_mcontrol6_slot(1, second_half_vaddr, AI_MICRO_CROSS_16B_MCONTROL6_LOAD);
    saved_mstatus = ai_micro_cross_16b_enable_machine_breakpoint_delivery();

    goto_priv(PRIV_HS);
    TEST_SETUP_EXCEPT();
    ai_micro_cross_16b_execute_load_once(vaddr);
    TEST_ASSERT("with execute-PC trigger armed and the address slot programmed to the true second-half start address, the first cross-16B load access-fault attempt still reports breakpoint on Spike in the adjacent-preservation corner",
        excpt.triggered == true &&
        excpt.cause == CAUSE_BKP
    );

    goto_priv(PRIV_M);
    TEST_ASSERT("the first trapped cross-16B load access-fault attempt still leaves the two touched aligned doublewords and their immediate neighbors unchanged while only the execute-PC slot is effective on Spike",
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
    ai_micro_cross_16b_execute_load_once(vaddr);
    TEST_ASSERT("with the same execute-PC and second-half-start slots still armed, the second cross-16B load access-fault attempt still reports breakpoint on Spike in the adjacent-preservation corner",
        excpt.triggered == true &&
        excpt.cause == CAUSE_BKP
    );

    goto_priv(PRIV_M);
    TEST_ASSERT("the second trapped cross-16B load access-fault attempt still leaves the two touched aligned doublewords and their immediate neighbors unchanged while only the execute-PC slot is effective on Spike",
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
    ai_micro_cross_16b_execute_load_once(vaddr);
    TEST_ASSERT("after two prior execute-trigger breakpoint hits, clearing only the execute-PC slot still leaves the second-half-start address slot ignored on Spike, so the cross-16B load falls back to load access fault in the adjacent-preservation corner",
        excpt.triggered == true &&
        excpt.cause == CAUSE_LAF &&
        excpt.tval == vaddr
    );

    goto_priv(PRIV_M);
    TEST_ASSERT("after the fallback load access fault, the same cross-16B load probe still leaves the two touched aligned doublewords and their immediate neighbors unchanged",
        read64(leading_phys) == leading_pattern &&
        read64(low_phys) == low_pattern &&
        read64(high_phys) == high_pattern &&
        read64(trailing_phys) == trailing_pattern
    );
    ai_micro_cross_16b_restore_machine_status(saved_mstatus);

    ai_micro_cross_16b_clear_mcontrol6_slot(1);

    goto_priv(PRIV_HS);
    hspt_init();
    sfence_vma();

    TEST_END();
}

bool ai_micro_hs_load_cross_16b_execute_and_address_trigger_access_fault_priority_corner()
{
    TEST_START();

    uintptr_t vaddr = ai_micro_cross_16b_cacheable_addr(VSRWX_GRWX);
    uintptr_t trigger_pc = (uintptr_t)ai_micro_cross_16b_execute_load_once;

    goto_priv(PRIV_HS);
    hspt_init();

    goto_priv(PRIV_M);
    hspt_llptw_ppn_high_bit_overflow_setup();
    sfence_vma();

    ai_micro_cross_16b_clear_mcontrol6_slot(0);
    ai_micro_cross_16b_clear_mcontrol6_slot(1);
    ai_micro_cross_16b_arm_mcontrol6_slot(0, trigger_pc, AI_MICRO_CROSS_16B_MCONTROL6_EXECUTE);
    ai_micro_cross_16b_arm_mcontrol6_slot(1, vaddr, AI_MICRO_CROSS_16B_MCONTROL6_LOAD);
    uint64_t saved_mstatus = ai_micro_cross_16b_enable_machine_breakpoint_delivery();

    goto_priv(PRIV_HS);
    TEST_SETUP_EXCEPT();
    ai_micro_cross_16b_execute_load_once(vaddr);
    TEST_ASSERT("an HS cross-16B load still reports breakpoint when both execute-PC and address trigger slots match a split load that would otherwise raise access fault",
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
    ai_micro_cross_16b_execute_load_once(vaddr);
    TEST_ASSERT("after clearing only the execute-PC slot, the remaining cross-16B load address trigger still reports breakpoint on the same access-fault path",
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
    ai_micro_cross_16b_execute_load_once(vaddr);
    TEST_ASSERT("after clearing both trigger slots, the same HS cross-16B load reports load access fault with the original misaligned VA",
        excpt.triggered == true &&
        excpt.cause == CAUSE_LAF &&
        excpt.tval == vaddr
    );

    goto_priv(PRIV_HS);
    hspt_init();
    sfence_vma();

    TEST_END();
}

bool ai_micro_hs_load_cross_16b_execute_and_address_trigger_access_fault_repeated_breakpoint_then_fault_corner()
{
    TEST_START();

    uintptr_t vaddr = ai_micro_cross_16b_cacheable_addr(VSRWX_GRWX);
    uintptr_t trigger_pc = (uintptr_t)ai_micro_cross_16b_execute_load_once;

    goto_priv(PRIV_HS);
    hspt_init();

    goto_priv(PRIV_M);
    hspt_llptw_ppn_high_bit_overflow_setup();
    sfence_vma();

    ai_micro_cross_16b_clear_mcontrol6_slot(0);
    ai_micro_cross_16b_clear_mcontrol6_slot(1);
    ai_micro_cross_16b_arm_mcontrol6_slot(0, trigger_pc, AI_MICRO_CROSS_16B_MCONTROL6_EXECUTE);
    ai_micro_cross_16b_arm_mcontrol6_slot(1, vaddr, AI_MICRO_CROSS_16B_MCONTROL6_LOAD);
    uint64_t saved_mstatus = ai_micro_cross_16b_enable_machine_breakpoint_delivery();

    goto_priv(PRIV_HS);
    TEST_SETUP_EXCEPT();
    ai_micro_cross_16b_execute_load_once(vaddr);
    TEST_ASSERT("with both execute-PC and address trigger slots armed on the cross-16B load access-fault path, the first access reports breakpoint",
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
    ai_micro_cross_16b_execute_load_once(vaddr);
    TEST_ASSERT("with both execute-PC and address trigger slots still armed on the same cross-16B load access-fault path, the second access still reports breakpoint",
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
    ai_micro_cross_16b_execute_load_once(vaddr);
    TEST_ASSERT("after two prior dual-slot breakpoint hits, clearing only the execute-PC slot still leaves the cross-16B load address trigger active on the same access-fault path",
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
    ai_micro_cross_16b_execute_load_once(vaddr);
    TEST_ASSERT("after clearing both trigger slots, the same cross-16B load falls back to load access fault with the original misaligned VA",
        excpt.triggered == true &&
        excpt.cause == CAUSE_LAF &&
        excpt.tval == vaddr
    );

    goto_priv(PRIV_HS);
    hspt_init();
    sfence_vma();

    TEST_END();
}

bool ai_micro_hs_load_cross_16b_execute_and_address_trigger_access_fault_repeated_breakpoint_then_fault_adjacent_preserved_corner()
{
    TEST_START();

    uintptr_t paddr = phys_page_base(VSRWX_GRWX);
    uintptr_t vaddr = ai_micro_cross_16b_cacheable_addr(VSRWX_GRWX);
    uintptr_t leading_phys = paddr;
    uintptr_t low_phys = paddr + 8U;
    uintptr_t high_phys = paddr + 16U;
    uintptr_t trailing_phys = paddr + 24U;
    uintptr_t trigger_pc = (uintptr_t)ai_micro_cross_16b_execute_load_once;
    const uint64_t leading_pattern = 0x1122aa55cc337788ULL;
    const uint64_t low_pattern = AI_MICRO_CROSS_16B_LOW_PATTERN;
    const uint64_t high_pattern = AI_MICRO_CROSS_16B_HIGH_PATTERN;
    const uint64_t trailing_pattern = 0x887733cc55aa2211ULL;
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
    ai_micro_cross_16b_arm_mcontrol6_slot(1, vaddr, AI_MICRO_CROSS_16B_MCONTROL6_LOAD);
    saved_mstatus = ai_micro_cross_16b_enable_machine_breakpoint_delivery();

    goto_priv(PRIV_HS);
    TEST_SETUP_EXCEPT();
    ai_micro_cross_16b_execute_load_once(vaddr);
    TEST_ASSERT("with both execute-PC and address trigger slots armed on the cross-16B load access-fault path, the first access still reports breakpoint in the adjacent-preservation corner",
        excpt.triggered == true &&
        excpt.cause == CAUSE_BKP
    );
    ai_micro_cross_16b_restore_machine_status(saved_mstatus);

    goto_priv(PRIV_M);
    TEST_ASSERT("the first trapped cross-16B load access-fault probe still leaves the two touched aligned doublewords and their immediate neighbors unchanged",
        read64(leading_phys) == leading_pattern &&
        read64(low_phys) == low_pattern &&
        read64(high_phys) == high_pattern &&
        read64(trailing_phys) == trailing_pattern
    );

    goto_priv(PRIV_HS);
    hspt_init();

    goto_priv(PRIV_M);
    hspt_llptw_ppn_high_bit_overflow_setup();
    sfence_vma();

    saved_mstatus = ai_micro_cross_16b_enable_machine_breakpoint_delivery();
    goto_priv(PRIV_HS);
    TEST_SETUP_EXCEPT();
    ai_micro_cross_16b_execute_load_once(vaddr);
    TEST_ASSERT("with both execute-PC and address trigger slots still armed on the same cross-16B load access-fault path, the second access still reports breakpoint in the adjacent-preservation corner",
        excpt.triggered == true &&
        excpt.cause == CAUSE_BKP
    );
    ai_micro_cross_16b_restore_machine_status(saved_mstatus);

    goto_priv(PRIV_M);
    TEST_ASSERT("the second trapped cross-16B load access-fault probe still leaves the two touched aligned doublewords and their immediate neighbors unchanged",
        read64(leading_phys) == leading_pattern &&
        read64(low_phys) == low_pattern &&
        read64(high_phys) == high_pattern &&
        read64(trailing_phys) == trailing_pattern
    );

    goto_priv(PRIV_HS);
    hspt_init();

    goto_priv(PRIV_M);
    hspt_llptw_ppn_high_bit_overflow_setup();
    sfence_vma();

    ai_micro_cross_16b_clear_mcontrol6_slot(0);
    saved_mstatus = ai_micro_cross_16b_enable_machine_breakpoint_delivery();
    goto_priv(PRIV_HS);
    TEST_SETUP_EXCEPT();
    ai_micro_cross_16b_execute_load_once(vaddr);
    TEST_ASSERT("after two prior dual-slot breakpoint hits, clearing only the execute-PC slot still leaves the cross-16B load address trigger active on the same access-fault path in the adjacent-preservation corner",
        excpt.triggered == true &&
        excpt.cause == CAUSE_BKP
    );
    ai_micro_cross_16b_restore_machine_status(saved_mstatus);

    goto_priv(PRIV_M);
    TEST_ASSERT("with only the address trigger slot armed, the trapped cross-16B load access-fault probe still leaves the two touched aligned doublewords and their immediate neighbors unchanged",
        read64(leading_phys) == leading_pattern &&
        read64(low_phys) == low_pattern &&
        read64(high_phys) == high_pattern &&
        read64(trailing_phys) == trailing_pattern
    );

    goto_priv(PRIV_HS);
    hspt_init();

    goto_priv(PRIV_M);
    hspt_llptw_ppn_high_bit_overflow_setup();
    sfence_vma();

    ai_micro_cross_16b_clear_mcontrol6_slot(1);
    goto_priv(PRIV_HS);
    TEST_SETUP_EXCEPT();
    ai_micro_cross_16b_execute_load_once(vaddr);
    TEST_ASSERT("after clearing both trigger slots, the same cross-16B load falls back to load access fault with the original misaligned VA in the adjacent-preservation corner",
        excpt.triggered == true &&
        excpt.cause == CAUSE_LAF &&
        excpt.tval == vaddr
    );

    goto_priv(PRIV_M);
    TEST_ASSERT("the fallback cross-16B load access-fault path still leaves the two touched aligned doublewords and their immediate neighbors unchanged",
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

bool ai_micro_hs_load_cross_16b_execute_and_address_trigger_access_fault_repeated_breakpoint_then_valid_cross_16b_load_recovers_corner()
{
    TEST_START();

    uintptr_t vaddr = ai_micro_cross_16b_cacheable_addr(VSRWX_GRWX);
    uintptr_t paddr = phys_page_base(VSRWX_GRWX);
    uintptr_t trigger_pc = (uintptr_t)ai_micro_cross_16b_execute_load_once;
    uint64_t saved_mstatus;
    uint64_t value = 0;

    goto_priv(PRIV_M);
    write64(paddr + 8U, AI_MICRO_CROSS_16B_LOW_PATTERN);
    write64(paddr + 16U, AI_MICRO_CROSS_16B_HIGH_PATTERN);

    goto_priv(PRIV_HS);
    hspt_init();

    goto_priv(PRIV_M);
    hspt_llptw_ppn_high_bit_overflow_setup();
    sfence_vma();

    ai_micro_cross_16b_clear_mcontrol6_slot(0);
    ai_micro_cross_16b_clear_mcontrol6_slot(1);
    ai_micro_cross_16b_arm_mcontrol6_slot(0, trigger_pc, AI_MICRO_CROSS_16B_MCONTROL6_EXECUTE);
    ai_micro_cross_16b_arm_mcontrol6_slot(1, vaddr, AI_MICRO_CROSS_16B_MCONTROL6_LOAD);
    saved_mstatus = ai_micro_cross_16b_enable_machine_breakpoint_delivery();

    goto_priv(PRIV_HS);
    TEST_SETUP_EXCEPT();
    ai_micro_cross_16b_execute_load_once(vaddr);
    TEST_ASSERT("with both execute-PC and address trigger slots armed on the cross-16B load access-fault path, the first access still reports breakpoint before recovery",
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
    ai_micro_cross_16b_execute_load_once(vaddr);
    TEST_ASSERT("with both trigger slots still armed on the same cross-16B load access-fault path, the second access still reports breakpoint before recovery",
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
    ai_micro_cross_16b_execute_load_once(vaddr);
    TEST_ASSERT("after two prior dual-slot breakpoint hits, clearing only the execute-PC slot still leaves the cross-16B load address trigger active on the same access-fault path before recovery",
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
    ai_micro_cross_16b_execute_load_once(vaddr);
    TEST_ASSERT("after clearing both trigger slots, the same cross-16B load first falls back to load access fault with the original misaligned VA before recovery",
        excpt.triggered == true &&
        excpt.cause == CAUSE_LAF &&
        excpt.tval == vaddr
    );

    goto_priv(PRIV_HS);
    hspt_init();
    sfence_vma();

    TEST_SETUP_EXCEPT();
    value = ld(vaddr);
    TEST_ASSERT("after repeated trigger hits and the fallback access fault, restoring translation lets the same cacheable cross-16B load succeed with the expected merged payload",
        excpt.triggered == false &&
        value == 0xeeddccbbaa998877ULL
    );

    TEST_END();
}

bool ai_micro_hs_load_cross_16b_execute_and_address_trigger_access_fault_repeated_breakpoint_then_valid_cross_16b_load_adjacent_preserved_corner()
{
    TEST_START();

    uintptr_t vaddr = ai_micro_cross_16b_cacheable_addr(VSRWX_GRWX);
    uintptr_t paddr = phys_page_base(VSRWX_GRWX);
    uintptr_t leading_phys = paddr;
    uintptr_t low_phys = paddr + 8U;
    uintptr_t high_phys = paddr + 16U;
    uintptr_t trailing_phys = paddr + 24U;
    uintptr_t trigger_pc = (uintptr_t)ai_micro_cross_16b_execute_load_once;
    const uint64_t leading_pattern = 0x1122334455667788ULL;
    const uint64_t low_pattern = AI_MICRO_CROSS_16B_LOW_PATTERN;
    const uint64_t high_pattern = AI_MICRO_CROSS_16B_HIGH_PATTERN;
    const uint64_t trailing_pattern = 0x8877665544332211ULL;
    uint64_t saved_mstatus;
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

    ai_micro_cross_16b_clear_mcontrol6_slot(0);
    ai_micro_cross_16b_clear_mcontrol6_slot(1);
    ai_micro_cross_16b_arm_mcontrol6_slot(0, trigger_pc, AI_MICRO_CROSS_16B_MCONTROL6_EXECUTE);
    ai_micro_cross_16b_arm_mcontrol6_slot(1, vaddr, AI_MICRO_CROSS_16B_MCONTROL6_LOAD);
    saved_mstatus = ai_micro_cross_16b_enable_machine_breakpoint_delivery();

    goto_priv(PRIV_HS);
    TEST_SETUP_EXCEPT();
    ai_micro_cross_16b_execute_load_once(vaddr);
    TEST_ASSERT("with both execute-PC and address trigger slots armed on the cross-16B load access-fault path, the first access still reports breakpoint before recovery in the adjacent-preservation corner",
        excpt.triggered == true &&
        excpt.cause == CAUSE_BKP
    );

    goto_priv(PRIV_M);
    TEST_ASSERT("the first trapped trigger-plus-access-fault probe still leaves the future recovery target doublewords and their adjacent doublewords unchanged",
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
    ai_micro_cross_16b_execute_load_once(vaddr);
    TEST_ASSERT("with both trigger slots still armed on the same cross-16B load access-fault path, the second access still reports breakpoint before recovery in the adjacent-preservation corner",
        excpt.triggered == true &&
        excpt.cause == CAUSE_BKP
    );

    goto_priv(PRIV_M);
    TEST_ASSERT("the second trapped trigger-plus-access-fault probe still leaves the future recovery target doublewords and their adjacent doublewords unchanged",
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
    ai_micro_cross_16b_execute_load_once(vaddr);
    TEST_ASSERT("after two prior dual-slot breakpoint hits, clearing only the execute-PC slot still leaves the cross-16B load address trigger active on the same access-fault path before recovery in the adjacent-preservation corner",
        excpt.triggered == true &&
        excpt.cause == CAUSE_BKP
    );

    goto_priv(PRIV_M);
    TEST_ASSERT("with only the address trigger slot armed, the trapped trigger-plus-access-fault probe still leaves the future recovery target doublewords and their adjacent doublewords unchanged",
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
    ai_micro_cross_16b_execute_load_once(vaddr);
    TEST_ASSERT("after clearing both trigger slots, the same cross-16B load first falls back to load access fault with the original misaligned VA before recovery in the adjacent-preservation corner",
        excpt.triggered == true &&
        excpt.cause == CAUSE_LAF &&
        excpt.tval == vaddr
    );

    goto_priv(PRIV_M);
    TEST_ASSERT("the fallback cross-16B load access-fault path still leaves the future recovery target doublewords and their adjacent doublewords unchanged",
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
    TEST_ASSERT("after repeated trigger hits and the fallback access fault, restoring translation lets the same cacheable cross-16B load succeed normally in the adjacent-preservation corner",
        excpt.triggered == false &&
        value == 0xeeddccbbaa998877ULL
    );

    goto_priv(PRIV_M);
    TEST_ASSERT("after the recovery load succeeds, the future recovery target doublewords and their adjacent doublewords remain unchanged",
        read64(leading_phys) == leading_pattern &&
        read64(low_phys) == low_pattern &&
        read64(high_phys) == high_pattern &&
        read64(trailing_phys) == trailing_pattern
    );

    TEST_END();
}

bool ai_micro_hs_store_cross_16b_second_half_address_trigger_ignored_spike_corner()
{
    TEST_START();

    uintptr_t paddr = phys_page_base(VSRWX_GRWX);
    uintptr_t vaddr = ai_micro_cross_16b_cacheable_addr(VSRWX_GRWX);
    uintptr_t second_half_vaddr = ai_micro_cross_16b_second_half_addr(VSRWX_GRWX);
    bool trap_triggered;

    goto_priv(PRIV_M);
    write64(paddr + 8U, AI_MICRO_CROSS_16B_LOW_PATTERN);
    write64(paddr + 16U, AI_MICRO_CROSS_16B_HIGH_PATTERN);

    goto_priv(PRIV_HS);
    hspt_init();
    sfence_vma();

    ai_micro_cross_16b_clear_mcontrol6_slot(0);
    ai_micro_cross_16b_clear_mcontrol6_slot(1);
    ai_micro_cross_16b_arm_mcontrol6_slot(0, second_half_vaddr, AI_MICRO_CROSS_16B_MCONTROL6_STORE);
    uint64_t saved_mstatus = ai_micro_cross_16b_enable_machine_breakpoint_delivery();

    goto_priv(PRIV_HS);
    TEST_SETUP_EXCEPT();
    ai_micro_cross_16b_execute_store_once(vaddr, AI_MICRO_CROSS_16B_STORE_PATTERN);
    trap_triggered = excpt.triggered;
    ai_micro_cross_16b_restore_machine_status(saved_mstatus);

    goto_priv(PRIV_M);
    TEST_ASSERT("on Spike, arming a cross-16B split-store address trigger at the true second-half start address does not match; the store commits both fragments normally",
        trap_triggered == false &&
        read64(paddr + 8U) == 0xef66554433221100ULL &&
        read64(paddr + 16U) == 0xff0123456789abcdULL
    );

    ai_micro_cross_16b_clear_mcontrol6_slot(0);
    TEST_END();
}

bool ai_micro_hs_store_cross_16b_second_half_address_trigger_ignored_before_page_fault_spike_corner()
{
    TEST_START();

    uintptr_t paddr = phys_page_base(VSI_GI);
    uintptr_t vaddr = ai_micro_cross_16b_cacheable_addr(VSI_GI);
    uintptr_t second_half_vaddr = ai_micro_cross_16b_second_half_addr(VSI_GI);

    goto_priv(PRIV_M);
    write64(paddr + 8U, AI_MICRO_CROSS_16B_LOW_PATTERN);
    write64(paddr + 16U, AI_MICRO_CROSS_16B_HIGH_PATTERN);

    goto_priv(PRIV_HS);
    hspt_init();
    sfence_vma();

    ai_micro_cross_16b_clear_mcontrol6_slot(0);
    ai_micro_cross_16b_arm_mcontrol6_slot(0, second_half_vaddr,
        AI_MICRO_CROSS_16B_MCONTROL6_STORE);
    uint64_t saved_mstatus = ai_micro_cross_16b_enable_machine_breakpoint_delivery();

    goto_priv(PRIV_HS);
    TEST_SETUP_EXCEPT();
    ai_micro_cross_16b_execute_store_once(vaddr, AI_MICRO_CROSS_16B_STORE_PATTERN);
    TEST_ASSERT("on Spike, a store-address trigger armed only at the true second-half start does not match a scalar cross-16B page-faulting store, so the original misaligned VA reports store page fault",
        excpt.triggered == true &&
        excpt.cause == CAUSE_SPF &&
        excpt.tval == vaddr
    );
    ai_micro_cross_16b_restore_machine_status(saved_mstatus);

    goto_priv(PRIV_M);
    TEST_ASSERT("the second-half-address-triggered page-faulting scalar cross-16B store leaves the backing boundary bytes unchanged before the later fallback fault probe",
        read64(paddr + 8U) == AI_MICRO_CROSS_16B_LOW_PATTERN &&
        read64(paddr + 16U) == AI_MICRO_CROSS_16B_HIGH_PATTERN
    );

    goto_priv(PRIV_M);
    ai_micro_cross_16b_clear_mcontrol6_slot(0);

    goto_priv(PRIV_HS);
    hspt_init();
    sfence_vma();

    TEST_SETUP_EXCEPT();
    ai_micro_cross_16b_execute_store_once(vaddr, AI_MICRO_CROSS_16B_STORE_PATTERN);
    TEST_ASSERT("after clearing only the second-half-start store trigger, the same scalar cross-16B page-fault path falls back to the underlying store page fault with the original misaligned VA",
        excpt.triggered == true &&
        excpt.cause == CAUSE_SPF &&
        excpt.tval == vaddr
    );

    goto_priv(PRIV_M);
    TEST_ASSERT("after the fallback store page fault, the same backing boundary bytes still remain unchanged",
        read64(paddr + 8U) == AI_MICRO_CROSS_16B_LOW_PATTERN &&
        read64(paddr + 16U) == AI_MICRO_CROSS_16B_HIGH_PATTERN
    );

    goto_priv(PRIV_HS);
    hspt_init();
    sfence_vma();

    TEST_END();
}

bool ai_micro_hs_store_cross_16b_second_half_address_trigger_ignored_before_access_fault_spike_corner()
{
    TEST_START();

    uintptr_t paddr = phys_page_base(VSRWX_GRWX);
    uintptr_t vaddr = ai_micro_cross_16b_cacheable_addr(VSRWX_GRWX);
    uintptr_t second_half_vaddr = ai_micro_cross_16b_second_half_addr(VSRWX_GRWX);

    goto_priv(PRIV_M);
    write64(paddr + 8U, AI_MICRO_CROSS_16B_LOW_PATTERN);
    write64(paddr + 16U, AI_MICRO_CROSS_16B_HIGH_PATTERN);

    goto_priv(PRIV_HS);
    hspt_init();

    goto_priv(PRIV_M);
    hspt_llptw_ppn_high_bit_overflow_setup();
    sfence_vma();

    ai_micro_cross_16b_clear_mcontrol6_slot(0);
    ai_micro_cross_16b_arm_mcontrol6_slot(0, second_half_vaddr,
        AI_MICRO_CROSS_16B_MCONTROL6_STORE);
    uint64_t saved_mstatus = ai_micro_cross_16b_enable_machine_breakpoint_delivery();

    goto_priv(PRIV_HS);
    TEST_SETUP_EXCEPT();
    ai_micro_cross_16b_execute_store_once(vaddr, AI_MICRO_CROSS_16B_STORE_PATTERN);
    TEST_ASSERT("on Spike, a store-address trigger armed only at the true second-half start does not match a scalar cross-16B access-faulting store, so the original misaligned VA reports store access fault",
        excpt.triggered == true &&
        excpt.cause == CAUSE_SAF &&
        excpt.tval == vaddr
    );
    ai_micro_cross_16b_restore_machine_status(saved_mstatus);

    goto_priv(PRIV_M);
    TEST_ASSERT("the second-half-address-triggered access-faulting scalar cross-16B store leaves the backing boundary bytes unchanged before the later fallback fault probe",
        read64(paddr + 8U) == AI_MICRO_CROSS_16B_LOW_PATTERN &&
        read64(paddr + 16U) == AI_MICRO_CROSS_16B_HIGH_PATTERN
    );

    goto_priv(PRIV_M);
    ai_micro_cross_16b_clear_mcontrol6_slot(0);

    goto_priv(PRIV_HS);
    TEST_SETUP_EXCEPT();
    ai_micro_cross_16b_execute_store_once(vaddr, AI_MICRO_CROSS_16B_STORE_PATTERN);
    TEST_ASSERT("after clearing only the second-half-start store trigger, the same scalar cross-16B access-fault path falls back to the underlying store access fault with the original misaligned VA",
        excpt.triggered == true &&
        excpt.cause == CAUSE_SAF &&
        excpt.tval == vaddr
    );

    goto_priv(PRIV_M);
    TEST_ASSERT("after the fallback store access fault, the same backing boundary bytes still remain unchanged",
        read64(paddr + 8U) == AI_MICRO_CROSS_16B_LOW_PATTERN &&
        read64(paddr + 16U) == AI_MICRO_CROSS_16B_HIGH_PATTERN
    );

    goto_priv(PRIV_HS);
    hspt_init();
    sfence_vma();

    TEST_END();
}

bool ai_micro_hs_store_cross_16b_dual_address_trigger_only_original_matches_spike_corner()
{
    TEST_START();

    uintptr_t paddr = phys_page_base(VSRWX_GRWX);
    uintptr_t vaddr = ai_micro_cross_16b_cacheable_addr(VSRWX_GRWX);
    uintptr_t second_half_vaddr = ai_micro_cross_16b_second_half_addr(VSRWX_GRWX);
    bool trap_triggered;

    goto_priv(PRIV_M);
    write64(paddr + 8U, AI_MICRO_CROSS_16B_LOW_PATTERN);
    write64(paddr + 16U, AI_MICRO_CROSS_16B_HIGH_PATTERN);

    goto_priv(PRIV_HS);
    hspt_init();
    sfence_vma();

    ai_micro_cross_16b_clear_mcontrol6_slot(0);
    ai_micro_cross_16b_clear_mcontrol6_slot(1);
    ai_micro_cross_16b_arm_mcontrol6_slot(0, vaddr, AI_MICRO_CROSS_16B_MCONTROL6_STORE);
    ai_micro_cross_16b_arm_mcontrol6_slot(1, second_half_vaddr, AI_MICRO_CROSS_16B_MCONTROL6_STORE);
    uint64_t saved_mstatus = ai_micro_cross_16b_enable_machine_breakpoint_delivery();

    goto_priv(PRIV_HS);
    TEST_SETUP_EXCEPT();
    ai_micro_cross_16b_execute_store_once(vaddr, AI_MICRO_CROSS_16B_STORE_PATTERN);
    trap_triggered = excpt.triggered;
    ai_micro_cross_16b_restore_machine_status(saved_mstatus);

    goto_priv(PRIV_M);
    TEST_ASSERT("with both the original misaligned store address slot and the true second-half-start slot armed, Spike still reports breakpoint before any split-store bytes commit because only the original address matches",
        trap_triggered == true &&
        read64(paddr + 8U) == AI_MICRO_CROSS_16B_LOW_PATTERN &&
        read64(paddr + 16U) == AI_MICRO_CROSS_16B_HIGH_PATTERN
    );

    ai_micro_cross_16b_clear_mcontrol6_slot(0);
    saved_mstatus = ai_micro_cross_16b_enable_machine_breakpoint_delivery();

    goto_priv(PRIV_HS);
    TEST_SETUP_EXCEPT();
    ai_micro_cross_16b_execute_store_once(vaddr, AI_MICRO_CROSS_16B_STORE_PATTERN);
    trap_triggered = excpt.triggered;
    ai_micro_cross_16b_restore_machine_status(saved_mstatus);

    goto_priv(PRIV_M);
    TEST_ASSERT("after clearing the original misaligned-address slot, the remaining second-half-start store slot stays ignored on Spike and the cross-16B store commits normally",
        trap_triggered == false &&
        read64(paddr + 8U) == 0xef66554433221100ULL &&
        read64(paddr + 16U) == 0xff0123456789abcdULL
    );

    ai_micro_cross_16b_clear_mcontrol6_slot(1);
    TEST_END();
}

bool ai_micro_hs_store_cross_16b_dual_address_trigger_repeated_breakpoint_then_completion_spike_corner()
{
    TEST_START();

    uintptr_t paddr = phys_page_base(VSRWX_GRWX);
    uintptr_t vaddr = ai_micro_cross_16b_cacheable_addr(VSRWX_GRWX);
    uintptr_t second_half_vaddr = ai_micro_cross_16b_second_half_addr(VSRWX_GRWX);
    bool trap_triggered;

    goto_priv(PRIV_M);
    write64(paddr + 8U, AI_MICRO_CROSS_16B_LOW_PATTERN);
    write64(paddr + 16U, AI_MICRO_CROSS_16B_HIGH_PATTERN);

    goto_priv(PRIV_HS);
    hspt_init();
    sfence_vma();

    ai_micro_cross_16b_clear_mcontrol6_slot(0);
    ai_micro_cross_16b_clear_mcontrol6_slot(1);
    ai_micro_cross_16b_arm_mcontrol6_slot(0, vaddr, AI_MICRO_CROSS_16B_MCONTROL6_STORE);
    ai_micro_cross_16b_arm_mcontrol6_slot(1, second_half_vaddr, AI_MICRO_CROSS_16B_MCONTROL6_STORE);
    uint64_t saved_mstatus = ai_micro_cross_16b_enable_machine_breakpoint_delivery();

    goto_priv(PRIV_HS);
    TEST_SETUP_EXCEPT();
    ai_micro_cross_16b_execute_store_once(vaddr, AI_MICRO_CROSS_16B_STORE_PATTERN);
    trap_triggered = excpt.triggered;
    ai_micro_cross_16b_restore_machine_status(saved_mstatus);

    goto_priv(PRIV_M);
    TEST_ASSERT("with both split-store address slots armed, the first cross-16B store still reports breakpoint on Spike before any fragment commits because only the original misaligned vaddr matches",
        trap_triggered == true &&
        read64(paddr + 8U) == AI_MICRO_CROSS_16B_LOW_PATTERN &&
        read64(paddr + 16U) == AI_MICRO_CROSS_16B_HIGH_PATTERN
    );

    saved_mstatus = ai_micro_cross_16b_enable_machine_breakpoint_delivery();
    goto_priv(PRIV_HS);
    TEST_SETUP_EXCEPT();
    ai_micro_cross_16b_execute_store_once(vaddr, AI_MICRO_CROSS_16B_STORE_PATTERN);
    trap_triggered = excpt.triggered;
    ai_micro_cross_16b_restore_machine_status(saved_mstatus);

    goto_priv(PRIV_M);
    TEST_ASSERT("repeating the same cross-16B store with both address slots still armed reports breakpoint again and still leaves both aligned doublewords untouched",
        trap_triggered == true &&
        read64(paddr + 8U) == AI_MICRO_CROSS_16B_LOW_PATTERN &&
        read64(paddr + 16U) == AI_MICRO_CROSS_16B_HIGH_PATTERN
    );

    ai_micro_cross_16b_clear_mcontrol6_slot(0);
    saved_mstatus = ai_micro_cross_16b_enable_machine_breakpoint_delivery();

    goto_priv(PRIV_HS);
    TEST_SETUP_EXCEPT();
    ai_micro_cross_16b_execute_store_once(vaddr, AI_MICRO_CROSS_16B_STORE_PATTERN);
    trap_triggered = excpt.triggered;
    ai_micro_cross_16b_restore_machine_status(saved_mstatus);

    goto_priv(PRIV_M);
    TEST_ASSERT("after two prior breakpoint hits and then clearing only the original-address slot, the remaining second-half-start store slot still stays ignored on Spike and the cross-16B store commits normally",
        trap_triggered == false &&
        read64(paddr + 8U) == 0xef66554433221100ULL &&
        read64(paddr + 16U) == 0xff0123456789abcdULL
    );

    ai_micro_cross_16b_clear_mcontrol6_slot(1);
    TEST_END();
}

bool ai_micro_hs_store_cross_16b_dual_address_trigger_page_fault_no_side_effect_spike_corner()
{
    TEST_START();

    uintptr_t paddr = phys_page_base(VSI_GI);
    uintptr_t vaddr = ai_micro_cross_16b_cacheable_addr(VSI_GI);
    uintptr_t second_half_vaddr = ai_micro_cross_16b_second_half_addr(VSI_GI);
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
    ai_micro_cross_16b_arm_mcontrol6_slot(0, vaddr, AI_MICRO_CROSS_16B_MCONTROL6_STORE);
    ai_micro_cross_16b_arm_mcontrol6_slot(1, second_half_vaddr, AI_MICRO_CROSS_16B_MCONTROL6_STORE);
    uint64_t saved_mstatus = ai_micro_cross_16b_enable_machine_breakpoint_delivery();

    goto_priv(PRIV_HS);
    TEST_SETUP_EXCEPT();
    ai_micro_cross_16b_execute_store_once(vaddr, AI_MICRO_CROSS_16B_STORE_PATTERN);
    TEST_ASSERT("with both the original misaligned store address slot and the true second-half-start slot armed, Spike still reports breakpoint on the cross-16B store page-fault path because only the original address matches",
        excpt.triggered == true &&
        excpt.cause == CAUSE_BKP
    );

    goto_priv(PRIV_M);
    TEST_ASSERT("while the original misaligned-address slot is still armed, the trapped cross-16B store page-fault path leaves both aligned doublewords unchanged",
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
    TEST_ASSERT("after clearing the original misaligned-address slot, the remaining second-half-start store slot stays ignored on Spike and the cross-16B store falls straight back to the underlying store page fault with no split-store side effects",
        trap_triggered == true &&
        trap_cause == CAUSE_SPF &&
        trap_tval == vaddr &&
        read64(paddr + 8U) == low_pattern &&
        read64(paddr + 16U) == high_pattern
    );

    ai_micro_cross_16b_clear_mcontrol6_slot(1);
    TEST_END();
}

bool ai_micro_hs_store_cross_16b_dual_address_trigger_access_fault_no_side_effect_spike_corner()
{
    TEST_START();

    uintptr_t paddr = phys_page_base(VSRWX_GRWX);
    uintptr_t vaddr = ai_micro_cross_16b_cacheable_addr(VSRWX_GRWX);
    uintptr_t second_half_vaddr = ai_micro_cross_16b_second_half_addr(VSRWX_GRWX);
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
    ai_micro_cross_16b_arm_mcontrol6_slot(0, vaddr, AI_MICRO_CROSS_16B_MCONTROL6_STORE);
    ai_micro_cross_16b_arm_mcontrol6_slot(1, second_half_vaddr, AI_MICRO_CROSS_16B_MCONTROL6_STORE);
    uint64_t saved_mstatus = ai_micro_cross_16b_enable_machine_breakpoint_delivery();

    goto_priv(PRIV_HS);
    TEST_SETUP_EXCEPT();
    ai_micro_cross_16b_execute_store_once(vaddr, AI_MICRO_CROSS_16B_STORE_PATTERN);
    TEST_ASSERT("with both the original misaligned store address slot and the true second-half-start slot armed, Spike still reports breakpoint on the cross-16B store access-fault path because only the original address matches",
        excpt.triggered == true &&
        excpt.cause == CAUSE_BKP
    );

    goto_priv(PRIV_M);
    TEST_ASSERT("while the original misaligned-address slot is still armed, the trapped cross-16B store access-fault path leaves both aligned doublewords unchanged",
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
    TEST_ASSERT("after clearing the original misaligned-address slot, the remaining second-half-start store slot stays ignored on Spike and the cross-16B store falls straight back to the underlying store access fault with no split-store side effects",
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

bool ai_micro_hs_store_cross_16b_trigger_breakpoint()
{
    TEST_START();

    uintptr_t paddr = phys_page_base(VSRWX_GRWX);
    uintptr_t vaddr = ai_micro_cross_16b_cacheable_addr(VSRWX_GRWX);

    goto_priv(PRIV_M);
    write64(paddr + 8U, AI_MICRO_CROSS_16B_LOW_PATTERN);
    write64(paddr + 16U, AI_MICRO_CROSS_16B_HIGH_PATTERN);

    goto_priv(PRIV_HS);
    hspt_init();
    sfence_vma();

    ai_micro_cross_16b_arm_mcontrol6_0(vaddr, AI_MICRO_CROSS_16B_MCONTROL6_STORE);
    uint64_t saved_mstatus = ai_micro_cross_16b_enable_machine_breakpoint_delivery();

    goto_priv(PRIV_HS);
    TEST_SETUP_EXCEPT();
    sd(vaddr, AI_MICRO_CROSS_16B_STORE_PATTERN);
    TEST_ASSERT("an HS cross-16B cacheable store reports breakpoint when the misaligned address hits an mcontrol6 store trigger",
        excpt.triggered == true &&
        excpt.cause == CAUSE_BKP
    );

    goto_priv(PRIV_M);
    TEST_ASSERT("the trapped HS cross-16B store leaves the lower aligned doubleword unchanged before the trigger is cleared",
        read64(paddr + 8U) == AI_MICRO_CROSS_16B_LOW_PATTERN
    );
    TEST_ASSERT("the trapped HS cross-16B store leaves the upper aligned doubleword unchanged before the trigger is cleared",
        read64(paddr + 16U) == AI_MICRO_CROSS_16B_HIGH_PATTERN
    );

    ai_micro_cross_16b_restore_machine_status(saved_mstatus);
    ai_micro_cross_16b_clear_mcontrol6_0();

    goto_priv(PRIV_HS);
    TEST_SETUP_EXCEPT();
    sd(vaddr, AI_MICRO_CROSS_16B_STORE_PATTERN);
    TEST_ASSERT("the same HS cross-16B cacheable store completes normally after clearing the trigger",
        excpt.triggered == false
    );

    goto_priv(PRIV_M);
    TEST_ASSERT("after the trigger is cleared, the lower aligned doubleword reflects the split store bytes",
        read64(paddr + 8U) == 0xef66554433221100ULL
    );
    TEST_ASSERT("after the trigger is cleared, the upper aligned doubleword reflects the split store bytes",
        read64(paddr + 16U) == 0xff0123456789abcdULL
    );

    TEST_END();
}

bool ai_micro_hs_store_cross_16b_execute_trigger_with_second_half_address_slot_ignored_spike_corner()
{
    TEST_START();

    uintptr_t paddr = phys_page_base(VSRWX_GRWX);
    uintptr_t vaddr = ai_micro_cross_16b_cacheable_addr(VSRWX_GRWX);
    uintptr_t second_half_vaddr = ai_micro_cross_16b_second_half_addr(VSRWX_GRWX);
    uintptr_t trigger_pc = (uintptr_t)ai_micro_cross_16b_execute_store_once;
    bool trap_triggered;

    goto_priv(PRIV_M);
    write64(paddr + 8U, AI_MICRO_CROSS_16B_LOW_PATTERN);
    write64(paddr + 16U, AI_MICRO_CROSS_16B_HIGH_PATTERN);

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
    TEST_ASSERT("with execute-PC trigger armed and the address slot programmed to the true second-half start address, Spike still reports breakpoint first on the cross-16B split store",
        excpt.triggered == true &&
        excpt.cause == CAUSE_BKP
    );

    goto_priv(PRIV_M);
    TEST_ASSERT("while the execute-PC slot is still armed, the misprogrammed second-half address slot does not matter and the trapped cross-16B split store leaves both aligned doublewords unchanged",
        read64(paddr + 8U) == AI_MICRO_CROSS_16B_LOW_PATTERN &&
        read64(paddr + 16U) == AI_MICRO_CROSS_16B_HIGH_PATTERN
    );
    ai_micro_cross_16b_restore_machine_status(saved_mstatus);

    ai_micro_cross_16b_clear_mcontrol6_slot(0);
    saved_mstatus = ai_micro_cross_16b_enable_machine_breakpoint_delivery();

    goto_priv(PRIV_HS);
    TEST_SETUP_EXCEPT();
    ai_micro_cross_16b_execute_store_once(vaddr, AI_MICRO_CROSS_16B_STORE_PATTERN);
    trap_triggered = excpt.triggered;
    ai_micro_cross_16b_restore_machine_status(saved_mstatus);

    goto_priv(PRIV_M);
    TEST_ASSERT("after clearing the execute-PC slot, the second-half-start address slot does not keep matching on Spike; the cross-16B split store commits normally",
        trap_triggered == false &&
        read64(paddr + 8U) == 0xef66554433221100ULL &&
        read64(paddr + 16U) == 0xff0123456789abcdULL
    );

    ai_micro_cross_16b_clear_mcontrol6_slot(1);
    TEST_END();
}

bool ai_micro_hs_store_cross_16b_execute_trigger_with_second_half_address_slot_repeated_breakpoint_then_completion_spike_corner()
{
    TEST_START();

    uintptr_t paddr = phys_page_base(VSRWX_GRWX);
    uintptr_t vaddr = ai_micro_cross_16b_cacheable_addr(VSRWX_GRWX);
    uintptr_t second_half_vaddr = ai_micro_cross_16b_second_half_addr(VSRWX_GRWX);
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
    ai_micro_cross_16b_arm_mcontrol6_slot(1, second_half_vaddr, AI_MICRO_CROSS_16B_MCONTROL6_STORE);
    uint64_t saved_mstatus = ai_micro_cross_16b_enable_machine_breakpoint_delivery();

    goto_priv(PRIV_HS);
    TEST_SETUP_EXCEPT();
    ai_micro_cross_16b_execute_store_once(vaddr, AI_MICRO_CROSS_16B_STORE_PATTERN);
    TEST_ASSERT("with execute-PC trigger armed and the address slot programmed to the true second-half start address, the first cross-16B split-store attempt still reports breakpoint on Spike",
        excpt.triggered == true &&
        excpt.cause == CAUSE_BKP
    );

    goto_priv(PRIV_M);
    TEST_ASSERT("the first trapped cross-16B split-store attempt still leaves both aligned doublewords unchanged while only the execute-PC slot is effective on Spike",
        read64(paddr + 8U) == AI_MICRO_CROSS_16B_LOW_PATTERN &&
        read64(paddr + 16U) == AI_MICRO_CROSS_16B_HIGH_PATTERN
    );
    ai_micro_cross_16b_restore_machine_status(saved_mstatus);

    saved_mstatus = ai_micro_cross_16b_enable_machine_breakpoint_delivery();
    goto_priv(PRIV_HS);
    TEST_SETUP_EXCEPT();
    ai_micro_cross_16b_execute_store_once(vaddr, AI_MICRO_CROSS_16B_STORE_PATTERN);
    TEST_ASSERT("with the same execute-PC and second-half-start slots still armed, the second cross-16B split-store attempt still reports breakpoint on Spike",
        excpt.triggered == true &&
        excpt.cause == CAUSE_BKP
    );

    goto_priv(PRIV_M);
    TEST_ASSERT("the second trapped cross-16B split-store attempt still leaves both aligned doublewords unchanged while only the execute-PC slot is effective on Spike",
        read64(paddr + 8U) == AI_MICRO_CROSS_16B_LOW_PATTERN &&
        read64(paddr + 16U) == AI_MICRO_CROSS_16B_HIGH_PATTERN
    );
    ai_micro_cross_16b_restore_machine_status(saved_mstatus);

    ai_micro_cross_16b_clear_mcontrol6_slot(0);
    saved_mstatus = ai_micro_cross_16b_enable_machine_breakpoint_delivery();

    goto_priv(PRIV_HS);
    TEST_SETUP_EXCEPT();
    ai_micro_cross_16b_execute_store_once(vaddr, AI_MICRO_CROSS_16B_STORE_PATTERN);
    TEST_ASSERT("after two prior execute-trigger breakpoint hits, clearing only the execute-PC slot still leaves the second-half-start address slot ignored on Spike, so the cross-16B split store commits normally",
        excpt.triggered == false
    );
    ai_micro_cross_16b_restore_machine_status(saved_mstatus);

    goto_priv(PRIV_M);
    TEST_ASSERT("after the repeated execute-trigger probes, the recovered cross-16B split store still writes the expected lower and upper aligned fragments",
        read64(paddr + 8U) == 0xef66554433221100ULL &&
        read64(paddr + 16U) == 0xff0123456789abcdULL
    );

    ai_micro_cross_16b_clear_mcontrol6_slot(1);
    TEST_END();
}

bool ai_micro_hs_store_cross_16b_execute_trigger_with_second_half_address_slot_repeated_breakpoint_then_completion_adjacent_preserved_spike_corner()
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
    TEST_ASSERT("with execute-PC trigger armed and the address slot programmed to the true second-half start address, the first cross-16B split-store attempt still reports breakpoint on Spike in the adjacent-preservation corner",
        excpt.triggered == true &&
        excpt.cause == CAUSE_BKP
    );

    goto_priv(PRIV_M);
    TEST_ASSERT("the first trapped cross-16B split-store attempt still leaves both aligned doublewords and their immediate neighbors unchanged while only the execute-PC slot is effective on Spike",
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
    TEST_ASSERT("with the same execute-PC and second-half-start slots still armed, the second cross-16B split-store attempt still reports breakpoint on Spike in the adjacent-preservation corner",
        excpt.triggered == true &&
        excpt.cause == CAUSE_BKP
    );

    goto_priv(PRIV_M);
    TEST_ASSERT("the second trapped cross-16B split-store attempt still leaves both aligned doublewords and their immediate neighbors unchanged while only the execute-PC slot is effective on Spike",
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
    TEST_ASSERT("after two prior execute-trigger breakpoint hits, clearing only the execute-PC slot still leaves the second-half-start address slot ignored on Spike, so the cross-16B split store commits normally in the adjacent-preservation corner",
        excpt.triggered == false
    );
    ai_micro_cross_16b_restore_machine_status(saved_mstatus);

    goto_priv(PRIV_M);
    TEST_ASSERT("after the repeated execute-trigger probes, the recovered cross-16B split store still writes only the expected lower and upper aligned fragments while preserving the immediate neighbors",
        read64(leading_phys) == leading_pattern &&
        read64(low_phys) == 0xef66554433221100ULL &&
        read64(high_phys) == 0xff0123456789abcdULL &&
        read64(trailing_phys) == trailing_pattern
    );

    ai_micro_cross_16b_clear_mcontrol6_slot(1);
    TEST_END();
}

bool ai_micro_hs_store_cross_16b_execute_trigger_with_second_half_address_slot_page_fault_spike_corner()
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
    TEST_ASSERT("with execute-PC trigger armed and the address slot programmed to the true second-half start address, Spike still reports breakpoint first on the cross-16B store page-fault path",
        excpt.triggered == true &&
        excpt.cause == CAUSE_BKP
    );

    goto_priv(PRIV_M);
    TEST_ASSERT("while the execute-PC slot is still armed, the misprogrammed second-half address slot does not matter and the trapped cross-16B store page-fault path leaves both aligned doublewords unchanged",
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
    TEST_ASSERT("after clearing the execute-PC slot, the second-half-start address slot does not keep matching on Spike; the cross-16B store falls straight back to the underlying store page fault with no split-store side effects",
        trap_triggered == true &&
        trap_cause == CAUSE_SPF &&
        trap_tval == vaddr &&
        read64(paddr + 8U) == low_pattern &&
        read64(paddr + 16U) == high_pattern
    );

    ai_micro_cross_16b_clear_mcontrol6_slot(1);
    TEST_END();
}
