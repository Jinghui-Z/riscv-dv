#include "ai_micro_mmode_memblock_retry_followup_byte4_exact_store_common.h"

bool ai_micro_m_samepage_cross16b_store_refault_upperx4_retry_4b4b_store_then_pure_producer_first_halfword_store_corner()
{
    TEST_START();

    static const uint8_t boundary_seed[16] = {
        0xe1, 0xd0, 0xcf, 0xbe, 0xad, 0x9c, 0x8b, 0x7a,
        0x69, 0x58, 0x47, 0x36, 0x25, 0x14, 0x03, 0xf2,
    };
    static const uint8_t guard_before_seed[8] = {
        0x12, 0x24, 0x36, 0x48, 0x5a, 0x6c, 0x7e, 0x80,
    };
    static const uint8_t guard_after_seed[8] = {
        0x91, 0xa3, 0xb5, 0xc7, 0xd9, 0xeb, 0xfd, 0x0f,
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
    const uintptr_t followup_pure_producer_first_halfword_store_vaddr =
        aligned_upper_store_vaddr + 4U;
    const uintptr_t deny_start = boundary_phys + 8U;
    const uintptr_t deny_end = deny_start + PAGE_SIZE;
    const uint64_t first_fault_store_value = 0x32435465768798a9ULL;
    const uint64_t refault_store_value = 0xcadbecfd0e1f2031ULL;
    const uint64_t aligned_upper_store_value_first = 0x435465768798a9baULL;
    const uint64_t aligned_upper_store_value_second = 0xdbecfd0e1f203142ULL;
    const uint64_t aligned_upper_store_value_third = 0x5465768798a9bacbULL;
    const uint64_t aligned_upper_store_value_fourth = 0xecfd0e1f20314253ULL;
    const uint64_t retry_4b4b_store_value = 0x65768798a9bacbdcULL;
    const uint16_t followup_pure_producer_first_halfword_store_value = 0x5e4fU;
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
    TEST_ASSERT("for the no-H same-page cross-16B translated scalar store, the first repeated 4B+4B second-split write fault must still report store access fault on the true second-half start address before the retried-success then pure-producer-first-halfword follow-up store probe",
        excpt.triggered == true &&
        excpt.cause == CAUSE_SAF &&
        excpt.tval == second_half_vaddr
    );

    TEST_SETUP_EXCEPT();
    ai_micro_mab_retry_byte4_exact_store_translated_sd(
        fault_vaddr, first_fault_store_value);
    TEST_ASSERT("repeating the same no-H second-split 4B+4B write fault keeps the same precise metadata before the retried-success then pure-producer-first-halfword follow-up store probe",
        excpt.triggered == true &&
        excpt.cause == CAUSE_SAF &&
        excpt.tval == second_half_vaddr
    );

    ai_micro_mab_retry_byte4_exact_store_restore_pmp_full_access();

    TEST_SETUP_EXCEPT();
    ai_micro_mab_retry_byte4_exact_store_translated_sd(
        aligned_upper_store_vaddr, aligned_upper_store_value_first);
    TEST_ASSERT("after repair, the first same-boundary translated upper-half aligned store should complete cleanly before the refaulted retry-4B+4B-store then pure-producer-first-halfword follow-up store probe",
        excpt.triggered == false
    );

    TEST_SETUP_EXCEPT();
    ai_micro_mab_retry_byte4_exact_store_translated_sd(
        aligned_upper_store_vaddr, aligned_upper_store_value_second);
    TEST_ASSERT("after one upper-half aligned translated store has already completed on the repaired boundary, the second upper-half aligned translated store should stay trap-free before the refaulted retry-4B+4B-store then pure-producer-first-halfword follow-up store probe",
        excpt.triggered == false
    );

    TEST_SETUP_EXCEPT();
    ai_micro_mab_retry_byte4_exact_store_translated_sd(
        aligned_upper_store_vaddr, aligned_upper_store_value_third);
    TEST_ASSERT("after two upper-half aligned translated stores have already completed on the repaired boundary, the third upper-half aligned translated store should stay trap-free before the refaulted retry-4B+4B-store then pure-producer-first-halfword follow-up store probe",
        excpt.triggered == false
    );

    TEST_SETUP_EXCEPT();
    ai_micro_mab_retry_byte4_exact_store_translated_sd(
        aligned_upper_store_vaddr, aligned_upper_store_value_fourth);
    TEST_ASSERT("after three upper-half aligned translated stores have already completed on the repaired boundary, the fourth upper-half aligned translated store should stay trap-free and become the latest committed high-half payload before the refaulted retry-4B+4B-store then pure-producer-first-halfword follow-up store probe",
        excpt.triggered == false
    );

    ai_micro_mab_retry_byte4_exact_store_configure_pmp_window(
        deny_start, deny_end,
        AI_MICRO_MAB_RETRY_BYTE4_EXACT_STORE_PMP_TOR_NO_W);

    TEST_SETUP_EXCEPT();
    ai_micro_mab_retry_byte4_exact_store_translated_sd(
        fault_vaddr, refault_store_value);
    TEST_ASSERT("after four upper-half aligned translated stores have already completed on the repaired boundary, restoring the upper-half deny window and reissuing the original 4B+4B misaligned store should refresh the same precise second-split store access fault metadata before the pure-producer-first-halfword follow-up store probe",
        excpt.triggered == true &&
        excpt.cause == CAUSE_SAF &&
        excpt.tval == second_half_vaddr
    );

    ai_micro_mab_retry_byte4_exact_store_restore_pmp_full_access();

    TEST_SETUP_EXCEPT();
    ai_micro_mab_retry_byte4_exact_store_translated_sd(
        fault_vaddr, retry_4b4b_store_value);
    TEST_ASSERT("after the refault episode is repaired again, retrying the same 4B+4B translated store as a success store should clear stale exception metadata before the immediate pure-producer-first-halfword follow-up store",
        excpt.triggered == false
    );

    TEST_SETUP_EXCEPT();
    ai_micro_mab_retry_byte4_exact_store_translated_sh(
        followup_pure_producer_first_halfword_store_vaddr,
        followup_pure_producer_first_halfword_store_value);
    TEST_ASSERT("after the retried original-template 4B+4B store has already succeeded on the same boundary, the immediate translated pure-producer first-halfword store should also stay trap-free and must not let a retired fake-crosspage exact 4-byte high-half owner/template leak back into bytes8-11 or widen beyond bytes12-13 while the new aligned producer-side halfword lands",
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
        (uint64_t)followup_pure_producer_first_halfword_store_value,
        sizeof(uint16_t));

    TEST_ASSERT("after the prolonged upper-half producer, repaired refault, retried 4B+4B success store, and immediate pure-producer first-halfword store, the final boundary bytes should preserve bytes4-11 from the retried image, refresh only bytes12-13, and keep bytes14-15 from the latest aligned upper-half image",
        ai_micro_mab_retry_byte4_exact_store_bytes_match(
            boundary_phys, expected_boundary, 16U)
    );

    TEST_ASSERT("guard regions around the same-page boundary should remain unchanged through the repeated 4B+4B fault, four upper-half aligned stores, refault, retried success store, and immediate pure-producer-first-halfword follow-up store sequence",
        ai_micro_mab_retry_byte4_exact_store_bytes_match(
            guard_before_phys, guard_before_seed, 8U) &&
        ai_micro_mab_retry_byte4_exact_store_bytes_match(
            guard_after_phys, guard_after_seed, 8U)
    );

    ai_micro_mab_retry_byte4_exact_store_reset_env();

    TEST_END(
        "ai_micro_m_samepage_cross16b_store_refault_upperx4_retry_4b4b_store_then_pure_producer_first_halfword_store_corner");
}

bool ai_micro_m_samepage_cross16b_store_refault_upperx4_retry_4b4b_store_then_pure_producer_tail_halfword_store_corner()
{
    TEST_START();

    static const uint8_t boundary_seed[16] = {
        0xf0, 0xef, 0xde, 0xcd, 0xbc, 0xab, 0x9a, 0x89,
        0x78, 0x67, 0x56, 0x45, 0x34, 0x23, 0x12, 0x01,
    };
    static const uint8_t guard_before_seed[8] = {
        0x10, 0x22, 0x34, 0x46, 0x58, 0x6a, 0x7c, 0x8e,
    };
    static const uint8_t guard_after_seed[8] = {
        0x9f, 0xb1, 0xc3, 0xd5, 0xe7, 0xf9, 0x0b, 0x1d,
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
    const uintptr_t followup_pure_producer_tail_halfword_store_vaddr =
        aligned_upper_store_vaddr + 6U;
    const uintptr_t deny_start = boundary_phys + 8U;
    const uintptr_t deny_end = deny_start + PAGE_SIZE;
    const uint64_t first_fault_store_value = 0x435465768798a9baULL;
    const uint64_t refault_store_value = 0xdbecfd0e1f203142ULL;
    const uint64_t aligned_upper_store_value_first = 0x5465768798a9bacbULL;
    const uint64_t aligned_upper_store_value_second = 0xecfd0e1f20314253ULL;
    const uint64_t aligned_upper_store_value_third = 0x65768798a9bacbdcULL;
    const uint64_t aligned_upper_store_value_fourth = 0xfd0e1f2031425364ULL;
    const uint64_t retry_4b4b_store_value = 0x768798a9bacbdcedULL;
    const uint16_t followup_pure_producer_tail_halfword_store_value = 0x7f6eU;
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
    TEST_ASSERT("for the no-H same-page cross-16B translated scalar store, the first repeated 4B+4B second-split write fault must still report store access fault on the true second-half start address before the retried-success then pure-producer-tail-halfword follow-up store probe",
        excpt.triggered == true &&
        excpt.cause == CAUSE_SAF &&
        excpt.tval == second_half_vaddr
    );

    TEST_SETUP_EXCEPT();
    ai_micro_mab_retry_byte4_exact_store_translated_sd(
        fault_vaddr, first_fault_store_value);
    TEST_ASSERT("repeating the same no-H second-split 4B+4B write fault keeps the same precise metadata before the retried-success then pure-producer-tail-halfword follow-up store probe",
        excpt.triggered == true &&
        excpt.cause == CAUSE_SAF &&
        excpt.tval == second_half_vaddr
    );

    ai_micro_mab_retry_byte4_exact_store_restore_pmp_full_access();

    TEST_SETUP_EXCEPT();
    ai_micro_mab_retry_byte4_exact_store_translated_sd(
        aligned_upper_store_vaddr, aligned_upper_store_value_first);
    TEST_ASSERT("after repair, the first same-boundary translated upper-half aligned store should complete cleanly before the refaulted retry-4B+4B-store then pure-producer-tail-halfword follow-up store probe",
        excpt.triggered == false
    );

    TEST_SETUP_EXCEPT();
    ai_micro_mab_retry_byte4_exact_store_translated_sd(
        aligned_upper_store_vaddr, aligned_upper_store_value_second);
    TEST_ASSERT("after one upper-half aligned translated store has already completed on the repaired boundary, the second upper-half aligned translated store should stay trap-free before the refaulted retry-4B+4B-store then pure-producer-tail-halfword follow-up store probe",
        excpt.triggered == false
    );

    TEST_SETUP_EXCEPT();
    ai_micro_mab_retry_byte4_exact_store_translated_sd(
        aligned_upper_store_vaddr, aligned_upper_store_value_third);
    TEST_ASSERT("after two upper-half aligned translated stores have already completed on the repaired boundary, the third upper-half aligned translated store should stay trap-free before the refaulted retry-4B+4B-store then pure-producer-tail-halfword follow-up store probe",
        excpt.triggered == false
    );

    TEST_SETUP_EXCEPT();
    ai_micro_mab_retry_byte4_exact_store_translated_sd(
        aligned_upper_store_vaddr, aligned_upper_store_value_fourth);
    TEST_ASSERT("after three upper-half aligned translated stores have already completed on the repaired boundary, the fourth upper-half aligned translated store should stay trap-free and become the latest committed high-half payload before the refaulted retry-4B+4B-store then pure-producer-tail-halfword follow-up store probe",
        excpt.triggered == false
    );

    ai_micro_mab_retry_byte4_exact_store_configure_pmp_window(
        deny_start, deny_end,
        AI_MICRO_MAB_RETRY_BYTE4_EXACT_STORE_PMP_TOR_NO_W);

    TEST_SETUP_EXCEPT();
    ai_micro_mab_retry_byte4_exact_store_translated_sd(
        fault_vaddr, refault_store_value);
    TEST_ASSERT("after four upper-half aligned translated stores have already completed on the repaired boundary, restoring the upper-half deny window and reissuing the original 4B+4B misaligned store should refresh the same precise second-split store access fault metadata before the pure-producer-tail-halfword follow-up store probe",
        excpt.triggered == true &&
        excpt.cause == CAUSE_SAF &&
        excpt.tval == second_half_vaddr
    );

    ai_micro_mab_retry_byte4_exact_store_restore_pmp_full_access();

    TEST_SETUP_EXCEPT();
    ai_micro_mab_retry_byte4_exact_store_translated_sd(
        fault_vaddr, retry_4b4b_store_value);
    TEST_ASSERT("after the refault episode is repaired again, retrying the same 4B+4B translated store as a success store should clear stale exception metadata before the immediate pure-producer-tail-halfword follow-up store",
        excpt.triggered == false
    );

    TEST_SETUP_EXCEPT();
    ai_micro_mab_retry_byte4_exact_store_translated_sh(
        followup_pure_producer_tail_halfword_store_vaddr,
        followup_pure_producer_tail_halfword_store_value);
    TEST_ASSERT("after the retried original-template 4B+4B store has already succeeded on the same boundary, the immediate translated pure-producer tail-halfword store should also stay trap-free and must not let a retired fake-crosspage exact 4-byte high-half owner/template leak back into bytes8-11 or widen beyond bytes14-15 while the new aligned producer-side tail halfword lands",
        excpt.triggered == false
    );

    ai_micro_mab_retry_byte4_exact_store_overlay_little_endian_bytes(
        expected_boundary, boundary_seed, 16U, 8U,
        aligned_upper_store_value_fourth, sizeof(uint64_t));
    ai_micro_mab_retry_byte4_exact_store_overlay_little_endian_bytes(
        expected_boundary, expected_boundary, 16U, 4U,
        retry_4b4b_store_value, sizeof(uint64_t));
    ai_micro_mab_retry_byte4_exact_store_overlay_little_endian_bytes(
        expected_boundary, expected_boundary, 16U, 14U,
        (uint64_t)followup_pure_producer_tail_halfword_store_value,
        sizeof(uint16_t));

    TEST_ASSERT("after the prolonged upper-half producer, repaired refault, retried 4B+4B success store, and immediate pure-producer tail-halfword store, the final boundary bytes should preserve bytes4-11 from the retried image, keep bytes12-13 from the latest aligned upper-half image, and refresh only bytes14-15",
        ai_micro_mab_retry_byte4_exact_store_bytes_match(
            boundary_phys, expected_boundary, 16U)
    );

    TEST_ASSERT("guard regions around the same-page boundary should remain unchanged through the repeated 4B+4B fault, four upper-half aligned stores, refault, retried success store, and immediate pure-producer-tail-halfword follow-up store sequence",
        ai_micro_mab_retry_byte4_exact_store_bytes_match(
            guard_before_phys, guard_before_seed, 8U) &&
        ai_micro_mab_retry_byte4_exact_store_bytes_match(
            guard_after_phys, guard_after_seed, 8U)
    );

    ai_micro_mab_retry_byte4_exact_store_reset_env();

    TEST_END(
        "ai_micro_m_samepage_cross16b_store_refault_upperx4_retry_4b4b_store_then_pure_producer_tail_halfword_store_corner");
}

bool ai_micro_m_samepage_cross16b_store_refault_upperx4_retry_4b4b_store_then_pure_producer_tail_byte_store_corner()
{
    TEST_START();

    static const uint8_t boundary_seed[16] = {
        0x1f, 0x2e, 0x3d, 0x4c, 0x5b, 0x6a, 0x79, 0x88,
        0x97, 0xa6, 0xb5, 0xc4, 0xd3, 0xe2, 0xf1, 0x10,
    };
    static const uint8_t guard_before_seed[8] = {
        0x21, 0x43, 0x65, 0x87, 0xa9, 0xcb, 0xed, 0x0f,
    };
    static const uint8_t guard_after_seed[8] = {
        0xf0, 0xde, 0xbc, 0x9a, 0x78, 0x56, 0x34, 0x12,
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
    const uintptr_t followup_pure_producer_tail_byte_store_vaddr =
        aligned_upper_store_vaddr + 7U;
    const uintptr_t deny_start = boundary_phys + 8U;
    const uintptr_t deny_end = deny_start + PAGE_SIZE;
    const uint64_t first_fault_store_value = 0x65768798a9bacbdcULL;
    const uint64_t refault_store_value = 0xfe0f203142536475ULL;
    const uint64_t aligned_upper_store_value_first = 0x768798a9bacbdcedULL;
    const uint64_t aligned_upper_store_value_second = 0x0e1f203142536475ULL;
    const uint64_t aligned_upper_store_value_third = 0x8798a9bacbdcedfeULL;
    const uint64_t aligned_upper_store_value_fourth = 0x1f20314253647586ULL;
    const uint64_t retry_4b4b_store_value = 0x98a9bacbdcedfe0fULL;
    const uint8_t followup_pure_producer_tail_byte_store_value = 0x7cU;
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
    TEST_ASSERT("for the no-H same-page cross-16B translated scalar store, the first repeated 4B+4B second-split write fault must still report store access fault on the true second-half start address before the retried-success then pure-producer-tail-byte follow-up store probe",
        excpt.triggered == true &&
        excpt.cause == CAUSE_SAF &&
        excpt.tval == second_half_vaddr
    );

    TEST_SETUP_EXCEPT();
    ai_micro_mab_retry_byte4_exact_store_translated_sd(
        fault_vaddr, first_fault_store_value);
    TEST_ASSERT("repeating the same no-H second-split 4B+4B write fault keeps the same precise metadata before the retried-success then pure-producer-tail-byte follow-up store probe",
        excpt.triggered == true &&
        excpt.cause == CAUSE_SAF &&
        excpt.tval == second_half_vaddr
    );

    ai_micro_mab_retry_byte4_exact_store_restore_pmp_full_access();

    TEST_SETUP_EXCEPT();
    ai_micro_mab_retry_byte4_exact_store_translated_sd(
        aligned_upper_store_vaddr, aligned_upper_store_value_first);
    TEST_ASSERT("after repair, the first same-boundary translated upper-half aligned store should complete cleanly before the refaulted retry-4B+4B-store then pure-producer-tail-byte follow-up store probe",
        excpt.triggered == false
    );

    TEST_SETUP_EXCEPT();
    ai_micro_mab_retry_byte4_exact_store_translated_sd(
        aligned_upper_store_vaddr, aligned_upper_store_value_second);
    TEST_ASSERT("after one upper-half aligned translated store has already completed on the repaired boundary, the second upper-half aligned translated store should stay trap-free before the refaulted retry-4B+4B-store then pure-producer-tail-byte follow-up store probe",
        excpt.triggered == false
    );

    TEST_SETUP_EXCEPT();
    ai_micro_mab_retry_byte4_exact_store_translated_sd(
        aligned_upper_store_vaddr, aligned_upper_store_value_third);
    TEST_ASSERT("after two upper-half aligned translated stores have already completed on the repaired boundary, the third upper-half aligned translated store should stay trap-free before the refaulted retry-4B+4B-store then pure-producer-tail-byte follow-up store probe",
        excpt.triggered == false
    );

    TEST_SETUP_EXCEPT();
    ai_micro_mab_retry_byte4_exact_store_translated_sd(
        aligned_upper_store_vaddr, aligned_upper_store_value_fourth);
    TEST_ASSERT("after three upper-half aligned translated stores have already completed on the repaired boundary, the fourth upper-half aligned translated store should stay trap-free and become the latest committed high-half payload before the refaulted retry-4B+4B-store then pure-producer-tail-byte follow-up store probe",
        excpt.triggered == false
    );

    ai_micro_mab_retry_byte4_exact_store_configure_pmp_window(
        deny_start, deny_end,
        AI_MICRO_MAB_RETRY_BYTE4_EXACT_STORE_PMP_TOR_NO_W);

    TEST_SETUP_EXCEPT();
    ai_micro_mab_retry_byte4_exact_store_translated_sd(
        fault_vaddr, refault_store_value);
    TEST_ASSERT("after four upper-half aligned translated stores have already completed on the repaired boundary, restoring the upper-half deny window and reissuing the original 4B+4B misaligned store should refresh the same precise second-split store access fault metadata before the pure-producer-tail-byte follow-up store probe",
        excpt.triggered == true &&
        excpt.cause == CAUSE_SAF &&
        excpt.tval == second_half_vaddr
    );

    ai_micro_mab_retry_byte4_exact_store_restore_pmp_full_access();

    TEST_SETUP_EXCEPT();
    ai_micro_mab_retry_byte4_exact_store_translated_sd(
        fault_vaddr, retry_4b4b_store_value);
    TEST_ASSERT("after the refault episode is repaired again, retrying the same 4B+4B translated store as a success store should clear stale exception metadata before the immediate pure-producer-tail-byte follow-up store",
        excpt.triggered == false
    );

    TEST_SETUP_EXCEPT();
    ai_micro_mab_retry_byte4_exact_store_translated_sb(
        followup_pure_producer_tail_byte_store_vaddr,
        followup_pure_producer_tail_byte_store_value);
    TEST_ASSERT("after the retried original-template 4B+4B store has already succeeded on the same boundary, the immediate translated pure-producer tail-byte store should also stay trap-free and must not let a retired fake-crosspage exact 4-byte high-half owner/template leak back into bytes8-11 or widen beyond byte15 while bytes12-14 stay owned by the latest aligned producer-side image",
        excpt.triggered == false
    );

    ai_micro_mab_retry_byte4_exact_store_overlay_little_endian_bytes(
        expected_boundary, boundary_seed, 16U, 8U,
        aligned_upper_store_value_fourth, sizeof(uint64_t));
    ai_micro_mab_retry_byte4_exact_store_overlay_little_endian_bytes(
        expected_boundary, expected_boundary, 16U, 4U,
        retry_4b4b_store_value, sizeof(uint64_t));
    ai_micro_mab_retry_byte4_exact_store_overlay_little_endian_bytes(
        expected_boundary, expected_boundary, 16U, 15U,
        (uint64_t)followup_pure_producer_tail_byte_store_value,
        sizeof(uint8_t));

    TEST_ASSERT("after the prolonged upper-half producer, repaired refault, retried 4B+4B success store, and immediate pure-producer tail-byte store, the final boundary bytes should preserve bytes4-11 from the retried image, keep bytes12-14 from the latest aligned upper-half image, and refresh only byte15",
        ai_micro_mab_retry_byte4_exact_store_bytes_match(
            boundary_phys, expected_boundary, 16U)
    );

    TEST_ASSERT("guard regions around the same-page boundary should remain unchanged through the repeated 4B+4B fault, four upper-half aligned stores, refault, retried success store, and immediate pure-producer-tail-byte follow-up store sequence",
        ai_micro_mab_retry_byte4_exact_store_bytes_match(
            guard_before_phys, guard_before_seed, 8U) &&
        ai_micro_mab_retry_byte4_exact_store_bytes_match(
            guard_after_phys, guard_after_seed, 8U)
    );

    ai_micro_mab_retry_byte4_exact_store_reset_env();

    TEST_END(
        "ai_micro_m_samepage_cross16b_store_refault_upperx4_retry_4b4b_store_then_pure_producer_tail_byte_store_corner");
}

bool ai_micro_m_samepage_cross16b_store_refault_upperx4_retry_4b4b_store_then_pure_producer_tail_byte_store_then_upper_half_load_corner()
{
    TEST_START();

    static const uint8_t boundary_seed[16] = {
        0x2f, 0x1e, 0x0d, 0xfc, 0xeb, 0xda, 0xc9, 0xb8,
        0xa7, 0x96, 0x85, 0x74, 0x63, 0x52, 0x41, 0x30,
    };
    static const uint8_t guard_before_seed[8] = {
        0x14, 0x28, 0x3c, 0x50, 0x64, 0x78, 0x8c, 0xa0,
    };
    static const uint8_t guard_after_seed[8] = {
        0xb1, 0xc5, 0xd9, 0xed, 0xf1, 0x05, 0x19, 0x2d,
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
    const uintptr_t followup_pure_producer_tail_byte_store_vaddr =
        aligned_upper_store_vaddr + 7U;
    const uintptr_t upper_half_load_vaddr = aligned_upper_store_vaddr;
    const uintptr_t deny_start = boundary_phys + 8U;
    const uintptr_t deny_end = deny_start + PAGE_SIZE;
    const uint64_t first_fault_store_value = 0x5465768798a9bacbULL;
    const uint64_t refault_store_value = 0xecfd0e1f20314253ULL;
    const uint64_t aligned_upper_store_value_first = 0x65768798a9bacbdcULL;
    const uint64_t aligned_upper_store_value_second = 0xfd0e1f2031425364ULL;
    const uint64_t aligned_upper_store_value_third = 0x768798a9bacbdcedULL;
    const uint64_t aligned_upper_store_value_fourth = 0x0e1f203142536475ULL;
    const uint64_t retry_4b4b_store_value = 0x8798a9bacbdcedfeULL;
    const uint8_t followup_pure_producer_tail_byte_store_value = 0x6dU;
    uint8_t expected_boundary[16];
    uint64_t expected_upper_half_load_value = 0ULL;
    uint64_t observed_upper_half_load_value = 0ULL;

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
    TEST_ASSERT("for the no-H same-page cross-16B translated scalar store, the first repeated 4B+4B second-split write fault must still report store access fault on the true second-half start address before the retried-success then pure-producer-tail-byte store plus upper-half load probe",
        excpt.triggered == true &&
        excpt.cause == CAUSE_SAF &&
        excpt.tval == second_half_vaddr
    );

    TEST_SETUP_EXCEPT();
    ai_micro_mab_retry_byte4_exact_store_translated_sd(
        fault_vaddr, first_fault_store_value);
    TEST_ASSERT("repeating the same no-H second-split 4B+4B write fault keeps the same precise metadata before the retried-success then pure-producer-tail-byte store plus upper-half load probe",
        excpt.triggered == true &&
        excpt.cause == CAUSE_SAF &&
        excpt.tval == second_half_vaddr
    );

    ai_micro_mab_retry_byte4_exact_store_restore_pmp_full_access();

    TEST_SETUP_EXCEPT();
    ai_micro_mab_retry_byte4_exact_store_translated_sd(
        aligned_upper_store_vaddr, aligned_upper_store_value_first);
    TEST_ASSERT("after repair, the first same-boundary translated upper-half aligned store should complete cleanly before the refaulted retry-4B+4B-store then pure-producer-tail-byte store plus upper-half load probe",
        excpt.triggered == false
    );

    TEST_SETUP_EXCEPT();
    ai_micro_mab_retry_byte4_exact_store_translated_sd(
        aligned_upper_store_vaddr, aligned_upper_store_value_second);
    TEST_ASSERT("after one upper-half aligned translated store has already completed on the repaired boundary, the second upper-half aligned translated store should stay trap-free before the refaulted retry-4B+4B-store then pure-producer-tail-byte store plus upper-half load probe",
        excpt.triggered == false
    );

    TEST_SETUP_EXCEPT();
    ai_micro_mab_retry_byte4_exact_store_translated_sd(
        aligned_upper_store_vaddr, aligned_upper_store_value_third);
    TEST_ASSERT("after two upper-half aligned translated stores have already completed on the repaired boundary, the third upper-half aligned translated store should stay trap-free before the refaulted retry-4B+4B-store then pure-producer-tail-byte store plus upper-half load probe",
        excpt.triggered == false
    );

    TEST_SETUP_EXCEPT();
    ai_micro_mab_retry_byte4_exact_store_translated_sd(
        aligned_upper_store_vaddr, aligned_upper_store_value_fourth);
    TEST_ASSERT("after three upper-half aligned translated stores have already completed on the repaired boundary, the fourth upper-half aligned translated store should stay trap-free and become the latest committed high-half payload before the refaulted retry-4B+4B-store then pure-producer-tail-byte store plus upper-half load probe",
        excpt.triggered == false
    );

    ai_micro_mab_retry_byte4_exact_store_configure_pmp_window(
        deny_start, deny_end,
        AI_MICRO_MAB_RETRY_BYTE4_EXACT_STORE_PMP_TOR_NO_W);

    TEST_SETUP_EXCEPT();
    ai_micro_mab_retry_byte4_exact_store_translated_sd(
        fault_vaddr, refault_store_value);
    TEST_ASSERT("after four upper-half aligned translated stores have already completed on the repaired boundary, restoring the upper-half deny window and reissuing the original 4B+4B misaligned store should refresh the same precise second-split store access fault metadata before the pure-producer-tail-byte store plus upper-half load probe",
        excpt.triggered == true &&
        excpt.cause == CAUSE_SAF &&
        excpt.tval == second_half_vaddr
    );

    ai_micro_mab_retry_byte4_exact_store_restore_pmp_full_access();

    TEST_SETUP_EXCEPT();
    ai_micro_mab_retry_byte4_exact_store_translated_sd(
        fault_vaddr, retry_4b4b_store_value);
    TEST_ASSERT("after the refault episode is repaired again, retrying the same 4B+4B translated store as a success store should clear stale exception metadata before the immediate pure-producer-tail-byte store and upper-half load probe",
        excpt.triggered == false
    );

    TEST_SETUP_EXCEPT();
    ai_micro_mab_retry_byte4_exact_store_translated_sb(
        followup_pure_producer_tail_byte_store_vaddr,
        followup_pure_producer_tail_byte_store_value);
    TEST_ASSERT("after the retried original-template 4B+4B store has already succeeded on the same boundary, the immediate translated pure-producer tail-byte store should also stay trap-free and must not let a retired fake-crosspage exact 4-byte high-half owner/template leak back into bytes8-11 or widen beyond byte15 while bytes12-14 stay owned by the latest aligned producer-side image",
        excpt.triggered == false
    );

    ai_micro_mab_retry_byte4_exact_store_overlay_little_endian_bytes(
        expected_boundary, boundary_seed, 16U, 8U,
        aligned_upper_store_value_fourth, sizeof(uint64_t));
    ai_micro_mab_retry_byte4_exact_store_overlay_little_endian_bytes(
        expected_boundary, expected_boundary, 16U, 4U,
        retry_4b4b_store_value, sizeof(uint64_t));
    ai_micro_mab_retry_byte4_exact_store_overlay_little_endian_bytes(
        expected_boundary, expected_boundary, 16U, 15U,
        (uint64_t)followup_pure_producer_tail_byte_store_value,
        sizeof(uint8_t));
    expected_upper_half_load_value =
        ai_micro_mab_retry_byte4_exact_store_read_little_endian(
            expected_boundary + 8U, sizeof(uint64_t));

    TEST_SETUP_EXCEPT();
    observed_upper_half_load_value =
        ai_micro_mab_retry_byte4_exact_store_translated_ld(
            upper_half_load_vaddr);
    TEST_ASSERT("after the pure-producer tail-byte store has already refreshed byte15 on top of the retried 4B+4B image, the immediate aligned upper-half translated load at boundary+8 should clear stale exception metadata and read the merged latest image of retry bytes8-11, retained producer bytes12-14, and the new tail byte",
        excpt.triggered == false &&
        observed_upper_half_load_value == expected_upper_half_load_value
    );

    TEST_ASSERT("after the prolonged upper-half producer, repaired refault, retried 4B+4B success store, immediate pure-producer tail-byte store, and aligned upper-half load probe, the final boundary bytes should preserve bytes4-11 from the retried image, keep bytes12-14 from the latest aligned upper-half image, and refresh only byte15",
        ai_micro_mab_retry_byte4_exact_store_bytes_match(
            boundary_phys, expected_boundary, 16U)
    );

    TEST_ASSERT("guard regions around the same-page boundary should remain unchanged through the repeated 4B+4B fault, four upper-half aligned stores, refault, retried success store, immediate pure-producer-tail-byte follow-up store, and the aligned upper-half load probe sequence",
        ai_micro_mab_retry_byte4_exact_store_bytes_match(
            guard_before_phys, guard_before_seed, 8U) &&
        ai_micro_mab_retry_byte4_exact_store_bytes_match(
            guard_after_phys, guard_after_seed, 8U)
    );

    ai_micro_mab_retry_byte4_exact_store_reset_env();

    TEST_END(
        "ai_micro_m_samepage_cross16b_store_refault_upperx4_retry_4b4b_store_then_pure_producer_tail_byte_store_then_upper_half_load_corner");
}

bool ai_micro_m_samepage_cross16b_store_refault_upperx4_retry_4b4b_store_then_pure_producer_tail_byte_store_then_tail_last_halfword_high_byte_load_then_tail_last_halfword_low_byte_load_corner()
{
    TEST_START();

    static const uint8_t boundary_seed[16] = {
        0x39, 0x28, 0x17, 0x06, 0xf5, 0xe4, 0xd3, 0xc2,
        0xb1, 0xa0, 0x9f, 0x8e, 0x7d, 0x6c, 0x5b, 0x4a,
    };
    static const uint8_t guard_before_seed[8] = {
        0x18, 0x2a, 0x3c, 0x4e, 0x60, 0x72, 0x84, 0x96,
    };
    static const uint8_t guard_after_seed[8] = {
        0xa7, 0xb9, 0xcb, 0xdd, 0xef, 0x01, 0x13, 0x25,
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
    const uintptr_t followup_pure_producer_tail_byte_store_vaddr =
        aligned_upper_store_vaddr + 7U;
    const uintptr_t tail_last_halfword_high_byte_load_vaddr =
        followup_pure_producer_tail_byte_store_vaddr;
    const uintptr_t tail_last_halfword_low_byte_load_vaddr =
        aligned_upper_store_vaddr + 6U;
    const uintptr_t deny_start = boundary_phys + 8U;
    const uintptr_t deny_end = deny_start + PAGE_SIZE;
    const uint64_t first_fault_store_value = 0x65768798a9bacbdcULL;
    const uint64_t refault_store_value = 0xfd0e1f2031425364ULL;
    const uint64_t aligned_upper_store_value_first = 0x768798a9bacbdcedULL;
    const uint64_t aligned_upper_store_value_second = 0x0e1f203142536475ULL;
    const uint64_t aligned_upper_store_value_third = 0x8798a9bacbdcedfeULL;
    const uint64_t aligned_upper_store_value_fourth = 0x1f20314253647586ULL;
    const uint64_t retry_4b4b_store_value = 0x98a9bacbdcedfe0fULL;
    const uint8_t followup_pure_producer_tail_byte_store_value = 0x7eU;
    uint8_t expected_boundary[16];
    uint64_t expected_tail_last_halfword_high_byte_value = 0ULL;
    uint64_t observed_tail_last_halfword_high_byte_value = 0ULL;
    uint64_t expected_tail_last_halfword_low_byte_value = 0ULL;
    uint64_t observed_tail_last_halfword_low_byte_value = 0ULL;

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
    TEST_ASSERT("for the no-H same-page cross-16B translated scalar store, the first repeated 4B+4B second-split write fault must still report store access fault on the true second-half start address before the retried-success then pure-producer-tail-byte store plus the byte15 and byte14 follow-up probes",
        excpt.triggered == true &&
        excpt.cause == CAUSE_SAF &&
        excpt.tval == second_half_vaddr
    );

    TEST_SETUP_EXCEPT();
    ai_micro_mab_retry_byte4_exact_store_translated_sd(
        fault_vaddr, first_fault_store_value);
    TEST_ASSERT("repeating the same no-H second-split 4B+4B write fault keeps the same precise metadata before the retried-success then pure-producer-tail-byte store plus the byte15 and byte14 follow-up probes",
        excpt.triggered == true &&
        excpt.cause == CAUSE_SAF &&
        excpt.tval == second_half_vaddr
    );

    ai_micro_mab_retry_byte4_exact_store_restore_pmp_full_access();

    TEST_SETUP_EXCEPT();
    ai_micro_mab_retry_byte4_exact_store_translated_sd(
        aligned_upper_store_vaddr, aligned_upper_store_value_first);
    TEST_ASSERT("after repair, the first same-boundary translated upper-half aligned store should complete cleanly before the refaulted retry-4B+4B-store then pure-producer-tail-byte store plus the byte15 and byte14 follow-up probes",
        excpt.triggered == false
    );

    TEST_SETUP_EXCEPT();
    ai_micro_mab_retry_byte4_exact_store_translated_sd(
        aligned_upper_store_vaddr, aligned_upper_store_value_second);
    TEST_ASSERT("after one upper-half aligned translated store has already completed on the repaired boundary, the second upper-half aligned translated store should stay trap-free before the refaulted retry-4B+4B-store then pure-producer-tail-byte store plus the byte15 and byte14 follow-up probes",
        excpt.triggered == false
    );

    TEST_SETUP_EXCEPT();
    ai_micro_mab_retry_byte4_exact_store_translated_sd(
        aligned_upper_store_vaddr, aligned_upper_store_value_third);
    TEST_ASSERT("after two upper-half aligned translated stores have already completed on the repaired boundary, the third upper-half aligned translated store should stay trap-free before the refaulted retry-4B+4B-store then pure-producer-tail-byte store plus the byte15 and byte14 follow-up probes",
        excpt.triggered == false
    );

    TEST_SETUP_EXCEPT();
    ai_micro_mab_retry_byte4_exact_store_translated_sd(
        aligned_upper_store_vaddr, aligned_upper_store_value_fourth);
    TEST_ASSERT("after three upper-half aligned translated stores have already completed on the repaired boundary, the fourth upper-half aligned translated store should stay trap-free and become the latest committed high-half payload before the refaulted retry-4B+4B-store then pure-producer-tail-byte store plus the byte15 and byte14 follow-up probes",
        excpt.triggered == false
    );

    ai_micro_mab_retry_byte4_exact_store_configure_pmp_window(
        deny_start, deny_end,
        AI_MICRO_MAB_RETRY_BYTE4_EXACT_STORE_PMP_TOR_NO_W);

    TEST_SETUP_EXCEPT();
    ai_micro_mab_retry_byte4_exact_store_translated_sd(
        fault_vaddr, refault_store_value);
    TEST_ASSERT("after four upper-half aligned translated stores have already completed on the repaired boundary, restoring the upper-half deny window and reissuing the original 4B+4B misaligned store should refresh the same precise second-split store access fault metadata before the pure-producer-tail-byte store plus the byte15 and byte14 follow-up probes",
        excpt.triggered == true &&
        excpt.cause == CAUSE_SAF &&
        excpt.tval == second_half_vaddr
    );

    ai_micro_mab_retry_byte4_exact_store_restore_pmp_full_access();

    TEST_SETUP_EXCEPT();
    ai_micro_mab_retry_byte4_exact_store_translated_sd(
        fault_vaddr, retry_4b4b_store_value);
    TEST_ASSERT("after the refault episode is repaired again, retrying the same 4B+4B translated store as a success store should clear stale exception metadata before the immediate pure-producer-tail-byte store and the byte15 plus byte14 follow-up probes",
        excpt.triggered == false
    );

    TEST_SETUP_EXCEPT();
    ai_micro_mab_retry_byte4_exact_store_translated_sb(
        followup_pure_producer_tail_byte_store_vaddr,
        followup_pure_producer_tail_byte_store_value);
    TEST_ASSERT("after the retried original-template 4B+4B store has already succeeded on the same boundary, the immediate translated pure-producer tail-byte store should also stay trap-free and must not let a retired fake-crosspage exact 4-byte high-half owner/template leak back into bytes8-11 or widen beyond byte15 while byte14 stays owned by the latest aligned producer-side image",
        excpt.triggered == false
    );

    ai_micro_mab_retry_byte4_exact_store_overlay_little_endian_bytes(
        expected_boundary, boundary_seed, 16U, 8U,
        aligned_upper_store_value_fourth, sizeof(uint64_t));
    ai_micro_mab_retry_byte4_exact_store_overlay_little_endian_bytes(
        expected_boundary, expected_boundary, 16U, 4U,
        retry_4b4b_store_value, sizeof(uint64_t));
    ai_micro_mab_retry_byte4_exact_store_overlay_little_endian_bytes(
        expected_boundary, expected_boundary, 16U, 15U,
        (uint64_t)followup_pure_producer_tail_byte_store_value,
        sizeof(uint8_t));
    expected_tail_last_halfword_high_byte_value =
        (uint64_t)expected_boundary[15];
    expected_tail_last_halfword_low_byte_value =
        (uint64_t)expected_boundary[14];

    TEST_SETUP_EXCEPT();
    observed_tail_last_halfword_high_byte_value =
        ai_micro_mab_retry_byte4_exact_store_translated_lbu(
            tail_last_halfword_high_byte_load_vaddr);
    TEST_ASSERT("after the pure-producer tail-byte store has already refreshed byte15 on top of the retried 4B+4B image, the immediate translated tail-last-halfword high-byte load at boundary+15 should clear stale exception metadata and read the new tail byte before the following retained-byte14 probe",
        excpt.triggered == false &&
        observed_tail_last_halfword_high_byte_value ==
            expected_tail_last_halfword_high_byte_value
    );

    TEST_SETUP_EXCEPT();
    observed_tail_last_halfword_low_byte_value =
        ai_micro_mab_retry_byte4_exact_store_translated_lbu(
            tail_last_halfword_low_byte_load_vaddr);
    TEST_ASSERT("after the tail-last-halfword high-byte probe has already shown the pure-producer update at byte15 visible on top of the retried 4B+4B image, the following translated retained-byte14 probe at boundary+14 should still clear stale exception metadata and keep the latest aligned producer byte14 instead of reviving stale exact-BYTE4 tail state",
        excpt.triggered == false &&
        observed_tail_last_halfword_low_byte_value ==
            expected_tail_last_halfword_low_byte_value
    );

    TEST_ASSERT("after the prolonged upper-half producer, repaired refault, retried 4B+4B success store, immediate pure-producer tail-byte store, the byte15 probe, and the retained-byte14 probe, the final boundary bytes should preserve bytes4-11 from the retried image, keep bytes12-14 from the latest aligned upper-half image, and refresh only byte15",
        ai_micro_mab_retry_byte4_exact_store_bytes_match(
            boundary_phys, expected_boundary, 16U)
    );

    TEST_ASSERT("guard regions around the same-page boundary should remain unchanged through the repeated 4B+4B fault, four upper-half aligned stores, refault, retried success store, immediate pure-producer-tail-byte follow-up store, the byte15 probe, and the retained-byte14 probe sequence",
        ai_micro_mab_retry_byte4_exact_store_bytes_match(
            guard_before_phys, guard_before_seed, 8U) &&
        ai_micro_mab_retry_byte4_exact_store_bytes_match(
            guard_after_phys, guard_after_seed, 8U)
    );

    ai_micro_mab_retry_byte4_exact_store_reset_env();

    TEST_END(
        "ai_micro_m_samepage_cross16b_store_refault_upperx4_retry_4b4b_store_then_pure_producer_tail_byte_store_then_tail_last_halfword_high_byte_load_then_tail_last_halfword_low_byte_load_corner");
}

bool ai_micro_m_samepage_cross16b_store_refault_upperx4_retry_4b4b_store_then_pure_producer_tail_byte_store_then_tail_last_halfword_low_byte_load_then_tail_last_halfword_high_byte_load_corner()
{
    TEST_START();

    static const uint8_t boundary_seed[16] = {
        0x39, 0x28, 0x17, 0x06, 0xf5, 0xe4, 0xd3, 0xc2,
        0xb1, 0xa0, 0x9f, 0x8e, 0x7d, 0x6c, 0x5b, 0x4a,
    };
    static const uint8_t guard_before_seed[8] = {
        0x18, 0x2a, 0x3c, 0x4e, 0x60, 0x72, 0x84, 0x96,
    };
    static const uint8_t guard_after_seed[8] = {
        0xa7, 0xb9, 0xcb, 0xdd, 0xef, 0x01, 0x13, 0x25,
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
    const uintptr_t followup_pure_producer_tail_byte_store_vaddr =
        aligned_upper_store_vaddr + 7U;
    const uintptr_t tail_last_halfword_low_byte_load_vaddr =
        aligned_upper_store_vaddr + 6U;
    const uintptr_t tail_last_halfword_high_byte_load_vaddr =
        followup_pure_producer_tail_byte_store_vaddr;
    const uintptr_t deny_start = boundary_phys + 8U;
    const uintptr_t deny_end = deny_start + PAGE_SIZE;
    const uint64_t first_fault_store_value = 0x65768798a9bacbdcULL;
    const uint64_t refault_store_value = 0xfd0e1f2031425364ULL;
    const uint64_t aligned_upper_store_value_first = 0x768798a9bacbdcedULL;
    const uint64_t aligned_upper_store_value_second = 0x0e1f203142536475ULL;
    const uint64_t aligned_upper_store_value_third = 0x8798a9bacbdcedfeULL;
    const uint64_t aligned_upper_store_value_fourth = 0x1f20314253647586ULL;
    const uint64_t retry_4b4b_store_value = 0x98a9bacbdcedfe0fULL;
    const uint8_t followup_pure_producer_tail_byte_store_value = 0x7eU;
    uint8_t expected_boundary[16];
    uint64_t expected_tail_last_halfword_low_byte_value = 0ULL;
    uint64_t observed_tail_last_halfword_low_byte_value = 0ULL;
    uint64_t expected_tail_last_halfword_high_byte_value = 0ULL;
    uint64_t observed_tail_last_halfword_high_byte_value = 0ULL;

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
    TEST_ASSERT("for the no-H same-page cross-16B translated scalar store, the first repeated 4B+4B second-split write fault must still report store access fault on the true second-half start address before the retried-success then pure-producer-tail-byte store plus the byte14 and byte15 follow-up probes",
        excpt.triggered == true &&
        excpt.cause == CAUSE_SAF &&
        excpt.tval == second_half_vaddr
    );

    TEST_SETUP_EXCEPT();
    ai_micro_mab_retry_byte4_exact_store_translated_sd(
        fault_vaddr, first_fault_store_value);
    TEST_ASSERT("repeating the same no-H second-split 4B+4B write fault keeps the same precise metadata before the retried-success then pure-producer-tail-byte store plus the byte14 and byte15 follow-up probes",
        excpt.triggered == true &&
        excpt.cause == CAUSE_SAF &&
        excpt.tval == second_half_vaddr
    );

    ai_micro_mab_retry_byte4_exact_store_restore_pmp_full_access();

    TEST_SETUP_EXCEPT();
    ai_micro_mab_retry_byte4_exact_store_translated_sd(
        aligned_upper_store_vaddr, aligned_upper_store_value_first);
    TEST_ASSERT("after repair, the first same-boundary translated upper-half aligned store should complete cleanly before the refaulted retry-4B+4B-store then pure-producer-tail-byte store plus the byte14 and byte15 follow-up probes",
        excpt.triggered == false
    );

    TEST_SETUP_EXCEPT();
    ai_micro_mab_retry_byte4_exact_store_translated_sd(
        aligned_upper_store_vaddr, aligned_upper_store_value_second);
    TEST_ASSERT("after one upper-half aligned translated store has already completed on the repaired boundary, the second upper-half aligned translated store should stay trap-free before the refaulted retry-4B+4B-store then pure-producer-tail-byte store plus the byte14 and byte15 follow-up probes",
        excpt.triggered == false
    );

    TEST_SETUP_EXCEPT();
    ai_micro_mab_retry_byte4_exact_store_translated_sd(
        aligned_upper_store_vaddr, aligned_upper_store_value_third);
    TEST_ASSERT("after two upper-half aligned translated stores have already completed on the repaired boundary, the third upper-half aligned translated store should stay trap-free before the refaulted retry-4B+4B-store then pure-producer-tail-byte store plus the byte14 and byte15 follow-up probes",
        excpt.triggered == false
    );

    TEST_SETUP_EXCEPT();
    ai_micro_mab_retry_byte4_exact_store_translated_sd(
        aligned_upper_store_vaddr, aligned_upper_store_value_fourth);
    TEST_ASSERT("after three upper-half aligned translated stores have already completed on the repaired boundary, the fourth upper-half aligned translated store should stay trap-free and become the latest committed high-half payload before the refaulted retry-4B+4B-store then pure-producer-tail-byte store plus the byte14 and byte15 follow-up probes",
        excpt.triggered == false
    );

    ai_micro_mab_retry_byte4_exact_store_configure_pmp_window(
        deny_start, deny_end,
        AI_MICRO_MAB_RETRY_BYTE4_EXACT_STORE_PMP_TOR_NO_W);

    TEST_SETUP_EXCEPT();
    ai_micro_mab_retry_byte4_exact_store_translated_sd(
        fault_vaddr, refault_store_value);
    TEST_ASSERT("after four upper-half aligned translated stores have already completed on the repaired boundary, restoring the upper-half deny window and reissuing the original 4B+4B misaligned store should refresh the same precise second-split store access fault metadata before the pure-producer-tail-byte store plus the byte14 and byte15 follow-up probes",
        excpt.triggered == true &&
        excpt.cause == CAUSE_SAF &&
        excpt.tval == second_half_vaddr
    );

    ai_micro_mab_retry_byte4_exact_store_restore_pmp_full_access();

    TEST_SETUP_EXCEPT();
    ai_micro_mab_retry_byte4_exact_store_translated_sd(
        fault_vaddr, retry_4b4b_store_value);
    TEST_ASSERT("after the refault episode is repaired again, retrying the same 4B+4B translated store as a success store should clear stale exception metadata before the immediate pure-producer-tail-byte store and the byte14 plus byte15 follow-up probes",
        excpt.triggered == false
    );

    TEST_SETUP_EXCEPT();
    ai_micro_mab_retry_byte4_exact_store_translated_sb(
        followup_pure_producer_tail_byte_store_vaddr,
        followup_pure_producer_tail_byte_store_value);
    TEST_ASSERT("after the retried original-template 4B+4B store has already succeeded on the same boundary, the immediate translated pure-producer tail-byte store should also stay trap-free and must not let a retired fake-crosspage exact 4-byte high-half owner/template leak back into bytes8-11 or widen beyond byte15 while byte14 stays owned by the latest aligned producer-side image",
        excpt.triggered == false
    );

    ai_micro_mab_retry_byte4_exact_store_overlay_little_endian_bytes(
        expected_boundary, boundary_seed, 16U, 8U,
        aligned_upper_store_value_fourth, sizeof(uint64_t));
    ai_micro_mab_retry_byte4_exact_store_overlay_little_endian_bytes(
        expected_boundary, expected_boundary, 16U, 4U,
        retry_4b4b_store_value, sizeof(uint64_t));
    ai_micro_mab_retry_byte4_exact_store_overlay_little_endian_bytes(
        expected_boundary, expected_boundary, 16U, 15U,
        (uint64_t)followup_pure_producer_tail_byte_store_value,
        sizeof(uint8_t));
    expected_tail_last_halfword_low_byte_value =
        (uint64_t)expected_boundary[14];
    expected_tail_last_halfword_high_byte_value =
        (uint64_t)expected_boundary[15];

    TEST_SETUP_EXCEPT();
    observed_tail_last_halfword_low_byte_value =
        ai_micro_mab_retry_byte4_exact_store_translated_lbu(
            tail_last_halfword_low_byte_load_vaddr);
    TEST_ASSERT("after the pure-producer tail-byte store has already refreshed byte15 on top of the retried 4B+4B image, the immediate translated tail-last-halfword low-byte load at boundary+14 should clear stale exception metadata and keep the latest aligned producer byte14 before the following byte15 confirmation probe",
        excpt.triggered == false &&
        observed_tail_last_halfword_low_byte_value ==
            expected_tail_last_halfword_low_byte_value
    );

    TEST_SETUP_EXCEPT();
    observed_tail_last_halfword_high_byte_value =
        ai_micro_mab_retry_byte4_exact_store_translated_lbu(
            tail_last_halfword_high_byte_load_vaddr);
    TEST_ASSERT("after the tail-last-halfword low-byte probe has already shown byte14 still owned by the latest aligned producer on top of the retried 4B+4B image, the following translated byte15 confirmation probe should still clear stale exception metadata and read the new tail byte instead of falling back to stale exact-BYTE4 state",
        excpt.triggered == false &&
        observed_tail_last_halfword_high_byte_value ==
            expected_tail_last_halfword_high_byte_value
    );

    TEST_ASSERT("after the prolonged upper-half producer, repaired refault, retried 4B+4B success store, immediate pure-producer tail-byte store, the byte14 probe, and the byte15 probe, the final boundary bytes should preserve bytes4-11 from the retried image, keep bytes12-14 from the latest aligned upper-half image, and refresh only byte15",
        ai_micro_mab_retry_byte4_exact_store_bytes_match(
            boundary_phys, expected_boundary, 16U)
    );

    TEST_ASSERT("guard regions around the same-page boundary should remain unchanged through the repeated 4B+4B fault, four upper-half aligned stores, refault, retried success store, immediate pure-producer-tail-byte follow-up store, the byte14 probe, and the byte15 probe sequence",
        ai_micro_mab_retry_byte4_exact_store_bytes_match(
            guard_before_phys, guard_before_seed, 8U) &&
        ai_micro_mab_retry_byte4_exact_store_bytes_match(
            guard_after_phys, guard_after_seed, 8U)
    );

    ai_micro_mab_retry_byte4_exact_store_reset_env();

    TEST_END(
        "ai_micro_m_samepage_cross16b_store_refault_upperx4_retry_4b4b_store_then_pure_producer_tail_byte_store_then_tail_last_halfword_low_byte_load_then_tail_last_halfword_high_byte_load_corner");
}

bool ai_micro_m_samepage_cross16b_store_refault_upperx4_retry_4b4b_store_then_pure_producer_tail_byte_store_then_tail_last_halfword_low_byte_load_then_upper_half_load_corner()
{
    TEST_START();

    static const uint8_t boundary_seed[16] = {
        0x39, 0x28, 0x17, 0x06, 0xf5, 0xe4, 0xd3, 0xc2,
        0xb1, 0xa0, 0x9f, 0x8e, 0x7d, 0x6c, 0x5b, 0x4a,
    };
    static const uint8_t guard_before_seed[8] = {
        0x18, 0x2a, 0x3c, 0x4e, 0x60, 0x72, 0x84, 0x96,
    };
    static const uint8_t guard_after_seed[8] = {
        0xa7, 0xb9, 0xcb, 0xdd, 0xef, 0x01, 0x13, 0x25,
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
    const uintptr_t followup_pure_producer_tail_byte_store_vaddr =
        aligned_upper_store_vaddr + 7U;
    const uintptr_t tail_last_halfword_low_byte_load_vaddr =
        aligned_upper_store_vaddr + 6U;
    const uintptr_t upper_half_load_vaddr = aligned_upper_store_vaddr;
    const uintptr_t deny_start = boundary_phys + 8U;
    const uintptr_t deny_end = deny_start + PAGE_SIZE;
    const uint64_t first_fault_store_value = 0x65768798a9bacbdcULL;
    const uint64_t refault_store_value = 0xfd0e1f2031425364ULL;
    const uint64_t aligned_upper_store_value_first = 0x768798a9bacbdcedULL;
    const uint64_t aligned_upper_store_value_second = 0x0e1f203142536475ULL;
    const uint64_t aligned_upper_store_value_third = 0x8798a9bacbdcedfeULL;
    const uint64_t aligned_upper_store_value_fourth = 0x1f20314253647586ULL;
    const uint64_t retry_4b4b_store_value = 0x98a9bacbdcedfe0fULL;
    const uint8_t followup_pure_producer_tail_byte_store_value = 0x7eU;
    uint8_t expected_boundary[16];
    uint64_t expected_tail_last_halfword_low_byte_value = 0ULL;
    uint64_t observed_tail_last_halfword_low_byte_value = 0ULL;
    uint64_t expected_upper_half_load_value = 0ULL;
    uint64_t observed_upper_half_load_value = 0ULL;

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
    TEST_ASSERT("for the no-H same-page cross-16B translated scalar store, the first repeated 4B+4B second-split write fault must still report store access fault on the true second-half start address before the retried-success then pure-producer-tail-byte store plus the byte14 and upper-half follow-up probes",
        excpt.triggered == true &&
        excpt.cause == CAUSE_SAF &&
        excpt.tval == second_half_vaddr
    );

    TEST_SETUP_EXCEPT();
    ai_micro_mab_retry_byte4_exact_store_translated_sd(
        fault_vaddr, first_fault_store_value);
    TEST_ASSERT("repeating the same no-H second-split 4B+4B write fault keeps the same precise metadata before the retried-success then pure-producer-tail-byte store plus the byte14 and upper-half follow-up probes",
        excpt.triggered == true &&
        excpt.cause == CAUSE_SAF &&
        excpt.tval == second_half_vaddr
    );

    ai_micro_mab_retry_byte4_exact_store_restore_pmp_full_access();

    TEST_SETUP_EXCEPT();
    ai_micro_mab_retry_byte4_exact_store_translated_sd(
        aligned_upper_store_vaddr, aligned_upper_store_value_first);
    TEST_ASSERT("after repair, the first same-boundary translated upper-half aligned store should complete cleanly before the refaulted retry-4B+4B-store then pure-producer-tail-byte store plus the byte14 and upper-half follow-up probes",
        excpt.triggered == false
    );

    TEST_SETUP_EXCEPT();
    ai_micro_mab_retry_byte4_exact_store_translated_sd(
        aligned_upper_store_vaddr, aligned_upper_store_value_second);
    TEST_ASSERT("after one upper-half aligned translated store has already completed on the repaired boundary, the second upper-half aligned translated store should stay trap-free before the refaulted retry-4B+4B-store then pure-producer-tail-byte store plus the byte14 and upper-half follow-up probes",
        excpt.triggered == false
    );

    TEST_SETUP_EXCEPT();
    ai_micro_mab_retry_byte4_exact_store_translated_sd(
        aligned_upper_store_vaddr, aligned_upper_store_value_third);
    TEST_ASSERT("after two upper-half aligned translated stores have already completed on the repaired boundary, the third upper-half aligned translated store should stay trap-free before the refaulted retry-4B+4B-store then pure-producer-tail-byte store plus the byte14 and upper-half follow-up probes",
        excpt.triggered == false
    );

    TEST_SETUP_EXCEPT();
    ai_micro_mab_retry_byte4_exact_store_translated_sd(
        aligned_upper_store_vaddr, aligned_upper_store_value_fourth);
    TEST_ASSERT("after three upper-half aligned translated stores have already completed on the repaired boundary, the fourth upper-half aligned translated store should stay trap-free and become the latest committed high-half payload before the refaulted retry-4B+4B-store then pure-producer-tail-byte store plus the byte14 and upper-half follow-up probes",
        excpt.triggered == false
    );

    ai_micro_mab_retry_byte4_exact_store_configure_pmp_window(
        deny_start, deny_end,
        AI_MICRO_MAB_RETRY_BYTE4_EXACT_STORE_PMP_TOR_NO_W);

    TEST_SETUP_EXCEPT();
    ai_micro_mab_retry_byte4_exact_store_translated_sd(
        fault_vaddr, refault_store_value);
    TEST_ASSERT("after four upper-half aligned translated stores have already completed on the repaired boundary, restoring the upper-half deny window and reissuing the original 4B+4B misaligned store should refresh the same precise second-split store access fault metadata before the pure-producer-tail-byte store plus the byte14 and upper-half follow-up probes",
        excpt.triggered == true &&
        excpt.cause == CAUSE_SAF &&
        excpt.tval == second_half_vaddr
    );

    ai_micro_mab_retry_byte4_exact_store_restore_pmp_full_access();

    TEST_SETUP_EXCEPT();
    ai_micro_mab_retry_byte4_exact_store_translated_sd(
        fault_vaddr, retry_4b4b_store_value);
    TEST_ASSERT("after the refault episode is repaired again, retrying the same 4B+4B translated store as a success store should clear stale exception metadata before the immediate pure-producer-tail-byte store and the byte14 plus upper-half follow-up probes",
        excpt.triggered == false
    );

    TEST_SETUP_EXCEPT();
    ai_micro_mab_retry_byte4_exact_store_translated_sb(
        followup_pure_producer_tail_byte_store_vaddr,
        followup_pure_producer_tail_byte_store_value);
    TEST_ASSERT("after the retried original-template 4B+4B store has already succeeded on the same boundary, the immediate translated pure-producer tail-byte store should also stay trap-free and must not let a retired fake-crosspage exact 4-byte high-half owner/template leak back into bytes8-11 or widen beyond byte15 while byte14 stays owned by the latest aligned producer-side image",
        excpt.triggered == false
    );

    ai_micro_mab_retry_byte4_exact_store_overlay_little_endian_bytes(
        expected_boundary, boundary_seed, 16U, 8U,
        aligned_upper_store_value_fourth, sizeof(uint64_t));
    ai_micro_mab_retry_byte4_exact_store_overlay_little_endian_bytes(
        expected_boundary, expected_boundary, 16U, 4U,
        retry_4b4b_store_value, sizeof(uint64_t));
    ai_micro_mab_retry_byte4_exact_store_overlay_little_endian_bytes(
        expected_boundary, expected_boundary, 16U, 15U,
        (uint64_t)followup_pure_producer_tail_byte_store_value,
        sizeof(uint8_t));
    expected_tail_last_halfword_low_byte_value =
        (uint64_t)expected_boundary[14];
    expected_upper_half_load_value =
        ai_micro_mab_retry_byte4_exact_store_read_little_endian(
            expected_boundary + 8U, sizeof(uint64_t));

    TEST_SETUP_EXCEPT();
    observed_tail_last_halfword_low_byte_value =
        ai_micro_mab_retry_byte4_exact_store_translated_lbu(
            tail_last_halfword_low_byte_load_vaddr);
    TEST_ASSERT("after the pure-producer tail-byte store has already refreshed byte15 on top of the retried 4B+4B image, the immediate translated tail-last-halfword low-byte load at boundary+14 should clear stale exception metadata and keep the latest aligned producer byte14 before the following upper-half confirmation probe",
        excpt.triggered == false &&
        observed_tail_last_halfword_low_byte_value ==
            expected_tail_last_halfword_low_byte_value
    );

    TEST_SETUP_EXCEPT();
    observed_upper_half_load_value =
        ai_micro_mab_retry_byte4_exact_store_translated_ld(
            upper_half_load_vaddr);
    TEST_ASSERT("after the tail-last-halfword low-byte probe has already shown byte14 still owned by the latest aligned producer on top of the retried 4B+4B image, the following aligned upper-half translated load at boundary+8 should still clear stale exception metadata and read the merged latest image of retry bytes8-11, retained producer bytes12-14, and the new tail byte",
        excpt.triggered == false &&
        observed_upper_half_load_value == expected_upper_half_load_value
    );

    TEST_ASSERT("after the prolonged upper-half producer, repaired refault, retried 4B+4B success store, immediate pure-producer tail-byte store, the byte14 probe, and the upper-half load probe, the final boundary bytes should preserve bytes4-11 from the retried image, keep bytes12-14 from the latest aligned upper-half image, and refresh only byte15",
        ai_micro_mab_retry_byte4_exact_store_bytes_match(
            boundary_phys, expected_boundary, 16U)
    );

    TEST_ASSERT("guard regions around the same-page boundary should remain unchanged through the repeated 4B+4B fault, four upper-half aligned stores, refault, retried success store, immediate pure-producer-tail-byte follow-up store, the byte14 probe, and the aligned upper-half load probe sequence",
        ai_micro_mab_retry_byte4_exact_store_bytes_match(
            guard_before_phys, guard_before_seed, 8U) &&
        ai_micro_mab_retry_byte4_exact_store_bytes_match(
            guard_after_phys, guard_after_seed, 8U)
    );

    ai_micro_mab_retry_byte4_exact_store_reset_env();

    TEST_END(
        "ai_micro_m_samepage_cross16b_store_refault_upperx4_retry_4b4b_store_then_pure_producer_tail_byte_store_then_tail_last_halfword_low_byte_load_then_upper_half_load_corner");
}

bool ai_micro_m_samepage_cross16b_store_refault_upperx4_retry_4b4b_store_then_pure_producer_tail_byte_store_then_tail_last_halfword_high_byte_load_then_upper_half_load_corner()
{
    TEST_START();

    static const uint8_t boundary_seed[16] = {
        0x39, 0x28, 0x17, 0x06, 0xf5, 0xe4, 0xd3, 0xc2,
        0xb1, 0xa0, 0x9f, 0x8e, 0x7d, 0x6c, 0x5b, 0x4a,
    };
    static const uint8_t guard_before_seed[8] = {
        0x18, 0x2a, 0x3c, 0x4e, 0x60, 0x72, 0x84, 0x96,
    };
    static const uint8_t guard_after_seed[8] = {
        0xa7, 0xb9, 0xcb, 0xdd, 0xef, 0x01, 0x13, 0x25,
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
    const uintptr_t followup_pure_producer_tail_byte_store_vaddr =
        aligned_upper_store_vaddr + 7U;
    const uintptr_t tail_last_halfword_high_byte_load_vaddr =
        aligned_upper_store_vaddr + 7U;
    const uintptr_t upper_half_load_vaddr = aligned_upper_store_vaddr;
    const uintptr_t deny_start = boundary_phys + 8U;
    const uintptr_t deny_end = deny_start + PAGE_SIZE;
    const uint64_t first_fault_store_value = 0x65768798a9bacbdcULL;
    const uint64_t refault_store_value = 0xfd0e1f2031425364ULL;
    const uint64_t aligned_upper_store_value_first = 0x768798a9bacbdcedULL;
    const uint64_t aligned_upper_store_value_second = 0x0e1f203142536475ULL;
    const uint64_t aligned_upper_store_value_third = 0x8798a9bacbdcedfeULL;
    const uint64_t aligned_upper_store_value_fourth = 0x1f20314253647586ULL;
    const uint64_t retry_4b4b_store_value = 0x98a9bacbdcedfe0fULL;
    const uint8_t followup_pure_producer_tail_byte_store_value = 0x7eU;
    uint8_t expected_boundary[16];
    uint64_t expected_tail_last_halfword_high_byte_value = 0ULL;
    uint64_t observed_tail_last_halfword_high_byte_value = 0ULL;
    uint64_t expected_upper_half_load_value = 0ULL;
    uint64_t observed_upper_half_load_value = 0ULL;

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
    TEST_ASSERT("for the no-H same-page cross-16B translated scalar store, the first repeated 4B+4B second-split write fault must still report store access fault on the true second-half start address before the retried-success then pure-producer-tail-byte store plus the byte15 and upper-half follow-up probes",
        excpt.triggered == true &&
        excpt.cause == CAUSE_SAF &&
        excpt.tval == second_half_vaddr
    );

    TEST_SETUP_EXCEPT();
    ai_micro_mab_retry_byte4_exact_store_translated_sd(
        fault_vaddr, first_fault_store_value);
    TEST_ASSERT("repeating the same no-H second-split 4B+4B write fault keeps the same precise metadata before the retried-success then pure-producer-tail-byte store plus the byte15 and upper-half follow-up probes",
        excpt.triggered == true &&
        excpt.cause == CAUSE_SAF &&
        excpt.tval == second_half_vaddr
    );

    ai_micro_mab_retry_byte4_exact_store_restore_pmp_full_access();

    TEST_SETUP_EXCEPT();
    ai_micro_mab_retry_byte4_exact_store_translated_sd(
        aligned_upper_store_vaddr, aligned_upper_store_value_first);
    TEST_ASSERT("after repair, the first same-boundary translated upper-half aligned store should complete cleanly before the refaulted retry-4B+4B-store then pure-producer-tail-byte store plus the byte15 and upper-half follow-up probes",
        excpt.triggered == false
    );

    TEST_SETUP_EXCEPT();
    ai_micro_mab_retry_byte4_exact_store_translated_sd(
        aligned_upper_store_vaddr, aligned_upper_store_value_second);
    TEST_ASSERT("after one upper-half aligned translated store has already completed on the repaired boundary, the second upper-half aligned translated store should stay trap-free before the refaulted retry-4B+4B-store then pure-producer-tail-byte store plus the byte15 and upper-half follow-up probes",
        excpt.triggered == false
    );

    TEST_SETUP_EXCEPT();
    ai_micro_mab_retry_byte4_exact_store_translated_sd(
        aligned_upper_store_vaddr, aligned_upper_store_value_third);
    TEST_ASSERT("after two upper-half aligned translated stores have already completed on the repaired boundary, the third upper-half aligned translated store should stay trap-free before the refaulted retry-4B+4B-store then pure-producer-tail-byte store plus the byte15 and upper-half follow-up probes",
        excpt.triggered == false
    );

    TEST_SETUP_EXCEPT();
    ai_micro_mab_retry_byte4_exact_store_translated_sd(
        aligned_upper_store_vaddr, aligned_upper_store_value_fourth);
    TEST_ASSERT("after three upper-half aligned translated stores have already completed on the repaired boundary, the fourth upper-half aligned translated store should stay trap-free and become the latest committed high-half payload before the refaulted retry-4B+4B-store then pure-producer-tail-byte store plus the byte15 and upper-half follow-up probes",
        excpt.triggered == false
    );

    ai_micro_mab_retry_byte4_exact_store_configure_pmp_window(
        deny_start, deny_end,
        AI_MICRO_MAB_RETRY_BYTE4_EXACT_STORE_PMP_TOR_NO_W);

    TEST_SETUP_EXCEPT();
    ai_micro_mab_retry_byte4_exact_store_translated_sd(
        fault_vaddr, refault_store_value);
    TEST_ASSERT("after four upper-half aligned translated stores have already completed on the repaired boundary, restoring the upper-half deny window and reissuing the original 4B+4B misaligned store should refresh the same precise second-split store access fault metadata before the pure-producer-tail-byte store plus the byte15 and upper-half follow-up probes",
        excpt.triggered == true &&
        excpt.cause == CAUSE_SAF &&
        excpt.tval == second_half_vaddr
    );

    ai_micro_mab_retry_byte4_exact_store_restore_pmp_full_access();

    TEST_SETUP_EXCEPT();
    ai_micro_mab_retry_byte4_exact_store_translated_sd(
        fault_vaddr, retry_4b4b_store_value);
    TEST_ASSERT("after the refault episode is repaired again, retrying the same 4B+4B translated store as a success store should clear stale exception metadata before the immediate pure-producer-tail-byte store and the byte15 plus upper-half follow-up probes",
        excpt.triggered == false
    );

    TEST_SETUP_EXCEPT();
    ai_micro_mab_retry_byte4_exact_store_translated_sb(
        followup_pure_producer_tail_byte_store_vaddr,
        followup_pure_producer_tail_byte_store_value);
    TEST_ASSERT("after the retried original-template 4B+4B store has already succeeded on the same boundary, the immediate translated pure-producer tail-byte store should also stay trap-free and must not let a retired fake-crosspage exact 4-byte high-half owner/template leak back into bytes8-11 or widen beyond byte15 while bytes12-14 stay owned by the latest aligned producer-side image",
        excpt.triggered == false
    );

    ai_micro_mab_retry_byte4_exact_store_overlay_little_endian_bytes(
        expected_boundary, boundary_seed, 16U, 8U,
        aligned_upper_store_value_fourth, sizeof(uint64_t));
    ai_micro_mab_retry_byte4_exact_store_overlay_little_endian_bytes(
        expected_boundary, expected_boundary, 16U, 4U,
        retry_4b4b_store_value, sizeof(uint64_t));
    ai_micro_mab_retry_byte4_exact_store_overlay_little_endian_bytes(
        expected_boundary, expected_boundary, 16U, 15U,
        (uint64_t)followup_pure_producer_tail_byte_store_value,
        sizeof(uint8_t));
    expected_tail_last_halfword_high_byte_value =
        (uint64_t)expected_boundary[15];
    expected_upper_half_load_value =
        ai_micro_mab_retry_byte4_exact_store_read_little_endian(
            expected_boundary + 8U, sizeof(uint64_t));

    TEST_SETUP_EXCEPT();
    observed_tail_last_halfword_high_byte_value =
        ai_micro_mab_retry_byte4_exact_store_translated_lbu(
            tail_last_halfword_high_byte_load_vaddr);
    TEST_ASSERT("after the pure-producer tail-byte store has already refreshed byte15 on top of the retried 4B+4B image, the immediate translated byte15 confirmation probe should clear stale exception metadata and read the new tail byte before the following upper-half load probe",
        excpt.triggered == false &&
        observed_tail_last_halfword_high_byte_value ==
            expected_tail_last_halfword_high_byte_value
    );

    TEST_SETUP_EXCEPT();
    observed_upper_half_load_value =
        ai_micro_mab_retry_byte4_exact_store_translated_ld(
            upper_half_load_vaddr);
    TEST_ASSERT("after the byte15 confirmation probe has already shown the new tail byte on top of the retried 4B+4B image, the following aligned upper-half translated load at boundary+8 should still clear stale exception metadata and read the merged latest image of retry bytes8-11, retained producer bytes12-14, and the new tail byte",
        excpt.triggered == false &&
        observed_upper_half_load_value == expected_upper_half_load_value
    );

    TEST_ASSERT("after the prolonged upper-half producer, repaired refault, retried 4B+4B success store, immediate pure-producer tail-byte store, the byte15 probe, and the upper-half load probe, the final boundary bytes should preserve bytes4-11 from the retried image, keep bytes12-14 from the latest aligned upper-half image, and refresh only byte15",
        ai_micro_mab_retry_byte4_exact_store_bytes_match(
            boundary_phys, expected_boundary, 16U)
    );

    TEST_ASSERT("guard regions around the same-page boundary should remain unchanged through the repeated 4B+4B fault, four upper-half aligned stores, refault, retried success store, immediate pure-producer-tail-byte follow-up store, the byte15 probe, and the aligned upper-half load probe sequence",
        ai_micro_mab_retry_byte4_exact_store_bytes_match(
            guard_before_phys, guard_before_seed, 8U) &&
        ai_micro_mab_retry_byte4_exact_store_bytes_match(
            guard_after_phys, guard_after_seed, 8U)
    );

    ai_micro_mab_retry_byte4_exact_store_reset_env();

    TEST_END(
        "ai_micro_m_samepage_cross16b_store_refault_upperx4_retry_4b4b_store_then_pure_producer_tail_byte_store_then_tail_last_halfword_high_byte_load_then_upper_half_load_corner");
}

bool ai_micro_m_samepage_cross16b_store_refault_upperx4_retry_4b4b_store_then_pure_producer_tail_byte_store_then_upper_half_load_then_tail_last_halfword_low_byte_load_corner()
{
    TEST_START();

    static const uint8_t boundary_seed[16] = {
        0x39, 0x28, 0x17, 0x06, 0xf5, 0xe4, 0xd3, 0xc2,
        0xb1, 0xa0, 0x9f, 0x8e, 0x7d, 0x6c, 0x5b, 0x4a,
    };
    static const uint8_t guard_before_seed[8] = {
        0x18, 0x2a, 0x3c, 0x4e, 0x60, 0x72, 0x84, 0x96,
    };
    static const uint8_t guard_after_seed[8] = {
        0xa7, 0xb9, 0xcb, 0xdd, 0xef, 0x01, 0x13, 0x25,
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
    const uintptr_t followup_pure_producer_tail_byte_store_vaddr =
        aligned_upper_store_vaddr + 7U;
    const uintptr_t upper_half_load_vaddr = aligned_upper_store_vaddr;
    const uintptr_t tail_last_halfword_low_byte_load_vaddr =
        aligned_upper_store_vaddr + 6U;
    const uintptr_t deny_start = boundary_phys + 8U;
    const uintptr_t deny_end = deny_start + PAGE_SIZE;
    const uint64_t first_fault_store_value = 0x65768798a9bacbdcULL;
    const uint64_t refault_store_value = 0xfd0e1f2031425364ULL;
    const uint64_t aligned_upper_store_value_first = 0x768798a9bacbdcedULL;
    const uint64_t aligned_upper_store_value_second = 0x0e1f203142536475ULL;
    const uint64_t aligned_upper_store_value_third = 0x8798a9bacbdcedfeULL;
    const uint64_t aligned_upper_store_value_fourth = 0x1f20314253647586ULL;
    const uint64_t retry_4b4b_store_value = 0x98a9bacbdcedfe0fULL;
    const uint8_t followup_pure_producer_tail_byte_store_value = 0x7eU;
    uint8_t expected_boundary[16];
    uint64_t expected_upper_half_load_value = 0ULL;
    uint64_t observed_upper_half_load_value = 0ULL;
    uint64_t expected_tail_last_halfword_low_byte_value = 0ULL;
    uint64_t observed_tail_last_halfword_low_byte_value = 0ULL;

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
    TEST_ASSERT("for the no-H same-page cross-16B translated scalar store, the first repeated 4B+4B second-split write fault must still report store access fault on the true second-half start address before the retried-success then pure-producer-tail-byte store plus the upper-half and byte14 follow-up probes",
        excpt.triggered == true &&
        excpt.cause == CAUSE_SAF &&
        excpt.tval == second_half_vaddr
    );

    TEST_SETUP_EXCEPT();
    ai_micro_mab_retry_byte4_exact_store_translated_sd(
        fault_vaddr, first_fault_store_value);
    TEST_ASSERT("repeating the same no-H second-split 4B+4B write fault keeps the same precise metadata before the retried-success then pure-producer-tail-byte store plus the upper-half and byte14 follow-up probes",
        excpt.triggered == true &&
        excpt.cause == CAUSE_SAF &&
        excpt.tval == second_half_vaddr
    );

    ai_micro_mab_retry_byte4_exact_store_restore_pmp_full_access();

    TEST_SETUP_EXCEPT();
    ai_micro_mab_retry_byte4_exact_store_translated_sd(
        aligned_upper_store_vaddr, aligned_upper_store_value_first);
    TEST_ASSERT("after repair, the first same-boundary translated upper-half aligned store should complete cleanly before the refaulted retry-4B+4B-store then pure-producer-tail-byte store plus the upper-half and byte14 follow-up probes",
        excpt.triggered == false
    );

    TEST_SETUP_EXCEPT();
    ai_micro_mab_retry_byte4_exact_store_translated_sd(
        aligned_upper_store_vaddr, aligned_upper_store_value_second);
    TEST_ASSERT("after one upper-half aligned translated store has already completed on the repaired boundary, the second upper-half aligned translated store should stay trap-free before the refaulted retry-4B+4B-store then pure-producer-tail-byte store plus the upper-half and byte14 follow-up probes",
        excpt.triggered == false
    );

    TEST_SETUP_EXCEPT();
    ai_micro_mab_retry_byte4_exact_store_translated_sd(
        aligned_upper_store_vaddr, aligned_upper_store_value_third);
    TEST_ASSERT("after two upper-half aligned translated stores have already completed on the repaired boundary, the third upper-half aligned translated store should stay trap-free before the refaulted retry-4B+4B-store then pure-producer-tail-byte store plus the upper-half and byte14 follow-up probes",
        excpt.triggered == false
    );

    TEST_SETUP_EXCEPT();
    ai_micro_mab_retry_byte4_exact_store_translated_sd(
        aligned_upper_store_vaddr, aligned_upper_store_value_fourth);
    TEST_ASSERT("after three upper-half aligned translated stores have already completed on the repaired boundary, the fourth upper-half aligned translated store should stay trap-free and become the latest committed high-half payload before the refaulted retry-4B+4B-store then pure-producer-tail-byte store plus the upper-half and byte14 follow-up probes",
        excpt.triggered == false
    );

    ai_micro_mab_retry_byte4_exact_store_configure_pmp_window(
        deny_start, deny_end,
        AI_MICRO_MAB_RETRY_BYTE4_EXACT_STORE_PMP_TOR_NO_W);

    TEST_SETUP_EXCEPT();
    ai_micro_mab_retry_byte4_exact_store_translated_sd(
        fault_vaddr, refault_store_value);
    TEST_ASSERT("after four upper-half aligned translated stores have already completed on the repaired boundary, restoring the upper-half deny window and reissuing the original 4B+4B misaligned store should refresh the same precise second-split store access fault metadata before the pure-producer-tail-byte store plus the upper-half and byte14 follow-up probes",
        excpt.triggered == true &&
        excpt.cause == CAUSE_SAF &&
        excpt.tval == second_half_vaddr
    );

    ai_micro_mab_retry_byte4_exact_store_restore_pmp_full_access();

    TEST_SETUP_EXCEPT();
    ai_micro_mab_retry_byte4_exact_store_translated_sd(
        fault_vaddr, retry_4b4b_store_value);
    TEST_ASSERT("after the refault episode is repaired again, retrying the same 4B+4B translated store as a success store should clear stale exception metadata before the immediate pure-producer-tail-byte store and the upper-half plus byte14 follow-up probes",
        excpt.triggered == false
    );

    TEST_SETUP_EXCEPT();
    ai_micro_mab_retry_byte4_exact_store_translated_sb(
        followup_pure_producer_tail_byte_store_vaddr,
        followup_pure_producer_tail_byte_store_value);
    TEST_ASSERT("after the retried original-template 4B+4B store has already succeeded on the same boundary, the immediate translated pure-producer tail-byte store should also stay trap-free and must not let a retired fake-crosspage exact 4-byte high-half owner/template leak back into bytes8-11 or widen beyond byte15 while byte14 stays owned by the latest aligned producer-side image",
        excpt.triggered == false
    );

    ai_micro_mab_retry_byte4_exact_store_overlay_little_endian_bytes(
        expected_boundary, boundary_seed, 16U, 8U,
        aligned_upper_store_value_fourth, sizeof(uint64_t));
    ai_micro_mab_retry_byte4_exact_store_overlay_little_endian_bytes(
        expected_boundary, expected_boundary, 16U, 4U,
        retry_4b4b_store_value, sizeof(uint64_t));
    ai_micro_mab_retry_byte4_exact_store_overlay_little_endian_bytes(
        expected_boundary, expected_boundary, 16U, 15U,
        (uint64_t)followup_pure_producer_tail_byte_store_value,
        sizeof(uint8_t));
    expected_upper_half_load_value =
        ai_micro_mab_retry_byte4_exact_store_read_little_endian(
            expected_boundary + 8U, sizeof(uint64_t));
    expected_tail_last_halfword_low_byte_value =
        (uint64_t)expected_boundary[14];

    TEST_SETUP_EXCEPT();
    observed_upper_half_load_value =
        ai_micro_mab_retry_byte4_exact_store_translated_ld(
            upper_half_load_vaddr);
    TEST_ASSERT("after the pure-producer tail-byte store has already refreshed byte15 on top of the retried 4B+4B image, the immediate aligned upper-half translated load at boundary+8 should clear stale exception metadata and read the merged latest image of retry bytes8-11, retained producer bytes12-14, and the new tail byte before the following byte14 confirmation probe",
        excpt.triggered == false &&
        observed_upper_half_load_value == expected_upper_half_load_value
    );

    TEST_SETUP_EXCEPT();
    observed_tail_last_halfword_low_byte_value =
        ai_micro_mab_retry_byte4_exact_store_translated_lbu(
            tail_last_halfword_low_byte_load_vaddr);
    TEST_ASSERT("after the aligned upper-half load probe has already shown the merged latest image on top of the retried 4B+4B store, the following translated tail-last-halfword low-byte load at boundary+14 should still clear stale exception metadata and keep the latest aligned producer byte14",
        excpt.triggered == false &&
        observed_tail_last_halfword_low_byte_value ==
            expected_tail_last_halfword_low_byte_value
    );

    TEST_ASSERT("after the prolonged upper-half producer, repaired refault, retried 4B+4B success store, immediate pure-producer tail-byte store, the upper-half load probe, and the byte14 probe, the final boundary bytes should preserve bytes4-11 from the retried image, keep bytes12-14 from the latest aligned upper-half image, and refresh only byte15",
        ai_micro_mab_retry_byte4_exact_store_bytes_match(
            boundary_phys, expected_boundary, 16U)
    );

    TEST_ASSERT("guard regions around the same-page boundary should remain unchanged through the repeated 4B+4B fault, four upper-half aligned stores, refault, retried success store, immediate pure-producer-tail-byte follow-up store, the aligned upper-half load probe, and the byte14 probe sequence",
        ai_micro_mab_retry_byte4_exact_store_bytes_match(
            guard_before_phys, guard_before_seed, 8U) &&
        ai_micro_mab_retry_byte4_exact_store_bytes_match(
            guard_after_phys, guard_after_seed, 8U)
    );

    ai_micro_mab_retry_byte4_exact_store_reset_env();

    TEST_END(
        "ai_micro_m_samepage_cross16b_store_refault_upperx4_retry_4b4b_store_then_pure_producer_tail_byte_store_then_upper_half_load_then_tail_last_halfword_low_byte_load_corner");
}

bool ai_micro_m_samepage_cross16b_store_refault_upperx4_retry_4b4b_store_then_pure_producer_tail_byte_store_then_upper_half_load_then_tail_last_halfword_high_byte_load_corner()
{
    TEST_START();

    static const uint8_t boundary_seed[16] = {
        0x39, 0x28, 0x17, 0x06, 0xf5, 0xe4, 0xd3, 0xc2,
        0xb1, 0xa0, 0x9f, 0x8e, 0x7d, 0x6c, 0x5b, 0x4a,
    };
    static const uint8_t guard_before_seed[8] = {
        0x18, 0x2a, 0x3c, 0x4e, 0x60, 0x72, 0x84, 0x96,
    };
    static const uint8_t guard_after_seed[8] = {
        0xa7, 0xb9, 0xcb, 0xdd, 0xef, 0x01, 0x13, 0x25,
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
    const uintptr_t followup_pure_producer_tail_byte_store_vaddr =
        aligned_upper_store_vaddr + 7U;
    const uintptr_t upper_half_load_vaddr = aligned_upper_store_vaddr;
    const uintptr_t tail_last_halfword_high_byte_load_vaddr =
        aligned_upper_store_vaddr + 7U;
    const uintptr_t deny_start = boundary_phys + 8U;
    const uintptr_t deny_end = deny_start + PAGE_SIZE;
    const uint64_t first_fault_store_value = 0x65768798a9bacbdcULL;
    const uint64_t refault_store_value = 0xfd0e1f2031425364ULL;
    const uint64_t aligned_upper_store_value_first = 0x768798a9bacbdcedULL;
    const uint64_t aligned_upper_store_value_second = 0x0e1f203142536475ULL;
    const uint64_t aligned_upper_store_value_third = 0x8798a9bacbdcedfeULL;
    const uint64_t aligned_upper_store_value_fourth = 0x1f20314253647586ULL;
    const uint64_t retry_4b4b_store_value = 0x98a9bacbdcedfe0fULL;
    const uint8_t followup_pure_producer_tail_byte_store_value = 0x7eU;
    uint8_t expected_boundary[16];
    uint64_t expected_upper_half_load_value = 0ULL;
    uint64_t observed_upper_half_load_value = 0ULL;
    uint64_t expected_tail_last_halfword_high_byte_value = 0ULL;
    uint64_t observed_tail_last_halfword_high_byte_value = 0ULL;

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
    TEST_ASSERT("for the no-H same-page cross-16B translated scalar store, the first repeated 4B+4B second-split write fault must still report store access fault on the true second-half start address before the retried-success then pure-producer-tail-byte store plus the upper-half and byte15 follow-up probes",
        excpt.triggered == true &&
        excpt.cause == CAUSE_SAF &&
        excpt.tval == second_half_vaddr
    );

    TEST_SETUP_EXCEPT();
    ai_micro_mab_retry_byte4_exact_store_translated_sd(
        fault_vaddr, first_fault_store_value);
    TEST_ASSERT("repeating the same no-H second-split 4B+4B write fault keeps the same precise metadata before the retried-success then pure-producer-tail-byte store plus the upper-half and byte15 follow-up probes",
        excpt.triggered == true &&
        excpt.cause == CAUSE_SAF &&
        excpt.tval == second_half_vaddr
    );

    ai_micro_mab_retry_byte4_exact_store_restore_pmp_full_access();

    TEST_SETUP_EXCEPT();
    ai_micro_mab_retry_byte4_exact_store_translated_sd(
        aligned_upper_store_vaddr, aligned_upper_store_value_first);
    TEST_ASSERT("after repair, the first same-boundary translated upper-half aligned store should complete cleanly before the refaulted retry-4B+4B-store then pure-producer-tail-byte store plus the upper-half and byte15 follow-up probes",
        excpt.triggered == false
    );

    TEST_SETUP_EXCEPT();
    ai_micro_mab_retry_byte4_exact_store_translated_sd(
        aligned_upper_store_vaddr, aligned_upper_store_value_second);
    TEST_ASSERT("after one upper-half aligned translated store has already completed on the repaired boundary, the second upper-half aligned translated store should stay trap-free before the refaulted retry-4B+4B-store then pure-producer-tail-byte store plus the upper-half and byte15 follow-up probes",
        excpt.triggered == false
    );

    TEST_SETUP_EXCEPT();
    ai_micro_mab_retry_byte4_exact_store_translated_sd(
        aligned_upper_store_vaddr, aligned_upper_store_value_third);
    TEST_ASSERT("after two upper-half aligned translated stores have already completed on the repaired boundary, the third upper-half aligned translated store should stay trap-free before the refaulted retry-4B+4B-store then pure-producer-tail-byte store plus the upper-half and byte15 follow-up probes",
        excpt.triggered == false
    );

    TEST_SETUP_EXCEPT();
    ai_micro_mab_retry_byte4_exact_store_translated_sd(
        aligned_upper_store_vaddr, aligned_upper_store_value_fourth);
    TEST_ASSERT("after three upper-half aligned translated stores have already completed on the repaired boundary, the fourth upper-half aligned translated store should stay trap-free and become the latest committed high-half payload before the refaulted retry-4B+4B-store then pure-producer-tail-byte store plus the upper-half and byte15 follow-up probes",
        excpt.triggered == false
    );

    ai_micro_mab_retry_byte4_exact_store_configure_pmp_window(
        deny_start, deny_end,
        AI_MICRO_MAB_RETRY_BYTE4_EXACT_STORE_PMP_TOR_NO_W);

    TEST_SETUP_EXCEPT();
    ai_micro_mab_retry_byte4_exact_store_translated_sd(
        fault_vaddr, refault_store_value);
    TEST_ASSERT("after four upper-half aligned translated stores have already completed on the repaired boundary, restoring the upper-half deny window and reissuing the original 4B+4B misaligned store should refresh the same precise second-split store access fault metadata before the pure-producer-tail-byte store plus the upper-half and byte15 follow-up probes",
        excpt.triggered == true &&
        excpt.cause == CAUSE_SAF &&
        excpt.tval == second_half_vaddr
    );

    ai_micro_mab_retry_byte4_exact_store_restore_pmp_full_access();

    TEST_SETUP_EXCEPT();
    ai_micro_mab_retry_byte4_exact_store_translated_sd(
        fault_vaddr, retry_4b4b_store_value);
    TEST_ASSERT("after the refault episode is repaired again, retrying the same 4B+4B translated store as a success store should clear stale exception metadata before the immediate pure-producer-tail-byte store and the upper-half plus byte15 follow-up probes",
        excpt.triggered == false
    );

    TEST_SETUP_EXCEPT();
    ai_micro_mab_retry_byte4_exact_store_translated_sb(
        followup_pure_producer_tail_byte_store_vaddr,
        followup_pure_producer_tail_byte_store_value);
    TEST_ASSERT("after the retried original-template 4B+4B store has already succeeded on the same boundary, the immediate translated pure-producer tail-byte store should also stay trap-free and must not let a retired fake-crosspage exact 4-byte high-half owner/template leak back into bytes8-11 or widen beyond byte15 while bytes12-14 stay owned by the latest aligned producer-side image",
        excpt.triggered == false
    );

    ai_micro_mab_retry_byte4_exact_store_overlay_little_endian_bytes(
        expected_boundary, boundary_seed, 16U, 8U,
        aligned_upper_store_value_fourth, sizeof(uint64_t));
    ai_micro_mab_retry_byte4_exact_store_overlay_little_endian_bytes(
        expected_boundary, expected_boundary, 16U, 4U,
        retry_4b4b_store_value, sizeof(uint64_t));
    ai_micro_mab_retry_byte4_exact_store_overlay_little_endian_bytes(
        expected_boundary, expected_boundary, 16U, 15U,
        (uint64_t)followup_pure_producer_tail_byte_store_value,
        sizeof(uint8_t));
    expected_upper_half_load_value =
        ai_micro_mab_retry_byte4_exact_store_read_little_endian(
            expected_boundary + 8U, sizeof(uint64_t));
    expected_tail_last_halfword_high_byte_value =
        (uint64_t)expected_boundary[15];

    TEST_SETUP_EXCEPT();
    observed_upper_half_load_value =
        ai_micro_mab_retry_byte4_exact_store_translated_ld(
            upper_half_load_vaddr);
    TEST_ASSERT("after the pure-producer tail-byte store has already refreshed byte15 on top of the retried 4B+4B image, the immediate aligned upper-half translated load at boundary+8 should clear stale exception metadata and read the merged latest image of retry bytes8-11, retained producer bytes12-14, and the new tail byte before the following byte15 confirmation probe",
        excpt.triggered == false &&
        observed_upper_half_load_value == expected_upper_half_load_value
    );

    TEST_SETUP_EXCEPT();
    observed_tail_last_halfword_high_byte_value =
        ai_micro_mab_retry_byte4_exact_store_translated_lbu(
            tail_last_halfword_high_byte_load_vaddr);
    TEST_ASSERT("after the aligned upper-half load probe has already shown the merged latest image on top of the retried 4B+4B store, the following translated byte15 confirmation probe should still clear stale exception metadata and read the new tail byte instead of falling back to stale exact-BYTE4 state",
        excpt.triggered == false &&
        observed_tail_last_halfword_high_byte_value ==
            expected_tail_last_halfword_high_byte_value
    );

    TEST_ASSERT("after the prolonged upper-half producer, repaired refault, retried 4B+4B success store, immediate pure-producer tail-byte store, the upper-half load probe, and the byte15 probe, the final boundary bytes should preserve bytes4-11 from the retried image, keep bytes12-14 from the latest aligned upper-half image, and refresh only byte15",
        ai_micro_mab_retry_byte4_exact_store_bytes_match(
            boundary_phys, expected_boundary, 16U)
    );

    TEST_ASSERT("guard regions around the same-page boundary should remain unchanged through the repeated 4B+4B fault, four upper-half aligned stores, refault, retried success store, immediate pure-producer-tail-byte follow-up store, the aligned upper-half load probe, and the byte15 probe sequence",
        ai_micro_mab_retry_byte4_exact_store_bytes_match(
            guard_before_phys, guard_before_seed, 8U) &&
        ai_micro_mab_retry_byte4_exact_store_bytes_match(
            guard_after_phys, guard_after_seed, 8U)
    );

    ai_micro_mab_retry_byte4_exact_store_reset_env();

TEST_END(
        "ai_micro_m_samepage_cross16b_store_refault_upperx4_retry_4b4b_store_then_pure_producer_tail_byte_store_then_upper_half_load_then_tail_last_halfword_high_byte_load_corner");
}

bool ai_micro_m_samepage_cross16b_store_refault_upperx4_retry_4b4b_store_then_pure_producer_tail_byte_store_then_upper_half_load_then_tail_last_halfword_load_corner()
{
    TEST_START();

    static const uint8_t boundary_seed[16] = {
        0x39, 0x28, 0x17, 0x06, 0xf5, 0xe4, 0xd3, 0xc2,
        0xb1, 0xa0, 0x9f, 0x8e, 0x7d, 0x6c, 0x5b, 0x4a,
    };
    static const uint8_t guard_before_seed[8] = {
        0x18, 0x2a, 0x3c, 0x4e, 0x60, 0x72, 0x84, 0x96,
    };
    static const uint8_t guard_after_seed[8] = {
        0xa7, 0xb9, 0xcb, 0xdd, 0xef, 0x01, 0x13, 0x25,
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
    const uintptr_t followup_pure_producer_tail_byte_store_vaddr =
        aligned_upper_store_vaddr + 7U;
    const uintptr_t upper_half_load_vaddr = aligned_upper_store_vaddr;
    const uintptr_t tail_last_halfword_load_vaddr =
        aligned_upper_store_vaddr + 6U;
    const uintptr_t deny_start = boundary_phys + 8U;
    const uintptr_t deny_end = deny_start + PAGE_SIZE;
    const uint64_t first_fault_store_value = 0x65768798a9bacbdcULL;
    const uint64_t refault_store_value = 0xfd0e1f2031425364ULL;
    const uint64_t aligned_upper_store_value_first = 0x768798a9bacbdcedULL;
    const uint64_t aligned_upper_store_value_second = 0x0e1f203142536475ULL;
    const uint64_t aligned_upper_store_value_third = 0x8798a9bacbdcedfeULL;
    const uint64_t aligned_upper_store_value_fourth = 0x1f20314253647586ULL;
    const uint64_t retry_4b4b_store_value = 0x98a9bacbdcedfe0fULL;
    const uint8_t followup_pure_producer_tail_byte_store_value = 0x7eU;
    uint8_t expected_boundary[16];
    uint64_t expected_upper_half_load_value = 0ULL;
    uint64_t observed_upper_half_load_value = 0ULL;
    uint64_t expected_tail_last_halfword_load_value = 0ULL;
    uint64_t observed_tail_last_halfword_load_value = 0ULL;

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
    TEST_ASSERT("for the no-H same-page cross-16B translated scalar store, the first repeated 4B+4B second-split write fault must still report store access fault on the true second-half start address before the retried-success then pure-producer-tail-byte store plus the upper-half and tail-last-halfword follow-up probes",
        excpt.triggered == true &&
        excpt.cause == CAUSE_SAF &&
        excpt.tval == second_half_vaddr
    );

    TEST_SETUP_EXCEPT();
    ai_micro_mab_retry_byte4_exact_store_translated_sd(
        fault_vaddr, first_fault_store_value);
    TEST_ASSERT("repeating the same no-H second-split 4B+4B write fault keeps the same precise metadata before the retried-success then pure-producer-tail-byte store plus the upper-half and tail-last-halfword follow-up probes",
        excpt.triggered == true &&
        excpt.cause == CAUSE_SAF &&
        excpt.tval == second_half_vaddr
    );

    ai_micro_mab_retry_byte4_exact_store_restore_pmp_full_access();

    TEST_SETUP_EXCEPT();
    ai_micro_mab_retry_byte4_exact_store_translated_sd(
        aligned_upper_store_vaddr, aligned_upper_store_value_first);
    TEST_ASSERT("after repair, the first same-boundary translated upper-half aligned store should complete cleanly before the refaulted retry-4B+4B-store then pure-producer-tail-byte store plus the upper-half and tail-last-halfword follow-up probes",
        excpt.triggered == false
    );

    TEST_SETUP_EXCEPT();
    ai_micro_mab_retry_byte4_exact_store_translated_sd(
        aligned_upper_store_vaddr, aligned_upper_store_value_second);
    TEST_ASSERT("after one upper-half aligned translated store has already completed on the repaired boundary, the second upper-half aligned translated store should stay trap-free before the refaulted retry-4B+4B-store then pure-producer-tail-byte store plus the upper-half and tail-last-halfword follow-up probes",
        excpt.triggered == false
    );

    TEST_SETUP_EXCEPT();
    ai_micro_mab_retry_byte4_exact_store_translated_sd(
        aligned_upper_store_vaddr, aligned_upper_store_value_third);
    TEST_ASSERT("after two upper-half aligned translated stores have already completed on the repaired boundary, the third upper-half aligned translated store should stay trap-free before the refaulted retry-4B+4B-store then pure-producer-tail-byte store plus the upper-half and tail-last-halfword follow-up probes",
        excpt.triggered == false
    );

    TEST_SETUP_EXCEPT();
    ai_micro_mab_retry_byte4_exact_store_translated_sd(
        aligned_upper_store_vaddr, aligned_upper_store_value_fourth);
    TEST_ASSERT("after three upper-half aligned translated stores have already completed on the repaired boundary, the fourth upper-half aligned translated store should stay trap-free and become the latest committed high-half payload before the refaulted retry-4B+4B-store then pure-producer-tail-byte store plus the upper-half and tail-last-halfword follow-up probes",
        excpt.triggered == false
    );

    ai_micro_mab_retry_byte4_exact_store_configure_pmp_window(
        deny_start, deny_end,
        AI_MICRO_MAB_RETRY_BYTE4_EXACT_STORE_PMP_TOR_NO_W);

    TEST_SETUP_EXCEPT();
    ai_micro_mab_retry_byte4_exact_store_translated_sd(
        fault_vaddr, refault_store_value);
    TEST_ASSERT("after four upper-half aligned translated stores have already completed on the repaired boundary, restoring the upper-half deny window and reissuing the original 4B+4B misaligned store should refresh the same precise second-split store access fault metadata before the pure-producer-tail-byte store plus the upper-half and tail-last-halfword follow-up probes",
        excpt.triggered == true &&
        excpt.cause == CAUSE_SAF &&
        excpt.tval == second_half_vaddr
    );

    ai_micro_mab_retry_byte4_exact_store_restore_pmp_full_access();

    TEST_SETUP_EXCEPT();
    ai_micro_mab_retry_byte4_exact_store_translated_sd(
        fault_vaddr, retry_4b4b_store_value);
    TEST_ASSERT("after the refault episode is repaired again, retrying the same 4B+4B translated store as a success store should clear stale exception metadata before the immediate pure-producer-tail-byte store and the upper-half plus tail-last-halfword follow-up probes",
        excpt.triggered == false
    );

    TEST_SETUP_EXCEPT();
    ai_micro_mab_retry_byte4_exact_store_translated_sb(
        followup_pure_producer_tail_byte_store_vaddr,
        followup_pure_producer_tail_byte_store_value);
    TEST_ASSERT("after the retried original-template 4B+4B store has already succeeded on the same boundary, the immediate translated pure-producer tail-byte store should also stay trap-free and must not let a retired fake-crosspage exact 4-byte high-half owner/template leak back into bytes8-11 or widen beyond byte15 while bytes12-14 stay owned by the latest aligned producer-side image",
        excpt.triggered == false
    );

    ai_micro_mab_retry_byte4_exact_store_overlay_little_endian_bytes(
        expected_boundary, boundary_seed, 16U, 8U,
        aligned_upper_store_value_fourth, sizeof(uint64_t));
    ai_micro_mab_retry_byte4_exact_store_overlay_little_endian_bytes(
        expected_boundary, expected_boundary, 16U, 4U,
        retry_4b4b_store_value, sizeof(uint64_t));
    ai_micro_mab_retry_byte4_exact_store_overlay_little_endian_bytes(
        expected_boundary, expected_boundary, 16U, 15U,
        (uint64_t)followup_pure_producer_tail_byte_store_value,
        sizeof(uint8_t));
    expected_upper_half_load_value =
        ai_micro_mab_retry_byte4_exact_store_read_little_endian(
            expected_boundary + 8U, sizeof(uint64_t));
    expected_tail_last_halfword_load_value =
        ai_micro_mab_retry_byte4_exact_store_read_little_endian(
            expected_boundary + 14U, sizeof(uint16_t));

    TEST_SETUP_EXCEPT();
    observed_upper_half_load_value =
        ai_micro_mab_retry_byte4_exact_store_translated_ld(
            upper_half_load_vaddr);
    TEST_ASSERT("after the pure-producer tail-byte store has already refreshed byte15 on top of the retried 4B+4B image, the immediate aligned upper-half translated load at boundary+8 should clear stale exception metadata and read the merged latest image of retry bytes8-11, retained producer bytes12-14, and the new tail byte before the following tail-last-halfword probe",
        excpt.triggered == false &&
        observed_upper_half_load_value == expected_upper_half_load_value
    );

    TEST_SETUP_EXCEPT();
    observed_tail_last_halfword_load_value =
        ai_micro_mab_retry_byte4_exact_store_translated_lhu(
            tail_last_halfword_load_vaddr);
    TEST_ASSERT("after the aligned upper-half load probe has already shown the merged latest image on top of the retried 4B+4B store, the following translated tail-last-halfword load at boundary+14 should still clear stale exception metadata and return the retained byte14 plus the new tail byte instead of falling back to stale exact-BYTE4 state",
        excpt.triggered == false &&
        observed_tail_last_halfword_load_value ==
            expected_tail_last_halfword_load_value
    );

    TEST_ASSERT("after the prolonged upper-half producer, repaired refault, retried 4B+4B success store, immediate pure-producer tail-byte store, the upper-half load probe, and the tail-last-halfword probe, the final boundary bytes should preserve bytes4-11 from the retried image, keep bytes12-14 from the latest aligned upper-half image, and refresh only byte15",
        ai_micro_mab_retry_byte4_exact_store_bytes_match(
            boundary_phys, expected_boundary, 16U)
    );

    TEST_ASSERT("guard regions around the same-page boundary should remain unchanged through the repeated 4B+4B fault, four upper-half aligned stores, refault, retried success store, immediate pure-producer-tail-byte follow-up store, the aligned upper-half load probe, and the tail-last-halfword probe sequence",
        ai_micro_mab_retry_byte4_exact_store_bytes_match(
            guard_before_phys, guard_before_seed, 8U) &&
        ai_micro_mab_retry_byte4_exact_store_bytes_match(
            guard_after_phys, guard_after_seed, 8U)
    );

    ai_micro_mab_retry_byte4_exact_store_reset_env();

    TEST_END(
        "ai_micro_m_samepage_cross16b_store_refault_upperx4_retry_4b4b_store_then_pure_producer_tail_byte_store_then_upper_half_load_then_tail_last_halfword_load_corner");
}

bool ai_micro_m_samepage_cross16b_store_refault_upperx4_retry_4b4b_store_then_pure_producer_tail_byte_store_then_upper_half_load_then_upper_tail_word_load_corner()
{
    TEST_START();

    static const uint8_t boundary_seed[16] = {
        0x39, 0x28, 0x17, 0x06, 0xf5, 0xe4, 0xd3, 0xc2,
        0xb1, 0xa0, 0x9f, 0x8e, 0x7d, 0x6c, 0x5b, 0x4a,
    };
    static const uint8_t guard_before_seed[8] = {
        0x18, 0x2a, 0x3c, 0x4e, 0x60, 0x72, 0x84, 0x96,
    };
    static const uint8_t guard_after_seed[8] = {
        0xa7, 0xb9, 0xcb, 0xdd, 0xef, 0x01, 0x13, 0x25,
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
    const uintptr_t followup_pure_producer_tail_byte_store_vaddr =
        aligned_upper_store_vaddr + 7U;
    const uintptr_t upper_half_load_vaddr = aligned_upper_store_vaddr;
    const uintptr_t upper_tail_word_load_vaddr =
        aligned_upper_store_vaddr + 4U;
    const uintptr_t deny_start = boundary_phys + 8U;
    const uintptr_t deny_end = deny_start + PAGE_SIZE;
    const uint64_t first_fault_store_value = 0x65768798a9bacbdcULL;
    const uint64_t refault_store_value = 0xfd0e1f2031425364ULL;
    const uint64_t aligned_upper_store_value_first = 0x768798a9bacbdcedULL;
    const uint64_t aligned_upper_store_value_second = 0x0e1f203142536475ULL;
    const uint64_t aligned_upper_store_value_third = 0x8798a9bacbdcedfeULL;
    const uint64_t aligned_upper_store_value_fourth = 0x1f20314253647586ULL;
    const uint64_t retry_4b4b_store_value = 0x98a9bacbdcedfe0fULL;
    const uint8_t followup_pure_producer_tail_byte_store_value = 0x7eU;
    uint8_t expected_boundary[16];
    uint64_t expected_upper_half_load_value = 0ULL;
    uint64_t observed_upper_half_load_value = 0ULL;
    uint64_t expected_upper_tail_word_load_value = 0ULL;
    uint64_t observed_upper_tail_word_load_value = 0ULL;

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
    TEST_ASSERT("for the no-H same-page cross-16B translated scalar store, the first repeated 4B+4B second-split write fault must still report store access fault on the true second-half start address before the retried-success then pure-producer-tail-byte store plus the upper-half and upper-tail-word follow-up probes",
        excpt.triggered == true &&
        excpt.cause == CAUSE_SAF &&
        excpt.tval == second_half_vaddr
    );

    TEST_SETUP_EXCEPT();
    ai_micro_mab_retry_byte4_exact_store_translated_sd(
        fault_vaddr, first_fault_store_value);
    TEST_ASSERT("repeating the same no-H second-split 4B+4B write fault keeps the same precise metadata before the retried-success then pure-producer-tail-byte store plus the upper-half and upper-tail-word follow-up probes",
        excpt.triggered == true &&
        excpt.cause == CAUSE_SAF &&
        excpt.tval == second_half_vaddr
    );

    ai_micro_mab_retry_byte4_exact_store_restore_pmp_full_access();

    TEST_SETUP_EXCEPT();
    ai_micro_mab_retry_byte4_exact_store_translated_sd(
        aligned_upper_store_vaddr, aligned_upper_store_value_first);
    TEST_ASSERT("after repair, the first same-boundary translated upper-half aligned store should complete cleanly before the refaulted retry-4B+4B-store then pure-producer-tail-byte store plus the upper-half and upper-tail-word follow-up probes",
        excpt.triggered == false
    );

    TEST_SETUP_EXCEPT();
    ai_micro_mab_retry_byte4_exact_store_translated_sd(
        aligned_upper_store_vaddr, aligned_upper_store_value_second);
    TEST_ASSERT("after one upper-half aligned translated store has already completed on the repaired boundary, the second upper-half aligned translated store should stay trap-free before the refaulted retry-4B+4B-store then pure-producer-tail-byte store plus the upper-half and upper-tail-word follow-up probes",
        excpt.triggered == false
    );

    TEST_SETUP_EXCEPT();
    ai_micro_mab_retry_byte4_exact_store_translated_sd(
        aligned_upper_store_vaddr, aligned_upper_store_value_third);
    TEST_ASSERT("after two upper-half aligned translated stores have already completed on the repaired boundary, the third upper-half aligned translated store should stay trap-free before the refaulted retry-4B+4B-store then pure-producer-tail-byte store plus the upper-half and upper-tail-word follow-up probes",
        excpt.triggered == false
    );

    TEST_SETUP_EXCEPT();
    ai_micro_mab_retry_byte4_exact_store_translated_sd(
        aligned_upper_store_vaddr, aligned_upper_store_value_fourth);
    TEST_ASSERT("after three upper-half aligned translated stores have already completed on the repaired boundary, the fourth upper-half aligned translated store should stay trap-free and become the latest committed high-half payload before the refaulted retry-4B+4B-store then pure-producer-tail-byte store plus the upper-half and upper-tail-word follow-up probes",
        excpt.triggered == false
    );

    ai_micro_mab_retry_byte4_exact_store_configure_pmp_window(
        deny_start, deny_end,
        AI_MICRO_MAB_RETRY_BYTE4_EXACT_STORE_PMP_TOR_NO_W);

    TEST_SETUP_EXCEPT();
    ai_micro_mab_retry_byte4_exact_store_translated_sd(
        fault_vaddr, refault_store_value);
    TEST_ASSERT("after four upper-half aligned translated stores have already completed on the repaired boundary, restoring the upper-half deny window and reissuing the original 4B+4B misaligned store should refresh the same precise second-split store access fault metadata before the pure-producer-tail-byte store plus the upper-half and upper-tail-word follow-up probes",
        excpt.triggered == true &&
        excpt.cause == CAUSE_SAF &&
        excpt.tval == second_half_vaddr
    );

    ai_micro_mab_retry_byte4_exact_store_restore_pmp_full_access();

    TEST_SETUP_EXCEPT();
    ai_micro_mab_retry_byte4_exact_store_translated_sd(
        fault_vaddr, retry_4b4b_store_value);
    TEST_ASSERT("after the refault episode is repaired again, retrying the same 4B+4B translated store as a success store should clear stale exception metadata before the immediate pure-producer-tail-byte store and the upper-half plus upper-tail-word follow-up probes",
        excpt.triggered == false
    );

    TEST_SETUP_EXCEPT();
    ai_micro_mab_retry_byte4_exact_store_translated_sb(
        followup_pure_producer_tail_byte_store_vaddr,
        followup_pure_producer_tail_byte_store_value);
    TEST_ASSERT("after the retried original-template 4B+4B store has already succeeded on the same boundary, the immediate translated pure-producer tail-byte store should also stay trap-free and must not let a retired fake-crosspage exact 4-byte high-half owner/template leak back into bytes8-11 or widen beyond byte15 while bytes12-14 stay owned by the latest aligned producer-side image",
        excpt.triggered == false
    );

    ai_micro_mab_retry_byte4_exact_store_overlay_little_endian_bytes(
        expected_boundary, boundary_seed, 16U, 8U,
        aligned_upper_store_value_fourth, sizeof(uint64_t));
    ai_micro_mab_retry_byte4_exact_store_overlay_little_endian_bytes(
        expected_boundary, expected_boundary, 16U, 4U,
        retry_4b4b_store_value, sizeof(uint64_t));
    ai_micro_mab_retry_byte4_exact_store_overlay_little_endian_bytes(
        expected_boundary, expected_boundary, 16U, 15U,
        (uint64_t)followup_pure_producer_tail_byte_store_value,
        sizeof(uint8_t));
    expected_upper_half_load_value =
        ai_micro_mab_retry_byte4_exact_store_read_little_endian(
            expected_boundary + 8U, sizeof(uint64_t));
    expected_upper_tail_word_load_value =
        ai_micro_mab_retry_byte4_exact_store_read_little_endian(
            expected_boundary + 12U, sizeof(uint32_t));

    TEST_SETUP_EXCEPT();
    observed_upper_half_load_value =
        ai_micro_mab_retry_byte4_exact_store_translated_ld(
            upper_half_load_vaddr);
    TEST_ASSERT("after the pure-producer tail-byte store has already refreshed byte15 on top of the retried 4B+4B image, the immediate aligned upper-half translated load at boundary+8 should clear stale exception metadata and read the merged latest image of retry bytes8-11, retained producer bytes12-14, and the new tail byte before the following upper-tail-word probe",
        excpt.triggered == false &&
        observed_upper_half_load_value == expected_upper_half_load_value
    );

    TEST_SETUP_EXCEPT();
    observed_upper_tail_word_load_value =
        ai_micro_mab_retry_byte4_exact_store_translated_lwu(
            upper_tail_word_load_vaddr);
    TEST_ASSERT("after the aligned upper-half load probe has already shown the merged latest image on top of the retried 4B+4B store, the following translated upper-tail-word load at boundary+12 should still clear stale exception metadata and return retained bytes12-14 plus the new tail byte instead of falling back to stale exact-BYTE4 state",
        excpt.triggered == false &&
        observed_upper_tail_word_load_value ==
            expected_upper_tail_word_load_value
    );

    TEST_ASSERT("after the prolonged upper-half producer, repaired refault, retried 4B+4B success store, immediate pure-producer tail-byte store, the upper-half load probe, and the upper-tail-word probe, the final boundary bytes should preserve bytes4-11 from the retried image, keep bytes12-14 from the latest aligned upper-half image, and refresh only byte15",
        ai_micro_mab_retry_byte4_exact_store_bytes_match(
            boundary_phys, expected_boundary, 16U)
    );

    TEST_ASSERT("guard regions around the same-page boundary should remain unchanged through the repeated 4B+4B fault, four upper-half aligned stores, refault, retried success store, immediate pure-producer-tail-byte follow-up store, the aligned upper-half load probe, and the upper-tail-word probe sequence",
        ai_micro_mab_retry_byte4_exact_store_bytes_match(
            guard_before_phys, guard_before_seed, 8U) &&
        ai_micro_mab_retry_byte4_exact_store_bytes_match(
            guard_after_phys, guard_after_seed, 8U)
    );

    ai_micro_mab_retry_byte4_exact_store_reset_env();

    TEST_END(
        "ai_micro_m_samepage_cross16b_store_refault_upperx4_retry_4b4b_store_then_pure_producer_tail_byte_store_then_upper_half_load_then_upper_tail_word_load_corner");
}
