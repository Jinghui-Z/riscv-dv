#include <rvh_test.h>
#include <page_tables.h>
#include <stdint.h>

/* Keep the byte15 / tail-halfword / tail-word follow-up stores separate from
 * the point7 byte-only probes so the upper-tail mask-edge scenarios can evolve
 * independently.
 */

#define AI_MICRO_MAB_RETRY_UPPER_TAIL_STORE_PMP_TOR_RWX 0x0fULL
#define AI_MICRO_MAB_RETRY_UPPER_TAIL_STORE_PMP_TOR_NO_W 0x09ULL

#define AI_MICRO_MAB_RETRY_UPPER_TAIL_STORE_BOUNDARY_BASE_OFFSET 8U
#define AI_MICRO_MAB_RETRY_UPPER_TAIL_STORE_CROSS_16B_SD_OFFSET 15U

static inline pte_t *ai_micro_mab_retry_upper_tail_store_hs_leaf_entry(
    enum test_page page)
{
#ifdef sv48
    return &hspt[4][page];
#else
    return &hspt[2][page];
#endif
}

static inline void ai_micro_mab_retry_upper_tail_store_set_leaf_paddr(
    enum test_page page, uintptr_t paddr, uint64_t flags)
{
    *ai_micro_mab_retry_upper_tail_store_hs_leaf_entry(page) =
        (paddr >> 2) | flags;
}

static inline void ai_micro_mab_retry_upper_tail_store_set_leaf(
    enum test_page page, uint64_t flags)
{
    ai_micro_mab_retry_upper_tail_store_set_leaf_paddr(
        page, phys_page_base(page), flags);
}

static inline void
ai_micro_mab_retry_upper_tail_store_prepare_supervisor_translation(void)
{
    goto_priv(PRIV_M);
    hspt_init();
    sfence_vma();
}

static inline void
ai_micro_mab_retry_upper_tail_store_restore_pmp_full_access(void)
{
    goto_priv(PRIV_M);
    CSRW(CSR_PMPCFG0, 0ULL);
    CSRW(CSR_PMPADDR0, (uint64_t)-1);
    CSRW(CSR_PMPADDR1, 0ULL);
    CSRW(CSR_PMPADDR2, 0ULL);
    CSRW(CSR_PMPCFG0, AI_MICRO_MAB_RETRY_UPPER_TAIL_STORE_PMP_TOR_RWX);
}

static inline void
ai_micro_mab_retry_upper_tail_store_clear_machine_access_mode(void)
{
    goto_priv(PRIV_M);
    CSRC(CSR_MSTATUS, MSTATUS_MPRV);
    CSRC(CSR_MSTATUS, 3ULL << 11);
    CSRW(CSR_SATP, 0ULL);
}

static inline void ai_micro_mab_retry_upper_tail_store_configure_pmp_window(
    uintptr_t deny_start, uintptr_t deny_end, uint64_t deny_cfg)
{
    goto_priv(PRIV_M);
    CSRW(CSR_PMPCFG0, 0ULL);
    CSRW(CSR_PMPADDR0, deny_start >> 2);
    CSRW(CSR_PMPADDR1, deny_end >> 2);
    CSRW(CSR_PMPADDR2, (uint64_t)-1);
    CSRW(CSR_PMPCFG0, AI_MICRO_MAB_RETRY_UPPER_TAIL_STORE_PMP_TOR_RWX |
        (deny_cfg << 8) |
        (AI_MICRO_MAB_RETRY_UPPER_TAIL_STORE_PMP_TOR_RWX << 16));
}

static inline void ai_micro_mab_retry_upper_tail_store_reset_env(void)
{
    ai_micro_mab_retry_upper_tail_store_restore_pmp_full_access();
    ai_micro_mab_retry_upper_tail_store_clear_machine_access_mode();
}

static inline void ai_micro_mab_retry_upper_tail_store_write_bytes(
    uintptr_t addr, const uint8_t *bytes, unsigned len)
{
    for (unsigned i = 0; i < len; ++i) {
        write8(addr + i, bytes[i]);
    }
}

static inline bool ai_micro_mab_retry_upper_tail_store_bytes_match(
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
ai_micro_mab_retry_upper_tail_store_overlay_little_endian_bytes(
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

#define AI_MICRO_MAB_RETRY_UPPER_TAIL_STORE_TRANSLATED_STORE_INSTRUCTION( \
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

AI_MICRO_MAB_RETRY_UPPER_TAIL_STORE_TRANSLATED_STORE_INSTRUCTION(
    ai_micro_mab_retry_upper_tail_store_translated_sd, "sd", uint64_t)
AI_MICRO_MAB_RETRY_UPPER_TAIL_STORE_TRANSLATED_STORE_INSTRUCTION(
    ai_micro_mab_retry_upper_tail_store_translated_sb, "sb", uint8_t)
AI_MICRO_MAB_RETRY_UPPER_TAIL_STORE_TRANSLATED_STORE_INSTRUCTION(
    ai_micro_mab_retry_upper_tail_store_translated_sh, "sh", uint16_t)
AI_MICRO_MAB_RETRY_UPPER_TAIL_STORE_TRANSLATED_STORE_INSTRUCTION(
    ai_micro_mab_retry_upper_tail_store_translated_sw, "sw", uint32_t)

bool ai_micro_m_samepage_cross16b_store_refault_upperx4_retry_store_then_upper_tail_last_halfword_high_byte_store_corner()
{
    TEST_START();

    static const uint8_t boundary_seed[16] = {
        0x93, 0x82, 0x71, 0x60, 0x5f, 0x4e, 0x3d, 0x2c,
        0x1b, 0x0a, 0xf9, 0xe8, 0xd7, 0xc6, 0xb5, 0xa4,
    };
    static const uint8_t guard_before_seed[8] = {
        0x18, 0x2a, 0x3c, 0x4e, 0x50, 0x62, 0x74, 0x86,
    };
    static const uint8_t guard_after_seed[8] = {
        0xc7, 0xd9, 0xeb, 0xfd, 0x0f, 0x11, 0x23, 0x35,
    };
    const enum test_page low_page = SWITCH1;
    const enum test_page high_page = SWITCH2;
    const uintptr_t page_phys = phys_page_base(low_page) + PAGE_SIZE - 16U;
    const uintptr_t page_vaddr = hs_page_base(low_page) + PAGE_SIZE - 16U;
    const uintptr_t boundary_phys =
        page_phys + AI_MICRO_MAB_RETRY_UPPER_TAIL_STORE_BOUNDARY_BASE_OFFSET;
    const uintptr_t guard_before_phys = page_phys;
    const uintptr_t guard_after_phys = page_phys + 24U;
    const uintptr_t fault_vaddr = page_vaddr +
        AI_MICRO_MAB_RETRY_UPPER_TAIL_STORE_CROSS_16B_SD_OFFSET;
    const uintptr_t second_half_vaddr = page_vaddr + 16U;
    const uintptr_t aligned_upper_store_vaddr = page_vaddr +
        AI_MICRO_MAB_RETRY_UPPER_TAIL_STORE_BOUNDARY_BASE_OFFSET + 8U;
    const uintptr_t followup_upper_tail_high_byte_store_vaddr =
        aligned_upper_store_vaddr + 7U;
    const uintptr_t deny_start = boundary_phys + 8U;
    const uintptr_t deny_end = deny_start + PAGE_SIZE;
    const uint64_t first_fault_store_value = 0x5162738495a6b7c8ULL;
    const uint64_t refault_store_value = 0xd1e2f30415263748ULL;
    const uint64_t aligned_upper_store_value_first = 0x2a3b4c5d6e7f8091ULL;
    const uint64_t aligned_upper_store_value_second = 0xa2b3c4d5e6f70819ULL;
    const uint64_t aligned_upper_store_value_third = 0x3b4c5d6e7f8091a2ULL;
    const uint64_t aligned_upper_store_value_fourth = 0xb3c4d5e6f708192aULL;
    const uint64_t retry_same_template_store_value = 0x748596a7b8c9daebULL;
    const uint8_t followup_upper_tail_high_byte_store_value = 0x6dU;
    uint8_t expected_boundary[16];

    goto_priv(PRIV_M);
    ai_micro_mab_retry_upper_tail_store_reset_env();
    ai_micro_mab_retry_upper_tail_store_write_bytes(
        boundary_phys, boundary_seed, 16U);
    ai_micro_mab_retry_upper_tail_store_write_bytes(
        guard_before_phys, guard_before_seed, 8U);
    ai_micro_mab_retry_upper_tail_store_write_bytes(
        guard_after_phys, guard_after_seed, 8U);

    ai_micro_mab_retry_upper_tail_store_configure_pmp_window(
        deny_start, deny_end, AI_MICRO_MAB_RETRY_UPPER_TAIL_STORE_PMP_TOR_NO_W);
    ai_micro_mab_retry_upper_tail_store_prepare_supervisor_translation();
    ai_micro_mab_retry_upper_tail_store_set_leaf(low_page, PTE_V | PTE_AD | PTE_RWX);
    ai_micro_mab_retry_upper_tail_store_set_leaf(high_page, PTE_V | PTE_AD | PTE_RWX);
    sfence_vma();

    TEST_SETUP_EXCEPT();
    ai_micro_mab_retry_upper_tail_store_translated_sd(
        fault_vaddr, first_fault_store_value);
    TEST_ASSERT("for the no-H same-page cross-16B translated scalar store, the first second-split write fault must report store access fault on the true second-half start address before the retried-success then upper-tail-last-halfword-high-byte follow-up store probe",
        excpt.triggered == true &&
        excpt.cause == CAUSE_SAF &&
        excpt.tval == second_half_vaddr
    );

    TEST_SETUP_EXCEPT();
    ai_micro_mab_retry_upper_tail_store_translated_sd(
        fault_vaddr, first_fault_store_value);
    TEST_ASSERT("repeating the same no-H second-split write fault keeps the same precise metadata before the retried-success then upper-tail-last-halfword-high-byte follow-up store probe",
        excpt.triggered == true &&
        excpt.cause == CAUSE_SAF &&
        excpt.tval == second_half_vaddr
    );

    ai_micro_mab_retry_upper_tail_store_restore_pmp_full_access();

    TEST_SETUP_EXCEPT();
    ai_micro_mab_retry_upper_tail_store_translated_sd(
        aligned_upper_store_vaddr, aligned_upper_store_value_first);
    TEST_ASSERT("after repair, the first same-boundary translated upper-half aligned store should complete cleanly before the refaulted retry-store then upper-tail-last-halfword-high-byte follow-up store probe",
        excpt.triggered == false
    );

    TEST_SETUP_EXCEPT();
    ai_micro_mab_retry_upper_tail_store_translated_sd(
        aligned_upper_store_vaddr, aligned_upper_store_value_second);
    TEST_ASSERT("after one upper-half aligned translated store has already completed on the repaired boundary, the second upper-half aligned translated store should stay trap-free before the refaulted retry-store then upper-tail-last-halfword-high-byte follow-up store probe",
        excpt.triggered == false
    );

    TEST_SETUP_EXCEPT();
    ai_micro_mab_retry_upper_tail_store_translated_sd(
        aligned_upper_store_vaddr, aligned_upper_store_value_third);
    TEST_ASSERT("after two upper-half aligned translated stores have already completed on the repaired boundary, the third upper-half aligned translated store should stay trap-free before the refaulted retry-store then upper-tail-last-halfword-high-byte follow-up store probe",
        excpt.triggered == false
    );

    TEST_SETUP_EXCEPT();
    ai_micro_mab_retry_upper_tail_store_translated_sd(
        aligned_upper_store_vaddr, aligned_upper_store_value_fourth);
    TEST_ASSERT("after three upper-half aligned translated stores have already completed on the repaired boundary, the fourth upper-half aligned translated store should stay trap-free and become the latest committed high-half payload before the refaulted retry-store then upper-tail-last-halfword-high-byte follow-up store probe",
        excpt.triggered == false
    );

    ai_micro_mab_retry_upper_tail_store_configure_pmp_window(
        deny_start, deny_end, AI_MICRO_MAB_RETRY_UPPER_TAIL_STORE_PMP_TOR_NO_W);

    TEST_SETUP_EXCEPT();
    ai_micro_mab_retry_upper_tail_store_translated_sd(
        fault_vaddr, refault_store_value);
    TEST_ASSERT("after four upper-half aligned translated stores have already completed on the repaired boundary, restoring the upper-half deny window and reissuing the original misaligned store should refresh the same precise second-split store access fault metadata before the upper-tail-last-halfword-high-byte follow-up store probe",
        excpt.triggered == true &&
        excpt.cause == CAUSE_SAF &&
        excpt.tval == second_half_vaddr
    );

    ai_micro_mab_retry_upper_tail_store_restore_pmp_full_access();

    TEST_SETUP_EXCEPT();
    ai_micro_mab_retry_upper_tail_store_translated_sd(
        fault_vaddr, retry_same_template_store_value);
    TEST_ASSERT("after the refault episode is repaired again, retrying the same 1B+7B translated store as a success store should clear stale exception metadata before the immediate upper-tail-last-halfword-high-byte follow-up store",
        excpt.triggered == false
    );

    TEST_SETUP_EXCEPT();
    ai_micro_mab_retry_upper_tail_store_translated_sb(
        followup_upper_tail_high_byte_store_vaddr,
        followup_upper_tail_high_byte_store_value);
    TEST_ASSERT("after the retried original-template 1B+7B store has already succeeded on the same boundary, the immediate upper-tail-last-halfword-high-byte translated store should also stay trap-free and must not revive the stale fake-crosspage 8-byte high-half mask edge",
        excpt.triggered == false
    );

    ai_micro_mab_retry_upper_tail_store_overlay_little_endian_bytes(
        expected_boundary, boundary_seed, 16U, 8U,
        aligned_upper_store_value_fourth, sizeof(uint64_t));
    ai_micro_mab_retry_upper_tail_store_overlay_little_endian_bytes(
        expected_boundary, expected_boundary, 16U, 7U,
        retry_same_template_store_value, sizeof(uint64_t));
    ai_micro_mab_retry_upper_tail_store_overlay_little_endian_bytes(
        expected_boundary, expected_boundary, 16U, 15U,
        (uint64_t)followup_upper_tail_high_byte_store_value, sizeof(uint8_t));

    TEST_ASSERT("after the prolonged upper-half producer, repaired refault, retried original-template success store, and immediate upper-tail-last-halfword-high-byte store, the final boundary bytes should preserve bytes7-14 from the retried image while refreshing only byte15",
        ai_micro_mab_retry_upper_tail_store_bytes_match(
            boundary_phys, expected_boundary, 16U)
    );

    TEST_ASSERT("guard regions around the same-page boundary should remain unchanged through the repeated fault, four upper-half aligned stores, refault, retried success store, and immediate upper-tail-last-halfword-high-byte follow-up store sequence",
        ai_micro_mab_retry_upper_tail_store_bytes_match(
            guard_before_phys, guard_before_seed, 8U) &&
        ai_micro_mab_retry_upper_tail_store_bytes_match(
            guard_after_phys, guard_after_seed, 8U)
    );

    ai_micro_mab_retry_upper_tail_store_reset_env();

    TEST_END(
        "ai_micro_m_samepage_cross16b_store_refault_upperx4_retry_store_then_upper_tail_last_halfword_high_byte_store_corner");
}

bool ai_micro_m_samepage_cross16b_store_refault_upperx4_retry_store_then_upper_tail_halfword_store_corner()
{
    TEST_START();

    static const uint8_t boundary_seed[16] = {
        0xa4, 0x93, 0x82, 0x71, 0x60, 0x5f, 0x4e, 0x3d,
        0x2c, 0x1b, 0x0a, 0xf9, 0xe8, 0xd7, 0xc6, 0xb5,
    };
    static const uint8_t guard_before_seed[8] = {
        0x19, 0x2b, 0x3d, 0x4f, 0x51, 0x63, 0x75, 0x87,
    };
    static const uint8_t guard_after_seed[8] = {
        0xd8, 0xea, 0xfc, 0x0e, 0x10, 0x22, 0x34, 0x46,
    };
    const enum test_page low_page = SWITCH1;
    const enum test_page high_page = SWITCH2;
    const uintptr_t page_phys = phys_page_base(low_page) + PAGE_SIZE - 16U;
    const uintptr_t page_vaddr = hs_page_base(low_page) + PAGE_SIZE - 16U;
    const uintptr_t boundary_phys =
        page_phys + AI_MICRO_MAB_RETRY_UPPER_TAIL_STORE_BOUNDARY_BASE_OFFSET;
    const uintptr_t guard_before_phys = page_phys;
    const uintptr_t guard_after_phys = page_phys + 24U;
    const uintptr_t fault_vaddr = page_vaddr +
        AI_MICRO_MAB_RETRY_UPPER_TAIL_STORE_CROSS_16B_SD_OFFSET;
    const uintptr_t second_half_vaddr = page_vaddr + 16U;
    const uintptr_t aligned_upper_store_vaddr = page_vaddr +
        AI_MICRO_MAB_RETRY_UPPER_TAIL_STORE_BOUNDARY_BASE_OFFSET + 8U;
    const uintptr_t followup_upper_tail_halfword_store_vaddr =
        aligned_upper_store_vaddr + 6U;
    const uintptr_t deny_start = boundary_phys + 8U;
    const uintptr_t deny_end = deny_start + PAGE_SIZE;
    const uint64_t first_fault_store_value = 0x62738495a6b7c8d9ULL;
    const uint64_t refault_store_value = 0xe2f3041526374859ULL;
    const uint64_t aligned_upper_store_value_first = 0x3c4d5e6f708192a3ULL;
    const uint64_t aligned_upper_store_value_second = 0xc4d5e6f708192a3bULL;
    const uint64_t aligned_upper_store_value_third = 0x4d5e6f708192a3b4ULL;
    const uint64_t aligned_upper_store_value_fourth = 0xd5e6f708192a3b4cULL;
    const uint64_t retry_same_template_store_value = 0x8596a7b8c9daebfcULL;
    const uint16_t followup_upper_tail_halfword_store_value = 0x7e6dU;
    uint8_t expected_boundary[16];

    goto_priv(PRIV_M);
    ai_micro_mab_retry_upper_tail_store_reset_env();
    ai_micro_mab_retry_upper_tail_store_write_bytes(
        boundary_phys, boundary_seed, 16U);
    ai_micro_mab_retry_upper_tail_store_write_bytes(
        guard_before_phys, guard_before_seed, 8U);
    ai_micro_mab_retry_upper_tail_store_write_bytes(
        guard_after_phys, guard_after_seed, 8U);

    ai_micro_mab_retry_upper_tail_store_configure_pmp_window(
        deny_start, deny_end, AI_MICRO_MAB_RETRY_UPPER_TAIL_STORE_PMP_TOR_NO_W);
    ai_micro_mab_retry_upper_tail_store_prepare_supervisor_translation();
    ai_micro_mab_retry_upper_tail_store_set_leaf(low_page, PTE_V | PTE_AD | PTE_RWX);
    ai_micro_mab_retry_upper_tail_store_set_leaf(high_page, PTE_V | PTE_AD | PTE_RWX);
    sfence_vma();

    TEST_SETUP_EXCEPT();
    ai_micro_mab_retry_upper_tail_store_translated_sd(
        fault_vaddr, first_fault_store_value);
    TEST_ASSERT("for the no-H same-page cross-16B translated scalar store, the first second-split write fault must report store access fault on the true second-half start address before the retried-success then upper-tail-halfword follow-up store probe",
        excpt.triggered == true &&
        excpt.cause == CAUSE_SAF &&
        excpt.tval == second_half_vaddr
    );

    TEST_SETUP_EXCEPT();
    ai_micro_mab_retry_upper_tail_store_translated_sd(
        fault_vaddr, first_fault_store_value);
    TEST_ASSERT("repeating the same no-H second-split write fault keeps the same precise metadata before the retried-success then upper-tail-halfword follow-up store probe",
        excpt.triggered == true &&
        excpt.cause == CAUSE_SAF &&
        excpt.tval == second_half_vaddr
    );

    ai_micro_mab_retry_upper_tail_store_restore_pmp_full_access();

    TEST_SETUP_EXCEPT();
    ai_micro_mab_retry_upper_tail_store_translated_sd(
        aligned_upper_store_vaddr, aligned_upper_store_value_first);
    TEST_ASSERT("after repair, the first same-boundary translated upper-half aligned store should complete cleanly before the refaulted retry-store then upper-tail-halfword follow-up store probe",
        excpt.triggered == false
    );

    TEST_SETUP_EXCEPT();
    ai_micro_mab_retry_upper_tail_store_translated_sd(
        aligned_upper_store_vaddr, aligned_upper_store_value_second);
    TEST_ASSERT("after one upper-half aligned translated store has already completed on the repaired boundary, the second upper-half aligned translated store should stay trap-free before the refaulted retry-store then upper-tail-halfword follow-up store probe",
        excpt.triggered == false
    );

    TEST_SETUP_EXCEPT();
    ai_micro_mab_retry_upper_tail_store_translated_sd(
        aligned_upper_store_vaddr, aligned_upper_store_value_third);
    TEST_ASSERT("after two upper-half aligned translated stores have already completed on the repaired boundary, the third upper-half aligned translated store should stay trap-free before the refaulted retry-store then upper-tail-halfword follow-up store probe",
        excpt.triggered == false
    );

    TEST_SETUP_EXCEPT();
    ai_micro_mab_retry_upper_tail_store_translated_sd(
        aligned_upper_store_vaddr, aligned_upper_store_value_fourth);
    TEST_ASSERT("after three upper-half aligned translated stores have already completed on the repaired boundary, the fourth upper-half aligned translated store should stay trap-free and become the latest committed high-half payload before the refaulted retry-store then upper-tail-halfword follow-up store probe",
        excpt.triggered == false
    );

    ai_micro_mab_retry_upper_tail_store_configure_pmp_window(
        deny_start, deny_end, AI_MICRO_MAB_RETRY_UPPER_TAIL_STORE_PMP_TOR_NO_W);

    TEST_SETUP_EXCEPT();
    ai_micro_mab_retry_upper_tail_store_translated_sd(
        fault_vaddr, refault_store_value);
    TEST_ASSERT("after four upper-half aligned translated stores have already completed on the repaired boundary, restoring the upper-half deny window and reissuing the original misaligned store should refresh the same precise second-split store access fault metadata before the upper-tail-halfword follow-up store probe",
        excpt.triggered == true &&
        excpt.cause == CAUSE_SAF &&
        excpt.tval == second_half_vaddr
    );

    ai_micro_mab_retry_upper_tail_store_restore_pmp_full_access();

    TEST_SETUP_EXCEPT();
    ai_micro_mab_retry_upper_tail_store_translated_sd(
        fault_vaddr, retry_same_template_store_value);
    TEST_ASSERT("after the refault episode is repaired again, retrying the same 1B+7B translated store as a success store should clear stale exception metadata before the immediate upper-tail-halfword follow-up store",
        excpt.triggered == false
    );

    TEST_SETUP_EXCEPT();
    ai_micro_mab_retry_upper_tail_store_translated_sh(
        followup_upper_tail_halfword_store_vaddr,
        followup_upper_tail_halfword_store_value);
    TEST_ASSERT("after the retried original-template 1B+7B store has already succeeded on the same boundary, the immediate upper-tail-halfword translated store should also stay trap-free and must not revive the stale fake-crosspage 8-byte high-half mask edge",
        excpt.triggered == false
    );

    ai_micro_mab_retry_upper_tail_store_overlay_little_endian_bytes(
        expected_boundary, boundary_seed, 16U, 8U,
        aligned_upper_store_value_fourth, sizeof(uint64_t));
    ai_micro_mab_retry_upper_tail_store_overlay_little_endian_bytes(
        expected_boundary, expected_boundary, 16U, 7U,
        retry_same_template_store_value, sizeof(uint64_t));
    ai_micro_mab_retry_upper_tail_store_overlay_little_endian_bytes(
        expected_boundary, expected_boundary, 16U, 14U,
        (uint64_t)followup_upper_tail_halfword_store_value, sizeof(uint16_t));

    TEST_ASSERT("after the prolonged upper-half producer, repaired refault, retried original-template success store, and immediate upper-tail-halfword store, the final boundary bytes should preserve bytes7-13 from the retried image while refreshing only bytes14-15",
        ai_micro_mab_retry_upper_tail_store_bytes_match(
            boundary_phys, expected_boundary, 16U)
    );

    TEST_ASSERT("guard regions around the same-page boundary should remain unchanged through the repeated fault, four upper-half aligned stores, refault, retried success store, and immediate upper-tail-halfword follow-up store sequence",
        ai_micro_mab_retry_upper_tail_store_bytes_match(
            guard_before_phys, guard_before_seed, 8U) &&
        ai_micro_mab_retry_upper_tail_store_bytes_match(
            guard_after_phys, guard_after_seed, 8U)
    );

    ai_micro_mab_retry_upper_tail_store_reset_env();

    TEST_END(
        "ai_micro_m_samepage_cross16b_store_refault_upperx4_retry_store_then_upper_tail_halfword_store_corner");
}

bool ai_micro_m_samepage_cross16b_store_refault_upperx4_retry_store_then_upper_tail_word_store_corner()
{
    TEST_START();

    static const uint8_t boundary_seed[16] = {
        0xb5, 0xa4, 0x93, 0x82, 0x71, 0x60, 0x5f, 0x4e,
        0x3d, 0x2c, 0x1b, 0x0a, 0xf9, 0xe8, 0xd7, 0xc6,
    };
    static const uint8_t guard_before_seed[8] = {
        0x1a, 0x2c, 0x3e, 0x40, 0x52, 0x64, 0x76, 0x88,
    };
    static const uint8_t guard_after_seed[8] = {
        0xe9, 0xfb, 0x0d, 0x1f, 0x21, 0x33, 0x45, 0x57,
    };
    const enum test_page low_page = SWITCH1;
    const enum test_page high_page = SWITCH2;
    const uintptr_t page_phys = phys_page_base(low_page) + PAGE_SIZE - 16U;
    const uintptr_t page_vaddr = hs_page_base(low_page) + PAGE_SIZE - 16U;
    const uintptr_t boundary_phys =
        page_phys + AI_MICRO_MAB_RETRY_UPPER_TAIL_STORE_BOUNDARY_BASE_OFFSET;
    const uintptr_t guard_before_phys = page_phys;
    const uintptr_t guard_after_phys = page_phys + 24U;
    const uintptr_t fault_vaddr = page_vaddr +
        AI_MICRO_MAB_RETRY_UPPER_TAIL_STORE_CROSS_16B_SD_OFFSET;
    const uintptr_t second_half_vaddr = page_vaddr + 16U;
    const uintptr_t aligned_upper_store_vaddr = page_vaddr +
        AI_MICRO_MAB_RETRY_UPPER_TAIL_STORE_BOUNDARY_BASE_OFFSET + 8U;
    const uintptr_t followup_upper_tail_word_store_vaddr =
        aligned_upper_store_vaddr + 4U;
    const uintptr_t deny_start = boundary_phys + 8U;
    const uintptr_t deny_end = deny_start + PAGE_SIZE;
    const uint64_t first_fault_store_value = 0x738495a6b7c8d9eaULL;
    const uint64_t refault_store_value = 0xf30415263748596aULL;
    const uint64_t aligned_upper_store_value_first = 0x4e5f708192a3b4c5ULL;
    const uint64_t aligned_upper_store_value_second = 0xd6e7f8091a2b3c4dULL;
    const uint64_t aligned_upper_store_value_third = 0x5f708192a3b4c5d6ULL;
    const uint64_t aligned_upper_store_value_fourth = 0xe7f8091a2b3c4d5eULL;
    const uint64_t retry_same_template_store_value = 0x96a7b8c9daebfc0dULL;
    const uint32_t followup_upper_tail_word_store_value = 0x8f7e6d5cU;
    uint8_t expected_boundary[16];

    goto_priv(PRIV_M);
    ai_micro_mab_retry_upper_tail_store_reset_env();
    ai_micro_mab_retry_upper_tail_store_write_bytes(
        boundary_phys, boundary_seed, 16U);
    ai_micro_mab_retry_upper_tail_store_write_bytes(
        guard_before_phys, guard_before_seed, 8U);
    ai_micro_mab_retry_upper_tail_store_write_bytes(
        guard_after_phys, guard_after_seed, 8U);

    ai_micro_mab_retry_upper_tail_store_configure_pmp_window(
        deny_start, deny_end, AI_MICRO_MAB_RETRY_UPPER_TAIL_STORE_PMP_TOR_NO_W);
    ai_micro_mab_retry_upper_tail_store_prepare_supervisor_translation();
    ai_micro_mab_retry_upper_tail_store_set_leaf(low_page, PTE_V | PTE_AD | PTE_RWX);
    ai_micro_mab_retry_upper_tail_store_set_leaf(high_page, PTE_V | PTE_AD | PTE_RWX);
    sfence_vma();

    TEST_SETUP_EXCEPT();
    ai_micro_mab_retry_upper_tail_store_translated_sd(
        fault_vaddr, first_fault_store_value);
    TEST_ASSERT("for the no-H same-page cross-16B translated scalar store, the first second-split write fault must report store access fault on the true second-half start address before the retried-success then upper-tail-word follow-up store probe",
        excpt.triggered == true &&
        excpt.cause == CAUSE_SAF &&
        excpt.tval == second_half_vaddr
    );

    TEST_SETUP_EXCEPT();
    ai_micro_mab_retry_upper_tail_store_translated_sd(
        fault_vaddr, first_fault_store_value);
    TEST_ASSERT("repeating the same no-H second-split write fault keeps the same precise metadata before the retried-success then upper-tail-word follow-up store probe",
        excpt.triggered == true &&
        excpt.cause == CAUSE_SAF &&
        excpt.tval == second_half_vaddr
    );

    ai_micro_mab_retry_upper_tail_store_restore_pmp_full_access();

    TEST_SETUP_EXCEPT();
    ai_micro_mab_retry_upper_tail_store_translated_sd(
        aligned_upper_store_vaddr, aligned_upper_store_value_first);
    TEST_ASSERT("after repair, the first same-boundary translated upper-half aligned store should complete cleanly before the refaulted retry-store then upper-tail-word follow-up store probe",
        excpt.triggered == false
    );

    TEST_SETUP_EXCEPT();
    ai_micro_mab_retry_upper_tail_store_translated_sd(
        aligned_upper_store_vaddr, aligned_upper_store_value_second);
    TEST_ASSERT("after one upper-half aligned translated store has already completed on the repaired boundary, the second upper-half aligned translated store should stay trap-free before the refaulted retry-store then upper-tail-word follow-up store probe",
        excpt.triggered == false
    );

    TEST_SETUP_EXCEPT();
    ai_micro_mab_retry_upper_tail_store_translated_sd(
        aligned_upper_store_vaddr, aligned_upper_store_value_third);
    TEST_ASSERT("after two upper-half aligned translated stores have already completed on the repaired boundary, the third upper-half aligned translated store should stay trap-free before the refaulted retry-store then upper-tail-word follow-up store probe",
        excpt.triggered == false
    );

    TEST_SETUP_EXCEPT();
    ai_micro_mab_retry_upper_tail_store_translated_sd(
        aligned_upper_store_vaddr, aligned_upper_store_value_fourth);
    TEST_ASSERT("after three upper-half aligned translated stores have already completed on the repaired boundary, the fourth upper-half aligned translated store should stay trap-free and become the latest committed high-half payload before the refaulted retry-store then upper-tail-word follow-up store probe",
        excpt.triggered == false
    );

    ai_micro_mab_retry_upper_tail_store_configure_pmp_window(
        deny_start, deny_end, AI_MICRO_MAB_RETRY_UPPER_TAIL_STORE_PMP_TOR_NO_W);

    TEST_SETUP_EXCEPT();
    ai_micro_mab_retry_upper_tail_store_translated_sd(
        fault_vaddr, refault_store_value);
    TEST_ASSERT("after four upper-half aligned translated stores have already completed on the repaired boundary, restoring the upper-half deny window and reissuing the original misaligned store should refresh the same precise second-split store access fault metadata before the upper-tail-word follow-up store probe",
        excpt.triggered == true &&
        excpt.cause == CAUSE_SAF &&
        excpt.tval == second_half_vaddr
    );

    ai_micro_mab_retry_upper_tail_store_restore_pmp_full_access();

    TEST_SETUP_EXCEPT();
    ai_micro_mab_retry_upper_tail_store_translated_sd(
        fault_vaddr, retry_same_template_store_value);
    TEST_ASSERT("after the refault episode is repaired again, retrying the same 1B+7B translated store as a success store should clear stale exception metadata before the immediate upper-tail-word follow-up store",
        excpt.triggered == false
    );

    TEST_SETUP_EXCEPT();
    ai_micro_mab_retry_upper_tail_store_translated_sw(
        followup_upper_tail_word_store_vaddr,
        followup_upper_tail_word_store_value);
    TEST_ASSERT("after the retried original-template 1B+7B store has already succeeded on the same boundary, the immediate upper-tail-word translated store should also stay trap-free and must not revive the stale fake-crosspage 8-byte high-half mask edge",
        excpt.triggered == false
    );

    ai_micro_mab_retry_upper_tail_store_overlay_little_endian_bytes(
        expected_boundary, boundary_seed, 16U, 8U,
        aligned_upper_store_value_fourth, sizeof(uint64_t));
    ai_micro_mab_retry_upper_tail_store_overlay_little_endian_bytes(
        expected_boundary, expected_boundary, 16U, 7U,
        retry_same_template_store_value, sizeof(uint64_t));
    ai_micro_mab_retry_upper_tail_store_overlay_little_endian_bytes(
        expected_boundary, expected_boundary, 16U, 12U,
        (uint64_t)followup_upper_tail_word_store_value, sizeof(uint32_t));

    TEST_ASSERT("after the prolonged upper-half producer, repaired refault, retried original-template success store, and immediate upper-tail-word store, the final boundary bytes should preserve bytes7-11 from the retried image while refreshing only bytes12-15",
        ai_micro_mab_retry_upper_tail_store_bytes_match(
            boundary_phys, expected_boundary, 16U)
    );

    TEST_ASSERT("guard regions around the same-page boundary should remain unchanged through the repeated fault, four upper-half aligned stores, refault, retried success store, and immediate upper-tail-word follow-up store sequence",
        ai_micro_mab_retry_upper_tail_store_bytes_match(
            guard_before_phys, guard_before_seed, 8U) &&
        ai_micro_mab_retry_upper_tail_store_bytes_match(
            guard_after_phys, guard_after_seed, 8U)
    );

    ai_micro_mab_retry_upper_tail_store_reset_env();

    TEST_END(
        "ai_micro_m_samepage_cross16b_store_refault_upperx4_retry_store_then_upper_tail_word_store_corner");
}
