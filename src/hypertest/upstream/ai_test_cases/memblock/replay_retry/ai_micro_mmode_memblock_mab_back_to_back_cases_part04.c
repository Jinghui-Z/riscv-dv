#include "ai_micro_mmode_memblock_mab_back_to_back_common.h"

bool ai_micro_m_crosspage_6b2b_store_fault_then_other_pair_5b3b_refault_then_5b3b_recovery_no_stale_split_template_corner()
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

    const uint64_t fault_low_seed = 0x02468ace13579bdfULL;
    const uint64_t fault_high_seed = 0xfedcba9876543210ULL;
    const uint64_t refault_low_initial = 0x1122334455667788ULL;
    const uint64_t refault_high_initial = 0x99aabbccddeeff00ULL;
    const uint64_t refault_low_seed = 0x3141592653589793ULL;
    const uint64_t refault_high_seed = 0x0123456789abcdefULL;
    const uint64_t fault_store_value = 0x8877665544332211ULL;
    const uint64_t refault_store_value = 0xa1b2c3d4e5f60718ULL;
    const uint64_t recovery_store_value = 0x0badc0ffee123456ULL;

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
        "pair untouched before the neighboring other-pair 5B+3B refault "
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
        "after a tail-halfword fault on pair A, the immediate other-pair "
        "neighboring 5B+3B refault must report SAF on pair B's true "
        "upper-page start address instead of reusing pair A's split "
        "template or stale widened high-half paddr",
        excpt.triggered == true &&
        excpt.cause == CAUSE_SAF &&
        excpt.tval == refault_second_half_vaddr);

    goto_priv(PRIV_M);
    TEST_ASSERT(
        "the neighboring other-pair 5B+3B refault probe must keep pair A "
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
        "after the pair-A 6B+2B fault then pair-B 5B+3B refault sequence is "
        "repaired, the neighboring other-pair 5B+3B recovery store must "
        "complete without reusing pair A's split template or stale "
        "widened high-half paddr",
        excpt.triggered == false);

    goto_priv(PRIV_M);
    TEST_ASSERT(
        "the fault-pair 6B+2B fault then other-pair 5B+3B refault/recovery "
        "sequence must keep pair A unchanged by the faulting store while pair "
        "B reflects only the current 5B+3B overlay",
        read64(fault_low_phys) == expected_fault_low_after_fault &&
        read64(fault_high_phys) == fault_high_seed &&
        read64(refault_low_phys) == expected_refault_low_after_recovery &&
        read64(refault_high_phys) == expected_refault_high_after_recovery);

    ai_micro_mab_b2b_reset_env();

    TEST_END(
        "ai_micro_m_crosspage_6b2b_store_fault_then_other_pair_5b3b_"
        "refault_then_5b3b_recovery_no_stale_split_template_corner");
}

bool ai_micro_m_crosspage_5b3b_fault_then_other_pair_4b4b_refault_then_4b4b_recovery_no_stale_merge_template_corner()
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

    const uint64_t fault_low_pattern = 0x1032547698badcfeULL;
    const uint64_t fault_high_pattern = 0xefcdab8967452301ULL;
    const uint64_t refault_low_initial = 0x445566778899aabbULL;
    const uint64_t refault_high_initial = 0xbad0c0ffee0ddf00ULL;
    const uint64_t refault_low_pattern = 0x2718281828459045ULL;
    const uint64_t refault_high_pattern = 0x3141592653589793ULL;
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
        "the fault-pair cross-page 5B+3B load must fault with LAF on the "
        "true upper-page start address when the fault pair upper page "
        "loses PMP read permission",
        excpt.triggered == true &&
        excpt.cause == CAUSE_LAF &&
        excpt.tval == fault_second_half_vaddr);

    goto_priv(PRIV_M);
    TEST_ASSERT(
        "the fault-pair 5B+3B load fault episode must keep both the fault "
        "pair and the later refault pair unchanged before the balanced "
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
        "after a widened-low-half fault on pair A, the immediate balanced "
        "other-pair 4B+4B refault must report LAF on pair B's true second-"
        "half start address instead of reusing pair A's splitLoadResp or "
        "stale owner-rebalanced merge metadata",
        excpt.triggered == true &&
        excpt.cause == CAUSE_LAF &&
        excpt.tval == refault_second_half_vaddr);

    goto_priv(PRIV_M);
    TEST_ASSERT(
        "the balanced other-pair 4B+4B refault probe must keep pair A "
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
        "after the pair-A 5B+3B fault then pair-B 4B+4B refault sequence is "
        "repaired, the balanced other-pair 4B+4B recovery load must return "
        "only pair B's current merged payload instead of stale pair-A "
        "widened-template response data",
        excpt.triggered == false &&
        observed == expected_recovery);

    goto_priv(PRIV_M);
    TEST_ASSERT(
        "the fault-pair 5B+3B fault then other-pair 4B+4B refault/recovery "
        "sequence must leave all four backing doublewords unchanged after "
        "the recovery load",
        read64(fault_low_phys) == fault_low_pattern &&
        read64(fault_high_phys) == fault_high_pattern &&
        read64(refault_low_phys) == refault_low_pattern &&
        read64(refault_high_phys) == refault_high_pattern);

    ai_micro_mab_b2b_reset_env();

    TEST_END(
        "ai_micro_m_crosspage_5b3b_fault_then_other_pair_4b4b_refault_then_"
        "4b4b_recovery_no_stale_merge_template_corner");
}

bool ai_micro_m_crosspage_5b3b_store_fault_then_other_pair_4b4b_refault_then_4b4b_recovery_no_stale_split_template_corner()
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

    const uint64_t fault_low_seed = 0xabcdef0123456789ULL;
    const uint64_t fault_high_seed = 0x9876543210fedcbaULL;
    const uint64_t refault_low_initial = 0x1122446688aacceeULL;
    const uint64_t refault_high_initial = 0xfedcba9876543210ULL;
    const uint64_t refault_low_seed = 0x0f1e2d3c4b5a6978ULL;
    const uint64_t refault_high_seed = 0x89abcdef76543210ULL;
    const uint64_t fault_store_value = 0x5566778899aabbccULL;
    const uint64_t refault_store_value = 0xc3d2e1f0a9b8c7d6ULL;
    const uint64_t recovery_store_value = 0x1020304050607081ULL;

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
        "pair untouched before the balanced other-pair 4B+4B refault store",
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
        "after a widened-low-half fault on pair A, the immediate balanced "
        "other-pair 4B+4B refault must report SAF on pair B's true second-"
        "half start address instead of reusing pair A's split template or "
        "stale owner-rebalanced second-half paddr",
        excpt.triggered == true &&
        excpt.cause == CAUSE_SAF &&
        excpt.tval == refault_second_half_vaddr);

    goto_priv(PRIV_M);
    TEST_ASSERT(
        "the balanced other-pair 4B+4B refault probe must keep pair A "
        "unchanged while pair B does not expose the current low-page upper-word "
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
        "after the pair-A 5B+3B fault then pair-B 4B+4B refault sequence is "
        "repaired, the balanced other-pair 4B+4B recovery store must "
        "complete without reusing pair A's split template or stale "
        "owner-rebalanced second-half paddr",
        excpt.triggered == false);

    goto_priv(PRIV_M);
    TEST_ASSERT(
        "the fault-pair 5B+3B fault then other-pair 4B+4B refault/recovery "
        "sequence must keep pair A unchanged by the faulting store while pair "
        "B reflects only the current 4B+4B overlay",
        read64(fault_low_phys) == expected_fault_low_after_fault &&
        read64(fault_high_phys) == fault_high_seed &&
        read64(refault_low_phys) == expected_refault_low_after_recovery &&
        read64(refault_high_phys) == expected_refault_high_after_recovery);

    ai_micro_mab_b2b_reset_env();

    TEST_END(
        "ai_micro_m_crosspage_5b3b_store_fault_then_other_pair_4b4b_"
        "refault_then_4b4b_recovery_no_stale_split_template_corner");
}

bool ai_micro_m_crosspage_4b4b_fault_then_other_pair_5b3b_refault_then_5b3b_recovery_no_stale_merge_template_corner()
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

    const uint64_t fault_low_pattern = 0x8877665544332211ULL;
    const uint64_t fault_high_pattern = 0x1021324354657687ULL;
    const uint64_t refault_low_initial = 0x89abcdef01234567ULL;
    const uint64_t refault_high_initial = 0xfedcba9876543210ULL;
    const uint64_t refault_low_pattern = 0x5566778899aabbccULL;
    const uint64_t refault_high_pattern = 0xc3d2e1f0a9b8c7d6ULL;
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
        "the fault-pair cross-page 4B+4B load must fault with LAF on the "
        "true second-half start address when the fault pair upper page "
        "loses PMP read permission",
        excpt.triggered == true &&
        excpt.cause == CAUSE_LAF &&
        excpt.tval == fault_second_half_vaddr);

    goto_priv(PRIV_M);
    TEST_ASSERT(
        "the fault-pair 4B+4B load fault episode must keep both the fault "
        "pair and the later refault pair unchanged before the mirrored "
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
        "after a balanced fault on pair A, the immediate mirrored other-pair "
        "5B+3B refault must report LAF on pair B's true upper-page start "
        "address instead of reusing pair A's splitLoadResp or stale "
        "owner-flipped merge metadata",
        excpt.triggered == true &&
        excpt.cause == CAUSE_LAF &&
        excpt.tval == refault_second_half_vaddr);

    goto_priv(PRIV_M);
    TEST_ASSERT(
        "the mirrored other-pair 5B+3B refault probe must keep pair A "
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
        "after the pair-A 4B+4B fault then pair-B 5B+3B refault sequence is "
        "repaired, the mirrored other-pair 5B+3B recovery load must return "
        "only pair B's current merged payload instead of stale pair-A "
        "balanced-template response data",
        excpt.triggered == false &&
        observed == expected_recovery);

    goto_priv(PRIV_M);
    TEST_ASSERT(
        "the fault-pair 4B+4B fault then other-pair 5B+3B refault/recovery "
        "sequence must leave all four backing doublewords unchanged after "
        "the recovery load",
        read64(fault_low_phys) == fault_low_pattern &&
        read64(fault_high_phys) == fault_high_pattern &&
        read64(refault_low_phys) == refault_low_pattern &&
        read64(refault_high_phys) == refault_high_pattern);

    ai_micro_mab_b2b_reset_env();

    TEST_END(
        "ai_micro_m_crosspage_4b4b_fault_then_other_pair_5b3b_refault_then_"
        "5b3b_recovery_no_stale_merge_template_corner");
}

bool ai_micro_m_crosspage_4b4b_store_fault_then_other_pair_5b3b_refault_then_5b3b_recovery_no_stale_split_template_corner()
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

    const uint64_t fault_low_seed = 0x1122334455667788ULL;
    const uint64_t fault_high_seed = 0x99aabbccddeeff00ULL;
    const uint64_t refault_low_initial = 0x123456789abcdef0ULL;
    const uint64_t refault_high_initial = 0x0fedcba987654321ULL;
    const uint64_t refault_low_seed = 0x2031425364758697ULL;
    const uint64_t refault_high_seed = 0x98a7b6c5d4e3f201ULL;
    const uint64_t fault_store_value = 0xa1b2c3d4e5f60718ULL;
    const uint64_t refault_store_value = 0x4a5b6c7d8e9fb0c1ULL;
    const uint64_t recovery_store_value = 0xe1f2031425364758ULL;

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
        "the fault-pair cross-page 4B+4B store must fault with SAF on the "
        "true second-half start address when the fault pair upper page "
        "loses PMP write permission",
        excpt.triggered == true &&
        excpt.cause == CAUSE_SAF &&
        excpt.tval == fault_second_half_vaddr);

    goto_priv(PRIV_M);
    TEST_ASSERT(
        "the fault-pair 4B+4B store fault episode must not expose the fault pair low-half side effect and keep the later refault pair "
        "untouched before the mirrored other-pair 5B+3B refault store",
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
        "after a balanced fault on pair A, the immediate mirrored other-pair "
        "5B+3B refault must report SAF on pair B's true upper-page start "
        "address instead of reusing pair A's split template or stale "
        "owner-flipped second-half paddr",
        excpt.triggered == true &&
        excpt.cause == CAUSE_SAF &&
        excpt.tval == refault_second_half_vaddr);

    goto_priv(PRIV_M);
    TEST_ASSERT(
        "the mirrored other-pair 5B+3B refault probe must keep pair A "
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
        "after the pair-A 4B+4B fault then pair-B 5B+3B refault sequence is "
        "repaired, the mirrored other-pair 5B+3B recovery store must "
        "complete without reusing pair A's split template or stale "
        "owner-flipped second-half paddr",
        excpt.triggered == false);

    goto_priv(PRIV_M);
    TEST_ASSERT(
        "the fault-pair 4B+4B fault then other-pair 5B+3B refault/recovery "
        "sequence must keep pair A unchanged by the faulting store while pair "
        "B reflects only the current 5B+3B overlay",
        read64(fault_low_phys) == expected_fault_low_after_fault &&
        read64(fault_high_phys) == fault_high_seed &&
        read64(refault_low_phys) == expected_refault_low_after_recovery &&
        read64(refault_high_phys) == expected_refault_high_after_recovery);

    ai_micro_mab_b2b_reset_env();

    TEST_END(
        "ai_micro_m_crosspage_4b4b_store_fault_then_other_pair_5b3b_"
        "refault_then_5b3b_recovery_no_stale_split_template_corner");
}

bool ai_micro_m_crosspage_5b3b_fault_then_other_pair_6b2b_refault_then_6b2b_recovery_no_stale_merge_template_corner()
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

    const uint64_t fault_low_pattern = 0x3141592653589793ULL;
    const uint64_t fault_high_pattern = 0xc3d2e1f0a9b8c7d6ULL;
    const uint64_t refault_low_initial = 0x0123456789abcdefULL;
    const uint64_t refault_high_initial = 0xfedcba9876543210ULL;
    const uint64_t refault_low_pattern = 0x8877665544332211ULL;
    const uint64_t refault_high_pattern = 0x1029384756a6b7c8ULL;
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
        "the fault-pair cross-page 5B+3B load must fault with LAF on the "
        "true upper-page start address when the fault pair upper page "
        "loses PMP read permission",
        excpt.triggered == true &&
        excpt.cause == CAUSE_LAF &&
        excpt.tval == fault_second_half_vaddr);

    goto_priv(PRIV_M);
    TEST_ASSERT(
        "the fault-pair 5B+3B load fault episode must keep both the fault "
        "pair and the later refault pair unchanged before the neighboring "
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
        "after a mirrored middle-ratio fault on pair A, the immediate "
        "other-pair neighboring 6B+2B refault must report LAF on pair B's "
        "true upper-page start address instead of reusing pair A's "
        "splitLoadResp or stale widened high-half merge metadata",
        excpt.triggered == true &&
        excpt.cause == CAUSE_LAF &&
        excpt.tval == refault_second_half_vaddr);

    goto_priv(PRIV_M);
    TEST_ASSERT(
        "the neighboring other-pair 6B+2B refault probe must keep pair A "
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
        "after the pair-A 5B+3B fault then pair-B 6B+2B refault sequence is "
        "repaired, the neighboring other-pair 6B+2B recovery load must "
        "return only pair B's current merged payload instead of stale "
        "splitLoadResp data or widened high-half merge metadata from "
        "pair A",
        excpt.triggered == false &&
        observed == expected_recovery);

    goto_priv(PRIV_M);
    TEST_ASSERT(
        "the fault-pair 5B+3B fault then other-pair 6B+2B refault/recovery "
        "sequence must leave all four backing doublewords unchanged after "
        "the recovery load",
        read64(fault_low_phys) == fault_low_pattern &&
        read64(fault_high_phys) == fault_high_pattern &&
        read64(refault_low_phys) == refault_low_pattern &&
        read64(refault_high_phys) == refault_high_pattern);

    ai_micro_mab_b2b_reset_env();

    TEST_END(
        "ai_micro_m_crosspage_5b3b_fault_then_other_pair_6b2b_refault_then_"
        "6b2b_recovery_no_stale_merge_template_corner");
}

bool ai_micro_m_crosspage_5b3b_store_fault_then_other_pair_6b2b_refault_then_6b2b_recovery_no_stale_split_template_corner()
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

    const uint64_t fault_low_seed = 0x5566778899aabbccULL;
    const uint64_t fault_high_seed = 0xc3d2e1f0a5b49786ULL;
    const uint64_t refault_low_initial = 0x0fedcba987654321ULL;
    const uint64_t refault_high_initial = 0x123456789abcdef0ULL;
    const uint64_t refault_low_seed = 0x21436587a9cbed0fULL;
    const uint64_t refault_high_seed = 0xf0debc9a78563412ULL;
    const uint64_t fault_store_value = 0xa1b2c3d4e5f60718ULL;
    const uint64_t refault_store_value = 0xfaceb00c76543210ULL;
    const uint64_t recovery_store_value = 0x2468ace013579bdfULL;

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
        "pair untouched before the neighboring other-pair 6B+2B refault "
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
        "other-pair neighboring 6B+2B refault must report SAF on pair B's "
        "true upper-page start address instead of reusing pair A's split "
        "template or stale widened high-half paddr",
        excpt.triggered == true &&
        excpt.cause == CAUSE_SAF &&
        excpt.tval == refault_second_half_vaddr);

    goto_priv(PRIV_M);
    TEST_ASSERT(
        "the neighboring other-pair 6B+2B refault probe must keep pair A "
        "unchanged while pair B does not expose the current low-page tail-six-"
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
        "after the pair-A 5B+3B fault then pair-B 6B+2B refault sequence is "
        "repaired, the neighboring other-pair 6B+2B recovery store must "
        "complete without reusing pair A's split template or stale "
        "widened high-half paddr",
        excpt.triggered == false);

    goto_priv(PRIV_M);
    TEST_ASSERT(
        "the fault-pair 5B+3B fault then other-pair 6B+2B refault/recovery "
        "sequence must keep pair A unchanged by the faulting store while pair "
        "B reflects only the current 6B+2B overlay",
        read64(fault_low_phys) == expected_fault_low_after_fault &&
        read64(fault_high_phys) == fault_high_seed &&
        read64(refault_low_phys) == expected_refault_low_after_recovery &&
        read64(refault_high_phys) == expected_refault_high_after_recovery);

    ai_micro_mab_b2b_reset_env();

    TEST_END(
        "ai_micro_m_crosspage_5b3b_store_fault_then_other_pair_6b2b_"
        "refault_then_6b2b_recovery_no_stale_split_template_corner");
}

bool ai_micro_m_crosspage_6b2b_fault_then_other_pair_7b1b_refault_then_7b1b_recovery_no_stale_merge_template_corner()
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

    const uint64_t fault_low_pattern = 0x89abcdef10234567ULL;
    const uint64_t fault_high_pattern = 0x76543210fedcba98ULL;
    const uint64_t refault_low_initial = 0x1122334455667788ULL;
    const uint64_t refault_high_initial = 0x99aabbccddeeff00ULL;
    const uint64_t refault_low_pattern = 0x7e6d5c4b3a291807ULL;
    const uint64_t refault_high_pattern = 0x9182736455463728ULL;
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
        "the fault-pair cross-page 6B+2B load must fault with LAF on the "
        "true upper-page start address when the fault pair upper page "
        "loses PMP read permission",
        excpt.triggered == true &&
        excpt.cause == CAUSE_LAF &&
        excpt.tval == fault_second_half_vaddr);

    goto_priv(PRIV_M);
    TEST_ASSERT(
        "the fault-pair 6B+2B load fault episode must keep both the fault "
        "pair and the later refault pair unchanged before the extreme "
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
        "after a tail-halfword fault on pair A, the immediate extreme "
        "other-pair 7B+1B refault must report LAF on pair B's true "
        "upper-page start address instead of reusing pair A's splitLoadResp "
        "or stale high-byte merge metadata",
        excpt.triggered == true &&
        excpt.cause == CAUSE_LAF &&
        excpt.tval == refault_second_half_vaddr);

    goto_priv(PRIV_M);
    TEST_ASSERT(
        "the extreme other-pair 7B+1B refault probe must keep pair A frozen "
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
        "after the pair-A 6B+2B fault then pair-B 7B+1B refault sequence is "
        "repaired, the extreme other-pair 7B+1B recovery load must return "
        "only pair B's current merged payload instead of stale splitLoadResp "
        "data or high-byte merge metadata from pair A",
        excpt.triggered == false &&
        observed == expected_recovery);

    goto_priv(PRIV_M);
    TEST_ASSERT(
        "the fault-pair 6B+2B fault then other-pair 7B+1B refault/recovery "
        "sequence must leave all four backing doublewords unchanged after "
        "the recovery load",
        read64(fault_low_phys) == fault_low_pattern &&
        read64(fault_high_phys) == fault_high_pattern &&
        read64(refault_low_phys) == refault_low_pattern &&
        read64(refault_high_phys) == refault_high_pattern);

    ai_micro_mab_b2b_reset_env();

    TEST_END(
        "ai_micro_m_crosspage_6b2b_fault_then_other_pair_7b1b_refault_then_"
        "7b1b_recovery_no_stale_merge_template_corner");
}

bool ai_micro_m_crosspage_6b2b_store_fault_then_other_pair_7b1b_refault_then_7b1b_recovery_no_stale_split_template_corner()
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

    const uint64_t fault_low_seed = 0x02468ace13579bdfULL;
    const uint64_t fault_high_seed = 0xfedcba9876543210ULL;
    const uint64_t refault_low_initial = 0x7766554433221100ULL;
    const uint64_t refault_high_initial = 0x1827364554637281ULL;
    const uint64_t refault_low_seed = 0x0badc0ffee123456ULL;
    const uint64_t refault_high_seed = 0xfaceb00c76543210ULL;
    const uint64_t fault_store_value = 0x8877665544332211ULL;
    const uint64_t refault_store_value = 0x2468ace013579bdfULL;
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
        "pair untouched before the extreme other-pair 7B+1B refault store",
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
        "after a tail-halfword fault on pair A, the immediate extreme "
        "other-pair 7B+1B refault must report SAF on pair B's true "
        "upper-page start address instead of reusing pair A's split "
        "template or stale high-byte paddr",
        excpt.triggered == true &&
        excpt.cause == CAUSE_SAF &&
        excpt.tval == refault_second_half_vaddr);

    goto_priv(PRIV_M);
    TEST_ASSERT(
        "the extreme other-pair 7B+1B refault probe must keep pair A frozen "
        "while pair B does not expose the current low-page tail-seven-byte "
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
        "after the pair-A 6B+2B fault then pair-B 7B+1B refault sequence is "
        "repaired, the extreme other-pair 7B+1B recovery store must "
        "complete without reusing pair A's split template or stale "
        "high-byte paddr",
        excpt.triggered == false);

    goto_priv(PRIV_M);
    TEST_ASSERT(
        "the fault-pair 6B+2B fault then other-pair 7B+1B refault/recovery "
        "sequence must keep pair A unchanged by the faulting store while pair "
        "B reflects only the current 7B+1B overlay",
        read64(fault_low_phys) == expected_fault_low_after_fault &&
        read64(fault_high_phys) == fault_high_seed &&
        read64(refault_low_phys) == expected_refault_low_after_recovery &&
        read64(refault_high_phys) == expected_refault_high_after_recovery);

    ai_micro_mab_b2b_reset_env();

    TEST_END(
        "ai_micro_m_crosspage_6b2b_store_fault_then_other_pair_7b1b_"
        "refault_then_7b1b_recovery_no_stale_split_template_corner");
}

bool ai_micro_m_crosspage_2b6b_fault_then_other_pair_1b7b_refault_then_1b7b_recovery_no_stale_merge_template_corner()
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

    const uint64_t fault_low_pattern = 0xb9a8978675645342ULL;
    const uint64_t fault_high_pattern = 0x15263748596a7b8cULL;
    const uint64_t refault_low_initial = 0x1a2b3c4d5e6f7081ULL;
    const uint64_t refault_high_initial = 0xd4c3b2a1908f7e6dULL;
    const uint64_t refault_low_pattern = 0x708192a3b4c5d6e7ULL;
    const uint64_t refault_high_pattern = 0xf102132435465768ULL;
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
        "the fault-pair cross-page 2B+6B load must fault with LAF on the "
        "true upper-page start address when the fault pair upper page "
        "loses PMP read permission",
        excpt.triggered == true &&
        excpt.cause == CAUSE_LAF &&
        excpt.tval == fault_second_half_vaddr);

    goto_priv(PRIV_M);
    TEST_ASSERT(
        "the fault-pair 2B+6B load fault episode must keep both the fault "
        "pair and the later extreme refault pair unchanged before the "
        "other-pair 1B+7B refault load",
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
        "after a penultimate-edge 2B+6B fault on pair A, the immediate "
        "extreme other-pair 1B+7B refault must report LAF on pair B's true "
        "upper-page start address instead of reusing pair A's splitLoadResp "
        "or stale widened-high-half merge metadata",
        excpt.triggered == true &&
        excpt.cause == CAUSE_LAF &&
        excpt.tval == refault_second_half_vaddr);

    goto_priv(PRIV_M);
    TEST_ASSERT(
        "the extreme other-pair 1B+7B refault probe must keep pair A frozen "
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
        "after the pair-A 2B+6B fault then pair-B 1B+7B refault sequence is "
        "repaired, the extreme other-pair 1B+7B recovery load must return "
        "only pair B's current merged payload instead of stale splitLoadResp "
        "data or widened-high-half merge metadata from pair A",
        excpt.triggered == false &&
        observed == expected_recovery);

    goto_priv(PRIV_M);
    TEST_ASSERT(
        "the fault-pair 2B+6B fault then other-pair 1B+7B refault/recovery "
        "sequence must leave all four backing doublewords unchanged after "
        "the recovery load",
        read64(fault_low_phys) == fault_low_pattern &&
        read64(fault_high_phys) == fault_high_pattern &&
        read64(refault_low_phys) == refault_low_pattern &&
        read64(refault_high_phys) == refault_high_pattern);

    ai_micro_mab_b2b_reset_env();

    TEST_END(
        "ai_micro_m_crosspage_2b6b_fault_then_other_pair_1b7b_refault_then_"
        "1b7b_recovery_no_stale_merge_template_corner");
}

bool ai_micro_m_crosspage_2b6b_store_fault_then_other_pair_1b7b_refault_then_1b7b_recovery_no_stale_split_template_corner()
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

    const uint64_t fault_low_seed = 0xd6c5b4a39281706fULL;
    const uint64_t fault_high_seed = 0x35465768798a9bacULL;
    const uint64_t refault_low_initial = 0x1021324354657687ULL;
    const uint64_t refault_high_initial = 0xf0e1d2c3b4a59687ULL;
    const uint64_t refault_low_seed = 0x5465768798a9bacbULL;
    const uint64_t refault_high_seed = 0xb2a1908f7e6d5c4bULL;
    const uint64_t fault_store_value = 0x6d5c4b3a291807f6ULL;
    const uint64_t refault_store_value = 0x3a4b5c6d7e8fa0b1ULL;
    const uint64_t recovery_store_value = 0x89abcdef10234567ULL;

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
        "the fault-pair cross-page 2B+6B store must fault with SAF on the "
        "true upper-page start address when the fault pair upper page "
        "loses PMP write permission",
        excpt.triggered == true &&
        excpt.cause == CAUSE_SAF &&
        excpt.tval == fault_second_half_vaddr);

    goto_priv(PRIV_M);
    TEST_ASSERT(
        "the fault-pair 2B+6B store fault episode must not expose the fault "
        "pair low-page tail-two-byte side effect and keep the later extreme "
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
        "after a penultimate-edge 2B+6B fault on pair A, the immediate "
        "extreme other-pair 1B+7B refault must report SAF on pair B's true "
        "upper-page start address instead of reusing pair A's split "
        "template or stale widened-high-half paddr",
        excpt.triggered == true &&
        excpt.cause == CAUSE_SAF &&
        excpt.tval == refault_second_half_vaddr);

    goto_priv(PRIV_M);
    TEST_ASSERT(
        "the extreme other-pair 1B+7B refault probe must keep pair A frozen "
        "while pair B does not expose the current low-page tail-byte side effect "
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
        "after the pair-A 2B+6B fault then pair-B 1B+7B refault sequence is "
        "repaired, the extreme other-pair 1B+7B recovery store must "
        "complete without reusing pair A's split template or stale "
        "widened-high-half paddr",
        excpt.triggered == false);

    goto_priv(PRIV_M);
    TEST_ASSERT(
        "the fault-pair 2B+6B fault then other-pair 1B+7B refault/recovery "
        "sequence must keep pair A unchanged by the faulting store while pair "
        "B reflects only the current 1B+7B overlay",
        read64(fault_low_phys) == expected_fault_low_after_fault &&
        read64(fault_high_phys) == fault_high_seed &&
        read64(refault_low_phys) == expected_refault_low_after_recovery &&
        read64(refault_high_phys) == expected_refault_high_after_recovery);

    ai_micro_mab_b2b_reset_env();

    TEST_END(
        "ai_micro_m_crosspage_2b6b_store_fault_then_other_pair_1b7b_"
        "refault_then_1b7b_recovery_no_stale_split_template_corner");
}
