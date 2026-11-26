#include <page_tables.h>
#include <rvh_test.h>
#include <stdint.h>

/* Keep the BYTE5 internal follow-up store separate from the BYTE5 tail-edge
 * cases so the internal semantic-window suspicion can evolve independently.
 */

#define AI_MICRO_MAB_RETRY_BYTE5_INTERNAL_STORE_PMP_TOR_RWX 0x0fULL
#define AI_MICRO_MAB_RETRY_BYTE5_INTERNAL_STORE_PMP_TOR_NO_W 0x09ULL

#define AI_MICRO_MAB_RETRY_BYTE5_INTERNAL_STORE_BOUNDARY_BASE_OFFSET 8U
#define AI_MICRO_MAB_RETRY_BYTE5_INTERNAL_STORE_CROSS_16B_SD_OFFSET 13U

static inline pte_t *ai_micro_mab_retry_byte5_internal_store_hs_leaf_entry(
    enum test_page page)
{
#ifdef sv48
    return &hspt[4][page];
#else
    return &hspt[2][page];
#endif
}

static inline void ai_micro_mab_retry_byte5_internal_store_set_leaf_paddr(
    enum test_page page, uintptr_t paddr, uint64_t flags)
{
    *ai_micro_mab_retry_byte5_internal_store_hs_leaf_entry(page) =
        (paddr >> 2) | flags;
}

static inline void ai_micro_mab_retry_byte5_internal_store_set_leaf(
    enum test_page page, uint64_t flags)
{
    ai_micro_mab_retry_byte5_internal_store_set_leaf_paddr(
        page, phys_page_base(page), flags);
}

static inline void
ai_micro_mab_retry_byte5_internal_store_prepare_supervisor_translation(void)
{
    goto_priv(PRIV_M);
    hspt_init();
    sfence_vma();
}

static inline void
ai_micro_mab_retry_byte5_internal_store_restore_pmp_full_access(void)
{
    goto_priv(PRIV_M);
    CSRW(CSR_PMPCFG0, 0ULL);
    CSRW(CSR_PMPADDR0, (uint64_t)-1);
    CSRW(CSR_PMPADDR1, 0ULL);
    CSRW(CSR_PMPADDR2, 0ULL);
    CSRW(CSR_PMPCFG0, AI_MICRO_MAB_RETRY_BYTE5_INTERNAL_STORE_PMP_TOR_RWX);
}

static inline void
ai_micro_mab_retry_byte5_internal_store_clear_machine_access_mode(void)
{
    goto_priv(PRIV_M);
    CSRC(CSR_MSTATUS, MSTATUS_MPRV);
    CSRC(CSR_MSTATUS, 3ULL << 11);
    CSRW(CSR_SATP, 0ULL);
}

static inline void ai_micro_mab_retry_byte5_internal_store_configure_pmp_window(
    uintptr_t deny_start, uintptr_t deny_end, uint64_t deny_cfg)
{
    goto_priv(PRIV_M);
    CSRW(CSR_PMPCFG0, 0ULL);
    CSRW(CSR_PMPADDR0, deny_start >> 2);
    CSRW(CSR_PMPADDR1, deny_end >> 2);
    CSRW(CSR_PMPADDR2, (uint64_t)-1);
    CSRW(CSR_PMPCFG0, AI_MICRO_MAB_RETRY_BYTE5_INTERNAL_STORE_PMP_TOR_RWX |
        (deny_cfg << 8) |
        (AI_MICRO_MAB_RETRY_BYTE5_INTERNAL_STORE_PMP_TOR_RWX << 16));
}

static inline void ai_micro_mab_retry_byte5_internal_store_reset_env(void)
{
    ai_micro_mab_retry_byte5_internal_store_restore_pmp_full_access();
    ai_micro_mab_retry_byte5_internal_store_clear_machine_access_mode();
}

static inline void ai_micro_mab_retry_byte5_internal_store_write_bytes(
    uintptr_t addr, const uint8_t *bytes, unsigned len)
{
    for (unsigned i = 0; i < len; ++i) {
        write8(addr + i, bytes[i]);
    }
}

static inline bool ai_micro_mab_retry_byte5_internal_store_bytes_match(
    uintptr_t addr, const uint8_t *bytes, unsigned len)
{
    for (unsigned i = 0; i < len; ++i) {
        if (read8(addr + i) != bytes[i]) {
            return false;
        }
    }
    return true;
}

static inline void
ai_micro_mab_retry_byte5_internal_store_overlay_little_endian_bytes(
    uint8_t *dst, const uint8_t *seed, unsigned len, unsigned start_byte,
    uint64_t value, unsigned size)
{
    for (unsigned i = 0; i < len; ++i) {
        dst[i] = seed[i];
    }
    for (unsigned i = 0; i < size; ++i) {
        dst[start_byte + i] = (uint8_t)((value >> (8U * i)) & 0xffU);
    }
}

#define AI_MICRO_MAB_RETRY_BYTE5_INTERNAL_STORE_TRANSLATED_STORE_INSTRUCTION( \
    name, inst, type) \
static inline void name(uintptr_t addr, type value) \
{ \
    const uint64_t clear_mpp = 3ULL << 11; \
    const uint64_t set_mprv = MSTATUS_MPRV; \
    const uint64_t set_mpp_s = MSTATUS_MPP; \
    asm volatile( \
        "csrc mstatus, %0\n\t" \
        "csrs mstatus, %1\n\t" \
        "csrs mstatus, %2\n\t" \
        inst " %3, 0(%4)\n\t" \
        "fence iorw, iorw\n\t" \
        "csrc mstatus, %1\n\t" \
        "csrc mstatus, %0\n\t" \
        : \
        : "r"(clear_mpp), "r"(set_mprv), "r"(set_mpp_s), "r"(value), "r"(addr) \
        : "memory"); \
}

AI_MICRO_MAB_RETRY_BYTE5_INTERNAL_STORE_TRANSLATED_STORE_INSTRUCTION(
    ai_micro_mab_retry_byte5_internal_store_translated_sb, "sb", uint8_t)
AI_MICRO_MAB_RETRY_BYTE5_INTERNAL_STORE_TRANSLATED_STORE_INSTRUCTION(
    ai_micro_mab_retry_byte5_internal_store_translated_sd, "sd", uint64_t)
AI_MICRO_MAB_RETRY_BYTE5_INTERNAL_STORE_TRANSLATED_STORE_INSTRUCTION(
    ai_micro_mab_retry_byte5_internal_store_translated_sh, "sh", uint16_t)
AI_MICRO_MAB_RETRY_BYTE5_INTERNAL_STORE_TRANSLATED_STORE_INSTRUCTION(
    ai_micro_mab_retry_byte5_internal_store_translated_sw, "sw", uint32_t)

bool ai_micro_m_samepage_cross16b_store_refault_upperx4_retry_3b5b_store_then_center_semantic_halfword_store_corner()
{
    TEST_START();

    static const uint8_t boundary_seed[16] = {
        0x82, 0x71, 0x60, 0x5f, 0x4e, 0x3d, 0x2c, 0x1b,
        0x0a, 0xf9, 0xe8, 0xd7, 0xc6, 0xb5, 0xa4, 0x93,
    };
    static const uint8_t guard_before_seed[8] = {
        0x14, 0x26, 0x38, 0x4a, 0x5c, 0x6e, 0x70, 0x82,
    };
    static const uint8_t guard_after_seed[8] = {
        0xc7, 0xd9, 0xeb, 0xfd, 0x0f, 0x11, 0x23, 0x35,
    };
    const enum test_page low_page = SWITCH1;
    const enum test_page high_page = SWITCH2;
    const uintptr_t page_phys = phys_page_base(low_page) + PAGE_SIZE - 16U;
    const uintptr_t page_vaddr = hs_page_base(low_page) + PAGE_SIZE - 16U;
    const uintptr_t boundary_phys = page_phys +
        AI_MICRO_MAB_RETRY_BYTE5_INTERNAL_STORE_BOUNDARY_BASE_OFFSET;
    const uintptr_t guard_before_phys = page_phys;
    const uintptr_t guard_after_phys = page_phys + 24U;
    const uintptr_t fault_vaddr = page_vaddr +
        AI_MICRO_MAB_RETRY_BYTE5_INTERNAL_STORE_CROSS_16B_SD_OFFSET;
    const uintptr_t second_half_vaddr = page_vaddr + 16U;
    const uintptr_t aligned_upper_store_vaddr = page_vaddr +
        AI_MICRO_MAB_RETRY_BYTE5_INTERNAL_STORE_BOUNDARY_BASE_OFFSET + 8U;
    const uintptr_t followup_center_semantic_halfword_store_vaddr =
        aligned_upper_store_vaddr + 2U;
    const uintptr_t deny_start = boundary_phys + 8U;
    const uintptr_t deny_end = deny_start + PAGE_SIZE;
    const uint64_t first_fault_store_value = 0x415263748596a7b8ULL;
    const uint64_t refault_store_value = 0xc9daebfc0d1e2f30ULL;
    const uint64_t aligned_upper_store_value_first = 0x31425364758697a8ULL;
    const uint64_t aligned_upper_store_value_second = 0xb9cadbecfd0e1f20ULL;
    const uint64_t aligned_upper_store_value_third = 0x425364758697a8b9ULL;
    const uint64_t aligned_upper_store_value_fourth = 0xcadbecfd0e1f2031ULL;
    const uint64_t retry_3b5b_store_value = 0x5364758697a8b9caULL;
    const uint16_t followup_center_semantic_halfword_store_value = 0x7e6dU;
    uint8_t expected_boundary[16];

    goto_priv(PRIV_M);
    ai_micro_mab_retry_byte5_internal_store_reset_env();
    ai_micro_mab_retry_byte5_internal_store_write_bytes(
        boundary_phys, boundary_seed, 16U);
    ai_micro_mab_retry_byte5_internal_store_write_bytes(
        guard_before_phys, guard_before_seed, 8U);
    ai_micro_mab_retry_byte5_internal_store_write_bytes(
        guard_after_phys, guard_after_seed, 8U);

    ai_micro_mab_retry_byte5_internal_store_configure_pmp_window(
        deny_start, deny_end,
        AI_MICRO_MAB_RETRY_BYTE5_INTERNAL_STORE_PMP_TOR_NO_W);
    ai_micro_mab_retry_byte5_internal_store_prepare_supervisor_translation();
    ai_micro_mab_retry_byte5_internal_store_set_leaf(
        low_page, PTE_V | PTE_AD | PTE_RWX);
    ai_micro_mab_retry_byte5_internal_store_set_leaf(
        high_page, PTE_V | PTE_AD | PTE_RWX);
    sfence_vma();

    TEST_SETUP_EXCEPT();
    ai_micro_mab_retry_byte5_internal_store_translated_sd(
        fault_vaddr, first_fault_store_value);
    TEST_ASSERT("for the no-H same-page cross-16B translated scalar store, the first repeated 3B+5B second-split write fault must still report store access fault on the true second-half start address before the retried-success then center-semantic-halfword follow-up store probe",
        excpt.triggered == true &&
        excpt.cause == CAUSE_SAF &&
        excpt.tval == second_half_vaddr
    );

    TEST_SETUP_EXCEPT();
    ai_micro_mab_retry_byte5_internal_store_translated_sd(
        fault_vaddr, first_fault_store_value);
    TEST_ASSERT("repeating the same no-H second-split 3B+5B write fault keeps the same precise metadata before the retried-success then center-semantic-halfword follow-up store probe",
        excpt.triggered == true &&
        excpt.cause == CAUSE_SAF &&
        excpt.tval == second_half_vaddr
    );

    ai_micro_mab_retry_byte5_internal_store_restore_pmp_full_access();

    TEST_SETUP_EXCEPT();
    ai_micro_mab_retry_byte5_internal_store_translated_sd(
        aligned_upper_store_vaddr, aligned_upper_store_value_first);
    TEST_ASSERT("after repair, the first same-boundary translated upper-half aligned store should complete cleanly before the refaulted retry-3B+5B-store then center-semantic-halfword follow-up store probe",
        excpt.triggered == false
    );

    TEST_SETUP_EXCEPT();
    ai_micro_mab_retry_byte5_internal_store_translated_sd(
        aligned_upper_store_vaddr, aligned_upper_store_value_second);
    TEST_ASSERT("after one upper-half aligned translated store has already completed on the repaired boundary, the second upper-half aligned translated store should stay trap-free before the refaulted retry-3B+5B-store then center-semantic-halfword follow-up store probe",
        excpt.triggered == false
    );

    TEST_SETUP_EXCEPT();
    ai_micro_mab_retry_byte5_internal_store_translated_sd(
        aligned_upper_store_vaddr, aligned_upper_store_value_third);
    TEST_ASSERT("after two upper-half aligned translated stores have already completed on the repaired boundary, the third upper-half aligned translated store should stay trap-free before the refaulted retry-3B+5B-store then center-semantic-halfword follow-up store probe",
        excpt.triggered == false
    );

    TEST_SETUP_EXCEPT();
    ai_micro_mab_retry_byte5_internal_store_translated_sd(
        aligned_upper_store_vaddr, aligned_upper_store_value_fourth);
    TEST_ASSERT("after three upper-half aligned translated stores have already completed on the repaired boundary, the fourth upper-half aligned translated store should stay trap-free and become the latest committed high-half payload before the refaulted retry-3B+5B-store then center-semantic-halfword follow-up store probe",
        excpt.triggered == false
    );

    ai_micro_mab_retry_byte5_internal_store_configure_pmp_window(
        deny_start, deny_end,
        AI_MICRO_MAB_RETRY_BYTE5_INTERNAL_STORE_PMP_TOR_NO_W);

    TEST_SETUP_EXCEPT();
    ai_micro_mab_retry_byte5_internal_store_translated_sd(
        fault_vaddr, refault_store_value);
    TEST_ASSERT("after four upper-half aligned translated stores have already completed on the repaired boundary, restoring the upper-half deny window and reissuing the original 3B+5B misaligned store should refresh the same precise second-split store access fault metadata before the center-semantic-halfword follow-up store probe",
        excpt.triggered == true &&
        excpt.cause == CAUSE_SAF &&
        excpt.tval == second_half_vaddr
    );

    ai_micro_mab_retry_byte5_internal_store_restore_pmp_full_access();

    TEST_SETUP_EXCEPT();
    ai_micro_mab_retry_byte5_internal_store_translated_sd(
        fault_vaddr, retry_3b5b_store_value);
    TEST_ASSERT("after the refault episode is repaired again, retrying the same 3B+5B translated store as a success store should clear stale exception metadata before the immediate center-semantic-halfword follow-up store",
        excpt.triggered == false
    );

    TEST_SETUP_EXCEPT();
    ai_micro_mab_retry_byte5_internal_store_translated_sh(
        followup_center_semantic_halfword_store_vaddr,
        followup_center_semantic_halfword_store_value);
    TEST_ASSERT("after the retried original-template 3B+5B store has already succeeded on the same boundary, the immediate translated center-semantic-halfword store should also stay trap-free and must not revive the stale fake-crosspage 8-byte high-half window beyond the semantic BYTE5 range",
        excpt.triggered == false
    );

    ai_micro_mab_retry_byte5_internal_store_overlay_little_endian_bytes(
        expected_boundary, boundary_seed, 16U, 8U,
        aligned_upper_store_value_fourth, sizeof(uint64_t));
    ai_micro_mab_retry_byte5_internal_store_overlay_little_endian_bytes(
        expected_boundary, expected_boundary, 16U, 5U,
        retry_3b5b_store_value, sizeof(uint64_t));
    ai_micro_mab_retry_byte5_internal_store_overlay_little_endian_bytes(
        expected_boundary, expected_boundary, 16U, 10U,
        (uint64_t)followup_center_semantic_halfword_store_value,
        sizeof(uint16_t));

    TEST_ASSERT("after the prolonged upper-half producer, repaired refault, retried 3B+5B success store, and immediate center-semantic-halfword store, the final boundary bytes should preserve bytes5-9 from the retried image, refresh only bytes10-11, preserve byte12 from the retried image, and keep bytes13-15 from the latest aligned upper-half image",
        ai_micro_mab_retry_byte5_internal_store_bytes_match(
            boundary_phys, expected_boundary, 16U)
    );

    TEST_ASSERT("guard regions around the same-page boundary should remain unchanged through the repeated 3B+5B fault, four upper-half aligned stores, refault, retried success store, and immediate center-semantic-halfword follow-up store sequence",
        ai_micro_mab_retry_byte5_internal_store_bytes_match(
            guard_before_phys, guard_before_seed, 8U) &&
        ai_micro_mab_retry_byte5_internal_store_bytes_match(
            guard_after_phys, guard_after_seed, 8U)
    );

    ai_micro_mab_retry_byte5_internal_store_reset_env();

    TEST_END(
        "ai_micro_m_samepage_cross16b_store_refault_upperx4_retry_3b5b_store_then_center_semantic_halfword_store_corner");
}

bool ai_micro_m_samepage_cross16b_store_refault_upperx4_retry_3b5b_store_then_head_side_semantic_word_store_corner()
{
    TEST_START();

    static const uint8_t boundary_seed[16] = {
        0x73, 0x62, 0x51, 0x40, 0x3f, 0x2e, 0x1d, 0x0c,
        0xfb, 0xea, 0xd9, 0xc8, 0xb7, 0xa6, 0x95, 0x84,
    };
    static const uint8_t guard_before_seed[8] = {
        0x18, 0x2a, 0x3c, 0x4e, 0x50, 0x62, 0x74, 0x86,
    };
    static const uint8_t guard_after_seed[8] = {
        0xcd, 0xdf, 0xe1, 0xf3, 0x05, 0x17, 0x29, 0x3b,
    };
    const enum test_page low_page = SWITCH1;
    const enum test_page high_page = SWITCH2;
    const uintptr_t page_phys = phys_page_base(low_page) + PAGE_SIZE - 16U;
    const uintptr_t page_vaddr = hs_page_base(low_page) + PAGE_SIZE - 16U;
    const uintptr_t boundary_phys = page_phys +
        AI_MICRO_MAB_RETRY_BYTE5_INTERNAL_STORE_BOUNDARY_BASE_OFFSET;
    const uintptr_t guard_before_phys = page_phys;
    const uintptr_t guard_after_phys = page_phys + 24U;
    const uintptr_t fault_vaddr = page_vaddr +
        AI_MICRO_MAB_RETRY_BYTE5_INTERNAL_STORE_CROSS_16B_SD_OFFSET;
    const uintptr_t second_half_vaddr = page_vaddr + 16U;
    const uintptr_t aligned_upper_store_vaddr = page_vaddr +
        AI_MICRO_MAB_RETRY_BYTE5_INTERNAL_STORE_BOUNDARY_BASE_OFFSET + 8U;
    const uintptr_t followup_head_side_semantic_word_store_vaddr =
        aligned_upper_store_vaddr;
    const uintptr_t deny_start = boundary_phys + 8U;
    const uintptr_t deny_end = deny_start + PAGE_SIZE;
    const uint64_t first_fault_store_value = 0x30415263748596a7ULL;
    const uint64_t refault_store_value = 0xb8c9daebfc0d1e2fULL;
    const uint64_t aligned_upper_store_value_first = 0x2031425364758697ULL;
    const uint64_t aligned_upper_store_value_second = 0xa8b9cadbecfd0e1fULL;
    const uint64_t aligned_upper_store_value_third = 0x31425364758697a8ULL;
    const uint64_t aligned_upper_store_value_fourth = 0xb9cadbecfd0e1f20ULL;
    const uint64_t retry_3b5b_store_value = 0x425364758697a8b9ULL;
    const uint32_t followup_head_side_semantic_word_store_value = 0x7c6b5a49U;
    uint8_t expected_boundary[16];

    goto_priv(PRIV_M);
    ai_micro_mab_retry_byte5_internal_store_reset_env();
    ai_micro_mab_retry_byte5_internal_store_write_bytes(
        boundary_phys, boundary_seed, 16U);
    ai_micro_mab_retry_byte5_internal_store_write_bytes(
        guard_before_phys, guard_before_seed, 8U);
    ai_micro_mab_retry_byte5_internal_store_write_bytes(
        guard_after_phys, guard_after_seed, 8U);

    ai_micro_mab_retry_byte5_internal_store_configure_pmp_window(
        deny_start, deny_end,
        AI_MICRO_MAB_RETRY_BYTE5_INTERNAL_STORE_PMP_TOR_NO_W);
    ai_micro_mab_retry_byte5_internal_store_prepare_supervisor_translation();
    ai_micro_mab_retry_byte5_internal_store_set_leaf(
        low_page, PTE_V | PTE_AD | PTE_RWX);
    ai_micro_mab_retry_byte5_internal_store_set_leaf(
        high_page, PTE_V | PTE_AD | PTE_RWX);
    sfence_vma();

    TEST_SETUP_EXCEPT();
    ai_micro_mab_retry_byte5_internal_store_translated_sd(
        fault_vaddr, first_fault_store_value);
    TEST_ASSERT("for the no-H same-page cross-16B translated scalar store, the first repeated 3B+5B second-split write fault must still report store access fault on the true second-half start address before the retried-success then head-side-semantic-word follow-up store probe",
        excpt.triggered == true &&
        excpt.cause == CAUSE_SAF &&
        excpt.tval == second_half_vaddr
    );

    TEST_SETUP_EXCEPT();
    ai_micro_mab_retry_byte5_internal_store_translated_sd(
        fault_vaddr, first_fault_store_value);
    TEST_ASSERT("repeating the same no-H second-split 3B+5B write fault keeps the same precise metadata before the retried-success then head-side-semantic-word follow-up store probe",
        excpt.triggered == true &&
        excpt.cause == CAUSE_SAF &&
        excpt.tval == second_half_vaddr
    );

    ai_micro_mab_retry_byte5_internal_store_restore_pmp_full_access();

    TEST_SETUP_EXCEPT();
    ai_micro_mab_retry_byte5_internal_store_translated_sd(
        aligned_upper_store_vaddr, aligned_upper_store_value_first);
    TEST_ASSERT("after repair, the first same-boundary translated upper-half aligned store should complete cleanly before the refaulted retry-3B+5B-store then head-side-semantic-word follow-up store probe",
        excpt.triggered == false
    );

    TEST_SETUP_EXCEPT();
    ai_micro_mab_retry_byte5_internal_store_translated_sd(
        aligned_upper_store_vaddr, aligned_upper_store_value_second);
    TEST_ASSERT("after one upper-half aligned translated store has already completed on the repaired boundary, the second upper-half aligned translated store should stay trap-free before the refaulted retry-3B+5B-store then head-side-semantic-word follow-up store probe",
        excpt.triggered == false
    );

    TEST_SETUP_EXCEPT();
    ai_micro_mab_retry_byte5_internal_store_translated_sd(
        aligned_upper_store_vaddr, aligned_upper_store_value_third);
    TEST_ASSERT("after two upper-half aligned translated stores have already completed on the repaired boundary, the third upper-half aligned translated store should stay trap-free before the refaulted retry-3B+5B-store then head-side-semantic-word follow-up store probe",
        excpt.triggered == false
    );

    TEST_SETUP_EXCEPT();
    ai_micro_mab_retry_byte5_internal_store_translated_sd(
        aligned_upper_store_vaddr, aligned_upper_store_value_fourth);
    TEST_ASSERT("after three upper-half aligned translated stores have already completed on the repaired boundary, the fourth upper-half aligned translated store should stay trap-free and become the latest committed high-half payload before the refaulted retry-3B+5B-store then head-side-semantic-word follow-up store probe",
        excpt.triggered == false
    );

    ai_micro_mab_retry_byte5_internal_store_configure_pmp_window(
        deny_start, deny_end,
        AI_MICRO_MAB_RETRY_BYTE5_INTERNAL_STORE_PMP_TOR_NO_W);

    TEST_SETUP_EXCEPT();
    ai_micro_mab_retry_byte5_internal_store_translated_sd(
        fault_vaddr, refault_store_value);
    TEST_ASSERT("after four upper-half aligned translated stores have already completed on the repaired boundary, restoring the upper-half deny window and reissuing the original 3B+5B misaligned store should refresh the same precise second-split store access fault metadata before the head-side-semantic-word follow-up store probe",
        excpt.triggered == true &&
        excpt.cause == CAUSE_SAF &&
        excpt.tval == second_half_vaddr
    );

    ai_micro_mab_retry_byte5_internal_store_restore_pmp_full_access();

    TEST_SETUP_EXCEPT();
    ai_micro_mab_retry_byte5_internal_store_translated_sd(
        fault_vaddr, retry_3b5b_store_value);
    TEST_ASSERT("after the refault episode is repaired again, retrying the same 3B+5B translated store as a success store should clear stale exception metadata before the immediate head-side-semantic-word follow-up store",
        excpt.triggered == false
    );

    TEST_SETUP_EXCEPT();
    ai_micro_mab_retry_byte5_internal_store_translated_sw(
        followup_head_side_semantic_word_store_vaddr,
        followup_head_side_semantic_word_store_value);
    TEST_ASSERT("after the retried original-template 3B+5B store has already succeeded on the same boundary, the immediate translated head-side-semantic-word store should also stay trap-free and must not revive the stale fake-crosspage 8-byte high-half window beyond the semantic BYTE5 range",
        excpt.triggered == false
    );

    ai_micro_mab_retry_byte5_internal_store_overlay_little_endian_bytes(
        expected_boundary, boundary_seed, 16U, 8U,
        aligned_upper_store_value_fourth, sizeof(uint64_t));
    ai_micro_mab_retry_byte5_internal_store_overlay_little_endian_bytes(
        expected_boundary, expected_boundary, 16U, 5U,
        retry_3b5b_store_value, sizeof(uint64_t));
    ai_micro_mab_retry_byte5_internal_store_overlay_little_endian_bytes(
        expected_boundary, expected_boundary, 16U, 8U,
        (uint64_t)followup_head_side_semantic_word_store_value,
        sizeof(uint32_t));

    TEST_ASSERT("after the prolonged upper-half producer, repaired refault, retried 3B+5B success store, and immediate head-side-semantic-word store, the final boundary bytes should preserve bytes5-7 from the retried image, refresh only bytes8-11, preserve byte12 from the retried image, and keep bytes13-15 from the latest aligned upper-half image",
        ai_micro_mab_retry_byte5_internal_store_bytes_match(
            boundary_phys, expected_boundary, 16U)
    );

    TEST_ASSERT("guard regions around the same-page boundary should remain unchanged through the repeated 3B+5B fault, four upper-half aligned stores, refault, retried success store, and immediate head-side-semantic-word follow-up store sequence",
        ai_micro_mab_retry_byte5_internal_store_bytes_match(
            guard_before_phys, guard_before_seed, 8U) &&
        ai_micro_mab_retry_byte5_internal_store_bytes_match(
            guard_after_phys, guard_after_seed, 8U)
    );

    ai_micro_mab_retry_byte5_internal_store_reset_env();

    TEST_END(
        "ai_micro_m_samepage_cross16b_store_refault_upperx4_retry_3b5b_store_then_head_side_semantic_word_store_corner");
}

bool ai_micro_m_samepage_cross16b_store_refault_upperx4_retry_3b5b_store_then_head_middle_bridge_halfword_store_corner()
{
    TEST_START();

    static const uint8_t boundary_seed[16] = {
        0x64, 0x53, 0x42, 0x31, 0x20, 0x1f, 0x0e, 0xfd,
        0xec, 0xdb, 0xca, 0xb9, 0xa8, 0x97, 0x86, 0x75,
    };
    static const uint8_t guard_before_seed[8] = {
        0x17, 0x29, 0x3b, 0x4d, 0x5f, 0x61, 0x73, 0x85,
    };
    static const uint8_t guard_after_seed[8] = {
        0xcc, 0xde, 0xf0, 0x02, 0x14, 0x26, 0x38, 0x4a,
    };
    const enum test_page low_page = SWITCH1;
    const enum test_page high_page = SWITCH2;
    const uintptr_t page_phys = phys_page_base(low_page) + PAGE_SIZE - 16U;
    const uintptr_t page_vaddr = hs_page_base(low_page) + PAGE_SIZE - 16U;
    const uintptr_t boundary_phys = page_phys +
        AI_MICRO_MAB_RETRY_BYTE5_INTERNAL_STORE_BOUNDARY_BASE_OFFSET;
    const uintptr_t guard_before_phys = page_phys;
    const uintptr_t guard_after_phys = page_phys + 24U;
    const uintptr_t fault_vaddr = page_vaddr +
        AI_MICRO_MAB_RETRY_BYTE5_INTERNAL_STORE_CROSS_16B_SD_OFFSET;
    const uintptr_t second_half_vaddr = page_vaddr + 16U;
    const uintptr_t aligned_upper_store_vaddr = page_vaddr +
        AI_MICRO_MAB_RETRY_BYTE5_INTERNAL_STORE_BOUNDARY_BASE_OFFSET + 8U;
    const uintptr_t followup_head_middle_bridge_halfword_store_vaddr =
        aligned_upper_store_vaddr + 1U;
    const uintptr_t deny_start = boundary_phys + 8U;
    const uintptr_t deny_end = deny_start + PAGE_SIZE;
    const uint64_t first_fault_store_value = 0x2132435465768798ULL;
    const uint64_t refault_store_value = 0xa9bacbdcedfe0f10ULL;
    const uint64_t aligned_upper_store_value_first = 0x1021324354657687ULL;
    const uint64_t aligned_upper_store_value_second = 0x98a9bacbdcedfe0fULL;
    const uint64_t aligned_upper_store_value_third = 0x2132435465768798ULL;
    const uint64_t aligned_upper_store_value_fourth = 0xa9bacbdcedfe0f10ULL;
    const uint64_t retry_3b5b_store_value = 0x32435465768798a9ULL;
    const uint16_t followup_head_middle_bridge_halfword_store_value = 0x6f5eU;
    uint8_t expected_boundary[16];

    goto_priv(PRIV_M);
    ai_micro_mab_retry_byte5_internal_store_reset_env();
    ai_micro_mab_retry_byte5_internal_store_write_bytes(
        boundary_phys, boundary_seed, 16U);
    ai_micro_mab_retry_byte5_internal_store_write_bytes(
        guard_before_phys, guard_before_seed, 8U);
    ai_micro_mab_retry_byte5_internal_store_write_bytes(
        guard_after_phys, guard_after_seed, 8U);

    ai_micro_mab_retry_byte5_internal_store_configure_pmp_window(
        deny_start, deny_end,
        AI_MICRO_MAB_RETRY_BYTE5_INTERNAL_STORE_PMP_TOR_NO_W);
    ai_micro_mab_retry_byte5_internal_store_prepare_supervisor_translation();
    ai_micro_mab_retry_byte5_internal_store_set_leaf(
        low_page, PTE_V | PTE_AD | PTE_RWX);
    ai_micro_mab_retry_byte5_internal_store_set_leaf(
        high_page, PTE_V | PTE_AD | PTE_RWX);
    sfence_vma();

    TEST_SETUP_EXCEPT();
    ai_micro_mab_retry_byte5_internal_store_translated_sd(
        fault_vaddr, first_fault_store_value);
    TEST_ASSERT("for the no-H same-page cross-16B translated scalar store, the first repeated 3B+5B second-split write fault must still report store access fault on the true second-half start address before the retried-success then head-middle-bridge-halfword follow-up store probe",
        excpt.triggered == true &&
        excpt.cause == CAUSE_SAF &&
        excpt.tval == second_half_vaddr
    );

    TEST_SETUP_EXCEPT();
    ai_micro_mab_retry_byte5_internal_store_translated_sd(
        fault_vaddr, first_fault_store_value);
    TEST_ASSERT("repeating the same no-H second-split 3B+5B write fault keeps the same precise metadata before the retried-success then head-middle-bridge-halfword follow-up store probe",
        excpt.triggered == true &&
        excpt.cause == CAUSE_SAF &&
        excpt.tval == second_half_vaddr
    );

    ai_micro_mab_retry_byte5_internal_store_restore_pmp_full_access();

    TEST_SETUP_EXCEPT();
    ai_micro_mab_retry_byte5_internal_store_translated_sd(
        aligned_upper_store_vaddr, aligned_upper_store_value_first);
    TEST_ASSERT("after repair, the first same-boundary translated upper-half aligned store should complete cleanly before the refaulted retry-3B+5B-store then head-middle-bridge-halfword follow-up store probe",
        excpt.triggered == false
    );

    TEST_SETUP_EXCEPT();
    ai_micro_mab_retry_byte5_internal_store_translated_sd(
        aligned_upper_store_vaddr, aligned_upper_store_value_second);
    TEST_ASSERT("after one upper-half aligned translated store has already completed on the repaired boundary, the second upper-half aligned translated store should stay trap-free before the refaulted retry-3B+5B-store then head-middle-bridge-halfword follow-up store probe",
        excpt.triggered == false
    );

    TEST_SETUP_EXCEPT();
    ai_micro_mab_retry_byte5_internal_store_translated_sd(
        aligned_upper_store_vaddr, aligned_upper_store_value_third);
    TEST_ASSERT("after two upper-half aligned translated stores have already completed on the repaired boundary, the third upper-half aligned translated store should stay trap-free before the refaulted retry-3B+5B-store then head-middle-bridge-halfword follow-up store probe",
        excpt.triggered == false
    );

    TEST_SETUP_EXCEPT();
    ai_micro_mab_retry_byte5_internal_store_translated_sd(
        aligned_upper_store_vaddr, aligned_upper_store_value_fourth);
    TEST_ASSERT("after three upper-half aligned translated stores have already completed on the repaired boundary, the fourth upper-half aligned translated store should stay trap-free and become the latest committed high-half payload before the refaulted retry-3B+5B-store then head-middle-bridge-halfword follow-up store probe",
        excpt.triggered == false
    );

    ai_micro_mab_retry_byte5_internal_store_configure_pmp_window(
        deny_start, deny_end,
        AI_MICRO_MAB_RETRY_BYTE5_INTERNAL_STORE_PMP_TOR_NO_W);

    TEST_SETUP_EXCEPT();
    ai_micro_mab_retry_byte5_internal_store_translated_sd(
        fault_vaddr, refault_store_value);
    TEST_ASSERT("after four upper-half aligned translated stores have already completed on the repaired boundary, restoring the upper-half deny window and reissuing the original 3B+5B misaligned store should refresh the same precise second-split store access fault metadata before the head-middle-bridge-halfword follow-up store probe",
        excpt.triggered == true &&
        excpt.cause == CAUSE_SAF &&
        excpt.tval == second_half_vaddr
    );

    ai_micro_mab_retry_byte5_internal_store_restore_pmp_full_access();

    TEST_SETUP_EXCEPT();
    ai_micro_mab_retry_byte5_internal_store_translated_sd(
        fault_vaddr, retry_3b5b_store_value);
    TEST_ASSERT("after the refault episode is repaired again, retrying the same 3B+5B translated store as a success store should clear stale exception metadata before the immediate head-middle-bridge-halfword follow-up store",
        excpt.triggered == false
    );

    TEST_SETUP_EXCEPT();
    ai_micro_mab_retry_byte5_internal_store_translated_sh(
        followup_head_middle_bridge_halfword_store_vaddr,
        followup_head_middle_bridge_halfword_store_value);
    TEST_ASSERT("after the retried original-template 3B+5B store has already succeeded on the same boundary, the immediate translated head-middle-bridge-halfword store should also stay trap-free and must not revive the stale fake-crosspage 8-byte high-half window beyond the semantic BYTE5 range",
        excpt.triggered == false
    );

    ai_micro_mab_retry_byte5_internal_store_overlay_little_endian_bytes(
        expected_boundary, boundary_seed, 16U, 8U,
        aligned_upper_store_value_fourth, sizeof(uint64_t));
    ai_micro_mab_retry_byte5_internal_store_overlay_little_endian_bytes(
        expected_boundary, expected_boundary, 16U, 5U,
        retry_3b5b_store_value, sizeof(uint64_t));
    ai_micro_mab_retry_byte5_internal_store_overlay_little_endian_bytes(
        expected_boundary, expected_boundary, 16U, 9U,
        (uint64_t)followup_head_middle_bridge_halfword_store_value,
        sizeof(uint16_t));

    TEST_ASSERT("after the prolonged upper-half producer, repaired refault, retried 3B+5B success store, and immediate head-middle-bridge-halfword store, the final boundary bytes should preserve bytes5-8 from the retried image, refresh only bytes9-10, preserve bytes11-12 from the retried image, and keep bytes13-15 from the latest aligned upper-half image",
        ai_micro_mab_retry_byte5_internal_store_bytes_match(
            boundary_phys, expected_boundary, 16U)
    );

    TEST_ASSERT("guard regions around the same-page boundary should remain unchanged through the repeated 3B+5B fault, four upper-half aligned stores, refault, retried success store, and immediate head-middle-bridge-halfword follow-up store sequence",
        ai_micro_mab_retry_byte5_internal_store_bytes_match(
            guard_before_phys, guard_before_seed, 8U) &&
        ai_micro_mab_retry_byte5_internal_store_bytes_match(
            guard_after_phys, guard_after_seed, 8U)
    );

    ai_micro_mab_retry_byte5_internal_store_reset_env();

    TEST_END(
        "ai_micro_m_samepage_cross16b_store_refault_upperx4_retry_3b5b_store_then_head_middle_bridge_halfword_store_corner");
}

bool ai_micro_m_samepage_cross16b_store_refault_upperx4_retry_3b5b_store_then_first_upper_halfword_store_corner()
{
    TEST_START();

    static const uint8_t boundary_seed[16] = {
        0x55, 0x44, 0x33, 0x22, 0x11, 0x10, 0xff, 0xee,
        0xdd, 0xcc, 0xbb, 0xaa, 0x99, 0x88, 0x77, 0x66,
    };
    static const uint8_t guard_before_seed[8] = {
        0x15, 0x27, 0x39, 0x4b, 0x5d, 0x6f, 0x71, 0x83,
    };
    static const uint8_t guard_after_seed[8] = {
        0xca, 0xdc, 0xee, 0xf0, 0x12, 0x24, 0x36, 0x48,
    };
    const enum test_page low_page = SWITCH1;
    const enum test_page high_page = SWITCH2;
    const uintptr_t page_phys = phys_page_base(low_page) + PAGE_SIZE - 16U;
    const uintptr_t page_vaddr = hs_page_base(low_page) + PAGE_SIZE - 16U;
    const uintptr_t boundary_phys = page_phys +
        AI_MICRO_MAB_RETRY_BYTE5_INTERNAL_STORE_BOUNDARY_BASE_OFFSET;
    const uintptr_t guard_before_phys = page_phys;
    const uintptr_t guard_after_phys = page_phys + 24U;
    const uintptr_t fault_vaddr = page_vaddr +
        AI_MICRO_MAB_RETRY_BYTE5_INTERNAL_STORE_CROSS_16B_SD_OFFSET;
    const uintptr_t second_half_vaddr = page_vaddr + 16U;
    const uintptr_t aligned_upper_store_vaddr = page_vaddr +
        AI_MICRO_MAB_RETRY_BYTE5_INTERNAL_STORE_BOUNDARY_BASE_OFFSET + 8U;
    const uintptr_t followup_first_upper_halfword_store_vaddr =
        aligned_upper_store_vaddr;
    const uintptr_t deny_start = boundary_phys + 8U;
    const uintptr_t deny_end = deny_start + PAGE_SIZE;
    const uint64_t first_fault_store_value = 0x2031425364758697ULL;
    const uint64_t refault_store_value = 0xa8b9cadbecfd0e1fULL;
    const uint64_t aligned_upper_store_value_first = 0x1021324354657687ULL;
    const uint64_t aligned_upper_store_value_second = 0x98a9bacbdcedfe0fULL;
    const uint64_t aligned_upper_store_value_third = 0x2132435465768798ULL;
    const uint64_t aligned_upper_store_value_fourth = 0xa9bacbdcedfe0f10ULL;
    const uint64_t retry_3b5b_store_value = 0x31425364758697a8ULL;
    const uint16_t followup_first_upper_halfword_store_value = 0x6e5dU;
    uint8_t expected_boundary[16];

    goto_priv(PRIV_M);
    ai_micro_mab_retry_byte5_internal_store_reset_env();
    ai_micro_mab_retry_byte5_internal_store_write_bytes(
        boundary_phys, boundary_seed, 16U);
    ai_micro_mab_retry_byte5_internal_store_write_bytes(
        guard_before_phys, guard_before_seed, 8U);
    ai_micro_mab_retry_byte5_internal_store_write_bytes(
        guard_after_phys, guard_after_seed, 8U);

    ai_micro_mab_retry_byte5_internal_store_configure_pmp_window(
        deny_start, deny_end,
        AI_MICRO_MAB_RETRY_BYTE5_INTERNAL_STORE_PMP_TOR_NO_W);
    ai_micro_mab_retry_byte5_internal_store_prepare_supervisor_translation();
    ai_micro_mab_retry_byte5_internal_store_set_leaf(
        low_page, PTE_V | PTE_AD | PTE_RWX);
    ai_micro_mab_retry_byte5_internal_store_set_leaf(
        high_page, PTE_V | PTE_AD | PTE_RWX);
    sfence_vma();

    TEST_SETUP_EXCEPT();
    ai_micro_mab_retry_byte5_internal_store_translated_sd(
        fault_vaddr, first_fault_store_value);
    TEST_ASSERT("for the no-H same-page cross-16B translated scalar store, the first repeated 3B+5B second-split write fault must still report store access fault on the true second-half start address before the retried-success then first-upper-halfword follow-up store probe",
        excpt.triggered == true &&
        excpt.cause == CAUSE_SAF &&
        excpt.tval == second_half_vaddr
    );

    TEST_SETUP_EXCEPT();
    ai_micro_mab_retry_byte5_internal_store_translated_sd(
        fault_vaddr, first_fault_store_value);
    TEST_ASSERT("repeating the same no-H second-split 3B+5B write fault keeps the same precise metadata before the retried-success then first-upper-halfword follow-up store probe",
        excpt.triggered == true &&
        excpt.cause == CAUSE_SAF &&
        excpt.tval == second_half_vaddr
    );

    ai_micro_mab_retry_byte5_internal_store_restore_pmp_full_access();

    TEST_SETUP_EXCEPT();
    ai_micro_mab_retry_byte5_internal_store_translated_sd(
        aligned_upper_store_vaddr, aligned_upper_store_value_first);
    TEST_ASSERT("after repair, the first same-boundary translated upper-half aligned store should complete cleanly before the refaulted retry-3B+5B-store then first-upper-halfword follow-up store probe",
        excpt.triggered == false
    );

    TEST_SETUP_EXCEPT();
    ai_micro_mab_retry_byte5_internal_store_translated_sd(
        aligned_upper_store_vaddr, aligned_upper_store_value_second);
    TEST_ASSERT("after one upper-half aligned translated store has already completed on the repaired boundary, the second upper-half aligned translated store should stay trap-free before the refaulted retry-3B+5B-store then first-upper-halfword follow-up store probe",
        excpt.triggered == false
    );

    TEST_SETUP_EXCEPT();
    ai_micro_mab_retry_byte5_internal_store_translated_sd(
        aligned_upper_store_vaddr, aligned_upper_store_value_third);
    TEST_ASSERT("after two upper-half aligned translated stores have already completed on the repaired boundary, the third upper-half aligned translated store should stay trap-free before the refaulted retry-3B+5B-store then first-upper-halfword follow-up store probe",
        excpt.triggered == false
    );

    TEST_SETUP_EXCEPT();
    ai_micro_mab_retry_byte5_internal_store_translated_sd(
        aligned_upper_store_vaddr, aligned_upper_store_value_fourth);
    TEST_ASSERT("after three upper-half aligned translated stores have already completed on the repaired boundary, the fourth upper-half aligned translated store should stay trap-free and become the latest committed high-half payload before the refaulted retry-3B+5B-store then first-upper-halfword follow-up store probe",
        excpt.triggered == false
    );

    ai_micro_mab_retry_byte5_internal_store_configure_pmp_window(
        deny_start, deny_end,
        AI_MICRO_MAB_RETRY_BYTE5_INTERNAL_STORE_PMP_TOR_NO_W);

    TEST_SETUP_EXCEPT();
    ai_micro_mab_retry_byte5_internal_store_translated_sd(
        fault_vaddr, refault_store_value);
    TEST_ASSERT("after four upper-half aligned translated stores have already completed on the repaired boundary, restoring the upper-half deny window and reissuing the original 3B+5B misaligned store should refresh the same precise second-split store access fault metadata before the first-upper-halfword follow-up store probe",
        excpt.triggered == true &&
        excpt.cause == CAUSE_SAF &&
        excpt.tval == second_half_vaddr
    );

    ai_micro_mab_retry_byte5_internal_store_restore_pmp_full_access();

    TEST_SETUP_EXCEPT();
    ai_micro_mab_retry_byte5_internal_store_translated_sd(
        fault_vaddr, retry_3b5b_store_value);
    TEST_ASSERT("after the refault episode is repaired again, retrying the same 3B+5B translated store as a success store should clear stale exception metadata before the immediate first-upper-halfword follow-up store",
        excpt.triggered == false
    );

    TEST_SETUP_EXCEPT();
    ai_micro_mab_retry_byte5_internal_store_translated_sh(
        followup_first_upper_halfword_store_vaddr,
        followup_first_upper_halfword_store_value);
    TEST_ASSERT("after the retried original-template 3B+5B store has already succeeded on the same boundary, the immediate translated first-upper-halfword store should also stay trap-free and must not revive the stale fake-crosspage 8-byte high-half head window beyond the semantic BYTE5 range",
        excpt.triggered == false
    );

    ai_micro_mab_retry_byte5_internal_store_overlay_little_endian_bytes(
        expected_boundary, boundary_seed, 16U, 8U,
        aligned_upper_store_value_fourth, sizeof(uint64_t));
    ai_micro_mab_retry_byte5_internal_store_overlay_little_endian_bytes(
        expected_boundary, expected_boundary, 16U, 5U,
        retry_3b5b_store_value, sizeof(uint64_t));
    ai_micro_mab_retry_byte5_internal_store_overlay_little_endian_bytes(
        expected_boundary, expected_boundary, 16U, 8U,
        (uint64_t)followup_first_upper_halfword_store_value,
        sizeof(uint16_t));

    TEST_ASSERT("after the prolonged upper-half producer, repaired refault, retried 3B+5B success store, and immediate first-upper-halfword store, the final boundary bytes should preserve bytes5-7 from the retried image, refresh only bytes8-9, preserve bytes10-12 from the retried image, and keep bytes13-15 from the latest aligned upper-half image",
        ai_micro_mab_retry_byte5_internal_store_bytes_match(
            boundary_phys, expected_boundary, 16U)
    );

    TEST_ASSERT("guard regions around the same-page boundary should remain unchanged through the repeated 3B+5B fault, four upper-half aligned stores, refault, retried success store, and immediate first-upper-halfword follow-up store sequence",
        ai_micro_mab_retry_byte5_internal_store_bytes_match(
            guard_before_phys, guard_before_seed, 8U) &&
        ai_micro_mab_retry_byte5_internal_store_bytes_match(
            guard_after_phys, guard_after_seed, 8U)
    );

    ai_micro_mab_retry_byte5_internal_store_reset_env();

    TEST_END(
        "ai_micro_m_samepage_cross16b_store_refault_upperx4_retry_3b5b_store_then_first_upper_halfword_store_corner");
}

bool ai_micro_m_samepage_cross16b_store_refault_upperx4_retry_3b5b_store_then_first_upper_byte_store_corner()
{
    TEST_START();

    static const uint8_t boundary_seed[16] = {
        0x7a, 0x69, 0x58, 0x47, 0x36, 0x25, 0x14, 0x03,
        0xf2, 0xe1, 0xd0, 0xcf, 0xbe, 0xad, 0x9c, 0x8b,
    };
    static const uint8_t guard_before_seed[8] = {
        0x12, 0x24, 0x36, 0x48, 0x5a, 0x6c, 0x7e, 0x80,
    };
    static const uint8_t guard_after_seed[8] = {
        0xcb, 0xdd, 0xef, 0xf1, 0x13, 0x25, 0x37, 0x49,
    };
    const enum test_page low_page = SWITCH1;
    const enum test_page high_page = SWITCH2;
    const uintptr_t page_phys = phys_page_base(low_page) + PAGE_SIZE - 16U;
    const uintptr_t page_vaddr = hs_page_base(low_page) + PAGE_SIZE - 16U;
    const uintptr_t boundary_phys = page_phys +
        AI_MICRO_MAB_RETRY_BYTE5_INTERNAL_STORE_BOUNDARY_BASE_OFFSET;
    const uintptr_t guard_before_phys = page_phys;
    const uintptr_t guard_after_phys = page_phys + 24U;
    const uintptr_t fault_vaddr = page_vaddr +
        AI_MICRO_MAB_RETRY_BYTE5_INTERNAL_STORE_CROSS_16B_SD_OFFSET;
    const uintptr_t second_half_vaddr = page_vaddr + 16U;
    const uintptr_t aligned_upper_store_vaddr = page_vaddr +
        AI_MICRO_MAB_RETRY_BYTE5_INTERNAL_STORE_BOUNDARY_BASE_OFFSET + 8U;
    const uintptr_t followup_first_upper_byte_store_vaddr =
        aligned_upper_store_vaddr;
    const uintptr_t deny_start = boundary_phys + 8U;
    const uintptr_t deny_end = deny_start + PAGE_SIZE;
    const uint64_t first_fault_store_value = 0x4a5b6c7d8e9fb0c1ULL;
    const uint64_t refault_store_value = 0xc9dafb0c1d2e3f40ULL;
    const uint64_t aligned_upper_store_value_first = 0x2233445566778899ULL;
    const uint64_t aligned_upper_store_value_second = 0xaabbccddeeff1021ULL;
    const uint64_t aligned_upper_store_value_third = 0x33445566778899aaULL;
    const uint64_t aligned_upper_store_value_fourth = 0xbbccddeeff102132ULL;
    const uint64_t retry_3b5b_store_value = 0x5c6d7e8fa0b1c2d3ULL;
    const uint8_t followup_first_upper_byte_store_value = 0x4eU;
    uint8_t expected_boundary[16];

    goto_priv(PRIV_M);
    ai_micro_mab_retry_byte5_internal_store_reset_env();
    ai_micro_mab_retry_byte5_internal_store_write_bytes(
        boundary_phys, boundary_seed, 16U);
    ai_micro_mab_retry_byte5_internal_store_write_bytes(
        guard_before_phys, guard_before_seed, 8U);
    ai_micro_mab_retry_byte5_internal_store_write_bytes(
        guard_after_phys, guard_after_seed, 8U);

    ai_micro_mab_retry_byte5_internal_store_configure_pmp_window(
        deny_start, deny_end,
        AI_MICRO_MAB_RETRY_BYTE5_INTERNAL_STORE_PMP_TOR_NO_W);
    ai_micro_mab_retry_byte5_internal_store_prepare_supervisor_translation();
    ai_micro_mab_retry_byte5_internal_store_set_leaf(
        low_page, PTE_V | PTE_AD | PTE_RWX);
    ai_micro_mab_retry_byte5_internal_store_set_leaf(
        high_page, PTE_V | PTE_AD | PTE_RWX);
    sfence_vma();

    TEST_SETUP_EXCEPT();
    ai_micro_mab_retry_byte5_internal_store_translated_sd(
        fault_vaddr, first_fault_store_value);
    TEST_ASSERT("for the no-H same-page cross-16B translated scalar store, the first repeated 3B+5B second-split write fault must still report store access fault on the true second-half start address before the retried-success then first-upper-byte follow-up store probe",
        excpt.triggered == true &&
        excpt.cause == CAUSE_SAF &&
        excpt.tval == second_half_vaddr
    );

    TEST_SETUP_EXCEPT();
    ai_micro_mab_retry_byte5_internal_store_translated_sd(
        fault_vaddr, first_fault_store_value);
    TEST_ASSERT("repeating the same no-H second-split 3B+5B write fault keeps the same precise metadata before the retried-success then first-upper-byte follow-up store probe",
        excpt.triggered == true &&
        excpt.cause == CAUSE_SAF &&
        excpt.tval == second_half_vaddr
    );

    ai_micro_mab_retry_byte5_internal_store_restore_pmp_full_access();

    TEST_SETUP_EXCEPT();
    ai_micro_mab_retry_byte5_internal_store_translated_sd(
        aligned_upper_store_vaddr, aligned_upper_store_value_first);
    TEST_ASSERT("after repair, the first same-boundary translated upper-half aligned store should complete cleanly before the refaulted retry-3B+5B-store then first-upper-byte follow-up store probe",
        excpt.triggered == false
    );

    TEST_SETUP_EXCEPT();
    ai_micro_mab_retry_byte5_internal_store_translated_sd(
        aligned_upper_store_vaddr, aligned_upper_store_value_second);
    TEST_ASSERT("after one upper-half aligned translated store has already completed on the repaired boundary, the second upper-half aligned translated store should stay trap-free before the refaulted retry-3B+5B-store then first-upper-byte follow-up store probe",
        excpt.triggered == false
    );

    TEST_SETUP_EXCEPT();
    ai_micro_mab_retry_byte5_internal_store_translated_sd(
        aligned_upper_store_vaddr, aligned_upper_store_value_third);
    TEST_ASSERT("after two upper-half aligned translated stores have already completed on the repaired boundary, the third upper-half aligned translated store should stay trap-free before the refaulted retry-3B+5B-store then first-upper-byte follow-up store probe",
        excpt.triggered == false
    );

    TEST_SETUP_EXCEPT();
    ai_micro_mab_retry_byte5_internal_store_translated_sd(
        aligned_upper_store_vaddr, aligned_upper_store_value_fourth);
    TEST_ASSERT("after three upper-half aligned translated stores have already completed on the repaired boundary, the fourth upper-half aligned translated store should stay trap-free and become the latest committed high-half payload before the refaulted retry-3B+5B-store then first-upper-byte follow-up store probe",
        excpt.triggered == false
    );

    ai_micro_mab_retry_byte5_internal_store_configure_pmp_window(
        deny_start, deny_end,
        AI_MICRO_MAB_RETRY_BYTE5_INTERNAL_STORE_PMP_TOR_NO_W);

    TEST_SETUP_EXCEPT();
    ai_micro_mab_retry_byte5_internal_store_translated_sd(
        fault_vaddr, refault_store_value);
    TEST_ASSERT("after four upper-half aligned translated stores have already completed on the repaired boundary, restoring the upper-half deny window and reissuing the original 3B+5B misaligned store should refresh the same precise second-split store access fault metadata before the first-upper-byte follow-up store probe",
        excpt.triggered == true &&
        excpt.cause == CAUSE_SAF &&
        excpt.tval == second_half_vaddr
    );

    ai_micro_mab_retry_byte5_internal_store_restore_pmp_full_access();

    TEST_SETUP_EXCEPT();
    ai_micro_mab_retry_byte5_internal_store_translated_sd(
        fault_vaddr, retry_3b5b_store_value);
    TEST_ASSERT("after the refault episode is repaired again, retrying the same 3B+5B translated store as a success store should clear stale exception metadata before the immediate first-upper-byte follow-up store",
        excpt.triggered == false
    );

    TEST_SETUP_EXCEPT();
    ai_micro_mab_retry_byte5_internal_store_translated_sb(
        followup_first_upper_byte_store_vaddr,
        followup_first_upper_byte_store_value);
    TEST_ASSERT("after the retried original-template 3B+5B store has already succeeded on the same boundary, the immediate translated first-upper-byte store should also stay trap-free and must not revive the stale fake-crosspage 8-byte high-half head window beyond the semantic BYTE5 range",
        excpt.triggered == false
    );

    ai_micro_mab_retry_byte5_internal_store_overlay_little_endian_bytes(
        expected_boundary, boundary_seed, 16U, 8U,
        aligned_upper_store_value_fourth, sizeof(uint64_t));
    ai_micro_mab_retry_byte5_internal_store_overlay_little_endian_bytes(
        expected_boundary, expected_boundary, 16U, 5U,
        retry_3b5b_store_value, sizeof(uint64_t));
    ai_micro_mab_retry_byte5_internal_store_overlay_little_endian_bytes(
        expected_boundary, expected_boundary, 16U, 8U,
        (uint64_t)followup_first_upper_byte_store_value, sizeof(uint8_t));

    TEST_ASSERT("after the prolonged upper-half producer, repaired refault, retried 3B+5B success store, and immediate first-upper-byte store, the final boundary bytes should preserve bytes5-7 from the retried image, refresh only byte8, preserve bytes9-12 from the retried image, and keep bytes13-15 from the latest aligned upper-half image",
        ai_micro_mab_retry_byte5_internal_store_bytes_match(
            boundary_phys, expected_boundary, 16U)
    );

    TEST_ASSERT("guard regions around the same-page boundary should remain unchanged through the repeated 3B+5B fault, four upper-half aligned stores, refault, retried success store, and immediate first-upper-byte follow-up store sequence",
        ai_micro_mab_retry_byte5_internal_store_bytes_match(
            guard_before_phys, guard_before_seed, 8U) &&
        ai_micro_mab_retry_byte5_internal_store_bytes_match(
            guard_after_phys, guard_after_seed, 8U)
    );

    ai_micro_mab_retry_byte5_internal_store_reset_env();

    TEST_END(
        "ai_micro_m_samepage_cross16b_store_refault_upperx4_retry_3b5b_store_then_first_upper_byte_store_corner");
}

bool ai_micro_m_samepage_cross16b_store_refault_upperx4_retry_3b5b_store_then_second_upper_byte_store_corner()
{
    TEST_START();

    static const uint8_t boundary_seed[16] = {
        0x8f, 0x7e, 0x6d, 0x5c, 0x4b, 0x3a, 0x29, 0x18,
        0x07, 0xf6, 0xe5, 0xd4, 0xc3, 0xb2, 0xa1, 0x90,
    };
    static const uint8_t guard_before_seed[8] = {
        0x19, 0x2b, 0x3d, 0x4f, 0x61, 0x73, 0x85, 0x97,
    };
    static const uint8_t guard_after_seed[8] = {
        0xd4, 0xe6, 0xf8, 0x0a, 0x1c, 0x2e, 0x30, 0x42,
    };
    const enum test_page low_page = SWITCH1;
    const enum test_page high_page = SWITCH2;
    const uintptr_t page_phys = phys_page_base(low_page) + PAGE_SIZE - 16U;
    const uintptr_t page_vaddr = hs_page_base(low_page) + PAGE_SIZE - 16U;
    const uintptr_t boundary_phys = page_phys +
        AI_MICRO_MAB_RETRY_BYTE5_INTERNAL_STORE_BOUNDARY_BASE_OFFSET;
    const uintptr_t guard_before_phys = page_phys;
    const uintptr_t guard_after_phys = page_phys + 24U;
    const uintptr_t fault_vaddr = page_vaddr +
        AI_MICRO_MAB_RETRY_BYTE5_INTERNAL_STORE_CROSS_16B_SD_OFFSET;
    const uintptr_t second_half_vaddr = page_vaddr + 16U;
    const uintptr_t aligned_upper_store_vaddr = page_vaddr +
        AI_MICRO_MAB_RETRY_BYTE5_INTERNAL_STORE_BOUNDARY_BASE_OFFSET + 8U;
    const uintptr_t followup_second_upper_byte_store_vaddr =
        aligned_upper_store_vaddr + 1U;
    const uintptr_t deny_start = boundary_phys + 8U;
    const uintptr_t deny_end = deny_start + PAGE_SIZE;
    const uint64_t first_fault_store_value = 0x8798a9bacbdcedfeULL;
    const uint64_t refault_store_value = 0x0718293a4b5c6d7eULL;
    const uint64_t aligned_upper_store_value_first = 0x718293a4b5c6d7e8ULL;
    const uint64_t aligned_upper_store_value_second = 0xf90a1b2c3d4e5f60ULL;
    const uint64_t aligned_upper_store_value_third = 0x8293a4b5c6d7e8f9ULL;
    const uint64_t aligned_upper_store_value_fourth = 0x0a1b2c3d4e5f6071ULL;
    const uint64_t retry_3b5b_store_value = 0xb8c9daebfc0d1e2fULL;
    const uint8_t followup_second_upper_byte_store_value = 0x41U;
    uint8_t expected_boundary[16];

    goto_priv(PRIV_M);
    ai_micro_mab_retry_byte5_internal_store_reset_env();
    ai_micro_mab_retry_byte5_internal_store_write_bytes(
        boundary_phys, boundary_seed, 16U);
    ai_micro_mab_retry_byte5_internal_store_write_bytes(
        guard_before_phys, guard_before_seed, 8U);
    ai_micro_mab_retry_byte5_internal_store_write_bytes(
        guard_after_phys, guard_after_seed, 8U);

    ai_micro_mab_retry_byte5_internal_store_configure_pmp_window(
        deny_start, deny_end,
        AI_MICRO_MAB_RETRY_BYTE5_INTERNAL_STORE_PMP_TOR_NO_W);
    ai_micro_mab_retry_byte5_internal_store_prepare_supervisor_translation();
    ai_micro_mab_retry_byte5_internal_store_set_leaf(
        low_page, PTE_V | PTE_AD | PTE_RWX);
    ai_micro_mab_retry_byte5_internal_store_set_leaf(
        high_page, PTE_V | PTE_AD | PTE_RWX);
    sfence_vma();

    TEST_SETUP_EXCEPT();
    ai_micro_mab_retry_byte5_internal_store_translated_sd(
        fault_vaddr, first_fault_store_value);
    TEST_ASSERT("for the no-H same-page cross-16B translated scalar store, the first repeated 3B+5B second-split write fault must still report store access fault on the true second-half start address before the retried-success then second-upper-byte follow-up store probe",
        excpt.triggered == true &&
        excpt.cause == CAUSE_SAF &&
        excpt.tval == second_half_vaddr
    );

    TEST_SETUP_EXCEPT();
    ai_micro_mab_retry_byte5_internal_store_translated_sd(
        fault_vaddr, first_fault_store_value);
    TEST_ASSERT("repeating the same no-H second-split 3B+5B write fault keeps the same precise metadata before the retried-success then second-upper-byte follow-up store probe",
        excpt.triggered == true &&
        excpt.cause == CAUSE_SAF &&
        excpt.tval == second_half_vaddr
    );

    ai_micro_mab_retry_byte5_internal_store_restore_pmp_full_access();

    TEST_SETUP_EXCEPT();
    ai_micro_mab_retry_byte5_internal_store_translated_sd(
        aligned_upper_store_vaddr, aligned_upper_store_value_first);
    TEST_ASSERT("after repair, the first same-boundary translated upper-half aligned store should complete cleanly before the refaulted retry-3B+5B-store then second-upper-byte follow-up store probe",
        excpt.triggered == false
    );

    TEST_SETUP_EXCEPT();
    ai_micro_mab_retry_byte5_internal_store_translated_sd(
        aligned_upper_store_vaddr, aligned_upper_store_value_second);
    TEST_ASSERT("after one upper-half aligned translated store has already completed on the repaired boundary, the second upper-half aligned translated store should stay trap-free before the refaulted retry-3B+5B-store then second-upper-byte follow-up store probe",
        excpt.triggered == false
    );

    TEST_SETUP_EXCEPT();
    ai_micro_mab_retry_byte5_internal_store_translated_sd(
        aligned_upper_store_vaddr, aligned_upper_store_value_third);
    TEST_ASSERT("after two upper-half aligned translated stores have already completed on the repaired boundary, the third upper-half aligned translated store should stay trap-free before the refaulted retry-3B+5B-store then second-upper-byte follow-up store probe",
        excpt.triggered == false
    );

    TEST_SETUP_EXCEPT();
    ai_micro_mab_retry_byte5_internal_store_translated_sd(
        aligned_upper_store_vaddr, aligned_upper_store_value_fourth);
    TEST_ASSERT("after three upper-half aligned translated stores have already completed on the repaired boundary, the fourth upper-half aligned translated store should stay trap-free and become the latest committed high-half payload before the refaulted retry-3B+5B-store then second-upper-byte follow-up store probe",
        excpt.triggered == false
    );

    ai_micro_mab_retry_byte5_internal_store_configure_pmp_window(
        deny_start, deny_end,
        AI_MICRO_MAB_RETRY_BYTE5_INTERNAL_STORE_PMP_TOR_NO_W);

    TEST_SETUP_EXCEPT();
    ai_micro_mab_retry_byte5_internal_store_translated_sd(
        fault_vaddr, refault_store_value);
    TEST_ASSERT("after four upper-half aligned translated stores have already completed on the repaired boundary, restoring the upper-half deny window and reissuing the original 3B+5B misaligned store should refresh the same precise second-split store access fault metadata before the second-upper-byte follow-up store probe",
        excpt.triggered == true &&
        excpt.cause == CAUSE_SAF &&
        excpt.tval == second_half_vaddr
    );

    ai_micro_mab_retry_byte5_internal_store_restore_pmp_full_access();

    TEST_SETUP_EXCEPT();
    ai_micro_mab_retry_byte5_internal_store_translated_sd(
        fault_vaddr, retry_3b5b_store_value);
    TEST_ASSERT("after the refault episode is repaired again, retrying the same 3B+5B translated store as a success store should clear stale exception metadata before the immediate second-upper-byte follow-up store",
        excpt.triggered == false
    );

    TEST_SETUP_EXCEPT();
    ai_micro_mab_retry_byte5_internal_store_translated_sb(
        followup_second_upper_byte_store_vaddr,
        followup_second_upper_byte_store_value);
    TEST_ASSERT("after the retried original-template 3B+5B store has already succeeded on the same boundary, the immediate translated second-upper-byte store should also stay trap-free and must not revive the stale fake-crosspage 8-byte high-half head-side internal window beyond the semantic BYTE5 range",
        excpt.triggered == false
    );

    ai_micro_mab_retry_byte5_internal_store_overlay_little_endian_bytes(
        expected_boundary, boundary_seed, 16U, 8U,
        aligned_upper_store_value_fourth, sizeof(uint64_t));
    ai_micro_mab_retry_byte5_internal_store_overlay_little_endian_bytes(
        expected_boundary, expected_boundary, 16U, 5U,
        retry_3b5b_store_value, sizeof(uint64_t));
    ai_micro_mab_retry_byte5_internal_store_overlay_little_endian_bytes(
        expected_boundary, expected_boundary, 16U, 9U,
        (uint64_t)followup_second_upper_byte_store_value,
        sizeof(uint8_t));

    TEST_ASSERT("after the prolonged upper-half producer, repaired refault, retried 3B+5B success store, and immediate second-upper-byte store, the final boundary bytes should preserve bytes5-8 from the retried image, refresh only byte9, preserve bytes10-12 from the retried image, and keep bytes13-15 from the latest aligned upper-half image",
        ai_micro_mab_retry_byte5_internal_store_bytes_match(
            boundary_phys, expected_boundary, 16U)
    );

    TEST_ASSERT("guard regions around the same-page boundary should remain unchanged through the repeated 3B+5B fault, four upper-half aligned stores, refault, retried success store, and immediate second-upper-byte follow-up store sequence",
        ai_micro_mab_retry_byte5_internal_store_bytes_match(
            guard_before_phys, guard_before_seed, 8U) &&
        ai_micro_mab_retry_byte5_internal_store_bytes_match(
            guard_after_phys, guard_after_seed, 8U)
    );

    ai_micro_mab_retry_byte5_internal_store_reset_env();

    TEST_END(
        "ai_micro_m_samepage_cross16b_store_refault_upperx4_retry_3b5b_store_then_second_upper_byte_store_corner");
}

bool ai_micro_m_samepage_cross16b_store_refault_upperx4_retry_3b5b_store_then_center_semantic_byte_store_corner()
{
    TEST_START();

    static const uint8_t boundary_seed[16] = {
        0x74, 0x63, 0x52, 0x41, 0x30, 0x2f, 0x1e, 0x0d,
        0xfc, 0xeb, 0xda, 0xc9, 0xb8, 0xa7, 0x96, 0x85,
    };
    static const uint8_t guard_before_seed[8] = {
        0x1f, 0x21, 0x33, 0x45, 0x57, 0x69, 0x7b, 0x8d,
    };
    static const uint8_t guard_after_seed[8] = {
        0xce, 0xe0, 0xf2, 0x04, 0x16, 0x28, 0x3a, 0x4c,
    };
    const enum test_page low_page = SWITCH1;
    const enum test_page high_page = SWITCH2;
    const uintptr_t page_phys = phys_page_base(low_page) + PAGE_SIZE - 16U;
    const uintptr_t page_vaddr = hs_page_base(low_page) + PAGE_SIZE - 16U;
    const uintptr_t boundary_phys = page_phys +
        AI_MICRO_MAB_RETRY_BYTE5_INTERNAL_STORE_BOUNDARY_BASE_OFFSET;
    const uintptr_t guard_before_phys = page_phys;
    const uintptr_t guard_after_phys = page_phys + 24U;
    const uintptr_t fault_vaddr = page_vaddr +
        AI_MICRO_MAB_RETRY_BYTE5_INTERNAL_STORE_CROSS_16B_SD_OFFSET;
    const uintptr_t second_half_vaddr = page_vaddr + 16U;
    const uintptr_t aligned_upper_store_vaddr = page_vaddr +
        AI_MICRO_MAB_RETRY_BYTE5_INTERNAL_STORE_BOUNDARY_BASE_OFFSET + 8U;
    const uintptr_t followup_center_semantic_byte_store_vaddr =
        aligned_upper_store_vaddr + 2U;
    const uintptr_t deny_start = boundary_phys + 8U;
    const uintptr_t deny_end = deny_start + PAGE_SIZE;
    const uint64_t first_fault_store_value = 0x65768798a9bacbdcULL;
    const uint64_t refault_store_value = 0xe5f60718293a4b5cULL;
    const uint64_t aligned_upper_store_value_first = 0x4e5f60718293a4b5ULL;
    const uint64_t aligned_upper_store_value_second = 0xc6d7e8f90a1b2c3dULL;
    const uint64_t aligned_upper_store_value_third = 0x5f60718293a4b5c6ULL;
    const uint64_t aligned_upper_store_value_fourth = 0xd7e8f90a1b2c3d4eULL;
    const uint64_t retry_3b5b_store_value = 0x96a7b8c9daebfc0dULL;
    const uint8_t followup_center_semantic_byte_store_value = 0x2fU;
    uint8_t expected_boundary[16];

    goto_priv(PRIV_M);
    ai_micro_mab_retry_byte5_internal_store_reset_env();
    ai_micro_mab_retry_byte5_internal_store_write_bytes(
        boundary_phys, boundary_seed, 16U);
    ai_micro_mab_retry_byte5_internal_store_write_bytes(
        guard_before_phys, guard_before_seed, 8U);
    ai_micro_mab_retry_byte5_internal_store_write_bytes(
        guard_after_phys, guard_after_seed, 8U);

    ai_micro_mab_retry_byte5_internal_store_configure_pmp_window(
        deny_start, deny_end,
        AI_MICRO_MAB_RETRY_BYTE5_INTERNAL_STORE_PMP_TOR_NO_W);
    ai_micro_mab_retry_byte5_internal_store_prepare_supervisor_translation();
    ai_micro_mab_retry_byte5_internal_store_set_leaf(
        low_page, PTE_V | PTE_AD | PTE_RWX);
    ai_micro_mab_retry_byte5_internal_store_set_leaf(
        high_page, PTE_V | PTE_AD | PTE_RWX);
    sfence_vma();

    TEST_SETUP_EXCEPT();
    ai_micro_mab_retry_byte5_internal_store_translated_sd(
        fault_vaddr, first_fault_store_value);
    TEST_ASSERT("for the no-H same-page cross-16B translated scalar store, the first repeated 3B+5B second-split write fault must still report store access fault on the true second-half start address before the retried-success then center-semantic-byte follow-up store probe",
        excpt.triggered == true &&
        excpt.cause == CAUSE_SAF &&
        excpt.tval == second_half_vaddr
    );

    TEST_SETUP_EXCEPT();
    ai_micro_mab_retry_byte5_internal_store_translated_sd(
        fault_vaddr, first_fault_store_value);
    TEST_ASSERT("repeating the same no-H second-split 3B+5B write fault keeps the same precise metadata before the retried-success then center-semantic-byte follow-up store probe",
        excpt.triggered == true &&
        excpt.cause == CAUSE_SAF &&
        excpt.tval == second_half_vaddr
    );

    ai_micro_mab_retry_byte5_internal_store_restore_pmp_full_access();

    TEST_SETUP_EXCEPT();
    ai_micro_mab_retry_byte5_internal_store_translated_sd(
        aligned_upper_store_vaddr, aligned_upper_store_value_first);
    TEST_ASSERT("after repair, the first same-boundary translated upper-half aligned store should complete cleanly before the refaulted retry-3B+5B-store then center-semantic-byte follow-up store probe",
        excpt.triggered == false
    );

    TEST_SETUP_EXCEPT();
    ai_micro_mab_retry_byte5_internal_store_translated_sd(
        aligned_upper_store_vaddr, aligned_upper_store_value_second);
    TEST_ASSERT("after one upper-half aligned translated store has already completed on the repaired boundary, the second upper-half aligned translated store should stay trap-free before the refaulted retry-3B+5B-store then center-semantic-byte follow-up store probe",
        excpt.triggered == false
    );

    TEST_SETUP_EXCEPT();
    ai_micro_mab_retry_byte5_internal_store_translated_sd(
        aligned_upper_store_vaddr, aligned_upper_store_value_third);
    TEST_ASSERT("after two upper-half aligned translated stores have already completed on the repaired boundary, the third upper-half aligned translated store should stay trap-free before the refaulted retry-3B+5B-store then center-semantic-byte follow-up store probe",
        excpt.triggered == false
    );

    TEST_SETUP_EXCEPT();
    ai_micro_mab_retry_byte5_internal_store_translated_sd(
        aligned_upper_store_vaddr, aligned_upper_store_value_fourth);
    TEST_ASSERT("after three upper-half aligned translated stores have already completed on the repaired boundary, the fourth upper-half aligned translated store should stay trap-free and become the latest committed high-half payload before the refaulted retry-3B+5B-store then center-semantic-byte follow-up store probe",
        excpt.triggered == false
    );

    ai_micro_mab_retry_byte5_internal_store_configure_pmp_window(
        deny_start, deny_end,
        AI_MICRO_MAB_RETRY_BYTE5_INTERNAL_STORE_PMP_TOR_NO_W);

    TEST_SETUP_EXCEPT();
    ai_micro_mab_retry_byte5_internal_store_translated_sd(
        fault_vaddr, refault_store_value);
    TEST_ASSERT("after four upper-half aligned translated stores have already completed on the repaired boundary, restoring the upper-half deny window and reissuing the original 3B+5B misaligned store should refresh the same precise second-split store access fault metadata before the center-semantic-byte follow-up store probe",
        excpt.triggered == true &&
        excpt.cause == CAUSE_SAF &&
        excpt.tval == second_half_vaddr
    );

    ai_micro_mab_retry_byte5_internal_store_restore_pmp_full_access();

    TEST_SETUP_EXCEPT();
    ai_micro_mab_retry_byte5_internal_store_translated_sd(
        fault_vaddr, retry_3b5b_store_value);
    TEST_ASSERT("after the refault episode is repaired again, retrying the same 3B+5B translated store as a success store should clear stale exception metadata before the immediate center-semantic-byte follow-up store",
        excpt.triggered == false
    );

    TEST_SETUP_EXCEPT();
    ai_micro_mab_retry_byte5_internal_store_translated_sb(
        followup_center_semantic_byte_store_vaddr,
        followup_center_semantic_byte_store_value);
    TEST_ASSERT("after the retried original-template 3B+5B store has already succeeded on the same boundary, the immediate translated center-semantic-byte store should also stay trap-free and must not revive the stale fake-crosspage 8-byte high-half center window beyond the semantic BYTE5 range",
        excpt.triggered == false
    );

    ai_micro_mab_retry_byte5_internal_store_overlay_little_endian_bytes(
        expected_boundary, boundary_seed, 16U, 8U,
        aligned_upper_store_value_fourth, sizeof(uint64_t));
    ai_micro_mab_retry_byte5_internal_store_overlay_little_endian_bytes(
        expected_boundary, expected_boundary, 16U, 5U,
        retry_3b5b_store_value, sizeof(uint64_t));
    ai_micro_mab_retry_byte5_internal_store_overlay_little_endian_bytes(
        expected_boundary, expected_boundary, 16U, 10U,
        (uint64_t)followup_center_semantic_byte_store_value,
        sizeof(uint8_t));

    TEST_ASSERT("after the prolonged upper-half producer, repaired refault, retried 3B+5B success store, and immediate center-semantic-byte store, the final boundary bytes should preserve bytes5-9 from the retried image, refresh only byte10, preserve bytes11-12 from the retried image, and keep bytes13-15 from the latest aligned upper-half image",
        ai_micro_mab_retry_byte5_internal_store_bytes_match(
            boundary_phys, expected_boundary, 16U)
    );

    TEST_ASSERT("guard regions around the same-page boundary should remain unchanged through the repeated 3B+5B fault, four upper-half aligned stores, refault, retried success store, and immediate center-semantic-byte follow-up store sequence",
        ai_micro_mab_retry_byte5_internal_store_bytes_match(
            guard_before_phys, guard_before_seed, 8U) &&
        ai_micro_mab_retry_byte5_internal_store_bytes_match(
            guard_after_phys, guard_after_seed, 8U)
    );

    ai_micro_mab_retry_byte5_internal_store_reset_env();

    TEST_END(
        "ai_micro_m_samepage_cross16b_store_refault_upperx4_retry_3b5b_store_then_center_semantic_byte_store_corner");
}

bool ai_micro_m_samepage_cross16b_store_refault_upperx4_retry_3b5b_store_then_near_tail_semantic_byte_store_corner()
{
    TEST_START();

    static const uint8_t boundary_seed[16] = {
        0x85, 0x74, 0x63, 0x52, 0x41, 0x30, 0x2f, 0x1e,
        0x0d, 0xfc, 0xeb, 0xda, 0xc9, 0xb8, 0xa7, 0x96,
    };
    static const uint8_t guard_before_seed[8] = {
        0x22, 0x34, 0x46, 0x58, 0x6a, 0x7c, 0x8e, 0x90,
    };
    static const uint8_t guard_after_seed[8] = {
        0xd1, 0xe3, 0xf5, 0x07, 0x19, 0x2b, 0x3d, 0x4f,
    };
    const enum test_page low_page = SWITCH1;
    const enum test_page high_page = SWITCH2;
    const uintptr_t page_phys = phys_page_base(low_page) + PAGE_SIZE - 16U;
    const uintptr_t page_vaddr = hs_page_base(low_page) + PAGE_SIZE - 16U;
    const uintptr_t boundary_phys = page_phys +
        AI_MICRO_MAB_RETRY_BYTE5_INTERNAL_STORE_BOUNDARY_BASE_OFFSET;
    const uintptr_t guard_before_phys = page_phys;
    const uintptr_t guard_after_phys = page_phys + 24U;
    const uintptr_t fault_vaddr = page_vaddr +
        AI_MICRO_MAB_RETRY_BYTE5_INTERNAL_STORE_CROSS_16B_SD_OFFSET;
    const uintptr_t second_half_vaddr = page_vaddr + 16U;
    const uintptr_t aligned_upper_store_vaddr = page_vaddr +
        AI_MICRO_MAB_RETRY_BYTE5_INTERNAL_STORE_BOUNDARY_BASE_OFFSET + 8U;
    const uintptr_t followup_near_tail_semantic_byte_store_vaddr =
        aligned_upper_store_vaddr + 3U;
    const uintptr_t deny_start = boundary_phys + 8U;
    const uintptr_t deny_end = deny_start + PAGE_SIZE;
    const uint64_t first_fault_store_value = 0x768798a9bacbdcedULL;
    const uint64_t refault_store_value = 0xf60718293a4b5c6dULL;
    const uint64_t aligned_upper_store_value_first = 0x60718293a4b5c6d7ULL;
    const uint64_t aligned_upper_store_value_second = 0xe8f90a1b2c3d4e5fULL;
    const uint64_t aligned_upper_store_value_third = 0x718293a4b5c6d7e8ULL;
    const uint64_t aligned_upper_store_value_fourth = 0xf90a1b2c3d4e5f60ULL;
    const uint64_t retry_3b5b_store_value = 0xa7b8c9daebfc0d1eULL;
    const uint8_t followup_near_tail_semantic_byte_store_value = 0x1dU;
    uint8_t expected_boundary[16];

    goto_priv(PRIV_M);
    ai_micro_mab_retry_byte5_internal_store_reset_env();
    ai_micro_mab_retry_byte5_internal_store_write_bytes(
        boundary_phys, boundary_seed, 16U);
    ai_micro_mab_retry_byte5_internal_store_write_bytes(
        guard_before_phys, guard_before_seed, 8U);
    ai_micro_mab_retry_byte5_internal_store_write_bytes(
        guard_after_phys, guard_after_seed, 8U);

    ai_micro_mab_retry_byte5_internal_store_configure_pmp_window(
        deny_start, deny_end,
        AI_MICRO_MAB_RETRY_BYTE5_INTERNAL_STORE_PMP_TOR_NO_W);
    ai_micro_mab_retry_byte5_internal_store_prepare_supervisor_translation();
    ai_micro_mab_retry_byte5_internal_store_set_leaf(
        low_page, PTE_V | PTE_AD | PTE_RWX);
    ai_micro_mab_retry_byte5_internal_store_set_leaf(
        high_page, PTE_V | PTE_AD | PTE_RWX);
    sfence_vma();

    TEST_SETUP_EXCEPT();
    ai_micro_mab_retry_byte5_internal_store_translated_sd(
        fault_vaddr, first_fault_store_value);
    TEST_ASSERT("for the no-H same-page cross-16B translated scalar store, the first repeated 3B+5B second-split write fault must still report store access fault on the true second-half start address before the retried-success then near-tail-semantic-byte follow-up store probe",
        excpt.triggered == true &&
        excpt.cause == CAUSE_SAF &&
        excpt.tval == second_half_vaddr
    );

    TEST_SETUP_EXCEPT();
    ai_micro_mab_retry_byte5_internal_store_translated_sd(
        fault_vaddr, first_fault_store_value);
    TEST_ASSERT("repeating the same no-H second-split 3B+5B write fault keeps the same precise metadata before the retried-success then near-tail-semantic-byte follow-up store probe",
        excpt.triggered == true &&
        excpt.cause == CAUSE_SAF &&
        excpt.tval == second_half_vaddr
    );

    ai_micro_mab_retry_byte5_internal_store_restore_pmp_full_access();

    TEST_SETUP_EXCEPT();
    ai_micro_mab_retry_byte5_internal_store_translated_sd(
        aligned_upper_store_vaddr, aligned_upper_store_value_first);
    TEST_ASSERT("after repair, the first same-boundary translated upper-half aligned store should complete cleanly before the refaulted retry-3B+5B-store then near-tail-semantic-byte follow-up store probe",
        excpt.triggered == false
    );

    TEST_SETUP_EXCEPT();
    ai_micro_mab_retry_byte5_internal_store_translated_sd(
        aligned_upper_store_vaddr, aligned_upper_store_value_second);
    TEST_ASSERT("after one upper-half aligned translated store has already completed on the repaired boundary, the second upper-half aligned translated store should stay trap-free before the refaulted retry-3B+5B-store then near-tail-semantic-byte follow-up store probe",
        excpt.triggered == false
    );

    TEST_SETUP_EXCEPT();
    ai_micro_mab_retry_byte5_internal_store_translated_sd(
        aligned_upper_store_vaddr, aligned_upper_store_value_third);
    TEST_ASSERT("after two upper-half aligned translated stores have already completed on the repaired boundary, the third upper-half aligned translated store should stay trap-free before the refaulted retry-3B+5B-store then near-tail-semantic-byte follow-up store probe",
        excpt.triggered == false
    );

    TEST_SETUP_EXCEPT();
    ai_micro_mab_retry_byte5_internal_store_translated_sd(
        aligned_upper_store_vaddr, aligned_upper_store_value_fourth);
    TEST_ASSERT("after three upper-half aligned translated stores have already completed on the repaired boundary, the fourth upper-half aligned translated store should stay trap-free and become the latest committed high-half payload before the refaulted retry-3B+5B-store then near-tail-semantic-byte follow-up store probe",
        excpt.triggered == false
    );

    ai_micro_mab_retry_byte5_internal_store_configure_pmp_window(
        deny_start, deny_end,
        AI_MICRO_MAB_RETRY_BYTE5_INTERNAL_STORE_PMP_TOR_NO_W);

    TEST_SETUP_EXCEPT();
    ai_micro_mab_retry_byte5_internal_store_translated_sd(
        fault_vaddr, refault_store_value);
    TEST_ASSERT("after four upper-half aligned translated stores have already completed on the repaired boundary, restoring the upper-half deny window and reissuing the original 3B+5B misaligned store should refresh the same precise second-split store access fault metadata before the near-tail-semantic-byte follow-up store probe",
        excpt.triggered == true &&
        excpt.cause == CAUSE_SAF &&
        excpt.tval == second_half_vaddr
    );

    ai_micro_mab_retry_byte5_internal_store_restore_pmp_full_access();

    TEST_SETUP_EXCEPT();
    ai_micro_mab_retry_byte5_internal_store_translated_sd(
        fault_vaddr, retry_3b5b_store_value);
    TEST_ASSERT("after the refault episode is repaired again, retrying the same 3B+5B translated store as a success store should clear stale exception metadata before the immediate near-tail-semantic-byte follow-up store",
        excpt.triggered == false
    );

    TEST_SETUP_EXCEPT();
    ai_micro_mab_retry_byte5_internal_store_translated_sb(
        followup_near_tail_semantic_byte_store_vaddr,
        followup_near_tail_semantic_byte_store_value);
    TEST_ASSERT("after the retried original-template 3B+5B store has already succeeded on the same boundary, the immediate translated near-tail-semantic-byte store should also stay trap-free and must not revive the stale fake-crosspage 8-byte high-half window beyond the semantic BYTE5 range",
        excpt.triggered == false
    );

    ai_micro_mab_retry_byte5_internal_store_overlay_little_endian_bytes(
        expected_boundary, boundary_seed, 16U, 8U,
        aligned_upper_store_value_fourth, sizeof(uint64_t));
    ai_micro_mab_retry_byte5_internal_store_overlay_little_endian_bytes(
        expected_boundary, expected_boundary, 16U, 5U,
        retry_3b5b_store_value, sizeof(uint64_t));
    ai_micro_mab_retry_byte5_internal_store_overlay_little_endian_bytes(
        expected_boundary, expected_boundary, 16U, 11U,
        (uint64_t)followup_near_tail_semantic_byte_store_value,
        sizeof(uint8_t));

    TEST_ASSERT("after the prolonged upper-half producer, repaired refault, retried 3B+5B success store, and immediate near-tail-semantic-byte store, the final boundary bytes should preserve bytes5-10 from the retried image, refresh only byte11, preserve byte12 from the retried image, and keep bytes13-15 from the latest aligned upper-half image",
        ai_micro_mab_retry_byte5_internal_store_bytes_match(
            boundary_phys, expected_boundary, 16U)
    );

    TEST_ASSERT("guard regions around the same-page boundary should remain unchanged through the repeated 3B+5B fault, four upper-half aligned stores, refault, retried success store, and immediate near-tail-semantic-byte follow-up store sequence",
        ai_micro_mab_retry_byte5_internal_store_bytes_match(
            guard_before_phys, guard_before_seed, 8U) &&
        ai_micro_mab_retry_byte5_internal_store_bytes_match(
            guard_after_phys, guard_after_seed, 8U)
    );

    ai_micro_mab_retry_byte5_internal_store_reset_env();

    TEST_END(
        "ai_micro_m_samepage_cross16b_store_refault_upperx4_retry_3b5b_store_then_near_tail_semantic_byte_store_corner");
}
