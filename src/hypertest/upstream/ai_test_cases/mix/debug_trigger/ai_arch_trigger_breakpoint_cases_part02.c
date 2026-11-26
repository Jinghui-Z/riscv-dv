#include "ai_arch_trigger_breakpoint_common.h"

bool ai_arch_load_trigger_high_half_page_fault_repeated_breakpoint_then_other_aligned_load_recovers_corner()
{
    TEST_START();

    enum test_page low_page = VSR_GX;
    enum test_page high_page = VSW_GI;
    uintptr_t low_phys = ai_arch_trigger_page_end_phys(low_page);
    uintptr_t high_phys = phys_page_base(high_page);
    uintptr_t vaddr = ai_arch_trigger_prepare_cross_page_high_half_load_pf_alias();
    uintptr_t second_half_vaddr = vaddr + 4U;
    uintptr_t recovery_vaddr = hs_page_base(low_page) + 0x160U;
    uintptr_t recovery_phys = phys_page_base(low_page) + 0x160U;
    const uint64_t low_pattern = 0x5465768798a9bacbULL;
    const uint64_t high_pattern = 0xdcfe1032547698baULL;
    const uint64_t recovery_expected = 0x89abcdef01234567ULL;
    bool first_preserved;
    bool second_preserved;

    goto_priv(PRIV_M);
    write64(low_phys, low_pattern);
    write64(high_phys, high_pattern);
    write64(recovery_phys, recovery_expected);

    ai_arch_arm_mcontrol6_0(vaddr, AI_ARCH_MCONTROL6_LOAD);
    uint64_t saved_mstatus = ai_arch_enable_machine_breakpoint_delivery();

    goto_priv(PRIV_HS);
    TEST_SETUP_EXCEPT();
    ld(vaddr);
    ai_arch_trigger_excpt_snapshot_t first_excpt = ai_arch_snapshot_excpt();
    goto_priv(PRIV_M);
    first_preserved = (read64(low_phys) == low_pattern && read64(high_phys) == high_pattern);
    goto_priv(PRIV_HS);
    TEST_ASSERT("with a load address trigger armed on a cross-page access whose low half is cacheable but whose high half would page-fault, the first access reports breakpoint and leaves both boundary doublewords unchanged before the unrelated aligned-load recovery probe",
        first_excpt.triggered == true &&
        first_excpt.cause == CAUSE_BKP &&
        first_preserved
    );

    TEST_SETUP_EXCEPT();
    ld(vaddr);
    ai_arch_trigger_excpt_snapshot_t second_excpt = ai_arch_snapshot_excpt();
    goto_priv(PRIV_M);
    second_preserved = (read64(low_phys) == low_pattern && read64(high_phys) == high_pattern);
    goto_priv(PRIV_HS);
    TEST_ASSERT("with the same load address trigger still armed on that cross-page high-half page-fault path, the second access still reports breakpoint and still leaves both boundary doublewords unchanged before the unrelated aligned-load recovery probe",
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
    ld(vaddr);
    TEST_ASSERT("after clearing the repeated load address trigger, the same split access falls back to the true second-half load page fault with the second-half start VA in tval before the unrelated aligned-load recovery probe",
        excpt.triggered == true &&
        excpt.cause == CAUSE_LPF &&
        excpt.tval == second_half_vaddr
    );

    goto_priv(PRIV_M);
    TEST_ASSERT("before the unrelated aligned-load recovery probe, repeated address-trigger hits and the fallback second-half load page fault still leave both boundary backing doublewords unchanged",
        read64(low_phys) == low_pattern &&
        read64(high_phys) == high_pattern &&
        read64(recovery_phys) == recovery_expected
    );

    goto_priv(PRIV_HS);
    TEST_SETUP_EXCEPT();
    uint64_t recovery_value = ld(recovery_vaddr);
    TEST_ASSERT("after repeated load address-trigger breakpoint hits and the fallback second-half load page fault, another aligned load in the same low page still recovers normally without repairing the faulting high half",
        excpt.triggered == false &&
        recovery_value == recovery_expected
    );

    TEST_SETUP_EXCEPT();
    ld(vaddr);
    TEST_ASSERT("after the unrelated aligned-load recovery probe, the original split-load shape still reports the same second-half load page fault until the high-half leaf is repaired",
        excpt.triggered == true &&
        excpt.cause == CAUSE_LPF &&
        excpt.tval == second_half_vaddr
    );

    goto_priv(PRIV_M);
    TEST_ASSERT("the repeated load address-trigger plus high-half-page-fault corner leaves the boundary backing doublewords untouched while preserving the unrelated aligned-load recovery payload",
        read64(low_phys) == low_pattern &&
        read64(high_phys) == high_pattern &&
        read64(recovery_phys) == recovery_expected
    );

    goto_priv(PRIV_HS);
    hspt_init();
    sfence_vma();

    TEST_END();
}

bool ai_arch_load_trigger_high_half_page_fault_repeated_breakpoint_then_other_aligned_load_adjacent_preserved_corner()
{
    TEST_START();

    enum test_page low_page = VSR_GX;
    enum test_page high_page = VSW_GI;
    uintptr_t low_adjacent_phys = phys_page_base(low_page) + PAGE_SIZE - (2 * sizeof(uint64_t));
    uintptr_t low_phys = ai_arch_trigger_page_end_phys(low_page);
    uintptr_t high_phys = phys_page_base(high_page);
    uintptr_t high_adjacent_phys = high_phys + sizeof(uint64_t);
    uintptr_t vaddr = ai_arch_trigger_prepare_cross_page_high_half_load_pf_alias();
    uintptr_t second_half_vaddr = vaddr + 4U;
    uintptr_t recovery_vaddr = hs_page_base(low_page) + 0x160U;
    uintptr_t recovery_adjacent_vaddr = recovery_vaddr + sizeof(uint64_t);
    uintptr_t recovery_phys = phys_page_base(low_page) + 0x160U;
    uintptr_t recovery_adjacent_phys = recovery_phys + sizeof(uint64_t);
    const uint64_t low_adjacent = 0x33aa55aa11cc77ccULL;
    const uint64_t low_pattern = 0x5465768798a9bacbULL;
    const uint64_t high_pattern = 0xdcfe1032547698baULL;
    const uint64_t high_adjacent = 0xaa77cc1133aa55ccULL;
    const uint64_t recovery_expected = 0x89abcdef01234567ULL;
    const uint64_t recovery_adjacent = 0x76543210fedcba98ULL;
    bool first_preserved;
    bool second_preserved;

    goto_priv(PRIV_M);
    write64(low_adjacent_phys, low_adjacent);
    write64(low_phys, low_pattern);
    write64(high_phys, high_pattern);
    write64(high_adjacent_phys, high_adjacent);
    write64(recovery_phys, recovery_expected);
    write64(recovery_adjacent_phys, recovery_adjacent);

    ai_arch_arm_mcontrol6_0(vaddr, AI_ARCH_MCONTROL6_LOAD);
    uint64_t saved_mstatus = ai_arch_enable_machine_breakpoint_delivery();

    goto_priv(PRIV_HS);
    TEST_SETUP_EXCEPT();
    ld(vaddr);
    ai_arch_trigger_excpt_snapshot_t first_excpt = ai_arch_snapshot_excpt();
    goto_priv(PRIV_M);
    first_preserved =
        (read64(low_adjacent_phys) == low_adjacent &&
        read64(low_phys) == low_pattern &&
        read64(high_phys) == high_pattern &&
        read64(high_adjacent_phys) == high_adjacent);
    goto_priv(PRIV_HS);
    TEST_ASSERT("with a load address trigger armed on a cross-page access whose low half is cacheable but whose high half would page-fault, the first access reports breakpoint and leaves both boundary and adjacent doublewords unchanged before the adjacent-preservation recovery probe",
        first_excpt.triggered == true &&
        first_excpt.cause == CAUSE_BKP &&
        first_preserved
    );

    TEST_SETUP_EXCEPT();
    ld(vaddr);
    ai_arch_trigger_excpt_snapshot_t second_excpt = ai_arch_snapshot_excpt();
    goto_priv(PRIV_M);
    second_preserved =
        (read64(low_adjacent_phys) == low_adjacent &&
        read64(low_phys) == low_pattern &&
        read64(high_phys) == high_pattern &&
        read64(high_adjacent_phys) == high_adjacent);
    goto_priv(PRIV_HS);
    TEST_ASSERT("with the same load address trigger still armed on that cross-page high-half page-fault path, the second access still reports breakpoint and still leaves both boundary and adjacent doublewords unchanged before the adjacent-preservation recovery probe",
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
    ld(vaddr);
    TEST_ASSERT("after clearing the repeated load address trigger, the same split access falls back to the true second-half load page fault with the second-half start VA in tval before the adjacent-preservation recovery probe",
        excpt.triggered == true &&
        excpt.cause == CAUSE_LPF &&
        excpt.tval == second_half_vaddr
    );

    goto_priv(PRIV_M);
    TEST_ASSERT("before the adjacent-preservation recovery probe, repeated address-trigger hits and the fallback second-half load page fault still leave both boundary doublewords, their adjacent neighbors, and the aligned recovery region untouched",
        read64(low_adjacent_phys) == low_adjacent &&
        read64(low_phys) == low_pattern &&
        read64(high_phys) == high_pattern &&
        read64(high_adjacent_phys) == high_adjacent &&
        read64(recovery_phys) == recovery_expected &&
        read64(recovery_adjacent_phys) == recovery_adjacent
    );

    goto_priv(PRIV_HS);
    TEST_SETUP_EXCEPT();
    uint64_t recovery_value = ld(recovery_vaddr);
    TEST_ASSERT("after repeated load address-trigger breakpoint hits and the fallback second-half load page fault, another aligned load in the same low page still recovers normally without repairing the faulting high half in the adjacent-preservation corner",
        excpt.triggered == false &&
        recovery_value == recovery_expected
    );

    TEST_SETUP_EXCEPT();
    uint64_t observed_adjacent = ld(recovery_adjacent_vaddr);
    TEST_ASSERT("the unrelated aligned-load recovery probe does not disturb the adjacent low-page word, which still reads back its original payload in the adjacent-preservation corner",
        excpt.triggered == false &&
        observed_adjacent == recovery_adjacent
    );

    TEST_SETUP_EXCEPT();
    ld(vaddr);
    TEST_ASSERT("after the adjacent-preservation recovery probe, the original split-load shape still reports the same second-half load page fault until the high-half leaf is repaired",
        excpt.triggered == true &&
        excpt.cause == CAUSE_LPF &&
        excpt.tval == second_half_vaddr
    );

    goto_priv(PRIV_M);
    TEST_ASSERT("the repeated load address-trigger plus high-half-page-fault corner leaves both boundary doublewords, their adjacent neighbors, and the aligned recovery region untouched while preserving the unrelated aligned-load payload",
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

bool ai_arch_store_trigger_page_fault_priority()
{
    TEST_START();

    uintptr_t pf_vaddr = hs_page_base(VSI_GI);

    ai_arch_prepare_hs_fault_page();
    ai_arch_arm_mcontrol6_0(pf_vaddr, AI_ARCH_MCONTROL6_STORE);
    uint64_t saved_mstatus = ai_arch_enable_machine_breakpoint_delivery();

    goto_priv(PRIV_HS);
    TEST_SETUP_EXCEPT();
    sd(pf_vaddr, 0xa1b2c3d4e5f60718ULL);
    if (!(excpt.triggered == true && excpt.cause == CAUSE_BKP)) {
        ai_arch_log_unexpected_trigger_result("unexpected store trigger+page-fault priority result");
    }
    TEST_ASSERT("a store address trigger takes priority when the same HS store would otherwise raise page fault",
        excpt.triggered == true &&
        excpt.cause == CAUSE_BKP
    );
    ai_arch_restore_machine_status(saved_mstatus);
    ai_arch_clear_mcontrol6_0();

    ai_arch_prepare_hs_fault_page();
    goto_priv(PRIV_HS);
    TEST_SETUP_EXCEPT();
    sd(pf_vaddr, 0x0fedcba987654321ULL);
    TEST_ASSERT("after clearing the trigger, the same HS store reports store page fault with the original faulting VA",
        excpt.triggered == true &&
        excpt.cause == CAUSE_SPF &&
        excpt.tval == pf_vaddr
    );

    TEST_END();
}

bool ai_arch_store_execute_trigger_page_fault_priority()
{
    TEST_START();

    uintptr_t trigger_pc = (uintptr_t)ai_arch_execute_trigger_store_once;
    uintptr_t pf_vaddr = hs_page_base(VSI_GI);

    ai_arch_prepare_hs_fault_page();
    ai_arch_arm_mcontrol6_0(trigger_pc, AI_ARCH_MCONTROL6_EXECUTE);
    uint64_t saved_mstatus = ai_arch_enable_machine_breakpoint_delivery();

    goto_priv(PRIV_HS);
    TEST_SETUP_EXCEPT();
    ai_arch_execute_trigger_store_once(pf_vaddr, 0xa1b2c3d4e5f60718ULL);
    if (!(excpt.triggered == true && excpt.cause == CAUSE_BKP)) {
        ai_arch_log_unexpected_trigger_result("unexpected store execute-PC trigger+page-fault priority result");
    }
    TEST_ASSERT("a store execute-PC trigger takes priority when the same HS store would otherwise raise page fault",
        excpt.triggered == true &&
        excpt.cause == CAUSE_BKP
    );
    ai_arch_restore_machine_status(saved_mstatus);
    ai_arch_clear_mcontrol6_0();

    ai_arch_prepare_hs_fault_page();
    goto_priv(PRIV_HS);
    TEST_SETUP_EXCEPT();
    ai_arch_execute_trigger_store_once(pf_vaddr, 0x0fedcba987654321ULL);
    TEST_ASSERT("after clearing the execute-PC trigger, the same HS store reports store page fault with the original faulting VA",
        excpt.triggered == true &&
        excpt.cause == CAUSE_SPF &&
        excpt.tval == pf_vaddr
    );

    TEST_END();
}

bool ai_arch_store_execute_trigger_page_fault_repeated_breakpoint_then_fault_corner()
{
    TEST_START();

    uintptr_t trigger_pc = (uintptr_t)ai_arch_execute_trigger_store_once;
    uintptr_t pf_vaddr = hs_page_base(VSI_GI);

    ai_arch_prepare_hs_fault_page();
    ai_arch_arm_mcontrol6_0(trigger_pc, AI_ARCH_MCONTROL6_EXECUTE);
    uint64_t saved_mstatus = ai_arch_enable_machine_breakpoint_delivery();

    goto_priv(PRIV_HS);
    TEST_SETUP_EXCEPT();
    ai_arch_execute_trigger_store_once(pf_vaddr, 0xa1b2c3d4e5f60718ULL);
    TEST_ASSERT("with a store execute-PC trigger armed on an HS page-faulting VA, the first access reports breakpoint before the page fault is observed",
        excpt.triggered == true &&
        excpt.cause == CAUSE_BKP
    );

    ai_arch_prepare_hs_fault_page();
    goto_priv(PRIV_HS);
    TEST_SETUP_EXCEPT();
    ai_arch_execute_trigger_store_once(pf_vaddr, 0x8877665544332211ULL);
    TEST_ASSERT("with the same store execute-PC trigger still armed on that HS page-faulting VA, the second access still reports breakpoint before the page fault is observed",
        excpt.triggered == true &&
        excpt.cause == CAUSE_BKP
    );
    ai_arch_restore_machine_status(saved_mstatus);
    ai_arch_clear_mcontrol6_0();

    ai_arch_prepare_hs_fault_page();
    goto_priv(PRIV_HS);
    TEST_SETUP_EXCEPT();
    ai_arch_execute_trigger_store_once(pf_vaddr, 0x0fedcba987654321ULL);
    TEST_ASSERT("after clearing the repeated store execute-PC trigger, the same HS access falls back to store page fault with the original faulting VA",
        excpt.triggered == true &&
        excpt.cause == CAUSE_SPF &&
        excpt.tval == pf_vaddr
    );

    TEST_END();
}

bool ai_arch_store_execute_trigger_high_half_page_fault_repeated_breakpoint_then_other_aligned_store_recovers_corner()
{
    TEST_START();

    enum test_page low_page = VSRW_GX;
    enum test_page high_page = VSRX_GRWX;
    uintptr_t low_phys = ai_arch_trigger_page_end_phys(low_page);
    uintptr_t high_phys = phys_page_base(high_page);
    uintptr_t trigger_pc = (uintptr_t)ai_arch_execute_trigger_store_once;
    uintptr_t vaddr = ai_arch_trigger_prepare_cross_page_high_half_store_pf_alias();
    uintptr_t second_half_vaddr = vaddr + 4U;
    uintptr_t recovery_vaddr = hs_page_base(low_page) + 0x140U;
    uintptr_t recovery_phys = phys_page_base(low_page) + 0x140U;
    const uint64_t low_pattern = 0x98a9bacbdcedfe10ULL;
    const uint64_t high_pattern = 0x21436587a9cbed0fULL;
    const uint64_t recovery_initial = 0x1021324354657687ULL;
    const uint64_t recovery_store = 0x89abcdef01234567ULL;
    bool first_preserved;
    bool second_preserved;

    goto_priv(PRIV_M);
    write64(low_phys, low_pattern);
    write64(high_phys, high_pattern);
    write64(recovery_phys, recovery_initial);

    ai_arch_arm_mcontrol6_0(trigger_pc, AI_ARCH_MCONTROL6_EXECUTE);
    uint64_t saved_mstatus = ai_arch_enable_machine_breakpoint_delivery();

    goto_priv(PRIV_HS);
    TEST_SETUP_EXCEPT();
    ai_arch_execute_trigger_store_once(vaddr, 0xa1b2c3d4e5f60718ULL);
    ai_arch_trigger_excpt_snapshot_t first_excpt = ai_arch_snapshot_excpt();
    goto_priv(PRIV_M);
    first_preserved = (read64(low_phys) == low_pattern && read64(high_phys) == high_pattern);
    goto_priv(PRIV_HS);
    TEST_ASSERT("with a store execute-PC trigger armed on a cross-page access whose low half is writable but whose high half would page-fault, the first access reports breakpoint and leaves both boundary doublewords unchanged before the unrelated aligned-store recovery probe",
        first_excpt.triggered == true &&
        first_excpt.cause == CAUSE_BKP &&
        first_preserved
    );

    TEST_SETUP_EXCEPT();
    ai_arch_execute_trigger_store_once(vaddr, 0x0fedcba987654321ULL);
    ai_arch_trigger_excpt_snapshot_t second_excpt = ai_arch_snapshot_excpt();
    goto_priv(PRIV_M);
    second_preserved = (read64(low_phys) == low_pattern && read64(high_phys) == high_pattern);
    goto_priv(PRIV_HS);
    TEST_ASSERT("with the same store execute-PC trigger still armed on that cross-page high-half page-fault path, the second access still reports breakpoint and still leaves both boundary doublewords unchanged before the unrelated aligned-store recovery probe",
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
    ai_arch_execute_trigger_store_once(vaddr, 0x1122334455667788ULL);
    TEST_ASSERT("after clearing the repeated store execute-PC trigger, the same split access falls back to the true second-half store page fault with the second-half start VA in tval before the unrelated aligned-store recovery probe",
        excpt.triggered == true &&
        excpt.cause == CAUSE_SPF &&
        excpt.tval == second_half_vaddr
    );

    goto_priv(PRIV_M);
    TEST_ASSERT("before the unrelated aligned-store recovery probe, repeated execute-trigger hits and the fallback second-half store page fault still leave both boundary backing doublewords unchanged",
        read64(low_phys) == low_pattern &&
        read64(high_phys) == high_pattern &&
        read64(recovery_phys) == recovery_initial
    );

    goto_priv(PRIV_HS);
    TEST_SETUP_EXCEPT();
    ai_arch_execute_trigger_store_once(recovery_vaddr, recovery_store);
    TEST_ASSERT("after repeated store execute-PC breakpoint hits and the fallback second-half store page fault, another aligned store in the same low page still retires normally without repairing the faulting high half",
        excpt.triggered == false
    );

    TEST_SETUP_EXCEPT();
    ai_arch_execute_trigger_store_once(vaddr, 0x8877665544332211ULL);
    TEST_ASSERT("after the unrelated aligned-store recovery probe, the original split-store shape still reports the same second-half store page fault until the high-half leaf is repaired",
        excpt.triggered == true &&
        excpt.cause == CAUSE_SPF &&
        excpt.tval == second_half_vaddr
    );

    goto_priv(PRIV_M);
    TEST_ASSERT("the repeated store execute-trigger plus high-half-page-fault corner leaves the boundary backing doublewords untouched while committing only the unrelated aligned-store recovery payload",
        read64(low_phys) == low_pattern &&
        read64(high_phys) == high_pattern &&
        read64(recovery_phys) == recovery_store
    );

    goto_priv(PRIV_HS);
    hspt_init();
    sfence_vma();

    TEST_END();
}

bool ai_arch_store_execute_trigger_high_half_page_fault_repeated_breakpoint_then_other_aligned_store_adjacent_preserved_corner()
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
    uintptr_t recovery_vaddr = hs_page_base(low_page) + 0x140U;
    uintptr_t recovery_adjacent_vaddr = recovery_vaddr + sizeof(uint64_t);
    uintptr_t recovery_phys = phys_page_base(low_page) + 0x140U;
    uintptr_t recovery_adjacent_phys = recovery_phys + sizeof(uint64_t);
    const uint64_t low_adjacent = 0xfaceb00c12344321ULL;
    const uint64_t low_pattern = 0x98a9bacbdcedfe10ULL;
    const uint64_t high_pattern = 0x21436587a9cbed0fULL;
    const uint64_t high_adjacent = 0xabcdef0123456789ULL;
    const uint64_t recovery_initial = 0x1021324354657687ULL;
    const uint64_t recovery_store = 0x89abcdef01234567ULL;
    const uint64_t recovery_adjacent = 0x76543210fedcba98ULL;
    bool first_preserved;
    bool second_preserved;

    goto_priv(PRIV_M);
    write64(low_adjacent_phys, low_adjacent);
    write64(low_phys, low_pattern);
    write64(high_phys, high_pattern);
    write64(high_adjacent_phys, high_adjacent);
    write64(recovery_phys, recovery_initial);
    write64(recovery_adjacent_phys, recovery_adjacent);

    ai_arch_arm_mcontrol6_0(trigger_pc, AI_ARCH_MCONTROL6_EXECUTE);
    uint64_t saved_mstatus = ai_arch_enable_machine_breakpoint_delivery();

    goto_priv(PRIV_HS);
    TEST_SETUP_EXCEPT();
    ai_arch_execute_trigger_store_once(vaddr, 0xa1b2c3d4e5f60718ULL);
    ai_arch_trigger_excpt_snapshot_t first_excpt = ai_arch_snapshot_excpt();
    goto_priv(PRIV_M);
    first_preserved =
        (read64(low_adjacent_phys) == low_adjacent &&
        read64(low_phys) == low_pattern &&
        read64(high_phys) == high_pattern &&
        read64(high_adjacent_phys) == high_adjacent);
    goto_priv(PRIV_HS);
    TEST_ASSERT("with a store execute-PC trigger armed on a cross-page access whose low half is writable but whose high half would page-fault, the first access reports breakpoint and leaves both boundary and adjacent doublewords unchanged before the adjacent-preservation aligned-store recovery probe",
        first_excpt.triggered == true &&
        first_excpt.cause == CAUSE_BKP &&
        first_preserved
    );

    TEST_SETUP_EXCEPT();
    ai_arch_execute_trigger_store_once(vaddr, 0x0fedcba987654321ULL);
    ai_arch_trigger_excpt_snapshot_t second_excpt = ai_arch_snapshot_excpt();
    goto_priv(PRIV_M);
    second_preserved =
        (read64(low_adjacent_phys) == low_adjacent &&
        read64(low_phys) == low_pattern &&
        read64(high_phys) == high_pattern &&
        read64(high_adjacent_phys) == high_adjacent);
    goto_priv(PRIV_HS);
    TEST_ASSERT("with the same store execute-PC trigger still armed on that cross-page high-half page-fault path, the second access still reports breakpoint and still leaves both boundary and adjacent doublewords unchanged before the adjacent-preservation aligned-store recovery probe",
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
    ai_arch_execute_trigger_store_once(vaddr, 0x1122334455667788ULL);
    TEST_ASSERT("after clearing the repeated store execute-PC trigger, the same split access falls back to the true second-half store page fault with the second-half start VA in tval before the adjacent-preservation aligned-store recovery probe",
        excpt.triggered == true &&
        excpt.cause == CAUSE_SPF &&
        excpt.tval == second_half_vaddr
    );

    goto_priv(PRIV_M);
    TEST_ASSERT("before the adjacent-preservation aligned-store recovery probe, repeated execute-trigger hits and the fallback second-half store page fault still leave both boundary doublewords, their adjacent neighbors, and the aligned recovery region untouched",
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
    TEST_ASSERT("after repeated store execute-PC breakpoint hits and the fallback second-half store page fault, another aligned store in the same low page still retires normally without repairing the faulting high half in the adjacent-preservation corner",
        excpt.triggered == false
    );

    TEST_SETUP_EXCEPT();
    uint64_t observed_adjacent = ai_arch_execute_trigger_load_once(recovery_adjacent_vaddr);
    TEST_ASSERT("the unrelated aligned-store recovery probe does not disturb the adjacent low-page word, which still reads back its original payload in the adjacent-preservation corner",
        excpt.triggered == false &&
        observed_adjacent == recovery_adjacent
    );

    TEST_SETUP_EXCEPT();
    ai_arch_execute_trigger_store_once(vaddr, 0x8877665544332211ULL);
    TEST_ASSERT("after the adjacent-preservation aligned-store recovery probe, the original split-store shape still reports the same second-half store page fault until the high-half leaf is repaired",
        excpt.triggered == true &&
        excpt.cause == CAUSE_SPF &&
        excpt.tval == second_half_vaddr
    );

    goto_priv(PRIV_M);
    TEST_ASSERT("the repeated store execute-trigger plus high-half-page-fault corner leaves both boundary doublewords, their adjacent neighbors, and the aligned recovery-region adjacent word untouched while committing only the unrelated aligned-store payload",
        read64(low_adjacent_phys) == low_adjacent &&
        read64(low_phys) == low_pattern &&
        read64(high_phys) == high_pattern &&
        read64(high_adjacent_phys) == high_adjacent &&
        read64(recovery_phys) == recovery_store &&
        read64(recovery_adjacent_phys) == recovery_adjacent
    );

    goto_priv(PRIV_HS);
    hspt_init();
    sfence_vma();

    TEST_END();
}

bool ai_arch_store_trigger_high_half_page_fault_repeated_breakpoint_then_other_aligned_store_recovers_corner()
{
    TEST_START();

    enum test_page low_page = VSRW_GX;
    enum test_page high_page = VSRX_GRWX;
    uintptr_t low_phys = ai_arch_trigger_page_end_phys(low_page);
    uintptr_t high_phys = phys_page_base(high_page);
    uintptr_t vaddr = ai_arch_trigger_prepare_cross_page_high_half_store_pf_alias();
    uintptr_t second_half_vaddr = vaddr + 4U;
    uintptr_t recovery_vaddr = hs_page_base(low_page) + 0x160U;
    uintptr_t recovery_phys = phys_page_base(low_page) + 0x160U;
    const uint64_t low_pattern = 0x8798a9bacbdcedfeULL;
    const uint64_t high_pattern = 0x1032547698badcfeULL;
    const uint64_t recovery_initial = 0x0f1e2d3c4b5a6978ULL;
    const uint64_t recovery_store = 0x76543210fedcba98ULL;
    bool first_preserved;
    bool second_preserved;

    goto_priv(PRIV_M);
    write64(low_phys, low_pattern);
    write64(high_phys, high_pattern);
    write64(recovery_phys, recovery_initial);

    ai_arch_arm_mcontrol6_0(vaddr, AI_ARCH_MCONTROL6_STORE);
    uint64_t saved_mstatus = ai_arch_enable_machine_breakpoint_delivery();

    goto_priv(PRIV_HS);
    TEST_SETUP_EXCEPT();
    sd(vaddr, 0xa1b2c3d4e5f60718ULL);
    ai_arch_trigger_excpt_snapshot_t first_excpt = ai_arch_snapshot_excpt();
    goto_priv(PRIV_M);
    first_preserved = (read64(low_phys) == low_pattern && read64(high_phys) == high_pattern);
    goto_priv(PRIV_HS);
    TEST_ASSERT("with a store address trigger armed on a cross-page access whose low half is writable but whose high half would page-fault, the first access reports breakpoint and leaves both boundary doublewords unchanged before the unrelated aligned-store recovery probe",
        first_excpt.triggered == true &&
        first_excpt.cause == CAUSE_BKP &&
        first_preserved
    );

    TEST_SETUP_EXCEPT();
    sd(vaddr, 0x0fedcba987654321ULL);
    ai_arch_trigger_excpt_snapshot_t second_excpt = ai_arch_snapshot_excpt();
    goto_priv(PRIV_M);
    second_preserved = (read64(low_phys) == low_pattern && read64(high_phys) == high_pattern);
    goto_priv(PRIV_HS);
    TEST_ASSERT("with the same store address trigger still armed on that cross-page high-half page-fault path, the second access still reports breakpoint and still leaves both boundary doublewords unchanged before the unrelated aligned-store recovery probe",
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
    sd(vaddr, 0x1122334455667788ULL);
    TEST_ASSERT("after clearing the repeated store address trigger, the same split access falls back to the true second-half store page fault with the second-half start VA in tval before the unrelated aligned-store recovery probe",
        excpt.triggered == true &&
        excpt.cause == CAUSE_SPF &&
        excpt.tval == second_half_vaddr
    );

    goto_priv(PRIV_M);
    TEST_ASSERT("before the unrelated aligned-store recovery probe, repeated address-trigger hits and the fallback second-half store page fault still leave both boundary backing doublewords unchanged",
        read64(low_phys) == low_pattern &&
        read64(high_phys) == high_pattern &&
        read64(recovery_phys) == recovery_initial
    );

    goto_priv(PRIV_HS);
    TEST_SETUP_EXCEPT();
    sd(recovery_vaddr, recovery_store);
    TEST_ASSERT("after repeated store address-trigger breakpoint hits and the fallback second-half store page fault, another aligned store in the same low page still retires normally without repairing the faulting high half",
        excpt.triggered == false
    );

    TEST_SETUP_EXCEPT();
    sd(vaddr, 0x8877665544332211ULL);
    TEST_ASSERT("after the unrelated aligned-store recovery probe, the original split-store shape still reports the same second-half store page fault until the high-half leaf is repaired",
        excpt.triggered == true &&
        excpt.cause == CAUSE_SPF &&
        excpt.tval == second_half_vaddr
    );

    goto_priv(PRIV_M);
    TEST_ASSERT("the repeated store address-trigger plus high-half-page-fault corner leaves the boundary backing doublewords untouched while committing only the unrelated aligned-store recovery payload",
        read64(low_phys) == low_pattern &&
        read64(high_phys) == high_pattern &&
        read64(recovery_phys) == recovery_store
    );

    goto_priv(PRIV_HS);
    hspt_init();
    sfence_vma();

    TEST_END();
}

bool ai_arch_store_trigger_high_half_page_fault_repeated_breakpoint_then_other_aligned_store_adjacent_preserved_corner()
{
    TEST_START();

    enum test_page low_page = VSRW_GX;
    enum test_page high_page = VSRX_GRWX;
    uintptr_t low_adjacent_phys = phys_page_base(low_page) + PAGE_SIZE - (2 * sizeof(uint64_t));
    uintptr_t low_phys = ai_arch_trigger_page_end_phys(low_page);
    uintptr_t high_phys = phys_page_base(high_page);
    uintptr_t high_adjacent_phys = high_phys + sizeof(uint64_t);
    uintptr_t vaddr = ai_arch_trigger_prepare_cross_page_high_half_store_pf_alias();
    uintptr_t second_half_vaddr = vaddr + 4U;
    uintptr_t recovery_vaddr = hs_page_base(low_page) + 0x160U;
    uintptr_t recovery_adjacent_vaddr = recovery_vaddr + sizeof(uint64_t);
    uintptr_t recovery_phys = phys_page_base(low_page) + 0x160U;
    uintptr_t recovery_adjacent_phys = recovery_phys + sizeof(uint64_t);
    const uint64_t low_adjacent = 0x12344321faceb00cULL;
    const uint64_t low_pattern = 0x8798a9bacbdcedfeULL;
    const uint64_t high_pattern = 0x1032547698badcfeULL;
    const uint64_t high_adjacent = 0xfedcba9876543210ULL;
    const uint64_t recovery_initial = 0x0f1e2d3c4b5a6978ULL;
    const uint64_t recovery_store = 0x76543210fedcba98ULL;
    const uint64_t recovery_adjacent = 0x89abcdef01234567ULL;
    bool first_preserved;
    bool second_preserved;

    goto_priv(PRIV_M);
    write64(low_adjacent_phys, low_adjacent);
    write64(low_phys, low_pattern);
    write64(high_phys, high_pattern);
    write64(high_adjacent_phys, high_adjacent);
    write64(recovery_phys, recovery_initial);
    write64(recovery_adjacent_phys, recovery_adjacent);

    ai_arch_arm_mcontrol6_0(vaddr, AI_ARCH_MCONTROL6_STORE);
    uint64_t saved_mstatus = ai_arch_enable_machine_breakpoint_delivery();

    goto_priv(PRIV_HS);
    TEST_SETUP_EXCEPT();
    sd(vaddr, 0xa1b2c3d4e5f60718ULL);
    ai_arch_trigger_excpt_snapshot_t first_excpt = ai_arch_snapshot_excpt();
    goto_priv(PRIV_M);
    first_preserved =
        (read64(low_adjacent_phys) == low_adjacent &&
        read64(low_phys) == low_pattern &&
        read64(high_phys) == high_pattern &&
        read64(high_adjacent_phys) == high_adjacent);
    goto_priv(PRIV_HS);
    TEST_ASSERT("with a store address trigger armed on a cross-page access whose low half is writable but whose high half would page-fault, the first access reports breakpoint and leaves both boundary and adjacent doublewords unchanged before the adjacent-preservation aligned-store recovery probe",
        first_excpt.triggered == true &&
        first_excpt.cause == CAUSE_BKP &&
        first_preserved
    );

    TEST_SETUP_EXCEPT();
    sd(vaddr, 0x0fedcba987654321ULL);
    ai_arch_trigger_excpt_snapshot_t second_excpt = ai_arch_snapshot_excpt();
    goto_priv(PRIV_M);
    second_preserved =
        (read64(low_adjacent_phys) == low_adjacent &&
        read64(low_phys) == low_pattern &&
        read64(high_phys) == high_pattern &&
        read64(high_adjacent_phys) == high_adjacent);
    goto_priv(PRIV_HS);
    TEST_ASSERT("with the same store address trigger still armed on that cross-page high-half page-fault path, the second access still reports breakpoint and still leaves both boundary and adjacent doublewords unchanged before the adjacent-preservation aligned-store recovery probe",
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
    sd(vaddr, 0x1122334455667788ULL);
    TEST_ASSERT("after clearing the repeated store address trigger, the same split access falls back to the true second-half store page fault with the second-half start VA in tval before the adjacent-preservation aligned-store recovery probe",
        excpt.triggered == true &&
        excpt.cause == CAUSE_SPF &&
        excpt.tval == second_half_vaddr
    );

    goto_priv(PRIV_M);
    TEST_ASSERT("before the adjacent-preservation aligned-store recovery probe, repeated address-trigger hits and the fallback second-half store page fault still leave both boundary doublewords, their adjacent neighbors, and the aligned recovery region untouched",
        read64(low_adjacent_phys) == low_adjacent &&
        read64(low_phys) == low_pattern &&
        read64(high_phys) == high_pattern &&
        read64(high_adjacent_phys) == high_adjacent &&
        read64(recovery_phys) == recovery_initial &&
        read64(recovery_adjacent_phys) == recovery_adjacent
    );

    goto_priv(PRIV_HS);
    TEST_SETUP_EXCEPT();
    sd(recovery_vaddr, recovery_store);
    TEST_ASSERT("after repeated store address-trigger breakpoint hits and the fallback second-half store page fault, another aligned store in the same low page still retires normally without repairing the faulting high half in the adjacent-preservation corner",
        excpt.triggered == false
    );

    TEST_SETUP_EXCEPT();
    uint64_t observed_adjacent = ld(recovery_adjacent_vaddr);
    TEST_ASSERT("the unrelated aligned-store recovery probe does not disturb the adjacent low-page word, which still reads back its original payload in the adjacent-preservation corner",
        excpt.triggered == false &&
        observed_adjacent == recovery_adjacent
    );

    TEST_SETUP_EXCEPT();
    sd(vaddr, 0x8877665544332211ULL);
    TEST_ASSERT("after the adjacent-preservation aligned-store recovery probe, the original split-store shape still reports the same second-half store page fault until the high-half leaf is repaired",
        excpt.triggered == true &&
        excpt.cause == CAUSE_SPF &&
        excpt.tval == second_half_vaddr
    );

    goto_priv(PRIV_M);
    TEST_ASSERT("the repeated store address-trigger plus high-half-page-fault corner leaves both boundary doublewords, their adjacent neighbors, and the aligned recovery-region adjacent word untouched while committing only the unrelated aligned-store payload",
        read64(low_adjacent_phys) == low_adjacent &&
        read64(low_phys) == low_pattern &&
        read64(high_phys) == high_pattern &&
        read64(high_adjacent_phys) == high_adjacent &&
        read64(recovery_phys) == recovery_store &&
        read64(recovery_adjacent_phys) == recovery_adjacent
    );

    goto_priv(PRIV_HS);
    hspt_init();
    sfence_vma();

    TEST_END();
}

bool ai_arch_load_trigger_misaligned_priority()
{
    TEST_START();

    uintptr_t paddr = phys_page_base(X);
    uintptr_t misaligned_vaddr = ai_arch_trigger_prepare_pbmt_nc_alias() + 7U;
    const uint64_t expected = 0x1122334455667788ULL;

    goto_priv(PRIV_M);
    write64(paddr, expected);

    ai_arch_arm_mcontrol6_0(misaligned_vaddr, AI_ARCH_MCONTROL6_LOAD);
    uint64_t saved_mstatus = ai_arch_enable_machine_breakpoint_delivery();

    goto_priv(PRIV_HS);
    TEST_SETUP_EXCEPT();
    ld(misaligned_vaddr);
    if (!(excpt.triggered == true && excpt.cause == CAUSE_BKP)) {
        ai_arch_log_unexpected_trigger_result("unexpected load address-trigger+misaligned priority result");
    }
    TEST_ASSERT("a load address trigger takes priority when the same HS PBMT-NC load would otherwise raise load misaligned",
        excpt.triggered == true &&
        excpt.cause == CAUSE_BKP
    );
    ai_arch_restore_machine_status(saved_mstatus);
    ai_arch_clear_mcontrol6_0();

    goto_priv(PRIV_HS);
    TEST_SETUP_EXCEPT();
    ld(misaligned_vaddr);
    TEST_ASSERT("after clearing the load address trigger, the same HS PBMT-NC load reports load misaligned with the raw misaligned VA in tval",
        excpt.triggered == true &&
        excpt.cause == CAUSE_LAM &&
        excpt.tval == misaligned_vaddr
    );

    goto_priv(PRIV_M);
    TEST_ASSERT("the load address-trigger+misaligned probe leaves the PBMT-NC backing word unchanged",
        read64(paddr) == expected
    );
    ai_arch_trigger_reset_pbmt_only_env();

    TEST_END();
}

bool ai_arch_load_trigger_misaligned_repeated_breakpoint_then_fault_corner()
{
    TEST_START();

    uintptr_t paddr = phys_page_base(X);
    uintptr_t misaligned_vaddr = ai_arch_trigger_prepare_pbmt_nc_alias() + 7U;
    const uint64_t expected = 0x2233445566778899ULL;
    bool first_preserved;
    bool second_preserved;

    goto_priv(PRIV_M);
    write64(paddr, expected);

    ai_arch_arm_mcontrol6_0(misaligned_vaddr, AI_ARCH_MCONTROL6_LOAD);
    uint64_t saved_mstatus = ai_arch_enable_machine_breakpoint_delivery();

    goto_priv(PRIV_HS);
    TEST_SETUP_EXCEPT();
    ld(misaligned_vaddr);
    ai_arch_trigger_excpt_snapshot_t first_excpt = ai_arch_snapshot_excpt();
    goto_priv(PRIV_M);
    first_preserved = (read64(paddr) == expected);
    goto_priv(PRIV_HS);
    TEST_ASSERT("with a load address trigger armed on an HS PBMT-NC misaligned load, the first access reports breakpoint and leaves the backing word unchanged",
        first_excpt.triggered == true &&
        first_excpt.cause == CAUSE_BKP &&
        first_preserved
    );

    TEST_SETUP_EXCEPT();
    ld(misaligned_vaddr);
    ai_arch_trigger_excpt_snapshot_t second_excpt = ai_arch_snapshot_excpt();
    goto_priv(PRIV_M);
    second_preserved = (read64(paddr) == expected);
    goto_priv(PRIV_HS);
    TEST_ASSERT("with the same load address trigger still armed on the same HS PBMT-NC misaligned load, the second access still reports breakpoint and still leaves the backing word unchanged",
        second_excpt.triggered == true &&
        second_excpt.cause == CAUSE_BKP &&
        second_preserved
    );
    ai_arch_restore_machine_status(saved_mstatus);
    ai_arch_clear_mcontrol6_0();

    goto_priv(PRIV_HS);
    TEST_SETUP_EXCEPT();
    ld(misaligned_vaddr);
    TEST_ASSERT("after clearing the repeated load address trigger, the same HS PBMT-NC access falls back to load misaligned with the raw misaligned VA in tval",
        excpt.triggered == true &&
        excpt.cause == CAUSE_LAM &&
        excpt.tval == misaligned_vaddr
    );

    goto_priv(PRIV_M);
    TEST_ASSERT("the repeated load address-trigger+misaligned probe still leaves the PBMT-NC backing word unchanged after the fallback fault",
        read64(paddr) == expected
    );
    ai_arch_trigger_reset_pbmt_only_env();

    TEST_END();
}

bool ai_arch_load_execute_trigger_misaligned_priority()
{
    TEST_START();

    uintptr_t paddr = phys_page_base(X);
    uintptr_t trigger_pc = (uintptr_t)ai_arch_execute_trigger_load_once;
    uintptr_t misaligned_vaddr = ai_arch_trigger_prepare_pbmt_nc_alias() + 7U;
    const uint64_t expected = 0x33445566778899aaULL;

    goto_priv(PRIV_M);
    write64(paddr, expected);

    ai_arch_arm_mcontrol6_0(trigger_pc, AI_ARCH_MCONTROL6_EXECUTE);
    uint64_t saved_mstatus = ai_arch_enable_machine_breakpoint_delivery();

    goto_priv(PRIV_HS);
    TEST_SETUP_EXCEPT();
    ai_arch_execute_trigger_load_once(misaligned_vaddr);
    if (!(excpt.triggered == true && excpt.cause == CAUSE_BKP)) {
        ai_arch_log_unexpected_trigger_result("unexpected load execute-PC trigger+misaligned priority result");
    }
    TEST_ASSERT("a load execute-PC trigger takes priority when the same HS PBMT-NC load would otherwise raise load misaligned",
        excpt.triggered == true &&
        excpt.cause == CAUSE_BKP
    );
    ai_arch_restore_machine_status(saved_mstatus);
    ai_arch_clear_mcontrol6_0();

    goto_priv(PRIV_HS);
    TEST_SETUP_EXCEPT();
    ai_arch_execute_trigger_load_once(misaligned_vaddr);
    TEST_ASSERT("after clearing the load execute-PC trigger, the same HS PBMT-NC load reports load misaligned with the raw misaligned VA in tval",
        excpt.triggered == true &&
        excpt.cause == CAUSE_LAM &&
        excpt.tval == misaligned_vaddr
    );

    goto_priv(PRIV_M);
    TEST_ASSERT("the load execute-trigger+misaligned probe leaves the PBMT-NC backing word unchanged",
        read64(paddr) == expected
    );
    ai_arch_trigger_reset_pbmt_only_env();

    TEST_END();
}

bool ai_arch_load_execute_trigger_misaligned_repeated_breakpoint_then_fault_corner()
{
    TEST_START();

    uintptr_t paddr = phys_page_base(X);
    uintptr_t trigger_pc = (uintptr_t)ai_arch_execute_trigger_load_once;
    uintptr_t misaligned_vaddr = ai_arch_trigger_prepare_pbmt_nc_alias() + 7U;
    const uint64_t expected = 0x5566778899aabbccULL;
    bool first_preserved;
    bool second_preserved;

    goto_priv(PRIV_M);
    write64(paddr, expected);

    ai_arch_arm_mcontrol6_0(trigger_pc, AI_ARCH_MCONTROL6_EXECUTE);
    uint64_t saved_mstatus = ai_arch_enable_machine_breakpoint_delivery();

    goto_priv(PRIV_HS);
    TEST_SETUP_EXCEPT();
    ai_arch_execute_trigger_load_once(misaligned_vaddr);
    ai_arch_trigger_excpt_snapshot_t first_excpt = ai_arch_snapshot_excpt();
    goto_priv(PRIV_M);
    first_preserved = (read64(paddr) == expected);
    goto_priv(PRIV_HS);
    TEST_ASSERT("with a load execute-PC trigger armed on an HS PBMT-NC misaligned load, the first access reports breakpoint and leaves the backing word unchanged",
        first_excpt.triggered == true &&
        first_excpt.cause == CAUSE_BKP &&
        first_preserved
    );

    TEST_SETUP_EXCEPT();
    ai_arch_execute_trigger_load_once(misaligned_vaddr);
    ai_arch_trigger_excpt_snapshot_t second_excpt = ai_arch_snapshot_excpt();
    goto_priv(PRIV_M);
    second_preserved = (read64(paddr) == expected);
    goto_priv(PRIV_HS);
    TEST_ASSERT("with the same load execute-PC trigger still armed on the same HS PBMT-NC misaligned load, the second access still reports breakpoint and still leaves the backing word unchanged",
        second_excpt.triggered == true &&
        second_excpt.cause == CAUSE_BKP &&
        second_preserved
    );
    ai_arch_restore_machine_status(saved_mstatus);
    ai_arch_clear_mcontrol6_0();

    goto_priv(PRIV_HS);
    TEST_SETUP_EXCEPT();
    ai_arch_execute_trigger_load_once(misaligned_vaddr);
    TEST_ASSERT("after clearing the repeated load execute-PC trigger, the same HS PBMT-NC access falls back to load misaligned with the raw misaligned VA in tval",
        excpt.triggered == true &&
        excpt.cause == CAUSE_LAM &&
        excpt.tval == misaligned_vaddr
    );

    goto_priv(PRIV_M);
    TEST_ASSERT("the repeated load execute-trigger+misaligned probe still leaves the PBMT-NC backing word unchanged after the fallback fault",
        read64(paddr) == expected
    );
    ai_arch_trigger_reset_pbmt_only_env();

    TEST_END();
}

bool ai_arch_load_execute_and_address_trigger_misaligned_priority_corner()
{
    TEST_START();

    uintptr_t paddr = phys_page_base(X);
    uintptr_t trigger_pc = (uintptr_t)ai_arch_execute_trigger_load_once;
    uintptr_t misaligned_vaddr = ai_arch_trigger_prepare_pbmt_nc_alias() + 7U;
    const uint64_t expected = 0x445566778899aabbULL;

    goto_priv(PRIV_M);
    write64(paddr, expected);

    ai_arch_clear_mcontrol6_slot(0);
    ai_arch_clear_mcontrol6_slot(1);
    ai_arch_arm_mcontrol6_slot(0, trigger_pc, AI_ARCH_MCONTROL6_EXECUTE);
    ai_arch_arm_mcontrol6_slot(1, misaligned_vaddr, AI_ARCH_MCONTROL6_LOAD);
    ai_arch_sync_trigger_state();

    uint64_t saved_mstatus = ai_arch_enable_machine_breakpoint_delivery();
    goto_priv(PRIV_HS);
    TEST_SETUP_EXCEPT();
    ai_arch_execute_trigger_load_once(misaligned_vaddr);
    TEST_ASSERT("when both execute-PC and address triggers match the same HS PBMT-NC misaligned load, breakpoint still wins first",
        excpt.triggered == true &&
        excpt.cause == CAUSE_BKP
    );
    ai_arch_restore_machine_status(saved_mstatus);

    ai_arch_clear_mcontrol6_slot(0);
    saved_mstatus = ai_arch_enable_machine_breakpoint_delivery();
    goto_priv(PRIV_HS);
    TEST_SETUP_EXCEPT();
    ai_arch_execute_trigger_load_once(misaligned_vaddr);
    TEST_ASSERT("after clearing only the execute-PC trigger, the remaining load-address trigger still reports breakpoint on the same HS PBMT-NC misaligned load",
        excpt.triggered == true &&
        excpt.cause == CAUSE_BKP
    );
    ai_arch_restore_machine_status(saved_mstatus);

    ai_arch_clear_mcontrol6_slot(1);
    goto_priv(PRIV_HS);
    TEST_SETUP_EXCEPT();
    ai_arch_execute_trigger_load_once(misaligned_vaddr);
    TEST_ASSERT("after clearing both load trigger slots, the same HS PBMT-NC access falls back to load misaligned with the raw misaligned VA in tval",
        excpt.triggered == true &&
        excpt.cause == CAUSE_LAM &&
        excpt.tval == misaligned_vaddr
    );

    goto_priv(PRIV_M);
    TEST_ASSERT("the dual-slot load trigger+misaligned priority corner leaves the PBMT-NC backing word unchanged across both breakpoint hits and the fallback misalign fault",
        read64(paddr) == expected
    );
    ai_arch_trigger_reset_pbmt_only_env();

    TEST_END();
}

bool ai_arch_load_execute_and_address_trigger_misaligned_repeated_breakpoint_then_fault_corner()
{
    TEST_START();

    uintptr_t paddr = phys_page_base(X);
    uintptr_t trigger_pc = (uintptr_t)ai_arch_execute_trigger_load_once;
    uintptr_t misaligned_vaddr = ai_arch_trigger_prepare_pbmt_nc_alias() + 7U;
    const uint64_t expected = 0x5566778899aabbccULL;
    bool first_preserved;
    bool second_preserved;
    bool third_preserved;

    goto_priv(PRIV_M);
    write64(paddr, expected);

    ai_arch_clear_mcontrol6_slot(0);
    ai_arch_clear_mcontrol6_slot(1);
    ai_arch_arm_mcontrol6_slot(0, trigger_pc, AI_ARCH_MCONTROL6_EXECUTE);
    ai_arch_arm_mcontrol6_slot(1, misaligned_vaddr, AI_ARCH_MCONTROL6_LOAD);
    ai_arch_sync_trigger_state();

    uint64_t saved_mstatus = ai_arch_enable_machine_breakpoint_delivery();
    goto_priv(PRIV_HS);
    TEST_SETUP_EXCEPT();
    ai_arch_execute_trigger_load_once(misaligned_vaddr);
    ai_arch_trigger_excpt_snapshot_t first_excpt = ai_arch_snapshot_excpt();
    goto_priv(PRIV_M);
    first_preserved = (read64(paddr) == expected);
    goto_priv(PRIV_HS);
    TEST_ASSERT("with both load trigger slots armed on the same HS PBMT-NC misaligned load, the first access reports breakpoint and still leaves the backing word unchanged",
        first_excpt.triggered == true &&
        first_excpt.cause == CAUSE_BKP &&
        first_preserved
    );
    ai_arch_sync_trigger_state();

    TEST_SETUP_EXCEPT();
    ai_arch_execute_trigger_load_once(misaligned_vaddr);
    ai_arch_trigger_excpt_snapshot_t second_excpt = ai_arch_snapshot_excpt();
    goto_priv(PRIV_M);
    second_preserved = (read64(paddr) == expected);
    goto_priv(PRIV_HS);
    TEST_ASSERT("with both load trigger slots still armed on that same HS PBMT-NC misaligned load, the second access still reports breakpoint and still leaves the backing word unchanged",
        second_excpt.triggered == true &&
        second_excpt.cause == CAUSE_BKP &&
        second_preserved
    );
    ai_arch_restore_machine_status(saved_mstatus);

    ai_arch_clear_mcontrol6_slot(0);
    ai_arch_sync_trigger_state();
    saved_mstatus = ai_arch_enable_machine_breakpoint_delivery();
    goto_priv(PRIV_HS);
    TEST_SETUP_EXCEPT();
    ai_arch_execute_trigger_load_once(misaligned_vaddr);
    ai_arch_trigger_excpt_snapshot_t third_excpt = ai_arch_snapshot_excpt();
    goto_priv(PRIV_M);
    third_preserved = (read64(paddr) == expected);
    goto_priv(PRIV_HS);
    TEST_ASSERT("after repeated dual-slot load breakpoint hits, clearing only the execute-PC slot still leaves the address trigger active on the same HS PBMT-NC misaligned load",
        third_excpt.triggered == true &&
        third_excpt.cause == CAUSE_BKP &&
        third_preserved
    );
    ai_arch_restore_machine_status(saved_mstatus);

    ai_arch_clear_mcontrol6_slot(1);
    ai_arch_sync_trigger_state();
    goto_priv(PRIV_HS);
    TEST_SETUP_EXCEPT();
    ai_arch_execute_trigger_load_once(misaligned_vaddr);
    TEST_ASSERT("after clearing both repeated load trigger slots, the same HS PBMT-NC access falls back to load misaligned with the raw misaligned VA in tval",
        excpt.triggered == true &&
        excpt.cause == CAUSE_LAM &&
        excpt.tval == misaligned_vaddr
    );

    goto_priv(PRIV_M);
    TEST_ASSERT("the repeated dual-slot load trigger+misaligned corner still leaves the PBMT-NC backing word unchanged after the fallback misalign fault",
        read64(paddr) == expected
    );
    ai_arch_trigger_reset_pbmt_only_env();

    TEST_END();
}

bool ai_arch_load_execute_and_address_trigger_misaligned_repeated_breakpoint_then_fault_adjacent_preserved_corner()
{
    TEST_START();

    uintptr_t misaligned_paddr = phys_page_base(X);
    uintptr_t misaligned_adjacent_paddr = misaligned_paddr + sizeof(uint64_t);
    uintptr_t trigger_pc = (uintptr_t)ai_arch_execute_trigger_load_once;
    uintptr_t misaligned_vaddr = ai_arch_trigger_prepare_pbmt_nc_alias() + 7U;
    const uint64_t misaligned_expected = 0x5566778899aabbccULL;
    const uint64_t misaligned_adjacent = 0xdeadc0de12345678ULL;
    bool first_preserved;
    bool second_preserved;
    bool third_preserved;

    goto_priv(PRIV_M);
    write64(misaligned_paddr, misaligned_expected);
    write64(misaligned_adjacent_paddr, misaligned_adjacent);

    ai_arch_clear_mcontrol6_slot(0);
    ai_arch_clear_mcontrol6_slot(1);
    ai_arch_arm_mcontrol6_slot(0, trigger_pc, AI_ARCH_MCONTROL6_EXECUTE);
    ai_arch_arm_mcontrol6_slot(1, misaligned_vaddr, AI_ARCH_MCONTROL6_LOAD);
    ai_arch_sync_trigger_state();

    uint64_t saved_mstatus = ai_arch_enable_machine_breakpoint_delivery();
    goto_priv(PRIV_HS);
    TEST_SETUP_EXCEPT();
    ai_arch_execute_trigger_load_once(misaligned_vaddr);
    ai_arch_trigger_excpt_snapshot_t first_excpt = ai_arch_snapshot_excpt();
    goto_priv(PRIV_M);
    first_preserved =
        (read64(misaligned_paddr) == misaligned_expected &&
        read64(misaligned_adjacent_paddr) == misaligned_adjacent);
    goto_priv(PRIV_HS);
    TEST_ASSERT("with both load trigger slots armed on the same HS PBMT-NC misaligned load, the first access reports breakpoint and still leaves both crossed backing doublewords unchanged in the adjacent-preservation corner",
        first_excpt.triggered == true &&
        first_excpt.cause == CAUSE_BKP &&
        first_preserved
    );
    ai_arch_sync_trigger_state();

    TEST_SETUP_EXCEPT();
    ai_arch_execute_trigger_load_once(misaligned_vaddr);
    ai_arch_trigger_excpt_snapshot_t second_excpt = ai_arch_snapshot_excpt();
    goto_priv(PRIV_M);
    second_preserved =
        (read64(misaligned_paddr) == misaligned_expected &&
        read64(misaligned_adjacent_paddr) == misaligned_adjacent);
    goto_priv(PRIV_HS);
    TEST_ASSERT("with both load trigger slots still armed on that same HS PBMT-NC misaligned load, the second access still reports breakpoint and still leaves both crossed backing doublewords unchanged in the adjacent-preservation corner",
        second_excpt.triggered == true &&
        second_excpt.cause == CAUSE_BKP &&
        second_preserved
    );
    ai_arch_restore_machine_status(saved_mstatus);

    ai_arch_clear_mcontrol6_slot(0);
    ai_arch_sync_trigger_state();
    saved_mstatus = ai_arch_enable_machine_breakpoint_delivery();
    goto_priv(PRIV_HS);
    TEST_SETUP_EXCEPT();
    ai_arch_execute_trigger_load_once(misaligned_vaddr);
    ai_arch_trigger_excpt_snapshot_t third_excpt = ai_arch_snapshot_excpt();
    goto_priv(PRIV_M);
    third_preserved =
        (read64(misaligned_paddr) == misaligned_expected &&
        read64(misaligned_adjacent_paddr) == misaligned_adjacent);
    goto_priv(PRIV_HS);
    TEST_ASSERT("after repeated dual-slot load breakpoint hits, clearing only the execute-PC slot still leaves the address trigger active on the same HS PBMT-NC misaligned load without dirtying either crossed doubleword in the adjacent-preservation corner",
        third_excpt.triggered == true &&
        third_excpt.cause == CAUSE_BKP &&
        third_preserved
    );
    ai_arch_restore_machine_status(saved_mstatus);

    ai_arch_clear_mcontrol6_slot(1);
    ai_arch_sync_trigger_state();
    goto_priv(PRIV_HS);
    TEST_SETUP_EXCEPT();
    ai_arch_execute_trigger_load_once(misaligned_vaddr);
    TEST_ASSERT("after clearing both repeated load trigger slots, the same HS PBMT-NC access falls back to load misaligned with the raw misaligned VA in tval in the adjacent-preservation corner",
        excpt.triggered == true &&
        excpt.cause == CAUSE_LAM &&
        excpt.tval == misaligned_vaddr
    );

    goto_priv(PRIV_M);
    TEST_ASSERT("the repeated dual-slot load trigger+misaligned corner still leaves both crossed PBMT-NC backing doublewords unchanged after the fallback misalign fault in the adjacent-preservation corner",
        read64(misaligned_paddr) == misaligned_expected &&
        read64(misaligned_adjacent_paddr) == misaligned_adjacent
    );
    ai_arch_trigger_reset_pbmt_only_env();

    TEST_END();
}

bool ai_arch_load_execute_and_address_trigger_misaligned_repeated_breakpoint_then_valid_hs_load_recovers_corner()
{
    TEST_START();

    uintptr_t misaligned_paddr = phys_page_base(X);
    uintptr_t trigger_pc = (uintptr_t)ai_arch_execute_trigger_load_once;
    uintptr_t misaligned_vaddr = ai_arch_trigger_prepare_pbmt_nc_alias() + 7U;
    uintptr_t recovery_paddr = phys_page_base(VSRWX_GRWX) + 0x180U;
    uintptr_t recovery_vaddr = hs_page_base(VSRWX_GRWX) + 0x180U;
    const uint64_t misaligned_expected = 0x66778899aabbccddULL;
    const uint64_t recovery_expected = 0x13579bdf2468ace0ULL;
    bool first_preserved;
    bool second_preserved;
    bool third_preserved;

    goto_priv(PRIV_M);
    write64(misaligned_paddr, misaligned_expected);
    write64(recovery_paddr, recovery_expected);

    ai_arch_clear_mcontrol6_slot(0);
    ai_arch_clear_mcontrol6_slot(1);
    ai_arch_arm_mcontrol6_slot(0, trigger_pc, AI_ARCH_MCONTROL6_EXECUTE);
    ai_arch_arm_mcontrol6_slot(1, misaligned_vaddr, AI_ARCH_MCONTROL6_LOAD);

    uint64_t saved_mstatus = ai_arch_enable_machine_breakpoint_delivery();
    goto_priv(PRIV_HS);
    TEST_SETUP_EXCEPT();
    ai_arch_execute_trigger_load_once(misaligned_vaddr);
    ai_arch_trigger_excpt_snapshot_t first_excpt = ai_arch_snapshot_excpt();
    goto_priv(PRIV_M);
    first_preserved = (read64(misaligned_paddr) == misaligned_expected);
    goto_priv(PRIV_HS);
    TEST_ASSERT("with both load trigger slots armed on the same HS PBMT-NC misaligned load, the first access reports breakpoint and still leaves the backing word unchanged",
        first_excpt.triggered == true &&
        first_excpt.cause == CAUSE_BKP &&
        first_preserved
    );

    TEST_SETUP_EXCEPT();
    ai_arch_execute_trigger_load_once(misaligned_vaddr);
    ai_arch_trigger_excpt_snapshot_t second_excpt = ai_arch_snapshot_excpt();
    goto_priv(PRIV_M);
    second_preserved = (read64(misaligned_paddr) == misaligned_expected);
    goto_priv(PRIV_HS);
    TEST_ASSERT("with both load trigger slots still armed on that same HS PBMT-NC misaligned load, the second access still reports breakpoint and still leaves the backing word unchanged",
        second_excpt.triggered == true &&
        second_excpt.cause == CAUSE_BKP &&
        second_preserved
    );
    ai_arch_restore_machine_status(saved_mstatus);

    ai_arch_clear_mcontrol6_slot(0);
    saved_mstatus = ai_arch_enable_machine_breakpoint_delivery();
    goto_priv(PRIV_HS);
    TEST_SETUP_EXCEPT();
    ai_arch_execute_trigger_load_once(misaligned_vaddr);
    ai_arch_trigger_excpt_snapshot_t third_excpt = ai_arch_snapshot_excpt();
    goto_priv(PRIV_M);
    third_preserved = (read64(misaligned_paddr) == misaligned_expected);
    goto_priv(PRIV_HS);
    TEST_ASSERT("after repeated dual-slot load breakpoint hits, clearing only the execute-PC slot still leaves the address trigger active on the same HS PBMT-NC misaligned load",
        third_excpt.triggered == true &&
        third_excpt.cause == CAUSE_BKP &&
        third_preserved
    );
    ai_arch_restore_machine_status(saved_mstatus);

    ai_arch_clear_mcontrol6_slot(1);
    goto_priv(PRIV_HS);
    TEST_SETUP_EXCEPT();
    ai_arch_execute_trigger_load_once(misaligned_vaddr);
    TEST_ASSERT("after clearing both repeated load trigger slots, the same HS PBMT-NC access falls back to load misaligned with the raw misaligned VA in tval",
        excpt.triggered == true &&
        excpt.cause == CAUSE_LAM &&
        excpt.tval == misaligned_vaddr
    );

    goto_priv(PRIV_M);
    TEST_ASSERT("the repeated dual-slot load trigger+misaligned corner still leaves the PBMT-NC backing word unchanged after the fallback misalign fault",
        read64(misaligned_paddr) == misaligned_expected
    );
    ai_arch_trigger_reset_pbmt_only_env();
    write64(recovery_paddr, recovery_expected);

    goto_priv(PRIV_HS);
    TEST_SETUP_EXCEPT();
    uint64_t recovery_value = ai_arch_execute_trigger_load_once(recovery_vaddr);
    TEST_ASSERT("after repeated dual-slot load trigger+misalign hits and the fallback misalign fault, a fresh aligned HS load still recovers normally once the PBMT-NC setup is removed",
        excpt.triggered == false &&
        recovery_value == recovery_expected
    );

    TEST_END();
}

bool ai_arch_load_execute_and_address_trigger_misaligned_repeated_breakpoint_then_valid_hs_load_adjacent_preserved_corner()
{
    TEST_START();

    uintptr_t misaligned_paddr = phys_page_base(X);
    uintptr_t misaligned_adjacent_paddr = misaligned_paddr + sizeof(uint64_t);
    uintptr_t trigger_pc = (uintptr_t)ai_arch_execute_trigger_load_once;
    uintptr_t misaligned_vaddr = ai_arch_trigger_prepare_pbmt_nc_alias() + 7U;
    uintptr_t recovery_paddr = phys_page_base(VSRWX_GRWX) + 0x1a0U;
    uintptr_t recovery_vaddr = hs_page_base(VSRWX_GRWX) + 0x1a0U;
    uintptr_t recovery_adjacent_paddr = recovery_paddr + sizeof(uint64_t);
    uintptr_t recovery_adjacent_vaddr = recovery_vaddr + sizeof(uint64_t);
    const uint64_t misaligned_expected = 0x66778899aabbccddULL;
    const uint64_t misaligned_adjacent = 0xdeadc0de12345678ULL;
    const uint64_t recovery_expected = 0x13579bdf2468ace0ULL;
    const uint64_t recovery_adjacent = 0x0f1e2d3c4b5a6978ULL;
    bool first_preserved;
    bool second_preserved;
    bool third_preserved;

    goto_priv(PRIV_M);
    write64(misaligned_paddr, misaligned_expected);
    write64(misaligned_adjacent_paddr, misaligned_adjacent);
    write64(recovery_paddr, recovery_expected);
    write64(recovery_adjacent_paddr, recovery_adjacent);

    ai_arch_clear_mcontrol6_slot(0);
    ai_arch_clear_mcontrol6_slot(1);
    ai_arch_arm_mcontrol6_slot(0, trigger_pc, AI_ARCH_MCONTROL6_EXECUTE);
    ai_arch_arm_mcontrol6_slot(1, misaligned_vaddr, AI_ARCH_MCONTROL6_LOAD);

    uint64_t saved_mstatus = ai_arch_enable_machine_breakpoint_delivery();
    goto_priv(PRIV_HS);
    TEST_SETUP_EXCEPT();
    ai_arch_execute_trigger_load_once(misaligned_vaddr);
    ai_arch_trigger_excpt_snapshot_t first_excpt = ai_arch_snapshot_excpt();
    goto_priv(PRIV_M);
    first_preserved =
        (read64(misaligned_paddr) == misaligned_expected &&
        read64(misaligned_adjacent_paddr) == misaligned_adjacent);
    goto_priv(PRIV_HS);
    TEST_ASSERT("with both load trigger slots armed on the same HS PBMT-NC misaligned load, the first access reports breakpoint and still leaves both crossed backing doublewords unchanged",
        first_excpt.triggered == true &&
        first_excpt.cause == CAUSE_BKP &&
        first_preserved
    );

    TEST_SETUP_EXCEPT();
    ai_arch_execute_trigger_load_once(misaligned_vaddr);
    ai_arch_trigger_excpt_snapshot_t second_excpt = ai_arch_snapshot_excpt();
    goto_priv(PRIV_M);
    second_preserved =
        (read64(misaligned_paddr) == misaligned_expected &&
        read64(misaligned_adjacent_paddr) == misaligned_adjacent);
    goto_priv(PRIV_HS);
    TEST_ASSERT("with both load trigger slots still armed on that same HS PBMT-NC misaligned load, the second access still reports breakpoint and still leaves both crossed backing doublewords unchanged",
        second_excpt.triggered == true &&
        second_excpt.cause == CAUSE_BKP &&
        second_preserved
    );
    ai_arch_restore_machine_status(saved_mstatus);

    ai_arch_clear_mcontrol6_slot(0);
    saved_mstatus = ai_arch_enable_machine_breakpoint_delivery();
    goto_priv(PRIV_HS);
    TEST_SETUP_EXCEPT();
    ai_arch_execute_trigger_load_once(misaligned_vaddr);
    ai_arch_trigger_excpt_snapshot_t third_excpt = ai_arch_snapshot_excpt();
    goto_priv(PRIV_M);
    third_preserved =
        (read64(misaligned_paddr) == misaligned_expected &&
        read64(misaligned_adjacent_paddr) == misaligned_adjacent);
    goto_priv(PRIV_HS);
    TEST_ASSERT("after repeated dual-slot load breakpoint hits, clearing only the execute-PC slot still leaves the address trigger active on the same HS PBMT-NC misaligned load without dirtying either crossed doubleword",
        third_excpt.triggered == true &&
        third_excpt.cause == CAUSE_BKP &&
        third_preserved
    );
    ai_arch_restore_machine_status(saved_mstatus);

    ai_arch_clear_mcontrol6_slot(1);
    goto_priv(PRIV_HS);
    TEST_SETUP_EXCEPT();
    ai_arch_execute_trigger_load_once(misaligned_vaddr);
    TEST_ASSERT("after clearing both repeated load trigger slots, the same HS PBMT-NC access falls back to load misaligned with the raw misaligned VA in tval in the adjacent-preservation corner",
        excpt.triggered == true &&
        excpt.cause == CAUSE_LAM &&
        excpt.tval == misaligned_vaddr
    );

    goto_priv(PRIV_M);
    TEST_ASSERT("the repeated dual-slot load trigger+misaligned corner still leaves both crossed PBMT-NC backing doublewords unchanged after the fallback misalign fault",
        read64(misaligned_paddr) == misaligned_expected &&
        read64(misaligned_adjacent_paddr) == misaligned_adjacent
    );
    ai_arch_trigger_reset_pbmt_only_env();
    write64(recovery_paddr, recovery_expected);
    write64(recovery_adjacent_paddr, recovery_adjacent);

    goto_priv(PRIV_HS);
    TEST_SETUP_EXCEPT();
    uint64_t recovery_value = ai_arch_execute_trigger_load_once(recovery_vaddr);
    TEST_ASSERT("after repeated dual-slot load trigger+misalign hits and the fallback misalign fault, a fresh aligned HS load still recovers normally once the PBMT-NC setup is removed in the adjacent-preservation corner",
        excpt.triggered == false &&
        recovery_value == recovery_expected
    );

    TEST_SETUP_EXCEPT();
    uint64_t observed_adjacent = ai_arch_execute_trigger_load_once(recovery_adjacent_vaddr);
    TEST_ASSERT("the aligned recovery load after the earlier repeated trigger+misalign sequence still leaves the adjacent cacheable HS word unchanged",
        excpt.triggered == false &&
        observed_adjacent == recovery_adjacent
    );

    goto_priv(PRIV_M);
    TEST_ASSERT("after the aligned recovery load, both the target and adjacent cacheable backing words still match their expected payloads",
        read64(recovery_paddr) == recovery_expected &&
        read64(recovery_adjacent_paddr) == recovery_adjacent
    );

    TEST_END();
}

bool ai_arch_store_trigger_misaligned_priority()
{
    TEST_START();

    uintptr_t paddr = phys_page_base(X);
    uintptr_t misaligned_vaddr = ai_arch_trigger_prepare_pbmt_nc_alias() + 7U;
    const uint64_t initial_value = 0x5566778899aabbccULL;

    goto_priv(PRIV_M);
    write64(paddr, initial_value);

    ai_arch_arm_mcontrol6_0(misaligned_vaddr, AI_ARCH_MCONTROL6_STORE);
    uint64_t saved_mstatus = ai_arch_enable_machine_breakpoint_delivery();

    goto_priv(PRIV_HS);
    TEST_SETUP_EXCEPT();
    sd(misaligned_vaddr, 0xa1b2c3d4e5f60718ULL);
    if (!(excpt.triggered == true && excpt.cause == CAUSE_BKP)) {
        ai_arch_log_unexpected_trigger_result("unexpected store address-trigger+misaligned priority result");
    }
    TEST_ASSERT("a store address trigger takes priority when the same HS PBMT-NC store would otherwise raise store misaligned",
        excpt.triggered == true &&
        excpt.cause == CAUSE_BKP
    );
    ai_arch_restore_machine_status(saved_mstatus);
    ai_arch_clear_mcontrol6_0();

    goto_priv(PRIV_HS);
    TEST_SETUP_EXCEPT();
    sd(misaligned_vaddr, 0x0fedcba987654321ULL);
    TEST_ASSERT("after clearing the store address trigger, the same HS PBMT-NC store reports store misaligned with the raw misaligned VA in tval",
        excpt.triggered == true &&
        excpt.cause == CAUSE_SAM &&
        excpt.tval == misaligned_vaddr
    );

    goto_priv(PRIV_M);
    TEST_ASSERT("the store address-trigger+misaligned probe leaves the PBMT-NC backing word unchanged",
        read64(paddr) == initial_value
    );
    ai_arch_trigger_reset_pbmt_only_env();

    TEST_END();
}

bool ai_arch_store_trigger_misaligned_repeated_breakpoint_then_fault_no_side_effect_corner()
{
    TEST_START();

    uintptr_t paddr = phys_page_base(X);
    uintptr_t misaligned_vaddr = ai_arch_trigger_prepare_pbmt_nc_alias() + 7U;
    const uint64_t initial_value = 0x66778899aabbccddULL;
    const uint64_t first_store_value = 0xa1b2c3d4e5f60718ULL;
    const uint64_t second_store_value = 0x1122334455667788ULL;
    bool first_preserved;
    bool second_preserved;

    goto_priv(PRIV_M);
    write64(paddr, initial_value);

    ai_arch_arm_mcontrol6_0(misaligned_vaddr, AI_ARCH_MCONTROL6_STORE);
    uint64_t saved_mstatus = ai_arch_enable_machine_breakpoint_delivery();

    goto_priv(PRIV_HS);
    TEST_SETUP_EXCEPT();
    sd(misaligned_vaddr, first_store_value);
    ai_arch_trigger_excpt_snapshot_t first_excpt = ai_arch_snapshot_excpt();
    goto_priv(PRIV_M);
    first_preserved = (read64(paddr) == initial_value);
    goto_priv(PRIV_HS);
    TEST_ASSERT("with a store address trigger armed on an HS PBMT-NC misaligned store, the first access reports breakpoint and still suppresses the store update",
        first_excpt.triggered == true &&
        first_excpt.cause == CAUSE_BKP &&
        first_preserved
    );

    TEST_SETUP_EXCEPT();
    sd(misaligned_vaddr, second_store_value);
    ai_arch_trigger_excpt_snapshot_t second_excpt = ai_arch_snapshot_excpt();
    goto_priv(PRIV_M);
    second_preserved = (read64(paddr) == initial_value);
    goto_priv(PRIV_HS);
    TEST_ASSERT("with the same store address trigger still armed on the same HS PBMT-NC misaligned store, the second access still reports breakpoint and still leaves memory unchanged",
        second_excpt.triggered == true &&
        second_excpt.cause == CAUSE_BKP &&
        second_preserved
    );
    ai_arch_restore_machine_status(saved_mstatus);
    ai_arch_clear_mcontrol6_0();

    goto_priv(PRIV_HS);
    TEST_SETUP_EXCEPT();
    sd(misaligned_vaddr, first_store_value);
    TEST_ASSERT("after clearing the repeated store address trigger, the same HS PBMT-NC access falls back to store misaligned with the raw misaligned VA in tval",
        excpt.triggered == true &&
        excpt.cause == CAUSE_SAM &&
        excpt.tval == misaligned_vaddr
    );

    goto_priv(PRIV_M);
    TEST_ASSERT("the repeated store address-trigger+misaligned probe still leaves the PBMT-NC backing word unchanged after the fallback misalign fault",
        read64(paddr) == initial_value
    );
    ai_arch_trigger_reset_pbmt_only_env();

    TEST_END();
}

bool ai_arch_store_execute_trigger_misaligned_priority()
{
    TEST_START();

    uintptr_t paddr = phys_page_base(X);
    uintptr_t trigger_pc = (uintptr_t)ai_arch_execute_trigger_store_once;
    uintptr_t misaligned_vaddr = ai_arch_trigger_prepare_pbmt_nc_alias() + 7U;
    const uint64_t initial_value = 0x778899aabbccddeeULL;

    goto_priv(PRIV_M);
    write64(paddr, initial_value);

    ai_arch_arm_mcontrol6_0(trigger_pc, AI_ARCH_MCONTROL6_EXECUTE);
    uint64_t saved_mstatus = ai_arch_enable_machine_breakpoint_delivery();

    goto_priv(PRIV_HS);
    TEST_SETUP_EXCEPT();
    ai_arch_execute_trigger_store_once(misaligned_vaddr, 0xa1b2c3d4e5f60718ULL);
    if (!(excpt.triggered == true && excpt.cause == CAUSE_BKP)) {
        ai_arch_log_unexpected_trigger_result("unexpected store execute-PC trigger+misaligned priority result");
    }
    TEST_ASSERT("a store execute-PC trigger takes priority when the same HS PBMT-NC store would otherwise raise store misaligned",
        excpt.triggered == true &&
        excpt.cause == CAUSE_BKP
    );
    ai_arch_restore_machine_status(saved_mstatus);
    ai_arch_clear_mcontrol6_0();

    goto_priv(PRIV_HS);
    TEST_SETUP_EXCEPT();
    ai_arch_execute_trigger_store_once(misaligned_vaddr, 0x0fedcba987654321ULL);
    TEST_ASSERT("after clearing the store execute-PC trigger, the same HS PBMT-NC store reports store misaligned with the raw misaligned VA in tval",
        excpt.triggered == true &&
        excpt.cause == CAUSE_SAM &&
        excpt.tval == misaligned_vaddr
    );

    goto_priv(PRIV_M);
    TEST_ASSERT("the store execute-trigger+misaligned probe leaves the PBMT-NC backing word unchanged",
        read64(paddr) == initial_value
    );
    ai_arch_trigger_reset_pbmt_only_env();

    TEST_END();
}

bool ai_arch_store_execute_trigger_misaligned_repeated_breakpoint_then_fault_no_side_effect_corner()
{
    TEST_START();

    uintptr_t paddr = phys_page_base(X);
    uintptr_t trigger_pc = (uintptr_t)ai_arch_execute_trigger_store_once;
    uintptr_t misaligned_vaddr = ai_arch_trigger_prepare_pbmt_nc_alias() + 7U;
    const uint64_t initial_value = 0x99aabbccddeeff00ULL;
    const uint64_t first_store_value = 0xa1b2c3d4e5f60718ULL;
    const uint64_t second_store_value = 0x1122334455667788ULL;
    bool first_preserved;
    bool second_preserved;

    goto_priv(PRIV_M);
    write64(paddr, initial_value);

    ai_arch_arm_mcontrol6_0(trigger_pc, AI_ARCH_MCONTROL6_EXECUTE);
    uint64_t saved_mstatus = ai_arch_enable_machine_breakpoint_delivery();

    goto_priv(PRIV_HS);
    TEST_SETUP_EXCEPT();
    ai_arch_execute_trigger_store_once(misaligned_vaddr, first_store_value);
    ai_arch_trigger_excpt_snapshot_t first_excpt = ai_arch_snapshot_excpt();
    goto_priv(PRIV_M);
    first_preserved = (read64(paddr) == initial_value);
    goto_priv(PRIV_HS);
    TEST_ASSERT("with a store execute-PC trigger armed on an HS PBMT-NC misaligned store, the first access reports breakpoint and still suppresses the store update",
        first_excpt.triggered == true &&
        first_excpt.cause == CAUSE_BKP &&
        first_preserved
    );

    TEST_SETUP_EXCEPT();
    ai_arch_execute_trigger_store_once(misaligned_vaddr, second_store_value);
    ai_arch_trigger_excpt_snapshot_t second_excpt = ai_arch_snapshot_excpt();
    goto_priv(PRIV_M);
    second_preserved = (read64(paddr) == initial_value);
    goto_priv(PRIV_HS);
    TEST_ASSERT("with the same store execute-PC trigger still armed on the same HS PBMT-NC misaligned store, the second access still reports breakpoint and still leaves memory unchanged",
        second_excpt.triggered == true &&
        second_excpt.cause == CAUSE_BKP &&
        second_preserved
    );
    ai_arch_restore_machine_status(saved_mstatus);
    ai_arch_clear_mcontrol6_0();

    goto_priv(PRIV_HS);
    TEST_SETUP_EXCEPT();
    ai_arch_execute_trigger_store_once(misaligned_vaddr, first_store_value);
    TEST_ASSERT("after clearing the repeated store execute-PC trigger, the same HS PBMT-NC access falls back to store misaligned with the raw misaligned VA in tval",
        excpt.triggered == true &&
        excpt.cause == CAUSE_SAM &&
        excpt.tval == misaligned_vaddr
    );

    goto_priv(PRIV_M);
    TEST_ASSERT("the repeated store execute-trigger+misaligned probe still leaves the PBMT-NC backing word unchanged after the fallback fault",
        read64(paddr) == initial_value
    );
    ai_arch_trigger_reset_pbmt_only_env();

    TEST_END();
}

bool ai_arch_store_execute_and_address_trigger_misaligned_priority_corner()
{
    TEST_START();

    uintptr_t paddr = phys_page_base(X);
    uintptr_t trigger_pc = (uintptr_t)ai_arch_execute_trigger_store_once;
    uintptr_t misaligned_vaddr = ai_arch_trigger_prepare_pbmt_nc_alias() + 7U;
    const uint64_t initial_value = 0x8899aabbccddeeffULL;
    const uint64_t store_value = 0xa1b2c3d4e5f60718ULL;
    bool first_preserved;
    bool second_preserved;

    goto_priv(PRIV_M);
    write64(paddr, initial_value);

    ai_arch_clear_mcontrol6_slot(0);
    ai_arch_clear_mcontrol6_slot(1);
    ai_arch_arm_mcontrol6_slot(0, trigger_pc, AI_ARCH_MCONTROL6_EXECUTE);
    ai_arch_arm_mcontrol6_slot(1, misaligned_vaddr, AI_ARCH_MCONTROL6_STORE);
    ai_arch_sync_trigger_state();

    uint64_t saved_mstatus = ai_arch_enable_machine_breakpoint_delivery();
    goto_priv(PRIV_HS);
    TEST_SETUP_EXCEPT();
    ai_arch_execute_trigger_store_once(misaligned_vaddr, store_value);
    ai_arch_trigger_excpt_snapshot_t first_excpt = ai_arch_snapshot_excpt();
    goto_priv(PRIV_M);
    first_preserved = (read64(paddr) == initial_value);
    goto_priv(PRIV_HS);
    TEST_ASSERT("when both execute-PC and address triggers match the same HS PBMT-NC misaligned store, breakpoint still wins before the store updates memory",
        first_excpt.triggered == true &&
        first_excpt.cause == CAUSE_BKP &&
        first_preserved
    );
    ai_arch_restore_machine_status(saved_mstatus);

    ai_arch_clear_mcontrol6_slot(0);
    saved_mstatus = ai_arch_enable_machine_breakpoint_delivery();
    goto_priv(PRIV_HS);
    TEST_SETUP_EXCEPT();
    ai_arch_execute_trigger_store_once(misaligned_vaddr, store_value);
    ai_arch_trigger_excpt_snapshot_t second_excpt = ai_arch_snapshot_excpt();
    goto_priv(PRIV_M);
    second_preserved = (read64(paddr) == initial_value);
    goto_priv(PRIV_HS);
    TEST_ASSERT("after clearing only the execute-PC trigger, the remaining store-address trigger still reports breakpoint on the same HS PBMT-NC misaligned store and still suppresses the write",
        second_excpt.triggered == true &&
        second_excpt.cause == CAUSE_BKP &&
        second_preserved
    );
    ai_arch_restore_machine_status(saved_mstatus);

    ai_arch_clear_mcontrol6_slot(1);
    goto_priv(PRIV_HS);
    TEST_SETUP_EXCEPT();
    ai_arch_execute_trigger_store_once(misaligned_vaddr, store_value);
    TEST_ASSERT("after clearing both store trigger slots, the same HS PBMT-NC access falls back to store misaligned with the raw misaligned VA in tval",
        excpt.triggered == true &&
        excpt.cause == CAUSE_SAM &&
        excpt.tval == misaligned_vaddr
    );

    goto_priv(PRIV_M);
    TEST_ASSERT("the dual-slot store trigger+misaligned priority corner leaves the PBMT-NC backing word unchanged across both breakpoint hits and the fallback misalign fault",
        read64(paddr) == initial_value
    );
    ai_arch_trigger_reset_pbmt_only_env();

    TEST_END();
}

bool ai_arch_store_execute_and_address_trigger_misaligned_repeated_breakpoint_then_fault_no_side_effect_corner()
{
    TEST_START();

    uintptr_t paddr = phys_page_base(X);
    uintptr_t trigger_pc = (uintptr_t)ai_arch_execute_trigger_store_once;
    uintptr_t misaligned_vaddr = ai_arch_trigger_prepare_pbmt_nc_alias() + 7U;
    const uint64_t initial_value = 0xaabbccddeeff0011ULL;
    const uint64_t first_store_value = 0xdeadbeefcafef00dULL;
    const uint64_t second_store_value = 0x1122334455667788ULL;
    const uint64_t third_store_value = 0x0fedcba987654321ULL;
    bool first_preserved;
    bool second_preserved;
    bool third_preserved;

    goto_priv(PRIV_M);
    write64(paddr, initial_value);

    ai_arch_clear_mcontrol6_slot(0);
    ai_arch_clear_mcontrol6_slot(1);
    ai_arch_arm_mcontrol6_slot(0, trigger_pc, AI_ARCH_MCONTROL6_EXECUTE);
    ai_arch_arm_mcontrol6_slot(1, misaligned_vaddr, AI_ARCH_MCONTROL6_STORE);
    ai_arch_sync_trigger_state();

    uint64_t saved_mstatus = ai_arch_enable_machine_breakpoint_delivery();
    goto_priv(PRIV_HS);
    TEST_SETUP_EXCEPT();
    ai_arch_execute_trigger_store_once(misaligned_vaddr, first_store_value);
    ai_arch_trigger_excpt_snapshot_t first_excpt = ai_arch_snapshot_excpt();
    goto_priv(PRIV_M);
    first_preserved = (read64(paddr) == initial_value);
    goto_priv(PRIV_HS);
    TEST_ASSERT("with both store trigger slots armed on the same HS PBMT-NC misaligned store, the first access reports breakpoint and still suppresses the write",
        first_excpt.triggered == true &&
        first_excpt.cause == CAUSE_BKP &&
        first_preserved
    );
    ai_arch_sync_trigger_state();

    TEST_SETUP_EXCEPT();
    ai_arch_execute_trigger_store_once(misaligned_vaddr, second_store_value);
    ai_arch_trigger_excpt_snapshot_t second_excpt = ai_arch_snapshot_excpt();
    goto_priv(PRIV_M);
    second_preserved = (read64(paddr) == initial_value);
    goto_priv(PRIV_HS);
    TEST_ASSERT("with both store trigger slots still armed on that same HS PBMT-NC misaligned store, the second access still reports breakpoint and still leaves memory unchanged",
        second_excpt.triggered == true &&
        second_excpt.cause == CAUSE_BKP &&
        second_preserved
    );
    ai_arch_restore_machine_status(saved_mstatus);

    ai_arch_clear_mcontrol6_slot(0);
    ai_arch_sync_trigger_state();
    saved_mstatus = ai_arch_enable_machine_breakpoint_delivery();
    goto_priv(PRIV_HS);
    TEST_SETUP_EXCEPT();
    ai_arch_execute_trigger_store_once(misaligned_vaddr, third_store_value);
    ai_arch_trigger_excpt_snapshot_t third_excpt = ai_arch_snapshot_excpt();
    goto_priv(PRIV_M);
    third_preserved = (read64(paddr) == initial_value);
    goto_priv(PRIV_HS);
    TEST_ASSERT("after repeated dual-slot store breakpoint hits, clearing only the execute-PC slot still leaves the address trigger active on the same HS PBMT-NC misaligned store",
        third_excpt.triggered == true &&
        third_excpt.cause == CAUSE_BKP &&
        third_preserved
    );
    ai_arch_restore_machine_status(saved_mstatus);

    ai_arch_clear_mcontrol6_slot(1);
    ai_arch_sync_trigger_state();
    goto_priv(PRIV_HS);
    TEST_SETUP_EXCEPT();
    ai_arch_execute_trigger_store_once(misaligned_vaddr, first_store_value);
    TEST_ASSERT("after clearing both repeated store trigger slots, the same HS PBMT-NC access falls back to store misaligned with the raw misaligned VA in tval",
        excpt.triggered == true &&
        excpt.cause == CAUSE_SAM &&
        excpt.tval == misaligned_vaddr
    );

    goto_priv(PRIV_M);
    TEST_ASSERT("the repeated dual-slot store trigger+misaligned corner still leaves the PBMT-NC backing word unchanged after the fallback misalign fault",
        read64(paddr) == initial_value
    );
    ai_arch_trigger_reset_pbmt_only_env();

    TEST_END();
}

bool ai_arch_store_execute_and_address_trigger_misaligned_repeated_breakpoint_then_fault_no_side_effect_adjacent_preserved_corner()
{
    TEST_START();

    uintptr_t misaligned_paddr = phys_page_base(X);
    uintptr_t misaligned_adjacent_paddr = misaligned_paddr + sizeof(uint64_t);
    uintptr_t trigger_pc = (uintptr_t)ai_arch_execute_trigger_store_once;
    uintptr_t misaligned_vaddr = ai_arch_trigger_prepare_pbmt_nc_alias() + 7U;
    const uint64_t misaligned_initial = 0xaabbccddeeff0011ULL;
    const uint64_t misaligned_adjacent = 0x123456789abcdef0ULL;
    const uint64_t first_store_value = 0xdeadbeefcafef00dULL;
    const uint64_t second_store_value = 0x1122334455667788ULL;
    const uint64_t third_store_value = 0x0fedcba987654321ULL;
    bool first_preserved;
    bool second_preserved;
    bool third_preserved;

    goto_priv(PRIV_M);
    write64(misaligned_paddr, misaligned_initial);
    write64(misaligned_adjacent_paddr, misaligned_adjacent);

    ai_arch_clear_mcontrol6_slot(0);
    ai_arch_clear_mcontrol6_slot(1);
    ai_arch_arm_mcontrol6_slot(0, trigger_pc, AI_ARCH_MCONTROL6_EXECUTE);
    ai_arch_arm_mcontrol6_slot(1, misaligned_vaddr, AI_ARCH_MCONTROL6_STORE);

    uint64_t saved_mstatus = ai_arch_enable_machine_breakpoint_delivery();
    goto_priv(PRIV_HS);
    TEST_SETUP_EXCEPT();
    ai_arch_execute_trigger_store_once(misaligned_vaddr, first_store_value);
    ai_arch_trigger_excpt_snapshot_t first_excpt = ai_arch_snapshot_excpt();
    goto_priv(PRIV_M);
    first_preserved =
        (read64(misaligned_paddr) == misaligned_initial &&
        read64(misaligned_adjacent_paddr) == misaligned_adjacent);
    goto_priv(PRIV_HS);
    TEST_ASSERT("with both store trigger slots armed on the same HS PBMT-NC misaligned store, the first access reports breakpoint and still suppresses writes to both crossed backing doublewords in the adjacent-preservation corner",
        first_excpt.triggered == true &&
        first_excpt.cause == CAUSE_BKP &&
        first_preserved
    );

    TEST_SETUP_EXCEPT();
    ai_arch_execute_trigger_store_once(misaligned_vaddr, second_store_value);
    ai_arch_trigger_excpt_snapshot_t second_excpt = ai_arch_snapshot_excpt();
    goto_priv(PRIV_M);
    second_preserved =
        (read64(misaligned_paddr) == misaligned_initial &&
        read64(misaligned_adjacent_paddr) == misaligned_adjacent);
    goto_priv(PRIV_HS);
    TEST_ASSERT("with both store trigger slots still armed on that same HS PBMT-NC misaligned store, the second access still reports breakpoint and still leaves both crossed backing doublewords unchanged in the adjacent-preservation corner",
        second_excpt.triggered == true &&
        second_excpt.cause == CAUSE_BKP &&
        second_preserved
    );
    ai_arch_restore_machine_status(saved_mstatus);

    ai_arch_clear_mcontrol6_slot(0);
    saved_mstatus = ai_arch_enable_machine_breakpoint_delivery();
    goto_priv(PRIV_HS);
    TEST_SETUP_EXCEPT();
    ai_arch_execute_trigger_store_once(misaligned_vaddr, third_store_value);
    ai_arch_trigger_excpt_snapshot_t third_excpt = ai_arch_snapshot_excpt();
    goto_priv(PRIV_M);
    third_preserved =
        (read64(misaligned_paddr) == misaligned_initial &&
        read64(misaligned_adjacent_paddr) == misaligned_adjacent);
    goto_priv(PRIV_HS);
    TEST_ASSERT("after repeated dual-slot store breakpoint hits, clearing only the execute-PC slot still leaves the address trigger active on the same HS PBMT-NC misaligned store without dirtying either crossed doubleword in the adjacent-preservation corner",
        third_excpt.triggered == true &&
        third_excpt.cause == CAUSE_BKP &&
        third_preserved
    );
    ai_arch_restore_machine_status(saved_mstatus);

    ai_arch_clear_mcontrol6_slot(1);
    goto_priv(PRIV_HS);
    TEST_SETUP_EXCEPT();
    ai_arch_execute_trigger_store_once(misaligned_vaddr, first_store_value);
    TEST_ASSERT("after clearing both repeated store trigger slots, the same HS PBMT-NC access falls back to store misaligned with the raw misaligned VA in tval in the adjacent-preservation corner",
        excpt.triggered == true &&
        excpt.cause == CAUSE_SAM &&
        excpt.tval == misaligned_vaddr
    );

    goto_priv(PRIV_M);
    TEST_ASSERT("the repeated dual-slot store trigger+misaligned corner still leaves both crossed PBMT-NC backing doublewords unchanged after the fallback misalign fault in the adjacent-preservation corner",
        read64(misaligned_paddr) == misaligned_initial &&
        read64(misaligned_adjacent_paddr) == misaligned_adjacent
    );
    ai_arch_trigger_reset_pbmt_only_env();

    TEST_END();
}

bool ai_arch_store_execute_and_address_trigger_misaligned_repeated_breakpoint_then_valid_hs_store_recovers_corner()
{
    TEST_START();

    uintptr_t misaligned_paddr = phys_page_base(X);
    uintptr_t trigger_pc = (uintptr_t)ai_arch_execute_trigger_store_once;
    uintptr_t misaligned_vaddr = ai_arch_trigger_prepare_pbmt_nc_alias() + 7U;
    uintptr_t recovery_paddr = phys_page_base(VSRW_GRW) + 0x180U;
    uintptr_t recovery_vaddr = hs_page_base(VSRW_GRW) + 0x180U;
    const uint64_t misaligned_initial = 0xbbccddee11223344ULL;
    const uint64_t first_store_value = 0xdeadbeefcafef00dULL;
    const uint64_t second_store_value = 0x1122334455667788ULL;
    const uint64_t third_store_value = 0x0fedcba987654321ULL;
    const uint64_t recovery_initial = 0x0102030405060708ULL;
    const uint64_t recovery_store = 0x89abcdef01234567ULL;
    bool first_preserved;
    bool second_preserved;
    bool third_preserved;

    goto_priv(PRIV_M);
    write64(misaligned_paddr, misaligned_initial);
    write64(recovery_paddr, recovery_initial);

    ai_arch_clear_mcontrol6_slot(0);
    ai_arch_clear_mcontrol6_slot(1);
    ai_arch_arm_mcontrol6_slot(0, trigger_pc, AI_ARCH_MCONTROL6_EXECUTE);
    ai_arch_arm_mcontrol6_slot(1, misaligned_vaddr, AI_ARCH_MCONTROL6_STORE);

    uint64_t saved_mstatus = ai_arch_enable_machine_breakpoint_delivery();
    goto_priv(PRIV_HS);
    TEST_SETUP_EXCEPT();
    ai_arch_execute_trigger_store_once(misaligned_vaddr, first_store_value);
    ai_arch_trigger_excpt_snapshot_t first_excpt = ai_arch_snapshot_excpt();
    goto_priv(PRIV_M);
    first_preserved = (read64(misaligned_paddr) == misaligned_initial);
    goto_priv(PRIV_HS);
    TEST_ASSERT("with both store trigger slots armed on the same HS PBMT-NC misaligned store, the first access reports breakpoint and still suppresses the write",
        first_excpt.triggered == true &&
        first_excpt.cause == CAUSE_BKP &&
        first_preserved
    );

    TEST_SETUP_EXCEPT();
    ai_arch_execute_trigger_store_once(misaligned_vaddr, second_store_value);
    ai_arch_trigger_excpt_snapshot_t second_excpt = ai_arch_snapshot_excpt();
    goto_priv(PRIV_M);
    second_preserved = (read64(misaligned_paddr) == misaligned_initial);
    goto_priv(PRIV_HS);
    TEST_ASSERT("with both store trigger slots still armed on that same HS PBMT-NC misaligned store, the second access still reports breakpoint and still leaves memory unchanged",
        second_excpt.triggered == true &&
        second_excpt.cause == CAUSE_BKP &&
        second_preserved
    );
    ai_arch_restore_machine_status(saved_mstatus);

    ai_arch_clear_mcontrol6_slot(0);
    saved_mstatus = ai_arch_enable_machine_breakpoint_delivery();
    goto_priv(PRIV_HS);
    TEST_SETUP_EXCEPT();
    ai_arch_execute_trigger_store_once(misaligned_vaddr, third_store_value);
    ai_arch_trigger_excpt_snapshot_t third_excpt = ai_arch_snapshot_excpt();
    goto_priv(PRIV_M);
    third_preserved = (read64(misaligned_paddr) == misaligned_initial);
    goto_priv(PRIV_HS);
    TEST_ASSERT("after repeated dual-slot store breakpoint hits, clearing only the execute-PC slot still leaves the address trigger active on the same HS PBMT-NC misaligned store",
        third_excpt.triggered == true &&
        third_excpt.cause == CAUSE_BKP &&
        third_preserved
    );
    ai_arch_restore_machine_status(saved_mstatus);

    ai_arch_clear_mcontrol6_slot(1);
    goto_priv(PRIV_HS);
    TEST_SETUP_EXCEPT();
    ai_arch_execute_trigger_store_once(misaligned_vaddr, first_store_value);
    TEST_ASSERT("after clearing both repeated store trigger slots, the same HS PBMT-NC access falls back to store misaligned with the raw misaligned VA in tval",
        excpt.triggered == true &&
        excpt.cause == CAUSE_SAM &&
        excpt.tval == misaligned_vaddr
    );

    goto_priv(PRIV_M);
    TEST_ASSERT("the repeated dual-slot store trigger+misaligned corner still leaves the PBMT-NC backing word unchanged after the fallback misalign fault",
        read64(misaligned_paddr) == misaligned_initial
    );
    ai_arch_trigger_reset_pbmt_only_env();
    write64(recovery_paddr, recovery_initial);

    goto_priv(PRIV_HS);
    TEST_SETUP_EXCEPT();
    ai_arch_execute_trigger_store_once(recovery_vaddr, recovery_store);
    TEST_ASSERT("after repeated dual-slot store trigger+misalign hits and the fallback misalign fault, a fresh aligned HS store still recovers normally once the PBMT-NC setup is removed",
        excpt.triggered == false
    );

    goto_priv(PRIV_M);
    TEST_ASSERT("the recovered aligned HS store commits its full payload after the earlier repeated trigger+misalign sequence is cleared",
        read64(recovery_paddr) == recovery_store
    );

    TEST_END();
}

bool ai_arch_store_execute_and_address_trigger_misaligned_repeated_breakpoint_then_valid_hs_store_adjacent_preserved_corner()
{
    TEST_START();

    uintptr_t misaligned_paddr = phys_page_base(X);
    uintptr_t misaligned_adjacent_paddr = misaligned_paddr + sizeof(uint64_t);
    uintptr_t trigger_pc = (uintptr_t)ai_arch_execute_trigger_store_once;
    uintptr_t misaligned_vaddr = ai_arch_trigger_prepare_pbmt_nc_alias() + 7U;
    uintptr_t recovery_paddr = phys_page_base(VSRW_GRW) + 0x1a0U;
    uintptr_t recovery_vaddr = hs_page_base(VSRW_GRW) + 0x1a0U;
    uintptr_t recovery_adjacent_paddr = recovery_paddr + sizeof(uint64_t);
    uintptr_t recovery_adjacent_vaddr = recovery_vaddr + sizeof(uint64_t);
    const uint64_t misaligned_initial = 0xbbccddee11223344ULL;
    const uint64_t misaligned_adjacent = 0x123456789abcdef0ULL;
    const uint64_t first_store_value = 0xdeadbeefcafef00dULL;
    const uint64_t second_store_value = 0x1122334455667788ULL;
    const uint64_t third_store_value = 0x0fedcba987654321ULL;
    const uint64_t recovery_initial = 0x0102030405060708ULL;
    const uint64_t recovery_store = 0x89abcdef01234567ULL;
    const uint64_t recovery_adjacent = 0xfedcba9876543210ULL;
    bool first_preserved;
    bool second_preserved;
    bool third_preserved;

    goto_priv(PRIV_M);
    write64(misaligned_paddr, misaligned_initial);
    write64(misaligned_adjacent_paddr, misaligned_adjacent);
    write64(recovery_paddr, recovery_initial);
    write64(recovery_adjacent_paddr, recovery_adjacent);

    ai_arch_clear_mcontrol6_slot(0);
    ai_arch_clear_mcontrol6_slot(1);
    ai_arch_arm_mcontrol6_slot(0, trigger_pc, AI_ARCH_MCONTROL6_EXECUTE);
    ai_arch_arm_mcontrol6_slot(1, misaligned_vaddr, AI_ARCH_MCONTROL6_STORE);

    uint64_t saved_mstatus = ai_arch_enable_machine_breakpoint_delivery();
    goto_priv(PRIV_HS);
    TEST_SETUP_EXCEPT();
    ai_arch_execute_trigger_store_once(misaligned_vaddr, first_store_value);
    ai_arch_trigger_excpt_snapshot_t first_excpt = ai_arch_snapshot_excpt();
    goto_priv(PRIV_M);
    first_preserved =
        (read64(misaligned_paddr) == misaligned_initial &&
        read64(misaligned_adjacent_paddr) == misaligned_adjacent);
    goto_priv(PRIV_HS);
    TEST_ASSERT("with both store trigger slots armed on the same HS PBMT-NC misaligned store, the first access reports breakpoint and still suppresses writes to both crossed backing doublewords",
        first_excpt.triggered == true &&
        first_excpt.cause == CAUSE_BKP &&
        first_preserved
    );

    TEST_SETUP_EXCEPT();
    ai_arch_execute_trigger_store_once(misaligned_vaddr, second_store_value);
    ai_arch_trigger_excpt_snapshot_t second_excpt = ai_arch_snapshot_excpt();
    goto_priv(PRIV_M);
    second_preserved =
        (read64(misaligned_paddr) == misaligned_initial &&
        read64(misaligned_adjacent_paddr) == misaligned_adjacent);
    goto_priv(PRIV_HS);
    TEST_ASSERT("with both store trigger slots still armed on that same HS PBMT-NC misaligned store, the second access still reports breakpoint and still leaves both crossed backing doublewords unchanged",
        second_excpt.triggered == true &&
        second_excpt.cause == CAUSE_BKP &&
        second_preserved
    );
    ai_arch_restore_machine_status(saved_mstatus);

    ai_arch_clear_mcontrol6_slot(0);
    saved_mstatus = ai_arch_enable_machine_breakpoint_delivery();
    goto_priv(PRIV_HS);
    TEST_SETUP_EXCEPT();
    ai_arch_execute_trigger_store_once(misaligned_vaddr, third_store_value);
    ai_arch_trigger_excpt_snapshot_t third_excpt = ai_arch_snapshot_excpt();
    goto_priv(PRIV_M);
    third_preserved =
        (read64(misaligned_paddr) == misaligned_initial &&
        read64(misaligned_adjacent_paddr) == misaligned_adjacent);
    goto_priv(PRIV_HS);
    TEST_ASSERT("after repeated dual-slot store breakpoint hits, clearing only the execute-PC slot still leaves the address trigger active on the same HS PBMT-NC misaligned store without dirtying either crossed doubleword",
        third_excpt.triggered == true &&
        third_excpt.cause == CAUSE_BKP &&
        third_preserved
    );
    ai_arch_restore_machine_status(saved_mstatus);

    ai_arch_clear_mcontrol6_slot(1);
    goto_priv(PRIV_HS);
    TEST_SETUP_EXCEPT();
    ai_arch_execute_trigger_store_once(misaligned_vaddr, first_store_value);
    TEST_ASSERT("after clearing both repeated store trigger slots, the same HS PBMT-NC access falls back to store misaligned with the raw misaligned VA in tval in the adjacent-preservation corner",
        excpt.triggered == true &&
        excpt.cause == CAUSE_SAM &&
        excpt.tval == misaligned_vaddr
    );

    goto_priv(PRIV_M);
    TEST_ASSERT("the repeated dual-slot store trigger+misaligned corner still leaves both crossed PBMT-NC backing doublewords unchanged after the fallback misalign fault",
        read64(misaligned_paddr) == misaligned_initial &&
        read64(misaligned_adjacent_paddr) == misaligned_adjacent
    );
    ai_arch_trigger_reset_pbmt_only_env();
    write64(recovery_paddr, recovery_initial);
    write64(recovery_adjacent_paddr, recovery_adjacent);

    goto_priv(PRIV_HS);
    TEST_SETUP_EXCEPT();
    ai_arch_execute_trigger_store_once(recovery_vaddr, recovery_store);
    TEST_ASSERT("after repeated dual-slot store trigger+misalign hits and the fallback misalign fault, a fresh aligned HS store still recovers normally once the PBMT-NC setup is removed in the adjacent-preservation corner",
        excpt.triggered == false
    );

    TEST_SETUP_EXCEPT();
    uint64_t observed_adjacent = ai_arch_execute_trigger_load_once(recovery_adjacent_vaddr);
    TEST_ASSERT("the aligned recovery store after the earlier repeated trigger+misalign sequence still leaves the adjacent cacheable HS word unchanged",
        excpt.triggered == false &&
        observed_adjacent == recovery_adjacent
    );

    goto_priv(PRIV_M);
    TEST_ASSERT("the recovered aligned HS store commits its full payload while preserving the adjacent cacheable backing word",
        read64(recovery_paddr) == recovery_store &&
        read64(recovery_adjacent_paddr) == recovery_adjacent
    );

    TEST_END();
}

bool ai_arch_load_trigger_misaligned_page_fault_priority_corner()
{
    TEST_START();

    enum test_page low_page = VSRWXPbmt1_GURWXPbmt0;
    enum test_page high_page = VSRWXPbmt1_GURWXPbmt1;
    uintptr_t low_phys = ai_arch_trigger_page_end_phys(low_page);
    uintptr_t high_phys = phys_page_base(high_page);
    const uint64_t low_pattern = 0x1021324354657687ULL;
    const uint64_t high_pattern = 0x98badcfe10325476ULL;
    uintptr_t vaddr = ai_arch_trigger_prepare_cross_page_pbmt_nc_load_pf_alias();

    goto_priv(PRIV_M);
    write64(low_phys, low_pattern);
    write64(high_phys, high_pattern);

    ai_arch_arm_mcontrol6_0(vaddr, AI_ARCH_MCONTROL6_LOAD);
    uint64_t saved_mstatus = ai_arch_enable_machine_breakpoint_delivery();

    goto_priv(PRIV_HS);
    TEST_SETUP_EXCEPT();
    ld(vaddr);
    if (!(excpt.triggered == true && excpt.cause == CAUSE_BKP)) {
        ai_arch_log_unexpected_trigger_result("unexpected load address-trigger+misalign+page-fault priority result");
    }
    TEST_ASSERT("a load address trigger takes priority when the same HS cross-page access would otherwise stop at low-half PBMT-NC load misalign before ever reaching the high-half page fault",
        excpt.triggered == true &&
        excpt.cause == CAUSE_BKP
    );
    ai_arch_restore_machine_status(saved_mstatus);
    ai_arch_clear_mcontrol6_0();

    goto_priv(PRIV_HS);
    TEST_SETUP_EXCEPT();
    ld(vaddr);
    TEST_ASSERT("after clearing the load address trigger, the same HS cross-page access falls back to the project-RTL low-half load misalign with the low-half start VA in tval",
        excpt.triggered == true &&
        excpt.cause == CAUSE_LAM &&
        excpt.tval == vaddr
    );

    goto_priv(PRIV_M);
    TEST_ASSERT("the load address-trigger+misalign+page-fault corner leaves both boundary backing doublewords unchanged",
        read64(low_phys) == low_pattern &&
        read64(high_phys) == high_pattern
    );
    ai_arch_trigger_reset_pbmt_only_env();

    TEST_END();
}

bool ai_arch_load_execute_trigger_misaligned_page_fault_priority_corner()
{
    TEST_START();

    enum test_page low_page = VSRWXPbmt1_GURWXPbmt0;
    enum test_page high_page = VSRWXPbmt1_GURWXPbmt1;
    uintptr_t low_phys = ai_arch_trigger_page_end_phys(low_page);
    uintptr_t high_phys = phys_page_base(high_page);
    const uint64_t low_pattern = 0x2132435465768798ULL;
    const uint64_t high_pattern = 0xa9cbed0f21436587ULL;
    uintptr_t trigger_pc = (uintptr_t)ai_arch_execute_trigger_load_once;
    uintptr_t vaddr = ai_arch_trigger_prepare_cross_page_pbmt_nc_load_pf_alias();

    goto_priv(PRIV_M);
    write64(low_phys, low_pattern);
    write64(high_phys, high_pattern);

    ai_arch_arm_mcontrol6_0(trigger_pc, AI_ARCH_MCONTROL6_EXECUTE);
    ai_arch_sync_trigger_state();
    uint64_t saved_mstatus = ai_arch_enable_machine_breakpoint_delivery();

    goto_priv(PRIV_HS);
    TEST_SETUP_EXCEPT();
    ai_arch_execute_trigger_load_once(vaddr);
    if (!(excpt.triggered == true && excpt.cause == CAUSE_BKP)) {
        ai_arch_log_unexpected_trigger_result("unexpected load execute-PC trigger+misalign+page-fault priority result");
    }
    TEST_ASSERT("a load execute-PC trigger takes priority when the same HS cross-page access would otherwise stop at low-half PBMT-NC load misalign before the high-half page fault",
        excpt.triggered == true &&
        excpt.cause == CAUSE_BKP
    );
    ai_arch_restore_machine_status(saved_mstatus);
    ai_arch_clear_mcontrol6_0();
    ai_arch_sync_trigger_state();

    goto_priv(PRIV_HS);
    TEST_SETUP_EXCEPT();
    ai_arch_execute_trigger_load_once(vaddr);
    TEST_ASSERT("after clearing the load execute-PC trigger, the same HS cross-page access falls back to the project-RTL low-half load misalign with the low-half start VA in tval",
        excpt.triggered == true &&
        excpt.cause == CAUSE_LAM &&
        excpt.tval == vaddr
    );

    goto_priv(PRIV_M);
    TEST_ASSERT("the load execute-trigger+misalign+page-fault corner leaves both boundary backing doublewords unchanged",
        read64(low_phys) == low_pattern &&
        read64(high_phys) == high_pattern
    );
    ai_arch_trigger_reset_pbmt_only_env();

    TEST_END();
}

bool ai_arch_load_execute_trigger_misaligned_page_fault_repeated_breakpoint_then_fault_corner()
{
    TEST_START();

    enum test_page low_page = VSRWXPbmt1_GURWXPbmt0;
    enum test_page high_page = VSRWXPbmt1_GURWXPbmt1;
    uintptr_t low_phys = ai_arch_trigger_page_end_phys(low_page);
    uintptr_t high_phys = phys_page_base(high_page);
    const uint64_t low_pattern = 0x31425364758697a8ULL;
    const uint64_t high_pattern = 0xb9dcef1021324354ULL;
    uintptr_t trigger_pc = (uintptr_t)ai_arch_execute_trigger_load_once;
    uintptr_t vaddr = ai_arch_trigger_prepare_cross_page_pbmt_nc_load_pf_alias();
    bool first_preserved;
    bool second_preserved;

    goto_priv(PRIV_M);
    write64(low_phys, low_pattern);
    write64(high_phys, high_pattern);

    ai_arch_arm_mcontrol6_0(trigger_pc, AI_ARCH_MCONTROL6_EXECUTE);
    uint64_t saved_mstatus = ai_arch_enable_machine_breakpoint_delivery();

    goto_priv(PRIV_HS);
    TEST_SETUP_EXCEPT();
    ai_arch_execute_trigger_load_once(vaddr);
    ai_arch_trigger_excpt_snapshot_t first_excpt = ai_arch_snapshot_excpt();
    goto_priv(PRIV_M);
    first_preserved = (read64(low_phys) == low_pattern && read64(high_phys) == high_pattern);
    goto_priv(PRIV_HS);
    TEST_ASSERT("with a load execute-PC trigger armed on the HS cross-page PBMT-NC misalign-plus-page-fault path, the first access reports breakpoint and leaves both boundary doublewords unchanged",
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
    TEST_ASSERT("with the same load execute-PC trigger still armed on that HS cross-page PBMT-NC misalign-plus-page-fault path, the second access still reports breakpoint and still leaves both boundary doublewords unchanged",
        second_excpt.triggered == true &&
        second_excpt.cause == CAUSE_BKP &&
        second_preserved
    );
    ai_arch_restore_machine_status(saved_mstatus);
    ai_arch_clear_mcontrol6_0();

    goto_priv(PRIV_HS);
    TEST_SETUP_EXCEPT();
    ai_arch_execute_trigger_load_once(vaddr);
    TEST_ASSERT("after clearing the repeated load execute-PC trigger, the same HS cross-page access falls back to the project-RTL low-half load misalign with the low-half start VA in tval",
        excpt.triggered == true &&
        excpt.cause == CAUSE_LAM &&
        excpt.tval == vaddr
    );

    goto_priv(PRIV_M);
    TEST_ASSERT("the repeated load execute-trigger+misalign+page-fault corner still leaves both boundary backing doublewords unchanged after the fallback misalign fault",
        read64(low_phys) == low_pattern &&
        read64(high_phys) == high_pattern
    );
    ai_arch_trigger_reset_pbmt_only_env();

    TEST_END();
}

bool ai_arch_load_execute_and_address_trigger_misaligned_page_fault_priority_corner()
{
    TEST_START();

    enum test_page low_page = VSRWXPbmt1_GURWXPbmt0;
    enum test_page high_page = VSRWXPbmt1_GURWXPbmt1;
    uintptr_t low_phys = ai_arch_trigger_page_end_phys(low_page);
    uintptr_t high_phys = phys_page_base(high_page);
    const uint64_t low_pattern = 0x32435465768798a9ULL;
    const uint64_t high_pattern = 0xbaed0f213547698bULL;
    uintptr_t trigger_pc = (uintptr_t)ai_arch_execute_trigger_load_once;
    uintptr_t vaddr = ai_arch_trigger_prepare_cross_page_pbmt_nc_load_pf_alias();

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
    TEST_ASSERT("when both execute-PC and address triggers match the same HS cross-page PBMT-NC misaligned load, breakpoint still wins before the low-half misalign shortcut and the high-half page fault are considered",
        excpt.triggered == true &&
        excpt.cause == CAUSE_BKP
    );
    ai_arch_restore_machine_status(saved_mstatus);

    ai_arch_clear_mcontrol6_slot(0);
    saved_mstatus = ai_arch_enable_machine_breakpoint_delivery();
    goto_priv(PRIV_HS);
    TEST_SETUP_EXCEPT();
    ai_arch_execute_trigger_load_once(vaddr);
    TEST_ASSERT("after clearing only the execute-PC trigger, the remaining load-address trigger still reports breakpoint on the same HS cross-page PBMT-NC misaligned load",
        excpt.triggered == true &&
        excpt.cause == CAUSE_BKP
    );
    ai_arch_restore_machine_status(saved_mstatus);

    ai_arch_clear_mcontrol6_slot(1);
    goto_priv(PRIV_HS);
    TEST_SETUP_EXCEPT();
    ai_arch_execute_trigger_load_once(vaddr);
    TEST_ASSERT("after clearing both load trigger slots, the same HS cross-page access falls back to the project-RTL low-half load misalign with the low-half start VA in tval",
        excpt.triggered == true &&
        excpt.cause == CAUSE_LAM &&
        excpt.tval == vaddr
    );

    goto_priv(PRIV_M);
    TEST_ASSERT("the dual-slot load trigger+misalign+page-fault corner leaves both boundary backing doublewords unchanged",
        read64(low_phys) == low_pattern &&
        read64(high_phys) == high_pattern
    );
    ai_arch_trigger_reset_pbmt_only_env();

    TEST_END();
}

bool ai_arch_load_execute_and_address_trigger_misaligned_page_fault_repeated_breakpoint_then_fault_corner()
{
    TEST_START();

    enum test_page low_page = VSRWXPbmt1_GURWXPbmt0;
    enum test_page high_page = VSRWXPbmt1_GURWXPbmt1;
    uintptr_t low_phys = ai_arch_trigger_page_end_phys(low_page);
    uintptr_t high_phys = phys_page_base(high_page);
    const uint64_t low_pattern = 0x768798a9bacbdcedULL;
    const uint64_t high_pattern = 0xfe1032547698badcULL;
    uintptr_t trigger_pc = (uintptr_t)ai_arch_execute_trigger_load_once;
    uintptr_t vaddr = ai_arch_trigger_prepare_cross_page_pbmt_nc_load_pf_alias();
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

    TEST_END();
}
