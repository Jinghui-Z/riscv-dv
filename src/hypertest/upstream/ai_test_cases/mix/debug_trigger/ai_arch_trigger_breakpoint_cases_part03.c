#include "ai_arch_trigger_breakpoint_common.h"

bool ai_arch_load_execute_and_address_trigger_misaligned_page_fault_repeated_breakpoint_then_fault_adjacent_preserved_corner()
{
    TEST_START();

    enum test_page low_page = VSRWXPbmt1_GURWXPbmt0;
    enum test_page high_page = VSRWXPbmt1_GURWXPbmt1;
    uintptr_t low_adjacent_phys = phys_page_base(low_page) + PAGE_SIZE - (2 * sizeof(uint64_t));
    uintptr_t low_phys = ai_arch_trigger_page_end_phys(low_page);
    uintptr_t high_phys = phys_page_base(high_page);
    uintptr_t high_adjacent_phys = high_phys + sizeof(uint64_t);
    const uint64_t low_adjacent = 0x55aa55aa33cc33ccULL;
    const uint64_t low_pattern = 0x768798a9bacbdcedULL;
    const uint64_t high_pattern = 0xfe1032547698badcULL;
    const uint64_t high_adjacent = 0xcc33cc3355aa55aaULL;
    uintptr_t trigger_pc = (uintptr_t)ai_arch_execute_trigger_load_once;
    uintptr_t vaddr = ai_arch_trigger_prepare_cross_page_pbmt_nc_load_pf_alias();
    bool first_preserved;
    bool second_preserved;
    bool third_preserved;

    goto_priv(PRIV_M);
    write64(low_adjacent_phys, low_adjacent);
    write64(low_phys, low_pattern);
    write64(high_phys, high_pattern);
    write64(high_adjacent_phys, high_adjacent);

    ai_arch_clear_mcontrol6_slot(0);
    ai_arch_clear_mcontrol6_slot(1);
    ai_arch_arm_mcontrol6_slot(0, trigger_pc, AI_ARCH_MCONTROL6_EXECUTE);
    ai_arch_arm_mcontrol6_slot(1, vaddr, AI_ARCH_MCONTROL6_LOAD);

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
    TEST_ASSERT("with both load trigger slots armed on the HS cross-page PBMT-NC misalign-plus-page-fault path, the first access reports breakpoint and leaves both boundary doublewords plus their adjacent neighbors unchanged in the adjacent-preservation corner",
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
    TEST_ASSERT("with both load trigger slots still armed on that HS cross-page PBMT-NC misalign-plus-page-fault path, the second access still reports breakpoint and still leaves both boundary doublewords plus their adjacent neighbors unchanged in the adjacent-preservation corner",
        second_excpt.triggered == true &&
        second_excpt.cause == CAUSE_BKP &&
        second_preserved
    );
    ai_arch_restore_machine_status(saved_mstatus);

    ai_arch_clear_mcontrol6_slot(0);
    saved_mstatus = ai_arch_enable_machine_breakpoint_delivery();
    goto_priv(PRIV_HS);
    TEST_SETUP_EXCEPT();
    ai_arch_execute_trigger_load_once(vaddr);
    ai_arch_trigger_excpt_snapshot_t third_excpt = ai_arch_snapshot_excpt();
    goto_priv(PRIV_M);
    third_preserved =
        (read64(low_adjacent_phys) == low_adjacent &&
        read64(low_phys) == low_pattern &&
        read64(high_phys) == high_pattern &&
        read64(high_adjacent_phys) == high_adjacent);
    goto_priv(PRIV_HS);
    TEST_ASSERT("after repeated dual-slot load breakpoint hits on the HS cross-page PBMT-NC misalign-plus-page-fault path, clearing only the execute-PC slot still leaves the address trigger active without dirtying either boundary doubleword or adjacent neighbor in the adjacent-preservation corner",
        third_excpt.triggered == true &&
        third_excpt.cause == CAUSE_BKP &&
        third_preserved
    );
    ai_arch_restore_machine_status(saved_mstatus);

    ai_arch_clear_mcontrol6_slot(1);
    goto_priv(PRIV_HS);
    TEST_SETUP_EXCEPT();
    ai_arch_execute_trigger_load_once(vaddr);
    TEST_ASSERT("after clearing both repeated load trigger slots, the same HS cross-page access falls back to the project-RTL low-half load misalign with the low-half start VA in tval in the adjacent-preservation corner",
        excpt.triggered == true &&
        excpt.cause == CAUSE_LAM &&
        excpt.tval == vaddr
    );

    goto_priv(PRIV_M);
    TEST_ASSERT("the repeated dual-slot load trigger+misalign+page-fault corner still leaves both boundary backing doublewords and their adjacent neighbors unchanged after the fallback misalign fault in the adjacent-preservation corner",
        read64(low_adjacent_phys) == low_adjacent &&
        read64(low_phys) == low_pattern &&
        read64(high_phys) == high_pattern &&
        read64(high_adjacent_phys) == high_adjacent
    );
    ai_arch_trigger_reset_pbmt_only_env();

    TEST_END();
}

bool ai_arch_load_execute_and_address_trigger_misaligned_page_fault_repeated_breakpoint_then_fixed_cross_page_load_recovers_corner()
{
    TEST_START();

    enum test_page low_page = VSRWXPbmt1_GURWXPbmt0;
    enum test_page high_page = VSRWXPbmt1_GURWXPbmt1;
    uintptr_t low_phys = ai_arch_trigger_page_end_phys(low_page);
    uintptr_t high_phys = phys_page_base(high_page);
    uintptr_t trigger_pc = (uintptr_t)ai_arch_execute_trigger_load_once;
    uintptr_t vaddr = ai_arch_trigger_prepare_cross_page_pbmt_nc_load_pf_alias();
    const uint64_t low_pattern = 0x8798a9bacbdcedfeULL;
    const uint64_t high_pattern = 0x1032547698badcfeULL;
    const uint64_t recovery_expected =
        ((high_pattern & 0xffffffffULL) << 32) | (low_pattern >> 32);
    bool first_preserved;
    bool second_preserved;
    bool third_preserved;

    goto_priv(PRIV_M);
    write64(low_phys, low_pattern);
    write64(high_phys, high_pattern);

    ai_arch_clear_mcontrol6_slot(0);
    ai_arch_clear_mcontrol6_slot(1);
    ai_arch_arm_mcontrol6_slot(0, trigger_pc, AI_ARCH_MCONTROL6_EXECUTE);
    ai_arch_arm_mcontrol6_slot(1, vaddr, AI_ARCH_MCONTROL6_LOAD);

    uint64_t saved_mstatus = ai_arch_enable_machine_breakpoint_delivery();
    goto_priv(PRIV_HS);
    TEST_SETUP_EXCEPT();
    ai_arch_execute_trigger_load_once(vaddr);
    ai_arch_trigger_excpt_snapshot_t first_excpt = ai_arch_snapshot_excpt();
    goto_priv(PRIV_M);
    first_preserved = (read64(low_phys) == low_pattern && read64(high_phys) == high_pattern);
    goto_priv(PRIV_HS);
    TEST_ASSERT("with both load trigger slots armed on the HS cross-page PBMT-NC misalign-plus-page-fault path, the first access reports breakpoint and leaves both boundary doublewords unchanged",
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
    TEST_ASSERT("with both load trigger slots still armed on that HS cross-page PBMT-NC misalign-plus-page-fault path, the second access still reports breakpoint and still leaves both boundary doublewords unchanged",
        second_excpt.triggered == true &&
        second_excpt.cause == CAUSE_BKP &&
        second_preserved
    );
    ai_arch_restore_machine_status(saved_mstatus);

    ai_arch_clear_mcontrol6_slot(0);
    saved_mstatus = ai_arch_enable_machine_breakpoint_delivery();
    goto_priv(PRIV_HS);
    TEST_SETUP_EXCEPT();
    ai_arch_execute_trigger_load_once(vaddr);
    ai_arch_trigger_excpt_snapshot_t third_excpt = ai_arch_snapshot_excpt();
    goto_priv(PRIV_M);
    third_preserved = (read64(low_phys) == low_pattern && read64(high_phys) == high_pattern);
    goto_priv(PRIV_HS);
    TEST_ASSERT("after repeated dual-slot load breakpoint hits on the HS cross-page PBMT-NC misalign-plus-page-fault path, clearing only the execute-PC slot still leaves the address trigger active",
        third_excpt.triggered == true &&
        third_excpt.cause == CAUSE_BKP &&
        third_preserved
    );
    ai_arch_restore_machine_status(saved_mstatus);

    ai_arch_clear_mcontrol6_slot(1);
    goto_priv(PRIV_HS);
    TEST_SETUP_EXCEPT();
    ai_arch_execute_trigger_load_once(vaddr);
    TEST_ASSERT("after clearing both repeated load trigger slots, the same HS cross-page access falls back to the project-RTL low-half load misalign with the low-half start VA in tval",
        excpt.triggered == true &&
        excpt.cause == CAUSE_LAM &&
        excpt.tval == vaddr
    );

    goto_priv(PRIV_M);
    TEST_ASSERT("the repeated dual-slot load trigger+misalign+page-fault corner still leaves both boundary backing doublewords unchanged after the fallback misalign fault",
        read64(low_phys) == low_pattern &&
        read64(high_phys) == high_pattern
    );

    ai_arch_trigger_reset_pbmt_only_env();
    ai_arch_trigger_set_hs_leaf(low_page, phys_page_base(low_page), PTE_V | PTE_AD | PTE_RWX);
    ai_arch_trigger_set_hs_leaf(high_page, phys_page_base(high_page), PTE_V | PTE_AD | PTE_RWX);
    write64(low_phys, low_pattern);
    write64(high_phys, high_pattern);
    sfence_vma();

    goto_priv(PRIV_HS);
    TEST_SETUP_EXCEPT();
    uint64_t recovery_value = ai_arch_execute_trigger_load_once(vaddr);
    TEST_ASSERT("after repeated dual-slot trigger hits on the cross-page misalign-plus-page-fault path, fixing both leaves lets the same split load recover and return the merged payload",
        excpt.triggered == false &&
        recovery_value == recovery_expected
    );

    TEST_END();
}

bool ai_arch_load_execute_and_address_trigger_misaligned_page_fault_repeated_breakpoint_then_fixed_cross_page_load_adjacent_preserved_corner()
{
    TEST_START();

    enum test_page low_page = VSRWXPbmt1_GURWXPbmt0;
    enum test_page high_page = VSRWXPbmt1_GURWXPbmt1;
    uintptr_t low_adjacent_phys = phys_page_base(low_page) + PAGE_SIZE - (2 * sizeof(uint64_t));
    uintptr_t low_phys = ai_arch_trigger_page_end_phys(low_page);
    uintptr_t high_phys = phys_page_base(high_page);
    uintptr_t high_adjacent_phys = high_phys + sizeof(uint64_t);
    uintptr_t trigger_pc = (uintptr_t)ai_arch_execute_trigger_load_once;
    uintptr_t vaddr = ai_arch_trigger_prepare_cross_page_pbmt_nc_load_pf_alias();
    const uint64_t low_adjacent = 0x55aa55aa33cc33ccULL;
    const uint64_t low_pattern = 0x8798a9bacbdcedfeULL;
    const uint64_t high_pattern = 0x1032547698badcfeULL;
    const uint64_t high_adjacent = 0xcc33cc3355aa55aaULL;
    const uint64_t recovery_expected =
        ((high_pattern & 0xffffffffULL) << 32) | (low_pattern >> 32);
    bool first_preserved;
    bool second_preserved;
    bool third_preserved;

    goto_priv(PRIV_M);
    write64(low_adjacent_phys, low_adjacent);
    write64(low_phys, low_pattern);
    write64(high_phys, high_pattern);
    write64(high_adjacent_phys, high_adjacent);

    ai_arch_clear_mcontrol6_slot(0);
    ai_arch_clear_mcontrol6_slot(1);
    ai_arch_arm_mcontrol6_slot(0, trigger_pc, AI_ARCH_MCONTROL6_EXECUTE);
    ai_arch_arm_mcontrol6_slot(1, vaddr, AI_ARCH_MCONTROL6_LOAD);

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
    TEST_ASSERT("with both load trigger slots armed on the HS cross-page PBMT-NC misalign-plus-page-fault path, the first access reports breakpoint and leaves both boundary and adjacent doublewords unchanged in the repaired-cross-page adjacent-preservation corner",
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
    TEST_ASSERT("with both load trigger slots still armed on that HS cross-page PBMT-NC misalign-plus-page-fault path, the second access still reports breakpoint and still leaves both boundary and adjacent doublewords unchanged in the repaired-cross-page adjacent-preservation corner",
        second_excpt.triggered == true &&
        second_excpt.cause == CAUSE_BKP &&
        second_preserved
    );
    ai_arch_restore_machine_status(saved_mstatus);

    ai_arch_clear_mcontrol6_slot(0);
    saved_mstatus = ai_arch_enable_machine_breakpoint_delivery();
    goto_priv(PRIV_HS);
    TEST_SETUP_EXCEPT();
    ai_arch_execute_trigger_load_once(vaddr);
    ai_arch_trigger_excpt_snapshot_t third_excpt = ai_arch_snapshot_excpt();
    goto_priv(PRIV_M);
    third_preserved =
        (read64(low_adjacent_phys) == low_adjacent &&
        read64(low_phys) == low_pattern &&
        read64(high_phys) == high_pattern &&
        read64(high_adjacent_phys) == high_adjacent);
    goto_priv(PRIV_HS);
    TEST_ASSERT("after repeated dual-slot load breakpoint hits on the HS cross-page PBMT-NC misalign-plus-page-fault path, clearing only the execute-PC slot still leaves the address trigger active without dirtying either adjacent boundary word in the repaired-cross-page adjacent-preservation corner",
        third_excpt.triggered == true &&
        third_excpt.cause == CAUSE_BKP &&
        third_preserved
    );
    ai_arch_restore_machine_status(saved_mstatus);

    ai_arch_clear_mcontrol6_slot(1);
    goto_priv(PRIV_HS);
    TEST_SETUP_EXCEPT();
    ai_arch_execute_trigger_load_once(vaddr);
    TEST_ASSERT("after clearing both repeated load trigger slots, the same HS cross-page access falls back to the project-RTL low-half load misalign with the low-half start VA in tval in the repaired-cross-page adjacent-preservation corner",
        excpt.triggered == true &&
        excpt.cause == CAUSE_LAM &&
        excpt.tval == vaddr
    );

    goto_priv(PRIV_M);
    TEST_ASSERT("the repeated dual-slot load trigger+misalign+page-fault corner still leaves both boundary backing doublewords and their adjacent neighbors unchanged after the fallback misalign fault in the repaired-cross-page adjacent-preservation corner",
        read64(low_adjacent_phys) == low_adjacent &&
        read64(low_phys) == low_pattern &&
        read64(high_phys) == high_pattern &&
        read64(high_adjacent_phys) == high_adjacent
    );

    ai_arch_trigger_reset_pbmt_only_env();
    ai_arch_trigger_set_hs_leaf(low_page, phys_page_base(low_page), PTE_V | PTE_AD | PTE_RWX);
    ai_arch_trigger_set_hs_leaf(high_page, phys_page_base(high_page), PTE_V | PTE_AD | PTE_RWX);
    write64(low_adjacent_phys, low_adjacent);
    write64(low_phys, low_pattern);
    write64(high_phys, high_pattern);
    write64(high_adjacent_phys, high_adjacent);
    sfence_vma();

    goto_priv(PRIV_HS);
    TEST_SETUP_EXCEPT();
    uint64_t recovery_value = ai_arch_execute_trigger_load_once(vaddr);
    TEST_ASSERT("after repeated dual-slot trigger hits on the cross-page misalign-plus-page-fault path, fixing both leaves lets the same split load recover and return the merged payload while keeping both adjacent boundary words untouched",
        excpt.triggered == false &&
        recovery_value == recovery_expected
    );

    goto_priv(PRIV_M);
    TEST_ASSERT("the recovered split-load path reads the merged payload without dirtying either adjacent boundary word on the repaired cross-page trigger corner",
        read64(low_adjacent_phys) == low_adjacent &&
        read64(low_phys) == low_pattern &&
        read64(high_phys) == high_pattern &&
        read64(high_adjacent_phys) == high_adjacent
    );

    TEST_END();
}

bool ai_arch_load_execute_and_address_trigger_high_half_page_fault_repeated_breakpoint_then_fix_high_half_load_recovers_corner()
{
    TEST_START();

    enum test_page low_page = VSR_GX;
    enum test_page high_page = VSW_GI;
    uintptr_t low_phys = ai_arch_trigger_page_end_phys(low_page);
    uintptr_t high_phys = phys_page_base(high_page);
    uintptr_t trigger_pc = (uintptr_t)ai_arch_execute_trigger_load_once;
    uintptr_t vaddr = ai_arch_trigger_prepare_cross_page_high_half_load_pf_alias();
    uintptr_t second_half_vaddr = vaddr + 4U;
    const uint64_t low_pattern = 0x768798a9bacbdcedULL;
    const uint64_t high_pattern = 0xfe1032547698badcULL;
    const uint64_t recovery_expected =
        ((high_pattern & 0xffffffffULL) << 32) | (low_pattern >> 32);
    bool first_preserved;
    bool second_preserved;
    bool third_preserved;

    goto_priv(PRIV_M);
    write64(low_phys, low_pattern);
    write64(high_phys, high_pattern);

    ai_arch_clear_mcontrol6_slot(0);
    ai_arch_clear_mcontrol6_slot(1);
    ai_arch_arm_mcontrol6_slot(0, trigger_pc, AI_ARCH_MCONTROL6_EXECUTE);
    ai_arch_arm_mcontrol6_slot(1, vaddr, AI_ARCH_MCONTROL6_LOAD);

    uint64_t saved_mstatus = ai_arch_enable_machine_breakpoint_delivery();
    goto_priv(PRIV_HS);
    TEST_SETUP_EXCEPT();
    ai_arch_execute_trigger_load_once(vaddr);
    ai_arch_trigger_excpt_snapshot_t first_excpt = ai_arch_snapshot_excpt();
    goto_priv(PRIV_M);
    first_preserved = (read64(low_phys) == low_pattern && read64(high_phys) == high_pattern);
    goto_priv(PRIV_HS);
    TEST_ASSERT("with both load trigger slots armed on a cross-page access whose low half is cacheable but whose high half would page-fault, the first access reports breakpoint and leaves both boundary doublewords unchanged",
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
    TEST_ASSERT("with both load trigger slots still armed on that cross-page high-half page-fault path, the second access still reports breakpoint and still leaves both boundary doublewords unchanged",
        second_excpt.triggered == true &&
        second_excpt.cause == CAUSE_BKP &&
        second_preserved
    );
    ai_arch_restore_machine_status(saved_mstatus);

    ai_arch_clear_mcontrol6_slot(0);
    saved_mstatus = ai_arch_enable_machine_breakpoint_delivery();
    goto_priv(PRIV_HS);
    TEST_SETUP_EXCEPT();
    ai_arch_execute_trigger_load_once(vaddr);
    ai_arch_trigger_excpt_snapshot_t third_excpt = ai_arch_snapshot_excpt();
    goto_priv(PRIV_M);
    third_preserved = (read64(low_phys) == low_pattern && read64(high_phys) == high_pattern);
    goto_priv(PRIV_HS);
    TEST_ASSERT("after repeated dual-slot load breakpoint hits on the cross-page high-half page-fault path, clearing only the execute-PC slot still leaves the address trigger active",
        third_excpt.triggered == true &&
        third_excpt.cause == CAUSE_BKP &&
        third_preserved
    );
    ai_arch_restore_machine_status(saved_mstatus);

    ai_arch_clear_mcontrol6_slot(1);
    goto_priv(PRIV_HS);
    TEST_SETUP_EXCEPT();
    ai_arch_execute_trigger_load_once(vaddr);
    TEST_ASSERT("after clearing both repeated load trigger slots, the same split access falls back to the true second-half load page fault with the second-half start VA in tval",
        excpt.triggered == true &&
        excpt.cause == CAUSE_LPF &&
        excpt.tval == second_half_vaddr
    );

    goto_priv(PRIV_M);
    TEST_ASSERT("before the upper leaf is repaired, repeated trigger hits and the fallback second-half load page fault still leave both boundary backing doublewords unchanged",
        read64(low_phys) == low_pattern &&
        read64(high_phys) == high_pattern
    );
    ai_arch_trigger_set_hs_leaf(high_page, phys_page_base(high_page), PTE_V | PTE_AD | PTE_RWX);
    sfence_vma();

    goto_priv(PRIV_HS);
    TEST_SETUP_EXCEPT();
    uint64_t recovery_value = ai_arch_execute_trigger_load_once(vaddr);
    TEST_ASSERT("after repeated dual-slot load trigger hits on the cross-page high-half page-fault path, fixing the upper leaf lets the same split load recover and return the merged payload",
        excpt.triggered == false &&
        recovery_value == recovery_expected
    );

    ai_arch_clear_mcontrol6_slot(0);
    ai_arch_clear_mcontrol6_slot(1);

    goto_priv(PRIV_HS);
    hspt_init();
    sfence_vma();

    TEST_END();
}

bool ai_arch_load_execute_and_address_trigger_high_half_page_fault_repeated_breakpoint_then_fix_high_half_load_adjacent_preserved_corner()
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
    const uint64_t low_adjacent = 0x55aa55aa33cc33ccULL;
    const uint64_t low_pattern = 0x768798a9bacbdcedULL;
    const uint64_t high_pattern = 0xfe1032547698badcULL;
    const uint64_t high_adjacent = 0xcc33cc3355aa55aaULL;
    const uint64_t recovery_expected =
        ((high_pattern & 0xffffffffULL) << 32) | (low_pattern >> 32);
    bool first_preserved;
    bool second_preserved;
    bool third_preserved;

    goto_priv(PRIV_M);
    write64(low_adjacent_phys, low_adjacent);
    write64(low_phys, low_pattern);
    write64(high_phys, high_pattern);
    write64(high_adjacent_phys, high_adjacent);

    ai_arch_clear_mcontrol6_slot(0);
    ai_arch_clear_mcontrol6_slot(1);
    ai_arch_arm_mcontrol6_slot(0, trigger_pc, AI_ARCH_MCONTROL6_EXECUTE);
    ai_arch_arm_mcontrol6_slot(1, vaddr, AI_ARCH_MCONTROL6_LOAD);

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
    TEST_ASSERT("with both load trigger slots armed on a cross-page access whose low half is cacheable but whose high half would page-fault, the first access reports breakpoint and leaves both boundary and adjacent doublewords unchanged",
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
    TEST_ASSERT("with both load trigger slots still armed on that cross-page high-half page-fault path, the second access still reports breakpoint and still leaves both boundary and adjacent doublewords unchanged",
        second_excpt.triggered == true &&
        second_excpt.cause == CAUSE_BKP &&
        second_preserved
    );
    ai_arch_restore_machine_status(saved_mstatus);

    ai_arch_clear_mcontrol6_slot(0);
    saved_mstatus = ai_arch_enable_machine_breakpoint_delivery();
    goto_priv(PRIV_HS);
    TEST_SETUP_EXCEPT();
    ai_arch_execute_trigger_load_once(vaddr);
    ai_arch_trigger_excpt_snapshot_t third_excpt = ai_arch_snapshot_excpt();
    goto_priv(PRIV_M);
    third_preserved =
        (read64(low_adjacent_phys) == low_adjacent &&
        read64(low_phys) == low_pattern &&
        read64(high_phys) == high_pattern &&
        read64(high_adjacent_phys) == high_adjacent);
    goto_priv(PRIV_HS);
    TEST_ASSERT("after repeated dual-slot load breakpoint hits on the cross-page high-half page-fault path, clearing only the execute-PC slot still leaves the address trigger active without dirtying either adjacent boundary word",
        third_excpt.triggered == true &&
        third_excpt.cause == CAUSE_BKP &&
        third_preserved
    );
    ai_arch_restore_machine_status(saved_mstatus);

    ai_arch_clear_mcontrol6_slot(1);
    goto_priv(PRIV_HS);
    TEST_SETUP_EXCEPT();
    ai_arch_execute_trigger_load_once(vaddr);
    TEST_ASSERT("after clearing both repeated load trigger slots, the same split access falls back to the true second-half load page fault with the second-half start VA in tval in the adjacent-preservation corner",
        excpt.triggered == true &&
        excpt.cause == CAUSE_LPF &&
        excpt.tval == second_half_vaddr
    );

    goto_priv(PRIV_M);
    TEST_ASSERT("before the upper leaf is repaired, repeated trigger hits and the fallback second-half load page fault still leave both boundary and adjacent backing doublewords unchanged",
        read64(low_adjacent_phys) == low_adjacent &&
        read64(low_phys) == low_pattern &&
        read64(high_phys) == high_pattern &&
        read64(high_adjacent_phys) == high_adjacent
    );
    ai_arch_trigger_set_hs_leaf(high_page, phys_page_base(high_page), PTE_V | PTE_AD | PTE_RWX);
    sfence_vma();

    goto_priv(PRIV_HS);
    TEST_SETUP_EXCEPT();
    uint64_t recovery_value = ai_arch_execute_trigger_load_once(vaddr);
    TEST_ASSERT("after repeated dual-slot load trigger hits on the cross-page high-half page-fault path, fixing the upper leaf lets the same split load recover while keeping both adjacent boundary words untouched",
        excpt.triggered == false &&
        recovery_value == recovery_expected
    );

    goto_priv(PRIV_M);
    TEST_ASSERT("the recovered split-load path reads the merged payload without dirtying either adjacent boundary word on the repaired trigger corner",
        read64(low_adjacent_phys) == low_adjacent &&
        read64(low_phys) == low_pattern &&
        read64(high_phys) == high_pattern &&
        read64(high_adjacent_phys) == high_adjacent
    );

    ai_arch_clear_mcontrol6_slot(0);
    ai_arch_clear_mcontrol6_slot(1);

    goto_priv(PRIV_HS);
    hspt_init();
    sfence_vma();

    TEST_END();
}

bool ai_arch_load_execute_and_address_trigger_high_half_page_fault_repeated_breakpoint_then_other_aligned_load_recovers_corner()
{
    TEST_START();

    enum test_page low_page = VSR_GX;
    enum test_page high_page = VSW_GI;
    uintptr_t low_phys = ai_arch_trigger_page_end_phys(low_page);
    uintptr_t high_phys = phys_page_base(high_page);
    uintptr_t trigger_pc = (uintptr_t)ai_arch_execute_trigger_load_once;
    uintptr_t vaddr = ai_arch_trigger_prepare_cross_page_high_half_load_pf_alias();
    uintptr_t second_half_vaddr = vaddr + 4U;
    uintptr_t recovery_vaddr = hs_page_base(low_page) + 0x100U;
    uintptr_t recovery_phys = phys_page_base(low_page) + 0x100U;
    const uint64_t low_pattern = 0x768798a9bacbdcedULL;
    const uint64_t high_pattern = 0xfe1032547698badcULL;
    const uint64_t recovery_expected = 0x0f1e2d3c4b5a6978ULL;
    bool first_preserved;
    bool second_preserved;
    bool third_preserved;

    goto_priv(PRIV_M);
    write64(low_phys, low_pattern);
    write64(high_phys, high_pattern);
    write64(recovery_phys, recovery_expected);

    ai_arch_clear_mcontrol6_slot(0);
    ai_arch_clear_mcontrol6_slot(1);
    ai_arch_arm_mcontrol6_slot(0, trigger_pc, AI_ARCH_MCONTROL6_EXECUTE);
    ai_arch_arm_mcontrol6_slot(1, vaddr, AI_ARCH_MCONTROL6_LOAD);

    uint64_t saved_mstatus = ai_arch_enable_machine_breakpoint_delivery();
    goto_priv(PRIV_HS);
    TEST_SETUP_EXCEPT();
    ai_arch_execute_trigger_load_once(vaddr);
    ai_arch_trigger_excpt_snapshot_t first_excpt = ai_arch_snapshot_excpt();
    goto_priv(PRIV_M);
    first_preserved = (read64(low_phys) == low_pattern && read64(high_phys) == high_pattern);
    goto_priv(PRIV_HS);
    TEST_ASSERT("with both load trigger slots armed on a cross-page access whose low half is cacheable but whose high half would page-fault, the first access reports breakpoint and leaves both boundary doublewords unchanged before the unrelated aligned-load recovery probe",
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
    TEST_ASSERT("with both load trigger slots still armed on that cross-page high-half page-fault path, the second access still reports breakpoint and still leaves both boundary doublewords unchanged before the unrelated aligned-load recovery probe",
        second_excpt.triggered == true &&
        second_excpt.cause == CAUSE_BKP &&
        second_preserved
    );
    ai_arch_restore_machine_status(saved_mstatus);

    ai_arch_clear_mcontrol6_slot(0);
    saved_mstatus = ai_arch_enable_machine_breakpoint_delivery();
    goto_priv(PRIV_HS);
    TEST_SETUP_EXCEPT();
    ai_arch_execute_trigger_load_once(vaddr);
    ai_arch_trigger_excpt_snapshot_t third_excpt = ai_arch_snapshot_excpt();
    goto_priv(PRIV_M);
    third_preserved = (read64(low_phys) == low_pattern && read64(high_phys) == high_pattern);
    goto_priv(PRIV_HS);
    TEST_ASSERT("after repeated dual-slot load breakpoint hits on the cross-page high-half page-fault path, clearing only the execute-PC slot still leaves the address trigger active before the unrelated aligned-load recovery probe",
        third_excpt.triggered == true &&
        third_excpt.cause == CAUSE_BKP &&
        third_preserved
    );
    ai_arch_restore_machine_status(saved_mstatus);

    ai_arch_clear_mcontrol6_slot(1);
    goto_priv(PRIV_HS);
    TEST_SETUP_EXCEPT();
    ai_arch_execute_trigger_load_once(vaddr);
    TEST_ASSERT("after clearing both repeated load trigger slots, the same split access falls back to the true second-half load page fault with the second-half start VA in tval before the unrelated aligned-load recovery probe",
        excpt.triggered == true &&
        excpt.cause == CAUSE_LPF &&
        excpt.tval == second_half_vaddr
    );

    goto_priv(PRIV_M);
    TEST_ASSERT("before the unrelated aligned-load recovery probe, repeated trigger hits and the fallback second-half load page fault still leave both boundary backing doublewords unchanged",
        read64(low_phys) == low_pattern &&
        read64(high_phys) == high_pattern &&
        read64(recovery_phys) == recovery_expected
    );

    goto_priv(PRIV_HS);
    TEST_SETUP_EXCEPT();
    uint64_t recovery_value = ai_arch_execute_trigger_load_once(recovery_vaddr);
    TEST_ASSERT("after repeated dual-slot load trigger hits and the fallback second-half load page fault, another aligned load in the same low page still recovers normally without repairing the faulting high half",
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
    TEST_ASSERT("the repeated dual-slot load trigger+high-half-page-fault corner leaves the boundary backing doublewords untouched while preserving the unrelated aligned-load recovery payload",
        read64(low_phys) == low_pattern &&
        read64(high_phys) == high_pattern &&
        read64(recovery_phys) == recovery_expected
    );

    ai_arch_clear_mcontrol6_slot(0);
    ai_arch_clear_mcontrol6_slot(1);

    goto_priv(PRIV_HS);
    hspt_init();
    sfence_vma();

    TEST_END();
}

bool ai_arch_load_execute_and_address_trigger_high_half_page_fault_repeated_breakpoint_then_other_aligned_load_adjacent_preserved_corner()
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
    uintptr_t recovery_vaddr = hs_page_base(low_page) + 0x100U;
    uintptr_t recovery_adjacent_vaddr = recovery_vaddr + sizeof(uint64_t);
    uintptr_t recovery_phys = phys_page_base(low_page) + 0x100U;
    uintptr_t recovery_adjacent_phys = recovery_phys + sizeof(uint64_t);
    const uint64_t low_adjacent = 0x55aa55aa33cc33ccULL;
    const uint64_t low_pattern = 0x768798a9bacbdcedULL;
    const uint64_t high_pattern = 0xfe1032547698badcULL;
    const uint64_t high_adjacent = 0xcc33cc3355aa55aaULL;
    const uint64_t recovery_expected = 0x0f1e2d3c4b5a6978ULL;
    const uint64_t recovery_adjacent = 0xfedcba9876543210ULL;
    bool first_preserved;
    bool second_preserved;
    bool third_preserved;

    goto_priv(PRIV_M);
    write64(low_adjacent_phys, low_adjacent);
    write64(low_phys, low_pattern);
    write64(high_phys, high_pattern);
    write64(high_adjacent_phys, high_adjacent);
    write64(recovery_phys, recovery_expected);
    write64(recovery_adjacent_phys, recovery_adjacent);

    ai_arch_clear_mcontrol6_slot(0);
    ai_arch_clear_mcontrol6_slot(1);
    ai_arch_arm_mcontrol6_slot(0, trigger_pc, AI_ARCH_MCONTROL6_EXECUTE);
    ai_arch_arm_mcontrol6_slot(1, vaddr, AI_ARCH_MCONTROL6_LOAD);

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
    TEST_ASSERT("with both load trigger slots armed on a cross-page access whose low half is cacheable but whose high half would page-fault, the first access reports breakpoint and leaves both boundary and adjacent doublewords unchanged before the adjacent-preservation recovery probe",
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
    TEST_ASSERT("with both load trigger slots still armed on that cross-page high-half page-fault path, the second access still reports breakpoint and still leaves both boundary and adjacent doublewords unchanged before the adjacent-preservation recovery probe",
        second_excpt.triggered == true &&
        second_excpt.cause == CAUSE_BKP &&
        second_preserved
    );
    ai_arch_restore_machine_status(saved_mstatus);

    ai_arch_clear_mcontrol6_slot(0);
    saved_mstatus = ai_arch_enable_machine_breakpoint_delivery();
    goto_priv(PRIV_HS);
    TEST_SETUP_EXCEPT();
    ai_arch_execute_trigger_load_once(vaddr);
    ai_arch_trigger_excpt_snapshot_t third_excpt = ai_arch_snapshot_excpt();
    goto_priv(PRIV_M);
    third_preserved =
        (read64(low_adjacent_phys) == low_adjacent &&
        read64(low_phys) == low_pattern &&
        read64(high_phys) == high_pattern &&
        read64(high_adjacent_phys) == high_adjacent);
    goto_priv(PRIV_HS);
    TEST_ASSERT("after repeated dual-slot load breakpoint hits on the cross-page high-half page-fault path, clearing only the execute-PC slot still leaves the address trigger active without dirtying either adjacent boundary word before the adjacent-preservation recovery probe",
        third_excpt.triggered == true &&
        third_excpt.cause == CAUSE_BKP &&
        third_preserved
    );
    ai_arch_restore_machine_status(saved_mstatus);

    ai_arch_clear_mcontrol6_slot(1);
    goto_priv(PRIV_HS);
    TEST_SETUP_EXCEPT();
    ai_arch_execute_trigger_load_once(vaddr);
    TEST_ASSERT("after clearing both repeated load trigger slots, the same split access falls back to the true second-half load page fault with the second-half start VA in tval before the adjacent-preservation recovery probe",
        excpt.triggered == true &&
        excpt.cause == CAUSE_LPF &&
        excpt.tval == second_half_vaddr
    );

    goto_priv(PRIV_M);
    TEST_ASSERT("before the adjacent-preservation recovery probe, repeated trigger hits and the fallback second-half load page fault still leave both boundary doublewords, their adjacent neighbors, and the aligned recovery region untouched",
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
    TEST_ASSERT("after repeated dual-slot load trigger hits and the fallback second-half load page fault, another aligned load in the same low page still recovers normally without repairing the faulting high half in the adjacent-preservation corner",
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
    TEST_ASSERT("the repeated dual-slot load trigger+high-half-page-fault corner leaves both boundary doublewords, their adjacent neighbors, and the aligned recovery region untouched while preserving the unrelated aligned-load payload",
        read64(low_adjacent_phys) == low_adjacent &&
        read64(low_phys) == low_pattern &&
        read64(high_phys) == high_pattern &&
        read64(high_adjacent_phys) == high_adjacent &&
        read64(recovery_phys) == recovery_expected &&
        read64(recovery_adjacent_phys) == recovery_adjacent
    );

    ai_arch_clear_mcontrol6_slot(0);
    ai_arch_clear_mcontrol6_slot(1);

    goto_priv(PRIV_HS);
    hspt_init();
    sfence_vma();

    TEST_END();
}

bool ai_arch_store_trigger_pbmt_nc_misaligned_store_fault_no_side_effect_corner()
{
    TEST_START();

    enum test_page low_page = VSRWXPbmt1_GURWXPbmt0;
    enum test_page high_page = VSRWXPbmt1_GURWXPbmt1;
    uintptr_t low_phys = ai_arch_trigger_page_end_phys(low_page);
    uintptr_t high_phys = phys_page_base(high_page);
    const uint64_t low_pattern = 0x435465768798a9baULL;
    const uint64_t high_pattern = 0xcbed1f31537597b9ULL;
    uintptr_t vaddr = ai_arch_trigger_prepare_cross_page_pbmt_nc_store_pf_alias();
    uintptr_t second_half_vaddr = vaddr + 4U;
    const uint64_t store_value = 0x0fedcba987654321ULL;
    const uint64_t expected_low_after_fault =
        ai_arch_trigger_expected_low_half_visible_store_fragment(low_pattern, store_value);

    goto_priv(PRIV_M);
    write64(low_phys, low_pattern);
    write64(high_phys, high_pattern);

    ai_arch_arm_mcontrol6_0(vaddr, AI_ARCH_MCONTROL6_STORE);
    uint64_t saved_mstatus = ai_arch_enable_machine_breakpoint_delivery();

    goto_priv(PRIV_HS);
    TEST_SETUP_EXCEPT();
    sd(vaddr, 0xdeadbeefcafef00dULL);
    if (!(excpt.triggered == true && excpt.cause == CAUSE_BKP)) {
        ai_arch_log_unexpected_trigger_result("unexpected store address-trigger+misalign+page-fault priority result");
    }
    TEST_ASSERT("a store address trigger takes priority when the same HS cross-page access would otherwise stop at low-half PBMT-NC store misalign before ever reaching the high-half page fault",
        excpt.triggered == true &&
        excpt.cause == CAUSE_BKP
    );
    ai_arch_restore_machine_status(saved_mstatus);
    ai_arch_clear_mcontrol6_0();

    goto_priv(PRIV_HS);
    TEST_SETUP_EXCEPT();
    sd(vaddr, store_value);
    TEST_ASSERT("after clearing the store address trigger, the same HS PBMT-NC cross-page store reports store-address-misaligned at the original VA before reaching the high-half page fault",
        excpt.triggered == true &&
        excpt.cause == CAUSE_SAM &&
        excpt.tval == vaddr
    );

    goto_priv(PRIV_M);
    TEST_ASSERT("after the store address trigger is cleared, the misaligned-faulting store leaves both low and high boundary doublewords unchanged",
        read64(low_phys) == low_pattern &&
        read64(high_phys) == high_pattern
    );
    ai_arch_trigger_reset_pbmt_only_env();

    TEST_END();
}

bool ai_arch_store_execute_trigger_pbmt_nc_misaligned_store_fault_no_side_effect_corner()
{
    TEST_START();

    enum test_page low_page = VSRWXPbmt1_GURWXPbmt0;
    enum test_page high_page = VSRWXPbmt1_GURWXPbmt1;
    uintptr_t low_phys = ai_arch_trigger_page_end_phys(low_page);
    uintptr_t high_phys = phys_page_base(high_page);
    const uint64_t low_pattern = 0x5465768798a9bacbULL;
    const uint64_t high_pattern = 0xdcfe20426486a8caULL;
    uintptr_t trigger_pc = (uintptr_t)ai_arch_execute_trigger_store_once;
    uintptr_t vaddr = ai_arch_trigger_prepare_cross_page_pbmt_nc_store_pf_alias();
    uintptr_t second_half_vaddr = vaddr + 4U;
    const uint64_t store_value = 0x0fedcba987654321ULL;
    const uint64_t expected_low_after_fault =
        ai_arch_trigger_expected_low_half_visible_store_fragment(low_pattern, store_value);

    goto_priv(PRIV_M);
    write64(low_phys, low_pattern);
    write64(high_phys, high_pattern);

    ai_arch_arm_mcontrol6_0(trigger_pc, AI_ARCH_MCONTROL6_EXECUTE);
    ai_arch_sync_trigger_state();
    uint64_t saved_mstatus = ai_arch_enable_machine_breakpoint_delivery();

    goto_priv(PRIV_HS);
    TEST_SETUP_EXCEPT();
    ai_arch_execute_trigger_store_once(vaddr, 0xdeadbeefcafef00dULL);
    if (!(excpt.triggered == true && excpt.cause == CAUSE_BKP)) {
        ai_arch_log_unexpected_trigger_result("unexpected store execute-PC trigger+misalign+page-fault priority result");
    }
    TEST_ASSERT("a store execute-PC trigger takes priority when the same HS cross-page access would otherwise stop at low-half PBMT-NC store misalign before the high-half page fault",
        excpt.triggered == true &&
        excpt.cause == CAUSE_BKP
    );
    ai_arch_restore_machine_status(saved_mstatus);
    ai_arch_clear_mcontrol6_0();
    ai_arch_sync_trigger_state();

    goto_priv(PRIV_HS);
    TEST_SETUP_EXCEPT();
    ai_arch_execute_trigger_store_once(vaddr, store_value);
    TEST_ASSERT("after clearing the store execute-PC trigger, the same HS PBMT-NC cross-page store reports store-address-misaligned at the original VA before reaching the high-half page fault",
        excpt.triggered == true &&
        excpt.cause == CAUSE_SAM &&
        excpt.tval == vaddr
    );

    goto_priv(PRIV_M);
    TEST_ASSERT("after the store execute-PC trigger is cleared, the misaligned-faulting store leaves both low and high boundary doublewords unchanged",
        read64(low_phys) == low_pattern &&
        read64(high_phys) == high_pattern
    );
    ai_arch_trigger_reset_pbmt_only_env();

    TEST_END();
}

bool ai_arch_store_execute_trigger_pbmt_nc_misaligned_repeated_breakpoint_then_fault_no_side_effect_corner()
{
    TEST_START();

    enum test_page low_page = VSRWXPbmt1_GURWXPbmt0;
    enum test_page high_page = VSRWXPbmt1_GURWXPbmt1;
    uintptr_t low_phys = ai_arch_trigger_page_end_phys(low_page);
    uintptr_t high_phys = phys_page_base(high_page);
    const uint64_t low_pattern = 0x425364758697a8b9ULL;
    const uint64_t high_pattern = 0xcaddef2031425364ULL;
    uintptr_t trigger_pc = (uintptr_t)ai_arch_execute_trigger_store_once;
    uintptr_t vaddr = ai_arch_trigger_prepare_cross_page_pbmt_nc_store_pf_alias();
    uintptr_t second_half_vaddr = vaddr + 4U;
    const uint64_t first_store_value = 0xdeadbeefcafef00dULL;
    const uint64_t second_store_value = 0x0fedcba987654321ULL;
    const uint64_t fallback_store_value = 0x123456789abcdef0ULL;
    const uint64_t expected_low_after_fault =
        ai_arch_trigger_expected_low_half_visible_store_fragment(low_pattern, fallback_store_value);
    bool first_preserved;
    bool second_preserved;
    bool first_triggered;
    bool second_triggered;
    uint64_t first_cause;
    uint64_t second_cause;

    goto_priv(PRIV_M);
    write64(low_phys, low_pattern);
    write64(high_phys, high_pattern);

    ai_arch_arm_mcontrol6_0(trigger_pc, AI_ARCH_MCONTROL6_EXECUTE);
    uint64_t saved_mstatus = ai_arch_enable_machine_breakpoint_delivery();

    goto_priv(PRIV_HS);
    TEST_SETUP_EXCEPT();
    ai_arch_execute_trigger_store_once(vaddr, first_store_value);
    first_triggered = excpt.triggered;
    first_cause = excpt.cause;
    goto_priv(PRIV_M);
    first_preserved = (read64(low_phys) == low_pattern && read64(high_phys) == high_pattern);
    goto_priv(PRIV_HS);
    TEST_ASSERT("with a store execute-PC trigger armed on the HS cross-page PBMT-NC misalign-plus-page-fault path, the first access reports breakpoint and leaves both boundary doublewords unchanged",
        first_triggered == true &&
        first_cause == CAUSE_BKP &&
        first_preserved
    );
    ai_arch_restore_machine_status(saved_mstatus);

    saved_mstatus = ai_arch_enable_machine_breakpoint_delivery();
    goto_priv(PRIV_HS);
    TEST_SETUP_EXCEPT();
    ai_arch_execute_trigger_store_once(vaddr, second_store_value);
    second_triggered = excpt.triggered;
    second_cause = excpt.cause;
    goto_priv(PRIV_M);
    second_preserved = (read64(low_phys) == low_pattern && read64(high_phys) == high_pattern);
    goto_priv(PRIV_HS);
    TEST_ASSERT("with the same store execute-PC trigger still armed on that HS cross-page PBMT-NC misalign-plus-page-fault path, the second access still reports breakpoint and still leaves both boundary doublewords unchanged",
        second_triggered == true &&
        second_cause == CAUSE_BKP &&
        second_preserved
    );
    ai_arch_restore_machine_status(saved_mstatus);
    ai_arch_clear_mcontrol6_0();

    goto_priv(PRIV_HS);
    TEST_SETUP_EXCEPT();
    ai_arch_execute_trigger_store_once(vaddr, fallback_store_value);
    TEST_ASSERT("after clearing the repeated store execute-PC trigger, the same HS PBMT-NC cross-page store reports store-address-misaligned at the original VA before reaching the high-half page fault",
        excpt.triggered == true &&
        excpt.cause == CAUSE_SAM &&
        excpt.tval == vaddr
    );

    goto_priv(PRIV_M);
    TEST_ASSERT("after the repeated store execute-PC trigger is cleared, the misaligned-faulting store leaves both low and high boundary doublewords unchanged",
        read64(low_phys) == low_pattern &&
        read64(high_phys) == high_pattern
    );
    ai_arch_trigger_reset_pbmt_only_env();

    TEST_END();
}

bool ai_arch_store_execute_and_address_trigger_pbmt_nc_misaligned_store_fault_no_side_effect_corner()
{
    TEST_START();

    enum test_page low_page = VSRWXPbmt1_GURWXPbmt0;
    enum test_page high_page = VSRWXPbmt1_GURWXPbmt1;
    uintptr_t low_phys = ai_arch_trigger_page_end_phys(low_page);
    uintptr_t high_phys = phys_page_base(high_page);
    const uint64_t low_pattern = 0x65768798a9bacbdcULL;
    const uint64_t high_pattern = 0xed0f31537597b9dbULL;
    uintptr_t trigger_pc = (uintptr_t)ai_arch_execute_trigger_store_once;
    uintptr_t vaddr = ai_arch_trigger_prepare_cross_page_pbmt_nc_store_pf_alias();
    uintptr_t second_half_vaddr = vaddr + 4U;
    const uint64_t store_value = 0xdeadbeefcafef00dULL;
    const uint64_t expected_low_after_fault =
        ai_arch_trigger_expected_low_half_visible_store_fragment(low_pattern, store_value);

    goto_priv(PRIV_M);
    write64(low_phys, low_pattern);
    write64(high_phys, high_pattern);

    ai_arch_clear_mcontrol6_slot(0);
    ai_arch_clear_mcontrol6_slot(1);
    ai_arch_arm_mcontrol6_slot(0, trigger_pc, AI_ARCH_MCONTROL6_EXECUTE);
    ai_arch_arm_mcontrol6_slot(1, vaddr, AI_ARCH_MCONTROL6_STORE);

    uint64_t saved_mstatus = ai_arch_enable_machine_breakpoint_delivery();
    goto_priv(PRIV_HS);
    TEST_SETUP_EXCEPT();
    ai_arch_execute_trigger_store_once(vaddr, store_value);
    TEST_ASSERT("when both execute-PC and address triggers match the same HS cross-page PBMT-NC misaligned store, breakpoint still wins before the low-half misalign shortcut and the high-half page fault are considered",
        excpt.triggered == true &&
        excpt.cause == CAUSE_BKP
    );
    ai_arch_restore_machine_status(saved_mstatus);

    ai_arch_clear_mcontrol6_slot(0);
    saved_mstatus = ai_arch_enable_machine_breakpoint_delivery();
    goto_priv(PRIV_HS);
    TEST_SETUP_EXCEPT();
    ai_arch_execute_trigger_store_once(vaddr, store_value);
    TEST_ASSERT("after clearing only the execute-PC trigger, the remaining store-address trigger still reports breakpoint on the same HS cross-page PBMT-NC misaligned store",
        excpt.triggered == true &&
        excpt.cause == CAUSE_BKP
    );
    ai_arch_restore_machine_status(saved_mstatus);

    ai_arch_clear_mcontrol6_slot(1);
    goto_priv(PRIV_HS);
    TEST_SETUP_EXCEPT();
    ai_arch_execute_trigger_store_once(vaddr, store_value);
    TEST_ASSERT("after clearing both store trigger slots, the same HS PBMT-NC cross-page store reports store-address-misaligned at the original VA before reaching the high-half page fault",
        excpt.triggered == true &&
        excpt.cause == CAUSE_SAM &&
        excpt.tval == vaddr
    );

    goto_priv(PRIV_M);
    TEST_ASSERT("after both store trigger slots are cleared, the misaligned-faulting store leaves both low and high boundary doublewords unchanged",
        read64(low_phys) == low_pattern &&
        read64(high_phys) == high_pattern
    );
    ai_arch_trigger_reset_pbmt_only_env();

    TEST_END();
}

bool ai_arch_store_execute_and_address_trigger_pbmt_nc_misaligned_repeated_breakpoint_then_fault_no_side_effect_corner()
{
    TEST_START();

    enum test_page low_page = VSRWXPbmt1_GURWXPbmt0;
    enum test_page high_page = VSRWXPbmt1_GURWXPbmt1;
    uintptr_t low_phys = ai_arch_trigger_page_end_phys(low_page);
    uintptr_t high_phys = phys_page_base(high_page);
    const uint64_t low_pattern = 0x8798a9bacbdcedfeULL;
    const uint64_t high_pattern = 0x1032547698badcfeULL;
    uintptr_t trigger_pc = (uintptr_t)ai_arch_execute_trigger_store_once;
    uintptr_t vaddr = ai_arch_trigger_prepare_cross_page_pbmt_nc_store_pf_alias();
    uintptr_t second_half_vaddr = vaddr + 4U;
    const uint64_t first_store_value = 0xdeadbeefcafef00dULL;
    const uint64_t second_store_value = 0x1122334455667788ULL;
    const uint64_t third_store_value = 0x0fedcba987654321ULL;
    const uint64_t fallback_store_value = first_store_value;
    const uint64_t expected_low_after_fault =
        ai_arch_trigger_expected_low_half_visible_store_fragment(low_pattern, fallback_store_value);
    bool first_preserved;
    bool second_preserved;
    bool third_preserved;
    bool first_triggered;
    bool second_triggered;
    bool third_triggered;
    uint64_t first_cause;
    uint64_t second_cause;
    uint64_t third_cause;

    goto_priv(PRIV_M);
    write64(low_phys, low_pattern);
    write64(high_phys, high_pattern);

    ai_arch_clear_mcontrol6_slot(0);
    ai_arch_clear_mcontrol6_slot(1);
    ai_arch_arm_mcontrol6_slot(0, trigger_pc, AI_ARCH_MCONTROL6_EXECUTE);
    ai_arch_arm_mcontrol6_slot(1, vaddr, AI_ARCH_MCONTROL6_STORE);

    uint64_t saved_mstatus = ai_arch_enable_machine_breakpoint_delivery();
    goto_priv(PRIV_HS);
    TEST_SETUP_EXCEPT();
    ai_arch_execute_trigger_store_once(vaddr, first_store_value);
    first_triggered = excpt.triggered;
    first_cause = excpt.cause;
    goto_priv(PRIV_M);
    first_preserved = (read64(low_phys) == low_pattern && read64(high_phys) == high_pattern);
    goto_priv(PRIV_HS);
    TEST_ASSERT("with both store trigger slots armed on the HS cross-page PBMT-NC misalign-plus-page-fault path, the first access reports breakpoint and leaves both boundary doublewords unchanged",
        first_triggered == true &&
        first_cause == CAUSE_BKP &&
        first_preserved
    );
    ai_arch_restore_machine_status(saved_mstatus);

    saved_mstatus = ai_arch_enable_machine_breakpoint_delivery();
    goto_priv(PRIV_HS);
    TEST_SETUP_EXCEPT();
    ai_arch_execute_trigger_store_once(vaddr, second_store_value);
    second_triggered = excpt.triggered;
    second_cause = excpt.cause;
    goto_priv(PRIV_M);
    second_preserved = (read64(low_phys) == low_pattern && read64(high_phys) == high_pattern);
    goto_priv(PRIV_HS);
    TEST_ASSERT("with both store trigger slots still armed on that HS cross-page PBMT-NC misalign-plus-page-fault path, the second access still reports breakpoint and still leaves both boundary doublewords unchanged",
        second_triggered == true &&
        second_cause == CAUSE_BKP &&
        second_preserved
    );
    ai_arch_restore_machine_status(saved_mstatus);

    ai_arch_clear_mcontrol6_slot(0);
    saved_mstatus = ai_arch_enable_machine_breakpoint_delivery();
    goto_priv(PRIV_HS);
    TEST_SETUP_EXCEPT();
    ai_arch_execute_trigger_store_once(vaddr, third_store_value);
    third_triggered = excpt.triggered;
    third_cause = excpt.cause;
    goto_priv(PRIV_M);
    third_preserved = (read64(low_phys) == low_pattern && read64(high_phys) == high_pattern);
    goto_priv(PRIV_HS);
    TEST_ASSERT("after repeated dual-slot store breakpoint hits on the HS cross-page PBMT-NC misalign-plus-page-fault path, clearing only the execute-PC slot still leaves the address trigger active",
        third_triggered == true &&
        third_cause == CAUSE_BKP &&
        third_preserved
    );
    ai_arch_restore_machine_status(saved_mstatus);

    ai_arch_clear_mcontrol6_slot(1);
    goto_priv(PRIV_HS);
    TEST_SETUP_EXCEPT();
    ai_arch_execute_trigger_store_once(vaddr, fallback_store_value);
    TEST_ASSERT("after clearing both repeated store trigger slots, the same HS PBMT-NC cross-page store reports store-address-misaligned at the original VA before reaching the high-half page fault",
        excpt.triggered == true &&
        excpt.cause == CAUSE_SAM &&
        excpt.tval == vaddr
    );

    goto_priv(PRIV_M);
    TEST_ASSERT("after both repeated store trigger slots are cleared, the misaligned-faulting store leaves both low and high boundary doublewords unchanged",
        read64(low_phys) == low_pattern &&
        read64(high_phys) == high_pattern
    );
    ai_arch_trigger_reset_pbmt_only_env();

    TEST_END();
}

bool ai_arch_store_execute_and_address_trigger_pbmt_nc_misaligned_repeated_breakpoint_then_fault_no_side_effect_adjacent_preserved_corner()
{
    TEST_START();

    enum test_page low_page = VSRWXPbmt1_GURWXPbmt0;
    enum test_page high_page = VSRWXPbmt1_GURWXPbmt1;
    uintptr_t low_adjacent_phys = phys_page_base(low_page) + PAGE_SIZE - (2 * sizeof(uint64_t));
    uintptr_t low_phys = ai_arch_trigger_page_end_phys(low_page);
    uintptr_t high_phys = phys_page_base(high_page);
    uintptr_t high_adjacent_phys = high_phys + sizeof(uint64_t);
    const uint64_t low_adjacent = 0xfaceb00c12344321ULL;
    const uint64_t low_pattern = 0x8798a9bacbdcedfeULL;
    const uint64_t high_pattern = 0x1032547698badcfeULL;
    const uint64_t high_adjacent = 0xabcdef0123456789ULL;
    uintptr_t trigger_pc = (uintptr_t)ai_arch_execute_trigger_store_once;
    uintptr_t vaddr = ai_arch_trigger_prepare_cross_page_pbmt_nc_store_pf_alias();
    uintptr_t second_half_vaddr = vaddr + 4U;
    const uint64_t first_store_value = 0xdeadbeefcafef00dULL;
    const uint64_t second_store_value = 0x1122334455667788ULL;
    const uint64_t third_store_value = 0x0fedcba987654321ULL;
    const uint64_t fallback_store_value = first_store_value;
    const uint64_t expected_low_after_fault =
        ai_arch_trigger_expected_low_half_visible_store_fragment(low_pattern, fallback_store_value);
    bool first_preserved;
    bool second_preserved;
    bool third_preserved;
    bool first_triggered;
    bool second_triggered;
    bool third_triggered;
    uint64_t first_cause;
    uint64_t second_cause;
    uint64_t third_cause;

    goto_priv(PRIV_M);
    write64(low_adjacent_phys, low_adjacent);
    write64(low_phys, low_pattern);
    write64(high_phys, high_pattern);
    write64(high_adjacent_phys, high_adjacent);

    ai_arch_clear_mcontrol6_slot(0);
    ai_arch_clear_mcontrol6_slot(1);
    ai_arch_arm_mcontrol6_slot(0, trigger_pc, AI_ARCH_MCONTROL6_EXECUTE);
    ai_arch_arm_mcontrol6_slot(1, vaddr, AI_ARCH_MCONTROL6_STORE);

    uint64_t saved_mstatus = ai_arch_enable_machine_breakpoint_delivery();
    goto_priv(PRIV_HS);
    TEST_SETUP_EXCEPT();
    ai_arch_execute_trigger_store_once(vaddr, first_store_value);
    first_triggered = excpt.triggered;
    first_cause = excpt.cause;
    goto_priv(PRIV_M);
    first_preserved =
        (read64(low_adjacent_phys) == low_adjacent &&
        read64(low_phys) == low_pattern &&
        read64(high_phys) == high_pattern &&
        read64(high_adjacent_phys) == high_adjacent);
    goto_priv(PRIV_HS);
    TEST_ASSERT("with both store trigger slots armed on the HS cross-page PBMT-NC misalign-plus-page-fault path, the first access reports breakpoint and leaves both boundary doublewords plus their adjacent neighbors unchanged in the adjacent-preservation corner",
        first_triggered == true &&
        first_cause == CAUSE_BKP &&
        first_preserved
    );
    ai_arch_restore_machine_status(saved_mstatus);

    saved_mstatus = ai_arch_enable_machine_breakpoint_delivery();
    goto_priv(PRIV_HS);
    TEST_SETUP_EXCEPT();
    ai_arch_execute_trigger_store_once(vaddr, second_store_value);
    second_triggered = excpt.triggered;
    second_cause = excpt.cause;
    goto_priv(PRIV_M);
    second_preserved =
        (read64(low_adjacent_phys) == low_adjacent &&
        read64(low_phys) == low_pattern &&
        read64(high_phys) == high_pattern &&
        read64(high_adjacent_phys) == high_adjacent);
    goto_priv(PRIV_HS);
    TEST_ASSERT("with both store trigger slots still armed on that HS cross-page PBMT-NC misalign-plus-page-fault path, the second access still reports breakpoint and still leaves both boundary doublewords plus their adjacent neighbors unchanged in the adjacent-preservation corner",
        second_triggered == true &&
        second_cause == CAUSE_BKP &&
        second_preserved
    );
    ai_arch_restore_machine_status(saved_mstatus);

    ai_arch_clear_mcontrol6_slot(0);
    saved_mstatus = ai_arch_enable_machine_breakpoint_delivery();
    goto_priv(PRIV_HS);
    TEST_SETUP_EXCEPT();
    ai_arch_execute_trigger_store_once(vaddr, third_store_value);
    third_triggered = excpt.triggered;
    third_cause = excpt.cause;
    goto_priv(PRIV_M);
    third_preserved =
        (read64(low_adjacent_phys) == low_adjacent &&
        read64(low_phys) == low_pattern &&
        read64(high_phys) == high_pattern &&
        read64(high_adjacent_phys) == high_adjacent);
    goto_priv(PRIV_HS);
    TEST_ASSERT("after repeated dual-slot store breakpoint hits on the HS cross-page PBMT-NC misalign-plus-page-fault path, clearing only the execute-PC slot still leaves the address trigger active without dirtying either boundary doubleword or adjacent neighbor in the adjacent-preservation corner",
        third_triggered == true &&
        third_cause == CAUSE_BKP &&
        third_preserved
    );
    ai_arch_restore_machine_status(saved_mstatus);

    ai_arch_clear_mcontrol6_slot(1);
    goto_priv(PRIV_HS);
    TEST_SETUP_EXCEPT();
    ai_arch_execute_trigger_store_once(vaddr, fallback_store_value);
    TEST_ASSERT("after clearing both repeated store trigger slots, the same HS PBMT-NC cross-page store reports store-address-misaligned at the original VA before reaching the high-half page fault in the adjacent-preservation corner",
        excpt.triggered == true &&
        excpt.cause == CAUSE_SAM &&
        excpt.tval == vaddr
    );

    goto_priv(PRIV_M);
    TEST_ASSERT("after both repeated store trigger slots are cleared, the misaligned-faulting store leaves both boundary doublewords and adjacent neighbors unchanged",
        read64(low_adjacent_phys) == low_adjacent &&
        read64(low_phys) == low_pattern &&
        read64(high_phys) == high_pattern &&
        read64(high_adjacent_phys) == high_adjacent
    );
    ai_arch_trigger_reset_pbmt_only_env();

    TEST_END();
}

bool ai_arch_store_execute_and_address_trigger_pbmt_nc_misaligned_repeated_breakpoint_then_repaired_cross_page_store_recovers_corner()
{
    TEST_START();

    enum test_page low_page = VSRWXPbmt1_GURWXPbmt0;
    enum test_page high_page = VSRWXPbmt1_GURWXPbmt1;
    uintptr_t low_phys = ai_arch_trigger_page_end_phys(low_page);
    uintptr_t high_phys = phys_page_base(high_page);
    uintptr_t trigger_pc = (uintptr_t)ai_arch_execute_trigger_store_once;
    uintptr_t vaddr = ai_arch_trigger_prepare_cross_page_pbmt_nc_store_pf_alias();
    uintptr_t second_half_vaddr = vaddr + 4U;
    const uint64_t low_pattern = 0x98a9bacbdcedfe10ULL;
    const uint64_t high_pattern = 0x21436587a9cbed0fULL;
    const uint64_t recovery_store = 0x0123456789abcdefULL;
    const uint64_t fallback_store_value = 0xdeadbeefcafef00dULL;
    const uint64_t expected_low_after_fault =
        ai_arch_trigger_expected_low_half_visible_store_fragment(low_pattern, fallback_store_value);
    const uint64_t expected_low_after_store =
        (low_pattern & 0xffffffffULL) | ((recovery_store & 0xffffffffULL) << 32);
    const uint64_t expected_high_after_store =
        (high_pattern & 0xffffffff00000000ULL) | (recovery_store >> 32);
    bool first_preserved;
    bool second_preserved;
    bool third_preserved;
    bool first_triggered;
    bool second_triggered;
    bool third_triggered;
    uint64_t first_cause;
    uint64_t second_cause;
    uint64_t third_cause;

    goto_priv(PRIV_M);
    write64(low_phys, low_pattern);
    write64(high_phys, high_pattern);

    ai_arch_clear_mcontrol6_slot(0);
    ai_arch_clear_mcontrol6_slot(1);
    ai_arch_arm_mcontrol6_slot(0, trigger_pc, AI_ARCH_MCONTROL6_EXECUTE);
    ai_arch_arm_mcontrol6_slot(1, vaddr, AI_ARCH_MCONTROL6_STORE);

    uint64_t saved_mstatus = ai_arch_enable_machine_breakpoint_delivery();
    goto_priv(PRIV_HS);
    TEST_SETUP_EXCEPT();
    ai_arch_execute_trigger_store_once(vaddr, 0xdeadbeefcafef00dULL);
    first_triggered = excpt.triggered;
    first_cause = excpt.cause;
    goto_priv(PRIV_M);
    first_preserved = (read64(low_phys) == low_pattern && read64(high_phys) == high_pattern);
    goto_priv(PRIV_HS);
    TEST_ASSERT("with both store trigger slots armed on the HS cross-page PBMT-NC misalign-plus-page-fault path, the first access reports breakpoint and leaves both boundary doublewords unchanged",
        first_triggered == true &&
        first_cause == CAUSE_BKP &&
        first_preserved
    );
    ai_arch_restore_machine_status(saved_mstatus);

    saved_mstatus = ai_arch_enable_machine_breakpoint_delivery();
    goto_priv(PRIV_HS);
    TEST_SETUP_EXCEPT();
    ai_arch_execute_trigger_store_once(vaddr, 0x1122334455667788ULL);
    second_triggered = excpt.triggered;
    second_cause = excpt.cause;
    goto_priv(PRIV_M);
    second_preserved = (read64(low_phys) == low_pattern && read64(high_phys) == high_pattern);
    goto_priv(PRIV_HS);
    TEST_ASSERT("with both store trigger slots still armed on that HS cross-page PBMT-NC misalign-plus-page-fault path, the second access still reports breakpoint and still leaves both boundary doublewords unchanged",
        second_triggered == true &&
        second_cause == CAUSE_BKP &&
        second_preserved
    );
    ai_arch_restore_machine_status(saved_mstatus);

    ai_arch_clear_mcontrol6_slot(0);
    saved_mstatus = ai_arch_enable_machine_breakpoint_delivery();
    goto_priv(PRIV_HS);
    TEST_SETUP_EXCEPT();
    ai_arch_execute_trigger_store_once(vaddr, 0x0fedcba987654321ULL);
    third_triggered = excpt.triggered;
    third_cause = excpt.cause;
    goto_priv(PRIV_M);
    third_preserved = (read64(low_phys) == low_pattern && read64(high_phys) == high_pattern);
    goto_priv(PRIV_HS);
    TEST_ASSERT("after repeated dual-slot store breakpoint hits on the HS cross-page PBMT-NC misalign-plus-page-fault path, clearing only the execute-PC slot still leaves the address trigger active",
        third_triggered == true &&
        third_cause == CAUSE_BKP &&
        third_preserved
    );
    ai_arch_restore_machine_status(saved_mstatus);

    ai_arch_clear_mcontrol6_slot(1);
    goto_priv(PRIV_HS);
    TEST_SETUP_EXCEPT();
    ai_arch_execute_trigger_store_once(vaddr, fallback_store_value);
    TEST_ASSERT("after clearing both repeated store trigger slots, the same HS PBMT-NC cross-page store reports store-address-misaligned at the original VA before reaching the high-half page fault",
        excpt.triggered == true &&
        excpt.cause == CAUSE_SAM &&
        excpt.tval == vaddr
    );

    goto_priv(PRIV_M);
    TEST_ASSERT("after both repeated store trigger slots are cleared, the misaligned-faulting store leaves both boundary doublewords unchanged before the repair step",
        read64(low_phys) == low_pattern &&
        read64(high_phys) == high_pattern
    );

    ai_arch_trigger_reset_pbmt_only_env();
    ai_arch_trigger_set_hs_leaf(low_page, phys_page_base(low_page), PTE_V | PTE_AD | PTE_RWX);
    ai_arch_trigger_set_hs_leaf(high_page, phys_page_base(high_page), PTE_V | PTE_AD | PTE_RWX);
    write64(low_phys, low_pattern);
    write64(high_phys, high_pattern);
    sfence_vma();

    goto_priv(PRIV_HS);
    TEST_SETUP_EXCEPT();
    ai_arch_execute_trigger_store_once(vaddr, recovery_store);
    TEST_ASSERT("after repeated dual-slot trigger hits on the cross-page misalign-plus-page-fault path, fixing both leaves lets the same split store recover and commit normally",
        excpt.triggered == false
    );

    goto_priv(PRIV_M);
    TEST_ASSERT("the recovered cross-page split store updates the low boundary fragment as expected after the earlier repeated trigger+misalign+page-fault sequence is cleared",
        read64(low_phys) == expected_low_after_store
    );
    TEST_ASSERT("the recovered cross-page split store updates the high boundary fragment as expected after the earlier repeated trigger+misalign+page-fault sequence is cleared",
        read64(high_phys) == expected_high_after_store
    );

    TEST_END();
}

bool ai_arch_store_execute_and_address_trigger_pbmt_nc_misaligned_repeated_breakpoint_then_repaired_cross_page_store_adjacent_preserved_corner()
{
    TEST_START();

    enum test_page low_page = VSRWXPbmt1_GURWXPbmt0;
    enum test_page high_page = VSRWXPbmt1_GURWXPbmt1;
    uintptr_t low_adjacent_phys = phys_page_base(low_page) + PAGE_SIZE - (2 * sizeof(uint64_t));
    uintptr_t low_phys = ai_arch_trigger_page_end_phys(low_page);
    uintptr_t high_phys = phys_page_base(high_page);
    uintptr_t high_adjacent_phys = high_phys + sizeof(uint64_t);
    uintptr_t trigger_pc = (uintptr_t)ai_arch_execute_trigger_store_once;
    uintptr_t vaddr = ai_arch_trigger_prepare_cross_page_pbmt_nc_store_pf_alias();
    uintptr_t second_half_vaddr = vaddr + 4U;
    const uint64_t low_adjacent = 0xfaceb00c12344321ULL;
    const uint64_t low_pattern = 0x98a9bacbdcedfe10ULL;
    const uint64_t high_pattern = 0x21436587a9cbed0fULL;
    const uint64_t high_adjacent = 0xabcdef0123456789ULL;
    const uint64_t recovery_store = 0x0123456789abcdefULL;
    const uint64_t fallback_store_value = 0xdeadbeefcafef00dULL;
    const uint64_t expected_low_after_fault =
        ai_arch_trigger_expected_low_half_visible_store_fragment(low_pattern, fallback_store_value);
    const uint64_t expected_low_after_store =
        (low_pattern & 0xffffffffULL) | ((recovery_store & 0xffffffffULL) << 32);
    const uint64_t expected_high_after_store =
        (high_pattern & 0xffffffff00000000ULL) | (recovery_store >> 32);
    bool first_preserved;
    bool second_preserved;
    bool third_preserved;
    bool first_triggered;
    bool second_triggered;
    bool third_triggered;
    uint64_t first_cause;
    uint64_t second_cause;
    uint64_t third_cause;

    goto_priv(PRIV_M);
    write64(low_adjacent_phys, low_adjacent);
    write64(low_phys, low_pattern);
    write64(high_phys, high_pattern);
    write64(high_adjacent_phys, high_adjacent);

    ai_arch_clear_mcontrol6_slot(0);
    ai_arch_clear_mcontrol6_slot(1);
    ai_arch_arm_mcontrol6_slot(0, trigger_pc, AI_ARCH_MCONTROL6_EXECUTE);
    ai_arch_arm_mcontrol6_slot(1, vaddr, AI_ARCH_MCONTROL6_STORE);

    uint64_t saved_mstatus = ai_arch_enable_machine_breakpoint_delivery();
    goto_priv(PRIV_HS);
    TEST_SETUP_EXCEPT();
    ai_arch_execute_trigger_store_once(vaddr, 0xdeadbeefcafef00dULL);
    first_triggered = excpt.triggered;
    first_cause = excpt.cause;
    goto_priv(PRIV_M);
    first_preserved =
        (read64(low_adjacent_phys) == low_adjacent &&
        read64(low_phys) == low_pattern &&
        read64(high_phys) == high_pattern &&
        read64(high_adjacent_phys) == high_adjacent);
    goto_priv(PRIV_HS);
    TEST_ASSERT("with both store trigger slots armed on the HS cross-page PBMT-NC misalign-plus-page-fault path, the first access reports breakpoint and leaves both boundary and adjacent doublewords unchanged in the repaired-cross-page adjacent-preservation corner",
        first_triggered == true &&
        first_cause == CAUSE_BKP &&
        first_preserved
    );
    ai_arch_restore_machine_status(saved_mstatus);

    saved_mstatus = ai_arch_enable_machine_breakpoint_delivery();
    goto_priv(PRIV_HS);
    TEST_SETUP_EXCEPT();
    ai_arch_execute_trigger_store_once(vaddr, 0x1122334455667788ULL);
    second_triggered = excpt.triggered;
    second_cause = excpt.cause;
    goto_priv(PRIV_M);
    second_preserved =
        (read64(low_adjacent_phys) == low_adjacent &&
        read64(low_phys) == low_pattern &&
        read64(high_phys) == high_pattern &&
        read64(high_adjacent_phys) == high_adjacent);
    goto_priv(PRIV_HS);
    TEST_ASSERT("with both store trigger slots still armed on that HS cross-page PBMT-NC misalign-plus-page-fault path, the second access still reports breakpoint and still leaves both boundary and adjacent doublewords unchanged in the repaired-cross-page adjacent-preservation corner",
        second_triggered == true &&
        second_cause == CAUSE_BKP &&
        second_preserved
    );
    ai_arch_restore_machine_status(saved_mstatus);

    ai_arch_clear_mcontrol6_slot(0);
    saved_mstatus = ai_arch_enable_machine_breakpoint_delivery();
    goto_priv(PRIV_HS);
    TEST_SETUP_EXCEPT();
    ai_arch_execute_trigger_store_once(vaddr, 0x0fedcba987654321ULL);
    third_triggered = excpt.triggered;
    third_cause = excpt.cause;
    goto_priv(PRIV_M);
    third_preserved =
        (read64(low_adjacent_phys) == low_adjacent &&
        read64(low_phys) == low_pattern &&
        read64(high_phys) == high_pattern &&
        read64(high_adjacent_phys) == high_adjacent);
    goto_priv(PRIV_HS);
    TEST_ASSERT("after repeated dual-slot store breakpoint hits on the HS cross-page PBMT-NC misalign-plus-page-fault path, clearing only the execute-PC slot still leaves the address trigger active without dirtying either adjacent boundary word in the repaired-cross-page adjacent-preservation corner",
        third_triggered == true &&
        third_cause == CAUSE_BKP &&
        third_preserved
    );
    ai_arch_restore_machine_status(saved_mstatus);

    ai_arch_clear_mcontrol6_slot(1);
    goto_priv(PRIV_HS);
    TEST_SETUP_EXCEPT();
    ai_arch_execute_trigger_store_once(vaddr, fallback_store_value);
    TEST_ASSERT("after clearing both repeated store trigger slots, the same HS PBMT-NC cross-page store reports store-address-misaligned at the original VA before reaching the high-half page fault in the repaired-cross-page adjacent-preservation corner",
        excpt.triggered == true &&
        excpt.cause == CAUSE_SAM &&
        excpt.tval == vaddr
    );

    goto_priv(PRIV_M);
    TEST_ASSERT("after both repeated store trigger slots are cleared, the misaligned-faulting store leaves both boundary doublewords and adjacent neighbors unchanged before the repair step",
        read64(low_adjacent_phys) == low_adjacent &&
        read64(low_phys) == low_pattern &&
        read64(high_phys) == high_pattern &&
        read64(high_adjacent_phys) == high_adjacent
    );

    ai_arch_trigger_reset_pbmt_only_env();
    ai_arch_trigger_set_hs_leaf(low_page, phys_page_base(low_page), PTE_V | PTE_AD | PTE_RWX);
    ai_arch_trigger_set_hs_leaf(high_page, phys_page_base(high_page), PTE_V | PTE_AD | PTE_RWX);
    write64(low_adjacent_phys, low_adjacent);
    write64(low_phys, low_pattern);
    write64(high_phys, high_pattern);
    write64(high_adjacent_phys, high_adjacent);
    sfence_vma();

    goto_priv(PRIV_HS);
    TEST_SETUP_EXCEPT();
    ai_arch_execute_trigger_store_once(vaddr, recovery_store);
    TEST_ASSERT("after repeated dual-slot trigger hits on the cross-page misalign-plus-page-fault path, fixing both leaves lets the same split store recover and commit normally while keeping both adjacent boundary words untouched",
        excpt.triggered == false
    );

    goto_priv(PRIV_M);
    TEST_ASSERT("the recovered cross-page split store updates only the two boundary fragments while leaving both adjacent boundary words untouched on the repaired trigger corner",
        read64(low_adjacent_phys) == low_adjacent &&
        read64(low_phys) == expected_low_after_store &&
        read64(high_phys) == expected_high_after_store &&
        read64(high_adjacent_phys) == high_adjacent
    );

    TEST_END();
}

bool ai_arch_store_execute_and_address_trigger_high_half_page_fault_repeated_breakpoint_then_fix_high_half_store_recovers_corner()
{
    TEST_START();

    enum test_page low_page = VSRW_GX;
    enum test_page high_page = VSRX_GRWX;
    uintptr_t low_phys = ai_arch_trigger_page_end_phys(low_page);
    uintptr_t high_phys = phys_page_base(high_page);
    uintptr_t trigger_pc = (uintptr_t)ai_arch_execute_trigger_store_once;
    uintptr_t vaddr = ai_arch_trigger_prepare_cross_page_high_half_store_pf_alias();
    uintptr_t second_half_vaddr = vaddr + 4U;
    const uint64_t low_pattern = 0x98a9bacbdcedfe10ULL;
    const uint64_t high_pattern = 0x21436587a9cbed0fULL;
    const uint64_t recovery_store = 0x0123456789abcdefULL;
    const uint64_t expected_low_after_store =
        (low_pattern & 0xffffffffULL) | ((recovery_store & 0xffffffffULL) << 32);
    const uint64_t expected_high_after_store =
        (high_pattern & 0xffffffff00000000ULL) | (recovery_store >> 32);
    bool first_preserved;
    bool second_preserved;
    bool third_preserved;

    goto_priv(PRIV_M);
    write64(low_phys, low_pattern);
    write64(high_phys, high_pattern);

    ai_arch_clear_mcontrol6_slot(0);
    ai_arch_clear_mcontrol6_slot(1);
    ai_arch_arm_mcontrol6_slot(0, trigger_pc, AI_ARCH_MCONTROL6_EXECUTE);
    ai_arch_arm_mcontrol6_slot(1, vaddr, AI_ARCH_MCONTROL6_STORE);

    uint64_t saved_mstatus = ai_arch_enable_machine_breakpoint_delivery();
    goto_priv(PRIV_HS);
    TEST_SETUP_EXCEPT();
    ai_arch_execute_trigger_store_once(vaddr, 0xdeadbeefcafef00dULL);
    ai_arch_trigger_excpt_snapshot_t first_excpt = ai_arch_snapshot_excpt();
    goto_priv(PRIV_M);
    first_preserved = (read64(low_phys) == low_pattern && read64(high_phys) == high_pattern);
    goto_priv(PRIV_HS);
    TEST_ASSERT("with both store trigger slots armed on a cross-page access whose low half is writable but whose high half would page-fault, the first access reports breakpoint and leaves both boundary doublewords unchanged",
        first_excpt.triggered == true &&
        first_excpt.cause == CAUSE_BKP &&
        first_preserved
    );

    TEST_SETUP_EXCEPT();
    ai_arch_execute_trigger_store_once(vaddr, 0x1122334455667788ULL);
    ai_arch_trigger_excpt_snapshot_t second_excpt = ai_arch_snapshot_excpt();
    goto_priv(PRIV_M);
    second_preserved = (read64(low_phys) == low_pattern && read64(high_phys) == high_pattern);
    goto_priv(PRIV_HS);
    TEST_ASSERT("with both store trigger slots still armed on that cross-page high-half page-fault path, the second access still reports breakpoint and still leaves both boundary doublewords unchanged",
        second_excpt.triggered == true &&
        second_excpt.cause == CAUSE_BKP &&
        second_preserved
    );
    ai_arch_restore_machine_status(saved_mstatus);

    ai_arch_clear_mcontrol6_slot(0);
    saved_mstatus = ai_arch_enable_machine_breakpoint_delivery();
    goto_priv(PRIV_HS);
    TEST_SETUP_EXCEPT();
    ai_arch_execute_trigger_store_once(vaddr, 0x8877665544332211ULL);
    ai_arch_trigger_excpt_snapshot_t third_excpt = ai_arch_snapshot_excpt();
    goto_priv(PRIV_M);
    third_preserved = (read64(low_phys) == low_pattern && read64(high_phys) == high_pattern);
    goto_priv(PRIV_HS);
    TEST_ASSERT("after repeated dual-slot store breakpoint hits on the cross-page high-half page-fault path, clearing only the execute-PC slot still leaves the address trigger active",
        third_excpt.triggered == true &&
        third_excpt.cause == CAUSE_BKP &&
        third_preserved
    );
    ai_arch_restore_machine_status(saved_mstatus);

    ai_arch_clear_mcontrol6_slot(1);
    goto_priv(PRIV_HS);
    TEST_SETUP_EXCEPT();
    ai_arch_execute_trigger_store_once(vaddr, 0xfedcba9876543210ULL);
    TEST_ASSERT("after clearing both repeated store trigger slots, the same split access falls back to the true second-half store page fault with the second-half start VA in tval",
        excpt.triggered == true &&
        excpt.cause == CAUSE_SPF &&
        excpt.tval == second_half_vaddr
    );

    goto_priv(PRIV_M);
    TEST_ASSERT("before the upper leaf is repaired, repeated trigger hits and the fallback second-half store page fault still leave both boundary backing doublewords unchanged",
        read64(low_phys) == low_pattern &&
        read64(high_phys) == high_pattern
    );
    ai_arch_trigger_set_hs_leaf(high_page, phys_page_base(high_page), PTE_V | PTE_AD | PTE_RWX);
    sfence_vma();

    goto_priv(PRIV_HS);
    TEST_SETUP_EXCEPT();
    ai_arch_execute_trigger_store_once(vaddr, recovery_store);
    TEST_ASSERT("after repeated dual-slot store trigger hits on the cross-page high-half page-fault path, fixing the upper leaf lets the same split store recover and retire normally",
        excpt.triggered == false
    );

    goto_priv(PRIV_M);
    TEST_ASSERT("after the recovered split-store path, the low boundary fragment matches the lower half of the committed payload while preserving the untouched low-page bytes",
        read64(low_phys) == expected_low_after_store
    );
    TEST_ASSERT("after the recovered split-store path, the high boundary fragment matches the upper half of the committed payload while preserving the untouched high-page bytes",
        read64(high_phys) == expected_high_after_store
    );

    ai_arch_clear_mcontrol6_slot(0);
    ai_arch_clear_mcontrol6_slot(1);

    goto_priv(PRIV_HS);
    hspt_init();
    sfence_vma();

    TEST_END();
}

bool ai_arch_store_execute_and_address_trigger_high_half_page_fault_repeated_breakpoint_then_fix_high_half_store_adjacent_preserved_corner()
{
    TEST_START();

    enum test_page low_page = VSRW_GX;
    enum test_page high_page = VSRX_GRWX;
    uintptr_t low_adjacent_phys = phys_page_base(low_page) + PAGE_SIZE - (2 * sizeof(uint64_t));
    uintptr_t low_phys = ai_arch_trigger_page_end_phys(low_page);
    uintptr_t high_phys = phys_page_base(high_page);
    uintptr_t high_adjacent_phys = high_phys + sizeof(uint64_t);
    uintptr_t trigger_pc = (uintptr_t)ai_arch_execute_trigger_store_once;
    uintptr_t vaddr = ai_arch_trigger_prepare_cross_page_high_half_store_pf_alias();
    uintptr_t second_half_vaddr = vaddr + 4U;
    const uint64_t low_adjacent = 0xfaceb00c12344321ULL;
    const uint64_t low_pattern = 0x98a9bacbdcedfe10ULL;
    const uint64_t high_pattern = 0x21436587a9cbed0fULL;
    const uint64_t high_adjacent = 0xabcdef0123456789ULL;
    const uint64_t recovery_store = 0x0123456789abcdefULL;
    const uint64_t expected_low_after_store =
        (low_pattern & 0xffffffffULL) | ((recovery_store & 0xffffffffULL) << 32);
    const uint64_t expected_high_after_store =
        (high_pattern & 0xffffffff00000000ULL) | (recovery_store >> 32);
    bool first_preserved;
    bool second_preserved;
    bool third_preserved;

    goto_priv(PRIV_M);
    write64(low_adjacent_phys, low_adjacent);
    write64(low_phys, low_pattern);
    write64(high_phys, high_pattern);
    write64(high_adjacent_phys, high_adjacent);

    ai_arch_clear_mcontrol6_slot(0);
    ai_arch_clear_mcontrol6_slot(1);
    ai_arch_arm_mcontrol6_slot(0, trigger_pc, AI_ARCH_MCONTROL6_EXECUTE);
    ai_arch_arm_mcontrol6_slot(1, vaddr, AI_ARCH_MCONTROL6_STORE);

    uint64_t saved_mstatus = ai_arch_enable_machine_breakpoint_delivery();
    goto_priv(PRIV_HS);
    TEST_SETUP_EXCEPT();
    ai_arch_execute_trigger_store_once(vaddr, 0xdeadbeefcafef00dULL);
    ai_arch_trigger_excpt_snapshot_t first_excpt = ai_arch_snapshot_excpt();
    goto_priv(PRIV_M);
    first_preserved =
        (read64(low_adjacent_phys) == low_adjacent &&
        read64(low_phys) == low_pattern &&
        read64(high_phys) == high_pattern &&
        read64(high_adjacent_phys) == high_adjacent);
    goto_priv(PRIV_HS);
    TEST_ASSERT("with both store trigger slots armed on a cross-page access whose low half is writable but whose high half would page-fault, the first access reports breakpoint and leaves both boundary and adjacent doublewords unchanged",
        first_excpt.triggered == true &&
        first_excpt.cause == CAUSE_BKP &&
        first_preserved
    );

    TEST_SETUP_EXCEPT();
    ai_arch_execute_trigger_store_once(vaddr, 0x1122334455667788ULL);
    ai_arch_trigger_excpt_snapshot_t second_excpt = ai_arch_snapshot_excpt();
    goto_priv(PRIV_M);
    second_preserved =
        (read64(low_adjacent_phys) == low_adjacent &&
        read64(low_phys) == low_pattern &&
        read64(high_phys) == high_pattern &&
        read64(high_adjacent_phys) == high_adjacent);
    goto_priv(PRIV_HS);
    TEST_ASSERT("with both store trigger slots still armed on that cross-page high-half page-fault path, the second access still reports breakpoint and still leaves both boundary and adjacent doublewords unchanged",
        second_excpt.triggered == true &&
        second_excpt.cause == CAUSE_BKP &&
        second_preserved
    );
    ai_arch_restore_machine_status(saved_mstatus);

    ai_arch_clear_mcontrol6_slot(0);
    saved_mstatus = ai_arch_enable_machine_breakpoint_delivery();
    goto_priv(PRIV_HS);
    TEST_SETUP_EXCEPT();
    ai_arch_execute_trigger_store_once(vaddr, 0x8877665544332211ULL);
    ai_arch_trigger_excpt_snapshot_t third_excpt = ai_arch_snapshot_excpt();
    goto_priv(PRIV_M);
    third_preserved =
        (read64(low_adjacent_phys) == low_adjacent &&
        read64(low_phys) == low_pattern &&
        read64(high_phys) == high_pattern &&
        read64(high_adjacent_phys) == high_adjacent);
    goto_priv(PRIV_HS);
    TEST_ASSERT("after repeated dual-slot store breakpoint hits on the cross-page high-half page-fault path, clearing only the execute-PC slot still leaves the address trigger active without dirtying either adjacent boundary word",
        third_excpt.triggered == true &&
        third_excpt.cause == CAUSE_BKP &&
        third_preserved
    );
    ai_arch_restore_machine_status(saved_mstatus);

    ai_arch_clear_mcontrol6_slot(1);
    goto_priv(PRIV_HS);
    TEST_SETUP_EXCEPT();
    ai_arch_execute_trigger_store_once(vaddr, 0xfedcba9876543210ULL);
    TEST_ASSERT("after clearing both repeated store trigger slots, the same split access falls back to the true second-half store page fault with the second-half start VA in tval in the adjacent-preservation corner",
        excpt.triggered == true &&
        excpt.cause == CAUSE_SPF &&
        excpt.tval == second_half_vaddr
    );

    goto_priv(PRIV_M);
    TEST_ASSERT("before the upper leaf is repaired, repeated trigger hits and the fallback second-half store page fault still leave both boundary and adjacent backing doublewords unchanged",
        read64(low_adjacent_phys) == low_adjacent &&
        read64(low_phys) == low_pattern &&
        read64(high_phys) == high_pattern &&
        read64(high_adjacent_phys) == high_adjacent
    );
    ai_arch_trigger_set_hs_leaf(high_page, phys_page_base(high_page), PTE_V | PTE_AD | PTE_RWX);
    sfence_vma();

    goto_priv(PRIV_HS);
    TEST_SETUP_EXCEPT();
    ai_arch_execute_trigger_store_once(vaddr, recovery_store);
    TEST_ASSERT("after repeated dual-slot store trigger hits on the cross-page high-half page-fault path, fixing the upper leaf lets the same split store recover while keeping both adjacent boundary words untouched",
        excpt.triggered == false
    );

    goto_priv(PRIV_M);
    TEST_ASSERT("the recovered split-store path updates the low boundary fragment as expected while preserving the adjacent low-page word on the repaired trigger corner",
        read64(low_adjacent_phys) == low_adjacent &&
        read64(low_phys) == expected_low_after_store
    );
    TEST_ASSERT("the recovered split-store path updates the high boundary fragment as expected while preserving the adjacent high-page word on the repaired trigger corner",
        read64(high_phys) == expected_high_after_store &&
        read64(high_adjacent_phys) == high_adjacent
    );

    ai_arch_clear_mcontrol6_slot(0);
    ai_arch_clear_mcontrol6_slot(1);

    goto_priv(PRIV_HS);
    hspt_init();
    sfence_vma();

    TEST_END();
}

bool ai_arch_store_execute_and_address_trigger_high_half_page_fault_repeated_breakpoint_then_other_aligned_store_recovers_corner()
{
    TEST_START();

    enum test_page low_page = VSRW_GX;
    enum test_page high_page = VSRX_GRWX;
    uintptr_t low_phys = ai_arch_trigger_page_end_phys(low_page);
    uintptr_t high_phys = phys_page_base(high_page);
    uintptr_t trigger_pc = (uintptr_t)ai_arch_execute_trigger_store_once;
    uintptr_t vaddr = ai_arch_trigger_prepare_cross_page_high_half_store_pf_alias();
    uintptr_t second_half_vaddr = vaddr + 4U;
    uintptr_t recovery_vaddr = hs_page_base(low_page) + 0x100U;
    uintptr_t recovery_phys = phys_page_base(low_page) + 0x100U;
    const uint64_t low_pattern = 0x98a9bacbdcedfe10ULL;
    const uint64_t high_pattern = 0x21436587a9cbed0fULL;
    const uint64_t recovery_initial = 0x2233445566778899ULL;
    const uint64_t recovery_store = 0xfedcba9876543210ULL;
    bool first_preserved;
    bool second_preserved;
    bool third_preserved;

    goto_priv(PRIV_M);
    write64(low_phys, low_pattern);
    write64(high_phys, high_pattern);
    write64(recovery_phys, recovery_initial);

    ai_arch_clear_mcontrol6_slot(0);
    ai_arch_clear_mcontrol6_slot(1);
    ai_arch_arm_mcontrol6_slot(0, trigger_pc, AI_ARCH_MCONTROL6_EXECUTE);
    ai_arch_arm_mcontrol6_slot(1, vaddr, AI_ARCH_MCONTROL6_STORE);

    uint64_t saved_mstatus = ai_arch_enable_machine_breakpoint_delivery();
    goto_priv(PRIV_HS);
    TEST_SETUP_EXCEPT();
    ai_arch_execute_trigger_store_once(vaddr, 0xdeadbeefcafef00dULL);
    ai_arch_trigger_excpt_snapshot_t first_excpt = ai_arch_snapshot_excpt();
    goto_priv(PRIV_M);
    first_preserved = (read64(low_phys) == low_pattern && read64(high_phys) == high_pattern);
    goto_priv(PRIV_HS);
    TEST_ASSERT("with both store trigger slots armed on a cross-page access whose low half is writable but whose high half would page-fault, the first access reports breakpoint and leaves both boundary doublewords unchanged before the unrelated aligned-store recovery probe",
        first_excpt.triggered == true &&
        first_excpt.cause == CAUSE_BKP &&
        first_preserved
    );

    TEST_SETUP_EXCEPT();
    ai_arch_execute_trigger_store_once(vaddr, 0x1122334455667788ULL);
    ai_arch_trigger_excpt_snapshot_t second_excpt = ai_arch_snapshot_excpt();
    goto_priv(PRIV_M);
    second_preserved = (read64(low_phys) == low_pattern && read64(high_phys) == high_pattern);
    goto_priv(PRIV_HS);
    TEST_ASSERT("with both store trigger slots still armed on that cross-page high-half page-fault path, the second access still reports breakpoint and still leaves both boundary doublewords unchanged before the unrelated aligned-store recovery probe",
        second_excpt.triggered == true &&
        second_excpt.cause == CAUSE_BKP &&
        second_preserved
    );
    ai_arch_restore_machine_status(saved_mstatus);

    ai_arch_clear_mcontrol6_slot(0);
    saved_mstatus = ai_arch_enable_machine_breakpoint_delivery();
    goto_priv(PRIV_HS);
    TEST_SETUP_EXCEPT();
    ai_arch_execute_trigger_store_once(vaddr, 0x8877665544332211ULL);
    ai_arch_trigger_excpt_snapshot_t third_excpt = ai_arch_snapshot_excpt();
    goto_priv(PRIV_M);
    third_preserved = (read64(low_phys) == low_pattern && read64(high_phys) == high_pattern);
    goto_priv(PRIV_HS);
    TEST_ASSERT("after repeated dual-slot store breakpoint hits on the cross-page high-half page-fault path, clearing only the execute-PC slot still leaves the address trigger active before the unrelated aligned-store recovery probe",
        third_excpt.triggered == true &&
        third_excpt.cause == CAUSE_BKP &&
        third_preserved
    );
    ai_arch_restore_machine_status(saved_mstatus);

    ai_arch_clear_mcontrol6_slot(1);
    goto_priv(PRIV_HS);
    TEST_SETUP_EXCEPT();
    ai_arch_execute_trigger_store_once(vaddr, 0xdeadbeefcafef00dULL);
    TEST_ASSERT("after clearing both repeated store trigger slots, the same split access falls back to the true second-half store page fault with the second-half start VA in tval before the unrelated aligned-store recovery probe",
        excpt.triggered == true &&
        excpt.cause == CAUSE_SPF &&
        excpt.tval == second_half_vaddr
    );

    goto_priv(PRIV_M);
    TEST_ASSERT("before the unrelated aligned-store recovery probe, repeated trigger hits and the fallback second-half store page fault still leave both boundary backing doublewords unchanged",
        read64(low_phys) == low_pattern &&
        read64(high_phys) == high_pattern &&
        read64(recovery_phys) == recovery_initial
    );

    goto_priv(PRIV_HS);
    TEST_SETUP_EXCEPT();
    ai_arch_execute_trigger_store_once(recovery_vaddr, recovery_store);
    TEST_ASSERT("after repeated dual-slot store trigger hits and the fallback second-half store page fault, another aligned store in the same low page still retires normally without repairing the faulting high half",
        excpt.triggered == false
    );

    TEST_SETUP_EXCEPT();
    ai_arch_execute_trigger_store_once(vaddr, 0x0fedcba987654321ULL);
    TEST_ASSERT("after the unrelated aligned-store recovery probe, the original split-store shape still reports the same second-half store page fault until the high-half leaf is repaired",
        excpt.triggered == true &&
        excpt.cause == CAUSE_SPF &&
        excpt.tval == second_half_vaddr
    );

    goto_priv(PRIV_M);
    TEST_ASSERT("the repeated dual-slot store trigger+high-half-page-fault corner leaves the boundary backing doublewords untouched while committing only the unrelated aligned-store recovery payload",
        read64(low_phys) == low_pattern &&
        read64(high_phys) == high_pattern &&
        read64(recovery_phys) == recovery_store
    );

    ai_arch_clear_mcontrol6_slot(0);
    ai_arch_clear_mcontrol6_slot(1);

    goto_priv(PRIV_HS);
    hspt_init();
    sfence_vma();

    TEST_END();
}

bool ai_arch_store_execute_and_address_trigger_high_half_page_fault_repeated_breakpoint_then_other_aligned_store_adjacent_preserved_corner()
{
    TEST_START();

    enum test_page low_page = VSRW_GX;
    enum test_page high_page = VSRX_GRWX;
    uintptr_t low_adjacent_phys = phys_page_base(low_page) + PAGE_SIZE - (2 * sizeof(uint64_t));
    uintptr_t low_phys = ai_arch_trigger_page_end_phys(low_page);
    uintptr_t high_phys = phys_page_base(high_page);
    uintptr_t high_adjacent_phys = high_phys + sizeof(uint64_t);
    uintptr_t trigger_pc = (uintptr_t)ai_arch_execute_trigger_store_once;
    uintptr_t vaddr = ai_arch_trigger_prepare_cross_page_high_half_store_pf_alias();
    uintptr_t second_half_vaddr = vaddr + 4U;
    uintptr_t recovery_vaddr = hs_page_base(low_page) + 0x100U;
    uintptr_t recovery_adjacent_vaddr = recovery_vaddr + sizeof(uint64_t);
    uintptr_t recovery_phys = phys_page_base(low_page) + 0x100U;
    uintptr_t recovery_adjacent_phys = recovery_phys + sizeof(uint64_t);
    const uint64_t low_adjacent = 0xfaceb00c12344321ULL;
    const uint64_t low_pattern = 0x98a9bacbdcedfe10ULL;
    const uint64_t high_pattern = 0x21436587a9cbed0fULL;
    const uint64_t high_adjacent = 0xabcdef0123456789ULL;
    const uint64_t recovery_initial = 0x2233445566778899ULL;
    const uint64_t recovery_store = 0xfedcba9876543210ULL;
    const uint64_t recovery_adjacent = 0x0123456789abcdefULL;
    bool first_preserved;
    bool second_preserved;
    bool third_preserved;

    goto_priv(PRIV_M);
    write64(low_adjacent_phys, low_adjacent);
    write64(low_phys, low_pattern);
    write64(high_phys, high_pattern);
    write64(high_adjacent_phys, high_adjacent);
    write64(recovery_phys, recovery_initial);
    write64(recovery_adjacent_phys, recovery_adjacent);

    ai_arch_clear_mcontrol6_slot(0);
    ai_arch_clear_mcontrol6_slot(1);
    ai_arch_arm_mcontrol6_slot(0, trigger_pc, AI_ARCH_MCONTROL6_EXECUTE);
    ai_arch_arm_mcontrol6_slot(1, vaddr, AI_ARCH_MCONTROL6_STORE);

    uint64_t saved_mstatus = ai_arch_enable_machine_breakpoint_delivery();
    goto_priv(PRIV_HS);
    TEST_SETUP_EXCEPT();
    ai_arch_execute_trigger_store_once(vaddr, 0xdeadbeefcafef00dULL);
    ai_arch_trigger_excpt_snapshot_t first_excpt = ai_arch_snapshot_excpt();
    goto_priv(PRIV_M);
    first_preserved =
        (read64(low_adjacent_phys) == low_adjacent &&
        read64(low_phys) == low_pattern &&
        read64(high_phys) == high_pattern &&
        read64(high_adjacent_phys) == high_adjacent);
    goto_priv(PRIV_HS);
    TEST_ASSERT("with both store trigger slots armed on a cross-page access whose low half is writable but whose high half would page-fault, the first access reports breakpoint and leaves both boundary and adjacent doublewords unchanged before the adjacent-preservation aligned-store recovery probe",
        first_excpt.triggered == true &&
        first_excpt.cause == CAUSE_BKP &&
        first_preserved
    );

    TEST_SETUP_EXCEPT();
    ai_arch_execute_trigger_store_once(vaddr, 0x1122334455667788ULL);
    ai_arch_trigger_excpt_snapshot_t second_excpt = ai_arch_snapshot_excpt();
    goto_priv(PRIV_M);
    second_preserved =
        (read64(low_adjacent_phys) == low_adjacent &&
        read64(low_phys) == low_pattern &&
        read64(high_phys) == high_pattern &&
        read64(high_adjacent_phys) == high_adjacent);
    goto_priv(PRIV_HS);
    TEST_ASSERT("with both store trigger slots still armed on that cross-page high-half page-fault path, the second access still reports breakpoint and still leaves both boundary and adjacent doublewords unchanged before the adjacent-preservation aligned-store recovery probe",
        second_excpt.triggered == true &&
        second_excpt.cause == CAUSE_BKP &&
        second_preserved
    );
    ai_arch_restore_machine_status(saved_mstatus);

    ai_arch_clear_mcontrol6_slot(0);
    saved_mstatus = ai_arch_enable_machine_breakpoint_delivery();
    goto_priv(PRIV_HS);
    TEST_SETUP_EXCEPT();
    ai_arch_execute_trigger_store_once(vaddr, 0x8877665544332211ULL);
    ai_arch_trigger_excpt_snapshot_t third_excpt = ai_arch_snapshot_excpt();
    goto_priv(PRIV_M);
    third_preserved =
        (read64(low_adjacent_phys) == low_adjacent &&
        read64(low_phys) == low_pattern &&
        read64(high_phys) == high_pattern &&
        read64(high_adjacent_phys) == high_adjacent);
    goto_priv(PRIV_HS);
    TEST_ASSERT("after repeated dual-slot store breakpoint hits on the cross-page high-half page-fault path, clearing only the execute-PC slot still leaves the address trigger active without dirtying either adjacent boundary word before the adjacent-preservation aligned-store recovery probe",
        third_excpt.triggered == true &&
        third_excpt.cause == CAUSE_BKP &&
        third_preserved
    );
    ai_arch_restore_machine_status(saved_mstatus);

    ai_arch_clear_mcontrol6_slot(1);
    goto_priv(PRIV_HS);
    TEST_SETUP_EXCEPT();
    ai_arch_execute_trigger_store_once(vaddr, 0xdeadbeefcafef00dULL);
    TEST_ASSERT("after clearing both repeated store trigger slots, the same split access falls back to the true second-half store page fault with the second-half start VA in tval before the adjacent-preservation aligned-store recovery probe",
        excpt.triggered == true &&
        excpt.cause == CAUSE_SPF &&
        excpt.tval == second_half_vaddr
    );

    goto_priv(PRIV_M);
    TEST_ASSERT("before the adjacent-preservation aligned-store recovery probe, repeated trigger hits and the fallback second-half store page fault still leave both boundary doublewords, their adjacent neighbors, and the aligned recovery region untouched",
        read64(low_adjacent_phys) == low_adjacent &&
        read64(low_phys) == low_pattern &&
        read64(high_phys) == high_pattern &&
        read64(high_adjacent_phys) == high_adjacent &&
        read64(recovery_phys) == recovery_initial &&
        read64(recovery_adjacent_phys) == recovery_adjacent
    );

    goto_priv(PRIV_HS);
    TEST_SETUP_EXCEPT();
    ai_arch_execute_trigger_store_once(recovery_vaddr, recovery_store);
    TEST_ASSERT("after repeated dual-slot store trigger hits and the fallback second-half store page fault, another aligned store in the same low page still retires normally without repairing the faulting high half in the adjacent-preservation corner",
        excpt.triggered == false
    );

    TEST_SETUP_EXCEPT();
    uint64_t observed_adjacent = ai_arch_execute_trigger_load_once(recovery_adjacent_vaddr);
    TEST_ASSERT("the unrelated aligned-store recovery probe does not disturb the adjacent low-page word, which still reads back its original payload in the adjacent-preservation corner",
        excpt.triggered == false &&
        observed_adjacent == recovery_adjacent
    );

    TEST_SETUP_EXCEPT();
    ai_arch_execute_trigger_store_once(vaddr, 0x0fedcba987654321ULL);
    TEST_ASSERT("after the adjacent-preservation aligned-store recovery probe, the original split-store shape still reports the same second-half store page fault until the high-half leaf is repaired",
        excpt.triggered == true &&
        excpt.cause == CAUSE_SPF &&
        excpt.tval == second_half_vaddr
    );

    goto_priv(PRIV_M);
    TEST_ASSERT("the repeated dual-slot store trigger+high-half-page-fault corner leaves both boundary doublewords, their adjacent neighbors, and the aligned recovery-region adjacent word untouched while committing only the unrelated aligned-store payload",
        read64(low_adjacent_phys) == low_adjacent &&
        read64(low_phys) == low_pattern &&
        read64(high_phys) == high_pattern &&
        read64(high_adjacent_phys) == high_adjacent &&
        read64(recovery_phys) == recovery_store &&
        read64(recovery_adjacent_phys) == recovery_adjacent
    );

    ai_arch_clear_mcontrol6_slot(0);
    ai_arch_clear_mcontrol6_slot(1);

    goto_priv(PRIV_HS);
    hspt_init();
    sfence_vma();

    TEST_END();
}

bool ai_arch_load_trigger_pbmt_nc_over_io_misaligned_priority_corner()
{
    TEST_START();

    uintptr_t vaddr = ai_arch_trigger_prepare_nc_page_over_io_region() + 7U;

    ai_arch_arm_mcontrol6_0(vaddr, AI_ARCH_MCONTROL6_LOAD);
    uint64_t saved_mstatus = ai_arch_enable_machine_breakpoint_delivery();

    goto_priv(PRIV_HS);
    TEST_SETUP_EXCEPT();
    ld(vaddr);
    if (!(excpt.triggered == true && excpt.cause == CAUSE_BKP)) {
        ai_arch_log_unexpected_trigger_result("unexpected load address-trigger+PBMT-NC-over-IO misalign priority result");
    }
    TEST_ASSERT("a load address trigger takes priority when the same HS PBMT=NC-over-IO misaligned load would otherwise report load misaligned",
        excpt.triggered == true &&
        excpt.cause == CAUSE_BKP
    );
    ai_arch_restore_machine_status(saved_mstatus);
    ai_arch_clear_mcontrol6_0();

    goto_priv(PRIV_HS);
    TEST_SETUP_EXCEPT();
    ld(vaddr);
    TEST_ASSERT("after clearing the load address trigger, the same HS PBMT=NC-over-IO misaligned load falls back to load misaligned with the raw VA in tval",
        excpt.triggered == true &&
        excpt.cause == CAUSE_LAM &&
        excpt.tval == vaddr
    );

    ai_arch_trigger_reset_svpbmt_pma_env();

    TEST_END();
}

bool ai_arch_load_execute_trigger_pbmt_nc_over_io_misaligned_priority_corner()
{
    TEST_START();

    uintptr_t trigger_pc = (uintptr_t)ai_arch_execute_trigger_load_once;
    uintptr_t vaddr = ai_arch_trigger_prepare_nc_page_over_io_region() + 7U;

    ai_arch_arm_mcontrol6_0(trigger_pc, AI_ARCH_MCONTROL6_EXECUTE);
    uint64_t saved_mstatus = ai_arch_enable_machine_breakpoint_delivery();

    goto_priv(PRIV_HS);
    TEST_SETUP_EXCEPT();
    ai_arch_execute_trigger_load_once(vaddr);
    if (!(excpt.triggered == true && excpt.cause == CAUSE_BKP)) {
        ai_arch_log_unexpected_trigger_result("unexpected load execute-PC trigger+PBMT-NC-over-IO misalign priority result");
    }
    TEST_ASSERT("a load execute-PC trigger takes priority when the same HS PBMT=NC-over-IO misaligned load would otherwise report load misaligned",
        excpt.triggered == true &&
        excpt.cause == CAUSE_BKP
    );
    ai_arch_restore_machine_status(saved_mstatus);
    ai_arch_clear_mcontrol6_0();

    goto_priv(PRIV_HS);
    TEST_SETUP_EXCEPT();
    ai_arch_execute_trigger_load_once(vaddr);
    TEST_ASSERT("after clearing the load execute-PC trigger, the same HS PBMT=NC-over-IO misaligned load falls back to load misaligned with the raw VA in tval",
        excpt.triggered == true &&
        excpt.cause == CAUSE_LAM &&
        excpt.tval == vaddr
    );

    ai_arch_trigger_reset_svpbmt_pma_env();

    TEST_END();
}

bool ai_arch_load_execute_trigger_pbmt_nc_over_io_misaligned_repeated_breakpoint_then_fault_corner()
{
    TEST_START();

    uintptr_t trigger_pc = (uintptr_t)ai_arch_execute_trigger_load_once;
    uintptr_t vaddr = ai_arch_trigger_prepare_nc_page_over_io_region() + 7U;

    ai_arch_arm_mcontrol6_0(trigger_pc, AI_ARCH_MCONTROL6_EXECUTE);
    uint64_t saved_mstatus = ai_arch_enable_machine_breakpoint_delivery();

    goto_priv(PRIV_HS);
    TEST_SETUP_EXCEPT();
    ai_arch_execute_trigger_load_once(vaddr);
    TEST_ASSERT("with a load execute-PC trigger armed on the HS PBMT=NC-over-IO misaligned load, the first access reports breakpoint",
        excpt.triggered == true &&
        excpt.cause == CAUSE_BKP
    );

    TEST_SETUP_EXCEPT();
    ai_arch_execute_trigger_load_once(vaddr);
    TEST_ASSERT("with the same load execute-PC trigger still armed on that HS PBMT=NC-over-IO misaligned load, the second access still reports breakpoint",
        excpt.triggered == true &&
        excpt.cause == CAUSE_BKP
    );
    ai_arch_restore_machine_status(saved_mstatus);
    ai_arch_clear_mcontrol6_0();

    goto_priv(PRIV_HS);
    TEST_SETUP_EXCEPT();
    ai_arch_execute_trigger_load_once(vaddr);
    TEST_ASSERT("after clearing the repeated load execute-PC trigger, the same HS PBMT=NC-over-IO misaligned load falls back to load misaligned with the raw VA in tval",
        excpt.triggered == true &&
        excpt.cause == CAUSE_LAM &&
        excpt.tval == vaddr
    );

    ai_arch_trigger_reset_svpbmt_pma_env();

    TEST_END();
}

bool ai_arch_load_execute_and_address_trigger_pbmt_nc_over_io_misaligned_priority_corner()
{
    TEST_START();

    uintptr_t trigger_pc = (uintptr_t)ai_arch_execute_trigger_load_once;
    uintptr_t vaddr = ai_arch_trigger_prepare_nc_page_over_io_region() + 7U;

    ai_arch_clear_mcontrol6_slot(0);
    ai_arch_clear_mcontrol6_slot(1);
    ai_arch_arm_mcontrol6_slot(0, trigger_pc, AI_ARCH_MCONTROL6_EXECUTE);
    ai_arch_arm_mcontrol6_slot(1, vaddr, AI_ARCH_MCONTROL6_LOAD);

    uint64_t saved_mstatus = ai_arch_enable_machine_breakpoint_delivery();
    goto_priv(PRIV_HS);
    TEST_SETUP_EXCEPT();
    ai_arch_execute_trigger_load_once(vaddr);
    TEST_ASSERT("when both execute-PC and address triggers match the same HS PBMT=NC-over-IO misaligned load, breakpoint still wins before the load misaligned classification is observed",
        excpt.triggered == true &&
        excpt.cause == CAUSE_BKP
    );
    ai_arch_restore_machine_status(saved_mstatus);

    ai_arch_clear_mcontrol6_slot(0);
    saved_mstatus = ai_arch_enable_machine_breakpoint_delivery();
    goto_priv(PRIV_HS);
    TEST_SETUP_EXCEPT();
    ai_arch_execute_trigger_load_once(vaddr);
    TEST_ASSERT("after clearing only the execute-PC trigger, the remaining load-address trigger still reports breakpoint on the same HS PBMT=NC-over-IO misaligned load",
        excpt.triggered == true &&
        excpt.cause == CAUSE_BKP
    );
    ai_arch_restore_machine_status(saved_mstatus);

    ai_arch_clear_mcontrol6_slot(1);
    goto_priv(PRIV_HS);
    TEST_SETUP_EXCEPT();
    ai_arch_execute_trigger_load_once(vaddr);
    TEST_ASSERT("after clearing both load trigger slots, the same HS PBMT=NC-over-IO misaligned load falls back to load misaligned with the raw VA in tval",
        excpt.triggered == true &&
        excpt.cause == CAUSE_LAM &&
        excpt.tval == vaddr
    );

    ai_arch_trigger_reset_svpbmt_pma_env();

    TEST_END();
}

bool ai_arch_load_execute_and_address_trigger_pbmt_nc_over_io_misaligned_repeated_breakpoint_then_fault_corner()
{
    TEST_START();

    uintptr_t trigger_pc = (uintptr_t)ai_arch_execute_trigger_load_once;
    uintptr_t vaddr = ai_arch_trigger_prepare_nc_page_over_io_region() + 7U;

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

    TEST_END();
}

bool ai_arch_load_execute_and_address_trigger_pbmt_nc_over_io_misaligned_repeated_breakpoint_then_fault_adjacent_preserved_corner()
{
    TEST_START();

    uintptr_t trigger_pc = (uintptr_t)ai_arch_execute_trigger_load_once;
    uintptr_t vaddr = ai_arch_trigger_prepare_nc_page_over_io_region() + 7U;

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

    TEST_END();
}
