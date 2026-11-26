#include "ai_micro_cross_page_trigger_common.h"

bool ai_micro_hs_load_cross_page_low_half_execute_trigger_with_second_half_address_slot_access_fault_spike_corner()
{
    TEST_START();

    uintptr_t vaddr = ai_micro_cross_page_trigger_addr(VSRWX_GRX, 4U);
    uintptr_t second_half_vaddr = vaddr + 4U;
    uintptr_t trigger_pc = (uintptr_t)ai_micro_cross_page_execute_load_once;
    uintptr_t deny_start = phys_page_base(VSRWX_GRX);
    uintptr_t deny_end = deny_start + PAGE_SIZE;

    ai_micro_cross_page_configure_pmp_window(deny_start, deny_end, AI_MICRO_PMP_TOR_NO_R);

    goto_priv(PRIV_HS);
    hspt_init();
    sfence_vma();

    ai_micro_cross_page_clear_mcontrol6_slot(0);
    ai_micro_cross_page_clear_mcontrol6_slot(1);
    ai_micro_cross_page_arm_mcontrol6_slot(0, trigger_pc,
        AI_MICRO_CROSS_PAGE_MCONTROL6_EXECUTE);
    ai_micro_cross_page_arm_mcontrol6_slot(1, second_half_vaddr,
        AI_MICRO_CROSS_PAGE_MCONTROL6_LOAD);
    uint64_t saved_mstatus = ai_micro_cross_page_enable_machine_breakpoint_delivery();

    goto_priv(PRIV_HS);
    TEST_SETUP_EXCEPT();
    ai_micro_cross_page_execute_load_once(vaddr);
    TEST_ASSERT("with execute-PC trigger armed and the address slot programmed to the true second-half start address, Spike still reports breakpoint first on the low-half access-fault split load",
        excpt.triggered == true &&
        excpt.cause == CAUSE_BKP
    );
    ai_micro_cross_page_restore_machine_status(saved_mstatus);

    goto_priv(PRIV_HS);
    hspt_init();
    sfence_vma();

    ai_micro_cross_page_clear_mcontrol6_slot(0);
    goto_priv(PRIV_HS);
    TEST_SETUP_EXCEPT();
    ai_micro_cross_page_execute_load_once(vaddr);
    TEST_ASSERT("after clearing the execute-PC slot, the second-half-start address slot does not keep matching when the low half faults first; Spike falls straight back to the underlying low-half load access fault",
        excpt.triggered == true &&
        excpt.cause == CAUSE_LAF &&
        excpt.tval == vaddr
    );

    ai_micro_cross_page_clear_mcontrol6_slot(1);
    ai_micro_cross_page_clear_pmp_window();

    TEST_END();
}

bool ai_micro_hs_load_cross_page_low_half_execute_trigger_with_second_half_address_slot_access_fault_repeated_breakpoint_then_fault_spike_corner()
{
    TEST_START();

    uintptr_t vaddr = ai_micro_cross_page_trigger_addr(VSRWX_GRX, 4U);
    uintptr_t second_half_vaddr = vaddr + 4U;
    uintptr_t trigger_pc = (uintptr_t)ai_micro_cross_page_execute_load_once;
    uintptr_t deny_start = phys_page_base(VSRWX_GRX);
    uintptr_t deny_end = deny_start + PAGE_SIZE;
    uint64_t saved_mstatus;

    ai_micro_cross_page_configure_pmp_window(deny_start, deny_end, AI_MICRO_PMP_TOR_NO_R);

    goto_priv(PRIV_HS);
    hspt_init();
    sfence_vma();

    ai_micro_cross_page_clear_mcontrol6_slot(0);
    ai_micro_cross_page_clear_mcontrol6_slot(1);
    ai_micro_cross_page_arm_mcontrol6_slot(0, trigger_pc,
        AI_MICRO_CROSS_PAGE_MCONTROL6_EXECUTE);
    ai_micro_cross_page_arm_mcontrol6_slot(1, second_half_vaddr,
        AI_MICRO_CROSS_PAGE_MCONTROL6_LOAD);
    saved_mstatus = ai_micro_cross_page_enable_machine_breakpoint_delivery();

    goto_priv(PRIV_HS);
    TEST_SETUP_EXCEPT();
    ai_micro_cross_page_execute_load_once(vaddr);
    TEST_ASSERT("with execute-PC trigger armed and the address slot programmed to the true second-half start address, the first low-half access-fault cross-page split-load attempt still reports breakpoint on Spike",
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
    TEST_ASSERT("with the same execute-PC and second-half-start slots still armed, the second low-half access-fault cross-page split-load attempt still reports breakpoint on Spike",
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
    TEST_ASSERT("after two prior execute-trigger breakpoint hits, clearing only the execute-PC slot still leaves the second-half-start address slot ignored when the low half faults first on Spike, so the split load falls back to the underlying low-half load access fault",
        excpt.triggered == true &&
        excpt.cause == CAUSE_LAF &&
        excpt.tval == vaddr
    );
    ai_micro_cross_page_restore_machine_status(saved_mstatus);

    ai_micro_cross_page_clear_mcontrol6_slot(1);
    ai_micro_cross_page_clear_pmp_window();

    TEST_END();
}

bool ai_micro_hs_load_cross_page_high_half_execute_and_address_trigger_page_fault_priority_corner()
{
    TEST_START();

    uintptr_t vaddr = ai_micro_cross_page_trigger_addr(VSR_GX, 4U);
    uintptr_t second_half_vaddr = vaddr + 4U;
    uintptr_t trigger_pc = (uintptr_t)ai_micro_cross_page_execute_load_once;

    goto_priv(PRIV_HS);
    hspt_init();
    sfence_vma();

    ai_micro_cross_page_clear_mcontrol6_slot(0);
    ai_micro_cross_page_clear_mcontrol6_slot(1);
    ai_micro_cross_page_arm_mcontrol6_slot(0, trigger_pc,
        AI_MICRO_CROSS_PAGE_MCONTROL6_EXECUTE);
    ai_micro_cross_page_arm_mcontrol6_slot(1, vaddr, AI_MICRO_CROSS_PAGE_MCONTROL6_LOAD);
    uint64_t saved_mstatus = ai_micro_cross_page_enable_machine_breakpoint_delivery();

    goto_priv(PRIV_HS);
    TEST_SETUP_EXCEPT();
    ai_micro_cross_page_execute_load_once(vaddr);
    TEST_ASSERT("an HS cross-page load still reports breakpoint when both execute-PC and address trigger slots match a split load whose upper half would otherwise raise page fault",
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
    TEST_ASSERT("after clearing only the execute-PC slot, the remaining cross-page load address trigger still reports breakpoint on the same upper-half page-fault path",
        excpt.triggered == true &&
        excpt.cause == CAUSE_BKP
    );
    ai_micro_cross_page_restore_machine_status(saved_mstatus);

    goto_priv(PRIV_HS);
    hspt_init();
    sfence_vma();

    ai_micro_cross_page_clear_mcontrol6_slot(1);
    goto_priv(PRIV_HS);
    TEST_SETUP_EXCEPT();
    ai_micro_cross_page_execute_load_once(vaddr);
    TEST_ASSERT("after clearing both trigger slots, the same HS cross-page load reports load page fault with the true second-half start address",
        excpt.triggered == true &&
        excpt.cause == CAUSE_LPF &&
        excpt.tval == second_half_vaddr
    );

    TEST_END();
}

bool ai_micro_hs_load_cross_page_high_half_execute_and_address_trigger_access_fault_priority_corner()
{
    TEST_START();

    uintptr_t vaddr = ai_micro_cross_page_trigger_addr(VSRWX_GRX, 4U);
    uintptr_t second_half_vaddr = vaddr + 4U;
    uintptr_t trigger_pc = (uintptr_t)ai_micro_cross_page_execute_load_once;
    uintptr_t deny_start = phys_page_base(VSRWX_GR);
    uintptr_t deny_end = deny_start + PAGE_SIZE;

    ai_micro_cross_page_configure_pmp_window(deny_start, deny_end, AI_MICRO_PMP_TOR_NO_R);

    goto_priv(PRIV_HS);
    hspt_init();
    sfence_vma();

    ai_micro_cross_page_clear_mcontrol6_slot(0);
    ai_micro_cross_page_clear_mcontrol6_slot(1);
    ai_micro_cross_page_arm_mcontrol6_slot(0, trigger_pc,
        AI_MICRO_CROSS_PAGE_MCONTROL6_EXECUTE);
    ai_micro_cross_page_arm_mcontrol6_slot(1, vaddr, AI_MICRO_CROSS_PAGE_MCONTROL6_LOAD);
    uint64_t saved_mstatus = ai_micro_cross_page_enable_machine_breakpoint_delivery();

    goto_priv(PRIV_HS);
    TEST_SETUP_EXCEPT();
    ai_micro_cross_page_execute_load_once(vaddr);
    TEST_ASSERT("an HS cross-page load still reports breakpoint when both execute-PC and address trigger slots match a split load whose upper half would otherwise raise access fault",
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
    TEST_ASSERT("after clearing only the execute-PC slot, the remaining cross-page load address trigger still reports breakpoint on the same upper-half access-fault path",
        excpt.triggered == true &&
        excpt.cause == CAUSE_BKP
    );
    ai_micro_cross_page_restore_machine_status(saved_mstatus);

    goto_priv(PRIV_HS);
    hspt_init();
    sfence_vma();

    ai_micro_cross_page_clear_mcontrol6_slot(1);
    goto_priv(PRIV_HS);
    TEST_SETUP_EXCEPT();
    ai_micro_cross_page_execute_load_once(vaddr);
    TEST_ASSERT("after clearing both trigger slots, the same HS cross-page load reports load access fault with the true second-half start address",
        excpt.triggered == true &&
        excpt.cause == CAUSE_LAF &&
        excpt.tval == second_half_vaddr
    );

    ai_micro_cross_page_clear_pmp_window();

    TEST_END();
}

bool ai_micro_hs_store_cross_page_low_half_execute_and_address_trigger_page_fault_priority_corner()
{
    TEST_START();

    uintptr_t vaddr = ai_micro_cross_page_trigger_addr(VSR_GX, 4U);
    uintptr_t low_phys = ai_micro_cross_page_trigger_page_end_phys(VSR_GX);
    uintptr_t high_phys = phys_page_base(VSX_GRWX);
    uintptr_t trigger_pc = (uintptr_t)ai_micro_cross_page_execute_store_once;
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
    ai_micro_cross_page_arm_mcontrol6_slot(0, trigger_pc,
        AI_MICRO_CROSS_PAGE_MCONTROL6_EXECUTE);
    ai_micro_cross_page_arm_mcontrol6_slot(1, vaddr, AI_MICRO_CROSS_PAGE_MCONTROL6_STORE);
    uint64_t saved_mstatus = ai_micro_cross_page_enable_machine_breakpoint_delivery();

    goto_priv(PRIV_HS);
    TEST_SETUP_EXCEPT();
    ai_micro_cross_page_execute_store_once(vaddr, 0x0123456789abcdefULL);
    TEST_ASSERT("an HS cross-page store still reports breakpoint when both execute-PC and address trigger slots match a split store whose low half would otherwise raise page fault",
        excpt.triggered == true &&
        excpt.cause == CAUSE_BKP
    );

    goto_priv(PRIV_M);
    TEST_ASSERT("with both trigger slots armed, the trapped HS cross-page low-half page-fault store leaves both page fragments unchanged",
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
    TEST_ASSERT("after clearing only the execute-PC slot, the remaining cross-page store address trigger still reports breakpoint on the same low-half page-fault path",
        excpt.triggered == true &&
        excpt.cause == CAUSE_BKP
    );

    goto_priv(PRIV_M);
    TEST_ASSERT("with only the address trigger slot armed, the trapped HS cross-page low-half page-fault store still leaves both page fragments unchanged",
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
    ai_micro_cross_page_execute_store_once(vaddr, 0x0123456789abcdefULL);
    trap_triggered = excpt.triggered;
    trap_cause = excpt.cause;
    trap_tval = excpt.tval;

    goto_priv(PRIV_M);
    TEST_ASSERT("after clearing both trigger slots, the same HS cross-page store reports low-half store page fault with the original VA and still leaves both page fragments unchanged",
        trap_triggered == true &&
        trap_cause == CAUSE_SPF &&
        trap_tval == vaddr &&
        read64(low_phys) == low_pattern &&
        read64(high_phys) == high_pattern
    );

    TEST_END();
}

bool ai_micro_hs_store_cross_page_low_half_execute_trigger_with_second_half_address_slot_page_fault_spike_corner()
{
    TEST_START();

    uintptr_t vaddr = ai_micro_cross_page_trigger_addr(VSR_GX, 4U);
    uintptr_t second_half_vaddr = vaddr + 4U;
    uintptr_t low_phys = ai_micro_cross_page_trigger_page_end_phys(VSR_GX);
    uintptr_t high_phys = phys_page_base(VSX_GRWX);
    uintptr_t trigger_pc = (uintptr_t)ai_micro_cross_page_execute_store_once;
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
    ai_micro_cross_page_arm_mcontrol6_slot(0, trigger_pc,
        AI_MICRO_CROSS_PAGE_MCONTROL6_EXECUTE);
    ai_micro_cross_page_arm_mcontrol6_slot(1, second_half_vaddr,
        AI_MICRO_CROSS_PAGE_MCONTROL6_STORE);
    uint64_t saved_mstatus = ai_micro_cross_page_enable_machine_breakpoint_delivery();

    goto_priv(PRIV_HS);
    TEST_SETUP_EXCEPT();
    ai_micro_cross_page_execute_store_once(vaddr, 0x0123456789abcdefULL);
    TEST_ASSERT("with execute-PC trigger armed and the address slot programmed to the true second-half start address, Spike still reports breakpoint first on the low-half page-fault split store",
        excpt.triggered == true &&
        excpt.cause == CAUSE_BKP
    );

    goto_priv(PRIV_M);
    TEST_ASSERT("while the execute-PC slot is still armed, the second-half-start address slot does not matter and the trapped low-half page-fault split store leaves both page fragments unchanged",
        read64(low_phys) == low_pattern &&
        read64(high_phys) == high_pattern
    );
    ai_micro_cross_page_restore_machine_status(saved_mstatus);

    goto_priv(PRIV_HS);
    hspt_init();
    sfence_vma();

    ai_micro_cross_page_clear_mcontrol6_slot(0);
    goto_priv(PRIV_HS);
    TEST_SETUP_EXCEPT();
    ai_micro_cross_page_execute_store_once(vaddr, 0x0123456789abcdefULL);
    trap_triggered = excpt.triggered;
    trap_cause = excpt.cause;
    trap_tval = excpt.tval;

    goto_priv(PRIV_M);
    TEST_ASSERT("after clearing the execute-PC slot, the second-half-start address slot does not keep matching when the low half faults first; Spike falls straight back to the underlying low-half store page fault with both page fragments unchanged",
        trap_triggered == true &&
        trap_cause == CAUSE_SPF &&
        trap_tval == vaddr &&
        read64(low_phys) == low_pattern &&
        read64(high_phys) == high_pattern
    );

    ai_micro_cross_page_clear_mcontrol6_slot(1);
    TEST_END();
}

bool ai_micro_hs_store_cross_page_low_half_execute_trigger_with_second_half_address_slot_page_fault_repeated_breakpoint_then_fault_no_side_effect_spike_corner()
{
    TEST_START();

    uintptr_t vaddr = ai_micro_cross_page_trigger_addr(VSR_GX, 4U);
    uintptr_t second_half_vaddr = vaddr + 4U;
    uintptr_t low_phys = ai_micro_cross_page_trigger_page_end_phys(VSR_GX);
    uintptr_t high_phys = phys_page_base(VSX_GRWX);
    uintptr_t trigger_pc = (uintptr_t)ai_micro_cross_page_execute_store_once;
    const uint64_t low_pattern = 0x1122334455667788ULL;
    const uint64_t high_pattern = 0x99aabbccddeeff00ULL;
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
    ai_micro_cross_page_arm_mcontrol6_slot(1, second_half_vaddr,
        AI_MICRO_CROSS_PAGE_MCONTROL6_STORE);
    saved_mstatus = ai_micro_cross_page_enable_machine_breakpoint_delivery();

    goto_priv(PRIV_HS);
    TEST_SETUP_EXCEPT();
    ai_micro_cross_page_execute_store_once(vaddr, 0x0123456789abcdefULL);
    TEST_ASSERT("with execute-PC trigger armed and the address slot programmed to the true second-half start address, the first low-half page-fault cross-page split-store attempt still reports breakpoint on Spike",
        excpt.triggered == true &&
        excpt.cause == CAUSE_BKP
    );

    goto_priv(PRIV_M);
    TEST_ASSERT("the first trapped low-half page-fault cross-page split-store attempt still leaves both page fragments unchanged while only the execute-PC slot is effective on Spike",
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
    ai_micro_cross_page_execute_store_once(vaddr, 0x0123456789abcdefULL);
    TEST_ASSERT("with the same execute-PC and second-half-start slots still armed, the second low-half page-fault cross-page split-store attempt still reports breakpoint on Spike",
        excpt.triggered == true &&
        excpt.cause == CAUSE_BKP
    );

    goto_priv(PRIV_M);
    TEST_ASSERT("the second trapped low-half page-fault cross-page split-store attempt still leaves both page fragments unchanged while only the execute-PC slot is effective on Spike",
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
    TEST_ASSERT("after two prior execute-trigger breakpoint hits, clearing only the execute-PC slot still leaves the second-half-start address slot ignored when the low half faults first on Spike, so the split store falls back to the underlying low-half store page fault with no side effects",
        trap_triggered == true &&
        trap_cause == CAUSE_SPF &&
        trap_tval == vaddr &&
        read64(low_phys) == low_pattern &&
        read64(high_phys) == high_pattern
    );

    ai_micro_cross_page_clear_mcontrol6_slot(1);
    TEST_END();
}

bool ai_micro_hs_store_cross_page_low_half_execute_and_address_trigger_access_fault_priority_corner()
{
    TEST_START();

    uintptr_t vaddr = ai_micro_cross_page_trigger_addr(VSRW_GRW, 4U);
    uintptr_t low_phys = ai_micro_cross_page_trigger_page_end_phys(VSRW_GRW);
    uintptr_t high_phys = phys_page_base(VSRW_GRX);
    uintptr_t deny_start = phys_page_base(VSRW_GRW);
    uintptr_t deny_end = deny_start + PAGE_SIZE;
    uintptr_t trigger_pc = (uintptr_t)ai_micro_cross_page_execute_store_once;
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
    ai_micro_cross_page_arm_mcontrol6_slot(0, trigger_pc,
        AI_MICRO_CROSS_PAGE_MCONTROL6_EXECUTE);
    ai_micro_cross_page_arm_mcontrol6_slot(1, vaddr, AI_MICRO_CROSS_PAGE_MCONTROL6_STORE);
    uint64_t saved_mstatus = ai_micro_cross_page_enable_machine_breakpoint_delivery();

    goto_priv(PRIV_HS);
    TEST_SETUP_EXCEPT();
    ai_micro_cross_page_execute_store_once(vaddr, 0x0123456789abcdefULL);
    TEST_ASSERT("an HS cross-page store still reports breakpoint when both execute-PC and address trigger slots match a split store whose low half would otherwise raise access fault",
        excpt.triggered == true &&
        excpt.cause == CAUSE_BKP
    );

    goto_priv(PRIV_M);
    TEST_ASSERT("with both trigger slots armed, the trapped HS cross-page low-half access-fault store leaves both page fragments unchanged",
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
    TEST_ASSERT("after clearing only the execute-PC slot, the remaining cross-page store address trigger still reports breakpoint on the same low-half access-fault path",
        excpt.triggered == true &&
        excpt.cause == CAUSE_BKP
    );

    goto_priv(PRIV_M);
    TEST_ASSERT("with only the address trigger slot armed, the trapped HS cross-page low-half access-fault store still leaves both page fragments unchanged",
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
    ai_micro_cross_page_execute_store_once(vaddr, 0x0123456789abcdefULL);
    trap_triggered = excpt.triggered;
    trap_cause = excpt.cause;
    trap_tval = excpt.tval;

    goto_priv(PRIV_M);
    TEST_ASSERT("after clearing both trigger slots, the same HS cross-page store reports low-half store access fault with the original VA and still leaves both page fragments unchanged",
        trap_triggered == true &&
        trap_cause == CAUSE_SAF &&
        trap_tval == vaddr &&
        read64(low_phys) == low_pattern &&
        read64(high_phys) == high_pattern
    );

    ai_micro_cross_page_clear_pmp_window();

    TEST_END();
}

bool ai_micro_hs_store_cross_page_low_half_execute_trigger_with_second_half_address_slot_access_fault_spike_corner()
{
    TEST_START();

    uintptr_t vaddr = ai_micro_cross_page_trigger_addr(VSRW_GRW, 4U);
    uintptr_t second_half_vaddr = vaddr + 4U;
    uintptr_t low_phys = ai_micro_cross_page_trigger_page_end_phys(VSRW_GRW);
    uintptr_t high_phys = phys_page_base(VSRW_GRX);
    uintptr_t deny_start = phys_page_base(VSRW_GRW);
    uintptr_t deny_end = deny_start + PAGE_SIZE;
    uintptr_t trigger_pc = (uintptr_t)ai_micro_cross_page_execute_store_once;
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
    ai_micro_cross_page_arm_mcontrol6_slot(0, trigger_pc,
        AI_MICRO_CROSS_PAGE_MCONTROL6_EXECUTE);
    ai_micro_cross_page_arm_mcontrol6_slot(1, second_half_vaddr,
        AI_MICRO_CROSS_PAGE_MCONTROL6_STORE);
    uint64_t saved_mstatus = ai_micro_cross_page_enable_machine_breakpoint_delivery();

    goto_priv(PRIV_HS);
    TEST_SETUP_EXCEPT();
    ai_micro_cross_page_execute_store_once(vaddr, 0x0123456789abcdefULL);
    TEST_ASSERT("with execute-PC trigger armed and the address slot programmed to the true second-half start address, Spike still reports breakpoint first on the low-half access-fault split store",
        excpt.triggered == true &&
        excpt.cause == CAUSE_BKP
    );

    goto_priv(PRIV_M);
    TEST_ASSERT("while the execute-PC slot is still armed, the second-half-start address slot does not matter and the trapped low-half access-fault split store leaves both page fragments unchanged",
        read64(low_phys) == low_pattern &&
        read64(high_phys) == high_pattern
    );
    ai_micro_cross_page_restore_machine_status(saved_mstatus);

    goto_priv(PRIV_HS);
    hspt_init();
    sfence_vma();

    ai_micro_cross_page_clear_mcontrol6_slot(0);
    goto_priv(PRIV_HS);
    TEST_SETUP_EXCEPT();
    ai_micro_cross_page_execute_store_once(vaddr, 0x0123456789abcdefULL);
    trap_triggered = excpt.triggered;
    trap_cause = excpt.cause;
    trap_tval = excpt.tval;

    goto_priv(PRIV_M);
    TEST_ASSERT("after clearing the execute-PC slot, the second-half-start address slot does not keep matching when the low half faults first; Spike falls straight back to the underlying low-half store access fault with both page fragments unchanged",
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

bool ai_micro_hs_store_cross_page_low_half_execute_trigger_with_second_half_address_slot_access_fault_repeated_breakpoint_then_fault_no_side_effect_spike_corner()
{
    TEST_START();

    uintptr_t vaddr = ai_micro_cross_page_trigger_addr(VSRW_GRW, 4U);
    uintptr_t second_half_vaddr = vaddr + 4U;
    uintptr_t low_phys = ai_micro_cross_page_trigger_page_end_phys(VSRW_GRW);
    uintptr_t high_phys = phys_page_base(VSRW_GRX);
    uintptr_t deny_start = phys_page_base(VSRW_GRW);
    uintptr_t deny_end = deny_start + PAGE_SIZE;
    uintptr_t trigger_pc = (uintptr_t)ai_micro_cross_page_execute_store_once;
    const uint64_t low_pattern = 0x1122334455667788ULL;
    const uint64_t high_pattern = 0x99aabbccddeeff00ULL;
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
    ai_micro_cross_page_arm_mcontrol6_slot(1, second_half_vaddr,
        AI_MICRO_CROSS_PAGE_MCONTROL6_STORE);
    saved_mstatus = ai_micro_cross_page_enable_machine_breakpoint_delivery();

    goto_priv(PRIV_HS);
    TEST_SETUP_EXCEPT();
    ai_micro_cross_page_execute_store_once(vaddr, 0x0123456789abcdefULL);
    TEST_ASSERT("with execute-PC trigger armed and the address slot programmed to the true second-half start address, the first low-half access-fault cross-page split-store attempt still reports breakpoint on Spike",
        excpt.triggered == true &&
        excpt.cause == CAUSE_BKP
    );

    goto_priv(PRIV_M);
    TEST_ASSERT("the first trapped low-half access-fault cross-page split-store attempt still leaves both page fragments unchanged while only the execute-PC slot is effective on Spike",
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
    ai_micro_cross_page_execute_store_once(vaddr, 0x0123456789abcdefULL);
    TEST_ASSERT("with the same execute-PC and second-half-start slots still armed, the second low-half access-fault cross-page split-store attempt still reports breakpoint on Spike",
        excpt.triggered == true &&
        excpt.cause == CAUSE_BKP
    );

    goto_priv(PRIV_M);
    TEST_ASSERT("the second trapped low-half access-fault cross-page split-store attempt still leaves both page fragments unchanged while only the execute-PC slot is effective on Spike",
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
    TEST_ASSERT("after two prior execute-trigger breakpoint hits, clearing only the execute-PC slot still leaves the second-half-start address slot ignored when the low half faults first on Spike, so the split store falls back to the underlying low-half store access fault with no side effects",
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

bool ai_micro_hs_store_cross_page_high_half_execute_and_address_trigger_page_fault_no_side_effect_corner()
{
    TEST_START();

    uintptr_t vaddr = ai_micro_cross_page_trigger_addr(VSRW_GX, 4U);
    uintptr_t high_half_vaddr = hs_page_base(VSRX_GRWX);
    uintptr_t low_phys = ai_micro_cross_page_trigger_page_end_phys(VSRW_GX);
    uintptr_t high_phys = phys_page_base(VSRX_GRWX);
    uintptr_t trigger_pc = (uintptr_t)ai_micro_cross_page_execute_store_once;
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
    ai_micro_cross_page_arm_mcontrol6_slot(0, trigger_pc,
        AI_MICRO_CROSS_PAGE_MCONTROL6_EXECUTE);
    ai_micro_cross_page_arm_mcontrol6_slot(1, vaddr, AI_MICRO_CROSS_PAGE_MCONTROL6_STORE);
    uint64_t saved_mstatus = ai_micro_cross_page_enable_machine_breakpoint_delivery();

    goto_priv(PRIV_HS);
    TEST_SETUP_EXCEPT();
    ai_micro_cross_page_execute_store_once(vaddr, 0xfedcba9876543210ULL);
    TEST_ASSERT("an HS cross-page store still reports breakpoint when both execute-PC and address trigger slots match a split store whose upper half would otherwise raise page fault",
        excpt.triggered == true &&
        excpt.cause == CAUSE_BKP
    );

    goto_priv(PRIV_M);
    TEST_ASSERT("with both trigger slots armed, the trapped HS cross-page store leaves both page fragments unchanged before any high-half page fault is exposed",
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
    TEST_ASSERT("after clearing only the execute-PC slot, the remaining cross-page store address trigger still reports breakpoint on the same upper-half page-fault path",
        excpt.triggered == true &&
        excpt.cause == CAUSE_BKP
    );

    goto_priv(PRIV_M);
    TEST_ASSERT("with only the address trigger slot armed, the trapped HS cross-page store still leaves both page fragments unchanged",
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
    ai_micro_cross_page_execute_store_once(vaddr, 0xfedcba9876543210ULL);
    trap_triggered = excpt.triggered;
    trap_cause = excpt.cause;
    trap_tval = excpt.tval;

    goto_priv(PRIV_M);
    bool final_ok =
        trap_triggered == true &&
        trap_cause == CAUSE_SPF &&
        trap_tval == high_half_vaddr &&
        read64(low_phys) == expected_low_after_fault &&
        read64(high_phys) == high_pattern;
    TEST_ASSERT("after clearing both trigger slots, the same HS cross-page store falls back to Spike's upper-half page-fault corner where the store has no low-half side effect and the upper page remains unchanged",
        final_ok
    );

    TEST_END();
}

bool ai_micro_hs_store_cross_page_high_half_execute_and_address_trigger_access_fault_no_side_effect_corner()
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
    uint64_t saved_mstatus = ai_micro_cross_page_enable_machine_breakpoint_delivery();

    goto_priv(PRIV_HS);
    TEST_SETUP_EXCEPT();
    ai_micro_cross_page_execute_store_once(vaddr, 0xfedcba9876543210ULL);
    TEST_ASSERT("an HS cross-page store still reports breakpoint when both execute-PC and address trigger slots match a split store whose upper half would otherwise raise access fault",
        excpt.triggered == true &&
        excpt.cause == CAUSE_BKP
    );

    goto_priv(PRIV_M);
    TEST_ASSERT("with both trigger slots armed, the trapped HS cross-page store leaves both page fragments unchanged before any high-half access fault is exposed",
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
    TEST_ASSERT("after clearing only the execute-PC slot, the remaining cross-page store address trigger still reports breakpoint on the same upper-half access-fault path",
        excpt.triggered == true &&
        excpt.cause == CAUSE_BKP
    );

    goto_priv(PRIV_M);
    TEST_ASSERT("with only the address trigger slot armed, the trapped HS cross-page store still leaves both page fragments unchanged",
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
    bool final_ok =
        trap_triggered == true &&
        trap_cause == CAUSE_SAF &&
        trap_tval == high_half_vaddr &&
        read64(low_phys) == expected_low_after_fault &&
        read64(deny_start) == high_pattern;
    TEST_ASSERT("after clearing both trigger slots, the same HS cross-page store falls back to Spike's upper-half access-fault corner where the store has no low-half side effect and the protected upper page remains unchanged",
        final_ok
    );

    ai_micro_cross_page_clear_pmp_window();

    TEST_END();
}

bool ai_micro_hs_load_cross_page_low_half_execute_and_address_trigger_page_fault_repeated_breakpoint_then_fault_corner()
{
    TEST_START();

    uintptr_t vaddr = ai_micro_cross_page_trigger_addr(VSX_GX, 4U);
    uintptr_t trigger_pc = (uintptr_t)ai_micro_cross_page_execute_load_once;
    uint64_t saved_mstatus;

    goto_priv(PRIV_HS);
    hspt_init();
    sfence_vma();

    ai_micro_cross_page_clear_mcontrol6_slot(0);
    ai_micro_cross_page_clear_mcontrol6_slot(1);
    ai_micro_cross_page_arm_mcontrol6_slot(0, trigger_pc,
        AI_MICRO_CROSS_PAGE_MCONTROL6_EXECUTE);
    ai_micro_cross_page_arm_mcontrol6_slot(1, vaddr, AI_MICRO_CROSS_PAGE_MCONTROL6_LOAD);
    saved_mstatus = ai_micro_cross_page_enable_machine_breakpoint_delivery();

    goto_priv(PRIV_HS);
    TEST_SETUP_EXCEPT();
    ai_micro_cross_page_execute_load_once(vaddr);
    TEST_ASSERT("with both execute-PC and address trigger slots armed on the same cross-page low-half page-fault split load, the first access still reports breakpoint",
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
    TEST_ASSERT("with both execute-PC and address trigger slots still armed on that same low-half page-fault split load, the second access still reports breakpoint",
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
    TEST_ASSERT("after two prior dual-slot breakpoint hits, clearing only the execute-PC slot still leaves the cross-page load address trigger active on the low-half page-fault path",
        excpt.triggered == true &&
        excpt.cause == CAUSE_BKP
    );
    ai_micro_cross_page_restore_machine_status(saved_mstatus);

    goto_priv(PRIV_HS);
    hspt_init();
    sfence_vma();

    ai_micro_cross_page_clear_mcontrol6_slot(1);
    goto_priv(PRIV_HS);
    TEST_SETUP_EXCEPT();
    ai_micro_cross_page_execute_load_once(vaddr);
    TEST_ASSERT("after clearing both trigger slots, the same cross-page load falls back to low-half load page fault with the original faulting VA",
        excpt.triggered == true &&
        excpt.cause == CAUSE_LPF &&
        excpt.tval == vaddr
    );

    TEST_END();
}

bool ai_micro_hs_load_cross_page_low_half_execute_and_address_trigger_page_fault_repeated_breakpoint_then_fault_adjacent_preserved_corner()
{
    TEST_START();

    enum test_page low_page = VSX_GX;
    uintptr_t vaddr = ai_micro_cross_page_trigger_addr(low_page, 4U);
    uintptr_t low_adjacent_phys = phys_page_base(low_page) + PAGE_SIZE - (2 * sizeof(uint64_t));
    uintptr_t low_phys = ai_micro_cross_page_trigger_page_end_phys(low_page);
    uintptr_t sentinel_phys = phys_page_base(VSRWX_GRWX);
    uintptr_t sentinel_adjacent_phys = sentinel_phys + sizeof(uint64_t);
    uintptr_t trigger_pc = (uintptr_t)ai_micro_cross_page_execute_load_once;
    const uint64_t low_adjacent = 0x55aa55aa33cc33ccULL;
    const uint64_t low_pattern = 0x8798a9bacbdcedfeULL;
    const uint64_t sentinel = 0x1234432155aa55aaULL;
    const uint64_t sentinel_adjacent = 0xaa55aa5512344321ULL;
    uint64_t saved_mstatus;

    goto_priv(PRIV_M);
    write64(low_adjacent_phys, low_adjacent);
    write64(low_phys, low_pattern);
    write64(sentinel_phys, sentinel);
    write64(sentinel_adjacent_phys, sentinel_adjacent);

    goto_priv(PRIV_HS);
    hspt_init();
    sfence_vma();

    ai_micro_cross_page_clear_mcontrol6_slot(0);
    ai_micro_cross_page_clear_mcontrol6_slot(1);
    ai_micro_cross_page_arm_mcontrol6_slot(0, trigger_pc,
        AI_MICRO_CROSS_PAGE_MCONTROL6_EXECUTE);
    ai_micro_cross_page_arm_mcontrol6_slot(1, vaddr, AI_MICRO_CROSS_PAGE_MCONTROL6_LOAD);
    saved_mstatus = ai_micro_cross_page_enable_machine_breakpoint_delivery();

    goto_priv(PRIV_HS);
    TEST_SETUP_EXCEPT();
    ai_micro_cross_page_execute_load_once(vaddr);
    TEST_ASSERT("with both execute-PC and address trigger slots armed on the same cross-page low-half page-fault split load, the first access still reports breakpoint in the adjacent-preservation corner",
        excpt.triggered == true &&
        excpt.cause == CAUSE_BKP
    );
    ai_micro_cross_page_restore_machine_status(saved_mstatus);

    goto_priv(PRIV_M);
    TEST_ASSERT("the first trapped low-half page-fault split load still leaves the faulting low-page tail word, its adjacent word, and an unrelated sentinel pair unchanged",
        read64(low_adjacent_phys) == low_adjacent &&
        read64(low_phys) == low_pattern &&
        read64(sentinel_phys) == sentinel &&
        read64(sentinel_adjacent_phys) == sentinel_adjacent
    );

    goto_priv(PRIV_HS);
    hspt_init();
    sfence_vma();

    saved_mstatus = ai_micro_cross_page_enable_machine_breakpoint_delivery();
    goto_priv(PRIV_HS);
    TEST_SETUP_EXCEPT();
    ai_micro_cross_page_execute_load_once(vaddr);
    TEST_ASSERT("with both execute-PC and address trigger slots still armed on that same low-half page-fault split load, the second access still reports breakpoint in the adjacent-preservation corner",
        excpt.triggered == true &&
        excpt.cause == CAUSE_BKP
    );
    ai_micro_cross_page_restore_machine_status(saved_mstatus);

    goto_priv(PRIV_M);
    TEST_ASSERT("the second trapped low-half page-fault split load still leaves the faulting low-page tail word, its adjacent word, and the sentinel pair unchanged",
        read64(low_adjacent_phys) == low_adjacent &&
        read64(low_phys) == low_pattern &&
        read64(sentinel_phys) == sentinel &&
        read64(sentinel_adjacent_phys) == sentinel_adjacent
    );

    goto_priv(PRIV_HS);
    hspt_init();
    sfence_vma();

    ai_micro_cross_page_clear_mcontrol6_slot(0);
    saved_mstatus = ai_micro_cross_page_enable_machine_breakpoint_delivery();
    goto_priv(PRIV_HS);
    TEST_SETUP_EXCEPT();
    ai_micro_cross_page_execute_load_once(vaddr);
    TEST_ASSERT("after two prior dual-slot breakpoint hits, clearing only the execute-PC slot still leaves the cross-page load address trigger active on the low-half page-fault path in the adjacent-preservation corner",
        excpt.triggered == true &&
        excpt.cause == CAUSE_BKP
    );
    ai_micro_cross_page_restore_machine_status(saved_mstatus);

    goto_priv(PRIV_M);
    TEST_ASSERT("with only the address trigger slot armed, the trapped low-half page-fault split load still leaves the low-page tail word, its adjacent word, and the sentinel pair unchanged",
        read64(low_adjacent_phys) == low_adjacent &&
        read64(low_phys) == low_pattern &&
        read64(sentinel_phys) == sentinel &&
        read64(sentinel_adjacent_phys) == sentinel_adjacent
    );

    goto_priv(PRIV_HS);
    hspt_init();
    sfence_vma();

    ai_micro_cross_page_clear_mcontrol6_slot(1);
    goto_priv(PRIV_HS);
    TEST_SETUP_EXCEPT();
    ai_micro_cross_page_execute_load_once(vaddr);
    TEST_ASSERT("after clearing both trigger slots, the same cross-page load falls back to low-half load page fault with the original faulting VA in the adjacent-preservation corner",
        excpt.triggered == true &&
        excpt.cause == CAUSE_LPF &&
        excpt.tval == vaddr
    );

    goto_priv(PRIV_M);
    TEST_ASSERT("the fallback low-half load page-fault path still leaves the low-page tail word, its adjacent word, and the sentinel pair unchanged",
        read64(low_adjacent_phys) == low_adjacent &&
        read64(low_phys) == low_pattern &&
        read64(sentinel_phys) == sentinel &&
        read64(sentinel_adjacent_phys) == sentinel_adjacent
    );

    TEST_END();
}

bool ai_micro_hs_load_cross_page_low_half_execute_and_address_trigger_access_fault_repeated_breakpoint_then_fault_corner()
{
    TEST_START();

    uintptr_t vaddr = ai_micro_cross_page_trigger_addr(VSRWX_GRX, 4U);
    uintptr_t trigger_pc = (uintptr_t)ai_micro_cross_page_execute_load_once;
    uintptr_t deny_start = phys_page_base(VSRWX_GRX);
    uintptr_t deny_end = deny_start + PAGE_SIZE;
    uint64_t saved_mstatus;

    ai_micro_cross_page_configure_pmp_window(deny_start, deny_end, AI_MICRO_PMP_TOR_NO_R);

    goto_priv(PRIV_HS);
    hspt_init();
    sfence_vma();

    ai_micro_cross_page_clear_mcontrol6_slot(0);
    ai_micro_cross_page_clear_mcontrol6_slot(1);
    ai_micro_cross_page_arm_mcontrol6_slot(0, trigger_pc,
        AI_MICRO_CROSS_PAGE_MCONTROL6_EXECUTE);
    ai_micro_cross_page_arm_mcontrol6_slot(1, vaddr, AI_MICRO_CROSS_PAGE_MCONTROL6_LOAD);
    saved_mstatus = ai_micro_cross_page_enable_machine_breakpoint_delivery();

    goto_priv(PRIV_HS);
    TEST_SETUP_EXCEPT();
    ai_micro_cross_page_execute_load_once(vaddr);
    TEST_ASSERT("with both execute-PC and address trigger slots armed on the same cross-page low-half access-fault split load, the first access still reports breakpoint",
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
    TEST_ASSERT("with both execute-PC and address trigger slots still armed on that same low-half access-fault split load, the second access still reports breakpoint",
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
    TEST_ASSERT("after two prior dual-slot breakpoint hits, clearing only the execute-PC slot still leaves the cross-page load address trigger active on the low-half access-fault path",
        excpt.triggered == true &&
        excpt.cause == CAUSE_BKP
    );
    ai_micro_cross_page_restore_machine_status(saved_mstatus);

    goto_priv(PRIV_HS);
    hspt_init();
    sfence_vma();

    ai_micro_cross_page_clear_mcontrol6_slot(1);
    goto_priv(PRIV_HS);
    TEST_SETUP_EXCEPT();
    ai_micro_cross_page_execute_load_once(vaddr);
    TEST_ASSERT("after clearing both trigger slots, the same cross-page load falls back to low-half load access fault with the original faulting VA",
        excpt.triggered == true &&
        excpt.cause == CAUSE_LAF &&
        excpt.tval == vaddr
    );

    ai_micro_cross_page_clear_pmp_window();

    TEST_END();
}

bool ai_micro_hs_load_cross_page_low_half_execute_and_address_trigger_access_fault_repeated_breakpoint_then_fault_adjacent_preserved_corner()
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
    uintptr_t deny_start = phys_page_base(low_page);
    uintptr_t deny_end = deny_start + PAGE_SIZE;
    const uint64_t low_adjacent = 0x0badc0de55aa1234ULL;
    const uint64_t low_pattern = 0x98a9bacbdcedfe10ULL;
    const uint64_t high_pattern = 0x21436587a9cbed0fULL;
    const uint64_t high_adjacent = 0x1234fedc5678ba90ULL;
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
    ai_micro_cross_page_arm_mcontrol6_slot(0, trigger_pc,
        AI_MICRO_CROSS_PAGE_MCONTROL6_EXECUTE);
    ai_micro_cross_page_arm_mcontrol6_slot(1, vaddr, AI_MICRO_CROSS_PAGE_MCONTROL6_LOAD);
    saved_mstatus = ai_micro_cross_page_enable_machine_breakpoint_delivery();

    goto_priv(PRIV_HS);
    TEST_SETUP_EXCEPT();
    ai_micro_cross_page_execute_load_once(vaddr);
    TEST_ASSERT("with both execute-PC and address trigger slots armed on the same cross-page low-half access-fault split load, the first access still reports breakpoint in the adjacent-preservation corner",
        excpt.triggered == true &&
        excpt.cause == CAUSE_BKP
    );
    ai_micro_cross_page_restore_machine_status(saved_mstatus);

    goto_priv(PRIV_M);
    TEST_ASSERT("the first trapped low-half access-fault split load still leaves both boundary doublewords and their adjacent neighbors unchanged",
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
    TEST_ASSERT("with both execute-PC and address trigger slots still armed on that same low-half access-fault split load, the second access still reports breakpoint in the adjacent-preservation corner",
        excpt.triggered == true &&
        excpt.cause == CAUSE_BKP
    );
    ai_micro_cross_page_restore_machine_status(saved_mstatus);

    goto_priv(PRIV_M);
    TEST_ASSERT("the second trapped low-half access-fault split load still leaves both boundary doublewords and their adjacent neighbors unchanged",
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
    TEST_ASSERT("after two prior dual-slot breakpoint hits, clearing only the execute-PC slot still leaves the cross-page load address trigger active on the low-half access-fault path in the adjacent-preservation corner",
        excpt.triggered == true &&
        excpt.cause == CAUSE_BKP
    );
    ai_micro_cross_page_restore_machine_status(saved_mstatus);

    goto_priv(PRIV_M);
    TEST_ASSERT("with only the address trigger slot armed, the trapped low-half access-fault split load still leaves both boundary doublewords and their adjacent neighbors unchanged",
        read64(low_adjacent_phys) == low_adjacent &&
        read64(low_phys) == low_pattern &&
        read64(high_phys) == high_pattern &&
        read64(high_adjacent_phys) == high_adjacent
    );

    goto_priv(PRIV_HS);
    hspt_init();
    sfence_vma();

    ai_micro_cross_page_clear_mcontrol6_slot(1);
    goto_priv(PRIV_HS);
    TEST_SETUP_EXCEPT();
    ai_micro_cross_page_execute_load_once(vaddr);
    TEST_ASSERT("after clearing both trigger slots, the same cross-page load falls back to low-half load access fault with the original faulting VA in the adjacent-preservation corner",
        excpt.triggered == true &&
        excpt.cause == CAUSE_LAF &&
        excpt.tval == vaddr
    );

    goto_priv(PRIV_M);
    TEST_ASSERT("the fallback low-half load access-fault path still leaves both boundary doublewords and their adjacent neighbors unchanged",
        read64(low_adjacent_phys) == low_adjacent &&
        read64(low_phys) == low_pattern &&
        read64(high_phys) == high_pattern &&
        read64(high_adjacent_phys) == high_adjacent
    );

    ai_micro_cross_page_clear_pmp_window();

    TEST_END();
}

bool ai_micro_hs_load_cross_page_high_half_execute_and_address_trigger_page_fault_repeated_breakpoint_then_fault_corner()
{
    TEST_START();

    uintptr_t vaddr = ai_micro_cross_page_trigger_addr(VSR_GX, 4U);
    uintptr_t second_half_vaddr = vaddr + 4U;
    uintptr_t trigger_pc = (uintptr_t)ai_micro_cross_page_execute_load_once;
    uint64_t saved_mstatus;

    goto_priv(PRIV_HS);
    hspt_init();
    sfence_vma();

    ai_micro_cross_page_clear_mcontrol6_slot(0);
    ai_micro_cross_page_clear_mcontrol6_slot(1);
    ai_micro_cross_page_arm_mcontrol6_slot(0, trigger_pc,
        AI_MICRO_CROSS_PAGE_MCONTROL6_EXECUTE);
    ai_micro_cross_page_arm_mcontrol6_slot(1, vaddr, AI_MICRO_CROSS_PAGE_MCONTROL6_LOAD);
    saved_mstatus = ai_micro_cross_page_enable_machine_breakpoint_delivery();

    goto_priv(PRIV_HS);
    TEST_SETUP_EXCEPT();
    ai_micro_cross_page_execute_load_once(vaddr);
    TEST_ASSERT("with both execute-PC and address trigger slots armed on the same cross-page high-half page-fault split load, the first access still reports breakpoint",
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
    TEST_ASSERT("with both execute-PC and address trigger slots still armed on that same high-half page-fault split load, the second access still reports breakpoint",
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
    TEST_ASSERT("after two prior dual-slot breakpoint hits, clearing only the execute-PC slot still leaves the cross-page load address trigger active on the high-half page-fault path",
        excpt.triggered == true &&
        excpt.cause == CAUSE_BKP
    );
    ai_micro_cross_page_restore_machine_status(saved_mstatus);

    goto_priv(PRIV_HS);
    hspt_init();
    sfence_vma();

    ai_micro_cross_page_clear_mcontrol6_slot(1);
    goto_priv(PRIV_HS);
    TEST_SETUP_EXCEPT();
    ai_micro_cross_page_execute_load_once(vaddr);
    TEST_ASSERT("after clearing both trigger slots, the same cross-page load falls back to high-half load page fault with the true second-half start address",
        excpt.triggered == true &&
        excpt.cause == CAUSE_LPF &&
        excpt.tval == second_half_vaddr
    );

    TEST_END();
}

bool ai_micro_hs_load_cross_page_high_half_execute_and_address_trigger_page_fault_repeated_breakpoint_then_fault_adjacent_preserved_corner()
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
    uintptr_t trigger_pc = (uintptr_t)ai_micro_cross_page_execute_load_once;
    const uint64_t low_adjacent = 0x55aa55aa33cc33ccULL;
    const uint64_t low_pattern = 0x8798a9bacbdcedfeULL;
    const uint64_t high_pattern = 0x1032547698badcfeULL;
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
    ai_micro_cross_page_arm_mcontrol6_slot(1, vaddr, AI_MICRO_CROSS_PAGE_MCONTROL6_LOAD);
    saved_mstatus = ai_micro_cross_page_enable_machine_breakpoint_delivery();

    goto_priv(PRIV_HS);
    TEST_SETUP_EXCEPT();
    ai_micro_cross_page_execute_load_once(vaddr);
    TEST_ASSERT("with both execute-PC and address trigger slots armed on the same cross-page high-half page-fault split load, the first access still reports breakpoint in the adjacent-preservation corner",
        excpt.triggered == true &&
        excpt.cause == CAUSE_BKP
    );
    ai_micro_cross_page_restore_machine_status(saved_mstatus);

    goto_priv(PRIV_M);
    TEST_ASSERT("the first trapped high-half page-fault split load still leaves both boundary doublewords and their adjacent neighbors unchanged in the adjacent-preservation corner",
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
    TEST_ASSERT("with both execute-PC and address trigger slots still armed on that same high-half page-fault split load, the second access still reports breakpoint in the adjacent-preservation corner",
        excpt.triggered == true &&
        excpt.cause == CAUSE_BKP
    );
    ai_micro_cross_page_restore_machine_status(saved_mstatus);

    goto_priv(PRIV_M);
    TEST_ASSERT("the second trapped high-half page-fault split load still leaves both boundary doublewords and their adjacent neighbors unchanged in the adjacent-preservation corner",
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
    TEST_ASSERT("after two prior dual-slot breakpoint hits, clearing only the execute-PC slot still leaves the cross-page load address trigger active on the high-half page-fault path in the adjacent-preservation corner",
        excpt.triggered == true &&
        excpt.cause == CAUSE_BKP
    );
    ai_micro_cross_page_restore_machine_status(saved_mstatus);

    goto_priv(PRIV_M);
    TEST_ASSERT("with only the address trigger slot armed, the trapped high-half page-fault split load still leaves both boundary doublewords and their adjacent neighbors unchanged in the adjacent-preservation corner",
        read64(low_adjacent_phys) == low_adjacent &&
        read64(low_phys) == low_pattern &&
        read64(high_phys) == high_pattern &&
        read64(high_adjacent_phys) == high_adjacent
    );

    goto_priv(PRIV_HS);
    hspt_init();
    sfence_vma();

    ai_micro_cross_page_clear_mcontrol6_slot(1);
    goto_priv(PRIV_HS);
    TEST_SETUP_EXCEPT();
    ai_micro_cross_page_execute_load_once(vaddr);
    TEST_ASSERT("after clearing both trigger slots, the same cross-page load falls back to high-half load page fault with the true second-half start address in the adjacent-preservation corner",
        excpt.triggered == true &&
        excpt.cause == CAUSE_LPF &&
        excpt.tval == second_half_vaddr
    );

    goto_priv(PRIV_M);
    TEST_ASSERT("the fallback high-half load page-fault path still leaves both boundary doublewords and their adjacent neighbors unchanged in the adjacent-preservation corner",
        read64(low_adjacent_phys) == low_adjacent &&
        read64(low_phys) == low_pattern &&
        read64(high_phys) == high_pattern &&
        read64(high_adjacent_phys) == high_adjacent
    );

    TEST_END();
}

bool ai_micro_hs_load_cross_page_high_half_execute_and_address_trigger_page_fault_repeated_breakpoint_then_fix_high_half_recovers_corner()
{
    TEST_START();

    enum test_page low_page = VSR_GX;
    enum test_page high_page = VSW_GI;
    uintptr_t vaddr = ai_micro_cross_page_trigger_addr(low_page, 4U);
    uintptr_t second_half_vaddr = vaddr + 4U;
    uintptr_t low_phys = ai_micro_cross_page_trigger_page_end_phys(low_page);
    uintptr_t high_phys = phys_page_base(high_page);
    uintptr_t trigger_pc = (uintptr_t)ai_micro_cross_page_execute_load_once;
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
    ai_micro_cross_page_arm_mcontrol6_slot(0, trigger_pc,
        AI_MICRO_CROSS_PAGE_MCONTROL6_EXECUTE);
    ai_micro_cross_page_arm_mcontrol6_slot(1, vaddr, AI_MICRO_CROSS_PAGE_MCONTROL6_LOAD);
    saved_mstatus = ai_micro_cross_page_enable_machine_breakpoint_delivery();

    goto_priv(PRIV_HS);
    TEST_SETUP_EXCEPT();
    ai_micro_cross_page_execute_load_once(vaddr);
    TEST_ASSERT("with both execute-PC and address trigger slots armed on the same cross-page high-half page-fault split load, the first access still reports breakpoint before any later repair",
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
    TEST_ASSERT("with both execute-PC and address trigger slots still armed on that same high-half page-fault split load, the second access still reports breakpoint before any later repair",
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
    TEST_ASSERT("after two prior dual-slot breakpoint hits, clearing only the execute-PC slot still leaves the cross-page load address trigger active on the high-half page-fault path before repair",
        excpt.triggered == true &&
        excpt.cause == CAUSE_BKP
    );
    ai_micro_cross_page_restore_machine_status(saved_mstatus);

    goto_priv(PRIV_HS);
    hspt_init();
    sfence_vma();

    ai_micro_cross_page_clear_mcontrol6_slot(1);
    goto_priv(PRIV_HS);
    TEST_SETUP_EXCEPT();
    ai_micro_cross_page_execute_load_once(vaddr);
    TEST_ASSERT("after clearing both trigger slots, the same cross-page load first falls back to high-half load page fault with the true second-half start address before repair",
        excpt.triggered == true &&
        excpt.cause == CAUSE_LPF &&
        excpt.tval == second_half_vaddr
    );

    goto_priv(PRIV_M);
    TEST_ASSERT("before repairing the upper leaf, repeated trigger hits and the fallback high-half load page fault leave both boundary backing doublewords unchanged",
        read64(low_phys) == low_pattern &&
        read64(high_phys) == high_pattern
    );
    ai_micro_cross_page_set_hs_leaf(high_page, phys_page_base(high_page),
        PTE_V | PTE_AD | PTE_RWX);
    sfence_vma();

    goto_priv(PRIV_HS);
    TEST_SETUP_EXCEPT();
    uint64_t observed = ai_micro_cross_page_execute_load_once(vaddr);
    TEST_ASSERT("after repeated dual-slot breakpoint hits on the high-half page-fault split load, fixing the upper leaf lets the same split load recover and return the merged payload",
        excpt.triggered == false &&
        observed == recovery_expected,
        "triggered=%d cause=0x%llx tval=0x%llx observed=0x%llx expected=0x%llx",
        excpt.triggered, excpt.cause, excpt.tval, observed, recovery_expected
    );

    goto_priv(PRIV_HS);
    hspt_init();
    sfence_vma();

    TEST_END();
}

bool ai_micro_hs_load_cross_page_high_half_execute_and_address_trigger_page_fault_repeated_breakpoint_then_fix_high_half_adjacent_preserved_corner()
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
    uintptr_t trigger_pc = (uintptr_t)ai_micro_cross_page_execute_load_once;
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
    ai_micro_cross_page_arm_mcontrol6_slot(0, trigger_pc,
        AI_MICRO_CROSS_PAGE_MCONTROL6_EXECUTE);
    ai_micro_cross_page_arm_mcontrol6_slot(1, vaddr, AI_MICRO_CROSS_PAGE_MCONTROL6_LOAD);
    saved_mstatus = ai_micro_cross_page_enable_machine_breakpoint_delivery();

    goto_priv(PRIV_HS);
    TEST_SETUP_EXCEPT();
    ai_micro_cross_page_execute_load_once(vaddr);
    TEST_ASSERT("with both execute-PC and address trigger slots armed on the same cross-page high-half page-fault split load, the first access still reports breakpoint before any later repair in the adjacent-preservation corner",
        excpt.triggered == true &&
        excpt.cause == CAUSE_BKP
    );
    ai_micro_cross_page_restore_machine_status(saved_mstatus);

    goto_priv(PRIV_M);
    TEST_ASSERT("the first trapped high-half page-fault split load still leaves both boundary doublewords and their adjacent neighbors unchanged before any later repair",
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
    TEST_ASSERT("with both execute-PC and address trigger slots still armed on that same high-half page-fault split load, the second access still reports breakpoint before any later repair in the adjacent-preservation corner",
        excpt.triggered == true &&
        excpt.cause == CAUSE_BKP
    );
    ai_micro_cross_page_restore_machine_status(saved_mstatus);

    goto_priv(PRIV_M);
    TEST_ASSERT("the second trapped high-half page-fault split load still leaves both boundary doublewords and their adjacent neighbors unchanged before any later repair",
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
    TEST_ASSERT("after two prior dual-slot breakpoint hits, clearing only the execute-PC slot still leaves the cross-page load address trigger active on the high-half page-fault path before repair in the adjacent-preservation corner",
        excpt.triggered == true &&
        excpt.cause == CAUSE_BKP
    );
    ai_micro_cross_page_restore_machine_status(saved_mstatus);

    goto_priv(PRIV_M);
    TEST_ASSERT("with only the address trigger slot armed, the trapped high-half page-fault split load still leaves both boundary doublewords and their adjacent neighbors unchanged",
        read64(low_adjacent_phys) == low_adjacent &&
        read64(low_phys) == low_pattern &&
        read64(high_phys) == high_pattern &&
        read64(high_adjacent_phys) == high_adjacent
    );

    goto_priv(PRIV_HS);
    hspt_init();
    sfence_vma();

    ai_micro_cross_page_clear_mcontrol6_slot(1);
    goto_priv(PRIV_HS);
    TEST_SETUP_EXCEPT();
    ai_micro_cross_page_execute_load_once(vaddr);
    TEST_ASSERT("after clearing both trigger slots, the same cross-page load first falls back to high-half load page fault with the true second-half start address before repair in the adjacent-preservation corner",
        excpt.triggered == true &&
        excpt.cause == CAUSE_LPF &&
        excpt.tval == second_half_vaddr
    );

    goto_priv(PRIV_M);
    TEST_ASSERT("before repairing the upper leaf, repeated trigger hits and the fallback high-half load page fault leave both boundary backing doublewords and their adjacent neighbors unchanged",
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
    TEST_ASSERT("after repeated dual-slot breakpoint hits on the high-half page-fault split load, fixing the upper leaf lets the same split load recover and return the merged payload while keeping adjacent words untouched",
        excpt.triggered == false &&
        observed == recovery_expected,
        "triggered=%d cause=0x%llx tval=0x%llx observed=0x%llx expected=0x%llx",
        excpt.triggered, excpt.cause, excpt.tval, observed, recovery_expected
    );

    goto_priv(PRIV_M);
    TEST_ASSERT("the recovered split-load path reads the merged payload without dirtying either adjacent boundary word",
        read64(low_adjacent_phys) == low_adjacent &&
        read64(low_phys) == low_pattern &&
        read64(high_phys) == high_pattern &&
        read64(high_adjacent_phys) == high_adjacent
    );

    goto_priv(PRIV_HS);
    hspt_init();
    sfence_vma();

    TEST_END();
}

bool ai_micro_hs_load_cross_page_high_half_execute_and_address_trigger_access_fault_repeated_breakpoint_then_fault_corner()
{
    TEST_START();

    uintptr_t vaddr = ai_micro_cross_page_trigger_addr(VSRWX_GRX, 4U);
    uintptr_t second_half_vaddr = vaddr + 4U;
    uintptr_t trigger_pc = (uintptr_t)ai_micro_cross_page_execute_load_once;
    uintptr_t deny_start = phys_page_base(VSRWX_GR);
    uintptr_t deny_end = deny_start + PAGE_SIZE;
    uint64_t saved_mstatus;

    ai_micro_cross_page_configure_pmp_window(deny_start, deny_end, AI_MICRO_PMP_TOR_NO_R);

    goto_priv(PRIV_HS);
    hspt_init();
    sfence_vma();

    ai_micro_cross_page_clear_mcontrol6_slot(0);
    ai_micro_cross_page_clear_mcontrol6_slot(1);
    ai_micro_cross_page_arm_mcontrol6_slot(0, trigger_pc,
        AI_MICRO_CROSS_PAGE_MCONTROL6_EXECUTE);
    ai_micro_cross_page_arm_mcontrol6_slot(1, vaddr, AI_MICRO_CROSS_PAGE_MCONTROL6_LOAD);
    saved_mstatus = ai_micro_cross_page_enable_machine_breakpoint_delivery();

    goto_priv(PRIV_HS);
    TEST_SETUP_EXCEPT();
    ai_micro_cross_page_execute_load_once(vaddr);
    TEST_ASSERT("with both execute-PC and address trigger slots armed on the same cross-page high-half access-fault split load, the first access still reports breakpoint",
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
    TEST_ASSERT("with both execute-PC and address trigger slots still armed on that same high-half access-fault split load, the second access still reports breakpoint",
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
    TEST_ASSERT("after two prior dual-slot breakpoint hits, clearing only the execute-PC slot still leaves the cross-page load address trigger active on the high-half access-fault path",
        excpt.triggered == true &&
        excpt.cause == CAUSE_BKP
    );
    ai_micro_cross_page_restore_machine_status(saved_mstatus);

    goto_priv(PRIV_HS);
    hspt_init();
    sfence_vma();

    ai_micro_cross_page_clear_mcontrol6_slot(1);
    goto_priv(PRIV_HS);
    TEST_SETUP_EXCEPT();
    ai_micro_cross_page_execute_load_once(vaddr);
    TEST_ASSERT("after clearing both trigger slots, the same cross-page load falls back to high-half load access fault with the true second-half start address",
        excpt.triggered == true &&
        excpt.cause == CAUSE_LAF &&
        excpt.tval == second_half_vaddr
    );

    ai_micro_cross_page_clear_pmp_window();

    TEST_END();
}

bool ai_micro_hs_load_cross_page_high_half_execute_and_address_trigger_access_fault_repeated_breakpoint_then_fault_adjacent_preserved_corner()
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
    uintptr_t trigger_pc = (uintptr_t)ai_micro_cross_page_execute_load_once;
    uintptr_t deny_start = phys_page_base(high_page);
    uintptr_t deny_end = deny_start + PAGE_SIZE;
    const uint64_t low_adjacent = 0x1234432155aa55aaULL;
    const uint64_t low_pattern = 0x98a9bacbdcedfe10ULL;
    const uint64_t high_pattern = 0x21436587a9cbed0fULL;
    const uint64_t high_adjacent = 0xaa55aa5512344321ULL;
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
    ai_micro_cross_page_arm_mcontrol6_slot(0, trigger_pc,
        AI_MICRO_CROSS_PAGE_MCONTROL6_EXECUTE);
    ai_micro_cross_page_arm_mcontrol6_slot(1, vaddr, AI_MICRO_CROSS_PAGE_MCONTROL6_LOAD);
    saved_mstatus = ai_micro_cross_page_enable_machine_breakpoint_delivery();

    goto_priv(PRIV_HS);
    TEST_SETUP_EXCEPT();
    ai_micro_cross_page_execute_load_once(vaddr);
    TEST_ASSERT("with both execute-PC and address trigger slots armed on the same cross-page high-half access-fault split load, the first access still reports breakpoint in the adjacent-preservation corner",
        excpt.triggered == true &&
        excpt.cause == CAUSE_BKP
    );
    ai_micro_cross_page_restore_machine_status(saved_mstatus);

    goto_priv(PRIV_M);
    TEST_ASSERT("the first trapped high-half access-fault split load still leaves both boundary doublewords and their adjacent neighbors unchanged in the adjacent-preservation corner",
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
    TEST_ASSERT("with both execute-PC and address trigger slots still armed on that same high-half access-fault split load, the second access still reports breakpoint in the adjacent-preservation corner",
        excpt.triggered == true &&
        excpt.cause == CAUSE_BKP
    );
    ai_micro_cross_page_restore_machine_status(saved_mstatus);

    goto_priv(PRIV_M);
    TEST_ASSERT("the second trapped high-half access-fault split load still leaves both boundary doublewords and their adjacent neighbors unchanged in the adjacent-preservation corner",
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
    TEST_ASSERT("after two prior dual-slot breakpoint hits, clearing only the execute-PC slot still leaves the cross-page load address trigger active on the high-half access-fault path in the adjacent-preservation corner",
        excpt.triggered == true &&
        excpt.cause == CAUSE_BKP
    );
    ai_micro_cross_page_restore_machine_status(saved_mstatus);

    goto_priv(PRIV_M);
    TEST_ASSERT("with only the address trigger slot armed, the trapped high-half access-fault split load still leaves both boundary doublewords and their adjacent neighbors unchanged in the adjacent-preservation corner",
        read64(low_adjacent_phys) == low_adjacent &&
        read64(low_phys) == low_pattern &&
        read64(high_phys) == high_pattern &&
        read64(high_adjacent_phys) == high_adjacent
    );

    goto_priv(PRIV_HS);
    hspt_init();
    sfence_vma();

    ai_micro_cross_page_clear_mcontrol6_slot(1);
    goto_priv(PRIV_HS);
    TEST_SETUP_EXCEPT();
    ai_micro_cross_page_execute_load_once(vaddr);
    TEST_ASSERT("after clearing both trigger slots, the same cross-page load falls back to high-half load access fault with the true second-half start address in the adjacent-preservation corner",
        excpt.triggered == true &&
        excpt.cause == CAUSE_LAF &&
        excpt.tval == second_half_vaddr
    );

    goto_priv(PRIV_M);
    TEST_ASSERT("the fallback high-half load access-fault path still leaves both boundary doublewords and their adjacent neighbors unchanged in the adjacent-preservation corner",
        read64(low_adjacent_phys) == low_adjacent &&
        read64(low_phys) == low_pattern &&
        read64(high_phys) == high_pattern &&
        read64(high_adjacent_phys) == high_adjacent
    );

    ai_micro_cross_page_clear_pmp_window();

    TEST_END();
}

bool ai_micro_hs_load_cross_page_high_half_execute_and_address_trigger_access_fault_repeated_breakpoint_then_restore_pmp_recovers_corner()
{
    TEST_START();

    enum test_page low_page = VSRWX_GRX;
    enum test_page high_page = VSRWX_GR;
    uintptr_t vaddr = ai_micro_cross_page_trigger_addr(low_page, 4U);
    uintptr_t second_half_vaddr = vaddr + 4U;
    uintptr_t low_phys = ai_micro_cross_page_trigger_page_end_phys(low_page);
    uintptr_t high_phys = phys_page_base(high_page);
    uintptr_t trigger_pc = (uintptr_t)ai_micro_cross_page_execute_load_once;
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
    ai_micro_cross_page_arm_mcontrol6_slot(0, trigger_pc,
        AI_MICRO_CROSS_PAGE_MCONTROL6_EXECUTE);
    ai_micro_cross_page_arm_mcontrol6_slot(1, vaddr, AI_MICRO_CROSS_PAGE_MCONTROL6_LOAD);
    saved_mstatus = ai_micro_cross_page_enable_machine_breakpoint_delivery();

    goto_priv(PRIV_HS);
    TEST_SETUP_EXCEPT();
    ai_micro_cross_page_execute_load_once(vaddr);
    TEST_ASSERT("with both execute-PC and address trigger slots armed on the same cross-page high-half access-fault split load, the first access still reports breakpoint before PMP permission is restored",
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
    TEST_ASSERT("with both execute-PC and address trigger slots still armed on that same high-half access-fault split load, the second access still reports breakpoint before PMP permission is restored",
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
    TEST_ASSERT("after two prior dual-slot breakpoint hits, clearing only the execute-PC slot still leaves the cross-page load address trigger active on the high-half access-fault path before PMP permission is restored",
        excpt.triggered == true &&
        excpt.cause == CAUSE_BKP
    );
    ai_micro_cross_page_restore_machine_status(saved_mstatus);

    goto_priv(PRIV_HS);
    hspt_init();
    sfence_vma();

    ai_micro_cross_page_clear_mcontrol6_slot(1);
    goto_priv(PRIV_HS);
    TEST_SETUP_EXCEPT();
    ai_micro_cross_page_execute_load_once(vaddr);
    TEST_ASSERT("after clearing both trigger slots, the same cross-page load first falls back to high-half load access fault with the true second-half start address before PMP permission is restored",
        excpt.triggered == true &&
        excpt.cause == CAUSE_LAF &&
        excpt.tval == second_half_vaddr
    );

    goto_priv(PRIV_M);
    TEST_ASSERT("before restoring PMP read permission, repeated trigger hits and the fallback high-half load access fault leave both boundary backing doublewords unchanged",
        read64(low_phys) == low_pattern &&
        read64(high_phys) == high_pattern
    );
    ai_micro_cross_page_clear_pmp_window();

    goto_priv(PRIV_HS);
    hspt_init();
    sfence_vma();

    TEST_SETUP_EXCEPT();
    uint64_t observed = ai_micro_cross_page_execute_load_once(vaddr);
    TEST_ASSERT("after repeated dual-slot breakpoint hits on the high-half access-fault split load, restoring PMP read permission lets the same split load recover and return the merged payload",
        excpt.triggered == false &&
        observed == recovery_expected
    );

    TEST_END();
}

bool ai_micro_hs_load_cross_page_high_half_execute_and_address_trigger_access_fault_repeated_breakpoint_then_restore_pmp_adjacent_preserved_corner()
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
    uintptr_t trigger_pc = (uintptr_t)ai_micro_cross_page_execute_load_once;
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
    ai_micro_cross_page_arm_mcontrol6_slot(0, trigger_pc,
        AI_MICRO_CROSS_PAGE_MCONTROL6_EXECUTE);
    ai_micro_cross_page_arm_mcontrol6_slot(1, vaddr, AI_MICRO_CROSS_PAGE_MCONTROL6_LOAD);
    saved_mstatus = ai_micro_cross_page_enable_machine_breakpoint_delivery();

    goto_priv(PRIV_HS);
    TEST_SETUP_EXCEPT();
    ai_micro_cross_page_execute_load_once(vaddr);
    TEST_ASSERT("with both execute-PC and address trigger slots armed on the same cross-page high-half access-fault split load, the first access still reports breakpoint before PMP permission is restored in the adjacent-preservation corner",
        excpt.triggered == true &&
        excpt.cause == CAUSE_BKP
    );
    ai_micro_cross_page_restore_machine_status(saved_mstatus);

    goto_priv(PRIV_M);
    TEST_ASSERT("the first trapped high-half access-fault split load still leaves both boundary doublewords and their adjacent neighbors unchanged before PMP permission is restored",
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
    TEST_ASSERT("with both execute-PC and address trigger slots still armed on that same high-half access-fault split load, the second access still reports breakpoint before PMP permission is restored in the adjacent-preservation corner",
        excpt.triggered == true &&
        excpt.cause == CAUSE_BKP
    );
    ai_micro_cross_page_restore_machine_status(saved_mstatus);

    goto_priv(PRIV_M);
    TEST_ASSERT("the second trapped high-half access-fault split load still leaves both boundary doublewords and their adjacent neighbors unchanged before PMP permission is restored",
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
    TEST_ASSERT("after two prior dual-slot breakpoint hits, clearing only the execute-PC slot still leaves the cross-page load address trigger active on the high-half access-fault path before PMP permission is restored in the adjacent-preservation corner",
        excpt.triggered == true &&
        excpt.cause == CAUSE_BKP
    );
    ai_micro_cross_page_restore_machine_status(saved_mstatus);

    goto_priv(PRIV_M);
    TEST_ASSERT("with only the address trigger slot armed, the trapped high-half access-fault split load still leaves both boundary doublewords and their adjacent neighbors unchanged",
        read64(low_adjacent_phys) == low_adjacent &&
        read64(low_phys) == low_pattern &&
        read64(high_phys) == high_pattern &&
        read64(high_adjacent_phys) == high_adjacent
    );

    goto_priv(PRIV_HS);
    hspt_init();
    sfence_vma();

    ai_micro_cross_page_clear_mcontrol6_slot(1);
    goto_priv(PRIV_HS);
    TEST_SETUP_EXCEPT();
    ai_micro_cross_page_execute_load_once(vaddr);
    TEST_ASSERT("after clearing both trigger slots, the same cross-page load first falls back to high-half load access fault with the true second-half start address before PMP permission is restored in the adjacent-preservation corner",
        excpt.triggered == true &&
        excpt.cause == CAUSE_LAF &&
        excpt.tval == second_half_vaddr
    );

    goto_priv(PRIV_M);
    TEST_ASSERT("before restoring PMP read permission, repeated trigger hits and the fallback high-half load access fault leave both boundary backing doublewords and their adjacent neighbors unchanged",
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
    TEST_ASSERT("after repeated dual-slot breakpoint hits on the high-half access-fault split load, restoring PMP read permission lets the same split load recover and return the merged payload while keeping adjacent words untouched",
        excpt.triggered == false &&
        observed == recovery_expected
    );

    goto_priv(PRIV_M);
    TEST_ASSERT("the recovered split-load path after PMP restore still leaves both adjacent boundary words unchanged",
        read64(low_adjacent_phys) == low_adjacent &&
        read64(low_phys) == low_pattern &&
        read64(high_phys) == high_pattern &&
        read64(high_adjacent_phys) == high_adjacent
    );

    TEST_END();
}

bool ai_micro_hs_store_cross_page_low_half_execute_and_address_trigger_page_fault_repeated_breakpoint_then_fault_no_side_effect_corner()
{
    TEST_START();

    uintptr_t vaddr = ai_micro_cross_page_trigger_addr(VSR_GX, 4U);
    uintptr_t low_phys = ai_micro_cross_page_trigger_page_end_phys(VSR_GX);
    uintptr_t high_phys = phys_page_base(VSX_GRWX);
    uintptr_t trigger_pc = (uintptr_t)ai_micro_cross_page_execute_store_once;
    const uint64_t low_pattern = 0x1122334455667788ULL;
    const uint64_t high_pattern = 0x99aabbccddeeff00ULL;
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
    ai_micro_cross_page_execute_store_once(vaddr, 0x0123456789abcdefULL);
    TEST_ASSERT("with both execute-PC and address trigger slots armed on the same cross-page low-half page-fault split store, the first access still reports breakpoint",
        excpt.triggered == true &&
        excpt.cause == CAUSE_BKP
    );

    goto_priv(PRIV_M);
    TEST_ASSERT("the first trapped cross-page low-half page-fault split store still leaves both page fragments unchanged",
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
    ai_micro_cross_page_execute_store_once(vaddr, 0x0123456789abcdefULL);
    TEST_ASSERT("with both execute-PC and address trigger slots still armed on that same low-half page-fault split store, the second access still reports breakpoint",
        excpt.triggered == true &&
        excpt.cause == CAUSE_BKP
    );

    goto_priv(PRIV_M);
    TEST_ASSERT("the second trapped cross-page low-half page-fault split store still leaves both page fragments unchanged",
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
    TEST_ASSERT("after two prior dual-slot breakpoint hits, clearing only the execute-PC slot still leaves the cross-page store address trigger active on the low-half page-fault path",
        excpt.triggered == true &&
        excpt.cause == CAUSE_BKP
    );

    goto_priv(PRIV_M);
    TEST_ASSERT("with only the address trigger slot armed, the trapped cross-page low-half page-fault split store still leaves both page fragments unchanged",
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
    ai_micro_cross_page_execute_store_once(vaddr, 0x0123456789abcdefULL);
    trap_triggered = excpt.triggered;
    trap_cause = excpt.cause;
    trap_tval = excpt.tval;

    goto_priv(PRIV_M);
    TEST_ASSERT("after clearing both trigger slots, the same cross-page store falls back to low-half store page fault with no write side effect on either page fragment",
        trap_triggered == true &&
        trap_cause == CAUSE_SPF &&
        trap_tval == vaddr &&
        read64(low_phys) == low_pattern &&
        read64(high_phys) == high_pattern
    );

    TEST_END();
}

bool ai_micro_hs_store_cross_page_low_half_execute_and_address_trigger_page_fault_repeated_breakpoint_then_fault_no_side_effect_adjacent_preserved_corner()
{
    TEST_START();

    enum test_page low_page = VSR_GX;
    enum test_page high_page = VSW_GI;
    uintptr_t vaddr = ai_micro_cross_page_trigger_addr(low_page, 4U);
    uintptr_t low_adjacent_phys = phys_page_base(low_page) + PAGE_SIZE - (2 * sizeof(uint64_t));
    uintptr_t low_phys = ai_micro_cross_page_trigger_page_end_phys(low_page);
    uintptr_t high_phys = phys_page_base(high_page);
    uintptr_t high_adjacent_phys = high_phys + sizeof(uint64_t);
    uintptr_t trigger_pc = (uintptr_t)ai_micro_cross_page_execute_store_once;
    const uint64_t low_adjacent = 0x55aa55aa33cc33ccULL;
    const uint64_t low_pattern = 0x1122334455667788ULL;
    const uint64_t high_pattern = 0x99aabbccddeeff00ULL;
    const uint64_t high_adjacent = 0xcc33cc3355aa55aaULL;
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
    ai_micro_cross_page_execute_store_once(vaddr, 0x0123456789abcdefULL);
    TEST_ASSERT("with both execute-PC and address trigger slots armed on the same cross-page low-half page-fault split store, the first access still reports breakpoint in the adjacent-preservation corner",
        excpt.triggered == true &&
        excpt.cause == CAUSE_BKP
    );

    goto_priv(PRIV_M);
    TEST_ASSERT("the first trapped cross-page low-half page-fault split store still leaves both page fragments and their adjacent neighbors unchanged",
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
    ai_micro_cross_page_execute_store_once(vaddr, 0x0123456789abcdefULL);
    TEST_ASSERT("with both execute-PC and address trigger slots still armed on that same low-half page-fault split store, the second access still reports breakpoint in the adjacent-preservation corner",
        excpt.triggered == true &&
        excpt.cause == CAUSE_BKP
    );

    goto_priv(PRIV_M);
    TEST_ASSERT("the second trapped cross-page low-half page-fault split store still leaves both page fragments and their adjacent neighbors unchanged",
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
    ai_micro_cross_page_execute_store_once(vaddr, 0x0123456789abcdefULL);
    TEST_ASSERT("after two prior dual-slot breakpoint hits, clearing only the execute-PC slot still leaves the cross-page store address trigger active on the low-half page-fault path in the adjacent-preservation corner",
        excpt.triggered == true &&
        excpt.cause == CAUSE_BKP
    );

    goto_priv(PRIV_M);
    TEST_ASSERT("with only the address trigger slot armed, the trapped cross-page low-half page-fault split store still leaves both page fragments and their adjacent neighbors unchanged",
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
    ai_micro_cross_page_execute_store_once(vaddr, 0x0123456789abcdefULL);
    trap_triggered = excpt.triggered;
    trap_cause = excpt.cause;
    trap_tval = excpt.tval;

    goto_priv(PRIV_M);
    TEST_ASSERT("after clearing both trigger slots, the same cross-page store falls back to low-half store page fault with no write side effect on either page fragment or adjacent neighbor",
        trap_triggered == true &&
        trap_cause == CAUSE_SPF &&
        trap_tval == vaddr &&
        read64(low_adjacent_phys) == low_adjacent &&
        read64(low_phys) == low_pattern &&
        read64(high_phys) == high_pattern &&
        read64(high_adjacent_phys) == high_adjacent
    );

    TEST_END();
}

bool ai_micro_hs_store_cross_page_low_half_execute_and_address_trigger_access_fault_repeated_breakpoint_then_fault_no_side_effect_corner()
{
    TEST_START();

    uintptr_t vaddr = ai_micro_cross_page_trigger_addr(VSRW_GRW, 4U);
    uintptr_t low_phys = ai_micro_cross_page_trigger_page_end_phys(VSRW_GRW);
    uintptr_t high_phys = phys_page_base(VSRW_GRX);
    uintptr_t deny_start = phys_page_base(VSRW_GRW);
    uintptr_t deny_end = deny_start + PAGE_SIZE;
    uintptr_t trigger_pc = (uintptr_t)ai_micro_cross_page_execute_store_once;
    const uint64_t low_pattern = 0x1122334455667788ULL;
    const uint64_t high_pattern = 0x99aabbccddeeff00ULL;
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
    ai_micro_cross_page_execute_store_once(vaddr, 0x0123456789abcdefULL);
    TEST_ASSERT("with both execute-PC and address trigger slots armed on the same cross-page low-half access-fault split store, the first access still reports breakpoint",
        excpt.triggered == true &&
        excpt.cause == CAUSE_BKP
    );

    goto_priv(PRIV_M);
    TEST_ASSERT("the first trapped cross-page low-half access-fault split store still leaves both page fragments unchanged",
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
    ai_micro_cross_page_execute_store_once(vaddr, 0x0123456789abcdefULL);
    TEST_ASSERT("with both execute-PC and address trigger slots still armed on that same low-half access-fault split store, the second access still reports breakpoint",
        excpt.triggered == true &&
        excpt.cause == CAUSE_BKP
    );

    goto_priv(PRIV_M);
    TEST_ASSERT("the second trapped cross-page low-half access-fault split store still leaves both page fragments unchanged",
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
    TEST_ASSERT("after two prior dual-slot breakpoint hits, clearing only the execute-PC slot still leaves the cross-page store address trigger active on the low-half access-fault path",
        excpt.triggered == true &&
        excpt.cause == CAUSE_BKP
    );

    goto_priv(PRIV_M);
    TEST_ASSERT("with only the address trigger slot armed, the trapped cross-page low-half access-fault split store still leaves both page fragments unchanged",
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
    ai_micro_cross_page_execute_store_once(vaddr, 0x0123456789abcdefULL);
    trap_triggered = excpt.triggered;
    trap_cause = excpt.cause;
    trap_tval = excpt.tval;

    goto_priv(PRIV_M);
    TEST_ASSERT("after clearing both trigger slots, the same cross-page store falls back to low-half store access fault with no write side effect on either page fragment",
        trap_triggered == true &&
        trap_cause == CAUSE_SAF &&
        trap_tval == vaddr &&
        read64(low_phys) == low_pattern &&
        read64(high_phys) == high_pattern
    );

    ai_micro_cross_page_clear_pmp_window();

    TEST_END();
}

bool ai_micro_hs_store_cross_page_low_half_execute_and_address_trigger_access_fault_repeated_breakpoint_then_fault_no_side_effect_adjacent_preserved_corner()
{
    TEST_START();

    enum test_page low_page = VSRW_GRW;
    enum test_page high_page = VSRW_GRX;
    uintptr_t vaddr = ai_micro_cross_page_trigger_addr(low_page, 4U);
    uintptr_t low_adjacent_phys = phys_page_base(low_page) + PAGE_SIZE - (2 * sizeof(uint64_t));
    uintptr_t low_phys = ai_micro_cross_page_trigger_page_end_phys(low_page);
    uintptr_t high_phys = phys_page_base(high_page);
    uintptr_t high_adjacent_phys = high_phys + sizeof(uint64_t);
    uintptr_t deny_start = phys_page_base(low_page);
    uintptr_t deny_end = deny_start + PAGE_SIZE;
    uintptr_t trigger_pc = (uintptr_t)ai_micro_cross_page_execute_store_once;
    const uint64_t low_adjacent = 0x0badc0de55aa1234ULL;
    const uint64_t low_pattern = 0x1122334455667788ULL;
    const uint64_t high_pattern = 0x99aabbccddeeff00ULL;
    const uint64_t high_adjacent = 0x1234fedc5678ba90ULL;
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
    ai_micro_cross_page_execute_store_once(vaddr, 0x0123456789abcdefULL);
    TEST_ASSERT("with both execute-PC and address trigger slots armed on the same cross-page low-half access-fault split store, the first access still reports breakpoint in the adjacent-preservation corner",
        excpt.triggered == true &&
        excpt.cause == CAUSE_BKP
    );

    goto_priv(PRIV_M);
    TEST_ASSERT("the first trapped cross-page low-half access-fault split store still leaves both page fragments and their adjacent neighbors unchanged",
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
    ai_micro_cross_page_execute_store_once(vaddr, 0x0123456789abcdefULL);
    TEST_ASSERT("with both execute-PC and address trigger slots still armed on that same low-half access-fault split store, the second access still reports breakpoint in the adjacent-preservation corner",
        excpt.triggered == true &&
        excpt.cause == CAUSE_BKP
    );

    goto_priv(PRIV_M);
    TEST_ASSERT("the second trapped cross-page low-half access-fault split store still leaves both page fragments and their adjacent neighbors unchanged",
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
    ai_micro_cross_page_execute_store_once(vaddr, 0x0123456789abcdefULL);
    TEST_ASSERT("after two prior dual-slot breakpoint hits, clearing only the execute-PC slot still leaves the cross-page store address trigger active on the low-half access-fault path in the adjacent-preservation corner",
        excpt.triggered == true &&
        excpt.cause == CAUSE_BKP
    );

    goto_priv(PRIV_M);
    TEST_ASSERT("with only the address trigger slot armed, the trapped cross-page low-half access-fault split store still leaves both page fragments and their adjacent neighbors unchanged",
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
    ai_micro_cross_page_execute_store_once(vaddr, 0x0123456789abcdefULL);
    trap_triggered = excpt.triggered;
    trap_cause = excpt.cause;
    trap_tval = excpt.tval;

    goto_priv(PRIV_M);
    TEST_ASSERT("after clearing both trigger slots, the same cross-page store falls back to low-half store access fault with no write side effect on either page fragment or adjacent neighbor",
        trap_triggered == true &&
        trap_cause == CAUSE_SAF &&
        trap_tval == vaddr &&
        read64(low_adjacent_phys) == low_adjacent &&
        read64(low_phys) == low_pattern &&
        read64(high_phys) == high_pattern &&
        read64(high_adjacent_phys) == high_adjacent
    );

    ai_micro_cross_page_clear_pmp_window();

    TEST_END();
}

bool ai_micro_hs_store_cross_page_high_half_execute_and_address_trigger_page_fault_repeated_breakpoint_then_fault_no_side_effect_corner()
{
    TEST_START();

    uintptr_t vaddr = ai_micro_cross_page_trigger_addr(VSRW_GX, 4U);
    uintptr_t high_half_vaddr = hs_page_base(VSRX_GRWX);
    uintptr_t low_phys = ai_micro_cross_page_trigger_page_end_phys(VSRW_GX);
    uintptr_t high_phys = phys_page_base(VSRX_GRWX);
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
    ai_micro_cross_page_execute_store_once(vaddr, 0xfedcba9876543210ULL);
    TEST_ASSERT("with both execute-PC and address trigger slots armed on the same cross-page high-half page-fault split store, the first access still reports breakpoint",
        excpt.triggered == true &&
        excpt.cause == CAUSE_BKP
    );

    goto_priv(PRIV_M);
    TEST_ASSERT("the first trapped cross-page high-half page-fault split store still leaves both page fragments unchanged",
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
    ai_micro_cross_page_execute_store_once(vaddr, 0xfedcba9876543210ULL);
    TEST_ASSERT("with both execute-PC and address trigger slots still armed on that same high-half page-fault split store, the second access still reports breakpoint",
        excpt.triggered == true &&
        excpt.cause == CAUSE_BKP
    );

    goto_priv(PRIV_M);
    TEST_ASSERT("the second trapped cross-page high-half page-fault split store still leaves both page fragments unchanged",
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
    TEST_ASSERT("after two prior dual-slot breakpoint hits, clearing only the execute-PC slot still leaves the cross-page store address trigger active on the high-half page-fault path",
        excpt.triggered == true &&
        excpt.cause == CAUSE_BKP
    );

    goto_priv(PRIV_M);
    TEST_ASSERT("with only the address trigger slot armed, the trapped cross-page high-half page-fault split store still leaves both page fragments unchanged",
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
    ai_micro_cross_page_execute_store_once(vaddr, 0xfedcba9876543210ULL);
    trap_triggered = excpt.triggered;
    trap_cause = excpt.cause;
    trap_tval = excpt.tval;

    goto_priv(PRIV_M);
    TEST_ASSERT("after clearing both trigger slots, the same cross-page store falls back to the high-half store page-fault corner with no store side effect",
        trap_triggered == true &&
        trap_cause == CAUSE_SPF &&
        trap_tval == high_half_vaddr &&
        read64(low_phys) == expected_low_after_fault &&
        read64(high_phys) == high_pattern
    );

    TEST_END();
}
