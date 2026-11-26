#include "ai_arch_trigger_breakpoint_common.h"

bool ai_arch_prefetch_i_execute_trigger_repeated_breakpoint_then_noncanonical_fetch_page_fault_adjacent_preserved_corner()
{
    TEST_START();

    uintptr_t exec_vaddr = hs_page_base(VSRWX_GRWX);
    uintptr_t bad_vaddr = ai_arch_trigger_make_noncanonical_vaddr(exec_vaddr);
    uintptr_t exec_paddr = phys_page_base(VSRWX_GRWX);
    uintptr_t adjacent_paddr = exec_paddr + sizeof(uint32_t);
    uintptr_t trigger_pc = (uintptr_t)ai_arch_execute_trigger_prefetch_i_once;
    const uint32_t instruction = 0x00008067U;
    const uint32_t adjacent = 0x00000013U;

    goto_priv(PRIV_M);
    write32(exec_paddr, instruction);
    write32(adjacent_paddr, adjacent);

    goto_priv(PRIV_HS);
    hspt_init();
    sfence_vma();

    ai_arch_arm_mcontrol6_0(trigger_pc, AI_ARCH_MCONTROL6_EXECUTE);
    uint64_t saved_mstatus = ai_arch_enable_machine_breakpoint_delivery();

    TEST_SETUP_EXCEPT();
    ai_arch_execute_trigger_prefetch_i_once(bad_vaddr);
    TEST_ASSERT("the first prefetch.i execute-PC trigger hit with a non-canonical target still reports breakpoint before any later fetch fault is exposed in the adjacent-preservation corner",
        excpt.triggered == true &&
        excpt.cause == CAUSE_BKP
    );
    ai_arch_reenable_breakpoint_delivery();

    TEST_SETUP_EXCEPT();
    ai_arch_execute_trigger_prefetch_i_once(bad_vaddr);
    TEST_ASSERT("the second prefetch.i execute-PC trigger hit with the same non-canonical target still reports breakpoint before any later fetch fault is exposed in the adjacent-preservation corner",
        excpt.triggered == true &&
        excpt.cause == CAUSE_BKP
    );
    ai_arch_reenable_breakpoint_delivery();
    ai_arch_restore_machine_status(saved_mstatus);
    ai_arch_clear_mcontrol6_0();

    goto_priv(PRIV_HS);
    TEST_SETUP_EXCEPT();
    ai_arch_execute_trigger_prefetch_i_once(bad_vaddr);
    TEST_ASSERT("after clearing the repeated execute-PC trigger, the same prefetch.i to the non-canonical target falls back to the normal silent-hint behavior in the adjacent-preservation corner",
        excpt.triggered == false
    );

    TEST_SETUP_EXCEPT();
    TEST_EXEC_EXCEPT(bad_vaddr);
    TEST_ASSERT("after repeated triggered prefetch.i probes, a real fetch to the same non-canonical HS target still reports instruction page fault with the raw VA in the adjacent-preservation corner",
        excpt.triggered == true &&
        excpt.cause == CAUSE_IPF &&
        excpt.tval == bad_vaddr
    );

    goto_priv(PRIV_M);
    TEST_ASSERT("repeated prefetch.i execute-trigger hits on the non-canonical target still leave both the target and adjacent backing instruction words unchanged in the adjacent-preservation corner",
        read32(exec_paddr) == instruction &&
        read32(adjacent_paddr) == adjacent
    );

    TEST_END();
}

bool ai_arch_prefetch_i_execute_trigger_repeated_breakpoint_then_noncanonical_valid_fetch_recovers_adjacent_preserved_corner()
{
    TEST_START();

    uintptr_t exec_vaddr = hs_page_base(VSRWX_GRWX);
    uintptr_t bad_vaddr = ai_arch_trigger_make_noncanonical_vaddr(exec_vaddr);
    uintptr_t exec_paddr = phys_page_base(VSRWX_GRWX);
    uintptr_t adjacent_paddr = exec_paddr + sizeof(uint32_t);
    uintptr_t trigger_pc = (uintptr_t)ai_arch_execute_trigger_prefetch_i_once;
    const uint32_t instruction = 0x00008067U;
    const uint32_t adjacent = 0x00000013U;

    goto_priv(PRIV_M);
    write32(exec_paddr, instruction);
    write32(adjacent_paddr, adjacent);

    goto_priv(PRIV_HS);
    hspt_init();
    sfence_vma();

    ai_arch_arm_mcontrol6_0(trigger_pc, AI_ARCH_MCONTROL6_EXECUTE);
    uint64_t saved_mstatus = ai_arch_enable_machine_breakpoint_delivery();

    TEST_SETUP_EXCEPT();
    ai_arch_execute_trigger_prefetch_i_once(bad_vaddr);
    TEST_ASSERT("the first prefetch.i execute-PC trigger hit with a non-canonical target reports breakpoint before any later canonical recovery in the adjacent-preservation corner",
        excpt.triggered == true &&
        excpt.cause == CAUSE_BKP
    );
    ai_arch_reenable_breakpoint_delivery();

    TEST_SETUP_EXCEPT();
    ai_arch_execute_trigger_prefetch_i_once(bad_vaddr);
    TEST_ASSERT("the second prefetch.i execute-PC trigger hit with the same non-canonical target still reports breakpoint before any later canonical recovery in the adjacent-preservation corner",
        excpt.triggered == true &&
        excpt.cause == CAUSE_BKP
    );
    ai_arch_reenable_breakpoint_delivery();
    ai_arch_restore_machine_status(saved_mstatus);
    ai_arch_clear_mcontrol6_0();

    goto_priv(PRIV_HS);
    TEST_SETUP_EXCEPT();
    ai_arch_execute_trigger_prefetch_i_once(bad_vaddr);
    TEST_ASSERT("after clearing the repeated execute-PC trigger, the same prefetch.i to the non-canonical target returns to the normal silent-hint behavior before canonical recovery in the adjacent-preservation corner",
        excpt.triggered == false
    );

    goto_priv(PRIV_M);
    TEST_ASSERT("repeated prefetch.i execute-trigger hits still leave both the target and adjacent backing instruction words unchanged before canonical recovery in the adjacent-preservation corner",
        read32(exec_paddr) == instruction &&
        read32(adjacent_paddr) == adjacent
    );

    goto_priv(PRIV_HS);
    TEST_SETUP_EXCEPT();
    ((void(*)(void))exec_vaddr)();
    TEST_ASSERT("after repeated prefetch.i execute-PC trigger hits on a non-canonical target, a subsequent canonical executable HS address still runs successfully in the adjacent-preservation corner",
        excpt.triggered == false
    );

    goto_priv(PRIV_M);
    TEST_ASSERT("after the recovered non-canonical prefetch.i execute-trigger path, both the target and adjacent backing instruction words remain untouched",
        read32(exec_paddr) == instruction &&
        read32(adjacent_paddr) == adjacent
    );

    TEST_END();
}

bool ai_arch_prefetch_i_redirect_kill_no_late_ifetch_prefetch_emit_corner()
{
    TEST_START();

    uintptr_t exec_vaddr = hs_page_base(VSRWX_GRWX);
    uintptr_t bad_vaddr = ai_arch_trigger_make_noncanonical_vaddr(exec_vaddr);
    uintptr_t exec_paddr = phys_page_base(VSRWX_GRWX);
    uintptr_t adjacent_paddr = exec_paddr + sizeof(uint32_t);
    uintptr_t trigger_pc = (uintptr_t)ai_arch_execute_trigger_prefetch_i_once;
    const uint32_t instruction = 0x00008067U;
    const uint32_t adjacent = 0x00000013U;

    goto_priv(PRIV_M);
    write32(exec_paddr, instruction);
    write32(adjacent_paddr, adjacent);

    goto_priv(PRIV_HS);
    hspt_init();
    sfence_vma();

    ai_arch_arm_mcontrol6_0(trigger_pc, AI_ARCH_MCONTROL6_EXECUTE);
    uint64_t saved_mstatus = ai_arch_enable_machine_breakpoint_delivery();

    TEST_SETUP_EXCEPT();
    ai_arch_execute_trigger_prefetch_i_once(bad_vaddr);
    TEST_ASSERT("an execute-trigger kill on prefetch.i reports breakpoint before the hint can retire in the redirect-kill approximation",
        excpt.triggered == true &&
        excpt.cause == CAUSE_BKP
    );
    ai_arch_restore_machine_status(saved_mstatus);
    ai_arch_clear_mcontrol6_0();

    goto_priv(PRIV_HS);
    TEST_SETUP_EXCEPT();
    TEST_EXEC_EXCEPT(bad_vaddr);
    TEST_ASSERT("after the killed prefetch.i window, a real fetch to the same non-canonical target still reports instruction page fault with the raw VA",
        excpt.triggered == true &&
        excpt.cause == CAUSE_IPF &&
        excpt.tval == bad_vaddr
    );

    TEST_SETUP_EXCEPT();
    ((void(*)(void))exec_vaddr)();
    TEST_ASSERT("the killed prefetch.i window does not poison a later real fetch to the canonical executable target",
        excpt.triggered == false
    );

    goto_priv(PRIV_M);
    TEST_ASSERT("the single killed prefetch.i approximation leaves both the canonical target and adjacent backing instruction words unchanged",
        read32(exec_paddr) == instruction &&
        read32(adjacent_paddr) == adjacent
    );

    TEST_END();
}

bool ai_arch_killed_prefetch_i_reused_load_slot_no_stale_frontend_prefetch_vaddr_corner()
{
    TEST_START();

    uintptr_t exec_vaddr = hs_page_base(VSRWX_GRWX);
    uintptr_t bad_vaddr = ai_arch_trigger_make_noncanonical_vaddr(exec_vaddr);
    uintptr_t exec_paddr = phys_page_base(VSRWX_GRWX);
    uintptr_t adjacent_paddr = exec_paddr + sizeof(uint32_t);
    uintptr_t trigger_pc = (uintptr_t)ai_arch_execute_trigger_prefetch_i_once;
    const uint32_t instruction = 0x00008067U;
    const uint32_t adjacent = 0x00000013U;

    goto_priv(PRIV_M);
    write32(exec_paddr, instruction);
    write32(adjacent_paddr, adjacent);

    goto_priv(PRIV_HS);
    hspt_init();
    sfence_vma();

    ai_arch_arm_mcontrol6_0(trigger_pc, AI_ARCH_MCONTROL6_EXECUTE);
    uint64_t saved_mstatus = ai_arch_enable_machine_breakpoint_delivery();

    TEST_SETUP_EXCEPT();
    ai_arch_execute_trigger_prefetch_i_once(bad_vaddr);
    TEST_ASSERT("the reused-load-slot approximation first kills prefetch.i through an execute trigger before any later valid fetch is attempted",
        excpt.triggered == true &&
        excpt.cause == CAUSE_BKP
    );
    ai_arch_restore_machine_status(saved_mstatus);
    ai_arch_clear_mcontrol6_0();

    goto_priv(PRIV_HS);
    TEST_SETUP_EXCEPT();
    ai_arch_execute_trigger_prefetch_i_once(exec_vaddr);
    TEST_ASSERT("after clearing the one-shot execute trigger, a later prefetch.i to the canonical executable target returns to normal silent-hint behavior",
        excpt.triggered == false
    );

    TEST_SETUP_EXCEPT();
    ((void(*)(void))exec_vaddr)();
    TEST_ASSERT("after the earlier killed bad-target prefetch.i, a later canonical fetch still executes successfully with no stale frontend target rebound",
        excpt.triggered == false
    );

    TEST_SETUP_EXCEPT();
    TEST_EXEC_EXCEPT(bad_vaddr);
    TEST_ASSERT("the earlier killed bad-target prefetch.i also does not consume or repair the bad target, so an explicit later fetch still faults on that original VA",
        excpt.triggered == true &&
        excpt.cause == CAUSE_IPF &&
        excpt.tval == bad_vaddr
    );

    goto_priv(PRIV_M);
    TEST_ASSERT("the killed-prefetch then canonical-reuse approximation leaves both the target and adjacent backing instruction words unchanged",
        read32(exec_paddr) == instruction &&
        read32(adjacent_paddr) == adjacent
    );

    TEST_END();
}

bool ai_arch_prefetch_i_execute_trigger_repeated_breakpoint_then_all_ones_fetch_page_fault_adjacent_preserved_corner()
{
    TEST_START();

    uintptr_t bad_vaddr = (uintptr_t)-1;
    uintptr_t fetch_target = bad_vaddr & ~1ULL;
    uintptr_t exec_paddr = phys_page_base(VSRWX_GRWX);
    uintptr_t adjacent_paddr = exec_paddr + sizeof(uint32_t);
    uintptr_t trigger_pc = (uintptr_t)ai_arch_execute_trigger_prefetch_i_once;
    const uint32_t instruction = 0x00008067U;
    const uint32_t adjacent = 0x00000013U;

    goto_priv(PRIV_M);
    write32(exec_paddr, instruction);
    write32(adjacent_paddr, adjacent);

    goto_priv(PRIV_HS);
    hspt_init();
    sfence_vma();

    ai_arch_arm_mcontrol6_0(trigger_pc, AI_ARCH_MCONTROL6_EXECUTE);
    uint64_t saved_mstatus = ai_arch_enable_machine_breakpoint_delivery();

    TEST_SETUP_EXCEPT();
    ai_arch_execute_trigger_prefetch_i_once(bad_vaddr);
    TEST_ASSERT("the first prefetch.i execute-PC trigger hit with an all-ones target still reports breakpoint before any later fetch fault is exposed in the adjacent-preservation corner",
        excpt.triggered == true &&
        excpt.cause == CAUSE_BKP
    );
    ai_arch_reenable_breakpoint_delivery();

    TEST_SETUP_EXCEPT();
    ai_arch_execute_trigger_prefetch_i_once(bad_vaddr);
    TEST_ASSERT("the second prefetch.i execute-PC trigger hit with the same all-ones target still reports breakpoint before any later fetch fault is exposed in the adjacent-preservation corner",
        excpt.triggered == true &&
        excpt.cause == CAUSE_BKP
    );
    ai_arch_reenable_breakpoint_delivery();
    ai_arch_restore_machine_status(saved_mstatus);
    ai_arch_clear_mcontrol6_0();

    goto_priv(PRIV_HS);
    TEST_SETUP_EXCEPT();
    ai_arch_execute_trigger_prefetch_i_once(bad_vaddr);
    TEST_ASSERT("after clearing the repeated execute-PC trigger, the same prefetch.i to the all-ones target falls back to the normal silent-hint behavior in the adjacent-preservation corner",
        excpt.triggered == false
    );

    TEST_SETUP_EXCEPT();
    TEST_EXEC_EXCEPT(bad_vaddr);
    TEST_ASSERT("after repeated triggered prefetch.i probes, a real fetch to the same all-ones target still reports instruction page fault with the JALR-cleared fetch target in the adjacent-preservation corner",
        excpt.triggered == true &&
        excpt.cause == CAUSE_IPF &&
        excpt.tval == fetch_target
    );

    goto_priv(PRIV_M);
    TEST_ASSERT("repeated prefetch.i execute-trigger hits on the all-ones target still leave both the target and adjacent backing instruction words unchanged in the adjacent-preservation corner",
        read32(exec_paddr) == instruction &&
        read32(adjacent_paddr) == adjacent
    );

    TEST_END();
}

bool ai_arch_prefetch_i_execute_trigger_repeated_breakpoint_then_all_ones_valid_fetch_recovers_adjacent_preserved_corner()
{
    TEST_START();

    uintptr_t bad_vaddr = (uintptr_t)-1;
    uintptr_t exec_vaddr = hs_page_base(VSRWX_GRWX);
    uintptr_t exec_paddr = phys_page_base(VSRWX_GRWX);
    uintptr_t adjacent_paddr = exec_paddr + sizeof(uint32_t);
    uintptr_t trigger_pc = (uintptr_t)ai_arch_execute_trigger_prefetch_i_once;
    const uint32_t instruction = 0x00008067U;
    const uint32_t adjacent = 0x00000013U;

    goto_priv(PRIV_M);
    write32(exec_paddr, instruction);
    write32(adjacent_paddr, adjacent);

    goto_priv(PRIV_HS);
    hspt_init();
    sfence_vma();

    ai_arch_arm_mcontrol6_0(trigger_pc, AI_ARCH_MCONTROL6_EXECUTE);
    uint64_t saved_mstatus = ai_arch_enable_machine_breakpoint_delivery();

    TEST_SETUP_EXCEPT();
    ai_arch_execute_trigger_prefetch_i_once(bad_vaddr);
    TEST_ASSERT("the first prefetch.i execute-PC trigger hit with an all-ones target reports breakpoint before any later canonical recovery in the adjacent-preservation corner",
        excpt.triggered == true &&
        excpt.cause == CAUSE_BKP
    );
    ai_arch_reenable_breakpoint_delivery();

    TEST_SETUP_EXCEPT();
    ai_arch_execute_trigger_prefetch_i_once(bad_vaddr);
    TEST_ASSERT("the second prefetch.i execute-PC trigger hit with the same all-ones target still reports breakpoint before any later canonical recovery in the adjacent-preservation corner",
        excpt.triggered == true &&
        excpt.cause == CAUSE_BKP
    );
    ai_arch_reenable_breakpoint_delivery();
    ai_arch_restore_machine_status(saved_mstatus);
    ai_arch_clear_mcontrol6_0();

    goto_priv(PRIV_HS);
    TEST_SETUP_EXCEPT();
    ai_arch_execute_trigger_prefetch_i_once(bad_vaddr);
    TEST_ASSERT("after clearing the repeated execute-PC trigger, the same prefetch.i to the all-ones target returns to the normal silent-hint behavior before canonical recovery in the adjacent-preservation corner",
        excpt.triggered == false
    );

    goto_priv(PRIV_M);
    TEST_ASSERT("repeated prefetch.i execute-trigger hits still leave both the target and adjacent backing instruction words unchanged before canonical recovery in the adjacent-preservation corner",
        read32(exec_paddr) == instruction &&
        read32(adjacent_paddr) == adjacent
    );

    goto_priv(PRIV_HS);
    TEST_SETUP_EXCEPT();
    ((void(*)(void))exec_vaddr)();
    TEST_ASSERT("after repeated prefetch.i execute-PC trigger hits on an all-ones target, a subsequent canonical executable HS address still runs successfully in the adjacent-preservation corner",
        excpt.triggered == false
    );

    goto_priv(PRIV_M);
    TEST_ASSERT("after the recovered all-ones prefetch.i execute-trigger path, both the target and adjacent backing instruction words remain untouched",
        read32(exec_paddr) == instruction &&
        read32(adjacent_paddr) == adjacent
    );

    TEST_END();
}

bool ai_arch_prefetch_i_execute_trigger_repeated_breakpoint_then_leaf_overflow_fetch_access_fault_adjacent_preserved_corner()
{
    TEST_START();

    uintptr_t vaddr = hs_page_base(VSRWX_GRWX);
    uintptr_t paddr = phys_page_base(VSRWX_GRWX);
    uintptr_t adjacent_paddr = paddr + sizeof(uint32_t);
    uintptr_t trigger_pc = (uintptr_t)ai_arch_execute_trigger_prefetch_i_once;
    const uint32_t instruction = 0x00008067U;
    const uint32_t adjacent = 0x00000013U;

    goto_priv(PRIV_M);
    write32(paddr, instruction);
    write32(adjacent_paddr, adjacent);

    goto_priv(PRIV_HS);
    hspt_init();

    goto_priv(PRIV_M);
    *ai_arch_trigger_hs_leaf_entry(VSRWX_GRWX) |= (1ULL << 46);
    sfence_vma();

    ai_arch_arm_mcontrol6_0(trigger_pc, AI_ARCH_MCONTROL6_EXECUTE);
    uint64_t saved_mstatus = ai_arch_enable_machine_breakpoint_delivery();

    TEST_SETUP_EXCEPT();
    ai_arch_execute_trigger_prefetch_i_once(vaddr);
    TEST_ASSERT("the first prefetch.i execute-PC trigger hit on a leaf-overflow HS target still reports breakpoint before any later fetch access fault is exposed in the adjacent-preservation corner",
        excpt.triggered == true &&
        excpt.cause == CAUSE_BKP
    );
    ai_arch_reenable_breakpoint_delivery();

    TEST_SETUP_EXCEPT();
    ai_arch_execute_trigger_prefetch_i_once(vaddr);
    TEST_ASSERT("the second prefetch.i execute-PC trigger hit on the same leaf-overflow HS target still reports breakpoint before any later fetch access fault is exposed in the adjacent-preservation corner",
        excpt.triggered == true &&
        excpt.cause == CAUSE_BKP
    );
    ai_arch_reenable_breakpoint_delivery();
    ai_arch_restore_machine_status(saved_mstatus);
    ai_arch_clear_mcontrol6_0();

    goto_priv(PRIV_HS);
    TEST_SETUP_EXCEPT();
    ai_arch_execute_trigger_prefetch_i_once(vaddr);
    TEST_ASSERT("after clearing the repeated execute-PC trigger, the same prefetch.i to the leaf-overflow target falls back to the normal silent-hint behavior in the adjacent-preservation corner",
        excpt.triggered == false
    );

    TEST_SETUP_EXCEPT();
    TEST_EXEC_EXCEPT(vaddr);
    TEST_ASSERT("after repeated triggered prefetch.i probes, a real fetch to the same leaf-overflow HS target still reports instruction access fault in the adjacent-preservation corner",
        excpt.triggered == true &&
        excpt.cause == CAUSE_IAF &&
        excpt.tval == vaddr
    );

    goto_priv(PRIV_M);
    TEST_ASSERT("repeated prefetch.i execute-trigger hits on the leaf-overflow target still leave both the target and adjacent backing instruction words unchanged in the adjacent-preservation corner",
        read32(paddr) == instruction &&
        read32(adjacent_paddr) == adjacent
    );

    goto_priv(PRIV_HS);
    hspt_init();
    sfence_vma();

    TEST_END();
}

bool ai_arch_prefetch_i_execute_trigger_repeated_breakpoint_then_leaf_overflow_fix_leaf_executes_adjacent_preserved_corner()
{
    TEST_START();

    uintptr_t vaddr = hs_page_base(VSRWX_GRWX);
    uintptr_t paddr = phys_page_base(VSRWX_GRWX);
    uintptr_t adjacent_paddr = paddr + sizeof(uint32_t);
    uintptr_t trigger_pc = (uintptr_t)ai_arch_execute_trigger_prefetch_i_once;
    const uint32_t instruction = 0x00008067U;
    const uint32_t adjacent = 0x00000013U;

    goto_priv(PRIV_M);
    write32(paddr, instruction);
    write32(adjacent_paddr, adjacent);

    goto_priv(PRIV_HS);
    hspt_init();

    goto_priv(PRIV_M);
    *ai_arch_trigger_hs_leaf_entry(VSRWX_GRWX) |= (1ULL << 46);
    sfence_vma();

    ai_arch_arm_mcontrol6_0(trigger_pc, AI_ARCH_MCONTROL6_EXECUTE);
    uint64_t saved_mstatus = ai_arch_enable_machine_breakpoint_delivery();

    TEST_SETUP_EXCEPT();
    ai_arch_execute_trigger_prefetch_i_once(vaddr);
    TEST_ASSERT("the first prefetch.i execute-PC trigger hit on a leaf-overflow HS target reports breakpoint before any later leaf repair in the adjacent-preservation corner",
        excpt.triggered == true &&
        excpt.cause == CAUSE_BKP
    );
    ai_arch_reenable_breakpoint_delivery();

    TEST_SETUP_EXCEPT();
    ai_arch_execute_trigger_prefetch_i_once(vaddr);
    TEST_ASSERT("the second prefetch.i execute-PC trigger hit on the same leaf-overflow HS target still reports breakpoint before any later leaf repair in the adjacent-preservation corner",
        excpt.triggered == true &&
        excpt.cause == CAUSE_BKP
    );
    ai_arch_reenable_breakpoint_delivery();
    ai_arch_restore_machine_status(saved_mstatus);
    ai_arch_clear_mcontrol6_0();

    goto_priv(PRIV_HS);
    TEST_SETUP_EXCEPT();
    ai_arch_execute_trigger_prefetch_i_once(vaddr);
    TEST_ASSERT("after clearing the repeated execute-PC trigger, the same prefetch.i to the leaf-overflow target returns to the normal silent-hint behavior before leaf repair in the adjacent-preservation corner",
        excpt.triggered == false
    );

    goto_priv(PRIV_M);
    TEST_ASSERT("repeated prefetch.i execute-trigger hits still leave both the target and adjacent backing instruction words unchanged before leaf repair in the adjacent-preservation corner",
        read32(paddr) == instruction &&
        read32(adjacent_paddr) == adjacent
    );
    ai_arch_trigger_set_hs_leaf(VSRWX_GRWX, paddr, PTE_V | PTE_AD | PTE_RWX);
    sfence_vma();

    goto_priv(PRIV_HS);
    TEST_SETUP_EXCEPT();
    ((void(*)(void))vaddr)();
    TEST_ASSERT("after repeated prefetch.i execute-PC trigger hits on a leaf-overflow target, repairing the leaf entry and flushing translation state lets the real fetch recover normally in the adjacent-preservation corner",
        excpt.triggered == false
    );

    goto_priv(PRIV_M);
    TEST_ASSERT("after the recovered leaf-overflow prefetch.i execute-trigger path, both the target and adjacent backing instruction words remain untouched",
        read32(paddr) == instruction &&
        read32(adjacent_paddr) == adjacent
    );

    goto_priv(PRIV_HS);
    hspt_init();
    sfence_vma();

    TEST_END();
}

bool ai_arch_load_execute_trigger_page_fault_rd_suppressed_corner()
{
    TEST_START();

    uintptr_t trigger_pc = ai_arch_execute_trigger_ld_probe_rd_load_pc();
    uintptr_t pf_vaddr = hs_page_base(VSI_GI);
    const uint64_t rd_sentinel = 0x1234facefeed5678ULL;

    ai_arch_prepare_hs_fault_page();
    ai_arch_arm_mcontrol6_0(trigger_pc, AI_ARCH_MCONTROL6_EXECUTE);
    uint64_t saved_mstatus = ai_arch_enable_machine_breakpoint_delivery();

    goto_priv(PRIV_HS);
    TEST_SETUP_EXCEPT();
    uint64_t trigger_rd =
        ai_arch_execute_trigger_ld_probe_rd_once(pf_vaddr, rd_sentinel);
    TEST_ASSERT("a load execute-PC trigger on a page-faulting HS address reports breakpoint and suppresses the load rd update",
        excpt.triggered == true &&
        excpt.cause == CAUSE_BKP &&
        trigger_rd == rd_sentinel
    );
    ai_arch_restore_machine_status(saved_mstatus);
    ai_arch_clear_mcontrol6_0();

    ai_arch_prepare_hs_fault_page();
    goto_priv(PRIV_HS);
    TEST_SETUP_EXCEPT();
    uint64_t fault_rd =
        ai_arch_execute_trigger_ld_probe_rd_once(pf_vaddr, rd_sentinel);
    TEST_ASSERT("after clearing the load execute-PC trigger, the same HS load exposes load page fault and still suppresses rd writeback",
        excpt.triggered == true &&
        excpt.cause == CAUSE_LPF &&
        excpt.tval == pf_vaddr &&
        fault_rd == rd_sentinel
    );

    TEST_END();
}

bool ai_arch_amo_address_trigger_rd_suppressed_no_rmw_then_clear_swaps_corner()
{
    TEST_START();

    uintptr_t addr = ai_arch_trigger_addr(0);
    uintptr_t guard_addr = ai_arch_trigger_addr(1);
    const uint64_t initial = 0x0000001055667788ULL;
    const uint64_t guard = 0x8877665544332211ULL;
    const uint32_t blocked_swap = 0xa1b2c3d4U;
    const uint32_t final_swap = 0x11223344U;
    const uint64_t rd_sentinel = 0xface00000000babeULL;

    ai_arch_trigger_mem[0] = initial;
    ai_arch_trigger_mem[1] = guard;

    ai_arch_arm_mcontrol6_0(addr, AI_ARCH_MCONTROL6_AMO);
    uint64_t saved_mstatus = ai_arch_enable_machine_breakpoint_delivery();

    TEST_SETUP_EXCEPT();
    uint64_t trigger_rd =
        ai_arch_trigger_amoswap_w_probe_rd(addr, blocked_swap, rd_sentinel);
    TEST_ASSERT("an AMO address trigger reports breakpoint, leaves rd at its sentinel value, and suppresses the RMW update",
        excpt.triggered == true &&
        excpt.cause == CAUSE_BKP &&
        trigger_rd == rd_sentinel &&
        ai_arch_trigger_mem[0] == initial &&
        ai_arch_trigger_mem[1] == guard
    );
    ai_arch_restore_machine_status(saved_mstatus);
    ai_arch_clear_mcontrol6_0();

    TEST_SETUP_EXCEPT();
    uint64_t old_value =
        ai_arch_trigger_amoswap_w_probe_rd(addr, final_swap, rd_sentinel);
    TEST_ASSERT("after clearing the AMO address trigger, the same AMO returns the old word and updates only the target word",
        excpt.triggered == false &&
        old_value == 0x55667788U &&
        ai_arch_trigger_mem[0] == 0x0000001011223344ULL &&
        ai_arch_trigger_mem[1] == guard
    );

    (void)guard_addr;

    TEST_END();
}

bool ai_arch_cbozero_execute_trigger_breakpoint_blocks_zero_then_clear_trigger_zeroes_block_corner()
{
    TEST_START();

    uintptr_t vaddr = hs_page_base(VSRW_GRWX) + 0x100U;
    uintptr_t block_vaddr = vaddr & ~((uintptr_t)63U);
    uintptr_t block_paddr = phys_page_base(VSRW_GRWX) + (block_vaddr - hs_page_base(VSRW_GRWX));
    uintptr_t adjacent_paddr = block_paddr + 64U;
    uintptr_t trigger_pc = (uintptr_t)ai_arch_execute_trigger_cbozero_once;
    uint64_t seed[8];
    uint64_t adjacent = 0xfeedfacecafebeefULL;

    goto_priv(PRIV_M);
    uint64_t original_menvcfg = CSRR(CSR_MENVCFG);
    CSRS(CSR_MENVCFG, MENVCFG_CBZE);
    for (size_t i = 0; i < 8U; ++i) {
        seed[i] = 0x1111222233334444ULL ^ (0x0101010101010101ULL * i);
        write64(block_paddr + i * sizeof(uint64_t), seed[i]);
    }
    write64(adjacent_paddr, adjacent);

    goto_priv(PRIV_HS);
    hspt_init();
    sfence_vma();

    ai_arch_arm_mcontrol6_0(trigger_pc, AI_ARCH_MCONTROL6_EXECUTE);
    uint64_t saved_mstatus = ai_arch_enable_machine_breakpoint_delivery();

    TEST_SETUP_EXCEPT();
    ai_arch_execute_trigger_cbozero_once(block_vaddr);
    TEST_ASSERT("a cbo.zero execute-PC trigger reports breakpoint before the block operation can retire",
        excpt.triggered == true &&
        excpt.cause == CAUSE_BKP
    );
    ai_arch_restore_machine_status(saved_mstatus);
    ai_arch_clear_mcontrol6_0();

    goto_priv(PRIV_M);
    bool unchanged = true;
    for (size_t i = 0; i < 8U; ++i) {
        unchanged = unchanged &&
            read64(block_paddr + i * sizeof(uint64_t)) == seed[i];
    }
    TEST_ASSERT("the breakpointed cbo.zero leaves the target block and adjacent guard unchanged",
        unchanged &&
        read64(adjacent_paddr) == adjacent
    );

    goto_priv(PRIV_HS);
    TEST_SETUP_EXCEPT();
    ai_arch_execute_trigger_cbozero_once(block_vaddr);
    TEST_ASSERT("after clearing the execute-PC trigger, the same cbo.zero retires normally",
        excpt.triggered == false
    );

    goto_priv(PRIV_M);
    bool zeroed = true;
    for (size_t i = 0; i < 8U; ++i) {
        zeroed = zeroed &&
            read64(block_paddr + i * sizeof(uint64_t)) == 0ULL;
    }
    TEST_ASSERT("the recovered cbo.zero zeroes exactly the target block and preserves the adjacent guard",
        zeroed &&
        read64(adjacent_paddr) == adjacent
    );
    CSRW(CSR_MENVCFG, original_menvcfg);

    TEST_END();
}
