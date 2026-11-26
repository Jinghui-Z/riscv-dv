#include "ai_micro_mmode_memblock_retry_followup_byte4_exact_store_common.h"

bool ai_micro_m_samepage_cross16b_store_refault_upperx4_retry_4b4b_store_then_pure_producer_tail_byte_store_then_upper_tail_word_load_then_upper_half_load_corner()
{
    TEST_START();

    static const uint8_t boundary_seed[16] = {
        0x52, 0x41, 0x30, 0x1f, 0x0e, 0xfd, 0xec, 0xdb,
        0xca, 0xb9, 0xa8, 0x97, 0x86, 0x75, 0x64, 0x53,
    };
    static const uint8_t guard_before_seed[8] = {
        0x11, 0x23, 0x35, 0x47, 0x59, 0x6b, 0x7d, 0x8f,
    };
    static const uint8_t guard_after_seed[8] = {
        0x9e, 0xb0, 0xc2, 0xd4, 0xe6, 0xf8, 0x0a, 0x1c,
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
    const uintptr_t upper_half_load_vaddr = aligned_upper_store_vaddr;
    const uintptr_t deny_start = boundary_phys + 8U;
    const uintptr_t deny_end = deny_start + PAGE_SIZE;
    const uint64_t first_fault_store_value = 0x132435465768798aULL;
    const uint64_t refault_store_value = 0x8b9cadbecfe0f102ULL;
    const uint64_t aligned_upper_store_value_first = 0x2435465768798a9bULL;
    const uint64_t aligned_upper_store_value_second = 0x9cadbecfe0f10213ULL;
    const uint64_t aligned_upper_store_value_third = 0x35465768798a9bacULL;
    const uint64_t aligned_upper_store_value_fourth = 0xadbecfe0f1021324ULL;
    const uint64_t retry_4b4b_store_value = 0x465768798a9bacbdULL;
    const uint8_t followup_pure_producer_tail_byte_store_value = 0x5eU;
    uint8_t expected_boundary[16];
    uint64_t expected_upper_tail_word_load_value = 0ULL;
    uint64_t observed_upper_tail_word_load_value = 0ULL;
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
    TEST_ASSERT("for the no-H same-page cross-16B translated scalar store, the first repeated 4B+4B second-split write fault must still report store access fault on the true second-half start address before the retried-success then pure-producer-tail-byte store plus the upper-tail-word and upper-half follow-up probes",
        excpt.triggered == true &&
        excpt.cause == CAUSE_SAF &&
        excpt.tval == second_half_vaddr
    );

    TEST_SETUP_EXCEPT();
    ai_micro_mab_retry_byte4_exact_store_translated_sd(
        fault_vaddr, first_fault_store_value);
    TEST_ASSERT("repeating the same no-H second-split 4B+4B write fault keeps the same precise metadata before the retried-success then pure-producer-tail-byte store plus the upper-tail-word and upper-half follow-up probes",
        excpt.triggered == true &&
        excpt.cause == CAUSE_SAF &&
        excpt.tval == second_half_vaddr
    );

    ai_micro_mab_retry_byte4_exact_store_restore_pmp_full_access();

    TEST_SETUP_EXCEPT();
    ai_micro_mab_retry_byte4_exact_store_translated_sd(
        aligned_upper_store_vaddr, aligned_upper_store_value_first);
    TEST_ASSERT("after repair, the first same-boundary translated upper-half aligned store should complete cleanly before the refaulted retry-4B+4B-store then pure-producer-tail-byte store plus the upper-tail-word and upper-half follow-up probes",
        excpt.triggered == false
    );

    TEST_SETUP_EXCEPT();
    ai_micro_mab_retry_byte4_exact_store_translated_sd(
        aligned_upper_store_vaddr, aligned_upper_store_value_second);
    TEST_ASSERT("after one upper-half aligned translated store has already completed on the repaired boundary, the second upper-half aligned translated store should stay trap-free before the refaulted retry-4B+4B-store then pure-producer-tail-byte store plus the upper-tail-word and upper-half follow-up probes",
        excpt.triggered == false
    );

    TEST_SETUP_EXCEPT();
    ai_micro_mab_retry_byte4_exact_store_translated_sd(
        aligned_upper_store_vaddr, aligned_upper_store_value_third);
    TEST_ASSERT("after two upper-half aligned translated stores have already completed on the repaired boundary, the third upper-half aligned translated store should stay trap-free before the refaulted retry-4B+4B-store then pure-producer-tail-byte store plus the upper-tail-word and upper-half follow-up probes",
        excpt.triggered == false
    );

    TEST_SETUP_EXCEPT();
    ai_micro_mab_retry_byte4_exact_store_translated_sd(
        aligned_upper_store_vaddr, aligned_upper_store_value_fourth);
    TEST_ASSERT("after three upper-half aligned translated stores have already completed on the repaired boundary, the fourth upper-half aligned translated store should stay trap-free and become the latest committed high-half payload before the refaulted retry-4B+4B-store then pure-producer-tail-byte store plus the upper-tail-word and upper-half follow-up probes",
        excpt.triggered == false
    );

    ai_micro_mab_retry_byte4_exact_store_configure_pmp_window(
        deny_start, deny_end,
        AI_MICRO_MAB_RETRY_BYTE4_EXACT_STORE_PMP_TOR_NO_W);

    TEST_SETUP_EXCEPT();
    ai_micro_mab_retry_byte4_exact_store_translated_sd(
        fault_vaddr, refault_store_value);
    TEST_ASSERT("after four upper-half aligned translated stores have already completed on the repaired boundary, restoring the upper-half deny window and reissuing the original 4B+4B misaligned store should refresh the same precise second-split store access fault metadata before the pure-producer-tail-byte store plus the upper-tail-word and upper-half follow-up probes",
        excpt.triggered == true &&
        excpt.cause == CAUSE_SAF &&
        excpt.tval == second_half_vaddr
    );

    ai_micro_mab_retry_byte4_exact_store_restore_pmp_full_access();

    TEST_SETUP_EXCEPT();
    ai_micro_mab_retry_byte4_exact_store_translated_sd(
        fault_vaddr, retry_4b4b_store_value);
    TEST_ASSERT("after the refault episode is repaired again, retrying the same 4B+4B translated store as a success store should clear stale exception metadata before the immediate pure-producer-tail-byte store and the upper-tail-word plus upper-half follow-up probes",
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
    expected_upper_half_load_value =
        ai_micro_mab_retry_byte4_exact_store_read_little_endian(
            expected_boundary + 8U, sizeof(uint64_t));

    TEST_SETUP_EXCEPT();
    observed_upper_tail_word_load_value =
        ai_micro_mab_retry_byte4_exact_store_translated_lwu(
            upper_tail_word_load_vaddr);
    TEST_ASSERT("after the pure-producer tail-byte store has already refreshed byte15 on top of the retried 4B+4B image, the immediate translated upper-tail-word load at boundary+12 should clear stale exception metadata and return retained bytes12-14 plus the new tail byte before the following upper-half widening probe",
        excpt.triggered == false &&
        observed_upper_tail_word_load_value ==
            expected_upper_tail_word_load_value
    );

    TEST_SETUP_EXCEPT();
    observed_upper_half_load_value =
        ai_micro_mab_retry_byte4_exact_store_translated_ld(
            upper_half_load_vaddr);
    TEST_ASSERT("after the upper-tail-word probe has already consumed the latest 4-byte tail window on top of the retried 4B+4B store, the following aligned upper-half translated load at boundary+8 should still clear stale exception metadata and widen back to the merged latest image of retry bytes8-11, retained producer bytes12-14, and the new tail byte instead of falling back to stale exact-BYTE4 state",
        excpt.triggered == false &&
        observed_upper_half_load_value == expected_upper_half_load_value
    );

    TEST_ASSERT("after the prolonged upper-half producer, repaired refault, retried 4B+4B success store, immediate pure-producer tail-byte store, the upper-tail-word probe, and the upper-half widening probe, the final boundary bytes should preserve bytes4-11 from the retried image, keep bytes12-14 from the latest aligned upper-half image, and refresh only byte15",
        ai_micro_mab_retry_byte4_exact_store_bytes_match(
            boundary_phys, expected_boundary, 16U)
    );

    TEST_ASSERT("guard regions around the same-page boundary should remain unchanged through the repeated 4B+4B fault, four upper-half aligned stores, refault, retried success store, immediate pure-producer-tail-byte follow-up store, the upper-tail-word probe, and the upper-half widening probe sequence",
        ai_micro_mab_retry_byte4_exact_store_bytes_match(
            guard_before_phys, guard_before_seed, 8U) &&
        ai_micro_mab_retry_byte4_exact_store_bytes_match(
            guard_after_phys, guard_after_seed, 8U)
    );

    ai_micro_mab_retry_byte4_exact_store_reset_env();

    TEST_END(
        "ai_micro_m_samepage_cross16b_store_refault_upperx4_retry_4b4b_store_then_pure_producer_tail_byte_store_then_upper_tail_word_load_then_upper_half_load_corner");
}

bool ai_micro_m_samepage_cross16b_store_refault_upperx4_retry_4b4b_store_then_pure_producer_tail_byte_store_then_tail_last_halfword_high_byte_load_then_upper_tail_word_load_corner()
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
    uint64_t expected_tail_last_halfword_high_byte_value = 0ULL;
    uint64_t observed_tail_last_halfword_high_byte_value = 0ULL;
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
    TEST_ASSERT("for the no-H same-page cross-16B translated scalar store, the first repeated 4B+4B second-split write fault must still report store access fault on the true second-half start address before the retried-success then pure-producer-tail-byte store plus the byte15 and upper-tail-word follow-up probes",
        excpt.triggered == true &&
        excpt.cause == CAUSE_SAF &&
        excpt.tval == second_half_vaddr
    );

    TEST_SETUP_EXCEPT();
    ai_micro_mab_retry_byte4_exact_store_translated_sd(
        fault_vaddr, first_fault_store_value);
    TEST_ASSERT("repeating the same no-H second-split 4B+4B write fault keeps the same precise metadata before the retried-success then pure-producer-tail-byte store plus the byte15 and upper-tail-word follow-up probes",
        excpt.triggered == true &&
        excpt.cause == CAUSE_SAF &&
        excpt.tval == second_half_vaddr
    );

    ai_micro_mab_retry_byte4_exact_store_restore_pmp_full_access();

    TEST_SETUP_EXCEPT();
    ai_micro_mab_retry_byte4_exact_store_translated_sd(
        aligned_upper_store_vaddr, aligned_upper_store_value_first);
    TEST_ASSERT("after repair, the first same-boundary translated upper-half aligned store should complete cleanly before the refaulted retry-4B+4B-store then pure-producer-tail-byte store plus the byte15 and upper-tail-word follow-up probes",
        excpt.triggered == false
    );

    TEST_SETUP_EXCEPT();
    ai_micro_mab_retry_byte4_exact_store_translated_sd(
        aligned_upper_store_vaddr, aligned_upper_store_value_second);
    TEST_ASSERT("after one upper-half aligned translated store has already completed on the repaired boundary, the second upper-half aligned translated store should stay trap-free before the refaulted retry-4B+4B-store then pure-producer-tail-byte store plus the byte15 and upper-tail-word follow-up probes",
        excpt.triggered == false
    );

    TEST_SETUP_EXCEPT();
    ai_micro_mab_retry_byte4_exact_store_translated_sd(
        aligned_upper_store_vaddr, aligned_upper_store_value_third);
    TEST_ASSERT("after two upper-half aligned translated stores have already completed on the repaired boundary, the third upper-half aligned translated store should stay trap-free before the refaulted retry-4B+4B-store then pure-producer-tail-byte store plus the byte15 and upper-tail-word follow-up probes",
        excpt.triggered == false
    );

    TEST_SETUP_EXCEPT();
    ai_micro_mab_retry_byte4_exact_store_translated_sd(
        aligned_upper_store_vaddr, aligned_upper_store_value_fourth);
    TEST_ASSERT("after three upper-half aligned translated stores have already completed on the repaired boundary, the fourth upper-half aligned translated store should stay trap-free and become the latest committed high-half payload before the refaulted retry-4B+4B-store then pure-producer-tail-byte store plus the byte15 and upper-tail-word follow-up probes",
        excpt.triggered == false
    );

    ai_micro_mab_retry_byte4_exact_store_configure_pmp_window(
        deny_start, deny_end,
        AI_MICRO_MAB_RETRY_BYTE4_EXACT_STORE_PMP_TOR_NO_W);

    TEST_SETUP_EXCEPT();
    ai_micro_mab_retry_byte4_exact_store_translated_sd(
        fault_vaddr, refault_store_value);
    TEST_ASSERT("after four upper-half aligned translated stores have already completed on the repaired boundary, restoring the upper-half deny window and reissuing the original 4B+4B misaligned store should refresh the same precise second-split store access fault metadata before the pure-producer-tail-byte store plus the byte15 and upper-tail-word follow-up probes",
        excpt.triggered == true &&
        excpt.cause == CAUSE_SAF &&
        excpt.tval == second_half_vaddr
    );

    ai_micro_mab_retry_byte4_exact_store_restore_pmp_full_access();

    TEST_SETUP_EXCEPT();
    ai_micro_mab_retry_byte4_exact_store_translated_sd(
        fault_vaddr, retry_4b4b_store_value);
    TEST_ASSERT("after the refault episode is repaired again, retrying the same 4B+4B translated store as a success store should clear stale exception metadata before the immediate pure-producer-tail-byte store and the byte15 plus upper-tail-word follow-up probes",
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
    expected_upper_tail_word_load_value =
        ai_micro_mab_retry_byte4_exact_store_read_little_endian(
            expected_boundary + 12U, sizeof(uint32_t));

    TEST_SETUP_EXCEPT();
    observed_tail_last_halfword_high_byte_value =
        ai_micro_mab_retry_byte4_exact_store_translated_lbu(
            tail_last_halfword_high_byte_load_vaddr);
    TEST_ASSERT("after the pure-producer tail-byte store has already refreshed byte15 on top of the retried 4B+4B image, the immediate translated byte15 confirmation probe should clear stale exception metadata and read the new tail byte before the following upper-tail-word widening probe",
        excpt.triggered == false &&
        observed_tail_last_halfword_high_byte_value ==
            expected_tail_last_halfword_high_byte_value
    );

    TEST_SETUP_EXCEPT();
    observed_upper_tail_word_load_value =
        ai_micro_mab_retry_byte4_exact_store_translated_lwu(
            upper_tail_word_load_vaddr);
    TEST_ASSERT("after the byte15 confirmation probe has already shown the new tail byte on top of the retried 4B+4B image, the following translated upper-tail-word load at boundary+12 should still clear stale exception metadata and return retained bytes12-14 plus the new tail byte instead of falling back to stale exact-BYTE4 state",
        excpt.triggered == false &&
        observed_upper_tail_word_load_value ==
            expected_upper_tail_word_load_value
    );

    TEST_ASSERT("after the prolonged upper-half producer, repaired refault, retried 4B+4B success store, immediate pure-producer tail-byte store, the byte15 probe, and the upper-tail-word probe, the final boundary bytes should preserve bytes4-11 from the retried image, keep bytes12-14 from the latest aligned upper-half image, and refresh only byte15",
        ai_micro_mab_retry_byte4_exact_store_bytes_match(
            boundary_phys, expected_boundary, 16U)
    );

    TEST_ASSERT("guard regions around the same-page boundary should remain unchanged through the repeated 4B+4B fault, four upper-half aligned stores, refault, retried success store, immediate pure-producer-tail-byte follow-up store, the byte15 probe, and the upper-tail-word probe sequence",
        ai_micro_mab_retry_byte4_exact_store_bytes_match(
            guard_before_phys, guard_before_seed, 8U) &&
        ai_micro_mab_retry_byte4_exact_store_bytes_match(
            guard_after_phys, guard_after_seed, 8U)
    );

    ai_micro_mab_retry_byte4_exact_store_reset_env();

    TEST_END(
        "ai_micro_m_samepage_cross16b_store_refault_upperx4_retry_4b4b_store_then_pure_producer_tail_byte_store_then_tail_last_halfword_high_byte_load_then_upper_tail_word_load_corner");
}

bool ai_micro_m_samepage_cross16b_store_refault_upperx4_retry_4b4b_store_then_pure_producer_tail_byte_store_then_tail_last_halfword_low_byte_load_then_upper_tail_word_load_corner()
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
    uint64_t expected_tail_last_halfword_low_byte_value = 0ULL;
    uint64_t observed_tail_last_halfword_low_byte_value = 0ULL;
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
    TEST_ASSERT("for the no-H same-page cross-16B translated scalar store, the first repeated 4B+4B second-split write fault must still report store access fault on the true second-half start address before the retried-success then pure-producer-tail-byte store plus the byte14 and upper-tail-word follow-up probes",
        excpt.triggered == true &&
        excpt.cause == CAUSE_SAF &&
        excpt.tval == second_half_vaddr
    );

    TEST_SETUP_EXCEPT();
    ai_micro_mab_retry_byte4_exact_store_translated_sd(
        fault_vaddr, first_fault_store_value);
    TEST_ASSERT("repeating the same no-H second-split 4B+4B write fault keeps the same precise metadata before the retried-success then pure-producer-tail-byte store plus the byte14 and upper-tail-word follow-up probes",
        excpt.triggered == true &&
        excpt.cause == CAUSE_SAF &&
        excpt.tval == second_half_vaddr
    );

    ai_micro_mab_retry_byte4_exact_store_restore_pmp_full_access();

    TEST_SETUP_EXCEPT();
    ai_micro_mab_retry_byte4_exact_store_translated_sd(
        aligned_upper_store_vaddr, aligned_upper_store_value_first);
    TEST_ASSERT("after repair, the first same-boundary translated upper-half aligned store should complete cleanly before the refaulted retry-4B+4B-store then pure-producer-tail-byte store plus the byte14 and upper-tail-word follow-up probes",
        excpt.triggered == false
    );

    TEST_SETUP_EXCEPT();
    ai_micro_mab_retry_byte4_exact_store_translated_sd(
        aligned_upper_store_vaddr, aligned_upper_store_value_second);
    TEST_ASSERT("after one upper-half aligned translated store has already completed on the repaired boundary, the second upper-half aligned translated store should stay trap-free before the refaulted retry-4B+4B-store then pure-producer-tail-byte store plus the byte14 and upper-tail-word follow-up probes",
        excpt.triggered == false
    );

    TEST_SETUP_EXCEPT();
    ai_micro_mab_retry_byte4_exact_store_translated_sd(
        aligned_upper_store_vaddr, aligned_upper_store_value_third);
    TEST_ASSERT("after two upper-half aligned translated stores have already completed on the repaired boundary, the third upper-half aligned translated store should stay trap-free before the refaulted retry-4B+4B-store then pure-producer-tail-byte store plus the byte14 and upper-tail-word follow-up probes",
        excpt.triggered == false
    );

    TEST_SETUP_EXCEPT();
    ai_micro_mab_retry_byte4_exact_store_translated_sd(
        aligned_upper_store_vaddr, aligned_upper_store_value_fourth);
    TEST_ASSERT("after three upper-half aligned translated stores have already completed on the repaired boundary, the fourth upper-half aligned translated store should stay trap-free and become the latest committed high-half payload before the refaulted retry-4B+4B-store then pure-producer-tail-byte store plus the byte14 and upper-tail-word follow-up probes",
        excpt.triggered == false
    );

    ai_micro_mab_retry_byte4_exact_store_configure_pmp_window(
        deny_start, deny_end,
        AI_MICRO_MAB_RETRY_BYTE4_EXACT_STORE_PMP_TOR_NO_W);

    TEST_SETUP_EXCEPT();
    ai_micro_mab_retry_byte4_exact_store_translated_sd(
        fault_vaddr, refault_store_value);
    TEST_ASSERT("after four upper-half aligned translated stores have already completed on the repaired boundary, restoring the upper-half deny window and reissuing the original 4B+4B misaligned store should refresh the same precise second-split store access fault metadata before the pure-producer-tail-byte store plus the byte14 and upper-tail-word follow-up probes",
        excpt.triggered == true &&
        excpt.cause == CAUSE_SAF &&
        excpt.tval == second_half_vaddr
    );

    ai_micro_mab_retry_byte4_exact_store_restore_pmp_full_access();

    TEST_SETUP_EXCEPT();
    ai_micro_mab_retry_byte4_exact_store_translated_sd(
        fault_vaddr, retry_4b4b_store_value);
    TEST_ASSERT("after the refault episode is repaired again, retrying the same 4B+4B translated store as a success store should clear stale exception metadata before the immediate pure-producer-tail-byte store and the byte14 plus upper-tail-word follow-up probes",
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
    expected_tail_last_halfword_low_byte_value =
        (uint64_t)expected_boundary[14];
    expected_upper_tail_word_load_value =
        ai_micro_mab_retry_byte4_exact_store_read_little_endian(
            expected_boundary + 12U, sizeof(uint32_t));

    TEST_SETUP_EXCEPT();
    observed_tail_last_halfword_low_byte_value =
        ai_micro_mab_retry_byte4_exact_store_translated_lbu(
            tail_last_halfword_low_byte_load_vaddr);
    TEST_ASSERT("after the pure-producer tail-byte store has already refreshed byte15 on top of the retried 4B+4B image, the immediate translated byte14 retained-byte probe should clear stale exception metadata and read the latest producer-owned byte14 before the following upper-tail-word widening probe",
        excpt.triggered == false &&
        observed_tail_last_halfword_low_byte_value ==
            expected_tail_last_halfword_low_byte_value
    );

    TEST_SETUP_EXCEPT();
    observed_upper_tail_word_load_value =
        ai_micro_mab_retry_byte4_exact_store_translated_lwu(
            upper_tail_word_load_vaddr);
    TEST_ASSERT("after the byte14 retained-byte probe has already shown the latest producer-owned byte14 on top of the retried 4B+4B image, the following translated upper-tail-word load at boundary+12 should still clear stale exception metadata and return retained bytes12-14 plus the new tail byte instead of falling back to stale exact-BYTE4 state",
        excpt.triggered == false &&
        observed_upper_tail_word_load_value ==
            expected_upper_tail_word_load_value
    );

    TEST_ASSERT("after the prolonged upper-half producer, repaired refault, retried 4B+4B success store, immediate pure-producer tail-byte store, the byte14 probe, and the upper-tail-word probe, the final boundary bytes should preserve bytes4-11 from the retried image, keep bytes12-14 from the latest aligned upper-half image, and refresh only byte15",
        ai_micro_mab_retry_byte4_exact_store_bytes_match(
            boundary_phys, expected_boundary, 16U)
    );

    TEST_ASSERT("guard regions around the same-page boundary should remain unchanged through the repeated 4B+4B fault, four upper-half aligned stores, refault, retried success store, immediate pure-producer-tail-byte follow-up store, the byte14 probe, and the upper-tail-word probe sequence",
        ai_micro_mab_retry_byte4_exact_store_bytes_match(
            guard_before_phys, guard_before_seed, 8U) &&
        ai_micro_mab_retry_byte4_exact_store_bytes_match(
            guard_after_phys, guard_after_seed, 8U)
    );

    ai_micro_mab_retry_byte4_exact_store_reset_env();

    TEST_END(
        "ai_micro_m_samepage_cross16b_store_refault_upperx4_retry_4b4b_store_then_pure_producer_tail_byte_store_then_tail_last_halfword_low_byte_load_then_upper_tail_word_load_corner");
}

bool ai_micro_m_samepage_cross16b_store_refault_upperx4_retry_4b4b_store_then_pure_producer_tail_byte_store_then_tail_last_halfword_load_then_upper_tail_word_load_corner()
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
    const uintptr_t tail_last_halfword_load_vaddr =
        aligned_upper_store_vaddr + 6U;
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
    uint64_t expected_tail_last_halfword_load_value = 0ULL;
    uint64_t observed_tail_last_halfword_load_value = 0ULL;
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
    TEST_ASSERT("for the no-H same-page cross-16B translated scalar store, the first repeated 4B+4B second-split write fault must still report store access fault on the true second-half start address before the retried-success then pure-producer-tail-byte store plus the tail-last-halfword and upper-tail-word follow-up probes",
        excpt.triggered == true &&
        excpt.cause == CAUSE_SAF &&
        excpt.tval == second_half_vaddr
    );

    TEST_SETUP_EXCEPT();
    ai_micro_mab_retry_byte4_exact_store_translated_sd(
        fault_vaddr, first_fault_store_value);
    TEST_ASSERT("repeating the same no-H second-split 4B+4B write fault keeps the same precise metadata before the retried-success then pure-producer-tail-byte store plus the tail-last-halfword and upper-tail-word follow-up probes",
        excpt.triggered == true &&
        excpt.cause == CAUSE_SAF &&
        excpt.tval == second_half_vaddr
    );

    ai_micro_mab_retry_byte4_exact_store_restore_pmp_full_access();

    TEST_SETUP_EXCEPT();
    ai_micro_mab_retry_byte4_exact_store_translated_sd(
        aligned_upper_store_vaddr, aligned_upper_store_value_first);
    TEST_ASSERT("after repair, the first same-boundary translated upper-half aligned store should complete cleanly before the refaulted retry-4B+4B-store then pure-producer-tail-byte store plus the tail-last-halfword and upper-tail-word follow-up probes",
        excpt.triggered == false
    );

    TEST_SETUP_EXCEPT();
    ai_micro_mab_retry_byte4_exact_store_translated_sd(
        aligned_upper_store_vaddr, aligned_upper_store_value_second);
    TEST_ASSERT("after one upper-half aligned translated store has already completed on the repaired boundary, the second upper-half aligned translated store should stay trap-free before the refaulted retry-4B+4B-store then pure-producer-tail-byte store plus the tail-last-halfword and upper-tail-word follow-up probes",
        excpt.triggered == false
    );

    TEST_SETUP_EXCEPT();
    ai_micro_mab_retry_byte4_exact_store_translated_sd(
        aligned_upper_store_vaddr, aligned_upper_store_value_third);
    TEST_ASSERT("after two upper-half aligned translated stores have already completed on the repaired boundary, the third upper-half aligned translated store should stay trap-free before the refaulted retry-4B+4B-store then pure-producer-tail-byte store plus the tail-last-halfword and upper-tail-word follow-up probes",
        excpt.triggered == false
    );

    TEST_SETUP_EXCEPT();
    ai_micro_mab_retry_byte4_exact_store_translated_sd(
        aligned_upper_store_vaddr, aligned_upper_store_value_fourth);
    TEST_ASSERT("after three upper-half aligned translated stores have already completed on the repaired boundary, the fourth upper-half aligned translated store should stay trap-free and become the latest committed high-half payload before the refaulted retry-4B+4B-store then pure-producer-tail-byte store plus the tail-last-halfword and upper-tail-word follow-up probes",
        excpt.triggered == false
    );

    ai_micro_mab_retry_byte4_exact_store_configure_pmp_window(
        deny_start, deny_end,
        AI_MICRO_MAB_RETRY_BYTE4_EXACT_STORE_PMP_TOR_NO_W);

    TEST_SETUP_EXCEPT();
    ai_micro_mab_retry_byte4_exact_store_translated_sd(
        fault_vaddr, refault_store_value);
    TEST_ASSERT("after four upper-half aligned translated stores have already completed on the repaired boundary, restoring the upper-half deny window and reissuing the original 4B+4B misaligned store should refresh the same precise second-split store access fault metadata before the pure-producer-tail-byte store plus the tail-last-halfword and upper-tail-word follow-up probes",
        excpt.triggered == true &&
        excpt.cause == CAUSE_SAF &&
        excpt.tval == second_half_vaddr
    );

    ai_micro_mab_retry_byte4_exact_store_restore_pmp_full_access();

    TEST_SETUP_EXCEPT();
    ai_micro_mab_retry_byte4_exact_store_translated_sd(
        fault_vaddr, retry_4b4b_store_value);
    TEST_ASSERT("after the refault episode is repaired again, retrying the same 4B+4B translated store as a success store should clear stale exception metadata before the immediate pure-producer-tail-byte store and the tail-last-halfword plus upper-tail-word follow-up probes",
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
    expected_tail_last_halfword_load_value =
        ai_micro_mab_retry_byte4_exact_store_read_little_endian(
            expected_boundary + 14U, sizeof(uint16_t));
    expected_upper_tail_word_load_value =
        ai_micro_mab_retry_byte4_exact_store_read_little_endian(
            expected_boundary + 12U, sizeof(uint32_t));

    TEST_SETUP_EXCEPT();
    observed_tail_last_halfword_load_value =
        ai_micro_mab_retry_byte4_exact_store_translated_lhu(
            tail_last_halfword_load_vaddr);
    TEST_ASSERT("after the pure-producer tail-byte store has already refreshed byte15 on top of the retried 4B+4B image, the immediate translated tail-last-halfword probe should clear stale exception metadata and read retained byte14 plus the new tail byte before the following upper-tail-word widening probe",
        excpt.triggered == false &&
        observed_tail_last_halfword_load_value ==
            expected_tail_last_halfword_load_value
    );

    TEST_SETUP_EXCEPT();
    observed_upper_tail_word_load_value =
        ai_micro_mab_retry_byte4_exact_store_translated_lwu(
            upper_tail_word_load_vaddr);
    TEST_ASSERT("after the tail-last-halfword probe has already shown retained byte14 plus the new tail byte on top of the retried 4B+4B image, the following translated upper-tail-word load at boundary+12 should still clear stale exception metadata and return retained bytes12-14 plus the new tail byte instead of falling back to stale exact-BYTE4 state",
        excpt.triggered == false &&
        observed_upper_tail_word_load_value ==
            expected_upper_tail_word_load_value
    );

    TEST_ASSERT("after the prolonged upper-half producer, repaired refault, retried 4B+4B success store, immediate pure-producer tail-byte store, the tail-last-halfword probe, and the upper-tail-word probe, the final boundary bytes should preserve bytes4-11 from the retried image, keep bytes12-14 from the latest aligned upper-half image, and refresh only byte15",
        ai_micro_mab_retry_byte4_exact_store_bytes_match(
            boundary_phys, expected_boundary, 16U)
    );

    TEST_ASSERT("guard regions around the same-page boundary should remain unchanged through the repeated 4B+4B fault, four upper-half aligned stores, refault, retried success store, immediate pure-producer-tail-byte follow-up store, the tail-last-halfword probe, and the upper-tail-word probe sequence",
        ai_micro_mab_retry_byte4_exact_store_bytes_match(
            guard_before_phys, guard_before_seed, 8U) &&
        ai_micro_mab_retry_byte4_exact_store_bytes_match(
            guard_after_phys, guard_after_seed, 8U)
    );

    ai_micro_mab_retry_byte4_exact_store_reset_env();

    TEST_END(
        "ai_micro_m_samepage_cross16b_store_refault_upperx4_retry_4b4b_store_then_pure_producer_tail_byte_store_then_tail_last_halfword_load_then_upper_tail_word_load_corner");
}

bool ai_micro_m_samepage_cross16b_store_refault_upperx4_retry_4b4b_store_then_pure_producer_tail_byte_store_then_tail_last_halfword_load_then_upper_half_load_corner()
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
    const uintptr_t tail_last_halfword_load_vaddr =
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
    uint64_t expected_tail_last_halfword_load_value = 0ULL;
    uint64_t observed_tail_last_halfword_load_value = 0ULL;
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
    TEST_ASSERT("for the no-H same-page cross-16B translated scalar store, the first repeated 4B+4B second-split write fault must still report store access fault on the true second-half start address before the retried-success then pure-producer-tail-byte store plus the tail-last-halfword and upper-half follow-up probes",
        excpt.triggered == true &&
        excpt.cause == CAUSE_SAF &&
        excpt.tval == second_half_vaddr
    );

    TEST_SETUP_EXCEPT();
    ai_micro_mab_retry_byte4_exact_store_translated_sd(
        fault_vaddr, first_fault_store_value);
    TEST_ASSERT("repeating the same no-H second-split 4B+4B write fault keeps the same precise metadata before the retried-success then pure-producer-tail-byte store plus the tail-last-halfword and upper-half follow-up probes",
        excpt.triggered == true &&
        excpt.cause == CAUSE_SAF &&
        excpt.tval == second_half_vaddr
    );

    ai_micro_mab_retry_byte4_exact_store_restore_pmp_full_access();

    TEST_SETUP_EXCEPT();
    ai_micro_mab_retry_byte4_exact_store_translated_sd(
        aligned_upper_store_vaddr, aligned_upper_store_value_first);
    TEST_ASSERT("after repair, the first same-boundary translated upper-half aligned store should complete cleanly before the refaulted retry-4B+4B-store then pure-producer-tail-byte store plus the tail-last-halfword and upper-half follow-up probes",
        excpt.triggered == false
    );

    TEST_SETUP_EXCEPT();
    ai_micro_mab_retry_byte4_exact_store_translated_sd(
        aligned_upper_store_vaddr, aligned_upper_store_value_second);
    TEST_ASSERT("after one upper-half aligned translated store has already completed on the repaired boundary, the second upper-half aligned translated store should stay trap-free before the refaulted retry-4B+4B-store then pure-producer-tail-byte store plus the tail-last-halfword and upper-half follow-up probes",
        excpt.triggered == false
    );

    TEST_SETUP_EXCEPT();
    ai_micro_mab_retry_byte4_exact_store_translated_sd(
        aligned_upper_store_vaddr, aligned_upper_store_value_third);
    TEST_ASSERT("after two upper-half aligned translated stores have already completed on the repaired boundary, the third upper-half aligned translated store should stay trap-free before the refaulted retry-4B+4B-store then pure-producer-tail-byte store plus the tail-last-halfword and upper-half follow-up probes",
        excpt.triggered == false
    );

    TEST_SETUP_EXCEPT();
    ai_micro_mab_retry_byte4_exact_store_translated_sd(
        aligned_upper_store_vaddr, aligned_upper_store_value_fourth);
    TEST_ASSERT("after three upper-half aligned translated stores have already completed on the repaired boundary, the fourth upper-half aligned translated store should stay trap-free and become the latest committed high-half payload before the refaulted retry-4B+4B-store then pure-producer-tail-byte store plus the tail-last-halfword and upper-half follow-up probes",
        excpt.triggered == false
    );

    ai_micro_mab_retry_byte4_exact_store_configure_pmp_window(
        deny_start, deny_end,
        AI_MICRO_MAB_RETRY_BYTE4_EXACT_STORE_PMP_TOR_NO_W);

    TEST_SETUP_EXCEPT();
    ai_micro_mab_retry_byte4_exact_store_translated_sd(
        fault_vaddr, refault_store_value);
    TEST_ASSERT("after four upper-half aligned translated stores have already completed on the repaired boundary, restoring the upper-half deny window and reissuing the original 4B+4B misaligned store should refresh the same precise second-split store access fault metadata before the pure-producer-tail-byte store plus the tail-last-halfword and upper-half follow-up probes",
        excpt.triggered == true &&
        excpt.cause == CAUSE_SAF &&
        excpt.tval == second_half_vaddr
    );

    ai_micro_mab_retry_byte4_exact_store_restore_pmp_full_access();

    TEST_SETUP_EXCEPT();
    ai_micro_mab_retry_byte4_exact_store_translated_sd(
        fault_vaddr, retry_4b4b_store_value);
    TEST_ASSERT("after the refault episode is repaired again, retrying the same 4B+4B translated store as a success store should clear stale exception metadata before the immediate pure-producer-tail-byte store and the tail-last-halfword plus upper-half follow-up probes",
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
    expected_tail_last_halfword_load_value =
        ai_micro_mab_retry_byte4_exact_store_read_little_endian(
            expected_boundary + 14U, sizeof(uint16_t));
    expected_upper_half_load_value =
        ai_micro_mab_retry_byte4_exact_store_read_little_endian(
            expected_boundary + 8U, sizeof(uint64_t));

    TEST_SETUP_EXCEPT();
    observed_tail_last_halfword_load_value =
        ai_micro_mab_retry_byte4_exact_store_translated_lhu(
            tail_last_halfword_load_vaddr);
    TEST_ASSERT("after the pure-producer tail-byte store has already refreshed byte15 on top of the retried 4B+4B image, the immediate translated tail-last-halfword probe should clear stale exception metadata and read retained byte14 plus the new tail byte before the following aligned upper-half widening probe",
        excpt.triggered == false &&
        observed_tail_last_halfword_load_value ==
            expected_tail_last_halfword_load_value
    );

    TEST_SETUP_EXCEPT();
    observed_upper_half_load_value =
        ai_micro_mab_retry_byte4_exact_store_translated_ld(
            upper_half_load_vaddr);
    TEST_ASSERT("after the tail-last-halfword probe has already shown retained byte14 plus the new tail byte on top of the retried 4B+4B image, the following aligned upper-half translated load at boundary+8 should still clear stale exception metadata and read the merged latest image of retry bytes8-11, retained producer bytes12-14, and the new tail byte instead of falling back to stale exact-BYTE4 state",
        excpt.triggered == false &&
        observed_upper_half_load_value == expected_upper_half_load_value
    );

    TEST_ASSERT("after the prolonged upper-half producer, repaired refault, retried 4B+4B success store, immediate pure-producer tail-byte store, the tail-last-halfword probe, and the upper-half load probe, the final boundary bytes should preserve bytes4-11 from the retried image, keep bytes12-14 from the latest aligned upper-half image, and refresh only byte15",
        ai_micro_mab_retry_byte4_exact_store_bytes_match(
            boundary_phys, expected_boundary, 16U)
    );

    TEST_ASSERT("guard regions around the same-page boundary should remain unchanged through the repeated 4B+4B fault, four upper-half aligned stores, refault, retried success store, immediate pure-producer-tail-byte follow-up store, the tail-last-halfword probe, and the aligned upper-half load probe sequence",
        ai_micro_mab_retry_byte4_exact_store_bytes_match(
            guard_before_phys, guard_before_seed, 8U) &&
        ai_micro_mab_retry_byte4_exact_store_bytes_match(
            guard_after_phys, guard_after_seed, 8U)
    );

    ai_micro_mab_retry_byte4_exact_store_reset_env();

    TEST_END(
        "ai_micro_m_samepage_cross16b_store_refault_upperx4_retry_4b4b_store_then_pure_producer_tail_byte_store_then_tail_last_halfword_load_then_upper_half_load_corner");
}

bool ai_micro_m_samepage_cross16b_store_refault_upperx4_retry_4b4b_store_then_pure_producer_tail_byte_store_then_upper_tail_first_byte_load_then_upper_tail_word_load_corner()
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
    const uintptr_t upper_tail_first_byte_load_vaddr =
        aligned_upper_store_vaddr + 4U;
    const uintptr_t upper_tail_word_load_vaddr =
        upper_tail_first_byte_load_vaddr;
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
    uint64_t expected_upper_tail_first_byte_value = 0ULL;
    uint64_t observed_upper_tail_first_byte_value = 0ULL;
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
    TEST_ASSERT("for the no-H same-page cross-16B translated scalar store, the first repeated 4B+4B second-split write fault must still report store access fault on the true second-half start address before the retried-success then pure-producer-tail-byte store plus the upper-tail-first-byte and upper-tail-word follow-up probes",
        excpt.triggered == true &&
        excpt.cause == CAUSE_SAF &&
        excpt.tval == second_half_vaddr
    );

    TEST_SETUP_EXCEPT();
    ai_micro_mab_retry_byte4_exact_store_translated_sd(
        fault_vaddr, first_fault_store_value);
    TEST_ASSERT("repeating the same no-H second-split 4B+4B write fault keeps the same precise metadata before the retried-success then pure-producer-tail-byte store plus the upper-tail-first-byte and upper-tail-word follow-up probes",
        excpt.triggered == true &&
        excpt.cause == CAUSE_SAF &&
        excpt.tval == second_half_vaddr
    );

    ai_micro_mab_retry_byte4_exact_store_restore_pmp_full_access();

    TEST_SETUP_EXCEPT();
    ai_micro_mab_retry_byte4_exact_store_translated_sd(
        aligned_upper_store_vaddr, aligned_upper_store_value_first);
    TEST_ASSERT("after repair, the first same-boundary translated upper-half aligned store should complete cleanly before the refaulted retry-4B+4B-store then pure-producer-tail-byte store plus the upper-tail-first-byte and upper-tail-word follow-up probes",
        excpt.triggered == false
    );

    TEST_SETUP_EXCEPT();
    ai_micro_mab_retry_byte4_exact_store_translated_sd(
        aligned_upper_store_vaddr, aligned_upper_store_value_second);
    TEST_ASSERT("after one upper-half aligned translated store has already completed on the repaired boundary, the second upper-half aligned translated store should stay trap-free before the refaulted retry-4B+4B-store then pure-producer-tail-byte store plus the upper-tail-first-byte and upper-tail-word follow-up probes",
        excpt.triggered == false
    );

    TEST_SETUP_EXCEPT();
    ai_micro_mab_retry_byte4_exact_store_translated_sd(
        aligned_upper_store_vaddr, aligned_upper_store_value_third);
    TEST_ASSERT("after two upper-half aligned translated stores have already completed on the repaired boundary, the third upper-half aligned translated store should stay trap-free before the refaulted retry-4B+4B-store then pure-producer-tail-byte store plus the upper-tail-first-byte and upper-tail-word follow-up probes",
        excpt.triggered == false
    );

    TEST_SETUP_EXCEPT();
    ai_micro_mab_retry_byte4_exact_store_translated_sd(
        aligned_upper_store_vaddr, aligned_upper_store_value_fourth);
    TEST_ASSERT("after three upper-half aligned translated stores have already completed on the repaired boundary, the fourth upper-half aligned translated store should stay trap-free and become the latest committed high-half payload before the refaulted retry-4B+4B-store then pure-producer-tail-byte store plus the upper-tail-first-byte and upper-tail-word follow-up probes",
        excpt.triggered == false
    );

    ai_micro_mab_retry_byte4_exact_store_configure_pmp_window(
        deny_start, deny_end,
        AI_MICRO_MAB_RETRY_BYTE4_EXACT_STORE_PMP_TOR_NO_W);

    TEST_SETUP_EXCEPT();
    ai_micro_mab_retry_byte4_exact_store_translated_sd(
        fault_vaddr, refault_store_value);
    TEST_ASSERT("after four upper-half aligned translated stores have already completed on the repaired boundary, restoring the upper-half deny window and reissuing the original 4B+4B misaligned store should refresh the same precise second-split store access fault metadata before the pure-producer-tail-byte store plus the upper-tail-first-byte and upper-tail-word follow-up probes",
        excpt.triggered == true &&
        excpt.cause == CAUSE_SAF &&
        excpt.tval == second_half_vaddr
    );

    ai_micro_mab_retry_byte4_exact_store_restore_pmp_full_access();

    TEST_SETUP_EXCEPT();
    ai_micro_mab_retry_byte4_exact_store_translated_sd(
        fault_vaddr, retry_4b4b_store_value);
    TEST_ASSERT("after the refault episode is repaired again, retrying the same 4B+4B translated store as a success store should clear stale exception metadata before the immediate pure-producer-tail-byte store and the upper-tail-first-byte plus upper-tail-word follow-up probes",
        excpt.triggered == false
    );

    TEST_SETUP_EXCEPT();
    ai_micro_mab_retry_byte4_exact_store_translated_sb(
        followup_pure_producer_tail_byte_store_vaddr,
        followup_pure_producer_tail_byte_store_value);
    TEST_ASSERT("after the retried original-template 4B+4B store has already succeeded on the same boundary, the immediate translated pure-producer tail-byte store should also stay trap-free and must not let a retired fake-crosspage exact 4-byte high-half owner/template leak back into bytes8-11 or drag bytes12-14 away from the latest aligned producer-side image while byte15 refreshes",
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
    expected_upper_tail_first_byte_value =
        (uint64_t)expected_boundary[12];
    expected_upper_tail_word_load_value =
        ai_micro_mab_retry_byte4_exact_store_read_little_endian(
            expected_boundary + 12U, sizeof(uint32_t));

    TEST_SETUP_EXCEPT();
    observed_upper_tail_first_byte_value =
        ai_micro_mab_retry_byte4_exact_store_translated_lbu(
            upper_tail_first_byte_load_vaddr);
    TEST_ASSERT("after the pure-producer tail-byte store has already refreshed byte15 on top of the retried 4B+4B image, the immediate translated upper-tail first-byte probe at boundary+12 should clear stale exception metadata and read the latest retained producer-owned byte12 before the following upper-tail-word widening probe",
        excpt.triggered == false &&
        observed_upper_tail_first_byte_value ==
            expected_upper_tail_first_byte_value
    );

    TEST_SETUP_EXCEPT();
    observed_upper_tail_word_load_value =
        ai_micro_mab_retry_byte4_exact_store_translated_lwu(
            upper_tail_word_load_vaddr);
    TEST_ASSERT("after the upper-tail first-byte probe has already shown the latest retained byte12 on top of the retried 4B+4B image, the following translated upper-tail-word load at boundary+12 should still clear stale exception metadata and return retained bytes12-14 plus the new tail byte instead of falling back to stale exact-BYTE4 state",
        excpt.triggered == false &&
        observed_upper_tail_word_load_value ==
            expected_upper_tail_word_load_value
    );

    TEST_ASSERT("after the prolonged upper-half producer, repaired refault, retried 4B+4B success store, immediate pure-producer tail-byte store, the upper-tail first-byte probe, and the upper-tail-word probe, the final boundary bytes should preserve bytes4-11 from the retried image, keep bytes12-14 from the latest aligned upper-half image, and refresh only byte15",
        ai_micro_mab_retry_byte4_exact_store_bytes_match(
            boundary_phys, expected_boundary, 16U)
    );

    TEST_ASSERT("guard regions around the same-page boundary should remain unchanged through the repeated 4B+4B fault, four upper-half aligned stores, refault, retried success store, immediate pure-producer-tail-byte follow-up store, the upper-tail first-byte probe, and the upper-tail-word probe sequence",
        ai_micro_mab_retry_byte4_exact_store_bytes_match(
            guard_before_phys, guard_before_seed, 8U) &&
        ai_micro_mab_retry_byte4_exact_store_bytes_match(
            guard_after_phys, guard_after_seed, 8U)
    );

    ai_micro_mab_retry_byte4_exact_store_reset_env();

    TEST_END(
        "ai_micro_m_samepage_cross16b_store_refault_upperx4_retry_4b4b_store_then_pure_producer_tail_byte_store_then_upper_tail_first_byte_load_then_upper_tail_word_load_corner");
}

bool ai_micro_m_samepage_cross16b_store_refault_upperx4_retry_4b4b_store_then_pure_producer_tail_byte_store_then_upper_tail_second_byte_load_then_upper_tail_word_load_corner()
{
    TEST_START();

    static const uint8_t boundary_seed[16] = {
        0x47, 0x36, 0x25, 0x14, 0x03, 0xf2, 0xe1, 0xd0,
        0xcf, 0xbe, 0xad, 0x9c, 0x8b, 0x7a, 0x69, 0x58,
    };
    static const uint8_t guard_before_seed[8] = {
        0x21, 0x43, 0x65, 0x87, 0xa9, 0xcb, 0xed, 0x0f,
    };
    static const uint8_t guard_after_seed[8] = {
        0x10, 0x32, 0x54, 0x76, 0x98, 0xba, 0xdc, 0xfe,
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
    const uintptr_t upper_tail_second_byte_load_vaddr =
        aligned_upper_store_vaddr + 5U;
    const uintptr_t upper_tail_word_load_vaddr =
        aligned_upper_store_vaddr + 4U;
    const uintptr_t deny_start = boundary_phys + 8U;
    const uintptr_t deny_end = deny_start + PAGE_SIZE;
    const uint64_t first_fault_store_value = 0x758697a8b9cadbecULL;
    const uint64_t refault_store_value = 0xe91a2b3c4d5e6f70ULL;
    const uint64_t aligned_upper_store_value_first = 0x8697a8b9cadbecfdULL;
    const uint64_t aligned_upper_store_value_second = 0x1a2b3c4d5e6f7081ULL;
    const uint64_t aligned_upper_store_value_third = 0x97a8b9cadbecfd0eULL;
    const uint64_t aligned_upper_store_value_fourth = 0x2b3c4d5e6f708192ULL;
    const uint64_t retry_4b4b_store_value = 0xa8b9cadbecfd0e1fULL;
    const uint8_t followup_pure_producer_tail_byte_store_value = 0x93U;
    uint8_t expected_boundary[16];
    uint64_t expected_upper_tail_second_byte_value = 0ULL;
    uint64_t observed_upper_tail_second_byte_value = 0ULL;
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
    TEST_ASSERT("for the no-H same-page cross-16B translated scalar store, the first repeated 4B+4B second-split write fault must still report store access fault on the true second-half start address before the retried-success then pure-producer-tail-byte store plus the upper-tail-second-byte and upper-tail-word follow-up probes",
        excpt.triggered == true &&
        excpt.cause == CAUSE_SAF &&
        excpt.tval == second_half_vaddr
    );

    TEST_SETUP_EXCEPT();
    ai_micro_mab_retry_byte4_exact_store_translated_sd(
        fault_vaddr, first_fault_store_value);
    TEST_ASSERT("repeating the same no-H second-split 4B+4B write fault keeps the same precise metadata before the retried-success then pure-producer-tail-byte store plus the upper-tail-second-byte and upper-tail-word follow-up probes",
        excpt.triggered == true &&
        excpt.cause == CAUSE_SAF &&
        excpt.tval == second_half_vaddr
    );

    ai_micro_mab_retry_byte4_exact_store_restore_pmp_full_access();

    TEST_SETUP_EXCEPT();
    ai_micro_mab_retry_byte4_exact_store_translated_sd(
        aligned_upper_store_vaddr, aligned_upper_store_value_first);
    TEST_ASSERT("after repair, the first same-boundary translated upper-half aligned store should complete cleanly before the refaulted retry-4B+4B-store then pure-producer-tail-byte store plus the upper-tail-second-byte and upper-tail-word follow-up probes",
        excpt.triggered == false
    );

    TEST_SETUP_EXCEPT();
    ai_micro_mab_retry_byte4_exact_store_translated_sd(
        aligned_upper_store_vaddr, aligned_upper_store_value_second);
    TEST_ASSERT("after one upper-half aligned translated store has already completed on the repaired boundary, the second upper-half aligned translated store should stay trap-free before the refaulted retry-4B+4B-store then pure-producer-tail-byte store plus the upper-tail-second-byte and upper-tail-word follow-up probes",
        excpt.triggered == false
    );

    TEST_SETUP_EXCEPT();
    ai_micro_mab_retry_byte4_exact_store_translated_sd(
        aligned_upper_store_vaddr, aligned_upper_store_value_third);
    TEST_ASSERT("after two upper-half aligned translated stores have already completed on the repaired boundary, the third upper-half aligned translated store should stay trap-free before the refaulted retry-4B+4B-store then pure-producer-tail-byte store plus the upper-tail-second-byte and upper-tail-word follow-up probes",
        excpt.triggered == false
    );

    TEST_SETUP_EXCEPT();
    ai_micro_mab_retry_byte4_exact_store_translated_sd(
        aligned_upper_store_vaddr, aligned_upper_store_value_fourth);
    TEST_ASSERT("after three upper-half aligned translated stores have already completed on the repaired boundary, the fourth upper-half aligned translated store should stay trap-free and become the latest committed high-half payload before the refaulted retry-4B+4B-store then pure-producer-tail-byte store plus the upper-tail-second-byte and upper-tail-word follow-up probes",
        excpt.triggered == false
    );

    ai_micro_mab_retry_byte4_exact_store_configure_pmp_window(
        deny_start, deny_end,
        AI_MICRO_MAB_RETRY_BYTE4_EXACT_STORE_PMP_TOR_NO_W);

    TEST_SETUP_EXCEPT();
    ai_micro_mab_retry_byte4_exact_store_translated_sd(
        fault_vaddr, refault_store_value);
    TEST_ASSERT("after four upper-half aligned translated stores have already completed on the repaired boundary, restoring the upper-half deny window and reissuing the original 4B+4B misaligned store should refresh the same precise second-split store access fault metadata before the pure-producer-tail-byte store plus the upper-tail-second-byte and upper-tail-word follow-up probes",
        excpt.triggered == true &&
        excpt.cause == CAUSE_SAF &&
        excpt.tval == second_half_vaddr
    );

    ai_micro_mab_retry_byte4_exact_store_restore_pmp_full_access();

    TEST_SETUP_EXCEPT();
    ai_micro_mab_retry_byte4_exact_store_translated_sd(
        fault_vaddr, retry_4b4b_store_value);
    TEST_ASSERT("after the refault episode is repaired again, retrying the same 4B+4B translated store as a success store should clear stale exception metadata before the immediate pure-producer-tail-byte store and the upper-tail-second-byte plus upper-tail-word follow-up probes",
        excpt.triggered == false
    );

    TEST_SETUP_EXCEPT();
    ai_micro_mab_retry_byte4_exact_store_translated_sb(
        followup_pure_producer_tail_byte_store_vaddr,
        followup_pure_producer_tail_byte_store_value);
    TEST_ASSERT("after the retried original-template 4B+4B store has already succeeded on the same boundary, the immediate translated pure-producer tail-byte store should also stay trap-free and must not let a retired fake-crosspage exact 4-byte high-half owner/template leak back into bytes8-11 or drag byte13 away from the latest aligned producer-side image while byte15 refreshes",
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
    expected_upper_tail_second_byte_value =
        (uint64_t)expected_boundary[13];
    expected_upper_tail_word_load_value =
        ai_micro_mab_retry_byte4_exact_store_read_little_endian(
            expected_boundary + 12U, sizeof(uint32_t));

    TEST_SETUP_EXCEPT();
    observed_upper_tail_second_byte_value =
        ai_micro_mab_retry_byte4_exact_store_translated_lbu(
            upper_tail_second_byte_load_vaddr);
    TEST_ASSERT("after the pure-producer tail-byte store has already refreshed byte15 on top of the retried 4B+4B image, the immediate translated upper-tail second-byte probe at boundary+13 should clear stale exception metadata and read the latest retained producer-owned byte13 before the following upper-tail-word widening probe",
        excpt.triggered == false &&
        observed_upper_tail_second_byte_value ==
            expected_upper_tail_second_byte_value
    );

    TEST_SETUP_EXCEPT();
    observed_upper_tail_word_load_value =
        ai_micro_mab_retry_byte4_exact_store_translated_lwu(
            upper_tail_word_load_vaddr);
    TEST_ASSERT("after the upper-tail second-byte probe has already shown the latest retained byte13 on top of the retried 4B+4B image, the following translated upper-tail-word load at boundary+12 should still clear stale exception metadata and return retained bytes12-14 plus the new tail byte instead of falling back to stale exact-BYTE4 state",
        excpt.triggered == false &&
        observed_upper_tail_word_load_value ==
            expected_upper_tail_word_load_value
    );

    TEST_ASSERT("after the prolonged upper-half producer, repaired refault, retried 4B+4B success store, immediate pure-producer tail-byte store, the upper-tail second-byte probe, and the upper-tail-word probe, the final boundary bytes should preserve bytes4-11 from the retried image, keep bytes12-14 from the latest aligned upper-half image, and refresh only byte15",
        ai_micro_mab_retry_byte4_exact_store_bytes_match(
            boundary_phys, expected_boundary, 16U)
    );

    TEST_ASSERT("guard regions around the same-page boundary should remain unchanged through the repeated 4B+4B fault, four upper-half aligned stores, refault, retried success store, immediate pure-producer-tail-byte follow-up store, the upper-tail second-byte probe, and the upper-tail-word probe sequence",
        ai_micro_mab_retry_byte4_exact_store_bytes_match(
            guard_before_phys, guard_before_seed, 8U) &&
        ai_micro_mab_retry_byte4_exact_store_bytes_match(
            guard_after_phys, guard_after_seed, 8U)
    );

    ai_micro_mab_retry_byte4_exact_store_reset_env();

    TEST_END(
        "ai_micro_m_samepage_cross16b_store_refault_upperx4_retry_4b4b_store_then_pure_producer_tail_byte_store_then_upper_tail_second_byte_load_then_upper_tail_word_load_corner");
}

bool ai_micro_m_samepage_cross16b_store_refault_upperx4_retry_4b4b_store_then_pure_producer_tail_byte_store_then_upper_tail_second_byte_load_then_upper_half_load_corner()
{
    TEST_START();

    static const uint8_t boundary_seed[16] = {
        0x58, 0x47, 0x36, 0x25, 0x14, 0x03, 0xf2, 0xe1,
        0xd0, 0xcf, 0xbe, 0xad, 0x9c, 0x8b, 0x7a, 0x69,
    };
    static const uint8_t guard_before_seed[8] = {
        0x12, 0x34, 0x56, 0x78, 0x9a, 0xbc, 0xde, 0xf0,
    };
    static const uint8_t guard_after_seed[8] = {
        0x0f, 0x1e, 0x2d, 0x3c, 0x4b, 0x5a, 0x69, 0x78,
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
    const uintptr_t upper_tail_second_byte_load_vaddr =
        aligned_upper_store_vaddr + 5U;
    const uintptr_t upper_half_load_vaddr = aligned_upper_store_vaddr;
    const uintptr_t deny_start = boundary_phys + 8U;
    const uintptr_t deny_end = deny_start + PAGE_SIZE;
    const uint64_t first_fault_store_value = 0x768798a9bacbdcedULL;
    const uint64_t refault_store_value = 0xfa0b1c2d3e4f6071ULL;
    const uint64_t aligned_upper_store_value_first = 0x8798a9bacbdcedfeULL;
    const uint64_t aligned_upper_store_value_second = 0x0b1c2d3e4f607182ULL;
    const uint64_t aligned_upper_store_value_third = 0x98a9bacbdcedfe0fULL;
    const uint64_t aligned_upper_store_value_fourth = 0x1c2d3e4f60718293ULL;
    const uint64_t retry_4b4b_store_value = 0xa9bacbdcedfe0f10ULL;
    const uint8_t followup_pure_producer_tail_byte_store_value = 0xa4U;
    uint8_t expected_boundary[16];
    uint64_t expected_upper_tail_second_byte_value = 0ULL;
    uint64_t observed_upper_tail_second_byte_value = 0ULL;
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
    TEST_ASSERT("for the no-H same-page cross-16B translated scalar store, the first repeated 4B+4B second-split write fault must still report store access fault on the true second-half start address before the retried-success then pure-producer-tail-byte store plus the upper-tail-second-byte and upper-half follow-up probes",
        excpt.triggered == true &&
        excpt.cause == CAUSE_SAF &&
        excpt.tval == second_half_vaddr
    );

    TEST_SETUP_EXCEPT();
    ai_micro_mab_retry_byte4_exact_store_translated_sd(
        fault_vaddr, first_fault_store_value);
    TEST_ASSERT("repeating the same no-H second-split 4B+4B write fault keeps the same precise metadata before the retried-success then pure-producer-tail-byte store plus the upper-tail-second-byte and upper-half follow-up probes",
        excpt.triggered == true &&
        excpt.cause == CAUSE_SAF &&
        excpt.tval == second_half_vaddr
    );

    ai_micro_mab_retry_byte4_exact_store_restore_pmp_full_access();

    TEST_SETUP_EXCEPT();
    ai_micro_mab_retry_byte4_exact_store_translated_sd(
        aligned_upper_store_vaddr, aligned_upper_store_value_first);
    TEST_ASSERT("after repair, the first same-boundary translated upper-half aligned store should complete cleanly before the refaulted retry-4B+4B-store then pure-producer-tail-byte store plus the upper-tail-second-byte and upper-half follow-up probes",
        excpt.triggered == false
    );

    TEST_SETUP_EXCEPT();
    ai_micro_mab_retry_byte4_exact_store_translated_sd(
        aligned_upper_store_vaddr, aligned_upper_store_value_second);
    TEST_ASSERT("after one upper-half aligned translated store has already completed on the repaired boundary, the second upper-half aligned translated store should stay trap-free before the refaulted retry-4B+4B-store then pure-producer-tail-byte store plus the upper-tail-second-byte and upper-half follow-up probes",
        excpt.triggered == false
    );

    TEST_SETUP_EXCEPT();
    ai_micro_mab_retry_byte4_exact_store_translated_sd(
        aligned_upper_store_vaddr, aligned_upper_store_value_third);
    TEST_ASSERT("after two upper-half aligned translated stores have already completed on the repaired boundary, the third upper-half aligned translated store should stay trap-free before the refaulted retry-4B+4B-store then pure-producer-tail-byte store plus the upper-tail-second-byte and upper-half follow-up probes",
        excpt.triggered == false
    );

    TEST_SETUP_EXCEPT();
    ai_micro_mab_retry_byte4_exact_store_translated_sd(
        aligned_upper_store_vaddr, aligned_upper_store_value_fourth);
    TEST_ASSERT("after three upper-half aligned translated stores have already completed on the repaired boundary, the fourth upper-half aligned translated store should stay trap-free and become the latest committed high-half payload before the refaulted retry-4B+4B-store then pure-producer-tail-byte store plus the upper-tail-second-byte and upper-half follow-up probes",
        excpt.triggered == false
    );

    ai_micro_mab_retry_byte4_exact_store_configure_pmp_window(
        deny_start, deny_end,
        AI_MICRO_MAB_RETRY_BYTE4_EXACT_STORE_PMP_TOR_NO_W);

    TEST_SETUP_EXCEPT();
    ai_micro_mab_retry_byte4_exact_store_translated_sd(
        fault_vaddr, refault_store_value);
    TEST_ASSERT("after four upper-half aligned translated stores have already completed on the repaired boundary, restoring the upper-half deny window and reissuing the original 4B+4B misaligned store should refresh the same precise second-split store access fault metadata before the pure-producer-tail-byte store plus the upper-tail-second-byte and upper-half follow-up probes",
        excpt.triggered == true &&
        excpt.cause == CAUSE_SAF &&
        excpt.tval == second_half_vaddr
    );

    ai_micro_mab_retry_byte4_exact_store_restore_pmp_full_access();

    TEST_SETUP_EXCEPT();
    ai_micro_mab_retry_byte4_exact_store_translated_sd(
        fault_vaddr, retry_4b4b_store_value);
    TEST_ASSERT("after the refault episode is repaired again, retrying the same 4B+4B translated store as a success store should clear stale exception metadata before the immediate pure-producer-tail-byte store and the upper-tail-second-byte plus upper-half follow-up probes",
        excpt.triggered == false
    );

    TEST_SETUP_EXCEPT();
    ai_micro_mab_retry_byte4_exact_store_translated_sb(
        followup_pure_producer_tail_byte_store_vaddr,
        followup_pure_producer_tail_byte_store_value);
    TEST_ASSERT("after the retried original-template 4B+4B store has already succeeded on the same boundary, the immediate translated pure-producer tail-byte store should also stay trap-free and must not let a retired fake-crosspage exact 4-byte high-half owner/template leak back into bytes8-11 or drag byte13 away from the latest aligned producer-side image while byte15 refreshes",
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
    expected_upper_tail_second_byte_value =
        (uint64_t)expected_boundary[13];
    expected_upper_half_load_value =
        ai_micro_mab_retry_byte4_exact_store_read_little_endian(
            expected_boundary + 8U, sizeof(uint64_t));

    TEST_SETUP_EXCEPT();
    observed_upper_tail_second_byte_value =
        ai_micro_mab_retry_byte4_exact_store_translated_lbu(
            upper_tail_second_byte_load_vaddr);
    TEST_ASSERT("after the pure-producer tail-byte store has already refreshed byte15 on top of the retried 4B+4B image, the immediate translated upper-tail second-byte probe at boundary+13 should clear stale exception metadata and read the latest retained producer-owned byte13 before the following aligned upper-half widening probe",
        excpt.triggered == false &&
        observed_upper_tail_second_byte_value ==
            expected_upper_tail_second_byte_value
    );

    TEST_SETUP_EXCEPT();
    observed_upper_half_load_value =
        ai_micro_mab_retry_byte4_exact_store_translated_ld(
            upper_half_load_vaddr);
    TEST_ASSERT("after the upper-tail second-byte probe has already shown the latest retained byte13 on top of the retried 4B+4B image, the following aligned upper-half translated load at boundary+8 should still clear stale exception metadata and read the merged latest image of retry bytes8-11, retained producer bytes12-14, and the new tail byte instead of falling back to stale exact-BYTE4 state",
        excpt.triggered == false &&
        observed_upper_half_load_value == expected_upper_half_load_value
    );

    TEST_ASSERT("after the prolonged upper-half producer, repaired refault, retried 4B+4B success store, immediate pure-producer tail-byte store, the upper-tail second-byte probe, and the upper-half load probe, the final boundary bytes should preserve bytes4-11 from the retried image, keep bytes12-14 from the latest aligned upper-half image, and refresh only byte15",
        ai_micro_mab_retry_byte4_exact_store_bytes_match(
            boundary_phys, expected_boundary, 16U)
    );

    TEST_ASSERT("guard regions around the same-page boundary should remain unchanged through the repeated 4B+4B fault, four upper-half aligned stores, refault, retried success store, immediate pure-producer-tail-byte follow-up store, the upper-tail second-byte probe, and the upper-half load probe sequence",
        ai_micro_mab_retry_byte4_exact_store_bytes_match(
            guard_before_phys, guard_before_seed, 8U) &&
        ai_micro_mab_retry_byte4_exact_store_bytes_match(
            guard_after_phys, guard_after_seed, 8U)
    );

    ai_micro_mab_retry_byte4_exact_store_reset_env();

    TEST_END(
        "ai_micro_m_samepage_cross16b_store_refault_upperx4_retry_4b4b_store_then_pure_producer_tail_byte_store_then_upper_tail_second_byte_load_then_upper_half_load_corner");
}

bool ai_micro_m_samepage_cross16b_store_refault_upperx4_retry_4b4b_store_then_pure_producer_tail_byte_store_then_upper_tail_first_byte_load_then_upper_half_load_corner()
{
    TEST_START();

    static const uint8_t boundary_seed[16] = {
        0x69, 0x58, 0x47, 0x36, 0x25, 0x14, 0x03, 0xf2,
        0xe1, 0xd0, 0xcf, 0xbe, 0xad, 0x9c, 0x8b, 0x7a,
    };
    static const uint8_t guard_before_seed[8] = {
        0x01, 0x23, 0x45, 0x67, 0x89, 0xab, 0xcd, 0xef,
    };
    static const uint8_t guard_after_seed[8] = {
        0xfe, 0xdc, 0xba, 0x98, 0x76, 0x54, 0x32, 0x10,
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
    const uintptr_t upper_tail_first_byte_load_vaddr =
        aligned_upper_store_vaddr + 4U;
    const uintptr_t upper_half_load_vaddr = aligned_upper_store_vaddr;
    const uintptr_t deny_start = boundary_phys + 8U;
    const uintptr_t deny_end = deny_start + PAGE_SIZE;
    const uint64_t first_fault_store_value = 0x8798a9bacbdcedfeULL;
    const uint64_t refault_store_value = 0xebfc0d1e2f405162ULL;
    const uint64_t aligned_upper_store_value_first = 0x98a9bacbdcedfe0fULL;
    const uint64_t aligned_upper_store_value_second = 0xfc0d1e2f40516273ULL;
    const uint64_t aligned_upper_store_value_third = 0xa9bacbdcedfe0f10ULL;
    const uint64_t aligned_upper_store_value_fourth = 0x0d1e2f4051627384ULL;
    const uint64_t retry_4b4b_store_value = 0xbacbdcedfe0f1021ULL;
    const uint8_t followup_pure_producer_tail_byte_store_value = 0xb5U;
    uint8_t expected_boundary[16];
    uint64_t expected_upper_tail_first_byte_value = 0ULL;
    uint64_t observed_upper_tail_first_byte_value = 0ULL;
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
    TEST_ASSERT("for the no-H same-page cross-16B translated scalar store, the first repeated 4B+4B second-split write fault must still report store access fault on the true second-half start address before the retried-success then pure-producer-tail-byte store plus the upper-tail-first-byte and upper-half follow-up probes",
        excpt.triggered == true &&
        excpt.cause == CAUSE_SAF &&
        excpt.tval == second_half_vaddr
    );

    TEST_SETUP_EXCEPT();
    ai_micro_mab_retry_byte4_exact_store_translated_sd(
        fault_vaddr, first_fault_store_value);
    TEST_ASSERT("repeating the same no-H second-split 4B+4B write fault keeps the same precise metadata before the retried-success then pure-producer-tail-byte store plus the upper-tail-first-byte and upper-half follow-up probes",
        excpt.triggered == true &&
        excpt.cause == CAUSE_SAF &&
        excpt.tval == second_half_vaddr
    );

    ai_micro_mab_retry_byte4_exact_store_restore_pmp_full_access();

    TEST_SETUP_EXCEPT();
    ai_micro_mab_retry_byte4_exact_store_translated_sd(
        aligned_upper_store_vaddr, aligned_upper_store_value_first);
    TEST_ASSERT("after repair, the first same-boundary translated upper-half aligned store should complete cleanly before the refaulted retry-4B+4B-store then pure-producer-tail-byte store plus the upper-tail-first-byte and upper-half follow-up probes",
        excpt.triggered == false
    );

    TEST_SETUP_EXCEPT();
    ai_micro_mab_retry_byte4_exact_store_translated_sd(
        aligned_upper_store_vaddr, aligned_upper_store_value_second);
    TEST_ASSERT("after one upper-half aligned translated store has already completed on the repaired boundary, the second upper-half aligned translated store should stay trap-free before the refaulted retry-4B+4B-store then pure-producer-tail-byte store plus the upper-tail-first-byte and upper-half follow-up probes",
        excpt.triggered == false
    );

    TEST_SETUP_EXCEPT();
    ai_micro_mab_retry_byte4_exact_store_translated_sd(
        aligned_upper_store_vaddr, aligned_upper_store_value_third);
    TEST_ASSERT("after two upper-half aligned translated stores have already completed on the repaired boundary, the third upper-half aligned translated store should stay trap-free before the refaulted retry-4B+4B-store then pure-producer-tail-byte store plus the upper-tail-first-byte and upper-half follow-up probes",
        excpt.triggered == false
    );

    TEST_SETUP_EXCEPT();
    ai_micro_mab_retry_byte4_exact_store_translated_sd(
        aligned_upper_store_vaddr, aligned_upper_store_value_fourth);
    TEST_ASSERT("after three upper-half aligned translated stores have already completed on the repaired boundary, the fourth upper-half aligned translated store should stay trap-free and become the latest committed high-half payload before the refaulted retry-4B+4B-store then pure-producer-tail-byte store plus the upper-tail-first-byte and upper-half follow-up probes",
        excpt.triggered == false
    );

    ai_micro_mab_retry_byte4_exact_store_configure_pmp_window(
        deny_start, deny_end,
        AI_MICRO_MAB_RETRY_BYTE4_EXACT_STORE_PMP_TOR_NO_W);

    TEST_SETUP_EXCEPT();
    ai_micro_mab_retry_byte4_exact_store_translated_sd(
        fault_vaddr, refault_store_value);
    TEST_ASSERT("after four upper-half aligned translated stores have already completed on the repaired boundary, restoring the upper-half deny window and reissuing the original 4B+4B misaligned store should refresh the same precise second-split store access fault metadata before the pure-producer-tail-byte store plus the upper-tail-first-byte and upper-half follow-up probes",
        excpt.triggered == true &&
        excpt.cause == CAUSE_SAF &&
        excpt.tval == second_half_vaddr
    );

    ai_micro_mab_retry_byte4_exact_store_restore_pmp_full_access();

    TEST_SETUP_EXCEPT();
    ai_micro_mab_retry_byte4_exact_store_translated_sd(
        fault_vaddr, retry_4b4b_store_value);
    TEST_ASSERT("after the refault episode is repaired again, retrying the same 4B+4B translated store as a success store should clear stale exception metadata before the immediate pure-producer-tail-byte store and the upper-tail-first-byte plus upper-half follow-up probes",
        excpt.triggered == false
    );

    TEST_SETUP_EXCEPT();
    ai_micro_mab_retry_byte4_exact_store_translated_sb(
        followup_pure_producer_tail_byte_store_vaddr,
        followup_pure_producer_tail_byte_store_value);
    TEST_ASSERT("after the retried original-template 4B+4B store has already succeeded on the same boundary, the immediate translated pure-producer tail-byte store should also stay trap-free and must not let a retired fake-crosspage exact 4-byte high-half owner/template leak back into bytes8-11 or drag byte12 away from the latest aligned producer-side image while byte15 refreshes",
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
    expected_upper_tail_first_byte_value =
        (uint64_t)expected_boundary[12];
    expected_upper_half_load_value =
        ai_micro_mab_retry_byte4_exact_store_read_little_endian(
            expected_boundary + 8U, sizeof(uint64_t));

    TEST_SETUP_EXCEPT();
    observed_upper_tail_first_byte_value =
        ai_micro_mab_retry_byte4_exact_store_translated_lbu(
            upper_tail_first_byte_load_vaddr);
    TEST_ASSERT("after the pure-producer tail-byte store has already refreshed byte15 on top of the retried 4B+4B image, the immediate translated upper-tail first-byte probe at boundary+12 should clear stale exception metadata and read the latest retained producer-owned byte12 before the following aligned upper-half widening probe",
        excpt.triggered == false &&
        observed_upper_tail_first_byte_value ==
            expected_upper_tail_first_byte_value
    );

    TEST_SETUP_EXCEPT();
    observed_upper_half_load_value =
        ai_micro_mab_retry_byte4_exact_store_translated_ld(
            upper_half_load_vaddr);
    TEST_ASSERT("after the upper-tail first-byte probe has already shown the latest retained byte12 on top of the retried 4B+4B image, the following aligned upper-half translated load at boundary+8 should still clear stale exception metadata and read the merged latest image of retry bytes8-11, retained producer bytes12-14, and the new tail byte instead of falling back to stale exact-BYTE4 state",
        excpt.triggered == false &&
        observed_upper_half_load_value == expected_upper_half_load_value
    );

    TEST_ASSERT("after the prolonged upper-half producer, repaired refault, retried 4B+4B success store, immediate pure-producer tail-byte store, the upper-tail first-byte probe, and the upper-half load probe, the final boundary bytes should preserve bytes4-11 from the retried image, keep bytes12-14 from the latest aligned upper-half image, and refresh only byte15",
        ai_micro_mab_retry_byte4_exact_store_bytes_match(
            boundary_phys, expected_boundary, 16U)
    );

    TEST_ASSERT("guard regions around the same-page boundary should remain unchanged through the repeated 4B+4B fault, four upper-half aligned stores, refault, retried success store, immediate pure-producer-tail-byte follow-up store, the upper-tail first-byte probe, and the upper-half load probe sequence",
        ai_micro_mab_retry_byte4_exact_store_bytes_match(
            guard_before_phys, guard_before_seed, 8U) &&
        ai_micro_mab_retry_byte4_exact_store_bytes_match(
            guard_after_phys, guard_after_seed, 8U)
    );

    ai_micro_mab_retry_byte4_exact_store_reset_env();

    TEST_END(
        "ai_micro_m_samepage_cross16b_store_refault_upperx4_retry_4b4b_store_then_pure_producer_tail_byte_store_then_upper_tail_first_byte_load_then_upper_half_load_corner");
}

bool ai_micro_m_samepage_cross16b_store_refault_upperx4_retry_4b4b_store_then_pure_producer_tail_byte_store_then_upper_half_load_then_upper_tail_first_byte_load_corner()
{
    TEST_START();

    static const uint8_t boundary_seed[16] = {
        0x7a, 0x69, 0x58, 0x47, 0x36, 0x25, 0x14, 0x03,
        0xf2, 0xe1, 0xd0, 0xcf, 0xbe, 0xad, 0x9c, 0x8b,
    };
    static const uint8_t guard_before_seed[8] = {
        0x10, 0x21, 0x32, 0x43, 0x54, 0x65, 0x76, 0x87,
    };
    static const uint8_t guard_after_seed[8] = {
        0x89, 0x9a, 0xab, 0xbc, 0xcd, 0xde, 0xef, 0xf1,
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
    const uintptr_t upper_tail_first_byte_load_vaddr =
        aligned_upper_store_vaddr + 4U;
    const uintptr_t deny_start = boundary_phys + 8U;
    const uintptr_t deny_end = deny_start + PAGE_SIZE;
    const uint64_t first_fault_store_value = 0x98a9bacbdcedfe0fULL;
    const uint64_t refault_store_value = 0x0a1b2c3d4e5f6071ULL;
    const uint64_t aligned_upper_store_value_first = 0xa9bacbdcedfe0f10ULL;
    const uint64_t aligned_upper_store_value_second = 0x1b2c3d4e5f607182ULL;
    const uint64_t aligned_upper_store_value_third = 0xbacbdcedfe0f1021ULL;
    const uint64_t aligned_upper_store_value_fourth = 0x2c3d4e5f60718293ULL;
    const uint64_t retry_4b4b_store_value = 0xcbdcedfe0f102132ULL;
    const uint8_t followup_pure_producer_tail_byte_store_value = 0xc6U;
    uint8_t expected_boundary[16];
    uint64_t expected_upper_half_load_value = 0ULL;
    uint64_t observed_upper_half_load_value = 0ULL;
    uint64_t expected_upper_tail_first_byte_value = 0ULL;
    uint64_t observed_upper_tail_first_byte_value = 0ULL;

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
    TEST_ASSERT("for the no-H same-page cross-16B translated scalar store, the first repeated 4B+4B second-split write fault must still report store access fault on the true second-half start address before the retried-success then pure-producer-tail-byte store plus the upper-half and upper-tail-first-byte follow-up probes",
        excpt.triggered == true &&
        excpt.cause == CAUSE_SAF &&
        excpt.tval == second_half_vaddr
    );

    TEST_SETUP_EXCEPT();
    ai_micro_mab_retry_byte4_exact_store_translated_sd(
        fault_vaddr, first_fault_store_value);
    TEST_ASSERT("repeating the same no-H second-split 4B+4B write fault keeps the same precise metadata before the retried-success then pure-producer-tail-byte store plus the upper-half and upper-tail-first-byte follow-up probes",
        excpt.triggered == true &&
        excpt.cause == CAUSE_SAF &&
        excpt.tval == second_half_vaddr
    );

    ai_micro_mab_retry_byte4_exact_store_restore_pmp_full_access();

    TEST_SETUP_EXCEPT();
    ai_micro_mab_retry_byte4_exact_store_translated_sd(
        aligned_upper_store_vaddr, aligned_upper_store_value_first);
    TEST_ASSERT("after repair, the first same-boundary translated upper-half aligned store should complete cleanly before the refaulted retry-4B+4B-store then pure-producer-tail-byte store plus the upper-half and upper-tail-first-byte follow-up probes",
        excpt.triggered == false
    );

    TEST_SETUP_EXCEPT();
    ai_micro_mab_retry_byte4_exact_store_translated_sd(
        aligned_upper_store_vaddr, aligned_upper_store_value_second);
    TEST_ASSERT("after one upper-half aligned translated store has already completed on the repaired boundary, the second upper-half aligned translated store should stay trap-free before the refaulted retry-4B+4B-store then pure-producer-tail-byte store plus the upper-half and upper-tail-first-byte follow-up probes",
        excpt.triggered == false
    );

    TEST_SETUP_EXCEPT();
    ai_micro_mab_retry_byte4_exact_store_translated_sd(
        aligned_upper_store_vaddr, aligned_upper_store_value_third);
    TEST_ASSERT("after two upper-half aligned translated stores have already completed on the repaired boundary, the third upper-half aligned translated store should stay trap-free before the refaulted retry-4B+4B-store then pure-producer-tail-byte store plus the upper-half and upper-tail-first-byte follow-up probes",
        excpt.triggered == false
    );

    TEST_SETUP_EXCEPT();
    ai_micro_mab_retry_byte4_exact_store_translated_sd(
        aligned_upper_store_vaddr, aligned_upper_store_value_fourth);
    TEST_ASSERT("after three upper-half aligned translated stores have already completed on the repaired boundary, the fourth upper-half aligned translated store should stay trap-free and become the latest committed high-half payload before the refaulted retry-4B+4B-store then pure-producer-tail-byte store plus the upper-half and upper-tail-first-byte follow-up probes",
        excpt.triggered == false
    );

    ai_micro_mab_retry_byte4_exact_store_configure_pmp_window(
        deny_start, deny_end,
        AI_MICRO_MAB_RETRY_BYTE4_EXACT_STORE_PMP_TOR_NO_W);

    TEST_SETUP_EXCEPT();
    ai_micro_mab_retry_byte4_exact_store_translated_sd(
        fault_vaddr, refault_store_value);
    TEST_ASSERT("after four upper-half aligned translated stores have already completed on the repaired boundary, restoring the upper-half deny window and reissuing the original 4B+4B misaligned store should refresh the same precise second-split store access fault metadata before the pure-producer-tail-byte store plus the upper-half and upper-tail-first-byte follow-up probes",
        excpt.triggered == true &&
        excpt.cause == CAUSE_SAF &&
        excpt.tval == second_half_vaddr
    );

    ai_micro_mab_retry_byte4_exact_store_restore_pmp_full_access();

    TEST_SETUP_EXCEPT();
    ai_micro_mab_retry_byte4_exact_store_translated_sd(
        fault_vaddr, retry_4b4b_store_value);
    TEST_ASSERT("after the refault episode is repaired again, retrying the same 4B+4B translated store as a success store should clear stale exception metadata before the immediate pure-producer-tail-byte store and the upper-half plus upper-tail-first-byte follow-up probes",
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
    expected_upper_tail_first_byte_value =
        (uint64_t)expected_boundary[12];

    TEST_SETUP_EXCEPT();
    observed_upper_half_load_value =
        ai_micro_mab_retry_byte4_exact_store_translated_ld(
            upper_half_load_vaddr);
    TEST_ASSERT("after the pure-producer tail-byte store has already refreshed byte15 on top of the retried 4B+4B image, the immediate aligned upper-half translated load at boundary+8 should clear stale exception metadata and read the merged latest image of retry bytes8-11, retained producer bytes12-14, and the new tail byte before the following upper-tail-first-byte probe",
        excpt.triggered == false &&
        observed_upper_half_load_value == expected_upper_half_load_value
    );

    TEST_SETUP_EXCEPT();
    observed_upper_tail_first_byte_value =
        ai_micro_mab_retry_byte4_exact_store_translated_lbu(
            upper_tail_first_byte_load_vaddr);
    TEST_ASSERT("after the aligned upper-half load probe has already shown the merged latest image on top of the retried 4B+4B store, the following translated upper-tail first-byte load at boundary+12 should still clear stale exception metadata and keep the latest retained producer-owned byte12",
        excpt.triggered == false &&
        observed_upper_tail_first_byte_value ==
            expected_upper_tail_first_byte_value
    );

    TEST_ASSERT("after the prolonged upper-half producer, repaired refault, retried 4B+4B success store, immediate pure-producer tail-byte store, the upper-half load probe, and the upper-tail first-byte probe, the final boundary bytes should preserve bytes4-11 from the retried image, keep bytes12-14 from the latest aligned upper-half image, and refresh only byte15",
        ai_micro_mab_retry_byte4_exact_store_bytes_match(
            boundary_phys, expected_boundary, 16U)
    );

    TEST_ASSERT("guard regions around the same-page boundary should remain unchanged through the repeated 4B+4B fault, four upper-half aligned stores, refault, retried success store, immediate pure-producer-tail-byte follow-up store, the aligned upper-half load probe, and the upper-tail first-byte probe sequence",
        ai_micro_mab_retry_byte4_exact_store_bytes_match(
            guard_before_phys, guard_before_seed, 8U) &&
        ai_micro_mab_retry_byte4_exact_store_bytes_match(
            guard_after_phys, guard_after_seed, 8U)
    );

    ai_micro_mab_retry_byte4_exact_store_reset_env();

    TEST_END(
        "ai_micro_m_samepage_cross16b_store_refault_upperx4_retry_4b4b_store_then_pure_producer_tail_byte_store_then_upper_half_load_then_upper_tail_first_byte_load_corner");
}

bool ai_micro_m_samepage_cross16b_store_refault_upperx4_retry_4b4b_store_then_pure_producer_tail_byte_store_then_upper_half_load_then_upper_tail_second_byte_load_corner()
{
    TEST_START();

    static const uint8_t boundary_seed[16] = {
        0x8b, 0x7a, 0x69, 0x58, 0x47, 0x36, 0x25, 0x14,
        0x03, 0xf2, 0xe1, 0xd0, 0xcf, 0xbe, 0xad, 0x9c,
    };
    static const uint8_t guard_before_seed[8] = {
        0x13, 0x24, 0x35, 0x46, 0x57, 0x68, 0x79, 0x8a,
    };
    static const uint8_t guard_after_seed[8] = {
        0x9b, 0xac, 0xbd, 0xce, 0xdf, 0xe0, 0xf1, 0x02,
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
    const uintptr_t upper_tail_second_byte_load_vaddr =
        aligned_upper_store_vaddr + 5U;
    const uintptr_t deny_start = boundary_phys + 8U;
    const uintptr_t deny_end = deny_start + PAGE_SIZE;
    const uint64_t first_fault_store_value = 0xa9bacbdcedfe0f10ULL;
    const uint64_t refault_store_value = 0x1b2c3d4e5f607182ULL;
    const uint64_t aligned_upper_store_value_first = 0xbacbdcedfe0f1021ULL;
    const uint64_t aligned_upper_store_value_second = 0x2c3d4e5f60718293ULL;
    const uint64_t aligned_upper_store_value_third = 0xcbdcedfe0f102132ULL;
    const uint64_t aligned_upper_store_value_fourth = 0x3d4e5f60718293a4ULL;
    const uint64_t retry_4b4b_store_value = 0xdcedfe0f10213243ULL;
    const uint8_t followup_pure_producer_tail_byte_store_value = 0xd7U;
    uint8_t expected_boundary[16];
    uint64_t expected_upper_half_load_value = 0ULL;
    uint64_t observed_upper_half_load_value = 0ULL;
    uint64_t expected_upper_tail_second_byte_value = 0ULL;
    uint64_t observed_upper_tail_second_byte_value = 0ULL;

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
    TEST_ASSERT("for the no-H same-page cross-16B translated scalar store, the first repeated 4B+4B second-split write fault must still report store access fault on the true second-half start address before the retried-success then pure-producer-tail-byte store plus the upper-half and upper-tail-second-byte follow-up probes",
        excpt.triggered == true &&
        excpt.cause == CAUSE_SAF &&
        excpt.tval == second_half_vaddr
    );

    TEST_SETUP_EXCEPT();
    ai_micro_mab_retry_byte4_exact_store_translated_sd(
        fault_vaddr, first_fault_store_value);
    TEST_ASSERT("repeating the same no-H second-split 4B+4B write fault keeps the same precise metadata before the retried-success then pure-producer-tail-byte store plus the upper-half and upper-tail-second-byte follow-up probes",
        excpt.triggered == true &&
        excpt.cause == CAUSE_SAF &&
        excpt.tval == second_half_vaddr
    );

    ai_micro_mab_retry_byte4_exact_store_restore_pmp_full_access();

    TEST_SETUP_EXCEPT();
    ai_micro_mab_retry_byte4_exact_store_translated_sd(
        aligned_upper_store_vaddr, aligned_upper_store_value_first);
    TEST_ASSERT("after repair, the first same-boundary translated upper-half aligned store should complete cleanly before the refaulted retry-4B+4B-store then pure-producer-tail-byte store plus the upper-half and upper-tail-second-byte follow-up probes",
        excpt.triggered == false
    );

    TEST_SETUP_EXCEPT();
    ai_micro_mab_retry_byte4_exact_store_translated_sd(
        aligned_upper_store_vaddr, aligned_upper_store_value_second);
    TEST_ASSERT("after one upper-half aligned translated store has already completed on the repaired boundary, the second upper-half aligned translated store should stay trap-free before the refaulted retry-4B+4B-store then pure-producer-tail-byte store plus the upper-half and upper-tail-second-byte follow-up probes",
        excpt.triggered == false
    );

    TEST_SETUP_EXCEPT();
    ai_micro_mab_retry_byte4_exact_store_translated_sd(
        aligned_upper_store_vaddr, aligned_upper_store_value_third);
    TEST_ASSERT("after two upper-half aligned translated stores have already completed on the repaired boundary, the third upper-half aligned translated store should stay trap-free before the refaulted retry-4B+4B-store then pure-producer-tail-byte store plus the upper-half and upper-tail-second-byte follow-up probes",
        excpt.triggered == false
    );

    TEST_SETUP_EXCEPT();
    ai_micro_mab_retry_byte4_exact_store_translated_sd(
        aligned_upper_store_vaddr, aligned_upper_store_value_fourth);
    TEST_ASSERT("after three upper-half aligned translated stores have already completed on the repaired boundary, the fourth upper-half aligned translated store should stay trap-free and become the latest committed high-half payload before the refaulted retry-4B+4B-store then pure-producer-tail-byte store plus the upper-half and upper-tail-second-byte follow-up probes",
        excpt.triggered == false
    );

    ai_micro_mab_retry_byte4_exact_store_configure_pmp_window(
        deny_start, deny_end,
        AI_MICRO_MAB_RETRY_BYTE4_EXACT_STORE_PMP_TOR_NO_W);

    TEST_SETUP_EXCEPT();
    ai_micro_mab_retry_byte4_exact_store_translated_sd(
        fault_vaddr, refault_store_value);
    TEST_ASSERT("after four upper-half aligned translated stores have already completed on the repaired boundary, restoring the upper-half deny window and reissuing the original 4B+4B misaligned store should refresh the same precise second-split store access fault metadata before the pure-producer-tail-byte store plus the upper-half and upper-tail-second-byte follow-up probes",
        excpt.triggered == true &&
        excpt.cause == CAUSE_SAF &&
        excpt.tval == second_half_vaddr
    );

    ai_micro_mab_retry_byte4_exact_store_restore_pmp_full_access();

    TEST_SETUP_EXCEPT();
    ai_micro_mab_retry_byte4_exact_store_translated_sd(
        fault_vaddr, retry_4b4b_store_value);
    TEST_ASSERT("after the refault episode is repaired again, retrying the same 4B+4B translated store as a success store should clear stale exception metadata before the immediate pure-producer-tail-byte store and the upper-half plus upper-tail-second-byte follow-up probes",
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
    expected_upper_tail_second_byte_value =
        (uint64_t)expected_boundary[13];

    TEST_SETUP_EXCEPT();
    observed_upper_half_load_value =
        ai_micro_mab_retry_byte4_exact_store_translated_ld(
            upper_half_load_vaddr);
    TEST_ASSERT("after the pure-producer tail-byte store has already refreshed byte15 on top of the retried 4B+4B image, the immediate aligned upper-half translated load at boundary+8 should clear stale exception metadata and read the merged latest image of retry bytes8-11, retained producer bytes12-14, and the new tail byte before the following upper-tail-second-byte probe",
        excpt.triggered == false &&
        observed_upper_half_load_value == expected_upper_half_load_value
    );

    TEST_SETUP_EXCEPT();
    observed_upper_tail_second_byte_value =
        ai_micro_mab_retry_byte4_exact_store_translated_lbu(
            upper_tail_second_byte_load_vaddr);
    TEST_ASSERT("after the aligned upper-half load probe has already shown the merged latest image on top of the retried 4B+4B store, the following translated upper-tail second-byte load at boundary+13 should still clear stale exception metadata and keep the latest retained producer-owned byte13",
        excpt.triggered == false &&
        observed_upper_tail_second_byte_value ==
            expected_upper_tail_second_byte_value
    );

    TEST_ASSERT("after the prolonged upper-half producer, repaired refault, retried 4B+4B success store, immediate pure-producer tail-byte store, the upper-half load probe, and the upper-tail second-byte probe, the final boundary bytes should preserve bytes4-11 from the retried image, keep bytes12-14 from the latest aligned upper-half image, and refresh only byte15",
        ai_micro_mab_retry_byte4_exact_store_bytes_match(
            boundary_phys, expected_boundary, 16U)
    );

    TEST_ASSERT("guard regions around the same-page boundary should remain unchanged through the repeated 4B+4B fault, four upper-half aligned stores, refault, retried success store, immediate pure-producer-tail-byte follow-up store, the aligned upper-half load probe, and the upper-tail second-byte probe sequence",
        ai_micro_mab_retry_byte4_exact_store_bytes_match(
            guard_before_phys, guard_before_seed, 8U) &&
        ai_micro_mab_retry_byte4_exact_store_bytes_match(
            guard_after_phys, guard_after_seed, 8U)
    );

    ai_micro_mab_retry_byte4_exact_store_reset_env();

    TEST_END(
        "ai_micro_m_samepage_cross16b_store_refault_upperx4_retry_4b4b_store_then_pure_producer_tail_byte_store_then_upper_half_load_then_upper_tail_second_byte_load_corner");
}
