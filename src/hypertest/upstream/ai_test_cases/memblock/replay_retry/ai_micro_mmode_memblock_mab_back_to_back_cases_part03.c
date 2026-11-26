#include "ai_micro_mmode_memblock_mab_back_to_back_common.h"

bool ai_micro_m_crosspage_2b6b_fault_then_other_pair_2b6b_refault_then_2b6b_recovery_no_stale_merge_template_corner()
{
    TEST_START();

    const enum test_page fault_low_page = SWITCH1;
    const enum test_page fault_high_page = SWITCH2;
    const enum test_page refault_low_page = VSRWX_GI;
    const enum test_page refault_high_page = VSRW_GI;
    const uintptr_t fault_vaddr =
        ai_micro_mab_b2b_cross_page_vaddr(fault_low_page, 2U);
    const uintptr_t fault_second_half_vaddr = fault_vaddr + 2U;
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

    const uint64_t fault_low_pattern = 0x6d5c4b3a291807f6ULL;
    const uint64_t fault_high_pattern = 0xa0b1c2d3e4f50617ULL;
    const uint64_t refault_low_initial = 0x1f2e3d4c5b6a7988ULL;
    const uint64_t refault_high_initial = 0x97a8b9cadbecfd0eULL;
    const uint64_t refault_low_pattern = 0x25364758697a8b9cULL;
    const uint64_t refault_high_pattern = 0xc0d1e2f304152637ULL;
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
        "the fault-pair cross-page 2B+6B load must fault with LAF on the "
        "true upper-page start address when the fault pair upper page "
        "loses PMP read permission",
        excpt.triggered == true &&
        excpt.cause == CAUSE_LAF &&
        excpt.tval == fault_second_half_vaddr);

    goto_priv(PRIV_M);
    TEST_ASSERT(
        "the fault-pair 2B+6B load fault episode must keep both the fault "
        "pair and the later refault pair unchanged before the same-template "
        "other-pair 2B+6B refault load",
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
        "2B+6B refault must report LAF on pair B's true upper-page start "
        "address instead of reusing pair A's splitLoadResp or merge "
        "template",
        excpt.triggered == true &&
        excpt.cause == CAUSE_LAF &&
        excpt.tval == refault_second_half_vaddr);

    goto_priv(PRIV_M);
    TEST_ASSERT(
        "the same-template other-pair 2B+6B refault probe must keep pair A "
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
        "after the pair-A 2B+6B fault then pair-B 2B+6B refault sequence is "
        "repaired, the same-template other-pair 2B+6B recovery load must "
        "return only pair B's current merged payload instead of stale "
        "splitLoadResp data or merge template from pair A",
        excpt.triggered == false &&
        observed == expected_recovery);

    goto_priv(PRIV_M);
    TEST_ASSERT(
        "the fault-pair 2B+6B fault then other-pair same-template 2B+6B "
        "refault/recovery sequence must leave all four backing doublewords "
        "unchanged after the recovery load",
        read64(fault_low_phys) == fault_low_pattern &&
        read64(fault_high_phys) == fault_high_pattern &&
        read64(refault_low_phys) == refault_low_pattern &&
        read64(refault_high_phys) == refault_high_pattern);

    ai_micro_mab_b2b_reset_env();

    TEST_END(
        "ai_micro_m_crosspage_2b6b_fault_then_other_pair_2b6b_refault_then_"
        "2b6b_recovery_no_stale_merge_template_corner");
}

bool ai_micro_m_crosspage_2b6b_store_fault_then_other_pair_2b6b_refault_then_2b6b_recovery_no_stale_split_template_corner()
{
    TEST_START();

    const enum test_page fault_low_page = SWITCH1;
    const enum test_page fault_high_page = SWITCH2;
    const enum test_page refault_low_page = VSRWX_GI;
    const enum test_page refault_high_page = VSRW_GI;
    const uintptr_t fault_vaddr =
        ai_micro_mab_b2b_cross_page_vaddr(fault_low_page, 2U);
    const uintptr_t fault_second_half_vaddr = fault_vaddr + 2U;
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

    const uint64_t fault_low_seed = 0x5e4d3c2b1a09f8e7ULL;
    const uint64_t fault_high_seed = 0x162738495a6b7c8dULL;
    const uint64_t refault_low_initial = 0xe0f1a2b3c4d59687ULL;
    const uint64_t refault_high_initial = 0x7f6e5d4c3b2a1908ULL;
    const uint64_t refault_low_seed = 0x31425364758697a8ULL;
    const uint64_t refault_high_seed = 0xb5c6d7e8f90a1b2cULL;
    const uint64_t fault_store_value = 0x9a8b7c6d5e4f3021ULL;
    const uint64_t refault_store_value = 0x1021324354657687ULL;
    const uint64_t recovery_store_value = 0xf0e1d2c3b4a59687ULL;

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
        "the fault-pair cross-page 2B+6B store must fault with SAF on the "
        "true upper-page start address when the fault pair upper page "
        "loses PMP write permission",
        excpt.triggered == true &&
        excpt.cause == CAUSE_SAF &&
        excpt.tval == fault_second_half_vaddr);

    goto_priv(PRIV_M);
    TEST_ASSERT(
        "the fault-pair 2B+6B store fault episode must not expose the fault "
        "pair low-page tail-halfword side effect and keep the later refault "
        "pair untouched before the same-template other-pair 2B+6B refault "
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
        "2B+6B refault must report SAF on pair B's true upper-page start "
        "address instead of reusing pair A's split template or stale "
        "high-half paddr",
        excpt.triggered == true &&
        excpt.cause == CAUSE_SAF &&
        excpt.tval == refault_second_half_vaddr);

    goto_priv(PRIV_M);
    TEST_ASSERT(
        "the same-template other-pair 2B+6B refault probe must keep pair A "
        "unchanged while pair B does not expose the current low-page tail-"
        "halfword side effect before recovery",
        read64(fault_low_phys) == expected_fault_low_after_fault &&
        read64(fault_high_phys) == fault_high_seed &&
        read64(refault_low_phys) == expected_refault_low_after_refault &&
        read64(refault_high_phys) == refault_high_seed);

    ai_micro_mab_b2b_restore_pmp_full_access();

    ai_micro_mab_b2b_enable_mprv_s();
    TEST_SETUP_EXCEPT();
    sd(refault_vaddr, recovery_store_value);
    TEST_ASSERT(
        "after the pair-A 2B+6B fault then pair-B 2B+6B refault sequence is "
        "repaired, the same-template other-pair 2B+6B recovery store must "
        "complete without reusing pair A's split template or stale "
        "high-half paddr",
        excpt.triggered == false);

    goto_priv(PRIV_M);
    TEST_ASSERT(
        "the fault-pair 2B+6B fault then other-pair same-template 2B+6B "
        "refault/recovery sequence must keep pair A unchanged "
        "while pair B reflects only the current 2B+6B overlay",
        read64(fault_low_phys) == expected_fault_low_after_fault &&
        read64(fault_high_phys) == fault_high_seed &&
        read64(refault_low_phys) == expected_refault_low_after_recovery &&
        read64(refault_high_phys) == expected_refault_high_after_recovery);

    ai_micro_mab_b2b_reset_env();

    TEST_END(
        "ai_micro_m_crosspage_2b6b_store_fault_then_other_pair_2b6b_"
        "refault_then_2b6b_recovery_no_stale_split_template_corner");
}

bool ai_micro_m_crosspage_2b6b_fault_then_other_pair_4b4b_refault_then_4b4b_recovery_no_stale_merge_template_corner()
{
    TEST_START();

    const enum test_page fault_low_page = SWITCH1;
    const enum test_page fault_high_page = SWITCH2;
    const enum test_page refault_low_page = VSRWX_GI;
    const enum test_page refault_high_page = VSRW_GI;
    const uintptr_t fault_vaddr =
        ai_micro_mab_b2b_cross_page_vaddr(fault_low_page, 2U);
    const uintptr_t fault_second_half_vaddr = fault_vaddr + 2U;
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

    const uint64_t fault_low_pattern = 0x1a2b3c4d5e6f7081ULL;
    const uint64_t fault_high_pattern = 0x92a3b4c5d6e7f809ULL;
    const uint64_t refault_low_initial = 0x0f1e2d3c4b5a6978ULL;
    const uint64_t refault_high_initial = 0xa9b8c7d6e5f40312ULL;
    const uint64_t refault_low_pattern = 0x3141592653589793ULL;
    const uint64_t refault_high_pattern = 0x2718281828459045ULL;
    const uint64_t expected_recovery =
        ai_micro_mab_b2b_expected_4b4b_load(
            refault_low_pattern, refault_high_pattern);
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
        "the fault-pair cross-page 2B+6B load must fault with LAF on the "
        "true upper-page start address when the fault pair upper page "
        "loses PMP read permission",
        excpt.triggered == true &&
        excpt.cause == CAUSE_LAF &&
        excpt.tval == fault_second_half_vaddr);

    goto_priv(PRIV_M);
    TEST_ASSERT(
        "the fault-pair 2B+6B load fault episode must keep both the fault "
        "pair and the later refault pair unchanged before the wider "
        "other-pair 4B+4B refault load",
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
        "after a narrower pair-A 2B+6B fault, the immediate wider other-"
        "pair 4B+4B refault must report LAF on pair B's true second-half "
        "start address instead of reusing pair A's splitLoadResp or merge "
        "metadata",
        excpt.triggered == true &&
        excpt.cause == CAUSE_LAF &&
        excpt.tval == refault_second_half_vaddr);

    goto_priv(PRIV_M);
    TEST_ASSERT(
        "the wider other-pair 4B+4B refault probe must keep pair A frozen "
        "and leave pair B's refreshed backing image unchanged before "
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
        "after the pair-A 2B+6B fault then pair-B 4B+4B refault sequence is "
        "repaired, the wider other-pair 4B+4B recovery load must return "
        "only pair B's current merged payload instead of stale pair-A "
        "narrow-template response data",
        excpt.triggered == false &&
        observed == expected_recovery);

    goto_priv(PRIV_M);
    TEST_ASSERT(
        "the fault-pair 2B+6B fault then wider other-pair 4B+4B "
        "refault/recovery sequence must leave all four backing doublewords "
        "unchanged after the recovery load",
        read64(fault_low_phys) == fault_low_pattern &&
        read64(fault_high_phys) == fault_high_pattern &&
        read64(refault_low_phys) == refault_low_pattern &&
        read64(refault_high_phys) == refault_high_pattern);

    ai_micro_mab_b2b_reset_env();

    TEST_END(
        "ai_micro_m_crosspage_2b6b_fault_then_other_pair_4b4b_refault_then_"
        "4b4b_recovery_no_stale_merge_template_corner");
}

bool ai_micro_m_crosspage_2b6b_store_fault_then_other_pair_4b4b_refault_then_4b4b_recovery_no_stale_split_template_corner()
{
    TEST_START();

    const enum test_page fault_low_page = SWITCH1;
    const enum test_page fault_high_page = SWITCH2;
    const enum test_page refault_low_page = VSRWX_GI;
    const enum test_page refault_high_page = VSRW_GI;
    const uintptr_t fault_vaddr =
        ai_micro_mab_b2b_cross_page_vaddr(fault_low_page, 2U);
    const uintptr_t fault_second_half_vaddr = fault_vaddr + 2U;
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

    const uint64_t fault_low_seed = 0x8899aabbccddeeffULL;
    const uint64_t fault_high_seed = 0x1020304050607081ULL;
    const uint64_t refault_low_initial = 0xf1e2d3c4b5a69788ULL;
    const uint64_t refault_high_initial = 0x1827364554637281ULL;
    const uint64_t refault_low_seed = 0x123456789abcdef0ULL;
    const uint64_t refault_high_seed = 0x0fedcba987654321ULL;
    const uint64_t fault_store_value = 0xa1b2c3d4e5f60718ULL;
    const uint64_t refault_store_value = 0x5566778899aabbccULL;
    const uint64_t recovery_store_value = 0xc3d2e1f0a9b8c7d6ULL;

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
        "the fault-pair cross-page 2B+6B store must fault with SAF on the "
        "true upper-page start address when the fault pair upper page "
        "loses PMP write permission",
        excpt.triggered == true &&
        excpt.cause == CAUSE_SAF &&
        excpt.tval == fault_second_half_vaddr);

    goto_priv(PRIV_M);
    TEST_ASSERT(
        "the fault-pair 2B+6B store fault episode must not expose the fault pair low-page tail-halfword side effect and keep the later "
        "refault pair untouched before the wider other-pair 4B+4B refault "
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
        "after a narrower pair-A 2B+6B fault, the immediate wider other-"
        "pair 4B+4B refault must report SAF on pair B's true second-half "
        "start address instead of reusing pair A's split template or stale "
        "high-half paddr",
        excpt.triggered == true &&
        excpt.cause == CAUSE_SAF &&
        excpt.tval == refault_second_half_vaddr);

    goto_priv(PRIV_M);
    TEST_ASSERT(
        "the wider other-pair 4B+4B refault probe must keep pair A frozen "
        "while pair B does not expose the current low-page upper-word side effect "
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
        "after the pair-A 2B+6B fault then pair-B 4B+4B refault sequence is "
        "repaired, the wider other-pair 4B+4B recovery store must complete "
        "without reusing pair A's split template or stale high-half paddr",
        excpt.triggered == false);

    goto_priv(PRIV_M);
    TEST_ASSERT(
        "the fault-pair 2B+6B fault then wider other-pair 4B+4B "
        "refault/recovery sequence must keep pair A unchanged "
        "while pair B reflects only the current 4B+4B overlay",
        read64(fault_low_phys) == expected_fault_low_after_fault &&
        read64(fault_high_phys) == fault_high_seed &&
        read64(refault_low_phys) == expected_refault_low_after_recovery &&
        read64(refault_high_phys) == expected_refault_high_after_recovery);

    ai_micro_mab_b2b_reset_env();

    TEST_END(
        "ai_micro_m_crosspage_2b6b_store_fault_then_other_pair_4b4b_"
        "refault_then_4b4b_recovery_no_stale_split_template_corner");
}

bool ai_micro_m_crosspage_3b5b_fault_then_other_pair_4b4b_refault_then_4b4b_recovery_no_stale_merge_template_corner()
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

    const uint64_t fault_low_pattern = 0x23456789abcdeff0ULL;
    const uint64_t fault_high_pattern = 0x1029384756a5b4c3ULL;
    const uint64_t refault_low_initial = 0x5566778899aabbccULL;
    const uint64_t refault_high_initial = 0xddeeff0011223344ULL;
    const uint64_t refault_low_pattern = 0x31425364758697a8ULL;
    const uint64_t refault_high_pattern = 0xc1d2e3f405162738ULL;
    const uint64_t expected_recovery =
        ai_micro_mab_b2b_expected_4b4b_load(
            refault_low_pattern, refault_high_pattern);
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
        "pair and the later refault pair unchanged before the wider "
        "other-pair 4B+4B refault load",
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
        "after a narrower pair-A 3B+5B fault, the immediate wider other-"
        "pair 4B+4B refault must report LAF on pair B's true second-half "
        "start address instead of reusing pair A's splitLoadResp or merge "
        "metadata",
        excpt.triggered == true &&
        excpt.cause == CAUSE_LAF &&
        excpt.tval == refault_second_half_vaddr);

    goto_priv(PRIV_M);
    TEST_ASSERT(
        "the wider other-pair 4B+4B refault probe must keep pair A frozen "
        "and leave pair B's refreshed backing image unchanged before "
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
        "after the pair-A 3B+5B fault then pair-B 4B+4B refault sequence is "
        "repaired, the wider other-pair 4B+4B recovery load must return "
        "only pair B's current merged payload instead of stale pair-A "
        "narrow-template response data",
        excpt.triggered == false &&
        observed == expected_recovery);

    goto_priv(PRIV_M);
    TEST_ASSERT(
        "the fault-pair 3B+5B fault then wider other-pair 4B+4B "
        "refault/recovery sequence must leave all four backing doublewords "
        "unchanged after the recovery load",
        read64(fault_low_phys) == fault_low_pattern &&
        read64(fault_high_phys) == fault_high_pattern &&
        read64(refault_low_phys) == refault_low_pattern &&
        read64(refault_high_phys) == refault_high_pattern);

    ai_micro_mab_b2b_reset_env();

    TEST_END(
        "ai_micro_m_crosspage_3b5b_fault_then_other_pair_4b4b_refault_then_"
        "4b4b_recovery_no_stale_merge_template_corner");
}

bool ai_micro_m_crosspage_3b5b_store_fault_then_other_pair_4b4b_refault_then_4b4b_recovery_no_stale_split_template_corner()
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

    const uint64_t fault_low_seed = 0x98a7b6c5d4e3f201ULL;
    const uint64_t fault_high_seed = 0x1122446688aacceeULL;
    const uint64_t refault_low_initial = 0x0f1e2d3c4b5a6978ULL;
    const uint64_t refault_high_initial = 0x8776655443322110ULL;
    const uint64_t refault_low_seed = 0xabcdef0123456789ULL;
    const uint64_t refault_high_seed = 0xfedcba9876543210ULL;
    const uint64_t fault_store_value = 0x2233445566778899ULL;
    const uint64_t refault_store_value = 0x3141592653589793ULL;
    const uint64_t recovery_store_value = 0xc3d4e5f60718293aULL;

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
        "the fault-pair cross-page 3B+5B store must fault with SAF on the "
        "true upper-page start address when the fault pair upper page "
        "loses PMP write permission",
        excpt.triggered == true &&
        excpt.cause == CAUSE_SAF &&
        excpt.tval == fault_second_half_vaddr);

    goto_priv(PRIV_M);
    TEST_ASSERT(
        "the fault-pair 3B+5B store fault episode must not expose the fault pair low-page tail-three-byte side effect and keep the later "
        "refault pair untouched before the wider other-pair 4B+4B refault "
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
        "after a narrower pair-A 3B+5B fault, the immediate wider other-"
        "pair 4B+4B refault must report SAF on pair B's true second-half "
        "start address instead of reusing pair A's split template or stale "
        "high-half paddr",
        excpt.triggered == true &&
        excpt.cause == CAUSE_SAF &&
        excpt.tval == refault_second_half_vaddr);

    goto_priv(PRIV_M);
    TEST_ASSERT(
        "the wider other-pair 4B+4B refault probe must keep pair A frozen "
        "while pair B does not expose the current low-page upper-word side effect "
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
        "after the pair-A 3B+5B fault then pair-B 4B+4B refault sequence is "
        "repaired, the wider other-pair 4B+4B recovery store must complete "
        "without reusing pair A's split template or stale high-half paddr",
        excpt.triggered == false);

    goto_priv(PRIV_M);
    TEST_ASSERT(
        "the fault-pair 3B+5B fault then wider other-pair 4B+4B "
        "refault/recovery sequence must keep pair A unchanged "
        "while pair B reflects only the current 4B+4B overlay",
        read64(fault_low_phys) == expected_fault_low_after_fault &&
        read64(fault_high_phys) == fault_high_seed &&
        read64(refault_low_phys) == expected_refault_low_after_recovery &&
        read64(refault_high_phys) == expected_refault_high_after_recovery);

    ai_micro_mab_b2b_reset_env();

    TEST_END(
        "ai_micro_m_crosspage_3b5b_store_fault_then_other_pair_4b4b_"
        "refault_then_4b4b_recovery_no_stale_split_template_corner");
}

bool ai_micro_m_crosspage_2b6b_fault_then_other_pair_3b5b_refault_then_3b5b_recovery_no_stale_merge_template_corner()
{
    TEST_START();

    const enum test_page fault_low_page = SWITCH1;
    const enum test_page fault_high_page = SWITCH2;
    const enum test_page refault_low_page = VSRWX_GI;
    const enum test_page refault_high_page = VSRW_GI;
    const uintptr_t fault_vaddr =
        ai_micro_mab_b2b_cross_page_vaddr(fault_low_page, 2U);
    const uintptr_t fault_second_half_vaddr = fault_vaddr + 2U;
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

    const uint64_t fault_low_pattern = 0x0a1b2c3d4e5f6071ULL;
    const uint64_t fault_high_pattern = 0x8293a4b5c6d7e8f9ULL;
    const uint64_t refault_low_initial = 0x1021324354657687ULL;
    const uint64_t refault_high_initial = 0x98a9babcbddceeffULL;
    const uint64_t refault_low_pattern = 0x2233445566778899ULL;
    const uint64_t refault_high_pattern = 0xa1b2c3d4e5f60718ULL;
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
        "the fault-pair cross-page 2B+6B load must fault with LAF on the "
        "true upper-page start address when the fault pair upper page "
        "loses PMP read permission",
        excpt.triggered == true &&
        excpt.cause == CAUSE_LAF &&
        excpt.tval == fault_second_half_vaddr);

    goto_priv(PRIV_M);
    TEST_ASSERT(
        "the fault-pair 2B+6B load fault episode must keep both the fault "
        "pair and the later refault pair unchanged before the neighboring "
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
        "after a narrower pair-A 2B+6B fault, the immediate neighboring "
        "other-pair 3B+5B refault must report LAF on pair B's true upper-"
        "page start address instead of reusing pair A's splitLoadResp or "
        "merge metadata",
        excpt.triggered == true &&
        excpt.cause == CAUSE_LAF &&
        excpt.tval == refault_second_half_vaddr);

    goto_priv(PRIV_M);
    TEST_ASSERT(
        "the neighboring other-pair 3B+5B refault probe must keep pair A "
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
        "after the pair-A 2B+6B fault then pair-B 3B+5B refault sequence is "
        "repaired, the neighboring other-pair 3B+5B recovery load must "
        "return only pair B's current merged payload instead of stale "
        "pair-A narrow-template response data",
        excpt.triggered == false &&
        observed == expected_recovery);

    goto_priv(PRIV_M);
    TEST_ASSERT(
        "the fault-pair 2B+6B fault then neighboring other-pair 3B+5B "
        "refault/recovery sequence must leave all four backing doublewords "
        "unchanged after the recovery load",
        read64(fault_low_phys) == fault_low_pattern &&
        read64(fault_high_phys) == fault_high_pattern &&
        read64(refault_low_phys) == refault_low_pattern &&
        read64(refault_high_phys) == refault_high_pattern);

    ai_micro_mab_b2b_reset_env();

    TEST_END(
        "ai_micro_m_crosspage_2b6b_fault_then_other_pair_3b5b_refault_then_"
        "3b5b_recovery_no_stale_merge_template_corner");
}

bool ai_micro_m_crosspage_2b6b_store_fault_then_other_pair_3b5b_refault_then_3b5b_recovery_no_stale_split_template_corner()
{
    TEST_START();

    const enum test_page fault_low_page = SWITCH1;
    const enum test_page fault_high_page = SWITCH2;
    const enum test_page refault_low_page = VSRWX_GI;
    const enum test_page refault_high_page = VSRW_GI;
    const uintptr_t fault_vaddr =
        ai_micro_mab_b2b_cross_page_vaddr(fault_low_page, 2U);
    const uintptr_t fault_second_half_vaddr = fault_vaddr + 2U;
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

    const uint64_t fault_low_seed = 0x31425364758697a8ULL;
    const uint64_t fault_high_seed = 0xb9cadbecfd0e1f20ULL;
    const uint64_t refault_low_initial = 0x5566778899aabbccULL;
    const uint64_t refault_high_initial = 0x0f1e2d3c4b5a6978ULL;
    const uint64_t refault_low_seed = 0x92a3b4c5d6e7f809ULL;
    const uint64_t refault_high_seed = 0x1122334455667788ULL;
    const uint64_t fault_store_value = 0xaabbccddeeff0011ULL;
    const uint64_t refault_store_value = 0x123456789abcdef0ULL;
    const uint64_t recovery_store_value = 0xc3d4e5f60718293aULL;

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
        "the fault-pair cross-page 2B+6B store must fault with SAF on the "
        "true upper-page start address when the fault pair upper page "
        "loses PMP write permission",
        excpt.triggered == true &&
        excpt.cause == CAUSE_SAF &&
        excpt.tval == fault_second_half_vaddr);

    goto_priv(PRIV_M);
    TEST_ASSERT(
        "the fault-pair 2B+6B store fault episode must not expose the fault pair low-page tail-halfword side effect and keep the later "
        "refault pair untouched before the neighboring other-pair 3B+5B "
        "refault store",
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
        "after a narrower pair-A 2B+6B fault, the immediate neighboring "
        "other-pair 3B+5B refault must report SAF on pair B's true upper-"
        "page start address instead of reusing pair A's split template or "
        "stale high-half paddr",
        excpt.triggered == true &&
        excpt.cause == CAUSE_SAF &&
        excpt.tval == refault_second_half_vaddr);

    goto_priv(PRIV_M);
    TEST_ASSERT(
        "the neighboring other-pair 3B+5B refault probe must keep pair A "
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
        "after the pair-A 2B+6B fault then pair-B 3B+5B refault sequence is "
        "repaired, the neighboring other-pair 3B+5B recovery store must "
        "complete without reusing pair A's split template or stale "
        "high-half paddr",
        excpt.triggered == false);

    goto_priv(PRIV_M);
    TEST_ASSERT(
        "the fault-pair 2B+6B fault then neighboring other-pair 3B+5B "
        "refault/recovery sequence must keep pair A unchanged "
        "while pair B reflects only the current 3B+5B overlay",
        read64(fault_low_phys) == expected_fault_low_after_fault &&
        read64(fault_high_phys) == fault_high_seed &&
        read64(refault_low_phys) == expected_refault_low_after_recovery &&
        read64(refault_high_phys) == expected_refault_high_after_recovery);

    ai_micro_mab_b2b_reset_env();

    TEST_END(
        "ai_micro_m_crosspage_2b6b_store_fault_then_other_pair_3b5b_"
        "refault_then_3b5b_recovery_no_stale_split_template_corner");
}

bool ai_micro_m_crosspage_3b5b_fault_then_other_pair_2b6b_refault_then_2b6b_recovery_no_stale_merge_template_corner()
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

    const uint64_t fault_low_pattern = 0x2132435465768798ULL;
    const uint64_t fault_high_pattern = 0xa9bacbdcedfe0f10ULL;
    const uint64_t refault_low_initial = 0x1122334455667788ULL;
    const uint64_t refault_high_initial = 0x98a7b6c5d4e3f201ULL;
    const uint64_t refault_low_pattern = 0x3141592653589793ULL;
    const uint64_t refault_high_pattern = 0x2718281828459045ULL;
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
        "the fault-pair cross-page 3B+5B load must fault with LAF on the "
        "true upper-page start address when the fault pair upper page "
        "loses PMP read permission",
        excpt.triggered == true &&
        excpt.cause == CAUSE_LAF &&
        excpt.tval == fault_second_half_vaddr);

    goto_priv(PRIV_M);
    TEST_ASSERT(
        "the fault-pair 3B+5B load fault episode must keep both the fault "
        "pair and the later refault pair unchanged before the neighboring "
        "other-pair 2B+6B refault load",
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
        "after a narrower pair-A 3B+5B fault, the immediate neighboring "
        "other-pair 2B+6B refault must report LAF on pair B's true upper-"
        "page start address instead of reusing pair A's splitLoadResp or "
        "merge metadata",
        excpt.triggered == true &&
        excpt.cause == CAUSE_LAF &&
        excpt.tval == refault_second_half_vaddr);

    goto_priv(PRIV_M);
    TEST_ASSERT(
        "the neighboring other-pair 2B+6B refault probe must keep pair A "
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
        "after the pair-A 3B+5B fault then pair-B 2B+6B refault sequence is "
        "repaired, the neighboring other-pair 2B+6B recovery load must "
        "return only pair B's current merged payload instead of stale "
        "pair-A narrow-template response data",
        excpt.triggered == false &&
        observed == expected_recovery);

    goto_priv(PRIV_M);
    TEST_ASSERT(
        "the fault-pair 3B+5B fault then neighboring other-pair 2B+6B "
        "refault/recovery sequence must leave all four backing doublewords "
        "unchanged after the recovery load",
        read64(fault_low_phys) == fault_low_pattern &&
        read64(fault_high_phys) == fault_high_pattern &&
        read64(refault_low_phys) == refault_low_pattern &&
        read64(refault_high_phys) == refault_high_pattern);

    ai_micro_mab_b2b_reset_env();

    TEST_END(
        "ai_micro_m_crosspage_3b5b_fault_then_other_pair_2b6b_refault_then_"
        "2b6b_recovery_no_stale_merge_template_corner");
}

bool ai_micro_m_crosspage_3b5b_store_fault_then_other_pair_2b6b_refault_then_2b6b_recovery_no_stale_split_template_corner()
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

    const uint64_t fault_low_seed = 0x92a3b4c5d6e7f809ULL;
    const uint64_t fault_high_seed = 0x1122334455667788ULL;
    const uint64_t refault_low_initial = 0x31425364758697a8ULL;
    const uint64_t refault_high_initial = 0xb9cadbecfd0e1f20ULL;
    const uint64_t refault_low_seed = 0x5566778899aabbccULL;
    const uint64_t refault_high_seed = 0x0f1e2d3c4b5a6978ULL;
    const uint64_t fault_store_value = 0x123456789abcdef0ULL;
    const uint64_t refault_store_value = 0xaabbccddeeff0011ULL;
    const uint64_t recovery_store_value = 0xc3d4e5f60718293aULL;

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
        "the fault-pair cross-page 3B+5B store must fault with SAF on the "
        "true upper-page start address when the fault pair upper page "
        "loses PMP write permission",
        excpt.triggered == true &&
        excpt.cause == CAUSE_SAF &&
        excpt.tval == fault_second_half_vaddr);

    goto_priv(PRIV_M);
    TEST_ASSERT(
        "the fault-pair 3B+5B store fault episode must not expose the fault pair low-page tail-three-byte side effect and keep the later "
        "refault pair untouched before the neighboring other-pair 2B+6B "
        "refault store",
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
        "after a narrower pair-A 3B+5B fault, the immediate neighboring "
        "other-pair 2B+6B refault must report SAF on pair B's true upper-"
        "page start address instead of reusing pair A's split template or "
        "stale high-half paddr",
        excpt.triggered == true &&
        excpt.cause == CAUSE_SAF &&
        excpt.tval == refault_second_half_vaddr);

    goto_priv(PRIV_M);
    TEST_ASSERT(
        "the neighboring other-pair 2B+6B refault probe must keep pair A "
        "unchanged while pair B does not expose the current low-page tail-"
        "halfword side effect before recovery",
        read64(fault_low_phys) == expected_fault_low_after_fault &&
        read64(fault_high_phys) == fault_high_seed &&
        read64(refault_low_phys) == expected_refault_low_after_refault &&
        read64(refault_high_phys) == refault_high_seed);

    ai_micro_mab_b2b_restore_pmp_full_access();

    ai_micro_mab_b2b_enable_mprv_s();
    TEST_SETUP_EXCEPT();
    sd(refault_vaddr, recovery_store_value);
    TEST_ASSERT(
        "after the pair-A 3B+5B fault then pair-B 2B+6B refault sequence is "
        "repaired, the neighboring other-pair 2B+6B recovery store must "
        "complete without reusing pair A's split template or stale "
        "high-half paddr",
        excpt.triggered == false);

    goto_priv(PRIV_M);
    TEST_ASSERT(
        "the fault-pair 3B+5B fault then neighboring other-pair 2B+6B "
        "refault/recovery sequence must keep pair A unchanged "
        "while pair B reflects only the current 2B+6B overlay",
        read64(fault_low_phys) == expected_fault_low_after_fault &&
        read64(fault_high_phys) == fault_high_seed &&
        read64(refault_low_phys) == expected_refault_low_after_recovery &&
        read64(refault_high_phys) == expected_refault_high_after_recovery);

    ai_micro_mab_b2b_reset_env();

    TEST_END(
        "ai_micro_m_crosspage_3b5b_store_fault_then_other_pair_2b6b_"
        "refault_then_2b6b_recovery_no_stale_split_template_corner");
}

bool ai_micro_m_crosspage_5b3b_fault_then_other_pair_5b3b_refault_then_5b3b_recovery_no_stale_merge_template_corner()
{
    TEST_START();

    const enum test_page fault_low_page = SWITCH1;
    const enum test_page fault_high_page = SWITCH2;
    const enum test_page refault_low_page = VSRWX_GI;
    const enum test_page refault_high_page = VSRW_GI;
    const uintptr_t fault_vaddr =
        ai_micro_mab_b2b_cross_page_vaddr(fault_low_page, 5U);
    const uintptr_t fault_second_half_vaddr = fault_vaddr + 5U;
    const uintptr_t refault_vaddr =
        ai_micro_mab_b2b_cross_page_vaddr(refault_low_page, 5U);
    const uintptr_t refault_second_half_vaddr = refault_vaddr + 5U;
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

    const uint64_t fault_low_pattern = 0x1021324354657687ULL;
    const uint64_t fault_high_pattern = 0x98a9bacbdcedfe0fULL;
    const uint64_t refault_low_initial = 0x2233445566778899ULL;
    const uint64_t refault_high_initial = 0xa1b2c3d4e5f60718ULL;
    const uint64_t refault_low_pattern = 0x3141592653589793ULL;
    const uint64_t refault_high_pattern = 0x2718281828459045ULL;
    const uint64_t expected_recovery =
        ai_micro_mab_b2b_expected_cross_page_ld(
            refault_low_pattern, refault_high_pattern, 5U);
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
        "the fault-pair cross-page 5B+3B load must fault with LAF on the "
        "true upper-page start address when the fault pair upper page "
        "loses PMP read permission",
        excpt.triggered == true &&
        excpt.cause == CAUSE_LAF &&
        excpt.tval == fault_second_half_vaddr);

    goto_priv(PRIV_M);
    TEST_ASSERT(
        "the fault-pair 5B+3B load fault episode must keep both the fault "
        "pair and the later refault pair unchanged before the same-template "
        "other-pair 5B+3B refault load",
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
        "after a mirrored middle-ratio fault on pair A, the immediate "
        "other-pair same-template 5B+3B refault must report LAF on pair B's "
        "true upper-page start address instead of reusing pair A's "
        "splitLoadResp or owner-flipped merge template",
        excpt.triggered == true &&
        excpt.cause == CAUSE_LAF &&
        excpt.tval == refault_second_half_vaddr);

    goto_priv(PRIV_M);
    TEST_ASSERT(
        "the same-template other-pair 5B+3B refault probe must keep pair A "
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
        "after the pair-A 5B+3B fault then pair-B 5B+3B refault sequence is "
        "repaired, the same-template other-pair 5B+3B recovery load must "
        "return only pair B's current merged payload instead of stale "
        "splitLoadResp data or merge template from pair A",
        excpt.triggered == false &&
        observed == expected_recovery);

    goto_priv(PRIV_M);
    TEST_ASSERT(
        "the fault-pair 5B+3B fault then other-pair same-template 5B+3B "
        "refault/recovery sequence must leave all four backing doublewords "
        "unchanged after the recovery load",
        read64(fault_low_phys) == fault_low_pattern &&
        read64(fault_high_phys) == fault_high_pattern &&
        read64(refault_low_phys) == refault_low_pattern &&
        read64(refault_high_phys) == refault_high_pattern);

    ai_micro_mab_b2b_reset_env();

    TEST_END(
        "ai_micro_m_crosspage_5b3b_fault_then_other_pair_5b3b_refault_then_"
        "5b3b_recovery_no_stale_merge_template_corner");
}

bool ai_micro_m_crosspage_5b3b_store_fault_then_other_pair_5b3b_refault_then_5b3b_recovery_no_stale_split_template_corner()
{
    TEST_START();

    const enum test_page fault_low_page = SWITCH1;
    const enum test_page fault_high_page = SWITCH2;
    const enum test_page refault_low_page = VSRWX_GI;
    const enum test_page refault_high_page = VSRW_GI;
    const uintptr_t fault_vaddr =
        ai_micro_mab_b2b_cross_page_vaddr(fault_low_page, 5U);
    const uintptr_t fault_second_half_vaddr = fault_vaddr + 5U;
    const uintptr_t refault_vaddr =
        ai_micro_mab_b2b_cross_page_vaddr(refault_low_page, 5U);
    const uintptr_t refault_second_half_vaddr = refault_vaddr + 5U;
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

    const uint64_t fault_low_seed = 0x0011223344556677ULL;
    const uint64_t fault_high_seed = 0x8899aabbccddeeffULL;
    const uint64_t refault_low_initial = 0x13579bdf2468ace0ULL;
    const uint64_t refault_high_initial = 0xfdb97531eca86420ULL;
    const uint64_t refault_low_seed = 0x1123581321345589ULL;
    const uint64_t refault_high_seed = 0x89abcdef01234567ULL;
    const uint64_t fault_store_value = 0xa55aa55af00df00dULL;
    const uint64_t refault_store_value = 0x0f1e2d3c4b5a6978ULL;
    const uint64_t recovery_store_value = 0x76543210fedcba98ULL;

    const uint64_t expected_fault_low_after_fault = fault_low_seed;
    const uint64_t expected_refault_low_after_refault = refault_low_seed;
    const uint64_t expected_refault_low_after_recovery =
        ai_micro_mab_b2b_expected_low_after_cross_page_store(
            refault_low_seed, recovery_store_value, 5U);
    const uint64_t expected_refault_high_after_recovery =
        ai_micro_mab_b2b_expected_high_after_cross_page_store(
            refault_high_seed, recovery_store_value, 5U);

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
        "the fault-pair cross-page 5B+3B store must fault with SAF on the "
        "true upper-page start address when the fault pair upper page "
        "loses PMP write permission",
        excpt.triggered == true &&
        excpt.cause == CAUSE_SAF &&
        excpt.tval == fault_second_half_vaddr);

    goto_priv(PRIV_M);
    TEST_ASSERT(
        "the fault-pair 5B+3B store fault episode must not expose the fault "
        "pair low-page tail-five-byte side effect and keep the later refault "
        "pair untouched before the same-template other-pair 5B+3B refault "
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
        "after a mirrored middle-ratio fault on pair A, the immediate "
        "other-pair same-template 5B+3B refault must report SAF on pair B's "
        "true upper-page start address instead of reusing pair A's split "
        "template or stale high-half paddr",
        excpt.triggered == true &&
        excpt.cause == CAUSE_SAF &&
        excpt.tval == refault_second_half_vaddr);

    goto_priv(PRIV_M);
    TEST_ASSERT(
        "the same-template other-pair 5B+3B refault probe must keep pair A "
        "unchanged while pair B does not expose the current low-page tail-five-"
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
        "after the pair-A 5B+3B fault then pair-B 5B+3B refault sequence is "
        "repaired, the same-template other-pair 5B+3B recovery store must "
        "complete without reusing pair A's split template or stale "
        "high-half paddr",
        excpt.triggered == false);

    goto_priv(PRIV_M);
    TEST_ASSERT(
        "the fault-pair 5B+3B fault then other-pair same-template 5B+3B "
        "refault/recovery sequence must keep pair A unchanged "
        "while pair B reflects only the current 5B+3B overlay",
        read64(fault_low_phys) == expected_fault_low_after_fault &&
        read64(fault_high_phys) == fault_high_seed &&
        read64(refault_low_phys) == expected_refault_low_after_recovery &&
        read64(refault_high_phys) == expected_refault_high_after_recovery);

    ai_micro_mab_b2b_reset_env();

    TEST_END(
        "ai_micro_m_crosspage_5b3b_store_fault_then_other_pair_5b3b_"
        "refault_then_5b3b_recovery_no_stale_split_template_corner");
}

bool ai_micro_m_crosspage_6b2b_fault_then_other_pair_6b2b_refault_then_6b2b_recovery_no_stale_merge_template_corner()
{
    TEST_START();

    const enum test_page fault_low_page = SWITCH1;
    const enum test_page fault_high_page = SWITCH2;
    const enum test_page refault_low_page = VSRWX_GI;
    const enum test_page refault_high_page = VSRW_GI;
    const uintptr_t fault_vaddr =
        ai_micro_mab_b2b_cross_page_vaddr(fault_low_page, 6U);
    const uintptr_t fault_second_half_vaddr = fault_vaddr + 6U;
    const uintptr_t refault_vaddr =
        ai_micro_mab_b2b_cross_page_vaddr(refault_low_page, 6U);
    const uintptr_t refault_second_half_vaddr = refault_vaddr + 6U;
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

    const uint64_t fault_low_pattern = 0x123456789abcdef0ULL;
    const uint64_t fault_high_pattern = 0x0f1e2d3c4b5a6978ULL;
    const uint64_t refault_low_initial = 0xa1b2c3d4e5f60718ULL;
    const uint64_t refault_high_initial = 0x8192a3b4c5d6e7f8ULL;
    const uint64_t refault_low_pattern = 0x8877665544332211ULL;
    const uint64_t refault_high_pattern = 0xfedcba9876543210ULL;
    const uint64_t expected_recovery =
        ai_micro_mab_b2b_expected_cross_page_ld(
            refault_low_pattern, refault_high_pattern, 6U);
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
        "the fault-pair cross-page 6B+2B load must fault with LAF on the "
        "true upper-page start address when the fault pair upper page "
        "loses PMP read permission",
        excpt.triggered == true &&
        excpt.cause == CAUSE_LAF &&
        excpt.tval == fault_second_half_vaddr);

    goto_priv(PRIV_M);
    TEST_ASSERT(
        "the fault-pair 6B+2B load fault episode must keep both the fault "
        "pair and the later refault pair unchanged before the same-template "
        "other-pair 6B+2B refault load",
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
        "after a tail-six-byte fault on pair A, the immediate other-pair "
        "same-template 6B+2B refault must report LAF on pair B's true "
        "upper-page start address instead of reusing pair A's "
        "splitLoadResp or stale tail-halfword merge metadata",
        excpt.triggered == true &&
        excpt.cause == CAUSE_LAF &&
        excpt.tval == refault_second_half_vaddr);

    goto_priv(PRIV_M);
    TEST_ASSERT(
        "the same-template other-pair 6B+2B refault probe must keep pair A "
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
        "after the pair-A 6B+2B fault then pair-B 6B+2B refault sequence is "
        "repaired, the same-template other-pair 6B+2B recovery load must "
        "return only pair B's current merged payload instead of stale "
        "splitLoadResp data or merge template from pair A",
        excpt.triggered == false &&
        observed == expected_recovery);

    goto_priv(PRIV_M);
    TEST_ASSERT(
        "the fault-pair 6B+2B fault then other-pair same-template 6B+2B "
        "refault/recovery sequence must leave all four backing doublewords "
        "unchanged after the recovery load",
        read64(fault_low_phys) == fault_low_pattern &&
        read64(fault_high_phys) == fault_high_pattern &&
        read64(refault_low_phys) == refault_low_pattern &&
        read64(refault_high_phys) == refault_high_pattern);

    ai_micro_mab_b2b_reset_env();

    TEST_END(
        "ai_micro_m_crosspage_6b2b_fault_then_other_pair_6b2b_refault_then_"
        "6b2b_recovery_no_stale_merge_template_corner");
}

bool ai_micro_m_crosspage_6b2b_store_fault_then_other_pair_6b2b_refault_then_6b2b_recovery_no_stale_split_template_corner()
{
    TEST_START();

    const enum test_page fault_low_page = SWITCH1;
    const enum test_page fault_high_page = SWITCH2;
    const enum test_page refault_low_page = VSRWX_GI;
    const enum test_page refault_high_page = VSRW_GI;
    const uintptr_t fault_vaddr =
        ai_micro_mab_b2b_cross_page_vaddr(fault_low_page, 6U);
    const uintptr_t fault_second_half_vaddr = fault_vaddr + 6U;
    const uintptr_t refault_vaddr =
        ai_micro_mab_b2b_cross_page_vaddr(refault_low_page, 6U);
    const uintptr_t refault_second_half_vaddr = refault_vaddr + 6U;
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

    const uint64_t fault_low_seed = 0x0badc0ffee123456ULL;
    const uint64_t fault_high_seed = 0xfaceb00c76543210ULL;
    const uint64_t refault_low_initial = 0x1122334455667788ULL;
    const uint64_t refault_high_initial = 0x99aabbccddeeff00ULL;
    const uint64_t refault_low_seed = 0x1029384756a6b7c8ULL;
    const uint64_t refault_high_seed = 0xc8b7a69584736251ULL;
    const uint64_t fault_store_value = 0x55aa33cc77ee11ffULL;
    const uint64_t refault_store_value = 0x2468ace013579bdfULL;
    const uint64_t recovery_store_value = 0xefcdab8967452301ULL;

    const uint64_t expected_fault_low_after_fault = fault_low_seed;
    const uint64_t expected_refault_low_after_refault = refault_low_seed;
    const uint64_t expected_refault_low_after_recovery =
        ai_micro_mab_b2b_expected_low_after_cross_page_store(
            refault_low_seed, recovery_store_value, 6U);
    const uint64_t expected_refault_high_after_recovery =
        ai_micro_mab_b2b_expected_high_after_cross_page_store(
            refault_high_seed, recovery_store_value, 6U);

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
        "the fault-pair cross-page 6B+2B store must fault with SAF on the "
        "true upper-page start address when the fault pair upper page "
        "loses PMP write permission",
        excpt.triggered == true &&
        excpt.cause == CAUSE_SAF &&
        excpt.tval == fault_second_half_vaddr);

    goto_priv(PRIV_M);
    TEST_ASSERT(
        "the fault-pair 6B+2B store fault episode must not expose the fault "
        "pair low-page tail-six-byte side effect and keep the later refault "
        "pair untouched before the same-template other-pair 6B+2B refault "
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
        "after a tail-six-byte fault on pair A, the immediate other-pair "
        "same-template 6B+2B refault must report SAF on pair B's true "
        "upper-page start address instead of reusing pair A's split "
        "template or stale high-half paddr",
        excpt.triggered == true &&
        excpt.cause == CAUSE_SAF &&
        excpt.tval == refault_second_half_vaddr);

    goto_priv(PRIV_M);
    TEST_ASSERT(
        "the same-template other-pair 6B+2B refault probe must keep pair A "
        "unchanged while pair B does not expose the current low-page tail-six-byte "
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
        "after the pair-A 6B+2B fault then pair-B 6B+2B refault sequence is "
        "repaired, the same-template other-pair 6B+2B recovery store must "
        "complete without reusing pair A's split template or stale "
        "high-half paddr",
        excpt.triggered == false);

    goto_priv(PRIV_M);
    TEST_ASSERT(
        "the fault-pair 6B+2B fault then other-pair same-template 6B+2B "
        "refault/recovery sequence must keep pair A unchanged "
        "while pair B reflects only the current 6B+2B overlay",
        read64(fault_low_phys) == expected_fault_low_after_fault &&
        read64(fault_high_phys) == fault_high_seed &&
        read64(refault_low_phys) == expected_refault_low_after_recovery &&
        read64(refault_high_phys) == expected_refault_high_after_recovery);

    ai_micro_mab_b2b_reset_env();

    TEST_END(
        "ai_micro_m_crosspage_6b2b_store_fault_then_other_pair_6b2b_"
        "refault_then_6b2b_recovery_no_stale_split_template_corner");
}

bool ai_micro_m_crosspage_7b1b_fault_then_other_pair_7b1b_refault_then_7b1b_recovery_no_stale_merge_template_corner()
{
    TEST_START();

    const enum test_page fault_low_page = SWITCH1;
    const enum test_page fault_high_page = SWITCH2;
    const enum test_page refault_low_page = VSRWX_GI;
    const enum test_page refault_high_page = VSRW_GI;
    const uintptr_t fault_vaddr =
        ai_micro_mab_b2b_cross_page_vaddr(fault_low_page, 7U);
    const uintptr_t fault_second_half_vaddr = fault_vaddr + 7U;
    const uintptr_t refault_vaddr =
        ai_micro_mab_b2b_cross_page_vaddr(refault_low_page, 7U);
    const uintptr_t refault_second_half_vaddr = refault_vaddr + 7U;
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

    const uint64_t fault_low_pattern = 0x90a1b2c3d4e5f607ULL;
    const uint64_t fault_high_pattern = 0x1827364554637281ULL;
    const uint64_t refault_low_initial = 0xfedcba9876543210ULL;
    const uint64_t refault_high_initial = 0x0f1e2d3c4b5a6978ULL;
    const uint64_t refault_low_pattern = 0x1122446688aacceeULL;
    const uint64_t refault_high_pattern = 0x13579bdf2468ace0ULL;
    const uint64_t expected_recovery =
        ai_micro_mab_b2b_expected_cross_page_ld(
            refault_low_pattern, refault_high_pattern, 7U);
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
        "the fault-pair cross-page 7B+1B load must fault with LAF on the "
        "true upper-page start address when the fault pair upper page "
        "loses PMP read permission",
        excpt.triggered == true &&
        excpt.cause == CAUSE_LAF &&
        excpt.tval == fault_second_half_vaddr);

    goto_priv(PRIV_M);
    TEST_ASSERT(
        "the fault-pair 7B+1B load fault episode must keep both the fault "
        "pair and the later refault pair unchanged before the same-template "
        "other-pair 7B+1B refault load",
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
        "after a tail-one-byte fault on pair A, the immediate other-pair "
        "same-template 7B+1B refault must report LAF on pair B's true "
        "upper-page start address instead of reusing pair A's "
        "splitLoadResp or stale high-byte merge metadata",
        excpt.triggered == true &&
        excpt.cause == CAUSE_LAF &&
        excpt.tval == refault_second_half_vaddr);

    goto_priv(PRIV_M);
    TEST_ASSERT(
        "the same-template other-pair 7B+1B refault probe must keep pair A "
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
        "after the pair-A 7B+1B fault then pair-B 7B+1B refault sequence is "
        "repaired, the same-template other-pair 7B+1B recovery load must "
        "return only pair B's current merged payload instead of stale "
        "splitLoadResp data or high-byte merge metadata from pair A",
        excpt.triggered == false &&
        observed == expected_recovery);

    goto_priv(PRIV_M);
    TEST_ASSERT(
        "the fault-pair 7B+1B fault then other-pair same-template 7B+1B "
        "refault/recovery sequence must leave all four backing doublewords "
        "unchanged after the recovery load",
        read64(fault_low_phys) == fault_low_pattern &&
        read64(fault_high_phys) == fault_high_pattern &&
        read64(refault_low_phys) == refault_low_pattern &&
        read64(refault_high_phys) == refault_high_pattern);

    ai_micro_mab_b2b_reset_env();

    TEST_END(
        "ai_micro_m_crosspage_7b1b_fault_then_other_pair_7b1b_refault_then_"
        "7b1b_recovery_no_stale_merge_template_corner");
}

bool ai_micro_m_crosspage_7b1b_store_fault_then_other_pair_7b1b_refault_then_7b1b_recovery_no_stale_split_template_corner()
{
    TEST_START();

    const enum test_page fault_low_page = SWITCH1;
    const enum test_page fault_high_page = SWITCH2;
    const enum test_page refault_low_page = VSRWX_GI;
    const enum test_page refault_high_page = VSRW_GI;
    const uintptr_t fault_vaddr =
        ai_micro_mab_b2b_cross_page_vaddr(fault_low_page, 7U);
    const uintptr_t fault_second_half_vaddr = fault_vaddr + 7U;
    const uintptr_t refault_vaddr =
        ai_micro_mab_b2b_cross_page_vaddr(refault_low_page, 7U);
    const uintptr_t refault_second_half_vaddr = refault_vaddr + 7U;
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

    const uint64_t fault_low_seed = 0x7766554433221100ULL;
    const uint64_t fault_high_seed = 0x8899aabbccddeeffULL;
    const uint64_t refault_low_initial = 0x1234567890abcdefULL;
    const uint64_t refault_high_initial = 0xf0e1d2c3b4a59687ULL;
    const uint64_t refault_low_seed = 0xcafebabedeadbeefULL;
    const uint64_t refault_high_seed = 0x1029384756a6b7c8ULL;
    const uint64_t fault_store_value = 0x3141592653589793ULL;
    const uint64_t refault_store_value = 0x55aa11cc33ee77ffULL;
    const uint64_t recovery_store_value = 0xefcdab8967452301ULL;

    const uint64_t expected_fault_low_after_fault = fault_low_seed;
    const uint64_t expected_refault_low_after_refault = refault_low_seed;
    const uint64_t expected_refault_low_after_recovery =
        ai_micro_mab_b2b_expected_low_after_cross_page_store(
            refault_low_seed, recovery_store_value, 7U);
    const uint64_t expected_refault_high_after_recovery =
        ai_micro_mab_b2b_expected_high_after_cross_page_store(
            refault_high_seed, recovery_store_value, 7U);

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
        "the fault-pair cross-page 7B+1B store must fault with SAF on the "
        "true upper-page start address when the fault pair upper page "
        "loses PMP write permission",
        excpt.triggered == true &&
        excpt.cause == CAUSE_SAF &&
        excpt.tval == fault_second_half_vaddr);

    goto_priv(PRIV_M);
    TEST_ASSERT(
        "the fault-pair 7B+1B store fault episode must not expose the fault "
        "pair low-page tail-seven-byte side effect and keep the later refault "
        "pair untouched before the same-template other-pair 7B+1B refault "
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
        "after a tail-one-byte fault on pair A, the immediate other-pair "
        "same-template 7B+1B refault must report SAF on pair B's true "
        "upper-page start address instead of reusing pair A's split "
        "template or stale high-byte paddr",
        excpt.triggered == true &&
        excpt.cause == CAUSE_SAF &&
        excpt.tval == refault_second_half_vaddr);

    goto_priv(PRIV_M);
    TEST_ASSERT(
        "the same-template other-pair 7B+1B refault probe must keep pair A "
        "unchanged while pair B does not expose the current low-page tail-seven-byte "
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
        "after the pair-A 7B+1B fault then pair-B 7B+1B refault sequence is "
        "repaired, the same-template other-pair 7B+1B recovery store must "
        "complete without reusing pair A's split template or stale "
        "high-byte paddr",
        excpt.triggered == false);

    goto_priv(PRIV_M);
    TEST_ASSERT(
        "the fault-pair 7B+1B fault then other-pair same-template 7B+1B "
        "refault/recovery sequence must keep pair A unchanged "
        "while pair B reflects only the current 7B+1B overlay",
        read64(fault_low_phys) == expected_fault_low_after_fault &&
        read64(fault_high_phys) == fault_high_seed &&
        read64(refault_low_phys) == expected_refault_low_after_recovery &&
        read64(refault_high_phys) == expected_refault_high_after_recovery);

    ai_micro_mab_b2b_reset_env();

    TEST_END(
        "ai_micro_m_crosspage_7b1b_store_fault_then_other_pair_7b1b_"
        "refault_then_7b1b_recovery_no_stale_split_template_corner");
}

bool ai_micro_m_crosspage_7b1b_fault_then_other_pair_6b2b_refault_then_6b2b_recovery_no_stale_merge_template_corner()
{
    TEST_START();

    const enum test_page fault_low_page = SWITCH1;
    const enum test_page fault_high_page = SWITCH2;
    const enum test_page refault_low_page = VSRWX_GI;
    const enum test_page refault_high_page = VSRW_GI;
    const uintptr_t fault_vaddr =
        ai_micro_mab_b2b_cross_page_vaddr(fault_low_page, 7U);
    const uintptr_t fault_second_half_vaddr = fault_vaddr + 7U;
    const uintptr_t refault_vaddr =
        ai_micro_mab_b2b_cross_page_vaddr(refault_low_page, 6U);
    const uintptr_t refault_second_half_vaddr = refault_vaddr + 6U;
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

    const uint64_t fault_low_pattern = 0x7e6d5c4b3a291807ULL;
    const uint64_t fault_high_pattern = 0x9182736455463728ULL;
    const uint64_t refault_low_initial = 0x1029384756a6b7c8ULL;
    const uint64_t refault_high_initial = 0xdecafbadfeed1234ULL;
    const uint64_t refault_low_pattern = 0x8877665544332211ULL;
    const uint64_t refault_high_pattern = 0xfedcba9876543210ULL;
    const uint64_t expected_recovery =
        ai_micro_mab_b2b_expected_cross_page_ld(
            refault_low_pattern, refault_high_pattern, 6U);
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
        "the fault-pair cross-page 7B+1B load must fault with LAF on the "
        "true upper-page start address when the fault pair upper page "
        "loses PMP read permission",
        excpt.triggered == true &&
        excpt.cause == CAUSE_LAF &&
        excpt.tval == fault_second_half_vaddr);

    goto_priv(PRIV_M);
    TEST_ASSERT(
        "the fault-pair 7B+1B load fault episode must keep both the fault "
        "pair and the later refault pair unchanged before the adjacent "
        "other-pair 6B+2B refault load",
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
        "after a tail-one-byte fault on pair A, the immediate other-pair "
        "adjacent-template 6B+2B refault must report LAF on pair B's true "
        "upper-page start address instead of reusing pair A's splitLoadResp "
        "or stale high-byte merge metadata",
        excpt.triggered == true &&
        excpt.cause == CAUSE_LAF &&
        excpt.tval == refault_second_half_vaddr);

    goto_priv(PRIV_M);
    TEST_ASSERT(
        "the adjacent other-pair 6B+2B refault probe must keep pair A frozen "
        "and leave pair B's refreshed backing image unchanged before "
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
        "after the pair-A 7B+1B fault then pair-B 6B+2B refault sequence is "
        "repaired, the adjacent other-pair 6B+2B recovery load must return "
        "only pair B's current merged payload instead of stale splitLoadResp "
        "data or high-byte merge metadata from pair A",
        excpt.triggered == false &&
        observed == expected_recovery);

    goto_priv(PRIV_M);
    TEST_ASSERT(
        "the fault-pair 7B+1B fault then other-pair 6B+2B refault/recovery "
        "sequence must leave all four backing doublewords unchanged after "
        "the recovery load",
        read64(fault_low_phys) == fault_low_pattern &&
        read64(fault_high_phys) == fault_high_pattern &&
        read64(refault_low_phys) == refault_low_pattern &&
        read64(refault_high_phys) == refault_high_pattern);

    ai_micro_mab_b2b_reset_env();

    TEST_END(
        "ai_micro_m_crosspage_7b1b_fault_then_other_pair_6b2b_refault_then_"
        "6b2b_recovery_no_stale_merge_template_corner");
}

bool ai_micro_m_crosspage_7b1b_store_fault_then_other_pair_6b2b_refault_then_6b2b_recovery_no_stale_split_template_corner()
{
    TEST_START();

    const enum test_page fault_low_page = SWITCH1;
    const enum test_page fault_high_page = SWITCH2;
    const enum test_page refault_low_page = VSRWX_GI;
    const enum test_page refault_high_page = VSRW_GI;
    const uintptr_t fault_vaddr =
        ai_micro_mab_b2b_cross_page_vaddr(fault_low_page, 7U);
    const uintptr_t fault_second_half_vaddr = fault_vaddr + 7U;
    const uintptr_t refault_vaddr =
        ai_micro_mab_b2b_cross_page_vaddr(refault_low_page, 6U);
    const uintptr_t refault_second_half_vaddr = refault_vaddr + 6U;
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

    const uint64_t fault_low_seed = 0x0123456789abcdefULL;
    const uint64_t fault_high_seed = 0x8899aabbccddeeffULL;
    const uint64_t refault_low_initial = 0x7766554433221100ULL;
    const uint64_t refault_high_initial = 0x1827364554637281ULL;
    const uint64_t refault_low_seed = 0x0badc0ffee123456ULL;
    const uint64_t refault_high_seed = 0xfaceb00c76543210ULL;
    const uint64_t fault_store_value = 0x3141592653589793ULL;
    const uint64_t refault_store_value = 0x2468ace013579bdfULL;
    const uint64_t recovery_store_value = 0xefcdab8967452301ULL;

    const uint64_t expected_fault_low_after_fault = fault_low_seed;
    const uint64_t expected_refault_low_after_refault = refault_low_seed;
    const uint64_t expected_refault_low_after_recovery =
        ai_micro_mab_b2b_expected_low_after_cross_page_store(
            refault_low_seed, recovery_store_value, 6U);
    const uint64_t expected_refault_high_after_recovery =
        ai_micro_mab_b2b_expected_high_after_cross_page_store(
            refault_high_seed, recovery_store_value, 6U);

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
        "the fault-pair cross-page 7B+1B store must fault with SAF on the "
        "true upper-page start address when the fault pair upper page "
        "loses PMP write permission",
        excpt.triggered == true &&
        excpt.cause == CAUSE_SAF &&
        excpt.tval == fault_second_half_vaddr);

    goto_priv(PRIV_M);
    TEST_ASSERT(
        "the fault-pair 7B+1B store fault episode must not expose the fault "
        "pair low-page tail-seven-byte side effect and keep the later refault "
        "pair untouched before the adjacent other-pair 6B+2B refault store",
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
        "after a tail-one-byte fault on pair A, the immediate other-pair "
        "adjacent-template 6B+2B refault must report SAF on pair B's true "
        "upper-page start address instead of reusing pair A's split template "
        "or stale high-byte paddr",
        excpt.triggered == true &&
        excpt.cause == CAUSE_SAF &&
        excpt.tval == refault_second_half_vaddr);

    goto_priv(PRIV_M);
    TEST_ASSERT(
        "the adjacent other-pair 6B+2B refault probe must keep pair A frozen "
        "while pair B does not expose the current low-page tail-six-byte "
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
        "after the pair-A 7B+1B fault then pair-B 6B+2B refault sequence is "
        "repaired, the adjacent other-pair 6B+2B recovery store must "
        "complete without reusing pair A's split template or stale "
        "high-byte paddr",
        excpt.triggered == false);

    goto_priv(PRIV_M);
    TEST_ASSERT(
        "the fault-pair 7B+1B fault then other-pair 6B+2B refault/recovery "
        "sequence must keep pair A unchanged by the faulting store while pair "
        "B reflects only the current 6B+2B overlay",
        read64(fault_low_phys) == expected_fault_low_after_fault &&
        read64(fault_high_phys) == fault_high_seed &&
        read64(refault_low_phys) == expected_refault_low_after_recovery &&
        read64(refault_high_phys) == expected_refault_high_after_recovery);

    ai_micro_mab_b2b_reset_env();

TEST_END(
        "ai_micro_m_crosspage_7b1b_store_fault_then_other_pair_6b2b_"
        "refault_then_6b2b_recovery_no_stale_split_template_corner");
}

bool ai_micro_m_crosspage_6b2b_fault_then_other_pair_5b3b_refault_then_5b3b_recovery_no_stale_merge_template_corner()
{
    TEST_START();

    const enum test_page fault_low_page = SWITCH1;
    const enum test_page fault_high_page = SWITCH2;
    const enum test_page refault_low_page = VSRWX_GI;
    const enum test_page refault_high_page = VSRW_GI;
    const uintptr_t fault_vaddr =
        ai_micro_mab_b2b_cross_page_vaddr(fault_low_page, 6U);
    const uintptr_t fault_second_half_vaddr = fault_vaddr + 6U;
    const uintptr_t refault_vaddr =
        ai_micro_mab_b2b_cross_page_vaddr(refault_low_page, 5U);
    const uintptr_t refault_second_half_vaddr = refault_vaddr + 5U;
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

    const uint64_t fault_low_pattern = 0x21436587a9cbed0fULL;
    const uint64_t fault_high_pattern = 0xf0debc9a78563412ULL;
    const uint64_t refault_low_initial = 0x5566778899aabbccULL;
    const uint64_t refault_high_initial = 0xc3d2e1f0a5b49786ULL;
    const uint64_t refault_low_pattern = 0x89abcdef10234567ULL;
    const uint64_t refault_high_pattern = 0x76543210fedcba98ULL;
    const uint64_t expected_recovery =
        ai_micro_mab_b2b_expected_cross_page_ld(
            refault_low_pattern, refault_high_pattern, 5U);
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
        "the fault-pair cross-page 6B+2B load must fault with LAF on the "
        "true upper-page start address when the fault pair upper page "
        "loses PMP read permission",
        excpt.triggered == true &&
        excpt.cause == CAUSE_LAF &&
        excpt.tval == fault_second_half_vaddr);

    goto_priv(PRIV_M);
    TEST_ASSERT(
        "the fault-pair 6B+2B load fault episode must keep both the fault "
        "pair and the later refault pair unchanged before the neighboring "
        "other-pair 5B+3B refault load",
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
        "after a tail-halfword fault on pair A, the immediate other-pair "
        "neighboring 5B+3B refault must report LAF on pair B's true "
        "upper-page start address instead of reusing pair A's splitLoadResp "
        "or stale widened high-half merge metadata",
        excpt.triggered == true &&
        excpt.cause == CAUSE_LAF &&
        excpt.tval == refault_second_half_vaddr);

    goto_priv(PRIV_M);
    TEST_ASSERT(
        "the neighboring other-pair 5B+3B refault probe must keep pair A "
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
        "after the pair-A 6B+2B fault then pair-B 5B+3B refault sequence is "
        "repaired, the neighboring other-pair 5B+3B recovery load must "
        "return only pair B's current merged payload instead of stale "
        "splitLoadResp data or widened merge metadata from pair A",
        excpt.triggered == false &&
        observed == expected_recovery);

    goto_priv(PRIV_M);
    TEST_ASSERT(
        "the fault-pair 6B+2B fault then other-pair 5B+3B refault/recovery "
        "sequence must leave all four backing doublewords unchanged after "
        "the recovery load",
        read64(fault_low_phys) == fault_low_pattern &&
        read64(fault_high_phys) == fault_high_pattern &&
        read64(refault_low_phys) == refault_low_pattern &&
        read64(refault_high_phys) == refault_high_pattern);

    ai_micro_mab_b2b_reset_env();

    TEST_END(
        "ai_micro_m_crosspage_6b2b_fault_then_other_pair_5b3b_refault_then_"
        "5b3b_recovery_no_stale_merge_template_corner");
}
