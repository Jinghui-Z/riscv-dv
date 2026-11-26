#include "ai_micro_prefetch_common.h"

bool ai_micro_m_prefetch_mprv_u_deny_then_allowed_translated_access_succeeds_corner()
{
    TEST_START();

    uintptr_t denied_vaddr = hs_page_base(AI_MICRO_PREFETCH_PAGE) + 0x2a0U;
    uintptr_t denied_paddr = phys_page_base(AI_MICRO_PREFETCH_PAGE) + 0x2a0U;
    uintptr_t allowed_vaddr = hs_page_base(SWITCH1) + 0x2a0U;
    uintptr_t allowed_paddr = phys_page_base(SWITCH1) + 0x2a0U;
    const uint64_t denied_initial = 0xcafebabedeadbeefULL;
    const uint64_t allowed_expected = 0x0123456789abcdefULL;

    goto_priv(PRIV_M);
    write64(denied_paddr, denied_initial);
    write64(allowed_paddr, allowed_expected);
    ai_micro_prefetch_restore_pmp_full_access();
    hspt_init();
    hspt_u_mode_allow();
    sfence_vma();

    goto_priv(PRIV_M);
    ai_micro_prefetch_configure_pmp_window(
        denied_paddr & PAGE_ADDR_MSK,
        (denied_paddr & PAGE_ADDR_MSK) + PAGE_SIZE, AI_MICRO_PREFETCH_PMP_TOR_NO_R);
    CSRC(CSR_MSTATUS, 3ULL << 11);
    CSRS(CSR_MSTATUS, MSTATUS_MPRV);

    TEST_SETUP_EXCEPT();
    prefetch_r(denied_vaddr);
    TEST_ASSERT("an M-mode translated software prefetch first stays silent when PMP denies reads to the selected translated page",
        excpt.triggered == false
    );

    TEST_SETUP_EXCEPT();
    prefetch_r(allowed_vaddr);
    TEST_ASSERT("a later M-mode translated software prefetch to a different allowed page still retires silently while MPRV=1 and MPP=U remain active",
        excpt.triggered == false
    );

    TEST_SETUP_EXCEPT();
    uint64_t observed = ld(allowed_vaddr);
    TEST_ASSERT("the later allowed translated address still loads the expected payload after the earlier silent deny-prefetch path",
        excpt.triggered == false &&
        observed == allowed_expected
    );

    TEST_SETUP_EXCEPT();
    ld(denied_vaddr);
    TEST_ASSERT("the same denied translated address still reports load access fault on a real M-mode load after the allowed-page recovery probe",
        excpt.triggered == true &&
        excpt.cause == CAUSE_LAF &&
        excpt.tval == denied_vaddr
    );

    goto_priv(PRIV_M);
    ai_micro_prefetch_restore_pmp_full_access();
    ai_micro_prefetch_clear_machine_access_mode();

    goto_priv(PRIV_HS);
    hspt_init();
    sfence_vma();

    TEST_END();
}

bool ai_micro_m_prefetch_mprv_u_repeated_deny_then_allowed_translated_access_succeeds_corner()
{
    TEST_START();

    uintptr_t denied_vaddr = hs_page_base(AI_MICRO_PREFETCH_PAGE) + 0x2e0U;
    uintptr_t denied_paddr = phys_page_base(AI_MICRO_PREFETCH_PAGE) + 0x2e0U;
    uintptr_t allowed_vaddr = hs_page_base(SWITCH1) + 0x2e0U;
    uintptr_t allowed_paddr = phys_page_base(SWITCH1) + 0x2e0U;
    const uint64_t denied_initial = 0x3141592653589793ULL;
    const uint64_t allowed_expected = 0xfedcba9876543210ULL;

    goto_priv(PRIV_M);
    write64(denied_paddr, denied_initial);
    write64(allowed_paddr, allowed_expected);
    ai_micro_prefetch_restore_pmp_full_access();
    hspt_init();
    hspt_u_mode_allow();
    sfence_vma();

    goto_priv(PRIV_M);
    ai_micro_prefetch_configure_pmp_window(
        denied_paddr & PAGE_ADDR_MSK,
        (denied_paddr & PAGE_ADDR_MSK) + PAGE_SIZE, AI_MICRO_PREFETCH_PMP_TOR_NO_R);
    CSRC(CSR_MSTATUS, 3ULL << 11);
    CSRS(CSR_MSTATUS, MSTATUS_MPRV);

    TEST_SETUP_EXCEPT();
    prefetch_r(denied_vaddr);
    TEST_ASSERT("a first M-mode translated software prefetch stays silent when PMP denies reads to the selected translated page",
        excpt.triggered == false
    );

    TEST_SETUP_EXCEPT();
    prefetch_r(denied_vaddr);
    TEST_ASSERT("repeating the same M-mode translated software prefetch on the denied page also stays silent",
        excpt.triggered == false
    );

    TEST_SETUP_EXCEPT();
    prefetch_r(allowed_vaddr);
    TEST_ASSERT("a later M-mode translated software prefetch to a different allowed page still retires silently after the repeated denied-page probes while MPRV=1 and MPP=U remain active",
        excpt.triggered == false
    );

    TEST_SETUP_EXCEPT();
    uint64_t observed = ld(allowed_vaddr);
    TEST_ASSERT("the later allowed translated address still loads the expected payload after the repeated silent deny-prefetch probes",
        excpt.triggered == false &&
        observed == allowed_expected
    );

    TEST_SETUP_EXCEPT();
    ld(denied_vaddr);
    TEST_ASSERT("the same denied translated address still reports load access fault on a real M-mode load after the repeated denied-page probes and allowed-page recovery probe",
        excpt.triggered == true &&
        excpt.cause == CAUSE_LAF &&
        excpt.tval == denied_vaddr
    );

    goto_priv(PRIV_M);
    ai_micro_prefetch_restore_pmp_full_access();
    ai_micro_prefetch_clear_machine_access_mode();

    goto_priv(PRIV_HS);
    hspt_init();
    sfence_vma();

    TEST_END();
}

bool ai_micro_m_prefetch_mprv_s_tlb_access_fault_silent()
{
    TEST_START();

    uintptr_t vaddr = hs_page_base(AI_MICRO_PREFETCH_PAGE);

    goto_priv(PRIV_M);
    hspt_init();
    *ai_micro_prefetch_hs_leaf_entry(AI_MICRO_PREFETCH_PAGE) |= (1ULL << 46);
    sfence_vma();
    ai_micro_prefetch_enable_mprv_supervisor_translation();

    TEST_SETUP_EXCEPT();
    prefetch_r(vaddr);
    TEST_ASSERT("an M-mode prefetch targeting S-mode translation stays silent when the translated leaf overflows with access-fault semantics",
        excpt.triggered == false
    );

    TEST_SETUP_EXCEPT();
    ld(vaddr);
    TEST_ASSERT("the same MPRV=1 MPP=S translated address still reports load access fault on a real load",
        excpt.triggered == true &&
        excpt.cause == CAUSE_LAF &&
        excpt.tval == vaddr
    );

    goto_priv(PRIV_M);
    ai_micro_prefetch_clear_machine_access_mode();

    goto_priv(PRIV_HS);
    hspt_init();
    sfence_vma();

    TEST_END();
}

bool ai_micro_m_prefetch_mprv_s_tlb_access_fault_then_valid_prefetch_recovers_corner()
{
    TEST_START();

    uintptr_t good_vaddr = hs_page_base(AI_MICRO_PREFETCH_PAGE) + 0x88U;
    uintptr_t good_paddr = phys_page_base(AI_MICRO_PREFETCH_PAGE) + 0x88U;
    uintptr_t bad_vaddr = ai_micro_prefetch_prepare_overflow_alias(
        SWITCH1, AI_MICRO_PREFETCH_PAGE) + 0x88U;
    const uint64_t expected = 0x0123456789abcdefULL;

    goto_priv(PRIV_M);
    write64(good_paddr, expected);
    ai_micro_prefetch_enable_mprv_supervisor_translation();

    TEST_SETUP_EXCEPT();
    prefetch_r(bad_vaddr);
    TEST_ASSERT("an M-mode prefetch targeting S-mode translation stays silent when an earlier alias leaf overflows with access-fault semantics",
        excpt.triggered == false
    );

    TEST_SETUP_EXCEPT();
    prefetch_r(good_vaddr);
    TEST_ASSERT("a later M-mode prefetch targeting a valid S-mode translation still retires silently after the earlier access-fault-class prefetch path",
        excpt.triggered == false
    );

    TEST_SETUP_EXCEPT();
    uint64_t observed = ld(good_vaddr);
    TEST_ASSERT("the later valid translated address still loads the expected payload after the earlier MPRV=S access-fault-class silent prefetch path",
        excpt.triggered == false &&
        observed == expected
    );

    TEST_SETUP_EXCEPT();
    ld(bad_vaddr);
    TEST_ASSERT("the same overflowing alias still reports load access fault on a real M-mode load with MPRV=1 and MPP=S after the valid-address recovery probe",
        excpt.triggered == true &&
        excpt.cause == CAUSE_LAF &&
        excpt.tval == bad_vaddr
    );

    goto_priv(PRIV_M);
    ai_micro_prefetch_clear_machine_access_mode();

    goto_priv(PRIV_HS);
    hspt_init();
    sfence_vma();

    TEST_END();
}

bool ai_micro_m_prefetch_mprv_s_tlb_access_fault_then_valid_prefetch_adjacent_preserved_corner()
{
    TEST_START();

    uintptr_t good_vaddr = hs_page_base(AI_MICRO_PREFETCH_PAGE) + 0x88U;
    uintptr_t adjacent_vaddr = good_vaddr + sizeof(uint64_t);
    uintptr_t good_paddr = phys_page_base(AI_MICRO_PREFETCH_PAGE) + 0x88U;
    uintptr_t adjacent_paddr = good_paddr + sizeof(uint64_t);
    uintptr_t bad_vaddr = ai_micro_prefetch_prepare_overflow_alias(
        SWITCH1, AI_MICRO_PREFETCH_PAGE) + 0x88U;
    const uint64_t expected = 0x0123456789abcdefULL;
    const uint64_t adjacent = 0x89abcdef01234567ULL;

    goto_priv(PRIV_M);
    write64(good_paddr, expected);
    write64(adjacent_paddr, adjacent);
    ai_micro_prefetch_enable_mprv_supervisor_translation();

    TEST_SETUP_EXCEPT();
    prefetch_r(bad_vaddr);
    TEST_ASSERT("an M-mode prefetch targeting S-mode translation stays silent when an earlier alias leaf overflows with access-fault semantics in the adjacent-preservation corner",
        excpt.triggered == false
    );

    TEST_SETUP_EXCEPT();
    prefetch_r(good_vaddr);
    TEST_ASSERT("a later M-mode prefetch targeting a valid S-mode translation still retires silently after the earlier access-fault-class prefetch path in the adjacent-preservation corner",
        excpt.triggered == false
    );

    TEST_SETUP_EXCEPT();
    uint64_t observed = ld(good_vaddr);
    TEST_ASSERT("the later valid translated address still loads the expected payload after the earlier MPRV=S access-fault-class silent prefetch path in the adjacent-preservation corner",
        excpt.triggered == false &&
        observed == expected
    );

    TEST_SETUP_EXCEPT();
    uint64_t observed_adjacent = ld(adjacent_vaddr);
    TEST_ASSERT("after the valid translated recovery probe, the adjacent translated word also still returns its original payload",
        excpt.triggered == false &&
        observed_adjacent == adjacent
    );

    TEST_SETUP_EXCEPT();
    ld(bad_vaddr);
    TEST_ASSERT("the same overflowing alias still reports load access fault on a real M-mode load with MPRV=1 and MPP=S after the valid-address recovery probe in the adjacent-preservation corner",
        excpt.triggered == true &&
        excpt.cause == CAUSE_LAF &&
        excpt.tval == bad_vaddr
    );

    goto_priv(PRIV_M);
    TEST_ASSERT("the MPRV=S access-fault-class silent prefetch recovery corner leaves both the valid target and its adjacent backing word unchanged",
        read64(good_paddr) == expected &&
        read64(adjacent_paddr) == adjacent
    );
    ai_micro_prefetch_clear_machine_access_mode();

    goto_priv(PRIV_HS);
    hspt_init();
    sfence_vma();

    TEST_END();
}

bool ai_micro_m_prefetch_mprv_s_tlb_access_fault_repeated_silent_corner()
{
    TEST_START();

    uintptr_t good_vaddr = hs_page_base(AI_MICRO_PREFETCH_PAGE) + 0xc8U;
    uintptr_t good_paddr = phys_page_base(AI_MICRO_PREFETCH_PAGE) + 0xc8U;
    uintptr_t adjacent_paddr = good_paddr + sizeof(uint64_t);
    uintptr_t bad_vaddr = ai_micro_prefetch_prepare_overflow_alias(
        SWITCH1, AI_MICRO_PREFETCH_PAGE) + 0xc8U;
    const uint64_t expected = 0x0123456789abcdefULL;
    const uint64_t adjacent = 0x89abcdef01234567ULL;

    goto_priv(PRIV_M);
    write64(good_paddr, expected);
    write64(adjacent_paddr, adjacent);
    ai_micro_prefetch_enable_mprv_supervisor_translation();

    TEST_SETUP_EXCEPT();
    prefetch_r(bad_vaddr);
    TEST_ASSERT("a first M-mode prefetch targeting an S-mode translation with overflowing PPN still retires silently",
        excpt.triggered == false
    );

    TEST_SETUP_EXCEPT();
    prefetch_r(bad_vaddr);
    TEST_ASSERT("repeating the same M-mode prefetch targeting the overflowing S-mode translation also retires silently",
        excpt.triggered == false
    );

    TEST_SETUP_EXCEPT();
    uint64_t observed = ld(good_vaddr);
    TEST_ASSERT("a later valid translated address still loads its expected payload after the repeated MPRV=S access-fault-class silent prefetch probes",
        excpt.triggered == false &&
        observed == expected
    );

    TEST_SETUP_EXCEPT();
    ld(bad_vaddr);
    TEST_ASSERT("the same overflowing alias still reports load access fault on a real M-mode load with MPRV=1 and MPP=S after the repeated silent probes",
        excpt.triggered == true &&
        excpt.cause == CAUSE_LAF &&
        excpt.tval == bad_vaddr
    );

    goto_priv(PRIV_M);
    TEST_ASSERT("repeated MPRV=S access-fault-class silent prefetch probes leave the later valid target and adjacent backing-memory word unchanged",
        read64(good_paddr) == expected &&
        read64(adjacent_paddr) == adjacent
    );
    ai_micro_prefetch_clear_machine_access_mode();

    goto_priv(PRIV_HS);
    hspt_init();
    sfence_vma();

    TEST_END();
}

bool ai_micro_m_prefetch_mprv_s_tlb_access_fault_repeated_then_valid_prefetch_recovers_corner()
{
    TEST_START();

    uintptr_t good_vaddr = hs_page_base(AI_MICRO_PREFETCH_PAGE) + 0x108U;
    uintptr_t good_paddr = phys_page_base(AI_MICRO_PREFETCH_PAGE) + 0x108U;
    uintptr_t bad_vaddr = ai_micro_prefetch_prepare_overflow_alias(
        SWITCH1, AI_MICRO_PREFETCH_PAGE) + 0x108U;
    const uint64_t expected = 0x0f1e2d3c4b5a6978ULL;

    goto_priv(PRIV_M);
    write64(good_paddr, expected);
    ai_micro_prefetch_enable_mprv_supervisor_translation();

    TEST_SETUP_EXCEPT();
    prefetch_r(bad_vaddr);
    TEST_ASSERT("a first M-mode prefetch targeting an S-mode translation with overflowing PPN still retires silently",
        excpt.triggered == false
    );

    TEST_SETUP_EXCEPT();
    prefetch_r(bad_vaddr);
    TEST_ASSERT("repeating the same M-mode prefetch targeting the overflowing S-mode translation also retires silently",
        excpt.triggered == false
    );

    TEST_SETUP_EXCEPT();
    prefetch_r(good_vaddr);
    TEST_ASSERT("a later M-mode prefetch targeting a valid S-mode translation still retires silently after the repeated access-fault-class probes",
        excpt.triggered == false
    );

    TEST_SETUP_EXCEPT();
    uint64_t observed = ld(good_vaddr);
    TEST_ASSERT("the later valid translated address still loads the expected payload after the repeated MPRV=S access-fault-class silent prefetch path",
        excpt.triggered == false &&
        observed == expected
    );

    TEST_SETUP_EXCEPT();
    ld(bad_vaddr);
    TEST_ASSERT("the same overflowing alias still reports load access fault on a real M-mode load with MPRV=1 and MPP=S after the repeated probes and valid-address recovery",
        excpt.triggered == true &&
        excpt.cause == CAUSE_LAF &&
        excpt.tval == bad_vaddr
    );

    goto_priv(PRIV_M);
    ai_micro_prefetch_clear_machine_access_mode();

    goto_priv(PRIV_HS);
    hspt_init();
    sfence_vma();

    TEST_END();
}

bool ai_micro_m_prefetch_mprv_s_tlb_access_fault_repeated_then_valid_prefetch_adjacent_preserved_corner()
{
    TEST_START();

    uintptr_t good_vaddr = hs_page_base(AI_MICRO_PREFETCH_PAGE) + 0x108U;
    uintptr_t adjacent_vaddr = good_vaddr + sizeof(uint64_t);
    uintptr_t good_paddr = phys_page_base(AI_MICRO_PREFETCH_PAGE) + 0x108U;
    uintptr_t adjacent_paddr = good_paddr + sizeof(uint64_t);
    uintptr_t bad_vaddr = ai_micro_prefetch_prepare_overflow_alias(
        SWITCH1, AI_MICRO_PREFETCH_PAGE) + 0x108U;
    const uint64_t expected = 0x0f1e2d3c4b5a6978ULL;
    const uint64_t adjacent = 0xb6c5d4e3f2011324ULL;

    goto_priv(PRIV_M);
    write64(good_paddr, expected);
    write64(adjacent_paddr, adjacent);
    ai_micro_prefetch_enable_mprv_supervisor_translation();

    TEST_SETUP_EXCEPT();
    prefetch_r(bad_vaddr);
    TEST_ASSERT("a first M-mode prefetch targeting an S-mode translation with overflowing PPN still retires silently in the adjacent-preservation corner",
        excpt.triggered == false
    );

    TEST_SETUP_EXCEPT();
    prefetch_r(bad_vaddr);
    TEST_ASSERT("repeating the same M-mode prefetch targeting the overflowing S-mode translation also retires silently in the adjacent-preservation corner",
        excpt.triggered == false
    );

    TEST_SETUP_EXCEPT();
    prefetch_r(good_vaddr);
    TEST_ASSERT("a later M-mode prefetch targeting a valid S-mode translation still retires silently after the repeated access-fault-class probes in the adjacent-preservation corner",
        excpt.triggered == false
    );

    TEST_SETUP_EXCEPT();
    uint64_t observed = ld(good_vaddr);
    TEST_ASSERT("the later valid translated address still loads the expected payload after the repeated MPRV=S access-fault-class silent prefetch path in the adjacent-preservation corner",
        excpt.triggered == false &&
        observed == expected
    );

    TEST_SETUP_EXCEPT();
    uint64_t observed_adjacent = ld(adjacent_vaddr);
    TEST_ASSERT("after the repeated valid translated recovery probe, the adjacent translated word also still returns its original payload",
        excpt.triggered == false &&
        observed_adjacent == adjacent
    );

    TEST_SETUP_EXCEPT();
    ld(bad_vaddr);
    TEST_ASSERT("the same overflowing alias still reports load access fault on a real M-mode load with MPRV=1 and MPP=S after the repeated probes and valid-address recovery in the adjacent-preservation corner",
        excpt.triggered == true &&
        excpt.cause == CAUSE_LAF &&
        excpt.tval == bad_vaddr
    );

    goto_priv(PRIV_M);
    TEST_ASSERT("the repeated MPRV=S access-fault-class silent prefetch recovery corner leaves both the valid target and its adjacent backing word unchanged",
        read64(good_paddr) == expected &&
        read64(adjacent_paddr) == adjacent
    );
    ai_micro_prefetch_clear_machine_access_mode();

    goto_priv(PRIV_HS);
    hspt_init();
    sfence_vma();

    TEST_END();
}

bool ai_micro_m_prefetch_mprv_s_tlb_page_fault_silent()
{
    TEST_START();

    uintptr_t vaddr = hs_page_base(VSI_GI);

    goto_priv(PRIV_M);
    hspt_init();
    sfence_vma();
    ai_micro_prefetch_enable_mprv_supervisor_translation();

    TEST_SETUP_EXCEPT();
    prefetch_r(vaddr);
    TEST_ASSERT("an M-mode prefetch targeting S-mode translation stays silent when the translated leaf is invalid",
        excpt.triggered == false
    );

    TEST_SETUP_EXCEPT();
    ld(vaddr);
    TEST_ASSERT("the same MPRV=1 MPP=S translated invalid leaf still reports load page fault on a real load",
        excpt.triggered == true &&
        excpt.cause == CAUSE_LPF &&
        excpt.tval == vaddr
    );

    goto_priv(PRIV_M);
    ai_micro_prefetch_clear_machine_access_mode();

    goto_priv(PRIV_HS);
    hspt_init();
    sfence_vma();

    TEST_END();
}

bool ai_micro_m_prefetch_mprv_s_tlb_page_fault_repeated_silent_corner()
{
    TEST_START();

    uintptr_t bad_vaddr = hs_page_base(VSI_GI) + 0x60U;
    uintptr_t good_vaddr = hs_page_base(AI_MICRO_PREFETCH_PAGE) + 0xe8U;
    uintptr_t good_paddr = phys_page_base(AI_MICRO_PREFETCH_PAGE) + 0xe8U;
    uintptr_t adjacent_paddr = good_paddr + sizeof(uint64_t);
    const uint64_t expected = 0xfedcba9876543210ULL;
    const uint64_t adjacent = 0x76543210fedcba98ULL;

    goto_priv(PRIV_M);
    write64(good_paddr, expected);
    write64(adjacent_paddr, adjacent);
    hspt_init();
    sfence_vma();
    ai_micro_prefetch_enable_mprv_supervisor_translation();

    TEST_SETUP_EXCEPT();
    prefetch_r(bad_vaddr);
    TEST_ASSERT("a first M-mode prefetch targeting an invalid S-mode translated leaf still retires silently",
        excpt.triggered == false
    );

    TEST_SETUP_EXCEPT();
    prefetch_r(bad_vaddr);
    TEST_ASSERT("repeating the same M-mode prefetch targeting the invalid S-mode translated leaf also retires silently",
        excpt.triggered == false
    );

    TEST_SETUP_EXCEPT();
    uint64_t observed = ld(good_vaddr);
    TEST_ASSERT("a later valid translated address still loads its expected payload after the repeated MPRV=S page-fault-class silent prefetch probes",
        excpt.triggered == false &&
        observed == expected
    );

    TEST_SETUP_EXCEPT();
    ld(bad_vaddr);
    TEST_ASSERT("the same invalid translated leaf still reports load page fault on a real M-mode load with MPRV=1 and MPP=S after the repeated silent probes",
        excpt.triggered == true &&
        excpt.cause == CAUSE_LPF &&
        excpt.tval == bad_vaddr
    );

    goto_priv(PRIV_M);
    TEST_ASSERT("repeated MPRV=S page-fault-class silent prefetch probes leave the later valid target and adjacent backing-memory word unchanged",
        read64(good_paddr) == expected &&
        read64(adjacent_paddr) == adjacent
    );
    ai_micro_prefetch_clear_machine_access_mode();

    goto_priv(PRIV_HS);
    hspt_init();
    sfence_vma();

    TEST_END();
}

bool ai_micro_m_prefetch_mprv_s_tlb_page_fault_repeated_then_valid_prefetch_recovers_corner()
{
    TEST_START();

    uintptr_t bad_vaddr = hs_page_base(VSI_GI) + 0x88U;
    uintptr_t good_vaddr = hs_page_base(AI_MICRO_PREFETCH_PAGE) + 0x128U;
    uintptr_t good_paddr = phys_page_base(AI_MICRO_PREFETCH_PAGE) + 0x128U;
    const uint64_t expected = 0x13579bdf2468ace0ULL;

    goto_priv(PRIV_M);
    write64(good_paddr, expected);
    hspt_init();
    sfence_vma();
    ai_micro_prefetch_enable_mprv_supervisor_translation();

    TEST_SETUP_EXCEPT();
    prefetch_r(bad_vaddr);
    TEST_ASSERT("a first M-mode prefetch targeting an invalid S-mode translated leaf still retires silently",
        excpt.triggered == false
    );

    TEST_SETUP_EXCEPT();
    prefetch_r(bad_vaddr);
    TEST_ASSERT("repeating the same M-mode prefetch targeting the invalid S-mode translated leaf also retires silently",
        excpt.triggered == false
    );

    TEST_SETUP_EXCEPT();
    prefetch_r(good_vaddr);
    TEST_ASSERT("a later M-mode prefetch targeting a valid S-mode translation still retires silently after the repeated page-fault-class probes",
        excpt.triggered == false
    );

    TEST_SETUP_EXCEPT();
    uint64_t observed = ld(good_vaddr);
    TEST_ASSERT("the later valid translated address still loads the expected payload after the repeated MPRV=S page-fault-class silent prefetch path",
        excpt.triggered == false &&
        observed == expected
    );

    TEST_SETUP_EXCEPT();
    ld(bad_vaddr);
    TEST_ASSERT("the same invalid translated leaf still reports load page fault on a real M-mode load with MPRV=1 and MPP=S after the repeated probes and valid-address recovery",
        excpt.triggered == true &&
        excpt.cause == CAUSE_LPF &&
        excpt.tval == bad_vaddr
    );

    goto_priv(PRIV_M);
    ai_micro_prefetch_clear_machine_access_mode();

    goto_priv(PRIV_HS);
    hspt_init();
    sfence_vma();

    TEST_END();
}

bool ai_micro_m_prefetch_mprv_s_tlb_page_fault_repeated_then_valid_prefetch_adjacent_preserved_corner()
{
    TEST_START();

    uintptr_t bad_vaddr = hs_page_base(VSI_GI) + 0x88U;
    uintptr_t good_vaddr = hs_page_base(AI_MICRO_PREFETCH_PAGE) + 0x128U;
    uintptr_t adjacent_vaddr = good_vaddr + sizeof(uint64_t);
    uintptr_t good_paddr = phys_page_base(AI_MICRO_PREFETCH_PAGE) + 0x128U;
    uintptr_t adjacent_paddr = good_paddr + sizeof(uint64_t);
    const uint64_t expected = 0x13579bdf2468ace0ULL;
    const uint64_t adjacent = 0x2468ace013579bdfULL;

    goto_priv(PRIV_M);
    write64(good_paddr, expected);
    write64(adjacent_paddr, adjacent);
    hspt_init();
    sfence_vma();
    ai_micro_prefetch_enable_mprv_supervisor_translation();

    TEST_SETUP_EXCEPT();
    prefetch_r(bad_vaddr);
    TEST_ASSERT("a first M-mode prefetch targeting an invalid S-mode translated leaf still retires silently in the adjacent-preservation corner",
        excpt.triggered == false
    );

    TEST_SETUP_EXCEPT();
    prefetch_r(bad_vaddr);
    TEST_ASSERT("repeating the same M-mode prefetch targeting the invalid S-mode translated leaf also retires silently in the adjacent-preservation corner",
        excpt.triggered == false
    );

    TEST_SETUP_EXCEPT();
    prefetch_r(good_vaddr);
    TEST_ASSERT("a later M-mode prefetch targeting a valid S-mode translation still retires silently after the repeated page-fault-class probes in the adjacent-preservation corner",
        excpt.triggered == false
    );

    TEST_SETUP_EXCEPT();
    uint64_t observed = ld(good_vaddr);
    TEST_ASSERT("the later valid translated address still loads the expected payload after the repeated MPRV=S page-fault-class silent prefetch path in the adjacent-preservation corner",
        excpt.triggered == false &&
        observed == expected
    );

    TEST_SETUP_EXCEPT();
    uint64_t observed_adjacent = ld(adjacent_vaddr);
    TEST_ASSERT("after the repeated valid translated recovery probe, the adjacent translated word also still returns its original payload in the page-fault-class adjacent-preservation corner",
        excpt.triggered == false &&
        observed_adjacent == adjacent
    );

    TEST_SETUP_EXCEPT();
    ld(bad_vaddr);
    TEST_ASSERT("the same invalid translated leaf still reports load page fault on a real M-mode load with MPRV=1 and MPP=S after the repeated probes and valid-address recovery in the adjacent-preservation corner",
        excpt.triggered == true &&
        excpt.cause == CAUSE_LPF &&
        excpt.tval == bad_vaddr
    );

    goto_priv(PRIV_M);
    TEST_ASSERT("the repeated MPRV=S page-fault-class silent prefetch recovery corner leaves both the valid target and its adjacent backing word unchanged",
        read64(good_paddr) == expected &&
        read64(adjacent_paddr) == adjacent
    );
    ai_micro_prefetch_clear_machine_access_mode();

    goto_priv(PRIV_HS);
    hspt_init();
    sfence_vma();

    TEST_END();
}

bool ai_micro_m_prefetch_mprv_s_tlb_page_fault_then_valid_prefetch_recovers_corner()
{
    TEST_START();

    uintptr_t bad_vaddr = hs_page_base(VSI_GI) + 0x40U;
    uintptr_t good_vaddr = hs_page_base(AI_MICRO_PREFETCH_PAGE) + 0xa8U;
    uintptr_t good_paddr = phys_page_base(AI_MICRO_PREFETCH_PAGE) + 0xa8U;
    const uint64_t expected = 0xfedcba9876543210ULL;

    goto_priv(PRIV_M);
    write64(good_paddr, expected);
    hspt_init();
    sfence_vma();
    ai_micro_prefetch_enable_mprv_supervisor_translation();

    TEST_SETUP_EXCEPT();
    prefetch_r(bad_vaddr);
    TEST_ASSERT("an M-mode prefetch targeting S-mode translation stays silent when an earlier translated leaf is invalid",
        excpt.triggered == false
    );

    TEST_SETUP_EXCEPT();
    prefetch_r(good_vaddr);
    TEST_ASSERT("a later M-mode prefetch targeting a valid S-mode translation still retires silently after the earlier page-fault-class prefetch path",
        excpt.triggered == false
    );

    TEST_SETUP_EXCEPT();
    uint64_t observed = ld(good_vaddr);
    TEST_ASSERT("the later valid translated address still loads the expected payload after the earlier MPRV=S page-fault-class silent prefetch path",
        excpt.triggered == false &&
        observed == expected
    );

    TEST_SETUP_EXCEPT();
    ld(bad_vaddr);
    TEST_ASSERT("the same invalid translated leaf still reports load page fault on a real M-mode load with MPRV=1 and MPP=S after the valid-address recovery probe",
        excpt.triggered == true &&
        excpt.cause == CAUSE_LPF &&
        excpt.tval == bad_vaddr
    );

    goto_priv(PRIV_M);
    ai_micro_prefetch_clear_machine_access_mode();

    goto_priv(PRIV_HS);
    hspt_init();
    sfence_vma();

    TEST_END();
}

bool ai_micro_m_prefetch_mprv_s_tlb_page_fault_then_valid_prefetch_adjacent_preserved_corner()
{
    TEST_START();

    uintptr_t bad_vaddr = hs_page_base(VSI_GI) + 0x40U;
    uintptr_t good_vaddr = hs_page_base(AI_MICRO_PREFETCH_PAGE) + 0xa8U;
    uintptr_t adjacent_vaddr = good_vaddr + sizeof(uint64_t);
    uintptr_t good_paddr = phys_page_base(AI_MICRO_PREFETCH_PAGE) + 0xa8U;
    uintptr_t adjacent_paddr = good_paddr + sizeof(uint64_t);
    const uint64_t expected = 0xfedcba9876543210ULL;
    const uint64_t adjacent = 0x76543210fedcba98ULL;

    goto_priv(PRIV_M);
    write64(good_paddr, expected);
    write64(adjacent_paddr, adjacent);
    hspt_init();
    sfence_vma();
    ai_micro_prefetch_enable_mprv_supervisor_translation();

    TEST_SETUP_EXCEPT();
    prefetch_r(bad_vaddr);
    TEST_ASSERT("an M-mode prefetch targeting S-mode translation stays silent when an earlier translated leaf is invalid in the adjacent-preservation corner",
        excpt.triggered == false
    );

    TEST_SETUP_EXCEPT();
    prefetch_r(good_vaddr);
    TEST_ASSERT("a later M-mode prefetch targeting a valid S-mode translation still retires silently after the earlier page-fault-class prefetch path in the adjacent-preservation corner",
        excpt.triggered == false
    );

    TEST_SETUP_EXCEPT();
    uint64_t observed = ld(good_vaddr);
    TEST_ASSERT("the later valid translated address still loads the expected payload after the earlier MPRV=S page-fault-class silent prefetch path in the adjacent-preservation corner",
        excpt.triggered == false &&
        observed == expected
    );

    TEST_SETUP_EXCEPT();
    uint64_t observed_adjacent = ld(adjacent_vaddr);
    TEST_ASSERT("after the valid translated recovery probe, the adjacent translated word also still returns its original payload in the page-fault-class adjacent-preservation corner",
        excpt.triggered == false &&
        observed_adjacent == adjacent
    );

    TEST_SETUP_EXCEPT();
    ld(bad_vaddr);
    TEST_ASSERT("the same invalid translated leaf still reports load page fault on a real M-mode load with MPRV=1 and MPP=S after the valid-address recovery probe in the adjacent-preservation corner",
        excpt.triggered == true &&
        excpt.cause == CAUSE_LPF &&
        excpt.tval == bad_vaddr
    );

    goto_priv(PRIV_M);
    TEST_ASSERT("the MPRV=S page-fault-class silent prefetch recovery corner leaves both the valid target and its adjacent backing word unchanged",
        read64(good_paddr) == expected &&
        read64(adjacent_paddr) == adjacent
    );
    ai_micro_prefetch_clear_machine_access_mode();

    goto_priv(PRIV_HS);
    hspt_init();
    sfence_vma();

    TEST_END();
}

bool ai_micro_hs_prefetch_mshr_full_repeated_then_pmp_deny_restore_adjacent_preserved_corner()
{
    TEST_START();

    const size_t page_count =
        sizeof(ai_micro_prefetch_mshr_pages) / sizeof(ai_micro_prefetch_mshr_pages[0]);
    const enum test_page target_page = ai_micro_prefetch_mshr_pages[page_count - 1];
    uintptr_t target_vaddr = hs_page_base(target_page) + 0x400U;
    uintptr_t adjacent_vaddr = target_vaddr + sizeof(uint64_t);
    uintptr_t target_paddr = phys_page_base(target_page) + 0x400U;
    uintptr_t adjacent_paddr = target_paddr + sizeof(uint64_t);
    const uint64_t seed = 0xa100b200c300d400ULL;
    const uint64_t expected = 0x13579bdf2468ace0ULL;
    const uint64_t adjacent = 0x89abcdef01234567ULL;

    goto_priv(PRIV_M);
    CSRS(CSR_MENVCFG, MENVCFG_CBIE01);
    for (size_t i = 0; i < page_count; ++i) {
        write64(phys_page_base(ai_micro_prefetch_mshr_pages[i]), seed + (i * 0x101ULL));
    }
    write64(target_paddr, expected);
    write64(adjacent_paddr, adjacent);
    ai_micro_prefetch_restore_pmp_full_access();

    goto_priv(PRIV_HS);
    hspt_init();
    sfence_vma();

    for (size_t i = 0; i < page_count; ++i) {
        uintptr_t vaddr = hs_page_base(ai_micro_prefetch_mshr_pages[i]);

        TEST_SETUP_EXCEPT();
        uint64_t warmed = ld(vaddr);
        TEST_ASSERT("each HS probe page is readable before the repeated MSHR-pressure-plus-PMP-deny sequence starts",
            excpt.triggered == false &&
            warmed == (seed + (i * 0x101ULL))
        );

        TEST_SETUP_EXCEPT();
        cbo_inval(vaddr);
        TEST_ASSERT("each warmed probe line can be invalidated before the repeated MSHR-pressure-plus-PMP-deny sequence",
            excpt.triggered == false
        );
    }

    for (size_t i = 0; i + 1 < page_count; ++i) {
        TEST_SETUP_EXCEPT();
        prefetch_r(hs_page_base(ai_micro_prefetch_mshr_pages[i]));
        TEST_ASSERT("a pressure-building HS software prefetch still retires silently while preceding cold-line requests are already in flight during the repeated MSHR-pressure-plus-PMP-deny probe",
            excpt.triggered == false
        );
    }

    goto_priv(PRIV_M);
    ai_micro_prefetch_configure_pmp_window(
        target_paddr & PAGE_ADDR_MSK,
        (target_paddr & PAGE_ADDR_MSK) + PAGE_SIZE, AI_MICRO_PREFETCH_PMP_TOR_NO_R);

    goto_priv(PRIV_HS);
    TEST_SETUP_EXCEPT();
    prefetch_r(target_vaddr);
    TEST_ASSERT("a first software prefetch on the denied HS target still remains silent even with the miss machinery already under pressure",
        excpt.triggered == false
    );

    TEST_SETUP_EXCEPT();
    prefetch_r(target_vaddr);
    TEST_ASSERT("repeating the same denied-target software prefetch also remains silent under the same MSHR-pressure-plus-PMP-deny conditions",
        excpt.triggered == false
    );

    TEST_SETUP_EXCEPT();
    ld(target_vaddr);
    TEST_ASSERT("the same HS load still reports load access fault before PMP read permission is restored after the repeated MSHR-pressure deny probes",
        excpt.triggered == true &&
        excpt.cause == CAUSE_LAF &&
        excpt.tval == target_vaddr
    );

    goto_priv(PRIV_M);
    TEST_ASSERT("the denied HS load still leaves both the target and adjacent backing-memory words unchanged after the repeated silent PMP-deny probes under MSHR pressure",
        read64(target_paddr) == expected &&
        read64(adjacent_paddr) == adjacent
    );
    ai_micro_prefetch_restore_pmp_full_access();

    goto_priv(PRIV_HS);
    TEST_SETUP_EXCEPT();
    uint64_t restored = ld(target_vaddr);
    TEST_ASSERT("restoring PMP read permission still makes the same HS target word readable again after the repeated MSHR-pressure-plus-PMP-deny probes",
        excpt.triggered == false &&
        restored == expected
    );

    TEST_SETUP_EXCEPT();
    uint64_t restored_adjacent = ld(adjacent_vaddr);
    TEST_ASSERT("after the repeated recovered PMP-deny path, the adjacent HS word remains unchanged in the MSHR-pressure-plus-PMP-deny corner",
        excpt.triggered == false &&
        restored_adjacent == adjacent
    );

    goto_priv(PRIV_M);
    TEST_ASSERT("the repeated MSHR-pressure-plus-PMP-deny restore corner preserves both the target and adjacent backing-memory words",
        read64(target_paddr) == expected &&
        read64(adjacent_paddr) == adjacent
    );
    CSRC(CSR_MENVCFG, MENVCFG_CBIE01);

    TEST_END();
}

bool ai_micro_hs_prefetch_mshr_full_repeated_then_pmp_resp_st_restore_adjacent_preserved_corner()
{
    TEST_START();

    const size_t page_count =
        sizeof(ai_micro_prefetch_mshr_pages) / sizeof(ai_micro_prefetch_mshr_pages[0]);
    const enum test_page target_page = SWITCH2;
    uintptr_t target_vaddr = hs_page_base(target_page) + 0x500U;
    uintptr_t adjacent_vaddr = target_vaddr + sizeof(uint64_t);
    uintptr_t target_paddr = phys_page_base(target_page) + 0x500U;
    uintptr_t adjacent_paddr = target_paddr + sizeof(uint64_t);
    const uint64_t seed = 0xe100f200a300b400ULL;
    const uint64_t initial = 0x2468ace013579bdfULL;
    const uint64_t warmed = 0x1122446688aacceeULL;
    const uint64_t stored = 0x55aa33cc77ee11ffULL;
    const uint64_t adjacent = 0x76543210fedcba98ULL;

    goto_priv(PRIV_M);
    CSRS(CSR_MENVCFG, MENVCFG_CBIE01);
    for (size_t i = 0; i < page_count; ++i) {
        write64(phys_page_base(ai_micro_prefetch_mshr_pages[i]), seed + (i * 0x101ULL));
    }
    write64(target_paddr, initial);
    write64(adjacent_paddr, adjacent);
    ai_micro_prefetch_restore_pmp_full_access();

    goto_priv(PRIV_HS);
    hspt_init();
    sfence_vma();

    TEST_SETUP_EXCEPT();
    sd(target_vaddr, warmed);
    TEST_ASSERT("an HS store first warms the translated cacheable target line before the repeated MSHR-pressure-plus-resp_st probe",
        excpt.triggered == false
    );

    goto_priv(PRIV_M);
    TEST_ASSERT("the warm-up HS store updates the backing memory before write permission is removed for the repeated MSHR-pressure-plus-resp_st corner",
        read64(target_paddr) == warmed
    );
    write64(target_paddr, initial);

    goto_priv(PRIV_HS);
    for (size_t i = 0; i + 1 < page_count; ++i) {
        uintptr_t vaddr = hs_page_base(ai_micro_prefetch_mshr_pages[i]);

        TEST_SETUP_EXCEPT();
        uint64_t probe = ld(vaddr);
        TEST_ASSERT("each HS pressure-building probe page is readable before the repeated MSHR-pressure-plus-resp_st sequence starts",
            excpt.triggered == false &&
            probe == (seed + (i * 0x101ULL))
        );

        TEST_SETUP_EXCEPT();
        cbo_inval(vaddr);
        TEST_ASSERT("each warmed pressure-building probe line can be invalidated before the repeated MSHR-pressure-plus-resp_st sequence",
            excpt.triggered == false
        );
    }

    for (size_t i = 0; i + 1 < page_count; ++i) {
        TEST_SETUP_EXCEPT();
        prefetch_r(hs_page_base(ai_micro_prefetch_mshr_pages[i]));
        TEST_ASSERT("a pressure-building HS software prefetch still retires silently while preceding cold-line requests are already in flight during the repeated MSHR-pressure-plus-resp_st probe",
            excpt.triggered == false
        );
    }

    goto_priv(PRIV_M);
    ai_micro_prefetch_configure_pmp_window(
        target_paddr & PAGE_ADDR_MSK,
        (target_paddr & PAGE_ADDR_MSK) + PAGE_SIZE, AI_MICRO_PREFETCH_PMP_TOR_NO_W);

    goto_priv(PRIV_HS);
    TEST_SETUP_EXCEPT();
    prefetch_w(target_vaddr);
    TEST_ASSERT("a first software prefetch on the hot HS target still remains silent even with the miss machinery already under pressure before the resp_st path",
        excpt.triggered == false
    );

    TEST_SETUP_EXCEPT();
    prefetch_w(target_vaddr);
    TEST_ASSERT("repeating the same hot-target software prefetch also remains silent under the same MSHR-pressure-plus-resp_st conditions",
        excpt.triggered == false
    );

    TEST_SETUP_EXCEPT();
    sd(target_vaddr, 0xf0e1d2c3b4a59687ULL);
    TEST_ASSERT("the same hot HS address still reports store access fault before PMP write permission is restored after the repeated MSHR-pressure resp_st probes",
        excpt.triggered == true &&
        excpt.cause == CAUSE_SAF &&
        excpt.tval == target_vaddr
    );

    goto_priv(PRIV_M);
    TEST_ASSERT("the denied hot-HS store still leaves both the target and adjacent backing-memory words unchanged after the repeated resp_st silent probes under MSHR pressure",
        read64(target_paddr) == initial &&
        read64(adjacent_paddr) == adjacent
    );
    ai_micro_prefetch_restore_pmp_full_access();

    goto_priv(PRIV_HS);
    TEST_SETUP_EXCEPT();
    sd(target_vaddr, stored);
    TEST_ASSERT("restoring PMP write permission still lets the same HS target word store succeed again after the repeated MSHR-pressure-plus-resp_st probes",
        excpt.triggered == false
    );

    TEST_SETUP_EXCEPT();
    uint64_t restored = ld(target_vaddr);
    TEST_ASSERT("after the repeated recovered store path, the same HS target word reads back the newly written payload in the MSHR-pressure-plus-resp_st corner",
        excpt.triggered == false &&
        restored == stored
    );

    TEST_SETUP_EXCEPT();
    uint64_t restored_adjacent = ld(adjacent_vaddr);
    TEST_ASSERT("after the repeated recovered store path, the adjacent HS word remains unchanged in the MSHR-pressure-plus-resp_st corner",
        excpt.triggered == false &&
        restored_adjacent == adjacent
    );

    goto_priv(PRIV_M);
    TEST_ASSERT("the repeated MSHR-pressure-plus-resp_st restore corner updates only the target backing word while preserving the adjacent word",
        read64(target_paddr) == stored &&
        read64(adjacent_paddr) == adjacent
    );
    CSRC(CSR_MENVCFG, MENVCFG_CBIE01);

    TEST_END();
}

bool ai_micro_hs_prefetch_same_paddr_alias_mshr_pressure_repeated_then_pmp_deny_restore_adjacent_preserved_corner()
{
    TEST_START();

    const size_t page_count =
        sizeof(ai_micro_prefetch_mshr_pages) / sizeof(ai_micro_prefetch_mshr_pages[0]);
    const enum test_page base_page = AI_MICRO_PREFETCH_PAGE;
    const enum test_page alias_page = SWITCH1;
    const uint64_t seed = 0xb100c200d300e400ULL;
    uintptr_t base_vaddr = hs_page_base(base_page) + 0x540U;
    uintptr_t alias_vaddr = ai_micro_prefetch_prepare_same_paddr_alias(
        alias_page, base_page) + 0x540U;
    uintptr_t adjacent_base_vaddr = base_vaddr + sizeof(uint64_t);
    uintptr_t adjacent_alias_vaddr = alias_vaddr + sizeof(uint64_t);
    uintptr_t paddr = phys_page_base(base_page) + 0x540U;
    uintptr_t adjacent_paddr = paddr + sizeof(uint64_t);
    const uint64_t expected = 0x2468ace013579bdfULL;
    const uint64_t adjacent = 0x76543210fedcba98ULL;

    goto_priv(PRIV_M);
    CSRS(CSR_MENVCFG, MENVCFG_CBIE01);
    for (size_t i = 0; i < page_count; ++i) {
        write64(phys_page_base(ai_micro_prefetch_mshr_pages[i]), seed + (i * 0x101ULL));
    }
    write64(paddr, expected);
    write64(adjacent_paddr, adjacent);
    ai_micro_prefetch_restore_pmp_full_access();

    goto_priv(PRIV_HS);
    TEST_SETUP_EXCEPT();
    uint64_t warmed = ld(base_vaddr);
    TEST_ASSERT("an HS load first warms the target alias line before the repeated alias+MSHR-pressure-plus-PMP-deny probe",
        excpt.triggered == false &&
        warmed == expected
    );

    TEST_SETUP_EXCEPT();
    cbo_inval(base_vaddr);
    TEST_ASSERT("invalidating the warmed target alias line succeeds before the repeated alias+MSHR-pressure-plus-PMP-deny probe",
        excpt.triggered == false
    );

    for (size_t i = 0; i < page_count; ++i) {
        uintptr_t vaddr = hs_page_base(ai_micro_prefetch_mshr_pages[i]);

        TEST_SETUP_EXCEPT();
        uint64_t probe = ld(vaddr);
        TEST_ASSERT("each HS pressure-building probe page is readable before the repeated alias+MSHR-pressure-plus-PMP-deny sequence starts",
            excpt.triggered == false &&
            probe == (seed + (i * 0x101ULL))
        );

        TEST_SETUP_EXCEPT();
        cbo_inval(vaddr);
        TEST_ASSERT("each warmed pressure-building probe line can be invalidated before the repeated alias+MSHR-pressure-plus-PMP-deny sequence",
            excpt.triggered == false
        );
    }

    for (size_t i = 0; i + 1 < page_count; ++i) {
        TEST_SETUP_EXCEPT();
        prefetch_r(hs_page_base(ai_micro_prefetch_mshr_pages[i]));
        TEST_ASSERT("a pressure-building HS software prefetch still retires silently while preceding cold-line requests are already in flight during the repeated alias+MSHR-pressure-plus-PMP-deny probe",
            excpt.triggered == false
        );
    }

    goto_priv(PRIV_M);
    ai_micro_prefetch_configure_pmp_window(
        paddr & PAGE_ADDR_MSK,
        (paddr & PAGE_ADDR_MSK) + PAGE_SIZE, AI_MICRO_PREFETCH_PMP_TOR_NO_R);

    goto_priv(PRIV_HS);
    TEST_SETUP_EXCEPT();
    prefetch_r(alias_vaddr);
    TEST_ASSERT("a first software prefetch through the switched alias remains silent after read permission is denied on the shared physical page even with miss-machinery pressure already building",
        excpt.triggered == false
    );

    TEST_SETUP_EXCEPT();
    prefetch_r(alias_vaddr);
    TEST_ASSERT("repeating the same switched-alias software prefetch also remains silent under the repeated alias+MSHR-pressure-plus-PMP-deny conditions",
        excpt.triggered == false
    );

    TEST_SETUP_EXCEPT();
    ld(alias_vaddr);
    TEST_ASSERT("the switched HS alias still reports load access fault on a real read before PMP read permission is restored after the repeated alias+MSHR-pressure probe",
        excpt.triggered == true &&
        excpt.cause == CAUSE_LAF &&
        excpt.tval == alias_vaddr
    );

    goto_priv(PRIV_M);
    TEST_ASSERT("the repeated alias+MSHR-pressure-plus-PMP-deny probe leaves both the target and adjacent backing-memory words unchanged before permission is restored",
        read64(paddr) == expected &&
        read64(adjacent_paddr) == adjacent
    );
    ai_micro_prefetch_restore_pmp_full_access();

    goto_priv(PRIV_HS);
    TEST_SETUP_EXCEPT();
    uint64_t observed_base = ld(base_vaddr);
    TEST_ASSERT("restoring PMP read permission lets the base HS alias load the shared target payload again after the repeated alias+MSHR-pressure-plus-PMP-deny probe",
        excpt.triggered == false &&
        observed_base == expected
    );

    TEST_SETUP_EXCEPT();
    uint64_t observed_alias = ld(alias_vaddr);
    TEST_ASSERT("restoring PMP read permission also lets the switched HS alias load the same shared target payload again after the repeated alias+MSHR-pressure-plus-PMP-deny probe",
        excpt.triggered == false &&
        observed_alias == expected
    );

    TEST_SETUP_EXCEPT();
    uint64_t observed_adjacent_base = ld(adjacent_base_vaddr);
    TEST_ASSERT("the adjacent word through the base alias remains unchanged after the repeated alias+MSHR-pressure-plus-PMP-deny restore path",
        excpt.triggered == false &&
        observed_adjacent_base == adjacent
    );

    TEST_SETUP_EXCEPT();
    uint64_t observed_adjacent_alias = ld(adjacent_alias_vaddr);
    TEST_ASSERT("the adjacent word through the switched alias also remains unchanged after the repeated alias+MSHR-pressure-plus-PMP-deny restore path",
        excpt.triggered == false &&
        observed_adjacent_alias == adjacent
    );

    goto_priv(PRIV_M);
    TEST_ASSERT("the repeated alias+MSHR-pressure-plus-PMP-deny restore corner preserves both the target and adjacent backing-memory words",
        read64(paddr) == expected &&
        read64(adjacent_paddr) == adjacent
    );
    CSRC(CSR_MENVCFG, MENVCFG_CBIE01);

    goto_priv(PRIV_HS);
    hspt_init();
    sfence_vma();

    TEST_END();
}

bool ai_micro_hs_prefetch_same_paddr_alias_mshr_pressure_repeated_then_pmp_resp_ld_restore_adjacent_preserved_corner()
{
    TEST_START();

    const size_t page_count =
        sizeof(ai_micro_prefetch_mshr_pages) / sizeof(ai_micro_prefetch_mshr_pages[0]);
    const enum test_page base_page = AI_MICRO_PREFETCH_PAGE;
    const enum test_page alias_page = SWITCH1;
    const uint64_t seed = 0xc100d200e300f400ULL;
    uintptr_t base_vaddr = hs_page_base(base_page) + 0x5c0U;
    uintptr_t alias_vaddr = ai_micro_prefetch_prepare_same_paddr_alias(
        alias_page, base_page) + 0x5c0U;
    uintptr_t adjacent_base_vaddr = base_vaddr + sizeof(uint64_t);
    uintptr_t adjacent_alias_vaddr = alias_vaddr + sizeof(uint64_t);
    uintptr_t paddr = phys_page_base(base_page) + 0x5c0U;
    uintptr_t adjacent_paddr = paddr + sizeof(uint64_t);
    const uint64_t expected = 0x13579bdf2468ace0ULL;
    const uint64_t adjacent = 0x89abcdef01234567ULL;

    goto_priv(PRIV_M);
    CSRS(CSR_MENVCFG, MENVCFG_CBIE01);
    for (size_t i = 0; i < page_count; ++i) {
        write64(phys_page_base(ai_micro_prefetch_mshr_pages[i]), seed + (i * 0x101ULL));
    }
    write64(paddr, expected);
    write64(adjacent_paddr, adjacent);
    ai_micro_prefetch_restore_pmp_full_access();

    goto_priv(PRIV_HS);
    TEST_SETUP_EXCEPT();
    uint64_t warmed_base = ld(base_vaddr);
    TEST_ASSERT("an HS load through the base alias first warms the translated cacheable line before the repeated alias+MSHR-pressure-plus-resp_ld probe",
        excpt.triggered == false &&
        warmed_base == expected
    );

    TEST_SETUP_EXCEPT();
    uint64_t warmed_alias = ld(alias_vaddr);
    TEST_ASSERT("an HS load through the switched alias also warms the same translated physical line before the repeated alias+MSHR-pressure-plus-resp_ld probe",
        excpt.triggered == false &&
        warmed_alias == expected
    );

    for (size_t i = 0; i + 1 < page_count; ++i) {
        uintptr_t vaddr = hs_page_base(ai_micro_prefetch_mshr_pages[i]);

        TEST_SETUP_EXCEPT();
        uint64_t probe = ld(vaddr);
        TEST_ASSERT("each HS pressure-building probe page is readable before the repeated alias+MSHR-pressure-plus-resp_ld sequence starts",
            excpt.triggered == false &&
            probe == (seed + (i * 0x101ULL))
        );

        TEST_SETUP_EXCEPT();
        cbo_inval(vaddr);
        TEST_ASSERT("each warmed pressure-building probe line can be invalidated before the repeated alias+MSHR-pressure-plus-resp_ld sequence",
            excpt.triggered == false
        );
    }

    for (size_t i = 0; i + 1 < page_count; ++i) {
        TEST_SETUP_EXCEPT();
        prefetch_r(hs_page_base(ai_micro_prefetch_mshr_pages[i]));
        TEST_ASSERT("a pressure-building HS software prefetch still retires silently while preceding cold-line requests are already in flight during the repeated alias+MSHR-pressure-plus-resp_ld probe",
            excpt.triggered == false
        );
    }

    goto_priv(PRIV_M);
    ai_micro_prefetch_configure_pmp_window(
        paddr & PAGE_ADDR_MSK,
        (paddr & PAGE_ADDR_MSK) + PAGE_SIZE, AI_MICRO_PREFETCH_PMP_TOR_NO_R);

    goto_priv(PRIV_HS);
    TEST_SETUP_EXCEPT();
    prefetch_r(alias_vaddr);
    TEST_ASSERT("a first software prefetch through the switched alias remains silent when the hot shared line later reaches the PMP resp_ld path under miss-machinery pressure",
        excpt.triggered == false
    );

    TEST_SETUP_EXCEPT();
    prefetch_r(alias_vaddr);
    TEST_ASSERT("repeating the same switched-alias software prefetch also remains silent under the repeated alias+MSHR-pressure-plus-resp_ld conditions",
        excpt.triggered == false
    );

    TEST_SETUP_EXCEPT();
    ld(alias_vaddr);
    TEST_ASSERT("the switched HS alias still reports load access fault on a real read after the repeated silent alias+MSHR-pressure-plus-resp_ld prefetch path",
        excpt.triggered == true &&
        excpt.cause == CAUSE_LAF &&
        excpt.tval == alias_vaddr
    );

    goto_priv(PRIV_M);
    TEST_ASSERT("the repeated alias+MSHR-pressure-plus-resp_ld silent prefetch probe leaves both the target and adjacent backing-memory words unchanged before permission is restored",
        read64(paddr) == expected &&
        read64(adjacent_paddr) == adjacent
    );
    ai_micro_prefetch_restore_pmp_full_access();

    goto_priv(PRIV_HS);
    TEST_SETUP_EXCEPT();
    uint64_t restored_base = ld(base_vaddr);
    TEST_ASSERT("restoring PMP read permission lets the base HS alias read the shared target payload again after the repeated alias+MSHR-pressure-plus-resp_ld probe",
        excpt.triggered == false &&
        restored_base == expected
    );

    TEST_SETUP_EXCEPT();
    uint64_t restored_alias = ld(alias_vaddr);
    TEST_ASSERT("restoring PMP read permission also lets the switched HS alias read the same shared target payload again after the repeated alias+MSHR-pressure-plus-resp_ld probe",
        excpt.triggered == false &&
        restored_alias == expected
    );

    TEST_SETUP_EXCEPT();
    uint64_t restored_adjacent_base = ld(adjacent_base_vaddr);
    TEST_ASSERT("the adjacent word through the base alias remains unchanged after the repeated alias+MSHR-pressure-plus-resp_ld restore path",
        excpt.triggered == false &&
        restored_adjacent_base == adjacent
    );

    TEST_SETUP_EXCEPT();
    uint64_t restored_adjacent_alias = ld(adjacent_alias_vaddr);
    TEST_ASSERT("the adjacent word through the switched alias also remains unchanged after the repeated alias+MSHR-pressure-plus-resp_ld restore path",
        excpt.triggered == false &&
        restored_adjacent_alias == adjacent
    );

    goto_priv(PRIV_M);
    TEST_ASSERT("the repeated alias+MSHR-pressure-plus-resp_ld restore corner preserves both the target and adjacent backing-memory words",
        read64(paddr) == expected &&
        read64(adjacent_paddr) == adjacent
    );
    CSRC(CSR_MENVCFG, MENVCFG_CBIE01);

    goto_priv(PRIV_HS);
    hspt_init();
    sfence_vma();

    TEST_END();
}

bool ai_micro_hs_prefetch_same_paddr_alias_mshr_pressure_repeated_then_pmp_resp_st_restore_adjacent_preserved_corner()
{
    TEST_START();

    const size_t page_count =
        sizeof(ai_micro_prefetch_mshr_pages) / sizeof(ai_micro_prefetch_mshr_pages[0]);
    const enum test_page base_page = AI_MICRO_PREFETCH_PAGE;
    const enum test_page alias_page = SWITCH2;
    const uint64_t seed = 0xd100e200f300a400ULL;
    uintptr_t base_vaddr = hs_page_base(base_page) + 0x580U;
    uintptr_t alias_vaddr = ai_micro_prefetch_prepare_same_paddr_alias(
        alias_page, base_page) + 0x580U;
    uintptr_t adjacent_base_vaddr = base_vaddr + sizeof(uint64_t);
    uintptr_t adjacent_alias_vaddr = alias_vaddr + sizeof(uint64_t);
    uintptr_t paddr = phys_page_base(base_page) + 0x580U;
    uintptr_t adjacent_paddr = paddr + sizeof(uint64_t);
    const uint64_t initial = 0x1122446688aacceeULL;
    const uint64_t warmed = 0x99bbddff11335577ULL;
    const uint64_t stored = 0xf0e1d2c3b4a59687ULL;
    const uint64_t adjacent = 0x76543210fedcba98ULL;

    goto_priv(PRIV_M);
    CSRS(CSR_MENVCFG, MENVCFG_CBIE01);
    for (size_t i = 0; i < page_count; ++i) {
        write64(phys_page_base(ai_micro_prefetch_mshr_pages[i]), seed + (i * 0x101ULL));
    }
    write64(paddr, initial);
    write64(adjacent_paddr, adjacent);
    ai_micro_prefetch_restore_pmp_full_access();

    goto_priv(PRIV_HS);
    TEST_SETUP_EXCEPT();
    sd(base_vaddr, warmed);
    TEST_ASSERT("an HS store through the base alias first warms the translated cacheable line before the repeated alias+MSHR-pressure-plus-resp_st probe",
        excpt.triggered == false
    );

    goto_priv(PRIV_M);
    TEST_ASSERT("the warm-up HS store through the base alias updates the shared backing memory before write permission is removed in the repeated alias+MSHR-pressure-plus-resp_st corner",
        read64(paddr) == warmed
    );
    write64(paddr, initial);
    write64(adjacent_paddr, adjacent);

    goto_priv(PRIV_HS);
    TEST_SETUP_EXCEPT();
    uint64_t warmed_alias = ld(alias_vaddr);
    TEST_ASSERT("an HS load through the switched alias also warms the same translated physical line before the repeated alias+MSHR-pressure-plus-resp_st probe",
        excpt.triggered == false &&
        warmed_alias == initial
    );

    for (size_t i = 0; i + 1 < page_count; ++i) {
        uintptr_t vaddr = hs_page_base(ai_micro_prefetch_mshr_pages[i]);

        TEST_SETUP_EXCEPT();
        uint64_t probe = ld(vaddr);
        TEST_ASSERT("each HS pressure-building probe page is readable before the repeated alias+MSHR-pressure-plus-resp_st sequence starts",
            excpt.triggered == false &&
            probe == (seed + (i * 0x101ULL))
        );

        TEST_SETUP_EXCEPT();
        cbo_inval(vaddr);
        TEST_ASSERT("each warmed pressure-building probe line can be invalidated before the repeated alias+MSHR-pressure-plus-resp_st sequence",
            excpt.triggered == false
        );
    }

    for (size_t i = 0; i + 1 < page_count; ++i) {
        TEST_SETUP_EXCEPT();
        prefetch_r(hs_page_base(ai_micro_prefetch_mshr_pages[i]));
        TEST_ASSERT("a pressure-building HS software prefetch still retires silently while preceding cold-line requests are already in flight during the repeated alias+MSHR-pressure-plus-resp_st probe",
            excpt.triggered == false
        );
    }

    goto_priv(PRIV_M);
    ai_micro_prefetch_configure_pmp_window(
        paddr & PAGE_ADDR_MSK,
        (paddr & PAGE_ADDR_MSK) + PAGE_SIZE, AI_MICRO_PREFETCH_PMP_TOR_NO_W);

    goto_priv(PRIV_HS);
    TEST_SETUP_EXCEPT();
    prefetch_w(alias_vaddr);
    TEST_ASSERT("a first software prefetch through the switched alias remains silent when the hot shared line later reaches the PMP resp_st path under miss-machinery pressure",
        excpt.triggered == false
    );

    TEST_SETUP_EXCEPT();
    prefetch_w(alias_vaddr);
    TEST_ASSERT("repeating the same switched-alias software prefetch also remains silent under the repeated alias+MSHR-pressure-plus-resp_st conditions",
        excpt.triggered == false
    );

    TEST_SETUP_EXCEPT();
    sd(alias_vaddr, stored);
    TEST_ASSERT("the switched HS alias still reports store access fault on a real write after the repeated silent alias+MSHR-pressure-plus-resp_st prefetch path",
        excpt.triggered == true &&
        excpt.cause == CAUSE_SAF &&
        excpt.tval == alias_vaddr
    );

    goto_priv(PRIV_M);
    TEST_ASSERT("the repeated alias+MSHR-pressure-plus-resp_st silent prefetch probe leaves both the target and adjacent backing-memory words unchanged before permission is restored",
        read64(paddr) == initial &&
        read64(adjacent_paddr) == adjacent
    );
    ai_micro_prefetch_restore_pmp_full_access();

    goto_priv(PRIV_HS);
    TEST_SETUP_EXCEPT();
    sd(alias_vaddr, stored);
    TEST_ASSERT("restoring PMP write permission lets the switched HS alias commit the shared target payload again after the repeated alias+MSHR-pressure-plus-resp_st probe",
        excpt.triggered == false
    );

    TEST_SETUP_EXCEPT();
    uint64_t restored_base = ld(base_vaddr);
    TEST_ASSERT("after the recovered repeated alias+MSHR-pressure-plus-resp_st path, the base HS alias reads back the newly written shared target payload",
        excpt.triggered == false &&
        restored_base == stored
    );

    TEST_SETUP_EXCEPT();
    uint64_t restored_alias = ld(alias_vaddr);
    TEST_ASSERT("after the recovered repeated alias+MSHR-pressure-plus-resp_st path, the switched HS alias also reads back the newly written shared target payload",
        excpt.triggered == false &&
        restored_alias == stored
    );

    TEST_SETUP_EXCEPT();
    uint64_t restored_adjacent_base = ld(adjacent_base_vaddr);
    TEST_ASSERT("the adjacent word through the base alias remains unchanged after the repeated alias+MSHR-pressure-plus-resp_st restore path",
        excpt.triggered == false &&
        restored_adjacent_base == adjacent
    );

    TEST_SETUP_EXCEPT();
    uint64_t restored_adjacent_alias = ld(adjacent_alias_vaddr);
    TEST_ASSERT("the adjacent word through the switched alias also remains unchanged after the repeated alias+MSHR-pressure-plus-resp_st restore path",
        excpt.triggered == false &&
        restored_adjacent_alias == adjacent
    );

    goto_priv(PRIV_M);
    TEST_ASSERT("the repeated alias+MSHR-pressure-plus-resp_st restore corner updates only the shared target backing word while preserving the adjacent word",
        read64(paddr) == stored &&
        read64(adjacent_paddr) == adjacent
    );
    CSRC(CSR_MENVCFG, MENVCFG_CBIE01);

    goto_priv(PRIV_HS);
    hspt_init();
    sfence_vma();

    TEST_END();
}
