#include "ai_micro_mmode_memblock_retry_followup_byte4_exact_store_common.h"

bool ai_micro_m_samepage_cross16b_store_refault_upperx4_retry_4b4b_store_then_exact_upper_tail_byte_store_corner()
{
    TEST_START();

    static const uint8_t boundary_seed[16] = {
        0x68, 0x57, 0x46, 0x35, 0x24, 0x13, 0x02, 0xf1,
        0xe0, 0xdf, 0xce, 0xbd, 0xac, 0x9b, 0x8a, 0x79,
    };
    static const uint8_t guard_before_seed[8] = {
        0x1a, 0x2c, 0x3e, 0x40, 0x52, 0x64, 0x76, 0x88,
    };
    static const uint8_t guard_after_seed[8] = {
        0xcb, 0xdd, 0xef, 0x01, 0x23, 0x35, 0x47, 0x59,
    };
    const enum test_page low_page = SWITCH1;
    const enum test_page high_page = SWITCH2;
    const uintptr_t page_phys = phys_page_base(low_page) + PAGE_SIZE - 16U;
    const uintptr_t page_vaddr = hs_page_base(low_page) + PAGE_SIZE - 16U;
    const uintptr_t boundary_phys = page_phys +
        AI_MICRO_MAB_RETRY_BYTE4_EXACT_STORE_BOUNDARY_BASE_OFFSET;
    const uintptr_t guard_before_phys = page_phys;
    const uintptr_t guard_after_phys = page_phys + 24U;
    const uintptr_t fault_vaddr = page_vaddr +
        AI_MICRO_MAB_RETRY_BYTE4_EXACT_STORE_CROSS_16B_SD_OFFSET;
    const uintptr_t second_half_vaddr = page_vaddr + 16U;
    const uintptr_t aligned_upper_store_vaddr = page_vaddr +
        AI_MICRO_MAB_RETRY_BYTE4_EXACT_STORE_BOUNDARY_BASE_OFFSET + 8U;
    const uintptr_t followup_exact_upper_tail_byte_store_vaddr =
        aligned_upper_store_vaddr + 3U;
    const uintptr_t deny_start = boundary_phys + 8U;
    const uintptr_t deny_end = deny_start + PAGE_SIZE;
    const uint64_t first_fault_store_value = 0x748596a7b8c9daebULL;
    const uint64_t refault_store_value = 0xf405162738495a6bULL;
    const uint64_t aligned_upper_store_value_first = 0x31425364758697a8ULL;
    const uint64_t aligned_upper_store_value_second = 0xb9cadbecfd0e1f20ULL;
    const uint64_t aligned_upper_store_value_third = 0x425364758697a8b9ULL;
    const uint64_t aligned_upper_store_value_fourth = 0xcadbecfd0e1f2031ULL;
    const uint64_t retry_4b4b_store_value = 0x5a6b7c8d9eafc0d1ULL;
    const uint8_t followup_exact_upper_tail_byte_store_value = 0x3eU;
    uint8_t expected_boundary[16];

    goto_priv(PRIV_M);
    ai_micro_mab_retry_byte4_exact_store_reset_env();
    ai_micro_mab_retry_byte4_exact_store_write_bytes(
        boundary_phys, boundary_seed, 16U);
    ai_micro_mab_retry_byte4_exact_store_write_bytes(
        guard_before_phys, guard_before_seed, 8U);
    ai_micro_mab_retry_byte4_exact_store_write_bytes(
        guard_after_phys, guard_after_seed, 8U);

    ai_micro_mab_retry_byte4_exact_store_configure_pmp_window(
        deny_start, deny_end,
        AI_MICRO_MAB_RETRY_BYTE4_EXACT_STORE_PMP_TOR_NO_W);
    ai_micro_mab_retry_byte4_exact_store_prepare_supervisor_translation();
    ai_micro_mab_retry_byte4_exact_store_set_leaf(
        low_page, PTE_V | PTE_AD | PTE_RWX);
    ai_micro_mab_retry_byte4_exact_store_set_leaf(
        high_page, PTE_V | PTE_AD | PTE_RWX);
    sfence_vma();

    TEST_SETUP_EXCEPT();
    ai_micro_mab_retry_byte4_exact_store_translated_sd(
        fault_vaddr, first_fault_store_value);
    TEST_ASSERT("for the no-H same-page cross-16B translated scalar store, the first repeated 4B+4B second-split write fault must still report store access fault on the true second-half start address before the retried-success then exact-upper-tail-byte follow-up store probe",
        excpt.triggered == true &&
        excpt.cause == CAUSE_SAF &&
        excpt.tval == second_half_vaddr
    );

    TEST_SETUP_EXCEPT();
    ai_micro_mab_retry_byte4_exact_store_translated_sd(
        fault_vaddr, first_fault_store_value);
    TEST_ASSERT("repeating the same no-H second-split 4B+4B write fault keeps the same precise metadata before the retried-success then exact-upper-tail-byte follow-up store probe",
        excpt.triggered == true &&
        excpt.cause == CAUSE_SAF &&
        excpt.tval == second_half_vaddr
    );

    ai_micro_mab_retry_byte4_exact_store_restore_pmp_full_access();

    TEST_SETUP_EXCEPT();
    ai_micro_mab_retry_byte4_exact_store_translated_sd(
        aligned_upper_store_vaddr, aligned_upper_store_value_first);
    TEST_ASSERT("after repair, the first same-boundary translated upper-half aligned store should complete cleanly before the refaulted retry-4B+4B-store then exact-upper-tail-byte follow-up store probe",
        excpt.triggered == false
    );

    TEST_SETUP_EXCEPT();
    ai_micro_mab_retry_byte4_exact_store_translated_sd(
        aligned_upper_store_vaddr, aligned_upper_store_value_second);
    TEST_ASSERT("after one upper-half aligned translated store has already completed on the repaired boundary, the second upper-half aligned translated store should stay trap-free before the refaulted retry-4B+4B-store then exact-upper-tail-byte follow-up store probe",
        excpt.triggered == false
    );

    TEST_SETUP_EXCEPT();
    ai_micro_mab_retry_byte4_exact_store_translated_sd(
        aligned_upper_store_vaddr, aligned_upper_store_value_third);
    TEST_ASSERT("after two upper-half aligned translated stores have already completed on the repaired boundary, the third upper-half aligned translated store should stay trap-free before the refaulted retry-4B+4B-store then exact-upper-tail-byte follow-up store probe",
        excpt.triggered == false
    );

    TEST_SETUP_EXCEPT();
    ai_micro_mab_retry_byte4_exact_store_translated_sd(
        aligned_upper_store_vaddr, aligned_upper_store_value_fourth);
    TEST_ASSERT("after three upper-half aligned translated stores have already completed on the repaired boundary, the fourth upper-half aligned translated store should stay trap-free and become the latest committed high-half payload before the refaulted retry-4B+4B-store then exact-upper-tail-byte follow-up store probe",
        excpt.triggered == false
    );

    ai_micro_mab_retry_byte4_exact_store_configure_pmp_window(
        deny_start, deny_end,
        AI_MICRO_MAB_RETRY_BYTE4_EXACT_STORE_PMP_TOR_NO_W);

    TEST_SETUP_EXCEPT();
    ai_micro_mab_retry_byte4_exact_store_translated_sd(
        fault_vaddr, refault_store_value);
    TEST_ASSERT("after four upper-half aligned translated stores have already completed on the repaired boundary, restoring the upper-half deny window and reissuing the original 4B+4B misaligned store should refresh the same precise second-split store access fault metadata before the exact-upper-tail-byte follow-up store probe",
        excpt.triggered == true &&
        excpt.cause == CAUSE_SAF &&
        excpt.tval == second_half_vaddr
    );

    ai_micro_mab_retry_byte4_exact_store_restore_pmp_full_access();

    TEST_SETUP_EXCEPT();
    ai_micro_mab_retry_byte4_exact_store_translated_sd(
        fault_vaddr, retry_4b4b_store_value);
    TEST_ASSERT("after the refault episode is repaired again, retrying the same 4B+4B translated store as a success store should clear stale exception metadata before the immediate exact-upper-tail-byte follow-up store",
        excpt.triggered == false
    );

    TEST_SETUP_EXCEPT();
    ai_micro_mab_retry_byte4_exact_store_translated_sb(
        followup_exact_upper_tail_byte_store_vaddr,
        followup_exact_upper_tail_byte_store_value);
    TEST_ASSERT("after the retried original-template 4B+4B store has already succeeded on the same boundary, the immediate translated exact-upper-tail-byte store should also stay trap-free and must not reuse the retired fake-crosspage exact 4-byte high-half word window beyond byte11",
        excpt.triggered == false
    );

    ai_micro_mab_retry_byte4_exact_store_overlay_little_endian_bytes(
        expected_boundary, boundary_seed, 16U, 8U,
        aligned_upper_store_value_fourth, sizeof(uint64_t));
    ai_micro_mab_retry_byte4_exact_store_overlay_little_endian_bytes(
        expected_boundary, expected_boundary, 16U, 4U,
        retry_4b4b_store_value, sizeof(uint64_t));
    ai_micro_mab_retry_byte4_exact_store_overlay_little_endian_bytes(
        expected_boundary, expected_boundary, 16U, 11U,
        (uint64_t)followup_exact_upper_tail_byte_store_value,
        sizeof(uint8_t));

    TEST_ASSERT("after the prolonged upper-half producer, repaired refault, retried 4B+4B success store, and immediate exact-upper-tail-byte store, the final boundary bytes should preserve bytes4-10 from the retried image, refresh only byte11, and keep bytes12-15 from the latest aligned upper-half image",
        ai_micro_mab_retry_byte4_exact_store_bytes_match(
            boundary_phys, expected_boundary, 16U)
    );

    TEST_ASSERT("guard regions around the same-page boundary should remain unchanged through the repeated 4B+4B fault, four upper-half aligned stores, refault, retried success store, and immediate exact-upper-tail-byte follow-up store sequence",
        ai_micro_mab_retry_byte4_exact_store_bytes_match(
            guard_before_phys, guard_before_seed, 8U) &&
        ai_micro_mab_retry_byte4_exact_store_bytes_match(
            guard_after_phys, guard_after_seed, 8U)
    );

    ai_micro_mab_retry_byte4_exact_store_reset_env();

    TEST_END(
        "ai_micro_m_samepage_cross16b_store_refault_upperx4_retry_4b4b_store_then_exact_upper_tail_byte_store_corner");
}

bool ai_micro_m_samepage_cross16b_store_refault_upperx4_retry_4b4b_store_then_exact_upper_tail_halfword_store_corner()
{
    TEST_START();

    static const uint8_t boundary_seed[16] = {
        0x79, 0x68, 0x57, 0x46, 0x35, 0x24, 0x13, 0x02,
        0xf1, 0xe0, 0xdf, 0xce, 0xbd, 0xac, 0x9b, 0x8a,
    };
    static const uint8_t guard_before_seed[8] = {
        0x1d, 0x2f, 0x31, 0x43, 0x55, 0x67, 0x79, 0x8b,
    };
    static const uint8_t guard_after_seed[8] = {
        0xc4, 0xd6, 0xe8, 0xfa, 0x0c, 0x1e, 0x20, 0x32,
    };
    const enum test_page low_page = SWITCH1;
    const enum test_page high_page = SWITCH2;
    const uintptr_t page_phys = phys_page_base(low_page) + PAGE_SIZE - 16U;
    const uintptr_t page_vaddr = hs_page_base(low_page) + PAGE_SIZE - 16U;
    const uintptr_t boundary_phys = page_phys +
        AI_MICRO_MAB_RETRY_BYTE4_EXACT_STORE_BOUNDARY_BASE_OFFSET;
    const uintptr_t guard_before_phys = page_phys;
    const uintptr_t guard_after_phys = page_phys + 24U;
    const uintptr_t fault_vaddr = page_vaddr +
        AI_MICRO_MAB_RETRY_BYTE4_EXACT_STORE_CROSS_16B_SD_OFFSET;
    const uintptr_t second_half_vaddr = page_vaddr + 16U;
    const uintptr_t aligned_upper_store_vaddr = page_vaddr +
        AI_MICRO_MAB_RETRY_BYTE4_EXACT_STORE_BOUNDARY_BASE_OFFSET + 8U;
    const uintptr_t followup_exact_upper_tail_halfword_store_vaddr =
        aligned_upper_store_vaddr + 2U;
    const uintptr_t deny_start = boundary_phys + 8U;
    const uintptr_t deny_end = deny_start + PAGE_SIZE;
    const uint64_t first_fault_store_value = 0x8596a7b8c9daebfcULL;
    const uint64_t refault_store_value = 0x05162738495a6b7cULL;
    const uint64_t aligned_upper_store_value_first = 0x425364758697a8b9ULL;
    const uint64_t aligned_upper_store_value_second = 0xcadbecfd0e1f2031ULL;
    const uint64_t aligned_upper_store_value_third = 0x5364758697a8b9caULL;
    const uint64_t aligned_upper_store_value_fourth = 0xdbecfd0e1f203142ULL;
    const uint64_t retry_4b4b_store_value = 0x6b7c8d9eafc0d1e2ULL;
    const uint16_t followup_exact_upper_tail_halfword_store_value = 0x4f3eU;
    uint8_t expected_boundary[16];

    goto_priv(PRIV_M);
    ai_micro_mab_retry_byte4_exact_store_reset_env();
    ai_micro_mab_retry_byte4_exact_store_write_bytes(
        boundary_phys, boundary_seed, 16U);
    ai_micro_mab_retry_byte4_exact_store_write_bytes(
        guard_before_phys, guard_before_seed, 8U);
    ai_micro_mab_retry_byte4_exact_store_write_bytes(
        guard_after_phys, guard_after_seed, 8U);

    ai_micro_mab_retry_byte4_exact_store_configure_pmp_window(
        deny_start, deny_end,
        AI_MICRO_MAB_RETRY_BYTE4_EXACT_STORE_PMP_TOR_NO_W);
    ai_micro_mab_retry_byte4_exact_store_prepare_supervisor_translation();
    ai_micro_mab_retry_byte4_exact_store_set_leaf(
        low_page, PTE_V | PTE_AD | PTE_RWX);
    ai_micro_mab_retry_byte4_exact_store_set_leaf(
        high_page, PTE_V | PTE_AD | PTE_RWX);
    sfence_vma();

    TEST_SETUP_EXCEPT();
    ai_micro_mab_retry_byte4_exact_store_translated_sd(
        fault_vaddr, first_fault_store_value);
    TEST_ASSERT("for the no-H same-page cross-16B translated scalar store, the first repeated 4B+4B second-split write fault must still report store access fault on the true second-half start address before the retried-success then exact-upper-tail-halfword follow-up store probe",
        excpt.triggered == true &&
        excpt.cause == CAUSE_SAF &&
        excpt.tval == second_half_vaddr
    );

    TEST_SETUP_EXCEPT();
    ai_micro_mab_retry_byte4_exact_store_translated_sd(
        fault_vaddr, first_fault_store_value);
    TEST_ASSERT("repeating the same no-H second-split 4B+4B write fault keeps the same precise metadata before the retried-success then exact-upper-tail-halfword follow-up store probe",
        excpt.triggered == true &&
        excpt.cause == CAUSE_SAF &&
        excpt.tval == second_half_vaddr
    );

    ai_micro_mab_retry_byte4_exact_store_restore_pmp_full_access();

    TEST_SETUP_EXCEPT();
    ai_micro_mab_retry_byte4_exact_store_translated_sd(
        aligned_upper_store_vaddr, aligned_upper_store_value_first);
    TEST_ASSERT("after repair, the first same-boundary translated upper-half aligned store should complete cleanly before the refaulted retry-4B+4B-store then exact-upper-tail-halfword follow-up store probe",
        excpt.triggered == false
    );

    TEST_SETUP_EXCEPT();
    ai_micro_mab_retry_byte4_exact_store_translated_sd(
        aligned_upper_store_vaddr, aligned_upper_store_value_second);
    TEST_ASSERT("after one upper-half aligned translated store has already completed on the repaired boundary, the second upper-half aligned translated store should stay trap-free before the refaulted retry-4B+4B-store then exact-upper-tail-halfword follow-up store probe",
        excpt.triggered == false
    );

    TEST_SETUP_EXCEPT();
    ai_micro_mab_retry_byte4_exact_store_translated_sd(
        aligned_upper_store_vaddr, aligned_upper_store_value_third);
    TEST_ASSERT("after two upper-half aligned translated stores have already completed on the repaired boundary, the third upper-half aligned translated store should stay trap-free before the refaulted retry-4B+4B-store then exact-upper-tail-halfword follow-up store probe",
        excpt.triggered == false
    );

    TEST_SETUP_EXCEPT();
    ai_micro_mab_retry_byte4_exact_store_translated_sd(
        aligned_upper_store_vaddr, aligned_upper_store_value_fourth);
    TEST_ASSERT("after three upper-half aligned translated stores have already completed on the repaired boundary, the fourth upper-half aligned translated store should stay trap-free and become the latest committed high-half payload before the refaulted retry-4B+4B-store then exact-upper-tail-halfword follow-up store probe",
        excpt.triggered == false
    );

    ai_micro_mab_retry_byte4_exact_store_configure_pmp_window(
        deny_start, deny_end,
        AI_MICRO_MAB_RETRY_BYTE4_EXACT_STORE_PMP_TOR_NO_W);

    TEST_SETUP_EXCEPT();
    ai_micro_mab_retry_byte4_exact_store_translated_sd(
        fault_vaddr, refault_store_value);
    TEST_ASSERT("after four upper-half aligned translated stores have already completed on the repaired boundary, restoring the upper-half deny window and reissuing the original 4B+4B misaligned store should refresh the same precise second-split store access fault metadata before the exact-upper-tail-halfword follow-up store probe",
        excpt.triggered == true &&
        excpt.cause == CAUSE_SAF &&
        excpt.tval == second_half_vaddr
    );

    ai_micro_mab_retry_byte4_exact_store_restore_pmp_full_access();

    TEST_SETUP_EXCEPT();
    ai_micro_mab_retry_byte4_exact_store_translated_sd(
        fault_vaddr, retry_4b4b_store_value);
    TEST_ASSERT("after the refault episode is repaired again, retrying the same 4B+4B translated store as a success store should clear stale exception metadata before the immediate exact-upper-tail-halfword follow-up store",
        excpt.triggered == false
    );

    TEST_SETUP_EXCEPT();
    ai_micro_mab_retry_byte4_exact_store_translated_sh(
        followup_exact_upper_tail_halfword_store_vaddr,
        followup_exact_upper_tail_halfword_store_value);
    TEST_ASSERT("after the retried original-template 4B+4B store has already succeeded on the same boundary, the immediate translated exact-upper-tail-halfword store should also stay trap-free and must not reuse the retired fake-crosspage exact 4-byte high-half word window beyond bytes10-11",
        excpt.triggered == false
    );

    ai_micro_mab_retry_byte4_exact_store_overlay_little_endian_bytes(
        expected_boundary, boundary_seed, 16U, 8U,
        aligned_upper_store_value_fourth, sizeof(uint64_t));
    ai_micro_mab_retry_byte4_exact_store_overlay_little_endian_bytes(
        expected_boundary, expected_boundary, 16U, 4U,
        retry_4b4b_store_value, sizeof(uint64_t));
    ai_micro_mab_retry_byte4_exact_store_overlay_little_endian_bytes(
        expected_boundary, expected_boundary, 16U, 10U,
        (uint64_t)followup_exact_upper_tail_halfword_store_value,
        sizeof(uint16_t));

    TEST_ASSERT("after the prolonged upper-half producer, repaired refault, retried 4B+4B success store, and immediate exact-upper-tail-halfword store, the final boundary bytes should preserve bytes4-9 from the retried image, refresh only bytes10-11, and keep bytes12-15 from the latest aligned upper-half image",
        ai_micro_mab_retry_byte4_exact_store_bytes_match(
            boundary_phys, expected_boundary, 16U)
    );

    TEST_ASSERT("guard regions around the same-page boundary should remain unchanged through the repeated 4B+4B fault, four upper-half aligned stores, refault, retried success store, and immediate exact-upper-tail-halfword follow-up store sequence",
        ai_micro_mab_retry_byte4_exact_store_bytes_match(
            guard_before_phys, guard_before_seed, 8U) &&
        ai_micro_mab_retry_byte4_exact_store_bytes_match(
            guard_after_phys, guard_after_seed, 8U)
    );

    ai_micro_mab_retry_byte4_exact_store_reset_env();

    TEST_END(
        "ai_micro_m_samepage_cross16b_store_refault_upperx4_retry_4b4b_store_then_exact_upper_tail_halfword_store_corner");
}

bool ai_micro_m_samepage_cross16b_store_refault_upperx4_retry_4b4b_store_then_first_upper_halfword_store_corner()
{
    TEST_START();

    static const uint8_t boundary_seed[16] = {
        0x8a, 0x79, 0x68, 0x57, 0x46, 0x35, 0x24, 0x13,
        0x02, 0xf1, 0xe0, 0xdf, 0xce, 0xbd, 0xac, 0x9b,
    };
    static const uint8_t guard_before_seed[8] = {
        0x20, 0x32, 0x44, 0x56, 0x68, 0x7a, 0x8c, 0x9e,
    };
    static const uint8_t guard_after_seed[8] = {
        0xc1, 0xd3, 0xe5, 0xf7, 0x09, 0x1b, 0x2d, 0x3f,
    };
    const enum test_page low_page = SWITCH1;
    const enum test_page high_page = SWITCH2;
    const uintptr_t page_phys = phys_page_base(low_page) + PAGE_SIZE - 16U;
    const uintptr_t page_vaddr = hs_page_base(low_page) + PAGE_SIZE - 16U;
    const uintptr_t boundary_phys = page_phys +
        AI_MICRO_MAB_RETRY_BYTE4_EXACT_STORE_BOUNDARY_BASE_OFFSET;
    const uintptr_t guard_before_phys = page_phys;
    const uintptr_t guard_after_phys = page_phys + 24U;
    const uintptr_t fault_vaddr = page_vaddr +
        AI_MICRO_MAB_RETRY_BYTE4_EXACT_STORE_CROSS_16B_SD_OFFSET;
    const uintptr_t second_half_vaddr = page_vaddr + 16U;
    const uintptr_t aligned_upper_store_vaddr = page_vaddr +
        AI_MICRO_MAB_RETRY_BYTE4_EXACT_STORE_BOUNDARY_BASE_OFFSET + 8U;
    const uintptr_t followup_first_upper_halfword_store_vaddr =
        aligned_upper_store_vaddr;
    const uintptr_t deny_start = boundary_phys + 8U;
    const uintptr_t deny_end = deny_start + PAGE_SIZE;
    const uint64_t first_fault_store_value = 0x96a7b8c9daebfc0dULL;
    const uint64_t refault_store_value = 0x162738495a6b7c8dULL;
    const uint64_t aligned_upper_store_value_first = 0x5364758697a8b9caULL;
    const uint64_t aligned_upper_store_value_second = 0xdbecfd0e1f203142ULL;
    const uint64_t aligned_upper_store_value_third = 0x64758697a8b9cadbULL;
    const uint64_t aligned_upper_store_value_fourth = 0xecfd0e1f20314253ULL;
    const uint64_t retry_4b4b_store_value = 0x7c8d9eafc0d1e2f3ULL;
    const uint16_t followup_first_upper_halfword_store_value = 0x5b4aU;
    uint8_t expected_boundary[16];

    goto_priv(PRIV_M);
    ai_micro_mab_retry_byte4_exact_store_reset_env();
    ai_micro_mab_retry_byte4_exact_store_write_bytes(
        boundary_phys, boundary_seed, 16U);
    ai_micro_mab_retry_byte4_exact_store_write_bytes(
        guard_before_phys, guard_before_seed, 8U);
    ai_micro_mab_retry_byte4_exact_store_write_bytes(
        guard_after_phys, guard_after_seed, 8U);

    ai_micro_mab_retry_byte4_exact_store_configure_pmp_window(
        deny_start, deny_end,
        AI_MICRO_MAB_RETRY_BYTE4_EXACT_STORE_PMP_TOR_NO_W);
    ai_micro_mab_retry_byte4_exact_store_prepare_supervisor_translation();
    ai_micro_mab_retry_byte4_exact_store_set_leaf(
        low_page, PTE_V | PTE_AD | PTE_RWX);
    ai_micro_mab_retry_byte4_exact_store_set_leaf(
        high_page, PTE_V | PTE_AD | PTE_RWX);
    sfence_vma();

    TEST_SETUP_EXCEPT();
    ai_micro_mab_retry_byte4_exact_store_translated_sd(
        fault_vaddr, first_fault_store_value);
    TEST_ASSERT("for the no-H same-page cross-16B translated scalar store, the first repeated 4B+4B second-split write fault must still report store access fault on the true second-half start address before the retried-success then first-upper-halfword follow-up store probe",
        excpt.triggered == true &&
        excpt.cause == CAUSE_SAF &&
        excpt.tval == second_half_vaddr
    );

    TEST_SETUP_EXCEPT();
    ai_micro_mab_retry_byte4_exact_store_translated_sd(
        fault_vaddr, first_fault_store_value);
    TEST_ASSERT("repeating the same no-H second-split 4B+4B write fault keeps the same precise metadata before the retried-success then first-upper-halfword follow-up store probe",
        excpt.triggered == true &&
        excpt.cause == CAUSE_SAF &&
        excpt.tval == second_half_vaddr
    );

    ai_micro_mab_retry_byte4_exact_store_restore_pmp_full_access();

    TEST_SETUP_EXCEPT();
    ai_micro_mab_retry_byte4_exact_store_translated_sd(
        aligned_upper_store_vaddr, aligned_upper_store_value_first);
    TEST_ASSERT("after repair, the first same-boundary translated upper-half aligned store should complete cleanly before the refaulted retry-4B+4B-store then first-upper-halfword follow-up store probe",
        excpt.triggered == false
    );

    TEST_SETUP_EXCEPT();
    ai_micro_mab_retry_byte4_exact_store_translated_sd(
        aligned_upper_store_vaddr, aligned_upper_store_value_second);
    TEST_ASSERT("after one upper-half aligned translated store has already completed on the repaired boundary, the second upper-half aligned translated store should stay trap-free before the refaulted retry-4B+4B-store then first-upper-halfword follow-up store probe",
        excpt.triggered == false
    );

    TEST_SETUP_EXCEPT();
    ai_micro_mab_retry_byte4_exact_store_translated_sd(
        aligned_upper_store_vaddr, aligned_upper_store_value_third);
    TEST_ASSERT("after two upper-half aligned translated stores have already completed on the repaired boundary, the third upper-half aligned translated store should stay trap-free before the refaulted retry-4B+4B-store then first-upper-halfword follow-up store probe",
        excpt.triggered == false
    );

    TEST_SETUP_EXCEPT();
    ai_micro_mab_retry_byte4_exact_store_translated_sd(
        aligned_upper_store_vaddr, aligned_upper_store_value_fourth);
    TEST_ASSERT("after three upper-half aligned translated stores have already completed on the repaired boundary, the fourth upper-half aligned translated store should stay trap-free and become the latest committed high-half payload before the refaulted retry-4B+4B-store then first-upper-halfword follow-up store probe",
        excpt.triggered == false
    );

    ai_micro_mab_retry_byte4_exact_store_configure_pmp_window(
        deny_start, deny_end,
        AI_MICRO_MAB_RETRY_BYTE4_EXACT_STORE_PMP_TOR_NO_W);

    TEST_SETUP_EXCEPT();
    ai_micro_mab_retry_byte4_exact_store_translated_sd(
        fault_vaddr, refault_store_value);
    TEST_ASSERT("after four upper-half aligned translated stores have already completed on the repaired boundary, restoring the upper-half deny window and reissuing the original 4B+4B misaligned store should refresh the same precise second-split store access fault metadata before the first-upper-halfword follow-up store probe",
        excpt.triggered == true &&
        excpt.cause == CAUSE_SAF &&
        excpt.tval == second_half_vaddr
    );

    ai_micro_mab_retry_byte4_exact_store_restore_pmp_full_access();

    TEST_SETUP_EXCEPT();
    ai_micro_mab_retry_byte4_exact_store_translated_sd(
        fault_vaddr, retry_4b4b_store_value);
    TEST_ASSERT("after the refault episode is repaired again, retrying the same 4B+4B translated store as a success store should clear stale exception metadata before the immediate first-upper-halfword follow-up store",
        excpt.triggered == false
    );

    TEST_SETUP_EXCEPT();
    ai_micro_mab_retry_byte4_exact_store_translated_sh(
        followup_first_upper_halfword_store_vaddr,
        followup_first_upper_halfword_store_value);
    TEST_ASSERT("after the retried original-template 4B+4B store has already succeeded on the same boundary, the immediate translated first-upper-halfword store should also stay trap-free and must not reuse the retired fake-crosspage exact 4-byte high-half word window beyond bytes8-9",
        excpt.triggered == false
    );

    ai_micro_mab_retry_byte4_exact_store_overlay_little_endian_bytes(
        expected_boundary, boundary_seed, 16U, 8U,
        aligned_upper_store_value_fourth, sizeof(uint64_t));
    ai_micro_mab_retry_byte4_exact_store_overlay_little_endian_bytes(
        expected_boundary, expected_boundary, 16U, 4U,
        retry_4b4b_store_value, sizeof(uint64_t));
    ai_micro_mab_retry_byte4_exact_store_overlay_little_endian_bytes(
        expected_boundary, expected_boundary, 16U, 8U,
        (uint64_t)followup_first_upper_halfword_store_value,
        sizeof(uint16_t));

    TEST_ASSERT("after the prolonged upper-half producer, repaired refault, retried 4B+4B success store, and immediate first-upper-halfword store, the final boundary bytes should preserve bytes4-7 from the retried image, refresh only bytes8-9, preserve bytes10-11 from the retried image, and keep bytes12-15 from the latest aligned upper-half image",
        ai_micro_mab_retry_byte4_exact_store_bytes_match(
            boundary_phys, expected_boundary, 16U)
    );

    TEST_ASSERT("guard regions around the same-page boundary should remain unchanged through the repeated 4B+4B fault, four upper-half aligned stores, refault, retried success store, and immediate first-upper-halfword follow-up store sequence",
        ai_micro_mab_retry_byte4_exact_store_bytes_match(
            guard_before_phys, guard_before_seed, 8U) &&
        ai_micro_mab_retry_byte4_exact_store_bytes_match(
            guard_after_phys, guard_after_seed, 8U)
    );

    ai_micro_mab_retry_byte4_exact_store_reset_env();

    TEST_END(
        "ai_micro_m_samepage_cross16b_store_refault_upperx4_retry_4b4b_store_then_first_upper_halfword_store_corner");
}

bool ai_micro_m_samepage_cross16b_store_refault_upperx4_retry_4b4b_store_then_middle_bridge_halfword_store_corner()
{
    TEST_START();

    static const uint8_t boundary_seed[16] = {
        0x9b, 0x8a, 0x79, 0x68, 0x57, 0x46, 0x35, 0x24,
        0x13, 0x02, 0xf1, 0xe0, 0xdf, 0xce, 0xbd, 0xac,
    };
    static const uint8_t guard_before_seed[8] = {
        0x23, 0x35, 0x47, 0x59, 0x6b, 0x7d, 0x8f, 0x91,
    };
    static const uint8_t guard_after_seed[8] = {
        0xbe, 0xd0, 0xe2, 0xf4, 0x06, 0x18, 0x2a, 0x3c,
    };
    const enum test_page low_page = SWITCH1;
    const enum test_page high_page = SWITCH2;
    const uintptr_t page_phys = phys_page_base(low_page) + PAGE_SIZE - 16U;
    const uintptr_t page_vaddr = hs_page_base(low_page) + PAGE_SIZE - 16U;
    const uintptr_t boundary_phys = page_phys +
        AI_MICRO_MAB_RETRY_BYTE4_EXACT_STORE_BOUNDARY_BASE_OFFSET;
    const uintptr_t guard_before_phys = page_phys;
    const uintptr_t guard_after_phys = page_phys + 24U;
    const uintptr_t fault_vaddr = page_vaddr +
        AI_MICRO_MAB_RETRY_BYTE4_EXACT_STORE_CROSS_16B_SD_OFFSET;
    const uintptr_t second_half_vaddr = page_vaddr + 16U;
    const uintptr_t aligned_upper_store_vaddr = page_vaddr +
        AI_MICRO_MAB_RETRY_BYTE4_EXACT_STORE_BOUNDARY_BASE_OFFSET + 8U;
    const uintptr_t followup_middle_bridge_halfword_store_vaddr =
        aligned_upper_store_vaddr + 1U;
    const uintptr_t deny_start = boundary_phys + 8U;
    const uintptr_t deny_end = deny_start + PAGE_SIZE;
    const uint64_t first_fault_store_value = 0xa7b8c9daebfc0d1eULL;
    const uint64_t refault_store_value = 0x2738495a6b7c8d9eULL;
    const uint64_t aligned_upper_store_value_first = 0x64758697a8b9cadbULL;
    const uint64_t aligned_upper_store_value_second = 0xecfd0e1f20314253ULL;
    const uint64_t aligned_upper_store_value_third = 0x758697a8b9cadbecULL;
    const uint64_t aligned_upper_store_value_fourth = 0xfd0e1f2031425364ULL;
    const uint64_t retry_4b4b_store_value = 0x8d9eafc0d1e2f304ULL;
    const uint16_t followup_middle_bridge_halfword_store_value = 0x6c5bU;
    uint8_t expected_boundary[16];

    goto_priv(PRIV_M);
    ai_micro_mab_retry_byte4_exact_store_reset_env();
    ai_micro_mab_retry_byte4_exact_store_write_bytes(
        boundary_phys, boundary_seed, 16U);
    ai_micro_mab_retry_byte4_exact_store_write_bytes(
        guard_before_phys, guard_before_seed, 8U);
    ai_micro_mab_retry_byte4_exact_store_write_bytes(
        guard_after_phys, guard_after_seed, 8U);

    ai_micro_mab_retry_byte4_exact_store_configure_pmp_window(
        deny_start, deny_end,
        AI_MICRO_MAB_RETRY_BYTE4_EXACT_STORE_PMP_TOR_NO_W);
    ai_micro_mab_retry_byte4_exact_store_prepare_supervisor_translation();
    ai_micro_mab_retry_byte4_exact_store_set_leaf(
        low_page, PTE_V | PTE_AD | PTE_RWX);
    ai_micro_mab_retry_byte4_exact_store_set_leaf(
        high_page, PTE_V | PTE_AD | PTE_RWX);
    sfence_vma();

    TEST_SETUP_EXCEPT();
    ai_micro_mab_retry_byte4_exact_store_translated_sd(
        fault_vaddr, first_fault_store_value);
    TEST_ASSERT("for the no-H same-page cross-16B translated scalar store, the first repeated 4B+4B second-split write fault must still report store access fault on the true second-half start address before the retried-success then middle-bridge-halfword follow-up store probe",
        excpt.triggered == true &&
        excpt.cause == CAUSE_SAF &&
        excpt.tval == second_half_vaddr
    );

    TEST_SETUP_EXCEPT();
    ai_micro_mab_retry_byte4_exact_store_translated_sd(
        fault_vaddr, first_fault_store_value);
    TEST_ASSERT("repeating the same no-H second-split 4B+4B write fault keeps the same precise metadata before the retried-success then middle-bridge-halfword follow-up store probe",
        excpt.triggered == true &&
        excpt.cause == CAUSE_SAF &&
        excpt.tval == second_half_vaddr
    );

    ai_micro_mab_retry_byte4_exact_store_restore_pmp_full_access();

    TEST_SETUP_EXCEPT();
    ai_micro_mab_retry_byte4_exact_store_translated_sd(
        aligned_upper_store_vaddr, aligned_upper_store_value_first);
    TEST_ASSERT("after repair, the first same-boundary translated upper-half aligned store should complete cleanly before the refaulted retry-4B+4B-store then middle-bridge-halfword follow-up store probe",
        excpt.triggered == false
    );

    TEST_SETUP_EXCEPT();
    ai_micro_mab_retry_byte4_exact_store_translated_sd(
        aligned_upper_store_vaddr, aligned_upper_store_value_second);
    TEST_ASSERT("after one upper-half aligned translated store has already completed on the repaired boundary, the second upper-half aligned translated store should stay trap-free before the refaulted retry-4B+4B-store then middle-bridge-halfword follow-up store probe",
        excpt.triggered == false
    );

    TEST_SETUP_EXCEPT();
    ai_micro_mab_retry_byte4_exact_store_translated_sd(
        aligned_upper_store_vaddr, aligned_upper_store_value_third);
    TEST_ASSERT("after two upper-half aligned translated stores have already completed on the repaired boundary, the third upper-half aligned translated store should stay trap-free before the refaulted retry-4B+4B-store then middle-bridge-halfword follow-up store probe",
        excpt.triggered == false
    );

    TEST_SETUP_EXCEPT();
    ai_micro_mab_retry_byte4_exact_store_translated_sd(
        aligned_upper_store_vaddr, aligned_upper_store_value_fourth);
    TEST_ASSERT("after three upper-half aligned translated stores have already completed on the repaired boundary, the fourth upper-half aligned translated store should stay trap-free and become the latest committed high-half payload before the refaulted retry-4B+4B-store then middle-bridge-halfword follow-up store probe",
        excpt.triggered == false
    );

    ai_micro_mab_retry_byte4_exact_store_configure_pmp_window(
        deny_start, deny_end,
        AI_MICRO_MAB_RETRY_BYTE4_EXACT_STORE_PMP_TOR_NO_W);

    TEST_SETUP_EXCEPT();
    ai_micro_mab_retry_byte4_exact_store_translated_sd(
        fault_vaddr, refault_store_value);
    TEST_ASSERT("after four upper-half aligned translated stores have already completed on the repaired boundary, restoring the upper-half deny window and reissuing the original 4B+4B misaligned store should refresh the same precise second-split store access fault metadata before the middle-bridge-halfword follow-up store probe",
        excpt.triggered == true &&
        excpt.cause == CAUSE_SAF &&
        excpt.tval == second_half_vaddr
    );

    ai_micro_mab_retry_byte4_exact_store_restore_pmp_full_access();

    TEST_SETUP_EXCEPT();
    ai_micro_mab_retry_byte4_exact_store_translated_sd(
        fault_vaddr, retry_4b4b_store_value);
    TEST_ASSERT("after the refault episode is repaired again, retrying the same 4B+4B translated store as a success store should clear stale exception metadata before the immediate middle-bridge-halfword follow-up store",
        excpt.triggered == false
    );

    TEST_SETUP_EXCEPT();
    ai_micro_mab_retry_byte4_exact_store_translated_sh(
        followup_middle_bridge_halfword_store_vaddr,
        followup_middle_bridge_halfword_store_value);
    TEST_ASSERT("after the retried original-template 4B+4B store has already succeeded on the same boundary, the immediate translated middle-bridge-halfword store should also stay trap-free and must not reuse the retired fake-crosspage exact 4-byte high-half word window outside bytes9-10",
        excpt.triggered == false
    );

    ai_micro_mab_retry_byte4_exact_store_overlay_little_endian_bytes(
        expected_boundary, boundary_seed, 16U, 8U,
        aligned_upper_store_value_fourth, sizeof(uint64_t));
    ai_micro_mab_retry_byte4_exact_store_overlay_little_endian_bytes(
        expected_boundary, expected_boundary, 16U, 4U,
        retry_4b4b_store_value, sizeof(uint64_t));
    ai_micro_mab_retry_byte4_exact_store_overlay_little_endian_bytes(
        expected_boundary, expected_boundary, 16U, 9U,
        (uint64_t)followup_middle_bridge_halfword_store_value,
        sizeof(uint16_t));

    TEST_ASSERT("after the prolonged upper-half producer, repaired refault, retried 4B+4B success store, and immediate middle-bridge-halfword store, the final boundary bytes should preserve bytes4-8 from the retried image, refresh only bytes9-10, preserve byte11 from the retried image, and keep bytes12-15 from the latest aligned upper-half image",
        ai_micro_mab_retry_byte4_exact_store_bytes_match(
            boundary_phys, expected_boundary, 16U)
    );

    TEST_ASSERT("guard regions around the same-page boundary should remain unchanged through the repeated 4B+4B fault, four upper-half aligned stores, refault, retried success store, and immediate middle-bridge-halfword follow-up store sequence",
        ai_micro_mab_retry_byte4_exact_store_bytes_match(
            guard_before_phys, guard_before_seed, 8U) &&
        ai_micro_mab_retry_byte4_exact_store_bytes_match(
            guard_after_phys, guard_after_seed, 8U)
    );

    ai_micro_mab_retry_byte4_exact_store_reset_env();

    TEST_END(
        "ai_micro_m_samepage_cross16b_store_refault_upperx4_retry_4b4b_store_then_middle_bridge_halfword_store_corner");
}

bool ai_micro_m_samepage_cross16b_store_refault_upperx4_retry_4b4b_store_then_first_upper_byte_store_corner()
{
    TEST_START();

    static const uint8_t boundary_seed[16] = {
        0xac, 0x9b, 0x8a, 0x79, 0x68, 0x57, 0x46, 0x35,
        0x24, 0x13, 0x02, 0xf1, 0xe0, 0xdf, 0xce, 0xbd,
    };
    static const uint8_t guard_before_seed[8] = {
        0x26, 0x38, 0x4a, 0x5c, 0x6e, 0x70, 0x82, 0x94,
    };
    static const uint8_t guard_after_seed[8] = {
        0xb9, 0xcb, 0xdd, 0xef, 0x01, 0x13, 0x25, 0x37,
    };
    const enum test_page low_page = SWITCH1;
    const enum test_page high_page = SWITCH2;
    const uintptr_t page_phys = phys_page_base(low_page) + PAGE_SIZE - 16U;
    const uintptr_t page_vaddr = hs_page_base(low_page) + PAGE_SIZE - 16U;
    const uintptr_t boundary_phys = page_phys +
        AI_MICRO_MAB_RETRY_BYTE4_EXACT_STORE_BOUNDARY_BASE_OFFSET;
    const uintptr_t guard_before_phys = page_phys;
    const uintptr_t guard_after_phys = page_phys + 24U;
    const uintptr_t fault_vaddr = page_vaddr +
        AI_MICRO_MAB_RETRY_BYTE4_EXACT_STORE_CROSS_16B_SD_OFFSET;
    const uintptr_t second_half_vaddr = page_vaddr + 16U;
    const uintptr_t aligned_upper_store_vaddr = page_vaddr +
        AI_MICRO_MAB_RETRY_BYTE4_EXACT_STORE_BOUNDARY_BASE_OFFSET + 8U;
    const uintptr_t followup_first_upper_byte_store_vaddr =
        aligned_upper_store_vaddr;
    const uintptr_t deny_start = boundary_phys + 8U;
    const uintptr_t deny_end = deny_start + PAGE_SIZE;
    const uint64_t first_fault_store_value = 0xb8c9daebfc0d1e2fULL;
    const uint64_t refault_store_value = 0x38495a6b7c8d9eafULL;
    const uint64_t aligned_upper_store_value_first = 0x758697a8b9cadbecULL;
    const uint64_t aligned_upper_store_value_second = 0xfd0e1f2031425364ULL;
    const uint64_t aligned_upper_store_value_third = 0x8697a8b9cadbecfdULL;
    const uint64_t aligned_upper_store_value_fourth = 0x0e1f203142536475ULL;
    const uint64_t retry_4b4b_store_value = 0x9eafc0d1e2f30415ULL;
    const uint8_t followup_first_upper_byte_store_value = 0x7dU;
    uint8_t expected_boundary[16];

    goto_priv(PRIV_M);
    ai_micro_mab_retry_byte4_exact_store_reset_env();
    ai_micro_mab_retry_byte4_exact_store_write_bytes(
        boundary_phys, boundary_seed, 16U);
    ai_micro_mab_retry_byte4_exact_store_write_bytes(
        guard_before_phys, guard_before_seed, 8U);
    ai_micro_mab_retry_byte4_exact_store_write_bytes(
        guard_after_phys, guard_after_seed, 8U);

    ai_micro_mab_retry_byte4_exact_store_configure_pmp_window(
        deny_start, deny_end,
        AI_MICRO_MAB_RETRY_BYTE4_EXACT_STORE_PMP_TOR_NO_W);
    ai_micro_mab_retry_byte4_exact_store_prepare_supervisor_translation();
    ai_micro_mab_retry_byte4_exact_store_set_leaf(
        low_page, PTE_V | PTE_AD | PTE_RWX);
    ai_micro_mab_retry_byte4_exact_store_set_leaf(
        high_page, PTE_V | PTE_AD | PTE_RWX);
    sfence_vma();

    TEST_SETUP_EXCEPT();
    ai_micro_mab_retry_byte4_exact_store_translated_sd(
        fault_vaddr, first_fault_store_value);
    TEST_ASSERT("for the no-H same-page cross-16B translated scalar store, the first repeated 4B+4B second-split write fault must still report store access fault on the true second-half start address before the retried-success then first-upper-byte follow-up store probe",
        excpt.triggered == true &&
        excpt.cause == CAUSE_SAF &&
        excpt.tval == second_half_vaddr
    );

    TEST_SETUP_EXCEPT();
    ai_micro_mab_retry_byte4_exact_store_translated_sd(
        fault_vaddr, first_fault_store_value);
    TEST_ASSERT("repeating the same no-H second-split 4B+4B write fault keeps the same precise metadata before the retried-success then first-upper-byte follow-up store probe",
        excpt.triggered == true &&
        excpt.cause == CAUSE_SAF &&
        excpt.tval == second_half_vaddr
    );

    ai_micro_mab_retry_byte4_exact_store_restore_pmp_full_access();

    TEST_SETUP_EXCEPT();
    ai_micro_mab_retry_byte4_exact_store_translated_sd(
        aligned_upper_store_vaddr, aligned_upper_store_value_first);
    TEST_ASSERT("after repair, the first same-boundary translated upper-half aligned store should complete cleanly before the refaulted retry-4B+4B-store then first-upper-byte follow-up store probe",
        excpt.triggered == false
    );

    TEST_SETUP_EXCEPT();
    ai_micro_mab_retry_byte4_exact_store_translated_sd(
        aligned_upper_store_vaddr, aligned_upper_store_value_second);
    TEST_ASSERT("after one upper-half aligned translated store has already completed on the repaired boundary, the second upper-half aligned translated store should stay trap-free before the refaulted retry-4B+4B-store then first-upper-byte follow-up store probe",
        excpt.triggered == false
    );

    TEST_SETUP_EXCEPT();
    ai_micro_mab_retry_byte4_exact_store_translated_sd(
        aligned_upper_store_vaddr, aligned_upper_store_value_third);
    TEST_ASSERT("after two upper-half aligned translated stores have already completed on the repaired boundary, the third upper-half aligned translated store should stay trap-free before the refaulted retry-4B+4B-store then first-upper-byte follow-up store probe",
        excpt.triggered == false
    );

    TEST_SETUP_EXCEPT();
    ai_micro_mab_retry_byte4_exact_store_translated_sd(
        aligned_upper_store_vaddr, aligned_upper_store_value_fourth);
    TEST_ASSERT("after three upper-half aligned translated stores have already completed on the repaired boundary, the fourth upper-half aligned translated store should stay trap-free and become the latest committed high-half payload before the refaulted retry-4B+4B-store then first-upper-byte follow-up store probe",
        excpt.triggered == false
    );

    ai_micro_mab_retry_byte4_exact_store_configure_pmp_window(
        deny_start, deny_end,
        AI_MICRO_MAB_RETRY_BYTE4_EXACT_STORE_PMP_TOR_NO_W);

    TEST_SETUP_EXCEPT();
    ai_micro_mab_retry_byte4_exact_store_translated_sd(
        fault_vaddr, refault_store_value);
    TEST_ASSERT("after four upper-half aligned translated stores have already completed on the repaired boundary, restoring the upper-half deny window and reissuing the original 4B+4B misaligned store should refresh the same precise second-split store access fault metadata before the first-upper-byte follow-up store probe",
        excpt.triggered == true &&
        excpt.cause == CAUSE_SAF &&
        excpt.tval == second_half_vaddr
    );

    ai_micro_mab_retry_byte4_exact_store_restore_pmp_full_access();

    TEST_SETUP_EXCEPT();
    ai_micro_mab_retry_byte4_exact_store_translated_sd(
        fault_vaddr, retry_4b4b_store_value);
    TEST_ASSERT("after the refault episode is repaired again, retrying the same 4B+4B translated store as a success store should clear stale exception metadata before the immediate first-upper-byte follow-up store",
        excpt.triggered == false
    );

    TEST_SETUP_EXCEPT();
    ai_micro_mab_retry_byte4_exact_store_translated_sb(
        followup_first_upper_byte_store_vaddr,
        followup_first_upper_byte_store_value);
    TEST_ASSERT("after the retried original-template 4B+4B store has already succeeded on the same boundary, the immediate translated first-upper-byte store should also stay trap-free and must not reuse the retired fake-crosspage exact 4-byte high-half word window beyond byte8",
        excpt.triggered == false
    );

    ai_micro_mab_retry_byte4_exact_store_overlay_little_endian_bytes(
        expected_boundary, boundary_seed, 16U, 8U,
        aligned_upper_store_value_fourth, sizeof(uint64_t));
    ai_micro_mab_retry_byte4_exact_store_overlay_little_endian_bytes(
        expected_boundary, expected_boundary, 16U, 4U,
        retry_4b4b_store_value, sizeof(uint64_t));
    ai_micro_mab_retry_byte4_exact_store_overlay_little_endian_bytes(
        expected_boundary, expected_boundary, 16U, 8U,
        (uint64_t)followup_first_upper_byte_store_value,
        sizeof(uint8_t));

    TEST_ASSERT("after the prolonged upper-half producer, repaired refault, retried 4B+4B success store, and immediate first-upper-byte store, the final boundary bytes should preserve bytes4-7 from the retried image, refresh only byte8, preserve bytes9-11 from the retried image, and keep bytes12-15 from the latest aligned upper-half image",
        ai_micro_mab_retry_byte4_exact_store_bytes_match(
            boundary_phys, expected_boundary, 16U)
    );

    TEST_ASSERT("guard regions around the same-page boundary should remain unchanged through the repeated 4B+4B fault, four upper-half aligned stores, refault, retried success store, and immediate first-upper-byte follow-up store sequence",
        ai_micro_mab_retry_byte4_exact_store_bytes_match(
            guard_before_phys, guard_before_seed, 8U) &&
        ai_micro_mab_retry_byte4_exact_store_bytes_match(
            guard_after_phys, guard_after_seed, 8U)
    );

    ai_micro_mab_retry_byte4_exact_store_reset_env();

    TEST_END(
        "ai_micro_m_samepage_cross16b_store_refault_upperx4_retry_4b4b_store_then_first_upper_byte_store_corner");
}

bool ai_micro_m_samepage_cross16b_store_refault_upperx4_retry_4b4b_store_then_second_upper_byte_store_corner()
{
    TEST_START();

    static const uint8_t boundary_seed[16] = {
        0xbd, 0xac, 0x9b, 0x8a, 0x79, 0x68, 0x57, 0x46,
        0x35, 0x24, 0x13, 0x02, 0xf1, 0xe0, 0xdf, 0xce,
    };
    static const uint8_t guard_before_seed[8] = {
        0x29, 0x3b, 0x4d, 0x5f, 0x61, 0x73, 0x85, 0x97,
    };
    static const uint8_t guard_after_seed[8] = {
        0xb4, 0xc6, 0xd8, 0xea, 0xfc, 0x0e, 0x10, 0x22,
    };
    const enum test_page low_page = SWITCH1;
    const enum test_page high_page = SWITCH2;
    const uintptr_t page_phys = phys_page_base(low_page) + PAGE_SIZE - 16U;
    const uintptr_t page_vaddr = hs_page_base(low_page) + PAGE_SIZE - 16U;
    const uintptr_t boundary_phys = page_phys +
        AI_MICRO_MAB_RETRY_BYTE4_EXACT_STORE_BOUNDARY_BASE_OFFSET;
    const uintptr_t guard_before_phys = page_phys;
    const uintptr_t guard_after_phys = page_phys + 24U;
    const uintptr_t fault_vaddr = page_vaddr +
        AI_MICRO_MAB_RETRY_BYTE4_EXACT_STORE_CROSS_16B_SD_OFFSET;
    const uintptr_t second_half_vaddr = page_vaddr + 16U;
    const uintptr_t aligned_upper_store_vaddr = page_vaddr +
        AI_MICRO_MAB_RETRY_BYTE4_EXACT_STORE_BOUNDARY_BASE_OFFSET + 8U;
    const uintptr_t followup_second_upper_byte_store_vaddr =
        aligned_upper_store_vaddr + 1U;
    const uintptr_t deny_start = boundary_phys + 8U;
    const uintptr_t deny_end = deny_start + PAGE_SIZE;
    const uint64_t first_fault_store_value = 0xc9daebfc0d1e2f30ULL;
    const uint64_t refault_store_value = 0x495a6b7c8d9eafc0ULL;
    const uint64_t aligned_upper_store_value_first = 0x8697a8b9cadbecfdULL;
    const uint64_t aligned_upper_store_value_second = 0x0e1f203142536475ULL;
    const uint64_t aligned_upper_store_value_third = 0x97a8b9cadbecfd0eULL;
    const uint64_t aligned_upper_store_value_fourth = 0x1f20314253647586ULL;
    const uint64_t retry_4b4b_store_value = 0xafc0d1e2f3041526ULL;
    const uint8_t followup_second_upper_byte_store_value = 0x8eU;
    uint8_t expected_boundary[16];

    goto_priv(PRIV_M);
    ai_micro_mab_retry_byte4_exact_store_reset_env();
    ai_micro_mab_retry_byte4_exact_store_write_bytes(
        boundary_phys, boundary_seed, 16U);
    ai_micro_mab_retry_byte4_exact_store_write_bytes(
        guard_before_phys, guard_before_seed, 8U);
    ai_micro_mab_retry_byte4_exact_store_write_bytes(
        guard_after_phys, guard_after_seed, 8U);

    ai_micro_mab_retry_byte4_exact_store_configure_pmp_window(
        deny_start, deny_end,
        AI_MICRO_MAB_RETRY_BYTE4_EXACT_STORE_PMP_TOR_NO_W);
    ai_micro_mab_retry_byte4_exact_store_prepare_supervisor_translation();
    ai_micro_mab_retry_byte4_exact_store_set_leaf(
        low_page, PTE_V | PTE_AD | PTE_RWX);
    ai_micro_mab_retry_byte4_exact_store_set_leaf(
        high_page, PTE_V | PTE_AD | PTE_RWX);
    sfence_vma();

    TEST_SETUP_EXCEPT();
    ai_micro_mab_retry_byte4_exact_store_translated_sd(
        fault_vaddr, first_fault_store_value);
    TEST_ASSERT("for the no-H same-page cross-16B translated scalar store, the first repeated 4B+4B second-split write fault must still report store access fault on the true second-half start address before the retried-success then second-upper-byte follow-up store probe",
        excpt.triggered == true &&
        excpt.cause == CAUSE_SAF &&
        excpt.tval == second_half_vaddr
    );

    TEST_SETUP_EXCEPT();
    ai_micro_mab_retry_byte4_exact_store_translated_sd(
        fault_vaddr, first_fault_store_value);
    TEST_ASSERT("repeating the same no-H second-split 4B+4B write fault keeps the same precise metadata before the retried-success then second-upper-byte follow-up store probe",
        excpt.triggered == true &&
        excpt.cause == CAUSE_SAF &&
        excpt.tval == second_half_vaddr
    );

    ai_micro_mab_retry_byte4_exact_store_restore_pmp_full_access();

    TEST_SETUP_EXCEPT();
    ai_micro_mab_retry_byte4_exact_store_translated_sd(
        aligned_upper_store_vaddr, aligned_upper_store_value_first);
    TEST_ASSERT("after repair, the first same-boundary translated upper-half aligned store should complete cleanly before the refaulted retry-4B+4B-store then second-upper-byte follow-up store probe",
        excpt.triggered == false
    );

    TEST_SETUP_EXCEPT();
    ai_micro_mab_retry_byte4_exact_store_translated_sd(
        aligned_upper_store_vaddr, aligned_upper_store_value_second);
    TEST_ASSERT("after one upper-half aligned translated store has already completed on the repaired boundary, the second upper-half aligned translated store should stay trap-free before the refaulted retry-4B+4B-store then second-upper-byte follow-up store probe",
        excpt.triggered == false
    );

    TEST_SETUP_EXCEPT();
    ai_micro_mab_retry_byte4_exact_store_translated_sd(
        aligned_upper_store_vaddr, aligned_upper_store_value_third);
    TEST_ASSERT("after two upper-half aligned translated stores have already completed on the repaired boundary, the third upper-half aligned translated store should stay trap-free before the refaulted retry-4B+4B-store then second-upper-byte follow-up store probe",
        excpt.triggered == false
    );

    TEST_SETUP_EXCEPT();
    ai_micro_mab_retry_byte4_exact_store_translated_sd(
        aligned_upper_store_vaddr, aligned_upper_store_value_fourth);
    TEST_ASSERT("after three upper-half aligned translated stores have already completed on the repaired boundary, the fourth upper-half aligned translated store should stay trap-free and become the latest committed high-half payload before the refaulted retry-4B+4B-store then second-upper-byte follow-up store probe",
        excpt.triggered == false
    );

    ai_micro_mab_retry_byte4_exact_store_configure_pmp_window(
        deny_start, deny_end,
        AI_MICRO_MAB_RETRY_BYTE4_EXACT_STORE_PMP_TOR_NO_W);

    TEST_SETUP_EXCEPT();
    ai_micro_mab_retry_byte4_exact_store_translated_sd(
        fault_vaddr, refault_store_value);
    TEST_ASSERT("after four upper-half aligned translated stores have already completed on the repaired boundary, restoring the upper-half deny window and reissuing the original 4B+4B misaligned store should refresh the same precise second-split store access fault metadata before the second-upper-byte follow-up store probe",
        excpt.triggered == true &&
        excpt.cause == CAUSE_SAF &&
        excpt.tval == second_half_vaddr
    );

    ai_micro_mab_retry_byte4_exact_store_restore_pmp_full_access();

    TEST_SETUP_EXCEPT();
    ai_micro_mab_retry_byte4_exact_store_translated_sd(
        fault_vaddr, retry_4b4b_store_value);
    TEST_ASSERT("after the refault episode is repaired again, retrying the same 4B+4B translated store as a success store should clear stale exception metadata before the immediate second-upper-byte follow-up store",
        excpt.triggered == false
    );

    TEST_SETUP_EXCEPT();
    ai_micro_mab_retry_byte4_exact_store_translated_sb(
        followup_second_upper_byte_store_vaddr,
        followup_second_upper_byte_store_value);
    TEST_ASSERT("after the retried original-template 4B+4B store has already succeeded on the same boundary, the immediate translated second-upper-byte store should also stay trap-free and must not reuse the retired fake-crosspage exact 4-byte high-half word window beyond byte9",
        excpt.triggered == false
    );

    ai_micro_mab_retry_byte4_exact_store_overlay_little_endian_bytes(
        expected_boundary, boundary_seed, 16U, 8U,
        aligned_upper_store_value_fourth, sizeof(uint64_t));
    ai_micro_mab_retry_byte4_exact_store_overlay_little_endian_bytes(
        expected_boundary, expected_boundary, 16U, 4U,
        retry_4b4b_store_value, sizeof(uint64_t));
    ai_micro_mab_retry_byte4_exact_store_overlay_little_endian_bytes(
        expected_boundary, expected_boundary, 16U, 9U,
        (uint64_t)followup_second_upper_byte_store_value,
        sizeof(uint8_t));

    TEST_ASSERT("after the prolonged upper-half producer, repaired refault, retried 4B+4B success store, and immediate second-upper-byte store, the final boundary bytes should preserve bytes4-8 from the retried image, refresh only byte9, preserve bytes10-11 from the retried image, and keep bytes12-15 from the latest aligned upper-half image",
        ai_micro_mab_retry_byte4_exact_store_bytes_match(
            boundary_phys, expected_boundary, 16U)
    );

    TEST_ASSERT("guard regions around the same-page boundary should remain unchanged through the repeated 4B+4B fault, four upper-half aligned stores, refault, retried success store, and immediate second-upper-byte follow-up store sequence",
        ai_micro_mab_retry_byte4_exact_store_bytes_match(
            guard_before_phys, guard_before_seed, 8U) &&
        ai_micro_mab_retry_byte4_exact_store_bytes_match(
            guard_after_phys, guard_after_seed, 8U)
    );

    ai_micro_mab_retry_byte4_exact_store_reset_env();

    TEST_END(
        "ai_micro_m_samepage_cross16b_store_refault_upperx4_retry_4b4b_store_then_second_upper_byte_store_corner");
}

bool ai_micro_m_samepage_cross16b_store_refault_upperx4_retry_4b4b_store_then_center_semantic_byte_store_corner()
{
    TEST_START();

    static const uint8_t boundary_seed[16] = {
        0xce, 0xbd, 0xac, 0x9b, 0x8a, 0x79, 0x68, 0x57,
        0x46, 0x35, 0x24, 0x13, 0x02, 0xf1, 0xe0, 0xdf,
    };
    static const uint8_t guard_before_seed[8] = {
        0x34, 0x46, 0x58, 0x6a, 0x7c, 0x8e, 0x90, 0xa2,
    };
    static const uint8_t guard_after_seed[8] = {
        0xc7, 0xd9, 0xeb, 0xfd, 0x0f, 0x11, 0x23, 0x35,
    };
    const enum test_page low_page = SWITCH1;
    const enum test_page high_page = SWITCH2;
    const uintptr_t page_phys = phys_page_base(low_page) + PAGE_SIZE - 16U;
    const uintptr_t page_vaddr = hs_page_base(low_page) + PAGE_SIZE - 16U;
    const uintptr_t boundary_phys = page_phys +
        AI_MICRO_MAB_RETRY_BYTE4_EXACT_STORE_BOUNDARY_BASE_OFFSET;
    const uintptr_t guard_before_phys = page_phys;
    const uintptr_t guard_after_phys = page_phys + 24U;
    const uintptr_t fault_vaddr = page_vaddr +
        AI_MICRO_MAB_RETRY_BYTE4_EXACT_STORE_CROSS_16B_SD_OFFSET;
    const uintptr_t second_half_vaddr = page_vaddr + 16U;
    const uintptr_t aligned_upper_store_vaddr = page_vaddr +
        AI_MICRO_MAB_RETRY_BYTE4_EXACT_STORE_BOUNDARY_BASE_OFFSET + 8U;
    const uintptr_t followup_center_semantic_byte_store_vaddr =
        aligned_upper_store_vaddr + 2U;
    const uintptr_t deny_start = boundary_phys + 8U;
    const uintptr_t deny_end = deny_start + PAGE_SIZE;
    const uint64_t first_fault_store_value = 0xdaebfc0d1e2f3041ULL;
    const uint64_t refault_store_value = 0x5a6b7c8d9eafc0d1ULL;
    const uint64_t aligned_upper_store_value_first = 0x97a8b9cadbecfd0eULL;
    const uint64_t aligned_upper_store_value_second = 0x1f20314253647586ULL;
    const uint64_t aligned_upper_store_value_third = 0xa8b9cadbecfd0e1fULL;
    const uint64_t aligned_upper_store_value_fourth = 0x2031425364758697ULL;
    const uint64_t retry_4b4b_store_value = 0xc0d1e2f304152637ULL;
    const uint8_t followup_center_semantic_byte_store_value = 0xc6U;
    uint8_t expected_boundary[16];

    goto_priv(PRIV_M);
    ai_micro_mab_retry_byte4_exact_store_reset_env();
    ai_micro_mab_retry_byte4_exact_store_write_bytes(
        boundary_phys, boundary_seed, 16U);
    ai_micro_mab_retry_byte4_exact_store_write_bytes(
        guard_before_phys, guard_before_seed, 8U);
    ai_micro_mab_retry_byte4_exact_store_write_bytes(
        guard_after_phys, guard_after_seed, 8U);

    ai_micro_mab_retry_byte4_exact_store_configure_pmp_window(
        deny_start, deny_end,
        AI_MICRO_MAB_RETRY_BYTE4_EXACT_STORE_PMP_TOR_NO_W);
    ai_micro_mab_retry_byte4_exact_store_prepare_supervisor_translation();
    ai_micro_mab_retry_byte4_exact_store_set_leaf(
        low_page, PTE_V | PTE_AD | PTE_RWX);
    ai_micro_mab_retry_byte4_exact_store_set_leaf(
        high_page, PTE_V | PTE_AD | PTE_RWX);
    sfence_vma();

    TEST_SETUP_EXCEPT();
    ai_micro_mab_retry_byte4_exact_store_translated_sd(
        fault_vaddr, first_fault_store_value);
    TEST_ASSERT("for the no-H same-page cross-16B translated scalar store, the first repeated 4B+4B second-split write fault must still report store access fault on the true second-half start address before the retried-success then center-semantic-byte follow-up store probe",
        excpt.triggered == true &&
        excpt.cause == CAUSE_SAF &&
        excpt.tval == second_half_vaddr
    );

    TEST_SETUP_EXCEPT();
    ai_micro_mab_retry_byte4_exact_store_translated_sd(
        fault_vaddr, first_fault_store_value);
    TEST_ASSERT("repeating the same no-H second-split 4B+4B write fault keeps the same precise metadata before the retried-success then center-semantic-byte follow-up store probe",
        excpt.triggered == true &&
        excpt.cause == CAUSE_SAF &&
        excpt.tval == second_half_vaddr
    );

    ai_micro_mab_retry_byte4_exact_store_restore_pmp_full_access();

    TEST_SETUP_EXCEPT();
    ai_micro_mab_retry_byte4_exact_store_translated_sd(
        aligned_upper_store_vaddr, aligned_upper_store_value_first);
    TEST_ASSERT("after repair, the first same-boundary translated upper-half aligned store should complete cleanly before the refaulted retry-4B+4B-store then center-semantic-byte follow-up store probe",
        excpt.triggered == false
    );

    TEST_SETUP_EXCEPT();
    ai_micro_mab_retry_byte4_exact_store_translated_sd(
        aligned_upper_store_vaddr, aligned_upper_store_value_second);
    TEST_ASSERT("after one upper-half aligned translated store has already completed on the repaired boundary, the second upper-half aligned translated store should stay trap-free before the refaulted retry-4B+4B-store then center-semantic-byte follow-up store probe",
        excpt.triggered == false
    );

    TEST_SETUP_EXCEPT();
    ai_micro_mab_retry_byte4_exact_store_translated_sd(
        aligned_upper_store_vaddr, aligned_upper_store_value_third);
    TEST_ASSERT("after two upper-half aligned translated stores have already completed on the repaired boundary, the third upper-half aligned translated store should stay trap-free before the refaulted retry-4B+4B-store then center-semantic-byte follow-up store probe",
        excpt.triggered == false
    );

    TEST_SETUP_EXCEPT();
    ai_micro_mab_retry_byte4_exact_store_translated_sd(
        aligned_upper_store_vaddr, aligned_upper_store_value_fourth);
    TEST_ASSERT("after three upper-half aligned translated stores have already completed on the repaired boundary, the fourth upper-half aligned translated store should stay trap-free and become the latest committed high-half payload before the refaulted retry-4B+4B-store then center-semantic-byte follow-up store probe",
        excpt.triggered == false
    );

    ai_micro_mab_retry_byte4_exact_store_configure_pmp_window(
        deny_start, deny_end,
        AI_MICRO_MAB_RETRY_BYTE4_EXACT_STORE_PMP_TOR_NO_W);

    TEST_SETUP_EXCEPT();
    ai_micro_mab_retry_byte4_exact_store_translated_sd(
        fault_vaddr, refault_store_value);
    TEST_ASSERT("after four upper-half aligned translated stores have already completed on the repaired boundary, restoring the upper-half deny window and reissuing the original 4B+4B misaligned store should refresh the same precise second-split store access fault metadata before the center-semantic-byte follow-up store probe",
        excpt.triggered == true &&
        excpt.cause == CAUSE_SAF &&
        excpt.tval == second_half_vaddr
    );

    ai_micro_mab_retry_byte4_exact_store_restore_pmp_full_access();

    TEST_SETUP_EXCEPT();
    ai_micro_mab_retry_byte4_exact_store_translated_sd(
        fault_vaddr, retry_4b4b_store_value);
    TEST_ASSERT("after the refault episode is repaired again, retrying the same 4B+4B translated store as a success store should clear stale exception metadata before the immediate center-semantic-byte follow-up store",
        excpt.triggered == false
    );

    TEST_SETUP_EXCEPT();
    ai_micro_mab_retry_byte4_exact_store_translated_sb(
        followup_center_semantic_byte_store_vaddr,
        followup_center_semantic_byte_store_value);
    TEST_ASSERT("after the retried original-template 4B+4B store has already succeeded on the same boundary, the immediate translated center-semantic-byte store should also stay trap-free and must not reuse the retired fake-crosspage exact 4-byte high-half word window beyond byte10",
        excpt.triggered == false
    );

    ai_micro_mab_retry_byte4_exact_store_overlay_little_endian_bytes(
        expected_boundary, boundary_seed, 16U, 8U,
        aligned_upper_store_value_fourth, sizeof(uint64_t));
    ai_micro_mab_retry_byte4_exact_store_overlay_little_endian_bytes(
        expected_boundary, expected_boundary, 16U, 4U,
        retry_4b4b_store_value, sizeof(uint64_t));
    ai_micro_mab_retry_byte4_exact_store_overlay_little_endian_bytes(
        expected_boundary, expected_boundary, 16U, 10U,
        (uint64_t)followup_center_semantic_byte_store_value,
        sizeof(uint8_t));

    TEST_ASSERT("after the prolonged upper-half producer, repaired refault, retried 4B+4B success store, and immediate center-semantic-byte store, the final boundary bytes should preserve bytes4-9 from the retried image, refresh only byte10, preserve byte11 from the retried image, and keep bytes12-15 from the latest aligned upper-half image",
        ai_micro_mab_retry_byte4_exact_store_bytes_match(
            boundary_phys, expected_boundary, 16U)
    );

    TEST_ASSERT("guard regions around the same-page boundary should remain unchanged through the repeated 4B+4B fault, four upper-half aligned stores, refault, retried success store, and immediate center-semantic-byte follow-up store sequence",
        ai_micro_mab_retry_byte4_exact_store_bytes_match(
            guard_before_phys, guard_before_seed, 8U) &&
        ai_micro_mab_retry_byte4_exact_store_bytes_match(
            guard_after_phys, guard_after_seed, 8U)
    );

    ai_micro_mab_retry_byte4_exact_store_reset_env();

    TEST_END(
        "ai_micro_m_samepage_cross16b_store_refault_upperx4_retry_4b4b_store_then_center_semantic_byte_store_corner");
}

bool ai_micro_m_samepage_cross16b_store_refault_upperx4_retry_4b4b_store_then_middle_tail_bridge_word_store_corner()
{
    TEST_START();

    static const uint8_t boundary_seed[16] = {
        0xdf, 0xce, 0xbd, 0xac, 0x9b, 0x8a, 0x79, 0x68,
        0x57, 0x46, 0x35, 0x24, 0x13, 0x02, 0xf1, 0xe0,
    };
    static const uint8_t guard_before_seed[8] = {
        0x3f, 0x51, 0x63, 0x75, 0x87, 0x99, 0xab, 0xbd,
    };
    static const uint8_t guard_after_seed[8] = {
        0xd6, 0xe8, 0xfa, 0x0c, 0x1e, 0x30, 0x42, 0x54,
    };
    const enum test_page low_page = SWITCH1;
    const enum test_page high_page = SWITCH2;
    const uintptr_t page_phys = phys_page_base(low_page) + PAGE_SIZE - 16U;
    const uintptr_t page_vaddr = hs_page_base(low_page) + PAGE_SIZE - 16U;
    const uintptr_t boundary_phys = page_phys +
        AI_MICRO_MAB_RETRY_BYTE4_EXACT_STORE_BOUNDARY_BASE_OFFSET;
    const uintptr_t guard_before_phys = page_phys;
    const uintptr_t guard_after_phys = page_phys + 24U;
    const uintptr_t fault_vaddr = page_vaddr +
        AI_MICRO_MAB_RETRY_BYTE4_EXACT_STORE_CROSS_16B_SD_OFFSET;
    const uintptr_t second_half_vaddr = page_vaddr + 16U;
    const uintptr_t aligned_upper_store_vaddr = page_vaddr +
        AI_MICRO_MAB_RETRY_BYTE4_EXACT_STORE_BOUNDARY_BASE_OFFSET + 8U;
    const uintptr_t followup_middle_tail_bridge_word_store_vaddr =
        aligned_upper_store_vaddr + 1U;
    const uintptr_t deny_start = boundary_phys + 8U;
    const uintptr_t deny_end = deny_start + PAGE_SIZE;
    const uint64_t first_fault_store_value = 0xebfc0d1e2f304152ULL;
    const uint64_t refault_store_value = 0x6b7c8d9eafc0d1e2ULL;
    const uint64_t aligned_upper_store_value_first = 0xa8b9cadbecfd0e1fULL;
    const uint64_t aligned_upper_store_value_second = 0x31425364758697a8ULL;
    const uint64_t aligned_upper_store_value_third = 0xb9cadbecfd0e1f20ULL;
    const uint64_t aligned_upper_store_value_fourth = 0x425364758697a8b9ULL;
    const uint64_t retry_4b4b_store_value = 0xd1e2f30415263748ULL;
    const uint32_t followup_middle_tail_bridge_word_store_value = 0x7d6c5b4aU;
    uint8_t expected_boundary[16];

    goto_priv(PRIV_M);
    ai_micro_mab_retry_byte4_exact_store_reset_env();
    ai_micro_mab_retry_byte4_exact_store_write_bytes(
        boundary_phys, boundary_seed, 16U);
    ai_micro_mab_retry_byte4_exact_store_write_bytes(
        guard_before_phys, guard_before_seed, 8U);
    ai_micro_mab_retry_byte4_exact_store_write_bytes(
        guard_after_phys, guard_after_seed, 8U);

    ai_micro_mab_retry_byte4_exact_store_configure_pmp_window(
        deny_start, deny_end,
        AI_MICRO_MAB_RETRY_BYTE4_EXACT_STORE_PMP_TOR_NO_W);
    ai_micro_mab_retry_byte4_exact_store_prepare_supervisor_translation();
    ai_micro_mab_retry_byte4_exact_store_set_leaf(
        low_page, PTE_V | PTE_AD | PTE_RWX);
    ai_micro_mab_retry_byte4_exact_store_set_leaf(
        high_page, PTE_V | PTE_AD | PTE_RWX);
    sfence_vma();

    TEST_SETUP_EXCEPT();
    ai_micro_mab_retry_byte4_exact_store_translated_sd(
        fault_vaddr, first_fault_store_value);
    TEST_ASSERT("for the no-H same-page cross-16B translated scalar store, the first repeated 4B+4B second-split write fault must still report store access fault on the true second-half start address before the retried-success then middle-tail-bridge-word follow-up store probe",
        excpt.triggered == true &&
        excpt.cause == CAUSE_SAF &&
        excpt.tval == second_half_vaddr
    );

    TEST_SETUP_EXCEPT();
    ai_micro_mab_retry_byte4_exact_store_translated_sd(
        fault_vaddr, first_fault_store_value);
    TEST_ASSERT("repeating the same no-H second-split 4B+4B write fault keeps the same precise metadata before the retried-success then middle-tail-bridge-word follow-up store probe",
        excpt.triggered == true &&
        excpt.cause == CAUSE_SAF &&
        excpt.tval == second_half_vaddr
    );

    ai_micro_mab_retry_byte4_exact_store_restore_pmp_full_access();

    TEST_SETUP_EXCEPT();
    ai_micro_mab_retry_byte4_exact_store_translated_sd(
        aligned_upper_store_vaddr, aligned_upper_store_value_first);
    TEST_ASSERT("after repair, the first same-boundary translated upper-half aligned store should complete cleanly before the refaulted retry-4B+4B-store then middle-tail-bridge-word follow-up store probe",
        excpt.triggered == false
    );

    TEST_SETUP_EXCEPT();
    ai_micro_mab_retry_byte4_exact_store_translated_sd(
        aligned_upper_store_vaddr, aligned_upper_store_value_second);
    TEST_ASSERT("after one upper-half aligned translated store has already completed on the repaired boundary, the second upper-half aligned translated store should stay trap-free before the refaulted retry-4B+4B-store then middle-tail-bridge-word follow-up store probe",
        excpt.triggered == false
    );

    TEST_SETUP_EXCEPT();
    ai_micro_mab_retry_byte4_exact_store_translated_sd(
        aligned_upper_store_vaddr, aligned_upper_store_value_third);
    TEST_ASSERT("after two upper-half aligned translated stores have already completed on the repaired boundary, the third upper-half aligned translated store should stay trap-free before the refaulted retry-4B+4B-store then middle-tail-bridge-word follow-up store probe",
        excpt.triggered == false
    );

    TEST_SETUP_EXCEPT();
    ai_micro_mab_retry_byte4_exact_store_translated_sd(
        aligned_upper_store_vaddr, aligned_upper_store_value_fourth);
    TEST_ASSERT("after three upper-half aligned translated stores have already completed on the repaired boundary, the fourth upper-half aligned translated store should stay trap-free and become the latest committed high-half payload before the refaulted retry-4B+4B-store then middle-tail-bridge-word follow-up store probe",
        excpt.triggered == false
    );

    ai_micro_mab_retry_byte4_exact_store_configure_pmp_window(
        deny_start, deny_end,
        AI_MICRO_MAB_RETRY_BYTE4_EXACT_STORE_PMP_TOR_NO_W);

    TEST_SETUP_EXCEPT();
    ai_micro_mab_retry_byte4_exact_store_translated_sd(
        fault_vaddr, refault_store_value);
    TEST_ASSERT("after four upper-half aligned translated stores have already completed on the repaired boundary, restoring the upper-half deny window and reissuing the original 4B+4B misaligned store should refresh the same precise second-split store access fault metadata before the middle-tail-bridge-word follow-up store probe",
        excpt.triggered == true &&
        excpt.cause == CAUSE_SAF &&
        excpt.tval == second_half_vaddr
    );

    ai_micro_mab_retry_byte4_exact_store_restore_pmp_full_access();

    TEST_SETUP_EXCEPT();
    ai_micro_mab_retry_byte4_exact_store_translated_sd(
        fault_vaddr, retry_4b4b_store_value);
    TEST_ASSERT("after the refault episode is repaired again, retrying the same 4B+4B translated store as a success store should clear stale exception metadata before the immediate middle-tail-bridge-word follow-up store",
        excpt.triggered == false
    );

    TEST_SETUP_EXCEPT();
    ai_micro_mab_retry_byte4_exact_store_translated_sw(
        followup_middle_tail_bridge_word_store_vaddr,
        followup_middle_tail_bridge_word_store_value);
    TEST_ASSERT("after the retried original-template 4B+4B store has already succeeded on the same boundary, the immediate translated middle-tail-bridge-word store should also stay trap-free and must not reuse the retired fake-crosspage exact 4-byte high-half word window beyond bytes9-11 while handing byte12 to the new split tail byte",
        excpt.triggered == false
    );

    ai_micro_mab_retry_byte4_exact_store_overlay_little_endian_bytes(
        expected_boundary, boundary_seed, 16U, 8U,
        aligned_upper_store_value_fourth, sizeof(uint64_t));
    ai_micro_mab_retry_byte4_exact_store_overlay_little_endian_bytes(
        expected_boundary, expected_boundary, 16U, 4U,
        retry_4b4b_store_value, sizeof(uint64_t));
    ai_micro_mab_retry_byte4_exact_store_overlay_little_endian_bytes(
        expected_boundary, expected_boundary, 16U, 9U,
        (uint64_t)followup_middle_tail_bridge_word_store_value,
        sizeof(uint32_t));

    TEST_ASSERT("after the prolonged upper-half producer, repaired refault, retried 4B+4B success store, and immediate middle-tail-bridge-word store, the final boundary bytes should preserve bytes4-8 from the retried image, refresh only bytes9-12, and keep bytes13-15 from the latest aligned upper-half image",
        ai_micro_mab_retry_byte4_exact_store_bytes_match(
            boundary_phys, expected_boundary, 16U)
    );

    TEST_ASSERT("guard regions around the same-page boundary should remain unchanged through the repeated 4B+4B fault, four upper-half aligned stores, refault, retried success store, and immediate middle-tail-bridge-word follow-up store sequence",
        ai_micro_mab_retry_byte4_exact_store_bytes_match(
            guard_before_phys, guard_before_seed, 8U) &&
        ai_micro_mab_retry_byte4_exact_store_bytes_match(
            guard_after_phys, guard_after_seed, 8U)
    );

    ai_micro_mab_retry_byte4_exact_store_reset_env();

    TEST_END(
        "ai_micro_m_samepage_cross16b_store_refault_upperx4_retry_4b4b_store_then_middle_tail_bridge_word_store_corner");
}

bool ai_micro_m_samepage_cross16b_store_refault_upperx4_retry_4b4b_store_then_tail_adjacent_bridge_word_store_corner()
{
    TEST_START();

    static const uint8_t boundary_seed[16] = {
        0xe0, 0xdf, 0xce, 0xbd, 0xac, 0x9b, 0x8a, 0x79,
        0x68, 0x57, 0x46, 0x35, 0x24, 0x13, 0x02, 0xf1,
    };
    static const uint8_t guard_before_seed[8] = {
        0x42, 0x54, 0x66, 0x78, 0x8a, 0x9c, 0xae, 0xc0,
    };
    static const uint8_t guard_after_seed[8] = {
        0xdb, 0xed, 0xff, 0x11, 0x23, 0x35, 0x47, 0x59,
    };
    const enum test_page low_page = SWITCH1;
    const enum test_page high_page = SWITCH2;
    const uintptr_t page_phys = phys_page_base(low_page) + PAGE_SIZE - 16U;
    const uintptr_t page_vaddr = hs_page_base(low_page) + PAGE_SIZE - 16U;
    const uintptr_t boundary_phys = page_phys +
        AI_MICRO_MAB_RETRY_BYTE4_EXACT_STORE_BOUNDARY_BASE_OFFSET;
    const uintptr_t guard_before_phys = page_phys;
    const uintptr_t guard_after_phys = page_phys + 24U;
    const uintptr_t fault_vaddr = page_vaddr +
        AI_MICRO_MAB_RETRY_BYTE4_EXACT_STORE_CROSS_16B_SD_OFFSET;
    const uintptr_t second_half_vaddr = page_vaddr + 16U;
    const uintptr_t aligned_upper_store_vaddr = page_vaddr +
        AI_MICRO_MAB_RETRY_BYTE4_EXACT_STORE_BOUNDARY_BASE_OFFSET + 8U;
    const uintptr_t followup_tail_adjacent_bridge_word_store_vaddr =
        aligned_upper_store_vaddr + 2U;
    const uintptr_t deny_start = boundary_phys + 8U;
    const uintptr_t deny_end = deny_start + PAGE_SIZE;
    const uint64_t first_fault_store_value = 0xfc0d1e2f30415263ULL;
    const uint64_t refault_store_value = 0x7c8d9eafc0d1e2f3ULL;
    const uint64_t aligned_upper_store_value_first = 0xb9cadbecfd0e1f20ULL;
    const uint64_t aligned_upper_store_value_second = 0x425364758697a8b9ULL;
    const uint64_t aligned_upper_store_value_third = 0xcadbecfd0e1f2031ULL;
    const uint64_t aligned_upper_store_value_fourth = 0x5364758697a8b9caULL;
    const uint64_t retry_4b4b_store_value = 0xe2f3041526374859ULL;
    const uint32_t followup_tail_adjacent_bridge_word_store_value =
        0x8e7d6c5bU;
    uint8_t expected_boundary[16];

    goto_priv(PRIV_M);
    ai_micro_mab_retry_byte4_exact_store_reset_env();
    ai_micro_mab_retry_byte4_exact_store_write_bytes(
        boundary_phys, boundary_seed, 16U);
    ai_micro_mab_retry_byte4_exact_store_write_bytes(
        guard_before_phys, guard_before_seed, 8U);
    ai_micro_mab_retry_byte4_exact_store_write_bytes(
        guard_after_phys, guard_after_seed, 8U);

    ai_micro_mab_retry_byte4_exact_store_configure_pmp_window(
        deny_start, deny_end,
        AI_MICRO_MAB_RETRY_BYTE4_EXACT_STORE_PMP_TOR_NO_W);
    ai_micro_mab_retry_byte4_exact_store_prepare_supervisor_translation();
    ai_micro_mab_retry_byte4_exact_store_set_leaf(
        low_page, PTE_V | PTE_AD | PTE_RWX);
    ai_micro_mab_retry_byte4_exact_store_set_leaf(
        high_page, PTE_V | PTE_AD | PTE_RWX);
    sfence_vma();

    TEST_SETUP_EXCEPT();
    ai_micro_mab_retry_byte4_exact_store_translated_sd(
        fault_vaddr, first_fault_store_value);
    TEST_ASSERT("for the no-H same-page cross-16B translated scalar store, the first repeated 4B+4B second-split write fault must still report store access fault on the true second-half start address before the retried-success then tail-adjacent-bridge-word follow-up store probe",
        excpt.triggered == true &&
        excpt.cause == CAUSE_SAF &&
        excpt.tval == second_half_vaddr
    );

    TEST_SETUP_EXCEPT();
    ai_micro_mab_retry_byte4_exact_store_translated_sd(
        fault_vaddr, first_fault_store_value);
    TEST_ASSERT("repeating the same no-H second-split 4B+4B write fault keeps the same precise metadata before the retried-success then tail-adjacent-bridge-word follow-up store probe",
        excpt.triggered == true &&
        excpt.cause == CAUSE_SAF &&
        excpt.tval == second_half_vaddr
    );

    ai_micro_mab_retry_byte4_exact_store_restore_pmp_full_access();

    TEST_SETUP_EXCEPT();
    ai_micro_mab_retry_byte4_exact_store_translated_sd(
        aligned_upper_store_vaddr, aligned_upper_store_value_first);
    TEST_ASSERT("after repair, the first same-boundary translated upper-half aligned store should complete cleanly before the refaulted retry-4B+4B-store then tail-adjacent-bridge-word follow-up store probe",
        excpt.triggered == false
    );

    TEST_SETUP_EXCEPT();
    ai_micro_mab_retry_byte4_exact_store_translated_sd(
        aligned_upper_store_vaddr, aligned_upper_store_value_second);
    TEST_ASSERT("after one upper-half aligned translated store has already completed on the repaired boundary, the second upper-half aligned translated store should stay trap-free before the refaulted retry-4B+4B-store then tail-adjacent-bridge-word follow-up store probe",
        excpt.triggered == false
    );

    TEST_SETUP_EXCEPT();
    ai_micro_mab_retry_byte4_exact_store_translated_sd(
        aligned_upper_store_vaddr, aligned_upper_store_value_third);
    TEST_ASSERT("after two upper-half aligned translated stores have already completed on the repaired boundary, the third upper-half aligned translated store should stay trap-free before the refaulted retry-4B+4B-store then tail-adjacent-bridge-word follow-up store probe",
        excpt.triggered == false
    );

    TEST_SETUP_EXCEPT();
    ai_micro_mab_retry_byte4_exact_store_translated_sd(
        aligned_upper_store_vaddr, aligned_upper_store_value_fourth);
    TEST_ASSERT("after three upper-half aligned translated stores have already completed on the repaired boundary, the fourth upper-half aligned translated store should stay trap-free and become the latest committed high-half payload before the refaulted retry-4B+4B-store then tail-adjacent-bridge-word follow-up store probe",
        excpt.triggered == false
    );

    ai_micro_mab_retry_byte4_exact_store_configure_pmp_window(
        deny_start, deny_end,
        AI_MICRO_MAB_RETRY_BYTE4_EXACT_STORE_PMP_TOR_NO_W);

    TEST_SETUP_EXCEPT();
    ai_micro_mab_retry_byte4_exact_store_translated_sd(
        fault_vaddr, refault_store_value);
    TEST_ASSERT("after four upper-half aligned translated stores have already completed on the repaired boundary, restoring the upper-half deny window and reissuing the original 4B+4B misaligned store should refresh the same precise second-split store access fault metadata before the tail-adjacent-bridge-word follow-up store probe",
        excpt.triggered == true &&
        excpt.cause == CAUSE_SAF &&
        excpt.tval == second_half_vaddr
    );

    ai_micro_mab_retry_byte4_exact_store_restore_pmp_full_access();

    TEST_SETUP_EXCEPT();
    ai_micro_mab_retry_byte4_exact_store_translated_sd(
        fault_vaddr, retry_4b4b_store_value);
    TEST_ASSERT("after the refault episode is repaired again, retrying the same 4B+4B translated store as a success store should clear stale exception metadata before the immediate tail-adjacent-bridge-word follow-up store",
        excpt.triggered == false
    );

    TEST_SETUP_EXCEPT();
    ai_micro_mab_retry_byte4_exact_store_translated_sw(
        followup_tail_adjacent_bridge_word_store_vaddr,
        followup_tail_adjacent_bridge_word_store_value);
    TEST_ASSERT("after the retried original-template 4B+4B store has already succeeded on the same boundary, the immediate translated tail-adjacent-bridge-word store should also stay trap-free and must not reuse the retired fake-crosspage exact 4-byte high-half word window beyond bytes10-11 while handing bytes12-13 to the new split tail halfword",
        excpt.triggered == false
    );

    ai_micro_mab_retry_byte4_exact_store_overlay_little_endian_bytes(
        expected_boundary, boundary_seed, 16U, 8U,
        aligned_upper_store_value_fourth, sizeof(uint64_t));
    ai_micro_mab_retry_byte4_exact_store_overlay_little_endian_bytes(
        expected_boundary, expected_boundary, 16U, 4U,
        retry_4b4b_store_value, sizeof(uint64_t));
    ai_micro_mab_retry_byte4_exact_store_overlay_little_endian_bytes(
        expected_boundary, expected_boundary, 16U, 10U,
        (uint64_t)followup_tail_adjacent_bridge_word_store_value,
        sizeof(uint32_t));

    TEST_ASSERT("after the prolonged upper-half producer, repaired refault, retried 4B+4B success store, and immediate tail-adjacent-bridge-word store, the final boundary bytes should preserve bytes4-9 from the retried image, refresh only bytes10-13, and keep bytes14-15 from the latest aligned upper-half image",
        ai_micro_mab_retry_byte4_exact_store_bytes_match(
            boundary_phys, expected_boundary, 16U)
    );

    TEST_ASSERT("guard regions around the same-page boundary should remain unchanged through the repeated 4B+4B fault, four upper-half aligned stores, refault, retried success store, and immediate tail-adjacent-bridge-word follow-up store sequence",
        ai_micro_mab_retry_byte4_exact_store_bytes_match(
            guard_before_phys, guard_before_seed, 8U) &&
        ai_micro_mab_retry_byte4_exact_store_bytes_match(
            guard_after_phys, guard_after_seed, 8U)
    );

    ai_micro_mab_retry_byte4_exact_store_reset_env();

    TEST_END(
        "ai_micro_m_samepage_cross16b_store_refault_upperx4_retry_4b4b_store_then_tail_adjacent_bridge_word_store_corner");
}

bool ai_micro_m_samepage_cross16b_store_refault_upperx4_retry_4b4b_store_then_exact_high_half_word_store_corner()
{
    TEST_START();

    static const uint8_t boundary_seed[16] = {
        0xf1, 0xe0, 0xdf, 0xce, 0xbd, 0xac, 0x9b, 0x8a,
        0x79, 0x68, 0x57, 0x46, 0x35, 0x24, 0x13, 0x02,
    };
    static const uint8_t guard_before_seed[8] = {
        0x45, 0x57, 0x69, 0x7b, 0x8d, 0x9f, 0xb1, 0xc3,
    };
    static const uint8_t guard_after_seed[8] = {
        0xe0, 0xf2, 0x04, 0x16, 0x28, 0x3a, 0x4c, 0x5e,
    };
    const enum test_page low_page = SWITCH1;
    const enum test_page high_page = SWITCH2;
    const uintptr_t page_phys = phys_page_base(low_page) + PAGE_SIZE - 16U;
    const uintptr_t page_vaddr = hs_page_base(low_page) + PAGE_SIZE - 16U;
    const uintptr_t boundary_phys = page_phys +
        AI_MICRO_MAB_RETRY_BYTE4_EXACT_STORE_BOUNDARY_BASE_OFFSET;
    const uintptr_t guard_before_phys = page_phys;
    const uintptr_t guard_after_phys = page_phys + 24U;
    const uintptr_t fault_vaddr = page_vaddr +
        AI_MICRO_MAB_RETRY_BYTE4_EXACT_STORE_CROSS_16B_SD_OFFSET;
    const uintptr_t second_half_vaddr = page_vaddr + 16U;
    const uintptr_t aligned_upper_store_vaddr = page_vaddr +
        AI_MICRO_MAB_RETRY_BYTE4_EXACT_STORE_BOUNDARY_BASE_OFFSET + 8U;
    const uintptr_t followup_exact_high_half_word_store_vaddr =
        aligned_upper_store_vaddr;
    const uintptr_t deny_start = boundary_phys + 8U;
    const uintptr_t deny_end = deny_start + PAGE_SIZE;
    const uint64_t first_fault_store_value = 0x0d1e2f3041526374ULL;
    const uint64_t refault_store_value = 0x8d9eafc0d1e2f304ULL;
    const uint64_t aligned_upper_store_value_first = 0xcadbecfd0e1f2031ULL;
    const uint64_t aligned_upper_store_value_second = 0x5364758697a8b9caULL;
    const uint64_t aligned_upper_store_value_third = 0xdbecfd0e1f203142ULL;
    const uint64_t aligned_upper_store_value_fourth = 0x64758697a8b9cadbULL;
    const uint64_t retry_4b4b_store_value = 0xf30415263748596aULL;
    const uint32_t followup_exact_high_half_word_store_value = 0x9f8e7d6cU;
    uint8_t expected_boundary[16];

    goto_priv(PRIV_M);
    ai_micro_mab_retry_byte4_exact_store_reset_env();
    ai_micro_mab_retry_byte4_exact_store_write_bytes(
        boundary_phys, boundary_seed, 16U);
    ai_micro_mab_retry_byte4_exact_store_write_bytes(
        guard_before_phys, guard_before_seed, 8U);
    ai_micro_mab_retry_byte4_exact_store_write_bytes(
        guard_after_phys, guard_after_seed, 8U);

    ai_micro_mab_retry_byte4_exact_store_configure_pmp_window(
        deny_start, deny_end,
        AI_MICRO_MAB_RETRY_BYTE4_EXACT_STORE_PMP_TOR_NO_W);
    ai_micro_mab_retry_byte4_exact_store_prepare_supervisor_translation();
    ai_micro_mab_retry_byte4_exact_store_set_leaf(
        low_page, PTE_V | PTE_AD | PTE_RWX);
    ai_micro_mab_retry_byte4_exact_store_set_leaf(
        high_page, PTE_V | PTE_AD | PTE_RWX);
    sfence_vma();

    TEST_SETUP_EXCEPT();
    ai_micro_mab_retry_byte4_exact_store_translated_sd(
        fault_vaddr, first_fault_store_value);
    TEST_ASSERT("for the no-H same-page cross-16B translated scalar store, the first repeated 4B+4B second-split write fault must still report store access fault on the true second-half start address before the retried-success then exact-high-half-word follow-up store probe",
        excpt.triggered == true &&
        excpt.cause == CAUSE_SAF &&
        excpt.tval == second_half_vaddr
    );

    TEST_SETUP_EXCEPT();
    ai_micro_mab_retry_byte4_exact_store_translated_sd(
        fault_vaddr, first_fault_store_value);
    TEST_ASSERT("repeating the same no-H second-split 4B+4B write fault keeps the same precise metadata before the retried-success then exact-high-half-word follow-up store probe",
        excpt.triggered == true &&
        excpt.cause == CAUSE_SAF &&
        excpt.tval == second_half_vaddr
    );

    ai_micro_mab_retry_byte4_exact_store_restore_pmp_full_access();

    TEST_SETUP_EXCEPT();
    ai_micro_mab_retry_byte4_exact_store_translated_sd(
        aligned_upper_store_vaddr, aligned_upper_store_value_first);
    TEST_ASSERT("after repair, the first same-boundary translated upper-half aligned store should complete cleanly before the refaulted retry-4B+4B-store then exact-high-half-word follow-up store probe",
        excpt.triggered == false
    );

    TEST_SETUP_EXCEPT();
    ai_micro_mab_retry_byte4_exact_store_translated_sd(
        aligned_upper_store_vaddr, aligned_upper_store_value_second);
    TEST_ASSERT("after one upper-half aligned translated store has already completed on the repaired boundary, the second upper-half aligned translated store should stay trap-free before the refaulted retry-4B+4B-store then exact-high-half-word follow-up store probe",
        excpt.triggered == false
    );

    TEST_SETUP_EXCEPT();
    ai_micro_mab_retry_byte4_exact_store_translated_sd(
        aligned_upper_store_vaddr, aligned_upper_store_value_third);
    TEST_ASSERT("after two upper-half aligned translated stores have already completed on the repaired boundary, the third upper-half aligned translated store should stay trap-free before the refaulted retry-4B+4B-store then exact-high-half-word follow-up store probe",
        excpt.triggered == false
    );

    TEST_SETUP_EXCEPT();
    ai_micro_mab_retry_byte4_exact_store_translated_sd(
        aligned_upper_store_vaddr, aligned_upper_store_value_fourth);
    TEST_ASSERT("after three upper-half aligned translated stores have already completed on the repaired boundary, the fourth upper-half aligned translated store should stay trap-free and become the latest committed high-half payload before the refaulted retry-4B+4B-store then exact-high-half-word follow-up store probe",
        excpt.triggered == false
    );

    ai_micro_mab_retry_byte4_exact_store_configure_pmp_window(
        deny_start, deny_end,
        AI_MICRO_MAB_RETRY_BYTE4_EXACT_STORE_PMP_TOR_NO_W);

    TEST_SETUP_EXCEPT();
    ai_micro_mab_retry_byte4_exact_store_translated_sd(
        fault_vaddr, refault_store_value);
    TEST_ASSERT("after four upper-half aligned translated stores have already completed on the repaired boundary, restoring the upper-half deny window and reissuing the original 4B+4B misaligned store should refresh the same precise second-split store access fault metadata before the exact-high-half-word follow-up store probe",
        excpt.triggered == true &&
        excpt.cause == CAUSE_SAF &&
        excpt.tval == second_half_vaddr
    );

    ai_micro_mab_retry_byte4_exact_store_restore_pmp_full_access();

    TEST_SETUP_EXCEPT();
    ai_micro_mab_retry_byte4_exact_store_translated_sd(
        fault_vaddr, retry_4b4b_store_value);
    TEST_ASSERT("after the refault episode is repaired again, retrying the same 4B+4B translated store as a success store should clear stale exception metadata before the immediate exact-high-half-word follow-up store",
        excpt.triggered == false
    );

    TEST_SETUP_EXCEPT();
    ai_micro_mab_retry_byte4_exact_store_translated_sw(
        followup_exact_high_half_word_store_vaddr,
        followup_exact_high_half_word_store_value);
    TEST_ASSERT("after the retried original-template 4B+4B store has already succeeded on the same boundary, the immediate translated exact-high-half-word store should also stay trap-free and must not reuse the retired fake-crosspage exact 4-byte high-half word window beyond bytes8-11",
        excpt.triggered == false
    );

    ai_micro_mab_retry_byte4_exact_store_overlay_little_endian_bytes(
        expected_boundary, boundary_seed, 16U, 8U,
        aligned_upper_store_value_fourth, sizeof(uint64_t));
    ai_micro_mab_retry_byte4_exact_store_overlay_little_endian_bytes(
        expected_boundary, expected_boundary, 16U, 4U,
        retry_4b4b_store_value, sizeof(uint64_t));
    ai_micro_mab_retry_byte4_exact_store_overlay_little_endian_bytes(
        expected_boundary, expected_boundary, 16U, 8U,
        (uint64_t)followup_exact_high_half_word_store_value,
        sizeof(uint32_t));

    TEST_ASSERT("after the prolonged upper-half producer, repaired refault, retried 4B+4B success store, and immediate exact-high-half-word store, the final boundary bytes should preserve bytes4-7 from the retried image, refresh only bytes8-11, and keep bytes12-15 from the latest aligned upper-half image",
        ai_micro_mab_retry_byte4_exact_store_bytes_match(
            boundary_phys, expected_boundary, 16U)
    );

    TEST_ASSERT("guard regions around the same-page boundary should remain unchanged through the repeated 4B+4B fault, four upper-half aligned stores, refault, retried success store, and immediate exact-high-half-word follow-up store sequence",
        ai_micro_mab_retry_byte4_exact_store_bytes_match(
            guard_before_phys, guard_before_seed, 8U) &&
        ai_micro_mab_retry_byte4_exact_store_bytes_match(
            guard_after_phys, guard_after_seed, 8U)
    );

    ai_micro_mab_retry_byte4_exact_store_reset_env();

    TEST_END(
        "ai_micro_m_samepage_cross16b_store_refault_upperx4_retry_4b4b_store_then_exact_high_half_word_store_corner");
}

bool ai_micro_m_samepage_cross16b_store_refault_upperx4_retry_4b4b_store_then_tail_last_bridge_word_store_corner()
{
    TEST_START();

    static const uint8_t boundary_seed[16] = {
        0x02, 0xf1, 0xe0, 0xdf, 0xce, 0xbd, 0xac, 0x9b,
        0x8a, 0x79, 0x68, 0x57, 0x46, 0x35, 0x24, 0x13,
    };
    static const uint8_t guard_before_seed[8] = {
        0x48, 0x5a, 0x6c, 0x7e, 0x90, 0xa2, 0xb4, 0xc6,
    };
    static const uint8_t guard_after_seed[8] = {
        0xe5, 0xf7, 0x09, 0x1b, 0x2d, 0x3f, 0x41, 0x53,
    };
    const enum test_page low_page = SWITCH1;
    const enum test_page high_page = SWITCH2;
    const uintptr_t page_phys = phys_page_base(low_page) + PAGE_SIZE - 16U;
    const uintptr_t page_vaddr = hs_page_base(low_page) + PAGE_SIZE - 16U;
    const uintptr_t boundary_phys = page_phys +
        AI_MICRO_MAB_RETRY_BYTE4_EXACT_STORE_BOUNDARY_BASE_OFFSET;
    const uintptr_t guard_before_phys = page_phys;
    const uintptr_t guard_after_phys = page_phys + 24U;
    const uintptr_t fault_vaddr = page_vaddr +
        AI_MICRO_MAB_RETRY_BYTE4_EXACT_STORE_CROSS_16B_SD_OFFSET;
    const uintptr_t second_half_vaddr = page_vaddr + 16U;
    const uintptr_t aligned_upper_store_vaddr = page_vaddr +
        AI_MICRO_MAB_RETRY_BYTE4_EXACT_STORE_BOUNDARY_BASE_OFFSET + 8U;
    const uintptr_t followup_tail_last_bridge_word_store_vaddr =
        aligned_upper_store_vaddr + 3U;
    const uintptr_t deny_start = boundary_phys + 8U;
    const uintptr_t deny_end = deny_start + PAGE_SIZE;
    const uint64_t first_fault_store_value = 0x1e2f304152637485ULL;
    const uint64_t refault_store_value = 0x9eafc0d1e2f30415ULL;
    const uint64_t aligned_upper_store_value_first = 0xdbecfd0e1f203142ULL;
    const uint64_t aligned_upper_store_value_second = 0x64758697a8b9cadbULL;
    const uint64_t aligned_upper_store_value_third = 0xecfd0e1f20314253ULL;
    const uint64_t aligned_upper_store_value_fourth = 0x758697a8b9cadbecULL;
    const uint64_t retry_4b4b_store_value = 0x0415263748596a7bULL;
    const uint32_t followup_tail_last_bridge_word_store_value = 0xaf9e8d7cU;
    uint8_t expected_boundary[16];

    goto_priv(PRIV_M);
    ai_micro_mab_retry_byte4_exact_store_reset_env();
    ai_micro_mab_retry_byte4_exact_store_write_bytes(
        boundary_phys, boundary_seed, 16U);
    ai_micro_mab_retry_byte4_exact_store_write_bytes(
        guard_before_phys, guard_before_seed, 8U);
    ai_micro_mab_retry_byte4_exact_store_write_bytes(
        guard_after_phys, guard_after_seed, 8U);

    ai_micro_mab_retry_byte4_exact_store_configure_pmp_window(
        deny_start, deny_end,
        AI_MICRO_MAB_RETRY_BYTE4_EXACT_STORE_PMP_TOR_NO_W);
    ai_micro_mab_retry_byte4_exact_store_prepare_supervisor_translation();
    ai_micro_mab_retry_byte4_exact_store_set_leaf(
        low_page, PTE_V | PTE_AD | PTE_RWX);
    ai_micro_mab_retry_byte4_exact_store_set_leaf(
        high_page, PTE_V | PTE_AD | PTE_RWX);
    sfence_vma();

    TEST_SETUP_EXCEPT();
    ai_micro_mab_retry_byte4_exact_store_translated_sd(
        fault_vaddr, first_fault_store_value);
    TEST_ASSERT("for the no-H same-page cross-16B translated scalar store, the first repeated 4B+4B second-split write fault must still report store access fault on the true second-half start address before the retried-success then tail-last-bridge-word follow-up store probe",
        excpt.triggered == true &&
        excpt.cause == CAUSE_SAF &&
        excpt.tval == second_half_vaddr
    );

    TEST_SETUP_EXCEPT();
    ai_micro_mab_retry_byte4_exact_store_translated_sd(
        fault_vaddr, first_fault_store_value);
    TEST_ASSERT("repeating the same no-H second-split 4B+4B write fault keeps the same precise metadata before the retried-success then tail-last-bridge-word follow-up store probe",
        excpt.triggered == true &&
        excpt.cause == CAUSE_SAF &&
        excpt.tval == second_half_vaddr
    );

    ai_micro_mab_retry_byte4_exact_store_restore_pmp_full_access();

    TEST_SETUP_EXCEPT();
    ai_micro_mab_retry_byte4_exact_store_translated_sd(
        aligned_upper_store_vaddr, aligned_upper_store_value_first);
    TEST_ASSERT("after repair, the first same-boundary translated upper-half aligned store should complete cleanly before the refaulted retry-4B+4B-store then tail-last-bridge-word follow-up store probe",
        excpt.triggered == false
    );

    TEST_SETUP_EXCEPT();
    ai_micro_mab_retry_byte4_exact_store_translated_sd(
        aligned_upper_store_vaddr, aligned_upper_store_value_second);
    TEST_ASSERT("after one upper-half aligned translated store has already completed on the repaired boundary, the second upper-half aligned translated store should stay trap-free before the refaulted retry-4B+4B-store then tail-last-bridge-word follow-up store probe",
        excpt.triggered == false
    );

    TEST_SETUP_EXCEPT();
    ai_micro_mab_retry_byte4_exact_store_translated_sd(
        aligned_upper_store_vaddr, aligned_upper_store_value_third);
    TEST_ASSERT("after two upper-half aligned translated stores have already completed on the repaired boundary, the third upper-half aligned translated store should stay trap-free before the refaulted retry-4B+4B-store then tail-last-bridge-word follow-up store probe",
        excpt.triggered == false
    );

    TEST_SETUP_EXCEPT();
    ai_micro_mab_retry_byte4_exact_store_translated_sd(
        aligned_upper_store_vaddr, aligned_upper_store_value_fourth);
    TEST_ASSERT("after three upper-half aligned translated stores have already completed on the repaired boundary, the fourth upper-half aligned translated store should stay trap-free and become the latest committed high-half payload before the refaulted retry-4B+4B-store then tail-last-bridge-word follow-up store probe",
        excpt.triggered == false
    );

    ai_micro_mab_retry_byte4_exact_store_configure_pmp_window(
        deny_start, deny_end,
        AI_MICRO_MAB_RETRY_BYTE4_EXACT_STORE_PMP_TOR_NO_W);

    TEST_SETUP_EXCEPT();
    ai_micro_mab_retry_byte4_exact_store_translated_sd(
        fault_vaddr, refault_store_value);
    TEST_ASSERT("after four upper-half aligned translated stores have already completed on the repaired boundary, restoring the upper-half deny window and reissuing the original 4B+4B misaligned store should refresh the same precise second-split store access fault metadata before the tail-last-bridge-word follow-up store probe",
        excpt.triggered == true &&
        excpt.cause == CAUSE_SAF &&
        excpt.tval == second_half_vaddr
    );

    ai_micro_mab_retry_byte4_exact_store_restore_pmp_full_access();

    TEST_SETUP_EXCEPT();
    ai_micro_mab_retry_byte4_exact_store_translated_sd(
        fault_vaddr, retry_4b4b_store_value);
    TEST_ASSERT("after the refault episode is repaired again, retrying the same 4B+4B translated store as a success store should clear stale exception metadata before the immediate tail-last-bridge-word follow-up store",
        excpt.triggered == false
    );

    TEST_SETUP_EXCEPT();
    ai_micro_mab_retry_byte4_exact_store_translated_sw(
        followup_tail_last_bridge_word_store_vaddr,
        followup_tail_last_bridge_word_store_value);
    TEST_ASSERT("after the retried original-template 4B+4B store has already succeeded on the same boundary, the immediate translated tail-last-bridge-word store should also stay trap-free and must not reuse the retired fake-crosspage exact 4-byte high-half word window beyond byte11 while handing bytes12-14 to the new split tail word",
        excpt.triggered == false
    );

    ai_micro_mab_retry_byte4_exact_store_overlay_little_endian_bytes(
        expected_boundary, boundary_seed, 16U, 8U,
        aligned_upper_store_value_fourth, sizeof(uint64_t));
    ai_micro_mab_retry_byte4_exact_store_overlay_little_endian_bytes(
        expected_boundary, expected_boundary, 16U, 4U,
        retry_4b4b_store_value, sizeof(uint64_t));
    ai_micro_mab_retry_byte4_exact_store_overlay_little_endian_bytes(
        expected_boundary, expected_boundary, 16U, 11U,
        (uint64_t)followup_tail_last_bridge_word_store_value,
        sizeof(uint32_t));

    TEST_ASSERT("after the prolonged upper-half producer, repaired refault, retried 4B+4B success store, and immediate tail-last-bridge-word store, the final boundary bytes should preserve bytes4-10 from the retried image, refresh only bytes11-14, and keep byte15 from the latest aligned upper-half image",
        ai_micro_mab_retry_byte4_exact_store_bytes_match(
            boundary_phys, expected_boundary, 16U)
    );

    TEST_ASSERT("guard regions around the same-page boundary should remain unchanged through the repeated 4B+4B fault, four upper-half aligned stores, refault, retried success store, and immediate tail-last-bridge-word follow-up store sequence",
        ai_micro_mab_retry_byte4_exact_store_bytes_match(
            guard_before_phys, guard_before_seed, 8U) &&
        ai_micro_mab_retry_byte4_exact_store_bytes_match(
            guard_after_phys, guard_after_seed, 8U)
    );

    ai_micro_mab_retry_byte4_exact_store_reset_env();

    TEST_END(
        "ai_micro_m_samepage_cross16b_store_refault_upperx4_retry_4b4b_store_then_tail_last_bridge_word_store_corner");
}

bool ai_micro_m_samepage_cross16b_store_refault_upperx4_retry_4b4b_store_then_tail_last_bridge_halfword_store_corner()
{
    TEST_START();

    static const uint8_t boundary_seed[16] = {
        0x13, 0x02, 0xf1, 0xe0, 0xdf, 0xce, 0xbd, 0xac,
        0x9b, 0x8a, 0x79, 0x68, 0x57, 0x46, 0x35, 0x24,
    };
    static const uint8_t guard_before_seed[8] = {
        0x4b, 0x5d, 0x6f, 0x81, 0x93, 0xa5, 0xb7, 0xc9,
    };
    static const uint8_t guard_after_seed[8] = {
        0xe8, 0xfa, 0x0c, 0x1e, 0x30, 0x42, 0x54, 0x66,
    };
    const enum test_page low_page = SWITCH1;
    const enum test_page high_page = SWITCH2;
    const uintptr_t page_phys = phys_page_base(low_page) + PAGE_SIZE - 16U;
    const uintptr_t page_vaddr = hs_page_base(low_page) + PAGE_SIZE - 16U;
    const uintptr_t boundary_phys = page_phys +
        AI_MICRO_MAB_RETRY_BYTE4_EXACT_STORE_BOUNDARY_BASE_OFFSET;
    const uintptr_t guard_before_phys = page_phys;
    const uintptr_t guard_after_phys = page_phys + 24U;
    const uintptr_t fault_vaddr = page_vaddr +
        AI_MICRO_MAB_RETRY_BYTE4_EXACT_STORE_CROSS_16B_SD_OFFSET;
    const uintptr_t second_half_vaddr = page_vaddr + 16U;
    const uintptr_t aligned_upper_store_vaddr = page_vaddr +
        AI_MICRO_MAB_RETRY_BYTE4_EXACT_STORE_BOUNDARY_BASE_OFFSET + 8U;
    const uintptr_t followup_tail_last_bridge_halfword_store_vaddr =
        aligned_upper_store_vaddr + 3U;
    const uintptr_t deny_start = boundary_phys + 8U;
    const uintptr_t deny_end = deny_start + PAGE_SIZE;
    const uint64_t first_fault_store_value = 0x2031425364758697ULL;
    const uint64_t refault_store_value = 0xa0b1c2d3e4f50617ULL;
    const uint64_t aligned_upper_store_value_first = 0xdff0011223344556ULL;
    const uint64_t aligned_upper_store_value_second = 0x6778899aabbccddeULL;
    const uint64_t aligned_upper_store_value_third = 0xef00112233445566ULL;
    const uint64_t aligned_upper_store_value_fourth = 0x778899aabbccddeeULL;
    const uint64_t retry_4b4b_store_value = 0x061728394a5b6c7dULL;
    const uint16_t followup_tail_last_bridge_halfword_store_value = 0x8f7eU;
    uint8_t expected_boundary[16];

    goto_priv(PRIV_M);
    ai_micro_mab_retry_byte4_exact_store_reset_env();
    ai_micro_mab_retry_byte4_exact_store_write_bytes(
        boundary_phys, boundary_seed, 16U);
    ai_micro_mab_retry_byte4_exact_store_write_bytes(
        guard_before_phys, guard_before_seed, 8U);
    ai_micro_mab_retry_byte4_exact_store_write_bytes(
        guard_after_phys, guard_after_seed, 8U);

    ai_micro_mab_retry_byte4_exact_store_configure_pmp_window(
        deny_start, deny_end,
        AI_MICRO_MAB_RETRY_BYTE4_EXACT_STORE_PMP_TOR_NO_W);
    ai_micro_mab_retry_byte4_exact_store_prepare_supervisor_translation();
    ai_micro_mab_retry_byte4_exact_store_set_leaf(
        low_page, PTE_V | PTE_AD | PTE_RWX);
    ai_micro_mab_retry_byte4_exact_store_set_leaf(
        high_page, PTE_V | PTE_AD | PTE_RWX);
    sfence_vma();

    TEST_SETUP_EXCEPT();
    ai_micro_mab_retry_byte4_exact_store_translated_sd(
        fault_vaddr, first_fault_store_value);
    TEST_ASSERT("for the no-H same-page cross-16B translated scalar store, the first repeated 4B+4B second-split write fault must still report store access fault on the true second-half start address before the retried-success then tail-last-bridge-halfword follow-up store probe",
        excpt.triggered == true &&
        excpt.cause == CAUSE_SAF &&
        excpt.tval == second_half_vaddr
    );

    TEST_SETUP_EXCEPT();
    ai_micro_mab_retry_byte4_exact_store_translated_sd(
        fault_vaddr, first_fault_store_value);
    TEST_ASSERT("repeating the same no-H second-split 4B+4B write fault keeps the same precise metadata before the retried-success then tail-last-bridge-halfword follow-up store probe",
        excpt.triggered == true &&
        excpt.cause == CAUSE_SAF &&
        excpt.tval == second_half_vaddr
    );

    ai_micro_mab_retry_byte4_exact_store_restore_pmp_full_access();

    TEST_SETUP_EXCEPT();
    ai_micro_mab_retry_byte4_exact_store_translated_sd(
        aligned_upper_store_vaddr, aligned_upper_store_value_first);
    TEST_ASSERT("after repair, the first same-boundary translated upper-half aligned store should complete cleanly before the refaulted retry-4B+4B-store then tail-last-bridge-halfword follow-up store probe",
        excpt.triggered == false
    );

    TEST_SETUP_EXCEPT();
    ai_micro_mab_retry_byte4_exact_store_translated_sd(
        aligned_upper_store_vaddr, aligned_upper_store_value_second);
    TEST_ASSERT("after one upper-half aligned translated store has already completed on the repaired boundary, the second upper-half aligned translated store should stay trap-free before the refaulted retry-4B+4B-store then tail-last-bridge-halfword follow-up store probe",
        excpt.triggered == false
    );

    TEST_SETUP_EXCEPT();
    ai_micro_mab_retry_byte4_exact_store_translated_sd(
        aligned_upper_store_vaddr, aligned_upper_store_value_third);
    TEST_ASSERT("after two upper-half aligned translated stores have already completed on the repaired boundary, the third upper-half aligned translated store should stay trap-free before the refaulted retry-4B+4B-store then tail-last-bridge-halfword follow-up store probe",
        excpt.triggered == false
    );

    TEST_SETUP_EXCEPT();
    ai_micro_mab_retry_byte4_exact_store_translated_sd(
        aligned_upper_store_vaddr, aligned_upper_store_value_fourth);
    TEST_ASSERT("after three upper-half aligned translated stores have already completed on the repaired boundary, the fourth upper-half aligned translated store should stay trap-free and become the latest committed high-half payload before the refaulted retry-4B+4B-store then tail-last-bridge-halfword follow-up store probe",
        excpt.triggered == false
    );

    ai_micro_mab_retry_byte4_exact_store_configure_pmp_window(
        deny_start, deny_end,
        AI_MICRO_MAB_RETRY_BYTE4_EXACT_STORE_PMP_TOR_NO_W);

    TEST_SETUP_EXCEPT();
    ai_micro_mab_retry_byte4_exact_store_translated_sd(
        fault_vaddr, refault_store_value);
    TEST_ASSERT("after four upper-half aligned translated stores have already completed on the repaired boundary, restoring the upper-half deny window and reissuing the original 4B+4B misaligned store should refresh the same precise second-split store access fault metadata before the tail-last-bridge-halfword follow-up store probe",
        excpt.triggered == true &&
        excpt.cause == CAUSE_SAF &&
        excpt.tval == second_half_vaddr
    );

    ai_micro_mab_retry_byte4_exact_store_restore_pmp_full_access();

    TEST_SETUP_EXCEPT();
    ai_micro_mab_retry_byte4_exact_store_translated_sd(
        fault_vaddr, retry_4b4b_store_value);
    TEST_ASSERT("after the refault episode is repaired again, retrying the same 4B+4B translated store as a success store should clear stale exception metadata before the immediate tail-last-bridge-halfword follow-up store",
        excpt.triggered == false
    );

    TEST_SETUP_EXCEPT();
    ai_micro_mab_retry_byte4_exact_store_translated_sh(
        followup_tail_last_bridge_halfword_store_vaddr,
        followup_tail_last_bridge_halfword_store_value);
    TEST_ASSERT("after the retried original-template 4B+4B store has already succeeded on the same boundary, the immediate translated tail-last-bridge-halfword store should also stay trap-free and must not reuse the retired fake-crosspage exact 4-byte high-half word window outside bytes11-12 while handing byte12 to the new producer-side byte",
        excpt.triggered == false
    );

    ai_micro_mab_retry_byte4_exact_store_overlay_little_endian_bytes(
        expected_boundary, boundary_seed, 16U, 8U,
        aligned_upper_store_value_fourth, sizeof(uint64_t));
    ai_micro_mab_retry_byte4_exact_store_overlay_little_endian_bytes(
        expected_boundary, expected_boundary, 16U, 4U,
        retry_4b4b_store_value, sizeof(uint64_t));
    ai_micro_mab_retry_byte4_exact_store_overlay_little_endian_bytes(
        expected_boundary, expected_boundary, 16U, 11U,
        (uint64_t)followup_tail_last_bridge_halfword_store_value,
        sizeof(uint16_t));

    TEST_ASSERT("after the prolonged upper-half producer, repaired refault, retried 4B+4B success store, and immediate tail-last-bridge-halfword store, the final boundary bytes should preserve bytes4-10 from the retried image, refresh only bytes11-12, and keep bytes13-15 from the latest aligned upper-half image",
        ai_micro_mab_retry_byte4_exact_store_bytes_match(
            boundary_phys, expected_boundary, 16U)
    );

    TEST_ASSERT("guard regions around the same-page boundary should remain unchanged through the repeated 4B+4B fault, four upper-half aligned stores, refault, retried success store, and immediate tail-last-bridge-halfword follow-up store sequence",
        ai_micro_mab_retry_byte4_exact_store_bytes_match(
            guard_before_phys, guard_before_seed, 8U) &&
        ai_micro_mab_retry_byte4_exact_store_bytes_match(
            guard_after_phys, guard_after_seed, 8U)
    );

    ai_micro_mab_retry_byte4_exact_store_reset_env();

    TEST_END(
        "ai_micro_m_samepage_cross16b_store_refault_upperx4_retry_4b4b_store_then_tail_last_bridge_halfword_store_corner");
}

bool ai_micro_m_samepage_cross16b_store_refault_upperx4_retry_4b4b_store_then_same_template_store_corner()
{
    TEST_START();

    static const uint8_t boundary_seed[16] = {
        0x24, 0x13, 0x02, 0xf1, 0xe0, 0xdf, 0xce, 0xbd,
        0xac, 0x9b, 0x8a, 0x79, 0x68, 0x57, 0x46, 0x35,
    };
    static const uint8_t guard_before_seed[8] = {
        0x52, 0x64, 0x76, 0x88, 0x9a, 0xac, 0xbe, 0xd0,
    };
    static const uint8_t guard_after_seed[8] = {
        0xf1, 0x03, 0x15, 0x27, 0x39, 0x4b, 0x5d, 0x6f,
    };
    const enum test_page low_page = SWITCH1;
    const enum test_page high_page = SWITCH2;
    const uintptr_t page_phys = phys_page_base(low_page) + PAGE_SIZE - 16U;
    const uintptr_t page_vaddr = hs_page_base(low_page) + PAGE_SIZE - 16U;
    const uintptr_t boundary_phys = page_phys +
        AI_MICRO_MAB_RETRY_BYTE4_EXACT_STORE_BOUNDARY_BASE_OFFSET;
    const uintptr_t guard_before_phys = page_phys;
    const uintptr_t guard_after_phys = page_phys + 24U;
    const uintptr_t fault_vaddr = page_vaddr +
        AI_MICRO_MAB_RETRY_BYTE4_EXACT_STORE_CROSS_16B_SD_OFFSET;
    const uintptr_t second_half_vaddr = page_vaddr + 16U;
    const uintptr_t aligned_upper_store_vaddr = page_vaddr +
        AI_MICRO_MAB_RETRY_BYTE4_EXACT_STORE_BOUNDARY_BASE_OFFSET + 8U;
    const uintptr_t deny_start = boundary_phys + 8U;
    const uintptr_t deny_end = deny_start + PAGE_SIZE;
    const uint64_t first_fault_store_value = 0x31425364758697a8ULL;
    const uint64_t refault_store_value = 0xb1c2d3e4f5061728ULL;
    const uint64_t aligned_upper_store_value_first = 0xe0f1021324354657ULL;
    const uint64_t aligned_upper_store_value_second = 0x68798a9bacbdcedfULL;
    const uint64_t aligned_upper_store_value_third = 0xf001122334455667ULL;
    const uint64_t aligned_upper_store_value_fourth = 0x78899aabbccddeefULL;
    const uint64_t retry_same_template_store_value = 0x1728394a5b6c7d8eULL;
    const uint64_t followup_same_template_store_value = 0x9fb0c1d2e3f40516ULL;
    uint8_t expected_boundary[16];

    goto_priv(PRIV_M);
    ai_micro_mab_retry_byte4_exact_store_reset_env();
    ai_micro_mab_retry_byte4_exact_store_write_bytes(
        boundary_phys, boundary_seed, 16U);
    ai_micro_mab_retry_byte4_exact_store_write_bytes(
        guard_before_phys, guard_before_seed, 8U);
    ai_micro_mab_retry_byte4_exact_store_write_bytes(
        guard_after_phys, guard_after_seed, 8U);

    ai_micro_mab_retry_byte4_exact_store_configure_pmp_window(
        deny_start, deny_end,
        AI_MICRO_MAB_RETRY_BYTE4_EXACT_STORE_PMP_TOR_NO_W);
    ai_micro_mab_retry_byte4_exact_store_prepare_supervisor_translation();
    ai_micro_mab_retry_byte4_exact_store_set_leaf(
        low_page, PTE_V | PTE_AD | PTE_RWX);
    ai_micro_mab_retry_byte4_exact_store_set_leaf(
        high_page, PTE_V | PTE_AD | PTE_RWX);
    sfence_vma();

    TEST_SETUP_EXCEPT();
    ai_micro_mab_retry_byte4_exact_store_translated_sd(
        fault_vaddr, first_fault_store_value);
    TEST_ASSERT("for the no-H same-page cross-16B translated scalar store, the first repeated 4B+4B second-split write fault must still report store access fault on the true second-half start address before the retried-success then same-template follow-up store probe",
        excpt.triggered == true &&
        excpt.cause == CAUSE_SAF &&
        excpt.tval == second_half_vaddr
    );

    TEST_SETUP_EXCEPT();
    ai_micro_mab_retry_byte4_exact_store_translated_sd(
        fault_vaddr, first_fault_store_value);
    TEST_ASSERT("repeating the same no-H second-split 4B+4B write fault keeps the same precise metadata before the retried-success then same-template follow-up store probe",
        excpt.triggered == true &&
        excpt.cause == CAUSE_SAF &&
        excpt.tval == second_half_vaddr
    );

    ai_micro_mab_retry_byte4_exact_store_restore_pmp_full_access();

    TEST_SETUP_EXCEPT();
    ai_micro_mab_retry_byte4_exact_store_translated_sd(
        aligned_upper_store_vaddr, aligned_upper_store_value_first);
    TEST_ASSERT("after repair, the first same-boundary translated upper-half aligned store should complete cleanly before the refaulted retry-4B+4B-store then same-template follow-up store probe",
        excpt.triggered == false
    );

    TEST_SETUP_EXCEPT();
    ai_micro_mab_retry_byte4_exact_store_translated_sd(
        aligned_upper_store_vaddr, aligned_upper_store_value_second);
    TEST_ASSERT("after one upper-half aligned translated store has already completed on the repaired boundary, the second upper-half aligned translated store should stay trap-free before the refaulted retry-4B+4B-store then same-template follow-up store probe",
        excpt.triggered == false
    );

    TEST_SETUP_EXCEPT();
    ai_micro_mab_retry_byte4_exact_store_translated_sd(
        aligned_upper_store_vaddr, aligned_upper_store_value_third);
    TEST_ASSERT("after two upper-half aligned translated stores have already completed on the repaired boundary, the third upper-half aligned translated store should stay trap-free before the refaulted retry-4B+4B-store then same-template follow-up store probe",
        excpt.triggered == false
    );

    TEST_SETUP_EXCEPT();
    ai_micro_mab_retry_byte4_exact_store_translated_sd(
        aligned_upper_store_vaddr, aligned_upper_store_value_fourth);
    TEST_ASSERT("after three upper-half aligned translated stores have already completed on the repaired boundary, the fourth upper-half aligned translated store should stay trap-free and become the latest committed high-half payload before the refaulted retry-4B+4B-store then same-template follow-up store probe",
        excpt.triggered == false
    );

    ai_micro_mab_retry_byte4_exact_store_configure_pmp_window(
        deny_start, deny_end,
        AI_MICRO_MAB_RETRY_BYTE4_EXACT_STORE_PMP_TOR_NO_W);

    TEST_SETUP_EXCEPT();
    ai_micro_mab_retry_byte4_exact_store_translated_sd(
        fault_vaddr, refault_store_value);
    TEST_ASSERT("after four upper-half aligned translated stores have already completed on the repaired boundary, restoring the upper-half deny window and reissuing the original 4B+4B misaligned store should refresh the same precise second-split store access fault metadata before the same-template follow-up store probe",
        excpt.triggered == true &&
        excpt.cause == CAUSE_SAF &&
        excpt.tval == second_half_vaddr
    );

    ai_micro_mab_retry_byte4_exact_store_restore_pmp_full_access();

    TEST_SETUP_EXCEPT();
    ai_micro_mab_retry_byte4_exact_store_translated_sd(
        fault_vaddr, retry_same_template_store_value);
    TEST_ASSERT("after the refault episode is repaired again, retrying the same 4B+4B translated store as a success store should clear stale exception metadata before the immediate same-template follow-up store",
        excpt.triggered == false
    );

    TEST_SETUP_EXCEPT();
    ai_micro_mab_retry_byte4_exact_store_translated_sd(
        fault_vaddr, followup_same_template_store_value);
    TEST_ASSERT("after the retried original-template 4B+4B store has already succeeded on the same boundary, the immediate same-address same-template translated store should also stay trap-free instead of inheriting fake-crosspage or stale exact-BYTE4 split-store state",
        excpt.triggered == false
    );

    ai_micro_mab_retry_byte4_exact_store_overlay_little_endian_bytes(
        expected_boundary, boundary_seed, 16U, 8U,
        aligned_upper_store_value_fourth, sizeof(uint64_t));
    ai_micro_mab_retry_byte4_exact_store_overlay_little_endian_bytes(
        expected_boundary, expected_boundary, 16U, 4U,
        retry_same_template_store_value, sizeof(uint64_t));
    ai_micro_mab_retry_byte4_exact_store_overlay_little_endian_bytes(
        expected_boundary, expected_boundary, 16U, 4U,
        followup_same_template_store_value, sizeof(uint64_t));

    TEST_ASSERT("after the prolonged upper-half producer, repaired refault, retried 4B+4B success store, and immediate same-template follow-up store, the final boundary bytes should match only the latest same-template 4B+4B overlay plus the latest high-half residue",
        ai_micro_mab_retry_byte4_exact_store_bytes_match(
            boundary_phys, expected_boundary, 16U)
    );

    TEST_ASSERT("guard regions around the same-page boundary should remain unchanged through the repeated 4B+4B fault, four upper-half aligned stores, refault, retried success store, and immediate same-template follow-up store sequence",
        ai_micro_mab_retry_byte4_exact_store_bytes_match(
            guard_before_phys, guard_before_seed, 8U) &&
        ai_micro_mab_retry_byte4_exact_store_bytes_match(
            guard_after_phys, guard_after_seed, 8U)
    );

    ai_micro_mab_retry_byte4_exact_store_reset_env();

    TEST_END(
        "ai_micro_m_samepage_cross16b_store_refault_upperx4_retry_4b4b_store_then_same_template_store_corner");
}

bool ai_micro_m_samepage_cross16b_store_refault_upperx4_retry_4b4b_store_then_pure_producer_word_store_corner()
{
    TEST_START();

    static const uint8_t boundary_seed[16] = {
        0x91, 0x80, 0x7f, 0x6e, 0x5d, 0x4c, 0x3b, 0x2a,
        0x19, 0x08, 0xf7, 0xe6, 0xd5, 0xc4, 0xb3, 0xa2,
    };
    static const uint8_t guard_before_seed[8] = {
        0x13, 0x24, 0x35, 0x46, 0x57, 0x68, 0x79, 0x8a,
    };
    static const uint8_t guard_after_seed[8] = {
        0x9b, 0xac, 0xbd, 0xce, 0xdf, 0xe1, 0xf2, 0x03,
    };
    const enum test_page low_page = SWITCH1;
    const enum test_page high_page = SWITCH2;
    const uintptr_t page_phys = phys_page_base(low_page) + PAGE_SIZE - 16U;
    const uintptr_t page_vaddr = hs_page_base(low_page) + PAGE_SIZE - 16U;
    const uintptr_t boundary_phys = page_phys +
        AI_MICRO_MAB_RETRY_BYTE4_EXACT_STORE_BOUNDARY_BASE_OFFSET;
    const uintptr_t guard_before_phys = page_phys;
    const uintptr_t guard_after_phys = page_phys + 24U;
    const uintptr_t fault_vaddr = page_vaddr +
        AI_MICRO_MAB_RETRY_BYTE4_EXACT_STORE_CROSS_16B_SD_OFFSET;
    const uintptr_t second_half_vaddr = page_vaddr + 16U;
    const uintptr_t aligned_upper_store_vaddr = page_vaddr +
        AI_MICRO_MAB_RETRY_BYTE4_EXACT_STORE_BOUNDARY_BASE_OFFSET + 8U;
    const uintptr_t followup_pure_producer_word_store_vaddr =
        aligned_upper_store_vaddr + 4U;
    const uintptr_t deny_start = boundary_phys + 8U;
    const uintptr_t deny_end = deny_start + PAGE_SIZE;
    const uint64_t first_fault_store_value = 0x1021324354657687ULL;
    const uint64_t refault_store_value = 0x98a9bacbdcedfe0fULL;
    const uint64_t aligned_upper_store_value_first = 0x2233445566778899ULL;
    const uint64_t aligned_upper_store_value_second = 0xaabbccddeeff1021ULL;
    const uint64_t aligned_upper_store_value_third = 0x31425364758697a8ULL;
    const uint64_t aligned_upper_store_value_fourth = 0xb9cadbecfd0e1f20ULL;
    const uint64_t retry_4b4b_store_value = 0x405162738495a6b7ULL;
    const uint32_t followup_pure_producer_word_store_value = 0x1a2b3c4dU;
    uint8_t expected_boundary[16];

    goto_priv(PRIV_M);
    ai_micro_mab_retry_byte4_exact_store_reset_env();
    ai_micro_mab_retry_byte4_exact_store_write_bytes(
        boundary_phys, boundary_seed, 16U);
    ai_micro_mab_retry_byte4_exact_store_write_bytes(
        guard_before_phys, guard_before_seed, 8U);
    ai_micro_mab_retry_byte4_exact_store_write_bytes(
        guard_after_phys, guard_after_seed, 8U);

    ai_micro_mab_retry_byte4_exact_store_configure_pmp_window(
        deny_start, deny_end,
        AI_MICRO_MAB_RETRY_BYTE4_EXACT_STORE_PMP_TOR_NO_W);
    ai_micro_mab_retry_byte4_exact_store_prepare_supervisor_translation();
    ai_micro_mab_retry_byte4_exact_store_set_leaf(
        low_page, PTE_V | PTE_AD | PTE_RWX);
    ai_micro_mab_retry_byte4_exact_store_set_leaf(
        high_page, PTE_V | PTE_AD | PTE_RWX);
    sfence_vma();

    TEST_SETUP_EXCEPT();
    ai_micro_mab_retry_byte4_exact_store_translated_sd(
        fault_vaddr, first_fault_store_value);
    TEST_ASSERT("for the no-H same-page cross-16B translated scalar store, the first repeated 4B+4B second-split write fault must still report store access fault on the true second-half start address before the retried-success then pure-producer-word follow-up store probe",
        excpt.triggered == true &&
        excpt.cause == CAUSE_SAF &&
        excpt.tval == second_half_vaddr
    );

    TEST_SETUP_EXCEPT();
    ai_micro_mab_retry_byte4_exact_store_translated_sd(
        fault_vaddr, first_fault_store_value);
    TEST_ASSERT("repeating the same no-H second-split 4B+4B write fault keeps the same precise metadata before the retried-success then pure-producer-word follow-up store probe",
        excpt.triggered == true &&
        excpt.cause == CAUSE_SAF &&
        excpt.tval == second_half_vaddr
    );

    ai_micro_mab_retry_byte4_exact_store_restore_pmp_full_access();

    TEST_SETUP_EXCEPT();
    ai_micro_mab_retry_byte4_exact_store_translated_sd(
        aligned_upper_store_vaddr, aligned_upper_store_value_first);
    TEST_ASSERT("after repair, the first same-boundary translated upper-half aligned store should complete cleanly before the refaulted retry-4B+4B-store then pure-producer-word follow-up store probe",
        excpt.triggered == false
    );

    TEST_SETUP_EXCEPT();
    ai_micro_mab_retry_byte4_exact_store_translated_sd(
        aligned_upper_store_vaddr, aligned_upper_store_value_second);
    TEST_ASSERT("after one upper-half aligned translated store has already completed on the repaired boundary, the second upper-half aligned translated store should stay trap-free before the refaulted retry-4B+4B-store then pure-producer-word follow-up store probe",
        excpt.triggered == false
    );

    TEST_SETUP_EXCEPT();
    ai_micro_mab_retry_byte4_exact_store_translated_sd(
        aligned_upper_store_vaddr, aligned_upper_store_value_third);
    TEST_ASSERT("after two upper-half aligned translated stores have already completed on the repaired boundary, the third upper-half aligned translated store should stay trap-free before the refaulted retry-4B+4B-store then pure-producer-word follow-up store probe",
        excpt.triggered == false
    );

    TEST_SETUP_EXCEPT();
    ai_micro_mab_retry_byte4_exact_store_translated_sd(
        aligned_upper_store_vaddr, aligned_upper_store_value_fourth);
    TEST_ASSERT("after three upper-half aligned translated stores have already completed on the repaired boundary, the fourth upper-half aligned translated store should stay trap-free and become the latest committed high-half payload before the refaulted retry-4B+4B-store then pure-producer-word follow-up store probe",
        excpt.triggered == false
    );

    ai_micro_mab_retry_byte4_exact_store_configure_pmp_window(
        deny_start, deny_end,
        AI_MICRO_MAB_RETRY_BYTE4_EXACT_STORE_PMP_TOR_NO_W);

    TEST_SETUP_EXCEPT();
    ai_micro_mab_retry_byte4_exact_store_translated_sd(
        fault_vaddr, refault_store_value);
    TEST_ASSERT("after four upper-half aligned translated stores have already completed on the repaired boundary, restoring the upper-half deny window and reissuing the original 4B+4B misaligned store should refresh the same precise second-split store access fault metadata before the pure-producer-word follow-up store probe",
        excpt.triggered == true &&
        excpt.cause == CAUSE_SAF &&
        excpt.tval == second_half_vaddr
    );

    ai_micro_mab_retry_byte4_exact_store_restore_pmp_full_access();

    TEST_SETUP_EXCEPT();
    ai_micro_mab_retry_byte4_exact_store_translated_sd(
        fault_vaddr, retry_4b4b_store_value);
    TEST_ASSERT("after the refault episode is repaired again, retrying the same 4B+4B translated store as a success store should clear stale exception metadata before the immediate pure-producer-word follow-up store",
        excpt.triggered == false
    );

    TEST_SETUP_EXCEPT();
    ai_micro_mab_retry_byte4_exact_store_translated_sw(
        followup_pure_producer_word_store_vaddr,
        followup_pure_producer_word_store_value);
    TEST_ASSERT("after the retried original-template 4B+4B store has already succeeded on the same boundary, the immediate translated pure-producer word store should also stay trap-free and must not let a retired fake-crosspage exact 4-byte high-half owner/template leak back into bytes8-11 while bytes12-15 are refreshed by the aligned producer-side word",
        excpt.triggered == false
    );

    ai_micro_mab_retry_byte4_exact_store_overlay_little_endian_bytes(
        expected_boundary, boundary_seed, 16U, 8U,
        aligned_upper_store_value_fourth, sizeof(uint64_t));
    ai_micro_mab_retry_byte4_exact_store_overlay_little_endian_bytes(
        expected_boundary, expected_boundary, 16U, 4U,
        retry_4b4b_store_value, sizeof(uint64_t));
    ai_micro_mab_retry_byte4_exact_store_overlay_little_endian_bytes(
        expected_boundary, expected_boundary, 16U, 12U,
        (uint64_t)followup_pure_producer_word_store_value,
        sizeof(uint32_t));

    TEST_ASSERT("after the prolonged upper-half producer, repaired refault, retried 4B+4B success store, and immediate pure-producer word store, the final boundary bytes should preserve bytes4-11 from the retried image and refresh only bytes12-15 with the new producer-side word",
        ai_micro_mab_retry_byte4_exact_store_bytes_match(
            boundary_phys, expected_boundary, 16U)
    );

    TEST_ASSERT("guard regions around the same-page boundary should remain unchanged through the repeated 4B+4B fault, four upper-half aligned stores, refault, retried success store, and immediate pure-producer-word follow-up store sequence",
        ai_micro_mab_retry_byte4_exact_store_bytes_match(
            guard_before_phys, guard_before_seed, 8U) &&
        ai_micro_mab_retry_byte4_exact_store_bytes_match(
            guard_after_phys, guard_after_seed, 8U)
    );

    ai_micro_mab_retry_byte4_exact_store_reset_env();

    TEST_END(
        "ai_micro_m_samepage_cross16b_store_refault_upperx4_retry_4b4b_store_then_pure_producer_word_store_corner");
}

bool ai_micro_m_samepage_cross16b_store_refault_upperx4_retry_4b4b_store_then_pure_producer_first_byte_store_corner()
{
    TEST_START();

    static const uint8_t boundary_seed[16] = {
        0xd2, 0xc1, 0xb0, 0xaf, 0x9e, 0x8d, 0x7c, 0x6b,
        0x5a, 0x49, 0x38, 0x27, 0x16, 0x05, 0xf4, 0xe3,
    };
    static const uint8_t guard_before_seed[8] = {
        0x11, 0x23, 0x35, 0x47, 0x59, 0x6b, 0x7d, 0x8f,
    };
    static const uint8_t guard_after_seed[8] = {
        0x90, 0xa2, 0xb4, 0xc6, 0xd8, 0xea, 0xfc, 0x0e,
    };
    const enum test_page low_page = SWITCH1;
    const enum test_page high_page = SWITCH2;
    const uintptr_t page_phys = phys_page_base(low_page) + PAGE_SIZE - 16U;
    const uintptr_t page_vaddr = hs_page_base(low_page) + PAGE_SIZE - 16U;
    const uintptr_t boundary_phys = page_phys +
        AI_MICRO_MAB_RETRY_BYTE4_EXACT_STORE_BOUNDARY_BASE_OFFSET;
    const uintptr_t guard_before_phys = page_phys;
    const uintptr_t guard_after_phys = page_phys + 24U;
    const uintptr_t fault_vaddr = page_vaddr +
        AI_MICRO_MAB_RETRY_BYTE4_EXACT_STORE_CROSS_16B_SD_OFFSET;
    const uintptr_t second_half_vaddr = page_vaddr + 16U;
    const uintptr_t aligned_upper_store_vaddr = page_vaddr +
        AI_MICRO_MAB_RETRY_BYTE4_EXACT_STORE_BOUNDARY_BASE_OFFSET + 8U;
    const uintptr_t followup_pure_producer_first_byte_store_vaddr =
        aligned_upper_store_vaddr + 4U;
    const uintptr_t deny_start = boundary_phys + 8U;
    const uintptr_t deny_end = deny_start + PAGE_SIZE;
    const uint64_t first_fault_store_value = 0x2132435465768798ULL;
    const uint64_t refault_store_value = 0xa9bacbdcedfe0f10ULL;
    const uint64_t aligned_upper_store_value_first = 0x32435465768798a9ULL;
    const uint64_t aligned_upper_store_value_second = 0xbacbdcedfe0f1021ULL;
    const uint64_t aligned_upper_store_value_third = 0x435465768798a9baULL;
    const uint64_t aligned_upper_store_value_fourth = 0xcbdcedfe0f102132ULL;
    const uint64_t retry_4b4b_store_value = 0x5465768798a9bacbULL;
    const uint8_t followup_pure_producer_first_byte_store_value = 0x3dU;
    uint8_t expected_boundary[16];

    goto_priv(PRIV_M);
    ai_micro_mab_retry_byte4_exact_store_reset_env();
    ai_micro_mab_retry_byte4_exact_store_write_bytes(
        boundary_phys, boundary_seed, 16U);
    ai_micro_mab_retry_byte4_exact_store_write_bytes(
        guard_before_phys, guard_before_seed, 8U);
    ai_micro_mab_retry_byte4_exact_store_write_bytes(
        guard_after_phys, guard_after_seed, 8U);

    ai_micro_mab_retry_byte4_exact_store_configure_pmp_window(
        deny_start, deny_end,
        AI_MICRO_MAB_RETRY_BYTE4_EXACT_STORE_PMP_TOR_NO_W);
    ai_micro_mab_retry_byte4_exact_store_prepare_supervisor_translation();
    ai_micro_mab_retry_byte4_exact_store_set_leaf(
        low_page, PTE_V | PTE_AD | PTE_RWX);
    ai_micro_mab_retry_byte4_exact_store_set_leaf(
        high_page, PTE_V | PTE_AD | PTE_RWX);
    sfence_vma();

    TEST_SETUP_EXCEPT();
    ai_micro_mab_retry_byte4_exact_store_translated_sd(
        fault_vaddr, first_fault_store_value);
    TEST_ASSERT("for the no-H same-page cross-16B translated scalar store, the first repeated 4B+4B second-split write fault must still report store access fault on the true second-half start address before the retried-success then pure-producer-first-byte follow-up store probe",
        excpt.triggered == true &&
        excpt.cause == CAUSE_SAF &&
        excpt.tval == second_half_vaddr
    );

    TEST_SETUP_EXCEPT();
    ai_micro_mab_retry_byte4_exact_store_translated_sd(
        fault_vaddr, first_fault_store_value);
    TEST_ASSERT("repeating the same no-H second-split 4B+4B write fault keeps the same precise metadata before the retried-success then pure-producer-first-byte follow-up store probe",
        excpt.triggered == true &&
        excpt.cause == CAUSE_SAF &&
        excpt.tval == second_half_vaddr
    );

    ai_micro_mab_retry_byte4_exact_store_restore_pmp_full_access();

    TEST_SETUP_EXCEPT();
    ai_micro_mab_retry_byte4_exact_store_translated_sd(
        aligned_upper_store_vaddr, aligned_upper_store_value_first);
    TEST_ASSERT("after repair, the first same-boundary translated upper-half aligned store should complete cleanly before the refaulted retry-4B+4B-store then pure-producer-first-byte follow-up store probe",
        excpt.triggered == false
    );

    TEST_SETUP_EXCEPT();
    ai_micro_mab_retry_byte4_exact_store_translated_sd(
        aligned_upper_store_vaddr, aligned_upper_store_value_second);
    TEST_ASSERT("after one upper-half aligned translated store has already completed on the repaired boundary, the second upper-half aligned translated store should stay trap-free before the refaulted retry-4B+4B-store then pure-producer-first-byte follow-up store probe",
        excpt.triggered == false
    );

    TEST_SETUP_EXCEPT();
    ai_micro_mab_retry_byte4_exact_store_translated_sd(
        aligned_upper_store_vaddr, aligned_upper_store_value_third);
    TEST_ASSERT("after two upper-half aligned translated stores have already completed on the repaired boundary, the third upper-half aligned translated store should stay trap-free before the refaulted retry-4B+4B-store then pure-producer-first-byte follow-up store probe",
        excpt.triggered == false
    );

    TEST_SETUP_EXCEPT();
    ai_micro_mab_retry_byte4_exact_store_translated_sd(
        aligned_upper_store_vaddr, aligned_upper_store_value_fourth);
    TEST_ASSERT("after three upper-half aligned translated stores have already completed on the repaired boundary, the fourth upper-half aligned translated store should stay trap-free and become the latest committed high-half payload before the refaulted retry-4B+4B-store then pure-producer-first-byte follow-up store probe",
        excpt.triggered == false
    );

    ai_micro_mab_retry_byte4_exact_store_configure_pmp_window(
        deny_start, deny_end,
        AI_MICRO_MAB_RETRY_BYTE4_EXACT_STORE_PMP_TOR_NO_W);

    TEST_SETUP_EXCEPT();
    ai_micro_mab_retry_byte4_exact_store_translated_sd(
        fault_vaddr, refault_store_value);
    TEST_ASSERT("after four upper-half aligned translated stores have already completed on the repaired boundary, restoring the upper-half deny window and reissuing the original 4B+4B misaligned store should refresh the same precise second-split store access fault metadata before the pure-producer-first-byte follow-up store probe",
        excpt.triggered == true &&
        excpt.cause == CAUSE_SAF &&
        excpt.tval == second_half_vaddr
    );

    ai_micro_mab_retry_byte4_exact_store_restore_pmp_full_access();

    TEST_SETUP_EXCEPT();
    ai_micro_mab_retry_byte4_exact_store_translated_sd(
        fault_vaddr, retry_4b4b_store_value);
    TEST_ASSERT("after the refault episode is repaired again, retrying the same 4B+4B translated store as a success store should clear stale exception metadata before the immediate pure-producer-first-byte follow-up store",
        excpt.triggered == false
    );

    TEST_SETUP_EXCEPT();
    ai_micro_mab_retry_byte4_exact_store_translated_sb(
        followup_pure_producer_first_byte_store_vaddr,
        followup_pure_producer_first_byte_store_value);
    TEST_ASSERT("after the retried original-template 4B+4B store has already succeeded on the same boundary, the immediate translated pure-producer first-byte store should also stay trap-free and must not let a retired fake-crosspage exact 4-byte high-half owner/template leak back into bytes8-11 or widen beyond byte12 while the new aligned producer-side byte lands",
        excpt.triggered == false
    );

    ai_micro_mab_retry_byte4_exact_store_overlay_little_endian_bytes(
        expected_boundary, boundary_seed, 16U, 8U,
        aligned_upper_store_value_fourth, sizeof(uint64_t));
    ai_micro_mab_retry_byte4_exact_store_overlay_little_endian_bytes(
        expected_boundary, expected_boundary, 16U, 4U,
        retry_4b4b_store_value, sizeof(uint64_t));
    ai_micro_mab_retry_byte4_exact_store_overlay_little_endian_bytes(
        expected_boundary, expected_boundary, 16U, 12U,
        (uint64_t)followup_pure_producer_first_byte_store_value,
        sizeof(uint8_t));

    TEST_ASSERT("after the prolonged upper-half producer, repaired refault, retried 4B+4B success store, and immediate pure-producer first-byte store, the final boundary bytes should preserve bytes4-11 from the retried image, refresh only byte12, and keep bytes13-15 from the latest aligned upper-half image",
        ai_micro_mab_retry_byte4_exact_store_bytes_match(
            boundary_phys, expected_boundary, 16U)
    );

    TEST_ASSERT("guard regions around the same-page boundary should remain unchanged through the repeated 4B+4B fault, four upper-half aligned stores, refault, retried success store, and immediate pure-producer-first-byte follow-up store sequence",
        ai_micro_mab_retry_byte4_exact_store_bytes_match(
            guard_before_phys, guard_before_seed, 8U) &&
        ai_micro_mab_retry_byte4_exact_store_bytes_match(
            guard_after_phys, guard_after_seed, 8U)
    );

    ai_micro_mab_retry_byte4_exact_store_reset_env();

    TEST_END(
        "ai_micro_m_samepage_cross16b_store_refault_upperx4_retry_4b4b_store_then_pure_producer_first_byte_store_corner");
}
