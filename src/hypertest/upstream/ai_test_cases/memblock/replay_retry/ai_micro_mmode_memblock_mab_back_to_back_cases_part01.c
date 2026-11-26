#include "ai_micro_mmode_memblock_mab_back_to_back_common.h"

bool ai_micro_m_crosspage_4b4b_load_fault_repair_back_to_back_load_no_stale_merge_corner()
{
    TEST_START();

    const enum test_page low_page = SWITCH1;
    const enum test_page high_page = SWITCH2;
    const uintptr_t vaddr = ai_micro_mab_b2b_cross_page_4b4b_vaddr(low_page);
    const uintptr_t second_half_vaddr = vaddr + 4U;
    const uintptr_t low_phys = ai_micro_mab_b2b_cross_page_low_phys(low_page);
    const uintptr_t high_phys =
        ai_micro_mab_b2b_cross_page_high_phys(high_page);
    const uintptr_t deny_start = phys_page_base(high_page);
    const uintptr_t deny_end = deny_start + PAGE_SIZE;

    const uint64_t low_pattern_a  = 0x1122334455667788ULL;
    const uint64_t high_pattern_a = 0xaabbccddeeff0011ULL;
    const uint64_t low_pattern_b  = 0xfedcba9876543210ULL;
    const uint64_t high_pattern_b = 0x0123456789abcdefULL;

    const uint64_t expected_a =
        ai_micro_mab_b2b_expected_4b4b_load(low_pattern_a, high_pattern_a);
    const uint64_t expected_b =
        ai_micro_mab_b2b_expected_4b4b_load(low_pattern_b, high_pattern_b);
    uint64_t observed = 0ULL;

    goto_priv(PRIV_M);
    ai_micro_mab_b2b_reset_env();
    write64(low_phys, low_pattern_a);
    write64(high_phys, high_pattern_a);

    ai_micro_mab_b2b_configure_pmp_window(
        deny_start, deny_end, AI_MICRO_MAB_B2B_PMP_TOR_X_ONLY);
    ai_micro_mab_b2b_prepare_supervisor_translation();
    ai_micro_mab_b2b_set_leaf(low_page, PTE_V | PTE_AD | PTE_RWX);
    ai_micro_mab_b2b_set_leaf(high_page, PTE_V | PTE_AD | PTE_RWX);
    sfence_vma();

    ai_micro_mab_b2b_enable_mprv_s();
    TEST_SETUP_EXCEPT();
    observed = ld(vaddr);
    TEST_ASSERT(
        "cross-page 4B+4B load must fault with LAF on the true second-half "
        "start address when the upper page loses PMP read permission",
        excpt.triggered == true &&
        excpt.cause == CAUSE_LAF &&
        excpt.tval == second_half_vaddr);

    goto_priv(PRIV_M);
    TEST_ASSERT(
        "the cross-page load access-fault probe must leave both boundary "
        "doublewords unchanged before same-address recovery",
        read64(low_phys) == low_pattern_a &&
        read64(high_phys) == high_pattern_a);

    ai_micro_mab_b2b_restore_pmp_full_access();

    ai_micro_mab_b2b_enable_mprv_s();
    TEST_SETUP_EXCEPT();
    observed = ld(vaddr);
    TEST_ASSERT(
        "after PMP repair the first recovery cross-page 4B+4B load must "
        "return the correct merged value from pattern-A backing data",
        excpt.triggered == false &&
        observed == expected_a);

    goto_priv(PRIV_M);
    write64(low_phys, low_pattern_b);
    write64(high_phys, high_pattern_b);

    ai_micro_mab_b2b_enable_mprv_s();
    TEST_SETUP_EXCEPT();
    observed = ld(vaddr);
    TEST_ASSERT(
        "the immediate back-to-back same-address cross-page 4B+4B load "
        "after the first recovery load must return the updated pattern-B "
        "merged value instead of stale splitLoadResp data from the "
        "previous load",
        excpt.triggered == false &&
        observed == expected_b);

    ai_micro_mab_b2b_reset_env();

    TEST_END(
        "ai_micro_m_crosspage_4b4b_load_fault_repair_back_to_back_"
        "load_no_stale_merge_corner");
}

bool ai_micro_m_crosspage_1b7b_fault_then_4b4b_recovery_no_stale_merge_template_corner()
{
    TEST_START();

    const enum test_page low_page = SWITCH1;
    const enum test_page high_page = SWITCH2;
    const uintptr_t fault_vaddr =
        ai_micro_mab_b2b_cross_page_vaddr(low_page, 1U);
    const uintptr_t fault_second_half_vaddr = fault_vaddr + 1U;
    const uintptr_t recovery_vaddr =
        ai_micro_mab_b2b_cross_page_vaddr(low_page, 4U);
    const uintptr_t low_phys = ai_micro_mab_b2b_cross_page_low_phys(low_page);
    const uintptr_t high_phys =
        ai_micro_mab_b2b_cross_page_high_phys(high_page);
    const uintptr_t deny_start = phys_page_base(high_page);
    const uintptr_t deny_end = deny_start + PAGE_SIZE;

    const uint64_t low_pattern = 0x8f7e6d5c4b3a2918ULL;
    const uint64_t high_pattern = 0x1021324354657687ULL;
    const uint64_t expected_recovery =
        ai_micro_mab_b2b_expected_cross_page_ld(
            low_pattern, high_pattern, 4U);
    uint64_t observed = 0ULL;

    goto_priv(PRIV_M);
    ai_micro_mab_b2b_reset_env();
    write64(low_phys, low_pattern);
    write64(high_phys, high_pattern);

    ai_micro_mab_b2b_configure_pmp_window(
        deny_start, deny_end, AI_MICRO_MAB_B2B_PMP_TOR_X_ONLY);
    ai_micro_mab_b2b_prepare_supervisor_translation();
    ai_micro_mab_b2b_set_leaf(low_page, PTE_V | PTE_AD | PTE_RWX);
    ai_micro_mab_b2b_set_leaf(high_page, PTE_V | PTE_AD | PTE_RWX);
    sfence_vma();

    ai_micro_mab_b2b_enable_mprv_s();
    TEST_SETUP_EXCEPT();
    observed = ld(fault_vaddr);
    TEST_ASSERT(
        "cross-page 1B+7B load must fault with LAF on the true upper-page "
        "start address when the upper page loses PMP read permission",
        excpt.triggered == true &&
        excpt.cause == CAUSE_LAF &&
        excpt.tval == fault_second_half_vaddr);

    goto_priv(PRIV_M);
    TEST_ASSERT(
        "the cross-page 1B+7B load fault probe must leave both boundary "
        "doublewords unchanged before the different-shape recovery load",
        read64(low_phys) == low_pattern &&
        read64(high_phys) == high_pattern);

    ai_micro_mab_b2b_restore_pmp_full_access();

    ai_micro_mab_b2b_enable_mprv_s();
    TEST_SETUP_EXCEPT();
    observed = ld(recovery_vaddr);
    TEST_ASSERT(
        "after the cross-page 1B+7B load fault episode is repaired, the "
        "immediate same-pair 4B+4B recovery load must return only its own "
        "current-width merged value instead of reusing the older split "
        "merge template",
        excpt.triggered == false &&
        observed == expected_recovery);

    goto_priv(PRIV_M);
    TEST_ASSERT(
        "the cross-page 1B+7B fault then 4B+4B recovery load sequence must "
        "not modify either backing doubleword",
        read64(low_phys) == low_pattern &&
        read64(high_phys) == high_pattern);

    ai_micro_mab_b2b_reset_env();

    TEST_END(
        "ai_micro_m_crosspage_1b7b_fault_then_4b4b_recovery_"
        "no_stale_merge_template_corner");
}

bool ai_micro_m_crosspage_4b4b_store_fault_repair_back_to_back_store_no_stale_split_corner()
{
    TEST_START();

    const enum test_page low_page = SWITCH1;
    const enum test_page high_page = SWITCH2;
    const uintptr_t vaddr = ai_micro_mab_b2b_cross_page_4b4b_vaddr(low_page);
    const uintptr_t second_half_vaddr = vaddr + 4U;
    const uintptr_t low_phys = ai_micro_mab_b2b_cross_page_low_phys(low_page);
    const uintptr_t high_phys =
        ai_micro_mab_b2b_cross_page_high_phys(high_page);
    const uintptr_t deny_start = phys_page_base(high_page);
    const uintptr_t deny_end = deny_start + PAGE_SIZE;

    const uint64_t low_seed  = 0xaaaaaaaaaaaaaaaaULL;
    const uint64_t high_seed = 0x5555555555555555ULL;
    const uint64_t fault_store_value    = 0x1111111122222222ULL;
    const uint64_t recovery_store_value = 0x3333333344444444ULL;
    const uint64_t followup_store_value = 0x5566778899aabbccULL;

    const uint64_t expected_low_after_fault = low_seed;
    const uint64_t expected_low_final =
        ai_micro_mab_b2b_expected_low_after_4b_store(
            ai_micro_mab_b2b_expected_low_after_4b_store(
                low_seed, recovery_store_value),
            followup_store_value);
    const uint64_t expected_high_final =
        ai_micro_mab_b2b_expected_high_after_4b_store(
            ai_micro_mab_b2b_expected_high_after_4b_store(
                high_seed, recovery_store_value),
            followup_store_value);

    goto_priv(PRIV_M);
    ai_micro_mab_b2b_reset_env();
    write64(low_phys, low_seed);
    write64(high_phys, high_seed);

    ai_micro_mab_b2b_configure_pmp_window(
        deny_start, deny_end, AI_MICRO_MAB_B2B_PMP_TOR_R_ONLY);
    ai_micro_mab_b2b_prepare_supervisor_translation();
    ai_micro_mab_b2b_set_leaf(low_page, PTE_V | PTE_AD | PTE_RWX);
    ai_micro_mab_b2b_set_leaf(high_page, PTE_V | PTE_AD | PTE_RWX);
    sfence_vma();

    ai_micro_mab_b2b_enable_mprv_s();
    TEST_SETUP_EXCEPT();
    sd(vaddr, fault_store_value);
    TEST_ASSERT(
        "cross-page 4B+4B store must fault with SAF on the true second-half "
        "start address when the upper page loses PMP write permission",
        excpt.triggered == true &&
        excpt.cause == CAUSE_SAF &&
        excpt.tval == second_half_vaddr);

    goto_priv(PRIV_M);
    TEST_ASSERT(
        "the cross-page store access-fault probe must not expose any "
        "low-half side effect before same-address recovery",
        read64(low_phys) == expected_low_after_fault &&
        read64(high_phys) == high_seed);

    ai_micro_mab_b2b_restore_pmp_full_access();

    ai_micro_mab_b2b_enable_mprv_s();
    TEST_SETUP_EXCEPT();
    sd(vaddr, recovery_store_value);
    TEST_ASSERT(
        "after PMP repair the first recovery cross-page 4B+4B store must "
        "complete without exception",
        excpt.triggered == false);

    ai_micro_mab_b2b_enable_mprv_s();
    TEST_SETUP_EXCEPT();
    sd(vaddr, followup_store_value);
    TEST_ASSERT(
        "the immediate back-to-back same-address cross-page 4B+4B store "
        "must "
        "also complete without exception instead of inheriting stale "
        "splitStoreResp state from the previous store",
        excpt.triggered == false);

    goto_priv(PRIV_M);
    TEST_ASSERT(
        "after the fault-repair-recovery-followup sequence the final "
        "backing memory must reflect only the last follow-up store "
        "overlay on both low and high doublewords",
        read64(low_phys) == expected_low_final &&
        read64(high_phys) == expected_high_final);

    ai_micro_mab_b2b_reset_env();

    TEST_END(
        "ai_micro_m_crosspage_4b4b_store_fault_repair_back_to_back_"
        "store_no_stale_split_corner");
}

bool ai_micro_m_crosspage_1b7b_store_fault_then_4b4b_recovery_no_stale_split_template_corner()
{
    TEST_START();

    const enum test_page low_page = SWITCH1;
    const enum test_page high_page = SWITCH2;
    const uintptr_t fault_vaddr =
        ai_micro_mab_b2b_cross_page_vaddr(low_page, 1U);
    const uintptr_t fault_second_half_vaddr = fault_vaddr + 1U;
    const uintptr_t recovery_vaddr =
        ai_micro_mab_b2b_cross_page_vaddr(low_page, 4U);
    const uintptr_t low_phys = ai_micro_mab_b2b_cross_page_low_phys(low_page);
    const uintptr_t high_phys =
        ai_micro_mab_b2b_cross_page_high_phys(high_page);
    const uintptr_t deny_start = phys_page_base(high_page);
    const uintptr_t deny_end = deny_start + PAGE_SIZE;

    const uint64_t low_seed = 0xd4c3b2a1908f7e6dULL;
    const uint64_t high_seed = 0x1a2b3c4d5e6f7081ULL;
    const uint64_t fault_store_value = 0x8f7e6d5c4b3a2918ULL;
    const uint64_t recovery_store_value = 0x1021324354657687ULL;

    const uint64_t expected_low_after_fault = low_seed;
    const uint64_t expected_low_after_recovery =
        ai_micro_mab_b2b_expected_low_after_cross_page_store(
            expected_low_after_fault, recovery_store_value, 4U);
    const uint64_t expected_high_after_recovery =
        ai_micro_mab_b2b_expected_high_after_cross_page_store(
            high_seed, recovery_store_value, 4U);

    goto_priv(PRIV_M);
    ai_micro_mab_b2b_reset_env();
    write64(low_phys, low_seed);
    write64(high_phys, high_seed);

    ai_micro_mab_b2b_configure_pmp_window(
        deny_start, deny_end, AI_MICRO_MAB_B2B_PMP_TOR_R_ONLY);
    ai_micro_mab_b2b_prepare_supervisor_translation();
    ai_micro_mab_b2b_set_leaf(low_page, PTE_V | PTE_AD | PTE_RWX);
    ai_micro_mab_b2b_set_leaf(high_page, PTE_V | PTE_AD | PTE_RWX);
    sfence_vma();

    ai_micro_mab_b2b_enable_mprv_s();
    TEST_SETUP_EXCEPT();
    sd(fault_vaddr, fault_store_value);
    TEST_ASSERT(
        "cross-page 1B+7B store must fault with SAF on the true upper-page "
        "start address when the upper page loses PMP write permission",
        excpt.triggered == true &&
        excpt.cause == CAUSE_SAF &&
        excpt.tval == fault_second_half_vaddr);

    goto_priv(PRIV_M);
    TEST_ASSERT(
        "the cross-page 1B+7B store fault probe must not expose any low-page "
        "tail byte before the different-shape recovery store",
        read64(low_phys) == expected_low_after_fault &&
        read64(high_phys) == high_seed);

    ai_micro_mab_b2b_restore_pmp_full_access();

    ai_micro_mab_b2b_enable_mprv_s();
    TEST_SETUP_EXCEPT();
    sd(recovery_vaddr, recovery_store_value);
    TEST_ASSERT(
        "after the cross-page 1B+7B store fault episode is repaired, the "
        "immediate same page-pair 4B+4B recovery store must complete without "
        "reusing the older split template or stale high-half paddr state",
        excpt.triggered == false);

    goto_priv(PRIV_M);
    TEST_ASSERT(
        "the cross-page 1B+7B store fault then 4B+4B recovery store sequence "
        "must leave only the current 4B+4B overlay on both boundary "
        "doublewords",
        read64(low_phys) == expected_low_after_recovery &&
        read64(high_phys) == expected_high_after_recovery);

    ai_micro_mab_b2b_reset_env();

    TEST_END(
        "ai_micro_m_crosspage_1b7b_store_fault_then_4b4b_recovery_"
        "no_stale_split_template_corner");
}

bool ai_micro_m_crosspage_4b4b_store_fault_then_other_pair_1b7b_recovery_no_stale_split_template_corner()
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

    const uint64_t fault_low_seed = 0x7162534435261708ULL;
    const uint64_t fault_high_seed = 0x8899aabbccddeeffULL;
    const uint64_t recovery_low_seed = 0xf0e1d2c3b4a59687ULL;
    const uint64_t recovery_high_seed = 0x1021324354657687ULL;
    const uint64_t fault_store_value = 0x1122334455667788ULL;
    const uint64_t recovery_store_value = 0x8f7e6d5c4b3a2918ULL;

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
        "cross-page 4B+4B store must fault with SAF on the true second-half "
        "start address when the fault pair upper page loses PMP write "
        "permission",
        excpt.triggered == true &&
        excpt.cause == CAUSE_SAF &&
        excpt.tval == fault_second_half_vaddr);

    goto_priv(PRIV_M);
    TEST_ASSERT(
        "the cross-page 4B+4B fault episode must not expose the low-half "
        "side effect on the fault pair and keep the later recovery pair "
        "untouched before the other-pair recovery store",
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
        "other-pair 1B+7B recovery store must complete without reusing the "
        "older split template or stale high-half paddr from the fault pair",
        excpt.triggered == false);

    goto_priv(PRIV_M);
    TEST_ASSERT(
        "the fault-pair 4B+4B store fault then other-pair 1B+7B recovery "
        "sequence must keep the older fault pair unchanged while the recovery pair reflects only the current 1B+7B "
        "overlay",
        read64(fault_low_phys) == expected_fault_low_after_fault &&
        read64(fault_high_phys) == fault_high_seed &&
        read64(recovery_low_phys) == expected_recovery_low_after_store &&
        read64(recovery_high_phys) == expected_recovery_high_after_store);

    ai_micro_mab_b2b_reset_env();

    TEST_END(
        "ai_micro_m_crosspage_4b4b_store_fault_then_other_pair_1b7b_"
        "recovery_no_stale_split_template_corner");
}

bool ai_micro_m_crosspage_4b4b_load_fault_then_other_pair_1b7b_recovery_no_stale_merge_template_corner()
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

    const uint64_t fault_low_pattern = 0x7162534435261708ULL;
    const uint64_t fault_high_pattern = 0x8899aabbccddeeffULL;
    const uint64_t recovery_low_pattern = 0xf0e1d2c3b4a59687ULL;
    const uint64_t recovery_high_pattern = 0x1021324354657687ULL;
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
        "cross-page 4B+4B load must fault with LAF on the true second-half "
        "start address when the fault pair upper page loses PMP read "
        "permission",
        excpt.triggered == true &&
        excpt.cause == CAUSE_LAF &&
        excpt.tval == fault_second_half_vaddr);

    goto_priv(PRIV_M);
    TEST_ASSERT(
        "the cross-page 4B+4B fault episode must keep both the fault pair "
        "and the later recovery pair unchanged before the other-pair "
        "recovery load",
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
        "other-pair 1B+7B recovery load must return only the current merged "
        "payload instead of reusing the older splitLoadResp data or merge "
        "template from the fault pair",
        excpt.triggered == false &&
        observed == expected_recovery);

    goto_priv(PRIV_M);
    TEST_ASSERT(
        "the fault-pair 4B+4B load fault then other-pair 1B+7B recovery "
        "sequence must leave all four backing doublewords unchanged",
        read64(fault_low_phys) == fault_low_pattern &&
        read64(fault_high_phys) == fault_high_pattern &&
        read64(recovery_low_phys) == recovery_low_pattern &&
        read64(recovery_high_phys) == recovery_high_pattern);

    ai_micro_mab_b2b_reset_env();

    TEST_END(
        "ai_micro_m_crosspage_4b4b_load_fault_then_other_pair_1b7b_"
        "recovery_no_stale_merge_template_corner");
}

bool ai_micro_m_crosspage_1b7b_load_fault_then_other_pair_4b4b_recovery_no_stale_merge_template_corner()
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

    const uint64_t fault_low_pattern = 0x8f7e6d5c4b3a2918ULL;
    const uint64_t fault_high_pattern = 0x1021324354657687ULL;
    const uint64_t recovery_low_pattern = 0x7162534435261708ULL;
    const uint64_t recovery_high_pattern = 0x8899aabbccddeeffULL;
    const uint64_t expected_recovery =
        ai_micro_mab_b2b_expected_cross_page_ld(
            recovery_low_pattern, recovery_high_pattern, 4U);
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
        "and the later recovery pair unchanged before the other-pair "
        "recovery load",
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
        "other-pair 4B+4B recovery load must return only the current merged "
        "payload instead of reusing the older splitLoadResp data or merge "
        "template from the fault pair",
        excpt.triggered == false &&
        observed == expected_recovery);

    goto_priv(PRIV_M);
    TEST_ASSERT(
        "the fault-pair 1B+7B load fault then other-pair 4B+4B recovery "
        "sequence must leave all four backing doublewords unchanged",
        read64(fault_low_phys) == fault_low_pattern &&
        read64(fault_high_phys) == fault_high_pattern &&
        read64(recovery_low_phys) == recovery_low_pattern &&
        read64(recovery_high_phys) == recovery_high_pattern);

    ai_micro_mab_b2b_reset_env();

    TEST_END(
        "ai_micro_m_crosspage_1b7b_load_fault_then_other_pair_4b4b_"
        "recovery_no_stale_merge_template_corner");
}

bool ai_micro_m_crosspage_1b7b_store_fault_then_other_pair_4b4b_recovery_no_stale_split_template_corner()
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

    const uint64_t fault_low_seed = 0xd4c3b2a1908f7e6dULL;
    const uint64_t fault_high_seed = 0x1a2b3c4d5e6f7081ULL;
    const uint64_t recovery_low_seed = 0x7162534435261708ULL;
    const uint64_t recovery_high_seed = 0x8899aabbccddeeffULL;
    const uint64_t fault_store_value = 0x8f7e6d5c4b3a2918ULL;
    const uint64_t recovery_store_value = 0x1122334455667788ULL;

    const uint64_t expected_fault_low_after_fault = fault_low_seed;
    const uint64_t expected_recovery_low_after_store =
        ai_micro_mab_b2b_expected_low_after_cross_page_store(
            recovery_low_seed, recovery_store_value, 4U);
    const uint64_t expected_recovery_high_after_store =
        ai_micro_mab_b2b_expected_high_after_cross_page_store(
            recovery_high_seed, recovery_store_value, 4U);

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
        "tail-byte side effect on the fault pair and keep the later recovery "
        "pair untouched before the other-pair recovery store",
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
        "other-pair 4B+4B recovery store must complete without reusing the "
        "older split template or stale high-half paddr from the fault pair",
        excpt.triggered == false);

    goto_priv(PRIV_M);
    TEST_ASSERT(
        "the fault-pair 1B+7B store fault then other-pair 4B+4B recovery "
        "sequence must keep the older fault pair unchanged while the recovery pair reflects only the current 4B+4B "
        "overlay",
        read64(fault_low_phys) == expected_fault_low_after_fault &&
        read64(fault_high_phys) == fault_high_seed &&
        read64(recovery_low_phys) == expected_recovery_low_after_store &&
        read64(recovery_high_phys) == expected_recovery_high_after_store);

    ai_micro_mab_b2b_reset_env();

    TEST_END(
        "ai_micro_m_crosspage_1b7b_store_fault_then_other_pair_4b4b_"
        "recovery_no_stale_split_template_corner");
}

bool ai_micro_m_crosspage_4b4b_fault_then_1b7b_recovery_no_stale_merge_template_corner()
{
    TEST_START();

    const enum test_page low_page = SWITCH1;
    const enum test_page high_page = SWITCH2;
    const uintptr_t fault_vaddr =
        ai_micro_mab_b2b_cross_page_vaddr(low_page, 4U);
    const uintptr_t fault_second_half_vaddr = fault_vaddr + 4U;
    const uintptr_t recovery_vaddr =
        ai_micro_mab_b2b_cross_page_vaddr(low_page, 1U);
    const uintptr_t low_phys = ai_micro_mab_b2b_cross_page_low_phys(low_page);
    const uintptr_t high_phys =
        ai_micro_mab_b2b_cross_page_high_phys(high_page);
    const uintptr_t deny_start = phys_page_base(high_page);
    const uintptr_t deny_end = deny_start + PAGE_SIZE;

    const uint64_t low_pattern = 0x7162534435261708ULL;
    const uint64_t high_pattern = 0x8899aabbccddeeffULL;
    const uint64_t expected_recovery =
        ai_micro_mab_b2b_expected_cross_page_ld(
            low_pattern, high_pattern, 1U);
    uint64_t observed = 0ULL;

    goto_priv(PRIV_M);
    ai_micro_mab_b2b_reset_env();
    write64(low_phys, low_pattern);
    write64(high_phys, high_pattern);

    ai_micro_mab_b2b_configure_pmp_window(
        deny_start, deny_end, AI_MICRO_MAB_B2B_PMP_TOR_X_ONLY);
    ai_micro_mab_b2b_prepare_supervisor_translation();
    ai_micro_mab_b2b_set_leaf(low_page, PTE_V | PTE_AD | PTE_RWX);
    ai_micro_mab_b2b_set_leaf(high_page, PTE_V | PTE_AD | PTE_RWX);
    sfence_vma();

    ai_micro_mab_b2b_enable_mprv_s();
    TEST_SETUP_EXCEPT();
    observed = ld(fault_vaddr);
    TEST_ASSERT(
        "cross-page 4B+4B load must fault with LAF on the true second-half "
        "start address when the upper page loses PMP read permission",
        excpt.triggered == true &&
        excpt.cause == CAUSE_LAF &&
        excpt.tval == fault_second_half_vaddr);

    goto_priv(PRIV_M);
    TEST_ASSERT(
        "the cross-page 4B+4B fault episode must leave both boundary "
        "doublewords unchanged before the same-pair 1B+7B recovery load",
        read64(low_phys) == low_pattern &&
        read64(high_phys) == high_pattern);

    ai_micro_mab_b2b_restore_pmp_full_access();

    ai_micro_mab_b2b_enable_mprv_s();
    TEST_SETUP_EXCEPT();
    observed = ld(recovery_vaddr);
    TEST_ASSERT(
        "after the cross-page 4B+4B load fault episode is repaired, the "
        "immediate same page-pair 1B+7B recovery load must return only the "
        "current merged payload instead of reusing the older splitLoadResp "
        "data or merge template",
        excpt.triggered == false &&
        observed == expected_recovery);

    goto_priv(PRIV_M);
    TEST_ASSERT(
        "the cross-page 4B+4B fault then 1B+7B recovery load sequence must "
        "not modify either backing doubleword",
        read64(low_phys) == low_pattern &&
        read64(high_phys) == high_pattern);

    ai_micro_mab_b2b_reset_env();

    TEST_END(
        "ai_micro_m_crosspage_4b4b_fault_then_1b7b_recovery_"
        "no_stale_merge_template_corner");
}

bool ai_micro_m_crosspage_4b4b_store_fault_then_1b7b_recovery_no_stale_split_template_corner()
{
    TEST_START();

    const enum test_page low_page = SWITCH1;
    const enum test_page high_page = SWITCH2;
    const uintptr_t fault_vaddr =
        ai_micro_mab_b2b_cross_page_vaddr(low_page, 4U);
    const uintptr_t fault_second_half_vaddr = fault_vaddr + 4U;
    const uintptr_t recovery_vaddr =
        ai_micro_mab_b2b_cross_page_vaddr(low_page, 1U);
    const uintptr_t low_phys = ai_micro_mab_b2b_cross_page_low_phys(low_page);
    const uintptr_t high_phys =
        ai_micro_mab_b2b_cross_page_high_phys(high_page);
    const uintptr_t deny_start = phys_page_base(high_page);
    const uintptr_t deny_end = deny_start + PAGE_SIZE;

    const uint64_t low_seed = 0x7162534435261708ULL;
    const uint64_t high_seed = 0x8899aabbccddeeffULL;
    const uint64_t fault_store_value = 0x1122334455667788ULL;
    const uint64_t recovery_store_value = 0x8f7e6d5c4b3a2918ULL;

    const uint64_t expected_low_after_fault = low_seed;
    const uint64_t expected_low_after_recovery =
        ai_micro_mab_b2b_expected_low_after_cross_page_store(
            expected_low_after_fault, recovery_store_value, 1U);
    const uint64_t expected_high_after_recovery =
        ai_micro_mab_b2b_expected_high_after_cross_page_store(
            high_seed, recovery_store_value, 1U);

    goto_priv(PRIV_M);
    ai_micro_mab_b2b_reset_env();
    write64(low_phys, low_seed);
    write64(high_phys, high_seed);

    ai_micro_mab_b2b_configure_pmp_window(
        deny_start, deny_end, AI_MICRO_MAB_B2B_PMP_TOR_R_ONLY);
    ai_micro_mab_b2b_prepare_supervisor_translation();
    ai_micro_mab_b2b_set_leaf(low_page, PTE_V | PTE_AD | PTE_RWX);
    ai_micro_mab_b2b_set_leaf(high_page, PTE_V | PTE_AD | PTE_RWX);
    sfence_vma();

    ai_micro_mab_b2b_enable_mprv_s();
    TEST_SETUP_EXCEPT();
    sd(fault_vaddr, fault_store_value);
    TEST_ASSERT(
        "cross-page 4B+4B store must fault with SAF on the true second-half "
        "start address when the upper page loses PMP write permission",
        excpt.triggered == true &&
        excpt.cause == CAUSE_SAF &&
        excpt.tval == fault_second_half_vaddr);

    goto_priv(PRIV_M);
    TEST_ASSERT(
        "the cross-page 4B+4B fault episode must not expose the low-half "
        "side effect before the same-pair 1B+7B recovery store",
        read64(low_phys) == expected_low_after_fault &&
        read64(high_phys) == high_seed);

    ai_micro_mab_b2b_restore_pmp_full_access();

    ai_micro_mab_b2b_enable_mprv_s();
    TEST_SETUP_EXCEPT();
    sd(recovery_vaddr, recovery_store_value);
    TEST_ASSERT(
        "after the cross-page 4B+4B store fault episode is repaired, the "
        "immediate same page-pair 1B+7B recovery store must complete "
        "without reusing the older split template or stale high-half paddr",
        excpt.triggered == false);

    goto_priv(PRIV_M);
    TEST_ASSERT(
        "the cross-page 4B+4B fault then 1B+7B recovery store sequence must "
        "leave only the current 1B+7B overlay on the same page pair",
        read64(low_phys) == expected_low_after_recovery &&
        read64(high_phys) == expected_high_after_recovery);

    ai_micro_mab_b2b_reset_env();

    TEST_END(
        "ai_micro_m_crosspage_4b4b_store_fault_then_1b7b_recovery_"
        "no_stale_split_template_corner");
}

bool ai_micro_m_crosspage_4b4b_fault_then_1b7b_refault_then_1b7b_recovery_no_stale_merge_template_corner()
{
    TEST_START();

    const enum test_page low_page = SWITCH1;
    const enum test_page high_page = SWITCH2;
    const uintptr_t fault_vaddr =
        ai_micro_mab_b2b_cross_page_vaddr(low_page, 4U);
    const uintptr_t fault_second_half_vaddr = fault_vaddr + 4U;
    const uintptr_t refault_vaddr =
        ai_micro_mab_b2b_cross_page_vaddr(low_page, 1U);
    const uintptr_t refault_second_half_vaddr = refault_vaddr + 1U;
    const uintptr_t low_phys = ai_micro_mab_b2b_cross_page_low_phys(low_page);
    const uintptr_t high_phys =
        ai_micro_mab_b2b_cross_page_high_phys(high_page);
    const uintptr_t deny_start = phys_page_base(high_page);
    const uintptr_t deny_end = deny_start + PAGE_SIZE;

    const uint64_t first_low_pattern = 0x7162534435261708ULL;
    const uint64_t first_high_pattern = 0x8899aabbccddeeffULL;
    const uint64_t second_low_pattern = 0xf0e1d2c3b4a59687ULL;
    const uint64_t second_high_pattern = 0x1021324354657687ULL;
    const uint64_t expected_recovery =
        ai_micro_mab_b2b_expected_cross_page_ld(
            second_low_pattern, second_high_pattern, 1U);
    uint64_t observed = 0ULL;

    goto_priv(PRIV_M);
    ai_micro_mab_b2b_reset_env();
    write64(low_phys, first_low_pattern);
    write64(high_phys, first_high_pattern);

    ai_micro_mab_b2b_configure_pmp_window(
        deny_start, deny_end, AI_MICRO_MAB_B2B_PMP_TOR_X_ONLY);
    ai_micro_mab_b2b_prepare_supervisor_translation();
    ai_micro_mab_b2b_set_leaf(low_page, PTE_V | PTE_AD | PTE_RWX);
    ai_micro_mab_b2b_set_leaf(high_page, PTE_V | PTE_AD | PTE_RWX);
    sfence_vma();

    ai_micro_mab_b2b_enable_mprv_s();
    TEST_SETUP_EXCEPT();
    observed = ld(fault_vaddr);
    TEST_ASSERT(
        "the first cross-page 4B+4B load must fault with LAF on the true "
        "second-half start address when the upper page loses PMP read "
        "permission",
        excpt.triggered == true &&
        excpt.cause == CAUSE_LAF &&
        excpt.tval == fault_second_half_vaddr);

    goto_priv(PRIV_M);
    TEST_ASSERT(
        "the first cross-page 4B+4B fault episode must leave both boundary "
        "doublewords unchanged before the narrower refault",
        read64(low_phys) == first_low_pattern &&
        read64(high_phys) == first_high_pattern);

    write64(low_phys, second_low_pattern);
    write64(high_phys, second_high_pattern);

    ai_micro_mab_b2b_enable_mprv_s();
    TEST_SETUP_EXCEPT();
    observed = ld(refault_vaddr);
    TEST_ASSERT(
        "after a prior 4B+4B fault, the immediate same-pair 1B+7B refault "
        "must report LAF on the true upper-page start address instead of "
        "reusing the older 4B+4B fault template",
        excpt.triggered == true &&
        excpt.cause == CAUSE_LAF &&
        excpt.tval == refault_second_half_vaddr);

    goto_priv(PRIV_M);
    TEST_ASSERT(
        "the narrower 1B+7B refault probe must still leave the refreshed "
        "backing doublewords unchanged before the repaired recovery load",
        read64(low_phys) == second_low_pattern &&
        read64(high_phys) == second_high_pattern);

    ai_micro_mab_b2b_restore_pmp_full_access();

    ai_micro_mab_b2b_enable_mprv_s();
    TEST_SETUP_EXCEPT();
    observed = ld(refault_vaddr);
    TEST_ASSERT(
        "after the 4B+4B fault then 1B+7B refault sequence is repaired, the "
        "same-pair 1B+7B recovery load must return only the current merged "
        "payload instead of stale splitLoadResp data or the older 4B+4B "
        "merge template",
        excpt.triggered == false &&
        observed == expected_recovery);

    goto_priv(PRIV_M);
    TEST_ASSERT(
        "the 4B+4B fault then 1B+7B refault/recovery load sequence must not "
        "modify either backing doubleword",
        read64(low_phys) == second_low_pattern &&
        read64(high_phys) == second_high_pattern);

    ai_micro_mab_b2b_reset_env();

    TEST_END(
        "ai_micro_m_crosspage_4b4b_fault_then_1b7b_refault_then_1b7b_"
        "recovery_no_stale_merge_template_corner");
}

bool ai_micro_m_crosspage_4b4b_store_fault_then_1b7b_refault_then_1b7b_recovery_no_stale_split_template_corner()
{
    TEST_START();

    const enum test_page low_page = SWITCH1;
    const enum test_page high_page = SWITCH2;
    const uintptr_t fault_vaddr =
        ai_micro_mab_b2b_cross_page_vaddr(low_page, 4U);
    const uintptr_t fault_second_half_vaddr = fault_vaddr + 4U;
    const uintptr_t refault_vaddr =
        ai_micro_mab_b2b_cross_page_vaddr(low_page, 1U);
    const uintptr_t refault_second_half_vaddr = refault_vaddr + 1U;
    const uintptr_t low_phys = ai_micro_mab_b2b_cross_page_low_phys(low_page);
    const uintptr_t high_phys =
        ai_micro_mab_b2b_cross_page_high_phys(high_page);
    const uintptr_t deny_start = phys_page_base(high_page);
    const uintptr_t deny_end = deny_start + PAGE_SIZE;

    const uint64_t first_low_seed = 0x7162534435261708ULL;
    const uint64_t first_high_seed = 0x8899aabbccddeeffULL;
    const uint64_t second_low_seed = 0xf0e1d2c3b4a59687ULL;
    const uint64_t second_high_seed = 0x1021324354657687ULL;
    const uint64_t fault_store_value = 0x1122334455667788ULL;
    const uint64_t refault_store_value = 0x8f7e6d5c4b3a2918ULL;
    const uint64_t recovery_store_value = 0x31425364758697a8ULL;

    const uint64_t expected_low_after_first_fault = first_low_seed;
    const uint64_t expected_low_after_refault = second_low_seed;
    const uint64_t expected_low_after_recovery =
        ai_micro_mab_b2b_expected_low_after_cross_page_store(
            second_low_seed, recovery_store_value, 1U);
    const uint64_t expected_high_after_recovery =
        ai_micro_mab_b2b_expected_high_after_cross_page_store(
            second_high_seed, recovery_store_value, 1U);

    goto_priv(PRIV_M);
    ai_micro_mab_b2b_reset_env();
    write64(low_phys, first_low_seed);
    write64(high_phys, first_high_seed);

    ai_micro_mab_b2b_configure_pmp_window(
        deny_start, deny_end, AI_MICRO_MAB_B2B_PMP_TOR_R_ONLY);
    ai_micro_mab_b2b_prepare_supervisor_translation();
    ai_micro_mab_b2b_set_leaf(low_page, PTE_V | PTE_AD | PTE_RWX);
    ai_micro_mab_b2b_set_leaf(high_page, PTE_V | PTE_AD | PTE_RWX);
    sfence_vma();

    ai_micro_mab_b2b_enable_mprv_s();
    TEST_SETUP_EXCEPT();
    sd(fault_vaddr, fault_store_value);
    TEST_ASSERT(
        "the first cross-page 4B+4B store must fault with SAF on the true "
        "second-half start address when the upper page loses PMP write "
        "permission",
        excpt.triggered == true &&
        excpt.cause == CAUSE_SAF &&
        excpt.tval == fault_second_half_vaddr);

    goto_priv(PRIV_M);
    TEST_ASSERT(
        "the first cross-page 4B+4B fault episode must not expose the low-half side effect before the narrower refault",
        read64(low_phys) == expected_low_after_first_fault &&
        read64(high_phys) == first_high_seed);

    write64(low_phys, second_low_seed);
    write64(high_phys, second_high_seed);

    ai_micro_mab_b2b_enable_mprv_s();
    TEST_SETUP_EXCEPT();
    sd(refault_vaddr, refault_store_value);
    TEST_ASSERT(
        "after a prior 4B+4B fault, the immediate same-pair 1B+7B refault "
        "must report SAF on the true upper-page start address instead of "
        "reusing the older 4B+4B split template",
        excpt.triggered == true &&
        excpt.cause == CAUSE_SAF &&
        excpt.tval == refault_second_half_vaddr);

    goto_priv(PRIV_M);
    TEST_ASSERT(
        "the narrower 1B+7B refault probe must not expose the current low-page tail-byte side effect on the refreshed backing image",
        read64(low_phys) == expected_low_after_refault &&
        read64(high_phys) == second_high_seed);

    ai_micro_mab_b2b_restore_pmp_full_access();

    ai_micro_mab_b2b_enable_mprv_s();
    TEST_SETUP_EXCEPT();
    sd(refault_vaddr, recovery_store_value);
    TEST_ASSERT(
        "after the 4B+4B fault then 1B+7B refault sequence is repaired, the "
        "same-pair 1B+7B recovery store must complete without reusing the "
        "older split template or stale high-half paddr",
        excpt.triggered == false);

    goto_priv(PRIV_M);
    TEST_ASSERT(
        "the 4B+4B fault then 1B+7B refault/recovery store sequence must "
        "leave only the current 1B+7B overlay on the refreshed backing "
        "image",
        read64(low_phys) == expected_low_after_recovery &&
        read64(high_phys) == expected_high_after_recovery);

    ai_micro_mab_b2b_reset_env();

    TEST_END(
        "ai_micro_m_crosspage_4b4b_store_fault_then_1b7b_refault_then_1b7b_"
        "recovery_no_stale_split_template_corner");
}

bool ai_micro_m_crosspage_1b7b_fault_then_4b4b_refault_then_4b4b_recovery_no_stale_merge_template_corner()
{
    TEST_START();

    const enum test_page low_page = SWITCH1;
    const enum test_page high_page = SWITCH2;
    const uintptr_t fault_vaddr =
        ai_micro_mab_b2b_cross_page_vaddr(low_page, 1U);
    const uintptr_t fault_second_half_vaddr = fault_vaddr + 1U;
    const uintptr_t refault_vaddr =
        ai_micro_mab_b2b_cross_page_vaddr(low_page, 4U);
    const uintptr_t refault_second_half_vaddr = refault_vaddr + 4U;
    const uintptr_t low_phys = ai_micro_mab_b2b_cross_page_low_phys(low_page);
    const uintptr_t high_phys =
        ai_micro_mab_b2b_cross_page_high_phys(high_page);
    const uintptr_t deny_start = phys_page_base(high_page);
    const uintptr_t deny_end = deny_start + PAGE_SIZE;

    const uint64_t first_low_pattern = 0x8f7e6d5c4b3a2918ULL;
    const uint64_t first_high_pattern = 0x1021324354657687ULL;
    const uint64_t second_low_pattern = 0x7162534435261708ULL;
    const uint64_t second_high_pattern = 0x8899aabbccddeeffULL;
    const uint64_t expected_recovery =
        ai_micro_mab_b2b_expected_cross_page_ld(
            second_low_pattern, second_high_pattern, 4U);
    uint64_t observed = 0ULL;

    goto_priv(PRIV_M);
    ai_micro_mab_b2b_reset_env();
    write64(low_phys, first_low_pattern);
    write64(high_phys, first_high_pattern);

    ai_micro_mab_b2b_configure_pmp_window(
        deny_start, deny_end, AI_MICRO_MAB_B2B_PMP_TOR_X_ONLY);
    ai_micro_mab_b2b_prepare_supervisor_translation();
    ai_micro_mab_b2b_set_leaf(low_page, PTE_V | PTE_AD | PTE_RWX);
    ai_micro_mab_b2b_set_leaf(high_page, PTE_V | PTE_AD | PTE_RWX);
    sfence_vma();

    ai_micro_mab_b2b_enable_mprv_s();
    TEST_SETUP_EXCEPT();
    observed = ld(fault_vaddr);
    TEST_ASSERT(
        "the first cross-page 1B+7B load must fault with LAF on the true "
        "upper-page start address when the upper page loses PMP read "
        "permission",
        excpt.triggered == true &&
        excpt.cause == CAUSE_LAF &&
        excpt.tval == fault_second_half_vaddr);

    goto_priv(PRIV_M);
    TEST_ASSERT(
        "the first cross-page 1B+7B fault episode must leave both boundary "
        "doublewords unchanged before the wider refault",
        read64(low_phys) == first_low_pattern &&
        read64(high_phys) == first_high_pattern);

    write64(low_phys, second_low_pattern);
    write64(high_phys, second_high_pattern);

    ai_micro_mab_b2b_enable_mprv_s();
    TEST_SETUP_EXCEPT();
    observed = ld(refault_vaddr);
    TEST_ASSERT(
        "after a prior 1B+7B fault, the immediate same-pair 4B+4B refault "
        "must report LAF on the true second-half start address instead of "
        "reusing the older 1B+7B fault template",
        excpt.triggered == true &&
        excpt.cause == CAUSE_LAF &&
        excpt.tval == refault_second_half_vaddr);

    goto_priv(PRIV_M);
    TEST_ASSERT(
        "the wider 4B+4B refault probe must still leave the refreshed "
        "backing doublewords unchanged before the repaired recovery load",
        read64(low_phys) == second_low_pattern &&
        read64(high_phys) == second_high_pattern);

    ai_micro_mab_b2b_restore_pmp_full_access();

    ai_micro_mab_b2b_enable_mprv_s();
    TEST_SETUP_EXCEPT();
    observed = ld(refault_vaddr);
    TEST_ASSERT(
        "after the 1B+7B fault then 4B+4B refault sequence is repaired, the "
        "same-pair 4B+4B recovery load must return only the current merged "
        "payload instead of stale splitLoadResp data or the older 1B+7B "
        "merge template",
        excpt.triggered == false &&
        observed == expected_recovery);

    goto_priv(PRIV_M);
    TEST_ASSERT(
        "the 1B+7B fault then 4B+4B refault/recovery load sequence must not "
        "modify either backing doubleword",
        read64(low_phys) == second_low_pattern &&
        read64(high_phys) == second_high_pattern);

    ai_micro_mab_b2b_reset_env();

    TEST_END(
        "ai_micro_m_crosspage_1b7b_fault_then_4b4b_refault_then_4b4b_"
        "recovery_no_stale_merge_template_corner");
}

bool ai_micro_m_crosspage_1b7b_store_fault_then_4b4b_refault_then_4b4b_recovery_no_stale_split_template_corner()
{
    TEST_START();

    const enum test_page low_page = SWITCH1;
    const enum test_page high_page = SWITCH2;
    const uintptr_t fault_vaddr =
        ai_micro_mab_b2b_cross_page_vaddr(low_page, 1U);
    const uintptr_t fault_second_half_vaddr = fault_vaddr + 1U;
    const uintptr_t refault_vaddr =
        ai_micro_mab_b2b_cross_page_vaddr(low_page, 4U);
    const uintptr_t refault_second_half_vaddr = refault_vaddr + 4U;
    const uintptr_t low_phys = ai_micro_mab_b2b_cross_page_low_phys(low_page);
    const uintptr_t high_phys =
        ai_micro_mab_b2b_cross_page_high_phys(high_page);
    const uintptr_t deny_start = phys_page_base(high_page);
    const uintptr_t deny_end = deny_start + PAGE_SIZE;

    const uint64_t first_low_seed = 0xd4c3b2a1908f7e6dULL;
    const uint64_t first_high_seed = 0x1a2b3c4d5e6f7081ULL;
    const uint64_t second_low_seed = 0x7162534435261708ULL;
    const uint64_t second_high_seed = 0x8899aabbccddeeffULL;
    const uint64_t fault_store_value = 0x8f7e6d5c4b3a2918ULL;
    const uint64_t refault_store_value = 0x1122334455667788ULL;
    const uint64_t recovery_store_value = 0x31425364758697a8ULL;

    const uint64_t expected_low_after_first_fault = first_low_seed;
    const uint64_t expected_low_after_refault = second_low_seed;
    const uint64_t expected_low_after_recovery =
        ai_micro_mab_b2b_expected_low_after_4b_store(
            second_low_seed, recovery_store_value);
    const uint64_t expected_high_after_recovery =
        ai_micro_mab_b2b_expected_high_after_4b_store(
            second_high_seed, recovery_store_value);

    goto_priv(PRIV_M);
    ai_micro_mab_b2b_reset_env();
    write64(low_phys, first_low_seed);
    write64(high_phys, first_high_seed);

    ai_micro_mab_b2b_configure_pmp_window(
        deny_start, deny_end, AI_MICRO_MAB_B2B_PMP_TOR_R_ONLY);
    ai_micro_mab_b2b_prepare_supervisor_translation();
    ai_micro_mab_b2b_set_leaf(low_page, PTE_V | PTE_AD | PTE_RWX);
    ai_micro_mab_b2b_set_leaf(high_page, PTE_V | PTE_AD | PTE_RWX);
    sfence_vma();

    ai_micro_mab_b2b_enable_mprv_s();
    TEST_SETUP_EXCEPT();
    sd(fault_vaddr, fault_store_value);
    TEST_ASSERT(
        "the first cross-page 1B+7B store must fault with SAF on the true "
        "upper-page start address when the upper page loses PMP write "
        "permission",
        excpt.triggered == true &&
        excpt.cause == CAUSE_SAF &&
        excpt.tval == fault_second_half_vaddr);

    goto_priv(PRIV_M);
    TEST_ASSERT(
        "the first cross-page 1B+7B fault episode must not expose the low-page tail-byte side effect before the wider refault",
        read64(low_phys) == expected_low_after_first_fault &&
        read64(high_phys) == first_high_seed);

    write64(low_phys, second_low_seed);
    write64(high_phys, second_high_seed);

    ai_micro_mab_b2b_enable_mprv_s();
    TEST_SETUP_EXCEPT();
    sd(refault_vaddr, refault_store_value);
    TEST_ASSERT(
        "after a prior 1B+7B fault, the immediate same-pair 4B+4B refault "
        "must report SAF on the true second-half start address instead of "
        "reusing the older 1B+7B split template",
        excpt.triggered == true &&
        excpt.cause == CAUSE_SAF &&
        excpt.tval == refault_second_half_vaddr);

    goto_priv(PRIV_M);
    TEST_ASSERT(
        "the wider 4B+4B refault probe must not expose the current low-half "
        "side effect on the refreshed backing image",
        read64(low_phys) == expected_low_after_refault &&
        read64(high_phys) == second_high_seed);

    ai_micro_mab_b2b_restore_pmp_full_access();

    ai_micro_mab_b2b_enable_mprv_s();
    TEST_SETUP_EXCEPT();
    sd(refault_vaddr, recovery_store_value);
    TEST_ASSERT(
        "after the 1B+7B fault then 4B+4B refault sequence is repaired, the "
        "same-pair 4B+4B recovery store must complete without reusing the "
        "older split template or stale high-half paddr",
        excpt.triggered == false);

    goto_priv(PRIV_M);
    TEST_ASSERT(
        "the 1B+7B fault then 4B+4B refault/recovery store sequence must "
        "leave only the current 4B+4B overlay on the refreshed backing "
        "image",
        read64(low_phys) == expected_low_after_recovery &&
        read64(high_phys) == expected_high_after_recovery);

    ai_micro_mab_b2b_reset_env();

    TEST_END(
        "ai_micro_m_crosspage_1b7b_store_fault_then_4b4b_refault_then_4b4b_"
        "recovery_no_stale_split_template_corner");
}

bool ai_micro_m_crosspage_4b4b_fault_then_other_pair_1b7b_refault_then_1b7b_recovery_no_stale_merge_template_corner()
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
        ai_micro_mab_b2b_cross_page_vaddr(refault_low_page, 1U);
    const uintptr_t refault_second_half_vaddr = refault_vaddr + 1U;
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

    const uint64_t fault_low_pattern = 0x7162534435261708ULL;
    const uint64_t fault_high_pattern = 0x8899aabbccddeeffULL;
    const uint64_t refault_low_initial = 0xd4c3b2a1908f7e6dULL;
    const uint64_t refault_high_initial = 0x1a2b3c4d5e6f7081ULL;
    const uint64_t refault_low_pattern = 0xf0e1d2c3b4a59687ULL;
    const uint64_t refault_high_pattern = 0x1021324354657687ULL;
    const uint64_t expected_recovery =
        ai_micro_mab_b2b_expected_cross_page_ld(
            refault_low_pattern, refault_high_pattern, 1U);
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
        "refault load",
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
        "after a fault on pair A, the immediate other-pair 1B+7B refault "
        "must report LAF on pair B's true upper-page start address instead "
        "of reusing the older fault pair template",
        excpt.triggered == true &&
        excpt.cause == CAUSE_LAF &&
        excpt.tval == refault_second_half_vaddr);

    goto_priv(PRIV_M);
    TEST_ASSERT(
        "the other-pair 1B+7B refault probe must keep the older fault pair "
        "frozen and the refreshed refault pair unchanged before recovery",
        read64(fault_low_phys) == fault_low_pattern &&
        read64(fault_high_phys) == fault_high_pattern &&
        read64(refault_low_phys) == refault_low_pattern &&
        read64(refault_high_phys) == refault_high_pattern);

    ai_micro_mab_b2b_restore_pmp_full_access();

    ai_micro_mab_b2b_enable_mprv_s();
    TEST_SETUP_EXCEPT();
    observed = ld(refault_vaddr);
    TEST_ASSERT(
        "after the fault-pair 4B+4B fault then other-pair 1B+7B refault "
        "sequence is repaired, the other-pair 1B+7B recovery load must "
        "return only pair B's current merged payload instead of stale "
        "splitLoadResp data or merge template from pair A",
        excpt.triggered == false &&
        observed == expected_recovery);

    goto_priv(PRIV_M);
    TEST_ASSERT(
        "the fault-pair 4B+4B fault then other-pair 1B+7B refault/recovery "
        "sequence must leave all four backing doublewords unchanged after "
        "the recovery load",
        read64(fault_low_phys) == fault_low_pattern &&
        read64(fault_high_phys) == fault_high_pattern &&
        read64(refault_low_phys) == refault_low_pattern &&
        read64(refault_high_phys) == refault_high_pattern);

    ai_micro_mab_b2b_reset_env();

    TEST_END(
        "ai_micro_m_crosspage_4b4b_fault_then_other_pair_1b7b_refault_then_"
        "1b7b_recovery_no_stale_merge_template_corner");
}

bool ai_micro_m_crosspage_4b4b_store_fault_then_other_pair_1b7b_refault_then_1b7b_recovery_no_stale_split_template_corner()
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
        ai_micro_mab_b2b_cross_page_vaddr(refault_low_page, 1U);
    const uintptr_t refault_second_half_vaddr = refault_vaddr + 1U;
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

    const uint64_t fault_low_seed = 0x7162534435261708ULL;
    const uint64_t fault_high_seed = 0x8899aabbccddeeffULL;
    const uint64_t refault_low_initial = 0xd4c3b2a1908f7e6dULL;
    const uint64_t refault_high_initial = 0x1a2b3c4d5e6f7081ULL;
    const uint64_t refault_low_seed = 0xf0e1d2c3b4a59687ULL;
    const uint64_t refault_high_seed = 0x1021324354657687ULL;
    const uint64_t fault_store_value = 0x1122334455667788ULL;
    const uint64_t refault_store_value = 0x8f7e6d5c4b3a2918ULL;
    const uint64_t recovery_store_value = 0x31425364758697a8ULL;

    const uint64_t expected_fault_low_after_fault = fault_low_seed;
    const uint64_t expected_refault_low_after_refault = refault_low_seed;
    const uint64_t expected_refault_low_after_recovery =
        ai_micro_mab_b2b_expected_low_after_cross_page_store(
            refault_low_seed, recovery_store_value, 1U);
    const uint64_t expected_refault_high_after_recovery =
        ai_micro_mab_b2b_expected_high_after_cross_page_store(
            refault_high_seed, recovery_store_value, 1U);

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
        "untouched before the other-pair refault store",
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
        "after a fault on pair A, the immediate other-pair 1B+7B refault "
        "must report SAF on pair B's true upper-page start address instead "
        "of reusing the older fault pair split template",
        excpt.triggered == true &&
        excpt.cause == CAUSE_SAF &&
        excpt.tval == refault_second_half_vaddr);

    goto_priv(PRIV_M);
    TEST_ASSERT(
        "the other-pair 1B+7B refault probe must keep the older fault pair "
        "unchanged while pair B does not expose the current low-page tail-byte "
        "side effect before recovery",
        read64(fault_low_phys) == expected_fault_low_after_fault &&
        read64(fault_high_phys) == fault_high_seed &&
        read64(refault_low_phys) == expected_refault_low_after_refault &&
        read64(refault_high_phys) == refault_high_seed);

    ai_micro_mab_b2b_restore_pmp_full_access();

    ai_micro_mab_b2b_enable_mprv_s();
    TEST_SETUP_EXCEPT();
    sd(refault_vaddr, recovery_store_value);
    TEST_ASSERT(
        "after the fault-pair 4B+4B fault then other-pair 1B+7B refault "
        "sequence is repaired, the other-pair 1B+7B recovery store must "
        "complete without reusing pair A's split template or stale "
        "high-half paddr",
        excpt.triggered == false);

    goto_priv(PRIV_M);
    TEST_ASSERT(
        "the fault-pair 4B+4B fault then other-pair 1B+7B refault/recovery "
        "sequence must keep pair A unchanged by the faulting store while pair "
        "B reflects only the current 1B+7B overlay",
        read64(fault_low_phys) == expected_fault_low_after_fault &&
        read64(fault_high_phys) == fault_high_seed &&
        read64(refault_low_phys) == expected_refault_low_after_recovery &&
        read64(refault_high_phys) == expected_refault_high_after_recovery);

    ai_micro_mab_b2b_reset_env();

    TEST_END(
        "ai_micro_m_crosspage_4b4b_store_fault_then_other_pair_1b7b_"
        "refault_then_1b7b_recovery_no_stale_split_template_corner");
}

bool ai_micro_m_crosspage_1b7b_fault_then_other_pair_4b4b_refault_then_4b4b_recovery_no_stale_merge_template_corner()
{
    TEST_START();

    const enum test_page fault_low_page = SWITCH1;
    const enum test_page fault_high_page = SWITCH2;
    const enum test_page refault_low_page = VSRWX_GI;
    const enum test_page refault_high_page = VSRW_GI;
    const uintptr_t fault_vaddr =
        ai_micro_mab_b2b_cross_page_vaddr(fault_low_page, 1U);
    const uintptr_t fault_second_half_vaddr = fault_vaddr + 1U;
    const uintptr_t refault_vaddr =
        ai_micro_mab_b2b_cross_page_vaddr(refault_low_page, 4U);
    const uintptr_t refault_second_half_vaddr = refault_vaddr + 4U;
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

    const uint64_t fault_low_pattern = 0x8f7e6d5c4b3a2918ULL;
    const uint64_t fault_high_pattern = 0x1021324354657687ULL;
    const uint64_t refault_low_initial = 0xd4c3b2a1908f7e6dULL;
    const uint64_t refault_high_initial = 0x1a2b3c4d5e6f7081ULL;
    const uint64_t refault_low_pattern = 0x7162534435261708ULL;
    const uint64_t refault_high_pattern = 0x8899aabbccddeeffULL;
    const uint64_t expected_recovery =
        ai_micro_mab_b2b_expected_cross_page_ld(
            refault_low_pattern, refault_high_pattern, 4U);
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
        "the fault-pair cross-page 1B+7B load must fault with LAF on the "
        "true upper-page start address when the fault pair upper page "
        "loses PMP read permission",
        excpt.triggered == true &&
        excpt.cause == CAUSE_LAF &&
        excpt.tval == fault_second_half_vaddr);

    goto_priv(PRIV_M);
    TEST_ASSERT(
        "the fault-pair 1B+7B load fault episode must keep both the fault "
        "pair and the later refault pair unchanged before the other-pair "
        "refault load",
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
        "after a fault on pair A, the immediate other-pair 4B+4B refault "
        "must report LAF on pair B's true second-half start address instead "
        "of reusing the older fault pair template",
        excpt.triggered == true &&
        excpt.cause == CAUSE_LAF &&
        excpt.tval == refault_second_half_vaddr);

    goto_priv(PRIV_M);
    TEST_ASSERT(
        "the other-pair 4B+4B refault probe must keep the older fault pair "
        "frozen and the refreshed refault pair unchanged before recovery",
        read64(fault_low_phys) == fault_low_pattern &&
        read64(fault_high_phys) == fault_high_pattern &&
        read64(refault_low_phys) == refault_low_pattern &&
        read64(refault_high_phys) == refault_high_pattern);

    ai_micro_mab_b2b_restore_pmp_full_access();

    ai_micro_mab_b2b_enable_mprv_s();
    TEST_SETUP_EXCEPT();
    observed = ld(refault_vaddr);
    TEST_ASSERT(
        "after the fault-pair 1B+7B fault then other-pair 4B+4B refault "
        "sequence is repaired, the other-pair 4B+4B recovery load must "
        "return only pair B's current merged payload instead of stale "
        "splitLoadResp data or merge template from pair A",
        excpt.triggered == false &&
        observed == expected_recovery);

    goto_priv(PRIV_M);
    TEST_ASSERT(
        "the fault-pair 1B+7B fault then other-pair 4B+4B refault/recovery "
        "sequence must leave all four backing doublewords unchanged after "
        "the recovery load",
        read64(fault_low_phys) == fault_low_pattern &&
        read64(fault_high_phys) == fault_high_pattern &&
        read64(refault_low_phys) == refault_low_pattern &&
        read64(refault_high_phys) == refault_high_pattern);

    ai_micro_mab_b2b_reset_env();

    TEST_END(
        "ai_micro_m_crosspage_1b7b_fault_then_other_pair_4b4b_refault_then_"
        "4b4b_recovery_no_stale_merge_template_corner");
}

bool ai_micro_m_crosspage_1b7b_store_fault_then_other_pair_4b4b_refault_then_4b4b_recovery_no_stale_split_template_corner()
{
    TEST_START();

    const enum test_page fault_low_page = SWITCH1;
    const enum test_page fault_high_page = SWITCH2;
    const enum test_page refault_low_page = VSRWX_GI;
    const enum test_page refault_high_page = VSRW_GI;
    const uintptr_t fault_vaddr =
        ai_micro_mab_b2b_cross_page_vaddr(fault_low_page, 1U);
    const uintptr_t fault_second_half_vaddr = fault_vaddr + 1U;
    const uintptr_t refault_vaddr =
        ai_micro_mab_b2b_cross_page_vaddr(refault_low_page, 4U);
    const uintptr_t refault_second_half_vaddr = refault_vaddr + 4U;
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

    const uint64_t fault_low_seed = 0xd4c3b2a1908f7e6dULL;
    const uint64_t fault_high_seed = 0x1a2b3c4d5e6f7081ULL;
    const uint64_t refault_low_initial = 0xf0e1d2c3b4a59687ULL;
    const uint64_t refault_high_initial = 0x1021324354657687ULL;
    const uint64_t refault_low_seed = 0x7162534435261708ULL;
    const uint64_t refault_high_seed = 0x8899aabbccddeeffULL;
    const uint64_t fault_store_value = 0x8f7e6d5c4b3a2918ULL;
    const uint64_t refault_store_value = 0x1122334455667788ULL;
    const uint64_t recovery_store_value = 0x31425364758697a8ULL;

    const uint64_t expected_fault_low_after_fault = fault_low_seed;
    const uint64_t expected_refault_low_after_refault = refault_low_seed;
    const uint64_t expected_refault_low_after_recovery =
        ai_micro_mab_b2b_expected_low_after_4b_store(
            refault_low_seed, recovery_store_value);
    const uint64_t expected_refault_high_after_recovery =
        ai_micro_mab_b2b_expected_high_after_4b_store(
            refault_high_seed, recovery_store_value);

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
        "the fault-pair cross-page 1B+7B store must fault with SAF on the "
        "true upper-page start address when the fault pair upper page "
        "loses PMP write permission",
        excpt.triggered == true &&
        excpt.cause == CAUSE_SAF &&
        excpt.tval == fault_second_half_vaddr);

    goto_priv(PRIV_M);
    TEST_ASSERT(
        "the fault-pair 1B+7B store fault episode must not expose the fault pair tail-byte side effect and keep the later refault pair "
        "untouched before the other-pair refault store",
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
        "after a fault on pair A, the immediate other-pair 4B+4B refault "
        "must report SAF on pair B's true second-half start address instead "
        "of reusing the older fault pair split template",
        excpt.triggered == true &&
        excpt.cause == CAUSE_SAF &&
        excpt.tval == refault_second_half_vaddr);

    goto_priv(PRIV_M);
    TEST_ASSERT(
        "the other-pair 4B+4B refault probe must keep the older fault pair "
        "unchanged while pair B does not expose the current low-half side effect "
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
        "after the fault-pair 1B+7B fault then other-pair 4B+4B refault "
        "sequence is repaired, the other-pair 4B+4B recovery store must "
        "complete without reusing pair A's split template or stale "
        "high-half paddr",
        excpt.triggered == false);

    goto_priv(PRIV_M);
    TEST_ASSERT(
        "the fault-pair 1B+7B fault then other-pair 4B+4B refault/recovery "
        "sequence must keep pair A unchanged by the faulting store while pair "
        "B reflects only the current 4B+4B overlay",
        read64(fault_low_phys) == expected_fault_low_after_fault &&
        read64(fault_high_phys) == fault_high_seed &&
        read64(refault_low_phys) == expected_refault_low_after_recovery &&
        read64(refault_high_phys) == expected_refault_high_after_recovery);

    ai_micro_mab_b2b_reset_env();

    TEST_END(
        "ai_micro_m_crosspage_1b7b_store_fault_then_other_pair_4b4b_"
        "refault_then_4b4b_recovery_no_stale_split_template_corner");
}

bool ai_micro_m_crosspage_4b4b_fault_then_other_pair_4b4b_refault_then_4b4b_recovery_no_stale_merge_template_corner()
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
        ai_micro_mab_b2b_cross_page_vaddr(refault_low_page, 4U);
    const uintptr_t refault_second_half_vaddr = refault_vaddr + 4U;
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

    const uint64_t fault_low_pattern = 0x7162534435261708ULL;
    const uint64_t fault_high_pattern = 0x8899aabbccddeeffULL;
    const uint64_t refault_low_initial = 0xd4c3b2a1908f7e6dULL;
    const uint64_t refault_high_initial = 0x1a2b3c4d5e6f7081ULL;
    const uint64_t refault_low_pattern = 0x8f7e6d5c4b3a2918ULL;
    const uint64_t refault_high_pattern = 0x1021324354657687ULL;
    const uint64_t expected_recovery =
        ai_micro_mab_b2b_expected_cross_page_ld(
            refault_low_pattern, refault_high_pattern, 4U);
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
        "pair and the later same-template refault pair unchanged before "
        "the other-pair 4B+4B refault load",
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
        "after a 4B+4B fault on pair A, the immediate other-pair 4B+4B "
        "refault must report LAF on pair B's true second-half start "
        "address instead of reusing pair A's splitLoadResp or merge "
        "template",
        excpt.triggered == true &&
        excpt.cause == CAUSE_LAF &&
        excpt.tval == refault_second_half_vaddr);

    goto_priv(PRIV_M);
    TEST_ASSERT(
        "the other-pair 4B+4B refault probe must keep the older fault pair "
        "frozen and the refreshed refault pair unchanged before recovery",
        read64(fault_low_phys) == fault_low_pattern &&
        read64(fault_high_phys) == fault_high_pattern &&
        read64(refault_low_phys) == refault_low_pattern &&
        read64(refault_high_phys) == refault_high_pattern);

    ai_micro_mab_b2b_restore_pmp_full_access();

    ai_micro_mab_b2b_enable_mprv_s();
    TEST_SETUP_EXCEPT();
    observed = ld(refault_vaddr);
    TEST_ASSERT(
        "after the fault-pair 4B+4B fault then other-pair 4B+4B refault "
        "sequence is repaired, the other-pair 4B+4B recovery load must "
        "return only pair B's current merged payload instead of stale "
        "splitLoadResp data or merge template from pair A",
        excpt.triggered == false &&
        observed == expected_recovery);

    goto_priv(PRIV_M);
    TEST_ASSERT(
        "the fault-pair 4B+4B fault then other-pair 4B+4B refault/recovery "
        "sequence must leave all four backing doublewords unchanged after "
        "the recovery load",
        read64(fault_low_phys) == fault_low_pattern &&
        read64(fault_high_phys) == fault_high_pattern &&
        read64(refault_low_phys) == refault_low_pattern &&
        read64(refault_high_phys) == refault_high_pattern);

    ai_micro_mab_b2b_reset_env();

    TEST_END(
        "ai_micro_m_crosspage_4b4b_fault_then_other_pair_4b4b_refault_then_"
        "4b4b_recovery_no_stale_merge_template_corner");
}

bool ai_micro_m_crosspage_4b4b_store_fault_then_other_pair_4b4b_refault_then_4b4b_recovery_no_stale_split_template_corner()
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
        ai_micro_mab_b2b_cross_page_vaddr(refault_low_page, 4U);
    const uintptr_t refault_second_half_vaddr = refault_vaddr + 4U;
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

    const uint64_t fault_low_seed = 0x7162534435261708ULL;
    const uint64_t fault_high_seed = 0x8899aabbccddeeffULL;
    const uint64_t refault_low_initial = 0xd4c3b2a1908f7e6dULL;
    const uint64_t refault_high_initial = 0x1a2b3c4d5e6f7081ULL;
    const uint64_t refault_low_seed = 0x8f7e6d5c4b3a2918ULL;
    const uint64_t refault_high_seed = 0x1021324354657687ULL;
    const uint64_t fault_store_value = 0x1122334455667788ULL;
    const uint64_t refault_store_value = 0x31425364758697a8ULL;
    const uint64_t recovery_store_value = 0xf0e1d2c3b4a59687ULL;

    const uint64_t expected_fault_low_after_fault = fault_low_seed;
    const uint64_t expected_refault_low_after_refault = refault_low_seed;
    const uint64_t expected_refault_low_after_recovery =
        ai_micro_mab_b2b_expected_low_after_4b_store(
            refault_low_seed, recovery_store_value);
    const uint64_t expected_refault_high_after_recovery =
        ai_micro_mab_b2b_expected_high_after_4b_store(
            refault_high_seed, recovery_store_value);

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
        "the fault-pair 4B+4B store fault episode must not expose the fault pair low-half side effect and keep the later same-template "
        "refault pair untouched before the other-pair 4B+4B refault store",
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
        "after a 4B+4B fault on pair A, the immediate other-pair 4B+4B "
        "refault must report SAF on pair B's true second-half start "
        "address instead of reusing pair A's split template or stale "
        "high-half paddr",
        excpt.triggered == true &&
        excpt.cause == CAUSE_SAF &&
        excpt.tval == refault_second_half_vaddr);

    goto_priv(PRIV_M);
    TEST_ASSERT(
        "the other-pair 4B+4B refault probe must keep the older fault pair "
        "unchanged while pair B does not expose the current low-half side effect "
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
        "after the fault-pair 4B+4B fault then other-pair 4B+4B refault "
        "sequence is repaired, the other-pair 4B+4B recovery store must "
        "complete without reusing pair A's split template or stale "
        "high-half paddr",
        excpt.triggered == false);

    goto_priv(PRIV_M);
    TEST_ASSERT(
        "the fault-pair 4B+4B fault then other-pair 4B+4B refault/recovery "
        "sequence must keep pair A unchanged by the faulting store while pair "
        "B reflects only the current 4B+4B overlay",
        read64(fault_low_phys) == expected_fault_low_after_fault &&
        read64(fault_high_phys) == fault_high_seed &&
        read64(refault_low_phys) == expected_refault_low_after_recovery &&
        read64(refault_high_phys) == expected_refault_high_after_recovery);

    ai_micro_mab_b2b_reset_env();

    TEST_END(
        "ai_micro_m_crosspage_4b4b_store_fault_then_other_pair_4b4b_"
        "refault_then_4b4b_recovery_no_stale_split_template_corner");
}

bool ai_micro_m_crosspage_1b7b_fault_then_other_pair_1b7b_refault_then_1b7b_recovery_no_stale_merge_template_corner()
{
    TEST_START();

    const enum test_page fault_low_page = SWITCH1;
    const enum test_page fault_high_page = SWITCH2;
    const enum test_page refault_low_page = VSRWX_GI;
    const enum test_page refault_high_page = VSRW_GI;
    const uintptr_t fault_vaddr =
        ai_micro_mab_b2b_cross_page_vaddr(fault_low_page, 1U);
    const uintptr_t fault_second_half_vaddr = fault_vaddr + 1U;
    const uintptr_t refault_vaddr =
        ai_micro_mab_b2b_cross_page_vaddr(refault_low_page, 1U);
    const uintptr_t refault_second_half_vaddr = refault_vaddr + 1U;
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

    const uint64_t fault_low_pattern = 0x8f7e6d5c4b3a2918ULL;
    const uint64_t fault_high_pattern = 0x1021324354657687ULL;
    const uint64_t refault_low_initial = 0xd4c3b2a1908f7e6dULL;
    const uint64_t refault_high_initial = 0x1a2b3c4d5e6f7081ULL;
    const uint64_t refault_low_pattern = 0x7162534435261708ULL;
    const uint64_t refault_high_pattern = 0x8899aabbccddeeffULL;
    const uint64_t expected_recovery =
        ai_micro_mab_b2b_expected_cross_page_ld(
            refault_low_pattern, refault_high_pattern, 1U);
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
        "the fault-pair cross-page 1B+7B load must fault with LAF on the "
        "true upper-page start address when the fault pair upper page "
        "loses PMP read permission",
        excpt.triggered == true &&
        excpt.cause == CAUSE_LAF &&
        excpt.tval == fault_second_half_vaddr);

    goto_priv(PRIV_M);
    TEST_ASSERT(
        "the fault-pair 1B+7B load fault episode must keep both the fault "
        "pair and the later same-template refault pair unchanged before "
        "the other-pair 1B+7B refault load",
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
        "after a 1B+7B fault on pair A, the immediate other-pair 1B+7B "
        "refault must report LAF on pair B's true upper-page start "
        "address instead of reusing pair A's splitLoadResp or merge "
        "template",
        excpt.triggered == true &&
        excpt.cause == CAUSE_LAF &&
        excpt.tval == refault_second_half_vaddr);

    goto_priv(PRIV_M);
    TEST_ASSERT(
        "the other-pair 1B+7B refault probe must keep the older fault pair "
        "frozen and the refreshed refault pair unchanged before recovery",
        read64(fault_low_phys) == fault_low_pattern &&
        read64(fault_high_phys) == fault_high_pattern &&
        read64(refault_low_phys) == refault_low_pattern &&
        read64(refault_high_phys) == refault_high_pattern);

    ai_micro_mab_b2b_restore_pmp_full_access();

    ai_micro_mab_b2b_enable_mprv_s();
    TEST_SETUP_EXCEPT();
    observed = ld(refault_vaddr);
    TEST_ASSERT(
        "after the fault-pair 1B+7B fault then other-pair 1B+7B refault "
        "sequence is repaired, the other-pair 1B+7B recovery load must "
        "return only pair B's current merged payload instead of stale "
        "splitLoadResp data or merge template from pair A",
        excpt.triggered == false &&
        observed == expected_recovery);

    goto_priv(PRIV_M);
    TEST_ASSERT(
        "the fault-pair 1B+7B fault then other-pair 1B+7B refault/recovery "
        "sequence must leave all four backing doublewords unchanged after "
        "the recovery load",
        read64(fault_low_phys) == fault_low_pattern &&
        read64(fault_high_phys) == fault_high_pattern &&
        read64(refault_low_phys) == refault_low_pattern &&
        read64(refault_high_phys) == refault_high_pattern);

    ai_micro_mab_b2b_reset_env();

    TEST_END(
        "ai_micro_m_crosspage_1b7b_fault_then_other_pair_1b7b_refault_then_"
        "1b7b_recovery_no_stale_merge_template_corner");
}

bool ai_micro_m_crosspage_1b7b_store_fault_then_other_pair_1b7b_refault_then_1b7b_recovery_no_stale_split_template_corner()
{
    TEST_START();

    const enum test_page fault_low_page = SWITCH1;
    const enum test_page fault_high_page = SWITCH2;
    const enum test_page refault_low_page = VSRWX_GI;
    const enum test_page refault_high_page = VSRW_GI;
    const uintptr_t fault_vaddr =
        ai_micro_mab_b2b_cross_page_vaddr(fault_low_page, 1U);
    const uintptr_t fault_second_half_vaddr = fault_vaddr + 1U;
    const uintptr_t refault_vaddr =
        ai_micro_mab_b2b_cross_page_vaddr(refault_low_page, 1U);
    const uintptr_t refault_second_half_vaddr = refault_vaddr + 1U;
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

    const uint64_t fault_low_seed = 0xd4c3b2a1908f7e6dULL;
    const uint64_t fault_high_seed = 0x1a2b3c4d5e6f7081ULL;
    const uint64_t refault_low_initial = 0xf0e1d2c3b4a59687ULL;
    const uint64_t refault_high_initial = 0x1021324354657687ULL;
    const uint64_t refault_low_seed = 0x7162534435261708ULL;
    const uint64_t refault_high_seed = 0x8899aabbccddeeffULL;
    const uint64_t fault_store_value = 0x8f7e6d5c4b3a2918ULL;
    const uint64_t refault_store_value = 0x31425364758697a8ULL;
    const uint64_t recovery_store_value = 0x1122334455667788ULL;

    const uint64_t expected_fault_low_after_fault = fault_low_seed;
    const uint64_t expected_refault_low_after_refault = refault_low_seed;
    const uint64_t expected_refault_low_after_recovery =
        ai_micro_mab_b2b_expected_low_after_cross_page_store(
            refault_low_seed, recovery_store_value, 1U);
    const uint64_t expected_refault_high_after_recovery =
        ai_micro_mab_b2b_expected_high_after_cross_page_store(
            refault_high_seed, recovery_store_value, 1U);

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
        "the fault-pair cross-page 1B+7B store must fault with SAF on the "
        "true upper-page start address when the fault pair upper page "
        "loses PMP write permission",
        excpt.triggered == true &&
        excpt.cause == CAUSE_SAF &&
        excpt.tval == fault_second_half_vaddr);

    goto_priv(PRIV_M);
    TEST_ASSERT(
        "the fault-pair 1B+7B store fault episode must not expose the fault pair tail-byte side effect and keep the later same-template "
        "refault pair untouched before the other-pair 1B+7B refault store",
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
        "after a 1B+7B fault on pair A, the immediate other-pair 1B+7B "
        "refault must report SAF on pair B's true upper-page start "
        "address instead of reusing pair A's split template or stale "
        "high-half paddr",
        excpt.triggered == true &&
        excpt.cause == CAUSE_SAF &&
        excpt.tval == refault_second_half_vaddr);

    goto_priv(PRIV_M);
    TEST_ASSERT(
        "the other-pair 1B+7B refault probe must keep the older fault pair "
        "unchanged while pair B does not expose the current low-page tail-byte "
        "side effect before recovery",
        read64(fault_low_phys) == expected_fault_low_after_fault &&
        read64(fault_high_phys) == fault_high_seed &&
        read64(refault_low_phys) == expected_refault_low_after_refault &&
        read64(refault_high_phys) == refault_high_seed);

    ai_micro_mab_b2b_restore_pmp_full_access();

    ai_micro_mab_b2b_enable_mprv_s();
    TEST_SETUP_EXCEPT();
    sd(refault_vaddr, recovery_store_value);
    TEST_ASSERT(
        "after the fault-pair 1B+7B fault then other-pair 1B+7B refault "
        "sequence is repaired, the other-pair 1B+7B recovery store must "
        "complete without reusing pair A's split template or stale "
        "high-half paddr",
        excpt.triggered == false);

    goto_priv(PRIV_M);
    TEST_ASSERT(
        "the fault-pair 1B+7B fault then other-pair 1B+7B refault/recovery "
        "sequence must keep pair A unchanged by the faulting store while pair "
        "B reflects only the current 1B+7B overlay",
        read64(fault_low_phys) == expected_fault_low_after_fault &&
        read64(fault_high_phys) == fault_high_seed &&
        read64(refault_low_phys) == expected_refault_low_after_recovery &&
        read64(refault_high_phys) == expected_refault_high_after_recovery);

    ai_micro_mab_b2b_reset_env();

    TEST_END(
        "ai_micro_m_crosspage_1b7b_store_fault_then_other_pair_1b7b_"
        "refault_then_1b7b_recovery_no_stale_split_template_corner");
}

bool ai_micro_m_crosspage_4b4b_fault_then_other_pair_4b4b_recovery_no_stale_merge_template_corner()
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

    const uint64_t fault_low_pattern = 0x7162534435261708ULL;
    const uint64_t fault_high_pattern = 0x8899aabbccddeeffULL;
    const uint64_t recovery_low_pattern = 0x8f7e6d5c4b3a2918ULL;
    const uint64_t recovery_high_pattern = 0x1021324354657687ULL;
    const uint64_t expected_recovery =
        ai_micro_mab_b2b_expected_cross_page_ld(
            recovery_low_pattern, recovery_high_pattern, 4U);
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
        "after the fault-pair 4B+4B load episode is repaired, the immediate "
        "other-pair 4B+4B recovery load must return only the current merged "
        "payload instead of reusing the older splitLoadResp data or merge "
        "template from the fault pair",
        excpt.triggered == false &&
        observed == expected_recovery);

    goto_priv(PRIV_M);
    TEST_ASSERT(
        "the fault-pair 4B+4B load fault then other-pair 4B+4B recovery "
        "sequence must leave all four backing doublewords unchanged",
        read64(fault_low_phys) == fault_low_pattern &&
        read64(fault_high_phys) == fault_high_pattern &&
        read64(recovery_low_phys) == recovery_low_pattern &&
        read64(recovery_high_phys) == recovery_high_pattern);

    ai_micro_mab_b2b_reset_env();

    TEST_END(
        "ai_micro_m_crosspage_4b4b_fault_then_other_pair_4b4b_recovery_"
        "no_stale_merge_template_corner");
}
