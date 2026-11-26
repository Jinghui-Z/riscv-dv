#include "ai_micro_mmode_memblock_common.h"

bool ai_micro_m_store_cross_16b_second_split_access_fault_repeated_then_same_address_valid_store_recovers_corner()
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
    const uint64_t fault_store_value = 0x7766554433221100ULL;
    const uint64_t recovery_store_value = 0x0f1e2d3c4b5a6978ULL;
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
    TEST_ASSERT("the first no-H same-page cross-16B split-store probe reports store access fault with the original misaligned VA when the second split is PMP-denied",
        excpt.triggered == true &&
        excpt.cause == CAUSE_SAF &&
        excpt.tval == ai_micro_m_memblock_cross_16b_second_half_vaddr(vaddr)
    );

    ai_micro_m_memblock_enable_mprv_supervisor_translation();
    TEST_SETUP_EXCEPT();
    sd(vaddr, fault_store_value);
    TEST_ASSERT("repeating the same no-H same-page cross-16B split-store probe keeps reporting the same second-split store access fault instead of reviving stale older metadata",
        excpt.triggered == true &&
        excpt.cause == CAUSE_SAF &&
        excpt.tval == ai_micro_m_memblock_cross_16b_second_half_vaddr(vaddr)
    );

    goto_priv(PRIV_M);
    TEST_ASSERT("the repeated second-split-only same-page cross-16B store access faults still leave both aligned doublewords unchanged before repair",
        read64(low_phys) == low_pattern &&
        read64(high_phys) == high_pattern
    );

    ai_micro_m_memblock_restore_pmp_full_access();

    ai_micro_m_memblock_enable_mprv_supervisor_translation();
    TEST_SETUP_EXCEPT();
    sd(vaddr, recovery_store_value);
    TEST_ASSERT("after restoring PMP write permission, the same no-H cross-16B split store recovers successfully even after the repeated second-split faults",
        excpt.triggered == false
    );

    goto_priv(PRIV_M);
    TEST_ASSERT("the repeated-fault same-page cross-16B store recovery must update both aligned doublewords with the expected 4B+4B merged store bytes",
        read64(low_phys) == expected_low_after_recovery &&
        read64(high_phys) == expected_high_after_recovery
    );

    ai_micro_m_memblock_reset_env();

    TEST_END();
}

bool ai_micro_m_load_cross_16b_second_split_access_fault_repeated_then_different_address_valid_load_recovers_corner()
{
    TEST_START();

    const enum test_page page = SWITCH1;
    const uintptr_t fault_offset = AI_MICRO_MEMBLOCK_CROSS_16B_OFFSET;
    const uintptr_t survivor_offset =
        AI_MICRO_MEMBLOCK_CROSS_16B_BOUNDARY1_SD_4B4B_OFFSET;
    const uintptr_t fault_vaddr =
        ai_micro_m_memblock_cross_16b_addr_with_offset(page, fault_offset);
    const uintptr_t survivor_vaddr =
        ai_micro_m_memblock_cross_16b_addr_with_offset(page, survivor_offset);
    const uintptr_t fault_low_phys =
        ai_micro_m_memblock_cross_16b_low_phys_with_offset(page, fault_offset);
    const uintptr_t fault_high_phys =
        ai_micro_m_memblock_cross_16b_high_phys_with_offset(page, fault_offset);
    const uintptr_t survivor_low_phys =
        ai_micro_m_memblock_cross_16b_low_phys_with_offset(page, survivor_offset);
    const uintptr_t survivor_high_phys =
        ai_micro_m_memblock_cross_16b_high_phys_with_offset(page, survivor_offset);
    const uintptr_t deny_start = fault_high_phys;
    const uintptr_t deny_end = fault_high_phys + PAGE_SIZE;
    const uint64_t fault_low_pattern = 0x0123456789abcdefULL;
    const uint64_t fault_high_pattern = 0xfedcba9876543210ULL;
    const uint64_t survivor_low_pattern = 0x7766554433221100ULL;
    const uint64_t survivor_high_pattern = 0xffeeddccbbaa9988ULL;
    const uint64_t expected_survivor_value =
        ai_micro_m_memblock_expected_cross_page_4b4b_load(
            survivor_low_pattern, survivor_high_pattern);
    uint64_t observed_value = 0ULL;

    goto_priv(PRIV_M);
    ai_micro_m_memblock_reset_env();
    write64(fault_low_phys, fault_low_pattern);
    write64(fault_high_phys, fault_high_pattern);
    write64(survivor_low_phys, survivor_low_pattern);
    write64(survivor_high_phys, survivor_high_pattern);
    ai_micro_m_memblock_configure_cross_16b_pmp_window(
        deny_start, deny_end, AI_MICRO_MEMBLOCK_PMP_TOR_NO_R);

    ai_micro_m_memblock_prepare_supervisor_translation();
    ai_micro_m_memblock_set_cross_16b_leaves(PTE_V | PTE_AD | PTE_RWX);
    sfence_vma();

    ai_micro_m_memblock_enable_mprv_supervisor_translation();
    TEST_SETUP_EXCEPT();
    observed_value = ld(fault_vaddr);
    TEST_ASSERT("the first no-H same-page cross-16B split-load probe on address A reports load access fault with the original misaligned VA when the second split is PMP-denied",
        excpt.triggered == true &&
        excpt.cause == CAUSE_LAF &&
        excpt.tval == ai_micro_m_memblock_cross_16b_second_half_vaddr(fault_vaddr)
    );

    ai_micro_m_memblock_enable_mprv_supervisor_translation();
    TEST_SETUP_EXCEPT();
    observed_value = ld(fault_vaddr);
    TEST_ASSERT("repeating the no-H same-page cross-16B split-load fault on address A keeps the same precise metadata before the different-address recovery probe",
        excpt.triggered == true &&
        excpt.cause == CAUSE_LAF &&
        excpt.tval == ai_micro_m_memblock_cross_16b_second_half_vaddr(fault_vaddr)
    );

    goto_priv(PRIV_M);
    TEST_ASSERT("before testing different-address recovery, the repeated second-split-only same-page cross-16B load faults on address A leave both aligned doublewords of address A and address B untouched",
        read64(fault_low_phys) == fault_low_pattern &&
        read64(fault_high_phys) == fault_high_pattern &&
        read64(survivor_low_phys) == survivor_low_pattern &&
        read64(survivor_high_phys) == survivor_high_pattern
    );

    ai_micro_m_memblock_restore_pmp_full_access();

    ai_micro_m_memblock_enable_mprv_supervisor_translation();
    TEST_SETUP_EXCEPT();
    observed_value = ld(survivor_vaddr);
    TEST_ASSERT("after the repeated no-H same-page cross-16B load fault episode on address A is repaired, a different same-page cross-16B load on address B must complete without inheriting stale exception metadata from address A",
        excpt.triggered == false &&
        observed_value == expected_survivor_value
    );

    goto_priv(PRIV_M);
    TEST_ASSERT("the different-address no-H same-page cross-16B load recovery probe leaves both aligned doublewords of address A and address B unchanged because both accesses are read-only",
        read64(fault_low_phys) == fault_low_pattern &&
        read64(fault_high_phys) == fault_high_pattern &&
        read64(survivor_low_phys) == survivor_low_pattern &&
        read64(survivor_high_phys) == survivor_high_pattern
    );

    ai_micro_m_memblock_reset_env();

    TEST_END();
}

bool ai_micro_m_load_cross_16b_second_split_access_fault_repeated_then_different_address_valid_store_recovers_corner()
{
    TEST_START();

    const enum test_page page = SWITCH1;
    const uintptr_t fault_offset = AI_MICRO_MEMBLOCK_CROSS_16B_OFFSET;
    const uintptr_t survivor_offset =
        AI_MICRO_MEMBLOCK_CROSS_16B_BOUNDARY1_SD_4B4B_OFFSET;
    const uintptr_t fault_vaddr =
        ai_micro_m_memblock_cross_16b_addr_with_offset(page, fault_offset);
    const uintptr_t survivor_vaddr =
        ai_micro_m_memblock_cross_16b_addr_with_offset(page, survivor_offset);
    const uintptr_t fault_low_phys =
        ai_micro_m_memblock_cross_16b_low_phys_with_offset(page, fault_offset);
    const uintptr_t fault_high_phys =
        ai_micro_m_memblock_cross_16b_high_phys_with_offset(page, fault_offset);
    const uintptr_t survivor_low_phys =
        ai_micro_m_memblock_cross_16b_low_phys_with_offset(page, survivor_offset);
    const uintptr_t survivor_high_phys =
        ai_micro_m_memblock_cross_16b_high_phys_with_offset(page, survivor_offset);
    const uintptr_t deny_start = fault_high_phys;
    const uintptr_t deny_end = fault_high_phys + PAGE_SIZE;
    const uint64_t fault_low_pattern = 0x21436587a9cbed0fULL;
    const uint64_t fault_high_pattern = 0xf0debc9a78563412ULL;
    const uint64_t survivor_low_pattern = 0x1021324354657687ULL;
    const uint64_t survivor_high_pattern = 0x98badcfe10325476ULL;
    const uint64_t survivor_store_value = 0x89abcdef01234567ULL;
    const uint64_t expected_survivor_low =
        ai_micro_m_memblock_expected_low_after_4b_store(
            survivor_low_pattern, survivor_store_value);
    const uint64_t expected_survivor_high =
        ai_micro_m_memblock_expected_high_after_4b_store(
            survivor_high_pattern, survivor_store_value);
    uint64_t observed_value = 0ULL;

    goto_priv(PRIV_M);
    ai_micro_m_memblock_reset_env();
    write64(fault_low_phys, fault_low_pattern);
    write64(fault_high_phys, fault_high_pattern);
    write64(survivor_low_phys, survivor_low_pattern);
    write64(survivor_high_phys, survivor_high_pattern);
    ai_micro_m_memblock_configure_cross_16b_pmp_window(
        deny_start, deny_end, AI_MICRO_MEMBLOCK_PMP_TOR_NO_R);

    ai_micro_m_memblock_prepare_supervisor_translation();
    ai_micro_m_memblock_set_cross_16b_leaves(PTE_V | PTE_AD | PTE_RWX);
    sfence_vma();

    ai_micro_m_memblock_enable_mprv_supervisor_translation();
    TEST_SETUP_EXCEPT();
    observed_value = ld(fault_vaddr);
    TEST_ASSERT("the first no-H same-page cross-16B split-load probe on address A reports load access fault with the original misaligned VA before the different-address store producer-switch recovery probe",
        excpt.triggered == true &&
        excpt.cause == CAUSE_LAF &&
        excpt.tval == ai_micro_m_memblock_cross_16b_second_half_vaddr(fault_vaddr)
    );

    ai_micro_m_memblock_enable_mprv_supervisor_translation();
    TEST_SETUP_EXCEPT();
    observed_value = ld(fault_vaddr);
    TEST_ASSERT("repeating the no-H same-page cross-16B split-load fault on address A keeps the same precise metadata before the different-address store producer-switch recovery probe",
        excpt.triggered == true &&
        excpt.cause == CAUSE_LAF &&
        excpt.tval == ai_micro_m_memblock_cross_16b_second_half_vaddr(fault_vaddr)
    );

    goto_priv(PRIV_M);
    TEST_ASSERT("before testing the different-address store producer-switch recovery probe, the repeated second-split-only same-page cross-16B load faults on address A leave both aligned doublewords of address A and address B untouched",
        read64(fault_low_phys) == fault_low_pattern &&
        read64(fault_high_phys) == fault_high_pattern &&
        read64(survivor_low_phys) == survivor_low_pattern &&
        read64(survivor_high_phys) == survivor_high_pattern
    );

    ai_micro_m_memblock_restore_pmp_full_access();

    ai_micro_m_memblock_enable_mprv_supervisor_translation();
    TEST_SETUP_EXCEPT();
    sd(survivor_vaddr, survivor_store_value);
    TEST_ASSERT("after the repeated no-H same-page cross-16B load fault episode on address A is repaired, a different same-page cross-16B store on address B must complete without inheriting stale load-side exception metadata from address A",
        excpt.triggered == false
    );

    goto_priv(PRIV_M);
    TEST_ASSERT("the different-address no-H same-page cross-16B store producer-switch recovery probe updates only address B with the expected 4B+4B merged bytes and leaves address A untouched",
        read64(fault_low_phys) == fault_low_pattern &&
        read64(fault_high_phys) == fault_high_pattern &&
        read64(survivor_low_phys) == expected_survivor_low &&
        read64(survivor_high_phys) == expected_survivor_high
    );

    ai_micro_m_memblock_reset_env();

    TEST_END();
}

bool ai_micro_m_load_cross_16b_second_split_access_fault_repeated_then_same_address_valid_load_then_different_address_load_fault_refreshes_new_tval_corner()
{
    TEST_START();

    const enum test_page page = SWITCH1;
    const uintptr_t first_offset = AI_MICRO_MEMBLOCK_CROSS_16B_OFFSET;
    const uintptr_t second_offset =
        AI_MICRO_MEMBLOCK_CROSS_16B_BOUNDARY1_SD_4B4B_OFFSET;
    const uintptr_t first_vaddr =
        ai_micro_m_memblock_cross_16b_addr_with_offset(page, first_offset);
    const uintptr_t second_vaddr =
        ai_micro_m_memblock_cross_16b_addr_with_offset(page, second_offset);
    const uintptr_t first_low_phys =
        ai_micro_m_memblock_cross_16b_low_phys_with_offset(page, first_offset);
    const uintptr_t first_high_phys =
        ai_micro_m_memblock_cross_16b_high_phys_with_offset(page, first_offset);
    const uintptr_t second_low_phys =
        ai_micro_m_memblock_cross_16b_low_phys_with_offset(page, second_offset);
    const uintptr_t second_high_phys =
        ai_micro_m_memblock_cross_16b_high_phys_with_offset(page, second_offset);
    const uint64_t first_low_pattern = 0x0123456789abcdefULL;
    const uint64_t first_high_pattern = 0xfedcba9876543210ULL;
    const uint64_t second_low_pattern = 0x7766554433221100ULL;
    const uint64_t second_high_pattern = 0xffeeddccbbaa9988ULL;
    const uint64_t expected_first_value =
        ai_micro_m_memblock_expected_cross_page_4b4b_load(
            first_low_pattern, first_high_pattern);
    uint64_t observed_value = 0ULL;

    goto_priv(PRIV_M);
    ai_micro_m_memblock_reset_env();
    write64(first_low_phys, first_low_pattern);
    write64(first_high_phys, first_high_pattern);
    write64(second_low_phys, second_low_pattern);
    write64(second_high_phys, second_high_pattern);
    ai_micro_m_memblock_configure_cross_16b_pmp_window(
        first_high_phys, first_high_phys + PAGE_SIZE,
        AI_MICRO_MEMBLOCK_PMP_TOR_NO_R);

    ai_micro_m_memblock_prepare_supervisor_translation();
    ai_micro_m_memblock_set_cross_16b_leaves(PTE_V | PTE_AD | PTE_RWX);
    sfence_vma();

    TEST_SETUP_EXCEPT();
    observed_value = ai_micro_m_memblock_translated_scalar_load(
        false, sizeof(uint64_t), first_vaddr);
    TEST_ASSERT("the first no-H same-page cross-16B repair-clear then new-load-fault probe reports load access fault with tval bound to the original address A",
        excpt.triggered == true &&
        excpt.cause == CAUSE_LAF &&
        excpt.tval == ai_micro_m_memblock_cross_16b_second_half_vaddr(first_vaddr)
    );

    TEST_SETUP_EXCEPT();
    observed_value = ai_micro_m_memblock_translated_scalar_load(
        false, sizeof(uint64_t), first_vaddr);
    TEST_ASSERT("repeating the same no-H same-page cross-16B load fault on address A keeps the same precise metadata before the repair-clear new-load-fault probe",
        excpt.triggered == true &&
        excpt.cause == CAUSE_LAF &&
        excpt.tval == ai_micro_m_memblock_cross_16b_second_half_vaddr(first_vaddr)
    );

    goto_priv(PRIV_M);
    TEST_ASSERT("before repairing the no-H same-page cross-16B repeated load fault on address A, both address windows remain unchanged",
        read64(first_low_phys) == first_low_pattern &&
        read64(first_high_phys) == first_high_pattern &&
        read64(second_low_phys) == second_low_pattern &&
        read64(second_high_phys) == second_high_pattern
    );

    ai_micro_m_memblock_restore_pmp_full_access();

    TEST_SETUP_EXCEPT();
    observed_value = ai_micro_m_memblock_translated_scalar_load(
        false, sizeof(uint64_t), first_vaddr);
    TEST_ASSERT("after repairing the repeated no-H same-page cross-16B load fault on address A, the same-address valid load returns the expected value before the younger load fault on address B is injected",
        excpt.triggered == false &&
        observed_value == expected_first_value
    );

    goto_priv(PRIV_M);
    TEST_ASSERT("the repaired same-address no-H same-page cross-16B load remains read-only and leaves both address windows unchanged",
        read64(first_low_phys) == first_low_pattern &&
        read64(first_high_phys) == first_high_pattern &&
        read64(second_low_phys) == second_low_pattern &&
        read64(second_high_phys) == second_high_pattern
    );

    ai_micro_m_memblock_configure_cross_16b_pmp_window(
        second_high_phys, second_high_phys + PAGE_SIZE,
        AI_MICRO_MEMBLOCK_PMP_TOR_NO_R);

    TEST_SETUP_EXCEPT();
    observed_value = ai_micro_m_memblock_translated_scalar_load(
        false, sizeof(uint64_t), second_vaddr);
    TEST_ASSERT("after the repaired no-H same-page cross-16B load on address A has already completed, a younger different-address load fault on address B must refresh tval to address B instead of reusing stale exception metadata from address A",
        excpt.triggered == true &&
        excpt.cause == CAUSE_LAF &&
        excpt.tval == ai_micro_m_memblock_cross_16b_second_half_vaddr(second_vaddr)
    );

    goto_priv(PRIV_M);
    TEST_ASSERT("the repair-clear then new-load-fault no-H same-page cross-16B probe leaves both address windows unchanged because both accesses are read-only",
        read64(first_low_phys) == first_low_pattern &&
        read64(first_high_phys) == first_high_pattern &&
        read64(second_low_phys) == second_low_pattern &&
        read64(second_high_phys) == second_high_pattern
    );

    ai_micro_m_memblock_reset_env();

    TEST_END();
}

bool ai_micro_m_store_cross_16b_second_split_access_fault_repeated_then_same_address_valid_store_then_different_address_store_fault_refreshes_new_tval_corner()
{
    TEST_START();

    const enum test_page page = SWITCH1;
    const uintptr_t first_offset = AI_MICRO_MEMBLOCK_CROSS_16B_OFFSET;
    const uintptr_t second_offset =
        AI_MICRO_MEMBLOCK_CROSS_16B_BOUNDARY1_SD_4B4B_OFFSET;
    const uintptr_t first_vaddr =
        ai_micro_m_memblock_cross_16b_addr_with_offset(page, first_offset);
    const uintptr_t second_vaddr =
        ai_micro_m_memblock_cross_16b_addr_with_offset(page, second_offset);
    const uintptr_t first_low_phys =
        ai_micro_m_memblock_cross_16b_low_phys_with_offset(page, first_offset);
    const uintptr_t first_high_phys =
        ai_micro_m_memblock_cross_16b_high_phys_with_offset(page, first_offset);
    const uintptr_t second_low_phys =
        ai_micro_m_memblock_cross_16b_low_phys_with_offset(page, second_offset);
    const uintptr_t second_high_phys =
        ai_micro_m_memblock_cross_16b_high_phys_with_offset(page, second_offset);
    const uint64_t first_low_pattern = 0x1021324354657687ULL;
    const uint64_t first_high_pattern = 0x98badcfe10325476ULL;
    const uint64_t second_low_pattern = 0x0f1e2d3c4b5a6978ULL;
    const uint64_t second_high_pattern = 0x8877665544332211ULL;
    const uint64_t first_store_value = 0xa1b2c3d4e5f60718ULL;
    const uint64_t second_store_value = 0x89abcdef01234567ULL;
    const uint64_t expected_first_low =
        ai_micro_m_memblock_expected_low_after_4b_store(
            first_low_pattern, first_store_value);
    const uint64_t expected_first_high =
        ai_micro_m_memblock_expected_high_after_4b_store(
            first_high_pattern, first_store_value);

    goto_priv(PRIV_M);
    ai_micro_m_memblock_reset_env();
    write64(first_low_phys, first_low_pattern);
    write64(first_high_phys, first_high_pattern);
    write64(second_low_phys, second_low_pattern);
    write64(second_high_phys, second_high_pattern);
    ai_micro_m_memblock_configure_cross_16b_pmp_window(
        first_high_phys, first_high_phys + PAGE_SIZE,
        AI_MICRO_MEMBLOCK_PMP_TOR_NO_W);

    ai_micro_m_memblock_prepare_supervisor_translation();
    ai_micro_m_memblock_set_cross_16b_leaves(PTE_V | PTE_AD | PTE_RWX);
    sfence_vma();

    TEST_SETUP_EXCEPT();
    ai_micro_m_memblock_translated_sd(first_vaddr, first_store_value);
    TEST_ASSERT("the first no-H same-page cross-16B repair-clear then new-store-fault probe reports store access fault with tval bound to the original address A",
        excpt.triggered == true &&
        excpt.cause == CAUSE_SAF &&
        excpt.tval == ai_micro_m_memblock_cross_16b_second_half_vaddr(first_vaddr)
    );

    TEST_SETUP_EXCEPT();
    ai_micro_m_memblock_translated_sd(first_vaddr, first_store_value);
    TEST_ASSERT("repeating the same no-H same-page cross-16B store fault on address A keeps the same precise metadata before the repair-clear new-store-fault probe",
        excpt.triggered == true &&
        excpt.cause == CAUSE_SAF &&
        excpt.tval == ai_micro_m_memblock_cross_16b_second_half_vaddr(first_vaddr)
    );

    goto_priv(PRIV_M);
    TEST_ASSERT("before repairing the no-H same-page cross-16B repeated store fault on address A, both address windows remain unchanged",
        read64(first_low_phys) == first_low_pattern &&
        read64(first_high_phys) == first_high_pattern &&
        read64(second_low_phys) == second_low_pattern &&
        read64(second_high_phys) == second_high_pattern
    );

    ai_micro_m_memblock_restore_pmp_full_access();

    TEST_SETUP_EXCEPT();
    ai_micro_m_memblock_translated_sd(first_vaddr, first_store_value);
    TEST_ASSERT("after repairing the repeated no-H same-page cross-16B store fault on address A, the same-address valid store commits before the younger store fault on address B is injected",
        excpt.triggered == false
    );

    goto_priv(PRIV_M);
    TEST_ASSERT("the repaired same-address no-H same-page cross-16B store updates only address A with the expected 4B+4B merged bytes",
        read64(first_low_phys) == expected_first_low &&
        read64(first_high_phys) == expected_first_high &&
        read64(second_low_phys) == second_low_pattern &&
        read64(second_high_phys) == second_high_pattern
    );

    ai_micro_m_memblock_configure_cross_16b_pmp_window(
        second_high_phys, second_high_phys + PAGE_SIZE,
        AI_MICRO_MEMBLOCK_PMP_TOR_NO_W);

    TEST_SETUP_EXCEPT();
    ai_micro_m_memblock_translated_sd(second_vaddr, second_store_value);
    TEST_ASSERT("after the repaired no-H same-page cross-16B store on address A has already completed, a younger different-address store fault on address B must refresh tval to address B instead of reusing stale exception metadata from address A",
        excpt.triggered == true &&
        excpt.cause == CAUSE_SAF &&
        excpt.tval == ai_micro_m_memblock_cross_16b_second_half_vaddr(second_vaddr)
    );

    goto_priv(PRIV_M);
    TEST_ASSERT("the repair-clear then new-store-fault no-H same-page cross-16B probe preserves the already-updated address A while keeping address B unchanged under the younger denied store",
        read64(first_low_phys) == expected_first_low &&
        read64(first_high_phys) == expected_first_high &&
        read64(second_low_phys) == second_low_pattern &&
        read64(second_high_phys) == second_high_pattern
    );

    ai_micro_m_memblock_reset_env();

    TEST_END();
}

bool ai_micro_m_load_cross_16b_second_split_access_fault_repeated_then_same_address_valid_load_then_different_address_store_fault_refreshes_new_cause_tval_corner()
{
    TEST_START();

    const enum test_page page = SWITCH1;
    const uintptr_t first_offset = AI_MICRO_MEMBLOCK_CROSS_16B_OFFSET;
    const uintptr_t second_offset =
        AI_MICRO_MEMBLOCK_CROSS_16B_BOUNDARY1_SD_4B4B_OFFSET;
    const uintptr_t first_vaddr =
        ai_micro_m_memblock_cross_16b_addr_with_offset(page, first_offset);
    const uintptr_t second_vaddr =
        ai_micro_m_memblock_cross_16b_addr_with_offset(page, second_offset);
    const uintptr_t first_low_phys =
        ai_micro_m_memblock_cross_16b_low_phys_with_offset(page, first_offset);
    const uintptr_t first_high_phys =
        ai_micro_m_memblock_cross_16b_high_phys_with_offset(page, first_offset);
    const uintptr_t second_low_phys =
        ai_micro_m_memblock_cross_16b_low_phys_with_offset(page, second_offset);
    const uintptr_t second_high_phys =
        ai_micro_m_memblock_cross_16b_high_phys_with_offset(page, second_offset);
    const uint64_t first_low_pattern = 0x21436587a9cbed0fULL;
    const uint64_t first_high_pattern = 0xf0debc9a78563412ULL;
    const uint64_t second_low_pattern = 0x1021324354657687ULL;
    const uint64_t second_high_pattern = 0x98badcfe10325476ULL;
    const uint64_t expected_first_value =
        ai_micro_m_memblock_expected_cross_page_4b4b_load(
            first_low_pattern, first_high_pattern);
    const uint64_t second_store_value = 0x89abcdef01234567ULL;
    uint64_t observed_value = 0ULL;

    goto_priv(PRIV_M);
    ai_micro_m_memblock_reset_env();
    write64(first_low_phys, first_low_pattern);
    write64(first_high_phys, first_high_pattern);
    write64(second_low_phys, second_low_pattern);
    write64(second_high_phys, second_high_pattern);
    ai_micro_m_memblock_configure_cross_16b_pmp_window(
        first_high_phys, first_high_phys + PAGE_SIZE,
        AI_MICRO_MEMBLOCK_PMP_TOR_NO_R);

    ai_micro_m_memblock_prepare_supervisor_translation();
    ai_micro_m_memblock_set_cross_16b_leaves(PTE_V | PTE_AD | PTE_RWX);
    sfence_vma();

    TEST_SETUP_EXCEPT();
    observed_value = ai_micro_m_memblock_translated_scalar_load(
        false, sizeof(uint64_t), first_vaddr);
    TEST_ASSERT("the first no-H same-page cross-16B repair-clear then load-to-store new-fault probe reports load access fault with tval bound to the original address A",
        excpt.triggered == true &&
        excpt.cause == CAUSE_LAF &&
        excpt.tval == ai_micro_m_memblock_cross_16b_second_half_vaddr(first_vaddr)
    );

    TEST_SETUP_EXCEPT();
    observed_value = ai_micro_m_memblock_translated_scalar_load(
        false, sizeof(uint64_t), first_vaddr);
    TEST_ASSERT("repeating the same no-H same-page cross-16B load fault on address A keeps the same precise metadata before the repair-clear load-to-store new-fault probe",
        excpt.triggered == true &&
        excpt.cause == CAUSE_LAF &&
        excpt.tval == ai_micro_m_memblock_cross_16b_second_half_vaddr(first_vaddr)
    );

    goto_priv(PRIV_M);
    TEST_ASSERT("before repairing the no-H same-page cross-16B repeated load fault on address A for the load-to-store new-fault probe, both address windows remain unchanged",
        read64(first_low_phys) == first_low_pattern &&
        read64(first_high_phys) == first_high_pattern &&
        read64(second_low_phys) == second_low_pattern &&
        read64(second_high_phys) == second_high_pattern
    );

    ai_micro_m_memblock_restore_pmp_full_access();

    TEST_SETUP_EXCEPT();
    observed_value = ai_micro_m_memblock_translated_scalar_load(
        false, sizeof(uint64_t), first_vaddr);
    TEST_ASSERT("after repairing the repeated no-H same-page cross-16B load fault on address A, the same-address valid load returns the expected value before the younger store fault on address B is injected",
        excpt.triggered == false &&
        observed_value == expected_first_value
    );

    goto_priv(PRIV_M);
    TEST_ASSERT("the repaired same-address no-H same-page cross-16B load in the load-to-store new-fault probe remains read-only and leaves both address windows unchanged",
        read64(first_low_phys) == first_low_pattern &&
        read64(first_high_phys) == first_high_pattern &&
        read64(second_low_phys) == second_low_pattern &&
        read64(second_high_phys) == second_high_pattern
    );

    ai_micro_m_memblock_configure_cross_16b_pmp_window(
        second_high_phys, second_high_phys + PAGE_SIZE,
        AI_MICRO_MEMBLOCK_PMP_TOR_NO_W);

    TEST_SETUP_EXCEPT();
    ai_micro_m_memblock_translated_sd(second_vaddr, second_store_value);
    TEST_ASSERT("after the repaired no-H same-page cross-16B load on address A has already completed, a younger different-address store fault on address B must refresh both cause and tval to the newer store-side fault instead of reusing stale load-side exception metadata",
        excpt.triggered == true &&
        excpt.cause == CAUSE_SAF &&
        excpt.tval == ai_micro_m_memblock_cross_16b_second_half_vaddr(second_vaddr)
    );

    goto_priv(PRIV_M);
    TEST_ASSERT("the repair-clear then load-to-store new-fault no-H same-page cross-16B probe leaves both address windows unchanged because the repaired load is read-only and the younger store is denied on its second split",
        read64(first_low_phys) == first_low_pattern &&
        read64(first_high_phys) == first_high_pattern &&
        read64(second_low_phys) == second_low_pattern &&
        read64(second_high_phys) == second_high_pattern
    );

    ai_micro_m_memblock_reset_env();

    TEST_END();
}

bool ai_micro_m_store_cross_16b_second_split_access_fault_repeated_then_same_address_valid_store_then_different_address_load_fault_refreshes_new_cause_tval_corner()
{
    TEST_START();

    const enum test_page page = SWITCH1;
    const uintptr_t first_offset = AI_MICRO_MEMBLOCK_CROSS_16B_OFFSET;
    const uintptr_t second_offset =
        AI_MICRO_MEMBLOCK_CROSS_16B_BOUNDARY1_SD_4B4B_OFFSET;
    const uintptr_t first_vaddr =
        ai_micro_m_memblock_cross_16b_addr_with_offset(page, first_offset);
    const uintptr_t second_vaddr =
        ai_micro_m_memblock_cross_16b_addr_with_offset(page, second_offset);
    const uintptr_t first_low_phys =
        ai_micro_m_memblock_cross_16b_low_phys_with_offset(page, first_offset);
    const uintptr_t first_high_phys =
        ai_micro_m_memblock_cross_16b_high_phys_with_offset(page, first_offset);
    const uintptr_t second_low_phys =
        ai_micro_m_memblock_cross_16b_low_phys_with_offset(page, second_offset);
    const uintptr_t second_high_phys =
        ai_micro_m_memblock_cross_16b_high_phys_with_offset(page, second_offset);
    const uint64_t first_low_pattern = 0x1021324354657687ULL;
    const uint64_t first_high_pattern = 0x98badcfe10325476ULL;
    const uint64_t second_low_pattern = 0x0123456789abcdefULL;
    const uint64_t second_high_pattern = 0xfedcba9876543210ULL;
    const uint64_t first_store_value = 0xa1b2c3d4e5f60718ULL;
    const uint64_t expected_first_low =
        ai_micro_m_memblock_expected_low_after_4b_store(
            first_low_pattern, first_store_value);
    const uint64_t expected_first_high =
        ai_micro_m_memblock_expected_high_after_4b_store(
            first_high_pattern, first_store_value);
    uint64_t observed_value = 0ULL;

    goto_priv(PRIV_M);
    ai_micro_m_memblock_reset_env();
    write64(first_low_phys, first_low_pattern);
    write64(first_high_phys, first_high_pattern);
    write64(second_low_phys, second_low_pattern);
    write64(second_high_phys, second_high_pattern);
    ai_micro_m_memblock_configure_cross_16b_pmp_window(
        first_high_phys, first_high_phys + PAGE_SIZE,
        AI_MICRO_MEMBLOCK_PMP_TOR_NO_W);

    ai_micro_m_memblock_prepare_supervisor_translation();
    ai_micro_m_memblock_set_cross_16b_leaves(PTE_V | PTE_AD | PTE_RWX);
    sfence_vma();

    TEST_SETUP_EXCEPT();
    ai_micro_m_memblock_translated_sd(first_vaddr, first_store_value);
    TEST_ASSERT("the first no-H same-page cross-16B repair-clear then store-to-load new-fault probe reports store access fault with tval bound to the original address A",
        excpt.triggered == true &&
        excpt.cause == CAUSE_SAF &&
        excpt.tval == ai_micro_m_memblock_cross_16b_second_half_vaddr(first_vaddr)
    );

    TEST_SETUP_EXCEPT();
    ai_micro_m_memblock_translated_sd(first_vaddr, first_store_value);
    TEST_ASSERT("repeating the same no-H same-page cross-16B store fault on address A keeps the same precise metadata before the repair-clear store-to-load new-fault probe",
        excpt.triggered == true &&
        excpt.cause == CAUSE_SAF &&
        excpt.tval == ai_micro_m_memblock_cross_16b_second_half_vaddr(first_vaddr)
    );

    goto_priv(PRIV_M);
    TEST_ASSERT("before repairing the no-H same-page cross-16B repeated store fault on address A for the store-to-load new-fault probe, both address windows remain unchanged",
        read64(first_low_phys) == first_low_pattern &&
        read64(first_high_phys) == first_high_pattern &&
        read64(second_low_phys) == second_low_pattern &&
        read64(second_high_phys) == second_high_pattern
    );

    ai_micro_m_memblock_restore_pmp_full_access();

    TEST_SETUP_EXCEPT();
    ai_micro_m_memblock_translated_sd(first_vaddr, first_store_value);
    TEST_ASSERT("after repairing the repeated no-H same-page cross-16B store fault on address A, the same-address valid store commits before the younger load fault on address B is injected",
        excpt.triggered == false
    );

    goto_priv(PRIV_M);
    TEST_ASSERT("the repaired same-address no-H same-page cross-16B store in the store-to-load new-fault probe updates only address A with the expected 4B+4B merged bytes",
        read64(first_low_phys) == expected_first_low &&
        read64(first_high_phys) == expected_first_high &&
        read64(second_low_phys) == second_low_pattern &&
        read64(second_high_phys) == second_high_pattern
    );

    ai_micro_m_memblock_configure_cross_16b_pmp_window(
        second_high_phys, second_high_phys + PAGE_SIZE,
        AI_MICRO_MEMBLOCK_PMP_TOR_NO_R);

    TEST_SETUP_EXCEPT();
    observed_value = ai_micro_m_memblock_translated_scalar_load(
        false, sizeof(uint64_t), second_vaddr);
    TEST_ASSERT("after the repaired no-H same-page cross-16B store on address A has already completed, a younger different-address load fault on address B must refresh both cause and tval to the newer load-side fault instead of reusing stale store-side exception metadata",
        excpt.triggered == true &&
        excpt.cause == CAUSE_LAF &&
        excpt.tval == ai_micro_m_memblock_cross_16b_second_half_vaddr(second_vaddr)
    );

    goto_priv(PRIV_M);
    TEST_ASSERT("the repair-clear then store-to-load new-fault no-H same-page cross-16B probe preserves the already-updated address A while keeping address B unchanged under the younger denied load",
        read64(first_low_phys) == expected_first_low &&
        read64(first_high_phys) == expected_first_high &&
        read64(second_low_phys) == second_low_pattern &&
        read64(second_high_phys) == second_high_pattern
    );

    ai_micro_m_memblock_reset_env();

    TEST_END();
}

bool ai_micro_m_samepage_cross_16b_fault_switch_load_then_store_keeps_latest_cause_and_tval_corner()
{
    TEST_START();

    const enum test_page page = SWITCH1;
    const uintptr_t load_offset = AI_MICRO_MEMBLOCK_CROSS_16B_OFFSET;
    const uintptr_t store_offset =
        AI_MICRO_MEMBLOCK_CROSS_16B_BOUNDARY1_SD_4B4B_OFFSET;
    const uintptr_t load_vaddr =
        ai_micro_m_memblock_cross_16b_addr_with_offset(page, load_offset);
    const uintptr_t store_vaddr =
        ai_micro_m_memblock_cross_16b_addr_with_offset(page, store_offset);
    const uintptr_t load_low_phys =
        ai_micro_m_memblock_cross_16b_low_phys_with_offset(page, load_offset);
    const uintptr_t load_high_phys =
        ai_micro_m_memblock_cross_16b_high_phys_with_offset(page, load_offset);
    const uintptr_t store_low_phys =
        ai_micro_m_memblock_cross_16b_low_phys_with_offset(page, store_offset);
    const uintptr_t store_high_phys =
        ai_micro_m_memblock_cross_16b_high_phys_with_offset(page, store_offset);
    const uint64_t load_low_pattern = 0x0123456789abcdefULL;
    const uint64_t load_high_pattern = 0xfedcba9876543210ULL;
    const uint64_t store_low_pattern = 0x1021324354657687ULL;
    const uint64_t store_high_pattern = 0x98badcfe10325476ULL;
    const uint64_t store_value = 0x89abcdef01234567ULL;
    uint64_t observed_value = 0ULL;

    goto_priv(PRIV_M);
    ai_micro_m_memblock_reset_env();
    write64(load_low_phys, load_low_pattern);
    write64(load_high_phys, load_high_pattern);
    write64(store_low_phys, store_low_pattern);
    write64(store_high_phys, store_high_pattern);
    ai_micro_m_memblock_configure_cross_16b_pmp_window(
        load_high_phys, load_high_phys + PAGE_SIZE,
        AI_MICRO_MEMBLOCK_PMP_TOR_NO_R);

    ai_micro_m_memblock_prepare_supervisor_translation();
    ai_micro_m_memblock_set_cross_16b_leaves(PTE_V | PTE_AD | PTE_RWX);
    sfence_vma();

    ai_micro_m_memblock_enable_mprv_supervisor_translation();
    TEST_SETUP_EXCEPT();
    observed_value = ld(load_vaddr);
    TEST_ASSERT("the first no-H same-page cross-16B load->store producer-switch probe reports a load access fault with tval bound to the original misaligned load address",
        excpt.triggered == true &&
        excpt.cause == CAUSE_LAF &&
        excpt.tval == ai_micro_m_memblock_cross_16b_second_half_vaddr(load_vaddr)
    );

    goto_priv(PRIV_M);
    TEST_ASSERT("after the first no-H same-page cross-16B load fault in the producer-switch probe, both fault windows remain unchanged before the follow-up store fault",
        read64(load_low_phys) == load_low_pattern &&
        read64(load_high_phys) == load_high_pattern &&
        read64(store_low_phys) == store_low_pattern &&
        read64(store_high_phys) == store_high_pattern
    );

    ai_micro_m_memblock_configure_cross_16b_pmp_window(
        store_high_phys, store_high_phys + PAGE_SIZE,
        AI_MICRO_MEMBLOCK_PMP_TOR_NO_W);

    ai_micro_m_memblock_enable_mprv_supervisor_translation();
    TEST_SETUP_EXCEPT();
    sd(store_vaddr, store_value);
    TEST_ASSERT("after the earlier same-page cross-16B load fault has already occupied the shared exception path, the follow-up store fault must refresh cause and tval to the newer store-side faulting address",
        excpt.triggered == true &&
        excpt.cause == CAUSE_SAF &&
        excpt.tval == ai_micro_m_memblock_cross_16b_second_half_vaddr(store_vaddr)
    );

    goto_priv(PRIV_M);
    TEST_ASSERT("the same-page cross-16B load->store producer-switch fault sequence leaves both windows unchanged because neither the load fault nor the second-split-only store fault should create visible memory side effects",
        read64(load_low_phys) == load_low_pattern &&
        read64(load_high_phys) == load_high_pattern &&
        read64(store_low_phys) == store_low_pattern &&
        read64(store_high_phys) == store_high_pattern
    );

    ai_micro_m_memblock_reset_env();

    TEST_END();
}

bool ai_micro_m_samepage_cross_16b_same_address_fault_switch_load_then_store_keeps_latest_cause_and_tval_corner()
{
    TEST_START();

    const enum test_page page = SWITCH1;
    const uintptr_t vaddr = ai_micro_m_memblock_cross_16b_addr(page);
    const uintptr_t low_phys = ai_micro_m_memblock_cross_16b_low_phys(page);
    const uintptr_t high_phys = ai_micro_m_memblock_cross_16b_high_phys(page);
    const uint64_t low_pattern = 0x0123456789abcdefULL;
    const uint64_t high_pattern = 0xfedcba9876543210ULL;
    const uint64_t store_value = 0x89abcdef01234567ULL;
    uint64_t observed_value = 0ULL;

    goto_priv(PRIV_M);
    ai_micro_m_memblock_reset_env();
    write64(low_phys, low_pattern);
    write64(high_phys, high_pattern);
    ai_micro_m_memblock_configure_cross_16b_pmp_window(
        high_phys, high_phys + PAGE_SIZE,
        AI_MICRO_MEMBLOCK_PMP_TOR_NO_R);

    ai_micro_m_memblock_prepare_supervisor_translation();
    ai_micro_m_memblock_set_cross_16b_leaves(PTE_V | PTE_AD | PTE_RWX);
    sfence_vma();

    ai_micro_m_memblock_enable_mprv_supervisor_translation();
    TEST_SETUP_EXCEPT();
    observed_value = ld(vaddr);
    TEST_ASSERT("the first no-H same-page cross-16B same-address load->store producer-switch probe reports a load access fault with tval bound to the original misaligned address",
        excpt.triggered == true &&
        excpt.cause == CAUSE_LAF &&
        excpt.tval == ai_micro_m_memblock_cross_16b_second_half_vaddr(vaddr)
    );

    goto_priv(PRIV_M);
    TEST_ASSERT("after the first no-H same-page cross-16B same-address load fault in the producer-switch probe, both aligned doublewords remain unchanged before the follow-up store fault",
        read64(low_phys) == low_pattern &&
        read64(high_phys) == high_pattern
    );

    ai_micro_m_memblock_configure_cross_16b_pmp_window(
        high_phys, high_phys + PAGE_SIZE,
        AI_MICRO_MEMBLOCK_PMP_TOR_NO_W);

    ai_micro_m_memblock_enable_mprv_supervisor_translation();
    TEST_SETUP_EXCEPT();
    sd(vaddr, store_value);
    TEST_ASSERT("after the earlier same-page cross-16B load fault has already occupied the shared exception path, the follow-up same-address store fault must refresh cause and tval to the newer store-side faulting access",
        excpt.triggered == true &&
        excpt.cause == CAUSE_SAF &&
        excpt.tval == ai_micro_m_memblock_cross_16b_second_half_vaddr(vaddr)
    );

    goto_priv(PRIV_M);
    TEST_ASSERT("the same-page cross-16B same-address load->store producer-switch fault sequence leaves both aligned doublewords unchanged because neither fault should create visible memory side effects",
        read64(low_phys) == low_pattern &&
        read64(high_phys) == high_pattern
    );

    ai_micro_m_memblock_reset_env();

    TEST_END();
}

bool ai_micro_m_load_cross_16b_second_split_access_fault_repeated_then_same_address_valid_store_recovers_corner()
{
    TEST_START();

    const enum test_page page = SWITCH1;
    const uintptr_t vaddr = ai_micro_m_memblock_cross_16b_addr(page);
    const uintptr_t low_phys = ai_micro_m_memblock_cross_16b_low_phys(page);
    const uintptr_t high_phys = ai_micro_m_memblock_cross_16b_high_phys(page);
    const uintptr_t deny_start = high_phys;
    const uintptr_t deny_end = high_phys + PAGE_SIZE;
    const uint64_t low_pattern = 0x21436587a9cbed0fULL;
    const uint64_t high_pattern = 0xf0debc9a78563412ULL;
    const uint64_t store_value = 0x89abcdef01234567ULL;
    const uint64_t expected_low =
        ai_micro_m_memblock_expected_low_after_4b_store(
            low_pattern, store_value);
    const uint64_t expected_high =
        ai_micro_m_memblock_expected_high_after_4b_store(
            high_pattern, store_value);
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
    TEST_ASSERT("the first no-H same-page cross-16B split-load probe on address A reports load access fault with the original misaligned VA before the same-address store producer-switch recovery probe",
        excpt.triggered == true &&
        excpt.cause == CAUSE_LAF &&
        excpt.tval == ai_micro_m_memblock_cross_16b_second_half_vaddr(vaddr)
    );

    ai_micro_m_memblock_enable_mprv_supervisor_translation();
    TEST_SETUP_EXCEPT();
    observed_value = ld(vaddr);
    TEST_ASSERT("repeating the no-H same-page cross-16B split-load fault on address A keeps the same precise metadata before the same-address store producer-switch recovery probe",
        excpt.triggered == true &&
        excpt.cause == CAUSE_LAF &&
        excpt.tval == ai_micro_m_memblock_cross_16b_second_half_vaddr(vaddr)
    );

    goto_priv(PRIV_M);
    TEST_ASSERT("before the same-address store producer-switch recovery probe, the repeated second-split-only same-page cross-16B load faults leave both aligned doublewords untouched",
        read64(low_phys) == low_pattern &&
        read64(high_phys) == high_pattern
    );

    ai_micro_m_memblock_restore_pmp_full_access();

    ai_micro_m_memblock_enable_mprv_supervisor_translation();
    TEST_SETUP_EXCEPT();
    sd(vaddr, store_value);
    TEST_ASSERT("after the repeated no-H same-page cross-16B load fault episode on address A is repaired, a same-address cross-16B store on address A must complete without inheriting stale load-side exception metadata",
        excpt.triggered == false
    );

    goto_priv(PRIV_M);
    TEST_ASSERT("the no-H same-address cross-16B store producer-switch recovery probe updates the two aligned doublewords with the expected 4B+4B merged bytes",
        read64(low_phys) == expected_low &&
        read64(high_phys) == expected_high
    );

    ai_micro_m_memblock_reset_env();

    TEST_END();
}

bool ai_micro_m_load_cross_16b_second_split_access_fault_repeated_then_same_address_different_width_store_recovers_corner()
{
    TEST_START();

    static const bool fault_signed[2] = {
        true,
        false,
    };
    static const uint64_t recovery_store_values[2] = {
        0x13579bdf2468ace0ULL,
        0xf0e1d2c3b4a59687ULL,
    };
    static const uint8_t init_patterns[2][2][16] = {
        {
            { 0x31, 0x42, 0x53, 0x64, 0x75, 0x86, 0x97, 0x80,
              0x81, 0xaa, 0xbb, 0xcc, 0xdd, 0xee, 0xff, 0x10 },
            { 0xd1, 0xc2, 0xb3, 0xa4, 0x95, 0x86, 0x77, 0x68,
              0x59, 0x4a, 0x3b, 0x2c, 0x1d, 0x0e, 0xff, 0xe0 },
        },
        {
            { 0x14, 0x26, 0x38, 0x4a, 0x5c, 0x6e, 0x70, 0x82,
              0x94, 0xa6, 0xb8, 0xca, 0xdc, 0xee, 0xf0, 0x12 },
            { 0x7e, 0x6d, 0x5c, 0x4b, 0x3a, 0x29, 0x18, 0x80,
              0x81, 0x93, 0xa5, 0xb7, 0xc9, 0xdb, 0xed, 0xff },
        },
    };
    static const uint8_t guard_before_patterns[2][8] = {
        { 0x12, 0x24, 0x36, 0x48, 0x5a, 0x6c, 0x7e, 0x80 },
        { 0x81, 0x6f, 0x5d, 0x4b, 0x39, 0x27, 0x15, 0x03 },
    };
    static const uint8_t guard_middle_patterns[2][16] = {
        { 0x91, 0xa3, 0xb5, 0xc7, 0xd9, 0xeb, 0xfd, 0x0f,
          0x10, 0x21, 0x32, 0x43, 0x54, 0x65, 0x76, 0x87 },
        { 0x18, 0x2a, 0x3c, 0x4e, 0x50, 0x62, 0x74, 0x86,
          0x98, 0xaa, 0xbc, 0xce, 0xd0, 0xe2, 0xf4, 0x06 },
    };
    static const uint8_t guard_after_patterns[2][8] = {
        { 0x25, 0x47, 0x69, 0x8b, 0xad, 0xcf, 0xe1, 0xf3 },
        { 0x4b, 0x5d, 0x6f, 0x71, 0x83, 0x95, 0xa7, 0xb9 },
    };
    const enum test_page page = SWITCH1;
    const uintptr_t target_vaddr[2] = {
        ai_micro_m_memblock_cross_16b_addr_with_offset(
            page, AI_MICRO_MEMBLOCK_CROSS_16B_3B1B_OFFSET),
        ai_micro_m_memblock_cross_16b_addr_with_offset(
            page, AI_MICRO_MEMBLOCK_CROSS_16B_BOUNDARY1_LW_OFFSET),
    };
    const uintptr_t boundary_phys[2] = {
        ai_micro_m_memblock_cross_16b_boundary_phys(0U),
        ai_micro_m_memblock_cross_16b_boundary_phys(1U),
    };
    const uintptr_t guard_before_phys =
        ai_micro_m_memblock_cross_16b_guard_before_phys();
    const uintptr_t guard_middle_phys =
        ai_micro_m_memblock_cross_16b_guard_middle_phys();
    const uintptr_t guard_after_phys =
        ai_micro_m_memblock_cross_16b_guard_after_phys();
    uint8_t expected_boundary[16];
    uint64_t observed_value = 0ULL;

    for (unsigned round = 0; round < 2U; ++round) {
        const unsigned target = round;
        const unsigned other = target ^ 1U;
        const uintptr_t vaddr = target_vaddr[target];
        const uintptr_t deny_start = boundary_phys[target] + 8U;
        const uintptr_t deny_end = deny_start + PAGE_SIZE;

        goto_priv(PRIV_M);
        ai_micro_m_memblock_reset_env();
        ai_micro_m_memblock_write_bytes(boundary_phys[0],
            init_patterns[0][round], 16U);
        ai_micro_m_memblock_write_bytes(boundary_phys[1],
            init_patterns[1][round], 16U);
        ai_micro_m_memblock_write_bytes(guard_before_phys,
            guard_before_patterns[round], 8U);
        ai_micro_m_memblock_write_bytes(guard_middle_phys,
            guard_middle_patterns[round], 16U);
        ai_micro_m_memblock_write_bytes(guard_after_phys,
            guard_after_patterns[round], 8U);
        ai_micro_m_memblock_overlay_little_endian_bytes(expected_boundary,
            init_patterns[target][round], 16U, 5U,
            recovery_store_values[round], 8U);
        ai_micro_m_memblock_configure_cross_16b_pmp_window(
            deny_start, deny_end, AI_MICRO_MEMBLOCK_PMP_TOR_NO_R);

        ai_micro_m_memblock_prepare_supervisor_translation();
        ai_micro_m_memblock_set_cross_16b_leaves(PTE_V | PTE_AD | PTE_RWX);
        sfence_vma();

        TEST_SETUP_EXCEPT();
        observed_value = ai_micro_m_memblock_translated_scalar_load(
            fault_signed[round], sizeof(uint32_t), vaddr);
        TEST_ASSERT("the first no-H same-page cross-16B same-address load fault in the cross-producer width-switch recovery probe reports load access fault with the original misaligned VA",
            excpt.triggered == true &&
            excpt.cause == CAUSE_LAF &&
            excpt.tval == ai_micro_m_memblock_cross_16b_second_half_vaddr(vaddr)
        );

        TEST_SETUP_EXCEPT();
        observed_value = ai_micro_m_memblock_translated_scalar_load(
            fault_signed[round], sizeof(uint32_t), vaddr);
        TEST_ASSERT("repeating the no-H same-page cross-16B same-address load fault keeps the same precise metadata before the different-width store recovery probe",
            excpt.triggered == true &&
            excpt.cause == CAUSE_LAF &&
            excpt.tval == ai_micro_m_memblock_cross_16b_second_half_vaddr(vaddr)
        );

        goto_priv(PRIV_M);
        TEST_ASSERT("before the same-address cross-producer different-width store recovery probe, both boundary windows and all guard regions remain untouched",
            ai_micro_m_memblock_bytes_match(boundary_phys[0],
                init_patterns[0][round], 16U) &&
            ai_micro_m_memblock_bytes_match(boundary_phys[1],
                init_patterns[1][round], 16U) &&
            ai_micro_m_memblock_bytes_match(guard_before_phys,
                guard_before_patterns[round], 8U) &&
            ai_micro_m_memblock_bytes_match(guard_middle_phys,
                guard_middle_patterns[round], 16U) &&
            ai_micro_m_memblock_bytes_match(guard_after_phys,
                guard_after_patterns[round], 8U)
        );

        ai_micro_m_memblock_restore_pmp_full_access();

        TEST_SETUP_EXCEPT();
        ai_micro_m_memblock_translated_store_by_size(
            vaddr, 8U, recovery_store_values[round]);
        TEST_ASSERT("after the repeated no-H same-page cross-16B load fault episode, the same-address different-width store recovery probe must commit without inheriting stale load-side exception metadata",
            excpt.triggered == false
        );

        goto_priv(PRIV_M);
        TEST_ASSERT("the no-H same-address cross-producer different-width store recovery probe leaves only the target boundary updated with the current 8-byte overlay while the other boundary and guard regions remain unchanged",
            ai_micro_m_memblock_bytes_match(boundary_phys[target],
                expected_boundary, 16U) &&
            ai_micro_m_memblock_bytes_match(boundary_phys[other],
                init_patterns[other][round], 16U) &&
            ai_micro_m_memblock_bytes_match(guard_before_phys,
                guard_before_patterns[round], 8U) &&
            ai_micro_m_memblock_bytes_match(guard_middle_phys,
                guard_middle_patterns[round], 16U) &&
            ai_micro_m_memblock_bytes_match(guard_after_phys,
                guard_after_patterns[round], 8U)
        );
    }

    ai_micro_m_memblock_reset_env();

    TEST_END();
}

bool ai_micro_m_load_cross_16b_second_split_access_fault_repeated_then_different_address_same_width_different_template_store_recovers_corner()
{
    TEST_START();

    static const uintptr_t load_fault_offsets[2] = {
        AI_MICRO_MEMBLOCK_CROSS_16B_BOUNDARY0_SD_4B4B_OFFSET,
        AI_MICRO_MEMBLOCK_CROSS_16B_BOUNDARY1_SD_OFFSET,
    };
    static const uintptr_t store_recovery_offsets[2] = {
        AI_MICRO_MEMBLOCK_CROSS_16B_BOUNDARY1_SD_OFFSET,
        AI_MICRO_MEMBLOCK_CROSS_16B_BOUNDARY0_SD_4B4B_OFFSET,
    };
    static const unsigned store_start_bytes[2] = {
        7U,
        4U,
    };
    static const uint64_t store_values[2] = {
        0x13579bdf2468ace0ULL,
        0x89abcdef01234567ULL,
    };
    static const uint8_t init_patterns[2][2][16] = {
        {
            { 0x41, 0x52, 0x63, 0x74, 0x85, 0x96, 0xa7, 0xb8,
              0xc9, 0xda, 0xeb, 0xfc, 0x0d, 0x1e, 0x2f, 0x30 },
            { 0xd3, 0xc4, 0xb5, 0xa6, 0x97, 0x88, 0x79, 0x6a,
              0x5b, 0x4c, 0x3d, 0x2e, 0x1f, 0x10, 0x01, 0xf2 },
        },
        {
            { 0x14, 0x25, 0x36, 0x47, 0x58, 0x69, 0x7a, 0x8b,
              0x9c, 0xad, 0xbe, 0xcf, 0xd0, 0xe1, 0xf2, 0x03 },
            { 0xb2, 0xa3, 0x94, 0x85, 0x76, 0x67, 0x58, 0x49,
              0x3a, 0x2b, 0x1c, 0x0d, 0xfe, 0xed, 0xdc, 0xcb },
        },
    };
    static const uint8_t guard_before_patterns[2][8] = {
        { 0x12, 0x24, 0x36, 0x48, 0x5a, 0x6c, 0x7e, 0x80 },
        { 0x81, 0x6f, 0x5d, 0x4b, 0x39, 0x27, 0x15, 0x03 },
    };
    static const uint8_t guard_middle_patterns[2][16] = {
        { 0x91, 0xa3, 0xb5, 0xc7, 0xd9, 0xeb, 0xfd, 0x0f,
          0x10, 0x21, 0x32, 0x43, 0x54, 0x65, 0x76, 0x87 },
        { 0x18, 0x2a, 0x3c, 0x4e, 0x50, 0x62, 0x74, 0x86,
          0x98, 0xaa, 0xbc, 0xce, 0xd0, 0xe2, 0xf4, 0x06 },
    };
    static const uint8_t guard_after_patterns[2][8] = {
        { 0x25, 0x47, 0x69, 0x8b, 0xad, 0xcf, 0xe1, 0xf3 },
        { 0x4b, 0x5d, 0x6f, 0x71, 0x83, 0x95, 0xa7, 0xb9 },
    };
    const enum test_page page = SWITCH1;
    const uintptr_t boundary_phys[2] = {
        ai_micro_m_memblock_cross_16b_boundary_phys(0U),
        ai_micro_m_memblock_cross_16b_boundary_phys(1U),
    };
    const uintptr_t guard_before_phys =
        ai_micro_m_memblock_cross_16b_guard_before_phys();
    const uintptr_t guard_middle_phys =
        ai_micro_m_memblock_cross_16b_guard_middle_phys();
    const uintptr_t guard_after_phys =
        ai_micro_m_memblock_cross_16b_guard_after_phys();
    uint8_t expected_boundary[16];
    uint64_t observed_value = 0ULL;

    for (unsigned round = 0; round < 2U; ++round) {
        const unsigned fault_index = round;
        const unsigned recovery_index = fault_index ^ 1U;
        const uintptr_t fault_vaddr =
            ai_micro_m_memblock_cross_16b_addr_with_offset(page,
                load_fault_offsets[round]);
        const uintptr_t recovery_vaddr =
            ai_micro_m_memblock_cross_16b_addr_with_offset(page,
                store_recovery_offsets[round]);
        const uintptr_t deny_start = boundary_phys[fault_index] + 8U;
        const uintptr_t deny_end = deny_start + PAGE_SIZE;

        goto_priv(PRIV_M);
        ai_micro_m_memblock_reset_env();
        ai_micro_m_memblock_write_bytes(boundary_phys[0],
            init_patterns[0][round], 16U);
        ai_micro_m_memblock_write_bytes(boundary_phys[1],
            init_patterns[1][round], 16U);
        ai_micro_m_memblock_write_bytes(guard_before_phys,
            guard_before_patterns[round], 8U);
        ai_micro_m_memblock_write_bytes(guard_middle_phys,
            guard_middle_patterns[round], 16U);
        ai_micro_m_memblock_write_bytes(guard_after_phys,
            guard_after_patterns[round], 8U);
        ai_micro_m_memblock_overlay_little_endian_bytes(expected_boundary,
            init_patterns[recovery_index][round], 16U,
            store_start_bytes[round], store_values[round], 8U);
        ai_micro_m_memblock_configure_cross_16b_pmp_window(
            deny_start, deny_end, AI_MICRO_MEMBLOCK_PMP_TOR_NO_R);

        ai_micro_m_memblock_prepare_supervisor_translation();
        ai_micro_m_memblock_set_cross_16b_leaves(PTE_V | PTE_AD | PTE_RWX);
        sfence_vma();

        TEST_SETUP_EXCEPT();
        observed_value = ai_micro_m_memblock_translated_scalar_load(
            false, sizeof(uint64_t), fault_vaddr);
        TEST_ASSERT("the first no-H same-page same-width different-template load fault reports load access fault with the original misaligned VA before the different-address store recovery probe",
            excpt.triggered == true &&
            excpt.cause == CAUSE_LAF &&
            excpt.tval == ai_micro_m_memblock_cross_16b_second_half_vaddr(fault_vaddr)
        );

        TEST_SETUP_EXCEPT();
        observed_value = ai_micro_m_memblock_translated_scalar_load(
            false, sizeof(uint64_t), fault_vaddr);
        TEST_ASSERT("repeating the no-H same-page same-width different-template load fault keeps the same precise metadata before the different-address store recovery probe",
            excpt.triggered == true &&
            excpt.cause == CAUSE_LAF &&
            excpt.tval == ai_micro_m_memblock_cross_16b_second_half_vaddr(fault_vaddr)
        );

        goto_priv(PRIV_M);
        TEST_ASSERT("before the different-address same-width different-template store recovery probe, both boundary windows and all guard regions remain untouched",
            ai_micro_m_memblock_bytes_match(boundary_phys[0],
                init_patterns[0][round], 16U) &&
            ai_micro_m_memblock_bytes_match(boundary_phys[1],
                init_patterns[1][round], 16U) &&
            ai_micro_m_memblock_bytes_match(guard_before_phys,
                guard_before_patterns[round], 8U) &&
            ai_micro_m_memblock_bytes_match(guard_middle_phys,
                guard_middle_patterns[round], 16U) &&
            ai_micro_m_memblock_bytes_match(guard_after_phys,
                guard_after_patterns[round], 8U)
        );

        ai_micro_m_memblock_restore_pmp_full_access();

        TEST_SETUP_EXCEPT();
        ai_micro_m_memblock_translated_sd(recovery_vaddr, store_values[round]);
        TEST_ASSERT("after the repeated no-H same-page same-width different-template load fault episode, the different-address store recovery probe must commit without inheriting stale load-side metadata or stale store template",
            excpt.triggered == false
        );

        goto_priv(PRIV_M);
        TEST_ASSERT("the different-address same-width different-template store recovery probe updates only its own boundary with the expected template-specific bytes and leaves the faulting boundary plus all guards unchanged",
            ai_micro_m_memblock_bytes_match(boundary_phys[recovery_index],
                expected_boundary, 16U) &&
            ai_micro_m_memblock_bytes_match(boundary_phys[fault_index],
                init_patterns[fault_index][round], 16U) &&
            ai_micro_m_memblock_bytes_match(guard_before_phys,
                guard_before_patterns[round], 8U) &&
            ai_micro_m_memblock_bytes_match(guard_middle_phys,
                guard_middle_patterns[round], 16U) &&
            ai_micro_m_memblock_bytes_match(guard_after_phys,
                guard_after_patterns[round], 8U)
        );
    }

    ai_micro_m_memblock_reset_env();

    TEST_END();
}

bool ai_micro_m_load_cross_16b_second_split_access_fault_repeated_then_different_address_width_signedness_switch_recovers_corner()
{
    TEST_START();

    static const unsigned load_sizes[2] = {
        sizeof(uint16_t),
        sizeof(uint32_t),
    };
    static const bool load_signed[2] = {
        true,
        false,
    };
    static const unsigned start_bytes[2] = {
        7U,
        5U,
    };
    static const uintptr_t load_offsets[2] = {
        AI_MICRO_MEMBLOCK_CROSS_16B_BOUNDARY0_LH_OFFSET,
        AI_MICRO_MEMBLOCK_CROSS_16B_BOUNDARY1_LW_OFFSET,
    };
    static const uint8_t init_patterns[2][2][16] = {
        {
            { 0x11, 0x24, 0x37, 0x4a, 0x5d, 0x70, 0x83, 0xf1,
              0x80, 0x9c, 0xaf, 0xc2, 0xd5, 0xe8, 0xfb, 0x0e },
            { 0xd2, 0xc1, 0xb0, 0xaf, 0x9e, 0x8d, 0x7c, 0x80,
              0x81, 0x62, 0x53, 0x44, 0x35, 0x26, 0x17, 0x08 },
        },
        {
            { 0x90, 0xa1, 0xb2, 0xc3, 0xd4, 0xe5, 0x7f, 0x80,
              0x81, 0x2a, 0x3b, 0x4c, 0x5d, 0x6e, 0x7f, 0x80 },
            { 0x3e, 0x4f, 0x60, 0x71, 0x82, 0x7f, 0x80, 0x81,
              0x92, 0xa3, 0xb4, 0xc5, 0xd6, 0xe7, 0xf8, 0x09 },
        },
    };
    static const uint8_t guard_before_patterns[2][8] = {
        { 0x15, 0x27, 0x39, 0x4b, 0x5d, 0x6f, 0x71, 0x83 },
        { 0x8d, 0x7b, 0x69, 0x57, 0x45, 0x33, 0x21, 0x1f },
    };
    static const uint8_t guard_middle_patterns[2][16] = {
        { 0xa0, 0xb2, 0xc4, 0xd6, 0xe8, 0xfa, 0x0c, 0x1e,
          0x2f, 0x40, 0x51, 0x62, 0x73, 0x84, 0x95, 0xa6 },
        { 0x1a, 0x2c, 0x3e, 0x40, 0x52, 0x64, 0x76, 0x88,
          0x9a, 0xac, 0xbe, 0xc0, 0xd2, 0xe4, 0xf6, 0x08 },
    };
    static const uint8_t guard_after_patterns[2][8] = {
        { 0x26, 0x48, 0x6a, 0x8c, 0xae, 0xc0, 0xd2, 0xe4 },
        { 0x4c, 0x5e, 0x70, 0x82, 0x94, 0xa6, 0xb8, 0xca },
    };
    const enum test_page page = SWITCH1;
    const uintptr_t load_vaddr[2] = {
        ai_micro_m_memblock_cross_16b_addr_with_offset(page,
            load_offsets[0]),
        ai_micro_m_memblock_cross_16b_addr_with_offset(page,
            load_offsets[1]),
    };
    const uintptr_t boundary_phys[2] = {
        ai_micro_m_memblock_cross_16b_boundary_phys(0U),
        ai_micro_m_memblock_cross_16b_boundary_phys(1U),
    };
    const uintptr_t second_split_phys[2] = {
        ai_micro_m_memblock_cross_16b_boundary_high_phys_for_index(0U),
        ai_micro_m_memblock_cross_16b_boundary_high_phys_for_index(1U),
    };
    const uintptr_t guard_before_phys =
        ai_micro_m_memblock_cross_16b_guard_before_phys();
    const uintptr_t guard_middle_phys =
        ai_micro_m_memblock_cross_16b_guard_middle_phys();
    const uintptr_t guard_after_phys =
        ai_micro_m_memblock_cross_16b_guard_after_phys();
    uint64_t observed_value = 0ULL;

    for (unsigned round = 0; round < 2U; ++round) {
        const bool forward = (round == 0U);
        const unsigned first = forward ? 0U : 1U;
        const unsigned second = forward ? 1U : 0U;
        const uint64_t expected_second_value =
            ai_micro_m_memblock_extract_scalar_from_bytes(
                init_patterns[second][round], start_bytes[second],
                load_sizes[second], load_signed[second]);

        goto_priv(PRIV_M);
        ai_micro_m_memblock_reset_env();
        ai_micro_m_memblock_write_bytes(boundary_phys[0],
            init_patterns[0][round], 16U);
        ai_micro_m_memblock_write_bytes(boundary_phys[1],
            init_patterns[1][round], 16U);
        ai_micro_m_memblock_write_bytes(guard_before_phys,
            guard_before_patterns[round], 8U);
        ai_micro_m_memblock_write_bytes(guard_middle_phys,
            guard_middle_patterns[round], 16U);
        ai_micro_m_memblock_write_bytes(guard_after_phys,
            guard_after_patterns[round], 8U);
        ai_micro_m_memblock_configure_cross_16b_pmp_window(second_split_phys[first],
            second_split_phys[first] + PAGE_SIZE,
            AI_MICRO_MEMBLOCK_PMP_TOR_NO_R);

        ai_micro_m_memblock_prepare_supervisor_translation();
        ai_micro_m_memblock_set_cross_16b_leaves(PTE_V | PTE_AD | PTE_RWX);
        sfence_vma();

        TEST_SETUP_EXCEPT();
        (void)ai_micro_m_memblock_translated_scalar_load(load_signed[first],
            load_sizes[first], load_vaddr[first]);
        TEST_ASSERT("the first no-H same-page cross-16B width/signedness-sensitive split-load fault on address A reports load access fault with the original misaligned VA when the second split is PMP-denied",
            excpt.triggered == true &&
            excpt.cause == CAUSE_LAF &&
            excpt.tval == ai_micro_m_memblock_cross_16b_second_half_vaddr(load_vaddr[first])
        );

        TEST_SETUP_EXCEPT();
        (void)ai_micro_m_memblock_translated_scalar_load(load_signed[first],
            load_sizes[first], load_vaddr[first]);
        TEST_ASSERT("repeating the no-H same-page cross-16B width/signedness-sensitive split-load fault on address A keeps the same precise metadata before the different-address width-switch recovery probe",
            excpt.triggered == true &&
            excpt.cause == CAUSE_LAF &&
            excpt.tval == ai_micro_m_memblock_cross_16b_second_half_vaddr(load_vaddr[first])
        );

        goto_priv(PRIV_M);
        TEST_ASSERT("before the different-address width/signedness-switch recovery probe, the repeated same-page cross-16B split-load faults on address A leave both boundary windows and all guard regions untouched",
            ai_micro_m_memblock_bytes_match(boundary_phys[0],
                init_patterns[0][round], 16U) &&
            ai_micro_m_memblock_bytes_match(boundary_phys[1],
                init_patterns[1][round], 16U) &&
            ai_micro_m_memblock_bytes_match(guard_before_phys,
                guard_before_patterns[round], 8U) &&
            ai_micro_m_memblock_bytes_match(guard_middle_phys,
                guard_middle_patterns[round], 16U) &&
            ai_micro_m_memblock_bytes_match(guard_after_phys,
                guard_after_patterns[round], 8U)
        );

        ai_micro_m_memblock_restore_pmp_full_access();

        ai_micro_m_memblock_enable_mprv_supervisor_translation();
        TEST_SETUP_EXCEPT();
        observed_value = ai_micro_m_memblock_scalar_load_current_decode(
            load_signed[second], load_sizes[second], load_vaddr[second]);
        ai_micro_m_memblock_disable_mprv_supervisor_translation();
        TEST_ASSERT("after repairing the repeated no-H same-page cross-16B load fault on address A, the first different-address width/signedness-switched split load on address B must return the value selected by its current merge template and sign/zero-extension decode",
            excpt.triggered == false &&
            observed_value == expected_second_value
        );

        goto_priv(PRIV_M);
        TEST_ASSERT("the no-H different-address width/signedness-switch split-load recovery probe remains read-only and leaves both boundary windows plus all guard regions unchanged",
            ai_micro_m_memblock_bytes_match(boundary_phys[0],
                init_patterns[0][round], 16U) &&
            ai_micro_m_memblock_bytes_match(boundary_phys[1],
                init_patterns[1][round], 16U) &&
            ai_micro_m_memblock_bytes_match(guard_before_phys,
                guard_before_patterns[round], 8U) &&
            ai_micro_m_memblock_bytes_match(guard_middle_phys,
                guard_middle_patterns[round], 16U) &&
            ai_micro_m_memblock_bytes_match(guard_after_phys,
                guard_after_patterns[round], 8U)
        );
    }

    ai_micro_m_memblock_reset_env();

    TEST_END();
}

bool ai_micro_m_load_cross_16b_second_split_access_fault_repeated_then_same_address_width_signedness_switch_recovers_corner()
{
    TEST_START();

    static const unsigned fault_sizes[2] = {
        sizeof(uint16_t),
        sizeof(uint32_t),
    };
    static const bool fault_signed[2] = {
        true,
        false,
    };
    static const unsigned recovery_sizes[2] = {
        sizeof(uint32_t),
        sizeof(uint16_t),
    };
    static const bool recovery_signed[2] = {
        false,
        true,
    };
    static const uintptr_t load_offsets[2] = {
        AI_MICRO_MEMBLOCK_CROSS_16B_BOUNDARY0_LH_OFFSET,
        AI_MICRO_MEMBLOCK_CROSS_16B_BOUNDARY1_SD_OFFSET,
    };
    static const uint8_t init_patterns[2][2][16] = {
        {
            { 0x31, 0x42, 0x53, 0x64, 0x75, 0x86, 0x97, 0x80,
              0x81, 0xa8, 0xb9, 0xca, 0xdb, 0xec, 0xfd, 0x0e },
            { 0xe1, 0xd2, 0xc3, 0xb4, 0xa5, 0x96, 0x87, 0xf0,
              0x80, 0x69, 0x5a, 0x4b, 0x3c, 0x2d, 0x1e, 0x0f },
        },
        {
            { 0x14, 0x26, 0x38, 0x4a, 0x5c, 0x6e, 0x70, 0x82,
              0x94, 0xa6, 0xb8, 0xca, 0xdc, 0xee, 0xf0, 0x12 },
            { 0x7e, 0x6d, 0x5c, 0x4b, 0x3a, 0x29, 0x18, 0x80,
              0x81, 0x93, 0xa5, 0xb7, 0xc9, 0xdb, 0xed, 0xff },
        },
    };
    static const uint8_t guard_before_patterns[2][8] = {
        { 0x16, 0x28, 0x3a, 0x4c, 0x5e, 0x60, 0x72, 0x84 },
        { 0x8e, 0x7c, 0x6a, 0x58, 0x46, 0x34, 0x22, 0x10 },
    };
    static const uint8_t guard_middle_patterns[2][16] = {
        { 0xb0, 0xc2, 0xd4, 0xe6, 0xf8, 0x0a, 0x1c, 0x2e,
          0x3f, 0x50, 0x61, 0x72, 0x83, 0x94, 0xa5, 0xb6 },
        { 0x19, 0x2b, 0x3d, 0x4f, 0x51, 0x63, 0x75, 0x87,
          0x99, 0xab, 0xbd, 0xcf, 0xd1, 0xe3, 0xf5, 0x07 },
    };
    static const uint8_t guard_after_patterns[2][8] = {
        { 0x27, 0x49, 0x6b, 0x8d, 0xaf, 0xc1, 0xd3, 0xe5 },
        { 0x4d, 0x5f, 0x71, 0x83, 0x95, 0xa7, 0xb9, 0xcb },
    };
    const enum test_page page = SWITCH1;
    const uintptr_t boundary_phys[2] = {
        ai_micro_m_memblock_cross_16b_boundary_phys(0U),
        ai_micro_m_memblock_cross_16b_boundary_phys(1U),
    };
    const uintptr_t guard_before_phys =
        ai_micro_m_memblock_cross_16b_guard_before_phys();
    const uintptr_t guard_middle_phys =
        ai_micro_m_memblock_cross_16b_guard_middle_phys();
    const uintptr_t guard_after_phys =
        ai_micro_m_memblock_cross_16b_guard_after_phys();
    uint64_t observed_value = 0ULL;

    for (unsigned round = 0; round < 2U; ++round) {
        const unsigned target = round;
        const uintptr_t load_vaddr =
            ai_micro_m_memblock_cross_16b_addr_with_offset(page,
                load_offsets[round]);
        const uintptr_t deny_start = boundary_phys[target] + 8U;
        const uintptr_t deny_end = deny_start + PAGE_SIZE;
        const uint64_t expected_recovery_value =
            ai_micro_m_memblock_extract_scalar_from_bytes(
                init_patterns[target][round], 7U, recovery_sizes[round],
                recovery_signed[round]);

        goto_priv(PRIV_M);
        ai_micro_m_memblock_reset_env();
        ai_micro_m_memblock_write_bytes(boundary_phys[0],
            init_patterns[0][round], 16U);
        ai_micro_m_memblock_write_bytes(boundary_phys[1],
            init_patterns[1][round], 16U);
        ai_micro_m_memblock_write_bytes(guard_before_phys,
            guard_before_patterns[round], 8U);
        ai_micro_m_memblock_write_bytes(guard_middle_phys,
            guard_middle_patterns[round], 16U);
        ai_micro_m_memblock_write_bytes(guard_after_phys,
            guard_after_patterns[round], 8U);
        ai_micro_m_memblock_configure_cross_16b_pmp_window(
            deny_start, deny_end, AI_MICRO_MEMBLOCK_PMP_TOR_NO_R);

        ai_micro_m_memblock_prepare_supervisor_translation();
        ai_micro_m_memblock_set_cross_16b_leaves(PTE_V | PTE_AD | PTE_RWX);
        sfence_vma();

        TEST_SETUP_EXCEPT();
        (void)ai_micro_m_memblock_translated_scalar_load(fault_signed[round],
            fault_sizes[round], load_vaddr);
        TEST_ASSERT("the first no-H same-address cross-16B width/signedness-switch split-load fault reports load access fault with the original misaligned VA when the second split is PMP-denied",
            excpt.triggered == true &&
            excpt.cause == CAUSE_LAF &&
            excpt.tval == ai_micro_m_memblock_cross_16b_second_half_vaddr(load_vaddr)
        );

        TEST_SETUP_EXCEPT();
        (void)ai_micro_m_memblock_translated_scalar_load(fault_signed[round],
            fault_sizes[round], load_vaddr);
        TEST_ASSERT("repeating the no-H same-address cross-16B width/signedness-switch split-load fault keeps the same precise metadata before the same-address recovery-shape switch probe",
            excpt.triggered == true &&
            excpt.cause == CAUSE_LAF &&
            excpt.tval == ai_micro_m_memblock_cross_16b_second_half_vaddr(load_vaddr)
        );

        goto_priv(PRIV_M);
        TEST_ASSERT("before the same-address width/signedness-switch recovery probe, the repeated same-page cross-16B split-load faults leave both boundary windows and all guard regions untouched",
            ai_micro_m_memblock_bytes_match(boundary_phys[0],
                init_patterns[0][round], 16U) &&
            ai_micro_m_memblock_bytes_match(boundary_phys[1],
                init_patterns[1][round], 16U) &&
            ai_micro_m_memblock_bytes_match(guard_before_phys,
                guard_before_patterns[round], 8U) &&
            ai_micro_m_memblock_bytes_match(guard_middle_phys,
                guard_middle_patterns[round], 16U) &&
            ai_micro_m_memblock_bytes_match(guard_after_phys,
                guard_after_patterns[round], 8U)
        );

        ai_micro_m_memblock_restore_pmp_full_access();

        ai_micro_m_memblock_enable_mprv_supervisor_translation();
        TEST_SETUP_EXCEPT();
        observed_value = ai_micro_m_memblock_scalar_load_current_decode(
            recovery_signed[round], recovery_sizes[round], load_vaddr);
        ai_micro_m_memblock_disable_mprv_supervisor_translation();
        TEST_ASSERT("after repairing the repeated no-H same-address cross-16B load fault, the first same-address width/signedness-switched split load must return the value selected by its current merge template and sign/zero-extension decode",
            excpt.triggered == false &&
            observed_value == expected_recovery_value
        );

        goto_priv(PRIV_M);
        TEST_ASSERT("the no-H same-address width/signedness-switch split-load recovery probe remains read-only and leaves both boundary windows plus all guard regions unchanged",
            ai_micro_m_memblock_bytes_match(boundary_phys[0],
                init_patterns[0][round], 16U) &&
            ai_micro_m_memblock_bytes_match(boundary_phys[1],
                init_patterns[1][round], 16U) &&
            ai_micro_m_memblock_bytes_match(guard_before_phys,
                guard_before_patterns[round], 8U) &&
            ai_micro_m_memblock_bytes_match(guard_middle_phys,
                guard_middle_patterns[round], 16U) &&
            ai_micro_m_memblock_bytes_match(guard_after_phys,
                guard_after_patterns[round], 8U)
        );
    }

    ai_micro_m_memblock_reset_env();

    TEST_END();
}

bool ai_micro_m_load_cross_16b_same_width_template_switch_no_stale_merge_corner()
{
    TEST_START();

    static const unsigned start_bytes[2] = {
        4U,
        7U,
    };
    static const uint8_t init_patterns[2][2][16] = {
        {
            { 0xe2, 0xd3, 0xc4, 0xb5, 0xa6, 0x97, 0x88, 0x79,
              0x6a, 0x5b, 0x4c, 0x3d, 0x2e, 0x1f, 0x10, 0x01 },
            { 0x24, 0x35, 0x46, 0x57, 0x68, 0x79, 0x8a, 0x9b,
              0xac, 0xbd, 0xce, 0xdf, 0xe0, 0xf1, 0x02, 0x13 },
        },
        {
            { 0x5c, 0x4d, 0x3e, 0x2f, 0x10, 0x01, 0xf2, 0xe3,
              0xd4, 0xc5, 0xb6, 0xa7, 0x98, 0x89, 0x7a, 0x6b },
            { 0xb3, 0xa4, 0x95, 0x86, 0x77, 0x68, 0x59, 0x4a,
              0x3b, 0x2c, 0x1d, 0x0e, 0xff, 0xe1, 0xd2, 0xc3 },
        },
    };
    static const uint8_t guard_before_patterns[2][8] = {
        { 0x16, 0x28, 0x3a, 0x4c, 0x5e, 0x60, 0x72, 0x84 },
        { 0x8e, 0x7c, 0x6a, 0x58, 0x46, 0x34, 0x22, 0x10 },
    };
    static const uint8_t guard_middle_patterns[2][16] = {
        { 0xb0, 0xc2, 0xd4, 0xe6, 0xf8, 0x0a, 0x1c, 0x2e,
          0x3f, 0x50, 0x61, 0x72, 0x83, 0x94, 0xa5, 0xb6 },
        { 0x19, 0x2b, 0x3d, 0x4f, 0x51, 0x63, 0x75, 0x87,
          0x99, 0xab, 0xbd, 0xcf, 0xd1, 0xe3, 0xf5, 0x07 },
    };
    static const uint8_t guard_after_patterns[2][8] = {
        { 0x27, 0x49, 0x6b, 0x8d, 0xaf, 0xc1, 0xd3, 0xe5 },
        { 0x4d, 0x5f, 0x71, 0x83, 0x95, 0xa7, 0xb9, 0xcb },
    };
    const enum test_page page = SWITCH1;
    const uintptr_t load_vaddr[2] = {
        ai_micro_m_memblock_cross_16b_addr_with_offset(
            page, AI_MICRO_MEMBLOCK_CROSS_16B_BOUNDARY0_SD_4B4B_OFFSET),
        ai_micro_m_memblock_cross_16b_addr_with_offset(
            page, AI_MICRO_MEMBLOCK_CROSS_16B_BOUNDARY1_SD_OFFSET),
    };
    const uintptr_t boundary_phys[2] = {
        ai_micro_m_memblock_cross_16b_boundary_phys(0U),
        ai_micro_m_memblock_cross_16b_boundary_phys(1U),
    };
    const uintptr_t guard_before_phys =
        ai_micro_m_memblock_cross_16b_guard_before_phys();
    const uintptr_t guard_middle_phys =
        ai_micro_m_memblock_cross_16b_guard_middle_phys();
    const uintptr_t guard_after_phys =
        ai_micro_m_memblock_cross_16b_guard_after_phys();
    uint64_t observed_value = 0ULL;

    for (unsigned round = 0; round < 2U; ++round) {
        const bool forward = (round == 0U);
        const unsigned first = forward ? 0U : 1U;
        const unsigned second = forward ? 1U : 0U;
        const uint64_t expected_first =
            ai_micro_m_memblock_extract_scalar_from_bytes(
                init_patterns[first][round], start_bytes[first],
                sizeof(uint64_t), false);
        const uint64_t expected_second =
            ai_micro_m_memblock_extract_scalar_from_bytes(
                init_patterns[second][round], start_bytes[second],
                sizeof(uint64_t), false);

        goto_priv(PRIV_M);
        ai_micro_m_memblock_reset_env();
        ai_micro_m_memblock_write_bytes(boundary_phys[0],
            init_patterns[0][round], 16U);
        ai_micro_m_memblock_write_bytes(boundary_phys[1],
            init_patterns[1][round], 16U);
        ai_micro_m_memblock_write_bytes(guard_before_phys,
            guard_before_patterns[round], 8U);
        ai_micro_m_memblock_write_bytes(guard_middle_phys,
            guard_middle_patterns[round], 16U);
        ai_micro_m_memblock_write_bytes(guard_after_phys,
            guard_after_patterns[round], 8U);

        ai_micro_m_memblock_prepare_supervisor_translation();
        ai_micro_m_memblock_set_cross_16b_leaves(PTE_V | PTE_AD | PTE_RWX);
        sfence_vma();

        ai_micro_m_memblock_enable_mprv_supervisor_translation();
        TEST_SETUP_EXCEPT();
        observed_value = ld(load_vaddr[first]);
        TEST_ASSERT("the first no-H same-page same-width cross-16B load returns the value selected by its own split template before the different-template follow-up",
            excpt.triggered == false &&
            observed_value == expected_first
        );

        goto_priv(PRIV_M);
        TEST_ASSERT("after the first no-H same-page same-width cross-16B load, both boundary windows and all guard regions remain unchanged",
            ai_micro_m_memblock_bytes_match(boundary_phys[0],
                init_patterns[0][round], 16U) &&
            ai_micro_m_memblock_bytes_match(boundary_phys[1],
                init_patterns[1][round], 16U) &&
            ai_micro_m_memblock_bytes_match(guard_before_phys,
                guard_before_patterns[round], 8U) &&
            ai_micro_m_memblock_bytes_match(guard_middle_phys,
                guard_middle_patterns[round], 16U) &&
            ai_micro_m_memblock_bytes_match(guard_after_phys,
                guard_after_patterns[round], 8U)
        );

        ai_micro_m_memblock_enable_mprv_supervisor_translation();
        TEST_SETUP_EXCEPT();
        observed_value = ld(load_vaddr[second]);
        TEST_ASSERT("the second no-H same-page same-width cross-16B load also returns the value selected by its own split template instead of inheriting stale merge parameters from the earlier different-template load",
            excpt.triggered == false &&
            observed_value == expected_second
        );

        goto_priv(PRIV_M);
        TEST_ASSERT("after the no-H same-page same-width cross-16B different-template load sequence, both boundary windows and all guard regions still remain unchanged because both accesses are read-only",
            ai_micro_m_memblock_bytes_match(boundary_phys[0],
                init_patterns[0][round], 16U) &&
            ai_micro_m_memblock_bytes_match(boundary_phys[1],
                init_patterns[1][round], 16U) &&
            ai_micro_m_memblock_bytes_match(guard_before_phys,
                guard_before_patterns[round], 8U) &&
            ai_micro_m_memblock_bytes_match(guard_middle_phys,
                guard_middle_patterns[round], 16U) &&
            ai_micro_m_memblock_bytes_match(guard_after_phys,
                guard_after_patterns[round], 8U)
        );
    }

    ai_micro_m_memblock_reset_env();

    TEST_END();
}

bool ai_micro_m_load_cross_16b_fault_path_same_width_template_switch_no_stale_merge_corner()
{
    TEST_START();

    test_status = test_status &&
        ai_micro_m_memblock_samepage_cross_16b_load_fault_same_width_template_switch_window(
            false);

    TEST_END();
}

bool ai_micro_m_load_cross_16b_fault_path_same_width_template_switch_adjacent_preserved_corner()
{
    TEST_START();

    test_status = test_status &&
        ai_micro_m_memblock_samepage_cross_16b_load_fault_same_width_template_switch_window(
            true);

    TEST_END();
}

bool ai_micro_m_load_cross_16b_fault_path_repair_then_valid_next_boundary_same_width_template_load_then_same_boundary_new_fault_refreshes_current_template_tval_corner()
{
    TEST_START();

    test_status = test_status &&
        ai_micro_m_memblock_samepage_cross_16b_load_fault_repair_then_next_boundary_template_fault_window(
            false);

    TEST_END();
}

bool ai_micro_m_load_cross_16b_fault_path_repair_then_valid_next_boundary_same_width_template_load_then_original_boundary_new_fault_refreshes_back_template_tval_corner()
{
    TEST_START();

    test_status = test_status &&
        ai_micro_m_memblock_samepage_cross_16b_load_fault_repair_then_return_fault_same_width_template_window(
            false);

    TEST_END();
}

bool ai_micro_m_load_cross_16b_fault_path_repair_then_valid_next_boundary_same_width_template_load_then_returned_cross_producer_store_fault_refreshes_back_cause_tval_corner()
{
    TEST_START();

    test_status = test_status &&
        ai_micro_m_memblock_samepage_cross_16b_load_fault_repair_then_return_cross_producer_fault_same_width_template_window(
            false);

    TEST_END();
}

bool ai_micro_m_load_cross_16b_fault_path_repair_then_valid_next_boundary_same_width_template_load_then_cross_producer_store_fault_refreshes_current_cause_tval_corner()
{
    TEST_START();

    test_status = test_status &&
        ai_micro_m_memblock_samepage_cross_16b_load_fault_repair_then_next_boundary_cross_producer_fault_window(
            false);

    TEST_END();
}

bool ai_micro_m_store_cross_16b_second_split_access_fault_repeated_then_different_address_valid_store_recovers_corner()
{
    TEST_START();

    const enum test_page page = SWITCH1;
    const uintptr_t fault_offset = AI_MICRO_MEMBLOCK_CROSS_16B_OFFSET;
    const uintptr_t survivor_offset =
        AI_MICRO_MEMBLOCK_CROSS_16B_BOUNDARY1_SD_4B4B_OFFSET;
    const uintptr_t fault_vaddr =
        ai_micro_m_memblock_cross_16b_addr_with_offset(page, fault_offset);
    const uintptr_t survivor_vaddr =
        ai_micro_m_memblock_cross_16b_addr_with_offset(page, survivor_offset);
    const uintptr_t fault_low_phys =
        ai_micro_m_memblock_cross_16b_low_phys_with_offset(page, fault_offset);
    const uintptr_t fault_high_phys =
        ai_micro_m_memblock_cross_16b_high_phys_with_offset(page, fault_offset);
    const uintptr_t survivor_low_phys =
        ai_micro_m_memblock_cross_16b_low_phys_with_offset(page, survivor_offset);
    const uintptr_t survivor_high_phys =
        ai_micro_m_memblock_cross_16b_high_phys_with_offset(page, survivor_offset);
    const uintptr_t deny_start = fault_high_phys;
    const uintptr_t deny_end = fault_high_phys + PAGE_SIZE;
    const uint64_t fault_low_pattern = 0x1021324354657687ULL;
    const uint64_t fault_high_pattern = 0x98badcfe10325476ULL;
    const uint64_t survivor_low_pattern = 0x0f1e2d3c4b5a6978ULL;
    const uint64_t survivor_high_pattern = 0x8877665544332211ULL;
    const uint64_t fault_store_value = 0xa1b2c3d4e5f60718ULL;
    const uint64_t survivor_store_value = 0x89abcdef01234567ULL;
    const uint64_t expected_survivor_low =
        ai_micro_m_memblock_expected_low_after_4b_store(
            survivor_low_pattern, survivor_store_value);
    const uint64_t expected_survivor_high =
        ai_micro_m_memblock_expected_high_after_4b_store(
            survivor_high_pattern, survivor_store_value);

    /*
     * Addresses A and B are translated 8-byte stores whose effective address is
     * low_page_end - 4, so each sd is split as low-page 4B + high-page 4B.
     * This probe currently checks the no-side-effect policy for a store whose
     * high-page half is rejected by PMP.
     */
    goto_priv(PRIV_M);
    ai_micro_m_memblock_reset_env();
    write64(fault_low_phys, fault_low_pattern);
    write64(fault_high_phys, fault_high_pattern);
    write64(survivor_low_phys, survivor_low_pattern);
    write64(survivor_high_phys, survivor_high_pattern);
    ai_micro_m_memblock_configure_cross_16b_pmp_window(
        deny_start, deny_end, AI_MICRO_MEMBLOCK_PMP_TOR_NO_W);

    ai_micro_m_memblock_prepare_supervisor_translation();
    ai_micro_m_memblock_set_cross_16b_leaves(PTE_V | PTE_AD | PTE_RWX);
    sfence_vma();

    ai_micro_m_memblock_enable_mprv_supervisor_translation();
    TEST_SETUP_EXCEPT();
    sd(fault_vaddr, fault_store_value);
    TEST_ASSERT("the first translated cross-page 4B+4B split-store probe on address A reports store access fault with tval at the high-page start when that second split is PMP no-W",
        excpt.triggered == true &&
        excpt.cause == CAUSE_SAF &&
        excpt.tval == ai_micro_m_memblock_cross_16b_second_half_vaddr(fault_vaddr)
    );

    ai_micro_m_memblock_enable_mprv_supervisor_translation();
    TEST_SETUP_EXCEPT();
    sd(fault_vaddr, fault_store_value);
    TEST_ASSERT("repeating the translated cross-page 4B+4B split-store fault on address A keeps the same store-fault cause and high-page-start tval before the different-address recovery probe",
        excpt.triggered == true &&
        excpt.cause == CAUSE_SAF &&
        excpt.tval == ai_micro_m_memblock_cross_16b_second_half_vaddr(fault_vaddr)
    );

    goto_priv(PRIV_M);
    TEST_ASSERT("no-side-effect check before different-address recovery: repeated high-page-denied split stores on address A must not expose the low-page 4B partial store, and address B remains untouched",
        read64(fault_low_phys) == fault_low_pattern &&
        read64(fault_high_phys) == fault_high_pattern &&
        read64(survivor_low_phys) == survivor_low_pattern &&
        read64(survivor_high_phys) == survivor_high_pattern
    );

    ai_micro_m_memblock_restore_pmp_full_access();

    ai_micro_m_memblock_enable_mprv_supervisor_translation();
    TEST_SETUP_EXCEPT();
    sd(survivor_vaddr, survivor_store_value);
    TEST_ASSERT("after the repeated split-store fault episode on address A is repaired, a different translated cross-page 4B+4B store on address B must complete without inheriting stale exception metadata from address A",
        excpt.triggered == false
    );

    goto_priv(PRIV_M);
    TEST_ASSERT("no-side-effect recovery check: address A remains original after its denied split stores, while the different-address store B updates only its expected 4B+4B merged bytes",
        read64(fault_low_phys) == fault_low_pattern &&
        read64(fault_high_phys) == fault_high_pattern &&
        read64(survivor_low_phys) == expected_survivor_low &&
        read64(survivor_high_phys) == expected_survivor_high
    );

    ai_micro_m_memblock_reset_env();

    TEST_END();
}

bool ai_micro_m_store_cross_16b_second_split_access_fault_repeated_then_different_address_valid_load_recovers_corner()
{
    TEST_START();

    const enum test_page page = SWITCH1;
    const uintptr_t fault_offset = AI_MICRO_MEMBLOCK_CROSS_16B_OFFSET;
    const uintptr_t survivor_offset =
        AI_MICRO_MEMBLOCK_CROSS_16B_BOUNDARY1_SD_4B4B_OFFSET;
    const uintptr_t fault_vaddr =
        ai_micro_m_memblock_cross_16b_addr_with_offset(page, fault_offset);
    const uintptr_t survivor_vaddr =
        ai_micro_m_memblock_cross_16b_addr_with_offset(page, survivor_offset);
    const uintptr_t fault_low_phys =
        ai_micro_m_memblock_cross_16b_low_phys_with_offset(page, fault_offset);
    const uintptr_t fault_high_phys =
        ai_micro_m_memblock_cross_16b_high_phys_with_offset(page, fault_offset);
    const uintptr_t survivor_low_phys =
        ai_micro_m_memblock_cross_16b_low_phys_with_offset(page, survivor_offset);
    const uintptr_t survivor_high_phys =
        ai_micro_m_memblock_cross_16b_high_phys_with_offset(page, survivor_offset);
    const uintptr_t deny_start = fault_high_phys;
    const uintptr_t deny_end = fault_high_phys + PAGE_SIZE;
    const uint64_t fault_low_pattern = 0x1021324354657687ULL;
    const uint64_t fault_high_pattern = 0x98badcfe10325476ULL;
    const uint64_t survivor_low_pattern = 0x0123456789abcdefULL;
    const uint64_t survivor_high_pattern = 0xfedcba9876543210ULL;
    const uint64_t fault_store_value = 0xa1b2c3d4e5f60718ULL;
    const uint64_t expected_survivor_value =
        ai_micro_m_memblock_expected_cross_page_4b4b_load(
            survivor_low_pattern, survivor_high_pattern);
    uint64_t observed_value = 0ULL;

    goto_priv(PRIV_M);
    ai_micro_m_memblock_reset_env();
    write64(fault_low_phys, fault_low_pattern);
    write64(fault_high_phys, fault_high_pattern);
    write64(survivor_low_phys, survivor_low_pattern);
    write64(survivor_high_phys, survivor_high_pattern);
    ai_micro_m_memblock_configure_cross_16b_pmp_window(
        deny_start, deny_end, AI_MICRO_MEMBLOCK_PMP_TOR_NO_W);

    ai_micro_m_memblock_prepare_supervisor_translation();
    ai_micro_m_memblock_set_cross_16b_leaves(PTE_V | PTE_AD | PTE_RWX);
    sfence_vma();

    ai_micro_m_memblock_enable_mprv_supervisor_translation();
    TEST_SETUP_EXCEPT();
    sd(fault_vaddr, fault_store_value);
    TEST_ASSERT("the first no-H same-page cross-16B split-store probe on address A reports store access fault with the original misaligned VA before the different-address load producer-switch recovery probe",
        excpt.triggered == true &&
        excpt.cause == CAUSE_SAF &&
        excpt.tval == ai_micro_m_memblock_cross_16b_second_half_vaddr(fault_vaddr)
    );

    ai_micro_m_memblock_enable_mprv_supervisor_translation();
    TEST_SETUP_EXCEPT();
    sd(fault_vaddr, fault_store_value);
    TEST_ASSERT("repeating the no-H same-page cross-16B split-store fault on address A keeps the same precise metadata before the different-address load producer-switch recovery probe",
        excpt.triggered == true &&
        excpt.cause == CAUSE_SAF &&
        excpt.tval == ai_micro_m_memblock_cross_16b_second_half_vaddr(fault_vaddr)
    );

    goto_priv(PRIV_M);
    TEST_ASSERT("before testing the different-address load producer-switch recovery probe, the repeated second-split-only same-page cross-16B store faults on address A leave both aligned doublewords of address A and address B untouched",
        read64(fault_low_phys) == fault_low_pattern &&
        read64(fault_high_phys) == fault_high_pattern &&
        read64(survivor_low_phys) == survivor_low_pattern &&
        read64(survivor_high_phys) == survivor_high_pattern
    );

    ai_micro_m_memblock_restore_pmp_full_access();

    ai_micro_m_memblock_enable_mprv_supervisor_translation();
    TEST_SETUP_EXCEPT();
    observed_value = ld(survivor_vaddr);
    TEST_ASSERT("after the repeated no-H same-page cross-16B store fault episode on address A is repaired, a different same-page cross-16B load on address B must complete without inheriting stale store-side exception metadata from address A",
        excpt.triggered == false &&
        observed_value == expected_survivor_value
    );

    goto_priv(PRIV_M);
    TEST_ASSERT("the different-address no-H same-page cross-16B load producer-switch recovery probe leaves both aligned doublewords of address A and address B unchanged because the recovery access is read-only",
        read64(fault_low_phys) == fault_low_pattern &&
        read64(fault_high_phys) == fault_high_pattern &&
        read64(survivor_low_phys) == survivor_low_pattern &&
        read64(survivor_high_phys) == survivor_high_pattern
    );

    ai_micro_m_memblock_reset_env();

    TEST_END();
}

bool ai_micro_m_store_cross_16b_second_split_access_fault_repeated_then_same_address_valid_load_recovers_corner()
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
    const uint64_t store_value = 0xa1b2c3d4e5f60718ULL;
    const uint64_t expected_value =
        ai_micro_m_memblock_expected_cross_page_4b4b_load(
            low_pattern, high_pattern);
    uint64_t observed_value = 0ULL;

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
    sd(vaddr, store_value);
    TEST_ASSERT("the first no-H same-page cross-16B split-store probe on address A reports store access fault with the original misaligned VA before the same-address load producer-switch recovery probe",
        excpt.triggered == true &&
        excpt.cause == CAUSE_SAF &&
        excpt.tval == ai_micro_m_memblock_cross_16b_second_half_vaddr(vaddr)
    );

    ai_micro_m_memblock_enable_mprv_supervisor_translation();
    TEST_SETUP_EXCEPT();
    sd(vaddr, store_value);
    TEST_ASSERT("repeating the no-H same-page cross-16B split-store fault on address A keeps the same precise metadata before the same-address load producer-switch recovery probe",
        excpt.triggered == true &&
        excpt.cause == CAUSE_SAF &&
        excpt.tval == ai_micro_m_memblock_cross_16b_second_half_vaddr(vaddr)
    );

    goto_priv(PRIV_M);
    TEST_ASSERT("before the same-address load producer-switch recovery probe, the repeated second-split-only same-page cross-16B store faults leave both aligned doublewords untouched",
        read64(low_phys) == low_pattern &&
        read64(high_phys) == high_pattern
    );

    ai_micro_m_memblock_restore_pmp_full_access();

    ai_micro_m_memblock_enable_mprv_supervisor_translation();
    TEST_SETUP_EXCEPT();
    observed_value = ld(vaddr);
    TEST_ASSERT("after the repeated no-H same-page cross-16B store fault episode on address A is repaired, a same-address cross-16B load on address A must complete without inheriting stale store-side exception metadata",
        excpt.triggered == false &&
        observed_value == expected_value
    );

    goto_priv(PRIV_M);
    TEST_ASSERT("the no-H same-address cross-16B load producer-switch recovery probe remains read-only and leaves both aligned doublewords unchanged",
        read64(low_phys) == low_pattern &&
        read64(high_phys) == high_pattern
    );

    ai_micro_m_memblock_reset_env();

    TEST_END();
}

bool ai_micro_m_store_cross_16b_second_split_access_fault_repeated_then_same_address_different_width_signedness_load_recovers_corner()
{
    TEST_START();

    static const bool recovery_signed[2] = {
        false,
        true,
    };
    static const uint8_t init_patterns[2][2][16] = {
        {
            { 0x11, 0x24, 0x37, 0x4a, 0x5d, 0x70, 0x83, 0xf1,
              0x80, 0x9c, 0xaf, 0xc2, 0xd5, 0xe8, 0xfb, 0x0e },
            { 0xd2, 0xc1, 0xb0, 0xaf, 0x9e, 0x8d, 0x7c, 0x80,
              0x81, 0x62, 0x53, 0x44, 0x35, 0x26, 0x17, 0x08 },
        },
        {
            { 0x90, 0xa1, 0xb2, 0xc3, 0xd4, 0xe5, 0x7f, 0x80,
              0x81, 0x2a, 0x3b, 0x4c, 0x5d, 0x6e, 0x7f, 0x80 },
            { 0x3e, 0x4f, 0x60, 0x71, 0x82, 0x7f, 0x80, 0x81,
              0x92, 0xa3, 0xb4, 0xc5, 0xd6, 0xe7, 0xf8, 0x09 },
        },
    };
    static const uint8_t guard_before_patterns[2][8] = {
        { 0x15, 0x27, 0x39, 0x4b, 0x5d, 0x6f, 0x71, 0x83 },
        { 0x8d, 0x7b, 0x69, 0x57, 0x45, 0x33, 0x21, 0x1f },
    };
    static const uint8_t guard_middle_patterns[2][16] = {
        { 0xa0, 0xb2, 0xc4, 0xd6, 0xe8, 0xfa, 0x0c, 0x1e,
          0x2f, 0x40, 0x51, 0x62, 0x73, 0x84, 0x95, 0xa6 },
        { 0x1a, 0x2c, 0x3e, 0x40, 0x52, 0x64, 0x76, 0x88,
          0x9a, 0xac, 0xbe, 0xc0, 0xd2, 0xe4, 0xf6, 0x08 },
    };
    static const uint8_t guard_after_patterns[2][8] = {
        { 0x26, 0x48, 0x6a, 0x8c, 0xae, 0xc0, 0xd2, 0xe4 },
        { 0x4c, 0x5e, 0x70, 0x82, 0x94, 0xa6, 0xb8, 0xca },
    };
    const enum test_page page = SWITCH1;
    const uintptr_t target_vaddr[2] = {
        ai_micro_m_memblock_cross_16b_addr_with_offset(
            page, AI_MICRO_MEMBLOCK_CROSS_16B_3B1B_OFFSET),
        ai_micro_m_memblock_cross_16b_addr_with_offset(
            page, AI_MICRO_MEMBLOCK_CROSS_16B_BOUNDARY1_LW_OFFSET),
    };
    const uintptr_t boundary_phys[2] = {
        ai_micro_m_memblock_cross_16b_boundary_phys(0U),
        ai_micro_m_memblock_cross_16b_boundary_phys(1U),
    };
    const uintptr_t guard_before_phys =
        ai_micro_m_memblock_cross_16b_guard_before_phys();
    const uintptr_t guard_middle_phys =
        ai_micro_m_memblock_cross_16b_guard_middle_phys();
    const uintptr_t guard_after_phys =
        ai_micro_m_memblock_cross_16b_guard_after_phys();
    uint64_t observed_value = 0ULL;

    for (unsigned round = 0; round < 2U; ++round) {
        const unsigned target = round;
        const unsigned other = target ^ 1U;
        const uintptr_t vaddr = target_vaddr[target];
        const uintptr_t deny_start = boundary_phys[target] + 8U;
        const uintptr_t deny_end = deny_start + PAGE_SIZE;
        const uint64_t expected_value =
            ai_micro_m_memblock_extract_scalar_from_bytes(
                init_patterns[target][round], 5U, sizeof(uint32_t),
                recovery_signed[round]);

        goto_priv(PRIV_M);
        ai_micro_m_memblock_reset_env();
        ai_micro_m_memblock_write_bytes(boundary_phys[0],
            init_patterns[0][round], 16U);
        ai_micro_m_memblock_write_bytes(boundary_phys[1],
            init_patterns[1][round], 16U);
        ai_micro_m_memblock_write_bytes(guard_before_phys,
            guard_before_patterns[round], 8U);
        ai_micro_m_memblock_write_bytes(guard_middle_phys,
            guard_middle_patterns[round], 16U);
        ai_micro_m_memblock_write_bytes(guard_after_phys,
            guard_after_patterns[round], 8U);
        ai_micro_m_memblock_configure_cross_16b_pmp_window(
            deny_start, deny_end, AI_MICRO_MEMBLOCK_PMP_TOR_NO_W);

        ai_micro_m_memblock_prepare_supervisor_translation();
        ai_micro_m_memblock_set_cross_16b_leaves(PTE_V | PTE_AD | PTE_RWX);
        sfence_vma();

        TEST_SETUP_EXCEPT();
        ai_micro_m_memblock_translated_store_by_size(vaddr, 8U,
            0xa1b2c3d4e5f60718ULL + round);
        TEST_ASSERT("the first no-H same-page cross-16B same-address store fault in the cross-producer width/signedness recovery probe reports store access fault with the original misaligned VA",
            excpt.triggered == true &&
            excpt.cause == CAUSE_SAF &&
            excpt.tval == ai_micro_m_memblock_cross_16b_second_half_vaddr(vaddr)
        );

        TEST_SETUP_EXCEPT();
        ai_micro_m_memblock_translated_store_by_size(vaddr, 8U,
            0xa1b2c3d4e5f60718ULL + round);
        TEST_ASSERT("repeating the no-H same-page cross-16B same-address store fault keeps the same precise metadata before the different-width signedness-switched load recovery probe",
            excpt.triggered == true &&
            excpt.cause == CAUSE_SAF &&
            excpt.tval == ai_micro_m_memblock_cross_16b_second_half_vaddr(vaddr)
        );

        goto_priv(PRIV_M);
        TEST_ASSERT("before the same-address cross-producer different-width signedness-switched load recovery probe, both boundary windows and all guard regions remain untouched",
            ai_micro_m_memblock_bytes_match(boundary_phys[0],
                init_patterns[0][round], 16U) &&
            ai_micro_m_memblock_bytes_match(boundary_phys[1],
                init_patterns[1][round], 16U) &&
            ai_micro_m_memblock_bytes_match(guard_before_phys,
                guard_before_patterns[round], 8U) &&
            ai_micro_m_memblock_bytes_match(guard_middle_phys,
                guard_middle_patterns[round], 16U) &&
            ai_micro_m_memblock_bytes_match(guard_after_phys,
                guard_after_patterns[round], 8U)
        );

        ai_micro_m_memblock_restore_pmp_full_access();

        TEST_SETUP_EXCEPT();
        observed_value = ai_micro_m_memblock_translated_scalar_load(
            recovery_signed[round], sizeof(uint32_t), vaddr);
        TEST_ASSERT("after the repeated no-H same-page cross-16B store fault episode, the same-address different-width signedness-switched load recovery probe must return the value selected by its current decode without inheriting stale store-side exception metadata",
            excpt.triggered == false &&
            observed_value == expected_value
        );

        goto_priv(PRIV_M);
        TEST_ASSERT("the no-H same-address cross-producer different-width signedness-switched load recovery probe remains read-only and leaves both boundary windows plus all guard regions unchanged",
            ai_micro_m_memblock_bytes_match(boundary_phys[0],
                init_patterns[0][round], 16U) &&
            ai_micro_m_memblock_bytes_match(boundary_phys[1],
                init_patterns[1][round], 16U) &&
            ai_micro_m_memblock_bytes_match(guard_before_phys,
                guard_before_patterns[round], 8U) &&
            ai_micro_m_memblock_bytes_match(guard_middle_phys,
                guard_middle_patterns[round], 16U) &&
            ai_micro_m_memblock_bytes_match(guard_after_phys,
                guard_after_patterns[round], 8U)
        );
    }

    ai_micro_m_memblock_reset_env();

    TEST_END();
}

bool ai_micro_m_store_cross_16b_second_split_access_fault_repeated_then_different_address_same_width_different_template_load_recovers_corner()
{
    TEST_START();

    static const uintptr_t fault_offsets[2] = {
        AI_MICRO_MEMBLOCK_CROSS_16B_BOUNDARY1_SD_OFFSET,
        AI_MICRO_MEMBLOCK_CROSS_16B_BOUNDARY0_SD_4B4B_OFFSET,
    };
    static const uintptr_t recovery_offsets[2] = {
        AI_MICRO_MEMBLOCK_CROSS_16B_BOUNDARY0_SD_4B4B_OFFSET,
        AI_MICRO_MEMBLOCK_CROSS_16B_BOUNDARY1_SD_OFFSET,
    };
    static const unsigned recovery_start_bytes[2] = {
        4U,
        7U,
    };
    static const uint8_t init_patterns[2][2][16] = {
        {
            { 0xe2, 0xd3, 0xc4, 0xb5, 0xa6, 0x97, 0x88, 0x79,
              0x6a, 0x5b, 0x4c, 0x3d, 0x2e, 0x1f, 0x10, 0x01 },
            { 0x24, 0x35, 0x46, 0x57, 0x68, 0x79, 0x8a, 0x9b,
              0xac, 0xbd, 0xce, 0xdf, 0xe0, 0xf1, 0x02, 0x13 },
        },
        {
            { 0x5c, 0x4d, 0x3e, 0x2f, 0x10, 0x01, 0xf2, 0xe3,
              0xd4, 0xc5, 0xb6, 0xa7, 0x98, 0x89, 0x7a, 0x6b },
            { 0xb3, 0xa4, 0x95, 0x86, 0x77, 0x68, 0x59, 0x4a,
              0x3b, 0x2c, 0x1d, 0x0e, 0xff, 0xe1, 0xd2, 0xc3 },
        },
    };
    static const uint8_t guard_before_patterns[2][8] = {
        { 0x16, 0x28, 0x3a, 0x4c, 0x5e, 0x60, 0x72, 0x84 },
        { 0x8e, 0x7c, 0x6a, 0x58, 0x46, 0x34, 0x22, 0x10 },
    };
    static const uint8_t guard_middle_patterns[2][16] = {
        { 0xb0, 0xc2, 0xd4, 0xe6, 0xf8, 0x0a, 0x1c, 0x2e,
          0x3f, 0x50, 0x61, 0x72, 0x83, 0x94, 0xa5, 0xb6 },
        { 0x19, 0x2b, 0x3d, 0x4f, 0x51, 0x63, 0x75, 0x87,
          0x99, 0xab, 0xbd, 0xcf, 0xd1, 0xe3, 0xf5, 0x07 },
    };
    static const uint8_t guard_after_patterns[2][8] = {
        { 0x27, 0x49, 0x6b, 0x8d, 0xaf, 0xc1, 0xd3, 0xe5 },
        { 0x4d, 0x5f, 0x71, 0x83, 0x95, 0xa7, 0xb9, 0xcb },
    };
    const enum test_page page = SWITCH1;
    const uintptr_t boundary_phys[2] = {
        ai_micro_m_memblock_cross_16b_boundary_phys(0U),
        ai_micro_m_memblock_cross_16b_boundary_phys(1U),
    };
    const uintptr_t guard_before_phys =
        ai_micro_m_memblock_cross_16b_guard_before_phys();
    const uintptr_t guard_middle_phys =
        ai_micro_m_memblock_cross_16b_guard_middle_phys();
    const uintptr_t guard_after_phys =
        ai_micro_m_memblock_cross_16b_guard_after_phys();
    uint64_t observed_value = 0ULL;

    for (unsigned round = 0; round < 2U; ++round) {
        const unsigned fault_index = round ^ 1U;
        const unsigned recovery_index = fault_index ^ 1U;
        const uintptr_t fault_vaddr =
            ai_micro_m_memblock_cross_16b_addr_with_offset(page,
                fault_offsets[round]);
        const uintptr_t recovery_vaddr =
            ai_micro_m_memblock_cross_16b_addr_with_offset(page,
                recovery_offsets[round]);
        const uintptr_t deny_start = boundary_phys[fault_index] + 8U;
        const uintptr_t deny_end = deny_start + PAGE_SIZE;
        const uint64_t expected_value =
            ai_micro_m_memblock_extract_scalar_from_bytes(
                init_patterns[recovery_index][round],
                recovery_start_bytes[round], sizeof(uint64_t), false);

        goto_priv(PRIV_M);
        ai_micro_m_memblock_reset_env();
        ai_micro_m_memblock_write_bytes(boundary_phys[0],
            init_patterns[0][round], 16U);
        ai_micro_m_memblock_write_bytes(boundary_phys[1],
            init_patterns[1][round], 16U);
        ai_micro_m_memblock_write_bytes(guard_before_phys,
            guard_before_patterns[round], 8U);
        ai_micro_m_memblock_write_bytes(guard_middle_phys,
            guard_middle_patterns[round], 16U);
        ai_micro_m_memblock_write_bytes(guard_after_phys,
            guard_after_patterns[round], 8U);
        ai_micro_m_memblock_configure_cross_16b_pmp_window(
            deny_start, deny_end, AI_MICRO_MEMBLOCK_PMP_TOR_NO_W);

        ai_micro_m_memblock_prepare_supervisor_translation();
        ai_micro_m_memblock_set_cross_16b_leaves(PTE_V | PTE_AD | PTE_RWX);
        sfence_vma();

        TEST_SETUP_EXCEPT();
        ai_micro_m_memblock_translated_sd(fault_vaddr,
            0xa1b2c3d4e5f60718ULL + round);
        TEST_ASSERT("the first no-H same-page same-width different-template store fault reports store access fault with the original misaligned VA before the different-address load recovery probe",
            excpt.triggered == true &&
            excpt.cause == CAUSE_SAF &&
            excpt.tval == ai_micro_m_memblock_cross_16b_second_half_vaddr(fault_vaddr)
        );

        TEST_SETUP_EXCEPT();
        ai_micro_m_memblock_translated_sd(fault_vaddr,
            0xa1b2c3d4e5f60718ULL + round);
        TEST_ASSERT("repeating the no-H same-page same-width different-template store fault keeps the same precise metadata before the different-address load recovery probe",
            excpt.triggered == true &&
            excpt.cause == CAUSE_SAF &&
            excpt.tval == ai_micro_m_memblock_cross_16b_second_half_vaddr(fault_vaddr)
        );

        goto_priv(PRIV_M);
        TEST_ASSERT("before the different-address same-width different-template load recovery probe, both boundary windows and all guard regions remain untouched",
            ai_micro_m_memblock_bytes_match(boundary_phys[0],
                init_patterns[0][round], 16U) &&
            ai_micro_m_memblock_bytes_match(boundary_phys[1],
                init_patterns[1][round], 16U) &&
            ai_micro_m_memblock_bytes_match(guard_before_phys,
                guard_before_patterns[round], 8U) &&
            ai_micro_m_memblock_bytes_match(guard_middle_phys,
                guard_middle_patterns[round], 16U) &&
            ai_micro_m_memblock_bytes_match(guard_after_phys,
                guard_after_patterns[round], 8U)
        );

        ai_micro_m_memblock_restore_pmp_full_access();

        ai_micro_m_memblock_enable_mprv_supervisor_translation();
        TEST_SETUP_EXCEPT();
        observed_value = ld(recovery_vaddr);
        TEST_ASSERT("after the repeated no-H same-page same-width different-template store fault episode, the different-address load recovery probe must return the current template value without inheriting stale store-side metadata",
            excpt.triggered == false &&
            observed_value == expected_value
        );

        goto_priv(PRIV_M);
        TEST_ASSERT("the different-address same-width different-template load recovery probe remains read-only and leaves both boundary windows plus all guard regions unchanged",
            ai_micro_m_memblock_bytes_match(boundary_phys[0],
                init_patterns[0][round], 16U) &&
            ai_micro_m_memblock_bytes_match(boundary_phys[1],
                init_patterns[1][round], 16U) &&
            ai_micro_m_memblock_bytes_match(guard_before_phys,
                guard_before_patterns[round], 8U) &&
            ai_micro_m_memblock_bytes_match(guard_middle_phys,
                guard_middle_patterns[round], 16U) &&
            ai_micro_m_memblock_bytes_match(guard_after_phys,
                guard_after_patterns[round], 8U)
        );
    }

    ai_micro_m_memblock_reset_env();

    TEST_END();
}

bool ai_micro_m_samepage_cross_16b_fault_switch_store_then_load_keeps_latest_cause_and_tval_corner()
{
    TEST_START();

    const enum test_page page = SWITCH1;
    const uintptr_t store_offset = AI_MICRO_MEMBLOCK_CROSS_16B_OFFSET;
    const uintptr_t load_offset =
        AI_MICRO_MEMBLOCK_CROSS_16B_BOUNDARY1_SD_4B4B_OFFSET;
    const uintptr_t store_vaddr =
        ai_micro_m_memblock_cross_16b_addr_with_offset(page, store_offset);
    const uintptr_t load_vaddr =
        ai_micro_m_memblock_cross_16b_addr_with_offset(page, load_offset);
    const uintptr_t store_low_phys =
        ai_micro_m_memblock_cross_16b_low_phys_with_offset(page, store_offset);
    const uintptr_t store_high_phys =
        ai_micro_m_memblock_cross_16b_high_phys_with_offset(page, store_offset);
    const uintptr_t load_low_phys =
        ai_micro_m_memblock_cross_16b_low_phys_with_offset(page, load_offset);
    const uintptr_t load_high_phys =
        ai_micro_m_memblock_cross_16b_high_phys_with_offset(page, load_offset);
    const uint64_t store_low_pattern = 0x21436587a9cbed0fULL;
    const uint64_t store_high_pattern = 0xf0debc9a78563412ULL;
    const uint64_t load_low_pattern = 0x7766554433221100ULL;
    const uint64_t load_high_pattern = 0xffeeddccbbaa9988ULL;
    const uint64_t store_value = 0xa1b2c3d4e5f60718ULL;
    uint64_t observed_value = 0ULL;

    goto_priv(PRIV_M);
    ai_micro_m_memblock_reset_env();
    write64(store_low_phys, store_low_pattern);
    write64(store_high_phys, store_high_pattern);
    write64(load_low_phys, load_low_pattern);
    write64(load_high_phys, load_high_pattern);
    ai_micro_m_memblock_configure_cross_16b_pmp_window(
        store_high_phys, store_high_phys + PAGE_SIZE,
        AI_MICRO_MEMBLOCK_PMP_TOR_NO_W);

    ai_micro_m_memblock_prepare_supervisor_translation();
    ai_micro_m_memblock_set_cross_16b_leaves(PTE_V | PTE_AD | PTE_RWX);
    sfence_vma();

    ai_micro_m_memblock_enable_mprv_supervisor_translation();
    TEST_SETUP_EXCEPT();
    sd(store_vaddr, store_value);
    TEST_ASSERT("the first no-H same-page cross-16B store->load producer-switch probe reports a store access fault with tval bound to the original misaligned store address",
        excpt.triggered == true &&
        excpt.cause == CAUSE_SAF &&
        excpt.tval == ai_micro_m_memblock_cross_16b_second_half_vaddr(store_vaddr)
    );

    goto_priv(PRIV_M);
    TEST_ASSERT("after the first no-H same-page cross-16B store fault in the producer-switch probe, both fault windows remain unchanged before the follow-up load fault",
        read64(store_low_phys) == store_low_pattern &&
        read64(store_high_phys) == store_high_pattern &&
        read64(load_low_phys) == load_low_pattern &&
        read64(load_high_phys) == load_high_pattern
    );

    ai_micro_m_memblock_configure_cross_16b_pmp_window(
        load_high_phys, load_high_phys + PAGE_SIZE,
        AI_MICRO_MEMBLOCK_PMP_TOR_NO_R);

    ai_micro_m_memblock_enable_mprv_supervisor_translation();
    TEST_SETUP_EXCEPT();
    observed_value = ld(load_vaddr);
    TEST_ASSERT("after the earlier same-page cross-16B store fault has already occupied the shared exception path, the follow-up load fault must refresh cause and tval to the newer load-side faulting address",
        excpt.triggered == true &&
        excpt.cause == CAUSE_LAF &&
        excpt.tval == ai_micro_m_memblock_cross_16b_second_half_vaddr(load_vaddr)
    );

    goto_priv(PRIV_M);
    TEST_ASSERT("the same-page cross-16B store->load producer-switch fault sequence leaves both windows unchanged because the second-split-only store fault and the follow-up load fault are both side-effect free",
        read64(store_low_phys) == store_low_pattern &&
        read64(store_high_phys) == store_high_pattern &&
        read64(load_low_phys) == load_low_pattern &&
        read64(load_high_phys) == load_high_pattern
    );

    ai_micro_m_memblock_reset_env();

    TEST_END();
}

bool ai_micro_m_samepage_cross_16b_same_address_fault_switch_store_then_load_keeps_latest_cause_and_tval_corner()
{
    TEST_START();

    const enum test_page page = SWITCH1;
    const uintptr_t vaddr = ai_micro_m_memblock_cross_16b_addr(page);
    const uintptr_t low_phys = ai_micro_m_memblock_cross_16b_low_phys(page);
    const uintptr_t high_phys = ai_micro_m_memblock_cross_16b_high_phys(page);
    const uint64_t low_pattern = 0x21436587a9cbed0fULL;
    const uint64_t high_pattern = 0xf0debc9a78563412ULL;
    const uint64_t store_value = 0xa1b2c3d4e5f60718ULL;
    uint64_t observed_value = 0ULL;

    goto_priv(PRIV_M);
    ai_micro_m_memblock_reset_env();
    write64(low_phys, low_pattern);
    write64(high_phys, high_pattern);
    ai_micro_m_memblock_configure_cross_16b_pmp_window(
        high_phys, high_phys + PAGE_SIZE,
        AI_MICRO_MEMBLOCK_PMP_TOR_NO_W);

    ai_micro_m_memblock_prepare_supervisor_translation();
    ai_micro_m_memblock_set_cross_16b_leaves(PTE_V | PTE_AD | PTE_RWX);
    sfence_vma();

    ai_micro_m_memblock_enable_mprv_supervisor_translation();
    TEST_SETUP_EXCEPT();
    sd(vaddr, store_value);
    TEST_ASSERT("the first no-H same-page cross-16B same-address store->load producer-switch probe reports a store access fault with tval bound to the original misaligned address",
        excpt.triggered == true &&
        excpt.cause == CAUSE_SAF &&
        excpt.tval == ai_micro_m_memblock_cross_16b_second_half_vaddr(vaddr)
    );

    goto_priv(PRIV_M);
    TEST_ASSERT("after the first no-H same-page cross-16B same-address store fault in the producer-switch probe, both aligned doublewords remain unchanged before the follow-up load fault",
        read64(low_phys) == low_pattern &&
        read64(high_phys) == high_pattern
    );

    ai_micro_m_memblock_configure_cross_16b_pmp_window(
        high_phys, high_phys + PAGE_SIZE,
        AI_MICRO_MEMBLOCK_PMP_TOR_NO_R);

    ai_micro_m_memblock_enable_mprv_supervisor_translation();
    TEST_SETUP_EXCEPT();
    observed_value = ld(vaddr);
    TEST_ASSERT("after the earlier same-page cross-16B store fault has already occupied the shared exception path, the follow-up same-address load fault must refresh cause and tval to the newer load-side faulting access",
        excpt.triggered == true &&
        excpt.cause == CAUSE_LAF &&
        excpt.tval == ai_micro_m_memblock_cross_16b_second_half_vaddr(vaddr)
    );

    goto_priv(PRIV_M);
    TEST_ASSERT("the same-page cross-16B same-address store->load producer-switch fault sequence leaves both aligned doublewords unchanged because both faults are side-effect free",
        read64(low_phys) == low_pattern &&
        read64(high_phys) == high_pattern
    );

    ai_micro_m_memblock_reset_env();

    TEST_END();
}

bool ai_micro_m_samepage_cross_16b_same_width_template_fault_switch_load_keeps_latest_tval_corner()
{
    TEST_START();

    static const uintptr_t load_offsets[2] = {
        AI_MICRO_MEMBLOCK_CROSS_16B_BOUNDARY0_SD_4B4B_OFFSET,
        AI_MICRO_MEMBLOCK_CROSS_16B_BOUNDARY1_SD_OFFSET,
    };
    static const uint8_t init_patterns[2][2][16] = {
        {
            { 0xe2, 0xd3, 0xc4, 0xb5, 0xa6, 0x97, 0x88, 0x79,
              0x6a, 0x5b, 0x4c, 0x3d, 0x2e, 0x1f, 0x10, 0x01 },
            { 0x24, 0x35, 0x46, 0x57, 0x68, 0x79, 0x8a, 0x9b,
              0xac, 0xbd, 0xce, 0xdf, 0xe0, 0xf1, 0x02, 0x13 },
        },
        {
            { 0x5c, 0x4d, 0x3e, 0x2f, 0x10, 0x01, 0xf2, 0xe3,
              0xd4, 0xc5, 0xb6, 0xa7, 0x98, 0x89, 0x7a, 0x6b },
            { 0xb3, 0xa4, 0x95, 0x86, 0x77, 0x68, 0x59, 0x4a,
              0x3b, 0x2c, 0x1d, 0x0e, 0xff, 0xe1, 0xd2, 0xc3 },
        },
    };
    static const uint8_t guard_before_patterns[2][8] = {
        { 0x16, 0x28, 0x3a, 0x4c, 0x5e, 0x60, 0x72, 0x84 },
        { 0x8e, 0x7c, 0x6a, 0x58, 0x46, 0x34, 0x22, 0x10 },
    };
    static const uint8_t guard_middle_patterns[2][16] = {
        { 0xb0, 0xc2, 0xd4, 0xe6, 0xf8, 0x0a, 0x1c, 0x2e,
          0x3f, 0x50, 0x61, 0x72, 0x83, 0x94, 0xa5, 0xb6 },
        { 0x19, 0x2b, 0x3d, 0x4f, 0x51, 0x63, 0x75, 0x87,
          0x99, 0xab, 0xbd, 0xcf, 0xd1, 0xe3, 0xf5, 0x07 },
    };
    static const uint8_t guard_after_patterns[2][8] = {
        { 0x27, 0x49, 0x6b, 0x8d, 0xaf, 0xc1, 0xd3, 0xe5 },
        { 0x4d, 0x5f, 0x71, 0x83, 0x95, 0xa7, 0xb9, 0xcb },
    };
    const enum test_page page = SWITCH1;
    const uintptr_t boundary_phys[2] = {
        ai_micro_m_memblock_cross_16b_boundary_phys(0U),
        ai_micro_m_memblock_cross_16b_boundary_phys(1U),
    };
    const uintptr_t guard_before_phys =
        ai_micro_m_memblock_cross_16b_guard_before_phys();
    const uintptr_t guard_middle_phys =
        ai_micro_m_memblock_cross_16b_guard_middle_phys();
    const uintptr_t guard_after_phys =
        ai_micro_m_memblock_cross_16b_guard_after_phys();
    uint64_t observed_value = 0ULL;

    for (unsigned round = 0; round < 2U; ++round) {
        const bool forward = (round == 0U);
        const unsigned first = forward ? 0U : 1U;
        const unsigned second = forward ? 1U : 0U;
        const uintptr_t first_vaddr =
            ai_micro_m_memblock_cross_16b_addr_with_offset(page,
                load_offsets[first]);
        const uintptr_t second_vaddr =
            ai_micro_m_memblock_cross_16b_addr_with_offset(page,
                load_offsets[second]);

        goto_priv(PRIV_M);
        ai_micro_m_memblock_reset_env();
        ai_micro_m_memblock_write_bytes(boundary_phys[0],
            init_patterns[0][round], 16U);
        ai_micro_m_memblock_write_bytes(boundary_phys[1],
            init_patterns[1][round], 16U);
        ai_micro_m_memblock_write_bytes(guard_before_phys,
            guard_before_patterns[round], 8U);
        ai_micro_m_memblock_write_bytes(guard_middle_phys,
            guard_middle_patterns[round], 16U);
        ai_micro_m_memblock_write_bytes(guard_after_phys,
            guard_after_patterns[round], 8U);
        ai_micro_m_memblock_configure_cross_16b_pmp_window(
            boundary_phys[first] + 8U, boundary_phys[first] + 16U,
            AI_MICRO_MEMBLOCK_PMP_TOR_NO_R);

        ai_micro_m_memblock_prepare_supervisor_translation();
        ai_micro_m_memblock_set_cross_16b_leaves(PTE_V | PTE_AD | PTE_RWX);
        sfence_vma();

        TEST_SETUP_EXCEPT();
        observed_value = ai_micro_m_memblock_translated_scalar_load(
            false, sizeof(uint64_t), first_vaddr);
        TEST_ASSERT("the first no-H same-page same-width different-template load fault reports a load access fault with tval bound to the original misaligned address",
            excpt.triggered == true &&
            excpt.cause == CAUSE_LAF &&
            excpt.tval == ai_micro_m_memblock_cross_16b_second_half_vaddr(first_vaddr)
        );

        goto_priv(PRIV_M);
        TEST_ASSERT("after the first no-H same-page same-width different-template load fault, both boundary windows and all guard regions remain unchanged before the follow-up load fault",
            ai_micro_m_memblock_bytes_match(boundary_phys[0],
                init_patterns[0][round], 16U) &&
            ai_micro_m_memblock_bytes_match(boundary_phys[1],
                init_patterns[1][round], 16U) &&
            ai_micro_m_memblock_bytes_match(guard_before_phys,
                guard_before_patterns[round], 8U) &&
            ai_micro_m_memblock_bytes_match(guard_middle_phys,
                guard_middle_patterns[round], 16U) &&
            ai_micro_m_memblock_bytes_match(guard_after_phys,
                guard_after_patterns[round], 8U)
        );

        ai_micro_m_memblock_configure_cross_16b_pmp_window(
            boundary_phys[second] + 8U, boundary_phys[second] + 16U,
            AI_MICRO_MEMBLOCK_PMP_TOR_NO_R);

        TEST_SETUP_EXCEPT();
        observed_value = ai_micro_m_memblock_translated_scalar_load(
            false, sizeof(uint64_t), second_vaddr);
        TEST_ASSERT("after the earlier same-page same-width different-template load fault has already occupied the shared exception path, the follow-up load fault must refresh tval to the newer template-specific address",
            excpt.triggered == true &&
            excpt.cause == CAUSE_LAF &&
            excpt.tval == ai_micro_m_memblock_cross_16b_second_half_vaddr(second_vaddr)
        );

        goto_priv(PRIV_M);
        TEST_ASSERT("the same-page same-width different-template load fault-switch sequence leaves both boundary windows and all guard regions unchanged because both faults are read-only",
            ai_micro_m_memblock_bytes_match(boundary_phys[0],
                init_patterns[0][round], 16U) &&
            ai_micro_m_memblock_bytes_match(boundary_phys[1],
                init_patterns[1][round], 16U) &&
            ai_micro_m_memblock_bytes_match(guard_before_phys,
                guard_before_patterns[round], 8U) &&
            ai_micro_m_memblock_bytes_match(guard_middle_phys,
                guard_middle_patterns[round], 16U) &&
            ai_micro_m_memblock_bytes_match(guard_after_phys,
                guard_after_patterns[round], 8U)
        );
    }

    ai_micro_m_memblock_reset_env();

    TEST_END();
}

bool ai_micro_m_samepage_cross_16b_same_width_template_fault_switch_store_keeps_latest_tval_corner()
{
    TEST_START();

    static const uintptr_t store_offsets[2] = {
        AI_MICRO_MEMBLOCK_CROSS_16B_BOUNDARY0_SD_4B4B_OFFSET,
        AI_MICRO_MEMBLOCK_CROSS_16B_BOUNDARY1_SD_OFFSET,
    };
    static const uint64_t store_values[2][2] = {
        { 0x0123456789abcdefULL, 0x13579bdf2468ace0ULL },
        { 0xf0e1d2c3b4a59687ULL, 0x89abcdef01234567ULL },
    };
    static const uint8_t init_patterns[2][2][16] = {
        {
            { 0xe2, 0xd3, 0xc4, 0xb5, 0xa6, 0x97, 0x88, 0x79,
              0x6a, 0x5b, 0x4c, 0x3d, 0x2e, 0x1f, 0x10, 0x01 },
            { 0x24, 0x35, 0x46, 0x57, 0x68, 0x79, 0x8a, 0x9b,
              0xac, 0xbd, 0xce, 0xdf, 0xe0, 0xf1, 0x02, 0x13 },
        },
        {
            { 0x5c, 0x4d, 0x3e, 0x2f, 0x10, 0x01, 0xf2, 0xe3,
              0xd4, 0xc5, 0xb6, 0xa7, 0x98, 0x89, 0x7a, 0x6b },
            { 0xb3, 0xa4, 0x95, 0x86, 0x77, 0x68, 0x59, 0x4a,
              0x3b, 0x2c, 0x1d, 0x0e, 0xff, 0xe1, 0xd2, 0xc3 },
        },
    };
    static const uint8_t guard_before_patterns[2][8] = {
        { 0x16, 0x28, 0x3a, 0x4c, 0x5e, 0x60, 0x72, 0x84 },
        { 0x8e, 0x7c, 0x6a, 0x58, 0x46, 0x34, 0x22, 0x10 },
    };
    static const uint8_t guard_middle_patterns[2][16] = {
        { 0xb0, 0xc2, 0xd4, 0xe6, 0xf8, 0x0a, 0x1c, 0x2e,
          0x3f, 0x50, 0x61, 0x72, 0x83, 0x94, 0xa5, 0xb6 },
        { 0x19, 0x2b, 0x3d, 0x4f, 0x51, 0x63, 0x75, 0x87,
          0x99, 0xab, 0xbd, 0xcf, 0xd1, 0xe3, 0xf5, 0x07 },
    };
    static const uint8_t guard_after_patterns[2][8] = {
        { 0x27, 0x49, 0x6b, 0x8d, 0xaf, 0xc1, 0xd3, 0xe5 },
        { 0x4d, 0x5f, 0x71, 0x83, 0x95, 0xa7, 0xb9, 0xcb },
    };
    const enum test_page page = SWITCH1;
    const uintptr_t boundary_phys[2] = {
        ai_micro_m_memblock_cross_16b_boundary_phys(0U),
        ai_micro_m_memblock_cross_16b_boundary_phys(1U),
    };
    const uintptr_t guard_before_phys =
        ai_micro_m_memblock_cross_16b_guard_before_phys();
    const uintptr_t guard_middle_phys =
        ai_micro_m_memblock_cross_16b_guard_middle_phys();
    const uintptr_t guard_after_phys =
        ai_micro_m_memblock_cross_16b_guard_after_phys();
    uint64_t observed_value = 0ULL;

    for (unsigned round = 0; round < 2U; ++round) {
        const bool forward = (round == 0U);
        const unsigned first = forward ? 0U : 1U;
        const unsigned second = forward ? 1U : 0U;
        const uintptr_t first_vaddr =
            ai_micro_m_memblock_cross_16b_addr_with_offset(page,
                store_offsets[first]);
        const uintptr_t second_vaddr =
            ai_micro_m_memblock_cross_16b_addr_with_offset(page,
                store_offsets[second]);

        goto_priv(PRIV_M);
        ai_micro_m_memblock_reset_env();
        ai_micro_m_memblock_write_bytes(boundary_phys[0],
            init_patterns[0][round], 16U);
        ai_micro_m_memblock_write_bytes(boundary_phys[1],
            init_patterns[1][round], 16U);
        ai_micro_m_memblock_write_bytes(guard_before_phys,
            guard_before_patterns[round], 8U);
        ai_micro_m_memblock_write_bytes(guard_middle_phys,
            guard_middle_patterns[round], 16U);
        ai_micro_m_memblock_write_bytes(guard_after_phys,
            guard_after_patterns[round], 8U);
        ai_micro_m_memblock_configure_cross_16b_pmp_window(
            boundary_phys[first] + 8U, boundary_phys[first] + 16U,
            AI_MICRO_MEMBLOCK_PMP_TOR_NO_W);

        ai_micro_m_memblock_prepare_supervisor_translation();
        ai_micro_m_memblock_set_cross_16b_leaves(PTE_V | PTE_AD | PTE_RWX);
        sfence_vma();

        TEST_SETUP_EXCEPT();
        ai_micro_m_memblock_translated_sd(first_vaddr, store_values[first][round]);
        TEST_ASSERT("the first no-H same-page same-width different-template store fault reports a store access fault with tval bound to the original misaligned address",
            excpt.triggered == true &&
            excpt.cause == CAUSE_SAF &&
            excpt.tval == ai_micro_m_memblock_cross_16b_second_half_vaddr(first_vaddr)
        );

        goto_priv(PRIV_M);
        TEST_ASSERT("after the first no-H same-page same-width different-template store fault, both boundary windows and all guard regions remain unchanged before the follow-up store fault",
            ai_micro_m_memblock_bytes_match(boundary_phys[0],
                init_patterns[0][round], 16U) &&
            ai_micro_m_memblock_bytes_match(boundary_phys[1],
                init_patterns[1][round], 16U) &&
            ai_micro_m_memblock_bytes_match(guard_before_phys,
                guard_before_patterns[round], 8U) &&
            ai_micro_m_memblock_bytes_match(guard_middle_phys,
                guard_middle_patterns[round], 16U) &&
            ai_micro_m_memblock_bytes_match(guard_after_phys,
                guard_after_patterns[round], 8U)
        );

        ai_micro_m_memblock_configure_cross_16b_pmp_window(
            boundary_phys[second] + 8U, boundary_phys[second] + 16U,
            AI_MICRO_MEMBLOCK_PMP_TOR_NO_W);

        TEST_SETUP_EXCEPT();
        ai_micro_m_memblock_translated_sd(second_vaddr, store_values[second][round]);
        TEST_ASSERT("after the earlier same-page same-width different-template store fault has already occupied the shared exception path, the follow-up store fault must refresh tval to the newer template-specific address",
            excpt.triggered == true &&
            excpt.cause == CAUSE_SAF &&
            excpt.tval == ai_micro_m_memblock_cross_16b_second_half_vaddr(second_vaddr)
        );

        goto_priv(PRIV_M);
        TEST_ASSERT("the same-page same-width different-template store fault-switch sequence leaves both boundary windows and all guard regions unchanged because both faults are side-effect free",
            ai_micro_m_memblock_bytes_match(boundary_phys[0],
                init_patterns[0][round], 16U) &&
            ai_micro_m_memblock_bytes_match(boundary_phys[1],
                init_patterns[1][round], 16U) &&
            ai_micro_m_memblock_bytes_match(guard_before_phys,
                guard_before_patterns[round], 8U) &&
            ai_micro_m_memblock_bytes_match(guard_middle_phys,
                guard_middle_patterns[round], 16U) &&
            ai_micro_m_memblock_bytes_match(guard_after_phys,
                guard_after_patterns[round], 8U)
        );
    }

    ai_micro_m_memblock_reset_env();

    TEST_END();
}

bool ai_micro_m_samepage_cross_16b_cross_producer_same_width_template_fault_switch_load_then_store_keeps_latest_cause_and_tval_corner()
{
    TEST_START();

    static const uintptr_t load_offsets[2] = {
        AI_MICRO_MEMBLOCK_CROSS_16B_BOUNDARY0_SD_4B4B_OFFSET,
        AI_MICRO_MEMBLOCK_CROSS_16B_BOUNDARY1_SD_OFFSET,
    };
    static const uintptr_t store_offsets[2] = {
        AI_MICRO_MEMBLOCK_CROSS_16B_BOUNDARY1_SD_OFFSET,
        AI_MICRO_MEMBLOCK_CROSS_16B_BOUNDARY0_SD_4B4B_OFFSET,
    };
    static const uint64_t store_values[2][2] = {
        { 0x0123456789abcdefULL, 0x13579bdf2468ace0ULL },
        { 0xf0e1d2c3b4a59687ULL, 0x89abcdef01234567ULL },
    };
    static const uint8_t init_patterns[2][2][16] = {
        {
            { 0xe2, 0xd3, 0xc4, 0xb5, 0xa6, 0x97, 0x88, 0x79,
              0x6a, 0x5b, 0x4c, 0x3d, 0x2e, 0x1f, 0x10, 0x01 },
            { 0x24, 0x35, 0x46, 0x57, 0x68, 0x79, 0x8a, 0x9b,
              0xac, 0xbd, 0xce, 0xdf, 0xe0, 0xf1, 0x02, 0x13 },
        },
        {
            { 0x5c, 0x4d, 0x3e, 0x2f, 0x10, 0x01, 0xf2, 0xe3,
              0xd4, 0xc5, 0xb6, 0xa7, 0x98, 0x89, 0x7a, 0x6b },
            { 0xb3, 0xa4, 0x95, 0x86, 0x77, 0x68, 0x59, 0x4a,
              0x3b, 0x2c, 0x1d, 0x0e, 0xff, 0xe1, 0xd2, 0xc3 },
        },
    };
    static const uint8_t guard_before_patterns[2][8] = {
        { 0x16, 0x28, 0x3a, 0x4c, 0x5e, 0x60, 0x72, 0x84 },
        { 0x8e, 0x7c, 0x6a, 0x58, 0x46, 0x34, 0x22, 0x10 },
    };
    static const uint8_t guard_middle_patterns[2][16] = {
        { 0xb0, 0xc2, 0xd4, 0xe6, 0xf8, 0x0a, 0x1c, 0x2e,
          0x3f, 0x50, 0x61, 0x72, 0x83, 0x94, 0xa5, 0xb6 },
        { 0x19, 0x2b, 0x3d, 0x4f, 0x51, 0x63, 0x75, 0x87,
          0x99, 0xab, 0xbd, 0xcf, 0xd1, 0xe3, 0xf5, 0x07 },
    };
    static const uint8_t guard_after_patterns[2][8] = {
        { 0x27, 0x49, 0x6b, 0x8d, 0xaf, 0xc1, 0xd3, 0xe5 },
        { 0x4d, 0x5f, 0x71, 0x83, 0x95, 0xa7, 0xb9, 0xcb },
    };
    const enum test_page page = SWITCH1;
    const uintptr_t boundary_phys[2] = {
        ai_micro_m_memblock_cross_16b_boundary_phys(0U),
        ai_micro_m_memblock_cross_16b_boundary_phys(1U),
    };
    const uintptr_t guard_before_phys =
        ai_micro_m_memblock_cross_16b_guard_before_phys();
    const uintptr_t guard_middle_phys =
        ai_micro_m_memblock_cross_16b_guard_middle_phys();
    const uintptr_t guard_after_phys =
        ai_micro_m_memblock_cross_16b_guard_after_phys();
    uint64_t observed_value = 0ULL;

    for (unsigned round = 0; round < 2U; ++round) {
        const unsigned first = round;
        const unsigned second = first ^ 1U;
        const uintptr_t load_vaddr =
            ai_micro_m_memblock_cross_16b_addr_with_offset(page,
                load_offsets[round]);
        const uintptr_t store_vaddr =
            ai_micro_m_memblock_cross_16b_addr_with_offset(page,
                store_offsets[round]);

        goto_priv(PRIV_M);
        ai_micro_m_memblock_reset_env();
        ai_micro_m_memblock_write_bytes(boundary_phys[0],
            init_patterns[0][round], 16U);
        ai_micro_m_memblock_write_bytes(boundary_phys[1],
            init_patterns[1][round], 16U);
        ai_micro_m_memblock_write_bytes(guard_before_phys,
            guard_before_patterns[round], 8U);
        ai_micro_m_memblock_write_bytes(guard_middle_phys,
            guard_middle_patterns[round], 16U);
        ai_micro_m_memblock_write_bytes(guard_after_phys,
            guard_after_patterns[round], 8U);
        ai_micro_m_memblock_configure_cross_16b_pmp_window(
            boundary_phys[first] + 8U, boundary_phys[first] + 16U,
            AI_MICRO_MEMBLOCK_PMP_TOR_NO_R);

        ai_micro_m_memblock_prepare_supervisor_translation();
        ai_micro_m_memblock_set_cross_16b_leaves(PTE_V | PTE_AD | PTE_RWX);
        sfence_vma();

        TEST_SETUP_EXCEPT();
        observed_value = ai_micro_m_memblock_translated_scalar_load(
            false, sizeof(uint64_t), load_vaddr);
        TEST_ASSERT("the first no-H same-page cross-16B cross-producer same-width different-template load fault reports a load access fault with tval bound to the original misaligned address",
            excpt.triggered == true &&
            excpt.cause == CAUSE_LAF &&
            excpt.tval == ai_micro_m_memblock_cross_16b_second_half_vaddr(load_vaddr)
        );

        goto_priv(PRIV_M);
        TEST_ASSERT("after the first no-H same-page cross-16B cross-producer same-width different-template load fault, both boundary windows and all guard regions remain unchanged before the follow-up store fault",
            ai_micro_m_memblock_bytes_match(boundary_phys[0],
                init_patterns[0][round], 16U) &&
            ai_micro_m_memblock_bytes_match(boundary_phys[1],
                init_patterns[1][round], 16U) &&
            ai_micro_m_memblock_bytes_match(guard_before_phys,
                guard_before_patterns[round], 8U) &&
            ai_micro_m_memblock_bytes_match(guard_middle_phys,
                guard_middle_patterns[round], 16U) &&
            ai_micro_m_memblock_bytes_match(guard_after_phys,
                guard_after_patterns[round], 8U)
        );

        ai_micro_m_memblock_configure_cross_16b_pmp_window(
            boundary_phys[second] + 8U, boundary_phys[second] + 16U,
            AI_MICRO_MEMBLOCK_PMP_TOR_NO_W);

        TEST_SETUP_EXCEPT();
        ai_micro_m_memblock_translated_sd(store_vaddr, store_values[second][round]);
        TEST_ASSERT("after the earlier same-page cross-16B cross-producer same-width different-template load fault has already occupied the shared exception path, the follow-up store fault must refresh cause and tval to the newer template-specific address",
            excpt.triggered == true &&
            excpt.cause == CAUSE_SAF &&
            excpt.tval == ai_micro_m_memblock_cross_16b_second_half_vaddr(store_vaddr)
        );

        goto_priv(PRIV_M);
        TEST_ASSERT("the same-page cross-16B cross-producer same-width different-template load->store fault-switch sequence leaves both boundary windows and all guard regions unchanged because both faults are side-effect free",
            ai_micro_m_memblock_bytes_match(boundary_phys[0],
                init_patterns[0][round], 16U) &&
            ai_micro_m_memblock_bytes_match(boundary_phys[1],
                init_patterns[1][round], 16U) &&
            ai_micro_m_memblock_bytes_match(guard_before_phys,
                guard_before_patterns[round], 8U) &&
            ai_micro_m_memblock_bytes_match(guard_middle_phys,
                guard_middle_patterns[round], 16U) &&
            ai_micro_m_memblock_bytes_match(guard_after_phys,
                guard_after_patterns[round], 8U)
        );
    }

    (void)observed_value;
    ai_micro_m_memblock_reset_env();

    TEST_END();
}
