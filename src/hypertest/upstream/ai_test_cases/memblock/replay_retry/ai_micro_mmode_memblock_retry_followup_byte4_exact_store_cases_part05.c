#include "ai_micro_mmode_memblock_retry_followup_byte4_exact_store_common.h"

bool ai_micro_m_samepage_cross16b_store_refault_upperx4_retry_4b4b_store_then_pure_producer_tail_byte_store_then_upper_tail_word_load_then_first_upper_word_load_corner()
{
    TEST_START();

    static const uint8_t boundary_seed[16] = {
        0x59, 0x48, 0x37, 0x26, 0x15, 0x04, 0xf3, 0xe2,
        0xd1, 0xc0, 0xaf, 0x9e, 0x8d, 0x7c, 0x6b, 0x5a,
    };
    static const uint8_t guard_before_seed[8] = {
        0x1a, 0x2c, 0x3e, 0x40, 0x52, 0x64, 0x76, 0x88,
    };
    static const uint8_t guard_after_seed[8] = {
        0x99, 0xab, 0xbd, 0xcf, 0xe1, 0xf3, 0x15, 0x27,
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
    const uintptr_t upper_tail_word_load_vaddr =
        aligned_upper_store_vaddr + 4U;
    const uintptr_t first_upper_word_load_vaddr =
        aligned_upper_store_vaddr;
    const uintptr_t deny_start = boundary_phys + 8U;
    const uintptr_t deny_end = deny_start + PAGE_SIZE;
    const uint64_t first_fault_store_value = 0x8798a9bacbdcedfeULL;
    const uint64_t refault_store_value = 0x1f20314253647586ULL;
    const uint64_t aligned_upper_store_value_first = 0x98a9bacbdcedfe0fULL;
    const uint64_t aligned_upper_store_value_second = 0x2031425364758697ULL;
    const uint64_t aligned_upper_store_value_third = 0xa9bacbdcedfe0f10ULL;
    const uint64_t aligned_upper_store_value_fourth = 0x31425364758697a8ULL;
    const uint64_t retry_4b4b_store_value = 0xbacbdcedfe0f1021ULL;
    const uint8_t followup_pure_producer_tail_byte_store_value = 0xa0U;
    uint8_t expected_boundary[16];
    uint64_t expected_upper_tail_word_load_value = 0ULL;
    uint64_t observed_upper_tail_word_load_value = 0ULL;
    uint64_t expected_first_upper_word_load_value = 0ULL;
    uint64_t observed_first_upper_word_load_value = 0ULL;

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
    TEST_ASSERT("for the no-H same-page cross-16B translated scalar store, the first repeated 4B+4B second-split write fault must still report store access fault on the true second-half start address before the retried-success then pure-producer-tail-byte store plus the upper-tail-word and first-upper-word follow-up probes",
        excpt.triggered == true &&
        excpt.cause == CAUSE_SAF &&
        excpt.tval == second_half_vaddr
    );

    TEST_SETUP_EXCEPT();
    ai_micro_mab_retry_byte4_exact_store_translated_sd(
        fault_vaddr, first_fault_store_value);
    TEST_ASSERT("repeating the same no-H second-split 4B+4B write fault keeps the same precise metadata before the retried-success then pure-producer-tail-byte store plus the upper-tail-word and first-upper-word follow-up probes",
        excpt.triggered == true &&
        excpt.cause == CAUSE_SAF &&
        excpt.tval == second_half_vaddr
    );

    ai_micro_mab_retry_byte4_exact_store_restore_pmp_full_access();

    TEST_SETUP_EXCEPT();
    ai_micro_mab_retry_byte4_exact_store_translated_sd(
        aligned_upper_store_vaddr, aligned_upper_store_value_first);
    TEST_ASSERT("after repair, the first same-boundary translated upper-half aligned store should complete cleanly before the refaulted retry-4B+4B-store then pure-producer-tail-byte store plus the upper-tail-word and first-upper-word follow-up probes",
        excpt.triggered == false
    );

    TEST_SETUP_EXCEPT();
    ai_micro_mab_retry_byte4_exact_store_translated_sd(
        aligned_upper_store_vaddr, aligned_upper_store_value_second);
    TEST_ASSERT("after one upper-half aligned translated store has already completed on the repaired boundary, the second upper-half aligned translated store should stay trap-free before the refaulted retry-4B+4B-store then pure-producer-tail-byte store plus the upper-tail-word and first-upper-word follow-up probes",
        excpt.triggered == false
    );

    TEST_SETUP_EXCEPT();
    ai_micro_mab_retry_byte4_exact_store_translated_sd(
        aligned_upper_store_vaddr, aligned_upper_store_value_third);
    TEST_ASSERT("after two upper-half aligned translated stores have already completed on the repaired boundary, the third upper-half aligned translated store should stay trap-free before the refaulted retry-4B+4B-store then pure-producer-tail-byte store plus the upper-tail-word and first-upper-word follow-up probes",
        excpt.triggered == false
    );

    TEST_SETUP_EXCEPT();
    ai_micro_mab_retry_byte4_exact_store_translated_sd(
        aligned_upper_store_vaddr, aligned_upper_store_value_fourth);
    TEST_ASSERT("after three upper-half aligned translated stores have already completed on the repaired boundary, the fourth upper-half aligned translated store should stay trap-free and become the latest committed high-half payload before the refaulted retry-4B+4B-store then pure-producer-tail-byte store plus the upper-tail-word and first-upper-word follow-up probes",
        excpt.triggered == false
    );

    ai_micro_mab_retry_byte4_exact_store_configure_pmp_window(
        deny_start, deny_end,
        AI_MICRO_MAB_RETRY_BYTE4_EXACT_STORE_PMP_TOR_NO_W);

    TEST_SETUP_EXCEPT();
    ai_micro_mab_retry_byte4_exact_store_translated_sd(
        fault_vaddr, refault_store_value);
    TEST_ASSERT("after four upper-half aligned translated stores have already completed on the repaired boundary, restoring the upper-half deny window and reissuing the original 4B+4B misaligned store should refresh the same precise second-split store access fault metadata before the pure-producer-tail-byte store plus the upper-tail-word and first-upper-word follow-up probes",
        excpt.triggered == true &&
        excpt.cause == CAUSE_SAF &&
        excpt.tval == second_half_vaddr
    );

    ai_micro_mab_retry_byte4_exact_store_restore_pmp_full_access();

    TEST_SETUP_EXCEPT();
    ai_micro_mab_retry_byte4_exact_store_translated_sd(
        fault_vaddr, retry_4b4b_store_value);
    TEST_ASSERT("after the refault episode is repaired again, retrying the same 4B+4B translated store as a success store should clear stale exception metadata before the immediate pure-producer-tail-byte store and the upper-tail-word plus first-upper-word follow-up probes",
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
    expected_upper_tail_word_load_value =
        ai_micro_mab_retry_byte4_exact_store_read_little_endian(
            expected_boundary + 12U, sizeof(uint32_t));
    expected_first_upper_word_load_value =
        ai_micro_mab_retry_byte4_exact_store_read_little_endian(
            expected_boundary + 8U, sizeof(uint32_t));

    TEST_SETUP_EXCEPT();
    observed_upper_tail_word_load_value =
        ai_micro_mab_retry_byte4_exact_store_translated_lwu(
            upper_tail_word_load_vaddr);
    TEST_ASSERT("after the pure-producer tail-byte store has already refreshed byte15 on top of the retried 4B+4B image, the immediate translated upper-tail-word load at boundary+12 should clear stale exception metadata and return retained bytes12-14 plus the new tail byte before the following first-upper-word probe",
        excpt.triggered == false &&
        observed_upper_tail_word_load_value ==
            expected_upper_tail_word_load_value
    );

    TEST_SETUP_EXCEPT();
    observed_first_upper_word_load_value =
        ai_micro_mab_retry_byte4_exact_store_translated_lwu(
            first_upper_word_load_vaddr);
    TEST_ASSERT("after the upper-tail-word probe has already shown the merged latest tail image on top of the retried 4B+4B store, the following translated first-upper-word load at boundary+8 should still clear stale exception metadata and keep the retried bytes8-11 image intact instead of falling back to stale exact-BYTE4 state",
        excpt.triggered == false &&
        observed_first_upper_word_load_value ==
            expected_first_upper_word_load_value
    );

    TEST_ASSERT("after the prolonged upper-half producer, repaired refault, retried 4B+4B success store, immediate pure-producer tail-byte store, the upper-tail-word probe, and the first-upper-word probe, the final boundary bytes should preserve bytes4-11 from the retried image, keep bytes12-14 from the latest aligned upper-half image, and refresh only byte15",
        ai_micro_mab_retry_byte4_exact_store_bytes_match(
            boundary_phys, expected_boundary, 16U)
    );

    TEST_ASSERT("guard regions around the same-page boundary should remain unchanged through the repeated 4B+4B fault, four upper-half aligned stores, refault, retried success store, immediate pure-producer-tail-byte follow-up store, the upper-tail-word probe, and the first-upper-word probe sequence",
        ai_micro_mab_retry_byte4_exact_store_bytes_match(
            guard_before_phys, guard_before_seed, 8U) &&
        ai_micro_mab_retry_byte4_exact_store_bytes_match(
            guard_after_phys, guard_after_seed, 8U)
    );

    ai_micro_mab_retry_byte4_exact_store_reset_env();

    TEST_END(
        "ai_micro_m_samepage_cross16b_store_refault_upperx4_retry_4b4b_store_then_pure_producer_tail_byte_store_then_upper_tail_word_load_then_first_upper_word_load_corner");
}

bool ai_micro_m_samepage_cross16b_store_refault_upperx4_retry_4b4b_store_then_pure_producer_tail_byte_store_then_upper_tail_word_load_then_first_upper_halfword_load_corner()
{
    TEST_START();

    static const uint8_t boundary_seed[16] = {
        0x59, 0x48, 0x37, 0x26, 0x15, 0x04, 0xf3, 0xe2,
        0xd1, 0xc0, 0xaf, 0x9e, 0x8d, 0x7c, 0x6b, 0x5a,
    };
    static const uint8_t guard_before_seed[8] = {
        0x1a, 0x2c, 0x3e, 0x40, 0x52, 0x64, 0x76, 0x88,
    };
    static const uint8_t guard_after_seed[8] = {
        0x99, 0xab, 0xbd, 0xcf, 0xe1, 0xf3, 0x15, 0x27,
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
    const uintptr_t upper_tail_word_load_vaddr =
        aligned_upper_store_vaddr + 4U;
    const uintptr_t first_upper_halfword_load_vaddr =
        aligned_upper_store_vaddr;
    const uintptr_t deny_start = boundary_phys + 8U;
    const uintptr_t deny_end = deny_start + PAGE_SIZE;
    const uint64_t first_fault_store_value = 0x8798a9bacbdcedfeULL;
    const uint64_t refault_store_value = 0x1f20314253647586ULL;
    const uint64_t aligned_upper_store_value_first = 0x98a9bacbdcedfe0fULL;
    const uint64_t aligned_upper_store_value_second = 0x2031425364758697ULL;
    const uint64_t aligned_upper_store_value_third = 0xa9bacbdcedfe0f10ULL;
    const uint64_t aligned_upper_store_value_fourth = 0x31425364758697a8ULL;
    const uint64_t retry_4b4b_store_value = 0xbacbdcedfe0f1021ULL;
    const uint8_t followup_pure_producer_tail_byte_store_value = 0xa0U;
    uint8_t expected_boundary[16];
    uint64_t expected_upper_tail_word_load_value = 0ULL;
    uint64_t observed_upper_tail_word_load_value = 0ULL;
    uint64_t expected_first_upper_halfword_load_value = 0ULL;
    uint64_t observed_first_upper_halfword_load_value = 0ULL;

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
    TEST_ASSERT("for the no-H same-page cross-16B translated scalar store, the first repeated 4B+4B second-split write fault must still report store access fault on the true second-half start address before the retried-success then pure-producer-tail-byte store plus the upper-tail-word and first-upper-halfword follow-up probes",
        excpt.triggered == true &&
        excpt.cause == CAUSE_SAF &&
        excpt.tval == second_half_vaddr
    );

    TEST_SETUP_EXCEPT();
    ai_micro_mab_retry_byte4_exact_store_translated_sd(
        fault_vaddr, first_fault_store_value);
    TEST_ASSERT("repeating the same no-H second-split 4B+4B write fault keeps the same precise metadata before the retried-success then pure-producer-tail-byte store plus the upper-tail-word and first-upper-halfword follow-up probes",
        excpt.triggered == true &&
        excpt.cause == CAUSE_SAF &&
        excpt.tval == second_half_vaddr
    );

    ai_micro_mab_retry_byte4_exact_store_restore_pmp_full_access();

    TEST_SETUP_EXCEPT();
    ai_micro_mab_retry_byte4_exact_store_translated_sd(
        aligned_upper_store_vaddr, aligned_upper_store_value_first);
    TEST_ASSERT("after repair, the first same-boundary translated upper-half aligned store should complete cleanly before the refaulted retry-4B+4B-store then pure-producer-tail-byte store plus the upper-tail-word and first-upper-halfword follow-up probes",
        excpt.triggered == false
    );

    TEST_SETUP_EXCEPT();
    ai_micro_mab_retry_byte4_exact_store_translated_sd(
        aligned_upper_store_vaddr, aligned_upper_store_value_second);
    TEST_ASSERT("after one upper-half aligned translated store has already completed on the repaired boundary, the second upper-half aligned translated store should stay trap-free before the refaulted retry-4B+4B-store then pure-producer-tail-byte store plus the upper-tail-word and first-upper-halfword follow-up probes",
        excpt.triggered == false
    );

    TEST_SETUP_EXCEPT();
    ai_micro_mab_retry_byte4_exact_store_translated_sd(
        aligned_upper_store_vaddr, aligned_upper_store_value_third);
    TEST_ASSERT("after two upper-half aligned translated stores have already completed on the repaired boundary, the third upper-half aligned translated store should stay trap-free before the refaulted retry-4B+4B-store then pure-producer-tail-byte store plus the upper-tail-word and first-upper-halfword follow-up probes",
        excpt.triggered == false
    );

    TEST_SETUP_EXCEPT();
    ai_micro_mab_retry_byte4_exact_store_translated_sd(
        aligned_upper_store_vaddr, aligned_upper_store_value_fourth);
    TEST_ASSERT("after three upper-half aligned translated stores have already completed on the repaired boundary, the fourth upper-half aligned translated store should stay trap-free and become the latest committed high-half payload before the refaulted retry-4B+4B-store then pure-producer-tail-byte store plus the upper-tail-word and first-upper-halfword follow-up probes",
        excpt.triggered == false
    );

    ai_micro_mab_retry_byte4_exact_store_configure_pmp_window(
        deny_start, deny_end,
        AI_MICRO_MAB_RETRY_BYTE4_EXACT_STORE_PMP_TOR_NO_W);

    TEST_SETUP_EXCEPT();
    ai_micro_mab_retry_byte4_exact_store_translated_sd(
        fault_vaddr, refault_store_value);
    TEST_ASSERT("after four upper-half aligned translated stores have already completed on the repaired boundary, restoring the upper-half deny window and reissuing the original 4B+4B misaligned store should refresh the same precise second-split store access fault metadata before the pure-producer-tail-byte store plus the upper-tail-word and first-upper-halfword follow-up probes",
        excpt.triggered == true &&
        excpt.cause == CAUSE_SAF &&
        excpt.tval == second_half_vaddr
    );

    ai_micro_mab_retry_byte4_exact_store_restore_pmp_full_access();

    TEST_SETUP_EXCEPT();
    ai_micro_mab_retry_byte4_exact_store_translated_sd(
        fault_vaddr, retry_4b4b_store_value);
    TEST_ASSERT("after the refault episode is repaired again, retrying the same 4B+4B translated store as a success store should clear stale exception metadata before the immediate pure-producer-tail-byte store and the upper-tail-word plus first-upper-halfword follow-up probes",
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
    expected_upper_tail_word_load_value =
        ai_micro_mab_retry_byte4_exact_store_read_little_endian(
            expected_boundary + 12U, sizeof(uint32_t));
    expected_first_upper_halfword_load_value =
        ai_micro_mab_retry_byte4_exact_store_read_little_endian(
            expected_boundary + 8U, sizeof(uint16_t));

    TEST_SETUP_EXCEPT();
    observed_upper_tail_word_load_value =
        ai_micro_mab_retry_byte4_exact_store_translated_lwu(
            upper_tail_word_load_vaddr);
    TEST_ASSERT("after the pure-producer tail-byte store has already refreshed byte15 on top of the retried 4B+4B image, the immediate translated upper-tail-word load at boundary+12 should clear stale exception metadata and return retained bytes12-14 plus the new tail byte before the following first-upper-halfword probe",
        excpt.triggered == false &&
        observed_upper_tail_word_load_value ==
            expected_upper_tail_word_load_value
    );

    TEST_SETUP_EXCEPT();
    observed_first_upper_halfword_load_value =
        ai_micro_mab_retry_byte4_exact_store_translated_lhu(
            first_upper_halfword_load_vaddr);
    TEST_ASSERT("after the upper-tail-word probe has already shown the merged latest tail image on top of the retried 4B+4B store, the following translated first-upper-halfword load at boundary+8 should still clear stale exception metadata and keep the retried bytes8-9 image intact instead of falling back to stale exact-BYTE4 state",
        excpt.triggered == false &&
        observed_first_upper_halfword_load_value ==
            expected_first_upper_halfword_load_value
    );

    TEST_ASSERT("after the prolonged upper-half producer, repaired refault, retried 4B+4B success store, immediate pure-producer tail-byte store, the upper-tail-word probe, and the first-upper-halfword probe, the final boundary bytes should preserve bytes4-11 from the retried image, keep bytes12-14 from the latest aligned upper-half image, and refresh only byte15",
        ai_micro_mab_retry_byte4_exact_store_bytes_match(
            boundary_phys, expected_boundary, 16U)
    );

    TEST_ASSERT("guard regions around the same-page boundary should remain unchanged through the repeated 4B+4B fault, four upper-half aligned stores, refault, retried success store, immediate pure-producer-tail-byte follow-up store, the upper-tail-word probe, and the first-upper-halfword probe sequence",
        ai_micro_mab_retry_byte4_exact_store_bytes_match(
            guard_before_phys, guard_before_seed, 8U) &&
        ai_micro_mab_retry_byte4_exact_store_bytes_match(
            guard_after_phys, guard_after_seed, 8U)
    );

    ai_micro_mab_retry_byte4_exact_store_reset_env();

    TEST_END(
        "ai_micro_m_samepage_cross16b_store_refault_upperx4_retry_4b4b_store_then_pure_producer_tail_byte_store_then_upper_tail_word_load_then_first_upper_halfword_load_corner");
}

bool ai_micro_m_samepage_cross16b_store_refault_upperx4_retry_4b4b_store_then_pure_producer_tail_byte_store_then_upper_tail_word_load_then_first_upper_byte_load_corner()
{
    TEST_START();

    static const uint8_t boundary_seed[16] = {
        0x59, 0x48, 0x37, 0x26, 0x15, 0x04, 0xf3, 0xe2,
        0xd1, 0xc0, 0xaf, 0x9e, 0x8d, 0x7c, 0x6b, 0x5a,
    };
    static const uint8_t guard_before_seed[8] = {
        0x1a, 0x2c, 0x3e, 0x40, 0x52, 0x64, 0x76, 0x88,
    };
    static const uint8_t guard_after_seed[8] = {
        0x99, 0xab, 0xbd, 0xcf, 0xe1, 0xf3, 0x15, 0x27,
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
    const uintptr_t upper_tail_word_load_vaddr =
        aligned_upper_store_vaddr + 4U;
    const uintptr_t first_upper_byte_load_vaddr =
        aligned_upper_store_vaddr;
    const uintptr_t deny_start = boundary_phys + 8U;
    const uintptr_t deny_end = deny_start + PAGE_SIZE;
    const uint64_t first_fault_store_value = 0x8798a9bacbdcedfeULL;
    const uint64_t refault_store_value = 0x1f20314253647586ULL;
    const uint64_t aligned_upper_store_value_first = 0x98a9bacbdcedfe0fULL;
    const uint64_t aligned_upper_store_value_second = 0x2031425364758697ULL;
    const uint64_t aligned_upper_store_value_third = 0xa9bacbdcedfe0f10ULL;
    const uint64_t aligned_upper_store_value_fourth = 0x31425364758697a8ULL;
    const uint64_t retry_4b4b_store_value = 0xbacbdcedfe0f1021ULL;
    const uint8_t followup_pure_producer_tail_byte_store_value = 0xa0U;
    uint8_t expected_boundary[16];
    uint64_t expected_upper_tail_word_load_value = 0ULL;
    uint64_t observed_upper_tail_word_load_value = 0ULL;
    uint64_t expected_first_upper_byte_load_value = 0ULL;
    uint64_t observed_first_upper_byte_load_value = 0ULL;

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
    TEST_ASSERT("for the no-H same-page cross-16B translated scalar store, the first repeated 4B+4B second-split write fault must still report store access fault on the true second-half start address before the retried-success then pure-producer-tail-byte store plus the upper-tail-word and first-upper-byte follow-up probes",
        excpt.triggered == true &&
        excpt.cause == CAUSE_SAF &&
        excpt.tval == second_half_vaddr
    );

    TEST_SETUP_EXCEPT();
    ai_micro_mab_retry_byte4_exact_store_translated_sd(
        fault_vaddr, first_fault_store_value);
    TEST_ASSERT("repeating the same no-H second-split 4B+4B write fault keeps the same precise metadata before the retried-success then pure-producer-tail-byte store plus the upper-tail-word and first-upper-byte follow-up probes",
        excpt.triggered == true &&
        excpt.cause == CAUSE_SAF &&
        excpt.tval == second_half_vaddr
    );

    ai_micro_mab_retry_byte4_exact_store_restore_pmp_full_access();

    TEST_SETUP_EXCEPT();
    ai_micro_mab_retry_byte4_exact_store_translated_sd(
        aligned_upper_store_vaddr, aligned_upper_store_value_first);
    TEST_ASSERT("after repair, the first same-boundary translated upper-half aligned store should complete cleanly before the refaulted retry-4B+4B-store then pure-producer-tail-byte store plus the upper-tail-word and first-upper-byte follow-up probes",
        excpt.triggered == false
    );

    TEST_SETUP_EXCEPT();
    ai_micro_mab_retry_byte4_exact_store_translated_sd(
        aligned_upper_store_vaddr, aligned_upper_store_value_second);
    TEST_ASSERT("after one upper-half aligned translated store has already completed on the repaired boundary, the second upper-half aligned translated store should stay trap-free before the refaulted retry-4B+4B-store then pure-producer-tail-byte store plus the upper-tail-word and first-upper-byte follow-up probes",
        excpt.triggered == false
    );

    TEST_SETUP_EXCEPT();
    ai_micro_mab_retry_byte4_exact_store_translated_sd(
        aligned_upper_store_vaddr, aligned_upper_store_value_third);
    TEST_ASSERT("after two upper-half aligned translated stores have already completed on the repaired boundary, the third upper-half aligned translated store should stay trap-free before the refaulted retry-4B+4B-store then pure-producer-tail-byte store plus the upper-tail-word and first-upper-byte follow-up probes",
        excpt.triggered == false
    );

    TEST_SETUP_EXCEPT();
    ai_micro_mab_retry_byte4_exact_store_translated_sd(
        aligned_upper_store_vaddr, aligned_upper_store_value_fourth);
    TEST_ASSERT("after three upper-half aligned translated stores have already completed on the repaired boundary, the fourth upper-half aligned translated store should stay trap-free and become the latest committed high-half payload before the refaulted retry-4B+4B-store then pure-producer-tail-byte store plus the upper-tail-word and first-upper-byte follow-up probes",
        excpt.triggered == false
    );

    ai_micro_mab_retry_byte4_exact_store_configure_pmp_window(
        deny_start, deny_end,
        AI_MICRO_MAB_RETRY_BYTE4_EXACT_STORE_PMP_TOR_NO_W);

    TEST_SETUP_EXCEPT();
    ai_micro_mab_retry_byte4_exact_store_translated_sd(
        fault_vaddr, refault_store_value);
    TEST_ASSERT("after four upper-half aligned translated stores have already completed on the repaired boundary, restoring the upper-half deny window and reissuing the original 4B+4B misaligned store should refresh the same precise second-split store access fault metadata before the pure-producer-tail-byte store plus the upper-tail-word and first-upper-byte follow-up probes",
        excpt.triggered == true &&
        excpt.cause == CAUSE_SAF &&
        excpt.tval == second_half_vaddr
    );

    ai_micro_mab_retry_byte4_exact_store_restore_pmp_full_access();

    TEST_SETUP_EXCEPT();
    ai_micro_mab_retry_byte4_exact_store_translated_sd(
        fault_vaddr, retry_4b4b_store_value);
    TEST_ASSERT("after the refault episode is repaired again, retrying the same 4B+4B translated store as a success store should clear stale exception metadata before the immediate pure-producer-tail-byte store and the upper-tail-word plus first-upper-byte follow-up probes",
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
    expected_upper_tail_word_load_value =
        ai_micro_mab_retry_byte4_exact_store_read_little_endian(
            expected_boundary + 12U, sizeof(uint32_t));
    expected_first_upper_byte_load_value =
        ai_micro_mab_retry_byte4_exact_store_read_little_endian(
            expected_boundary + 8U, sizeof(uint8_t));

    TEST_SETUP_EXCEPT();
    observed_upper_tail_word_load_value =
        ai_micro_mab_retry_byte4_exact_store_translated_lwu(
            upper_tail_word_load_vaddr);
    TEST_ASSERT("after the pure-producer tail-byte store has already refreshed byte15 on top of the retried 4B+4B image, the immediate translated upper-tail-word load at boundary+12 should clear stale exception metadata and return retained bytes12-14 plus the new tail byte before the following first-upper-byte probe",
        excpt.triggered == false &&
        observed_upper_tail_word_load_value ==
            expected_upper_tail_word_load_value
    );

    TEST_SETUP_EXCEPT();
    observed_first_upper_byte_load_value =
        ai_micro_mab_retry_byte4_exact_store_translated_lbu(
            first_upper_byte_load_vaddr);
    TEST_ASSERT("after the upper-tail-word probe has already shown the merged latest tail image on top of the retried 4B+4B store, the following translated first-upper-byte load at boundary+8 should still clear stale exception metadata and keep the retried byte8 image intact instead of falling back to stale exact-BYTE4 state",
        excpt.triggered == false &&
        observed_first_upper_byte_load_value ==
            expected_first_upper_byte_load_value
    );

    TEST_ASSERT("after the prolonged upper-half producer, repaired refault, retried 4B+4B success store, immediate pure-producer tail-byte store, the upper-tail-word probe, and the first-upper-byte probe, the final boundary bytes should preserve bytes4-11 from the retried image, keep bytes12-14 from the latest aligned upper-half image, and refresh only byte15",
        ai_micro_mab_retry_byte4_exact_store_bytes_match(
            boundary_phys, expected_boundary, 16U)
    );

    TEST_ASSERT("guard regions around the same-page boundary should remain unchanged through the repeated 4B+4B fault, four upper-half aligned stores, refault, retried success store, immediate pure-producer-tail-byte follow-up store, the upper-tail-word probe, and the first-upper-byte probe sequence",
        ai_micro_mab_retry_byte4_exact_store_bytes_match(
            guard_before_phys, guard_before_seed, 8U) &&
        ai_micro_mab_retry_byte4_exact_store_bytes_match(
            guard_after_phys, guard_after_seed, 8U)
    );

    ai_micro_mab_retry_byte4_exact_store_reset_env();

    TEST_END(
        "ai_micro_m_samepage_cross16b_store_refault_upperx4_retry_4b4b_store_then_pure_producer_tail_byte_store_then_upper_tail_word_load_then_first_upper_byte_load_corner");
}

bool ai_micro_m_samepage_cross16b_store_refault_upperx4_retry_4b4b_store_then_pure_producer_tail_byte_store_then_upper_tail_word_load_then_second_upper_byte_load_corner()
{
    TEST_START();

    static const uint8_t boundary_seed[16] = {
        0x59, 0x48, 0x37, 0x26, 0x15, 0x04, 0xf3, 0xe2,
        0xd1, 0xc0, 0xaf, 0x9e, 0x8d, 0x7c, 0x6b, 0x5a,
    };
    static const uint8_t guard_before_seed[8] = {
        0x1a, 0x2c, 0x3e, 0x40, 0x52, 0x64, 0x76, 0x88,
    };
    static const uint8_t guard_after_seed[8] = {
        0x99, 0xab, 0xbd, 0xcf, 0xe1, 0xf3, 0x15, 0x27,
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
    const uintptr_t upper_tail_word_load_vaddr =
        aligned_upper_store_vaddr + 4U;
    const uintptr_t second_upper_byte_load_vaddr =
        aligned_upper_store_vaddr + 1U;
    const uintptr_t deny_start = boundary_phys + 8U;
    const uintptr_t deny_end = deny_start + PAGE_SIZE;
    const uint64_t first_fault_store_value = 0x8798a9bacbdcedfeULL;
    const uint64_t refault_store_value = 0x1f20314253647586ULL;
    const uint64_t aligned_upper_store_value_first = 0x98a9bacbdcedfe0fULL;
    const uint64_t aligned_upper_store_value_second = 0x2031425364758697ULL;
    const uint64_t aligned_upper_store_value_third = 0xa9bacbdcedfe0f10ULL;
    const uint64_t aligned_upper_store_value_fourth = 0x31425364758697a8ULL;
    const uint64_t retry_4b4b_store_value = 0xbacbdcedfe0f1021ULL;
    const uint8_t followup_pure_producer_tail_byte_store_value = 0xa0U;
    uint8_t expected_boundary[16];
    uint64_t expected_upper_tail_word_load_value = 0ULL;
    uint64_t observed_upper_tail_word_load_value = 0ULL;
    uint64_t expected_second_upper_byte_load_value = 0ULL;
    uint64_t observed_second_upper_byte_load_value = 0ULL;

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
    TEST_ASSERT("for the no-H same-page cross-16B translated scalar store, the first repeated 4B+4B second-split write fault must still report store access fault on the true second-half start address before the retried-success then pure-producer-tail-byte store plus the upper-tail-word and second-upper-byte follow-up probes",
        excpt.triggered == true &&
        excpt.cause == CAUSE_SAF &&
        excpt.tval == second_half_vaddr
    );

    TEST_SETUP_EXCEPT();
    ai_micro_mab_retry_byte4_exact_store_translated_sd(
        fault_vaddr, first_fault_store_value);
    TEST_ASSERT("repeating the same no-H second-split 4B+4B write fault keeps the same precise metadata before the retried-success then pure-producer-tail-byte store plus the upper-tail-word and second-upper-byte follow-up probes",
        excpt.triggered == true &&
        excpt.cause == CAUSE_SAF &&
        excpt.tval == second_half_vaddr
    );

    ai_micro_mab_retry_byte4_exact_store_restore_pmp_full_access();

    TEST_SETUP_EXCEPT();
    ai_micro_mab_retry_byte4_exact_store_translated_sd(
        aligned_upper_store_vaddr, aligned_upper_store_value_first);
    TEST_ASSERT("after repair, the first same-boundary translated upper-half aligned store should complete cleanly before the refaulted retry-4B+4B-store then pure-producer-tail-byte store plus the upper-tail-word and second-upper-byte follow-up probes",
        excpt.triggered == false
    );

    TEST_SETUP_EXCEPT();
    ai_micro_mab_retry_byte4_exact_store_translated_sd(
        aligned_upper_store_vaddr, aligned_upper_store_value_second);
    TEST_ASSERT("after one upper-half aligned translated store has already completed on the repaired boundary, the second upper-half aligned translated store should stay trap-free before the refaulted retry-4B+4B-store then pure-producer-tail-byte store plus the upper-tail-word and second-upper-byte follow-up probes",
        excpt.triggered == false
    );

    TEST_SETUP_EXCEPT();
    ai_micro_mab_retry_byte4_exact_store_translated_sd(
        aligned_upper_store_vaddr, aligned_upper_store_value_third);
    TEST_ASSERT("after two upper-half aligned translated stores have already completed on the repaired boundary, the third upper-half aligned translated store should stay trap-free before the refaulted retry-4B+4B-store then pure-producer-tail-byte store plus the upper-tail-word and second-upper-byte follow-up probes",
        excpt.triggered == false
    );

    TEST_SETUP_EXCEPT();
    ai_micro_mab_retry_byte4_exact_store_translated_sd(
        aligned_upper_store_vaddr, aligned_upper_store_value_fourth);
    TEST_ASSERT("after three upper-half aligned translated stores have already completed on the repaired boundary, the fourth upper-half aligned translated store should stay trap-free and become the latest committed high-half payload before the refaulted retry-4B+4B-store then pure-producer-tail-byte store plus the upper-tail-word and second-upper-byte follow-up probes",
        excpt.triggered == false
    );

    ai_micro_mab_retry_byte4_exact_store_configure_pmp_window(
        deny_start, deny_end,
        AI_MICRO_MAB_RETRY_BYTE4_EXACT_STORE_PMP_TOR_NO_W);

    TEST_SETUP_EXCEPT();
    ai_micro_mab_retry_byte4_exact_store_translated_sd(
        fault_vaddr, refault_store_value);
    TEST_ASSERT("after four upper-half aligned translated stores have already completed on the repaired boundary, restoring the upper-half deny window and reissuing the original 4B+4B misaligned store should refresh the same precise second-split store access fault metadata before the pure-producer-tail-byte store plus the upper-tail-word and second-upper-byte follow-up probes",
        excpt.triggered == true &&
        excpt.cause == CAUSE_SAF &&
        excpt.tval == second_half_vaddr
    );

    ai_micro_mab_retry_byte4_exact_store_restore_pmp_full_access();

    TEST_SETUP_EXCEPT();
    ai_micro_mab_retry_byte4_exact_store_translated_sd(
        fault_vaddr, retry_4b4b_store_value);
    TEST_ASSERT("after the refault episode is repaired again, retrying the same 4B+4B translated store as a success store should clear stale exception metadata before the immediate pure-producer-tail-byte store and the upper-tail-word plus second-upper-byte follow-up probes",
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
    expected_upper_tail_word_load_value =
        ai_micro_mab_retry_byte4_exact_store_read_little_endian(
            expected_boundary + 12U, sizeof(uint32_t));
    expected_second_upper_byte_load_value =
        ai_micro_mab_retry_byte4_exact_store_read_little_endian(
            expected_boundary + 9U, sizeof(uint8_t));

    TEST_SETUP_EXCEPT();
    observed_upper_tail_word_load_value =
        ai_micro_mab_retry_byte4_exact_store_translated_lwu(
            upper_tail_word_load_vaddr);
    TEST_ASSERT("after the pure-producer tail-byte store has already refreshed byte15 on top of the retried 4B+4B image, the immediate translated upper-tail-word load at boundary+12 should clear stale exception metadata and return retained bytes12-14 plus the new tail byte before the following second-upper-byte probe",
        excpt.triggered == false &&
        observed_upper_tail_word_load_value ==
            expected_upper_tail_word_load_value
    );

    TEST_SETUP_EXCEPT();
    observed_second_upper_byte_load_value =
        ai_micro_mab_retry_byte4_exact_store_translated_lbu(
            second_upper_byte_load_vaddr);
    TEST_ASSERT("after the upper-tail-word probe has already shown the merged latest tail image on top of the retried 4B+4B store, the following translated second-upper-byte load at boundary+9 should still clear stale exception metadata and keep the retried byte9 image intact instead of falling back to stale exact-BYTE4 state",
        excpt.triggered == false &&
        observed_second_upper_byte_load_value ==
            expected_second_upper_byte_load_value
    );

    TEST_ASSERT("after the prolonged upper-half producer, repaired refault, retried 4B+4B success store, immediate pure-producer tail-byte store, the upper-tail-word probe, and the second-upper-byte probe, the final boundary bytes should preserve bytes4-11 from the retried image, keep bytes12-14 from the latest aligned upper-half image, and refresh only byte15",
        ai_micro_mab_retry_byte4_exact_store_bytes_match(
            boundary_phys, expected_boundary, 16U)
    );

    TEST_ASSERT("guard regions around the same-page boundary should remain unchanged through the repeated 4B+4B fault, four upper-half aligned stores, refault, retried success store, immediate pure-producer-tail-byte follow-up store, the upper-tail-word probe, and the second-upper-byte probe sequence",
        ai_micro_mab_retry_byte4_exact_store_bytes_match(
            guard_before_phys, guard_before_seed, 8U) &&
        ai_micro_mab_retry_byte4_exact_store_bytes_match(
            guard_after_phys, guard_after_seed, 8U)
    );

    ai_micro_mab_retry_byte4_exact_store_reset_env();

    TEST_END(
        "ai_micro_m_samepage_cross16b_store_refault_upperx4_retry_4b4b_store_then_pure_producer_tail_byte_store_then_upper_tail_word_load_then_second_upper_byte_load_corner");
}

bool ai_micro_m_samepage_cross16b_store_refault_upperx4_retry_4b4b_store_then_pure_producer_tail_byte_store_then_upper_tail_word_load_then_head_middle_bridge_halfword_load_corner()
{
    TEST_START();

    static const uint8_t boundary_seed[16] = {
        0x59, 0x48, 0x37, 0x26, 0x15, 0x04, 0xf3, 0xe2,
        0xd1, 0xc0, 0xaf, 0x9e, 0x8d, 0x7c, 0x6b, 0x5a,
    };
    static const uint8_t guard_before_seed[8] = {
        0x1a, 0x2c, 0x3e, 0x40, 0x52, 0x64, 0x76, 0x88,
    };
    static const uint8_t guard_after_seed[8] = {
        0x99, 0xab, 0xbd, 0xcf, 0xe1, 0xf3, 0x15, 0x27,
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
    const uintptr_t upper_tail_word_load_vaddr =
        aligned_upper_store_vaddr + 4U;
    const uintptr_t head_middle_bridge_halfword_load_vaddr =
        aligned_upper_store_vaddr + 1U;
    const uintptr_t deny_start = boundary_phys + 8U;
    const uintptr_t deny_end = deny_start + PAGE_SIZE;
    const uint64_t first_fault_store_value = 0x8798a9bacbdcedfeULL;
    const uint64_t refault_store_value = 0x1f20314253647586ULL;
    const uint64_t aligned_upper_store_value_first = 0x98a9bacbdcedfe0fULL;
    const uint64_t aligned_upper_store_value_second = 0x2031425364758697ULL;
    const uint64_t aligned_upper_store_value_third = 0xa9bacbdcedfe0f10ULL;
    const uint64_t aligned_upper_store_value_fourth = 0x31425364758697a8ULL;
    const uint64_t retry_4b4b_store_value = 0xbacbdcedfe0f1021ULL;
    const uint8_t followup_pure_producer_tail_byte_store_value = 0xa0U;
    uint8_t expected_boundary[16];
    uint64_t expected_upper_tail_word_load_value = 0ULL;
    uint64_t observed_upper_tail_word_load_value = 0ULL;
    uint64_t expected_head_middle_bridge_halfword_load_value = 0ULL;
    uint64_t observed_head_middle_bridge_halfword_load_value = 0ULL;

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
    TEST_ASSERT("for the no-H same-page cross-16B translated scalar store, the first repeated 4B+4B second-split write fault must still report store access fault on the true second-half start address before the retried-success then pure-producer-tail-byte store plus the upper-tail-word and head-middle-bridge-halfword follow-up probes",
        excpt.triggered == true &&
        excpt.cause == CAUSE_SAF &&
        excpt.tval == second_half_vaddr
    );

    TEST_SETUP_EXCEPT();
    ai_micro_mab_retry_byte4_exact_store_translated_sd(
        fault_vaddr, first_fault_store_value);
    TEST_ASSERT("repeating the same no-H second-split 4B+4B write fault keeps the same precise metadata before the retried-success then pure-producer-tail-byte store plus the upper-tail-word and head-middle-bridge-halfword follow-up probes",
        excpt.triggered == true &&
        excpt.cause == CAUSE_SAF &&
        excpt.tval == second_half_vaddr
    );

    ai_micro_mab_retry_byte4_exact_store_restore_pmp_full_access();

    TEST_SETUP_EXCEPT();
    ai_micro_mab_retry_byte4_exact_store_translated_sd(
        aligned_upper_store_vaddr, aligned_upper_store_value_first);
    TEST_ASSERT("after repair, the first same-boundary translated upper-half aligned store should complete cleanly before the refaulted retry-4B+4B-store then pure-producer-tail-byte store plus the upper-tail-word and head-middle-bridge-halfword follow-up probes",
        excpt.triggered == false
    );

    TEST_SETUP_EXCEPT();
    ai_micro_mab_retry_byte4_exact_store_translated_sd(
        aligned_upper_store_vaddr, aligned_upper_store_value_second);
    TEST_ASSERT("after one upper-half aligned translated store has already completed on the repaired boundary, the second upper-half aligned translated store should stay trap-free before the refaulted retry-4B+4B-store then pure-producer-tail-byte store plus the upper-tail-word and head-middle-bridge-halfword follow-up probes",
        excpt.triggered == false
    );

    TEST_SETUP_EXCEPT();
    ai_micro_mab_retry_byte4_exact_store_translated_sd(
        aligned_upper_store_vaddr, aligned_upper_store_value_third);
    TEST_ASSERT("after two upper-half aligned translated stores have already completed on the repaired boundary, the third upper-half aligned translated store should stay trap-free before the refaulted retry-4B+4B-store then pure-producer-tail-byte store plus the upper-tail-word and head-middle-bridge-halfword follow-up probes",
        excpt.triggered == false
    );

    TEST_SETUP_EXCEPT();
    ai_micro_mab_retry_byte4_exact_store_translated_sd(
        aligned_upper_store_vaddr, aligned_upper_store_value_fourth);
    TEST_ASSERT("after three upper-half aligned translated stores have already completed on the repaired boundary, the fourth upper-half aligned translated store should stay trap-free and become the latest committed high-half payload before the refaulted retry-4B+4B-store then pure-producer-tail-byte store plus the upper-tail-word and head-middle-bridge-halfword follow-up probes",
        excpt.triggered == false
    );

    ai_micro_mab_retry_byte4_exact_store_configure_pmp_window(
        deny_start, deny_end,
        AI_MICRO_MAB_RETRY_BYTE4_EXACT_STORE_PMP_TOR_NO_W);

    TEST_SETUP_EXCEPT();
    ai_micro_mab_retry_byte4_exact_store_translated_sd(
        fault_vaddr, refault_store_value);
    TEST_ASSERT("after four upper-half aligned translated stores have already completed on the repaired boundary, restoring the upper-half deny window and reissuing the original 4B+4B misaligned store should refresh the same precise second-split store access fault metadata before the pure-producer-tail-byte store plus the upper-tail-word and head-middle-bridge-halfword follow-up probes",
        excpt.triggered == true &&
        excpt.cause == CAUSE_SAF &&
        excpt.tval == second_half_vaddr
    );

    ai_micro_mab_retry_byte4_exact_store_restore_pmp_full_access();

    TEST_SETUP_EXCEPT();
    ai_micro_mab_retry_byte4_exact_store_translated_sd(
        fault_vaddr, retry_4b4b_store_value);
    TEST_ASSERT("after the refault episode is repaired again, retrying the same 4B+4B translated store as a success store should clear stale exception metadata before the immediate pure-producer-tail-byte store and the upper-tail-word plus head-middle-bridge-halfword follow-up probes",
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
    expected_upper_tail_word_load_value =
        ai_micro_mab_retry_byte4_exact_store_read_little_endian(
            expected_boundary + 12U, sizeof(uint32_t));
    expected_head_middle_bridge_halfword_load_value =
        ai_micro_mab_retry_byte4_exact_store_read_little_endian(
            expected_boundary + 9U, sizeof(uint16_t));

    TEST_SETUP_EXCEPT();
    observed_upper_tail_word_load_value =
        ai_micro_mab_retry_byte4_exact_store_translated_lwu(
            upper_tail_word_load_vaddr);
    TEST_ASSERT("after the pure-producer tail-byte store has already refreshed byte15 on top of the retried 4B+4B image, the immediate translated upper-tail-word load at boundary+12 should clear stale exception metadata and return retained bytes12-14 plus the new tail byte before the following head-middle-bridge-halfword overlap probe",
        excpt.triggered == false &&
        observed_upper_tail_word_load_value ==
            expected_upper_tail_word_load_value
    );

    TEST_SETUP_EXCEPT();
    observed_head_middle_bridge_halfword_load_value =
        ai_micro_mab_retry_byte4_exact_store_translated_lhu(
            head_middle_bridge_halfword_load_vaddr);
    TEST_ASSERT("after the upper-tail-word probe has already shown the merged latest tail image on top of the retried 4B+4B store, the following translated head-middle bridge halfword load at boundary+9 should still clear stale exception metadata and keep retried bytes9-10 intact instead of falling back to stale exact-BYTE4 state",
        excpt.triggered == false &&
        observed_head_middle_bridge_halfword_load_value ==
            expected_head_middle_bridge_halfword_load_value
    );

    TEST_ASSERT("after the prolonged upper-half producer, repaired refault, retried 4B+4B success store, immediate pure-producer tail-byte store, the upper-tail-word probe, and the head-middle-bridge-halfword probe, the final boundary bytes should preserve bytes4-11 from the retried image, keep bytes12-14 from the latest aligned upper-half image, and refresh only byte15",
        ai_micro_mab_retry_byte4_exact_store_bytes_match(
            boundary_phys, expected_boundary, 16U)
    );

    TEST_ASSERT("guard regions around the same-page boundary should remain unchanged through the repeated 4B+4B fault, four upper-half aligned stores, refault, retried success store, immediate pure-producer-tail-byte follow-up store, the upper-tail-word probe, and the head-middle-bridge-halfword probe sequence",
        ai_micro_mab_retry_byte4_exact_store_bytes_match(
            guard_before_phys, guard_before_seed, 8U) &&
        ai_micro_mab_retry_byte4_exact_store_bytes_match(
            guard_after_phys, guard_after_seed, 8U)
    );

    ai_micro_mab_retry_byte4_exact_store_reset_env();

    TEST_END(
        "ai_micro_m_samepage_cross16b_store_refault_upperx4_retry_4b4b_store_then_pure_producer_tail_byte_store_then_upper_tail_word_load_then_head_middle_bridge_halfword_load_corner");
}

bool ai_micro_m_samepage_cross16b_store_refault_upperx4_retry_4b4b_store_then_pure_producer_tail_byte_store_then_upper_tail_word_load_then_middle_tail_bridge_halfword_load_corner()
{
    TEST_START();

    static const uint8_t boundary_seed[16] = {
        0x59, 0x48, 0x37, 0x26, 0x15, 0x04, 0xf3, 0xe2,
        0xd1, 0xc0, 0xaf, 0x9e, 0x8d, 0x7c, 0x6b, 0x5a,
    };
    static const uint8_t guard_before_seed[8] = {
        0x1a, 0x2c, 0x3e, 0x40, 0x52, 0x64, 0x76, 0x88,
    };
    static const uint8_t guard_after_seed[8] = {
        0x99, 0xab, 0xbd, 0xcf, 0xe1, 0xf3, 0x15, 0x27,
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
    const uintptr_t upper_tail_word_load_vaddr =
        aligned_upper_store_vaddr + 4U;
    const uintptr_t middle_tail_bridge_halfword_load_vaddr =
        aligned_upper_store_vaddr + 2U;
    const uintptr_t deny_start = boundary_phys + 8U;
    const uintptr_t deny_end = deny_start + PAGE_SIZE;
    const uint64_t first_fault_store_value = 0x8798a9bacbdcedfeULL;
    const uint64_t refault_store_value = 0x1f20314253647586ULL;
    const uint64_t aligned_upper_store_value_first = 0x98a9bacbdcedfe0fULL;
    const uint64_t aligned_upper_store_value_second = 0x2031425364758697ULL;
    const uint64_t aligned_upper_store_value_third = 0xa9bacbdcedfe0f10ULL;
    const uint64_t aligned_upper_store_value_fourth = 0x31425364758697a8ULL;
    const uint64_t retry_4b4b_store_value = 0xbacbdcedfe0f1021ULL;
    const uint8_t followup_pure_producer_tail_byte_store_value = 0xa0U;
    uint8_t expected_boundary[16];
    uint64_t expected_upper_tail_word_load_value = 0ULL;
    uint64_t observed_upper_tail_word_load_value = 0ULL;
    uint64_t expected_middle_tail_bridge_halfword_load_value = 0ULL;
    uint64_t observed_middle_tail_bridge_halfword_load_value = 0ULL;

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
    TEST_ASSERT("for the no-H same-page cross-16B translated scalar store, the first repeated 4B+4B second-split write fault must still report store access fault on the true second-half start address before the retried-success then pure-producer-tail-byte store plus the upper-tail-word and middle-tail-bridge-halfword follow-up probes",
        excpt.triggered == true &&
        excpt.cause == CAUSE_SAF &&
        excpt.tval == second_half_vaddr
    );

    TEST_SETUP_EXCEPT();
    ai_micro_mab_retry_byte4_exact_store_translated_sd(
        fault_vaddr, first_fault_store_value);
    TEST_ASSERT("repeating the same no-H second-split 4B+4B write fault keeps the same precise metadata before the retried-success then pure-producer-tail-byte store plus the upper-tail-word and middle-tail-bridge-halfword follow-up probes",
        excpt.triggered == true &&
        excpt.cause == CAUSE_SAF &&
        excpt.tval == second_half_vaddr
    );

    ai_micro_mab_retry_byte4_exact_store_restore_pmp_full_access();

    TEST_SETUP_EXCEPT();
    ai_micro_mab_retry_byte4_exact_store_translated_sd(
        aligned_upper_store_vaddr, aligned_upper_store_value_first);
    TEST_ASSERT("after repair, the first same-boundary translated upper-half aligned store should complete cleanly before the refaulted retry-4B+4B-store then pure-producer-tail-byte store plus the upper-tail-word and middle-tail-bridge-halfword follow-up probes",
        excpt.triggered == false
    );

    TEST_SETUP_EXCEPT();
    ai_micro_mab_retry_byte4_exact_store_translated_sd(
        aligned_upper_store_vaddr, aligned_upper_store_value_second);
    TEST_ASSERT("after one upper-half aligned translated store has already completed on the repaired boundary, the second upper-half aligned translated store should stay trap-free before the refaulted retry-4B+4B-store then pure-producer-tail-byte store plus the upper-tail-word and middle-tail-bridge-halfword follow-up probes",
        excpt.triggered == false
    );

    TEST_SETUP_EXCEPT();
    ai_micro_mab_retry_byte4_exact_store_translated_sd(
        aligned_upper_store_vaddr, aligned_upper_store_value_third);
    TEST_ASSERT("after two upper-half aligned translated stores have already completed on the repaired boundary, the third upper-half aligned translated store should stay trap-free before the refaulted retry-4B+4B-store then pure-producer-tail-byte store plus the upper-tail-word and middle-tail-bridge-halfword follow-up probes",
        excpt.triggered == false
    );

    TEST_SETUP_EXCEPT();
    ai_micro_mab_retry_byte4_exact_store_translated_sd(
        aligned_upper_store_vaddr, aligned_upper_store_value_fourth);
    TEST_ASSERT("after three upper-half aligned translated stores have already completed on the repaired boundary, the fourth upper-half aligned translated store should stay trap-free and become the latest committed high-half payload before the refaulted retry-4B+4B-store then pure-producer-tail-byte store plus the upper-tail-word and middle-tail-bridge-halfword follow-up probes",
        excpt.triggered == false
    );

    ai_micro_mab_retry_byte4_exact_store_configure_pmp_window(
        deny_start, deny_end,
        AI_MICRO_MAB_RETRY_BYTE4_EXACT_STORE_PMP_TOR_NO_W);

    TEST_SETUP_EXCEPT();
    ai_micro_mab_retry_byte4_exact_store_translated_sd(
        fault_vaddr, refault_store_value);
    TEST_ASSERT("after four upper-half aligned translated stores have already completed on the repaired boundary, restoring the upper-half deny window and reissuing the original 4B+4B misaligned store should refresh the same precise second-split store access fault metadata before the pure-producer-tail-byte store plus the upper-tail-word and middle-tail-bridge-halfword follow-up probes",
        excpt.triggered == true &&
        excpt.cause == CAUSE_SAF &&
        excpt.tval == second_half_vaddr
    );

    ai_micro_mab_retry_byte4_exact_store_restore_pmp_full_access();

    TEST_SETUP_EXCEPT();
    ai_micro_mab_retry_byte4_exact_store_translated_sd(
        fault_vaddr, retry_4b4b_store_value);
    TEST_ASSERT("after the refault episode is repaired again, retrying the same 4B+4B translated store as a success store should clear stale exception metadata before the immediate pure-producer-tail-byte store and the upper-tail-word plus middle-tail-bridge-halfword follow-up probes",
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
    expected_upper_tail_word_load_value =
        ai_micro_mab_retry_byte4_exact_store_read_little_endian(
            expected_boundary + 12U, sizeof(uint32_t));
    expected_middle_tail_bridge_halfword_load_value =
        ai_micro_mab_retry_byte4_exact_store_read_little_endian(
            expected_boundary + 10U, sizeof(uint16_t));

    TEST_SETUP_EXCEPT();
    observed_upper_tail_word_load_value =
        ai_micro_mab_retry_byte4_exact_store_translated_lwu(
            upper_tail_word_load_vaddr);
    TEST_ASSERT("after the pure-producer tail-byte store has already refreshed byte15 on top of the retried 4B+4B image, the immediate translated upper-tail-word load at boundary+12 should clear stale exception metadata and return retained bytes12-14 plus the new tail byte before the following middle-tail-bridge-halfword overlap probe",
        excpt.triggered == false &&
        observed_upper_tail_word_load_value ==
            expected_upper_tail_word_load_value
    );

    TEST_SETUP_EXCEPT();
    observed_middle_tail_bridge_halfword_load_value =
        ai_micro_mab_retry_byte4_exact_store_translated_lhu(
            middle_tail_bridge_halfword_load_vaddr);
    TEST_ASSERT("after the upper-tail-word probe has already shown the merged latest tail image on top of the retried 4B+4B store, the following translated middle-tail bridge halfword load at boundary+10 should still clear stale exception metadata and keep retried bytes10-11 intact instead of falling back to stale exact-BYTE4 state",
        excpt.triggered == false &&
        observed_middle_tail_bridge_halfword_load_value ==
            expected_middle_tail_bridge_halfword_load_value
    );

    TEST_ASSERT("after the prolonged upper-half producer, repaired refault, retried 4B+4B success store, immediate pure-producer tail-byte store, the upper-tail-word probe, and the middle-tail-bridge-halfword probe, the final boundary bytes should preserve bytes4-11 from the retried image, keep bytes12-14 from the latest aligned upper-half image, and refresh only byte15",
        ai_micro_mab_retry_byte4_exact_store_bytes_match(
            boundary_phys, expected_boundary, 16U)
    );

    TEST_ASSERT("guard regions around the same-page boundary should remain unchanged through the repeated 4B+4B fault, four upper-half aligned stores, refault, retried success store, immediate pure-producer-tail-byte follow-up store, the upper-tail-word probe, and the middle-tail-bridge-halfword probe sequence",
        ai_micro_mab_retry_byte4_exact_store_bytes_match(
            guard_before_phys, guard_before_seed, 8U) &&
        ai_micro_mab_retry_byte4_exact_store_bytes_match(
            guard_after_phys, guard_after_seed, 8U)
    );

    ai_micro_mab_retry_byte4_exact_store_reset_env();

    TEST_END(
        "ai_micro_m_samepage_cross16b_store_refault_upperx4_retry_4b4b_store_then_pure_producer_tail_byte_store_then_upper_tail_word_load_then_middle_tail_bridge_halfword_load_corner");
}

bool ai_micro_m_samepage_cross16b_store_refault_upperx4_retry_4b4b_store_then_pure_producer_tail_byte_store_then_upper_tail_word_load_then_middle_tail_first_byte_load_corner()
{
    TEST_START();

    static const uint8_t boundary_seed[16] = {
        0x59, 0x48, 0x37, 0x26, 0x15, 0x04, 0xf3, 0xe2,
        0xd1, 0xc0, 0xaf, 0x9e, 0x8d, 0x7c, 0x6b, 0x5a,
    };
    static const uint8_t guard_before_seed[8] = {
        0x1a, 0x2c, 0x3e, 0x40, 0x52, 0x64, 0x76, 0x88,
    };
    static const uint8_t guard_after_seed[8] = {
        0x99, 0xab, 0xbd, 0xcf, 0xe1, 0xf3, 0x15, 0x27,
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
    const uintptr_t upper_tail_word_load_vaddr =
        aligned_upper_store_vaddr + 4U;
    const uintptr_t middle_tail_first_byte_load_vaddr =
        aligned_upper_store_vaddr + 2U;
    const uintptr_t deny_start = boundary_phys + 8U;
    const uintptr_t deny_end = deny_start + PAGE_SIZE;
    const uint64_t first_fault_store_value = 0x8798a9bacbdcedfeULL;
    const uint64_t refault_store_value = 0x1f20314253647586ULL;
    const uint64_t aligned_upper_store_value_first = 0x98a9bacbdcedfe0fULL;
    const uint64_t aligned_upper_store_value_second = 0x2031425364758697ULL;
    const uint64_t aligned_upper_store_value_third = 0xa9bacbdcedfe0f10ULL;
    const uint64_t aligned_upper_store_value_fourth = 0x31425364758697a8ULL;
    const uint64_t retry_4b4b_store_value = 0xbacbdcedfe0f1021ULL;
    const uint8_t followup_pure_producer_tail_byte_store_value = 0xa0U;
    uint8_t expected_boundary[16];
    uint64_t expected_upper_tail_word_load_value = 0ULL;
    uint64_t observed_upper_tail_word_load_value = 0ULL;
    uint64_t expected_middle_tail_first_byte_load_value = 0ULL;
    uint64_t observed_middle_tail_first_byte_load_value = 0ULL;

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
    TEST_ASSERT("for the no-H same-page cross-16B translated scalar store, the first repeated 4B+4B second-split write fault must still report store access fault on the true second-half start address before the retried-success then pure-producer-tail-byte store plus the upper-tail-word and middle-tail-first-byte follow-up probes",
        excpt.triggered == true &&
        excpt.cause == CAUSE_SAF &&
        excpt.tval == second_half_vaddr
    );

    TEST_SETUP_EXCEPT();
    ai_micro_mab_retry_byte4_exact_store_translated_sd(
        fault_vaddr, first_fault_store_value);
    TEST_ASSERT("repeating the same no-H second-split 4B+4B write fault keeps the same precise metadata before the retried-success then pure-producer-tail-byte store plus the upper-tail-word and middle-tail-first-byte follow-up probes",
        excpt.triggered == true &&
        excpt.cause == CAUSE_SAF &&
        excpt.tval == second_half_vaddr
    );

    ai_micro_mab_retry_byte4_exact_store_restore_pmp_full_access();

    TEST_SETUP_EXCEPT();
    ai_micro_mab_retry_byte4_exact_store_translated_sd(
        aligned_upper_store_vaddr, aligned_upper_store_value_first);
    TEST_ASSERT("after repair, the first same-boundary translated upper-half aligned store should complete cleanly before the refaulted retry-4B+4B-store then pure-producer-tail-byte store plus the upper-tail-word and middle-tail-first-byte follow-up probes",
        excpt.triggered == false
    );

    TEST_SETUP_EXCEPT();
    ai_micro_mab_retry_byte4_exact_store_translated_sd(
        aligned_upper_store_vaddr, aligned_upper_store_value_second);
    TEST_ASSERT("after one upper-half aligned translated store has already completed on the repaired boundary, the second upper-half aligned translated store should stay trap-free before the refaulted retry-4B+4B-store then pure-producer-tail-byte store plus the upper-tail-word and middle-tail-first-byte follow-up probes",
        excpt.triggered == false
    );

    TEST_SETUP_EXCEPT();
    ai_micro_mab_retry_byte4_exact_store_translated_sd(
        aligned_upper_store_vaddr, aligned_upper_store_value_third);
    TEST_ASSERT("after two upper-half aligned translated stores have already completed on the repaired boundary, the third upper-half aligned translated store should stay trap-free before the refaulted retry-4B+4B-store then pure-producer-tail-byte store plus the upper-tail-word and middle-tail-first-byte follow-up probes",
        excpt.triggered == false
    );

    TEST_SETUP_EXCEPT();
    ai_micro_mab_retry_byte4_exact_store_translated_sd(
        aligned_upper_store_vaddr, aligned_upper_store_value_fourth);
    TEST_ASSERT("after three upper-half aligned translated stores have already completed on the repaired boundary, the fourth upper-half aligned translated store should stay trap-free and become the latest committed high-half payload before the refaulted retry-4B+4B-store then pure-producer-tail-byte store plus the upper-tail-word and middle-tail-first-byte follow-up probes",
        excpt.triggered == false
    );

    ai_micro_mab_retry_byte4_exact_store_configure_pmp_window(
        deny_start, deny_end,
        AI_MICRO_MAB_RETRY_BYTE4_EXACT_STORE_PMP_TOR_NO_W);

    TEST_SETUP_EXCEPT();
    ai_micro_mab_retry_byte4_exact_store_translated_sd(
        fault_vaddr, refault_store_value);
    TEST_ASSERT("after four upper-half aligned translated stores have already completed on the repaired boundary, restoring the upper-half deny window and reissuing the original 4B+4B misaligned store should refresh the same precise second-split store access fault metadata before the pure-producer-tail-byte store plus the upper-tail-word and middle-tail-first-byte follow-up probes",
        excpt.triggered == true &&
        excpt.cause == CAUSE_SAF &&
        excpt.tval == second_half_vaddr
    );

    ai_micro_mab_retry_byte4_exact_store_restore_pmp_full_access();

    TEST_SETUP_EXCEPT();
    ai_micro_mab_retry_byte4_exact_store_translated_sd(
        fault_vaddr, retry_4b4b_store_value);
    TEST_ASSERT("after the refault episode is repaired again, retrying the same 4B+4B translated store as a success store should clear stale exception metadata before the immediate pure-producer-tail-byte store and the upper-tail-word plus middle-tail-first-byte follow-up probes",
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
    expected_upper_tail_word_load_value =
        ai_micro_mab_retry_byte4_exact_store_read_little_endian(
            expected_boundary + 12U, sizeof(uint32_t));
    expected_middle_tail_first_byte_load_value =
        ai_micro_mab_retry_byte4_exact_store_read_little_endian(
            expected_boundary + 10U, sizeof(uint8_t));

    TEST_SETUP_EXCEPT();
    observed_upper_tail_word_load_value =
        ai_micro_mab_retry_byte4_exact_store_translated_lwu(
            upper_tail_word_load_vaddr);
    TEST_ASSERT("after the pure-producer tail-byte store has already refreshed byte15 on top of the retried 4B+4B image, the immediate translated upper-tail-word load at boundary+12 should clear stale exception metadata and return retained bytes12-14 plus the new tail byte before the following middle-tail-first-byte probe",
        excpt.triggered == false &&
        observed_upper_tail_word_load_value ==
            expected_upper_tail_word_load_value
    );

    TEST_SETUP_EXCEPT();
    observed_middle_tail_first_byte_load_value =
        ai_micro_mab_retry_byte4_exact_store_translated_lbu(
            middle_tail_first_byte_load_vaddr);
    TEST_ASSERT("after the upper-tail-word probe has already shown the merged latest tail image on top of the retried 4B+4B store, the following translated middle-tail first-byte load at boundary+10 should still clear stale exception metadata and keep the retried byte10 image intact instead of falling back to stale exact-BYTE4 state",
        excpt.triggered == false &&
        observed_middle_tail_first_byte_load_value ==
            expected_middle_tail_first_byte_load_value
    );

    TEST_ASSERT("after the prolonged upper-half producer, repaired refault, retried 4B+4B success store, immediate pure-producer tail-byte store, the upper-tail-word probe, and the middle-tail-first-byte probe, the final boundary bytes should preserve bytes4-11 from the retried image, keep bytes12-14 from the latest aligned upper-half image, and refresh only byte15",
        ai_micro_mab_retry_byte4_exact_store_bytes_match(
            boundary_phys, expected_boundary, 16U)
    );

    TEST_ASSERT("guard regions around the same-page boundary should remain unchanged through the repeated 4B+4B fault, four upper-half aligned stores, refault, retried success store, immediate pure-producer-tail-byte follow-up store, the upper-tail-word probe, and the middle-tail-first-byte probe sequence",
        ai_micro_mab_retry_byte4_exact_store_bytes_match(
            guard_before_phys, guard_before_seed, 8U) &&
        ai_micro_mab_retry_byte4_exact_store_bytes_match(
            guard_after_phys, guard_after_seed, 8U)
    );

    ai_micro_mab_retry_byte4_exact_store_reset_env();

    TEST_END(
        "ai_micro_m_samepage_cross16b_store_refault_upperx4_retry_4b4b_store_then_pure_producer_tail_byte_store_then_upper_tail_word_load_then_middle_tail_first_byte_load_corner");
}
