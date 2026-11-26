#include "ai_micro_mmode_memblock_mab_back_to_back_common.h"

bool ai_micro_m_crosspage_4b4b_store_fault_then_other_pair_4b4b_recovery_no_stale_split_template_corner()
{
    TEST_START();

    const enum test_page fault_low_page = SWITCH1;
    const enum test_page fault_high_page = SWITCH2;
    const enum test_page recovery_low_page = VSRWX_GI;
    const enum test_page recovery_high_page = VSRW_GI;
    const uintptr_t fault_vaddr =
        ai_micro_mab_b2b_cross_page_vaddr(fault_low_page, 4U);
    const uintptr_t fault_second_half_vaddr = fault_vaddr + 4U;
    const uintptr_t recovery_vaddr =
        ai_micro_mab_b2b_cross_page_vaddr(recovery_low_page, 4U);
    const uintptr_t fault_low_phys =
        ai_micro_mab_b2b_cross_page_low_phys(fault_low_page);
    const uintptr_t fault_high_phys =
        ai_micro_mab_b2b_cross_page_high_phys(fault_high_page);
    const uintptr_t recovery_low_phys =
        ai_micro_mab_b2b_cross_page_low_phys(recovery_low_page);
    const uintptr_t recovery_high_phys =
        ai_micro_mab_b2b_cross_page_high_phys(recovery_high_page);
    const uintptr_t deny_start = phys_page_base(fault_high_page);
    const uintptr_t deny_end = deny_start + PAGE_SIZE;

    const uint64_t fault_low_seed = 0x7162534435261708ULL;
    const uint64_t fault_high_seed = 0x8899aabbccddeeffULL;
    const uint64_t recovery_low_seed = 0x8f7e6d5c4b3a2918ULL;
    const uint64_t recovery_high_seed = 0x1021324354657687ULL;
    const uint64_t fault_store_value = 0x1122334455667788ULL;
    const uint64_t recovery_store_value = 0x31425364758697a8ULL;

    const uint64_t expected_fault_low_after_fault = fault_low_seed;
    const uint64_t expected_recovery_low_after_store =
        ai_micro_mab_b2b_expected_low_after_4b_store(
            recovery_low_seed, recovery_store_value);
    const uint64_t expected_recovery_high_after_store =
        ai_micro_mab_b2b_expected_high_after_4b_store(
            recovery_high_seed, recovery_store_value);

    goto_priv(PRIV_M);
    ai_micro_mab_b2b_reset_env();
    write64(fault_low_phys, fault_low_seed);
    write64(fault_high_phys, fault_high_seed);
    write64(recovery_low_phys, recovery_low_seed);
    write64(recovery_high_phys, recovery_high_seed);

    ai_micro_mab_b2b_configure_pmp_window(
        deny_start, deny_end, AI_MICRO_MAB_B2B_PMP_TOR_R_ONLY);
    ai_micro_mab_b2b_prepare_supervisor_translation();
    ai_micro_mab_b2b_set_leaf(fault_low_page, PTE_V | PTE_AD | PTE_RWX);
    ai_micro_mab_b2b_set_leaf(fault_high_page, PTE_V | PTE_AD | PTE_RWX);
    ai_micro_mab_b2b_set_leaf(recovery_low_page, PTE_V | PTE_AD | PTE_RWX);
    ai_micro_mab_b2b_set_leaf(recovery_high_page, PTE_V | PTE_AD | PTE_RWX);
    sfence_vma();

    ai_micro_mab_b2b_enable_mprv_s();
    TEST_SETUP_EXCEPT();
    sd(fault_vaddr, fault_store_value);
    TEST_ASSERT(
        "cross-page 4B+4B store must fault with SAF on the true second-half "
        "start address when the fault pair upper page loses PMP write "
        "permission",
        excpt.triggered == true &&
        excpt.cause == CAUSE_SAF &&
        excpt.tval == fault_second_half_vaddr);

    goto_priv(PRIV_M);
    TEST_ASSERT(
        "the cross-page 4B+4B fault episode must not expose the low-half "
        "side effect on the fault pair and keep the later same-template "
        "recovery pair untouched before the other-pair recovery store",
        read64(fault_low_phys) == expected_fault_low_after_fault &&
        read64(fault_high_phys) == fault_high_seed &&
        read64(recovery_low_phys) == recovery_low_seed &&
        read64(recovery_high_phys) == recovery_high_seed);

    ai_micro_mab_b2b_restore_pmp_full_access();

    ai_micro_mab_b2b_enable_mprv_s();
    TEST_SETUP_EXCEPT();
    sd(recovery_vaddr, recovery_store_value);
    TEST_ASSERT(
        "after the fault-pair 4B+4B store episode is repaired, the immediate "
        "other-pair 4B+4B recovery store must complete without reusing the "
        "older split template or stale high-half paddr from the fault pair",
        excpt.triggered == false);

    goto_priv(PRIV_M);
    TEST_ASSERT(
        "the fault-pair 4B+4B store fault then other-pair 4B+4B recovery "
        "sequence must keep the older fault pair unchanged while the recovery pair reflects only the current 4B+4B "
        "overlay",
        read64(fault_low_phys) == expected_fault_low_after_fault &&
        read64(fault_high_phys) == fault_high_seed &&
        read64(recovery_low_phys) == expected_recovery_low_after_store &&
        read64(recovery_high_phys) == expected_recovery_high_after_store);

    ai_micro_mab_b2b_reset_env();

    TEST_END(
        "ai_micro_m_crosspage_4b4b_store_fault_then_other_pair_4b4b_"
        "recovery_no_stale_split_template_corner");
}

bool ai_micro_m_crosspage_1b7b_fault_then_other_pair_1b7b_recovery_no_stale_merge_template_corner()
{
    TEST_START();

    const enum test_page fault_low_page = SWITCH1;
    const enum test_page fault_high_page = SWITCH2;
    const enum test_page recovery_low_page = VSRWX_GI;
    const enum test_page recovery_high_page = VSRW_GI;
    const uintptr_t fault_vaddr =
        ai_micro_mab_b2b_cross_page_vaddr(fault_low_page, 1U);
    const uintptr_t fault_second_half_vaddr = fault_vaddr + 1U;
    const uintptr_t recovery_vaddr =
        ai_micro_mab_b2b_cross_page_vaddr(recovery_low_page, 1U);
    const uintptr_t fault_low_phys =
        ai_micro_mab_b2b_cross_page_low_phys(fault_low_page);
    const uintptr_t fault_high_phys =
        ai_micro_mab_b2b_cross_page_high_phys(fault_high_page);
    const uintptr_t recovery_low_phys =
        ai_micro_mab_b2b_cross_page_low_phys(recovery_low_page);
    const uintptr_t recovery_high_phys =
        ai_micro_mab_b2b_cross_page_high_phys(recovery_high_page);
    const uintptr_t deny_start = phys_page_base(fault_high_page);
    const uintptr_t deny_end = deny_start + PAGE_SIZE;

    const uint64_t fault_low_pattern = 0x8f7e6d5c4b3a2918ULL;
    const uint64_t fault_high_pattern = 0x1021324354657687ULL;
    const uint64_t recovery_low_pattern = 0x7162534435261708ULL;
    const uint64_t recovery_high_pattern = 0x8899aabbccddeeffULL;
    const uint64_t expected_recovery =
        ai_micro_mab_b2b_expected_cross_page_ld(
            recovery_low_pattern, recovery_high_pattern, 1U);
    uint64_t observed = 0ULL;

    goto_priv(PRIV_M);
    ai_micro_mab_b2b_reset_env();
    write64(fault_low_phys, fault_low_pattern);
    write64(fault_high_phys, fault_high_pattern);
    write64(recovery_low_phys, recovery_low_pattern);
    write64(recovery_high_phys, recovery_high_pattern);

    ai_micro_mab_b2b_configure_pmp_window(
        deny_start, deny_end, AI_MICRO_MAB_B2B_PMP_TOR_X_ONLY);
    ai_micro_mab_b2b_prepare_supervisor_translation();
    ai_micro_mab_b2b_set_leaf(fault_low_page, PTE_V | PTE_AD | PTE_RWX);
    ai_micro_mab_b2b_set_leaf(fault_high_page, PTE_V | PTE_AD | PTE_RWX);
    ai_micro_mab_b2b_set_leaf(recovery_low_page, PTE_V | PTE_AD | PTE_RWX);
    ai_micro_mab_b2b_set_leaf(recovery_high_page, PTE_V | PTE_AD | PTE_RWX);
    sfence_vma();

    ai_micro_mab_b2b_enable_mprv_s();
    TEST_SETUP_EXCEPT();
    observed = ld(fault_vaddr);
    TEST_ASSERT(
        "cross-page 1B+7B load must fault with LAF on the true upper-page "
        "start address when the fault pair upper page loses PMP read "
        "permission",
        excpt.triggered == true &&
        excpt.cause == CAUSE_LAF &&
        excpt.tval == fault_second_half_vaddr);

    goto_priv(PRIV_M);
    TEST_ASSERT(
        "the cross-page 1B+7B fault episode must keep both the fault pair "
        "and the later same-template recovery pair unchanged before the "
        "other-pair recovery load",
        read64(fault_low_phys) == fault_low_pattern &&
        read64(fault_high_phys) == fault_high_pattern &&
        read64(recovery_low_phys) == recovery_low_pattern &&
        read64(recovery_high_phys) == recovery_high_pattern);

    ai_micro_mab_b2b_restore_pmp_full_access();

    ai_micro_mab_b2b_enable_mprv_s();
    TEST_SETUP_EXCEPT();
    observed = ld(recovery_vaddr);
    TEST_ASSERT(
        "after the fault-pair 1B+7B load episode is repaired, the immediate "
        "other-pair 1B+7B recovery load must return only the current merged "
        "payload instead of reusing the older splitLoadResp data or merge "
        "template from the fault pair",
        excpt.triggered == false &&
        observed == expected_recovery);

    goto_priv(PRIV_M);
    TEST_ASSERT(
        "the fault-pair 1B+7B load fault then other-pair 1B+7B recovery "
        "sequence must leave all four backing doublewords unchanged",
        read64(fault_low_phys) == fault_low_pattern &&
        read64(fault_high_phys) == fault_high_pattern &&
        read64(recovery_low_phys) == recovery_low_pattern &&
        read64(recovery_high_phys) == recovery_high_pattern);

    ai_micro_mab_b2b_reset_env();

    TEST_END(
        "ai_micro_m_crosspage_1b7b_fault_then_other_pair_1b7b_recovery_"
        "no_stale_merge_template_corner");
}

bool ai_micro_m_crosspage_1b7b_store_fault_then_other_pair_1b7b_recovery_no_stale_split_template_corner()
{
    TEST_START();

    const enum test_page fault_low_page = SWITCH1;
    const enum test_page fault_high_page = SWITCH2;
    const enum test_page recovery_low_page = VSRWX_GI;
    const enum test_page recovery_high_page = VSRW_GI;
    const uintptr_t fault_vaddr =
        ai_micro_mab_b2b_cross_page_vaddr(fault_low_page, 1U);
    const uintptr_t fault_second_half_vaddr = fault_vaddr + 1U;
    const uintptr_t recovery_vaddr =
        ai_micro_mab_b2b_cross_page_vaddr(recovery_low_page, 1U);
    const uintptr_t fault_low_phys =
        ai_micro_mab_b2b_cross_page_low_phys(fault_low_page);
    const uintptr_t fault_high_phys =
        ai_micro_mab_b2b_cross_page_high_phys(fault_high_page);
    const uintptr_t recovery_low_phys =
        ai_micro_mab_b2b_cross_page_low_phys(recovery_low_page);
    const uintptr_t recovery_high_phys =
        ai_micro_mab_b2b_cross_page_high_phys(recovery_high_page);
    const uintptr_t deny_start = phys_page_base(fault_high_page);
    const uintptr_t deny_end = deny_start + PAGE_SIZE;

    const uint64_t fault_low_seed = 0xd4c3b2a1908f7e6dULL;
    const uint64_t fault_high_seed = 0x1a2b3c4d5e6f7081ULL;
    const uint64_t recovery_low_seed = 0x7162534435261708ULL;
    const uint64_t recovery_high_seed = 0x8899aabbccddeeffULL;
    const uint64_t fault_store_value = 0x8f7e6d5c4b3a2918ULL;
    const uint64_t recovery_store_value = 0x31425364758697a8ULL;

    const uint64_t expected_fault_low_after_fault = fault_low_seed;
    const uint64_t expected_recovery_low_after_store =
        ai_micro_mab_b2b_expected_low_after_cross_page_store(
            recovery_low_seed, recovery_store_value, 1U);
    const uint64_t expected_recovery_high_after_store =
        ai_micro_mab_b2b_expected_high_after_cross_page_store(
            recovery_high_seed, recovery_store_value, 1U);

    goto_priv(PRIV_M);
    ai_micro_mab_b2b_reset_env();
    write64(fault_low_phys, fault_low_seed);
    write64(fault_high_phys, fault_high_seed);
    write64(recovery_low_phys, recovery_low_seed);
    write64(recovery_high_phys, recovery_high_seed);

    ai_micro_mab_b2b_configure_pmp_window(
        deny_start, deny_end, AI_MICRO_MAB_B2B_PMP_TOR_R_ONLY);
    ai_micro_mab_b2b_prepare_supervisor_translation();
    ai_micro_mab_b2b_set_leaf(fault_low_page, PTE_V | PTE_AD | PTE_RWX);
    ai_micro_mab_b2b_set_leaf(fault_high_page, PTE_V | PTE_AD | PTE_RWX);
    ai_micro_mab_b2b_set_leaf(recovery_low_page, PTE_V | PTE_AD | PTE_RWX);
    ai_micro_mab_b2b_set_leaf(recovery_high_page, PTE_V | PTE_AD | PTE_RWX);
    sfence_vma();

    ai_micro_mab_b2b_enable_mprv_s();
    TEST_SETUP_EXCEPT();
    sd(fault_vaddr, fault_store_value);
    TEST_ASSERT(
        "cross-page 1B+7B store must fault with SAF on the true upper-page "
        "start address when the fault pair upper page loses PMP write "
        "permission",
        excpt.triggered == true &&
        excpt.cause == CAUSE_SAF &&
        excpt.tval == fault_second_half_vaddr);

    goto_priv(PRIV_M);
    TEST_ASSERT(
        "the cross-page 1B+7B fault episode must not expose the low-page "
        "tail-byte side effect on the fault pair and keep the later "
        "same-template recovery pair untouched before the other-pair "
        "recovery store",
        read64(fault_low_phys) == expected_fault_low_after_fault &&
        read64(fault_high_phys) == fault_high_seed &&
        read64(recovery_low_phys) == recovery_low_seed &&
        read64(recovery_high_phys) == recovery_high_seed);

    ai_micro_mab_b2b_restore_pmp_full_access();

    ai_micro_mab_b2b_enable_mprv_s();
    TEST_SETUP_EXCEPT();
    sd(recovery_vaddr, recovery_store_value);
    TEST_ASSERT(
        "after the fault-pair 1B+7B store episode is repaired, the immediate "
        "other-pair 1B+7B recovery store must complete without reusing the "
        "older split template or stale high-half paddr from the fault pair",
        excpt.triggered == false);

    goto_priv(PRIV_M);
    TEST_ASSERT(
        "the fault-pair 1B+7B store fault then other-pair 1B+7B recovery "
        "sequence must keep the older fault pair unchanged while the recovery pair reflects only the current 1B+7B "
        "overlay",
        read64(fault_low_phys) == expected_fault_low_after_fault &&
        read64(fault_high_phys) == fault_high_seed &&
        read64(recovery_low_phys) == expected_recovery_low_after_store &&
        read64(recovery_high_phys) == expected_recovery_high_after_store);

    ai_micro_mab_b2b_reset_env();

    TEST_END(
        "ai_micro_m_crosspage_1b7b_store_fault_then_other_pair_1b7b_"
        "recovery_no_stale_split_template_corner");
}

bool ai_micro_m_crosspage_2b6b_fault_then_other_pair_2b6b_recovery_no_stale_merge_template_corner()
{
    TEST_START();

    const enum test_page fault_low_page = SWITCH1;
    const enum test_page fault_high_page = SWITCH2;
    const enum test_page recovery_low_page = VSRWX_GI;
    const enum test_page recovery_high_page = VSRW_GI;
    const uintptr_t fault_vaddr =
        ai_micro_mab_b2b_cross_page_vaddr(fault_low_page, 2U);
    const uintptr_t fault_second_half_vaddr = fault_vaddr + 2U;
    const uintptr_t recovery_vaddr =
        ai_micro_mab_b2b_cross_page_vaddr(recovery_low_page, 2U);
    const uintptr_t fault_low_phys =
        ai_micro_mab_b2b_cross_page_low_phys(fault_low_page);
    const uintptr_t fault_high_phys =
        ai_micro_mab_b2b_cross_page_high_phys(fault_high_page);
    const uintptr_t recovery_low_phys =
        ai_micro_mab_b2b_cross_page_low_phys(recovery_low_page);
    const uintptr_t recovery_high_phys =
        ai_micro_mab_b2b_cross_page_high_phys(recovery_high_page);
    const uintptr_t deny_start = phys_page_base(fault_high_page);
    const uintptr_t deny_end = deny_start + PAGE_SIZE;

    const uint64_t fault_low_pattern = 0x9c8b7a6958473625ULL;
    const uint64_t fault_high_pattern = 0x1425364758697a8bULL;
    const uint64_t recovery_low_pattern = 0x0f1e2d3c4b5a6978ULL;
    const uint64_t recovery_high_pattern = 0x98a9bacbdcedfe0fULL;
    const uint64_t expected_recovery =
        ai_micro_mab_b2b_expected_cross_page_ld(
            recovery_low_pattern, recovery_high_pattern, 2U);
    uint64_t observed = 0ULL;

    goto_priv(PRIV_M);
    ai_micro_mab_b2b_reset_env();
    write64(fault_low_phys, fault_low_pattern);
    write64(fault_high_phys, fault_high_pattern);
    write64(recovery_low_phys, recovery_low_pattern);
    write64(recovery_high_phys, recovery_high_pattern);

    ai_micro_mab_b2b_configure_pmp_window(
        deny_start, deny_end, AI_MICRO_MAB_B2B_PMP_TOR_X_ONLY);
    ai_micro_mab_b2b_prepare_supervisor_translation();
    ai_micro_mab_b2b_set_leaf(fault_low_page, PTE_V | PTE_AD | PTE_RWX);
    ai_micro_mab_b2b_set_leaf(fault_high_page, PTE_V | PTE_AD | PTE_RWX);
    ai_micro_mab_b2b_set_leaf(recovery_low_page, PTE_V | PTE_AD | PTE_RWX);
    ai_micro_mab_b2b_set_leaf(recovery_high_page, PTE_V | PTE_AD | PTE_RWX);
    sfence_vma();

    ai_micro_mab_b2b_enable_mprv_s();
    TEST_SETUP_EXCEPT();
    observed = ld(fault_vaddr);
    TEST_ASSERT(
        "cross-page 2B+6B load must fault with LAF on the true upper-page "
        "start address when the fault pair upper page loses PMP read "
        "permission",
        excpt.triggered == true &&
        excpt.cause == CAUSE_LAF &&
        excpt.tval == fault_second_half_vaddr);

    goto_priv(PRIV_M);
    TEST_ASSERT(
        "the cross-page 2B+6B fault episode must keep both the fault pair "
        "and the later same-template recovery pair unchanged before the "
        "other-pair recovery load",
        read64(fault_low_phys) == fault_low_pattern &&
        read64(fault_high_phys) == fault_high_pattern &&
        read64(recovery_low_phys) == recovery_low_pattern &&
        read64(recovery_high_phys) == recovery_high_pattern);

    ai_micro_mab_b2b_restore_pmp_full_access();

    ai_micro_mab_b2b_enable_mprv_s();
    TEST_SETUP_EXCEPT();
    observed = ld(recovery_vaddr);
    TEST_ASSERT(
        "after the fault-pair 2B+6B load episode is repaired, the immediate "
        "other-pair 2B+6B recovery load must return only the current merged "
        "payload instead of reusing the older splitLoadResp data or merge "
        "template from the fault pair",
        excpt.triggered == false &&
        observed == expected_recovery);

    goto_priv(PRIV_M);
    TEST_ASSERT(
        "the fault-pair 2B+6B load fault then other-pair 2B+6B recovery "
        "sequence must leave all four backing doublewords unchanged",
        read64(fault_low_phys) == fault_low_pattern &&
        read64(fault_high_phys) == fault_high_pattern &&
        read64(recovery_low_phys) == recovery_low_pattern &&
        read64(recovery_high_phys) == recovery_high_pattern);

    ai_micro_mab_b2b_reset_env();

    TEST_END(
        "ai_micro_m_crosspage_2b6b_fault_then_other_pair_2b6b_recovery_"
        "no_stale_merge_template_corner");
}

bool ai_micro_m_crosspage_2b6b_store_fault_then_other_pair_2b6b_recovery_no_stale_split_template_corner()
{
    TEST_START();

    const enum test_page fault_low_page = SWITCH1;
    const enum test_page fault_high_page = SWITCH2;
    const enum test_page recovery_low_page = VSRWX_GI;
    const enum test_page recovery_high_page = VSRW_GI;
    const uintptr_t fault_vaddr =
        ai_micro_mab_b2b_cross_page_vaddr(fault_low_page, 2U);
    const uintptr_t fault_second_half_vaddr = fault_vaddr + 2U;
    const uintptr_t recovery_vaddr =
        ai_micro_mab_b2b_cross_page_vaddr(recovery_low_page, 2U);
    const uintptr_t fault_low_phys =
        ai_micro_mab_b2b_cross_page_low_phys(fault_low_page);
    const uintptr_t fault_high_phys =
        ai_micro_mab_b2b_cross_page_high_phys(fault_high_page);
    const uintptr_t recovery_low_phys =
        ai_micro_mab_b2b_cross_page_low_phys(recovery_low_page);
    const uintptr_t recovery_high_phys =
        ai_micro_mab_b2b_cross_page_high_phys(recovery_high_page);
    const uintptr_t deny_start = phys_page_base(fault_high_page);
    const uintptr_t deny_end = deny_start + PAGE_SIZE;

    const uint64_t fault_low_seed = 0xe5d4c3b2a1908f7eULL;
    const uint64_t fault_high_seed = 0x2435465768798a9bULL;
    const uint64_t recovery_low_seed = 0x1029384756a5b4c3ULL;
    const uint64_t recovery_high_seed = 0xcfdecdbcab9a8978ULL;
    const uint64_t fault_store_value = 0x8a7b6c5d4e3f2011ULL;
    const uint64_t recovery_store_value = 0x425364758697a8b9ULL;

    const uint64_t expected_fault_low_after_fault = fault_low_seed;
    const uint64_t expected_recovery_low_after_store =
        ai_micro_mab_b2b_expected_low_after_cross_page_store(
            recovery_low_seed, recovery_store_value, 2U);
    const uint64_t expected_recovery_high_after_store =
        ai_micro_mab_b2b_expected_high_after_cross_page_store(
            recovery_high_seed, recovery_store_value, 2U);

    goto_priv(PRIV_M);
    ai_micro_mab_b2b_reset_env();
    write64(fault_low_phys, fault_low_seed);
    write64(fault_high_phys, fault_high_seed);
    write64(recovery_low_phys, recovery_low_seed);
    write64(recovery_high_phys, recovery_high_seed);

    ai_micro_mab_b2b_configure_pmp_window(
        deny_start, deny_end, AI_MICRO_MAB_B2B_PMP_TOR_R_ONLY);
    ai_micro_mab_b2b_prepare_supervisor_translation();
    ai_micro_mab_b2b_set_leaf(fault_low_page, PTE_V | PTE_AD | PTE_RWX);
    ai_micro_mab_b2b_set_leaf(fault_high_page, PTE_V | PTE_AD | PTE_RWX);
    ai_micro_mab_b2b_set_leaf(recovery_low_page, PTE_V | PTE_AD | PTE_RWX);
    ai_micro_mab_b2b_set_leaf(recovery_high_page, PTE_V | PTE_AD | PTE_RWX);
    sfence_vma();

    ai_micro_mab_b2b_enable_mprv_s();
    TEST_SETUP_EXCEPT();
    sd(fault_vaddr, fault_store_value);
    TEST_ASSERT(
        "cross-page 2B+6B store must fault with SAF on the true upper-page "
        "start address when the fault pair upper page loses PMP write "
        "permission",
        excpt.triggered == true &&
        excpt.cause == CAUSE_SAF &&
        excpt.tval == fault_second_half_vaddr);

    goto_priv(PRIV_M);
    TEST_ASSERT(
        "the cross-page 2B+6B fault episode must not expose the low-page "
        "tail-halfword side effect on the fault pair and keep the later "
        "same-template recovery pair untouched before the other-pair "
        "recovery store",
        read64(fault_low_phys) == expected_fault_low_after_fault &&
        read64(fault_high_phys) == fault_high_seed &&
        read64(recovery_low_phys) == recovery_low_seed &&
        read64(recovery_high_phys) == recovery_high_seed);

    ai_micro_mab_b2b_restore_pmp_full_access();

    ai_micro_mab_b2b_enable_mprv_s();
    TEST_SETUP_EXCEPT();
    sd(recovery_vaddr, recovery_store_value);
    TEST_ASSERT(
        "after the fault-pair 2B+6B store episode is repaired, the immediate "
        "other-pair 2B+6B recovery store must complete without reusing the "
        "older split template or stale high-half paddr from the fault pair",
        excpt.triggered == false);

    goto_priv(PRIV_M);
    TEST_ASSERT(
        "the fault-pair 2B+6B store fault then other-pair 2B+6B recovery "
        "sequence must keep the older fault pair unchanged while the recovery pair reflects only the current 2B+6B "
        "overlay",
        read64(fault_low_phys) == expected_fault_low_after_fault &&
        read64(fault_high_phys) == fault_high_seed &&
        read64(recovery_low_phys) == expected_recovery_low_after_store &&
        read64(recovery_high_phys) == expected_recovery_high_after_store);

    ai_micro_mab_b2b_reset_env();

    TEST_END(
        "ai_micro_m_crosspage_2b6b_store_fault_then_other_pair_2b6b_"
        "recovery_no_stale_split_template_corner");
}

bool ai_micro_m_crosspage_3b5b_fault_then_other_pair_3b5b_recovery_no_stale_merge_template_corner()
{
    TEST_START();

    const enum test_page fault_low_page = SWITCH1;
    const enum test_page fault_high_page = SWITCH2;
    const enum test_page recovery_low_page = VSRWX_GI;
    const enum test_page recovery_high_page = VSRW_GI;
    const uintptr_t fault_vaddr =
        ai_micro_mab_b2b_cross_page_vaddr(fault_low_page, 3U);
    const uintptr_t fault_second_half_vaddr = fault_vaddr + 3U;
    const uintptr_t recovery_vaddr =
        ai_micro_mab_b2b_cross_page_vaddr(recovery_low_page, 3U);
    const uintptr_t fault_low_phys =
        ai_micro_mab_b2b_cross_page_low_phys(fault_low_page);
    const uintptr_t fault_high_phys =
        ai_micro_mab_b2b_cross_page_high_phys(fault_high_page);
    const uintptr_t recovery_low_phys =
        ai_micro_mab_b2b_cross_page_low_phys(recovery_low_page);
    const uintptr_t recovery_high_phys =
        ai_micro_mab_b2b_cross_page_high_phys(recovery_high_page);
    const uintptr_t deny_start = phys_page_base(fault_high_page);
    const uintptr_t deny_end = deny_start + PAGE_SIZE;

    const uint64_t fault_low_pattern = 0xab9c8d7e6f504132ULL;
    const uint64_t fault_high_pattern = 0x15263748596a7b8cULL;
    const uint64_t recovery_low_pattern = 0x2132435465768798ULL;
    const uint64_t recovery_high_pattern = 0xa9bacbdcedfe1021ULL;
    const uint64_t expected_recovery =
        ai_micro_mab_b2b_expected_cross_page_ld(
            recovery_low_pattern, recovery_high_pattern, 3U);
    uint64_t observed = 0ULL;

    goto_priv(PRIV_M);
    ai_micro_mab_b2b_reset_env();
    write64(fault_low_phys, fault_low_pattern);
    write64(fault_high_phys, fault_high_pattern);
    write64(recovery_low_phys, recovery_low_pattern);
    write64(recovery_high_phys, recovery_high_pattern);

    ai_micro_mab_b2b_configure_pmp_window(
        deny_start, deny_end, AI_MICRO_MAB_B2B_PMP_TOR_X_ONLY);
    ai_micro_mab_b2b_prepare_supervisor_translation();
    ai_micro_mab_b2b_set_leaf(fault_low_page, PTE_V | PTE_AD | PTE_RWX);
    ai_micro_mab_b2b_set_leaf(fault_high_page, PTE_V | PTE_AD | PTE_RWX);
    ai_micro_mab_b2b_set_leaf(recovery_low_page, PTE_V | PTE_AD | PTE_RWX);
    ai_micro_mab_b2b_set_leaf(recovery_high_page, PTE_V | PTE_AD | PTE_RWX);
    sfence_vma();

    ai_micro_mab_b2b_enable_mprv_s();
    TEST_SETUP_EXCEPT();
    observed = ld(fault_vaddr);
    TEST_ASSERT(
        "cross-page 3B+5B load must fault with LAF on the true upper-page "
        "start address when the fault pair upper page loses PMP read "
        "permission",
        excpt.triggered == true &&
        excpt.cause == CAUSE_LAF &&
        excpt.tval == fault_second_half_vaddr);

    goto_priv(PRIV_M);
    TEST_ASSERT(
        "the cross-page 3B+5B fault episode must keep both the fault pair "
        "and the later same-template recovery pair unchanged before the "
        "other-pair recovery load",
        read64(fault_low_phys) == fault_low_pattern &&
        read64(fault_high_phys) == fault_high_pattern &&
        read64(recovery_low_phys) == recovery_low_pattern &&
        read64(recovery_high_phys) == recovery_high_pattern);

    ai_micro_mab_b2b_restore_pmp_full_access();

    ai_micro_mab_b2b_enable_mprv_s();
    TEST_SETUP_EXCEPT();
    observed = ld(recovery_vaddr);
    TEST_ASSERT(
        "after the fault-pair 3B+5B load episode is repaired, the immediate "
        "other-pair 3B+5B recovery load must return only the current merged "
        "payload instead of reusing the older splitLoadResp data or merge "
        "template from the fault pair",
        excpt.triggered == false &&
        observed == expected_recovery);

    goto_priv(PRIV_M);
    TEST_ASSERT(
        "the fault-pair 3B+5B load fault then other-pair 3B+5B recovery "
        "sequence must leave all four backing doublewords unchanged",
        read64(fault_low_phys) == fault_low_pattern &&
        read64(fault_high_phys) == fault_high_pattern &&
        read64(recovery_low_phys) == recovery_low_pattern &&
        read64(recovery_high_phys) == recovery_high_pattern);

    ai_micro_mab_b2b_reset_env();

    TEST_END(
        "ai_micro_m_crosspage_3b5b_fault_then_other_pair_3b5b_recovery_"
        "no_stale_merge_template_corner");
}

bool ai_micro_m_crosspage_3b5b_store_fault_then_other_pair_3b5b_recovery_no_stale_split_template_corner()
{
    TEST_START();

    const enum test_page fault_low_page = SWITCH1;
    const enum test_page fault_high_page = SWITCH2;
    const enum test_page recovery_low_page = VSRWX_GI;
    const enum test_page recovery_high_page = VSRW_GI;
    const uintptr_t fault_vaddr =
        ai_micro_mab_b2b_cross_page_vaddr(fault_low_page, 3U);
    const uintptr_t fault_second_half_vaddr = fault_vaddr + 3U;
    const uintptr_t recovery_vaddr =
        ai_micro_mab_b2b_cross_page_vaddr(recovery_low_page, 3U);
    const uintptr_t fault_low_phys =
        ai_micro_mab_b2b_cross_page_low_phys(fault_low_page);
    const uintptr_t fault_high_phys =
        ai_micro_mab_b2b_cross_page_high_phys(fault_high_page);
    const uintptr_t recovery_low_phys =
        ai_micro_mab_b2b_cross_page_low_phys(recovery_low_page);
    const uintptr_t recovery_high_phys =
        ai_micro_mab_b2b_cross_page_high_phys(recovery_high_page);
    const uintptr_t deny_start = phys_page_base(fault_high_page);
    const uintptr_t deny_end = deny_start + PAGE_SIZE;

    const uint64_t fault_low_seed = 0xf6e5d4c3b2a1908fULL;
    const uint64_t fault_high_seed = 0x35465768798a9bacULL;
    const uint64_t recovery_low_seed = 0x32435465768798a9ULL;
    const uint64_t recovery_high_seed = 0xdeedccbb9a897867ULL;
    const uint64_t fault_store_value = 0x9b8a7c6d5e4f3021ULL;
    const uint64_t recovery_store_value = 0x5364758697a8b9caULL;

    const uint64_t expected_fault_low_after_fault = fault_low_seed;
    const uint64_t expected_recovery_low_after_store =
        ai_micro_mab_b2b_expected_low_after_cross_page_store(
            recovery_low_seed, recovery_store_value, 3U);
    const uint64_t expected_recovery_high_after_store =
        ai_micro_mab_b2b_expected_high_after_cross_page_store(
            recovery_high_seed, recovery_store_value, 3U);

    goto_priv(PRIV_M);
    ai_micro_mab_b2b_reset_env();
    write64(fault_low_phys, fault_low_seed);
    write64(fault_high_phys, fault_high_seed);
    write64(recovery_low_phys, recovery_low_seed);
    write64(recovery_high_phys, recovery_high_seed);

    ai_micro_mab_b2b_configure_pmp_window(
        deny_start, deny_end, AI_MICRO_MAB_B2B_PMP_TOR_R_ONLY);
    ai_micro_mab_b2b_prepare_supervisor_translation();
    ai_micro_mab_b2b_set_leaf(fault_low_page, PTE_V | PTE_AD | PTE_RWX);
    ai_micro_mab_b2b_set_leaf(fault_high_page, PTE_V | PTE_AD | PTE_RWX);
    ai_micro_mab_b2b_set_leaf(recovery_low_page, PTE_V | PTE_AD | PTE_RWX);
    ai_micro_mab_b2b_set_leaf(recovery_high_page, PTE_V | PTE_AD | PTE_RWX);
    sfence_vma();

    ai_micro_mab_b2b_enable_mprv_s();
    TEST_SETUP_EXCEPT();
    sd(fault_vaddr, fault_store_value);
    TEST_ASSERT(
        "cross-page 3B+5B store must fault with SAF on the true upper-page "
        "start address when the fault pair upper page loses PMP write "
        "permission",
        excpt.triggered == true &&
        excpt.cause == CAUSE_SAF &&
        excpt.tval == fault_second_half_vaddr);

    goto_priv(PRIV_M);
    TEST_ASSERT(
        "the cross-page 3B+5B fault episode must not expose the low-page "
        "tail-three-byte side effect on the fault pair and keep the later "
        "same-template recovery pair untouched before the other-pair "
        "recovery store",
        read64(fault_low_phys) == expected_fault_low_after_fault &&
        read64(fault_high_phys) == fault_high_seed &&
        read64(recovery_low_phys) == recovery_low_seed &&
        read64(recovery_high_phys) == recovery_high_seed);

    ai_micro_mab_b2b_restore_pmp_full_access();

    ai_micro_mab_b2b_enable_mprv_s();
    TEST_SETUP_EXCEPT();
    sd(recovery_vaddr, recovery_store_value);
    TEST_ASSERT(
        "after the fault-pair 3B+5B store episode is repaired, the immediate "
        "other-pair 3B+5B recovery store must complete without reusing the "
        "older split template or stale high-half paddr from the fault pair",
        excpt.triggered == false);

    goto_priv(PRIV_M);
    TEST_ASSERT(
        "the fault-pair 3B+5B store fault then other-pair 3B+5B recovery "
        "sequence must keep the older fault pair unchanged while the recovery pair reflects only the current "
        "3B+5B overlay",
        read64(fault_low_phys) == expected_fault_low_after_fault &&
        read64(fault_high_phys) == fault_high_seed &&
        read64(recovery_low_phys) == expected_recovery_low_after_store &&
        read64(recovery_high_phys) == expected_recovery_high_after_store);

    ai_micro_mab_b2b_reset_env();

    TEST_END(
        "ai_micro_m_crosspage_3b5b_store_fault_then_other_pair_3b5b_"
        "recovery_no_stale_split_template_corner");
}

bool ai_micro_m_crosspage_3b5b_fault_then_other_pair_2b6b_recovery_no_stale_merge_template_corner()
{
    TEST_START();

    const enum test_page fault_low_page = SWITCH1;
    const enum test_page fault_high_page = SWITCH2;
    const enum test_page recovery_low_page = VSRWX_GI;
    const enum test_page recovery_high_page = VSRW_GI;
    const uintptr_t fault_vaddr =
        ai_micro_mab_b2b_cross_page_vaddr(fault_low_page, 3U);
    const uintptr_t fault_second_half_vaddr = fault_vaddr + 3U;
    const uintptr_t recovery_vaddr =
        ai_micro_mab_b2b_cross_page_vaddr(recovery_low_page, 2U);
    const uintptr_t fault_low_phys =
        ai_micro_mab_b2b_cross_page_low_phys(fault_low_page);
    const uintptr_t fault_high_phys =
        ai_micro_mab_b2b_cross_page_high_phys(fault_high_page);
    const uintptr_t recovery_low_phys =
        ai_micro_mab_b2b_cross_page_low_phys(recovery_low_page);
    const uintptr_t recovery_high_phys =
        ai_micro_mab_b2b_cross_page_high_phys(recovery_high_page);
    const uintptr_t deny_start = phys_page_base(fault_high_page);
    const uintptr_t deny_end = deny_start + PAGE_SIZE;

    const uint64_t fault_low_pattern = 0xb8a7968574635241ULL;
    const uint64_t fault_high_pattern = 0x132435465768798aULL;
    const uint64_t recovery_low_pattern = 0x4f5e6d7c8b9aa9b8ULL;
    const uint64_t recovery_high_pattern = 0xc1d2e3f405162738ULL;
    const uint64_t expected_recovery =
        ai_micro_mab_b2b_expected_cross_page_ld(
            recovery_low_pattern, recovery_high_pattern, 2U);
    uint64_t observed = 0ULL;

    goto_priv(PRIV_M);
    ai_micro_mab_b2b_reset_env();
    write64(fault_low_phys, fault_low_pattern);
    write64(fault_high_phys, fault_high_pattern);
    write64(recovery_low_phys, recovery_low_pattern);
    write64(recovery_high_phys, recovery_high_pattern);

    ai_micro_mab_b2b_configure_pmp_window(
        deny_start, deny_end, AI_MICRO_MAB_B2B_PMP_TOR_X_ONLY);
    ai_micro_mab_b2b_prepare_supervisor_translation();
    ai_micro_mab_b2b_set_leaf(fault_low_page, PTE_V | PTE_AD | PTE_RWX);
    ai_micro_mab_b2b_set_leaf(fault_high_page, PTE_V | PTE_AD | PTE_RWX);
    ai_micro_mab_b2b_set_leaf(recovery_low_page, PTE_V | PTE_AD | PTE_RWX);
    ai_micro_mab_b2b_set_leaf(recovery_high_page, PTE_V | PTE_AD | PTE_RWX);
    sfence_vma();

    ai_micro_mab_b2b_enable_mprv_s();
    TEST_SETUP_EXCEPT();
    observed = ld(fault_vaddr);
    TEST_ASSERT(
        "cross-page 3B+5B load must fault with LAF on the true upper-page "
        "start address when the fault pair upper page loses PMP read "
        "permission",
        excpt.triggered == true &&
        excpt.cause == CAUSE_LAF &&
        excpt.tval == fault_second_half_vaddr);

    goto_priv(PRIV_M);
    TEST_ASSERT(
        "the cross-page 3B+5B fault episode must keep both the fault pair "
        "and the later narrower-template recovery pair unchanged before the "
        "other-pair 2B+6B recovery load",
        read64(fault_low_phys) == fault_low_pattern &&
        read64(fault_high_phys) == fault_high_pattern &&
        read64(recovery_low_phys) == recovery_low_pattern &&
        read64(recovery_high_phys) == recovery_high_pattern);

    ai_micro_mab_b2b_restore_pmp_full_access();

    ai_micro_mab_b2b_enable_mprv_s();
    TEST_SETUP_EXCEPT();
    observed = ld(recovery_vaddr);
    TEST_ASSERT(
        "after the fault-pair 3B+5B load episode is repaired, the immediate "
        "other-pair 2B+6B recovery load must return only the current "
        "narrower merged payload instead of reusing the older wider-LW "
        "splitLoadResp data or merge template from the fault pair",
        excpt.triggered == false &&
        observed == expected_recovery);

    goto_priv(PRIV_M);
    TEST_ASSERT(
        "the fault-pair 3B+5B load fault then other-pair 2B+6B recovery "
        "sequence must leave all four backing doublewords unchanged",
        read64(fault_low_phys) == fault_low_pattern &&
        read64(fault_high_phys) == fault_high_pattern &&
        read64(recovery_low_phys) == recovery_low_pattern &&
        read64(recovery_high_phys) == recovery_high_pattern);

    ai_micro_mab_b2b_reset_env();

    TEST_END(
        "ai_micro_m_crosspage_3b5b_fault_then_other_pair_2b6b_recovery_"
        "no_stale_merge_template_corner");
}

bool ai_micro_m_crosspage_3b5b_store_fault_then_other_pair_2b6b_recovery_no_stale_split_template_corner()
{
    TEST_START();

    const enum test_page fault_low_page = SWITCH1;
    const enum test_page fault_high_page = SWITCH2;
    const enum test_page recovery_low_page = VSRWX_GI;
    const enum test_page recovery_high_page = VSRW_GI;
    const uintptr_t fault_vaddr =
        ai_micro_mab_b2b_cross_page_vaddr(fault_low_page, 3U);
    const uintptr_t fault_second_half_vaddr = fault_vaddr + 3U;
    const uintptr_t recovery_vaddr =
        ai_micro_mab_b2b_cross_page_vaddr(recovery_low_page, 2U);
    const uintptr_t fault_low_phys =
        ai_micro_mab_b2b_cross_page_low_phys(fault_low_page);
    const uintptr_t fault_high_phys =
        ai_micro_mab_b2b_cross_page_high_phys(fault_high_page);
    const uintptr_t recovery_low_phys =
        ai_micro_mab_b2b_cross_page_low_phys(recovery_low_page);
    const uintptr_t recovery_high_phys =
        ai_micro_mab_b2b_cross_page_high_phys(recovery_high_page);
    const uintptr_t deny_start = phys_page_base(fault_high_page);
    const uintptr_t deny_end = deny_start + PAGE_SIZE;

    const uint64_t fault_low_seed = 0xf5e4d3c2b1a09f8eULL;
    const uint64_t fault_high_seed = 0x465768798a9bacbdULL;
    const uint64_t recovery_low_seed = 0x65768798a9bacbdcULL;
    const uint64_t recovery_high_seed = 0xd4c3b2a1908f7e6dULL;
    const uint64_t fault_store_value = 0x8f7e6d5c4b3a2918ULL;
    const uint64_t recovery_store_value = 0x5a6b7c8d9eafc0d1ULL;

    const uint64_t expected_fault_low_after_fault = fault_low_seed;
    const uint64_t expected_recovery_low_after_store =
        ai_micro_mab_b2b_expected_low_after_cross_page_store(
            recovery_low_seed, recovery_store_value, 2U);
    const uint64_t expected_recovery_high_after_store =
        ai_micro_mab_b2b_expected_high_after_cross_page_store(
            recovery_high_seed, recovery_store_value, 2U);

    goto_priv(PRIV_M);
    ai_micro_mab_b2b_reset_env();
    write64(fault_low_phys, fault_low_seed);
    write64(fault_high_phys, fault_high_seed);
    write64(recovery_low_phys, recovery_low_seed);
    write64(recovery_high_phys, recovery_high_seed);

    ai_micro_mab_b2b_configure_pmp_window(
        deny_start, deny_end, AI_MICRO_MAB_B2B_PMP_TOR_R_ONLY);
    ai_micro_mab_b2b_prepare_supervisor_translation();
    ai_micro_mab_b2b_set_leaf(fault_low_page, PTE_V | PTE_AD | PTE_RWX);
    ai_micro_mab_b2b_set_leaf(fault_high_page, PTE_V | PTE_AD | PTE_RWX);
    ai_micro_mab_b2b_set_leaf(recovery_low_page, PTE_V | PTE_AD | PTE_RWX);
    ai_micro_mab_b2b_set_leaf(recovery_high_page, PTE_V | PTE_AD | PTE_RWX);
    sfence_vma();

    ai_micro_mab_b2b_enable_mprv_s();
    TEST_SETUP_EXCEPT();
    sd(fault_vaddr, fault_store_value);
    TEST_ASSERT(
        "cross-page 3B+5B store must fault with SAF on the true upper-page "
        "start address when the fault pair upper page loses PMP write "
        "permission",
        excpt.triggered == true &&
        excpt.cause == CAUSE_SAF &&
        excpt.tval == fault_second_half_vaddr);

    goto_priv(PRIV_M);
    TEST_ASSERT(
        "the cross-page 3B+5B fault episode must not expose the low-page "
        "tail-three-byte side effect on the fault pair and keep the later "
        "narrower-template recovery pair untouched before the other-pair "
        "2B+6B recovery store",
        read64(fault_low_phys) == expected_fault_low_after_fault &&
        read64(fault_high_phys) == fault_high_seed &&
        read64(recovery_low_phys) == recovery_low_seed &&
        read64(recovery_high_phys) == recovery_high_seed);

    ai_micro_mab_b2b_restore_pmp_full_access();

    ai_micro_mab_b2b_enable_mprv_s();
    TEST_SETUP_EXCEPT();
    sd(recovery_vaddr, recovery_store_value);
    TEST_ASSERT(
        "after the fault-pair 3B+5B store episode is repaired, the immediate "
        "other-pair 2B+6B recovery store must complete without reusing the "
        "older wider-SW split template or stale high-half paddr from the "
        "fault pair",
        excpt.triggered == false);

    goto_priv(PRIV_M);
    TEST_ASSERT(
        "the fault-pair 3B+5B store fault then other-pair 2B+6B recovery "
        "sequence must keep the older fault pair unchanged while the recovery pair reflects only the current "
        "2B+6B overlay",
        read64(fault_low_phys) == expected_fault_low_after_fault &&
        read64(fault_high_phys) == fault_high_seed &&
        read64(recovery_low_phys) == expected_recovery_low_after_store &&
        read64(recovery_high_phys) == expected_recovery_high_after_store);

    ai_micro_mab_b2b_reset_env();

    TEST_END(
        "ai_micro_m_crosspage_3b5b_store_fault_then_other_pair_2b6b_"
        "recovery_no_stale_split_template_corner");
}

bool ai_micro_m_crosspage_3b5b_fault_then_other_pair_1b7b_recovery_no_stale_merge_template_corner()
{
    TEST_START();

    const enum test_page fault_low_page = SWITCH1;
    const enum test_page fault_high_page = SWITCH2;
    const enum test_page recovery_low_page = VSRWX_GI;
    const enum test_page recovery_high_page = VSRW_GI;
    const uintptr_t fault_vaddr =
        ai_micro_mab_b2b_cross_page_vaddr(fault_low_page, 3U);
    const uintptr_t fault_second_half_vaddr = fault_vaddr + 3U;
    const uintptr_t recovery_vaddr =
        ai_micro_mab_b2b_cross_page_vaddr(recovery_low_page, 1U);
    const uintptr_t fault_low_phys =
        ai_micro_mab_b2b_cross_page_low_phys(fault_low_page);
    const uintptr_t fault_high_phys =
        ai_micro_mab_b2b_cross_page_high_phys(fault_high_page);
    const uintptr_t recovery_low_phys =
        ai_micro_mab_b2b_cross_page_low_phys(recovery_low_page);
    const uintptr_t recovery_high_phys =
        ai_micro_mab_b2b_cross_page_high_phys(recovery_high_page);
    const uintptr_t deny_start = phys_page_base(fault_high_page);
    const uintptr_t deny_end = deny_start + PAGE_SIZE;

    const uint64_t fault_low_pattern = 0xc8b7a69584736251ULL;
    const uint64_t fault_high_pattern = 0x2435465768798a9bULL;
    const uint64_t recovery_low_pattern = 0x6f7e8d9cabbac9d8ULL;
    const uint64_t recovery_high_pattern = 0xe1f2031425364758ULL;
    const uint64_t expected_recovery =
        ai_micro_mab_b2b_expected_cross_page_ld(
            recovery_low_pattern, recovery_high_pattern, 1U);
    uint64_t observed = 0ULL;

    goto_priv(PRIV_M);
    ai_micro_mab_b2b_reset_env();
    write64(fault_low_phys, fault_low_pattern);
    write64(fault_high_phys, fault_high_pattern);
    write64(recovery_low_phys, recovery_low_pattern);
    write64(recovery_high_phys, recovery_high_pattern);

    ai_micro_mab_b2b_configure_pmp_window(
        deny_start, deny_end, AI_MICRO_MAB_B2B_PMP_TOR_X_ONLY);
    ai_micro_mab_b2b_prepare_supervisor_translation();
    ai_micro_mab_b2b_set_leaf(fault_low_page, PTE_V | PTE_AD | PTE_RWX);
    ai_micro_mab_b2b_set_leaf(fault_high_page, PTE_V | PTE_AD | PTE_RWX);
    ai_micro_mab_b2b_set_leaf(recovery_low_page, PTE_V | PTE_AD | PTE_RWX);
    ai_micro_mab_b2b_set_leaf(recovery_high_page, PTE_V | PTE_AD | PTE_RWX);
    sfence_vma();

    ai_micro_mab_b2b_enable_mprv_s();
    TEST_SETUP_EXCEPT();
    observed = ld(fault_vaddr);
    TEST_ASSERT(
        "cross-page 3B+5B load must fault with LAF on the true upper-page "
        "start address when the fault pair upper page loses PMP read "
        "permission",
        excpt.triggered == true &&
        excpt.cause == CAUSE_LAF &&
        excpt.tval == fault_second_half_vaddr);

    goto_priv(PRIV_M);
    TEST_ASSERT(
        "the cross-page 3B+5B fault episode must keep both the fault pair "
        "and the later narrowest-template recovery pair unchanged before "
        "the other-pair 1B+7B recovery load",
        read64(fault_low_phys) == fault_low_pattern &&
        read64(fault_high_phys) == fault_high_pattern &&
        read64(recovery_low_phys) == recovery_low_pattern &&
        read64(recovery_high_phys) == recovery_high_pattern);

    ai_micro_mab_b2b_restore_pmp_full_access();

    ai_micro_mab_b2b_enable_mprv_s();
    TEST_SETUP_EXCEPT();
    observed = ld(recovery_vaddr);
    TEST_ASSERT(
        "after the fault-pair 3B+5B load episode is repaired, the immediate "
        "other-pair 1B+7B recovery load must return only the current "
        "narrowest merged payload instead of reusing the older wider-LW "
        "splitLoadResp data or merge template from the fault pair",
        excpt.triggered == false &&
        observed == expected_recovery);

    goto_priv(PRIV_M);
    TEST_ASSERT(
        "the fault-pair 3B+5B load fault then other-pair 1B+7B recovery "
        "sequence must leave all four backing doublewords unchanged",
        read64(fault_low_phys) == fault_low_pattern &&
        read64(fault_high_phys) == fault_high_pattern &&
        read64(recovery_low_phys) == recovery_low_pattern &&
        read64(recovery_high_phys) == recovery_high_pattern);

    ai_micro_mab_b2b_reset_env();

    TEST_END(
        "ai_micro_m_crosspage_3b5b_fault_then_other_pair_1b7b_recovery_"
        "no_stale_merge_template_corner");
}

bool ai_micro_m_crosspage_3b5b_store_fault_then_other_pair_1b7b_recovery_no_stale_split_template_corner()
{
    TEST_START();

    const enum test_page fault_low_page = SWITCH1;
    const enum test_page fault_high_page = SWITCH2;
    const enum test_page recovery_low_page = VSRWX_GI;
    const enum test_page recovery_high_page = VSRW_GI;
    const uintptr_t fault_vaddr =
        ai_micro_mab_b2b_cross_page_vaddr(fault_low_page, 3U);
    const uintptr_t fault_second_half_vaddr = fault_vaddr + 3U;
    const uintptr_t recovery_vaddr =
        ai_micro_mab_b2b_cross_page_vaddr(recovery_low_page, 1U);
    const uintptr_t fault_low_phys =
        ai_micro_mab_b2b_cross_page_low_phys(fault_low_page);
    const uintptr_t fault_high_phys =
        ai_micro_mab_b2b_cross_page_high_phys(fault_high_page);
    const uintptr_t recovery_low_phys =
        ai_micro_mab_b2b_cross_page_low_phys(recovery_low_page);
    const uintptr_t recovery_high_phys =
        ai_micro_mab_b2b_cross_page_high_phys(recovery_high_page);
    const uintptr_t deny_start = phys_page_base(fault_high_page);
    const uintptr_t deny_end = deny_start + PAGE_SIZE;

    const uint64_t fault_low_seed = 0xe6d5c4b3a291807fULL;
    const uint64_t fault_high_seed = 0x5768798a9bacbdceULL;
    const uint64_t recovery_low_seed = 0x768798a9bacbdcedULL;
    const uint64_t recovery_high_seed = 0xc3b2a1908f7e6d5cULL;
    const uint64_t fault_store_value = 0x7e6d5c4b3a291807ULL;
    const uint64_t recovery_store_value = 0x4b5c6d7e8fa0b1c2ULL;

    const uint64_t expected_fault_low_after_fault = fault_low_seed;
    const uint64_t expected_recovery_low_after_store =
        ai_micro_mab_b2b_expected_low_after_cross_page_store(
            recovery_low_seed, recovery_store_value, 1U);
    const uint64_t expected_recovery_high_after_store =
        ai_micro_mab_b2b_expected_high_after_cross_page_store(
            recovery_high_seed, recovery_store_value, 1U);

    goto_priv(PRIV_M);
    ai_micro_mab_b2b_reset_env();
    write64(fault_low_phys, fault_low_seed);
    write64(fault_high_phys, fault_high_seed);
    write64(recovery_low_phys, recovery_low_seed);
    write64(recovery_high_phys, recovery_high_seed);

    ai_micro_mab_b2b_configure_pmp_window(
        deny_start, deny_end, AI_MICRO_MAB_B2B_PMP_TOR_R_ONLY);
    ai_micro_mab_b2b_prepare_supervisor_translation();
    ai_micro_mab_b2b_set_leaf(fault_low_page, PTE_V | PTE_AD | PTE_RWX);
    ai_micro_mab_b2b_set_leaf(fault_high_page, PTE_V | PTE_AD | PTE_RWX);
    ai_micro_mab_b2b_set_leaf(recovery_low_page, PTE_V | PTE_AD | PTE_RWX);
    ai_micro_mab_b2b_set_leaf(recovery_high_page, PTE_V | PTE_AD | PTE_RWX);
    sfence_vma();

    ai_micro_mab_b2b_enable_mprv_s();
    TEST_SETUP_EXCEPT();
    sd(fault_vaddr, fault_store_value);
    TEST_ASSERT(
        "cross-page 3B+5B store must fault with SAF on the true upper-page "
        "start address when the fault pair upper page loses PMP write "
        "permission",
        excpt.triggered == true &&
        excpt.cause == CAUSE_SAF &&
        excpt.tval == fault_second_half_vaddr);

    goto_priv(PRIV_M);
    TEST_ASSERT(
        "the cross-page 3B+5B fault episode must not expose the low-page "
        "tail-three-byte side effect on the fault pair and keep the later "
        "narrowest-template recovery pair untouched before the other-pair "
        "1B+7B recovery store",
        read64(fault_low_phys) == expected_fault_low_after_fault &&
        read64(fault_high_phys) == fault_high_seed &&
        read64(recovery_low_phys) == recovery_low_seed &&
        read64(recovery_high_phys) == recovery_high_seed);

    ai_micro_mab_b2b_restore_pmp_full_access();

    ai_micro_mab_b2b_enable_mprv_s();
    TEST_SETUP_EXCEPT();
    sd(recovery_vaddr, recovery_store_value);
    TEST_ASSERT(
        "after the fault-pair 3B+5B store episode is repaired, the immediate "
        "other-pair 1B+7B recovery store must complete without reusing the "
        "older wider-SW split template or stale high-half paddr from the "
        "fault pair",
        excpt.triggered == false);

    goto_priv(PRIV_M);
    TEST_ASSERT(
        "the fault-pair 3B+5B store fault then other-pair 1B+7B recovery "
        "sequence must keep the older fault pair unchanged while the recovery pair reflects only the current "
        "1B+7B overlay",
        read64(fault_low_phys) == expected_fault_low_after_fault &&
        read64(fault_high_phys) == fault_high_seed &&
        read64(recovery_low_phys) == expected_recovery_low_after_store &&
        read64(recovery_high_phys) == expected_recovery_high_after_store);

    ai_micro_mab_b2b_reset_env();

    TEST_END(
        "ai_micro_m_crosspage_3b5b_store_fault_then_other_pair_1b7b_"
        "recovery_no_stale_split_template_corner");
}

bool ai_micro_m_crosspage_2b6b_fault_then_other_pair_1b7b_recovery_no_stale_merge_template_corner()
{
    TEST_START();

    const enum test_page fault_low_page = SWITCH1;
    const enum test_page fault_high_page = SWITCH2;
    const enum test_page recovery_low_page = VSRWX_GI;
    const enum test_page recovery_high_page = VSRW_GI;
    const uintptr_t fault_vaddr =
        ai_micro_mab_b2b_cross_page_vaddr(fault_low_page, 2U);
    const uintptr_t fault_second_half_vaddr = fault_vaddr + 2U;
    const uintptr_t recovery_vaddr =
        ai_micro_mab_b2b_cross_page_vaddr(recovery_low_page, 1U);
    const uintptr_t fault_low_phys =
        ai_micro_mab_b2b_cross_page_low_phys(fault_low_page);
    const uintptr_t fault_high_phys =
        ai_micro_mab_b2b_cross_page_high_phys(fault_high_page);
    const uintptr_t recovery_low_phys =
        ai_micro_mab_b2b_cross_page_low_phys(recovery_low_page);
    const uintptr_t recovery_high_phys =
        ai_micro_mab_b2b_cross_page_high_phys(recovery_high_page);
    const uintptr_t deny_start = phys_page_base(fault_high_page);
    const uintptr_t deny_end = deny_start + PAGE_SIZE;

    const uint64_t fault_low_pattern = 0xb9a8978675645342ULL;
    const uint64_t fault_high_pattern = 0x15263748596a7b8cULL;
    const uint64_t recovery_low_pattern = 0x708192a3b4c5d6e7ULL;
    const uint64_t recovery_high_pattern = 0xf102132435465768ULL;
    const uint64_t expected_recovery =
        ai_micro_mab_b2b_expected_cross_page_ld(
            recovery_low_pattern, recovery_high_pattern, 1U);
    uint64_t observed = 0ULL;

    goto_priv(PRIV_M);
    ai_micro_mab_b2b_reset_env();
    write64(fault_low_phys, fault_low_pattern);
    write64(fault_high_phys, fault_high_pattern);
    write64(recovery_low_phys, recovery_low_pattern);
    write64(recovery_high_phys, recovery_high_pattern);

    ai_micro_mab_b2b_configure_pmp_window(
        deny_start, deny_end, AI_MICRO_MAB_B2B_PMP_TOR_X_ONLY);
    ai_micro_mab_b2b_prepare_supervisor_translation();
    ai_micro_mab_b2b_set_leaf(fault_low_page, PTE_V | PTE_AD | PTE_RWX);
    ai_micro_mab_b2b_set_leaf(fault_high_page, PTE_V | PTE_AD | PTE_RWX);
    ai_micro_mab_b2b_set_leaf(recovery_low_page, PTE_V | PTE_AD | PTE_RWX);
    ai_micro_mab_b2b_set_leaf(recovery_high_page, PTE_V | PTE_AD | PTE_RWX);
    sfence_vma();

    ai_micro_mab_b2b_enable_mprv_s();
    TEST_SETUP_EXCEPT();
    observed = ld(fault_vaddr);
    TEST_ASSERT(
        "cross-page 2B+6B load must fault with LAF on the true upper-page "
        "start address when the fault pair upper page loses PMP read "
        "permission",
        excpt.triggered == true &&
        excpt.cause == CAUSE_LAF &&
        excpt.tval == fault_second_half_vaddr);

    goto_priv(PRIV_M);
    TEST_ASSERT(
        "the cross-page 2B+6B fault episode must keep both the fault pair "
        "and the later narrowest-template recovery pair unchanged before "
        "the other-pair 1B+7B recovery load",
        read64(fault_low_phys) == fault_low_pattern &&
        read64(fault_high_phys) == fault_high_pattern &&
        read64(recovery_low_phys) == recovery_low_pattern &&
        read64(recovery_high_phys) == recovery_high_pattern);

    ai_micro_mab_b2b_restore_pmp_full_access();

    ai_micro_mab_b2b_enable_mprv_s();
    TEST_SETUP_EXCEPT();
    observed = ld(recovery_vaddr);
    TEST_ASSERT(
        "after the fault-pair 2B+6B load episode is repaired, the immediate "
        "other-pair 1B+7B recovery load must return only the current "
        "narrowest merged payload instead of reusing the older wider-LH "
        "splitLoadResp data or merge template from the fault pair",
        excpt.triggered == false &&
        observed == expected_recovery);

    goto_priv(PRIV_M);
    TEST_ASSERT(
        "the fault-pair 2B+6B load fault then other-pair 1B+7B recovery "
        "sequence must leave all four backing doublewords unchanged",
        read64(fault_low_phys) == fault_low_pattern &&
        read64(fault_high_phys) == fault_high_pattern &&
        read64(recovery_low_phys) == recovery_low_pattern &&
        read64(recovery_high_phys) == recovery_high_pattern);

    ai_micro_mab_b2b_reset_env();

    TEST_END(
        "ai_micro_m_crosspage_2b6b_fault_then_other_pair_1b7b_recovery_"
        "no_stale_merge_template_corner");
}

bool ai_micro_m_crosspage_2b6b_store_fault_then_other_pair_1b7b_recovery_no_stale_split_template_corner()
{
    TEST_START();

    const enum test_page fault_low_page = SWITCH1;
    const enum test_page fault_high_page = SWITCH2;
    const enum test_page recovery_low_page = VSRWX_GI;
    const enum test_page recovery_high_page = VSRW_GI;
    const uintptr_t fault_vaddr =
        ai_micro_mab_b2b_cross_page_vaddr(fault_low_page, 2U);
    const uintptr_t fault_second_half_vaddr = fault_vaddr + 2U;
    const uintptr_t recovery_vaddr =
        ai_micro_mab_b2b_cross_page_vaddr(recovery_low_page, 1U);
    const uintptr_t fault_low_phys =
        ai_micro_mab_b2b_cross_page_low_phys(fault_low_page);
    const uintptr_t fault_high_phys =
        ai_micro_mab_b2b_cross_page_high_phys(fault_high_page);
    const uintptr_t recovery_low_phys =
        ai_micro_mab_b2b_cross_page_low_phys(recovery_low_page);
    const uintptr_t recovery_high_phys =
        ai_micro_mab_b2b_cross_page_high_phys(recovery_high_page);
    const uintptr_t deny_start = phys_page_base(fault_high_page);
    const uintptr_t deny_end = deny_start + PAGE_SIZE;

    const uint64_t fault_low_seed = 0xd6c5b4a39281706fULL;
    const uint64_t fault_high_seed = 0x35465768798a9bacULL;
    const uint64_t recovery_low_seed = 0x5465768798a9bacbULL;
    const uint64_t recovery_high_seed = 0xb2a1908f7e6d5c4bULL;
    const uint64_t fault_store_value = 0x6d5c4b3a291807f6ULL;
    const uint64_t recovery_store_value = 0x3a4b5c6d7e8fa0b1ULL;

    const uint64_t expected_fault_low_after_fault = fault_low_seed;
    const uint64_t expected_recovery_low_after_store =
        ai_micro_mab_b2b_expected_low_after_cross_page_store(
            recovery_low_seed, recovery_store_value, 1U);
    const uint64_t expected_recovery_high_after_store =
        ai_micro_mab_b2b_expected_high_after_cross_page_store(
            recovery_high_seed, recovery_store_value, 1U);

    goto_priv(PRIV_M);
    ai_micro_mab_b2b_reset_env();
    write64(fault_low_phys, fault_low_seed);
    write64(fault_high_phys, fault_high_seed);
    write64(recovery_low_phys, recovery_low_seed);
    write64(recovery_high_phys, recovery_high_seed);

    ai_micro_mab_b2b_configure_pmp_window(
        deny_start, deny_end, AI_MICRO_MAB_B2B_PMP_TOR_R_ONLY);
    ai_micro_mab_b2b_prepare_supervisor_translation();
    ai_micro_mab_b2b_set_leaf(fault_low_page, PTE_V | PTE_AD | PTE_RWX);
    ai_micro_mab_b2b_set_leaf(fault_high_page, PTE_V | PTE_AD | PTE_RWX);
    ai_micro_mab_b2b_set_leaf(recovery_low_page, PTE_V | PTE_AD | PTE_RWX);
    ai_micro_mab_b2b_set_leaf(recovery_high_page, PTE_V | PTE_AD | PTE_RWX);
    sfence_vma();

    ai_micro_mab_b2b_enable_mprv_s();
    TEST_SETUP_EXCEPT();
    sd(fault_vaddr, fault_store_value);
    TEST_ASSERT(
        "cross-page 2B+6B store must fault with SAF on the true upper-page "
        "start address when the fault pair upper page loses PMP write "
        "permission",
        excpt.triggered == true &&
        excpt.cause == CAUSE_SAF &&
        excpt.tval == fault_second_half_vaddr);

    goto_priv(PRIV_M);
    TEST_ASSERT(
        "the cross-page 2B+6B fault episode must not expose the low-page "
        "tail-halfword side effect on the fault pair and keep the later "
        "narrowest-template recovery pair untouched before the other-pair "
        "1B+7B recovery store",
        read64(fault_low_phys) == expected_fault_low_after_fault &&
        read64(fault_high_phys) == fault_high_seed &&
        read64(recovery_low_phys) == recovery_low_seed &&
        read64(recovery_high_phys) == recovery_high_seed);

    ai_micro_mab_b2b_restore_pmp_full_access();

    ai_micro_mab_b2b_enable_mprv_s();
    TEST_SETUP_EXCEPT();
    sd(recovery_vaddr, recovery_store_value);
    TEST_ASSERT(
        "after the fault-pair 2B+6B store episode is repaired, the immediate "
        "other-pair 1B+7B recovery store must complete without reusing the "
        "older wider-SH split template or stale high-half paddr from the "
        "fault pair",
        excpt.triggered == false);

    goto_priv(PRIV_M);
    TEST_ASSERT(
        "the fault-pair 2B+6B store fault then other-pair 1B+7B recovery "
        "sequence must keep the older fault pair unchanged while the recovery pair reflects only the "
        "current 1B+7B overlay",
        read64(fault_low_phys) == expected_fault_low_after_fault &&
        read64(fault_high_phys) == fault_high_seed &&
        read64(recovery_low_phys) == expected_recovery_low_after_store &&
        read64(recovery_high_phys) == expected_recovery_high_after_store);

    ai_micro_mab_b2b_reset_env();

    TEST_END(
        "ai_micro_m_crosspage_2b6b_store_fault_then_other_pair_1b7b_"
        "recovery_no_stale_split_template_corner");
}

bool ai_micro_m_crosspage_4b4b_fault_then_other_pair_3b5b_recovery_no_stale_merge_template_corner()
{
    TEST_START();

    const enum test_page fault_low_page = SWITCH1;
    const enum test_page fault_high_page = SWITCH2;
    const enum test_page recovery_low_page = VSRWX_GI;
    const enum test_page recovery_high_page = VSRW_GI;
    const uintptr_t fault_vaddr =
        ai_micro_mab_b2b_cross_page_vaddr(fault_low_page, 4U);
    const uintptr_t fault_second_half_vaddr = fault_vaddr + 4U;
    const uintptr_t recovery_vaddr =
        ai_micro_mab_b2b_cross_page_vaddr(recovery_low_page, 3U);
    const uintptr_t fault_low_phys =
        ai_micro_mab_b2b_cross_page_low_phys(fault_low_page);
    const uintptr_t fault_high_phys =
        ai_micro_mab_b2b_cross_page_high_phys(fault_high_page);
    const uintptr_t recovery_low_phys =
        ai_micro_mab_b2b_cross_page_low_phys(recovery_low_page);
    const uintptr_t recovery_high_phys =
        ai_micro_mab_b2b_cross_page_high_phys(recovery_high_page);
    const uintptr_t deny_start = phys_page_base(fault_high_page);
    const uintptr_t deny_end = deny_start + PAGE_SIZE;

    const uint64_t fault_low_pattern = 0x6152433425161708ULL;
    const uint64_t fault_high_pattern = 0x98a9bacbdcedfe0fULL;
    const uint64_t recovery_low_pattern = 0xa7b6c5d4e3f20110ULL;
    const uint64_t recovery_high_pattern = 0x2132435465768798ULL;
    const uint64_t expected_recovery =
        ai_micro_mab_b2b_expected_cross_page_ld(
            recovery_low_pattern, recovery_high_pattern, 3U);
    uint64_t observed = 0ULL;

    goto_priv(PRIV_M);
    ai_micro_mab_b2b_reset_env();
    write64(fault_low_phys, fault_low_pattern);
    write64(fault_high_phys, fault_high_pattern);
    write64(recovery_low_phys, recovery_low_pattern);
    write64(recovery_high_phys, recovery_high_pattern);

    ai_micro_mab_b2b_configure_pmp_window(
        deny_start, deny_end, AI_MICRO_MAB_B2B_PMP_TOR_X_ONLY);
    ai_micro_mab_b2b_prepare_supervisor_translation();
    ai_micro_mab_b2b_set_leaf(fault_low_page, PTE_V | PTE_AD | PTE_RWX);
    ai_micro_mab_b2b_set_leaf(fault_high_page, PTE_V | PTE_AD | PTE_RWX);
    ai_micro_mab_b2b_set_leaf(recovery_low_page, PTE_V | PTE_AD | PTE_RWX);
    ai_micro_mab_b2b_set_leaf(recovery_high_page, PTE_V | PTE_AD | PTE_RWX);
    sfence_vma();

    ai_micro_mab_b2b_enable_mprv_s();
    TEST_SETUP_EXCEPT();
    observed = ld(fault_vaddr);
    TEST_ASSERT(
        "cross-page 4B+4B load must fault with LAF on the true second-half "
        "start address when the fault pair upper page loses PMP read "
        "permission",
        excpt.triggered == true &&
        excpt.cause == CAUSE_LAF &&
        excpt.tval == fault_second_half_vaddr);

    goto_priv(PRIV_M);
    TEST_ASSERT(
        "the cross-page 4B+4B fault episode must keep both the fault pair "
        "and the later narrower-template recovery pair unchanged before the "
        "other-pair 3B+5B recovery load",
        read64(fault_low_phys) == fault_low_pattern &&
        read64(fault_high_phys) == fault_high_pattern &&
        read64(recovery_low_phys) == recovery_low_pattern &&
        read64(recovery_high_phys) == recovery_high_pattern);

    ai_micro_mab_b2b_restore_pmp_full_access();

    ai_micro_mab_b2b_enable_mprv_s();
    TEST_SETUP_EXCEPT();
    observed = ld(recovery_vaddr);
    TEST_ASSERT(
        "after the fault-pair 4B+4B load episode is repaired, the immediate "
        "other-pair 3B+5B recovery load must return only the current "
        "narrower merged payload instead of reusing the older 4B+4B "
        "splitLoadResp data or merge template from the fault pair",
        excpt.triggered == false &&
        observed == expected_recovery);

    goto_priv(PRIV_M);
    TEST_ASSERT(
        "the fault-pair 4B+4B load fault then other-pair 3B+5B recovery "
        "sequence must leave all four backing doublewords unchanged",
        read64(fault_low_phys) == fault_low_pattern &&
        read64(fault_high_phys) == fault_high_pattern &&
        read64(recovery_low_phys) == recovery_low_pattern &&
        read64(recovery_high_phys) == recovery_high_pattern);

    ai_micro_mab_b2b_reset_env();

    TEST_END(
        "ai_micro_m_crosspage_4b4b_fault_then_other_pair_3b5b_recovery_"
        "no_stale_merge_template_corner");
}

bool ai_micro_m_crosspage_4b4b_store_fault_then_other_pair_3b5b_recovery_no_stale_split_template_corner()
{
    TEST_START();

    const enum test_page fault_low_page = SWITCH1;
    const enum test_page fault_high_page = SWITCH2;
    const enum test_page recovery_low_page = VSRWX_GI;
    const enum test_page recovery_high_page = VSRW_GI;
    const uintptr_t fault_vaddr =
        ai_micro_mab_b2b_cross_page_vaddr(fault_low_page, 4U);
    const uintptr_t fault_second_half_vaddr = fault_vaddr + 4U;
    const uintptr_t recovery_vaddr =
        ai_micro_mab_b2b_cross_page_vaddr(recovery_low_page, 3U);
    const uintptr_t fault_low_phys =
        ai_micro_mab_b2b_cross_page_low_phys(fault_low_page);
    const uintptr_t fault_high_phys =
        ai_micro_mab_b2b_cross_page_high_phys(fault_high_page);
    const uintptr_t recovery_low_phys =
        ai_micro_mab_b2b_cross_page_low_phys(recovery_low_page);
    const uintptr_t recovery_high_phys =
        ai_micro_mab_b2b_cross_page_high_phys(recovery_high_page);
    const uintptr_t deny_start = phys_page_base(fault_high_page);
    const uintptr_t deny_end = deny_start + PAGE_SIZE;

    const uint64_t fault_low_seed = 0x7c6d5e4f30211203ULL;
    const uint64_t fault_high_seed = 0xbadcfed0f1e2d3c4ULL;
    const uint64_t recovery_low_seed = 0x2031425364758697ULL;
    const uint64_t recovery_high_seed = 0x98a7b6c5d4e3f201ULL;
    const uint64_t fault_store_value = 0x1122334455667788ULL;
    const uint64_t recovery_store_value = 0x4a5b6c7d8e9fb0c1ULL;

    const uint64_t expected_fault_low_after_fault = fault_low_seed;
    const uint64_t expected_recovery_low_after_store =
        ai_micro_mab_b2b_expected_low_after_cross_page_store(
            recovery_low_seed, recovery_store_value, 3U);
    const uint64_t expected_recovery_high_after_store =
        ai_micro_mab_b2b_expected_high_after_cross_page_store(
            recovery_high_seed, recovery_store_value, 3U);

    goto_priv(PRIV_M);
    ai_micro_mab_b2b_reset_env();
    write64(fault_low_phys, fault_low_seed);
    write64(fault_high_phys, fault_high_seed);
    write64(recovery_low_phys, recovery_low_seed);
    write64(recovery_high_phys, recovery_high_seed);

    ai_micro_mab_b2b_configure_pmp_window(
        deny_start, deny_end, AI_MICRO_MAB_B2B_PMP_TOR_R_ONLY);
    ai_micro_mab_b2b_prepare_supervisor_translation();
    ai_micro_mab_b2b_set_leaf(fault_low_page, PTE_V | PTE_AD | PTE_RWX);
    ai_micro_mab_b2b_set_leaf(fault_high_page, PTE_V | PTE_AD | PTE_RWX);
    ai_micro_mab_b2b_set_leaf(recovery_low_page, PTE_V | PTE_AD | PTE_RWX);
    ai_micro_mab_b2b_set_leaf(recovery_high_page, PTE_V | PTE_AD | PTE_RWX);
    sfence_vma();

    ai_micro_mab_b2b_enable_mprv_s();
    TEST_SETUP_EXCEPT();
    sd(fault_vaddr, fault_store_value);
    TEST_ASSERT(
        "cross-page 4B+4B store must fault with SAF on the true second-half "
        "start address when the fault pair upper page loses PMP write "
        "permission",
        excpt.triggered == true &&
        excpt.cause == CAUSE_SAF &&
        excpt.tval == fault_second_half_vaddr);

    goto_priv(PRIV_M);
    TEST_ASSERT(
        "the cross-page 4B+4B fault episode must not expose the low-half "
        "side effect on the fault pair and keep the later narrower-template "
        "recovery pair untouched before the other-pair 3B+5B recovery "
        "store",
        read64(fault_low_phys) == expected_fault_low_after_fault &&
        read64(fault_high_phys) == fault_high_seed &&
        read64(recovery_low_phys) == recovery_low_seed &&
        read64(recovery_high_phys) == recovery_high_seed);

    ai_micro_mab_b2b_restore_pmp_full_access();

    ai_micro_mab_b2b_enable_mprv_s();
    TEST_SETUP_EXCEPT();
    sd(recovery_vaddr, recovery_store_value);
    TEST_ASSERT(
        "after the fault-pair 4B+4B store episode is repaired, the immediate "
        "other-pair 3B+5B recovery store must complete without reusing the "
        "older 4B+4B split template or stale high-half paddr from the "
        "fault pair",
        excpt.triggered == false);

    goto_priv(PRIV_M);
    TEST_ASSERT(
        "the fault-pair 4B+4B store fault then other-pair 3B+5B recovery "
        "sequence must keep the older fault pair unchanged while the recovery pair reflects only the current 3B+5B "
        "overlay",
        read64(fault_low_phys) == expected_fault_low_after_fault &&
        read64(fault_high_phys) == fault_high_seed &&
        read64(recovery_low_phys) == expected_recovery_low_after_store &&
        read64(recovery_high_phys) == expected_recovery_high_after_store);

    ai_micro_mab_b2b_reset_env();

    TEST_END(
        "ai_micro_m_crosspage_4b4b_store_fault_then_other_pair_3b5b_"
        "recovery_no_stale_split_template_corner");
}

bool ai_micro_m_crosspage_4b4b_fault_then_other_pair_2b6b_recovery_no_stale_merge_template_corner()
{
    TEST_START();

    const enum test_page fault_low_page = SWITCH1;
    const enum test_page fault_high_page = SWITCH2;
    const enum test_page recovery_low_page = VSRWX_GI;
    const enum test_page recovery_high_page = VSRW_GI;
    const uintptr_t fault_vaddr =
        ai_micro_mab_b2b_cross_page_vaddr(fault_low_page, 4U);
    const uintptr_t fault_second_half_vaddr = fault_vaddr + 4U;
    const uintptr_t recovery_vaddr =
        ai_micro_mab_b2b_cross_page_vaddr(recovery_low_page, 2U);
    const uintptr_t fault_low_phys =
        ai_micro_mab_b2b_cross_page_low_phys(fault_low_page);
    const uintptr_t fault_high_phys =
        ai_micro_mab_b2b_cross_page_high_phys(fault_high_page);
    const uintptr_t recovery_low_phys =
        ai_micro_mab_b2b_cross_page_low_phys(recovery_low_page);
    const uintptr_t recovery_high_phys =
        ai_micro_mab_b2b_cross_page_high_phys(recovery_high_page);
    const uintptr_t deny_start = phys_page_base(fault_high_page);
    const uintptr_t deny_end = deny_start + PAGE_SIZE;

    const uint64_t fault_low_pattern = 0x524334251607f8e9ULL;
    const uint64_t fault_high_pattern = 0xa1b2c3d4e5f60718ULL;
    const uint64_t recovery_low_pattern = 0x31425364758697a8ULL;
    const uint64_t recovery_high_pattern = 0xc9dafb0c1d2e3f40ULL;
    const uint64_t expected_recovery =
        ai_micro_mab_b2b_expected_cross_page_ld(
            recovery_low_pattern, recovery_high_pattern, 2U);
    uint64_t observed = 0ULL;

    goto_priv(PRIV_M);
    ai_micro_mab_b2b_reset_env();
    write64(fault_low_phys, fault_low_pattern);
    write64(fault_high_phys, fault_high_pattern);
    write64(recovery_low_phys, recovery_low_pattern);
    write64(recovery_high_phys, recovery_high_pattern);

    ai_micro_mab_b2b_configure_pmp_window(
        deny_start, deny_end, AI_MICRO_MAB_B2B_PMP_TOR_X_ONLY);
    ai_micro_mab_b2b_prepare_supervisor_translation();
    ai_micro_mab_b2b_set_leaf(fault_low_page, PTE_V | PTE_AD | PTE_RWX);
    ai_micro_mab_b2b_set_leaf(fault_high_page, PTE_V | PTE_AD | PTE_RWX);
    ai_micro_mab_b2b_set_leaf(recovery_low_page, PTE_V | PTE_AD | PTE_RWX);
    ai_micro_mab_b2b_set_leaf(recovery_high_page, PTE_V | PTE_AD | PTE_RWX);
    sfence_vma();

    ai_micro_mab_b2b_enable_mprv_s();
    TEST_SETUP_EXCEPT();
    observed = ld(fault_vaddr);
    TEST_ASSERT(
        "cross-page 4B+4B load must fault with LAF on the true second-half "
        "start address when the fault pair upper page loses PMP read "
        "permission",
        excpt.triggered == true &&
        excpt.cause == CAUSE_LAF &&
        excpt.tval == fault_second_half_vaddr);

    goto_priv(PRIV_M);
    TEST_ASSERT(
        "the cross-page 4B+4B fault episode must keep both the fault pair "
        "and the later narrower-template recovery pair unchanged before the "
        "other-pair 2B+6B recovery load",
        read64(fault_low_phys) == fault_low_pattern &&
        read64(fault_high_phys) == fault_high_pattern &&
        read64(recovery_low_phys) == recovery_low_pattern &&
        read64(recovery_high_phys) == recovery_high_pattern);

    ai_micro_mab_b2b_restore_pmp_full_access();

    ai_micro_mab_b2b_enable_mprv_s();
    TEST_SETUP_EXCEPT();
    observed = ld(recovery_vaddr);
    TEST_ASSERT(
        "after the fault-pair 4B+4B load episode is repaired, the immediate "
        "other-pair 2B+6B recovery load must return only the current "
        "narrower merged payload instead of reusing the older 4B+4B "
        "splitLoadResp data or merge template from the fault pair",
        excpt.triggered == false &&
        observed == expected_recovery);

    goto_priv(PRIV_M);
    TEST_ASSERT(
        "the fault-pair 4B+4B load fault then other-pair 2B+6B recovery "
        "sequence must leave all four backing doublewords unchanged",
        read64(fault_low_phys) == fault_low_pattern &&
        read64(fault_high_phys) == fault_high_pattern &&
        read64(recovery_low_phys) == recovery_low_pattern &&
        read64(recovery_high_phys) == recovery_high_pattern);

    ai_micro_mab_b2b_reset_env();

    TEST_END(
        "ai_micro_m_crosspage_4b4b_fault_then_other_pair_2b6b_recovery_"
        "no_stale_merge_template_corner");
}

bool ai_micro_m_crosspage_4b4b_store_fault_then_other_pair_2b6b_recovery_no_stale_split_template_corner()
{
    TEST_START();

    const enum test_page fault_low_page = SWITCH1;
    const enum test_page fault_high_page = SWITCH2;
    const enum test_page recovery_low_page = VSRWX_GI;
    const enum test_page recovery_high_page = VSRW_GI;
    const uintptr_t fault_vaddr =
        ai_micro_mab_b2b_cross_page_vaddr(fault_low_page, 4U);
    const uintptr_t fault_second_half_vaddr = fault_vaddr + 4U;
    const uintptr_t recovery_vaddr =
        ai_micro_mab_b2b_cross_page_vaddr(recovery_low_page, 2U);
    const uintptr_t fault_low_phys =
        ai_micro_mab_b2b_cross_page_low_phys(fault_low_page);
    const uintptr_t fault_high_phys =
        ai_micro_mab_b2b_cross_page_high_phys(fault_high_page);
    const uintptr_t recovery_low_phys =
        ai_micro_mab_b2b_cross_page_low_phys(recovery_low_page);
    const uintptr_t recovery_high_phys =
        ai_micro_mab_b2b_cross_page_high_phys(recovery_high_page);
    const uintptr_t deny_start = phys_page_base(fault_high_page);
    const uintptr_t deny_end = deny_start + PAGE_SIZE;

    const uint64_t fault_low_seed = 0x63748596a7b8c9daULL;
    const uint64_t fault_high_seed = 0x0f1e2d3c4b5a6978ULL;
    const uint64_t recovery_low_seed = 0x98a7b6c5d4e3f201ULL;
    const uint64_t recovery_high_seed = 0x425364758697a8b9ULL;
    const uint64_t fault_store_value = 0x1122334455667788ULL;
    const uint64_t recovery_store_value = 0x2b3c4d5e6f708192ULL;

    const uint64_t expected_fault_low_after_fault = fault_low_seed;
    const uint64_t expected_recovery_low_after_store =
        ai_micro_mab_b2b_expected_low_after_cross_page_store(
            recovery_low_seed, recovery_store_value, 2U);
    const uint64_t expected_recovery_high_after_store =
        ai_micro_mab_b2b_expected_high_after_cross_page_store(
            recovery_high_seed, recovery_store_value, 2U);

    goto_priv(PRIV_M);
    ai_micro_mab_b2b_reset_env();
    write64(fault_low_phys, fault_low_seed);
    write64(fault_high_phys, fault_high_seed);
    write64(recovery_low_phys, recovery_low_seed);
    write64(recovery_high_phys, recovery_high_seed);

    ai_micro_mab_b2b_configure_pmp_window(
        deny_start, deny_end, AI_MICRO_MAB_B2B_PMP_TOR_R_ONLY);
    ai_micro_mab_b2b_prepare_supervisor_translation();
    ai_micro_mab_b2b_set_leaf(fault_low_page, PTE_V | PTE_AD | PTE_RWX);
    ai_micro_mab_b2b_set_leaf(fault_high_page, PTE_V | PTE_AD | PTE_RWX);
    ai_micro_mab_b2b_set_leaf(recovery_low_page, PTE_V | PTE_AD | PTE_RWX);
    ai_micro_mab_b2b_set_leaf(recovery_high_page, PTE_V | PTE_AD | PTE_RWX);
    sfence_vma();

    ai_micro_mab_b2b_enable_mprv_s();
    TEST_SETUP_EXCEPT();
    sd(fault_vaddr, fault_store_value);
    TEST_ASSERT(
        "cross-page 4B+4B store must fault with SAF on the true second-half "
        "start address when the fault pair upper page loses PMP write "
        "permission",
        excpt.triggered == true &&
        excpt.cause == CAUSE_SAF &&
        excpt.tval == fault_second_half_vaddr);

    goto_priv(PRIV_M);
    TEST_ASSERT(
        "the cross-page 4B+4B fault episode must not expose the low-half "
        "side effect on the fault pair and keep the later narrower-template "
        "recovery pair untouched before the other-pair 2B+6B recovery "
        "store",
        read64(fault_low_phys) == expected_fault_low_after_fault &&
        read64(fault_high_phys) == fault_high_seed &&
        read64(recovery_low_phys) == recovery_low_seed &&
        read64(recovery_high_phys) == recovery_high_seed);

    ai_micro_mab_b2b_restore_pmp_full_access();

    ai_micro_mab_b2b_enable_mprv_s();
    TEST_SETUP_EXCEPT();
    sd(recovery_vaddr, recovery_store_value);
    TEST_ASSERT(
        "after the fault-pair 4B+4B store episode is repaired, the immediate "
        "other-pair 2B+6B recovery store must complete without reusing the "
        "older 4B+4B split template or stale high-half paddr from the "
        "fault pair",
        excpt.triggered == false);

    goto_priv(PRIV_M);
    TEST_ASSERT(
        "the fault-pair 4B+4B store fault then other-pair 2B+6B recovery "
        "sequence must keep the older fault pair unchanged while the recovery pair reflects only the current 2B+6B "
        "overlay",
        read64(fault_low_phys) == expected_fault_low_after_fault &&
        read64(fault_high_phys) == fault_high_seed &&
        read64(recovery_low_phys) == expected_recovery_low_after_store &&
        read64(recovery_high_phys) == expected_recovery_high_after_store);

    ai_micro_mab_b2b_reset_env();

    TEST_END(
        "ai_micro_m_crosspage_4b4b_store_fault_then_other_pair_2b6b_"
        "recovery_no_stale_split_template_corner");
}

bool ai_micro_m_crosspage_4b4b_fault_then_other_pair_3b5b_refault_then_3b5b_recovery_no_stale_merge_template_corner()
{
    TEST_START();

    const enum test_page fault_low_page = SWITCH1;
    const enum test_page fault_high_page = SWITCH2;
    const enum test_page refault_low_page = VSRWX_GI;
    const enum test_page refault_high_page = VSRW_GI;
    const uintptr_t fault_vaddr =
        ai_micro_mab_b2b_cross_page_vaddr(fault_low_page, 4U);
    const uintptr_t fault_second_half_vaddr = fault_vaddr + 4U;
    const uintptr_t refault_vaddr =
        ai_micro_mab_b2b_cross_page_vaddr(refault_low_page, 3U);
    const uintptr_t refault_second_half_vaddr = refault_vaddr + 3U;
    const uintptr_t fault_low_phys =
        ai_micro_mab_b2b_cross_page_low_phys(fault_low_page);
    const uintptr_t fault_high_phys =
        ai_micro_mab_b2b_cross_page_high_phys(fault_high_page);
    const uintptr_t refault_low_phys =
        ai_micro_mab_b2b_cross_page_low_phys(refault_low_page);
    const uintptr_t refault_high_phys =
        ai_micro_mab_b2b_cross_page_high_phys(refault_high_page);
    const uintptr_t fault_deny_start = phys_page_base(fault_high_page);
    const uintptr_t fault_deny_end = fault_deny_start + PAGE_SIZE;
    const uintptr_t refault_deny_start = phys_page_base(refault_high_page);
    const uintptr_t refault_deny_end = refault_deny_start + PAGE_SIZE;

    const uint64_t fault_low_pattern = 0x6152433425161708ULL;
    const uint64_t fault_high_pattern = 0x98a9bacbdcedfe0fULL;
    const uint64_t refault_low_initial = 0xd5c4b3a291807f6eULL;
    const uint64_t refault_high_initial = 0x4c5d6e7f8091a2b3ULL;
    const uint64_t refault_low_pattern = 0xa7b6c5d4e3f20110ULL;
    const uint64_t refault_high_pattern = 0x2132435465768798ULL;
    const uint64_t expected_recovery =
        ai_micro_mab_b2b_expected_cross_page_ld(
            refault_low_pattern, refault_high_pattern, 3U);
    uint64_t observed = 0ULL;

    goto_priv(PRIV_M);
    ai_micro_mab_b2b_reset_env();
    write64(fault_low_phys, fault_low_pattern);
    write64(fault_high_phys, fault_high_pattern);
    write64(refault_low_phys, refault_low_initial);
    write64(refault_high_phys, refault_high_initial);

    ai_micro_mab_b2b_configure_pmp_window(
        fault_deny_start, fault_deny_end, AI_MICRO_MAB_B2B_PMP_TOR_X_ONLY);
    ai_micro_mab_b2b_prepare_supervisor_translation();
    ai_micro_mab_b2b_set_leaf(fault_low_page, PTE_V | PTE_AD | PTE_RWX);
    ai_micro_mab_b2b_set_leaf(fault_high_page, PTE_V | PTE_AD | PTE_RWX);
    ai_micro_mab_b2b_set_leaf(refault_low_page, PTE_V | PTE_AD | PTE_RWX);
    ai_micro_mab_b2b_set_leaf(refault_high_page, PTE_V | PTE_AD | PTE_RWX);
    sfence_vma();

    ai_micro_mab_b2b_enable_mprv_s();
    TEST_SETUP_EXCEPT();
    observed = ld(fault_vaddr);
    TEST_ASSERT(
        "the fault-pair cross-page 4B+4B load must fault with LAF on the "
        "true second-half start address when the fault pair upper page "
        "loses PMP read permission",
        excpt.triggered == true &&
        excpt.cause == CAUSE_LAF &&
        excpt.tval == fault_second_half_vaddr);

    goto_priv(PRIV_M);
    TEST_ASSERT(
        "the fault-pair 4B+4B load fault episode must keep both the fault "
        "pair and the later refault pair unchanged before the other-pair "
        "3B+5B refault load",
        read64(fault_low_phys) == fault_low_pattern &&
        read64(fault_high_phys) == fault_high_pattern &&
        read64(refault_low_phys) == refault_low_initial &&
        read64(refault_high_phys) == refault_high_initial);

    write64(refault_low_phys, refault_low_pattern);
    write64(refault_high_phys, refault_high_pattern);
    ai_micro_mab_b2b_configure_pmp_window(
        refault_deny_start, refault_deny_end, AI_MICRO_MAB_B2B_PMP_TOR_X_ONLY);

    ai_micro_mab_b2b_enable_mprv_s();
    TEST_SETUP_EXCEPT();
    observed = ld(refault_vaddr);
    TEST_ASSERT(
        "after a fault on pair A, the immediate other-pair 3B+5B refault "
        "must report LAF on pair B's true upper-page start address instead "
        "of reusing the older pair-A 4B+4B merge template",
        excpt.triggered == true &&
        excpt.cause == CAUSE_LAF &&
        excpt.tval == refault_second_half_vaddr);

    goto_priv(PRIV_M);
    TEST_ASSERT(
        "the other-pair 3B+5B refault probe must keep pair A frozen and "
        "leave pair B's refreshed backing image unchanged before recovery",
        read64(fault_low_phys) == fault_low_pattern &&
        read64(fault_high_phys) == fault_high_pattern &&
        read64(refault_low_phys) == refault_low_pattern &&
        read64(refault_high_phys) == refault_high_pattern);

    ai_micro_mab_b2b_restore_pmp_full_access();

    ai_micro_mab_b2b_enable_mprv_s();
    TEST_SETUP_EXCEPT();
    observed = ld(refault_vaddr);
    TEST_ASSERT(
        "after the pair-A 4B+4B fault then pair-B 3B+5B refault sequence is "
        "repaired, the other-pair 3B+5B recovery load must return only pair "
        "B's current merged payload instead of stale splitLoadResp data or "
        "merge template from pair A",
        excpt.triggered == false &&
        observed == expected_recovery);

    goto_priv(PRIV_M);
    TEST_ASSERT(
        "the fault-pair 4B+4B fault then other-pair 3B+5B refault/recovery "
        "sequence must leave all four backing doublewords unchanged after "
        "the recovery load",
        read64(fault_low_phys) == fault_low_pattern &&
        read64(fault_high_phys) == fault_high_pattern &&
        read64(refault_low_phys) == refault_low_pattern &&
        read64(refault_high_phys) == refault_high_pattern);

    ai_micro_mab_b2b_reset_env();

    TEST_END(
        "ai_micro_m_crosspage_4b4b_fault_then_other_pair_3b5b_refault_then_"
        "3b5b_recovery_no_stale_merge_template_corner");
}

bool ai_micro_m_crosspage_4b4b_store_fault_then_other_pair_3b5b_refault_then_3b5b_recovery_no_stale_split_template_corner()
{
    TEST_START();

    const enum test_page fault_low_page = SWITCH1;
    const enum test_page fault_high_page = SWITCH2;
    const enum test_page refault_low_page = VSRWX_GI;
    const enum test_page refault_high_page = VSRW_GI;
    const uintptr_t fault_vaddr =
        ai_micro_mab_b2b_cross_page_vaddr(fault_low_page, 4U);
    const uintptr_t fault_second_half_vaddr = fault_vaddr + 4U;
    const uintptr_t refault_vaddr =
        ai_micro_mab_b2b_cross_page_vaddr(refault_low_page, 3U);
    const uintptr_t refault_second_half_vaddr = refault_vaddr + 3U;
    const uintptr_t fault_low_phys =
        ai_micro_mab_b2b_cross_page_low_phys(fault_low_page);
    const uintptr_t fault_high_phys =
        ai_micro_mab_b2b_cross_page_high_phys(fault_high_page);
    const uintptr_t refault_low_phys =
        ai_micro_mab_b2b_cross_page_low_phys(refault_low_page);
    const uintptr_t refault_high_phys =
        ai_micro_mab_b2b_cross_page_high_phys(refault_high_page);
    const uintptr_t fault_deny_start = phys_page_base(fault_high_page);
    const uintptr_t fault_deny_end = fault_deny_start + PAGE_SIZE;
    const uintptr_t refault_deny_start = phys_page_base(refault_high_page);
    const uintptr_t refault_deny_end = refault_deny_start + PAGE_SIZE;

    const uint64_t fault_low_seed = 0x7c6d5e4f30211203ULL;
    const uint64_t fault_high_seed = 0xbadcfed0f1e2d3c4ULL;
    const uint64_t refault_low_initial = 0x1021324354657687ULL;
    const uint64_t refault_high_initial = 0xc4d5e6f708192a3bULL;
    const uint64_t refault_low_seed = 0x2031425364758697ULL;
    const uint64_t refault_high_seed = 0x98a7b6c5d4e3f201ULL;
    const uint64_t fault_store_value = 0x1122334455667788ULL;
    const uint64_t refault_store_value = 0x4a5b6c7d8e9fb0c1ULL;
    const uint64_t recovery_store_value = 0xe1f2031425364758ULL;

    const uint64_t expected_fault_low_after_fault = fault_low_seed;
    const uint64_t expected_refault_low_after_refault = refault_low_seed;
    const uint64_t expected_refault_low_after_recovery =
        ai_micro_mab_b2b_expected_low_after_cross_page_store(
            refault_low_seed, recovery_store_value, 3U);
    const uint64_t expected_refault_high_after_recovery =
        ai_micro_mab_b2b_expected_high_after_cross_page_store(
            refault_high_seed, recovery_store_value, 3U);

    goto_priv(PRIV_M);
    ai_micro_mab_b2b_reset_env();
    write64(fault_low_phys, fault_low_seed);
    write64(fault_high_phys, fault_high_seed);
    write64(refault_low_phys, refault_low_initial);
    write64(refault_high_phys, refault_high_initial);

    ai_micro_mab_b2b_configure_pmp_window(
        fault_deny_start, fault_deny_end, AI_MICRO_MAB_B2B_PMP_TOR_R_ONLY);
    ai_micro_mab_b2b_prepare_supervisor_translation();
    ai_micro_mab_b2b_set_leaf(fault_low_page, PTE_V | PTE_AD | PTE_RWX);
    ai_micro_mab_b2b_set_leaf(fault_high_page, PTE_V | PTE_AD | PTE_RWX);
    ai_micro_mab_b2b_set_leaf(refault_low_page, PTE_V | PTE_AD | PTE_RWX);
    ai_micro_mab_b2b_set_leaf(refault_high_page, PTE_V | PTE_AD | PTE_RWX);
    sfence_vma();

    ai_micro_mab_b2b_enable_mprv_s();
    TEST_SETUP_EXCEPT();
    sd(fault_vaddr, fault_store_value);
    TEST_ASSERT(
        "the fault-pair cross-page 4B+4B store must fault with SAF on the "
        "true second-half start address when the fault pair upper page "
        "loses PMP write permission",
        excpt.triggered == true &&
        excpt.cause == CAUSE_SAF &&
        excpt.tval == fault_second_half_vaddr);

    goto_priv(PRIV_M);
    TEST_ASSERT(
        "the fault-pair 4B+4B store fault episode must not expose the fault pair low-half side effect and keep the later refault pair "
        "untouched before the other-pair 3B+5B refault store",
        read64(fault_low_phys) == expected_fault_low_after_fault &&
        read64(fault_high_phys) == fault_high_seed &&
        read64(refault_low_phys) == refault_low_initial &&
        read64(refault_high_phys) == refault_high_initial);

    write64(refault_low_phys, refault_low_seed);
    write64(refault_high_phys, refault_high_seed);
    ai_micro_mab_b2b_configure_pmp_window(
        refault_deny_start, refault_deny_end, AI_MICRO_MAB_B2B_PMP_TOR_R_ONLY);

    ai_micro_mab_b2b_enable_mprv_s();
    TEST_SETUP_EXCEPT();
    sd(refault_vaddr, refault_store_value);
    TEST_ASSERT(
        "after a fault on pair A, the immediate other-pair 3B+5B refault "
        "must report SAF on pair B's true upper-page start address instead "
        "of reusing the older fault pair split template",
        excpt.triggered == true &&
        excpt.cause == CAUSE_SAF &&
        excpt.tval == refault_second_half_vaddr);

    goto_priv(PRIV_M);
    TEST_ASSERT(
        "the other-pair 3B+5B refault probe must keep pair A frozen while "
        "pair B does not expose the current low-page tail-three-byte side effect "
        "before recovery",
        read64(fault_low_phys) == expected_fault_low_after_fault &&
        read64(fault_high_phys) == fault_high_seed &&
        read64(refault_low_phys) == expected_refault_low_after_refault &&
        read64(refault_high_phys) == refault_high_seed);

    ai_micro_mab_b2b_restore_pmp_full_access();

    ai_micro_mab_b2b_enable_mprv_s();
    TEST_SETUP_EXCEPT();
    sd(refault_vaddr, recovery_store_value);
    TEST_ASSERT(
        "after the pair-A 4B+4B fault then pair-B 3B+5B refault sequence is "
        "repaired, the other-pair 3B+5B recovery store must complete "
        "without reusing pair A's split template or stale high-half paddr",
        excpt.triggered == false);

    goto_priv(PRIV_M);
    TEST_ASSERT(
        "the fault-pair 4B+4B fault then other-pair 3B+5B refault/recovery "
        "sequence must keep pair A unchanged by the faulting store while pair "
        "B reflects only the current 3B+5B overlay",
        read64(fault_low_phys) == expected_fault_low_after_fault &&
        read64(fault_high_phys) == fault_high_seed &&
        read64(refault_low_phys) == expected_refault_low_after_recovery &&
        read64(refault_high_phys) == expected_refault_high_after_recovery);

    ai_micro_mab_b2b_reset_env();

    TEST_END(
        "ai_micro_m_crosspage_4b4b_store_fault_then_other_pair_3b5b_"
        "refault_then_3b5b_recovery_no_stale_split_template_corner");
}

bool ai_micro_m_crosspage_4b4b_fault_then_other_pair_2b6b_refault_then_2b6b_recovery_no_stale_merge_template_corner()
{
    TEST_START();

    const enum test_page fault_low_page = SWITCH1;
    const enum test_page fault_high_page = SWITCH2;
    const enum test_page refault_low_page = VSRWX_GI;
    const enum test_page refault_high_page = VSRW_GI;
    const uintptr_t fault_vaddr =
        ai_micro_mab_b2b_cross_page_vaddr(fault_low_page, 4U);
    const uintptr_t fault_second_half_vaddr = fault_vaddr + 4U;
    const uintptr_t refault_vaddr =
        ai_micro_mab_b2b_cross_page_vaddr(refault_low_page, 2U);
    const uintptr_t refault_second_half_vaddr = refault_vaddr + 2U;
    const uintptr_t fault_low_phys =
        ai_micro_mab_b2b_cross_page_low_phys(fault_low_page);
    const uintptr_t fault_high_phys =
        ai_micro_mab_b2b_cross_page_high_phys(fault_high_page);
    const uintptr_t refault_low_phys =
        ai_micro_mab_b2b_cross_page_low_phys(refault_low_page);
    const uintptr_t refault_high_phys =
        ai_micro_mab_b2b_cross_page_high_phys(refault_high_page);
    const uintptr_t fault_deny_start = phys_page_base(fault_high_page);
    const uintptr_t fault_deny_end = fault_deny_start + PAGE_SIZE;
    const uintptr_t refault_deny_start = phys_page_base(refault_high_page);
    const uintptr_t refault_deny_end = refault_deny_start + PAGE_SIZE;

    const uint64_t fault_low_pattern = 0x524334251607f8e9ULL;
    const uint64_t fault_high_pattern = 0xa1b2c3d4e5f60718ULL;
    const uint64_t refault_low_initial = 0xd7c6b5a493827160ULL;
    const uint64_t refault_high_initial = 0x4d5e6f708192a3b4ULL;
    const uint64_t refault_low_pattern = 0x31425364758697a8ULL;
    const uint64_t refault_high_pattern = 0xc9dafb0c1d2e3f40ULL;
    const uint64_t expected_recovery =
        ai_micro_mab_b2b_expected_cross_page_ld(
            refault_low_pattern, refault_high_pattern, 2U);
    uint64_t observed = 0ULL;

    goto_priv(PRIV_M);
    ai_micro_mab_b2b_reset_env();
    write64(fault_low_phys, fault_low_pattern);
    write64(fault_high_phys, fault_high_pattern);
    write64(refault_low_phys, refault_low_initial);
    write64(refault_high_phys, refault_high_initial);

    ai_micro_mab_b2b_configure_pmp_window(
        fault_deny_start, fault_deny_end, AI_MICRO_MAB_B2B_PMP_TOR_X_ONLY);
    ai_micro_mab_b2b_prepare_supervisor_translation();
    ai_micro_mab_b2b_set_leaf(fault_low_page, PTE_V | PTE_AD | PTE_RWX);
    ai_micro_mab_b2b_set_leaf(fault_high_page, PTE_V | PTE_AD | PTE_RWX);
    ai_micro_mab_b2b_set_leaf(refault_low_page, PTE_V | PTE_AD | PTE_RWX);
    ai_micro_mab_b2b_set_leaf(refault_high_page, PTE_V | PTE_AD | PTE_RWX);
    sfence_vma();

    ai_micro_mab_b2b_enable_mprv_s();
    TEST_SETUP_EXCEPT();
    observed = ld(fault_vaddr);
    TEST_ASSERT(
        "the fault-pair cross-page 4B+4B load must fault with LAF on the "
        "true second-half start address when the fault pair upper page "
        "loses PMP read permission",
        excpt.triggered == true &&
        excpt.cause == CAUSE_LAF &&
        excpt.tval == fault_second_half_vaddr);

    goto_priv(PRIV_M);
    TEST_ASSERT(
        "the fault-pair 4B+4B load fault episode must keep both the fault "
        "pair and the later refault pair unchanged before the other-pair "
        "2B+6B refault load",
        read64(fault_low_phys) == fault_low_pattern &&
        read64(fault_high_phys) == fault_high_pattern &&
        read64(refault_low_phys) == refault_low_initial &&
        read64(refault_high_phys) == refault_high_initial);

    write64(refault_low_phys, refault_low_pattern);
    write64(refault_high_phys, refault_high_pattern);
    ai_micro_mab_b2b_configure_pmp_window(
        refault_deny_start, refault_deny_end, AI_MICRO_MAB_B2B_PMP_TOR_X_ONLY);

    ai_micro_mab_b2b_enable_mprv_s();
    TEST_SETUP_EXCEPT();
    observed = ld(refault_vaddr);
    TEST_ASSERT(
        "after a fault on pair A, the immediate other-pair 2B+6B refault "
        "must report LAF on pair B's true upper-page start address instead "
        "of reusing the older pair-A 4B+4B merge template",
        excpt.triggered == true &&
        excpt.cause == CAUSE_LAF &&
        excpt.tval == refault_second_half_vaddr);

    goto_priv(PRIV_M);
    TEST_ASSERT(
        "the other-pair 2B+6B refault probe must keep pair A frozen and "
        "leave pair B's refreshed backing image unchanged before recovery",
        read64(fault_low_phys) == fault_low_pattern &&
        read64(fault_high_phys) == fault_high_pattern &&
        read64(refault_low_phys) == refault_low_pattern &&
        read64(refault_high_phys) == refault_high_pattern);

    ai_micro_mab_b2b_restore_pmp_full_access();

    ai_micro_mab_b2b_enable_mprv_s();
    TEST_SETUP_EXCEPT();
    observed = ld(refault_vaddr);
    TEST_ASSERT(
        "after the pair-A 4B+4B fault then pair-B 2B+6B refault sequence is "
        "repaired, the other-pair 2B+6B recovery load must return only pair "
        "B's current merged payload instead of stale splitLoadResp data or "
        "merge template from pair A",
        excpt.triggered == false &&
        observed == expected_recovery);

    goto_priv(PRIV_M);
    TEST_ASSERT(
        "the fault-pair 4B+4B fault then other-pair 2B+6B refault/recovery "
        "sequence must leave all four backing doublewords unchanged after "
        "the recovery load",
        read64(fault_low_phys) == fault_low_pattern &&
        read64(fault_high_phys) == fault_high_pattern &&
        read64(refault_low_phys) == refault_low_pattern &&
        read64(refault_high_phys) == refault_high_pattern);

    ai_micro_mab_b2b_reset_env();

    TEST_END(
        "ai_micro_m_crosspage_4b4b_fault_then_other_pair_2b6b_refault_then_"
        "2b6b_recovery_no_stale_merge_template_corner");
}

bool ai_micro_m_crosspage_4b4b_store_fault_then_other_pair_2b6b_refault_then_2b6b_recovery_no_stale_split_template_corner()
{
    TEST_START();

    const enum test_page fault_low_page = SWITCH1;
    const enum test_page fault_high_page = SWITCH2;
    const enum test_page refault_low_page = VSRWX_GI;
    const enum test_page refault_high_page = VSRW_GI;
    const uintptr_t fault_vaddr =
        ai_micro_mab_b2b_cross_page_vaddr(fault_low_page, 4U);
    const uintptr_t fault_second_half_vaddr = fault_vaddr + 4U;
    const uintptr_t refault_vaddr =
        ai_micro_mab_b2b_cross_page_vaddr(refault_low_page, 2U);
    const uintptr_t refault_second_half_vaddr = refault_vaddr + 2U;
    const uintptr_t fault_low_phys =
        ai_micro_mab_b2b_cross_page_low_phys(fault_low_page);
    const uintptr_t fault_high_phys =
        ai_micro_mab_b2b_cross_page_high_phys(fault_high_page);
    const uintptr_t refault_low_phys =
        ai_micro_mab_b2b_cross_page_low_phys(refault_low_page);
    const uintptr_t refault_high_phys =
        ai_micro_mab_b2b_cross_page_high_phys(refault_high_page);
    const uintptr_t fault_deny_start = phys_page_base(fault_high_page);
    const uintptr_t fault_deny_end = fault_deny_start + PAGE_SIZE;
    const uintptr_t refault_deny_start = phys_page_base(refault_high_page);
    const uintptr_t refault_deny_end = refault_deny_start + PAGE_SIZE;

    const uint64_t fault_low_seed = 0x63748596a7b8c9daULL;
    const uint64_t fault_high_seed = 0x0f1e2d3c4b5a6978ULL;
    const uint64_t refault_low_initial = 0x2132435465768798ULL;
    const uint64_t refault_high_initial = 0xd0e1f20314253647ULL;
    const uint64_t refault_low_seed = 0x98a7b6c5d4e3f201ULL;
    const uint64_t refault_high_seed = 0x425364758697a8b9ULL;
    const uint64_t fault_store_value = 0x1122334455667788ULL;
    const uint64_t refault_store_value = 0x2b3c4d5e6f708192ULL;
    const uint64_t recovery_store_value = 0xe2f3041526374859ULL;

    const uint64_t expected_fault_low_after_fault = fault_low_seed;
    const uint64_t expected_refault_low_after_refault = refault_low_seed;
    const uint64_t expected_refault_low_after_recovery =
        ai_micro_mab_b2b_expected_low_after_cross_page_store(
            refault_low_seed, recovery_store_value, 2U);
    const uint64_t expected_refault_high_after_recovery =
        ai_micro_mab_b2b_expected_high_after_cross_page_store(
            refault_high_seed, recovery_store_value, 2U);

    goto_priv(PRIV_M);
    ai_micro_mab_b2b_reset_env();
    write64(fault_low_phys, fault_low_seed);
    write64(fault_high_phys, fault_high_seed);
    write64(refault_low_phys, refault_low_initial);
    write64(refault_high_phys, refault_high_initial);

    ai_micro_mab_b2b_configure_pmp_window(
        fault_deny_start, fault_deny_end, AI_MICRO_MAB_B2B_PMP_TOR_R_ONLY);
    ai_micro_mab_b2b_prepare_supervisor_translation();
    ai_micro_mab_b2b_set_leaf(fault_low_page, PTE_V | PTE_AD | PTE_RWX);
    ai_micro_mab_b2b_set_leaf(fault_high_page, PTE_V | PTE_AD | PTE_RWX);
    ai_micro_mab_b2b_set_leaf(refault_low_page, PTE_V | PTE_AD | PTE_RWX);
    ai_micro_mab_b2b_set_leaf(refault_high_page, PTE_V | PTE_AD | PTE_RWX);
    sfence_vma();

    ai_micro_mab_b2b_enable_mprv_s();
    TEST_SETUP_EXCEPT();
    sd(fault_vaddr, fault_store_value);
    TEST_ASSERT(
        "the fault-pair cross-page 4B+4B store must fault with SAF on the "
        "true second-half start address when the fault pair upper page "
        "loses PMP write permission",
        excpt.triggered == true &&
        excpt.cause == CAUSE_SAF &&
        excpt.tval == fault_second_half_vaddr);

    goto_priv(PRIV_M);
    TEST_ASSERT(
        "the fault-pair 4B+4B store fault episode must not expose the fault pair low-half side effect and keep the later refault pair "
        "untouched before the other-pair 2B+6B refault store",
        read64(fault_low_phys) == expected_fault_low_after_fault &&
        read64(fault_high_phys) == fault_high_seed &&
        read64(refault_low_phys) == refault_low_initial &&
        read64(refault_high_phys) == refault_high_initial);

    write64(refault_low_phys, refault_low_seed);
    write64(refault_high_phys, refault_high_seed);
    ai_micro_mab_b2b_configure_pmp_window(
        refault_deny_start, refault_deny_end, AI_MICRO_MAB_B2B_PMP_TOR_R_ONLY);

    ai_micro_mab_b2b_enable_mprv_s();
    TEST_SETUP_EXCEPT();
    sd(refault_vaddr, refault_store_value);
    TEST_ASSERT(
        "after a fault on pair A, the immediate other-pair 2B+6B refault "
        "must report SAF on pair B's true upper-page start address instead "
        "of reusing the older fault pair split template",
        excpt.triggered == true &&
        excpt.cause == CAUSE_SAF &&
        excpt.tval == refault_second_half_vaddr);

    goto_priv(PRIV_M);
    TEST_ASSERT(
        "the other-pair 2B+6B refault probe must keep pair A frozen while "
        "pair B does not expose the current low-page tail-halfword side effect "
        "before recovery",
        read64(fault_low_phys) == expected_fault_low_after_fault &&
        read64(fault_high_phys) == fault_high_seed &&
        read64(refault_low_phys) == expected_refault_low_after_refault &&
        read64(refault_high_phys) == refault_high_seed);

    ai_micro_mab_b2b_restore_pmp_full_access();

    ai_micro_mab_b2b_enable_mprv_s();
    TEST_SETUP_EXCEPT();
    sd(refault_vaddr, recovery_store_value);
    TEST_ASSERT(
        "after the pair-A 4B+4B fault then pair-B 2B+6B refault sequence is "
        "repaired, the other-pair 2B+6B recovery store must complete "
        "without reusing pair A's split template or stale high-half paddr",
        excpt.triggered == false);

    goto_priv(PRIV_M);
    TEST_ASSERT(
        "the fault-pair 4B+4B fault then other-pair 2B+6B refault/recovery "
        "sequence must keep pair A unchanged by the faulting store while pair "
        "B reflects only the current 2B+6B overlay",
        read64(fault_low_phys) == expected_fault_low_after_fault &&
        read64(fault_high_phys) == fault_high_seed &&
        read64(refault_low_phys) == expected_refault_low_after_recovery &&
        read64(refault_high_phys) == expected_refault_high_after_recovery);

    ai_micro_mab_b2b_reset_env();

    TEST_END(
        "ai_micro_m_crosspage_4b4b_store_fault_then_other_pair_2b6b_"
        "refault_then_2b6b_recovery_no_stale_split_template_corner");
}

bool ai_micro_m_crosspage_3b5b_fault_then_other_pair_3b5b_refault_then_3b5b_recovery_no_stale_merge_template_corner()
{
    TEST_START();

    const enum test_page fault_low_page = SWITCH1;
    const enum test_page fault_high_page = SWITCH2;
    const enum test_page refault_low_page = VSRWX_GI;
    const enum test_page refault_high_page = VSRW_GI;
    const uintptr_t fault_vaddr =
        ai_micro_mab_b2b_cross_page_vaddr(fault_low_page, 3U);
    const uintptr_t fault_second_half_vaddr = fault_vaddr + 3U;
    const uintptr_t refault_vaddr =
        ai_micro_mab_b2b_cross_page_vaddr(refault_low_page, 3U);
    const uintptr_t refault_second_half_vaddr = refault_vaddr + 3U;
    const uintptr_t fault_low_phys =
        ai_micro_mab_b2b_cross_page_low_phys(fault_low_page);
    const uintptr_t fault_high_phys =
        ai_micro_mab_b2b_cross_page_high_phys(fault_high_page);
    const uintptr_t refault_low_phys =
        ai_micro_mab_b2b_cross_page_low_phys(refault_low_page);
    const uintptr_t refault_high_phys =
        ai_micro_mab_b2b_cross_page_high_phys(refault_high_page);
    const uintptr_t fault_deny_start = phys_page_base(fault_high_page);
    const uintptr_t fault_deny_end = fault_deny_start + PAGE_SIZE;
    const uintptr_t refault_deny_start = phys_page_base(refault_high_page);
    const uintptr_t refault_deny_end = refault_deny_start + PAGE_SIZE;

    const uint64_t fault_low_pattern = 0x7a6b5c4d3e2f1001ULL;
    const uint64_t fault_high_pattern = 0x9182736455463728ULL;
    const uint64_t refault_low_initial = 0xf0e1d2c3b4a59687ULL;
    const uint64_t refault_high_initial = 0x2c3d4e5f60718293ULL;
    const uint64_t refault_low_pattern = 0x8697a8b9cadbecfdULL;
    const uint64_t refault_high_pattern = 0x31425364758697a8ULL;
    const uint64_t expected_recovery =
        ai_micro_mab_b2b_expected_cross_page_ld(
            refault_low_pattern, refault_high_pattern, 3U);
    uint64_t observed = 0ULL;

    goto_priv(PRIV_M);
    ai_micro_mab_b2b_reset_env();
    write64(fault_low_phys, fault_low_pattern);
    write64(fault_high_phys, fault_high_pattern);
    write64(refault_low_phys, refault_low_initial);
    write64(refault_high_phys, refault_high_initial);

    ai_micro_mab_b2b_configure_pmp_window(
        fault_deny_start, fault_deny_end, AI_MICRO_MAB_B2B_PMP_TOR_X_ONLY);
    ai_micro_mab_b2b_prepare_supervisor_translation();
    ai_micro_mab_b2b_set_leaf(fault_low_page, PTE_V | PTE_AD | PTE_RWX);
    ai_micro_mab_b2b_set_leaf(fault_high_page, PTE_V | PTE_AD | PTE_RWX);
    ai_micro_mab_b2b_set_leaf(refault_low_page, PTE_V | PTE_AD | PTE_RWX);
    ai_micro_mab_b2b_set_leaf(refault_high_page, PTE_V | PTE_AD | PTE_RWX);
    sfence_vma();

    ai_micro_mab_b2b_enable_mprv_s();
    TEST_SETUP_EXCEPT();
    observed = ld(fault_vaddr);
    TEST_ASSERT(
        "the fault-pair cross-page 3B+5B load must fault with LAF on the "
        "true upper-page start address when the fault pair upper page "
        "loses PMP read permission",
        excpt.triggered == true &&
        excpt.cause == CAUSE_LAF &&
        excpt.tval == fault_second_half_vaddr);

    goto_priv(PRIV_M);
    TEST_ASSERT(
        "the fault-pair 3B+5B load fault episode must keep both the fault "
        "pair and the later refault pair unchanged before the same-template "
        "other-pair 3B+5B refault load",
        read64(fault_low_phys) == fault_low_pattern &&
        read64(fault_high_phys) == fault_high_pattern &&
        read64(refault_low_phys) == refault_low_initial &&
        read64(refault_high_phys) == refault_high_initial);

    write64(refault_low_phys, refault_low_pattern);
    write64(refault_high_phys, refault_high_pattern);
    ai_micro_mab_b2b_configure_pmp_window(
        refault_deny_start, refault_deny_end, AI_MICRO_MAB_B2B_PMP_TOR_X_ONLY);

    ai_micro_mab_b2b_enable_mprv_s();
    TEST_SETUP_EXCEPT();
    observed = ld(refault_vaddr);
    TEST_ASSERT(
        "after a fault on pair A, the immediate other-pair same-template "
        "3B+5B refault must report LAF on pair B's true upper-page start "
        "address instead of reusing pair A's splitLoadResp or merged "
        "payload",
        excpt.triggered == true &&
        excpt.cause == CAUSE_LAF &&
        excpt.tval == refault_second_half_vaddr);

    goto_priv(PRIV_M);
    TEST_ASSERT(
        "the same-template other-pair 3B+5B refault probe must keep pair A "
        "frozen and leave pair B's refreshed backing image unchanged before "
        "recovery",
        read64(fault_low_phys) == fault_low_pattern &&
        read64(fault_high_phys) == fault_high_pattern &&
        read64(refault_low_phys) == refault_low_pattern &&
        read64(refault_high_phys) == refault_high_pattern);

    ai_micro_mab_b2b_restore_pmp_full_access();

    ai_micro_mab_b2b_enable_mprv_s();
    TEST_SETUP_EXCEPT();
    observed = ld(refault_vaddr);
    TEST_ASSERT(
        "after the pair-A 3B+5B fault then pair-B 3B+5B refault sequence is "
        "repaired, the same-template other-pair 3B+5B recovery load must "
        "return only pair B's current merged payload instead of stale "
        "splitLoadResp data or merge template from pair A",
        excpt.triggered == false &&
        observed == expected_recovery);

    goto_priv(PRIV_M);
    TEST_ASSERT(
        "the fault-pair 3B+5B fault then other-pair same-template 3B+5B "
        "refault/recovery sequence must leave all four backing doublewords "
        "unchanged after the recovery load",
        read64(fault_low_phys) == fault_low_pattern &&
        read64(fault_high_phys) == fault_high_pattern &&
        read64(refault_low_phys) == refault_low_pattern &&
        read64(refault_high_phys) == refault_high_pattern);

    ai_micro_mab_b2b_reset_env();

    TEST_END(
        "ai_micro_m_crosspage_3b5b_fault_then_other_pair_3b5b_refault_then_"
        "3b5b_recovery_no_stale_merge_template_corner");
}

bool ai_micro_m_crosspage_3b5b_store_fault_then_other_pair_3b5b_refault_then_3b5b_recovery_no_stale_split_template_corner()
{
    TEST_START();

    const enum test_page fault_low_page = SWITCH1;
    const enum test_page fault_high_page = SWITCH2;
    const enum test_page refault_low_page = VSRWX_GI;
    const enum test_page refault_high_page = VSRW_GI;
    const uintptr_t fault_vaddr =
        ai_micro_mab_b2b_cross_page_vaddr(fault_low_page, 3U);
    const uintptr_t fault_second_half_vaddr = fault_vaddr + 3U;
    const uintptr_t refault_vaddr =
        ai_micro_mab_b2b_cross_page_vaddr(refault_low_page, 3U);
    const uintptr_t refault_second_half_vaddr = refault_vaddr + 3U;
    const uintptr_t fault_low_phys =
        ai_micro_mab_b2b_cross_page_low_phys(fault_low_page);
    const uintptr_t fault_high_phys =
        ai_micro_mab_b2b_cross_page_high_phys(fault_high_page);
    const uintptr_t refault_low_phys =
        ai_micro_mab_b2b_cross_page_low_phys(refault_low_page);
    const uintptr_t refault_high_phys =
        ai_micro_mab_b2b_cross_page_high_phys(refault_high_page);
    const uintptr_t fault_deny_start = phys_page_base(fault_high_page);
    const uintptr_t fault_deny_end = fault_deny_start + PAGE_SIZE;
    const uintptr_t refault_deny_start = phys_page_base(refault_high_page);
    const uintptr_t refault_deny_end = refault_deny_start + PAGE_SIZE;

    const uint64_t fault_low_seed = 0x3d2e1f0011223344ULL;
    const uint64_t fault_high_seed = 0x5566778899aabbccULL;
    const uint64_t refault_low_initial = 0xc3d4e5f60718293aULL;
    const uint64_t refault_high_initial = 0xb2a1908f7e6d5c4bULL;
    const uint64_t refault_low_seed = 0x4f60718293a4b5c6ULL;
    const uint64_t refault_high_seed = 0xd7c6b5a493827160ULL;
    const uint64_t fault_store_value = 0x8877665544332211ULL;
    const uint64_t refault_store_value = 0x1029384756a6b7c8ULL;
    const uint64_t recovery_store_value = 0xefcdab8967452301ULL;

    const uint64_t expected_fault_low_after_fault = fault_low_seed;
    const uint64_t expected_refault_low_after_refault = refault_low_seed;
    const uint64_t expected_refault_low_after_recovery =
        ai_micro_mab_b2b_expected_low_after_cross_page_store(
            refault_low_seed, recovery_store_value, 3U);
    const uint64_t expected_refault_high_after_recovery =
        ai_micro_mab_b2b_expected_high_after_cross_page_store(
            refault_high_seed, recovery_store_value, 3U);

    goto_priv(PRIV_M);
    ai_micro_mab_b2b_reset_env();
    write64(fault_low_phys, fault_low_seed);
    write64(fault_high_phys, fault_high_seed);
    write64(refault_low_phys, refault_low_initial);
    write64(refault_high_phys, refault_high_initial);

    ai_micro_mab_b2b_configure_pmp_window(
        fault_deny_start, fault_deny_end, AI_MICRO_MAB_B2B_PMP_TOR_R_ONLY);
    ai_micro_mab_b2b_prepare_supervisor_translation();
    ai_micro_mab_b2b_set_leaf(fault_low_page, PTE_V | PTE_AD | PTE_RWX);
    ai_micro_mab_b2b_set_leaf(fault_high_page, PTE_V | PTE_AD | PTE_RWX);
    ai_micro_mab_b2b_set_leaf(refault_low_page, PTE_V | PTE_AD | PTE_RWX);
    ai_micro_mab_b2b_set_leaf(refault_high_page, PTE_V | PTE_AD | PTE_RWX);
    sfence_vma();

    ai_micro_mab_b2b_enable_mprv_s();
    TEST_SETUP_EXCEPT();
    sd(fault_vaddr, fault_store_value);
    TEST_ASSERT(
        "the fault-pair cross-page 3B+5B store must fault with SAF on the "
        "true upper-page start address when the fault pair upper page "
        "loses PMP write permission",
        excpt.triggered == true &&
        excpt.cause == CAUSE_SAF &&
        excpt.tval == fault_second_half_vaddr);

    goto_priv(PRIV_M);
    TEST_ASSERT(
        "the fault-pair 3B+5B store fault episode must not expose the fault "
        "pair low-page tail-three-byte side effect and keep the later refault "
        "pair untouched before the same-template other-pair 3B+5B refault "
        "store",
        read64(fault_low_phys) == expected_fault_low_after_fault &&
        read64(fault_high_phys) == fault_high_seed &&
        read64(refault_low_phys) == refault_low_initial &&
        read64(refault_high_phys) == refault_high_initial);

    write64(refault_low_phys, refault_low_seed);
    write64(refault_high_phys, refault_high_seed);
    ai_micro_mab_b2b_configure_pmp_window(
        refault_deny_start, refault_deny_end, AI_MICRO_MAB_B2B_PMP_TOR_R_ONLY);

    ai_micro_mab_b2b_enable_mprv_s();
    TEST_SETUP_EXCEPT();
    sd(refault_vaddr, refault_store_value);
    TEST_ASSERT(
        "after a fault on pair A, the immediate other-pair same-template "
        "3B+5B refault must report SAF on pair B's true upper-page start "
        "address instead of reusing pair A's split template or stale "
        "high-half paddr",
        excpt.triggered == true &&
        excpt.cause == CAUSE_SAF &&
        excpt.tval == refault_second_half_vaddr);

    goto_priv(PRIV_M);
    TEST_ASSERT(
        "the same-template other-pair 3B+5B refault probe must keep pair A "
        "unchanged while pair B does not expose the current low-page tail-three-"
        "byte side effect before recovery",
        read64(fault_low_phys) == expected_fault_low_after_fault &&
        read64(fault_high_phys) == fault_high_seed &&
        read64(refault_low_phys) == expected_refault_low_after_refault &&
        read64(refault_high_phys) == refault_high_seed);

    ai_micro_mab_b2b_restore_pmp_full_access();

    ai_micro_mab_b2b_enable_mprv_s();
    TEST_SETUP_EXCEPT();
    sd(refault_vaddr, recovery_store_value);
    TEST_ASSERT(
        "after the pair-A 3B+5B fault then pair-B 3B+5B refault sequence is "
        "repaired, the same-template other-pair 3B+5B recovery store must "
        "complete without reusing pair A's split template or stale "
        "high-half paddr",
        excpt.triggered == false);

    goto_priv(PRIV_M);
    TEST_ASSERT(
        "the fault-pair 3B+5B fault then other-pair same-template 3B+5B "
        "refault/recovery sequence must keep pair A unchanged "
        "while pair B reflects only the current 3B+5B overlay",
        read64(fault_low_phys) == expected_fault_low_after_fault &&
        read64(fault_high_phys) == fault_high_seed &&
        read64(refault_low_phys) == expected_refault_low_after_recovery &&
        read64(refault_high_phys) == expected_refault_high_after_recovery);

    ai_micro_mab_b2b_reset_env();

    TEST_END(
        "ai_micro_m_crosspage_3b5b_store_fault_then_other_pair_3b5b_"
        "refault_then_3b5b_recovery_no_stale_split_template_corner");
}
