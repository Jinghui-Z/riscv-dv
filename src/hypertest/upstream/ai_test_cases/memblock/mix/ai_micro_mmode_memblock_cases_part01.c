#include "ai_micro_mmode_memblock_common.h"

bool ai_micro_m_load_cross_page_page_fault_tval_refreshes_from_low_half_to_high_half_corner()
{
    TEST_START();

    const enum test_page low_page = SWITCH1;
    const enum test_page high_page = SWITCH2;
    const uintptr_t vaddr = ai_micro_m_memblock_cross_page_addr(low_page, 4U);
    const uintptr_t second_half_vaddr = vaddr + 4U;
    const uintptr_t low_phys = ai_micro_m_memblock_page_end_phys(low_page);
    const uintptr_t high_phys = phys_page_base(high_page);
    const uint64_t low_pattern = 0x0f1e2d3c4b5a6978ULL;
    const uint64_t high_pattern = 0x8877665544332211ULL;

    goto_priv(PRIV_M);
    ai_micro_m_memblock_reset_env();
    write64(low_phys, low_pattern);
    write64(high_phys, high_pattern);

    ai_micro_m_memblock_prepare_supervisor_translation();
    ai_micro_m_memblock_set_leaf(low_page, PTE_V | PTE_A | PTE_X);
    ai_micro_m_memblock_set_leaf(high_page, PTE_V | PTE_AD | PTE_RWX);
    sfence_vma();

    ai_micro_m_memblock_enable_mprv_supervisor_translation();
    TEST_SETUP_EXCEPT();
    ld(vaddr);
    TEST_ASSERT("under no-H M-mode+MPRV=S translation, the first split-load probe reports a low-half page fault with tval bound to the original misaligned cross-page address",
        excpt.triggered == true &&
        excpt.cause == CAUSE_LPF &&
        excpt.tval == vaddr
    );

    goto_priv(PRIV_M);
    TEST_ASSERT("the first low-half page-faulting split load leaves both participating boundary doublewords unchanged before the same-window high-half refresh probe",
        read64(low_phys) == low_pattern &&
        read64(high_phys) == high_pattern
    );

    ai_micro_m_memblock_set_leaf(low_page, PTE_V | PTE_A | PTE_R);
    ai_micro_m_memblock_set_leaf(high_page, PTE_V | PTE_A | PTE_X);
    sfence_vma();

    ai_micro_m_memblock_enable_mprv_supervisor_translation();
    TEST_SETUP_EXCEPT();
    ld(vaddr);
    TEST_ASSERT("after the earlier low-half page fault has already exercised the shared exception path, the follow-up high-half page fault must refresh tval to the second-half start address",
        excpt.triggered == true &&
        excpt.cause == CAUSE_LPF &&
        excpt.tval == second_half_vaddr
    );

    goto_priv(PRIV_M);
    TEST_ASSERT("the refreshed high-half page-fault probe still leaves both boundary doublewords unchanged because faulting split loads must not create visible memory side effects",
        read64(low_phys) == low_pattern &&
        read64(high_phys) == high_pattern
    );

    ai_micro_m_memblock_reset_env();

    TEST_END();
}

bool ai_micro_m_load_cross_page_page_fault_tval_refreshes_back_to_low_half_after_high_half_corner()
{
    TEST_START();

    const enum test_page low_page = SWITCH1;
    const enum test_page high_page = SWITCH2;
    const uintptr_t vaddr = ai_micro_m_memblock_cross_page_addr(low_page, 4U);
    const uintptr_t second_half_vaddr = vaddr + 4U;
    const uintptr_t low_phys = ai_micro_m_memblock_page_end_phys(low_page);
    const uintptr_t high_phys = phys_page_base(high_page);
    const uint64_t low_pattern = 0x13579bdf2468ace0ULL;
    const uint64_t high_pattern = 0x0f1e2d3c4b5a6978ULL;

    goto_priv(PRIV_M);
    ai_micro_m_memblock_reset_env();
    write64(low_phys, low_pattern);
    write64(high_phys, high_pattern);

    ai_micro_m_memblock_prepare_supervisor_translation();
    ai_micro_m_memblock_set_leaf(low_page, PTE_V | PTE_A | PTE_X);
    ai_micro_m_memblock_set_leaf(high_page, PTE_V | PTE_AD | PTE_RWX);
    sfence_vma();

    ai_micro_m_memblock_enable_mprv_supervisor_translation();
    TEST_SETUP_EXCEPT();
    ld(vaddr);
    TEST_ASSERT("in the no-H low->high->low split-load refresh probe, the first round must report a low-half page fault with tval bound to the original misaligned cross-page address",
        excpt.triggered == true &&
        excpt.cause == CAUSE_LPF &&
        excpt.tval == vaddr
    );

    goto_priv(PRIV_M);
    TEST_ASSERT("the first round of the no-H low->high->low split-load refresh probe leaves both participating boundary doublewords unchanged",
        read64(low_phys) == low_pattern &&
        read64(high_phys) == high_pattern
    );

    ai_micro_m_memblock_set_leaf(low_page, PTE_V | PTE_A | PTE_R);
    ai_micro_m_memblock_set_leaf(high_page, PTE_V | PTE_A | PTE_X);
    sfence_vma();

    ai_micro_m_memblock_enable_mprv_supervisor_translation();
    TEST_SETUP_EXCEPT();
    ld(vaddr);
    TEST_ASSERT("after switching to the high-half faulting mapping in the same no-H split-load refresh probe, tval must move to the second-half start address",
        excpt.triggered == true &&
        excpt.cause == CAUSE_LPF &&
        excpt.tval == second_half_vaddr
    );

    goto_priv(PRIV_M);
    TEST_ASSERT("the high-half round of the no-H low->high->low split-load refresh probe still keeps both boundary doublewords unchanged",
        read64(low_phys) == low_pattern &&
        read64(high_phys) == high_pattern
    );

    ai_micro_m_memblock_set_leaf(low_page, PTE_V | PTE_A | PTE_X);
    ai_micro_m_memblock_set_leaf(high_page, PTE_V | PTE_AD | PTE_RWX);
    sfence_vma();

    ai_micro_m_memblock_enable_mprv_supervisor_translation();
    TEST_SETUP_EXCEPT();
    ld(vaddr);
    TEST_ASSERT("after returning to the original low-half faulting mapping in that no-H split-load refresh probe, the newest fault metadata must refresh back to the original low-half cross-page address",
        excpt.triggered == true &&
        excpt.cause == CAUSE_LPF &&
        excpt.tval == vaddr
    );

    goto_priv(PRIV_M);
    TEST_ASSERT("across the full no-H low->high->low split-load refresh round-trip, faulting probes do not create visible writes on either boundary doubleword",
        read64(low_phys) == low_pattern &&
        read64(high_phys) == high_pattern
    );

    ai_micro_m_memblock_reset_env();

    TEST_END();
}

bool ai_micro_m_store_cross_page_page_fault_tval_refreshes_from_low_half_to_high_half_corner()
{
    TEST_START();

    const enum test_page low_page = SWITCH1;
    const enum test_page high_page = SWITCH2;
    const uintptr_t vaddr = ai_micro_m_memblock_cross_page_addr(low_page, 4U);
    const uintptr_t second_half_vaddr = vaddr + 4U;
    const uintptr_t low_phys = ai_micro_m_memblock_page_end_phys(low_page);
    const uintptr_t high_phys = phys_page_base(high_page);
    const uint64_t low_pattern = 0x1122334455667788ULL;
    const uint64_t high_pattern = 0x99aabbccddeeff00ULL;
    const uint64_t first_store_value = 0x0123456789abcdefULL;
    const uint64_t second_store_value = 0xfedcba9876543210ULL;
    const uint64_t expected_low_after_second_fault = low_pattern;

    goto_priv(PRIV_M);
    ai_micro_m_memblock_reset_env();
    write64(low_phys, low_pattern);
    write64(high_phys, high_pattern);

    ai_micro_m_memblock_prepare_supervisor_translation();
    ai_micro_m_memblock_set_leaf(low_page, PTE_V | PTE_A | PTE_R);
    ai_micro_m_memblock_set_leaf(high_page, PTE_V | PTE_AD | PTE_RWX);
    sfence_vma();

    ai_micro_m_memblock_enable_mprv_supervisor_translation();
    TEST_SETUP_EXCEPT();
    sd(vaddr, first_store_value);
    TEST_ASSERT("under no-H M-mode+MPRV=S translation, the first split-store probe reports a low-half page fault with tval bound to the original misaligned cross-page address",
        excpt.triggered == true &&
        excpt.cause == CAUSE_SPF &&
        excpt.tval == vaddr
    );

    goto_priv(PRIV_M);
    TEST_ASSERT("the first low-half page-faulting split store leaves both participating pages unchanged before the same-window high-half refresh probe",
        read64(low_phys) == low_pattern &&
        read64(high_phys) == high_pattern
    );

    ai_micro_m_memblock_set_leaf(low_page, PTE_V | PTE_AD | PTE_RWX);
    ai_micro_m_memblock_set_leaf(high_page, PTE_V | PTE_AD | PTE_RX);
    sfence_vma();

    ai_micro_m_memblock_enable_mprv_supervisor_translation();
    TEST_SETUP_EXCEPT();
    sd(vaddr, second_store_value);
    TEST_ASSERT("after the earlier low-half page fault has already exercised the shared exception path, the follow-up high-half page fault must refresh tval to the second-half start address",
        excpt.triggered == true &&
        excpt.cause == CAUSE_SPF &&
        excpt.tval == second_half_vaddr
    );

    goto_priv(PRIV_M);
    TEST_ASSERT("the refreshed high-half page-fault probe still exposes no low-half side effect from the newer store value while the upper page still keeps its original bytes",
        read64(low_phys) == expected_low_after_second_fault &&
        read64(high_phys) == high_pattern
    );

    ai_micro_m_memblock_reset_env();

    TEST_END();
}

bool ai_micro_m_cross_page_fault_switch_load_then_store_keeps_latest_cause_and_tval_corner()
{
    TEST_START();

    const enum test_page low_page = SWITCH1;
    const enum test_page high_page = SWITCH2;
    const uintptr_t vaddr = ai_micro_m_memblock_cross_page_addr(low_page, 4U);
    const uintptr_t second_half_vaddr = vaddr + 4U;
    const uintptr_t low_phys = ai_micro_m_memblock_page_end_phys(low_page);
    const uintptr_t high_phys = phys_page_base(high_page);
    const uint64_t low_pattern = 0x1122334455667788ULL;
    const uint64_t high_pattern = 0x99aabbccddeeff00ULL;
    const uint64_t store_value = 0xfedcba9876543210ULL;
    const uint64_t expected_low_after_store_fault = low_pattern;

    goto_priv(PRIV_M);
    ai_micro_m_memblock_reset_env();
    write64(low_phys, low_pattern);
    write64(high_phys, high_pattern);

    ai_micro_m_memblock_prepare_supervisor_translation();
    ai_micro_m_memblock_set_leaf(low_page, PTE_V | PTE_A | PTE_X);
    ai_micro_m_memblock_set_leaf(high_page, PTE_V | PTE_AD | PTE_RWX);
    sfence_vma();

    ai_micro_m_memblock_enable_mprv_supervisor_translation();
    TEST_SETUP_EXCEPT();
    ld(vaddr);
    TEST_ASSERT("the first shared cross-page exception probe comes from the translated load path and reports a low-half load page fault with tval at the original split-load address",
        excpt.triggered == true &&
        excpt.cause == CAUSE_LPF &&
        excpt.tval == vaddr
    );

    goto_priv(PRIV_M);
    TEST_ASSERT("after the first low-half load page fault, both participating boundary doublewords remain unchanged before the follow-up translated store-fault probe",
        read64(low_phys) == low_pattern &&
        read64(high_phys) == high_pattern
    );

    ai_micro_m_memblock_set_leaf(low_page, PTE_V | PTE_AD | PTE_RWX);
    ai_micro_m_memblock_set_leaf(high_page, PTE_V | PTE_AD | PTE_RX);
    sfence_vma();

    ai_micro_m_memblock_enable_mprv_supervisor_translation();
    TEST_SETUP_EXCEPT();
    sd(vaddr, store_value);
    TEST_ASSERT("when the next shared cross-page exception comes from the translated store path on the same split window, MemBlock must refresh both cause and tval to the newer high-half store page fault",
        excpt.triggered == true &&
        excpt.cause == CAUSE_SPF &&
        excpt.tval == second_half_vaddr
    );

    goto_priv(PRIV_M);
    TEST_ASSERT("the load-then-store shared-fault switch still shows no newer store-side low-half side effect while the faulting upper page keeps its original bytes",
        read64(low_phys) == expected_low_after_store_fault &&
        read64(high_phys) == high_pattern
    );

    ai_micro_m_memblock_reset_env();

    TEST_END();
}

bool ai_micro_m_cross_page_fault_switch_store_then_load_keeps_latest_cause_and_tval_corner()
{
    TEST_START();

    const enum test_page low_page = SWITCH1;
    const enum test_page high_page = SWITCH2;
    const uintptr_t vaddr = ai_micro_m_memblock_cross_page_addr(low_page, 4U);
    const uintptr_t second_half_vaddr = vaddr + 4U;
    const uintptr_t low_phys = ai_micro_m_memblock_page_end_phys(low_page);
    const uintptr_t high_phys = phys_page_base(high_page);
    const uint64_t low_pattern = 0x1122334455667788ULL;
    const uint64_t high_pattern = 0x99aabbccddeeff00ULL;
    const uint64_t store_value = 0xfedcba9876543210ULL;
    const uint64_t expected_low_after_store_fault = low_pattern;

    goto_priv(PRIV_M);
    ai_micro_m_memblock_reset_env();
    write64(low_phys, low_pattern);
    write64(high_phys, high_pattern);

    ai_micro_m_memblock_prepare_supervisor_translation();
    ai_micro_m_memblock_set_leaf(low_page, PTE_V | PTE_AD | PTE_RWX);
    ai_micro_m_memblock_set_leaf(high_page, PTE_V | PTE_AD | PTE_RX);
    sfence_vma();

    ai_micro_m_memblock_enable_mprv_supervisor_translation();
    TEST_SETUP_EXCEPT();
    sd(vaddr, store_value);
    TEST_ASSERT("the first shared cross-page exception probe comes from the translated store path and reports a high-half store page fault with tval at the second-half start address",
        excpt.triggered == true &&
        excpt.cause == CAUSE_SPF &&
        excpt.tval == second_half_vaddr
    );

    goto_priv(PRIV_M);
    TEST_ASSERT("after the first high-half store page fault, no low-half side effect from the store becomes visible while the faulting upper-page bytes remain unchanged",
        read64(low_phys) == expected_low_after_store_fault &&
        read64(high_phys) == high_pattern
    );

    ai_micro_m_memblock_set_leaf(low_page, PTE_V | PTE_A | PTE_X);
    ai_micro_m_memblock_set_leaf(high_page, PTE_V | PTE_AD | PTE_RWX);
    sfence_vma();

    ai_micro_m_memblock_enable_mprv_supervisor_translation();
    TEST_SETUP_EXCEPT();
    ld(vaddr);
    TEST_ASSERT("when the next shared cross-page exception comes from the translated load path on the same split window, MemBlock must refresh both cause and tval to the newer low-half load page fault",
        excpt.triggered == true &&
        excpt.cause == CAUSE_LPF &&
        excpt.tval == vaddr
    );

    goto_priv(PRIV_M);
    TEST_ASSERT("the store-then-load shared-fault switch keeps the earlier store-side fault side-effect-free while the follow-up load fault adds no new memory side effects",
        read64(low_phys) == expected_low_after_store_fault &&
        read64(high_phys) == high_pattern
    );

    ai_micro_m_memblock_reset_env();

    TEST_END();
}

bool ai_micro_m_load_cross_page_access_fault_tval_refreshes_from_low_half_to_high_half_corner()
{
    TEST_START();

    const enum test_page low_page = SWITCH1;
    const enum test_page high_page = SWITCH2;
    const uintptr_t vaddr = ai_micro_m_memblock_cross_page_addr(low_page, 4U);
    const uintptr_t second_half_vaddr = vaddr + 4U;
    const uintptr_t low_phys = ai_micro_m_memblock_page_end_phys(low_page);
    const uintptr_t high_phys = phys_page_base(high_page);
    const uintptr_t low_deny_start = phys_page_base(low_page);
    const uintptr_t low_deny_end = low_deny_start + PAGE_SIZE;
    const uintptr_t high_deny_start = phys_page_base(high_page);
    const uintptr_t high_deny_end = high_deny_start + PAGE_SIZE;
    const uint64_t low_pattern = 0x0f1e2d3c4b5a6978ULL;
    const uint64_t high_pattern = 0x8877665544332211ULL;

    goto_priv(PRIV_M);
    ai_micro_m_memblock_reset_env();
    write64(low_phys, low_pattern);
    write64(high_phys, high_pattern);
    ai_micro_m_memblock_configure_pmp_window(
        low_deny_start, low_deny_end, AI_MICRO_MEMBLOCK_PMP_TOR_NO_R);

    ai_micro_m_memblock_prepare_supervisor_translation();
    ai_micro_m_memblock_set_leaf(low_page, PTE_V | PTE_AD | PTE_RWX);
    ai_micro_m_memblock_set_leaf(high_page, PTE_V | PTE_AD | PTE_RWX);
    sfence_vma();

    ai_micro_m_memblock_enable_mprv_supervisor_translation();
    TEST_SETUP_EXCEPT();
    ld(vaddr);
    TEST_ASSERT("under no-H M-mode+MPRV=S translation, the first split-load probe reports a low-half access fault with tval bound to the original misaligned cross-page address",
        excpt.triggered == true &&
        excpt.cause == CAUSE_LAF &&
        excpt.tval == vaddr
    );

    goto_priv(PRIV_M);
    TEST_ASSERT("the first low-half access-faulting split load leaves both participating pages unchanged before the same-window high-half refresh probe",
        read64(low_phys) == low_pattern &&
        read64(high_phys) == high_pattern
    );
    ai_micro_m_memblock_configure_pmp_window(
        high_deny_start, high_deny_end, AI_MICRO_MEMBLOCK_PMP_TOR_NO_R);

    ai_micro_m_memblock_enable_mprv_supervisor_translation();
    TEST_SETUP_EXCEPT();
    ld(vaddr);
    TEST_ASSERT("after the earlier low-half access fault has already exercised the shared exception path, the follow-up high-half access fault must refresh tval to the second-half start address",
        excpt.triggered == true &&
        excpt.cause == CAUSE_LAF &&
        excpt.tval == second_half_vaddr
    );

    goto_priv(PRIV_M);
    TEST_ASSERT("the refreshed high-half access-fault probe still leaves both boundary doublewords unchanged because faulting split loads must not create visible memory side effects",
        read64(low_phys) == low_pattern &&
        read64(high_phys) == high_pattern
    );

    ai_micro_m_memblock_reset_env();

    TEST_END();
}

bool ai_micro_m_store_cross_page_access_fault_tval_refreshes_from_low_half_to_high_half_corner()
{
    TEST_START();

    const enum test_page low_page = SWITCH1;
    const enum test_page high_page = SWITCH2;
    const uintptr_t vaddr = ai_micro_m_memblock_cross_page_addr(low_page, 4U);
    const uintptr_t second_half_vaddr = vaddr + 4U;
    const uintptr_t low_phys = ai_micro_m_memblock_page_end_phys(low_page);
    const uintptr_t high_phys = phys_page_base(high_page);
    const uintptr_t low_deny_start = phys_page_base(low_page);
    const uintptr_t low_deny_end = low_deny_start + PAGE_SIZE;
    const uintptr_t high_deny_start = phys_page_base(high_page);
    const uintptr_t high_deny_end = high_deny_start + PAGE_SIZE;
    const uint64_t low_pattern = 0x1122334455667788ULL;
    const uint64_t high_pattern = 0x99aabbccddeeff00ULL;
    const uint64_t first_store_value = 0x0123456789abcdefULL;
    const uint64_t second_store_value = 0xfedcba9876543210ULL;
    const uint64_t expected_low_after_second_fault = low_pattern;

    goto_priv(PRIV_M);
    ai_micro_m_memblock_reset_env();
    write64(low_phys, low_pattern);
    write64(high_phys, high_pattern);
    ai_micro_m_memblock_configure_pmp_window(
        low_deny_start, low_deny_end, AI_MICRO_MEMBLOCK_PMP_TOR_NO_W);

    ai_micro_m_memblock_prepare_supervisor_translation();
    ai_micro_m_memblock_set_leaf(low_page, PTE_V | PTE_AD | PTE_RWX);
    ai_micro_m_memblock_set_leaf(high_page, PTE_V | PTE_AD | PTE_RWX);
    sfence_vma();

    ai_micro_m_memblock_enable_mprv_supervisor_translation();
    TEST_SETUP_EXCEPT();
    sd(vaddr, first_store_value);
    TEST_ASSERT("under no-H M-mode+MPRV=S translation, the first split-store probe reports a low-half access fault with tval bound to the original misaligned cross-page address",
        excpt.triggered == true &&
        excpt.cause == CAUSE_SAF &&
        excpt.tval == vaddr
    );

    goto_priv(PRIV_M);
    TEST_ASSERT("the first low-half access-faulting split store leaves both participating pages unchanged before the same-window high-half refresh probe",
        read64(low_phys) == low_pattern &&
        read64(high_phys) == high_pattern
    );
    ai_micro_m_memblock_configure_pmp_window(
        high_deny_start, high_deny_end, AI_MICRO_MEMBLOCK_PMP_TOR_NO_W);

    ai_micro_m_memblock_enable_mprv_supervisor_translation();
    TEST_SETUP_EXCEPT();
    sd(vaddr, second_store_value);
    TEST_ASSERT("after the earlier low-half access fault has already exercised the shared exception path, the follow-up high-half access fault must refresh tval to the second-half start address",
        excpt.triggered == true &&
        excpt.cause == CAUSE_SAF &&
        excpt.tval == second_half_vaddr
    );

    goto_priv(PRIV_M);
    TEST_ASSERT("the refreshed high-half access-fault probe still exposes no low-half side effect from the newer store value while the protected upper page still keeps its original bytes",
        read64(low_phys) == expected_low_after_second_fault &&
        read64(high_phys) == high_pattern
    );

    ai_micro_m_memblock_reset_env();

    TEST_END();
}

bool ai_micro_m_cross_page_access_fault_switch_load_then_store_keeps_latest_cause_and_tval_corner()
{
    TEST_START();

    const enum test_page low_page = SWITCH1;
    const enum test_page high_page = SWITCH2;
    const uintptr_t vaddr = ai_micro_m_memblock_cross_page_addr(low_page, 4U);
    const uintptr_t second_half_vaddr = vaddr + 4U;
    const uintptr_t low_phys = ai_micro_m_memblock_page_end_phys(low_page);
    const uintptr_t high_phys = phys_page_base(high_page);
    const uintptr_t low_deny_start = phys_page_base(low_page);
    const uintptr_t low_deny_end = low_deny_start + PAGE_SIZE;
    const uintptr_t high_deny_start = phys_page_base(high_page);
    const uintptr_t high_deny_end = high_deny_start + PAGE_SIZE;
    const uint64_t low_pattern = 0x1122334455667788ULL;
    const uint64_t high_pattern = 0x99aabbccddeeff00ULL;
    const uint64_t store_value = 0xfedcba9876543210ULL;
    const uint64_t expected_low_after_store_fault = low_pattern;

    goto_priv(PRIV_M);
    ai_micro_m_memblock_reset_env();
    write64(low_phys, low_pattern);
    write64(high_phys, high_pattern);
    ai_micro_m_memblock_configure_pmp_window(
        low_deny_start, low_deny_end, AI_MICRO_MEMBLOCK_PMP_TOR_NO_R);

    ai_micro_m_memblock_prepare_supervisor_translation();
    ai_micro_m_memblock_set_leaf(low_page, PTE_V | PTE_AD | PTE_RWX);
    ai_micro_m_memblock_set_leaf(high_page, PTE_V | PTE_AD | PTE_RWX);
    sfence_vma();

    ai_micro_m_memblock_enable_mprv_supervisor_translation();
    TEST_SETUP_EXCEPT();
    ld(vaddr);
    TEST_ASSERT("the first shared cross-page exception probe comes from the translated load path and reports a low-half load access fault with tval at the original split-load address",
        excpt.triggered == true &&
        excpt.cause == CAUSE_LAF &&
        excpt.tval == vaddr
    );

    goto_priv(PRIV_M);
    TEST_ASSERT("after the first low-half load access fault, both participating boundary doublewords remain unchanged before the follow-up translated store-fault probe",
        read64(low_phys) == low_pattern &&
        read64(high_phys) == high_pattern
    );
    ai_micro_m_memblock_configure_pmp_window(
        high_deny_start, high_deny_end, AI_MICRO_MEMBLOCK_PMP_TOR_NO_W);

    ai_micro_m_memblock_enable_mprv_supervisor_translation();
    TEST_SETUP_EXCEPT();
    sd(vaddr, store_value);
    TEST_ASSERT("when the next shared cross-page exception comes from the translated store path on the same split window, MemBlock must refresh both cause and tval to the newer high-half store access fault",
        excpt.triggered == true &&
        excpt.cause == CAUSE_SAF &&
        excpt.tval == second_half_vaddr
    );

    goto_priv(PRIV_M);
    TEST_ASSERT("the load-then-store shared access-fault switch still shows no newer store-side low-half side effect while the protected upper page keeps its original bytes",
        read64(low_phys) == expected_low_after_store_fault &&
        read64(high_phys) == high_pattern
    );

    ai_micro_m_memblock_reset_env();

    TEST_END();
}

bool ai_micro_m_cross_page_access_fault_switch_store_then_load_keeps_latest_cause_and_tval_corner()
{
    TEST_START();

    const enum test_page low_page = SWITCH1;
    const enum test_page high_page = SWITCH2;
    const uintptr_t vaddr = ai_micro_m_memblock_cross_page_addr(low_page, 4U);
    const uintptr_t second_half_vaddr = vaddr + 4U;
    const uintptr_t low_phys = ai_micro_m_memblock_page_end_phys(low_page);
    const uintptr_t high_phys = phys_page_base(high_page);
    const uintptr_t low_deny_start = phys_page_base(low_page);
    const uintptr_t low_deny_end = low_deny_start + PAGE_SIZE;
    const uintptr_t high_deny_start = phys_page_base(high_page);
    const uintptr_t high_deny_end = high_deny_start + PAGE_SIZE;
    const uint64_t low_pattern = 0x1122334455667788ULL;
    const uint64_t high_pattern = 0x99aabbccddeeff00ULL;
    const uint64_t store_value = 0xfedcba9876543210ULL;
    const uint64_t expected_low_after_store_fault = low_pattern;

    goto_priv(PRIV_M);
    ai_micro_m_memblock_reset_env();
    write64(low_phys, low_pattern);
    write64(high_phys, high_pattern);
    ai_micro_m_memblock_configure_pmp_window(
        high_deny_start, high_deny_end, AI_MICRO_MEMBLOCK_PMP_TOR_NO_W);

    ai_micro_m_memblock_prepare_supervisor_translation();
    ai_micro_m_memblock_set_leaf(low_page, PTE_V | PTE_AD | PTE_RWX);
    ai_micro_m_memblock_set_leaf(high_page, PTE_V | PTE_AD | PTE_RWX);
    sfence_vma();

    ai_micro_m_memblock_enable_mprv_supervisor_translation();
    TEST_SETUP_EXCEPT();
    sd(vaddr, store_value);
    TEST_ASSERT("the first shared cross-page exception probe comes from the translated store path and reports a high-half store access fault with tval at the second-half start address",
        excpt.triggered == true &&
        excpt.cause == CAUSE_SAF &&
        excpt.tval == second_half_vaddr
    );

    goto_priv(PRIV_M);
    TEST_ASSERT("after the first high-half store access fault, no low-half side effect from the store becomes visible while the protected upper-page bytes remain unchanged",
        read64(low_phys) == expected_low_after_store_fault &&
        read64(high_phys) == high_pattern
    );
    ai_micro_m_memblock_configure_pmp_window(
        low_deny_start, low_deny_end, AI_MICRO_MEMBLOCK_PMP_TOR_NO_R);

    ai_micro_m_memblock_enable_mprv_supervisor_translation();
    TEST_SETUP_EXCEPT();
    ld(vaddr);
    TEST_ASSERT("when the next shared cross-page exception comes from the translated load path on the same split window, MemBlock must refresh both cause and tval to the newer low-half load access fault",
        excpt.triggered == true &&
        excpt.cause == CAUSE_LAF &&
        excpt.tval == vaddr
    );

    goto_priv(PRIV_M);
    TEST_ASSERT("the store-then-load shared access-fault switch keeps the earlier store-side fault side-effect-free while the follow-up load fault adds no new memory side effects",
        read64(low_phys) == expected_low_after_store_fault &&
        read64(high_phys) == high_pattern
    );

    ai_micro_m_memblock_reset_env();

    TEST_END();
}

bool ai_micro_m_load_cross_page_page_fault_then_access_fault_keeps_latest_cause_and_tval_corner()
{
    TEST_START();

    const enum test_page low_page = SWITCH1;
    const enum test_page high_page = SWITCH2;
    const uintptr_t vaddr = ai_micro_m_memblock_cross_page_addr(low_page, 4U);
    const uintptr_t second_half_vaddr = vaddr + 4U;
    const uintptr_t low_phys = ai_micro_m_memblock_page_end_phys(low_page);
    const uintptr_t high_phys = phys_page_base(high_page);
    const uintptr_t high_deny_start = phys_page_base(high_page);
    const uintptr_t high_deny_end = high_deny_start + PAGE_SIZE;
    const uint64_t low_pattern = 0x0f1e2d3c4b5a6978ULL;
    const uint64_t high_pattern = 0x8877665544332211ULL;

    goto_priv(PRIV_M);
    ai_micro_m_memblock_reset_env();
    write64(low_phys, low_pattern);
    write64(high_phys, high_pattern);

    ai_micro_m_memblock_prepare_supervisor_translation();
    ai_micro_m_memblock_set_leaf(low_page, PTE_V | PTE_A | PTE_X);
    ai_micro_m_memblock_set_leaf(high_page, PTE_V | PTE_AD | PTE_RWX);
    sfence_vma();

    ai_micro_m_memblock_enable_mprv_supervisor_translation();
    TEST_SETUP_EXCEPT();
    ld(vaddr);
    TEST_ASSERT("the first same-producer cross-page fault probe comes from the translated load path and reports a low-half load page fault with tval at the original split-load address",
        excpt.triggered == true &&
        excpt.cause == CAUSE_LPF &&
        excpt.tval == vaddr
    );

    goto_priv(PRIV_M);
    TEST_ASSERT("after the first low-half load page fault, both participating boundary doublewords remain unchanged before the follow-up high-half load access-fault probe",
        read64(low_phys) == low_pattern &&
        read64(high_phys) == high_pattern
    );

    ai_micro_m_memblock_set_leaf(low_page, PTE_V | PTE_AD | PTE_RWX);
    ai_micro_m_memblock_set_leaf(high_page, PTE_V | PTE_AD | PTE_RWX);
    sfence_vma();
    ai_micro_m_memblock_configure_pmp_window(
        high_deny_start, high_deny_end, AI_MICRO_MEMBLOCK_PMP_TOR_NO_R);

    ai_micro_m_memblock_enable_mprv_supervisor_translation();
    TEST_SETUP_EXCEPT();
    ld(vaddr);
    TEST_ASSERT("when the next same-producer split-load fault switches from page fault to access fault on the high half, MemBlock must refresh both cause and tval to the newer load access fault",
        excpt.triggered == true &&
        excpt.cause == CAUSE_LAF &&
        excpt.tval == second_half_vaddr
    );

    goto_priv(PRIV_M);
    TEST_ASSERT("the page-fault-then-access-fault split-load sequence still leaves both boundary doublewords unchanged because neither faulting load may create visible memory side effects",
        read64(low_phys) == low_pattern &&
        read64(high_phys) == high_pattern
    );

    ai_micro_m_memblock_reset_env();

    TEST_END();
}

bool ai_micro_m_load_cross_page_access_fault_then_page_fault_keeps_latest_cause_and_tval_corner()
{
    TEST_START();

    const enum test_page low_page = SWITCH1;
    const enum test_page high_page = SWITCH2;
    const uintptr_t vaddr = ai_micro_m_memblock_cross_page_addr(low_page, 4U);
    const uintptr_t second_half_vaddr = vaddr + 4U;
    const uintptr_t low_phys = ai_micro_m_memblock_page_end_phys(low_page);
    const uintptr_t high_phys = phys_page_base(high_page);
    const uintptr_t low_deny_start = phys_page_base(low_page);
    const uintptr_t low_deny_end = low_deny_start + PAGE_SIZE;
    const uint64_t low_pattern = 0x0f1e2d3c4b5a6978ULL;
    const uint64_t high_pattern = 0x8877665544332211ULL;

    goto_priv(PRIV_M);
    ai_micro_m_memblock_reset_env();
    write64(low_phys, low_pattern);
    write64(high_phys, high_pattern);
    ai_micro_m_memblock_configure_pmp_window(
        low_deny_start, low_deny_end, AI_MICRO_MEMBLOCK_PMP_TOR_NO_R);

    ai_micro_m_memblock_prepare_supervisor_translation();
    ai_micro_m_memblock_set_leaf(low_page, PTE_V | PTE_AD | PTE_RWX);
    ai_micro_m_memblock_set_leaf(high_page, PTE_V | PTE_AD | PTE_RWX);
    sfence_vma();

    ai_micro_m_memblock_enable_mprv_supervisor_translation();
    TEST_SETUP_EXCEPT();
    ld(vaddr);
    TEST_ASSERT("the first same-producer cross-page fault probe comes from the translated load path and reports a low-half load access fault with tval at the original split-load address",
        excpt.triggered == true &&
        excpt.cause == CAUSE_LAF &&
        excpt.tval == vaddr
    );

    goto_priv(PRIV_M);
    TEST_ASSERT("after the first low-half load access fault, both participating boundary doublewords remain unchanged before the follow-up high-half load page-fault probe",
        read64(low_phys) == low_pattern &&
        read64(high_phys) == high_pattern
    );

    ai_micro_m_memblock_restore_pmp_full_access();
    ai_micro_m_memblock_set_leaf(low_page, PTE_V | PTE_AD | PTE_RWX);
    ai_micro_m_memblock_set_leaf(high_page, PTE_V | PTE_A | PTE_X);
    sfence_vma();

    ai_micro_m_memblock_enable_mprv_supervisor_translation();
    TEST_SETUP_EXCEPT();
    ld(vaddr);
    TEST_ASSERT("when the next same-producer split-load fault switches from access fault to page fault on the high half, MemBlock must refresh both cause and tval to the newer load page fault",
        excpt.triggered == true &&
        excpt.cause == CAUSE_LPF &&
        excpt.tval == second_half_vaddr
    );

    goto_priv(PRIV_M);
    TEST_ASSERT("the access-fault-then-page-fault split-load sequence still leaves both boundary doublewords unchanged because neither faulting load may create visible memory side effects",
        read64(low_phys) == low_pattern &&
        read64(high_phys) == high_pattern
    );

    ai_micro_m_memblock_reset_env();

    TEST_END();
}

bool ai_micro_m_store_cross_page_page_fault_then_access_fault_keeps_latest_cause_and_tval_corner()
{
    TEST_START();

    const enum test_page low_page = SWITCH1;
    const enum test_page high_page = SWITCH2;
    const uintptr_t vaddr = ai_micro_m_memblock_cross_page_addr(low_page, 4U);
    const uintptr_t second_half_vaddr = vaddr + 4U;
    const uintptr_t low_phys = ai_micro_m_memblock_page_end_phys(low_page);
    const uintptr_t high_phys = phys_page_base(high_page);
    const uintptr_t high_deny_start = phys_page_base(high_page);
    const uintptr_t high_deny_end = high_deny_start + PAGE_SIZE;
    const uint64_t low_pattern = 0x1122334455667788ULL;
    const uint64_t high_pattern = 0x99aabbccddeeff00ULL;
    const uint64_t store_value = 0xfedcba9876543210ULL;
    const uint64_t expected_low_after_second_fault = low_pattern;

    goto_priv(PRIV_M);
    ai_micro_m_memblock_reset_env();
    write64(low_phys, low_pattern);
    write64(high_phys, high_pattern);

    ai_micro_m_memblock_prepare_supervisor_translation();
    ai_micro_m_memblock_set_leaf(low_page, PTE_V | PTE_A | PTE_R);
    ai_micro_m_memblock_set_leaf(high_page, PTE_V | PTE_AD | PTE_RWX);
    sfence_vma();

    ai_micro_m_memblock_enable_mprv_supervisor_translation();
    TEST_SETUP_EXCEPT();
    sd(vaddr, store_value);
    TEST_ASSERT("the first same-producer cross-page fault probe comes from the translated store path and reports a low-half store page fault with tval at the original split-store address",
        excpt.triggered == true &&
        excpt.cause == CAUSE_SPF &&
        excpt.tval == vaddr
    );

    goto_priv(PRIV_M);
    TEST_ASSERT("after the first low-half store page fault, both participating pages remain unchanged before the follow-up high-half store access-fault probe",
        read64(low_phys) == low_pattern &&
        read64(high_phys) == high_pattern
    );

    ai_micro_m_memblock_set_leaf(low_page, PTE_V | PTE_AD | PTE_RWX);
    ai_micro_m_memblock_set_leaf(high_page, PTE_V | PTE_AD | PTE_RWX);
    sfence_vma();
    ai_micro_m_memblock_configure_pmp_window(
        high_deny_start, high_deny_end, AI_MICRO_MEMBLOCK_PMP_TOR_NO_W);

    ai_micro_m_memblock_enable_mprv_supervisor_translation();
    TEST_SETUP_EXCEPT();
    sd(vaddr, store_value);
    TEST_ASSERT("when the next same-producer split-store fault switches from page fault to access fault on the high half, MemBlock must refresh both cause and tval to the newer store access fault",
        excpt.triggered == true &&
        excpt.cause == CAUSE_SAF &&
        excpt.tval == second_half_vaddr
    );

    goto_priv(PRIV_M);
    TEST_ASSERT("the page-fault-then-access-fault split-store sequence still exposes no low-half side effect from the newer store value while the protected upper page keeps its original bytes",
        read64(low_phys) == expected_low_after_second_fault &&
        read64(high_phys) == high_pattern
    );

    ai_micro_m_memblock_reset_env();

    TEST_END();
}

bool ai_micro_m_store_cross_page_access_fault_then_page_fault_keeps_latest_cause_and_tval_corner()
{
    TEST_START();

    const enum test_page low_page = SWITCH1;
    const enum test_page high_page = SWITCH2;
    const uintptr_t vaddr = ai_micro_m_memblock_cross_page_addr(low_page, 4U);
    const uintptr_t second_half_vaddr = vaddr + 4U;
    const uintptr_t low_phys = ai_micro_m_memblock_page_end_phys(low_page);
    const uintptr_t high_phys = phys_page_base(high_page);
    const uintptr_t low_deny_start = phys_page_base(low_page);
    const uintptr_t low_deny_end = low_deny_start + PAGE_SIZE;
    const uint64_t low_pattern = 0x1122334455667788ULL;
    const uint64_t high_pattern = 0x99aabbccddeeff00ULL;
    const uint64_t store_value = 0xfedcba9876543210ULL;
    const uint64_t expected_low_after_second_fault = low_pattern;

    goto_priv(PRIV_M);
    ai_micro_m_memblock_reset_env();
    write64(low_phys, low_pattern);
    write64(high_phys, high_pattern);
    ai_micro_m_memblock_configure_pmp_window(
        low_deny_start, low_deny_end, AI_MICRO_MEMBLOCK_PMP_TOR_NO_W);

    ai_micro_m_memblock_prepare_supervisor_translation();
    ai_micro_m_memblock_set_leaf(low_page, PTE_V | PTE_AD | PTE_RWX);
    ai_micro_m_memblock_set_leaf(high_page, PTE_V | PTE_AD | PTE_RWX);
    sfence_vma();

    ai_micro_m_memblock_enable_mprv_supervisor_translation();
    TEST_SETUP_EXCEPT();
    sd(vaddr, store_value);
    TEST_ASSERT("the first same-producer cross-page fault probe comes from the translated store path and reports a low-half store access fault with tval at the original split-store address",
        excpt.triggered == true &&
        excpt.cause == CAUSE_SAF &&
        excpt.tval == vaddr
    );

    goto_priv(PRIV_M);
    TEST_ASSERT("after the first low-half store access fault, both participating pages remain unchanged before the follow-up high-half store page-fault probe",
        read64(low_phys) == low_pattern &&
        read64(high_phys) == high_pattern
    );

    ai_micro_m_memblock_restore_pmp_full_access();
    ai_micro_m_memblock_set_leaf(low_page, PTE_V | PTE_AD | PTE_RWX);
    ai_micro_m_memblock_set_leaf(high_page, PTE_V | PTE_AD | PTE_RX);
    sfence_vma();

    ai_micro_m_memblock_enable_mprv_supervisor_translation();
    TEST_SETUP_EXCEPT();
    sd(vaddr, store_value);
    TEST_ASSERT("when the next same-producer split-store fault switches from access fault to page fault on the high half, MemBlock must refresh both cause and tval to the newer store page fault",
        excpt.triggered == true &&
        excpt.cause == CAUSE_SPF &&
        excpt.tval == second_half_vaddr
    );

    goto_priv(PRIV_M);
    TEST_ASSERT("the access-fault-then-page-fault split-store sequence still exposes no low-half side effect from the newer store value while the faulting upper page keeps its original bytes",
        read64(low_phys) == expected_low_after_second_fault &&
        read64(high_phys) == high_pattern
    );

    ai_micro_m_memblock_reset_env();

    TEST_END();
}

bool ai_micro_m_load_cross_page_high_half_page_fault_then_same_address_valid_load_recovers_corner()
{
    TEST_START();

    const enum test_page low_page = SWITCH1;
    const enum test_page high_page = SWITCH2;
    const uintptr_t vaddr = ai_micro_m_memblock_cross_page_addr(low_page, 4U);
    const uintptr_t second_half_vaddr = vaddr + 4U;
    const uintptr_t low_phys = ai_micro_m_memblock_page_end_phys(low_page);
    const uintptr_t high_phys = phys_page_base(high_page);
    const uint64_t low_pattern = 0x0f1e2d3c4b5a6978ULL;
    const uint64_t high_pattern = 0x8877665544332211ULL;
    const uint64_t expected_value =
        ai_micro_m_memblock_expected_cross_page_4b4b_load(
            low_pattern, high_pattern);
    uint64_t value = 0;

    goto_priv(PRIV_M);
    ai_micro_m_memblock_reset_env();
    write64(low_phys, low_pattern);
    write64(high_phys, high_pattern);

    ai_micro_m_memblock_prepare_supervisor_translation();
    ai_micro_m_memblock_set_leaf(low_page, PTE_V | PTE_AD | PTE_RWX);
    ai_micro_m_memblock_set_leaf(high_page, PTE_V | PTE_A | PTE_X);
    sfence_vma();

    ai_micro_m_memblock_enable_mprv_supervisor_translation();
    TEST_SETUP_EXCEPT();
    ld(vaddr);
    TEST_ASSERT("the first translated split-load probe reports a high-half load page fault when only the upper page loses read permission",
        excpt.triggered == true &&
        excpt.cause == CAUSE_LPF &&
        excpt.tval == second_half_vaddr
    );

    goto_priv(PRIV_M);
    TEST_ASSERT("the high-half load page-fault path leaves both participating boundary doublewords unchanged before same-address recovery is attempted",
        read64(low_phys) == low_pattern &&
        read64(high_phys) == high_pattern
    );

    ai_micro_m_memblock_set_leaf(high_page, PTE_V | PTE_AD | PTE_RWX);
    sfence_vma();

    ai_micro_m_memblock_enable_mprv_supervisor_translation();
    TEST_SETUP_EXCEPT();
    value = ld(vaddr);
    TEST_ASSERT("after repairing the upper-page permission, the same translated split-load address must complete normally and return the freshly merged 4B+4B payload",
        excpt.triggered == false &&
        value == expected_value
    );

    goto_priv(PRIV_M);
    TEST_ASSERT("the successful same-address split-load recovery still leaves both participating boundary doublewords unchanged",
        read64(low_phys) == low_pattern &&
        read64(high_phys) == high_pattern
    );

    ai_micro_m_memblock_reset_env();

    TEST_END();
}

bool ai_micro_m_load_cross_page_high_half_access_fault_then_same_address_valid_load_recovers_corner()
{
    TEST_START();

    const enum test_page low_page = SWITCH1;
    const enum test_page high_page = SWITCH2;
    const uintptr_t vaddr = ai_micro_m_memblock_cross_page_addr(low_page, 4U);
    const uintptr_t second_half_vaddr = vaddr + 4U;
    const uintptr_t low_phys = ai_micro_m_memblock_page_end_phys(low_page);
    const uintptr_t high_phys = phys_page_base(high_page);
    const uintptr_t high_deny_start = phys_page_base(high_page);
    const uintptr_t high_deny_end = high_deny_start + PAGE_SIZE;
    const uint64_t low_pattern = 0x0f1e2d3c4b5a6978ULL;
    const uint64_t high_pattern = 0x8877665544332211ULL;
    const uint64_t expected_value =
        ai_micro_m_memblock_expected_cross_page_4b4b_load(
            low_pattern, high_pattern);
    uint64_t value = 0;

    goto_priv(PRIV_M);
    ai_micro_m_memblock_reset_env();
    write64(low_phys, low_pattern);
    write64(high_phys, high_pattern);
    ai_micro_m_memblock_configure_pmp_window(
        high_deny_start, high_deny_end, AI_MICRO_MEMBLOCK_PMP_TOR_NO_R);

    ai_micro_m_memblock_prepare_supervisor_translation();
    ai_micro_m_memblock_set_leaf(low_page, PTE_V | PTE_AD | PTE_RWX);
    ai_micro_m_memblock_set_leaf(high_page, PTE_V | PTE_AD | PTE_RWX);
    sfence_vma();

    ai_micro_m_memblock_enable_mprv_supervisor_translation();
    TEST_SETUP_EXCEPT();
    ld(vaddr);
    TEST_ASSERT("the first translated split-load probe reports a high-half load access fault when only the upper page loses PMP read permission",
        excpt.triggered == true &&
        excpt.cause == CAUSE_LAF &&
        excpt.tval == second_half_vaddr
    );

    goto_priv(PRIV_M);
    TEST_ASSERT("the high-half load access-fault path leaves both participating boundary doublewords unchanged before same-address recovery is attempted",
        read64(low_phys) == low_pattern &&
        read64(high_phys) == high_pattern
    );

    ai_micro_m_memblock_restore_pmp_full_access();

    ai_micro_m_memblock_enable_mprv_supervisor_translation();
    TEST_SETUP_EXCEPT();
    value = ld(vaddr);
    TEST_ASSERT("after restoring PMP read permission, the same translated split-load address must complete normally and return the freshly merged 4B+4B payload",
        excpt.triggered == false &&
        value == expected_value
    );

    goto_priv(PRIV_M);
    TEST_ASSERT("the successful same-address split-load recovery after the access fault still leaves both participating boundary doublewords unchanged",
        read64(low_phys) == low_pattern &&
        read64(high_phys) == high_pattern
    );

    ai_micro_m_memblock_reset_env();

    TEST_END();
}

bool ai_micro_m_store_cross_page_high_half_page_fault_then_same_address_valid_store_recovers_corner()
{
    TEST_START();

    const enum test_page low_page = SWITCH1;
    const enum test_page high_page = SWITCH2;
    const uintptr_t vaddr = ai_micro_m_memblock_cross_page_addr(low_page, 4U);
    const uintptr_t second_half_vaddr = vaddr + 4U;
    const uintptr_t low_phys = ai_micro_m_memblock_page_end_phys(low_page);
    const uintptr_t high_phys = phys_page_base(high_page);
    const uint64_t low_pattern = 0x1122334455667788ULL;
    const uint64_t high_pattern = 0x99aabbccddeeff00ULL;
    const uint64_t first_store_value = 0x0123456789abcdefULL;
    const uint64_t recovery_store_value = 0xfedcba9876543210ULL;
    const uint64_t expected_low_after_fault = low_pattern;
    const uint64_t expected_low_after_recovery =
        ai_micro_m_memblock_expected_low_after_4b_store(
            low_pattern, recovery_store_value);
    const uint64_t expected_high_after_recovery =
        ai_micro_m_memblock_expected_high_after_4b_store(
            high_pattern, recovery_store_value);

    goto_priv(PRIV_M);
    ai_micro_m_memblock_reset_env();
    write64(low_phys, low_pattern);
    write64(high_phys, high_pattern);

    ai_micro_m_memblock_prepare_supervisor_translation();
    ai_micro_m_memblock_set_leaf(low_page, PTE_V | PTE_AD | PTE_RWX);
    ai_micro_m_memblock_set_leaf(high_page, PTE_V | PTE_AD | PTE_RX);
    sfence_vma();

    ai_micro_m_memblock_enable_mprv_supervisor_translation();
    TEST_SETUP_EXCEPT();
    sd(vaddr, first_store_value);
    TEST_ASSERT("the first translated split-store probe reports a high-half store page fault when only the upper page loses write permission",
        excpt.triggered == true &&
        excpt.cause == CAUSE_SPF &&
        excpt.tval == second_half_vaddr
    );

    goto_priv(PRIV_M);
    TEST_ASSERT("the high-half store page-fault path exposes no low-half side effect from the first store value before same-address recovery is attempted",
        read64(low_phys) == expected_low_after_fault &&
        read64(high_phys) == high_pattern
    );

    ai_micro_m_memblock_set_leaf(high_page, PTE_V | PTE_AD | PTE_RWX);
    sfence_vma();

    ai_micro_m_memblock_enable_mprv_supervisor_translation();
    TEST_SETUP_EXCEPT();
    sd(vaddr, recovery_store_value);
    TEST_ASSERT("after repairing the upper-page permission, the same translated split-store address must complete normally without reusing stale exception state",
        excpt.triggered == false
    );

    goto_priv(PRIV_M);
    TEST_ASSERT("the successful same-address split-store recovery must update both halves to the newer store value and overwrite the earlier side-effect-free fault episode",
        read64(low_phys) == expected_low_after_recovery &&
        read64(high_phys) == expected_high_after_recovery
    );

    ai_micro_m_memblock_reset_env();

    TEST_END();
}

bool ai_micro_m_store_cross_page_high_half_access_fault_then_same_address_valid_store_recovers_corner()
{
    TEST_START();

    const enum test_page low_page = SWITCH1;
    const enum test_page high_page = SWITCH2;
    const uintptr_t vaddr = ai_micro_m_memblock_cross_page_addr(low_page, 4U);
    const uintptr_t second_half_vaddr = vaddr + 4U;
    const uintptr_t low_phys = ai_micro_m_memblock_page_end_phys(low_page);
    const uintptr_t high_phys = phys_page_base(high_page);
    const uintptr_t high_deny_start = phys_page_base(high_page);
    const uintptr_t high_deny_end = high_deny_start + PAGE_SIZE;
    const uint64_t low_pattern = 0x1122334455667788ULL;
    const uint64_t high_pattern = 0x99aabbccddeeff00ULL;
    const uint64_t first_store_value = 0x0123456789abcdefULL;
    const uint64_t recovery_store_value = 0xfedcba9876543210ULL;
    const uint64_t expected_low_after_fault = low_pattern;
    const uint64_t expected_low_after_recovery =
        ai_micro_m_memblock_expected_low_after_4b_store(
            low_pattern, recovery_store_value);
    const uint64_t expected_high_after_recovery =
        ai_micro_m_memblock_expected_high_after_4b_store(
            high_pattern, recovery_store_value);

    goto_priv(PRIV_M);
    ai_micro_m_memblock_reset_env();
    write64(low_phys, low_pattern);
    write64(high_phys, high_pattern);
    ai_micro_m_memblock_configure_pmp_window(
        high_deny_start, high_deny_end, AI_MICRO_MEMBLOCK_PMP_TOR_NO_W);

    ai_micro_m_memblock_prepare_supervisor_translation();
    ai_micro_m_memblock_set_leaf(low_page, PTE_V | PTE_AD | PTE_RWX);
    ai_micro_m_memblock_set_leaf(high_page, PTE_V | PTE_AD | PTE_RWX);
    sfence_vma();

    ai_micro_m_memblock_enable_mprv_supervisor_translation();
    TEST_SETUP_EXCEPT();
    sd(vaddr, first_store_value);
    TEST_ASSERT("the first translated split-store probe reports a high-half store access fault when only the upper page loses PMP write permission",
        excpt.triggered == true &&
        excpt.cause == CAUSE_SAF &&
        excpt.tval == second_half_vaddr
    );

    goto_priv(PRIV_M);
    TEST_ASSERT("the high-half store access-fault path exposes no low-half side effect from the first store value before same-address recovery is attempted",
        read64(low_phys) == expected_low_after_fault &&
        read64(high_phys) == high_pattern
    );

    ai_micro_m_memblock_restore_pmp_full_access();

    ai_micro_m_memblock_enable_mprv_supervisor_translation();
    TEST_SETUP_EXCEPT();
    sd(vaddr, recovery_store_value);
    TEST_ASSERT("after restoring PMP write permission, the same translated split-store address must complete normally without reusing stale exception state",
        excpt.triggered == false
    );

    goto_priv(PRIV_M);
    TEST_ASSERT("the successful same-address split-store recovery after the access fault must update both halves to the newer store value and overwrite the earlier side-effect-free fault episode",
        read64(low_phys) == expected_low_after_recovery &&
        read64(high_phys) == expected_high_after_recovery
    );

    ai_micro_m_memblock_reset_env();

    TEST_END();
}

bool ai_micro_m_load_cross_page_high_half_page_fault_repeated_then_same_address_valid_load_recovers_corner()
{
    TEST_START();

    const enum test_page low_page = SWITCH1;
    const enum test_page high_page = SWITCH2;
    const uintptr_t vaddr = ai_micro_m_memblock_cross_page_addr(low_page, 4U);
    const uintptr_t second_half_vaddr = vaddr + 4U;
    const uintptr_t low_phys = ai_micro_m_memblock_page_end_phys(low_page);
    const uintptr_t high_phys = phys_page_base(high_page);
    const uint64_t low_pattern = 0x0f1e2d3c4b5a6978ULL;
    const uint64_t high_pattern = 0x8877665544332211ULL;
    const uint64_t expected_value =
        ai_micro_m_memblock_expected_cross_page_4b4b_load(
            low_pattern, high_pattern);
    uint64_t value = 0;

    goto_priv(PRIV_M);
    ai_micro_m_memblock_reset_env();
    write64(low_phys, low_pattern);
    write64(high_phys, high_pattern);

    ai_micro_m_memblock_prepare_supervisor_translation();
    ai_micro_m_memblock_set_leaf(low_page, PTE_V | PTE_AD | PTE_RWX);
    ai_micro_m_memblock_set_leaf(high_page, PTE_V | PTE_A | PTE_X);
    sfence_vma();

    ai_micro_m_memblock_enable_mprv_supervisor_translation();
    TEST_SETUP_EXCEPT();
    ld(vaddr);
    TEST_ASSERT("the first translated split-load probe reports a high-half load page fault when only the upper page loses read permission in the repeated-fault recovery corner",
        excpt.triggered == true &&
        excpt.cause == CAUSE_LPF &&
        excpt.tval == second_half_vaddr
    );

    ai_micro_m_memblock_enable_mprv_supervisor_translation();
    TEST_SETUP_EXCEPT();
    ld(vaddr);
    TEST_ASSERT("a second probe to the same translated split-load window must keep reporting the same newer high-half load page fault instead of reusing any stale older address",
        excpt.triggered == true &&
        excpt.cause == CAUSE_LPF &&
        excpt.tval == second_half_vaddr
    );

    goto_priv(PRIV_M);
    TEST_ASSERT("the repeated high-half load page-fault probes still leave both participating boundary doublewords unchanged before repair",
        read64(low_phys) == low_pattern &&
        read64(high_phys) == high_pattern
    );

    ai_micro_m_memblock_set_leaf(high_page, PTE_V | PTE_AD | PTE_RWX);
    sfence_vma();

    ai_micro_m_memblock_enable_mprv_supervisor_translation();
    TEST_SETUP_EXCEPT();
    value = ld(vaddr);
    TEST_ASSERT("after repairing the upper-page permission, the same translated split-load address must recover successfully even after the repeated high-half page faults",
        excpt.triggered == false &&
        value == expected_value
    );

    goto_priv(PRIV_M);
    TEST_ASSERT("the successful repeated-fault split-load recovery still leaves both participating boundary doublewords unchanged",
        read64(low_phys) == low_pattern &&
        read64(high_phys) == high_pattern
    );

    ai_micro_m_memblock_reset_env();

    TEST_END();
}

bool ai_micro_m_load_cross_page_high_half_access_fault_repeated_then_same_address_valid_load_recovers_corner()
{
    TEST_START();

    const enum test_page low_page = SWITCH1;
    const enum test_page high_page = SWITCH2;
    const uintptr_t vaddr = ai_micro_m_memblock_cross_page_addr(low_page, 4U);
    const uintptr_t second_half_vaddr = vaddr + 4U;
    const uintptr_t low_phys = ai_micro_m_memblock_page_end_phys(low_page);
    const uintptr_t high_phys = phys_page_base(high_page);
    const uintptr_t high_deny_start = phys_page_base(high_page);
    const uintptr_t high_deny_end = high_deny_start + PAGE_SIZE;
    const uint64_t low_pattern = 0x0f1e2d3c4b5a6978ULL;
    const uint64_t high_pattern = 0x8877665544332211ULL;
    const uint64_t expected_value =
        ai_micro_m_memblock_expected_cross_page_4b4b_load(
            low_pattern, high_pattern);
    uint64_t value = 0;

    goto_priv(PRIV_M);
    ai_micro_m_memblock_reset_env();
    write64(low_phys, low_pattern);
    write64(high_phys, high_pattern);
    ai_micro_m_memblock_configure_pmp_window(
        high_deny_start, high_deny_end, AI_MICRO_MEMBLOCK_PMP_TOR_NO_R);

    ai_micro_m_memblock_prepare_supervisor_translation();
    ai_micro_m_memblock_set_leaf(low_page, PTE_V | PTE_AD | PTE_RWX);
    ai_micro_m_memblock_set_leaf(high_page, PTE_V | PTE_AD | PTE_RWX);
    sfence_vma();

    ai_micro_m_memblock_enable_mprv_supervisor_translation();
    TEST_SETUP_EXCEPT();
    ld(vaddr);
    TEST_ASSERT("the first translated split-load probe reports a high-half load access fault when only the upper page loses PMP read permission in the repeated-fault recovery corner",
        excpt.triggered == true &&
        excpt.cause == CAUSE_LAF &&
        excpt.tval == second_half_vaddr
    );

    ai_micro_m_memblock_enable_mprv_supervisor_translation();
    TEST_SETUP_EXCEPT();
    ld(vaddr);
    TEST_ASSERT("a second probe to the same translated split-load window must keep reporting the same newer high-half load access fault instead of reusing any stale older address",
        excpt.triggered == true &&
        excpt.cause == CAUSE_LAF &&
        excpt.tval == second_half_vaddr
    );

    goto_priv(PRIV_M);
    TEST_ASSERT("the repeated high-half load access-fault probes still leave both participating boundary doublewords unchanged before repair",
        read64(low_phys) == low_pattern &&
        read64(high_phys) == high_pattern
    );

    ai_micro_m_memblock_restore_pmp_full_access();

    ai_micro_m_memblock_enable_mprv_supervisor_translation();
    TEST_SETUP_EXCEPT();
    value = ld(vaddr);
    TEST_ASSERT("after restoring PMP read permission, the same translated split-load address must recover successfully even after the repeated high-half access faults",
        excpt.triggered == false &&
        value == expected_value
    );

    goto_priv(PRIV_M);
    TEST_ASSERT("the successful repeated-fault split-load recovery after the access faults still leaves both participating boundary doublewords unchanged",
        read64(low_phys) == low_pattern &&
        read64(high_phys) == high_pattern
    );

    ai_micro_m_memblock_reset_env();

    TEST_END();
}

bool ai_micro_m_store_cross_page_high_half_page_fault_repeated_then_same_address_valid_store_recovers_corner()
{
    TEST_START();

    const enum test_page low_page = SWITCH1;
    const enum test_page high_page = SWITCH2;
    const uintptr_t vaddr = ai_micro_m_memblock_cross_page_addr(low_page, 4U);
    const uintptr_t second_half_vaddr = vaddr + 4U;
    const uintptr_t low_phys = ai_micro_m_memblock_page_end_phys(low_page);
    const uintptr_t high_phys = phys_page_base(high_page);
    const uint64_t low_pattern = 0x1122334455667788ULL;
    const uint64_t high_pattern = 0x99aabbccddeeff00ULL;
    const uint64_t first_store_value = 0x0123456789abcdefULL;
    const uint64_t recovery_store_value = 0xfedcba9876543210ULL;
    const uint64_t expected_low_after_fault = low_pattern;
    const uint64_t expected_low_after_recovery =
        ai_micro_m_memblock_expected_low_after_4b_store(
            low_pattern, recovery_store_value);
    const uint64_t expected_high_after_recovery =
        ai_micro_m_memblock_expected_high_after_4b_store(
            high_pattern, recovery_store_value);

    goto_priv(PRIV_M);
    ai_micro_m_memblock_reset_env();
    write64(low_phys, low_pattern);
    write64(high_phys, high_pattern);

    ai_micro_m_memblock_prepare_supervisor_translation();
    ai_micro_m_memblock_set_leaf(low_page, PTE_V | PTE_AD | PTE_RWX);
    ai_micro_m_memblock_set_leaf(high_page, PTE_V | PTE_AD | PTE_RX);
    sfence_vma();

    ai_micro_m_memblock_enable_mprv_supervisor_translation();
    TEST_SETUP_EXCEPT();
    sd(vaddr, first_store_value);
    TEST_ASSERT("the first translated split-store probe reports a high-half store page fault when only the upper page loses write permission in the repeated-fault recovery corner",
        excpt.triggered == true &&
        excpt.cause == CAUSE_SPF &&
        excpt.tval == second_half_vaddr
    );

    ai_micro_m_memblock_enable_mprv_supervisor_translation();
    TEST_SETUP_EXCEPT();
    sd(vaddr, first_store_value);
    TEST_ASSERT("a second probe to the same translated split-store window must keep reporting the same newer high-half store page fault instead of reusing any stale older address",
        excpt.triggered == true &&
        excpt.cause == CAUSE_SPF &&
        excpt.tval == second_half_vaddr
    );

    goto_priv(PRIV_M);
    TEST_ASSERT("the repeated high-half store page-fault probes keep no low-half side effect visible before repair while the upper page remains unchanged",
        read64(low_phys) == expected_low_after_fault &&
        read64(high_phys) == high_pattern
    );

    ai_micro_m_memblock_set_leaf(high_page, PTE_V | PTE_AD | PTE_RWX);
    sfence_vma();

    ai_micro_m_memblock_enable_mprv_supervisor_translation();
    TEST_SETUP_EXCEPT();
    sd(vaddr, recovery_store_value);
    TEST_ASSERT("after repairing the upper-page permission, the same translated split-store address must recover successfully even after the repeated high-half page faults",
        excpt.triggered == false
    );

    goto_priv(PRIV_M);
    TEST_ASSERT("the successful repeated-fault split-store recovery must update both halves to the newer store value and overwrite the earlier side-effect-free fault episode",
        read64(low_phys) == expected_low_after_recovery &&
        read64(high_phys) == expected_high_after_recovery
    );

    ai_micro_m_memblock_reset_env();

    TEST_END();
}

bool ai_micro_m_store_cross_page_high_half_access_fault_repeated_then_same_address_valid_store_recovers_corner()
{
    TEST_START();

    const enum test_page low_page = SWITCH1;
    const enum test_page high_page = SWITCH2;
    const uintptr_t vaddr = ai_micro_m_memblock_cross_page_addr(low_page, 4U);
    const uintptr_t second_half_vaddr = vaddr + 4U;
    const uintptr_t low_phys = ai_micro_m_memblock_page_end_phys(low_page);
    const uintptr_t high_phys = phys_page_base(high_page);
    const uintptr_t high_deny_start = phys_page_base(high_page);
    const uintptr_t high_deny_end = high_deny_start + PAGE_SIZE;
    const uint64_t low_pattern = 0x1122334455667788ULL;
    const uint64_t high_pattern = 0x99aabbccddeeff00ULL;
    const uint64_t first_store_value = 0x0123456789abcdefULL;
    const uint64_t recovery_store_value = 0xfedcba9876543210ULL;
    const uint64_t expected_low_after_fault = low_pattern;
    const uint64_t expected_low_after_recovery =
        ai_micro_m_memblock_expected_low_after_4b_store(
            low_pattern, recovery_store_value);
    const uint64_t expected_high_after_recovery =
        ai_micro_m_memblock_expected_high_after_4b_store(
            high_pattern, recovery_store_value);

    goto_priv(PRIV_M);
    ai_micro_m_memblock_reset_env();
    write64(low_phys, low_pattern);
    write64(high_phys, high_pattern);
    ai_micro_m_memblock_configure_pmp_window(
        high_deny_start, high_deny_end, AI_MICRO_MEMBLOCK_PMP_TOR_NO_W);

    ai_micro_m_memblock_prepare_supervisor_translation();
    ai_micro_m_memblock_set_leaf(low_page, PTE_V | PTE_AD | PTE_RWX);
    ai_micro_m_memblock_set_leaf(high_page, PTE_V | PTE_AD | PTE_RWX);
    sfence_vma();

    ai_micro_m_memblock_enable_mprv_supervisor_translation();
    TEST_SETUP_EXCEPT();
    sd(vaddr, first_store_value);
    TEST_ASSERT("the first translated split-store probe reports a high-half store access fault when only the upper page loses PMP write permission in the repeated-fault recovery corner",
        excpt.triggered == true &&
        excpt.cause == CAUSE_SAF &&
        excpt.tval == second_half_vaddr
    );

    ai_micro_m_memblock_enable_mprv_supervisor_translation();
    TEST_SETUP_EXCEPT();
    sd(vaddr, first_store_value);
    TEST_ASSERT("a second probe to the same translated split-store window must keep reporting the same newer high-half store access fault instead of reusing any stale older address",
        excpt.triggered == true &&
        excpt.cause == CAUSE_SAF &&
        excpt.tval == second_half_vaddr
    );

    goto_priv(PRIV_M);
    TEST_ASSERT("the repeated high-half store access-fault probes keep no low-half side effect visible before repair while the upper page remains unchanged",
        read64(low_phys) == expected_low_after_fault &&
        read64(high_phys) == high_pattern
    );

    ai_micro_m_memblock_restore_pmp_full_access();

    ai_micro_m_memblock_enable_mprv_supervisor_translation();
    TEST_SETUP_EXCEPT();
    sd(vaddr, recovery_store_value);
    TEST_ASSERT("after restoring PMP write permission, the same translated split-store address must recover successfully even after the repeated high-half access faults",
        excpt.triggered == false
    );

    goto_priv(PRIV_M);
    TEST_ASSERT("the successful repeated-fault split-store recovery after the access faults must update both halves to the newer store value and overwrite the earlier side-effect-free fault episode",
        read64(low_phys) == expected_low_after_recovery &&
        read64(high_phys) == expected_high_after_recovery
    );

    ai_micro_m_memblock_reset_env();

    TEST_END();
}

bool ai_micro_m_single_step_exception_multi_source_same_cycle_oldest_must_win_diagnostic_corner()
{
    TEST_START();

    const enum test_page pf_page = SWITCH1;
    const enum test_page af_page = SWITCH2;
    const uintptr_t pf_vaddr = hs_page_base(pf_page) + 0x100U;
    const uintptr_t af_vaddr = hs_page_base(af_page) + 0x180U;
    const uintptr_t af_paddr = phys_page_base(af_page) + 0x180U;
    const uintptr_t marker_paddr = phys_page_base(SCRATCHPAD);
    struct ai_micro_m_memblock_exception_snapshot first = {0};

    goto_priv(PRIV_M);
    write64(af_paddr, 0x1122334455667788ULL);
    write64(marker_paddr, 0ULL);
    ai_micro_m_memblock_reset_env();
    ai_micro_m_memblock_prepare_supervisor_translation();
    ai_micro_m_memblock_set_leaf(pf_page, PTE_V | PTE_A | PTE_X);
    ai_micro_m_memblock_set_leaf(af_page, PTE_V | PTE_AD | PTE_RWX);
    sfence_vma();
    ai_micro_m_memblock_configure_pmp_page_window(
        af_paddr, AI_MICRO_MEMBLOCK_PMP_TOR_NO_W);

    TEST_SETUP_EXCEPT();
    ai_micro_m_memblock_translated_load_amo_fault_pair_capture(
        pf_vaddr, af_vaddr, marker_paddr, 0x0102030405060708ULL, &first);

    TEST_ASSERT("in the no-H LSQ-plus-AMO diagnostic pair with older translated load PF and younger translated AMO AF, the first observed precise trap remains the older load page fault",
        first.triggered == true &&
        first.cause == CAUSE_LPF &&
        first.tval == pf_vaddr
    );
    TEST_ASSERT("after the in-sequence rearm step, the younger translated AMO in that no-H diagnostic pair still reports the expected store access fault",
        excpt.triggered == true &&
        excpt.cause == CAUSE_SAF &&
        excpt.tval == af_vaddr
    );
    goto_priv(PRIV_M);
    TEST_ASSERT("the younger translated AMO leaves its target word untouched in the no-H older-load-first LSQ-plus-AMO diagnostic pair, and the trailing marker store executes only after both precise traps have been observed",
        read64(af_paddr) == 0x1122334455667788ULL &&
        read64(marker_paddr) == 1ULL
    );

    first = (struct ai_micro_m_memblock_exception_snapshot){0};

    goto_priv(PRIV_M);
    write64(af_paddr, 0x8899aabbccddeeffULL);
    write64(marker_paddr, 0ULL);
    ai_micro_m_memblock_reset_env();
    ai_micro_m_memblock_prepare_supervisor_translation();
    ai_micro_m_memblock_set_leaf(pf_page, PTE_V | PTE_A | PTE_R);
    ai_micro_m_memblock_set_leaf(af_page, PTE_V | PTE_AD | PTE_RWX);
    sfence_vma();
    ai_micro_m_memblock_configure_pmp_page_window(
        af_paddr, AI_MICRO_MEMBLOCK_PMP_TOR_NO_R);

    TEST_SETUP_EXCEPT();
    ai_micro_m_memblock_translated_amo_load_fault_pair_capture(
        pf_vaddr, af_vaddr, marker_paddr, 0x1111111122222222ULL, &first);

    TEST_ASSERT("in the reverse no-H LSQ-plus-AMO diagnostic pair with older translated AMO PF and younger translated load AF, the first observed precise trap remains the older AMO page fault",
        first.triggered == true &&
        first.cause == CAUSE_SPF &&
        first.tval == pf_vaddr
    );
    TEST_ASSERT("after the in-sequence rearm step, the younger translated load in the reverse no-H LSQ-plus-AMO diagnostic pair still reports the expected load access fault",
        excpt.triggered == true &&
        excpt.cause == CAUSE_LAF &&
        excpt.tval == af_vaddr
    );
    goto_priv(PRIV_M);
    TEST_ASSERT("the younger translated load leaves its backing word untouched in the reverse no-H LSQ-plus-AMO diagnostic pair, and the trailing marker store executes only after both precise traps have been observed",
        read64(af_paddr) == 0x8899aabbccddeeffULL &&
        read64(marker_paddr) == 1ULL
    );

    ai_micro_m_memblock_reset_env();

    TEST_END();
}

bool ai_micro_m_samepage_amo_fault_switch_amo_then_load_keeps_latest_cause_and_tval_corner()
{
    TEST_START();

    static const uintptr_t offsets[2] = { 0x180U, 0x1c0U };
    static const uint64_t target_patterns[2][2] = {
        { 0x1122334455667788ULL, 0x89abcdef01234567ULL },
        { 0xfedcba9876543210ULL, 0x13579bdf2468ace0ULL },
    };
    static const uint64_t guard_patterns[2] = {
        0x0f1e2d3c4b5a6978ULL,
        0x8877665544332211ULL,
    };
    static const uint64_t amo_masks[2][2] = {
        { 0x00ff0000ff0000ffULL, 0xff0000ff0000ff00ULL },
        { 0x1357000013570000ULL, 0x0000246800002468ULL },
    };
    const enum test_page page = SWITCH1;
    const uintptr_t page_phys = phys_page_base(page);
    const uintptr_t target_phys[2] = {
        page_phys + offsets[0],
        page_phys + offsets[1],
    };
    const uintptr_t guard_phys = page_phys + 0x200U;
    uint64_t observed_value = 0ULL;

    for (unsigned round = 0; round < 2U; ++round) {
        const unsigned first = round;
        const unsigned second = first ^ 1U;
        const uintptr_t amo_vaddr = hs_page_base(page) + offsets[first];
        const uintptr_t load_vaddr = hs_page_base(page) + offsets[second];

        goto_priv(PRIV_M);
        ai_micro_m_memblock_reset_env();
        write64(target_phys[0], target_patterns[0][round]);
        write64(target_phys[1], target_patterns[1][round]);
        write64(guard_phys, guard_patterns[round]);
        ai_micro_m_memblock_configure_pmp_page_window(
            target_phys[first], AI_MICRO_MEMBLOCK_PMP_TOR_NO_W);

        ai_micro_m_memblock_prepare_supervisor_translation();
        ai_micro_m_memblock_set_leaf(page, PTE_V | PTE_AD | PTE_RWX);
        sfence_vma();

        TEST_SETUP_EXCEPT();
        (void)ai_micro_m_memblock_translated_amoor_d(amo_vaddr,
            amo_masks[first][round]);
        TEST_ASSERT("the first no-H same-page shared exception probe comes from the translated atomic path and reports a store access fault with tval bound to the AMO address",
            excpt.triggered == true &&
            excpt.cause == CAUSE_SAF &&
            excpt.tval == amo_vaddr
        );

        goto_priv(PRIV_M);
        TEST_ASSERT("after the first no-H same-page translated atomic access fault, both target words and the nearby guard word remain unchanged before the follow-up translated load fault",
            read64(target_phys[0]) == target_patterns[0][round] &&
            read64(target_phys[1]) == target_patterns[1][round] &&
            read64(guard_phys) == guard_patterns[round]
        );

        ai_micro_m_memblock_configure_pmp_page_window(
            target_phys[second], AI_MICRO_MEMBLOCK_PMP_TOR_NO_R);

        TEST_SETUP_EXCEPT();
        observed_value = ai_micro_m_memblock_translated_scalar_load(
            false, sizeof(uint64_t), load_vaddr);
        TEST_ASSERT("after the earlier translated atomic fault has already occupied the shared MemExceptionBuffer path, the follow-up translated load fault must refresh cause and tval to the newer load-side access fault",
            excpt.triggered == true &&
            excpt.cause == CAUSE_LAF &&
            excpt.tval == load_vaddr
        );

        goto_priv(PRIV_M);
        TEST_ASSERT("the same-page translated atomic->load fault-switch sequence leaves both target words and the nearby guard word unchanged because neither fault should create visible memory side effects",
            read64(target_phys[0]) == target_patterns[0][round] &&
            read64(target_phys[1]) == target_patterns[1][round] &&
            read64(guard_phys) == guard_patterns[round]
        );
    }

    (void)observed_value;
    ai_micro_m_memblock_reset_env();

    TEST_END();
}

bool ai_micro_m_samepage_amo_repeated_access_fault_then_same_address_valid_atomic_recovers_corner()
{
    TEST_START();

    const enum test_page page = SWITCH1;
    const uintptr_t vaddr = hs_page_base(page) + 0x180U;
    const uintptr_t paddr = phys_page_base(page) + 0x180U;
    const uintptr_t guard_paddr = paddr + sizeof(uint64_t);
    const uint64_t initial_value = 0x1122334455667788ULL;
    const uint64_t guard_value = 0x89abcdef01234567ULL;
    const uint64_t amo_mask = 0x00ff0000ff0000ffULL;
    const uint64_t expected_value = initial_value | amo_mask;
    uint64_t old_value = 0ULL;

    goto_priv(PRIV_M);
    ai_micro_m_memblock_reset_env();
    write64(paddr, initial_value);
    write64(guard_paddr, guard_value);
    ai_micro_m_memblock_configure_pmp_page_window(
        paddr, AI_MICRO_MEMBLOCK_PMP_TOR_NO_W);

    ai_micro_m_memblock_prepare_supervisor_translation();
    ai_micro_m_memblock_set_leaf(page, PTE_V | PTE_AD | PTE_RWX);
    sfence_vma();

    TEST_SETUP_EXCEPT();
    (void)ai_micro_m_memblock_translated_amoor_d(vaddr, amo_mask);
    TEST_ASSERT("the first no-H same-page translated atomic in the repeated-fault recovery probe reports store access fault with tval bound to the original address",
        excpt.triggered == true &&
        excpt.cause == CAUSE_SAF &&
        excpt.tval == vaddr
    );

    TEST_SETUP_EXCEPT();
    (void)ai_micro_m_memblock_translated_amoor_d(vaddr, amo_mask);
    TEST_ASSERT("repeating the same no-H translated atomic while the deny window remains active still reports store access fault with the same address instead of reviving stale older metadata",
        excpt.triggered == true &&
        excpt.cause == CAUSE_SAF &&
        excpt.tval == vaddr
    );

    goto_priv(PRIV_M);
    TEST_ASSERT("before repairing the no-H translated atomic repeated-fault probe, both the target word and the adjacent guard word remain unchanged",
        read64(paddr) == initial_value &&
        read64(guard_paddr) == guard_value
    );

    ai_micro_m_memblock_restore_pmp_full_access();

    TEST_SETUP_EXCEPT();
    old_value = ai_micro_m_memblock_translated_amoor_d(vaddr, amo_mask);
    TEST_ASSERT("after restoring permission, the same-address no-H translated atomic succeeds immediately after the repeated fault episode and returns the original value",
        excpt.triggered == false &&
        old_value == initial_value
    );

    goto_priv(PRIV_M);
    TEST_ASSERT("the no-H translated atomic repeated-fault recovery probe updates only the target word while preserving the adjacent guard word",
        read64(paddr) == expected_value &&
        read64(guard_paddr) == guard_value
    );

    ai_micro_m_memblock_reset_env();

    TEST_END();
}

bool ai_micro_m_samepage_lr_repeated_access_fault_then_same_address_valid_lr_recovers_corner()
{
    TEST_START();

    const enum test_page page = SWITCH1;
    const uintptr_t vaddr = hs_page_base(page) + 0x1c0U;
    const uintptr_t paddr = phys_page_base(page) + 0x1c0U;
    const uintptr_t guard_paddr = paddr + sizeof(uint64_t);
    const uint64_t initial_value = 0xfedcba9876543210ULL;
    const uint64_t guard_value = 0x13579bdf2468ace0ULL;
    uint64_t observed_value = 0ULL;

    goto_priv(PRIV_M);
    ai_micro_m_memblock_reset_env();
    write64(paddr, initial_value);
    write64(guard_paddr, guard_value);
    ai_micro_m_memblock_configure_pmp_page_window(
        paddr, AI_MICRO_MEMBLOCK_PMP_TOR_NO_R);

    ai_micro_m_memblock_prepare_supervisor_translation();
    ai_micro_m_memblock_set_leaf(page, PTE_V | PTE_AD | PTE_RWX);
    sfence_vma();

    TEST_SETUP_EXCEPT();
    observed_value = ai_micro_m_memblock_translated_lr_d(vaddr);
    TEST_ASSERT("the first no-H same-page translated lr.d in the repeated-fault recovery probe reports load access fault with tval bound to the original address",
        excpt.triggered == true &&
        excpt.cause == CAUSE_LAF &&
        excpt.tval == vaddr
    );

    TEST_SETUP_EXCEPT();
    observed_value = ai_micro_m_memblock_translated_lr_d(vaddr);
    TEST_ASSERT("repeating the same no-H translated lr.d while the deny window remains active still reports load access fault with the same address instead of reviving stale older metadata",
        excpt.triggered == true &&
        excpt.cause == CAUSE_LAF &&
        excpt.tval == vaddr
    );

    goto_priv(PRIV_M);
    TEST_ASSERT("before repairing the no-H translated lr.d repeated-fault probe, both the target word and the adjacent guard word remain unchanged",
        read64(paddr) == initial_value &&
        read64(guard_paddr) == guard_value
    );

    ai_micro_m_memblock_restore_pmp_full_access();

    TEST_SETUP_EXCEPT();
    observed_value = ai_micro_m_memblock_translated_lr_d(vaddr);
    TEST_ASSERT("after restoring permission, the same-address no-H translated lr.d succeeds immediately after the repeated fault episode and returns the original value",
        excpt.triggered == false &&
        observed_value == initial_value
    );

    goto_priv(PRIV_M);
    TEST_ASSERT("the no-H translated lr.d repeated-fault recovery probe leaves both the target word and the adjacent guard word unchanged",
        read64(paddr) == initial_value &&
        read64(guard_paddr) == guard_value
    );

    ai_micro_m_memblock_reset_env();

    TEST_END();
}

bool ai_micro_m_samepage_amo_repair_then_different_address_fault_refreshes_new_tval_corner()
{
    TEST_START();

    const enum test_page page = SWITCH1;
    const uintptr_t first_offset = 0x180U;
    const uintptr_t second_offset = 0x1c0U;
    const uintptr_t first_vaddr = hs_page_base(page) + first_offset;
    const uintptr_t second_vaddr = hs_page_base(page) + second_offset;
    const uintptr_t first_paddr = phys_page_base(page) + first_offset;
    const uintptr_t second_paddr = phys_page_base(page) + second_offset;
    const uintptr_t guard_paddr = phys_page_base(page) + 0x200U;
    const uint64_t first_seed = 0x1122334455667788ULL;
    const uint64_t second_seed = 0x89abcdef01234567ULL;
    const uint64_t guard_seed = 0xfedcba9876543210ULL;
    const uint64_t amo_mask = 0x00ff0000ff0000ffULL;
    const uint64_t expected_first = first_seed | amo_mask;
    uint64_t observed_old = 0ULL;

    goto_priv(PRIV_M);
    ai_micro_m_memblock_reset_env();
    write64(first_paddr, first_seed);
    write64(second_paddr, second_seed);
    write64(guard_paddr, guard_seed);
    ai_micro_m_memblock_configure_pmp_page_window(
        first_paddr, AI_MICRO_MEMBLOCK_PMP_TOR_NO_W);

    ai_micro_m_memblock_prepare_supervisor_translation();
    ai_micro_m_memblock_set_leaf(page, PTE_V | PTE_AD | PTE_RWX);
    sfence_vma();

    TEST_SETUP_EXCEPT();
    (void)ai_micro_m_memblock_translated_amoor_d(first_vaddr, amo_mask);
    TEST_ASSERT("the first no-H translated atomic in the repair-then-new-fault probe reports store access fault with tval bound to the original address",
        excpt.triggered == true &&
        excpt.cause == CAUSE_SAF &&
        excpt.tval == first_vaddr
    );

    TEST_SETUP_EXCEPT();
    (void)ai_micro_m_memblock_translated_amoor_d(first_vaddr, amo_mask);
    TEST_ASSERT("repeating the same no-H translated atomic while the deny window remains active keeps reporting the same store access fault before the repair-clear probe",
        excpt.triggered == true &&
        excpt.cause == CAUSE_SAF &&
        excpt.tval == first_vaddr
    );

    goto_priv(PRIV_M);
    TEST_ASSERT("before the repeated-fault translated atomic is repaired, both data words and the guard word remain unchanged",
        read64(first_paddr) == first_seed &&
        read64(second_paddr) == second_seed &&
        read64(guard_paddr) == guard_seed
    );

    ai_micro_m_memblock_restore_pmp_full_access();

    TEST_SETUP_EXCEPT();
    observed_old = ai_micro_m_memblock_translated_amoor_d(first_vaddr, amo_mask);
    TEST_ASSERT("after restoring permission, the same-address translated atomic succeeds and returns the original value, proving the earlier fault episode has been repaired architecturally",
        excpt.triggered == false &&
        observed_old == first_seed
    );

    goto_priv(PRIV_M);
    TEST_ASSERT("after the repaired translated atomic commits, only the first target word changes while the second target and guard word remain untouched",
        read64(first_paddr) == expected_first &&
        read64(second_paddr) == second_seed &&
        read64(guard_paddr) == guard_seed
    );

    ai_micro_m_memblock_configure_pmp_page_window(
        second_paddr, AI_MICRO_MEMBLOCK_PMP_TOR_NO_W);

    TEST_SETUP_EXCEPT();
    (void)ai_micro_m_memblock_translated_amoor_d(second_vaddr, amo_mask);
    TEST_ASSERT("after the repaired no-H translated atomic has already completed on the first address, a new translated atomic fault on the second address must refresh tval to the newer address instead of reusing stale exception metadata from the first episode",
        excpt.triggered == true &&
        excpt.cause == CAUSE_SAF &&
        excpt.tval == second_vaddr
    );

    goto_priv(PRIV_M);
    TEST_ASSERT("the repair-then-new-fault translated atomic probe preserves the already-updated first word while keeping the second word and guard word unchanged under the younger denied atomic",
        read64(first_paddr) == expected_first &&
        read64(second_paddr) == second_seed &&
        read64(guard_paddr) == guard_seed
    );

    ai_micro_m_memblock_reset_env();

    TEST_END();
}

bool ai_micro_m_samepage_lr_repair_then_different_address_fault_refreshes_new_tval_corner()
{
    TEST_START();

    const enum test_page page = SWITCH1;
    const uintptr_t first_offset = 0x180U;
    const uintptr_t second_offset = 0x1c0U;
    const uintptr_t first_vaddr = hs_page_base(page) + first_offset;
    const uintptr_t second_vaddr = hs_page_base(page) + second_offset;
    const uintptr_t first_paddr = phys_page_base(page) + first_offset;
    const uintptr_t second_paddr = phys_page_base(page) + second_offset;
    const uintptr_t guard_paddr = phys_page_base(page) + 0x200U;
    const uint64_t first_seed = 0xfedcba9876543210ULL;
    const uint64_t second_seed = 0x13579bdf2468ace0ULL;
    const uint64_t guard_seed = 0x0123456789abcdefULL;
    uint64_t observed_value = 0ULL;

    goto_priv(PRIV_M);
    ai_micro_m_memblock_reset_env();
    write64(first_paddr, first_seed);
    write64(second_paddr, second_seed);
    write64(guard_paddr, guard_seed);
    ai_micro_m_memblock_configure_pmp_page_window(
        first_paddr, AI_MICRO_MEMBLOCK_PMP_TOR_NO_R);

    ai_micro_m_memblock_prepare_supervisor_translation();
    ai_micro_m_memblock_set_leaf(page, PTE_V | PTE_AD | PTE_RWX);
    sfence_vma();

    TEST_SETUP_EXCEPT();
    observed_value = ai_micro_m_memblock_translated_lr_d(first_vaddr);
    TEST_ASSERT("the first no-H translated lr.d in the repair-then-new-fault probe reports load access fault with tval bound to the original address",
        excpt.triggered == true &&
        excpt.cause == CAUSE_LAF &&
        excpt.tval == first_vaddr
    );

    TEST_SETUP_EXCEPT();
    observed_value = ai_micro_m_memblock_translated_lr_d(first_vaddr);
    TEST_ASSERT("repeating the same no-H translated lr.d while the deny window remains active keeps reporting the same load access fault before the repair-clear probe",
        excpt.triggered == true &&
        excpt.cause == CAUSE_LAF &&
        excpt.tval == first_vaddr
    );

    goto_priv(PRIV_M);
    TEST_ASSERT("before the repeated-fault translated lr.d is repaired, both data words and the guard word remain unchanged",
        read64(first_paddr) == first_seed &&
        read64(second_paddr) == second_seed &&
        read64(guard_paddr) == guard_seed
    );

    ai_micro_m_memblock_restore_pmp_full_access();

    TEST_SETUP_EXCEPT();
    observed_value = ai_micro_m_memblock_translated_lr_d(first_vaddr);
    TEST_ASSERT("after restoring permission, the same-address translated lr.d succeeds and returns the original value, proving the earlier fault episode has been repaired architecturally",
        excpt.triggered == false &&
        observed_value == first_seed
    );

    goto_priv(PRIV_M);
    TEST_ASSERT("the repaired translated lr.d remains read-only and leaves the first word, second word and guard word unchanged",
        read64(first_paddr) == first_seed &&
        read64(second_paddr) == second_seed &&
        read64(guard_paddr) == guard_seed
    );

    ai_micro_m_memblock_configure_pmp_page_window(
        second_paddr, AI_MICRO_MEMBLOCK_PMP_TOR_NO_R);

    TEST_SETUP_EXCEPT();
    observed_value = ai_micro_m_memblock_translated_lr_d(second_vaddr);
    TEST_ASSERT("after the repaired no-H translated lr.d has already completed on the first address, a new translated lr.d fault on the second address must refresh tval to the newer address instead of reusing stale exception metadata from the first episode",
        excpt.triggered == true &&
        excpt.cause == CAUSE_LAF &&
        excpt.tval == second_vaddr
    );

    goto_priv(PRIV_M);
    TEST_ASSERT("the repair-then-new-fault translated lr.d probe leaves both data words and the guard word unchanged under the younger denied load",
        read64(first_paddr) == first_seed &&
        read64(second_paddr) == second_seed &&
        read64(guard_paddr) == guard_seed
    );

    ai_micro_m_memblock_reset_env();

    TEST_END();
}

bool ai_micro_m_samepage_amo_repair_then_different_address_load_fault_refreshes_new_cause_tval_corner()
{
    TEST_START();

    const enum test_page page = SWITCH1;
    const uintptr_t first_offset = 0x180U;
    const uintptr_t second_offset = 0x1c0U;
    const uintptr_t first_vaddr = hs_page_base(page) + first_offset;
    const uintptr_t second_vaddr = hs_page_base(page) + second_offset;
    const uintptr_t first_paddr = phys_page_base(page) + first_offset;
    const uintptr_t second_paddr = phys_page_base(page) + second_offset;
    const uintptr_t guard_paddr = phys_page_base(page) + 0x200U;
    const uint64_t first_seed = 0x1122334455667788ULL;
    const uint64_t second_seed = 0x89abcdef01234567ULL;
    const uint64_t guard_seed = 0xfedcba9876543210ULL;
    const uint64_t amo_mask = 0x00ff0000ff0000ffULL;
    const uint64_t expected_first = first_seed | amo_mask;
    uint64_t observed_old = 0ULL;
    uint64_t observed_load = 0ULL;

    goto_priv(PRIV_M);
    ai_micro_m_memblock_reset_env();
    write64(first_paddr, first_seed);
    write64(second_paddr, second_seed);
    write64(guard_paddr, guard_seed);
    ai_micro_m_memblock_configure_pmp_page_window(
        first_paddr, AI_MICRO_MEMBLOCK_PMP_TOR_NO_W);

    ai_micro_m_memblock_prepare_supervisor_translation();
    ai_micro_m_memblock_set_leaf(page, PTE_V | PTE_AD | PTE_RWX);
    sfence_vma();

    TEST_SETUP_EXCEPT();
    (void)ai_micro_m_memblock_translated_amoor_d(first_vaddr, amo_mask);
    TEST_ASSERT("the first no-H translated atomic in the repair-then-cross-producer-fault probe reports store access fault with tval bound to the original address",
        excpt.triggered == true &&
        excpt.cause == CAUSE_SAF &&
        excpt.tval == first_vaddr
    );

    TEST_SETUP_EXCEPT();
    (void)ai_micro_m_memblock_translated_amoor_d(first_vaddr, amo_mask);
    TEST_ASSERT("repeating the same no-H translated atomic while the deny window remains active keeps reporting the same store access fault before the repair-clear cross-producer probe",
        excpt.triggered == true &&
        excpt.cause == CAUSE_SAF &&
        excpt.tval == first_vaddr
    );

    goto_priv(PRIV_M);
    TEST_ASSERT("before repairing the translated atomic repeated-fault episode, both data words and the guard word remain unchanged",
        read64(first_paddr) == first_seed &&
        read64(second_paddr) == second_seed &&
        read64(guard_paddr) == guard_seed
    );

    ai_micro_m_memblock_restore_pmp_full_access();

    TEST_SETUP_EXCEPT();
    observed_old = ai_micro_m_memblock_translated_amoor_d(first_vaddr, amo_mask);
    TEST_ASSERT("after restoring permission, the same-address translated atomic succeeds and returns the original value before the younger translated load fault is injected",
        excpt.triggered == false &&
        observed_old == first_seed
    );

    goto_priv(PRIV_M);
    TEST_ASSERT("after the repaired translated atomic commits, only the first target word changes while the second target and guard word remain untouched",
        read64(first_paddr) == expected_first &&
        read64(second_paddr) == second_seed &&
        read64(guard_paddr) == guard_seed
    );

    ai_micro_m_memblock_configure_pmp_page_window(
        second_paddr, AI_MICRO_MEMBLOCK_PMP_TOR_NO_R);

    TEST_SETUP_EXCEPT();
    observed_load = ai_micro_m_memblock_translated_scalar_load(
        false, sizeof(uint64_t), second_vaddr);
    TEST_ASSERT("after the repaired no-H translated atomic has already completed on the first address, a new translated load fault on the second address must refresh both cause and tval to the newer load-side fault instead of reusing stale atomic metadata",
        excpt.triggered == true &&
        excpt.cause == CAUSE_LAF &&
        excpt.tval == second_vaddr
    );

    goto_priv(PRIV_M);
    TEST_ASSERT("the repair-then-cross-producer-fault atomic->load probe preserves the already-updated first word while keeping the second word and guard word unchanged under the younger denied load",
        read64(first_paddr) == expected_first &&
        read64(second_paddr) == second_seed &&
        read64(guard_paddr) == guard_seed
    );

    (void)observed_load;
    ai_micro_m_memblock_reset_env();

    TEST_END();
}

bool ai_micro_m_samepage_lr_repair_then_different_address_atomic_fault_refreshes_new_cause_tval_corner()
{
    TEST_START();

    const enum test_page page = SWITCH1;
    const uintptr_t first_offset = 0x180U;
    const uintptr_t second_offset = 0x1c0U;
    const uintptr_t first_vaddr = hs_page_base(page) + first_offset;
    const uintptr_t second_vaddr = hs_page_base(page) + second_offset;
    const uintptr_t first_paddr = phys_page_base(page) + first_offset;
    const uintptr_t second_paddr = phys_page_base(page) + second_offset;
    const uintptr_t guard_paddr = phys_page_base(page) + 0x200U;
    const uint64_t first_seed = 0xfedcba9876543210ULL;
    const uint64_t second_seed = 0x13579bdf2468ace0ULL;
    const uint64_t guard_seed = 0x0123456789abcdefULL;
    const uint64_t amo_mask = 0x00ff0000ff0000ffULL;
    uint64_t observed_value = 0ULL;

    goto_priv(PRIV_M);
    ai_micro_m_memblock_reset_env();
    write64(first_paddr, first_seed);
    write64(second_paddr, second_seed);
    write64(guard_paddr, guard_seed);
    ai_micro_m_memblock_configure_pmp_page_window(
        first_paddr, AI_MICRO_MEMBLOCK_PMP_TOR_NO_R);

    ai_micro_m_memblock_prepare_supervisor_translation();
    ai_micro_m_memblock_set_leaf(page, PTE_V | PTE_AD | PTE_RWX);
    sfence_vma();

    TEST_SETUP_EXCEPT();
    observed_value = ai_micro_m_memblock_translated_lr_d(first_vaddr);
    TEST_ASSERT("the first no-H translated lr.d in the repair-then-cross-producer-fault probe reports load access fault with tval bound to the original address",
        excpt.triggered == true &&
        excpt.cause == CAUSE_LAF &&
        excpt.tval == first_vaddr
    );

    TEST_SETUP_EXCEPT();
    observed_value = ai_micro_m_memblock_translated_lr_d(first_vaddr);
    TEST_ASSERT("repeating the same no-H translated lr.d while the deny window remains active keeps reporting the same load access fault before the repair-clear cross-producer probe",
        excpt.triggered == true &&
        excpt.cause == CAUSE_LAF &&
        excpt.tval == first_vaddr
    );

    goto_priv(PRIV_M);
    TEST_ASSERT("before repairing the translated lr.d repeated-fault episode, both data words and the guard word remain unchanged",
        read64(first_paddr) == first_seed &&
        read64(second_paddr) == second_seed &&
        read64(guard_paddr) == guard_seed
    );

    ai_micro_m_memblock_restore_pmp_full_access();

    TEST_SETUP_EXCEPT();
    observed_value = ai_micro_m_memblock_translated_lr_d(first_vaddr);
    TEST_ASSERT("after restoring permission, the same-address translated lr.d succeeds and returns the original value before the younger translated atomic fault is injected",
        excpt.triggered == false &&
        observed_value == first_seed
    );

    goto_priv(PRIV_M);
    TEST_ASSERT("the repaired translated lr.d remains read-only and leaves the first word, second word and guard word unchanged",
        read64(first_paddr) == first_seed &&
        read64(second_paddr) == second_seed &&
        read64(guard_paddr) == guard_seed
    );

    ai_micro_m_memblock_configure_pmp_page_window(
        second_paddr, AI_MICRO_MEMBLOCK_PMP_TOR_NO_W);

    TEST_SETUP_EXCEPT();
    (void)ai_micro_m_memblock_translated_amoor_d(second_vaddr, amo_mask);
    TEST_ASSERT("after the repaired no-H translated lr.d has already completed on the first address, a new translated atomic fault on the second address must refresh both cause and tval to the newer atomic fault instead of reusing stale load-side metadata",
        excpt.triggered == true &&
        excpt.cause == CAUSE_SAF &&
        excpt.tval == second_vaddr
    );

    goto_priv(PRIV_M);
    TEST_ASSERT("the repair-then-cross-producer-fault lr.d->atomic probe leaves both data words and the guard word unchanged under the younger denied atomic",
        read64(first_paddr) == first_seed &&
        read64(second_paddr) == second_seed &&
        read64(guard_paddr) == guard_seed
    );

    ai_micro_m_memblock_reset_env();

    TEST_END();
}

bool ai_micro_m_samepage_amo_fault_switch_load_then_amo_keeps_latest_cause_and_tval_corner()
{
    TEST_START();

    static const uintptr_t offsets[2] = { 0x180U, 0x1c0U };
    static const uint64_t target_patterns[2][2] = {
        { 0x0123456789abcdefULL, 0xf0e1d2c3b4a59687ULL },
        { 0x7766554433221100ULL, 0x21436587a9cbed0fULL },
    };
    static const uint64_t guard_patterns[2] = {
        0xaabbccddeeff0011ULL,
        0x1021324354657687ULL,
    };
    static const uint64_t amo_masks[2][2] = {
        { 0x00ff00ff00ff00ffULL, 0xff00ff00ff00ff00ULL },
        { 0x3333000033330000ULL, 0x0000555500005555ULL },
    };
    const enum test_page page = SWITCH1;
    const uintptr_t page_phys = phys_page_base(page);
    const uintptr_t target_phys[2] = {
        page_phys + offsets[0],
        page_phys + offsets[1],
    };
    const uintptr_t guard_phys = page_phys + 0x200U;
    uint64_t observed_value = 0ULL;

    for (unsigned round = 0; round < 2U; ++round) {
        const unsigned first = round;
        const unsigned second = first ^ 1U;
        const uintptr_t load_vaddr = hs_page_base(page) + offsets[first];
        const uintptr_t amo_vaddr = hs_page_base(page) + offsets[second];

        goto_priv(PRIV_M);
        ai_micro_m_memblock_reset_env();
        write64(target_phys[0], target_patterns[0][round]);
        write64(target_phys[1], target_patterns[1][round]);
        write64(guard_phys, guard_patterns[round]);
        ai_micro_m_memblock_configure_pmp_page_window(
            target_phys[first], AI_MICRO_MEMBLOCK_PMP_TOR_NO_R);

        ai_micro_m_memblock_prepare_supervisor_translation();
        ai_micro_m_memblock_set_leaf(page, PTE_V | PTE_AD | PTE_RWX);
        sfence_vma();

        TEST_SETUP_EXCEPT();
        observed_value = ai_micro_m_memblock_translated_scalar_load(
            false, sizeof(uint64_t), load_vaddr);
        TEST_ASSERT("the first no-H same-page shared exception probe comes from the translated load path and reports a load access fault with tval bound to the load address",
            excpt.triggered == true &&
            excpt.cause == CAUSE_LAF &&
            excpt.tval == load_vaddr
        );

        goto_priv(PRIV_M);
        TEST_ASSERT("after the first no-H same-page translated load access fault, both target words and the nearby guard word remain unchanged before the follow-up translated atomic fault",
            read64(target_phys[0]) == target_patterns[0][round] &&
            read64(target_phys[1]) == target_patterns[1][round] &&
            read64(guard_phys) == guard_patterns[round]
        );

        ai_micro_m_memblock_configure_pmp_page_window(
            target_phys[second], AI_MICRO_MEMBLOCK_PMP_TOR_NO_W);

        TEST_SETUP_EXCEPT();
        (void)ai_micro_m_memblock_translated_amoor_d(amo_vaddr,
            amo_masks[second][round]);
        TEST_ASSERT("after the earlier translated load fault has already occupied the shared MemExceptionBuffer path, the follow-up translated atomic fault must refresh cause and tval to the newer AMO-side access fault",
            excpt.triggered == true &&
            excpt.cause == CAUSE_SAF &&
            excpt.tval == amo_vaddr
        );

        goto_priv(PRIV_M);
        TEST_ASSERT("the same-page translated load->atomic fault-switch sequence leaves both target words and the nearby guard word unchanged because neither fault should create visible memory side effects",
            read64(target_phys[0]) == target_patterns[0][round] &&
            read64(target_phys[1]) == target_patterns[1][round] &&
            read64(guard_phys) == guard_patterns[round]
        );
    }

    (void)observed_value;
    ai_micro_m_memblock_reset_env();

    TEST_END();
}

bool ai_micro_m_samepage_amo_fault_switch_amo_then_store_keeps_latest_tval_corner()
{
    TEST_START();

    static const uintptr_t offsets[2] = { 0x180U, 0x1c0U };
    static const uint64_t target_patterns[2][2] = {
        { 0x89abcdef01234567ULL, 0x1122334455667788ULL },
        { 0xf0e1d2c3b4a59687ULL, 0x7766554433221100ULL },
    };
    static const uint64_t guard_patterns[2] = {
        0x0f1e2d3c4b5a6978ULL,
        0x8877665544332211ULL,
    };
    static const uint64_t amo_masks[2][2] = {
        { 0x00ff0000ff0000ffULL, 0xff0000ff0000ff00ULL },
        { 0x1357000013570000ULL, 0x0000246800002468ULL },
    };
    static const uint64_t store_values[2][2] = {
        { 0x0123456789abcdefULL, 0xfedcba9876543210ULL },
        { 0x0f1e2d3c4b5a6978ULL, 0x8877665544332211ULL },
    };
    const enum test_page page = SWITCH1;
    const uintptr_t page_phys = phys_page_base(page);
    const uintptr_t target_phys[2] = {
        page_phys + offsets[0],
        page_phys + offsets[1],
    };
    const uintptr_t guard_phys = page_phys + 0x200U;

    for (unsigned round = 0; round < 2U; ++round) {
        const unsigned first = round;
        const unsigned second = first ^ 1U;
        const uintptr_t amo_vaddr = hs_page_base(page) + offsets[first];
        const uintptr_t store_vaddr = hs_page_base(page) + offsets[second];

        goto_priv(PRIV_M);
        ai_micro_m_memblock_reset_env();
        write64(target_phys[0], target_patterns[0][round]);
        write64(target_phys[1], target_patterns[1][round]);
        write64(guard_phys, guard_patterns[round]);
        ai_micro_m_memblock_configure_pmp_page_window(
            target_phys[first], AI_MICRO_MEMBLOCK_PMP_TOR_NO_W);

        ai_micro_m_memblock_prepare_supervisor_translation();
        ai_micro_m_memblock_set_leaf(page, PTE_V | PTE_AD | PTE_RWX);
        sfence_vma();

        TEST_SETUP_EXCEPT();
        (void)ai_micro_m_memblock_translated_amoor_d(amo_vaddr,
            amo_masks[first][round]);
        TEST_ASSERT("the first no-H same-page shared store-side exception probe comes from the translated atomic path and reports store access fault with tval bound to the AMO address",
            excpt.triggered == true &&
            excpt.cause == CAUSE_SAF &&
            excpt.tval == amo_vaddr
        );

        goto_priv(PRIV_M);
        TEST_ASSERT("after the first no-H same-page translated atomic store fault, both target words and the nearby guard word remain unchanged before the follow-up translated store fault",
            read64(target_phys[0]) == target_patterns[0][round] &&
            read64(target_phys[1]) == target_patterns[1][round] &&
            read64(guard_phys) == guard_patterns[round]
        );

        ai_micro_m_memblock_configure_pmp_page_window(
            target_phys[second], AI_MICRO_MEMBLOCK_PMP_TOR_NO_W);

        TEST_SETUP_EXCEPT();
        ai_micro_m_memblock_translated_sd(store_vaddr, store_values[second][round]);
        TEST_ASSERT("after the earlier translated atomic fault has already occupied the shared MemExceptionBuffer path, the follow-up translated store fault must refresh tval to the newer store-side address even though the cause class remains store access fault",
            excpt.triggered == true &&
            excpt.cause == CAUSE_SAF &&
            excpt.tval == store_vaddr
        );

        goto_priv(PRIV_M);
        TEST_ASSERT("the same-page translated atomic->store fault-switch sequence leaves both target words and the nearby guard word unchanged because both store-side faults are side-effect free",
            read64(target_phys[0]) == target_patterns[0][round] &&
            read64(target_phys[1]) == target_patterns[1][round] &&
            read64(guard_phys) == guard_patterns[round]
        );
    }

    ai_micro_m_memblock_reset_env();

    TEST_END();
}

bool ai_micro_m_samepage_amo_fault_switch_store_then_amo_keeps_latest_tval_corner()
{
    TEST_START();

    static const uintptr_t offsets[2] = { 0x180U, 0x1c0U };
    static const uint64_t target_patterns[2][2] = {
        { 0x0123456789abcdefULL, 0x7766554433221100ULL },
        { 0xf0e1d2c3b4a59687ULL, 0x21436587a9cbed0fULL },
    };
    static const uint64_t guard_patterns[2] = {
        0xaabbccddeeff0011ULL,
        0x1021324354657687ULL,
    };
    static const uint64_t amo_masks[2][2] = {
        { 0x00ff00ff00ff00ffULL, 0xff00ff00ff00ff00ULL },
        { 0x3333000033330000ULL, 0x0000555500005555ULL },
    };
    static const uint64_t store_values[2][2] = {
        { 0x89abcdef01234567ULL, 0xfedcba9876543210ULL },
        { 0x0f1e2d3c4b5a6978ULL, 0x8877665544332211ULL },
    };
    const enum test_page page = SWITCH1;
    const uintptr_t page_phys = phys_page_base(page);
    const uintptr_t target_phys[2] = {
        page_phys + offsets[0],
        page_phys + offsets[1],
    };
    const uintptr_t guard_phys = page_phys + 0x200U;

    for (unsigned round = 0; round < 2U; ++round) {
        const unsigned first = round;
        const unsigned second = first ^ 1U;
        const uintptr_t store_vaddr = hs_page_base(page) + offsets[first];
        const uintptr_t amo_vaddr = hs_page_base(page) + offsets[second];

        goto_priv(PRIV_M);
        ai_micro_m_memblock_reset_env();
        write64(target_phys[0], target_patterns[0][round]);
        write64(target_phys[1], target_patterns[1][round]);
        write64(guard_phys, guard_patterns[round]);
        ai_micro_m_memblock_configure_pmp_page_window(
            target_phys[first], AI_MICRO_MEMBLOCK_PMP_TOR_NO_W);

        ai_micro_m_memblock_prepare_supervisor_translation();
        ai_micro_m_memblock_set_leaf(page, PTE_V | PTE_AD | PTE_RWX);
        sfence_vma();

        TEST_SETUP_EXCEPT();
        ai_micro_m_memblock_translated_sd(store_vaddr, store_values[first][round]);
        TEST_ASSERT("the first no-H same-page shared store-side exception probe comes from the translated scalar store path and reports store access fault with tval bound to the store address",
            excpt.triggered == true &&
            excpt.cause == CAUSE_SAF &&
            excpt.tval == store_vaddr
        );

        goto_priv(PRIV_M);
        TEST_ASSERT("after the first no-H same-page translated store access fault, both target words and the nearby guard word remain unchanged before the follow-up translated atomic fault",
            read64(target_phys[0]) == target_patterns[0][round] &&
            read64(target_phys[1]) == target_patterns[1][round] &&
            read64(guard_phys) == guard_patterns[round]
        );

        ai_micro_m_memblock_configure_pmp_page_window(
            target_phys[second], AI_MICRO_MEMBLOCK_PMP_TOR_NO_W);

        TEST_SETUP_EXCEPT();
        (void)ai_micro_m_memblock_translated_amoor_d(amo_vaddr,
            amo_masks[second][round]);
        TEST_ASSERT("after the earlier translated store fault has already occupied the shared MemExceptionBuffer path, the follow-up translated atomic fault must refresh tval to the newer AMO-side address even though both producers report store access fault",
            excpt.triggered == true &&
            excpt.cause == CAUSE_SAF &&
            excpt.tval == amo_vaddr
        );

        goto_priv(PRIV_M);
        TEST_ASSERT("the same-page translated store->atomic fault-switch sequence leaves both target words and the nearby guard word unchanged because both store-side faults are side-effect free",
            read64(target_phys[0]) == target_patterns[0][round] &&
            read64(target_phys[1]) == target_patterns[1][round] &&
            read64(guard_phys) == guard_patterns[round]
        );
    }

    ai_micro_m_memblock_reset_env();

    TEST_END();
}

bool ai_micro_m_samepage_lr_fault_switch_lr_then_load_keeps_latest_tval_corner()
{
    TEST_START();

    static const uintptr_t offsets[2] = { 0x180U, 0x1c0U };
    static const uint64_t target_patterns[2][2] = {
        { 0x1122334455667788ULL, 0x89abcdef01234567ULL },
        { 0xfedcba9876543210ULL, 0x13579bdf2468ace0ULL },
    };
    static const uint64_t guard_patterns[2] = {
        0x0f1e2d3c4b5a6978ULL,
        0x8877665544332211ULL,
    };
    const enum test_page page = SWITCH1;
    const uintptr_t page_phys = phys_page_base(page);
    const uintptr_t target_phys[2] = {
        page_phys + offsets[0],
        page_phys + offsets[1],
    };
    const uintptr_t guard_phys = page_phys + 0x200U;
    uint64_t observed_value = 0ULL;

    for (unsigned round = 0; round < 2U; ++round) {
        const unsigned first = round;
        const unsigned second = first ^ 1U;
        const uintptr_t lr_vaddr = hs_page_base(page) + offsets[first];
        const uintptr_t load_vaddr = hs_page_base(page) + offsets[second];

        goto_priv(PRIV_M);
        ai_micro_m_memblock_reset_env();
        write64(target_phys[0], target_patterns[0][round]);
        write64(target_phys[1], target_patterns[1][round]);
        write64(guard_phys, guard_patterns[round]);
        ai_micro_m_memblock_configure_pmp_page_window(
            target_phys[first], AI_MICRO_MEMBLOCK_PMP_TOR_NO_R);

        ai_micro_m_memblock_prepare_supervisor_translation();
        ai_micro_m_memblock_set_leaf(page, PTE_V | PTE_AD | PTE_RWX);
        sfence_vma();

        TEST_SETUP_EXCEPT();
        observed_value = ai_micro_m_memblock_translated_lr_d(lr_vaddr);
        TEST_ASSERT("the first no-H same-page shared load-side exception probe comes from the translated lr.d path and reports load access fault with tval bound to the LR address",
            excpt.triggered == true &&
            excpt.cause == CAUSE_LAF &&
            excpt.tval == lr_vaddr
        );

        goto_priv(PRIV_M);
        TEST_ASSERT("after the first no-H same-page translated lr.d access fault, both target words and the nearby guard word remain unchanged before the follow-up translated load fault",
            read64(target_phys[0]) == target_patterns[0][round] &&
            read64(target_phys[1]) == target_patterns[1][round] &&
            read64(guard_phys) == guard_patterns[round]
        );

        ai_micro_m_memblock_configure_pmp_page_window(
            target_phys[second], AI_MICRO_MEMBLOCK_PMP_TOR_NO_R);

        TEST_SETUP_EXCEPT();
        observed_value = ai_micro_m_memblock_translated_scalar_load(
            false, sizeof(uint64_t), load_vaddr);
        TEST_ASSERT("after the earlier translated lr.d fault has already occupied the shared MemExceptionBuffer path, the follow-up translated load fault must refresh tval to the newer load-side address even though both producers report load access fault",
            excpt.triggered == true &&
            excpt.cause == CAUSE_LAF &&
            excpt.tval == load_vaddr
        );

        goto_priv(PRIV_M);
        TEST_ASSERT("the same-page translated lr.d->load fault-switch sequence leaves both target words and the nearby guard word unchanged because both load-side faults are side-effect free",
            read64(target_phys[0]) == target_patterns[0][round] &&
            read64(target_phys[1]) == target_patterns[1][round] &&
            read64(guard_phys) == guard_patterns[round]
        );
    }

    (void)observed_value;
    ai_micro_m_memblock_reset_env();

    TEST_END();
}

bool ai_micro_m_samepage_lr_fault_switch_load_then_lr_keeps_latest_tval_corner()
{
    TEST_START();

    static const uintptr_t offsets[2] = { 0x180U, 0x1c0U };
    static const uint64_t target_patterns[2][2] = {
        { 0x0123456789abcdefULL, 0x7766554433221100ULL },
        { 0xf0e1d2c3b4a59687ULL, 0x21436587a9cbed0fULL },
    };
    static const uint64_t guard_patterns[2] = {
        0xaabbccddeeff0011ULL,
        0x1021324354657687ULL,
    };
    const enum test_page page = SWITCH1;
    const uintptr_t page_phys = phys_page_base(page);
    const uintptr_t target_phys[2] = {
        page_phys + offsets[0],
        page_phys + offsets[1],
    };
    const uintptr_t guard_phys = page_phys + 0x200U;
    uint64_t observed_value = 0ULL;

    for (unsigned round = 0; round < 2U; ++round) {
        const unsigned first = round;
        const unsigned second = first ^ 1U;
        const uintptr_t load_vaddr = hs_page_base(page) + offsets[first];
        const uintptr_t lr_vaddr = hs_page_base(page) + offsets[second];

        goto_priv(PRIV_M);
        ai_micro_m_memblock_reset_env();
        write64(target_phys[0], target_patterns[0][round]);
        write64(target_phys[1], target_patterns[1][round]);
        write64(guard_phys, guard_patterns[round]);
        ai_micro_m_memblock_configure_pmp_page_window(
            target_phys[first], AI_MICRO_MEMBLOCK_PMP_TOR_NO_R);

        ai_micro_m_memblock_prepare_supervisor_translation();
        ai_micro_m_memblock_set_leaf(page, PTE_V | PTE_AD | PTE_RWX);
        sfence_vma();

        TEST_SETUP_EXCEPT();
        observed_value = ai_micro_m_memblock_translated_scalar_load(
            false, sizeof(uint64_t), load_vaddr);
        TEST_ASSERT("the first no-H same-page shared load-side exception probe comes from the translated load path and reports load access fault with tval bound to the load address",
            excpt.triggered == true &&
            excpt.cause == CAUSE_LAF &&
            excpt.tval == load_vaddr
        );

        goto_priv(PRIV_M);
        TEST_ASSERT("after the first no-H same-page translated load access fault, both target words and the nearby guard word remain unchanged before the follow-up translated lr.d fault",
            read64(target_phys[0]) == target_patterns[0][round] &&
            read64(target_phys[1]) == target_patterns[1][round] &&
            read64(guard_phys) == guard_patterns[round]
        );

        ai_micro_m_memblock_configure_pmp_page_window(
            target_phys[second], AI_MICRO_MEMBLOCK_PMP_TOR_NO_R);

        TEST_SETUP_EXCEPT();
        observed_value = ai_micro_m_memblock_translated_lr_d(lr_vaddr);
        TEST_ASSERT("after the earlier translated load fault has already occupied the shared MemExceptionBuffer path, the follow-up translated lr.d fault must refresh tval to the newer LR address even though both producers report load access fault",
            excpt.triggered == true &&
            excpt.cause == CAUSE_LAF &&
            excpt.tval == lr_vaddr
        );

        goto_priv(PRIV_M);
        TEST_ASSERT("the same-page translated load->lr.d fault-switch sequence leaves both target words and the nearby guard word unchanged because both load-side faults are side-effect free",
            read64(target_phys[0]) == target_patterns[0][round] &&
            read64(target_phys[1]) == target_patterns[1][round] &&
            read64(guard_phys) == guard_patterns[round]
        );
    }

    (void)observed_value;
    ai_micro_m_memblock_reset_env();

    TEST_END();
}

bool ai_micro_m_samepage_lr_fault_switch_lr_then_store_keeps_latest_cause_and_tval_corner()
{
    TEST_START();

    static const uintptr_t offsets[2] = { 0x180U, 0x1c0U };
    static const uint64_t target_patterns[2][2] = {
        { 0x89abcdef01234567ULL, 0x1122334455667788ULL },
        { 0xf0e1d2c3b4a59687ULL, 0x7766554433221100ULL },
    };
    static const uint64_t guard_patterns[2] = {
        0x0f1e2d3c4b5a6978ULL,
        0x8877665544332211ULL,
    };
    static const uint64_t store_values[2][2] = {
        { 0x0123456789abcdefULL, 0xfedcba9876543210ULL },
        { 0x0f1e2d3c4b5a6978ULL, 0x8877665544332211ULL },
    };
    const enum test_page page = SWITCH1;
    const uintptr_t page_phys = phys_page_base(page);
    const uintptr_t target_phys[2] = {
        page_phys + offsets[0],
        page_phys + offsets[1],
    };
    const uintptr_t guard_phys = page_phys + 0x200U;
    uint64_t observed_value = 0ULL;

    for (unsigned round = 0; round < 2U; ++round) {
        const unsigned first = round;
        const unsigned second = first ^ 1U;
        const uintptr_t lr_vaddr = hs_page_base(page) + offsets[first];
        const uintptr_t store_vaddr = hs_page_base(page) + offsets[second];

        goto_priv(PRIV_M);
        ai_micro_m_memblock_reset_env();
        write64(target_phys[0], target_patterns[0][round]);
        write64(target_phys[1], target_patterns[1][round]);
        write64(guard_phys, guard_patterns[round]);
        ai_micro_m_memblock_configure_pmp_page_window(
            target_phys[first], AI_MICRO_MEMBLOCK_PMP_TOR_NO_R);

        ai_micro_m_memblock_prepare_supervisor_translation();
        ai_micro_m_memblock_set_leaf(page, PTE_V | PTE_AD | PTE_RWX);
        sfence_vma();

        TEST_SETUP_EXCEPT();
        observed_value = ai_micro_m_memblock_translated_lr_d(lr_vaddr);
        TEST_ASSERT("the first no-H same-page shared exception probe comes from the translated lr.d path and reports load access fault with tval bound to the LR address",
            excpt.triggered == true &&
            excpt.cause == CAUSE_LAF &&
            excpt.tval == lr_vaddr
        );

        goto_priv(PRIV_M);
        TEST_ASSERT("after the first no-H same-page translated lr.d access fault, both target words and the nearby guard word remain unchanged before the follow-up translated store fault",
            read64(target_phys[0]) == target_patterns[0][round] &&
            read64(target_phys[1]) == target_patterns[1][round] &&
            read64(guard_phys) == guard_patterns[round]
        );

        ai_micro_m_memblock_configure_pmp_page_window(
            target_phys[second], AI_MICRO_MEMBLOCK_PMP_TOR_NO_W);

        TEST_SETUP_EXCEPT();
        ai_micro_m_memblock_translated_sd(store_vaddr, store_values[second][round]);
        TEST_ASSERT("after the earlier translated lr.d fault has already occupied the shared MemExceptionBuffer path, the follow-up translated store fault must refresh both cause and tval to the newer store-side address",
            excpt.triggered == true &&
            excpt.cause == CAUSE_SAF &&
            excpt.tval == store_vaddr
        );

        goto_priv(PRIV_M);
        TEST_ASSERT("the same-page translated lr.d->store fault-switch sequence leaves both target words and the nearby guard word unchanged because both faults are side-effect free",
            read64(target_phys[0]) == target_patterns[0][round] &&
            read64(target_phys[1]) == target_patterns[1][round] &&
            read64(guard_phys) == guard_patterns[round]
        );
    }

    (void)observed_value;
    ai_micro_m_memblock_reset_env();

    TEST_END();
}

bool ai_micro_m_samepage_lr_fault_switch_store_then_lr_keeps_latest_cause_and_tval_corner()
{
    TEST_START();

    static const uintptr_t offsets[2] = { 0x180U, 0x1c0U };
    static const uint64_t target_patterns[2][2] = {
        { 0x0123456789abcdefULL, 0x7766554433221100ULL },
        { 0xf0e1d2c3b4a59687ULL, 0x21436587a9cbed0fULL },
    };
    static const uint64_t guard_patterns[2] = {
        0xaabbccddeeff0011ULL,
        0x1021324354657687ULL,
    };
    static const uint64_t store_values[2][2] = {
        { 0x89abcdef01234567ULL, 0xfedcba9876543210ULL },
        { 0x0f1e2d3c4b5a6978ULL, 0x8877665544332211ULL },
    };
    const enum test_page page = SWITCH1;
    const uintptr_t page_phys = phys_page_base(page);
    const uintptr_t target_phys[2] = {
        page_phys + offsets[0],
        page_phys + offsets[1],
    };
    const uintptr_t guard_phys = page_phys + 0x200U;
    uint64_t observed_value = 0ULL;

    for (unsigned round = 0; round < 2U; ++round) {
        const unsigned first = round;
        const unsigned second = first ^ 1U;
        const uintptr_t store_vaddr = hs_page_base(page) + offsets[first];
        const uintptr_t lr_vaddr = hs_page_base(page) + offsets[second];

        goto_priv(PRIV_M);
        ai_micro_m_memblock_reset_env();
        write64(target_phys[0], target_patterns[0][round]);
        write64(target_phys[1], target_patterns[1][round]);
        write64(guard_phys, guard_patterns[round]);
        ai_micro_m_memblock_configure_pmp_page_window(
            target_phys[first], AI_MICRO_MEMBLOCK_PMP_TOR_NO_W);

        ai_micro_m_memblock_prepare_supervisor_translation();
        ai_micro_m_memblock_set_leaf(page, PTE_V | PTE_AD | PTE_RWX);
        sfence_vma();

        TEST_SETUP_EXCEPT();
        ai_micro_m_memblock_translated_sd(store_vaddr, store_values[first][round]);
        TEST_ASSERT("the first no-H same-page shared exception probe comes from the translated store path and reports store access fault with tval bound to the store address",
            excpt.triggered == true &&
            excpt.cause == CAUSE_SAF &&
            excpt.tval == store_vaddr
        );

        goto_priv(PRIV_M);
        TEST_ASSERT("after the first no-H same-page translated store access fault, both target words and the nearby guard word remain unchanged before the follow-up translated lr.d fault",
            read64(target_phys[0]) == target_patterns[0][round] &&
            read64(target_phys[1]) == target_patterns[1][round] &&
            read64(guard_phys) == guard_patterns[round]
        );

        ai_micro_m_memblock_configure_pmp_page_window(
            target_phys[second], AI_MICRO_MEMBLOCK_PMP_TOR_NO_R);

        TEST_SETUP_EXCEPT();
        observed_value = ai_micro_m_memblock_translated_lr_d(lr_vaddr);
        TEST_ASSERT("after the earlier translated store fault has already occupied the shared MemExceptionBuffer path, the follow-up translated lr.d fault must refresh both cause and tval to the newer LR-side address",
            excpt.triggered == true &&
            excpt.cause == CAUSE_LAF &&
            excpt.tval == lr_vaddr
        );

        goto_priv(PRIV_M);
        TEST_ASSERT("the same-page translated store->lr.d fault-switch sequence leaves both target words and the nearby guard word unchanged because both faults are side-effect free",
            read64(target_phys[0]) == target_patterns[0][round] &&
            read64(target_phys[1]) == target_patterns[1][round] &&
            read64(guard_phys) == guard_patterns[round]
        );
    }

    (void)observed_value;
    ai_micro_m_memblock_reset_env();

    TEST_END();
}

bool ai_micro_m_samepage_lr_fault_switch_lr_then_amo_keeps_latest_cause_and_tval_corner()
{
    TEST_START();

    static const uintptr_t offsets[2] = { 0x180U, 0x1c0U };
    static const uint64_t target_patterns[2][2] = {
        { 0x1122334455667788ULL, 0x89abcdef01234567ULL },
        { 0xfedcba9876543210ULL, 0x13579bdf2468ace0ULL },
    };
    static const uint64_t guard_patterns[2] = {
        0x0f1e2d3c4b5a6978ULL,
        0x8877665544332211ULL,
    };
    static const uint64_t amo_masks[2][2] = {
        { 0x00ff0000ff0000ffULL, 0xff0000ff0000ff00ULL },
        { 0x1357000013570000ULL, 0x0000246800002468ULL },
    };
    const enum test_page page = SWITCH1;
    const uintptr_t page_phys = phys_page_base(page);
    const uintptr_t target_phys[2] = {
        page_phys + offsets[0],
        page_phys + offsets[1],
    };
    const uintptr_t guard_phys = page_phys + 0x200U;
    uint64_t observed_value = 0ULL;

    for (unsigned round = 0; round < 2U; ++round) {
        const unsigned first = round;
        const unsigned second = first ^ 1U;
        const uintptr_t lr_vaddr = hs_page_base(page) + offsets[first];
        const uintptr_t amo_vaddr = hs_page_base(page) + offsets[second];

        goto_priv(PRIV_M);
        ai_micro_m_memblock_reset_env();
        write64(target_phys[0], target_patterns[0][round]);
        write64(target_phys[1], target_patterns[1][round]);
        write64(guard_phys, guard_patterns[round]);
        ai_micro_m_memblock_configure_pmp_page_window(
            target_phys[first], AI_MICRO_MEMBLOCK_PMP_TOR_NO_R);

        ai_micro_m_memblock_prepare_supervisor_translation();
        ai_micro_m_memblock_set_leaf(page, PTE_V | PTE_AD | PTE_RWX);
        sfence_vma();

        TEST_SETUP_EXCEPT();
        observed_value = ai_micro_m_memblock_translated_lr_d(lr_vaddr);
        TEST_ASSERT("the first no-H same-page shared single-port probe comes from the translated lr.d path and reports load access fault with tval bound to the LR address",
            excpt.triggered == true &&
            excpt.cause == CAUSE_LAF &&
            excpt.tval == lr_vaddr
        );

        goto_priv(PRIV_M);
        TEST_ASSERT("after the first no-H same-page translated lr.d access fault, both target words and the nearby guard word remain unchanged before the follow-up translated atomic fault",
            read64(target_phys[0]) == target_patterns[0][round] &&
            read64(target_phys[1]) == target_patterns[1][round] &&
            read64(guard_phys) == guard_patterns[round]
        );

        ai_micro_m_memblock_configure_pmp_page_window(
            target_phys[second], AI_MICRO_MEMBLOCK_PMP_TOR_NO_W);

        TEST_SETUP_EXCEPT();
        (void)ai_micro_m_memblock_translated_amoor_d(amo_vaddr,
            amo_masks[second][round]);
        TEST_ASSERT("after the earlier translated lr.d fault has already occupied the shared single-port MemExceptionBuffer path, the follow-up translated atomic fault must refresh both cause and tval to the newer AMO-side address",
            excpt.triggered == true &&
            excpt.cause == CAUSE_SAF &&
            excpt.tval == amo_vaddr
        );

        goto_priv(PRIV_M);
        TEST_ASSERT("the same-page translated lr.d->atomic fault-switch sequence leaves both target words and the nearby guard word unchanged because both single-port faults are side-effect free",
            read64(target_phys[0]) == target_patterns[0][round] &&
            read64(target_phys[1]) == target_patterns[1][round] &&
            read64(guard_phys) == guard_patterns[round]
        );
    }

    (void)observed_value;
    ai_micro_m_memblock_reset_env();

    TEST_END();
}

bool ai_micro_m_samepage_amo_fault_switch_amo_then_lr_keeps_latest_cause_and_tval_corner()
{
    TEST_START();

    static const uintptr_t offsets[2] = { 0x180U, 0x1c0U };
    static const uint64_t target_patterns[2][2] = {
        { 0x0123456789abcdefULL, 0xf0e1d2c3b4a59687ULL },
        { 0x7766554433221100ULL, 0x21436587a9cbed0fULL },
    };
    static const uint64_t guard_patterns[2] = {
        0xaabbccddeeff0011ULL,
        0x1021324354657687ULL,
    };
    static const uint64_t amo_masks[2][2] = {
        { 0x00ff00ff00ff00ffULL, 0xff00ff00ff00ff00ULL },
        { 0x3333000033330000ULL, 0x0000555500005555ULL },
    };
    const enum test_page page = SWITCH1;
    const uintptr_t page_phys = phys_page_base(page);
    const uintptr_t target_phys[2] = {
        page_phys + offsets[0],
        page_phys + offsets[1],
    };
    const uintptr_t guard_phys = page_phys + 0x200U;
    uint64_t observed_value = 0ULL;

    for (unsigned round = 0; round < 2U; ++round) {
        const unsigned first = round;
        const unsigned second = first ^ 1U;
        const uintptr_t amo_vaddr = hs_page_base(page) + offsets[first];
        const uintptr_t lr_vaddr = hs_page_base(page) + offsets[second];

        goto_priv(PRIV_M);
        ai_micro_m_memblock_reset_env();
        write64(target_phys[0], target_patterns[0][round]);
        write64(target_phys[1], target_patterns[1][round]);
        write64(guard_phys, guard_patterns[round]);
        ai_micro_m_memblock_configure_pmp_page_window(
            target_phys[first], AI_MICRO_MEMBLOCK_PMP_TOR_NO_W);

        ai_micro_m_memblock_prepare_supervisor_translation();
        ai_micro_m_memblock_set_leaf(page, PTE_V | PTE_AD | PTE_RWX);
        sfence_vma();

        TEST_SETUP_EXCEPT();
        (void)ai_micro_m_memblock_translated_amoor_d(amo_vaddr,
            amo_masks[first][round]);
        TEST_ASSERT("the first no-H same-page shared single-port probe comes from the translated atomic path and reports store access fault with tval bound to the AMO address",
            excpt.triggered == true &&
            excpt.cause == CAUSE_SAF &&
            excpt.tval == amo_vaddr
        );

        goto_priv(PRIV_M);
        TEST_ASSERT("after the first no-H same-page translated atomic access fault, both target words and the nearby guard word remain unchanged before the follow-up translated lr.d fault",
            read64(target_phys[0]) == target_patterns[0][round] &&
            read64(target_phys[1]) == target_patterns[1][round] &&
            read64(guard_phys) == guard_patterns[round]
        );

        ai_micro_m_memblock_configure_pmp_page_window(
            target_phys[second], AI_MICRO_MEMBLOCK_PMP_TOR_NO_R);

        TEST_SETUP_EXCEPT();
        observed_value = ai_micro_m_memblock_translated_lr_d(lr_vaddr);
        TEST_ASSERT("after the earlier translated atomic fault has already occupied the shared single-port MemExceptionBuffer path, the follow-up translated lr.d fault must refresh both cause and tval to the newer LR-side address",
            excpt.triggered == true &&
            excpt.cause == CAUSE_LAF &&
            excpt.tval == lr_vaddr
        );

        goto_priv(PRIV_M);
        TEST_ASSERT("the same-page translated atomic->lr.d fault-switch sequence leaves both target words and the nearby guard word unchanged because both single-port faults are side-effect free",
            read64(target_phys[0]) == target_patterns[0][round] &&
            read64(target_phys[1]) == target_patterns[1][round] &&
            read64(guard_phys) == guard_patterns[round]
        );
    }

    (void)observed_value;
    ai_micro_m_memblock_reset_env();

    TEST_END();
}

bool ai_micro_m_samepage_lr_fault_switch_lr_then_lr_keeps_latest_tval_corner()
{
    TEST_START();

    static const uintptr_t offsets[2] = { 0x180U, 0x1c0U };
    static const uint64_t target_patterns[2][2] = {
        { 0x1122334455667788ULL, 0x89abcdef01234567ULL },
        { 0xfedcba9876543210ULL, 0x13579bdf2468ace0ULL },
    };
    static const uint64_t guard_patterns[2] = {
        0x0f1e2d3c4b5a6978ULL,
        0x8877665544332211ULL,
    };
    const enum test_page page = SWITCH1;
    const uintptr_t page_phys = phys_page_base(page);
    const uintptr_t target_phys[2] = {
        page_phys + offsets[0],
        page_phys + offsets[1],
    };
    const uintptr_t guard_phys = page_phys + 0x200U;
    uint64_t observed_value = 0ULL;

    for (unsigned round = 0; round < 2U; ++round) {
        const unsigned first = round;
        const unsigned second = first ^ 1U;
        const uintptr_t first_vaddr = hs_page_base(page) + offsets[first];
        const uintptr_t second_vaddr = hs_page_base(page) + offsets[second];

        goto_priv(PRIV_M);
        ai_micro_m_memblock_reset_env();
        write64(target_phys[0], target_patterns[0][round]);
        write64(target_phys[1], target_patterns[1][round]);
        write64(guard_phys, guard_patterns[round]);
        ai_micro_m_memblock_configure_pmp_page_window(
            target_phys[first], AI_MICRO_MEMBLOCK_PMP_TOR_NO_R);

        ai_micro_m_memblock_prepare_supervisor_translation();
        ai_micro_m_memblock_set_leaf(page, PTE_V | PTE_AD | PTE_RWX);
        sfence_vma();

        TEST_SETUP_EXCEPT();
        observed_value = ai_micro_m_memblock_translated_lr_d(first_vaddr);
        TEST_ASSERT("the first no-H same-page shared single-port lr.d fault reports load access fault with tval bound to the first LR address",
            excpt.triggered == true &&
            excpt.cause == CAUSE_LAF &&
            excpt.tval == first_vaddr
        );

        goto_priv(PRIV_M);
        TEST_ASSERT("after the first no-H same-page translated lr.d fault, both target words and the nearby guard word remain unchanged before the follow-up lr.d fault",
            read64(target_phys[0]) == target_patterns[0][round] &&
            read64(target_phys[1]) == target_patterns[1][round] &&
            read64(guard_phys) == guard_patterns[round]
        );

        ai_micro_m_memblock_configure_pmp_page_window(
            target_phys[second], AI_MICRO_MEMBLOCK_PMP_TOR_NO_R);

        TEST_SETUP_EXCEPT();
        observed_value = ai_micro_m_memblock_translated_lr_d(second_vaddr);
        TEST_ASSERT("after the earlier translated lr.d fault has already occupied the shared single-port MemExceptionBuffer path, the follow-up translated lr.d fault must refresh tval to the newer LR address even though both producers report load access fault",
            excpt.triggered == true &&
            excpt.cause == CAUSE_LAF &&
            excpt.tval == second_vaddr
        );

        goto_priv(PRIV_M);
        TEST_ASSERT("the same-page translated lr.d->lr.d fault-switch sequence leaves both target words and the nearby guard word unchanged because both single-port load-side faults are side-effect free",
            read64(target_phys[0]) == target_patterns[0][round] &&
            read64(target_phys[1]) == target_patterns[1][round] &&
            read64(guard_phys) == guard_patterns[round]
        );
    }

    (void)observed_value;
    ai_micro_m_memblock_reset_env();

    TEST_END();
}

bool ai_micro_m_samepage_amo_fault_switch_amo_then_amo_keeps_latest_tval_corner()
{
    TEST_START();

    static const uintptr_t offsets[2] = { 0x180U, 0x1c0U };
    static const uint64_t target_patterns[2][2] = {
        { 0x0123456789abcdefULL, 0xf0e1d2c3b4a59687ULL },
        { 0x7766554433221100ULL, 0x21436587a9cbed0fULL },
    };
    static const uint64_t guard_patterns[2] = {
        0xaabbccddeeff0011ULL,
        0x1021324354657687ULL,
    };
    static const uint64_t amo_masks[2][2] = {
        { 0x00ff00ff00ff00ffULL, 0xff00ff00ff00ff00ULL },
        { 0x3333000033330000ULL, 0x0000555500005555ULL },
    };
    const enum test_page page = SWITCH1;
    const uintptr_t page_phys = phys_page_base(page);
    const uintptr_t target_phys[2] = {
        page_phys + offsets[0],
        page_phys + offsets[1],
    };
    const uintptr_t guard_phys = page_phys + 0x200U;

    for (unsigned round = 0; round < 2U; ++round) {
        const unsigned first = round;
        const unsigned second = first ^ 1U;
        const uintptr_t first_vaddr = hs_page_base(page) + offsets[first];
        const uintptr_t second_vaddr = hs_page_base(page) + offsets[second];

        goto_priv(PRIV_M);
        ai_micro_m_memblock_reset_env();
        write64(target_phys[0], target_patterns[0][round]);
        write64(target_phys[1], target_patterns[1][round]);
        write64(guard_phys, guard_patterns[round]);
        ai_micro_m_memblock_configure_pmp_page_window(
            target_phys[first], AI_MICRO_MEMBLOCK_PMP_TOR_NO_W);

        ai_micro_m_memblock_prepare_supervisor_translation();
        ai_micro_m_memblock_set_leaf(page, PTE_V | PTE_AD | PTE_RWX);
        sfence_vma();

        TEST_SETUP_EXCEPT();
        (void)ai_micro_m_memblock_translated_amoor_d(first_vaddr,
            amo_masks[first][round]);
        TEST_ASSERT("the first no-H same-page shared single-port atomic fault reports store access fault with tval bound to the first AMO address",
            excpt.triggered == true &&
            excpt.cause == CAUSE_SAF &&
            excpt.tval == first_vaddr
        );

        goto_priv(PRIV_M);
        TEST_ASSERT("after the first no-H same-page translated atomic fault, both target words and the nearby guard word remain unchanged before the follow-up atomic fault",
            read64(target_phys[0]) == target_patterns[0][round] &&
            read64(target_phys[1]) == target_patterns[1][round] &&
            read64(guard_phys) == guard_patterns[round]
        );

        ai_micro_m_memblock_configure_pmp_page_window(
            target_phys[second], AI_MICRO_MEMBLOCK_PMP_TOR_NO_W);

        TEST_SETUP_EXCEPT();
        (void)ai_micro_m_memblock_translated_amoor_d(second_vaddr,
            amo_masks[second][round]);
        TEST_ASSERT("after the earlier translated atomic fault has already occupied the shared single-port MemExceptionBuffer path, the follow-up translated atomic fault must refresh tval to the newer AMO address even though both producers report store access fault",
            excpt.triggered == true &&
            excpt.cause == CAUSE_SAF &&
            excpt.tval == second_vaddr
        );

        goto_priv(PRIV_M);
        TEST_ASSERT("the same-page translated atomic->atomic fault-switch sequence leaves both target words and the nearby guard word unchanged because both single-port store-side faults are side-effect free",
            read64(target_phys[0]) == target_patterns[0][round] &&
            read64(target_phys[1]) == target_patterns[1][round] &&
            read64(guard_phys) == guard_patterns[round]
        );
    }

    ai_micro_m_memblock_reset_env();

    TEST_END();
}

bool ai_micro_m_load_cross_16b_second_split_access_fault_then_same_address_valid_load_recovers_corner()
{
    TEST_START();

    const enum test_page page = SWITCH1;
    const uintptr_t vaddr = ai_micro_m_memblock_cross_16b_addr(page);
    const uintptr_t low_phys = ai_micro_m_memblock_cross_16b_low_phys(page);
    const uintptr_t high_phys = ai_micro_m_memblock_cross_16b_high_phys(page);
    const uintptr_t deny_start = high_phys;
    const uintptr_t deny_end = high_phys + PAGE_SIZE;
    const uint64_t low_pattern = 0x7766554433221100ULL;
    const uint64_t high_pattern = 0xffeeddccbbaa9988ULL;
    const uint64_t expected_value =
        ai_micro_m_memblock_expected_cross_page_4b4b_load(
            low_pattern, high_pattern);
    uint64_t observed_value = 0ULL;

    goto_priv(PRIV_M);
    ai_micro_m_memblock_reset_env();
    write64(low_phys, low_pattern);
    write64(high_phys, high_pattern);
    ai_micro_m_memblock_configure_cross_16b_pmp_window(
        deny_start, deny_end, AI_MICRO_MEMBLOCK_PMP_TOR_NO_R);

    ai_micro_m_memblock_prepare_supervisor_translation();
    ai_micro_m_memblock_set_cross_16b_leaves(PTE_V | PTE_AD | PTE_RWX);
    sfence_vma();

    ai_micro_m_memblock_enable_mprv_supervisor_translation();
    TEST_SETUP_EXCEPT();
    observed_value = ld(vaddr);
    TEST_ASSERT("the no-H same-page cross-16B split load reports load access fault with the original misaligned VA when only the second split loses PMP read permission",
        excpt.triggered == true &&
        excpt.cause == CAUSE_LAF &&
        excpt.tval == ai_micro_m_memblock_cross_16b_second_half_vaddr(vaddr)
    );

    goto_priv(PRIV_M);
    TEST_ASSERT("the second-split-only same-page cross-16B load access fault leaves both aligned doublewords untouched before the same-address recovery probe",
        read64(low_phys) == low_pattern &&
        read64(high_phys) == high_pattern
    );

    ai_micro_m_memblock_restore_pmp_full_access();

    ai_micro_m_memblock_enable_mprv_supervisor_translation();
    TEST_SETUP_EXCEPT();
    observed_value = ld(vaddr);
    TEST_ASSERT("after restoring PMP read permission, the same no-H cross-16B split load recovers immediately and returns the expected 4B+4B merged value",
        excpt.triggered == false &&
        observed_value == expected_value
    );

    goto_priv(PRIV_M);
    TEST_ASSERT("the successful same-page cross-16B load recovery still leaves both aligned doublewords unchanged because the access is read-only",
        read64(low_phys) == low_pattern &&
        read64(high_phys) == high_pattern
    );

    ai_micro_m_memblock_reset_env();

    TEST_END();
}

bool ai_micro_m_store_cross_16b_second_split_access_fault_then_same_address_valid_store_recovers_corner()
{
    TEST_START();

    const enum test_page page = SWITCH1;
    const uintptr_t vaddr = ai_micro_m_memblock_cross_16b_addr(page);
    const uintptr_t low_phys = ai_micro_m_memblock_cross_16b_low_phys(page);
    const uintptr_t high_phys = ai_micro_m_memblock_cross_16b_high_phys(page);
    const uintptr_t deny_start = high_phys;
    const uintptr_t deny_end = high_phys + PAGE_SIZE;
    const uint64_t low_pattern = 0x7766554433221100ULL;
    const uint64_t high_pattern = 0xffeeddccbbaa9988ULL;
    const uint64_t fault_store_value = 0x0123456789abcdefULL;
    const uint64_t recovery_store_value = 0xfedcba9876543210ULL;
    const uint64_t expected_low_after_recovery =
        ai_micro_m_memblock_expected_low_after_4b_store(
            low_pattern, recovery_store_value);
    const uint64_t expected_high_after_recovery =
        ai_micro_m_memblock_expected_high_after_4b_store(
            high_pattern, recovery_store_value);

    goto_priv(PRIV_M);
    ai_micro_m_memblock_reset_env();
    write64(low_phys, low_pattern);
    write64(high_phys, high_pattern);
    ai_micro_m_memblock_configure_cross_16b_pmp_window(
        deny_start, deny_end, AI_MICRO_MEMBLOCK_PMP_TOR_NO_W);

    ai_micro_m_memblock_prepare_supervisor_translation();
    ai_micro_m_memblock_set_cross_16b_leaves(PTE_V | PTE_AD | PTE_RWX);
    sfence_vma();

    ai_micro_m_memblock_enable_mprv_supervisor_translation();
    TEST_SETUP_EXCEPT();
    sd(vaddr, fault_store_value);
    TEST_ASSERT("the no-H same-page cross-16B split store reports store access fault with the original misaligned VA when only the second split loses PMP write permission",
        excpt.triggered == true &&
        excpt.cause == CAUSE_SAF &&
        excpt.tval == ai_micro_m_memblock_cross_16b_second_half_vaddr(vaddr)
    );

    goto_priv(PRIV_M);
    TEST_ASSERT("the second-split-only same-page cross-16B store access fault leaves both aligned doublewords unchanged before the same-address recovery probe",
        read64(low_phys) == low_pattern &&
        read64(high_phys) == high_pattern
    );

    ai_micro_m_memblock_restore_pmp_full_access();

    ai_micro_m_memblock_enable_mprv_supervisor_translation();
    TEST_SETUP_EXCEPT();
    sd(vaddr, recovery_store_value);
    TEST_ASSERT("after restoring PMP write permission, the same no-H cross-16B split store recovers immediately on the original misaligned address",
        excpt.triggered == false
    );

    goto_priv(PRIV_M);
    TEST_ASSERT("the successful same-page cross-16B store recovery must update both aligned doublewords with the expected 4B+4B merged store bytes",
        read64(low_phys) == expected_low_after_recovery &&
        read64(high_phys) == expected_high_after_recovery
    );

    ai_micro_m_memblock_reset_env();

    TEST_END();
}

bool ai_micro_m_load_cross_16b_second_split_access_fault_repeated_then_same_address_valid_load_recovers_corner()
{
    TEST_START();

    const enum test_page page = SWITCH1;
    const uintptr_t vaddr = ai_micro_m_memblock_cross_16b_addr(page);
    const uintptr_t low_phys = ai_micro_m_memblock_cross_16b_low_phys(page);
    const uintptr_t high_phys = ai_micro_m_memblock_cross_16b_high_phys(page);
    const uintptr_t deny_start = high_phys;
    const uintptr_t deny_end = high_phys + PAGE_SIZE;
    const uint64_t low_pattern = 0x0123456789abcdefULL;
    const uint64_t high_pattern = 0xfedcba9876543210ULL;
    const uint64_t expected_value =
        ai_micro_m_memblock_expected_cross_page_4b4b_load(
            low_pattern, high_pattern);
    uint64_t observed_value = 0ULL;

    goto_priv(PRIV_M);
    ai_micro_m_memblock_reset_env();
    write64(low_phys, low_pattern);
    write64(high_phys, high_pattern);
    ai_micro_m_memblock_configure_cross_16b_pmp_window(
        deny_start, deny_end, AI_MICRO_MEMBLOCK_PMP_TOR_NO_R);

    ai_micro_m_memblock_prepare_supervisor_translation();
    ai_micro_m_memblock_set_cross_16b_leaves(PTE_V | PTE_AD | PTE_RWX);
    sfence_vma();

    ai_micro_m_memblock_enable_mprv_supervisor_translation();
    TEST_SETUP_EXCEPT();
    observed_value = ld(vaddr);
    TEST_ASSERT("the first no-H same-page cross-16B split-load probe reports load access fault with the original misaligned VA when the second split is PMP-denied",
        excpt.triggered == true &&
        excpt.cause == CAUSE_LAF &&
        excpt.tval == ai_micro_m_memblock_cross_16b_second_half_vaddr(vaddr)
    );

    ai_micro_m_memblock_enable_mprv_supervisor_translation();
    TEST_SETUP_EXCEPT();
    observed_value = ld(vaddr);
    TEST_ASSERT("repeating the same no-H same-page cross-16B split-load probe keeps reporting the same second-split load access fault instead of reusing any stale older metadata",
        excpt.triggered == true &&
        excpt.cause == CAUSE_LAF &&
        excpt.tval == ai_micro_m_memblock_cross_16b_second_half_vaddr(vaddr)
    );

    goto_priv(PRIV_M);
    TEST_ASSERT("the repeated second-split-only same-page cross-16B load access faults still leave both aligned doublewords untouched before repair",
        read64(low_phys) == low_pattern &&
        read64(high_phys) == high_pattern
    );

    ai_micro_m_memblock_restore_pmp_full_access();

    ai_micro_m_memblock_enable_mprv_supervisor_translation();
    TEST_SETUP_EXCEPT();
    observed_value = ld(vaddr);
    TEST_ASSERT("after restoring PMP read permission, the same no-H cross-16B split load recovers successfully even after the repeated second-split faults",
        excpt.triggered == false &&
        observed_value == expected_value
    );

    goto_priv(PRIV_M);
    TEST_ASSERT("the repeated-fault same-page cross-16B load recovery still leaves both aligned doublewords unchanged because the access is read-only",
        read64(low_phys) == low_pattern &&
        read64(high_phys) == high_pattern
    );

    ai_micro_m_memblock_reset_env();

    TEST_END();
}
