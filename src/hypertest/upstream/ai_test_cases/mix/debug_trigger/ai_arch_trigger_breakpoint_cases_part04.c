#include "ai_arch_trigger_breakpoint_common.h"

bool ai_arch_load_execute_and_address_trigger_pbmt_nc_over_io_misaligned_repeated_breakpoint_then_valid_hs_load_recovers_corner()
{
    TEST_START();

    uintptr_t trigger_pc = (uintptr_t)ai_arch_execute_trigger_load_once;
    uintptr_t vaddr = ai_arch_trigger_prepare_nc_page_over_io_region() + 7U;
    uintptr_t recovery_paddr = phys_page_base(VSRWX_GRWX) + 0x280U;
    uintptr_t recovery_vaddr = hs_page_base(VSRWX_GRWX) + 0x280U;
    const uint64_t recovery_expected = 0x0f1e2d3c4b5a6978ULL;

    goto_priv(PRIV_M);
    write64(recovery_paddr, recovery_expected);

    ai_arch_clear_mcontrol6_slot(0);
    ai_arch_clear_mcontrol6_slot(1);
    ai_arch_arm_mcontrol6_slot(0, trigger_pc, AI_ARCH_MCONTROL6_EXECUTE);
    ai_arch_arm_mcontrol6_slot(1, vaddr, AI_ARCH_MCONTROL6_LOAD);

    uint64_t saved_mstatus = ai_arch_enable_machine_breakpoint_delivery();
    goto_priv(PRIV_HS);
    TEST_SETUP_EXCEPT();
    ai_arch_execute_trigger_load_once(vaddr);
    TEST_ASSERT("with both load trigger slots armed on the HS PBMT=NC-over-IO misaligned load, the first access reports breakpoint",
        excpt.triggered == true &&
        excpt.cause == CAUSE_BKP
    );

    TEST_SETUP_EXCEPT();
    ai_arch_execute_trigger_load_once(vaddr);
    TEST_ASSERT("with both load trigger slots still armed on that HS PBMT=NC-over-IO misaligned load, the second access still reports breakpoint",
        excpt.triggered == true &&
        excpt.cause == CAUSE_BKP
    );
    ai_arch_restore_machine_status(saved_mstatus);

    ai_arch_clear_mcontrol6_slot(0);
    saved_mstatus = ai_arch_enable_machine_breakpoint_delivery();
    goto_priv(PRIV_HS);
    TEST_SETUP_EXCEPT();
    ai_arch_execute_trigger_load_once(vaddr);
    TEST_ASSERT("after repeated dual-slot load breakpoint hits on the HS PBMT=NC-over-IO misaligned load, clearing only the execute-PC slot still leaves the address trigger active",
        excpt.triggered == true &&
        excpt.cause == CAUSE_BKP
    );
    ai_arch_restore_machine_status(saved_mstatus);

    ai_arch_clear_mcontrol6_slot(1);
    goto_priv(PRIV_HS);
    TEST_SETUP_EXCEPT();
    ai_arch_execute_trigger_load_once(vaddr);
    TEST_ASSERT("after clearing both repeated load trigger slots, the same HS PBMT=NC-over-IO misaligned load falls back to load misaligned with the raw VA in tval",
        excpt.triggered == true &&
        excpt.cause == CAUSE_LAM &&
        excpt.tval == vaddr
    );

    ai_arch_trigger_reset_svpbmt_pma_env();
    goto_priv(PRIV_HS);
    hspt_init();
    sfence_vma();

    goto_priv(PRIV_M);
    write64(recovery_paddr, recovery_expected);

    goto_priv(PRIV_HS);
    TEST_SETUP_EXCEPT();
    uint64_t recovery_value = ai_arch_execute_trigger_load_once(recovery_vaddr);
    TEST_ASSERT("after repeated dual-slot load trigger+PBMT=NC-over-IO misalign hits and the fallback misalign fault, a fresh aligned HS load still recovers normally once the IO-region setup is removed",
        excpt.triggered == false &&
        recovery_value == recovery_expected
    );

    TEST_END();
}

bool ai_arch_load_execute_and_address_trigger_pbmt_nc_over_io_misaligned_repeated_breakpoint_then_valid_hs_load_adjacent_preserved_corner()
{
    TEST_START();

    uintptr_t trigger_pc = (uintptr_t)ai_arch_execute_trigger_load_once;
    uintptr_t vaddr = ai_arch_trigger_prepare_nc_page_over_io_region() + 7U;
    uintptr_t recovery_paddr = phys_page_base(VSRWX_GRWX) + 0x280U;
    uintptr_t recovery_vaddr = hs_page_base(VSRWX_GRWX) + 0x280U;
    uintptr_t recovery_adjacent_paddr = recovery_paddr + sizeof(uint64_t);
    uintptr_t recovery_adjacent_vaddr = recovery_vaddr + sizeof(uint64_t);
    const uint64_t recovery_expected = 0x0f1e2d3c4b5a6978ULL;
    const uint64_t recovery_adjacent = 0x1021324354657687ULL;

    goto_priv(PRIV_M);
    write64(recovery_paddr, recovery_expected);
    write64(recovery_adjacent_paddr, recovery_adjacent);

    ai_arch_clear_mcontrol6_slot(0);
    ai_arch_clear_mcontrol6_slot(1);
    ai_arch_arm_mcontrol6_slot(0, trigger_pc, AI_ARCH_MCONTROL6_EXECUTE);
    ai_arch_arm_mcontrol6_slot(1, vaddr, AI_ARCH_MCONTROL6_LOAD);

    uint64_t saved_mstatus = ai_arch_enable_machine_breakpoint_delivery();
    goto_priv(PRIV_HS);
    TEST_SETUP_EXCEPT();
    ai_arch_execute_trigger_load_once(vaddr);
    TEST_ASSERT("with both load trigger slots armed on the HS PBMT=NC-over-IO misaligned load, the first access reports breakpoint in the adjacent-preservation corner",
        excpt.triggered == true &&
        excpt.cause == CAUSE_BKP
    );

    TEST_SETUP_EXCEPT();
    ai_arch_execute_trigger_load_once(vaddr);
    TEST_ASSERT("with both load trigger slots still armed on that HS PBMT=NC-over-IO misaligned load, the second access still reports breakpoint in the adjacent-preservation corner",
        excpt.triggered == true &&
        excpt.cause == CAUSE_BKP
    );
    ai_arch_restore_machine_status(saved_mstatus);

    ai_arch_clear_mcontrol6_slot(0);
    saved_mstatus = ai_arch_enable_machine_breakpoint_delivery();
    goto_priv(PRIV_HS);
    TEST_SETUP_EXCEPT();
    ai_arch_execute_trigger_load_once(vaddr);
    TEST_ASSERT("after repeated dual-slot load breakpoint hits on the HS PBMT=NC-over-IO misaligned load, clearing only the execute-PC slot still leaves the address trigger active in the adjacent-preservation corner",
        excpt.triggered == true &&
        excpt.cause == CAUSE_BKP
    );
    ai_arch_restore_machine_status(saved_mstatus);

    ai_arch_clear_mcontrol6_slot(1);
    goto_priv(PRIV_HS);
    TEST_SETUP_EXCEPT();
    ai_arch_execute_trigger_load_once(vaddr);
    TEST_ASSERT("after clearing both repeated load trigger slots, the same HS PBMT=NC-over-IO misaligned load falls back to load misaligned with the raw VA in tval in the adjacent-preservation corner",
        excpt.triggered == true &&
        excpt.cause == CAUSE_LAM &&
        excpt.tval == vaddr
    );

    ai_arch_trigger_reset_svpbmt_pma_env();
    goto_priv(PRIV_HS);
    hspt_init();
    sfence_vma();

    goto_priv(PRIV_M);
    write64(recovery_paddr, recovery_expected);
    write64(recovery_adjacent_paddr, recovery_adjacent);

    goto_priv(PRIV_HS);
    TEST_SETUP_EXCEPT();
    uint64_t recovery_value = ai_arch_execute_trigger_load_once(recovery_vaddr);
    TEST_ASSERT("after repeated dual-slot load trigger+PBMT=NC-over-IO misalign hits and the fallback misalign fault, a fresh aligned HS load still recovers normally once the IO-region setup is removed in the adjacent-preservation corner",
        excpt.triggered == false &&
        recovery_value == recovery_expected
    );

    TEST_SETUP_EXCEPT();
    uint64_t observed_adjacent = ai_arch_execute_trigger_load_once(recovery_adjacent_vaddr);
    TEST_ASSERT("the aligned recovery load after the earlier repeated trigger+PBMT=NC-over-IO misalign sequence still leaves the adjacent cacheable HS word unchanged in the adjacent-preservation corner",
        excpt.triggered == false &&
        observed_adjacent == recovery_adjacent
    );

    goto_priv(PRIV_M);
    TEST_ASSERT("after the aligned recovery load, both the target and adjacent cacheable backing words still match their expected payloads in the adjacent-preservation corner",
        read64(recovery_paddr) == recovery_expected &&
        read64(recovery_adjacent_paddr) == recovery_adjacent
    );

    TEST_END();
}

bool ai_arch_store_trigger_pbmt_nc_over_io_misaligned_no_side_effect_corner()
{
    TEST_START();

    uintptr_t vaddr = ai_arch_trigger_prepare_nc_page_over_io_region() + 7U;

    ai_arch_arm_mcontrol6_0(vaddr, AI_ARCH_MCONTROL6_STORE);
    uint64_t saved_mstatus = ai_arch_enable_machine_breakpoint_delivery();

    goto_priv(PRIV_HS);
    TEST_SETUP_EXCEPT();
    sd(vaddr, 0xdeadbeefcafef00dULL);
    if (!(excpt.triggered == true && excpt.cause == CAUSE_BKP)) {
        ai_arch_log_unexpected_trigger_result("unexpected store address-trigger+PBMT-NC-over-IO misalign priority result");
    }
    TEST_ASSERT("a store address trigger takes priority when the same HS PBMT=NC-over-IO misaligned store would otherwise report store misaligned",
        excpt.triggered == true &&
        excpt.cause == CAUSE_BKP
    );
    ai_arch_restore_machine_status(saved_mstatus);
    ai_arch_clear_mcontrol6_0();

    goto_priv(PRIV_HS);
    TEST_SETUP_EXCEPT();
    sd(vaddr, 0x0fedcba987654321ULL);
    TEST_ASSERT("after clearing the store address trigger, the same HS PBMT=NC-over-IO misaligned store falls back to store misaligned with the raw VA in tval",
        excpt.triggered == true &&
        excpt.cause == CAUSE_SAM &&
        excpt.tval == vaddr
    );

    ai_arch_trigger_reset_svpbmt_pma_env();

    TEST_END();
}

bool ai_arch_store_execute_trigger_pbmt_nc_over_io_misaligned_no_side_effect_corner()
{
    TEST_START();

    uintptr_t trigger_pc = (uintptr_t)ai_arch_execute_trigger_store_once;
    uintptr_t vaddr = ai_arch_trigger_prepare_nc_page_over_io_region() + 7U;

    ai_arch_arm_mcontrol6_0(trigger_pc, AI_ARCH_MCONTROL6_EXECUTE);
    uint64_t saved_mstatus = ai_arch_enable_machine_breakpoint_delivery();

    goto_priv(PRIV_HS);
    TEST_SETUP_EXCEPT();
    ai_arch_execute_trigger_store_once(vaddr, 0xdeadbeefcafef00dULL);
    if (!(excpt.triggered == true && excpt.cause == CAUSE_BKP)) {
        ai_arch_log_unexpected_trigger_result("unexpected store execute-PC trigger+PBMT-NC-over-IO misalign priority result");
    }
    TEST_ASSERT("a store execute-PC trigger takes priority when the same HS PBMT=NC-over-IO misaligned store would otherwise report store misaligned",
        excpt.triggered == true &&
        excpt.cause == CAUSE_BKP
    );
    ai_arch_restore_machine_status(saved_mstatus);
    ai_arch_clear_mcontrol6_0();

    goto_priv(PRIV_HS);
    TEST_SETUP_EXCEPT();
    ai_arch_execute_trigger_store_once(vaddr, 0x0fedcba987654321ULL);
    TEST_ASSERT("after clearing the store execute-PC trigger, the same HS PBMT=NC-over-IO misaligned store falls back to store misaligned with the raw VA in tval",
        excpt.triggered == true &&
        excpt.cause == CAUSE_SAM &&
        excpt.tval == vaddr
    );

    ai_arch_trigger_reset_svpbmt_pma_env();

    TEST_END();
}

bool ai_arch_store_execute_trigger_pbmt_nc_over_io_misaligned_repeated_breakpoint_then_fault_no_side_effect_corner()
{
    TEST_START();

    uintptr_t trigger_pc = (uintptr_t)ai_arch_execute_trigger_store_once;
    uintptr_t vaddr = ai_arch_trigger_prepare_nc_page_over_io_region() + 7U;

    ai_arch_arm_mcontrol6_0(trigger_pc, AI_ARCH_MCONTROL6_EXECUTE);
    uint64_t saved_mstatus = ai_arch_enable_machine_breakpoint_delivery();

    goto_priv(PRIV_HS);
    TEST_SETUP_EXCEPT();
    ai_arch_execute_trigger_store_once(vaddr, 0xdeadbeefcafef00dULL);
    TEST_ASSERT("with a store execute-PC trigger armed on the HS PBMT=NC-over-IO misaligned store, the first access reports breakpoint",
        excpt.triggered == true &&
        excpt.cause == CAUSE_BKP
    );

    TEST_SETUP_EXCEPT();
    ai_arch_execute_trigger_store_once(vaddr, 0x0fedcba987654321ULL);
    TEST_ASSERT("with the same store execute-PC trigger still armed on that HS PBMT=NC-over-IO misaligned store, the second access still reports breakpoint",
        excpt.triggered == true &&
        excpt.cause == CAUSE_BKP
    );
    ai_arch_restore_machine_status(saved_mstatus);
    ai_arch_clear_mcontrol6_0();

    goto_priv(PRIV_HS);
    TEST_SETUP_EXCEPT();
    ai_arch_execute_trigger_store_once(vaddr, 0x123456789abcdef0ULL);
    TEST_ASSERT("after clearing the repeated store execute-PC trigger, the same HS PBMT=NC-over-IO misaligned store falls back to store misaligned with the raw VA in tval",
        excpt.triggered == true &&
        excpt.cause == CAUSE_SAM &&
        excpt.tval == vaddr
    );

    ai_arch_trigger_reset_svpbmt_pma_env();

    TEST_END();
}

bool ai_arch_store_execute_and_address_trigger_pbmt_nc_over_io_misaligned_no_side_effect_corner()
{
    TEST_START();

    uintptr_t trigger_pc = (uintptr_t)ai_arch_execute_trigger_store_once;
    uintptr_t vaddr = ai_arch_trigger_prepare_nc_page_over_io_region() + 7U;
    const uint64_t store_value = 0xdeadbeefcafef00dULL;

    ai_arch_clear_mcontrol6_slot(0);
    ai_arch_clear_mcontrol6_slot(1);
    ai_arch_arm_mcontrol6_slot(0, trigger_pc, AI_ARCH_MCONTROL6_EXECUTE);
    ai_arch_arm_mcontrol6_slot(1, vaddr, AI_ARCH_MCONTROL6_STORE);

    uint64_t saved_mstatus = ai_arch_enable_machine_breakpoint_delivery();
    goto_priv(PRIV_HS);
    TEST_SETUP_EXCEPT();
    ai_arch_execute_trigger_store_once(vaddr, store_value);
    TEST_ASSERT("when both execute-PC and address triggers match the same HS PBMT=NC-over-IO misaligned store, breakpoint still wins before the store misaligned classification is observed",
        excpt.triggered == true &&
        excpt.cause == CAUSE_BKP
    );
    ai_arch_restore_machine_status(saved_mstatus);

    ai_arch_clear_mcontrol6_slot(0);
    saved_mstatus = ai_arch_enable_machine_breakpoint_delivery();
    goto_priv(PRIV_HS);
    TEST_SETUP_EXCEPT();
    ai_arch_execute_trigger_store_once(vaddr, store_value);
    TEST_ASSERT("after clearing only the execute-PC trigger, the remaining store-address trigger still reports breakpoint on the same HS PBMT=NC-over-IO misaligned store",
        excpt.triggered == true &&
        excpt.cause == CAUSE_BKP
    );
    ai_arch_restore_machine_status(saved_mstatus);

    ai_arch_clear_mcontrol6_slot(1);
    goto_priv(PRIV_HS);
    TEST_SETUP_EXCEPT();
    ai_arch_execute_trigger_store_once(vaddr, store_value);
    TEST_ASSERT("after clearing both store trigger slots, the same HS PBMT=NC-over-IO misaligned store falls back to store misaligned with the raw VA in tval",
        excpt.triggered == true &&
        excpt.cause == CAUSE_SAM &&
        excpt.tval == vaddr
    );

    ai_arch_trigger_reset_svpbmt_pma_env();

    TEST_END();
}

bool ai_arch_store_execute_and_address_trigger_pbmt_nc_over_io_misaligned_repeated_breakpoint_then_fault_no_side_effect_corner()
{
    TEST_START();

    uintptr_t trigger_pc = (uintptr_t)ai_arch_execute_trigger_store_once;
    uintptr_t vaddr = ai_arch_trigger_prepare_nc_page_over_io_region() + 7U;
    const uint64_t first_store_value = 0xdeadbeefcafef00dULL;
    const uint64_t second_store_value = 0x1122334455667788ULL;
    const uint64_t third_store_value = 0x0fedcba987654321ULL;

    ai_arch_clear_mcontrol6_slot(0);
    ai_arch_clear_mcontrol6_slot(1);
    ai_arch_arm_mcontrol6_slot(0, trigger_pc, AI_ARCH_MCONTROL6_EXECUTE);
    ai_arch_arm_mcontrol6_slot(1, vaddr, AI_ARCH_MCONTROL6_STORE);

    uint64_t saved_mstatus = ai_arch_enable_machine_breakpoint_delivery();
    goto_priv(PRIV_HS);
    TEST_SETUP_EXCEPT();
    ai_arch_execute_trigger_store_once(vaddr, first_store_value);
    TEST_ASSERT("with both store trigger slots armed on the HS PBMT=NC-over-IO misaligned store, the first access reports breakpoint",
        excpt.triggered == true &&
        excpt.cause == CAUSE_BKP
    );

    TEST_SETUP_EXCEPT();
    ai_arch_execute_trigger_store_once(vaddr, second_store_value);
    TEST_ASSERT("with both store trigger slots still armed on that HS PBMT=NC-over-IO misaligned store, the second access still reports breakpoint",
        excpt.triggered == true &&
        excpt.cause == CAUSE_BKP
    );
    ai_arch_restore_machine_status(saved_mstatus);

    ai_arch_clear_mcontrol6_slot(0);
    saved_mstatus = ai_arch_enable_machine_breakpoint_delivery();
    goto_priv(PRIV_HS);
    TEST_SETUP_EXCEPT();
    ai_arch_execute_trigger_store_once(vaddr, third_store_value);
    TEST_ASSERT("after repeated dual-slot store breakpoint hits on the HS PBMT=NC-over-IO misaligned store, clearing only the execute-PC slot still leaves the address trigger active",
        excpt.triggered == true &&
        excpt.cause == CAUSE_BKP
    );
    ai_arch_restore_machine_status(saved_mstatus);

    ai_arch_clear_mcontrol6_slot(1);
    goto_priv(PRIV_HS);
    TEST_SETUP_EXCEPT();
    ai_arch_execute_trigger_store_once(vaddr, first_store_value);
    TEST_ASSERT("after clearing both repeated store trigger slots, the same HS PBMT=NC-over-IO misaligned store falls back to store misaligned with the raw VA in tval",
        excpt.triggered == true &&
        excpt.cause == CAUSE_SAM &&
        excpt.tval == vaddr
    );

    ai_arch_trigger_reset_svpbmt_pma_env();

    TEST_END();
}

bool ai_arch_store_execute_and_address_trigger_pbmt_nc_over_io_misaligned_repeated_breakpoint_then_fault_no_side_effect_adjacent_preserved_corner()
{
    TEST_START();

    uintptr_t trigger_pc = (uintptr_t)ai_arch_execute_trigger_store_once;
    uintptr_t vaddr = ai_arch_trigger_prepare_nc_page_over_io_region() + 7U;
    const uint64_t first_store_value = 0xdeadbeefcafef00dULL;
    const uint64_t second_store_value = 0x1122334455667788ULL;
    const uint64_t third_store_value = 0x0fedcba987654321ULL;

    ai_arch_clear_mcontrol6_slot(0);
    ai_arch_clear_mcontrol6_slot(1);
    ai_arch_arm_mcontrol6_slot(0, trigger_pc, AI_ARCH_MCONTROL6_EXECUTE);
    ai_arch_arm_mcontrol6_slot(1, vaddr, AI_ARCH_MCONTROL6_STORE);

    uint64_t saved_mstatus = ai_arch_enable_machine_breakpoint_delivery();
    goto_priv(PRIV_HS);
    TEST_SETUP_EXCEPT();
    ai_arch_execute_trigger_store_once(vaddr, first_store_value);
    TEST_ASSERT("with both store trigger slots armed on the HS PBMT=NC-over-IO misaligned store, the first access reports breakpoint in the adjacent-preservation corner",
        excpt.triggered == true &&
        excpt.cause == CAUSE_BKP
    );

    TEST_SETUP_EXCEPT();
    ai_arch_execute_trigger_store_once(vaddr, second_store_value);
    TEST_ASSERT("with both store trigger slots still armed on that HS PBMT=NC-over-IO misaligned store, the second access still reports breakpoint in the adjacent-preservation corner",
        excpt.triggered == true &&
        excpt.cause == CAUSE_BKP
    );
    ai_arch_restore_machine_status(saved_mstatus);

    ai_arch_clear_mcontrol6_slot(0);
    saved_mstatus = ai_arch_enable_machine_breakpoint_delivery();
    goto_priv(PRIV_HS);
    TEST_SETUP_EXCEPT();
    ai_arch_execute_trigger_store_once(vaddr, third_store_value);
    TEST_ASSERT("after repeated dual-slot store breakpoint hits on the HS PBMT=NC-over-IO misaligned store, clearing only the execute-PC slot still leaves the address trigger active in the adjacent-preservation corner",
        excpt.triggered == true &&
        excpt.cause == CAUSE_BKP
    );
    ai_arch_restore_machine_status(saved_mstatus);

    ai_arch_clear_mcontrol6_slot(1);
    goto_priv(PRIV_HS);
    TEST_SETUP_EXCEPT();
    ai_arch_execute_trigger_store_once(vaddr, first_store_value);
    TEST_ASSERT("after clearing both repeated store trigger slots, the same HS PBMT=NC-over-IO misaligned store falls back to store misaligned with the raw VA in tval in the adjacent-preservation corner",
        excpt.triggered == true &&
        excpt.cause == CAUSE_SAM &&
        excpt.tval == vaddr
    );

    ai_arch_trigger_reset_svpbmt_pma_env();

    TEST_END();
}

bool ai_arch_store_execute_and_address_trigger_pbmt_nc_over_io_misaligned_repeated_breakpoint_then_valid_hs_store_recovers_corner()
{
    TEST_START();

    uintptr_t trigger_pc = (uintptr_t)ai_arch_execute_trigger_store_once;
    uintptr_t vaddr = ai_arch_trigger_prepare_nc_page_over_io_region() + 7U;
    uintptr_t recovery_paddr = phys_page_base(VSRW_GRW) + 0x280U;
    uintptr_t recovery_vaddr = hs_page_base(VSRW_GRW) + 0x280U;
    const uint64_t recovery_initial = 0x2233445566778899ULL;
    const uint64_t recovery_store = 0xfedcba9876543210ULL;

    goto_priv(PRIV_M);
    write64(recovery_paddr, recovery_initial);

    ai_arch_clear_mcontrol6_slot(0);
    ai_arch_clear_mcontrol6_slot(1);
    ai_arch_arm_mcontrol6_slot(0, trigger_pc, AI_ARCH_MCONTROL6_EXECUTE);
    ai_arch_arm_mcontrol6_slot(1, vaddr, AI_ARCH_MCONTROL6_STORE);

    uint64_t saved_mstatus = ai_arch_enable_machine_breakpoint_delivery();
    goto_priv(PRIV_HS);
    TEST_SETUP_EXCEPT();
    ai_arch_execute_trigger_store_once(vaddr, 0xdeadbeefcafef00dULL);
    TEST_ASSERT("with both store trigger slots armed on the HS PBMT=NC-over-IO misaligned store, the first access reports breakpoint",
        excpt.triggered == true &&
        excpt.cause == CAUSE_BKP
    );

    TEST_SETUP_EXCEPT();
    ai_arch_execute_trigger_store_once(vaddr, 0x1122334455667788ULL);
    TEST_ASSERT("with both store trigger slots still armed on that HS PBMT=NC-over-IO misaligned store, the second access still reports breakpoint",
        excpt.triggered == true &&
        excpt.cause == CAUSE_BKP
    );
    ai_arch_restore_machine_status(saved_mstatus);

    ai_arch_clear_mcontrol6_slot(0);
    saved_mstatus = ai_arch_enable_machine_breakpoint_delivery();
    goto_priv(PRIV_HS);
    TEST_SETUP_EXCEPT();
    ai_arch_execute_trigger_store_once(vaddr, 0x0fedcba987654321ULL);
    TEST_ASSERT("after repeated dual-slot store breakpoint hits on the HS PBMT=NC-over-IO misaligned store, clearing only the execute-PC slot still leaves the address trigger active",
        excpt.triggered == true &&
        excpt.cause == CAUSE_BKP
    );
    ai_arch_restore_machine_status(saved_mstatus);

    ai_arch_clear_mcontrol6_slot(1);
    goto_priv(PRIV_HS);
    TEST_SETUP_EXCEPT();
    ai_arch_execute_trigger_store_once(vaddr, 0xdeadbeefcafef00dULL);
    TEST_ASSERT("after clearing both repeated store trigger slots, the same HS PBMT=NC-over-IO misaligned store falls back to store misaligned with the raw VA in tval",
        excpt.triggered == true &&
        excpt.cause == CAUSE_SAM &&
        excpt.tval == vaddr
    );

    ai_arch_trigger_reset_svpbmt_pma_env();
    goto_priv(PRIV_HS);
    hspt_init();
    sfence_vma();

    goto_priv(PRIV_M);
    write64(recovery_paddr, recovery_initial);

    goto_priv(PRIV_HS);
    TEST_SETUP_EXCEPT();
    ai_arch_execute_trigger_store_once(recovery_vaddr, recovery_store);
    TEST_ASSERT("after repeated dual-slot store trigger+PBMT=NC-over-IO misalign hits and the fallback misalign fault, a fresh aligned HS store still recovers normally once the IO-region setup is removed",
        excpt.triggered == false
    );

    goto_priv(PRIV_M);
    TEST_ASSERT("the recovered aligned HS store commits its full payload after the earlier repeated trigger+PBMT=NC-over-IO misalign sequence is cleared",
        read64(recovery_paddr) == recovery_store
    );

    TEST_END();
}

bool ai_arch_store_execute_and_address_trigger_pbmt_nc_over_io_misaligned_repeated_breakpoint_then_valid_hs_store_adjacent_preserved_corner()
{
    TEST_START();

    uintptr_t trigger_pc = (uintptr_t)ai_arch_execute_trigger_store_once;
    uintptr_t vaddr = ai_arch_trigger_prepare_nc_page_over_io_region() + 7U;
    uintptr_t recovery_paddr = phys_page_base(VSRW_GRW) + 0x280U;
    uintptr_t recovery_vaddr = hs_page_base(VSRW_GRW) + 0x280U;
    uintptr_t recovery_adjacent_paddr = recovery_paddr + sizeof(uint64_t);
    uintptr_t recovery_adjacent_vaddr = recovery_vaddr + sizeof(uint64_t);
    const uint64_t recovery_initial = 0x2233445566778899ULL;
    const uint64_t recovery_store = 0xfedcba9876543210ULL;
    const uint64_t recovery_adjacent = 0x0123456789abcdefULL;

    goto_priv(PRIV_M);
    write64(recovery_paddr, recovery_initial);
    write64(recovery_adjacent_paddr, recovery_adjacent);

    ai_arch_clear_mcontrol6_slot(0);
    ai_arch_clear_mcontrol6_slot(1);
    ai_arch_arm_mcontrol6_slot(0, trigger_pc, AI_ARCH_MCONTROL6_EXECUTE);
    ai_arch_arm_mcontrol6_slot(1, vaddr, AI_ARCH_MCONTROL6_STORE);

    uint64_t saved_mstatus = ai_arch_enable_machine_breakpoint_delivery();
    goto_priv(PRIV_HS);
    TEST_SETUP_EXCEPT();
    ai_arch_execute_trigger_store_once(vaddr, 0xdeadbeefcafef00dULL);
    TEST_ASSERT("with both store trigger slots armed on the HS PBMT=NC-over-IO misaligned store, the first access reports breakpoint in the adjacent-preservation corner",
        excpt.triggered == true &&
        excpt.cause == CAUSE_BKP
    );

    TEST_SETUP_EXCEPT();
    ai_arch_execute_trigger_store_once(vaddr, 0x1122334455667788ULL);
    TEST_ASSERT("with both store trigger slots still armed on that HS PBMT=NC-over-IO misaligned store, the second access still reports breakpoint in the adjacent-preservation corner",
        excpt.triggered == true &&
        excpt.cause == CAUSE_BKP
    );
    ai_arch_restore_machine_status(saved_mstatus);

    ai_arch_clear_mcontrol6_slot(0);
    saved_mstatus = ai_arch_enable_machine_breakpoint_delivery();
    goto_priv(PRIV_HS);
    TEST_SETUP_EXCEPT();
    ai_arch_execute_trigger_store_once(vaddr, 0x0fedcba987654321ULL);
    TEST_ASSERT("after repeated dual-slot store breakpoint hits on the HS PBMT=NC-over-IO misaligned store, clearing only the execute-PC slot still leaves the address trigger active in the adjacent-preservation corner",
        excpt.triggered == true &&
        excpt.cause == CAUSE_BKP
    );
    ai_arch_restore_machine_status(saved_mstatus);

    ai_arch_clear_mcontrol6_slot(1);
    goto_priv(PRIV_HS);
    TEST_SETUP_EXCEPT();
    ai_arch_execute_trigger_store_once(vaddr, 0xdeadbeefcafef00dULL);
    TEST_ASSERT("after clearing both repeated store trigger slots, the same HS PBMT=NC-over-IO misaligned store falls back to store misaligned with the raw VA in tval in the adjacent-preservation corner",
        excpt.triggered == true &&
        excpt.cause == CAUSE_SAM &&
        excpt.tval == vaddr
    );

    ai_arch_trigger_reset_svpbmt_pma_env();
    goto_priv(PRIV_HS);
    hspt_init();
    sfence_vma();

    goto_priv(PRIV_M);
    write64(recovery_paddr, recovery_initial);
    write64(recovery_adjacent_paddr, recovery_adjacent);

    goto_priv(PRIV_HS);
    TEST_SETUP_EXCEPT();
    ai_arch_execute_trigger_store_once(recovery_vaddr, recovery_store);
    TEST_ASSERT("after repeated dual-slot store trigger+PBMT=NC-over-IO misalign hits and the fallback misalign fault, a fresh aligned HS store still recovers normally once the IO-region setup is removed in the adjacent-preservation corner",
        excpt.triggered == false
    );

    TEST_SETUP_EXCEPT();
    uint64_t observed_adjacent = ai_arch_execute_trigger_load_once(recovery_adjacent_vaddr);
    TEST_ASSERT("the aligned recovery store after the earlier repeated trigger+PBMT=NC-over-IO misalign sequence still leaves the adjacent cacheable HS word unchanged in the adjacent-preservation corner",
        excpt.triggered == false &&
        observed_adjacent == recovery_adjacent
    );

    goto_priv(PRIV_M);
    TEST_ASSERT("the recovered aligned HS store commits its full payload while preserving the adjacent cacheable backing word in the adjacent-preservation corner",
        read64(recovery_paddr) == recovery_store &&
        read64(recovery_adjacent_paddr) == recovery_adjacent
    );

    TEST_END();
}

bool ai_arch_prefetch_r_execute_trigger_repeated_breakpoint_corner()
{
    TEST_START();

    uintptr_t vaddr = hs_page_base(VSRWX_GRWX) + 0x180U;
    uintptr_t paddr = phys_page_base(VSRWX_GRWX) + 0x180U;
    uintptr_t adjacent_paddr = paddr + sizeof(uint64_t);
    uintptr_t trigger_pc = (uintptr_t)ai_arch_execute_trigger_prefetch_r_once;
    const uint64_t expected = 0x3141592653589793ULL;
    const uint64_t adjacent = 0x2718281828459045ULL;

    goto_priv(PRIV_M);
    write64(paddr, expected);
    write64(adjacent_paddr, adjacent);

    goto_priv(PRIV_HS);
    hspt_init();
    sfence_vma();

    ai_arch_arm_mcontrol6_0(trigger_pc, AI_ARCH_MCONTROL6_EXECUTE);
    uint64_t saved_mstatus = ai_arch_enable_machine_breakpoint_delivery();

    TEST_SETUP_EXCEPT();
    ai_arch_execute_trigger_prefetch_r_once(vaddr);
    TEST_ASSERT("the first prefetch.r execute-PC trigger hit reports breakpoint before the hint retires",
        excpt.triggered == true &&
        excpt.cause == CAUSE_BKP
    );
    ai_arch_reenable_breakpoint_delivery();

    TEST_SETUP_EXCEPT();
    ai_arch_execute_trigger_prefetch_r_once(vaddr);
    TEST_ASSERT("the second prefetch.r execute-PC trigger hit still reports breakpoint and still leaves both backing words unchanged",
        excpt.triggered == true &&
        excpt.cause == CAUSE_BKP
    );
    ai_arch_reenable_breakpoint_delivery();
    ai_arch_restore_machine_status(saved_mstatus);
    ai_arch_clear_mcontrol6_0();

    goto_priv(PRIV_HS);
    TEST_SETUP_EXCEPT();
    ai_arch_execute_trigger_prefetch_r_once(vaddr);
    TEST_ASSERT("after clearing the repeated prefetch.r execute-PC trigger, the same hint retires silently again",
        excpt.triggered == false
    );

    TEST_SETUP_EXCEPT();
    uint64_t observed = ld(vaddr);
    TEST_ASSERT("after repeated prefetch.r execute-PC trigger hits, the same HS load still returns the original payload",
        excpt.triggered == false &&
        observed == expected
    );

    goto_priv(PRIV_M);
    TEST_ASSERT("repeated prefetch.r execute-PC trigger hits preserve both the target and adjacent backing-memory words",
        read64(paddr) == expected &&
        read64(adjacent_paddr) == adjacent
    );

    TEST_END();
}

bool ai_arch_prefetch_w_execute_trigger_repeated_breakpoint_corner()
{
    TEST_START();

    uintptr_t vaddr = hs_page_base(VSRWX_GRWX) + 0x1c0U;
    uintptr_t paddr = phys_page_base(VSRWX_GRWX) + 0x1c0U;
    uintptr_t adjacent_paddr = paddr + sizeof(uint64_t);
    uintptr_t trigger_pc = (uintptr_t)ai_arch_execute_trigger_prefetch_w_once;
    const uint64_t initial = 0x8877665544332211ULL;
    const uint64_t stored = 0xdeadbeefcafef00dULL;
    const uint64_t adjacent = 0x0123456789abcdefULL;

    goto_priv(PRIV_M);
    write64(paddr, initial);
    write64(adjacent_paddr, adjacent);

    goto_priv(PRIV_HS);
    hspt_init();
    sfence_vma();

    ai_arch_arm_mcontrol6_0(trigger_pc, AI_ARCH_MCONTROL6_EXECUTE);
    ai_arch_sync_trigger_state();
    uint64_t saved_mstatus = ai_arch_enable_machine_breakpoint_delivery();

    TEST_SETUP_EXCEPT();
    ai_arch_execute_trigger_prefetch_w_once(vaddr);
    TEST_ASSERT("the first prefetch.w execute-PC trigger hit reports breakpoint before the hint retires",
        excpt.triggered == true &&
        excpt.cause == CAUSE_BKP
    );
    ai_arch_sync_trigger_state();

    TEST_SETUP_EXCEPT();
    ai_arch_execute_trigger_prefetch_w_once(vaddr);
    TEST_ASSERT("the second prefetch.w execute-PC trigger hit still reports breakpoint and still leaves both backing words unchanged",
        excpt.triggered == true &&
        excpt.cause == CAUSE_BKP
    );
    ai_arch_reenable_breakpoint_delivery();
    ai_arch_restore_machine_status(saved_mstatus);
    ai_arch_clear_mcontrol6_0();
    ai_arch_sync_trigger_state();

    goto_priv(PRIV_HS);
    TEST_SETUP_EXCEPT();
    ai_arch_execute_trigger_prefetch_w_once(vaddr);
    TEST_ASSERT("after clearing the repeated prefetch.w execute-PC trigger, the same hint retires silently again",
        excpt.triggered == false
    );

    TEST_SETUP_EXCEPT();
    sd(vaddr, stored);
    TEST_ASSERT("after repeated prefetch.w execute-PC trigger hits, a later real HS store still retires normally",
        excpt.triggered == false
    );

    TEST_SETUP_EXCEPT();
    uint64_t observed = ld(vaddr);
    TEST_ASSERT("after the recovered store path, the same HS address reads back only the architecturally committed store payload",
        excpt.triggered == false &&
        observed == stored
    );

    goto_priv(PRIV_M);
    TEST_ASSERT("repeated prefetch.w execute-PC trigger hits preserve the adjacent backing word and never write the target until the real store retires",
        read64(paddr) == stored &&
        read64(adjacent_paddr) == adjacent
    );

    TEST_END();
}

bool ai_arch_prefetch_r_execute_trigger_then_no_r_load_page_fault_corner()
{
    TEST_START();

    uintptr_t vaddr = hs_page_base(VSX_GURWX);
    uintptr_t paddr = phys_page_base(VSX_GURWX);
    uintptr_t trigger_pc = (uintptr_t)ai_arch_execute_trigger_prefetch_r_once;
    const uint64_t seed = 0x1122334455667788ULL;

    goto_priv(PRIV_M);
    write64(paddr, seed);

    goto_priv(PRIV_HS);
    hspt_init();
    sfence_vma();

    ai_arch_arm_mcontrol6_0(trigger_pc, AI_ARCH_MCONTROL6_EXECUTE);
    uint64_t saved_mstatus = ai_arch_enable_machine_breakpoint_delivery();

    TEST_SETUP_EXCEPT();
    ai_arch_execute_trigger_prefetch_r_once(vaddr);
    TEST_ASSERT("a prefetch.r execute-PC trigger reports breakpoint even when the hinted HS address is mapped through a no-R leaf",
        excpt.triggered == true &&
        excpt.cause == CAUSE_BKP
    );
    ai_arch_restore_machine_status(saved_mstatus);
    ai_arch_clear_mcontrol6_0();

    goto_priv(PRIV_HS);
    TEST_SETUP_EXCEPT();
    ai_arch_execute_trigger_prefetch_r_once(vaddr);
    TEST_ASSERT("after clearing the execute-PC trigger, the same prefetch.r to the no-R HS page falls back to the normal silent-hint behavior",
        excpt.triggered == false
    );

    TEST_SETUP_EXCEPT();
    ld(vaddr);
    TEST_ASSERT("after the triggered silent prefetch.r probe, a real load to the same no-R HS page still reports load page fault",
        excpt.triggered == true &&
        excpt.cause == CAUSE_LPF &&
        excpt.tval == vaddr
    );

    goto_priv(PRIV_M);
    TEST_ASSERT("the prefetch.r execute-trigger plus no-R load-page-fault corner leaves backing memory unchanged",
        read64(paddr) == seed
    );

    TEST_END();
}

bool ai_arch_prefetch_r_execute_trigger_repeated_breakpoint_then_no_r_load_page_fault_corner()
{
    TEST_START();

    uintptr_t vaddr = hs_page_base(VSX_GURWX);
    uintptr_t paddr = phys_page_base(VSX_GURWX);
    uintptr_t trigger_pc = (uintptr_t)ai_arch_execute_trigger_prefetch_r_once;
    const uint64_t seed = 0x99aabbccddeeff00ULL;

    goto_priv(PRIV_M);
    write64(paddr, seed);

    goto_priv(PRIV_HS);
    hspt_init();
    sfence_vma();

    ai_arch_arm_mcontrol6_0(trigger_pc, AI_ARCH_MCONTROL6_EXECUTE);
    uint64_t saved_mstatus = ai_arch_enable_machine_breakpoint_delivery();

    TEST_SETUP_EXCEPT();
    ai_arch_execute_trigger_prefetch_r_once(vaddr);
    TEST_ASSERT("the first prefetch.r execute-PC trigger hit still reports breakpoint even when the hinted HS address is mapped through a no-R leaf",
        excpt.triggered == true &&
        excpt.cause == CAUSE_BKP
    );
    ai_arch_reenable_breakpoint_delivery();

    TEST_SETUP_EXCEPT();
    ai_arch_execute_trigger_prefetch_r_once(vaddr);
    TEST_ASSERT("the second prefetch.r execute-PC trigger hit on the same no-R HS page still reports breakpoint and does not let the hint retire silently early",
        excpt.triggered == true &&
        excpt.cause == CAUSE_BKP
    );
    ai_arch_reenable_breakpoint_delivery();
    ai_arch_restore_machine_status(saved_mstatus);
    ai_arch_clear_mcontrol6_0();

    goto_priv(PRIV_HS);
    TEST_SETUP_EXCEPT();
    ai_arch_execute_trigger_prefetch_r_once(vaddr);
    TEST_ASSERT("after clearing the repeated execute-PC trigger, the same prefetch.r to the no-R HS page falls back to the normal silent-hint behavior",
        excpt.triggered == false
    );

    TEST_SETUP_EXCEPT();
    ld(vaddr);
    TEST_ASSERT("after repeated triggered prefetch.r probes, a real load to the same no-R HS page still reports load page fault",
        excpt.triggered == true &&
        excpt.cause == CAUSE_LPF &&
        excpt.tval == vaddr
    );

    goto_priv(PRIV_M);
    TEST_ASSERT("repeated prefetch.r execute-trigger hits on a no-R HS page still leave backing memory unchanged",
        read64(paddr) == seed
    );

    TEST_END();
}

bool ai_arch_prefetch_w_execute_trigger_then_no_w_store_page_fault_corner()
{
    TEST_START();

    uintptr_t vaddr = hs_page_base(VSRX_GURWX);
    uintptr_t paddr = phys_page_base(VSRX_GURWX);
    uintptr_t trigger_pc = (uintptr_t)ai_arch_execute_trigger_prefetch_w_once;
    const uint64_t seed = 0x8877665544332211ULL;
    const uint64_t denied = 0xdeadbeefcafef00dULL;

    goto_priv(PRIV_M);
    write64(paddr, seed);

    goto_priv(PRIV_HS);
    hspt_init();
    sfence_vma();

    ai_arch_arm_mcontrol6_0(trigger_pc, AI_ARCH_MCONTROL6_EXECUTE);
    uint64_t saved_mstatus = ai_arch_enable_machine_breakpoint_delivery();

    TEST_SETUP_EXCEPT();
    ai_arch_execute_trigger_prefetch_w_once(vaddr);
    TEST_ASSERT("a prefetch.w execute-PC trigger reports breakpoint even when the hinted HS address is mapped through a no-W leaf",
        excpt.triggered == true &&
        excpt.cause == CAUSE_BKP
    );
    ai_arch_restore_machine_status(saved_mstatus);
    ai_arch_clear_mcontrol6_0();

    goto_priv(PRIV_HS);
    TEST_SETUP_EXCEPT();
    ai_arch_execute_trigger_prefetch_w_once(vaddr);
    TEST_ASSERT("after clearing the execute-PC trigger, the same prefetch.w to the no-W HS page falls back to the normal silent-hint behavior",
        excpt.triggered == false
    );

    TEST_SETUP_EXCEPT();
    sd(vaddr, denied);
    TEST_ASSERT("after the triggered silent prefetch.w probe, a real store to the same no-W HS page still reports store page fault",
        excpt.triggered == true &&
        excpt.cause == CAUSE_SPF &&
        excpt.tval == vaddr
    );

    goto_priv(PRIV_M);
    TEST_ASSERT("the prefetch.w execute-trigger plus no-W store-page-fault corner leaves backing memory unchanged",
        read64(paddr) == seed
    );

    TEST_END();
}

bool ai_arch_prefetch_w_execute_trigger_repeated_breakpoint_then_no_w_store_page_fault_corner()
{
    TEST_START();

    uintptr_t vaddr = hs_page_base(VSRX_GURWX);
    uintptr_t paddr = phys_page_base(VSRX_GURWX);
    uintptr_t trigger_pc = (uintptr_t)ai_arch_execute_trigger_prefetch_w_once;
    const uint64_t seed = 0x0123456789abcdefULL;
    const uint64_t denied = 0x55aa33cc77ee11ffULL;

    goto_priv(PRIV_M);
    write64(paddr, seed);

    goto_priv(PRIV_HS);
    hspt_init();
    sfence_vma();

    ai_arch_arm_mcontrol6_0(trigger_pc, AI_ARCH_MCONTROL6_EXECUTE);
    uint64_t saved_mstatus = ai_arch_enable_machine_breakpoint_delivery();

    TEST_SETUP_EXCEPT();
    ai_arch_execute_trigger_prefetch_w_once(vaddr);
    TEST_ASSERT("the first prefetch.w execute-PC trigger hit still reports breakpoint even when the hinted HS address is mapped through a no-W leaf",
        excpt.triggered == true &&
        excpt.cause == CAUSE_BKP
    );
    ai_arch_reenable_breakpoint_delivery();

    TEST_SETUP_EXCEPT();
    ai_arch_execute_trigger_prefetch_w_once(vaddr);
    TEST_ASSERT("the second prefetch.w execute-PC trigger hit on the same no-W HS page still reports breakpoint and does not let the hint retire silently early",
        excpt.triggered == true &&
        excpt.cause == CAUSE_BKP
    );
    ai_arch_reenable_breakpoint_delivery();
    ai_arch_restore_machine_status(saved_mstatus);
    ai_arch_clear_mcontrol6_0();

    goto_priv(PRIV_HS);
    TEST_SETUP_EXCEPT();
    ai_arch_execute_trigger_prefetch_w_once(vaddr);
    TEST_ASSERT("after clearing the repeated execute-PC trigger, the same prefetch.w to the no-W HS page falls back to the normal silent-hint behavior",
        excpt.triggered == false
    );

    TEST_SETUP_EXCEPT();
    sd(vaddr, denied);
    TEST_ASSERT("after repeated triggered prefetch.w probes, a real store to the same no-W HS page still reports store page fault",
        excpt.triggered == true &&
        excpt.cause == CAUSE_SPF &&
        excpt.tval == vaddr
    );

    goto_priv(PRIV_M);
    TEST_ASSERT("repeated prefetch.w execute-trigger hits on a no-W HS page still leave backing memory unchanged",
        read64(paddr) == seed
    );

    TEST_END();
}

bool ai_arch_prefetch_r_execute_trigger_repeated_breakpoint_then_grant_r_load_recovers_corner()
{
    TEST_START();

    uintptr_t vaddr = hs_page_base(VSX_GURWX);
    uintptr_t paddr = phys_page_base(VSX_GURWX);
    uintptr_t trigger_pc = (uintptr_t)ai_arch_execute_trigger_prefetch_r_once;
    const uint64_t seed = 0x1122334455667788ULL;

    goto_priv(PRIV_M);
    write64(paddr, seed);

    goto_priv(PRIV_HS);
    hspt_init();
    sfence_vma();

    ai_arch_arm_mcontrol6_0(trigger_pc, AI_ARCH_MCONTROL6_EXECUTE);
    uint64_t saved_mstatus = ai_arch_enable_machine_breakpoint_delivery();

    TEST_SETUP_EXCEPT();
    ai_arch_execute_trigger_prefetch_r_once(vaddr);
    TEST_ASSERT("the first prefetch.r execute-PC trigger hit on a no-R HS page reports breakpoint before any later permission repair",
        excpt.triggered == true &&
        excpt.cause == CAUSE_BKP
    );
    ai_arch_reenable_breakpoint_delivery();

    TEST_SETUP_EXCEPT();
    ai_arch_execute_trigger_prefetch_r_once(vaddr);
    TEST_ASSERT("the second prefetch.r execute-PC trigger hit on the same no-R HS page still reports breakpoint before any later permission repair",
        excpt.triggered == true &&
        excpt.cause == CAUSE_BKP
    );
    ai_arch_reenable_breakpoint_delivery();
    ai_arch_restore_machine_status(saved_mstatus);
    ai_arch_clear_mcontrol6_0();

    goto_priv(PRIV_HS);
    TEST_SETUP_EXCEPT();
    ai_arch_execute_trigger_prefetch_r_once(vaddr);
    TEST_ASSERT("after clearing the repeated execute-PC trigger, the same prefetch.r returns to normal silent-hint behavior before read permission is restored",
        excpt.triggered == false
    );

    goto_priv(PRIV_M);
    TEST_ASSERT("repeated prefetch.r execute-trigger hits still leave the backing data unchanged before read permission is restored",
        read64(paddr) == seed
    );
    ai_arch_trigger_set_hs_leaf(VSX_GURWX, paddr, PTE_V | PTE_AD | PTE_RWX);
    sfence_vma();

    goto_priv(PRIV_HS);
    TEST_SETUP_EXCEPT();
    uint64_t observed = ld(vaddr);
    TEST_ASSERT("after repeated prefetch.r execute-PC trigger hits, restoring read permission and flushing translation state lets the real load recover normally",
        excpt.triggered == false &&
        observed == seed
    );

    hspt_init();
    sfence_vma();

    TEST_END();
}

bool ai_arch_prefetch_r_execute_trigger_repeated_breakpoint_then_grant_r_load_adjacent_preserved_corner()
{
    TEST_START();

    uintptr_t vaddr = hs_page_base(VSX_GURWX) + 0x40U;
    uintptr_t adjacent_vaddr = vaddr + sizeof(uint64_t);
    uintptr_t paddr = phys_page_base(VSX_GURWX) + 0x40U;
    uintptr_t adjacent_paddr = paddr + sizeof(uint64_t);
    uintptr_t trigger_pc = (uintptr_t)ai_arch_execute_trigger_prefetch_r_once;
    const uint64_t seed = 0x1122334455667788ULL;
    const uint64_t adjacent = 0xa5b4c3d2e1f00112ULL;

    goto_priv(PRIV_M);
    write64(paddr, seed);
    write64(adjacent_paddr, adjacent);

    goto_priv(PRIV_HS);
    hspt_init();
    sfence_vma();

    ai_arch_arm_mcontrol6_0(trigger_pc, AI_ARCH_MCONTROL6_EXECUTE);
    uint64_t saved_mstatus = ai_arch_enable_machine_breakpoint_delivery();

    TEST_SETUP_EXCEPT();
    ai_arch_execute_trigger_prefetch_r_once(vaddr);
    TEST_ASSERT("the first prefetch.r execute-PC trigger hit on a no-R HS page reports breakpoint before any later permission repair in the adjacent-preservation corner",
        excpt.triggered == true &&
        excpt.cause == CAUSE_BKP
    );
    ai_arch_reenable_breakpoint_delivery();

    TEST_SETUP_EXCEPT();
    ai_arch_execute_trigger_prefetch_r_once(vaddr);
    TEST_ASSERT("the second prefetch.r execute-PC trigger hit on the same no-R HS page still reports breakpoint before any later permission repair in the adjacent-preservation corner",
        excpt.triggered == true &&
        excpt.cause == CAUSE_BKP
    );
    ai_arch_reenable_breakpoint_delivery();
    ai_arch_restore_machine_status(saved_mstatus);
    ai_arch_clear_mcontrol6_0();

    goto_priv(PRIV_HS);
    TEST_SETUP_EXCEPT();
    ai_arch_execute_trigger_prefetch_r_once(vaddr);
    TEST_ASSERT("after clearing the repeated execute-PC trigger, the same prefetch.r returns to normal silent-hint behavior before read permission is restored in the adjacent-preservation corner",
        excpt.triggered == false
    );

    goto_priv(PRIV_M);
    TEST_ASSERT("repeated prefetch.r execute-trigger hits still leave the target and adjacent backing data unchanged before read permission is restored in the adjacent-preservation corner",
        read64(paddr) == seed &&
        read64(adjacent_paddr) == adjacent
    );
    ai_arch_trigger_set_hs_leaf(VSX_GURWX, phys_page_base(VSX_GURWX), PTE_V | PTE_AD | PTE_RWX);
    sfence_vma();

    goto_priv(PRIV_HS);
    TEST_SETUP_EXCEPT();
    uint64_t observed = ld(vaddr);
    TEST_ASSERT("after repeated prefetch.r execute-PC trigger hits, restoring read permission and flushing translation state lets the real load recover normally in the adjacent-preservation corner",
        excpt.triggered == false &&
        observed == seed
    );

    TEST_SETUP_EXCEPT();
    uint64_t observed_adjacent = ld(adjacent_vaddr);
    TEST_ASSERT("after the recovered prefetch.r execute-trigger read path, the adjacent HS word also remains unchanged",
        excpt.triggered == false &&
        observed_adjacent == adjacent
    );

    goto_priv(PRIV_M);
    TEST_ASSERT("the recovered prefetch.r execute-trigger plus permission-repair corner leaves both the target and adjacent backing words at their expected values",
        read64(paddr) == seed &&
        read64(adjacent_paddr) == adjacent
    );

    goto_priv(PRIV_HS);
    hspt_init();
    sfence_vma();

    TEST_END();
}

bool ai_arch_prefetch_r_address_trigger_silent_then_grant_r_load_recovers_corner()
{
    TEST_START();

    uintptr_t vaddr = hs_page_base(VSX_GURWX) + 0x80U;
    uintptr_t adjacent_vaddr = vaddr + sizeof(uint64_t);
    uintptr_t paddr = phys_page_base(VSX_GURWX) + 0x80U;
    uintptr_t adjacent_paddr = paddr + sizeof(uint64_t);
    const uint64_t seed = 0x2233445566778899ULL;
    const uint64_t adjacent = 0xa5b4c3d2e1f00112ULL;

    goto_priv(PRIV_M);
    write64(paddr, seed);
    write64(adjacent_paddr, adjacent);

    goto_priv(PRIV_HS);
    hspt_init();
    sfence_vma();

    ai_arch_arm_mcontrol6_0(vaddr, AI_ARCH_MCONTROL6_LOAD);
    uint64_t saved_mstatus = ai_arch_enable_machine_breakpoint_delivery();

    TEST_SETUP_EXCEPT();
    ai_arch_execute_trigger_prefetch_r_once(vaddr);
    TEST_ASSERT("a load-address trigger on prefetch.r does not treat the hint operand as an architectural load before read permission repair",
        excpt.triggered == false
    );
    ai_arch_reenable_breakpoint_delivery();

    TEST_SETUP_EXCEPT();
    ai_arch_execute_trigger_prefetch_r_once(vaddr);
    TEST_ASSERT("repeating prefetch.r with only a load-address trigger armed still retires as a silent hint before read permission repair",
        excpt.triggered == false
    );
    ai_arch_reenable_breakpoint_delivery();
    ai_arch_restore_machine_status(saved_mstatus);
    ai_arch_clear_mcontrol6_0();

    goto_priv(PRIV_HS);
    TEST_SETUP_EXCEPT();
    ai_arch_execute_trigger_prefetch_r_once(vaddr);
    TEST_ASSERT("after clearing the repeated address trigger, the same prefetch.r returns to normal silent-hint behavior before read permission is restored",
        excpt.triggered == false
    );

    goto_priv(PRIV_M);
    TEST_ASSERT("silent prefetch.r address-trigger probes still leave the target and adjacent backing data unchanged before read permission is restored",
        read64(paddr) == seed &&
        read64(adjacent_paddr) == adjacent
    );
    ai_arch_trigger_set_hs_leaf(VSX_GURWX, phys_page_base(VSX_GURWX), PTE_V | PTE_AD | PTE_RWX);
    sfence_vma();

    goto_priv(PRIV_HS);
    TEST_SETUP_EXCEPT();
    uint64_t observed = ld(vaddr);
    TEST_ASSERT("after silent prefetch.r address-trigger probes, restoring read permission and flushing translation state lets the real load recover normally",
        excpt.triggered == false &&
        observed == seed
    );

    TEST_SETUP_EXCEPT();
    uint64_t observed_adjacent = ld(adjacent_vaddr);
    TEST_ASSERT("after the recovered read path, the adjacent HS word also remains unchanged in the prefetch.r address-trigger repair corner",
        excpt.triggered == false &&
        observed_adjacent == adjacent
    );

    goto_priv(PRIV_HS);
    hspt_init();
    sfence_vma();

    TEST_END();
}

bool ai_arch_prefetch_r_address_trigger_silent_then_grant_r_load_adjacent_preserved_corner()
{
    TEST_START();

    uintptr_t vaddr = hs_page_base(VSX_GURWX) + 0x80U;
    uintptr_t adjacent_vaddr = vaddr + sizeof(uint64_t);
    uintptr_t paddr = phys_page_base(VSX_GURWX) + 0x80U;
    uintptr_t adjacent_paddr = paddr + sizeof(uint64_t);
    const uint64_t seed = 0x2233445566778899ULL;
    const uint64_t adjacent = 0xa5b4c3d2e1f00112ULL;

    goto_priv(PRIV_M);
    write64(paddr, seed);
    write64(adjacent_paddr, adjacent);

    goto_priv(PRIV_HS);
    hspt_init();
    sfence_vma();

    ai_arch_arm_mcontrol6_0(vaddr, AI_ARCH_MCONTROL6_LOAD);
    uint64_t saved_mstatus = ai_arch_enable_machine_breakpoint_delivery();

    TEST_SETUP_EXCEPT();
    ai_arch_execute_trigger_prefetch_r_once(vaddr);
    TEST_ASSERT("a load-address trigger on prefetch.r does not treat the hint operand as an architectural load before read permission repair in the adjacent-preservation corner",
        excpt.triggered == false
    );
    ai_arch_reenable_breakpoint_delivery();

    TEST_SETUP_EXCEPT();
    ai_arch_execute_trigger_prefetch_r_once(vaddr);
    TEST_ASSERT("repeating prefetch.r with only a load-address trigger armed still retires as a silent hint before read permission repair in the adjacent-preservation corner",
        excpt.triggered == false
    );
    ai_arch_reenable_breakpoint_delivery();
    ai_arch_restore_machine_status(saved_mstatus);
    ai_arch_clear_mcontrol6_0();

    goto_priv(PRIV_HS);
    TEST_SETUP_EXCEPT();
    ai_arch_execute_trigger_prefetch_r_once(vaddr);
    TEST_ASSERT("after clearing the repeated address trigger, the same prefetch.r returns to normal silent-hint behavior before read permission is restored in the adjacent-preservation corner",
        excpt.triggered == false
    );

    goto_priv(PRIV_M);
    TEST_ASSERT("silent prefetch.r address-trigger probes still leave the target and adjacent backing data unchanged before read permission is restored in the adjacent-preservation corner",
        read64(paddr) == seed &&
        read64(adjacent_paddr) == adjacent
    );
    ai_arch_trigger_set_hs_leaf(VSX_GURWX, phys_page_base(VSX_GURWX), PTE_V | PTE_AD | PTE_RWX);
    sfence_vma();

    goto_priv(PRIV_HS);
    TEST_SETUP_EXCEPT();
    uint64_t observed = ld(vaddr);
    TEST_ASSERT("after silent prefetch.r address-trigger probes, restoring read permission and flushing translation state lets the real load recover normally in the adjacent-preservation corner",
        excpt.triggered == false &&
        observed == seed
    );

    TEST_SETUP_EXCEPT();
    uint64_t observed_adjacent = ld(adjacent_vaddr);
    TEST_ASSERT("after the recovered prefetch.r address-trigger read path, the adjacent HS word also remains unchanged",
        excpt.triggered == false &&
        observed_adjacent == adjacent
    );

    goto_priv(PRIV_M);
    TEST_ASSERT("the recovered prefetch.r address-trigger plus permission-repair corner leaves both the target and adjacent backing words at their expected values",
        read64(paddr) == seed &&
        read64(adjacent_paddr) == adjacent
    );

    goto_priv(PRIV_HS);
    hspt_init();
    sfence_vma();

    TEST_END();
}

bool ai_arch_prefetch_r_execute_then_address_trigger_silent_grant_r_load_recovers_corner()
{
    TEST_START();

    uintptr_t vaddr = hs_page_base(VSX_GURWX) + 0xc0U;
    uintptr_t adjacent_vaddr = vaddr + sizeof(uint64_t);
    uintptr_t paddr = phys_page_base(VSX_GURWX) + 0xc0U;
    uintptr_t adjacent_paddr = paddr + sizeof(uint64_t);
    uintptr_t trigger_pc = (uintptr_t)ai_arch_execute_trigger_prefetch_r_once;
    const uint64_t seed = 0x33445566778899aaULL;
    const uint64_t adjacent = 0xb6c5d4e3f2011324ULL;

    goto_priv(PRIV_M);
    write64(paddr, seed);
    write64(adjacent_paddr, adjacent);

    goto_priv(PRIV_HS);
    hspt_init();
    sfence_vma();

    ai_arch_clear_mcontrol6_slot(0);
    ai_arch_clear_mcontrol6_slot(1);
    ai_arch_arm_mcontrol6_slot(0, trigger_pc, AI_ARCH_MCONTROL6_EXECUTE);
    ai_arch_arm_mcontrol6_slot(1, vaddr, AI_ARCH_MCONTROL6_LOAD);

    uint64_t saved_mstatus = ai_arch_enable_machine_breakpoint_delivery();
    TEST_SETUP_EXCEPT();
    ai_arch_execute_trigger_prefetch_r_once(vaddr);
    TEST_ASSERT("with both execute-PC and address triggers armed, the first prefetch.r probe on a no-R HS page reports breakpoint before any later permission repair",
        excpt.triggered == true &&
        excpt.cause == CAUSE_BKP
    );
    ai_arch_reenable_breakpoint_delivery();

    TEST_SETUP_EXCEPT();
    ai_arch_execute_trigger_prefetch_r_once(vaddr);
    TEST_ASSERT("with both execute-PC and address triggers still armed, the second prefetch.r probe on the same no-R HS page still reports breakpoint",
        excpt.triggered == true &&
        excpt.cause == CAUSE_BKP
    );
    ai_arch_reenable_breakpoint_delivery();
    ai_arch_restore_machine_status(saved_mstatus);

    ai_arch_clear_mcontrol6_slot(0);
    saved_mstatus = ai_arch_enable_machine_breakpoint_delivery();
    goto_priv(PRIV_HS);
    TEST_SETUP_EXCEPT();
    ai_arch_execute_trigger_prefetch_r_once(vaddr);
    TEST_ASSERT("after clearing the execute-PC slot, the remaining load-address trigger does not fire on the prefetch.r hint operand",
        excpt.triggered == false
    );
    ai_arch_restore_machine_status(saved_mstatus);

    ai_arch_clear_mcontrol6_slot(1);
    goto_priv(PRIV_HS);
    TEST_SETUP_EXCEPT();
    ai_arch_execute_trigger_prefetch_r_once(vaddr);
    TEST_ASSERT("after clearing both repeated prefetch.r trigger slots, the same hint returns to normal silent behavior before read permission is restored",
        excpt.triggered == false
    );

    goto_priv(PRIV_M);
    TEST_ASSERT("execute-triggered prefetch.r hits plus an address-trigger-silent hint still leave the target and adjacent backing data unchanged before read permission is restored",
        read64(paddr) == seed &&
        read64(adjacent_paddr) == adjacent
    );
    ai_arch_trigger_set_hs_leaf(VSX_GURWX, phys_page_base(VSX_GURWX), PTE_V | PTE_AD | PTE_RWX);
    sfence_vma();

    goto_priv(PRIV_HS);
    TEST_SETUP_EXCEPT();
    uint64_t observed = ld(vaddr);
    TEST_ASSERT("after execute-triggered prefetch.r hits followed by an address-trigger-silent hint, restoring read permission and flushing translation state lets the real load recover normally",
        excpt.triggered == false &&
        observed == seed
    );

    TEST_SETUP_EXCEPT();
    uint64_t observed_adjacent = ld(adjacent_vaddr);
    TEST_ASSERT("after the recovered dual-slot prefetch.r path, the adjacent HS word also remains unchanged",
        excpt.triggered == false &&
        observed_adjacent == adjacent
    );

    ai_arch_clear_mcontrol6_slot(0);
    ai_arch_clear_mcontrol6_slot(1);

    goto_priv(PRIV_HS);
    hspt_init();
    sfence_vma();

    TEST_END();
}

bool ai_arch_prefetch_r_execute_then_address_trigger_silent_grant_r_load_adjacent_preserved_corner()
{
    TEST_START();

    uintptr_t vaddr = hs_page_base(VSX_GURWX) + 0xc0U;
    uintptr_t adjacent_vaddr = vaddr + sizeof(uint64_t);
    uintptr_t paddr = phys_page_base(VSX_GURWX) + 0xc0U;
    uintptr_t adjacent_paddr = paddr + sizeof(uint64_t);
    uintptr_t trigger_pc = (uintptr_t)ai_arch_execute_trigger_prefetch_r_once;
    const uint64_t seed = 0x33445566778899aaULL;
    const uint64_t adjacent = 0xb6c5d4e3f2011324ULL;

    goto_priv(PRIV_M);
    write64(paddr, seed);
    write64(adjacent_paddr, adjacent);

    goto_priv(PRIV_HS);
    hspt_init();
    sfence_vma();

    ai_arch_clear_mcontrol6_slot(0);
    ai_arch_clear_mcontrol6_slot(1);
    ai_arch_arm_mcontrol6_slot(0, trigger_pc, AI_ARCH_MCONTROL6_EXECUTE);
    ai_arch_arm_mcontrol6_slot(1, vaddr, AI_ARCH_MCONTROL6_LOAD);

    uint64_t saved_mstatus = ai_arch_enable_machine_breakpoint_delivery();
    TEST_SETUP_EXCEPT();
    ai_arch_execute_trigger_prefetch_r_once(vaddr);
    TEST_ASSERT("with both execute-PC and address triggers armed, the first prefetch.r probe on a no-R HS page reports breakpoint before any later permission repair in the adjacent-preservation corner",
        excpt.triggered == true &&
        excpt.cause == CAUSE_BKP
    );
    ai_arch_reenable_breakpoint_delivery();

    TEST_SETUP_EXCEPT();
    ai_arch_execute_trigger_prefetch_r_once(vaddr);
    TEST_ASSERT("with both execute-PC and address triggers still armed, the second prefetch.r probe on the same no-R HS page still reports breakpoint in the adjacent-preservation corner",
        excpt.triggered == true &&
        excpt.cause == CAUSE_BKP
    );
    ai_arch_reenable_breakpoint_delivery();
    ai_arch_restore_machine_status(saved_mstatus);

    ai_arch_clear_mcontrol6_slot(0);
    saved_mstatus = ai_arch_enable_machine_breakpoint_delivery();
    goto_priv(PRIV_HS);
    TEST_SETUP_EXCEPT();
    ai_arch_execute_trigger_prefetch_r_once(vaddr);
    TEST_ASSERT("after clearing the execute-PC slot, the remaining load-address trigger does not fire on the prefetch.r hint operand in the adjacent-preservation corner",
        excpt.triggered == false
    );
    ai_arch_restore_machine_status(saved_mstatus);

    ai_arch_clear_mcontrol6_slot(1);
    goto_priv(PRIV_HS);
    TEST_SETUP_EXCEPT();
    ai_arch_execute_trigger_prefetch_r_once(vaddr);
    TEST_ASSERT("after clearing both repeated prefetch.r trigger slots, the same hint returns to normal silent behavior before read permission is restored in the adjacent-preservation corner",
        excpt.triggered == false
    );

    goto_priv(PRIV_M);
    TEST_ASSERT("execute-triggered prefetch.r hits plus an address-trigger-silent hint still leave the target and adjacent backing data unchanged before read permission is restored in the adjacent-preservation corner",
        read64(paddr) == seed &&
        read64(adjacent_paddr) == adjacent
    );
    ai_arch_trigger_set_hs_leaf(VSX_GURWX, phys_page_base(VSX_GURWX), PTE_V | PTE_AD | PTE_RWX);
    sfence_vma();

    goto_priv(PRIV_HS);
    TEST_SETUP_EXCEPT();
    uint64_t observed = ld(vaddr);
    TEST_ASSERT("after execute-triggered prefetch.r hits followed by an address-trigger-silent hint, restoring read permission and flushing translation state lets the real load recover normally in the adjacent-preservation corner",
        excpt.triggered == false &&
        observed == seed
    );

    TEST_SETUP_EXCEPT();
    uint64_t observed_adjacent = ld(adjacent_vaddr);
    TEST_ASSERT("after the recovered dual-slot prefetch.r path, the adjacent HS word also remains unchanged in the adjacent-preservation corner",
        excpt.triggered == false &&
        observed_adjacent == adjacent
    );

    goto_priv(PRIV_M);
    TEST_ASSERT("the recovered dual-slot prefetch.r permission-repair corner leaves both the target and adjacent backing words at their expected values",
        read64(paddr) == seed &&
        read64(adjacent_paddr) == adjacent
    );

    ai_arch_clear_mcontrol6_slot(0);
    ai_arch_clear_mcontrol6_slot(1);

    goto_priv(PRIV_HS);
    hspt_init();
    sfence_vma();

    TEST_END();
}

bool ai_arch_prefetch_w_execute_trigger_repeated_breakpoint_then_grant_w_store_recovers_corner()
{
    TEST_START();

    uintptr_t vaddr = hs_page_base(VSRX_GURWX);
    uintptr_t paddr = phys_page_base(VSRX_GURWX);
    uintptr_t trigger_pc = (uintptr_t)ai_arch_execute_trigger_prefetch_w_once;
    const uint64_t seed = 0x8877665544332211ULL;
    const uint64_t stored = 0xdeadbeefcafef00dULL;

    goto_priv(PRIV_M);
    write64(paddr, seed);

    goto_priv(PRIV_HS);
    hspt_init();
    sfence_vma();

    ai_arch_arm_mcontrol6_0(trigger_pc, AI_ARCH_MCONTROL6_EXECUTE);
    uint64_t saved_mstatus = ai_arch_enable_machine_breakpoint_delivery();

    TEST_SETUP_EXCEPT();
    ai_arch_execute_trigger_prefetch_w_once(vaddr);
    TEST_ASSERT("the first prefetch.w execute-PC trigger hit on a no-W HS page reports breakpoint before any later permission repair",
        excpt.triggered == true &&
        excpt.cause == CAUSE_BKP
    );
    ai_arch_reenable_breakpoint_delivery();

    TEST_SETUP_EXCEPT();
    ai_arch_execute_trigger_prefetch_w_once(vaddr);
    TEST_ASSERT("the second prefetch.w execute-PC trigger hit on the same no-W HS page still reports breakpoint before any later permission repair",
        excpt.triggered == true &&
        excpt.cause == CAUSE_BKP
    );
    ai_arch_reenable_breakpoint_delivery();
    ai_arch_restore_machine_status(saved_mstatus);
    ai_arch_clear_mcontrol6_0();

    goto_priv(PRIV_HS);
    TEST_SETUP_EXCEPT();
    ai_arch_execute_trigger_prefetch_w_once(vaddr);
    TEST_ASSERT("after clearing the repeated execute-PC trigger, the same prefetch.w returns to normal silent-hint behavior before write permission is restored",
        excpt.triggered == false
    );

    goto_priv(PRIV_M);
    TEST_ASSERT("repeated prefetch.w execute-trigger hits still leave the backing data unchanged before write permission is restored",
        read64(paddr) == seed
    );
    ai_arch_trigger_set_hs_leaf(VSRX_GURWX, paddr, PTE_V | PTE_AD | PTE_RWX);
    sfence_vma();

    goto_priv(PRIV_HS);
    TEST_SETUP_EXCEPT();
    sd(vaddr, stored);
    TEST_ASSERT("after repeated prefetch.w execute-PC trigger hits, restoring write permission and flushing translation state lets the real store recover normally",
        excpt.triggered == false
    );

    TEST_SETUP_EXCEPT();
    uint64_t observed = ld(vaddr);
    TEST_ASSERT("after the recovered store path, the same HS address reads back only the architecturally committed store payload",
        excpt.triggered == false &&
        observed == stored
    );

    goto_priv(PRIV_M);
    TEST_ASSERT("the recovered prefetch.w execute-trigger plus permission-repair corner updates backing memory only after write permission is restored",
        read64(paddr) == stored
    );

    goto_priv(PRIV_HS);
    hspt_init();
    sfence_vma();

    TEST_END();
}

bool ai_arch_prefetch_w_execute_trigger_repeated_breakpoint_then_grant_w_store_adjacent_preserved_corner()
{
    TEST_START();

    uintptr_t vaddr = hs_page_base(VSRX_GURWX) + 0x60U;
    uintptr_t adjacent_vaddr = vaddr + sizeof(uint64_t);
    uintptr_t paddr = phys_page_base(VSRX_GURWX) + 0x60U;
    uintptr_t adjacent_paddr = paddr + sizeof(uint64_t);
    uintptr_t trigger_pc = (uintptr_t)ai_arch_execute_trigger_prefetch_w_once;
    const uint64_t seed = 0x8877665544332211ULL;
    const uint64_t stored = 0xdeadbeefcafef00dULL;
    const uint64_t adjacent = 0x123456789abcdef0ULL;

    goto_priv(PRIV_M);
    write64(paddr, seed);
    write64(adjacent_paddr, adjacent);

    goto_priv(PRIV_HS);
    hspt_init();
    sfence_vma();

    ai_arch_arm_mcontrol6_0(trigger_pc, AI_ARCH_MCONTROL6_EXECUTE);
    uint64_t saved_mstatus = ai_arch_enable_machine_breakpoint_delivery();

    TEST_SETUP_EXCEPT();
    ai_arch_execute_trigger_prefetch_w_once(vaddr);
    TEST_ASSERT("the first prefetch.w execute-PC trigger hit on a no-W HS page reports breakpoint before any later permission repair in the adjacent-preservation corner",
        excpt.triggered == true &&
        excpt.cause == CAUSE_BKP
    );
    ai_arch_reenable_breakpoint_delivery();

    TEST_SETUP_EXCEPT();
    ai_arch_execute_trigger_prefetch_w_once(vaddr);
    TEST_ASSERT("the second prefetch.w execute-PC trigger hit on the same no-W HS page still reports breakpoint before any later permission repair in the adjacent-preservation corner",
        excpt.triggered == true &&
        excpt.cause == CAUSE_BKP
    );
    ai_arch_reenable_breakpoint_delivery();
    ai_arch_restore_machine_status(saved_mstatus);
    ai_arch_clear_mcontrol6_0();

    goto_priv(PRIV_HS);
    TEST_SETUP_EXCEPT();
    ai_arch_execute_trigger_prefetch_w_once(vaddr);
    TEST_ASSERT("after clearing the repeated execute-PC trigger, the same prefetch.w returns to normal silent-hint behavior before write permission is restored in the adjacent-preservation corner",
        excpt.triggered == false
    );

    goto_priv(PRIV_M);
    TEST_ASSERT("repeated prefetch.w execute-trigger hits still leave the target and adjacent backing data unchanged before write permission is restored in the adjacent-preservation corner",
        read64(paddr) == seed &&
        read64(adjacent_paddr) == adjacent
    );
    ai_arch_trigger_set_hs_leaf(VSRX_GURWX, phys_page_base(VSRX_GURWX), PTE_V | PTE_AD | PTE_RWX);
    sfence_vma();

    goto_priv(PRIV_HS);
    TEST_SETUP_EXCEPT();
    sd(vaddr, stored);
    TEST_ASSERT("after repeated prefetch.w execute-PC trigger hits, restoring write permission and flushing translation state lets the real store recover normally in the adjacent-preservation corner",
        excpt.triggered == false
    );

    TEST_SETUP_EXCEPT();
    uint64_t observed = ld(vaddr);
    TEST_ASSERT("after the recovered prefetch.w execute-trigger store path, the same HS address reads back only the architecturally committed store payload in the adjacent-preservation corner",
        excpt.triggered == false &&
        observed == stored
    );

    TEST_SETUP_EXCEPT();
    uint64_t observed_adjacent = ld(adjacent_vaddr);
    TEST_ASSERT("after the recovered prefetch.w execute-trigger store path, the adjacent HS word also remains unchanged",
        excpt.triggered == false &&
        observed_adjacent == adjacent
    );

    goto_priv(PRIV_M);
    TEST_ASSERT("the recovered prefetch.w execute-trigger plus permission-repair corner updates only the target backing word while leaving the adjacent word untouched",
        read64(paddr) == stored &&
        read64(adjacent_paddr) == adjacent
    );

    goto_priv(PRIV_HS);
    hspt_init();
    sfence_vma();

    TEST_END();
}

bool ai_arch_prefetch_w_address_trigger_silent_then_grant_w_store_recovers_corner()
{
    TEST_START();

    uintptr_t vaddr = hs_page_base(VSRX_GURWX) + 0xa0U;
    uintptr_t adjacent_vaddr = vaddr + sizeof(uint64_t);
    uintptr_t paddr = phys_page_base(VSRX_GURWX) + 0xa0U;
    uintptr_t adjacent_paddr = paddr + sizeof(uint64_t);
    const uint64_t seed = 0x9988776655443322ULL;
    const uint64_t stored = 0xcafebabedeadf00dULL;
    const uint64_t adjacent = 0x123456789abcdef0ULL;

    goto_priv(PRIV_M);
    write64(paddr, seed);
    write64(adjacent_paddr, adjacent);

    goto_priv(PRIV_HS);
    hspt_init();
    sfence_vma();

    ai_arch_arm_mcontrol6_0(vaddr, AI_ARCH_MCONTROL6_STORE);
    uint64_t saved_mstatus = ai_arch_enable_machine_breakpoint_delivery();

    TEST_SETUP_EXCEPT();
    ai_arch_execute_trigger_prefetch_w_once(vaddr);
    TEST_ASSERT("a store-address trigger on prefetch.w does not treat the hint operand as an architectural store before write permission repair",
        excpt.triggered == false
    );
    ai_arch_reenable_breakpoint_delivery();

    TEST_SETUP_EXCEPT();
    ai_arch_execute_trigger_prefetch_w_once(vaddr);
    TEST_ASSERT("repeating prefetch.w with only a store-address trigger armed still retires as a silent hint before write permission repair",
        excpt.triggered == false
    );
    ai_arch_reenable_breakpoint_delivery();
    ai_arch_restore_machine_status(saved_mstatus);
    ai_arch_clear_mcontrol6_0();

    goto_priv(PRIV_HS);
    TEST_SETUP_EXCEPT();
    ai_arch_execute_trigger_prefetch_w_once(vaddr);
    TEST_ASSERT("after clearing the repeated address trigger, the same prefetch.w returns to normal silent-hint behavior before write permission is restored",
        excpt.triggered == false
    );

    goto_priv(PRIV_M);
    TEST_ASSERT("silent prefetch.w address-trigger probes still leave the target and adjacent backing data unchanged before write permission is restored",
        read64(paddr) == seed &&
        read64(adjacent_paddr) == adjacent
    );
    ai_arch_trigger_set_hs_leaf(VSRX_GURWX, phys_page_base(VSRX_GURWX), PTE_V | PTE_AD | PTE_RWX);
    sfence_vma();

    goto_priv(PRIV_HS);
    TEST_SETUP_EXCEPT();
    sd(vaddr, stored);
    TEST_ASSERT("after silent prefetch.w address-trigger probes, restoring write permission and flushing translation state lets the real store recover normally",
        excpt.triggered == false
    );

    TEST_SETUP_EXCEPT();
    uint64_t observed = ld(vaddr);
    TEST_ASSERT("after the recovered store path, the same HS address reads back only the architecturally committed store payload in the prefetch.w address-trigger repair corner",
        excpt.triggered == false &&
        observed == stored
    );

    TEST_SETUP_EXCEPT();
    uint64_t observed_adjacent = ld(adjacent_vaddr);
    TEST_ASSERT("after the recovered store path, the adjacent HS word also remains unchanged in the prefetch.w address-trigger repair corner",
        excpt.triggered == false &&
        observed_adjacent == adjacent
    );

    goto_priv(PRIV_M);
    TEST_ASSERT("the recovered prefetch.w address-trigger plus permission-repair corner updates only the target backing word",
        read64(paddr) == stored &&
        read64(adjacent_paddr) == adjacent
    );

    goto_priv(PRIV_HS);
    hspt_init();
    sfence_vma();

    TEST_END();
}

bool ai_arch_prefetch_w_address_trigger_silent_then_grant_w_store_adjacent_preserved_corner()
{
    TEST_START();

    uintptr_t vaddr = hs_page_base(VSRX_GURWX) + 0xa0U;
    uintptr_t adjacent_vaddr = vaddr + sizeof(uint64_t);
    uintptr_t paddr = phys_page_base(VSRX_GURWX) + 0xa0U;
    uintptr_t adjacent_paddr = paddr + sizeof(uint64_t);
    const uint64_t seed = 0x9988776655443322ULL;
    const uint64_t stored = 0xcafebabedeadf00dULL;
    const uint64_t adjacent = 0x123456789abcdef0ULL;

    goto_priv(PRIV_M);
    write64(paddr, seed);
    write64(adjacent_paddr, adjacent);

    goto_priv(PRIV_HS);
    hspt_init();
    sfence_vma();

    ai_arch_arm_mcontrol6_0(vaddr, AI_ARCH_MCONTROL6_STORE);
    uint64_t saved_mstatus = ai_arch_enable_machine_breakpoint_delivery();

    TEST_SETUP_EXCEPT();
    ai_arch_execute_trigger_prefetch_w_once(vaddr);
    TEST_ASSERT("a store-address trigger on prefetch.w does not treat the hint operand as an architectural store before write permission repair in the adjacent-preservation corner",
        excpt.triggered == false
    );
    ai_arch_reenable_breakpoint_delivery();

    TEST_SETUP_EXCEPT();
    ai_arch_execute_trigger_prefetch_w_once(vaddr);
    TEST_ASSERT("repeating prefetch.w with only a store-address trigger armed still retires as a silent hint before write permission repair in the adjacent-preservation corner",
        excpt.triggered == false
    );
    ai_arch_reenable_breakpoint_delivery();
    ai_arch_restore_machine_status(saved_mstatus);
    ai_arch_clear_mcontrol6_0();

    goto_priv(PRIV_HS);
    TEST_SETUP_EXCEPT();
    ai_arch_execute_trigger_prefetch_w_once(vaddr);
    TEST_ASSERT("after clearing the repeated address trigger, the same prefetch.w returns to normal silent-hint behavior before write permission is restored in the adjacent-preservation corner",
        excpt.triggered == false
    );

    goto_priv(PRIV_M);
    TEST_ASSERT("silent prefetch.w address-trigger probes still leave the target and adjacent backing data unchanged before write permission is restored in the adjacent-preservation corner",
        read64(paddr) == seed &&
        read64(adjacent_paddr) == adjacent
    );
    ai_arch_trigger_set_hs_leaf(VSRX_GURWX, phys_page_base(VSRX_GURWX), PTE_V | PTE_AD | PTE_RWX);
    sfence_vma();

    goto_priv(PRIV_HS);
    TEST_SETUP_EXCEPT();
    sd(vaddr, stored);
    TEST_ASSERT("after silent prefetch.w address-trigger probes, restoring write permission and flushing translation state lets the real store recover normally in the adjacent-preservation corner",
        excpt.triggered == false
    );

    TEST_SETUP_EXCEPT();
    uint64_t observed = ld(vaddr);
    TEST_ASSERT("after the recovered prefetch.w address-trigger store path, the same HS address reads back only the architecturally committed store payload in the adjacent-preservation corner",
        excpt.triggered == false &&
        observed == stored
    );

    TEST_SETUP_EXCEPT();
    uint64_t observed_adjacent = ld(adjacent_vaddr);
    TEST_ASSERT("after the recovered prefetch.w address-trigger store path, the adjacent HS word also remains unchanged",
        excpt.triggered == false &&
        observed_adjacent == adjacent
    );

    goto_priv(PRIV_M);
    TEST_ASSERT("the recovered prefetch.w address-trigger plus permission-repair corner updates only the target backing word while leaving the adjacent word untouched",
        read64(paddr) == stored &&
        read64(adjacent_paddr) == adjacent
    );

    goto_priv(PRIV_HS);
    hspt_init();
    sfence_vma();

    TEST_END();
}

bool ai_arch_prefetch_w_execute_then_address_trigger_silent_grant_w_store_recovers_corner()
{
    TEST_START();

    uintptr_t vaddr = hs_page_base(VSRX_GURWX) + 0xe0U;
    uintptr_t adjacent_vaddr = vaddr + sizeof(uint64_t);
    uintptr_t paddr = phys_page_base(VSRX_GURWX) + 0xe0U;
    uintptr_t adjacent_paddr = paddr + sizeof(uint64_t);
    uintptr_t trigger_pc = (uintptr_t)ai_arch_execute_trigger_prefetch_w_once;
    const uint64_t seed = 0xaa99887766554433ULL;
    const uint64_t stored = 0x0ddf00dcafebab1eULL;
    const uint64_t adjacent = 0x0f1e2d3c4b5a6978ULL;

    goto_priv(PRIV_M);
    write64(paddr, seed);
    write64(adjacent_paddr, adjacent);

    goto_priv(PRIV_HS);
    hspt_init();
    sfence_vma();

    ai_arch_clear_mcontrol6_slot(0);
    ai_arch_clear_mcontrol6_slot(1);
    ai_arch_arm_mcontrol6_slot(0, trigger_pc, AI_ARCH_MCONTROL6_EXECUTE);
    ai_arch_arm_mcontrol6_slot(1, vaddr, AI_ARCH_MCONTROL6_STORE);

    uint64_t saved_mstatus = ai_arch_enable_machine_breakpoint_delivery();
    TEST_SETUP_EXCEPT();
    ai_arch_execute_trigger_prefetch_w_once(vaddr);
    TEST_ASSERT("with both execute-PC and address triggers armed, the first prefetch.w probe on a no-W HS page reports breakpoint before any later permission repair",
        excpt.triggered == true &&
        excpt.cause == CAUSE_BKP
    );
    ai_arch_reenable_breakpoint_delivery();

    TEST_SETUP_EXCEPT();
    ai_arch_execute_trigger_prefetch_w_once(vaddr);
    TEST_ASSERT("with both execute-PC and address triggers still armed, the second prefetch.w probe on the same no-W HS page still reports breakpoint",
        excpt.triggered == true &&
        excpt.cause == CAUSE_BKP
    );
    ai_arch_reenable_breakpoint_delivery();
    ai_arch_restore_machine_status(saved_mstatus);

    ai_arch_clear_mcontrol6_slot(0);
    saved_mstatus = ai_arch_enable_machine_breakpoint_delivery();
    goto_priv(PRIV_HS);
    TEST_SETUP_EXCEPT();
    ai_arch_execute_trigger_prefetch_w_once(vaddr);
    TEST_ASSERT("after clearing the execute-PC slot, the remaining store-address trigger does not fire on the prefetch.w hint operand",
        excpt.triggered == false
    );
    ai_arch_restore_machine_status(saved_mstatus);

    ai_arch_clear_mcontrol6_slot(1);
    goto_priv(PRIV_HS);
    TEST_SETUP_EXCEPT();
    ai_arch_execute_trigger_prefetch_w_once(vaddr);
    TEST_ASSERT("after clearing both repeated prefetch.w trigger slots, the same hint returns to normal silent behavior before write permission is restored",
        excpt.triggered == false
    );

    goto_priv(PRIV_M);
    TEST_ASSERT("execute-triggered prefetch.w hits plus an address-trigger-silent hint still leave the target and adjacent backing data unchanged before write permission is restored",
        read64(paddr) == seed &&
        read64(adjacent_paddr) == adjacent
    );
    ai_arch_trigger_set_hs_leaf(VSRX_GURWX, phys_page_base(VSRX_GURWX), PTE_V | PTE_AD | PTE_RWX);
    sfence_vma();

    goto_priv(PRIV_HS);
    TEST_SETUP_EXCEPT();
    sd(vaddr, stored);
    TEST_ASSERT("after execute-triggered prefetch.w hits followed by an address-trigger-silent hint, restoring write permission and flushing translation state lets the real store recover normally",
        excpt.triggered == false
    );

    TEST_SETUP_EXCEPT();
    uint64_t observed = ld(vaddr);
    TEST_ASSERT("after the recovered dual-slot prefetch.w path, the same HS address reads back only the architecturally committed store payload",
        excpt.triggered == false &&
        observed == stored
    );

    TEST_SETUP_EXCEPT();
    uint64_t observed_adjacent = ld(adjacent_vaddr);
    TEST_ASSERT("after the recovered dual-slot prefetch.w path, the adjacent HS word also remains unchanged",
        excpt.triggered == false &&
        observed_adjacent == adjacent
    );

    goto_priv(PRIV_M);
    TEST_ASSERT("the recovered dual-slot prefetch.w permission-repair corner updates only the target backing word",
        read64(paddr) == stored &&
        read64(adjacent_paddr) == adjacent
    );

    ai_arch_clear_mcontrol6_slot(0);
    ai_arch_clear_mcontrol6_slot(1);

    goto_priv(PRIV_HS);
    hspt_init();
    sfence_vma();

    TEST_END();
}

bool ai_arch_prefetch_w_execute_then_address_trigger_silent_grant_w_store_adjacent_preserved_corner()
{
    TEST_START();

    uintptr_t vaddr = hs_page_base(VSRX_GURWX) + 0xe0U;
    uintptr_t adjacent_vaddr = vaddr + sizeof(uint64_t);
    uintptr_t paddr = phys_page_base(VSRX_GURWX) + 0xe0U;
    uintptr_t adjacent_paddr = paddr + sizeof(uint64_t);
    uintptr_t trigger_pc = (uintptr_t)ai_arch_execute_trigger_prefetch_w_once;
    const uint64_t seed = 0xaa99887766554433ULL;
    const uint64_t stored = 0x0ddf00dcafebab1eULL;
    const uint64_t adjacent = 0x0f1e2d3c4b5a6978ULL;

    goto_priv(PRIV_M);
    write64(paddr, seed);
    write64(adjacent_paddr, adjacent);

    goto_priv(PRIV_HS);
    hspt_init();
    sfence_vma();

    ai_arch_clear_mcontrol6_slot(0);
    ai_arch_clear_mcontrol6_slot(1);
    ai_arch_arm_mcontrol6_slot(0, trigger_pc, AI_ARCH_MCONTROL6_EXECUTE);
    ai_arch_arm_mcontrol6_slot(1, vaddr, AI_ARCH_MCONTROL6_STORE);
    ai_arch_sync_trigger_state();

    uint64_t saved_mstatus = ai_arch_enable_machine_breakpoint_delivery();
    TEST_SETUP_EXCEPT();
    ai_arch_execute_trigger_prefetch_w_once(vaddr);
    TEST_ASSERT("with both execute-PC and address triggers armed, the first prefetch.w probe on a no-W HS page reports breakpoint before any later permission repair in the adjacent-preservation corner",
        excpt.triggered == true &&
        excpt.cause == CAUSE_BKP
    );
    ai_arch_sync_trigger_state();

    TEST_SETUP_EXCEPT();
    ai_arch_execute_trigger_prefetch_w_once(vaddr);
    TEST_ASSERT("with both execute-PC and address triggers still armed, the second prefetch.w probe on the same no-W HS page still reports breakpoint in the adjacent-preservation corner",
        excpt.triggered == true &&
        excpt.cause == CAUSE_BKP
    );
    ai_arch_reenable_breakpoint_delivery();
    ai_arch_restore_machine_status(saved_mstatus);

    ai_arch_clear_mcontrol6_slot(0);
    ai_arch_sync_trigger_state();
    saved_mstatus = ai_arch_enable_machine_breakpoint_delivery();
    goto_priv(PRIV_HS);
    TEST_SETUP_EXCEPT();
    ai_arch_execute_trigger_prefetch_w_once(vaddr);
    TEST_ASSERT("after clearing the execute-PC slot, the remaining store-address trigger does not fire on the prefetch.w hint operand in the adjacent-preservation corner",
        excpt.triggered == false
    );
    ai_arch_restore_machine_status(saved_mstatus);

    ai_arch_clear_mcontrol6_slot(1);
    ai_arch_sync_trigger_state();
    goto_priv(PRIV_HS);
    TEST_SETUP_EXCEPT();
    ai_arch_execute_trigger_prefetch_w_once(vaddr);
    TEST_ASSERT("after clearing both repeated prefetch.w trigger slots, the same hint returns to normal silent behavior before write permission is restored in the adjacent-preservation corner",
        excpt.triggered == false
    );

    goto_priv(PRIV_M);
    TEST_ASSERT("execute-triggered prefetch.w hits plus an address-trigger-silent hint still leave the target and adjacent backing data unchanged before write permission is restored in the adjacent-preservation corner",
        read64(paddr) == seed &&
        read64(adjacent_paddr) == adjacent
    );
    ai_arch_trigger_set_hs_leaf(VSRX_GURWX, phys_page_base(VSRX_GURWX), PTE_V | PTE_AD | PTE_RWX);
    sfence_vma();

    goto_priv(PRIV_HS);
    TEST_SETUP_EXCEPT();
    sd(vaddr, stored);
    TEST_ASSERT("after execute-triggered prefetch.w hits followed by an address-trigger-silent hint, restoring write permission and flushing translation state lets the real store recover normally in the adjacent-preservation corner",
        excpt.triggered == false
    );

    TEST_SETUP_EXCEPT();
    uint64_t observed = ld(vaddr);
    TEST_ASSERT("after the recovered dual-slot prefetch.w path, the same HS address reads back only the architecturally committed store payload in the adjacent-preservation corner",
        excpt.triggered == false &&
        observed == stored
    );

    TEST_SETUP_EXCEPT();
    uint64_t observed_adjacent = ld(adjacent_vaddr);
    TEST_ASSERT("after the recovered dual-slot prefetch.w path, the adjacent HS word also remains unchanged in the adjacent-preservation corner",
        excpt.triggered == false &&
        observed_adjacent == adjacent
    );

    goto_priv(PRIV_M);
    TEST_ASSERT("the recovered dual-slot prefetch.w permission-repair corner updates only the target backing word while leaving the adjacent word untouched",
        read64(paddr) == stored &&
        read64(adjacent_paddr) == adjacent
    );

    ai_arch_clear_mcontrol6_slot(0);
    ai_arch_clear_mcontrol6_slot(1);

    goto_priv(PRIV_HS);
    hspt_init();
    sfence_vma();

    TEST_END();
}

bool ai_arch_prefetch_i_execute_trigger_repeated_breakpoint_corner()
{
    TEST_START();

    uintptr_t vaddr = hs_page_base(VSRWX_GRWX);
    uintptr_t paddr = phys_page_base(VSRWX_GRWX);
    uintptr_t adjacent_paddr = paddr + sizeof(uint32_t);
    uintptr_t trigger_pc = (uintptr_t)ai_arch_execute_trigger_prefetch_i_once;
    const uint32_t instruction = 0x00008067U;
    const uint32_t adjacent = 0x00008067U;

    goto_priv(PRIV_M);
    write32(paddr, instruction);
    write32(adjacent_paddr, adjacent);

    goto_priv(PRIV_HS);
    hspt_init();
    sfence_vma();

    ai_arch_arm_mcontrol6_0(trigger_pc, AI_ARCH_MCONTROL6_EXECUTE);
    uint64_t saved_mstatus = ai_arch_enable_machine_breakpoint_delivery();

    TEST_SETUP_EXCEPT();
    ai_arch_execute_trigger_prefetch_i_once(vaddr);
    TEST_ASSERT("the first prefetch.i execute-PC trigger hit reports breakpoint before the hint retires",
        excpt.triggered == true &&
        excpt.cause == CAUSE_BKP
    );
    ai_arch_reenable_breakpoint_delivery();

    TEST_SETUP_EXCEPT();
    ai_arch_execute_trigger_prefetch_i_once(vaddr);
    TEST_ASSERT("the second prefetch.i execute-PC trigger hit still reports breakpoint and still leaves both backing instruction words unchanged",
        excpt.triggered == true &&
        excpt.cause == CAUSE_BKP
    );
    ai_arch_reenable_breakpoint_delivery();
    ai_arch_restore_machine_status(saved_mstatus);
    ai_arch_clear_mcontrol6_0();

    goto_priv(PRIV_HS);
    TEST_SETUP_EXCEPT();
    ai_arch_execute_trigger_prefetch_i_once(vaddr);
    TEST_ASSERT("after clearing the repeated prefetch.i execute-PC trigger, the same hint retires silently again",
        excpt.triggered == false
    );

    TEST_SETUP_EXCEPT();
    ((void(*)(void))vaddr)();
    TEST_ASSERT("after repeated prefetch.i execute-PC trigger hits, a later real HS fetch to the same backing instruction still executes successfully",
        excpt.triggered == false
    );

    goto_priv(PRIV_M);
    TEST_ASSERT("repeated prefetch.i execute-PC trigger hits preserve both the target and adjacent backing instruction words",
        read32(paddr) == instruction &&
        read32(adjacent_paddr) == adjacent
    );

    TEST_END();
}

bool ai_arch_prefetch_i_execute_trigger_repeated_breakpoint_then_no_x_fetch_page_fault_corner()
{
    TEST_START();

    uintptr_t vaddr = hs_page_base(VSRW_GURWX);
    uintptr_t paddr = phys_page_base(VSRW_GURWX);
    uintptr_t trigger_pc = (uintptr_t)ai_arch_execute_trigger_prefetch_i_once;
    const uint32_t instruction = 0x00008067U;

    goto_priv(PRIV_M);
    write32(paddr, instruction);

    goto_priv(PRIV_HS);
    hspt_init();
    sfence_vma();

    ai_arch_arm_mcontrol6_0(trigger_pc, AI_ARCH_MCONTROL6_EXECUTE);
    uint64_t saved_mstatus = ai_arch_enable_machine_breakpoint_delivery();

    TEST_SETUP_EXCEPT();
    ai_arch_execute_trigger_prefetch_i_once(vaddr);
    TEST_ASSERT("the first prefetch.i execute-PC trigger hit on a no-X HS page still reports breakpoint before any later fetch fault is exposed",
        excpt.triggered == true &&
        excpt.cause == CAUSE_BKP
    );
    ai_arch_reenable_breakpoint_delivery();

    TEST_SETUP_EXCEPT();
    ai_arch_execute_trigger_prefetch_i_once(vaddr);
    TEST_ASSERT("the second prefetch.i execute-PC trigger hit on the same no-X HS page still reports breakpoint before any later fetch fault is exposed",
        excpt.triggered == true &&
        excpt.cause == CAUSE_BKP
    );
    ai_arch_reenable_breakpoint_delivery();
    ai_arch_restore_machine_status(saved_mstatus);
    ai_arch_clear_mcontrol6_0();

    goto_priv(PRIV_HS);
    TEST_SETUP_EXCEPT();
    ai_arch_execute_trigger_prefetch_i_once(vaddr);
    TEST_ASSERT("after clearing the repeated execute-PC trigger, the same prefetch.i to the no-X HS page falls back to the normal silent-hint behavior",
        excpt.triggered == false
    );

    TEST_SETUP_EXCEPT();
    TEST_EXEC_EXCEPT(vaddr);
    TEST_ASSERT("after repeated triggered prefetch.i probes, a real fetch to the same no-X HS page still reports instruction page fault",
        excpt.triggered == true &&
        excpt.cause == CAUSE_IPF &&
        excpt.tval == vaddr
    );

    goto_priv(PRIV_M);
    TEST_ASSERT("repeated prefetch.i execute-trigger hits on a no-X HS page still leave the backing instruction word unchanged",
        read32(paddr) == instruction
    );

    TEST_END();
}

bool ai_arch_prefetch_i_execute_trigger_repeated_breakpoint_then_grant_x_executes_corner()
{
    TEST_START();

    uintptr_t vaddr = hs_page_base(VSRW_GURWX);
    uintptr_t paddr = phys_page_base(VSRW_GURWX);
    uintptr_t trigger_pc = (uintptr_t)ai_arch_execute_trigger_prefetch_i_once;
    const uint32_t instruction = 0x00008067U;

    goto_priv(PRIV_M);
    write32(paddr, instruction);

    goto_priv(PRIV_HS);
    hspt_init();
    sfence_vma();

    ai_arch_arm_mcontrol6_0(trigger_pc, AI_ARCH_MCONTROL6_EXECUTE);
    uint64_t saved_mstatus = ai_arch_enable_machine_breakpoint_delivery();

    TEST_SETUP_EXCEPT();
    ai_arch_execute_trigger_prefetch_i_once(vaddr);
    TEST_ASSERT("the first prefetch.i execute-PC trigger hit on a no-X HS page reports breakpoint before any later permission repair",
        excpt.triggered == true &&
        excpt.cause == CAUSE_BKP
    );
    ai_arch_reenable_breakpoint_delivery();

    TEST_SETUP_EXCEPT();
    ai_arch_execute_trigger_prefetch_i_once(vaddr);
    TEST_ASSERT("the second prefetch.i execute-PC trigger hit on the same no-X HS page still reports breakpoint before any later permission repair",
        excpt.triggered == true &&
        excpt.cause == CAUSE_BKP
    );
    ai_arch_reenable_breakpoint_delivery();
    ai_arch_restore_machine_status(saved_mstatus);
    ai_arch_clear_mcontrol6_0();

    goto_priv(PRIV_HS);
    TEST_SETUP_EXCEPT();
    ai_arch_execute_trigger_prefetch_i_once(vaddr);
    TEST_ASSERT("after clearing the repeated execute-PC trigger, the same prefetch.i returns to normal silent-hint behavior before execute permission is restored",
        excpt.triggered == false
    );

    goto_priv(PRIV_M);
    ai_arch_trigger_set_hs_leaf(VSRW_GURWX, paddr, PTE_V | PTE_AD | PTE_RWX);
    sfence_vma();

    goto_priv(PRIV_HS);
    TEST_SETUP_EXCEPT();
    ((void(*)(void))vaddr)();
    TEST_ASSERT("after repeated prefetch.i execute-PC trigger hits, restoring execute permission and flushing translation state lets the real fetch recover normally",
        excpt.triggered == false
    );

    goto_priv(PRIV_M);
    TEST_ASSERT("the recovered prefetch.i execute-trigger plus permission-repair corner leaves the backing instruction word unchanged",
        read32(paddr) == instruction
    );

    goto_priv(PRIV_HS);
    hspt_init();
    sfence_vma();

    TEST_END();
}

bool ai_arch_prefetch_i_execute_trigger_repeated_breakpoint_then_no_x_fetch_page_fault_adjacent_preserved_corner()
{
    TEST_START();

    uintptr_t vaddr = hs_page_base(VSRW_GURWX);
    uintptr_t paddr = phys_page_base(VSRW_GURWX);
    uintptr_t adjacent_paddr = paddr + sizeof(uint32_t);
    uintptr_t trigger_pc = (uintptr_t)ai_arch_execute_trigger_prefetch_i_once;
    const uint32_t instruction = 0x00008067U;
    const uint32_t adjacent = 0x00000013U;

    goto_priv(PRIV_M);
    write32(paddr, instruction);
    write32(adjacent_paddr, adjacent);

    goto_priv(PRIV_HS);
    hspt_init();
    sfence_vma();

    ai_arch_arm_mcontrol6_0(trigger_pc, AI_ARCH_MCONTROL6_EXECUTE);
    uint64_t saved_mstatus = ai_arch_enable_machine_breakpoint_delivery();

    TEST_SETUP_EXCEPT();
    ai_arch_execute_trigger_prefetch_i_once(vaddr);
    TEST_ASSERT("the first prefetch.i execute-PC trigger hit on a no-X HS page still reports breakpoint before any later fetch fault is exposed in the adjacent-preservation corner",
        excpt.triggered == true &&
        excpt.cause == CAUSE_BKP
    );
    ai_arch_reenable_breakpoint_delivery();

    TEST_SETUP_EXCEPT();
    ai_arch_execute_trigger_prefetch_i_once(vaddr);
    TEST_ASSERT("the second prefetch.i execute-PC trigger hit on the same no-X HS page still reports breakpoint before any later fetch fault is exposed in the adjacent-preservation corner",
        excpt.triggered == true &&
        excpt.cause == CAUSE_BKP
    );
    ai_arch_reenable_breakpoint_delivery();
    ai_arch_restore_machine_status(saved_mstatus);
    ai_arch_clear_mcontrol6_0();

    goto_priv(PRIV_HS);
    TEST_SETUP_EXCEPT();
    ai_arch_execute_trigger_prefetch_i_once(vaddr);
    TEST_ASSERT("after clearing the repeated execute-PC trigger, the same prefetch.i to the no-X HS page falls back to the normal silent-hint behavior in the adjacent-preservation corner",
        excpt.triggered == false
    );

    TEST_SETUP_EXCEPT();
    TEST_EXEC_EXCEPT(vaddr);
    TEST_ASSERT("after repeated triggered prefetch.i probes, a real fetch to the same no-X HS page still reports instruction page fault in the adjacent-preservation corner",
        excpt.triggered == true &&
        excpt.cause == CAUSE_IPF &&
        excpt.tval == vaddr
    );

    goto_priv(PRIV_M);
    TEST_ASSERT("repeated prefetch.i execute-trigger hits on a no-X HS page still leave both the target and adjacent backing instruction words unchanged in the adjacent-preservation corner",
        read32(paddr) == instruction &&
        read32(adjacent_paddr) == adjacent
    );

    TEST_END();
}

bool ai_arch_prefetch_i_execute_trigger_repeated_breakpoint_then_grant_x_executes_adjacent_preserved_corner()
{
    TEST_START();

    uintptr_t vaddr = hs_page_base(VSRW_GURWX);
    uintptr_t paddr = phys_page_base(VSRW_GURWX);
    uintptr_t adjacent_paddr = paddr + sizeof(uint32_t);
    uintptr_t trigger_pc = (uintptr_t)ai_arch_execute_trigger_prefetch_i_once;
    const uint32_t instruction = 0x00008067U;
    const uint32_t adjacent = 0x00000013U;

    goto_priv(PRIV_M);
    write32(paddr, instruction);
    write32(adjacent_paddr, adjacent);

    goto_priv(PRIV_HS);
    hspt_init();
    sfence_vma();

    ai_arch_arm_mcontrol6_0(trigger_pc, AI_ARCH_MCONTROL6_EXECUTE);
    uint64_t saved_mstatus = ai_arch_enable_machine_breakpoint_delivery();

    TEST_SETUP_EXCEPT();
    ai_arch_execute_trigger_prefetch_i_once(vaddr);
    TEST_ASSERT("the first prefetch.i execute-PC trigger hit on a no-X HS page reports breakpoint before any later permission repair in the adjacent-preservation corner",
        excpt.triggered == true &&
        excpt.cause == CAUSE_BKP
    );
    ai_arch_reenable_breakpoint_delivery();

    TEST_SETUP_EXCEPT();
    ai_arch_execute_trigger_prefetch_i_once(vaddr);
    TEST_ASSERT("the second prefetch.i execute-PC trigger hit on the same no-X HS page still reports breakpoint before any later permission repair in the adjacent-preservation corner",
        excpt.triggered == true &&
        excpt.cause == CAUSE_BKP
    );
    ai_arch_reenable_breakpoint_delivery();
    ai_arch_restore_machine_status(saved_mstatus);
    ai_arch_clear_mcontrol6_0();

    goto_priv(PRIV_HS);
    TEST_SETUP_EXCEPT();
    ai_arch_execute_trigger_prefetch_i_once(vaddr);
    TEST_ASSERT("after clearing the repeated execute-PC trigger, the same prefetch.i returns to normal silent-hint behavior before execute permission is restored in the adjacent-preservation corner",
        excpt.triggered == false
    );

    goto_priv(PRIV_M);
    TEST_ASSERT("repeated prefetch.i execute-trigger hits still leave both the target and adjacent backing instruction words unchanged before execute permission is restored in the adjacent-preservation corner",
        read32(paddr) == instruction &&
        read32(adjacent_paddr) == adjacent
    );
    ai_arch_trigger_set_hs_leaf(VSRW_GURWX, paddr, PTE_V | PTE_AD | PTE_RWX);
    sfence_vma();

    goto_priv(PRIV_HS);
    TEST_SETUP_EXCEPT();
    ((void(*)(void))vaddr)();
    TEST_ASSERT("after repeated prefetch.i execute-PC trigger hits, restoring execute permission and flushing translation state lets the real fetch recover normally in the adjacent-preservation corner",
        excpt.triggered == false
    );

    goto_priv(PRIV_M);
    TEST_ASSERT("after the recovered prefetch.i execute-trigger plus permission-repair corner, both the target and adjacent backing instruction words remain untouched",
        read32(paddr) == instruction &&
        read32(adjacent_paddr) == adjacent
    );

    goto_priv(PRIV_HS);
    hspt_init();
    sfence_vma();

    TEST_END();
}
