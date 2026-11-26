#include "ai_arch_trigger_breakpoint_common.h"

bool ai_arch_load_execute_trigger_breakpoint()
{
    TEST_START();

    uintptr_t addr = ai_arch_trigger_addr(0);
    uintptr_t trigger_pc = (uintptr_t)ai_arch_execute_trigger_load_once;
    ai_arch_trigger_mem[0] = 0x1122334455667788ULL;

    ai_arch_arm_mcontrol6_0(trigger_pc, AI_ARCH_MCONTROL6_EXECUTE);
    ai_arch_sync_trigger_state();
    uint64_t saved_mstatus = ai_arch_enable_machine_breakpoint_delivery();

    TEST_SETUP_EXCEPT();
    ai_arch_execute_trigger_load_once(addr);
    TEST_ASSERT("a load execute-PC trigger configured through mcontrol6 reports breakpoint before the load executes",
        excpt.triggered == true &&
        excpt.cause == CAUSE_BKP
    );
    ai_arch_restore_machine_status(saved_mstatus);

    ai_arch_clear_mcontrol6_0();

    TEST_SETUP_EXCEPT();
    uint64_t value = ai_arch_execute_trigger_load_once(addr);
    TEST_ASSERT("the same load instruction completes normally after clearing the execute-PC trigger",
        excpt.triggered == false &&
        value == 0x1122334455667788ULL
    );

    TEST_END();
}

bool ai_arch_load_execute_trigger_repeated_breakpoint_corner()
{
    TEST_START();

    uintptr_t addr = ai_arch_trigger_addr(0);
    uintptr_t trigger_pc = (uintptr_t)ai_arch_execute_trigger_load_once;
    const uint64_t expected_value = 0x1122334455667788ULL;

    ai_arch_trigger_mem[0] = expected_value;

    ai_arch_arm_mcontrol6_0(trigger_pc, AI_ARCH_MCONTROL6_EXECUTE);
    ai_arch_sync_trigger_state();
    uint64_t saved_mstatus = ai_arch_enable_machine_breakpoint_delivery();

    TEST_SETUP_EXCEPT();
    ai_arch_execute_trigger_load_once(addr);
    TEST_ASSERT("the first load execute-PC trigger hit reports breakpoint before the load retires",
        excpt.triggered == true &&
        excpt.cause == CAUSE_BKP &&
        ai_arch_trigger_mem[0] == expected_value
    );
    ai_arch_reenable_breakpoint_delivery();

    TEST_SETUP_EXCEPT();
    ai_arch_execute_trigger_load_once(addr);
    TEST_ASSERT("the second load execute-PC trigger hit still reports breakpoint and still leaves the backing word unchanged",
        excpt.triggered == true &&
        excpt.cause == CAUSE_BKP &&
        ai_arch_trigger_mem[0] == expected_value
    );
    ai_arch_restore_machine_status(saved_mstatus);

    ai_arch_clear_mcontrol6_0();

    TEST_SETUP_EXCEPT();
    uint64_t value = ai_arch_execute_trigger_load_once(addr);
    TEST_ASSERT("after clearing the repeated load execute-PC trigger, the same load completes normally and still reads the original value",
        excpt.triggered == false &&
        value == expected_value
    );

    TEST_END();
}

bool ai_arch_store_execute_trigger_breakpoint()
{
    TEST_START();

    uintptr_t addr = ai_arch_trigger_addr(0);
    uintptr_t trigger_pc = (uintptr_t)ai_arch_execute_trigger_store_once;
    const uint64_t initial_value = 0x8877665544332211ULL;
    const uint64_t store_value = 0xa1b2c3d4e5f60718ULL;

    ai_arch_trigger_mem[0] = initial_value;

    ai_arch_arm_mcontrol6_0(trigger_pc, AI_ARCH_MCONTROL6_EXECUTE);
    uint64_t saved_mstatus = ai_arch_enable_machine_breakpoint_delivery();

    TEST_SETUP_EXCEPT();
    ai_arch_execute_trigger_store_once(addr, store_value);
    TEST_ASSERT("a store execute-PC trigger configured through mcontrol6 reports breakpoint before the store executes",
        excpt.triggered == true &&
        excpt.cause == CAUSE_BKP &&
        ai_arch_trigger_mem[0] == initial_value
    );
    ai_arch_restore_machine_status(saved_mstatus);

    ai_arch_clear_mcontrol6_0();
    ai_arch_trigger_mem[0] = initial_value;

    TEST_SETUP_EXCEPT();
    ai_arch_execute_trigger_store_once(addr, store_value);
    TEST_ASSERT("the same store instruction completes normally after clearing the execute-PC trigger",
        excpt.triggered == false &&
        ai_arch_trigger_mem[0] == store_value
    );

    TEST_END();
}

bool ai_arch_store_execute_trigger_repeated_breakpoint_no_side_effect_corner()
{
    TEST_START();

    uintptr_t addr = ai_arch_trigger_addr(0);
    uintptr_t trigger_pc = (uintptr_t)ai_arch_execute_trigger_store_once;
    const uint64_t initial_value = 0x8877665544332211ULL;
    const uint64_t first_store_value = 0xa1b2c3d4e5f60718ULL;
    const uint64_t second_store_value = 0x0fedcba987654321ULL;

    ai_arch_trigger_mem[0] = initial_value;

    ai_arch_arm_mcontrol6_0(trigger_pc, AI_ARCH_MCONTROL6_EXECUTE);
    uint64_t saved_mstatus = ai_arch_enable_machine_breakpoint_delivery();

    TEST_SETUP_EXCEPT();
    ai_arch_execute_trigger_store_once(addr, first_store_value);
    TEST_ASSERT("the first store execute-PC trigger hit reports breakpoint before the store executes",
        excpt.triggered == true &&
        excpt.cause == CAUSE_BKP &&
        ai_arch_trigger_mem[0] == initial_value
    );
    ai_arch_reenable_breakpoint_delivery();

    TEST_SETUP_EXCEPT();
    ai_arch_execute_trigger_store_once(addr, second_store_value);
    TEST_ASSERT("the second store execute-PC trigger hit still reports breakpoint and still suppresses the store update",
        excpt.triggered == true &&
        excpt.cause == CAUSE_BKP &&
        ai_arch_trigger_mem[0] == initial_value
    );
    ai_arch_restore_machine_status(saved_mstatus);

    ai_arch_clear_mcontrol6_0();
    ai_arch_trigger_mem[0] = initial_value;

    TEST_SETUP_EXCEPT();
    ai_arch_execute_trigger_store_once(addr, first_store_value);
    TEST_ASSERT("after clearing the repeated store execute-PC trigger, the same store completes normally and updates memory",
        excpt.triggered == false &&
        ai_arch_trigger_mem[0] == first_store_value
    );

    TEST_END();
}

bool ai_arch_amo_execute_trigger_breakpoint()
{
    TEST_START();

    uintptr_t addr = ai_arch_trigger_addr(0);
    uintptr_t trigger_pc = (uintptr_t)ai_arch_execute_trigger_amoswap_w_once;

    ai_arch_trigger_mem[0] = 0x0000001055667788ULL;

    ai_arch_arm_mcontrol6_0(trigger_pc, AI_ARCH_MCONTROL6_EXECUTE);
    uint64_t saved_mstatus = ai_arch_enable_machine_breakpoint_delivery();

    TEST_SETUP_EXCEPT();
    ai_arch_execute_trigger_amoswap_w_once(addr, 0xa1b2c3d4U);
    if (!(excpt.triggered == true && excpt.cause == CAUSE_BKP)) {
        ai_arch_log_unexpected_trigger_result("unexpected AMO execute-PC trigger result");
    }
    TEST_ASSERT("an AMO execute-PC trigger configured through mcontrol6 reports breakpoint before the atomic access executes",
        excpt.triggered == true &&
        excpt.cause == CAUSE_BKP &&
        ai_arch_trigger_mem[0] == 0x0000001055667788ULL
    );
    ai_arch_restore_machine_status(saved_mstatus);

    ai_arch_clear_mcontrol6_0();
    ai_arch_trigger_mem[0] = 0x0000001055667788ULL;

    TEST_SETUP_EXCEPT();
    uint32_t old_value = ai_arch_execute_trigger_amoswap_w_once(addr, 0xa1b2c3d4U);
    TEST_ASSERT("the same AMO completes normally after clearing the execute-PC trigger",
        excpt.triggered == false &&
        old_value == 0x55667788U &&
        ai_arch_trigger_mem[0] == 0x00000010a1b2c3d4ULL
    );

    TEST_END();
}

bool ai_arch_amo_execute_trigger_repeated_breakpoint_no_side_effect_corner()
{
    TEST_START();

    uintptr_t addr = ai_arch_trigger_addr(0);
    uintptr_t trigger_pc = (uintptr_t)ai_arch_execute_trigger_amoswap_w_once;
    const uint64_t initial_value = 0x0000001055667788ULL;
    const uint32_t first_swap_value = 0xa1b2c3d4U;
    const uint32_t second_swap_value = 0x11223344U;

    ai_arch_trigger_mem[0] = initial_value;

    ai_arch_arm_mcontrol6_0(trigger_pc, AI_ARCH_MCONTROL6_EXECUTE);
    uint64_t saved_mstatus = ai_arch_enable_machine_breakpoint_delivery();

    TEST_SETUP_EXCEPT();
    ai_arch_execute_trigger_amoswap_w_once(addr, first_swap_value);
    if (!(excpt.triggered == true && excpt.cause == CAUSE_BKP)) {
        ai_arch_log_unexpected_trigger_result("unexpected repeated AMO execute-PC trigger first-hit result");
    }
    TEST_ASSERT("the first AMO execute-PC trigger hit reports breakpoint and suppresses the atomic update",
        excpt.triggered == true &&
        excpt.cause == CAUSE_BKP &&
        ai_arch_trigger_mem[0] == initial_value
    );
    ai_arch_reenable_breakpoint_delivery();

    TEST_SETUP_EXCEPT();
    ai_arch_execute_trigger_amoswap_w_once(addr, second_swap_value);
    if (!(excpt.triggered == true && excpt.cause == CAUSE_BKP)) {
        ai_arch_log_unexpected_trigger_result("unexpected repeated AMO execute-PC trigger second-hit result");
    }
    TEST_ASSERT("the second AMO execute-PC trigger hit still reports breakpoint and still leaves memory unchanged",
        excpt.triggered == true &&
        excpt.cause == CAUSE_BKP &&
        ai_arch_trigger_mem[0] == initial_value
    );
    ai_arch_restore_machine_status(saved_mstatus);

    ai_arch_clear_mcontrol6_0();
    ai_arch_trigger_mem[0] = initial_value;

    TEST_SETUP_EXCEPT();
    uint32_t old_value = ai_arch_execute_trigger_amoswap_w_once(addr, first_swap_value);
    TEST_ASSERT("after clearing the repeated execute-PC trigger, the same AMO completes normally with the expected RV64 low-word swap semantics",
        excpt.triggered == false &&
        old_value == 0x55667788U &&
        ai_arch_trigger_mem[0] == 0x00000010a1b2c3d4ULL
    );

    TEST_END();
}

bool ai_arch_load_address_trigger_breakpoint()
{
    TEST_START();

    uintptr_t addr = ai_arch_trigger_addr(0);
    ai_arch_trigger_mem[0] = 0x1122334455667788ULL;

    ai_arch_arm_mcontrol6_0(addr, AI_ARCH_MCONTROL6_LOAD);
    uint64_t saved_mstatus = ai_arch_enable_machine_breakpoint_delivery();

    TEST_SETUP_EXCEPT();
    ld(addr);
    TEST_ASSERT("a load address trigger configured through mcontrol6 reports breakpoint",
        excpt.triggered == true &&
        excpt.cause == CAUSE_BKP
    );
    ai_arch_restore_machine_status(saved_mstatus);

    ai_arch_clear_mcontrol6_0();

    TEST_SETUP_EXCEPT();
    uint64_t value = ld(addr);
    TEST_ASSERT("the same load completes normally after clearing the trigger",
        excpt.triggered == false &&
        value == 0x1122334455667788ULL
    );

    TEST_END();
}

bool ai_arch_load_address_trigger_repeated_breakpoint_corner()
{
    TEST_START();

    uintptr_t addr = ai_arch_trigger_addr(0);
    const uint64_t expected_value = 0x1122334455667788ULL;

    ai_arch_trigger_mem[0] = expected_value;

    ai_arch_arm_mcontrol6_0(addr, AI_ARCH_MCONTROL6_LOAD);
    uint64_t saved_mstatus = ai_arch_enable_machine_breakpoint_delivery();

    TEST_SETUP_EXCEPT();
    ld(addr);
    TEST_ASSERT("the first load address trigger hit reports breakpoint before the load retires",
        excpt.triggered == true &&
        excpt.cause == CAUSE_BKP &&
        ai_arch_trigger_mem[0] == expected_value
    );
    ai_arch_reenable_breakpoint_delivery();

    TEST_SETUP_EXCEPT();
    ld(addr);
    TEST_ASSERT("the second load address trigger hit still reports breakpoint and still leaves the backing word unchanged",
        excpt.triggered == true &&
        excpt.cause == CAUSE_BKP &&
        ai_arch_trigger_mem[0] == expected_value
    );
    ai_arch_restore_machine_status(saved_mstatus);

    ai_arch_clear_mcontrol6_0();

    TEST_SETUP_EXCEPT();
    uint64_t value = ld(addr);
    TEST_ASSERT("after clearing the repeated load address trigger, the same load completes normally and still reads the original value",
        excpt.triggered == false &&
        value == expected_value
    );

    TEST_END();
}

bool ai_arch_load_execute_and_address_trigger_both_breakpoint_corner()
{
    TEST_START();

    uintptr_t addr = ai_arch_trigger_addr(0);
    uintptr_t trigger_pc = (uintptr_t)ai_arch_execute_trigger_load_once;
    const uint64_t expected_value = 0x1122334455667788ULL;

    ai_arch_trigger_mem[0] = expected_value;
    ai_arch_clear_mcontrol6_slot(0);
    ai_arch_clear_mcontrol6_slot(1);
    ai_arch_arm_mcontrol6_slot(0, trigger_pc, AI_ARCH_MCONTROL6_EXECUTE);
    ai_arch_arm_mcontrol6_slot(1, addr, AI_ARCH_MCONTROL6_LOAD);

    uint64_t saved_mstatus = ai_arch_enable_machine_breakpoint_delivery();
    TEST_SETUP_EXCEPT();
    ai_arch_execute_trigger_load_once(addr);
    TEST_ASSERT("when both execute-PC and address triggers match the same load, Spike reports breakpoint before the load completes",
        excpt.triggered == true &&
        excpt.cause == CAUSE_BKP
    );
    ai_arch_restore_machine_status(saved_mstatus);

    ai_arch_clear_mcontrol6_slot(0);
    saved_mstatus = ai_arch_enable_machine_breakpoint_delivery();
    TEST_SETUP_EXCEPT();
    ai_arch_execute_trigger_load_once(addr);
    TEST_ASSERT("after clearing only the execute-PC trigger, the remaining load-address trigger still reports breakpoint for the same load",
        excpt.triggered == true &&
        excpt.cause == CAUSE_BKP
    );
    ai_arch_restore_machine_status(saved_mstatus);

    ai_arch_clear_mcontrol6_slot(1);
    TEST_SETUP_EXCEPT();
    uint64_t value = ai_arch_execute_trigger_load_once(addr);
    TEST_ASSERT("after clearing both trigger slots, the same load completes normally and reads the original value",
        excpt.triggered == false &&
        value == expected_value
    );

    TEST_END();
}

bool ai_arch_load_execute_and_address_trigger_repeated_breakpoint_corner()
{
    TEST_START();

    uintptr_t addr = ai_arch_trigger_addr(0);
    uintptr_t trigger_pc = (uintptr_t)ai_arch_execute_trigger_load_once;
    const uint64_t expected_value = 0x1122334455667788ULL;

    ai_arch_trigger_mem[0] = expected_value;
    ai_arch_clear_mcontrol6_slot(0);
    ai_arch_clear_mcontrol6_slot(1);
    ai_arch_arm_mcontrol6_slot(0, trigger_pc, AI_ARCH_MCONTROL6_EXECUTE);
    ai_arch_arm_mcontrol6_slot(1, addr, AI_ARCH_MCONTROL6_LOAD);

    uint64_t saved_mstatus = ai_arch_enable_machine_breakpoint_delivery();
    TEST_SETUP_EXCEPT();
    ai_arch_execute_trigger_load_once(addr);
    TEST_ASSERT("the first dual-slot load trigger hit reports breakpoint before the load completes",
        excpt.triggered == true &&
        excpt.cause == CAUSE_BKP &&
        ai_arch_trigger_mem[0] == expected_value
    );
    ai_arch_reenable_breakpoint_delivery();

    TEST_SETUP_EXCEPT();
    ai_arch_execute_trigger_load_once(addr);
    TEST_ASSERT("with both trigger slots still armed, a second hit on the same load still reports breakpoint and leaves the backing word unchanged",
        excpt.triggered == true &&
        excpt.cause == CAUSE_BKP &&
        ai_arch_trigger_mem[0] == expected_value
    );
    ai_arch_restore_machine_status(saved_mstatus);

    ai_arch_clear_mcontrol6_slot(0);
    saved_mstatus = ai_arch_enable_machine_breakpoint_delivery();
    TEST_SETUP_EXCEPT();
    ai_arch_execute_trigger_load_once(addr);
    TEST_ASSERT("after repeated dual-slot hits, clearing only the execute-PC slot still leaves the address trigger active on the same load",
        excpt.triggered == true &&
        excpt.cause == CAUSE_BKP &&
        ai_arch_trigger_mem[0] == expected_value
    );
    ai_arch_restore_machine_status(saved_mstatus);

    ai_arch_clear_mcontrol6_slot(1);
    TEST_SETUP_EXCEPT();
    uint64_t value = ai_arch_execute_trigger_load_once(addr);
    TEST_ASSERT("after clearing both trigger slots, the same load completes normally and still reads the original value",
        excpt.triggered == false &&
        value == expected_value
    );

    TEST_END();
}

bool ai_arch_load_execute_and_address_trigger_access_fault_priority_corner()
{
    TEST_START();

    uintptr_t trigger_pc = (uintptr_t)ai_arch_execute_trigger_load_once;
    uintptr_t bad_paddr = AI_ARCH_TRIGGER_BAD_PADDR;

    ai_arch_clear_mcontrol6_slot(0);
    ai_arch_clear_mcontrol6_slot(1);
    ai_arch_arm_mcontrol6_slot(0, trigger_pc, AI_ARCH_MCONTROL6_EXECUTE);
    ai_arch_arm_mcontrol6_slot(1, bad_paddr, AI_ARCH_MCONTROL6_LOAD);

    uint64_t saved_mstatus = ai_arch_enable_machine_breakpoint_delivery();
    TEST_SETUP_EXCEPT();
    ai_arch_execute_trigger_load_once(bad_paddr);
    if (!(excpt.triggered == true && excpt.cause == CAUSE_BKP)) {
        ai_arch_log_unexpected_trigger_result("unexpected load dual-trigger+access-fault priority result");
    }
    TEST_ASSERT("when both execute-PC and address triggers match a load that would otherwise raise direct M-mode access fault, Spike still reports breakpoint first",
        excpt.triggered == true &&
        excpt.cause == CAUSE_BKP
    );
    ai_arch_restore_machine_status(saved_mstatus);

    ai_arch_clear_mcontrol6_slot(0);
    saved_mstatus = ai_arch_enable_machine_breakpoint_delivery();
    TEST_SETUP_EXCEPT();
    ai_arch_execute_trigger_load_once(bad_paddr);
    TEST_ASSERT("after clearing only the execute-PC trigger, the remaining load-address trigger still reports breakpoint on the same direct-M access-fault path",
        excpt.triggered == true &&
        excpt.cause == CAUSE_BKP
    );
    ai_arch_restore_machine_status(saved_mstatus);

    ai_arch_clear_mcontrol6_slot(1);
    TEST_SETUP_EXCEPT();
    ai_arch_execute_trigger_load_once(bad_paddr);
    TEST_ASSERT("after clearing both trigger slots, the same load reports load access fault with the raw address in tval",
        excpt.triggered == true &&
        excpt.cause == CAUSE_LAF &&
        excpt.tval == bad_paddr
    );

    TEST_END();
}

bool ai_arch_load_execute_and_address_trigger_access_fault_repeated_breakpoint_then_fault_corner()
{
    TEST_START();

    uintptr_t trigger_pc = (uintptr_t)ai_arch_execute_trigger_load_once;
    uintptr_t bad_paddr = AI_ARCH_TRIGGER_BAD_PADDR;
    const uint64_t scratch0 = 0x1122334455667788ULL;
    const uint64_t scratch1 = 0x8877665544332211ULL;

    ai_arch_trigger_mem[0] = scratch0;
    ai_arch_trigger_mem[1] = scratch1;
    ai_arch_clear_mcontrol6_slot(0);
    ai_arch_clear_mcontrol6_slot(1);
    ai_arch_arm_mcontrol6_slot(0, trigger_pc, AI_ARCH_MCONTROL6_EXECUTE);
    ai_arch_arm_mcontrol6_slot(1, bad_paddr, AI_ARCH_MCONTROL6_LOAD);

    uint64_t saved_mstatus = ai_arch_enable_machine_breakpoint_delivery();
    TEST_SETUP_EXCEPT();
    ai_arch_execute_trigger_load_once(bad_paddr);
    TEST_ASSERT("with both load trigger slots armed on a direct-M access-faulting PA, the first access reports breakpoint and leaves scratch state unchanged",
        excpt.triggered == true &&
        excpt.cause == CAUSE_BKP &&
        ai_arch_trigger_mem[0] == scratch0 &&
        ai_arch_trigger_mem[1] == scratch1
    );
    ai_arch_reenable_breakpoint_delivery();

    TEST_SETUP_EXCEPT();
    ai_arch_execute_trigger_load_once(bad_paddr);
    TEST_ASSERT("with both load trigger slots still armed on the same direct-M access-faulting PA, the second access still reports breakpoint and still leaves scratch state unchanged",
        excpt.triggered == true &&
        excpt.cause == CAUSE_BKP &&
        ai_arch_trigger_mem[0] == scratch0 &&
        ai_arch_trigger_mem[1] == scratch1
    );
    ai_arch_restore_machine_status(saved_mstatus);

    ai_arch_clear_mcontrol6_slot(0);
    saved_mstatus = ai_arch_enable_machine_breakpoint_delivery();
    TEST_SETUP_EXCEPT();
    ai_arch_execute_trigger_load_once(bad_paddr);
    TEST_ASSERT("after repeated dual-slot load breakpoint hits, clearing only the execute-PC slot still leaves the address trigger active on the same direct-M access-fault path",
        excpt.triggered == true &&
        excpt.cause == CAUSE_BKP &&
        ai_arch_trigger_mem[0] == scratch0 &&
        ai_arch_trigger_mem[1] == scratch1
    );
    ai_arch_restore_machine_status(saved_mstatus);

    ai_arch_clear_mcontrol6_slot(1);
    TEST_SETUP_EXCEPT();
    ai_arch_execute_trigger_load_once(bad_paddr);
    TEST_ASSERT("after clearing both load trigger slots, the same direct-M access falls back to load access fault with the raw address and still leaves scratch state unchanged",
        excpt.triggered == true &&
        excpt.cause == CAUSE_LAF &&
        excpt.tval == bad_paddr &&
        ai_arch_trigger_mem[0] == scratch0 &&
        ai_arch_trigger_mem[1] == scratch1
    );

    TEST_END();
}

bool ai_arch_load_execute_and_address_trigger_page_fault_priority_corner()
{
    TEST_START();

    uintptr_t trigger_pc = (uintptr_t)ai_arch_execute_trigger_load_once;
    uintptr_t pf_vaddr = hs_page_base(VSI_GI);

    ai_arch_prepare_hs_fault_page();
    ai_arch_clear_mcontrol6_slot(0);
    ai_arch_clear_mcontrol6_slot(1);
    ai_arch_arm_mcontrol6_slot(0, trigger_pc, AI_ARCH_MCONTROL6_EXECUTE);
    ai_arch_arm_mcontrol6_slot(1, pf_vaddr, AI_ARCH_MCONTROL6_LOAD);

    uint64_t saved_mstatus = ai_arch_enable_machine_breakpoint_delivery();
    goto_priv(PRIV_HS);
    TEST_SETUP_EXCEPT();
    ai_arch_execute_trigger_load_once(pf_vaddr);
    if (!(excpt.triggered == true && excpt.cause == CAUSE_BKP)) {
        ai_arch_log_unexpected_trigger_result("unexpected load dual-trigger+page-fault priority result");
    }
    TEST_ASSERT("when both execute-PC and address triggers match a load that would otherwise raise HS page fault, Spike still reports breakpoint first",
        excpt.triggered == true &&
        excpt.cause == CAUSE_BKP
    );
    ai_arch_restore_machine_status(saved_mstatus);

    ai_arch_prepare_hs_fault_page();
    ai_arch_clear_mcontrol6_slot(0);
    saved_mstatus = ai_arch_enable_machine_breakpoint_delivery();
    goto_priv(PRIV_HS);
    TEST_SETUP_EXCEPT();
    ai_arch_execute_trigger_load_once(pf_vaddr);
    TEST_ASSERT("after clearing only the execute-PC trigger, the remaining load-address trigger still reports breakpoint on the same HS page-fault path",
        excpt.triggered == true &&
        excpt.cause == CAUSE_BKP
    );
    ai_arch_restore_machine_status(saved_mstatus);

    ai_arch_prepare_hs_fault_page();
    ai_arch_clear_mcontrol6_slot(1);
    goto_priv(PRIV_HS);
    TEST_SETUP_EXCEPT();
    ai_arch_execute_trigger_load_once(pf_vaddr);
    TEST_ASSERT("after clearing both trigger slots, the same load reports load page fault with the original faulting VA",
        excpt.triggered == true &&
        excpt.cause == CAUSE_LPF &&
        excpt.tval == pf_vaddr
    );

    TEST_END();
}

bool ai_arch_load_execute_and_address_trigger_page_fault_repeated_breakpoint_then_fault_corner()
{
    TEST_START();

    uintptr_t trigger_pc = (uintptr_t)ai_arch_execute_trigger_load_once;
    uintptr_t pf_vaddr = hs_page_base(VSI_GI);

    ai_arch_prepare_hs_fault_page();
    ai_arch_clear_mcontrol6_slot(0);
    ai_arch_clear_mcontrol6_slot(1);
    ai_arch_arm_mcontrol6_slot(0, trigger_pc, AI_ARCH_MCONTROL6_EXECUTE);
    ai_arch_arm_mcontrol6_slot(1, pf_vaddr, AI_ARCH_MCONTROL6_LOAD);

    uint64_t saved_mstatus = ai_arch_enable_machine_breakpoint_delivery();
    goto_priv(PRIV_HS);
    TEST_SETUP_EXCEPT();
    ai_arch_execute_trigger_load_once(pf_vaddr);
    TEST_ASSERT("with both load trigger slots armed on an HS page-faulting VA, the first access reports breakpoint",
        excpt.triggered == true &&
        excpt.cause == CAUSE_BKP
    );

    ai_arch_prepare_hs_fault_page();
    goto_priv(PRIV_HS);
    TEST_SETUP_EXCEPT();
    ai_arch_execute_trigger_load_once(pf_vaddr);
    TEST_ASSERT("with both load trigger slots still armed on the same HS page-faulting VA, the second access still reports breakpoint",
        excpt.triggered == true &&
        excpt.cause == CAUSE_BKP
    );
    ai_arch_restore_machine_status(saved_mstatus);

    ai_arch_prepare_hs_fault_page();
    ai_arch_clear_mcontrol6_slot(0);
    saved_mstatus = ai_arch_enable_machine_breakpoint_delivery();
    goto_priv(PRIV_HS);
    TEST_SETUP_EXCEPT();
    ai_arch_execute_trigger_load_once(pf_vaddr);
    TEST_ASSERT("after repeated dual-slot breakpoint hits, clearing only the execute-PC slot still leaves the load-address trigger active on the same page-fault path",
        excpt.triggered == true &&
        excpt.cause == CAUSE_BKP
    );
    ai_arch_restore_machine_status(saved_mstatus);

    ai_arch_prepare_hs_fault_page();
    ai_arch_clear_mcontrol6_slot(1);
    goto_priv(PRIV_HS);
    TEST_SETUP_EXCEPT();
    ai_arch_execute_trigger_load_once(pf_vaddr);
    TEST_ASSERT("after clearing both load trigger slots, the same access falls back to the original load page fault with the raw faulting VA",
        excpt.triggered == true &&
        excpt.cause == CAUSE_LPF &&
        excpt.tval == pf_vaddr
    );

    TEST_END();
}

bool ai_arch_store_address_trigger_breakpoint()
{
    TEST_START();

    uintptr_t addr = ai_arch_trigger_addr(0);
    const uint64_t initial_value = 0x8877665544332211ULL;
    const uint64_t store_value = 0xa1b2c3d4e5f60718ULL;

    ai_arch_trigger_mem[0] = initial_value;

    ai_arch_arm_mcontrol6_0(addr, AI_ARCH_MCONTROL6_STORE);
    uint64_t saved_mstatus = ai_arch_enable_machine_breakpoint_delivery();

    TEST_SETUP_EXCEPT();
    sd(addr, store_value);
    TEST_ASSERT("a store address trigger configured through mcontrol6 reports breakpoint",
        excpt.triggered == true &&
        excpt.cause == CAUSE_BKP &&
        ai_arch_trigger_mem[0] == initial_value
    );
    ai_arch_restore_machine_status(saved_mstatus);

    ai_arch_clear_mcontrol6_0();
    ai_arch_trigger_mem[0] = initial_value;

    TEST_SETUP_EXCEPT();
    sd(addr, store_value);
    TEST_ASSERT("the same store completes normally after clearing the trigger",
        excpt.triggered == false &&
        ai_arch_trigger_mem[0] == store_value
    );

    TEST_END();
}

bool ai_arch_store_address_trigger_repeated_breakpoint_no_side_effect_corner()
{
    TEST_START();

    uintptr_t addr = ai_arch_trigger_addr(0);
    const uint64_t initial_value = 0x8877665544332211ULL;
    const uint64_t first_store_value = 0xa1b2c3d4e5f60718ULL;
    const uint64_t second_store_value = 0x0fedcba987654321ULL;

    ai_arch_trigger_mem[0] = initial_value;

    ai_arch_arm_mcontrol6_0(addr, AI_ARCH_MCONTROL6_STORE);
    uint64_t saved_mstatus = ai_arch_enable_machine_breakpoint_delivery();

    TEST_SETUP_EXCEPT();
    sd(addr, first_store_value);
    TEST_ASSERT("the first store address trigger hit reports breakpoint and suppresses the store update",
        excpt.triggered == true &&
        excpt.cause == CAUSE_BKP &&
        ai_arch_trigger_mem[0] == initial_value
    );
    ai_arch_reenable_breakpoint_delivery();

    TEST_SETUP_EXCEPT();
    sd(addr, second_store_value);
    TEST_ASSERT("the second store address trigger hit still reports breakpoint and still leaves memory unchanged",
        excpt.triggered == true &&
        excpt.cause == CAUSE_BKP &&
        ai_arch_trigger_mem[0] == initial_value
    );
    ai_arch_restore_machine_status(saved_mstatus);

    ai_arch_clear_mcontrol6_0();
    ai_arch_trigger_mem[0] = initial_value;

    TEST_SETUP_EXCEPT();
    sd(addr, first_store_value);
    TEST_ASSERT("after clearing the repeated store address trigger, the same store completes normally and updates memory",
        excpt.triggered == false &&
        ai_arch_trigger_mem[0] == first_store_value
    );

    TEST_END();
}

bool ai_arch_store_execute_and_address_trigger_both_breakpoint_corner()
{
    TEST_START();

    uintptr_t addr = ai_arch_trigger_addr(0);
    uintptr_t trigger_pc = (uintptr_t)ai_arch_execute_trigger_store_once;
    const uint64_t initial_value = 0x8877665544332211ULL;
    const uint64_t store_value = 0xa1b2c3d4e5f60718ULL;

    ai_arch_trigger_mem[0] = initial_value;
    ai_arch_clear_mcontrol6_slot(0);
    ai_arch_clear_mcontrol6_slot(1);
    ai_arch_arm_mcontrol6_slot(0, trigger_pc, AI_ARCH_MCONTROL6_EXECUTE);
    ai_arch_arm_mcontrol6_slot(1, addr, AI_ARCH_MCONTROL6_STORE);

    uint64_t saved_mstatus = ai_arch_enable_machine_breakpoint_delivery();
    TEST_SETUP_EXCEPT();
    ai_arch_execute_trigger_store_once(addr, store_value);
    TEST_ASSERT("when both execute-PC and address triggers match the same store, Spike reports breakpoint before the store updates memory",
        excpt.triggered == true &&
        excpt.cause == CAUSE_BKP &&
        ai_arch_trigger_mem[0] == initial_value
    );
    ai_arch_restore_machine_status(saved_mstatus);

    ai_arch_clear_mcontrol6_slot(0);
    saved_mstatus = ai_arch_enable_machine_breakpoint_delivery();
    TEST_SETUP_EXCEPT();
    ai_arch_execute_trigger_store_once(addr, store_value);
    TEST_ASSERT("after clearing only the execute-PC trigger, the remaining store-address trigger still reports breakpoint and still suppresses the store",
        excpt.triggered == true &&
        excpt.cause == CAUSE_BKP &&
        ai_arch_trigger_mem[0] == initial_value
    );
    ai_arch_restore_machine_status(saved_mstatus);

    ai_arch_clear_mcontrol6_slot(1);
    TEST_SETUP_EXCEPT();
    ai_arch_execute_trigger_store_once(addr, store_value);
    TEST_ASSERT("after clearing both trigger slots, the same store completes normally and updates memory",
        excpt.triggered == false &&
        ai_arch_trigger_mem[0] == store_value
    );

    TEST_END();
}

bool ai_arch_store_execute_and_address_trigger_repeated_breakpoint_no_side_effect_corner()
{
    TEST_START();

    uintptr_t addr = ai_arch_trigger_addr(0);
    uintptr_t trigger_pc = (uintptr_t)ai_arch_execute_trigger_store_once;
    const uint64_t initial_value = 0x8877665544332211ULL;
    const uint64_t first_store_value = 0xa1b2c3d4e5f60718ULL;
    const uint64_t second_store_value = 0x0fedcba987654321ULL;

    ai_arch_trigger_mem[0] = initial_value;
    ai_arch_clear_mcontrol6_slot(0);
    ai_arch_clear_mcontrol6_slot(1);
    ai_arch_arm_mcontrol6_slot(0, trigger_pc, AI_ARCH_MCONTROL6_EXECUTE);
    ai_arch_arm_mcontrol6_slot(1, addr, AI_ARCH_MCONTROL6_STORE);

    uint64_t saved_mstatus = ai_arch_enable_machine_breakpoint_delivery();
    TEST_SETUP_EXCEPT();
    ai_arch_execute_trigger_store_once(addr, first_store_value);
    TEST_ASSERT("the first dual-slot store trigger hit reports breakpoint before the store executes",
        excpt.triggered == true &&
        excpt.cause == CAUSE_BKP &&
        ai_arch_trigger_mem[0] == initial_value
    );
    ai_arch_reenable_breakpoint_delivery();

    TEST_SETUP_EXCEPT();
    ai_arch_execute_trigger_store_once(addr, second_store_value);
    TEST_ASSERT("with both store trigger slots still armed, a second hit still reports breakpoint and still suppresses the store update",
        excpt.triggered == true &&
        excpt.cause == CAUSE_BKP &&
        ai_arch_trigger_mem[0] == initial_value
    );
    ai_arch_restore_machine_status(saved_mstatus);

    ai_arch_clear_mcontrol6_slot(0);
    saved_mstatus = ai_arch_enable_machine_breakpoint_delivery();
    TEST_SETUP_EXCEPT();
    ai_arch_execute_trigger_store_once(addr, second_store_value);
    TEST_ASSERT("after repeated dual-slot store hits, clearing only the execute-PC slot still leaves the store-address trigger active and memory unchanged",
        excpt.triggered == true &&
        excpt.cause == CAUSE_BKP &&
        ai_arch_trigger_mem[0] == initial_value
    );
    ai_arch_restore_machine_status(saved_mstatus);

    ai_arch_clear_mcontrol6_slot(1);
    TEST_SETUP_EXCEPT();
    ai_arch_execute_trigger_store_once(addr, first_store_value);
    TEST_ASSERT("after clearing both trigger slots, the same store completes normally and updates memory",
        excpt.triggered == false &&
        ai_arch_trigger_mem[0] == first_store_value
    );

    TEST_END();
}

bool ai_arch_store_execute_and_address_trigger_access_fault_priority_corner()
{
    TEST_START();

    uintptr_t trigger_pc = (uintptr_t)ai_arch_execute_trigger_store_once;
    uintptr_t bad_paddr = AI_ARCH_TRIGGER_BAD_PADDR;

    ai_arch_clear_mcontrol6_slot(0);
    ai_arch_clear_mcontrol6_slot(1);
    ai_arch_arm_mcontrol6_slot(0, trigger_pc, AI_ARCH_MCONTROL6_EXECUTE);
    ai_arch_arm_mcontrol6_slot(1, bad_paddr, AI_ARCH_MCONTROL6_STORE);

    uint64_t saved_mstatus = ai_arch_enable_machine_breakpoint_delivery();
    TEST_SETUP_EXCEPT();
    ai_arch_execute_trigger_store_once(bad_paddr, 0x1122334455667788ULL);
    if (!(excpt.triggered == true && excpt.cause == CAUSE_BKP)) {
        ai_arch_log_unexpected_trigger_result("unexpected store dual-trigger+access-fault priority result");
    }
    TEST_ASSERT("when both execute-PC and address triggers match a store that would otherwise raise direct M-mode access fault, Spike still reports breakpoint first",
        excpt.triggered == true &&
        excpt.cause == CAUSE_BKP
    );
    ai_arch_restore_machine_status(saved_mstatus);

    ai_arch_clear_mcontrol6_slot(0);
    saved_mstatus = ai_arch_enable_machine_breakpoint_delivery();
    TEST_SETUP_EXCEPT();
    ai_arch_execute_trigger_store_once(bad_paddr, 0x8877665544332211ULL);
    TEST_ASSERT("after clearing only the execute-PC trigger, the remaining store-address trigger still reports breakpoint on the same direct-M access-fault path",
        excpt.triggered == true &&
        excpt.cause == CAUSE_BKP
    );
    ai_arch_restore_machine_status(saved_mstatus);

    ai_arch_clear_mcontrol6_slot(1);
    TEST_SETUP_EXCEPT();
    ai_arch_execute_trigger_store_once(bad_paddr, 0x0fedcba987654321ULL);
    TEST_ASSERT("after clearing both trigger slots, the same store reports store access fault with the raw address in tval",
        excpt.triggered == true &&
        excpt.cause == CAUSE_SAF &&
        excpt.tval == bad_paddr
    );

    TEST_END();
}

bool ai_arch_store_execute_and_address_trigger_access_fault_repeated_breakpoint_then_fault_corner()
{
    TEST_START();

    uintptr_t trigger_pc = (uintptr_t)ai_arch_execute_trigger_store_once;
    uintptr_t bad_paddr = AI_ARCH_TRIGGER_BAD_PADDR;
    const uint64_t scratch0 = 0x0123456789abcdefULL;
    const uint64_t scratch1 = 0xfedcba9876543210ULL;

    ai_arch_trigger_mem[0] = scratch0;
    ai_arch_trigger_mem[1] = scratch1;
    ai_arch_clear_mcontrol6_slot(0);
    ai_arch_clear_mcontrol6_slot(1);
    ai_arch_arm_mcontrol6_slot(0, trigger_pc, AI_ARCH_MCONTROL6_EXECUTE);
    ai_arch_arm_mcontrol6_slot(1, bad_paddr, AI_ARCH_MCONTROL6_STORE);

    uint64_t saved_mstatus = ai_arch_enable_machine_breakpoint_delivery();
    TEST_SETUP_EXCEPT();
    ai_arch_execute_trigger_store_once(bad_paddr, 0x1122334455667788ULL);
    TEST_ASSERT("with both store trigger slots armed on a direct-M access-faulting PA, the first access reports breakpoint and leaves scratch state unchanged",
        excpt.triggered == true &&
        excpt.cause == CAUSE_BKP &&
        ai_arch_trigger_mem[0] == scratch0 &&
        ai_arch_trigger_mem[1] == scratch1
    );
    ai_arch_reenable_breakpoint_delivery();

    TEST_SETUP_EXCEPT();
    ai_arch_execute_trigger_store_once(bad_paddr, 0x8877665544332211ULL);
    TEST_ASSERT("with both store trigger slots still armed on the same direct-M access-faulting PA, the second access still reports breakpoint and still leaves scratch state unchanged",
        excpt.triggered == true &&
        excpt.cause == CAUSE_BKP &&
        ai_arch_trigger_mem[0] == scratch0 &&
        ai_arch_trigger_mem[1] == scratch1
    );
    ai_arch_restore_machine_status(saved_mstatus);

    ai_arch_clear_mcontrol6_slot(0);
    saved_mstatus = ai_arch_enable_machine_breakpoint_delivery();
    TEST_SETUP_EXCEPT();
    ai_arch_execute_trigger_store_once(bad_paddr, 0x0fedcba987654321ULL);
    TEST_ASSERT("after repeated dual-slot store breakpoint hits, clearing only the execute-PC slot still leaves the address trigger active on the same direct-M access-fault path",
        excpt.triggered == true &&
        excpt.cause == CAUSE_BKP &&
        ai_arch_trigger_mem[0] == scratch0 &&
        ai_arch_trigger_mem[1] == scratch1
    );
    ai_arch_restore_machine_status(saved_mstatus);

    ai_arch_clear_mcontrol6_slot(1);
    TEST_SETUP_EXCEPT();
    ai_arch_execute_trigger_store_once(bad_paddr, 0x5566778899aabbccULL);
    TEST_ASSERT("after clearing both store trigger slots, the same direct-M access falls back to store access fault with the raw address and still leaves scratch state unchanged",
        excpt.triggered == true &&
        excpt.cause == CAUSE_SAF &&
        excpt.tval == bad_paddr &&
        ai_arch_trigger_mem[0] == scratch0 &&
        ai_arch_trigger_mem[1] == scratch1
    );

    TEST_END();
}

bool ai_arch_store_execute_and_address_trigger_page_fault_priority_corner()
{
    TEST_START();

    uintptr_t trigger_pc = (uintptr_t)ai_arch_execute_trigger_store_once;
    uintptr_t pf_vaddr = hs_page_base(VSI_GI);

    ai_arch_prepare_hs_fault_page();
    ai_arch_clear_mcontrol6_slot(0);
    ai_arch_clear_mcontrol6_slot(1);
    ai_arch_arm_mcontrol6_slot(0, trigger_pc, AI_ARCH_MCONTROL6_EXECUTE);
    ai_arch_arm_mcontrol6_slot(1, pf_vaddr, AI_ARCH_MCONTROL6_STORE);

    uint64_t saved_mstatus = ai_arch_enable_machine_breakpoint_delivery();
    goto_priv(PRIV_HS);
    TEST_SETUP_EXCEPT();
    ai_arch_execute_trigger_store_once(pf_vaddr, 0xa1b2c3d4e5f60718ULL);
    if (!(excpt.triggered == true && excpt.cause == CAUSE_BKP)) {
        ai_arch_log_unexpected_trigger_result("unexpected store dual-trigger+page-fault priority result");
    }
    TEST_ASSERT("when both execute-PC and address triggers match a store that would otherwise raise HS page fault, Spike still reports breakpoint first",
        excpt.triggered == true &&
        excpt.cause == CAUSE_BKP
    );
    ai_arch_restore_machine_status(saved_mstatus);

    ai_arch_prepare_hs_fault_page();
    ai_arch_clear_mcontrol6_slot(0);
    saved_mstatus = ai_arch_enable_machine_breakpoint_delivery();
    goto_priv(PRIV_HS);
    TEST_SETUP_EXCEPT();
    ai_arch_execute_trigger_store_once(pf_vaddr, 0x0fedcba987654321ULL);
    TEST_ASSERT("after clearing only the execute-PC trigger, the remaining store-address trigger still reports breakpoint on the same HS page-fault path",
        excpt.triggered == true &&
        excpt.cause == CAUSE_BKP
    );
    ai_arch_restore_machine_status(saved_mstatus);

    ai_arch_prepare_hs_fault_page();
    ai_arch_clear_mcontrol6_slot(1);
    goto_priv(PRIV_HS);
    TEST_SETUP_EXCEPT();
    ai_arch_execute_trigger_store_once(pf_vaddr, 0x1122334455667788ULL);
    TEST_ASSERT("after clearing both trigger slots, the same store reports store page fault with the original faulting VA",
        excpt.triggered == true &&
        excpt.cause == CAUSE_SPF &&
        excpt.tval == pf_vaddr
    );

    TEST_END();
}

bool ai_arch_store_execute_and_address_trigger_page_fault_repeated_breakpoint_then_fault_corner()
{
    TEST_START();

    uintptr_t trigger_pc = (uintptr_t)ai_arch_execute_trigger_store_once;
    uintptr_t pf_vaddr = hs_page_base(VSI_GI);

    ai_arch_prepare_hs_fault_page();
    ai_arch_clear_mcontrol6_slot(0);
    ai_arch_clear_mcontrol6_slot(1);
    ai_arch_arm_mcontrol6_slot(0, trigger_pc, AI_ARCH_MCONTROL6_EXECUTE);
    ai_arch_arm_mcontrol6_slot(1, pf_vaddr, AI_ARCH_MCONTROL6_STORE);

    uint64_t saved_mstatus = ai_arch_enable_machine_breakpoint_delivery();
    goto_priv(PRIV_HS);
    TEST_SETUP_EXCEPT();
    ai_arch_execute_trigger_store_once(pf_vaddr, 0xa1b2c3d4e5f60718ULL);
    TEST_ASSERT("with both store trigger slots armed on an HS page-faulting VA, the first access reports breakpoint",
        excpt.triggered == true &&
        excpt.cause == CAUSE_BKP
    );

    ai_arch_prepare_hs_fault_page();
    goto_priv(PRIV_HS);
    TEST_SETUP_EXCEPT();
    ai_arch_execute_trigger_store_once(pf_vaddr, 0x0fedcba987654321ULL);
    TEST_ASSERT("with both store trigger slots still armed on the same HS page-faulting VA, the second access still reports breakpoint",
        excpt.triggered == true &&
        excpt.cause == CAUSE_BKP
    );
    ai_arch_restore_machine_status(saved_mstatus);

    ai_arch_prepare_hs_fault_page();
    ai_arch_clear_mcontrol6_slot(0);
    saved_mstatus = ai_arch_enable_machine_breakpoint_delivery();
    goto_priv(PRIV_HS);
    TEST_SETUP_EXCEPT();
    ai_arch_execute_trigger_store_once(pf_vaddr, 0x1122334455667788ULL);
    TEST_ASSERT("after repeated dual-slot breakpoint hits, clearing only the execute-PC slot still leaves the store-address trigger active on the same page-fault path",
        excpt.triggered == true &&
        excpt.cause == CAUSE_BKP
    );
    ai_arch_restore_machine_status(saved_mstatus);

    ai_arch_prepare_hs_fault_page();
    ai_arch_clear_mcontrol6_slot(1);
    goto_priv(PRIV_HS);
    TEST_SETUP_EXCEPT();
    ai_arch_execute_trigger_store_once(pf_vaddr, 0x8877665544332211ULL);
    TEST_ASSERT("after clearing both store trigger slots, the same access falls back to the original store page fault with the raw faulting VA",
        excpt.triggered == true &&
        excpt.cause == CAUSE_SPF &&
        excpt.tval == pf_vaddr
    );

    TEST_END();
}

bool ai_arch_amo_address_trigger_breakpoint()
{
    TEST_START();

    uintptr_t addr = ai_arch_trigger_addr(0);
    const uint64_t initial_value = 0x0000001055667788ULL;
    const uint32_t swap_value = 0xa1b2c3d4U;

    ai_arch_trigger_mem[0] = initial_value;

    ai_arch_arm_mcontrol6_0(addr, AI_ARCH_MCONTROL6_AMO);
    uint64_t saved_mstatus = ai_arch_enable_machine_breakpoint_delivery();

    TEST_SETUP_EXCEPT();
    amoswap_w(addr, swap_value);
    if (!(excpt.triggered == true && excpt.cause == CAUSE_BKP)) {
        ai_arch_log_unexpected_trigger_result("unexpected AMO address-trigger result");
    }
    TEST_ASSERT("an AMO address trigger configured through mcontrol6 reports breakpoint",
        excpt.triggered == true &&
        excpt.cause == CAUSE_BKP &&
        ai_arch_trigger_mem[0] == initial_value
    );
    ai_arch_restore_machine_status(saved_mstatus);

    ai_arch_clear_mcontrol6_0();
    ai_arch_trigger_mem[0] = initial_value;

    TEST_SETUP_EXCEPT();
    uint32_t old_value = amoswap_w(addr, swap_value);
    TEST_ASSERT("the same AMO completes normally after clearing the trigger",
        excpt.triggered == false &&
        old_value == 0x55667788U &&
        ai_arch_trigger_mem[0] == 0x00000010a1b2c3d4ULL
    );

    TEST_END();
}

bool ai_arch_amo_address_trigger_repeated_breakpoint_no_side_effect_corner()
{
    TEST_START();

    uintptr_t addr = ai_arch_trigger_addr(0);
    const uint64_t initial_value = 0x0000001055667788ULL;
    const uint32_t first_swap_value = 0xa1b2c3d4U;
    const uint32_t second_swap_value = 0x11223344U;

    ai_arch_trigger_mem[0] = initial_value;

    ai_arch_arm_mcontrol6_0(addr, AI_ARCH_MCONTROL6_AMO);
    uint64_t saved_mstatus = ai_arch_enable_machine_breakpoint_delivery();

    TEST_SETUP_EXCEPT();
    amoswap_w(addr, first_swap_value);
    if (!(excpt.triggered == true && excpt.cause == CAUSE_BKP)) {
        ai_arch_log_unexpected_trigger_result("unexpected repeated AMO address-trigger first-hit result");
    }
    TEST_ASSERT("the first AMO address trigger hit reports breakpoint and suppresses the atomic update",
        excpt.triggered == true &&
        excpt.cause == CAUSE_BKP &&
        ai_arch_trigger_mem[0] == initial_value
    );
    ai_arch_reenable_breakpoint_delivery();

    TEST_SETUP_EXCEPT();
    amoswap_w(addr, second_swap_value);
    if (!(excpt.triggered == true && excpt.cause == CAUSE_BKP)) {
        ai_arch_log_unexpected_trigger_result("unexpected repeated AMO address-trigger second-hit result");
    }
    TEST_ASSERT("the second AMO address trigger hit still reports breakpoint and still leaves memory unchanged",
        excpt.triggered == true &&
        excpt.cause == CAUSE_BKP &&
        ai_arch_trigger_mem[0] == initial_value
    );
    ai_arch_restore_machine_status(saved_mstatus);

    ai_arch_clear_mcontrol6_0();
    ai_arch_trigger_mem[0] = initial_value;

    TEST_SETUP_EXCEPT();
    uint32_t old_value = amoswap_w(addr, first_swap_value);
    TEST_ASSERT("after clearing the repeated AMO address trigger, the same AMO completes normally with the expected RV64 low-word swap semantics",
        excpt.triggered == false &&
        old_value == 0x55667788U &&
        ai_arch_trigger_mem[0] == 0x00000010a1b2c3d4ULL
    );

    TEST_END();
}

bool ai_arch_amo_execute_and_address_trigger_both_breakpoint_corner()
{
    TEST_START();

    uintptr_t addr = ai_arch_trigger_addr(0);
    uintptr_t trigger_pc = (uintptr_t)ai_arch_execute_trigger_amoswap_w_once;
    const uint64_t initial_value = 0x0000001055667788ULL;
    const uint32_t swap_value = 0xa1b2c3d4U;

    ai_arch_trigger_mem[0] = initial_value;
    ai_arch_clear_mcontrol6_slot(0);
    ai_arch_clear_mcontrol6_slot(1);
    ai_arch_arm_mcontrol6_slot(0, trigger_pc, AI_ARCH_MCONTROL6_EXECUTE);
    ai_arch_arm_mcontrol6_slot(1, addr, AI_ARCH_MCONTROL6_AMO);

    uint64_t saved_mstatus = ai_arch_enable_machine_breakpoint_delivery();
    TEST_SETUP_EXCEPT();
    ai_arch_execute_trigger_amoswap_w_once(addr, swap_value);
    if (!(excpt.triggered == true && excpt.cause == CAUSE_BKP)) {
        ai_arch_log_unexpected_trigger_result("unexpected AMO execute-PC+address dual-trigger result");
    }
    TEST_ASSERT("when both execute-PC and address triggers match the same AMO, Spike reports breakpoint before the atomic update commits",
        excpt.triggered == true &&
        excpt.cause == CAUSE_BKP &&
        ai_arch_trigger_mem[0] == initial_value
    );
    ai_arch_restore_machine_status(saved_mstatus);

    ai_arch_clear_mcontrol6_slot(0);
    saved_mstatus = ai_arch_enable_machine_breakpoint_delivery();
    TEST_SETUP_EXCEPT();
    ai_arch_execute_trigger_amoswap_w_once(addr, swap_value);
    if (!(excpt.triggered == true && excpt.cause == CAUSE_BKP)) {
        ai_arch_log_unexpected_trigger_result("unexpected AMO address-only dual-trigger follow-up result");
    }
    TEST_ASSERT("after clearing only the execute-PC trigger, the remaining AMO address trigger still reports breakpoint and still suppresses the atomic update",
        excpt.triggered == true &&
        excpt.cause == CAUSE_BKP &&
        ai_arch_trigger_mem[0] == initial_value
    );
    ai_arch_restore_machine_status(saved_mstatus);

    ai_arch_clear_mcontrol6_slot(1);
    TEST_SETUP_EXCEPT();
    uint32_t old_value = ai_arch_execute_trigger_amoswap_w_once(addr, swap_value);
    TEST_ASSERT("after clearing both trigger slots, the same AMO completes normally with the expected RV64 low-word swap semantics",
        excpt.triggered == false &&
        old_value == 0x55667788U &&
        ai_arch_trigger_mem[0] == 0x00000010a1b2c3d4ULL
    );

    TEST_END();
}

bool ai_arch_amo_execute_and_address_trigger_page_fault_spike_corner()
{
    TEST_START();

    uintptr_t trigger_pc = (uintptr_t)ai_arch_execute_trigger_amoswap_w_once;
    uintptr_t pf_vaddr = hs_page_base(VSI_GI);

    ai_arch_prepare_hs_fault_page();
    ai_arch_clear_mcontrol6_slot(0);
    ai_arch_clear_mcontrol6_slot(1);
    ai_arch_arm_mcontrol6_slot(0, trigger_pc, AI_ARCH_MCONTROL6_EXECUTE);
    ai_arch_arm_mcontrol6_slot(1, pf_vaddr, AI_ARCH_MCONTROL6_AMO);

    uint64_t saved_mstatus = ai_arch_enable_machine_breakpoint_delivery();
    goto_priv(PRIV_HS);
    TEST_SETUP_EXCEPT();
    ai_arch_execute_trigger_amoswap_w_once(pf_vaddr, 0xa1b2c3d4U);
    if (!(excpt.triggered == true && excpt.cause == CAUSE_BKP)) {
        ai_arch_log_unexpected_trigger_result("unexpected AMO dual-trigger+page-fault first-hit result");
    }
    TEST_ASSERT("when both execute-PC and address triggers match an AMO that would otherwise raise HS store page fault, Spike still reports breakpoint first because the execute trigger is present",
        excpt.triggered == true &&
        excpt.cause == CAUSE_BKP
    );
    ai_arch_restore_machine_status(saved_mstatus);

    ai_arch_prepare_hs_fault_page();
    ai_arch_clear_mcontrol6_slot(0);
    saved_mstatus = ai_arch_enable_machine_breakpoint_delivery();
    goto_priv(PRIV_HS);
    TEST_SETUP_EXCEPT();
    ai_arch_execute_trigger_amoswap_w_once(pf_vaddr, 0x55667788U);
    if (!(excpt.triggered == true &&
        excpt.cause == CAUSE_BKP &&
        excpt.tval == pf_vaddr)) {
        ai_arch_log_unexpected_trigger_result("unexpected AMO address-only dual-trigger page-fault Spike corner result");
    }
    TEST_ASSERT("after clearing only the execute-PC trigger, the remaining AMO address trigger still reports breakpoint before the HS store page fault",
        excpt.triggered == true &&
        excpt.cause == CAUSE_BKP &&
        excpt.tval == pf_vaddr
    );
    ai_arch_restore_machine_status(saved_mstatus);

    ai_arch_prepare_hs_fault_page();
    ai_arch_clear_mcontrol6_slot(1);
    goto_priv(PRIV_HS);
    TEST_SETUP_EXCEPT();
    ai_arch_execute_trigger_amoswap_w_once(pf_vaddr, 0x11223344U);
    TEST_ASSERT("after clearing both trigger slots, the same HS AMO still reports store page fault with the original faulting VA",
        excpt.triggered == true &&
        excpt.cause == CAUSE_SPF &&
        excpt.tval == pf_vaddr
    );

    TEST_END();
}

bool ai_arch_amo_execute_and_address_trigger_misaligned_access_fault_spike_corner()
{
    TEST_START();

    uintptr_t trigger_pc = (uintptr_t)ai_arch_execute_trigger_amoswap_w_once;
    uintptr_t misaligned_vaddr = hs_page_base(VSRWX_GURWX) + 1;

    goto_priv(PRIV_HS);
    hspt_init();
    sfence_vma();

    ai_arch_clear_mcontrol6_slot(0);
    ai_arch_clear_mcontrol6_slot(1);
    ai_arch_arm_mcontrol6_slot(0, trigger_pc, AI_ARCH_MCONTROL6_EXECUTE);
    ai_arch_arm_mcontrol6_slot(1, misaligned_vaddr, AI_ARCH_MCONTROL6_AMO);

    uint64_t saved_mstatus = ai_arch_enable_machine_breakpoint_delivery();
    goto_priv(PRIV_HS);
    TEST_SETUP_EXCEPT();
    ai_arch_execute_trigger_amoswap_w_once(misaligned_vaddr, 0xa1b2c3d4U);
    if (!(excpt.triggered == true && excpt.cause == CAUSE_BKP)) {
        ai_arch_log_unexpected_trigger_result("unexpected AMO dual-trigger+misaligned first-hit result");
    }
    TEST_ASSERT("when both execute-PC and address triggers match an AMO that would otherwise raise misaligned-driven store access fault, Spike still reports breakpoint first because the execute trigger is present",
        excpt.triggered == true &&
        excpt.cause == CAUSE_BKP
    );
    ai_arch_restore_machine_status(saved_mstatus);

    goto_priv(PRIV_HS);
    hspt_init();
    sfence_vma();

    ai_arch_clear_mcontrol6_slot(0);
    saved_mstatus = ai_arch_enable_machine_breakpoint_delivery();
    goto_priv(PRIV_HS);
    TEST_SETUP_EXCEPT();
    ai_arch_execute_trigger_amoswap_w_once(misaligned_vaddr, 0x55667788U);
    if (!(excpt.triggered == true &&
        excpt.cause == CAUSE_BKP &&
        excpt.tval == misaligned_vaddr)) {
        ai_arch_log_unexpected_trigger_result("unexpected AMO address-only dual-trigger misaligned Spike corner result");
    }
    TEST_ASSERT("after clearing only the execute-PC trigger, the remaining AMO address trigger still reports breakpoint before the misaligned-driven store access fault",
        excpt.triggered == true &&
        excpt.cause == CAUSE_BKP &&
        excpt.tval == misaligned_vaddr
    );
    ai_arch_restore_machine_status(saved_mstatus);

    goto_priv(PRIV_HS);
    hspt_init();
    sfence_vma();

    ai_arch_clear_mcontrol6_slot(1);
    TEST_SETUP_EXCEPT();
    ai_arch_execute_trigger_amoswap_w_once(misaligned_vaddr, 0x11223344U);
    TEST_ASSERT("after clearing both trigger slots, the same HS AMO still reports store access fault with the original misaligned VA in tval",
        excpt.triggered == true &&
        excpt.cause == CAUSE_SAF &&
        excpt.tval == misaligned_vaddr
    );

    TEST_END();
}

bool ai_arch_amo_execute_and_address_trigger_access_fault_priority_corner()
{
    TEST_START();

    uintptr_t trigger_pc = (uintptr_t)ai_arch_execute_trigger_amoswap_w_once;
    uintptr_t bad_paddr = AI_ARCH_TRIGGER_BAD_PADDR;

    ai_arch_clear_mcontrol6_slot(0);
    ai_arch_clear_mcontrol6_slot(1);
    ai_arch_arm_mcontrol6_slot(0, trigger_pc, AI_ARCH_MCONTROL6_EXECUTE);
    ai_arch_arm_mcontrol6_slot(1, bad_paddr, AI_ARCH_MCONTROL6_AMO);

    uint64_t saved_mstatus = ai_arch_enable_machine_breakpoint_delivery();
    TEST_SETUP_EXCEPT();
    ai_arch_execute_trigger_amoswap_w_once(bad_paddr, 0xa1b2c3d4U);
    if (!(excpt.triggered == true && excpt.cause == CAUSE_BKP)) {
        ai_arch_log_unexpected_trigger_result("unexpected AMO dual-trigger+access-fault priority result");
    }
    TEST_ASSERT("when both execute-PC and address triggers match an AMO that would otherwise raise direct M-mode access fault, Spike still reports breakpoint first",
        excpt.triggered == true &&
        excpt.cause == CAUSE_BKP
    );
    ai_arch_restore_machine_status(saved_mstatus);

    ai_arch_clear_mcontrol6_slot(0);
    saved_mstatus = ai_arch_enable_machine_breakpoint_delivery();
    TEST_SETUP_EXCEPT();
    ai_arch_execute_trigger_amoswap_w_once(bad_paddr, 0x55667788U);
    TEST_ASSERT("after clearing only the execute-PC trigger, the remaining AMO address trigger still reports breakpoint on the same direct-M access-fault path",
        excpt.triggered == true &&
        excpt.cause == CAUSE_BKP
    );
    ai_arch_restore_machine_status(saved_mstatus);

    ai_arch_clear_mcontrol6_slot(1);
    TEST_SETUP_EXCEPT();
    ai_arch_execute_trigger_amoswap_w_once(bad_paddr, 0x11223344U);
    TEST_ASSERT("after clearing both trigger slots, the same AMO reports store access fault with the raw address in tval",
        excpt.triggered == true &&
        excpt.cause == CAUSE_SAF &&
        excpt.tval == bad_paddr
    );

    TEST_END();
}

bool ai_arch_amo_execute_and_address_trigger_access_fault_repeated_breakpoint_then_fault_corner()
{
    TEST_START();

    uintptr_t trigger_pc = (uintptr_t)ai_arch_execute_trigger_amoswap_w_once;
    uintptr_t bad_paddr = AI_ARCH_TRIGGER_BAD_PADDR;
    const uint64_t scratch0 = 0x13579bdf2468ace0ULL;
    const uint64_t scratch1 = 0x0eca8642fdb97531ULL;

    ai_arch_trigger_mem[0] = scratch0;
    ai_arch_trigger_mem[1] = scratch1;
    ai_arch_clear_mcontrol6_slot(0);
    ai_arch_clear_mcontrol6_slot(1);
    ai_arch_arm_mcontrol6_slot(0, trigger_pc, AI_ARCH_MCONTROL6_EXECUTE);
    ai_arch_arm_mcontrol6_slot(1, bad_paddr, AI_ARCH_MCONTROL6_AMO);

    uint64_t saved_mstatus = ai_arch_enable_machine_breakpoint_delivery();
    TEST_SETUP_EXCEPT();
    ai_arch_execute_trigger_amoswap_w_once(bad_paddr, 0xa1b2c3d4U);
    TEST_ASSERT("with both AMO trigger slots armed on a direct-M access-faulting PA, the first access reports breakpoint and leaves scratch state unchanged",
        excpt.triggered == true &&
        excpt.cause == CAUSE_BKP &&
        ai_arch_trigger_mem[0] == scratch0 &&
        ai_arch_trigger_mem[1] == scratch1
    );
    ai_arch_reenable_breakpoint_delivery();

    TEST_SETUP_EXCEPT();
    ai_arch_execute_trigger_amoswap_w_once(bad_paddr, 0x55667788U);
    TEST_ASSERT("with both AMO trigger slots still armed on the same direct-M access-faulting PA, the second access still reports breakpoint and still leaves scratch state unchanged",
        excpt.triggered == true &&
        excpt.cause == CAUSE_BKP &&
        ai_arch_trigger_mem[0] == scratch0 &&
        ai_arch_trigger_mem[1] == scratch1
    );
    ai_arch_restore_machine_status(saved_mstatus);

    ai_arch_clear_mcontrol6_slot(0);
    saved_mstatus = ai_arch_enable_machine_breakpoint_delivery();
    TEST_SETUP_EXCEPT();
    ai_arch_execute_trigger_amoswap_w_once(bad_paddr, 0x11223344U);
    TEST_ASSERT("after repeated dual-slot AMO breakpoint hits, clearing only the execute-PC slot still leaves the address trigger active on the same direct-M access-fault path",
        excpt.triggered == true &&
        excpt.cause == CAUSE_BKP &&
        ai_arch_trigger_mem[0] == scratch0 &&
        ai_arch_trigger_mem[1] == scratch1
    );
    ai_arch_restore_machine_status(saved_mstatus);

    ai_arch_clear_mcontrol6_slot(1);
    TEST_SETUP_EXCEPT();
    ai_arch_execute_trigger_amoswap_w_once(bad_paddr, 0xdeadbeefU);
    TEST_ASSERT("after clearing both AMO trigger slots, the same direct-M access falls back to store access fault with the raw address and still leaves scratch state unchanged",
        excpt.triggered == true &&
        excpt.cause == CAUSE_SAF &&
        excpt.tval == bad_paddr &&
        ai_arch_trigger_mem[0] == scratch0 &&
        ai_arch_trigger_mem[1] == scratch1
    );

    TEST_END();
}

bool ai_arch_amo_trigger_access_fault_priority()
{
    TEST_START();

    uintptr_t bad_paddr = AI_ARCH_TRIGGER_BAD_PADDR;

    ai_arch_arm_mcontrol6_0(bad_paddr, AI_ARCH_MCONTROL6_AMO);
    uint64_t saved_mstatus = ai_arch_enable_machine_breakpoint_delivery();

    TEST_SETUP_EXCEPT();
    amoswap_w(bad_paddr, 0xa1b2c3d4U);
    if (!(excpt.triggered == true && excpt.cause == CAUSE_BKP)) {
        ai_arch_log_unexpected_trigger_result("unexpected AMO address-trigger+access-fault priority result");
    }
    TEST_ASSERT("an AMO address trigger takes priority when the same direct M-mode AMO would otherwise raise store access fault",
        excpt.triggered == true &&
        excpt.cause == CAUSE_BKP
    );
    ai_arch_restore_machine_status(saved_mstatus);
    ai_arch_clear_mcontrol6_0();

    TEST_SETUP_EXCEPT();
    amoswap_w(bad_paddr, 0x55667788U);
    TEST_ASSERT("after clearing the AMO address trigger, the same direct M-mode AMO reports store access fault with the raw address in tval",
        excpt.triggered == true &&
        excpt.cause == CAUSE_SAF &&
        excpt.tval == bad_paddr
    );

    TEST_END();
}

bool ai_arch_amo_trigger_access_fault_repeated_breakpoint_then_fault_corner()
{
    TEST_START();

    uintptr_t bad_paddr = AI_ARCH_TRIGGER_BAD_PADDR;
    const uint64_t scratch0 = 0x89abcdef01234567ULL;
    const uint64_t scratch1 = 0x76543210fedcba98ULL;

    ai_arch_trigger_mem[0] = scratch0;
    ai_arch_trigger_mem[1] = scratch1;
    ai_arch_arm_mcontrol6_0(bad_paddr, AI_ARCH_MCONTROL6_AMO);
    uint64_t saved_mstatus = ai_arch_enable_machine_breakpoint_delivery();

    TEST_SETUP_EXCEPT();
    amoswap_w(bad_paddr, 0xa1b2c3d4U);
    TEST_ASSERT("with an AMO address trigger armed on a direct-M access-faulting PA, the first access reports breakpoint and leaves scratch state unchanged",
        excpt.triggered == true &&
        excpt.cause == CAUSE_BKP &&
        ai_arch_trigger_mem[0] == scratch0 &&
        ai_arch_trigger_mem[1] == scratch1
    );
    ai_arch_reenable_breakpoint_delivery();

    TEST_SETUP_EXCEPT();
    amoswap_w(bad_paddr, 0x55667788U);
    TEST_ASSERT("with the same AMO address trigger still armed on that direct-M access-faulting PA, the second access still reports breakpoint and still leaves scratch state unchanged",
        excpt.triggered == true &&
        excpt.cause == CAUSE_BKP &&
        ai_arch_trigger_mem[0] == scratch0 &&
        ai_arch_trigger_mem[1] == scratch1
    );
    ai_arch_restore_machine_status(saved_mstatus);
    ai_arch_clear_mcontrol6_0();

    TEST_SETUP_EXCEPT();
    amoswap_w(bad_paddr, 0x11223344U);
    TEST_ASSERT("after clearing the repeated AMO address trigger, the same direct-M access falls back to store access fault with the raw address and still leaves scratch state unchanged",
        excpt.triggered == true &&
        excpt.cause == CAUSE_SAF &&
        excpt.tval == bad_paddr &&
        ai_arch_trigger_mem[0] == scratch0 &&
        ai_arch_trigger_mem[1] == scratch1
    );

    TEST_END();
}

bool ai_arch_amo_execute_trigger_access_fault_priority()
{
    TEST_START();

    uintptr_t trigger_pc = (uintptr_t)ai_arch_execute_trigger_amoswap_w_once;
    uintptr_t bad_paddr = AI_ARCH_TRIGGER_BAD_PADDR;

    ai_arch_arm_mcontrol6_0(trigger_pc, AI_ARCH_MCONTROL6_EXECUTE);
    uint64_t saved_mstatus = ai_arch_enable_machine_breakpoint_delivery();

    TEST_SETUP_EXCEPT();
    ai_arch_execute_trigger_amoswap_w_once(bad_paddr, 0xa1b2c3d4U);
    if (!(excpt.triggered == true && excpt.cause == CAUSE_BKP)) {
        ai_arch_log_unexpected_trigger_result("unexpected AMO execute-PC trigger+access-fault priority result");
    }
    TEST_ASSERT("an AMO execute-PC trigger takes priority when the same direct M-mode AMO would otherwise raise store access fault",
        excpt.triggered == true &&
        excpt.cause == CAUSE_BKP
    );
    ai_arch_restore_machine_status(saved_mstatus);
    ai_arch_clear_mcontrol6_0();

    TEST_SETUP_EXCEPT();
    ai_arch_execute_trigger_amoswap_w_once(bad_paddr, 0x55667788U);
    TEST_ASSERT("after clearing the AMO execute-PC trigger, the same direct M-mode AMO reports store access fault with the raw address in tval",
        excpt.triggered == true &&
        excpt.cause == CAUSE_SAF &&
        excpt.tval == bad_paddr
    );

    TEST_END();
}

bool ai_arch_amo_trigger_misaligned_access_fault_spike_corner()
{
    TEST_START();

    uintptr_t misaligned_vaddr = hs_page_base(VSRWX_GURWX) + 1;

    goto_priv(PRIV_HS);
    hspt_init();
    sfence_vma();

    ai_arch_arm_mcontrol6_0(misaligned_vaddr, AI_ARCH_MCONTROL6_AMO);
    uint64_t saved_mstatus = ai_arch_enable_machine_breakpoint_delivery();

    goto_priv(PRIV_HS);
    TEST_SETUP_EXCEPT();
    amoswap_w(misaligned_vaddr, 0xa1b2c3d4U);
    if (!(excpt.triggered == true &&
        excpt.cause == CAUSE_BKP &&
        excpt.tval == misaligned_vaddr)) {
        ai_arch_log_unexpected_trigger_result("unexpected AMO address-trigger+misaligned access-fault Spike corner result");
    }
    TEST_ASSERT("an AMO address trigger reports breakpoint before the misaligned-driven store access fault",
        excpt.triggered == true &&
        excpt.cause == CAUSE_BKP &&
        excpt.tval == misaligned_vaddr
    );
    ai_arch_restore_machine_status(saved_mstatus);
    ai_arch_clear_mcontrol6_0();
    ai_arch_sync_trigger_state();

    goto_priv(PRIV_HS);
    hspt_init();
    sfence_vma();

    TEST_SETUP_EXCEPT();
    amoswap_w(misaligned_vaddr, 0x55667788U);
    TEST_ASSERT("after clearing the AMO address trigger, the same HS AMO still reports store access fault with the misaligned VA in tval",
        excpt.triggered == true &&
        excpt.cause == CAUSE_SAF &&
        excpt.tval == misaligned_vaddr
    );

    TEST_END();
}

bool ai_arch_amo_trigger_misaligned_repeated_breakpoints_then_aligned_recovers_spike_corner()
{
    TEST_START();

    const uint64_t initial_target = 0x1122334499aabbccULL;
    const uint64_t adjacent_seed = 0x5566778899aabbccULL;
    const uint32_t recovered_word = 0x7fedcba9U;
    const uint64_t expected_old_value = 0xffffffff99aabbccULL;
    const uint64_t expected_target_after_recovery = 0x112233447fedcba9ULL;

    uintptr_t paddr = phys_page_base(VSRWX_GURWX);
    uintptr_t base_vaddr = hs_page_base(VSRWX_GURWX);
    uintptr_t misaligned_vaddr = base_vaddr + 1U;

    goto_priv(PRIV_M);
    write64(paddr, initial_target);
    write64(paddr + sizeof(uint64_t), adjacent_seed);

    goto_priv(PRIV_HS);
    hspt_init();
    sfence_vma();

    ai_arch_arm_mcontrol6_0(misaligned_vaddr, AI_ARCH_MCONTROL6_AMO);
    uint64_t saved_mstatus = ai_arch_enable_machine_breakpoint_delivery();

    goto_priv(PRIV_HS);
    TEST_SETUP_EXCEPT();
    amoswap_w(misaligned_vaddr, 0xa1b2c3d4U);
    if (!(excpt.triggered == true &&
        excpt.cause == CAUSE_BKP &&
        excpt.tval == misaligned_vaddr)) {
        ai_arch_log_unexpected_trigger_result("unexpected repeated AMO address-trigger+misaligned first-hit Spike corner result");
    }
    TEST_ASSERT("with an AMO address trigger armed, the first misaligned HS AMO reports breakpoint with the raw misaligned VA in tval",
        excpt.triggered == true &&
        excpt.cause == CAUSE_BKP &&
        excpt.tval == misaligned_vaddr
    );

    ai_arch_reenable_breakpoint_delivery();
    goto_priv(PRIV_HS);
    TEST_SETUP_EXCEPT();
    amoswap_w(misaligned_vaddr, 0x55667788U);
    if (!(excpt.triggered == true &&
        excpt.cause == CAUSE_BKP &&
        excpt.tval == misaligned_vaddr)) {
        ai_arch_log_unexpected_trigger_result("unexpected repeated AMO address-trigger+misaligned second-hit Spike corner result");
    }
    TEST_ASSERT("with the same AMO address trigger still armed, a second misaligned HS AMO again reports breakpoint with the same raw misaligned VA",
        excpt.triggered == true &&
        excpt.cause == CAUSE_BKP &&
        excpt.tval == misaligned_vaddr
    );
    ai_arch_restore_machine_status(saved_mstatus);
    ai_arch_clear_mcontrol6_0();

    goto_priv(PRIV_M);
    TEST_ASSERT("repeated AMO address-trigger breakpoints on the misaligned path leave both the target doubleword and its adjacent doubleword unchanged",
        read64(paddr) == initial_target &&
        read64(paddr + sizeof(uint64_t)) == adjacent_seed
    );

    goto_priv(PRIV_HS);
    hspt_init();
    sfence_vma();

    TEST_SETUP_EXCEPT();
    uint64_t old_value = ai_arch_trigger_amoswap_w_raw64(base_vaddr, recovered_word);
    TEST_ASSERT("after repeated AMO address-trigger breakpoints on the misaligned path, a subsequent aligned HS amoswap.w to the same backing word still succeeds and returns the sign-extended old word",
        excpt.triggered == false &&
        old_value == expected_old_value
    );

    goto_priv(PRIV_M);
    TEST_ASSERT("the aligned recovery AMO after repeated AMO address-trigger breakpoints on the misaligned path updates only the target lower lane while preserving the adjacent doubleword",
        read64(paddr) == expected_target_after_recovery &&
        read64(paddr + sizeof(uint64_t)) == adjacent_seed
    );

    TEST_END();
}

bool ai_arch_amo_execute_trigger_misaligned_access_fault_priority()
{
    TEST_START();

    uintptr_t trigger_pc = (uintptr_t)ai_arch_execute_trigger_amoswap_w_once;
    uintptr_t misaligned_vaddr = hs_page_base(VSRWX_GURWX) + 1;

    goto_priv(PRIV_HS);
    hspt_init();
    sfence_vma();

    ai_arch_arm_mcontrol6_0(trigger_pc, AI_ARCH_MCONTROL6_EXECUTE);
    uint64_t saved_mstatus = ai_arch_enable_machine_breakpoint_delivery();

    goto_priv(PRIV_HS);
    TEST_SETUP_EXCEPT();
    ai_arch_execute_trigger_amoswap_w_once(misaligned_vaddr, 0xa1b2c3d4U);
    if (!(excpt.triggered == true && excpt.cause == CAUSE_BKP)) {
        ai_arch_log_unexpected_trigger_result("unexpected AMO execute-PC trigger+misaligned access-fault priority result");
    }
    TEST_ASSERT("an AMO execute-PC trigger takes priority when the same HS AMO would otherwise raise misaligned-driven store access fault",
        excpt.triggered == true &&
        excpt.cause == CAUSE_BKP
    );
    ai_arch_restore_machine_status(saved_mstatus);
    ai_arch_clear_mcontrol6_0();
    ai_arch_sync_trigger_state();

    goto_priv(PRIV_HS);
    hspt_init();
    sfence_vma();

    TEST_SETUP_EXCEPT();
    ai_arch_execute_trigger_amoswap_w_once(misaligned_vaddr, 0x55667788U);
    TEST_ASSERT("after clearing the AMO execute-PC trigger, the same HS AMO reports store access fault with the misaligned VA in tval",
        excpt.triggered == true &&
        excpt.cause == CAUSE_SAF &&
        excpt.tval == misaligned_vaddr
    );

    TEST_END();
}

bool ai_arch_load_trigger_access_fault_priority()
{
    TEST_START();

    uintptr_t bad_paddr = AI_ARCH_TRIGGER_BAD_PADDR;

    ai_arch_arm_mcontrol6_0(bad_paddr, AI_ARCH_MCONTROL6_LOAD);
    uint64_t saved_mstatus = ai_arch_enable_machine_breakpoint_delivery();

    TEST_SETUP_EXCEPT();
    ld(bad_paddr);
    if (!(excpt.triggered == true && excpt.cause == CAUSE_BKP)) {
        ai_arch_log_unexpected_trigger_result("unexpected load trigger+access-fault priority result");
    }
    TEST_ASSERT("a load address trigger takes priority when the same direct M-mode load would otherwise raise access fault",
        excpt.triggered == true &&
        excpt.cause == CAUSE_BKP
    );
    ai_arch_restore_machine_status(saved_mstatus);
    ai_arch_clear_mcontrol6_0();

    TEST_SETUP_EXCEPT();
    ld(bad_paddr);
    TEST_ASSERT("after clearing the trigger, the same direct M-mode load reports load access fault with the raw address in tval",
        excpt.triggered == true &&
        excpt.cause == CAUSE_LAF &&
        excpt.tval == bad_paddr
    );

    TEST_END();
}

bool ai_arch_load_trigger_access_fault_repeated_breakpoint_then_fault_corner()
{
    TEST_START();

    uintptr_t bad_paddr = AI_ARCH_TRIGGER_BAD_PADDR;
    const uint64_t scratch0 = 0xa5a55a5aa5a55a5aULL;
    const uint64_t scratch1 = 0x5a5aa5a55a5aa5a5ULL;

    ai_arch_trigger_mem[0] = scratch0;
    ai_arch_trigger_mem[1] = scratch1;
    ai_arch_arm_mcontrol6_0(bad_paddr, AI_ARCH_MCONTROL6_LOAD);
    uint64_t saved_mstatus = ai_arch_enable_machine_breakpoint_delivery();

    TEST_SETUP_EXCEPT();
    ld(bad_paddr);
    TEST_ASSERT("with a load address trigger armed on a direct-M access-faulting PA, the first access reports breakpoint and leaves scratch state unchanged",
        excpt.triggered == true &&
        excpt.cause == CAUSE_BKP &&
        ai_arch_trigger_mem[0] == scratch0 &&
        ai_arch_trigger_mem[1] == scratch1
    );
    ai_arch_reenable_breakpoint_delivery();

    TEST_SETUP_EXCEPT();
    ld(bad_paddr);
    TEST_ASSERT("with the same load address trigger still armed on that direct-M access-faulting PA, the second access still reports breakpoint and still leaves scratch state unchanged",
        excpt.triggered == true &&
        excpt.cause == CAUSE_BKP &&
        ai_arch_trigger_mem[0] == scratch0 &&
        ai_arch_trigger_mem[1] == scratch1
    );
    ai_arch_restore_machine_status(saved_mstatus);
    ai_arch_clear_mcontrol6_0();

    TEST_SETUP_EXCEPT();
    ld(bad_paddr);
    TEST_ASSERT("after clearing the repeated load address trigger, the same direct-M access falls back to load access fault with the raw address and still leaves scratch state unchanged",
        excpt.triggered == true &&
        excpt.cause == CAUSE_LAF &&
        excpt.tval == bad_paddr &&
        ai_arch_trigger_mem[0] == scratch0 &&
        ai_arch_trigger_mem[1] == scratch1
    );

    TEST_END();
}

bool ai_arch_load_execute_trigger_access_fault_priority()
{
    TEST_START();

    uintptr_t trigger_pc = (uintptr_t)ai_arch_execute_trigger_load_once;
    uintptr_t bad_paddr = AI_ARCH_TRIGGER_BAD_PADDR;

    ai_arch_arm_mcontrol6_0(trigger_pc, AI_ARCH_MCONTROL6_EXECUTE);
    uint64_t saved_mstatus = ai_arch_enable_machine_breakpoint_delivery();

    TEST_SETUP_EXCEPT();
    ai_arch_execute_trigger_load_once(bad_paddr);
    if (!(excpt.triggered == true && excpt.cause == CAUSE_BKP)) {
        ai_arch_log_unexpected_trigger_result("unexpected load execute-PC trigger+access-fault priority result");
    }
    TEST_ASSERT("a load execute-PC trigger takes priority when the same direct M-mode load would otherwise raise access fault",
        excpt.triggered == true &&
        excpt.cause == CAUSE_BKP
    );
    ai_arch_restore_machine_status(saved_mstatus);
    ai_arch_clear_mcontrol6_0();

    TEST_SETUP_EXCEPT();
    ai_arch_execute_trigger_load_once(bad_paddr);
    TEST_ASSERT("after clearing the execute-PC trigger, the same direct M-mode load reports load access fault with the raw address in tval",
        excpt.triggered == true &&
        excpt.cause == CAUSE_LAF &&
        excpt.tval == bad_paddr
    );

    TEST_END();
}

bool ai_arch_load_execute_trigger_access_fault_repeated_breakpoint_then_fault_corner()
{
    TEST_START();

    uintptr_t trigger_pc = (uintptr_t)ai_arch_execute_trigger_load_once;
    uintptr_t bad_paddr = AI_ARCH_TRIGGER_BAD_PADDR;
    const uint64_t scratch0 = 0x13579bdf2468ace0ULL;
    const uint64_t scratch1 = 0x02468ace13579bdfULL;

    ai_arch_trigger_mem[0] = scratch0;
    ai_arch_trigger_mem[1] = scratch1;
    ai_arch_arm_mcontrol6_0(trigger_pc, AI_ARCH_MCONTROL6_EXECUTE);
    uint64_t saved_mstatus = ai_arch_enable_machine_breakpoint_delivery();

    TEST_SETUP_EXCEPT();
    ai_arch_execute_trigger_load_once(bad_paddr);
    TEST_ASSERT("with a load execute-PC trigger armed on a direct-M access-faulting PA, the first access reports breakpoint and leaves scratch state unchanged",
        excpt.triggered == true &&
        excpt.cause == CAUSE_BKP &&
        ai_arch_trigger_mem[0] == scratch0 &&
        ai_arch_trigger_mem[1] == scratch1
    );
    ai_arch_reenable_breakpoint_delivery();

    TEST_SETUP_EXCEPT();
    ai_arch_execute_trigger_load_once(bad_paddr);
    TEST_ASSERT("with the same load execute-PC trigger still armed on that direct-M access-faulting PA, the second access still reports breakpoint and still leaves scratch state unchanged",
        excpt.triggered == true &&
        excpt.cause == CAUSE_BKP &&
        ai_arch_trigger_mem[0] == scratch0 &&
        ai_arch_trigger_mem[1] == scratch1
    );
    ai_arch_restore_machine_status(saved_mstatus);
    ai_arch_clear_mcontrol6_0();

    TEST_SETUP_EXCEPT();
    ai_arch_execute_trigger_load_once(bad_paddr);
    TEST_ASSERT("after clearing the repeated load execute-PC trigger, the same direct-M access falls back to load access fault with the raw address and still leaves scratch state unchanged",
        excpt.triggered == true &&
        excpt.cause == CAUSE_LAF &&
        excpt.tval == bad_paddr &&
        ai_arch_trigger_mem[0] == scratch0 &&
        ai_arch_trigger_mem[1] == scratch1
    );

    TEST_END();
}

bool ai_arch_store_trigger_access_fault_priority()
{
    TEST_START();

    uintptr_t bad_paddr = AI_ARCH_TRIGGER_BAD_PADDR;

    ai_arch_arm_mcontrol6_0(bad_paddr, AI_ARCH_MCONTROL6_STORE);
    uint64_t saved_mstatus = ai_arch_enable_machine_breakpoint_delivery();

    TEST_SETUP_EXCEPT();
    sd(bad_paddr, 0x1122334455667788ULL);
    if (!(excpt.triggered == true && excpt.cause == CAUSE_BKP)) {
        ai_arch_log_unexpected_trigger_result("unexpected store trigger+access-fault priority result");
    }
    TEST_ASSERT("a store address trigger takes priority when the same direct M-mode store would otherwise raise access fault",
        excpt.triggered == true &&
        excpt.cause == CAUSE_BKP
    );
    ai_arch_restore_machine_status(saved_mstatus);
    ai_arch_clear_mcontrol6_0();

    TEST_SETUP_EXCEPT();
    sd(bad_paddr, 0x8877665544332211ULL);
    TEST_ASSERT("after clearing the trigger, the same direct M-mode store reports store access fault with the raw address in tval",
        excpt.triggered == true &&
        excpt.cause == CAUSE_SAF &&
        excpt.tval == bad_paddr
    );

    TEST_END();
}

bool ai_arch_store_trigger_access_fault_repeated_breakpoint_then_fault_corner()
{
    TEST_START();

    uintptr_t bad_paddr = AI_ARCH_TRIGGER_BAD_PADDR;
    const uint64_t scratch0 = 0x0102030405060708ULL;
    const uint64_t scratch1 = 0x8877665544332211ULL;

    ai_arch_trigger_mem[0] = scratch0;
    ai_arch_trigger_mem[1] = scratch1;
    ai_arch_arm_mcontrol6_0(bad_paddr, AI_ARCH_MCONTROL6_STORE);
    uint64_t saved_mstatus = ai_arch_enable_machine_breakpoint_delivery();

    TEST_SETUP_EXCEPT();
    sd(bad_paddr, 0x1122334455667788ULL);
    TEST_ASSERT("with a store address trigger armed on a direct-M access-faulting PA, the first access reports breakpoint and leaves scratch state unchanged",
        excpt.triggered == true &&
        excpt.cause == CAUSE_BKP &&
        ai_arch_trigger_mem[0] == scratch0 &&
        ai_arch_trigger_mem[1] == scratch1
    );
    ai_arch_reenable_breakpoint_delivery();

    TEST_SETUP_EXCEPT();
    sd(bad_paddr, 0x8877665544332211ULL);
    TEST_ASSERT("with the same store address trigger still armed on that direct-M access-faulting PA, the second access still reports breakpoint and still leaves scratch state unchanged",
        excpt.triggered == true &&
        excpt.cause == CAUSE_BKP &&
        ai_arch_trigger_mem[0] == scratch0 &&
        ai_arch_trigger_mem[1] == scratch1
    );
    ai_arch_restore_machine_status(saved_mstatus);
    ai_arch_clear_mcontrol6_0();

    TEST_SETUP_EXCEPT();
    sd(bad_paddr, 0x5566778899aabbccULL);
    TEST_ASSERT("after clearing the repeated store address trigger, the same direct-M access falls back to store access fault with the raw address and still leaves scratch state unchanged",
        excpt.triggered == true &&
        excpt.cause == CAUSE_SAF &&
        excpt.tval == bad_paddr &&
        ai_arch_trigger_mem[0] == scratch0 &&
        ai_arch_trigger_mem[1] == scratch1
    );

    TEST_END();
}

bool ai_arch_store_execute_trigger_access_fault_priority()
{
    TEST_START();

    uintptr_t trigger_pc = (uintptr_t)ai_arch_execute_trigger_store_once;
    uintptr_t bad_paddr = AI_ARCH_TRIGGER_BAD_PADDR;

    ai_arch_arm_mcontrol6_0(trigger_pc, AI_ARCH_MCONTROL6_EXECUTE);
    uint64_t saved_mstatus = ai_arch_enable_machine_breakpoint_delivery();

    TEST_SETUP_EXCEPT();
    ai_arch_execute_trigger_store_once(bad_paddr, 0x1122334455667788ULL);
    if (!(excpt.triggered == true && excpt.cause == CAUSE_BKP)) {
        ai_arch_log_unexpected_trigger_result("unexpected store execute-PC trigger+access-fault priority result");
    }
    TEST_ASSERT("a store execute-PC trigger takes priority when the same direct M-mode store would otherwise raise access fault",
        excpt.triggered == true &&
        excpt.cause == CAUSE_BKP
    );
    ai_arch_restore_machine_status(saved_mstatus);
    ai_arch_clear_mcontrol6_0();

    TEST_SETUP_EXCEPT();
    ai_arch_execute_trigger_store_once(bad_paddr, 0x8877665544332211ULL);
    TEST_ASSERT("after clearing the execute-PC trigger, the same direct M-mode store reports store access fault with the raw address in tval",
        excpt.triggered == true &&
        excpt.cause == CAUSE_SAF &&
        excpt.tval == bad_paddr
    );

    TEST_END();
}

bool ai_arch_store_execute_trigger_access_fault_repeated_breakpoint_then_fault_corner()
{
    TEST_START();

    uintptr_t trigger_pc = (uintptr_t)ai_arch_execute_trigger_store_once;
    uintptr_t bad_paddr = AI_ARCH_TRIGGER_BAD_PADDR;
    const uint64_t scratch0 = 0x0f1e2d3c4b5a6978ULL;
    const uint64_t scratch1 = 0x8877665544332211ULL;

    ai_arch_trigger_mem[0] = scratch0;
    ai_arch_trigger_mem[1] = scratch1;
    ai_arch_arm_mcontrol6_0(trigger_pc, AI_ARCH_MCONTROL6_EXECUTE);
    uint64_t saved_mstatus = ai_arch_enable_machine_breakpoint_delivery();

    TEST_SETUP_EXCEPT();
    ai_arch_execute_trigger_store_once(bad_paddr, 0x1122334455667788ULL);
    TEST_ASSERT("with a store execute-PC trigger armed on a direct-M access-faulting PA, the first access reports breakpoint and leaves scratch state unchanged",
        excpt.triggered == true &&
        excpt.cause == CAUSE_BKP &&
        ai_arch_trigger_mem[0] == scratch0 &&
        ai_arch_trigger_mem[1] == scratch1
    );
    ai_arch_reenable_breakpoint_delivery();

    TEST_SETUP_EXCEPT();
    ai_arch_execute_trigger_store_once(bad_paddr, 0x8877665544332211ULL);
    TEST_ASSERT("with the same store execute-PC trigger still armed on that direct-M access-faulting PA, the second access still reports breakpoint and still leaves scratch state unchanged",
        excpt.triggered == true &&
        excpt.cause == CAUSE_BKP &&
        ai_arch_trigger_mem[0] == scratch0 &&
        ai_arch_trigger_mem[1] == scratch1
    );
    ai_arch_restore_machine_status(saved_mstatus);
    ai_arch_clear_mcontrol6_0();

    TEST_SETUP_EXCEPT();
    ai_arch_execute_trigger_store_once(bad_paddr, 0x5566778899aabbccULL);
    TEST_ASSERT("after clearing the repeated store execute-PC trigger, the same direct-M access falls back to store access fault with the raw address and still leaves scratch state unchanged",
        excpt.triggered == true &&
        excpt.cause == CAUSE_SAF &&
        excpt.tval == bad_paddr &&
        ai_arch_trigger_mem[0] == scratch0 &&
        ai_arch_trigger_mem[1] == scratch1
    );

    TEST_END();
}

bool ai_arch_amo_trigger_page_fault_spike_corner()
{
    TEST_START();

    uintptr_t pf_vaddr = hs_page_base(VSI_GI);

    ai_arch_prepare_hs_fault_page();
    ai_arch_arm_mcontrol6_0(pf_vaddr, AI_ARCH_MCONTROL6_AMO);
    uint64_t saved_mstatus = ai_arch_enable_machine_breakpoint_delivery();

    goto_priv(PRIV_HS);
    TEST_SETUP_EXCEPT();
    amoswap_w(pf_vaddr, 0xa1b2c3d4U);
    if (!(excpt.triggered == true &&
        excpt.cause == CAUSE_BKP &&
        excpt.tval == pf_vaddr)) {
        ai_arch_log_unexpected_trigger_result("unexpected AMO address-trigger+page-fault Spike corner result");
    }
    TEST_ASSERT("an AMO address trigger reports breakpoint before the HS store page fault",
        excpt.triggered == true &&
        excpt.cause == CAUSE_BKP &&
        excpt.tval == pf_vaddr
    );
    ai_arch_restore_machine_status(saved_mstatus);
    ai_arch_clear_mcontrol6_0();

    ai_arch_prepare_hs_fault_page();
    goto_priv(PRIV_HS);
    TEST_SETUP_EXCEPT();
    amoswap_w(pf_vaddr, 0x55667788U);
    TEST_ASSERT("after clearing the AMO address trigger, the same HS AMO reports store page fault with the original faulting VA",
        excpt.triggered == true &&
        excpt.cause == CAUSE_SPF &&
        excpt.tval == pf_vaddr
    );

    TEST_END();
}

bool ai_arch_amo_trigger_page_fault_repeated_breakpoints_then_valid_access_recovers_spike_corner()
{
    TEST_START();

    const uint64_t initial_target = 0x1122334499aabbccULL;
    const uint64_t adjacent_seed = 0x5566778899aabbccULL;
    const uint32_t recovered_word = 0x7fedcba9U;
    const uint64_t expected_old_value = 0xffffffff99aabbccULL;
    const uint64_t expected_target_after_recovery = 0x112233447fedcba9ULL;

    uintptr_t pf_vaddr = hs_page_base(VSI_GI);
    uintptr_t paddr = phys_page_base(VSRWX_GURWX);
    uintptr_t valid_vaddr = hs_page_base(VSRWX_GURWX);

    goto_priv(PRIV_M);
    write64(paddr, initial_target);
    write64(paddr + sizeof(uint64_t), adjacent_seed);

    ai_arch_prepare_hs_fault_page();
    ai_arch_arm_mcontrol6_0(pf_vaddr, AI_ARCH_MCONTROL6_AMO);
    uint64_t saved_mstatus = ai_arch_enable_machine_breakpoint_delivery();

    goto_priv(PRIV_HS);
    TEST_SETUP_EXCEPT();
    amoswap_w(pf_vaddr, 0xa1b2c3d4U);
    if (!(excpt.triggered == true &&
        excpt.cause == CAUSE_BKP &&
        excpt.tval == pf_vaddr)) {
        ai_arch_log_unexpected_trigger_result("unexpected repeated AMO address-trigger+page-fault first-hit Spike corner result");
    }
    TEST_ASSERT("with an AMO address trigger armed, the first HS AMO to a page-faulting VA reports breakpoint with the raw VA in tval",
        excpt.triggered == true &&
        excpt.cause == CAUSE_BKP &&
        excpt.tval == pf_vaddr
    );

    ai_arch_prepare_hs_fault_page();
    ai_arch_reenable_breakpoint_delivery();
    goto_priv(PRIV_HS);
    TEST_SETUP_EXCEPT();
    amoswap_w(pf_vaddr, 0x55667788U);
    if (!(excpt.triggered == true &&
        excpt.cause == CAUSE_BKP &&
        excpt.tval == pf_vaddr)) {
        ai_arch_log_unexpected_trigger_result("unexpected repeated AMO address-trigger+page-fault second-hit Spike corner result");
    }
    TEST_ASSERT("with the same AMO address trigger still armed, a second HS AMO to the page-faulting VA again reports breakpoint with the same raw VA",
        excpt.triggered == true &&
        excpt.cause == CAUSE_BKP &&
        excpt.tval == pf_vaddr
    );
    ai_arch_restore_machine_status(saved_mstatus);
    ai_arch_clear_mcontrol6_0();

    goto_priv(PRIV_M);
    TEST_ASSERT("repeated AMO address-trigger breakpoints on the page-fault path do not modify the later valid target doubleword or its adjacent doubleword",
        read64(paddr) == initial_target &&
        read64(paddr + sizeof(uint64_t)) == adjacent_seed
    );

    goto_priv(PRIV_HS);
    hspt_init();
    sfence_vma();

    TEST_SETUP_EXCEPT();
    uint64_t old_value = ai_arch_trigger_amoswap_w_raw64(valid_vaddr, recovered_word);
    TEST_ASSERT("after repeated AMO address-trigger breakpoints on the page-fault path, a subsequent valid HS amoswap.w on a mapped page still succeeds and returns the sign-extended old word",
        excpt.triggered == false &&
        old_value == expected_old_value
    );

    goto_priv(PRIV_M);
    TEST_ASSERT("the valid recovery AMO after repeated AMO address-trigger breakpoints on the page-fault path updates only the target lower lane while preserving the adjacent doubleword",
        read64(paddr) == expected_target_after_recovery &&
        read64(paddr + sizeof(uint64_t)) == adjacent_seed
    );

    TEST_END();
}

bool ai_arch_amo_execute_trigger_page_fault_priority()
{
    TEST_START();

    uintptr_t trigger_pc = (uintptr_t)ai_arch_execute_trigger_amoswap_w_once;
    uintptr_t pf_vaddr = hs_page_base(VSI_GI);

    ai_arch_prepare_hs_fault_page();
    ai_arch_arm_mcontrol6_0(trigger_pc, AI_ARCH_MCONTROL6_EXECUTE);
    uint64_t saved_mstatus = ai_arch_enable_machine_breakpoint_delivery();

    goto_priv(PRIV_HS);
    TEST_SETUP_EXCEPT();
    ai_arch_execute_trigger_amoswap_w_once(pf_vaddr, 0xa1b2c3d4U);
    if (!(excpt.triggered == true && excpt.cause == CAUSE_BKP)) {
        ai_arch_log_unexpected_trigger_result("unexpected AMO execute-PC trigger+page-fault priority result");
    }
    TEST_ASSERT("an AMO execute-PC trigger takes priority when the same HS AMO would otherwise raise store page fault",
        excpt.triggered == true &&
        excpt.cause == CAUSE_BKP
    );
    ai_arch_restore_machine_status(saved_mstatus);
    ai_arch_clear_mcontrol6_0();

    ai_arch_prepare_hs_fault_page();
    goto_priv(PRIV_HS);
    TEST_SETUP_EXCEPT();
    ai_arch_execute_trigger_amoswap_w_once(pf_vaddr, 0x55667788U);
    TEST_ASSERT("after clearing the AMO execute-PC trigger, the same HS AMO reports store page fault with the original faulting VA",
        excpt.triggered == true &&
        excpt.cause == CAUSE_SPF &&
        excpt.tval == pf_vaddr
    );

    TEST_END();
}

bool ai_arch_load_trigger_page_fault_priority()
{
    TEST_START();

    uintptr_t pf_vaddr = hs_page_base(VSI_GI);

    ai_arch_prepare_hs_fault_page();
    ai_arch_arm_mcontrol6_0(pf_vaddr, AI_ARCH_MCONTROL6_LOAD);
    uint64_t saved_mstatus = ai_arch_enable_machine_breakpoint_delivery();

    goto_priv(PRIV_HS);
    TEST_SETUP_EXCEPT();
    lb(pf_vaddr);
    if (!(excpt.triggered == true && excpt.cause == CAUSE_BKP)) {
        ai_arch_log_unexpected_trigger_result("unexpected load trigger+page-fault priority result");
    }
    TEST_ASSERT("a load address trigger takes priority when the same HS load would otherwise raise page fault",
        excpt.triggered == true &&
        excpt.cause == CAUSE_BKP
    );
    ai_arch_restore_machine_status(saved_mstatus);
    ai_arch_clear_mcontrol6_0();

    ai_arch_prepare_hs_fault_page();
    goto_priv(PRIV_HS);
    TEST_SETUP_EXCEPT();
    lb(pf_vaddr);
    TEST_ASSERT("after clearing the trigger, the same HS load reports load page fault with the original faulting VA",
        excpt.triggered == true &&
        excpt.cause == CAUSE_LPF &&
        excpt.tval == pf_vaddr
    );

    TEST_END();
}

bool ai_arch_load_execute_trigger_page_fault_priority()
{
    TEST_START();

    uintptr_t trigger_pc = (uintptr_t)ai_arch_execute_trigger_load_once;
    uintptr_t pf_vaddr = hs_page_base(VSI_GI);

    ai_arch_prepare_hs_fault_page();
    ai_arch_arm_mcontrol6_0(trigger_pc, AI_ARCH_MCONTROL6_EXECUTE);
    uint64_t saved_mstatus = ai_arch_enable_machine_breakpoint_delivery();

    goto_priv(PRIV_HS);
    TEST_SETUP_EXCEPT();
    ai_arch_execute_trigger_load_once(pf_vaddr);
    if (!(excpt.triggered == true && excpt.cause == CAUSE_BKP)) {
        ai_arch_log_unexpected_trigger_result("unexpected load execute-PC trigger+page-fault priority result");
    }
    TEST_ASSERT("a load execute-PC trigger takes priority when the same HS load would otherwise raise page fault",
        excpt.triggered == true &&
        excpt.cause == CAUSE_BKP
    );
    ai_arch_restore_machine_status(saved_mstatus);
    ai_arch_clear_mcontrol6_0();

    ai_arch_prepare_hs_fault_page();
    goto_priv(PRIV_HS);
    TEST_SETUP_EXCEPT();
    ai_arch_execute_trigger_load_once(pf_vaddr);
    TEST_ASSERT("after clearing the execute-PC trigger, the same HS load reports load page fault with the original faulting VA",
        excpt.triggered == true &&
        excpt.cause == CAUSE_LPF &&
        excpt.tval == pf_vaddr
    );

    TEST_END();
}

bool ai_arch_load_execute_trigger_page_fault_repeated_breakpoint_then_fault_corner()
{
    TEST_START();

    uintptr_t trigger_pc = (uintptr_t)ai_arch_execute_trigger_load_once;
    uintptr_t pf_vaddr = hs_page_base(VSI_GI);

    ai_arch_prepare_hs_fault_page();
    ai_arch_arm_mcontrol6_0(trigger_pc, AI_ARCH_MCONTROL6_EXECUTE);
    uint64_t saved_mstatus = ai_arch_enable_machine_breakpoint_delivery();

    goto_priv(PRIV_HS);
    TEST_SETUP_EXCEPT();
    ai_arch_execute_trigger_load_once(pf_vaddr);
    TEST_ASSERT("with a load execute-PC trigger armed on an HS page-faulting VA, the first access reports breakpoint before the page fault is observed",
        excpt.triggered == true &&
        excpt.cause == CAUSE_BKP
    );

    ai_arch_prepare_hs_fault_page();
    goto_priv(PRIV_HS);
    TEST_SETUP_EXCEPT();
    ai_arch_execute_trigger_load_once(pf_vaddr);
    TEST_ASSERT("with the same load execute-PC trigger still armed on that HS page-faulting VA, the second access still reports breakpoint before the page fault is observed",
        excpt.triggered == true &&
        excpt.cause == CAUSE_BKP
    );
    ai_arch_restore_machine_status(saved_mstatus);
    ai_arch_clear_mcontrol6_0();

    ai_arch_prepare_hs_fault_page();
    goto_priv(PRIV_HS);
    TEST_SETUP_EXCEPT();
    ai_arch_execute_trigger_load_once(pf_vaddr);
    TEST_ASSERT("after clearing the repeated load execute-PC trigger, the same HS access falls back to load page fault with the original faulting VA",
        excpt.triggered == true &&
        excpt.cause == CAUSE_LPF &&
        excpt.tval == pf_vaddr
    );

    TEST_END();
}

bool ai_arch_load_execute_trigger_high_half_page_fault_repeated_breakpoint_then_other_aligned_load_recovers_corner()
{
    TEST_START();

    enum test_page low_page = VSR_GX;
    enum test_page high_page = VSW_GI;
    uintptr_t low_phys = ai_arch_trigger_page_end_phys(low_page);
    uintptr_t high_phys = phys_page_base(high_page);
    uintptr_t trigger_pc = (uintptr_t)ai_arch_execute_trigger_load_once;
    uintptr_t vaddr = ai_arch_trigger_prepare_cross_page_high_half_load_pf_alias();
    uintptr_t second_half_vaddr = vaddr + 4U;
    uintptr_t recovery_vaddr = hs_page_base(low_page) + 0x140U;
    uintptr_t recovery_phys = phys_page_base(low_page) + 0x140U;
    const uint64_t low_pattern = 0x65768798a9bacbdcULL;
    const uint64_t high_pattern = 0xedfe1032547698baULL;
    const uint64_t recovery_expected = 0x1234fedc5678ba90ULL;
    bool first_preserved;
    bool second_preserved;

    goto_priv(PRIV_M);
    write64(low_phys, low_pattern);
    write64(high_phys, high_pattern);
    write64(recovery_phys, recovery_expected);

    ai_arch_arm_mcontrol6_0(trigger_pc, AI_ARCH_MCONTROL6_EXECUTE);
    uint64_t saved_mstatus = ai_arch_enable_machine_breakpoint_delivery();

    goto_priv(PRIV_HS);
    TEST_SETUP_EXCEPT();
    ai_arch_execute_trigger_load_once(vaddr);
    ai_arch_trigger_excpt_snapshot_t first_excpt = ai_arch_snapshot_excpt();
    goto_priv(PRIV_M);
    first_preserved = (read64(low_phys) == low_pattern && read64(high_phys) == high_pattern);
    goto_priv(PRIV_HS);
    TEST_ASSERT("with a load execute-PC trigger armed on a cross-page access whose low half is cacheable but whose high half would page-fault, the first access reports breakpoint and leaves both boundary doublewords unchanged before the unrelated aligned-load recovery probe",
        first_excpt.triggered == true &&
        first_excpt.cause == CAUSE_BKP &&
        first_preserved
    );

    TEST_SETUP_EXCEPT();
    ai_arch_execute_trigger_load_once(vaddr);
    ai_arch_trigger_excpt_snapshot_t second_excpt = ai_arch_snapshot_excpt();
    goto_priv(PRIV_M);
    second_preserved = (read64(low_phys) == low_pattern && read64(high_phys) == high_pattern);
    goto_priv(PRIV_HS);
    TEST_ASSERT("with the same load execute-PC trigger still armed on that cross-page high-half page-fault path, the second access still reports breakpoint and still leaves both boundary doublewords unchanged before the unrelated aligned-load recovery probe",
        second_excpt.triggered == true &&
        second_excpt.cause == CAUSE_BKP &&
        second_preserved
    );
    ai_arch_restore_machine_status(saved_mstatus);
    ai_arch_clear_mcontrol6_0();

    goto_priv(PRIV_HS);
    hspt_init();
    sfence_vma();

    TEST_SETUP_EXCEPT();
    ai_arch_execute_trigger_load_once(vaddr);
    TEST_ASSERT("after clearing the repeated load execute-PC trigger, the same split access falls back to the true second-half load page fault with the second-half start VA in tval before the unrelated aligned-load recovery probe",
        excpt.triggered == true &&
        excpt.cause == CAUSE_LPF &&
        excpt.tval == second_half_vaddr
    );

    goto_priv(PRIV_M);
    TEST_ASSERT("before the unrelated aligned-load recovery probe, repeated execute-trigger hits and the fallback second-half load page fault still leave both boundary backing doublewords unchanged",
        read64(low_phys) == low_pattern &&
        read64(high_phys) == high_pattern &&
        read64(recovery_phys) == recovery_expected
    );

    goto_priv(PRIV_HS);
    TEST_SETUP_EXCEPT();
    uint64_t recovery_value = ai_arch_execute_trigger_load_once(recovery_vaddr);
    TEST_ASSERT("after repeated load execute-PC breakpoint hits and the fallback second-half load page fault, another aligned load in the same low page still recovers normally without repairing the faulting high half",
        excpt.triggered == false &&
        recovery_value == recovery_expected
    );

    TEST_SETUP_EXCEPT();
    ai_arch_execute_trigger_load_once(vaddr);
    TEST_ASSERT("after the unrelated aligned-load recovery probe, the original split-load shape still reports the same second-half load page fault until the high-half leaf is repaired",
        excpt.triggered == true &&
        excpt.cause == CAUSE_LPF &&
        excpt.tval == second_half_vaddr
    );

    goto_priv(PRIV_M);
    TEST_ASSERT("the repeated load execute-trigger plus high-half-page-fault corner leaves the boundary backing doublewords untouched while preserving the unrelated aligned-load recovery payload",
        read64(low_phys) == low_pattern &&
        read64(high_phys) == high_pattern &&
        read64(recovery_phys) == recovery_expected
    );

    goto_priv(PRIV_HS);
    hspt_init();
    sfence_vma();

    TEST_END();
}

bool ai_arch_load_execute_trigger_high_half_page_fault_repeated_breakpoint_then_other_aligned_load_adjacent_preserved_corner()
{
    TEST_START();

    enum test_page low_page = VSR_GX;
    enum test_page high_page = VSW_GI;
    uintptr_t low_adjacent_phys = phys_page_base(low_page) + PAGE_SIZE - (2 * sizeof(uint64_t));
    uintptr_t low_phys = ai_arch_trigger_page_end_phys(low_page);
    uintptr_t high_phys = phys_page_base(high_page);
    uintptr_t high_adjacent_phys = high_phys + sizeof(uint64_t);
    uintptr_t trigger_pc = (uintptr_t)ai_arch_execute_trigger_load_once;
    uintptr_t vaddr = ai_arch_trigger_prepare_cross_page_high_half_load_pf_alias();
    uintptr_t second_half_vaddr = vaddr + 4U;
    uintptr_t recovery_vaddr = hs_page_base(low_page) + 0x140U;
    uintptr_t recovery_adjacent_vaddr = recovery_vaddr + sizeof(uint64_t);
    uintptr_t recovery_phys = phys_page_base(low_page) + 0x140U;
    uintptr_t recovery_adjacent_phys = recovery_phys + sizeof(uint64_t);
    const uint64_t low_adjacent = 0x44aa55aa11cc33ccULL;
    const uint64_t low_pattern = 0x65768798a9bacbdcULL;
    const uint64_t high_pattern = 0xedfe1032547698baULL;
    const uint64_t high_adjacent = 0x33cc11cc55aa77aaULL;
    const uint64_t recovery_expected = 0x1234fedc5678ba90ULL;
    const uint64_t recovery_adjacent = 0x0f1e2d3c4b5a6978ULL;
    bool first_preserved;
    bool second_preserved;

    goto_priv(PRIV_M);
    write64(low_adjacent_phys, low_adjacent);
    write64(low_phys, low_pattern);
    write64(high_phys, high_pattern);
    write64(high_adjacent_phys, high_adjacent);
    write64(recovery_phys, recovery_expected);
    write64(recovery_adjacent_phys, recovery_adjacent);

    ai_arch_arm_mcontrol6_0(trigger_pc, AI_ARCH_MCONTROL6_EXECUTE);
    uint64_t saved_mstatus = ai_arch_enable_machine_breakpoint_delivery();

    goto_priv(PRIV_HS);
    TEST_SETUP_EXCEPT();
    ai_arch_execute_trigger_load_once(vaddr);
    ai_arch_trigger_excpt_snapshot_t first_excpt = ai_arch_snapshot_excpt();
    goto_priv(PRIV_M);
    first_preserved =
        (read64(low_adjacent_phys) == low_adjacent &&
        read64(low_phys) == low_pattern &&
        read64(high_phys) == high_pattern &&
        read64(high_adjacent_phys) == high_adjacent);
    goto_priv(PRIV_HS);
    TEST_ASSERT("with a load execute-PC trigger armed on a cross-page access whose low half is cacheable but whose high half would page-fault, the first access reports breakpoint and leaves both boundary and adjacent doublewords unchanged before the adjacent-preservation recovery probe",
        first_excpt.triggered == true &&
        first_excpt.cause == CAUSE_BKP &&
        first_preserved
    );

    TEST_SETUP_EXCEPT();
    ai_arch_execute_trigger_load_once(vaddr);
    ai_arch_trigger_excpt_snapshot_t second_excpt = ai_arch_snapshot_excpt();
    goto_priv(PRIV_M);
    second_preserved =
        (read64(low_adjacent_phys) == low_adjacent &&
        read64(low_phys) == low_pattern &&
        read64(high_phys) == high_pattern &&
        read64(high_adjacent_phys) == high_adjacent);
    goto_priv(PRIV_HS);
    TEST_ASSERT("with the same load execute-PC trigger still armed on that cross-page high-half page-fault path, the second access still reports breakpoint and still leaves both boundary and adjacent doublewords unchanged before the adjacent-preservation recovery probe",
        second_excpt.triggered == true &&
        second_excpt.cause == CAUSE_BKP &&
        second_preserved
    );
    ai_arch_restore_machine_status(saved_mstatus);
    ai_arch_clear_mcontrol6_0();

    goto_priv(PRIV_HS);
    hspt_init();
    sfence_vma();

    TEST_SETUP_EXCEPT();
    ai_arch_execute_trigger_load_once(vaddr);
    TEST_ASSERT("after clearing the repeated load execute-PC trigger, the same split access falls back to the true second-half load page fault with the second-half start VA in tval before the adjacent-preservation recovery probe",
        excpt.triggered == true &&
        excpt.cause == CAUSE_LPF &&
        excpt.tval == second_half_vaddr
    );

    goto_priv(PRIV_M);
    TEST_ASSERT("before the adjacent-preservation recovery probe, repeated execute-trigger hits and the fallback second-half load page fault still leave both boundary doublewords, their adjacent neighbors, and the aligned recovery region untouched",
        read64(low_adjacent_phys) == low_adjacent &&
        read64(low_phys) == low_pattern &&
        read64(high_phys) == high_pattern &&
        read64(high_adjacent_phys) == high_adjacent &&
        read64(recovery_phys) == recovery_expected &&
        read64(recovery_adjacent_phys) == recovery_adjacent
    );

    goto_priv(PRIV_HS);
    TEST_SETUP_EXCEPT();
    uint64_t recovery_value = ai_arch_execute_trigger_load_once(recovery_vaddr);
    TEST_ASSERT("after repeated load execute-PC breakpoint hits and the fallback second-half load page fault, another aligned load in the same low page still recovers normally without repairing the faulting high half in the adjacent-preservation corner",
        excpt.triggered == false &&
        recovery_value == recovery_expected
    );

    TEST_SETUP_EXCEPT();
    uint64_t observed_adjacent = ai_arch_execute_trigger_load_once(recovery_adjacent_vaddr);
    TEST_ASSERT("the unrelated aligned-load recovery probe does not disturb the adjacent low-page word, which still reads back its original payload in the adjacent-preservation corner",
        excpt.triggered == false &&
        observed_adjacent == recovery_adjacent
    );

    TEST_SETUP_EXCEPT();
    ai_arch_execute_trigger_load_once(vaddr);
    TEST_ASSERT("after the adjacent-preservation recovery probe, the original split-load shape still reports the same second-half load page fault until the high-half leaf is repaired",
        excpt.triggered == true &&
        excpt.cause == CAUSE_LPF &&
        excpt.tval == second_half_vaddr
    );

    goto_priv(PRIV_M);
    TEST_ASSERT("the repeated load execute-trigger plus high-half-page-fault corner leaves both boundary doublewords, their adjacent neighbors, and the aligned recovery region untouched while preserving the unrelated aligned-load payload",
        read64(low_adjacent_phys) == low_adjacent &&
        read64(low_phys) == low_pattern &&
        read64(high_phys) == high_pattern &&
        read64(high_adjacent_phys) == high_adjacent &&
        read64(recovery_phys) == recovery_expected &&
        read64(recovery_adjacent_phys) == recovery_adjacent
    );

    goto_priv(PRIV_HS);
    hspt_init();
    sfence_vma();

    TEST_END();
}
